/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "mp3_codec.h"
#include "hal_log.h"
#include "hal_gpt.h"
#include "ff.h"
#include "cli.h"

static FATFS fatfs;
static FIL fdst;
static FRESULT res;
static UINT length_read;
static uint8_t first_time_f_mount = 1;
mp3_codec_media_handle_t *g_hdl = NULL;

typedef enum {
    PLAY_FROM_NONE = 0,
    PLAY_FROM_USB  = 1,
    PLAY_FROM_SD   = 2,
} play_from_t;
static uint8_t play_from = PLAY_FROM_NONE;

static int16_t test_open_file(FIL *fp, const TCHAR *path)
{
    FRESULT res = FR_OK;
    if (first_time_f_mount) {
        if (play_from == PLAY_FROM_USB) {
            res = f_mount(&fatfs, _T("USB:"), 1);
        } else if (play_from == PLAY_FROM_SD) {
            res = f_mount(&fatfs, _T("SD:"), 1);
        }
        first_time_f_mount = 0;
    } else {
        printf("[MP3 Codec Demo] already fmount\n");
        res = FR_OK;
    }
    if (!res) {
        printf("[MP3 Codec Demo] fmount ok \n");
        res = f_open(fp, path, FA_OPEN_EXISTING | FA_WRITE | FA_READ);
        if (!res) {
            printf("[MP3 Codec Demo] fopen ok \n");
        } else {
            printf("[MP3 Codec Demo] fopen error \n");
            return -1;
        }
    } else {
        printf("[MP3 Codec Demo] fmount error \n");
    }

    if (res != FR_OK) {
        return -1;
    } else {
        return 0;
    }
}

static void test_stop_read(mp3_codec_media_handle_t *hdl)
{
    hdl->stop(hdl);
    res = f_close(&fdst);
    if (!res) {
        printf("[MP3 Codec Demo] fclose success \n");
    } else {
        printf("[MP3 Codec Demo] fclose error \n");
    }
    hdl->close_codec(hdl);
    printf("[MP3 Codec Demo] close codec() - \n");
}


static void test_event_callback(mp3_codec_media_handle_t *hdl, mp3_codec_event_t event)
{
    uint8_t *share_buf;
    uint32_t share_buf_len;
    FRESULT res;

    switch (event) {
        case MP3_CODEC_MEDIA_JUMP_FILE_TO:
            res = f_lseek(&fdst, (DWORD)hdl->jump_file_to_specified_position);
            break;
        case MP3_CODEC_MEDIA_REQUEST:
        case MP3_CODEC_MEDIA_UNDERFLOW:

            if (f_eof(&fdst)) {
                printf("[MP3 Codec Demo] [EOF]End of this mp3 file. Stop mp3\n");
                test_stop_read(hdl);
                g_hdl = NULL;
                break;
            }

            for (uint32_t loop_idx = 0; loop_idx < 2; loop_idx++) {
                hdl->get_write_buffer(hdl, &share_buf, &share_buf_len);
                res = f_read(&fdst, share_buf, share_buf_len, &length_read);
                if (res) {
                    log_hal_error("[MP3 Codec Demo] f_read fail \r\n");
                    return;
                }
                hdl->write_data_done(hdl, share_buf_len);
            }
            hdl->finish_write_data(hdl);
            break;
    }
}

static uint8_t mp3_codec_demo(uint8_t len, char *param[])
{
    uint8_t *share_buf;
    uint32_t share_buf_len;

    // Open the mp3 codec.
    printf("[MP3 Codec Demo] open mp3 codec\n");

    if (strncmp(param[0], "USB", 3) == 0) {
        play_from = PLAY_FROM_USB;
        printf("[MP3 Codec Demo] play from USB\n");
    } else if (strncmp(param[0], "SD", 2) == 0) {
        play_from = PLAY_FROM_SD;
        printf("[MP3 Codec Demo] play from SD\n");
    } else {
        play_from = PLAY_FROM_NONE;
        printf("[MP3 Codec Demo] unexpected play storage\n");
    }

    g_hdl = mp3_codec_open(test_event_callback);
    if (g_hdl == NULL) {
        log_hal_error("[MP3 Codec Demo] Fail to open the codec.");
        return 0;
    }

    mp3_codec_set_memory2();

    if (test_open_file(&fdst, _T(param[0])) < 0) {
        log_hal_error("[MP3 Codec Demo] Fail to open %s.\n", param[0]);
        return 0;
    }

    /* prefill data to share buffer */
    g_hdl->get_write_buffer(g_hdl, &share_buf, &share_buf_len);
    f_read(&fdst, share_buf, share_buf_len, &length_read);

    g_hdl->write_data_done(g_hdl, share_buf_len);
    g_hdl->finish_write_data(g_hdl);
    g_hdl->skip_id3v2_and_reach_next_frame(g_hdl, (uint32_t)f_size(&fdst));

    printf("[MP3 Codec Demo] play +\r\n");
    g_hdl->play(g_hdl);
    printf("[MP3 Codec Demo] play -\r\n");

    //printf("[MP3 Codec Demo] vTaskDelay(5000 / portTICK_RATE_MS); +\r\n");
    //vTaskDelay(5000 / portTICK_RATE_MS);
    //printf("[MP3 Codec Demo] vTaskDelay(5000 / portTICK_RATE_MS); -\r\n");

    //printf("[MP3 Codec Demo] pause +\r\n");
    //g_hdl->pause(g_hdl);
    //printf("[MP3 Codec Demo] pause -\r\n");

    //printf("[MP3 Codec Demo] hal_gpt_delay_ms(3000); +\r\n");
    //hal_gpt_delay_ms(3000);
    //printf("[MP3 Codec Demo] hal_gpt_delay_ms(3000); -\r\n");


    //printf("[MP3 Codec Demo] resume() +\r\n");
    //g_hdl->resume(g_hdl);
    //printf("[MP3 Codec Demo] resume() -\r\n");

    return 0;
}

static uint8_t mp3_codec_pause(uint8_t len, char *param[])
{
    printf("[MP3 Codec Demo] pause +\r\n");
    if (g_hdl) {
        g_hdl->pause(g_hdl);
    } else {
        printf("%s: g_hdl is NULL, invalid.\r\n", __func__);
    }
    printf("[MP3 Codec Demo] pause -\r\n");

    return 0;
}

static uint8_t mp3_codec_resume(uint8_t len, char *param[])
{
    printf("[MP3 Codec Demo] resume() +\r\n");
    if (g_hdl) {
        g_hdl->resume(g_hdl);
    } else {
        printf("%s: g_hdl is NULL, invalid.\r\n", __func__);
    }
    printf("[MP3 Codec Demo] resume() -\r\n");

    return 0;
}

cmd_t mp3_cli[] = {
    { "play",  "play",     mp3_codec_demo,   NULL },
    { "pause",  "pause",   mp3_codec_pause,  NULL },
    { "resume",  "resume", mp3_codec_resume, NULL },
    { NULL }
};
