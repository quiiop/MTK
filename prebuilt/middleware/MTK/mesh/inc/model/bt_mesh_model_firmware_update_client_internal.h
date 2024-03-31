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

#ifndef __BT_MESH_MODEL_FW_UPDATE_CLIENT_H__
#define __BT_MESH_MODEL_FW_UPDATE_CLIENT_H__
#include "bt_mesh_access.h"

/*************************************************************************
* Type define
*************************************************************************/

/*!
    @brief status of firmware update.
*/
typedef enum {
    fw_update_success = 0,              /**! success*/
    fw_update_wrong_cid_fid,            /**! wrong company or firmware identifier*/
    fw_update_other_ongoing,            /**! another update is ongoing */
    fw_update_cid_fid_apply_failed,     /**! fail to apply company or firmware identfier*/
    fw_update_cid_fid_reject_too_old,   /**! firmware too old*/
    fw_update_cid_fid_reject_failed,    /**! firmware update failed*/
    fw_update_unkown_failed,            /**! vendor defined failed*/
} fw_update_status_e;

/*!
   @breaif phase of firmware update.
*/
typedef enum {
    fw_update_idle = 0,            /**! No ongoing firmware update */
    fw_update_prepared,             /**! Preapared for a upcoming firmware update*/
    fw_update_in_progress,         /**! firmware update is ongoing*/
    fw_update_dfu_ready,           /**! firmware has been downloaded completely*/
} fw_update_phase_e;

typedef struct {
    void (*confirmation_firmware_update_status)(uint16_t addr, fw_update_status_e status, fw_update_phase_e phase, uint32_t firmware_id, uint8_t obj_id[8]);
    void (*confirmation_firmware_info_status)(uint16_t addr, uint16_t cid, uint32_t firmware_id, uint8_t *url);
} firmware_update_client_callbacks_t;

/*************************************************************************
* Public functions
*************************************************************************/

/*!
    @brief Get firmware information
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @return
#BT_MESH_SUCCESS, send request successfully.\n
#BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_fw_update_client_get_information(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta);

/*!
    @brief Get the current status of the firmware update process.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] company_id Company identifier.
    @param[in] fw_id Firmware identifier.
    @return
#BT_MESH_SUCCESS, send request successfully.\n
#BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_fw_update_client_get(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint16_t company_id, uint32_t fw_id);

/*!
    @brief Prepare for the firmware update process.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] company_id Company identifier.
    @param[in] fw_id Firmware identifier.
    @param[in] obj_id Object identifier.
    @return
#BT_MESH_SUCCESS, send request successfully.\n
#BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_fw_update_client_prepare(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint16_t company_id, uint32_t fw_id, uint8_t obj_id[8]);

/*!
    @brief Start the firmware update process.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data.
    @param[in] policy Update policy.
    @param[in] company_id Company identifier.
    @param[in] fw_id Firmware identifier.
    @return
#BT_MESH_SUCCESS, send request successfully.\n
#BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_fw_update_client_start(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint8_t policy, uint16_t company_id, uint32_t fw_id);

/*!
    @brief Abort the firmware update and delete any stored information about the update.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] company_id Company identifier.
    @param[in] fw_id Firmware identifier.
    @return
#BT_MESH_SUCCESS, send request successfully.\n
#BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_fw_update_client_abort(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint16_t company_id, uint32_t fw_id);

/*!
    @brief Apply the new firmware stored in the device.
    @param[in] model_handle Handle of model
    @param[in] tx_meta Message meta-data
    @param[in] company_id Company identifier.
    @param[in] fw_id Firmware identifier.
    @return
#BT_MESH_SUCCESS, send request successfully.\n
#BT_MESH_ERROR_OOM, not enough memory for sending request. \n
*/
bt_mesh_status_t bt_mesh_model_fw_update_client_apply(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta,  uint16_t company_id, uint32_t fw_id);


/*!
    @brief Add a firmware update client model
    @param[out] model_handle is the handle of this added model.
    @param[in] callback is the message handler for client model
    @return
    @c true means the client model was added successfully. \n
    @c false means adding the client model failed.
*/
bool bt_mesh_model_add_fw_update_client(uint16_t *model_handle, uint16_t elementidx, const firmware_update_client_callbacks_t *callback);

#endif
