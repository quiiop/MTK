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

#ifndef __BT_MESH_MODEL_FW_UPDATE_SERVER_H__
#define __BT_MESH_MODEL_FW_UPDATE_SERVER_H__
#include "bt_mesh_access.h"


/*************************************************************************
* Type define
*************************************************************************/
/*!
    @brief Callback functions of update server.
*/
typedef struct {
    void (*notification_firmware_update_prepare)(uint16_t, uint32_t, uint8_t [8]); /**! This will be called after received BT_MESH_ACCESS_MSG_FIRMWARE_UPDATE_PREPARE*/
    void (*notification_firmware_update_start)(uint8_t, uint16_t, uint32_t); /**! This will be called after received BT_MESH_ACCESS_MSG_FIRMWARE_UPDATE_START*/
    void (*notification_firmware_update_abort)(uint16_t, uint32_t); /**! This will be called after received BT_MESH_ACCESS_MSG_FIRMWARE_UPDATE_ABORT*/
    void (*notification_firmware_infomation_get)();/**! This will be called after received BT_MESH_ACCESS_MSG_FIRMWARE_INFORMATION_GET*/
    void (*notification_firmware_update_get)(uint16_t, uint32_t);/**! This will be called after received BT_MESH_ACCESS_MSG_FIRMWARE_UPDATE_GET*/
    void (*notification_firmware_update_apply)(uint16_t, uint32_t);/**! This will be called after received BT_MESH_ACCESS_MSG_FIRMWARE_UPDATE_APPLY*/
} firmware_update_server_callbacks_t;

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

/*************************************************************************
* Public functions
*************************************************************************/

/*!
    @brief Add a firmware update server model
    @param[out] model_handle is the handle of this added model.
    @param[in] callback is the message handler for server model
    @param[in] elementidx Element index.
    @return
    @c true means the server model was added successfully. \n
    @c false means adding the server model failed.
*/
bool bt_mesh_model_add_fw_update_server(uint16_t *model_handle, uint16_t elementidx, const firmware_update_server_callbacks_t *callback);


/*!
    @brief Send update status to client.
    @param[in] model_handle is the handle of this model
    @param[in] status firmware update status
    @param[in] phase firmware update phase
    @param[in] firmware_id firmware update identifier
    @param[in] obj_id firmware update object identifier
    @note This should only be called in the callbacks functions' of @ref firmware_update_server_callbacks_t

*/
bt_mesh_status_t bt_mesh_model_fw_update_server_status(uint16_t model_handle, fw_update_status_e status, fw_update_phase_e phase, uint32_t firmware_id, uint8_t obj_id[8]);

/*!
    @brief Send infmation status to client.
    @param[in] model_handle is the handle of this model
    @param[in] firmware_id firmware update identifier
    @note This should only be called in the callbacks functions' of @ref firmware_update_server_callbacks_t

*/bt_mesh_status_t bt_mesh_model_fw_update_information_status(uint16_t model_handle, uint32_t firmware_id);

#endif
