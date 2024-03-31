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

extern bool bt_app_advertising;
extern bool bt_app_scanning;

bt_status_t bt_app_gap_io_callback(void *input, void *output)
{
    const char *cmd = input;
    unsigned long ulcn = 0;

    if (UT_APP_CMP("gap start_adv")) {
        const char *str = cmd + 14;
        uint8_t advdata[] = {0x02, 0x01, 0x06, 0x06, 0x09, 0x4d, 0x54, 0x4b, 0x48, 0x42};

        bt_app_advertising = true;
        if (!memcmp(str, "def", 3)) {
            adv_data.advertising_data_length = sizeof(advdata);
            memcpy(adv_data.advertising_data, advdata, sizeof(advdata));
        } else {
            memset(gatts_device_name, 0x00, sizeof(gatts_device_name));
            memcpy(gatts_device_name, &adv_data.advertising_data[5], 3);
        }

        BT_COLOR_SET(BT_COLOR_BLUE);
        BT_LOGI("APP", "start advertising");
        BT_COLOR_SET(BT_COLOR_WHITE);

        adv_enable.advertising_enable = BT_HCI_ENABLE;
        bt_gap_le_set_advertising(&adv_enable, &adv_para, &adv_data, &scan_data);
    }

    else if (UT_APP_CMP("gap stop_adv")) {
        bt_app_advertising = false;
        bt_hci_cmd_le_set_advertising_enable_t enable;
        enable.advertising_enable = BT_HCI_DISABLE;
        bt_gap_le_set_advertising(&enable, NULL, NULL, NULL);
    }
    /* gap start_scan [scan type] [scan interval] [scan window] [own address type] [scan filter policy] [filter duplicate]
       [scan type]: 0 is passive, 1 is active
       [filter duplicate]: 0:disable, 1: enable, 2: enable, reset for each period
    */
    else if (UT_APP_CMP("gap start_scan")) {
        ulcn = strtoul(cmd + 15, NULL, 10);
        if (ulcn > 1) {
            BT_LOGE("APP", "scan_type: 0 or 1");
            return BT_STATUS_FAIL;
        }
        scan_para.le_scan_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 17, NULL, 16);
        if (ulcn > 0x4000 || ulcn < 0x4) {
            BT_LOGE("APP", "scan_interval: 0x0004~0x4000");
            return BT_STATUS_FAIL;
        }
        scan_para.le_scan_interval = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 22, NULL, 16);
        if (ulcn > 0x4000 || ulcn < 0x4) {
            BT_LOGE("APP", "scan_window: 0x0004~0x4000");
            return BT_STATUS_FAIL;
        }
        scan_para.le_scan_window = (uint16_t)ulcn;

        ulcn = strtoul(cmd + 27, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "own_addr_type: 0~3");
            return BT_STATUS_FAIL;
        }
        scan_para.own_address_type = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 29, NULL, 10);
        if (ulcn > 0xFF) {
            BT_LOGE("APP", "scanning_filter_policy: 0x00~0xFF");
            return BT_STATUS_FAIL;
        }
        scan_para.scanning_filter_policy = (uint8_t)ulcn;

        ulcn = strtoul(cmd + 31, NULL, 10);
        if (ulcn > 3) {
            BT_LOGE("APP", "filter dup: 0x00~0x02");
            return BT_STATUS_FAIL;
        }
        scan_enable.filter_duplicates = (uint8_t)ulcn;

        bt_app_scanning = true;
        bt_gap_le_set_scan(&scan_enable, &scan_para);
    }

    else if (UT_APP_CMP("gap stop_scan")) {
        bt_app_scanning = false;
        bt_gap_le_set_scan(&scan_disable, NULL);
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
                BT_LOGE("APP", "peer_addr_type: 0~3");
                return BT_STATUS_FAIL;
            }
            peer_addr_type = (uint8_t)ulcn;

            copy_str_to_addr(addr, addr_str);
            connect_para.peer_address.type = peer_addr_type;
            memcpy(connect_para.peer_address.addr, addr, sizeof(addr));
            bt_gap_le_connect(&connect_para);
        } else {
        }
    } else if (UT_APP_CMP("gap cancel connect")) {
        bt_gap_le_cancel_connection();
    }

    /* Usage:   disconnect <handle in hex>
       Example: disconnect 0200 */
    else if (UT_APP_CMP("gap disconnect")) {
        const char *handle = cmd + strlen("gap disconnect ");
        ulcn = strtoul(handle, NULL, 16);
        if (ulcn > 0x0EFF) {
            BT_LOGE("APP", "conn_hdl: 0~0x0EFF");
            return BT_STATUS_FAIL;
        }
        disconnect_para.connection_handle = (uint16_t)ulcn;
        BT_LOGI("APP", "conn_hdl(0x%04x)", disconnect_para.connection_handle);
        bt_gap_le_disconnect(&disconnect_para);
    } else {
        return BT_STATUS_SUCCESS;
    }
    return BT_STATUS_SUCCESS;
}
