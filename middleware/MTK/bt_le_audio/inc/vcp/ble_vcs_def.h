/* Copyright Statement:
 *
 * (C) 2020  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#ifndef __BLE_VCS_DEF_H__
#define __BLE_VCS_DEF_H__

#include "bt_type.h"

/**
 * @brief The VCS service UUID.
 */
#define BT_SIG_UUID16_VOLUME_CONTROL_SERVICE           (0x1844)    /**< Volume control service. */

/**
 * @brief The VCS characteristic UUID definitions.
 */
#define BT_SIG_UUID16_VOLUME_STATE                      (0x2B7D)    /**< Volume State. */
#define BT_SIG_UUID16_VOLUME_CONTROL_POINT              (0x2B7E)    /**< Volume Control Point. */
#define BT_SIG_UUID16_VOLUME_FLAGS                      (0x2B7F)    /**< Volume Flags. */

/**
 * @brief The VCS UUID type definitions.
 */
#define BLE_VCS_UUID_TYPE_VOLUME_CONTROL_SERVICE         0           /**< Volume control service. */
#define BLE_VCS_UUID_TYPE_VOLUME_STATE                   1           /**< Volume State. */
#define BLE_VCS_UUID_TYPE_VOLUME_CONTROL_POINT           2           /**< Volume control point. */
#define BLE_VCS_UUID_TYPE_VOLUME_FLAGS                   3           /**< Volume flags. */
#define BLE_VCS_UUID_TYPE_MAX_NUM                        4           /**< The max number of VCS UUID type.*/
#define BLE_VCS_UUID_TYPE_INVALID                        0xFF        /**< The invalid VCS UUID type.*/
typedef uint8_t ble_vcs_uuid_t;                                      /**< UUID type.*/

#define BLE_VCS_UUID_TYPE_CHARC_START                    BLE_VCS_UUID_TYPE_VOLUME_STATE

/**
 * @brief The VCS max number of characteristics.
 */
#define BLE_VCS_MAX_CHARC_NUMBER     (BLE_VCS_UUID_TYPE_MAX_NUM-1)    /**< The number of VCS characteristics.*/

/**
 * @brief The VCS control point opcode.
 */
#define BLE_VCS_OPCODE_RELATIVE_VOLUME_DOWN              0x00        /**< Relative volume down. */
#define BLE_VCS_OPCODE_RELATIVE_VOLUME_UP                0x01        /**< Relative volume up. */
#define BLE_VCS_OPCODE_UNMUTE_RELATIVE_VOLUME_DOWN       0x02        /**< Unmute and relative volume down. */
#define BLE_VCS_OPCODE_UNMUTE_RELATIVE_VOLUME_UP         0x03        /**< Unmute and relative volume up. */
#define BLE_VCS_OPCODE_SET_ABSOLUTE_VOLUME               0x04        /**< Set absolute volume. */
#define BLE_VCS_OPCODE_UNMUTE                            0x05        /**< Unmute. */
#define BLE_VCS_OPCODE_MUTE                              0x06        /**< Mute. */
#define BLE_VCS_OPCODE_RFU                               0x07        /**< Reserved for future use. */
typedef uint8_t ble_vcs_control_opcode_t;

/**
 * @brief The VCS volume flags.
 */
#define BLE_VCS_VOLUME_FLAGS_SETTING_NOT_PERSISTED   0x0             /**< Not persisted. */
#define BLE_VCS_VOLUME_FLAGS_SETTING_PERSISTED       0x1             /**< Persisted. */
typedef uint8_t ble_vcs_volume_flags_t;


/**
 * @brief The VCS volume range.
 */
#define BLE_VCS_VOLUME_MIN           0          /**< The minimum volume value. */
#define BLE_VCS_VOLUME_MAX           0xFF       /**< The maximum volume value. */
typedef uint8_t ble_vcs_volume_t;

/**
 *  @brief Defines the VCS mute state.
 */
#define BLE_VCS_UNMUTE      0
#define BLE_VCS_MUTE        1
typedef uint8_t ble_vcs_mute_t;

/**
 * @brief The volume state.
 */
typedef struct {
    ble_vcs_volume_t volume;            /**< The volume value. */
    ble_vcs_mute_t mute;                /**< The mute state. */
} ble_vcs_volume_state_t;

/**
 * @brief The VCS GATT type definitions.
 */
#define BLE_VCS_READ_VOLUME_STATE            0x00 /**< Read volume state. */
#define BLE_VCS_READ_VOLUME_STATE_CCCD       0x01 /**< Read volume state CCCD. */
#define BLE_VCS_WRITE_VOLUME_STATE_CCCD      0x02 /**< Write volume state CCCD. */
#define BLE_VCS_WRITE_VOLUME_CONTROL_POINT   0x03 /**< Write volume control point. */
#define BLE_VCS_READ_VOLUME_FLAGS            0x04 /**< Read volume flags. */
#define BLE_VCS_READ_VOLUME_FLAGS_CCCD       0x05 /**< Read volume flags CCCD. */
#define BLE_VCS_WRITE_VOLUME_FLAGS_CCCD      0x06 /**< Write volume flags CCCD. */
#define BLE_VCS_GATTS_REQ_MAX                0x07 /**< The maximum number of VCS GATT type. */
typedef uint8_t ble_vcs_gatt_request_t;

/**
 *  @brief This structure defines VOCS attribute handle detail.
 */
typedef struct {
    ble_vcs_uuid_t uuid_type;           /**< UUID type */
    uint16_t att_handle;                /**< Attribute handle */
} ble_vcs_attribute_handle_t;

/**
 * @brief                       This function distributes VCS request to the corresponding handler from specified remote device.
 * @param[in] req               is the type of VCS request.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] data              is the data.
 * @param[in] size              is the size of data.
 * @return                      the size of responded data.
 *
 */
uint32_t ble_vcs_gatt_request_handler(ble_vcs_gatt_request_t req, bt_handle_t handle, void *data, uint16_t size);

#endif  /* __BLE_VCS_DEF_H__ */

