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

#ifndef __BT_LE_AUDIO_SOURCE_H__
#define __BT_LE_AUDIO_SOURCE_H__

#include "bt_le_audio_type.h"
#include "bt_le_audio_def.h"
#include "ble_mcs.h"
#include "ble_tbs.h"

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothLeaudio LE_AUDIO
 * @{
 * This section introduces the LE Audio related operation, profiles and services.
 * @addtogroup BluetoothLeaudioSource LEAudioSource
 * @{
 * This section introduces the LE AUDIO source definitions, API prototypes, request and response parameter structures.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b TBS                        | Telephone Bearer Service. |
 * |\b MCS                        | Media Control Service. |
 * |\b URI                        | Uniform Resource Identifier. |
 *
 * @section bt_le_audio_source_api_usage How to use this module
 *  - Step1: Mandatory, initialize LE AUDIO source during system initialization.
 *   - Sample code:
 *    @code
 *           bt_le_audio_source_init((BLE_TMAP_ROLE_MASK_CG|BLE_TMAP_ROLE_MASK_UMS|BLE_TMAP_ROLE_MASK_BMS), app_le_audio_source_event_callback, MAX_LINK_NUM);
 *    @endcode
 *  - Step2: Mandatory, implement app_le_audio_source_event_callback() to handle the LE AUDIO source events, such as status changed and caller information, etc.
 *   - Sample code:
 *    @code
 *       static uint16_t app_le_audio_source_event_callback(bt_le_audio_source_event_t event, void *msg)
 *       {
 *           switch (event) {
 *               case BT_LE_AUDIO_SOURCE_EVENT_MEDIA_PLAYING: {
 *                   LE_AUDIO_MSGLOG_I("[APP_SOURCE] MEDIA PLAYING", 0);
 *                   break;
 *               }
 *               case BT_LE_AUDIO_SOURCE_EVENT_MEDIA_PAUSED: {
 *                   LE_AUDIO_MSGLOG_I("[APP_SOURCE] MEDIA PAUSED\n", 0);
 *                   break;
 *               }
 *               case BT_LE_AUDIO_SOURCE_EVENT_MEDIA_PREVIOUS_TRACK: {
 *                   LE_AUDIO_MSGLOG_I("[APP_SOURCE] MEDIA PREVIOUS TRACK\n", 0);
 *                   break;
 *               }
 *               case BT_LE_AUDIO_SOURCE_EVENT_MEDIA_NEXT_TRACK: {
 *                   LE_AUDIO_MSGLOG_I("[APP_SOURCE] MEDIA NEXT TRACK\n", 0);
 *                   break;
 *               }
 *           }
 *
 *           return 0;
 *       }
 *    @endcode
 *  - Step3: Optional, call the #bt_le_audio_source_send_action to play media.
 *   - Sample code:
 *    @code
 *        bt_le_audio_source_action_param_t le_param;
 *        le_param.service_index = 0xFF;
 *        bt_le_audio_source_send_action(BT_LE_AUDIO_SOURCE_ACTION_MEDIA_PLAY, &le_param);
 *    @endcode
 */

/**
 * @defgroup Bluetooth_Leaudio_source_define Define
 * @{
 * This section defines the LE AUDIO source action options and events.
 */

/**
 *  @brief LE AUDIO source actions.
 */
#define BT_LE_AUDIO_SOURCE_ACTION_CALL_ACCEPT                   (BT_LE_AUDIO_MODULE_CALL|0x0001U)      /**< This action sends a request to accept call with parameter #bt_le_audio_source_call_accept_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_CALL_TERMINATE                (BT_LE_AUDIO_MODULE_CALL|0x0002U)      /**< This action sends a request to terminate call with parameter #bt_le_audio_source_call_terminate_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_CALL_HOLD                     (BT_LE_AUDIO_MODULE_CALL|0x0003U)      /**< This action sends a request to hold call with parameter #bt_le_audio_source_call_hold_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_CALL_UNHOLD                   (BT_LE_AUDIO_MODULE_CALL|0x0004U)      /**< This action sends a request to unhold call with parameter #bt_le_audio_source_call_unhold_t. */

#define BT_LE_AUDIO_SOURCE_ACTION_CALL_ALERTING                 (BT_LE_AUDIO_MODULE_CALL|0x0005U)      /**< This action sends a request to set the call state to alerting with parameter #bt_le_audio_source_call_alerting_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_CALL_ACTIVE                   (BT_LE_AUDIO_MODULE_CALL|0x0006U)      /**< This action sends a request to set the call state to active with parameter #bt_le_audio_source_call_active_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_CALL_ENDED                    (BT_LE_AUDIO_MODULE_CALL|0x0007U)      /**< This action sends a request to set the call state to ended with parameter #bt_le_audio_source_call_ended_t. */

#define BT_LE_AUDIO_SOURCE_ACTION_MEDIA_PLAY                    (BT_LE_AUDIO_MODULE_MEDIA|0x0001U)     /**< This action sends a request to play media with parameter #bt_le_audio_source_action_param_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_MEDIA_PAUSE                   (BT_LE_AUDIO_MODULE_MEDIA|0x0002U)     /**< This action sends a request to pause media with parameter #bt_le_audio_source_action_param_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_MEDIA_STOP                    (BT_LE_AUDIO_MODULE_MEDIA|0x0003U)     /**< This action sends a request to stop media with parameter #bt_le_audio_source_action_param_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_MEDIA_NEXT_TRACK              (BT_LE_AUDIO_MODULE_MEDIA|0x0004U)     /**< This action sends a request to play next track with parameter #bt_le_audio_source_action_param_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_MEDIA_PREVIOUS_TRACK          (BT_LE_AUDIO_MODULE_MEDIA|0x0005U)     /**< This action sends a request to play previous track with parameter #bt_le_audio_source_action_param_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_VOLUME_UNMUTE                 (BT_LE_AUDIO_MODULE_VOLUME|0x0001U)    /**< This action sends a request to set volume unmute. */
#define BT_LE_AUDIO_SOURCE_ACTION_VOLUME_MUTE                   (BT_LE_AUDIO_MODULE_VOLUME|0x0002U)    /**< This action sends a request to set volume mute. */
#define BT_LE_AUDIO_SOURCE_ACTION_VOLUME_DOWN                   (BT_LE_AUDIO_MODULE_VOLUME|0x0003U)    /**< This action sends a request to set volume down. */
#define BT_LE_AUDIO_SOURCE_ACTION_VOLUME_UP                     (BT_LE_AUDIO_MODULE_VOLUME|0x0004U)    /**< This action sends a request to set volume up. */
#define BT_LE_AUDIO_SOURCE_ACTION_VOLUME_UNMUTE_DOWN            (BT_LE_AUDIO_MODULE_VOLUME|0x0005U)    /**< This action sends a request to set unmute volume down. */
#define BT_LE_AUDIO_SOURCE_ACTION_VOLUME_UNMUTE_UP              (BT_LE_AUDIO_MODULE_VOLUME|0x0006U)    /**< This action sends a request to set unmute volume up. */
#define BT_LE_AUDIO_SOURCE_ACTION_VOLUME_SET_ABSOLUTE_VOLUME    (BT_LE_AUDIO_MODULE_VOLUME|0x0007U)    /**< This action sends a request to set absolute volume with parameter #bt_le_audio_source_volume_set_absolute_volume_t. */
#define BT_LE_AUDIO_SOURCE_ACTION_MIC_UNMUTE                    (BT_LE_AUDIO_MODULE_MICROPHONE|0x0001U)    /**< This action sends a request to set mic unmute. */
#define BT_LE_AUDIO_SOURCE_ACTION_MIC_MUTE                      (BT_LE_AUDIO_MODULE_MICROPHONE|0x0002U)    /**< This action sends a request to set mic mute. */
typedef uint16_t bt_le_audio_source_action_t; /**< The type of the LE AUDIO source action. */
/**
 * @brief The LE AUDIO source events.
 */
#define BT_LE_AUDIO_SOURCE_EVENT_CALL_INCOMING          (BT_LE_AUDIO_MODULE_CALL|BLE_TBS_EVENT_INCOMING_CALL)     /**< This event indicates current call state is incoming with parameter #bt_le_audio_source_event_call_incoming_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_CALL_DIALING           (BT_LE_AUDIO_MODULE_CALL|BLE_TBS_EVENT_DIALING)           /**< This event indicates current call state is dialing with parameter #bt_le_audio_source_event_call_dialing_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_CALL_ALERTING          (BT_LE_AUDIO_MODULE_CALL|BLE_TBS_EVENT_ALERTING)          /**< This event indicates current call state is alerting with parameter #bt_le_audio_source_event_call_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_CALL_ACTIVE            (BT_LE_AUDIO_MODULE_CALL|BLE_TBS_EVENT_ACTIVE)            /**< This event indicates current call state is active with parameter #bt_le_audio_source_event_call_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_CALL_ENDED             (BT_LE_AUDIO_MODULE_CALL|BLE_TBS_EVENT_CALL_ENDED)        /**< This event indicates current call state is call ended with parameter #bt_le_audio_source_event_call_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_CALL_HELD              (BT_LE_AUDIO_MODULE_CALL|BLE_TBS_EVENT_CALL_HELD)         /**< This event indicates current call state is call held with parameter #bt_le_audio_source_event_call_t. */

#define BT_LE_AUDIO_SOURCE_EVENT_CALL_ACCEPT            (BT_LE_AUDIO_MODULE_CALL|BLE_TBS_EVENT_ACCEPT_CALL)       /**< This event indicates to accept call with parameter #bt_le_audio_source_event_call_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_CALL_TERMINATE         (BT_LE_AUDIO_MODULE_CALL|BLE_TBS_EVENT_TERMINATE_CALL)    /**< This event indicates to terminate call with parameter #bt_le_audio_source_event_call_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_HOLD_CALL              (BT_LE_AUDIO_MODULE_CALL|BLE_TBS_EVENT_HOLD_CALL)         /**< This event indicates to hold call with parameter #bt_le_audio_source_event_call_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_UNHOLD_CALL            (BT_LE_AUDIO_MODULE_CALL|BLE_TBS_EVENT_UNHOLD_CALL)       /**< This event indicates to unhold call with parameter #bt_le_audio_source_event_call_t. */

#define BT_LE_AUDIO_SOURCE_EVENT_MEDIA_PLAYING          (BT_LE_AUDIO_MODULE_MEDIA|BLE_MCS_EVENT_PLAYING)          /**< This event indicates the media playing event with parameter #bt_le_audio_source_event_media_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_MEDIA_PAUSED           (BT_LE_AUDIO_MODULE_MEDIA|BLE_MCS_EVENT_PAUSED)           /**< This event indicates the media receive paused event with parameter #bt_le_audio_source_event_media_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_MEDIA_PREVIOUS_TRACK   (BT_LE_AUDIO_MODULE_MEDIA|BLE_MCS_EVENT_PREVIOUS_TRACK)   /**< This event indicates the media receive previous track event with parameter #bt_le_audio_source_event_media_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_MEDIA_NEXT_TRACK       (BT_LE_AUDIO_MODULE_MEDIA|BLE_MCS_EVENT_NEXT_TRACK)       /**< This event indicates the media receive play next track event with parameter #bt_le_audio_source_event_media_t. */
#define BT_LE_AUDIO_SOURCE_EVENT_VOLUME_STATE           (BT_LE_AUDIO_MODULE_VOLUME|0x0001)                        /**< This event indicates the volume state changed with parameter #bt_le_audio_source_event_volume_t. */
typedef uint16_t bt_le_audio_source_event_t;    /**< The type of the LE AUDIO source events.*/

/**
 * @}
 */

/**
 * @defgroup Bluetooth_Leaudio_source_struct struct
 * @{
 */
/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SOURCE_EVENT_CALL_INCOMING.
 */
#define bt_le_audio_source_event_call_incoming_t    ble_tbs_event_incoming_call_t

/**
 *  @brief This structure defines the parameter data type for event #BT_LE_AUDIO_SOURCE_EVENT_CALL_DIALING.
 */
#define bt_le_audio_source_event_call_dialing_t     ble_tbs_event_dialing_t

/**
 *  @brief This structure defines the parameter data type for call events.
 */
#define bt_le_audio_source_event_call_t             ble_tbs_event_parameter_t

/**
 *  @brief This structure defines the parameter data type for media events.
 */
#define bt_le_audio_source_event_media_t            ble_mcs_event_parameter_t

/**
 *  @brief This structure defines the parameter data type for volume events.
 */
typedef struct {
    uint8_t volume;     /**< Volume.*/
    bool mute;          /**< Mute.*/
} bt_le_audio_source_event_volume_t;

/**
 *  @brief This structure defines the parameter data type for actions.
 */
typedef struct {
    uint8_t service_index;  /**< Service index.*/
} bt_le_audio_source_action_param_t;

/**
 *  @brief This structure defines the parameter data type for #BT_LE_AUDIO_SOURCE_ACTION_CALL_ACCEPT.
 */
typedef struct {
    uint8_t service_index;                  /**< Service index.*/
    bt_le_audio_call_index_t call_index;    /**< Call index.*/
} bt_le_audio_source_call_accept_t;

/**
 *  @brief This structure defines the parameter data type for #BT_LE_AUDIO_SOURCE_ACTION_CALL_ALERTING.
 */
typedef bt_le_audio_source_call_accept_t bt_le_audio_source_call_alerting_t;

/**
 *  @brief This structure defines the parameter data type for #BT_LE_AUDIO_SOURCE_ACTION_CALL_ACTIVE.
 */
typedef bt_le_audio_source_call_accept_t bt_le_audio_source_call_active_t;

/**
 *  @brief This structure defines the parameter data type for #BT_LE_AUDIO_SOURCE_ACTION_CALL_ENDED.
 */
typedef bt_le_audio_source_call_accept_t bt_le_audio_source_call_ended_t;

/**
 *  @brief This structure defines the parameter data type for #BT_LE_AUDIO_SOURCE_ACTION_CALL_HOLD.
 */
typedef bt_le_audio_source_call_accept_t bt_le_audio_source_call_hold_t;

/**
 *  @brief This structure defines the parameter data type for #BT_LE_AUDIO_SOURCE_ACTION_CALL_UNHOLD.
 */
typedef bt_le_audio_source_call_accept_t bt_le_audio_source_call_unhold_t;

/**
 *  @brief This structure defines the parameter data type for #BT_LE_AUDIO_SOURCE_ACTION_CALL_TERMINATE.
 */
typedef struct {
    uint8_t service_index;                  /**< Service index.*/
    bt_le_audio_call_index_t call_index;    /**< Call index.*/
    bt_le_audio_call_termination_reason_type_t reason;  /**< Termination_reason.*/
} bt_le_audio_source_call_terminate_t;

/**
 *  @brief This structure defines the parameter data type for #BT_LE_AUDIO_SOURCE_ACTION_VOLUME_SET_ABSOLUTE_VOLUME.
 */
typedef struct {
    uint8_t volume;     /**< Volume.*/
} bt_le_audio_source_volume_set_absolute_volume_t;

/**
 * @brief The LE AUDIO source event callback.
 * @param[in] event             is the event ID, refer to #bt_le_audio_source_event_t.
 * @param[in] msg               is the event message.
 */
typedef uint16_t (*bt_le_audio_source_cb_t)(bt_le_audio_source_event_t event, void *msg);

/**
 * @}
 */

/**
 * @brief                       This function initializes the LE AUDIO source.
 * @param[in] role              is the role of the device.
 * @param[in] callback          is the function pointer of the callback. This callback will be called when receive event.
 * @param[in] max_link          is the maximum of LE AUDIO link supported.
 * @return                      BT_STATUS_SUCCESS, the operation completed successfully.
 *                              BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t bt_le_audio_source_init(ble_tmap_role_t role, bt_le_audio_source_cb_t callback, uint8_t max_link);

/**
 * @brief                       This function triggers call, media or volume action.
 * @param[in] action            is the action to be triggered.
 * @param[in] params            is the parameter of the related action.
 * @return                      BT_STATUS_SUCCESS, the operation completed successfully.
 *                              BT_STATUS_FAIL, the operation has failed.
 *                              BT_STATUS_UNSUPPORTED, the operation is unsupported.
 */
bt_status_t bt_le_audio_source_send_action(bt_le_audio_source_action_t action, void *params);

/**
 * @brief                       This function sets state to incoming call.
 * @param[in] service_index     is the service index.
 * @param[in] uri               is the URI information of the incoming call.
 * @param[in] uri_len           is the URI information length of the incoming call.
 * @param[in] name              is the call friendly name of the incoming call.
 * @param[in] name_len          is the call friendly name length of the incoming call.
 * @return                      is the call index.
 */
bt_le_audio_call_index_t bt_le_audio_source_call_set_incoming_call(uint8_t service_index, uint8_t *uri, uint8_t uri_len, uint8_t *name, uint8_t name_len);

/**
 * @brief                       This function starts a call and informs the TBS client.
 * @param[in] service_index     is the service index.
 * @param[in] uri               is the URI information of the outgoing call.
 * @param[in] uri_len           is the URI information length of the outgoing call.
 * @return                      is the call index.
 */
bt_le_audio_call_index_t bt_le_audio_source_call_originate(uint8_t service_index, uint8_t *uri, uint8_t uri_len);


/**
 * @brief                       This function get the call state.
 * @param[in] service_index     is the service index.
 * @param[in] call_index        is the call index.
 * @return                      is the call state.
 */
bt_le_audio_call_state_t bt_le_audio_source_call_get_state(uint8_t service_index, bt_le_audio_call_index_t call_index);

/**
 * @brief                       This function gets the media state.
 * @param[in] service_index     is the service index.
 * @return                      is the media state.
 */
bt_le_audio_media_state_t bt_le_audio_source_media_get_state(uint8_t service_index);


/**
 * @}
 * @}
 * @}
 */

#endif  /* __BT_LE_AUDIO_SOURCE_H__ */

