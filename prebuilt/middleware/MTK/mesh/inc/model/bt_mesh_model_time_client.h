/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#ifndef __BT_MESH_MODEL_TIME_CLIENT_H__
#define __BT_MESH_MODEL_TIME_CLIENT_H__

/**
 *   @addtogroup BluetoothMesh Mesh
 *   @{
 *   @addtogroup BluetoothMeshSigModel Sig_Model
 *   @{
 *   @addtogroup BluetoothMeshTimeModel Time_Model
 *   @{
 *      bt_mesh_model_time_client.h defines the SIG Mesh Time Client Model APIs.
*
* @section bt_mesh_time_api_usage How to add and use a time client model.
* - Sample code:
*      @code
   static void mesh_create_device(void)
   {
       bt_mesh_model_add_time_client(&model_handle, element_index, _light_server_msg_handler, NULL);
   }

*      @endcode
*

*/

#include "bt_mesh_access.h"

/**
 * @defgroup Bluetooth_mesh_time_model_enum Enum
 * @{
 */

/*!
     @brief Time client event id
     @{
 */
typedef enum {
    BT_MESH_TIME_CLIENT_EVT_TIME_STATUS,                    /**< Event for time status. */
    BT_MESH_TIME_CLIENT_EVT_TIMEZONE_STATUS,           /**< Event for time register status. */
    BT_MESH_TIME_CLIENT_EVT_TAI_UTC_DELTA_STATUS,           /**< Event for time register status. */
    BT_MESH_TIME_CLIENT_EVT_TIMEROLE_STATUS,           /**< Event for time register status. */
} bt_mesh_time_client_event_id_t;
/*!  @} */
/**
 * @}
 */

/**
 * @defgroup Bluetooth_mesh_time_model Struct
 * @{
*/

/*!
     @brief Message parameter of time zone set
 */
typedef struct {
    uint8_t time_zone_offset_new;
    uint64_t tai_of_zone_change;
} __attribute__((packed)) bt_mesh_time_client_timezone_set_t;

/*!
     @brief Message parameter of TAI-UTC delta status
 */
typedef struct {
    uint64_t tai_utc_delta_new : 15;
    uint64_t padding : 1;
    uint64_t tai_of_delta_change : 40;
    uint64_t rfu : 8;
} __attribute__((packed)) bt_mesh_time_client_evt_taiutc_delta_set_t;


/*!
     @brief Message parameter of time status
 */
typedef struct {
    uint64_t tai_seconds : 40;   /**< Indicate the the status code for the last operation.. */
    uint64_t rfu : 8;
    uint64_t subsecond : 8;   /**< Indicate the Time Number of a current time. */
    uint64_t uncertainty : 8;   /**< Indicate the Time Number of a target time. (Optional) */
    uint16_t time_authority : 1; /**< Indicate the remaining time of transition. */
    uint16_t tai_utc_delta : 15;
    uint8_t time_zone_offset;
} __attribute__((packed)) bt_mesh_time_client_evt_time_status_t;

/*!
     @brief Message parameter of time zone status
 */
typedef struct {
    uint64_t offset_current : 8;   /**< Indicate the the current TAI time in seconds. */
    uint64_t offset_new : 8;   /**< Indicate the The sub-second time in units of 1/256th second. */
    uint64_t tai_of_zone_change: 40;    /**< The estimated uncertainty in 10-millisecond steps */
    uint64_t rfu : 8;
} __attribute__((packed)) bt_mesh_time_client_evt_timezone_status_t;

/*!
     @brief Message parameter of TAI-UTC delta status
 */
typedef struct {
    uint16_t tai_utc_delta_current : 15;   /**< Indicate the the status code for the last operation.. */
    uint16_t padding_1 : 1;   /**< Indicate the Time Number of a current time. */
    uint64_t tai_utc_delta_new : 15;   /**< Indicate the Time Number of a target time. (Optional) */
    uint64_t padding_2 : 1; /**< Indicate the remaining time of transition. */
    uint64_t tai_of_delta_change : 40;
    uint64_t rfu : 8;
} __attribute__((packed)) bt_mesh_time_client_evt_taiutc_delta_status_t;

/*!
     @brief Message parameter of time role status
 */
typedef struct {
    uint8_t time_role;   /**< Indicate the the status code for the last operation.. */
} __attribute__((packed)) bt_mesh_time_client_evt_timerole_status_t;

/*!
     @brief Time client event structure.
 */
typedef struct {
    bt_mesh_time_client_event_id_t evt_id;   /**<  Time client event ID. */
    union {
        bt_mesh_time_client_evt_time_status_t              time_status;    /**<  Time status */
        bt_mesh_time_client_evt_timezone_status_t          timezone_status;    /**<  Time Zone status */
        bt_mesh_time_client_evt_taiutc_delta_status_t      taiutc_status;    /**<  TAI-UTC delta status */
        bt_mesh_time_client_evt_timerole_status_t          timerole_status;    /**<  Time Role status */
    } data; /**<  Union for data. */
} bt_mesh_time_client_evt_t;
/**
 * @}
 */

/**
 * @defgroup Bluetooth_mesh_time_model_typedef Typedef
 * @{
*/

/** @brief  This defines the time client event handler prototype.
 *  @param[in] model_handle is the model handle which wants to handle these messages.
 *  @param[in] msg is the original received message.
 *  @param[in] event is the parsed time client event.
 *  @return NONE
 * #bt_mesh_model_add_time_client,\n
 */
typedef void (*bt_mesh_time_client_evt_handler)(uint16_t model_handle, const bt_mesh_access_message_rx_t *msg, const bt_mesh_time_client_evt_t *event);
/**
 * @}
 */

/*!
    @brief Add a time client model.
    @param[out] model_handle is the handle of this added model.
    @param[in] element_index is the index of element that this model to be added in.
    @param[in] callback is the message handler for time server model.
    @param[in] publish_timeout_cb is the periodic publishing timeout callback.
    @return
    @c true means adding time client model successfully. \n
    @c false means adding time client model failed.
*/
bool bt_mesh_model_add_time_client(
    uint16_t *model_handle, uint16_t element_index,
    bt_mesh_time_client_evt_handler callback, bt_mesh_access_publish_timeout_cb_t publish_timeout_cb);

/*!
 * @brief Sends a request to store time.
 * @param[in] model_handle is the model handle which the message belongs to.
 * @param[in] tx_meta is the metadata of the message.
 * @return
 * #BT_MESH_SUCCESS, packets successfully sent. \n
 * #BT_MESH_ERROR_OOM, not enough memory for sending the packet. \n
 * #BT_MESH_ERROR_NULL, tx_params or data is NULL. \n
 * #BT_MESH_ERROR_INVALID_ADDR, invalid source or destination address. \n
 * #BT_MESH_ERROR_INVALID_TTL, invalid TTL value. \n
 * #BT_MESH_ERROR_INVALID_KEY, invalid key index or no bound network key. \n
*/
bt_mesh_status_t bt_mesh_model_time_client_get_time(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta);

bt_mesh_status_t bt_mesh_model_time_client_set_time(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta,
    bt_mesh_time_client_evt_time_status_t *param);

bt_mesh_status_t bt_mesh_model_time_client_get_time_zone(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta);

bt_mesh_status_t bt_mesh_model_time_client_set_time_zone(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta,
    bt_mesh_time_client_timezone_set_t *param);


bt_mesh_status_t bt_mesh_model_time_client_get_taiutc_delta(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta);

bt_mesh_status_t bt_mesh_model_time_client_set_taiutc_delta(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta,
    bt_mesh_time_client_evt_taiutc_delta_set_t *patam);

bt_mesh_status_t bt_mesh_model_time_client_get_time_role(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta);

bt_mesh_status_t bt_mesh_model_time_client_set_time_role(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta,
    bt_mesh_time_client_evt_timerole_status_t *param);

/*!
@}
@}
@}
*/

#endif // __BT_MESH_MODEL_TIME_CLIENT_H__


