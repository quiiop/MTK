/*
 * This file is part of the libpayload project.
 *
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

#include <hal_nvic.h>
#include <virtual.h>
#include <xhci_private.h>
#include <ssusb_hw_regs.h>
#include <usb.h>

/*
  mediatek xhci sram debug
*/
/************************************************************************************/

#define XHCI_HSRAM_DBGCTL    (0X900)
#define XHCI_HSRAM_DBGMODE   (0X904)
#define XHCI_HSRAM_DBGSEL    (0X908)
#define XHCI_HSRAM_DBGADR    (0X90C)
#define XHCI_HSRAM_DBGGDR    (0X910)

struct hsramreg {
    u32 dbgctl;
    u32 dbgmode;
    u32 dbgsel;
    u32 dbgadr;
    u32 dbggdr;
};


static void xhci_sram_dbg_init(xhci_t *const xhci, u32 dbg_ctl,
                               u32 dbg_mode, u32 dbg_sel)
{
    volatile struct hsramreg *hsrr;
    hsrr = (struct hsramreg *)(((u8 *)xhci->capreg) + XHCI_HSRAM_DBGCTL);

    hsrr->dbgmode = dbg_mode;
    hsrr->dbgsel = dbg_sel;
    hsrr->dbgctl = dbg_ctl;
    usb_debug("mode[%p]:%x, sel[%p]:%x, ctl[%p]:%x\n\r",
              &hsrr->dbgmode, hsrr->dbgmode,
              &hsrr->dbgsel, hsrr->dbgsel,
              &hsrr->dbgctl, hsrr->dbgctl);
}

static void xhci_sram_dbg_read(xhci_t *const xhci, u32 dbg_adr)
{
    u32 debug_data[4];
    u32 i, temp;
    volatile struct hsramreg *hsrr;

    hsrr = (struct hsramreg *)(((u8 *)xhci->capreg) + XHCI_HSRAM_DBGCTL);
    usb_debug("sram addr 0x%x", dbg_adr);
    for (i = 0; i < 4; i++) {
        temp = (dbg_adr << 3);
        temp |= i;
        hsrr->dbgadr = temp;
        debug_data[i] = hsrr->dbggdr;
        usb_crit("0x%x", debug_data[i]);
    }
    usb_debug("\n\r");
}

void printk_sram_data(xhci_t *const xhci)
{
    int i;

    xhci_sram_dbg_init(xhci, 0x01, 1 << 1, 1 << 1);
    for (i = 0; i < 32; i++) {
        usb_debug("**********sram addr %d*****************\n", i);
        xhci_sram_dbg_read(xhci, i);
    }
}

void xhci_reset_event_ring(event_ring_t *const er)
{
    int i;
    for (i = 0; i < EVENT_RING_SIZE; ++i)
        er->ring[i].control &= ~TRB_CYCLE;
    er->cur     = er->ring;
    er->last    = er->ring + EVENT_RING_SIZE;
    er->ccs     = 1;
    er->adv     = 1;
}

static inline int xhci_event_ready(const event_ring_t *const er)
{
    return (er->cur->control & TRB_CYCLE) == er->ccs;
}

void xhci_update_event_dq(xhci_t *const xhci)
{
    u32 tmp;
    if (xhci->er.adv) {
        usb_debug("Updating dq ptr: @%p(0x%08x) -> %p\n",
                  phys_to_virt(xhci->hcrreg->intrrs[0].erdp_lo),
                  xhci->hcrreg->intrrs[0].erdp_lo, xhci->er.cur);
        tmp = virt_to_phys(xhci->er.cur);
        tmp |= 1 << 3;
        writel(tmp, &xhci->hcrreg->intrrs[0].erdp_lo);
        xhci->hcrreg->intrrs[0].erdp_hi = 0;
        xhci->er.adv = 0;
    }
}

void xhci_advance_event_ring(xhci_t *const xhci)
{
    xhci->er.cur++;
    xhci->er.adv = 1;

    if (xhci->er.cur == xhci->er.last) {
        usb_debug("Roll over in event ring\n");
        xhci->er.cur = xhci->er.ring;
        xhci->er.ccs ^= 1;
        xhci_update_event_dq(xhci);
    }
    usb_debug("%s %d dq ptr: @%p control:%x ccs:%x\n", __func__, __LINE__,
              phys_to_virt(xhci->hcrreg->intrrs[0].erdp_lo),
              xhci->er.cur->control, xhci->er.ccs);
}

static void process_isoc_td(xhci_t *const xhci, struct xhci_td *td)
{
    int idx;
    struct urb *urb;
    struct urb_priv *urb_priv;
    //struct xhci_td *td;
    struct usb_iso_packet_desc *frame;
    bool sum_trbs_for_length = false;
    u32 remaining, requested;
    trb_t *transfer_trb;

    const trb_t *const ev = xhci->er.cur;
    const int cc = TRB_GET(CC, ev);
    transfer_trb = phys_to_virt(ev->ptr_low);

    urb = td->urb;
    urb_priv = urb->hcpriv;
    idx = urb_priv->num_tds_done;
    if (td != &urb_priv->td[idx]) {
        usb_err("%s: something error, td->urb:%p td:%p != urb_priv->td[%d]:%p urb_priv->td->urb:%p\n", __func__, td->urb, td, idx, &urb_priv->td[idx], urb_priv->td[idx].urb);
        return;
    }

    frame = &urb->iso_frame_desc[idx];
    requested = frame->length;
    remaining = TRB_GET(EVTL, ev);

    /* handle completion code */
    switch (cc) {
        case CC_SUCCESS:
            frame->status = 0;
            break;
        case CC_SHORT_PACKET:
            frame->status = 0;
            sum_trbs_for_length = true;
            break;
        case CC_USB_TRANSACTION_ERROR:
            usb_err("%s %d CC_USB_TRANSACTION_ERROR CC:%d \n", __func__, __LINE__, cc);
            frame->status = -1;
            if (transfer_trb != td->last_trb)
                goto error;
            break;
        case CC_TRB_ERROR:
            usb_err("%s %d CC_TRB_ERROR CC:%d \n", __func__, __LINE__, cc);
            frame->status = -1;
            goto error;
        case CC_ISOCH_BUFFER_OVERRUN:
            usb_err("%s %d CC_ISOCH_BUFFER_OVERRUN CC:%d \n", __func__, __LINE__, cc);
            frame->status = 100;
            goto error;
        default:
            usb_err("%s %d ERROR CC:%d \n", __func__, __LINE__, cc);
            sum_trbs_for_length = true;
            frame->status = -2;
            goto error;
    }

    if (sum_trbs_for_length)
        frame->actual_length = requested - remaining;
    else
        frame->actual_length = requested;

    td->urb->actual_length += frame->actual_length;

    urb_priv->num_tds_done++;
    if (urb_priv->num_tds_done == urb_priv->num_tds)
        urb->complete(urb);

    return;

error:
    urb_priv->num_tds_done++;
    if (urb_priv->num_tds_done == urb_priv->num_tds)
        urb->complete(urb);

    return;
}

static void xhci_handle_transfer_event(xhci_t *const xhci)
{
    const trb_t *const ev = xhci->er.cur;

    const int cc = TRB_GET(CC, ev);
    const int id = TRB_GET(ID, ev);
    const int ep = TRB_GET(EP, ev);
    transfer_ring_t *tr;
    trb_t *transfer_trb;
    struct urb *urb;
    struct xhci_td *td;
    u32 trb_type;
    u32 remaining;
    portBASE_TYPE pxHigherPriorityTaskWoken = pdFALSE;

    intrq_t *intrq;
    /* Handle interrupt endpoint event */
    if (id && id <= xhci->max_slots_en &&
        (intrq = xhci->dev[id].interrupt_queues[ep])) {
        intrq->ready = phys_to_virt(ev->ptr_low);
        usb_debug("%s: Interrupt Transfer\n", __func__);
        if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET) {
            TRB_SET(TL, intrq->ready,
                    intrq->size - TRB_GET(EVTL, ev));
        } else {
            usb_debug("Interrupt Transfer failed: %d\n", cc);
            TRB_SET(TL, intrq->ready, 0);
        }
        goto advance_event_ring;
    }

    tr = xhci->dev[id].transfer_rings[ep];

    if (tr && tr->isoc_queue) {
        switch (cc) {
            case CC_COMMAND_MISS_SERVICE_ERROR:
            case CC_RING_UNDERRUN:
            case CC_RING_OVERRUN:
                goto advance_event_ring;
        }

        if (pdPASS != xQueueReceiveFromISR(tr->isoc_queue, &td, &pxHigherPriorityTaskWoken)) {
            usb_debug("EP%d isoc queue is empty.\n", ep);
            goto advance_event_ring;
        }

        if (!usb_endpoint_xfer_isoc(td->urb->ep->type)) {
            usb_err("EP%d is not isoc type.\n", ep);
        }

        /* Handle isochronous endpoint event */
        process_isoc_td(xhci, td);
        goto advance_event_ring;
    }

    if (pdPASS != xQueueReceiveFromISR(tr->trb_queue, &urb, &pxHigherPriorityTaskWoken)) {
        usb_debug("EP%d queue is empty.\n", ep);
        goto advance_event_ring;
    }

    if (pxHigherPriorityTaskWoken == pdTRUE)
        usb_debug("%s Warning: pxHigherPriorityTaskWoken IS TRUE \n", __func__);

    /* Handle bulk/control endpoint event */
    if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET) {
        trb_type = TRB_GET(TT, xhci->er.cur);
        transfer_trb = phys_to_virt(xhci->er.cur->ptr_low);
        trb_type = TRB_GET(TT, transfer_trb);
        switch (trb_type) {
            case TRB_STATUS_STAGE:
                urb->actual_length = urb->transfer_buffer_length;
                break;
            case TRB_NORMAL:
                remaining = TRB_GET(EVTL, xhci->er.cur);
                urb->actual_length = urb->transfer_buffer_length - remaining;
                break;
        }

        if (pdPASS != xSemaphoreGiveFromISR(urb->xSemaphore, &pxHigherPriorityTaskWoken)) {
            usb_err("%s: give semphore fail\n", __func__);
        }
    } else if (cc == CC_STOPPED || cc == CC_STOPPED_LENGTH_INVALID) {
        /* Ignore 'Forced Stop Events' */
        usb_debug("%s %d ERROR \n", __func__, __LINE__);
    } else {
        usb_debug("Warning: "
                  "Spurious transfer event for ID %d, EP %d:\n"
                  "  Pointer: 0x%08x%08x\n"
                  "       TL: 0x%06x\n"
                  "       CC: %d\n",
                  id, ep,
                  ev->ptr_high, ev->ptr_low,
                  TRB_GET(EVTL, ev), cc);
    }

advance_event_ring:
    xhci_advance_event_ring(xhci);
    return;
}

static void xhci_handle_command_completion_event(xhci_t *const xhci)
{
    int cc;
    u32 cmd_type;
    xhci_cmd_info_t *cmd_info;
    const trb_t *const ev = xhci->er.cur;
    portBASE_TYPE pxHPrTW = pdFALSE;

    if (pdPASS != xQueueReceiveFromISR(xhci->cmd_queue, &cmd_info, &pxHPrTW)) {
        usb_err("error: cmd queue is empty. \n");
        goto error;
    }
    if (pxHPrTW == pdTRUE)
        usb_debug("Warning: pxHigherPriorityTaskWoken IS TRUE \n");

    if (xhci->er.cur->ptr_low != virt_to_phys(cmd_info->cmd)) {
        usb_err("Command completion event does not match command\n");
        goto error;
    }

    cc = TRB_GET(CC, xhci->er.cur);
    if (cc == CC_COMMAND_RING_STOPPED || cc == CC_COMMAND_ABORTED) {
        usb_debug("cmd ring stop or abort :%d\n", cc);
        goto cmd_done;
    }

    cmd_type = TRB_GET(TT, cmd_info->cmd);
    switch (cmd_type) {
        case TRB_CMD_ENABLE_SLOT:
            cc = TRB_GET(CC, xhci->er.cur);
            if (cc == CC_SUCCESS) {
                cmd_info->slot_id = TRB_GET(ID, xhci->er.cur);
                if (cmd_info->slot_id > xhci->max_slots_en)
                    cc = CONTROLLER_ERROR;
            }
            cmd_info->status = cc;
            usb_debug("TRB_CMD_ENABLE_SLOT cc:%x slot_id:%x xhci:%p\n",
                      cc, cmd_info->slot_id, xhci);
            break;
        case TRB_CMD_DISABLE_SLOT:
        case TRB_CMD_ADDRESS_DEV:
        case TRB_CMD_CONFIGURE_EP:
        case TRB_CMD_EVAL_CTX:
        case TRB_CMD_RESET_EP:
        case TRB_CMD_STOP_EP:
        case TRB_CMD_SET_TR_DQ:
            cmd_info->status = TRB_GET(CC, xhci->er.cur);
            break;
        default:
            /* Skip over unknown commands on the event ring */
            usb_err("INFO unknown command type %d\n", cmd_type);
            break;
    }

cmd_done:
    mb();
    xhci_advance_event_ring(xhci);

    if (pdPASS != xSemaphoreGiveFromISR(cmd_info->xSemaphore, &pxHPrTW)) {
        usb_err("%s: give semphore fail\n", __func__);
        return;
    }

    if (pxHPrTW == pdTRUE)
        usb_debug("Warning: pxHigherPriorityTaskWoken IS TRUE \n");

    return;

error:
    usb_crit("Warning: Spurious command completion event:\n"
             "  Pointer: 0x%08x%08x\n"
             "       CC: %d\n"
             "  Slot ID: %d\n"
             "    Cycle: %d\n",
             ev->ptr_high, ev->ptr_low,
             TRB_GET(CC, ev), TRB_GET(ID, ev), ev->control & TRB_CYCLE);
    xhci_advance_event_ring(xhci);
}

static void xhci_handle_host_controller_event(xhci_t *const xhci)
{
    const trb_t *const ev = xhci->er.cur;

    const int cc = TRB_GET(CC, ev);
    switch (cc) {
        case CC_EVENT_RING_FULL_ERROR:
            usb_debug("Event ring full! (@%p)\n", xhci->er.cur);
            /*
             * If we get here, we have processed the whole queue:
             * xHC pushes this event, when it sees the ring full,
             * full of other events.
             * IMO it's save and necessary to update the dequeue
             * pointer here.
             */
            xhci_advance_event_ring(xhci);
            xhci_update_event_dq(xhci);
            break;
        default:
            usb_debug("Warning: Spurious host controller event: %d\n", cc);
            xhci_advance_event_ring(xhci);
            break;
    }
}

/* handle standard types:
 * - command completion event
 * - port status change event
 * - transfer event
 * - host controller event
 */
static int xhci_handle_event(xhci_t *const xhci)
{
    const trb_t *const ev = xhci->er.cur;

    if ((ev->control & TRB_CYCLE) != xhci->er.ccs)
        return 0;

    const int trb_type = TRB_GET(TT, ev);
    switch (trb_type) {
        /* Either pass along the event or advance event ring */
        case TRB_EV_TRANSFER:
            xhci_handle_transfer_event(xhci);
            break;
        case TRB_EV_CMD_CMPL:
            xhci_handle_command_completion_event(xhci);
            break;
        case TRB_EV_PORTSC:
            usb_debug("Port Status Change Event for %d: %d\n",
                      TRB_GET(PORT, ev), TRB_GET(CC, ev));
            /* We ignore the event as we look for the PORTSC
               registers instead, at a time when it suits _us_. */
            usb_debug("%s %d dq ptr: @%p\n", __func__, __LINE__,
                      phys_to_virt(xhci->hcrreg->intrrs[0].erdp_lo));
            xhci_advance_event_ring(xhci);
            break;
        case TRB_EV_HOST:
            xhci_handle_host_controller_event(xhci);
            break;
        default:
            usb_debug("Warning: Spurious event: %d, Completion Code: %d\n",
                      trb_type, TRB_GET(CC, ev));
            xhci_advance_event_ring(xhci);
            break;
    }

    return 1;
}

void xhci_handle_events(xhci_t *const xhci)
{
    while (xhci_event_ready(&xhci->er))
        xhci_handle_event(xhci);
    xhci_update_event_dq(xhci);
}

extern hci_t *global_hcd;
void xhci_irq(hal_nvic_irq_t irq_number)
{
    u32 status;
    uint32_t mask;
    xhci_t *xhci = (xhci_t *)global_hcd->instance;

    hal_nvic_save_and_set_interrupt_mask(&mask);
    /* clear interrupt status */
    status = readl(&xhci->opreg->usbsts);
    status &= ~(1 << 2 | 1 << 3 | 1 << 4 | 1 << 10);
    status |= USBSTS_EINT;
    writel(status, &xhci->opreg->usbsts);

    status = readl(&xhci->hcrreg->intrrs[0].iman);
    status |= 0x1;
    writel(status, &xhci->hcrreg->intrrs[0].iman);

    while (xhci_handle_event(xhci) > 0)
        ;

    xhci_update_event_dq(xhci);
    hal_nvic_restore_interrupt_mask(mask);
}

