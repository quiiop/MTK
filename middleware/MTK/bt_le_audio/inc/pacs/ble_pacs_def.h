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

#ifndef __BLE_PACS_DEF_H__
#define __BLE_PACS_DEF_H__

#include "bt_le_audio_def.h"

#ifndef PACKED
#define PACKED  __attribute__((packed))
#endif

/**
 * @brief The PACS service UUID.
 */
#define BT_GATT_UUID16_PACS_SERVICE             (0x1850)            /**< Published Audio Capabilities Service UUID. */

/**
* @brief PACS characteristic UUID.
*/
#define BT_SIG_UUID16_SINK_PAC                  (0x2BC9)            /**< Sink PAC Characteristic UUID. */
#define BT_SIG_UUID16_SINK_LOCATION             (0x2BCA)            /**< Sink Location Characteristic UUID. */
#define BT_SIG_UUID16_SOURCE_PAC                (0x2BCB)            /**< Source PAC Characteristic UUID. */
#define BT_SIG_UUID16_SOURCE_LOCATION           (0x2BCC)            /**< Source Location Characteristic UUID. */
#define BT_SIG_UUID16_AVAILABLE_AUDIO_CONTEXTS  (0x2BCD)            /**< Available Audio Contexts Characteristic UUID. */
#define BT_SIG_UUID16_SUPPORTED_AUDIO_CONTEXTS  (0x2BCE)            /**< Supported Audio Contexts Characteristic UUID. */

/**
 * @brief The PACS UUID type definitions.
 */
#define BLE_PACS_UUID_TYPE_PACS_SERVICE                  0           /**< PACS UUID type.*/
#define BLE_PACS_UUID_TYPE_SINK_PAC                      1           /**< Sink PAC UUID type.*/
#define BLE_PACS_UUID_TYPE_SINK_LOCATION                 2           /**< Sink Location UUID type.*/
#define BLE_PACS_UUID_TYPE_SOURCE_PAC                    3           /**< Source PAC UUID type.*/
#define BLE_PACS_UUID_TYPE_SOURCE_LOCATION               4           /**< Source Location UUID type.*/
#define BLE_PACS_UUID_TYPE_AVAILABLE_AUDIO_CONTEXTS      5           /**< Available Audio Contexts UUID type.*/
#define BLE_PACS_UUID_TYPE_SUPPORTED_AUDIO_CONTEXTS      6           /**< Supported Audio Contexts UUID type.*/
#define BLE_PACS_UUID_TYPE_MAX_NUM                       7           /**< The max number of PACS UUID type.*/
#define BLE_PACS_UUID_TYPE_INVALID                       0xFF        /**< The invalid PACS UUID type.*/
typedef uint8_t ble_pacs_uuid_t;                                     /**< UUID type.*/


#define BLE_PACS_UUID_TYPE_CHARC_START       BLE_PACS_UUID_TYPE_SINK_PAC

/**
 * @brief The PACS max number of characteristics.
 */
#define BLE_PACS_MAX_CHARC_NUMBER    (BLE_PACS_UUID_TYPE_MAX_NUM-1)   /**< The number of PACS characteristics.*/

/**
 * @brief The PACS GATT type definitions.
 */
#define BLE_PACS_READ_SINK_PAC                          0x00    /**< Read Sink PAC. */
#define BLE_PACS_READ_SINK_PAC_CCCD                     0x01    /**< Read Sink PAC CCCD. */
#define BLE_PACS_WRITE_SINK_PAC_CCCD                    0x02    /**< Write Sink PAC CCCD. */
#define BLE_PACS_READ_SINK_LOCATION                     0x03    /**< Read Sink Location. */
#define BLE_PACS_READ_SINK_LOCATION_CCCD                0x04    /**< Read Sink Location CCCD. */
#define BLE_PACS_WRITE_SINK_LOCATION_CCCD               0x05    /**< Write Sink Location CCCD. */
#define BLE_PACS_READ_SOURCE_PAC                        0x06    /**< Read Source PAC. */
#define BLE_PACS_READ_SOURCE_PAC_CCCD                   0x07    /**< Read Source PAC CCCD. */
#define BLE_PACS_WRITE_SOURCE_PAC_CCCD                  0x08    /**< Write Source PAC CCCD. */
#define BLE_PACS_READ_SOURCE_LOCATION                   0x09    /**< Read Source Location. */
#define BLE_PACS_READ_SOURCE_LOCATION_CCCD              0x0A    /**< Read Source Location CCCD. */
#define BLE_PACS_WRITE_SOURCE_LOCATION_CCCD             0x0B    /**< Write Source Location CCCD. */
#define BLE_PACS_READ_AVAILABLE_AUDIO_CONTEXTS          0x0C    /**< Read Available Audio Contexts. */
#define BLE_PACS_READ_AVAILABLE_AUDIO_CONTEXTS_CCCD     0x0D    /**< Read Available Audio Contexts CCCD. */
#define BLE_PACS_WRITE_AVAILABLE_AUDIO_CONTEXTS_CCCD    0x0E    /**< Write Available Audio Contexts CCCD. */
#define BLE_PACS_READ_SUPPORTED_AUDIO_CONTEXTS          0x0F    /**< Read Supported Audio Contexts. */
#define BLE_PACS_READ_SUPPORTED_AUDIO_CONTEXTS_CCCD     0x10    /**< Read Supported Audio Contexts CCCD. */
#define BLE_PACS_WRITE_SUPPORTED_AUDIO_CONTEXTS_CCCD    0x11    /**< Write Supported Audio Contexts CCCD. */
#define BLE_PACS_GATTS_REQ_MAX                          0x12    /**< The maximum number of PACS GATT type. */
typedef uint8_t ble_pacs_gatt_request_t;

/**
 *  @brief This structure defines the parameters in Available Audio Contexts characteristic.
 */
typedef struct {
    bt_le_audio_content_type_t sink;                                  /**< Bitmask of audio data Context Type values available for reception. */
    bt_le_audio_content_type_t source;                                /**< Bitmask of audio data Context Type values available for transmission. */
} PACKED ble_pacs_available_audio_contexts_t;

/**
 *  @brief This structure defines the parameters in Supported Audio Contexts characteristic.
 */
typedef struct {
    bt_le_audio_content_type_t sink;                                  /**< Bitmask of audio data Context Type values supported for reception. */
    bt_le_audio_content_type_t source;                                /**< Bitmask of audio data Context Type values supported for reception. */
} PACKED ble_pacs_supported_audio_contexts_t;

/**
 *  @brief This structure defines PACS attribute handle detail.
 */
typedef struct {
    ble_pacs_uuid_t uuid_type;  /**< UUID type */
    uint16_t att_handle;        /**< Attribute handle */
} ble_pacs_attribute_handle_t;


/**
 * @brief                       This function distributes PACS request to corresponded handler from specified remote device.
 * @param[in] req               is the type of PACS request.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] charc_idx         is the characteristic index.
 * @param[in] data              is the data.
 * @param[in] size              is the size of data.
 * @param[in] offset            is the offset.
 * @return                      the size of responded data.
 */
uint32_t ble_pacs_gatt_request_handler(ble_pacs_gatt_request_t req, bt_handle_t handle, uint8_t charc_idx, void *data, uint16_t size, uint16_t offset);

#endif  /* __BLE_PACS_DEF_H__ */


