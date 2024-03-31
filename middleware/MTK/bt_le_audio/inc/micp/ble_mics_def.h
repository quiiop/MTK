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

#ifndef __BLE_MICS_DEF_H__
#define __BLE_MICS_DEF_H__

#include "bt_type.h"

/**
 * @brief The MICS service UUID.
 */
#define BT_SIG_UUID16_MICS                             (0x184D)    /**< Microphone control service. */

/**
 * @brief The MICS characteristic UUID definitions.
 */
#define BT_SIG_UUID16_MUTE                              (0x2BC3)    /**< Mute. */

/**
 * @brief The MICS UUID type definitions.
 */
#define BLE_MICS_UUID_TYPE_MICS                          0           /**< Microphone control service. */
#define BLE_MICS_UUID_TYPE_MUTE                          1
#define BLE_MICS_UUID_TYPE_MAX_NUM                       2           /**< The max number of MICS UUID.*/
#define BLE_MICS_UUID_TYPE_INVALID                       0xFF        /**< The invalid MICS UUID type.*/
typedef uint8_t ble_mics_uuid_t;                                /**< UUID type.*/

/**
 * @brief The MICS max number of characteristics.
 */
#define BLE_MICS_MAX_CHARC_NUMBER    (BLE_MICS_UUID_TYPE_MAX_NUM-1)   /**< The number of MICS characteristics.*/

/**
 * @brief The MICS mute state.
 */
#define BLE_MICS_MUTE_STATE_UNMUTE                       0           /**< Unmute.*/
#define BLE_MICS_MUTE_STATE_MUTE                         1           /**< Mute.*/
#define BLE_MICS_MUTE_STATE_DISABLE                      2           /**< Disable.*/
typedef uint8_t ble_mics_mute_state_t;                               /**< The type of mute state.*/

/**
 * @brief The MICS GATT type definitions.
 */
#define BLE_MICS_GATT_REQUEST_READ_MUTE              0x00 /**< Read mute.*/
#define BLE_MICS_GATT_REQUEST_WRITE_MUTE             0x01 /**< Write mute.*/
#define BLE_MICS_GATT_REQUEST_READ_MUTE_CCCD         0x02 /**< Read mute CCCD.*/
#define BLE_MICS_GATT_REQUEST_WRITE_MUTE_CCCD        0x03 /**< Write mute CCCD.*/
typedef uint8_t ble_mics_gatt_request_t;

/**
 *  @brief This structure defines MICS attribute handle detail.
 */
typedef struct {
    ble_mics_uuid_t uuid_type;      /**< UUID type */
    uint16_t att_handle;            /**< Attribute handle */
} ble_mics_attribute_handle_t;

/**
 * @brief                       This function distributes MICS request to corresponded handler from specified remote device.
 * @param[in] req               is the type of AICS request.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] data              is the data.
 * @param[in] size              is the size of data.
 * @return                      the size of responded data.
 *
 */
uint16_t ble_mics_gatt_request_handler(ble_mics_gatt_request_t req, bt_handle_t handle, void *data, uint16_t size);


#endif  /* __BLE_MICS_DEF_H__ */

