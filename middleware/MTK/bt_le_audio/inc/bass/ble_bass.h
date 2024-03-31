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
#ifndef __BLE_BASS_H__
#define __BLE_BASS_H__

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothLeaudio LE_AUDIO
 * @{
 * @addtogroup BluetoothLeAudioBASS BASS
 * @{
 * This section introduces the BASS operation codes, request and response structures, API prototypes.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b BASS                       | Broadcast Audio Scan Service. |
 *
 * @section ble_bass_api_usage How to use this module
 *    - Sample code:
 *     @code
 *
 *     @endcode
 */

#include "bt_type.h"

#ifndef PACKED
#define PACKED  __attribute__((packed))
#endif

#define BLE_BASS_BROADCAST_CODE_SIZE               16
#define BLE_BASS_BROADCAST_ID_SIZE                 3
#define BLE_BASS_PA_INTERVAL_SIZE                  2

/**
 * @defgroup Bluetoothble_BASS_define Define
 * @{
 * This section defines the BASS opcode and the enumerated values.
 */
#define BLE_BASS_READ_BROADCAST_RECEIVE_STATE_CNF               0x01    /**< The result of reading broadcast receive state, with #ble_bass_read_broadcast_receive_state_cnf_t as the payload in the callback function. */
#define BLE_BASS_SET_BROADCAST_RECEIVE_STATE_NOTIFICATION_CNF   0x02    /**< The result of setting notification for broadcast receive state, with #ble_bass_write_cnf_ts as the payload in the callback function. */
#define BLE_BASS_REMOTE_SCAN_STOP_CNF                           0x03    /**< The result of informing the peer the scanning has stopped, with #ble_bass_write_cnf_t as the payload in the callback function. */
#define BLE_BASS_REMOTE_SCAN_START_CNF                          0x04    /**< The result of informing the peer the scanning has started, with #ble_bass_write_cnf_t as the payload in the callback function. */
#define BLE_BASS_ADD_SOURCE_CNF                                 0x05    /**< The result of adding source, with #ble_bass_write_cnf_t as the payload in the callback function. */
#define BLE_BASS_MODIFY_SOURCE_CNF                              0x06    /**< The result of modifying source, with #ble_bass_write_cnf_t as the payload in the callback function. */
#define BLE_BASS_SET_BROADCAST_CODE_CNF                         0x07    /**< The result of setting broadcast code, with #ble_bass_write_cnf_t as the payload in the callback function. */
#define BLE_BASS_REMOVE_SOURCE_CNF                              0x08    /**< The result of removing source, with #ble_bass_write_cnf_t as the payload in the callback function. */
#define BLE_BASS_BROADCAST_RECEIVE_STATE_NOTIFY                 0x09    /**< The notification when the peer's broadcast receive state has changed, with #ble_bass_broadcast_receive_state_notify_t as the payload in the callback function. */
#define BLE_BASS_DISCOVER_SERVICE_COMPLETE_NOTIFY               0x0A    /**< The notification when discover all BASS instances complete, with #ble_bass_discover_service_complete_t as the payload in the callback function. */

#define BT_BASS_BROADCAST_ADDRESS_TYPE_PUBLIC                   0x00    /**< Public address type for broadcast source. */
#define BT_BASS_BROADCAST_ADDRESS_TYPE_RANDOM                   0x01    /**< Random address type for broadcast source. */
typedef uint8_t bt_bass_address_type_t;

#define BT_BASS_PA_SYNC_DO_NOT_SYNC_TO_PA                       0x00    /**< Do not synchronize to PA. */
#define BT_BASS_PA_SYNC_SYNCHRONIZE_TO_PA_PAST_AVAILABLE        0x01    /**< Synchronize to PA, PAST is available. */
#define BT_BASS_PA_SYNC_SYNCHRONIZE_TO_PA_PAST_NOT_AVAILABLE    0x02    /**< Synchronize to PA, PAST is not available. */
typedef uint8_t bt_bass_pa_sync_t;

#define BT_BASS_PA_SYNC_STATE_NOT_SYNCHRONIZED_TO_PA            0x00    /**< Not synchronized to PA. */
#define BT_BASS_PA_SYNC_STATE_SYNCINFO_REQUEST                  0x01    /**< The peer is requesting the SyncInfo. */
#define BT_BASS_PA_SYNC_STATE_SYNCHRONIZED_TO_PA                0x02    /**< Synchronized to PA. */
#define BT_BASS_PA_SYNC_STATE_FAILED_TO_SYNCHRONIZE_TO_PA       0x03    /**< Failed to synchronize to PA. */
#define BT_BASS_PA_SYNC_STATE_NO_PAST                           0x04    /**< No P.A.S.T. */
typedef uint8_t bt_bass_pa_sync_state_t;

#define BT_BASS_BIS_SYNC_NO_PREFERENCE                          0xFFFFFFFF    /**< Sync BIS with no preference. */

#define BT_BASS_FAIL_TO_SYNC_BIS_STATE                          0xFFFFFFFF    /**< Fail to sync BIS. */

#define BT_BASS_BIG_ENCRYPTION_NOT_ENCRYPTED                    0x00    /**< BIG is not encyprted. */
#define BT_BASS_BIG_ENCRYPTION_BROADCAST_CODE_REQUIRED          0x01    /**< Broadcast code is required for BIG. */
#define BT_BASS_BIG_ENCRYPTION_DECRYPTING                       0x02    /**< BIG is being decyprted. */
#define BT_BASS_BIG_BAD_CODE                                    0x03    /**< Incorrect encryption key. */
typedef uint8_t bt_bass_big_encryption_t;
/**
 * @}
 */

/**
 * @defgroup Bluetoothble_BASS_struct Struct
 * @{
 * This section defines basic data structures for the BASS.
 */

/**
 *  @brief This structure defines the parameter data type for event #BLE_BASS_READ_BROADCAST_RECEIVE_STATE_CNF.
 */
typedef struct {
    bt_handle_t handle;                                 /**< Connection handle. */
    uint8_t source_id;                                  /**< Assigned by the server, Range: 0x00-0xFF */
    bt_bass_address_type_t source_addr_type;            /**< Source address type. */
    bt_bd_addr_t source_addr;                           /**< Source address. */
    uint8_t source_adv_sid;                             /**< Advertising SID subfield in the ADI field of the AUX_ADV_IND PDU or the LL_PERIODIC_SYNC_IND containing the SyncInfo that points to the PA transmitted by the Broadcast Source. */
    uint8_t broadcast_id[BLE_BASS_BROADCAST_ID_SIZE];   /**< Broadcast ID of the Broadcast source */
    bt_bass_pa_sync_state_t pa_sync_state;              /**< Current synchronization state of the server with respect to a PA. */
    bt_bass_big_encryption_t big_encryption;            /**< BIG encryption. */
    uint8_t *bad_code;                                  /**< If big_encryption = 0, 1, or 2: bad_code is empty (zero length). If big_encryption = 3, bad_code is incorrect broadcast code*/
    uint8_t subgroup_length;                            /**< Subgroup length. */
    uint8_t *subgroup;                                  /**< Subgroup. */
} ble_bass_read_broadcast_receive_state_cnf_t;

/**
 *  @brief This structure defines the parameter data type for the events of #BLE_BASS_REMOTE_SCAN_STOP_CNF, #BLE_BASS_REMOTE_SCAN_START_CNF,
 * #BLE_BASS_ADD_SOURCE_CNF, #BLE_BASS_MODIFY_SOURCE_CNF, #BLE_BASS_SET_BROADCAST_CODE_CNF, and #BLE_BASS_REMOVE_SOURCE_CNF.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    bt_status_t status;         /**< Event status. */
} ble_bass_write_cnf_t;

typedef ble_bass_read_broadcast_receive_state_cnf_t ble_bass_broadcast_receive_state_notify_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_BASS_DISCOVER_SERVICE_COMPLETE_NOTIFY.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    bt_status_t status;         /**< Event status. */
} ble_bass_discover_service_complete_t;

typedef struct {
    bt_bass_address_type_t source_addr_type;            /**< Source address type. */
    bt_bd_addr_t source_addr;                           /**< Source address. */
    uint8_t source_adv_sid;                             /**< Advertising SID subfield in the ADI field of the AUX_ADV_IND PDU or the LL_PERIODIC_SYNC_IND containing the SyncInfo that points to the PA transmitted by the Broadcast Source. */
    uint8_t broadcast_id[BLE_BASS_BROADCAST_ID_SIZE];   /**< Broadcast ID of the Broadcast source. */
    bt_bass_pa_sync_t pa_sync;                          /**< PA sync operation. */
    uint8_t pa_interval[BLE_BASS_PA_INTERVAL_SIZE];     /**< PA Interval */
    uint8_t subgroup_length;                            /**< Subgroup data length. */
    uint8_t *subgroup;                                  /**< Subgroup data. */
} PACKED ble_bass_add_source_param_t;

typedef struct {
    uint8_t source_id;                                   /**< Source ID */
    bt_bass_pa_sync_t pa_sync;                           /**< PA sync operation. */
    uint8_t pa_interval[BLE_BASS_PA_INTERVAL_SIZE];      /**< PA Interval */
    uint8_t subgroup_length;                             /**< Subgroup data length. */
    uint8_t *subgroup;                                   /**< Subgroup data. */
} PACKED ble_bass_modify_source_param_t;

/**
 * @brief This structure defines the event callback for BASS.
 * @param[in] event_id          is the event id.
 * @param[in] msg               is the event message.
 */
typedef void (*ble_bass_client_callback_t)(uint8_t event_id, void *msg);

/**
 * @}
 */

/**
 * @brief                       This function initializes Broadcast Audio Scan Service.
 * @param[in] callback          is the event callback for BASS.
 * @param[in] max_link_num      is the maximum number of links.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bass_client_init(ble_bass_client_callback_t callback, uint8_t max_link_num);

/**
 * @brief                       This function subscribes the notifications for broadcast receive state, the #BLE_BASS_SET_BROADCAST_RECEIVE_STATE_NOTIFICATION_CNF event is reported to
 *                              to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] enable            sets the value for the subscriptions of the notification.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bass_set_broadcast_receive_state_notification_req(bt_handle_t handle, bool enable);

/**
 * @brief                       This function reads broadcast receive state, the #BLE_BASS_READ_BROADCAST_RECEIVE_STATE_CNF event is reported
 *                              to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bass_read_broadcast_receive_state_req(bt_handle_t handle);

/**
 * @brief                       This function informs the BASS server that the client has stopped scanning on behalf of the server,
 *                              the #BLE_BASS_REMOTE_SCAN_STOP_CNF event is reported to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bass_remote_scan_stop(bt_handle_t connection_handle);

/**
 * @brief                       This function informs the BASS server that the client has started scanning on behalf of the server,
 *                              the #BLE_BASS_REMOTE_SCAN_START_CNFBLE_BASS_REMOTE_SCAN_START_CNF event is reported to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bass_remote_scan_start(bt_handle_t connection_handle);

/**
 * @brief                       This function adds a scanned audio source to the server,
 *                              the #BLE_BASS_ADD_SOURCE_CNF event is reported to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] param             is the information of the source to synchronize to.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bass_add_source(bt_handle_t connection_handle, const ble_bass_add_source_param_t *param);

/**
 * @brief                       This function modifies the information of the source,
 *                              the #BLE_BASS_MODIFY_SOURCE_CNF event is reported to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] param             is the information of the source which has been added to the server.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bass_modify_source(bt_handle_t connection_handle, const ble_bass_modify_source_param_t *param);

/**
 * @brief                       This function sets the broadcast code that is used in the BIS stream,
 *                              the #BLE_BASS_SET_BROADCAST_CODE_CNF event is reported to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] source_id         is the source id of the BIS stream.
 * @param[in] param             is the information of the source to synchronize to.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bass_set_broadcast_code(bt_handle_t connection_handle, uint8_t source_id, const uint8_t *param);
/**
 * @brief                       This function requests the server to remove the audio source,
 *                              the #BLE_BASS_REMOVE_SOURCE_CNF event is reported to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] source_id         is the source id of the BIS stream.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bass_remove_source(bt_handle_t connection_handle, uint8_t source_id);

/**
 * @}
 * @}
 * @}
 */

#endif  /* __BLE_BASS_H__ */
