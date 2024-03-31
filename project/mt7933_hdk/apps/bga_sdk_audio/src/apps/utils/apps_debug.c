/* Copyright Statement:
 *
 * (C) 2005-2021  MediaTek Inc. All rights reserved.
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

#include "apps_debug.h"

log_create_module(apps, PRINT_LEVEL_DEBUG);

#ifdef MTK_BT_AUDIO_PR
#include "task.h"
#include <string.h>

#define MIN(A,B) { if(A > B) A = B; }
#define MAX(A,B) { if(A < B) A = B; }

static struct app_bt_dbg_audio_decode_path_record *gp_path_record = NULL;

void app_bt_dbg_audio_pr_init(void)
{
    if (!gp_path_record)
        gp_path_record = (struct app_bt_dbg_audio_decode_path_record *)pvPortMalloc(sizeof(struct app_bt_dbg_audio_decode_path_record));

    if (!gp_path_record) {
        LOG_E(apps, "Malloc path record fail!");
        return;
    }

    memset(gp_path_record, 0, sizeof(struct app_bt_dbg_audio_decode_path_record));
    gp_path_record->trigger = false;
}

void app_bt_dbg_audio_pr_deinit(void)
{
    if (gp_path_record) {
        vPortFree(gp_path_record);
        gp_path_record = NULL;
    }
}

void app_bt_dbg_audio_pr_write(unsigned char *p_in, unsigned int shift, unsigned int location)
{
    unsigned char *p_data;
    unsigned short sbcFNo;
    struct app_bt_dbg_audio_decode_path_packet *p_pkt = NULL;
    TickType_t cur_tick = xTaskGetTickCount();
    unsigned int i, index;

    if (!gp_path_record || !p_in) //Did not initialized
        return;

    p_data = p_in + shift;

    if (gp_path_record->trigger == true)
        return;

    /* if (location == BT_AUDIO_DECODE_PATH_LOCATION_BTTASK)
        LOG_I(apps, "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x",
              p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5], p_data[6], p_data[7]);

    LOG_I(apps, "[%d] SBC Frame No %d", gp_path_record->index, sbcFNo);*/

    switch (location) {
        case APP_BT_DBG_AUDIO_DECODE_PATH_RX: {
                if (!(p_data[0] == 0x80 && p_data[1] == 0x60)) //Not SBC packet
                    return;
                sbcFNo = p_data[2] << 8 | p_data[3];
                p_pkt = &(gp_path_record->record[gp_path_record->index[location]]);
                memset(p_pkt, 0, sizeof(struct app_bt_dbg_audio_decode_path_packet));
                p_pkt->sbc_frame_no = sbcFNo;
                p_pkt->ticktime[location] = cur_tick;
                gp_path_record->index[location] ++;
                if (gp_path_record->index[location] == APP_BT_DBG_AUDIO_PATH_RECORD_COUNT)
                    gp_path_record->index[location] = 0;
                return;
            }
        case APP_BT_DBG_AUDIO_DECODE_PATH_BTTASK:
        case APP_BT_DBG_AUDIO_DECODE_PATH_DECODE_BEFORE: {
                if (!(p_data[0] == 0x80 && p_data[1] == 0x60)) /* Not SBC packet */
                    return;
                sbcFNo = p_data[2] << 8 | p_data[3];
                /* Find corresponding record */
                for (i = 0; i < APP_BT_DBG_AUDIO_PATH_RECORD_COUNT; i++) {
                    if (gp_path_record->index[location] < i)
                        index = APP_BT_DBG_AUDIO_PATH_RECORD_COUNT + gp_path_record->index[location] - i;
                    else
                        index = gp_path_record->index[location] - i;

                    p_pkt = &gp_path_record->record[index];
                    if (p_pkt->sbc_frame_no == sbcFNo)
                        break;
                }

                if (index != gp_path_record->index[location]) {
                    /* LOG_E(app, "May have bug? %d != %d", index, gp_path_record->index[location]); */
                    gp_path_record->index[location] = index;
                }
                break;
            }
        case APP_BT_DBG_AUDIO_DECODE_PATH_DECODE_AFTER:
        case APP_BT_DBG_AUDIO_DECODE_PATH_PLAY: {
                p_pkt = &gp_path_record->record[gp_path_record->index[location]];
                break;
            }
        default: {
                break;
            }
    }

    /* Check previous location record */
    for (i = 0; i < location; i++) {
        if (p_pkt->ticktime[i] == 0)
            break;
    }

    /* Write tick time */
    if (i == location) {
        p_pkt->ticktime[location] = cur_tick;
        gp_path_record->index[location] ++;
        if (gp_path_record->index[location] == APP_BT_DBG_AUDIO_PATH_RECORD_COUNT)
            gp_path_record->index[location] = 0;
    }
}

void app_bt_dbg_audio_pr_dump(void)
{
    struct app_bt_dbg_audio_decode_path_packet *p_pkt = NULL;
    struct app_bt_dbg_audio_decode_path_packet *p_pre_pkt = NULL;
    int i, j;
    unsigned int index;
    TickType_t total;
    TickType_t min_total = 9999;
    TickType_t max_total = 0;
    TickType_t aver_total = 0;
    TickType_t delta[APP_BT_DBG_AUDIO_DECODE_PATH_MAX - 1];
    TickType_t min_delta[APP_BT_DBG_AUDIO_DECODE_PATH_MAX - 1] = {999, 999, 999, 999};
    TickType_t max_delta[APP_BT_DBG_AUDIO_DECODE_PATH_MAX - 1] = {0};
    double delta_percent[APP_BT_DBG_AUDIO_DECODE_PATH_MAX - 1] = {0};
    double min_delta_percent[APP_BT_DBG_AUDIO_DECODE_PATH_MAX - 1] = {999, 999, 999, 999};
    double max_delta_percent[APP_BT_DBG_AUDIO_DECODE_PATH_MAX - 1] = {0};
    TickType_t aver_delta[APP_BT_DBG_AUDIO_DECODE_PATH_MAX - 1] = {0};
    TickType_t min_without_decode = 999;
    TickType_t max_without_decode = 0;

    if (!gp_path_record) {/* Did not initialized */
        LOG_E(apps, "path record did not initialized!");
        return;
    }

    taskENTER_CRITICAL();
    gp_path_record->trigger = true;
    taskEXIT_CRITICAL();

    index = gp_path_record->index[0];
    LOG_I(apps, "======================================== DECODE PATH ANALYSIS ========================================");
    LOG_I(apps, "[idx][Frame No][Rx -> BT Task -> Decode(B) -> Decode(A) -> Play][Total]");
    for (i = 0; i < APP_BT_DBG_AUDIO_PATH_RECORD_COUNT; i++) {
        p_pre_pkt = p_pkt;
        p_pkt = &gp_path_record->record[index];

        if (!p_pkt->ticktime[APP_BT_DBG_AUDIO_DECODE_PATH_MAX - 1])
            break;

        total = p_pkt->ticktime[4] - p_pkt->ticktime[0];
        aver_total += total;

        MIN(min_total, total);
        MAX(max_total, total);

        for (j = 0; j < APP_BT_DBG_AUDIO_DECODE_PATH_MAX - 1; j++) {
            if (!p_pkt->ticktime[j + 1] || !p_pkt->ticktime[j])
                break;

            delta[j] = p_pkt->ticktime[j + 1] - p_pkt->ticktime[j];
            delta_percent[j] = (double) delta[j] * 100 / total;

            MIN(min_delta[j], delta[j]);
            MAX(max_delta[j], delta[j]);
            MIN(min_delta_percent[j], delta_percent[j]);
            MAX(max_delta_percent[j], delta_percent[j]);

            if (j == 1) {
                MIN(min_without_decode, total - delta[j]);
                MAX(max_without_decode, total - delta[j]);
            }

            aver_delta[j] += delta[j];
        }

        LOG_I(apps, "[%03d][%d][%d {%02d} -> %d (%02d,%5.2f%%) -> %d (%02d,%5.2f%%) -> %d (%02d,%5.2f%%) -> %d (%02d,%5.2f%%)][%02d]",
              index, p_pkt->sbc_frame_no, p_pkt->ticktime[0],
              index == gp_path_record->index[0] ? 0 : p_pkt->ticktime[0] - p_pre_pkt->ticktime[0],
              p_pkt->ticktime[1], delta[0], delta_percent[0],
              p_pkt->ticktime[2], delta[1], delta_percent[1],
              p_pkt->ticktime[3], delta[2], delta_percent[2],
              p_pkt->ticktime[4], delta[3], delta_percent[3],
              total);
        index ++;
        if (index == APP_BT_DBG_AUDIO_PATH_RECORD_COUNT)
            index = 0;

        if (i % 8 == 7)
            vTaskDelay(10);
    }
    LOG_I(apps, "Average                  %5.2f%% (%5.2f%%~%5.2f%%) -> %5.2f%% (%5.2f%%~%5.2f%%)\
          -> %5.2f%% (%5.2f%%~%5.2f%%) -> %5.2f%% (%5.2f%%~%5.2f%%)",
          (double) aver_delta[0] * 100 / aver_total, min_delta_percent[0], max_delta_percent[0],
          (double) aver_delta[1] * 100 / aver_total, min_delta_percent[1], max_delta_percent[1],
          (double) aver_delta[2] * 100 / aver_total, min_delta_percent[2], max_delta_percent[2],
          (double) aver_delta[3] * 100 / aver_total, min_delta_percent[3], max_delta_percent[3]);
    LOG_I(apps, "Push to Queue  %5.2f (%02d~%02d)", (double) aver_delta[0] / i, min_delta[0], max_delta[0]);
    LOG_I(apps, "Decode         %5.2f (%02d~%02d)", (double) aver_delta[1] / i, min_delta[1], max_delta[1]);
    LOG_I(apps, "Without Decode %5.2f (%02d~%02d)", (double)(aver_total - aver_delta[1]) / i, min_without_decode, max_without_decode);
    LOG_I(apps, "Total          %5.2f (%02d~%02d)", (double) aver_total / i, min_total, max_total);
    LOG_I(apps, "======================================================================================================");

    taskENTER_CRITICAL();
    memset(gp_path_record, 0, sizeof(struct app_bt_dbg_audio_decode_path_record));
    gp_path_record->trigger = false;
    taskEXIT_CRITICAL();
}

#endif /* #ifdef MTK_BT_AUDIO_PR */
