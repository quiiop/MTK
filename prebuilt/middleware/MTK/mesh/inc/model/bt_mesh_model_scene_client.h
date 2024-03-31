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

#ifndef __BT_MESH_MODEL_SCENE_CLIENT_H__
#define __BT_MESH_MODEL_SCENE_CLIENT_H__

/**
 *   @addtogroup BluetoothMesh Mesh
 *   @{
 *   @addtogroup BluetoothMeshSigModel Sig_Model
 *   @{
 *   @addtogroup BluetoothMeshSceneModel Scene_Model
 *   @{
 *      bt_mesh_model_scene_client.h defines the SIG Mesh Scene Client Model APIs.
*
* @section bt_mesh_scene_api_usage How to add and use a scene client model.
* - Please refer to sample code for a simple scene client.
* - Sample code:
*      @code
    static void _scene_client_msg_handler(uint16_t model_handle,
        const bt_mesh_access_message_rx_t *msg, const bt_mesh_scene_client_evt_t *event)
    {
        SWITCH_DEBUG_EVT_SIG(msg->opcode.opcode);

        switch(event->evt_id)
        {
            case BT_MESH_SCENE_CLIENT_EVT_STATUS: {
                const bt_mesh_scene_client_evt_status_t *evt = &event->data.scene_status;
                printf(CUF "StatusCode   : %02x\n", 40, evt->status_code);
                printf(CUF "CurrentScene : %04x\n", 40, evt->current_scene);
                if (msg->length > 3) {
                    printf(CUF "TargetScene  : %04x\n", 40, evt->target_scene);
                    printf(CUF "RemainingTime: %02x\n", 40, evt->remaining_time);
                }
                break;
            }
            case BT_MESH_SCENE_CLIENT_EVT_REGISTER_STATUS: {
                const bt_mesh_scene_client_evt_register_status_t *evt = &event->data.register_status;
                printf(CUF "StatusCode   : %02x\n", 40, evt->status_code);
                printf(CUF "CurrentScene : %04x\n", 40, evt->current_scene);
                printf(CUF "Scenes  : %d {", 40, evt->scene_list_size);
                if (evt->scene_list != NULL && evt->scene_list_size > 0) {
                    uint16_t count = 0;
                    while(count < evt->scene_list_size) {
                        printf("%04x", evt->scene_list[count]);
                        count++;
                    }
                }
                printf("}\n");
                break;
            }
            default: {
                AB_Log_HexDisplay(40, &msg->buffer[0], msg->length, NULL);
                break;
            }
        }
    }

    static void mesh_create_device(void)
    {
        bt_mesh_model_add_scene_client(&model_handle, element_index, _scene_client_msg_handler, NULL);
    }

*      @endcode
*

*/

#include "bt_mesh_access.h"

/**
 * @defgroup Bluetooth_mesh_scene_model_enum Enum
 * @{
 */

/*!
     @brief Scene client event id
     @{
 */
typedef enum {
    BT_MESH_SCENE_CLIENT_EVT_STATUS,                    /**< Event for scene status. */
    BT_MESH_SCENE_CLIENT_EVT_REGISTER_STATUS,           /**< Event for scene register status. */
} bt_mesh_scene_client_event_id_t;
/*!  @} */
/**
 * @}
 */

/**
 * @defgroup Bluetooth_mesh_scene_model Struct
 * @{
*/

/*!
     @brief Message parameter of scene status
 */
typedef struct {
    uint8_t status_code;   /**< Indicate the the status code for the last operation.. */
    uint16_t current_scene;   /**< Indicate the Scene Number of a current scene. */
    uint16_t target_scene;   /**< Indicate the Scene Number of a target scene. (Optional) */
    uint8_t remaining_time; /**< Indicate the remaining time of transition. (Shall be present if target_scene exists)*/
} __attribute__((packed)) bt_mesh_scene_client_evt_status_t;

/*!
     @brief Message parameter of scene register status
 */
typedef struct {
    uint8_t status_code;   /**< Indicate the the status code for the last operation. */
    uint16_t current_scene;   /**< Indicate the Scene Number of a current scene. */
    uint16_t *scene_list;    /**< A list of scenes stored within an element */
    uint16_t scene_list_size;   /**< The number of scene_list. */
} __attribute__((packed)) bt_mesh_scene_client_evt_register_status_t;

/*!
     @brief Scene client event structure.
 */
typedef struct {
    bt_mesh_scene_client_event_id_t evt_id;   /**<  Scene client event ID. */
    union {
        bt_mesh_scene_client_evt_status_t                   scene_status;    /**<  Scene status */
        bt_mesh_scene_client_evt_register_status_t          register_status;    /**<  Scene Register status */
    } data; /**<  Union for data. */
} bt_mesh_scene_client_evt_t;
/**
 * @}
 */

/**
 * @defgroup Bluetooth_mesh_scene_model_typedef Typedef
 * @{
*/

/** @brief  This defines the scene client event handler prototype.
 *  @param[in] model_handle is the model handle which wants to handle these messages.
 *  @param[in] msg is the original received message.
 *  @param[in] event is the parsed scene client event.
 *  @return NONE
 * #bt_mesh_model_add_scene_client,\n
 */
typedef void (*bt_mesh_scene_client_evt_handler)(uint16_t model_handle, const bt_mesh_access_message_rx_t *msg, const bt_mesh_scene_client_evt_t *event);
/**
 * @}
 */

/*!
    @brief Add a scene client model.
    @param[out] model_handle is the handle of this added model.
    @param[in] element_index is the index of element that this model to be added in.
    @param[in] callback is the message handler for scene server model.
    @param[in] publish_timeout_cb is the periodic publishing timeout callback.
    @return
    @c true means adding scene client model successfully. \n
    @c false means adding scene client model failed.
*/
bool bt_mesh_model_add_scene_client(
    uint16_t *model_handle, uint16_t element_index,
    bt_mesh_scene_client_evt_handler callback, bt_mesh_access_publish_timeout_cb_t publish_timeout_cb);

/*!
 * @brief Sends a request to store scene.
 * @param[in] model_handle is the model handle which the message belongs to.
 * @param[in] tx_meta is the metadata of the message.
 * @param[in] scene_number is the number of the scene to be stored.
 * @param[in] reliable is to send the request as a reliable message or not.
 * @note Response: #BT_MESH_SCENE_CLIENT_EVT_STATUS
 * @return
 * #BT_MESH_SUCCESS, packets successfully sent. \n
 * #BT_MESH_ERROR_OOM, not enough memory for sending the packet. \n
 * #BT_MESH_ERROR_NULL, tx_params or data is NULL. \n
 * #BT_MESH_ERROR_INVALID_ADDR, invalid source or destination address. \n
 * #BT_MESH_ERROR_INVALID_TTL, invalid TTL value. \n
 * #BT_MESH_ERROR_INVALID_KEY, invalid key index or no bound network key. \n
*/
bt_mesh_status_t bt_mesh_model_scene_client_store(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta,
    uint16_t scene_number, bool reliable);

/*!
 * @brief Sends a request to recall scene.
 * @param[in] model_handle is the model handle which the message belongs to.
 * @param[in] tx_meta is the metadata of the message.
 * @param[in] buffer is the message payload to be sent.
 * @param[in] buffer_length is the message payload length.
 * @param[in] reliable is to send the request as a reliable message or not.
 * @note Response: #BT_MESH_SCENE_CLIENT_EVT_STATUS
 * @return
 * #BT_MESH_SUCCESS, packets successfully sent. \n
 * #BT_MESH_ERROR_OOM, not enough memory for sending the packet. \n
 * #BT_MESH_ERROR_NULL, tx_params or data is NULL. \n
 * #BT_MESH_ERROR_INVALID_ADDR, invalid source or destination address. \n
 * #BT_MESH_ERROR_INVALID_TTL, invalid TTL value. \n
 * #BT_MESH_ERROR_INVALID_KEY, invalid key index or no bound network key. \n
 * @note buffer should be composed in this format
    Field       | Size (octets) | Notes
    ------------|---------------|-------
    SceneNumber | 2 | The number of the scene to be recalled.
    TID         | 1 | Transaction Identifier
    Transition Time | 1 | Transition time (Optional)
    Delay       | 1 | Delay time. If the Transition Time field is present, the Delay field shall also be present; otherwise these fields shall not be present.
*/
bt_mesh_status_t bt_mesh_model_scene_client_recall(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta,
    uint8_t *buffer, uint8_t buffer_length, bool reliable);

/*!
 * @brief Sends a request to delete scene.
 * @param[in] model_handle is the model handle which the message belongs to.
 * @param[in] tx_meta is the metadata of the message.
 * @param[in] scene_number is the number of the scene to be deleted.
 * @param[in] reliable is to send the request as a reliable message or not.
 * @note Response: #BT_MESH_SCENE_CLIENT_EVT_STATUS
 * @return
 * #BT_MESH_SUCCESS, packets successfully sent. \n
 * #BT_MESH_ERROR_OOM, not enough memory for sending the packet. \n
 * #BT_MESH_ERROR_NULL, tx_params or data is NULL. \n
 * #BT_MESH_ERROR_INVALID_ADDR, invalid source or destination address. \n
 * #BT_MESH_ERROR_INVALID_TTL, invalid TTL value. \n
 * #BT_MESH_ERROR_INVALID_KEY, invalid key index or no bound network key. \n
*/
bt_mesh_status_t bt_mesh_model_scene_client_delete(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta,
    uint16_t scene_number, bool reliable);

/*!
 * @brief Sends a request to get scene state.
 * @param[in] model_handle is the model handle which the message belongs to.
 * @param[in] tx_meta is the metadata of the message.
 * @note Response: #BT_MESH_SCENE_CLIENT_EVT_STATUS
 * @return
 * #BT_MESH_SUCCESS, packets successfully sent. \n
 * #BT_MESH_ERROR_OOM, not enough memory for sending the packet. \n
 * #BT_MESH_ERROR_NULL, tx_params or data is NULL. \n
 * #BT_MESH_ERROR_INVALID_ADDR, invalid source or destination address. \n
 * #BT_MESH_ERROR_INVALID_TTL, invalid TTL value. \n
 * #BT_MESH_ERROR_INVALID_KEY, invalid key index or no bound network key. \n
*/
bt_mesh_status_t bt_mesh_model_scene_client_get(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta);

/*!
 * @brief Sends a request to get scene register.
 * @param[in] model_handle is the model handle which the message belongs to.
 * @param[in] tx_meta is the metadata of the message.
 * @note Response: #BT_MESH_SCENE_CLIENT_EVT_REGISTER_STATUS
 * @return
 * #BT_MESH_SUCCESS, packets successfully sent. \n
 * #BT_MESH_ERROR_OOM, not enough memory for sending the packet. \n
 * #BT_MESH_ERROR_NULL, tx_params or data is NULL. \n
 * #BT_MESH_ERROR_INVALID_ADDR, invalid source or destination address. \n
 * #BT_MESH_ERROR_INVALID_TTL, invalid TTL value. \n
 * #BT_MESH_ERROR_INVALID_KEY, invalid key index or no bound network key. \n
*/
bt_mesh_status_t bt_mesh_model_scene_client_get_register(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta);


/*!
@}
@}
@}
*/

#endif // __BT_MESH_MODEL_SCENE_CLIENT_H__


