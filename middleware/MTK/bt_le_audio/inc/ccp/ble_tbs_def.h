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

#ifndef __BLE_TBS_DEF_H__
#define __BLE_TBS_DEF_H__

#include "bt_type.h"

/**
 * @brief The service UUID of GTBS.
 */
#define BT_SIG_UUID16_GENERIC_TELEPHONE_BEARER_SERVICE (0x184C)    /**< Generic Telephone Bearer Service (GTBS) UUID. */

/**
 * @brief The service UUID of TBS.
 */
#define BT_SIG_UUID16_TELEPHONE_BEARER_SERVICE         (0x184B)    /**< Telephone Bearer Service (TBS) UUID. */

/**
 * @brief The UUID index definitions of TBS and GTBS.
 */
#define BLE_TBS_UUID_TYPE_TELEPHONE_BEARER_SERVICE                      0x00    /**< The UUID type of TBS.*/
#define BLE_TBS_UUID_TYPE_CHARC_START                                   0x01    /**< The first characteristic UUID type.*/
#define BLE_TBS_UUID_TYPE_BEARER_PROVIDER_NAME                          BLE_TBS_UUID_TYPE_CHARC_START    /**< The UUID type of TBS bearer provider name.*/
#define BLE_TBS_UUID_TYPE_BEARER_UCI                                    0x02    /**< The UUID type of TBS bearer UCI.*/
#define BLE_TBS_UUID_TYPE_BEARER_TECHNOLOGY                             0x03    /**< The UUID type of TBS bearer technology.*/
#define BLE_TBS_UUID_TYPE_BEARER_URI_SCHEMES_SUPPORTED_LIST             0x04    /**< The UUID type of TBS bearer uri schemes supported list.*/
#define BLE_TBS_UUID_TYPE_BEARER_SIGNAL_STRENGTH                        0x05    /**< The UUID type of TBS bearer signal strength.*/
#define BLE_TBS_UUID_TYPE_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL     0x06    /**< The UUID type of TBS bearer signal strength reporting interval.*/
#define BLE_TBS_UUID_TYPE_BEARER_LIST_CURRENT_CALLS                     0x07    /**< The UUID type of TBS bearer list current calls.*/
#define BLE_TBS_UUID_TYPE_CONTENT_CONTROL_ID                            0x08    /**< The UUID type of TBS content control ID.*/
#define BLE_TBS_UUID_TYPE_STATUS_FLAGS                                  0x09    /**< The UUID type of TBS status flags.*/
#define BLE_TBS_UUID_TYPE_INCOMING_CALL_TARGET_BEARER_URI               0x0A    /**< The UUID type of TBS incoming call target bearer URI.*/
#define BLE_TBS_UUID_TYPE_CALL_STATE                                    0x0B    /**< The UUID type of TBS call state.*/
#define BLE_TBS_UUID_TYPE_CALL_CONTROL_POINT                            0x0C    /**< The UUID type of TBS call control point.*/
#define BLE_TBS_UUID_TYPE_CALL_CONTROL_POINT_OPTIONAL_OPCODES           0x0D    /**< The UUID type of TBS call control point optional opcodes.*/
#define BLE_TBS_UUID_TYPE_TERMINATION_REASON                            0x0E    /**< The UUID type of TBS termination reason.*/
#define BLE_TBS_UUID_TYPE_INCOMING_CALL                                 0x0F    /**< The UUID type of TBS incoming call.*/
#define BLE_TBS_UUID_TYPE_CALL_FRIENDLY_NAME                            0x10    /**< The UUID type of TBS call friendly name.*/
#define BLE_TBS_UUID_TYPE_MAX_NUM                                       0x11    /**< The max number of TBS UUID type.*/
#define BLE_TBS_UUID_TYPE_GENERIC_TELEPHONE_BEARER_SERVICE              0xE0    /**< The UUID type of GTBS.*/
#define BLE_TBS_UUID_TYPE_INVALID                                       0xFF    /**< The TBS invalid UUID type.*/
typedef uint8_t ble_tbs_uuid_t;                                                 /**< The type of TBS UUID.*/

/**
 * @brief The TBS max number of characteristics.
 */
#define BLE_TBS_MAX_CHARC_NUMBER                     (BLE_TBS_UUID_TYPE_MAX_NUM-1)   /**< The max number of TBS characteristics.*/

/**
 * @brief Defines for call index.
 */
#define BLE_TBS_INVALID_CALL_INDEX   0                       /**<  Invalid call index. */
typedef uint8_t ble_tbs_call_index_t;                        /**<  The type of call index.*/

/**
 * @brief Defines for technology.
 */
#define BLE_TBS_TECHNOLOGY_3G                           0x01    /**<  3G. */
#define BLE_TBS_TECHNOLOGY_4G                           0x02    /**<  4G. */
#define BLE_TBS_TECHNOLOGY_LTE                          0x03    /**<  LTE. */
#define BLE_TBS_TECHNOLOGY_WIFI                         0x04    /**<  WIFI. */
#define BLE_TBS_TECHNOLOGY_5G                           0x05    /**<  5G. */
#define BLE_TBS_TECHNOLOGY_GSM                          0x06    /**<  GSM. */
#define BLE_TBS_TECHNOLOGY_CDMA                         0x07    /**<  CDMA. */
#define BLE_TBS_TECHNOLOGY_2G                           0x08    /**<  2G. */
#define BLE_TBS_TECHNOLOGY_WCDMA                        0x09    /**<  WCDMA. */
#define BLE_TBS_TECHNOLOGY_IP                           0x0A    /**<  IP. */
typedef uint8_t ble_tbs_technology_t;                           /**<  The type of technology.*/

/**
 * @brief Defines for in-band ringtone feature.
 */
#define BLE_TBS_IN_BAND_RINGTONE_DISABLED                    0       /**<  In-band ringtone disabled. */
#define BLE_TBS_IN_BAND_RINGTONE_ENABLED                     1       /**<  In-band ringtone enabled. */
typedef uint8_t ble_tbs_in_band_ringtone_status_t;                  /**<  The type of in-band ringtone status.*/

/**
 * @brief Defines for silent mode status.
 */
#define BLE_TBS_SERVER_NOT_IN_SILENT_MODE                    0       /**<  Server is not in silent mode. */
#define BLE_TBS_SERVER_IN_SILENT_MODE                        1       /**<  Server is in silent mode. */
typedef uint8_t ble_tbs_silent_mode_status_t;                        /**<  The type of silent mode status.*/

/**
 * @brief Defines for status flags supported feature.
 */
#define BLE_TBS_STATUS_DISABLED                              0x0000  /**<  status flags disable. */
#define BLE_TBS_STATUS_IN_BAND_RINGTONE_ENABLED              0x0001  /**<  In-band ringtone enabled. */
#define BLE_TBS_STATUS_SERVER_IN_SILENT_MODE                 0x0002  /**<  Server is in silent mode. */
typedef uint16_t ble_tbs_status_flags_t;                             /**<  The type of status flags.*/

/**
 * @brief Defines for local hold call control point opcoes supported feature.
 */
#define BLE_TBS_LOCAL_HOLD_NOT_SUPPORTED                     0       /**<  local hold is not supported. */
#define BLE_TBS_LOCAL_HOLD_SUPPORTED                         1       /**<  local hold is supported. */
typedef uint8_t ble_tbs_local_hold_feature_t;                        /**<  The type of local hold call feature.*/

/**
 * @brief Defines for local retrieve call control point opcoes supported feature.
 */
#define BLE_TBS_LOCAL_RETRIEVE_NOT_SUPPORTED                 0       /**<  local retrieve is not supported. */
#define BLE_TBS_LOCAL_RETRIEVE_SUPPORTED                     1       /**<  local retrieve is supported. */
typedef uint8_t ble_tbs_local_retrieve_feature_t;                    /**<  The type of local retrieve call feature.*/

/**
 * @brief Defines for join call control point opcoes supported feature.
 */
#define BLE_TBS_JOIN_NOT_SUPPORTED                           0       /**<  join call is not supported. */
#define BLE_TBS_JOIN_SUPPORTED                               1       /**<  join call is supported. */
typedef uint8_t ble_tbs_join_feature_t;                              /**<  The type of join call feature.*/

/**
 * @brief Defines for call control point optional opcoes supported feature.
 */
#define BLE_TBS_OPTIONAL_OPCODES_NOT_SUPPORTED               0x0000  /**<  optional opcodes not supported. */
#define BLE_TBS_LOCAL_HOLD_AND_LOCAL_RETRIEVE_SUPPORTED      0x0001  /**<  local hold and local retrieveis supported. */
#define BLE_TBS_OPTIONAL_JOIN_SUPPORTED                      0x0002  /**<  join call is supported. */
typedef uint16_t ble_tbs_optional_opcodes_feature_t;                 /**<  The type of call control point optional opcoes feature.*/

/**
 * @brief Defines for TBS state.
 */
#define BLE_TBS_STATE_INCOMING                         0x00    /**<  A remote party is calling (incoming call). */
#define BLE_TBS_STATE_DIALING                          0x01    /**<  The process to call the remote party has started on the server but the remote party is not being alerted (outgoing call). */
#define BLE_TBS_STATE_ALERTING                         0x02    /**<  A remote party is being alerted (outgoing call). */
#define BLE_TBS_STATE_ACTIVE                           0X03    /**<  The call is in an active conversation. */
#define BLE_TBS_STATE_LOCALLY_HELD                     0x04    /**<  The call is connected but held locally. */
#define BLE_TBS_STATE_REMOTELY_HELD                    0X05    /**<  The call is connected but held remotely. */
#define BLE_TBS_STATE_LOCALLY_AND_REMOTELY_HELD        0x06    /**<  The call is connected but held both locally and remotely. */
#define BLE_TBS_STATE_IDLE                             0x07    /**<  Call service idle. */
#define BLE_TBS_STATE_NOTIFY_ACTIVE_DEVICE             0xFE    /**<  Notify call state to active device,not a realy call state. */
#define BLE_TBS_STATE_INVALID                          0xFF    /**<  Invalid call state. */
typedef uint8_t ble_tbs_state_t;                               /**<  The type of TBS state.*/

/**
 * @brief Defines for call flags.
 */
#define BLE_TBS_CALL_FLAGS_INCOMING 0                        /**<  Call is an incoming call. */
#define BLE_TBS_CALL_FLAGS_OUTGOING 1                        /**<  Call is an outgoing call. */
#define BLE_TBS_CALL_FLAGS_NOT_WITHHELD 0                    /**<  Not withheld. */
#define BLE_TBS_CALL_FLAGS_WITHHELD 2                        /**<  Withheld. */
#define BLE_TBS_CALL_FLAGS_PROVIDED_BY_NETWORK 0             /**<  Provided by network call. */
#define BLE_TBS_CALL_FLAGS_WITHHELD_BY_NETWORK 4             /**<  Withheld by network call. */
typedef uint8_t ble_tbs_call_flags_t;                        /**<  The type of call flags.*/
typedef union{
    struct{
        uint8_t outgoing_call:1;                /**< Bit0:Incoming/Outgoing. */
        uint8_t info_withheld_by_server:1;      /**< Bit1:information withheld by server. */
        uint8_t info_withheld_by_network:1;     /**< Bit2:information withheld by network. */
        uint8_t reserved:5;                     /**< Bit1~7:RFU. */
    }call_flags_map;
    uint8_t call_flags;
}ble_tbs_call_flag_t;
/**
 * @brief Defines for call control opcode.
 */
#define BLE_TBS_CALL_CONTROL_OPCODE_TYPE_ACCEPT              0x00    /**<  Answer the incoming call. */
#define BLE_TBS_CALL_CONTROL_OPCODE_TYPE_TERMINATE           0x01    /**<  End the currently active, calling (outgoing), alerting, or held (locally or remotely) call. */
#define BLE_TBS_CALL_CONTROL_OPCODE_TYPE_LOCAL_HOLD          0x02    /**<  Place the currently active or alerting call on local hold. */
#define BLE_TBS_CALL_CONTROL_OPCODE_TYPE_LOCAL_RETRIEVE      0x03    /**<  Move a locally held call to an active call.Move a locally and remotely held call to a remotely held call. */
#define BLE_TBS_CALL_CONTROL_OPCODE_TYPE_ORIGINATE           0x04    /**<  Initiate a call by URI. */
#define BLE_TBS_CALL_CONTROL_OPCODE_TYPE_JOIN                0x05    /**<  Put a call (in any state) to the active state and join all calls that are in the active state. */
#define BLE_TBS_CALL_CONTROL_OPCODE_INVALID                  0xFF    /**<  Invalid call control opcode. */
typedef uint8_t ble_tbs_call_control_opcode_t;                       /**<  The type of call control opcode.*/

/**
 * @brief Defines for call control result.
 */
#define BLE_TBS_CALL_CONTROL_RESULT_SUCCESS                  0x00    /**<  Opcode write was successful. */
#define BLE_TBS_CALL_CONTROL_RESULT_OPCODE_NOT_SUPPORTED     0x01    /**<  An invalid opcode was used for the Call Control Point write. */
#define BLE_TBS_CALL_CONTROL_RESULT_OPERATION_NOT_POSSIBLE   0x02    /**<  Requested operation cannot be completed. */
#define BLE_TBS_CALL_CONTROL_RESULT_INVALID_CALL_INDEX       0x03    /**<  The Call index used for the Call Control Point write is invalid. */
#define BLE_TBS_CALL_CONTROL_RESULT_STATE_MISMATCH           0x04    /**<  The opcode written to the Call Control Point was received when the current Call State for the Call Index is not in the expected state. */
#define BLE_TBS_CALL_CONTROL_RESULT_LACK_OF_RESOURCES        0x05    /**<  Lack of internal resources to complete the requested action. */
#define BLE_TBS_CALL_CONTROL_RESULT_INVALID_OUTGOING_URI     0x06    /**<  The Outgoing URI is incorrect or invalid when an Originate opcode is sent. */
typedef uint8_t ble_tbs_call_control_result_t;                       /**<  The type of call control result.*/

/**
 * @brief Defines for call termination reason.
 */
#define BLE_TBS_TERMINATION_REASON_OUTGOING_URI_ERROR         0x00    /**<  URI value used to originate a call was formed improperly. */
#define BLE_TBS_TERMINATION_REASON_CALL_FAIL                  0x01    /**<  Call fail. */
#define BLE_TBS_TERMINATION_REASON_REMOTE_PARTY_END_CALL      0x02    /**<  Remote party ended call. */
#define BLE_TBS_TERMINATION_REASON_CALL_END_FROM_SERVER       0x03    /**<  Call ended from the server. */
#define BLE_TBS_TERMINATION_REASON_LINE_BUSY                  0x04    /**<  Line busy. */
#define BLE_TBS_TERMINATION_REASON_NEWORK_CONGESTION          0x05    /**<  Network congestion. */
#define BLE_TBS_TERMINATION_REASON_CLIENT_TERMINATED          0x06    /**<  Client terminated. */
#define BLE_TBS_TERMINATION_REASON_NO_SERVICE                 0x07    /**<  No service. */
#define BLE_TBS_TERMINATION_REASON_NO_ANSWER                  0x08    /**<  No answer. */
#define BLE_TBS_TERMINATION_REASON_UNSPECIFIED                0x09    /**<  Unspecified. */
typedef uint8_t ble_tbs_termination_reason_t;                         /**<  The type of call termination reason.*/

/**
 *  @brief This structure defines the UUID type and attribute handle.
 */
typedef struct {
    ble_tbs_uuid_t uuid_type;   /**<  UUID type. */
    uint16_t att_handle;        /**<  Attribute handle. */
} ble_tbs_attribute_handle_t;

/**
 *  @brief This structure defines the data type for originate call.
 */
BT_PACKED(
typedef struct {
    uint8_t uri[1];                 /**<  URI. */
}) ble_tbs_originate_call_t;

/**
 *  @brief This structure defines the data type for join call.
 */
BT_PACKED(
typedef struct {
    uint8_t call_index_list[1];     /**<  Call index list. */
}) ble_tbs_join_call_t;

/**
 *  @brief This structure defines the data type of the call control command.
 */
BT_PACKED(
typedef struct {
    ble_tbs_call_control_opcode_t opcode;           /**< Call control opcode. */
    union {
        uint8_t call_index;                         /**<  Call index list. */
        ble_tbs_originate_call_t originate_call;    /**<  Originate call data. */
        ble_tbs_join_call_t join_call;              /**<  Join call data. */
    } params;
}) ble_tbs_call_control_point_t;

#endif /* __BLE_TBS_DEF_H__ */

