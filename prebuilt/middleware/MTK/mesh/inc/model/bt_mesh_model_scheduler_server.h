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

#ifndef __BT_MESH_MODEL_SCHEDULER_SERVER_H__
#define __BT_MESH_MODEL_SCHEDULER_SERVER_H__
/**
 *   @addtogroup BluetoothMesh Mesh
 *   @{
 *   @addtogroup BluetoothMeshSigModel Sig_Model
 *   @{
 *   @addtogroup BluetoothMeshSchedulerModel Scheduler_Model
 *   @{
 *      bt_mesh_model_scheduler_server.h defines the SIG Mesh Scheduler Server Model APIs.

* - Scheduler server is regarded as combined usage of both time server and scene server.
* - To create scheduler setup server in your device, call the API #bt_mesh_model_add_scheduler_setup_server() declared in bt_mesh_model_scheduler_server.h.
* - Scheduler provides a means of autonomous change of states of a device based on a register of defined time points with associated state-changing actions.
* - For example, a lamp may automatically turn off every day at 2AM, or a coffee machine may make coffee at 6:30AM.
* - The scheduler is based on a register that is capable of storing up to sixteen scheduled entries, each containing a starting point in local time, that may include values that represent multiple values, and an associated action to perform.
* - The index for the Scheduler register entry ranges from 0x00 to 0x0F. Values from 0x10 to 0xFF are prohibited.

* - Please refer to sample code below for a sample stucture of scheduler server model:
* -
* -
* - Sample code:
*      @code

   typedef struct {
       uint16_t schedules;
       scheduler_entry_t schedule_register[SCHEDULER_REGISTER_ENTRY_COUNT];
   } scheduler_server_model_t;

   static void _scheduler_server_msg_handler(uint16_t model_handle, const bt_mesh_access_message_rx_t* msg, const void* arg)
   {
       {
           switch(msg->opcode.opcode)
           ...
           case BT_MESH_MODEL_SCHEDULER_ACTION_GET:
           {
               //reply with:
               bt_mesh_model_scheduler_action_status(model_handle, reply, msg);
               break;
           }
           case BT_MESH_MODEL_SCHEDULER_GET:
           {
               //reply with:
               bt_mesh_model_scheduler_status(model_handle, gScheduler_Server->schedules, msg);
               break;
           }
       }
   }

   static void mesh_create_device(void)
   {
       gScheduler_Server = malloc(sizeof(scheduler_server_model_t));
       memset(gScheduler_Server, 0, sizeof(scheduler_server_model_t));

       bt_mesh_model_add_scheduler_setup_server(&model_handle, element_index, _scheduler_server_msg_handler, NULL);
   }

*      @endcode
*

*/

#include "bt_mesh_access.h"

/*!
    @brief Add a scheduler server model.
    @param[out] model_handle is the handle of this added model.
    @param[in] element_index is the index of element that this model to be added in.
    @param[in] callback is the message handler for scheduler server model.
    @param[in] publish_timeout_cb is the periodic publishing timeout callback.
    @return
    @c true means adding scheduler server model successfully. \n
    @c false means adding scheduler server model failed.
*/
bool bt_mesh_model_add_scheduler_server(
    uint16_t *model_handle, uint16_t element_index,
    bt_mesh_access_msg_handler callback, bt_mesh_access_publish_timeout_cb_t publish_timeout_cb);

/*!
    @brief Add a scheduler setup server model.
    @param[out] model_handle is the handle of this added model.
    @param[in] element_index is the index of element that this model to be added in.
    @param[in] callback is the message handler for scheduler setup server model.
    @param[in] publish_timeout_cb is the periodic publishing timeout callback.
    @return
    @c true means adding scheduler setup server model successfully. \n
    @c false means adding scheduler setup server model failed.
*/
bool bt_mesh_model_add_scheduler_setup_server(
    uint16_t *model_handle, uint16_t element_index,
    bt_mesh_access_msg_handler callback, bt_mesh_access_publish_timeout_cb_t publish_timeout_cb);

/*!
    @brief Sends a scheduler status message.
    @param[in] model_handle is the model handle which the message belongs to.
    @param[in] schedules is defined actions in the schedule register.
    @param[in] msg is the received message which this API replies to.
    @return
    #BT_MESH_SUCCESS, requesting message is performed successfully. \n
    #BT_MESH_ERROR_INVALID_ADDR, cannot find corresponding address. \n
    #BT_MESH_ERROR_OOM, not enough memory for sending reply
*/
bt_mesh_status_t bt_mesh_model_scheduler_status(uint16_t model_handle,
        uint16_t schedules, const bt_mesh_access_message_rx_t *msg);

/*!
    @brief Sends a scheduler action status message.
    @param[in] model_handle is the model handle which the message belongs to.
    @param[in] buffer is the message payload to be sent.
    @param[in] msg is the received message which this API replies to.
    @return
    #BT_MESH_SUCCESS, requesting message is performed successfully. \n
    #BT_MESH_ERROR_INVALID_ADDR, cannot find corresponding address. \n
    #BT_MESH_ERROR_OOM, not enough memory for sending reply
*/
bt_mesh_status_t bt_mesh_model_scheduler_action_status(uint16_t model_handle,
        uint8_t *buffer, const bt_mesh_access_message_rx_t *msg);

/*!
@}
@}
@}
*/

#endif // __BT_MESH_MODEL_SCHEDULER_SERVER_H__


