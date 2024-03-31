/* Copyright Statement:
 *
 * (C) 2021  MediaTek Inc. All rights reserved.
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


#include "apps_debug.h"
#include "apps_customer_config.h"
#include "apps_config_multi_le_adv.h"
#include "apps_config_event_list.h"
#include "apps_events_event_group.h"
#include "app_bt_utils.h"

#include "app_le_audio.h"
#include "app_mcp.h"
#include "ui_shell_manager.h"

#include "bt_gap_le.h"
#include "bt_gap_le_audio.h"
#include "bt_gattc.h"

#include "ble_csis.h"
#include "ble_pacs.h"
#include "ble_bass_def.h"
#include "ble_ascs_def.h"
#include "ble_cas_def.h"
#include "ble_tmas_def.h"

#include "bt_sink_srv_le.h"
#include "bt_sink_srv_le_cap_audio_manager.h"
#include "bt_callback_manager.h"
#include "bt_device_manager_common.h"
#include "bt_device_manager_le.h"

#include "bt_sink_srv_le_volume.h"

#ifdef MTK_NVDM_ENABLE
#include "nvkey.h"

#define NVDM_GROUP_BT_VOL "BT_VOL"
#define NVDM_ITEM_BT_VOL  "VOL_STATE"
#endif /* #ifdef MTK_NVDM_ENABLE */


//#define APPS_LEA_USE_LEGACY_ADV

#define APP_LEA_DEFAULT_VOLUME 0x7F
#define APP_LEA_DEFAULT_MUTE      0

/* For vol_state in app_lea_volume_state_t */
#define APP_LEA_VOLUME_STATE_FLAGS_NOT_PERSISTED 0  /* Reset Volume Setting */
#define APP_LEA_VOLUME_STATE_FLAGS_PERSISTED     1  /* User Set Volume Setting */

#define APP_LOG_TAG "[APP_LA]"

/* With this macro enabled, we will use random address for LE Audio Advertising. */
#define APP_LEA_ADV_USE_RANDOM_ADDR

#define APP_LE_AUDIO_ADV_INTERVAL_MIN_S 0x0020  /* 20 ms */
#define APP_LE_AUDIO_ADV_INTERVAL_MAX_S 0x0030  /* 30 ms */

#ifdef APPS_LEA_USE_LEGACY_ADV
#define APP_LE_AUDIO_ADV_DATA_LEN_MAX       31 /* In core spec., Legacy ADV data cannot exceed 31bytes*/
#else /* #ifdef APPS_LEA_USE_LEGACY_ADV */
#define APP_LE_AUDIO_ADV_DATA_LEN_MAX       (48 + BT_GAP_LE_MAX_DEVICE_NAME_LENGTH)
#endif /* #ifdef APPS_LEA_USE_LEGACY_ADV */
#define APP_LE_AUDIO_ADV_SCAN_RSP_LEN_MAX   (BT_GAP_LE_MAX_DEVICE_NAME_LENGTH + 2)

typedef enum {
    LEA_ADV_STATE_DISABLED,
    LEA_ADV_STATE_CONFIGING,
    LEA_ADV_STATE_CONFIG,
    LEA_ADV_STATE_ENABLING,
    LEA_ADV_STATE_ENABLED,
    LEA_ADV_STATE_DISABLING,
} app_lea_adv_state_e;

typedef struct {
    uint8_t adv_hdl;
    app_lea_adv_state_e state;
} app_lea_adv_ctrl_t;

typedef struct {
    uint8_t volume;    /* Volume value, Range 0 ~ 255 */
    uint8_t mute;      /* Mute state, 0:mute  1:unmute*/
    uint8_t vol_flags; /* Volume setting persisted, 0:Reset Volume Setting  1:User Set Volume Setting */
} app_lea_volume_state_t;

static app_lea_volume_state_t g_le_vol_state = {0};
static uint16_t g_le_audio_adv_interval_min = APP_LE_AUDIO_ADV_INTERVAL_MIN_S;
static uint16_t g_le_audio_adv_interval_max = APP_LE_AUDIO_ADV_INTERVAL_MAX_S;
static app_lea_adv_ctrl_t g_lea_adv_ctrl = {0};

//Extern functions
extern bt_status_t bt_le_audio_sink_set_adv_handle(uint8_t adv_handle);


static bool _app_le_is_lea_adv_handle(uint8_t handle)
{
    bool ret = false;
    app_bt_utils_mutex_lock();
    if (g_lea_adv_ctrl.adv_hdl == handle)
        ret = true;
    app_bt_utils_mutex_unlock();
    return ret;
}

static void _app_le_set_adv_ctrl_state(uint8_t handle, app_lea_adv_state_e state)
{
    app_bt_utils_mutex_lock();
    do {
        if (state == LEA_ADV_STATE_CONFIGING &&
            g_lea_adv_ctrl.adv_hdl == 0) {
            g_lea_adv_ctrl.adv_hdl = handle;
            bt_le_audio_sink_set_adv_handle(handle);
        } else if (g_lea_adv_ctrl.adv_hdl != handle) {
            //not expected handle id
            APPS_LOG_MSGID_I(APP_LOG_TAG", not le adv hdl (0x%x, exp: 0x%x)", 2, handle, g_lea_adv_ctrl.adv_hdl);
            break;
        } else if (state == LEA_ADV_STATE_DISABLED) {
            g_lea_adv_ctrl.adv_hdl = 0;
            bt_le_audio_sink_set_adv_handle(0);
        }
        g_lea_adv_ctrl.state = state;
    } while (0);
    app_bt_utils_mutex_unlock();
}

static app_lea_adv_state_e _app_le_get_adv_ctrl_state(uint8_t handle)
{
    app_lea_adv_state_e state;

    app_bt_utils_mutex_lock();
    state = g_lea_adv_ctrl.state;
    app_bt_utils_mutex_unlock();
    return state;
}

#ifdef APPS_LEA_USE_LEGACY_ADV
static void _app_le_audio_get_scan_response(bt_gap_le_set_ext_scan_response_data_t *scan_rsp)
{
    uint16_t device_name_len = 0;

    if (scan_rsp->data_length < APP_LE_AUDIO_ADV_SCAN_RSP_LEN_MAX) {
        APPS_LOG_MSGID_E(APP_LOG_TAG", ERR: pls check scan rsp len %d < %d", 2, scan_rsp->data_length, APP_LE_AUDIO_ADV_SCAN_RSP_LEN_MAX);
        return;
    }

    char device_name[BT_GAP_LE_MAX_DEVICE_NAME_LENGTH] = {0};
    bt_bd_addr_t *local_addr = bt_device_manager_get_local_address();

    snprintf(device_name, BT_GAP_LE_MAX_DEVICE_NAME_LENGTH, "MTK_LEA_%.2X%.2X%.2X%.2X%.2X%.2X",
             (*local_addr)[5], (*local_addr)[4], (*local_addr)[3],
             (*local_addr)[2], (*local_addr)[1], (*local_addr)[0]);

    device_name_len = strlen((char *)device_name);

    /* scan_rsp: AD_TYPE_NAME_COMPLETE */
    scan_rsp->data[0] = device_name_len + 1;
    scan_rsp->data[1] = BT_GAP_LE_AD_TYPE_NAME_COMPLETE;
    memcpy(&scan_rsp->data[2], device_name, device_name_len);
    scan_rsp->data_length = device_name_len + 2;

    APPS_LOG_MSGID_I(APP_LOG_TAG", get_scan_rsp, dev_name:%s", 1, device_name);

    /* set GATT GAP service device name */
    //(Titan no src)bt_gatts_service_set_le_audio_device_name((const uint8_t *)device_name, device_name_len);
}
#endif /* #ifdef APPS_LEA_USE_LEGACY_ADV */

static int32_t _app_le_audio_get_adv_data(multi_ble_adv_info_t *adv_data)
{
    //LE_AUDIO_MSGLOG_I(LOG_TAG"[APP] app_le_audio_get_adv_data", 0);

    /* SCAN RSP */
    if (NULL != adv_data->scan_rsp) {
#ifdef APPS_LEA_USE_LEGACY_ADV
        _app_le_audio_get_scan_response(adv_data->scan_rsp);
#endif /* #ifdef APPS_LEA_USE_LEGACY_ADV */
    }

    /* ADV DATA */
    if ((NULL != adv_data->adv_data) && (NULL != adv_data->adv_data->data)) {
        uint16_t sink_conent, source_conent;
        uint8_t rsi[6];
        uint8_t len = 0;

        if (adv_data->adv_data->data_length < APP_LE_AUDIO_ADV_DATA_LEN_MAX) {
            APPS_LOG_MSGID_E(APP_LOG_TAG", ERR: pls check adv data len %d < %d", 2,
                             adv_data->adv_data->data_length, APP_LE_AUDIO_ADV_DATA_LEN_MAX);
            return -1;
        }

        adv_data->adv_data->data[len] = 2;
        adv_data->adv_data->data[len + 1] = BT_GAP_LE_AD_TYPE_FLAG;
        adv_data->adv_data->data[len + 2] = BT_GAP_LE_AD_FLAG_BR_EDR_NOT_SUPPORTED |
                                            BT_GAP_LE_AD_FLAG_GENERAL_DISCOVERABLE;
        len += 3;

        /* adv_data: RSI (Resolvable Set Identifier) */
        adv_data->adv_data->data[len] = 7;
        adv_data->adv_data->data[len + 1] = 0x2E;
        ble_csis_get_rsi(rsi);
        memcpy(&adv_data->adv_data->data[len + 2], rsi, sizeof(rsi));
        len += 8;

        /* adv_data: AD_TYPE_SERVICE_DATA (BAP)*/
        adv_data->adv_data->data[len] = 9;
        adv_data->adv_data->data[len + 1] = BT_GAP_LE_AD_TYPE_SERVICE_DATA;
        /* ASCS UUID: 2 bytes */
        adv_data->adv_data->data[len + 2] = (BT_GATT_UUID16_ASCS_SERVICE & 0x00FF);
        adv_data->adv_data->data[len + 3] = ((BT_GATT_UUID16_ASCS_SERVICE & 0xFF00) >> 8);
        /* Announcement Type: 1 bytes */
        adv_data->adv_data->data[len + 4] = ANNOUNCEMENT_TYPE_GENERAL;
        /* Available Audio Contect: 4 bytes */
        ble_pacs_get_available_audio_contexts(&sink_conent, &source_conent);
        memcpy(&adv_data->adv_data->data[len + 5], &sink_conent, 2);
        memcpy(&adv_data->adv_data->data[len + 7], &source_conent, 2);
        adv_data->adv_data->data[len + 9] = 0x00; /* Length of the Metadata field = 0 */
        len += 10;

        /* adv_data: TX_POWER (BAP)*/
        adv_data->adv_data->data[len] = 2;
        adv_data->adv_data->data[len + 1] = BT_GAP_LE_AD_TYPE_TX_POWER;
        adv_data->adv_data->data[len + 2] = 0x7F;
        len += 3;

        /* adv_data: AD_TYPE_APPEARANCE (TMAP) */
        adv_data->adv_data->data[len] = 3;
        adv_data->adv_data->data[len + 1] = BT_GAP_LE_AD_TYPE_APPEARANCE;
        /* value: 2 bytes (EX: 0x0941 = earbud, 0x0841 = audio sink*/
        adv_data->adv_data->data[len + 2] = 0x41;
        adv_data->adv_data->data[len + 3] = 0x08;
        len += 4;

        /* adv_data: AD_TYPE_SERVICE_DATA (TMAS)*/
        adv_data->adv_data->data[len] = 5;
        adv_data->adv_data->data[len + 1] = BT_GAP_LE_AD_TYPE_SERVICE_DATA;
        /* TMAS UUID: 2 bytes */
        adv_data->adv_data->data[len + 2] = (BT_SIG_UUID16_TMAS & 0x00FF);
        adv_data->adv_data->data[len + 3] = ((BT_SIG_UUID16_TMAS & 0xFF00) >> 8);
        /* TMAS Data: 2 bytes */
        ble_tmap_role_t role = ble_tmas_get_role();
        adv_data->adv_data->data[len + 4] = (role & 0x00FF);
        adv_data->adv_data->data[len + 5] = ((role & 0xFF00) >> 8);
        len += 6;

#ifdef MTK_LE_AUDIO_BMR
        /* adv_data: AD_TYPE_SERVICE_DATA (BASS)*/
        adv_data->adv_data->data[len] = 3;
        adv_data->adv_data->data[len + 1] = BT_GAP_LE_AD_TYPE_SERVICE_DATA;
        /* BASS UUID: 2 bytes */
        adv_data->adv_data->data[len + 2] = (BT_SIG_UUID16_BASS & 0x00FF);
        adv_data->adv_data->data[len + 3] = ((BT_SIG_UUID16_BASS & 0xFF00) >> 8);
        len += 4;
#endif /* #ifdef MTK_LE_AUDIO_BMR */

        /* adv_data: AD_TYPE_SERVICE_DATA (CAS)*/
        adv_data->adv_data->data[len] = 4;
        adv_data->adv_data->data[len + 1] = BT_GAP_LE_AD_TYPE_SERVICE_DATA;
        /* CAS UUID: 2 bytes */
        adv_data->adv_data->data[len + 2] = (BT_SIG_UUID16_CAS & 0x00FF);
        adv_data->adv_data->data[len + 3] = ((BT_SIG_UUID16_CAS & 0xFF00) >> 8);
        adv_data->adv_data->data[len + 4] = ANNOUNCEMENT_TYPE_GENERAL;
        len += 5;

#ifndef APPS_LEA_USE_LEGACY_ADV
        uint16_t device_name_len = 0;

        char device_name[BT_GAP_LE_MAX_DEVICE_NAME_LENGTH] = {0};
        bt_bd_addr_t *local_addr = bt_device_manager_get_local_address();

        snprintf(device_name, BT_GAP_LE_MAX_DEVICE_NAME_LENGTH, "MTK_LEA_%.2X%.2X%.2X%.2X%.2X%.2X",
                 (*local_addr)[5], (*local_addr)[4], (*local_addr)[3],
                 (*local_addr)[2], (*local_addr)[1], (*local_addr)[0]);

        device_name_len = strlen((char *)device_name);

        /* scan_rsp: AD_TYPE_NAME_COMPLETE*/
        adv_data->adv_data->data[len] = device_name_len + 1;
        adv_data->adv_data->data[len + 1] = BT_GAP_LE_AD_TYPE_NAME_COMPLETE;
        memcpy(&adv_data->adv_data->data[len + 2], device_name, device_name_len);

        /*set GASTT GAP service device name*/
        //bt_gatts_service_set_le_audio_device_name((const uint8_t *)device_name, device_name_len);

        len += 2 + device_name_len;
#endif /* #ifndef APPS_LEA_USE_LEGACY_ADV */

        if (len > APP_LE_AUDIO_ADV_DATA_LEN_MAX) {
            //serious error
            APPS_LOG_MSGID_E(APP_LOG_TAG", ERR: pls check code, data may overlapped len %d > %d", 2,
                             len, APP_LE_AUDIO_ADV_DATA_LEN_MAX);
            return -2;
        }

        adv_data->adv_data->data_length = len;
    }

    if (NULL != adv_data->adv_param) {
#ifdef APPS_LEA_USE_LEGACY_ADV
        //legacy PDU
        adv_data->adv_param->advertising_event_properties = BT_HCI_ADV_EVT_PROPERTIES_MASK_CONNECTABLE
                                                            | BT_HCI_ADV_EVT_PROPERTIES_MASK_SCANNABLE
                                                            | BT_HCI_ADV_EVT_PROPERTIES_MASK_LEGACY_PDU;
#else /* #ifdef APPS_LEA_USE_LEGACY_ADV */
        //AE -- shall not be both connectable and scannable
        adv_data->adv_param->advertising_event_properties = BT_HCI_ADV_EVT_PROPERTIES_MASK_CONNECTABLE;
#endif /* #ifdef APPS_LEA_USE_LEGACY_ADV */

        /* Interval should be no larger than 100ms when discoverable */
        adv_data->adv_param->primary_advertising_interval_min = g_le_audio_adv_interval_min;
        adv_data->adv_param->primary_advertising_interval_max = g_le_audio_adv_interval_max;
        adv_data->adv_param->primary_advertising_channel_map = 0x07;
#ifdef APP_LEA_ADV_USE_RANDOM_ADDR
        adv_data->adv_param->own_address_type = BT_ADDR_RANDOM;
#else /* #ifdef APP_LEA_ADV_USE_RANDOM_ADDR */
        adv_data->adv_param->own_address_type = BT_ADDR_PUBLIC;
#endif /* #ifdef APP_LEA_ADV_USE_RANDOM_ADDR */
        adv_data->adv_param->advertising_filter_policy = 0;
        adv_data->adv_param->advertising_tx_power = 0x7F; //0x7F = Host has no preference.
        adv_data->adv_param->primary_advertising_phy = BT_HCI_LE_ADV_PHY_1M;
        adv_data->adv_param->secondary_advertising_phy = BT_HCI_LE_ADV_PHY_1M;
    }
    return 0;
}

static bt_status_t _app_le_audio_config_adv_data(uint8_t adv_hdl)
{
    uint8_t adv_raw[APP_LE_AUDIO_ADV_DATA_LEN_MAX] = "DDDDDHUMMINGBIRD_ADV_DATA";
    uint8_t scan_raw[APP_LE_AUDIO_ADV_SCAN_RSP_LEN_MAX] = {0};

    bt_hci_le_set_ext_advertising_parameters_t ext_adv_para = {0};

    bt_gap_le_set_ext_advertising_data_t ext_data = {0};
    ext_data.data_length = sizeof(adv_raw);
    ext_data.data = adv_raw;
    ext_data.fragment_preference = 0;

    bt_gap_le_set_ext_scan_response_data_t ext_scan = {0};
    ext_scan.data_length = sizeof(scan_raw);
    ext_scan.data = scan_raw;
    ext_scan.fragment_preference = 0;

    multi_ble_adv_info_t adv_info;
    adv_info.adv_param = &ext_adv_para;
    adv_info.adv_data = &ext_data;
    adv_info.scan_rsp = &ext_scan;

    if (_app_le_audio_get_adv_data(&adv_info) != 0) {
        return BT_STATUS_FAIL;
    }
#ifdef APP_LEA_ADV_USE_RANDOM_ADDR
    return bt_gap_le_config_extended_advertising(adv_hdl, bt_gap_le_get_random_address(),
                                                 &ext_adv_para, &ext_data, &ext_scan);
#else /* #ifdef APP_LEA_ADV_USE_RANDOM_ADDR */
    return bt_gap_le_config_extended_advertising(adv_hdl, NULL, &ext_adv_para, &ext_data, &ext_scan);
#endif /* #ifdef APP_LEA_ADV_USE_RANDOM_ADDR */
}

static void _app_le_audio_enable_advertising(uint8_t handle)
{
    bt_hci_le_set_ext_advertising_enable_t ext_adv_enable = {0};
    bt_status_t status;
    ext_adv_enable.enable = BT_HCI_ENABLE;
    ext_adv_enable.duration = 0;  // 0: always, time: dur * 10ms

    _app_le_set_adv_ctrl_state(handle, LEA_ADV_STATE_ENABLING);
    status = bt_gap_le_enable_extended_advertising(handle, &ext_adv_enable);
    if (status != BT_STATUS_SUCCESS) {
        APPS_LOG_MSGID_I(APP_LOG_TAG", start adv fail (%d)", 1, status);
        _app_le_set_adv_ctrl_state(handle, LEA_ADV_STATE_DISABLED);
    }
}

static void _app_le_audio_start_advertising(void)
{
    APPS_LOG_MSGID_I(APP_LOG_TAG", start adv", 0);

    uint8_t adv_hdl = MULTI_ADV_INST_ID_LEA;
    app_lea_adv_state_e adv_state = _app_le_get_adv_ctrl_state(adv_hdl);
    bt_status_t status;

    if (LEA_ADV_STATE_DISABLED != adv_state) {
        APPS_LOG_MSGID_E(APP_LOG_TAG", please disable lea adv b4 start (state = %d)", 1, adv_state);
        return;
    }

    _app_le_set_adv_ctrl_state(adv_hdl, LEA_ADV_STATE_CONFIGING);
    status = _app_le_audio_config_adv_data(adv_hdl);
    if (status != BT_STATUS_SUCCESS) {
        APPS_LOG_MSGID_I(APP_LOG_TAG", start adv fail (%d)", 1, status);
        _app_le_set_adv_ctrl_state(adv_hdl, LEA_ADV_STATE_DISABLED);
    }
}

static void _app_le_audio_stop_advertising(void)
{
    APPS_LOG_MSGID_I(APP_LOG_TAG", stop adv", 0);

    bt_hci_le_set_ext_advertising_enable_t ext_adv_enable = {0};
    uint8_t adv_hdl = MULTI_ADV_INST_ID_LEA;
    app_lea_adv_state_e adv_state = _app_le_get_adv_ctrl_state(adv_hdl);
    bt_status_t status;

    if (adv_state == LEA_ADV_STATE_DISABLED)
        return;

    ext_adv_enable.enable = BT_HCI_DISABLE;
    ext_adv_enable.duration = 0;

    _app_le_set_adv_ctrl_state(adv_hdl, LEA_ADV_STATE_DISABLING);
    status = bt_gap_le_enable_extended_advertising(adv_hdl, &ext_adv_enable);
    if (status != BT_STATUS_SUCCESS) {
        APPS_LOG_MSGID_I(APP_LOG_TAG", stop adv fail (%d)", 1, status);
        _app_le_set_adv_ctrl_state(adv_hdl, LEA_ADV_STATE_DISABLED);
    }
}

static void _app_le_audio_discover_mcp_callback(bt_handle_t handle, bool support_gmcs)
{
    APPS_LOG_MSGID_I(APP_LOG_TAG", MCP is %s in peer device, hanle:0x%x", 2,
                     support_gmcs ? "supported" : "not supported", handle);
}

static bt_status_t _app_le_audio_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{
    /*
    [Caution] This event callback is running on BT_Task.
              If you have some control variables on UI Shell task, please consider thread protection.
    */
    switch (msg) {
        /* GAP */
        case BT_GAP_LE_DISCONNECT_IND: {
                break;
            }

        case BT_GAP_LE_CIS_ESTABLISHED_IND: {
                bt_gap_le_cis_established_ind_t *ind = (bt_gap_le_cis_established_ind_t *) buff;
                APPS_LOG_MSGID_I(APP_LOG_TAG", BT_GAP_LE_CIS_ESTABLISHED_IND, connHdl:0x%x", 1, ind->connection_handle);
                //app_le_audio_set_sniff(false);
                break;
            }

        case BT_GAP_LE_CIS_TERMINATED_IND: {
                bt_gap_le_cis_terminated_ind_t *ind = (bt_gap_le_cis_terminated_ind_t *) buff;
                APPS_LOG_MSGID_I(APP_LOG_TAG", BT_GAP_LE_CIS_TERMINATED_IND, connHdl:0x%x", 1, ind->connection_handle);
                //app_le_audio_set_sniff(true);
                break;
            }

        case BT_GAP_LE_BIG_SYNC_LOST_IND: {
                APPS_LOG_MSGID_I(APP_LOG_TAG", BT_GAP_LE_BIG_SYNC_LOST_IND", 0);
                break;
            }
        case BT_GAP_LE_ADVERTISING_SET_TERMINATED_IND: {
                bt_gap_le_advertising_set_terminated_ind_t *ind = (bt_gap_le_advertising_set_terminated_ind_t *)buff;
                APPS_LOG_MSGID_I(APP_LOG_TAG", ADV_SET_TERMINATED, handle:%x adv_handle:%x", 2, ind->connection_handle, ind->handle);
                if (_app_le_is_lea_adv_handle(ind->handle)) {
                    _app_le_set_adv_ctrl_state(ind->handle, LEA_ADV_STATE_DISABLED);
                }
                /*
                if (ind->handle == bt_le_audio_sink_get_adv_handle()) {
                    s_le_audio_adv_enabled = false;
                    app_le_audio_set_connection(ind->connection_handle);
                    app_le_audio_start_advertising(0);
                    app_le_audio_set_connected_flag();
                }*/
                break;
            }
        case BT_GAP_LE_CONFIG_EXTENDED_ADVERTISING_CNF: {
                bt_gap_le_config_extended_advertising_cnf_t *cnf = (bt_gap_le_config_extended_advertising_cnf_t *)buff;
                APPS_LOG_MSGID_I(APP_LOG_TAG", LE_CONFIG_EXTENDED_ADVERTISING_CNF, handle:%x", 1, cnf->handle);
                if (_app_le_is_lea_adv_handle(cnf->handle)) {
                    if (status == BT_STATUS_SUCCESS) {
                        _app_le_audio_enable_advertising(cnf->handle);
                    } else {
                        _app_le_set_adv_ctrl_state(cnf->handle, LEA_ADV_STATE_DISABLED);
                    }
                }

                break;
            }
        case BT_GAP_LE_ENABLE_EXTENDED_ADVERTISING_CNF: {
                bt_gap_le_enable_extended_advertising_cnf_t *cnf = (bt_gap_le_enable_extended_advertising_cnf_t *)buff;
                APPS_LOG_MSGID_I(APP_LOG_TAG", LE_ENABLE_EXTENDED_ADVERTISING_CNF, handle:%x enable:%x", 2, cnf->handle, cnf->enable);
                if (_app_le_is_lea_adv_handle(cnf->handle)) {
                    if (status == BT_STATUS_SUCCESS && cnf->enable == BT_HCI_ENABLE) {
                        _app_le_set_adv_ctrl_state(cnf->handle, LEA_ADV_STATE_ENABLED);
                    } else {
                        _app_le_set_adv_ctrl_state(cnf->handle, LEA_ADV_STATE_DISABLED);
                    }
                }
                /*
                if (cnf->handle == bt_le_audio_sink_get_adv_handle()) {
                    if (cnf->enable) {
                        s_le_audio_adv_enabled = true;
                    } else {
                        s_le_audio_adv_enabled = false;
                    }

                }*/
                break;
            }

        case BT_GATTC_READ_CHARC:
        case BT_GATTC_WRITE_CHARC:
        case BT_GATTC_CHARC_VALUE_NOTIFICATION: {
                //app_le_audio_aird_client_event_handler(msg, status, buff);
                break;
            }
        default:
            break;
    }

    return BT_STATUS_SUCCESS;
}

#ifdef MTK_NVDM_ENABLE
static void _app_le_audio_volume_setting_store_complete_cb(nvkey_status_t status, void *user_data)
{
    if (status != NVKEY_STATUS_OK)
        APPS_LOG_MSGID_E(APP_LOG_TAG", Store Volume setting FAIL!, status:%x", 1, status);
}
#endif /* #ifdef MTK_NVDM_ENABLE */

static void _app_le_audio_le_load_volume_setting(void)
{
#ifdef MTK_NVDM_ENABLE
    uint32_t size = (uint32_t)sizeof(app_lea_volume_state_t);
    nvdm_status_t status;

    /* Read volume setting from NVDM */
    status = nvdm_read_data_item(NVDM_GROUP_BT_VOL, NVDM_ITEM_BT_VOL, (uint8_t *)&g_le_vol_state, &size);

    if (status != NVDM_STATUS_OK) {
        APPS_LOG_MSGID_E(APP_LOG_TAG", Read Volume setting FAIL!, status:%x", 1, status);
#endif /* #ifdef MTK_NVDM_ENABLE */
        g_le_vol_state.volume     = APP_LEA_DEFAULT_VOLUME;
        g_le_vol_state.mute       = APP_LEA_DEFAULT_MUTE;
        g_le_vol_state.vol_flags  = APP_LEA_VOLUME_STATE_FLAGS_NOT_PERSISTED;
#ifdef MTK_NVDM_ENABLE
    }
#endif /* #ifdef MTK_NVDM_ENABLE */
}

static bt_status_t _app_le_audio_disconnect(const bt_addr_t *bt_addr)
{
    bt_handle_t conn_hdl = BT_HANDLE_INVALID;
    bt_hci_cmd_disconnect_t param;
    /*
     * The connection information for LEA link may not align in sink and DM,
     * so we will check DM first, if check fail, we will check the information in sink.
     */
#ifdef MTK_BLE_DM_SUPPORT
    conn_hdl = bt_device_manager_le_get_hdl_by_addr(bt_addr);
#endif /* #ifdef MTK_BLE_DM_SUPPORT */
    if (conn_hdl == BT_HANDLE_INVALID) {
        conn_hdl = bt_sink_srv_cap_get_hdl_by_addr(bt_addr);
        if (conn_hdl == BT_HANDLE_INVALID) {
            APPS_LOG_MSGID_E(APP_LOG_TAG", This may not lea link. addr(%d)=0%x-%x-%x-%x-%x-%x",
                             7, bt_addr->type,
                             bt_addr->addr[5], bt_addr->addr[4], bt_addr->addr[3],
                             bt_addr->addr[2], bt_addr->addr[1], bt_addr->addr[0]);
            return BT_STATUS_FAIL;
        }
    }

    param.connection_handle = conn_hdl;
    param.reason = BT_HCI_STATUS_REMOTE_USER_TERMINATED_CONNECTION;
    return bt_gap_le_disconnect(&param);
}

#if 0 // Move to app_bt_conn_componet_in_homescreen.c
static char *_app_le_link_state_parser(bt_le_link_state_t status)
{
    switch (status) {
        case BT_BLE_LINK_DISCONNECTED:
            return "Disconnected";
        case BT_BLE_LINK_DISCONNECTING:
            return "Disconnecting";
        case BT_BLE_LINK_CONNECTING:
            return "Connecting";
        case BT_BLE_LINK_CONNECTED:
            return "Connected";
        default:
            return "Unknown";
    }
}

static bool _proc_bt_sink_event_group(ui_shell_activity_t *self, uint32_t event_id,
                                      void *extra_data, size_t data_len)
{
    bool ret = false;

    switch (event_id) {
        case LE_SINK_SRV_EVENT_REMOTE_INFO_UPDATE: {
                bt_le_sink_srv_event_remote_info_update_t *info = (bt_le_sink_srv_event_remote_info_update_t *)extra_data;
                if (!info) {
                    break;
                }
                /* Print BT_CM update log. */
                APPS_LOG_MSGID_I(APP_LOG_TAG", LE Link State:%s, Reason:0x%x, Profile:%x->%x, Addr:%02x:%02x:%02x:%02x:%02x:%02x",
                                 10, _app_le_link_state_parser(info->state), info->reason,
                                 info->pre_connected_service, info->connected_service,
                                 info->address[5], info->address[4], info->address[3],
                                 info->address[2], info->address[1], info->address[0]);
            }
            break;
        default:
            break;
    }

    return ret;
}
#endif /* #if 0 // Move to app_bt_conn_componet_in_homescreen.c */

static bool _proc_key_event_group(ui_shell_activity_t *self, uint32_t event_id,
                                  void *extra_data, size_t data_len)
{
    apps_config_key_action_t action = event_id & 0xFFFF;
    bool ret = true; //ret true means this key will not process by others

    switch (action) {
        case KEY_LE_AUDIO_START_ADV: {
#if defined(MTK_BT_A2DP_ENABLE) && defined(MTK_BT_CM_SUPPORT)
                bt_bd_addr_t addr = {0};
#endif /* #if defined(MTK_BT_A2DP_ENABLE) && defined(MTK_BT_CM_SUPPORT) */

                /* If LEA connected, return. */
                if (bt_sink_srv_cap_check_links_state(BT_SINK_SRV_CAP_STATE_CONNECTED) != BT_HANDLE_INVALID) {
                    APPS_LOG_MSGID_I(APP_LOG_TAG" Cannot start LEA adv when LEA connected", 0);
                    break;
                }
#if defined(MTK_BT_A2DP_ENABLE) && defined(MTK_BT_CM_SUPPORT)
                /* If A2DP connected, return.*/
                else if (bt_cm_get_connected_devices(BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_A2DP_SINK), &addr, 1)) {
                    APPS_LOG_MSGID_I(APP_LOG_TAG" Cannot start LEA adv when A2DP connected", 0);
                    break;
                }
#endif /* #if defined(MTK_BT_A2DP_ENABLE) && defined(MTK_BT_CM_SUPPORT) */

                _app_le_audio_start_advertising();
                break;
            }
        case KEY_LE_AUDIO_STOP_ADV:
            _app_le_audio_stop_advertising();
            break;
        case KEY_LE_AUDIO_DISCONNECT: {
                bt_addr_t bt_addr;
                memcpy(&bt_addr, extra_data, sizeof(bt_addr_t));
                _app_le_audio_disconnect(&bt_addr);
                break;
            }
        case KEY_LE_AUDIO_UNPAIR: {
                bt_addr_t bt_addr;
                memcpy(&bt_addr, extra_data, sizeof(bt_addr_t));
                _app_le_audio_disconnect(&bt_addr);
                bt_device_manager_le_remove_bonded_device(&bt_addr);
                break;
            }
        case KEY_LE_AUDIO_RESET_PAIRED_LIST:
            //note: This is a special reset action, so we will not do disconnect all here. (depend on spec.)
            bt_device_manager_le_clear_all_bonded_info();
            break;
        case KEY_LE_AUDIO_PAIRED_LIST:
            bt_device_manager_le_show_bonding_info();
            break;
        case KEY_LE_AUDIO_CONN_LIST:
            bt_sink_srv_cap_show_link_info(); //Show conntion info only on le audio link.
            break;
        default:
            ret = false;
            break;
    }
    return ret;
}

bool app_le_audio_idle_activity_proc(struct _ui_shell_activity *self, uint32_t event_group, uint32_t event_id, void *extra_data, size_t data_len)
{
    bool ret = false;

    //APPS_LOG_MSGID_I(APP_LOG_TAG", IDLE_ACT, event_group : %x, id: %x", 2, event_group, event_id);

    switch (event_group) {
        case EVENT_GROUP_UI_SHELL_KEY: {
                /* Key event. */
                ret = _proc_key_event_group(self, event_id, extra_data, data_len);
                break;
            }
        //case EVENT_GROUP_UI_SHELL_BT_SINK: {
        //        ret = _proc_bt_sink_event_group(self, event_id, extra_data, data_len);
        //        break;
        //    }
        //case EVENT_GROUP_UI_SHELL_BT: {
        // Compare to "app_le_audio_event_callback()", this is running in UI Shell task.
        //    ret = app_le_audio_proc_bt_group(self, event_id, extra_data, data_len);
        //    break;
        //}
        //case EVENT_GROUP_UI_SHELL_LE_AUDIO: {
        //    ret = app_le_audio_proc(self, event_id, extra_data, data_len);
        //    break;
        //}
        default:
            break;
    }

    return ret;
}

void app_le_audio_switch_advertising(bool enable)
{
    if (enable)
        _app_le_audio_start_advertising();
    else
        _app_le_audio_stop_advertising();
}

void app_le_audio_dump_volume_setting(void)
{
    APPS_LOG_MSGID_I(APP_LOG_TAG" Volume State", 0);
    APPS_LOG_MSGID_I(APP_LOG_TAG" Volume : 0x%02x", 1, g_le_vol_state.volume);
    APPS_LOG_MSGID_I(APP_LOG_TAG" Mute   :    %d", 1, g_le_vol_state.mute);
    APPS_LOG_MSGID_I(APP_LOG_TAG" Flags  :    %d", 1, g_le_vol_state.vol_flags);
}

void app_le_audio_store_volume_setting(uint8_t volume, uint8_t mute)
{
    g_le_vol_state.volume     = volume;
    g_le_vol_state.mute       = mute;
    g_le_vol_state.vol_flags  = APP_LEA_VOLUME_STATE_FLAGS_PERSISTED;

    app_le_audio_dump_volume_setting();

#ifdef MTK_NVDM_ENABLE
    nvdm_write_data_item_non_blocking(NVDM_GROUP_BT_VOL, NVDM_ITEM_BT_VOL, NVDM_DATA_ITEM_TYPE_RAW_DATA,
                                      (const uint8_t *)&g_le_vol_state, sizeof(app_lea_volume_state_t),
                                      (nvdm_user_callback_t)_app_le_audio_volume_setting_store_complete_cb, NULL);
#endif /* #ifdef MTK_NVDM_ENABLE */
}

void app_le_audio_power_on_setup(void)
{
    bt_sink_srv_cap_am_init();

    /* reset it when power on */
    memset(&g_lea_adv_ctrl, 0, sizeof(app_lea_adv_ctrl_t));
}

void app_le_audio_power_off_setup(void)
{
    // reset le audio data
    bt_sink_srv_cap_am_deinit();
}

void app_le_audio_init(void)
{
    APPS_LOG_MSGID_I(APP_LOG_TAG", %s", 1, __func__);

    _app_le_audio_le_load_volume_setting();

    le_sink_srv_init(APP_LE_AUDIO_MAX_LINK_NUM);

    bt_callback_manager_register_callback(bt_callback_type_app_event, MODULE_MASK_GAP | MODULE_MASK_SYSTEM | MODULE_MASK_GATT, (void *)_app_le_audio_event_callback);

    //app_le_audio_reset_connection();

    //app_ccp_init(_app_le_audio_discover_ccp_callback);
    app_mcp_init(_app_le_audio_discover_mcp_callback);

#ifdef AIR_LE_AUDIO_BIS_ENABLE
    //app_le_audio_bis_init();
#endif /* #ifdef AIR_LE_AUDIO_BIS_ENABLE */
}

#ifdef MTK_COMMON_LE_VCP_EVT
/* Used for Sink module to initialize volume setting */
bt_status_t bt_sink_srv_le_volume_get_init_setting(uint8_t *volume, uint8_t *mute, uint8_t *flags)
{
    *volume = g_le_vol_state.volume;
    *mute = g_le_vol_state.mute;
    *flags = g_le_vol_state.vol_flags;

    return BT_STATUS_SUCCESS;
}
#endif /* #ifdef MTK_COMMON_LE_VCP_EVT */
