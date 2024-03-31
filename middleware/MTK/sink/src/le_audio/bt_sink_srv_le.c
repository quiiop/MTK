/* Copyright Statement:
 *
 * (C) 2020  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */


#include "bt_sink_srv_le.h"
#include "bt_sink_srv_le_cap.h"
#include "bt_sink_srv_le_cap_stream.h"
#include "bt_sink_srv_music.h"
#include "bt_le_audio_sink.h"
//#ifdef AIR_BT_CODEC_BLE_ENABLED
#include "bt_gap_le_audio.h"
//#include "atci.h" //MTK_COMMON
#include "bt_sink_srv_state_notify.h"

#ifdef MTK_COMMON_LE_VCP_EVT
#include "bt_sink_srv_le_volume.h"
#endif


static void le_sink_srv_event_callback(uint16_t event_id, void *p_msg);
bt_sink_srv_state_t g_le_sink_srv_state;

#ifdef AIR_LE_AUDIO_CIS_ENABLE



static void le_sink_srv_mcp_state_change_notify(ble_mcs_media_state_t state);
static void le_sink_srv_state_change_notify(bt_sink_srv_state_t previous, bt_sink_srv_state_t now);
static bt_sink_srv_state_t le_sink_srv_state_transfer_to_bt_state(uint16_t module, bt_sink_srv_cap_state le_state);
static bt_sink_srv_state_t le_sink_srv_call_state_mapping(bt_handle_t handle, bt_le_audio_sink_call_list_t *p_call_noti);
le_sink_srv_context_t g_le_sink_srv_cntx[CAP_UNICAST_DEVICE_NUM];
bt_le_sink_srv_music_active_handle g_music_active_handle;
bt_le_sink_srv_call_active_handle g_call_active_handle;
static bt_le_sink_srv_call_flag_info_t g_le_sink_srv_call_flag_info = {0};
static uint8_t g_le_sink_srv_sync_pa_retry_count = 0;

bool le_sink_srv_get_call_flag(bt_le_sink_srv_call_flag_t **call_flag)
{
    bt_le_sink_srv_call_flag_t *n = (bt_le_sink_srv_call_flag_t *)&g_le_sink_srv_call_flag_info.head;
    uint8_t i = 0;

    while (1)
    {
        n = n->next;

        if (!n) return FALSE;

        call_flag[i++] = n;

        if (i == 2) return TRUE;
    }

    return FALSE;
}

static void le_sink_srv_update_call_flag(bt_handle_t conn_hdl, uint8_t link_idx, bool check)
{
    bt_le_sink_srv_call_flag_t *n = (bt_le_sink_srv_call_flag_t *)&g_le_sink_srv_call_flag_info.head;
    bt_le_sink_srv_call_flag_t *m = &g_le_sink_srv_call_flag_info.flag[link_idx];
    uint8_t idx;

    if (check)
    {
        m->incoming = bt_le_audio_sink_call_check_state(conn_hdl, 0xFF, BLE_TBS_STATE_INCOMING);
        m->outgoing = bt_le_audio_sink_call_check_state(conn_hdl, 0xFF, BLE_TBS_STATE_ALERTING);
        m->active   = bt_le_audio_sink_call_check_state(conn_hdl, 0xFF, BLE_TBS_STATE_ACTIVE);
        m->hold     = bt_le_audio_sink_call_check_state(conn_hdl, 0xFF, BLE_TBS_STATE_LOCALLY_HELD);

        if (!m->outgoing) m->outgoing = bt_le_audio_sink_call_check_state(conn_hdl, 0xFF, BLE_TBS_STATE_DIALING);
    }

    idx = m->incoming || m->outgoing || m->active || m->hold;

    bt_sink_srv_report_id("[Sink] update_call_flag link_idx = %d incoming:%d, outgoing:%d, active:%d, hold:%d", 5, link_idx, m->incoming, m->outgoing, m->active, m->hold);

    if (!m->in && idx)
    {
        while (1)
        {
            if (!n->next)
            {
                n->next = m;
                m->next = 0;
                m->in = 1;
                break;
            }

            n = n->next;
        }
    }
    else if (m->in && !idx)
    {
        bt_le_sink_srv_call_flag_t *pre;

        while (1)
        {
            pre = n;
            n = n->next;

            if (!n) break;

            if (n == m)
            {
                pre->next = m->next;
                m->next = 0;
                m->in = 0;
                break;
            }

        }
    }
}

static void le_sink_srv_update_call_state_flag(uint16_t event_id, void *p_msg)
{
    bool showMsg = true; //MTK_COMMON: David - add for prevent dummy log

    switch (event_id)
    {
        case BT_LE_AUDIO_SINK_EVENT_DISCONNECTED:
        {
            bt_le_audio_sink_event_disconnected_t *noti = (bt_le_audio_sink_event_disconnected_t *)p_msg;
            uint8_t link_idx = bt_sink_srv_cap_get_link_index(noti->handle);
            bt_le_sink_srv_call_flag_t *m;

            if (link_idx >= CAP_UNICAST_DEVICE_NUM) return;

            m = &g_le_sink_srv_call_flag_info.flag[link_idx];

            //if (!m->used) return;

            m->used = 0;
            m->incoming = 0;
            m->outgoing = 0;
            m->active = 0;
            m->hold = 0;

            le_sink_srv_update_call_flag(noti->handle, link_idx, FALSE);

            break;
        }

        case BT_LE_AUDIO_SINK_EVENT_CALL_SERVICE_READY:
        {
            bt_le_audio_sink_event_service_ready_t *noti = (bt_le_audio_sink_event_service_ready_t *)p_msg;
            uint8_t link_idx = bt_sink_srv_cap_get_link_index(noti->handle);
            bt_le_sink_srv_call_flag_t *m;

            if (link_idx >= CAP_UNICAST_DEVICE_NUM) return;

            m = &g_le_sink_srv_call_flag_info.flag[link_idx];

            if (m->used) return;

            m->used = 1;
            m->link_idx = link_idx;
            m->conn_hdl = noti->handle;

            le_sink_srv_update_call_flag(noti->handle, link_idx, TRUE);

            break;
        }

        case BT_LE_AUDIO_SINK_EVENT_CALL_STATE:
        {
            bt_le_audio_sink_event_call_state_t *noti = (bt_le_audio_sink_event_call_state_t *)p_msg;
            uint8_t link_idx;

            if (NULL == noti) return;

            link_idx = bt_sink_srv_cap_get_link_index(noti->handle);

            if (link_idx >= CAP_UNICAST_DEVICE_NUM) return;

            if (!g_le_sink_srv_call_flag_info.flag[link_idx].used) return;

            le_sink_srv_update_call_flag(noti->handle, link_idx, TRUE);

            break;
        }

        case BT_LE_AUDIO_SINK_EVENT_CALL_LIST:
        {
            bt_le_audio_sink_event_call_list_t *noti = (bt_le_audio_sink_event_call_list_t *)p_msg;
            uint8_t link_idx = bt_sink_srv_cap_get_link_index(noti->handle);

            if (link_idx >= CAP_UNICAST_DEVICE_NUM) return;

            if (!g_le_sink_srv_call_flag_info.flag[link_idx].used) return;

            le_sink_srv_update_call_flag(noti->handle, link_idx, TRUE);

            break;
        }

        case BT_LE_AUDIO_SINK_EVENT_CALL_MISSED_CALL: // miss incoming
        {
            bt_le_audio_sink_event_call_missed_call_t *noti = (bt_le_audio_sink_event_call_missed_call_t *)p_msg;
            uint8_t link_idx = bt_sink_srv_cap_get_link_index(noti->handle);

            if (link_idx >= CAP_UNICAST_DEVICE_NUM) return;

            if (!g_le_sink_srv_call_flag_info.flag[link_idx].used) return;

            le_sink_srv_update_call_flag(noti->handle, link_idx, TRUE);

            break;
        }

        default:
            showMsg = false;
            break;
    }

    if (showMsg == true)
        bt_sink_srv_report_id("[Sink] update_call_state_flag event_id:%x", 1, event_id);
}

le_sink_srv_context_t *le_sink_srv_get_context(bt_handle_t handle)
{
    uint8_t index = bt_sink_srv_cap_get_link_index(handle);
    if (index < CAP_UNICAST_DEVICE_NUM) {
        return &g_le_sink_srv_cntx[index];
    }
    return NULL;
}

bt_sink_srv_action_callback_table_t le_sink_srv_action_callback_table[] = {
    {
        SINK_MODULE_MASK_COMMON | SINK_MODULE_MASK_HFP | SINK_MODULE_MASK_HSP,
        bt_sink_srv_le_call_action_handler
    },
    {
        SINK_MODULE_MASK_COMMON | SINK_MODULE_MASK_A2DP | SINK_MODULE_MASK_AVRCP,
        bt_sink_srv_le_music_action_handler
    },
#if 0
#ifdef MTK_AWS_MCE_ENABLE
    {
        SINK_MODULE_MASK_COMMON | SINK_MODULE_MASK_A2DP | SINK_MODULE_MASK_AVRCP,
        bt_sink_srv_music_aws_a2dp_action_handler
    },
#endif/*MTK_AWS_MCE_ENABLE*/
    {
        SINK_MODULE_MASK_COMMON | SINK_MODULE_MASK_AVRCP,
        bt_sink_srv_music_avrcp_action_handler
    },
    {
        SINK_MODULE_MASK_COMMON | SINK_MODULE_MASK_PBAPC,
        bt_sink_srv_pbapc_action_handler
    },
#ifdef MTK_AWS_MCE_ENABLE
#ifndef MTK_BT_CM_SUPPORT
    {
        SINK_MODULE_MASK_COMMON | SINK_MODULE_MASK_AWS_MCE,
        bt_sink_srv_aws_mce_action_handler
    },
#endif
#endif/*MTK_AWS_MCE_ENABLE*/
#endif
};
//typedef

static void le_sink_srv_update_active_handle(uint16_t event_id, void *p_msg)
{
    bool showMsg = true; //MTK_COMMON: David - add for prevent dummy log
    le_sink_srv_update_call_state_flag(event_id, p_msg);

    switch (event_id) {
        case BT_LE_AUDIO_SINK_EVENT_DISCONNECTED: {
            /*If active handle is equal to disconnected handle, switch to another link*/
            bt_le_audio_sink_event_disconnected_t *noti = (bt_le_audio_sink_event_disconnected_t *)p_msg;
            bt_handle_t check_handle = bt_sink_srv_cap_get_another_connected_link_handle(noti->handle);
            g_music_active_handle.handle = (g_music_active_handle.handle == noti->handle ? check_handle : g_music_active_handle.handle);
            g_call_active_handle.accept_handle = (g_call_active_handle.accept_handle == noti->handle ? check_handle : g_call_active_handle.accept_handle);
            g_call_active_handle.reject_handle = (g_call_active_handle.reject_handle == noti->handle ? check_handle : g_call_active_handle.reject_handle);
            g_call_active_handle.dial_handle = (g_call_active_handle.dial_handle == noti->handle ? check_handle : g_call_active_handle.dial_handle);
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_MEDIA_SERVICE_READY: {
            bt_le_audio_sink_event_service_ready_t *noti = (bt_le_audio_sink_event_service_ready_t *)p_msg;
            bt_handle_t check_handle = bt_sink_srv_cap_get_another_connected_link_handle(noti->handle);
            if (check_handle == BT_HANDLE_INVALID ||
                (check_handle != BT_HANDLE_INVALID && check_handle != bt_sink_srv_cap_get_ble_link_by_streaming_mode(bt_sink_srv_cap_am_get_current_mode()))) {
                //check another link is playing or not
                g_music_active_handle.handle = noti->handle;
            } else
                showMsg = false;
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_CALL_SERVICE_READY: {
            bt_le_audio_sink_event_service_ready_t *noti = (bt_le_audio_sink_event_service_ready_t *)p_msg;
            if (BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(noti->handle, 0xFF, BLE_TBS_STATE_INCOMING) ||
                BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(noti->handle, 0xFF, BLE_TBS_STATE_ALERTING)) {
                g_call_active_handle.accept_handle = noti->handle;
                g_call_active_handle.reject_handle = noti->handle;
            }
            if (BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(noti->handle, 0xFF, BLE_TBS_STATE_ACTIVE)) {
                g_call_active_handle.reject_handle = noti->handle;
            }
            g_call_active_handle.dial_handle = noti->handle;
            break;
        }
        case BT_SINK_SRV_CAP_EVENT_ASE_STATE: {
            bt_sink_srv_cap_event_ase_state_t *noti = (bt_sink_srv_cap_event_ase_state_t *)p_msg;
            if (BT_SINK_SRV_CAP_STATE_ASE_STREAMING_MUSIC == noti->current_state ||
                (BT_SINK_SRV_CAP_STATE_ASE_STREAMING_MUSIC == noti->pre_state && BT_SINK_SRV_CAP_STATE_CONNECTED == noti->current_state)) {
                g_music_active_handle.handle = noti->connect_handle;
            } else
                showMsg = false;
            break;
        }

        case BT_LE_AUDIO_SINK_EVENT_MEDIA_STATE: {
            bt_le_audio_sink_event_media_state_t *noti = (bt_le_audio_sink_event_media_state_t*)p_msg;
            g_music_active_handle.handle = noti->handle;
            break;
        }

        case BT_LE_AUDIO_SINK_EVENT_CALL_STATE: {
            bt_le_audio_sink_event_call_state_t *noti = (bt_le_audio_sink_event_call_state_t *)p_msg;
            if (NULL == noti) {
                break;
            }
            if (noti->cur_state == BLE_TBS_STATE_ACTIVE || noti->cur_state == BLE_TBS_STATE_ALERTING || noti->cur_state == BLE_TBS_STATE_LOCALLY_HELD ||
                noti->cur_state == BLE_TBS_STATE_DIALING || noti->cur_state == BLE_TBS_STATE_REMOTELY_HELD || noti->cur_state == BLE_TBS_STATE_LOCALLY_AND_REMOTELY_HELD) {
                g_call_active_handle.reject_handle = noti->handle;
            } else if (noti->cur_state == BLE_TBS_STATE_INCOMING) {
                g_call_active_handle.accept_handle = noti->handle;
                g_call_active_handle.reject_handle = noti->handle;
            } else
                showMsg = false;
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_CALL_LIST: {
            bt_le_audio_sink_event_call_list_t *noti = (bt_le_audio_sink_event_call_list_t *)p_msg;
            if (noti->call_list.length)
                break;

            /*No calls, check another link is active or not*/
            bt_handle_t check_handle = bt_sink_srv_cap_get_another_connected_link_handle(noti->handle);
            if (bt_le_audio_sink_call_get_first_call_state(check_handle, 0xFF) != NULL) {
                g_call_active_handle.reject_handle = check_handle;
            } else
                showMsg = false;
            if (bt_le_audio_sink_call_check_state(check_handle, 0xFF, BLE_TBS_STATE_INCOMING)) {
                g_call_active_handle.accept_handle = check_handle;
            } else
                showMsg = false;
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_CALL_MISSED_CALL: {
            bt_le_audio_sink_event_call_missed_call_t *noti = (bt_le_audio_sink_event_call_missed_call_t *)p_msg;
            g_call_active_handle.dial_handle = noti->handle;
            break;
        }

        default:
            showMsg = false;
            break;
    }
    //MTK_COMMON: David - reduce log
    if (showMsg == true)
        bt_sink_srv_report_id("[Sink] music:%d, accept:%d, reject:%d, dial:%d", 4, g_music_active_handle.handle,
                              g_call_active_handle.accept_handle, g_call_active_handle.reject_handle, g_call_active_handle.dial_handle);
}

static bt_sink_srv_state_t le_sink_srv_state_transfer_to_bt_state(uint16_t module, bt_sink_srv_cap_state le_state)
{
    bt_sink_srv_state_t state = BT_SINK_SRV_STATE_NONE;

    if (module == BT_SINK_SRV_CAP_EVENT_ASE_STATE) {
        switch (le_state) {
            case BT_SINK_SRV_CAP_STATE_IDLE: {
                state = BT_SINK_SRV_STATE_POWER_ON;
                break;
            }
            case BT_SINK_SRV_CAP_STATE_CONNECTED: {
                state = BT_SINK_SRV_STATE_CONNECTED;
                break;
            }
            case BT_SINK_SRV_CAP_STATE_ASE_STREAMING_MUSIC: {
                state = BT_SINK_SRV_STATE_STREAMING;
                break;
            }
            case BT_SINK_SRV_CAP_STATE_ASE_STREAMING_CALL: {
                state = BT_SINK_SRV_STATE_ACTIVE;
                break;
            }
            default: {
                break;
            }
        }
    } else if (module == BT_LE_AUDIO_SINK_EVENT_CALL_STATE) {
        switch (le_state) {
            case BLE_TBS_STATE_INCOMING: {
                state = BT_SINK_SRV_STATE_INCOMING;
                break;
            }
            case BLE_TBS_STATE_DIALING:
            case BLE_TBS_STATE_ALERTING: {
                state = BT_SINK_SRV_STATE_OUTGOING;
                break;
            }
            case BLE_TBS_STATE_ACTIVE: {
                state = BT_SINK_SRV_STATE_ACTIVE;
                break;
            }
            case BLE_TBS_STATE_LOCALLY_AND_REMOTELY_HELD:
            case BLE_TBS_STATE_REMOTELY_HELD:
            case BLE_TBS_STATE_LOCALLY_HELD: {
                state = BT_SINK_SRV_STATE_HELD_REMAINING;
                break;
            }
            default: {
                break;
            }
        }
    } else if (module == BT_LE_AUDIO_SINK_EVENT_MEDIA_STATE) {
        switch (le_state) {
            case BLE_MCS_MEDIA_STATE_PLAYING: {
                state = BT_SINK_SRV_STATE_STREAMING;
                break;
            }
            case BLE_MCS_MEDIA_STATE_PAUSED:
            case BLE_MCS_MEDIA_STATE_STOPED: {
                state = BT_SINK_SRV_STATE_CONNECTED;
                break;
            }
            default: {
                break;
            }
        }
    }
    return state;
}

static bt_sink_srv_state_t le_sink_srv_call_state_mapping(bt_handle_t handle, bt_le_audio_sink_call_list_t *p_call_list)
{
    bt_sink_srv_state_t state = BT_SINK_SRV_STATE_NONE;

    if (handle == BT_HANDLE_INVALID || p_call_list == NULL)
    {
        return BT_SINK_SRV_STATE_NONE;
    }

    // Single Call
    if (p_call_list->length == p_call_list->current_calls->item_length + 1)
    {
        ble_ccp_call_state_t *first_call = bt_le_audio_sink_call_get_first_call_state(handle, 0xFF);

        if (first_call != NULL)
        {
            bt_sink_srv_report_id("[Sink] le_sink_srv_call_state_mapping, Single call:%d, state:%d", 2, first_call->call_index, first_call->state);
        }
        else
        {
            bt_sink_srv_report_id("[Sink] le_sink_srv_call_state_mapping, first call is NULL!", 0);
        }

        if(bt_le_audio_sink_call_check_state(handle, 0xFF, BLE_TBS_STATE_INCOMING))
        {
            state = BT_SINK_SRV_STATE_INCOMING;
        }
        else if (bt_le_audio_sink_call_check_state(handle, 0xFF, BLE_TBS_STATE_ALERTING))
        {
            state = BT_SINK_SRV_STATE_OUTGOING;
        }
        else if (bt_le_audio_sink_call_check_state(handle, 0xFF, BLE_TBS_STATE_ACTIVE))
        {
            state = BT_SINK_SRV_STATE_ACTIVE;
        }
        else if (first_call != NULL && first_call->state != BLE_TBS_STATE_DIALING)
        {
            /*Held call*/
            state = BT_SINK_SRV_STATE_HELD_REMAINING;
        }
    }
    // Multi Call
    // Three way call priority incoming > outgoing > active
    else
    {
        bt_sink_srv_report_id("[Sink] le_sink_srv_call_state_mapping, Multi call", 0);

        if(bt_le_audio_sink_call_check_state(handle, 0xFF, BLE_TBS_STATE_INCOMING))
        {
            state = BT_SINK_SRV_STATE_TWC_INCOMING;
        }
        else if (bt_le_audio_sink_call_check_state(handle, 0xFF, BLE_TBS_STATE_ALERTING))
        {
            state = BT_SINK_SRV_STATE_TWC_OUTGOING;
        }
        else if (bt_le_audio_sink_call_check_state(handle, 0xFF, BLE_TBS_STATE_ACTIVE))
        {
            state = BT_SINK_SRV_STATE_HELD_ACTIVE;
        }
        else
        {
            state = BT_SINK_SRV_STATE_HELD_REMAINING;
        }
    }

    bt_sink_srv_report_id("[Sink] le_sink_srv_call_state_mapping, handle:0x%x, sink_state:%x", 2, handle, state);

    return state;
}

bt_status_t le_sink_srv_send_action(uint32_t action, void *params)
{
    bt_status_t result = BT_STATUS_FAIL;
    uint32_t index;
    uint32_t action_module = (action & 0xF8F00000);
    bt_sink_srv_report_id("[Sink]le_sink_srv_send_action, action: 0x%x", 1, action);

    if (BT_SINK_SRV_ACTION_PROFILE_INIT == action) {
        // return bt_sink_srv_cap_init(le_sink_srv_event_callback, 1);
    }

    if (BT_MODULE_CUSTOM_SINK == action_module) {
        bt_sink_module_mask_t module_mask = SINK_MODULE_MASK_OFFSET(action);
        //(MTK_COMMON: Reduce log)bt_sink_srv_report_id("[Sink]le_sink_srv_send_action, module mask: 0x%x", 1, module_mask);
        for (index = 0; index < sizeof(le_sink_srv_action_callback_table) / sizeof(bt_sink_srv_action_callback_table_t); index++) {
            if ((le_sink_srv_action_callback_table[index].module & module_mask)
                && le_sink_srv_action_callback_table[index].callback) {
                result = le_sink_srv_action_callback_table[index].callback(action, params);
                //(MTK_COMMON: Reduce log)bt_sink_srv_report_id("[Sink]le_sink_srv_send_action, result: 0x%x", 1, result);
                if (result == BT_STATUS_SUCCESS) {
                    return result;
                }
            }
        }
    }
    return result;
}

static void le_sink_srv_profile_status_notify(bt_sink_srv_profile_type_t profile_type, bt_handle_t handle,
                                              bt_sink_srv_profile_connection_state_t state, bt_status_t status)
{
    bt_addr_t dev_addr = bt_sink_srv_cap_get_peer_bdaddr(bt_sink_srv_cap_get_link_index(handle));
    le_sink_srv_context_t *le_sink_cntx = (le_sink_srv_context_t *)le_sink_srv_get_context(handle);
    //le_sink_srv_music_device_t *dev = le_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_A2DP, &dev_addr);
    bt_sink_srv_report_id("[Sink] profile_type: 0x%x, state: %x", 2, profile_type, state); //MTK_COMMON: David - reduce log
    if (le_sink_cntx == NULL) {
        return;
    }
    switch (state) {
        case BT_SINK_SRV_PROFILE_CONNECTION_STATE_CONNECTED: {
            //if (le_sink_cntx->state == BT_BLE_LINK_CONNECTED) {
            // bt_sink_srv_report_id("[Sink]status_notify, wrong state", 0);
            //bt_sink_srv_assert(0);
            //} else {


            le_sink_cntx->dev_addr = &dev_addr;

            if (profile_type == BT_SINK_SRV_PROFILE_A2DP_SINK) {
                le_sink_cntx->music_state = BT_SINK_SRV_STATE_CONNECTED;
            }
            //}
            bt_le_link_state_t old_state = le_sink_cntx->state;
            le_sink_cntx->state = BT_BLE_LINK_CONNECTED;
            bt_le_service_mask_t pre_connected_service = le_sink_cntx->conn_mask;
            bt_sink_srv_report_id("[Sink]srv_profile_status_notify, profile_type: 0x%x, conn_mask: %x", 2, profile_type, le_sink_cntx->conn_mask);
            le_sink_cntx->conn_mask |= profile_type;

            bt_le_sink_srv_event_remote_info_update_t update_ind;
            memcpy(&(update_ind.address), &(dev_addr), sizeof(bt_addr_t));
            update_ind.reason = BT_STATUS_SUCCESS;
            update_ind.pre_state = old_state;
            update_ind.state = BT_BLE_LINK_CONNECTED;
            update_ind.pre_connected_service = pre_connected_service;
            update_ind.connected_service = le_sink_cntx->conn_mask;
            bt_sink_srv_event_callback(LE_SINK_SRV_EVENT_REMOTE_INFO_UPDATE, &update_ind, sizeof(bt_le_sink_srv_event_remote_info_update_t));
            break;
        }
        case BT_SINK_SRV_PROFILE_CONNECTION_STATE_DISCONNECTED: {
            bt_le_link_state_t old_state = le_sink_cntx->state;
            //bt_sink_srv_report_id("[Sink]old_state:%x", 1, old_state); //MTK_COMMON: David - reduce log
            le_sink_cntx->state = BT_BLE_LINK_DISCONNECTED;
            bt_le_service_mask_t pre_connected_service = le_sink_cntx->conn_mask;
            le_sink_cntx->conn_mask &= (~profile_type);

            bt_le_sink_srv_event_remote_info_update_t update_ind;
            memcpy(&(update_ind.address), &(dev_addr), sizeof(bt_addr_t));
#if 1 //MTK_COMMON: To support reason code update
            update_ind.reason = status;
#else
            update_ind.reason = BT_STATUS_SUCCESS;
#endif
            update_ind.pre_state = old_state;
            update_ind.state = BT_BLE_LINK_DISCONNECTED;
            update_ind.pre_connected_service = pre_connected_service;
            update_ind.connected_service = le_sink_cntx->conn_mask;
            bt_sink_srv_event_callback(LE_SINK_SRV_EVENT_REMOTE_INFO_UPDATE, &update_ind, sizeof(bt_le_sink_srv_event_remote_info_update_t));
            break;
        }
        default: {
            break;
        }
    }
}

#ifdef MTK_COMMON_LE_MCP_EVT
void le_sink_srv_mcp_event_ind_notify(bt_msg_type_t msg, bt_status_t status, void *param)
{
    bt_sink_srv_event_mcp_ind_t ind;
    ind.event_type = msg;
    ind.status = status;
    if (param) {
        ind.is_param_valid = true;
        memcpy(&ind.param, param, sizeof(bt_sink_srv_event_mcp_ind_param_t));
    } else
        ind.is_param_valid = false;

    bt_sink_srv_event_callback(BT_SINK_SRV_EVENT_LE_MCP_EVENT_IND,
                               &ind, sizeof(bt_sink_srv_event_mcp_ind_t));
}
#endif

static void le_sink_srv_mcp_state_change_notify(ble_mcs_media_state_t state)
{
#ifdef MTK_COMMON_LE_MCP_EVT
    bt_sink_srv_event_param_t params;

    bt_addr_t dev_addr = bt_sink_srv_cap_get_peer_bdaddr(0xFF);

    bt_sink_srv_memcpy(&params.mcp_status_change.address, &dev_addr,sizeof(bt_bd_addr_t));
    params.mcp_status_change.mcp_status = state;
    bt_sink_srv_event_callback(BT_SINK_SRV_EVENT_LE_MCP_STATUS_CHANGE,
                               &params, sizeof(bt_sink_srv_event_param_t));
#else
    bt_avrcp_status_t avrcp_statre = BT_AVRCP_STATUS_PLAY_ERROR;
    bt_addr_t dev_addr = bt_sink_srv_cap_get_peer_bdaddr(0xFF);
    switch (state) {
        case BLE_MCS_MEDIA_STATE_STOPED:
            avrcp_statre = BT_AVRCP_STATUS_PLAY_STOPPED;
            break;
        case BLE_MCS_MEDIA_STATE_PLAYING:
            avrcp_statre = BT_AVRCP_STATUS_PLAY_PLAYING;
            break;
        case BLE_MCS_MEDIA_STATE_PAUSED:
            avrcp_statre = BT_AVRCP_STATUS_PLAY_PAUSED;
            break;
        default:
            return;
    }
    bt_sink_srv_music_avrcp_status_change_notify(&dev_addr.addr, avrcp_statre);
#endif
}

static void le_sink_srv_state_change_notify(bt_sink_srv_state_t previous, bt_sink_srv_state_t now)
{
#ifdef MTK_COMMON_LE_STATUS_EVT
    bt_sink_srv_event_param_t *params = bt_sink_srv_memory_alloc(sizeof(bt_sink_srv_event_param_t));
    params->state_change.previous = previous;
    params->state_change.current = now;
    bt_sink_srv_event_callback(LE_SINK_SRV_EVENT_STATE_CHANGE, params, sizeof(bt_sink_srv_event_param_t));
    bt_sink_srv_memory_free(params);
    g_le_sink_srv_state = now;
#else
    bt_sink_srv_map_new_state_notify(now, BT_SINK_SRV_STATE_LE_AUDIO_TYPE);
#endif
}
#endif

static void le_sink_srv_event_callback(uint16_t event_id, void *p_msg)
{
    //(MTK_Titan: reduce log)bt_sink_srv_report_id("[Sink] le_sink_srv_event_callback, event_id: 0x%x", 1, event_id);

    le_sink_srv_update_active_handle(event_id, p_msg);

    switch (event_id) {
#ifdef AIR_LE_AUDIO_CIS_ENABLE
        case BT_LE_AUDIO_SINK_EVENT_CONNECTED: {
            bt_le_audio_sink_event_connected_t *noti = (bt_le_audio_sink_event_connected_t *)p_msg;
            bt_hci_le_set_periodic_advrtising_sync_transfer_params_t params = {0};
            params.handle = noti->handle;
            params.past_params.mode = 0x02;
            params.past_params.sync_timeout = 360;
            bt_gap_le_set_periodic_advertising_sync_transfer_parameters(&params);
            bt_sink_srv_cap_set_link(noti->handle);
#ifdef MTK_COMMON_LE_VCP_EVT
            bt_sink_srv_le_volume_init_am();
#endif
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_DISCONNECTED: {
            bt_le_audio_sink_event_disconnected_t *noti = (bt_le_audio_sink_event_disconnected_t *)p_msg;
#if 1 //MTK_COMMON: To support reason code update
            bt_sink_srv_cap_clear_link(noti->handle, noti->reason);
#else
            bt_sink_srv_cap_clear_link(noti->handle);
#endif
            break;
        }
        case BT_SINK_SRV_CAP_EVENT_ASE_STATE: {
            bt_sink_srv_cap_event_ase_state_t *noti = (bt_sink_srv_cap_event_ase_state_t *)p_msg;
            if (noti) {
                le_sink_srv_context_t *le_sink_cntx = (le_sink_srv_context_t *)le_sink_srv_get_context(noti->connect_handle);
                if (le_sink_cntx == NULL) {
                    break;
                }
                uint16_t le_call_state = 0;
                le_sink_cntx->music_state = le_sink_srv_state_transfer_to_bt_state(BT_SINK_SRV_CAP_EVENT_ASE_STATE, noti->current_state);
                bt_sink_srv_state_t pre_state = le_sink_srv_state_transfer_to_bt_state(BT_SINK_SRV_CAP_EVENT_ASE_STATE, noti->pre_state);
                bt_sink_srv_state_t cur_state = le_sink_srv_state_transfer_to_bt_state(BT_SINK_SRV_CAP_EVENT_ASE_STATE, noti->current_state);
                bt_sink_srv_report_id("[Sink] streaming state, pre_state:%x, cur_state:%x", 2, pre_state, cur_state);
                bt_sink_srv_state_t sink_state = bt_sink_srv_get_state();
                if (BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(noti->connect_handle, BLE_CCP_GTBS_INDEX, BLE_TBS_STATE_INCOMING)) {
                    le_call_state = BT_SINK_SRV_STATE_INCOMING;
                }
                bt_sink_srv_report_id("[Sink] get_state: state:%x, le_state:%x", 2, sink_state, le_call_state); //MTK_COMMON: David - reduce log

                if (pre_state == BT_SINK_SRV_STATE_POWER_ON && cur_state == BT_SINK_SRV_STATE_CONNECTED) {
                    break;/* LE conn no need notify */
                }

                if (pre_state >= BT_SINK_SRV_STATE_CONNECTED && cur_state == BT_SINK_SRV_STATE_POWER_ON) {/*LE DISCONNECTED*/
                    bool state_change = true;

                    /* check current device is call incoming */
                    if (BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(noti->connect_handle, BLE_CCP_GTBS_INDEX, BLE_TBS_STATE_INCOMING)) {
                        state_change = false;
                    }

                    /* check another device is call incoming/active */
                    if (sink_state >= BT_SINK_SRV_STATE_STREAMING && sink_state <= BT_SINK_SRV_STATE_MULTIPARTY) {
                        /* EDR music is playing or Call is doing*/
                        state_change = false;
                    }

                    bt_handle_t check_handle = bt_sink_srv_cap_get_link_handle((bt_sink_srv_cap_get_link_index(noti->connect_handle) + 1) % CAP_UNICAST_DEVICE_NUM);
                    if (BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(check_handle, BLE_CCP_GTBS_INDEX, BLE_TBS_STATE_INCOMING) || bt_le_audio_sink_call_check_state(check_handle, 0xFF, BLE_TBS_STATE_ACTIVE)) {
                        state_change = false;
                    }

                    if (state_change) {
                        pre_state = g_le_sink_srv_state;
                        le_sink_srv_state_change_notify(pre_state, cur_state);
                    }
#if 1 //MTK_COMMON: To support reason code update
                    le_sink_srv_profile_status_notify((BT_SINK_SRV_PROFILE_A2DP_SINK | BT_SINK_SRV_PROFILE_HFP),
                                                      noti->connect_handle,
                                                      BT_SINK_SRV_PROFILE_CONNECTION_STATE_DISCONNECTED,
                                                      noti->reason);
#else
                    le_sink_srv_profile_status_notify((BT_SINK_SRV_PROFILE_A2DP_SINK | BT_SINK_SRV_PROFILE_HFP), noti->connect_handle, BT_SINK_SRV_PROFILE_CONNECTION_STATE_DISCONNECTED, BT_STATUS_SUCCESS);
#endif
                }

            }
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_MEDIA_SERVICE_READY: {
            bt_le_audio_sink_event_service_ready_t *noti = (bt_le_audio_sink_event_service_ready_t *)p_msg;
            if (BT_STATUS_SUCCESS == noti->status) {
                le_sink_srv_profile_status_notify(BT_SINK_SRV_PROFILE_A2DP_SINK, noti->handle, BT_SINK_SRV_PROFILE_CONNECTION_STATE_CONNECTED, BT_STATUS_SUCCESS);
            }
#if 0//MTK_COMMON: remvoed due to not used code
            bt_sink_srv_report_id("Media config complete! handle:0x%04x", 1, noti->handle);
            atci_response_t *response = NULL;
            response = (atci_response_t*)bt_sink_srv_memory_alloc(sizeof(atci_response_t));
            if (NULL != response) {
                memset(response, 0, sizeof(atci_response_t));
                snprintf ((char *)response->response_buf, sizeof(response->response_buf), "Media config complete! handle:0x%04x\r\n",noti->handle);
                response->response_len = strlen((char *)response->response_buf);
                response->response_flag = 0 | ATCI_RESPONSE_FLAG_URC_FORMAT;
                atci_send_response(response);
                bt_sink_srv_memory_free(response);
            }
#endif
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_CALL_SERVICE_READY: {
            bt_le_audio_sink_event_service_ready_t *noti = (bt_le_audio_sink_event_service_ready_t *)p_msg;
            if (BT_STATUS_SUCCESS == noti->status) {
                le_sink_srv_profile_status_notify(BT_SINK_SRV_PROFILE_HFP, noti->handle, BT_SINK_SRV_PROFILE_CONNECTION_STATE_CONNECTED, BT_STATUS_SUCCESS);
            }
            bt_sink_srv_report_id("Call config complete! handle:0x%04x", 1, noti->handle);
#if 0//MTK_COMMON: remvoed due to not used code
            atci_response_t *response = NULL;
            response = (atci_response_t*)bt_sink_srv_memory_alloc(sizeof(atci_response_t));
            if (NULL != response) {
                memset(response, 0, sizeof(atci_response_t));
                snprintf ((char *)response->response_buf, sizeof(response->response_buf), "Call config complete! handle:0x%04x\r\n",noti->handle);
                response->response_len = strlen((char *)response->response_buf);
                response->response_flag = 0 | ATCI_RESPONSE_FLAG_URC_FORMAT;
                atci_send_response(response);
                bt_sink_srv_memory_free(response);
            }
#endif
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_MEDIA_STATE: {
            bt_le_audio_sink_event_media_state_t *noti = (bt_le_audio_sink_event_media_state_t *)p_msg;
            bt_sink_srv_report_id("[Sink] mcp state %d", 1, noti->state);

            if (noti->state == BLE_MCS_MEDIA_STATE_PLAYING) {
                bt_sink_srv_cap_stream_start_unicast_streaming(noti->handle);
            }
#if 0 //inform streaming state from AM play/stop
            bt_sink_srv_state_t sink_state = bt_sink_srv_get_state();
            uint16_t le_call_state = 0;
            if (BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(noti->handle, BLE_CCP_GTBS_INDEX, BLE_TBS_STATE_INCOMING)) {
                 le_call_state = BT_SINK_SRV_STATE_INCOMING;
            } else if (BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(noti->handle, BLE_CCP_GTBS_INDEX, BLE_TBS_STATE_ACTIVE)) {
                le_call_state = BT_SINK_SRV_STATE_ACTIVE;
            }

            bt_sink_srv_report_id("[Sink] le_bt_sink_srv_get_state: state:%x, le_state:%x", 2, sink_state, le_call_state);

            if (sink_state >= BT_SINK_SRV_STATE_STREAMING && sink_state <= BT_SINK_SRV_STATE_MULTIPARTY) {
                break; /* EDR music is playing or Call is doing*/
            }

            if (le_call_state) {
                break; /* LE Music is interrupted by LE Call */
            }

            bt_sink_srv_state_t cur_state = le_sink_srv_state_transfer_to_bt_state(BT_LE_AUDIO_SINK_EVENT_MEDIA_STATE, noti->state);
            bt_sink_srv_state_t pre_state = BT_SINK_SRV_STATE_NONE;

            if (BLE_MCS_MEDIA_STATE_PLAYING == noti->state) {
                pre_state = BT_SINK_SRV_STATE_CONNECTED;
            } else {
                pre_state = BT_SINK_SRV_STATE_STREAMING;
            }
            bt_sink_srv_report_id("[Sink] mcp state change: pre_state:%x, cur_state:%x", 2, pre_state, cur_state);
            le_sink_srv_state_change_notify(pre_state, cur_state);
#endif
            le_sink_srv_mcp_state_change_notify(noti->state);
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_CALL_STATE: {
            bt_le_audio_sink_event_call_state_t *call_noti = (bt_le_audio_sink_event_call_state_t *)p_msg;
            if (call_noti && (call_noti->cur_state != BLE_TBS_STATE_IDLE && call_noti->cur_state != BLE_TBS_STATE_INVALID)) {
                //TO DO
                bt_sink_srv_report_id("[Sink] call state, call pre_state: %x, cur_state:%x", 2,
                                    le_sink_srv_state_transfer_to_bt_state(BT_LE_AUDIO_SINK_EVENT_CALL_STATE, call_noti->prev_state),
                                    le_sink_srv_state_transfer_to_bt_state(BT_LE_AUDIO_SINK_EVENT_CALL_STATE, call_noti->cur_state));
                //le_sink_srv_state_change_notify(pre_state, cur_state);
            }
            break;
        }

        case BT_LE_AUDIO_SINK_EVENT_CALL_LIST: {
            bt_le_audio_sink_event_call_list_t *noti = (bt_le_audio_sink_event_call_list_t *)p_msg;
            bt_sink_srv_cap_am_mode mode = bt_sink_srv_cap_am_get_current_mode();
            //No call
            if (!noti->call_list.length) {
                /*No calls, check another device is music playing or call incoming/active*/
                bt_sink_srv_state_t pre_state = BT_SINK_SRV_STATE_ACTIVE;
                bt_sink_srv_state_t cur_state = BT_SINK_SRV_STATE_CONNECTED;
                bt_handle_t check_handle = bt_sink_srv_cap_get_link_handle((bt_sink_srv_cap_get_link_index(noti->handle) + 1) % CAP_UNICAST_DEVICE_NUM);
                bt_sink_srv_state_t sink_state = bt_sink_srv_get_state();

                if (sink_state >= BT_SINK_SRV_STATE_STREAMING && sink_state <= BT_SINK_SRV_STATE_MULTIPARTY) {
                    /* EDR music is playing or Call is doing*/
                    cur_state = sink_state;
                } else if (BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(check_handle, BLE_CCP_GTBS_INDEX, BLE_TBS_STATE_INCOMING)) {
                    cur_state = BT_SINK_SRV_STATE_INCOMING;
                } else if (BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(check_handle, BLE_CCP_GTBS_INDEX, BLE_TBS_STATE_ACTIVE)) {
                    cur_state = BT_SINK_SRV_STATE_ACTIVE;
                } else if (CAP_AM_UNICAST_MUSIC_MODE_START <= mode && mode <= CAP_MAX_DEVICE_NUM) {
                    cur_state = BT_SINK_SRV_STATE_STREAMING;
                }
                bt_sink_srv_report_id("[Sink] Empty LE call list, cur_state:%x", 1, cur_state);
                le_sink_srv_state_change_notify(pre_state, cur_state);
            }
            // Multi call or Still have call
            else
            {
                bt_sink_srv_state_t pre_state = BT_SINK_SRV_STATE_CONNECTED;// Unused
                bt_sink_srv_state_t cur_state = le_sink_srv_call_state_mapping(noti->handle, &noti->call_list);
                if(CAP_AM_UNICAST_MUSIC_MODE_START <= mode && mode <= CAP_MAX_DEVICE_NUM)
                {
                    pre_state = BT_SINK_SRV_STATE_STREAMING;
                }
                if(cur_state != BT_SINK_SRV_STATE_NONE)
                {
                    bt_sink_srv_report_id("[Sink] pre_state:0x%x, cur_state:0x%x", 2, pre_state, cur_state);
                    le_sink_srv_state_change_notify(pre_state, cur_state);
                }

                /*take audio resource if active call exist*/
                if (bt_le_audio_sink_call_check_state(noti->handle, 0xFF, BLE_TBS_STATE_ACTIVE) != BLE_TBS_INVALID_CALL_INDEX) {
                    bt_sink_srv_cap_stream_start_unicast_streaming(noti->handle);
                }
            }
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_CALL_MISSED_CALL: {
            /*bt_sink_srv_state_t pre_state = BT_SINK_SRV_STATE_INCOMING;
            bt_sink_srv_state_t cur_state = BT_SINK_SRV_STATE_CONNECTED;
            le_sink_srv_state_change_notify(pre_state, cur_state);*/
            break;
        }
        case BT_LE_AUDIO_SINK_EVENT_MEDIA_RESUME:
        case BT_LE_AUDIO_SINK_EVENT_MEDIA_SUSPEND:
        {
            /*bt_sink_srv_cap_event_media_change_state_t *info = (bt_sink_srv_cap_event_media_change_state_t *)p_msg;
            if (info) {
                bt_sink_srv_le_media_state_change_callback(event_id, info->connect_handle, info->resume);
            }*/
            break;
        }

        case BT_LE_AUDIO_SINK_EVENT_CALL_INCOMING_CALL: {
        #ifdef AIR_BT_SINK_SRV_STATE_MANAGER_ENABLE
            bt_gap_le_srv_conn_info_t *conn_info = NULL;
            bt_le_audio_sink_event_call_incoming_call_t *incoming_call = (bt_le_audio_sink_event_call_incoming_call_t *)p_msg;
            if (NULL != incoming_call) {
                conn_info = bt_gap_le_srv_get_conn_info(incoming_call->handle);
            }
            if (NULL != conn_info) {
                bt_sink_srv_state_manager_notify_ring_ind(BT_SINK_SRV_STATE_MANAGER_DEVICE_TYPE_LE, &conn_info->peer_addr.addr, true);
            }
        #endif
        }
#endif
        /*Broadcast*/
        case BT_SINK_SRV_CAP_EVENT_BASE_BROADCAST_AUDIO_ANNOUNCEMENTS: {
            bt_sink_srv_report_id("[Sink][BIS] Broadcast audio announcements", 0);
            bt_sink_srv_cap_stream_bmr_scan_info_t *scan_info = bt_sink_srv_cap_stream_get_bmr_scan_info();
            if (BT_SINK_SRV_CAP_STREAM_SYNC_POLICY_SELECT_BIS != scan_info->sync_policy) {
                bt_sink_srv_cap_event_base_broadcast_audio_announcements_t *data = (bt_sink_srv_cap_event_base_broadcast_audio_announcements_t *)p_msg;
                bt_sink_srv_cap_stream_broadcast_sync_periodic_advretising(data->addr, data->advertising_sid);
            }
            break;
        }

        case BT_SINK_SRV_CAP_EVENT_BASE_PERIODIC_ADV_SYNC_ESTABLISHED: {
            ble_bap_periodic_adv_sync_established_notify_t *noti = (ble_bap_periodic_adv_sync_established_notify_t *)p_msg;

            if (BT_STATUS_SUCCESS == noti->status) {
                bt_sink_srv_report_id("[Sink][BIS] PA sync established", 0);
                g_le_sink_srv_sync_pa_retry_count = 0;
                return;
            }

            if (BT_HCI_STATUS_CONNECTION_FAILED_TO_BE_ESTABLISHED == noti->status) {
                g_le_sink_srv_sync_pa_retry_count ++;
                if (MAX_PA_SYNC_RETRY_NUM >= g_le_sink_srv_sync_pa_retry_count) {
                    bt_sink_srv_report_id("[Sink][BIS] Fail to sync PA, retry count:%x, max count:%x", 2, g_le_sink_srv_sync_pa_retry_count, MAX_PA_SYNC_RETRY_NUM);
                    bt_sink_srv_cap_stream_broadcast_sync_periodic_advretising(noti->advertiser_addr, noti->advertising_sid);
                    return;
                }
            }

            bt_sink_srv_report_id("[Sink][BIS] Fail to sync PA", 0);
            g_le_sink_srv_sync_pa_retry_count = 0;
            bt_sink_srv_cap_stream_stop_scanning_broadcast_source();
            break;
        }
        case BT_SINK_SRV_CAP_EVENT_BASE_BASIC_AUDIO_ANNOUNCEMENTS: {
            bt_sink_srv_report_id("[Sink][BIS] Basic audio announcements", 0);
            //bt_sink_srv_cap_event_base_basic_audio_announcements_t *data = (bt_sink_srv_cap_event_base_basic_audio_announcements_t *)p_msg;
            break;
        }

        case BT_SINK_SRV_CAP_EVENT_BASE_BIGINFO_ADV_REPORT: {
            /*bt_sink_srv_report_id("[Sink][BIS] BIG info received", 0);
            bt_sink_srv_cap_event_base_biginfo_adv_report_t *data = (bt_sink_srv_cap_event_base_biginfo_adv_report_t *)p_msg;
            uint8_t bis_indices = 0xFF;
            bt_sink_srv_cap_stream_start_broadcast_reception(data->sync_handle, 1, 1, &bis_indices);*/
            break;
        }

        case BT_SINK_SRV_CAP_EVENT_BASE_BIG_SYNC_IND: {
            bt_sink_srv_cap_event_base_big_sync_ind_t *data = (bt_sink_srv_cap_event_base_big_sync_ind_t *)p_msg;
            bt_sink_srv_cap_stream_bmr_scan_info_t *scan_info = bt_sink_srv_cap_stream_get_bmr_scan_info();
            bt_sink_srv_report_id("[Sink] sync_handle:0x%4X, num_bis:%d, bis_indices[0]:%d, sync_policy:%d", 4,
                                  data->sync_handle, data->num_bis, data->bis_indices[0], scan_info->sync_policy);
            if (le_audio_get_device_type() == LE_AUDIO_DEVICE_TYPE_HEADSET && BT_SINK_SRV_CAP_STREAM_SYNC_POLICY_SELECT_BIS != scan_info->sync_policy) {
                if (data->bis_indices[0] == 0) {
                    uint8_t bis_indices[2] = {BIS_INDEX_INVALID, BIS_INDEX_INVALID};
                    /*Checking 1 stereo BIS*/
                    bis_indices[0] = bt_sink_srv_cap_stream_get_bis_index_in_pa_level_1(AUDIO_LOCATION_FRONT_LEFT | AUDIO_LOCATION_FRONT_RIGHT);
                    bt_sink_srv_report_id("[Sink] check stereo BIS index: 0x%x", 1, bis_indices[0]);
                    if (BIS_INDEX_INVALID == bis_indices[0]) {
                        /*Checking 2 mono BIS*/
                        bis_indices[0] = bt_sink_srv_cap_stream_get_bis_index_in_pa_level_1(AUDIO_LOCATION_FRONT_LEFT);
                        bis_indices[1] = bt_sink_srv_cap_stream_get_bis_index_in_pa_level_1(AUDIO_LOCATION_FRONT_RIGHT);
                        bt_sink_srv_report_id("[Sink] check mono BIS index: 0x%x, 0x%x", 2, bis_indices[0], bis_indices[1]);
                        if (BIS_INDEX_INVALID == bis_indices[0] || BIS_INDEX_INVALID == bis_indices[1]) {
                            /*Dual mono is not supported*/
                            bt_sink_srv_report_id("[Sink] Fail to find bis index in PA", 0);
                            break;
                        }
                        bt_sink_srv_cap_stream_start_broadcast_reception(data->sync_handle, 1, 2, bis_indices);
                    } else {
                        bt_sink_srv_cap_stream_start_broadcast_reception(data->sync_handle, 1, 1, bis_indices);
                    }
                } else {
                    /*BSA select BIS index*/
                    bt_sink_srv_cap_stream_start_broadcast_reception(data->sync_handle, 1, data->num_bis, &data->bis_indices[0]);
                }

            } else {
                if (data->num_bis == 1 && data->bis_indices[0] == 0) {
                    uint8_t bis_indices = BIS_INDEX_INVALID;
                    /*No prefered BIS index*/
                    if (BT_SINK_SRV_CAP_STREAM_SYNC_POLICY_SELECT_BIS != scan_info->sync_policy) {
                        bt_sink_srv_cap_stream_start_broadcast_reception(data->sync_handle, 1, 1, &bis_indices);
                    }
                } else {
                    /*BSA select BIS index*/
                    bt_sink_srv_cap_stream_start_broadcast_reception(data->sync_handle, 1, data->num_bis, &data->bis_indices[0]);
                }
            }
            scan_info->sync_policy = BT_SINK_SRV_CAP_STREAM_SYNC_POLICY_NONE;
            break;
        }

        case BT_SINK_SRV_CAP_EVENT_BASE_BIG_SYNC_ESTABLISHED: {
            bt_sink_srv_report_id("[Sink][BIS] BIG sync established", 0);
#if 0 /*No need to report sink srv state*/
            bt_sink_srv_state_t pre_state = BT_SINK_SRV_STATE_POWER_ON;
            bt_sink_srv_state_t cur_state = BT_SINK_SRV_STATE_STREAMING;
            le_sink_srv_state_change_notify(pre_state, cur_state);
#endif
            break;
        }
        case BT_SINK_SRV_CAP_EVENT_BASE_BIG_TERMINATE_IND: {
            bt_sink_srv_report_id("[Sink][BIS] BIG sync terminate ind", 0);
            break;
        }
        case BT_SINK_SRV_CAP_EVENT_BASE_BIG_TERMINATE_CFM: {
            bt_sink_srv_report_id("[Sink][BIS] BIG sync terminate cfm", 0);
#if 0 /*No need to report sink srv state*/
            bt_sink_srv_state_t sink_state = bt_sink_srv_get_state();
            bt_sink_srv_report_id("[Sink]EDR state:%X", sink_state);

            if (sink_state >= BT_SINK_SRV_STATE_STREAMING && sink_state <= BT_SINK_SRV_STATE_MULTIPARTY) {
                break; /* EDR music is playing or Call is doing*/
            }

            for (uint8_t i = 0; i < CAP_UNICAST_DEVICE_NUM; i++) {
                if (BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(bt_sink_srv_cap_get_link_handle(i), 0xFF, BLE_TBS_STATE_INCOMING) ||
                    BLE_TBS_INVALID_CALL_INDEX != bt_le_audio_sink_call_check_state(bt_sink_srv_cap_get_link_handle(i), 0xFF, BLE_TBS_STATE_ACTIVE)) {
                    return;  /* LE call is incoming/active */
                }
            }

            bt_sink_srv_state_t pre_state = BT_SINK_SRV_STATE_STREAMING;
            bt_sink_srv_state_t cur_state = BT_SINK_SRV_STATE_POWER_ON;
            le_sink_srv_state_change_notify(pre_state, cur_state);
#endif
            break;
        }
        default:
            break;
    }
}

void le_sink_srv_init(uint8_t max_link_num)
{
    bt_sink_srv_cap_init(le_sink_srv_event_callback, max_link_num);
}

void le_sink_srv_set_streaming_state(bool is_streaming)
{
    bt_sink_srv_state_t sink_state = bt_sink_srv_get_state();

    bt_sink_srv_report_id("[Sink] le_sink_srv_set_streaming_state: is_streaming:%d, le_state:%x, bt_state:%x", 3, is_streaming, g_le_sink_srv_state, sink_state);
#ifndef AIR_BT_SINK_SRV_STATE_MANAGER_ENABLE
    if (sink_state >= BT_SINK_SRV_STATE_STREAMING && sink_state <= BT_SINK_SRV_STATE_MULTIPARTY) {
        return; /* EDR music is playing or Call is doing*/
    }

    if (g_le_sink_srv_state >= BT_SINK_SRV_STATE_INCOMING && g_le_sink_srv_state <= BT_SINK_SRV_STATE_MULTIPARTY) {
        return; /* LE calling*/
    }
#endif
    bt_sink_srv_state_t pre_state = (is_streaming ? BT_SINK_SRV_STATE_CONNECTED : BT_SINK_SRV_STATE_STREAMING);
    bt_sink_srv_state_t cur_state = (is_streaming ? BT_SINK_SRV_STATE_STREAMING : BT_SINK_SRV_STATE_CONNECTED);

    //MTK_COMMON: David - shrink log
    bt_sink_srv_report_id("[Sink] streaming state change: %x -> %x", 2, pre_state, cur_state);
    le_sink_srv_state_change_notify(pre_state, cur_state);
    //le_sink_srv_mcp_state_change_notify(noti->state);
}

bt_sink_srv_state_t le_sink_srv_get_state(void)
{
    bt_sink_srv_report_id("[Sink][State] get_state:0x%x", 1, g_le_sink_srv_state);

    return g_le_sink_srv_state;
}

//#endif /* #ifdef AIR_BT_CODEC_BLE_ENABLED */


