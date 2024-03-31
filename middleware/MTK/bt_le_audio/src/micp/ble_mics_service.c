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

#include "ble_mics.h"

#include "ble_aics_def.h"

#include "bt_gap_le.h"
#include "bt_gatts.h"

/************************************************
*   Attribute handle
*************************************************/
#define BLE_MICS_START_HANDLE           (0x5001)
#define BLE_MICS_VALUE_HANDLE_MUTE      (0x5004)   /**< Attribute Value Handle of Mute Characteristic. */
#define BLE_MICS_END_HANDLE             (0x5005)

/************************************************
*   UUID
*************************************************/
static const bt_uuid_t BT_SIG_UUID_MUTE = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_MUTE);

/************************************************
*   ATTRIBUTE VALUE HANDLE
*************************************************/
typedef struct {
    ble_mics_uuid_t uuid_type;           /**< UUID type */
    ble_mics_gatt_request_t write_request_id;               /**< request id */
    ble_mics_gatt_request_t read_request_id;                /**< request id */
} ble_mics_attr_cccd_handler_t;

static ble_mics_attribute_handle_t g_mics_att_handle_tbl[] = {
    {BLE_MICS_UUID_TYPE_MICS,                        BLE_MICS_START_HANDLE},
    {BLE_MICS_UUID_TYPE_MUTE,                        BLE_MICS_VALUE_HANDLE_MUTE},
    {BLE_MICS_UUID_TYPE_INVALID,                     BLE_MICS_END_HANDLE}, /* END g_mics_att_handle_tbl */
};

static ble_mics_attr_cccd_handler_t g_mics_att_cccd_tbl[] = {
    {BLE_MICS_UUID_TYPE_MUTE,                        BLE_MICS_GATT_REQUEST_WRITE_MUTE_CCCD, BLE_MICS_GATT_REQUEST_READ_MUTE_CCCD},
};

/************************************************
*   CALLBACK
*************************************************/
static uint32_t ble_mics_mute_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_mics_mute_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

/************************************************
*   SERVICE TABLE
*************************************************/
BT_GATTS_NEW_PRIMARY_SERVICE_16(ble_mics_primary_service, BT_SIG_UUID16_MICS);

/* include service: AICS */
BT_GATTS_NEW_INCLUDED_SERVICE_16(ble_aics_secondary_service_included,
                                 BLE_AICS_START_HANDLE,
                                 BLE_AICS_END_HANDLE,
                                 BT_SIG_UUID16_AUDIO_INPUT_CONTROL_SERVICE);

BT_GATTS_NEW_CHARC_16(ble_mics_char4_mute,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY | BT_GATT_CHARC_PROP_WRITE,
                      BLE_MICS_VALUE_HANDLE_MUTE,
                      BT_SIG_UUID16_MUTE);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_mics_mute,
                                  BT_SIG_UUID_MUTE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_mics_mute_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_mics_mute_client_config,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_mics_mute_client_config_callback);

static const bt_gatts_service_rec_t *ble_mics_service_rec[] = {
    (const bt_gatts_service_rec_t *) &ble_mics_primary_service,
    (const bt_gatts_service_rec_t *) &ble_aics_secondary_service_included,
    (const bt_gatts_service_rec_t *) &ble_mics_char4_mute,
    (const bt_gatts_service_rec_t *) &ble_mics_mute,
    (const bt_gatts_service_rec_t *) &ble_mics_mute_client_config,
};

const bt_gatts_service_t ble_mics_service = {
    .starting_handle = BLE_MICS_START_HANDLE,   /* 0x5001 */
    .ending_handle = BLE_MICS_END_HANDLE,       /* 0x5005 */
    .required_encryption_key_size = 0,
    .records = ble_mics_service_rec
};

/************************************************
*   CALLBACK
*************************************************/
static uint32_t ble_mics_mute_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            ble_mics_gatt_request_handler(BLE_MICS_GATT_REQUEST_WRITE_MUTE, handle, data, size);
            return BT_GATTS_ASYNC_RESPONSE;

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_mics_gatt_request_handler(BLE_MICS_GATT_REQUEST_READ_MUTE, handle, data, size);
        }
    }

    return 0;
}

static uint32_t ble_mics_mute_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //le_audio_log("[MICS] ble_mics_mute_client_config_callback, opcode:%d, size:%d \r\n", 2, rw, size);

    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_mics_gatt_request_handler(BLE_MICS_GATT_REQUEST_WRITE_MUTE_CCCD, handle, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_mics_gatt_request_handler(BLE_MICS_GATT_REQUEST_READ_MUTE_CCCD, handle, data, size);
        }
    }

    return 0;
}

/************************************************
*   Public functions
*************************************************/
ble_mics_attribute_handle_t *ble_mics_get_attribute_handle_tbl(void)
{
    return g_mics_att_handle_tbl;
}

bool ble_mics_set_cccd_handler(bt_handle_t conn_handle, uint16_t attr_handle, uint16_t value)
{
    if (attr_handle < BLE_MICS_END_HANDLE && attr_handle > BLE_MICS_START_HANDLE) {
        uint8_t i=0,j=0;
        for (j=0; j<(sizeof(g_mics_att_handle_tbl)/sizeof(ble_mics_attribute_handle_t)); j++) {
            if (g_mics_att_handle_tbl[j].att_handle == attr_handle) {
                for (i=0; i<(sizeof(g_mics_att_cccd_tbl)/sizeof(ble_mics_attr_cccd_handler_t)); i++) {
                    if (g_mics_att_cccd_tbl[i].uuid_type == g_mics_att_handle_tbl[j].uuid_type) {
                        ble_mics_gatt_request_handler(g_mics_att_cccd_tbl[i].write_request_id, conn_handle, &value, 2);
                        break;
                    }
                }
            }
        }
        return true;
    }
    return false;
}

bt_le_audio_cccd_record_t* ble_mics_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num)
{
    bt_le_audio_cccd_record_t *cccd_record = NULL;
    uint8_t i=0, j=0, idx=0;

    if (num == NULL) {
        return NULL;
    }

    *num = sizeof(g_mics_att_cccd_tbl)/sizeof(ble_mics_attr_cccd_handler_t);

    if (NULL == (cccd_record = le_audio_malloc(*num * sizeof(bt_le_audio_cccd_record_t)))) {
        return NULL;
    }

    for (i=0; i<(sizeof(g_mics_att_cccd_tbl)/sizeof(ble_mics_attr_cccd_handler_t)); i++) {
        ble_mics_gatt_request_handler(g_mics_att_cccd_tbl[i].read_request_id, conn_handle, &cccd_record[idx].cccd_value, 2);

        for (j=0; j<(sizeof(g_mics_att_handle_tbl)/sizeof(ble_mics_attribute_handle_t)); j++) {
            if (g_mics_att_handle_tbl[j].uuid_type == g_mics_att_cccd_tbl[i].uuid_type) {
                cccd_record[idx].attr_handle = g_mics_att_handle_tbl[j].att_handle;
            }
        }

        idx++;

        if (idx == *num) {
            break;
        }
    }

    return cccd_record;
}
