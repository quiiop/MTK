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

#ifndef __BT_MESH_MODEL_SCHEDULER_CLIENT_H__
#define __BT_MESH_MODEL_SCHEDULER_CLIENT_H__

/**
 *   @addtogroup BluetoothMesh Mesh
 *   @{
 *   @addtogroup BluetoothMeshSigModel Sig_Model
 *   @{
 *   @addtogroup BluetoothMeshSchedulerModel Scheduler_Model
 *   @{
 *      bt_mesh_model_scheduler_client.h defines the SIG Mesh Scheduler Client Model APIs.
*
* @section bt_mesh_scheduler_api_usage How to add and use a scheduler client model.
* - Sample code:
*      @code
   static void mesh_create_device(void)
   {
       bt_mesh_model_add_scheduler_client(&model_handle, element_index, _light_server_msg_handler, NULL);
   }

*      @endcode
*

*/

#include "bt_mesh_access.h"

/**
 * @defgroup Bluetooth_mesh_scheduler_model_enum Enum
 * @{
 */

/*!
     @brief Scheduler client event id
     @{
 */
typedef enum {
    BT_MESH_SCHEDULER_CLIENT_EVT_SCHEDULER_STATUS,                    /**< Event for scheduler status. */
    BT_MESH_SCHEDULER_CLIENT_EVT_ACTION_STATUS,           /**< Event for scheduler register status. */
} bt_mesh_scheduler_client_event_id_t;
/*!  @} */
/**
 * @}
 */

/**
 * @defgroup Bluetooth_mesh_scheduler_model Struct
 * @{
*/

/*!
     @brief Message parameter of scheduler status
 */
typedef struct {
    uint16_t schedules;
} __attribute__((packed)) bt_mesh_scheduler_client_evt_scheduler_status_t;

/*!
     @brief Message parameter of scheduler action status
 */
typedef struct {
    uint64_t index : 4;
    uint64_t year : 7;
    uint64_t month : 12;
    uint64_t day : 5;
    uint64_t hour : 5;
    uint64_t minute : 6;
    uint64_t second : 6;
    uint64_t day_of_week : 7;
    uint64_t action : 4;
    uint64_t transition_time : 8;
    uint16_t scene_number;
} __attribute__((packed)) bt_mesh_scheduler_client_evt_action_status_t;

/*!
     @brief Scheduler client event structure.
 */
typedef struct {
    bt_mesh_scheduler_client_event_id_t evt_id;   /**<  Scheduler client event ID. */
    union {
        bt_mesh_scheduler_client_evt_scheduler_status_t       scheduler_status;    /**<  Scheduler status */
        bt_mesh_scheduler_client_evt_action_status_t          action_status;    /**<  Scheduler Zone status */
    } data; /**<  Union for data. */
} bt_mesh_scheduler_client_evt_t;
/**
 * @}
 */

/**
 * @defgroup Bluetooth_mesh_scheduler_model_typedef Typedef
 * @{
*/

/** @brief  This defines the scheduler client event handler prototype.
 *  @param[in] model_handle is the model handle which wants to handle these messages.
 *  @param[in] msg is the original received message.
 *  @param[in] event is the parsed scheduler client event.
 *  @return NONE
 * #bt_mesh_model_add_scheduler_client,\n
 */
typedef void (*bt_mesh_scheduler_client_evt_handler)(uint16_t model_handle, const bt_mesh_access_message_rx_t *msg, const bt_mesh_scheduler_client_evt_t *event);
/**
 * @}
 */

/*!
    @brief Add a scheduler client model.
    @param[out] model_handle is the handle of this added model.
    @param[in] element_index is the index of element that this model to be added in.
    @param[in] callback is the message handler for scheduler server model.
    @param[in] publish_schedulerout_cb is the periodic publishing schedulerout callback.
    @return
    @c true means adding scheduler client model successfully. \n
    @c false means adding scheduler client model failed.
*/
bool bt_mesh_model_add_scheduler_client(
    uint16_t *model_handle, uint16_t element_index,
    bt_mesh_scheduler_client_evt_handler callback, bt_mesh_access_publish_timeout_cb_t publish_timeout_cb);

/*!
 * @brief Sends a request to store scheduler.
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
bt_mesh_status_t bt_mesh_model_scheduler_client_get(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta);

bt_mesh_status_t bt_mesh_model_scheduler_client_get_action(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint8_t index);

bt_mesh_status_t bt_mesh_model_scheduler_client_set_action(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta,
    bt_mesh_scheduler_client_evt_action_status_t *param, bool reliable);
/*!
@}
@}
@}
*/

#endif // __BT_MESH_MODEL_SCHEDULER_CLIENT_H__


