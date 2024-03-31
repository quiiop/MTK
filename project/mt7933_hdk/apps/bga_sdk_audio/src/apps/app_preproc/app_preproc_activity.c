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

#include "apps_debug.h"
#include "app_preproc_activity.h"
#include "apps_events_event_group.h"
#include "apps_events_interaction_event.h"

#include "app_bt_state_service.h"

//#include "bt_app_common.h"


static bool _proc_ui_shell_group(
    struct _ui_shell_activity *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len)
{
    bool ret = true; // UI shell internal event must process by this activity, so default is true

    switch (event_id) {
        case EVENT_ID_SHELL_SYSTEM_ON_CREATE:
            break;
        case EVENT_ID_SHELL_SYSTEM_ON_DESTROY:
            APPS_LOG_MSGID_I("app_preproc_activity destroy", 0);
            break;
        case EVENT_ID_SHELL_SYSTEM_ON_RESUME:
            APPS_LOG_MSGID_I("app_preproc_activity resume", 0);
            break;
        case EVENT_ID_SHELL_SYSTEM_ON_PAUSE:
            APPS_LOG_MSGID_I("app_preproc_activity pause", 0);
            break;
        case EVENT_ID_SHELL_SYSTEM_ON_REFRESH:
            APPS_LOG_MSGID_I("app_preproc_activity refresh", 0);
            break;
        case EVENT_ID_SHELL_SYSTEM_ON_RESULT:
            APPS_LOG_MSGID_I("app_preproc_activity result", 0);
            break;
        default:
            break;
    }
    return ret;
}

#if 0 // not used now
static bool pre_proc_key_event_proc(ui_shell_activity_t *self, uint32_t event_id, void *extra_data, size_t data_len)
{
    if (extra_data) {
        uint8_t key_id;
        airo_key_event_t key_event;
        uint16_t *p_key_action = (uint16_t *)extra_data;
        if (*p_key_action) {
            APPS_LOG_MSGID_I("The key pressed from power on, do special %04x", 1, event_id);
            return true;
        }
        app_event_key_event_decode(&key_id, &key_event, event_id);
        *p_key_action = apps_config_key_event_remapper_map_action(key_id, key_event);
    }
    return false;
}

static bool pre_proc_app_interaction_event_proc(ui_shell_activity_t *self, uint32_t event_id, void *extra_data, size_t data_len)
{
    bool ret = false;

    switch (event_id) {
        case APPS_EVENTS_INTERACTION_INCREASE_BLE_ADV_INTERVAL:
            bt_app_common_trigger_increase_ble_adv();
            ret = true;
            break;
        case APPS_EVENTS_INTERACTION_RELOAD_KEY_ACTION_FROM_NVKEY:
            apps_config_key_remaper_init_configurable_table();
            ret = true;
            break;
        default:
            break;
    }

    return ret;
}
#endif

bool app_preproc_activity_proc(
    ui_shell_activity_t *self,
    uint32_t event_group,
    uint32_t event_id,
    void *extra_data,
    size_t data_len)
{
    bool ret = false;

    //APPS_LOG_MSGID_I("pre proc rx evt_group : %d, id : %x", 2, event_group, event_id);

    switch (event_group) {
        case EVENT_GROUP_UI_SHELL_SYSTEM: {
                ret = _proc_ui_shell_group(self, event_id, extra_data, data_len);
                break;
            }
        case EVENT_GROUP_UI_SHELL_APP_INTERACTION:
            //ret = pre_proc_app_interaction_event_proc(self, event_id, extra_data, data_len);
            break;
        case EVENT_GROUP_UI_SHELL_KEY:
            //ret = pre_proc_key_event_proc(self, event_id, extra_data, data_len);
            break;
        case EVENT_GROUP_UI_SHELL_BT_SINK:
            //ret = bt_app_common_sink_event_proc(event_id, extra_data, data_len);
            break;
        case EVENT_GROUP_UI_SHELL_SYSTEM_POWER:
            //ret = sys_pwr_component_event_proc(self, event_id, extra_data, data_len);
            break;
        default:
            break;
    }

    if (!ret) {
        /* Handle again if ret is not TRUE, see app_bt_state_service.c. */
        ret = app_bt_state_service_process_events(event_group, event_id, extra_data, data_len);
    }

    return ret;
}
