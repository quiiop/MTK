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

//#define __BT_MULTI_ADV__ // marcus temp mark
#include "ut_app.h"
#include <string.h>

#ifdef MTK_BLE_DM_SUPPORT
#include "bt_device_manager_common.h"
#include "bt_device_manager_le.h"
#endif /* #ifdef MTK_BLE_DM_SUPPORT */

bt_status_t bqb_gap_io_callback(void *input, void *output);

// Weak symbol declaration
#if _MSC_VER >= 1500
#pragma comment(linker, "/alternatename:_bqb_gap_io_callback=_default_bqb_gap_io_callback")
#elif defined(__GNUC__) || defined(__ICCARM__) || defined(__CC_ARM)
#pragma weak bqb_gap_io_callback = default_bqb_gap_io_callback
#else /* #if _MSC_VER >= 1500 */
#error "Unsupported Platform"
#endif /* #if _MSC_VER >= 1500 */

#define HCI_CMDCODE_SET_TX_POWER_DBM    0xFC23

extern bt_hci_cmd_read_rssi_t read_rssi;
#ifndef MTK_BLE_DM_SUPPORT
extern bt_bd_addr_t g_local_public_addr;
extern bt_bd_addr_t g_local_random_addr;
#endif /* #ifndef MTK_BLE_DM_SUPPORT */
extern bool bt_app_advertising;
extern bool bt_app_scanning;
extern bool bt_app_scanning;

bt_status_t default_bqb_gap_io_callback(void *input, void *output)
{
    return BT_STATUS_SUCCESS;
}

static void ut_app_gap_convert_hex_str(const char *str, uint8_t *output, uint8_t len)
{
    uint8_t i = 0;
    char tempbuf[2];

    while (len) {
        memcpy(tempbuf, (str + (i * 2)), 2);
        output[i] = (uint8_t)strtoul(tempbuf, NULL, 16);
        len = len - 2;
        i++;
    }
}

bt_status_t bt_app_gap_io_callback(void *input, void *output)
{
    const char *cmd = (const char *)input;
    unsigned long ulcn = 0;

    if (UT_APP_CMP("gap power_on")) {
#ifndef MTK_BLE_DM_SUPPORT
        bt_power_on((bt_bd_addr_ptr_t)&g_local_public_addr, (bt_bd_addr_ptr_t)&g_local_random_addr);
#else /* #ifndef MTK_BLE_DM_SUPPORT */
        bt_power_on((bt_bd_addr_ptr_t)bt_device_manager_get_local_address(),
                    (bt_bd_addr_ptr_t)bt_device_manager_get_local_random_address());
#endif /* #ifndef MTK_BLE_DM_SUPPORT */
        //bt_gatts_set_max_mtu(128); /* This value should consider with MM Tx/Rx buffer size. */
    }

    else if (UT_APP_CMP("gap power_off")) {
        bt_power_off();
    }

    /* Usage: advanced power_on [public address] [random address].
       Note:  Set N if you doesn't need it. */
    else if (UT_APP_CMP("advanced power_on")) {
        if (strlen(cmd) >= 18) {
            uint8_t public_addr[6] = {0};
            uint8_t random_addr[6] = {0};
            const char *addr_str = cmd + 18;

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
    }

    /* Usage: random_addr [random address].
       Note:  [random address] should be existed. */
    else if (UT_APP_CMP("gap random_addr")) {
        if (strlen(cmd) >= 16) {
            const char *addr_str = cmd + 16;
            uint8_t addr[6];
            copy_str_to_addr(addr, addr_str);

            bt_gap_le_set_random_address(addr);
        } else {
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGW("APP", "please input the specific random address");
            BT_LOGW("APP", "gap random_addr [random address]");
            BT_COLOR_SET(BT_COLOR_WHITE);
        }
    }

    else if (UT_APP_CMP("gap get_random_addr")) {
        const uint8_t *addr = bt_gap_le_get_random_address();
        if (addr != NULL)
            BT_LOGE("APP", "gap rand_addr: %02x-%02x-%02x-%02x-%02x-%02x",
                    addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    }

    /* Usage: gap wl add [0:public / 1:random] [bt address].
       Note:  default use #lt_addr_type and #lt_addr */
    else if (UT_APP_CMP("gap wl add")) {
        bt_addr_t device;
        if (strlen(cmd) >= 11) {

            uint8_t addr_type = (uint8_t)strtoul(cmd + 11, NULL, 10);

            if (addr_type != 0 && addr_type != 1) {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGW("APP", "please input the correct address type");
                BT_LOGW("APP", "gap wl add [0:public / 1:random] [bt address]");
                BT_COLOR_SET(BT_COLOR_WHITE);
            } else {
                const char *addr_str = cmd + 13;
                uint8_t addr[6];
                copy_str_to_addr(addr, addr_str);

                device.type = addr_type;
                memcpy(device.addr, addr, sizeof(addr));
                bt_gap_le_set_white_list(BT_GAP_LE_ADD_TO_WHITE_LIST, &device);
            }
        } else {
            //device.address_type = lt_addr_type;
            //memcpy(device, lt_addr, sizeof(lt_addr));
            //bt_gap_le_set_white_list(BT_ADD_TO_WHITE_LIST, &device);
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGW("APP", "please input the correct cmd");
            BT_LOGW("APP", "gap wl add [0:public / 1:random] [bt address]");
            BT_COLOR_SET(BT_COLOR_WHITE);
        }
    }

    /* Usage: gap wl remove [0:public / 1:random] [bt address].
       Note:  default use #lt_addr_type and #lt_addr */
    else if (UT_APP_CMP("gap wl remove")) {
        bt_addr_t device;
        if (strlen(cmd) >= 14) {

            uint8_t addr_type = (uint8_t)strtoul(cmd + 14, NULL, 10);
            if (addr_type != 0 && addr_type != 1) {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGW("APP", "please input the correct address type");
                BT_LOGW("APP", "gap wl remove [0:public / 1:random] [bt address]");
                BT_COLOR_SET(BT_COLOR_WHITE);
            } else {
                const char *addr_str = cmd + 16;
                uint8_t addr[6];
                copy_str_to_addr(addr, addr_str);

                device.type = addr_type;
                memcpy(device.addr, addr, sizeof(addr));
                bt_gap_le_set_white_list(BT_GAP_LE_REMOVE_FROM_WHITE_LIST, &device);
            }
        } else {
            //device.address_type = lt_addr_type;
            //memcpy(device.address, lt_addr, sizeof(lt_addr));
            //bt_gap_le_set_white_list(BT_REMOVE_FROM_WHITE_LIST, &device);
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGW("APP", "please input the correct cmd");
            BT_LOGW("APP", "gap wl remove [0:public / 1:random] [bt address]");
            BT_COLOR_SET(BT_COLOR_WHITE);
        }
    }

    else if (UT_APP_CMP("gap wl clear")) {
        bt_gap_le_set_white_list(BT_GAP_LE_CLEAR_WHITE_LIST, NULL);
    }

    else if (UT_APP_CMP("gap set_adv_data")) {
        if (strlen(cmd) >= 17) {
            const char *adv_data_str = cmd + 17;
            uint8_t len = strlen(adv_data_str);

            adv_data.advertising_data_length = 31;
            ut_app_gap_convert_hex_str(adv_data_str, adv_data.advertising_data, len);
            BT_LOGW("APP", "[GAP] adv data:%x, %x\n", adv_data.advertising_data[0], adv_data.advertising_data[1]);
        }
    }

    else if (UT_APP_CMP("gap set_scan_rsp_data")) {
        if (strlen(cmd) >= 22) {
            const char *scan_data_str = cmd + 22;
            uint8_t len = strlen(scan_data_str);

            scan_data.scan_response_data_length = 31;
            ut_app_gap_convert_hex_str(scan_data_str, scan_data.scan_response_data, len);
        }
    }

    /*gap set_adv_params [min_interval] [max_interval] [adv type] [own addr type] [peer addr type] [peer BT addr] [channel map] [advertising_filter_policy]
      [adv type] : Chck src/hbif/bt_gap_le_spec.h BT_GAP_LE_AD_xxxx 0~4
      [own addr type] :0:public / 1:random/ 2: Gen RPA from resolving list or public address host provide/ 3: Gen RPA from resolving list or static random address host provide
      [peer addr type]:0:public / 1:random
      [advertising_filter_policy]: define in spec, 0~4
      [peer BT Addr] : peer BT address for BT_GAP_LE_AD_CONNECTABLE_DIRECTED_HIGH or BT_GAP_LE_AD_CONNECTABLE_DIRECTED_LOW
     */
    else if (UT_APP_CMP("gap set_adv_params")) {
        uint16_t min_interval;
        uint16_t max_interval;
        uint8_t adv_type;
        uint8_t own_addr_type;
        uint8_t peer_addr_type;
        uint8_t map;
        uint8_t policy;
        const char *addr_str = cmd + 35;
        uint8_t addr[6];

        ulcn = strtoul(cmd + 19, NULL, 16);
        if (ulcn > 0xFFFF) {
            BT_LOGE("APP", "gap set_adv_params, min_interval > 0xFFFF");
            return BT_STATUS_FAIL;
        }
        min_interval = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 24, NULL, 16);
        if (ulcn > 0xFFFF) {
            BT_LOGE("APP", "gap set_adv_params, max_interval > 0xFFFF");
            return BT_STATUS_FAIL;
        }
        max_interval = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 29, NULL, 10);
        if (ulcn > 4) {
            BT_LOGE("APP", "gap set_adv_params, adv_type should be 0 ~ 4");
            return BT_STATUS_FAIL;
        }
        adv_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 31, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "gap set_adv_params, own_addr_type should be 0 ~ 3");
            return BT_STATUS_FAIL;
        }
        own_addr_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 33, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "gap set_adv_params, peer_addr_type should be 0 ~ 3");
            return BT_STATUS_FAIL;
        }
        peer_addr_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 53, NULL, 10);
        if (ulcn > 0xFF) {
            BT_LOGE("APP", "gap set_adv_params, map > 0xFF");
            return BT_STATUS_FAIL;
        }
        map = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 55, NULL, 10);
        if (ulcn > 4) {
            BT_LOGE("APP", "gap set_adv_params, policy should be 0 ~ 4");
            return BT_STATUS_FAIL;
        }
        policy = (uint8_t)ulcn;

        copy_str_to_addr(addr, addr_str);
        BT_COLOR_SET(BT_COLOR_BLUE);
        BT_LOGI("APP", "set advertising params");
        BT_LOGI("APP", "own_addr_type[%d] adv_type[%d] adv_policy[%d] peer_addr_type[%d]",
                own_addr_type, adv_type, policy, peer_addr_type);
        BT_LOGI("APP", "peer_addr(%02x:%02x:%02x:%02x:%02x:%02x)",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        BT_COLOR_SET(BT_COLOR_WHITE);

        adv_para.advertising_interval_min = min_interval;
        adv_para.advertising_interval_max = max_interval;
        adv_para.advertising_type = adv_type;
        adv_para.own_address_type = own_addr_type;
        adv_para.peer_address.type = peer_addr_type;
        memcpy(adv_para.peer_address.addr, addr, 6);
        adv_para.advertising_channel_map = map;
        adv_para.advertising_filter_policy = policy;
    }
    /*
     * Usage:   gap set_ext_scan [enable]
     * Example: gap set_ext_scan 1
     */
    else if (UT_APP_CMP("gap set_ext_scan")) {
        bt_hci_le_set_ext_scan_parameters_t params = { 0 };
        bt_hci_cmd_le_set_extended_scan_enable_t enable = { 0 };
        le_ext_scan_item_t phy_1m = { 0 };

        params.scanning_filter_policy = BT_HCI_ADV_FILTER_ACCEPT_SCAN_CONNECT_FROM_ALL;
        params.scanning_phys_mask = BT_HCI_LE_PHY_MASK_1M;
        phy_1m.scan_type = BT_HCI_SCAN_TYPE_ACTIVE;
        phy_1m.scan_interval = 0xa0;
        phy_1m.scan_window = 0xa0;
        params.params_phy_1M = &phy_1m;
        params.params_phy_coded = NULL;

        enable.enable = strtoul(cmd + 17, NULL, 10);
        if (enable.enable > 1) {
            BT_LOGE("APP", "enable should be 0 ~ 1");
            return BT_STATUS_FAIL;
        }
        enable.filter_duplicates = 1;
        bt_gap_le_set_extended_scan(&params, &enable);
    }

#ifdef __BT_MULTI_ADV__
    /*gap start_multi_adv [instance] [tx_power] [address]
     [instance] : 01 ~ (max_adv - 1).
     [tx_power] : -70 ~ 020, default 005.
     [address]: ex. AA11223344CC
     [advertising_filter_policy]: define in spec, 0~4
     [peer BT Addr] : peer BT address for BT_GAP_LE_AD_CONNECTABLE_DIRECTED_HIGH or BT_GAP_LE_AD_CONNECTABLE_DIRECTED_LOW
    */
    else if (UT_APP_CMP("gap start_multi_adv")) {
        bt_status_t ret;
        BT_LOGI("APP", "start multi adv %d", __LINE__);
        if (strlen(cmd) >= sizeof("gap start_multi_adv xx xx")) {
            uint8_t instance;
            int8_t tx_power;
            uint8_t addr[6];

            ulcn = strtoul(cmd + sizeof("gap start_multi_adv "), NULL, 10);
            if (ulcn > 0xFF) {
                BT_LOGE("APP", "gap start_multi_adv, instance should be 0x00 ~ 0xFF");
                return BT_STATUS_FAIL;
            }
            instance = (uint8_t)ulcn;

            ulcn = strtoul(cmd + sizeof("gap start_multi_adv xx "), NULL, 10);
            if (ulcn > 0x7F) {
                BT_LOGE("APP", "gap start_multi_adv, tx_power should be 0x00 ~ 0x7F");
                return BT_STATUS_FAIL;
            }
            tx_power = (int8_t)ulcn;

            copy_str_to_addr(addr, cmd + sizeof("gap start_multi_adv xx xxx"));
            BT_LOGI("APP", "MADV(%d) min: %x, max: %x, adv_type %d, own_type %d, map %x, policy %d", instance,
                    adv_para.advertising_interval_min,
                    adv_para.advertising_interval_max,
                    adv_para.advertising_type,
                    adv_para.own_address_type,
                    adv_para.advertising_channel_map,
                    adv_para.advertising_filter_policy);
            ret = bt_gap_le_start_multiple_advertising(instance, tx_power, addr, &adv_para, &adv_data, &scan_data);
            BT_LOGI("APP", "start multi adv return %x", ret);
        }
    } else if (UT_APP_CMP("gap stop_multi_adv")) {
        if (strlen(cmd) >= sizeof("gap stop_multi_adv")) {
            uint8_t instance = (uint8_t)strtoul(cmd + sizeof("gap stop_multi_adv "), NULL, 10);
            bt_gap_le_stop_multiple_advertising(instance);
        }
    } else if (UT_APP_CMP("gap get_adv_instance")) {
        BT_LOGI("APP", "Max adv instance %d", bt_gap_le_get_max_multiple_advertising_instances());
    }
#endif /* #ifdef __BT_MULTI_ADV__ */
    else if (UT_APP_CMP("gap start_adv")) {
        bt_app_advertising = true;
        memset(gatts_device_name, 0x00, sizeof(gatts_device_name));
        memcpy(gatts_device_name, &adv_data.advertising_data[5], 3);

        BT_COLOR_SET(BT_COLOR_BLUE);
        BT_LOGI("APP", "start advertising");
        BT_COLOR_SET(BT_COLOR_WHITE);

        adv_enable.advertising_enable = BT_HCI_ENABLE;
        bt_gap_le_set_advertising(&adv_enable, &adv_para, &adv_data, &scan_data);
    }
    /*
     * Usage:   gap set_ext_adv [adv_hdl][properties][min_interval][max_interval][adv_sid]
     * Example: gap set_ext_adv 06 0040 000100 000100 01
     */
    else if (UT_APP_CMP("gap set_ext_adv")) {
        bt_app_advertising = true;
        bt_hci_le_set_ext_advertising_parameters_t ext_adv_para = {
            // Example for AE properties:
            //.advertising_event_properties = BT_HCI_ADV_EVT_PROPERTIES_MASK_SCANNABLE | BT_HCI_ADV_EVT_PROPERTIES_MASK_DIRECTED | BT_HCI_ADV_EVT_PROPERTIES_MASK_INCLUDE_TXPOWER,
            // Example for Legacy ADV properties
            //.advertising_event_properties = BT_HCI_ADV_EVT_PROPERTIES_MASK_CONNECTABLE | BT_HCI_ADV_EVT_PROPERTIES_MASK_SCANNABLE | BT_HCI_ADV_EVT_PROPERTIES_MASK_LEGACY_PDU,
            .advertising_event_properties = BT_HCI_ADV_EVT_PROPERTIES_MASK_INCLUDE_TXPOWER,
            .primary_advertising_interval_min = 0x100,   // min * 0.625ms
            .primary_advertising_interval_max = 0x100,   // max * 0.625ms
            .primary_advertising_channel_map = 7, //bit 0 - 37, bit 1 - 38, bit 2 -39
            .own_address_type = 0x00,  //0x02,
            .peer_address = { 0 },  //{0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}},
            .advertising_filter_policy = 0,
            .primary_advertising_phy = 0x01,   // 1: 1M PHY, 3: coded PHY
            .secondary_advertising_max_skip = 0,
            .secondary_advertising_phy = 0x01, // 1: 1M PHY, 3: coded PHY
            .advertisng_SID = 0x0,
            .scan_request_notify_enable = 0x1,
        };
        uint8_t adv_hdl = 0;
        uint16_t evt_prop = 0;
        uint32_t min_interval = 0;
        uint32_t max_interval = 0;
        uint8_t adv_sid = 0;

        adv_hdl = strtoul(cmd + 16, NULL, 16);
        if (adv_hdl > 0xEF) {
            BT_LOGE("APP", "adv hdl in 0~0xEF");
            return BT_STATUS_FAIL;
        }

        evt_prop = strtoul(cmd + 19, NULL, 16);
        if (evt_prop > 0x007F) {
            BT_LOGE("APP", "event_prop should be <= 0x7F");
            return BT_STATUS_FAIL;
        }
        ext_adv_para.advertising_event_properties = evt_prop;
        if (evt_prop & BT_HCI_ADV_EVT_PROPERTIES_MASK_INCLUDE_TXPOWER)
            ext_adv_para.advertising_tx_power = 0x03;

        min_interval = strtoul(cmd + 24, NULL, 16);
        if (min_interval > 0xFFFFFF) {
            BT_LOGE("APP", "min interval <= 0xFFFFFF");
            return BT_STATUS_FAIL;
        }
        ext_adv_para.primary_advertising_interval_min = min_interval;

        max_interval = strtoul(cmd + 31, NULL, 16);
        if (max_interval > 0xFFFFFF) {
            BT_LOGE("APP", "max interval <= 0xFFFFFF");
            return BT_STATUS_FAIL;
        }
        ext_adv_para.primary_advertising_interval_max = max_interval;

        adv_sid = strtoul(cmd + 38, NULL, 16);
        if (adv_sid > 0x0F) {
            BT_LOGE("APP", "adv hdl in 0~0xF");
            return BT_STATUS_FAIL;
        }
        ext_adv_para.advertisng_SID = adv_sid;

        uint8_t adv_raw[31] = "DDDDDHUMMINGBIRD_ADV_DATA";
        uint8_t scan_raw[1] = {0};

        adv_raw[0] = 2; //adv_length
        adv_raw[1] = BT_GAP_LE_AD_TYPE_FLAG;
        adv_raw[2] = BT_GAP_LE_AD_FLAG_BR_EDR_NOT_SUPPORTED | BT_GAP_LE_AD_FLAG_GENERAL_DISCOVERABLE;
        adv_raw[3] = 21; //adv_length
        adv_raw[4] = 0x09;

        bt_gap_le_set_ext_advertising_data_t ext_data = {0};
        ext_data.data_length = 31;
        ext_data.data = adv_raw;

        bt_gap_le_set_ext_scan_response_data_t ext_scan = {0};
        ext_scan.data_length = 1;
        scan_raw[0] = 0xFF;
        ext_scan.data = scan_raw;
        ext_scan.fragment_preference = 0;

        bt_gap_le_config_extended_advertising(adv_hdl, NULL, &ext_adv_para, &ext_data, &ext_scan);
    }
    /*
     * Usage:   gap enable_ext_adv [adv_hdl][enable][duration]
     * Example: gap enable_ext_adv 06 1 2000 (for enable)
     *          gap enable_ext_adv 06 0 (for disable)
     */
    else if (UT_APP_CMP("gap enable_ext_adv")) {
        bt_hci_le_set_ext_advertising_enable_t ext_adv_enable = { 0 };
        uint8_t adv_hdl = 0;
        uint8_t enable = BT_HCI_DISABLE;
        uint32_t dur = 0;

        adv_hdl = strtoul(cmd + 19, NULL, 16);
        if (adv_hdl > 0xEF) {
            BT_LOGE("APP", "adv hdl in 0~0xEF");
            return BT_STATUS_FAIL;
        }

        enable = (uint8_t)strtoul(cmd + 22, NULL, 10);
        if (enable > 1) {
            BT_LOGE("APP", "enable is 0~1");
            return BT_STATUS_FAIL;
        }

        dur = strtoul(cmd + 24, NULL, 16);
        if (dur > 0xFFFF) {
            BT_LOGE("APP", "duration < 0xFFFF");
            return BT_STATUS_FAIL;
        }

        ext_adv_enable.enable = enable;
        ext_adv_enable.duration = (uint16_t)dur;  // 0: always, time: dur * 10ms

        bt_gap_le_enable_extended_advertising(adv_hdl, &ext_adv_enable);
    }
    /*
     * Usage: ble gap rm_ext_adv <adv hdl> <clear or not>
     * Example: ble gap rm_ext_adv 0008 1
     */
    else if (UT_APP_CMP("gap rm_ext_adv")) {
        uint8_t adv_hdl = 0;
        uint8_t clear_all = 0;

        ulcn = strtoul(cmd + 15, NULL, 16);
        if (ulcn > 0xEF) {
            BT_LOGE("APP", "hdl should be 0~0xEF");
            return BT_STATUS_FAIL;
        }
        adv_hdl = ulcn;

        ulcn = strtoul(cmd + 20, NULL, 16);
        if (ulcn > 1) {
            BT_LOGE("APP", "1:clear all, 0:no clear");
            return BT_STATUS_FAIL;
        }
        clear_all = ulcn;

        bt_gap_le_remove_extended_advertising(adv_hdl, clear_all);
    }
#ifdef BT_LE_AUDIO_ENABLE
    /*
     * Usage:   gap set_periodic_adv [adv_hdl][min_interval][max_interval][properties]
     * Example: gap set_periodic_adv 06 0030 0030 0020
     */
    else if (UT_APP_CMP("gap set_periodic_adv")) {
        bt_hci_le_set_periodic_advertising_parameters_t per_param;
        bt_gap_le_set_periodic_advertising_data_t per_data;
        uint8_t data[19] = "xxperiodic_adv_data";
        uint32_t min_inter;
        uint32_t max_inter;
        uint16_t prop;
        uint8_t adv_hdl;

        adv_hdl = strtoul(cmd + 21, NULL, 16);
        if (adv_hdl > 0xEF) {
            BT_LOGE("APP", "adv hdl in 0~0xEF");
            return BT_STATUS_FAIL;
        }

        min_inter = strtoul(cmd + 24, NULL, 16);
        if (min_inter > 0xFFFF || min_inter < 0x0006) {
            BT_LOGE("APP", "min interval should be 6~0xFFFF");
            return BT_STATUS_FAIL;
        }
        per_param.interval_min = min_inter;  // time = min * 1.25ms

        max_inter = strtoul(cmd + 29, NULL, 16);
        if (max_inter > 0xFFFF || max_inter < 0x0006) {
            BT_LOGE("APP", "max interval should be 6~0xFFFF");
            return BT_STATUS_FAIL;
        }
        per_param.interval_max = max_inter;  // time = max * 1.25ms

        prop = strtoul(cmd + 34, NULL, 16);
        if (prop > 0x7F) {
            BT_LOGE("APP", "properties should < 0x7F");
            return BT_STATUS_FAIL;
        }
        per_param.properties = prop;

        per_data.data_length = 0x13;
        data[0] = 0x12;
        data[1] = 0x09;
        per_data.data = data;
        bt_gap_le_config_periodic_advertising(adv_hdl, &per_param, &per_data);
    }
    /*
     * Usage:   gap enable_periodic_adv [adv_hdl][enable]
     * Example: gap enable_periodic_adv 06 1
     */
    else if (UT_APP_CMP("gap enable_periodic_adv")) {
        uint8_t adv_hdl = 0;
        uint8_t enable = 0;

        adv_hdl = strtoul(cmd + 24, NULL, 16);
        if (adv_hdl > 0xEF) {
            BT_LOGE("APP", "adv hdl in 0~0xEF");
            return BT_STATUS_FAIL;
        }

        enable = strtoul(cmd + 27, NULL, 16);
        if (enable > 1) {
            BT_LOGE("APP", "periodic adv enable is 0~1");
            return BT_STATUS_FAIL;
        }

        bt_gap_le_enable_periodic_advertising(adv_hdl, enable);
    }
    /*
     * Usage:   gap set_periodic_list [opcode][dev_type][dev_addr][adv_sid]
     * Example: gap enable_periodic_adv 1 0 xx-xx-xx-xx-xx-xx 01
     */
    else if (UT_APP_CMP("gap set_periodic_list")) {
        uint8_t op = 0;
        uint16_t op_cmd = 0;
        const char *addr_str = cmd + 26;
        bt_hci_cmd_le_add_device_to_periodic_advertiser_list_t dev_info;


        op = (uint8_t)strtoul(cmd + 22, NULL, 10);
        if (op > 1) {
            BT_LOGE("APP", "opcode should be 0~1");
            return BT_STATUS_FAIL;
        }
        op_cmd = op ? BT_GAP_LE_ADD_TO_PERIODIC_ADV_LIST : BT_GAP_LE_REMOVE_FROM_PERIODIC_ADV_LIST;

        dev_info.advertiser_address_type = (uint8_t)strtoul(cmd + 24, NULL, 10);
        if (dev_info.advertiser_address_type > 1) {
            BT_LOGE("APP", "addr type should be 0~1");
            return BT_STATUS_FAIL;
        }

        copy_str_to_addr(dev_info.advertiser_address, addr_str);

        dev_info.advertising_sid = (uint8_t)strtoul(cmd + 44, NULL, 16);
        if (dev_info.advertising_sid > 0x0F) {
            BT_LOGE("APP", "adv sid should be 0~0xF");
            return BT_STATUS_FAIL;
        }

        bt_gap_le_set_periodic_advertiser_list(op_cmd, (void *)&dev_info);
    }
    /*
     * Usage:   gap periodic_adv_create_sync [adv_sig][filter_policy]
     * Example: gap periodic_adv_create_sync 01 01
     */
    else if (UT_APP_CMP("gap periodic_adv_create_sync")) {
        bt_hci_cmd_le_periodic_advertising_create_sync_t sync_param = { 0 };

        sync_param.advertising_SID = (uint8_t)strtoul(cmd + 29, NULL, 16);
        if (sync_param.advertising_SID > 0x0F) {
            BT_LOGE("APP", "adv sig be 0~F");
            return BT_STATUS_FAIL;
        }
        sync_param.filter_policy = (uint8_t)strtoul(cmd + 32, NULL, 16);
        if (sync_param.filter_policy > 0x03) {
            BT_LOGE("APP", "options be 0~3");
            return BT_STATUS_FAIL;
        }

        sync_param.sync_timeout = 0x400;
        bt_gap_le_periodic_advertising_create_sync(&sync_param);
    }
    /*
     * Usage:   gap periodic_adv_term_sync [sync_hdl]
     * Example: gap periodic_adv_term_sync 00a1
     */
    else if (UT_APP_CMP("gap periodic_adv_term_sync")) {
        bt_handle_t hdl = 0;

        hdl = strtoul(cmd + 27, NULL, 16);
        if (hdl > 0x0EFF) {
            BT_LOGE("APP", "sync hdl should be 0~0xEFF");
            return BT_STATUS_FAIL;
        }
        bt_gap_le_periodic_advertising_terminate_sync(hdl);
    }
    /*
     * Usage:   gap le_create_big [big_hdl][adv_hdl][num_bis][sdu_interval]
     * Example: gap le_create_big 01 0b 03 002710
     */
    else if (UT_APP_CMP("gap le_create_big")) {
        bt_hci_le_create_big_t biginfo = { 0 };

        biginfo.big_handle = strtoul(cmd + 18, NULL, 16);
        if (biginfo.big_handle > 0xEF) {
            BT_LOGE("APP", "big hdl should be 0~0xEF");
            return BT_STATUS_FAIL;
        }

        biginfo.adv_handle = strtoul(cmd + 21, NULL, 16);
        if (biginfo.adv_handle > 0xEF) {
            BT_LOGE("APP", "adv hdl should be 0~0xEF");
            return BT_STATUS_FAIL;
        }

        biginfo.num_of_bis = strtoul(cmd + 24, NULL, 16);
        if (biginfo.num_of_bis > 0x1F) {
            BT_LOGE("APP", "num_of_bis should be 01 ~ 1F");
            return BT_STATUS_FAIL;
        }

        biginfo.sdu_interval = strtoul(cmd + 27, NULL, 16);
        if (biginfo.sdu_interval > 0x0FFFFF) {
            BT_LOGE("APP", "sdu interval should be 100~0xFFFFF");
            return BT_STATUS_FAIL;
        }

        biginfo.max_sdu = 0xF0;
        biginfo.max_transport_latency = 0x3C;
        biginfo.retransmission_number = 0x04;
        biginfo.phy = 0x02;
        bt_gap_le_create_big(&biginfo);
    }
    /*
     * Usage:   gap big_create_sync [big_hdl][sync_hdl][sync_timeout]
     * Example: gap big_create_sync 01 00a1 0bb8
     */
    else if (UT_APP_CMP("gap big_create_sync")) {
        bt_gap_le_big_create_sync_t sync_info = { 0 };
        uint8_t bis[3];

        sync_info.big_handle = strtoul(cmd + 20, NULL, 16);
        if (sync_info.big_handle > 0xEF) {
            BT_LOGE("APP", "big_hdl should be 1~0xEF");
            return BT_STATUS_FAIL;
        }

        sync_info.sync_handle = strtoul(cmd + 23, NULL, 16);
        if (sync_info.sync_handle > 0x0EFF) {
            BT_LOGE("APP", "sync_hdl should be 0~0xEFF");
            return BT_STATUS_FAIL;
        }

        sync_info.big_sync_timeout = strtoul(cmd + 28, NULL, 16);
        if (sync_info.big_sync_timeout > 0x4000 || sync_info.big_sync_timeout < 0x000a) {
            BT_LOGE("APP", "big_sync_timeout should be 0xA~0x4000");
            return BT_STATUS_FAIL;
        }

        sync_info.num_of_bis = 3;
        bis[0] = 1;
        bis[1] = 2;
        bis[2] = 3;
        sync_info.bis_list = bis;
        bt_gap_le_big_create_sync(&sync_info);
    }
    /*
     * Usage:   gap setup_iso_data_path [conn_hdl][conn_dir]
     * Example: gap setup_iso_data_path 0400 00
     */
    else if (UT_APP_CMP("gap setup_iso_data_path")) {
        bt_gap_le_setup_iso_data_path_t iso_data = { 0 };

        iso_data.handle = strtoul(cmd + 24, NULL, 16);
        if (iso_data.handle > 0x0EFF) {
            BT_LOGE("APP", "con_hdl should be 0~0xEFF");
            return BT_STATUS_FAIL;
        }

        //BT_GAP_LE_ISO_DATA_PATH_DIRECTION_INPUT:0, BT_GAP_LE_ISO_DATA_PATH_DIRECTION_OUTPUT:1
        iso_data.direction = strtoul(cmd + 29, NULL, 16);
        if (iso_data.direction > 0x01) {
            BT_LOGE("APP", "data_path_dir should be 0~1");
            return BT_STATUS_FAIL;
        }

        bt_gap_le_setup_iso_data_path(&iso_data);
    }
    /*
     * Usage:   gap remove_iso_data_path [conn_hdl][conn_dir]
     * Example: gap remove_iso_data_path 0400 00
     */
    else if (UT_APP_CMP("gap remove_iso_data_path")) {
        bt_gap_le_remove_iso_data_path_t iso_data = { 0 };

        iso_data.handle = strtoul(cmd + 25, NULL, 16);
        if (iso_data.handle > 0x0EFF) {
            BT_LOGE("APP", "con_hdl should be 0~0xEFF");
            return BT_STATUS_FAIL;
        }

        iso_data.data_path_direction = strtoul(cmd + 30, NULL, 16);
        if (iso_data.data_path_direction > 0x01) {
            BT_LOGE("APP", "data_path_dir should be 0~1");
            return BT_STATUS_FAIL;
        }

        bt_gap_le_remove_iso_data_path(&iso_data);
    }
    /*
     * Usage:   gap set_cig_param [cig_id][cis_count][cis_id1][cis_id2]...
     * Example: gap set_cig_param 03 03 01 02 03
     */
    else if (UT_APP_CMP("gap set_cig_param")) {
        bt_gap_le_set_cig_params_t param = { 0 };
        bt_gap_le_cis_params_t cis_param[4] = { 0 };  // 4 is temp num, you can change it larger
        uint8_t i = 0;

        param.sdu_interval_m_to_s = 0x002710;
        param.sdu_interval_s_to_m = 0x002710;
        param.max_transport_latency_m_to_s = 0x003c;
        param.max_transport_latency_s_to_m = 0x003c;

        param.cig_id = strtoul(cmd + 18, NULL, 16);
        if (param.cig_id > 0xEF) {
            BT_LOGE("APP", "cig id should be 0~0xEF");
            return BT_STATUS_FAIL;
        }

        param.cis_count = strtoul(cmd + 21, NULL, 16);
        if (param.cis_count > 0x1F) {
            BT_LOGE("APP", "cis count should be 0~0x1F");
            return BT_STATUS_FAIL;
        }

        if (param.cis_count > 4)
            param.cis_count = 4;

        for (i = 0; i < param.cis_count; i++) {
            cis_param[i].cis_id = strtoul(cmd + 24 + 3 * i, NULL, 16);
            cis_param[i].max_sdu_m_to_s = 0x0064;
            cis_param[i].max_sdu_s_to_m = 0x0064;
            cis_param[i].phy_m_to_s = 0x03;
            cis_param[i].phy_s_to_m = 0x03;
            cis_param[i].rtn_m_to_s = 0x04;
            cis_param[i].rtn_s_to_m = 0x04;
        }

        param.cis_list = (bt_gap_le_cis_params_t *)cis_param;
        bt_gap_le_set_cig_parameters(&param);
    }
    /*
     * Usage:   gap le_create_cis [cis_count][cis_hdl][acl_hdl][cis_hdl][acl_hdl]...
     * Example: gap le_create_cis 03 0003 0400 0004 0400 0005 0401
     */
    else if (UT_APP_CMP("gap le_create_cis")) {
        bt_gap_le_create_cis_t cis_info = { 0 };
        bt_gap_le_cis_set_t cis_set_info[4] = { 0 };  // 4 is temp number, you can change it larger
        uint8_t idx = 0;

        cis_info.cis_count = strtoul(cmd + 18, NULL, 16);
        if (cis_info.cis_count > 0x1F) {
            BT_LOGE("APP", "cis_count should be 00 ~ 1F");
            return BT_STATUS_FAIL;
        }

        for (idx = 0; idx < cis_info.cis_count && idx < 4; idx++) {
            cis_set_info[idx].cis_connection_handle = strtoul(cmd + 21 + 10 * idx, NULL, 16);
            if (cis_set_info[idx].cis_connection_handle > 0x0EFF) {
                BT_LOGE("APP", "cis_con_hdl should be 0~0xEFF");
                return BT_STATUS_FAIL;
            }

            cis_set_info[idx].acl_connection_handle = strtoul(cmd + 26 + 10 * idx, NULL, 16);
            if (cis_set_info[idx].acl_connection_handle > 0x0EFF) {
                BT_LOGE("APP", "acl_con_hdl should be 0~0xEFF");
                return BT_STATUS_FAIL;
            }
        }

        cis_info.cis_list = (bt_gap_le_cis_set_t *)cis_set_info;
        bt_gap_le_create_cis(&cis_info);
    }
    /*
     * Usage:   gap reply_cis_req [action][conn_hdl][reason]
     * Example: gap reply_cis_req 0 0400       (accept request)
     *          gap reply_cis_req 1 0400 01    (reject request)
     */
    else if (UT_APP_CMP("gap reply_cis_req")) {
        bt_gap_le_reply_cis_request_t req = { 0 };

        req.action = strtoul(cmd + 18, NULL, 16);
        if (req.action > 1) {
            BT_LOGE("APP", "action should be 0 or 1");
            return BT_STATUS_FAIL;
        }

        if (req.action == BT_GAP_LE_REPLY_CIS_ACTION_ACCEPT) {
            req.accept.handle = strtoul(cmd + 20, NULL, 16);
            if (req.accept.handle > 0x0EFF) {
                BT_LOGE("APP", "accept hdl should be 0~0xEFF");
                return BT_STATUS_FAIL;
            }
            bt_gap_le_reply_cis_request(&req);
        } else {
            req.reject.handle = strtoul(cmd + 20, NULL, 16);
            if (req.reject.handle > 0x0EFF) {
                BT_LOGE("APP", "reject hdl should be 0~0xEFF");
                return BT_STATUS_FAIL;
            }

            req.reject.reason = strtoul(cmd + 25, NULL, 16);
            if (req.reject.reason > 0x45) {
                BT_LOGE("APP", "reject reason should be 0~0x45");
                return BT_STATUS_FAIL;
            }
            bt_gap_le_reply_cis_request(&req);
        }
    }
#endif /* #ifdef BT_LE_AUDIO_ENABLE */
    else if (UT_APP_CMP("gap stop_adv")) {
        bt_app_advertising = false;
        bt_hci_cmd_le_set_advertising_enable_t enable;
        enable.advertising_enable = BT_HCI_DISABLE;
        bt_gap_le_set_advertising(&enable, NULL, NULL, NULL);
    }
    /* gap start_scan [scan type] [scan interval] [scan window] [own address type] [scan filter policy] [filter duplicate] [instant display report]
       [scan type]: 0 is passive, 1 is active
       [filter duplicate]: 0:disable, 1: enable, 2: enable, reset for each period
       [instant display report]: 0:disable 1:enable
    */
    else if (UT_APP_CMP("gap start_scan")) {
        ulcn = strtoul(cmd + 15, NULL, 10);
        if (ulcn > 1) {
            BT_LOGE("APP", "gap start_scan, le_scan_type should be 0 ~ 1");
            return BT_STATUS_FAIL;
        }
        scan_para.le_scan_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 17, NULL, 16);
        if (ulcn > 0x4000 || ulcn < 0x4) {
            BT_LOGE("APP", "gap start_scan, le_scan_interval should be 0x0004 ~ 0x4000");
            return BT_STATUS_FAIL;
        }
        scan_para.le_scan_interval = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 22, NULL, 16);
        if (ulcn > 0x4000 || ulcn < 0x4) {
            BT_LOGE("APP", "gap start_scan, le_scan_window should be 0x0004 ~ 0x4000");
            return BT_STATUS_FAIL;
        }
        scan_para.le_scan_window = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 27, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "gap start_scan, own_address_type should be 0 ~ 3");
            return BT_STATUS_FAIL;
        }
        scan_para.own_address_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 29, NULL, 10);
        if (ulcn > 0xFF) {
            BT_LOGE("APP", "gap start_scan, scanning_filter_policy should be 0x00 ~ 0xFF");
            return BT_STATUS_FAIL;
        }
        scan_para.scanning_filter_policy = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 31, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "gap start_scan, filter duplicates should be 00 ~ 02");
            return BT_STATUS_FAIL;
        }
        scan_enable.filter_duplicates = (uint8_t)ulcn;

#ifdef BT_UT_GAP_LE_SCAN_LIST
        /* Used for instant display. 0:Disable instant display 1:Enable instant display*/
        uint8_t is_instant_display = 0;

        if (strlen(cmd) > 33) {
            is_instant_display = (uint8_t)strtoul(cmd + 33, NULL, 10);
            if (is_instant_display > 1) {
                BT_LOGE("APP", "gap start_scan, instant display report should be 0 or 1");
                return BT_STATUS_FAIL;
            }
        }

        if (!is_instant_display)
            bt_ut_gap_le_scan_list_init();
#endif /* #ifdef BT_UT_GAP_LE_SCAN_LIST */

        bt_app_scanning = true;
        bt_gap_le_set_scan(&scan_enable, &scan_para);
    }

#ifdef BT_UT_GAP_LE_SCAN_LIST
    else if (UT_APP_CMP("gap list_scan")) {
        bt_ut_gap_le_scan_list_dump();
    }
#endif /* #ifdef BT_UT_GAP_LE_SCAN_LIST */

    else if (UT_APP_CMP("gap stop_scan")) {
        bt_app_scanning = false;
        bt_gap_le_set_scan(&scan_disable, NULL);
#ifdef BT_UT_GAP_LE_SCAN_LIST
        bt_ut_gap_le_scan_list_deinit();
#endif /* #ifdef BT_UT_GAP_LE_SCAN_LIST */
    }

    /* Usage: gap connect [0:public / 1:random] [bt address].
        Note:  default use #lt_addr_type and #lt_addr */
    else if (UT_APP_CMP("gap connect")) {
        if (strlen(cmd) >= 12) {
            uint8_t peer_addr_type;
            const char *addr_str = cmd + 14;
            uint8_t addr[6];

            ulcn = strtoul(cmd + 12, NULL, 10);
            if (ulcn > 3) {
                BT_LOGE("APP", "gap connect, peer_addr_type should be 0 ~ 3");
                return BT_STATUS_FAIL;
            }
            peer_addr_type = (uint8_t)ulcn;

            copy_str_to_addr(addr, addr_str);
            connect_para.peer_address.type = peer_addr_type;
            memcpy(connect_para.peer_address.addr, addr, sizeof(addr));
            bt_gap_le_connect(&connect_para);
        } else {
        }
    }

    /* Usage: gap advanced_conn [scan interval] [scan window] [initiator_filter_policy] [peer_address_type] [peer_address] [own_address_type]
              [conn_interval_min] [conn_interval_max] [conn_latency] [supervision_timeout] [minimum_ce_length] [maximum_ce_length].
    */
    else if (UT_APP_CMP("gap advanced_conn")) {
        if (strlen(cmd) >= 18) {
            const char *addr_str = cmd + 32;
            uint8_t addr[6];

            ulcn = strtoul(cmd + 18, NULL, 16);
            if (ulcn > 0x4000 || ulcn < 0x4) {
                BT_LOGE("APP", "le_scan_interval: 0x0004~0x4000");
                return BT_STATUS_FAIL;
            }
            connect_para.le_scan_interval = (uint16_t)ulcn;

            ulcn = strtoul(cmd + 23, NULL, 16);
            if (ulcn > 0x4000 || ulcn < 0x4) {
                BT_LOGE("APP", "le_scan_window: 0x0004~0x4000");
                return BT_STATUS_FAIL;
            }
            connect_para.le_scan_window = (uint16_t)ulcn;

            ulcn = strtoul(cmd + 28, NULL, 10);
            if (ulcn > 0xFF) {
                BT_LOGE("APP", "initiator_filter_policy: 0x00~0xFF");
                return BT_STATUS_FAIL;
            }
            connect_para.initiator_filter_policy = (uint8_t)ulcn;

            ulcn = strtoul(cmd + 30, NULL, 10);
            if (ulcn > 3) {
                BT_LOGE("APP", "peer_addr_type: 0~3");
                return BT_STATUS_FAIL;
            }
            connect_para.peer_address.type = (uint8_t)ulcn;

            copy_str_to_addr(addr, addr_str);
            memcpy(connect_para.peer_address.addr, addr, sizeof(addr));
            ulcn = strtoul(cmd + 50, NULL, 10);
            if (ulcn > 3) {
                BT_LOGE("APP", "own_addr_type: 0~3");
                return BT_STATUS_FAIL;
            }
            connect_para.own_address_type = (uint8_t)ulcn;

            ulcn = strtoul(cmd + 52, NULL, 16);
            if (ulcn > 0xC80 || ulcn < 0x6) {
                BT_LOGE("APP", "conn_interval_min: 0x0006~0x0C80");
                return BT_STATUS_FAIL;
            }
            connect_para.conn_interval_min = (uint16_t)ulcn;

            ulcn = strtoul(cmd + 57, NULL, 16);
            if (ulcn > 0xC80 || ulcn < 0x6) {
                BT_LOGE("APP", "conn_interval_max: 0x0006~0x0C80");
                return BT_STATUS_FAIL;
            }
            connect_para.conn_interval_max = (uint16_t)ulcn;

            ulcn = strtoul(cmd + 62, NULL, 16);
            if (ulcn > 0x1F3) {
                BT_LOGE("APP", "conn_latency: 0x0000~0x01F3");
                return BT_STATUS_FAIL;
            }
            connect_para.conn_latency = (uint16_t)ulcn;

            ulcn = strtoul(cmd + 67, NULL, 16);
            if (ulcn > 0xC80 || ulcn < 0xA) {
                BT_LOGE("APP", "super_tmout: 0x000A~0x0C80");
                return BT_STATUS_FAIL;
            }
            connect_para.supervision_timeout = (uint16_t)ulcn;

            ulcn = strtoul(cmd + 72, NULL, 16);
            if (ulcn > 0xFFFF) {
                BT_LOGE("APP", "min_ce_len: 0x0000~0xFFFF");
                return BT_STATUS_FAIL;
            }
            connect_para.minimum_ce_length = (uint16_t)ulcn;

            ulcn = strtoul(cmd + 77, NULL, 16);
            if (ulcn > 0xFFFF) {
                BT_LOGE("APP", "max_ce_len: 0x0000~0xFFFF");
                return BT_STATUS_FAIL;
            }
            connect_para.maximum_ce_length = (uint16_t)ulcn;
            bt_gap_le_connect(&connect_para);
        } else {
        }
    }

    else if (UT_APP_CMP("gap cancel connect")) {
        bt_gap_le_cancel_connection();
    }

    /* Usage:   disconnect <handle in hex>
       Example: disconnect 0200 */
    else if (UT_APP_CMP("gap disconnect")) {
        const char *handle = cmd + strlen("gap disconnect ");
        ulcn = strtoul(handle, NULL, 16);
        if (ulcn > 0xFFFF) {
            BT_LOGE("APP", "gap disconnect, connection_handle should be 0x0000 ~ 0xFFFF");
            return BT_STATUS_FAIL;
        }
        disconnect_para.connection_handle = (uint16_t)ulcn;
        BT_LOGI("APP", "connection_handle(0x%04x)", disconnect_para.connection_handle);
        bt_gap_le_disconnect(&disconnect_para);
    }

    else if (UT_APP_CMP("gap read_rssi")) {
        ulcn = strtoul(cmd + 14, NULL, 16);
        if (ulcn > 0xFFFF) {
            BT_LOGE("APP", "gap read_rssi, connection_handle should be 0x0000 ~ 0xFFFF");
            return BT_STATUS_FAIL;
        }
        read_rssi.handle = (uint16_t)ulcn;
        bt_gap_le_read_rssi(&read_rssi);
    }

    else if (UT_APP_CMP("gap update_conn")) {
        ulcn = strtoul(cmd + 16, NULL, 16);
        if (ulcn > 0x0EFF) {
            BT_LOGE("APP", "conn_hdl: 0x0000~0x0EFF");
            return BT_STATUS_FAIL;
        }
        conn_update_para.connection_handle = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 21, NULL, 16);
        if (ulcn > 0xC80 || ulcn < 0x6) {
            BT_LOGE("APP", "conn_interval_min: 0x0006~0x0C80");
            return BT_STATUS_FAIL;
        }
        conn_update_para.conn_interval_min = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 26, NULL, 16);
        if (ulcn > 0xC80 || ulcn < 0x6) {
            BT_LOGE("APP", "conn_interval_max: 0x0006~0x0C80");
            return BT_STATUS_FAIL;
        }
        conn_update_para.conn_interval_max = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 31, NULL, 16);
        if (ulcn > 0x1F3) {
            BT_LOGE("APP", "conn_latency: 0x0000~0x01F3");
            return BT_STATUS_FAIL;
        }
        conn_update_para.conn_latency = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 36, NULL, 16);
        if (ulcn > 0xC80 || ulcn < 0xA) {
            BT_LOGE("APP", "super_tmout: 0x000A~0x0C80");
            return BT_STATUS_FAIL;
        }
        conn_update_para.supervision_timeout = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 41, NULL, 16);
        if (ulcn > 0xFFFF) {
            BT_LOGE("APP", "mini_ce_len: 0x0000~0xFFFF");
            return BT_STATUS_FAIL;
        }
        conn_update_para.minimum_ce_length = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 46, NULL, 16);
        if (ulcn > 0xFFFF) {
            BT_LOGE("APP", "max_ce_len: 0x0000~0xFFFF");
            return BT_STATUS_FAIL;
        }
        conn_update_para.maximum_ce_length = (uint16_t)ulcn;
        bt_gap_le_update_connection_parameter(&conn_update_para);
    }

    /* Usage: update data length <handle in hex> <tx octets in hex> <tx time in hex>.
       Example: update data length 0200 0030 0500*/
    else if (UT_APP_CMP("gap update data length")) {
        bt_hci_cmd_le_set_data_length_t data_length;

        ulcn = strtoul(cmd + 23, NULL, 16);
        if (ulcn > 0x0EFF) {
            BT_LOGE("APP", "gap update data length, connection_handle should be 0x0000 ~ 0x0EFF");
            return BT_STATUS_FAIL;
        }
        data_length.connection_handle = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 28, NULL, 16);
        if (ulcn > 0x00FB || ulcn < 0x001B) {
            BT_LOGE("APP", "gap update data length, tx_octets should be 0x001B ~ 0x00FB");
            return BT_STATUS_FAIL;
        }
        data_length.tx_octets = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 33, NULL, 16);
        if (ulcn > 0x0848 || ulcn < 0x0148) {
            BT_LOGE("APP", "gap update data length, tx_time should be 0x0148 ~ 0x0848");
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

    /* Usage: tx_power <handle in hex> <tx power level in integer>.
       [tx power level]: 0~7
       Example: tx_power 0200 1*/
    else if (UT_APP_CMP("gap tx_power")) {
        bt_hci_cmd_le_set_tx_power_t tx_power_t;

        ulcn = strtoul(cmd + 13, NULL, 16);
        if (ulcn > 0x0EFF) {
            BT_LOGE("APP", "gap tx_power, connection_handle should be 0x0000 ~ 0x0EFF");
            return BT_STATUS_FAIL;
        }
        tx_power_t.connection_handle = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 18, NULL, 10);
        if (ulcn > 7) {
            BT_LOGE("APP", "gap tx_power, tx_power_level should be 0 ~ 7");
            return BT_STATUS_FAIL;
        }
        tx_power_t.tx_power_level = (uint16_t)ulcn;

        if (tx_power_t.connection_handle > 0x0f00 ||
            tx_power_t.tx_power_level > 7) {
            BT_LOGW("APP", "Usage: tx_power <handle in hex> <tx power level in integer>.");
            BT_LOGW("APP", "The range of connection handle is 0x0000-0x0EFF");
            BT_LOGW("APP", "The range of tx power level is 0-7");
        } else {
            BT_LOGI("APP", "tx power handle(%04x) tx_power_level(%d)",
                    tx_power_t.connection_handle, tx_power_t.tx_power_level);
            bt_gap_le_set_tx_power(&tx_power_t);
        }
    }

    /* Usage:   gap bond <handle in hex> [io capability] [oob data flag] [auth req]
                [initiator_key_distribution] [responder_key_distribution]
       Example: gap bond 0200 3 0 1 0 0*/
    else if (UT_APP_CMP("gap bond")) {
        const char *handle = cmd + strlen("gap bond ");

        ulcn = strtoul(cmd + 14, NULL, 10);
        if (ulcn > 5) {
            BT_LOGE("APP", "gap bond, io_capability should be 0 ~ 5");
            return BT_STATUS_FAIL;
        }
        pairing_config_req.io_capability = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 16, NULL, 10);
        if (ulcn > 1) {
            BT_LOGE("APP", "gap bond, oob_data_flag should be 0 ~ 1");
            return BT_STATUS_FAIL;
        }
        pairing_config_req.oob_data_flag = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 18, NULL, 10);
        if (ulcn > 0xFF) {
            BT_LOGE("APP", "gap bond, auth_req should be 0x00 ~ 0xFF");
            return BT_STATUS_FAIL;
        }
        pairing_config_req.auth_req = (uint8_t)ulcn;
        pairing_config_req.maximum_encryption_key_size = 16;

        ulcn = strtoul(cmd + 20, NULL, 10);
        if (ulcn > 0xFF) {
            BT_LOGE("APP", "gap bond, initiator_key_distribution should be 0x00 ~ 0xFF");
            return BT_STATUS_FAIL;
        }
        pairing_config_req.initiator_key_distribution = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 22, NULL, 10);
        if (ulcn > 0xFF) {
            BT_LOGE("APP", "gap bond, responder_key_distribution should be 0x00 ~ 0xFF");
            return BT_STATUS_FAIL;
        }
        pairing_config_req.responder_key_distribution = (uint8_t)ulcn;

        ulcn = strtoul(handle, NULL, 16);
        if (ulcn > 0xFFFF) {
            BT_LOGE("APP", "gap bond, handle should be 0x0000 ~ 0xFFFF");
            return BT_STATUS_FAIL;
        }
#ifdef MTK_BLE_DM_SUPPORT
        bt_device_manager_le_gap_set_pairing_configuration(&pairing_config_req);
#endif /* #ifdef MTK_BLE_DM_SUPPORT */
        bt_gap_le_bond(ulcn, &pairing_config_req);
    } else if (UT_APP_CMP("gap set_phy")) {
        const char *handle = cmd + strlen("gap set_phy ");
        bt_hci_le_set_phy_t phy_info;

        ulcn = strtoul(cmd + 17, NULL, 10);
        if (ulcn > (BT_HCI_LE_PHY_MASK_1M | BT_HCI_LE_PHY_MASK_2M | BT_HCI_LE_PHY_MASK_CODED)) {
            BT_LOGE("APP", "tx phy type should be 0 ~ 7");
            return BT_STATUS_FAIL;
        }
        phy_info.tx = ulcn;

        ulcn = strtoul(cmd + 19, NULL, 10);
        if (ulcn > (BT_HCI_LE_PHY_MASK_1M | BT_HCI_LE_PHY_MASK_2M | BT_HCI_LE_PHY_MASK_CODED)) {
            BT_LOGE("APP", "rx phy type should be 0 ~ 7");
            return BT_STATUS_FAIL;
        }
        phy_info.rx = ulcn;

        ulcn = strtoul(cmd + 21, NULL, 10);
        if (ulcn > BT_HCI_LE_PHY_CODED_S8) {
            BT_LOGE("APP", "tx phy options should be 0 ~ 2");
            return BT_STATUS_FAIL;
        }
        phy_info.tx_options = ulcn;

        ulcn = strtoul(handle, NULL, 16);
        if (ulcn > 0x0EFF) {
            BT_LOGE("APP", "handle should be 0x0000 ~ 0x0EFF");
            return BT_STATUS_FAIL;
        }

        bt_gap_le_set_phy(ulcn, &phy_info);
    } else if (UT_APP_CMP("gap get_phy")) {
        const char *handle = cmd + strlen("gap get_phy ");
        ulcn = strtoul(handle, NULL, 16);
        if (ulcn > 0x0EFF) {
            BT_LOGE("APP", "handle should be 0x0000 ~ 0x0EFF");
            return BT_STATUS_FAIL;
        }
        bt_gap_le_get_phy(ulcn);
    } else if (UT_APP_CMP("gap txpow_dbm")) {
        uint8_t cmd_param[3];
        cmd_param[0] = 0x08; // sub_opcode

        ulcn = strtoul(cmd + 14, NULL, 10);
        if (ulcn != 0 && ulcn != 1) {
            BT_LOGE("APP", "gap txpow_dbm(0x%x), only accept 0: disable, 1: enable", ulcn);
            return BT_STATUS_FAIL;
        }
        cmd_param[1] = (uint8_t)ulcn; // enable/disable

        ulcn = strtoul(cmd + 16, NULL, 16);
        if (ulcn > 0xff) {
            BT_LOGE("APP", "gap txpow_dbm(0x%x), only accept 1 byte", ulcn);
            return BT_STATUS_FAIL;
        }
        cmd_param[2] = (uint8_t)ulcn; // power value, unit is dbm
        bt_hci_send_vendor_cmd(HCI_CMDCODE_SET_TX_POWER_DBM, cmd_param, sizeof(cmd_param));

    }
#ifdef MTK_BT_EXTRA_CLI_TEST
    else if (UT_APP_CMP("gap get_max_adv_len")) {
        uint32_t adv_len = bt_gap_le_get_max_adv_length();
        BT_LOGE("APP", "max adv len: %04x", adv_len);
    }

    /* Usage: ble gap set_pub_addr <public addr>
       Example: ble gap set_pub_addr 0C-20-20-20-20-20 */
    else if (UT_APP_CMP("gap set_pub_addr")) {
        if (strlen(cmd) >= 17) {
            const char *addr_str = cmd + 17;
            uint8_t addr[6];
            copy_str_to_addr(addr, addr_str);
            bt_set_local_public_address(addr);
        } else {
            BT_LOGE("APP", "input pub addr");
            return BT_STATUS_FAIL;
        }
    }
#endif /* #ifdef MTK_BT_EXTRA_CLI_TEST */
    else {
        return bqb_gap_io_callback(input, output);
    }
    return BT_STATUS_SUCCESS;
}
