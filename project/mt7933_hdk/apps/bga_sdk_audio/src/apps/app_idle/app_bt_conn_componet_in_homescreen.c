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

#include "FreeRTOS.h"
#include "timers.h"

#include "apps_debug.h"
#include "apps_customer_config.h"
#include "ui_shell_manager.h"

#include "app_bt_conn_componet_in_homescreen.h"
#include "apps_events_event_group.h"
#include "apps_events_interaction_event.h"
#include "apps_events_bt_event.h"
#include "app_bt_state_service.h"
#include "app_music_utils.h"
#include "apps_config_event_list.h"
#ifdef AIR_LE_AUDIO_ENABLE
#include "app_le_audio.h"
#include "bt_sink_srv_le.h"
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */

//#include "bt_sink_srv.h"
#include "bt_device_manager.h"
#include "bt_connection_manager.h"

#define APP_LOG_TAG "[APP_BT_CONN_Home]"
#define APP_A2DP_SINK_RECON_BASE_TIME 200
// set a prime number, avoid setting the same value as the phone
#define APP_A2DP_SINK_RECON_RANDOM_TIME 311
#define APP_A2DP_SINK_RECON_TIMES 2

static uint8_t g_bt_app_a2dp_sink_recon_cnt;

static void bt_conn_component_a2dp_sink_recon(bt_bd_addr_t *addr)
{
    bt_bd_addr_t *event_params = NULL;
    uint32_t rand_tm = 0;

    if (!addr) {
        APPS_LOG_MSGID_E("%s, BT addr is null", 1, __func__);
        return;
    }

    event_params = (bt_bd_addr_t *)pvPortMalloc(BT_BD_ADDR_LEN);
    if (!event_params) {
        APPS_LOG_MSGID_E("%s, malloc fail", 1, __func__);
        return;
    }

    memcpy(event_params, addr, BT_BD_ADDR_LEN);
    rand_tm = xTaskGetTickCount() % APP_A2DP_SINK_RECON_RANDOM_TIME + APP_A2DP_SINK_RECON_BASE_TIME;
    APPS_LOG_MSGID_I("%s, rand_tm: %ld", 2, __func__, rand_tm);

    ui_shell_send_event(false, EVENT_PRIORITY_MIDDLE, EVENT_GROUP_UI_SHELL_APP_INTERACTION,
                        APPS_EVENTS_INTERACTION_BT_RECONNECT_DEVICE, event_params,
                        BT_BD_ADDR_LEN, NULL, rand_tm);
}

#ifdef AIR_LE_AUDIO_ENABLE
static char *_bt_conn_le_link_state_parser(bt_le_link_state_t status)
{
    switch (status) {
        case BT_BLE_LINK_DISCONNECTED:
            return "Disconnected";
        case BT_BLE_LINK_DISCONNECTING:
            return "Disconnecting";
        case BT_BLE_LINK_CONNECTING:
            return "Connecting";
        case BT_BLE_LINK_CONNECTED:
            return "Connected";
        default:
            return "Unknown";
    }
}
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */

static char *_bt_conn_link_state_parser(bt_cm_acl_link_state_t status)
{
    switch (status) {
        case BT_CM_ACL_LINK_DISCONNECTED:
            return "ACL Disconnected";
        case BT_CM_ACL_LINK_CONNECTED:
            return "ACL Connected Lvl 0";
        case BT_CM_ACL_LINK_ENCRYPTED:
            return "ACL Encrypted Connection";
        default:
            return "Unknown";
    }
}

#ifdef APPS_AUTO_SET_BT_DISCOVERABLE
static void _bt_conn_auto_setup_scan_mode(bool enable)
{
    if (!enable) {
        app_bt_state_service_set_bt_scan_mode(false, false, 0);
        return;
    }

    if (bt_device_manager_get_paired_number() == 0) {
        APPS_LOG_MSGID_D(APP_LOG_TAG", start visible when power on if paired_num is 0", 0);
        app_bt_state_service_set_bt_scan_mode(true, false, APPS_BT_VISIBLE_TIMEOUT);
    } else {
        /* Enable connectable */
        APPS_LOG_MSGID_D(APP_LOG_TAG", start connectable if paired_num > 0", 0);
        app_bt_state_service_set_bt_scan_mode(false, true, 0);
    }
}
#endif /* #ifdef APPS_AUTO_SET_BT_DISCOVERABLE */

bool bt_conn_component_bt_sink_event_proc(ui_shell_activity_t *self, uint32_t event_id, void *extra_data, size_t data_len)
{
    bool ret = true;
    home_screen_local_context_type_t *local_ctx = (home_screen_local_context_type_t *) self->local_context;

    if (!local_ctx)
        return false;

    switch (event_id) {
#ifdef AIR_LE_AUDIO_ENABLE
        case LE_SINK_SRV_EVENT_REMOTE_INFO_UPDATE: {
                bt_le_sink_srv_event_remote_info_update_t *info = (bt_le_sink_srv_event_remote_info_update_t *) extra_data;
                if (info == NULL) {
                    ret = false;
                    break;
                }

                /* Print BT_CM update log. */
                APPS_LOG_MSGID_I(APP_LOG_TAG", LE Link State:%s, Reason:0x%x, Profile:%x->%x, Addr:%02x:%02x:%02x:%02x:%02x:%02x",
                                 10, _bt_conn_le_link_state_parser(info->state), info->reason,
                                 info->pre_connected_service, info->connected_service,
                                 info->address.addr[5], info->address.addr[4], info->address.addr[3],
                                 info->address.addr[2], info->address.addr[1], info->address.addr[0]);

                /* Procedure for Power OFF */
                if (local_ctx->bt_power_state == APP_HOME_SCREEN_BT_POWER_STATE_DISABLING) {
                    app_bt_state_service_set_bt_on_off(false);
                    break;
                } else if (local_ctx->bt_power_state != APP_HOME_SCREEN_BT_POWER_STATE_ENABLED)
                    break;

#ifdef APPS_AUTO_SET_LEA_ADV
                if (info->state == BT_BLE_LINK_DISCONNECTED &&
                    info->reason != BT_HCI_STATUS_REMOTE_USER_TERMINATED_CONNECTION &&
                    info->reason != BT_HCI_STATUS_CONNECTION_TERMINATED_BY_LOCAL_HOST) {
                    app_le_audio_switch_advertising(true);
                }
#endif /* #ifdef APPS_AUTO_SET_LEA_ADV */

#ifdef APPS_AUTO_SET_BT_DISCOVERABLE
                if (info->state == BT_BLE_LINK_DISCONNECTED) {
                    _bt_conn_auto_setup_scan_mode(true);
                } else if (info->state == BT_BLE_LINK_CONNECTED) {
                    _bt_conn_auto_setup_scan_mode(false);
                }
#endif /* #ifdef APPS_AUTO_SET_BT_DISCOVERABLE */
            }
            break;
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */
        default:
            ret = false;
            break;
    }
    return ret;
}

bool bt_conn_component_bt_cm_event_proc(ui_shell_activity_t *self, uint32_t event_id, void *extra_data, size_t data_len)
{
    bool ret = false;
    bool disconnect = false;
#ifdef APPS_AUTO_SET_LEA_ADV
    bool connect = false;
#endif /* #ifdef APPS_AUTO_SET_LEA_ADV */
    bool bt_on = false;
    home_screen_local_context_type_t *local_ctx = (home_screen_local_context_type_t *)self->local_context;

    switch (event_id) {
        case BT_CM_EVENT_REMOTE_INFO_UPDATE: {
                bt_cm_remote_info_update_ind_t *remote_update = (bt_cm_remote_info_update_ind_t *)extra_data;
                if (!remote_update) {
                    break;
                }
                /* Print BT_CM update log. */
                APPS_LOG_MSGID_I(APP_LOG_TAG", Link State:%s, Reason:0x%x, Profile:%s, Addr:%02x:%02x:%02x:%02x:%02x:%02x", 9,
                                 _bt_conn_link_state_parser(remote_update->acl_state),
                                 remote_update->reason,
                                 bt_cm_profile_parser(remote_update->connected_service),
                                 remote_update->address[5], remote_update->address[4], remote_update->address[3],
                                 remote_update->address[2], remote_update->address[1], remote_update->address[0]);

                if (remote_update->acl_state == BT_CM_ACL_LINK_DISCONNECTED) {
                    /* Free AVRCP attribute list when disconnet */
                    app_bt_music_free_attr_list();
                    disconnect = true;
                }
#ifdef APPS_AUTO_SET_LEA_ADV
                else if (remote_update->acl_state == BT_CM_ACL_LINK_CONNECTED ||
                         remote_update->acl_state == BT_CM_ACL_LINK_ENCRYPTED)
                    connect = true;
#endif /* #ifdef APPS_AUTO_SET_LEA_ADV */

                /* Procedure for Power OFF */
                if (local_ctx->bt_power_state == APP_HOME_SCREEN_BT_POWER_STATE_DISABLING) {
                    app_bt_state_service_set_bt_on_off(false);
                    break;
                }

                /* when DUT and phone connect at the same time, phone will return no resource available,
                 * then will do reconnect twice
                 */
                if ((remote_update->acl_state == BT_CM_ACL_LINK_DISCONNECTED) &&
                    (remote_update->update_service == BT_CM_PROFILE_SERVICE_A2DP_SINK) &&
                    (remote_update->reason == (BT_STATUS_L2CAP_REMOTE_NO_RESOURCES & 0xFF)) &&
                    (g_bt_app_a2dp_sink_recon_cnt < APP_A2DP_SINK_RECON_TIMES)) {
                    APPS_LOG_MSGID_I(APP_LOG_TAG", avdtp_con_req fail, no resource, retry(%d/%d)", 2,
                                     g_bt_app_a2dp_sink_recon_cnt + 1, APP_A2DP_SINK_RECON_TIMES);
                    g_bt_app_a2dp_sink_recon_cnt++;
                    bt_conn_component_a2dp_sink_recon(&remote_update->address);
                    return true;
                }
                if (g_bt_app_a2dp_sink_recon_cnt) {
                    g_bt_app_a2dp_sink_recon_cnt = 0;
                    /* maybe peer device reconnect successfullly during waiting timer to reconnect,
                     * so we should remove reconnect event
                     */
                    ui_shell_remove_event(EVENT_GROUP_UI_SHELL_APP_INTERACTION,
                                          APPS_EVENTS_INTERACTION_BT_RECONNECT_DEVICE);
                }
            }
            break;
        case BT_CM_EVENT_VISIBILITY_STATE_UPDATE: {
                bt_cm_visibility_state_update_ind_t *visible_update = (bt_cm_visibility_state_update_ind_t *)extra_data;
                if (NULL == local_ctx || NULL == visible_update) {
                    break;
                }
                APPS_LOG_MSGID_I(APP_LOG_TAG", visibility_state: %d, connectivity_state: %d", 2,
                                 visible_update->visibility_state, visible_update->connectivity_state);

                /* Update BT visibility state for Homescreen APP. */
                local_ctx->is_bt_visiable = visible_update->visibility_state;
                local_ctx->is_bt_connectable = visible_update->connectivity_state;

            }
            break;
        case BT_CM_EVENT_POWER_STATE_UPDATE: {
                bt_cm_power_state_update_ind_t *power_update = (bt_cm_power_state_update_ind_t *)extra_data;
                if (NULL == local_ctx || NULL == power_update) {
                    break;
                }

                if (BT_CM_POWER_STATE_ON == power_update->power_state) {
                    if (APP_HOME_SCREEN_BT_POWER_STATE_ENABLED == local_ctx->bt_power_state) {
                        APPS_LOG_MSGID_W(APP_LOG_TAG", Why get Power ON again??", 0);
                        break;
                    }
                    /* Switch BT state from off to on. */
                    APPS_LOG_MSGID_I(APP_LOG_TAG", Power ON", 0);
                    local_ctx->bt_power_state = APP_HOME_SCREEN_BT_POWER_STATE_ENABLED;
#ifdef AIR_LE_AUDIO_ENABLE
                    app_le_audio_power_on_setup();
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */
                    bt_on = true;
                } else if (BT_CM_POWER_STATE_OFF == power_update->power_state) {
                    if (APP_HOME_SCREEN_BT_POWER_STATE_DISABLED == local_ctx->bt_power_state) {
                        APPS_LOG_MSGID_W(APP_LOG_TAG", Why get Power OFF again??", 0);
                        break;
                    }
                    /* Switch BT state from on to off. */
                    APPS_LOG_MSGID_I(APP_LOG_TAG", Power OFF", 0);
                    local_ctx->is_bt_visiable = false;
                    local_ctx->bt_power_state = APP_HOME_SCREEN_BT_POWER_STATE_DISABLED;
                    local_ctx->connection_state = false;
#ifdef AIR_LE_AUDIO_ENABLE
                    app_le_audio_power_off_setup();
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */
                }
            }
            break;
        default:
            break;
    }

    if (local_ctx->bt_power_state == APP_HOME_SCREEN_BT_POWER_STATE_ENABLED) {
        if (bt_on || disconnect) {
#ifdef APPS_AUTO_SET_BT_DISCOVERABLE
            /* Disconnected from Smart phone, set the flag to prepare start BT discoverable. */
            /* If user refused pairing on Smart phone, must restart discoverable. */
            _bt_conn_auto_setup_scan_mode(true);
#endif /* #ifdef APPS_AUTO_SET_BT_DISCOVERABLE */
#ifdef APPS_AUTO_SET_LEA_ADV
            app_le_audio_switch_advertising(true);
        } else if (connect) {
            app_le_audio_switch_advertising(false);
#endif /* #ifdef APPS_AUTO_SET_LEA_ADV */
        }
    }

    return ret;
}

bool bt_conn_component_bt_event_proc(ui_shell_activity_t *self, uint32_t event_id, void *extra_data, size_t data_len)
{
    bool ret = false;
    /*
    apps_bt_event_data_t *bt_event_data = (apps_bt_event_data_t *)extra_data;
    home_screen_local_context_type_t *local_ctx = (home_screen_local_context_type_t *)self->local_context;

    switch(event_id) {
        default:
            break;

    }
    */
    return ret;
}

