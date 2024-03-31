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

#ifndef __BT_MESH_MODEL_OBJ_TRANS_SERVER_H__
#define __BT_MESH_MODEL_OBJ_TRANS_SERVER_H__
#include "bt_mesh_access.h"


/*************************************************************************
* Type define
*************************************************************************/
/*!
    @brief Noification callback functions for incomming message
*/
typedef struct {
    void (*notification_object_transfer_get)(uint8_t [8]);/**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_OBJECT_TRANSFER_GET*/
    void (*notification_object_transfer_start)(uint8_t [8], uint32_t, uint8_t);/**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_OBJECT_TRANSFER_START*/
    void (*notification_object_transfer_abort)(uint8_t [8]);/**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_OBJECT_TRANSFER_ABORT*/
    void (*notification_object_block_transfer_start)(uint8_t obj_id[8], uint16_t blk_idx, uint16_t chunk_size, uint8_t cs_algo, uint8_t checksum[4]);/**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_OBJECT_BLOCK_TRANSFER_START*/
    void (*notification_object_block_transfer_start_partial)(uint8_t obj_id[8], uint16_t blk_idx, uint16_t chunk_size, uint8_t cs_algo, uint8_t checksum[4], uint16_t blk_size);/**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_OBJECT_BLOCK_TRANSFER_START*/
    void (*notification_object_chunk_transfer)(uint16_t, uint8_t *, uint16_t); /**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_OBJECT_CHUNK_TRANSFER*/
    void (*notification_object_block_get)(uint8_t [8], uint16_t blk_idx);/**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_OBJECT_BLOCK_GET*/
} object_transfer_server_callback_t;

/*!
    @name Status code of @ref bt_mesh_model_object_transfer_server_status
*/
typedef enum {
    OBJECT_TRANSFER_STATUS_READY_NOT_ACTIVE = 0,
    OBJECT_TRANSFER_STATUS_BUSY_ACTIVE,
    OBJECT_TRANSFER_STATUS_BUSY_OTHER_ACTIVE,
    OBJECT_TRANSFER_STATUS_OBJECT_TOO_BIG,
} object_transfer_status_e;

/*!
    @name Status code of @ref bt_mesh_model_object_transfer_server_block_transfer_status
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
/*************************************************************************
* Public functions
*************************************************************************/

/*!
    @brief Add a firmware distribution server model
    @param[out] model_handle is the handle of this added model.
    @param[in] callback is the message handler for server model
    @param[in] elementidx Element index.
    @return
    true means the server model was added successfully. \n
    false means adding the server model failed.
*/
bool bt_mesh_model_add_object_transfer_server(uint16_t *model_handle, uint16_t elementidx, const object_transfer_server_callback_t *callback);

/*!
    @brief Send object transfer status to client.
    @param[in] model_handle is the handle of this model
    @param[in] status object transfer status
    @param[in] obj_id object identifier
    @param[in] obj_size object size in byte
    @param[in] blk_size_log block size by log.
    @note This should only be called in the callbacks functions' of @ref object_transfer_server_callback_t
*/
bt_mesh_status_t bt_mesh_model_object_transfer_server_status(uint16_t model_handle, object_transfer_status_e status, uint8_t obj_id[8], uint32_t obj_size, uint8_t blk_size_log);

/*!
    @brief Send block transfer status to client.
    @param[in] model_handle is the handle of this model
    @param[in] status block transfer status
    @note This should only be called in the callbacks functions' of @ref object_transfer_server_callback_t
*/
bt_mesh_status_t bt_mesh_model_object_transfer_server_block_transfer_status(uint16_t model_handle, object_transfer_block_transfer_status_e status);

/*!
    @brief Send block status to client.
    @param[in] model_handle is the handle of this model
    @param[in] status block status
    @param[in] miss_list uin16_t array of missing chunk's number.
    @param[in] miss_count count of miss_list array.
    @note This should only be called in the callbacks functions' of @ref object_transfer_server_callback_t
*/
bt_mesh_status_t bt_mesh_model_object_transfer_server_block_status(uint16_t model_handle, object_transfer_block_status_e status, uint16_t *miss_list, uint16_t miss_count);

#endif

