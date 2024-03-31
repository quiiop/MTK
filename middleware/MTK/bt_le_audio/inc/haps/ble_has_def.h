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

#ifndef __BLE_HAS_DEF_H__
#define __BLE_HAS_DEF_H__

#include "bt_le_audio_type.h"
#include "bt_le_audio_def.h"

/**
 * @brief The HAS service UUID.
 */
#define BT_GATT_UUID16_HAS_SERVICE                     (0x1854)    /**< Hearing Access Service UUID. */

/**
* @brief HAS characteristic UUID.
*/
#define BT_SIG_UUID16_HAS_FEATURES                     (0x2BDA)    /**< Hearing Aid Features Characteristic UUID. */
#define BT_SIG_UUID16_HAS_PRESET_CONTROL_POINT         (0x2BDB)    /**< Hearing Aid Preset Control Point Characteristic UUID. */
#define BT_SIG_UUID16_HAS_ACTIVE_PRESET_INDEX          (0x2BDC)    /**< Active Preset Index Characteristic UUID. */

/**
* @brief HAS ATT handle.
*/
#define HAS_HANDLE_START                               0xA600      /**< HAS service start handle.*/
#define HAS_HANDLE_FEATURES_VALUE                      0xA602      /**< HAS Features Characteristic handle.*/
#define HAS_HANDLE_PRESET_CONTROL_POINT_VALUE          0xA604      /**< HAS Preset Control Point Characteristic handle.*/
#define HAS_HANDLE_PRESET_CONTROL_POINT_CCCD           0xA605      /**< HAS Preset Control Point Client Config Descriptor handle.*/
#define HAS_HANDLE_ACTIVE_PRESET_INDEX_VALUE           0xA607      /**< HAS Active Preset Index Characteristic handle.*/
#define HAS_HANDLE_ACTIVE_PRESET_INDEX_CCCD            0xA608      /**< HAS Active Preset Index Client Config Descriptor handle.*/
#define HAS_HANDLE_END                                 0xA608      /**< HAS service end handle.*/

/**
 * @brief HAS Handle type.
 */
#define HAS_HDL_TYPE_FEATURES                          0
#define HAS_HDL_TYPE_PRESET_CTRL_PT                    1
#define HAS_HDL_TYPE_ACTIVE_PRESET_INDEX               2
#define HAS_HDL_TYPE_MAX_NUM                           3

/**
 * @brief The HAS GATT request type definitions.
 */
#define BLE_HAS_READ_FEATURES_VAL                      0x00        /**< Read Features */
#define BLE_HAS_READ_FEATURES_CCCD                     0x01        /**< Read Features CCCD */
#define BLE_HAS_WRITE_FEATURES_CCCD                    0x02        /**< Write Features CCCD */
#define BLE_HAS_WRITE_PRESET_CONTROL_POINT_VAL         0x03        /**< Write Preset Control Point */
#define BLE_HAS_READ_PRESET_CONTROL_POINT_CCCD         0x04        /**< Read Preset Control Point CCCD */
#define BLE_HAS_WRITE_PRESET_CONTROL_POINT_CCCD        0x05        /**< Write Preset Control Point CCCD */
#define BLE_HAS_READ_PRESET_ACTIVE_INDEX_VAL           0x06        /**< Read Preset Active Index */
#define BLE_HAS_READ_PRESET_ACTIVE_INDEX_CCCD          0x07        /**< Read Preset Active Index CCCD */
#define BLE_HAS_WRITE_PRESET_ACTIVE_INDEX_CCCD         0x08        /**< Write Preset Active Index CCCD */
#define BLE_HAS_GATTS_REQ_MAX                          0x09        /**< The maximum number of HAS GATT request type. */


/**
 *  @brief This structure defines HAPS feature detail.
 */
typedef struct
{
    uint8_t hearing_aid_type:2;         /**< Hearing Aid Type */
    uint8_t sync_support:1;             /**< Preset Synchronization Support */
    uint8_t independent:1;              /**< Independent Presets */
    uint8_t dynamic:1;                  /**< Dynamic Presets */
    uint8_t wr_support:1;               /**< Writable Presets Support */
    uint8_t rfu:2;                      /**< Reserved for Future Use */
} ble_ha_features_t;

/**
 *  @brief This structure defines HAPS Preset detail.
 */
typedef struct
{
    uint8_t index;                      /**< Preset index */
    uint8_t property;                   /**< Preset property */
    void *name;                         /**< Preset name */
} ble_ha_preset_t;

/**
 *  @brief This structure defines HAPS characteristic ATT handle.
 */
typedef struct
{
    uint16_t val_hdl;                   /**< HAS characteristic value attribute handle */
    uint16_t cccd_hdl;                  /**< HAS CCCD attribute handle */
} ble_ha_service_charac_hdl_t;

/**
 * @brief                       This function distributes HAS request to corresponded handler from specified remote device.
 * @param[in] req               is the type of HAS request.
 * @param[in] conn_hdl          is the connection handle of the Bluetooth link.
 * @param[in] data              is the data.
 * @param[in] size              is the size of data.
 * @param[in] offset            is the offset.
 * @return                      the size of responded data.
 *
 */
uint32_t ble_haps_gatt_request_handler(uint8_t req, bt_handle_t conn_hdl, uint8_t *data, uint16_t size, uint16_t offset);

/**
 * @brief                       This function is used to initialize HAP Server.
 * @param[in] max_link_num      is the max number of Bluetooth link.
 * @param[in] features          is the feature of HAS.
 * @param[in] charac_hdl_list   is the list of ATT handle of HAS service.
 * @param[in] preset_count      is the count of Preset.
 * @param[in] preset            is the Preset structure.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_OUT_OF_MEMORY, no enough memory to complete this operation.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_haps_init(uint8_t max_link_num, ble_ha_features_t features, ble_ha_service_charac_hdl_t *charac_hdl_list, uint8_t preset_count, ble_ha_preset_t *preset);

#endif
