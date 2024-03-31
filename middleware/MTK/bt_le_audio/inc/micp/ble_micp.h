/* Copyright Statement:
 *
 * (C) 2020  MediaTek Inc. All rights reserved.
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

#ifndef __BLE_MICP_H__
#define __BLE_MICP_H__

#include "bt_le_audio_type.h"

/**************************************************************************************************
* Define
**************************************************************************************************/

#define BLE_MICP_ACTION_READ_MUTE                                (0x0101)  /**< Set NULL in params.*/
#define BLE_MICP_ACTION_SET_MUTE                                  (0x0201)  /**< Set true for mute, false for unmute in params.*/
#define BLE_MICP_ACTION_SET_MUTE_NOTIFICATION        (0x0301)  /**< Set true for enable cccd, false for disable cccd in params.*/
typedef uint16_t ble_micp_action_t;                                   /**< The MICP action type.*/

#define BLE_MICP_IDLE         0x00
#define BLE_MICP_DISCOVER_SERVICE_COMPLETE_IND          (BT_LE_AUDIO_MODULE_MICP | 0x0001)
#define BLE_MICP_MICS_MUTE_IND                                         (BT_LE_AUDIO_MODULE_MICP | 0x0002)
#define BLE_MICP_MICS_READ_MUTE_CNF                               (BT_LE_AUDIO_MODULE_MICP | 0x0101)
#define BLE_MICP_MICS_SET_MUTE_CNF                                 (BT_LE_AUDIO_MODULE_MICP | 0x0201)
#define BLE_MICP_MICS_SET_MUTE_NOTIFICATION_CNF        (BT_LE_AUDIO_MODULE_MICP | 0x0301)
typedef uint32_t ble_micp_event_t;                                    /**< The type of MICP events.*/


/**************************************************************************************************
* Structure
**************************************************************************************************/

/**
 *  @brief This structure defines the parameter data type for send action #BLE_MICP_MICS_SET_MUTE.
 */
typedef struct {
    bool   ismute;             /**< The mute status. */
} ble_micp_action_set_mute_t;

/**
 *  @brief This structure defines the parameter data type for send action #BLE_MICP_MICS_SET_MUTE_NOTIFICATION.
 */
typedef struct {
    bool enable_cccd;       /**< The cccd status. */
} ble_micp_action_set_mute_notification_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MICP_DISCOVER_SERVICE_COMPLETE_IND.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    bt_status_t status;         /**< Event status. */
} ble_micp_discover_service_complete_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MICP_MICS_MUTE_IND.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    bool   ismute;             /**< The mute status. */
} ble_micp_mics_mute_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MICP_MICS_READ_MUTE_CNF.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    bool   ismute;             /**< The mute status. */
} ble_micp_mics_read_mute_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MICP_MICS_SET_MUTE_CNF.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
} ble_micp_mics_set_mute_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MICP_MICS_SET_MUTE_NOTIFICATION_CNF.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
} ble_micp_mics_set_mute_notification_cnf_t;

/**************************************************************************************************
* Public Functions
**************************************************************************************************/

/**
 * @brief                       This function can handle three action about mute.
 * @action                        is the option to read mute, set mute, or set mute notificatiopn.
 * @param[in] action            is the option to set mute status or enable cccd.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_micp_send_action(bt_handle_t handle, ble_micp_action_t action, void *params);

/**
* @}
* @}
* @}
*/

#endif  /* __BLE_MICP_H__ */

