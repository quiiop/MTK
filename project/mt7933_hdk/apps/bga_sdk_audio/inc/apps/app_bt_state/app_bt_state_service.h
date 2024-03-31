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
 * File: app_bt_state_service.h
 *
 * Description: This file defines the interface of app_bt_state_service.c.
 *
 */

#ifndef __APP_BT_STATE_SERVICE_H__
#define __APP_BT_STATE_SERVICE_H__


#define BT_VISIBLE_TIMEOUT_INVALID      (0)                 /* Invalid BT visibility duration. */


/**
 *  @brief This enum defines the states of BT connection.
 */
typedef enum {
    APP_BT_CONNECTION_SERVICE_BT_STATE_BT_OFF,              /**<  BT off. */
    APP_BT_CONNECTION_SERVICE_BT_STATE_DISCONNECTED,        /**<  BT on but ACL (Asynchronous Connectionless) disconnected. */
    APP_BT_CONNECTION_SERVICE_BT_STATE_ACL_CONNECTED,       /**<  BT on and ACL (Asynchronous Connectionless) connected, but no profile connected. */
    APP_BT_CONNECTION_SERVICE_BT_STATE_PROFILE_CONNECTED    /**<  BT on and profile connected. */
} app_bt_state_service_connection_state_t;


/**
 *  @brief This structure defines the status/context of BT state service.
 */
typedef struct {
    app_bt_state_service_connection_state_t connection_state;   /**<  BT connection state. */
    bool bt_visible;                                            /**<  BT visibility state. */
    bool bt_connectable;                                        /**<  BT connectivity state. */
} app_bt_state_service_status_t;


/**
* @brief      This function is used to turn on/off BT.
* @param[in]  on, TRUE - turn on BT, FALSE - turn off BT.
* @return     If return true, the bt on off is executed.
*/
bool app_bt_state_service_set_bt_on_off(bool on);


/**
* @brief      This function is used to handle events and update BT state after pre-process.
* @param[in]  event_group, the current event group to be handled.
* @param[in]  event_id, the current event ID to be handled.
* @param[in]  extra_data, extra data pointer of the current event, NULL means there is no extra data.
* @param[in]  data_len, the length of the extra data. 0 means extra_data is NULL.
* @return     If return true, the current event cannot be handle by the next activity.
*/
bool app_bt_state_service_process_events(uint32_t event_group,
                                         uint32_t event_id,
                                         void *extra_data,
                                         size_t data_len);

#ifdef MTK_BT_DUO_ENABLE
/**
* @brief      This function is used to turn on/off BT scan mode.
* @param[in]  enable_visible, TRUE - enable BT visibility, FALSE - disable BT visibility.
* @param[in]  enable_connectable, TRUE - enable BT connectivity, FALSE - disable BT connectivity.
* @param[in]  timeout, BT visibility/connectivity duration time.
* @return     If return true, the operation is successful.
*/
bool app_bt_state_service_set_bt_scan_mode(bool enable_visible, bool enable_connectable, uint32_t timeout);
#endif /* #ifdef MTK_BT_DUO_ENABLE */

#endif /* #ifndef __APP_BT_STATE_SERVICE_H__ */
