/*
* (C) 2019  MediaTek Inc. All rights reserved.
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
#include "bt_le_audio_util.h"

#include "ble_aics.h"

#include "bt_gap_le.h"
#include "bt_gatts.h"

/**
 * @brief The AICS channel
 */
typedef uint8_t ble_aics_channel_t;
#define BLE_AICS_1                   0x00 /**< Audio channel 1 */
#define BLE_AICS_MAX_CHANNEL_NUM     0x01 /**< The maximum number of AICS channel. */

/************************************************
*   UUID
*************************************************/
static const bt_uuid_t BT_SIG_UUID_INPUT_STATE = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_INPUT_STATE);
static const bt_uuid_t BT_SIG_UUID_GAIN_SETTING_PROPERTIES = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_GAIN_SETTING_PROPERTIES);
static const bt_uuid_t BT_SIG_UUID_INPUT_TYPE = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_INPUT_TYPE);
static const bt_uuid_t BT_SIG_UUID_INPUT_STATUS = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_INPUT_STATUS);
static const bt_uuid_t BT_SIG_UUID_AUDIO_INPUT_CONTROL_POINT = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_AUDIO_INPUT_CONTROL_POINT);
static const bt_uuid_t BT_SIG_UUID_AUDIO_INPUT_DESCRIPTION = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_AUDIO_INPUT_DESCRIPTION);

/************************************************
*   ATTRIBUTE VALUE HANDLE
*************************************************/
typedef struct {
    ble_aics_uuid_t uuid_type;           /**< UUID type */
    ble_aics_gatt_request_t write_request_id;               /**< request id */
    ble_aics_gatt_request_t read_request_id;                /**< request id */
} ble_aics_attr_cccd_handler_t;

/* BLE_AICS_1 */
static ble_aics_attribute_handle_t g_aics_att_handle_tbl[] = {
    {BLE_AICS_UUID_TYPE_AUDIO_INPUT_CONTROL_SERVICE, BLE_AICS_START_HANDLE},
    {BLE_AICS_UUID_TYPE_INPUT_STATE,                 BLE_AICS_VALUE_HANDLE_INPUT_STATE},
    {BLE_AICS_UUID_TYPE_GAIN_SETTING_PROPERTIES,     BLE_AICS_VALUE_HANDLE_GAIN_SETTING_PROPERTIES},
    {BLE_AICS_UUID_TYPE_INPUT_TYPE,                  BLE_AICS_VALUE_HANDLE_INPUT_TYPE},
    {BLE_AICS_UUID_TYPE_INPUT_STATUS,                BLE_AICS_VALUE_HANDLE_INPUT_STATUS},
    {BLE_AICS_UUID_TYPE_AUDIO_INPUT_CONTROL_POINT,   BLE_AICS_VALUE_HANDLE_CONTROL_POINT},
    {BLE_AICS_UUID_TYPE_AUDIO_INPUT_DESCRIPTION,     BLE_AICS_VALUE_HANDLE_AUDIO_INPUT_DESCRIPTION},
    {BLE_AICS_UUID_TYPE_INVALID,                     BLE_AICS_END_HANDLE}, /* END g_aics_att_handle_tbl */
};

static ble_aics_attr_cccd_handler_t g_aics_att_cccd_tbl[] = {
    {BLE_AICS_UUID_TYPE_INPUT_STATE,                 BLE_AICS_WRITE_INPUT_STATE_CCCD,               BLE_AICS_READ_INPUT_STATE_CCCD},
    {BLE_AICS_UUID_TYPE_INPUT_STATUS,                BLE_AICS_WRITE_INPUT_STATUS_CCCD,              BLE_AICS_READ_INPUT_STATUS_CCCD},
    {BLE_AICS_UUID_TYPE_AUDIO_INPUT_DESCRIPTION,     BLE_AICS_WRITE_AUDIO_INPUT_DESCRIPTION_CCCD,   BLE_AICS_READ_AUDIO_INPUT_DESCRIPTION_CCCD},
};

/************************************************
*   CALLBACK
*************************************************/
/* BLE_AICS_1 */
static uint32_t ble_aics_input_state_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_aics_input_state_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_aics_gain_setting_properties_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_aics_input_type_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_aics_input_status_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_aics_input_status_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_aics_audio_input_control_point_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_aics_audio_input_description_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_aics_audio_input_description_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

/************************************************
*   SERVICE TABLE
*************************************************/
/* BLE_AICS_1 */
BT_GATTS_NEW_SECONDARY_SERVICE_16(ble_aics_secondary_service, BT_SIG_UUID16_AUDIO_INPUT_CONTROL_SERVICE);

BT_GATTS_NEW_CHARC_16(ble_aics_char4_input_state,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      BLE_AICS_VALUE_HANDLE_INPUT_STATE,
                      BT_SIG_UUID16_INPUT_STATE);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_aics_input_state,
                                  BT_SIG_UUID_INPUT_STATE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_aics_input_state_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_aics_input_state_client_config,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_aics_input_state_client_config_callback);

BT_GATTS_NEW_CHARC_16(ble_aics_charc4_gain_setting_properties,
                      BT_GATT_CHARC_PROP_READ,
                      BLE_AICS_VALUE_HANDLE_GAIN_SETTING_PROPERTIES,
                      BT_SIG_UUID16_GAIN_SETTING_PROPERTIES);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_aics_gain_setting_properties,
                                  BT_SIG_UUID_GAIN_SETTING_PROPERTIES,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_aics_gain_setting_properties_callback);

BT_GATTS_NEW_CHARC_16(ble_aics_charc4_input_type,
                      BT_GATT_CHARC_PROP_READ,
                      BLE_AICS_VALUE_HANDLE_INPUT_TYPE,
                      BT_SIG_UUID16_INPUT_TYPE);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_aics_input_type,
                                  BT_SIG_UUID_INPUT_TYPE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_aics_input_type_callback);

BT_GATTS_NEW_CHARC_16(ble_aics_char4_input_status,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      BLE_AICS_VALUE_HANDLE_INPUT_STATUS,
                      BT_SIG_UUID16_INPUT_STATUS);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_aics_input_status,
                                  BT_SIG_UUID_INPUT_STATUS,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_aics_input_status_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_aics_input_status_client_config,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_aics_input_status_client_config_callback);

BT_GATTS_NEW_CHARC_16(ble_aics_charc4_audio_input_control_point,
                      BT_GATT_CHARC_PROP_WRITE,
                      BLE_AICS_VALUE_HANDLE_CONTROL_POINT,
                      BT_SIG_UUID16_AUDIO_INPUT_CONTROL_POINT);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_aics_audio_input_control_point,
                                  BT_SIG_UUID_AUDIO_INPUT_CONTROL_POINT,
                                  BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_aics_audio_input_control_point_callback);

BT_GATTS_NEW_CHARC_16(ble_aics_char4_audio_input_description,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY | BT_GATT_CHARC_PROP_WRITE_WITHOUT_RSP,
                      BLE_AICS_VALUE_HANDLE_AUDIO_INPUT_DESCRIPTION,
                      BT_SIG_UUID16_AUDIO_INPUT_DESCRIPTION);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_aics_audio_input_description,
                                  BT_SIG_UUID_AUDIO_INPUT_DESCRIPTION,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_aics_audio_input_description_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_aics_audio_input_description_client_config,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_aics_audio_input_description_client_config_callback);

static const bt_gatts_service_rec_t *ble_aics_service_rec[] = {
    (const bt_gatts_service_rec_t *) &ble_aics_secondary_service,
    (const bt_gatts_service_rec_t *) &ble_aics_char4_input_state,
    (const bt_gatts_service_rec_t *) &ble_aics_input_state,
    (const bt_gatts_service_rec_t *) &ble_aics_input_state_client_config,
    (const bt_gatts_service_rec_t *) &ble_aics_charc4_gain_setting_properties,
    (const bt_gatts_service_rec_t *) &ble_aics_gain_setting_properties,
    (const bt_gatts_service_rec_t *) &ble_aics_charc4_input_type,
    (const bt_gatts_service_rec_t *) &ble_aics_input_type,
    (const bt_gatts_service_rec_t *) &ble_aics_char4_input_status,
    (const bt_gatts_service_rec_t *) &ble_aics_input_status,
    (const bt_gatts_service_rec_t *) &ble_aics_input_status_client_config,
    (const bt_gatts_service_rec_t *) &ble_aics_charc4_audio_input_control_point,
    (const bt_gatts_service_rec_t *) &ble_aics_audio_input_control_point,
    (const bt_gatts_service_rec_t *) &ble_aics_char4_audio_input_description,
    (const bt_gatts_service_rec_t *) &ble_aics_audio_input_description,
    (const bt_gatts_service_rec_t *) &ble_aics_audio_input_description_client_config,
};

const bt_gatts_service_t ble_aics_service = {
    .starting_handle = BLE_AICS_START_HANDLE,   /* 0x4001 */
    .ending_handle = BLE_AICS_END_HANDLE,       /* 0x4010 */
    .required_encryption_key_size = 0,
    .records = ble_aics_service_rec
};

/************************************************
*   CALLBACK
*************************************************/
/* BLE_AICS_1 */
static uint32_t ble_aics_input_state_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_aics_gatt_request_handler(BLE_AICS_READ_INPUT_STATE, handle, BLE_AICS_1, data, size);
    }

    return 0;
}

static uint32_t ble_aics_input_state_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_aics_gatt_request_handler(BLE_AICS_WRITE_INPUT_STATE_CCCD, handle, BLE_AICS_1, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_aics_gatt_request_handler(BLE_AICS_READ_INPUT_STATE_CCCD, handle, BLE_AICS_1, data, size);
        }
    }
    return 0;
}

static uint32_t ble_aics_gain_setting_properties_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_aics_gatt_request_handler(BLE_AICS_READ_GAIN_SETTING_PROPERTIES, handle, BLE_AICS_1, data, size);
    }

    return 0;
}

static uint32_t ble_aics_input_type_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_aics_gatt_request_handler(BLE_AICS_READ_INPUT_TYPE, handle, BLE_AICS_1, data, size);
    }

    return 0;
}

static uint32_t ble_aics_input_status_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_aics_gatt_request_handler(BLE_AICS_READ_INPUT_STATUS, handle, BLE_AICS_1, data, size);
    }

    return 0;
}

static uint32_t ble_aics_input_status_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //le_audio_log("[AICS] ble_aics_input_status_client_config_callback, opcode:%d, size:%d \r\n", 2, rw, size);

    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_aics_gatt_request_handler(BLE_AICS_WRITE_INPUT_STATUS_CCCD, handle, BLE_AICS_1, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_aics_gatt_request_handler(BLE_AICS_READ_INPUT_STATUS_CCCD, handle, BLE_AICS_1, data, size);
        }
    }
    return 0;
}

static uint32_t ble_aics_audio_input_control_point_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //MTK_Titan: Add BT_GATTS_CALLBACK_WRITE_COMMAND
    if (handle != BT_HANDLE_INVALID && (rw == BT_GATTS_CALLBACK_WRITE || rw == BT_GATTS_CALLBACK_WRITE_COMMAND)) {
        ble_aics_gatt_request_handler(BLE_AICS_WRITE_AUDIO_INPUT_CONTROL_POINT, handle, BLE_AICS_1, data, size);

        return BT_GATTS_ASYNC_RESPONSE;
    }

    return 0;
}

static uint32_t ble_aics_audio_input_description_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_aics_gatt_request_handler(BLE_AICS_WRITE_AUDIO_INPUT_DESCRIPTION, handle, BLE_AICS_1, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_aics_gatt_request_handler(BLE_AICS_READ_AUDIO_INPUT_DESCRIPTION, handle, BLE_AICS_1, data, size);
        }
    }

    return 0;
}

static uint32_t ble_aics_audio_input_description_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //le_audio_log("[AICS] ble_aics_input_status_client_config_callback, opcode:%d, size:%d \r\n", 2, rw, size);

    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_aics_gatt_request_handler(BLE_AICS_WRITE_AUDIO_INPUT_DESCRIPTION_CCCD, handle, BLE_AICS_1, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_aics_gatt_request_handler(BLE_AICS_READ_AUDIO_INPUT_DESCRIPTION_CCCD, handle, BLE_AICS_1, data, size);
        }
    }

    return 0;
}

/************************************************
*   Functions
*************************************************/
/* INPUT_DESCRIPTION */
/* BLE_AICS_1 */
#define INPUT_DESCRIPTION_LENGTH    10
static uint8_t input_description[INPUT_DESCRIPTION_LENGTH] = {0x6d, 0x69, 0x63, 0x72, 0x6f, 0x70, 0x68, 0x6f, 0x6e, 0x65}; /* microphone */

ble_aics_attribute_handle_t *ble_aics_get_attribute_handle_tbl(uint8_t channel)
{
    switch (channel) {
        case BLE_AICS_1: {
            return g_aics_att_handle_tbl;
        }
        default:
            break;
    }
    return NULL;
}

uint8_t ble_aics_get_channel_number(void)
{
    return BLE_AICS_MAX_CHANNEL_NUM;
}

void ble_aics_init_parameter(void)
{
    /* BLE_AICS_1 */
    ble_aics_set_audio_input_description_by_channel(BLE_AICS_1, input_description, INPUT_DESCRIPTION_LENGTH);
}

bool ble_aics_set_cccd_handler(bt_handle_t conn_handle, uint16_t attr_handle, uint16_t value)
{
    if (attr_handle < BLE_AICS_END_HANDLE && attr_handle > BLE_AICS_START_HANDLE) {
        uint8_t i=0,j=0;
        for (j=0; j<(sizeof(g_aics_att_handle_tbl)/sizeof(ble_aics_attribute_handle_t)); j++) {
            if (g_aics_att_handle_tbl[j].att_handle == attr_handle) {
                for (i=0; i<(sizeof(g_aics_att_cccd_tbl)/sizeof(ble_aics_attr_cccd_handler_t)); i++) {
                    if (g_aics_att_cccd_tbl[i].uuid_type == g_aics_att_handle_tbl[j].uuid_type) {
                        ble_aics_gatt_request_handler(g_aics_att_cccd_tbl[i].write_request_id, conn_handle, BLE_AICS_1, &value, 2);
                        break;
                    }
                }
            }
        }
        return true;
    }
    return false;
}

bt_le_audio_cccd_record_t* ble_aics_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num)
{
    bt_le_audio_cccd_record_t *cccd_record = NULL;
    uint8_t i=0, j=0, idx=0;

    if (num == NULL) {
        return NULL;
    }

    *num = sizeof(g_aics_att_cccd_tbl)/sizeof(ble_aics_attr_cccd_handler_t)*BLE_AICS_MAX_CHANNEL_NUM;

    if (NULL == (cccd_record = le_audio_malloc(*num * sizeof(bt_le_audio_cccd_record_t)))) {
        return NULL;
    }

    for (i=0; i<(sizeof(g_aics_att_cccd_tbl)/sizeof(ble_aics_attr_cccd_handler_t)); i++) {
        ble_aics_gatt_request_handler(g_aics_att_cccd_tbl[i].read_request_id, conn_handle, BLE_AICS_1, &cccd_record[idx].cccd_value, 2);

        for (j=0; j<(sizeof(g_aics_att_handle_tbl)/sizeof(ble_aics_attribute_handle_t)); j++) {
            if (g_aics_att_handle_tbl[j].uuid_type == g_aics_att_cccd_tbl[i].uuid_type) {
                cccd_record[idx].attr_handle = g_aics_att_handle_tbl[j].att_handle;
            }
        }

        idx++;

        if (idx == *num) {
            break;
        }
    }

    return cccd_record;
}

