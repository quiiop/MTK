/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2010 Patrick Georgi
 * Copyright (C) 2013 secunet Security Networks AG
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "hal_nvic.h"
#include "hal_nvic_internal.h"

#ifdef HAL_CACHE_MODULE_ENABLED
#include "hal_cache.h"
#include "memory_map.h"
#endif /* #ifdef HAL_CACHE_MODULE_ENABLED */

#include <usb_phy.h>
#include <virtual.h>
#include <xhci_private.h>
#include <xhci.h>
#include <ssusb_hw_regs.h>

static void xhci_start(hci_t *controller);
static void xhci_stop(hci_t *controller);
static void xhci_reset(hci_t *controller);
static void xhci_reinit(hci_t *controller);
static void xhci_enable_irq(hci_t *const controller);
static int xhci_cmd_queue_init(hci_t *const controller);
static void xhci_shutdown(hci_t *controller);
static int xhci_bulk(endpoint_t *const ep, struct urb *urb);
static int xhci_control(usbdev_t *dev, struct urb *urb);
static int xhci_isochronous(endpoint_t *const ep, struct urb *urb);
static void *xhci_create_intr_queue(endpoint_t *ep, int reqsize,
                                    int reqcount, int reqtiming);
static void xhci_destroy_intr_queue(endpoint_t *ep, void *queue);
static u8 *xhci_poll_intr_queue(void *queue);

#define ADDR_NUMB 32

typedef struct {
    void *org_addr;
    void *align_addr;
} xhci_address;

static xhci_address alloc_address[ADDR_NUMB];

/*
 * Some structures must not cross page boundaries. To get this,
 * we align them by their size (or the next greater power of 2).
 */
void *xhci_align(const size_t min_align, const size_t size)
{
    void *org_address = NULL;
    void *align_address = NULL;
    int i;
    u32 align = min_align - 1;
    //org_address = pvPortMallocNC(size + align);
    org_address = SYS_MALLOC_NC(size + align);
    if (org_address == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return NULL;
    }

    align_address = (void *)(((u32)org_address + align) & (~align));

    for (i = 0; i < ADDR_NUMB; i++) {
        if (alloc_address[i].org_addr == NULL) {
            alloc_address[i].org_addr = org_address;
            alloc_address[i].align_addr = align_address;
            usb_debug("%s: org_addr[%d]:%p align_addr[%d]:%p\n",
                      __func__, i, alloc_address[i].org_addr,
                      i, alloc_address[i].align_addr);
            break;
        }
    }

    if (i == ADDR_NUMB) {
        usb_err("%s: ADDR_NUMB overflow\r", __func__);
    }

    return align_address;
}

void xhci_align_free(void *align)
{
    int i;
    if (align == NULL) {
        usb_err("%s: WARNNING free NULL\n", __func__);
        return;
    }

    for (i = 0; i < ADDR_NUMB; i++) {
        if (alloc_address[i].align_addr == align) {
            usb_debug("%s:free org_addr[%d]:%p align_addr[%d]:%p\n",
                      __func__, i, alloc_address[i].org_addr, i,
                      alloc_address[i].align_addr);
            //vPortFreeNC(alloc_address[i].org_addr);
            SYS_FREE_NC(alloc_address[i].org_addr);
            alloc_address[i].org_addr = NULL;
            alloc_address[i].align_addr = NULL;
            break;
        }
    }

    if (i == ADDR_NUMB) {
        usb_err("%s: ADDR_NUMB overflow\r", __func__);
    }
}

transfer_ring_t *xhci_alloc_cycle_ring(void)
{
    transfer_ring_t *tr = pvPortMalloc(sizeof(*tr));
    if (!tr) {
        usb_err("%s: alloc ring fail\n", __func__);
        return NULL;
    }

    memset(tr, 0, sizeof(*tr));
    tr->trb_queue = xQueueCreate(TRANSFER_RING_SIZE, (sizeof(struct urb *)));
    if (!tr->trb_queue) {
        usb_err("%s: alloc tr queue fail\n", __func__);
        return NULL;
    }
    return tr;
}

transfer_ring_t *xhci_alloc_isoc_ring(void)
{
    transfer_ring_t *tr = pvPortMalloc(sizeof(*tr));
    if (!tr) {
        usb_err("%s: alloc ring fail\n", __func__);
        return NULL;
    }

    memset(tr, 0, sizeof(*tr));
    tr->isoc_queue = xQueueCreate(ISOC_PACKET_NUM, (sizeof(struct xhci_td *)));
    if (!tr->isoc_queue) {
        usb_err("%s: alloc isoc_queue queue fail\n", __func__);
        return NULL;
    }
    return tr;
}

void xhci_clear_trb(trb_t *const trb, const int pcs)
{
    trb->ptr_low    = 0;
    trb->ptr_high   = 0;
    trb->status = 0;
    trb->control = 0;
    trb->control |= !pcs;
}

void xhci_init_cycle_ring(transfer_ring_t *const tr, const size_t ring_size)
{
    memset((void *)tr->ring, 0, ring_size * sizeof(*tr->ring));
    TRB_SET(TT, &tr->ring[ring_size - 1], TRB_LINK);
    TRB_SET(TC, &tr->ring[ring_size - 1], 1);
    /* only one segment that points to itself */
    tr->ring[ring_size - 1].ptr_low = virt_to_phys(tr->ring);

    tr->pcs = 1;
    tr->cur = tr->ring;
}

static int xhci_wait_ready(xhci_t *const xhci)
{
    usb_debug("Waiting for controller to be ready... ");
    if (xhci_handshake(xhci->opreg->usbsts, USBSTS_CNR, 0, 100000L)) {
        usb_err("Waiting for controller ready timeout!\n");
        return -1;
    }
    usb_debug("ok.\n");
    return 0;
}

static void xhci_isr_init(unsigned int irq, void *data)
{
    hal_nvic_status_t ret_status;

    ret_status = hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        usb_err("%s: set type fail\n", __func__);
        return;
    }

    ret_status = hal_nvic_set_priority(irq, SSUSB_XHCI_IRQ_PRIORITY);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        usb_err("%s set priority fail\n", __func__);
        return;
    }

    ret_status = hal_nvic_register_isr_handler(irq, xhci_irq);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        usb_err("%s register isr fail\n", __func__);
        return;
    }

    ret_status = hal_nvic_enable_irq(irq);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        usb_err("%s enable isr fail\n", __func__);
        return;
    }

    return;
}

hci_t *xhci_init(void *physical_bar, int irq, void *data)
{
    usb_debug("xhci address:%p\n", physical_bar);

    /* First, allocate and initialize static controller structures */
    hci_t *const controller = new_controller();
    if (!controller)
        return NULL;

    controller->start       = xhci_start;
    controller->stop        = xhci_stop;
    controller->reset       = xhci_reset;
    controller->init        = xhci_reinit;
    controller->shutdown        = xhci_shutdown;
    controller->bulk        = xhci_bulk;
    controller->control     = xhci_control;
    controller->isochronous     = xhci_isochronous;
    controller->set_address     = xhci_set_address;
    controller->finish_device_config = xhci_finish_device_config;
    controller->destroy_device  = xhci_destroy_dev;
    controller->create_intr_queue   = xhci_create_intr_queue;
    controller->destroy_intr_queue  = xhci_destroy_intr_queue;
    controller->poll_intr_queue = xhci_poll_intr_queue;

    controller->reg_base = physical_bar;
    controller->instance = pvPortMalloc(sizeof(xhci_t));
    memset(controller->instance, 0, sizeof(xhci_t));
    xhci_t *const xhci = (xhci_t *)controller->instance;
    usb_debug("instance:%x \n", (u32)(controller->instance));

    init_device_entry(controller, 0);
    xhci->roothub = controller->devices[0];
    xhci->cr.ring = xhci_align(64, COMMAND_RING_SIZE * sizeof(trb_t));
    xhci->er.ring = xhci_align(64, EVENT_RING_SIZE * sizeof(trb_t));
    xhci->ev_ring_table = xhci_align(64, sizeof(erst_entry_t));
    if (!xhci->roothub || !xhci->cr.ring ||
        !xhci->er.ring || !xhci->ev_ring_table) {
        usb_err("Out of memory\n");
        goto _free_xhci;
    }
    xhci->capreg    = phys_to_virt(physical_bar);
    xhci->opreg = ((void *)xhci->capreg) + xhci->capreg->caplength;
    xhci->hcrreg    = ((void *)xhci->capreg) + xhci->capreg->rtsoff;
    xhci->dbreg = ((void *)xhci->capreg) + xhci->capreg->dboff;
    usb_debug("regbase: 0x%p\n", physical_bar);
    usb_debug("caplen:  0x%x\n", xhci->capreg->caplength);
    usb_debug("rtsoff:  0x%x\n", xhci->capreg->rtsoff);
    usb_debug("dboff:   0x%x\n", xhci->capreg->dboff);

    usb_debug("hciversion: %x.%x\n",
              xhci->capreg->hciver_hi, xhci->capreg->hciver_lo);
    if ((xhci->capreg->hciversion < 0x96) ||
        (xhci->capreg->hciversion > 0x110)) {
        usb_debug("Unsupported xHCI version\n");
        goto _free_xhci;
    }

    if (xhci_cmd_queue_init(controller))
        goto _free_xhci;

    usb_debug("context size: %dB\n", CTXSIZE(xhci));
    usb_debug("maxslots: 0x%02x\n", xhci->capreg->MaxSlots);
    usb_debug("maxports: 0x%02x\n", xhci->capreg->MaxPorts);
    const unsigned pagesize = xhci->opreg->pagesize << 12;
    usb_crit("pagesize: 0x%04x\n", pagesize);

    /*
     * We haven't touched the hardware yet. So we allocate all dynamic
     * structures at first and can still chicken out easily if we run out
     * of memory.
     */
    xhci->max_slots_en = xhci->capreg->MaxSlots;
    xhci->dcbaa = xhci_align(64, (xhci->max_slots_en + 1) * sizeof(u64));
    xhci->dev = pvPortMalloc((xhci->max_slots_en + 1) * sizeof(*xhci->dev));
    if (!xhci->dcbaa || !xhci->dev) {
        usb_debug("Out of memory\n");
        goto _free_xhci;
    }
    memset(xhci->dcbaa, 0x00, (xhci->max_slots_en + 1) * sizeof(u64));
    memset(xhci->dev, 0x00, (xhci->max_slots_en + 1) * sizeof(*xhci->dev));

    usb_debug("xhci->dcbaa:0x%p\n", xhci->dcbaa);
    usb_debug("xhci->dev:0x%p\n", xhci->dev);
    xhci->priv = data;
    xhci->dma_buffer = xhci_align(64, DMA_SIZE);
    if (!xhci->dma_buffer) {
        usb_err("Not enough memory for DMA bounce buffer\n");
        goto _free_xhci_structs;
    }

    /* Now start working on the hardware */
    if (xhci_wait_ready(xhci))
        goto _free_xhci_structs;

    /* TODO: Check if BIOS claims ownership (and hand over) */

    xhci_reset(controller);
    xhci_reinit(controller);

    xhci->roothub->controller = controller;
    xhci->roothub->init = xhci_rh_init;
    xhci->roothub->init(xhci->roothub);

    xhci_isr_init(irq, (void *)xhci);
    xhci_enable_irq(controller);
    return controller;

_free_xhci_structs:
    xhci_align_free(xhci->dma_buffer);
    xhci_align_free(xhci->dcbaa);
_free_xhci:
    xhci_align_free((void *)xhci->ev_ring_table);
    xhci_align_free((void *)xhci->er.ring);
    xhci_align_free((void *)xhci->cr.ring);
    vPortFree(xhci->roothub);
    vPortFree(xhci->dev);
    vPortFree(xhci);
    /* _free_controller: */
    detach_controller(controller);
    vPortFree(controller);
    return NULL;
}


static void xhci_reset(hci_t *const controller)
{
    xhci_t *const xhci = XHCI_INST(controller);

    xhci_stop(controller);
    xhci->opreg->usbcmd |= USBCMD_HCRST;
    usb_debug("Resetting controller... ");
    if (xhci_handshake(xhci->opreg->usbcmd, USBCMD_HCRST, 0, 100000L))
        usb_err("Resetting controller timeout!\n");
    else
        usb_debug("ok.\n");
}

static void xhci_enable_irq(hci_t *const controller)
{
    u32 status;
    xhci_t *const xhci = XHCI_INST(controller);

    /* enable xhci irq */
    xhci->hcrreg->intrrs[0].imod &= ~0xffff;
    xhci->hcrreg->intrrs[0].imod |= (5000 / 250) & 0xffff;
    xhci->opreg->usbcmd |= USBCMD_INTE;

    status = readl(&xhci->hcrreg->intrrs[0].iman);
    status &= 0xfffffffe;
    status |= 0x2;
    writel(status, &xhci->hcrreg->intrrs[0].iman);
    usb_debug("xhci->hcrreg->intrrs[0].iman:%p \n",
              &xhci->hcrreg->intrrs[0].iman);
}

static void xhci_reinit(hci_t *controller)
{
    xhci_t *const xhci = XHCI_INST(controller);

    if (xhci_wait_ready(xhci))
        return;

    /* Enable all available slots */
    xhci->opreg->config = xhci->max_slots_en;

    /* Set DCBAA */
    xhci->opreg->dcbaap_lo = virt_to_phys(xhci->dcbaa);
    xhci->opreg->dcbaap_hi = 0;

    /* Initialize command ring */
    xhci_init_cycle_ring(&xhci->cr, COMMAND_RING_SIZE);
    usb_debug("command ring @%p (0x%08lx)\n",
              xhci->cr.ring, virt_to_phys(xhci->cr.ring));
    xhci->opreg->crcr_lo = virt_to_phys(xhci->cr.ring) | CRCR_RCS;
    xhci->opreg->crcr_hi = 0;

    /* Make sure interrupts are disabled */
    xhci->opreg->usbcmd &= ~USBCMD_INTE;

    /* Initialize event ring */
    xhci_reset_event_ring(&xhci->er);
    usb_debug("event ring @%p (0x%08lx)\n",
              xhci->er.ring, virt_to_phys(xhci->er.ring));
    usb_debug("ERST Max: 0x%x ->  0x%x entries\n",
              xhci->capreg->ERST_Max, 1 << xhci->capreg->ERST_Max);
    memset((void *)xhci->ev_ring_table, 0x00, sizeof(erst_entry_t));
    xhci->ev_ring_table[0].seg_base_lo = virt_to_phys(xhci->er.ring);
    xhci->ev_ring_table[0].seg_base_hi = 0;
    xhci->ev_ring_table[0].seg_size = EVENT_RING_SIZE;

    /* pass event ring table to hardware */
    mb();
    /* Initialize primary interrupter */
    xhci->hcrreg->intrrs[0].erstsz = 1;
    xhci_update_event_dq(xhci);
    /* erstba has to be written at last */
    xhci->hcrreg->intrrs[0].erstba_lo = virt_to_phys(xhci->ev_ring_table);
    xhci->hcrreg->intrrs[0].erstba_hi = 0;

    xhci_start(controller);
}

static void xhci_shutdown(hci_t *const controller)
{
    if (controller == 0)
        return;

    detach_controller(controller);

    xhci_t *const xhci = XHCI_INST(controller);

    xhci_align_free(xhci->dma_buffer);
    xhci_align_free(xhci->dcbaa);
    vPortFree(xhci->dev);
    xhci_align_free((void *)xhci->ev_ring_table);
    xhci_align_free((void *)xhci->er.ring);
    xhci_align_free((void *)xhci->cr.ring);
    vPortFree(xhci);
    vPortFree(controller);
}

static void xhci_start(hci_t *controller)
{
    xhci_t *const xhci = XHCI_INST(controller);

    xhci->opreg->usbcmd |= USBCMD_RS;
    if (xhci_handshake(xhci->opreg->usbsts, USBSTS_HCH, 0, 100000L))
        usb_err("Controller didn't start within 1s\n");
}

static void xhci_stop(hci_t *controller)
{
    xhci_t *const xhci = XHCI_INST(controller);

    xhci->opreg->usbcmd &= ~USBCMD_RS;
    if (xhci_handshake(xhci->opreg->usbsts, USBSTS_HCH, 0, 100000L))
        usb_err("Controller didn't halt within 1s\n");
}

u32 xhci_port_state_to_neutral(u32 state)
{
    return (state & XHCI_PORT_RO) | (state & XHCI_PORT_RWS);
}

static int xhci_reset_endpoint(usbdev_t *const dev, endpoint_t *const ep)
{
    xhci_t *const xhci = XHCI_INST(dev->controller);
    const int slot_id = dev->address;
    const int ep_id = ep ? xhci_ep_id(ep) : 1;
    epctx_t *const epctx = xhci->dev[slot_id].ctx.ep[ep_id];

    usb_debug("Resetting ID %d EP %d (ep state: %d)\n",
              slot_id, ep_id, EC_GET(STATE, epctx));

    /* Run Reset Endpoint Command if the EP is in Halted state */
    if (EC_GET(STATE, epctx) == 2) {
        const int cc = xhci_cmd_reset_endpoint(xhci, slot_id, ep_id);
        if (cc != CC_SUCCESS) {
            usb_err("Reset Endpoint Command failed: %d\n", cc);
            return 1;
        }
    }

    /* Clear TT buffer for bulk and control endpoints behind a TT */
    const int hub = dev->hub;
    if (hub && dev->speed < HIGH_SPEED &&
        dev->controller->devices[hub]->speed == HIGH_SPEED) {
        /* TODO */;
    }

    /* Reset transfer ring if the endpoint is in the right state */
    const unsigned ep_state = EC_GET(STATE, epctx);
    if (ep_state == 3 || ep_state == 4) {
        transfer_ring_t *const tr =
            xhci->dev[slot_id].transfer_rings[ep_id];
        const int cc = xhci_cmd_set_tr_dq(xhci, slot_id, ep_id,
                                          tr->ring, 1);
        if (cc != CC_SUCCESS) {
            usb_err("Set TR Dequeue Command failed: %d\n", cc);
            return 1;
        }
        xhci_init_cycle_ring(tr, TRANSFER_RING_SIZE);
    }

    usb_debug("Finished resetting ID %d EP %d (ep state: %d)\n",
              slot_id, ep_id, EC_GET(STATE, epctx));

    return 0;
}

static bool trb_is_link(trb_t *cur)
{
    return TRB_GET(TT, cur) == TRB_LINK;
}

static void inc_enq(transfer_ring_t *const tr, bool more_trbs_coming)
{
    u32 chain, trb_cycle;
    chain = TRB_GET(CH, tr->cur);
    ++(tr->cur);

    while (trb_is_link(tr->cur)) {
        if (!chain && !more_trbs_coming)
            break;

        TRB_SET(CH, tr->cur, chain);
        trb_cycle = TRB_GET(C, tr->cur);
        trb_cycle ^= 1;
        TRB_SET(C, tr->cur, trb_cycle);

        if (TRB_GET(TC, tr->cur))
            tr->pcs ^= 1;

        tr->cur = phys_to_virt(tr->cur->ptr_low);
    }
}

static u32 xhci_td_remainder(int running_total,
                             int trb_buff_len,
                             unsigned int total_packet_count,
                             int maxpacketsize,
                             unsigned int num_trbs_left)
{
    int packets_transferred;
    if (num_trbs_left == 0 || (running_total == 0 && trb_buff_len == 0))
        return 0;

    packets_transferred = (running_total + trb_buff_len) / maxpacketsize;
    if ((total_packet_count - packets_transferred) > 31) {
        usb_err("%s td size overflow \n", __func__);
        return 31;
    }

    usb_debug("%s td size:%x\n", __func__, (total_packet_count - packets_transferred));

    return (total_packet_count - packets_transferred);
}

static int prepare_ring(transfer_ring_t *const tr)
{
    trb_t *cur = tr->cur;
    u32 trb_cycle;

    while (trb_is_link(cur)) {
        TRB_SET(CH, cur, 0);
        trb_cycle = TRB_GET(C, cur);
        trb_cycle ^= 1;
        TRB_SET(C, cur, trb_cycle);
        if (TRB_GET(TC, tr->cur))
            tr->pcs ^= 1;

        tr->cur = phys_to_virt(tr->cur->ptr_low);
        cur = tr->cur;
    }

    return 0;
}

static void xhci_enqueue_trb(transfer_ring_t *const tr)
{
    const int chain = TRB_GET(CH, tr->cur);
    TRB_SET(C, tr->cur, tr->pcs);
    ++tr->cur;

    while (TRB_GET(TT, tr->cur) == TRB_LINK) {
        usb_debug("Handling LINK pointer\n");
        const int tc = TRB_GET(TC, tr->cur);
        TRB_SET(CH, tr->cur, chain);
        mb();
        TRB_SET(C, tr->cur, tr->pcs);
        tr->cur = phys_to_virt(tr->cur->ptr_low);
        if (tc)
            tr->pcs ^= 1;
    }
}

static void xhci_ring_doorbell(endpoint_t *const ep)
{
    /* Ensure all TRB changes are written to memory. */
    mb();
    XHCI_INST(ep->dev->controller)->dbreg[ep->dev->address] =
        xhci_ep_id(ep);
}

static void giveback_first_trb(int start_cycle, trb_t *start_trb)
{
    TRB_SET(C, start_trb, start_cycle);

    usb_debug("%s %d cur:%p ptr_low:%x ptr_high:%x status:%x control:%x\n",
              __func__, __LINE__, start_trb,
              start_trb->ptr_low, start_trb->ptr_high,
              start_trb->status, start_trb->control);
    return;
}

static int xhci_queue_control_td(transfer_ring_t *const tr, struct urb *urb)
{
    unsigned long data = urb->transfer_dma;
    int dalen = urb->transfer_buffer_length;
    direction_t dir = urb->in;
    unsigned char *devreq = urb->setup_packet;

    /* Fill and enqueue setup TRB */
    trb_t *const setup = tr->cur;
    u32 start_cycle = tr->pcs;

    xhci_clear_trb(setup, tr->pcs);
    if (start_cycle == 0)
        TRB_SET(C, tr->cur, TRB_CYCLE);

    setup->ptr_low = ((u32 *)devreq)[0];
    setup->ptr_high = ((u32 *)devreq)[1];
    TRB_SET(TL, setup, 8);
    TRB_SET(TRT, setup, (dalen)
            ? ((dir == OUT) ? TRB_TRT_OUT_DATA : TRB_TRT_IN_DATA)
            : TRB_TRT_NO_DATA);
    TRB_SET(TT, setup, TRB_SETUP_STAGE);
    TRB_SET(IDT, setup, 1);
    inc_enq(tr, true);

    /* Fill and enqueue data TRB */
    if (dalen > 0) {
        trb_t *const trb = tr->cur;
        xhci_clear_trb(trb, tr->pcs);
        trb->ptr_low = data;
        TRB_SET(TL, trb, dalen);
        TRB_SET(DIR, trb, dir);
        TRB_SET(TT, trb, TRB_DATA_STAGE);
        TRB_SET(C, tr->cur, tr->pcs);
        inc_enq(tr, true);
    }

    /* Fill and enqueue status TRB */
    trb_t *const status = tr->cur;
    xhci_clear_trb(status, tr->pcs);
    TRB_SET(DIR, status, (dir == OUT) ? TRB_DIR_IN : TRB_DIR_OUT);
    TRB_SET(TT, status, TRB_STATUS_STAGE);
    TRB_SET(IOC, status, 1);
    TRB_SET(C, tr->cur, tr->pcs);
    inc_enq(tr, true);

    giveback_first_trb(start_cycle, setup);

    if (pdPASS != xQueueSend(tr->trb_queue, &urb, 0)) {
        usb_err("%s: send urb to queue fail\n", __func__);
        return -1;
    }

    return 0;
}

static int xhci_queue_bulk_td(transfer_ring_t *const tr, struct urb *urb)
{
    int dir = urb->ep->direction;
    int maxp = usb_endpoint_maxp(urb->ep);
    unsigned long addr = urb->transfer_dma;
    u32 length = urb->transfer_buffer_length;
    u32 trb_buff_len;
    u32 num_trbs = 0;
    int ret;
    trb_t *start_trb;
    u32 start_cycle;
    u32 total_packet_count;
    bool first_trb = true;
    u32 running_total;

#ifdef HAL_CACHE_MODULE_ENABLED
    hal_cache_flush_multiple_cache_lines(urb->transfer_dma, length);
    addr = HAL_CACHE_VIRTUAL_TO_PHYSICAL(urb->transfer_dma);
#endif /* #ifdef HAL_CACHE_MODULE_ENABLED */

    running_total = TRB_MAX_BUFF_SIZE -
                    (urb->transfer_dma & (TRB_MAX_BUFF_SIZE - 1));
    trb_buff_len = running_total;
    running_total &= TRB_MAX_BUFF_SIZE - 1;
    if (running_total != 0 || urb->transfer_buffer_length == 0)
        num_trbs++;
    while (running_total < urb->transfer_buffer_length) {
        num_trbs++;
        running_total += TRB_MAX_BUFF_SIZE;
    }

    ret = prepare_ring(tr);
    if (ret)
        return ret;

    running_total = 0;
    start_trb = tr->cur;
    start_cycle = tr->pcs;
    total_packet_count = DIV_ROUND_UP(length, maxp);
    if (trb_buff_len > length)
        trb_buff_len = length;

    do {
        u32 remainder = 0;
        prepare_ring(tr);
        xhci_clear_trb(tr->cur, tr->pcs);
        if (first_trb) {
            first_trb = false;
            if (start_cycle == 0)
                TRB_SET(C, tr->cur, TRB_CYCLE);
        } else {
            TRB_SET(C, tr->cur, tr->pcs);
        }

        if (num_trbs > 1)
            TRB_SET(CH, tr->cur, 1);
        else
            TRB_SET(IOC, tr->cur, 1);

        if (dir == IN)
            TRB_SET(ISP, tr->cur, 1);

        remainder = xhci_td_remainder(running_total, trb_buff_len,
                                      total_packet_count, maxp,
                                      num_trbs - 1);
        tr->cur->ptr_low = addr;
        TRB_SET(TDS, tr->cur, remainder);
        TRB_SET(TL, tr->cur, trb_buff_len);
        TRB_SET(TT, tr->cur, TRB_NORMAL);

        inc_enq(tr, (num_trbs > 1));
        --num_trbs;
        running_total += trb_buff_len;
        addr += trb_buff_len;
        trb_buff_len = MIN((length - running_total), TRB_MAX_BUFF_SIZE);
    } while (running_total < length);

#ifdef HAL_CACHE_MODULE_ENABLED
    hal_cache_invalidate_multiple_cache_lines(urb->transfer_dma, length);
#endif /* #ifdef HAL_CACHE_MODULE_ENABLED */

    giveback_first_trb(start_cycle, start_trb);
    if (pdPASS != xQueueSend(tr->trb_queue, &urb, 0)) {
        usb_err("%s: send urb to queue fail\n", __func__);
        return -1;
    }

    return 0;
}

static unsigned int count_isoc_trbs_needed(struct urb *urb, int i)
{
    unsigned long addr, len;
    unsigned int num_trbs;

    addr = (u64)(urb->transfer_dma + urb->iso_frame_desc[i].offset);
    len = urb->iso_frame_desc[i].length;

    num_trbs = DIV_ROUND_UP(len + (addr & (TRB_MAX_BUFF_SIZE - 1)),
                            TRB_MAX_BUFF_SIZE);
    if (num_trbs == 0)
        num_trbs++;

    return num_trbs;
}

/* Current only support hs isoc, ss/ssp isoc needs to modify the ep
 * descriptor structure to store information about the ss/ssp info.
 * Fix Me
 */
static int xhci_queue_isoc_td(transfer_ring_t *const tr, struct urb *urb)
{
    int dir = urb->ep->direction;
    int max_pkt = usb_endpoint_maxp(urb->ep);
    u32 length = urb->transfer_buffer_length;
    int num_tds = urb->number_of_packets;
    unsigned long start_addr, addr;
    int running_total, trb_buff_len, td_len, td_remain_len, ret;
    int i, j, trbs_per_td;
    trb_t *start_trb;
    u32 start_cycle;
    bool first_trb;
    struct urb_priv *urb_priv;
    struct xhci_td *td;

#ifdef HAL_CACHE_MODULE_ENABLED
    hal_cache_flush_multiple_cache_lines(urb->transfer_dma, length);
    addr = HAL_CACHE_VIRTUAL_TO_PHYSICAL(urb->transfer_dma);
#endif /* #ifdef HAL_CACHE_MODULE_ENABLED */

    start_addr = urb->transfer_dma;
    urb_priv = urb->hcpriv;
    urb_priv->num_tds = num_tds;
    urb_priv->num_tds_done = 0;

    ret = prepare_ring(tr);
    if (ret)
        return ret;

    start_trb = tr->cur;
    start_cycle = tr->pcs;

    for (i = 0; i < num_tds; i++) {
        unsigned int total_pkt_count;
        unsigned int burst_count, last_burst_pkt_count;

        first_trb = true;
        running_total = 0;
        addr = start_addr + urb->iso_frame_desc[i].offset;
        td_len = urb->iso_frame_desc[i].length;
        td_remain_len = td_len;
        total_pkt_count = DIV_ROUND_UP(td_len, max_pkt);

        if (total_pkt_count == 0)
            total_pkt_count++;

        /* Just support hs speed. USB 2.0 devices can only do one "burst",
         * so the last burst packet count is equal to the total number
         * of packets in the TD.
         */
        burst_count = 0;
        last_burst_pkt_count = total_pkt_count - 1;
        prepare_ring(tr);

        trbs_per_td = count_isoc_trbs_needed(urb, i);
        td = &urb_priv->td[i];
        td->first_trb = tr->cur;
        td->last_trb = tr->cur;
        td->urb = urb;
        td->num_trb = trbs_per_td;

        xhci_clear_trb(tr->cur, tr->pcs);
        TRB_SET(TT, tr->cur, TRB_ISOC);
        TRB_SET(TLBPC, tr->cur, last_burst_pkt_count);
        TRB_SET(TBC, tr->cur, burst_count);
        TRB_SET(SIA, tr->cur, 1);
        TRB_SET(C, tr->cur, (i ? tr->pcs : !start_cycle));

        for (j = 0; j < td->num_trb; j++) {
            u32 remainder = 0;

            prepare_ring(tr);
            if (!first_trb) {
                xhci_clear_trb(tr->cur, tr->pcs);
                TRB_SET(TT, tr->cur, TRB_NORMAL);
                TRB_SET(C, tr->cur, tr->pcs);
            }

            if (dir == IN)
                TRB_SET(ISP, tr->cur, 1);

            /* Set the chain bit for all except the last TRB  */
            if (j < td->num_trb - 1)
                TRB_SET(CH, tr->cur, 1);
            else {
                td->last_trb = tr->cur;
                TRB_SET(IOC, tr->cur, 1);
            }
            /* Calculate TRB length */
            trb_buff_len = TRB_BUFF_LEN_UP_TO_BOUNDARY(addr);
            if (trb_buff_len > td_remain_len)
                trb_buff_len = td_remain_len;

            /* Set the TRB length, TD size, & interrupter fields. */
            remainder = xhci_td_remainder(running_total, trb_buff_len,
                                          total_pkt_count, max_pkt,
                                          td->num_trb - 1);

            tr->cur->ptr_low = addr;
            TRB_SET(TL, tr->cur, trb_buff_len);
            TRB_SET(TDS, tr->cur, remainder);
            first_trb = false;

            inc_enq(tr, (trbs_per_td > 1));
            --trbs_per_td;
            running_total += trb_buff_len;
            addr += trb_buff_len;
            td_remain_len -= trb_buff_len;
        }

        /* Check TD length */
        if (running_total != td_len) {
            usb_err("ERROR: ISOC TD length unmatch! \n");
            ret = -1;
            goto cleanup;
        }
    }

#ifdef HAL_CACHE_MODULE_ENABLED
    hal_cache_invalidate_multiple_cache_lines(urb->transfer_dma, length);
#endif /* #ifdef HAL_CACHE_MODULE_ENABLED */

    for (i = 0; i < num_tds; i++) {
        td = &urb_priv->td[i];
        if (pdPASS != xQueueSend(tr->isoc_queue, &td, 0)) {
            usb_err("%s: send urb to queue fail\n", __func__);
            ret = -1;
            goto cleanup;
        }
    }
    giveback_first_trb(start_cycle, start_trb);

    return 0;

cleanup:
    usb_err("%s: urb queue fail\n", __func__);
    urb_priv->td[0].last_trb = tr->cur;
    tr->cur = urb_priv->td[0].first_trb;
    tr->pcs = start_cycle;
    xhci_clear_trb(tr->cur, tr->pcs);

    for (i = 0; i < num_tds; i++) {
        if (0 == uxQueueMessagesWaiting(tr->isoc_queue))
            return ret;
        else
            xQueueReceive(tr->isoc_queue, &td, 0);
    }

    usb_err("%s: fail, never go this flow\n", __func__);
    return ret;
}


static int xhci_control(usbdev_t *dev, struct urb *urb)
{
    int ret;
    unsigned long data = urb->transfer_dma;
    int dalen = urb->transfer_buffer_length;
    xhci_t *const xhci = XHCI_INST(dev->controller);
    epctx_t *const epctx = xhci->dev[dev->address].ctx.ep0;
    transfer_ring_t *const tr = xhci->dev[dev->address].transfer_rings[1];

    const size_t off = (size_t)data & 0xffff;
    if ((off + dalen) > ((TRANSFER_RING_SIZE - 4) << 16)) {
        usb_err("%s: Error, transfer size crosses 64KB boundaries\n",
                __func__);
        return -1;
    }

    /* Reset endpoint if it's not running */
    const unsigned ep_state = EC_GET(STATE, epctx);
    if (ep_state > 1) {
        if (xhci_reset_endpoint(dev, NULL))
            return -1;
    }

    ret = xhci_queue_control_td(tr, urb);
    if (ret)
        return -1;

    xhci_ring_doorbell(&dev->endpoints[0]);
    return 0;
}

static int xhci_bulk(endpoint_t *const ep, struct urb *urb)
{
    int ret;
    unsigned long data = urb->transfer_dma;
    int dalen = urb->transfer_buffer_length;
    xhci_t *const xhci = XHCI_INST(ep->dev->controller);
    const int slot_id = ep->dev->address;
    const int ep_id = xhci_ep_id(ep);
    epctx_t *const epctx = xhci->dev[slot_id].ctx.ep[ep_id];
    transfer_ring_t *const tr = xhci->dev[slot_id].transfer_rings[ep_id];

    const size_t off = (size_t)data & 0xffff;
    if ((off + dalen) > ((TRANSFER_RING_SIZE - 2) << 16)) {
        usb_err("%s: Unsupported bulk transfer size\n", __func__);
        return -1;
    }

    /* Reset endpoint if it's not running */
    const unsigned ep_state = EC_GET(STATE, epctx);
    if (ep_state > 1) {
        if (xhci_reset_endpoint(ep->dev, ep))
            return -1;
    }

    ret = xhci_queue_bulk_td(tr, urb);
    if (ret)
        return -1;

    xhci_ring_doorbell(ep);
    return 0;
}

static int xhci_isochronous(endpoint_t *const ep, struct urb *urb)
{
    int ret;
    unsigned long data = urb->transfer_dma;
    int dalen = urb->transfer_buffer_length;
    xhci_t *const xhci = XHCI_INST(ep->dev->controller);
    const int slot_id = ep->dev->address;
    const int ep_id = xhci_ep_id(ep);
    epctx_t *const epctx = xhci->dev[slot_id].ctx.ep[ep_id];
    transfer_ring_t *const tr = xhci->dev[slot_id].transfer_rings[ep_id];

    const size_t off = (size_t)data & 0xffff;
    if ((off + dalen) > ((TRANSFER_RING_SIZE - 2) << 16)) {
        usb_err("%s: Unsupported isoc transfer size\n", __func__);
        return -1;
    }

    memset(urb->buffer, 0x00, urb->transfer_buffer_length);
    /* Reset endpoint if it's not running */
    const unsigned ep_state = EC_GET(STATE, epctx);
    if (ep_state > 1) {
        if (xhci_reset_endpoint(ep->dev, ep))
            return -1;
    }

    ret = xhci_queue_isoc_td(tr, urb);
    if (ret)
        return -1;

    xhci_ring_doorbell(ep);
    return 0;

}

static trb_t *xhci_next_trb(trb_t *cur, u8 *const pcs)
{
    ++cur;
    while (TRB_GET(TT, cur) == TRB_LINK) {
        if (pcs && TRB_GET(TC, cur))
            *pcs ^= 1;
        cur = phys_to_virt(cur->ptr_low);
    }
    return cur;
}

/* create and hook-up an intr queue into device schedule */
static void *xhci_create_intr_queue(endpoint_t *const ep,
                                    const int reqsize, const int reqcount,
                                    const int reqtiming)
{
    /* reqtiming: We ignore it and use the interval from the
              endpoint descriptor configured earlier. */

    xhci_t *const xhci = XHCI_INST(ep->dev->controller);
    const int slot_id = ep->dev->address;
    const int ep_id = xhci_ep_id(ep);
    transfer_ring_t *const tr = xhci->dev[slot_id].transfer_rings[ep_id];

    if (reqcount > (TRANSFER_RING_SIZE - 2)) {
        usb_err("reqcount is too high, at most %d supported\n",
                TRANSFER_RING_SIZE - 2);
        return NULL;
    }
    if (reqsize > 0x10000) {
        usb_err("reqsize is too large, at most 64KiB supported\n");
        return NULL;
    }
    if (xhci->dev[slot_id].interrupt_queues[ep_id]) {
        usb_err("Only one interrupt queue per endpoint supported\n");
        return NULL;
    }

    /* Allocate intrq structure and reqdata chunks */

    intrq_t *const intrq = pvPortMallocNC(sizeof(*intrq));
    if (!intrq) {
        usb_err("Out of memory\n");
        return NULL;
    }

    int i;
    u8 pcs = tr->pcs;
    trb_t *cur = tr->cur;
    for (i = 0; i < reqcount; ++i) {
        if (TRB_GET(C, cur) == pcs) {
            usb_err("Not enough empty TRBs\n");
            goto _free_return;
        }
        void *const reqdata = pvPortMallocNC(reqsize);
        if (!reqdata) {
            usb_err("Out of memory\n");
            goto _free_return;
        }
        xhci_clear_trb(cur, pcs);
        cur->ptr_low = virt_to_phys(reqdata);
        cur->ptr_high = 0;
        TRB_SET(TL, cur, reqsize);
        TRB_SET(TT, cur, TRB_NORMAL);
        TRB_SET(ISP, cur, 1);
        TRB_SET(IOC, cur, 1);

        cur = xhci_next_trb(cur, &pcs);
    }

    intrq->size = reqsize;
    intrq->count    = reqcount;
    intrq->next = tr->cur;
    intrq->ready    = NULL;
    intrq->ep   = ep;
    xhci->dev[slot_id].interrupt_queues[ep_id] = intrq;

    /* Now enqueue all the prepared TRBs but the last
       and ring the doorbell. */
    for (i = 0; i < (reqcount - 1); ++i)
        xhci_enqueue_trb(tr);
    xhci_ring_doorbell(ep);

    return intrq;

_free_return:
    cur = tr->cur;
    for (--i; i >= 0; --i) {
        vPortFreeNC(phys_to_virt(cur->ptr_low));
        cur = xhci_next_trb(cur, NULL);
    }
    vPortFreeNC(intrq);
    return NULL;
}

/* remove queue from device schedule, dropping all data that came in */
static void xhci_destroy_intr_queue(endpoint_t *const ep, void *const q)
{
    xhci_t *const xhci = XHCI_INST(ep->dev->controller);
    const int slot_id = ep->dev->address;
    const int ep_id = xhci_ep_id(ep);
    transfer_ring_t *const tr = xhci->dev[slot_id].transfer_rings[ep_id];

    intrq_t *const intrq = (intrq_t *)q;

    /* Make sure the endpoint is stopped */
    if (EC_GET(STATE, xhci->dev[slot_id].ctx.ep[ep_id]) == 1) {
        const int cc = xhci_cmd_stop_endpoint(xhci, slot_id, ep_id);
        if (cc != CC_SUCCESS)
            usb_err("Warning: Failed to stop endpoint\n");
    }

    /* Free all pending transfers and the interrupt queue structure */
    unsigned int i;
    for (i = 0; i < intrq->count; ++i) {
        vPortFreeNC(phys_to_virt(intrq->next->ptr_low));
        intrq->next = xhci_next_trb(intrq->next, NULL);
    }
    xhci->dev[slot_id].interrupt_queues[ep_id] = NULL;
    vPortFreeNC((void *)intrq);

    /* Reset the controller's dequeue pointer and reinitialize the ring */
    xhci_cmd_set_tr_dq(xhci, slot_id, ep_id, tr->ring, 1);
    xhci_init_cycle_ring(tr, TRANSFER_RING_SIZE);
}

static u8 *xhci_poll_intr_queue(void *const q)
{
    if (!q)
        return NULL;

    intrq_t *const intrq = (intrq_t *)q;
    endpoint_t *const ep = intrq->ep;
    xhci_t *const xhci = XHCI_INST(ep->dev->controller);

    /* TODO: Reset interrupt queue if it gets halted? */

    u8 *reqdata = NULL;
    while (!reqdata && intrq->ready) {
        const int ep_id = xhci_ep_id(ep);
        transfer_ring_t *const tr =
            xhci->dev[ep->dev->address].transfer_rings[ep_id];

        /* Fetch the request's buffer */
        reqdata = phys_to_virt(intrq->next->ptr_low);

        /* Enqueue the last (spare) TRB and ring doorbell */
        xhci_enqueue_trb(tr);
        xhci_ring_doorbell(ep);

        /* Reuse the current buffer for the next spare TRB */
        xhci_clear_trb(tr->cur, tr->pcs);
        tr->cur->ptr_low = virt_to_phys(reqdata);
        tr->cur->ptr_high = 0;
        TRB_SET(TL, tr->cur, intrq->size);
        TRB_SET(TT, tr->cur, TRB_NORMAL);
        TRB_SET(ISP,    tr->cur, 1);
        TRB_SET(IOC,    tr->cur, 1);

        /* Check if anything was transferred */
        const size_t read = TRB_GET(TL, intrq->next);
        if (!read)
            reqdata = NULL;
        else if (read < intrq->size)
            /* At least zero it, poll interface is rather limited */
            memset(reqdata + read, 0x00, intrq->size - read);

        /* Advance the interrupt queue */
        if (intrq->ready == intrq->next)
            /* This was last TRB being ready */
            intrq->ready = NULL;
        intrq->next = xhci_next_trb(intrq->next, NULL);
    }

    return reqdata;
}

static int xhci_cmd_queue_init(hci_t *const controller)
{
    xhci_t *const xhci = XHCI_INST(controller);
    xhci->cmd_queue = xQueueCreate(COMMAND_RING_SIZE,
                                   (sizeof(xhci_cmd_info_t *)));
    if (!xhci->cmd_queue) {
        usb_err("%s: alloc cmd queue fail\n", __func__);
        return -1;
    }

    return 0;
}

