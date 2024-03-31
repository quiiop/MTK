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
#ifndef __BLE_CSIP_H__
#define __BLE_CSIP_H__

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothLeaudio LE_AUDIO
 * @{
 * @addtogroup BluetoothLeAudioCSIP CSIP
 * @{
 * This section introduces the CSIP operation codes, request and response structures, API prototypes.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b CSIP                       | Coordinated Set Identification Profile. |
 * |\b CSIS                       | Coordinated Set Identification Service. |
 * |\b SIRK                       | Set Identity Resolving Key. |
 * |\b PSRI                       | Private Set Random Identifier. |
 *
 * @section ble_csip_api_usage How to use this module
 *   - Start and stop the discovery procedure with #ble_csip_discover_coordinated_set().
 *   - The found coordinated set device will be received by event #BLE_CSIP_DISCOVER_COORDINATED_SET_CNF.
 *    - Sample code:
 *     @code
 *           bt_status_t app_le_audio_discover_coordinated_set(void)
 *           {
 *               bt_status_t status;
 *               if (BT_STATUS_SUCCESS != (status = ble_csip_discover_coordinated_set(true))) {
 *                   printf("[APP_SOURCE][CSIP] discover CS failed %x", status);
 *               }
 *
 *               return status;
 *           }
 *
 *           bt_status_t app_le_audio_stop_discovering_coordinated_set(void)
 *           {
 *               return ble_csip_discover_coordinated_set(false);
 *           }
 *
 *           static void app_le_audio_csip_callback(ble_csip_event_t event, void *msg)
 *           {
 *               switch (event) {
 *                   case BLE_CSIP_DISCOVER_COORDINATED_SET_CNF: {
 *                       ble_csip_discover_coordinated_set_cnf_t *cnf = (ble_csip_discover_coordinated_set_cnf_t *)msg;
 *
 *                       printf("[APP] BLE_CSIP_DISCOVER_COORDINATED_SET_CNF [%d] %02x:%02x:%02x:%02x:%02x:%02x", cnf->address.type
 *                           , cnf->address.addr[0], cnf->address.addr[1], cnf->address.addr[2], cnf->address.addr[3], cnf->address.addr[4], cnf->address.addr[5]);
 *                       break;
 *                   }
 *               }
 *           }
 *
 *           void app_le_audio_source_init(void)
 *           {
 *               ble_csip_init(app_le_audio_csip_callback, MAX_LINK_NUM); // initialize CSIP module
 *
 *               app_le_audio_discover_coordinated_set(); // start discovering coordinated set
 *
 *               app_le_audio_stop_discovering_coordinated_set(); // stop discovering coordinated set
 *           }
 *
 *     @endcode
 */

#include "bt_type.h"

/**
 * @defgroup Bluetoothble_CSIP_define Define
 * @{
 * This section defines the CSIP opcode and error codes.
 */

#define BLE_CSIP_DISCOVER_COORDINATED_SET_CNF                0x01    /**< The result of discovery coordinated set, with #ble_csip_discover_coordinated_set_cnf_t as the payload in the callback function. */
#define BLE_CSIP_READ_COORDINATED_SET_SIZE_CNF               0x02    /**< The result of read coordinated set size, with #ble_csip_read_set_size_cnf_t as the payload in the callback function. */
#define BLE_CSIP_READ_SET_MEMBER_LOCK_CNF                    0x03    /**< The result of read set member lock, with #ble_csip_read_set_lock_cnf_t as the payload in the callback function. */
#define BLE_CSIP_READ_SIRK_CNF                               0x04    /**< The result of read SIRK, with #ble_csip_read_sirk_cnf_t as the payload in the callback function. */
#define BLE_CSIP_WRITE_SET_MEMBER_LOCK_CNF                   0x05    /**< The result of write set member lock, withiout payload in the callback function. */
#define BLE_CSIP_NOTIFY_SET_MEMBER_LOCK_NOTIFY               0x06    /**< The result of notify set member lock, with #ble_csip_notify_set_lock_ind_t as the payload in the callback function. */
#define BLE_CSIP_READ_SET_MEMBER_RANK_CNF                    0x07    /**< The result of read set member rank, with #ble_csip_read_rank_cnf_t as the payload in the callback function. */
#define BLE_CSIP_DISCOVER_SERVICE_COMPLETE_NOTIFY            0x08    /**< The notification when discover all RAAS instances complete, with #ble_csip_discover_service_complete_t as the payload in the callback function. */
#define BLE_CSIP_SET_SET_MEMBER_LOCK_NOTIFICATION_CNF        0x09    /**< The result of set set member lock notification cccd, without payload in the callback function. */
typedef uint8_t ble_csip_event_t;                                    /**< The type of CSIP events.*/

/**
 * @}
 */

/**
 * @defgroup Bluetoothble_CSIP_struct Struct
 * @{
 * This section defines basic data structures for the CSIP.
 */

/**
 *  @brief This structure defines the parameter data type for event #BLE_CSIP_READ_SIRK_CNF.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    bt_key_t   sirk;            /**< The SIRK key value. */
} ble_csip_read_sirk_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CSIP_DISCOVER_COORDINATED_SET_CNF.
 */
typedef struct {
    bt_handle_t handle;        /**< Connection handle. */
    bt_addr_t   address;       /**< The found coordinated set address. */
} ble_csip_discover_coordinated_set_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CSIP_READ_COORDINATED_SET_SIZE_CNF.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    uint8_t   size;             /**< The number of devices comprising the Coordinated Set. */
} ble_csip_read_set_size_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CSIP_READ_SET_MEMBER_LOCK_CNF.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    uint8_t   lock;             /**< The lock status.*/
} ble_csip_read_set_lock_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CSIP_READ_SET_MEMBER_RANK_CNF.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    uint8_t   rank;             /**< A numeric value that shall be unique within a Coordinated Set and allows a client to perform procedures with servers that are part of a Coordinated Set in a defined order. */
} ble_csip_read_rank_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CSIP_NOTIFY_SET_MEMBER_LOCK_NOTIFY.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    uint8_t   lock;             /**< The lock status. */
} ble_csip_notify_set_lock_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CSIP_DISCOVER_SERVICE_COMPLETE_NOTIFY.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle. */
    bt_status_t status;         /**< Event status. */
} ble_csip_discover_service_complete_t;

/**
 * @brief This structure defines the event callback for CSIP.
 * @param[in] event             is the event id.
 * @param[in] msg               is the event message.
 */
typedef void (*ble_csip_callback_t)(ble_csip_event_t event, void *msg);

/**
 * @}
 */

/**
 * @brief                       This function initializes Coordinated Set Identification Service.
 * @param[in] callback          is the event callback for CSIP.
 * @param[in] max_link_num      is the maximum number of links.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csip_init(ble_csip_callback_t callback, uint8_t max_link_num);

/**
 * @brief                       This function writes set member lock, the #BLE_CSIP_WRITE_SET_MEMBER_LOCK_CNF event is reported to
 *                              to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csip_write_set_member_lock_req(bt_handle_t handle, uint8_t lock);

/**
 * @brief                       This function reads SIRK, the #BLE_CSIP_READ_SIRK_CNF event is reported to
 *                              to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csip_read_sirk_req(bt_handle_t handle);


/**
 * @brief                       This function reads coordinated set size, the #BLE_CSIP_READ_COORDINATED_SET_SIZE_CNF event is reported to
 *                              to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csip_read_coordinated_set_size_req(bt_handle_t handle);

/**
 * @brief                       This function reads set member lock, the #BLE_CSIP_READ_SET_MEMBER_LOCK_CNF event is reported to
 *                              to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csip_read_set_member_lock_req(bt_handle_t handle);

/**
 * @brief                       This function reads set member rank, the #BLE_CSIP_READ_SET_MEMBER_RANK_CNF event is reported to
 *                              to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csip_read_set_member_rank_req(bt_handle_t handle);

/**
 * @brief                       This function verifies the RSI is in the same coordinated set.
 * @param[in] psri              is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csip_verify_rsi(uint8_t *rsi);

/**
 * @brief                       This function starts the discovery procedure for coordinated set.
 * @param[in] enable            is the flag to enable or disable discovery procedure.
 * @param[in] sirk              is the value of SIRK.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csip_discover_coordinated_set_by_sirk(bool enable, bt_key_t sirk);

/**
 * @}
 * @}
 * @}
 */

#endif  /* __BLE_CSIP_H__ */
