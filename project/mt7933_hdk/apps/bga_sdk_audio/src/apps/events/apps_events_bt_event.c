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


/**
 * File: apps_events_bt_event.c
 *
 * Description: This file defines callback of BT and send events to APPs
 *
 */

#include "ui_shell_manager.h"
#include "ui_shell_message_queue.h"
#include "apps_debug.h"
#include "apps_events_event_group.h"
#include "apps_events_bt_event.h"
#include "apps_events_interaction_event.h"
#include "bt_gap_le.h"
#include "bt_connection_manager.h"
#include "bt_device_manager.h"

#include "bt_callback_manager.h"
#include "FreeRTOS.h"
#include "nvkey_id_list.h"

#include "bt_init.h"

#include "bt_sink_srv.h"
#include "bt_sink_srv_avrcp.h"

#define LOG_TAG         "[bt_evt]"

#ifdef MTK_COMMON_LE_MCP_EVT
static char *_app_bt_strdup(const char *in)
{
    size_t len = strlen(in) + 1;
    //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT] alloc len %d for %s", 2, len, in);
    char *p_new = pvPortMalloc(len);
    if (p_new == NULL)
        return NULL;
    memcpy(p_new, in, len);
    p_new[len] = '\0';
    //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT] alloc str at %p (size = %d)", 2, p_new, len);
    return p_new;
}

static void _app_bt_sink_mcp_event_free(void *data)
{
    ui_shell_msg_t *msg = (ui_shell_msg_t *)data;
    bt_sink_srv_event_mcp_ind_t *p_ind;

    //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT] special free, id %x", 1, msg->msg_id);

    if (msg->msg_id != BT_SINK_SRV_EVENT_LE_MCP_EVENT_IND)
        return;

    if (msg->msg_data == NULL)
        return;

    p_ind = (bt_sink_srv_event_mcp_ind_t *)msg->msg_data;

    if (!p_ind->is_param_valid)
        return;

    switch (p_ind->event_type) {
        case BLE_MCP_READ_MEDIA_PLAYER_NAME_CNF: {
                ble_mcp_read_media_player_name_cnf_t *p_cnf = &p_ind->param.player_name_cnf;
                if (p_cnf->p_media_player_name) {
                    //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT] free player name at %p", 1, p_cnf->p_media_player_name);
                    vPortFree(p_cnf->p_media_player_name);
                }
                break;
            }
        case BLE_MCP_READ_MEDIA_PLAYER_ICON_URL_CNF: {
                ble_mcp_read_media_player_icon_url_cnf_t *p_cnf = &p_ind->param.player_icon_url_cnf;
                if (p_cnf->p_media_player_icon_url) {
                    //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT] free url at %p", 1, p_cnf->p_media_player_icon_url);
                    vPortFree(p_cnf->p_media_player_icon_url);
                }
                break;
            }
        case BLE_MCP_READ_TRACK_TITLE_CNF: {
                ble_mcp_read_track_title_cnf_t *p_cnf = &p_ind->param.track_title_cnf;
                if (p_cnf->p_track_title) {
                    //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT] free title at %p", 1, p_cnf->p_track_title);
                    vPortFree(p_cnf->p_track_title);
                }
                break;
            }
        case BLE_MCP_MEDIA_PLAYER_NAME_IND: {
                ble_mcp_media_player_name_ind_t *p_cnf = &p_ind->param.player_name_ind;
                if (p_cnf->p_media_player_name) {
                    //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT] free at %p", 1, p_cnf->p_media_player_name);
                    vPortFree(p_cnf->p_media_player_name);
                }
                break;
            }
        case BLE_MCP_TRACK_TITLE_IND: {
                ble_mcp_track_title_ind_t *p_cnf = &p_ind->param.track_title_ind;
                if (p_cnf->p_track_title) {
                    //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT] free at %p", 1, p_cnf->p_track_title);
                    vPortFree(p_cnf->p_track_title);
                }
                break;
            }

        default:
            break;
    }

}

static void _app_bt_sink_mcp_event_callback(void *param, uint32_t param_len)
{
    bt_sink_srv_event_mcp_ind_t *p_ind;
    bt_sink_srv_event_mcp_ind_t *p_new_ind;
    size_t ind_len = sizeof(bt_sink_srv_event_mcp_ind_t);
    char *p_temp_str = NULL;


    if (param == NULL || param_len == 0)
        return;
    p_ind = (bt_sink_srv_event_mcp_ind_t *)param;
    //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT]type = 0x%x, status = %d", 2, p_ind->event_type, p_ind->status);

    if (!p_ind->is_param_valid) {
        APPS_LOG_MSGID_E(LOG_TAG"[MCP_EVT]sub param is invalid, so ignore!!", 0);
        return;
    }

    p_new_ind = pvPortMalloc(ind_len);
    if (p_new_ind == NULL) {
        APPS_LOG_MSGID_E(LOG_TAG"[MCP_EVT]alloc send evnet fail!!", 0);
        return;
    }
    memcpy(p_new_ind, p_ind, ind_len);

    /* Add event that need exta malloc; otherwise, you can ignore it.*/
    switch (p_ind->event_type) {
        case BLE_MCP_READ_MEDIA_PLAYER_NAME_CNF: {
                ble_mcp_read_media_player_name_cnf_t *p_cnf = &p_ind->param.player_name_cnf;
                //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT]Read player name cnf, hdl:0x%x, name =%s(len:%d)", 3,
                //                 p_cnf->handle,
                //                 p_cnf->media_player_name_length > 0 ? (char *)p_cnf->p_media_player_name : "",
                //                 p_cnf->media_player_name_length);
                //restore the string
                p_temp_str = _app_bt_strdup((const char *)p_cnf->p_media_player_name);
                if (!p_temp_str) {
                    goto set_mcp_event_error;
                }
                p_new_ind->param.player_name_cnf.p_media_player_name = (uint8_t *)p_temp_str;
                break;
            }

        case BLE_MCP_READ_MEDIA_PLAYER_ICON_URL_CNF: {
                ble_mcp_read_media_player_icon_url_cnf_t *p_cnf = &p_ind->param.player_icon_url_cnf;
                //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT]Read player icon url cnf(0x%x), hdl:0x%x, url =%s(len:%d)", 4,
                //                 p_ind->status, p_cnf->handle,
                //                 p_cnf->media_player_icon_url_length > 0 ? (char *)p_cnf->p_media_player_icon_url : "",
                //                 p_cnf->media_player_icon_url_length);
                p_temp_str = _app_bt_strdup((const char *)p_cnf->p_media_player_icon_url);
                if (!p_temp_str) {
                    goto set_mcp_event_error;
                }
                p_new_ind->param.player_icon_url_cnf.p_media_player_icon_url = (uint8_t *)p_temp_str;
                break;
            }
        case BLE_MCP_READ_TRACK_TITLE_CNF: {
                ble_mcp_read_track_title_cnf_t *p_cnf = &p_ind->param.track_title_cnf;
                //APPS_LOG_MSGID_I(LOG_TAG"[MCP_EVT]Read track title cnf(0x%x), hdl:0x%x, name =%s(len:%d)", 4,
                //                 p_ind->status, p_cnf->handle,
                //                 p_cnf->track_title_length > 0 ? (char *)p_cnf->p_track_title : "",
                //                 p_cnf->track_title_length);
                p_temp_str = _app_bt_strdup((const char *)p_cnf->p_track_title);
                if (!p_temp_str) {
                    goto set_mcp_event_error;
                }
                p_new_ind->param.track_title_cnf.p_track_title = (uint8_t *)p_temp_str;
                break;
            }
        case BLE_MCP_MEDIA_PLAYER_NAME_IND: {
                ble_mcp_media_player_name_ind_t *p_cnf = &p_ind->param.player_name_ind;
                p_temp_str = _app_bt_strdup((const char *)p_cnf->p_media_player_name);
                if (!p_temp_str) {
                    goto set_mcp_event_error;
                }
                p_new_ind->param.player_name_ind.p_media_player_name = (uint8_t *)p_temp_str;
                break;
            }
        case BLE_MCP_TRACK_TITLE_IND: {
                ble_mcp_track_title_ind_t *p_cnf = &p_ind->param.track_title_ind;
                p_temp_str = _app_bt_strdup((const char *)p_cnf->p_track_title);
                if (!p_temp_str) {
                    goto set_mcp_event_error;
                }
                p_new_ind->param.track_title_ind.p_track_title = (uint8_t *)p_temp_str;
                break;
            }
        default:
            break;
    }


    ui_shell_send_event(false, EVENT_PRIORITY_MIDDLE, EVENT_GROUP_UI_SHELL_BT_SINK,
                        BT_SINK_SRV_EVENT_LE_MCP_EVENT_IND, p_new_ind,
                        ind_len, _app_bt_sink_mcp_event_free, 0);
    return;
set_mcp_event_error:

    if (p_new_ind)
        vPortFree(p_new_ind);

    APPS_LOG_MSGID_E(LOG_TAG"[MCP_EVT]evnet callback fail at 0x%x", 1, p_ind->event_type);

}
#endif /* #ifdef MTK_COMMON_LE_MCP_EVT */

/**
 * @brief      MTK_David: The implementation of event parser entrance.
 */
char *bt_sink_event_parser(bt_sink_srv_event_t event_id, void *param)
{
    uint32_t module = event_id & BT_SINK_MODULE_EVENT_MASK;

    switch (module) {
        case BT_SINK_MODULE_COMMON_EVENT:
            return "[COMM]";
            /*case BT_SINK_MODULE_HFP_EVENT:
                return;
            case BT_SINK_MODULE_HSP_EVENT:
                return;*/
#ifdef MTK_BT_A2DP_ENABLE
            /*case BT_SINK_MODULE_A2DP_EVENT:
                return;*/
#endif /* #ifdef MTK_BT_A2DP_ENABLE */
#ifdef MTK_BT_AVRCP_ENABLE
        case BT_SINK_MODULE_AVRCP_EVENT:
            return bt_sink_srv_avrcp_event_parser(event_id, param);
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */
#if defined(AIR_LE_AUDIO_ENABLE) && defined(MTK_COMMON_LE_MCP_EVT)
        case BT_SINK_MODULE_LE_MCP_EVENT:
            return "[MCP]";
#endif /* #if defined(AIR_LE_AUDIO_ENABLE) && defined(MTK_COMMON_LE_MCP_EVT) */
        default:
            return ""; //return empty string. Use UNKNOWN term will feel confuse
    }
}

#ifdef MTK_BT_AVRCP_ENABLE
static void _app_bt_sink_srv_avrcp_special_malloc(bt_sink_srv_event_t event_id, void *data)
{
    if (data == NULL)
        return;

    switch (event_id) {
        case BT_SINK_SRV_EVENT_AVRCP_GET_ELEMENT_ATTRIBUTES_CNF: {
                bt_sink_srv_avrcp_get_element_attributes_cnf_t *params = (bt_sink_srv_avrcp_get_element_attributes_cnf_t *)data;

                APPS_LOG_MSGID_I(LOG_TAG"AVRCP special malloc, id %x", 1, event_id);

                if (params->packet_type == BT_AVRCP_METADATA_PACKET_TYPE_START ||
                    params->packet_type == BT_AVRCP_METADATA_PACKET_TYPE_NON_FRAGMENT) {
                    bt_avrcp_get_element_attributes_response_value_t *attribute_list;

                    attribute_list = (bt_avrcp_get_element_attributes_response_value_t *)pvPortMalloc(params->length);
                    if (!attribute_list) {
                        APPS_LOG_MSGID_E(LOG_TAG"malloc fail! %x", 1, event_id);
                        params->attribute_list = NULL;
                        return;
                    }

                    memcpy(attribute_list, params->attribute_list, params->length);
                    params->attribute_list = attribute_list;
                } else if (params->packet_type == BT_AVRCP_METADATA_PACKET_TYPE_CONTINUE ||
                           params->packet_type == BT_AVRCP_METADATA_PACKET_TYPE_END) {
                    uint8_t *data;

                    data = (uint8_t *)pvPortMalloc(params->length);
                    if (!data) {
                        APPS_LOG_MSGID_E(LOG_TAG"malloc fail! %x", 1, event_id);
                        params->data = NULL;
                        return;
                    }
                    memcpy(data, params->data, params->length);
                    params->data = data;
                }
                break;
            }
        default:
            break;
    }
}

static void _app_bt_sink_srv_avrcp_special_free(void *data)
{
    ui_shell_msg_t *msg = (ui_shell_msg_t *)data;

    if (msg->msg_data == NULL)
        return;

    switch (msg->msg_id) {
        case BT_SINK_SRV_EVENT_AVRCP_GET_ELEMENT_ATTRIBUTES_CNF: {
                uint8_t packet_type = ((bt_sink_srv_avrcp_get_element_attributes_cnf_t *)(msg->msg_data))->packet_type;

                APPS_LOG_MSGID_I(LOG_TAG"AVRCP special free, id %x, type 0x%x", 2, msg->msg_id, packet_type);

                if (packet_type == BT_AVRCP_METADATA_PACKET_TYPE_START ||
                    packet_type == BT_AVRCP_METADATA_PACKET_TYPE_NON_FRAGMENT) {
                    bt_avrcp_get_element_attributes_response_value_t *attribute_list =
                        ((bt_sink_srv_avrcp_get_element_attributes_cnf_t *)(msg->msg_data))->attribute_list;

                    if (attribute_list)
                        vPortFree(attribute_list);
                } else if (packet_type == BT_AVRCP_METADATA_PACKET_TYPE_CONTINUE ||
                           packet_type == BT_AVRCP_METADATA_PACKET_TYPE_END) {
                    uint8_t *data = ((bt_sink_srv_avrcp_get_element_attributes_cnf_t *)(msg->msg_data))->data;

                    if (data)
                        vPortFree(data);
                }
                break;
            }
        default:
            break;
    }
}
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */

static void _app_bt_sink_srv_special_malloc(bt_sink_srv_event_t event_id, void *data)
{
    if (data == NULL)
        return;

#ifdef MTK_BT_AVRCP_ENABLE
    if ((event_id & BT_SINK_MODULE_AVRCP_EVENT) == BT_SINK_MODULE_AVRCP_EVENT)
        _app_bt_sink_srv_avrcp_special_malloc(event_id, data);
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */
}

static void _app_bt_sink_srv_special_free(void *data)
{
    ui_shell_msg_t *msg = (ui_shell_msg_t *)data;

    if (msg->msg_data == NULL)
        return;

#ifdef MTK_BT_AVRCP_ENABLE
    if ((msg->msg_id & BT_SINK_MODULE_AVRCP_EVENT) == BT_SINK_MODULE_AVRCP_EVENT)
        _app_bt_sink_srv_avrcp_special_free(data);
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */
}

/**
 * @brief      The implementation of the weak symbol bt_sink_srv_event_callback which declared in bt_sink_srv.h.
 */
void bt_sink_srv_event_callback(bt_sink_srv_event_t event_id, void *param, uint32_t param_len)
{
    void *event_params = NULL;

#ifdef MTK_COMMON_LE_MCP_EVT
    /* Do the special process for LE audio MCP events. And put here to reduce log */
    if (event_id == BT_SINK_SRV_EVENT_LE_MCP_EVENT_IND) {
        _app_bt_sink_mcp_event_callback(param, param_len);
        return;
    }
#endif /* #ifdef MTK_COMMON_LE_MCP_EVT */

    if ((event_id & BT_SINK_MODULE_EVENT_MASK) == BT_SINK_MODULE_AVRCP_EVENT)
        APPS_LOG_MSGID_I(LOG_TAG"Srv_event_callback :%x %s", 2, event_id, bt_sink_event_parser(event_id, param));

    if (NULL != param) {
        /* Calculate the total length, it's 4 bytes align. */
        size_t total_len = (((param_len + 3) >> 2) << 2);
        event_params = pvPortMalloc(total_len);
        if (NULL != event_params) {
            /* Copy the data into event_params from the data of callback. */
            memcpy(event_params, param, param_len);
        } else {
            APPS_LOG_MSGID_I("malloc fail", 0);
        }
    }

    /* If there is a pointer in event_param, should malloc a buffer to keep the data. */
    _app_bt_sink_srv_special_malloc(event_id, event_params);

    ui_shell_send_event(false, EVENT_PRIORITY_MIDDLE, EVENT_GROUP_UI_SHELL_BT_SINK, event_id, event_params,
                        param_len, _app_bt_sink_srv_special_free, 0);
}

/**
 * @brief      The implementation of the weak symbol bt_cm_event_callback which declared in bt_connection_manager.h.
 */
void bt_cm_event_callback(bt_cm_event_t event_id, void *params, uint32_t params_len)
{
    void *event_params = NULL;
    //APPS_LOG_MSGID_I(LOG_TAG"bt_cm_event_callback() id: %x", 1, event_id);

    if (NULL != params && params_len) {
        event_params = pvPortMalloc(params_len);
        if (event_params) {
            memcpy(event_params, params, params_len);
        } else {
            APPS_LOG_MSGID_I("malloc fail", 0);
            return;
        }
    }

    ui_shell_send_event(false, EVENT_PRIORITY_MIDDLE, EVENT_GROUP_UI_SHELL_BT_CONN_MANAGER, event_id, event_params,
                        params_len, NULL, 0);
}

/**
 * @brief      The implementation of the bt event callback, refer to bt_callback_manager.h.
 * @param[in]  msg, the message type of the callback event.
 * @param[in]  status, the status, refer to bt_status_t.
 * @param[in]  buffer, the data buffer.
 */
static void registered_bt_event_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
{
    apps_bt_event_data_t *bt_data = NULL;
    if (BT_GAP_LE_ADVERTISING_REPORT_IND == msg || BT_GAP_LE_EXT_ADVERTISING_REPORT_IND == msg) {
        // Because the BT module sends a large amounts of the messages, must ignore it here. If customer want to receive the message, must check it here.
    } else {
        uint32_t buffer_size;
        switch (msg) {
            /* This is an example. If the user wants to use the buffer in app, the user must copy the buffer to a heap memory; */
            case BT_GAP_LE_DISCONNECT_IND:
                buffer_size = sizeof(bt_gap_le_disconnect_ind_t);
                break;
            case BT_GAP_LE_CONNECT_IND:
                buffer_size = sizeof(bt_gap_le_connection_ind_t);
                break;
            case BT_GAP_LE_RPA_ROTAION_IND:
                buffer_size = sizeof(bt_gap_le_rpa_rotation_ind_t);
                break;
            default:
                buffer_size = 0;
                break;
        }
        /* To decrease the times of calling malloc, append buffer after bt_data */
        bt_data = (apps_bt_event_data_t *)pvPortMalloc(sizeof(apps_bt_event_data_t) + buffer_size);
        if (bt_data) {
            bt_data->status = status;
            bt_data->buffer = NULL;
            if (buffer_size) {
                memcpy(((uint8_t *)bt_data) + sizeof(apps_bt_event_data_t), buffer, buffer_size);
                bt_data->buffer = ((uint8_t *)bt_data) + sizeof(apps_bt_event_data_t);
            }
            ui_shell_send_event(false, EVENT_PRIORITY_MIDDLE, EVENT_GROUP_UI_SHELL_BT,
                                msg, bt_data, sizeof(apps_bt_event_data_t), NULL, 0);
        }

    }
}

void apps_events_bt_event_init(void)
{
    /* Only care a part of events, so the second parameter is not all modules. */
    bt_callback_manager_register_callback(bt_callback_type_app_event,
                                          (uint32_t)(MODULE_MASK_GAP | MODULE_MASK_SYSTEM),
                                          (void *)registered_bt_event_callback);
}

/*void apps_bt_events_le_service_callback(bt_gap_le_srv_event_t event, void *data)
{
    uint32_t data_len = 0;
    void *extra_data = NULL;
    APPS_LOG_MSGID_I(LOG_TAG"apps_bt_events_le_service_callback() event: %x", 1, event);
    switch (event) {
        case BT_GAP_LE_SRV_EVENT_ADV_COMPLETE:
            data_len = sizeof(bt_gap_le_srv_adv_complete_t);
            break;
        case BT_GAP_LE_SRV_EVENT_ADV_CLEARED:
        case BT_GAP_LE_SRV_EVENT_CONN_CLEARED:
        case BT_GAP_LE_SRV_EVENT_BLE_DISABLED:
            data_len = sizeof(bt_gap_le_srv_common_result_t);
            break;
        case BT_GAP_LE_SRV_EVENT_CONN_UPDATED:
            data_len = sizeof(bt_gap_le_srv_conn_update_t);
            break;
        default:
            return;
    }
    extra_data = pvPortMalloc(data_len);
    if (extra_data) {
        memcpy(extra_data, data, data_len);
    } else {
        return;
    }
    ui_shell_send_event(false, EVENT_PRIORITY_MIDDLE, EVENT_GROUP_UI_SHELL_LE_SERVICE,
                        event,
                        extra_data,
                        data_len,
                        NULL, 0);
}*/
