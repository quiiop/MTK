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

#include <virtual.h>
#include <xhci_private.h>

struct xhci_cmd_info *xhci_alloc_command(xhci_t *const xhci)
{
    struct xhci_cmd_info *cmd_info = pvPortMalloc(sizeof(struct xhci_cmd_info));
    if (!cmd_info) {
        usb_err("alloc cmd fail\n");
        return NULL;
    }
    memset(cmd_info, 0, (sizeof(struct xhci_cmd_info)));

    cmd_info->xSemaphore = xSemaphoreCreateBinary();
    if (cmd_info->xSemaphore == NULL) {
        usb_err("%s: create cmd semahpone fail\n", __func__);
        vPortFree(cmd_info);
        return NULL;
    }

    xhci->cmd_info = cmd_info;
    return cmd_info;
}

void xhci_free_command(xhci_t *const xhci, struct xhci_cmd_info *cmd_info)
{
    vSemaphoreDelete(cmd_info->xSemaphore);
    vPortFree(cmd_info);
    if (xhci->cmd_info != cmd_info) {
        usb_err("%s: free cmd not match\n", __func__);
        return;
    }

    xhci->cmd_info = NULL;
    return;
}

trb_t *xhci_next_command_trb(xhci_t *const xhci)
{
    xhci_clear_trb(xhci->cr.cur, xhci->cr.pcs);
    return xhci->cr.cur;
}

static int xhci_abort_command(xhci_t *const xhci, struct xhci_cmd_info *cmd_info)
{
    trb_t *cmd_trb;
    cmd_trb = xhci->cr.cur;

    usb_crit("Aborting command (@%p), CRCR: 0x%x\n",
             cmd_trb, xhci->opreg->crcr_lo);
    xhci->opreg->crcr_lo |= CRCR_CS | CRCR_CA;
    xhci->opreg->crcr_hi = 0;

    if (xhci_handshake(xhci->opreg->crcr_lo, CRCR_CRR, 0, 5 * 1000 * 1000))
        usb_crit("Abort failed to stop command ring!\n");

    return 0;
}

static int xhci_post_command(xhci_t *const xhci, struct xhci_cmd_info *cmd_info)
{
    trb_t *trb;

    trb = xhci->cr.cur;
    TRB_SET(C, xhci->cr.cur, xhci->cr.pcs);
    cmd_info->cmd = trb;
    ++xhci->cr.cur;

    if (pdPASS != xQueueSend(xhci->cmd_queue, &cmd_info, 0)) {
        usb_err("%s: send cmd to queue fail\n", __func__);
        return -1;
    }

    /* pass command trb to hardware */
    mb();
    /* Ring the doorbell */
    xhci->dbreg[0] = 0;

    while (TRB_GET(TT, xhci->cr.cur) == TRB_LINK) {
        usb_debug("Handling LINK pointer (@%p)\n", xhci->cr.cur);
        const int tc = TRB_GET(TC, xhci->cr.cur);
        TRB_SET(C, xhci->cr.cur, xhci->cr.pcs);
        xhci->cr.cur = phys_to_virt(xhci->cr.cur->ptr_low);
        if (tc)
            xhci->cr.pcs ^= 1;
    }

    if (pdPASS != xSemaphoreTake(cmd_info->xSemaphore, 5000 / portTICK_RATE_MS)) {
        usb_err("%s: wait cmd done timeout\n", __func__);
        xhci_abort_command(xhci, cmd_info);
        return -1;
    }

    return 0;
}

int xhci_cmd_enable_slot(xhci_t *const xhci, int *const slot_id)
{
    int ret;
    struct xhci_cmd_info *cmd_info = xhci_alloc_command(xhci);
    if (!cmd_info)
        return -1;

    cmd_info->cmd = xhci_next_command_trb(xhci);
    TRB_SET(TT, cmd_info->cmd, TRB_CMD_ENABLE_SLOT);
    ret = xhci_post_command(xhci, cmd_info);
    if (ret)
        return ret;

    *slot_id = cmd_info->slot_id;
    ret = cmd_info->status;
    xhci_free_command(xhci, cmd_info);
    return ret;
}

int xhci_cmd_disable_slot(xhci_t *const xhci, const int slot_id)
{
    int ret;
    struct xhci_cmd_info *cmd_info = xhci_alloc_command(xhci);
    if (!cmd_info)
        return -1;

    cmd_info->cmd = xhci_next_command_trb(xhci);
    TRB_SET(TT, cmd_info->cmd, TRB_CMD_DISABLE_SLOT);
    TRB_SET(ID, cmd_info->cmd, slot_id);
    ret = xhci_post_command(xhci, cmd_info);
    if (ret)
        return ret;

    ret = cmd_info->status;
    xhci_free_command(xhci, cmd_info);
    return ret;
}

int xhci_cmd_address_device(xhci_t *const xhci,
                            const int slot_id,
                            inputctx_t *const ic)
{
    int ret;
    struct xhci_cmd_info *cmd_info = xhci_alloc_command(xhci);
    if (!cmd_info)
        return -1;

    cmd_info->cmd = xhci_next_command_trb(xhci);
    TRB_SET(TT, cmd_info->cmd, TRB_CMD_ADDRESS_DEV);
    TRB_SET(ID, cmd_info->cmd, slot_id);
    cmd_info->cmd->ptr_low = virt_to_phys(ic->raw);
    ret = xhci_post_command(xhci, cmd_info);
    if (ret)
        return ret;

    ret = cmd_info->status;
    xhci_free_command(xhci, cmd_info);
    return ret;
}

int xhci_cmd_configure_endpoint(xhci_t *const xhci,
                                const int slot_id,
                                const int config_id,
                                inputctx_t *const ic)
{
    int ret;
    struct xhci_cmd_info *cmd_info = xhci_alloc_command(xhci);
    if (!cmd_info)
        return -1;

    cmd_info->cmd = xhci_next_command_trb(xhci);
    TRB_SET(TT, cmd_info->cmd, TRB_CMD_CONFIGURE_EP);
    TRB_SET(ID, cmd_info->cmd, slot_id);
    cmd_info->cmd->ptr_low = virt_to_phys(ic->raw);
    if (config_id == 0)
        TRB_SET(DC, cmd_info->cmd, 1);
    ret = xhci_post_command(xhci, cmd_info);
    if (ret)
        return ret;

    ret = cmd_info->status;
    xhci_free_command(xhci, cmd_info);
    return ret;
}

int xhci_cmd_evaluate_context(xhci_t *const xhci,
                              const int slot_id,
                              inputctx_t *const ic)
{
    int ret;
    struct xhci_cmd_info *cmd_info = xhci_alloc_command(xhci);
    if (!cmd_info)
        return -1;

    cmd_info->cmd = xhci_next_command_trb(xhci);
    TRB_SET(TT, cmd_info->cmd, TRB_CMD_EVAL_CTX);
    TRB_SET(ID, cmd_info->cmd, slot_id);
    cmd_info->cmd->ptr_low = virt_to_phys(ic->raw);

    ret = xhci_post_command(xhci, cmd_info);
    if (ret)
        return ret;

    ret = cmd_info->status;
    xhci_free_command(xhci, cmd_info);
    return ret;
}

int xhci_cmd_reset_endpoint(xhci_t *const xhci, const int slot_id, const int ep)
{
    int ret;
    struct xhci_cmd_info *cmd_info = xhci_alloc_command(xhci);
    if (!cmd_info)
        return -1;

    cmd_info->cmd = xhci_next_command_trb(xhci);
    TRB_SET(TT, cmd_info->cmd, TRB_CMD_RESET_EP);
    TRB_SET(ID, cmd_info->cmd, slot_id);
    TRB_SET(EP, cmd_info->cmd, ep);

    ret = xhci_post_command(xhci, cmd_info);
    if (ret)
        return ret;

    ret = cmd_info->status;
    xhci_free_command(xhci, cmd_info);
    return ret;
}

int xhci_cmd_stop_endpoint(xhci_t *const xhci, const int slot_id, const int ep)
{
    int ret;
    struct xhci_cmd_info *cmd_info = xhci_alloc_command(xhci);
    if (!cmd_info)
        return -1;

    cmd_info->cmd = xhci_next_command_trb(xhci);
    TRB_SET(TT, cmd_info->cmd, TRB_CMD_STOP_EP);
    TRB_SET(ID, cmd_info->cmd, slot_id);
    TRB_SET(EP, cmd_info->cmd, ep);

    ret = xhci_post_command(xhci, cmd_info);
    if (ret)
        return ret;

    ret = cmd_info->status;
    xhci_free_command(xhci, cmd_info);
    return ret;
}

int xhci_cmd_set_tr_dq(xhci_t *const xhci, const int slot_id, const int ep,
                       trb_t *const dq_trb, const int dcs)
{
    int ret;
    struct xhci_cmd_info *cmd_info = xhci_alloc_command(xhci);
    if (!cmd_info)
        return -1;

    cmd_info->cmd = xhci_next_command_trb(xhci);
    TRB_SET(TT, cmd_info->cmd, TRB_CMD_SET_TR_DQ);
    TRB_SET(ID, cmd_info->cmd, slot_id);
    TRB_SET(EP, cmd_info->cmd, ep);
    cmd_info->cmd->ptr_low = virt_to_phys(dq_trb) | dcs;

    ret = xhci_post_command(xhci, cmd_info);
    if (ret)
        return ret;

    ret = cmd_info->status;
    xhci_free_command(xhci, cmd_info);
    return ret;
}
