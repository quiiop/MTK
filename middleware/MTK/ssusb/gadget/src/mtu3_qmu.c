/*
 * mtu3_qmu.c - Queue Management Unit driver for device controller
 *
 * Copyright (C) 2018 MediaTek Inc.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Queue Management Unit (QMU) is designed to unload SW effort
 * to serve DMA interrupts.
 * By preparing General Purpose Descriptor (GPD) and Buffer Descriptor (BD),
 * SW links data buffers and triggers QMU to send / receive data to
 * host / from device at a time.
 * And now only GPD is supported.
 *
 * For more detailed information, please refer to QMU Programming Guide
 */

#include "FreeRTOS.h"
#include <string.h>
#include <stdlib.h>

#include "mtu3_qmu.h"
#include "ssusb_hw_regs.h"

#define QMU_CHECKSUM_LEN    16

#define GPD_FLAGS_HWO   BIT(0)
#define GPD_FLAGS_BDP   BIT(1)
#define GPD_FLAGS_BPS   BIT(2)
#define GPD_FLAGS_IOC   BIT(7)

#define GPD_EXT_FLAG_ZLP    BIT(5)

#ifdef SUPPORT_QMU

static u32 va_to_pa(void *vaddr)
{
    return ((unsigned long)(vaddr));
}

static struct qmu_gpd *gpd_dma_to_virt(struct mtu3_gpd_ring *ring, u32 dma_addr)
{
    u32 dma_base = ring->dma;
    struct qmu_gpd *gpd_head = ring->start;
    u32 offset = (dma_addr - dma_base) / sizeof(*gpd_head);

    if (offset >= MAX_GPD_NUM)
        return NULL;

    return gpd_head + offset;
}

static u32 gpd_virt_to_dma(struct mtu3_gpd_ring *ring, struct qmu_gpd *gpd)
{
    u32 dma_base = ring->dma;
    struct qmu_gpd *gpd_head = ring->start;
    u32 offset;

    offset = gpd - gpd_head;
    if (offset >= MAX_GPD_NUM)
        return 0;

    return dma_base + (offset * sizeof(*gpd));
}

static void gpd_ring_init(struct mtu3_gpd_ring *ring, struct qmu_gpd *gpd)
{
    ring->start = gpd;
    ring->enqueue = gpd;
    ring->dequeue = gpd;
    ring->end = gpd + MAX_GPD_NUM - 1;
}

static void reset_gpd_list(struct udc_endpoint *mep)
{
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    struct qmu_gpd *gpd = ring->start;

    if (gpd) {
        gpd->flag &= ~GPD_FLAGS_HWO;
        gpd_ring_init(ring, gpd);
    }
}

int mtu3_gpd_ring_alloc(struct udc_endpoint *mep)
{
    struct qmu_gpd *gpd;
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    u32 size;

    /* software own all gpds as default */
    size = sizeof(struct qmu_gpd) * MAX_GPD_NUM;
    gpd = (struct qmu_gpd *)pvPortMallocNC(size);
    if (!gpd)
        return -ENOMEM;

    memset(gpd, 0, size);
    ring->dma = va_to_pa(gpd);
    gpd_ring_init(ring, gpd);
    return 0;
}

void mtu3_gpd_ring_free(struct udc_endpoint *mep)
{
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;

    vPortFreeNC(ring->start);
    memset(ring, 0, sizeof(*ring));
}

/*
 * calculate check sum of a gpd or bd
 * add "noinline" and "mb" to prevent wrong calculation
 */
static u8 qmu_calc_checksum(u8 *data)
{
    u8 chksum = 0;
    int i;

    data[1] = 0x0;  /* set checksum to 0 */

    mb();   /* ensure the gpd/bd is really up-to-date */
    for (i = 0; i < QMU_CHECKSUM_LEN; i++)
        chksum += data[i];

    /* Default: HWO=1, @flag[bit0] */
    chksum += 1;

    return 0xFF - chksum;
}

void mtu3_qmu_resume(struct udc_endpoint *mep)
{
    struct mu3d *u3d = mep->u3d;
    void *base = u3d->mac_base;
    int epnum = mep->num;
    u32 qcsr;

    qcsr = mep->in ? USB_QMU_TQCSR(epnum) : USB_QMU_RQCSR(epnum);

    ssusb_writel(base, qcsr, QMU_Q_RESUME);
    if (!(ssusb_readl(base, qcsr) & QMU_Q_ACTIVE))
        ssusb_writel(base, qcsr, QMU_Q_RESUME);
}

static struct qmu_gpd *advance_enq_gpd(struct mtu3_gpd_ring *ring)
{
    if (ring->enqueue < ring->end)
        ring->enqueue++;
    else
        ring->enqueue = ring->start;

    return ring->enqueue;
}

static struct qmu_gpd *advance_deq_gpd(struct mtu3_gpd_ring *ring)
{
    if (ring->dequeue < ring->end)
        ring->dequeue++;
    else
        ring->dequeue = ring->start;

    return ring->dequeue;
}

/* check if a ring is emtpy */
static int gpd_ring_empty(struct mtu3_gpd_ring *ring)
{
    struct qmu_gpd *enq = ring->enqueue;
    struct qmu_gpd *next;

    if (ring->enqueue < ring->end)
        next = enq + 1;
    else
        next = ring->start;

    /* one gpd is reserved to simplify gpd preparation */
    return next == ring->dequeue;
}

int mtu3_prepare_transfer(struct udc_endpoint *mep)
{
    int ret = gpd_ring_empty(&mep->gpd_ring);
    return ret;
}

static int mtu3_prepare_tx_gpd(struct udc_endpoint *mep, struct mu3d_req *mreq)
{
    struct qmu_gpd *enq;
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    struct qmu_gpd *gpd = ring->enqueue;
    struct udc_request *req = &mreq->req;

    /* set all fields to zero as default value */
    memset(gpd, 0, sizeof(*gpd));

    gpd->buffer = (u32)va_to_pa(req->buffer);
    gpd->buf_len = (req->length);
    gpd->flag |= GPD_FLAGS_IOC;

    if (req->zero)
        gpd->ext_flag |= GPD_EXT_FLAG_ZLP;

    /* get the next GPD */
    enq = advance_enq_gpd(ring);
    DBG_QMU("TX %s queue gpd=%p, enq=%p\n", mep->name, gpd, enq);

    enq->flag &= ~GPD_FLAGS_HWO;
    gpd->next_gpd = (u32)gpd_virt_to_dma(ring, enq);

    if (mep->type != USB_EP_XFER_ISO)
        gpd->ext_flag |= GPD_EXT_FLAG_ZLP;

    gpd->chksum = qmu_calc_checksum((u8 *)gpd);
    gpd->flag |= GPD_FLAGS_HWO;

    mreq->gpd = gpd;

    return 0;
}

static int mtu3_prepare_rx_gpd(struct udc_endpoint *mep, struct mu3d_req *mreq)
{
    struct qmu_gpd *enq;
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    struct qmu_gpd *gpd = ring->enqueue;
    struct udc_request *req = &mreq->req;

    /* set all fields to zero as default value */
    memset(gpd, 0, sizeof(*gpd));

    gpd->buffer = (u32)va_to_pa(req->buffer);
    gpd->data_buf_len = req->length;
    gpd->flag |= GPD_FLAGS_IOC;

    /* get the next GPD */
    enq = advance_enq_gpd(ring);
    DBG_QMU("RX %s queue gpd=%p, enq=%p\n", mep->name, gpd, enq);

    enq->flag &= ~GPD_FLAGS_HWO;
    gpd->next_gpd = (u32)gpd_virt_to_dma(ring, enq);
    gpd->chksum = qmu_calc_checksum((u8 *)gpd);
    gpd->flag |= GPD_FLAGS_HWO;

    mreq->gpd = gpd;

    return 0;
}

void mtu3_insert_gpd(struct udc_endpoint *mep, struct mu3d_req *mreq)
{
    if (mep->in)
        mtu3_prepare_tx_gpd(mep, mreq);
    else
        mtu3_prepare_rx_gpd(mep, mreq);
}

int mtu3_qmu_start(struct udc_endpoint *mep)
{
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    struct mu3d *u3d = mep->u3d;
    void *base = u3d->mac_base;
    u8 epnum = mep->num;

    if (mep->in) {
        /* set QMU start address */
        ssusb_writel(base, USB_QMU_TQSAR(mep->num), ring->dma);
        ssusb_setbits(base, MU3D_EP_TXCR0(mep->num), TX_DMAREQEN);
        ssusb_setbits(base, U3D_QCR0, QMU_TX_CS_EN(epnum));
        /* send zero length packet according to ZLP flag in GPD */
        ssusb_setbits(base, U3D_QCR1, QMU_TX_ZLP(epnum));
        ssusb_writel(base, U3D_TQERRIESR0, QMU_TX_LEN_ERR(epnum) | QMU_TX_CS_ERR(epnum));

        if (ssusb_readl(base, USB_QMU_TQCSR(epnum)) & QMU_Q_ACTIVE) {
            DBG_QMU("%s Active Now!\n", mep->name);
            return 0;
        }
        ssusb_writel(base, USB_QMU_TQCSR(epnum), QMU_Q_START);

    } else {
        ssusb_writel(base, USB_QMU_RQSAR(mep->num), ring->dma);
        ssusb_setbits(base, MU3D_EP_RXCR0(mep->num), RX_DMAREQEN);
        ssusb_setbits(base, U3D_QCR0, QMU_RX_CS_EN(epnum));
        /* don't expect ZLP */
        ssusb_clrbits(base, U3D_QCR3, QMU_RX_ZLP(epnum));
        /* move to next GPD when receive ZLP */
        ssusb_setbits(base, U3D_QCR3, QMU_RX_COZ(epnum));
        ssusb_writel(base, U3D_RQERRIESR0, QMU_RX_LEN_ERR(epnum) | QMU_RX_CS_ERR(epnum));
        ssusb_writel(base, U3D_RQERRIESR1, QMU_RX_ZLP_ERR(epnum));

        if (ssusb_readl(base, USB_QMU_RQCSR(epnum)) & QMU_Q_ACTIVE) {
            DBG_QMU("%s Active Now!\n", mep->name);
            return 0;
        }
        ssusb_writel(base, USB_QMU_RQCSR(epnum), QMU_Q_START);
    }
    DBG_QMU("%s's qmu start now!\n", mep->name);

    return 0;
}

/* may called in atomic context */
static void mtu3_qmu_stop(struct udc_endpoint *mep)
{
    struct mu3d *u3d = mep->u3d;
    void *base = u3d->mac_base;
    int epnum = mep->num;
    u32 qcsr;
    int ret;

    qcsr = mep->in ? USB_QMU_TQCSR(epnum) : USB_QMU_RQCSR(epnum);

    if (!(ssusb_readl(base, qcsr) & QMU_Q_ACTIVE)) {
        DBG_QMU("%s's qmu is inactive now!\n", mep->name);
        return;
    }

    ssusb_writel(base, qcsr, QMU_Q_STOP);
    ret = wait_for_value(base + qcsr, QMU_Q_ACTIVE, 0, 10, 100);
    if (ret) {
        DBG_QMU("stop %s's qmu failed\n", mep->name);
        return;
    }

    DBG_QMU("%s's qmu stop now!\n", mep->name);
}

void mtu3_qmu_flush(struct udc_endpoint *mep)
{
    DBG_QMU("%s flush QMU %s\n", __func__, mep->name);
    /*Stop QMU */
    mtu3_qmu_stop(mep);
    reset_gpd_list(mep);
}

static void qmu_done_tx(struct mu3d *u3d, u8 epnum)
{
    struct udc_endpoint *mep;
    struct mtu3_gpd_ring *ring;
    struct qmu_gpd *gpd;
    struct qmu_gpd *gpd_current;
    struct udc_request *request;
    struct mu3d_req *mreq;
    void *base = u3d->mac_base;
    u32 gpd_dma;

    mep = mtu3_find_ep(u3d, epnum, USB_DIR_IN);
    if (!mep || !mep->req)
        return;

    ring = &mep->gpd_ring;
    gpd = ring->dequeue;
    gpd_dma = ssusb_readl(base, USB_QMU_TQCPR(epnum));
    /*transfer phy address got from QMU register to virtual address */
    gpd_current = gpd_dma_to_virt(ring, gpd_dma);

    DBG_QMU("%s %s, last=%p, current=%p, enq=%p\n",
            __func__, mep->name, gpd, gpd_current, ring->enqueue);

    while (gpd != gpd_current && !(gpd->flag & GPD_FLAGS_HWO)) {
        request = mep->req;
        mreq = to_mu3d_req(request);
        if (!mreq || mreq->gpd != gpd) {
            DBG_QMU("no correct TX req is found\n");
            break;
        }

        mreq->actual = gpd->buf_len;
        handle_ept_complete(mep, 0);
        gpd = advance_deq_gpd(ring);
    }

    DBG_QMU("%s EP%dIN, deq=%p, enq=%p, complete\n",
            __func__, epnum, ring->dequeue, ring->enqueue);
}

static void qmu_done_rx(struct mu3d *u3d, u8 epnum)
{
    struct udc_endpoint *mep;
    struct mtu3_gpd_ring *ring;
    struct qmu_gpd *gpd;
    struct qmu_gpd *gpd_current;
    struct udc_request *request;
    struct mu3d_req *mreq;
    void *base = u3d->mac_base;
    u32 gpd_dma;

    mep = mtu3_find_ep(u3d, epnum, USB_DIR_OUT);
    if (!mep || !mep->req)
        return;

    ring = &mep->gpd_ring;
    gpd = ring->dequeue;
    gpd_dma = ssusb_readl(base, USB_QMU_RQCPR(epnum));
    gpd_current = gpd_dma_to_virt(ring, gpd_dma);

    DBG_QMU("%s %s, last=%p, current=%p, enq=%p\n",
            __func__, mep->name, gpd, gpd_current, ring->enqueue);

    while (gpd != gpd_current && !(gpd->flag & GPD_FLAGS_HWO)) {
        request = mep->req;
        mreq = to_mu3d_req(request);
        if (!mreq || mreq->gpd != gpd) {
            DBG_QMU("no correct RX req is found\n");
            break;
        }
        mreq->actual = gpd->buf_len;
        handle_ept_complete(mep, 0);
        gpd = advance_deq_gpd(ring);
    }

    DBG_QMU("%s EP%dOUT, deq=%p, enq=%p, complete\n",
            __func__, epnum, ring->dequeue, ring->enqueue);
}

static void qmu_done_isr(struct mu3d *u3d, u32 done_status)
{
    int i;

    for (i = 1; i <= (MT_EP_NUM / 2); i++) {
        if (done_status & QMU_RX_DONE_INT(i))
            qmu_done_rx(u3d, i);
        if (done_status & QMU_TX_DONE_INT(i))
            qmu_done_tx(u3d, i);
    }
}

static void qmu_exception_isr(struct mu3d *u3d, u32 qmu_status)
{
    void *base = u3d->mac_base;
    u32 errval;
    int i;

    if ((qmu_status & RXQ_CSERR_INT) || (qmu_status & RXQ_LENERR_INT)) {
        errval = ssusb_readl(base, U3D_RQERRIR0);
        for (i = 1; i <= (MT_EP_NUM / 2); i++) {
            if (errval & QMU_RX_CS_ERR(i))
                DBG_QMU("Rx EP%d CS error!\n", i);

            if (errval & QMU_RX_LEN_ERR(i))
                DBG_QMU("RX EP%d Length error\n", i);
        }
        ssusb_writel(base, U3D_RQERRIR0, errval);
    }

    if (qmu_status & RXQ_ZLPERR_INT) {
        errval = ssusb_readl(base, U3D_RQERRIR1);
        for (i = 1; i <= (MT_EP_NUM / 2); i++) {
            if (errval & QMU_RX_ZLP_ERR(i))
                DBG_QMU("RX EP%d Recv ZLP\n", i);
        }
        ssusb_writel(base, U3D_RQERRIR1, errval);
    }

    if ((qmu_status & TXQ_CSERR_INT) || (qmu_status & TXQ_LENERR_INT)) {
        errval = ssusb_readl(base, U3D_TQERRIR0);
        for (i = 1; i <= (MT_EP_NUM / 2); i++) {
            if (errval & QMU_TX_CS_ERR(i))
                DBG_QMU("Tx EP%d checksum error!\n", i);

            if (errval & QMU_TX_LEN_ERR(i))
                DBG_QMU("Tx EP%d send ZLP failed\n", i);
        }
        ssusb_writel(base, U3D_TQERRIR0, errval);
    }
}

void mtu3_qmu_isr(struct mu3d *u3d)
{
    u32 qmu_status;
    u32 qmu_done_status;
    void *base = u3d->mac_base;

    /* U3D_QISAR1 is read update */
    qmu_status = ssusb_readl(base, U3D_QISAR1);
    qmu_status &= ssusb_readl(base, U3D_QIER1);

    qmu_done_status = ssusb_readl(base, U3D_QISAR0);
    qmu_done_status &= ssusb_readl(base, U3D_QIER0);
    ssusb_writel(base, U3D_QISAR0, qmu_done_status); /* W1C */
    DBG_QMU("[INTR] QMUdone[TX=%x, RX=%x] QMUexp[%x]\n",
            (qmu_done_status & 0xFFFF), qmu_done_status >> 16,
            qmu_status);

    if (qmu_done_status)
        qmu_done_isr(u3d, qmu_done_status);

    if (qmu_status)
        qmu_exception_isr(u3d, qmu_status);
}

int mtu3_qmu_init(void)
{
    if (QMU_GPD_SIZE != 16) {
        DBG_QMU("QMU_GPD size SHOULD be 16 Bytes");
        return -EINVAL;
    }
    return 0;
}

#else /* #ifdef SUPPORT_QMU */

void mtu3_qmu_flush(struct udc_endpoint *mep)
{}

int mtu3_gpd_ring_alloc(struct udc_endpoint *mep)
{
    return 0;
}

void mtu3_gpd_ring_free(struct udc_endpoint *mep)
{}

void mtu3_qmu_isr(struct mu3d *u3d)
{}

int mtu3_qmu_init(void)
{
    return 0;
}

#endif /* #ifdef SUPPORT_QMU */
