/*
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

#ifndef __BLE_VOCS_DEF_H__
#define __BLE_VOCS_DEF_H__

#include "bt_type.h"

/**
 * @brief The VOCS service start handle for #BLE_VOCS_CHANNEL_1.
 */
#define BLE_VOCS_START_HANDLE_CHANNEL_1                             (0x2001)    /**< VOCS channel 1 Start Handle. */
#define BLE_VOCS_VALUE_HANDLE_OFFSET_STATE_CHANNEL_1                (0x2003)    /**< Attribute Value Handle of Volume State Characteristic. */
#define BLE_VOCS_VALUE_HANDLE_AUDIO_LOCATION_CHANNEL_1              (0x2006)    /**< Attribute Value Handle of Volume State Characteristic. */
#define BLE_VOCS_VALUE_HANDLE_CONTROL_POINT_CHANNEL_1               (0x2009)    /**< Attribute Value Handle of Volume Control Point Characteristic. */
#define BLE_VOCS_VALUE_HANDLE_AUDIO_OUTPUT_DESCRIPTION_CHANNEL_1    (0x200B)    /**< Attribute Value Handle of Volume Flags Characteristic. */
#define BLE_VOCS_END_HANDLE_CHANNEL_1                               (0x200C)    /**< VOCS channel 1 End Handle. */

/**
 * @brief The VOCS service start handle for #BLE_VOCS_CHANNEL_2.
 */
#define BLE_VOCS_START_HANDLE_CHANNEL_2                             (0x3001)    /**< VOCS channel 2 Start Handle. */
#define BLE_VOCS_VALUE_HANDLE_OFFSET_STATE_CHANNEL_2                (0x3003)    /**< Attribute Value Handle of Volume State Characteristic. */
#define BLE_VOCS_VALUE_HANDLE_AUDIO_LOCATION_CHANNEL_2              (0x3006)    /**< Attribute Value Handle of Audio Location Characteristic. */
#define BLE_VOCS_VALUE_HANDLE_CONTROL_POINT_CHANNEL_2               (0x3009)    /**< Attribute Value Handle of Volume Control Point Characteristic. */
#define BLE_VOCS_VALUE_HANDLE_AUDIO_OUTPUT_DESCRIPTION_CHANNEL_2    (0x300B)    /**< Attribute Value Handle of Volume Flags Characteristic. */
#define BLE_VOCS_END_HANDLE_CHANNEL_2                               (0x300C)    /**< VOCS channel 2 End Handle. */

/**
 * @brief The VOCS service UUID.
 */
#define BT_SIG_UUID16_VOLUME_OFFSET_CONTROL_SERVICE    (0x1845)    /**< Volume offset control service. */

/**
 * @brief The VOCS characteristic UUID definitions.
 */
#define BT_SIG_UUID16_OFFSET_STATE                      (0x2B80)    /**< Offset State. */
#define BT_SIG_UUID16_AUDIO_LOCATION                    (0x2B81)    /**< Audio Location. */
#define BT_SIG_UUID16_VOLUME_OFFSET_CONTROL_POINT       (0x2B82)    /**< Volume Offset Control Point. */
#define BT_SIG_UUID16_AUDIO_OUTPUT_DESCRIPTION          (0x2B83)    /**< Audio Output Description. */

/**
 * @brief The VOCS UUID type definitions.
 */
#define BLE_VOCS_UUID_TYPE_VOLUME_OFFSET_CONTROL_SERVICE 0           /**< Volume offset control service. */
#define BLE_VOCS_UUID_TYPE_OFFSET_STATE                  1           /**< Offset state. */
#define BLE_VOCS_UUID_TYPE_AUDIO_LOCATION                2           /**< Audio location. */
#define BLE_VOCS_UUID_TYPE_VOLUME_OFFSET_CONTROL_POINT   3           /**< Volume offset control point. */
#define BLE_VOCS_UUID_TYPE_AUDIO_OUTPUT_DESCRIPTION      4           /**< Audio output description. */
#define BLE_VOCS_UUID_TYPE_MAX_NUM                       5           /**< The max number of VOCS UUID type.*/
#define BLE_VOCS_UUID_TYPE_INVALID                       0xFF        /**< The invalid VOCS UUID type.*/
typedef uint8_t ble_vocs_uuid_t;                                     /**< UUID type.*/

/**
 * @brief The VOCS max number of characteristics.
 */
#define BLE_VOCS_MAX_CHARC_NUMBER     (BLE_VOCS_UUID_TYPE_MAX_NUM-1)  /**< The number of VOCS characteristics.*/

#define BLE_VOCS_VOLUME_OFFSET_MIN   (-255)                          /**< The minimum value of volume offset. */
#define BLE_VOCS_VOLUME_OFFSET_MAX   (255)                           /**< The maximum value of volume offset. */

/**
 * @brief The VOCS GATT type definitions.
 */
#define BLE_VOCS_READ_VOLUME_OFFSET_STATE                0x00 /**< Read volume offset state. */
#define BLE_VOCS_READ_VOLUME_OFFSET_STATE_CCCD           0x01 /**< Read volume offset state CCCD. */
#define BLE_VOCS_WRITE_VOLUME_OFFSET_STATE_CCCD          0x02 /**< Write volume offset state CCCD. */
#define BLE_VOCS_READ_AUDIO_LOCATION                     0x03 /**< Read audio location. */
#define BLE_VOCS_WRITE_AUDIO_LOCATION                    0x04 /**< Write audio location. */
#define BLE_VOCS_READ_AUDIO_LOCATION_CCCD                0x05 /**< Read audio location CCCD. */
#define BLE_VOCS_WRITE_AUDIO_LOCATION_CCCD               0x06 /**< Write audio location CCCD. */
#define BLE_VOCS_WRITE_VOLUME_OFFSET_CONTROL_POINT       0x07 /**< Write volume offset control point. */
#define BLE_VOCS_READ_AUDIO_OUTPUT_DESCRIPTION           0x08 /**< Read audio output description. */
#define BLE_VOCS_WRITE_AUDIO_OUTPUT_DESCRIPTION          0x09 /**< Write audio output description. */
#define BLE_VOCS_READ_AUDIO_OUTPUT_DESCRIPTION_CCCD      0x0A /**< Read audio output description CCCD. */
#define BLE_VOCS_WRITE_AUDIO_OUTPUT_DESCRIPTION_CCCD     0x0B /**< Write audio output description CCCD. */
#define BLE_VOCS_GATTS_REQ_MAX                           0x0C /**< The maximum number of VOCS GATT type. */
typedef uint8_t ble_vocs_gatt_request_t;

/**
 *  @brief This structure defines VOCS attribute handle detail.
 */
typedef struct {
    ble_vocs_uuid_t uuid_type;      /**< UUID type */
    uint16_t att_handle;            /**< Attribute handle */
} ble_vocs_attribute_handle_t;

/**
 * @brief                       This function distributes VOCS request to the corresponding handler from specified remote device.
 * @param[in] req               is the type of VOCS request.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] channel           is the channel.
 * @param[in] data              is the data.
 * @param[in] size              is the size of data.
 * @return                      the size of responded data.
 *
 */
uint16_t ble_vocs_gatt_request_handler(ble_vocs_gatt_request_t req, bt_handle_t handle, uint8_t channel, void *data, uint16_t size);

#endif  /* __BLE_VOCS_DEF_H__ */

