/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/*
 * MediaTek Inc. (C) 2018. All rights reserved.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 *
 * driver for SSUSB controller
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include "hal_nvic.h"
#include "hal_nvic_internal.h"
#include <stdbool.h>
#include <string.h>

#include "mtu3.h"

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager_internal.h"
#include "hal_sleep_manager_platform.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

/* bits used in ep interrupts etc */
#define EPT_RX(n) (1 << ((n) + 16))
#define EPT_TX(n) (1 << (n))
struct mu3d *g_u3d;

static void dump_setup_packet(char *str, struct usb_setup *setup)
{
    mtu3_debug("%s\n", str);
    mtu3_debug("    bmRequestType = %x\n", setup->bmRequestType);
    mtu3_debug("    bRequest = %x\n", setup->bRequest);
    mtu3_debug("    wValue = %x\n", setup->wValue);
    mtu3_debug("    wIndex = %x\n", setup->wIndex);
    mtu3_debug("    wLength = %x\n", setup->wLength);
}

static void copy_desc(struct udc_request *req, void *desc, int max_len, int max_buf)
{
    int len, available;

    if (!desc)
        return;

    len = max_len;
    available = max_buf - req->length;
    if (available <= 0)
        return;

    if (len > available)
        len = available;

    memcpy(req->buffer + req->length, desc, len);
    req->length += len;

    mtu3_debug("%s length: %d, len: %d\n", __func__, req->length, len);
}

static int std_get_descs(struct gadget_dev *gdev, struct udc_request *req,
                         struct usb_setup *setup)
{
    struct device_descriptor *dev_desc;
    struct device_qualifier_descriptor *dev_qdesc;
    struct mt_config *conf_array;
    struct string_descriptor *str_desc;
    int max = setup->wLength;
    int desc_type = (setup->wValue) >> 8;
    int index = (setup->wValue) & 0xff;

    /* setup tx urb */
    req->actual = 0;
    req->length = 0;

    switch (desc_type) {
        case USB_DESCRIPTOR_TYPE_DEVICE:
            mtu3_debug("USB_DESCRIPTOR_TYPE_DEVICE\n");
            dev_desc = gdev->dev_desc;
            copy_desc(req, dev_desc, dev_desc->bLength, max);
            break;
        case USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER:
            mtu3_debug("USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER\n");
            dev_qdesc = gdev->dev_qualifier_desc;
            copy_desc(req, dev_qdesc, dev_qdesc->bLength, max);
            break;
        case USB_DESCRIPTOR_TYPE_CONFIGURATION:
            mtu3_debug("USB_DESCRIPTOR_TYPE_CONFIGURATION\n");
            conf_array = gdev->conf_array;
            copy_desc(req, conf_array->raw, conf_array->config_desc->wTotalLength, max);
            break;
        case USB_DESCRIPTOR_TYPE_STRING:
            mtu3_debug("USB_DESCRIPTOR_TYPE_STRING\n");
            str_desc = gdev->string_table[index];
            copy_desc(req, str_desc, str_desc->bLength, max);
            break;
        case USB_DESCRIPTOR_TYPE_BOS:
            mtu3_debug("USB_DESCRIPTOR_TYPE_BOS\n");
            copy_desc(req, gdev->bos_desc, gdev->bos_desc->bLength, max);
            copy_desc(req, gdev->ext_cap_desc, gdev->ext_cap_desc->bLength, max);
            copy_desc(req, gdev->ss_cap_desc, gdev->ss_cap_desc->bLength, max);
            break;
        default:
            return -1;
    }

    return 0;
}

static inline void writel_rep(volatile void *addr, const void *buffer,
                              unsigned int count)
{
    if (count) {
        const u32 *buf = buffer;

        do {
            writel(*buf++, addr);
        } while (--count);
    }
}

static inline void readl_rep(const volatile void *addr, void *buffer,
                             unsigned int count)
{
    if (count) {
        u32 *buf = buffer;

        do {
            u32 x = readl(addr);
            *buf++ = x;
        } while (--count);
    }
}

static int pio_read_fifo(void *base, int ep_num, u8 *dst, u16 len)
{
    void *fifo = base + USB_FIFO(ep_num);
    u32 index = 0;
    u32 value;

    if (len >= 4) {
        readl_rep(fifo, dst, len >> 2);
        index = len & ~0x03;
    }
    if (len & 0x3) {
        value = readl(fifo);
        memcpy(&dst[index], &value, len & 0x3);
    }

    mtu3_debug("%s - ep_num: %d, len: %d, dst: %p\n",
               __func__, ep_num, len, dst);

    return len;
}

static void pio_write_fifo(void *base, int ep_num, u8 *src, u16 len)
{
    void *fifo = base + USB_FIFO(ep_num);
    u32 index = 0;

    mtu3_debug("%s - ep_num: %d, len: %d, src: %p\n",
               __func__, ep_num, len, src);

    if (len >= 4) {
        writel_rep(fifo, src, len >> 2);
        index = len & ~0x03;
    }
    if (len & 0x02) {
        writew(*(u16 *)&src[index], fifo);
        index += 2;
    }
    if (len & 0x01)
        writeb(src[index], fifo);
}

/* enable/disable U3D SS function */
static void mtu3_ss_func_set(struct mu3d *u3d, bool enable)
{
    void *base = u3d->mac_base;

    /* If usb3_en==0, LTSSM will go to SS.Disable state */
    if (enable)
        ssusb_setbits(base, U3D_USB3_CONFIG, USB3_EN);
    else
        ssusb_clrbits(base, U3D_USB3_CONFIG, USB3_EN);

    mtu3_crit("U3 pullup D%s\n", enable ? "+" : "-");
}

/* set/clear U3D HS device soft connect */
static void mtu3_hs_softconn_set(struct mu3d *u3d, bool enable)
{
    void *base = u3d->mac_base;

    if (enable)
        ssusb_setbits(base, U3D_POWER_MANAGEMENT, SOFT_CONN | SUSPENDM_ENABLE);
    else
        ssusb_clrbits(base, U3D_POWER_MANAGEMENT, SOFT_CONN | SUSPENDM_ENABLE);

    mtu3_crit("U2 pullup D%s\n", enable ? "+" : "-");
}

void mtu3_soft_connect(struct mu3d *u3d)
{
    if (u3d->is_u3_ip && u3d->speed > SSUSB_SPEED_HIGH)
        mtu3_ss_func_set(u3d, true);
    else
        mtu3_hs_softconn_set(u3d, true);
}

void mtu3_soft_disconnect(struct mu3d *u3d)
{
    if (u3d->is_u3_ip && u3d->speed > SSUSB_SPEED_HIGH)
        mtu3_ss_func_set(u3d, false);
    else
        mtu3_hs_softconn_set(u3d, false);
}

void mtu3_intr_enable(struct mu3d *u3d)
{
    u32 value;
    void *base = u3d->mac_base;

    /* enable LV1 ISR */
    value = BMU_INTR | QMU_INTR | MAC3_INTR | MAC2_INTR | EP_CTRL_INTR;
    ssusb_writel(base, U3D_LV1IESR, value);
    /* enable U2 common interrupts */
    value = SUSPEND_INTR | RESUME_INTR | RESET_INTR;
    ssusb_writel(base, U3D_COMMON_USB_INTR_ENABLE, value);

    /* Enable U3 LTSSM interrupts */
    if (u3d->is_u3_ip) {
        value = HOT_RST_INTR | WARM_RST_INTR | VBUS_RISE_INTR |
                VBUS_FALL_INTR | ENTER_U3_INTR | EXIT_U3_INTR;
        ssusb_writel(base, U3D_LTSSM_INTR_ENABLE, value);
    }

    /* Enable QMU interrupts. */
    value = TXQ_CSERR_INT | TXQ_LENERR_INT | RXQ_CSERR_INT |
            RXQ_LENERR_INT | RXQ_ZLPERR_INT;
    ssusb_writel(base, U3D_QIESR1, value);
    /* Enable speed change interrupt */
    ssusb_writel(base, U3D_DEV_LINK_INTR_ENABLE, SSUSB_DEV_SPEED_CHG_INTR);
}

static void mtu3_all_intr_disable(struct mu3d *u3d)
{
    void *base = u3d->mac_base;

    ssusb_writel(base, U3D_EPISR, 0xffffffff);
    ssusb_writel(base, U3D_QISAR0, 0xffffffff);
    ssusb_writel(base, U3D_QISAR1, 0xffffffff);
    ssusb_writel(base, U3D_TQERRIR0, 0xffffffff);
    ssusb_writel(base, U3D_RQERRIR0, 0xffffffff);
    ssusb_writel(base, U3D_RQERRIR1, 0xffffffff);
    ssusb_writel(base, U3D_LV1IECR, 0xffffffff);
    ssusb_writel(base, U3D_EPIECR, 0xffffffff);

    /* clear registers */
    ssusb_writel(base, U3D_QIECR0, 0xffffffff);
    ssusb_writel(base, U3D_QIECR1, 0xffffffff);
    ssusb_writel(base, U3D_TQERRIECR0, 0xffffffff);
    ssusb_writel(base, U3D_RQERRIECR0, 0xffffffff);
    ssusb_writel(base, U3D_RQERRIECR1, 0xffffffff);
    ssusb_writel(base, U3D_COMMON_USB_INTR, 0xffffffff);
}

static USB_SPEED mtu3_get_speed(struct mu3d *u3d)
{
    void *base = u3d->mac_base;
    const char *spd_str[] = {"UNKNOWN", "LS", "FS", "HS", "SS", "SSP"};
    USB_SPEED spd;

    switch (SSUSB_DEV_SPEED(ssusb_readl(base, U3D_DEVICE_CONF))) {
        case 1:
            spd = SSUSB_SPEED_FULL;
            break;
        case 3:
            spd = SSUSB_SPEED_HIGH;
            break;
        case 4:
            spd = SSUSB_SPEED_SUPER;
            break;
        case 5:
            spd = SSUSB_SPEED_SUPER_PLUS;
            break;
        default:
            spd = SSUSB_SPEED_UNKNOWN;
            break;
    }

    mtu3_crit("%s (%d) is detected\n", spd_str[spd % ARRAY_SIZE(spd_str)], spd);
    return spd;
}

/* SSP is not supported tmp. */
static void mtu3_set_speed(struct mu3d *u3d)
{
    void *base = u3d->mac_base;
    USB_SPEED spd = u3d->speed;
    const char *spd_str[] = {"UNKNOWN", "LS", "FS", "HS", "SS", "SSP"};

    switch (spd) {
        case SSUSB_SPEED_FULL:
            ssusb_clrbits(base, U3D_USB3_CONFIG, USB3_EN);
            ssusb_clrbits(base, U3D_POWER_MANAGEMENT, HS_ENABLE);
            break;
        case SSUSB_SPEED_HIGH:
            ssusb_clrbits(base, U3D_USB3_CONFIG, USB3_EN);
            ssusb_setbits(base, U3D_POWER_MANAGEMENT, HS_ENABLE);
            break;
        case SSUSB_SPEED_SUPER:
        default:
            break;
    }

    mtu3_crit("%s %s (%d)\n", __func__, spd_str[spd % ARRAY_SIZE(spd_str)], spd);
}

struct udc_endpoint *mtu3_find_ep(struct mu3d *u3d, int ep_num, u8 dir)
{
    struct udc_endpoint *ept;
    int i;
    u8 in;

    in = (dir == USB_DIR_IN) ? 1 : 0;

    for (i = 1; i < u3d->gdev->num_eps; i++) {
        ept = u3d->gdev->eps[i];
        if (ept->num == ep_num && ept->in == in)
            return ept;
    }

    mtu3_crit("%s: can't find ep\n", __func__);
    return NULL;
}

static void mtu3_flush_fifo(struct mu3d *u3d, u8 ep_num, u8 dir)
{
    void *base = u3d->mac_base;

    if (ep_num == 0) {
        ssusb_setbits(base, U3D_EP_RST, EP0_RST);
        ssusb_clrbits(base, U3D_EP_RST, EP0_RST);
    } else {
        ssusb_setbits(base, U3D_EP_RST, EP_RST((dir == USB_DIR_IN), ep_num));
        ssusb_clrbits(base, U3D_EP_RST, EP_RST((dir == USB_DIR_IN), ep_num));
    }
}

static void ep0_stall_set(struct mu3d *u3d, bool set, u32 pktrdy)
{
    u32 csr;
    void *base = u3d->mac_base;

    /* EP0_SENTSTALL is W1C */
    csr = ssusb_readl(base, U3D_EP0CSR) & EP0_W1C_BITS;
    if (set)
        csr |= EP0_SENDSTALL | pktrdy;
    else
        csr = (csr & ~EP0_SENDSTALL) | EP0_SENTSTALL;
    ssusb_writel(base, U3D_EP0CSR, csr);

    u3d->ep0_state = EP0_IDLE;
}

/*
 * Return value indicates the TxFIFO size of 2^n bytes, (ex: value 10 means 2^10 =
 * 1024 bytes.) TXFIFOSEGSIZE should be equal or bigger than 4. The TxFIFO size of
 * 2^n bytes also should be equal or bigger than TXMAXPKTSZ. This EndPoint occupy
 * total memory size  (TX_SLOT + 1 )*2^TXFIFOSEGSIZE bytes.
 */
static u8 get_seg_size(u32 maxp)
{
    /* Set fifo size(double buffering is currently not enabled) */
    switch (maxp) {
        case 8:
        case 16:
            return USB_FIFOSZ_SIZE_16;
        case 32:
            return USB_FIFOSZ_SIZE_32;
        case 64:
            return USB_FIFOSZ_SIZE_64;
        case 128:
            return USB_FIFOSZ_SIZE_128;
        case 256:
            return USB_FIFOSZ_SIZE_256;
        case 512:
            return USB_FIFOSZ_SIZE_512;
        case 1023:
        case 1024:
        case 2048:
        case 3072:
        case 4096:
            return USB_FIFOSZ_SIZE_1024;
        default:
            mtu3_debug("The maxp %d is not supported\n", maxp);
            return USB_FIFOSZ_SIZE_512;
    }
}

static int check_interval(int interval)
{
    if (interval < 1)
        interval = 1;

    if (interval > 16)
        interval = 16;

    return interval - 1;
}

static void mtu3_setup_ep(struct mu3d *u3d, struct udc_endpoint *ept)
{
    u32 csr0, csr1, csr2;
    u32 fifo_addr;
    u8 seg_size;
    void *base = u3d->mac_base;
    u32 ep_num = ept->num;
    struct endpoint_descriptor *desc = ept->desc;
    struct gadget_dev *gdev = u3d->gdev;
    unsigned char type = desc->bmAttributes & 0x3;
    int max_packet = desc->wMaxPacketSize;
    u32 interval = desc->bInterval;

    /* Nothing needs to be done for ep0 */
    if (ep_num == 0)
        return;

    switch (gdev->speed) {
        case SSUSB_SPEED_SUPER:
        case SSUSB_SPEED_SUPER_PLUS:
            if ((type == USB_EP_XFER_INT) || (type == USB_EP_XFER_ISO)) {
                interval = desc->bInterval;
                interval = check_interval(interval);
            }
            break;
        case SSUSB_SPEED_HIGH:
            if ((type == USB_EP_XFER_INT) || (type == USB_EP_XFER_ISO)) {
                interval = desc->bInterval;
                interval = check_interval(interval);
            }
            break;
        default:
            break; /*others are ignored */
    }

    /* Set fifo address, fifo size, and fifo max packet size */
    mtu3_debug("%s: eptype[%d] ep%d%s, maxpkt: %d\n", __func__,
               ept->type, ept->num, ept->in ? "in" : "out", ept->maxpkt);

    /* Set fifo size(only supports single buffering) */
    seg_size = get_seg_size(max_packet & 0x7ff);

    if (ept->in) {  /* TX case */
        mtu3_flush_fifo(u3d, ep_num, USB_DIR_IN);

        csr0 = TX_TXMAXPKTSZ(max_packet & 0x7ff);
        fifo_addr = u3d->tx_fifo_addr + (U3D_FIFO_SIZE_UNIT * ep_num);
        csr2 = TX_FIFOADDR(fifo_addr >> 4);
        csr2 |= TX_FIFOSEGSIZE(seg_size);

        switch (type) {
            case USB_EP_XFER_BULK:
                csr1 = TX_TYPE(TYPE_BULK);
                break;
            case USB_EP_XFER_ISO:
                csr1 = TX_TYPE(TYPE_ISO);
                csr2 |= TX_BINTERVAL(interval);
                break;
            case USB_EP_XFER_INT:
                csr1 = TX_TYPE(TYPE_INT);
                csr2 |= TX_BINTERVAL(interval);
                break;
            default:
                mtu3_err("%s: error ep type\n", __func__);
                return;
        }

#ifdef SUPPORT_QMU
        csr0 |= TX_DMAREQEN;
        /* Enable QMU Done interrupt */
        ssusb_setbits(base, U3D_QIESR0, QMU_TX_DONE_INT(ep_num));
#else /* #ifdef SUPPORT_QMU */
        ssusb_setbits(base, U3D_EPIECR, EP_TXISR(ep_num));  /* W1C */
        ssusb_setbits(base, U3D_EPIESR, EP_TXISR(ep_num));  /* W1S */
#endif /* #ifdef SUPPORT_QMU */

        ssusb_writel(base, MU3D_EP_TXCR0(ep_num), csr0);
        ssusb_writel(base, MU3D_EP_TXCR1(ep_num), csr1);
        ssusb_writel(base, MU3D_EP_TXCR2(ep_num), csr2);

    } else {    /* RX case */
        mtu3_flush_fifo(u3d, ep_num, USB_DIR_OUT);

        csr0 = RX_RXMAXPKTSZ(max_packet & 0x7ff);
        fifo_addr = u3d->rx_fifo_addr + (U3D_FIFO_SIZE_UNIT * ep_num);
        csr2 = RX_FIFOADDR(fifo_addr >> 4);
        csr2 |= RX_FIFOSEGSIZE(seg_size);

        switch (type) {
            case USB_EP_XFER_BULK:
                csr1 = RX_TYPE(TYPE_BULK);
                break;
            case USB_EP_XFER_ISO:
                csr1 = RX_TYPE(TYPE_ISO);
                csr2 |= RX_BINTERVAL(interval);
                break;
            case USB_EP_XFER_INT:
                csr1 = RX_TYPE(TYPE_INT);
                csr2 |= RX_BINTERVAL(interval);
                break;
            default:
                mtu3_err("%s: error ep type\n", __func__);
                return;
        }

#ifdef SUPPORT_QMU
        csr0 |= RX_DMAREQEN;
        /* Enable QMU Done interrupt */
        ssusb_setbits(base, U3D_QIESR0, QMU_RX_DONE_INT(ep_num));
#else /* #ifdef SUPPORT_QMU */
        ssusb_setbits(base, U3D_EPIECR, EP_RXISR(ep_num));  /* W1C */
        /* enable it when queue RX request */
        /* setbits32(EP_RXISR(ep_num), U3D_EPIESR);*/   /* W1S */
#endif /* #ifdef SUPPORT_QMU */
        ssusb_writel(base, MU3D_EP_RXCR0(ep_num), csr0);
        ssusb_writel(base, MU3D_EP_RXCR1(ep_num), csr1);
        ssusb_writel(base, MU3D_EP_RXCR2(ep_num), csr2);
    }

#ifdef SUPPORT_QMU
    mtu3_qmu_start(ept);
#endif /* #ifdef SUPPORT_QMU */
}

static void mtu3_ep0en(struct mu3d *u3d)
{
    u32 temp = 0;
    void *base = u3d->mac_base;
    struct udc_endpoint *ep0 = u3d->ep0;

    sprintf(ep0->name, "ep0\r");
    ep0->endpoint_address = 0;
    ep0->type = USB_EP_XFER_CTRL;
    ep0->num = EP0;
    if (u3d->speed == SSUSB_SPEED_SUPER)
        ep0->maxpkt = EP0_MAX_PKT_SIZE_U3;
    else
        ep0->maxpkt = EP0_MAX_PKT_SIZE;

    temp = ssusb_readl(base, U3D_EP0CSR);
    temp &= ~(EP0_MAXPKTSZ_MSK | EP0_AUTOCLEAR | EP0_AUTOSET | EP0_DMAREQEN);
    temp |= EP0_MAXPKTSZ(ep0->maxpkt);
    temp &= EP0_W1C_BITS;
    ssusb_writel(base, U3D_EP0CSR, temp);

    /* enable EP0 interrupts */
    ssusb_setbits(base, U3D_EPIESR, EP_EP0ISR);
}

static void mtu3_reg_init(struct mu3d *u3d)
{
    void *base = u3d->mac_base;

    mtu3_all_intr_disable(u3d);
    if (u3d->is_u3_ip) {
        /* disable LGO_U1/U2 by default */
        ssusb_clrbits(base, U3D_LINK_POWER_CONTROL, SW_U1_ACCEPT_ENABLE |
                      SW_U2_ACCEPT_ENABLE | SW_U1_REQUEST_ENABLE |
                      SW_U2_REQUEST_ENABLE);
        /* device responses to u3_exit from host automatically */
        ssusb_clrbits(base, U3D_LTSSM_CTRL, SOFT_U3_EXIT_EN);
        /* automatically build U2 link when U3 detect fail */
        ssusb_setbits(base, U3D_USB2_TEST_MODE, U2U3_AUTO_SWITCH);
    }
    /* delay about 0.1us from detecting reset to send chirp-K */
    ssusb_clrbits(base, U3D_LINK_RESET_INFO, WTCHRP_MSK);
    /* U2/U3 detected by HW */
    ssusb_writel(base, U3D_DEVICE_CONF, 0);
    /* enable QMU 16B checksum */
    ssusb_setbits(base, U3D_QCR0, QMU_CS16B_EN);

    /* vbus detected by HW */
    ssusb_clrbits(base, U3D_MISC_CTRL, VBUS_FRC_EN | VBUS_ON);
    /* force vbus */
    /* ssusb_setbits(base, U3D_MISC_CTRL, VBUS_FRC_EN | VBUS_ON); */

    mtu3_set_speed(u3d);
    mtu3_intr_enable(u3d);
    mtu3_ep0en(u3d);
}

static void mtu3_setup_eps(struct mu3d *u3d)
{
    struct gadget_dev *gdev = u3d->gdev;
    int i;

    for (i = 1; i < gdev->num_eps; ++i)
        mtu3_setup_ep(u3d, gdev->eps[i]);
}

void handle_ept_complete(struct udc_endpoint *ept, int status)
{
    struct udc_request *req;
    struct mu3d_req *mreq;
    unsigned int actual;

    req = ept->req;
    mreq = to_mu3d_req(req);
    list_del(&mreq->list);
    if (req) {
        /* ept->req = NULL; */

        if (status)
            mtu3_crit("%s: %s FAIL status: %d\n", __func__, ept->name, status);

        actual = status ? 0 : mreq->actual;
        req->status = status;

        mtu3_debug("%s: %s, req: %p: complete: %d/%d: status: %d\n",
                   __func__, ept->name, req, actual, req->length, status);

        if (req->complete)
            req->complete(req, actual, status);
    }
}

static int mtu3_read_fifo(void *base, struct udc_endpoint *ept)
{
    struct udc_request *req = ept->req;
    struct mu3d_req *mreq = to_mu3d_req(req);
    int ep_num = ept->num;
    u32 count = 0;

    if (mreq) {
        if (ep_num == 0)
            count = ssusb_readl(base, U3D_RXCOUNT0);
        else
            count = EP_RX_COUNT(ssusb_readl(base, MU3D_EP_RXCR3(ep_num)));

        count = MIN(req->length - mreq->actual, count);
        pio_read_fifo(base, ep_num, req->buffer + mreq->actual, count);
        mreq->actual += count;

        mtu3_debug("%s: ep%dout, mreq: %p, buf: %p, length: %d, actual: %d\n",
                   __func__, ep_num, mreq, req->buffer, req->length, mreq->actual);
    }

    return count;
}

static int mtu3_write_fifo(void *base, struct udc_endpoint *ept)
{
    struct udc_request *req = ept->req;
    struct mu3d_req *mreq = to_mu3d_req(req);
    unsigned char *buf;
    int ep_num = ept->num;
    int count = 0;

    if (mreq) {
        mtu3_debug("%s: ep%din mreq: %p, length: %d, actual: %d, maxp: %d\n",
                   __func__, ep_num, mreq, req->length, mreq->actual, ept->maxpkt);

        count = MIN(req->length - mreq->actual, ept->maxpkt);
        buf = req->buffer + mreq->actual;
        pio_write_fifo(base, ep_num, buf, count);
        mreq->actual += count;
    }

    return count;
}

static void mtu3_ep0_write(struct mu3d *u3d)
{
    struct udc_endpoint *ep0 = u3d->ep0;
    struct udc_request *req = ep0->req;
    struct mu3d_req *mreq = to_mu3d_req(req);
    void *base = u3d->mac_base;
    unsigned int count = 0;
    u32 csr0;

    csr0 = ssusb_readl(base, U3D_EP0CSR);
    if (csr0 & EP0_TXPKTRDY) {
        mtu3_debug("%s: ep0 is not ready to be written\n", __func__);
        return;
    }

    count = mtu3_write_fifo(base, ep0);

    /* hardware limitiation: can't set (EP0_TXPKTRDY | EP0_DATAEND) at same time */
    csr0 |= (EP0_TXPKTRDY);
    ssusb_writel(base, U3D_EP0CSR, csr0);

    mtu3_debug("%s: length=%d, actual=%d\n", __func__, req->length, mreq->actual);
    if (count < ep0->maxpkt || req->length == mreq->actual) {
        /* last packet */
        mreq->actual = 0;
        u3d->ep0_state = EP0_TX_END;
    }
}

static void mtu3_ep0_read(struct mu3d *u3d)
{
    struct udc_endpoint *ep0 = u3d->ep0;
    struct udc_request *req = ep0->req;
    struct mu3d_req *mreq = to_mu3d_req(req);
    void *base = u3d->mac_base;
    unsigned int count = 0;
    u32 csr0 = 0;

    csr0 = ssusb_readl(base, U3D_EP0CSR);

    /* erroneous ep0 interrupt */
    if (!(csr0 & EP0_RXPKTRDY))
        return;

    count = mtu3_read_fifo(base, ep0);

    /* work around: cannot set  (EP0_RXPKTRDY | EP0_DATAEND) at same time */
    csr0 |= (EP0_RXPKTRDY);
    ssusb_writel(base, U3D_EP0CSR, csr0);

    if (count < ep0->maxpkt || mreq->actual == req->length) {
        /* last packet */
        mreq->actual = 0;
        csr0 |= EP0_DATAEND;
        u3d->ep0_state = EP0_IDLE;
    } else {
        /* more packets are waiting to be transferred */
        csr0 |= EP0_RXPKTRDY;
    }

    ssusb_writel(base, U3D_EP0CSR, csr0);
}

/* for high speed test mode; see USB 2.0 spec 7.1.20 */
static const u8 test_packet[53] = {
    /* implicit SYNC then DATA0 to start */

    /* JKJKJKJK x9 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* JJKKJJKK x8 */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    /* JJJJKKKK x8 */
    0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
    /* JJJJJJJKKKKKKK x8 */
    0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* JJJJJJJK x8 */
    0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd,
    /* JKKKKKKK x10, JK */
    0xfc, 0x7e, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0x7e,
    /* implicit CRC16 then EOP to end */
};

static void load_test_packet(struct mu3d *u3d)
{
    mtu3_crit("LOAD TEST PACKET\n");
    pio_write_fifo(u3d->mac_base, 0, (u8 *)test_packet, sizeof(test_packet));
}

static int handle_test_mode(struct mu3d *u3d, struct usb_setup *setup)
{
    int ret;
    u8 test_mode;
    u32 value;

    switch (setup->wIndex >> 8) {
        case TEST_J:
            test_mode = TEST_J_MODE;
            mtu3_crit("TEST_J\n");
            break;
        case TEST_K:
            test_mode = TEST_K_MODE;
            mtu3_crit("TEST_K\n");
            break;
        case TEST_SE0_NAK:
            test_mode = TEST_SE0_NAK_MODE;
            mtu3_crit("TEST_SE0_NAK\n");
            break;
        case TEST_PACKET:
            test_mode = TEST_PACKET_MODE;
            mtu3_crit("TEST_PACKET\n");
            break;
        default:
            return -EINVAL;
    }

    u3d->test_mode = true;
    if (test_mode == TEST_PACKET_MODE)
        load_test_packet(u3d);

    value = ssusb_readl(u3d->mac_base, U3D_EP0CSR) & EP0_W1C_BITS;
    ssusb_writel(u3d->mac_base, U3D_EP0CSR, value | EP0_SETUPPKTRDY | EP0_DATAEND);
    ret = wait_for_value(u3d->mac_base, EP0_DATAEND, 0, 10, 500);
    if (ret) {
        mtu3_err("%s: failed\n", __func__);
        return -EINVAL;
    }
    ssusb_writel(u3d->mac_base, U3D_USB2_TEST_MODE, test_mode);
    mtu3_crit("%s: TEST ADDR: 0x%p\n", __func__, (u3d->mac_base + U3D_USB2_TEST_MODE));
    u3d->ep0_state = EP0_IDLE;
    return 0;
}

static int class_set_alt(struct mu3d *u3d, const struct usb_setup *usb_ctrl)
{
    int ret;

    if (!u3d->gdev->driver.set_alt) {
        mtu3_err("%s no class set_alt\n", __func__);
        return -1;
    }

    ret = u3d->gdev->driver.set_alt(u3d->ep0->req, usb_ctrl);
    return ret;
}

static int ep0_handle_feature(struct mu3d *u3d, struct usb_setup *setup)
{
    int handled = -EINVAL;

    switch ((setup->bmRequestType & USB_RECIP_MASK)) {
        case USB_RECIP_DEVICE:
            switch (setup->wValue) {
                case USB_DEVICE_TEST_MODE:
                    if (u3d->speed != SSUSB_SPEED_HIGH || setup->wIndex & 0xff) {
                        mtu3_crit("%s %d: feature not support.\n", __func__, __LINE__);
                        goto error;
                    }
                    handled = handle_test_mode(u3d, setup);
                    if (handled == 0)
                        return 0;
                    break;
                default:
                    mtu3_crit("%s %d: feature not support.\n", __func__, __LINE__);
                    goto error;
            }
            break;
        default:
            mtu3_crit("%s %d: feature not support.\n", __func__, __LINE__);
            goto error;
    }

    return 0;

error:
    mtu3_crit("%s %d: feature not support.\n", __func__, __LINE__);
    return -EINVAL;
}

static int ep0_standard_setup(struct mu3d *u3d, struct usb_setup *setup)
{
    struct gadget_dev *gdev = u3d->gdev;
    struct udc_request *req = u3d->ep0->req;
    u8 *cp = req->buffer;
    u32 value;
    int ret;

    if ((setup->bmRequestType & USB_TYPE_MASK) != USB_TYPE_STANDARD)
        return -EINVAL; /* Class-specific requests are handled elsewhere */

    /* handle all requests that return data (direction bit set on bm RequestType) */
    if ((setup->bmRequestType & USB_DIR_MASK)) {
        /* send the descriptor */
        u3d->ep0_state = EP0_TX;

        switch (setup->bRequest) {
            /* data stage: from device to host */
            case STDREQ_GET_STATUS:
                mtu3_debug("GET_STATUS\n");
                req->length = 2;
                cp[0] = 0;
                cp[1] = 0;

                switch (setup->bmRequestType & USB_RECIP_MASK) {
                    case USB_RECIP_DEVICE:
                        cp[0] = USB_STAT_SELFPOWERED;
                        break;
                    case USB_RECIP_OTHER:
                        req->length = 0;
                        break;
                    default:
                        break;
                }
                return 0;
            case STDREQ_GET_DESCRIPTOR:
                mtu3_debug("GET_DESCRIPTOR\n");
                return std_get_descs(gdev, req, setup);
            case STDREQ_GET_CONFIGURATION:
                mtu3_debug("GET_CONFIGURATION\n");
                req->length = 1;
                cp[0] = gdev->configuration;
                return 0;
            case STDREQ_GET_INTERFACE:
                mtu3_debug("GET_INTERFACE\n");
                req->length = 1;
                cp[0] = gdev->alternate;
                return 0;
            default:
                mtu3_crit("Unsupported command with TX data stage\n");
                break;
        }
    } else {
        switch (setup->bRequest) {
            case STDREQ_SET_ADDRESS:
                mtu3_debug("SET_ADDRESS\n");
                u3d->address = (setup->wValue);
                value = ssusb_readl(u3d->mac_base, U3D_DEVICE_CONF);
                value &= ~DEV_ADDR_MSK;
                value |= DEV_ADDR(u3d->address);
                ssusb_writel(u3d->mac_base, U3D_DEVICE_CONF, value);
                return 0;
            case STDREQ_SET_CONFIGURATION:
                mtu3_debug("SET_CONFIGURATION\n");
                u3d->usb_online = setup->wValue ? 1 : 0;
                if (setup->wValue == 1) {
                    mtu3_setup_eps(u3d);
                    udelay(50);
                    gdev->notify(gdev, UDC_EVENT_ONLINE);
                } else {
                    gdev->notify(gdev, UDC_EVENT_OFFLINE);
                }
                gdev->configuration = (setup->wValue) & 0x7f;
                gdev->interface = 0;
                gdev->alternate = 0;
                mtu3_crit("usb_online: %d\n", u3d->usb_online);
                return 0;
            case STDREQ_CLEAR_FEATURE:
                mtu3_debug("CLEAR_FEATURE\n");
                switch ((setup->bmRequestType & USB_RECIP_MASK)) {
                    case USB_RECIP_ENDPOINT: {
                            u8 dir = setup->wIndex & USB_DIR_MASK;
                            u8 ep_num = setup->wIndex & 0x0f;

                            mtu3_crit("CLEAR_FEATURE, ep_num=%d, dir=%d\n", ep_num, dir);
                        }
                        return 0;
                    default:
                        mtu3_crit("Unsupported bmRequestType\n");
                        break;
                }
                break;
            case STDREQ_SET_FEATURE:
                return ep0_handle_feature(u3d, setup);
            case STDREQ_SET_INTERFACE:
                mtu3_debug("SET_INTERFACE\n");
                ret = class_set_alt(u3d, setup);
                if (ret) {
                    mtu3_err("SET_INTERFACE error\n");
                    return -EINVAL;
                }
                return 0;
            default:
                mtu3_debug("setup->request: %x, setup->value: %x\n",
                           setup->bRequest, setup->wValue);
                mtu3_crit("Unsupported command with RX data stage\n");
                break;
        } /* switch request */
    }

    return -EINVAL;
}

static int forward_to_driver_out(struct mu3d *u3d)
{
    int ret;

    if (!u3d->gdev->driver.rx_setup) {
        return -1;
    }

    mtu3_debug("%s forward to RX class driver\n", __func__);
    ret = u3d->gdev->driver.rx_setup(u3d->ep0->req);
    return ret;
}

static int forward_to_driver(struct mu3d *u3d, const struct usb_setup *usb_ctrl)
{
    int ret;

    if (!u3d->gdev->driver.setup) {
        mtu3_err("%s no class driver\n", __func__);
        return -1;
    }

    ret = u3d->gdev->driver.setup(u3d->ep0->req, usb_ctrl);
    return ret;
}

static void mtu3_ep0_setup(struct mu3d *u3d)
{
    struct usb_setup setup;
    int stall = -ENOTSUP;
    u8 req_type = 0;
    u32 csr0;
    u32 len;
    void *base = u3d->mac_base;

    csr0 = ssusb_readl(base, U3D_EP0CSR);
    if (!(csr0 & EP0_SETUPPKTRDY))
        return;

    len = ssusb_readl(base, U3D_RXCOUNT0);
    if (len != 8) {
        mtu3_crit("SETUP packet len %d != 8?\n", len);
        return;
    }

    /* unload fifo */
    pio_read_fifo(u3d->mac_base, EP0, (u8 *)&setup, len);

    dump_setup_packet("Device Request", &setup);

    /* decode command */
    req_type = setup.bmRequestType & USB_TYPE_MASK;
    if (req_type == USB_TYPE_STANDARD) {
        mtu3_debug("Standard Request\n");
        stall = ep0_standard_setup(u3d, &setup);
    } else if (req_type == USB_TYPE_CLASS || req_type == USB_TYPE_VENDOR) {
        mtu3_debug("Class Request\n");
        if (setup.wLength == 0)
            ; /* no data stage, nothing to do */
        else if (setup.bmRequestType & USB_DIR_IN)
            u3d->ep0_state = EP0_TX;
        else
            u3d->ep0_state = EP0_RX;

        stall = forward_to_driver(u3d, &setup);
    }

    /* command is not supported, inlcude  USB_TYPE_CLASS & USB_TYPE_VENDOR */
    if (stall) {
        ep0_stall_set(u3d, true, EP0_SETUPPKTRDY);
        return;
    }

    /* handle EP0 state */
    switch (u3d->ep0_state) {
        case EP0_TX:
            mtu3_debug("%s: EP0_TX\n", __func__);
            csr0 = ssusb_readl(base, U3D_EP0CSR);
            csr0 |= (EP0_SETUPPKTRDY | EP0_DPHTX);
            ssusb_writel(base, U3D_EP0CSR, csr0);
            mtu3_ep0_write(u3d);
            break;
        case EP0_RX:
            mtu3_debug("%s: EP0_RX\n", __func__);
            csr0 = ssusb_readl(base, U3D_EP0CSR);
            csr0 |= (EP0_SETUPPKTRDY);
            ssusb_writel(base, U3D_EP0CSR, csr0);
            break;
        case EP0_IDLE:
            if (u3d->test_mode) {
                mtu3_debug("%s: TEST MODE\n", __func__);
                break;
            }
            /* no data stage */
            mtu3_debug("%s: EP0_IDLE\n", __func__);
            csr0 = ssusb_readl(base, U3D_EP0CSR);
            csr0 |= (EP0_SETUPPKTRDY | EP0_DATAEND);
            ssusb_writel(base, U3D_EP0CSR, csr0);
            break;
        default:
            break;
    }
}

static void mtu3_ep0_isr(struct mu3d *u3d)
{
    u32 csr0;
    void *base = u3d->mac_base;

    csr0 = ssusb_readl(base, U3D_EP0CSR);

    if (csr0 & EP0_SENTSTALL) {
        mtu3_debug("USB: [EP0] SENTSTALL\n");
        ep0_stall_set(u3d, false, 0);
        csr0 = ssusb_readl(base, U3D_EP0CSR);
    }

    switch (u3d->ep0_state) {
        case EP0_IDLE:
            mtu3_debug("%s: EP0_IDLE\n", __func__);
            mtu3_ep0_setup(u3d);
            break;
        case EP0_TX:
            mtu3_debug("%s: EP0_TX\n", __func__);
            mtu3_ep0_write(u3d);
            break;
        case EP0_TX_END:
            mtu3_debug("%s: EP0_TX_END\n", __func__);
            csr0 |= EP0_DATAEND;
            ssusb_writel(base, U3D_EP0CSR, csr0);
            u3d->ep0_state = EP0_IDLE;
            break;
        case EP0_RX:
            mtu3_debug("%s: EP0_RX\n", __func__);
            mtu3_ep0_read(u3d);
            forward_to_driver_out(u3d);
            u3d->ep0_state = EP0_IDLE;
            break;
        default:
            mtu3_err("[ERR]: Unrecognized ep0 state %d\n", u3d->ep0_state);
            break;
    }
}

#ifndef SUPPORT_QMU
/* PIO: TX packet */
static int mtu3_epx_write(struct mu3d *u3d, struct udc_endpoint *ept)
{
    int ep_num = ept->num;
    int count;
    u32 csr;

    /* only for non-ep0 */
    if (ep_num == 0)
        return -EACCES;

    if (!ept->in)
        return -EINVAL;

    csr = ssusb_readl(u3d->mac_base, MU3D_EP_TXCR0(ep_num));
    if (csr & TX_TXPKTRDY) {
        mtu3_err("%s: ep%d is busy!\n", __func__, ep_num);
        return -EBUSY;
    }
    count = mtu3_write_fifo(u3d->mac_base, ept);

    csr |= TX_TXPKTRDY;
    ssusb_writel(u3d->mac_base, MU3D_EP_TXCR0(ep_num), csr);

    return count;
}

static void mtu3_epx_isr(struct mu3d *u3d, u8 ep_num, u8 dir)
{
    u32 csr;
    u32 count;
    struct udc_endpoint *ept;
    struct mu3d_req *mreq;
    struct udc_request *req;
    void *base = u3d->mac_base;

    ept = mtu3_find_ep(u3d, ep_num, dir);
    if (!ept || !ept->req)
        return;

    mtu3_debug("%s Interrupt\n", ept->name);
    req = ept->req;
    mreq = to_mu3d_req(req);

    if (dir == USB_DIR_IN) {
        csr = ssusb_readl(base, MU3D_EP_TXCR0(ep_num));
        if (csr & TX_SENTSTALL) {
            mtu3_err("EP%dIN: STALL\n", ep_num);
            handle_ept_complete(ept, -EPIPE);
            /* exception handling: implement this!! */
            return;
        }

        if (csr & TX_TXPKTRDY) {
            mtu3_err("%s: EP%dIN is busy\n", __func__, ep_num);
            return;
        }

        if (req->length == mreq->actual) {
            handle_ept_complete(ept, 0);
            return;
        }

        count = mtu3_write_fifo(base, ept);
        if (count) {
            csr |= TX_TXPKTRDY;
            ssusb_writel(base, MU3D_EP_TXCR0(ep_num), csr);
        }

        mtu3_debug("EP%dIN, count=%d, %d/%d\n",
                   ep_num, count, mreq->actual, req->length);

    } else {
        csr = ssusb_readl(base, MU3D_EP_RXCR0(ep_num));
        if (csr & RX_SENTSTALL) {
            mtu3_err("EP%dOUT: STALL\n", ep_num);
            /* exception handling: implement this!! */
            return;
        }

        if (!(csr & RX_RXPKTRDY)) {
            mtu3_debug("EP%dOUT: ERRONEOUS INTERRUPT\n", ep_num);
            return;
        }

        count = mtu3_read_fifo(base, ept);

        mtu3_debug("EP%dOUT, count = %d\n", ep_num, count);

        /* write 1 to clear RXPKTRDY */
        csr |= RX_RXPKTRDY;
        ssusb_writel(base, MU3D_EP_RXCR0(ep_num), csr);

        if (ssusb_readl(base, MU3D_EP_RXCR0(ep_num)) & RX_RXPKTRDY)
            mtu3_debug("%s: rxpktrdy clear failed\n", __func__);

        if (req->length == mreq->actual || count < ept->maxpkt) {
            /* disable EP RX intr */
            ssusb_setbits(base, U3D_EPIECR, EP_RXISR(ep_num));  /* W1C */
            handle_ept_complete(ept, 0);
        }
    }
}
#endif /* #ifndef SUPPORT_QMU */

/* handle abnormal DATA transfer if we had any, like USB unplugged */
static void mtu3_suspend(struct mu3d *u3d)
{
    struct gadget_dev *gdev = u3d->gdev;
    struct udc_endpoint *ept;
    int i;

    u3d->usb_online = 0;
    gdev->notify(gdev, UDC_EVENT_OFFLINE);

    /* error out any pending reqs, except ep0 */
    for (i = 1; i < gdev->num_eps; i++) {
        ept = gdev->eps[i];
        /* End operation when encounter uninitialized ept */
        if (ept->num == 0)
            break;

        mtu3_debug("%s: ep%d%s, req: %p\n", __func__,
                   ept->num, ept->in ? "in" : "out", ept->req);
        mtu3_qmu_flush(ept);

        /* if (ept->req) */
        /*  handle_ept_complete(ept, -ESHUTDOWN); */
    }
}

static void mtu3_status_reset(struct mu3d *u3d)
{
    u3d->ep0_state = EP0_IDLE;
    u3d->address = 0;
    u3d->test_mode = false;
}

static void mtu3_link_isr(struct mu3d *u3d)
{
    u32 linkint;
    void *base = u3d->mac_base;

    linkint = ssusb_readl(base, U3D_DEV_LINK_INTR);
    linkint &= ssusb_readl(base, U3D_DEV_LINK_INTR_ENABLE);
    ssusb_writel(base, U3D_DEV_LINK_INTR, linkint);

    if (!(linkint & SSUSB_DEV_SPEED_CHG_INTR))
        return;

    mtu3_debug("[INTR] Speed Change\n");
    u3d->speed = mtu3_get_speed(u3d);
    if (u3d->speed == SSUSB_SPEED_UNKNOWN) {
#ifdef HAL_SLEEP_MANAGER_ENABLED
        hal_sleep_manager_unlock_sleep(SLEEP_LOCK_CM33);
#endif
        mtu3_suspend(u3d);
    } else {
#ifdef HAL_SLEEP_MANAGER_ENABLED
        hal_sleep_manager_lock_sleep(SLEEP_LOCK_CM33);
#endif
        mtu3_ep0en(u3d);
        u3d->gdev->assign_descs(u3d->speed);
    }
}

static void mtu3_u2_common_isr(struct mu3d *u3d)
{
    u32 intrusb = 0;
    void *base = u3d->mac_base;

    intrusb = ssusb_readl(base, U3D_COMMON_USB_INTR);
    intrusb &= ssusb_readl(base, U3D_COMMON_USB_INTR_ENABLE);
    ssusb_writel(base, U3D_COMMON_USB_INTR, intrusb);

    if (intrusb & RESET_INTR) {
        mtu3_debug("[INTR] Reset\n");
        mtu3_status_reset(u3d);
    }

    if (intrusb & SUSPEND_INTR) {
        mtu3_debug("[INTR] Suspend\n");
        mtu3_suspend(u3d);
    }

    if (intrusb & RESUME_INTR)
        mtu3_debug("[INTR] Resume\n");
}

static void mtu3_u3_ltssm_isr(struct mu3d *u3d)
{
    u32 ltssm;
    void *base = u3d->mac_base;

    ltssm = ssusb_readl(base, U3D_LTSSM_INTR);
    ltssm &= ssusb_readl(base, U3D_LTSSM_INTR_ENABLE);
    ssusb_writel(base, U3D_LTSSM_INTR, ltssm); /* W1C */
    mtu3_debug("=== LTSSM[%x] ===\n", ltssm);

    if (ltssm & (HOT_RST_INTR | WARM_RST_INTR))
        mtu3_status_reset(u3d);

    if (ltssm & VBUS_FALL_INTR) {
        mtu3_ss_func_set(u3d, false);
        mtu3_status_reset(u3d);
    }

    if (ltssm & VBUS_RISE_INTR)
        mtu3_ss_func_set(u3d, true);

    if (ltssm & ENTER_U3_INTR)
        mtu3_suspend(u3d);

    if (ltssm & EXIT_U3_INTR)
        mtu3_debug("[INTR] Resume\n");
}

static void mtu3_bmu_isr(struct mu3d *u3d)
{
    u32 intrep;
    void *base = u3d->mac_base;

    intrep = ssusb_readl(base, U3D_EPISR);
    intrep &= ssusb_readl(base, U3D_EPIER);
    ssusb_writel(base, U3D_EPISR, intrep);
    mtu3_debug("[INTR] BMU[tx:%x, rx:%x] IER: %x\n",
               intrep & 0xffff, intrep >> 16, ssusb_readl(base, U3D_EPIER));

    /* For EP0 */
    if (intrep & 0x1) {
        mtu3_ep0_isr(u3d);
        intrep &= ~0x1;
    }

#ifndef SUPPORT_QMU
    if (intrep) {
        u32 ep_num;

        for (ep_num = 1; ep_num <= (MT_EP_NUM / 2); ep_num++) {
            if (intrep & EPT_RX(ep_num))
                mtu3_epx_isr(u3d, ep_num, USB_DIR_OUT);

            if (intrep & EPT_TX(ep_num))
                mtu3_epx_isr(u3d, ep_num, USB_DIR_IN);
        }
    }
#endif /* #ifndef SUPPORT_QMU */
}

static void mtu3_isr(hal_nvic_irq_t irq_number)
{
    uint32_t mask;
    u32 lv1_isr;
    struct mu3d *u3d = (struct mu3d *)g_u3d;
    void *base = u3d->mac_base;

    hal_nvic_save_and_set_interrupt_mask(&mask);
    lv1_isr = ssusb_readl(base, U3D_LV1ISR);  /* LV1ISR is RU */
    lv1_isr &= ssusb_readl(base, U3D_LV1IER);

    if (lv1_isr)
        mtu3_debug("[INTR] lv1_isr:0x%x\n", lv1_isr);

    if (lv1_isr & EP_CTRL_INTR)
        mtu3_link_isr(u3d);

    if (lv1_isr & MAC2_INTR)
        mtu3_u2_common_isr(u3d);

    if (lv1_isr & MAC3_INTR)
        mtu3_u3_ltssm_isr(u3d);

    if (lv1_isr & BMU_INTR)
        mtu3_bmu_isr(u3d);

    if (lv1_isr & QMU_INTR)
        mtu3_qmu_isr(u3d);

    hal_nvic_restore_interrupt_mask(mask);
}

static void mtu3_isr_init(struct mu3d *u3d)
{
    hal_nvic_status_t ret_status;

    g_u3d = u3d;
    ret_status = hal_nvic_irq_set_type(u3d->dev_irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    if (ret_status != HAL_NVIC_STATUS_OK) {
        mtu3_err("%s: set type fail\n", __func__);
        return;
    }

    ret_status = hal_nvic_set_priority(u3d->dev_irq, SSUSB_DEV_IRQ_PRIORITY);
    if (ret_status != HAL_NVIC_STATUS_OK) {
        mtu3_err("%s: set priority fail\n", __func__);
        return;
    }

    ret_status = hal_nvic_register_isr_handler(u3d->dev_irq, mtu3_isr);
    if (ret_status != HAL_NVIC_STATUS_OK) {
        mtu3_err("%s: register isr fail\n", __func__);
        return;
    }

    ret_status = hal_nvic_enable_irq(u3d->dev_irq);
    if (ret_status != HAL_NVIC_STATUS_OK) {
        mtu3_err("%s: enable isr fail\n", __func__);
        return;
    }
}

static int g_u3d_init(struct mu3d *u3d)
{
    struct mu3d_req *mreq = &u3d->ep0_mreq;
    struct udc_request *req = &mreq->req;

    u3d->ep0_state = EP0_IDLE;
    u3d->address = 0;
    u3d->test_mode = false;
    if (u3d->is_u3_ip) {
        u3d->speed = U3D_U3IP_DFT_SPEED;
        u3d->tx_fifo_addr = U3IP_TX_FIFO_START_ADDR;
        u3d->rx_fifo_addr = U3IP_RX_FIFO_START_ADDR;
    } else {
        u3d->speed = U3D_U2IP_DFT_SPEED;
        u3d->tx_fifo_addr = U2IP_TX_FIFO_START_ADDR;
        u3d->rx_fifo_addr = U2IP_RX_FIFO_START_ADDR;
    }

    u3d->ep0 = &u3d->eps[EP0];

    req->buffer = pvPortMallocNC(512);

    u3d->ep0->req = req;
    u3d->ept_alloc_table = EPT_TX(0) | EPT_RX(0);
    return mtu3_qmu_init();
}

static struct udc_endpoint *_udc_endpoint_alloc(struct mu3d *u3d, unsigned int type,
                                                unsigned char num, unsigned char in)
{
    struct udc_endpoint *ep_list = u3d->eps;
    struct udc_endpoint *ept;
    int ret;
    int i;

    /* allocated and enabled by default */
    if (num == EP0)
        return NULL;

    /*
     * find an unused slot in ep_list from EP1 to MAX_EP
     * for example, EP1 will use 2 eps, one for IN and the other for OUT
     */
    for (i = 1; i < MT_EP_NUM; i++) {
        if (ep_list[i].num == 0) /* usable */
            break;
    }
    if (i == MT_EP_NUM) /* ep has been exhausted. */
        return NULL;

    ept = &ep_list[i];
    sprintf(ept->name, "ep%d%s\r", num, in ? "in" : "out");

    ret = mtu3_gpd_ring_alloc(ept);
    if (ret) {
        mtu3_crit("%s gpd alloc failed\n", ept->name);
        return NULL;
    }

    ept->u3d = u3d;
    ept->type = type;
    ept->num = num;
    ept->in = !!in;
    ept->req = NULL;
    INIT_LIST_HEAD(&ept->req_list);

    /* store EPT_TX/RX info */
    if (ept->in)
        ept->bit = EPT_TX(num);
    else
        ept->bit = EPT_RX(num);

    /* write parameters to this ep (write to hardware) when SET_CONFIG */

    mtu3_debug("%s @%p/%p bit=%x\n", ept->name,
               ept, &ep_list, ept->bit);

    return &ep_list[i];
}

struct udc_endpoint *udc_endpoint_alloc(struct mu3d *u3d,
                                        unsigned int type, unsigned char in)
{
    struct udc_endpoint *ept;
    unsigned int n;

    if (!u3d) {
        mtu3_err("%s:error u3d is NULL\n", __func__);
        return NULL;
    }

    mtu3_debug("%s %d\n", __func__, __LINE__);

    /* udc_endpoint_alloc is used for EPx except EP0 */
    for (n = 1; n < MT_EP_NUM; n++) {
        unsigned int bit = in ? EPT_TX(n) : EPT_RX(n);

        if (u3d->ept_alloc_table & bit)
            continue;

        ept = _udc_endpoint_alloc(u3d, type, n, in);
        if (ept) {
            u3d->ept_alloc_table |= bit;
            return ept;
        }
    }

    return NULL;
}

void udc_endpoint_free(struct udc_endpoint *ept)
{
    struct mu3d *u3d;

    if (!ept) {
        mtu3_err("%s: ept is null\n", __func__);
        return;
    }

    u3d = ept->u3d;
    if (!u3d) {
        mtu3_err("%s: u3d is null\n", __func__);
        return;
    }

    if (ept->num)
        mtu3_gpd_ring_free(ept);

    ept->num = 0;
    u3d->ept_alloc_table &= ~(ept->bit);
}

void udc_endpoint_enable(struct mu3d *u3d, struct udc_endpoint *ept)
{
    mtu3_setup_ep(u3d, ept);
}

void udc_endpoint_disable(struct udc_endpoint *ept)
{
    mtu3_qmu_flush(ept);
}

struct udc_request *udc_request_alloc(struct udc_endpoint *ept)
{
    struct mu3d_req *mreq;

    mreq = pvPortMalloc(sizeof(*mreq));
    if (!mreq) {
        mtu3_err("%s %s fail\n", __func__, ept->name);
        return NULL;
    }

    memset(mreq, 0, sizeof(*mreq));
    mreq->ept = ept;
    mreq->req.xSemaphore = xSemaphoreCreateBinary();
    if (mreq->req.xSemaphore == NULL) {
        mtu3_err("%s: create semahpone fail\n", __func__);
        vPortFree(mreq);
        return NULL;
    }

    return &mreq->req;
}

void udc_request_free(struct udc_request *req)
{
    struct mu3d_req *mreq = to_mu3d_req(req);

    if (!req) {
        mtu3_err("%s: req is NULL\n", __func__);
        return;
    }

    mtu3_debug("%s: req:%p\n", __func__, req);
    vSemaphoreDelete(mreq->req.xSemaphore);
    vPortFree(mreq);
    mreq = NULL;
}

int udc_request_queue(udc_endpoint_t *ept, udc_request_t *req)
{
    struct mu3d_req *mreq;
    struct mu3d *u3d;
    int ret = 0;

    if (!ept || !req) {
        mtu3_err("%s fail\n", __func__);
        return -ENXIO;
    }

    mreq = to_mu3d_req(req);
    u3d = ept->u3d;

    if (!u3d || !u3d->usb_online) {
        mtu3_err("%s fail\n", __func__);
        return -ENXIO;
    }

    mtu3_debug("%s: %s, req=%p, buf: %p, length=%d\n", __func__,
               ept->name, req, req->buffer, req->length);

    hal_nvic_disable_irq(u3d->dev_irq);
    ept->req = req;
    mreq->ept = ept;
    mreq->actual = 0;

#ifdef SUPPORT_QMU
    if (req->length > GPD_BUF_SIZE) {
        mtu3_crit("req length > supported MAX:%d requested:%d\n",
                  GPD_BUF_SIZE, req->length);
        ret = -EINVAL;
        goto out;
    }

    if (mtu3_prepare_transfer(ept)) {
        ret = -ENOMEM;
        goto out;
    }

    list_add_tail(&mreq->list, &ept->req_list);
    mtu3_insert_gpd(ept, mreq);
    mtu3_qmu_resume(ept);
#else /* #ifdef SUPPORT_QMU */
    /*
     * PIO mode:
     * when class driver shares a buffer to TX and RX data,
     * mtu3 sends a data to host, then host sends a data back immediately,
     * cause EP TX and RX interrupts arise at the same time,
     * but the buffer is using by the TX, so no buffer for RX to receive data.
     * To fix the issue:
     * disable EP RX interrupt by default, enable it when queue RX
     * request and disable it again when complete the request.
     */
    if (ept->in)
        mtu3_epx_write(u3d, ept);
    else
        ssusb_setbits(u3d->mac_base, U3D_EPIESR, EP_RXISR(ept->num));  /* W1S */
#endif /* #ifdef SUPPORT_QMU */

out:
    hal_nvic_enable_irq(u3d->dev_irq);
    return ret;
}

void udc_pullup(struct mu3d *u3d, bool enable)
{
    if (enable)
        mtu3_soft_connect(u3d);
    else
        mtu3_soft_disconnect(u3d);
}

/* Switch on the UDC */
int udc_enable(struct mu3d *u3d)
{
    if (!u3d) {
        mtu3_err("%s fail\n", __func__);
        return -1;
    }

    g_u3d_init(u3d);
    mtu3_reg_init(u3d);
    mdelay(1);
    mtu3_isr_init(u3d);

    return 0;
}

/* Switch off the UDC */
int udc_disable(struct mu3d *u3d)
{
    struct udc_request *req;

    if (!u3d) {
        mtu3_err("%s fail\n", __func__);
        return -1;
    }

    req = &u3d->ep0_mreq.req;
    if (req->buffer) {
        vPortFreeNC(req->buffer);
        req->buffer = NULL;
    }
    mtu3_soft_disconnect(u3d);
    return 0;
}

