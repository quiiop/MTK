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

#ifndef __BT_LE_AUDIO_SINK_H__
#define __BT_LE_AUDIO_SINK_H__

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothLeaudio LE_AUDIO
 * @{
 * This section introduces the LE Audio related operation, profiles and services.
 * @addtogroup BluetoothLeaudioSink LEAudioSink
 * @{
 * This section introduces the LE AUDIO definitions, API prototypes, request and response parameter structures.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b CCP                        | Call Control Profile. |
 * |\b MCP                        | Media Control Profile. |
 * |\b UCI                        | Uniform Caller Identifier. |
 * |\b URI                        | Uniform Resource Identifier. |
 *
 */
#include "bt_le_audio_type.h"
#include "bt_le_audio_def.h"
#include "bt_le_audio_util.h"

#include "ble_ccp.h"
#include "ble_mcp.h"

/**
 * @defgroup Bluetooth_Leaudio_define Define
 * @{
 * This section defines the LE AUDIO sink action options and events.
 */
/**
 *  @brief LE AUDIO sink actions.
 */
#define BT_LE_AUDIO_SINK_ACTION_CALL_ACCEPT                                 (BT_LE_AUDIO_MODULE_CALL|0x0001U)      /**< This action sends a request to accept call with parameter #bt_le_audio_sink_call_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_CALL_TERMINATE                              (BT_LE_AUDIO_MODULE_CALL|0x0002U)      /**< This action sends a request to terminate call with parameter #bt_le_audio_sink_call_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_CALL_HOLD                                   (BT_LE_AUDIO_MODULE_CALL|0x0003U)      /**< This action sends a request to hold call with parameter #bt_le_audio_sink_call_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_CALL_RETRIEVE                               (BT_LE_AUDIO_MODULE_CALL|0x0004U)      /**< This action sends a request to retrieve call with parameter #bt_le_audio_sink_call_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_CALL_ORIGINATE                              (BT_LE_AUDIO_MODULE_CALL|0x0005U)      /**< This action sends a request to originate call with parameter #bt_le_audio_sink_call_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_CALL_JOIN                                   (BT_LE_AUDIO_MODULE_CALL|0x0006U)      /**< This action sends a request to join call with parameter #bt_le_audio_sink_call_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_CALL_DIAL_LAST                              (BT_LE_AUDIO_MODULE_CALL|0x0007U)      /**< This action sends a request to dail last call with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_CALL_DIAL_MISSED                            (BT_LE_AUDIO_MODULE_CALL|0x0008U)      /**< This action sends a request to dail missed call with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_CALL_HOLD_ALL_ACTIVE_ACCEPT_OTHERS          (BT_LE_AUDIO_MODULE_CALL|0x0009U)      /**< This action sends a request to hold all active calls and accept other calls with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_CALL_RELEASE_ALL_HELD                       (BT_LE_AUDIO_MODULE_CALL|0x000AU)      /**< This action sends a request to release all held calls with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_PLAY                                  (BT_LE_AUDIO_MODULE_MEDIA|0x0001U)     /**< This action sends a request to play media with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_PAUSE                                 (BT_LE_AUDIO_MODULE_MEDIA|0x0002U)     /**< This action sends a request to pause media with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_STOP                                  (BT_LE_AUDIO_MODULE_MEDIA|0x0003U)     /**< This action sends a request to stop media with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_NEXT_TRACK                            (BT_LE_AUDIO_MODULE_MEDIA|0x0004U)     /**< This action sends a request to play next track with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_PREVIOUS_TRACK                        (BT_LE_AUDIO_MODULE_MEDIA|0x0005U)     /**< This action sends a request to play previous track with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_FIRST_TRACK                           (BT_LE_AUDIO_MODULE_MEDIA|0x0006U)     /**< This action sends a request to play first track with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_LAST_TRACK                            (BT_LE_AUDIO_MODULE_MEDIA|0x0007U)     /**< This action sends a request to play last track with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_FAST_FORWARD                          (BT_LE_AUDIO_MODULE_MEDIA|0x0008U)     /**< This action sends a request to fast forward with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_FAST_REWIND                           (BT_LE_AUDIO_MODULE_MEDIA|0x0009U)     /**< This action sends a request to fast rewind with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_READ_MEDIA_STATE                      (BT_LE_AUDIO_MODULE_MEDIA|0x000AU)     /**< This action sends a request to read media state with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_READ_MEDIA_CONTROL_OPCODES_SUPPORTED  (BT_LE_AUDIO_MODULE_MEDIA|0x000BU)     /**< This action sends a request to read media contol opcodes supported with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_MEDIA_READ_CURRENT_TRACK_OBJECT_INFORMATION (BT_LE_AUDIO_MODULE_MEDIA|0x000CU)     /**< This action sends a request to read current track object information with parameter #bt_le_audio_sink_action_param_t. */
#define BT_LE_AUDIO_SINK_ACTION_INVALID                                     (0xFFFF)                               /**< Invalid LE AUDIO sink action. */
typedef uint16_t bt_le_audio_sink_action_t; /**< The type of the LE AUDIO sink action. */

/**
 *  @brief LE AUDIO sink events.
 */
#define BT_LE_AUDIO_SINK_EVENT_CONNECTED                        (BT_LE_AUDIO_MODULE_COMMON|0x0001U)        /**< This event indicates the LE AUDIO connection is created with parameter #bt_le_audio_sink_event_connected_t. */
#define BT_LE_AUDIO_SINK_EVENT_DISCONNECTED                     (BT_LE_AUDIO_MODULE_COMMON|0x0002U)        /**< This event indicates the LE AUDIO connection is disconnected with parameter #bt_le_audio_sink_event_disconnected_t. */
#define BT_LE_AUDIO_SINK_EVENT_CALL_SERVICE_READY               (BT_LE_AUDIO_MODULE_CALL|0x0001U)          /**< This event indicates the call service ready with parameter #bt_le_audio_sink_event_service_ready_t. */
#define BT_LE_AUDIO_SINK_EVENT_CALL_ACTION_FAIL                 (BT_LE_AUDIO_MODULE_CALL|0x0002U)          /**< This event indicates the call action fail with parameter #bt_le_audio_sink_event_service_ready_t. */
#define BT_LE_AUDIO_SINK_EVENT_CALL_UCI                         (BT_LE_AUDIO_MODULE_CALL|0x0003U)          /**< This event indicates the call UCI information with parameter #bt_le_audio_sink_event_call_uci_t. */
#define BT_LE_AUDIO_SINK_EVENT_CALL_MISSED_CALL                 (BT_LE_AUDIO_MODULE_CALL|0x0004U)          /**< This event indicates the missed call information with parameter #bt_le_audio_sink_event_call_missed_call_t. */
#define BT_LE_AUDIO_SINK_EVENT_CALL_STATE                       (BT_LE_AUDIO_MODULE_CALL|0x0005U)          /**< This event indicates the call state changed with parameter #bt_le_audio_sink_event_call_state_t. */
#define BT_LE_AUDIO_SINK_EVENT_CALL_LIST                        (BT_LE_AUDIO_MODULE_CALL|0x0006U)          /**< This event indicates the call list changed with parameter #bt_le_audio_sink_event_call_state_t. */
#define BT_LE_AUDIO_SINK_EVENT_CALL_INCOMING_CALL               (BT_LE_AUDIO_MODULE_CALL|0x0007U)          /**< This event indicates the incoming call information with parameter #bt_le_audio_sink_event_call_incoming_call_t. */
#define BT_LE_AUDIO_SINK_EVENT_MEDIA_SERVICE_READY              (BT_LE_AUDIO_MODULE_MEDIA|0x0001U)         /**< This event indicates the media service ready with parameter #bt_le_audio_sink_event_service_ready_t. */
#define BT_LE_AUDIO_SINK_EVENT_MEDIA_ACTION_FAIL                (BT_LE_AUDIO_MODULE_MEDIA|0x0002U)         /**< This event indicates the media action fail with parameter #bt_le_audio_sink_event_media_action_fail_t. */
#define BT_LE_AUDIO_SINK_EVENT_MEDIA_STATE                      (BT_LE_AUDIO_MODULE_MEDIA|0x0003U)         /**< This event indicates the media state changed with parameter #bt_le_audio_sink_event_media_state_t. */
#define BT_LE_AUDIO_SINK_EVENT_MEDIA_SUSPEND                    (BT_LE_AUDIO_MODULE_MEDIA|0x0004U)         /**< This event indicates meida is suspend by high level app*/
#define BT_LE_AUDIO_SINK_EVENT_MEDIA_RESUME                     (BT_LE_AUDIO_MODULE_MEDIA|0x0005U)         /**< This event indicates meida is resumed after high level app release audio source */

typedef uint16_t bt_le_audio_sink_event_t;  /**< The type of the LE AUDIO sink events. */
/**
 * @}
 */

/**
 * @defgroup Bluetooth_Leaudio_struct struct
 * @{
 * This section defines structure of the LE AUDIO sink.
 */

/**
 *  @brief This structure defines the parameter data type of call list.
 */
typedef struct {
    uint16_t length;                                    /**< Length.*/
    ble_ccp_bearer_list_current_calls_t *current_calls; /**< Current calls.*/
} bt_le_audio_sink_call_list_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SINK_EVENT_CONNECTED.
 */
typedef struct {
    bt_handle_t handle;     /**< Connection handle.*/
} bt_le_audio_sink_event_connected_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SINK_EVENT_DISCONNECTED.
 */
typedef struct {
    bt_handle_t handle;     /**< Connection handle.*/
//MTK_COMMON: Let disconnection has its own struct to append disconnection reason code
    bt_status_t reason;     /**< Disconnection reason code. */
//(end MTK_COMMON)
} bt_le_audio_sink_event_disconnected_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SINK_EVENT_CALL_SERVICE_READY and #BT_LE_AUDIO_SINK_EVENT_MEDIA_SERVICE_READY.
 */
typedef struct {
    bt_handle_t handle;     /**< Connection handle.*/
    bt_status_t status;     /**< Service status.*/
    uint8_t service_num;    /**< Number of services.*/
} bt_le_audio_sink_event_service_ready_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SINK_EVENT_CALL_ACTION_FAIL.
 */
typedef struct {
    bt_handle_t handle;     /**< Connection handle.*/
    uint8_t action;         /**< Action.*/
    uint8_t service_idx;    /**< Service index.*/
} bt_le_audio_sink_event_call_action_fail_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SINK_EVENT_CALL_UCI.
 */
typedef struct {
    bt_handle_t handle;     /**< Connection handle.*/
    uint8_t uci[1];         /**< UCI.*/
} bt_le_audio_sink_event_call_uci_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SINK_EVENT_CALL_MISSED_CALL.
 */
typedef struct {
    bt_handle_t handle;     /**< Connection handle.*/
    uint8_t service_idx;    /**< Service index.*/
    uint16_t uri_length;    /**< Length of URI.*/
    uint8_t *uri;           /**< URI.*/
} bt_le_audio_sink_event_call_missed_call_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SINK_EVENT_CALL_STATE.
 */
typedef struct {
    bt_handle_t handle;                     /**< Connection handle.*/
    uint8_t service_idx;                    /**< Service index.*/
    uint8_t call_index;                     /**< Call index.*/
    bt_le_audio_call_state_t prev_state;    /**< Previous state.*/
    bt_le_audio_call_state_t cur_state;     /**< Current state.*/
    uint16_t uri_length;                    /**< Length of URI.*/
    uint8_t *uri;                         /**< URI.*/
} bt_le_audio_sink_event_call_state_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SINK_EVENT_CALL_LIST.
 */
typedef struct {
    bt_handle_t handle;                     /**< Connection handle.*/
    uint8_t service_idx;                    /**< Service index.*/
    bt_le_audio_sink_call_list_t call_list; /**< Call list.*/
} bt_le_audio_sink_event_call_list_t;

/**
 *  @brief This structure defines the parameter data type of event #BT_LE_AUDIO_SINK_EVENT_CALL_INCOMING_CALL.
 */
typedef struct {
    bt_handle_t handle;                     /**< Connection handle. */
    uint8_t service_idx;                    /**< Service index. */
    uint16_t uri_length;                    /**< Length of URI. */
    uint8_t *uri;                           /**< URI. */
} bt_le_audio_sink_event_call_incoming_call_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SINK_EVENT_MEDIA_ACTION_FAIL.
 */
typedef struct {
    bt_handle_t handle; /**< Connection handle.*/
} bt_le_audio_sink_event_media_action_fail_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SINK_EVENT_MEDIA_STATE.
 */
typedef struct {
    bt_handle_t handle; /**< Connection handle.*/
    uint8_t media_id;   /**< Media ID.*/
    uint8_t state;      /**< Media state.*/
} bt_le_audio_sink_event_media_state_t;

/**
 *  @brief This structure defines the parameter data type for call and media actions.
 */
typedef struct {
    uint8_t service_idx;    /**< Service index.*/
} bt_le_audio_sink_action_param_t;

/**
 *  @brief This structure defines the parameter data type for call actions.
 */
typedef struct {
    uint8_t service_idx;                                /**< Service index.*/
    uint16_t length;                                    /**< The call control list length. */
    ble_tbs_call_control_point_t *call_control_point;   /**< The call control list. */
} bt_le_audio_sink_call_action_param_t;

/**
 * @brief The LE AUDIO sink event callback.
 * @param[in] event             is the event ID, refer to #bt_le_audio_sink_event_t.
 * @param[in] msg               is the event message.
 */
typedef void (*bt_le_audio_sink_callback_t)(bt_le_audio_sink_event_t event, void *msg);

/**
 * @}
 */

/**
 * @brief                       This function initializes LE AUDIO sink.
 * @param[in] role              is the role of the device, refer to ble_tmap_role_t.
 * @param[in] callback          is the function pointer of the callback.
 * @param[in] max_link_num      is the maximum of link that LE AUDIO sink supported.
 * @return                      BT_STATUS_SUCCESS, the operation completed successfully.
 *                              BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t bt_le_audio_sink_init(ble_tmap_role_t role, bt_le_audio_sink_callback_t callback, uint8_t max_link_num);

/**
 * @brief                       This function triggers call, media or volume action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] action            is the action to be triggered.
 * @param[in] params            is the parameter of the related action.
 * @return                      BT_STATUS_SUCCESS, the operation completed successfully.
 *                              BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t bt_le_audio_sink_send_action(bt_handle_t handle, bt_le_audio_sink_action_t action, void *params);

/**
 * @brief                       This function gets the call list.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service index.
 * @return                      is the list of current calls.
 */
bt_le_audio_sink_call_list_t *bt_le_audio_sink_call_get_call_list(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function checks the call state which is valid in call list or not.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service index.
 * @param[in] state             is the call state.
 * @return                      is the call index valid in call list.
 */
ble_tbs_call_index_t bt_le_audio_sink_call_check_state(bt_handle_t handle, uint8_t service_idx, ble_tbs_state_t state);

/**
 * @brief                       This function return the first valid call index.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service index.
 * @return                      is the first valid call index in call list.
 */
ble_tbs_call_index_t bt_le_audio_sink_call_get_first(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function returns the first valid call state.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service index.
 * @return                      is the first valid call state information.
 */
ble_ccp_call_state_t *bt_le_audio_sink_call_get_first_call_state(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function gets media state.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service index.
 * @return                      is the media state.
 */
bt_le_audio_media_state_t bt_le_audio_sink_media_get_state(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function gets call status flags.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service index.
 * @return                      the status flags.
 */
ble_ccp_status_flags_t bt_le_audio_sink_call_get_status_flags(bt_handle_t handle, uint8_t service_idx);

/**
 * @}
 * @}
 * @}
 */

#endif  /* __BT_LE_AUDIO_SINK_H__ */

