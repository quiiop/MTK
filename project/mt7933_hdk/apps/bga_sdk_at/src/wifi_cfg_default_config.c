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
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nvdm.h"
#include "nvdm_ctrl.h"

static const group_data_item_t g_wifi_cfg_item_array[] = {
    NVDM_DATA_ITEM("Qos",               "1"),
    NVDM_DATA_ITEM("StaHT",             "1"),
    NVDM_DATA_ITEM("StaVHT",            "1"),
    NVDM_DATA_ITEM("StaHE",             "1"),
    NVDM_DATA_ITEM("ApHT",              "1"),
    NVDM_DATA_ITEM("ApVHT",             "1"),
    NVDM_DATA_ITEM("ApHE",              "0"),
    NVDM_DATA_ITEM("P2pGoHT",           "1"),
    NVDM_DATA_ITEM("P2pGoVHT",          "1"),
    NVDM_DATA_ITEM("P2pGoHE",           "0"),
    NVDM_DATA_ITEM("P2pGcHT",           "1"),
    NVDM_DATA_ITEM("P2pGcVHT",          "1"),
    NVDM_DATA_ITEM("P2pGcHE",           "0"),
    NVDM_DATA_ITEM("ApChannel",         "0"),
    NVDM_DATA_ITEM("ApChnlDefFromCfg",  "1"),
    NVDM_DATA_ITEM("ApSco",             "0"),
    NVDM_DATA_ITEM("P2pGoSco",          "0"),
    NVDM_DATA_ITEM("StaBw",             "0"),
    NVDM_DATA_ITEM("Sta2gBw",           "0"),
    NVDM_DATA_ITEM("Sta5gBw",           "0"),
    NVDM_DATA_ITEM("P2p2gBw",           "0"),
    NVDM_DATA_ITEM("P2p5gBw",           "0"),
    NVDM_DATA_ITEM("ApBw",              "0"),
    NVDM_DATA_ITEM("Ap2gBw",            "0"),
    NVDM_DATA_ITEM("Ap5gBw",            "0"),
    NVDM_DATA_ITEM("NSS",               "1"),
    NVDM_DATA_ITEM("Ap5gNSS",           "1"),
    NVDM_DATA_ITEM("Ap2gNSS",           "1"),
    NVDM_DATA_ITEM("Go5gNSS",           "1"),
    NVDM_DATA_ITEM("Go2gNSS",           "1"),
    NVDM_DATA_ITEM("TrigMacPadDur",     "2"),
    NVDM_DATA_ITEM("TWTRequester",      "1"),
    NVDM_DATA_ITEM("TWTResponder",      "0"),
    NVDM_DATA_ITEM("TWTStaBandBitmap",  "3"),
    NVDM_DATA_ITEM("HeOMCtrl",          "1"),
    NVDM_DATA_ITEM("SREnable",          "0"),
    NVDM_DATA_ITEM("FrdHeTrig2Host",    "0"),
    NVDM_DATA_ITEM("ExtendedRange",     "1"),
    NVDM_DATA_ITEM("StaHTBfee",         "0"),
    NVDM_DATA_ITEM("StaVHTBfee",        "1"),
    NVDM_DATA_ITEM("StaVHTMuBfee",      "1"),
    NVDM_DATA_ITEM("StaHEBfee",         "1"),
    NVDM_DATA_ITEM("StaHTBfer",         "0"),
    NVDM_DATA_ITEM("StaVHTBfer",        "0"),
    NVDM_DATA_ITEM("DataTxDone",        "0"),
    NVDM_DATA_ITEM("DhcpTxDone",        "1"),
    NVDM_DATA_ITEM("ArpTxDone",         "1"),
    NVDM_DATA_ITEM("DataTxRateMode",    "0"),
    NVDM_DATA_ITEM("DataTxRateCode",    "0"),
    NVDM_DATA_ITEM("MtkOui",            "1"),
    NVDM_DATA_ITEM("Probe256QAM",       "1"),
    NVDM_DATA_ITEM("VhtIeIn2G",         "1"),
    NVDM_DATA_ITEM("NetifStopTh",       "256"),
    NVDM_DATA_ITEM("NetifStartTh",      "256"),
    NVDM_DATA_ITEM("TxBaSize",          "16"),
    NVDM_DATA_ITEM("RxHtBaSize",        "8"),
    NVDM_DATA_ITEM("RxVhtBaSize",       "8"),
    NVDM_DATA_ITEM("RxHeBaSize",        "8"),
    NVDM_DATA_ITEM("TxHeBaSize",        "16"),
    NVDM_DATA_ITEM("CtiaMode",          "0"),
    NVDM_DATA_ITEM("TpTestMode",        "0"),
    NVDM_DATA_ITEM("DbdcMode",          "0"),
    NVDM_DATA_ITEM("EfuseBufferModeCal",    "2"),
    NVDM_DATA_ITEM("Wow",               "1"),
    NVDM_DATA_ITEM("ScreenoffDTIMPeriod",   "1"),
    NVDM_DATA_ITEM("AdvPws",            "1"),
    NVDM_DATA_ITEM("WowOnMdtim",        "1"),
    NVDM_DATA_ITEM("WowOffMdtim",       "1"),
    NVDM_DATA_ITEM("PmStaPmPolicy",     "0"),
    NVDM_DATA_ITEM("RtsPktLen",         "8192"),
    NVDM_DATA_ITEM("EapolOffload",      "0"),
    NVDM_DATA_ITEM("DisOnlineScan",     "0"),
    NVDM_DATA_ITEM("DisBcnLostDetection",   "0"),
    NVDM_DATA_ITEM("ScreenOnBeaconTimeoutCount", "10"),
    NVDM_DATA_ITEM("BcnTimeoutCountBT", "10"),
    NVDM_DATA_ITEM("DisRoaming",        "0"),
    NVDM_DATA_ITEM("RoamingByRSSI",     "0"),
    NVDM_DATA_ITEM("RoamingRSSIValue",  "0"),
    NVDM_DATA_ITEM("RoamingByBcn",      "0"),
    NVDM_DATA_ITEM("RoamingBeaconTimeSec", "5"),
    NVDM_DATA_ITEM("RoamingRetryLimit", "0"),
    NVDM_DATA_ITEM("RoamingBlockTimeSec",  "30"),
    NVDM_DATA_ITEM("RoamingRetryTimeSec",  "2"),
    NVDM_DATA_ITEM("RoamingStableTimeSec", "2"),
    NVDM_DATA_ITEM("RoamingTXErrorEnable", "0"),
    NVDM_DATA_ITEM("TdlsBufferSTASleep",    "0"),
    NVDM_DATA_ITEM("ChipResetRecover",  "0"),
    NVDM_DATA_ITEM("ForceSTSNum",       "0"),
    NVDM_DATA_ITEM("SerEnable",         "1"),
    NVDM_DATA_ITEM("RegP2pIfAtProbe",   "0"),
    NVDM_DATA_ITEM("ApAllowHtVhtTkip",  "0"),
    NVDM_DATA_ITEM("EnableDefaultFastPSP", "1"),
    NVDM_DATA_ITEM("EnableFastConnect",         "0"),
    NVDM_DATA_ITEM("Disable11K",        "0"),
    NVDM_DATA_ITEM("Disable11V",        "0"),
    NVDM_DATA_ITEM("AntDivMode",        "2"),
    NVDM_DATA_ITEM("ChannelDwellTime",  "0"),
    NVDM_DATA_ITEM("ScnChannelDFSTime", "0"),
};

static void wifi_check_default_value(void)
{
    check_default_value("wifi",
                        g_wifi_cfg_item_array,
                        sizeof(g_wifi_cfg_item_array) / sizeof(g_wifi_cfg_item_array[0]));
}

static void wifi_reset_to_default(void)
{
    reset_to_default("wifi",
                     g_wifi_cfg_item_array,
                     sizeof(g_wifi_cfg_item_array) / sizeof(g_wifi_cfg_item_array[0]));
}

static void wifi_show_value(void)
{
    show_group_value("wifi",
                     g_wifi_cfg_item_array,
                     sizeof(g_wifi_cfg_item_array) / sizeof(g_wifi_cfg_item_array[0]));
}

const user_data_item_operate_t wifi_cfg_item_operate_array[] = {
    {
        "wifi",
        wifi_check_default_value,
        wifi_reset_to_default,
        wifi_show_value,
    },
};


