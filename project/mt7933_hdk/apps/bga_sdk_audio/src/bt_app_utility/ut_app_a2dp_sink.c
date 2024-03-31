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

#include "ut_app.h"
#include <string.h>
#include "FreeRTOS.h"

#include "ui_shell_manager.h"
#include "ui_shell_message_queue.h"

#include "apps_events_event_group.h"
#include "apps_config_event_list.h"

#include "bt_sink_srv_common.h"

#define CLI_HEAD_STR    "sink "


static void _ut_app_sink_print_help(void)
{
    printf("ble sink  -Sink cmds : Description\n"
           " po  : power on\n"
           " pf  : power off\n"
#ifdef MTK_BT_DUO_ENABLE
           " discoverable          : discoverable by peer\n"
           " resetpairinfo         : delete all pair infomation\n"
           " deletepairinfo [addr] : delete pair infomation of addr\n"
           " pairinfo              : List all pair infomation\n"
           " connect [addr]        : connect to addr\n"
           " disconnect [addr]     : disconnect to addr\n"
           " lastdevice            : connect to last connected device\n"
           " setrole : Set role for current ACL Link (only for test)\n"
           "           0 : Master  1 : Slave\n"
           " getrole : Get role for current ACL Link (only for test)\n"
           " showstatus : print the current Sink stutus\n"
           "              (connected device, connected profile, role, etc)\n"
#ifdef MTK_BT_AVRCP_ENABLE
           " play  : play music\n"
           " pause : pause music\n"
           " fwd   : Forward\n"
           " bwd   : Backward\n"
           " ffwd  : fast forward (0:press 1:release)\n"
           " rwd   : rewind (0:press 1:release)\n"
           " gea   : get element attribute\n"
           " gps   : get play status\n"
           " getc  : get attr (Only get Evt supported(Id:3))\n"
           " regevt [event] : register event notification\n"
           "                 1 : EVENT_PLAYBACK_STATUS_CHANGED\n"
           "                 2 : EVENT_TRACK_CHANGED\n"
           "                 3 : EVENT_TRACK_REACHED_END\n"
           "                 4 : EVENT_TRACK_REACHED_START\n"
           "                 5 : EVENT_PLAYBACK_POS_CHANGED\n"
           "                 6 : EVENT_BATT_STATUS_CHANGED\n"
           "                 7 : EVENT_SYSTEM_STATUS_CHANGED\n"
           "                 8 : EVENT_PLAYER_APP_SETTING_CHANGED\n"
           "                 9 : EVENT_NOW_PLAYING_CONTENT_CHANGED\n"
           "                10 : EVENT_AVAILABLE_PLAYERS_CHANGED\n"
           "                11 : EVENT_ADDRESSED_PLAYER_CHANGED\n"
           "                12 : EVENT_UIDS_CHANGED\n"
           "                13 : EVENT_VOLUME_CHANGED\n"
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */
#ifdef MTK_BT_A2DP_ENABLE
           " sbcmaxbitpool : Set SBC max bit pool size (only for test)\n"
#ifdef MTK_BT_AUDIO_PR
           " audiopr : Dump decode path record (only for test)\n"
#endif /* #ifdef MTK_BT_AUDIO_PR */
#endif /* #ifdef MTK_BT_A2DP_ENABLE */
#endif /* #ifdef MTK_BT_DUO_ENABLE */
          );
}

static void _ut_app_sink_special_free(void *data)
{
    ui_shell_msg_t *msg = (ui_shell_msg_t *)data;
    BT_LOGE("APP", "sink special free, id %x", msg->msg_id);

    if (msg->msg_data == NULL)
        return;

    switch (msg->msg_id) {
#ifdef MTK_BT_AVRCP_ENABLE
        case KEY_AVRCP_GET_ATTRIBUTE: {
                bt_avrcp_get_element_attributes_t *attribute_list =
                    ((bt_sink_srv_avrcp_get_element_attributes_parameter_t *)(msg->msg_data))->attribute_list;
                if (attribute_list)
                    vPortFree(attribute_list);
            }
            break;
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */
        default:
            break;
    }
}

static void _ut_app_sink_key_action(apps_config_key_action_t action, void *data, size_t len)
{
    void *extra_data = NULL;

    if (data && len > 0) {
        extra_data = pvPortMalloc(len);
        if (extra_data) {
            memcpy(extra_data, data, len);
        } else {
            BT_LOGE("APP", "_ut_app_sink_key_action malloc fail");
            return;
        }
    }
    ui_shell_send_event(false, EVENT_PRIORITY_MIDDLE, EVENT_GROUP_UI_SHELL_KEY,
                        action, extra_data, len, _ut_app_sink_special_free, 0);
}

bt_status_t bt_app_sink_io_callback(void *input, void *output)
{
    const char *cmd = input;

    if (!UT_APP_CMP(CLI_HEAD_STR))
        return BT_STATUS_SUCCESS;

    cmd += strlen(CLI_HEAD_STR);

    if (UT_APP_CMP("?")) {
        _ut_app_sink_print_help();
    } else if (UT_APP_CMP("po")) {
        _ut_app_sink_key_action(KEY_POWER_ON, NULL, 0);
    } else if (UT_APP_CMP("pf")) {
        _ut_app_sink_key_action(KEY_POWER_OFF, NULL, 0);
#ifdef MTK_BT_DUO_ENABLE
    } else if (UT_APP_CMP("discoverable")) {
        _ut_app_sink_key_action(KEY_DISCOVERABLE, NULL, 0); //default timeout setting is 2min
    } else if (UT_APP_CMP("hidden")) {
        _ut_app_sink_key_action(KEY_HIDDEN, NULL, 0);
    } else if (UT_APP_CMP("resetpairinfo")) {
        _ut_app_sink_key_action(KEY_RESET_PAIRED_DEVICES, NULL, 0);
    } else if (UT_APP_CMP("deletepairinfo")) {
        bt_bd_addr_t addr;
        if (bt_app_get_param_addr(&addr, "deletepairinfo ", cmd) == BT_STATUS_FAIL)
            return BT_STATUS_FAIL;
        _ut_app_sink_key_action(KEY_DELETE_PAIRED_DEVICES, &addr, BT_BD_ADDR_LEN);
    } else if (UT_APP_CMP("pairinfo")) {
        _ut_app_sink_key_action(KEY_PAIRED_DEVICES_LIST, NULL, 0);
    } else if (UT_APP_CMP("connect")) {
        bt_bd_addr_t addr;
        if (bt_app_get_param_addr(&addr, "connect ", cmd) == BT_STATUS_FAIL)
            return BT_STATUS_FAIL;
        _ut_app_sink_key_action(KEY_SINK_CONNECT, &addr, BT_BD_ADDR_LEN);
    } else if (UT_APP_CMP("disconnect")) {
        bt_bd_addr_t addr;
        if (bt_app_get_param_addr(&addr, "disconnect ", cmd) == BT_STATUS_FAIL)
            return BT_STATUS_FAIL;
        _ut_app_sink_key_action(KEY_SINK_DISCONN, &addr, BT_BD_ADDR_LEN);
    } else if (UT_APP_CMP("lastdevice")) {
        _ut_app_sink_key_action(KEY_RECONNECT_LAST_DEVICE, NULL, 0);
    } else if (UT_APP_CMP("showstatus")) {
        _ut_app_sink_key_action(KEY_SHOW_STATUS, NULL, 0);
    } else if (UT_APP_CMP("setrole")) {
        uint32_t role = (uint32_t)((*(cmd + 8)) - '0');
        _ut_app_sink_key_action(KEY_ROLE_SWITCH, (void *)&role, sizeof(uint32_t));
    } else if (UT_APP_CMP("getrole")) {
        _ut_app_sink_key_action(KEY_GET_ROLE, NULL, 0);
#ifdef MTK_BT_AVRCP_ENABLE
        /* Musc Control Cli */
    } else if (UT_APP_CMP("play")) {
        _ut_app_sink_key_action(KEY_AVRCP_PLAY, NULL, 0);
    } else if (UT_APP_CMP("pause")) {
        _ut_app_sink_key_action(KEY_AVRCP_PAUSE, NULL, 0);
    } else if (UT_APP_CMP("fwd")) { //next track
        _ut_app_sink_key_action(KEY_AVRCP_FORWARD, NULL, 0);
    } else if (UT_APP_CMP("bwd")) { //previous track
        _ut_app_sink_key_action(KEY_AVRCP_BACKWARD, NULL, 0);
    } else if (UT_APP_CMP("ffwd")) { //fast forward
        uint8_t op = (uint8_t)strtoul(cmd + 5, NULL, 10);
        if (op != 0 && op != 1) {
            BT_LOGE("APP", "sink ffwd, operation should be 0 or 1");
            return BT_STATUS_FAIL;
        }
        if (op == 0)
            _ut_app_sink_key_action(KEY_AVRCP_FAST_FORWARD_PRESS, NULL, 0);
        else
            _ut_app_sink_key_action(KEY_AVRCP_FAST_FORWARD_RELEASE, NULL, 0);
    } else if (UT_APP_CMP("rwd")) { //rewind
        uint8_t op = (uint8_t)strtoul(cmd + 4, NULL, 10);
        if (op != 0 && op != 1) {
            BT_LOGE("APP", "sink rwd, operation should be 0 or 1");
            return BT_STATUS_FAIL;
        }
        if (op == 0)
            _ut_app_sink_key_action(KEY_AVRCP_FAST_REWIND_PRESS, NULL, 0);
        else
            _ut_app_sink_key_action(KEY_AVRCP_FAST_REWIND_RELEASE, NULL, 0);
    }

    /*AVRCP additional info cli */
    else if (UT_APP_CMP("gea")) {
        bt_sink_srv_avrcp_get_element_attributes_parameter_t attribute_params;
        memset(&attribute_params, 0, sizeof(bt_sink_srv_avrcp_get_element_attributes_parameter_t));
        attribute_params.accept_fragment = false;
        attribute_params.attribute_size = 28;
        bt_avrcp_get_element_attributes_t *attribute_list;
        attribute_list = pvPortMalloc(sizeof(bt_avrcp_get_element_attributes_t) * 7);
        memset(attribute_list, 0, sizeof(bt_avrcp_get_element_attributes_t) * 7);
        attribute_list[0].attribute_id = 1;
        attribute_list[1].attribute_id = 2;
        attribute_list[2].attribute_id = 3;
        attribute_list[3].attribute_id = 4;
        attribute_list[4].attribute_id = 5;
        attribute_list[5].attribute_id = 6;
        attribute_list[6].attribute_id = 7;
        attribute_params.address = NULL;
        attribute_params.attribute_list = attribute_list;
        _ut_app_sink_key_action(KEY_AVRCP_GET_ATTRIBUTE, (void *)&attribute_params,
                                sizeof(bt_sink_srv_avrcp_get_element_attributes_parameter_t));
    } else if (UT_APP_CMP("gps")) {
        _ut_app_sink_key_action(KEY_AVRCP_GET_PLAY_STATUS, NULL, 0);
    } else if (UT_APP_CMP("getc")) {
        bt_sink_srv_avrcp_get_capability_parameter_t capability_parameter;
        bt_avrcp_capability_types_t capability_type = 3;
        /*capability_type = (bt_avrcp_capability_types_t)((*(cmd + 5)) - '0');
        if (capability_type != 0x3) {
            BT_LOGE("APP", "sink getc, operation should be 3", capability_type);
            return BT_STATUS_FAIL;
        }*/

        memset(&capability_parameter, 0, sizeof(bt_sink_srv_avrcp_get_capability_parameter_t));
        capability_parameter.type = capability_type;
        _ut_app_sink_key_action(KEY_AVRCP_GET_CAPABILITY, (void *)&capability_parameter,
                                sizeof(bt_sink_srv_avrcp_get_capability_parameter_t));
    } else if (UT_APP_CMP("regevt")) {
        bt_avrcp_event_t event = (bt_avrcp_event_t)((*(cmd + 7)) - '0');
        _ut_app_sink_key_action(KEY_AVRCP_REGISTER_EVENT, (void *)&event,
                                sizeof(bt_avrcp_event_t));
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */
#ifdef MTK_BT_A2DP_ENABLE
    } else if (UT_APP_CMP("sbcmaxbitpool")) {
        uint32_t value = (uint32_t)((*(cmd + 14)) - '0');
        if ((*(cmd + 15)))
            value = value * 10 + (uint32_t)((*(cmd + 15)) - '0');
        if (value < 5 || value > 75) {
            BT_LOGE("APP", "sbcmaxbitpool, value not valid");
            return BT_STATUS_FAIL;
        }
        _ut_app_sink_key_action(KEY_SBC_MAX_BIT_POOL, (void *)&value, sizeof(uint32_t));
#ifdef MTK_BT_AUDIO_PR
    } else if (UT_APP_CMP("audiopr")) {
        _ut_app_sink_key_action(KEY_AUDIO_PR, NULL, 0);
#endif /* #ifdef MTK_BT_AUDIO_PR */
#endif /* #ifdef MTK_BT_A2DP_ENABLE */
#endif /* #ifdef MTK_BT_DUO_ENABLE */
    } else if (UT_APP_CMP("mm")) {
        extern void bt_mm_dump_state(bt_memory_packet_t type);
        bt_mm_dump_state(BT_MEMORY_RX_BUFFER);
    }

    return BT_STATUS_SUCCESS;
}


