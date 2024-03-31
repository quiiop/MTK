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

#include "app_music_utils.h"
#include "app_music_activity.h"
#include "bt_sink_srv_music.h"
#include "bt_device_manager.h"
#include "apps_debug.h"
#include "bt_sink_srv.h"
#ifdef AIR_LE_AUDIO_ENABLE
#include "app_le_audio.h"
#include "bt_sink_srv_le.h"
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */
#include "bt_sink_srv_ami.h"
#include "apps_events_interaction_event.h"
#include "apps_events_event_group.h"
#include "bt_connection_manager.h"

#define APP_MUSIC_UTILS "[APP_MUSIC_UTIL]"

#define APP_MCP_SUP_OP_DETAIL_LOG

static unsigned int g_attr_length = 0;
static unsigned int g_attr_index = 0;
static bt_avrcp_get_element_attributes_response_value_t *g_attr_list = NULL;

#ifdef APP_MCP_SUP_OP_DETAIL_LOG
#define APP_MCP_SUP_OP_PARSER(SUP_OP, OP) \
    if (SUP_OP & BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_##OP) \
        APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP] "#OP, 0)
#endif /* #ifdef APP_MCP_SUP_OP_DETAIL_LOG */

#ifdef AIR_LE_AUDIO_ENABLE
bool app_do_le_music_action(ui_shell_activity_t *self, apps_config_key_action_t action, void *data)
{
    bt_sink_srv_action_t sink_action = BT_SINK_SRV_ACTION_NONE;
    //apps_music_local_context_t *local_context = (apps_music_local_context_t *)self->local_context;
    bt_status_t bt_status;
    void *op = NULL;

    bool ret = true;

    switch (action) {
        case KEY_LE_MCP_PLAY: {
                sink_action = BT_SINK_SRV_ACTION_PLAY;
                break;
            }
        case KEY_LE_MCP_PAUSE: {
                sink_action = BT_SINK_SRV_ACTION_PAUSE;
                break;
            }
        case KEY_LE_MCP_FAST_FORWARD: {
                sink_action = BT_SINK_SRV_ACTION_FAST_FORWARD;
                break;
            }
        case KEY_LE_MCP_FAST_REWIND: {
                sink_action = BT_SINK_SRV_ACTION_REWIND;
                break;
            }
        case KEY_LE_MCP_NEXT_TRACK: {
                sink_action = BT_SINK_SRV_ACTION_NEXT_TRACK;
                break;
            }
        case KEY_LE_MCP_PRE_TRACK: {
                sink_action = BT_SINK_SRV_ACTION_PREV_TRACK;
                break;
            }
        case KEY_LE_MCP_GET_SUPPORTED_CTRL_OPCODE: {
                sink_action = BT_SINK_SRV_ACTION_LE_GET_CTR_OPCODES;//BT_SINK_SRV_ACTION_GET_CAPABILITY;
                break;
            }
        case KEY_LE_MCP_GET_MEDIA_STATE: {
                sink_action = BT_SINK_SRV_ACTION_GET_PLAY_STATUS;
                break;
            }
#ifdef MTK_COMMON_LE_MCP_EVT
        case KEY_LE_MCP_GET_MEDIA_PLAYER_NAME: {
                sink_action = BT_SINK_SRV_ACTION_LE_GET_PLAYER_NAME;
                break;
            }
        case KEY_LE_MCP_GET_TRACK_TITLE: {
                sink_action = BT_SINK_SRV_ACTION_LE_GET_TRACK_TITLE;
                break;
            }
        case KEY_LE_MCP_GET_TRACK_DURATION: {
                sink_action = BT_SINK_SRV_ACTION_LE_GET_TRACK_DURATION;
                break;
            }
        case KEY_LE_MCP_GET_TRACK_POSITION: {
                sink_action = BT_SINK_SRV_ACTION_LE_GET_TRACK_POSITION;
                break;
            }
#endif /* #ifdef MTK_COMMON_LE_MCP_EVT */
        case KEY_LE_VCP_GET_VOLUME_INFO: {
                app_le_audio_dump_volume_setting();
                return ret;
            }
        default: {
                ret = false;
                break;
            }
    }

    //APPS_LOG_MSGID_I(APP_MUSIC_UTILS"do music action: ret: 0x%x, sink_action : 0x%x", 2, ret, sink_action);

    if (ret) {
        //APPS_LOG_MSGID_I(APP_MUSIC_UTILS"do music action: param: 0x%x, sink_action : 0x%x", 2, param, sink_action);
        bt_status = le_sink_srv_send_action(sink_action, op);
        if (bt_status != BT_STATUS_SUCCESS) {
            APPS_LOG_MSGID_E(APP_MUSIC_UTILS"do le music action fail, bt_status: 0x%x, sink_action : 0x%x", 2, bt_status, sink_action);
        }
    }
    return ret;
}
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */

bool app_do_music_action(ui_shell_activity_t *self, apps_config_key_action_t action, void *data)
{
    bt_sink_srv_action_t sink_action = BT_SINK_SRV_ACTION_NONE;
    bt_status_t bt_status;
    void *op = NULL;
#ifdef MTK_BT_AVRCP_ENABLE
    apps_music_local_context_t *local_context = (apps_music_local_context_t *)self->local_context;
    bt_sink_srv_avrcp_operation_state_t param;
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */

    bool ret = true;

    /* APPS_LOG_MSGID_I(APP_MUSIC_UTILS"do music action: 0x%x, avrcp op sta: %d", 2,
                     action, local_context->avrcp_op_sta);*/

    /* Map key action to sink service action. */
    switch (action) {
#ifdef MTK_BT_AVRCP_ENABLE
        case KEY_AVRCP_PLAY:
        case KEY_AVRCP_PAUSE: {
                sink_action = BT_SINK_SRV_ACTION_PLAY_PAUSE;
                break;
            }
        case KEY_VOICE_UP: {
                sink_action = BT_SINK_SRV_ACTION_VOLUME_UP;
                break;
            }
        case KEY_VOICE_DN: {
                sink_action = BT_SINK_SRV_ACTION_VOLUME_DOWN;
                break;
            }
        case KEY_AVRCP_BACKWARD: {
                sink_action = BT_SINK_SRV_ACTION_PREV_TRACK;
                break;
            }
        case KEY_AVRCP_FORWARD: {
                sink_action = BT_SINK_SRV_ACTION_NEXT_TRACK;
                break;
            }
        case KEY_AVRCP_FAST_FORWARD_PRESS: {
                if (local_context->avrcp_op_sta == AVRCP_OPERATION_STA_IDLE) {
                    param = BT_SINK_SRV_AVRCP_OPERATION_PRESS;
                    op = (void *)&param;
                    sink_action = BT_SINK_SRV_ACTION_FAST_FORWARD;
                    local_context->avrcp_op_sta = AVRCP_OPERATION_STA_FAST_FORWARD_PRESS;
                } else {
                    ret = false;
                }
                break;
            }
        case KEY_AVRCP_FAST_FORWARD_RELEASE: {
                if (local_context->avrcp_op_sta == AVRCP_OPERATION_STA_FAST_FORWARD_PRESS) {
                    param = BT_SINK_SRV_AVRCP_OPERATION_RELEASE;
                    op = (void *)&param;
                    sink_action = BT_SINK_SRV_ACTION_FAST_FORWARD;
                    local_context->avrcp_op_sta = AVRCP_OPERATION_STA_IDLE;
                } else {
                    ret = false;
                }
                break;
            }
        case KEY_AVRCP_FAST_REWIND_PRESS: {
                if (local_context->avrcp_op_sta == AVRCP_OPERATION_STA_IDLE) {
                    param = BT_SINK_SRV_AVRCP_OPERATION_PRESS;
                    op = (void *)&param;
                    sink_action = BT_SINK_SRV_ACTION_REWIND;
                    local_context->avrcp_op_sta = AVRCP_OPERATION_STA_FAST_REWIND_PRESS;
                } else {
                    ret = false;
                }
                break;
            }
        case KEY_AVRCP_FAST_REWIND_RELEASE: {
                if (local_context->avrcp_op_sta == AVRCP_OPERATION_STA_FAST_REWIND_PRESS) {
                    param = BT_SINK_SRV_AVRCP_OPERATION_RELEASE;
                    op = (void *)&param;
                    sink_action = BT_SINK_SRV_ACTION_REWIND;
                    local_context->avrcp_op_sta = AVRCP_OPERATION_STA_IDLE;
                } else {
                    ret = false;
                }
                break;
            }
        case KEY_AVRCP_GET_ATTRIBUTE: {
                op = data;
                sink_action = BT_SINK_SRV_ACTION_GET_ELEMENT_ATTRIBUTE;
                break;
            }
        case KEY_AVRCP_GET_PLAY_STATUS: {
                sink_action = BT_SINK_SRV_ACTION_GET_PLAY_STATUS;
                break;
            }
        case KEY_AVRCP_GET_CAPABILITY: {
                op = data;
                sink_action = BT_SINK_SRV_ACTION_GET_CAPABILITY;
                break;
            }
        case KEY_AVRCP_REGISTER_EVENT: {
                bt_sink_srv_music_device_t *dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_SP, NULL);
                if (dev) {
                    APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[SINK][ATCI]reg_event: dev:%x, evt:%x, status:%x", 3,
                                     dev->avrcp_hd, *(bt_avrcp_event_t *)data,
                                     bt_avrcp_register_notification(dev->avrcp_hd, *(bt_avrcp_event_t *)data, 0));
                } else
                    APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[SINK][ATCI]reg_event: device not found", 0);
                break;
            }
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */
#ifdef MTK_BT_A2DP_ENABLE
        case KEY_SBC_MAX_BIT_POOL: {
                bt_sink_srv_music_set_max_bit_pool(*(uint32_t *)data);
                ret = true;
                break;
            }
#endif /* #ifdef MTK_BT_A2DP_ENABLE */
        default: {
                ret = false;
                break;
            }
    }

    //APPS_LOG_MSGID_I(APP_MUSIC_UTILS"do music action: ret: 0x%x, sink_action : 0x%x", 2, ret, sink_action);

    if (ret) {
        //APPS_LOG_MSGID_I(APP_MUSIC_UTILS"do music action: param: 0x%x, sink_action : 0x%x", 2, param, sink_action);
        bt_status = bt_sink_srv_send_action(sink_action, op);
        if (bt_status != BT_STATUS_SUCCESS) {
            APPS_LOG_MSGID_E(APP_MUSIC_UTILS"do music action fail, bt_status: 0x%x, sink_action : 0x%x", 2, bt_status, sink_action);
        }
    }
#ifdef AIR_LE_AUDIO_ENABLE
    else
        ret = app_do_le_music_action(self, action, data);
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */

    return ret;
}

static char *app_bt_music_play_status_text(bt_avrcp_status_t status)
{
    switch (status) {
#ifdef MTK_BT_AVRCP_ENABLE
        case BT_AVRCP_STATUS_PLAY_STOPPED:
            return "STOPPED";
        case BT_AVRCP_STATUS_PLAY_PLAYING:
            return "PLAYING";
        case BT_AVRCP_STATUS_PLAY_PAUSED:
            return "PAUSED";
        case BT_AVRCP_STATUS_PLAY_FWD_SEEK:
            return "FWD SEEK";
        case BT_AVRCP_STATUS_PLAY_REV_SEEK:
            return "REV SEEK";
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */
        default:
            return "Unknown";
    }
}

#if defined(AIR_LE_AUDIO_ENABLE) && defined(MTK_COMMON_LE_MCP_EVT)
static char *app_bt_music_mcp_opcode_to_str(ble_mcs_media_control_point_t opcode)
{
    switch (opcode) {
        case BLE_MCS_MEDIA_CONTROL_POINT_PLAY:
            return "PLAY";
        case BLE_MCS_MEDIA_CONTROL_POINT_PAUSE:
            return "PAUSE";
        case BLE_MCS_MEDIA_CONTROL_POINT_FAST_REWIND:
            return "FRWD";
        case BLE_MCS_MEDIA_CONTROL_POINT_FAST_FORWARD:
            return "FFWD";
        case BLE_MCS_MEDIA_CONTROL_POINT_PREVIOUS_TRACK:
            return "PRE_TRACK";
        case BLE_MCS_MEDIA_CONTROL_POINT_NEXT_TRACK:
            return "NEXT_TRACK";
        default:
            break;
    }
    return NULL;
}

static char *app_bt_music_mcp_ctrlPointResult_to_str(ble_mcs_media_control_point_t opcode)
{
    switch (opcode) {
        case BLE_MCS_MEDIA_CONTROL_POINT_RESULT_SUCCESS:
            return "SUCCESS";
        case BLE_MCS_MEDIA_CONTROL_POINT_RESULT_OPCODE_NOT_SUPPORTED:
            return "NOT SUPPORTED";
        case BLE_MCS_MEDIA_CONTROL_POINT_RESULT_OPCODE_ACTION_UNSUCCESSFUL:
            return "FAIL";
        case BLE_MCS_MEDIA_CONTROL_POINT_RESULT_MEDIA_PLAYER_INACTIVE:
            return "INACTIVE";
        default:
            break;
    }
    return "UNKNOWN";
}

static char *app_bt_music_mcp_play_status_text(ble_mcs_media_state_t status)
{
    switch (status) {
        case BLE_MCS_MEDIA_STATE_STOPED:
            return "STOPPED";
        case BLE_MCS_MEDIA_STATE_PLAYING:
            return "PLAYING";
        case BLE_MCS_MEDIA_STATE_PAUSED:
            return "PAUSED";
        case BLE_MCS_MEDIA_STATE_SEEKING:
            return "SEEKING";
        default:
            return "Unknown";
    }
}

static void app_bt_muisc_mcp_evt_ind_parser(bt_sink_srv_event_mcp_ind_t *p_ind)
{
    if (!p_ind)
        return;

    if (!p_ind->is_param_valid) {
        APPS_LOG_MSGID_I(APP_MUSIC_UTILS",Why input param is invalid!", 0);
        return;
    }

    switch (p_ind->event_type) {
        case BLE_MCP_SET_MEDIA_PLAYER_NAME_NOTIFICATION_CNF:
        case BLE_MCP_SET_TRACK_CHANGED_NOTIFICATION_CNF:
        case BLE_MCP_SET_TRACK_TITLE_NOTIFICATION_CNF:
        case BLE_MCP_SET_TRACK_DURATION_NOTIFICATION_CNF:
        case BLE_MCP_SET_TRACK_POSITION_NOTIFICATION_CNF:
        case BLE_MCP_SET_PLAYBACK_SPEED_NOTIFICATION_CNF:
        case BLE_MCP_SET_SEEKING_SPEED_NOTIFICATION_CNF:
        case BLE_MCP_SET_PLAYING_ORDER_NOTIFICATION_CNF:
        case BLE_MCP_SET_MEDIA_STATE_NOTIFICATION_CNF:
        case BLE_MCP_SET_MEDIA_CONTROL_OPCODES_SUPPORTED_NOTIFICATION_CNF: {
                ble_mcp_event_parameter_t *p_cnf = &p_ind->param.event_param;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Noti cnf (0x%x), hdl=0x%x", 2, p_ind->status, p_cnf->handle);
                break;
            }

        case BLE_MCP_READ_MEDIA_PLAYER_NAME_CNF: {
                ble_mcp_read_media_player_name_cnf_t *p_cnf = &p_ind->param.player_name_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read player name cnf(0x%x), hdl:0x%x, name: %s(len:%d)", 4,
                                 p_ind->status, p_cnf->handle,
                                 p_cnf->media_player_name_length > 0 ? (char *)p_cnf->p_media_player_name : "",
                                 p_cnf->media_player_name_length);
                break;
            }

        case BLE_MCP_READ_MEDIA_PLAYER_ICON_URL_CNF: {
                ble_mcp_read_media_player_icon_url_cnf_t *p_cnf = &p_ind->param.player_icon_url_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read player icon url cnf(0x%x), hdl:0x%x, url: %s(len:%d)", 4,
                                 p_ind->status, p_cnf->handle,
                                 p_cnf->media_player_icon_url_length > 0 ? (char *)p_cnf->p_media_player_icon_url : "",
                                 p_cnf->media_player_icon_url_length);
                break;
            }
        case BLE_MCP_READ_TRACK_TITLE_CNF: {
                ble_mcp_read_track_title_cnf_t *p_cnf = &p_ind->param.track_title_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read track title cnf(0x%x), hdl:0x%x, name: %s(len:%d)", 4,
                                 p_ind->status, p_cnf->handle,
                                 p_cnf->track_title_length > 0 ? (char *)p_cnf->p_track_title : "",
                                 p_cnf->track_title_length);
                break;
            }
        case BLE_MCP_READ_TRACK_DURATION_CNF: {
                ble_mcp_read_track_duration_cnf_t *p_cnf = &p_ind->param.track_dur_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read track duration cnf(0x%x), hdl:0x%x, dur: %d", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->track_duration);
                break;
            }
        case BLE_MCP_READ_TRACK_POSITION_CNF: {
                ble_mcp_read_track_position_cnf_t *p_cnf = &p_ind->param.track_pos_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read track position cnf(0x%x), hdl:0x%x, pos: %d", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->track_position);
                break;
            }
        case BLE_MCP_READ_PLAYBACK_SPEED_CNF: {
                ble_mcp_read_playback_speed_cnf_t *p_cnf = &p_ind->param.playback_speed_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read playback speed cnf(0x%x), hdl:0x%x, speed: %d", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->playback_speed);
                break;
            }
        case BLE_MCP_READ_SEEKING_SPEED_CNF: {
                ble_mcp_read_seeking_speed_cnf_t *p_cnf = &p_ind->param.seeking_speed_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read seeking speed cnf(0x%x), hdl:0x%x, speed: %d", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->seeking_speed);
                break;
            }
        case BLE_MCP_READ_PLAYING_ORDER_CNF: {
                ble_mcp_read_playing_order_cnf_t *p_cnf = &p_ind->param.playing_order_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read playing order cnf(0x%x), hdl:0x%x, value: 0x%x", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->playing_order);
                break;
            }
        case BLE_MCP_READ_PLAYING_ORDERS_SUPPORTED_CNF: {
                ble_mcp_read_playing_orders_supported_cnf_t *p_cnf = &p_ind->param.playing_order_sup_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read playing order sup cnf(0x%x), hdl:0x%x, value: 0x%x", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->playing_order_supported);
                break;
            }
        case BLE_MCP_READ_MEDIA_STATE_CNF: {
                ble_mcp_read_media_state_cnf_t *p_cnf = &p_ind->param.media_state_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read media state cnf(0x%x), hdl:0x%x, value: 0x%x", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->media_state);
                break;
            }

        case BLE_MCP_READ_MEDIA_CONTROL_OPCODES_SUPPORTED_CNF: {
                ble_mcp_read_media_control_opcodes_supported_cnf_t *p_cnf  = &p_ind->param.ctrl_op_sup_cnf;
                ble_mcs_media_control_opcodes_supported_t sup_op = p_cnf->media_control_opcodes_supported;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read ctr op support(0x%x), hdl:0x%x, value: 0x%x", 3,
                                 p_ind->status, p_cnf->handle, sup_op);
#ifdef APP_MCP_SUP_OP_DETAIL_LOG
                if (!sup_op)
                    break;

                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP] Supported commands", 0);
                APP_MCP_SUP_OP_PARSER(sup_op, PLAY);
                APP_MCP_SUP_OP_PARSER(sup_op, PAUSE);
                APP_MCP_SUP_OP_PARSER(sup_op, FAST_FORWARD);
                APP_MCP_SUP_OP_PARSER(sup_op, FAST_REWIND);
                APP_MCP_SUP_OP_PARSER(sup_op, STOP);
                APP_MCP_SUP_OP_PARSER(sup_op, MOVE_RELATIVE);
                APP_MCP_SUP_OP_PARSER(sup_op, PREVIOUS_SEGMENT);
                APP_MCP_SUP_OP_PARSER(sup_op, NEXT_SEGMENT);
                APP_MCP_SUP_OP_PARSER(sup_op, FIRST_SEGMENT);
                APP_MCP_SUP_OP_PARSER(sup_op, LAST_SEGMENT);
                APP_MCP_SUP_OP_PARSER(sup_op, GOTO_SEGMENT);
                APP_MCP_SUP_OP_PARSER(sup_op, PREVIOUS_TRACK);
                APP_MCP_SUP_OP_PARSER(sup_op, NEXT_TRACK);
                APP_MCP_SUP_OP_PARSER(sup_op, FIRST_TRACK);
                APP_MCP_SUP_OP_PARSER(sup_op, LAST_TRACK);
                APP_MCP_SUP_OP_PARSER(sup_op, GOTO_TRACK);
                APP_MCP_SUP_OP_PARSER(sup_op, PREVIOUS_GROUP);
                APP_MCP_SUP_OP_PARSER(sup_op, NEXT_GROUP);
                APP_MCP_SUP_OP_PARSER(sup_op, FIRST_GROUP);
                APP_MCP_SUP_OP_PARSER(sup_op, LAST_GROUP);
                APP_MCP_SUP_OP_PARSER(sup_op, GOTO_GROUP);
#endif /* #ifdef APP_MCP_SUP_OP_DETAIL_LOG */
                break;
            }
        case BLE_MCP_READ_CONTENT_CONTROL_ID_CNF: {
                ble_mcp_read_content_control_id_cnf_t *p_cnf  = &p_ind->param.content_ctrl_id_cnf;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Read ctr id cnf(0x%x), hdl:0x%x, value: 0x%x", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->content_control_id);
                break;
            }
        case BLE_MCP_MEDIA_PLAYER_NAME_IND: {
                ble_mcp_media_player_name_ind_t *p_cnf  = &p_ind->param.player_name_ind;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Player name ind(0x%x), hdl:0x%x, name: %s(len:%d)", 4,
                                 p_ind->status, p_cnf->handle,
                                 p_cnf->media_player_name_length > 0 ? (char *)p_cnf->p_media_player_name : "",
                                 p_cnf->media_player_name_length);
                break;
            }
        case BLE_MCP_TRACK_CHANGED_IND: {
                ble_mcp_track_changed_ind_t *p_cnf  = &p_ind->param.track_changed_ind;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Track changed ind(0x%x), hdl:0x%x", 2,
                                 p_ind->status, p_cnf->handle);
                break;
            }
        case BLE_MCP_TRACK_TITLE_IND: {
                ble_mcp_track_title_ind_t *p_cnf = &p_ind->param.track_title_ind;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Track title ind(0x%x), hdl:0x%x, name: %s(len:%d)", 4,
                                 p_ind->status, p_cnf->handle,
                                 p_cnf->track_title_length > 0 ? (char *)p_cnf->p_track_title : "",
                                 p_cnf->track_title_length);
                break;
            }
        case BLE_MCP_TRACK_DURATION_IND: {
                ble_mcp_track_duration_ind_t *p_cnf = &p_ind->param.track_dur_ind;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Track duration ind(0x%x), hdl:0x%x, dur: %d", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->track_duration);
                break;
            }
        case BLE_MCP_TRACK_POSITION_IND: {
                ble_mcp_track_position_ind_t *p_cnf = &p_ind->param.track_pos_ind;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Track position ind(0x%x), hdl:0x%x, pos: %d", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->track_position);
                break;
            }
        case BLE_MCP_PLAYBACK_SPEED_IND: {
                ble_mcp_playback_speed_ind_t *p_cnf = &p_ind->param.playback_speed_ind;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Playback speed ind(0x%x), hdl:0x%x, speed: %d", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->playback_speed);
                break;
            }
        case BLE_MCP_SEEKING_SPEED_IND: {
                ble_mcp_seeking_speed_ind_t *p_cnf = &p_ind->param.seeking_speed_ind;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Seeking speed ind(0x%x), hdl:0x%x, speed: %d", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->seeking_speed);
                break;
            }
        case BLE_MCP_PLAYING_ORDER_IND: {
                ble_mcp_playing_order_ind_t *p_cnf = &p_ind->param.playing_order_ind;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Playing order ind(0x%x), hdl:0x%x, value: 0x%x", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->playing_order);
                break;
            }
        case BLE_MCP_MEDIA_STATE_IND: {
                ble_mcp_media_state_ind_t *p_cnf = &p_ind->param.media_state_ind;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Media state ind(0x%x), hdl:0x%x, value: 0x%x", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->media_state);
                break;
            }
        case BLE_MCP_MEDIA_CONTROL_OPCODES_SUPPORTED_IND: {
                ble_mcp_media_control_opcodes_supported_ind_t *p_cnf  = &p_ind->param.ctrl_op_sup_ind;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Ctrl op support ind(0x%x), hdl:0x%x, value: 0x%x", 3,
                                 p_ind->status, p_cnf->handle, p_cnf->media_control_opcodes_supported);
                break;
            }
        case BLE_MCP_MEDIA_CONTROL_POINT_IND: {
                ble_mcp_media_control_point_ind_t *p_cnf  = &p_ind->param.ctrl_point_ind;
                if (app_bt_music_mcp_opcode_to_str(p_cnf->requested_opcode))
                    APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Ctrl point ind(0x%x), hdl:0x%x, op: %s, result = %s", 4,
                                     p_ind->status, p_cnf->handle,
                                     app_bt_music_mcp_opcode_to_str(p_cnf->requested_opcode),
                                     app_bt_music_mcp_ctrlPointResult_to_str(p_cnf->result_code));
                else
                    APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Ctrl point ind(0x%x), hdl:0x%x, op: 0x%x, result = 0x%x", 4,
                                     p_ind->status, p_cnf->handle, p_cnf->requested_opcode, p_cnf->result_code);
                break;
            }
        case BLE_MCP_PLAY_CURRENT_TRACK_CNF:
        case BLE_MCP_PAUSE_CURRENT_TRACK_CNF:
        case BLE_MCP_MOVE_TO_NEXT_TRACK_CNF:
        case BLE_MCP_MOVE_TO_PREVIOUS_TRACK_CNF:
        case BLE_MCP_FAST_FORWARD_CNF:
        case BLE_MCP_FAST_REWIND_CNF: {
                ble_mcp_event_parameter_t *p_cnf  = &p_ind->param.event_param;
                if (p_ind->status == BT_STATUS_UNSUPPORTED) {
                    APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Key evt = 0x%x not supported", 1, p_ind->event_type);
                } else
                    APPS_LOG_MSGID_I(APP_MUSIC_UTILS"[MCP]Key cnf (0x%x), hdl:0x%x, evt = 0x%x", 3,
                                     p_ind->status, p_cnf->handle, p_ind->event_type);
                break;
            }
        default:
            APPS_LOG_MSGID_W(APP_MUSIC_UTILS"[MCP]not supported event 0x%x, status = 0x%x", 2,
                             p_ind->event_type, p_ind->status);
            break;
    }

}
#endif /* #if defined(AIR_LE_AUDIO_ENABLE) && defined(MTK_COMMON_LE_MCP_EVT) */

static void _app_bt_music_endian_order_swap(uint8_t *dest, const uint8_t *src, uint8_t len)
{
    uint8_t temp[4]; /*Add temp variable to support dest and src are same point*/
    uint8_t i;

    if (len == 2 || len == 4) {
        for (i = 0; i < len; i++)
            *(temp + i) = *(src + (len - 1) - i);

        memcpy(dest, temp, len);
    }
}

void app_bt_music_free_attr_list(void)
{
    if (g_attr_list) {
        vPortFree(g_attr_list);
        g_attr_list = NULL;
    }
    g_attr_length = 0;
    g_attr_index = 0;
}

bool app_bt_music_proc_basic_state_event(ui_shell_activity_t *self, uint32_t event_id,
                                         void *extra_data, size_t data_len)
{
    bool ret = false;
    apps_music_local_context_t *local_context = (apps_music_local_context_t *)self->local_context;

    /* APPS_LOG_MSGID_I(APP_MUSIC_UTILS"event_id : 0x%x", 1, event_id); */

    switch (event_id) {
#if defined(AIR_LE_AUDIO_ENABLE) && defined(MTK_COMMON_LE_STATUS_EVT)
        case LE_SINK_SRV_EVENT_STATE_CHANGE:
#endif /* #if defined(AIR_LE_AUDIO_ENABLE) && defined(MTK_COMMON_LE_STATUS_EVT) */
        case BT_SINK_SRV_EVENT_STATE_CHANGE: {
                bt_sink_srv_state_change_t *param = (bt_sink_srv_state_change_t *)extra_data;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS",bt_music_state_change: now = 0x%x, pre = 0x%x", 2, param->current, param->previous);
                /* Try to start app_music_activity when the music starts playing. */
                if ((param->previous != BT_SINK_SRV_STATE_STREAMING) && (param->current == BT_SINK_SRV_STATE_STREAMING)) {
                    if (local_context) {
                        /* If music_playing is true, it indicates that the app_music_activity already exists. */
                        if (!local_context->music_playing) {
                            local_context->isAutoPaused = false;
                            ui_shell_start_activity(self, app_music_activity_proc, ACTIVITY_PRIORITY_MIDDLE, local_context, 0);
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS",start app_music_activity_proc, curr act: %x, isAutoPaused: %d", 2,
                                             (uint32_t)self, local_context->isAutoPaused);
                            local_context->music_playing = true;
                        }
                    }
                }
                ret = true;
                break;
            }
        case BT_SINK_SRV_EVENT_AVRCP_STATUS_CHANGE: {
                bt_sink_srv_event_param_t *event = (bt_sink_srv_event_param_t *)extra_data;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"AVRCP Status Change : %s", 1, app_bt_music_play_status_text(event->avrcp_status_change.avrcp_status));
                ret = true;
                break;
            }
#ifdef AIR_LE_AUDIO_ENABLE
#ifdef MTK_COMMON_LE_MCP_EVT
        case BT_SINK_SRV_EVENT_LE_MCP_STATUS_CHANGE: {
                bt_sink_srv_event_param_t *event = (bt_sink_srv_event_param_t *)extra_data;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"MCP Status Change : %s", 1, app_bt_music_mcp_play_status_text(event->mcp_status_change.mcp_status));
                ret = true;
                break;
            }
        case BT_SINK_SRV_EVENT_LE_MCP_EVENT_IND: {
                bt_sink_srv_event_mcp_ind_t *event = (bt_sink_srv_event_mcp_ind_t *)extra_data;
                //APPS_LOG_MSGID_I(APP_MUSIC_UTILS"MCP Event Ind : 0x%x", 1, event->event_type);
                app_bt_muisc_mcp_evt_ind_parser(event);
                ret = true;
                break;
            }
#endif /* #ifdef MTK_COMMON_LE_MCP_EVT */
#ifdef MTK_COMMON_LE_VCP_EVT
        case BT_SINK_SRV_EVENT_LE_VCP_VOLUME_STATE_CHANGE: {
                ble_vcs_volume_state_t *vol_state = (ble_vcs_volume_state_t *)extra_data;

                /* Store volume setting */
                app_le_audio_store_volume_setting(vol_state->volume, vol_state->mute);
                ret = true;
                break;
            }
#endif /* #ifdef MTK_COMMON_LE_VCP_EVT */
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */
        case BT_SINK_SRV_EVENT_AVRCP_GET_ELEMENT_ATTRIBUTES_CNF: {
                bt_sink_srv_avrcp_get_element_attributes_cnf_t *event = (bt_sink_srv_avrcp_get_element_attributes_cnf_t *)extra_data;
                uint16_t attr_length = g_attr_length;
                uint16_t attr_index = g_attr_index;
                bt_avrcp_get_element_attributes_response_value_t *attr_list = g_attr_list;
                bt_avrcp_get_element_attributes_response_value_t *attr = NULL;
                uint16_t attr_header_len = sizeof(bt_avrcp_get_element_attributes_response_value_t) - 1;
                uint16_t current_attr_len = 0;

                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Get Element Attribute type 0x%x len %d", 2, event->packet_type, event->length);

                switch (event->packet_type) {
                    case BT_AVRCP_METADATA_PACKET_TYPE_NON_FRAGMENT :
                    case BT_AVRCP_METADATA_PACKET_TYPE_START :
                        if (attr_list)
                            vPortFree(attr_list);

                        if (!event->attribute_list)
                            return true;

                        if (event->packet_type == BT_AVRCP_METADATA_PACKET_TYPE_START) {
                            attr_list = pvPortMalloc(event->length);
                            if (!attr_list) {
                                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"malloc for attr list failed.", 0);
                                return true;
                            }
                            memcpy(attr_list, event->attribute_list, event->length);
                        } else
                            attr_list = event->attribute_list;

                        attr_length = event->length;
                        break;
                    case BT_AVRCP_METADATA_PACKET_TYPE_CONTINUE :
                    case BT_AVRCP_METADATA_PACKET_TYPE_END : {
                            uint8_t *temp = NULL;
                            if (!attr_list) {
                                APPS_LOG_MSGID_E(APP_MUSIC_UTILS"previous attr not exists!.", 0);
                                return true;
                            }

                            /* Malloc a new buffer to store remain data and new data */
                            temp = pvPortMalloc(attr_length + event->length);
                            if (!temp) {
                                APPS_LOG_MSGID_E(APP_MUSIC_UTILS"malloc for attr list failed.", 0);
                                app_bt_music_free_attr_list();
                                return true;
                            }
                            memcpy(temp, (uint8_t *)attr_list + attr_index, attr_length);
                            memcpy(temp + attr_length, event->data, event->length);

                            /* Free the old buffer and change pointer to new buffer */
                            vPortFree(attr_list);
                            attr_list = (bt_avrcp_get_element_attributes_response_value_t *)temp;

                            /* If remain length >= 8, the data already handled by BT stack, so need to bypass it. */
                            if (attr_length < 8) {
                                attr_index = 0;
                                attr = attr_list;
                            } else {
                                attr_index = attr_header_len + attr_list->attribute_value_length;
                                attr = (bt_avrcp_get_element_attributes_response_value_t *)((uint8_t *)attr_list + attr_index);
                            }

                            attr_length += event->length;

                            while (attr_index + attr_header_len <= attr_length) { /* make sure it has enough data to handle */
                                _app_bt_music_endian_order_swap((uint8_t *)(&attr->attribute_id),
                                                                (uint8_t *)(&attr->attribute_id), sizeof(bt_avrcp_media_attribute_t));
                                _app_bt_music_endian_order_swap((uint8_t *)(&attr->character_set_id),
                                                                (uint8_t *)(&attr->character_set_id), 2);
                                _app_bt_music_endian_order_swap((uint8_t *)(&attr->attribute_value_length),
                                                                (uint8_t *)(&attr->attribute_value_length), 2);
                                attr_index += attr_header_len + attr->attribute_value_length;
                                attr = (bt_avrcp_get_element_attributes_response_value_t *)((uint8_t *)attr_list + attr_index);
                            }

                            attr_index = 0;
                            break;
                        }
                    default :
                        APPS_LOG_MSGID_I(APP_MUSIC_UTILS"unknown packet type 0x%x", 1, event->packet_type);
                        return true;
                }

                attr = attr_list;
                while (attr->attribute_value_length + attr_header_len <= attr_length) {
                    switch (attr->attribute_id) {
                        case BT_AVRCP_MEDIA_ATTRIBUTE_TITLE:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Title     %-.32s", 1, attr->attribute_value);
                            break;
                        case BT_AVRCP_MEDIA_ATTRIBUTE_ARTIST_NAME:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Artist    %-.32s", 1, attr->attribute_value);
                            break;
                        case BT_AVRCP_MEDIA_ATTRIBUTE_ALBUM_NAME:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Album     %-.32s", 1, attr->attribute_value);
                            break;
                        /*case BT_AVRCP_MEDIA_ATTRIBUTE_MEDIA_NUMBER:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"NoT       %-.32d", 1, attr->attribute_value);
                            break;
                        case BT_AVRCP_MEDIA_ATTRIBUTE_TOTAL_MEDIA_NUMBER:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"TNoT      %-.32d", 1, attr->attribute_value);
                            break;
                        case BT_AVRCP_MEDIA_ATTRIBUTE_GENRE:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Genre     %-.32s", 1, attr->attribute_value);
                            break;*/
                        case BT_AVRCP_MEDIA_ATTRIBUTE_PLAYING_TIME:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Play Time %-.32d ms", 1, attr->attribute_value);
                            break;
                        default:
                            break;
                    }
                    current_attr_len = attr_header_len + attr->attribute_value_length;
                    attr_index += current_attr_len;
                    attr_length -= current_attr_len;
                    attr = (bt_avrcp_get_element_attributes_response_value_t *)((uint8_t *)attr_list + attr_index);
                }

                if (event->packet_type == BT_AVRCP_METADATA_PACKET_TYPE_END) {
                    attr_length = 0;
                    attr_index = 0;
                    vPortFree(attr_list);
                    attr_list = NULL;
                }

                /* Sync back to global variable */
                if (event->packet_type != BT_AVRCP_METADATA_PACKET_TYPE_NON_FRAGMENT) {
                    g_attr_length = attr_length;
                    g_attr_index = attr_index;
                    g_attr_list = attr_list;
                }

                ret = true;
                break;
            }
        case BT_SINK_SRV_EVENT_AVRCP_GET_PLAY_STATUS_CNF: {
                bt_sink_srv_avrcp_get_play_status_cnf_t *event = (bt_sink_srv_avrcp_get_play_status_cnf_t *)extra_data;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Get Play Status Event", 0);
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Play Status   %s", 1, app_bt_music_play_status_text(event->play_status));
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Song Length   %d ms", 1, event->song_length);
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Song Position %d ms", 1, event->song_position);
                ret = true;
                break;
            }
        case BT_SINK_SRV_EVENT_AVRCP_GET_CAPABILITY_CNF: {
                int i;
                bt_sink_srv_avrcp_get_capability_cnf_t *event = (bt_sink_srv_avrcp_get_capability_cnf_t *)extra_data;
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Get Capability Event", 0);
                APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Capability Count %d", 1, event->number);
                for (i = 0; i < event->number; i++) {
                    switch (event->capability_value[i]) {
                        case BT_AVRCP_EVENT_PLAYBACK_STATUS_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_PLAYBACK_STATUS_CHANGED", 1, i);
                            break;
                        case BT_AVRCP_EVENT_TRACK_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_TRACK_CHANGED", 1, i);
                            break;
                        case BT_AVRCP_EVENT_TRACK_REACHED_END:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_TRACK_REACHED_END", 1, i);
                            break;
                        case BT_AVRCP_EVENT_TRACK_REACHED_START:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_TRACK_REACHED_START", 1, i);
                            break;
                        case BT_AVRCP_EVENT_PLAYBACK_POS_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_PLAYBACK_POS_CHANGED", 1, i);
                            break;
                        case BT_AVRCP_EVENT_BATT_STATUS_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_BATT_STATUS_CHANGED", 1, i);
                            break;
                        case BT_AVRCP_EVENT_SYSTEM_STATUS_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_SYSTEM_STATUS_CHANGED", 1, i);
                            break;
                        case BT_AVRCP_EVENT_PLAYER_APP_SETTING_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_PLAYER_APP_SETTING_CHANGED", 1, i);
                            break;
                        case BT_AVRCP_EVENT_NOW_PLAYING_CONTENT_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_NOW_PLAYING_CONTENT_CHANGED", 1, i);
                            break;
                        case BT_AVRCP_EVENT_AVAILABLE_PLAYERS_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_AVAILABLE_PLAYERS_CHANGED", 1, i);
                            break;
                        case BT_AVRCP_EVENT_ADDRESSED_PLAYER_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_ADDRESSED_PLAYER_CHANGED", 1, i);
                            break;
                        case BT_AVRCP_EVENT_UIDS_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_UIDS_CHANGED", 1, i);
                            break;
                        case BT_AVRCP_EVENT_VOLUME_CHANGED:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT_VOLUME_CHANGED", 1, i);
                            break;
                        default:
                            APPS_LOG_MSGID_I(APP_MUSIC_UTILS"Cap[%d] EVENT Unknown", 1, i);
                            break;
                    }
                }
                ret = true;
                break;
            }
        default:
            break;
    }

    return ret;
}

bool app_music_util_bt_cm_event_proc(ui_shell_activity_t *self, uint32_t event_id,
                                     void *extra_data, size_t data_len)
{
    bool ret = false;

    switch (event_id) {
        default:
            break;
    }
    return ret;
}
