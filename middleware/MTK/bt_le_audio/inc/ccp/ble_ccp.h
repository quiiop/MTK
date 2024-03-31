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


#ifndef __BLE_CCP_H__
#define __BLE_CCP_H__

#include "ble_tbs_def.h"

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothLeaudio LE_AUDIO
 * @{
 * @addtogroup BluetoothLeAudioCCP CCP
 * @{
 * This section introduces the CCP operation codes, request and response structures, API prototypes.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b CCP                        | Call Control Profile. |
 * |\b TBS                        | Telephone Bearer Service. |
 * |\b GTBS                       | Generic Telephone Bearer Service. |
 * |\b UCI                        | Uniform Caller Identifier. |
 * |\b URI                        | Uniform Resource Identifier. |
 * |\b CCID                       | Content Control ID. |
 *
 * @section ble_ccp_api_usage How to use this module
 *   - Send CCP action with #ble_ccp_send_action() to control GTBS or TBS server.
 *   - Sample code:
 *     @code
 *          // send an action to configure the termination reason characteristic notification
 *          ble_ccp_action_set_notification_t param = {
 *              .enable = 1,
 *          };
 *          bt_handle_t handle = 0x0001;                        // LE connection handle
 *          uint8_t service_idx = BLE_CCP_SERVICE_INDEX_GTBS;   // The service index for the action. BLE_CCP_SERVICE_INDEX_GTBS is the service index of the GTBS.
 *          ble_ccp_send_action(handle, BLE_CCP_ACTION_SET_TERMINATION_REASON_NOTIFICATION, &param, service_idx);
 *
 *     @endcode
 *
 *   - Implement the event callback to process the related CCP events.
 *   - Sample code:
 *     @code
 *          bt_status_t bt_sink_srv_le_call_ccp_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
 *          {
 *              switch (msg) {
 *                  case BLE_CCP_SET_TERMINATION_REASON_NOTIFICATION_CNF: {
 *                      ble_ccp_event_parameter_t *cnf = (ble_ccp_event_parameter_t *)buffer;
 *                      // receive the result of configuring the termination reason characteristic notification
 *                      printf("[CCP] SET_TERMINATION_REASON_NOTIFICATION_CNF, handle:%x status:%x", cnf->handle, status);
 *                      break;
 *                  }
 *
 *                  case BLE_CCP_TERMINATION_REASON_IND: {
 *                      // receive the termination reason notification
 *                      ble_ccp_termination_reason_ind_t *ind = (ble_ccp_termination_reason_ind_t *)buffer;
 *                      printf("[CCP] TERMINATION_REASON_IND, handle:%x status:%x", ind->handle, status);
 *                      printf("[CCP] TERMINATION_REASON_IND, call_index:%x reason:%x", ind->call_index, ind->reason);
 *                      break;
 *                  }
 *
 *                  default:
 *                      break;
 *              }
 *
 *              return BT_STATUS_SUCCESS;
 *          }
 *
 *     @endcode
 */

/**
 * @defgroup Bluetoothble_CCP_define Define
 * @{
 * This section defines the CCP action opcode and event IDs.
 */

/**
 *  @brief Defines the CCP actions.
 */
#define BLE_CCP_ACTION_READ_BEARER_PROVIDER_NAME                        (0x00000101)    /**< This action sends a request to read the bearer provider name. */
#define BLE_CCP_ACTION_READ_BEARER_UCI                                  (0x00000102)    /**< This action sends a request to read the bearer UCI. */
#define BLE_CCP_ACTION_READ_BEARER_TECHNOLOGY                           (0x00000103)    /**< This action sends a request to read the bearer technology. */
#define BLE_CCP_ACTION_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST           (0x00000104)    /**< This action sends a request to read the bearer URI prefixes supported list. */
#define BLE_CCP_ACTION_READ_BEARER_SIGNAL_STRENGTH                      (0x00000105)    /**< This action sends a request to read the bearer signal strength. */
#define BLE_CCP_ACTION_READ_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL   (0x00000106)    /**< This action sends a request to read the bearer signal strength reporting interval. */
#define BLE_CCP_ACTION_READ_BEARER_LIST_CURRENT_CALLS                   (0x00000107)    /**< This action sends a request to read the bearer list current calls. */
#define BLE_CCP_ACTION_READ_CONTENT_CONTROL_ID                          (0x00000108)    /**< This action sends a request to read the content control ID. */
#define BLE_CCP_ACTION_READ_STATUS_FLAGS                                (0x00000109)    /**< This action sends a request to read the status flags. */
#define BLE_CCP_ACTION_READ_INCOMING_CALL_TARGET_BEARER_URI             (0x0000010A)    /**< This action sends a request to read the incoming call target bearer URI. */
#define BLE_CCP_ACTION_READ_CALL_STATE                                  (0x0000010B)    /**< This action sends a request to read the call state information. */
#define BLE_CCP_ACTION_READ_CALL_CONTROL_POINT_OPTIONAL_OPCODES         (0x0000010C)    /**< This action sends a request to read the call control point optional opcodes. */
#define BLE_CCP_ACTION_READ_INCOMING_CALL                               (0x0000010D)    /**< This action sends a request to read the incoming call information. */
#define BLE_CCP_ACTION_READ_CALL_FRIENDLY_NAME                          (0x0000010E)    /**< This action sends a request to read the call friendly name information. */

#define BLE_CCP_ACTION_SET_BEARER_PROVIDER_NAME_NOTIFICATION                (0x00000301)    /**< This action sends a request to set notification of the bearer provider name characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_SET_BEARER_TECHNOLOGY_NOTIFICATION                   (0x00000302)    /**< This action sends a request to set notification of the bearer technology characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_SET_BEARER_URI_SCHEMES_SUPPORTED_LIST_NOTIFICATION   (0x00000303)    /**< This action sends a request to set notification of the bearer URI schemes supported list characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_SET_BEARER_SIGNAL_STRENGTH_NOTIFICATION              (0x00000304)    /**< This action sends a request to set notification of the bearer signal strength characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_SET_BEARER_LIST_CURRENT_CALLS_NOTIFICATION           (0x00000305)    /**< This action sends a request to set notification of the bearer list current calls characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_SET_STATUS_FLAGS_NOTIFICATION                        (0x00000306)    /**< This action sends a request to set notification of the status flags characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_SET_INCOMING_CALL_TARGET_BEARER_URI_NOTIFICATION     (0x00000307)    /**< This action sends a request to set notification of the incoming call target bearer URI characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_SET_CALL_STATE_NOTIFICATION                          (0x00000308)    /**< This action sends a request to set notification of the call state characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_SET_TERMINATION_REASON_NOTIFICATION                  (0x0000030A)    /**< This action sends a request to set notification of the termination reason characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_SET_INCOMING_CALL_NOTIFICATION                       (0x0000030B)    /**< This action sends a request to set notification of the incoming call characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_SET_CALL_FRIENDLY_NAME_NOTIFICATION                  (0x0000030C)    /**< This action sends a request to set notification of the call friendly name characteristic with parameter #ble_ccp_action_set_notification_t. */
#define BLE_CCP_ACTION_INVALID                                              (0xFFFFFFFF)    /**< Invalid action type. */
typedef uint32_t ble_ccp_action_t;                                                      /**< The type of CCP actions.*/

/**
 *  @brief Defines the CCP events.
 */
#define BLE_CCP_BEARER_PROVIDER_NAME_IND                         (BT_LE_AUDIO_MODULE_CCP | 0x0001)    /**< This event indicates the bearer provider name changes with payload #ble_ccp_bearer_provider_name_ind_t. */
#define BLE_CCP_BEARER_TECHNOLOGY_IND                            (BT_LE_AUDIO_MODULE_CCP | 0x0002)    /**< This event indicates the bearer technology changes with payload #ble_ccp_bearer_technology_ind_t. */
#define BLE_CCP_BEARER_URI_SCHEMES_SUPPORTED_LIST_IND            (BT_LE_AUDIO_MODULE_CCP | 0x0003)    /**< This event indicates the bearer URI schemes supported list changes with payload #ble_ccp_bearer_uri_schemes_supported_list_ind_t. */
#define BLE_CCP_BEARER_SIGNAL_STRENGTH_IND                       (BT_LE_AUDIO_MODULE_CCP | 0x0004)    /**< This event indicates the bearer signal strength changes with payload #ble_ccp_bearer_signal_strength_ind_t. */
#define BLE_CCP_BEARER_LIST_CURRENT_CALLS_IND                    (BT_LE_AUDIO_MODULE_CCP | 0x0005)    /**< This event indicates the bearer list current calls changes with payload #ble_ccp_bearer_list_current_calls_ind_t. */
#define BLE_CCP_STATUS_FLAGS_IND                                 (BT_LE_AUDIO_MODULE_CCP | 0x0006)    /**< This event indicates the features and status flags changes with payload #ble_ccp_status_flags_ind_t. */
#define BLE_CCP_INCOMING_CALL_TARGET_BEARER_URI_IND              (BT_LE_AUDIO_MODULE_CCP | 0x0007)    /**< This event indicates the incoming call target bearer URI changes with payload #ble_ccp_incoming_call_target_bearer_uri_ind_t. */
#define BLE_CCP_CALL_STATE_IND                                   (BT_LE_AUDIO_MODULE_CCP | 0x0008)    /**< This event indicates the call state changes with payload #ble_ccp_call_state_ind_t. */
#define BLE_CCP_TERMINATION_REASON_IND                           (BT_LE_AUDIO_MODULE_CCP | 0x000A)    /**< This event indicates the termination reason changes with payload #ble_ccp_termination_reason_ind_t. */
#define BLE_CCP_INCOMING_CALL_IND                                (BT_LE_AUDIO_MODULE_CCP | 0x000B)    /**< This event indicates the incoming call information changes with payload #ble_ccp_incoming_call_ind_t. */
#define BLE_CCP_CALL_FRIENDLY_NAME_IND                           (BT_LE_AUDIO_MODULE_CCP | 0x000C)    /**< This event indicates the call friendly name changes with payload #ble_ccp_call_friendly_name_ind_t. */

#define BLE_CCP_READ_BEARER_PROVIDER_NAME_CNF                        (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_BEARER_PROVIDER_NAME)                        /**< The result of reading bearer provider name with payload #ble_ccp_read_bearer_provider_name_cnf_t. */
#define BLE_CCP_READ_BEARER_UCI_CNF                                  (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_BEARER_UCI)                                  /**< The result of reading bearer UCI with payload #ble_ccp_read_bearer_uci_cnf_t. */
#define BLE_CCP_READ_BEARER_TECHNOLOGY_CNF                           (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_BEARER_TECHNOLOGY)                           /**< The result of reading bearer technology with payload #ble_ccp_read_bearer_technology_cnf_t. */
#define BLE_CCP_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CNF           (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST)           /**< The result of reading bearer URI schemes supported list with payload #ble_ccp_read_bearer_uri_schemes_supported_list_cnf_t. */
#define BLE_CCP_READ_BEARER_SIGNAL_STRENGTH_CNF                      (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_BEARER_SIGNAL_STRENGTH)                      /**< The result of reading bearer signal strength with payload #ble_ccp_read_bearer_signal_strength_cnf_t. */
#define BLE_CCP_READ_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL_CNF   (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL)   /**< The result of reading bearer signal strength reporting interval with payload #ble_ccp_read_bearer_signal_strength_reporting_interval_cnf_t. */
#define BLE_CCP_READ_BEARER_LIST_CURRENT_CALLS_CNF                   (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_BEARER_LIST_CURRENT_CALLS)                   /**< The result of reading bearer list current calls with payload #ble_ccp_read_bearer_list_current_calls_cnf_t. */
#define BLE_CCP_READ_CONTENT_CONTROL_ID_CNF                          (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_CONTENT_CONTROL_ID)                          /**< The result of reading content control ID with payload #ble_ccp_read_content_control_id_cnf_t. */
#define BLE_CCP_READ_STATUS_FLAGS_CNF                                (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_STATUS_FLAGS)                                /**< The result of reading status flags with payload #ble_ccp_read_status_flags_cnf_t. */
#define BLE_CCP_READ_INCOMING_CALL_TARGET_BEARER_URI_CNF             (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_INCOMING_CALL_TARGET_BEARER_URI)             /**< The result of reading incoming call target bearer URI with payload #ble_ccp_read_incoming_call_target_bearer_uri_cnf_t. */
#define BLE_CCP_READ_CALL_STATE_CNF                                  (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_CALL_STATE)                                  /**< The result of reading call state with payload #ble_ccp_read_call_state_cnf_t. */
#define BLE_CCP_READ_CALL_CONTROL_POINT_OPTIONAL_OPCODES_CNF         (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_CALL_CONTROL_POINT_OPTIONAL_OPCODES)         /**< The result of reading call control point optional opcodes with payload #ble_ccp_read_call_control_point_optional_opcodes_cnf_t. */
#define BLE_CCP_READ_INCOMING_CALL_CNF                               (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_INCOMING_CALL)                               /**< The result of reading incoming call with payload #ble_ccp_read_incoming_call_cnf_t. */
#define BLE_CCP_READ_CALL_FRIENDLY_NAME_CNF                          (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_READ_CALL_FRIENDLY_NAME)                          /**< The result of reading call friendly name with payload #ble_ccp_read_call_friendly_name_cnf_t. */

#define BLE_CCP_SET_BEARER_PROVIDER_NAME_NOTIFICATION_CNF                (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_BEARER_PROVIDER_NAME_NOTIFICATION)                /**< The result of setting notification of the bearer provider name characteristic with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_SET_BEARER_TECHNOLOGY_NOTIFICATION_CNF                   (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_BEARER_TECHNOLOGY_NOTIFICATION)                   /**< The result of setting notification of the bearer technology characteristic with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_SET_BEARER_URI_SCHEMES_SUPPORTED_LIST_NOTIFICATION_CNF   (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_BEARER_URI_SCHEMES_SUPPORTED_LIST_NOTIFICATION)   /**< The result of setting notification of the bearer URI schemes supported list characteristic with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_SET_BEARER_SIGNAL_STRENGTH_NOTIFICATION_CNF              (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_BEARER_SIGNAL_STRENGTH_NOTIFICATION)              /**< The result of setting notification of the bearer signal strength characteristic with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_SET_BEARER_LIST_CURRENT_CALLS_NOTIFICATION_CNF           (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_BEARER_LIST_CURRENT_CALLS_NOTIFICATION)           /**< The result of setting notification of the bearer list current calls characteristic with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_SET_STATUS_FLAGS_NOTIFICATION_CNF                        (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_STATUS_FLAGS_NOTIFICATION)                        /**< The result of setting notification of the status flags characteristic with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_SET_INCOMING_CALL_TARGET_BEARER_URI_NOTIFICATION_CNF     (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_INCOMING_CALL_TARGET_BEARER_URI_NOTIFICATION)     /**< The result of setting notification of the incoming call target bearer URI characteristic with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_SET_CALL_STATE_NOTIFICATION_CNF                          (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_CALL_STATE_NOTIFICATION)                          /**< The result of setting notification of the call state characteristic with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_SET_TERMINATION_REASON_NOTIFICATION_CNF                  (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_TERMINATION_REASON_NOTIFICATION)                  /**< The result of setting notification of the termination reason characteristic with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_SET_INCOMING_CALL_NOTIFICATION_CNF                       (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_INCOMING_CALL_NOTIFICATION)                       /**< The result of setting notification of the incoming call characteristic with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_SET_CALL_FRIENDLY_NAME_NOTIFICATION_CNF                  (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_CALL_FRIENDLY_NAME_NOTIFICATION)                  /**< The result of setting notification of the call friendly name characteristic with payload #ble_ccp_event_parameter_t. */
typedef uint32_t ble_ccp_event_t;

/**
 * @brief The GTBS service index.
 */
#define BLE_CCP_SERVICE_INDEX_GTBS              (0xFF)  /**< The service index of GTBS. */
#define BLE_CCP_GTBS_INDEX                       (0xFF)  /**< Using the service index BLE_CCP_GTBS_INDEX is to use GTBS as the service. */

/**
 * @}
 */

/**
 * @defgroup Bluetoothble_CCP_struct Struct
 * @{
 * This section defines the data structures of the CCP.
 */

/**
 *  @brief This structure defines the parameter data type of the set notification actions.
 */
typedef struct {
    bool enable;    /**< Enable notification.*/
} ble_ccp_action_set_notification_t;

/**
 *  @brief This structure defines the parameter data type for the bearer list current calls.
 */
BT_PACKED(
typedef struct {
    uint8_t item_length;                    /**< Item length. */
    uint8_t call_index;                     /**< Call index. */
    ble_tbs_state_t call_state;             /**< Call state. */
    ble_tbs_call_flags_t call_flags;   /**< Call flags. */
    uint8_t uri[1];                           /**< Call URI. */
}) ble_ccp_bearer_list_current_calls_t;

/**
 *  @brief This structure defines the parameter data type for the status flags.
 */
BT_PACKED(
typedef struct {
    ble_tbs_in_band_ringtone_status_t in_band_ringtone: 1;           /**< The in-band ringtone status in server. */
    ble_tbs_silent_mode_status_t silent_mode: 1;                     /**< The silent mode status in server. */
    uint8_t reserved: 6;                                             /**< Reserved. */
    uint8_t reserved_byte;                                           /**< Reserved bytes. */
}) ble_ccp_status_flags_t;

/**
 *  @brief This structure defines the parameter data type for the incoming call target bearer URI.
 */
BT_PACKED(
typedef struct {
    uint8_t call_index;                                    /**< Call index. */
    uint8_t incoming_call_target_uri[1];                     /**< Incoming call target URI. */
}) ble_ccp_incoming_call_target_bearer_uri_t;

/**
 *  @brief This structure defines the data type of the call state.
 */
BT_PACKED(
typedef struct {
    uint8_t call_index;                /**< Call index. */
    ble_tbs_state_t state;             /**< Call state. */
    ble_tbs_call_flags_t call_flags;   /**< Call flags. */
}) ble_ccp_call_state_t;

/**
 *  @brief This structure defines the parameter data type for call control point optional opcodes.
 */
BT_PACKED(
typedef struct {
    ble_tbs_local_hold_feature_t local_hold: 1;                      /**< The local hold feature. */
    ble_tbs_join_feature_t join: 1;                                  /**< The join feature. */
    uint8_t reserved: 6;                                             /**< Reserved. */
    uint8_t reserved_byte;                                           /**< Reserved bytes. */
}) ble_ccp_call_control_point_optional_opcodes_t;

/**
 *  @brief This structure defines the parameter data type for incoming call.
 */
BT_PACKED(
typedef struct {
    uint8_t call_index;                     /**< Call index. */
    uint8_t uri[1];                           /**< URI. */
}) ble_ccp_incoming_call_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_CALL_FRIENDLY_NAME_CNF and #BLE_CCP_CALL_FRIENDLY_NAME_IND.
 */
BT_PACKED(
typedef struct {
    uint8_t call_index;                     /**< Call index. */
    uint8_t friendly_name[1];               /**< The call friendly name. */
}) ble_ccp_call_friendly_name_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_BEARER_PROVIDER_NAME_IND.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    uint16_t length;                /**< The length of the bearer provider name. */
    uint8_t *bearer_provider_name;  /**< Bearer provider name. */
} ble_ccp_bearer_provider_name_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_BEARER_TECHNOLOGY_IND.
 */
typedef struct {
    bt_handle_t handle;                 /**< Connection handle. */
    uint8_t service_idx;                /**< Service idx. */
    ble_tbs_technology_t technology;    /**< Bearer technology. */
} ble_ccp_bearer_technology_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_BEARER_URI_SCHEMES_SUPPORTED_LIST_IND.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    uint16_t length;                /**< The length of bearer URI schemes string list. */
    uint8_t *schemes_string;        /**< Bearer URI schemes string list. */
} ble_ccp_bearer_uri_schemes_supported_list_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_BEARER_SIGNAL_STRENGTH_IND.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    uint8_t signal_strength;        /**< The level of signal strength, which the range is from 0 to 255. */
} ble_ccp_bearer_signal_strength_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_BEARER_LIST_CURRENT_CALLS_IND.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    uint16_t length;                /**< The length of the current call list. */
    uint8_t *current_call_list;     /**< Current call list with structure t#ble_ccp_bearer_list_current_calls_t. */
} ble_ccp_bearer_list_current_calls_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_STATUS_FLAGS_IND.
 */
typedef struct {
    bt_handle_t handle;                     /**< Connection handle. */
    uint8_t service_idx;                    /**< Service idx. */
    ble_ccp_status_flags_t status_flags;    /**< Status flags. */
} ble_ccp_status_flags_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_INCOMING_CALL_TARGET_BEARER_URI_IND.
 */
typedef struct {
    bt_handle_t handle;                     /**< Connection handle. */
    uint8_t service_idx;                    /**< Service idx. */
    uint16_t length;                        /**< The length of the incoming call target bearer URI. */
    ble_ccp_incoming_call_target_bearer_uri_t *incoming_call;   /**< Incoming call target bearer URI. */
} ble_ccp_incoming_call_target_bearer_uri_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_CALL_STATE_IND.
 */
typedef struct {
    bt_handle_t handle;                     /**< Connection handle. */
    uint8_t service_idx;                    /**< Service idx. */
    uint16_t length;                        /**< The total length of call state list. */
    ble_ccp_call_state_t *call_state_list;  /**< The call state list. */
} ble_ccp_call_state_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_TERMINATION_REASON_IND.
 */
typedef struct {
    bt_handle_t handle;                     /**< Connection handle. */
    uint8_t service_idx;                    /**< Service idx. */
    uint8_t call_index;                     /**< Call index. */
    ble_tbs_termination_reason_t reason;    /**< Call termination reason. */
} ble_ccp_termination_reason_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_INCOMING_CALL_IND.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    uint16_t length;                /**< Incoming call information length. */
    ble_ccp_incoming_call_t *incoming_call;  /**< Incoming call information. */
} ble_ccp_incoming_call_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_CALL_FRIENDLY_NAME_IND.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    uint16_t length;                /**< Call friendly name length. */
    ble_ccp_call_friendly_name_t *friendly_name;               /**< The call friendly name. */
} ble_ccp_call_friendly_name_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_BEARER_PROVIDER_NAME_CNF.
 */
typedef ble_ccp_bearer_provider_name_ind_t ble_ccp_read_bearer_provider_name_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_BEARER_UCI_CNF.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    uint16_t length;                /**< Bearer provider name length. */
    uint8_t *uci;                   /**< Bearer UCI. */
} ble_ccp_read_bearer_uci_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_BEARER_TECHNOLOGY_CNF.
 */
typedef ble_ccp_bearer_technology_ind_t ble_ccp_read_bearer_technology_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CNF.
 */
typedef ble_ccp_bearer_uri_schemes_supported_list_ind_t ble_ccp_read_bearer_uri_schemes_supported_list_cnf_t;


/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_BEARER_SIGNAL_STRENGTH_CNF.
 */
typedef ble_ccp_bearer_signal_strength_ind_t ble_ccp_read_bearer_signal_strength_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL_CNF.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    uint8_t interval;               /**< The interval of reporting signal strength in second, which the range is from 0 to 255. */
} ble_ccp_read_bearer_signal_strength_reporting_interval_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_BEARER_LIST_CURRENT_CALLS_CNF.
 */
typedef ble_ccp_bearer_list_current_calls_ind_t ble_ccp_read_bearer_list_current_calls_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_CONTENT_CONTROL_ID_CNF.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    uint8_t ccid;                   /**< The content control ID. */
} ble_ccp_read_content_control_id_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_STATUS_FLAGS_CNF.
 */
typedef ble_ccp_status_flags_ind_t ble_ccp_read_status_flags_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_INCOMING_CALL_TARGET_BEARER_URI_CNF.
 */
typedef ble_ccp_incoming_call_target_bearer_uri_ind_t ble_ccp_read_incoming_call_target_bearer_uri_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_CALL_STATE_CNF.
 */
typedef ble_ccp_call_state_ind_t ble_ccp_read_call_state_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_CALL_CONTROL_POINT_OPTIONAL_OPCODES_CNF.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    ble_ccp_call_control_point_optional_opcodes_t call_control_point_optional_opcodes;  /**< Call control point optional opcodes. */
} ble_ccp_read_call_control_point_optional_opcodes_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_INCOMING_CALL_CNF.
 */
typedef ble_ccp_incoming_call_ind_t ble_ccp_read_incoming_call_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_READ_CALL_FRIENDLY_NAME_CNF.
 */
typedef ble_ccp_call_friendly_name_ind_t ble_ccp_read_call_friendly_name_cnf_t;

/**
 *  @brief This structure defines the parameter for CCP events.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
} ble_ccp_event_parameter_t;


/**
 * @}
 */


/**
 * @brief                       This function triggers call action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] action            is the action to be triggered.
 * @param[in] params            is the parameter of the related action.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation failed.
 *                              #BT_STATUS_UNSUPPORTED, the operation is unsupported.
 */
bt_status_t ble_ccp_send_action(bt_handle_t handle, ble_ccp_action_t action, void *params, uint8_t service_idx);

/**
 * @}
 * @}
 * @}
 */

#endif /* __BLE_CCP_H__ */

