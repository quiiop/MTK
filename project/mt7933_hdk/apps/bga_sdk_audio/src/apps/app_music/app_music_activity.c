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

#include "app_music_activity.h"
#include "app_music_utils.h"
#include "apps_debug.h"
#include "bt_sink_srv.h"
#include "apps_events_event_group.h"
#include "apps_events_interaction_event.h"
#include "apps_config_event_list.h"

#define APP_MUSIC_LOG_TAG  "[APP_Music]act "

static bool _proc_key_event_group(struct _ui_shell_activity *self, uint32_t event_id, void *extra_data, size_t data_len);

static bool _proc_ui_shell_group(struct _ui_shell_activity *self, uint32_t event_id, void *extra_data, size_t data_len)
{
    /* UI shell internal event must process by this activity, so default is true */
    bool ret = true;
    switch (event_id) {
        case EVENT_ID_SHELL_SYSTEM_ON_CREATE: {
                APPS_LOG_MSGID_I(APP_MUSIC_LOG_TAG"create  current activity : %x", 1, (uint32_t)self);
                if (extra_data) {
                    self->local_context = extra_data;
                    apps_music_local_context_t *local_context = (apps_music_local_context_t *)self->local_context;
                    local_context->avrcp_op_sta = AVRCP_OPERATION_STA_IDLE;
                }
                break;
            }
        case EVENT_ID_SHELL_SYSTEM_ON_DESTROY: {
                APPS_LOG_MSGID_I(APP_MUSIC_LOG_TAG"destroy", 0);
                break;
            }
        case EVENT_ID_SHELL_SYSTEM_ON_RESUME: {
                APPS_LOG_MSGID_I(APP_MUSIC_LOG_TAG"resume", 0);
                break;
            }
        case EVENT_ID_SHELL_SYSTEM_ON_PAUSE: {
                APPS_LOG_MSGID_I(APP_MUSIC_LOG_TAG"pause", 0);
                break;
            }
        case EVENT_ID_SHELL_SYSTEM_ON_REFRESH: {
                APPS_LOG_MSGID_I(APP_MUSIC_LOG_TAG"refresh", 0);
                break;
            }
        case EVENT_ID_SHELL_SYSTEM_ON_RESULT: {
                APPS_LOG_MSGID_I(APP_MUSIC_LOG_TAG"result", 0);
                break;
            }
        default:
            break;
    }
    return ret;
}

static bool _proc_key_event_group(
    struct _ui_shell_activity *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len)
{
    // In 793x, event id will just be mapping to our key event
    apps_config_key_action_t action = event_id & 0xFFFF;
    return app_do_music_action(self, action, extra_data);
}

static bool _proc_bt_event_when_playing(
    struct _ui_shell_activity *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len)
{
    if (event_id == BT_SINK_SRV_EVENT_STATE_CHANGE
#if defined(AIR_LE_AUDIO_ENABLE) && defined(MTK_COMMON_LE_STATUS_EVT)
        || event_id == LE_SINK_SRV_EVENT_STATE_CHANGE
#endif /* #if defined(AIR_LE_AUDIO_ENABLE) && defined(MTK_COMMON_LE_STATUS_EVT) */
       ) {
        bt_sink_srv_state_change_t *param = (bt_sink_srv_state_change_t *)extra_data;
        apps_music_local_context_t *local_context = (apps_music_local_context_t *)self->local_context;

        APPS_LOG_MSGID_I(APP_MUSIC_LOG_TAG",bt_music_state_change now = 0x%x, pre = 0x%x, is playing: 0x%x",
                         3, param->current, param->previous, local_context->music_playing);

        /* Finish current activity when the a2dp streaming stops. */
        if ((param->previous == BT_SINK_SRV_STATE_STREAMING) && (param->current != BT_SINK_SRV_STATE_STREAMING)) {
            if (local_context->music_playing) {
                ui_shell_finish_activity(self, self);
                local_context->music_playing = false;
                local_context->isAutoPaused = false;
                APPS_LOG_MSGID_I(APP_MUSIC_LOG_TAG",finish app_music_activity_proc, curr act: 0x%x, isAutoPaused:%d", 2,
                                 (uint32_t)self, local_context->isAutoPaused);
            }
        }
    }
    return false;
}

static bool _proc_apps_internal_events(
    struct _ui_shell_activity *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len)
{
    bool ret = false;

    switch (event_id) {
        default:
            APPS_LOG_MSGID_I("Not supported event id = 0x%x", 1, event_id);
            break;
    }

    return ret;
}

bool app_music_activity_proc(
    struct _ui_shell_activity *self,
    uint32_t event_group,
    uint32_t event_id,
    void *extra_data,
    size_t data_len)
{
    bool ret = false;
    switch (event_group) {
        case EVENT_GROUP_UI_SHELL_SYSTEM: {
                /* ui_shell internal events. */
                ret = _proc_ui_shell_group(self, event_id, extra_data, data_len);
                break;
            }
        case EVENT_GROUP_UI_SHELL_KEY: {
                /* key event. */
                ret = _proc_key_event_group(self, event_id, extra_data, data_len);
                break;
            }
        case EVENT_GROUP_UI_SHELL_BT_SINK: {
                /* BT_SINK events, indicates the state of music. */
                ret = _proc_bt_event_when_playing(self, event_id, extra_data, data_len);
                break;
            }
        case EVENT_GROUP_UI_SHELL_APP_INTERACTION: {
                /* interaction events. */
                ret = _proc_apps_internal_events(self, event_id, extra_data, data_len);
                break;
            }
        default:
            break;
    }
    return ret;
}
