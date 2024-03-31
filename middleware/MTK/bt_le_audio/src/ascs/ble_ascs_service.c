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

#include "ble_ascs_def.h"
#include "ble_ascs.h"
#include "bt_le_audio_log.h"

#include "bt_gap_le.h"
#include "bt_gatts.h"

/************************************************
*   ATTRIBUTE  HANDLE
*************************************************/
#define ASCS_START_HANDLE                   0x1103          /**< ASCS service start handle.*/
#define ASCS_VALUE_HANDLE_SINK_ASE_1        0x1105          /**< Sink ASE_1 characteristic handle.*/
#define ASCS_VALUE_HANDLE_SINK_ASE_2        0x1108          /**< Sink ASE_2 characteristic handle.*/
#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
#define ASCS_VALUE_HANDLE_SINK_ASE_3        0x110B          /**< Sink ASE_3 characteristic handle.*/
#define ASCS_VALUE_HANDLE_SINK_ASE_4        0x110E          /**< Sink ASE_4 characteristic handle.*/
#define ASCS_VALUE_HANDLE_SOURCE_ASE_1      0x1111          /**< Source ASE_1 characteristic handle.*/
#define ASCS_VALUE_HANDLE_ASE_CONTROL_POINT 0x1114          /**< ASE Control Point characteristic handle.*/
#define ASCS_END_HANDLE                     0x1115          /**< ASCS service end handle.*/
#else
#ifdef MTK_LE_AUDIO_CT
#define ASCS_VALUE_HANDLE_SOURCE_ASE_1      0x110B          /**< Source ASE_1 characteristic handle.*/
#define ASCS_VALUE_HANDLE_ASE_CONTROL_POINT 0x110E          /**< ASE Control Point characteristic handle.*/
#else
#define ASCS_VALUE_HANDLE_ASE_CONTROL_POINT 0x110B          /**< ASE Control Point characteristic handle.*/
#endif
#define ASCS_END_HANDLE                     ASCS_VALUE_HANDLE_ASE_CONTROL_POINT + 1 /**< ASCS service end handle.*/
#endif

/************************************************
*   CHARC INDEX
*************************************************/
typedef enum {
    BLE_ASCS_SINK_ASE_1,
    BLE_ASCS_SINK_ASE_2,
#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
    BLE_ASCS_SINK_ASE_3,
    BLE_ASCS_SINK_ASE_4,
#endif
#ifdef MTK_LE_AUDIO_CT
    BLE_ASCS_SOURCE_ASE_1,
#endif
    BLE_ASCS_ASE_MAX_NUM,
} ble_ascs_charc_ase_t;

#define BLE_ASCS_CHARC_NORMAL  0

/************************************************
*   UUID
*************************************************/
static const bt_uuid_t BT_SIG_UUID_SINK_ASE                 = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_SINK_ASE);
static const bt_uuid_t BT_SIG_UUID_SOURCE_ASE               = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_SOURCE_ASE);
static const bt_uuid_t BT_SIG_UUID_ASE_CONTROL_POINT = BT_UUID_INIT_WITH_UUID16(BT_SIG_UUID16_ASE_CONTROL_POINT);

/************************************************
*   ATTRIBUTE VALUE HANDLE
*************************************************/
static ble_ascs_attribute_handle_t g_ascs_att_handle_tbl[] = {
    {BLE_ASCS_UUID_TYPE_ASCS_SERVICE,        ASCS_START_HANDLE},
    /* BLE_ASCS_SINK_ASE_1 */
    {BLE_ASCS_UUID_TYPE_SINK_ASE,            ASCS_VALUE_HANDLE_SINK_ASE_1},
    /* BLE_ASCS_SINK_ASE_2 */
    {BLE_ASCS_UUID_TYPE_SINK_ASE,            ASCS_VALUE_HANDLE_SINK_ASE_2},
#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
    /* BLE_ASCS_SINK_ASE_3 */
    {BLE_ASCS_UUID_TYPE_SINK_ASE,            ASCS_VALUE_HANDLE_SINK_ASE_3},
    /* BLE_ASCS_SINK_ASE_4 */
    {BLE_ASCS_UUID_TYPE_SINK_ASE,            ASCS_VALUE_HANDLE_SINK_ASE_4},
    /* BLE_ASCS_SOURCE_ASE_1 */
    {BLE_ASCS_UUID_TYPE_SOURCE_ASE,          ASCS_VALUE_HANDLE_SOURCE_ASE_1},
    /* ASE Control Point */
    {BLE_ASCS_UUID_TYPE_ASE_CONTROL_POINT,   ASCS_VALUE_HANDLE_ASE_CONTROL_POINT},
    {BLE_ASCS_UUID_TYPE_INVALID,             ASCS_END_HANDLE},
#else
#ifdef MTK_LE_AUDIO_CT
    /* BLE_ASCS_SOURCE_ASE_1 */
    {BLE_ASCS_UUID_TYPE_SOURCE_ASE,          ASCS_VALUE_HANDLE_SOURCE_ASE_1},
#endif
    /* ASE Control Point */
    {BLE_ASCS_UUID_TYPE_ASE_CONTROL_POINT,   ASCS_VALUE_HANDLE_ASE_CONTROL_POINT},
    {BLE_ASCS_UUID_TYPE_INVALID,             ASCS_END_HANDLE},
#endif
};

/************************************************
*   QOS PREFERENCE
*************************************************/
ble_ascs_default_qos_parameters_t g_default_qos_low_latency_7p5ms[] = {
    {
        /*24k 48kbps*/
        .preferred_retranmission_num = 2,
        .max_transport_latency = 8,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*16k 32kbps*/
        .preferred_retranmission_num = 2,
        .max_transport_latency = 8,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*32k 64kbps*/
        .preferred_retranmission_num = 2,
        .max_transport_latency = 8,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 80kbps*/
        .preferred_retranmission_num = 5,
        .max_transport_latency = 15,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 96kbps*/
        .preferred_retranmission_num = 5,
        .max_transport_latency = 15,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 124kbps*/
        .preferred_retranmission_num = 5,
        .max_transport_latency = 15,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
};

ble_ascs_default_qos_parameters_t g_default_qos_high_reliability_7p5ms[] = {
    {
        /*24k 48kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 75,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*16k 32kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 75,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*32k 64kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 75,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 80kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 75,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 96kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 75,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 124kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 75,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
};

ble_ascs_default_qos_parameters_t g_default_qos_low_latency_10ms[] = {
    {
        /*24k 48kbps*/
        .preferred_retranmission_num = 2,
        .max_transport_latency = 10,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*16k 32kbps*/
        .preferred_retranmission_num = 2,
        .max_transport_latency = 10,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*32k 64kbps*/
        .preferred_retranmission_num = 2,
        .max_transport_latency = 10,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 80kbps*/
        .preferred_retranmission_num = 5,
        .max_transport_latency = 20,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 96kbps*/
        .preferred_retranmission_num = 5,
        .max_transport_latency = 20,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 124kbps*/
        .preferred_retranmission_num = 5,
        .max_transport_latency = 20,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
};

ble_ascs_default_qos_parameters_t g_default_qos_high_reliability_10ms[] = {
    {
        /*24k 48kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 95,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*16k 32kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 95,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*32k 64kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 95,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 80kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 95,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 96kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 100,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
    {
        /*48k 124kbps*/
        .preferred_retranmission_num = 13,
        .max_transport_latency = 100,
        .supported_presentation_delay_min = SUPPORTED_PRESENTATION_DELAY_MIN,
        .supported_presentation_delay_max = SUPPORTED_PRESENTATION_DELAY_MAX,
        .preferred_presentation_delay_min = PREFERRED_PRESENTATION_DELAY_MIN,
        .preferred_presentation_delay_max = PREFERRED_PRESENTATION_DELAY_MAX,
    },
};

/************************************************
*   CALLBACK
*************************************************/
/* BLE_ASCS_SINK_ASE_1 */
static uint32_t ble_ascs_sink_ase_value_callback_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_ascs_sink_ase_client_config_callback_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

/* BLE_ASCS_SINK_ASE_2 */
static uint32_t ble_ascs_sink_ase_value_callback_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_ascs_sink_ase_client_config_callback_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
/* BLE_ASCS_SINK_ASE_3 */
static uint32_t ble_ascs_sink_ase_value_callback_3(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_ascs_sink_ase_client_config_callback_3(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);

/* BLE_ASCS_SINK_ASE_4 */
static uint32_t ble_ascs_sink_ase_value_callback_4(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_ascs_sink_ase_client_config_callback_4(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
#endif

#ifdef MTK_LE_AUDIO_CT
/* BLE_ASCS_SOURCE_ASE_1 */
static uint32_t ble_ascs_source_ase_value_callback_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_ascs_source_ase_client_config_callback_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
#endif

/* ASE Control Point */
static uint32_t ble_ascs_ase_control_point_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);
static uint32_t ble_ascs_ase_control_point_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset);


/************************************************
*   SERVICE TABLE
*************************************************/
BT_GATTS_NEW_PRIMARY_SERVICE_16(ble_ascs_primary_service, BT_GATT_UUID16_ASCS_SERVICE);

/* BLE_ASCS_SINK_ASE_1 */
BT_GATTS_NEW_CHARC_16(ble_ascs_char4_ascs_sink_ase_1,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      ASCS_VALUE_HANDLE_SINK_ASE_1,
                      BT_SIG_UUID16_SINK_ASE);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_ascs_sink_ase_value_1,
                                  BT_SIG_UUID_SINK_ASE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_ascs_sink_ase_value_callback_1);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_ascs_sink_ase_config_1,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_ascs_sink_ase_client_config_callback_1);

/* BLE_ASCS_SINK_ASE_2 */
BT_GATTS_NEW_CHARC_16(ble_ascs_char4_ascs_sink_ase_2,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      ASCS_VALUE_HANDLE_SINK_ASE_2,
                      BT_SIG_UUID16_SINK_ASE);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_ascs_sink_ase_value_2,
                                  BT_SIG_UUID_SINK_ASE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_ascs_sink_ase_value_callback_2);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_ascs_sink_ase_config_2,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_ascs_sink_ase_client_config_callback_2);

#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
/* BLE_ASCS_SINK_ASE_3 */
BT_GATTS_NEW_CHARC_16(ble_ascs_char4_ascs_sink_ase_3,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      ASCS_VALUE_HANDLE_SINK_ASE_3,
                      BT_SIG_UUID16_SINK_ASE);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_ascs_sink_ase_value_3,
                                  BT_SIG_UUID_SINK_ASE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_ascs_sink_ase_value_callback_3);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_ascs_sink_ase_config_3,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_ascs_sink_ase_client_config_callback_3);

/* BLE_ASCS_SINK_ASE_4 */
BT_GATTS_NEW_CHARC_16(ble_ascs_char4_ascs_sink_ase_4,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      ASCS_VALUE_HANDLE_SINK_ASE_4,
                      BT_SIG_UUID16_SINK_ASE);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_ascs_sink_ase_value_4,
                                  BT_SIG_UUID_SINK_ASE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_ascs_sink_ase_value_callback_4);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_ascs_sink_ase_config_4,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_ascs_sink_ase_client_config_callback_4);
#endif

#ifdef MTK_LE_AUDIO_CT
/* BLE_ASCS_SOURCE_ASE_1 */
BT_GATTS_NEW_CHARC_16(ble_ascs_char4_ascs_source_ase_1,
                      BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY,
                      ASCS_VALUE_HANDLE_SOURCE_ASE_1,
                      BT_SIG_UUID16_SOURCE_ASE);

BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_ascs_source_ase_value_1,
                                  BT_SIG_UUID_SOURCE_ASE,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_ascs_source_ase_value_callback_1);

BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_ascs_source_ase_config_1,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_ascs_source_ase_client_config_callback_1);
#endif

/* ASE Control Point */
BT_GATTS_NEW_CHARC_16(ble_ascs_char4_ascs_ase_control_point,
                      BT_GATT_CHARC_PROP_NOTIFY | BT_GATT_CHARC_PROP_WRITE | BT_GATT_CHARC_PROP_WRITE_WITHOUT_RSP,
                      ASCS_VALUE_HANDLE_ASE_CONTROL_POINT,
                      BT_SIG_UUID16_ASE_CONTROL_POINT);
BT_GATTS_NEW_CHARC_VALUE_CALLBACK(ble_ascs_ase_control_point_value,
                                  BT_SIG_UUID_ASE_CONTROL_POINT,
                                  BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                  ble_ascs_ase_control_point_value_callback);
BT_GATTS_NEW_CLIENT_CHARC_CONFIG(ble_ascs_ase_control_point_config,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_ascs_ase_control_point_client_config_callback);

static const bt_gatts_service_rec_t *ble_ascs_service_rec[] = {
    (const bt_gatts_service_rec_t *) &ble_ascs_primary_service,
    /* BLE_ASCS_SINK_ASE_1 */
    (const bt_gatts_service_rec_t *) &ble_ascs_char4_ascs_sink_ase_1,
    (const bt_gatts_service_rec_t *) &ble_ascs_sink_ase_value_1,
    (const bt_gatts_service_rec_t *) &ble_ascs_sink_ase_config_1,

    /* BLE_ASCS_SINK_ASE_2 */
    (const bt_gatts_service_rec_t *) &ble_ascs_char4_ascs_sink_ase_2,
    (const bt_gatts_service_rec_t *) &ble_ascs_sink_ase_value_2,
    (const bt_gatts_service_rec_t *) &ble_ascs_sink_ase_config_2,

#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
    /* BLE_ASCS_SINK_ASE_3 */
    (const bt_gatts_service_rec_t *) &ble_ascs_char4_ascs_sink_ase_3,
    (const bt_gatts_service_rec_t *) &ble_ascs_sink_ase_value_3,
    (const bt_gatts_service_rec_t *) &ble_ascs_sink_ase_config_3,

    /* BLE_ASCS_SINK_ASE_4 */
    (const bt_gatts_service_rec_t *) &ble_ascs_char4_ascs_sink_ase_4,
    (const bt_gatts_service_rec_t *) &ble_ascs_sink_ase_value_4,
    (const bt_gatts_service_rec_t *) &ble_ascs_sink_ase_config_4,
#endif

#ifdef MTK_LE_AUDIO_CT
    /* BLE_ASCS_SOURCE_ASE_1 */
    (const bt_gatts_service_rec_t *) &ble_ascs_char4_ascs_source_ase_1,
    (const bt_gatts_service_rec_t *) &ble_ascs_source_ase_value_1,
    (const bt_gatts_service_rec_t *) &ble_ascs_source_ase_config_1,
#endif

    /* BLE_ASCS_ASE_CONTROL_POINT  */
    (const bt_gatts_service_rec_t *) &ble_ascs_char4_ascs_ase_control_point,
    (const bt_gatts_service_rec_t *) &ble_ascs_ase_control_point_value,
    (const bt_gatts_service_rec_t *) &ble_ascs_ase_control_point_config,
};

const bt_gatts_service_t ble_ascs_service = {
    .starting_handle = ASCS_START_HANDLE,
    .ending_handle = ASCS_END_HANDLE,
    .required_encryption_key_size = 7,
    .records = ble_ascs_service_rec
};

/************************************************
*   CALLBACK
*************************************************/
/* BLE_ASCS_SINK_ASE_1 */
static uint32_t ble_ascs_sink_ase_value_callback_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE, handle, BLE_ASCS_SINK_ASE_1, data, size, offset);
    }

    return 0;
}

static uint32_t ble_ascs_sink_ase_client_config_callback_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        //MTK_Titan: add BT_GATTS_CALLBACK_WRITE_COMMAND
        if (rw == BT_GATTS_CALLBACK_WRITE || rw == BT_GATTS_CALLBACK_WRITE_COMMAND) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_WRITE_ASE_CCCD, handle, BLE_ASCS_SINK_ASE_1, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE_CCCD, handle, BLE_ASCS_SINK_ASE_1, data, size, offset);
        }
    }
    return 0;
}

/* BLE_ASCS_SINK_ASE_2 */
static uint32_t ble_ascs_sink_ase_value_callback_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE, handle, BLE_ASCS_SINK_ASE_2, data, size, offset);
    }

    return 0;
}

static uint32_t ble_ascs_sink_ase_client_config_callback_2(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        //MTK_Titan: add BT_GATTS_CALLBACK_WRITE_COMMAND
        if (rw == BT_GATTS_CALLBACK_WRITE || rw == BT_GATTS_CALLBACK_WRITE_COMMAND) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_WRITE_ASE_CCCD, handle, BLE_ASCS_SINK_ASE_2, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE_CCCD, handle, BLE_ASCS_SINK_ASE_2, data, size, offset);
        }
    }
    return 0;
}

#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
/* BLE_ASCS_SINK_ASE_3 */
static uint32_t ble_ascs_sink_ase_value_callback_3(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE, handle, BLE_ASCS_SINK_ASE_3, data, size, offset);
    }

    return 0;
}

static uint32_t ble_ascs_sink_ase_client_config_callback_3(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_WRITE_ASE_CCCD, handle, BLE_ASCS_SINK_ASE_3, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE_CCCD, handle, BLE_ASCS_SINK_ASE_3, data, size, offset);
        }
    }
    return 0;
}

/* BLE_ASCS_SINK_ASE_4 */
static uint32_t ble_ascs_sink_ase_value_callback_4(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE, handle, BLE_ASCS_SINK_ASE_4, data, size, offset);
    }

    return 0;
}

static uint32_t ble_ascs_sink_ase_client_config_callback_4(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        if (rw == BT_GATTS_CALLBACK_WRITE) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_WRITE_ASE_CCCD, handle, BLE_ASCS_SINK_ASE_4, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE_CCCD, handle, BLE_ASCS_SINK_ASE_4, data, size, offset);
        }
    }
    return 0;
}
#endif

#ifdef MTK_LE_AUDIO_CT
/* BLE_ASCS_SOURCE_ASE_1 */
static uint32_t ble_ascs_source_ase_value_callback_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID && rw == BT_GATTS_CALLBACK_READ) {
        return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE, handle, BLE_ASCS_SOURCE_ASE_1, data, size, offset);
    }

    return 0;
}

static uint32_t ble_ascs_source_ase_client_config_callback_1(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        //MTK_Titan: add BT_GATTS_CALLBACK_WRITE_COMMAND
        if (rw == BT_GATTS_CALLBACK_WRITE || rw == BT_GATTS_CALLBACK_WRITE_COMMAND) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_WRITE_ASE_CCCD, handle, BLE_ASCS_SOURCE_ASE_1, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE_CCCD, handle, BLE_ASCS_SOURCE_ASE_1, data, size, offset);
        }
    }
    return 0;
}
#endif

static uint32_t ble_ascs_ase_control_point_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    LE_AUDIO_LOG_I(BT_LE_AUDIO_ASCS_023, 5, rw, handle , data, size, offset);
     //MTK_Titan: add BT_GATTS_CALLBACK_WRITE_COMMAND
     if (handle != BT_HANDLE_INVALID &&
        (rw == BT_GATTS_CALLBACK_WRITE || rw == BT_GATTS_CALLBACK_PREPARE_WRITE || rw == BT_GATTS_CALLBACK_WRITE_COMMAND)) {
        return ble_ascs_gatt_request_handler(BLE_ASCS_WRITE_ASE_CONTROL_POINT, handle, BLE_ASCS_CHARC_NORMAL, data, size, offset);
    }

    return 0;
}

static uint32_t ble_ascs_ase_control_point_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    if (handle != BT_HANDLE_INVALID) {
        //MTK_Titan: add BT_GATTS_CALLBACK_WRITE_COMMAND
        if (rw == BT_GATTS_CALLBACK_WRITE || rw == BT_GATTS_CALLBACK_WRITE_COMMAND) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_WRITE_ASE_CONTROL_POINT_CCCD, handle, BLE_ASCS_CHARC_NORMAL, data, size, offset);

        } else if (rw == BT_GATTS_CALLBACK_READ) {
            return ble_ascs_gatt_request_handler(BLE_ASCS_READ_ASE_CONTROL_POINT_CCCD, handle, BLE_ASCS_CHARC_NORMAL, data, size, offset);
        }
    }
    return 0;
}

/************************************************
*   Public functions
*************************************************/
ble_ascs_attribute_handle_t *ble_ascs_get_attribute_handle_tbl(void)
{
    return g_ascs_att_handle_tbl;
}

uint8_t ble_ascs_get_ase_number(void)
{
    return BLE_ASCS_ASE_MAX_NUM;
}

bool ble_ascs_set_cccd_handler(bt_handle_t conn_handle, uint16_t attr_handle, uint16_t value)
{
    if (attr_handle < ASCS_END_HANDLE && attr_handle > ASCS_START_HANDLE) {
        ble_ascs_set_cccd(conn_handle, attr_handle, value);
        return true;
    }
    return false;
}

bt_le_audio_cccd_record_t* ble_ascs_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num)
{
    if (num == NULL) {
        return NULL;
    }

    return ble_ascs_get_cccd(conn_handle, num);
}

ble_ascs_default_qos_parameters_t *ble_ascs_get_qos_preference(uint8_t target_latency, uint8_t frame_duration, uint16_t configed_octets_per_codec_frame)
{
    uint8_t preference_idx = 0;

    if (frame_duration == FRAME_DURATIONS_10_MS) {
        switch (configed_octets_per_codec_frame) {
            case ASCS_OCTETS_PER_CODEC_FRAME_40:
                preference_idx = QOS_32KBPS_SETTING;
                break;

            case ASCS_OCTETS_PER_CODEC_FRAME_60:
                preference_idx = QOS_48KBPS_SETTING;
                break;

            case ASCS_OCTETS_PER_CODEC_FRAME_80:
                preference_idx = QOS_64KBPS_SETTING;
                break;

            case ASCS_OCTETS_PER_CODEC_FRAME_100:
                preference_idx = QOS_80KBPS_SETTING;
                break;

            case ASCS_OCTETS_PER_CODEC_FRAME_120:
                preference_idx = QOS_96KBPS_SETTING;
                break;

            case ASCS_OCTETS_PER_CODEC_FRAME_155:
                preference_idx = QOS_124KBPS_SETTING;
                break;

            default:
                assert(0);
        }
        return (target_latency == TARGET_HIGHER_RELIABILITY ? (&g_default_qos_high_reliability_10ms[preference_idx]) : (&g_default_qos_low_latency_10ms[preference_idx]));
    } else if (frame_duration == FRAME_DURATIONS_7P5_MS) {

        switch (configed_octets_per_codec_frame) {
            case ASCS_OCTETS_PER_CODEC_FRAME_30:
                preference_idx = QOS_32KBPS_SETTING;
                break;

            case ASCS_OCTETS_PER_CODEC_FRAME_45:
                preference_idx = QOS_48KBPS_SETTING;
                break;

            case ASCS_OCTETS_PER_CODEC_FRAME_60:
                preference_idx = QOS_64KBPS_SETTING;
                break;

            case ASCS_OCTETS_PER_CODEC_FRAME_75:
                preference_idx = QOS_80KBPS_SETTING;
                break;

            case ASCS_OCTETS_PER_CODEC_FRAME_90:
                preference_idx = QOS_96KBPS_SETTING;
                break;

            case ASCS_OCTETS_PER_CODEC_FRAME_117:
                preference_idx = QOS_124KBPS_SETTING;
                break;

            default:
                assert(0);
        }

        return (target_latency == TARGET_HIGHER_RELIABILITY ? (&g_default_qos_high_reliability_7p5ms[preference_idx]) : (&g_default_qos_low_latency_7p5ms[preference_idx]));
    }

    return NULL;
}

