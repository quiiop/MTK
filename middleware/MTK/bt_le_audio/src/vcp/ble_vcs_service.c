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

#include "ble_vcs.h"

#include "ble_vocs_def.h"

#include "ble_aics_def.h"

#include "bt_gap_le.h"
#include "bt_gatts.h"

/************************************************
*   Attribute handle
*************************************************/
#define BLE_VCS_START_HANDLE                                        (0x1301)

#if 1 //__MTK_VCP_CMMON__
#ifdef MTK_LEA_VOCS_ENABLE
#if defined(AIR_LE_AUDIO_HEADSET_ENABLE) || defined(MTK_LE_AUDIO_TWO_CHANNEL)
#define BLE_VOCS_SECOND_SERVICE_COUNT 2
#else
#define BLE_VOCS_SECOND_SERVICE_COUNT 1
#endif
#else
#define BLE_VOCS_SECOND_SERVICE_COUNT 0
#endif

#ifdef MTK_LEA_AICS_ENABLE
#define BLE_AICS_SECOND_SERVICE_COUNT 1
#else
#define BLE_AICS_SECOND_SERVICE_COUNT 0
#endif

#define BLE_VCS_CHAR_START_HANDLE                  (BLE_VCS_START_HANDLE + BLE_VOCS_SECOND_SERVICE_COUNT + BLE_AICS_SECOND_SERVICE_COUNT + 2)
#define BLE_VCS_VALUE_HANDLE_VOLUME_STATE          (BLE_VCS_CHAR_START_HANDLE)       /**< Attribute Value Handle of Volume State Characteristic. */
#define BLE_VCS_VALUE_HANDLE_VOLUME_CONTROL_POINT  (BLE_VCS_CHAR_START_HANDLE + 3)   /**< Attribute Value Handle of Volume Control Point Characteristic. */
#define BLE_VCS_VALUE_HANDLE_VOLUME_FLAGS          (BLE_VCS_CHAR_START_HANDLE + 5)   /**< Attribute Value Handle of Volume Flags Characteristic. */
#define BLE_VCS_END_HANDLE                         (BLE_VCS_CHAR_START_HANDLE + 6)
#else //Airoha original
#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
#define BLE_VCS_VALUE_HANDLE_VOLUME_STATE                           (0x1306)   /**< Attribute Value Handle of Volume State Characteristic. */
#define BLE_VCS_VALUE_HANDLE_VOLUME_CONTROL_POINT                   (0x1309)   /**< Attribute Value Handle of Volume Control Point Characteristic. */
#define BLE_VCS_VALUE_HANDLE_VOLUME_FLAGS                           (0x130B)   /**< Attribute Value Handle of Volume Flags Characteristic. */
#define BLE_VCS_END_HANDLE                                          (0x130C)
#else
#define BLE_VCS_VALUE_HANDLE_VOLUME_STATE                           (0x1305)   /**< Attribute Value Handle of Volume State Characteristic. */
#define BLE_VCS_VALUE_HANDLE_VOLUME_CONTROL_POINT                   (0x1308)   /**< Attribute Value Handle of Volume Control Point Characteristic. */
#define BLE_VCS_VALUE_HANDLE_VOLUME_FLAGS                           (0x130A)   /**< Attribute Value Handle of Volume Flags Characteristic. */
#define BLE_VCS_END_HANDLE                                          (0x130B)
#endif
#endif /* __MTK_VCP_COMMON__*/
/************************************************
*   UUID
*************************************************/
static const bt_uuid_t BT_SIG_UUID_VOLUME_STATE = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_VOLUME_STATE);
static const bt_uuid_t BT_SIG_UUID_VOLUME_CONTROL_POINT = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_VOLUME_CONTROL_POINT);
static const bt_uuid_t BT_SIG_UUID_VOLUME_FLAGS = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_VOLUME_FLAGS);

/************************************************
*   ATTRIBUTE VALUE HANDLE
*************************************************/
typedef struct {
    ble_vcs_uuid_t uuid_type;           /**< UUID type */
    ble_vcs_gatt_request_t write_request_id;               /**< request id */
    ble_vcs_gatt_request_t read_request_id;                /**< request id */
} ble_vcs_attr_cccd_handler_t;

static ble_vcs_attribute_handle_t g_vcs_att_handle_tbl[] = {
    {BLE_VCS_UUID_TYPE_VOLUME_CONTROL_SERVICE,   BLE_VCS_START_HANDLE},
    {BLE_VCS_UUID_TYPE_VOLUME_STATE,             BLE_VCS_VALUE_HANDLE_VOLUME_STATE},
    {BLE_VCS_UUID_TYPE_VOLUME_CONTROL_POINT,     BLE_VCS_VALUE_HANDLE_VOLUME_CONTROL_POINT},
    {BLE_VCS_UUID_TYPE_VOLUME_FLAGS,             BLE_VCS_VALUE_HANDLE_VOLUME_FLAGS},
    {BLE_VCS_UUID_TYPE_INVALID,                  BLE_VCS_END_HANDLE},
};

static ble_vcs_attr_cccd_handler_t g_vcs_att_cccd_tbl[] = {
    {BLE_VCS_UUID_TYPE_VOLUME_STATE,             BLE_VCS_WRITE_VOLUME_STATE_CCCD,   BLE_VCS_READ_VOLUME_STATE_CCCD},
    {BLE_VCS_UUID_TYPE_VOLUME_FLAGS,             BLE_VCS_WRITE_VOLUME_FLAGS_CCCD,   BLE_VCS_READ_VOLUME_FLAGS_CCCD},
};
/************************************************
*   CALLBACK
*************************************************/
static uint32_t ble_vcs_volume_state_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vcs_volume_state_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vcs_volume_control_point_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vcs_volume_flags_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_vcs_volume_flags_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

/************************************************
*   SERVICE TABLE
*************************************************/
BT_GATTS_NEW_PRIMARY_SERVICE_16(ble_vcs_primary_service, BT_SIG_UUID16_VOLUME_CONTROL_SERVICE);

/* include service: vocs channel_1 */
BT_GATTS_NEW_INCLUDED_SERVICE_16(ble_vocs_secondary_service_included_channel_1,
                                 BLE_VOCS_START_HANDLE_CHANNEL_1,
                                 BLE_VOCS_END_HANDLE_CHANNEL_1,
                                 BT_SIG_UUID16_VOLUME_OFFSET_CONTROL_SERVICE);
#if defined(AIR_LE_AUDIO_HEADSET_ENABLE) || defined(MTK_LE_AUDIO_TWO_CHANNEL)
/* include service: vocs channel_2 */
BT_GATTS_NEW_INCLUDED_SERVICE_16(ble_vocs_secondary_service_included_channel_2,
                                 BLE_VOCS_START_HANDLE_CHANNEL_2,
                                 BLE_VOCS_END_HANDLE_CHANNEL_2,
                                 BT_SIG_UUID16_VOLUME_OFFSET_CONTROL_SERVICE);
#endif

/* include service: aics */
BT_GATTS_NEW_INCLUDED_SERVICE_16(ble_aics_secondary_service_included,
                                 BLE_AICS_START_HANDLE,
                                 BLE_AICS_END_HANDLE,
                                 BT_SIG_UUID16_AUDIO_INPUT_CONTROL_SERVICE);

BT_GATTS_NEW_CHARC_16(ble_vcs_char4_volume_state,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      BLE_VCS_VALUE_HANDLE_VOLUME_STATE,
                      BT_SIG_UUID16_VOLUME_STATE);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vcs_volume_state,
                                  BT_SIG_UUID_VOLUME_STATE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_vcs_volume_state_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_vcs_volume_state_client_config,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_vcs_volume_state_client_config_callback);

BT_GATTS_NEW_CHARC_16(ble_vcs_charc4_control_point,
                      BT_GATT_CHARC_PROP_WRITE,
                      BLE_VCS_VALUE_HANDLE_VOLUME_CONTROL_POINT,
                      BT_SIG_UUID16_VOLUME_CONTROL_POINT);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vcs_control_point,
                                  BT_SIG_UUID_VOLUME_CONTROL_POINT,
                                  BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_vcs_volume_control_point_callback);

BT_GATTS_NEW_CHARC_16(ble_vcs_char4_volume_flags,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      BLE_VCS_VALUE_HANDLE_VOLUME_FLAGS,
                      BT_SIG_UUID16_VOLUME_FLAGS);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_vcs_volume_flags,
                                  BT_SIG_UUID_VOLUME_FLAGS,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_vcs_volume_flags_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_vcs_volume_flags_client_config,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_vcs_volume_flags_client_config_callback);

static const bt_gatts_service_rec_t *ble_vcs_service_rec[] = {
    (const bt_gatts_service_rec_t *) &ble_vcs_primary_service,
#ifdef MTK_LEA_VOCS_ENABLE
    (const bt_gatts_service_rec_t *) &ble_vocs_secondary_service_included_channel_1,
#if defined(AIR_LE_AUDIO_HEADSET_ENABLE) || defined(MTK_LE_AUDIO_TWO_CHANNEL)
    (const bt_gatts_service_rec_t *) &ble_vocs_secondary_service_included_channel_2,
#endif
#endif
#ifdef MTK_LEA_AICS_ENABLE
    (const bt_gatts_service_rec_t *) &ble_aics_secondary_service_included,
#endif
    (const bt_gatts_service_rec_t *) &ble_vcs_char4_volume_state,
    (const bt_gatts_service_rec_t *) &ble_vcs_volume_state,
    (const bt_gatts_service_rec_t *) &ble_vcs_volume_state_client_config,
    (const bt_gatts_service_rec_t *) &ble_vcs_charc4_control_point,
    (const bt_gatts_service_rec_t *) &ble_vcs_control_point,
    (const bt_gatts_service_rec_t *) &ble_vcs_char4_volume_flags,
    (const bt_gatts_service_rec_t *) &ble_vcs_volume_flags,
    (const bt_gatts_service_rec_t *) &ble_vcs_volume_flags_client_config,
};

const bt_gatts_service_t ble_vcs_service = {
    .starting_handle = BLE_VCS_START_HANDLE,    /* 0x1301 */
    .ending_handle = BLE_VCS_END_HANDLE,        /* 0x130B for earbuds 0x130C for headset */
    .required_encryption_key_size = 0,
    .records = ble_vcs_service_rec
};

/************************************************
*   CALLBACK
*************************************************/
static uint32_t ble_vcs_volume_state_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_vcs_gatt_request_handler(BLE_VCS_READ_VOLUME_STATE, handle, data, size);
    }

    return 0;
}

static uint32_t ble_vcs_volume_state_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vcs_gatt_request_handler(BLE_VCS_WRITE_VOLUME_STATE_CCCD, handle, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vcs_gatt_request_handler(BLE_VCS_READ_VOLUME_STATE_CCCD, handle, data, size);
        }
    }
    return 0;
}

static uint32_t ble_vcs_volume_control_point_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    //MTK_Titan: Add BT_GATTS_CALLBACK_WRITE_COMMAND
    if (handle != BT_HANDLE_INVALID && (rw == BT_GATTS_CALLBACK_WRITE || rw == BT_GATTS_CALLBACK_WRITE_COMMAND)) {
       return ble_vcs_gatt_request_handler(BLE_VCS_WRITE_VOLUME_CONTROL_POINT, handle, data, size);
    }

    return 0;

}

static uint32_t ble_vcs_volume_flags_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_vcs_gatt_request_handler(BLE_VCS_READ_VOLUME_FLAGS, handle, data, size);
    }

    return 0;
}

static uint32_t ble_vcs_volume_flags_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_vcs_gatt_request_handler(BLE_VCS_WRITE_VOLUME_FLAGS_CCCD, handle, data, size);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_vcs_gatt_request_handler(BLE_VCS_READ_VOLUME_FLAGS_CCCD, handle, data, size);
        }
    }
    return 0;
}

/************************************************
*   FUNCTIONS
*************************************************/
#define BLE_VCS_DEFAULT_VOLUME   119

ble_vcs_attribute_handle_t *ble_vcs_get_attribute_handle_tbl(void)
{
    return g_vcs_att_handle_tbl;
}

bool ble_vcs_set_cccd_handler(bt_handle_t conn_handle, uint16_t attr_handle, uint16_t value)
{
    if (attr_handle < BLE_VCS_END_HANDLE && attr_handle > BLE_VCS_START_HANDLE) {
        uint8_t i=0,j=0;
        for (j=0; j<(sizeof(g_vcs_att_handle_tbl)/sizeof(ble_vcs_attribute_handle_t)); j++) {
            if (g_vcs_att_handle_tbl[j].att_handle == attr_handle) {
                for (i=0; i<(sizeof(g_vcs_att_cccd_tbl)/sizeof(ble_vcs_attr_cccd_handler_t)); i++) {
                    if (g_vcs_att_cccd_tbl[i].uuid_type == g_vcs_att_handle_tbl[j].uuid_type) {
                        ble_vcs_gatt_request_handler(g_vcs_att_cccd_tbl[i].write_request_id, conn_handle, &value, 2);
                        break;
                    }
                }
            }
        }
        return true;
    }
    return false;
}

bt_le_audio_cccd_record_t* ble_vcs_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num)
{
    bt_le_audio_cccd_record_t *cccd_record = NULL;
    uint8_t i=0, j=0, idx=0;

    if (num == NULL) {
        return NULL;
    }

    *num = sizeof(g_vcs_att_cccd_tbl)/sizeof(ble_vcs_attr_cccd_handler_t);

    if (NULL == (cccd_record = le_audio_malloc(*num * sizeof(bt_le_audio_cccd_record_t)))) {
        return NULL;
    }

    for (i=0; i<(sizeof(g_vcs_att_cccd_tbl)/sizeof(ble_vcs_attr_cccd_handler_t)); i++) {
        ble_vcs_gatt_request_handler(g_vcs_att_cccd_tbl[i].read_request_id, conn_handle, &cccd_record[idx].cccd_value, 2);

        for (j=0; j<(sizeof(g_vcs_att_handle_tbl)/sizeof(ble_vcs_attribute_handle_t)); j++) {
            if (g_vcs_att_handle_tbl[j].uuid_type == g_vcs_att_cccd_tbl[i].uuid_type) {
                cccd_record[idx].attr_handle = g_vcs_att_handle_tbl[j].att_handle;
            }
        }

        idx++;

        if (idx == *num) {
            break;
        }
    }

    return cccd_record;
}

