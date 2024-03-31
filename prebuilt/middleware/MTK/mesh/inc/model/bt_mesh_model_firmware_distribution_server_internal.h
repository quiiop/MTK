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

#ifndef __BT_MESH_MODEL_FW_DIS_SERVER_H__
#define __BT_MESH_MODEL_FW_DIS_SERVER_H__
#include "bt_mesh_access.h"

/*************************************************************************
* Type define
*************************************************************************/
/*!
    @brief Noification callback functions for incomming message
*/
typedef struct {
    void (*indication_firmware_distribution_start)(uint16_t cid, uint32_t, uint16_t, uint16_t *, uint8_t); /**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_FIRMWARE_DISTRIBUTION_START*/
    void (*indication_firmware_distribution_stop)(uint16_t, uint32_t); /**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_FIRMWARE_DISTRIBUTION_STOP*/
    void (*indication_firmware_distribution_get)(uint16_t, uint32_t); /**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_FIRMWARE_DISTRIBUTION_GET*/
    void (*indication_firmware_distribution_detail_get)(uint16_t, uint32_t); /**< Indicate and inform received @ref BT_MESH_ACCESS_MSG_FIRMWARE_DISTRIBUTION_DETAIL_GET*/
} firmware_distribution_server_callback_t;

/*!
    @name Status code of @ref bt_mesh_model_firmware_distribution_server_status
*/
typedef enum {
    BT_MESH_DISTRIBUTION_STATUS_READY = 0,
    BT_MESH_DISTRIBUTION_STATUS_ACTIVE,
    BT_MESH_DISTRIBUTION_STATUS_CID_FID_INVALID,
    BT_MESH_DISTRIBUTION_STATUS_BUSY_WITH_OTHER,
    BT_MESH_DISTRIBUTION_STATUS_NODES_TOO_LONG,
    BT_MESH_DISTRIBUTION_STATUS_UNKNOWN_ERROR,
    BT_MESH_DISTRIBUTION_STATUS_NOT_SUPPORTED,
} firmware_distribution_status_e;

/*************************************************************************
* Public functions
*************************************************************************/

/*!
    @brief Add a firmware distribution server model
    @param[out] model_handle is the handle of this added model.
    @param[in] cb Callback functions. Usually called by received client's message, or get some information from upper layer.
    @return
    @c true means the server model was added successfully. \n
    @c false means adding the server model failed.
*/
bool bt_mesh_model_add_firmware_distribution_server(uint16_t *model_handle, uint16_t elementidx, const firmware_distribution_server_callback_t *cb);


/*!
    @brief Send distribution status to client.
    @param[in] model_handle is the handle of this model
    @param[in] status firmware distribution status
    @param[in] firmware_id firmware update identifier
    @note This should only be called in the callbacks functions' of @ref firmware_distribution_server_callback_t

*/
bt_mesh_status_t bt_mesh_model_firmware_distribution_server_status(uint16_t model_handle, uint32_t firmware_id, firmware_distribution_status_e status);

/*!
    @brief Response the distribution detail get
    @param[in] model_handle is the handle of this model.
    @param[in] node unicast address of updaters.
    @param[in] status status of updaters.
    @param[in] count node count.
*/
bt_mesh_status_t bt_mesh_model_firmware_distribution_detail_list(uint16_t model_handle, uint16_t nodes[], uint8_t status[], uint16_t count);

#endif
