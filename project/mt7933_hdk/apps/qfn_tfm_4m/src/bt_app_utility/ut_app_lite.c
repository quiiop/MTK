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

#include "ut_app.h"
#include "bt_driver_btsnoop.h"
#include "bt_setting.h"

/* Start of changable configuration. */
#include "ut_app_config.h"
#include "bt_debug.h"
//#include "bt_lwip.h"
#include <string.h>
#include "bt_a2dp.h"
#ifdef MTK_BLE_SMTCN_ENABLE
#include "ble_smtcn.h"
#endif /* #ifdef MTK_BLE_SMTCN_ENABLE */
#ifdef MTK_BT_ACEMW_ENABLE
#include "bt_ace_common.h"
#endif /* #ifdef MTK_BT_ACEMW_ENABLE */
#include "hal_psram.h"
#ifdef BLE_THROUGHPUT
static uint8_t enable_dle = 0;
#endif /* #ifdef BLE_THROUGHPUT */
#ifdef __MTK_BT_MESH_ENABLE__
#include "bt_mesh.h"
#include "mesh_app_util.h"
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */


extern uint32_t bt_gatt_service_execute_write(uint16_t handle, uint8_t flag);
extern uint16_t conn_interval; /* this is for calculating ble throughput*/
/* Lower Tester Information (PTS) */
const uint8_t lt_addr_type = BT_ADDR_PUBLIC;
uint8_t lt_addr[6] = APP_LT_ADDR;
/* Fill ABCD0000000000000000000004030201 in pts for SM OOB. */
const uint8_t oob_data[] = "\x01\x02\x03\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xcd\xab";
/* End of changable configuration. */

extern void bt_gap_dump(void);
#ifdef BT_DEBUG
extern void bt_gap_debug_cmd_sending(uint8_t *buffer);
#endif /* #ifdef BT_DEBUG */

extern bt_bd_addr_t g_local_public_addr;
static bt_gap_le_local_config_req_ind_t local_config;
static bt_app_power_state_t g_bt_app_power_state = BT_APP_POWER_STATE_INIT_NONE;
#ifdef MTK_BT_SUPPORT_FW_ASSERT_RECOVERY
static bool bt_app_is_assert_recovery = false;
#endif /* #ifdef MTK_BT_SUPPORT_FW_ASSERT_RECOVERY */

/* Start of flash. */
bt_gap_le_smp_pairing_config_t pairing_config_req, pairing_config_req_default = {
    .auth_req = BT_GAP_LE_SMP_AUTH_REQ_MITM | BT_GAP_LE_SMP_AUTH_REQ_BONDING | BT_GAP_LE_SMP_AUTH_REQ_SECURE_CONNECTION,
    //.auth_req = BT_GAP_LE_SMP_AUTH_REQ_BONDING,
    .maximum_encryption_key_size = 16,
    .io_capability = BT_GAP_LE_SMP_NO_INPUT_NO_OUTPUT,
    .initiator_key_distribution = BT_GAP_LE_SMP_KEY_DISTRIBUTE_ENCKEY | BT_GAP_LE_SMP_KEY_DISTRIBUTE_IDKEY | BT_GAP_LE_SMP_KEY_DISTRIBUTE_SIGN,
    .responder_key_distribution = BT_GAP_LE_SMP_KEY_DISTRIBUTE_ENCKEY | BT_GAP_LE_SMP_KEY_DISTRIBUTE_IDKEY | BT_GAP_LE_SMP_KEY_DISTRIBUTE_SIGN,
};

bt_gap_le_local_key_t local_key_req, local_key_req_default = {
    .encryption_info.ltk = { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc8, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf },
    .master_id.ediv = 0x1005,
    .master_id.rand = { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7 },
    .identity_info.irk = { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf },
    .signing_info.csrk = { 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf }
};
/* End of flash. */

/* Start of default configuration, don't edit here. */
bool sc_only, sc_only_default = true;

bt_hci_cmd_le_set_advertising_enable_t adv_enable, adv_enable_default = {
    .advertising_enable = BT_HCI_ENABLE,
};

bt_hci_cmd_le_set_advertising_parameters_t adv_para, adv_para_default = {
    .advertising_interval_min = 0x0800,
    .advertising_interval_max = 0x0800,
    .advertising_type = BT_HCI_ADV_TYPE_CONNECTABLE_UNDIRECTED,
    .advertising_channel_map = 7,
    .advertising_filter_policy = 0
};

#if 0
bt_hci_cmd_le_set_multi_advertising_enable_t multi_adv_enable, multi_adv_enable_default = {
    .advertising_enable = BT_HCI_ENABLE,
};

bt_hci_cmd_le_set_multi_advertising_parameters_t multi_adv_para, multi_adv_para_default = {
    .advertising_interval_min = 0x0800,
    .advertising_interval_max = 0x0800,
    .advertising_type = BT_HCI_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED,
    .advertising_channel_map = 7,
    .advertising_filter_policy = 0
};
#endif /* #if 0 */

bt_hci_cmd_le_set_scan_enable_t scan_enable = {
    .le_scan_enable = BT_HCI_ENABLE,
    .filter_duplicates = BT_HCI_ENABLE,
};

const bt_hci_cmd_le_set_scan_enable_t scan_disable = {
    .le_scan_enable = BT_HCI_DISABLE,
    .filter_duplicates = BT_HCI_DISABLE,
};

bt_hci_cmd_le_set_scan_parameters_t scan_para, scan_para_default = {
    .le_scan_type = BT_HCI_SCAN_TYPE_ACTIVE,
    .le_scan_interval = 0x0024,
    .le_scan_window = 0x0011,
    .own_address_type = BT_HCI_SCAN_ADDR_PUBLIC,
    .scanning_filter_policy = BT_HCI_SCAN_FILTER_ACCEPT_ALL_ADVERTISING_PACKETS,
};

bt_hci_cmd_le_create_connection_t connect_para, connect_para_default = {
    .le_scan_interval = 0x0010,
    .le_scan_window = 0x0010,
    .initiator_filter_policy = BT_HCI_CONN_FILTER_ASSIGNED_ADDRESS,
    .peer_address = {
        .type = BT_ADDR_PUBLIC,
    },
    .own_address_type = BT_ADDR_PUBLIC,
    .conn_interval_min = 0x0006,
    .conn_interval_max = 0x0320,
    .conn_latency = 0x0000,
    .supervision_timeout = 0x07d0,
    .minimum_ce_length = 0x0000,
    .maximum_ce_length = 0x0190,
};

bt_hci_cmd_disconnect_t disconnect_para, disconnect_para_default = {
    .connection_handle = 0x0200,
    .reason = BT_HCI_STATUS_REMOTE_USER_TERMINATED_CONNECTION,
};

bt_hci_cmd_le_connection_update_t conn_update_para, conn_update_para_default = {
    .connection_handle = 0x0200,
    .conn_interval_min = 0x0320,
    .conn_interval_max = 0x0320,
    .conn_latency = 0x0006,
    .supervision_timeout = 0x0962,
    .minimum_ce_length = 0x0000,
    .maximum_ce_length = 0x0190,
};

bt_hci_cmd_read_rssi_t read_rssi = {
    .handle = 0x0200,
};

#define CMD_PROTECT_LIST_SIZE  20
//If cmd has parameter we should add exta space to prevent an other cmd use same string.
const char *cli_cmd_protect_list[CMD_PROTECT_LIST_SIZE] = {
    "gap ",
    "gattc ",
    "gatts ",
    "gatt "
};

bt_hci_cmd_le_set_advertising_data_t adv_data, adv_data_default = {0};
bt_hci_cmd_le_set_scan_response_data_t scan_data, scan_data_default = {0};
//bt_hci_cmd_le_set_multi_advertising_data_t multi_adv_data, multi_adv_data_default = {0};
//bt_hci_cmd_le_set_multi_scan_response_data_t multi_scan_data, multi_scan_data_default = {0};
static uint8_t ut_app_reset_global_config_flag = true;
bt_status_t (*ut_app_callback)(bt_msg_type_t, bt_status_t, void *) = NULL;
/* End of default configuration. */

bool bt_app_advertising = false;
bool bt_app_scanning = false;
bool bt_app_connecting = false;
#define BT_APP_RESOLVING_LIST_UPDATING 0x01
#define BT_APP_WHITE_LIST_UPDATING 0x02
uint8_t list_updating = 0;//combination of BT_APP_RESOLVING_LIST_UPDATING & BT_APP_WHITE_LIST_UPDATING

bt_status_t bt_app_gap_io_callback(void *input, void *output);

static const struct bt_app_callback_table_t {
    const char *name;
    bt_status_t (*io_callback)(void *, void *);
} bt_app_callback_table[] = {
    {"gap",     bt_app_gap_io_callback},
};

uint8_t bt_app_cmd_in_protected_list(char *cmd)
{
    uint8_t i = 0;

    while (cli_cmd_protect_list[i] != NULL && i < CMD_PROTECT_LIST_SIZE) {
        if (!memcmp(cli_cmd_protect_list[i], cmd, strlen(cli_cmd_protect_list[i])))
            return true;
        else
            i++;
    }
    return false;
}

void ut_app_reset_flash(void)
{
    clear_bonded_info();
    pairing_config_req = pairing_config_req_default;
    local_key_req = local_key_req_default;
}

void ut_app_reset_global_config(void)
{
    ut_app_callback = NULL;
    adv_enable = adv_enable_default;
    adv_para = adv_para_default;
    scan_para = scan_para_default;
    connect_para = connect_para_default;
    disconnect_para = disconnect_para_default;
    conn_update_para = conn_update_para_default;
    adv_data = adv_data_default;
    scan_data = scan_data_default;
    sc_only = sc_only_default;
    //multi_adv_data = multi_adv_data_default;
    //multi_scan_data = multi_scan_data_default;
    //multi_adv_para = multi_adv_para_default;
    //multi_adv_enable = multi_adv_enable_default;
}

static char *get_event_type(uint8_t type)
{
    switch (type) {
        case BT_GAP_LE_ADV_REPORT_EVT_TYPE_ADV_IND:
            return "ADV_IND";
        case BT_GAP_LE_ADV_REPORT_EVT_TYPE_ADV_DIRECT_IND:
            return "ADV_DIRECT_IND";
        case BT_GAP_LE_ADV_REPORT_EVT_TYPE_ADV_SCAN_IND:
            return "ADV_SCAN_IND";
        case BT_GAP_LE_ADV_REPORT_EVT_TYPE_ADV_NONCONN_IND:
            return "ADV_NONCONN_IND";
        case BT_GAP_LE_ADV_REPORT_EVT_TYPE_ADV_SCAN_RSP:
            return "SCAN_RSP";
        default:
            return "NULL";
    }
}


void print_advertising_report(void *p)
{
    const bt_gap_le_advertising_report_ind_t *report = (bt_gap_le_advertising_report_ind_t *)p;

    BT_COLOR_SET(BT_COLOR_BLUE);
    BT_LOGI("APP", "=== adv repo ===");
    BT_LOGI("APP", " Addr:\t%s", bt_debug_addr2str(&report->address));
    BT_LOGI("APP", " Type:\t%s", get_event_type(report->event_type));
    uint8_t count, ad_data_len, ad_data_type, ad_data_idx;
    count = 0;
    uint8_t buff[100] = {0};
    while (count < report->data_length) {
        if (count >= BT_HCI_LE_ADVERTISING_DATA_LENGTH_MAXIMUM - 2) {
            BT_LOGW("%s, cnt(%u) > adv max len", __func__, count);
            break;
        }
        ad_data_len = report->data[count];
        /* Error handling for data length over 30 bytes. */
        if (ad_data_len >= 0x1F) {
            BT_LOGI("APP", "AD Data Len Err");
            break;
        }
        ad_data_type = report->data[count + 1];
        count += 2;

        if (ad_data_type == BT_GAP_LE_AD_TYPE_FLAG) {
            if (report->data[count] & BT_GAP_LE_AD_FLAG_LIMITED_DISCOVERABLE) {
                BT_LOGI("APP", " AD Flags:\tLE Limited Disc Mode");
            } else if (report->data[count] & BT_GAP_LE_AD_FLAG_GENERAL_DISCOVERABLE) {
                BT_LOGI("APP", " AD Flags:\tLE General Disc Mode");
            } else {
                BT_LOGI("APP", " AD Flags:\tUnknown: 0x%02x", report->data[count]);
            }
            count += (ad_data_len - 1);
        } else if (ad_data_type == BT_GAP_LE_AD_TYPE_NAME_COMPLETE) {
            for (ad_data_idx = 0; ad_data_idx < (ad_data_len - 1); ad_data_idx++, count++) {
                buff[ad_data_idx] = report->data[count];
            }
            BT_LOGI("APP", " Name:\t%s", buff);
        } else {
            count += (ad_data_len - 1);
        }
    }
#if defined(MTK_BT_LWIP_ENABLE)
    bt_lwip_send(report->data, report->data_length);
    bt_lwip_send("\r\n", 5);
#endif /* #if defined(MTK_BT_LWIP_ENABLE) */
    BT_LOGI("APP", "======");
    BT_COLOR_SET(BT_COLOR_WHITE);
}

//only print report that has name
void print_advertising_report_name_only(void *p)
{
    const bt_gap_le_advertising_report_ind_t *report = (bt_gap_le_advertising_report_ind_t *)p;

    BT_COLOR_SET(BT_COLOR_BLUE);
    uint8_t count, ad_data_len, ad_data_type, ad_data_idx;
    count = 0;
    uint8_t buff[100] = {0};
    bool isNameExist = false;
    while (count < report->data_length) {
        if (count >= BT_HCI_LE_ADVERTISING_DATA_LENGTH_MAXIMUM - 2) {
            BT_LOGW("%s, cnt(%u) > adv max len", __func__, count);
            break;
        }

        ad_data_len = report->data[count];
        /* Error handling for data length over 30 bytes. */
        if (ad_data_len >= 0x1F) {
            BT_LOGI("APP", "AD Data Len Err");
            break;
        }
        ad_data_type = report->data[count + 1];
        count += 2;

        if (ad_data_type == BT_GAP_LE_AD_TYPE_FLAG) {
            count += (ad_data_len - 1);
        } else if (ad_data_type == BT_GAP_LE_AD_TYPE_NAME_COMPLETE) {
            for (ad_data_idx = 0; ad_data_idx < (ad_data_len - 1); ad_data_idx++, count++) {
                buff[ad_data_idx] = report->data[count];
            }
            isNameExist = true;
            break; //should be removed, if we want to parser more
        } else {
            count += (ad_data_len - 1);
        }
    }

    if (isNameExist) {
        BT_LOGI("APP", "=== adv info ===");
        BT_LOGI("APP", " Addr:\t%s", bt_debug_addr2str(&report->address));
        BT_LOGI("APP", " Type:\t%s", get_event_type(report->event_type));
        BT_LOGI("APP", " Name:\t%s", buff);
    }

    BT_COLOR_SET(BT_COLOR_WHITE);
}

void ut_print_help(void)
{
    printf("Cmd line usage ex: (ble ?), (ble gap stop_scan)\n"
           "ble        -Common cmds\n"
           "  init                   -Init ble and power on BT\n"
           "  btdrv snoop [on/off]   -Enable/Disable hcisnoop log\n"
           "  [po/pf]                -Power on/off BT\n"
           "  wifi smart             -start wifi smart adv\n"
           "==== ble gap ====\n"
           "ble gap          -GAP command line\n"
           "  set_adv_data [data_buf]       -Set adv data\n"
           "  start_scan [scan_type][interval][window][own addr type][scan filter policy][filter dup]\n"
           "                         -Start scan\n"
           "  stop_scan              -Stop scan\n"
           "  connect [addr_type][address]  -Connect remote dev\n"
           "  disconnect [con_hdl]   -Disconnect connection\n"
          );
}

bool copy_str_to_addr(uint8_t *addr, const char *str)
{
    unsigned int i, value;
    int using_long_format = 0;
    int using_hex_sign = 0;
    int icn = 0;

    if (str[2] == ':' || str[2] == '-') {
        using_long_format = 1;
    }

    if (str[1] == 'x') {
        using_hex_sign = 2;
    }

    /*
     * For example :
     * 11:22:33:44:55:66             -> OK
     * 0x11:0x22:0x33:0x44:0x55:0x66 -> OK
     * 112233445566                  -> OK
     * 0x11:22:33:44:55:66           -> NG
     */
    if ((size_t)(6 * (using_hex_sign + 2 + using_long_format) - using_long_format) > strlen(str)) {
        BT_LOGE("APP", "Expect %d  Actual %d",
                (size_t)(6 * (using_hex_sign + 2 + using_long_format) - using_long_format), strlen(str));
        return false;
    }

    for (i = 0; i < 6; i++) {
        icn = sscanf(str + using_hex_sign + i * (2 + using_long_format), "%02x", &value);
        if (icn < 0)
            return false;
        addr[5 - i] = (uint8_t) value;
    }

    return true;
}

static uint32_t sm_passkey;
static uint8_t nc_value_correct[1];

bt_status_t app_add_dev_2_resolving_list(const bt_gap_le_bonding_info_t *bonded_info)
{
    bt_status_t st = BT_STATUS_SUCCESS;
    if (BT_ADDR_TYPE_UNKNOW != bonded_info->identity_addr.address.type) {
        bt_hci_cmd_le_add_device_to_resolving_list_t dev;
        dev.peer_identity_address = bonded_info->identity_addr.address;
        memcpy(dev.peer_irk, &(bonded_info->identity_info), sizeof(dev.peer_irk));
        memcpy(dev.local_irk, &(local_key_req.identity_info), sizeof(dev.local_irk));
        st = bt_gap_le_set_resolving_list(BT_GAP_LE_ADD_TO_RESOLVING_LIST, (void *)&dev);
        if (BT_STATUS_OUT_OF_MEMORY == st) {
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "Add dev to Resolving List Failed [OOM]");
            BT_COLOR_SET(BT_COLOR_WHITE);
        }
    }
    return st;
}
/* If we have peer's identity address info(type/address), we will add identity info to white list, or
   Add peer_addr info if we don't have peer's identity address.
*/
bt_status_t app_add_dev_2_white_list(const bt_gap_le_bonding_info_t *bonded_info, const bt_addr_t *peer_addr)
{
    bt_status_t st = BT_STATUS_SUCCESS;
    bt_addr_t device;

    if (BT_ADDR_TYPE_UNKNOW != bonded_info->identity_addr.address.type) {
        memcpy((void *)&device, (void *) & (bonded_info->identity_addr.address), sizeof(device));
        st = bt_gap_le_set_white_list(BT_GAP_LE_ADD_TO_WHITE_LIST, &device);
    } else if (peer_addr->type != BT_ADDR_TYPE_UNKNOW) {
        memcpy((void *)&device, (void *)peer_addr, sizeof(device));
        st = bt_gap_le_set_white_list(BT_GAP_LE_ADD_TO_WHITE_LIST, &device);
    }

    if (BT_STATUS_OUT_OF_MEMORY == st) {
        BT_COLOR_SET(BT_COLOR_RED);
        BT_LOGI("APP", "Add dev to White List Failed [OOM]");
        BT_COLOR_SET(BT_COLOR_WHITE);
    }
    return st;
}

static void bt_app_power_state_set(bt_app_power_state_t app_power_state)
{
    g_bt_app_power_state = app_power_state;
    return;
}

static bt_app_power_state_t bt_app_power_state_get(void)
{
    return g_bt_app_power_state;
}

bt_status_t bt_app_io_callback(void *input, void *output)
{
    char *cmd = input;

    BT_LOGI("APP", "cli_cmd: %s", cmd);
    if (bt_app_cmd_in_protected_list(cmd)) {
        if (bt_app_power_state_get() != BT_APP_POWER_STATE_POWER_ON) {
            BT_LOGE("APP", "BT not power on(st = %d).", bt_app_power_state_get());
            return BT_STATUS_SUCCESS;
        }
    }
    if (ut_app_reset_global_config_flag) {
        ut_app_reset_global_config();
    }

    if (UT_APP_CMP("?")) {
        ut_print_help();
        return BT_STATUS_SUCCESS;
    }

    else if (UT_APP_CMP("init")) {
        if (bt_app_power_state_get() == BT_APP_POWER_STATE_INIT_NONE) {
            bt_app_power_state_set(BT_APP_POWER_STATE_INIT_START);
            bt_create_task();
        } else {
            BT_LOGI("APP", "already init");
        }
    }

    else if (UT_APP_CMP("reset config off")) {
        ut_app_reset_global_config_flag = false;
    }

    else if (UT_APP_CMP("reset config on")) {
        ut_app_reset_global_config_flag = true;
    }

    else if (UT_APP_CMP("get local addr")) {
        BT_LOGI("APP", "local_pub_addr [%02X:%02X:%02X:%02X:%02X:%02X]",
                g_local_public_addr[5], g_local_public_addr[4], g_local_public_addr[3],
                g_local_public_addr[2], g_local_public_addr[1], g_local_public_addr[0]);
    }

    else if (UT_APP_CMP("po")) {
        if (bt_app_power_state_get() == BT_APP_POWER_STATE_POWER_OFF) {
            bt_app_power_state_set(BT_APP_POWER_STATE_POWER_ON_START);
            bt_power_on((bt_bd_addr_ptr_t)&g_local_public_addr, NULL);
            //keep first po's default value, otherwise PB_GATT cannot conneted after power off/power on
            //bt_gatts_set_max_mtu(128); /* This value should consider with MM Tx/Rx buffer size. */
        } else {
            BT_LOGI("APP", "not in power off state");
        }
    }

    else if (UT_APP_CMP("pf")) {
        if (bt_app_power_state_get() == BT_APP_POWER_STATE_POWER_ON) {
            bt_app_power_state_set(BT_APP_POWER_STATE_POWER_OFF_START);
#ifdef __MTK_BT_MESH_ENABLE__
            //mesh don't need disable if bt do not open
            if (bt_get_init_mode() == BT_BOOT_INIT_MODE_MESH) {
                if (bt_mesh_disable() != BT_MESH_SUCCESS) {
                    BT_LOGI("APP", "bt_disable fail, please retry");
                    return BT_STATUS_FAIL;;
                }
            }
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */
            bt_power_off();
        } else {
            BT_LOGI("APP", "not in power on state");
        }
    }

    else if (UT_APP_CMP("list connection")) {
        BT_COLOR_SET(BT_COLOR_BLUE);
        dump_connection_info_list();
        BT_COLOR_SET(BT_COLOR_WHITE);
    }

#ifdef MTK_BLE_SMTCN_ENABLE
    else if (UT_APP_CMP("wifi smart")) {
        BT_LOGI("APP", "[DTP]start adv\n");
        ble_smtcn_init();
        if (strlen(cmd) > strlen("wifi smart "))
            ble_smtcn_start_adv(cmd + strlen("wifi smart "), strlen(cmd) - strlen("wifi smart "));
        else
            ble_smtcn_start_adv(NULL, 0);
    }
#endif /* #ifdef MTK_BLE_SMTCN_ENABLE */
    else if (UT_APP_CMP("btdrv snoop")) {
        bt_setting_value_t value;

        if (UT_APP_CMP("btdrv snoop on")) {
            BT_LOGI("APP", "btdrv snoop on");
            value.value_bool = true;
        } else if (UT_APP_CMP("btdrv snoop off")) {
            BT_LOGI("APP", "btdrv snoop off");
            value.value_bool = false;
        } else {
            BT_LOGI("APP", "cmd not found");
            return BT_STATUS_SUCCESS;
        }

        if (bt_setting_value_set(BT_SETTING_KEY_LOG_SNOOP, value))
            bt_driver_btsnoop_ctrl(value.value_bool);
        else
            BT_LOGE("APP", "set snoop log fail");
    } else {
        unsigned int i;
        for (i = 0; i < sizeof(bt_app_callback_table) / sizeof(struct bt_app_callback_table_t); i++) {
            if (UT_APP_CMP(bt_app_callback_table[i].name)) {
                return bt_app_callback_table[i].io_callback(input, output);
            }
        }
        BT_LOGE("APP", "%s: cmd not found", cmd);
    }

    return BT_STATUS_SUCCESS;
}

const bt_gap_config_t bt_custom_config = {
    .inquiry_mode = 2,
    .io_capability = BT_GAP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
    .cod = 0x2C0404,
    .device_name = {"HB Duo dev"},
};

void bt_gap_set_local_configuration_name(char *name)
{
    if (strlen(name) <= strlen(bt_custom_config.device_name)) {
        memcpy((void *)bt_custom_config.device_name, (void *)name, strlen(bt_custom_config.device_name));
    }
    BT_LOGI("APP", "Change BT Local Name = %s", bt_custom_config.device_name);
}

const bt_gap_config_t *bt_gap_get_local_configuration(void)
{
    return &bt_custom_config;
}

bt_gap_le_bonding_info_t *bt_ut_gap_le_get_bonding_info(const bt_addr_t remote_addr)
{
    app_bt_bonded_info_t *bonded_info = get_bonded_info(&remote_addr, 1);
    if (bonded_info) {
        return &(bonded_info->info);
    }

    return NULL;
}

bt_gap_le_local_config_req_ind_t *bt_ut_gap_le_get_local_config(void)
{
    local_config.local_key_req = &local_key_req;
    local_config.sc_only_mode_req = sc_only;

    return &local_config;
}

bt_status_t bt_ut_gap_le_get_pairing_config(bt_gap_le_bonding_start_ind_t *ind)
{
    ind->pairing_config_req = pairing_config_req;

    return BT_STATUS_SUCCESS;
}

bool bt_ut_gap_le_is_connection_update_request_accepted(bt_handle_t handle, bt_gap_le_connection_update_param_t *parm)
{
    BT_LOGI("APP", "%s: always accept so far", __FUNCTION__);
    return true;
}

bt_gap_le_local_key_t *bt_ut_gap_le_get_local_key(void)
{
    return &local_key_req_default;
}

bt_status_t bt_ut_app_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{
    if (msg != BT_MEMORY_FREE_GARBAGE_IND &&
        msg != BT_GAP_LE_ADVERTISING_REPORT_IND &&
        msg != BT_GAP_LE_SET_ADVERTISING_CNF) {
        BT_COLOR_SET(BT_COLOR_GREEN);
        BT_LOGI("APP", "%s: status(0x%04x), msg = 0x%x", __FUNCTION__, status, msg);
        BT_COLOR_SET(BT_COLOR_WHITE);
    }

#ifdef MTK_BLE_SMTCN_ENABLE
    ble_smtcn_event_callback(msg, status, buff);
#endif /* #ifdef MTK_BLE_SMTCN_ENABLE */

    switch (msg) {
        /* GAP */
        case BT_PANIC_COREDUMP_DONE:
            BT_LOGI("APP", "BT_COREDUMP_DONE!");
#ifdef MTK_BT_SUPPORT_FW_ASSERT_RECOVERY
#ifdef __MTK_BT_MESH_ENABLE__
            //mesh don't need disable if bt do not open
            if (bt_get_init_mode() == BT_BOOT_INIT_MODE_MESH) {
                bt_mesh_disable();
                //wait mesh disable complete/event received finished, then power off.
                bt_os_layer_sleep_task(100);
            }
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */
            bt_app_is_assert_recovery = true;
            bt_app_power_state_set(BT_APP_POWER_STATE_POWER_OFF_START);
            bt_power_off();
#endif /* #ifdef MTK_BT_SUPPORT_FW_ASSERT_RECOVERY */
            break;
        case BT_POWER_ON_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_POWER_ON_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            ut_app_reset_flash();
            bt_app_power_state_set(BT_APP_POWER_STATE_POWER_ON);
            break;
        /* GAP */
        case BT_POWER_OFF_CNF:
            bt_app_power_state_set(BT_APP_POWER_STATE_POWER_OFF);
#ifdef MTK_BT_SUPPORT_FW_ASSERT_RECOVERY
            if (bt_app_is_assert_recovery) {
                BT_LOGI("APP", "BT_COREDUMP_DONE! Now do power on...");
                bt_app_power_state_set(BT_APP_POWER_STATE_POWER_ON_START);
                bt_power_on((bt_bd_addr_ptr_t)&g_local_public_addr, NULL);
                bt_app_is_assert_recovery = false;
            }
#endif /* #ifdef MTK_BT_SUPPORT_FW_ASSERT_RECOVERY */
            break;
        case BT_GAP_LE_SET_WHITE_LIST_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_SET_WHITE_LIST_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            list_updating = list_updating | (~BT_APP_WHITE_LIST_UPDATING);
            if (list_updating == 0x00) {
                if (bt_app_advertising) {
                    adv_enable.advertising_enable = BT_HCI_ENABLE;
                    bt_gap_le_set_advertising(&adv_enable, NULL, NULL, NULL);
                }
                if (bt_app_scanning) {
                    bt_gap_le_set_scan(&scan_enable, &scan_para);
                }
            }
            break;
        case BT_GAP_LE_SET_RESOLVING_LIST_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_SET_RESOLVING_LIST_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            list_updating = list_updating | (~BT_APP_RESOLVING_LIST_UPDATING);
            if (list_updating == 0x00) {
                if (bt_app_advertising) {
                    adv_enable.advertising_enable = BT_HCI_ENABLE;
                    bt_gap_le_set_advertising(&adv_enable, NULL, NULL, NULL);
                }
                if (bt_app_scanning) {
                    bt_gap_le_set_scan(&scan_enable, &scan_para);
                }
            }
            break;
        case BT_GAP_LE_SET_ADDRESS_RESOLUTION_ENABLE_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "set addr resolv %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_SET_ADVERTISING_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "set adv %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_SET_SCAN_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "set scan %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_ADVERTISING_REPORT_IND:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "adv report %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            print_advertising_report(buff);
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_CONNECT_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "con cnf %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            bt_app_connecting = status == BT_STATUS_SUCCESS;
            break;
        case BT_GAP_LE_CONNECT_IND: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "con ind %s",
                        (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
                BT_COLOR_SET(BT_COLOR_BLUE);

                bt_gap_le_connection_ind_t *connection_ind = (bt_gap_le_connection_ind_t *)buff;
                BT_LOGI("APP", "conn_hdl=0x%04x", connection_ind->connection_handle);
                BT_LOGI("APP", "role=%s", (connection_ind->role == BT_ROLE_MASTER) ? "Master" : "Slave");
                BT_LOGI("APP", "peer addr:%s", bt_debug_addr2str(&connection_ind->peer_addr));
#ifdef BLE_THROUGHPUT
                printf("conn_hdl=0x%04x\n", connection_ind->connection_handle);
                printf("peer addr:%s\n", bt_debug_addr2str(&connection_ind->peer_addr));
#endif /* #ifdef BLE_THROUGHPUT */
                BT_COLOR_SET(BT_COLOR_WHITE);
                if (status == BT_STATUS_SUCCESS) {
                    add_connection_info(buff);
                    bt_handle_t handle = connection_ind->connection_handle;
                    disconnect_para.connection_handle = handle;
                    conn_update_para.connection_handle = handle;
                    read_rssi.handle = handle;
                    conn_interval = (connection_ind->conn_interval * 5) / 4;
                    BT_LOGI("APP", "ACL connected, start bonding, hdl = 0x%x", handle);
                    //mesh connect not need pairing, gatt also may not need, remove firstly.
                    //bt_gap_le_bond(handle, &pairing_config_req);
#ifdef BLE_THROUGHPUT
                    if (enable_dle) {
                        bt_hci_cmd_le_set_data_length_t data_length;
                        data_length.connection_handle = handle;
                        data_length.tx_octets = 0xFA;
                        data_length.tx_time = 0x150;
                        bt_gap_le_update_data_length(&data_length);
                    }
#endif /* #ifdef BLE_THROUGHPUT */
                }
                bt_app_advertising = false;
                break;
            }
        case BT_GAP_LE_CONNECT_CANCEL_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "con cancel cnf %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            bt_app_connecting =  false;
            break;
        case BT_GAP_LE_DISCONNECT_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "discon cnf %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_DISCONNECT_IND:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "discon ind %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            delete_connection_info(buff);
            break;
        case BT_GAP_LE_CONNECTION_UPDATE_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "update cnf %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_CONNECTION_UPDATE_IND:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "update ind %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_BONDING_REPLY_REQ_IND:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "bond reply %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            {
                if (buff == NULL) {
                    BT_LOGI("APP", "status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    return status;
                }
                bt_gap_le_bonding_reply_t rsp = {{{0}}};
                bt_gap_le_bonding_reply_req_ind_t *ind = (bt_gap_le_bonding_reply_req_ind_t *)buff;
                if (ind->method & BT_GAP_LE_SMP_PASSKEY_DISPLAY_MASK) {
                    BT_COLOR_SET(BT_COLOR_BLUE);
                    printf("---->Passkey: %06u<----\n", ind->passkey_display);
                    BT_COLOR_SET(BT_COLOR_WHITE);

                } else if (ind->method & BT_GAP_LE_SMP_PASSKEY_INPUT_MASK) {
                    uint32_t i;
                    BT_COLOR_SET(BT_COLOR_BLUE);
                    sm_passkey = 0;
                    printf("\nInput passkey: \n");
                    //wait for input
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    for (i = 0; i < 40; i++) {
                        if (sm_passkey != 0) {
                            break;
                        }
                        bt_os_layer_sleep_task(1000);
                    }
                    rsp.passkey = sm_passkey;
                    status = bt_gap_le_bonding_reply(ind->handle, &rsp);
                } else if (ind->method == BT_GAP_LE_SMP_OOB) {
                    memcpy(rsp.oob_data, oob_data, 16);

                    status = bt_gap_le_bonding_reply(ind->handle, &rsp);
                } else if (ind->method & BT_GAP_LE_SMP_NUMERIC_COMPARISON_MASK) {
                    uint32_t i;
                    BT_COLOR_SET(BT_COLOR_BLUE);
                    printf("---->Passkey: %06u<----\n", ind->passkey_display);
                    sm_passkey = 0;
                    printf("\nConfirm numeric num:Y/N\n");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    for (i = 0; i < 40; i++) {
                        if (nc_value_correct[0] != 0) {
                            break;
                        }
                        bt_os_layer_sleep_task(1000);
                    }
                    if (nc_value_correct[0] != 'n' && nc_value_correct[0] != 'N') {
                        rsp.nc_value_matched = true;
                    } else {
                        rsp.nc_value_matched = false;
                    }

                    status = bt_gap_le_bonding_reply(ind->handle, &rsp);
                }
            }
            break;
        case BT_GAP_LE_BONDING_COMPLETE_IND:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "bond cmp %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            if (status == BT_STATUS_SUCCESS) {
                const bt_gap_le_bonding_complete_ind_t *ind = (bt_gap_le_bonding_complete_ind_t *)buff;
                app_bt_bonded_info_t *bonded_info = find_bonded_info_by_handle(ind->handle);
                if (!bonded_info) {
                    status = BT_STATUS_FAIL;
                    break;
                }
                app_bt_connection_cb_t *con = find_connection_info_by_handle(ind->handle);
                if (!con) {
                    status = BT_STATUS_FAIL;
                    break;
                }
                /* If peer identity address is not null, update to resolving list and white list*/
                //If advertising or scanning is enable, Disable advertising or scanning.
                if (bt_app_advertising) {
                    adv_enable.advertising_enable = BT_HCI_DISABLE;
                    bt_gap_le_set_advertising(&adv_enable, NULL, NULL, NULL);
                }
                if (bt_app_scanning) {
                    bt_gap_le_set_scan(&scan_disable, NULL);
                }
                // If we got IRK/Identity address from peer, we have to change
                // 1. connection info's bd address; app_bt_connection_cb_t
                // 2. bonding info's bd address; app_bt_bonded_info_t
                if (BT_ADDR_TYPE_UNKNOW != bonded_info->info.identity_addr.address.type) {
                    /*Because value of bonded_info->info.identity_addr.address_type is 0[Public Identity] or 1[Random Identity],
                     *but Identity address type were definied 2 or 3 in spec.
                     *We have to "+2" for synchronization.
                    */
                    con->peer_addr = bonded_info->info.identity_addr.address;
                    bonded_info->bt_addr = bonded_info->info.identity_addr.address;
                }
                //update resolving list
                if (BT_STATUS_SUCCESS == app_add_dev_2_resolving_list(&(bonded_info->info))) {
                    list_updating = list_updating | BT_APP_RESOLVING_LIST_UPDATING;
                    bt_gap_le_set_address_resolution_enable(true);
                }
                //update white list(use identity address or address)
                if (BT_STATUS_SUCCESS == app_add_dev_2_white_list(&(bonded_info->info), &(con->peer_addr))) {
                    list_updating = list_updating | BT_APP_WHITE_LIST_UPDATING;
                }
            }
            break;
#ifdef __BT_HB_DUO__
        case BT_GAP_IO_CAPABILITY_REQ_IND: {
                //bt_gap_connection_handle_t param;
                //param = (bt_gap_connection_handle_t)buff;
                BT_LOGI("APP", "io cap req %s", (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
                bt_gap_reply_io_capability_request(BT_GAP_OOB_DATA_PRESENTED_NONE, BT_GAP_SECURITY_AUTH_REQUEST_MITM_GENERAL_BONDING);
                return BT_STATUS_SUCCESS;
            }
#endif /* #ifdef __BT_HB_DUO__ */
    }

    if (status == BT_STATUS_OUT_OF_MEMORY) {
        return BT_STATUS_OUT_OF_MEMORY;
    }

    if (ut_app_callback) {
        status = ut_app_callback(msg, status, buff);
    }

    return status;
}



bt_status_t bt_ut_gatts_get_authorization_check_result(bt_gatts_authorization_check_req_t *req)
{
    bt_gap_le_bonding_info_t *bonded_info = &(find_bonded_info_by_handle(req->connection_handle)->info);
    if (!bonded_info)
        return BT_STATUS_FAIL;

    BT_LOGI("APP", "Peer ask to access attr with author req.");
    BT_LOGI("APP", "conn[0x%04x] attr hdl[0x%04x] [%s]", req->connection_handle, req->attribute_handle,
            req->read_write == BT_GATTS_CALLBACK_READ ? "Read" : "Write");
    BT_LOGI("APP", "Sec mode[0x%02x]", bonded_info->key_security_mode);
    if ((bonded_info->key_security_mode & BT_GAP_LE_SECURITY_AUTHENTICATION_MASK) > 0) {
        /* If you agree peer device can access all characteristic with
           authorization permission, you can set #BT_GAP_LE_SECURITY_AUTHORIZATION_MASK
           flag, and GATTS will not call for authorization check again. */
        bonded_info->key_security_mode = bonded_info->key_security_mode | BT_GAP_LE_SECURITY_AUTHORIZATION_MASK;
        /* If application accept peer access this attribute. */
        return BT_STATUS_SUCCESS;
    } else {
        /* If application reject peer access this attribute. */
        return BT_STATUS_UNSUPPORTED;
    }

}

bt_status_t bt_ut_app_set_btsnoop_callback(void)
{
    bt_setting_value_t get_value;

    get_value = bt_setting_value_get(BT_SETTING_KEY_LOG_SNOOP);
    bt_driver_btsnoop_ctrl(get_value.value_bool);

    return BT_STATUS_SUCCESS;
}

