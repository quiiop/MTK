/* Copyright Statement:
 *
 * (C) 2022  Airoha Technology Corp. All rights reserved.
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

#ifndef __BLE_HAPS_H__
#define __BLE_HAPS_H__

#include "ble_has_def.h"

/**
 *  @brief Defines the HAPS events.
 */
#define BLE_HAPS_EVT_PRESET_LOCK_IND            (BT_LE_AUDIO_MODULE_HAPS | 0x0001)    /**< Preset operation is disallowed. */
#define BLE_HAPS_EVT_PRESET_UNLOCK_IND          (BT_LE_AUDIO_MODULE_HAPS | 0x0002)    /**< Preset operation is allowed. */
#define BLE_HAPS_EVT_PRESET_NAME_CHANGED_IND    (BT_LE_AUDIO_MODULE_HAPS | 0x0003)    /**< Name of preset is changed. */
#define BLE_HAPS_EVT_PRESET_ACTIVE_IND          (BT_LE_AUDIO_MODULE_HAPS | 0x0004)    /**< Preset is active. */

/**
 * @brief The Preset Property definitions.
 */
#define HAS_PRESET_WRITABLE                     (1 << 0)    /**< Name of preset is writable. */
#define HAS_PRESET_AVAILABLE                    (1 << 1)    /**< Preset is available. */

/**
 * @brief The Preset type definitions.
 */
#define HEARING_AID_TYPE_BINAURAL               0           /**< Binaural hearing aid */
#define HEARING_AID_TYPE_MONAURAL               1           /**< Monaural hearing aid */
#define HEARING_AID_TYPE_BANDED                 2           /**< Banded hearing aid */

/**
 * @brief The flag parameter value of API ble_haps_change_preset.
 */
#define HAS_CHANGE_FLAG_PRESET_PROPERTY         (1 << 0)    /**< flag for ble_haps_change_preset API */
#define HAS_CHANGE_FLAG_PRESET_NAME             (1 << 1)    /**< flag for ble_haps_change_preset API */

/**
 *  @brief This structure defines HAPS event BLE_HAPS_EVT_PRESET_NAME_CHANGED_IND detail.
 */
typedef struct
{
    uint8_t index;                              /**< Preset index */
    uint8_t name_len;                           /**< Preset name length */
    uint8_t name[1];                            /**< Preset name */
} ble_haps_evt_preset_name_changed_ind_t;

/**
 *  @brief This structure defines HAPS event BLE_HAPS_EVT_PRESET_ACTIVE_IND detail.
 */
typedef struct
{
    uint8_t index;                              /**< Preset index */
} ble_haps_evt_preset_active_ind_t;


/**
 * @brief                       This function is used to initialize HAP Server.
 * @param[in] max_link_num      is the max number of Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_OUT_OF_MEMORY, not enough memory to complete this operation.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_haps_init_server(uint8_t max_link_num);

/**
 * @brief                       This function is used to add new Preset.
 * @param[in] index             is the index of Preset.
 * @param[in] property          is the property of Preset.
 * @param[in] len               is the length of Preset name.
 * @param[in] name              is the name of Preset.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_OUT_OF_MEMORY, not enough memory to complete this operation.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_haps_add_new_preset(uint8_t index, uint8_t property, uint8_t len, void *name);

/**
 * @brief                       This function is used to remove Preset.
 * @param[in] index             is the index of Preset.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_haps_remove_preset(uint8_t index);

/**
 * @brief                       This function is used to change the property and name of Preset.
 * @param[in] index             is the index of Preset.
 * @param[in] flag              is the value to indicate what change to do.
 * @param[in] property          is the property of Preset.
 * @param[in] len               is the length of Preset name.
 * @param[in] name              is the name of Preset.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_OUT_OF_MEMORY, not enough memory to complete this operation.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_haps_change_preset(uint8_t index, uint8_t flag, uint8_t property, uint8_t len, void *name);

/**
 * @brief                       This function is used to set the active Preset.
 * @param[in] index             is the index of Preset.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_haps_set_active_preset(uint8_t index);

#endif
