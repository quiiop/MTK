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

#ifndef __BT_MESH_MODEL_TIME_SERVER_H__
#define __BT_MESH_MODEL_TIME_SERVER_H__

/**
 *   @addtogroup BluetoothMesh Mesh
 *   @{
 *   @addtogroup BluetoothMeshSigModel Sig_Model
 *   @{
 *   @addtogroup BluetoothMeshTimeModel Time_Model
 *   @{

 *      bt_mesh_model_time_server.h defines the SIG Mesh Time Server Model APIs.

* - Mesh defines times based on International Atomic Time (TAI). The base representation of times is the number of seconds after 00:00:00 TAI on 2000-01-01 (that is, 1999-12-31T23:59:28 UTC).
* -
* - To create time setup server in your device, call the API #bt_mesh_model_add_time_setup_server() declared in bt_mesh_model_time_server.h.
* - The Time state represents the present TAI time, the current TAI-UTC Delta and local time zone offset, and the next change to each of the latter (e.g., because of a switch from winter to summer time or an announced leap second).
* - It is possible if a device use UTC as time reference. The device can convert to TAI time from UTC time.
* - SIG MESH Model specification provides an algorithm for converting TAI to UTC. Please refer to the spec section 5.1.1 if needed.
* - Please refer to sample code below for a sample stucture:
* -
* -
* - Sample code:
*      @code

   typedef struct {
       uint8_t tai_seconds[5];
       uint8_t subsecond;
       uint8_t uncertainty;
       uint16_t time_authority : 1;
       uint16_t tai_utc_delta_current : 15;
       uint16_t tai_utc_delta_new;
       uint8_t time_zone_offset_current;
       uint8_t time_zone_offset_new;
       uint8_t tai_of_zone_change[5];
       uint8_t tai_of_delta_change[5];
       uint8_t time_role;
   } time_server_model_t;

   static void _light_server_msg_handler(uint16_t model_handle, const bt_mesh_access_message_rx_t* msg, const void* arg)
   {
       switch(msg->opcode.opcode)
       {
           ...
           case BT_MESH_MODEL_TIME_GET:
           {
               if(TAI seconds == 0)
               {
                   //omit the rest of fields
                   //tx payload contains only TAI seconds
               }

               //reply with:
               bt_mesh_model_time_status(model_handle, reply, length, msg);
               break;
           }
           case BT_MESH_MODEL_TIME_ZONE_GET:
           {
               //reply with:
               bt_mesh_model_time_zone_status(model_handle, gTime_server->time_zone_offset_current,
                gTime_server->time_zone_offset_new, gTime_server->tai_of_zone_change, msg);
               break;
           }
           case BT_MESH_MODEL_TIME_ROLE_GET:
           {
               //reply with:
               bt_mesh_model_time_role_status(model_handle, gTime_server->time_role, msg);

               break;
           }
           case BT_MESH_MODEL_TAI_UTC_DELTA_GET:
           {
               //reply with:
               bt_mesh_model_tai_utc_delta_status(model_handle, gTime_server->tai_utc_delta_current,
                   gTime_server->tai_utc_delta_new, gTime_server->tai_of_delta_change, msg);
               break;
           }
       }
   }

   static void mesh_create_device(void)
   {
       gTime_server = malloc(sizeof(time_server_model_t));
       memset(gTime_server, 0, sizeof(time_server_model_t));

       bt_mesh_model_add_time_setup_server(&model_handle, element_index, _light_server_msg_handler, NULL);
   }

*      @endcode
*

*/


#include "bt_mesh_access.h"

/*!
    @brief Add a time server model.
    @param[out] model_handle is the handle of this added model.
    @param[in] element_index is the index of element that this model to be added in.
    @param[in] callback is the message handler for time server model.
    @param[in] publish_timeout_cb is the periodic publishing timeout callback.
    @return
    @c true means adding time server model successfully. \n
    @c false means adding time server model failed.
*/
bool bt_mesh_model_add_time_server(
    uint16_t *model_handle, uint16_t element_index,
    bt_mesh_access_msg_handler callback, bt_mesh_access_publish_timeout_cb_t publish_timeout_cb);

/*!
    @brief Add a time setup server model.
    @param[out] model_handle is the handle of this added model.
    @param[in] element_index is the index of element that this model to be added in.
    @param[in] callback is the message handler for time setup server model.
    @param[in] publish_timeout_cb is the periodic publishing timeout callback.
    @return
    @c true means adding time setup server model successfully. \n
    @c false means adding time setup server model failed.
*/
bool bt_mesh_model_add_time_setup_server(
    uint16_t *model_handle, uint16_t element_index,
    bt_mesh_access_msg_handler callback, bt_mesh_access_publish_timeout_cb_t publish_timeout_cb);

/*!
    @brief Sends a time status message.
    @param[in] model_handle is the model handle which the message belongs to.
    @param[in] buffer is the message payload to be sent.
    @param[in] buffer_length is the message payload length.
    @param[in] msg is the received message which this API replies to.
    @return
    #BT_MESH_SUCCESS, requesting message is performed successfully. \n
    #BT_MESH_ERROR_INVALID_ADDR, cannot find corresponding address. \n
    #BT_MESH_ERROR_OOM, not enough memory for sending reply
*/
bt_mesh_status_t bt_mesh_model_time_status(uint16_t model_handle,
        uint8_t *buffer, uint8_t buffer_length, const bt_mesh_access_message_rx_t *msg);

/*!
    @brief Sends a time zone status message.
    @param[in] model_handle is the model handle which the message belongs to.
    @param[in] time_zone_offset_current is the current time zone offset.
    @param[in] time_zone_offset_new is the new time zone offset.
    @param[in] tai_of_zone_change is the message payload length.
    @param[in] msg is the received message which this API replies to.
    @return
    #BT_MESH_SUCCESS, requesting message is performed successfully. \n
    #BT_MESH_ERROR_INVALID_ADDR, cannot find corresponding address. \n
    #BT_MESH_ERROR_OOM, not enough memory for sending reply
*/
bt_mesh_status_t bt_mesh_model_time_zone_status(uint16_t model_handle,
        uint8_t time_zone_offset_current, uint8_t time_zone_offset_new,
        uint8_t *tai_of_zone_change, const bt_mesh_access_message_rx_t *msg);

/*!
    @brief Sends a TAI-UTC delta status message.
    @param[in] model_handle is the model handle which the message belongs to.
    @param[in] tai_utc_delta_current is the current TAI-UTC delta value.
    @param[in] tai_utc_delta_new is the new TAI-UTC delta value.
    @param[in] tai_of_zone_change is the TAI second of zone change.
    @param[in] msg is the received message which this API replies to.
    @return
    #BT_MESH_SUCCESS, requesting message is performed successfully. \n
    #BT_MESH_ERROR_INVALID_ADDR, cannot find corresponding address. \n
    #BT_MESH_ERROR_OOM, not enough memory for sending reply
*/
bt_mesh_status_t bt_mesh_model_tai_utc_delta_status(uint16_t model_handle,
        uint16_t tai_utc_delta_current, uint16_t tai_utc_delta_new, uint8_t *tai_of_zone_change,
        const bt_mesh_access_message_rx_t *msg);

/*!
    @brief Sends a time role status message.
    @param[in] model_handle is the model handle which the message belongs to.
    @param[in] time_role is the time role of time server.
    @param[in] msg is the received message which this API replies to.
    @return
    #BT_MESH_SUCCESS, requesting message is performed successfully. \n
    #BT_MESH_ERROR_INVALID_ADDR, cannot find corresponding address. \n
    #BT_MESH_ERROR_OOM, not enough memory for sending reply
*/
bt_mesh_status_t bt_mesh_model_time_role_status(uint16_t model_handle, uint8_t time_role,
        const bt_mesh_access_message_rx_t *msg);


/*!
@}
@}
@}
*/

#endif // __BT_MESH_MODEL_TIME_SERVER_H__


