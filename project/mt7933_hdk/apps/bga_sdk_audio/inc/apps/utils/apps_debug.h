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

#ifndef __APPS_DEBUG_H__
#define __APPS_DEBUG_H__

#include "syslog.h"

#define APPS_LOG_E(msg, ...)                    LOG_E(apps, (msg), ##__VA_ARGS__)
#define APPS_LOG_W(msg, ...)                    LOG_W(apps, (msg), ##__VA_ARGS__)
#define APPS_LOG_I(msg, ...)                    LOG_I(apps, (msg), ##__VA_ARGS__)
#define APPS_LOG_D(msg, ...)                    LOG_D(apps, (msg), ##__VA_ARGS__)

#if 1// 7933
#define APPS_LOG_MSGID_E(msg, arg_cnt, ...)     LOG_E(apps, msg, ##__VA_ARGS__)
#define APPS_LOG_MSGID_W(msg, arg_cnt, ...)     LOG_W(apps, msg, ##__VA_ARGS__)
#define APPS_LOG_MSGID_I(msg, arg_cnt, ...)     LOG_I(apps, msg, ##__VA_ARGS__)
#define APPS_LOG_MSGID_D(msg, arg_cnt, ...)     LOG_D(apps, msg, ##__VA_ARGS__)

#define APPS_LOG_DUMP_E(msg, buffer, len, ...)
#define APPS_LOG_DUMP_W(msg, buffer, len, ...)
#define APPS_LOG_DUMP_I(msg, buffer, len, ...)
#define APPS_LOG_DUMP_D(msg, buffer, len, ...)
#else /* #if 1// 7933 */
#define APPS_LOG_MSGID_E(msg, ...)              LOG_MSGID_E(apps, msg, ##__VA_ARGS__)
#define APPS_LOG_MSGID_W(msg, ...)              LOG_MSGID_W(apps, msg, ##__VA_ARGS__)
#define APPS_LOG_MSGID_I(msg, ...)              LOG_MSGID_I(apps, msg, ##__VA_ARGS__)
#define APPS_LOG_MSGID_D(msg, ...)              LOG_MSGID_D(apps, msg, ##__VA_ARGS__)

#define APPS_LOG_DUMP_E(msg, buffer, len, ...)  LOG_HEXDUMP_E(apps, (msg), (buffer), (len), ##__VA_ARGS__)
#define APPS_LOG_DUMP_W(msg, buffer, len, ...)  LOG_HEXDUMP_W(apps, (msg), (buffer), (len), ##__VA_ARGS__)
#define APPS_LOG_DUMP_I(msg, buffer, len, ...)  LOG_HEXDUMP_I(apps, (msg), (buffer), (len), ##__VA_ARGS__)
#define APPS_LOG_DUMP_D(msg, buffer, len, ...)  LOG_HEXDUMP_D(apps, (msg), (buffer), (len), ##__VA_ARGS__)
#endif /* #if 1// 7933 */

#ifdef MTK_BT_AUDIO_PR
/* Provide debug APIs to log SBC decode path to check data latency*/
#include "FreeRTOS.h"

#define APP_BT_DBG_AUDIO_DECODE_PATH_RX            0x0
#define APP_BT_DBG_AUDIO_DECODE_PATH_BTTASK        0x1
#define APP_BT_DBG_AUDIO_DECODE_PATH_DECODE_BEFORE 0x2
#define APP_BT_DBG_AUDIO_DECODE_PATH_DECODE_AFTER  0x3
#define APP_BT_DBG_AUDIO_DECODE_PATH_PLAY          0x4
#define APP_BT_DBG_AUDIO_DECODE_PATH_MAX           0x5

#define APP_BT_DBG_AUDIO_PATH_RECORD_COUNT 64

struct app_bt_dbg_audio_decode_path_packet {
    unsigned short sbc_frame_no;
    TickType_t ticktime[APP_BT_DBG_AUDIO_DECODE_PATH_MAX];
};

struct app_bt_dbg_audio_decode_path_record {
    unsigned int index[APP_BT_DBG_AUDIO_PATH_RECORD_COUNT];
    struct app_bt_dbg_audio_decode_path_packet record[APP_BT_DBG_AUDIO_PATH_RECORD_COUNT];
    bool trigger;
};

void app_bt_dbg_audio_pr_init(void);
void app_bt_dbg_audio_pr_deinit(void);
void app_bt_dbg_audio_pr_write(unsigned char *p_in, unsigned int shift, unsigned int location);
void app_bt_dbg_audio_pr_dump(void);

#endif /* #ifdef MTK_BT_AUDIO_PR */

#endif /* #ifndef __APPS_DEBUG_H__ */
