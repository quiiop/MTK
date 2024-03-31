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

#pragma GCC diagnostic ignored "-Wshift-count-overflow"

#include <virtual.h>
#include <usb.h>
#include "internal_list.h"
#include <xhci_private.h>
#include <xhci_mtk.h>

static u32 xhci_gen_route(xhci_t *const xhci, const int hubport, const int hubaddr)
{
    if (!hubaddr)
        return 0;
    u32 route_string = SC_GET(ROUTE, xhci->dev[hubaddr].ctx.slot);
    int i;
    for (i = 0; i < 20; i += 4) {
        if (!(route_string & (0xf << i))) {
            route_string |= (hubport & 0xf) << i;
            break;
        }
    }
    return route_string;
}

static int xhci_get_rh_port(xhci_t *const xhci, const int hubport, const int hubaddr)
{
    if (!hubaddr)
        return hubport;
    return SC_GET(RHPORT, xhci->dev[hubaddr].ctx.slot);
}

static int xhci_get_tt(xhci_t *const xhci, const usb_speed speed,
                       const int hubport, const int hubaddr,
                       int *const tt, int *const tt_port)
{
    if (!hubaddr)
        return 0;
    const slotctx_t *const slot = xhci->dev[hubaddr].ctx.slot;
    if ((*tt = SC_GET(TTID, slot))) {
        *tt_port = SC_GET(TTPORT, slot);
    } else if (speed < HIGH_SPEED &&
               SC_GET(SPEED1, slot) - 1 == HIGH_SPEED) {
        *tt = hubaddr;
        *tt_port = hubport;
    }
    return *tt != 0;
}

static u32 xhci_get_endpoint_max_burst(endpoint_t *ep)
{
    usbdev_t *dev = ep->dev;

    /* Super speed and Plus have max burst in ep companion desc */
    /* if (dev->speed >= SUPER_SPEED) */
    /* return ep->ss_ep_comp.bMaxBurst; */

    if (dev->speed == HIGH_SPEED &&
        (usb_endpoint_xfer_isoc(ep->type) ||
         usb_endpoint_xfer_int(ep->type)))
        return usb_endpoint_maxp_mult(ep) - 1;

    return 0;
}

static u32 xhci_get_max_esit_payload(endpoint_t *ep)
{
    int max_burst;
    int max_packet;

    /* Only applies for interrupt or isochronous endpoints */
    if (usb_endpoint_xfer_control(ep->type) ||
        usb_endpoint_xfer_bulk(ep->type))
        return 0;

    /* SuperSpeedPlus Isoc ep sending over 48k per esit */
    //if (dev->speed >= SUPER_SPEED_PLUS))

    /* SuperSpeed or SuperSpeedPlus Isoc ep with less than 48k per esit */
    //else if (udev->speed >= SUPER_SPEED)


    max_packet = usb_endpoint_maxp(ep);
    max_burst = usb_endpoint_maxp_mult(ep);
    /* A 0 in max burst means 1 transfer per ESIT */
    return max_packet * max_burst;
}

static void xhci_reap_slots(xhci_t *const xhci, int skip_slot)
{
    int i;

    usb_debug("xHC resource shortage, trying to reap old slots...\n");
    for (i = 1; i <= xhci->max_slots_en; i++) {
        if (i == skip_slot)
            continue;   /* don't reap slot we were working on */
        if (xhci->dev[i].transfer_rings[1])
            continue;   /* slot still in use */
        if (!xhci->dev[i].ctx.raw)
            continue;   /* slot already disabled */

        const int cc = xhci_cmd_disable_slot(xhci, i);
        if (cc != CC_SUCCESS)
            usb_debug("Failed to disable slot %d: %d\n", i, cc);
        else
            usb_debug("Successfully reaped slot %d\n", i);
        xhci->dcbaa[i] = 0;
        xhci_align_free(xhci->dev[i].ctx.raw);
        xhci->dev[i].ctx.raw = NULL;
    }
}

static inputctx_t *xhci_make_inputctx(const size_t ctxsize)
{
    int i;
    const size_t size = (1 + NUM_EPS) * ctxsize;
    inputctx_t *const ic = pvPortMallocNC(sizeof(*ic));
    void *dma_buffer = xhci_align(64, size);

    if (!ic || !dma_buffer) {
        vPortFreeNC(ic);
        xhci_align_free(dma_buffer);
        return NULL;
    }
    memset(dma_buffer, 0, size);
    ic->drop = dma_buffer + 0;
    ic->add = dma_buffer + 4;
    dma_buffer += ctxsize;
    for (i = 0; i < NUM_EPS; i++, dma_buffer += ctxsize)
        ic->dev.ep[i] = dma_buffer;

    return ic;
}

usbdev_t *xhci_set_address(hci_t *controller, usb_speed speed, int hubport, int hubaddr)
{
    xhci_t *const xhci = XHCI_INST(controller);
    const size_t ctxsize = CTXSIZE(xhci);
    devinfo_t *di = NULL;
    usbdev_t *dev = NULL;
    int i;

    usb_debug("%s: xhci: %p\n", __func__, xhci);

    inputctx_t *const ic = xhci_make_inputctx(ctxsize);
    transfer_ring_t *const tr = xhci_alloc_cycle_ring();
    if (tr)
        tr->ring = xhci_align(16, TRANSFER_RING_SIZE * sizeof(trb_t));
    if (!ic || !tr || !tr->ring) {
        usb_debug("Out of memory\n");
        goto _free_return;
    }

    int slot_id;
    int cc = xhci_cmd_enable_slot(xhci, &slot_id);
    if (cc == CC_NO_SLOTS_AVAILABLE) {
        xhci_reap_slots(xhci, 0);
        cc = xhci_cmd_enable_slot(xhci, &slot_id);
    }
    if (cc != CC_SUCCESS) {
        usb_debug("Enable slot failed: %d\n", cc);
        goto _free_return;
    } else {
        usb_debug("Enabled slot %d\n", slot_id);
    }

    di = &xhci->dev[slot_id];
    void *dma_buffer = xhci_align(64, NUM_EPS * ctxsize);
    if (!dma_buffer)
        goto _disable_return;
    memset(dma_buffer, 0, NUM_EPS * ctxsize);
    for (i = 0; i < NUM_EPS; i++, dma_buffer += ctxsize)
        di->ctx.ep[i] = dma_buffer;

    *ic->add = (1 << 0) /* Slot Context */ | (1 << 1) /* EP0 Context */ ;

    SC_SET(ROUTE,   ic->dev.slot, xhci_gen_route(xhci, hubport, hubaddr));
    SC_SET(SPEED1,  ic->dev.slot, speed + 1);
    SC_SET(CTXENT,  ic->dev.slot, 1); /* the endpoint 0 context */
    SC_SET(RHPORT,  ic->dev.slot, xhci_get_rh_port(xhci, hubport, hubaddr));

    int tt, tt_port;
    if (xhci_get_tt(xhci, speed, hubport, hubaddr, &tt, &tt_port)) {
        usb_debug("TT for %d: %d[%d]\n", slot_id, tt, tt_port);
        SC_SET(MTT, ic->dev.slot, SC_GET(MTT, xhci->dev[tt].ctx.slot));
        SC_SET(TTID, ic->dev.slot, tt);
        SC_SET(TTPORT, ic->dev.slot, tt_port);
    }

    di->transfer_rings[1] = tr;
    xhci_init_cycle_ring(tr, TRANSFER_RING_SIZE);

    ic->dev.ep0->tr_dq_low  = virt_to_phys(tr->ring);
    ic->dev.ep0->tr_dq_high = 0;
    EC_SET(TYPE,    ic->dev.ep0, EP_CONTROL);
    EC_SET(AVRTRB,  ic->dev.ep0, 8);
    EC_SET(MPS, ic->dev.ep0, speed_to_default_mps(speed));
    EC_SET(CERR,    ic->dev.ep0, 3);
    EC_SET(DCS, ic->dev.ep0, 1);

    xhci->dcbaa[slot_id] = virt_to_phys(di->ctx.raw);

    cc = xhci_cmd_address_device(xhci, slot_id, ic);
    if (cc == CC_RESOURCE_ERROR) {
        xhci_reap_slots(xhci, slot_id);
        cc = xhci_cmd_address_device(xhci, slot_id, ic);
    }
    if (cc != CC_SUCCESS) {
        usb_debug("Address device failed: %d\n", cc);
        goto _disable_return;
    } else {
        usb_debug("Addressed device %d (USB: %d)\n",
                  slot_id, SC_GET(UADDR, di->ctx.slot));
    }
    mdelay(SET_ADDRESS_MDELAY);

    dev = init_device_entry(controller, slot_id);
    if (!dev)
        goto _disable_return;

    dev->address = slot_id;
    dev->hub = hubaddr;
    dev->port = hubport;
    dev->speed = speed;
    dev->endpoints[0].dev = dev;
    dev->endpoints[0].endpoint = 0;
    dev->endpoints[0].toggle = 0;
    dev->endpoints[0].direction = SETUP;
    dev->endpoints[0].type = CONTROL;

    u8 *buf = (u8 *)pvPortMallocNC(8);
    if (buf == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        goto _disable_return;
    }

    if (get_descriptor(dev, gen_bmRequestType(device_to_host, standard_type,
                                              dev_recp), DT_DEV, 0, buf, 8) != 8) {
        usb_debug("first get_descriptor(DT_DEV) failed\n");
        vPortFreeNC((void *)buf);
        goto _disable_return;
    }

    dev->endpoints[0].maxpacketsize = usb_decode_mps0(speed, buf[7]);
    vPortFreeNC((void *)buf);
    if (dev->endpoints[0].maxpacketsize != speed_to_default_mps(speed)) {
        memset((void *)ic->dev.ep0, 0x00, ctxsize);
        *ic->add = (1 << 1); /* EP0 Context */
        EC_SET(MPS, ic->dev.ep0, dev->endpoints[0].maxpacketsize);
        cc = xhci_cmd_evaluate_context(xhci, slot_id, ic);
        if (cc == CC_RESOURCE_ERROR) {
            xhci_reap_slots(xhci, slot_id);
            cc = xhci_cmd_evaluate_context(xhci, slot_id, ic);
        }
        if (cc != CC_SUCCESS) {
            usb_debug("Context evaluation failed: %d\n", cc);
            goto _disable_return;
        }
    }
    goto _free_ic_return;

_disable_return:
    xhci_cmd_disable_slot(xhci, slot_id);
    xhci->dcbaa[slot_id] = 0;
    usb_detach_device(controller, slot_id);
    dev = NULL;
_free_return:
    if (tr) {
        xhci_align_free((void *)tr->ring);
        vPortFree(tr);
    }
    if (di) {
        xhci_align_free(di->ctx.raw);
        di->transfer_rings[1] = NULL;
        di->ctx.raw = 0;
    }
_free_ic_return:
    if (ic)
        xhci_align_free(ic->raw);
    vPortFreeNC(ic);
    return dev;
}

static int xhci_finish_hub_config(usbdev_t *const dev, inputctx_t *const ic)
{
    int type = is_usb_speed_ss(dev->speed) ? 0x2a : 0x29; /* similar enough */
    hub_descriptor_t *desc;
    desc = (hub_descriptor_t *)pvPortMallocNC(sizeof(hub_descriptor_t));
    if (desc == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;
    }

    if (get_descriptor(dev, gen_bmRequestType(device_to_host, class_type,
                                              dev_recp), type, 0, desc, sizeof(hub_descriptor_t)) != sizeof(hub_descriptor_t)) {
        usb_debug("Failed to fetch hub descriptor\n");
        vPortFreeNC((void *)desc);
        return COMMUNICATION_ERROR;
    }

    SC_SET(HUB, ic->dev.slot, 1);
    SC_SET(MTT, ic->dev.slot, 0); /* No support for Multi-TT */
    SC_SET(NPORTS,  ic->dev.slot, desc->bNbrPorts);
    if (dev->speed == HIGH_SPEED)
        SC_SET(TTT, ic->dev.slot,
               (desc->wHubCharacteristics >> 5) & 0x0003);

    vPortFreeNC((void *)desc);
    return 0;
}

static size_t xhci_bound_interval(const endpoint_t *const ep)
{
    if ((ep->dev->speed == LOW_SPEED &&
         (ep->type == ISOCHRONOUS ||
          ep->type == INTERRUPT)) ||
        (ep->dev->speed == FULL_SPEED &&
         ep->type == INTERRUPT)) {
        if (ep->interval < 3)
            return 3;
        else if (ep->interval > 11)
            return 11;
        else
            return ep->interval;
    } else {
        if (ep->interval < 0)
            return 0;
        else if (ep->interval > 15)
            return 15;
        else
            return ep->interval;
    }
}

static int xhci_finish_ep_config(endpoint_t *ep, inputctx_t *const ic)
{
    transfer_ring_t *tr;
    u32 max_esit_payload;
    u32 max_burst;
    u32 avrtrb;

    xhci_t *const xhci = XHCI_INST(ep->dev->controller);
    const unsigned int ep_id = xhci_ep_id(ep);

    if (ep_id <= 1 || 32 <= ep_id)
        return DRIVER_ERROR;

    if (ep->type == ISOCHRONOUS)
        tr = xhci_alloc_isoc_ring();
    else
        tr = xhci_alloc_cycle_ring();

    if (tr)
        tr->ring = xhci_align(16, TRANSFER_RING_SIZE * sizeof(trb_t));
    if (!tr || !tr->ring) {
        vPortFree(tr);
        usb_debug("Out of memory\n");
        return OUT_OF_MEMORY;
    }

    xhci->dev[ep->dev->address].transfer_rings[ep_id] = tr;
    xhci_init_cycle_ring(tr, TRANSFER_RING_SIZE);

    *ic->add |= (1 << ep_id);
    if (SC_GET(CTXENT, ic->dev.slot) < ep_id)
        SC_SET(CTXENT, ic->dev.slot, ep_id);

    epctx_t *const epctx = ic->dev.ep[ep_id];
    usb_debug("Filling epctx (@%p)\n", epctx);
    max_burst = xhci_get_endpoint_max_burst(ep);
    max_esit_payload = xhci_get_max_esit_payload(ep);
    epctx->tr_dq_low    = virt_to_phys(tr->ring);
    epctx->tr_dq_high    = 0;
    EC_SET(INTVAL, epctx, xhci_bound_interval(ep));
    EC_SET(CERR, epctx, 3);
    EC_SET(TYPE, epctx, ep->type | ((ep->direction != OUT) << 2));
    EC_SET(MPS, epctx, usb_endpoint_maxp(ep));
    EC_SET(MBS, epctx, max_burst);
    EC_SET(MULT, epctx, 0);
    EC_SET(DCS, epctx, 1);
    EC_SET(MAXESITLO, epctx, max_esit_payload);

    EC_SET(BPKTS, epctx, 1);
    EC_SET(BBM, epctx, 1);
    switch (ep->type) {
        case BULK:
            avrtrb = 3 * 1024;
            break;
        case ISOCHRONOUS:
            avrtrb = 3 * 1024;
            EC_SET(CERR, epctx, 0);
            /* MTK Schedule Isoc */
            EC_SET(BPKTS, epctx, usb_endpoint_maxp_mult(ep));
            break;
        case INTERRUPT:
            avrtrb = 1024;
            break;
        default:
            avrtrb = 8;
            break;
    }

    EC_SET(AVRTRB, epctx, avrtrb);
    return 0;

    /* return xhci_mtk_add_ep_quirk(xhci, ep->dev, ep); */
}

int xhci_finish_device_config(usbdev_t *const dev)
{
    xhci_t *const xhci = XHCI_INST(dev->controller);
    int slot_id = dev->address;
    devinfo_t *const di = &xhci->dev[slot_id];

    int i, ret = 0;

    inputctx_t *const ic = xhci_make_inputctx(CTXSIZE(xhci));
    if (!ic) {
        usb_debug("Out of memory\n");
        return OUT_OF_MEMORY;
    }

    *ic->add = (1 << 0); /* Slot Context */

    xhci_dump_slotctx(di->ctx.slot);
    ic->dev.slot->f1 = di->ctx.slot->f1;
    ic->dev.slot->f2 = di->ctx.slot->f2;
    ic->dev.slot->f3 = di->ctx.slot->f3;
    /* f4 *must* be 0 in the Input Context... yeah, it's weird, I know. */

    if (dev->descriptor->bDeviceClass == 0x09) {
        ret = xhci_finish_hub_config(dev, ic);
        if (ret)
            goto _free_return;
    }

    for (i = 1; i < dev->num_endp; ++i) {
        ret = xhci_finish_ep_config(&dev->endpoints[i], ic);
        if (ret)
            goto _free_ep_ctx_return;
    }

    xhci_dump_inputctx(ic);

    const int config_id = dev->configuration->bConfigurationValue;
    int cc = xhci_cmd_configure_endpoint(xhci, slot_id, config_id, ic);
    if (cc == CC_RESOURCE_ERROR || cc == CC_BANDWIDTH_ERROR) {
        xhci_reap_slots(xhci, slot_id);
        cc = xhci_cmd_configure_endpoint(xhci, slot_id, config_id, ic);
    }
    if (cc != CC_SUCCESS) {
        usb_debug("Configure endpoint failed: %d\n", cc);
        ret = CONTROLLER_ERROR;
        goto _free_ep_ctx_return;
    } else {
        usb_debug("Endpoints configured\n");
    }

    goto _free_return;

_free_ep_ctx_return:
    for (i = 2; i < 31; ++i) {
        if (di->transfer_rings[i])
            xhci_align_free((void *)di->transfer_rings[i]->ring);
        vPortFree(di->transfer_rings[i]);
        di->transfer_rings[i] = NULL;
    }
_free_return:
    xhci_align_free(ic->raw);
    vPortFreeNC(ic);
    return ret;
}

void xhci_destroy_dev(hci_t *const controller, const int slot_id)
{
    xhci_t *const xhci = XHCI_INST(controller);
    usbdev_t *dev = controller->devices[slot_id];

    if (slot_id <= 0 || slot_id > xhci->max_slots_en)
        return;

    inputctx_t *const ic = xhci_make_inputctx(CTXSIZE(xhci));
    if (!ic) {
        usb_debug("Out of memory, leaking resources!\n");
        return;
    }

    *ic->add = 0;   /* Leave Slot/EP0 state as it is for now. */
    *ic->drop = (1 << NUM_EPS) - 1; /* Drop all endpoints we can. */
    *ic->drop &= ~(1 << 1 | 1 << 0); /* Not allowed to drop EP0 or Slot. */
    int cc = xhci_cmd_evaluate_context(xhci, slot_id, ic);
    xhci_align_free(ic->raw);
    vPortFreeNC(ic);
    if (cc != CC_SUCCESS)
        usb_debug("Failed to quiesce slot %d: %d\n", slot_id, cc);
    cc = xhci_cmd_stop_endpoint(xhci, slot_id, 1);
    if (cc != CC_SUCCESS)
        usb_debug("Failed to stop EP0 on slot %d: %d\n", slot_id, cc);

    int i;
    devinfo_t *const di = &xhci->dev[slot_id];
    for (i = 1; i < NUM_EPS; ++i) {
        if (di->transfer_rings[i]) {
            xhci_align_free((void *)di->transfer_rings[i]->ring);
            vPortFree(di->transfer_rings[i]);
            di->transfer_rings[i] = NULL;
        }

        if (di->interrupt_queues[i]) {
            vPortFree(di->interrupt_queues[i]);
            di->interrupt_queues[i] = NULL;
        }
    }

    for (i = 1; i < dev->num_endp; ++i) {
        /* xhci_mtk_drop_ep_quirk(xhci, dev, &dev->endpoints[i]); */
    }

    xhci_align_free(di->ctx.raw);
    di->ctx.raw = 0;

    usb_debug("Stopped slot %d, but not disabling it yet.\n", slot_id);
    di->transfer_rings[1] = NULL;
}
