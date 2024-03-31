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

#ifndef __BT_MESH_MODEL_OBJ_TRANS_CLIENT_H__
#define __BT_MESH_MODEL_OBJ_TRANS_CLIENT_H__
#include "bt_mesh_access.h"


/*************************************************************************
* Typedefine
*************************************************************************/
/*!
    @name Status code of @ref confirmation_obj_transfer_status
*/
typedef enum {
    OBJECT_TRANSFER_STATUS_READY_NOT_ACTIVE = 0,
    OBJECT_TRANSFER_STATUS_BUSY_ACTIVE,
    OBJECT_TRANSFER_STATUS_BUSY_OTHER_ACTIVE,
    OBJECT_TRANSFER_STATUS_OBJECT_TOO_BIG,
} object_transfer_status_e;

/*!
    @name Status code of @ref confirmation_obj_transfer_status
*/
typedef enum {
    BLOCK_TRANSFER_STATUS_ACCEPTED = 0,
    BLOCK_TRANSFER_STATUS_ALREADY_TRANSFERRED,
    BLOCK_TRANSFER_STATUS_INVALID_BLOCK_NUMBER, //no previous
    BLOCK_TRANSFER_STATUS_WRONG_BLOCK_SIZE,
    BLOCK_TRANSFER_STATUS_WRONG_CHUNK_SIZE,
    BLOCK_TRANSFER_STATUS_UNKNOWN_CHECKSUM,
    BLOCK_TRANSFER_STATUS_REJECTED = 0x0F,
} object_transfer_block_transfer_status_e;

/*!
    @name Status code of @ref bt_mesh_model_object_transfer_server_block_status
*/
typedef enum {
    BLOCK_STATUS_ALL_CHUNK_RECEIVED = 0,
    BLOCK_STATUS_NOT_ALL_CHUNK_RECEIVED,
    BLOCK_STATUS_WRONG_CHECKSUM,
    BLOCK_STATUS_WRONG_OBJECT_ID,
    BLOCK_STATUS_WRONG_BLOCK,
} object_transfer_block_status_e;


/*!
    @brief structure for object transfer client model callback functions.
*/
typedef struct {
    void (*confirmation_obj_info_status)(uint16_t,  uint8_t, uint8_t, uint16_t); /**< object infomation status */
    void (*confirmation_obj_transfer_status)(uint16_t, object_transfer_status_e, uint8_t [8], uint32_t, uint8_t);/**< object transfer status*/
    void (*confirmation_block_transfer_status)(uint16_t, object_transfer_block_transfer_status_e); /**< block transfer status*/
    void (*confirmation_block_status)(uint16_t, object_transfer_block_status_e, uint16_t *, uint8_t);
} object_transfer_client_callbacks_t;


/*************************************************************************
* Public functions
*************************************************************************/

/*!
    @brief Add a firmware distribution client model.
    @param[out] model_handle is the handle of this added model.
    @param[in] callback callback functions for client model
    @return
    @c true means the client model was added successfully. \n
    @c false means adding the client model failed.
*/
bool bt_mesh_model_add_object_transfer_client( uint16_t *model_handle, uint16_t elementidx, const object_transfer_client_callbacks_t *callback);

/*!
    @brief Get the current state of the object transfer process.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] obj_id Object identifier
    @return
    #BT_MESH_SUCCESS, send request successfully.\n
    #BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_object_transfer_client_get(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint8_t obj_id[8]);
/*!
    @brief Start new object transfer process.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] obj_id Object identifier
    @param[in] obj_sz Object size
    @param[in] blk_sz_log Log of Block size.
    @return
    #BT_MESH_SUCCESS, send request successfully.\n
    #BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_object_transfer_client_start(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta,  uint8_t obj_id[8], uint32_t obj_sz, uint8_t blk_sz_log);
/*!
    @brief Abort the ongoing object transfer process.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] obj_id Object identifier
    @return
    #BT_MESH_SUCCESS, send request successfully.\n
    #BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_object_transfer_client_abort(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint8_t obj_id[8]);
/*!
    @brief Start block transfer to the node.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] obj_id Object identifier
    @param[in] blk_num Block number
    @param[in] chunk_sz Chunk size
    @param[in] cs_algo Checksum algorithm
    @param[in] cs Checksum byte array
    @param[in] cs_len Check byte array length
    @param[in] blk_sz Block size. 0xffff means use block size provided by @ref bt_mesh_model_object_transfer_client_start.
    @return
    #BT_MESH_SUCCESS, send request successfully.\n
    #BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_object_transfer_client_start_block(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint8_t obj_id[8], uint16_t blk_num, uint16_t chunk_sz, uint8_t cs_algo, uint8_t *cs, uint8_t cs_len, uint16_t blk_sz);
/*!
    @brief Deliver chunk of a current block to the node or to the group of nodes.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] chunk_num Chunk number
    @param[in] buf Chunk data byte array
    @param[in] len Chunk data byte array length.
    @return
    #BT_MESH_SUCCESS, send request successfully.\n
    #BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_object_transfer_client_start_chunk(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint16_t chunk_num, uint8_t *buf, uint16_t len);
/*!
    @brief Get status of the current block transfer
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] obj_id Object identifier
    @param[in] blk_num Block number
    @return
    #BT_MESH_SUCCESS, send request successfully.\n
    #BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_object_transfer_client_get_block(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint8_t obj_id[8], uint16_t blk_num);
/*!
    @brief Get object transfer capabilities of the node.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @return
    #BT_MESH_SUCCESS, send request successfully.\n
    #BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_object_transfer_client_get_information(uint16_t model_handle,  const bt_mesh_access_message_tx_meta_t *tx_meta);

#endif

