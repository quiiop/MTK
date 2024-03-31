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
#include "ui_shell_manager.h"

#include "apps_debug.h"
#include "apps_customer_config.h"
#include "app_home_screen_idle_activity.h"
#include "app_bt_conn_componet_in_homescreen.h"
#include "app_bt_state_service.h"

#include "apps_events_event_group.h"
#include "apps_events_interaction_event.h"
#include "apps_events_bt_event.h"
#include "apps_config_event_list.h"

#include "bt_sink_srv.h"
#include "bt_device_manager.h"
#include "bt_connection_manager.h"
#ifdef AIR_LE_AUDIO_ENABLE
#include "bt_sink_srv_le_cap.h"
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */

#define APP_LOG_TAG  "[APP_Home]idle_act"

// Use a timeout before power off, to show LED and play VP
#define POWER_OFF_TIMER_NAME       "POWER_OFF"
#define WAIT_TIME_BEFORE_POWER_OFF  (3 * 1000)
//(move to customer config)#define VISIBLE_TIMEOUT             (2 * 60 * 1000)         /* The timeout of BT visibility. */

//#define CONN_PARAM_PROFILE  (BT_SINK_SRV_PROFILE_A2DP_SINK | BT_SINK_SRV_PROFILE_AVRCP)
#define CONN_PARAM_PROFILE   (BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_A2DP_SINK) |\
                              BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_AVRCP))

static ui_shell_activity_t *s_homescreen_self = NULL;

static void app_home_screen_check_and_do_power_off(home_screen_local_context_type_t *local_context)
{
    /* Must confirm BT off before system power off and reboot. */
    if (local_context->bt_power_state == APP_HOME_SCREEN_BT_POWER_STATE_DISABLED) {
        if (APP_HOME_SCREEN_STATE_POWERING_OFF == local_context->state &&
            !local_context->power_off_waiting_time_out) {
            APPS_LOG_MSGID_I(APP_LOG_TAG", ready to system off", 0);
            local_context->state = APP_HOME_SCREEN_STATE_IDLE;
        } else if (APP_HOME_SCREEN_STATE_REBOOT == local_context->state &&
                   !local_context->power_off_waiting_time_out) {
            APPS_LOG_MSGID_I(APP_LOG_TAG", ready to reboot", 0);
            local_context->state = APP_HOME_SCREEN_STATE_IDLE;
        }
    }
}

static void _trigger_power_off_flow(struct _ui_shell_activity *self, bool need_wait)
{
    home_screen_local_context_type_t *local_context = (home_screen_local_context_type_t *)(self->local_context);

    if (local_context->state != APP_HOME_SCREEN_STATE_IDLE) {
        APPS_LOG_MSGID_I(APP_LOG_TAG", _trigger_power_off_flow, already prepared to power off, just wait", 0);
        return;
    } else {
        local_context->state = APP_HOME_SCREEN_STATE_POWERING_OFF;
    }

    // (todo) Disable Audio before power off to avoid pop sound.
    // (todo) Disable BT

    if (local_context->bt_power_state == APP_HOME_SCREEN_BT_POWER_STATE_ENABLED) {
        local_context->bt_power_state = APP_HOME_SCREEN_BT_POWER_STATE_DISABLING;
        APPS_LOG_MSGID_E(APP_LOG_TAG", BT power off #1", 0);
        app_bt_state_service_set_bt_on_off(false);
        APPS_LOG_MSGID_E(APP_LOG_TAG", BT power off #2", 0);
    } else {
        APPS_LOG_MSGID_E(APP_LOG_TAG", BT power state is not enabled (state =%d)", 1, local_context->bt_power_state);
        local_context->state = APP_HOME_SCREEN_STATE_IDLE; //change back to idle state due to power off fail
        return;
    }

    if (need_wait) {
        local_context->power_off_waiting_time_out = true;
        ui_shell_send_event(false, EVENT_PRIORITY_HIGNEST, EVENT_GROUP_UI_SHELL_APP_INTERACTION,
                            APPS_EVENTS_INTERACTION_POWER_OFF_WAIT_TIMEOUT, NULL, 0,
                            NULL, WAIT_TIME_BEFORE_POWER_OFF);
    } else {
        /* Check and do power off. */
        local_context->power_off_waiting_time_out = false;
        app_home_screen_check_and_do_power_off(local_context);
    }
}



static bool _proc_ui_shell_group(
    struct _ui_shell_activity *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len)
{
    bool ret = true; // UI shell internal event must process by this activity, so default is true
    home_screen_local_context_type_t *local_ctx = (home_screen_local_context_type_t *)self->local_context;

    switch (event_id) {
        case EVENT_ID_SHELL_SYSTEM_ON_CREATE:
            APPS_LOG_MSGID_I(APP_LOG_TAG": create", 0);
            self->local_context = pvPortMalloc(sizeof(home_screen_local_context_type_t));
            if (self->local_context) {
                memset(self->local_context, 0, sizeof(home_screen_local_context_type_t));
                local_ctx = (home_screen_local_context_type_t *)self->local_context;
                local_ctx->state = APP_HOME_SCREEN_STATE_IDLE;
                local_ctx->bt_power_state = APP_HOME_SCREEN_BT_POWER_STATE_NONE;
                local_ctx->target_bt_power_state = APP_HOME_SCREEN_BT_POWER_STATE_NONE;
                local_ctx->connection_state = BT_SINK_SRV_STATE_NONE;
                local_ctx->power_off_waiting_release_key = KEY_ACTION_INVALID;
                local_ctx->is_bt_visiable = false;
                s_homescreen_self = self;
            }
            break;
        case EVENT_ID_SHELL_SYSTEM_ON_DESTROY:
            APPS_LOG_MSGID_I(APP_LOG_TAG": destroy", 0);
            if (self->local_context) {
                vPortFree(self->local_context);
            }
            break;
        /*case EVENT_ID_SHELL_SYSTEM_ON_RESUME:
            APPS_LOG_MSGID_I(APP_LOG_TAG": resume", 0);
            break;
        case EVENT_ID_SHELL_SYSTEM_ON_PAUSE:
            APPS_LOG_MSGID_I(APP_LOG_TAG": pause", 0);
            break;
        case EVENT_ID_SHELL_SYSTEM_ON_REFRESH:
            APPS_LOG_MSGID_I(APP_LOG_TAG": refresh", 0);
            break;
        case EVENT_ID_SHELL_SYSTEM_ON_RESULT:
            APPS_LOG_MSGID_I(APP_LOG_TAG": result", 0);
            if (extra_data) {
                APPS_LOG_MSGID_I(APP_LOG_TAG", extra data fo result", 0);
            }
            break;*/
        default:
            break;
    }
    return ret;
}

static bool _proc_key_event_group(
    ui_shell_activity_t *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len)
{
    bool ret = false;
    home_screen_local_context_type_t *local_context = (home_screen_local_context_type_t *)(self->local_context);

    // In 793x, event id will just be mapping to our key event
    //home_screen_local_context_type_t *local_context = (home_screen_local_context_type_t *)(self->local_context);
    apps_config_key_action_t action = event_id & 0xFFFF;

    if (local_context->bt_power_state == APP_HOME_SCREEN_BT_POWER_STATE_NONE) {
        APPS_LOG_MSGID_E(APP_LOG_TAG", Please call 'ble init' first", 0);
        return true;
    }

    if (action != KEY_POWER_ON && local_context->bt_power_state != APP_HOME_SCREEN_BT_POWER_STATE_ENABLED) {
        APPS_LOG_MSGID_E(APP_LOG_TAG", Pls make sure you have power on BT (now %d)", 1, local_context->bt_power_state);
        return true;
    }

    switch (action) {
        case KEY_POWER_OFF: {
                _trigger_power_off_flow(self, false);
#ifdef MTK_BT_AUDIO_PR
                app_bt_dbg_audio_pr_deinit();
#endif /* #ifdef MTK_BT_AUDIO_PR */
                ret = true;
                break;
            }
        case KEY_POWER_ON: {
                if (local_context->state != APP_HOME_SCREEN_STATE_IDLE) {
                    APPS_LOG_MSGID_E(APP_LOG_TAG", App dosn't in idle state!", 0);
                    return true;
                }

                //if (local_context->connection_state == APP_BT_OFF)
                //    app_home_screen_trigger_bt_enable_disable(self, true);
                /* Power on BT if current bt_power_state is disabled. */
                if (local_context->bt_power_state == APP_HOME_SCREEN_BT_POWER_STATE_DISABLED) {
                    if (app_bt_state_service_set_bt_on_off(true))
                        local_context->bt_power_state = APP_HOME_SCREEN_BT_POWER_STATE_ENABLING;
                    //next: set to enabled after event callback
                } else {
                    APPS_LOG_MSGID_I(APP_LOG_TAG", Can't do power on. (BT power state = %d)", 1, local_context->bt_power_state);
                }
#ifdef MTK_BT_AUDIO_PR
                app_bt_dbg_audio_pr_init();
#endif /* #ifdef MTK_BT_AUDIO_PR */
                ret = true;
                break;
            }
#ifdef MTK_BT_DUO_ENABLE
        case KEY_DISCOVERABLE: {
#if defined(MTK_BT_A2DP_ENABLE) && defined(MTK_BT_CM_SUPPORT)
                /* Add for prevent A2DP and LEA connected at the same time. */
                bt_bd_addr_t addr = {0};

                /* If A2DP connected, return.*/
                if (bt_cm_get_connected_devices(BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_A2DP_SINK), &addr, 1)) {
                    APPS_LOG_MSGID_I(APP_LOG_TAG" Cannot start discoverable when A2DP connected", 0);
                    break;
                }
#ifdef AIR_LE_AUDIO_ENABLE
                /* If LEA connected, return. */
                else if (bt_sink_srv_cap_check_links_state(BT_SINK_SRV_CAP_STATE_CONNECTED) != BT_HANDLE_INVALID) {
                    APPS_LOG_MSGID_I(APP_LOG_TAG" Cannot start discoverable when LEA connected", 0);
                    break;
                }
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */
#endif /* #if defined(MTK_BT_A2DP_ENABLE) && defined(MTK_BT_CM_SUPPORT) */

                app_bt_state_service_set_bt_scan_mode(true, false, APPS_BT_VISIBLE_TIMEOUT);
                ret = true;
                break;
            }
        case KEY_HIDDEN: {
                app_bt_state_service_set_bt_scan_mode(false, false, 0);
                ret = true;
                break;
            }
        case KEY_SINK_CONNECT: {
#ifdef AIR_LE_AUDIO_ENABLE
                /* If LEA connected, return. */
                if (bt_sink_srv_cap_check_links_state(BT_SINK_SRV_CAP_STATE_CONNECTED) != BT_HANDLE_INVALID) {
                    APPS_LOG_MSGID_I(APP_LOG_TAG" Cannot connect A2DP device when LEA connected", 0);
                    break;
                }
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */

                bt_cm_connect_t connect_param;
                connect_param.profile = CONN_PARAM_PROFILE;
                memcpy(connect_param.address, extra_data, sizeof(bt_bd_addr_t));
                bt_cm_connect(&connect_param);
                ret = true;
                break;
            }
        case KEY_SINK_DISCONN: {
                bt_cm_connect_t connect_param;
                memcpy(connect_param.address, extra_data, sizeof(bt_bd_addr_t));
                connect_param.profile = CONN_PARAM_PROFILE;
                bt_cm_disconnect(&connect_param);
                ret = true;
                break;
            }
        case KEY_RESET_PAIRED_DEVICES: {
                bt_device_manager_unpair_all();
                bt_device_manager_db_flush_all(BT_DEVICE_MANAGER_DB_FLUSH_BLOCK);
                ret = true;
                break;
            }
        case KEY_DELETE_PAIRED_DEVICES: {
                bt_bd_addr_t addr;
                bt_cm_connect_t connect_param;
                memcpy(connect_param.address, extra_data, sizeof(bt_bd_addr_t));
                bt_cm_disconnect(&connect_param);
                memcpy(addr, extra_data, sizeof(bt_bd_addr_t));
                bt_device_manager_delete_paired_device((bt_bd_addr_ptr_t)&addr);
                bt_device_manager_db_flush_all(BT_DEVICE_MANAGER_DB_FLUSH_BLOCK);
                ret = true;
                break;
            }
        case KEY_PAIRED_DEVICES_LIST: {
                uint32_t i;
                bt_device_manager_paired_infomation_t *paired_list =
                    (bt_device_manager_paired_infomation_t *)pvPortMalloc(BT_DEVICE_MANAGER_MAX_PAIR_NUM *
                                                                          sizeof(bt_device_manager_paired_infomation_t));
                uint32_t dev_count = BT_DEVICE_MANAGER_MAX_PAIR_NUM;
                bt_device_manager_get_paired_list(paired_list, &dev_count);
                for (i = 0; i < dev_count; i++) {
                    APPS_LOG_MSGID_I(APP_LOG_TAG", Dev[%d]", 1, i);
                    APPS_LOG_MSGID_I(APP_LOG_TAG", Addr %02x:%02x:%02x:%02x:%02x:%02x", 6,
                                     paired_list[i].address[5], paired_list[i].address[4], paired_list[i].address[3],
                                     paired_list[i].address[2], paired_list[i].address[1], paired_list[i].address[0]);
                    APPS_LOG_MSGID_I(APP_LOG_TAG", Name %s", 1, paired_list[i].name);
                }
                ret = true;
                break;
            }
        case KEY_RECONNECT_LAST_DEVICE: {
                bt_cm_connect_t connect_param;
                bt_bd_addr_t *p_bd_addr = bt_device_manager_remote_get_dev_by_seq_num(1);
                if (p_bd_addr) {
                    connect_param.profile = CONN_PARAM_PROFILE;
                    memcpy(connect_param.address, *p_bd_addr, sizeof(bt_bd_addr_t));
                    bt_cm_connect(&connect_param);
                    ret = true;
                }
                break;
            }
        case KEY_ROLE_SWITCH: {
                bt_bd_addr_t *p_bd_addr = bt_device_manager_remote_get_dev_by_seq_num(1);
                if (p_bd_addr)
                    bt_cm_switch_role(*p_bd_addr, *(bt_role_t *)extra_data);
                ret = true;
                break;
            }
        case KEY_GET_ROLE: {
                bt_bd_addr_t *p_bd_addr = bt_device_manager_remote_get_dev_by_seq_num(1);
                if (p_bd_addr)
                    bt_cm_get_role(*p_bd_addr);
                ret = true;
                break;
            }
        case KEY_SHOW_STATUS: {
                bt_cm_protected_show_devices();
                ret = true;
                break;
            }
#endif /* #ifdef MTK_BT_DUO_ENABLE */
#if defined(MTK_BT_A2DP_ENABLE) && defined(MTK_BT_AUDIO_PR)
        case KEY_AUDIO_PR: {
                app_bt_dbg_audio_pr_dump();
            }
#endif /* #if defined(MTK_BT_A2DP_ENABLE) && defined(MTK_BT_AUDIO_PR) */
        default:
            break;
    }

    return ret;
}


static bool homescreen_app_bt_sink_event_proc(ui_shell_activity_t *self, uint32_t event_id, void *extra_data, size_t data_len)
{
    bool ret = false;

    //APPS_LOG_MSGID_I(APP_LOG_TAG", BT event = 0x%x", 1, event_id);

    ret = bt_conn_component_bt_sink_event_proc(self, event_id, extra_data, data_len);

    return ret;
}

static bool homescreen_app_bt_event_proc(ui_shell_activity_t *self, uint32_t event_id, void *extra_data, size_t data_len)
{
    bool ret = false;
    //apps_bt_event_data_t *bt_event_data = (apps_bt_event_data_t *)extra_data;
    /* Use to update power state if no CM
        home_screen_local_context_type_t *local_ctx = (home_screen_local_context_type_t *)self->local_context;

        switch(event_id) {
            case BT_POWER_ON_CNF:
                local_ctx->bt_power_state = APP_HOME_SCREEN_BT_POWER_STATE_ENABLED;
                break;
            case BT_POWER_OFF_CNF:
                local_ctx->bt_power_state = APP_HOME_SCREEN_BT_POWER_STATE_DISABLED;
                break;
            default:
                break;

        }
    */
    //ret = bt_conn_component_bt_event_proc(self, event_id, extra_data, data_len);

    return ret;
}

static bool homescreen_app_bt_connection_manager_event_proc(ui_shell_activity_t *self,
                                                            uint32_t event_id, void *extra_data, size_t data_len)
{
    bool ret = false;
    home_screen_local_context_type_t *local_context = (home_screen_local_context_type_t *)(self->local_context);
    if (!local_context) {
        return ret;
    }

    ret = bt_conn_component_bt_cm_event_proc(self, event_id, extra_data, data_len);

    app_home_screen_check_and_do_power_off(local_context);

    return ret;
}

static bool _app_interaction_event_proc(ui_shell_activity_t *self, uint32_t event_id, void *extra_data, size_t data_len)
{
    home_screen_local_context_type_t *local_ctx = (home_screen_local_context_type_t *)self->local_context;
    bool ret = false;

    switch (event_id) {
        case APPS_EVENTS_INTERACTION_POWER_OFF_WAIT_TIMEOUT: {
                APPS_LOG_MSGID_I(APP_LOG_TAG", Timeout before power off", 0);
                /* Check and do power off or reboot if WAIT_TIME_BEFORE_POWER_OFF timeout. */
                if (APP_HOME_SCREEN_STATE_IDLE != local_ctx->state) {
                    local_ctx->power_off_waiting_time_out = false;
                    app_home_screen_check_and_do_power_off(local_ctx);
                    ret = true;
                }
            }
            break;
        case APPS_EVENTS_INTERACTION_BT_RECONNECT_DEVICE: {
                APPS_LOG_MSGID_I(APP_LOG_TAG", do reconnect, BT power state: %d", 1, local_ctx->bt_power_state);
                if (local_ctx->bt_power_state == APP_HOME_SCREEN_BT_POWER_STATE_ENABLED) {
                    bt_cm_connect_t connect_param;
                    connect_param.profile = CONN_PARAM_PROFILE;
                    memcpy(connect_param.address, extra_data, sizeof(bt_bd_addr_t));
                    bt_cm_connect(&connect_param);
                }
                ret = true;
            }
            break;
        default:
            APPS_LOG_MSGID_I(APP_LOG_TAG", Not supported event id = %d", 1, event_id);
            break;
    }
    return ret;
}

bool app_home_screen_idle_activity_proc(
    ui_shell_activity_t *self,
    uint32_t event_group,
    uint32_t event_id,
    void *extra_data,
    size_t data_len)
{
    bool ret = false;

    //APPS_LOG_MSGID_I(APP_LOG_TAG", receive event_group : %d, id: %x", 2, event_group, event_id);

    switch (event_group) {
        case EVENT_GROUP_UI_SHELL_SYSTEM: {
                ret = _proc_ui_shell_group(self, event_id, extra_data, data_len);
                break;
            }
        case EVENT_GROUP_UI_SHELL_KEY: {
                ret = _proc_key_event_group(self, event_id, extra_data, data_len);
                break;
            }
        case EVENT_GROUP_UI_SHELL_BT_SINK: {
                ret = homescreen_app_bt_sink_event_proc(self, event_id, extra_data, data_len);
                break;

            }
        case EVENT_GROUP_UI_SHELL_BT: {
                ret = homescreen_app_bt_event_proc(self, event_id, extra_data, data_len);
                break;

            }
        case EVENT_GROUP_UI_SHELL_BT_CONN_MANAGER: {
                ret = homescreen_app_bt_connection_manager_event_proc(self, event_id, extra_data, data_len);
                break;
            }
        case EVENT_GROUP_UI_SHELL_APP_INTERACTION:
            ret = _app_interaction_event_proc(self, event_id, extra_data, data_len);
            break;

        default:
            break;
    }
    return ret;
}


