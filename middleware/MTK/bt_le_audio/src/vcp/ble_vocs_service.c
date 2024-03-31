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
#include "bt_sink_srv_ami.h"

#include "bt_le_audio_util.h"

#include "ble_vocs.h"
#include "ble_vocs_def.h"

#include "bt_gap_le.h"
#include "bt_gatts.h"

/************************************************
* @brief The VOCS channel.
*************************************************/

#if defined(AIR_LE_AUDIO_HEADSET_ENABLE) || defined(MTK_LE_AUDIO_TWO_CHANNEL)

typedef uint8_t ble_vocs_channel_t;
#define BLE_VOCS_CHANNEL_1           0x00 /**< VOCS channel 1. */
#define BLE_VOCS_CHANNEL_2           0x01 /**< VOCS channel 2. */
#define BLE_VOCS_MAX_CHANNEL_NUM     0x02 /**< The maximum number of VOCS channel. */

#else /*#ifdef AIR_LE_AUDIO_HEADSET_ENABLE*/

typedef uint8_t ble_vocs_channel_t;
#define BLE_VOCS_CHANNEL_1           0x00 /**< VOCS channel 1. */
#define BLE_VOCS_MAX_CHANNEL_NUM     0x01/**< The maximum number of VOCS channel. */

#endif /*#ifdef AIR_LE_AUDIO_HEADSET_ENABLE*/

/************************************************
*   UUID
*************************************************/
static const bt_uuid_t BT_SIG_UUID_OFFSET_STATE = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_OFFSET_STATE);
static const bt_uuid_t BT_SIG_UUID_AUDIO_LOCATION = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_AUDIO_LOCATION);
static const bt_uuid_t BT_SIG_UUID_VOLUME_OFFSET_CONTROL_POINT = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_VOLUME_OFFSET_CONTROL_POINT);
static const bt_uuid_t BT_SIG_UUID_AUDIO_OUTPUT_DESCRIPTION = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_AUDIO_OUTPUT_DESCRIPTION);

/* BLE_VOCS_CHANNEL_1 */
/************************************************
*   ATTRIBUTE VALUE HANDLE
*************************************************/
typedef struct {
    ble_vocs_uuid_t uuid_type;           /**< UUID type */
    ble_vocs_gatt_request_t write_request_id;                /**< request id */
    ble_vocs_gatt_request_t read_request_id;                /**< request id */
} ble_vocs_attr_cccd_handler_t;

static ble_vocs_attribute_handle_t g_vocs_att_handle_tbl_1[] = {
    {BLE_VOCS_UUID_TYPE_VOLUME_OFFSET_CONTROL_SERVICE,   BLE_VOCS_START_HANDLE_CHANNEL_1},
    {BLE_VOCS_UUID_TYPE_OFFSET_STATE,                    BLE_VOCS_VALUE_HANDLE_OFFSET_STATE_CHANNEL_1},
    {BLE_VOCS_UUID_TYPE_AUDIO_LOCATION,                  BLE_VOCS_VALUE_HANDLE_AUDIO_LOCATION_CHANNEL_1},
    {BLE_VOCS_UUID_TYPE_VOLUME_OFFSET_CONTROL_POINT,     BLE_VOCS_VALUE_HANDLE_CONTROL_POINT_CHANNEL_1},
    {BLE_VOCS_UUID_TYPE_AUDIO_OUTPUT_DESCRIPTION,        BLE_VOCS_VALUE_HANDLE_AUDIO_OUTPUT_DESCRIPTION_CHANNEL_1},
    {BLE_VOCS_UUID_TYPE_INVALID,                         BLE_VOCS_END_HANDLE_CHANNEL_1},
};

static ble_vocs_attr_cccd_handler_t g_vocs_att_cccd_tbl[] = {
    {BLE_VOCS_UUID_TYPE_OFFSET_STATE,                    BLE_VOCS_WRITE_VOLUME_OFFSET_STATE_CCCD,       BLE_VOCS_READ_VOLUME_OFFSET_STATE_CCCD},
    {BLE_VOCS_UUID_TYPE_AUDIO_LOCATION,                  BLE_VOCS_WRITE_AUDIO_LOCATION_CCCD,            BLE_VOCS_READ_AUDIO_LOCATION_CCCD},
    {BLE_VOCS_UUID_TYPE_AUDIO_OUTPUT_DESCRIPTION,        BLE_VOCS_WRITE_AUDIO_OUTPUT_DESCRIPTION_CCCD,  BLE_VOCS_READ_AUDIO_OUTPUT_DESCRIPTION_CCCD},
};

/************************************************
*   CALLBACK
*************************************************/
static uint32_t ble_vocs_offset_state_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_offset_state_client_config_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_audio_location_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_audio_location_client_config_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_volume_offset_control_point_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_audio_output_description_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_audio_output_description_client_config_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

/************************************************
*   SERVICE TABLE
*************************************************/
/* BLE_VOCS_CHANNEL_1 */
BT_GATTS_NEW_SECONDARY_SERVICE_16(ble_vocs_secondary_service_channel_1, BT_SIG_UUID16_VOLUME_OFFSET_CONTROL_SERVICE);

BT_GATTS_NEW_CHARC_16(ble_vocs_char4_offset_state_channel_1,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      BLE_VOCS_VALUE_HANDLE_OFFSET_STATE_CHANNEL_1,
                      BT_SIG_UUID16_OFFSET_STATE);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vocs_offset_state_channel_1,
                                  BT_SIG_UUID_OFFSET_STATE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_vocs_offset_state_callback_channel_1);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_vocs_offset_state_client_config_channel_1,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_vocs_offset_state_client_config_callback_channel_1);

BT_GATTS_NEW_CHARC_16(ble_vocs_char4_audio_location_channel_1,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY | BT_GATT_CHARC_PROP_WRITE_WITHOUT_RSP,
                      BLE_VOCS_VALUE_HANDLE_AUDIO_LOCATION_CHANNEL_1,
                      BT_SIG_UUID16_AUDIO_LOCATION);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vocs_audio_location_channel_1,
                                  BT_SIG_UUID_AUDIO_LOCATION,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_vocs_audio_location_callback_channel_1);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_vocs_audio_location_client_config_channel_1,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_vocs_audio_location_client_config_callback_channel_1);

BT_GATTS_NEW_CHARC_16(ble_vocs_charc4_volume_offset_control_point_channel_1,
                      BT_GATT_CHARC_PROP_WRITE,
                      BLE_VOCS_VALUE_HANDLE_CONTROL_POINT_CHANNEL_1,
                      BT_SIG_UUID16_VOLUME_OFFSET_CONTROL_POINT);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vocs_volume_offset_control_point_channel_1,
                                  BT_SIG_UUID_VOLUME_OFFSET_CONTROL_POINT,
                                  BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_vocs_volume_offset_control_point_callback_channel_1);

BT_GATTS_NEW_CHARC_16(ble_vocs_char4_audio_output_description_channel_1,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY | BT_GATT_CHARC_PROP_WRITE_WITHOUT_RSP,
                      BLE_VOCS_VALUE_HANDLE_AUDIO_OUTPUT_DESCRIPTION_CHANNEL_1,
                      BT_SIG_UUID16_AUDIO_OUTPUT_DESCRIPTION);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vocs_audio_output_description_channel_1,
                                  BT_SIG_UUID_AUDIO_OUTPUT_DESCRIPTION,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_vocs_audio_output_description_callback_channel_1);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_vocs_audio_output_description_client_config_channel_1,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_vocs_audio_output_description_client_config_callback_channel_1);

static const bt_gatts_service_rec_t *ble_vocs_service_rec_channel_1[] = {
    (const bt_gatts_service_rec_t *) &ble_vocs_secondary_service_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_char4_offset_state_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_offset_state_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_offset_state_client_config_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_char4_audio_location_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_audio_location_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_audio_location_client_config_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_charc4_volume_offset_control_point_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_volume_offset_control_point_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_char4_audio_output_description_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_audio_output_description_channel_1,
    (const bt_gatts_service_rec_t *) &ble_vocs_audio_output_description_client_config_channel_1,
};

const bt_gatts_service_t ble_vocs_service_channel_1 = {
    .starting_handle = BLE_VOCS_START_HANDLE_CHANNEL_1, /* 0x2001 */
    .ending_handle = BLE_VOCS_END_HANDLE_CHANNEL_1,     /* 0x200C */
    .required_encryption_key_size = 0,
    .records = ble_vocs_service_rec_channel_1
};

#if defined(AIR_LE_AUDIO_HEADSET_ENABLE) || defined(MTK_LE_AUDIO_TWO_CHANNEL)
/* BLE_VOCS_CHANNEL_2 */
/************************************************
*   ATTRIBUTE VALUE HANDLE
*************************************************/
static ble_vocs_attribute_handle_t g_vocs_att_handle_tbl_2[] = {
    {BLE_VOCS_UUID_TYPE_VOLUME_OFFSET_CONTROL_SERVICE,   BLE_VOCS_START_HANDLE_CHANNEL_2},
    {BLE_VOCS_UUID_TYPE_OFFSET_STATE,                    BLE_VOCS_VALUE_HANDLE_OFFSET_STATE_CHANNEL_2},
    {BLE_VOCS_UUID_TYPE_AUDIO_LOCATION,                  BLE_VOCS_VALUE_HANDLE_AUDIO_LOCATION_CHANNEL_2},
    {BLE_VOCS_UUID_TYPE_VOLUME_OFFSET_CONTROL_POINT,     BLE_VOCS_VALUE_HANDLE_CONTROL_POINT_CHANNEL_2},
    {BLE_VOCS_UUID_TYPE_AUDIO_OUTPUT_DESCRIPTION,        BLE_VOCS_VALUE_HANDLE_AUDIO_OUTPUT_DESCRIPTION_CHANNEL_2},
    {BLE_VOCS_UUID_TYPE_INVALID,                         BLE_VOCS_END_HANDLE_CHANNEL_2},
};

/************************************************
*   CALLBACK
*************************************************/
static uint32_t ble_vocs_offset_state_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_offset_state_client_config_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_audio_location_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_audio_location_client_config_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_volume_offset_control_point_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_audio_output_description_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vocs_audio_output_description_client_config_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

/************************************************
*   SERVICE TABLE
*************************************************/
BT_GATTS_NEW_SECONDARY_SERVICE_16(ble_vocs_secondary_service_channel_2, BT_SIG_UUID16_VOLUME_OFFSET_CONTROL_SERVICE);

BT_GATTS_NEW_CHARC_16(ble_vocs_char4_offset_state_channel_2,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      BLE_VOCS_VALUE_HANDLE_OFFSET_STATE_CHANNEL_2,
                      BT_SIG_UUID16_OFFSET_STATE);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vocs_offset_state_channel_2,
                                  BT_SIG_UUID_OFFSET_STATE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_vocs_offset_state_callback_channel_2);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_vocs_offset_state_client_config_channel_2,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_vocs_offset_state_client_config_callback_channel_2);

BT_GATTS_NEW_CHARC_16(ble_vocs_char4_audio_location_channel_2,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY | BT_GATT_CHARC_PROP_WRITE_WITHOUT_RSP,
                      BLE_VOCS_VALUE_HANDLE_AUDIO_LOCATION_CHANNEL_2,
                      BT_SIG_UUID16_AUDIO_LOCATION);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vocs_audio_location_channel_2,
                                  BT_SIG_UUID_AUDIO_LOCATION,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_vocs_audio_location_callback_channel_2);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_vocs_audio_location_client_config_channel_2,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_vocs_audio_location_client_config_callback_channel_2);

BT_GATTS_NEW_CHARC_16(ble_vocs_charc4_volume_offset_control_point_channel_2,
                      BT_GATT_CHARC_PROP_WRITE,
                      BLE_VOCS_VALUE_HANDLE_CONTROL_POINT_CHANNEL_2,
                      BT_SIG_UUID16_VOLUME_OFFSET_CONTROL_POINT);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vocs_volume_offset_control_point_channel_2,
                                  BT_SIG_UUID_VOLUME_OFFSET_CONTROL_POINT,
                                  BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_vocs_volume_offset_control_point_callback_channel_2);

BT_GATTS_NEW_CHARC_16(ble_vocs_char4_audio_output_description_channel_2,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY | BT_GATT_CHARC_PROP_WRITE_WITHOUT_RSP,
                      BLE_VOCS_VALUE_HANDLE_AUDIO_OUTPUT_DESCRIPTION_CHANNEL_2,
                      BT_SIG_UUID16_AUDIO_OUTPUT_DESCRIPTION);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vocs_audio_output_description_channel_2,
                                  BT_SIG_UUID_AUDIO_OUTPUT_DESCRIPTION,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_vocs_audio_output_description_callback_channel_2);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_vocs_audio_output_description_client_config_channel_2,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_vocs_audio_output_description_client_config_callback_channel_2);

static const bt_gatts_service_rec_t *ble_vocs_service_rec_channel_2[] = {
    (const bt_gatts_service_rec_t *) &ble_vocs_secondary_service_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_char4_offset_state_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_offset_state_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_offset_state_client_config_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_char4_audio_location_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_audio_location_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_audio_location_client_config_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_charc4_volume_offset_control_point_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_volume_offset_control_point_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_char4_audio_output_description_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_audio_output_description_channel_2,
    (const bt_gatts_service_rec_t *) &ble_vocs_audio_output_description_client_config_channel_2,
};

const bt_gatts_service_t ble_vocs_service_channel_2 = {
    .starting_handle = BLE_VOCS_START_HANDLE_CHANNEL_2, /* 0x3001 */
    .ending_handle = BLE_VOCS_END_HANDLE_CHANNEL_2,     /* 0x300C */
    .required_encryption_key_size = 0,
    .records = ble_vocs_service_rec_channel_2
};
#endif
/************************************************
*   CALLBACK
*************************************************/
/* BLE_VOCS_CHANNEL_1 */
static uint32_t ble_vocs_offset_state_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_vocs_gatt_request_handler(BLE_VOCS_READ_VOLUME_OFFSET_STATE, handle, BLE_VOCS_CHANNEL_1, data, size);
    }

    return 0;
}

static uint32_t ble_vocs_offset_state_client_config_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //le_audio_log("[VOCS] ble_vocs_input_state_client_config_callback, opcode:%d, size:%d \r\n", 2, rw, size);

    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_VOLUME_OFFSET_STATE_CCCD, handle, BLE_VOCS_CHANNEL_1, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_READ_VOLUME_OFFSET_STATE_CCCD, handle, BLE_VOCS_CHANNEL_1, data, size);
        }
    }

    return 0;
}

static uint32_t ble_vocs_audio_location_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_AUDIO_LOCATION, handle, BLE_VOCS_CHANNEL_1, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_READ_AUDIO_LOCATION, handle, BLE_VOCS_CHANNEL_1, data, size);
        }
    }

    return 0;
}

static uint32_t ble_vocs_audio_location_client_config_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //le_audio_log("[VOCS] ble_vocs_input_status_client_config_callback, opcode:%d, size:%d \r\n", 2, rw, size);

    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_AUDIO_LOCATION_CCCD, handle, BLE_VOCS_CHANNEL_1, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_READ_AUDIO_LOCATION_CCCD, handle, BLE_VOCS_CHANNEL_1, data, size);
        }
    }

    return 0;
}

static uint32_t ble_vocs_volume_offset_control_point_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_WRITE) {
        ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_VOLUME_OFFSET_CONTROL_POINT, handle, BLE_VOCS_CHANNEL_1, data, size);

        return BT_GATTS_ASYNC_RESPONSE;
    }

    return 0;
}

/* BLE_VOCS_CHANNEL_2 */
static uint32_t ble_vocs_audio_output_description_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_AUDIO_OUTPUT_DESCRIPTION, handle, BLE_VOCS_CHANNEL_1, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_READ_AUDIO_OUTPUT_DESCRIPTION, handle, BLE_VOCS_CHANNEL_1, data, size);
        }
    }

    return 0;
}

static uint32_t ble_vocs_audio_output_description_client_config_callback_channel_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //le_audio_log("[VOCS] ble_vocs_input_status_client_config_callback, opcode:%d, size:%d \r\n", 2, rw, size);

    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_AUDIO_OUTPUT_DESCRIPTION_CCCD, handle, BLE_VOCS_CHANNEL_1, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_READ_AUDIO_OUTPUT_DESCRIPTION_CCCD, handle, BLE_VOCS_CHANNEL_1, data, size);
        }
    }

    return 0;
}

#if defined(AIR_LE_AUDIO_HEADSET_ENABLE) || defined(MTK_LE_AUDIO_TWO_CHANNEL)
/* BLE_VOCS_CHANNEL_2 */
static uint32_t ble_vocs_offset_state_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_vocs_gatt_request_handler(BLE_VOCS_READ_VOLUME_OFFSET_STATE, handle, BLE_VOCS_CHANNEL_2, data, size);
    }

    return 0;
}

static uint32_t ble_vocs_offset_state_client_config_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //le_audio_log("[VOCS] ble_vocs_input_state_client_config_callback, opcode:%d, size:%d \r\n", 2, rw, size);

    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_VOLUME_OFFSET_STATE_CCCD, handle, BLE_VOCS_CHANNEL_2, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_READ_VOLUME_OFFSET_STATE_CCCD, handle, BLE_VOCS_CHANNEL_2, data, size);
        }
    }

    return 0;
}

static uint32_t ble_vocs_audio_location_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_AUDIO_LOCATION, handle, BLE_VOCS_CHANNEL_2, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_READ_AUDIO_LOCATION, handle, BLE_VOCS_CHANNEL_2, data, size);
        }
    }

    return 0;
}

static uint32_t ble_vocs_audio_location_client_config_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //le_audio_log("[VOCS] ble_vocs_input_status_client_config_callback, opcode:%d, size:%d \r\n", 2, rw, size);

    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_AUDIO_LOCATION_CCCD, handle, BLE_VOCS_CHANNEL_2, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_READ_AUDIO_LOCATION_CCCD, handle, BLE_VOCS_CHANNEL_2, data, size);
        }
    }

    return 0;
}

static uint32_t ble_vocs_volume_offset_control_point_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_WRITE) {
        ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_VOLUME_OFFSET_CONTROL_POINT, handle, BLE_VOCS_CHANNEL_2, data, size);

        return BT_GATTS_ASYNC_RESPONSE;
    }

    return 0;
}

static uint32_t ble_vocs_audio_output_description_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_AUDIO_OUTPUT_DESCRIPTION, handle, BLE_VOCS_CHANNEL_2, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_READ_AUDIO_OUTPUT_DESCRIPTION, handle, BLE_VOCS_CHANNEL_2, data, size);

        }
    }

    return 0;
}

static uint32_t ble_vocs_audio_output_description_client_config_callback_channel_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //le_audio_log("[VOCS] ble_vocs_input_status_client_config_callback, opcode:%d, size:%d \r\n", 2, rw, size);

    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_WRITE_AUDIO_OUTPUT_DESCRIPTION_CCCD, handle, BLE_VOCS_CHANNEL_2, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vocs_gatt_request_handler(BLE_VOCS_READ_AUDIO_OUTPUT_DESCRIPTION_CCCD, handle, BLE_VOCS_CHANNEL_2, data, size);
        }
    }

    return 0;
}
#endif
/************************************************
*   Public functions
*************************************************/
/* Audio output description: left */
#define VOCS_AUDIO_OUTPUT_DESC_SIZE_LEFT    4
const uint8_t vocs_audio_output_desc_left[VOCS_AUDIO_OUTPUT_DESC_SIZE_LEFT] = {0x6c, 0x65, 0x66, 0x74};     /* left */

/* Audio output description: right */
#define VOCS_AUDIO_OUTPUT_DESC_SIZE_RIGHT   5
const uint8_t vocs_audio_output_desc_right[VOCS_AUDIO_OUTPUT_DESC_SIZE_RIGHT] = {0x72, 0x69, 0x67, 0x68, 0x74}; /* right */

ble_vocs_attribute_handle_t *ble_vocs_get_attribute_handle_tbl(uint8_t channel)
{
    switch (channel) {
        case BLE_VOCS_CHANNEL_1: {
            return g_vocs_att_handle_tbl_1;
        }
#if defined(AIR_LE_AUDIO_HEADSET_ENABLE) || defined(MTK_LE_AUDIO_TWO_CHANNEL)
        case BLE_VOCS_CHANNEL_2: {
            return g_vocs_att_handle_tbl_2;
        }
#endif
        default:
            break;
    }
    return NULL;
}

uint8_t ble_vocs_get_channel_number(void)
{
    return BLE_VOCS_MAX_CHANNEL_NUM;
}

void ble_vocs_init_parameter(void)
{
#if defined(AIR_LE_AUDIO_HEADSET_ENABLE) || defined(MTK_LE_AUDIO_TWO_CHANNEL)
    /* BLE_VOCS_CHANNEL_1 */
    ble_vocs_set_audio_output_description_by_channel(BLE_VOCS_CHANNEL_1, (uint8_t *)vocs_audio_output_desc_left, VOCS_AUDIO_OUTPUT_DESC_SIZE_LEFT);
    /* BLE_VOCS_CHANNEL_2 */
    ble_vocs_set_audio_output_description_by_channel(BLE_VOCS_CHANNEL_2, (uint8_t *)vocs_audio_output_desc_right, VOCS_AUDIO_OUTPUT_DESC_SIZE_RIGHT);
#else
    audio_channel_t channel = ami_get_audio_channel();

    if (channel == AUDIO_CHANNEL_R) {
        ble_vocs_set_audio_output_description_by_channel(BLE_VOCS_CHANNEL_1, (uint8_t *)vocs_audio_output_desc_right, VOCS_AUDIO_OUTPUT_DESC_SIZE_RIGHT);
    } else {
        ble_vocs_set_audio_output_description_by_channel(BLE_VOCS_CHANNEL_1, (uint8_t *)vocs_audio_output_desc_left, VOCS_AUDIO_OUTPUT_DESC_SIZE_LEFT);
    }
#endif
}

bool ble_vocs_set_cccd_handler(bt_handle_t conn_handle, uint16_t attr_handle, uint16_t value)
{
    if (attr_handle < BLE_VOCS_END_HANDLE_CHANNEL_1 && attr_handle > BLE_VOCS_START_HANDLE_CHANNEL_1) {
        uint8_t i=0,j=0;
        for (j=0; j<(sizeof(g_vocs_att_handle_tbl_1)/sizeof(ble_vocs_attribute_handle_t)); j++) {
            if (g_vocs_att_handle_tbl_1[j].att_handle == attr_handle) {
                for (i=0; i<(sizeof(g_vocs_att_cccd_tbl)/sizeof(ble_vocs_attr_cccd_handler_t)); i++) {
                    if (g_vocs_att_cccd_tbl[i].uuid_type == g_vocs_att_handle_tbl_1[j].uuid_type) {
                        ble_vocs_gatt_request_handler(g_vocs_att_cccd_tbl[i].write_request_id, conn_handle, BLE_VOCS_CHANNEL_1, &value, 2);
                        break;
                    }
                }
            }
        }
        return true;
    }
#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
    if (attr_handle < BLE_VOCS_END_HANDLE_CHANNEL_2 && attr_handle > BLE_VOCS_START_HANDLE_CHANNEL_2) {
        uint8_t i=0,j=0;
        for (j=0; j<(sizeof(g_vocs_att_handle_tbl_2)/sizeof(ble_vocs_attribute_handle_t)); j++) {
            if (g_vocs_att_handle_tbl_2[j].att_handle == attr_handle) {
                for (i=0; i<(sizeof(g_vocs_att_cccd_tbl)/sizeof(ble_vocs_attr_cccd_handler_t)); i++) {
                    if (g_vocs_att_cccd_tbl[i].uuid_type == g_vocs_att_handle_tbl_2[j].uuid_type) {
                        ble_vocs_gatt_request_handler(g_vocs_att_cccd_tbl[i].write_request_id, conn_handle, BLE_VOCS_CHANNEL_2, &value, 2);
                        break;
                    }
                }
            }
        }
        return true;
    }
#endif
    return false;
}

bt_le_audio_cccd_record_t* ble_vocs_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num)
{
    bt_le_audio_cccd_record_t *cccd_record = NULL;
    uint8_t i=0, j=0, idx=0;

    if (num == NULL) {
        return NULL;
    }

    *num = sizeof(g_vocs_att_cccd_tbl)/sizeof(ble_vocs_attr_cccd_handler_t) * BLE_VOCS_MAX_CHANNEL_NUM;

    if (NULL == (cccd_record = le_audio_malloc(*num * sizeof(bt_le_audio_cccd_record_t)))) {
        return NULL;
    }

    for (i=0; i<(sizeof(g_vocs_att_cccd_tbl)/sizeof(ble_vocs_attr_cccd_handler_t)); i++) {
        ble_vocs_gatt_request_handler(g_vocs_att_cccd_tbl[i].read_request_id, conn_handle, BLE_VOCS_CHANNEL_1, &cccd_record[idx].cccd_value, 2);

        for (j=0; j<(sizeof(g_vocs_att_handle_tbl_1)/sizeof(ble_vocs_attribute_handle_t)); j++) {
            if (g_vocs_att_handle_tbl_1[j].uuid_type == g_vocs_att_cccd_tbl[i].uuid_type) {
                cccd_record[idx].attr_handle = g_vocs_att_handle_tbl_1[j].att_handle;
            }
        }

        idx++;

        if (idx == *num) {
            break;
        }
    }

#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
    for (i=0; i<(sizeof(g_vocs_att_cccd_tbl)/sizeof(ble_vocs_attr_cccd_handler_t)); i++) {
        ble_vocs_gatt_request_handler(g_vocs_att_cccd_tbl[i].read_request_id, conn_handle, BLE_VOCS_CHANNEL_2, &cccd_record[idx].cccd_value, 2);

        for (j=0; j<(sizeof(g_vocs_att_handle_tbl_2)/sizeof(ble_vocs_attribute_handle_t)); j++) {
            if (g_vocs_att_handle_tbl_2[j].uuid_type == g_vocs_att_cccd_tbl[i].uuid_type) {
                cccd_record[idx].attr_handle = g_vocs_att_handle_tbl_2[j].att_handle;
            }
        }

        idx++;

        if (idx == *num) {
            break;
        }
    }
#endif

    return cccd_record;
}

