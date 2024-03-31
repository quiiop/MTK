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


#include "bt_le_audio_util.h"
#include "bt_gap_le.h"
#include "bt_gatts.h"
#include "ble_has_def.h"
#include "ble_haps.h"


/************************************************
*   UUID
*************************************************/
static const bt_uuid_t BT_SIG_UUID_HAS_FEATURES                 = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_HAS_FEATURES);
static const bt_uuid_t BT_SIG_UUID_HAS_PRESET_CONTROL_POINT     = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_HAS_PRESET_CONTROL_POINT);
static const bt_uuid_t BT_SIG_UUID_HAS_ACTIVE_PRESET_INDEX      = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_HAS_ACTIVE_PRESET_INDEX);

/************************************************
*   CALLBACK
*************************************************/
static uint32_t ble_has_features_value_callback(const uint8_t rw, bt_handle_t conn_hdl, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_has_control_point_value_callback(const uint8_t rw, bt_handle_t conn_hdl, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_has_control_point_client_config_callback(const uint8_t rw, bt_handle_t conn_hdl, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_has_active_index_value_callback(const uint8_t rw, bt_handle_t conn_hdl, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_has_active_index_client_config_callback(const uint8_t rw, bt_handle_t conn_hdl, void *data, uint16_t size, uint16_t offset);

/************************************************
* Prototype declaration
*************************************************/
extern bool ble_haps_set_cccd_with_att_handle(bt_handle_t conn_hdl, uint16_t att_hdl, uint16_t value);
extern bt_le_audio_cccd_record_t* ble_haps_get_cccd_with_att_handle(bt_handle_t conn_hdl, uint32_t *num);

/************************************************
*   SERVICE TABLE
*************************************************/
BT_GATTS_NEW_PRIMARY_SERVICE_16(ble_has_primary_service, BT_GATT_UUID16_HAS_SERVICE);
BT_GATTS_NEW_CHARC_16(ble_has_char4_features, BT_GATT_CHARC_PROP_READ, HAS_HANDLE_FEATURES_VALUE, BT_SIG_UUID16_HAS_FEATURES);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_has_features_value, BT_SIG_UUID_HAS_FEATURES, BT_GATTS_REC_PERM_READABLE_ENCRYPTION, ble_has_features_value_callback);
BT_GATTS_NEW_CHARC_16(ble_has_char4_preset_control_point, BT_GATT_CHARC_PROP_WRITE | BT_GATT_CHARC_PROP_INDICATE, HAS_HANDLE_PRESET_CONTROL_POINT_VALUE, BT_SIG_UUID16_HAS_PRESET_CONTROL_POINT);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_has_preset_control_point_value, BT_SIG_UUID_HAS_PRESET_CONTROL_POINT, BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION, ble_has_control_point_value_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_has_preset_control_point_cccd, BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION, ble_has_control_point_client_config_callback);
BT_GATTS_NEW_CHARC_16(ble_has_char4_preset_active_index, BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY, HAS_HANDLE_ACTIVE_PRESET_INDEX_VALUE, BT_SIG_UUID16_HAS_ACTIVE_PRESET_INDEX);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_has_preset_active_index_value, BT_SIG_UUID_HAS_ACTIVE_PRESET_INDEX, BT_GATTS_REC_PERM_READABLE_ENCRYPTION, ble_has_active_index_value_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_has_preset_active_index_cccd, BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION, ble_has_active_index_client_config_callback);


static const bt_gatts_service_rec_t *ble_has_service_rec[] =
{
    (const bt_gatts_service_rec_t*)&ble_has_primary_service,
    (const bt_gatts_service_rec_t*)&ble_has_char4_features,
    (const bt_gatts_service_rec_t*)&ble_has_features_value,
    (const bt_gatts_service_rec_t*)&ble_has_char4_preset_control_point,
    (const bt_gatts_service_rec_t*)&ble_has_preset_control_point_value,
    (const bt_gatts_service_rec_t*)&ble_has_preset_control_point_cccd,
    (const bt_gatts_service_rec_t*)&ble_has_char4_preset_active_index,
    (const bt_gatts_service_rec_t*)&ble_has_preset_active_index_value,
    (const bt_gatts_service_rec_t*)&ble_has_preset_active_index_cccd,
};

const bt_gatts_service_t ble_has_service =
{
    .starting_handle = HAS_HANDLE_START,
    .ending_handle = HAS_HANDLE_END,
    .required_encryption_key_size = 7,
    .records = ble_has_service_rec
};

/************************************************
*   CALLBACK
*************************************************/
static uint32_t ble_has_features_value_callback(const uint8_t rw, bt_handle_t conn_hdl, void *data, uint16_t size, uint16_t offset)
{
    if (conn_hdl != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ)
    {
        return ble_haps_gatt_request_handler(BLE_HAS_READ_FEATURES_VAL, conn_hdl, data, size, offset);
    }

    return 0;
}

static uint32_t ble_has_control_point_value_callback(const uint8_t rw, bt_handle_t conn_hdl, void *data, uint16_t size, uint16_t offset)
{
    if (conn_hdl != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_WRITE)
    {
        return ble_haps_gatt_request_handler(BLE_HAS_WRITE_PRESET_CONTROL_POINT_VAL, conn_hdl, data, size, offset);
    }

    return 0;
}

static uint32_t ble_has_control_point_client_config_callback(const uint8_t rw, bt_handle_t conn_hdl, void *data, uint16_t size, uint16_t offset)
{
    if (conn_hdl != BT_HANDLE_INVALID)
    {
        if (rw == BT_GATTS_CALLBACK_WRITE)
        {
            return ble_haps_gatt_request_handler(BLE_HAS_WRITE_PRESET_CONTROL_POINT_CCCD, conn_hdl, data, size, offset);
        }
        else if (rw == BT_GATTS_CALLBACK_READ)
        {
            return ble_haps_gatt_request_handler(BLE_HAS_READ_PRESET_CONTROL_POINT_CCCD, conn_hdl, data, size, offset);
        }
    }

    return 0;
}

static uint32_t ble_has_active_index_value_callback(const uint8_t rw, bt_handle_t conn_hdl, void *data, uint16_t size, uint16_t offset)
{
    if (conn_hdl == BT_HANDLE_INVALID) return 0;

    if (rw == BT_GATTS_CALLBACK_READ)
    {
        return ble_haps_gatt_request_handler(BLE_HAS_READ_PRESET_ACTIVE_INDEX_VAL, conn_hdl, data, size, offset);
    }

    return 0;
}

static uint32_t ble_has_active_index_client_config_callback(const uint8_t rw, bt_handle_t conn_hdl, void *data, uint16_t size, uint16_t offset)
{
    if (conn_hdl != BT_HANDLE_INVALID)
    {
        if (rw == BT_GATTS_CALLBACK_WRITE)
        {
            return ble_haps_gatt_request_handler(BLE_HAS_WRITE_PRESET_ACTIVE_INDEX_CCCD, conn_hdl, data, size, offset);
        }
        else if (rw == BT_GATTS_CALLBACK_READ)
        {
            return ble_haps_gatt_request_handler(BLE_HAS_READ_PRESET_ACTIVE_INDEX_CCCD, conn_hdl, data, size, offset);
        }
    }

    return 0;
}

bool ble_haps_set_cccd_handler(bt_handle_t conn_handle, uint16_t attr_handle, uint16_t value)
{
    if (attr_handle <= HAS_HANDLE_END && attr_handle > HAS_HANDLE_START)
    {
        ble_haps_set_cccd_with_att_handle(conn_handle, attr_handle, value);

        return true;
    }

    return false;
}

bt_le_audio_cccd_record_t* ble_haps_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num)
{
    if (num == NULL) return NULL;

    return ble_haps_get_cccd_with_att_handle(conn_handle, num);
}

bt_status_t ble_haps_init_server(uint8_t max_link_num)
{
    ble_ha_features_t features = {HEARING_AID_TYPE_BINAURAL, 0, 1, 1, 1, 0};
    ble_ha_service_charac_hdl_t charac_hdl[HAS_HDL_TYPE_MAX_NUM] =
    {
        {HAS_HANDLE_FEATURES_VALUE, 0},
        {HAS_HANDLE_PRESET_CONTROL_POINT_VALUE, HAS_HANDLE_PRESET_CONTROL_POINT_CCCD},
        {HAS_HANDLE_ACTIVE_PRESET_INDEX_VALUE, HAS_HANDLE_ACTIVE_PRESET_INDEX_CCCD}
    };
    ble_ha_preset_t preset[] =
    {
        {1, HAS_PRESET_WRITABLE | HAS_PRESET_AVAILABLE, "Universal"},
        {3, HAS_PRESET_WRITABLE | HAS_PRESET_AVAILABLE, "Noisy environment"},
        {4, HAS_PRESET_WRITABLE | HAS_PRESET_AVAILABLE, "Outdoor"},
        {6, 0, "indoor"},
        {7, HAS_PRESET_WRITABLE | HAS_PRESET_AVAILABLE, "Reverberant room"}
    };

    return ble_haps_init(max_link_num, features, charac_hdl, sizeof(preset) / sizeof(ble_ha_preset_t), preset);
}

