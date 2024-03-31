/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
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

/*
        BLE SMART CONNECTION is able to receive the SSID, password and security mode from the APK by BLE connection, and then connect to the WIFI AP.
It support GCC, Keil and IAR compilation. The lib is in middleware/MTK/ble_smtcn. Please switch MTK_BLE_SMTCN_ENABLE to enable/disable this feature
in project/mt7697_hdk/apps/iot_sdk/GCC/feature.mk.


*************How to use BLE SMART CONNECTION*************

step1, Download the bin file to MT7697;

step2, Get the APK in SDK packet in tools/ble_smart_connection/ble_smart_connection.apk and install it on Android Smartphone(Android 4.3 or later);

step3, Make Smartphone connect to a WIFI AP;

step4, Connect MT7697 to the PC, and connect to ComPortLogger or putty, set speed to be 115200.

step5, Reset MT7697, input ATcommand "ble wifi smart" in ComPortLogger , ble smart connection will be started;

step6, Start the APK and search BLE device, select the MT7697 device named "BLE_SMTCN" to connect;

step7, Input the password of the connected WIFI AP and press "connect" button in the APK, MT7697 will start to connect to the same WIFI AP, and
        the result will show in the APK.
*/



/*BLE SMART CONNECTION compile option, switch it in feature.mk*/
#ifdef MTK_BLE_SMTCN_ENABLE
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "bt_gap_le.h"
#include "bt_gatts.h"
#include "bt_uuid.h"
#include "wifi_api_ex.h"
#include "lwip/netif.h"
#include "lwip/inet.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "lwip/dhcp.h"

#define BLE_SMTCN_ADDRESS               {0x0C, 0x01, 0x02, 0x03, 0x04, 0x05}
#define BLE_SMTCN_ADV_DATA              "DDDDDDDDDBLE_SMTCN"
#define BLE_SMTCN_ADV_DATA_LEN          31        /*MAX 31*/
#define BLE_SMTCN_DEVICE_NAME           "BLE_SMTCN"
#define BLE_SMTCN_SERVICE_UUID          (0x18AA)
#define BLE_SMTCN_MAX_INTERVAL          0x00C0    /*The range is from 0x0020 to 0x4000.*/
#define BLE_SMTCN_MIN_INTERVAL          0x00C0    /*The range is from 0x0020 to 0x4000.*/
#define BLE_SMTCN_CHANNEL_NUM           7
#define BLE_SMTCN_FILTER_POLICY         0
#define BLE_SMTCN_AD_FLAG_LEN           2
#define BLE_SMTCN_AD_UUID_LEN           3
#define BLE_SMTCN_AD_NAME_LEN           10        /*MAX 23*/
#define BLE_SMTCN_OTHER_LEN             9

#define BLE_SMTCN_SERVICE_UUID        (0x18AA) // Data Transfer Service
#define BLE_SMTCN_CHAR_UUID           (0x2AAA)
#define BLE_SMTCN_SSID_LEN            32
#define BLE_SMTCN_PW_LEN              64
#define BLE_SMTCN_IP_LEN              18
#define BLE_SMTCN_CHAR_VALUE_HANDLE   0x0016
#define BLE_SMTCN_MAX_DATA_LEN        64
#define BLE_SMTCN_TIMER_PERIOD       1000

typedef enum {
    BLE_SMTCN_WIFI_INFO_SSID = 0x01,
    BLE_SMTCN_WIFI_INFO_PW,
    BLE_SMTCN_WIFI_INFO_SEC_MODE,
    BLE_SMTCN_WIFI_INFO_IP,

    BLE_SMTCN_WIFI_INFO_DISCONNECTED = 0x07,
    BLE_SMTCN_WIFI_INFO_CONNECTED = 0x08
} ble_smtcn_wifi_info_id_t;

typedef struct {
    uint16_t conn_handle;
    uint16_t indicate_enable;

    char ip_addr[BLE_SMTCN_IP_LEN];
    uint8_t ssidrx[BLE_SMTCN_SSID_LEN];
    uint8_t ssidrx_len;
    uint8_t pwrx[BLE_SMTCN_PW_LEN];
    uint8_t pwrx_len;
    wifi_auth_mode_t authrx;
    wifi_encrypt_type_t encryptrx;
    uint16_t wifi_conn_enabled;

    TimerHandle_t dtp_timer;
    bool smtcn_started;
} ble_smtcn_context_t;

typedef struct {
    uint16_t conn_handle;
    uint8_t index;
    uint8_t expected_len;
    uint8_t buf[BLE_SMTCN_PW_LEN];
} ble_smtcn_buffer_t;

static ble_smtcn_buffer_t prepare_buf = {0};

static ble_smtcn_context_t ble_smtcn_cntx;
static ble_smtcn_context_t *p_smtcn = &ble_smtcn_cntx;
/*configration for DTP*/
const bt_uuid_t BLE_SMTCN_CHAR_UUID128 = BT_UUID_INIT_WITH_UUID16(BLE_SMTCN_CHAR_UUID);
static bool ble_smtcn_init_done = pdFALSE;

log_create_module(BLE_SMTCN, PRINT_LEVEL_INFO);


/*****************************************************************************
* FUNCTION
*  ble_smtcn_set_adv
* DESCRIPTION
*  Set ble smart connection ramdom address
* PARAMETERS
*  void
* RETURNS
*  void
*****************************************************************************/
void ble_smtcn_set_adv(void)
{
    bt_bd_addr_t addr = BLE_SMTCN_ADDRESS;
    bt_bd_addr_ptr_t random_addr = addr;

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s\n", __func__);

    bt_gap_le_set_random_address(random_addr);

    p_smtcn->smtcn_started = true;
}

/*****************************************************************************
* FUNCTION
*  ble_smtcn_start_adv
* DESCRIPTION
*  Start ble smart connection advertising
* PARAMETERS
*  char * name
*  int    name_len
* RETURNS
*  void
*****************************************************************************/
void ble_smtcn_start_adv(char *name, int name_len)
{
    bt_hci_cmd_le_set_advertising_enable_t enable;
    bt_hci_cmd_le_set_advertising_parameters_t adv_param = {
        .advertising_interval_min = BLE_SMTCN_MIN_INTERVAL,
        .advertising_interval_max = BLE_SMTCN_MAX_INTERVAL,
        .advertising_type = BT_HCI_ADV_TYPE_CONNECTABLE_UNDIRECTED,
        .own_address_type = BT_ADDR_PUBLIC,
        .advertising_channel_map = BLE_SMTCN_CHANNEL_NUM,
        .advertising_filter_policy = BLE_SMTCN_FILTER_POLICY
    };
    bt_hci_cmd_le_set_advertising_data_t adv_data = {
        .advertising_data_length = BLE_SMTCN_ADV_DATA_LEN,
        .advertising_data = BLE_SMTCN_ADV_DATA
    };
    int adv_name_len = BLE_SMTCN_AD_NAME_LEN;

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s\n", __func__);

    if (name && name_len > 0) {
        LOG_I(BLE_SMTCN, "[BLE_SMTCN]Set name as %s\n", name);
        name_len = name_len > BLE_SMTCN_ADV_DATA_LEN - BLE_SMTCN_OTHER_LEN - 1 ? BLE_SMTCN_ADV_DATA_LEN - BLE_SMTCN_OTHER_LEN - 1 : name_len;
        strncpy((char *)adv_data.advertising_data + BLE_SMTCN_OTHER_LEN, name, name_len);
        adv_data.advertising_data[BLE_SMTCN_OTHER_LEN + name_len] = '\0';
        adv_name_len = strlen((const char *)adv_data.advertising_data) - BLE_SMTCN_OTHER_LEN;
        adv_data.advertising_data_length = strlen((const char *)adv_data.advertising_data);
    }

    // If we have set random address, Change address type to random.
    if (p_smtcn->smtcn_started == true) {
        LOG_D(BLE_SMTCN, "[BLE_SMTCN] Change address type to random\n");
        adv_param.own_address_type = BT_ADDR_RANDOM;
    }

    adv_data.advertising_data[0] = BLE_SMTCN_AD_FLAG_LEN;
    adv_data.advertising_data[1] = BT_GAP_LE_AD_TYPE_FLAG;
    adv_data.advertising_data[2] = BT_GAP_LE_AD_FLAG_BR_EDR_NOT_SUPPORTED | BT_GAP_LE_AD_FLAG_GENERAL_DISCOVERABLE;
    adv_data.advertising_data[3] = BLE_SMTCN_AD_UUID_LEN;
    adv_data.advertising_data[4] = BT_GAP_LE_AD_TYPE_16_BIT_UUID_COMPLETE;
    adv_data.advertising_data[5] = BLE_SMTCN_SERVICE_UUID & 0x00FF;
    adv_data.advertising_data[6] = (BLE_SMTCN_SERVICE_UUID & 0xFF00) >> 8;
    adv_data.advertising_data[7] = adv_name_len;
    adv_data.advertising_data[8] = BT_GAP_LE_AD_TYPE_NAME_COMPLETE;

    enable.advertising_enable = BT_HCI_ENABLE;
    bt_gap_le_set_advertising(&enable, &adv_param, &adv_data, NULL);
}

/*****************************************************************************
* FUNCTION
*  ble_smtcn_event_callback
* DESCRIPTION
*  Deal with event from bt stack
* PARAMETERS
*  void
* RETURNS
*  void
*****************************************************************************/
void ble_smtcn_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{
    switch (msg) {
        case BT_GAP_LE_SET_RANDOM_ADDRESS_CNF: {
                LOG_I(BLE_SMTCN, "[BLE_SMTCN]start advertising\n");
                if (p_smtcn->smtcn_started == true) {
                    ble_smtcn_start_adv(NULL, 0);
                    p_smtcn->smtcn_started = false;
                }
            }
            break;

        default:
            break;
    }
}


void ble_smtcn_send_indication(ble_smtcn_wifi_info_id_t tag_id, uint8_t len, uint8_t *value)
{
    uint8_t buf[64] = {0};
    uint8_t pak[BLE_SMTCN_MAX_DATA_LEN + 2] = {0};
    uint8_t pak_len;
    bt_gattc_charc_value_notification_indication_t *req;
    bt_status_t send_status;

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: tag_id = %d\n", __func__, tag_id);

    pak[0] = tag_id;

    pak_len = 1;

    if (len > BLE_SMTCN_MAX_DATA_LEN) {
        LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: data length = %d, > 18!!!\n", __func__, len);
    }
    if (tag_id == BLE_SMTCN_WIFI_INFO_SSID || tag_id == BLE_SMTCN_WIFI_INFO_IP) {
        pak[1] = (len > BLE_SMTCN_MAX_DATA_LEN) ? BLE_SMTCN_MAX_DATA_LEN : len;
        memcpy(pak + 2, value, pak[1]);
        pak_len += pak[1] + 1;
    }

    req = (bt_gattc_charc_value_notification_indication_t *)buf;
    req->attribute_value_length = 3 + pak_len;
    req->att_req.opcode = BT_ATT_OPCODE_HANDLE_VALUE_INDICATION;
    req->att_req.handle = BLE_SMTCN_CHAR_VALUE_HANDLE;
    memcpy(req->att_req.attribute_value, pak, pak_len);
    send_status = bt_gatts_send_charc_value_notification_indication(p_smtcn->conn_handle, req);

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: send_status = %x\n", __func__, send_status);
}


void ble_smtcn_parse_data(uint8_t *data, uint16_t size)
{
    ble_smtcn_wifi_info_id_t tag_id;
    uint8_t tag_len;

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s\n", __func__);

    tag_id = (ble_smtcn_wifi_info_id_t)data[0];
    tag_len = data[1];

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: tag_id = %d, tag_len = %d\n", __func__, tag_id, tag_len);
    switch (tag_id) {
        case BLE_SMTCN_WIFI_INFO_SSID: {
                tag_len = tag_len > BLE_SMTCN_SSID_LEN ? BLE_SMTCN_SSID_LEN : tag_len;
                memset(p_smtcn->ssidrx, 0, BLE_SMTCN_SSID_LEN);
                memcpy(p_smtcn->ssidrx, data + 2, tag_len);
                p_smtcn->ssidrx_len = tag_len;
                p_smtcn->wifi_conn_enabled |= (1 << BLE_SMTCN_WIFI_INFO_SSID);
                LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: ssid = %s, wifi_conn_enabled = 0x%x, len = 0x%x\n",
                      __func__, p_smtcn->ssidrx, p_smtcn->wifi_conn_enabled, tag_len);
            }
            break;

        case BLE_SMTCN_WIFI_INFO_PW: {
                tag_len = tag_len > BLE_SMTCN_PW_LEN ? BLE_SMTCN_PW_LEN : tag_len;
                memset(p_smtcn->pwrx, 0, BLE_SMTCN_PW_LEN);
                memcpy(p_smtcn->pwrx, data + 2, tag_len);
                p_smtcn->pwrx_len = tag_len;
                p_smtcn->wifi_conn_enabled |= (1 << BLE_SMTCN_WIFI_INFO_PW);
                LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: pw = %s, wifi_conn_enabled = 0x%x, len = 0x%x\n",
                      __func__, p_smtcn->pwrx, p_smtcn->wifi_conn_enabled, tag_len);
            }
            break;

        case BLE_SMTCN_WIFI_INFO_SEC_MODE:
            if (tag_len == 2) {
                p_smtcn->authrx = (wifi_auth_mode_t)data[2];
                p_smtcn->encryptrx = (wifi_encrypt_type_t)data[3]; //0, 1, 4, 6, 8 are always used
                p_smtcn->wifi_conn_enabled |= (1 << BLE_SMTCN_WIFI_INFO_SEC_MODE);
                LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: auth = %d, encryt = %d, wifi_conn_enabled = 0x%x\n",
                      __func__, p_smtcn->authrx, p_smtcn->encryptrx, p_smtcn->wifi_conn_enabled);
            } else {
                LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: security mode is wrong, tag_len = %d\n", __func__, tag_len);
            }
            break;

        default:
            LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: wrong tag id\n", __func__, tag_id);
            break;
    }
}


void ble_smtcn_timeout_callback(TimerHandle_t xTimer)
{
    uint8_t link_status;
    uint8_t ssid[BLE_SMTCN_SSID_LEN] = {0};
    uint8_t ssid_len;

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s\n", __func__);

    if (wifi_connection_get_link_status(&link_status) >= 0) {
        if (link_status == WIFI_STATUS_LINK_CONNECTED) {
            if (wifi_config_get_ssid(0, ssid, &(ssid_len)) >= 0) {

                ble_smtcn_send_indication(BLE_SMTCN_WIFI_INFO_CONNECTED, 0, NULL);
                ble_smtcn_send_indication(BLE_SMTCN_WIFI_INFO_SSID, ssid_len, ssid);
                p_smtcn->ip_addr[BLE_SMTCN_IP_LEN - 1] = '\0';
                if (strlen(p_smtcn->ip_addr)) {
                    //send indication
                    ble_smtcn_send_indication(BLE_SMTCN_WIFI_INFO_IP, strlen(p_smtcn->ip_addr), (uint8_t *)(p_smtcn->ip_addr));

                    memset(p_smtcn->ip_addr, 0, BLE_SMTCN_IP_LEN);
                }
            } else {
                //get ssid fail
                LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: get ssid fail\n", __func__);
            }
        } else {
            //disconnected
            ble_smtcn_send_indication(BLE_SMTCN_WIFI_INFO_DISCONNECTED, 0, NULL);
        }
    }
}

void ble_smtcn_setup_wifi_connection(void)
{
    int32_t result;
    uint8_t port = WIFI_PORT_STA;

    result = wifi_config_set_ssid(port, p_smtcn->ssidrx, strlen((char *)(p_smtcn->ssidrx)));
    LOG_I(BLE_SMTCN, "[BLE_SMTCN]set ssid = %d\n", result);
    if (result < 0) {
        return;
    }

    result = wifi_config_set_security_mode(port, p_smtcn->authrx, p_smtcn->encryptrx);
    LOG_I(BLE_SMTCN, "[BLE_SMTCN]set security mode = %d, encrypt = %d\n", result, p_smtcn->encryptrx);
    if (result < 0) {
        return;
    }

    if (p_smtcn->encryptrx == 0) {
        wifi_wep_key_t wep_key;
        uint8_t key_id = 0;

        if (p_smtcn->pwrx_len == 5 || p_smtcn->pwrx_len == 10 || p_smtcn->pwrx_len == 13) {
            wep_key.wep_tx_key_index = key_id;

            memcpy(wep_key.wep_key[key_id], p_smtcn->pwrx, p_smtcn->pwrx_len);
            wep_key.wep_key_length[key_id] = p_smtcn->pwrx_len;

            result = wifi_config_set_wep_key(port, &wep_key); // wep encryption mode, the pw length can only be 5, 10 or 13
            LOG_I(BLE_SMTCN, "[BLE_SMTCN]set wep key = %d\n", result);
            if (result < 0) {
                return;
            }
        } else {
            LOG_I(BLE_SMTCN, "[BLE_SMTCN]invalid password length = %d\n", p_smtcn->pwrx_len);
        }
    } else {
        result = wifi_config_set_wpa_psk_key(port, p_smtcn->pwrx, p_smtcn->pwrx_len);//the passwd length should be 8-64
        LOG_I(BLE_SMTCN, "[BLE_SMTCN]set psk key = %d\n", result);
        if (result < 0) {
            return;
        }
    }

    result = wifi_config_reload_setting();
    LOG_I(BLE_SMTCN, "[BLE_SMTCN]reload = %d\n", result);
}

void ble_smtcn_charc_execute_write_callback(uint16_t handle)
{
    if (prepare_buf.conn_handle != handle) {
        LOG_E(BLE_SMTCN, "%s: handle not correct Expected 0x%x Actual 0x%x", __func__, prepare_buf.conn_handle, handle);
        return;
    }
    if (prepare_buf.expected_len == 0) {
        LOG_E(BLE_SMTCN, "%s: Did not have prepare write data", __func__);
        return;
    }

    if (prepare_buf.expected_len == prepare_buf.index) {
        ble_smtcn_parse_data(prepare_buf.buf, prepare_buf.index);
    } else {
        LOG_E(BLE_SMTCN, "%s: Unexpected Data! Expected %d  Actual %d", __func__, prepare_buf.expected_len, prepare_buf.index);
    }
    memset((void *)&prepare_buf, 0, sizeof(ble_smtcn_buffer_t));
}

static uint32_t ble_smtcn_charc_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    uint8_t link_status;
    int32_t ret = 0;
    uint8_t ssid[BLE_SMTCN_SSID_LEN] = {0};
    uint8_t ssid_len;
    uint8_t *p_data = (uint8_t *)data;
    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: rw = %d, handle = 0x%x, size = %d\n", __func__, rw, handle, size);

    if (rw == BT_GATTS_CALLBACK_WRITE) {
        ble_smtcn_parse_data(p_data, size);
    } else if (rw == BT_GATTS_CALLBACK_PREPARE_WRITE) {
        ble_smtcn_wifi_info_id_t tag_id;

        LOG_I(BLE_SMTCN, "%s: Prepare Write %s", __func__, p_data);

        if (prepare_buf.conn_handle == 0) {
            prepare_buf.conn_handle = handle;
            tag_id = (ble_smtcn_wifi_info_id_t)p_data[0];
            prepare_buf.expected_len = p_data[1];

            switch (tag_id) {
                case BLE_SMTCN_WIFI_INFO_SSID: {
                        if (prepare_buf.expected_len > BLE_SMTCN_SSID_LEN) {
                            LOG_E(BLE_SMTCN, "%s: SSID only supported less than %d bytes!", __func__, BLE_SMTCN_SSID_LEN);
                            return 0;
                        }
                    }
                    break;
                case BLE_SMTCN_WIFI_INFO_PW: {
                        if (prepare_buf.expected_len > BLE_SMTCN_PW_LEN) {
                            LOG_E(BLE_SMTCN, "%s: PASSWORD only supported less than %d bytes!", __func__, BLE_SMTCN_PW_LEN);
                            return 0;
                        }
                    }
                    break;
                default:
                    LOG_E(BLE_SMTCN, "%s: Unexpected Input %d!", __func__, tag_id);
                    return 0;
            }
            prepare_buf.expected_len += 2;
        } else if (prepare_buf.conn_handle != handle) {
            LOG_W(BLE_SMTCN, "%s: Prepare write for 0x%x not finished. So 0x%x sould not write.",
                  __func__, prepare_buf.conn_handle, handle);
            return 0;
        }

        memcpy(prepare_buf.buf + prepare_buf.index, data, size);
        prepare_buf.index += size;

        LOG_I(BLE_SMTCN, "%s: (%d,%d)", __func__, prepare_buf.expected_len, prepare_buf.index);
    } else {
        return 0;
    }

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: %x %x\n", __func__, p_smtcn->wifi_conn_enabled, p_smtcn->indicate_enable);
    if (!(p_smtcn->wifi_conn_enabled == 0x0E && p_smtcn->indicate_enable == 0x0002)) {
        LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: setting not complete yet 0x%x 0x%x\n", __func__, p_smtcn->wifi_conn_enabled, p_smtcn->indicate_enable);
        return (uint32_t)size;
    }

    //every tag is ready and indication is enabled
    ret = wifi_connection_get_link_status(&link_status);
    if (ret < 0) {
        LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: get wifi link status fail 0x%x\n", __func__, ret);
        goto exit;
    }

    if (link_status == WIFI_STATUS_LINK_CONNECTED) {
        LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: wifi connected\n", __func__);
        if (wifi_config_get_ssid(0, ssid, &(ssid_len)) < 0) {
            LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: get ssid fail\n", __func__);
            goto exit;
        }

        LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: The connected ssid id %s\n", __func__, ssid);
        if (!memcmp(ssid, p_smtcn->ssidrx, BLE_SMTCN_SSID_LEN)) {
            //the wifi AP is already connected
            ble_smtcn_send_indication(BLE_SMTCN_WIFI_INFO_CONNECTED, 0, NULL);
            ble_smtcn_send_indication(BLE_SMTCN_WIFI_INFO_SSID, ssid_len, ssid);
            p_smtcn->ip_addr[BLE_SMTCN_IP_LEN - 1] = '\0';
            if (strlen(p_smtcn->ip_addr)) {
                LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: send indication\n", __func__);
                //send indication
                ble_smtcn_send_indication(BLE_SMTCN_WIFI_INFO_IP, strlen(p_smtcn->ip_addr), (uint8_t *)(p_smtcn->ip_addr));
                memset(p_smtcn->ip_addr, 0, BLE_SMTCN_IP_LEN);
            }
        } else {
            //different ssid, setup new wifi connection
            ble_smtcn_setup_wifi_connection();
        }
    } else {
        //disconnected
        LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: wifi disconnected\n", __func__);
        ble_smtcn_setup_wifi_connection();
    }
exit:
    p_smtcn->wifi_conn_enabled = 0;
    return (uint32_t)size;
}

static uint32_t ble_smtcn_client_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]CCC, rw = %d, handle = 0x%x, size = %d\n", rw, handle, size);

    p_smtcn->conn_handle = handle;

    //add a timer
    if (p_smtcn->dtp_timer == NULL) {
        p_smtcn->dtp_timer = xTimerCreate("BLE_SMTCN_TIMER",
                                          BLE_SMTCN_TIMER_PERIOD / portTICK_PERIOD_MS, pdFALSE,
                                          (void *)0,
                                          ble_smtcn_timeout_callback);

        if (!p_smtcn->dtp_timer) {
            LOG_I(BLE_SMTCN, "[BLE_SMTCN]CCC, create timer fail, timer = 0x%x\n", p_smtcn->dtp_timer);
        }
    }

    if (rw == BT_GATTS_CALLBACK_WRITE) {
        if (size != sizeof(p_smtcn->indicate_enable)) { //Size check
            return 0;
        }
        p_smtcn->indicate_enable = *(uint16_t *)data;

        if (p_smtcn->indicate_enable == 0x0002) {
            //send indication
            if (xTimerStart(p_smtcn->dtp_timer, 0) != pdPASS) {
                LOG_I(BLE_SMTCN, "[BLE_SMTCN]CCC: timer start fail\n");
            } else {
                LOG_I(BLE_SMTCN, "[BLE_SMTCN]CCC: timer start\n");
            }
        }
    } else {
        if (size != 0) {
            memcpy(data, &p_smtcn->indicate_enable, sizeof(p_smtcn->indicate_enable));
        }
    }

    return sizeof(p_smtcn->indicate_enable);
}


static void ble_smtcn_ip_change_callback(struct netif *p_netif)
{
    char *p = NULL;

    if (p_netif == NULL)
        return;

    if (ip_addr_isany_val(p_netif->ip_addr))
        return;

    p = inet_ntoa(p_netif->ip_addr);
    if (p == NULL)
        return;

    memcpy(p_smtcn->ip_addr, p, BLE_SMTCN_IP_LEN);
    //strncpy(p_smtcn->ip_addr, p, BLE_SMTCN_IP_LEN);
    LOG_I(BLE_SMTCN, "[BLE_SMTCN] got IP:%s", p_smtcn->ip_addr);
    p_smtcn->ip_addr[BLE_SMTCN_IP_LEN - 1] = '\0';

    if (p_smtcn->indicate_enable == 0x0002) {
        ble_smtcn_send_indication(BLE_SMTCN_WIFI_INFO_IP, strlen(p_smtcn->ip_addr), (uint8_t *)(p_smtcn->ip_addr));
        memset(p_smtcn->ip_addr, 0, BLE_SMTCN_IP_LEN);
    }

}


int32_t ble_smtcn_wifi_event_handler(wifi_event_t event, uint8_t *payload, uint32_t length)
{
    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: event = %d\n", __func__, event);

    switch (event) {
        case WIFI_EVENT_IOT_PORT_SECURE: {
                if (p_smtcn->indicate_enable == 0x0002) {
                    uint8_t ssid[BLE_SMTCN_SSID_LEN] = {0};
                    uint8_t ssid_len;
                    struct netif *sta_if = netif_find("st2");

                    if (sta_if == NULL) {
                        LOG_E(BLE_SMTCN, "%s, sta_if is NULL", __func__);
                        return -1;
                    }
                    netif_set_status_callback(sta_if, ble_smtcn_ip_change_callback);
                    netif_set_link_up(sta_if);
                    dhcp_start(sta_if);

                    wifi_config_get_ssid(0, ssid, &(ssid_len));

                    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: ssid = %s, ssid_len = %d\n", __func__, ssid, ssid_len);


                    //send connected indication
                    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s: send connected indication\n", __func__);

                    ble_smtcn_send_indication(BLE_SMTCN_WIFI_INFO_CONNECTED, 0, NULL);
                    ble_smtcn_send_indication(BLE_SMTCN_WIFI_INFO_SSID, ssid_len, ssid);
                }
                break;
            }

        case WIFI_EVENT_IOT_DISCONNECTED: {
                struct netif *sta_if = netif_find("st2");
                if (sta_if == NULL) {
                    LOG_E(BLE_SMTCN, "%s, sta_if is NULL", __func__);
                    return -1;
                }
                netif_set_status_callback(sta_if, NULL);
                netif_set_link_down(sta_if);
                netif_set_addr(sta_if, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
                break;
            }
        default:
            break;
    }

    return 1;
}


int32_t ble_smtcn_init(void)
{
    int32_t status;

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s\n", __func__);

    if (ble_smtcn_init_done == pdTRUE) {
        LOG_E(BLE_SMTCN, "[BLE_SMTCN] Already init\n");
        return 0;
    }

    memset(p_smtcn, 0, sizeof(ble_smtcn_context_t));

    status = wifi_connection_register_event_handler(WIFI_EVENT_IOT_PORT_SECURE, ble_smtcn_wifi_event_handler);

    if (status < 0) {
        return status;
    }

    status = wifi_connection_register_event_handler(WIFI_EVENT_IOT_DISCONNECTED, ble_smtcn_wifi_event_handler);

    if (status < 0) {
        return status;
    }

    ble_smtcn_init_done = pdTRUE;
    return 0;
}


int32_t ble_smtcn_deinit(void)
{
    int32_t status;

    LOG_I(BLE_SMTCN, "[BLE_SMTCN]%s\n", __func__);

    if (ble_smtcn_init_done == pdFALSE) {
        LOG_E(BLE_SMTCN, "[BLE_SMTCN] Already deinit\n");
        return 0;
    }

    memset(p_smtcn, 0, sizeof(ble_smtcn_context_t));

    status = wifi_connection_unregister_event_handler(WIFI_EVENT_IOT_PORT_SECURE, ble_smtcn_wifi_event_handler);
    if (status < 0) {
        return status;
    }

    status = wifi_connection_unregister_event_handler(WIFI_EVENT_IOT_DISCONNECTED, ble_smtcn_wifi_event_handler);
    if (status < 0) {
        return status;
    }

    ble_smtcn_init_done = pdFALSE;
    return 0;
}

BT_GATTS_NEW_PRIMARY_SERVICE_16(bt_if_dtp_primary_service, BLE_SMTCN_SERVICE_UUID);

BT_GATTS_NEW_CHARC_16(bt_if_dtp_char,
                      BT_GATT_CHARC_PROP_WRITE | BT_GATT_CHARC_PROP_INDICATE, BLE_SMTCN_CHAR_VALUE_HANDLE, BLE_SMTCN_CHAR_UUID);

BT_GATTS_NEW_CHARC_VALUE_CALLBACK(bt_if_dtp_char_value, BLE_SMTCN_CHAR_UUID128,
                                  BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION, ble_smtcn_charc_value_callback);

BT_GATTS_NEW_CLIENT_CHARC_CONFIG(bt_if_dtp_client_config,
                                 BT_GATTS_REC_PERM_READABLE_ENCRYPTION | BT_GATTS_REC_PERM_WRITABLE_ENCRYPTION,
                                 ble_smtcn_client_config_callback);

static const bt_gatts_service_rec_t *bt_if_ble_smtcn_service_rec[] = {
    (const bt_gatts_service_rec_t *) &bt_if_dtp_primary_service,
    (const bt_gatts_service_rec_t *) &bt_if_dtp_char,
    (const bt_gatts_service_rec_t *) &bt_if_dtp_char_value,
    (const bt_gatts_service_rec_t *) &bt_if_dtp_client_config
};

const bt_gatts_service_t bt_if_ble_smtcn_service = {
    .starting_handle = 0x0014,
    .ending_handle = 0x0017,
    .required_encryption_key_size = 0,
    .records = bt_if_ble_smtcn_service_rec
};

#endif /* #ifdef MTK_BLE_SMTCN_ENABLE */




