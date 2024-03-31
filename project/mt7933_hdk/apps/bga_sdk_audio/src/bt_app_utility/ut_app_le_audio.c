/* Copyright Statement:
 *
 * (C) 2022  MediaTek Inc. All rights reserved.
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

#include "ut_app.h"
#include <string.h>
#include "FreeRTOS.h"

#include "ui_shell_manager.h"
#include "ui_shell_message_queue.h"

#include "apps_events_event_group.h"
#include "apps_config_event_list.h"

#include "bt_sink_srv_common.h"
#include "bt_le_audio_sink.h"

#include "ble_csis.h"

#ifdef MTK_BLE_DM_SUPPORT
#include "bt_device_manager_le.h"
#endif /* #ifdef MTK_BLE_DM_SUPPORT */


//lea = le audio
#define CLI_HEAD_STR    "lea"


static void _ut_app_lea_print_help(void)
{
    printf("ble lea  -le audio cmds : Description\n"
           " start adv\n"
           " stop adv\n"
           " disconnect [type][addr]\n"
           " unpair [type][addr]\n"
           " reset paired list: reset bonded data base\n"
           " show paired list : show all bonded info\n"
           " show conn list   : show UMR connection info\n"
#ifdef MTK_BLE_DM_SUPPORT
           " show dm conn list: show connection info in DM\n"
#endif /* #ifdef MTK_BLE_DM_SUPPORT */
           " play  : play media\n"
           " pause : pause media\n"
           " ffwd  : fast forward\n"
           " frwd  : fast rewind\n"
           " fwd   : next track\n"
           " bwd   : previous track\n"
           " get sup op : get supported control opcodes\n"
           " get media state : get media state\n"
           " get media player name : get media player name\n"
           " get track title : get track title\n"
           " get track dur : get track duratio\n"
           " get track pos : get track position\n"
           " get vol info : get volume information\n"
           " set sirk [sirk value]: set SIRK (16bytes) into NVDM\n"

          );
}

static void _ut_app_lea_special_free(void *data)
{
    ui_shell_msg_t *msg = (ui_shell_msg_t *)data;
    BT_LOGE("APP", "lea special free, id %x", msg->msg_id);

    if (msg->msg_data == NULL)
        return;

    switch (msg->msg_id) {
        default:
            break;
    }
}

static void _ut_app_lea_key_action(apps_config_key_action_t action, void *data, size_t len)
{
    void *extra_data = NULL;

    if (data && len > 0) {
        extra_data = pvPortMalloc(len);
        if (extra_data) {
            memcpy(extra_data, data, len);
        } else {
            BT_LOGE("APP", "_ut_app_lea_key_action malloc fail");
            return;
        }
    }
    ui_shell_send_event(false, EVENT_PRIORITY_MIDDLE, EVENT_GROUP_UI_SHELL_KEY,
                        action, extra_data, len, _ut_app_lea_special_free, 0);
}

bt_status_t bt_app_lea_io_callback(void *input, void *output)
{
    const char *cmd = input;

    //if (!UT_APP_CMP(CLI_HEAD_STR))
    //    return BT_STATUS_SUCCESS;

    cmd += strlen(CLI_HEAD_STR) + 1; //+1 to shift space

    if (UT_APP_CMP("?")) {
        _ut_app_lea_print_help();
    } else if (UT_APP_CMP("start adv")) {
        _ut_app_lea_key_action(KEY_LE_AUDIO_START_ADV, NULL, 0);
    } else if (UT_APP_CMP("stop adv")) {
        _ut_app_lea_key_action(KEY_LE_AUDIO_STOP_ADV, NULL, 0);
    } else if (UT_APP_CMP("disconnect")) {
        //disconnect <type> <addr>
        bt_addr_t bt_addr;
        bt_addr.type = (bt_addr_type_t) strtoul(cmd + 11, NULL, 10);
        if (bt_addr.type > 0x03) {
            BT_LOGE("APP", "type range is 0~3");
            return BT_STATUS_FAIL;
        }
        if (bt_app_get_param_addr(&bt_addr.addr, "", cmd + 13) != BT_STATUS_SUCCESS)
            return BT_STATUS_FAIL;
        /*BT_LOGD("APP", "Input addr(%d)=0%x-%x-%x-%x-%x-%x", bt_addr.type,
                bt_addr.addr[5], bt_addr.addr[4], bt_addr.addr[3],
                bt_addr.addr[2], bt_addr.addr[1], bt_addr.addr[0]);*/
        _ut_app_lea_key_action(KEY_LE_AUDIO_DISCONNECT, &bt_addr, sizeof(bt_addr));
    } else if (UT_APP_CMP("unpair")) {
        //unpair <type> <addr>
        bt_addr_t bt_addr;
        bt_addr.type = (bt_addr_type_t) strtoul(cmd + 7, NULL, 10);
        if (bt_addr.type > 0x03) {
            BT_LOGE("APP", "type range is 0~3");
            return BT_STATUS_FAIL;
        }
        if (bt_app_get_param_addr(&bt_addr.addr, "", cmd + 9) != BT_STATUS_SUCCESS)
            return BT_STATUS_FAIL;
        _ut_app_lea_key_action(KEY_LE_AUDIO_UNPAIR, &bt_addr, sizeof(bt_addr));
    } else if (UT_APP_CMP("reset paired list")) {
        _ut_app_lea_key_action(KEY_LE_AUDIO_RESET_PAIRED_LIST, NULL, 0);
    } else if (UT_APP_CMP("show paired list")) {
        _ut_app_lea_key_action(KEY_LE_AUDIO_PAIRED_LIST, NULL, 0);
    } else if (UT_APP_CMP("show conn list")) {
        // show connetion info which belong to UMR link
        _ut_app_lea_key_action(KEY_LE_AUDIO_CONN_LIST, NULL, 0);
#ifdef MTK_BLE_DM_SUPPORT
    } else if (UT_APP_CMP("show dm conn list")) {
        // A debug API to show all le connection info in DM, and not just for lea link
        bt_device_manager_le_show_conn_info();
#endif /* #ifdef MTK_BLE_DM_SUPPORT */
    } else if (UT_APP_CMP("play")) {
        _ut_app_lea_key_action(KEY_LE_MCP_PLAY, NULL, 0);
    } else if (UT_APP_CMP("pause")) {
        _ut_app_lea_key_action(KEY_LE_MCP_PAUSE, NULL, 0);
    } else if (UT_APP_CMP("ffwd")) {
        _ut_app_lea_key_action(KEY_LE_MCP_FAST_FORWARD, NULL, 0);
    } else if (UT_APP_CMP("frwd")) {
        _ut_app_lea_key_action(KEY_LE_MCP_FAST_REWIND, NULL, 0);
    } else if (UT_APP_CMP("fwd")) {
        _ut_app_lea_key_action(KEY_LE_MCP_NEXT_TRACK, NULL, 0);
    } else if (UT_APP_CMP("bwd")) {
        _ut_app_lea_key_action(KEY_LE_MCP_PRE_TRACK, NULL, 0);
    } else if (UT_APP_CMP("get sup op")) {
        _ut_app_lea_key_action(KEY_LE_MCP_GET_SUPPORTED_CTRL_OPCODE, NULL, 0);
    } else if (UT_APP_CMP("get media state")) {
        _ut_app_lea_key_action(KEY_LE_MCP_GET_MEDIA_STATE, NULL, 0);
    } else if (UT_APP_CMP("get media player name")) {
        _ut_app_lea_key_action(KEY_LE_MCP_GET_MEDIA_PLAYER_NAME, NULL, 0);

    } else if (UT_APP_CMP("get track title")) {
        _ut_app_lea_key_action(KEY_LE_MCP_GET_TRACK_TITLE, NULL, 0);
    } else if (UT_APP_CMP("get track dur")) {
        _ut_app_lea_key_action(KEY_LE_MCP_GET_TRACK_DURATION, NULL, 0);
    } else if (UT_APP_CMP("get track pos")) {
        _ut_app_lea_key_action(KEY_LE_MCP_GET_TRACK_POSITION, NULL, 0);
    } else if (UT_APP_CMP("get vol info")) {
        _ut_app_lea_key_action(KEY_LE_VCP_GET_VOLUME_INFO, NULL, 0);
    } else if (UT_APP_CMP("set sirk")) {
        bt_key_t key = {0};
        const char *str = NULL;
        uint8_t i = 0;
        unsigned int value = 0;
        int32_t ret = 0;

        if (strlen(cmd) < BT_KEY_SIZE * 2 + 9) {
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGW("APP", "set sirk [value]: value has 16 bytes");
            BT_COLOR_SET(BT_COLOR_WHITE);
            return BT_STATUS_FAIL;
        }

        str = (const char *)(cmd + 9);
        for (i = 0; i < BT_KEY_SIZE; i++) {
            ret = sscanf(str + i * 2, "%02x", &value);
            if (ret < 0) {
                BT_LOGE("APP", "set sirk FAIL!");
                return BT_STATUS_FAIL;
            }
            key[BT_KEY_SIZE - 1 - i] = (uint8_t)value;
        }
        BT_LOGI("APP", "SIRK: 0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                key[15], key[14], key[13], key[12], key[11], key[10], key[9], key[8],
                key[7], key[6], key[5], key[4], key[3], key[2], key[1], key[0]);

        ble_csis_write_nvkey_sirk(&key);

        /*can directly send to le MW*/
    } else if (UT_APP_CMP("le play")) {
        //a test code to call LE MW action but not sink action
        bt_handle_t handle;
        bt_le_audio_sink_action_param_t le_param = {
            .service_idx = 0xFF,
        };
        handle = (bt_handle_t)strtoul(cmd + 5, NULL, 16);
        bt_le_audio_sink_send_action(handle, BT_LE_AUDIO_SINK_ACTION_MEDIA_PLAY, &le_param);
    } else if (UT_APP_CMP("le pause")) {
        //a test code to call LE MW action but not sink action
        bt_handle_t handle;
        bt_le_audio_sink_action_param_t le_param = {
            .service_idx = 0xFF,
        };
        handle = (bt_handle_t)strtoul(cmd + 5, NULL, 16);
        bt_le_audio_sink_send_action(handle, BT_LE_AUDIO_SINK_ACTION_MEDIA_PAUSE, &le_param);
        /*(end) can directly send to le MW*/
    } else {
        BT_LOGI("APP", "not lea cli!!");
    }

    return BT_STATUS_SUCCESS;
}


