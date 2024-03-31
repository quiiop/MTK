/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef __AUTOBT_H_
#define __AUTOBT_H_

#include <stdbool.h>

#ifndef FALSE
#define FALSE     0
#endif
#ifndef TRUE
#define TRUE      1
#endif
#ifndef BOOL
#define BOOL      bool
#endif

typedef unsigned char UCHAR;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

#define BTTOOL_LOG_LVL_V	4
#define BTTOOL_LOG_LVL_D	3
#define BTTOOL_LOG_LVL_I	2
#define BTTOOL_LOG_LVL_W	1
#define BTTOOL_LOG_LVL_E	0

#define BTTOOL_LOGV(fmt, args...) \
    do { if (bt_tool_get_log_lvl() >= BTTOOL_LOG_LVL_V) printf("[BTTOOL][V]"fmt"\r\n", ##args); } while(0)
#define BTTOOL_LOGD(fmt, args...) \
    do { if (bt_tool_get_log_lvl() >= BTTOOL_LOG_LVL_D) printf("[BTTOOL][D]"fmt"\r\n", ##args); } while(0)
#define BTTOOL_LOGI(fmt, args...) \
    do { if (bt_tool_get_log_lvl() >= BTTOOL_LOG_LVL_I) printf("[BTTOOL][I]"fmt"\r\n", ##args); } while(0)
#define BTTOOL_LOGW(fmt, args...) \
    do { if (bt_tool_get_log_lvl() >= BTTOOL_LOG_LVL_W) printf("[BTTOOL][W]"fmt"\r\n", ##args); } while(0)
#define BTTOOL_LOGE(fmt, args...) \
    do { if (bt_tool_get_log_lvl() >= BTTOOL_LOG_LVL_E) printf("[BTTOOL][E]"fmt"\r\n", ##args); } while(0)
#define BTTOOL_LOG_FUNC() \
    do { if (bt_tool_get_log_lvl() >= BTTOOL_LOG_LVL_D) printf("[BTTOOL][F]%s() L %d\r\n", __func__, __LINE__); } while(0)


enum {
    BT_HW_TEST_CMD_NONE = 0,
    BT_HW_TEST_CMD_TX,
    BT_HW_TEST_CMD_NSRX,
    BT_HW_TEST_CMD_TEST_MODE,
    BT_HW_TEST_CMD_INQUIRY,
    BT_HW_TEST_CMD_BLE_TX,
    BT_HW_TEST_CMD_BLE_RX,
    BT_HW_TEST_CMD_STOP,
    BT_HW_TEST_CMD_RELAYER,
    BT_HW_TEST_CMD_GET_RX,
    BT_HW_TEST_CMD_TX_PWR_OFFSET,
    BT_HW_TEST_CMD_MAX,
};

typedef struct _CMD_TX_PARAMS
{
    UINT8 tx_pattern;
    UINT8 hopping;
    int channel;
    UINT8 packet_type;
    UINT32 packet_len;
}CMD_TX_PARAMS_T;

typedef struct _CMD_NSRX_PARAMS
{
    UINT8 rx_pattern;
    int channel;
    UINT8 packet_type;
    UINT32 tester_addr;
}CMD_NSRX_PARAMS_T;

typedef struct _CMD_BLE_TX_PARAMS
{
    UINT8 tx_pattern;
    int channel;
}CMD_BLE_TX_PARAMS_T;

typedef struct _CMD_BLE_RX_PARAMS
{
    int channel;
}CMD_BLE_RX_PARAMS_T;

typedef struct _BT_HW_TEST_CMD
{
    UINT8 cmd;
    int test_duration_s;
    union {
        CMD_TX_PARAMS_T cmd_tx;
        CMD_NSRX_PARAMS_T cmd_nsrx;
        int cmd_tm_power;
        CMD_BLE_TX_PARAMS_T cmd_ble_tx;
        CMD_BLE_RX_PARAMS_T cmd_ble_rx;
    }params;
}BT_HW_TEST_CMD_T;


int bt_tool_init(void);
void bt_tool_cmd_submit(BT_HW_TEST_CMD_T *target_cmd);

#endif

