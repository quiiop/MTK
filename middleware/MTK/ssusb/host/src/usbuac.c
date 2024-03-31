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
#include <uac.h>
#include <xhci_private.h>
#include <unistd.h>

extern struct uac_device *uac_udev;
static struct uac_streaming *cap_stream = NULL;
QueueHandle_t write_queue = NULL;
TaskHandle_t xWriteTaskHandle = NULL;

static int uac_open_test_file(struct uac_streaming *stream, int mode)
{
    FRESULT f_ret;
    int f_mode = 0;

    if (!mode)
        f_mode = FA_READ;
    else
        f_mode = FA_CREATE_ALWAYS | FA_WRITE;
    f_ret = f_open(&stream->fid, stream->file_name, f_mode);
    if (f_ret) {
        uac_err("f_open error: %d", f_ret);
        return -1;
    }
    return 0;
}

static void uac_play_stop(struct uac_streaming *stream)
{
    endpoint_t ep = stream->stream_desc->ep;

    if (ep.direction == OUT) {
        uac_crit("playback end\n");
        f_close(&stream->fid);
    } else {
        if (!stream->file_flag) {
            uac_crit("capture end\n");
            f_close(&stream->fid);
            stream->file_flag = true;
        }
    }
}

static void uac_playback_write_data(struct uac_streaming *stream)
{
    uint8_t *share_buf;
    uint32_t share_buf_len;
    UINT f_br;

    uac_debug("uac_playback_write_data!\n");
    if (f_eof(&stream->fid)) {
        stream->file_flag = true;
        uac_err("[EOF]write_pos = 0x%lx", stream->share_buf.write);
        return;
    } else {
        uac_semaphore_lock(stream);
        uac_get_pl_buffer_write_pos(stream, &share_buf, &share_buf_len);
        if (share_buf_len > 0) {
            f_read(&stream->fid, share_buf, share_buf_len, &f_br);
            uac_set_pl_buffer_write_done(stream, share_buf_len);
        } else
            uac_debug("don't need to read\n");
        uac_semaphore_unlock(stream);
    }
    uac_debug("uac_playback_write_data-done\n");
}

static void uac_playback_write_data_task(void *arg)
{
    struct uac_streaming *stream;

    do {
        if (pdPASS == xQueueReceive(write_queue,
                                    &stream, 5))
            uac_playback_write_data(stream);
    } while (1);
}

static void uac_capture_data(u8 *data, uint32_t len)
{
    UINT f_bw;
    struct uac_streaming *stream = cap_stream;

    f_write(&stream->fid, data, len, &f_bw);
    if ((uint32_t)f_bw != len)
        uac_err("capture_data: f_write error\n");
}

static int play_task_resource_init(void)
{
    if (write_queue) {
        vQueueDelete(write_queue);
        write_queue = NULL;
    }
    if (xWriteTaskHandle) {
        vTaskDelete(xWriteTaskHandle);
        xWriteTaskHandle = NULL;
    }

    write_queue = xQueueCreate(MAX_URBS,
                               (sizeof(struct uac_streaming *)));
    if (!write_queue) {
        uac_err("alloc uac queue fail\n");
        return -1;
    }
    xTaskCreate(uac_playback_write_data_task, "Wri",
                1000, (void *)100, 8, &xWriteTaskHandle);

    return 0;
}

uint8_t t_uac_playback(uint8_t len, char *param[])
{
    struct uac_device *udev;
    struct uac_streaming *stream;
    int ret, channel, sample;
    u8 mode = 0;

    if (len == 4) {
        channel = atoi(param[1]);
        sample = atoi(param[2]);
    } else if (len == 1) {
        if (cap_stream) {
            uac_crit("Stop capture\r\n");
            uac_playback_stop(cap_stream);
        }
        return 0;

    } else {
        uac_err("parameters error!!\n");
        return -1;
    }

    udev = uac_udev;
    mode = atoi(param[0]);
    stream = udev->streaming[mode];
    snprintf(stream->file_name, sizeof(stream->file_name), "SD:/%s", param[3]);
    uac_crit("file_name = %s, mode = %d, channel = %d, sample = %d\n",
             stream->file_name, mode, channel, sample);
    ret = uac_open_test_file(stream, mode);
    if (ret)
        return -1;

    switch (mode) {
        case PLAYBACK:
            uac_reinit_ring_buffer(stream);
            uac_init_ring_buffer_data(stream);
            if (play_task_resource_init())
                return -1;
            break;
        case CAPTURE:
            cap_stream = stream;
            uac_register_cap_callback(stream, uac_capture_data);
            break;
        default:
            return -1;
    }
    uac_register_stop_callback(stream, uac_play_stop);
    uac_set_channel(stream, channel);
    if (uac_sel_sample_rate(stream, sample))
        return -1;
    uac_playback_start(stream, mode);
    if (!mode)
        uac_register_playback_callback(stream, uac_playback_write_data);
    return 0;
}
