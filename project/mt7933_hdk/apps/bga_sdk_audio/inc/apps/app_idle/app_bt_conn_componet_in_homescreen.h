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

#ifndef __APP_BT_CONN_COMPONENT_IN_HOMESCREEN_H__
#define __APP_BT_CONN_COMPONENT_IN_HOMESCREEN_H__

#include "ui_shell_activity.h"
#include "apps_config_state_list.h"
#include "bt_sink_srv.h"

typedef enum {
    APP_HOME_SCREEN_STATE_IDLE,
    APP_HOME_SCREEN_STATE_POWERING_OFF,
    APP_HOME_SCREEN_STATE_REBOOT,
} app_home_screen_state_t;

typedef enum {
    APP_HOME_SCREEN_BT_POWER_STATE_NONE,
    APP_HOME_SCREEN_BT_POWER_STATE_DISABLED,
    APP_HOME_SCREEN_BT_POWER_STATE_ENABLED,
    APP_HOME_SCREEN_BT_POWER_STATE_DISABLING,
    APP_HOME_SCREEN_BT_POWER_STATE_ENABLING
} app_home_screen_bt_power_state_t;

enum {
    BLE_ADV_STATE_STOPPED       = 0,
    BLE_ADV_STATE_STARTING,
    BLE_ADV_STATE_STARTED
};

typedef struct {
    app_home_screen_state_t state;
    app_home_screen_bt_power_state_t bt_power_state;
    app_home_screen_bt_power_state_t target_bt_power_state;
    bt_sink_srv_state_t connection_state;/*sp connected state*/
    bool power_off_waiting_time_out;
    bool auto_start_visiable;
    bool is_bt_visiable;
    bool is_bt_connectable;

    uint8_t power_off_waiting_release_key;
    uint8_t ble_adv_state;
    uint8_t key_pressed_times;  // For press multi times to goto DUT mode
} home_screen_local_context_type_t;

bool bt_conn_component_bt_sink_event_proc(
    ui_shell_activity_t *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len);

bool bt_conn_component_bt_event_proc(
    ui_shell_activity_t *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len);


/**
* @brief      This function is used to handle BT CM event and update Homescreen APP context.
* @param[in]  self, the context pointer of the activity.
* @param[in]  event_id, the current event ID to be handled.
* @param[in]  extra_data, extra data pointer of the current event, NULL means there is no extra data.
* @param[in]  data_len, the length of the extra data. 0 means extra_data is NULL.
* @return     If return true, the current event cannot be handle by the next activity.
*/
bool bt_conn_component_bt_cm_event_proc(ui_shell_activity_t *self,
                                        uint32_t event_id,
                                        void *extra_data,
                                        size_t data_len);

#endif /* __APP_BT_CONN_COMPONENT_IN_HOMESCREEN_H__ */


