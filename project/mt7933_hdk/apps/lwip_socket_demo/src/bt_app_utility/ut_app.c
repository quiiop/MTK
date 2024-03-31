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
#include "bt_debug_ext.h"
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
#ifdef MTK_BLE_DM_SUPPORT
#include "bt_device_manager_le.h"
#endif /* #ifdef MTK_BLE_DM_SUPPORT */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

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

extern bt_bd_addr_t local_public_addr;
static bt_gap_le_local_config_req_ind_t local_config;
static bt_app_power_state_t g_bt_app_power_state = BT_APP_POWER_STATE_INIT_NONE;
#ifdef MTK_BT_SUPPORT_FW_ASSERT_RECOVERY
static bool bt_app_is_assert_recovery = false;
#endif /* #ifdef MTK_BT_SUPPORT_FW_ASSERT_RECOVERY */

#ifdef BT_UT_GAP_LE_SCAN_LIST
#define BT_UT_GAP_LE_SCAN_LIST_SIZE         32
#define BT_UT_GAP_LE_SCAN_LIST_MAX_NAME_LEN 32

typedef struct _bt_ut_gap_le_scan_list_element_t {
    uint8_t name[BT_UT_GAP_LE_SCAN_LIST_MAX_NAME_LEN];
    bool name_exist;
    bt_addr_t addr;
    uint8_t event_type;
    struct _bt_ut_gap_le_scan_list_element_t *prev;
    struct _bt_ut_gap_le_scan_list_element_t *next;
} bt_ut_gap_le_scan_list_element_t;

typedef struct {
    bt_ut_gap_le_scan_list_element_t *head;
    bt_ut_gap_le_scan_list_element_t *tail;
    uint32_t list_number;
} bt_ut_gap_le_scan_list_t;

bt_ut_gap_le_scan_list_t *pg_bt_gap_le_scan_list = NULL;
xSemaphoreHandle g_bt_gap_le_scan_list_mtx = NULL;
#endif /* #ifdef BT_UT_GAP_LE_SCAN_LIST */

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
    "wifi smart",
    "cancel connect",
    "adv ",
    "advanced adv ",
    "advanced connect ",
    "advanced scan ",
    "single ",
    "scan ",
    "disconnect ",
    "connect ",
    "wl ",
    "gap ",
    "sm ",
    "gattc ",
    "gatts ",
    "gatt "
};

#if 0
#define BT_A2DP_MAKE_CODEC_SBC(role, min_bit_pool, max_bit_pool, block_length, subband_num, alloc_method, sample_rate, channel_mode) { \
                    BT_A2DP_CODEC_SBC, role, sizeof(bt_a2dp_sbc_codec_t), {{\
                    (channel_mode&0x0F) | (sample_rate&0x0F)<<4, \
                    (alloc_method&0x03) | (subband_num&0x03)<<2 | (block_length&0x0F)<<4, \
                    (min_bit_pool & 0xF) ,(min_bit_pool & 0xF)>>2,((min_bit_pool>>4) & 0xF), (max_bit_pool & 0xFF)}}}

const static bt_a2dp_codec_capability_t init_codec[] = {
    //{ 0, 1, 4, { 0xff, 0xff, 0x19, 0x4d } },
    //{ 2, 1, 6, { 0xc0, 0xff, 0x8c, 0xe0, 0x00, 0x00 } }
    BT_A2DP_MAKE_CODEC_SBC(BT_A2DP_SOURCE, 2, 75, 0x0f, 0x0f, 0x03, 0x0f, 0x0f),
    // BT_A2DP_MAKE_CODEC_AAC(BT_A2DP_SINK, 1, 0xC0, 0x03, 0x0ff8, 0x60000)
};
#endif /* #if 0 */
bt_hci_cmd_le_set_advertising_data_t adv_data, adv_data_default = {0};
bt_hci_cmd_le_set_scan_response_data_t scan_data, scan_data_default = {0};
//bt_hci_cmd_le_set_multi_advertising_data_t multi_adv_data, multi_adv_data_default = {0};
//bt_hci_cmd_le_set_multi_scan_response_data_t multi_scan_data, multi_scan_data_default = {0};

static uint8_t ut_app_reset_global_config_flag = true;
static uint8_t ut_app_reset_flash_flag = true;
bt_status_t (*ut_app_callback)(bt_msg_type_t, bt_status_t, void *) = NULL;
/* End of default configuration. */

bool bt_app_advertising = false;
bool bt_app_scanning = false;
bool bt_app_connecting = false;
bool bt_app_wait_peer_central_address_resolution_rsp = false;
#define BT_APP_RESOLVING_LIST_UPDATING 0x01
#define BT_APP_WHITE_LIST_UPDATING 0x02
uint8_t list_updating = 0;//combination of BT_APP_RESOLVING_LIST_UPDATING & BT_APP_WHITE_LIST_UPDATING

bt_status_t bt_app_gap_io_callback(void *input, void *output);
bt_status_t bt_app_sm_io_callback(void *input, void *output);
bt_status_t bt_app_l2cap_io_callback(void *input, void *output);
bt_status_t bt_app_gatts_io_callback(void *input, void *output);
bt_status_t bt_app_gattc_io_callback(void *input, void *output);
//bt_status_t bt_app_demo_io_callback(void *input, void *output);
bt_status_t bt_cmd_gattc_io_callback(void *input, void *output);

static const struct bt_app_callback_table_t {
    const char *name;
    bt_status_t (*io_callback)(void *, void *);
} bt_app_callback_table[] = {
    {"gap",     bt_app_gap_io_callback},
    {"sm",      bt_app_sm_io_callback},
    {"l2cap",   bt_app_l2cap_io_callback},
    {"gatts",   bt_app_gatts_io_callback},
    {"gattc",   bt_app_gattc_io_callback},
    //{"demo",    bt_app_demo_io_callback},
    {"gatt",    bt_cmd_gattc_io_callback},
};

/*Weak symbol declaration for l2cap */
bt_status_t bt_app_l2cap_io_callback(void *input, void *output);
bt_status_t default_bt_app_l2cap_io_callback(void *input, void *output)
{
    return BT_STATUS_SUCCESS;
}

#if _MSC_VER >= 1500
#pragma comment(linker, "/alternatename:_bt_app_l2cap_io_callback=_default_bt_app_l2cap_io_callback")
#elif defined(__GNUC__) || defined(__ICCARM__) || defined(__CC_ARM)
#pragma weak bt_app_l2cap_io_callback = default_bt_app_l2cap_io_callback
#else /* #if _MSC_VER >= 1500 */
#error "Unsupported Platform"
#endif /* #if _MSC_VER >= 1500 */

/*Weak symbol declaration for sm */
bt_status_t bt_app_sm_io_callback(void *input, void *output);
bt_status_t default_bt_app_sm_io_callback(void *input, void *output)
{
    return BT_STATUS_SUCCESS;
}

#if _MSC_VER >= 1500
#pragma comment(linker, "/alternatename:_bt_app_sm_io_callback=_default_bt_app_sm_io_callback")
#elif defined(__GNUC__) || defined(__ICCARM__) || defined(__CC_ARM)
#pragma weak bt_app_sm_io_callback = default_bt_app_sm_io_callback
#else /* #if _MSC_VER >= 1500 */
#error "Unsupported Platform"
#endif /* #if _MSC_VER >= 1500 */

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
#ifdef MTK_BLE_DM_SUPPORT
    bt_device_manager_le_gap_set_local_configuration(&local_key_req, sc_only);
    bt_device_manager_le_gap_set_pairing_configuration(&pairing_config_req);
#endif /* #ifdef MTK_BLE_DM_SUPPORT */

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
    //sc_only = sc_only_default; <-(rm run time change support)
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
    BT_LOGI("APP", "========================================");
    BT_LOGI("APP", "Addr:\t%s", bt_debug_addr2str(&report->address));
    BT_LOGI("APP", "Evt Type:\t%s", get_event_type(report->event_type));
    uint8_t count, ad_data_len, ad_data_type, ad_data_idx;
    count = 0;
    uint8_t buff[100] = {0};
    while (count < report->data_length) {
        if (count >= BT_HCI_LE_ADVERTISING_DATA_LENGTH_MAXIMUM - 2) {
            BT_LOGW("APP", "larger than report len (%u)", count);
            break;
        }
        ad_data_len = report->data[count];
        /* Error handling for data length over 30 bytes. */
        if (ad_data_len >= 0x1F) {
            BT_LOGI("APP", "AD Data Len Error");
            break;
        }
        count += 1;
        ad_data_type = report->data[count];
        count += 1;

        if (ad_data_type == BT_GAP_LE_AD_TYPE_FLAG) {
            if (report->data[count] & BT_GAP_LE_AD_FLAG_LIMITED_DISCOVERABLE) {
                BT_LOGI("APP", "AD Flags:\tLE Limited Discoverable Mode");
            } else if (report->data[count] & BT_GAP_LE_AD_FLAG_GENERAL_DISCOVERABLE) {
                BT_LOGI("APP", "AD Flags:\tLE General Discoverable Mode");
            } else {
                BT_LOGI("APP", "AD Flags:\tUnknown: 0x%02x", report->data[count]);
            }
            count += (ad_data_len - 1);
        } else if (ad_data_type == BT_GAP_LE_AD_TYPE_NAME_COMPLETE) {
            for (ad_data_idx = 0; ad_data_idx < (ad_data_len - 1); ad_data_idx++, count++) {
                buff[ad_data_idx] = report->data[count];
            }
            BT_LOGI("APP", "Name:\t%s", buff);
        } else {
            count += (ad_data_len - 1);
        }
    }
#if 0 //reduce logs on MT7933
    /* print raw data */
    printf("[I][APP] RAW DATA=0x");
    for (count = 0; count < report->data_length; count++) {
        printf("%02x", report->data[count]);
    }
    printf("\n");
#endif /* #if 0 //reduce logs on MT7933 */
#if defined(MTK_BT_LWIP_ENABLE)
    bt_lwip_send(report->data, report->data_length);
    bt_lwip_send("\r\n", 5);
#endif /* #if defined(MTK_BT_LWIP_ENABLE) */
    BT_LOGI("APP", "========================================");
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
            BT_LOGW("APP", "larger than report len (%u)", count);
            break;
        }

        ad_data_len = report->data[count];
        /* Error handling for data length over 30 bytes. */
        if (ad_data_len >= 0x1F) {
            BT_LOGI("APP", "AD Data Len Error");
            break;
        }
        count += 1;
        ad_data_type = report->data[count];
        count += 1;

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
        BT_LOGI("APP", "========================================");
        BT_LOGI("APP", "CL:10Address:\t%s", bt_debug_addr2str(&report->address));
        BT_LOGI("APP", "CL:10Event Type:\t%s", get_event_type(report->event_type));
        BT_LOGI("APP", "CL:10Complete Name:\t%s", buff);
    }

    BT_COLOR_SET(BT_COLOR_WHITE);
}

void ut_print_help(void)
{
    printf("Cmd line usage ex: (ble ?), (ble gap stop_scan)\n"
           "ble        -Common cmds\n"
           "  init                   -Init ble and power on BT\n"
           "  reset config [on/off]  -Reset/keep config in each cmd\n"
           "  reset flash [on/off]   -Reset/keep flash in each cmd\n"
           "  reset config           -Reset config\n"
           "  reset flash            -Reset flash\n"
           "  add resolving_list [peer_addr_type][peer_addr][peer_irk]\n"
           "                         -Add the device to resolving list\n"
           "  rl clear               -Clear the resolving list\n"
           "  set irk [16B key]      -Set the IRK\n"
           "  ar [on/off]            -Enable/disable addr resolution\n"
           "  set address_timeout [tout]  -Set resolvable private addr timeout\n"
           "  bond [con_hdl]         -Bond the remote device\n"
           "  remove bond [addr_type][addr]  -Remove bonding info array\n"
           "  sm passkey [key]       -Input the passkey\n"
           "  sm numeric compare [result]    -Input the numeric compare result\n"
           "  mitm on                -Enable the MITM protection\n"
           "  lesc on                -Enable LE secure connection\n"
           "  btdrv snoop [on/off]   -Enable/Disable hcisnoop log\n"
           "  boot mode [gatts/mesh] -Set mode to gatts/mesh, then reboot\n"
           "  [po/pf]                -Power on/off BT\n"
           "  set address_timeout [tout]  -Set addr timeout\n"
           "  random address [addr]  -Set random addr\n"
           "  advanced adv [own addr type][adv type][adv_filter_policy][peer addr type][peer BT addr]\n"
           "                         -Advanced adv set and start\n"
           "  list bond              -List all bonding info\n"
           "  list connection        -List all connection info\n"
           "  show status            -Show config, bonding, conn status, etc.\n"
           "  wifi smart             -start wifi smart adv\n"
           "  get local addr         -Get local public addr\n"
           "===============================================================================\n"
           "ble gap          -GAP command line\n"
           "  set_adv_data [data_buf]       -Set adv data\n"
           "  set_scan_rsp_data [data_buf]  -Set scan rsp data\n"
           "  set_adv_params [min_interval][max_interval][adv_type][own_addr_type][peer_addr_type][peer_addr][channel_map][filter_policy]\n"
           "                         -Set adv params\n"
           "  start_scan [scan_type][interval][window][own addr type][scan filter policy][filter duplicate][instant display report]\n"
           "                         -Start scan\n"
#ifdef BT_UT_GAP_LE_SCAN_LIST
           "  list_scan              -Dump the list of advertising report\n"
#endif /* #ifdef BT_UT_GAP_LE_SCAN_LIST */
           "  stop_scan              -Stop scan\n"
           "  connect [addr_type][address]  -Connect remote dev\n"
           "  advanced_conn [scan interval][scan window][init filter policy][peer addr_type][peer addr][own addr type][conn interval min][conn interval max][conn latency][super tout][min ce len][max ce len]\n"
           "                         -Connect remote dev by more params\n"
           "  update_conn [con_hdl][conn interval min][conn interval max][conn latency][super timeout][min ce len][max ce len]\n"
           "                         -Update connection\n"
           "  cancel connect         -Cancel connection\n"
           "  disconnect [con_hdl]   -Disconnect connection\n"
           "  bond [con_hdl][io cap][oob flag][auth req][init key][rsp key]  -Bond the remote dev\n"
           "  random_addr [addr]     -Set random addr\n"
           "  read_rssi [con_hdl]    -Read RSSI\n"
           "  wl [add/remove] [0:pub/1:rand][addr]     -Add/Remove the dev in white list\n"
           "  wl clear               -Clear the white list\n"
           "  update data length [con_hdl][tx octets][tx time]  -Update data len\n"
           "  get_adv_instance       -Get max mul adv instances\n"
           "  set_phy [con_hdl][tx phy][rx phy][tx opts]    -Set phy info\n"
           "  get_phy [con_hdl]      -Get phy info\n"
          );
}

void copy_str_to_addr(uint8_t *addr, const char *str)
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

    for (i = 0; i < 6; i++) {
        icn = sscanf(str + using_hex_sign + i * (2 + using_long_format), "%02x", &value);
        if (icn < 0)
            return;
        addr[5 - i] = (uint8_t) value;
    }
}

void copy_str_to_byte(uint8_t *des, const char *str, uint32_t len)
{
    unsigned int i, value = 0;
    int using_long_format = 0;
    int using_hex_sign = 0;
    int icn = 0;

    if (str[2] == ':' || str[2] == '-') {
        using_long_format = 1;
    }

    if (str[1] == 'x') {
        using_hex_sign = 2;
    }

    for (i = 0; i < len; i++) {
        icn = sscanf(str + using_hex_sign + i * (2 + using_long_format), "%02x", &value);
        if (icn < 0)
            return;
        des[i] = (uint8_t) value;
        value = 0;
    }
}

static uint32_t sm_passkey;
static uint8_t nc_value_correct[1];

bt_status_t app_clear_resolving_list(void)
{
    bt_status_t st = BT_STATUS_SUCCESS;
    st = bt_gap_le_set_resolving_list(BT_GAP_LE_CLEAR_RESOLVING_LIST, NULL);
    if (BT_STATUS_OUT_OF_MEMORY == st) {
        BT_COLOR_SET(BT_COLOR_RED);
        BT_LOGI("APP", "Add device to Resolving List Failed [OOM]");
        BT_COLOR_SET(BT_COLOR_WHITE);
    }
    return st;
}
bt_status_t app_delete_dev_from_resolving_list(const bt_gap_le_bonding_info_t *bonded_info)
{
    bt_status_t st = BT_STATUS_SUCCESS;
    if (BT_ADDR_TYPE_UNKNOW != bonded_info->identity_addr.address.type) {
        bt_hci_cmd_le_remove_device_from_resolving_list_t dev;
        dev.peer_identity_address = bonded_info->identity_addr.address;
        st = bt_gap_le_set_resolving_list(BT_GAP_LE_REMOVE_FROM_RESOLVING_LIST, (void *)&dev);
        if (BT_STATUS_OUT_OF_MEMORY == st) {
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "Add device to Resolving List Failed [OOM]");
            BT_COLOR_SET(BT_COLOR_WHITE);
        }
    }
    return st;
}
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
            BT_LOGI("APP", "Add device to Resolving List Failed [OOM]");
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
        BT_LOGI("APP", "CL:10Add device to White List Failed [OOM]");
        BT_COLOR_SET(BT_COLOR_WHITE);
    }
    return st;
}

static bool g_bt_hci_log_enable = true;

bool bt_hci_log_enabled(void)
{
    return g_bt_hci_log_enable;
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

bt_status_t bt_vendor_cmd_set_tx_power_cbk(uint32_t is_timeout, uint32_t timer_id, uint32_t data, const void *arg)
{
    BT_LOGI("APP", "Set tx power level, %s", is_timeout ? "Cmd timeout!" : "Success");
    return BT_STATUS_SUCCESS;
}

void bt_app_common_init(void)
{
#ifdef MTK_BLE_DM_SUPPORT
    bt_device_manager_le_init();
    bt_device_manager_le_gap_set_local_configuration(&local_key_req, sc_only);
    bt_device_manager_le_gap_set_pairing_configuration(&pairing_config_req);
#endif /* #ifdef MTK_BLE_DM_SUPPORT */
}

bt_status_t bt_app_io_callback(void *input, void *output)
{
    char *cmd = (char *)input;
    unsigned long ulcn = 0;

    BT_LOGI("APP", "bt_app_io_callback cli_cmd: %s", cmd);
    if (bt_app_cmd_in_protected_list(cmd)) {
        if (bt_app_power_state_get() != BT_APP_POWER_STATE_POWER_ON) {
            BT_LOGE("APP", "BT is not in power ON state(state = %d).", bt_app_power_state_get());
            return BT_STATUS_SUCCESS;
        }
    }

    if (ut_app_reset_global_config_flag) {
        ut_app_reset_global_config();
    }

    if (ut_app_reset_flash_flag) {
        ut_app_reset_flash();

    }

    if (UT_APP_CMP("?")) {
        ut_print_help();
        return BT_STATUS_SUCCESS;
    } else if (UT_APP_CMP("init")) {
        if (bt_app_power_state_get() == BT_APP_POWER_STATE_INIT_NONE) {
            bt_app_power_state_set(BT_APP_POWER_STATE_INIT_START);
            init_connection_info();
            bt_create_task();
        } else {
            BT_LOGI("APP", "bt already init, only need once init");
        }
    } else if (UT_APP_CMP("hci on")) {
        g_bt_hci_log_enable = true;
    } else if (UT_APP_CMP("hci off")) {
        g_bt_hci_log_enable = false;
    } else if (UT_APP_CMP("assert")) {
        assert(0);
    } else if (UT_APP_CMP("reset config off")) {
        ut_app_reset_global_config_flag = false;
    }

    else if (UT_APP_CMP("reset flash off")) {
        ut_app_reset_flash_flag = false;
    }
    /* Usage: set pts_addr [pts address]*/
    else if (UT_APP_CMP("set pts_addr")) {
        const char *addr_str = cmd + 13;
        copy_str_to_addr(lt_addr, addr_str);
        BT_LOGI("APP", "change to lt_addr: %x-%x-%x-%x-%x-%x", lt_addr[5], lt_addr[4], lt_addr[3], lt_addr[2], lt_addr[1], lt_addr[0]);
    } else if (UT_APP_CMP("reset config on")) {
        ut_app_reset_global_config_flag = true;
    }

    else if (UT_APP_CMP("reset flash on")) {
        ut_app_reset_flash_flag = true;
    }

    else if (UT_APP_CMP("reset config")) {
        ut_app_reset_global_config();
    }

    else if (UT_APP_CMP("reset flash")) {
        ut_app_reset_flash();
    }

    else if (UT_APP_CMP("po")) {
        if (bt_app_power_state_get() == BT_APP_POWER_STATE_POWER_OFF) {
            bt_app_power_state_set(BT_APP_POWER_STATE_POWER_ON_START);
            bt_power_on((bt_bd_addr_ptr_t)&local_public_addr, NULL);
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
    } else if (UT_APP_CMP("tx power")) {
        if (strlen(cmd) != 33) {
            BT_LOGI("APP", "cmd usage: ble tx power <BR/EDR tx_power> <BLE default tx_power> <BR/EDR max power> <enable_lv9> <diff mode>");
            BT_LOGI("APP", "eg: ble tx power 0x11 0x11 0x14 0x01 0x00");
            return BT_STATUS_FAIL;
        } else if (bt_app_power_state_get() != BT_APP_POWER_STATE_POWER_ON) {
            BT_LOGI("APP", "not in power on state");
            return BT_STATUS_FAIL;
        }
        uint8_t power_setting[8] = {0};

        power_setting[1] = 0x00; // Don't change or get advice from FW owner.
        power_setting[2] = 0x00; // Don't change or get advice from FW owner.
        power_setting[3] = 0x00; // Don't change or get advice from FW owner.

        power_setting[0] = (uint8_t)strtoul(cmd + 9, NULL, 16); //BR/EDR init power(dBm)
        power_setting[4] = (uint8_t)strtoul(cmd + 14, NULL, 16); //BLE default power(dBm).
        power_setting[5] = (uint8_t)strtoul(cmd + 19, NULL, 16); //BR/EDR maximum power(dBm).
        power_setting[6] = (uint8_t)strtoul(cmd + 24, NULL, 16); //lv9 enable; "0" means disable lv9.
        power_setting[7] = (uint8_t)strtoul(cmd + 29, NULL, 16); //Diff mode. "0" means 3dB diff between BR/EDR and BLE. "1" means 0 diff mode.

        bt_hci_send_vendor_cmd_ex(0xFC2C, power_setting, 8, 0, bt_vendor_cmd_set_tx_power_cbk);
    }

    /* Usage: advanced po [public address] [random address].
       Note:  Set N if you doesn't need it. */
    else if (UT_APP_CMP("advanced po")) {
        if (strlen(cmd) >= 12) {
            uint8_t public_addr[6] = {0};
            uint8_t random_addr[6] = {0};
            const char *addr_str = cmd + 12;

            /* Find public address */
            if (strncmp("N", addr_str, 1) != 0) {
                copy_str_to_addr(public_addr, addr_str);
            } else {
                public_addr[0] = 'N';
            }

            /* Jump to the start of the random address */
            uint32_t i = 0;
            while (i < 18) {
                if (strncmp(" ", addr_str, 1) == 0)
                    break;
                addr_str++;
                i++;
            }
            addr_str++;

            /* Find random address */
            if (strncmp("N", addr_str, 1) != 0) {
                copy_str_to_addr(random_addr, addr_str);
            } else {
                random_addr[0] = 'N';
            }

            bt_power_on((public_addr[0] == 'N' ? NULL : public_addr),
                        (random_addr[0] == 'N' ? NULL : random_addr));
        } else {
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGW("APP", "please input the specific public address and random address");
            BT_LOGW("APP", "format: advanced po [public address/N] [random address/N]");
            BT_COLOR_SET(BT_COLOR_WHITE);
        }
    } else if (UT_APP_CMP("set address_timeout")) {
        if (strlen(cmd) >= 20) {
            uint16_t timeout = (uint16_t)strtoul(cmd + 20, NULL, 10);
            BT_LOGW("APP", "timeout = %u", timeout);
            bt_gap_le_set_resolvable_private_address_timeout(timeout);
        }
    } else if (UT_APP_CMP("set irk")) {
        if (strlen((char *)cmd) >= 8) {
            const char *key = cmd + 8;
            copy_str_to_byte(local_key_req_default.identity_info.irk, key, BT_KEY_SIZE);
            local_key_req.identity_info = local_key_req_default.identity_info;
#ifdef MTK_BLE_DM_SUPPORT
            bt_device_manager_le_gap_set_local_configuration(&local_key_req, sc_only);
#endif /* #ifdef MTK_BLE_DM_SUPPORT */

        }
    } else if (UT_APP_CMP("add resolving_list")) {
        if (strlen(cmd) >= 34) {
            uint8_t addr_type = (uint8_t)strtoul(cmd + 19, NULL, 10);
            const char *addr_str = cmd + 21;
            const char *key = cmd + 39;
            if (addr_type != 0 && addr_type != 1) {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGW("APP", "add resolving_list [0:public_indentity / 1:random_identity] [bt address] [irk]");
                BT_COLOR_SET(BT_COLOR_WHITE);
            } else {
                uint8_t addr[6];
                bt_hci_cmd_le_add_device_to_resolving_list_t dev;
                copy_str_to_addr(addr, addr_str);
                dev.peer_identity_address.type = addr_type;
                memcpy(dev.peer_identity_address.addr, addr, sizeof(addr));
                copy_str_to_byte((uint8_t *)(&(dev.peer_irk)), key, 16);
                memcpy(dev.local_irk, &(local_key_req.identity_info), sizeof(dev.local_irk));
                bt_gap_le_set_resolving_list(BT_GAP_LE_ADD_TO_RESOLVING_LIST, (void *)&dev);
                if (BT_STATUS_OUT_OF_MEMORY == bt_gap_le_set_resolving_list(BT_GAP_LE_ADD_TO_RESOLVING_LIST, (void *)&dev)) {
                    BT_COLOR_SET(BT_COLOR_RED);
                    BT_LOGI("APP", "Add device to Resolving List Failed [OOM]");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                }
            }
        } else {
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGW("APP", "add resolving_list [2:public_indentity / 3:random_identity] [bt address] [irk]");
            BT_COLOR_SET(BT_COLOR_WHITE);
        }
    }
    /* update peer Central Address Resolution supporting */
    else if (UT_APP_CMP("check peer CAR supporting")) {
        const char *handle = cmd + 26;
        bt_gattc_read_using_charc_uuid_req_t req;
        uint16_t uuid = BT_SIG_UUID16_CENTRAL_ADDRESS_RESOLUTION;
        req.opcode = BT_ATT_OPCODE_READ_BY_TYPE_REQUEST;
        req.starting_handle = 0x0001;
        req.ending_handle = 0xffff;
        bt_uuid_load(&req.type, (void *)&uuid, 2);
        ulcn = strtoul(handle, NULL, 16);
        if (ulcn > 0xFFFF) {
            BT_LOGE("APP", "check peer CAR supporting, handle should be 0x0000 ~ 0xFFFF");
            return BT_STATUS_FAIL;
        }
        bt_gattc_read_using_charc_uuid((uint16_t)ulcn, &req);
        bt_app_wait_peer_central_address_resolution_rsp = true;
    }

    /* Usage: random address [random address].
       Note:  [random address] should be existed. */
    else if (UT_APP_CMP("random address")) {
        if (strlen(cmd) >= 15) {
            const char *addr_str = cmd + 15;
            uint8_t addr[6];
            copy_str_to_addr(addr, addr_str);

            bt_gap_le_set_random_address(addr);
        } else {
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGW("APP", "please input the specific random address");
            BT_LOGW("APP", "random address [random address]");
            BT_COLOR_SET(BT_COLOR_WHITE);
        }
    }

    else if (UT_APP_CMP("ar on")) {
        /* Set address resolition enable*/
        bt_gap_le_set_address_resolution_enable(1);
    } else if (UT_APP_CMP("ar off")) {
        /* Set address resolition disable*/
        bt_gap_le_set_address_resolution_enable(0);
    } else if (UT_APP_CMP("rl add")) {
        uint8_t idx = (uint8_t)strtoul(cmd + 7, NULL, 10);
        app_bt_bonded_info_t *app_bonded_info = NULL;
        app_bonded_info = find_bonded_info_by_index(idx);
        if (app_bonded_info != NULL) {
            bt_gap_le_bonding_info_t *bonded_info = &(app_bonded_info->info);
            //remove device from resolving list
            if (BT_STATUS_SUCCESS != app_add_dev_2_resolving_list(bonded_info)) {
                BT_LOGE("APP", "Add Device to Resolving List FAILED!!!");
            }
        } else {
            BT_LOGE("APP", "Can not find the bonded info idx[%d]. Please use \"list bond\" to check bonded info.", idx);
        }
    } else if (UT_APP_CMP("rl remove")) {
        uint8_t idx = (uint8_t)strtoul(cmd + 10, NULL, 10);
        app_bt_bonded_info_t *app_bonded_info = NULL;
        app_bonded_info = find_bonded_info_by_index(idx);
        if (app_bonded_info != NULL) {
            bt_gap_le_bonding_info_t *bonded_info = &(app_bonded_info->info);
            //remove device from resolving list
            if (BT_STATUS_SUCCESS != app_delete_dev_from_resolving_list(bonded_info)) {
                BT_LOGE("APP", "Remove Device from Resolving List FAILED!!!");
            }
        } else {
            BT_LOGE("APP", "Can not find the bonded info idx[%d]. Please use \"list bond\" to check bonded info.", idx);
        }
    } else if (UT_APP_CMP("rl clear")) {
        app_clear_resolving_list();
    }
    /* Usage: wl add [0:public / 1:random] [bt address].
       Note:  default use #lt_addr_type and #lt_addr */
    else if (UT_APP_CMP("wl add")) {
        bt_addr_t device;
        if (strlen(cmd) >= 7) {

            uint8_t addr_type = (uint8_t)strtoul(cmd + 7, NULL, 10);

            if (addr_type != 0 && addr_type != 1) {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGW("APP", "please input the correct address type");
                BT_LOGW("APP", "wl add [0:public / 1:random] [bt address]");
                BT_COLOR_SET(BT_COLOR_WHITE);
            } else {
                const char *addr_str = cmd + 9;
                uint8_t addr[6];
                copy_str_to_addr(addr, addr_str);

                device.type = addr_type;
                memcpy(device.addr, addr, sizeof(addr));
                bt_gap_le_set_white_list(BT_GAP_LE_ADD_TO_WHITE_LIST, &device);
            }
        } else {
            device.type = lt_addr_type;
            memcpy(device.addr, lt_addr, sizeof(lt_addr));
            bt_gap_le_set_white_list(BT_GAP_LE_ADD_TO_WHITE_LIST, &device);
        }
    }

    /* Usage: wl remove [0:public / 1:random] [bt address].
       Note:  default use #lt_addr_type and #lt_addr */
    else if (UT_APP_CMP("wl remove")) {
        bt_addr_t device;
        if (strlen(cmd) >= 10) {

            uint8_t addr_type = (uint8_t)strtoul(cmd + 10, NULL, 10);
            if (addr_type != 0 && addr_type != 1) {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGW("APP", "please input the correct address type");
                BT_LOGW("APP", "wl add [0:public / 1:random] [bt address]");
                BT_COLOR_SET(BT_COLOR_WHITE);
            } else {
                const char *addr_str = cmd + 12;
                uint8_t addr[6];
                copy_str_to_addr(addr, addr_str);

                device.type = addr_type;
                memcpy(device.addr, addr, sizeof(addr));
                bt_gap_le_set_white_list(BT_GAP_LE_REMOVE_FROM_WHITE_LIST, &device);
            }
        } else {
            device.type = lt_addr_type;
            memcpy(device.addr, lt_addr, sizeof(lt_addr));
            bt_gap_le_set_white_list(BT_GAP_LE_REMOVE_FROM_WHITE_LIST, &device);
        }
    }

    else if (UT_APP_CMP("wl clear")) {
        bt_gap_le_set_white_list(BT_GAP_LE_CLEAR_WHITE_LIST, NULL);
    }
    /*advanced scan [scan type] [Own Address Type] [Scanning Filter Policy]
    */
    else if (UT_APP_CMP("advanced scan")) {
        uint8_t scan_type;
        uint8_t own_address_type;
        uint8_t policy;

        ulcn = strtoul(cmd + 14, NULL, 10);
        if (ulcn > 1) {
            BT_LOGE("APP", "advanced scan, scan_type should be 0 or 1");
            return BT_STATUS_FAIL;
        }
        scan_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 16, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "advanced scan, own_address_type should be 0 ~ 3");
            return BT_STATUS_FAIL;
        }
        own_address_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 18, NULL, 10);
        if (ulcn > 4) {
            BT_LOGE("APP", "advanced scan, policy should be 0 ~ 4");
            return BT_STATUS_FAIL;
        }
        policy = (uint8_t)ulcn;

        BT_COLOR_SET(BT_COLOR_BLUE);
        BT_LOGI("APP", "Advanced Scan test");
        BT_LOGI("APP", "Scan Type[%d] Own Address Type[%d] Scanning Filter Policy[%d]\n", scan_type, own_address_type, policy);
        BT_COLOR_SET(BT_COLOR_WHITE);
        scan_para.le_scan_type = scan_type,
        scan_para.own_address_type = own_address_type,
        scan_para.scanning_filter_policy = policy,
        bt_app_scanning = true;
        bt_gap_le_set_scan(&scan_enable, &scan_para);
    } else if (UT_APP_CMP("gap dump")) {
        bt_gap_dump();
    }
#ifdef BT_DEBUG
    else if (UT_APP_CMP("bt debug cmd")) {
        uint32_t length = strlen("bt debug cmd ");
        unsigned int temp_value;
        uint32_t i = 0;
        int icn = 0;
        uint8_t *cmd_data_buff = (uint8_t *)cmd;
        uint8_t value[4] = {*(cmd + length), *(cmd + length + 1), 0};
        while (value[0] != 0) {
            icn = sscanf((const char *)value, "%02x", &temp_value);
            if (icn < 0)
                return BT_STATUS_FAIL;
            *(cmd_data_buff + (i >> 1)) = (uint8_t)temp_value;
            i += 2;
            value[0] = *(cmd + length + i);
            value[1] = *(cmd + length + i + 1);
        }
        bt_gap_debug_cmd_sending(cmd_data_buff);
    }
#endif /* #ifdef BT_DEBUG */

#if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE)) && defined(__MTK_BT_MESH_ENABLE__)//only mesh or only gatts not need switch
    else if (UT_APP_CMP("boot mode")) {
        bool save_ok;
        bt_boot_mode_t user_mode;

        if (UT_APP_CMP("boot mode mesh")) {
            BT_LOGI("APP", "boot mode mesh");
            user_mode = BT_BOOT_INIT_MODE_MESH;
        } else if (UT_APP_CMP("boot mode gatts")) {
            BT_LOGI("APP", "boot mode gatts");
            user_mode = BT_BOOT_INIT_MODE_GATTS;
        } else {
            BT_LOGI("APP", "mode not found");
            return BT_STATUS_SUCCESS;
        }

        if (bt_get_init_mode() != user_mode) {
            save_ok = bt_save_init_mode((uint8_t)user_mode);
            if (save_ok)
                bt_reboot_system();
            else
                BT_LOGI("APP", "boot mode, nvm save fail!");
        } else
            BT_LOGI("APP", "not need switch mode");
    }
#endif /* #if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE)) && defined(__MTK_BT_MESH_ENABLE__)//only mesh or only gatts not need switch */

    else if (UT_APP_CMP("scan on")) {
        bt_app_scanning = true;
        bt_gap_le_set_scan(&scan_enable, &scan_para);
    }

    else if (UT_APP_CMP("scan off")) {
        bt_app_scanning = false;
        bt_gap_le_set_scan(&scan_disable, NULL);
    }

    else if (UT_APP_CMP("adv on")) {
        bt_app_advertising = true;
        adv_enable.advertising_enable = BT_HCI_ENABLE;
        bt_gap_le_set_advertising(&adv_enable, &adv_para, NULL, NULL);
    }

    else if (UT_APP_CMP("adv off")) {
        bt_app_advertising = false;
        bt_hci_cmd_le_set_advertising_enable_t enable;
        enable.advertising_enable = BT_HCI_DISABLE;
        bt_gap_le_set_advertising(&enable, NULL, NULL, NULL);
    }

    else if (UT_APP_CMP("single adv off")) {
        bt_app_advertising = false;
        bt_hci_cmd_le_set_advertising_enable_t enable;
        enable.advertising_enable = BT_HCI_DISABLE;
        bt_gap_le_set_advertising_single(&enable, NULL, NULL, NULL);
    }

    /*single adv [own addr type] [adv type] [advertising_filter_policy] [peer addr type] [peer BT addr]
    [own addr type] :0:public / 1:random/ 2: Gen RPA from resolving list or public address host provide/ 3: Gen RPA from resolving list or static random address host provide
    [adv type] : 0:ADV_IND, 1:ADV_DIRECT_IND high duty cycle, 2: ADV_SCAN_IND, 3:ADV_NONCONN_IND or 4.ADV_DIRECT_IND low duty cycle.
    [peer addr type]:0:public / 1:random
    [advertising_filter_policy]: define in spec, 0~4
    [peer addr type] : Chck src/hbif/bt_gap_le_spec.h BT_GAP_LE_AD_xxxx 0~4
    [peer BT Addr] : peer BT address for BT_GAP_LE_AD_CONNECTABLE_DIRECTED_HIGH or BT_GAP_LE_AD_CONNECTABLE_DIRECTED_LOW
    Hint: for [peer addr type] and [peer BT addr], you can refer bond info for the device we had bonded before.
    */
    else if (UT_APP_CMP("single adv")) {
        bt_hci_cmd_le_set_advertising_data_t adv_data = {
            .advertising_data_length = 25,
            .advertising_data = "DDDDDHUMMINGBIRD_ADV_DATA",
        };
        bt_hci_cmd_le_set_scan_response_data_t scan_data = {
            .scan_response_data_length = 23,
            .scan_response_data = "DDSCAN_DATA_HUMMINGBIRD",
        };
        bt_app_advertising = true;
        memset(gatts_device_name, 0x00, sizeof(gatts_device_name));
        memcpy(gatts_device_name, &adv_data.advertising_data[5], 11);
        gap_appearance = 0x4567;
        adv_data.advertising_data[0] = 2; //adv_length
        adv_data.advertising_data[1] = BT_GAP_LE_AD_TYPE_FLAG;
        adv_data.advertising_data[2] = BT_GAP_LE_AD_FLAG_BR_EDR_NOT_SUPPORTED | BT_GAP_LE_AD_FLAG_GENERAL_DISCOVERABLE;
        adv_data.advertising_data[3] = 21; //adv_length
        adv_data.advertising_data[4] = 0x09;
        scan_data.scan_response_data[0] = 22; /* ADV length. */
        scan_data.scan_response_data[1] = 0x08;
        uint8_t own_addr_type;
        uint8_t adv_type;
        uint8_t policy;
        uint8_t peer_addr_type;
        const char *addr_str = cmd + 19;
        uint8_t addr[6];

        ulcn = strtoul(cmd + 11, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "single adv, own_addr_type should be 0 ~ 3");
            return BT_STATUS_FAIL;
        }
        own_addr_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 13, NULL, 10);
        if (ulcn > 4) {
            BT_LOGE("APP", "single adv, adv_type should be 0 ~ 4");
            return BT_STATUS_FAIL;
        }
        adv_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 15, NULL, 10);
        if (ulcn > 4) {
            BT_LOGE("APP", "single adv, policy should be 0 ~ 4");
            return BT_STATUS_FAIL;
        }
        policy = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 17, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "single adv, peer_addr_type should be 0 ~ 3");
            return BT_STATUS_FAIL;
        }
        peer_addr_type = (uint8_t)ulcn;

        copy_str_to_addr(addr, addr_str);
        BT_COLOR_SET(BT_COLOR_BLUE);
        BT_LOGI("APP", "Advanced advertising test");
        BT_LOGI("APP", "own_addr_type[%d] adv_type[%d] adv_policy[%d] peer_addr_type[%d]",
                own_addr_type, adv_type, policy, peer_addr_type);
        BT_LOGI("APP", "peer_addr(%02x:%02x:%02x:%02x:%02x:%02x)",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        BT_COLOR_SET(BT_COLOR_WHITE);

        adv_enable.advertising_enable = true;

        adv_para.advertising_interval_min = ADVERTISING_INTERVAL_MIN;
        adv_para.advertising_interval_max = ADVERTISING_INTERVAL_MAX;
        adv_para.advertising_type = adv_type;
        adv_para.own_address_type = own_addr_type;
        adv_para.peer_address.type = peer_addr_type;
        memcpy(adv_para.peer_address.addr, &addr, 6);
        adv_para.advertising_channel_map = 7;
        adv_para.advertising_filter_policy = policy;
        if ((adv_para.advertising_type == ADVERTISING_TYPE_DIRECT_IND_HIGH_DUTY) ||
            (adv_para.advertising_type == ADVERTISING_TYPE_DIRECT_IND_LOW_DUTY)) {
            bt_gap_le_set_advertising_single(&adv_enable, &adv_para, &adv_data, NULL);
        } else {
            bt_gap_le_set_advertising_single(&adv_enable, &adv_para, &adv_data, &scan_data);
        }
    }

    /*advanced adv [own addr type] [adv type] [advertising_filter_policy] [peer addr type] [peer BT addr]
      [own addr type] :0:public / 1:random/ 2: Gen RPA from resolving list or public address host provide/ 3: Gen RPA from resolving list or static random address host provide
      [adv type] : 0:ADV_IND, 1:ADV_DIRECT_IND high duty cycle, 2: ADV_SCAN_IND, 3:ADV_NONCONN_IND or 4.ADV_DIRECT_IND low duty cycle.
      [peer addr type]:0:public / 1:random
      [advertising_filter_policy]: define in spec, 0~4
      [peer addr type] : Chck src/hbif/bt_gap_le_spec.h BT_GAP_LE_AD_xxxx 0~4
      [peer BT Addr] : peer BT address for BT_GAP_LE_AD_CONNECTABLE_DIRECTED_HIGH or BT_GAP_LE_AD_CONNECTABLE_DIRECTED_LOW
      Hint: for [peer addr type] and [peer BT addr], you can refer bond info for the device we had bonded before.
     */
    else if (UT_APP_CMP("advanced adv")) {
        bt_hci_cmd_le_set_advertising_data_t adv_data = {
            .advertising_data_length = 31,
            .advertising_data = "DDDDDHUMMINGBIRD_ADV_DATA",
        };
        bt_hci_cmd_le_set_scan_response_data_t scan_data = {
            .scan_response_data_length = 31,
            .scan_response_data = "DDSCAN_DATA_HUMMINGBIRD",
        };
        bt_app_advertising = true;
        memset(gatts_device_name, 0x00, sizeof(gatts_device_name));
        memcpy(gatts_device_name, &adv_data.advertising_data[5], 11);
        gap_appearance = 0x4567;
        adv_data.advertising_data[0] = 2; //adv_length
        adv_data.advertising_data[1] = BT_GAP_LE_AD_TYPE_FLAG;
        adv_data.advertising_data[2] = BT_GAP_LE_AD_FLAG_BR_EDR_NOT_SUPPORTED | BT_GAP_LE_AD_FLAG_GENERAL_DISCOVERABLE;
        adv_data.advertising_data[3] = 21; //adv_length
        adv_data.advertising_data[4] = 0x09;
        scan_data.scan_response_data[0] = 22; /* ADV length. */
        scan_data.scan_response_data[1] = 0x08;
        uint8_t own_addr_type;
        uint8_t adv_type;
        uint8_t policy;
        uint8_t peer_addr_type;
        const char *addr_str = cmd + 21;
        uint8_t addr[6];

        ulcn = strtoul(cmd + 13, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "advanced adv, own_addr_type should be 0 ~ 3");
            return BT_STATUS_FAIL;
        }
        own_addr_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 15, NULL, 10);
        if (ulcn > 4) {
            BT_LOGE("APP", "advanced adv, adv_type should be 0 ~ 4");
            return BT_STATUS_FAIL;
        }
        adv_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 17, NULL, 10);
        if (ulcn > 4) {
            BT_LOGE("APP", "advanced adv, policy should be 0 ~ 4");
            return BT_STATUS_FAIL;
        }
        policy = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 19, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "advanced adv, peer_addr_type should be 0 ~ 3");
            return BT_STATUS_FAIL;
        }
        peer_addr_type = (uint8_t)ulcn;

        copy_str_to_addr(addr, addr_str);
        BT_COLOR_SET(BT_COLOR_BLUE);
        BT_LOGI("APP", "Advanced advertising test");
        BT_LOGI("APP", "own_addr_type[%d] adv_type[%d] adv_policy[%d] peer_addr_type[%d]",
                own_addr_type, adv_type, policy, peer_addr_type);
        BT_LOGI("APP", "peer_addr(%02x:%02x:%02x:%02x:%02x:%02x)",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        BT_COLOR_SET(BT_COLOR_WHITE);

        adv_enable.advertising_enable = true;

        adv_para.advertising_interval_min = 0x0800;
        adv_para.advertising_interval_max = 0x1000;
        adv_para.advertising_type = adv_type;
        adv_para.own_address_type = own_addr_type;
        adv_para.peer_address.type = peer_addr_type;
        memcpy(adv_para.peer_address.addr, &addr, 6);
        adv_para.advertising_channel_map = 7;
        adv_para.advertising_filter_policy = policy;
        if ((adv_para.advertising_type == 1) || (adv_para.advertising_type == 4)) {
            bt_gap_le_set_advertising(&adv_enable, &adv_para, NULL, NULL);
        } else {
            bt_gap_le_set_advertising(&adv_enable, &adv_para, &adv_data, &scan_data);
        }
    }

    /*advanced connect [Initiator_Filter_Policy] [Own_Address_Type] [Peer_Address_Type] [Peer_Address]
      [Initiator_Filter_Policy] :0;white list is not used. 1;white list is used.
      [Own_Address_Type] : 0~4;Public/Random/RPA or Public/RPA or Random
      [Peer_Address_Type] : 0~4; Public/Random/Public Identity/Random Identity
      [Peer_Address] :
      Test case command for Privacy 1.2:
      [ar on]
      advanced connect 0 2 2 [Peer Identity Address]
      advanced connect 1 2 0 0x000000000000
     */
    else if (UT_APP_CMP("advanced connect")) {
        uint8_t policy;
        uint8_t own_address_type;
        uint8_t peer_address_type;
        const char *addr_str;
        uint8_t addr[6];

        ulcn = strtoul(cmd + 17, NULL, 10);
        if (ulcn > 4) {
            BT_LOGE("APP", "advanced connect, policy should be 0 ~ 4");
            return BT_STATUS_FAIL;
        }
        policy = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 19, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "advanced connect, own_address_type should be 0 ~ 3");
            return BT_STATUS_FAIL;
        }
        own_address_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 21, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "advanced connect, peer_address_type should be 0 ~ 3");
            return BT_STATUS_FAIL;
        }
        peer_address_type = (uint8_t)ulcn;

        addr_str = cmd + 23;
        copy_str_to_addr(addr, addr_str);
        BT_COLOR_SET(BT_COLOR_BLUE);
        BT_LOGI("APP", "Advanced connect ");
        BT_LOGI("APP", "Initiator_Filter_Policy[%d] Own_Address_Type[%d] Peer_Address_Type[%d]",
                policy, own_address_type, peer_address_type);
        BT_LOGI("APP", "peer_addr(%02x:%02x:%02x:%02x:%02x:%02x)",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        BT_COLOR_SET(BT_COLOR_WHITE);

        connect_para.initiator_filter_policy = policy;
        connect_para.own_address_type = own_address_type;
        connect_para.peer_address.type = peer_address_type;
        memcpy(connect_para.peer_address.addr, addr, sizeof(addr));

        bt_gap_le_connect(&connect_para);
    }

    /* Usage: connect [0:public / 1:random] [bt address].
       Note:  default use #lt_addr_type and #lt_addr */
    else if (UT_APP_CMP("connect")) {
        if (strlen(cmd) >= 8) {
            const char *addr_str = cmd + 10;
            uint8_t addr[6];
            uint8_t peer_addr_type;

            ulcn = strtoul(cmd + 8, NULL, 10);
            if (ulcn > 3) {
                BT_LOGE("APP", "disconnect, addr type should be 0 ~ 3");
                return BT_STATUS_FAIL;
            }
            peer_addr_type = (uint8_t)ulcn;

            copy_str_to_addr(addr, addr_str);
#ifdef BLE_THROUGHPUT
            //const char *conn_interval = cmd + 23;
            enable_dle = (uint8_t)strtoul(cmd + 23, NULL, 10);
            uint16_t interval_conn = (uint16_t)strtoul(cmd + 25, NULL, 10);
            //uint16_t interval_conn = (uint16_t)strtoul(cmd + 23, NULL, 10);
            connect_para.conn_interval_min = interval_conn;
            connect_para.conn_interval_max = interval_conn;
            // 0x50 is for BLE4.2
            //connect_para.conn_interval_min = 0x50;
            //connect_para.conn_interval_max = 0x50;
#endif /* #ifdef BLE_THROUGHPUT */
            connect_para.peer_address.type = peer_addr_type;
            memcpy(connect_para.peer_address.addr, addr, sizeof(addr));
            bt_gap_le_connect(&connect_para);
        } else {
            connect_para.peer_address.type = lt_addr_type;
            memcpy(connect_para.peer_address.addr, lt_addr, sizeof(lt_addr));
            bt_gap_le_connect(&connect_para);
        }
    }

    else if (UT_APP_CMP("cancel connect")) {
        bt_gap_le_cancel_connection();
    }

    /* Usage:   disconnect <handle in hex>
       Example: disconnect 0200 */
    else if (UT_APP_CMP("disconnect")) {
        const char *handle = cmd + strlen("disconnect ");
        ulcn = strtoul(handle, NULL, 16);
        if (ulcn > 0xFFFF) {
            BT_LOGE("APP", "disconnect, handle should be 0x0000 ~ 0xFFFF");
            return BT_STATUS_FAIL;
        }
        disconnect_para.connection_handle = (uint16_t)ulcn;
        BT_LOGI("APP", "connection_handle(0x%04x)", disconnect_para.connection_handle);
        bt_gap_le_disconnect(&disconnect_para);
    }

    else if (UT_APP_CMP("read rssi")) {
        bt_gap_le_read_rssi(&read_rssi);
    }

    else if (UT_APP_CMP("update conn param")) {
        bt_gap_le_update_connection_parameter(&conn_update_para);
    }

    /* Usage: update data length <handle in hex> <tx octets in hex> <tx time in hex>.
       Example: update data length 0200 0030 0500*/
    else if (UT_APP_CMP("update data length")) {
        bt_hci_cmd_le_set_data_length_t data_length;

        ulcn = strtoul(cmd + 19, NULL, 16);
        if (ulcn > 0x0F00) {
            BT_LOGE("APP", "update data length, handle should be 0x0000 ~ 0x0F00");
            return BT_STATUS_FAIL;
        }
        data_length.connection_handle = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 24, NULL, 16);
        if (ulcn > 0x00FB || ulcn < 0x001B) {
            BT_LOGE("APP", "update data length, tx_octets should be 0x001B ~ 0x00FB");
            return BT_STATUS_FAIL;
        }
        data_length.tx_octets = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 29, NULL, 16);
        if (ulcn > 0x0848 || ulcn < 0x0148) {
            BT_LOGE("APP", "update data length, tx_time should be 0x0148 ~ 0x0848");
            return BT_STATUS_FAIL;
        }
        data_length.tx_time = (uint16_t)ulcn;
        if (data_length.connection_handle > 0x0f00 ||
            (data_length.tx_octets < 0x001B || data_length.tx_octets > 0x00FB) ||
            (data_length.tx_time < 0x0148 || data_length.tx_time > 0x0848)) {
            BT_LOGW("APP", "Usage: update data length <handle in hex> <tx octets in hex> <tx time in hex>.");
            BT_LOGW("APP", "The range of connection handle is 0x0000-0x0EFF");
            BT_LOGW("APP", "The range of tx octets is 0x001B-0x00FB");
            BT_LOGW("APP", "The range of tx time is 0x0148-0x0848");
        } else {
            BT_LOGI("APP", "update data length handle(%04x) tx_octets(%04x) tx_time(%04x)",
                    data_length.connection_handle, data_length.tx_octets, data_length.tx_time);
            bt_gap_le_update_data_length(&data_length);
        }
    }

    /* Usage:   bond <handle in hex>
       Example: bond 0200 */
    else if (UT_APP_CMP("bond")) {
        const char *handle = cmd + strlen("bond ");

        ulcn = strtoul(handle, NULL, 16);
        if (ulcn > 0xFFFF) {
            BT_LOGE("APP", "bond error, handle should be 0x0000 ~ 0xFFFF");
            return BT_STATUS_FAIL;
        }
        bt_gap_le_bond(ulcn, &pairing_config_req);
    } else if (UT_APP_CMP("sm passkey")) {
        int iret = 0;
        iret = atoi(cmd + 11);
        if (iret < 0) {
            BT_LOGE("APP", "sm passkey error");
            return BT_STATUS_FAIL;
        }
        sm_passkey = (uint32_t)iret;
    } else if (UT_APP_CMP("sm numeric compare")) {
        nc_value_correct[0] = *((uint8_t *)(cmd + 19));
    } else if (UT_APP_CMP("remove bond")) {
        bt_addr_t addr = {
            .type = (uint8_t)strtoul(cmd + strlen("remove bond "), NULL, 10),
        };
        copy_str_to_addr(addr.addr, cmd + strlen("remove bond 0 "));
        cancel_bonded_info(&addr);
    }

    else if (UT_APP_CMP("list bond")) {
        BT_COLOR_SET(BT_COLOR_BLUE);
        dump_bonded_info_list();
        BT_COLOR_SET(BT_COLOR_WHITE);
    }

    else if (UT_APP_CMP("list connection")) {
        BT_COLOR_SET(BT_COLOR_BLUE);
        dump_connection_info_list();
        BT_COLOR_SET(BT_COLOR_WHITE);
    }

    else if (UT_APP_CMP("show status")) {
        BT_COLOR_SET(BT_COLOR_BLUE);
        BT_LOGD("APP", "Advertising:\t%s", bt_app_advertising ? "ON" : "OFF");
        BT_LOGD("APP", "Scanning:\t%s", bt_app_scanning ? "ON" : "OFF");
        BT_LOGD("APP", "Connecting:\t%s", bt_app_connecting ? "ON" : "OFF");
        BT_LOGD("APP", "MITM:\t\t%s", pairing_config_req.auth_req & BT_GAP_LE_SMP_AUTH_REQ_MITM ? "ON" : "OFF");
        BT_LOGD("APP", "Bonding:\t%s", pairing_config_req.auth_req & BT_GAP_LE_SMP_AUTH_REQ_BONDING ? "ON" : "OFF");
        BT_LOGD("APP", "LESC:\t\t%s", pairing_config_req.auth_req & BT_GAP_LE_SMP_AUTH_REQ_SECURE_CONNECTION ? "ON" : "OFF");
        BT_LOGD("APP", "OOB:\t\t%s", pairing_config_req.oob_data_flag ? "ON" : "OFF");
        switch (pairing_config_req.io_capability) {
            case BT_GAP_LE_SMP_DISPLAY_ONLY:
                BT_LOGD("APP", "IO Capability:\tBT_GAP_LE_SMP_DISPLAY_ONLY");
                break;
            case BT_GAP_LE_SMP_KEYBOARD_DISPLAY:
                BT_LOGD("APP", "IO Capability:\tBT_GAP_LE_SMP_KEYBOARD_DISPLAY");
                break;
            default:
                BT_LOGD("APP", "IO Capability:\t%d", pairing_config_req.io_capability);
        }
        BT_LOGD("APP", "Master LTK:\t%s",
                pairing_config_req.initiator_key_distribution & BT_GAP_LE_SMP_KEY_DISTRIBUTE_ENCKEY ? "ON" : "OFF");
        BT_LOGD("APP", "Master CSRK:\t%s",
                pairing_config_req.initiator_key_distribution & BT_GAP_LE_SMP_KEY_DISTRIBUTE_SIGN ? "ON" : "OFF");
        BT_LOGD("APP", "Master IRK:\t%s",
                pairing_config_req.initiator_key_distribution & BT_GAP_LE_SMP_KEY_DISTRIBUTE_IDKEY ? "ON" : "OFF");
        BT_LOGD("APP", "Slave LTK:\t%s",
                pairing_config_req.responder_key_distribution & BT_GAP_LE_SMP_KEY_DISTRIBUTE_ENCKEY ? "ON" : "OFF");
        BT_LOGD("APP", "Slave CSRK:\t%s",
                pairing_config_req.responder_key_distribution & BT_GAP_LE_SMP_KEY_DISTRIBUTE_SIGN ? "ON" : "OFF");
        BT_LOGD("APP", "Slave IRK:\t%s",
                pairing_config_req.responder_key_distribution & BT_GAP_LE_SMP_KEY_DISTRIBUTE_IDKEY ? "ON" : "OFF");
        dump_bonded_info_list();
        dump_connection_info_list();
        BT_COLOR_SET(BT_COLOR_WHITE);
    }

#ifdef __BT_HB_DUO__
    else if (UT_APP_CMP("bt debug cmd")) {
        uint32_t length = strlen("bt debug cmd ");
        uint32_t i = 0;
        int iret = 0;
        uint8_t value[4] = {*(cmd + length), *(cmd + length + 1), 0};
        while (value[0] != 0) {
            iret = sscanf((const char *)value, "%02hhx", cmd + (i >> 1));
            if (iret < 0)
                return BT_STATUS_FAIL;
            i += 2;
            value[0] = *(cmd + length + i);
            value[1] = *(cmd + length + i + 1);
        }
        bt_gap_debug_cmd_sending((uint8_t *)cmd);
    } else if (UT_APP_CMP("bt eir")) {
        /* bt eir [eir_data]
           e.g. bt eir 2b3bfe1234 */
        uint32_t length = strlen("bt eir ");
        uint32_t len = strlen(cmd + length);
        uint32_t i = 0;
        int iret = 0;
        uint8_t value[4] = {*(cmd + length), *(cmd + length + 1), 0};
        while (value[0] != 0) {
            iret = sscanf((const char *)value, "%02hhx", cmd + (i >> 1));
            if (iret < 0)
                return BT_STATUS_FAIL;
            i += 2;
            value[0] = *(cmd + length + i);
            value[1] = *(cmd + length + i + 1);
        }
        bt_gap_set_extended_inquiry_response((uint8_t *)cmd, len >> 1);
    } else if (UT_APP_CMP("bt write tx")) {
        /* bt write tx [tx_value]
           e.g. bt write tx -20*/
        uint8_t tx = (uint8_t)strtoul(cmd + strlen("bt write tx "), NULL, 16);
        bt_gap_write_inquiry_tx(tx);
    } else if (UT_APP_CMP("bt read tx")) {
        bt_gap_read_inquiry_response_tx();
    } else if (UT_APP_CMP("bt disconnect")) {
        /* bt disconnect [hci_handle]
           e.g. bt disconnect 0b */
#if 0
        bt_handle_t handle = (bt_handle_t)strtoul(cmd + strlen("bt disconnect "), NULL, 16);
        bt_gap_connection_handle_t conn_handle = (bt_gap_connection_handle_t)bt_gap_find_connection_by_handle(handle);
        BT_LOGI("APP", "connection_handle(0x%08x)", handle);
        bt_gap_disconnect(conn_handle);
#endif /* #if 0 */
    } else if (UT_APP_CMP("bt cancel name")) {
        /* bt bt cancel name [addr]
           e.g. bt cancel name 112233445566 */
        bt_bd_addr_t addr;
        copy_str_to_addr(&addr[0], cmd + strlen("bt cancel name "));
        bt_gap_cancel_name_request(&addr);
    } else if (UT_APP_CMP("bt remote name")) {
        /* bt bt remote name [addr]
           e.g. bt remote name 112233445566 */
        bt_bd_addr_t addr;
        copy_str_to_addr(&addr[0], cmd + strlen("bt remote name "));
        bt_gap_read_remote_name(&addr);
    } else if (UT_APP_CMP("bt connect")) {
#if 0
        /* bt connect [addr]
           e.g. bt connect 112233445566 */
        bt_bd_addr_t addr;
        copy_str_to_addr(&addr[0], cmd + strlen("bt connect "));
        bt_gap_connect(&addr);
#endif /* #if 0 */
    }
#if 0
    else if (UT_APP_CMP("bt set role")) {
        /* bt set role [hci_handle] [role]
           e.g. bt set role 000b 1*/
        bt_role_t role;
        bt_handle_t handle = (bt_handle_t)strtoul(cmd + strlen("bt set role "), NULL, 16);
        role = (bt_role_t)strtoul(cmd + strlen("bt set role 0000 "), NULL, 16);
        bt_gap_connection_handle_t conn_handle = (bt_gap_connection_handle_t)bt_gap_find_connection_by_handle(handle);
        bt_gap_set_role(conn_handle, role);
    } else if (UT_APP_CMP("bt get role")) {
        /* bt set role [hci_handle]
           e.g. bt get role 000b*/
        bt_handle_t handle = (bt_handle_t)strtoul(cmd + strlen("bt get role "), NULL, 16);
        bt_gap_connection_handle_t conn_handle = (bt_gap_connection_handle_t)bt_gap_find_connection_by_handle(handle);
        bt_gap_get_role(conn_handle);
    } else if (UT_APP_CMP("bt read rssi")) {
        /* bt read rssi [hci_handle]
           e.g. bt read rssi 000b*/
        bt_handle_t handle = (bt_handle_t)strtoul(cmd + strlen("bt read rssi "), NULL, 16);
        bt_gap_connection_handle_t conn_handle = (bt_gap_connection_handle_t)bt_gap_find_connection_by_handle(handle);
        bt_gap_read_rssi(conn_handle);
    } else if (UT_APP_CMP("bt sniff lock")) {
        /* bt sniff lock [hci_handle]
           e.g. bt sniff lock 000b*/
        bt_handle_t handle = (bt_handle_t)strtoul(cmd + strlen("bt sniff lock "), NULL, 16);
        bt_gap_connection_handle_t conn_handle = (bt_gap_connection_handle_t)bt_gap_find_connection_by_handle(handle);
        bt_gap_sniff_lock(conn_handle);
    } else if (UT_APP_CMP("bt sniff unlock")) {
        /* bt sniff unlock [hci_handle]
           e.g. bt sniff unlock 000b*/
        bt_handle_t handle = (bt_handle_t)strtoul(cmd + strlen("bt sniff unlock "), NULL, 16);
        bt_gap_connection_handle_t conn_handle = (bt_gap_connection_handle_t)bt_gap_find_connection_by_handle(handle);
        bt_gap_sniff_unlock(conn_handle);
    }
#endif /* #if 0 */
    else if (UT_APP_CMP("bt sco")) {
#if 0
        /* bt connect [addr]
           e.g. bt connect 112233445566 */
        bt_bd_addr_t addr;
        copy_str_to_addr(&addr[0], cmd + strlen("bt sco "));
        bt_gap_create_sco(&addr);
#endif /* #if 0 */
    } else if (UT_APP_CMP("bt inquiry")) {
        /* bt inquiry [during] [max_count]
           e.g. bt inquiry 10 10 */
        uint8_t during;
        uint8_t count;

        ulcn = strtoul(cmd + strlen("bt inquiry "), NULL, 16);
        if (ulcn > 0xFF) {
            BT_LOGE("APP", "bt inquiry, during should be 0x00 ~ 0xFF");
            return BT_STATUS_FAIL;
        }
        during = (uint8_t)ulcn;

        ulcn = strtoul(cmd + strlen("bt inquiry 00 "), NULL, 16);
        if (ulcn > 0xFF) {
            BT_LOGE("APP", "bt inquiry, count should be 0x00 ~ 0xFF");
            return BT_STATUS_FAIL;
        }
        count = (uint8_t)ulcn;
        bt_gap_inquiry(during, count);
    } else if (UT_APP_CMP("bt cancel inquiry")) {
        /* bt cancel inquiry */
        bt_gap_cancel_inquiry();
    } else if (UT_APP_CMP("bt scan mode")) {
        /* bt scan mode [mode]
           e.g. bt scan mode 3 */
        bt_gap_scan_mode_t mode = (bt_gap_scan_mode_t)strtoul(cmd + strlen("bt scan mode "), NULL, 16);
        bt_gap_set_scan_mode(mode);
    } else if (UT_APP_CMP("bt bond")) {
#if 0
        /* bt bond [hci_handle]
           e.g. bt bond 0b */
        bt_handle_t handle = (bt_handle_t)strtoul(cmd + strlen("bt bond "), NULL, 16);
        bt_gap_connection_handle_t conn_handle = (bt_gap_connection_handle_t)bt_gap_find_connection_by_handle(handle);
        bt_gap_auth_request(conn_handle, BT_GAP_SECURITY_LEVEL_3);
#endif /* #if 0 */
    } else if (UT_APP_CMP("bt passkey")) {
        /* bt passkey [value]
           e.g. bt passkey 123456 */
        uint32_t bt_passkey = (uint32_t)strtoul(cmd + strlen("bt passkey "), NULL, 10);
        bt_gap_reply_passkey_request(&bt_passkey);
    } else if (UT_APP_CMP("bt confirm")) {
        /* bt confirm [value]
           e.g. bt confirm 1 */
        bool accept = (bool)strtoul(cmd + strlen("bt confirm "), NULL, 10);
        bt_gap_reply_user_confirm_request(accept);
    }
#endif /* #ifdef __BT_HB_DUO__ */

#ifdef MTK_BLE_SMTCN_ENABLE
    else if (UT_APP_CMP("wifi smart")) {
#ifdef __MTK_BT_MESH_ENABLE__
        if (!bt_app_mesh_is_enabled()) {
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */
            BT_LOGI("APP", "[DTP]start adv\n");
            ble_smtcn_init();
            if (strlen(cmd) > strlen("wifi smart "))
                ble_smtcn_start_adv(cmd + strlen("wifi smart "), strlen(cmd) - strlen("wifi smart "));
            else
                ble_smtcn_start_adv(NULL, 0);
#ifdef __MTK_BT_MESH_ENABLE__
        } else {
            BT_LOGI("APP", "mesh isn't disabled!");
        }
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */
    }
#endif /* #ifdef MTK_BLE_SMTCN_ENABLE */
    else if (UT_APP_CMP("get local addr")) {
        BT_LOGI("APP", "local_public_addr [%02X:%02X:%02X:%02X:%02X:%02X]", local_public_addr[5],
                local_public_addr[4], local_public_addr[3], local_public_addr[2], local_public_addr[1], local_public_addr[0]);
    } else if (UT_APP_CMP("btdrv snoop")) {
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
    } else if (UT_APP_CMP("setting")) {
        if (UT_APP_CMP("setting on")) {
            BT_LOGI("APP", "setting on");
            if (bt_setting_onoff_set(1))
                bt_reboot_system();
        } else if (UT_APP_CMP("setting off")) {
            BT_LOGI("APP", "setting off");
            if (bt_setting_onoff_set(0))
                bt_reboot_system();
        } else if (UT_APP_CMP("setting dump")) {
            BT_LOGI("APP", "setting dump");
            bt_setting_dump();
        } else {
            BT_LOGI("APP", "cmd not found");
            return BT_STATUS_SUCCESS;
        }
#ifdef MTK_BLE_DM_SUPPORT
    } else if (UT_APP_CMP("rm le bonded info")) {
        bt_device_manager_le_clear_all_bonded_info();
#endif /* #ifdef MTK_BLE_DM_SUPPORT */
    } else {
        unsigned int i;
        for (i = 0; i < sizeof(bt_app_callback_table) / sizeof(struct bt_app_callback_table_t); i++) {
            if (UT_APP_CMP(bt_app_callback_table[i].name)) {
                return bt_app_callback_table[i].io_callback(input, output);
            }
        }
        BT_LOGE("APP", "%s: command not found", cmd);
    }

    return BT_STATUS_SUCCESS;
}

bt_gap_config_t bt_custom_config = {
    .inquiry_mode = 2,
    .io_capability = BT_GAP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
    .cod = 0x2C0404,
    .device_name = {"HB Duo dev AA-BB"},
};

const bt_gap_config_t *bt_gap_get_local_configuration(void)
{
    return (const bt_gap_config_t *)&bt_custom_config;
}

void bt_gap_set_local_configuration_name(char *name)
{
    if (strlen(name) <= strlen(bt_custom_config.device_name)) {
        strncpy(bt_custom_config.device_name, name, strlen(bt_custom_config.device_name));
    }
    BT_LOGI("APP", "Change BT Local Name = %s", bt_custom_config.device_name);
}

#ifndef MTK_BLE_DM_SUPPORT
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
#endif /* #ifndef MTK_BLE_DM_SUPPORT */

bool bt_ut_gap_le_is_connection_update_request_accepted(bt_handle_t handle, bt_gap_le_connection_update_param_t *parm)
{
    BT_LOGI("APP", "%s: always accept so far", __FUNCTION__);
    return true;
}

bt_gap_le_local_key_t *bt_ut_gap_le_get_local_key(void)
{
    return &local_key_req_default;
}

#ifdef BT_UT_GAP_LE_SCAN_LIST
static void bt_ut_gap_le_scan_list_destroy(void)
{
    bt_ut_gap_le_scan_list_element_t *p = NULL;

    if (!pg_bt_gap_le_scan_list)
        return;

    while (pg_bt_gap_le_scan_list->head) {
        p = pg_bt_gap_le_scan_list->head;
        pg_bt_gap_le_scan_list->head = p->next;
        if (pg_bt_gap_le_scan_list->head)
            pg_bt_gap_le_scan_list->head->prev = NULL;
        vPortFree(p);
        pg_bt_gap_le_scan_list->list_number --;
    }
}

static bt_ut_gap_le_scan_list_element_t *bt_ut_gap_le_scan_list_compare_addr(
    bt_gap_le_advertising_report_ind_t *data,
    bt_ut_gap_le_scan_list_t *list)
{
    bt_ut_gap_le_scan_list_element_t *p = list->head;
    uint32_t i;

    if (!data || !list)
        return NULL;

    /* Compare Address */
    for (i = 0; i < list->list_number && p; i++) {
        if (memcmp(&data->address, &p->addr, sizeof(bt_addr_t)) == 0)
            break;
        p = p->next;
    }

    return p;
}

static void bt_ut_gap_le_scan_list_move_to_head(
    bt_ut_gap_le_scan_list_element_t *node,
    bt_ut_gap_le_scan_list_t *list)
{
    if (!node || !list)
        return;

    if (node == list->head)
        return;

    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else if (node == list->tail)
        list->tail = node->prev;

    list->head->prev = node;
    node->next = list->head;
    node->prev = NULL;
    list->head = node;
}

static void bt_ut_gap_le_scan_list_add(void *data, bt_ut_gap_le_scan_list_t *list)
{
    bt_gap_le_advertising_report_ind_t *report = (bt_gap_le_advertising_report_ind_t *)data;
    bt_ut_gap_le_scan_list_element_t *p = NULL;
    uint8_t count = 0, ad_data_len, ad_data_type;

    /* Compare Address. Find out the corresponding node. */
    p = bt_ut_gap_le_scan_list_compare_addr(report, list);

    if (!p) {
        /* If node not exist and list_number less than list_size, create it! */
        if (list->list_number < BT_UT_GAP_LE_SCAN_LIST_SIZE) {
            p = (bt_ut_gap_le_scan_list_element_t *)pvPortMalloc(sizeof(bt_ut_gap_le_scan_list_element_t));
            if (!p)
                return;

            memset(p, 0, sizeof(bt_ut_gap_le_scan_list_element_t));

            if (!list->head) {
                list->head = p;
                list->tail = p;
            }
            list->list_number ++;

            /* If the list already full, replaced the tail with the current report.*/
        } else {
            p = list->tail;
            memset(p->name, 0, sizeof(BT_UT_GAP_LE_SCAN_LIST_MAX_NAME_LEN));
            p->name_exist = false;
            memset(&p->addr, 0, sizeof(bt_addr_t));
            p->event_type = 0;;
        }

        memcpy(&p->addr, &report->address, sizeof(bt_addr_t));
    }

    /* Swap current node to the head. */
    if (p != list->head)
        bt_ut_gap_le_scan_list_move_to_head(p, list);

    /* If event type and name already recorded, return! */
    if ((p->event_type & report->event_type) && p->name)
        return;

    /* Check advertising data have name or not */
    while (count < report->data_length) {
        if (count >= BT_HCI_LE_ADVERTISING_DATA_LENGTH_MAXIMUM - 2) {
            BT_LOGW("APP", "larger than report len (%u)", count);
            break;
        }

        ad_data_len = report->data[count];
        /* Error handling for data length over 30 bytes. */
        if (ad_data_len >= 0x1F) {
            BT_LOGI("APP", "AD Data Len Error");
            break;
        }

        count += 1;
        ad_data_type = report->data[count];
        count += 1;

        if (ad_data_type == BT_GAP_LE_AD_TYPE_NAME_COMPLETE) {
            p->name_exist = true;
            memcpy(p->name, report->data + count,
                   ad_data_len > BT_UT_GAP_LE_SCAN_LIST_MAX_NAME_LEN ? BT_UT_GAP_LE_SCAN_LIST_MAX_NAME_LEN : ad_data_len);
            break;
        }

        count += (ad_data_len - 1);
    }

    /* Record Event Type */
    p->event_type |= report->event_type;

    return;
}

static bool bt_ut_gap_le_scan_is_direct_print(void *data)
{
    if (!data || !pg_bt_gap_le_scan_list || !g_bt_gap_le_scan_list_mtx)
        return true;

    if (xSemaphoreTake(g_bt_gap_le_scan_list_mtx, 0) == pdTRUE) {
        bt_ut_gap_le_scan_list_add(data, pg_bt_gap_le_scan_list);
        xSemaphoreGive(g_bt_gap_le_scan_list_mtx);
    }

    return false;
}

void bt_ut_gap_le_scan_list_deinit(void)
{
    if (pg_bt_gap_le_scan_list) {
        bt_ut_gap_le_scan_list_destroy();
        vPortFree((void *)pg_bt_gap_le_scan_list);
        pg_bt_gap_le_scan_list = NULL;
    }

    if (g_bt_gap_le_scan_list_mtx) {
        vSemaphoreDelete(g_bt_gap_le_scan_list_mtx);
        g_bt_gap_le_scan_list_mtx = NULL;
    }
}

void bt_ut_gap_le_scan_list_init(void)
{
    if (!pg_bt_gap_le_scan_list) {
        pg_bt_gap_le_scan_list = (bt_ut_gap_le_scan_list_t *)pvPortMalloc(sizeof(bt_ut_gap_le_scan_list_t));
        if (pg_bt_gap_le_scan_list)
            memset(pg_bt_gap_le_scan_list, 0, sizeof(bt_ut_gap_le_scan_list_t));
        else {
            BT_LOGI("APP", "bt gap le scan list init FAIL!");
            return;
        }
    }

    if (!g_bt_gap_le_scan_list_mtx) {
        g_bt_gap_le_scan_list_mtx = xSemaphoreCreateMutex();
        if (!g_bt_gap_le_scan_list_mtx) {
            BT_LOGI("APP", "bt gap le scan list mtx create FAIL!");
            bt_ut_gap_le_scan_list_deinit();
        }
    }
}

void bt_ut_gap_le_scan_list_dump(void)
{
    uint8_t i = 0;
    bt_ut_gap_le_scan_list_element_t *p = NULL;

    if (!pg_bt_gap_le_scan_list || !g_bt_gap_le_scan_list_mtx)
        return;

    if (xSemaphoreTake(g_bt_gap_le_scan_list_mtx, portMAX_DELAY) == pdFALSE) {
        return;
    }

    p = pg_bt_gap_le_scan_list->head;
    while (p) {
        BT_LOGI("APP", "==============================");
        BT_LOGI("APP", "[%03d] Addr:%s", i, bt_debug_addr2str(&p->addr));
        if (p->name_exist == true)
            BT_LOGI("APP", "      Name:%s", p->name);

        i++;
        p = p->next;
        if (i % 4 == 0)
            vTaskDelay(5);
    }

    xSemaphoreGive(g_bt_gap_le_scan_list_mtx);
}
#endif /* #ifdef BT_UT_GAP_LE_SCAN_LIST */

bt_status_t bt_ut_app_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{
    if (msg != BT_MEMORY_FREE_GARBAGE_IND &&
        msg != BT_GAP_LE_ADVERTISING_REPORT_IND &&
        msg != BT_GAP_LE_SET_ADVERTISING_CNF &&
        msg != BT_GAP_LE_EXT_ADVERTISING_REPORT_IND &&
        msg != BT_GAP_LE_PERIODIC_ADVERTISING_REPORT_IND
#ifdef BT_LE_AUDIO_ENABLE
        && msg != BT_GAP_LE_BIGINFO_ADV_REPORT_IND
#endif /* #ifdef BT_LE_AUDIO_ENABLE */
       ) {
        BT_COLOR_SET(BT_COLOR_GREEN);
        BT_LOGI("APP", "CL:10%s: status(0x%04x), msg = 0x%x", __FUNCTION__, status, msg);
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
#if 0 // MT7658
            {
                uint8_t idx;
                for (idx = 0; idx < BT_LE_CONNECTION_MAX; idx++) {
                    app_bt_bonded_info_t *app_bonded_info = NULL;
                    app_bonded_info = find_bonded_info_by_index(idx);
                    if (app_bonded_info != NULL) {
                        bt_gap_le_bonding_info_t *bonded_info = &(app_bonded_info->info);
                        //update resolving list
                        if (BT_STATUS_SUCCESS != app_add_dev_2_resolving_list(bonded_info)) {
                            BT_LOGE("APP", "Add Device to Resolving List FAILED!!!");
                        }
                        //update white list(use identity address or address)
                        if (BT_STATUS_SUCCESS != app_add_dev_2_white_list(bonded_info, &(app_bonded_info->bt_addr))) {
                            BT_LOGE("APP", "Add Device to White List FAILED!!!");
                        }
                    }
                }
                /* set RPA timeout */
                bt_gap_le_set_resolvable_private_address_timeout(0x0384);
            }
            BT_COLOR_SET(BT_COLOR_WHITE);
            bt_app_advertising = false;
            bt_app_scanning = false;
            bt_app_connecting = false;
#endif /* #if 0 // MT7658 */
            break;
        /* GAP */
        case BT_POWER_OFF_CNF:
            bt_app_power_state_set(BT_APP_POWER_STATE_POWER_OFF);
#ifdef MTK_BT_SUPPORT_FW_ASSERT_RECOVERY
            if (bt_app_is_assert_recovery) {
                BT_LOGI("APP", "BT_COREDUMP_DONE! Now do power on...");
                bt_app_power_state_set(BT_APP_POWER_STATE_POWER_ON_START);
                bt_power_on((bt_bd_addr_ptr_t)&local_public_addr, NULL);
                bt_app_is_assert_recovery = false;
            }
#endif /* #ifdef MTK_BT_SUPPORT_FW_ASSERT_RECOVERY */
            break;
        case BT_GAP_LE_SET_RANDOM_ADDRESS_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_SET_RANDOM_ADDRESS_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
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
            BT_LOGI("APP", "BT_GAP_LE_SET_ADDRESS_RESOLUTION_ENABLE_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_SET_ADVERTISING_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
#ifdef __MTK_BT_MESH_ENABLE__
            if (status != BT_STATUS_SUCCESS)
                BT_LOGW("APP", "BT_GAP_LE_SET_ADVERTISING_CNF Failed");
            else if (
#if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE))
                (bt_get_init_mode() == BT_BOOT_INIT_MODE_GATTS) ||
#endif /* #if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE)) */
                (bt_get_init_mode() == BT_BOOT_INIT_MODE_MESH && !bt_debug_get_mesh_filter()))
                BT_LOGI("APP", "BT_GAP_LE_SET_ADVERTISING_CNF Success");
#else /* #ifdef __MTK_BT_MESH_ENABLE__ */
            BT_LOGI("APP", "BT_GAP_LE_SET_ADVERTISING_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_SET_SCAN_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_SET_SCAN_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_ADVERTISING_REPORT_IND:
#ifdef BT_UT_GAP_LE_SCAN_LIST
            if (!bt_ut_gap_le_scan_is_direct_print(buff))
                break;
#endif /* #ifdef BT_UT_GAP_LE_SCAN_LIST */
            BT_COLOR_SET(BT_COLOR_RED);
#ifndef __MTK_BT_MESH_ENABLE__
            BT_LOGI("APP", "BT_GAP_LE_ADVERTISING_REPORT_IND %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            print_advertising_report(buff);//too many logs when enable mesh
#else /* #ifndef __MTK_BT_MESH_ENABLE__ */
            if (bt_get_init_mode() != BT_BOOT_INIT_MODE_MESH)
                print_advertising_report_name_only(buff);
#endif /* #ifndef __MTK_BT_MESH_ENABLE__ */

            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_CONNECT_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_CONNECT_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            bt_app_connecting = status == BT_STATUS_SUCCESS;
            break;
        case BT_GAP_LE_CONNECT_IND: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GAP_LE_CONNECT_IND %s",
                        (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
                BT_COLOR_SET(BT_COLOR_BLUE);

                bt_gap_le_connection_ind_t *connection_ind = (bt_gap_le_connection_ind_t *)buff;
                BT_LOGI("APP", "connection handle=0x%04x", connection_ind->connection_handle);
                BT_LOGI("APP", "role=%s", (connection_ind->role == BT_ROLE_MASTER) ? "Master" : "Slave");
                BT_LOGI("APP", "peer address:%s", bt_debug_addr2str(&connection_ind->peer_addr));
#ifdef BLE_THROUGHPUT
                printf("connection handle=0x%04x\n", connection_ind->connection_handle);
                printf("peer address:%s\n", bt_debug_addr2str(&connection_ind->peer_addr));
#endif /* #ifdef BLE_THROUGHPUT */
                BT_COLOR_SET(BT_COLOR_WHITE);
                if (status == BT_STATUS_SUCCESS) {
                    //Titan:(TODO) If hit the max connection num or bonded num
                    //      , BT_CONNECTION_MAX, we should do disconnect directly.
                    add_connection_info(buff);
                    bt_handle_t handle = connection_ind->connection_handle;
                    disconnect_para.connection_handle = handle;
                    conn_update_para.connection_handle = handle;
                    read_rssi.handle = handle;
                    conn_interval = (connection_ind->conn_interval * 5) / 4;
                    BT_LOGI("APP", "ACL connected, so start bonding, handle = 0x%x", handle);
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
            BT_LOGI("APP", "BT_GAP_LE_CONNECT_CANCEL_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            bt_app_connecting =  false;
            break;
        case BT_GAP_LE_DISCONNECT_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_DISCONNECT_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_DISCONNECT_IND:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_DISCONNECT_IND %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            delete_connection_info(buff);
            break;
        case BT_GAP_LE_CONNECTION_UPDATE_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_CONNECTION_UPDATE_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_CONNECTION_UPDATE_IND:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_CONNECTION_UPDATE_IND %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_BONDING_REPLY_REQ_IND:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_BONDING_REPLY_REQ_IND %s",
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
                    printf("------------------------------>Passkey: %06u<---------------------------------\n", ind->passkey_display);
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
                    printf("------------------------------>Passkey: %06u<---------------------------------\n", ind->passkey_display);
                    sm_passkey = 0;
                    printf("\nConfirm numeric number:Y/N\n");
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
            BT_LOGI("APP", "BT_GAP_LE_BONDING_COMPLETE_IND %s",
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
#ifndef MTK_BLE_DM_SUPPORT
                //update resolving list
                if (BT_STATUS_SUCCESS == app_add_dev_2_resolving_list(&(bonded_info->info))) {
                    list_updating = list_updating | BT_APP_RESOLVING_LIST_UPDATING;
                    bt_gap_le_set_address_resolution_enable(true);
                }
#endif /* #ifndef MTK_BLE_DM_SUPPORT */
                //update white list(use identity address or address)
                if (BT_STATUS_SUCCESS == app_add_dev_2_white_list(&(bonded_info->info), &(con->peer_addr))) {
                    list_updating = list_updating | BT_APP_WHITE_LIST_UPDATING;
                }
            }
            break;
        case BT_GAP_LE_READ_RSSI_CNF: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GAP_LE_READ_RSSI_CNF %s",
                        (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
                BT_COLOR_SET(BT_COLOR_BLUE);

                const bt_hci_evt_cc_read_rssi_t *rssi = (bt_hci_evt_cc_read_rssi_t *)buff;
                BT_LOGI("APP", "connection handle=0x%04x", rssi->handle);
                if (rssi->rssi == 127) {
                    BT_LOGI("APP", "rssi cannot be read");
                } else {
                    if ((rssi->rssi >> 7) > 0) {
                        BT_LOGI("APP", "rssi=%ddBm", ((~rssi->rssi) & 0xFF) + 0x01);
                    } else {
                        BT_LOGI("APP", "rssi=%ddBm", rssi->rssi);
                    }
                }
                BT_COLOR_SET(BT_COLOR_WHITE);
                break;
            }
        case BT_GAP_LE_UPDATE_DATA_LENGTH_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_UPDATE_DATA_LENGTH_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_SET_TX_POWER_CNF:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_SET_TX_POWER_CNF %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;
        case BT_GAP_LE_READ_PHY_CNF:
            BT_LOGI("APP", "read phy info %s, ret = %d", status == BT_STATUS_SUCCESS ? "success" : "fail", status);
            if (status == BT_STATUS_SUCCESS) {
                bt_hci_evt_cc_le_read_phy_t *phy_info = NULL;
                phy_info = (bt_hci_evt_cc_le_read_phy_t *)buff;
                BT_LOGI("APP", "phy info: tx = %d, rx = %d", phy_info->tx, phy_info->rx);
            }
            break;
        case BT_GAP_LE_SET_PHY_CNF:
            BT_LOGI("APP", "set phy %s, ret = %d", status == BT_STATUS_SUCCESS ? "success" : "error", status);
            break;
        case BT_GAP_LE_PHY_UPDATE_IND:
            BT_LOGI("APP", "update phy info %s, ret = %d", status == BT_STATUS_SUCCESS ? "success" : "fail", status);
            if (status == BT_STATUS_SUCCESS) {
                bt_gap_le_phy_update_ind_t *phy_info = NULL;
                phy_info = (bt_gap_le_phy_update_ind_t *)buff;
                BT_LOGI("APP", "phy info: tx = %d, rx = %d", phy_info->tx, phy_info->rx);
            }
            break;
#if 0
        case  BT_GAP_LE_MULTI_ADVERTISING_STATE_CHANGE_IND: {
                BT_LOGI("APP", "BT_GAP_LE_MULTI_ADVERTISING_STATE_CHANGE_IND");
                bt_gap_le_multi_advertising_state_change_ind_t *state_change_t =
                    (bt_gap_le_multi_advertising_state_change_ind_t *)buff;
                BT_LOGI("APP", "instance:%d, reason:0x%02x, connection handle:0x%04x",
                        state_change_t->instance,
                        state_change_t->reason,
                        state_change_t->connection_handle);
                break;
            }
#endif /* #if 0 */
#ifdef BT_BQB
        case BT_GAP_LE_BQB_DISCONNECT_REQ_IND:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GAP_LE_BQB_DISCONNECT_REQ_IND %s",
                    (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
            BT_COLOR_SET(BT_COLOR_WHITE);
            return bt_gap_le_disconnect(&disconnect_para);
#endif /* #ifdef BT_BQB */
        case BT_GATTC_READ_USING_CHARC_UUID: {
                if (bt_app_wait_peer_central_address_resolution_rsp == true) {
                    bt_app_wait_peer_central_address_resolution_rsp = false;
                    BT_COLOR_SET(BT_COLOR_RED);
                    BT_LOGI("APP", "Read Peer Central Address Resolution characteristic");
                    BT_COLOR_SET(BT_COLOR_BLUE);
                    bt_gattc_read_by_type_rsp_t rsp = *((bt_gattc_read_by_type_rsp_t *)buff);

                    if (rsp.att_rsp == NULL) {
                        BT_LOGI("APP", "status = %d", status);
                        BT_COLOR_SET(BT_COLOR_WHITE);
                        break;
                    }

                    if (rsp.att_rsp->opcode == BT_ATT_OPCODE_READ_BY_TYPE_RESPONSE) {

                        if (status == BT_STATUS_SUCCESS && rsp.att_rsp == NULL) {
                            BT_LOGI("APP", "Read Peer Central Address Resolution characteristic FINISHED!!");
                            BT_COLOR_SET(BT_COLOR_WHITE);
                            break;
                        }

                        uint8_t *attribute_data_list = rsp.att_rsp->attribute_data_list;
                        uint8_t Peer_CAR_supporting = 0;

                        if (rsp.att_rsp->length - 2 == 1) {
                            Peer_CAR_supporting = *((uint8_t *)(attribute_data_list + 2));
                            BT_LOGI("APP", "Peer Central Address Resolution Supporting= %d", Peer_CAR_supporting);
                        }

                    } else if (rsp.att_rsp->opcode == 0x1) {
                        bt_gattc_error_rsp_t error_rsp = *((bt_gattc_error_rsp_t *)buff);
                        BT_LOGI("APP", "Can not find Peer Central Address Resolution");
                        BT_LOGI("APP", "Error_opcode=0x%02x, error_code=0x%02x", error_rsp.att_rsp->error_opcode, error_rsp.att_rsp->error_code);
                    } else {
                        BT_LOGI("APP", "Read Peer Central Address Resolution Error:Can not handle feedback");
                    }
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    return BT_STATUS_SUCCESS;
                }
                break;
            }
        case BT_GAP_IO_CAPABILITY_REQ_IND: {
                //bt_gap_connection_handle_t param;
                //param = (bt_gap_connection_handle_t)buff;
                BT_LOGI("APP", "BT_GAP_IO_CAPABILITY_REQ_IND %s", (status == BT_STATUS_SUCCESS) ? "Success" : "Failed");
                bt_gap_reply_io_capability_request(BT_GAP_OOB_DATA_PRESENTED_NONE, BT_GAP_SECURITY_AUTH_REQUEST_MITM_GENERAL_BONDING);
                return BT_STATUS_SUCCESS;
            }
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

    BT_LOGI("APP", "Peer ask to access attribute with authorization requirement.");
    BT_LOGI("APP", "connection[0x%04x] attribute handle[0x%04x] [%s]", req->connection_handle, req->attribute_handle,
            req->read_write == BT_GATTS_CALLBACK_READ ? "Read" : "Write");
    BT_LOGI("APP", "Security mode[0x%02x]", bonded_info->key_security_mode);
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

#if 0
bt_status_t bt_a2dp_get_init_params(bt_a2dp_init_params_t *params)
{
    if (params == NULL) {
        return BT_STATUS_FAIL;
    }

    params->codec_number = 2;
    params->codec_list = (bt_a2dp_codec_capability_t *)init_codec;
    BT_LOGI("APP", "codec, type:%d, 0x%08x", init_codec[0].type, *(uint32_t *)&init_codec[0].codec);
    BT_LOGI("APP", "codec, type:%d, 0x%08x", init_codec[1].type, *(uint32_t *)&init_codec[1].codec);

    BT_LOGI("APP", "A2DP init. OK.");

    return BT_STATUS_SUCCESS;
}
#endif /* #if 0 */
#ifdef MTK_BT_ACEMW_ENABLE

static void cb_gatts_register(BTStatus_t xStatus, uint8_t ucServerIf, BTUuid_t *pxAppUuid);
static void cb_gatts_unregister(BTStatus_t xStatus, uint8_t ucServerIf);
static void cb_gatts_conn(uint16_t usConnId, uint8_t ucServerIf, bool bConnected, BTBdaddr_t *pxBda);
static void cb_gatts_service_added(BTStatus_t xStatus,
                                   uint8_t ucServerIf, BTGattSrvcId_t *pxSrvcId, uint16_t usSrvcHandle);
static void cb_gatts_characteristic_added(BTStatus_t xStatus,
                                          uint8_t ucServerIf, BTUuid_t *pxUuid, uint16_t usSrvcHandle, uint16_t usCharHandle);
static void cb_gatts_descriptor_added(BTStatus_t xStatus,
                                      uint8_t ucServerIf, BTUuid_t *pxUuid, uint16_t usSrvcHandle, uint16_t usDescrHandle);
static void cb_gatts_request_read(uint16_t usConnId, uint32_t ulUlTransId,
                                  BTBdaddr_t *pxBda, uint16_t usAttrHandle, uint16_t usOffset);
static void cb_gatts_request_write(uint16_t usConnId, uint32_t ulUlTransId, BTBdaddr_t *pxBda,
                                   uint16_t usAttrHandle, uint16_t usOffset, size_t xLength, bool bNeedRsp, bool bIsPrep, uint8_t *pucValue);

BTGattServerCallbacks_t g_gatts_cb = {
    cb_gatts_register,
    cb_gatts_unregister,
    cb_gatts_conn,
    cb_gatts_service_added,
    NULL,
    cb_gatts_characteristic_added,
    NULL,
    cb_gatts_descriptor_added,
    NULL,
    NULL,
    NULL,
    cb_gatts_request_read,
    cb_gatts_request_write,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static uint8_t g_server_if = 0;
static uint16_t g_srvcHandle = 0;

void cb_gatts_register(BTStatus_t xStatus, uint8_t ucServerIf, BTUuid_t *pxAppUuid)
{
    BT_LOGI("App", "%s: ucServerIf = 0x%x", __FUNCTION__, ucServerIf);
    g_server_if = ucServerIf;
}

static void cb_gatts_unregister(BTStatus_t xStatus, uint8_t ucServerIf)
{
    BT_LOGI("App", "%s: ucServerIf = 0x%x", __FUNCTION__, ucServerIf);
}

static void cb_gatts_conn(uint16_t usConnId, uint8_t ucServerIf, bool bConnected, BTBdaddr_t *pxBda)
{
    const uint8_t *addr = (const uint8_t *)pxBda;
    BT_LOGI("App", "%s: conn_id = 0x%x, server_if = 0x%x, connected = 0x%x",
            __FUNCTION__, usConnId, ucServerIf, bConnected);
    if (addr) {
        BT_LOGI("App", "%s: addr = 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x",
                __FUNCTION__, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    }
}

static void cb_gatts_service_added(BTStatus_t xStatus,
                                   uint8_t ucServerIf, BTGattSrvcId_t *pxSrvcId, uint16_t usSrvcHandle)
{
    BT_LOGI("App", "%s: ucServerIf = 0x%x, usSrvcHandle = 0x%x", __FUNCTION__, ucServerIf, usSrvcHandle);
    g_srvcHandle = usSrvcHandle;
}

static void cb_gatts_characteristic_added(BTStatus_t xStatus,
                                          uint8_t ucServerIf, BTUuid_t *pxUuid, uint16_t usSrvcHandle, uint16_t usCharHandle)
{
    BT_LOGI("App", "%s: ucServerIf = 0x%x, usSrvcHandle = 0x%x, usCharHandle = 0x%x",
            __FUNCTION__, ucServerIf, usSrvcHandle, usCharHandle);
}

static void cb_gatts_descriptor_added(BTStatus_t xStatus,
                                      uint8_t ucServerIf, BTUuid_t *pxUuid, uint16_t usSrvcHandle, uint16_t usDescrHandle)
{
    BT_LOGI("App", "%s: ucServerIf = 0x%x, usSrvcHandle = 0x%x, usCharHandle = 0x%x",
            __FUNCTION__, ucServerIf, usSrvcHandle, usDescrHandle);
}

static void cb_gatts_request_read(uint16_t usConnId, uint32_t ulUlTransId,
                                  BTBdaddr_t *pxBda, uint16_t usAttrHandle, uint16_t usOffset)
{
    const uint8_t *addr = (const uint8_t *)pxBda;
    BT_LOGI("App", "%s: usConnId = 0x%x, ulUlTransId = 0x%x, peer addr = 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, attrHandle = 0x%x",
            __FUNCTION__, usConnId, ulUlTransId, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], usAttrHandle);
}

static void cb_gatts_request_write(uint16_t usConnId, uint32_t ulUlTransId, BTBdaddr_t *pxBda,
                                   uint16_t usAttrHandle, uint16_t usOffset, size_t xLength, bool bNeedRsp, bool bIsPrep, uint8_t *pucValue)
{
    const uint8_t *addr = (const uint8_t *)pxBda;
    BT_LOGI("App", "%s: usConnId = 0x%x, ulUlTransId = 0x%x, peer addr = 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, attrHandle = 0x%x",
            __FUNCTION__, usConnId, ulUlTransId, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], usAttrHandle);
}

bt_status_t acemw_gatts_cb(void *input, void *output)
{
    const char *cmd = input;
    const BTGattServerInterface_t *hal = BT_GetGattServer();

    if (UT_APP_CMP("gatts init")) {
        hal->pxGattServerInit(&g_gatts_cb);
    } else if (UT_APP_CMP("gatts register")) {
        BTUuid_t uuid;
        memset(&uuid, 0, sizeof(uuid));
        uuid.ucType = eBTuuidType16;
        uuid.uu.uu16 = 0x2222;
        hal->pxRegisterServer(&uuid);
    } else if (UT_APP_CMP("ble gatts unregister")) {
        hal->pxUnregisterServer(g_server_if);
    } else if (UT_APP_CMP("gatts add service")) {
        BTUuid_t uuid;
        memset(&uuid, 0, sizeof(uuid));
        uuid.ucType = eBTuuidType16;
        uuid.uu.uu16 = 0x1800; // GAP
        BTGattSrvcId_t srvc_id = {{uuid, 0x30}, eBTServiceTypePrimary};
        hal->pxAddService(g_server_if, &srvc_id, 0x12);
    } else if (UT_APP_CMP("gatts add characteristic")) {
        BTUuid_t uuid;
        memset(&uuid, 0, sizeof(uuid));
        uuid.ucType = eBTuuidType16;
        uuid.uu.uu16 = 0x2a00; // Device name
        uint8_t prop = BT_GATT_CHARC_PROP_READ | BT_GATT_CHARC_PROP_NOTIFY | BT_GATT_CHARC_PROP_INDICATE;
        hal->pxAddCharacteristic(g_server_if, g_srvcHandle, &uuid, prop, BT_GATTS_REC_PERM_READABLE);
    } else if (UT_APP_CMP("gatts add descriptor")) {
        BTUuid_t uuid;
        memset(&uuid, 0, sizeof(uuid));
        uuid.ucType = eBTuuidType16;
        uuid.uu.uu16 = 0x2901; // user description
        hal->pxAddDescriptor(g_server_if, g_srvcHandle, &uuid, BT_GATTS_REC_PERM_READABLE);
    } else if (UT_APP_CMP("gatts send response")) {
        uint8_t name[6] = {"MTK_HB"};
        static BTGattResponse_t gatts_rsp;
        gatts_rsp.usHandle = 0x200;
        gatts_rsp.xAttrValue.usHandle = 0x8888;
        gatts_rsp.xAttrValue.pucValue = name;
        gatts_rsp.xAttrValue.usOffset = 0;
        gatts_rsp.xAttrValue.xLen = 6;
        gatts_rsp.xAttrValue.xRspErrorStatus = eBTStatusSuccess;
        hal->pxSendResponse(0x200, 99, eBTStatusSuccess, &gatts_rsp);
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_app_acemw_callback(void *input, void *output)
{
    const char *cmd = input;
    const BTBleAdapter_t *hal = BT_GetBLEAdapter();

    if (strncmp("gatts", cmd, 5) == 0) {
        acemw_gatts_cb(input, output);
        return BT_STATUS_SUCCESS;
    }

    BT_LOGI("APP", "CL:bt_cli_acemw %s", cmd);
    if (UT_APP_CMP("ble scan on")) {
        hal->pxScan(1);
    } else if (UT_APP_CMP("ble scan off")) {
        hal->pxScan(0);
    } else if (UT_APP_CMP("ble adv on")) {
        hal->pxStartAdv(0);
    } else if (UT_APP_CMP("ble adv off")) {
        hal->pxStopAdv(0);
    } else if (UT_APP_CMP("ble adv data")) {
        BTUuid_t uuid;
        uuid.ucType = eBTuuidType16;
        uuid.uu.uu16 = 0xaabb;
        BTGattAdvertismentParams_t parm = {0, 1, 1, 0, 0, 0x0800, 0x0800, 7, 0xf1, 0, 0, 0, 0};
        hal->pxSetAdvData(0, &parm, 3, "abc", 0, NULL, &uuid, 1);
    } else if (UT_APP_CMP("ble adv rawdata")) {
        uint8_t raw[31] = {
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
            0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
            0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40
        };
        hal->pxSetAdvRawData(0, raw, sizeof(raw));
    } else if (UT_APP_CMP("ble multiadv on")) {
        BTGattAdvertismentParams_t parm = {0, 1, 1, 0, 0, 0x0800, 0x0800, 7 /*ucChannelMap*/, 5 /*ucTxPower*/, 0, 0, 0, BTAddrTypePublic};
        hal->pxMultiAdvEnable(1, &parm); // 1: instance number
    } else if (UT_APP_CMP("ble multiadv update")) {
        BTGattAdvertismentParams_t parm = {0, 1, 1, 0, 0, 0x0800, 0x0800, 7, 5 /*ucTxPower*/, 0, 0, 0, 0};
        hal->pxMultiAdvUpdate(1, &parm); // 1: instance number
    } else if (UT_APP_CMP("ble multiadv data")) {
        hal->pxMultiAdvSetInstData(1, 0, 1, 1, 0x1234, 3, "MTK", 0, NULL, NULL, 0);
    } else if (UT_APP_CMP("ble multiadv off")) {
        hal->pxMultiAdvDisable(1); // 1: instance number
    } else if (UT_APP_CMP("ble connect")) {
        //const BTBdaddr_t addr = {{0x71, 0x70, 0xF9, 0xE1, 0xE0, 0xD8}};
        //const BTBdaddr_t addr = {{0x4A, 0xF1, 0x0C, 0x55, 0xC2, 0x88}};
        const BTBdaddr_t addr = {{0x9C, 0x0E, 0x00, 0x83, 0xFC, 0x0C}}; // Airoha BLE RC
        hal->pxConnect(0, &addr, 1, 0); // 4th parm 0: don't care
    }
    return BT_STATUS_SUCCESS;
}
#endif /* #ifdef MTK_BT_ACEMW_ENABLE */
