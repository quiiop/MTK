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

#ifndef __BT_MESH_MODEL_FW_DIS_CLIENT_H__
#define __BT_MESH_MODEL_FW_DIS_CLIENT_H__
#include "bt_mesh_access.h"


/*************************************************************************
* Type define
*************************************************************************/

/*************************************************************************
* Public functions
*************************************************************************/
/*!
    @brief Get the state of the firmware distribution process of the firmware distributor node for a given firmware.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] company_id Company identifier.
    @param[in] fw_id Firmware identifier.
    @return
#BT_MESH_SUCCESS, send request successfully.\n
#BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_firmware_distribution_client_get(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint16_t company_id, uint32_t fw_id);
/*!
    @brief  Start the firmware distribution to the group of the nodes.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] company_id Company identifier.
    @param[in] fw_id Firmware identifier.
    @param[in] grp_addr Group address for multicast update mode.
    @param[in] nodes Update nodes list
    @param[in] node_count Update nodes list count.
    @return
#BT_MESH_SUCCESS, send request successfully.\n
#BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_firmware_distribution_client_start(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint16_t company_id, uint32_t fw_id, uint16_t grp_addr, uint16_t *nodes, uint8_t node_count);
/*!
    @brief Stop the firmware distribution.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] company_id Company identifier.
    @param[in] fw_id Firmware identifier.
    @return
#BT_MESH_SUCCESS, send request successfully.\n
#BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_firmware_distribution_client_stop(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint16_t company_id, uint32_t fw_id);

/*!
    @brief Get the current status of the firmware update node list.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] company_id Company identifier.
    @param[in] fw_id Firmware identifier.
    @return
#BT_MESH_SUCCESS, send request successfully.\n
#BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_firmware_distribution_client_get_detail(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint16_t company_id, uint32_t fw_id);


/*!
    @brief Add a firmware distribution client model
    @param[out] model_handle is the handle of this added model.
    @param[in] callback is the message handler for client model
    @return
    @c true means the client model was added successfully. \n
    @c false means adding the client model failed.
*/
bool bt_mesh_model_add_firmware_distribution_client(uint16_t *model_handle, uint16_t elementidx, bt_mesh_access_msg_handler callback);

#endif
