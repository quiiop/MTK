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

#include "bt_type.h"
#include "bt_debug.h"
#include "syslog.h"

#ifdef __BT_DEBUG__
log_create_module(BT, PRINT_LEVEL_INFO);
log_create_module(BTMM, PRINT_LEVEL_INFO);
log_create_module(BTHCI, PRINT_LEVEL_INFO);
log_create_module(BTL2CAP, PRINT_LEVEL_INFO);
log_create_module(BTRFCOMM, PRINT_LEVEL_INFO);
log_create_module(BTSPP, PRINT_LEVEL_INFO);

//#define BT_DEBUG_NO_BTIF
#define BT_DEBUG_BUFF_SIZE  200 //Some HB log may over 150bytes, so enlarge to 200
static char bt_debug_buff[BT_DEBUG_BUFF_SIZE];
static char bt_tmp_debug_buff[BT_DEBUG_BUFF_SIZE];

#ifdef __MTK_BT_MESH_ENABLE__
static bool g_bt_debug_mesh_filter = false; //To control some verbose logs can be printed or not
bool bt_debug_check_mesh_filter(const char **pMsg);
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */

#ifdef __MTK_BT_SINK_SUPPORT__
static bool g_bt_debug_sink_filter = true; //To control some verbose logs can be printed or not
bool bt_debug_check_sink_filter(const char **pMsg);
#endif /* #ifdef __MTK_BT_SINK_SUPPORT__ */

void bt_debug_log(const char *module, const char *function_name, const uint16_t line_num, const char *format, ...)
{
    uint8_t len = 0;
    int cnt = 0;
    va_list arg;

    if (0
#ifdef BT_DEBUG_NO_MM
        || strstr(module, "[MM]")
#endif /* #ifdef BT_DEBUG_NO_MM */
#ifdef BT_DEBUG_NO_TIMER
        || strstr(module, "[TIMER]")
#endif /* #ifdef BT_DEBUG_NO_TIMER */
#ifdef BT_DEBUG_NO_HCI
        || strstr(module, "[HCI]")
#endif /* #ifdef BT_DEBUG_NO_HCI */
#ifdef BT_DEBUG_NO_BTIF
        || strstr(module, "BTIF")
#endif /* #ifdef BT_DEBUG_NO_BTIF */
#ifdef BT_DEBUG_NO_GAP
        || strstr(module, "[GAP]")
#endif /* #ifdef BT_DEBUG_NO_GAP */
#ifdef BT_DEBUG_NO_A2DP
        || strstr(module, "[A2DP]")
#endif /* #ifdef BT_DEBUG_NO_A2DP */
#ifdef BT_DEBUG_NO_AVDTP
        || strstr(module, "[AVDTP]")
#endif /* #ifdef BT_DEBUG_NO_AVDTP */

#ifdef BT_DEBUG_NO_I
        || strstr(module, "[I]")
#endif /* #ifdef BT_DEBUG_NO_I */
#ifdef BT_DEBUG_NO_D
        || strstr(module, "[D]")
#endif /* #ifdef BT_DEBUG_NO_D */
       ) {
        return;
    }

#ifdef __MTK_BT_MESH_ENABLE__
    if (bt_debug_check_mesh_filter(&format))
        return;
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */

#ifdef __MTK_BT_SINK_SUPPORT__
    if (bt_debug_check_sink_filter(&format))
        return;
#endif /* #ifdef __MTK_BT_SINK_SUPPORT__ */


    len = strlen(module);
    memset(bt_debug_buff, 0, sizeof(bt_debug_buff));
    memcpy(bt_debug_buff, module, len);

    va_start(arg, format);
    cnt = vsnprintf(bt_debug_buff + len, BT_DEBUG_BUFF_SIZE - len, format, arg);
    if (cnt < 0 || cnt > BT_DEBUG_BUFF_SIZE - len) {
        LOG_E(BT, "vsnprintf error, count out of bound (cnt = %d)", cnt);
        va_end(arg);
        return;
    }
    va_end(arg);
    bt_debug_buff[BT_DEBUG_BUFF_SIZE - 1] = '\0';
    if (strstr(bt_debug_buff, "[MM]")) {
        LOG_I(BTMM, "[%s(%d)]%s", function_name, line_num, bt_debug_buff);
    } else if (strstr(bt_debug_buff, "[RFCOMM]")) {
        LOG_I(BTRFCOMM, "[%s(%d)]%s", function_name, line_num, bt_debug_buff);
    } else if (strstr(bt_debug_buff, "[SPP]")) {
        LOG_I(BTSPP, "[%s(%d)]%s", function_name, line_num, bt_debug_buff);
    } else if (strstr(bt_debug_buff, "[L2CAP]")) {
        LOG_I(BTL2CAP, "%s", &bt_debug_buff[len]); //ignore duplicated name by shifting len
    } else if (strstr(bt_debug_buff, "[HCI]")) {
        LOG_I(BTHCI, "[%s(%d)]%s", function_name, line_num, &bt_debug_buff[len]); //ignore duplicated name by shifting len
    }  else if (strstr(bt_debug_buff, "[BTIF]")) {
        LOG_I(BTIF, "[%s(%d)]%s", function_name, line_num, bt_debug_buff);
#ifdef __MTK_BT_SINK_SUPPORT__
    }  else if (strstr(bt_debug_buff, "[BT_CMGR]")) {
        LOG_I(BT, "%s", bt_debug_buff);
    }  else if (strstr(bt_debug_buff, "[SDP]")) {
        LOG_I(BT, "%s", &bt_debug_buff[len]); //ignore duplicated name by shifting len
    }  else if (strstr(bt_debug_buff, "[A2DP]") || strstr(bt_debug_buff, "[AVDTP]") ||
                strstr(bt_debug_buff, "[AVRCP]")) {
        LOG_I(BT, "%s", &bt_debug_buff[len]); //ignore duplicated name by shifting len
#endif /* #ifdef __MTK_BT_SINK_SUPPORT__ */
    } else {
        LOG_I(BT, "[%s(%d)]%s", function_name, line_num, bt_debug_buff);
    }
}

const char *bt_debug_bd_addr2str(const bt_bd_addr_t addr)
{
    int cn = 0;

    cn = snprintf(bt_tmp_debug_buff, BT_DEBUG_BUFF_SIZE, "%02x-%02x-%02x-%02x-%02x-%02x",
                  addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    if (cn < 0 || cn > BT_DEBUG_BUFF_SIZE) {
        LOG_E(BT, "%s, L: %d, snprintf error, ret = %d", __func__, __LINE__, cn);
        return NULL;
    }
    return bt_tmp_debug_buff;
}

const char *bt_debug_bd_addr2str2(const bt_bd_addr_t addr)
{
    int cn = 0;

    cn = snprintf(bt_tmp_debug_buff, BT_DEBUG_BUFF_SIZE, "LAP: %02x-%02x-%02x, UAP: %02x, NAP: %02x-%02x",
                  addr[2], addr[1], addr[0], addr[3], addr[5], addr[4]);
    if (cn < 0 || cn > BT_DEBUG_BUFF_SIZE) {
        LOG_E(BT, "%s, L: %d, snprintf error, ret = %d", __func__, __LINE__, cn);
        return NULL;
    }
    return bt_tmp_debug_buff;
}

const char *bt_debug_addr2str(const bt_addr_t *p)
{
    int cn = 0;
    bt_bd_addr_ptr_t addr = p->addr;

    cn = snprintf(bt_tmp_debug_buff, BT_DEBUG_BUFF_SIZE, "[%s%s] %02x-%02x-%02x-%02x-%02x-%02x",
                  (p->type & 0x01) == BT_ADDR_PUBLIC ? "PUBLIC" : "RANDOM",
                  p->type >= 2 ? "_IDENTITY" : "",
                  addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    if (cn < 0 || cn > BT_DEBUG_BUFF_SIZE) {
        LOG_E(BT, "%s, L: %d, snprintf error, ret = %d", __func__, __LINE__, cn);
        return NULL;
    }
    return bt_tmp_debug_buff;
}

const char *bt_debug_addr2str2(const bt_addr_t *p)
{
    int cn = 0;
    bt_bd_addr_ptr_t addr = p->addr;

    cn = snprintf(bt_tmp_debug_buff, BT_DEBUG_BUFF_SIZE, "[%s%s] LAP: %02x-%02x-%02x, UAP: %02x, NAP: %02x-%02x",
                  (p->type & 0x01) == BT_ADDR_PUBLIC ? "PUBLIC" : "RANDOM",
                  p->type >= 2 ? "_IDENTITY" : "",
                  addr[2], addr[1], addr[0], addr[3], addr[5], addr[4]);
    if (cn < 0 || cn > BT_DEBUG_BUFF_SIZE) {
        LOG_E(BT, "%s, L: %d, snprintf error, ret = %d", __func__, __LINE__, cn);
        return NULL;
    }
    return bt_tmp_debug_buff;
}

const char *bt_debug_bd_addr2str3(const bt_bd_addr_t addr)
{
    int cn = 0;
    memset(bt_tmp_debug_buff, 0, BT_DEBUG_BUFF_SIZE);

    cn = snprintf(bt_tmp_debug_buff, BT_DEBUG_BUFF_SIZE, "%02X:%02X:%02X:%02X:%02X:%02X",
                  addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    if (cn < 0 || cn > BT_DEBUG_BUFF_SIZE) {
        LOG_E(BT, "%s, L: %d, snprintf error, ret = %d", __func__, __LINE__, cn);
        return NULL;
    }
    return bt_tmp_debug_buff;
}

#ifdef __MTK_BT_MESH_ENABLE__
void bt_debug_set_mesh_filter(bool isOn)
{
    g_bt_debug_mesh_filter = isOn;
}

bool bt_debug_get_mesh_filter(void)
{
    return g_bt_debug_mesh_filter;
}

/* The purpose of this function is to filter logs in HB. */
bool bt_debug_check_mesh_filter(const char **pMsg)
{

    uint32_t msgAddr = (uint32_t) * pMsg;

    if (bt_debug_get_mesh_filter() == false)
        return false;

    if (msgAddr == (uint32_t)&BTGAP_228
        || msgAddr == (uint32_t)&BTGAP_241
        || msgAddr == (uint32_t)&BTGAP_248
        //GATT
        || msgAddr == (uint32_t)&BTATT_010
        || msgAddr == (uint32_t)&BTHCI_024
       )
        return true;

    return false;
}
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */

#ifdef __MTK_BT_SINK_SUPPORT__
void bt_debug_set_sink_filter(bool isOn)
{
    g_bt_debug_sink_filter = isOn;
}

bool bt_debug_get_sink_filter(void)
{
    return g_bt_debug_sink_filter;
}

/* The purpose of this function is to filter logs in HB. */
bool bt_debug_check_sink_filter(const char **pMsg)
{

    uint32_t msgAddr = (uint32_t) * pMsg;

    if (bt_debug_get_sink_filter() == false)
        return false;

    if (msgAddr == (uint32_t)&BTHCI_016
        || msgAddr == (uint32_t)&BTHCI_037
        || msgAddr == (uint32_t)&BTHCI_057
        //GAP
        || msgAddr == (uint32_t)&BTGAP_045
        || msgAddr == (uint32_t)&BTGAP_065
        //L2CAP
        || msgAddr == (uint32_t)&BTL2CAP_241
        || msgAddr == (uint32_t)&BTL2CAP_017
        || msgAddr == (uint32_t)&BTTIMER_001 //ex: reduce bt_gap_connection_sniff_timeout
        //L2CAP assembly logs
        || msgAddr == (uint32_t)&BTHCI_038
        || msgAddr == (uint32_t)&BTHCI_046
        || msgAddr == (uint32_t)&BTHCI_049
        || msgAddr == (uint32_t)&BTHCI_092
#ifdef AIR_LE_AUDIO_ENABLE
        //GATT
        || msgAddr == (uint32_t)&BTATT_010
        || msgAddr == (uint32_t)&BTHCI_012
        || msgAddr == (uint32_t)&BTHCI_024
        || msgAddr == (uint32_t)&BTGATT_056
        || msgAddr == (uint32_t)&BTGATT_057
        || msgAddr == (uint32_t)&BTHCI_065
        || msgAddr == (uint32_t)&BTHCI_059
        || msgAddr == (uint32_t)&BTGAP_107
        || msgAddr == (uint32_t)&BTGAP_242
        || msgAddr == (uint32_t)&BTGAP_274
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */
       )
        return true;

    return false;
}

#endif /* #ifdef __MTK_BT_SINK_SUPPORT__ */

#endif /* #ifdef __BT_DEBUG__ */

