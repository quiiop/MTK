/* Copyright Statement:
 *
 * (C) 2019  MediaTek Inc. All rights reserved.
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

#include "bt_le_audio_msglog.h"
#include "bt_le_audio_log.h"

#include "syslog.h"
#include <stdio.h>

#define LEA_LOG_BUFF_SIZE  208
bt_lea_log_level_t g_default_lea_log_lv = BTLEA_LOG_LV_INFO;


log_create_module(LE_AUDIO, PRINT_LEVEL_INFO);
/**************************************************************************************************
* Functions
**************************************************************************************************/
void bt_lea_log(bt_lea_log_level_t level, const char *msg, ...)
{
    int cnt = 0;
    va_list arg;
    char log_buff[LEA_LOG_BUFF_SIZE] = {0};

    if (level > g_default_lea_log_lv)
        return;
    //memset(log_buff, 0, sizeof(log_buff));

    va_start(arg, msg);
    cnt = vsnprintf(log_buff, LEA_LOG_BUFF_SIZE, msg, arg);
    if (cnt < 0 || cnt > LEA_LOG_BUFF_SIZE) {
        LOG_E(BT, "vsnprintf error, count out of bound (cnt = %d)", cnt);
        va_end(arg);
        return;
    }
    va_end(arg);
    log_buff[LEA_LOG_BUFF_SIZE - 1] = '\0';
    LOG_I(LE_AUDIO, "%s", log_buff);
}

void le_audio_log_i(const char *msg, uint32_t arg_cnt, ...)
{
#ifdef MTK_DEBUG_LEVEL_INFO
    va_list ap;
    va_start(ap, arg_cnt);
    //log_print_msgid(&log_control_block_LE_AUDIO, PRINT_LEVEL_INFO, msg, arg_cnt, ap);
    va_end(ap);
#endif
}

void le_audio_log_w(const char *msg, uint32_t arg_cnt, ...)
{
#ifdef MTK_DEBUG_LEVEL_WARNING
    va_list ap;
    va_start(ap, arg_cnt);
    //log_print_msgid(&log_control_block_LE_AUDIO, PRINT_LEVEL_WARNING, msg, arg_cnt, ap);
    va_end(ap);
#endif
}

void le_audio_log_e(const char *msg, uint32_t arg_cnt, ...)
{
#ifdef MTK_DEBUG_LEVEL_ERROR
    va_list ap;
    va_start(ap, arg_cnt);
    //log_print_msgid(&log_control_block_LE_AUDIO, PRINT_LEVEL_ERROR, msg, arg_cnt, ap);
    va_end(ap);
#endif
}

void le_audio_log_d(const char *msg, uint32_t arg_cnt, ...)
{
#ifdef MTK_DEBUG_LEVEL_DEBUG
    va_list ap;
    va_start(ap, arg_cnt);
    //log_print_msgid(&log_control_block_LE_AUDIO, PRINT_LEVEL_DEBUG, msg, arg_cnt, ap);
    va_end(ap);
#endif
}

/**************************************************************************************************
* Variable
**************************************************************************************************/
#define ATTR_LOG_STRING_LIB const char
#define LOG_INFO_PREFIX(filter)
#define LOG_WARNING_PREFIX(filter)

ATTR_LOG_STRING_LIB BT_LE_AUDIO_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[SINK] send action, handle:%04X action:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[SOURCE] send action, action:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_002[] = LOG_INFO_PREFIX(LE_AUDIO) "[LINK] set ADV handle (%x)";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_003[] = LOG_INFO_PREFIX(LE_AUDIO) "[LINK] LE Link, handle:%04X adv_handle:%x (%x)";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_004[] = LOG_INFO_PREFIX(LE_AUDIO) "[LINK] LE AUDIO connected, handle:%04X idx:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_005[] = LOG_INFO_PREFIX(LE_AUDIO) "[LINK] LE AUDIO disconnected, handle:%04X idx:%x";
/* CCP */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] init, malloc fail";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] set_attribute, handle:%04X gtbs:%x complete:%x charc_num:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_002[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] set_service, malloc fail";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_003[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] config TBS, service:%x state:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_004[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] config TBS, fail, service:%x state:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_005[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] discover TBS continue, gtbs:%x tbs:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_006[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] discover TBS complete, gtbs:%x tbs_num:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_007[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP][%x] invalid service status, status:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_008[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP][%x] invalid attHdl, service:%x uuid:%x cccd:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_009[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP][%x] invalid command, service:%x uuid:%x cccd:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_00A[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP][%x] gatt reqest fail, status:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_00B[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP][%x] malloc fail";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_00C[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] err rsp, status:%lx handle:%x errOp:%x errHdl:%x evtID:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_00D[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] noti, invalid attHdl:%x service:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_00E[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] noti, invalid charc:%x service:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CCP_00F[] = LOG_INFO_PREFIX(LE_AUDIO) "[CCP] noti, service:%x evtID:%x charc:%x attHdl:%x";

/* TBS */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_TBS_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[TBS][%x] call idx: %d, call state %d ---> %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_TBS_001[] = LOG_WARNING_PREFIX(LE_AUDIO) "[TBS][%x] call idx: %d not found";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_TBS_002[] = LOG_WARNING_PREFIX(LE_AUDIO) "[TBS][%x] call idx: %d, call state %d -X-> %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_TBS_003[] = LOG_INFO_PREFIX(LE_AUDIO) "[TBS][%x] call idx: %d, Opcode %04x ";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_TBS_004[] = LOG_INFO_PREFIX(LE_AUDIO) "[TBS]ble_tbs_switch_device_completed";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_TBS_005[] = LOG_INFO_PREFIX(LE_AUDIO) "[TBS]Handle[%x] is inactive device";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_TBS_006[] = LOG_INFO_PREFIX(LE_AUDIO) "[TBS] set_call_state, update: call_idx:0x%x, call_state:0x%x";

/* MCP */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] set_attribute, handle:%04X gmcs:%x complete:%x charc_num:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] set_attribute, malloc fail";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_002[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] config MCS, service:%x state:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_003[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] discover MCS complete, gmcs:%x mcs_num:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_004[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] discover MCS continue, gmcs:%x mcs:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_005[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] config MCS result, state:%x result:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_006[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP][%x] invalid attHdl, service:%x uuid:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_007[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP][%x] gatt reqest fail, status:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_008[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP][%x] malloc fail";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_009[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] noti, invalid charc:%x service:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_00A[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] noti, service:%x evtID:%x charc:%x attHdl:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_00B[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] set supported opcodes, handle:%x service:%x op:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_00C[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] get supported opcodes, handle:%x service:%x op:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_00D[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] set media state, handle:%x service:%x state:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_00E[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] get media state, handle:%x service:%x state:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_100[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] BT_OTP_DISCOVER_SERVICE_COMPLETE_NOTIFY:status[0x%x] ots[%d]";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_101[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] ble_mcp_is_object_action service_idx:%d state:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MCP_102[] = LOG_INFO_PREFIX(LE_AUDIO) "[MCP] ble_mcp_read_object_state_machine state:%d event:0x%x";

/* VCP */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCP_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCP] control_action:%04X";
/* VCS */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] gatt request, link not exist (handle:%x)";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_002[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] gatt request, handle:%x req:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_003[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] control point, invalid opcode:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_004[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] control point, invalid change counter:%x curr_change_counter:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_005[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] control point, handle:%x opcode:%x size:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_006[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] change_counter:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_007[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] send volume flags noti, handle:%x flags:%x ";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_008[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] send write rsp, invalid ATT handle, uuid_type:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_009[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] send noti, link not exist (handle:%x), uuid_type:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_00A[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] send noti, cccd not enable, uuid_type:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_00B[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] send noti, invalid ATT handle, uuid_type:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_00C[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] send volume state noti, handle:%x volume:%x mute:%x change_counter:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_00D[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] send read rsp, link not exist (handle:%x), uuid_type:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_00E[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] send read rsp, invalid ATT handle, uuid_type:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_00F[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] send read volume state rsp, handle:%x result:%x volume:%x mute:%x change_counter:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_VCS_010[] = LOG_INFO_PREFIX(LE_AUDIO) "[VCS] send read volume flags rsp, handle:%x result:%x flags:%x";

/* ASCS */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] check qos config para valid, error_reason:%d, ASE ID:%d, sdu_interval:0x%x, framing:0x%x, phy:0x%x, maximum_sdu_size:0x%x, transport_latency:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] check codec config para valid, error_reason:%d, ASE ID:%d, codec_id[0]:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_002[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] send charc value NOTIFICATION, conn handle:0x%x, att handle:0x%x, status:0x%x, data len (sent/total):(0x%x/0x%x)";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_003[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] handle ASE control point, get control point param FAIL";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_004[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] handle ASE control point, truncated operation";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_005[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] handle ASE control point, invalid ASE ID:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_006[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] handle ASE control point, invalid ASE State Machine Transition, state:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_007[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] handle ASE control point, length error";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_008[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] handle ASE control point, ASE unavailable, sink ASE:%d, source ASE:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_009[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] handle ASE control point, ASE num:%d, failed ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_00A[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ASE control point IND: Config Codec, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_00B[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ASE control point IND: Config QoS, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_00C[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] send ASE control point NOTIFICATION, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_00D[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] check qos config para supported, ASE ID:0x%x, Rx: sdu_interval:0x%x, framing:0x%x, phy:0x%x, max_sdu:0x%x, retrans_num:0x%x, max_transport_latency:0x%x, presentation_delay:0x%x, p_allocation:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_00E[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] check qos config para supported, error_reason:0x%x, Supported: sdu_interval:[0x%x,0x%x], phy:0x%x, max_sdu:0x%x, retrans_num:0x%x, max_transport_latency:0x%x, presentation_delay:[0x%x,0x%x]";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_00F[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] check codec config para supported, error_reason:0x%x, ASE ID:%d, codec_id[0]:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_010[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] modify ASE control point error status, handle:0x%x, p_ctrl:0x%x, ASE ID:%d, rsp_code:0x%x, reason:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_011[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] send ASE state NOTIFICATION FAILED %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_012[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] send ASE state NOTIFICATION, ASE ID:%d, ASE state:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_013[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] push ASE ID FAILED %d, ASE ID:%d, p_queue:0x%x, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_014[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ASE control point IND: Enable, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_015[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ASE control point IND: Reciver Start Ready, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_016[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ASE control point IND: Disable, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_017[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ASE control point IND: Receiver Stop Ready, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_018[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ASE control point IND: Update Meta Data, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_019[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ASE control point IND: Release, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_01A[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ASE control point IND: Unknow:0x%x, ASE num:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_01B[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] check metadata para valid, error_reason:0x%x, ASE ID:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_01C[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ASE ID:%d is processing";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_01D[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] Invalid direction, ASE ID:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_01E[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] Handle %s, ASE ID:%d, direction:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_01F[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] check qos config para supported, error: max SDU, frame:0x%x, audio location:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_020[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] check qos config para supported, error: PHY, phy:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_021[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] %s response, handle:0x%x, ase id:%d, rsp_code:0x%x, reason:0x%x, p_ase_info:0x%x, ase_char_idx:0x%x, temp_req:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_022[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] check all control point ASEs processed, p_ctrl:0x%x, control point ASE num: queue(%d), fail(%d), table(%d)";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_023[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ascs cpt callback, rw:%d, handle:0x%x, data:0x%x, size:%d, offset:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_024[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] ble_ascs_gatt_request_handler, req:%d, handle:0x%x, charc_idx:%d, data:0x%x, size:%d, offset:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_025[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] Null ase_info for setting CCCD";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_026[] = LOG_INFO_PREFIX(LE_AUDIO) "[ASCS] Set connection success, handle:0x%x";

//ASE Control Point Opcode from SPEC
ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_ASE_OPCODE_STRING[][30] = {
    "None",
    "Config Codec",
    "Config QoS",
    "Enable",
    "Receiver Start Ready",
    "Disable",
    "Receiver Stop Ready",
    "Update Metadata",
    "Release",
};

ATTR_LOG_STRING_LIB BT_LE_AUDIO_ASCS_ASE_OPCODE_STRING2[][30] = {
    "Receiver Start / Stop Ready",
};

/* PACS */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_PACS_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[PACS] send charc value NOTIFICATION, conn handle:0x%x, att handle:0x%x, status:0x%x, data len (sent/total):(0x%x/0x%x)";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_PACS_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[PACS] set audio location, direction:%d, location:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_PACS_002[] = LOG_INFO_PREFIX(LE_AUDIO) "[PACS] get audio location, direction:%d, location:0x%x";

/* BAP */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BAP_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[BAP] Get ASE state, handle:%04X, ase_id:%d, state:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BAP_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[BAP] Set ASE state, handle:%04X, ase_id:%d, cur_state:%d, next_state:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BAP_002[] = LOG_INFO_PREFIX(LE_AUDIO) "[BAP] Set CIS info SUCCESS, handle:%04X, cig_id:%x, cis_id:%x, ase_id:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BAP_003[] = LOG_INFO_PREFIX(LE_AUDIO) "[BAP] Set CIS handle, LE_handle:%04X, ASE_ID:%d, CIS_handle:%04X, cig_id:%x, cis_id:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BAP_004[] = LOG_INFO_PREFIX(LE_AUDIO) "[BAP] Clear CIS info, LE_handle:%04X, ASE_ID:%d, cig_id:%x, cis_id:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BAP_005[] = LOG_INFO_PREFIX(LE_AUDIO) "[BAP] BASS CPT callback, handle:%04X, event:%d, source_id:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BAP_006[] = LOG_INFO_PREFIX(LE_AUDIO) "[BAP] check broadcast state mask, curret:%X, check:%X";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BAP_007[] = LOG_INFO_PREFIX(LE_AUDIO) "[BAP] Broadcast code: %x %x %x %x %x %x %x %x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BAP_008[] = LOG_INFO_PREFIX(LE_AUDIO) "[BAP] Clear CIS handle, LE_handle:%04X, CIS_handle:%04X, ase_id:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BAP_009[] = LOG_INFO_PREFIX(LE_AUDIO) "[BAP] BIG sync lost, BIG handle:0x%x, reason:0x%x";

/* BASS */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BASS_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[BASS] Invalid subgroup num, current:%d, request:%d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_BASS_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[BASS] Invalid operation";

/* CAP */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CAP_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[CAP][MCP] action, handle:%04X action:%X service_idx:%x state:%X sub_state:%X";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CAP_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[CAP][MCP] set state, handle:%04X service_idx:%x state:%X";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CAP_002[] = LOG_INFO_PREFIX(LE_AUDIO) "[CAP][MCP] event callback:%X";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CAP_003[] = LOG_INFO_PREFIX(LE_AUDIO) "[CAP][MCP] control action, fail, result:%x evt:%x hdlrIdx:%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CAP_004[] = LOG_INFO_PREFIX(LE_AUDIO) "[CAP][CCP] save pre noti, handle:0x%x, evt:0x%x len:%d";

/* CSIS */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CSIS_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[CSIS] read SIRK, fail, no bonding info";

/* CSIP */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CSIP_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[CSIP] set service attribute, property:0x%x, value handle:0x%x, UUID:0x%x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CSIP_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[CSIP] read SIRK cnf, encrypted SIRK:0x%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_CSIP_002[] = LOG_INFO_PREFIX(LE_AUDIO) "[CSIP] read SIRK cnf, plain text SIRK:0x%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x";

/* HAPS */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_000[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] update_change_item success bond_id = %d add = %d change_id = %d idx = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] update_read_preset_send_data get_next_preset = %x read num = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_002[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] check_send_data status = %x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_003[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] ctrl_pt_send_data status";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_004[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] ctrl_pt_send_data free curr send";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_005[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] ctrl_pt_send_data wait = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_006[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] notify_preset_read type = %d last = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_007[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] notify_preset_generic_update idx = %d id = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_008[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] notify_preset_changed idx = %d id = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_009[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] notify_change_item idx = %d id = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_00A[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] ctrl_point_write conn_hdl = %x op = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_00B[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] ctrl_point_write err = %x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_00C[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] set_connection conn_hdl = %x idx = %d, peer_addr_ptr = %x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_00D[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] reset_connection conn_hdl = %x idx = %d lock = %x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_00E[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] gatt_request_handler conn idx = %d req = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_00F[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] VALUE_CONFIRMATION conn_hdl = %x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_010[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] init success feature = %x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_011[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] add_new_preset success idx = %d property = %x";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_012[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] remove_preset success idx = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_013[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] change_preset success idx = %d flag = %x prop = %x len = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_014[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] set_active_preset success idx = %d";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_015[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] change_feature = %x\n";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_016[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] peer idx = %d write_preset_name [%s]";
ATTR_LOG_STRING_LIB BT_LE_AUDIO_HAPS_017[] = LOG_INFO_PREFIX(LE_AUDIO) "[HAPS] adv_hdl = %x conn_hdl = %x";

/* MICP */
ATTR_LOG_STRING_LIB BT_LE_AUDIO_MICP_001[] = LOG_INFO_PREFIX(LE_AUDIO) "[MICP] discovery_state:%x current_event:%x";

