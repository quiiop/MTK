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

#ifndef __APP_MUSIC_UTILS_H__
#define __APP_MUSIC_UTILS_H__

#include "bt_sink_srv.h"
#include "ui_shell_manager.h"
#include "ui_shell_activity.h"
#include "apps_config_event_list.h"

#define AVRCP_OPERATION_STA_IDLE 0
#define AVRCP_OPERATION_STA_FAST_FORWARD_PRESS 1
#define AVRCP_OPERATION_STA_FAST_REWIND_PRESS 2

/**
 *  @brief This structure defines the music app's context
 */
typedef struct {
    bool music_playing;                 /**<  Indicates whether the music is playing. */
    bool isAutoPaused;                  /**<  Indicates whether the music is suspended by the music app itself. */
    uint32_t avrcp_op_sta;              /**<  Record the last avrcp operation. */
} apps_music_local_context_t;

/**
* @brief      This function is used to free the remain data for AVRCP Element Attribute.
*/
void app_bt_music_free_attr_list(void);

/**
* @brief      This function is used to handle the key action.
* @param[in]  self, the context pointer of the activity.
* @param[in]  action, the operation ID of avrcp.
* @param[in]  data, data pointer of the current event, NULL means there is no data.
* @return     If return true, the current event cannot be handle by the next activity.
*/
bool app_do_music_action(
    ui_shell_activity_t *self,
    apps_config_key_action_t action,
    void *data);

/**
* @brief      This function is used to handle the key action.
* @param[in]  self, the context pointer of the activity.
* @param[in]  event_id, the current event ID to be handled.
* @param[in]  extra_data, extra data pointer of the current event, NULL means there is no extra data.
* @param[in]  data_len, the length of the extra data. 0 means extra_data is NULL.
* @return     If return true, the current event cannot be handle by the next activity.
*/
/*apps_config_key_action_t app_bt_music_proc_key_event(
    ui_shell_activity_t *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len);*/

/**
* @brief      This function is used to handle the event come from sink service module.
* @param[in]  self, the context pointer of the activity.
* @param[in]  event_id, the current event ID to be handled.
* @param[in]  extra_data, extra data pointer of the current event, NULL means there is no extra data.
* @param[in]  data_len, the length of the extra data. 0 means extra_data is NULL.
* @return     If return true, the current event cannot be handle by the next activity.
*/
bool app_bt_music_proc_basic_state_event(
    ui_shell_activity_t *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len);

/**
* @brief      This function is used to handle the event come from bt connection manager.
* @param[in]  self, the context pointer of the activity.
* @param[in]  event_id, the current event ID to be handled.
* @param[in]  extra_data, extra data pointer of the current event, NULL means there is no extra data.
* @param[in]  data_len, the length of the extra data. 0 means extra_data is NULL.
* @return     If return true, the current event cannot be handle by the next activity.
*/
bool app_music_util_bt_cm_event_proc(
    ui_shell_activity_t *self,
    uint32_t event_id,
    void *extra_data,
    size_t data_len);
#endif /* #ifndef __APP_MUSIC_UTILS_H__ */

