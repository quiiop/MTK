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
 * MediaTek Inc. (C) 2022. All rights reserved.
 *
 * Author:
 *  Tianping Fang<tianping.fang@mediatek.com>
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

#include <usbuac.h>
#include <xhci_private.h>
#include <unistd.h>
#include <queue.h>

struct uac_device *uac_udev;
QueueHandle_t uac_queue[FUNC_NUM] = {NULL};
extern QueueHandle_t write_queue;

static void print_ac_header_desc(struct uac1_header_descriptor *ac_header)
{
    int i = 0;

    if (!ac_header)
        return;

    uac_debug("**** Control Interface Header Descriptor ****\n");
    uac_debug("bLength            : 0x%x \n", ac_header->bLength);
    uac_debug("bDescriptorType    : 0x%x \n", ac_header->bDescriptorType);
    uac_debug("bDescriptorSubType : 0x%x \n", ac_header->bDescriptorSubtype);
    uac_debug("bcdADC             : 0x%x \n", ac_header->bcdADC);
    uac_debug("wTotalLength       : 0x%x \n", ac_header->wTotalLength);
    uac_debug("dwClockFrequency   : 0x%x \n", ac_header->bInCollection);
    for (i = 0; i < ADC_INTERFACE_NBR; i++) {
        uac_debug("baInterfaceNr[%d]   :0x%x \n", i, ac_header->baInterfaceNr[i]);
    }
}

static void print_ac_input_terminal(struct uac_input_terminal_descriptor *ac_in_term)
{
    if (!ac_in_term)
        return;

    uac_debug("**** AUDIO Input Terminal Descriptor ****\n");
    uac_debug("bLength            : 0x%x \n", ac_in_term->bLength);
    uac_debug("bDescriptorType    : 0x%x \n", ac_in_term->bDescriptorType);
    uac_debug("bDescriptorSubType : 0x%x \n", ac_in_term->bDescriptorSubtype);
    uac_debug("bTerminalID        : 0x%x \n", ac_in_term->bTerminalID);
    uac_debug("wTerminalType      : 0x%x \n", ac_in_term->wTerminalType);
    uac_debug("bAssocTerminal     : 0x%x \n", ac_in_term->bAssocTerminal);
    uac_debug("bNrChannels        : 0x%x \n", ac_in_term->bNrChannels);
    uac_debug("wChannelConfig     : 0x%x \n", ac_in_term->wChannelConfig);
    uac_debug("iChannelNames      : 0x%x \n", ac_in_term->iChannelNames);
    uac_debug("iTerminal          : 0x%x \n", ac_in_term->iTerminal);
}

static void print_ac_output_terminal
(struct uac_output_terminal_descriptor *ac_out_term)
{
    if (!ac_out_term)
        return;

    uac_debug("**** AUDIO Output Terminal Descriptor ****\n");
    uac_debug("bLength            : 0x%x \n", ac_out_term->bLength);
    uac_debug("bDescriptorType    : 0x%x \n", ac_out_term->bDescriptorType);
    uac_debug("bDescriptorSubType : 0x%x \n", ac_out_term->bDescriptorSubtype);
    uac_debug("bTerminalID        : 0x%x \n", ac_out_term->bTerminalID);
    uac_debug("wTerminalType      : 0x%x \n", ac_out_term->wTerminalType);
    uac_debug("bAssocTerminal     : 0x%x \n", ac_out_term->bAssocTerminal);
    uac_debug("bSourceID          : 0x%x \n", ac_out_term->bSourceID);
    uac_debug("iTerminal          : 0x%x \n", ac_out_term->iTerminal);
}

static void print_ac_sel_unit
(struct uac_selector_unit_descriptor *ac_sel_unit)
{
    if (!ac_sel_unit)
        return;

    uac_debug("**** AUDIO Select Unit Descriptor ****\n");
    uac_debug("bLength            : 0x%x \n", ac_sel_unit->bLength);
    uac_debug("bDescriptorType    : 0x%x \n", ac_sel_unit->bDescriptorType);
    uac_debug("bDescriptorSubType : 0x%x \n", ac_sel_unit->bDescriptorSubtype);
    uac_debug("bUintID            : 0x%x \n", ac_sel_unit->bUintID);
    uac_debug("bNrInPins          : 0x%x \n", ac_sel_unit->bNrInPins);
    uac_debug("baSourceID         : 0x%x \n", ac_sel_unit->baSourceID);
}

static void print_ac_fea_unit
(struct uac_feature_unit_descriptor *ac_fea_unit)
{
    if (!ac_fea_unit)
        return;

    uac_debug("**** AUDIO Feature Unit Descriptor ****\n");
    uac_debug("bLength            : 0x%x \n", ac_fea_unit->bLength);
    uac_debug("bDescriptorType    : 0x%x \n", ac_fea_unit->bDescriptorType);
    uac_debug("bDescriptorSubType : 0x%x \n", ac_fea_unit->bDescriptorSubtype);
    uac_debug("bUnitID            : 0x%x \n", ac_fea_unit->bUnitID);
    uac_debug("bSourceID          : 0x%x \n", ac_fea_unit->bSourceID);
    uac_debug("bControlSize       : 0x%x \n", ac_fea_unit->bControlSize);
}

static void print_ac_mixer_unit
(struct uac_mixer_unit_descriptor *ac_mix_unit)
{
    if (!ac_mix_unit)
        return;

    uac_debug("**** AUDIO Mixer Unit Descriptor ****\n");
    uac_debug("bLength            : 0x%x \n", ac_mix_unit->bLength);
    uac_debug("bDescriptorType    : 0x%x \n", ac_mix_unit->bDescriptorType);
    uac_debug("bDescriptorSubType : 0x%x \n", ac_mix_unit->bDescriptorSubtype);
    uac_debug("bUnitID            : 0x%x \n", ac_mix_unit->bUnitID);
    uac_debug("bSourceID          : 0x%x \n", ac_mix_unit->bNrInPins);
    uac_debug("baSourceID[0]      : 0x%x \n", ac_mix_unit->baSourceID[0]);
}

static void print_as_intf_head(struct uac_as_header_descriptor *as_head)
{
    if (!as_head)
        return;

    uac_debug("**** Audio Streaming Interface Descriptor ****\n");
    uac_debug("bLength            : 0x%x \n", as_head->bLength);
    uac_debug("bDescriptorType    : 0x%x \n", as_head->bDescriptorType);
    uac_debug("bDescriptorSubType : 0x%x \n", as_head->bDescriptorSubtype);
    uac_debug("bTerminalLink      : 0x%x \n", as_head->bTerminalLink);
    uac_debug("bDelay             : 0x%x \n", as_head->bDelay);
    uac_debug("wFormatTag         : 0x%x \n", as_head->wFormatTag);
}

static void print_as_intf_fmt(struct uac_as_format_type_descriptor *fmt)
{
    int i;

    if (!fmt)
        return;

    uac_debug("**** Audio Streaming Format Type Descriptor ****\n");
    uac_debug("bLength            : 0x%x \n", fmt->bLength);
    uac_debug("bDescriptorType    : 0x%x \n", fmt->bDescriptorType);
    uac_debug("bDescriptorSubType : 0x%x \n", fmt->bDescriptorSubtype);
    uac_debug("bFormatType        : 0x%x \n", fmt->bFormatType);
    uac_debug("bNrChannels        : 0x%x \n", fmt->bNrChannels);
    uac_debug("bSubframeSize      : 0x%x \n", fmt->bSubframeSize);
    uac_debug("bBitResolution     : 0x%x \n", fmt->bBitResolution);
    uac_debug("bSamFreqType       : 0x%x \n", fmt->bSamFreqType);
    for (i = 0; i < fmt->bSamFreqType; i++) {
        uac_debug("tSamFreq[%d]        : 0x%x \n", i, fmt->tSamFreq[i][0]);
        uac_debug("tSamFreq[%d]        : 0x%x \n", i, fmt->tSamFreq[i][1]);
        uac_debug("tSamFreq[%d]        : 0x%x \n", i, fmt->tSamFreq[i][2]);
    }
}

int uac_semaphore_lock(struct uac_streaming *stream)
{
    uac_debug("uac_semaphore_lock\n");
    if (pdPASS != xSemaphoreTake(stream->xSemaphore,
                                 500 / portTICK_RATE_MS)) {
        uac_err("take semphore timeout\n");
        return -1;
    }
    uac_debug("uac_semaphore_lock-end\n");

    return 0;
}

int uac_semaphore_unlock(struct uac_streaming *stream)
{
    uac_debug("uac_semaphore_unlock\n");
    if (pdPASS != xSemaphoreGive(stream->xSemaphore)) {
        uac_err("give semphore fail\n");
        return -1;
    }
    uac_debug("uac_semaphore_unlock -end\n");

    return 0;
}

static void uac_init_ring_buffer(struct uac_streaming *stream,
                                 uint8_t *base)
{
    stream->share_buf.buffer_base = base;
    stream->share_buf.buffer_size = UAC_RING_BUFFER_LENGTH;
    stream->share_buf.write = 0;
    stream->share_buf.read = 0;
    stream->share_buf.waiting = false;
    stream->share_buf.underflow = false;
}

void uac_get_pl_buffer_write_pos(struct uac_streaming *stream,
                                 uint8_t **buffer, uint32_t *length)
{
    int32_t count = 0;

    if (stream->share_buf.read > stream->share_buf.write) {
        count = stream->share_buf.read - stream->share_buf.write - 1;
    } else if (stream->share_buf.read == 0) {
        count = stream->share_buf.buffer_size - stream->share_buf.write - 1;
    } else {
        count = stream->share_buf.buffer_size - stream->share_buf.write;
    }
    *buffer = stream->share_buf.buffer_base + stream->share_buf.write;
    *length = count;
}

void uac_set_pl_buffer_write_done(struct uac_streaming *stream,
                                  uint32_t length)
{
    stream->share_buf.write += length;
    if (stream->share_buf.write == stream->share_buf.buffer_size) {
        stream->share_buf.write = 0;
    }
}

void uac_finish_write(struct uac_streaming *stream)
{
    stream->share_buf.waiting = false;
    stream->share_buf.underflow = false;
}

void uac_get_pl_buffer_read_pos(struct uac_streaming *stream,
                                uint8_t **buffer, uint32_t *length)
{
    int32_t count = 0;

    if (stream->share_buf.write >= stream->share_buf.read) {
        count = stream->share_buf.write - stream->share_buf.read;
    } else {
        count = stream->share_buf.buffer_size - stream->share_buf.read;
    }
    *buffer = stream->share_buf.buffer_base + stream->share_buf.read;
    *length = count;
}

void uac_set_pl_buffer_read_done(struct uac_streaming *stream,
                                 uint32_t length)
{
    stream->share_buf.read += length;
    if (stream->share_buf.read == stream->share_buf.buffer_size) {
        stream->share_buf.read = 0;
    }
}

int32_t uac_get_pl_buffer_data_count(struct uac_streaming *stream)
{
    int32_t count = 0;

    count = stream->share_buf.write - stream->share_buf.read;
    if (count < 0) {
        count += stream->share_buf.buffer_size;
    }

    return count;
}

static uint8_t *uac_alloc_ring_buffer(int size)
{
    uint8_t *ring_buffer;

    ring_buffer = pvPortMallocNC(sizeof(uint8_t) * size);
    if (NULL == ring_buffer) {
        uac_err("Cannot alloc stream_buffer\r\n");
        return NULL;
    }
    memset(ring_buffer, 0, size);
    return ring_buffer;
}

void uac_reinit_ring_buffer(struct uac_streaming *stream)
{
    memset(stream->share_buf.buffer_base, 0,
           stream->share_buf.buffer_size);
    stream->share_buf.write = 0;
    stream->share_buf.read = 0;
    stream->share_buf.waiting = false;
    stream->share_buf.underflow = false;
}

void uac_init_ring_buffer_data(struct uac_streaming *stream)
{
    uint8_t *share_buf;
    uint32_t share_buf_len;
    UINT f_br;

    uac_get_pl_buffer_write_pos(stream, &share_buf, &share_buf_len);
    f_read(&stream->fid, share_buf, share_buf_len, &f_br);
    uac_set_pl_buffer_write_done(stream, share_buf_len);
    uac_finish_write(stream);
}

static inline unsigned get_usb_full_speed_rate(unsigned int rate)
{
    return ((rate << 13) + 62) / 125;
}

static inline unsigned get_usb_high_speed_rate(unsigned int rate)
{
    return ((rate << 10) + 62) / 125;
}

int uac_get_packet_size(struct ep_attr *ep)
{
    int ret;

    ep->sample_accum += ep->sample_rem;
    if (ep->sample_accum >= ep->pps) {
        ep->sample_accum -= ep->pps;
        ret = ep->datasize[1];
    } else {
        ret = ep->datasize[0];
    }

    return ret;
}

int uac_submit_urb(struct uac_streaming *stream, struct urb *urb)
{
    int ret;
    endpoint_t *ep = stream->ep;

    taskENTER_CRITICAL();
    ret = ep->dev->controller->isochronous(ep, urb);
    if (ret < 0) {
        uac_err("**uac_submit urb fail** \n");
        taskEXIT_CRITICAL();
        return ret;
    }
    taskEXIT_CRITICAL();
    return 0;
}

int uac_prepare_slience_urb(struct urb *urb)
{
    struct uac_streaming *stream = urb->context;
    struct ep_attr *ep_att = stream->ep_att;
    int i, counts, frames = 0, ret = 0;
    uint32_t bytes = 0;

    uac_err("uac_prepare_slience_urb!\n");
    urb->number_of_packets = 0;
    for (i = 0; i < stream->isoc_packet; i++) {
        counts = uac_get_packet_size(ep_att);
        /* set up descriptor */
        urb->iso_frame_desc[i].offset = frames * ep_att->stride;
        urb->iso_frame_desc[i].length = counts * ep_att->stride;
        urb->number_of_packets++;
        frames += counts;
    }
    bytes = frames * ep_att->stride;
    memset(urb->buffer, ep_att->slience_value, bytes);

    ret = uac_submit_urb(stream, urb);
    if (ret)
        uac_err("slience: submit urb failed!\n");

    return 0;
}

int uac_prepare_playback_urb(struct urb *urb)
{
    int i, counts, frames = 0, ret = 0;
    struct uac_streaming *stream = urb->context;
    struct ep_attr *ep_att = stream->ep_att;
    uint32_t bytes = 0;
    uint8_t *share_buf, *remain_buffer;
    uint32_t share_buf_len, remain_buf_len, remain_len, tmp_len, base_len;
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;

    uac_semaphore_lock(stream);
    uac_get_pl_buffer_read_pos(stream, &share_buf, &share_buf_len);
    if ((share_buf_len == 0) || stream->file_flag) {
        if (share_buf_len == 0)
            uac_crit("data is not enough!\n");
        else {
            uac_crit("playback is end!\n");
            stream->play_status = false;
        }
        uac_semaphore_unlock(stream);
        return -1;
    }
    memset(urb->buffer, 0, urb->transfer_buffer_length);
    urb->number_of_packets = 0;
    for (i = 0; i < stream->isoc_packet; i++) {
        counts = uac_get_packet_size(ep_att);
        /* set up descriptor */
        urb->iso_frame_desc[i].offset = frames * ep_att->stride;
        urb->iso_frame_desc[i].length = counts * ep_att->stride;
        urb->number_of_packets++;
        frames += counts;
    }
    bytes = frames * ep_att->stride;

    if (share_buf_len >= bytes) {
        memcpy(urb->buffer, share_buf, bytes);
        uac_set_pl_buffer_read_done(stream, bytes);
    } else {
        base_len = tmp_len = share_buf_len;
        remain_len = bytes - tmp_len;
        memcpy(urb->buffer, share_buf, share_buf_len);
        uac_set_pl_buffer_read_done(stream, share_buf_len);
        while (remain_len > 0) {
            if ((stream->share_buf.write == stream->share_buf.read)) {
                uac_crit("reach the boundary!\n");
                stream->play_status = false;
                memset(urb->buffer + base_len,
                       ep_att->slience_value, remain_len);
                break;
            }
            uac_get_pl_buffer_read_pos(stream, &remain_buffer, &remain_buf_len);
            if (remain_buf_len >= remain_len) {
                memcpy(urb->buffer + base_len, remain_buffer, remain_len);
                uac_set_pl_buffer_read_done(stream, remain_len);
                break;
            } else {
                memcpy(urb->buffer + base_len, remain_buffer, remain_buf_len);
                base_len += remain_buf_len;
                remain_len -= remain_buf_len;
                uac_set_pl_buffer_read_done(stream, remain_buf_len);
            }
        }
    }

    if ((uac_get_pl_buffer_data_count(stream) <= UAC_RING_BUFFER_LENGTH / 4) &&
        !stream->file_flag) {
        if (write_queue) {
            if (pdPASS != xQueueSendFromISR(write_queue,
                                            &stream, &xHigherPriorityTaskWoken))
                uac_err("%s: send write queue fail\n", __func__);
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR(pdTRUE);
        } else if (stream->pl_callback)
            stream->pl_callback(stream);
    }

    uac_semaphore_unlock(stream);
    urb->transfer_buffer_length = bytes;

    ret = uac_submit_urb(stream, urb);
    if (ret)
        uac_err("playback: submit urb failed!\n");

    return 0;
}

static int uac_handle_inbound_urb(struct urb *urb)
{
    struct uac_streaming *stream = urb->context;
    int i, tmp_len, offset, ret = 0;
    uint8_t *data;

    for (i = 0; i < urb->number_of_packets; i++) {
        if (urb->iso_frame_desc[i].status < 0) {
            uac_err("inbound urb status: %d\n",
                    urb->iso_frame_desc[i].status);
            continue;
        }

        tmp_len = urb->iso_frame_desc[i].actual_length;
        if (tmp_len == 0) {
            uac_debug(" len is 0, continue\n");
            continue;
        }
        offset = urb->iso_frame_desc[i].offset;
        data = urb->buffer + offset;
        if (stream->cap_callback)
            stream->cap_callback(data, tmp_len);
    }
    memset(urb->buffer, 0, urb->transfer_buffer_length);

    ret = uac_submit_urb(stream, urb);
    if (ret)
        uac_err("capture: submit urb failed!\n");

    return 0;
}

static void uac_prepare_playback_task(void *arg)
{
    struct urb *urb;

    do {
        if (pdPASS == xQueueReceive(uac_queue[0],
                                    &urb, 5)) {
            uac_prepare_playback_urb(urb);
        }
    } while (1);
}

static void uac_handle_capture_task(void *arg)
{
    struct urb *urb;

    do {
        if (pdPASS == xQueueReceive(uac_queue[1],
                                    &urb, 5)) {
            uac_handle_inbound_urb(urb);
        }
    } while (1);
}

static int uac_urb_complete(struct urb *urb)
{
    struct uac_streaming *stream = urb->context;
    endpoint_t ep = stream->stream_desc->ep;
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;

    if (stream->play_status) {
        if ((ep.direction == OUT) && !stream->pl_callback)
            uac_prepare_slience_urb(urb);
        else {
            if (pdPASS != xQueueSendFromISR(stream->urb_queue,
                                            &urb, &xHigherPriorityTaskWoken))
                uac_debug("%s: pl send urb to queue fail\n", __func__);
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR(pdTRUE);
        }
    } else {
        uac_debug("urb complete stop\n");
        if (stream->stop_callback) {
            stream->stop_callback(stream);
        }
    }

    return 0;
}

static void uac_parse_control_descriptor(struct uac_device *udev,
                                         char *buf)
{
    struct uac_control_desc *control_desc = &udev->control_desc;

    switch (buf[2]) {
        /* audio control subclass */
        case UAC_HEADER:
            control_desc->uac1_control_header =
                (struct uac1_header_descriptor *)buf;
            print_ac_header_desc(control_desc->uac1_control_header);
            break;
        case UAC_INPUT_TERMINAL:
            if (udev->input_terminal_num < ADC_MAX_TERMINAL) {
                control_desc->uac_input_terminal[udev->input_terminal_num] =
                    (struct uac_input_terminal_descriptor *)buf;
                print_ac_input_terminal(control_desc->uac_input_terminal
                                        [udev->input_terminal_num]);
                udev->input_terminal_num++;
            }
            break;
        case UAC_OUTPUT_TERMINAL:
            if (udev->output_terminal_num < ADC_MAX_TERMINAL) {
                control_desc->uac_output_terminal[udev->output_terminal_num] =
                    (struct uac_output_terminal_descriptor *)buf;
                print_ac_output_terminal(control_desc->uac_output_terminal
                                         [udev->output_terminal_num]);
                udev->output_terminal_num++;
            }
            break;
        case UAC_SELECTOR_UNIT:
            if (udev->select_unit_num < ADC_MAX_SEL_UNIT) {
                control_desc->uac_sel_unit[udev->select_unit_num] =
                    (struct uac_selector_unit_descriptor *)buf;
                print_ac_sel_unit(control_desc->uac_sel_unit[udev->select_unit_num]);
                udev->select_unit_num++;
            }
            break;
        case UAC_FEATURE_UNIT:
            if (udev->feature_unit_num < ADC_MAX_FEATURE_UNIT) {
                control_desc->uac_fea_unit[udev->feature_unit_num] =
                    (struct uac_feature_unit_descriptor *)buf;
                print_ac_fea_unit(control_desc->uac_fea_unit[udev->feature_unit_num]);
                udev->feature_unit_num++;
            }
            break;
        case UAC_MIXER_UNIT:
            if (udev->mixer_unit_num < ADC_MAX_MIX_UNIT) {
                control_desc->uac_mixer_unit[udev->mixer_unit_num] =
                    (struct uac_mixer_unit_descriptor *)buf;
                print_ac_mixer_unit(control_desc->uac_mixer_unit[udev->mixer_unit_num]);
                udev->mixer_unit_num++;
            }
            break;
        default:
            break;
    };
}

static void uac_parser_stream_descriptor(struct uac_device *udev,
                                         char *buf, int inf_index, int itf_num, int alt)
{
    struct uac_stream_desc *stream_desc;
    int dir, i;

    dir = (udev->dev->config.intf[inf_index].ep_desc[0].bEndpointAddress &
           USB_DIR_IN) ? 1 : 0;
    for (i = 0; i < FUNC_NUM; i++) {
        if (!udev->stream_desc[dir][i].valid) {
            stream_desc = &udev->stream_desc[dir][i];
            break;
        } else {
            uac_err("Can't find available resource\n");
            return;
        }
    }
    switch (buf[2]) {
        /* Audio streamimg subclass */
        case UAC_AS_GENERAL:
            stream_desc->as_header = (struct uac_as_header_descriptor *)buf;
            stream_desc->intface_index = inf_index;
            stream_desc->alternatesetting = alt;
            stream_desc->intface_num = itf_num;
            print_as_intf_head(stream_desc->as_header);
            break;
        case UAC_FORMAT_TYPE:
            stream_desc->fmt = (struct uac_as_format_type_descriptor *)buf;
            stream_desc->valid = true;
            print_as_intf_fmt(stream_desc->fmt);
            break;
        default:
            break;
    };
}

static int uac_find_interface_index(struct uac_device *udev,
                                    int itf_num, int alt_setting)
{
    int i;

    for (i = 0; i < MAX_INTERFACES; i++) {
        if ((udev->dev->config.intf[i].intf_desc.bInterfaceNumber == itf_num) &&
            (udev->dev->config.intf[i].intf_desc.bAlternateSetting == alt_setting))
            return i;
    }

    return -1;
}

static void uac_parse_descriptors(usbdev_t *dev)
{
    struct uac_device *udev;
    usb_descheader_t *desc;
    char *buf;
    u32 config_len;
    u32 itf_num = 0, alt_setting = 0;
    int itf_index = 0;
    u32 sub_class = 0;
    u32 length = 0;

    udev = (struct uac_device *)(dev->data);
    desc = (usb_descheader_t *)dev->configuration;
    config_len = dev->configuration->wTotalLength;
    while (length < config_len) {
        desc = usb_get_nextdesc((u8 *)desc, &length);
        buf = (char *)desc;

        switch (desc->bDescriptorType) {
            case DT_INTF:
                itf_num = *(buf + 2);
                alt_setting = *(buf + 3);
                itf_index = uac_find_interface_index(udev, itf_num, alt_setting);
                if (itf_index >= 0)
                    sub_class = udev->dev->config.intf[itf_index].intf_desc.bInterfaceSubClass;
                break;
            case UAC_CS_INTERFACE:
                if (sub_class == USB_SUBCLASS_AUDIOCONTROL)
                    uac_parse_control_descriptor(udev, buf);
                else if (sub_class == USB_SUBCLASS_AUDIOSTREAMING)
                    uac_parser_stream_descriptor(udev, buf,
                                                 itf_index, itf_num, alt_setting);
                break;
            default:
                break;
        }
    }

}

static void uac_set_ep_para(struct uac_streaming *stream, int rate,
                            endpoint_t *endpoint, u8 channels, u8 dir)
{
    struct uac_device *udev = stream->udev;
    struct ep_attr *ep = stream->ep_att;
    int frame_bits;
    int pcm_format = 16;
    int minsize, maxsize;
    int packs_per_ms, max_packs_per_urb, max_urbs;
    int max_packs_per_period, urbs_per_period, urb_packs;

    frame_bits = channels * pcm_format;
    ep->rate = rate;
    ep->channels = channels;

    if (udev->dev->speed == FULL_SPEED) {
        ep->freqn = get_usb_full_speed_rate(rate);
        ep->datainterval = 0;
        ep->pps = 1000 >> ep->datainterval;
        packs_per_ms = 1;
        max_packs_per_urb = MAX_PACKS;
    } else {
        ep->freqn = get_usb_high_speed_rate(rate);
        ep->datainterval = endpoint->interval - 1;
        ep->pps = 8000 >> ep->datainterval;
        packs_per_ms = 8 >> ep->datainterval;
        max_packs_per_urb = MAX_PACKS_HS;
    }

    ep->sample_rem = rate % ep->pps;
    ep->datasize[0] = rate / ep->pps;
    ep->datasize[1] = (rate + (ep->pps - 1)) / ep->pps;
    ep->freqm = ep->freqn;
    ep->maxpacksize = endpoint->maxpacketsize;
    ep->slience_value = 0x0;
    ep->stride = frame_bits >> 3;
    ep->dir = dir;
    ep->fill_max = 0;
    ep->freqmax = ep->freqn + (ep->freqn >> 1);
    maxsize = (((ep->freqmax << ep->datainterval) + 0xffff) >> 16) *
              (frame_bits >> 3);
    if (ep->maxpacksize && ep->maxpacksize < maxsize) {
        /* whatever fits into a max. size packet */
        int data_maxsize = maxsize = ep->maxpacksize;

        ep->freqmax = (data_maxsize / (frame_bits >> 3))
                      << (16 - ep->datainterval);
    }
    if (ep->fill_max)
        ep->curpacksize = ep->maxpacksize;
    else
        ep->curpacksize = maxsize;

    /*Capture*/
    if (ep->dir) {
        ep->nurbs = 2;
        ep->npackets = MAX_PACKS;
    } else {
        /*playback*/
        minsize = (ep->freqn >> (16 - ep->datainterval)) *
                  (frame_bits >> 3);

        max_packs_per_period = DIV_ROUND_UP(PERIOD_BYTES, minsize);
        /* calc URBs will contain a period */
        urbs_per_period = DIV_ROUND_UP(max_packs_per_period,
                                       max_packs_per_urb);
        /* calc packets are needed in each URB */
        urb_packs = DIV_ROUND_UP(max_packs_per_period, urbs_per_period);
        ep->npackets = urb_packs;

        /* limit the number of frames in a single URB */
        ep->max_urb_frames = DIV_ROUND_UP(FRAMES_PER_PERIOD,
                                          urbs_per_period);
        max_urbs = min(MAX_URBS,
                       MAX_QUEUE * packs_per_ms / urb_packs);
        ep->nurbs = min(max_urbs, urbs_per_period * PERIODS_PER_BUFFER);
    }
}

static void uac_ep_init(struct uac_device *udev,
                        endpoint_t *endpoint, endpoint_descriptor_t ep_desc)
{
    usbdev_t *dev = udev->dev;
    endpoint_t *ep = endpoint;

    ep->dev = udev->dev;
    ep->type = ep_desc.bmAttributes & 0x3;
    ep->direction = (ep_desc.bEndpointAddress & 0x80) ? IN : OUT;
    ep->endpoint = ep_desc.bEndpointAddress;
    ep->interval = usb_decode_interval(dev->speed, ep->type, ep_desc.bInterval);
    ep->maxpacketsize = ep_desc.wMaxPacketSize;
}

static void uac_stream_init(struct uac_device *udev)
{
    usbdev_t *dev = udev->dev;
    struct uac_stream_desc *stream_desc;
    endpoint_t *ep;
    endpoint_descriptor_t ep_desc;
    struct uac_streaming *stream;
    int itf_num = 0, i;

    for (i = 0; i < FUNC_NUM; i++) {
        /*initial endpoint*/
        stream_desc = &udev->stream_desc[i][0];
        stream = udev->streaming[i];
        itf_num = stream_desc->intface_index;
        ep_desc = dev->config.intf[itf_num].ep_desc[0];
        ep = &stream_desc->ep;
        uac_ep_init(udev, ep, ep_desc);

        stream->ep = &stream_desc->ep;
        stream->stream_desc = stream_desc;
    }
}

static int uac_get_channel(struct uac_streaming *stream)
{
    return stream->stream_desc->fmt->bNrChannels;
}

void uac_set_channel(struct uac_streaming *stream, int channels)
{
    int ch;

    ch = uac_get_channel(stream);
    if (channels > ch) {
        uac_err("channel is not mach: ch1 =%d, ch2 = %d\n",
                channels, ch);
        channels = ch;
    }
    stream->ep_att->channels = channels;
}

int uac_sel_sample_rate(struct uac_streaming *stream, int sample)
{
    struct uac_stream_desc *stream_desc = stream->stream_desc;
    struct uac_as_format_type_descriptor *fmt = stream_desc->fmt;
    int tmp = 0, i;

    for (i = 0; i < fmt->bSamFreqType; i++) {
        tmp = fmt->tSamFreq[i][0] | (fmt->tSamFreq[i][1] << 8) |
              (fmt->tSamFreq[i][2] << 16);
        if (tmp == sample) {
            uac_crit("find the avaliable sample rate: %d\n", tmp);
            stream->ep_att->rate = sample;
            return 0;
        }
    }
    uac_err("sample rate is error\n");
    return -1;
}

static int uac_set_sample_rate(struct uac_device *udev,
                               int rate, int ep_num)
{
    int ret;
    dev_req_t *dr;
    u8 *data;

    data = (u8 *)pvPortMallocNC(3);
    if (! data) {
        uac_err("Not enough memory for USB UAC.\n");
        return 0;
    }

    data[0] = rate;
    data[1] = rate >> 8;
    data[2] = rate >> 16;

    dr = pvPortMallocNC(sizeof(dev_req_t));
    if (dr == NULL) {
        uac_err("%s: Out of memory\n", __func__);
        return -1;
    };

    dr->bmRequestType = gen_bmRequestType(host_to_device,
                                          class_type, endp_recp);
    dr->data_dir = host_to_device;
    dr->bRequest = UAC_SET_CUR;
    dr->wValue = UAC_EP_CS_ATTR_SAMPLE_RATE << 8;
    dr->wIndex = ep_num;
    dr->wLength = 3;
    ret = usb_control_msg(udev->dev, OUT, dr, 3, data);

    data[0] = 0;
    data[1] = 0;
    data[2] = 0;

    dr->bmRequestType = gen_bmRequestType(device_to_host,
                                          class_type, endp_recp);
    dr->data_dir = device_to_host;
    dr->bRequest = UAC_GET_CUR;
    dr->wValue = UAC_EP_CS_ATTR_SAMPLE_RATE << 8;
    dr->wIndex = ep_num;
    dr->wLength = 3;
    ret = usb_control_msg(udev->dev, IN, dr, 3, data);

    vPortFreeNC((void *)dr);
    vPortFreeNC((void *)data);
    return ret;
}

static void uac_ep_para_setting(struct uac_streaming *stream, u8 mode)
{
    int channels, rate;
    struct uac_stream_desc *stream_desc = stream->stream_desc;
    endpoint_t *ep = &stream_desc->ep;

    channels = stream->ep_att->channels;
    rate = stream->ep_att->rate;

    uac_set_ep_para(stream, rate, ep, channels, mode);
}

static int uac_init_stream_buffer(struct uac_streaming *stream)
{
    uint8_t *buffer_base = NULL;

    buffer_base = uac_alloc_ring_buffer(UAC_RING_BUFFER_LENGTH);
    if (NULL == buffer_base) {
        uac_err("Cannot alloc stream_buffer\r\n");
        return -1;
    }
    uac_init_ring_buffer(stream, buffer_base);
    return 0;
}

struct uac_streaming *uac_stream_new(struct uac_device *udev, int dir)
{
    struct uac_streaming *stream;
    int ret;

    stream = (struct uac_streaming *)
             pvPortMallocNC(sizeof(struct uac_streaming));
    if (!stream) {
        uac_err("stream alloc fail\n");
        return NULL;
    }
    memset(stream, 0, sizeof(struct uac_streaming));

    stream->xSemaphore = xSemaphoreCreateBinary();
    if (stream->xSemaphore == NULL) {
        uac_err("create cmd semahpone fail\n");
        goto err2;
    }
    xSemaphoreGive(stream->xSemaphore);

    stream->ep_att = pvPortMallocNC(sizeof(struct ep_attr));
    if (!stream->ep_att) {
        uac_err("Not enough memory for uac ep att.\n");
        goto err2;;
    }
    memset(stream->ep_att, 0, sizeof(struct ep_attr));

    uac_queue[dir] = stream->urb_queue = xQueueCreate(MAX_URBS,
                                                      (sizeof(struct urb *)));
    if (!stream->urb_queue) {
        uac_err("alloc uac urb queue fail\n");
        goto err1;
        return NULL;
    }

    if (!dir) {
        ret = uac_init_stream_buffer(stream);
        if (ret)
            goto err1;
    }

    stream->urb_size = 0;
    stream->udev = udev;
    return stream;

err1:
    vPortFreeNC(stream->ep_att);
    stream->ep_att = NULL;
err2:
    vPortFreeNC(stream);
    stream = NULL;
    return NULL;
}

static void usb_uac_poll(usbdev_t *dev)
{
    return;
}

static void uac_free_control_res(struct uac_control_desc *desc)
{
    int i;

    desc->uac1_control_header = NULL;
    for (i = 0; i < ADC_MAX_TERMINAL; i ++) {
        desc->uac_input_terminal[i] = NULL;
        desc->uac_output_terminal[i] = NULL;
    }

    for (i = 0; i < ADC_MAX_SEL_UNIT; i ++)
        desc->uac_sel_unit[i] = NULL;
    for (i = 0; i < ADC_MAX_FEATURE_UNIT; i ++)
        desc->uac_fea_unit[i] = NULL;
    for (i = 0; i < ADC_MAX_MIX_UNIT; i ++)
        desc->uac_mixer_unit[i] = NULL;
}

static void uac_free_stream_res(struct uac_device *udev)
{
    struct uac_stream_desc *stream_desc;
    struct uac_streaming *streaming;
    int i = 0;

    for (i = 0; i < FUNC_NUM; i++) {
        streaming = udev->streaming[i];
        stream_desc = streaming[i].stream_desc;
        stream_desc->as_header = NULL;
        stream_desc->fmt = NULL;
        vPortFreeNC(streaming->ep_att);
        streaming->ep_att = NULL;
        vPortFreeNC(streaming->share_buf.buffer_base);
        streaming->share_buf.buffer_base = NULL;
        vPortFreeNC(udev->streaming[i]);
        udev->streaming[i] = NULL;
    }
}

static void usb_uac_destroy(usbdev_t *dev)
{
    struct uac_device *udev;

    if (!dev->data)
        return;
    udev = (struct uac_device *)(dev->data);

    uac_free_control_res(&udev->control_desc);
    uac_free_stream_res(udev);
    udev->dev = NULL;
    vPortFree(dev->data);
    dev->data = NULL;
    uac_udev = NULL;
    return;
}
static int uac_alloc_urb_buffers(struct uac_streaming *stream, int counts)
{
    unsigned int i, j;
    struct urb *urb;
    endpoint_t *ep = stream->ep;

    for (i = 0; i < stream->urb_num; ++i) {
        stream->urb_size = counts * stream->isoc_packet;
        stream->urb_buffer[i] = pvPortMallocNC(stream->urb_size);
        if (!stream->urb_buffer[i]) {
            uac_err("alloc urb buffer fail\n");
            return -1;
        }
    }

    for (i = 0; i < stream->urb_num; ++i) {
        urb = usb_alloc_urb(stream->isoc_packet);
        if (urb == NULL) {
            uac_err("alloc urb failed\n");
            return -1;
        }
        urb->in = ep->direction;
        urb->ep = ep;
        urb->transfer_dma = virt_to_phys(stream->urb_buffer[i]);
        urb->transfer_buffer_length = stream->urb_size;
        urb->buffer = stream->urb_buffer[i];
        urb->interval = ep->interval;
        urb->number_of_packets = stream->isoc_packet;

        urb->context = stream;
        urb->complete = uac_urb_complete;

        for (j = 0; j < stream->isoc_packet; ++j) {
            urb->iso_frame_desc[j].offset = j * counts;
            urb->iso_frame_desc[j].length = counts;
        }

        stream->urb[i] = urb;
    }

    return 0;
}

void uac_endpoint_start(struct uac_streaming *stream)
{
    int i;
    int ret = 0;
    endpoint_t ep = stream->stream_desc->ep;

    for (i = 0; i < stream->urb_num; ++i) {
        if (ep.direction == OUT) {
            if (!stream->pl_callback)
                ret = uac_prepare_slience_urb(stream->urb[i]);
            else
                ret = uac_prepare_playback_urb(stream->urb[i]);
            if (ret) {
                uac_err("endpoint start error!\n");
                return;
            }
        } else
            uac_submit_urb(stream, stream->urb[i]);
    }
}

void uac_playback_stop(struct uac_streaming *stream)
{
    stream->play_status = false;
}

void uac_playback_start(struct uac_streaming *stream, u8 mode)
{
    struct ep_attr *ep_att;
    int counts;
    int ifnum, altsetting;
    int ep_num = stream->ep->endpoint;
    struct uac_device *udev = stream->udev;

    uac_ep_para_setting(stream, mode);

    ep_att = stream->ep_att;
    ep_att->sample_accum = 0;
    stream->urb_num = ep_att->nurbs;
    stream->isoc_packet = ep_att->npackets;

    counts = ep_att->curpacksize;
    uac_alloc_urb_buffers(stream, counts);

    ifnum = stream->stream_desc->intface_num;
    altsetting = stream->stream_desc->alternatesetting;
    uac_set_sample_rate(udev, ep_att->rate, ep_num);
    set_interface(udev->dev, ifnum, altsetting);
    stream->file_flag = false;
    stream->play_status = true;

    uac_endpoint_start(stream);
    uac_crit("if = %d, alt = %d channels = %d, sample_rate = %d\n",
             ifnum, altsetting, ep_att->channels, ep_att->rate);
    uac_crit("urb_num = %d, iso_packet = %d, count = %d, rate = %d, ep = %d\n",
             stream->urb_num, stream->isoc_packet,
             counts, ep_att->rate, ep_num);
}

struct uac_device *get_uac_device(void)
{
    return uac_udev;
}

void usb_uac_init(usbdev_t *dev)
{
    struct uac_device *udev;
    int i;

    uac_debug("usb_uac_init start\n");
    dev->data = pvPortMalloc(sizeof(struct uac_device));
    if (!dev->data) {
        uac_err("Not enough memory for uac device.\n");
        return;
    }

    memset(dev->data, 0, sizeof(struct uac_device));
    udev = dev->data;
    udev->dev = dev;

    dev->destroy = usb_uac_destroy;
    dev->poll = usb_uac_poll;

    uac_parse_descriptors(dev);
    for (i = 0; i < FUNC_NUM; i++) {
        udev->streaming[i] = uac_stream_new(udev, i);
        if (!udev->streaming[i]) {
            uac_err("Not enough memory for stream.\n");
            goto err_exit;
        }
    }

    uac_stream_init(udev);
    xTaskCreate(uac_prepare_playback_task, "UAC_PLAYBACK",
                1000, (void *)100, 8, &udev->xPlayTaskHandle);
    xTaskCreate(uac_handle_capture_task, "UAC_CAPTURE",
                1000, (void *)100, 8, &udev->xCapTaskHandle);
    uac_crit("usb_uac_init done\n");
    uac_udev = dev->data;

    return;

err_exit:
    uac_free_control_res(&udev->control_desc);
    uac_free_stream_res(udev);
    vPortFree(dev->data);
    dev->data = NULL;
}

void uac_register_playback_callback(struct uac_streaming *stream,
                          void (*pl_callback)(struct uac_streaming *stream))
{
    if (!stream)
        return;

    stream->pl_callback = pl_callback;
}

void uac_register_cap_callback(struct uac_streaming *stream,
                          void (*cap_callback)(u8 *data, uint32_t len))
{
    if (!stream)
        return;

    stream->cap_callback = cap_callback;
}

void uac_register_stop_callback(struct uac_streaming *stream,
                          void (*stop_callback)(struct uac_streaming *stream))
{
    if (!stream)
        return;

    stream->stop_callback = stop_callback;
}
