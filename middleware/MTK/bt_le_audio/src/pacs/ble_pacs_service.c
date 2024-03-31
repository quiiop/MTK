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

#include "ble_pacs.h"

#include "bt_sink_srv_ami.h"

#include "bt_gap_le.h"
#include "bt_gatts.h"

#if defined(MTK_LE_AUDIO_TWO_CHANNEL) || defined(AIR_LE_AUDIO_HEADSET_ENABLE)
#define MTK_PACS_AUDIO_LOCATION_EN
//#define MTK_PACS_STEREO_EN // Not yet support
#endif

#ifdef MTK_PACS_AUDIO_LOCATION_EN
#define PACS_AUDIO_LOCATION_CHARC_SHIFT 3
#else
#define PACS_AUDIO_LOCATION_CHARC_SHIFT 0
#endif

/************************************************
*   ATTRIBUTE  HANDLE
*************************************************/
#if 1 //__MTK_PACS_CMMON__
#define PACS_START_HANDLE                               (0x1200)                                                              /**< PACS service start handle.*/
/* BLE_PACS_SINK_PAC_1 */
#define PACS_VALUE_HANDLE_SINK_PAC_1                    (0x1202)                                                              /**< Sink PAC_1 characteristic handle.*/
#define PACS_VALUE_HANDLE_SINK_LOCATION                 (PACS_VALUE_HANDLE_SINK_PAC_1 + 3)                                    /**< Sink Location characteristic handle.*/
#ifdef MTK_LE_AUDIO_CT
/* BLE_PACS_SOURCE_PAC_1 */
#define PACS_VALUE_HANDLE_SOURCE_PAC_1                  (PACS_VALUE_HANDLE_SINK_LOCATION + PACS_AUDIO_LOCATION_CHARC_SHIFT)   /**< Source PAC_1 characteristic handle.*/
#define PACS_VALUE_HANDLE_SOURCE_LOCATION               (PACS_VALUE_HANDLE_SOURCE_PAC_1 + 3)                                  /**< Source Location characteristic handle.*/
#define PACS_VALUE_HANDLE_AVAILABILIE_AUDIO_CONTEXTS    (PACS_VALUE_HANDLE_SOURCE_LOCATION + PACS_AUDIO_LOCATION_CHARC_SHIFT) /**< Available Audio Contexts characteristic handle.*/
#else
#define PACS_VALUE_HANDLE_AVAILABILIE_AUDIO_CONTEXTS    (PACS_VALUE_HANDLE_SINK_LOCATION + PACS_AUDIO_LOCATION_CHARC_SHIFT)   /**< Available Audio Contexts characteristic handle.*/
#endif
#define PACS_VALUE_HANDLE_SUPPORTED_AUDIO_CONTEXTS      (PACS_VALUE_HANDLE_AVAILABILIE_AUDIO_CONTEXTS + 3)                    /**< Supported Audio Contexts characteristic handle.*/
#define PACS_END_HANDLE                                 (PACS_VALUE_HANDLE_SUPPORTED_AUDIO_CONTEXTS + 1)                      /**< PACS service end handle.*/
#else
#define PACS_START_HANDLE                               (0x1200)        /**< PACS service start handle.*/
/* BLE_PACS_SINK_PAC_1 */
#define PACS_VALUE_HANDLE_SINK_PAC_1                    (0x1202)        /**< Sink PAC_1 characteristic handle.*/
#define PACS_VALUE_HANDLE_SINK_LOCATION                 (0x1205)        /**< Sink Location characteristic handle.*/
/* BLE_PACS_SOURCE_PAC_1 */
#define PACS_VALUE_HANDLE_SOURCE_PAC_1                  (0x1208)        /**< Source PAC_1 characteristic handle.*/
#define PACS_VALUE_HANDLE_SOURCE_LOCATION               (0x120B)        /**< Source Location characteristic handle.*/
#define PACS_VALUE_HANDLE_AVAILABILIE_AUDIO_CONTEXTS    (0x120E)        /**< Available Audio Contexts characteristic handle.*/
#define PACS_VALUE_HANDLE_SUPPORTED_AUDIO_CONTEXTS      (0x1211)        /**< Supported Audio Contexts characteristic handle.*/
#define PACS_END_HANDLE                                 (0x1212)        /**< PACS service end handle.*/
#endif //__MTK_PACS_CMMON__

/************************************************
*   CHARC INDEX
*************************************************/
typedef enum {
    BLE_PACS_SINK_PAC_1,
    BLE_PACS_SINK_PAC_MAX_NUM,
} ble_pacs_sink_pac_t;

typedef enum {
#ifdef MTK_LE_AUDIO_CT
    BLE_PACS_SOURCE_PAC_1,
#endif
    BLE_PACS_SOURCE_PAC_MAX_NUM,
} ble_pacs_source_pac_t;

#define BLE_PACS_CHARC_NORMAL  0

/************************************************
*   UUID
*************************************************/
static const bt_uuid_t BT_SIG_UUID_SINK_PAC                  = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_SINK_PAC);
#ifdef MTK_PACS_AUDIO_LOCATION_EN
static const bt_uuid_t BT_SIG_UUID_SINK_LOCATION             = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_SINK_LOCATION);
#endif
static const bt_uuid_t BT_SIG_UUID_SOURCE_PAC                = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_SOURCE_PAC);
static const bt_uuid_t BT_SIG_UUID_SOURCE_LOCATION           = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_SOURCE_LOCATION);
static const bt_uuid_t BT_SIG_UUID_AVAILABLE_AUDIO_CONTEXTS  = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_AVAILABLE_AUDIO_CONTEXTS);
static const bt_uuid_t BT_SIG_UUID_SUPPORTED_AUDIO_CONTEXTS  = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_SUPPORTED_AUDIO_CONTEXTS);

/************************************************
*   ATTRIBUTE VALUE HANDLE
*************************************************/
static ble_pacs_attribute_handle_t g_pacs_att_handle_tbl[] = {
    {BLE_PACS_UUID_TYPE_PACS_SERVICE,                PACS_START_HANDLE},

    /* BLE_PACS_SINK_PAC_1 */
    {BLE_PACS_UUID_TYPE_SINK_PAC,                    PACS_VALUE_HANDLE_SINK_PAC_1},

#ifdef MTK_PACS_AUDIO_LOCATION_EN
    /* Sink Audio Locations */
    {BLE_PACS_UUID_TYPE_SINK_LOCATION,               PACS_VALUE_HANDLE_SINK_LOCATION},
#endif

#ifdef MTK_LE_AUDIO_CT
    /* BLE_PACS_SOURCE_PAC_1 */
    {BLE_PACS_UUID_TYPE_SOURCE_PAC,                  PACS_VALUE_HANDLE_SOURCE_PAC_1},

#ifdef MTK_PACS_AUDIO_LOCATION_EN
    /* Source Audio Locations */
    {BLE_PACS_UUID_TYPE_SOURCE_LOCATION,             PACS_VALUE_HANDLE_SOURCE_LOCATION},
#endif /* MTK_PACS_AUDIO_LOCATION_EN */
#endif /* MTK_LE_AUDIO_CT */

    /* Available Audio Contexts */
    {BLE_PACS_UUID_TYPE_AVAILABLE_AUDIO_CONTEXTS,    PACS_VALUE_HANDLE_AVAILABILIE_AUDIO_CONTEXTS},

    /* Supported Audio Contexts */
    {BLE_PACS_UUID_TYPE_SUPPORTED_AUDIO_CONTEXTS,    PACS_VALUE_HANDLE_SUPPORTED_AUDIO_CONTEXTS},

    {BLE_PACS_UUID_TYPE_INVALID,                     PACS_END_HANDLE}, /* END g_pacs_att_handle_tbl */
};

/************************************************
*   CALLBACK
*************************************************/
/* BLE_PACS_SINK_PAC_1 */
static uint32_t ble_pacs_sink_pac_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_pacs_sink_pac_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

#ifdef MTK_PACS_AUDIO_LOCATION_EN
/* Sink Audio Locations */
static uint32_t ble_pacs_sink_location_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_pacs_sink_location_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
#endif

#ifdef MTK_LE_AUDIO_CT
/* BLE_PACS_SOURCE_PAC_1 */
static uint32_t ble_pacs_source_pac_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_pacs_source_pac_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

#ifdef MTK_PACS_AUDIO_LOCATION_EN
/* Source Audio Locations */
static uint32_t ble_pacs_source_location_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_pacs_source_location_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
#endif /* MTK_PACS_AUDIO_LOCATION_EN */
#endif /* MTK_LE_AUDIO_CT */

/* Available Audio Contexts */
static uint32_t ble_pacs_available_audio_contexts_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_pacs_available_audio_contexts_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

/* Supported Audio Contexts */
static uint32_t ble_pacs_supported_audio_contexts_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_pacs_supported_audio_contexts_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

/************************************************
*   SERVICE TABLE
*************************************************/
/* PACS Service */
BT_GATTS_NEW_PRIMARY_SERVICE_16(ble_pacs_primary_service, BT_GATT_UUID16_PACS_SERVICE);

/* BLE_PACS_SINK_PAC_1 */
BT_GATTS_NEW_CHARC_16(ble_pacs_char4_sink_pac,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      PACS_VALUE_HANDLE_SINK_PAC_1,
                      BT_SIG_UUID16_SINK_PAC);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_pacs_sink_pac_value,
                                  BT_SIG_UUID_SINK_PAC,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_pacs_sink_pac_value_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_pacs_sink_pac_config,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_pacs_sink_pac_client_config_callback);

#ifdef MTK_PACS_AUDIO_LOCATION_EN
/* Sink Audio Locations */
BT_GATTS_NEW_CHARC_16(ble_pacs_char4_sink_location,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      PACS_VALUE_HANDLE_SINK_LOCATION,
                      BT_SIG_UUID16_SINK_LOCATION);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_pacs_sink_location_value,
                                  BT_SIG_UUID_SINK_LOCATION,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_pacs_sink_location_value_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_pacs_sink_location_config,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_pacs_sink_location_client_config_callback);

#endif
#ifdef MTK_LE_AUDIO_CT
/* BLE_PACS_SOURCE_PAC_1 */
BT_GATTS_NEW_CHARC_16(ble_pacs_char4_source_pac,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      PACS_VALUE_HANDLE_SOURCE_PAC_1,
                      BT_SIG_UUID16_SOURCE_PAC);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_pacs_source_pac_value,
                                  BT_SIG_UUID_SOURCE_PAC,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_pacs_source_pac_value_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_pacs_source_pac_config,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_pacs_source_pac_client_config_callback);

#ifdef MTK_PACS_AUDIO_LOCATION_EN
/* Source Audio Locations */
BT_GATTS_NEW_CHARC_16(ble_pacs_char4_source_location,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      PACS_VALUE_HANDLE_SOURCE_LOCATION,
                      BT_SIG_UUID16_SOURCE_LOCATION);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_pacs_source_location_value,
                                  BT_SIG_UUID_SOURCE_LOCATION,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_pacs_source_location_value_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_pacs_source_location_config,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_pacs_source_location_client_config_callback);

#endif /* MTK_PACS_AUDIO_LOCATION_EN */
#endif /* MTK_LE_AUDIO_CT */
/* Available Audio Contexts */
BT_GATTS_NEW_CHARC_16(ble_pacs_char4_available_audio_contexts,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      PACS_VALUE_HANDLE_AVAILABILIE_AUDIO_CONTEXTS,
                      BT_SIG_UUID16_AVAILABLE_AUDIO_CONTEXTS);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_pacs_available_audio_contexts_value,
                                  BT_SIG_UUID_AVAILABLE_AUDIO_CONTEXTS,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_pacs_available_audio_contexts_value_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_pacs_available_audio_contexts_config,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_pacs_available_audio_contexts_client_config_callback);

/* Supported Audio Contexts */
BT_GATTS_NEW_CHARC_16(ble_pacs_char4_supported_audio_contexts,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      PACS_VALUE_HANDLE_SUPPORTED_AUDIO_CONTEXTS,
                      BT_SIG_UUID16_SUPPORTED_AUDIO_CONTEXTS);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_pacs_supported_audio_contexts_value,
                                  BT_SIG_UUID_SUPPORTED_AUDIO_CONTEXTS,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION,
                                  ble_pacs_supported_audio_contexts_value_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_pacs_supported_audio_contexts_config,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_pacs_supported_audio_contexts_client_config_callback);


static const bt_gatts_service_rec_t *ble_pacs_service_rec[] = {
    (const bt_gatts_service_rec_t *) &ble_pacs_primary_service,
    /* BLE_PACS_SINK_PAC_1 */
    (const bt_gatts_service_rec_t *) &ble_pacs_char4_sink_pac,
    (const bt_gatts_service_rec_t *) &ble_pacs_sink_pac_value,
    (const bt_gatts_service_rec_t *) &ble_pacs_sink_pac_config,
#ifdef MTK_PACS_AUDIO_LOCATION_EN
    (const bt_gatts_service_rec_t *) &ble_pacs_char4_sink_location,
    (const bt_gatts_service_rec_t *) &ble_pacs_sink_location_value,
    (const bt_gatts_service_rec_t *) &ble_pacs_sink_location_config,
#endif
#ifdef MTK_LE_AUDIO_CT
    /* BLE_PACS_SOURCE_PAC_1 */
    (const bt_gatts_service_rec_t *) &ble_pacs_char4_source_pac,
    (const bt_gatts_service_rec_t *) &ble_pacs_source_pac_value,
    (const bt_gatts_service_rec_t *) &ble_pacs_source_pac_config,
#ifdef MTK_PACS_AUDIO_LOCATION_EN
    (const bt_gatts_service_rec_t *) &ble_pacs_char4_source_location,
    (const bt_gatts_service_rec_t *) &ble_pacs_source_location_value,
    (const bt_gatts_service_rec_t *) &ble_pacs_source_location_config,
#endif /* MTK_PACS_AUDIO_LOCATION_EN */
#endif /* MTK_LE_AUDIO_CT */
    (const bt_gatts_service_rec_t *) &ble_pacs_char4_available_audio_contexts,
    (const bt_gatts_service_rec_t *) &ble_pacs_available_audio_contexts_value,
    (const bt_gatts_service_rec_t *) &ble_pacs_available_audio_contexts_config,
    (const bt_gatts_service_rec_t *) &ble_pacs_char4_supported_audio_contexts,
    (const bt_gatts_service_rec_t *) &ble_pacs_supported_audio_contexts_value,
    (const bt_gatts_service_rec_t *) &ble_pacs_supported_audio_contexts_config,
};

const bt_gatts_service_t ble_pacs_service = {
    .starting_handle = PACS_START_HANDLE,
    .ending_handle = PACS_END_HANDLE,
    .required_encryption_key_size = 7,
    .records = ble_pacs_service_rec
};

/************************************************
*   CALLBACK
*************************************************/
/* BLE_PACS_SINK_PAC_1 */
static uint32_t ble_pacs_sink_pac_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_pacs_gatt_request_handler(BLE_PACS_READ_SINK_PAC, handle, BLE_PACS_SINK_PAC_1, data, size, offset);
    }

    return 0;
}

static uint32_t ble_pacs_sink_pac_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_pacs_gatt_request_handler(BLE_PACS_WRITE_SINK_PAC_CCCD, handle, BLE_PACS_SINK_PAC_1, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_pacs_gatt_request_handler(BLE_PACS_READ_SINK_PAC_CCCD, handle, BLE_PACS_SINK_PAC_1, data, size, offset);
        }
    }
    return 0;
}

#ifdef MTK_PACS_AUDIO_LOCATION_EN
/* Sink Audio Locations */
static uint32_t ble_pacs_sink_location_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_pacs_gatt_request_handler(BLE_PACS_READ_SINK_LOCATION, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);
    }

    return 0;
}

static uint32_t ble_pacs_sink_location_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_pacs_gatt_request_handler(BLE_PACS_WRITE_SINK_LOCATION_CCCD, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_pacs_gatt_request_handler(BLE_PACS_READ_SINK_LOCATION_CCCD, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);
        }
    }
    return 0;
}
#endif

#ifdef MTK_LE_AUDIO_CT
/* BLE_PACS_SOURCE_PAC_1 */
static uint32_t ble_pacs_source_pac_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_pacs_gatt_request_handler(BLE_PACS_READ_SOURCE_PAC, handle, BLE_PACS_SOURCE_PAC_1, data, size, offset);
    }

    return 0;
}

static uint32_t ble_pacs_source_pac_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_pacs_gatt_request_handler(BLE_PACS_WRITE_SOURCE_PAC_CCCD, handle, BLE_PACS_SOURCE_PAC_1, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_pacs_gatt_request_handler(BLE_PACS_READ_SOURCE_PAC_CCCD, handle, BLE_PACS_SOURCE_PAC_1, data, size, offset);
        }
    }
    return 0;
}

#ifdef MTK_PACS_AUDIO_LOCATION_EN
/* Source Audio Locations */
static uint32_t ble_pacs_source_location_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_pacs_gatt_request_handler(BLE_PACS_READ_SOURCE_LOCATION, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);
    }

    return 0;
}

static uint32_t ble_pacs_source_location_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_pacs_gatt_request_handler(BLE_PACS_WRITE_SOURCE_LOCATION_CCCD, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_pacs_gatt_request_handler(BLE_PACS_READ_SOURCE_LOCATION_CCCD, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);
        }
    }
    return 0;
}
#endif /* MTK_PACS_AUDIO_LOCATION_EN */
#endif /* MTK_LE_AUDIO_CT */

/* Available Audio Contexts */
static uint32_t ble_pacs_available_audio_contexts_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_pacs_gatt_request_handler(BLE_PACS_READ_AVAILABLE_AUDIO_CONTEXTS, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);
    }

    return 0;
}


static uint32_t ble_pacs_available_audio_contexts_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_pacs_gatt_request_handler(BLE_PACS_WRITE_AVAILABLE_AUDIO_CONTEXTS_CCCD, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_pacs_gatt_request_handler(BLE_PACS_READ_AVAILABLE_AUDIO_CONTEXTS_CCCD, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);
        }
    }
    return 0;
}

/* Supported Audio Contexts */
static uint32_t ble_pacs_supported_audio_contexts_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_pacs_gatt_request_handler(BLE_PACS_READ_SUPPORTED_AUDIO_CONTEXTS, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);
    }

    return 0;
}


static uint32_t ble_pacs_supported_audio_contexts_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_pacs_gatt_request_handler(BLE_PACS_WRITE_SUPPORTED_AUDIO_CONTEXTS_CCCD, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_pacs_gatt_request_handler(BLE_PACS_READ_SUPPORTED_AUDIO_CONTEXTS_CCCD, handle, BLE_PACS_CHARC_NORMAL, data, size, offset);
        }
    }
    return 0;
}

/************************************************
*   Public functions
*************************************************/
/* BLE_PACS_SINK_PAC_1 */
#ifdef MTK_PACS_STEREO_EN
#define PACS_SINK_PAC_1_RECORD_NUM      0x08
#else
#define PACS_SINK_PAC_1_RECORD_NUM      0x04
#endif
/* BLE_PACS_SOURCE_PAC_1 */
#ifdef MTK_LE_AUDIO_CT
#define PACS_SOURCE_PAC_1_RECORD_NUM    0x04
#else
#define PACS_SOURCE_PAC_1_RECORD_NUM    0x00
#endif

#define PACS_CODEC_CAPABLITIES_LEN      0x13
#define PACS_METADATA_LEN               0x04

static uint8_t g_pacs_codec_capabilities_16k[] = {
    CODEC_CAPABILITY_LEN_SUPPORTED_SAMPLING_FREQUENCY,
    CODEC_CAPABILITY_TYPE_SUPPORTED_SAMPLING_FREQUENCY,
    (uint8_t)SUPPORTED_SAMPLING_FREQ_16KHZ, (uint8_t)(SUPPORTED_SAMPLING_FREQ_16KHZ >> 8),

    CODEC_CAPABILITY_LEN_SUPPORTED_FRAME_DURATIONS,
    CODEC_CAPABILITY_TYPE_SUPPORTED_FRAME_DURATIONS,
    SUPPORTED_FRAME_DURATIONS_7P5_MS | SUPPORTED_FRAME_DURATIONS_10_MS,

    CODEC_CAPABILITY_LEN_AUDIO_CHANNEL_COUNTS,
    CODEC_CAPABILITY_TYPE_AUDIO_CHANNEL_COUNTS,
#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
#else
    AUDIO_CHANNEL_COUNTS_1,
#endif

    CODEC_CAPABILITY_LEN_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    CODEC_CAPABILITY_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    (uint8_t)SUPPORTED_OCTETS_PER_CODEC_FRAME_30_40, (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_30_40 >> 8),
    (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_30_40 >> 16), (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_30_40 >> 24),

    CODEC_CAPABILITY_LEN_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    CODEC_CAPABILITY_TYPE_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    MAX_SUPPORTED_LC3_FRAMES_PER_SDU_1,
};

#ifdef MTK_PACS_STEREO_EN
static uint8_t g_pacs_codec_capabilities_16k_1[] = {
    CODEC_CAPABILITY_LEN_SUPPORTED_SAMPLING_FREQUENCY,
    CODEC_CAPABILITY_TYPE_SUPPORTED_SAMPLING_FREQUENCY,
    (uint8_t)SUPPORTED_SAMPLING_FREQ_16KHZ, (uint8_t)(SUPPORTED_SAMPLING_FREQ_16KHZ >> 8),

    CODEC_CAPABILITY_LEN_SUPPORTED_FRAME_DURATIONS,
    CODEC_CAPABILITY_TYPE_SUPPORTED_FRAME_DURATIONS,
    SUPPORTED_FRAME_DURATIONS_7P5_MS | SUPPORTED_FRAME_DURATIONS_10_MS,

    CODEC_CAPABILITY_LEN_AUDIO_CHANNEL_COUNTS,
    CODEC_CAPABILITY_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_2,

    CODEC_CAPABILITY_LEN_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    CODEC_CAPABILITY_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    (uint8_t)SUPPORTED_OCTETS_PER_CODEC_FRAME_30_40, (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_30_40 >> 8),
    (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_30_40 >> 16), (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_30_40 >> 24),

    CODEC_CAPABILITY_LEN_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    CODEC_CAPABILITY_TYPE_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    MAX_SUPPORTED_LC3_FRAMES_PER_SDU_1,
};
#endif

static uint8_t g_pacs_codec_capabilities_24k[] = {
    CODEC_CAPABILITY_LEN_SUPPORTED_SAMPLING_FREQUENCY,
    CODEC_CAPABILITY_TYPE_SUPPORTED_SAMPLING_FREQUENCY,
    (uint8_t)SUPPORTED_SAMPLING_FREQ_24KHZ, (uint8_t)(SUPPORTED_SAMPLING_FREQ_24KHZ >> 8),

    CODEC_CAPABILITY_LEN_SUPPORTED_FRAME_DURATIONS,
    CODEC_CAPABILITY_TYPE_SUPPORTED_FRAME_DURATIONS,
    SUPPORTED_FRAME_DURATIONS_7P5_MS | SUPPORTED_FRAME_DURATIONS_10_MS,

    CODEC_CAPABILITY_LEN_AUDIO_CHANNEL_COUNTS,
    CODEC_CAPABILITY_TYPE_AUDIO_CHANNEL_COUNTS,
#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
#else
    AUDIO_CHANNEL_COUNTS_1,
#endif

    CODEC_CAPABILITY_LEN_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    CODEC_CAPABILITY_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    (uint8_t)SUPPORTED_OCTETS_PER_CODEC_FRAME_45_60, (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_45_60 >> 8),
    (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_45_60 >> 16), (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_45_60 >> 24),

    CODEC_CAPABILITY_LEN_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    CODEC_CAPABILITY_TYPE_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    MAX_SUPPORTED_LC3_FRAMES_PER_SDU_1,
};

#ifdef MTK_PACS_STEREO_EN
static uint8_t g_pacs_codec_capabilities_24k_1[] = {
    CODEC_CAPABILITY_LEN_SUPPORTED_SAMPLING_FREQUENCY,
    CODEC_CAPABILITY_TYPE_SUPPORTED_SAMPLING_FREQUENCY,
    (uint8_t)SUPPORTED_SAMPLING_FREQ_24KHZ, (uint8_t)(SUPPORTED_SAMPLING_FREQ_24KHZ >> 8),

    CODEC_CAPABILITY_LEN_SUPPORTED_FRAME_DURATIONS,
    CODEC_CAPABILITY_TYPE_SUPPORTED_FRAME_DURATIONS,
    SUPPORTED_FRAME_DURATIONS_7P5_MS | SUPPORTED_FRAME_DURATIONS_10_MS,

    CODEC_CAPABILITY_LEN_AUDIO_CHANNEL_COUNTS,
    CODEC_CAPABILITY_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_2,

    CODEC_CAPABILITY_LEN_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    CODEC_CAPABILITY_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    (uint8_t)SUPPORTED_OCTETS_PER_CODEC_FRAME_45_60, (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_45_60 >> 8),
    (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_45_60 >> 16), (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_45_60 >> 24),

    CODEC_CAPABILITY_LEN_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    CODEC_CAPABILITY_TYPE_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    MAX_SUPPORTED_LC3_FRAMES_PER_SDU_1,
};
#endif

static uint8_t g_pacs_codec_capabilities_32k[] = {
    CODEC_CAPABILITY_LEN_SUPPORTED_SAMPLING_FREQUENCY,     /* length */
    CODEC_CAPABILITY_TYPE_SUPPORTED_SAMPLING_FREQUENCY,    /* type */
    (uint8_t)SUPPORTED_SAMPLING_FREQ_32KHZ, (uint8_t)(SUPPORTED_SAMPLING_FREQ_32KHZ >> 8),  /* value */

    CODEC_CAPABILITY_LEN_SUPPORTED_FRAME_DURATIONS,
    CODEC_CAPABILITY_TYPE_SUPPORTED_FRAME_DURATIONS,
    SUPPORTED_FRAME_DURATIONS_7P5_MS | SUPPORTED_FRAME_DURATIONS_10_MS,

    CODEC_CAPABILITY_LEN_AUDIO_CHANNEL_COUNTS,
    CODEC_CAPABILITY_TYPE_AUDIO_CHANNEL_COUNTS,
#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
#else
    AUDIO_CHANNEL_COUNTS_1,
#endif

    CODEC_CAPABILITY_LEN_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    CODEC_CAPABILITY_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    (uint8_t)SUPPORTED_OCTETS_PER_CODEC_FRAME_60_80, (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_60_80 >> 8),
    (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_60_80 >> 16), (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_60_80 >> 24),

    CODEC_CAPABILITY_LEN_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    CODEC_CAPABILITY_TYPE_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    MAX_SUPPORTED_LC3_FRAMES_PER_SDU_1,
};

#ifdef MTK_PACS_STEREO_EN
static uint8_t g_pacs_codec_capabilities_32k_1[] = {
    CODEC_CAPABILITY_LEN_SUPPORTED_SAMPLING_FREQUENCY,     /* length */
    CODEC_CAPABILITY_TYPE_SUPPORTED_SAMPLING_FREQUENCY,    /* type */
    (uint8_t)SUPPORTED_SAMPLING_FREQ_32KHZ, (uint8_t)(SUPPORTED_SAMPLING_FREQ_32KHZ >> 8),  /* value */

    CODEC_CAPABILITY_LEN_SUPPORTED_FRAME_DURATIONS,
    CODEC_CAPABILITY_TYPE_SUPPORTED_FRAME_DURATIONS,
    SUPPORTED_FRAME_DURATIONS_7P5_MS | SUPPORTED_FRAME_DURATIONS_10_MS,

    CODEC_CAPABILITY_LEN_AUDIO_CHANNEL_COUNTS,
    CODEC_CAPABILITY_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_2,

    CODEC_CAPABILITY_LEN_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    CODEC_CAPABILITY_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    (uint8_t)SUPPORTED_OCTETS_PER_CODEC_FRAME_60_80, (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_60_80 >> 8),
    (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_60_80 >> 16), (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_60_80 >> 24),

    CODEC_CAPABILITY_LEN_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    CODEC_CAPABILITY_TYPE_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    MAX_SUPPORTED_LC3_FRAMES_PER_SDU_1,
};
#endif

static uint8_t g_pacs_codec_capabilities_48k[] = {
    CODEC_CAPABILITY_LEN_SUPPORTED_SAMPLING_FREQUENCY,
    CODEC_CAPABILITY_TYPE_SUPPORTED_SAMPLING_FREQUENCY,
    (uint8_t)SUPPORTED_SAMPLING_FREQ_48KHZ, (uint8_t)(SUPPORTED_SAMPLING_FREQ_48KHZ >> 8),

    CODEC_CAPABILITY_LEN_SUPPORTED_FRAME_DURATIONS,
    CODEC_CAPABILITY_TYPE_SUPPORTED_FRAME_DURATIONS,
    SUPPORTED_FRAME_DURATIONS_7P5_MS | SUPPORTED_FRAME_DURATIONS_10_MS,

    CODEC_CAPABILITY_LEN_AUDIO_CHANNEL_COUNTS,
    CODEC_CAPABILITY_TYPE_AUDIO_CHANNEL_COUNTS,
#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
#else
    AUDIO_CHANNEL_COUNTS_1,
#endif

    CODEC_CAPABILITY_LEN_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    CODEC_CAPABILITY_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    (uint8_t)SUPPORTED_OCTETS_PER_CODEC_FRAME_75_155, (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_75_155 >> 8),
    (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_75_155 >> 16), (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_75_155 >> 24),

    CODEC_CAPABILITY_LEN_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    CODEC_CAPABILITY_TYPE_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    MAX_SUPPORTED_LC3_FRAMES_PER_SDU_1,
};

#ifdef MTK_PACS_STEREO_EN
static uint8_t g_pacs_codec_capabilities_48k_1[] = {
    CODEC_CAPABILITY_LEN_SUPPORTED_SAMPLING_FREQUENCY,
    CODEC_CAPABILITY_TYPE_SUPPORTED_SAMPLING_FREQUENCY,
    (uint8_t)SUPPORTED_SAMPLING_FREQ_48KHZ, (uint8_t)(SUPPORTED_SAMPLING_FREQ_48KHZ >> 8),

    CODEC_CAPABILITY_LEN_SUPPORTED_FRAME_DURATIONS,
    CODEC_CAPABILITY_TYPE_SUPPORTED_FRAME_DURATIONS,
    SUPPORTED_FRAME_DURATIONS_7P5_MS | SUPPORTED_FRAME_DURATIONS_10_MS,

    CODEC_CAPABILITY_LEN_AUDIO_CHANNEL_COUNTS,
    CODEC_CAPABILITY_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_2,

    CODEC_CAPABILITY_LEN_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    CODEC_CAPABILITY_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    (uint8_t)SUPPORTED_OCTETS_PER_CODEC_FRAME_75_155, (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_75_155 >> 8),
    (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_75_155 >> 16), (uint8_t)(SUPPORTED_OCTETS_PER_CODEC_FRAME_75_155 >> 24),

    CODEC_CAPABILITY_LEN_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    CODEC_CAPABILITY_TYPE_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU,
    MAX_SUPPORTED_LC3_FRAMES_PER_SDU_1,
};
#endif

static uint8_t g_pacs_metadata[] = {0x03, 0x01, 0x01, 0x00};

static ble_pacs_pac_record_t g_pacs_pac_1[] = {
    {
        CODEC_ID_LC3,
        PACS_CODEC_CAPABLITIES_LEN,
        &g_pacs_codec_capabilities_16k[0],
        PACS_METADATA_LEN,
        &g_pacs_metadata[0],
    },
#ifdef MTK_PACS_STEREO_EN
    {
        CODEC_ID_LC3,
        PACS_CODEC_CAPABLITIES_LEN,
        &g_pacs_codec_capabilities_16k_1[0],
        PACS_METADATA_LEN,
        &g_pacs_metadata[0],
    },
#endif
    {
        CODEC_ID_LC3,
        PACS_CODEC_CAPABLITIES_LEN,
        &g_pacs_codec_capabilities_32k[0],
        PACS_METADATA_LEN,
        &g_pacs_metadata[0],
    },
#ifdef MTK_PACS_STEREO_EN
    {
        CODEC_ID_LC3,
        PACS_CODEC_CAPABLITIES_LEN,
        &g_pacs_codec_capabilities_32k_1[0],
        PACS_METADATA_LEN,
        &g_pacs_metadata[0],
    },
#endif
    {
        CODEC_ID_LC3,
        PACS_CODEC_CAPABLITIES_LEN,
        &g_pacs_codec_capabilities_24k[0],
        PACS_METADATA_LEN,
        &g_pacs_metadata[0],
    },
#ifdef MTK_PACS_STEREO_EN
    {
        CODEC_ID_LC3,
        PACS_CODEC_CAPABLITIES_LEN,
        &g_pacs_codec_capabilities_24k_1[0],
        PACS_METADATA_LEN,
        &g_pacs_metadata[0],
    },
#endif
    {
        CODEC_ID_LC3,
        PACS_CODEC_CAPABLITIES_LEN,
        &g_pacs_codec_capabilities_48k[0],
        PACS_METADATA_LEN,
        &g_pacs_metadata[0],
    },
#ifdef MTK_PACS_STEREO_EN
    {
        CODEC_ID_LC3,
        PACS_CODEC_CAPABLITIES_LEN,
        &g_pacs_codec_capabilities_48k_1[0],
        PACS_METADATA_LEN,
        &g_pacs_metadata[0],
    },
#endif
};

/* BLE_PACS_PAC_1 */
ble_pacs_pac_t g_pacs_pac_empty = {
    0,
    NULL,
};

/* BLE_PACS_SINK_PAC_1 */
ble_pacs_pac_t g_pacs_sink_pac_1 = {
    PACS_SINK_PAC_1_RECORD_NUM,
    &g_pacs_pac_1[0],
};

ble_pacs_pac_t g_pacs_source_pac_1 = {
    PACS_SOURCE_PAC_1_RECORD_NUM,
#ifdef MTK_LE_AUDIO_CT
    &g_pacs_pac_1[0],
#else
    &g_pacs_pac_empty,
#endif
};

ble_pacs_attribute_handle_t *ble_pacs_get_attribute_handle_tbl(void)
{
    return g_pacs_att_handle_tbl;
}

uint8_t ble_pacs_get_sink_pac_number(void)
{
    return BLE_PACS_SINK_PAC_MAX_NUM;
}

uint8_t ble_pacs_get_source_pac_number(void)
{
    return BLE_PACS_SOURCE_PAC_MAX_NUM;
}

bt_status_t ble_pacs_set_pac_record_availability(bt_le_audio_direction_t direction, bool available)
{
    //bt_handle_t handle = BT_HANDLE_INVALID;
    bt_status_t status = BT_STATUS_FAIL;
    if (direction == AUDIO_DIRECTION_SINK) {
        status = ble_pacs_set_pac(direction, BLE_PACS_SINK_PAC_1, (available ? &g_pacs_sink_pac_1 : &g_pacs_pac_empty));
#ifdef MTK_LE_AUDIO_CT
    } else if (direction == AUDIO_DIRECTION_SOURCE) {
        status = ble_pacs_set_pac(direction, BLE_PACS_SOURCE_PAC_1, (available ? &g_pacs_source_pac_1 : &g_pacs_pac_empty));
#endif
    }

    return status;
}

void ble_pacs_init_parameter(void)
{
    audio_channel_t channel = ami_get_audio_channel();

#ifdef MTK_LE_AUDIO_TWO_CHANNEL
    ble_pacs_set_audio_location(AUDIO_DIRECTION_SINK, AUDIO_LOCATION_FRONT_LEFT | AUDIO_LOCATION_FRONT_RIGHT);
#ifdef MTK_LE_AUDIO_CT
    ble_pacs_set_audio_location(AUDIO_DIRECTION_SOURCE, AUDIO_LOCATION_FRONT_LEFT | AUDIO_LOCATION_FRONT_RIGHT);
#endif
#elif defined(AIR_LE_AUDIO_HEADSET_ENABLE)
    ble_pacs_set_audio_location(AUDIO_DIRECTION_SINK, AUDIO_LOCATION_FRONT_LEFT | AUDIO_LOCATION_FRONT_RIGHT);

    ble_pacs_set_audio_location(AUDIO_DIRECTION_SOURCE, AUDIO_LOCATION_FRONT_LEFT | AUDIO_LOCATION_FRONT_RIGHT);
#elif defined(MTK_PACS_AUDIO_LOCATION_EN) /* 1 mono */
    ble_pacs_set_audio_location(AUDIO_DIRECTION_SINK, (channel == AUDIO_CHANNEL_NONE) ? AUDIO_LOCATION_NONE : (channel == AUDIO_CHANNEL_R) ? AUDIO_LOCATION_FRONT_RIGHT : AUDIO_LOCATION_FRONT_LEFT);

    ble_pacs_set_audio_location(AUDIO_DIRECTION_SOURCE, AUDIO_LOCATION_FRONT_LEFT);
#endif

#ifdef MTK_LE_AUDIO_CT
    ble_pacs_set_available_audio_contexts(AUDIO_CONTENT_TYPE_ALL,
                                          AUDIO_CONTENT_TYPE_CONVERSATIONAL);

    ble_pacs_set_supported_audio_contexts(AUDIO_CONTENT_TYPE_ALL,
                                          AUDIO_CONTENT_TYPE_ALL);
#else
    ble_pacs_set_available_audio_contexts(AUDIO_CONTENT_TYPE_MEDIA, AUDIO_CONTENT_TYPE_NOT_AVAILABLE);

    ble_pacs_set_supported_audio_contexts(AUDIO_CONTENT_TYPE_MEDIA, AUDIO_CONTENT_TYPE_NOT_AVAILABLE);
#endif

    ble_pacs_set_pac(AUDIO_DIRECTION_SINK, BLE_PACS_SINK_PAC_1, &g_pacs_sink_pac_1);
#ifdef MTK_LE_AUDIO_CT
    ble_pacs_set_pac(AUDIO_DIRECTION_SOURCE, BLE_PACS_SOURCE_PAC_1, &g_pacs_source_pac_1);
#endif
}

bool ble_pacs_set_cccd_handler(bt_handle_t conn_handle, uint16_t attr_handle, uint16_t value)
{
    if (attr_handle < PACS_END_HANDLE && attr_handle > PACS_START_HANDLE) {
        ble_pacs_set_cccd_with_att_handle(conn_handle, attr_handle, value);
        return true;
    }
    return false;
}

bt_le_audio_cccd_record_t* ble_pacs_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num)
{
    if (num == NULL) {
        return NULL;
    }

    return ble_pacs_get_cccd_with_att_handle(conn_handle, num);
}

