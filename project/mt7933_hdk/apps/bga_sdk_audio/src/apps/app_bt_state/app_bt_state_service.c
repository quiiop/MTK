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
 * File: app_bt_state_service.c
 *
 * Description: This file provides many utility function to control BT switch and visibility.
 *
 */

#include "ui_shell_manager.h"

#include "apps_debug.h"

#include "apps_events_event_group.h"
#include "apps_events_interaction_event.h"
#include "app_bt_state_service.h"

#include "bt_connection_manager.h"

#ifdef AIR_LE_AUDIO_ENABLE
#include "bt_sink_srv_le_cap.h"
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */

#ifdef MTK_BLE_DM_SUPPORT
#include "bt_device_manager_common.h"
#endif /* #ifdef MTK_BLE_DM_SUPPORT */

#define LOG_TAG     "[APP_btStateSrv]"

/* Current status/context of BT state service. */
static app_bt_state_service_status_t s_current_status = {
    .connection_state = APP_BT_CONNECTION_SERVICE_BT_STATE_BT_OFF,
    .bt_visible = false,
    .bt_connectable = false,
};

#ifndef MTK_BLE_DM_SUPPORT
extern bt_bd_addr_t g_local_public_addr;
extern bt_bd_addr_t g_local_random_addr;
#endif /* #ifndef MTK_BLE_DM_SUPPORT */

#ifdef MTK_BT_DUO_ENABLE
/**
* @brief      This function is used to refresh BT visibility duration time.
* @param[in]  timeout, updated BT visibility duration time.
*/
static void app_bt_state_service_refresh_visible_timeout(uint32_t timeout)
{
    APPS_LOG_MSGID_I(LOG_TAG" refresh_visible_timeout: %d", 1, timeout);
    ui_shell_remove_event(EVENT_GROUP_UI_SHELL_APP_INTERACTION, APPS_EVENTS_INTERACTION_BT_VISIBLE_TIMEOUT);
    if (timeout > BT_VISIBLE_TIMEOUT_INVALID) {
        ui_shell_send_event(false, EVENT_PRIORITY_HIGNEST, EVENT_GROUP_UI_SHELL_APP_INTERACTION,
                            APPS_EVENTS_INTERACTION_BT_VISIBLE_TIMEOUT, NULL, 0,
                            NULL, timeout);
    }
}
#endif /* #ifdef MTK_BT_DUO_ENABLE */

/******************************************************************************
******************************* Public APIs ***********************************
******************************************************************************/
bool app_bt_state_service_set_bt_on_off(bool on)
{
    /* We should use CM to disconnect first, and then do bt power off */
    if (on) {
#ifndef MTK_BLE_DM_SUPPORT
        bt_power_on((bt_bd_addr_ptr_t)&g_local_public_addr, (bt_bd_addr_ptr_t)&g_local_random_addr);
#else /* #ifndef MTK_BLE_DM_SUPPORT */
        bt_power_on((bt_bd_addr_ptr_t)bt_device_manager_get_local_address(),
                    (bt_bd_addr_ptr_t)bt_device_manager_get_local_random_address());
#endif /* #ifndef MTK_BLE_DM_SUPPORT */
        return true;
    }

    ui_shell_remove_event(EVENT_GROUP_UI_SHELL_APP_INTERACTION, APPS_EVENTS_INTERACTION_BT_RECONNECT_DEVICE);

#ifdef MTK_BT_DUO_ENABLE
    /* Disconnect all BR/EDR links */
    if (bt_cm_get_connected_devices(BT_CM_PROFILE_SERVICE_MASK_NONE, NULL, 0)) {
        if (bt_cm_force_disconnect(NULL) == BT_STATUS_SUCCESS)
            return false;
    }
#endif /* #ifdef MTK_BT_DUO_ENABLE */

#ifdef AIR_LE_AUDIO_ENABLE
    /* Disconnect all LE links */
    if (bt_sink_srv_cap_disconnect_all() == BT_STATUS_SUCCESS)
        return false;
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */

    bt_power_off();
    return true;
}

#ifdef MTK_BT_DUO_ENABLE
bool app_bt_state_service_set_bt_scan_mode(bool enable_visible, bool enable_connectable, uint32_t timeout)
{
    bool ret = true;
    APPS_LOG_MSGID_I(LOG_TAG" set_bt_scanmode visible: %d  connectable: %d, timeout = %d", 2,
                     enable_visible, enable_connectable, timeout);

    /* No action if wanted BT visibility and current BT status is same. */
    if (enable_visible == s_current_status.bt_visible &&
        enable_connectable == s_current_status.bt_connectable) {
        APPS_LOG_MSGID_I(LOG_TAG" No action on set_bt_scan_mode (already %d, %d)", 1,
                         enable_visible, enable_connectable);
        if (enable_visible || enable_connectable) {
            /* Refresh BT visibility duration if enable_visible is TRUE. */
            app_bt_state_service_refresh_visible_timeout(timeout);
        }
        return true;
    }
    if (enable_visible) {
        /* Enable BT visibility, disable BT visibility pending request. */
        bt_cm_scan_mode(true, true);
        app_bt_state_service_refresh_visible_timeout(timeout);
        APPS_LOG_MSGID_I(LOG_TAG" Set discoverable", 0);
    } else if (enable_connectable) {
        bt_cm_scan_mode(false, true);
        app_bt_state_service_refresh_visible_timeout(timeout);
        APPS_LOG_MSGID_I(LOG_TAG" Set connectable", 0);
    } else {
        /* Agent stopped BT visibility. */
        bt_cm_scan_mode(false, false);
        ui_shell_remove_event(EVENT_GROUP_UI_SHELL_APP_INTERACTION, APPS_EVENTS_INTERACTION_BT_VISIBLE_TIMEOUT);
    }

    return ret;
}
#endif /* #ifdef MTK_BT_DUO_ENABLE */

/******************************************************************************
****************************** Process events *********************************
******************************************************************************/
#ifdef MTK_BT_DUO_ENABLE
static bool app_bt_state_service_process_interaction_events(uint32_t event_id,
                                                            void *extra_data,
                                                            size_t data_len)
{
    bool ret = false;
    switch (event_id) {
        case APPS_EVENTS_INTERACTION_BT_VISIBLE_TIMEOUT:
            APPS_LOG_MSGID_I(LOG_TAG" received BT visible/connectable timeout", 0);
            /* set as connectable after set visible and connectable timeout */
            if (s_current_status.bt_visible && s_current_status.bt_connectable) {
                if (bt_cm_get_connected_devices(BT_CM_PROFILE_SERVICE_MASK_NONE, NULL, 0))
                    bt_cm_scan_mode(false, false);
                else
                    bt_cm_scan_mode(false, true);
            }
            ret = true;
            break;
        /*
        case APPS_EVENTS_INTERACTION_BT_RETRY_POWER_ON_OFF:
            APPS_LOG_MSGID_I(LOG_TAG" retry bt on/off", 0);
            app_bt_state_service_check_and_do_bt_enable_disable(false);
            ret = true;
            break;
        */
        default:
            break;
    }

    return ret;
}
#endif /* #ifdef MTK_BT_DUO_ENABLE */

static bool app_bt_state_service_process_bt_cm_events(uint32_t event_id,
                                                      void *extra_data,
                                                      size_t data_len)
{
    bool ret = false;
    switch (event_id) {
        case BT_CM_EVENT_POWER_STATE_UPDATE: {
                bt_cm_power_state_update_ind_t *power_update = (bt_cm_power_state_update_ind_t *)extra_data;
                if (NULL == power_update)
                    break;

                if (BT_CM_POWER_STATE_OFF == power_update->power_state) {
                    /* BT on switch to off. */
                    APPS_LOG_MSGID_I(LOG_TAG" power_update Power OFF %x", 1, power_update->power_state);
                    s_current_status.bt_visible = false;
                    s_current_status.bt_connectable = false;
                    ui_shell_remove_event(EVENT_GROUP_UI_SHELL_APP_INTERACTION, APPS_EVENTS_INTERACTION_BT_VISIBLE_TIMEOUT);
                }
                break;
            }
        case BT_CM_EVENT_VISIBILITY_STATE_UPDATE: {
                bt_cm_visibility_state_update_ind_t *visible_update = (bt_cm_visibility_state_update_ind_t *)extra_data;
                if (NULL == visible_update) {
                    break;
                }
                /*APPS_LOG_MSGID_I(LOG_TAG" visibility_state: %d  connectivity_state: %d", 2,
                                 visible_update->visibility_state, visible_update->connectivity_state);*/
                /* Update BT visibility state. */
                s_current_status.bt_visible = visible_update->visibility_state;
                s_current_status.bt_connectable = visible_update->connectivity_state;
                if (!s_current_status.bt_visible && !s_current_status.bt_connectable) {
                    /* No need visibility_timeout event if BT visibility disabled. */
                    ui_shell_remove_event(EVENT_GROUP_UI_SHELL_APP_INTERACTION, APPS_EVENTS_INTERACTION_BT_VISIBLE_TIMEOUT);
                }

                break;
            }
        case BT_CM_EVENT_REMOTE_INFO_UPDATE: {


            }
            break;

        default:
            break;
    }


    return ret;
}


bool app_bt_state_service_process_events(uint32_t event_group,
                                         uint32_t event_id,
                                         void *extra_data,
                                         size_t data_len)
{
    bool ret = false;
    switch (event_group) {
        /* UI Shell APP_INTERACTION events. */
#ifdef MTK_BT_DUO_ENABLE
        case EVENT_GROUP_UI_SHELL_APP_INTERACTION:
            ret = app_bt_state_service_process_interaction_events(event_id, extra_data, data_len);
            break;
#endif /* #ifdef MTK_BT_DUO_ENABLE */
        /* UI Shell BT Connection Manager events. */
        case EVENT_GROUP_UI_SHELL_BT_CONN_MANAGER:
            ret = app_bt_state_service_process_bt_cm_events(event_id, extra_data, data_len);
            break;
        case EVENT_GROUP_UI_SHELL_BT_SINK:
            //ret = app_bt_state_service_process_bt_sink_events(event_id, extra_data, data_len);
            break;

        default:
            break;
    }
    return ret;
}

