/*******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
/*
 ** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux
 *      /gl_wext_priv.c#8
 */

/*! \file gl_wext_priv.c
 *    \brief This file includes private ioctl support.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "gl_os.h"
#include "gl_wifi_cli.h"
#include "mt7933_pos.h"
#include "hal_nvic.h"
#include "gl_init.h"
#include "wifi_netif.h"
#include "hal_spm.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
#include "lwip/stats.h"
#include "lwip/netif.h"
#include "wifi_api_ex.h"
#include "get_profile_string.h"
#include "misc.h"

#ifdef MTK_MINISUPP_ENABLE
#include "utils/os.h"
#endif /* #ifdef MTK_MINISUPP_ENABLE */

#ifdef MTK_NVDM_ENABLE
#include "nvdm.h"
#endif /* #ifdef MTK_NVDM_ENABLE */

#if CFG_SUPPORT_QA_TOOL
#include "gl_ate_agent.h"
#include "gl_qa_agent.h"
#endif /* #if CFG_SUPPORT_QA_TOOL */

#if CFG_ENABLE_WIFI_DIRECT
#include "gl_p2p_os.h"
#endif /* #if CFG_ENABLE_WIFI_DIRECT */

#ifdef MTK_SIGMA_ENABLE
#include "wfa_main.h"
#endif /* #ifdef MTK_SIGMA_ENABLE */

/*
 * #if CFG_SUPPORT_QA_TOOL
 * extern UINT_16 g_u2DumpIndex;
 * #endif
 */

#if (CFG_SUPPORT_QA_TOOL != 1) && (CONFIG_WLAN_SERVICE == 1)
//extern uint8_t aucOidBuf[CMD_OID_BUF_LENGTH];
#endif /* #if (CFG_SUPPORT_QA_TOOL != 1) && (CONFIG_WLAN_SERVICE == 1) */
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define WIFI_CLI_RETURN_STRING(ret) ((ret >= 0) ? "Success" : "Error")
#define WIFI_CLI_CALC_PER_BEFORE_DOT(x, y) ((y > 0) ? (100 * (x) / (y)) : 0)
#define WIFI_CLI_CALC_PER_AFTER_DOT(x, y)  ((y > 0) ? \
    ((100 * (x) % (y)) * 100 / (y)) : 0)

#if (CFG_SUPPORT_TWT == 1)
#define CMD_TWT_ACTION_TEN_PARAMS        10
#define CMD_TWT_ACTION_THREE_PARAMS      3
#define CMD_TWT_MAX_PARAMS CMD_TWT_ACTION_TEN_PARAMS
#endif /* #if (CFG_SUPPORT_TWT == 1) */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
uint8_t wifi_connect_send_raw_ex(uint8_t len, char *param[]);

/*******************************************************************************
 *                       P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#define CMD_START       "START"
#define CMD_STOP        "STOP"
#define CMD_SCAN_ACTIVE     "SCAN-ACTIVE"
#define CMD_SCAN_PASSIVE    "SCAN-PASSIVE"
#define CMD_RSSI        "RSSI"
#define CMD_LINKSPEED       "LINKSPEED"
#define CMD_RXFILTER_START  "RXFILTER-START"
#define CMD_RXFILTER_STOP   "RXFILTER-STOP"
#define CMD_RXFILTER_ADD    "RXFILTER-ADD"
#define CMD_RXFILTER_REMOVE "RXFILTER-REMOVE"
#define CMD_BTCOEXSCAN_START    "BTCOEXSCAN-START"
#define CMD_BTCOEXSCAN_STOP "BTCOEXSCAN-STOP"
#define CMD_BTCOEXMODE      "BTCOEXMODE"
#define CMD_SETSUSPENDOPT   "SETSUSPENDOPT"
#define CMD_SETSUSPENDMODE  "SETSUSPENDMODE"
#define CMD_P2P_DEV_ADDR    "P2P_DEV_ADDR"
#define CMD_SETFWPATH       "SETFWPATH"
#define CMD_SETBAND     "SETBAND"
#define CMD_GETBAND     "GETBAND"
#define CMD_AP_START        "AP_START"

#if CFG_SUPPORT_QA_TOOL
#define CMD_GET_RX_STATISTICS   "GET_RX_STATISTICS"
#endif /* #if CFG_SUPPORT_QA_TOOL */
#define CMD_GET_STAT        "GET_STAT"
#define CMD_GET_BSS_STATISTICS  "GET_BSS_STATISTICS"
#define CMD_GET_STA_STATISTICS  "GET_STA_STATISTICS"
#define CMD_GET_WTBL_INFO   "GET_WTBL"
#define CMD_GET_MIB_INFO    "GET_MIB"
#define CMD_GET_STA_INFO    "GET_STA"
#define CMD_SET_FW_LOG      "SET_FWLOG"
#define CMD_GET_QUE_INFO    "GET_QUE"
#define CMD_GET_MEM_INFO    "GET_MEM"
#define CMD_GET_HIF_INFO    "GET_HIF"
#define CMD_GET_TP_INFO     "GET_TP"
#define CMD_GET_STA_KEEP_CNT    "KEEPCOUNTER"
#define CMD_STAT_RESET_CNT      "RESETCOUNTER"
#define CMD_STAT_NOISE_SEL      "NOISESELECT"
#define CMD_STAT_GROUP_SEL      "GROUP"

#define CMD_SET_TXPOWER         "SET_TXPOWER"
#define CMD_COUNTRY         "COUNTRY"
#define CMD_CSA             "CSA"
#define CMD_GET_COUNTRY         "GET_COUNTRY"
#define CMD_GET_CHANNELS        "GET_CHANNELS"
#define CMD_P2P_SET_NOA         "P2P_SET_NOA"
#define CMD_P2P_GET_NOA         "P2P_GET_NOA"
#define CMD_P2P_SET_PS          "P2P_SET_PS"
#define CMD_SET_AP_WPS_P2P_IE       "SET_AP_WPS_P2P_IE"
#define CMD_SETROAMMODE         "SETROAMMODE"
#define CMD_MIRACAST            "MIRACAST"

#if (CFG_SUPPORT_DFS_MASTER == 1)
#define CMD_SHOW_DFS_STATE      "SHOW_DFS_STATE"
#define CMD_SHOW_DFS_RADAR_PARAM    "SHOW_DFS_RADAR_PARAM"
#define CMD_SHOW_DFS_HELP       "SHOW_DFS_HELP"
#define CMD_SHOW_DFS_CAC_TIME       "SHOW_DFS_CAC_TIME"
#endif /* #if (CFG_SUPPORT_DFS_MASTER == 1) */

#define CMD_PNOSSIDCLR_SET  "PNOSSIDCLR"
#define CMD_PNOSETUP_SET    "PNOSETUP "
#define CMD_PNOENABLE_SET   "PNOFORCE"
#define CMD_PNODEBUG_SET    "PNODEBUG"
#define CMD_WLS_BATCHING    "WLS_BATCHING"

#define CMD_OKC_SET_PMK     "SET_PMK"
#define CMD_OKC_ENABLE      "OKC_ENABLE"

#define CMD_SETMONITOR      "MONITOR"
#define CMD_SETBUFMODE      "BUFFER_MODE"

#define CMD_GET_CH_RANK_LIST    "GET_CH_RANK_LIST"
#define CMD_GET_CH_DIRTINESS    "GET_CH_DIRTINESS"

#if CFG_CHIP_RESET_HANG
#define CMD_SET_RST_HANG                 "RST_HANG_SET"

#define CMD_SET_RST_HANG_ARG_NUM        2
#endif /* #if CFG_CHIP_RESET_HANG */


#define CMD_EFUSE       "EFUSE"

#if (CFG_SUPPORT_TWT == 1)
#define CMD_SET_TWT_PARAMS  "SET_TWT_PARAMS"
#endif /* #if (CFG_SUPPORT_TWT == 1) */

#define CMD_CCCR        "CCCR"

/* miracast related definition */
#define MIRACAST_MODE_OFF   0
#define MIRACAST_MODE_SOURCE    1
#define MIRACAST_MODE_SINK  2

#ifndef MIRACAST_AMPDU_SIZE
#define MIRACAST_AMPDU_SIZE 8
#endif /* #ifndef MIRACAST_AMPDU_SIZE */

#ifndef MIRACAST_MCHAN_ALGO
#define MIRACAST_MCHAN_ALGO     1
#endif /* #ifndef MIRACAST_MCHAN_ALGO */

#ifndef MIRACAST_MCHAN_BW
#define MIRACAST_MCHAN_BW       25
#endif /* #ifndef MIRACAST_MCHAN_BW */

#define CMD_BAND_TYPE_AUTO  0
#define CMD_BAND_TYPE_5G    1
#define CMD_BAND_TYPE_2G    2
#define CMD_BAND_TYPE_ALL   3

/* Mediatek private command */
#define CMD_SET_MCR     "SET_MCR"
#define CMD_GET_MCR     "GET_MCR"
#define CMD_SCAN        "SCAN"
#define CMD_GET_SCAN_RES    "GET_SCAN_RESULT"
#define CMD_CONNECT         "CONNECT"
#define CMD_LWIP_LINK_UP    "LWIP_LINK_UP"
#define CMD_LWIP_LINK_DOWN    "LWIP_LINK_DOWN"
#define CMD_SET_DRV_MCR     "SET_DRV_MCR"
#define CMD_GET_DRV_MCR     "GET_DRV_MCR"
#define CMD_SET_UHW_MCR     "SET_UHW_MCR"
#define CMD_GET_UHW_MCR     "GET_UHW_MCR"
#define CMD_SET_SW_CTRL         "SET_SW_CTRL"
#define CMD_GET_SW_CTRL         "GET_SW_CTRL"
#define CMD_SET_CFG             "SET_CFG"
#define CMD_GET_CFG             "GET_CFG"
#define CMD_SET_CHIP            "SET_CHIP"
#define CMD_GET_CHIP            "GET_CHIP"
#define CMD_SET_DBG_LEVEL       "SET_DBG_LEVEL"
#define CMD_GET_DBG_LEVEL       "GET_DBG_LEVEL"
#define CMD_ADD_TS      "addts"
#define CMD_DEL_TS      "delts"
#define CMD_DUMP_TS     "dumpts"
#define CMD_RM_IT       "RM-IT"
#define CMD_DUMP_UAPSD      "dumpuapsd"
#define CMD_FW_EVENT        "FW-EVENT "
#define CMD_GET_WIFI_TYPE   "GET_WIFI_TYPE"
#define CMD_SET_PWR_CTRL        "SET_PWR_CTRL"
#define PRIV_CMD_SIZE 512
#define CMD_SET_FIXED_RATE      "FixedRate"
#define CMD_GET_VERSION         "VER"
#define CMD_SET_TEST_MODE   "SET_TEST_MODE"
#define CMD_SET_TEST_CMD    "SET_TEST_CMD"
#define CMD_GET_TEST_RESULT "GET_TEST_RESULT"
#define CMD_GET_STA_STAT        "STAT"
#define CMD_GET_STA_STAT2       "STAT2"
#define CMD_GET_STA_RX_STAT "RX_STAT"
#define CMD_SET_ACL_POLICY      "SET_ACL_POLICY"
#define CMD_ADD_ACL_ENTRY       "ADD_ACL_ENTRY"
#define CMD_DEL_ACL_ENTRY       "DEL_ACL_ENTRY"
#define CMD_SHOW_ACL_ENTRY      "SHOW_ACL_ENTRY"
#define CMD_CLEAR_ACL_ENTRY     "CLEAR_ACL_ENTRY"
#define CMD_SET_RA_DBG      "RADEBUG"
#define CMD_SET_FIXED_FALLBACK  "FIXEDRATEFALLBACK"
#define CMD_GET_STA_IDX         "GET_STA_IDX"
#define CMD_GET_TX_POWER_INFO   "TxPowerInfo"

/* neptune doens't support "show" entry, use "driver" to handle
 * MU GET request, and MURX_PKTCNT comes from RX_STATS,
 * so this command will reuse RX_STAT's flow
 */
#define CMD_GET_MU_RX_PKTCNT    "hqa_get_murx_pktcnt"
#define CMD_RUN_HQA "hqa"

#if CFG_SUPPORT_CSI
#define CMD_SET_CSI             "SET_CSI"
#endif /* #if CFG_SUPPORT_CSI */

#if CFG_WOW_SUPPORT
#define CMD_WOW_START       "WOW_START"
#define CMD_SET_WOW_ENABLE  "SET_WOW_ENABLE"
#define CMD_SET_WOW_PAR     "SET_WOW_PAR"
#define CMD_SET_WOW_UDP     "SET_WOW_UDP"
#define CMD_SET_WOW_TCP     "SET_WOW_TCP"
#define CMD_GET_WOW_PORT    "GET_WOW_PORT"
#define CMD_GET_WOW_REASON  "GET_WOW_REASON"
#endif /* #if CFG_WOW_SUPPORT */
#define CMD_SET_ADV_PWS     "SET_ADV_PWS"
#define CMD_SET_MDTIM       "SET_MDTIM"

#define CMD_SET_DBDC        "SET_DBDC"

#define CMD_SET_AMPDU_TX        "SET_AMPDU_TX"
#define CMD_SET_AMPDU_RX        "SET_AMPDU_RX"
#define CMD_SET_BF              "SET_BF"
#define CMD_SET_NSS             "SET_NSS"
#define CMD_SET_AMSDU_TX        "SET_AMSDU_TX"
#define CMD_SET_AMSDU_RX        "SET_AMSDU_RX"
#define CMD_SET_QOS             "SET_QOS"
#if (CFG_SUPPORT_802_11AX == 1)
#define CMD_SET_BA_SIZE         "SET_BA_SIZE"
#define CMD_SET_TP_TEST_MODE    "SET_TP_TEST_MODE"
#define CMD_SET_MUEDCA_OVERRIDE "MUEDCA_OVERRIDE"
#define CMD_SET_TX_MCSMAP       "SET_MCS_MAP"
#define CMD_SET_TX_PPDU         "TX_PPDU"
#define CMD_SET_LDPC            "SET_LDPC"
#define CMD_FORCE_AMSDU_TX      "FORCE_AMSDU_TX"
#define CMD_SET_OM_CH_BW        "SET_OM_CHBW"
#define CMD_SET_OM_RX_NSS       "SET_OM_RXNSS"
#define CMD_SET_OM_TX_NSS       "SET_OM_TXNSTS"
#define CMD_SET_OM_MU_DISABLE   "SET_OM_MU_DISABLE"
#define CMD_SET_TX_OM_PACKET    "TX_OM_PACKET"
#define CMD_SET_TX_CCK_1M_PWR   "TX_CCK_1M_PWR"
#define CMD_SET_PAD_DUR         "SET_PAD_DUR"
#endif /* #if (CFG_SUPPORT_802_11AX == 1) */

#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
#define CMD_SET_CALBACKUP_TEST_DRV_FW       "SET_CALBACKUP_TEST_DRV_FW"
#endif /* #if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST */

#define CMD_GET_CNM     "GET_CNM"

#ifdef UT_TEST_MODE
#define CMD_RUN_UT      "UT"
#endif /* #ifdef UT_TEST_MODE */

#if CFG_SUPPORT_ADVANCE_CONTROL
#define CMD_SW_DBGCTL_ADVCTL_SET_ID 0xa1260000
#define CMD_SW_DBGCTL_ADVCTL_GET_ID 0xb1260000
#define CMD_SET_NOISE       "SET_NOISE"
#define CMD_GET_NOISE           "GET_NOISE"
#define CMD_SET_POP     "SET_POP"
#define CMD_SET_ED      "SET_ED"
#define CMD_SET_PD      "SET_PD"
#define CMD_SET_MAX_RFGAIN  "SET_MAX_RFGAIN"
#if CFG_SUPPORT_TRAFFIC_REPORT
#define CMD_TRAFFIC_REPORT  "TRAFFIC_REPORT"
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT */
#endif /* #if CFG_SUPPORT_ADVANCE_CONTROL */

#if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1)
#define CMD_ARP_OFFLOAD  "ARP_OFFLOAD"
#endif /* #if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1) */

#if CFG_ENABLE_WFDMA_DVT
#define CMD_SET_DVT_INIT  "SET_DVT_INIT"
#define CMD_SET_DVT_START "SET_DVT_START"
#define CMD_SET_DVT_STOP  "SET_DVT_STOP"
#define CMD_GET_DVT_INFO  "GET_DVT_INFO"
#endif /* #if CFG_ENABLE_WFDMA_DVT */

#if CFG_SUPPORT_WIFI_SYSDVT
#define CMD_WIFI_SYSDVT         "DVT"
#define CMD_SET_TXS_TEST        "TXS_TEST"
#define CMD_SET_TXS_TEST_RESULT "TXS_RESULT"
#define CMD_SET_RXV_TEST        "RXV_TEST"
#define CMD_SET_RXV_TEST_RESULT        "RXV_RESULT"
#if CFG_TCP_IP_CHKSUM_OFFLOAD
#define CMD_SET_CSO_TEST        "CSO_TEST"
#endif /* #if CFG_TCP_IP_CHKSUM_OFFLOAD */
#define CMD_SET_TX_TEST          "TX_TEST"
#define CMD_SET_TX_AC_TEST       "TX_AC_TEST"
#define CMD_SET_SKIP_CH_CHECK   "SKIP_CH_CHECK"

#if (CFG_SUPPORT_DMASHDL_SYSDVT)
#define CMD_SET_DMASHDL_DUMP    "DMASHDL_DUMP_MEM"
#define CMD_SET_DMASHDL_DVT_ITEM "DMASHDL_DVT_ITEM"
#endif /* #if (CFG_SUPPORT_DMASHDL_SYSDVT) */
#endif /* #if CFG_SUPPORT_WIFI_SYSDVT */

#define CMD_SET_SW_AMSDU_NUM      "SET_SW_AMSDU_NUM"
#define CMD_SET_SW_AMSDU_SIZE      "SET_SW_AMSDU_SIZE"

#define CMD_SET_DRV_SER           "SET_DRV_SER"

#define CMD_GET_PLE_INFO        "GET_PLE_INFO"
#define CMD_GET_PSE_INFO        "GET_PSE_INFO"
#define CMD_GET_WFDMA_INFO      "GET_WFDMA_INFO"
#define CMD_GET_CSR_INFO        "GET_CSR_INFO"
#define CMD_GET_DMASCH_INFO     "GET_DMASCH_INFO"
#define CMD_SHOW_TXD_INFO       "SHOW_TXD_INFO"

#if (CFG_SUPPORT_CONNAC2X == 1)
#define CMD_GET_FWTBL_UMAC      "GET_UMAC_FWTBL"
#endif /* #if (CFG_SUPPORT_CONNAC2X == 1) */

/* Debug for consys */
#define CMD_DBG_SHOW_TR_INFO            "show-tr"
#define CMD_DBG_SHOW_PLE_INFO           "show-ple"
#define CMD_DBG_SHOW_PSE_INFO           "show-pse"
#define CMD_DBG_SHOW_CSR_INFO           "show-csr"
#define CMD_DBG_SHOW_DMASCH_INFO        "show-dmasch"

#if CFG_SUPPORT_EASY_DEBUG
#define CMD_FW_PARAM                "set_fw_param"
#endif /* #if CFG_SUPPORT_EASY_DEBUG */

#if CFG_SUPPORT_802_11K
#define CMD_NEIGHBOR_REQ            "neighbor-request"
#endif /* #if CFG_SUPPORT_802_11K */

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
#define CMD_BTM_QUERY               "bss-transition-query"
#endif /* #if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT */

#if defined(_HIF_AXI) && (CFG_SUPPORT_MANUAL_OWN_CTRL == 1)
#define CMD_SET_FWOWN "SET_FW_OWN"
#define CMD_GET_FWOWN "GET_FW_OWN"
#endif /* #if defined(_HIF_AXI) && (CFG_SUPPORT_MANUAL_OWN_CTRL == 1) */

#if (CFG_WIFI_GET_MCS_INFO == 1)
#define CMD_GET_MCS_INFO        "GET_MCS_INFO"
#endif /* #if (CFG_WIFI_GET_MCS_INFO == 1) */

#if CFG_SUPPORT_WFD
static uint8_t g_ucMiracastMode = MIRACAST_MODE_OFF;
#endif /* #if CFG_SUPPORT_WFD */

#if defined(MTK_RELEASE_MODE) && (MTK_RELEASE_MODE == MTK_M_RELEASE)
#define LOG_USAGE(args...) do { } while (0)
#else /* #if defined(MTK_RELEASE_MODE) && (MTK_RELEASE_MODE == MTK_M_RELEASE) */
#define LOG_USAGE LOG_FUNC
#endif /* #if defined(MTK_RELEASE_MODE) && (MTK_RELEASE_MODE == MTK_M_RELEASE) */

struct cmd_tlv {
    char prefix;
    char version;
    char subver;
    char reserved;
};

struct priv_driver_cmd_s {
    char buf[PRIV_CMD_SIZE];
    int used_len;
    int total_len;
};

int priv_driver_set_dbg_level(struct GLUE_INFO *prGlueInfo,
                              char *pcCommand, int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint32_t u4DbgIdx = 0, u4DbgMask = 0;
    int32_t u4Ret = 0;
    uint32_t u4Idx;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 3) {
        /* u4DbgIdx = kalStrtoul(apcArgv[1], NULL, 0); */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4DbgIdx);
        u4Ret = kalkStrtou32(apcArgv[2], 0, &u4DbgMask);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);
        if (u4DbgIdx == DBG_ALL_MODULE_IDX) {
            for (u4Idx = 0; u4Idx < DBG_MODULE_NUM; u4Idx++)
                wlanSetDriverDbgLevel(u4Idx,
                                      (u4DbgMask & DBG_CLASS_MASK));
        } else {
            wlanSetDriverDbgLevel(u4DbgIdx,
                                  (u4DbgMask & DBG_CLASS_MASK));
        }
    }

    i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                              "Set done\r\n");
    if (i4BytesWritten < 0)
        DBGLOG(REQ, ERROR, "kalSnprintf fail\n");
    return i4BytesWritten;

}               /* priv_driver_get_sw_ctrl */

int priv_driver_get_dbg_level(struct GLUE_INFO *prGlueInfo,
                              char *pcCommand, int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint32_t u4DbgIdx = 0, u4DbgMask = 0;
    u_int8_t fgIsCmdAccept = FALSE;
    int32_t u4Ret = 0;
    uint32_t u4Idx;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 2) {
        /* u4DbgIdx = kalStrtoul(apcArgv[1], NULL, 0); */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4DbgIdx);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);

        if (u4DbgIdx == DBG_ALL_MODULE_IDX) {
            fgIsCmdAccept = TRUE;
            for (u4Idx = 0; u4Idx < DBG_MODULE_NUM; u4Idx++)
                if (wlanGetDriverDbgLevel(u4Idx, &u4DbgMask) ==
                    WLAN_STATUS_SUCCESS)
                    i4BytesWritten +=
                        snprintf(pcCommand + i4BytesWritten,
                                 i4TotalLen - i4BytesWritten,
                                 "Get DBG module[%u] log level => [0x%02x]!\r\n",
                                 u4Idx, (uint8_t) u4DbgMask);
        } else if (wlanGetDriverDbgLevel(u4DbgIdx, &u4DbgMask) ==
                   WLAN_STATUS_SUCCESS) {
            fgIsCmdAccept = TRUE;
            i4BytesWritten =
                snprintf(pcCommand, i4TotalLen,
                         "Get DBG module[%u] log level => [0x%02x]!",
                         u4DbgIdx,
                         (uint8_t) u4DbgMask);
            if (i4BytesWritten < 0)
                DBGLOG(REQ, ERROR, "kalSnprintf fail\n");
        }
    }

    if (!fgIsCmdAccept) {
        i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                                  "Get DBG module log level failed!");
        if (i4BytesWritten < 0)
            DBGLOG(REQ, ERROR, "kalSnprintf fail\n");
    }

    return i4BytesWritten;

}               /* priv_driver_get_sw_ctrl */

static int priv_driver_set_fw_log(struct GLUE_INFO *prGlueInfo,
                                  IN char *pcCommand, IN int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint32_t u4McuDest = 0;
    uint32_t u4LogType = 0;
    struct CMD_FW_LOG_2_HOST_CTRL *prFwLog2HostCtrl;
    uint32_t u4Ret = 0;

    DBGLOG(RSN, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    DBGLOG(RSN, INFO, "MT6632 : priv_driver_set_fw_log\n");

    prFwLog2HostCtrl = (struct CMD_FW_LOG_2_HOST_CTRL *)kalMemAlloc(
                           sizeof(struct CMD_FW_LOG_2_HOST_CTRL), VIR_MEM_TYPE);
    if (!prFwLog2HostCtrl)
        return -1;

    if (i4Argc == 3) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4McuDest);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse u4McuDest error u4Ret=%d\n",
                   u4Ret);

        u4Ret = kalkStrtou32(apcArgv[2], 0, &u4LogType);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse u4LogType error u4Ret=%d\n",
                   u4Ret);

        prFwLog2HostCtrl->ucMcuDest = (uint8_t)u4McuDest;
        prFwLog2HostCtrl->ucFwLog2HostCtrl = (uint8_t)u4LogType;

        rStatus = kalIoctl(prGlueInfo, wlanoidSetFwLog2Host,
                           prFwLog2HostCtrl,
                           sizeof(struct CMD_FW_LOG_2_HOST_CTRL),
                           TRUE, TRUE, TRUE, &u4BufLen);

        DBGLOG(REQ, INFO, "%s: command result is %s (%d %d)\n",
               __func__, pcCommand, u4McuDest, u4LogType);
        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            kalMemFree(prFwLog2HostCtrl, VIR_MEM_TYPE,
                       sizeof(struct CMD_FW_LOG_2_HOST_CTRL));
            return -1;
        }
    } else {
        DBGLOG(REQ, ERROR, "argc %i is not equal to 3\n", i4Argc);
        i4BytesWritten = -1;
    }

    kalMemFree(prFwLog2HostCtrl, VIR_MEM_TYPE,
               sizeof(struct CMD_FW_LOG_2_HOST_CTRL));
    return i4BytesWritten;
}

static int priv_driver_get_mcr(struct GLUE_INFO *prGlueInfo,
                               IN char *pcCommand, IN int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint32_t u4Ret;
    int32_t i4ArgNum = 2;
    struct CMD_ACCESS_REG rCmdAccessReg;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        kalMemSet(&rCmdAccessReg, 0, sizeof(rCmdAccessReg));
        u4Ret = kalkStrtou32((char *)apcArgv[1], 0, &(rCmdAccessReg.u4Address));
        if (u4Ret)
            DBGLOG(REQ, LOUD,
                   "parse get_mcr error (Address) u4Ret=%d\n",
                   u4Ret);

        /* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */

        DBGLOG(REQ, LOUD, "address is %x\n", rCmdAccessReg.u4Address);

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryMcrRead,
                           &rCmdAccessReg, sizeof(rCmdAccessReg),
                           TRUE, TRUE, TRUE, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;

        i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "0x%08x",
                                     (unsigned int)rCmdAccessReg.u4Data);
        DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
               pcCommand);
    }

    return i4BytesWritten;
}               /* priv_driver_get_mcr */

int priv_driver_set_mcr(struct GLUE_INFO *prGlueInfo, IN char *pcCommand,
                        IN int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    uint32_t u4Ret;
    int32_t i4ArgNum = 3;
    struct CMD_ACCESS_REG rCmdAccessReg;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        u4Ret = kalkStrtou32((char *)apcArgv[1], 0, &(rCmdAccessReg.u4Address));
        if (u4Ret)
            DBGLOG(REQ, LOUD,
                   "parse get_mcr error (Address) u4Ret=%d\n",
                   u4Ret);

        u4Ret = kalkStrtou32((char *)apcArgv[2], 0, &(rCmdAccessReg.u4Data));
        if (u4Ret)
            DBGLOG(REQ, LOUD,
                   "parse get_mcr error (Data) u4Ret=%d\n", u4Ret);

        /* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
        /* rCmdAccessReg.u4Data = kalStrtoul(apcArgv[2], NULL, 0); */

        rStatus = kalIoctl(prGlueInfo, wlanoidSetMcrWrite,
                           &rCmdAccessReg, sizeof(rCmdAccessReg),
                           FALSE, FALSE, TRUE, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;
    }

    return i4BytesWritten;

}

int priv_driver_get_scan_res(struct GLUE_INFO *prGlueInfo,
                             char *pcCommand, int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    struct SCAN_INFO *prScanInfo;
    struct LINK *prBSSDescList;
    struct BSS_DESC *prBSSDescNext;
    struct BSS_DESC *prBssDesc;

    ASSERT(prGlueInfo->prAdapter);

    prScanInfo = &(prGlueInfo->prAdapter->rWifiVar.rScanInfo);
    prBSSDescList = &prScanInfo->rBSSDescList;

    i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                               i4TotalLen - i4BytesWritten,
                               "\n%s|%10s|%4s(%4s)|%4s|%3s\r\n", "SSID",
                               "BSSID", "CH", "BW", "RSSI", "RSN");

    /* Search BSS Desc from current SCAN result list. */
    LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext,
                             prBSSDescList, rLinkEntry, struct BSS_DESC) {

        i4BytesWritten += snprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "%s|%02x:%02x:%02x:%02x:%02x:%02x|%4u(%4x)|%4d|%3x\r\n",
                                   prBssDesc->aucSSID,
                                   prBssDesc->aucBSSID[0], prBssDesc->aucBSSID[1],
                                   prBssDesc->aucBSSID[2], prBssDesc->aucBSSID[3],
                                   prBssDesc->aucBSSID[4], prBssDesc->aucBSSID[5],
                                   prBssDesc->ucChannelNum,
                                   prBssDesc->eChannelWidth,
                                   (int32_t) RCPI_TO_dBm(prBssDesc->ucRCPI),
                                   prBssDesc->u2RsnCap);
    }

    return i4BytesWritten;
}


int priv_driver_set_scan(struct GLUE_INFO *prGlueInfo, IN char *pcCommand,
                         IN int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    u16 center_freq[MAXIMUM_OPERATION_CHANNEL_LIST] = {0};
    int num_ch = 0;
    int i = 0;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc <= 1) {
        LOG_USAGE("usage, driver scan [CH#]\n");
        i4BytesWritten = -1;
        return i4BytesWritten;
    }

    for (i = 1; i < i4Argc; i++) {
        DBGLOG(REQ, LOUD, "arg[%d] is %s\n", i, apcArgv[i]);
        if (kalkStrtou16((char *)apcArgv[i], 0,
                         &(center_freq[i - 1]))) {
            DBGLOG(REQ, LOUD, "channel parse fail\n");
            return -1;
        }
    }

    num_ch = i4Argc - 1;

    DBGLOG(REQ, EVENT, "scan param num %d\n", num_ch);

    for (i = 0; i < num_ch; i++)
        DBGLOG(REQ, INFO, "%d ", center_freq[i]);

    mtk_freertos_scan(center_freq, num_ch);
    return i4BytesWritten;
}

int priv_driver_set_connect(struct GLUE_INFO *prGlueInfo, IN char *pcCommand,
                            IN int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    int i = 0;
    u16 center_freq = 0;
    uint8_t bssid[ETH_ALEN] = {};

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc != 4) {
        i4BytesWritten = kalScnprintf(pcCommand, i4TotalLen,
                                      "format: \"connect $CH $BSSID $SSID\"");
        return -1;
    }

    for (i = 0; i < i4Argc; i++)
        DBGLOG(REQ, LOUD, "arg[%d] is %s\n", i, apcArgv[i]);

    if (kalkStrtou16((char *)apcArgv[1], 0, &(center_freq))) {
        DBGLOG(REQ, LOUD, "channel parse fail\n");
        return -1;
    }

    i = wlanHwAddrToBin(apcArgv[2], bssid);
    DBGLOG(REQ, LOUD, "mtk_freertos_connect> freq %d \
		bssid [%02x:%02x:%02x:%02x:%02x:%02x] ssid %s ssid_len %d\n",
           center_freq, bssid[0], bssid[1], bssid[2], bssid[3],
           bssid[4], bssid[5], apcArgv[3], kalStrLen((char *)apcArgv[3]));

    mtk_freertos_connect(center_freq, bssid, (uint8_t *)apcArgv[3],
                         kalStrLen((char *)apcArgv[3]));

    return i4BytesWritten;
}

int netif_set_link_up_api(struct GLUE_INFO *prGlueInfo, IN char *pcCommand, IN int i4TotalLen)
{
    int32_t i4BytesWritten = 0;

    netif_set_link_up(prGlueInfo->prDevHandler->netif);

    return i4BytesWritten;
}

int netif_set_link_down_api(struct GLUE_INFO *prGlueInfo, IN char *pcCommand, IN int i4TotalLen)
{
    int32_t i4BytesWritten = 0;

    netif_set_link_down(prGlueInfo->prDevHandler->netif);

    return i4BytesWritten;
}

#if CFG_SUPPORT_QA_TOOL || (CONFIG_WLAN_SERVICE == 1)
static int priv_driver_set_test_mode(struct GLUE_INFO *prGlueInfo,
                                     char *pcCommand, int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint32_t u4Ret;
    int32_t i4ArgNum = 2;
    uint32_t u4MagicKey = 0;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {

        u4Ret = kalkStrtou32(apcArgv[1], 0, &(u4MagicKey));
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse Magic Key error u4Ret=%d\n",
                   u4Ret);

        DBGLOG(REQ, LOUD, "The Set Test Mode Magic Key is %d\n",
               u4MagicKey);

        if (u4MagicKey == PRIV_CMD_TEST_MAGIC_KEY) {
            rStatus = kalIoctl(prGlueInfo,
                               wlanoidRftestSetTestMode,
                               NULL, 0, FALSE, FALSE, TRUE,
                               &u4BufLen);
        } else if (u4MagicKey == 0) {
            rStatus = kalIoctl(prGlueInfo,
                               wlanoidRftestSetAbortTestMode,
                               NULL, 0, FALSE, FALSE, TRUE,
                               &u4BufLen);
        }

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);

        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;
    }

    return i4BytesWritten;

}               /* priv_driver_set_test_mode */

static int priv_driver_set_test_cmd(struct GLUE_INFO *prGlueInfo,
                                    char *pcCommand, int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint32_t u4Ret;
    int32_t i4ArgNum = 3;
    struct PARAM_MTK_WIFI_TEST_STRUCT rRfATInfo;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        kalMemSet(&rRfATInfo, 0, sizeof(rRfATInfo));
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rRfATInfo.u4FuncIndex));
        if (u4Ret)
            DBGLOG(REQ, LOUD,
                   "Parse Test CMD Index error u4Ret=%d\n", u4Ret);

        u4Ret = kalkStrtou32(apcArgv[2], 0, &(rRfATInfo.u4FuncData));
        if (u4Ret)
            DBGLOG(REQ, LOUD,
                   "Parse Test CMD Data error u4Ret=%d\n", u4Ret);

        DBGLOG(REQ, LOUD,
               "Set Test CMD FuncIndex = %d, FuncData = %d\n",
               rRfATInfo.u4FuncIndex, rRfATInfo.u4FuncData);

        rStatus = kalIoctl(prGlueInfo, wlanoidRftestSetAutoTest,
                           &rRfATInfo, sizeof(rRfATInfo),
                           FALSE, FALSE, TRUE, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;
    }

    return i4BytesWritten;

}               /* priv_driver_set_test_cmd */

static int priv_driver_get_test_result(struct GLUE_INFO *prGlueInfo,
                                       char *pcCommand, int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint32_t u4Ret;
    uint32_t u4Data = 0;
    int32_t i4ArgNum = 3;
    struct PARAM_MTK_WIFI_TEST_STRUCT rRfATInfo;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {
        kalMemSet(&rRfATInfo, 0, sizeof(rRfATInfo));
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rRfATInfo.u4FuncIndex));
        if (u4Ret)
            DBGLOG(REQ, LOUD,
                   "Parse Test CMD Index error u4Ret=%d\n", u4Ret);

        u4Ret = kalkStrtou32(apcArgv[2], 0, &(rRfATInfo.u4FuncData));
        if (u4Ret)
            DBGLOG(REQ, LOUD,
                   "Parse Test CMD Data error u4Ret=%d\n",
                   u4Ret);

        DBGLOG(REQ, LOUD,
               "Get Test CMD FuncIndex = %d, FuncData = %d\n",
               rRfATInfo.u4FuncIndex, rRfATInfo.u4FuncData);

        rStatus = kalIoctl(prGlueInfo, wlanoidRftestQueryAutoTest,
                           &rRfATInfo, sizeof(rRfATInfo),
                           TRUE, TRUE, TRUE, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;
        u4Data = (unsigned int)rRfATInfo.u4FuncData;
        i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
                                     "%d[0x%08x]", u4Data, u4Data);
        DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
               pcCommand);
    }

    return i4BytesWritten;

}   /* priv_driver_get_test_result */
#endif /* #if CFG_SUPPORT_QA_TOOL || (CONFIG_WLAN_SERVICE == 1) */

#if (CFG_SUPPORT_PRIV_CHIP_CONFIG == 1)
static int priv_driver_set_chip_config(IN struct GLUE_INFO *prGlueInfo,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
#if (CFG_SUPPORT_802_11AX == 1)
    struct ADAPTER *prAdapter = NULL;
#endif /* #if (CFG_SUPPORT_802_11AX == 1) */
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    uint32_t u4CmdLen = 0;
    uint32_t u4PrefixLen = 0;
    /* INT_32 i4Argc = 0; */
    /* PCHAR  apcArgv[WLAN_CFG_ARGV_MAX] = {0}; */

    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

    if (prGlueInfo == NULL)
        return -1;

#if (CFG_SUPPORT_802_11AX == 1)
    prAdapter = prGlueInfo->prAdapter;
#endif /* #if (CFG_SUPPORT_802_11AX == 1) */

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    /* wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv); */
    /* DBGLOG(REQ, LOUD,("argc is %i\n",i4Argc)); */
    /*  */
    u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);
    u4PrefixLen = kalStrLen(CMD_SET_CHIP) + 1 /*space */;

    kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

    /* if(i4Argc >= 2) { */
    if (u4CmdLen > u4PrefixLen) {
        rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
        /* rChipConfigInfo.u2MsgSize = kalStrnLen(apcArgv[1],
         *                  CHIP_CONFIG_RESP_SIZE);
         */
        rChipConfigInfo.u2MsgSize = u4CmdLen - u4PrefixLen;
        /* kalStrnCpy(rChipConfigInfo.aucCmd, apcArgv[1],
         *        CHIP_CONFIG_RESP_SIZE);
         */
        kalStrnCpy((char *)rChipConfigInfo.aucCmd,
                   pcCommand + u4PrefixLen,
                   CHIP_CONFIG_RESP_SIZE - 1);
        rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

#if (CFG_SUPPORT_802_11AX == 1)
        if (kalStrnCmp("FrdHeTrig2Host",
                       pcCommand, kalStrLen("FrdHeTrig2Host"))) {
            uint32_t idx = kalStrLen("set_chip FrdHeTrig2Host ");

            prAdapter->fgEnShowHETrigger = pcCommand[idx] - 0x30;
        }
#endif /* #if (CFG_SUPPORT_802_11AX == 1) */

        rStatus = kalIoctl(prGlueInfo, wlanoidSetChipConfig,
                           &rChipConfigInfo, sizeof(rChipConfigInfo),
                           FALSE, FALSE, TRUE, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, INFO, "%s: kalIoctl ret=%d\n", __func__,
                   rStatus);
            i4BytesWritten = -1;
        }
    }

    return i4BytesWritten;

}               /* priv_driver_set_chip_config  */

void priv_driver_get_chip_config_16(uint8_t *pucStartAddr, uint32_t u4Length,
                                    uint32_t u4Line, int i4TotalLen,
                                    int32_t i4BytesWritten, char *pcCommand)
{

    while (u4Length >= 16) {
        if (i4TotalLen > i4BytesWritten) {
            i4BytesWritten +=
                kalSnprintf(pcCommand + i4BytesWritten,
                            i4TotalLen - i4BytesWritten,
                            "%04x %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x %02x %02x  %02x %02x %02x %02x\n",
                            u4Line, pucStartAddr[0],
                            pucStartAddr[1], pucStartAddr[2],
                            pucStartAddr[3], pucStartAddr[4],
                            pucStartAddr[5], pucStartAddr[6],
                            pucStartAddr[7], pucStartAddr[8],
                            pucStartAddr[9], pucStartAddr[10],
                            pucStartAddr[11], pucStartAddr[12],
                            pucStartAddr[13], pucStartAddr[14],
                            pucStartAddr[15]);
        }

        pucStartAddr += 16;
        u4Length -= 16;
        u4Line += 16;
    }           /* u4Length */
}

void priv_driver_get_chip_config_4(uint32_t *pu4StartAddr, uint32_t u4Length,
                                   uint32_t u4Line, int i4TotalLen,
                                   int32_t i4BytesWritten, char *pcCommand)
{
    while (u4Length >= 16) {
        if (i4TotalLen > i4BytesWritten) {
            i4BytesWritten +=
                kalSnprintf(pcCommand + i4BytesWritten,
                            i4TotalLen - i4BytesWritten,
                            "%04x %08x %08x %08x %08x\n", u4Line,
                            pu4StartAddr[0], pu4StartAddr[1],
                            pu4StartAddr[2], pu4StartAddr[3]);
        }

        pu4StartAddr += 4;
        u4Length -= 16;
        u4Line += 4;
    }           /* u4Length */
}

int priv_driver_get_chip_config(IN struct GLUE_INFO *prGlueInfo,
                                IN char *pcCommand, IN int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    int32_t i4BytesWritten = 0;
    uint32_t u4BufLen = 0;
    uint32_t u2MsgSize = 0;
    uint32_t u4CmdLen = 0;
    uint32_t u4PrefixLen = 0;
    /* INT_32 i4Argc = 0; */
    /* PCHAR  apcArgv[WLAN_CFG_ARGV_MAX]; */

    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

    if (prGlueInfo == NULL)
        return -1;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    /* wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv); */
    /* DBGLOG(REQ, LOUD,("argc is %i\n",i4Argc)); */

    u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);
    u4PrefixLen = kalStrLen(CMD_GET_CHIP) + 1 /*space */;

    /* if(i4Argc >= 2) { */
    if (u4CmdLen > u4PrefixLen) {
        rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
        /* rChipConfigInfo.u2MsgSize = kalStrnLen(apcArgv[1],
         *                             CHIP_CONFIG_RESP_SIZE);
         */
        rChipConfigInfo.u2MsgSize = u4CmdLen - u4PrefixLen;
        /* kalStrnCpy(rChipConfigInfo.aucCmd, apcArgv[1],
         *            CHIP_CONFIG_RESP_SIZE);
         */
        kalStrnCpy((char *)rChipConfigInfo.aucCmd,
                   pcCommand + u4PrefixLen,
                   CHIP_CONFIG_RESP_SIZE - 1);
        rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

        rStatus = kalIoctl(prGlueInfo, wlanoidQueryChipConfig,
                           &rChipConfigInfo, sizeof(rChipConfigInfo),
                           TRUE, TRUE, TRUE, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, INFO, "%s: kalIoctl ret=%d\n", __func__,
                   rStatus);
            return -1;
        }

        /* Check respType */
        u2MsgSize = rChipConfigInfo.u2MsgSize;
        DBGLOG(REQ, INFO, "%s: RespTyep  %u\n", __func__,
               rChipConfigInfo.ucRespType);
        DBGLOG(REQ, INFO, "%s: u2MsgSize %u\n", __func__,
               rChipConfigInfo.u2MsgSize);

        if (u2MsgSize > sizeof(rChipConfigInfo.aucCmd)) {
            DBGLOG(REQ, INFO, "%s: u2MsgSize error ret=%u\n",
                   __func__, rChipConfigInfo.u2MsgSize);
            return -1;
        }

        if (u2MsgSize > 0) {

            if (rChipConfigInfo.ucRespType ==
                CHIP_CONFIG_TYPE_ASCII) {
                i4BytesWritten =
                    kalSnprintf(pcCommand + i4BytesWritten,
                                i4TotalLen, "%s",
                                rChipConfigInfo.aucCmd);
            } else {
                uint32_t u4Length;
                uint32_t u4Line;

                if (rChipConfigInfo.ucRespType ==
                    CHIP_CONFIG_TYPE_MEM8) {
                    uint8_t *pucStartAddr = NULL;

                    pucStartAddr = (uint8_t *)
                                   rChipConfigInfo.aucCmd;
                    /* align 16 bytes because one print line
                     * is 16 bytes
                     */
                    u4Length = (((u2MsgSize + 15) >> 4))
                               << 4;
                    u4Line = 0;
                    priv_driver_get_chip_config_16(
                        pucStartAddr, u4Length, u4Line,
                        i4TotalLen, i4BytesWritten,
                        pcCommand);
                } else {
                    uint32_t *pu4StartAddr = NULL;

                    pu4StartAddr = (uint32_t *)
                                   rChipConfigInfo.aucCmd;
                    /* align 16 bytes because one print line
                     * is 16 bytes
                     */
                    u4Length = (((u2MsgSize + 15) >> 4))
                               << 4;
                    u4Line = 0;

                    if (IS_ALIGN_4(
                            (unsigned long) pu4StartAddr)) {
                        priv_driver_get_chip_config_4(
                            pu4StartAddr, u4Length,
                            u4Line, i4TotalLen,
                            i4BytesWritten,
                            pcCommand);
                    } else {
                        DBGLOG(REQ, INFO,
                               "%s: rChipConfigInfo.aucCmd is not 4 bytes alignment %p\n",
                               __func__,
                               rChipConfigInfo.aucCmd);
                    }
                }   /* ChipConfigInfo.ucRespType */
            }
        }
        /* u2MsgSize > 0 */
        DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__,
               pcCommand);
    }
    /* i4Argc */
    return i4BytesWritten;

} /* priv_driver_get_chip_config  */
#endif /* #if (CFG_SUPPORT_PRIV_CHIP_CONFIG == 1) */

#if (CFG_SUPPORT_PRIV_RUN_HQA == 1)
static int priv_driver_run_hqa(
    IN struct GLUE_INFO *prGlueInfo,
    IN char *pcCommand,
    IN int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int8_t *this_char = NULL;
#if (CONFIG_WLAN_SERVICE == 1)
    struct hqa_frame_ctrl local_hqa;
    uint8_t *dataptr = NULL;
    uint8_t *oridataptr = NULL;
    int32_t datalen = 0;
#if DBG || (CONFIG_WIFI_MEM_DBG == 1)
    int32_t oridatalen = 0;
#endif /* #if DBG || (CONFIG_WIFI_MEM_DBG == 1) */
    int32_t ret = WLAN_STATUS_FAILURE;
    int16_t i2tmpVal = 0;
    int32_t i4tmpVal = 0;
#endif /* #if (CONFIG_WLAN_SERVICE == 1) */
    int32_t i = 0;

    if (GLUE_CHK_PR2(prGlueInfo, pcCommand) == FALSE)
        return -1;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    /*Roll over "HQA ", handle xxx=y,y,y,y,y,y....*/
    /* iwpriv wlan0 driver HQA xxx=y,y,y,y,y,y.....*/
    this_char = (int8_t *)kalStrStr(pcCommand, "HQA ");
    if (!this_char)
        return -1;
    this_char += strlen("HQA ");

    /*handle white space*/
    i = strspn((char *)this_char, " ");
    this_char += i;

    DBGLOG(REQ, LOUD, "this_char is %s\n", this_char);
#if (CONFIG_WLAN_SERVICE == 1)
    if (this_char) {
        local_hqa.type = 1;
        local_hqa.hqa_frame_comm.hqa_frame_string = this_char;
        ret = mt_agent_hqa_cmd_handler(&prGlueInfo->rService,
                                       &local_hqa);
    }

    if (ret != WLAN_STATUS_SUCCESS)
        return -1;

    datalen = NTOHS(local_hqa.hqa_frame_comm.hqa_frame_eth->length);
    dataptr = kalMemAlloc(datalen, VIR_MEM_TYPE);
    if (dataptr == NULL)
        return -1;
    /* Backup Original Ptr /Len for mem Free */
    oridataptr = dataptr;
#if DBG || (CONFIG_WIFI_MEM_DBG == 1)
    oridatalen = datalen;
#endif /* #if DBG || (CONFIG_WIFI_MEM_DBG == 1) */

    kalMemCopy(dataptr,
               local_hqa.hqa_frame_comm.hqa_frame_eth->data, datalen);

    DBGLOG(REQ, LOUD,
           "priv_driver_run_hqa datalen is %d\n", datalen);
    DBGLOG_MEM8(REQ, LOUD, dataptr, datalen);

    /*parsing ret 2 bytes*/
    if ((dataptr) && (datalen)) {
        i2tmpVal = dataptr[1] << 8 | dataptr[0];
        i4BytesWritten += kalSnprintf(pcCommand, i4TotalLen,
                                      "Return : 0x%04x\n", i2tmpVal);

        datalen -= 2;
        dataptr += 2;
    } else {
        DBGLOG(REQ, ERROR,
               "priv_driver_run_hqa not support\n");
#if DBG || (CONFIG_WIFI_MEM_DBG == 1)
        kalMemFree(oridataptr, VIR_MEM_TYPE, oridatalen);
#else /* #if DBG || (CONFIG_WIFI_MEM_DBG == 1) */
        kalMemFree(oridataptr, VIR_MEM_TYPE, 0);
#endif /* #if DBG || (CONFIG_WIFI_MEM_DBG == 1) */
        return -1;
    }

    /*parsing remaining data n bytes ( 4 bytes per parameter)*/
    for (i = 0; i < datalen ; i += 4, dataptr += 4) {
        if ((dataptr) && (datalen)) {
            i4tmpVal = dataptr[3] << 24 | dataptr[2] << 16 |
                       dataptr[1] << 8 | dataptr[0];
            if (datalen == 4) {
                i4BytesWritten +=
                    kalSnprintf(pcCommand + i4BytesWritten,
                                i4TotalLen, "ExtId : 0x%08x\n", i4tmpVal);
            } else if (datalen == 8) {
                i4BytesWritten +=
                    kalSnprintf(pcCommand + i4BytesWritten,
                                i4TotalLen, "Band%d TX : 0x%08x\n", i / 4,
                                NTOHL(i4tmpVal));
            } else {
                i4BytesWritten +=
                    kalSnprintf(pcCommand + i4BytesWritten,
                                i4TotalLen, "id%d : 0x%08x\n", i / 4,
                                NTOHL(i4tmpVal));
            }
        }
    }

    kalMemFree(oridataptr, VIR_MEM_TYPE, 0);
#else /* #if (CONFIG_WLAN_SERVICE == 1) */
    DBGLOG(REQ, ERROR,
           "wlan_service not support\n");
#endif /* #if (CONFIG_WLAN_SERVICE == 1) */

    return i4BytesWritten;

}

#endif /* #if (CFG_SUPPORT_PRIV_RUN_HQA == 1) */

#if (CFG_SUPPORT_PRIV_GET_MIB_INFO == 1)
static int priv_driver_get_mib_info(IN struct GLUE_INFO *prGlueInfo,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    uint8_t i;
    uint32_t u4Per;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    struct PARAM_HW_MIB_INFO *prHwMibInfo;
    struct RX_CTRL *prRxCtrl;

    if (GLUE_CHK_PR2(prGlueInfo, pcCommand) == FALSE)
        return -1;

    prRxCtrl = &prGlueInfo->prAdapter->rRxCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    DBGLOG(REQ, INFO, "MT6632 : priv_driver_get_mib_info\n");

    prHwMibInfo = (struct PARAM_HW_MIB_INFO *)kalMemAlloc(
                      sizeof(struct PARAM_HW_MIB_INFO), VIR_MEM_TYPE);
    if (!prHwMibInfo)
        return -1;

    if (i4Argc == 1)
        prHwMibInfo->u4Index = 0;

    if (i4Argc >= 2)
        kalkStrtou32(apcArgv[1], 0, &prHwMibInfo->u4Index);

    DBGLOG(REQ, INFO, "MT6632 : index = %d\n", prHwMibInfo->u4Index);

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryMibInfo, prHwMibInfo,
                       sizeof(struct PARAM_HW_MIB_INFO),
                       TRUE, TRUE, TRUE, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        kalMemFree(prHwMibInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_HW_MIB_INFO));
        return -1;
    }

    if (prHwMibInfo->u4Index < 2) {
        i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s",
                                     "\n\nmib state:\n");
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "Dump MIB info of IDX         = %d\n",
                                      prHwMibInfo->u4Index);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "%s", "===Rx Related Counters===\n");
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tRx with CRC=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4RxFcsErrCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx drop due to out of resource=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4RxFifoFullCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tRx Mpdu=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4RxMpduCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tRx AMpdu=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4RxAMPDUCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tRx PF Drop=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4PFDropCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx Len Mismatch=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4RxLenMismatchCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx data indicate total=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_DATA_INDICATION_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx data retain total=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_DATA_RETAINED_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx drop by SW total=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_DROP_TOTAL_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tRx reorder miss=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_MISS_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx reorder within=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_WITHIN_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx reorder ahead=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_AHEAD_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx reorder behind=%llu\n", RX_GET_CNT(prRxCtrl,
                                                                               RX_DATA_REORDER_BEHIND_COUNT));

        do {
            uint32_t u4AmsduCntx100 = 0;

            if (RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT))
                u4AmsduCntx100 =
                    (uint32_t)div64_u64(RX_GET_CNT(prRxCtrl,
                                                   RX_DATA_MSDU_IN_AMSDU_COUNT) * 100,
                                        RX_GET_CNT(prRxCtrl,
                                                   RX_DATA_AMSDU_COUNT));

            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tRx avg MSDU in AMSDU=%1d.%02d\n",
                                  u4AmsduCntx100 / 100, u4AmsduCntx100 % 100);
        } while (FALSE);

        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx total MSDU in AMSDU=%llu\n", RX_GET_CNT(prRxCtrl,
                                                                                    RX_DATA_MSDU_IN_AMSDU_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx AMSDU=%llu\n", RX_GET_CNT(prRxCtrl,
                                                                      RX_DATA_AMSDU_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tRx AMSDU miss=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_MISS_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx no StaRec drop=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_NO_STA_DROP_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx inactive BSS drop=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_INACTIVE_BSS_DROP_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx HS20 drop=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_HS20_DROP_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx low SwRfb drop=%llu\n", RX_GET_CNT(prRxCtrl,
                                                                               RX_LESS_SW_RFB_DROP_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx dupicate drop=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_DUPICATE_DROP_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tRx MIC err drop=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_MIC_ERROR_DROP_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tRx BAR handle=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_BAR_DROP_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx non-interest drop=%llu\n", RX_GET_CNT(prRxCtrl,
                                                                                  RX_NO_INTEREST_DROP_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx type err drop=%llu\n",
                                      RX_GET_CNT(prRxCtrl, RX_TYPE_ERR_DROP_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRx class err drop=%llu\n", RX_GET_CNT(prRxCtrl,
                                                                               RX_CLASS_ERR_DROP_COUNT));
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "%s", "===Phy/Timing Related Counters===\n");
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tChannelIdleCnt=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4ChannelIdleCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tCCA_NAV_Tx_Time=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4CcaNavTx);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tRx_MDRDY_CNT=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4MdrdyCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tCCK_MDRDY=%d, OFDM_MDRDY=0x%x, OFDM_GREEN_MDRDY=0x%x\n",
                                      prHwMibInfo->rHwMibCnt.u4CCKMdrdyCnt,
                                      prHwMibInfo->rHwMibCnt.u4OFDMLGMixMdrdy,
                                      prHwMibInfo->rHwMibCnt.u4OFDMGreenMdrdy);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tPrim CCA Time=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4PCcaTime);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tSec CCA Time=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4SCcaTime);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tPrim ED Time=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4PEDTime);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "%s",
                                      "===Tx Related Counters(Generic)===\n");
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tBeaconTxCnt=%d\n",
                                      prHwMibInfo->rHwMibCnt.u4BeaconTxCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tTx 40MHz Cnt=%d\n",
                                      prHwMibInfo->rHwMib2Cnt.u4Tx40MHzCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tTx 80MHz Cnt=%d\n",
                                      prHwMibInfo->rHwMib2Cnt.u4Tx80MHzCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tTx 160MHz Cnt=%d\n",
                                      prHwMibInfo->rHwMib2Cnt.u4Tx160MHzCnt);
        for (i = 0; i < BSSID_NUM; i++) {
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\t===BSSID[%d] Related Counters===\n", i);
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tBA Miss Cnt=%d\n",
                                  prHwMibInfo->rHwMibCnt.au4BaMissedCnt[i]);
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tRTS Tx Cnt=%d\n",
                                  prHwMibInfo->rHwMibCnt.au4RtsTxCnt[i]);
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tFrame Retry Cnt=%d\n",
                                  prHwMibInfo->rHwMibCnt.au4FrameRetryCnt[i]);
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tFrame Retry 2 Cnt=%d\n",
                                  prHwMibInfo->rHwMibCnt.au4FrameRetry2Cnt[i]);
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tRTS Retry Cnt=%d\n",
                                  prHwMibInfo->rHwMibCnt.au4RtsRetryCnt[i]);
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tAck Failed Cnt=%d\n",
                                  prHwMibInfo->rHwMibCnt.au4AckFailedCnt[i]);
        }

        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "%s", "===AMPDU Related Counters===\n");
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tTx AMPDU_Pkt_Cnt=%d\n",
                                      prHwMibInfo->rHwTxAmpduMts.u2TxAmpduCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tTx AMPDU_MPDU_Pkt_Cnt=%d\n",
                                      prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "\tAMPDU SuccessCnt=%d\n",
                                      prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tAMPDU Tx success      = %d\n",
                                      prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt);

        u4Per = prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt == 0 ? 0 :
                (1000 * (prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt -
                         prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt)) /
                prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt;
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tAMPDU Tx fail count   = %d, PER=%d.%1d%%\n",
                                      prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt -
                                      prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt,
                                      u4Per / 10, u4Per % 10);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "%s", "\tTx Agg\n");
#if (CFG_SUPPORT_802_11AX == 1)
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "%s", "\tRange:  1    2~9   10~18    19~27   ");
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "%s", "28~36    37~45    46~54    55~78\n");
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\t\t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d\n",
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange1AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange2AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange3AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange4AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange5AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange6AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange7AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange8AmpduCnt);
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "%s", "\tRange: 79~102 103~126 127~150 151~174 ");
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "%s", "174~198 199~222 223~246 247~255\n");
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\t\t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d\n",
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange9AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange10AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange11AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange12AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange13AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange14AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange15AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange16AmpduCnt);
#else /* #if (CFG_SUPPORT_802_11AX == 1) */
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "%s",
                                      "\tRange:  1    2~5   6~15    16~22   23~33    34~49    50~57    58~64\n"
                                     );
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\t\t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d\n",
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange1AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange2AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange3AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange4AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange5AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange6AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange7AmpduCnt,
                                      prHwMibInfo->rHwTxAmpduMts.u2TxRange8AmpduCnt);
#endif /* #if (CFG_SUPPORT_802_11AX == 1) */
    } else
        i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s",
                                     "\nClear All Statistics\n");

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

    kalMemFree(prHwMibInfo, VIR_MEM_TYPE, sizeof(struct PARAM_HW_MIB_INFO));

    nicRxClearStatistics(prGlueInfo->prAdapter);

    return i4BytesWritten;
}
#endif /* #if (CFG_SUPPORT_PRIV_GET_MIB_INFO == 1) */

#if (CFG_SUPPORT_PRIV_GET_DRV_VER == 1)
static int priv_driver_get_version(IN struct GLUE_INFO *prGlueInfo,
                                   IN char *pcCommand, IN int i4TotalLen)
{
    struct ADAPTER *prAdapter;
    int32_t i4BytesWritten = 0;
    uint32_t u4Offset = 0;

    prAdapter = prGlueInfo->prAdapter;

    u4Offset += fwDlGetFwdlInfo(prAdapter, pcCommand, i4TotalLen);
    u4Offset += kalSnprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                            "WiFi Driver Version %u.%u.%u-%s(%s)\n",
                            NIC_DRIVER_MAJOR_VERSION,
                            NIC_DRIVER_MINOR_VERSION,
                            NIC_DRIVER_SERIAL_VERSION,
                            kalGetDrvBuildTime(),
                            kalGetDrvToTSha());

    i4BytesWritten = (int32_t)u4Offset;

    halDumpHifStatus(prAdapter, NULL, 0);

    return i4BytesWritten;
}
#endif /* #if (CFG_SUPPORT_PRIV_GET_DRV_VER == 1) */

#if (CFG_SUPPORT_PRIV_FIXED_RATE == 1)
int priv_driver_set_fixed_rate(IN struct GLUE_INFO *prGlueInfo,
                               IN char *pcCommand, IN int i4TotalLen)
{
    struct ADAPTER *prAdapter = NULL;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
    /* INT_32 u4Ret = 0; */
    uint32_t u4WCID = 0;
    int32_t i4Recv = 0;
    int8_t *this_char = NULL;
    uint32_t u4Id = 0xa0610000;
    uint32_t u4Id2 = 0xa0600000;
    static uint8_t fgIsUseWCID = FALSE;
    struct FIXED_RATE_INFO rFixedRate;
    struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

    if (GLUE_CHK_PR2(prGlueInfo, pcCommand) == FALSE)
        return -1;
    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

    this_char = (int8_t *)kalStrStr((char *)*apcArgv, "=");
    if (!this_char)
        return -1;
    this_char++;

    kalMemZero(&rFixedRate, sizeof(struct FIXED_RATE_INFO));
    DBGLOG(REQ, LOUD, "string = %s\n", this_char);

    if (strnicmp((char *)this_char, "auto", strlen("auto")) == 0) {
        i4Recv = 1;
    } else if (strnicmp((char *)this_char,
                        "UseWCID", strlen("UseWCID")) == 0) {
        i4Recv = 2;
        fgIsUseWCID = TRUE;
    } else if (strnicmp((char *)this_char,
                        "ApplyAll", strlen("ApplyAll")) == 0) {
        i4Recv = 3;
        fgIsUseWCID = FALSE;
    } else {
        i4Recv = sscanf((char *)this_char,
                        "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
                        &(u4WCID),
                        &(rFixedRate.u4Mode),
                        &(rFixedRate.u4Bw),
                        &(rFixedRate.u4Mcs),
                        &(rFixedRate.u4VhtNss),
                        &(rFixedRate.u4SGI),
                        &(rFixedRate.u4Preamble),
                        &(rFixedRate.u4STBC),
                        &(rFixedRate.u4LDPC),
                        &(rFixedRate.u4SpeEn),
                        &(rFixedRate.u4HeLTF),
                        &(rFixedRate.u4HeErDCM),
                        &(rFixedRate.u4HeEr106t));

        DBGLOG(REQ, LOUD, "u4WCID=%d\nu4Mode=%d\nu4Bw=%d\n", u4WCID,
               rFixedRate.u4Mode, rFixedRate.u4Bw);
        DBGLOG(REQ, LOUD, "u4Mcs=%d\nu4VhtNss=%d\nu4GI=%d\n",
               rFixedRate.u4Mcs,
               rFixedRate.u4VhtNss, rFixedRate.u4SGI);
        DBGLOG(REQ, LOUD, "u4Preamble=%d\nu4STBC=%d\n",
               rFixedRate.u4Preamble,
               rFixedRate.u4STBC);
        DBGLOG(REQ, LOUD, "u4LDPC=%d\nu4SpeEn=%d\nu4HeLTF=%d\n",
               rFixedRate.u4LDPC, rFixedRate.u4SpeEn,
               rFixedRate.u4HeLTF);
        DBGLOG(REQ, LOUD, "u4HeErDCM=%d\nu4HeEr106t=%d\n",
               rFixedRate.u4HeErDCM, rFixedRate.u4HeEr106t);
        DBGLOG(REQ, LOUD, "fgIsUseWCID=%d\n", fgIsUseWCID);
    }

    if (i4Recv == 1) {
        rSwCtrlInfo.u4Id = u4Id2;
        rSwCtrlInfo.u4Data = 0;

        rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
                           &rSwCtrlInfo, sizeof(rSwCtrlInfo),
                           FALSE, FALSE, TRUE, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;
    } else if (i4Recv == 2 || i4Recv == 3) {
        i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
                                     "Update fgIsUseWCID %d\n", fgIsUseWCID);
    } else if (i4Recv == 10 || i4Recv == 13) {
        rSwCtrlInfo.u4Id = u4Id;
        if (fgIsUseWCID && u4WCID < prAdapter->ucWtblEntryNum &&
            prAdapter->rWifiVar.arWtbl[u4WCID].ucUsed) {
            rSwCtrlInfo.u4Id |= u4WCID;
            rSwCtrlInfo.u4Id |= BIT(8);
            i4BytesWritten = kalSnprintf(
                                 pcCommand, i4TotalLen,
                                 "Apply WCID %d\n", u4WCID);
        } else {
            i4BytesWritten = kalSnprintf(
                                 pcCommand, i4TotalLen, "Apply All\n");
        }

        if (rFixedRate.u4Mode >= TX_RATE_MODE_HE_SU) {
            if (i4Recv == 10) {
                /* Give default value */
                rFixedRate.u4HeLTF = HE_LTF_2X;
                rFixedRate.u4HeErDCM = 0;
                rFixedRate.u4HeEr106t = 0;
            }
            /* check HE-LTF and HE GI combinations */
            if (WLAN_STATUS_SUCCESS !=
                nicRateHeLtfCheckGi(&rFixedRate))
                return -1;

            /* check DCM limitation */
            if (rFixedRate.u4HeErDCM) {
                if ((rFixedRate.u4STBC) ||
                    (rFixedRate.u4VhtNss > 2))
                    return -1;

                if ((rFixedRate.u4Mcs > 4) ||
                    (rFixedRate.u4Mcs == 2))
                    return -1;
            }

            /* check ER_SU limitation */
            if (rFixedRate.u4Mode == TX_RATE_MODE_HE_ER) {
                if ((rFixedRate.u4Bw > 0) ||
                    (rFixedRate.u4VhtNss > 1))
                    return -1;

                if (rFixedRate.u4HeEr106t) {
                    if (rFixedRate.u4Mcs > 0)
                        return -1;
                } else {
                    if (rFixedRate.u4Mcs > 2)
                        return -1;
                }
            }
        }

        rStatus = nicSetFixedRateData(&rFixedRate, &rSwCtrlInfo.u4Data);

        if (rStatus == WLAN_STATUS_SUCCESS) {
            rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
                               &rSwCtrlInfo, sizeof(rSwCtrlInfo),
                               FALSE, FALSE, TRUE, &u4BufLen);
        }

        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;
    } else {
        DBGLOG(REQ, ERROR, "iwpriv wlanXX driver FixedRate=Option\n");
        DBGLOG(REQ, ERROR, "Option support 10 or 13 parameters\n");
        DBGLOG(REQ, ERROR,
               "Option:[WCID]-[Mode]-[BW]-[MCS]-[VhtHeNss]-[GI]-[Preamble]-[STBC]-[LDPC]-[SPE_EN]\n");
        DBGLOG(REQ, ERROR,
               "13 param support another [HE-LTF]-[HE-ER-DCM]-[HE-ER-106]\n");
        DBGLOG(REQ, ERROR, "[WCID]Wireless Client ID\n");
        DBGLOG(REQ, ERROR, "[Mode]CCK=0, OFDM=1, HT=2, GF=3, VHT=4");
        DBGLOG(REQ, ERROR,
               "PLR=5, HE_SU=8, HE_ER_SU=9, HE_TRIG=10, HE_MU=11\n");
        DBGLOG(REQ, ERROR, "[BW]BW20=0, BW40=1, BW80=2,BW160=3\n");
        DBGLOG(REQ, ERROR,
               "[MCS]CCK=0~3, OFDM=0~7, HT=0~32, VHT=0~9, HE=0~11\n");
        DBGLOG(REQ, ERROR, "[VhtHeNss]1~8, Other=ignore\n");
        DBGLOG(REQ, ERROR, "[GI]HT/VHT: 0:Long, 1:Short, ");
        DBGLOG(REQ, ERROR,
               "HE: SGI=0(0.8us), MGI=1(1.6us), LGI=2(3.2us)\n");
        DBGLOG(REQ, ERROR, "[Preamble]Long=0, Other=Short\n");
        DBGLOG(REQ, ERROR, "[STBC]Enable=1, Disable=0\n");
        DBGLOG(REQ, ERROR, "[LDPC]BCC=0, LDPC=1\n");
        DBGLOG(REQ, ERROR, "[HE-LTF]1X=0, 2X=1, 4X=2\n");
        DBGLOG(REQ, ERROR, "[HE-ER-DCM]Enable=1, Disable=0\n");
        DBGLOG(REQ, ERROR, "[HE-ER-106]106-tone=1\n");
    }

    return i4BytesWritten;
}
#endif /* #if (CFG_SUPPORT_PRIV_FIXED_RATE == 1) */

#if (CFG_SUPPORT_PRIV_STA_STAT == 1)
static int priv_driver_get_sta_stat(IN struct GLUE_INFO *prGlueInfo,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    struct ADAPTER *prAdapter = NULL;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0, u4Ret, u4StatGroup = 0xFFFFFFFF;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint8_t aucMacAddr[MAC_ADDR_LEN];
    uint8_t ucWlanIndex = 0;
    uint8_t *pucMacAddr = NULL;
    struct PARAM_HW_WLAN_INFO *prHwWlanInfo = NULL;
    struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics = NULL;
    u_int8_t fgResetCnt = FALSE;
    u_int8_t fgRxCCSel = FALSE;
    u_int8_t fgSearchMacAddr = FALSE;
    struct BSS_INFO *prAisBssInfo = NULL;
    uint8_t ucStaIdx = 0;

    if (GLUE_CHK_PR2(prGlueInfo, pcCommand) == FALSE)
        return -1;

    prAdapter = prGlueInfo->prAdapter;

    prAisBssInfo = aisGetAisBssInfo(
                       prGlueInfo->prAdapter, prGlueInfo->prDevHandler->bss_idx);

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 4) {
        if (strnicmp((char *)apcArgv[2], CMD_STAT_GROUP_SEL,
                     strlen(CMD_STAT_GROUP_SEL)) == 0) {
            u4Ret = kalkStrtou32(apcArgv[3], 0, &(u4StatGroup));

            if (u4Ret)
                DBGLOG(REQ, LOUD,
                       "parse get_sta_stat error (Group) u4Ret=%d\n",
                       u4Ret);
            if (u4StatGroup == 0)
                u4StatGroup = 0xFFFFFFFF;

            wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);

            if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter,
                                         &aucMacAddr[0], &ucWlanIndex)) {
                DBGLOG(REQ, INFO,
                       "wlan index of %pM is not found!\n",
                       aucMacAddr);
                goto out;
            }
        } else {
            goto out;
        }
    } else if (i4Argc >= 3) {
        if (strnicmp((char *)apcArgv[1], CMD_STAT_GROUP_SEL,
                     strlen(CMD_STAT_GROUP_SEL)) == 0) {
            u4Ret = kalkStrtou32(apcArgv[2], 0, &(u4StatGroup));

            if (u4Ret)
                DBGLOG(REQ, LOUD,
                       "parse get_sta_stat error (Group) u4Ret=%d\n",
                       u4Ret);
            if (u4StatGroup == 0)
                u4StatGroup = 0xFFFFFFFF;

            if (prAisBssInfo->prStaRecOfAP) {
                ucWlanIndex = prAisBssInfo->prStaRecOfAP
                              ->ucWlanIndex;
            } else if (!wlanGetWlanIdxByAddress(
                           prGlueInfo->prAdapter, NULL,
                           &ucWlanIndex)) {
                DBGLOG(REQ, INFO,
                       "wlan index of %pM is not found!\n",
                       aucMacAddr);
                goto out;
            }
        } else {
            if (strnicmp((char *)apcArgv[1], CMD_STAT_RESET_CNT,
                         strlen(CMD_STAT_RESET_CNT)) == 0) {
                wlanHwAddrToBin(apcArgv[2], &aucMacAddr[0]);
                fgResetCnt = TRUE;
            } else if (strnicmp((char *)apcArgv[2],
                                CMD_STAT_RESET_CNT,
                                strlen(CMD_STAT_RESET_CNT)) == 0) {
                wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);
                fgResetCnt = TRUE;
            } else {
                wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);
                fgResetCnt = FALSE;
            }

            if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter,
                                         &aucMacAddr[0], &ucWlanIndex)) {
                DBGLOG(REQ, INFO,
                       "wlan index of %pM is not found!\n",
                       aucMacAddr);
                goto out;
            }
        }
    } else {
        if (i4Argc == 1) {
            fgSearchMacAddr = TRUE;
        } else if (i4Argc == 2) {
            if (strnicmp((char *)apcArgv[1], CMD_STAT_RESET_CNT,
                         strlen(CMD_STAT_RESET_CNT)) == 0) {
                fgResetCnt = TRUE;
                fgSearchMacAddr = TRUE;
            } else if (strnicmp((char *)apcArgv[1],
                                CMD_STAT_NOISE_SEL,
                                strlen(CMD_STAT_NOISE_SEL)) == 0) {
                fgRxCCSel = TRUE;
                fgSearchMacAddr = TRUE;
            } else {
                wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);

                if (!wlanGetWlanIdxByAddress(prGlueInfo->
                                             prAdapter, &aucMacAddr[0], &ucWlanIndex)) {
                    DBGLOG(REQ, INFO,
                           "No connected peer found!\n");
                    goto out;
                }
            }
        }

        if (fgSearchMacAddr) {
            /* Get AIS AP address for no argument */
            if (prAisBssInfo->prStaRecOfAP) {
                ucWlanIndex = prAisBssInfo->prStaRecOfAP
                              ->ucWlanIndex;
            } else if (!wlanGetWlanIdxByAddress(prGlueInfo->
                                                prAdapter, NULL, &ucWlanIndex)) {
                DBGLOG(REQ, INFO, "No connected peer found!\n");
                goto out;
            }
        }
    }

    prHwWlanInfo = (struct PARAM_HW_WLAN_INFO *)kalMemAlloc(
                       sizeof(struct PARAM_HW_WLAN_INFO), VIR_MEM_TYPE);
    if (!prHwWlanInfo) {
        DBGLOG(REQ, ERROR,
               "Allocate prHwWlanInfo failed!\n");
        i4BytesWritten = -1;
        goto out;
    }

    prHwWlanInfo->u4Index = ucWlanIndex;
    if (fgRxCCSel == TRUE)
        prHwWlanInfo->rWtblRxCounter.fgRxCCSel = TRUE;
    else
        prHwWlanInfo->rWtblRxCounter.fgRxCCSel = FALSE;

    DBGLOG(REQ, ERROR, "index = %d i4TotalLen = %d\n",
           prHwWlanInfo->u4Index, i4TotalLen);

    /* Get WTBL info */
    rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
                       sizeof(struct PARAM_HW_WLAN_INFO),
                       TRUE, TRUE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "Query prHwWlanInfo failed!\n");
        i4BytesWritten = -1;
        goto out;
    }

    /* Get Statistics info */
    prQueryStaStatistics =
        (struct PARAM_GET_STA_STATISTICS *)kalMemAlloc(
            sizeof(struct PARAM_GET_STA_STATISTICS), VIR_MEM_TYPE);
    if (!prQueryStaStatistics) {
        DBGLOG(REQ, ERROR,
               "Allocate prQueryStaStatistics failed!\n");
        i4BytesWritten = -1;
        goto out;
    }

    prQueryStaStatistics->ucResetCounter = fgResetCnt;

    pucMacAddr = wlanGetStaAddrByWlanIdx(prGlueInfo->prAdapter,
                                         ucWlanIndex);

    if (!pucMacAddr) {
        DBGLOG(REQ, ERROR, "Addr of WlanIndex %d is not found!\n",
               ucWlanIndex);
        i4BytesWritten = -1;
        goto out;
    }
    COPY_MAC_ADDR(prQueryStaStatistics->aucMacAddr, pucMacAddr);

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
                       prQueryStaStatistics,
                       sizeof(struct PARAM_GET_STA_STATISTICS),
                       TRUE, TRUE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "Query prQueryStaStatistics failed!\n");
        i4BytesWritten = -1;
        goto out;
    }

    if (pucMacAddr) {
        struct CHIP_DBG_OPS *prChipDbg;

        prChipDbg = prAdapter->chip_info->prDebugOps;

        if (prChipDbg && prChipDbg->show_stat_info)
            i4BytesWritten = prChipDbg->show_stat_info(prAdapter,
                                                       pcCommand, i4TotalLen, prHwWlanInfo,
                                                       prQueryStaStatistics, fgResetCnt, u4StatGroup);
    }
    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

    if (wlanGetStaIdxByWlanIdx(prGlueInfo->prAdapter,
                               ucWlanIndex, &ucStaIdx) == WLAN_STATUS_SUCCESS)
        cnmDumpStaRec(prGlueInfo->prAdapter, ucStaIdx);
out:
    if (prHwWlanInfo)
        kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_HW_WLAN_INFO));

    if (prQueryStaStatistics)
        kalMemFree(prQueryStaStatistics, VIR_MEM_TYPE,
                   sizeof(struct PARAM_GET_STA_STATISTICS));


    if (fgResetCnt)
        nicRxClearStatistics(prGlueInfo->prAdapter);

#ifdef MTK_SIGMA_ENABLE
    if (i4BytesWritten > 0) {
        char *ret;
        int pos = 0;
        memset(g_rssi, 0, sizeof(g_rssi));
        ret = strstr(pcCommand, "Beacon RSSI");
        ret = strchr(ret, '=');
        ret++;
        while (*ret != '\n') {
            if (*ret != ' ')
                g_rssi[pos++] = *ret;
            ret++;
        }
    }
#endif /* #ifdef MTK_SIGMA_ENABLE */
    return i4BytesWritten;
}
#endif /* #if (CFG_SUPPORT_PRIV_STA_STAT == 1) */

#if (CFG_SUPPORT_PRIV_WTBL_INFO == 1)
static u_int8_t priv_driver_get_sgi_info(
    IN struct PARAM_PEER_CAP *prWtblPeerCap)
{
    if (!prWtblPeerCap)
        return FALSE;

    switch (prWtblPeerCap->ucFrequencyCapability) {
        case BW_20:
            return prWtblPeerCap->fgG2;
        case BW_40:
            return prWtblPeerCap->fgG4;
        case BW_80:
            return prWtblPeerCap->fgG8;
        case BW_160:
            return prWtblPeerCap->fgG16;
        default:
            return FALSE;
    }
}

static u_int8_t priv_driver_get_ldpc_info(
    IN struct PARAM_TX_CONFIG *prWtblTxConfig)
{
    if (!prWtblTxConfig)
        return FALSE;

    if (prWtblTxConfig->fgIsVHT)
        return prWtblTxConfig->fgVhtLDPC;
    else
        return prWtblTxConfig->fgLDPC;
}

int32_t priv_driver_rate_to_string(IN char *pcCommand,
                                   IN int i4TotalLen, uint8_t TxRx,
                                   struct PARAM_HW_WLAN_INFO *prHwWlanInfo)
{
    uint8_t i, txmode, rate, stbc;
    uint8_t nss;
    int32_t i4BytesWritten = 0;

    for (i = 0; i < AUTO_RATE_NUM; i++) {

        txmode = HW_TX_RATE_TO_MODE(
                     prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);
        if (txmode >= ENUM_TX_MODE_NUM)
            txmode = ENUM_TX_MODE_NUM - 1;
        rate = HW_TX_RATE_TO_MCS(
                   prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);
        nss = HW_TX_RATE_TO_NSS(
                  prHwWlanInfo->rWtblRateInfo.au2RateCode[i]) + 1;
        stbc = HW_TX_RATE_TO_STBC(
                   prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);

        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "\tRate index[%d] ", i);

        if (prHwWlanInfo->rWtblRateInfo.ucRateIdx == i) {
            if (TxRx == 0)
                i4BytesWritten += kalSnprintf(
                                      pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "%s", "[Last RX Rate] ");
            else
                i4BytesWritten += kalSnprintf(
                                      pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "%s", "[Last TX Rate] ");
        } else
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "%s", "               ");

        if (txmode == TX_RATE_MODE_CCK)
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "%s, ",
                                  rate < 4 ? HW_TX_RATE_CCK_STR[rate] :
                                  HW_TX_RATE_CCK_STR[4]);
        else if (txmode == TX_RATE_MODE_OFDM)
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "%s, ",
                                  nicHwRateOfdmStr(rate));
        else {
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "NSS%d_MCS%d, ", nss, rate);
        }

        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "%s, ", HW_TX_RATE_BW[
                                   prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability]);

        if (txmode == TX_RATE_MODE_CCK)
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "%s, ",
                                  rate < 4 ? "LP" : "SP");
        else if (txmode == TX_RATE_MODE_OFDM)
            ;
        else
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "%s, ",
                                  priv_driver_get_sgi_info(
                                      &prHwWlanInfo->rWtblPeerCap) == 0 ?
                                  "LGI" : "SGI");
        i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "%s%s %s\n",
                                      HW_TX_MODE_STR[txmode], stbc ? "STBC" : " ",
                                      priv_driver_get_ldpc_info(&prHwWlanInfo->rWtblTxConfig)
                                      == 0 ? "BCC" : "LDPC");
    }

    return i4BytesWritten;
}

static int32_t priv_driver_dump_helper_wtbl_info(IN char *pcCommand,
                                                 IN int i4TotalLen, struct PARAM_HW_WLAN_INFO *prHwWlanInfo)
{
    uint8_t i;
    int32_t i4BytesWritten = 0;

    if (!pcCommand) {
        DBGLOG(HAL, ERROR, "%s: pcCommand is NULL.\n",
               __func__);
        return i4BytesWritten;
    }

    i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s",
                                 "\n\nwtbl:\n");
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "Dump WTBL info of WLAN_IDX	    = %d\n",
                                  prHwWlanInfo->u4Index);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\tAddr="MACSTR"\n",
                                  MAC2STR(prHwWlanInfo->rWtblTxConfig.aucPA));
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\tMUAR_Idx	 = %d\n",
                                  prHwWlanInfo->rWtblSecConfig.ucMUARIdx);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\trc_a1/rc_a2:%d/%d\n",
                                  prHwWlanInfo->rWtblSecConfig.fgRCA1,
                                  prHwWlanInfo->rWtblSecConfig.fgRCA2);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tKID:%d/RCID:%d/RKV:%d/RV:%d/IKV:%d/WPI_FLAG:%d\n",
                                  prHwWlanInfo->rWtblSecConfig.ucKeyID,
                                  prHwWlanInfo->rWtblSecConfig.fgRCID,
                                  prHwWlanInfo->rWtblSecConfig.fgRKV,
                                  prHwWlanInfo->rWtblSecConfig.fgRV,
                                  prHwWlanInfo->rWtblSecConfig.fgIKV,
                                  prHwWlanInfo->rWtblSecConfig.fgEvenPN);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "%s", "\tGID_SU:NA");
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\tsw/DIS_RHTR:%d/%d\n",
                                  prHwWlanInfo->rWtblTxConfig.fgSW,
                                  prHwWlanInfo->rWtblTxConfig.fgDisRxHdrTran);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tHT/VHT/HT-LDPC/VHT-LDPC/DYN_BW/MMSS:%d/%d/%d/%d/%d/%d\n",
                                  prHwWlanInfo->rWtblTxConfig.fgIsHT,
                                  prHwWlanInfo->rWtblTxConfig.fgIsVHT,
                                  prHwWlanInfo->rWtblTxConfig.fgLDPC,
                                  prHwWlanInfo->rWtblTxConfig.fgVhtLDPC,
                                  prHwWlanInfo->rWtblTxConfig.fgDynBw,
                                  prHwWlanInfo->rWtblPeerCap.ucMMSS);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tFCAP/G2/G4/G8/G16/CBRN:%d/%d/%d/%d/%d/%d\n",
                                  prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability,
                                  prHwWlanInfo->rWtblPeerCap.fgG2,
                                  prHwWlanInfo->rWtblPeerCap.fgG4,
                                  prHwWlanInfo->rWtblPeerCap.fgG8,
                                  prHwWlanInfo->rWtblPeerCap.fgG16,
                                  prHwWlanInfo->rWtblPeerCap.ucChangeBWAfterRateN);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tHT-TxBF(tibf/tebf):%d/%d, VHT-TxBF(tibf/tebf):%d/%d, PFMU_IDX=%d\n",
                                  prHwWlanInfo->rWtblTxConfig.fgTIBF,
                                  prHwWlanInfo->rWtblTxConfig.fgTEBF,
                                  prHwWlanInfo->rWtblTxConfig.fgVhtTIBF,
                                  prHwWlanInfo->rWtblTxConfig.fgVhtTEBF,
                                  prHwWlanInfo->rWtblTxConfig.ucPFMUIdx);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "%s", "\tSPE_IDX=NA\n");
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tBA Enable:0x%x, BAFail Enable:%d\n",
                                  prHwWlanInfo->rWtblBaConfig.ucBaEn,
                                  prHwWlanInfo->rWtblTxConfig.fgBAFEn);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tQoS Enable:%d\n", prHwWlanInfo->rWtblTxConfig.fgIsQoS);
    if (prHwWlanInfo->rWtblTxConfig.fgIsQoS) {
        for (i = 0; i < 8; i += 2) {
            i4BytesWritten += kalSnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\t\tBA WinSize: TID 0 - %d, TID 1 - %d\n",
                                  (uint32_t)
                                  ((prHwWlanInfo->rWtblBaConfig.u4BaWinSize >>
                                    (i * 3)) & BITS(0, 2)),
                                  (uint32_t)
                                  ((prHwWlanInfo->rWtblBaConfig.u4BaWinSize >>
                                    ((i + 1) * 3)) & BITS(0, 2)));
        }
    }

    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\tpartial_aid:%d\n",
                                  prHwWlanInfo->rWtblTxConfig.u2PartialAID);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\twpi_even:%d\n",
                                  prHwWlanInfo->rWtblSecConfig.fgEvenPN);
    i4BytesWritten += scnprintf(pcCommand + i4BytesWritten,
                                i4TotalLen - i4BytesWritten, "\tAAD_OM/CipherSuit:%d/%d\n",
                                prHwWlanInfo->rWtblTxConfig.fgAADOM,
                                prHwWlanInfo->rWtblSecConfig.ucCipherSuit);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\taf:%d\n",
                                  prHwWlanInfo->rWtblPeerCap.ucAmpduFactor);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\trdg_ba:%d/rdg capability:%d\n",
                                  prHwWlanInfo->rWtblTxConfig.fgRdgBA,
                                  prHwWlanInfo->rWtblTxConfig.fgRDG);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\tcipher_suit:%d\n",
                                  prHwWlanInfo->rWtblSecConfig.ucCipherSuit);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\tFromDS:%d\n",
                                  prHwWlanInfo->rWtblTxConfig.fgIsFromDS);
    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "\tToDS:%d\n",
                                  prHwWlanInfo->rWtblTxConfig.fgIsToDS);

    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\tRSSI = %d %d %d %d\n",
                                  RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi0),
                                  RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi1),
                                  RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi2),
                                  RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi3));

    i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "%s", "\tRate Info\n");

    i4BytesWritten += priv_driver_rate_to_string(pcCommand + i4BytesWritten,
                                                 i4TotalLen - i4BytesWritten, 1, prHwWlanInfo);


    return i4BytesWritten;
}

static int priv_driver_get_wtbl_info_default(
    IN struct GLUE_INFO *prGlueInfo,
    IN uint32_t u4Index,
    IN char *pcCommand,
    IN int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    struct PARAM_HW_WLAN_INFO *prHwWlanInfo;

    prHwWlanInfo = (struct PARAM_HW_WLAN_INFO *)kalMemAlloc(
                       sizeof(struct PARAM_HW_WLAN_INFO), VIR_MEM_TYPE);
    if (!prHwWlanInfo)
        return i4BytesWritten;

    prHwWlanInfo->u4Index = u4Index;
    DBGLOG(REQ, INFO, "%s : index = %d\n",
           __func__,
           prHwWlanInfo->u4Index);

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo,
                       prHwWlanInfo,
                       sizeof(struct PARAM_HW_WLAN_INFO),
                       TRUE, TRUE, TRUE, &u4BufLen);

    DBGLOG(REQ, INFO, "rStatus %u u4BufLen = %d\n", rStatus, u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS) {
        kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_HW_WLAN_INFO));
        return i4BytesWritten;
    }
    i4BytesWritten = priv_driver_dump_helper_wtbl_info(
                         pcCommand,
                         i4TotalLen,
                         prHwWlanInfo);

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

    kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
               sizeof(struct PARAM_HW_WLAN_INFO));

    return i4BytesWritten;
}

static int priv_driver_get_wtbl_info(IN struct GLUE_INFO *prGlueInfo,
                                     IN char *pcCommand, IN int i4TotalLen)
{
    int32_t i4Argc = 0;
    int32_t u4Ret = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
    uint32_t u4Index = 0;
    struct CHIP_DBG_OPS *prDbgOps;

    if (GLUE_CHK_PR2(prGlueInfo, pcCommand) == FALSE)
        return -1;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 2)
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Index);

    if (u4Ret)
        return -1;

    prDbgOps = prGlueInfo->prAdapter->chip_info->prDebugOps;

    if (prDbgOps->showWtblInfo)
        return prDbgOps->showWtblInfo(
                   prGlueInfo->prAdapter, u4Index, pcCommand, i4TotalLen);
    else
        return priv_driver_get_wtbl_info_default(
                   prGlueInfo, u4Index, pcCommand, i4TotalLen);
}
#endif /* #if (CFG_SUPPORT_PRIV_WTBL_INFO == 1) */

#if (CFG_SUPPORT_PRIV_SW_CTRL == 1)
int priv_driver_set_sw_ctrl(IN struct GLUE_INFO *prGlueInfo, IN char *pcCommand,
                            IN int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    int32_t u4Ret = 0;

    struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

    if (GLUE_CHK_PR2(prGlueInfo, pcCommand) == FALSE)
        return -1;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 3) {
        /* rSwCtrlInfo.u4Id = kalStrtoul(apcArgv[1], NULL, 0);
         *  rSwCtrlInfo.u4Data = kalStrtoul(apcArgv[2], NULL, 0);
         */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &(rSwCtrlInfo.u4Id));
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n",
                   u4Ret);
        u4Ret = kalkStrtou32(apcArgv[2], 0, &(rSwCtrlInfo.u4Data));
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n",
                   u4Ret);

        rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
                           &rSwCtrlInfo, sizeof(rSwCtrlInfo),
                           FALSE, FALSE, TRUE, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;

    }

    return i4BytesWritten;

}
#endif /* #if (CFG_SUPPORT_PRIV_SW_CTRL == 1) */

#if (CFG_SUPPORT_TWT == 1)
static int priv_driver_set_twtparams(
    struct GLUE_INFO *prGlueInfo,
    char *pcCommand,
    int i4TotalLen)
{
    struct ADAPTER *prAdapter = NULL;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX_LONG] = { 0 };
    struct _TWT_CTRL_T rTWTCtrl;
    struct _TWT_PARAMS_T *prTWTParams;
    uint16_t i;
    int32_t u4Ret = 0;
    uint16_t au2Setting[CMD_TWT_MAX_PARAMS] = { 0 };
    struct _MSG_TWT_PARAMS_SET_T *prTWTParamSetMsg;

    if (GLUE_CHK_PR2(prGlueInfo, pcCommand) == FALSE)
        return -1;

    DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

    prAdapter = prGlueInfo->prAdapter;

    /* Check param number and convert TWT params to integer type */
    if (i4Argc == CMD_TWT_ACTION_TEN_PARAMS ||
        i4Argc == CMD_TWT_ACTION_THREE_PARAMS) {
        for (i = 0; i < (i4Argc - 1); i++) {

            u4Ret = kalkStrtou16(apcArgv[i + 1],
                                 0, &(au2Setting[i]));

            if (u4Ret)
                DBGLOG(REQ, INFO, "Argv error ret=%d\n", u4Ret);
        }
    } else {
        DBGLOG(REQ, INFO, "set_twtparams wrong argc : %d\n", i4Argc);
        return -1;
    }

    if ((IS_TWT_PARAM_ACTION_DEL(au2Setting[0])
#if (CFG_SUPPORT_TWT_SUSPEND_RESUME == 1)
         || IS_TWT_PARAM_ACTION_SUSPEND(au2Setting[0])
         || IS_TWT_PARAM_ACTION_RESUME(au2Setting[0])
#endif /* #if (CFG_SUPPORT_TWT_SUSPEND_RESUME == 1) */
        ) && i4Argc == CMD_TWT_ACTION_THREE_PARAMS) {

        DBGLOG(REQ, INFO, "Action=%d\n", au2Setting[0]);
        DBGLOG(REQ, INFO, "TWT Flow ID=%d\n", au2Setting[1]);

        if (au2Setting[1] >= TWT_MAX_FLOW_NUM) {
            /* Simple sanity check failure */
            DBGLOG(REQ, INFO, "Invalid TWT Params\n");
            return -1;
        }

        rTWTCtrl.ucBssIdx = prGlueInfo->prDevHandler->bss_idx;
        rTWTCtrl.ucCtrlAction = au2Setting[0];
        rTWTCtrl.ucTWTFlowId = au2Setting[1];

    } else if (i4Argc == CMD_TWT_ACTION_TEN_PARAMS) {
        DBGLOG(REQ, INFO, "Action bitmap=%d\n", au2Setting[0]);
        DBGLOG(REQ, INFO,
               "TWT Flow ID=%d Setup Command=%d Trig enabled=%d\n",
               au2Setting[1], au2Setting[2], au2Setting[3]);
        DBGLOG(REQ, INFO,
               "Unannounced enabled=%d Wake Interval Exponent=%d\n",
               au2Setting[4], au2Setting[5]);
        DBGLOG(REQ, INFO, "Protection enabled=%d Duration=%d\n",
               au2Setting[6], au2Setting[7]);
        DBGLOG(REQ, INFO, "Wake Interval Mantissa=%d\n", au2Setting[8]);
        /*
         *  au2Setting[0]: Whether bypassing nego or not
         *  au2Setting[1]: TWT Flow ID
         *  au2Setting[2]: TWT Setup Command
         *  au2Setting[3]: Trigger enabled
         *  au2Setting[4]: Unannounced enabled
         *  au2Setting[5]: TWT Wake Interval Exponent
         *  au2Setting[6]: TWT Protection enabled
         *  au2Setting[7]: Nominal Minimum TWT Wake Duration
         *  au2Setting[8]: TWT Wake Interval Mantissa
         */
        if (au2Setting[1] >= TWT_MAX_FLOW_NUM ||
            au2Setting[2] > TWT_SETUP_CMD_DEMAND ||
            au2Setting[5] > TWT_MAX_WAKE_INTVAL_EXP) {
            /* Simple sanity check failure */
            DBGLOG(REQ, INFO, "Invalid TWT Params\n");
            return -1;
        }

        prTWTParams = &(rTWTCtrl.rTWTParams);
        kalMemSet(prTWTParams, 0, sizeof(struct _TWT_PARAMS_T));
        prTWTParams->fgReq = TRUE;
        prTWTParams->ucSetupCmd = (uint8_t) au2Setting[2];
        prTWTParams->fgTrigger = (au2Setting[3]) ? TRUE : FALSE;
        prTWTParams->fgUnannounced = (au2Setting[4]) ? TRUE : FALSE;
        prTWTParams->ucWakeIntvalExponent = (uint8_t) au2Setting[5];
        prTWTParams->fgProtect = (au2Setting[6]) ? TRUE : FALSE;
        prTWTParams->ucMinWakeDur = (uint8_t) au2Setting[7];
        prTWTParams->u2WakeIntvalMantiss = au2Setting[8];

        rTWTCtrl.ucBssIdx = prGlueInfo->prDevHandler->bss_idx;
        rTWTCtrl.ucCtrlAction = au2Setting[0];
        rTWTCtrl.ucTWTFlowId = au2Setting[1];
    } else {
        DBGLOG(REQ, INFO, "wrong argc for update agrt: %d\n", i4Argc);
        return -1;
    }

    prTWTParamSetMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                   sizeof(struct _MSG_TWT_REQFSM_RESUME_T));
    if (prTWTParamSetMsg) {
        prTWTParamSetMsg->rMsgHdr.eMsgId =
            MID_TWT_PARAMS_SET;
        kalMemCopy(&prTWTParamSetMsg->rTWTCtrl,
                   &rTWTCtrl, sizeof(rTWTCtrl));

        mboxSendMsg(prAdapter, MBOX_ID_0,
                    (struct MSG_HDR *) prTWTParamSetMsg,
                    MSG_SEND_METHOD_BUF);
    } else
        return -1;

    return 0;
}
#endif /* #if (CFG_SUPPORT_TWT == 1) */

#if (CFG_SUPPORT_PRIV_SUSPEND_MODE == 1)
int priv_driver_set_suspend_mode(IN struct GLUE_INFO *prGlueInfo,
                                 IN char *pcCommand, IN int i4TotalLen)
{
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    u_int8_t fgEnable;
    uint32_t u4Enable = 0;
    int32_t u4Ret = 0;

    ASSERT(prGlueInfo);
    if (GLUE_CHK_PR2(prGlueInfo, pcCommand) == FALSE)
        return -1;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 2) {
        /* fgEnable = (kalStrtoul(apcArgv[1], NULL, 0) == 1) ? TRUE :
         *            FALSE;
         */
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Enable);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse u4Enable error u4Ret=%d\n",
                   u4Ret);
        if (u4Enable == 1)
            fgEnable = TRUE;
        else
            fgEnable = FALSE;

        if (prGlueInfo->fgIsInSuspendMode == fgEnable) {
            DBGLOG(REQ, INFO,
                   "%s: Already in suspend mode [%u], SKIP!\n",
                   __func__, fgEnable);
            return 0;
        }

        DBGLOG(REQ, INFO, "%s: Set suspend mode [%u]\n", __func__,
               fgEnable);

        prGlueInfo->fgIsInSuspendMode = fgEnable;

        wlanSetSuspendMode(prGlueInfo, fgEnable);
#if CFG_ENABLE_WIFI_DIRECT
        p2pSetSuspendMode(prGlueInfo, fgEnable);
#endif /* #if CFG_ENABLE_WIFI_DIRECT */
    }

    return 0;
}
#endif /* #if (CFG_SUPPORT_PRIV_SUSPEND_MODE == 1) */

#if CFG_SUPPORT_SINGLE_SKU || CFG_SUPPORT_SINGLE_SKU_LOCAL_DB
int priv_driver_set_country(IN struct GLUE_INFO *prGlueInfo,
                            IN char *pcCommand, IN int i4TotalLen)
{

    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint8_t aucCountry[2];

    DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);

    if (regd_is_single_sku_en()) {
        uint8_t aucCountry_code[4] = {0, 0, 0, 0};
        uint8_t i, count;

        /* command like "COUNTRY US", "COUNTRY US1" and
         * "COUNTRY US01"
         */
        count = kalStrnLen((char *)apcArgv[1], sizeof(aucCountry_code));
        for (i = 0; i < count; i++)
            aucCountry_code[i] = apcArgv[1][i];


        rStatus = kalIoctl(prGlueInfo, wlanoidSetCountryCode,
                           &aucCountry_code[0], count,
                           FALSE, FALSE, TRUE, &u4BufLen);
        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;

        return 0;
    }


    if (i4Argc >= 2) {
        /* command like "COUNTRY US", "COUNTRY EU" and "COUNTRY JP" */
        aucCountry[0] = apcArgv[1][0];
        aucCountry[1] = apcArgv[1][1];

        rStatus = kalIoctl(prGlueInfo, wlanoidSetCountryCode,
                           &aucCountry[0], 2, FALSE, FALSE, TRUE,
                           &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;
    }
    return 0;
}
#endif /* #if CFG_SUPPORT_SINGLE_SKU || CFG_SUPPORT_SINGLE_SKU_LOCAL_DB */

int priv_driver_get_country(IN struct GLUE_INFO *prGlueInfo,
                            IN char *pcCommand, IN int i4TotalLen)
{
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint32_t i4BytesWritten = 0;
    uint32_t country = 0;
    char acCountryStr[MAX_COUNTRY_CODE_LEN + 1] = {0};


    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (!regd_is_single_sku_en()) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "Not Supported.");
        return i4BytesWritten;
    }

    country = rlmDomainGetCountryCode();
    rlmDomainU32ToAlpha(country, acCountryStr);

    LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
           "\nCountry Code: %s (0x%x)", acCountryStr, country);

    return  i4BytesWritten;
}

int priv_driver_get_channels(IN struct GLUE_INFO *prGlueInfo,
                             IN char *pcCommand, IN int i4TotalLen)
{
    uint32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
#if CFG_SUPPORT_SINGLE_SKU || CFG_SUPPORT_SINGLE_SKU_LOCAL_DB
    uint32_t ch_idx, start_idx, end_idx;
    struct CMD_DOMAIN_CHANNEL *pCh;
    uint32_t ch_num = 0;
    uint8_t maxbw = 160;
#endif /* #if CFG_SUPPORT_SINGLE_SKU || CFG_SUPPORT_SINGLE_SKU_LOCAL_DB */
    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (!regd_is_single_sku_en()) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "Not Supported.");
        return i4BytesWritten;
    }

#if CFG_SUPPORT_SINGLE_SKU || CFG_SUPPORT_SINGLE_SKU_LOCAL_DB
    /**
     * Usage: iwpriv wlan0 driver "get_channels [2g |5g |ch_num]"
     **/
    if (i4Argc >= 2 && (apcArgv[1][0] == '2') && (apcArgv[1][1] == 'g')) {
        start_idx = 0;
        end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
    } else if (i4Argc >= 2 && (apcArgv[1][0] == '5') &&
               (apcArgv[1][1] == 'g')) {
        start_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
        end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
                  + rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ);
    } else {
        start_idx = 0;
        end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
                  + rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ);
        if (i4Argc >= 2)
            /* Dump only specified channel */
            kalkStrtou32(apcArgv[1], 0, &ch_num);
    }

    if (regd_is_single_sku_en()) {
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\n");

        for (ch_idx = start_idx; ch_idx < end_idx; ch_idx++) {

            pCh = (rlmDomainGetActiveChannels() + ch_idx);
            maxbw = 160;

            if (ch_num && (ch_num != pCh->u2ChNum))
                continue; /*show specific channel information*/

            /* Channel number */
            LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "CH-%d:",
                   pCh->u2ChNum);

            /* Active/Passive */
            if (pCh->eFlags & IEEE80211_CHAN_PASSIVE_FLAG) {
                /* passive channel */
                LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                       " " IEEE80211_CHAN_PASSIVE_STR);
            } else
                LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                       " ACTIVE");

            /* Max BW */
            if ((pCh->eFlags & IEEE80211_CHAN_NO_160MHZ) ==
                IEEE80211_CHAN_NO_160MHZ)
                maxbw = 80;
            if ((pCh->eFlags & IEEE80211_CHAN_NO_80MHZ) ==
                IEEE80211_CHAN_NO_80MHZ)
                maxbw = 40;
            if ((pCh->eFlags & IEEE80211_CHAN_NO_HT40) ==
                IEEE80211_CHAN_NO_HT40)
                maxbw = 20;
            LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                   " BW_%dMHz", maxbw);

            /* Channel flags */
            if (pCh->eFlags & IEEE80211_CHAN_RADAR)
                LOGBUF(pcCommand, i4TotalLen,
                       i4BytesWritten, ", DFS");
            LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
                   "  (flags=0x%x)\n", pCh->eFlags);
        }
    }
#endif /* #if CFG_SUPPORT_SINGLE_SKU || CFG_SUPPORT_SINGLE_SKU_LOCAL_DB */

    return i4BytesWritten;
}

#ifdef MTK_SIGMA_ENABLE
int priv_driver_set_nss(IN struct GLUE_INFO *prGlueInfo, IN char *pcCommand,
                        IN int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
    uint32_t u4Ret, u4Parse = 0;
    uint8_t ucNSS;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc == 2) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);

        ucNSS = (uint8_t) u4Parse;
        prGlueInfo->prAdapter->rWifiVar.ucNSS = ucNSS;
        DBGLOG(REQ, LOUD, "ucNSS = %d\n", ucNSS);
    } else {
        DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_NSS <nss>\n");
    }

    return i4BytesWritten;
}

#if (CFG_SUPPORT_802_11AX == 1)
int priv_driver_set_mcsmap(IN struct GLUE_INFO *prGlueInfo, IN char *pcCommand,
                           IN int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
    uint32_t u4Ret, u4Parse = 0;
    uint8_t ucTxMcsMap;
    struct ADAPTER *prAdapter = NULL;
    /*UINT_8 ucBssIndex;*/
    /*P_BSS_INFO_T prBssInfo;*/

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc == 2) {

        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);

        ucTxMcsMap = (uint8_t) u4Parse;
        if (ucTxMcsMap <= 2) {
            prAdapter = prGlueInfo->prAdapter;
            prAdapter->ucMcsMapSetFromSigma = ucTxMcsMap;

            DBGLOG(REQ, ERROR, "ucMcsMapSetFromSigma = %d\n",
                   prGlueInfo->prAdapter->ucMcsMapSetFromSigma);

            prGlueInfo->prAdapter->fgMcsMapBeenSet = TRUE;
        } else {
            prGlueInfo->prAdapter->fgMcsMapBeenSet = FALSE;
        }
    } else {
        DBGLOG(INIT, ERROR, "iwpriv wlan0 driver SET_TX_MCSMAP <en>\n");
        DBGLOG(INIT, ERROR, "<en> 1: enable. 0: disable.\n");
    }

    return i4BytesWritten;
}

/* This command is for sigma to disable TxPPDU. */
int priv_driver_set_tx_ppdu(IN struct GLUE_INFO *prGlueInfo, IN char *pcCommand,
                            IN int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
    uint32_t u4Ret, u4Parse = 0;
    struct ADAPTER *prAdapter = NULL;
    struct STA_RECORD *prStaRec;

    prAdapter = prGlueInfo->prAdapter;
    prStaRec = cnmGetStaRecByIndex(prAdapter, 0);

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc == 2) {

        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);

        if (u4Parse) {
            /* HE_SU is allowed. */
            prAdapter->fgTxPPDU = TRUE;
            NIC_TX_PPDU_ENABLE(prAdapter);
        } else {
            /* HE_SU is not allowed. */
            prAdapter->fgTxPPDU = FALSE;
            if (prStaRec && prStaRec->fgIsTxAllowed)
                NIC_TX_PPDU_DISABLE(prAdapter);
        }

        DBGLOG(REQ, STATE, "fgTxPPDU is %d\n", prAdapter->fgTxPPDU);
    } else {
        DBGLOG(INIT, ERROR, "iwpriv wlanXX driver TX_PPDU\n");
        DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
    }

    return i4BytesWritten;
}

int priv_driver_set_ba_size(IN struct GLUE_INFO *prGlueInfo, IN char *pcCommand,
                            IN int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
    uint32_t u4Ret, u4Parse = 0;
    uint16_t u2HeBaSize;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc == 2) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n",
                   u4Ret);

        u2HeBaSize = (uint16_t) u4Parse;

        prGlueInfo->prAdapter->rWifiVar.u2TxHeBaSize = u2HeBaSize;
        prGlueInfo->prAdapter->rWifiVar.u2RxHeBaSize = u2HeBaSize;
    } else {
        DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_BA_SIZE\n");
        DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
    }

    return i4BytesWritten;
}
#endif /* #if (CFG_SUPPORT_802_11AX == 1) */
#endif /* #ifdef MTK_SIGMA_ENABLE */

#if defined(_HIF_AXI) && (CFG_SUPPORT_MANUAL_OWN_CTRL == 1)
static int priv_driver_set_fwown(struct GLUE_INFO *prGlueInfo,
                                 IN char *pcCommand, IN int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint32_t u4Ret;
    int32_t i4ArgNum = 2;
    uint32_t u4FwOwn = 0;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {

        u4Ret = kalkStrtou32((char *)apcArgv[1], 0, &(u4FwOwn));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "parse set_fw_own error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        DBGLOG(REQ, LOUD, "command is %s Fw own\n",
               (u4FwOwn == 0 ? "CLR" :
                (u4FwOwn == 1 ? "SET" : "DIS Manual")));

        rStatus = kalIoctl(prGlueInfo, wlanoidSetClrFwOwn,
                           &u4FwOwn, sizeof(u4FwOwn),
                           FALSE, FALSE, TRUE, &u4BufLen);

        DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
        if (rStatus != WLAN_STATUS_SUCCESS)
            return -1;

        i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
                                     "fgIsFwOwn: %d",
                                     (unsigned int)prGlueInfo->prAdapter->fgIsFwOwn);
        DBGLOG(REQ, INFO, "%s: command result is %s.\n", __func__,
               pcCommand);
    }

    return i4BytesWritten;
}

static int priv_driver_get_fwown(struct GLUE_INFO *prGlueInfo,
                                 IN char *pcCommand, IN int i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    int32_t i4ArgNum = 1;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= i4ArgNum) {

        DBGLOG(REQ, LOUD, "command is Get Fw own\n");

        i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen,
                                     "IsFwOwn: %d",
                                     (unsigned int)prGlueInfo->prAdapter->fgIsFwOwn);
        DBGLOG(REQ, INFO, "%s: command result is %s.\n", __func__,
               pcCommand);
    }

    return i4BytesWritten;
}
#endif /* #if defined(_HIF_AXI) && (CFG_SUPPORT_MANUAL_OWN_CTRL == 1) */

#if CFG_WOW_SUPPORT
static int priv_driver_set_wow(IN struct GLUE_INFO *prGlueInfo,
                               IN char *pcCommand, IN int i4TotalLen)
{
    struct WOW_CTRL *pWOW_CTRL = NULL;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    uint32_t u4Ret = 0;
    uint32_t Enable = 0;

    if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
        return -1;

    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou32(apcArgv[1], 0, &Enable);

    if (u4Ret)
        DBGLOG(REQ, LOUD, "parse bEnable error u4Ret=%d\n", u4Ret);

    DBGLOG(INIT, INFO, "CMD set_wow_enable = %d\n", Enable);
    DBGLOG(INIT, INFO, "Scenario ID %d\n", pWOW_CTRL->ucScenarioId);
    DBGLOG(INIT, INFO, "ucBlockCount %d\n", pWOW_CTRL->ucBlockCount);
    DBGLOG(INIT, INFO, "interface %d\n",
           pWOW_CTRL->astWakeHif[0].ucWakeupHif);
    DBGLOG(INIT, INFO, "gpio_pin %d\n",
           pWOW_CTRL->astWakeHif[0].ucGpioPin);
    DBGLOG(INIT, INFO, "gpio_level 0x%x\n",
           pWOW_CTRL->astWakeHif[0].ucTriggerLvl);
    DBGLOG(INIT, INFO, "gpio_timer %d\n",
           pWOW_CTRL->astWakeHif[0].u4GpioInterval);
    /* kalWowProcess(prGlueInfo, Enable); */
    prGlueInfo->fgIsInSuspendMode = Enable;
    if (Enable == TRUE)
        wlanSuspendPmHandle(prGlueInfo);
    else
        wlanResumePmHandle(prGlueInfo);

    return 0;
}

static int priv_driver_set_wow_enable(IN struct GLUE_INFO *prGlueInfo,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    struct WOW_CTRL *pWOW_CTRL = NULL;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    uint32_t u4Ret = 0;
    uint8_t ucEnable = 0;

    if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
        return -1;

    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou8(apcArgv[1], 0, &ucEnable);

    if (u4Ret)
        DBGLOG(REQ, LOUD, "parse bEnable error u4Ret=%d\n", u4Ret);

    pWOW_CTRL->fgWowEnable = ucEnable;

    DBGLOG(PF, INFO, "WOW enable %d\n", pWOW_CTRL->fgWowEnable);

    return 0;
}

static int priv_driver_set_wow_par(IN struct GLUE_INFO *prGlueInfo,
                                   IN char *pcCommand, IN int i4TotalLen)
{

    struct WOW_CTRL *pWOW_CTRL = NULL;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    int32_t u4Ret = 0;
    uint8_t ucWakeupHif = 0, GpioPin = 0, ucGpioLevel = 0, ucBlockCount = 0,
            ucScenario = 0;
    uint32_t u4GpioTimer = 0;

    if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
        return -1;

    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc > 3) {

        u4Ret = kalkStrtou8(apcArgv[1], 0, &ucWakeupHif);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
                   u4Ret);
        pWOW_CTRL->astWakeHif[0].ucWakeupHif = ucWakeupHif;

        u4Ret = kalkStrtou8(apcArgv[2], 0, &GpioPin);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse GpioPin error u4Ret=%d\n",
                   u4Ret);
        pWOW_CTRL->astWakeHif[0].ucGpioPin = GpioPin;

        u4Ret = kalkStrtou8(apcArgv[3], 0, &ucGpioLevel);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse Gpio level error u4Ret=%d\n",
                   u4Ret);
        pWOW_CTRL->astWakeHif[0].ucTriggerLvl = ucGpioLevel;

        u4Ret = kalkStrtou32(apcArgv[4], 0, &u4GpioTimer);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse u4GpioTimer error u4Ret=%d\n",
                   u4Ret);
        pWOW_CTRL->astWakeHif[0].u4GpioInterval = u4GpioTimer;

        u4Ret = kalkStrtou8(apcArgv[5], 0, &ucScenario);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse ucScenario error u4Ret=%d\n",
                   u4Ret);
        pWOW_CTRL->ucScenarioId = ucScenario;

        u4Ret = kalkStrtou8(apcArgv[6], 0, &ucBlockCount);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse ucBlockCnt error u4Ret=%d\n",
                   u4Ret);
        pWOW_CTRL->ucBlockCount = ucBlockCount;

        DBGLOG(INIT, INFO, "gpio_scenario%d\n",
               pWOW_CTRL->ucScenarioId);
        DBGLOG(INIT, INFO, "interface %d\n",
               pWOW_CTRL->astWakeHif[0].ucWakeupHif);
        DBGLOG(INIT, INFO, "gpio_pin %d\n",
               pWOW_CTRL->astWakeHif[0].ucGpioPin);
        DBGLOG(INIT, INFO, "gpio_level %d\n",
               pWOW_CTRL->astWakeHif[0].ucTriggerLvl);
        DBGLOG(INIT, INFO, "gpio_timer %d\n",
               pWOW_CTRL->astWakeHif[0].u4GpioInterval);

        return 0;
    } else
        return -1;


}

static int priv_driver_set_wow_udpport(IN struct GLUE_INFO *prGlueInfo,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    struct WOW_CTRL *pWOW_CTRL = NULL;
    int32_t i4Argc = 0;
    int8_t *apcPortArgv[WLAN_CFG_ARGV_MAX_LONG] = { 0 };
    /* to input 20 port */
    int32_t u4Ret = 0, ii = 0;
    uint8_t ucVer = 0, ucCount = 0;
    uint16_t u2Port = 0;
    uint16_t *pausPortArry = NULL;

    if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
        return -1;

    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgumentLong(pcCommand, &i4Argc, apcPortArgv);
    DBGLOG(REQ, WARN, "argc is %i\n", i4Argc);

    /* example: set_wow_udp 0 5353,8080 (set) */
    /* example: set_wow_udp 1 (clear) */

    if (i4Argc >= 3) {

        /* Pick Max */
        ucCount = ((i4Argc - 2) > MAX_TCP_UDP_PORT) ? MAX_TCP_UDP_PORT :
                  (i4Argc - 2);
        DBGLOG(PF, INFO, "UDP ucCount=%d\n", ucCount);

        u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        /* IPv4/IPv6 */
        DBGLOG(PF, INFO, "ucVer=%d\n", ucVer);
        if (ucVer == 0) {
            pWOW_CTRL->stWowPort.ucIPv4UdpPortCnt = ucCount;
            pausPortArry = pWOW_CTRL->stWowPort.ausIPv4UdpPort;
        } else {
            pWOW_CTRL->stWowPort.ucIPv6UdpPortCnt = ucCount;
            pausPortArry = pWOW_CTRL->stWowPort.ausIPv6UdpPort;
        }

        /* Port */
        for (ii = 0; ii < ucCount; ii++) {
            u4Ret = kalkStrtou16(apcPortArgv[ii + 2], 0, &u2Port);
            if (u4Ret) {
                DBGLOG(PF, ERROR,
                       "parse u2Port error u4Ret=%d\n", u4Ret);
                return -1;
            }

            pausPortArry[ii] = u2Port;
            DBGLOG(PF, INFO, "ucPort=%d, idx=%d\n", u2Port, ii);
        }

        return 0;
    } else if (i4Argc == 2) {

        u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        if (ucVer == 0) {
            kalMemZero(prGlueInfo->prAdapter->rWowCtrl.stWowPort
                       .ausIPv4UdpPort,
                       sizeof(uint16_t) * MAX_TCP_UDP_PORT);
            prGlueInfo->prAdapter->rWowCtrl.stWowPort
            .ucIPv4UdpPortCnt = 0;
        } else {
            kalMemZero(prGlueInfo->prAdapter->rWowCtrl.stWowPort
                       .ausIPv6UdpPort,
                       sizeof(uint16_t) * MAX_TCP_UDP_PORT);
            prGlueInfo->prAdapter->rWowCtrl.stWowPort
            .ucIPv6UdpPortCnt = 0;
        }

        return 0;
    } else
        return -1;

}

static int priv_driver_set_wow_tcpport(IN struct GLUE_INFO *prGlueInfo,
                                       IN char *pcCommand, IN int i4TotalLen)
{
    struct WOW_CTRL *pWOW_CTRL = NULL;
    int32_t i4Argc = 0;
    int8_t *apcPortArgv[WLAN_CFG_ARGV_MAX_LONG] = { 0 };
    /* to input 20 port */
    int32_t u4Ret = 0, ii = 0;
    uint8_t ucVer = 0, ucCount = 0;
    uint16_t u2Port = 0;
    uint16_t *pausPortArry = NULL;

    if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
        return -1;

    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgumentLong(pcCommand, &i4Argc, apcPortArgv);
    DBGLOG(REQ, WARN, "argc is %i\n", i4Argc);

    /* example: set_wow_tcp 0 5353,8080 (Set) */
    /* example: set_wow_tcp 1 (clear) */

    if (i4Argc >= 3) {

        /* Pick Max */
        ucCount = ((i4Argc - 2) > MAX_TCP_UDP_PORT) ? MAX_TCP_UDP_PORT :
                  (i4Argc - 2);
        DBGLOG(PF, INFO, "TCP ucCount=%d\n", ucCount);

        u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        /* IPv4/IPv6 */
        DBGLOG(PF, INFO, "Ver=%d\n", ucVer);
        if (ucVer == 0) {
            pWOW_CTRL->stWowPort.ucIPv4TcpPortCnt = ucCount;
            pausPortArry = pWOW_CTRL->stWowPort.ausIPv4TcpPort;
        } else {
            pWOW_CTRL->stWowPort.ucIPv6TcpPortCnt = ucCount;
            pausPortArry = pWOW_CTRL->stWowPort.ausIPv6TcpPort;
        }

        /* Port */
        for (ii = 0; ii < ucCount; ii++) {
            u4Ret = kalkStrtou16(apcPortArgv[ii + 2], 0, &u2Port);
            if (u4Ret) {
                DBGLOG(PF, ERROR,
                       "parse u2Port error u4Ret=%d\n", u4Ret);
                return -1;
            }

            pausPortArry[ii] = u2Port;
            DBGLOG(PF, INFO, "ucPort=%d, idx=%d\n", u2Port, ii);
        }

        return 0;
    } else if (i4Argc == 2) {

        u4Ret = kalkStrtou8(apcPortArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        if (ucVer == 0) {
            kalMemZero(
                prGlueInfo->prAdapter->rWowCtrl.stWowPort
                .ausIPv4TcpPort,
                sizeof(uint16_t) * MAX_TCP_UDP_PORT);
            prGlueInfo->prAdapter->rWowCtrl.stWowPort
            .ucIPv4TcpPortCnt = 0;
        } else {
            kalMemZero(
                prGlueInfo->prAdapter->rWowCtrl.stWowPort
                .ausIPv6TcpPort,
                sizeof(uint16_t) * MAX_TCP_UDP_PORT);
            prGlueInfo->prAdapter->rWowCtrl.stWowPort
            .ucIPv6TcpPortCnt = 0;
        }

        return 0;
    } else
        return -1;

}

static int priv_driver_get_wow_port(IN struct GLUE_INFO *prGlueInfo,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    struct WOW_CTRL *pWOW_CTRL = NULL;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    int32_t u4Ret = 0, ii;
    uint8_t ucVer = 0, ucProto = 0;
    uint16_t ucCount;
    uint16_t *pausPortArry;
    char  *aucIp[2] = {"IPv4", "IPv6"};
    char  *aucProto[2] = {"UDP", "TCP"};

    if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
        return -1;

    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    /* example: get_wow_port 0 0 (ipv4-udp) */
    /* example: get_wow_port 0 1 (ipv4-tcp) */
    /* example: get_wow_port 1 0 (ipv6-udp) */
    /* example: get_wow_port 1 1 (ipv6-tcp) */

    if (i4Argc >= 3) {

        /* 0=IPv4, 1=IPv6 */
        u4Ret = kalkStrtou8(apcArgv[1], 0, &ucVer);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse argc[1] error u4Ret=%d\n",
                   u4Ret);

        /* 0=UDP, 1=TCP */
        u4Ret = kalkStrtou8(apcArgv[2], 0, &ucProto);
        if (u4Ret)
            DBGLOG(REQ, LOUD, "parse argc[2] error u4Ret=%d\n",
                   u4Ret);

        if (ucVer > 1)
            ucVer = 0;

        if (ucProto > 1)
            ucProto = 0;

        if (ucVer == 0) {
            if (ucProto == 0) {
                /* IPv4/UDP */
                ucCount = pWOW_CTRL->stWowPort.ucIPv4UdpPortCnt;
                pausPortArry =
                    pWOW_CTRL->stWowPort.ausIPv4UdpPort;
            } else {
                /* IPv4/TCP */
                ucCount = pWOW_CTRL->stWowPort.ucIPv4TcpPortCnt;
                pausPortArry =
                    pWOW_CTRL->stWowPort.ausIPv4TcpPort;
            }
        } else {
            if (ucProto == 0) {
                /* IPv6/UDP */
                ucCount = pWOW_CTRL->stWowPort.ucIPv6UdpPortCnt;
                pausPortArry =
                    pWOW_CTRL->stWowPort.ausIPv6UdpPort;
            } else {
                /* IPv6/TCP */
                ucCount = pWOW_CTRL->stWowPort.ucIPv6TcpPortCnt;
                pausPortArry =
                    pWOW_CTRL->stWowPort.ausIPv6TcpPort;
            }
        }

        /* Dunp Port */
        for (ii = 0; ii < ucCount; ii++)
            DBGLOG(PF, INFO, "ucPort=%d, idx=%d\n",
                   pausPortArry[ii], ii);


        DBGLOG(PF, INFO, "[%s/%s] count:%d\n", aucIp[ucVer],
               aucProto[ucProto], ucCount);

        return 0;
    } else
        return -1;

}

static int priv_driver_get_wow_reason(IN struct GLUE_INFO *prGlueInfo,
                                      IN char *pcCommand, IN int i4TotalLen)
{
    int32_t i4Argc = 0;
    int32_t i4BytesWritten = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    struct WOW_CTRL *pWOW_CTRL = NULL;

    if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
        return -1;

    pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (pWOW_CTRL->ucReason != INVALID_WOW_WAKE_UP_REASON)
        LOGBUF(pcCommand, i4TotalLen, i4BytesWritten,
               "\nwakeup_reason:%d", pWOW_CTRL->ucReason);

    return i4BytesWritten;
}
#endif /* #if CFG_WOW_SUPPORT */
static int priv_driver_set_adv_pws(IN struct GLUE_INFO *prGlueInfo,
                                   IN char *pcCommand, IN int i4TotalLen)
{
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    uint32_t u4Ret = 0;
    uint8_t ucAdvPws = 0;

    if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
        return -1;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    u4Ret = kalkStrtou8(apcArgv[1], 0, &ucAdvPws);

    if (u4Ret)
        DBGLOG(REQ, LOUD, "parse bEnable error u4Ret=%d\n",
               u4Ret);

    prGlueInfo->prAdapter->rWifiVar.ucAdvPws = ucAdvPws;

    DBGLOG(INIT, INFO, "AdvPws:%d\n",
           prGlueInfo->prAdapter->rWifiVar.ucAdvPws);

    return 0;

}

static int priv_driver_set_mdtim(IN struct GLUE_INFO *prGlueInfo,
                                 IN char *pcCommand, IN int i4TotalLen)
{
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    uint32_t u4Ret = 0;
    uint8_t ucMultiDtim = 0, ucVer = 0;

    if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
        return -1;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    /* iwpriv wlan0 driver "set_mdtim 1 3 */
    if (i4Argc >= 3) {

        u4Ret = kalkStrtou8(apcArgv[1], 0, &ucVer);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse apcArgv1 error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        u4Ret = kalkStrtou8(apcArgv[2], 0, &ucMultiDtim);
        if (u4Ret) {
            DBGLOG(REQ, ERROR, "parse apcArgv2 error u4Ret=%d\n",
                   u4Ret);
            return -1;
        }

        if (ucVer == 0) {
            prGlueInfo->prAdapter->rWifiVar.ucWowOnMdtim =
                ucMultiDtim;
            DBGLOG(REQ, INFO, "WOW On MDTIM:%d\n",
                   prGlueInfo->prAdapter->rWifiVar.ucWowOnMdtim);
        } else {
            prGlueInfo->prAdapter->rWifiVar.ucWowOffMdtim =
                ucMultiDtim;
            DBGLOG(REQ, INFO, "WOW Off MDTIM:%d\n",
                   prGlueInfo->prAdapter->rWifiVar.ucWowOffMdtim);
        }
    }

    return 0;

}

static int priv_driver_get_que_info(IN struct GLUE_INFO *prGlueInfo,
                                    IN char *pcCommand, IN int i4TotalLen)
{
    return qmDumpQueueStatus(prGlueInfo->prAdapter,
                             (uint8_t *)pcCommand, i4TotalLen);
}

#if CFG_SUPPORT_ADVANCE_CONTROL
static int priv_driver_set_noise(struct GLUE_INFO *prGlueInfo,
                                 char *pcCommand, int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    int32_t u4Ret = 0;
    uint32_t u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + 1;
    uint32_t u4Sel = 0;
    struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    rSwCtrlInfo.u4Id = u4Id;

    if (i4Argc <= 1) {
        DBGLOG(REQ, ERROR, "Argc(%d) ERR: SET_NOISE <Sel>\n", i4Argc);
        return -1;
    }

    u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Sel);
    if (u4Ret)
        DBGLOG(REQ, ERROR, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);

    rSwCtrlInfo.u4Data = u4Sel << 30;
    DBGLOG(REQ, LOUD, "u4Sel=%d u4Data=0x%x,\n", u4Sel, rSwCtrlInfo.u4Data);
    rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)\n", rStatus);
        return -1;
    }

    return i4BytesWritten;

}

static int priv_driver_get_noise(struct GLUE_INFO *prGlueInfo,
                                 char *pcCommand, int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    uint32_t u4Id = CMD_SW_DBGCTL_ADVCTL_GET_ID + 1;
    uint32_t u4Offset = 0;
    struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;
    int16_t u2Wf0AvgPwr, u2Wf1AvgPwr;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    rSwCtrlInfo.u4Data = 0;
    rSwCtrlInfo.u4Id = u4Id;

    rStatus = kalIoctl(prGlueInfo, wlanoidQuerySwCtrlRead, &rSwCtrlInfo,
                       sizeof(rSwCtrlInfo), TRUE, TRUE, TRUE, &u4BufLen);

    DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
    if (rStatus != WLAN_STATUS_SUCCESS)
        return -1;

    u2Wf0AvgPwr = rSwCtrlInfo.u4Data & 0xFFFF;
    u2Wf1AvgPwr = (rSwCtrlInfo.u4Data >> 16) & 0xFFFF;

    u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
                         "Noise Idle Avg. Power: WF0:%ddB WF1:%ddB\n",
                         u2Wf0AvgPwr, u2Wf1AvgPwr);

    i4BytesWritten = (int32_t)u4Offset;

    return i4BytesWritten;

}               /* priv_driver_get_sw_ctrl */
#endif /* #if CFG_SUPPORT_ADVANCE_CONTROL */

#if (CFG_WIFI_GET_MCS_INFO == 1)
static int priv_driver_last_sec_mcs_info(struct ADAPTER *prAdapter,
                                         char *pcCommand, int i4TotalLen, struct PARAM_TX_MCS_INFO *prTxMcsInfo)
{
    uint8_t ucBssIdx = 0, i = 0, j = 0, ucCnt = 0, ucPerSum = 0;
#if (CFG_SUPPORT_CONNAC2X == 1)
    uint8_t dcm = 0, ersu106t = 0;
#endif /* #if (CFG_SUPPORT_CONNAC2X == 1) */
    uint16_t u2RateCode = 0;
    uint32_t au4RxV0[MCS_INFO_SAMPLE_CNT], au4RxV1[MCS_INFO_SAMPLE_CNT],
             au4RxV2[MCS_INFO_SAMPLE_CNT];
    uint32_t txmode = 0, rate = 0, frmode = 0, sgi = 0,
             nsts = 0, ldpc = 0, stbc = 0, groupid = 0, mu = 0, bw = 0;
    uint32_t u4RxV0 = 0, u4RxV1 = 0, u4RxV2 = 0;
    int32_t i4BytesWritten = 0;
    struct STA_RECORD *prStaRec = NULL;
    struct CHIP_DBG_OPS *prChipDbg;

    ucBssIdx = GET_IOCTL_BSSIDX(prAdapter);
    prStaRec = aisGetTargetStaRec(prAdapter, ucBssIdx);

    if (prStaRec != NULL && prStaRec->fgIsValid && prStaRec->fgIsInUse) {
        kalMemCopy(au4RxV0, prStaRec->au4RxV0, sizeof(au4RxV0));
        kalMemCopy(au4RxV1, prStaRec->au4RxV1, sizeof(au4RxV1));
        kalMemCopy(au4RxV2, prStaRec->au4RxV2, sizeof(au4RxV2));
    } else {
        i4BytesWritten += kalScnprintf(
                              pcCommand + i4BytesWritten,
                              i4TotalLen - i4BytesWritten,
                              "Not Connect to AP\n");
        return i4BytesWritten;
    }


    /* Output the TX MCS Info */
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten, "\nTx MCS:\n");

    for (i = 0; i < MCS_INFO_SAMPLE_CNT; i++) {
        if (prTxMcsInfo->au2TxRateCode[i] == 0xFFFF)
            continue;

        txmode = HW_TX_RATE_TO_MODE(prTxMcsInfo->au2TxRateCode[i]);
        if (txmode >= ENUM_TX_MODE_NUM)
            txmode = ENUM_TX_MODE_NUM - 1;

        rate = HW_TX_RATE_TO_MCS(prTxMcsInfo->au2TxRateCode[i]);
        nsts = HW_TX_RATE_TO_NSS(prTxMcsInfo->au2TxRateCode[i]) + 1;
        stbc = HW_TX_RATE_TO_STBC(prTxMcsInfo->au2TxRateCode[i]);
        bw = prTxMcsInfo->aucTxBw[i];
        sgi = prTxMcsInfo->aucTxSgi[i];
        ldpc = prTxMcsInfo->aucTxLdpc[i];

#if (CFG_SUPPORT_CONNAC2X == 1)
        dcm = HW_TX_RATE_TO_DCM(prTxMcsInfo->au2TxRateCode[i]);
        ersu106t = HW_TX_RATE_TO_106T(prTxMcsInfo->au2TxRateCode[i]);

        if (dcm)
            rate = CONNAC2X_HW_TX_RATE_UNMASK_DCM(rate);
        if (ersu106t)
            rate = CONNAC2X_HW_TX_RATE_UNMASK_106T(rate);
#endif /* #if (CFG_SUPPORT_CONNAC2X == 1) */

        ucCnt = 0;
        ucPerSum = 0;
        u2RateCode = prTxMcsInfo->au2TxRateCode[i];
        for (j = 0; j < MCS_INFO_SAMPLE_CNT; j++) {
            if (u2RateCode == prTxMcsInfo->au2TxRateCode[j]) {
                ucPerSum += prTxMcsInfo->aucTxRatePer[j];
                ucCnt++;
                prTxMcsInfo->au2TxRateCode[j] = 0xFFFF;
                prTxMcsInfo->aucTxBw[j] = 0xFF;
                prTxMcsInfo->aucTxSgi[j] = 0xFF;
                prTxMcsInfo->aucTxLdpc[j] = 0xFF;
            }
        }

        if (txmode == TX_RATE_MODE_CCK)
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "%s, ", HW_TX_RATE_CCK_STR[rate & 0x3]);
        else if (txmode == TX_RATE_MODE_OFDM)
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "%s, ", nicHwRateOfdmStr(rate));
        else if ((txmode == TX_RATE_MODE_HTMIX) ||
                 (txmode == TX_RATE_MODE_HTGF))
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "MCS%d, ", rate);
        else
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "NSS%d_MCS%d, ", nsts, rate);

        i4BytesWritten += kalScnprintf(
                              pcCommand + i4BytesWritten,
                              i4TotalLen - i4BytesWritten,
                              "%s, ", (bw < 4) ? HW_TX_RATE_BW[bw] : HW_TX_RATE_BW[4]);

        if (txmode == TX_RATE_MODE_CCK)
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "%s, ", (rate < 4) ? "LP" : "SP");
        else if (txmode == TX_RATE_MODE_OFDM)
            ;
        else if ((txmode == TX_RATE_MODE_HTMIX) ||
                 (txmode == TX_RATE_MODE_HTGF) ||
                 (txmode == TX_RATE_MODE_VHT) ||
                 (txmode == TX_RATE_MODE_PLR))
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "%s, ", (sgi == 0) ? "LGI" : "SGI");
        else
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "%s, ",
                                  (sgi == 0) ?
                                  "SGI" : (sgi == 1 ? "MGI" : "LGI"));

        i4BytesWritten += kalScnprintf(
                              pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
                              "%s%s%s%s%s [PER: %02d%%]\t",
                              (txmode <= ENUM_TX_MODE_NUM) ?
                              HW_TX_MODE_STR[txmode] : "N/A",
#if (CFG_SUPPORT_CONNAC2X == 1)
                              dcm ? ", DCM" : "", ersu106t ? ", 106t" : "",
#else /* #if (CFG_SUPPORT_CONNAC2X == 1) */
                              "", "",
#endif /* #if (CFG_SUPPORT_CONNAC2X == 1) */
                              stbc ? ", STBC, " : ", ",
                              ((ldpc == 0) ||
                               (txmode == TX_RATE_MODE_CCK) ||
                               (txmode == TX_RATE_MODE_OFDM)) ?
                              "BCC" : "LDPC", ucPerSum / ucCnt);

        for (j = 0; j < ucCnt; j++)
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "*");

        i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                       i4TotalLen - i4BytesWritten, "\n");

    }


    /* Output the RX MCS info */
    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten, "\nRx MCS:\n");

    /* connac2x_show_rx_rate_info() */
    prChipDbg = prAdapter->chip_info->prDebugOps;

    for (i = 0; i < MCS_INFO_SAMPLE_CNT; i++) {
        if (au4RxV0[i] == 0xFFFFFFFF)
            continue;

        u4RxV0 = au4RxV0[i];
        u4RxV1 = au4RxV1[i];
        u4RxV2 = au4RxV2[i];

        if (prChipDbg &&
            prChipDbg->show_rx_rate_info ==
            connac2x_show_rx_rate_info) {

            if (CONNAC2X_RX_DBG_INFO_GRP3(prAdapter)) {
                /* Rx DBG info from group 3 */
                goto GET_MCS_INFO_BIT_MASK_G3;
            } else {
                /* Rx DBG info from group 3 and group 5 */
                goto GET_MCS_INFO_BIT_MASK_G3_G5;
            }

        } else {
            goto GET_MCS_INFO_BIT_MASK_LEGACY;
        }


    GET_MCS_INFO_BIT_MASK_G3:
        /* u4RxV0: Group3 PRXV0[0:31] */
        /* P-RXV0 */
        rate = (u4RxV0 & CONNAC2X_RX_VT_RX_RATE_MASK)
               >> CONNAC2X_RX_VT_RX_RATE_OFFSET;
        nsts = ((u4RxV0 & CONNAC2X_RX_VT_NSTS_MASK)
                >> CONNAC2X_RX_VT_NSTS_OFFSET);
        ldpc = u4RxV0 & CONNAC2X_RX_VT_LDPC;
        frmode =
            (u4RxV0 & CONNAC2X_RX_VT_FR_MODE_MASK_V2)
            >> CONNAC2X_RX_VT_FR_MODE_OFFSET_V2;
        sgi = (u4RxV0 & CONNAC2X_RX_VT_SHORT_GI_MASK_V2)
              >> CONNAC2X_RX_VT_SHORT_GI_OFFSET_V2;
        stbc = (u4RxV0 & CONNAC2X_RX_VT_STBC_MASK_V2)
               >> CONNAC2X_RX_VT_STBC_OFFSET_V2;
        txmode =
            (u4RxV0 & CONNAC2X_RX_VT_RX_MODE_MASK_V2)
            >> CONNAC2X_RX_VT_RX_MODE_OFFSET_V2;
        mu = (u4RxV0 & CONNAC2X_RX_VT_MU);
        dcm = (u4RxV0 & CONNAC2X_RX_VT_DCM);

        if (mu == 0)
            nsts += 1;

        goto GET_MCS_INFO_OUTPUT_RX;


    GET_MCS_INFO_BIT_MASK_G3_G5:
        /* u4RxV0: Group3 PRXV0[0:31] */
        /* u4RxV1: Group5 C-B-0[0:31] */
        /* u4RxV2: Group5 C-B-1[0:31] */

        /* P-RXV0 */
        rate = (u4RxV0 & CONNAC2X_RX_VT_RX_RATE_MASK)
               >> CONNAC2X_RX_VT_RX_RATE_OFFSET;
        nsts = ((u4RxV0 & CONNAC2X_RX_VT_NSTS_MASK)
                >> CONNAC2X_RX_VT_NSTS_OFFSET);
        ldpc = u4RxV0 & CONNAC2X_RX_VT_LDPC;

        /* C-B-0 */
        stbc = (u4RxV1 & CONNAC2X_RX_VT_STBC_MASK)
               >> CONNAC2X_RX_VT_STBC_OFFSET;
        txmode =
            (u4RxV1 & CONNAC2X_RX_VT_RX_MODE_MASK)
            >> CONNAC2X_RX_VT_RX_MODE_OFFSET;
        frmode =
            (u4RxV1 & CONNAC2X_RX_VT_FR_MODE_MASK)
            >> CONNAC2X_RX_VT_FR_MODE_OFFSET;
        sgi = (u4RxV1 & CONNAC2X_RX_VT_SHORT_GI_MASK)
              >> CONNAC2X_RX_VT_SHORT_GI_OFFSET;
        /* C-B-1 */
        groupid =
            (u4RxV2 & CONNAC2X_RX_VT_GROUP_ID_MASK)
            >> CONNAC2X_RX_VT_GROUP_ID_OFFSET;

        if (groupid && groupid != 63) {
            mu = 1;
        } else {
            mu = 0;
            nsts += 1;
        }

        goto GET_MCS_INFO_OUTPUT_RX;


    GET_MCS_INFO_BIT_MASK_LEGACY:
        /* 1st Cycle RX_VT_RX_MODE : bit 12 - 14 */
        txmode = (u4RxV0 & RX_VT_RX_MODE_MASK)
                 >> RX_VT_RX_MODE_OFFSET;

        /* 1st Cycle RX_VT_RX_RATE: bit 0 - 6 */
        rate = (u4RxV0 & RX_VT_RX_RATE_MASK)
               >> RX_VT_RX_RATE_OFFSET;

        /* 1st Cycle RX_VT_RX_FR_MODE: bit 15 - 16 */
        frmode = (u4RxV0 & RX_VT_FR_MODE_MASK)
                 >> RX_VT_FR_MODE_OFFSET;

        /* 2nd Cycle RX_VT_NSTS: bit 27 - 29 */
        nsts = ((u4RxV1 & RX_VT_NSTS_MASK)
                >> RX_VT_NSTS_OFFSET);

        /* 1st Cycle RX_VT_STBC: bit 7 - 8 */
        stbc = (u4RxV0 & RX_VT_STBC_MASK)
               >> RX_VT_STBC_OFFSET;

        /* 1st Cycle RX_VT_SHORT_GI: bit 19 */
        sgi = u4RxV0 & RX_VT_SHORT_GI;

        /* 1st Cycle RX_VT_LDPC: bit 9 */
        ldpc = u4RxV0 & RX_VT_LDPC;

        /* 2nd Cycle RX_VT_GROUP_ID_MASK: bit 21 - 26 */
        groupid =
            (u4RxV1 & RX_VT_GROUP_ID_MASK)
            >> RX_VT_GROUP_ID_OFFSET;

        if (groupid && groupid != 63) {
            mu = 1;
        } else {
            mu = 0;
            nsts += 1;
        }

        goto GET_MCS_INFO_OUTPUT_RX;


    GET_MCS_INFO_OUTPUT_RX:

        /* Distribution Calculation clear the same sample content */
        ucCnt = 0;
        for (j = 0; j < MCS_INFO_SAMPLE_CNT; j++) {
            if ((u4RxV0 & RX_MCS_INFO_MASK) ==
                (au4RxV0[j] & RX_MCS_INFO_MASK)) {
                au4RxV0[j] = 0xFFFFFFFF;
                ucCnt++;
            }
        }

        if (prChipDbg &&
            prChipDbg->show_rx_rate_info ==
            connac2x_show_rx_rate_info) {

            if (txmode == TX_RATE_MODE_CCK)
                i4BytesWritten +=
                    kalScnprintf(pcCommand + i4BytesWritten,
                                 i4TotalLen - i4BytesWritten,
                                 "%s, ", (rate < 4) ?
                                 HW_TX_RATE_CCK_STR[rate] :
                                 ((rate < 8) ?
                                  HW_TX_RATE_CCK_STR[rate - 4] :
                                  HW_TX_RATE_CCK_STR[4]));
            else if (txmode == TX_RATE_MODE_OFDM)
                i4BytesWritten +=
                    kalScnprintf(pcCommand + i4BytesWritten,
                                 i4TotalLen - i4BytesWritten,
                                 "%s, ",
                                 nicHwRateOfdmStr(rate));
            else if ((txmode == TX_RATE_MODE_HTMIX) ||
                     (txmode == TX_RATE_MODE_HTGF))
                i4BytesWritten +=
                    kalScnprintf(pcCommand + i4BytesWritten,
                                 i4TotalLen - i4BytesWritten,
                                 "MCS%d, ", rate);
            else
                i4BytesWritten +=
                    kalScnprintf(pcCommand + i4BytesWritten,
                                 i4TotalLen - i4BytesWritten,
                                 "%s%d_MCS%d, ",
                                 (stbc == 1) ? "NSTS" : "NSS",
                                 nsts, rate);

        } else {

            if (txmode == TX_RATE_MODE_CCK)
                i4BytesWritten += kalScnprintf(
                                      pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "%s, ",
                                      (rate < 4) ?
                                      HW_TX_RATE_CCK_STR[rate] :
                                      HW_TX_RATE_CCK_STR[4]);
            else if (txmode == TX_RATE_MODE_OFDM)
                i4BytesWritten += kalScnprintf(
                                      pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "%s, ",
                                      nicHwRateOfdmStr(rate));
            else if ((txmode == TX_RATE_MODE_HTMIX) ||
                     (txmode == TX_RATE_MODE_HTGF))
                i4BytesWritten += kalScnprintf(
                                      pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "MCS%d, ", rate);
            else
                i4BytesWritten += kalScnprintf(
                                      pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "NSS%d_MCS%d, ",
                                      nsts, rate);
        }

        i4BytesWritten +=
            kalScnprintf(pcCommand + i4BytesWritten,
                         i4TotalLen - i4BytesWritten, "%s, ",
                         (frmode < 4) ?
                         HW_TX_RATE_BW[frmode] : HW_TX_RATE_BW[4]);

        if (txmode == TX_RATE_MODE_CCK)
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "%s, ",
                                  (rate < 4) ? "LP" : "SP");
        else if (txmode == TX_RATE_MODE_OFDM)
            ;
        else if (txmode == TX_RATE_MODE_HTMIX ||
                 txmode == TX_RATE_MODE_HTGF ||
                 txmode == TX_RATE_MODE_VHT)
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "%s, ",
                                  (sgi == 0) ? "LGI" : "SGI");
        else
            i4BytesWritten +=
                kalScnprintf(pcCommand + i4BytesWritten,
                             i4TotalLen - i4BytesWritten, "%s, ",
                             (sgi == 0) ? "SGI" :
                             (sgi == 1 ? "MGI" : "LGI"));

        i4BytesWritten +=
            kalScnprintf(pcCommand + i4BytesWritten,
                         i4TotalLen - i4BytesWritten, "%s",
                         (stbc == 0) ? "" : "STBC, ");

        if (prChipDbg &&
            prChipDbg->show_rx_rate_info ==
            connac2x_show_rx_rate_info) {

            if (CONNAC2X_RX_DBG_INFO_GRP3(prAdapter))
                i4BytesWritten +=
                    kalScnprintf(pcCommand + i4BytesWritten,
                                 i4TotalLen - i4BytesWritten,
                                 "%s",
                                 (dcm == 0) ? "" : "DCM, ");

            if (mu) {
                if (CONNAC2X_RX_DBG_INFO_GRP3(prAdapter))
                    i4BytesWritten += kalScnprintf(
                                          pcCommand + i4BytesWritten,
                                          i4TotalLen - i4BytesWritten,
                                          "%s, %s, %s\t",
                                          (txmode < ENUM_TX_MODE_NUM) ?
                                          HW_TX_MODE_STR[txmode] :
                                          "N/A",
                                          (ldpc == 0) ? "BCC" : "LDPC",
                                          "MU");
                else
                    i4BytesWritten += kalScnprintf(
                                          pcCommand + i4BytesWritten,
                                          i4TotalLen - i4BytesWritten,
                                          "%s, %s, %s (%d)\t",
                                          (txmode < ENUM_TX_MODE_NUM) ?
                                          HW_TX_MODE_STR[txmode] : "N/A",
                                          (ldpc == 0) ?
                                          "BCC" : "LDPC",
                                          "MU", groupid);
            } else {
                i4BytesWritten +=
                    kalScnprintf(pcCommand + i4BytesWritten,
                                 i4TotalLen - i4BytesWritten,
                                 "%s, %s\t",
                                 (txmode < ENUM_TX_MODE_NUM) ?
                                 HW_TX_MODE_STR[txmode] :
                                 "N/A",
                                 (ldpc == 0) ? "BCC" : "LDPC");
            }

        } else {

            if (mu) {
                i4BytesWritten += kalScnprintf(
                                      pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten,
                                      "%s, %s, %s (%d)\t",
                                      (txmode < ENUM_TX_MODE_NUM) ?
                                      HW_TX_MODE_STR[txmode] : "N/A",
                                      (ldpc == 0) ? "BCC" : "LDPC",
                                      "MU", groupid);
            } else {
                i4BytesWritten += kalScnprintf(
                                      pcCommand + i4BytesWritten,
                                      i4TotalLen - i4BytesWritten, "%s, %s\t",
                                      (txmode < ENUM_TX_MODE_NUM) ?
                                      HW_TX_MODE_STR[txmode] : "N/A",
                                      (ldpc == 0) ? "BCC" : "LDPC");
            }

        }

        /* Output the using times of this data rate */
        for (j = 0; j < ucCnt; j++)
            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten, "*");

        i4BytesWritten +=
            kalScnprintf(pcCommand + i4BytesWritten,
                         i4TotalLen - i4BytesWritten, "\n");
    }

    return i4BytesWritten;
}

static int priv_driver_get_mcs_info(struct GLUE_INFO *prGlueInfo,
                                    char *pcCommand, int i4TotalLen)
{
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    int32_t i4BytesWritten = 0, i4Argc = 0;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    struct ADAPTER *prAdapter = NULL;
    struct PARAM_TX_MCS_INFO *prTxMcsInfo = NULL;

    prAdapter = prGlueInfo->prAdapter;
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    if (prAdapter->rRxMcsInfoTimer.pfMgmtTimeOutFunc == NULL) {
        cnmTimerInitTimer(prAdapter,
                          &prAdapter->rRxMcsInfoTimer,
                          (PFN_MGMT_TIMEOUT_FUNC) wlanRxMcsInfoMonitor,
                          (unsigned long) NULL);
    }

    if (i4Argc >= 2) {

        if (strnicmp((char *)apcArgv[1], "START",
                     strlen("START")) == 0) {
            cnmTimerStartTimer(prAdapter,
                               &prAdapter->rRxMcsInfoTimer, MCS_INFO_SAMPLE_PERIOD);
            prAdapter->fgIsMcsInfoValid = TRUE;

            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\nStart the MCS Info Function\n");
            return i4BytesWritten;

        } else if (strnicmp((char *)apcArgv[1], "STOP",
                            strlen("STOP")) == 0) {
            cnmTimerStopTimer(prAdapter,
                              &prAdapter->rRxMcsInfoTimer);
            prAdapter->fgIsMcsInfoValid = FALSE;

            i4BytesWritten += kalScnprintf(
                                  pcCommand + i4BytesWritten,
                                  i4TotalLen - i4BytesWritten,
                                  "\nStop the MCS Info Function\n");
            return i4BytesWritten;

        } else
            goto warning;
    }

    if (prGlueInfo->prAdapter->fgIsMcsInfoValid != TRUE)
        goto warning;

    prTxMcsInfo = (struct PARAM_TX_MCS_INFO *)kalMemAlloc(
                      sizeof(struct PARAM_TX_MCS_INFO), VIR_MEM_TYPE);
    if (!prTxMcsInfo) {
        DBGLOG(REQ, ERROR, "Allocate prTxMcsInfo failed!\n");
        i4BytesWritten = -1;
        goto out;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidTxQueryMcsInfo, prTxMcsInfo,
                       sizeof(struct PARAM_TX_MCS_INFO),
                       TRUE, TRUE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS)
        goto out;

    i4BytesWritten = priv_driver_last_sec_mcs_info(prGlueInfo->prAdapter,
                                                   pcCommand, i4TotalLen, prTxMcsInfo);

    DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

    goto out;

warning:

    i4BytesWritten += kalScnprintf(pcCommand + i4BytesWritten,
                                   i4TotalLen - i4BytesWritten,
                                   "\nWARNING: Use GET_MCS_INFO [START/STOP]\n");

out:

    if (prTxMcsInfo)
        kalMemFree(prTxMcsInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_TX_MCS_INFO));

    return i4BytesWritten;
}
#endif /* #if (CFG_WIFI_GET_MCS_INFO == 1) */

#if CFG_ENABLE_WFDMA_DVT
struct wfdma_dvt mtk_wfdma_dvt = {0};
extern struct HIF_MEM rDvtTxBuf[WFDMA_DVT_RING_NUM][WFDMA_DVT_RING_SIZE];
extern struct HIF_MEM rDvtRxBuf[WFDMA_DVT_RING_NUM][WFDMA_DVT_RING_SIZE];

static int priv_driver_wfdma_dvt_init(
    struct GLUE_INFO *prGlueInfo,
    char *pcCommand,
    int i4TotalLen)
{
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    int32_t i4BytesWritten = 0;
    uint32_t u4Ret;
    uint8_t ucMode = 0;
    uint32_t u4StartRing = 0;
    uint8_t i = 0;

    DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);
    u4Ret = kalkStrtou8(apcArgv[1], 0, &ucMode);
    u4Ret = kalkStrtou32(apcArgv[2], 0, &u4StartRing);
    DBGLOG(INIT, ERROR, "----- WFDMA DVT Init: Mode=%s StartRing=%d -----\n",
           (ucMode) ? "Force" : "Correspond",
           u4StartRing);
    DBGLOG(INIT, INFO, "u4Ret %x\r\n", u4Ret);

    mtk_wfdma_dvt.enable = 1;
    mtk_wfdma_dvt.rx_mode = ucMode;
    mtk_wfdma_dvt.tx_start = u4StartRing;
    if (ucMode)
        mtk_wfdma_dvt.rx_start = 0;
    else
        mtk_wfdma_dvt.rx_start = u4StartRing;

    for (i = 0; i < WFDMA_DVT_RING_NUM; i++) {
        mtk_wfdma_dvt.tx_counts[i] = 0;
        mtk_wfdma_dvt.rx_counts[i] = 0;
    }

    if (!halWfdmaDvtInit(prGlueInfo->prAdapter, ucMode)) {
        DBGLOG(INIT, ERROR, "halWfdmaDvtInit failed!\n");
    }

    return i4BytesWritten;
}

void DvtShowWfdmaInfo(void)
{
    uint32_t u4Idx = 0, u4Idx_1 = 0;
    uint32_t u4RegAddr = 0, u4RegVal = 0;
    uint32_t u4RegVal_1 = 0, u4RegVal_2 = 0, u4RegVal_3 = 0;
    struct ADAPTER *prAdapter = &g_rAdapter;

    DBGLOG(INIT, ERROR, "WFDMA DVT SW Record:\n");
    DBGLOG(INIT, ERROR, "enable:%d\n", mtk_wfdma_dvt.enable);
    DBGLOG(INIT, ERROR, "tx_start:%d\n", mtk_wfdma_dvt.tx_start);
    DBGLOG(INIT, ERROR, "rx_start:%d\n", mtk_wfdma_dvt.rx_start);
    for (u4Idx = 0; u4Idx < WFDMA_DVT_RING_NUM; u4Idx++) {
        DBGLOG(INIT, ERROR, "tx_counts%d:%d\n",
               u4Idx, mtk_wfdma_dvt.tx_counts[u4Idx]);
        DBGLOG(INIT, ERROR, "rx_counts%d:%d\n",
               u4Idx, mtk_wfdma_dvt.rx_counts[u4Idx]);
    }

    DBGLOG(INIT, STATE, "WFDMA HW Reg:\n");
    u4RegAddr = CONNAC2X_WPDMA_INT_STA(CONNAC2X_HOST_WPDMA_0_BASE);
    HAL_MCR_RD(prAdapter, u4RegAddr, &u4RegVal);
    HAL_MCR_RD(prAdapter, u4RegAddr + 0x4, &u4RegVal_1);
    HAL_MCR_RD(prAdapter, u4RegAddr + 0x8, &u4RegVal_2);
    DBGLOG(INIT, STATE, "[0x%x]=0x%x 0x%x 0x%x\n", u4RegAddr, u4RegVal, u4RegVal_1, u4RegVal_2);
    for (u4Idx = 0; u4Idx < WFDMA_DVT_RING_NUM; u4Idx++) {
        u4RegAddr = CONNAC2X_TX_RING_BASE(CONNAC2X_HOST_WPDMA_0_BASE) + 0x10 * (mtk_wfdma_dvt.tx_start + u4Idx);
        HAL_MCR_RD(prAdapter, u4RegAddr, &u4RegVal);
        HAL_MCR_RD(prAdapter, u4RegAddr + 0x4, &u4RegVal_1);
        HAL_MCR_RD(prAdapter, u4RegAddr + 0x8, &u4RegVal_2);
        HAL_MCR_RD(prAdapter, u4RegAddr + 0xc, &u4RegVal_3);
        DBGLOG(INIT, STATE, "[0x%x]=0x%x 0x%x 0x%x 0x%x\n", u4RegAddr, u4RegVal, u4RegVal_1, u4RegVal_2, u4RegVal_3);
    }

    for (u4Idx = 0; u4Idx < WFDMA_DVT_RING_NUM; u4Idx++) {
        u4RegAddr = CONNAC2X_RX_RING_BASE(CONNAC2X_HOST_WPDMA_0_BASE) + 0x10 * (mtk_wfdma_dvt.rx_start + u4Idx);
        HAL_MCR_RD(prAdapter, u4RegAddr, &u4RegVal);
        HAL_MCR_RD(prAdapter, u4RegAddr + 0x4, &u4RegVal_1);
        HAL_MCR_RD(prAdapter, u4RegAddr + 0x8, &u4RegVal_2);
        HAL_MCR_RD(prAdapter, u4RegAddr + 0xc, &u4RegVal_3);
        DBGLOG(INIT, STATE, "[0x%x]=0x%x 0x%x 0x%x 0x%x\n", u4RegAddr, u4RegVal, u4RegVal_1, u4RegVal_2, u4RegVal_3);
    }

    for (u4Idx = 0; u4Idx < WFDMA_DVT_RING_NUM; u4Idx++) {
        for (u4Idx_1 = 0; u4Idx_1 < WFDMA_DVT_RING_SIZE; u4Idx_1++) {
            DBGLOG(INIT, STATE, "TxData [%d][%d]: 0x%x[%p]=0x%x 0x%x 0x%x 0x%x\n", u4Idx, u4Idx_1,
                   (uint32_t)rDvtTxBuf[u4Idx][u4Idx_1].pa, rDvtTxBuf[u4Idx][u4Idx_1].va,
                   *((uint32_t *)rDvtTxBuf[u4Idx][u4Idx_1].va), *(((uint32_t *)rDvtTxBuf[u4Idx][u4Idx_1].va) + 1),
                   *(((uint32_t *)rDvtTxBuf[u4Idx][u4Idx_1].va) + 2), *(((uint32_t *)rDvtTxBuf[u4Idx][u4Idx_1].va) + 3));
        }
    }

    for (u4Idx = 0; u4Idx < WFDMA_DVT_RING_NUM; u4Idx++) {
        for (u4Idx_1 = 0; u4Idx_1 < WFDMA_DVT_RING_SIZE; u4Idx_1++) {
            DBGLOG(INIT, STATE, "RxData [%d][%d]: 0x%x[%p]=0x%x 0x%x 0x%x 0x%x\n", u4Idx, u4Idx_1,
                   (uint32_t)rDvtRxBuf[u4Idx][u4Idx_1].pa, rDvtRxBuf[u4Idx][u4Idx_1].va,
                   *((uint32_t *)rDvtRxBuf[u4Idx][u4Idx_1].va), *(((uint32_t *)rDvtRxBuf[u4Idx][u4Idx_1].va) + 1),
                   *(((uint32_t *)rDvtRxBuf[u4Idx][u4Idx_1].va) + 2), *(((uint32_t *)rDvtRxBuf[u4Idx][u4Idx_1].va) + 3));
        }
    }
}

int8_t halWfdmaDvtStart(IN struct GLUE_INFO *prGlueInfo, uint8_t ucType, uint32_t ucLen)
{
    struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
    struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
    struct RTMP_TX_RING *pTxRing;
    struct RTMP_DMACB *prTxCell;
    struct RTMP_RX_RING *pRxRing;
    struct RTMP_DMACB *prRxCell;
    uint32_t tx_count = 0, tx_len = ucLen;
    uint32_t rx_count = 0;
    uint32_t tx_idx = 0, rx_idx = 0;
    uint32_t idx;
    uint32_t *tx_data = (uint32_t *)kalMemAlloc(WFDMA_DVT_FRAME_SIZE,
                                                VIR_MEM_TYPE);
    uint32_t *rx_data = (uint32_t *)kalMemAlloc(WFDMA_DVT_FRAME_SIZE,
                                                VIR_MEM_TYPE);
    uint32_t ret = 0, done_b = 0;
    uint8_t burst = WFDMA_DVT_RING_SIZE - 1;

    if (rx_data == NULL || tx_data == NULL) {
        DBGLOG(INIT, ERROR, "tx/rx_data allocate %d fail\n",
               WFDMA_DVT_FRAME_SIZE);
        return -1;
    }
    for (tx_idx = 0; tx_idx < WFDMA_DVT_RING_NUM; tx_idx++) {
        if (mtk_wfdma_dvt.enable == 0)
            break;
        pTxRing = &prHifInfo->TxRing[tx_idx];
        tx_count = mtk_wfdma_dvt.tx_counts[tx_idx];
        while (tx_count < mtk_wfdma_dvt.tx_counts[tx_idx] + burst) {
            if (mtk_wfdma_dvt.enable == 0)
                break;
            kalMemZero((uint8_t *)tx_data, WFDMA_DVT_FRAME_SIZE);
            *tx_data = tx_idx << 24 | tx_count;
            tx_data++;
            *tx_data = tx_len;
            tx_data++;
            kalMemSet((uint8_t *)(tx_data + 1), 0xA5, tx_len - 4);
            tx_data -= 2;
            DBGLOG(INIT, STATE, "WRITE RING[%d] CNT %d DVT.TX_CNTS %d\n",
                   tx_idx, tx_count, mtk_wfdma_dvt.tx_counts[tx_idx] + burst);
            ret = kalDevPortWrite(prGlueInfo, tx_idx, tx_len,
                                  (uint8_t *)tx_data,
                                  prAdapter->u4CoalescingBufCachedSize);
            DBGLOG(INIT, STATE, "WRITE RING[%d] CNT %d DONE\n",
                   tx_idx, tx_count);
            if (ret)
                tx_count++;
        }
        mtk_wfdma_dvt.tx_counts[tx_idx] = tx_count;
    }
    for (tx_idx = 0; tx_idx < WFDMA_DVT_RING_NUM; tx_idx++) {
        pTxRing = &prHifInfo->TxRing[tx_idx];
        tx_count = mtk_wfdma_dvt.tx_counts[tx_idx];
        if (tx_count) {
            tx_count = (tx_count - burst) % WFDMA_DVT_RING_SIZE;
        }
        for (idx = 0; idx < burst; idx++) {
            prTxCell = &pTxRing->Cell[tx_count + idx];
            DBGLOG(INIT, STATE, "Tx Ring%d[%d]:\n",
                   tx_idx, tx_count + idx);
            DBGLOG(INIT, STATE, "TxDMAD: 0x%x[0x%p]: ",
                   prTxCell->AllocPa, prTxCell->AllocVa);
            DBGLOG(INIT, STATE, "0x%x 0x%x 0x%x 0x%x\n",
                   *((uint32_t *)prTxCell->AllocVa),
                   *(((uint32_t *)prTxCell->AllocVa) + 1),
                   *(((uint32_t *)prTxCell->AllocVa) + 2),
                   *(((uint32_t *)prTxCell->AllocVa) + 3));
            DBGLOG(INIT, STATE, "TxData: [0x%p] (0x%x[0x%p]): ",
                   prTxCell->DmaBuf.AllocVa,
                   rDvtTxBuf[tx_idx][tx_count + idx].pa,
                   rDvtTxBuf[tx_idx][tx_count + idx].va);
            DBGLOG(INIT, STATE, "0x%x 0x%x 0x%x 0x%x\n",
                   *((uint32_t *)prTxCell->DmaBuf.AllocVa),
                   *(((uint32_t *)prTxCell->DmaBuf.AllocVa) + 1),
                   *(((uint32_t *)prTxCell->DmaBuf.AllocVa) + 2),
                   *(((uint32_t *)prTxCell->DmaBuf.AllocVa) + 3));
        }
    }
    for (rx_idx = 0; rx_idx < WFDMA_DVT_RING_NUM; rx_idx++) {
        if (mtk_wfdma_dvt.enable == 0)
            break;
        pRxRing = &prHifInfo->RxRing[rx_idx];
        rx_count = mtk_wfdma_dvt.rx_counts[rx_idx];
        prRxCell = &pRxRing->Cell[rx_count % WFDMA_DVT_RING_SIZE];
        done_b = *(((uint32_t *)prRxCell->AllocVa) + 1) >> 31;
        while (done_b) {
            if (mtk_wfdma_dvt.enable == 0)
                break;
            DBGLOG(INIT, STATE, "IDX %u COUNT %d done_b %d VA %p\n",
                   rx_idx, rx_count, done_b, prRxCell->AllocVa);
            DBGLOG(INIT, STATE, "READ RING[%d] CNT %d DVT.RX_CNTS %d\n",
                   rx_idx, rx_count, mtk_wfdma_dvt.rx_counts[rx_idx]);
            ret = kalDevPortRead(prGlueInfo, rx_idx,
                                 WFDMA_DVT_FRAME_SIZE, (uint8_t *)rx_data,
                                 prAdapter->u4CoalescingBufCachedSize);
            if (!ret) {
                DBGLOG(INIT, ERROR, "RX fail\n");
                break;
            }
            DBGLOG(INIT, STATE, "READ RING[%d] CNT %d DONE\n",
                   rx_idx, rx_count);
            rx_count++;
            prRxCell = &pRxRing->Cell[rx_count % WFDMA_DVT_RING_SIZE];
            done_b = *(((uint32_t *)prRxCell->AllocVa) + 1) >> 31;
        }

        if (!done_b)
            DBGLOG(INIT, STATE, "IDX %u COUNT %d/%d done_b %d VA %p(%x)\n",
                   rx_idx, rx_count, WFDMA_DVT_RING_SIZE, done_b,
                   prRxCell->AllocVa,
                   *(((uint32_t *)prRxCell->AllocVa) + 1));

        if (pRxRing->fgSwRead) {
            kalDevRegWrite(prAdapter->prGlueInfo,
                           pRxRing->hw_cidx_addr, pRxRing->RxCpuIdx);
            pRxRing->fgSwRead = false;
        }
        mtk_wfdma_dvt.rx_counts[rx_idx] = rx_count;
        if (mtk_wfdma_dvt.rx_counts[rx_idx] !=
            mtk_wfdma_dvt.tx_counts[rx_idx]) {
            DBGLOG(INIT, ERROR, "IDX %u CNT %d,%d RX!=TX FAILED\n",
                   rx_idx,
                   mtk_wfdma_dvt.rx_counts[rx_idx],
                   mtk_wfdma_dvt.tx_counts[rx_idx]);
            return -1;
        }
    }

    for (rx_idx = 0; rx_idx < WFDMA_DVT_RING_NUM; rx_idx++) {
        pRxRing = &prHifInfo->RxRing[rx_idx];
        rx_count = mtk_wfdma_dvt.rx_counts[rx_idx];
        if (rx_count) {
            rx_count = (rx_count - burst) % WFDMA_DVT_RING_SIZE;
        }
        for (idx = 0; idx < burst; idx++) {
            prRxCell = &pRxRing->Cell[rx_count + idx];
            DBGLOG(INIT, STATE, "(After) Rx Ring%d[%d]:\n",
                   rx_idx, rx_count + idx);
            DBGLOG(INIT, STATE, "RxDMAD: 0x%x[0x%p]: ",
                   prRxCell->AllocPa, prRxCell->AllocVa);
            DBGLOG(INIT, STATE, "0x%x 0x%x 0x%x 0x%x\n",
                   *((uint32_t *)prRxCell->AllocVa),
                   *(((uint32_t *)prRxCell->AllocVa) + 1),
                   *(((uint32_t *)prRxCell->AllocVa) + 2),
                   *(((uint32_t *)prRxCell->AllocVa) + 3));
            DBGLOG(INIT, STATE, "RxData: [0x%p] (0x%x[0x%p]): ",
                   prRxCell->DmaBuf.AllocVa,
                   rDvtRxBuf[rx_idx][rx_count + idx].pa,
                   rDvtRxBuf[rx_idx][rx_count + idx].va);
            DBGLOG(INIT, STATE, "0x%x 0x%x 0x%x 0x%x\n",
                   *((uint32_t *)prRxCell->DmaBuf.AllocVa),
                   *(((uint32_t *)prRxCell->DmaBuf.AllocVa) + 1),
                   *(((uint32_t *)prRxCell->DmaBuf.AllocVa) + 2),
                   *(((uint32_t *)prRxCell->DmaBuf.AllocVa) + 3));
        }
    }

    if (tx_data != NULL)
        kalMemFree(tx_data, WFDMA_DVT_FRAME_SIZE, VIR_MEM_TYPE);
    if (rx_data != NULL)
        kalMemFree(rx_data, WFDMA_DVT_FRAME_SIZE, VIR_MEM_TYPE);
    return 0;
}

static int priv_driver_wfdma_dvt_stop(
    struct GLUE_INFO *prGlueInfo,
    char *pcCommand,
    int i4TotalLen)
{
    int32_t i4BytesWritten = 0;

    DBGLOG(INIT, ERROR, "WFDMA DVT STOP\n");

    mtk_wfdma_dvt.enable = 0;

    return i4BytesWritten;
}

static int priv_driver_wfdma_show_info(
    struct GLUE_INFO *prGlueInfo,
    char *pcCommand,
    int i4TotalLen)
{
    int32_t i4BytesWritten = 0;

    DBGLOG(INIT, ERROR, "WFDMA DVT SHOW INFO\n");
    DvtShowWfdmaInfo();

    return i4BytesWritten;
}

static int priv_driver_wfdma_dvt_start(
    struct GLUE_INFO *prGlueInfo,
    char *pcCommand,
    int i4TotalLen)
{
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    int32_t i4BytesWritten = 0;
    uint32_t u4Ret = 0;
    uint8_t ucTxType = 0;
    uint32_t u4TxLen = 0;
    uint32_t u4LoopTimes = 0;
    int8_t iTestResults = 0;

    DBGLOG(REQ, INFO, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, INFO, "argc is %i\n", i4Argc);
    if (i4Argc >= 2)
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4LoopTimes);
    if (i4Argc >= 3)
        u4Ret = kalkStrtou8(apcArgv[2], 0, &ucTxType);
    if (i4Argc >= 4)
        u4Ret = kalkStrtou32(apcArgv[3], 0, &u4TxLen);

    if (u4Ret)
        DBGLOG(REQ, ERROR, "parse error u4Ret=%d\n", u4Ret);
    if (u4TxLen >= WFDMA_DVT_FRAME_SIZE || u4TxLen == 0)
        u4TxLen = WFDMA_DVT_FRAME_SIZE - 64;
    DBGLOG(INIT, ERROR, "WFDMA DVT Start: Type=%u Len=%u Loop=%u\n",
           ucTxType, u4TxLen, u4LoopTimes);
    do {
        iTestResults = halWfdmaDvtStart(prGlueInfo, ucTxType, u4TxLen);
        if (iTestResults < 0)
            break;
    } while (u4LoopTimes--);

    priv_driver_wfdma_show_info(prGlueInfo, NULL, 0);

    if (!iTestResults)
        DBGLOG(REQ, ERROR, "WFDMA HW LOOPBACK PASS u4Ret=%d\n",
               iTestResults);
    else
        DBGLOG(REQ, ERROR, "WFDMA HW LOOPBACK FAILED u4Ret=%d\n",
               iTestResults);
    return i4BytesWritten;
}
#endif /* #if CFG_ENABLE_WFDMA_DVT */

#if CFG_SUPPORT_TRAFFIC_REPORT
static int priv_driver_get_traffic_report(
    struct GLUE_INFO *prGlueInfo,
    char *pcCommand,
    int i4TotalLen)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    int32_t i4BytesWritten = 0;
    uint32_t u4BufLen = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
    struct CMD_RLM_AIRTIME_MON *cmd = NULL;
    uint8_t ucBand = ENUM_BAND_0;
    uint16_t u2Val = 0;
#if CFG_SUPPORT_DBDC || (CFG_SUPPORT_TRAFFIC_REPORT_VER == 1)
    uint8_t ucVal = 0;
#endif /* #if CFG_SUPPORT_DBDC || (CFG_SUPPORT_TRAFFIC_REPORT_VER == 1) */
    int32_t u4Ret = 0;
    uint32_t persentage = 0;
    uint32_t sample_dur = 0;
#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
    unsigned char fgGetDbg = FALSE;
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */

    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

    cmd = (struct CMD_RLM_AIRTIME_MON *)
          kalMemAlloc(sizeof(*cmd), VIR_MEM_TYPE);
    if (!cmd)
        goto get_report_invalid;

    if ((i4Argc > 4) || (i4Argc < 2)) {
        DBGLOG(REQ, ERROR, "%s %d\n", __func__, __LINE__);
        goto get_report_invalid;
    }

    kalMemSet(cmd, 0, sizeof(*cmd));

    cmd->u2Type = CMD_GET_REPORT_TYPE;
    cmd->u2Len = sizeof(*cmd);
    cmd->ucBand = ucBand;

    if (strnicmp((char *)apcArgv[1], "ENABLE", strlen("ENABLE")) == 0) {
        prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap |=
            KEEP_FULL_PWR_TRAFFIC_REPORT_BIT;
        cmd->ucAction = CMD_GET_REPORT_ENABLE;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
    } else if (strnicmp((char *)apcArgv[1],
                        "DISABLE", strlen("DISABLE")) == 0) {
        prGlueInfo->prAdapter->u4IsKeepFullPwrBitmap &=
            ~KEEP_FULL_PWR_TRAFFIC_REPORT_BIT;
        cmd->ucAction = CMD_GET_REPORT_DISABLE;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
    } else if (strnicmp((char *)apcArgv[1],
                        "RESET", strlen("RESET")) == 0) {
        cmd->ucAction = CMD_GET_REPORT_RESET;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
    } else if ((strnicmp((char *)apcArgv[1], "GET", strlen("GET")) == 0) ||
               (strnicmp((char *)apcArgv[1],
                         "GETDBG", strlen("GETDBG")) == 0)) {
        cmd->ucAction = CMD_GET_REPORT_GET;
#if CFG_SUPPORT_DBDC
        if ((i4Argc == 4) &&
            (strnicmp((char *)apcArgv[2],
                      "BAND", strlen("BAND")) == 0)) {
            u4Ret = kalkStrtou8((char *)apcArgv[3], 0, &ucVal);
            if (u4Ret)
                goto get_report_invalid;
            cmd->ucBand = ucVal;
        }
#endif /* #if CFG_SUPPORT_DBDC */
#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
        if (strnicmp((char *)apcArgv[1],
                     "GETDBG", strlen("GETDBG")) == 0)
            fgGetDbg = TRUE;
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
    } else if ((strnicmp((char *)apcArgv[1],
                         "SAMPLEPOINTS", strlen("SAMPLEPOINTS")) == 0)
               && (i4Argc == 3)) {
        u4Ret = kalkStrtou16(apcArgv[2], 0, &u2Val);
        if (u4Ret)
            goto get_report_invalid;
        cmd->i2SamplePoints = u2Val;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->ucAction = CMD_SET_REPORT_SAMPLE_POINT;
    }
#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
    else if ((strnicmp((char *)apcArgv[1],
                       "TXTHRES", strlen("TXTHRES")) == 0) && (i4Argc == 3)) {
        u4Ret = kalkStrtou8(apcArgv[2], 0, &ucVal);
        if (u4Ret)
            goto get_report_invalid;
        /* valid val range is from 0 - 100% */
        if (ucVal > 100)
            ucVal = 100;
        cmd->ucTxThres = ucVal;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->ucAction = CMD_SET_REPORT_TXTHRES;
    } else if ((strnicmp((char *)apcArgv[1],
                         "RXTHRES", strlen("RXTHRES")) == 0) && (i4Argc == 3)) {
        u4Ret = kalkStrtou8(apcArgv[2], 0, &ucVal);
        if (u4Ret)
            goto get_report_invalid;
        /* valid val range is from 0 - 100% */
        if (ucVal > 100)
            ucVal = 100;
        cmd->ucRxThres = ucVal;
        cmd->u2Type |= CMD_ADV_CONTROL_SET;
        cmd->ucAction = CMD_SET_REPORT_RXTHRES;
    }
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
    else
        goto get_report_invalid;

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidAdvCtrl, cmd, sizeof(*cmd), TRUE, TRUE, TRUE, &u4BufLen);

    if ((rStatus != WLAN_STATUS_SUCCESS) &&
        (rStatus != WLAN_STATUS_PENDING))
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten,
                     i4TotalLen - i4BytesWritten,
                     "\ncommand failed %x", rStatus);
    else if (cmd->ucAction == CMD_GET_REPORT_GET) {
#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
        sample_dur = cmd->i4total_sample_duration_sixSec;
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\nCCK false detect cnt: %d"
                     , (cmd->u2FalseCCA >> EVENT_REPORT_CCK_FCCA) &
                     EVENT_REPORT_CCK_FCCA_FEILD);
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\nOFDM false detect cnt: %d"
                     , (cmd->u2FalseCCA >> EVENT_REPORT_OFDM_FCCA) &
                     EVENT_REPORT_OFDM_FCCA_FEILD);
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\nCCK Sig CRC cnt: %d"
                     , (cmd->u2HdrCRC >> EVENT_REPORT_CCK_SIGERR) &
                     EVENT_REPORT_CCK_SIGERR_FEILD);
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\nOFDM Sig CRC cnt: %d"
                     , (cmd->u2HdrCRC >> EVENT_REPORT_OFDM_SIGERR) &
                     EVENT_REPORT_OFDM_SIGERR_FEILD);
#else /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
        sample_dur = cmd->u4FetchEd - cmd->u4FetchSt;
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\nBand%d Info:", cmd->ucBand);
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\n\tSample every %u ms with %u points"
#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
                     , cmd->u2TimerDur, cmd->i2SamplePoints
#else /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
                     , cmd->u4TimerDur, cmd->i2SamplePoints
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
                    );

#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
        if (fgGetDbg) {
            i4BytesWritten +=
                snprintf(pcCommand + i4BytesWritten
                         , i4TotalLen - i4BytesWritten,
                         " from systime %u-%u total_dur %u ms f_cost %u us t_drift %d ms"
                         , cmd->u4FetchSt, cmd->u4FetchEd
                         , sample_dur
                         , cmd->u2FetchCost, cmd->TimerDrift);
            i4BytesWritten +=
                snprintf(pcCommand + i4BytesWritten
                         , i4TotalLen - i4BytesWritten,
                         "\n\tbusy-RMAC %u us, idle-TMAC %u us, t_total %u"
                         , cmd->i4total_TXTime_duration_sixSec
                         , cmd->i4total_chIdle_duration_sixSec
                         , cmd->i4total_sample_duration_sixSec);
            i4BytesWritten +=
                snprintf(pcCommand + i4BytesWritten
                         , i4TotalLen - i4BytesWritten,
                         "\n\theavy tx threshold %u%% rx threshold %u%%"
                         , cmd->ucTxThres, cmd->ucRxThres);
        }
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\n\tch_busy %u us, ch_idle %u us, total_period %u us"
#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
                     , cmd->i4total_OBSS_duration_sixSec
                     , cmd->i4total_chIdle_duration_sixSec
#else /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
                     , sample_dur - cmd->u4ChIdle
                     , cmd->u4ChIdle
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
                     , sample_dur);
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\n\tmy_tx_time: %u us"
#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
                     , cmd->i4total_TXTime_duration_sixSec
#else /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
                     , cmd->u4TxAirTime
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
                    );
        if (cmd->u4FetchEd - cmd->u4FetchSt) {
#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
            persentage =
                cmd->i4total_TXTime_duration_sixSec
                / (sample_dur / 1000);
#else /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
            persentage = cmd->u4TxAirTime / (sample_dur / 1000);
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
            i4BytesWritten +=
                snprintf(pcCommand + i4BytesWritten
                         , i4TotalLen - i4BytesWritten,
                         ", tx utility: %d.%1d%%"
                         , persentage / 10
                         , persentage % 10);
        }
#if CFG_SUPPORT_RX_TRAFFIC_REPORT == 1
        i4BytesWritten += snprintf(pcCommand + i4BytesWritten
                                   , i4TotalLen - i4BytesWritten,
                                   "\n\tmy_data_rx_time (no BMC data): %u us"
                                   , cmd->i4total_RxTime_duration_sixSec
                                  );
        if (cmd->u4FetchEd - cmd->u4FetchSt) {
#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
            persentage =
                cmd->i4total_RxTime_duration_sixSec
                / (sample_dur / 1000);
#else /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
            persentage = cmd->u4RxAirTime / (sample_dur / 1000);
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
            i4BytesWritten += snprintf(pcCommand + i4BytesWritten
                                       , i4TotalLen - i4BytesWritten,
                                       ", rx utility: %d.%1d%%"
                                       , persentage / 10
                                       , persentage % 10);
        }
#endif /* #if CFG_SUPPORT_RX_TRAFFIC_REPORT == 1 */

#if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\n\tTotal packet transmitted: %u"
                     , cmd->u2PktSent);
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\n\tTotal tx ok packet: %u"
                     , cmd->u2PktSent - cmd->u2PktTxfailed);
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\n\tTotal tx failed packet: %u"
                     , cmd->u2PktTxfailed);
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\n\tTotal tx retried packet: %u"
                     , cmd->u2PktRetried);
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\n\tTotal rx mpdu: %u", cmd->u2RxMPDU);
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\n\tTotal rx fcs: %u", cmd->u2RxFcs);
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT_VER == 1 */
    } else
        i4BytesWritten +=
            snprintf(pcCommand + i4BytesWritten
                     , i4TotalLen - i4BytesWritten,
                     "\ncommand sent %x", rStatus);

    if (cmd)
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));

    return i4BytesWritten;
get_report_invalid:
    if (cmd)
        kalMemFree(cmd, VIR_MEM_TYPE, sizeof(*cmd));
    i4BytesWritten += snprintf(pcCommand + i4BytesWritten
                               , i4TotalLen - i4BytesWritten,
                               "\nformat:get_report [enable|disable|get|reset]");
    return i4BytesWritten;
}
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT */

#if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1)
int priv_driver_set_arp_offload(IN struct GLUE_INFO *prGlueInfo,
                                IN char *pcCommand, IN int i4TotalLen)
{
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    uint8_t fgEnable;
    uint32_t u4Enable = 0;
    uint32_t u4Idx = 0;
    int32_t u4Ret = 0;
    uint32_t u4SetInfoLen = 0;
    uint32_t u4Len = 0;
    struct PARAM_NETWORK_ADDRESS_LIST rParamNetAddrList;
    struct net_device *prDev = NULL;

    ASSERT(prGlueInfo);
    if (GLUE_CHK_PR2(prGlueInfo, pcCommand) == FALSE)
        return -1;

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

    if (i4Argc >= 2) {
        u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Enable);
        if (u4Ret) {
            DBGLOG(REQ, LOUD, "parse u4Enable error u4Ret=%d\n",
                   u4Ret);
            return u4Ret;
        }

        if (u4Enable == 1)
            fgEnable = TRUE;
        else
            fgEnable = FALSE;

        prGlueInfo->prAdapter->rWifiVar.ucArpOffload = fgEnable;

        DBGLOG(REQ, INFO, "%s: Set Arp offload [%u] [%u]\n", __func__,
               fgEnable, prGlueInfo->fgIsInSuspendMode);

        for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
            prDev = wlanGetNetDev(prGlueInfo, u4Idx);
            if (!prDev)
                continue;

            if (fgEnable == FALSE) {
                rParamNetAddrList.ucBssIdx = prDev->bss_idx;
                rParamNetAddrList.u4AddressCount = 0;
                rParamNetAddrList.u2AddressType =
                    PARAM_PROTOCOL_ID_TCP_IP;
                u4Len = OFFSET_OF(struct
                                  PARAM_NETWORK_ADDRESS_LIST, arAddress);
                u4Ret = kalIoctl(prGlueInfo,
                                 wlanoidSetNetworkAddress,
                                 (void *) &rParamNetAddrList, u4Len,
                                 FALSE, FALSE, TRUE, &u4SetInfoLen);
            } else {
                if (prGlueInfo->fgIsInSuspendMode) {
                    /* if in suspendmode, update arp setting to FW
                     * otherwise, update by set_suspendmode cmd
                     */
                    kalSetNetAddressFromInterface(prGlueInfo, prDev, fgEnable);
                    wlanNotifyFwSuspend(prGlueInfo, prDev, fgEnable);
                }
            }
        }
    }
    return 0;
}

#endif /* #if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1) */

typedef int(*PRIV_CMD_FUNCTION)(
    IN struct GLUE_INFO *prGlueInfo,
    IN char *pcCommand,
    IN int i4TotalLen);

struct PRIV_CMD_HANDLER {
    char *pcCmdStr;
    PRIV_CMD_FUNCTION pfHandler;
};

struct PRIV_CMD_HANDLER priv_cmd_handlers[] = {
    {CMD_SET_MCR, priv_driver_set_mcr},
    {CMD_GET_MCR, priv_driver_get_mcr},
    {CMD_SCAN, priv_driver_set_scan},
    {CMD_GET_SCAN_RES, priv_driver_get_scan_res},
    {CMD_CONNECT, priv_driver_set_connect},
    {CMD_LWIP_LINK_UP, netif_set_link_up_api},
    {CMD_LWIP_LINK_DOWN, netif_set_link_down_api},
#if CFG_SUPPORT_QA_TOOL || (CONFIG_WLAN_SERVICE == 1)
    {CMD_SET_TEST_MODE, priv_driver_set_test_mode},
    {CMD_SET_TEST_CMD, priv_driver_set_test_cmd},
    {CMD_GET_TEST_RESULT, priv_driver_get_test_result},
#endif /* #if CFG_SUPPORT_QA_TOOL || (CONFIG_WLAN_SERVICE == 1) */
#if (CFG_SUPPORT_PRIV_DBGLVL == 1)
    /* wlanDebugInit */
    {CMD_SET_DBG_LEVEL, priv_driver_set_dbg_level},
    {CMD_GET_DBG_LEVEL, priv_driver_get_dbg_level},
#endif /* #if (CFG_SUPPORT_PRIV_DBGLVL == 1) */
    {CMD_SET_FW_LOG, priv_driver_set_fw_log},
#if defined(_HIF_AXI) && (CFG_SUPPORT_MANUAL_OWN_CTRL == 1)
    {CMD_SET_FWOWN, priv_driver_set_fwown},
    {CMD_GET_FWOWN, priv_driver_get_fwown},
#endif /* #if defined(_HIF_AXI) && (CFG_SUPPORT_MANUAL_OWN_CTRL == 1) */
#if (CFG_SUPPORT_PRIV_CHIP_CONFIG == 1)
    {CMD_SET_CHIP, priv_driver_set_chip_config},
    {CMD_GET_CHIP, priv_driver_get_chip_config},
#endif /* #if (CFG_SUPPORT_PRIV_CHIP_CONFIG == 1) */
#if (CFG_SUPPORT_PRIV_RUN_HQA == 1)
    {CMD_RUN_HQA, priv_driver_run_hqa},
#endif /* #if (CFG_SUPPORT_PRIV_RUN_HQA == 1) */
#if (CFG_SUPPORT_CSI == 1)
    {CMD_SET_CSI, priv_driver_set_csi},
#endif /* #if (CFG_SUPPORT_CSI == 1) */
#if (CFG_SUPPORT_PRIV_GET_MIB_INFO == 1)
    {CMD_GET_MIB_INFO, priv_driver_get_mib_info},
#endif /* #if (CFG_SUPPORT_PRIV_GET_MIB_INFO == 1) */
#if (CFG_SUPPORT_PRIV_GET_DRV_VER == 1)
    {CMD_GET_VERSION, priv_driver_get_version},
#endif /* #if (CFG_SUPPORT_PRIV_GET_DRV_VER == 1) */
#if (CFG_SUPPORT_PRIV_FIXED_RATE == 1)
    {CMD_SET_FIXED_RATE, priv_driver_set_fixed_rate},
#endif /* #if (CFG_SUPPORT_PRIV_FIXED_RATE == 1) */
#if (CFG_SUPPORT_PRIV_STA_STAT == 1)
    {CMD_GET_STA_STAT, priv_driver_get_sta_stat},
#endif /* #if (CFG_SUPPORT_PRIV_STA_STAT == 1) */
#if (CFG_SUPPORT_PRIV_WTBL_INFO == 1)
    {CMD_GET_WTBL_INFO, priv_driver_get_wtbl_info},
#endif /* #if (CFG_SUPPORT_PRIV_WTBL_INFO == 1) */
#if (CFG_SUPPORT_TWT == 1)
    {CMD_SET_TWT_PARAMS, priv_driver_set_twtparams},
#endif /* #if (CFG_SUPPORT_TWT == 1) */
#if (CFG_SUPPORT_PRIV_SUSPEND_MODE == 1)
    {CMD_SETSUSPENDMODE, priv_driver_set_suspend_mode},
#endif /* #if (CFG_SUPPORT_PRIV_SUSPEND_MODE == 1) */
#if CFG_SUPPORT_SINGLE_SKU || CFG_SUPPORT_SINGLE_SKU_LOCAL_DB
    {CMD_COUNTRY, priv_driver_set_country},
#endif /* #if CFG_SUPPORT_SINGLE_SKU || CFG_SUPPORT_SINGLE_SKU_LOCAL_DB */
    {CMD_GET_COUNTRY, priv_driver_get_country},
    {CMD_GET_CHANNELS, priv_driver_get_channels},
#if CFG_WOW_SUPPORT
    {CMD_WOW_START, priv_driver_set_wow},
    {CMD_SET_WOW_ENABLE, priv_driver_set_wow_enable},
    {CMD_SET_WOW_PAR, priv_driver_set_wow_par},
    {CMD_SET_WOW_UDP, priv_driver_set_wow_udpport},
    {CMD_SET_WOW_TCP, priv_driver_set_wow_tcpport},
    {CMD_GET_WOW_PORT, priv_driver_get_wow_port},
    {CMD_GET_WOW_REASON, priv_driver_get_wow_reason},
#endif /* #if CFG_WOW_SUPPORT */
    {CMD_SET_ADV_PWS, priv_driver_set_adv_pws},
    {CMD_SET_MDTIM, priv_driver_set_mdtim},
    {CMD_GET_QUE_INFO, priv_driver_get_que_info},
#ifdef MTK_SIGMA_ENABLE
    {CMD_SET_NSS, priv_driver_set_nss},
#if (CFG_SUPPORT_802_11AX == 1)
    {CMD_SET_TX_MCSMAP, priv_driver_set_mcsmap},
    {CMD_SET_TX_PPDU, priv_driver_set_tx_ppdu},
    {CMD_SET_BA_SIZE, priv_driver_set_ba_size},
#endif /* #if (CFG_SUPPORT_802_11AX == 1) */
#endif /* #ifdef MTK_SIGMA_ENABLE */
#if (CFG_WIFI_GET_MCS_INFO == 1)
    {CMD_GET_MCS_INFO, priv_driver_get_mcs_info},
#endif /* #if (CFG_WIFI_GET_MCS_INFO == 1) */
#if CFG_ENABLE_WFDMA_DVT
    {CMD_SET_DVT_INIT, priv_driver_wfdma_dvt_init},
    {CMD_SET_DVT_START, priv_driver_wfdma_dvt_start},
    {CMD_SET_DVT_STOP, priv_driver_wfdma_dvt_stop},
    {CMD_GET_DVT_INFO, priv_driver_wfdma_show_info},
#endif /* #if CFG_ENABLE_WFDMA_DVT */
#if CFG_SUPPORT_ADVANCE_CONTROL
    {CMD_SET_NOISE, priv_driver_set_noise},
    {CMD_GET_NOISE, priv_driver_get_noise},
#endif /* #if CFG_SUPPORT_ADVANCE_CONTROL */
#if CFG_SUPPORT_TRAFFIC_REPORT
    {CMD_TRAFFIC_REPORT, priv_driver_get_traffic_report},
#endif /* #if CFG_SUPPORT_TRAFFIC_REPORT */
#if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1)
    {CMD_ARP_OFFLOAD, priv_driver_set_arp_offload},
#endif /* #if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1) */
};
#define PRIV_CMD_HANDLER_ENTRIES (sizeof(priv_cmd_handlers) / sizeof(struct PRIV_CMD_HANDLER))

static int priv_cmd_not_support(struct GLUE_INFO *prGlueInfo,
                                char *pcCommand, int32_t i4TotalLen)
{
    uint8_t i = 0;
    int32_t i4BytesWritten = 0;
    char *head = NULL;
    int ret = 0;

    head = kalMemAlloc(i4TotalLen, VIR_MEM_TYPE);
    if (!head) {
        i4BytesWritten = snprintf(pcCommand, i4TotalLen,
                                  "[%s] %s\r\n",
                                  __func__, "malloc error");
        if (i4BytesWritten < 0)
            goto invalid_ret;
    } else {

        /* prevent coverity issue */
        ret = snprintf(head, i4TotalLen, "[%s] not supported\r\n, list:", pcCommand);
        if (ret < 0)
            goto invalid_ret;

        i4BytesWritten = snprintf(pcCommand, i4TotalLen, "%s", head);
        kalMemFree(head, VIR_MEM_TYPE, i4TotalLen);
        if (i4BytesWritten < 0)
            goto invalid_ret;

        /* prevent coverity issue */
        for (i = 0; i < PRIV_CMD_HANDLER_ENTRIES; i++) {
            ret = snprintf(pcCommand + i4BytesWritten,
                           i4TotalLen - i4BytesWritten,
                           "[%d][%s]\r\n",
                           i, priv_cmd_handlers[i].pcCmdStr);
            if (ret < 0)
                goto invalid_ret;
            i4BytesWritten += ret;
        }
    }

    return i4BytesWritten;
invalid_ret:
    if (head != NULL)
        kalMemFree(head, VIR_MEM_TYPE, i4TotalLen);
    DBGLOG(REQ, ERROR, "snprintf return invalid ret\n");
    return -1;
}

int32_t priv_driver_cmds(struct GLUE_INFO *prGlueInfo, char *pcCommand,
                         int32_t i4TotalLen)
{
    int32_t i4BytesWritten = 0;
    int32_t i4CmdFound = 0;
    uint8_t i;

    if (!prGlueInfo) {
        DBGLOG(REQ, WARN, "OS info is not init, skip priv_driver_cmds\n");
        return -1;
    }

    if (g_u4HaltFlag) {
        DBGLOG(REQ, WARN, "wlan is halt, skip priv_driver_cmds\n");
        return -1;
    }

    for (i = 0; i < sizeof(priv_cmd_handlers) / sizeof(struct
                                                       PRIV_CMD_HANDLER); i++) {
        if (strnicmp(pcCommand,
                     (char *)priv_cmd_handlers[i].pcCmdStr,
                     strlen((char *)priv_cmd_handlers[i].pcCmdStr)) == 0) {

            if (priv_cmd_handlers[i].pfHandler != NULL) {
                i4BytesWritten =
                    priv_cmd_handlers[i].pfHandler(
                        prGlueInfo,
                        pcCommand,
                        i4TotalLen);
            }
            i4CmdFound = 1;
        }
    }

    if (i4CmdFound == 0)
        i4BytesWritten = priv_cmd_not_support(prGlueInfo,
                                              pcCommand, i4TotalLen);

    if (i4BytesWritten >= 0) {
        if ((i4BytesWritten == 0) && (i4TotalLen > 0)) {
            /* reset the command buffer */
            pcCommand[0] = '\0';
        }

        if (i4BytesWritten >= i4TotalLen) {
            DBGLOG(REQ, INFO,
                   "%s: i4BytesWritten %d > i4TotalLen < %d\n",
                   __func__, i4BytesWritten, i4TotalLen);
            i4BytesWritten = i4TotalLen;
        } else {
            pcCommand[i4BytesWritten] = '\0';
            i4BytesWritten++;
        }
    }

    return i4BytesWritten;

} /* priv_driver_cmds */

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
/* dynamic tx power control */
static int priv_driver_set_power_control(IN struct net_device *prNetDev,
                                         IN char *pcCommand, IN int i4TotalLen)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    struct PARAM_TX_PWR_CTRL_IOCTL rPwrCtrlParam = { 0 };
    u_int8_t fgIndex = FALSE;
    char *ptr = pcCommand, *ptr2 = NULL;
    char *str = NULL, *cmd = NULL, *name = NULL, *setting = NULL;
    uint8_t index = 0;
    uint32_t u4SetInfoLen = 0;

    while ((str = strsep(&ptr, " ")) != NULL) {
        if (kalStrLen(str) <= 0)
            continue;
        if (cmd == NULL)
            cmd = str;
        else if (name == NULL)
            name = str;
        else if (fgIndex == FALSE) {
            ptr2 = str;
            if (kalkStrtou8(str, 0, &index) != 0) {
                DBGLOG(REQ, INFO,
                       "index is wrong: %s\n", ptr2);
                return -1;
            }
            fgIndex = TRUE;
        } else if (setting == NULL) {
            setting = str;
            break;
        }
    }

    if ((name == NULL) || (fgIndex == FALSE)) {
        DBGLOG(REQ, INFO, "name(%s) or fgIndex(%d) is wrong\n",
               name, fgIndex);
        return -1;
    }

    rPwrCtrlParam.fgApplied = (index == 0) ? FALSE : TRUE;
    rPwrCtrlParam.name = name;
    rPwrCtrlParam.index = index;
    rPwrCtrlParam.newSetting = setting;

    DBGLOG(REQ, INFO, "applied=[%d], name=[%s], index=[%u], setting=[%s]\n",
           rPwrCtrlParam.fgApplied,
           rPwrCtrlParam.name,
           rPwrCtrlParam.index,
           rPwrCtrlParam.newSetting);

    prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
    kalIoctl(prGlueInfo,
             wlanoidTxPowerControl,
             (void *)&rPwrCtrlParam,
             sizeof(struct PARAM_TX_PWR_CTRL_IOCTL),
             FALSE,
             FALSE,
             TRUE,
             &u4SetInfoLen);

    return 0;
}
#endif /* #if CFG_SUPPORT_DYNAMIC_PWR_LIMIT */

#if defined(MTK_MINICLI_ENABLE)
#if (CFG_SUPPORT_PRIV_SET_PM == 1)
static int priv_set_power_mode(
    IN struct GLUE_INFO *prGlueInfo,
    IN char *commmand)
{
    enum PARAM_POWER_MODE ePowerMode;
    struct PARAM_POWER_MODE_ rPowerMode;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;

    if (prGlueInfo == NULL)
        return -1;

    rPowerMode.ucBssIdx =
        prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    rPowerMode.ePowerMode = Param_PowerModeCAM;

    ePowerMode = (uint8_t)kalAtoi((uint8_t)commmand[0]);
    if (ePowerMode < Param_PowerModeMax)
        rPowerMode.ePowerMode = ePowerMode;

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSet802dot11PowerSaveProfile,
                       &rPowerMode, sizeof(struct PARAM_POWER_MODE_),
                       FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS)
        return -1;

    return 0;
} /* priv_set_power_mode */

static int priv_get_power_mode(
    IN struct GLUE_INFO *prGlueInfo,
    OUT char *command)
{
    enum PARAM_POWER_MODE ePowerMode;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;

    if (prGlueInfo == NULL || command == NULL)
        return -1;

    rStatus = wlanQueryInformation(prGlueInfo->prAdapter,
                                   wlanoidQuery802dot11PowerSaveProfile,
                                   &ePowerMode, sizeof(ePowerMode),
                                   &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS)
        return -1;

    if (ePowerMode == Param_PowerModeCAM)
        kalSnprintf(command, 7, "CAM%4u", ePowerMode);
    else if (ePowerMode == Param_PowerModeMAX_PSP)
        kalSnprintf(command, 11, "MAX_PSP%4u", ePowerMode);
    else if (ePowerMode == Param_PowerModeFast_PSP)
        kalSnprintf(command, 12, "Fast_PSP%4u", ePowerMode);
    else
        kalSnprintf(command, 18, "unknown error!%4u", ePowerMode);

    return 0;
} /* priv_get_power_mode */
#endif /* #if (CFG_SUPPORT_PRIV_SET_PM == 1) */

#if CFG_SUPPORT_CSI
int priv_driver_set_csi(IN struct GLUE_INFO *prGlueInfo,
                        IN char *pcCommand, IN int i4TotalLen)
{
    int ret = 0;

    ret = wifi_config_set_csi(pcCommand, i4TotalLen);

    return ret;
}
#endif /* #if CFG_SUPPORT_CSI */

static int wlan_conbine_cmd(int n_param,
                            char **param, char *command, int max_len)
{
    int i = 0;
    int tmp_len = 0;
    int cmd_len = 0;
    char *tmp_cmd = command;

    DBGLOG(INIT, TRACE, "%s command buffer %p\r\n", __func__, command);
    DBGLOG(INIT, TRACE, "%s n_param: %d\r\n", __func__, n_param);
    for (i = 0; i < n_param; i++) {
        /* for cmd + space */
        tmp_len = strlen(param[i]) + 1;
        cmd_len += tmp_len;
        if (cmd_len > max_len)
            return -1;

        if (sprintf(tmp_cmd, "%s ", param[i]) < 0)
            DBGLOG(REQ, ERROR, "kalSnprintf fail\n");
        tmp_cmd += tmp_len;
        DBGLOG(INIT, TRACE, "append param[%d] %s, cmd %s\r\n",
               i, param[i], command);
    }
    /* command[cmd_len] = '\0'; */

    DBGLOG(INIT, TRACE, "%s length: %d\r\n", __func__, cmd_len);
    DBGLOG(INIT, TRACE, "%s command: %s\r\n", __func__, command);

    return cmd_len;
}

static int wlan_priv_command(int n_param, char **param,
                             char *command, int max_len)
{
    /* char* tmpcmd = NULL; */
    int ret = -2;
    int cmd_len = 0;
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;

    DBGLOG(INIT, TRACE, "%s command buffer %p\r\n", __func__, command);
    if (strncasecmp(param[0], "driver", strlen("driver")) == 0 &&
        n_param >= 2) {
        cmd_len = wlan_conbine_cmd(n_param - 1, &param[1], command, max_len);

        if (cmd_len <= 0)
            return ret;

        DBGLOG(INIT, TRACE, "driver command: %s\n", command);
        ret = priv_driver_cmds(prGlueInfo, command, max_len);
        /* free(tmpcmd); */
#if (CFG_SUPPORT_PRIV_SET_PM == 1)
    } else if (strncasecmp(param[0], "set_power_mode",
                           strlen("set_power_mode")) == 0 && n_param >= 2) {
        ret = priv_set_power_mode(prGlueInfo, param[1]);
    } else if (strncasecmp(param[0], "get_power_mode",
                           strlen("get_power_mode")) == 0) {
        ret = priv_get_power_mode(prGlueInfo, command);
#endif /* #if (CFG_SUPPORT_PRIV_SET_PM == 1) */
#if (CFG_SUPPORT_PRIV_RUN_HQA == 1)
#if (CFG_SUPPORT_QA_TOOL != 1) && (CONFIG_WLAN_SERVICE == 1)
    } else if (strncasecmp(param[0], "set",
                           strlen("set")) == 0) {
        ret = __priv_ate_set(prGlueInfo, &param[1], max_len);
#endif /* #if (CFG_SUPPORT_QA_TOOL != 1) && (CONFIG_WLAN_SERVICE == 1) */
#endif /* #if (CFG_SUPPORT_PRIV_RUN_HQA == 1) */
    } else {
        DBGLOG(INIT, ERROR, "driver command not support\n");
        ret = -EOPNOTSUPP;
    }
    return ret;
}

uint8_t _wifi_set_init_dbg(uint8_t len, char *param[])
{
    uint8_t i = 0;
    uint32_t lvl = 0x3f;
    uint32_t ret = 0;

    LOG_FUNC("# param: %u\r\t", len);

    for (i = 0; i < len; i++)
        LOG_FUNC("[%d]param: %s\r\t", i, param[i]);

    if (len != 1)
        return -1;

    ret = kalkStrtou32((char *)param[0], 0, &lvl);

    if (ret)
        LOG_FUNC("parse param error %u\r\t", ret);
    else {
        LOG_FUNC("set init debug level %x\r\t", lvl);
        gu4DbgLevl = lvl;
        wlanSetDriverDbgLevel(DBG_ALL_MODULE_IDX, lvl);
    }
    return 0;
}

uint8_t _wifi_try_get_semaphore(uint8_t len, char *param[])
{
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
    struct completion *comp = NULL;
    uint32_t ret = 0;

    if (!prGlueInfo) {
        LOG_FUNC("Please do wifi on first\r\n");
        return -1;
    }

    comp = &prGlueInfo->rPendComp;

    ret = xSemaphoreTake(comp->wait_lock, 0);
    if (ret == pdTRUE) {
        LOG_FUNC("comp->wait_lock is fine\r\n");
        xSemaphoreGive(comp->wait_lock);
    } else
        LOG_FUNC("comp->wait_lock is locked %d\r\n", comp->done);

    ret = xSemaphoreTake(comp->wait_complete, 0);
    if (ret == pdTRUE) {
        LOG_FUNC("comp->wait_complete is fine\r\n");
        xSemaphoreGive(comp->wait_complete);
    } else
        LOG_FUNC("comp->wait_complete is locked\r\n");

    return 0;
}

uint8_t _wifi_get_init_dbg(uint8_t len, char *param[])
{
    LOG_FUNC("WIFI DBG LVL: %x\r\n", gu4DbgLevl);
    return 0;
}

static void _gl_wificli_print_stainfo(struct GLUE_INFO *prGlueInfo,
                                      struct netif *prNetif)
{
    struct ADAPTER *prAdapter = NULL;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex = 0;
    uint8_t ucWlanIndex = 0;
    uint8_t ucStaIndex = 0;

    prAdapter = prGlueInfo->prAdapter;

    ucBssIndex = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

    LOG_FUNC("STA INFO:\r\n");
    LOG_FUNC("interface name: %s\r\n", prNetif->name);
    LOG_FUNC("interface flags: %x\r\n", prNetif->flags);
    LOG_FUNC("Test Mode: %d\r\n", prAdapter->rWifiVar.ucEfuseBufferModeCal);
    LOG_FUNC("mac: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
             prNetif->hwaddr[0],
             prNetif->hwaddr[1],
             prNetif->hwaddr[2],
             prNetif->hwaddr[3],
             prNetif->hwaddr[4],
             prNetif->hwaddr[5]);
    if (wlanGetWlanIdxByAddress(prAdapter, prNetif->hwaddr, &ucWlanIndex)
        == 0)
        LOG_FUNC("Get wlanIndex fail.");
    else {
        if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIndex, &ucStaIndex)
            == 0) {
            LOG_FUNC("wlanIdx: %u\r\n", ucWlanIndex);
            LOG_FUNC("staIdx: %u\r\n", ucStaIndex);
        } else
            LOG_FUNC("wlanIdx: %u\r\n", ucWlanIndex);
    }
    LOG_FUNC("Connect Status: %s\r\n",
             prBssInfo->eConnectionState != 0 ? "Not Connected" :
             "Connected");

    if (prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
        LOG_FUNC("Ssid: %.*s\r\n", prBssInfo->ucSSIDLen,
                 prBssInfo->aucSSID);
        LOG_FUNC("BssId: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                 prBssInfo->aucBSSID[0],
                 prBssInfo->aucBSSID[1],
                 prBssInfo->aucBSSID[2],
                 prBssInfo->aucBSSID[3],
                 prBssInfo->aucBSSID[4],
                 prBssInfo->aucBSSID[5]);
        LOG_FUNC("BssIndex: %u\r\n", ucBssIndex);
        LOG_FUNC("Band: %s (%x)\r\n",
                 prBssInfo->eBand == BAND_2G4 ? "2G" :
                 prBssInfo->eBand == BAND_5G ? "5G" :
                 "n/a", prBssInfo->eBand);
        LOG_FUNC("CH: %u\r\n", prBssInfo->ucPrimaryChannel);
        LOG_FUNC("BW: %x\r\n", prBssInfo->ucOpChangeChannelWidth);

        if (prBssInfo->prStaRecOfAP) {
            ucWlanIndex = prBssInfo->prStaRecOfAP->ucWlanIndex;
            ucStaIndex = prBssInfo->prStaRecOfAP->ucIndex;

            LOG_FUNC("[0]STA INFO under BSS: %x\r\n",
                     prBssInfo->prStaRecOfAP->fgIsInUse);
            LOG_FUNC("wlanIdx: %u\r\n", ucWlanIndex);
            LOG_FUNC("staIdx: %u\r\n", ucStaIndex);
            LOG_FUNC("MAC_ADDR: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                     prBssInfo->prStaRecOfAP->aucMacAddr[0],
                     prBssInfo->prStaRecOfAP->aucMacAddr[1],
                     prBssInfo->prStaRecOfAP->aucMacAddr[2],
                     prBssInfo->prStaRecOfAP->aucMacAddr[3],
                     prBssInfo->prStaRecOfAP->aucMacAddr[4],
                     prBssInfo->prStaRecOfAP->aucMacAddr[5]);
        }
#if LWIP_DHCP
        if (lwip_get_ipmode() == REQ_IP_MODE_DHCP) {
            LOG_FUNC("mode: dhcp\r\n");
            if (dhcp_supplied_address(prNetif)) {
                struct dhcp *d = netif_dhcp_data(prNetif);

                LOG_FUNC("ip: %s\r\n",
                         ip4addr_ntoa(&d->offered_ip_addr));
                LOG_FUNC("netmask: %s\r\n",
                         ip4addr_ntoa(&d->offered_sn_mask));
                LOG_FUNC("gateway: %s\r\n",
                         ip4addr_ntoa(&d->offered_gw_addr));
            } else
                LOG_FUNC("dhcp on going\r\n");
        } else
#endif /* #if LWIP_DHCP */
        {
            LOG_FUNC("mode: static\r\n");
            LOG_FUNC("ip: %s\r\n", ip4addr_ntoa(ip_2_ip4(&prNetif->ip_addr)));
            LOG_FUNC("netmask: %s\r\n",
                     ip4addr_ntoa(ip_2_ip4(&prNetif->netmask)));
            LOG_FUNC("gateway: %s\r\n", ip4addr_ntoa(ip_2_ip4(&prNetif->gw)));
        }
    }
}

static void _gl_wificli_print_apinfo(struct GLUE_INFO *prGlueInfo,
                                     struct netif *netif)
{
}

uint8_t _wifi_info(uint8_t len, char *param[])
{
    if (!g_prGlueInfo) {
        LOG_FUNC("[ERROR]Init glueInfo first\r\n");
        return 0;
    }

    if (wlan_sta_netif)
        _gl_wificli_print_stainfo(g_prGlueInfo, wlan_sta_netif);

    if (wlan_ap_netif)
        _gl_wificli_print_apinfo(g_prGlueInfo, wlan_ap_netif);

    return 0;
}

uint8_t _wifi_isconnected(void)
{
    struct ADAPTER *prAdapter = NULL;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex = 0;

    if (!g_prGlueInfo) {
        LOG_FUNC("[ERROR]Init glueInfo first\r\n");
        return 0;
    }

    prAdapter  = g_prGlueInfo->prAdapter;
    ucBssIndex = prAdapter->prAisBssInfo[0]->ucBssIndex;
    prBssInfo  = prAdapter->aprBssInfo[ucBssIndex];

    return ((prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) ? 1 : 0);
}


const cmd_t   wifi_config_set_pub_cli[] = {
    { "opmode", "STA/AP", wifi_config_set_opmode_ex, NULL },
    { "ssid", "SSID", wifi_config_set_ssid_ex, NULL },
    { "bssid", "BSSID", wifi_config_set_bssid_ex, NULL },
    { "sec", "Security", wifi_config_set_security_mode_ex, NULL },
    { "msec", "multiple security", wifi_config_set_multi_security_mode_ex, NULL },
    { "ieee80211w", "ieee80211w", wifi_config_set_ieee80211w_ex, NULL },
    { "proto", "proto", wifi_config_set_proto_ex, NULL },
    { "psk", "wpa psk key", wifi_config_set_psk_ex, NULL },
    { "pmk", "pmk key", wifi_config_set_pmk_ex, NULL },
    { "wep", "wep key", wifi_config_set_wep_key_ex, NULL },
    { "bw", "bandwidth", wifi_config_set_bandwidth_ex, NULL },
    { "channel", "channel", wifi_config_set_channel_ex, NULL },
    { "multi_channel", "multi_channel", wifi_config_set_multi_channel_ex, NULL },
    { "wirelessmode", "wireless mode", wifi_config_set_wireless_mode_ex, NULL },
    { "country_code", "country code", wifi_config_set_country_code_ex, NULL },
    { "bss_pref", "BSS preference", wifi_config_set_bss_preference_ex, NULL },
    { "radio", "OFF/ON", wifi_config_set_radio_on_ex, NULL },
    { "ps_mode", "PS mode", wifi_config_set_ps_mode_ex, NULL },
    { "listen", "listen interval", wifi_config_set_listen_interval_ex, NULL },
    {
        "pretbtt", "set pretbtt value in psmode",
        wifi_config_set_pretbtt_ex, NULL
    },
    {
        "bcn_lost", "clear beacon lost count",
        wifi_config_clear_bcn_lost_cnt_ex, NULL
    },
    { "ps_log", "powersave log on/off", wifi_config_set_ps_log_ex, NULL },
    { "reload", "reload", wifi_config_set_reload_ex, NULL },
    { "dtim", "dtim", wifi_config_set_dtim_interval_ex, NULL },
    { "bcn_int", "bcn_int", wifi_config_set_bcn_interval_ex, NULL },
    { "retry_limit", "retry limit", wifi_config_set_retry_limit_ex, NULL },
    { "tx_rate", "tx rate", wifi_config_set_tx_rate_ex, NULL },
#if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1)
    { "arp_offload", "arp offload", wifi_config_set_arp_offload_ex, NULL },
#endif /* #if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1) */
    { "rx_filter", "set RX Filter", wifi_config_set_rx_filter_ex, NULL },
    { "mc_address", "set mc address", wifi_config_set_mc_address_ex, NULL },
    { "rxraw", "set RX RAW packet", wifi_config_set_rx_raw_pkt_ex, NULL },
    {
        "wow_handler", "set magic packet handler for wow enable",
        wifi_config_set_wow_handler_ex, NULL
    },
    { "rx_mode", "set Rx handler mode", wifi_config_set_rx_handler_mode_ex, NULL },
    { "wow", "start/stop wow mode", wifi_config_set_wow_ex, NULL },
    { "wow_udp", "set UDP IPv4 wow wakeup port", wifi_config_set_wow_udp_ex, NULL },
    { "wow_udp_del", "del UDP IPv4 wow wakeup port", wifi_config_set_wow_udp_del_ex, NULL },
    { "wow_tcp", "set TCP IPv4 wow wakeup port", wifi_config_set_wow_tcp_ex, NULL },
    { "wow_tcp_del", "del TCP IPv4 wow wakeup port", wifi_config_set_wow_tcp_del_ex, NULL },
#if IP_NAPT
    { "napt_timer", "set NAPT timer", wifi_config_set_napt_entry_timer_ex, NULL },
#endif /* #if IP_NAPT */
#if CFG_SUPPORT_ROAMING_CUSTOMIZED
    { "rssidelta", "set RSSI delta", wifi_config_set_roam_delta_ex, NULL },
    {
        "roamscanch", "set roaming scan channel",
        wifi_config_set_roam_scan_channel_ex, NULL
    },
    {
        "clearroamstatistic", "set clearroamstatistic",
        wifi_config_clear_roam_statistic_ex, NULL
    },
    {
        "RoamingByRssi", "set RoamByRssi",
        wifi_config_set_roam_by_rssi_ex, NULL
    },
    {
        "RoamingRssiValue", "set RoamRssiValue",
        wifi_config_set_roam_rssithreshold_ex, NULL
    },
    {
        "RoamingByBcn", "set RoamByBcn",
        wifi_config_set_roam_by_bcnmiss_ex, NULL
    },
    {
        "ScreenOnBeaconTimeoutCount", "set ScreenOnBeaconTimeoutCount",
        wifi_config_set_roam_by_bcnmissthreshold_ex, NULL
    },
    {
        "RoamingBlockTimeSec", "set RoamingBlockTimeSec",
        wifi_config_set_roam_block_time_ex, NULL
    },
    {
        "RoamingRetryTimeSec", "set RoamingRetryTimeSec",
        wifi_config_set_roam_lock_time_ex, NULL
    },
    {
        "RoamingRetryLimit", "set RoamingRetryLimit",
        wifi_config_set_roam_maxlock_count_ex, NULL
    },
    {
        "RoamingBeaconTimeSec", "set RoamingBeaconTimeSec",
        wifi_config_set_bto_time_ex, NULL
    },
    {
        "RoamingStableTimeSec", "set RoamingStableTimeSec",
        wifi_config_set_stable_time_ex, NULL
    },
    {
        "splitscan", "set splitscan",
        wifi_config_set_split_scan_ex, NULL
    },
    {
        "autoroam", "set autoroam",
        wifi_config_set_autoroam_ex, NULL
    },
#endif /* #if CFG_SUPPORT_ROAMING_CUSTOMIZED */
#if CFG_SUPPORT_CSI
    {
        "csi", "set csi",
        wifi_set_csi_ex, NULL
    },
#endif /* #if CFG_SUPPORT_CSI */
#if CFG_CHIP_RESET_SUPPORT
    {
        "ser", "set ser",
        wifi_config_set_ser_ex, NULL
    },
#endif /* #if CFG_CHIP_RESET_SUPPORT */
    { "twt", "set TWT mode", wifi_config_set_twt_ex, NULL },
    { "bcdrop", "set BC data Drop", wifi_config_set_bc_drop_ex, NULL },
#if CFG_SUPPORT_NON_PREF_CHAN
    {
        "non_pref_chan", "set non_pref_chan",
        wifi_config_set_non_pref_chan_ex, NULL
    },
#endif /* #if CFG_SUPPORT_NON_PREF_CHAN */
#if CFG_SUPPORT_11KV_SWITCH
    {
        "disable_11k", "set disable_11k",
        wifi_config_set_disable_11k_ex, NULL
    },
    {
        "disable_11v", "set disable_11v",
        wifi_config_set_disable_11v_ex, NULL
    },
#endif /* #if CFG_SUPPORT_11KV_SWITCH */
#if CFG_SUPPORT_ANT_DIV
    { "antmode", "antenna diversity mode", wifi_config_set_antdiv_mode_ex, NULL },
#endif /* #if CFG_SUPPORT_ANT_DIV */
#if CFG_SUPPORT_SCAN_CH_TIME
    {
        "scnChTime", "set scan channel dwell time",
        wifi_config_set_dwll_time_ex, NULL
    },
    {
        "scnDfsChActive", "set scan channel DFS dwell time in active scan",
        wifi_config_set_DFS_dwll_time_ex, NULL
    },
#endif /* #if CFG_SUPPORT_SCAN_CH_TIME */
    { NULL }
};


const cmd_t   wifi_config_get_pub_cli[] = {
    { "opmode", "STA/AP", wifi_config_get_opmode_ex, NULL },
    { "mac", "MAC address", wifi_config_get_mac_address_ex, NULL },
    { "ssid", "SSID", wifi_config_get_ssid_ex, NULL },
    { "bssid", "BSSID", wifi_config_get_bssid_ex, NULL },
    { "sec", "Security Mode", wifi_config_get_security_mode_ex, NULL },
    { "psk", "wpa psk key", wifi_config_get_psk_ex, NULL },
    { "pmk", "pmk key", wifi_config_get_pmk_ex, NULL },
    { "wep", "wep key", wifi_config_get_wep_key_ex, NULL },
    { "bw", "bandwidth", wifi_config_get_bandwidth_ex, NULL },
    { "channel", "channel", wifi_config_get_channel_ex, NULL },
    { "wirelessmode", "wireless mode", wifi_config_get_wireless_mode_ex, NULL },
    { "chbw", "channel bandwidth", wifi_config_get_chbw_ex, NULL },
    { "country_code", "country code", wifi_config_get_country_code_ex, NULL },
    { "radio", "OFF/ON", wifi_config_get_radio_on_ex, NULL },
    {
        "connected_ap_info", "get connected ap info",
        wifi_config_get_connected_ap_info_ex, NULL
    },
    { "wlanstat", "get wlan statistic", wifi_config_get_statistic_ex, NULL },
    { "ps_mode", "PS mode", wifi_config_get_ps_mode_ex, NULL },
    { "dtim", "DTIM interval", wifi_config_get_dtim_interval_ex, NULL },
    { "listen", "listen interval", wifi_config_get_listen_interval_ex, NULL },
    {
        "pretbtt", "get pretbtt value in psmode",
        wifi_config_get_pretbtt_ex, NULL
    },
    {
        "bcn_lost", "get beacon lost count",
        wifi_config_get_bcn_lost_cnt_ex, NULL
    },
    { "ps_log", "powersave log on/off", wifi_config_get_ps_log_ex, NULL },
    {
        "sys_temperature", "get temperature",
        wifi_config_get_sys_temperature_ex, NULL
    },
    { "rx_filter", "get RX Filter", wifi_config_get_rx_filter_ex, NULL },
    { "wow", "get wow mode", wifi_config_get_wow_ex, NULL },
    { "wow_reason", "get wow reason of the last wakeup", wifi_config_get_wow_reason_ex, NULL },
    { "wow_udp", "get UDP IPv4 wow wakeup port", wifi_config_get_wow_udp_ex, NULL },
    { "wow_tcp", "get TCP IPv4 wow wakeup port", wifi_config_get_wow_tcp_ex, NULL },
#if CFG_SUPPORT_REG_RULES
    { "reg_rules", "check the reg rules", wifi_config_reg_rules_allow_channel_ex, NULL },
#endif /* #if CFG_SUPPORT_REG_RULES */
#if CFG_SUPPORT_ROAMING_CUSTOMIZED
    { "rssidelta", "get RSSI delta", wifi_config_get_roam_delta_ex, NULL },
    {
        "roamscanch", "get roaming scan channel",
        wifi_config_get_roam_scan_channel_ex, NULL
    },
    {
        "roamtype", "get roaming type",
        wifi_config_get_roam_type_ex, NULL
    },
    {
        "isconnectftap", "Is connected to FT AP?",
        wifi_config_is_connect_ft_ap_ex, NULL
    },
    {
        "roamstatistic", "get roamstatistic",
        wifi_config_get_roam_statistic_ex, NULL
    },
    {
        "RoamingByRssi", "get RoamByRssi",
        wifi_config_get_roam_by_rssi_ex, NULL
    },
    {
        "RoamingRssiValue", "get RoamRssiValue",
        wifi_config_get_roam_rssithreshold_ex, NULL
    },
    {
        "RoamingByBcn", "get RoamByBcn",
        wifi_config_get_roam_by_bcnmiss_ex, NULL
    },
    {
        "ScreenOnBeaconTimeoutCount", "get ScreenOnBeaconTimeoutCount",
        wifi_config_get_roam_by_bcnmissthreshold_ex, NULL
    },
    {
        "RoamingBlockTimeSec", "get RoamingBlockTimeSec",
        wifi_config_get_roam_block_time_ex, NULL
    },
    {
        "RoamingRetryTimeSec", "get RoamingRetryTimeSec",
        wifi_config_get_roam_lock_time_ex, NULL
    },
    {
        "RoamingRetryLimit", "get RoamingRetryLimit",
        wifi_config_get_roam_maxlock_count_ex, NULL
    },
    {
        "RoamingBeaconTimeSec", "get RoamingBeaconTimeSec",
        wifi_config_get_bto_time_ex, NULL
    },
    {
        "RoamingStableTimeSec", "get RoamingStableTimeSec",
        wifi_config_get_stable_time_ex, NULL
    },
    {
        "autoroam", "get autoroam",
        wifi_config_get_autoroam_ex, NULL
    },
#endif /* #if CFG_SUPPORT_ROAMING_CUSTOMIZED */
#if CFG_SUPPORT_11KV_SWITCH
    {
        "disable_11k", "get disable_11k",
        wifi_config_get_disable_11k_ex, NULL
    },
    {
        "disable_11v", "get disable_11v",
        wifi_config_get_disable_11v_ex, NULL
    },
#endif /* #if CFG_SUPPORT_11KV_SWITCH */
#if CFG_SUPPORT_ANT_DIV
    { "antmode", "antenna diversity mode", wifi_config_get_antdiv_mode_ex, NULL },
    { "antidx", "antenna diversity current index", wifi_config_get_antdiv_cur_idx_ex, NULL },
#endif /* #if CFG_SUPPORT_ANT_DIV */
    { "twt", "get twt param", wifi_config_get_twt_param_ex, NULL },
    { NULL }
};

cmd_t   wifi_connect_set_pub_cli[] = {
    { "disconap", "disconnect ap", wifi_connect_disconnect_ap_ex, NULL },
    { "scan", "start/stop", wifi_connect_set_scan_ex, NULL },
    { "sched_scan", "on/off", wifi_connect_set_sched_scan_ex, NULL },
    {
        "deauth", "de-authenticate specific STA",
        wifi_connect_deauth_station_ex, NULL
    },
    { "send_raw", "send raw 80211", wifi_connect_send_raw_ex, NULL},
    {
        "eventcb", "register/unregister event",
        wifi_connect_set_event_callback_ex, NULL
    },
    { NULL }
};

cmd_t   wifi_connect_get_pub_cli[] = {
    { "stalist", "get STA list", wifi_connect_get_station_list_ex, NULL },
    { "linkstatus", "link status", wifi_connect_get_link_status_ex, NULL },
    { "rssi", "RSSI", wifi_connect_get_rssi_ex, NULL },
    { NULL }
};

#ifdef MTK_WIFI_PROFILE_ENABLE
cmd_t   wifi_profile_set_pub_cli[] = {
    { "opmode", "operation mode", wifi_profile_set_opmode_ex, NULL },
    { "ssid", "SSID", wifi_profile_set_ssid_ex, NULL },
    {
        "wirelessmode", "wireless mode",
        wifi_profile_set_wireless_mode_ex, NULL
    },
    {
        "sec",  "WPA/WPA2PSK Authmode, Encrypt Type",
        wifi_profile_set_security_mode_ex, NULL
    },
    { "psk", "psk key", wifi_profile_set_psk_ex, NULL },
    { "wep", "wep key", wifi_profile_set_wep_key_ex, NULL },
    { "mac", "MAC address", wifi_profile_set_mac_address_ex, NULL },
    { "channel", "channel", wifi_profile_set_channel_ex, NULL },
    { "bw", "bandwidth", wifi_profile_set_bandwidth_ex, NULL },
    { "dtim", "DTIM interval", wifi_profile_set_dtim_interval_ex, NULL },
    { "listen", "listen interval", wifi_profile_set_listen_interval_ex, NULL },
    { "ps_mode", "PS mode", wifi_profile_set_power_save_mode_ex, NULL },
#if IP_NAPT
    { "napt_tcp_entry_num", "TCP entries", wifi_profile_set_napt_tcp_entry_num_ex, NULL },
    { "napt_udp_entry_num", "UDP entries", wifi_profile_set_napt_udp_entry_num_ex, NULL },
#endif /* #if IP_NAPT */
    { NULL }
};

cmd_t   wifi_profile_get_pub_cli[] = {
    { "opmode", "operation mode", wifi_profile_get_opmode_ex, NULL },
    { "ssid", "SSID", wifi_profile_get_ssid_ex, NULL },
    {
        "wirelessmode", "wireless mode",
        wifi_profile_get_wireless_mode_ex, NULL
    },
    {
        "sec", "WPA/WPA2PSK Authmode, Encrypt Type",
        wifi_profile_get_security_mode_ex, NULL
    },
    { "psk", "psk key", wifi_profile_get_psk_ex, NULL },
    { "wep", "wep key", wifi_profile_get_wep_key_ex, NULL },
    { "mac", "MAC address", wifi_profile_get_mac_address_ex, NULL },
    { "channel", "channel", wifi_profile_get_channel_ex, NULL },
    { "bw", "bandwidth", wifi_profile_get_bandwidth_ex, NULL },
    { "dtim", "DTIM interval", wifi_profile_get_dtim_interval_ex, NULL },
    { "listen", "listen interval", wifi_profile_get_listen_interval_ex, NULL },
    { "ps_mode", "PS mode", wifi_profile_get_power_save_mode_ex, NULL },
    { NULL }
};
#endif /* #ifdef MTK_WIFI_PROFILE_ENABLE */

static cmd_t wifi_config_pub_cli[] = {
    { "set", "wifi config set", NULL, wifi_config_set_pub_cli},
    { "get", "wifi config get", NULL, wifi_config_get_pub_cli},
    { NULL, NULL, NULL, NULL}
};

static cmd_t wifi_connect_pub_cli[] = {
    { "set", "wifi connect set", NULL, wifi_connect_set_pub_cli},
    { "get", "wifi connect get", NULL, wifi_connect_get_pub_cli},
    { NULL, NULL, NULL, NULL}
};

#ifdef MTK_WIFI_PROFILE_ENABLE
static cmd_t wifi_profile_pub_cli[] = {
    { "set", "wifi profile set", NULL, wifi_profile_set_pub_cli},
    { "get", "wifi profile get", NULL, wifi_profile_get_pub_cli},
    { NULL, NULL, NULL, NULL}
};
#endif /* #ifdef MTK_WIFI_PROFILE_ENABLE */

#ifdef MTK_SIGMA_ENABLE
const cmd_t   wifi_cert_set_pub_cli[] = {
    { "ip_mode", "dhcp/static", wifi_config_set_ip_mode_ex, NULL },
    { "ip_addr", "ipv4 static ip address", wifi_config_set_ip_addr_ex, NULL },
    { NULL }
};
const cmd_t wifi_cert_pub_cli[] = {
    { "set", "wifi cert set", NULL, wifi_cert_set_pub_cli},
    { NULL, NULL, NULL, NULL}
};
#endif /* #ifdef MTK_SIGMA_ENABLE */

const cmd_t wifi_init_cli[] = {
    { "on", "Wifi init", _wifi_on, NULL},
    { "info", "Wifi info", _wifi_info, NULL},
    { "set_dbg", "set init dbg level", _wifi_set_init_dbg, NULL},
    { "get_dbg", "get init dbg level", _wifi_get_init_dbg, NULL},
    { "config", "wifi config", NULL, wifi_config_pub_cli},
    { "connect", "wifi connect", NULL, wifi_connect_pub_cli},
#ifdef MTK_WIFI_PROFILE_ENABLE
    { "profile", "wifi profile", NULL, wifi_profile_pub_cli},
#endif /* #ifdef MTK_WIFI_PROFILE_ENABLE */
    { "init", "wifi init for API", wifi_init_ex, NULL},
    { "deinit", "wifi deinit for API", wifi_deinit_ex, NULL},
#if CFG_SUPPORT_802_11K
    { "neighbor_rep_request", "send neighbor report request", wifi_neighbor_rep_request_ex, NULL},
#endif /* #if CFG_SUPPORT_802_11K */
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
    { "wnm_bss_query", "send bss transition query", wifi_wnm_bss_query_ex, NULL},
#endif /* #if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT */
#ifdef MTK_SIGMA_ENABLE
    { "cert_config", "wifi config for cert", NULL, wifi_cert_pub_cli},
#endif /* #ifdef MTK_SIGMA_ENABLE */
    { NULL }
};

uint8_t iwpriv_cli(uint8_t len, char *param[])
{
    int ret = -2;
    char command[2000]; //command buffer: send command and return string

    if (len == 0) {
        DBGLOG(INIT, ERROR, "Wrong command!!\n");
        return 0;
    }

    memset(command, '\0', sizeof(command));

    DBGLOG(INIT, TRACE, "%s n_param: %d\r\n", __func__, len);
    if (strncasecmp(param[0], "wlan", strlen("wlan")) == 0)
        ret = wlan_priv_command(len - 1, &param[1], command, 2000); // send command and return buffer and buffer len
    else
        ret = -EOPNOTSUPP;

    DBGLOG(INIT, TRACE, "wlan_priv_command return value: %d\n", ret);
    if (ret >= 0) {
        char *token = command;
        char *end;
        LOG_FUNC("Command success.\r\n");
        if (strncasecmp(param[1], "driver", strlen("driver")) == 0)
            LOG_FUNC("wlan0\tdriver: \r\n");
        while ((end = strchr(token, '\n')) != NULL) {
            *end = '\0';
            /* syslog will add a newline */
            LOG_FUNC("%s", token);
            token = end + 1;
        }
        LOG_FUNC("%s", token);
    } else if (ret == -1)
        DBGLOG(INIT, ERROR, "command fail\n");
    else
        DBGLOG(INIT, ERROR, "command not supported\n");

    return 0;
}

/**
* @brief port sanity check
* @parameter [IN] port
*
* @return  >=0 means port, <0 means fail
*/
int32_t port_sanity_check(char *port_str)
{
    int32_t port = (int32_t)atoi(port_str);

    if (port > WIFI_PORT_AP) {
        LOG_FUNC("Invalid port argument: %d\n", (int)port);
        return -1;
    }
    return port;
}

int wifi_init_done_handler(wifi_event_t event, uint8_t *payload,
                           uint32_t length)
{
    LOG_FUNC("WIFI_IOT_EVENT_INIT_COMPLETE SUCCESS.\n");
    return 0;
}

uint8_t wifi_init_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    wifi_config_t config = {0};
    wifi_config_ext_t config_ext = {0};
    struct wifi_cfg wifi_config = {0};

    if (wifi_config_init(&wifi_config) != 0) {
        LOG_FUNC("wifi config init fail\n");
        return -1;
    }

    config.opmode = wifi_config.opmode;

    kalMemCopy(config.sta_config.ssid, wifi_config.sta_ssid,
               WIFI_MAX_LENGTH_OF_SSID);
    config.sta_config.ssid_length = wifi_config.sta_ssid_len;
    config.sta_config.bssid_present = 0;
    config.sta_config.channel = wifi_config.sta_channel;
    kalMemCopy(config.sta_config.password, wifi_config.sta_wpa_psk,
               WIFI_LENGTH_PASSPHRASE);
    config.sta_config.password_length = wifi_config.sta_wpa_psk_len;
    if (wifi_config.sta_default_key_id == 255)
        config_ext.sta_wep_key_index_present = 0;
    else
        config_ext.sta_wep_key_index_present = 1;
    config_ext.sta_wep_key_index = wifi_config.sta_default_key_id;
    config_ext.sta_listen_interval_present = 1;
    config_ext.sta_listen_interval = wifi_config.sta_listen_interval;
    config_ext.sta_power_save_mode_present = 1;
    config_ext.sta_power_save_mode = wifi_config.sta_power_save_mode;

    kalMemCopy(config.ap_config.ssid, wifi_config.ap_ssid,
               WIFI_MAX_LENGTH_OF_SSID);
    config.ap_config.ssid_length = wifi_config.ap_ssid_len;
    kalMemCopy(config.ap_config.password, wifi_config.ap_wpa_psk,
               WIFI_LENGTH_PASSPHRASE);
    config.ap_config.password_length = wifi_config.ap_wpa_psk_len;
    config.ap_config.auth_mode =
        (wifi_auth_mode_t)wifi_config.ap_auth_mode;
    config.ap_config.encrypt_type =
        (wifi_encrypt_type_t)wifi_config.ap_encryp_type;
    config.ap_config.channel = wifi_config.ap_channel;
    config_ext.ap_wep_key_index_present = 1;
    config_ext.ap_wep_key_index = wifi_config.ap_default_key_id;
    config_ext.ap_dtim_interval_present = 1;
    config_ext.ap_dtim_interval = wifi_config.ap_dtim_interval;

    wifi_connection_register_event_handler(
        (wifi_event_t)WIFI_EVENT_IOT_INIT_COMPLETE,
        (wifi_event_handler_t)wifi_init_done_handler);
    wifi_init(&config, &config_ext);
    wifi_connection_scan_init(g_ap_list, CFG_MAX_NUM_BSS_LIST);
    LOG_FUNC("wifi_init(), ret:%s, Code=%d\n", WIFI_CLI_RETURN_STRING(ret),
             (int)ret);
    return ret;
}

uint8_t wifi_deinit_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    int type = WIFI_RADIO_OFF;

    if (len == 1)
        type = atoi(param[0]);

    wifi_deinit(type);
    LOG_FUNC("wifi_deinit(), ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

#if CFG_SUPPORT_CSI
uint8_t wifi_set_csi_ex(uint8_t len, char *param[])
{
    int32_t i, ret = 0;
    char command[100];
    char csi_cmd[] = "set_csi";
    char *tmp_cmd = command;
    int32_t cmd_buf = sizeof(command);
    int ret_spf;

    kalMemSet(command, '\0', sizeof(command));
    /* prevent coverity issue */
    ret_spf = sprintf(tmp_cmd, "%s ", csi_cmd);
    if (ret_spf < 0)
        goto invalid_ret;

    tmp_cmd += strlen(csi_cmd) + 1;
    cmd_buf -= strlen(csi_cmd) + 1;

    for (i = 0; i < len; i++) {
        cmd_buf -= strlen(param[i]) + 1;
        if (cmd_buf > 0) {

            /* prevent coverity issue */
            ret_spf = sprintf(tmp_cmd, "%s ", param[i]);
            if (ret_spf < 0)
                goto invalid_ret;
            tmp_cmd += strlen(param[i]) + 1;
        } else {
            return -1;
        }
    }
    *tmp_cmd = '\n';

    ret = wifi_config_set_csi(command, strlen(command));

    return ret;
invalid_ret:
    DBGLOG(REQ, ERROR, "snprintf return invalid ret\n");
    return -1;
}
#endif /* #if CFG_SUPPORT_CSI */

#if CFG_SUPPORT_802_11K
/**
* @brief Example of Send Neighbor Report Request.
* wifi neighbor_rep_request <ssid>
* @param [IN]ssid
*
* @return  =0 means success, >0 means fail
*
*/
uint8_t wifi_neighbor_rep_request_ex(uint8_t len, char *param[])
{
    uint8_t *pucSSID = NULL;
    int32_t ret = 0;
    uint8_t ssid_length = 0;

    if (len == 1) {
        pucSSID = (uint8_t *)param[0];
        ssid_length = strlen((char *)pucSSID);
    }
    DBGLOG(REQ, INFO, "NEIGHBOR-REQUEST, ssid=%s", pucSSID);

    ret = wifi_neighbor_rep_request(pucSSID, ssid_length);

    LOG_FUNC("wifi_neighbor_rep_request(0), ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}
#endif /* #if CFG_SUPPORT_802_11K */

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
/**
* @brief Example of Send BSS Transition Query.
* wifi wnm_bss_query <reason>
* @param [IN]reason
*
* @return  =0 means success, >0 means fail
*
*/
uint8_t wifi_wnm_bss_query_ex(uint8_t len, char *param[])
{
    int reason = 0;
    int32_t ret = 0;

    if (len != 1) {
        LOG_USAGE("Usage: wifi wnm_bss_query <reason>");
        return -1;
    }
    reason = atoi(param[0]);
    LOG_FUNC("BSS-TRANSITION-QUERY, pucQueryReason=%d", reason);

    ret = wifi_wnm_bss_query(reason);
    LOG_FUNC("wifi_wnm_bss_query(0), ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}
#endif /* #if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT */

/**
* @brief Example of Set WiFi Operation Mode.
* wifi config set opmode <mode>
* @param [IN]mode
* @     1 WIFI_MODE_STA_ONLY
* @     2 WIFI_MODE_AP_ONLY
* @     3 WIFI_MODE_REPEATER
*
* @return  =0 means success, >0 means fail
*
* @note Set WiFi Operation Mode will RESET all the configuration set by
*       previous WIFI-CONFIG APIs
*/
uint8_t wifi_config_set_opmode_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t mode = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set opmode <mode>\n");
        return 1;
    }

    mode = (uint8_t)atoi(param[0]);
    ret =  wifi_config_set_opmode(mode);

    LOG_FUNC("wifi_config_set_opmode(%d), ret:%s, Code=%d\n",
             (unsigned int)mode, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}


/**
* @brief Example of Get WiFi Operation Mode.
* wifi config get opmode
* @param [OUT]mode
* @      1 WIFI_MODE_STA_ONLY
* @      2 WIFI_MODE_AP_ONLY
* @      3 WIFI_MODE_REPEATER
*
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_config_get_opmode_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t mode = 0;

    ret = wifi_config_get_opmode(&mode);

    LOG_FUNC("wifi_config_get_opmode(%d), ret:%s, Code=%d\n",
             (unsigned int)mode, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Get MAC address for STA/AP wireless port
*
*  wifi config get mac <port> --> get port0 (STA) MAC address
* @parameter
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_get_mac_address_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t addr[WIFI_MAC_ADDRESS_LENGTH] = {0};
    int32_t port = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get mac <port> \n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    ret = wifi_config_get_mac_address(port, addr);

    LOG_FUNC("wifi_config_get_mac_address(port%d): (%02x:%02x:%02x:%02x:%02x:%02x), ret:%s, Code=%d\n",
             (int)port,
             addr[0], addr[1], addr[2],
             addr[3], addr[4], addr[5],
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Configure bandwidth for STA/AP  wireless port.
*
* wifi config set bw <port>  <0:HT20/1:HT40>
* @parameter
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_bandwidth_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    int32_t port = 0;
    char bw = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set bw <port> <BW>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    bw = atoi(param[1]);
    if (port < 0)
        return -1;

    ret = wifi_config_set_bandwidth(port, bw);
    if (ret < 0)
        status = 1;

    LOG_FUNC("wifi_config_set_bandwidth(port%d): %d, ret:%s, Code=%d\n",
             (int)port, bw, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return status;
}

/**
* @brief Example of get bandwidth for STA/AP wireless port.
*  wifi config get bw <port>
* @parameter
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_get_bandwidth_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    uint8_t bw = 0;
    int32_t port = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get bw <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return 1;

    ret = wifi_config_get_bandwidth(port, &bw);
    if (ret < 0)
        status = 1;

    LOG_FUNC("wifi_config_get_bandwidth(port%d): %d, ret:%s, Code=%d\n",
             (int)port, bw, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return status;
}

/**
* @brief Example of Configure channel for STA/AP wireless port. STA will keep
*        idle and stay in channel specified
* wifi config set ch <port> <ch>
* @parameter
*     [IN] channel  1~14 are supported for 2.4G only product
* @return >=0 means success, <0 means fail
*/
uint8_t wifi_config_set_channel_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    int32_t port = 0;
    char ch = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set ch <port> <ch>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    ch = atoi(param[1]);

    if (port < 0)
        return -1;

    if (ch < 1) {
        LOG_FUNC("Invalid channel number\n");
        return -1;
    }

    ret = wifi_config_set_channel(port, ch);

    LOG_FUNC("wifi_config_set_channel(port%d): %d, ret:%s, Code=%d\n",
             (int)port,  ch, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}


/**
* @brief Example of get the current channel for STA/AP wireless port.
* wifi config get channel <port>
* @parameter
*     [OUT] channel I1~14 are supported for 2.4G only product
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_get_channel_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    int32_t port = 0;
    uint8_t ch = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get channel <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    ret = wifi_config_get_channel(port, &ch);

    LOG_FUNC("wifi_config_get_channel(port%d): %d, ret:%s, Code=%d\n",
             (int)port, ch, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

/**
* @brief Example of Configure channel for STA wireless port. STA will scan
*        the input channels
* wifi config set ch <port> <amount_of_channel> <ch A> <ch B>...
* @parameter
*     [IN] channel  1~14 are supported for 2.4G only product
* @return >=0 means success, <0 means fail
*/
uint8_t wifi_config_set_multi_channel_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    int32_t port = 0;
    int32_t ch_len = 0;
    uint8_t *ch = NULL;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set ch <port> <amount_of_channel> <ch A> <ch B>...\n");
        return -1;
    }

    port = port_sanity_check(param[0]);

    ch_len = atoi(param[1]);
    if (ch_len < 1 || ch_len > 15)
        return -1;

    if (port < 0)
        return -1;

    ch = (uint8_t *)kalMemAlloc(sizeof(uint8_t) * ch_len, VIR_MEM_TYPE);
    for (uint8_t i = 0; i < ch_len; i++) {
        if (atoi(param[i + 2]) >= 1) {
            ch[i] = atoi(param[i + 2]);
        } else {
            LOG_FUNC("Invalid channel number\n");
            return -1;
        }
    }

    ret = wifi_config_set_multi_channel(port, ch, ch_len);

    LOG_FUNC("wifi_config_set_multi_channel(port%d): len=%d, ret:%s, Code=%d\n",
             (int)port, ch_len, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    kalMemFree(ch, VIR_MEM_TYPE, sizeof(uint8_t)*ch_len);

    return ret;
}

/**
* @brief Get the channel bandwidth of connecting channel.
*
* @parameter
*     [OUT] channel bandwidth
* @return >=0 means success, <0 means fail
*/
uint8_t wifi_config_get_chbw_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    uint8_t bw = 0;
    int32_t port = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get chbw <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return 1;

    ret = wifi_config_get_chbw(port, &bw);
    if (ret < 0)
        status = 1;

    LOG_FUNC("wifi_config_get_bandwidth(port%d): %d, ret:%s, Code=%d\n",
             (int)port, bw, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return status;
}

/**
* @brief Example of Set WiFi Wireless Mode
* wifi config set wirelessmode <port> <mode>
* @param [IN]mode, @sa Refer to #wifi_phy_mode_t
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_set_wireless_mode_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t mode;
    int32_t port = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set wirelessmode <port> <mode>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;
    mode = atoi(param[1]);

    ret = wifi_config_set_wireless_mode(port, (wifi_phy_mode_t)mode);

    LOG_FUNC("wifi_config_set_wireless_mode(mode=%d), ret:%s, Code=%d\n",
             mode, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Get WiFi Wireless Mode
* wifi config get wirelessmode <port>
* @mode  @sa Refer to #wifi_phy_mode_t
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_get_wireless_mode_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t mode = 0;
    int32_t port = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get wirelessmode <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    ret = wifi_config_get_wireless_mode(port, (wifi_phy_mode_t *)&mode);

    LOG_FUNC("wifi_config_get_wireless_mode(mode=%d), ret:%s, Code=%d\n",
             mode, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Set Country Code
* wifi config set country_code <code>
*
* @return >=0 means success, <0 means fail
*/
uint8_t wifi_config_set_country_code_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    wifi_country_code_t *country_code = NULL;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set country_code <code> \n");
        return -1;
    }

    LOG_FUNC("Set country code: %s.\n", param[0]);

    if (kalStrLen(param[0]) > 4) {
        LOG_FUNC("Invalid country code length: %d.\n", strlen(param[0]));
        return -1;
    }

    country_code = (wifi_country_code_t *)kalMemAlloc
                   (sizeof(wifi_country_code_t), VIR_MEM_TYPE);
    if (country_code == NULL) {
        LOG_FUNC("malloc country_code fail.\n");
        return -1;
    }

    kalMemZero(country_code, sizeof(wifi_country_code_t));
    kalMemCopy(country_code, param[0], kalStrLen(param[0]));

    ret = wifi_config_set_country_code(country_code);

    kalMemFree(country_code, VIR_MEM_TYPE, sizeof(wifi_country_code_t));

    LOG_FUNC("wifi_config_set_country_code_ex, ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Get Country Code
* wifi config get country_code
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_get_country_code_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    wifi_country_code_t *country_code = NULL;

    country_code = (wifi_country_code_t *)kalMemAlloc
                   (sizeof(wifi_country_code_t), VIR_MEM_TYPE);
    if (country_code == NULL) {
        LOG_FUNC("malloc country_code fail.\n");
        return -1;
    }

    kalMemZero(country_code, sizeof(wifi_country_code_t));

    ret = wifi_config_get_country_code(country_code);

    kalMemFree(country_code, VIR_MEM_TYPE, sizeof(wifi_country_code_t));

    LOG_FUNC("wifi_config_get_country_code_ex(COUNTRY=%s), ret:%s, Code=%d\n",
             (char *)country_code->country_code, WIFI_CLI_RETURN_STRING(ret),
             (int)ret);
    return ret;
}

#if CFG_SUPPORT_REG_RULES
/**
* @brief Check the reg rules.
* @param [IN]channel and country_code
* wifi config get reg_rules
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_reg_rules_allow_channel_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t ch;
    uint8_t country[MAX_COUNTRY_CODE_LEN + 1];

    if (len < 2) {
        LOG_FUNC("Usage: wifi config get reg_rules <ch> <country>\n");
        return -1;
    }
    if (kalStrLen(param[1]) > MAX_COUNTRY_CODE_LEN) {
        LOG_FUNC("Invalid country code length.\n");
        return -1;
    }

    ch = atoi(param[0]);
    kalMemZero(country, MAX_COUNTRY_CODE_LEN);
    kalMemCopy(country, param[1], MAX_COUNTRY_CODE_LEN);
    LOG_FUNC("Search CH %d in country code: %s.\n", ch, country);

    ret = wifi_config_reg_rules_allow_channel(ch, country);

    LOG_FUNC("wifi_config_reg_rules_allow_channel_ex, ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}
#endif /* #if CFG_SUPPORT_REG_RULES */

/**
* @brief Example of Set WiFi SSID.
* wifi config set ssid <port> <ssid>
* @param [IN]port
*       0 STA
*       1 AP
* @param [IN]ssid SSID
* @param [IN]ssid_len Length of SSID
*
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_config_set_ssid_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    int32_t port = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set ssid <port> <ssid>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

#ifdef MTK_MINISUPP_ENABLE
    ret = wifi_config_set_ssid(port, (uint8_t *)param[1],
                               os_strlen(param[1]));
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    LOG_FUNC("wifi_config_set_ssid(port:%d), [%s], ret:%s, Code=%d\n",
             (int)port, param[1], WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Get WiFi SSID.
* wifi config get ssid <port>
* @param [IN]port
*       0 STA
*       1 AP
* @param [OUT]ssid SSID
* @param [OUT]ssid_len Length of SSID
*
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_config_get_ssid_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t length = 0;
    uint8_t ssid[32] = {0};
    int32_t port = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get ssid <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    ret = wifi_config_get_ssid(port, ssid, &length);

    LOG_FUNC("wifi_config_get_ssid(port:%d), [%s], ret:%s, Code=%d\n",
             (int)port, ssid, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Set WiFi BSSID.
* wifi config set bssid <bssid>
* @param [IN]bssid BSSID
*
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_config_set_bssid_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t bssid[6] = {0};

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set bssid <bssid>\n");
        return -1;
    }

    wifi_conf_get_mac_from_str((char *)bssid, (char *)param[0]);
    ret = wifi_config_set_bssid((uint8_t *)bssid);

    LOG_FUNC("wifi_config_set_bssid(), [%s], ret:%s, Code=%d\n", param[0],
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Get WiFi BSSID.
* wifi config get bssid
* @param [OUT]bssid BSSID
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_get_bssid_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t bssid[6] = {0};

    ret = wifi_config_get_bssid((uint8_t *)bssid);

    LOG_FUNC("wifi_config_get_bssid(), %02x:%02x:%02x:%02x:%02x:%02x, ret:%s, Code=%d\n",
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}


/**
* @brief Example of get the authentication mode for the specified STA/AP port
* wifi config get sec <port>
* @param [OUT]authmode method index:
*           1 WPAPSK
*           2 WPA2PSK
*           3 WPA1PSKWPA2PSK
* @param [OUT] encryption method index:
*              1 AES
*              2 TKIP
*              3 TKIPAES
*
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_config_get_security_mode_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t auth = 0;
    uint8_t encrypt = 0;
    int32_t port = 0;

    if (len < 1) {
        LOG_USAGE("Usage:wifi config get sec <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    ret = wifi_config_get_security_mode(port, (wifi_auth_mode_t *)&auth,
                                        (wifi_encrypt_type_t *)&encrypt);

    LOG_FUNC("wifi_config_get_security_mode_ex: port:%d, auth mode:%d, encrypt type:%d, ret:%s, Code=%d\n",
             (int)port, auth, encrypt, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

/**
* @brief Example of Set the authentication mode and encryption mode
   for the specified STA/AP port
* wifi config set sec <port> <auth> <encrypt>
* @param [IN]port
* @param [IN]authmode
*           1 WPAPSK
*           2 WPA2PSK
*           3 WPA1PSKWPA2PSK
* @param  [IN] encryption method index:
*              1 AES
*              2 TKIP
*              3 TKIPAES
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_security_mode_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t auth = 0;
    uint8_t encrypt = 0;
    int32_t port = 0;

    if (len < 3) {
        LOG_USAGE("Usage: wifi config set sec <port> <auth> <encrypt>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;
    auth = atoi(param[1]);
    encrypt = atoi(param[2]);
    ret = wifi_config_set_security_mode(port, (wifi_auth_mode_t)auth,
                                        (wifi_encrypt_type_t)encrypt);

    LOG_FUNC(
        "wifi_config_set_security_mode_ex: port:%d, auth mode:%d, encrypt type:%d, ret:%s, Code=%d\n",
        (int)port, auth, encrypt, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}


/**
* @brief Example of Set (multiple) authentication modes and the encryption
*  mode for the specified STA/AP port
* wifi config set sec <port> <auth> <encrypt>
* @param [IN]port
* @param [IN]authmode
*          bit1 WIFI_AUTH_MODE_OPEN
*          bit2 WIFI_AUTH_MODE_SHARED
*          bit3 WIFI_AUTH_MODE_AUTO_WEP
* @param  [IN] encryption method index:
*          1 AES
*          2 TKIP
*          3 TKIPAES
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_multi_security_mode_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint32_t auth = 0;
    uint8_t encrypt = 0;
    int32_t port = 0;

    if (len < 3) {
        LOG_USAGE("Usage: wifi config set msec <port> <auth> <encrypt>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;
    auth = atoi(param[1]);
    encrypt = atoi(param[2]);
#ifdef MTK_MINISUPP_ENABLE
    ret = wifi_config_set_multi_security_mode(port, auth,
                                              (wifi_encrypt_type_t)encrypt);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    LOG_FUNC(
        "wifi_config_set_multi_security_mode_ex: port:%d, auth mode:%d, encrypt type:%d, ret:%s, Code=%d\n",
        (int)port, auth, encrypt, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}


/**
* @brief Example of Set ieee80211w
* wifi config set ieee80211w <port> <ieee80211w>
* @param [IN]port
* @param [IN]
*          0 disabled
*          1 enabled
*          2 required
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_ieee80211w_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t ieee80211w = 0;
    int32_t port = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set msec <port> <ieee80211w>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;
    ieee80211w = atoi(param[1]);
#ifdef MTK_MINISUPP_ENABLE
    ret = wifi_config_set_ieee80211w(port, ieee80211w);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    LOG_FUNC(
        "wifi_config_set_multi_security_mode_ex: port:%d, ieee80211w:%d, ret:%s, Code=%d\n",
        (int)port, ieee80211w, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}


/**
* @brief Example of Set proto
* wifi config set ieee80211w <port> <proto>
* @param [IN]port
* @param [IN]
*          bit0 WPA_PROTO_WPA
*          bit1 WPA_PROTO_RSN
*          bit2 WPA_PROTO_WAPI
*          bit3 WPA_PROTO_OSEN
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_proto_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t proto = 0;
    int32_t port = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set msec <port> <proto>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;
    proto = atoi(param[1]);
#ifdef MTK_MINISUPP_ENABLE
    ret = wifi_config_set_proto(port, proto);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    LOG_FUNC(
        "wifi_config_set_multi_security_mode_ex: port:%d, proto:%d, ret:%s, Code=%d\n",
        (int)port, proto, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}


/**
* @brief Example of set the psk for the specified STA/AP port
* wifi config set psk <port> <password>
@param [IN]passphrase 8 ~ 63 bytes ASCII or 64 bytes Hex
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_psk_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    int32_t port = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set psk <port> <password>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

#ifdef MTK_MINISUPP_ENABLE
    ret = wifi_config_set_wpa_psk_key(port, (uint8_t *)param[1],
                                      os_strlen(param[1]));
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    LOG_FUNC("wifi_config_set_psk_ex: port:%d, psk(%s), ret:%s, Code=%d\n",
             (int)port, param[1], WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}


/**
* @brief Example of get the psk for the specified STA/AP port
* wifi config get psk <port> <password>
@param [IN]passphrase 8 ~ 63 bytes ASCII or 64 bytes Hex
*
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_config_get_psk_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t password[64] = {0};
    uint8_t length = 0;
    int32_t port = 0;

    if (len < 1) {
        LOG_USAGE("Usage:wifi config get psk <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    ret = wifi_config_get_wpa_psk_key(port, password, &length);

    LOG_FUNC("wifi_config_get_psk_ex: port:%d, psk(%s), ret:%s, Code=%d\n",
             (int)port, password, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

/**
* @brief Example of Set PMK for the specified STA/AP port
* wifi config set pmk <port> <PMK>
* @param [IN]port
*       0 STA / AP Client
*       1 AP
* @param  [IN] PMK (in hex)
*       00, 05, 30, ......(size 32)
* @return =0 means success, >0 means fail
* @note Default to OPEN
*/
uint8_t wifi_config_set_pmk_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t hex[32];
    int32_t port = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set pmk <port> <PMK>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

#ifdef MTK_MINISUPP_ENABLE
    kalMemSet(hex, 0, sizeof(hex));
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    convert_string_to_hex_array(param[1], hex);
    ret = wifi_config_set_pmk(port, hex);

    LOG_FUNC("wifi_config_set_pmk_ex: port:%d, ret:%s, Code=%d\n",
             (int)port, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}


/**
* @brief Example of Get PMK for the specified STA/AP port
* wifi config get pmk <port>
* @param [IN]port
*       0 STA / AP Client
*       1 AP
* @return  >=0 means success, <0 means fail
* @note Default to OPEN
*/
uint8_t wifi_config_get_pmk_ex(uint8_t len, char *param[])
{
    uint8_t i;
    int32_t ret = 0;
    uint8_t hex[32];
    int32_t port = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get pmk <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

#ifdef MTK_MINISUPP_ENABLE
    kalMemSet(hex, 0, sizeof(hex));
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    ret = wifi_config_get_pmk(port, hex);

    LOG_FUNC("wifi_config_get_pmk_ex: port:%d, ret:%s, Code=%d, key dump:\n",
             (int)port, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    for (i = 0; i < sizeof(hex); i++) {
        if (i % 16 == 0)
            LOG_FUNC("\n\t");
        LOG_FUNC("%02x ", (unsigned int)hex[i]);
    }
    LOG_FUNC("\n");

    return ret;
}


/**
* @brief Example of Set WiFi WEP Keys
* wifi config set wep <port> <key_id> <key_string>
* @param [IN]wifi_wep_key_t
*
* @return >=0 means success, <0 means fail
*/
uint8_t wifi_config_set_wep_key_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    wifi_wep_key_t wep_key = {{{0}, {0} }, {0}, 0};
    int32_t port = 0;
    char *section = NULL;

    if (len < 3) {
        LOG_USAGE("Usage: wifi config set wep <port> <key_id> <key_string>\n");
        return 1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;
    section = (port == WIFI_PORT_STA ? WIFI_PROFILE_BUFFER_STA :
               WIFI_PROFILE_BUFFER_AP);
#ifdef MTK_MINISUPP_ENABLE
    wep_key.wep_tx_key_index = atoi(param[1]);
    if (os_strlen(param[2]) == 10 || os_strlen(param[2]) == 26) {
        wep_key.wep_key_length[wep_key.wep_tx_key_index] =
            os_strlen(param[2]) / 2;
        AtoH((char *)param[2],
             (char *)&wep_key.wep_key[wep_key.wep_tx_key_index],
             (int)wep_key.wep_key_length[wep_key.wep_tx_key_index]);
    } else if (os_strlen(param[2]) == 5 || os_strlen(param[2]) == 13) {
        os_memcpy(wep_key.wep_key[wep_key.wep_tx_key_index],
                  param[2], os_strlen(param[2]));
        wep_key.wep_key_length[wep_key.wep_tx_key_index] =
            os_strlen(param[2]);
    } else {
        LOG_FUNC("invalid length of value.\n");
        return -1;
    }
    for (int i = 0; i < WIFI_NUMBER_WEP_KEYS; ++i)
        wep_key.wep_key[i][WIFI_MAX_WEP_KEY_LENGTH] = '\0';

    ret = wifi_config_set_wep_key((uint8_t)port, &wep_key);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    LOG_FUNC("[%s] save wep key =(%s, %s, %s, %s) key id=%d, len=(%d, %d, %d, %d) done, ret:%s, Code=%d\n",
             section,
             wep_key.wep_key[0],
             wep_key.wep_key[1],
             wep_key.wep_key[2],
             wep_key.wep_key[3],
             wep_key.wep_tx_key_index,
             wep_key.wep_key_length[0],
             wep_key.wep_key_length[1],
             wep_key.wep_key_length[2],
             wep_key.wep_key_length[3],
             WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

/**
* @brief Example of get WiFi WEP Keys
* wifi config get wep <port>
* @param [OUT]wifi_wep_key_t
*
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_config_get_wep_key_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    int32_t port = 0;
#ifdef MTK_MINISUPP_ENABLE
    wifi_wep_key_t *keys = NULL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get wep <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

#ifdef MTK_MINISUPP_ENABLE
    keys = (wifi_wep_key_t *)os_zalloc(sizeof(wifi_wep_key_t));
    if (keys)
        kalMemSet(keys, 0, sizeof(wifi_wep_key_t));
    else {
        LOG_FUNC("%s: keys alloc fail.\n", __func__);
        return -1;
    }
    ret = wifi_config_get_wep_key(port, keys);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    LOG_FUNC("wifi_config_get_wep_ex: port:%d, ret:%s, Code=%d\n",
             (int)port, WIFI_CLI_RETURN_STRING(ret), (int)ret);

#ifdef MTK_MINISUPP_ENABLE
    if (keys->wep_tx_key_index < 4) {
        int i;
        int offset = 0;
        int res;
        int key_buflen = 2 * WIFI_MAX_WEP_KEY_LENGTH;
        char wep_key[2 * WIFI_MAX_WEP_KEY_LENGTH + 1];

        kalMemSet(wep_key, 0, key_buflen + 1);
        for (i = 0; i < keys->wep_key_length[keys->wep_tx_key_index]; i++) {
            res = kalSnprintf(wep_key + offset, key_buflen - offset, "%02x",
                              keys->wep_key[keys->wep_tx_key_index][i]);
            if (res < 0) {
                LOG_FUNC("failed to parse wep key");
                ret = -1;
                break;
            }
            offset += res;
        }
        LOG_FUNC("keys[%d]=%s\n", (int)keys->wep_tx_key_index, wep_key);
    } else {
        LOG_FUNC("Invalid key id:[%d]\n", (int)keys->wep_tx_key_index);
        ret = -1;
    }

    os_free(keys);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    return ret;
}

/**
* @brief Example of Get WiFi DTIM Interval
* wifi config get dtim
* @interval: 1~255
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_get_dtim_interval_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t dtim = 0;

    ret = wifi_config_get_dtim_interval(&dtim);

    LOG_FUNC("wifi_config_get_dtim_interval dtim:%d, ret:%s, Code=%d\n",
             dtim, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Set WiFi DTIM Interval
* wifi config set dtim <dtim interval>
* @interval: 1~255
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_set_dtim_interval_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t dtim = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set dtim <dtim interval>\n");
        return -1;
    }

    dtim = atoi(param[0]);
    ret = wifi_config_set_dtim_interval(dtim);

    LOG_FUNC("wifi_config_set_dtim_interval dtim:%d, ret:%s, Code=%d\n",
             dtim, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Set WiFi DTIM Interval
* wifi config set bcn_int <beacon interval>
* @interval: 1~255
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_set_bcn_interval_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t beacon = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set bcn_int <beacon interval>\n");
        return -1;
    }

    beacon = atoi(param[0]);
    ret = wifi_config_set_bcn_interval(beacon);

    LOG_FUNC("wifi_config_set_bcn_interval bcn_int:%d, ret:%s, Code=%d\n",
             beacon, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Set WiFi Listen Interval
* wifi config set listen <listen interval>
* @param [IN]interval 1 ~ 255
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_set_listen_interval_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t listen = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set listen <listen interval>\n");
        return -1;
    }

    listen = atoi(param[0]);
    ret = wifi_config_set_listen_interval(listen);

    LOG_FUNC("wifi_config_set_listen_interval listen:%d, ret:%s, Code=%d\n",
             listen, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Get WiFi Listen Interval
* wifi config get listen
* @param [OUT]interval 1 ~ 255
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_get_listen_interval_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t listen = 0;

    ret = wifi_config_get_listen_interval(&listen);

    LOG_FUNC("wifi_config_get_listen_interval listen:%d, ret:%s, Code=%d\n",
             listen, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
 * @brief Set Bss Preference when scanning
 *
 * @return >=0 means success, <0 means fail
 *
 * @params [Mode]off=0, prefer_2G=1, prefer_5G=2
 */
uint8_t wifi_config_set_bss_preference_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t mode = atoi(param[0]);

    if (len != 1) {
        LOG_USAGE("Usage: wifi config set bss_pref <mode>\n");
        return -1;
    }

    ret = wifi_config_set_bss_preference(mode);

    LOG_FUNC("wifi_config_set_bss_preference, mode:%d, ret:%s, Code=%d\n",
             mode, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of WiFi Radio ON/OFF
* wifi config set radio <onoff>
* @param [IN]onoff
*       0 OFF
*       1 ON
* @return  =0 means success, >0 means fail
* @note in MODE_Dual, both WiFi interface radio will be turn on/off at the
*       same time
*/
uint8_t wifi_config_set_radio_on_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t flag = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set radio <onoff>\n");
        return -1;
    }

    flag = atoi(param[0]);
    ret = wifi_config_set_radio(flag);

    LOG_FUNC("wifi_config_set_radio(onoff=%d), ret:%s, Code=%d\n", flag,
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}


/**
* @brief Example of get WiFi Radio ON/OFF
* wifi config get radio
* @param [OUT]onoff
*       0 OFF
*       1 ON
* @return  >=0 means success, <0 means fail
* @note in MODE_Dual, both WiFi interface radio will be turn on/off at the
*       same time
*/
uint8_t wifi_config_get_radio_on_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t flag = 0;

    ret = wifi_config_get_radio(&flag);

    LOG_FUNC("wifi_config_get_radio(onoff=%d), ret:%s, Code=%d\n", (int)flag,
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_get_connected_ap_info_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    connected_ap_info_s connected_ap_info;

    ret = wifi_config_get_connected_ap_info(&connected_ap_info);

    LOG_FUNC("wifi_config_get_connected_ap_info_ex (beacon_interval: %d, dtim_period:%d), ret:%s, Code=%d\n",
             connected_ap_info.beacon_interval, connected_ap_info.dtim_period,
             WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

uint8_t wifi_config_set_ps_mode_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t ps_mode = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set ps_mode <ps_mode>\n");
        return -1;
    }

    ps_mode = atoi(param[0]);
    ret = wifi_config_set_power_save_mode(ps_mode);

    LOG_FUNC("wifi_config_set_ps_mode_ex ps_power:%d, ret:%s, Code=%d\n",
             ps_mode, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

uint8_t wifi_config_get_ps_mode_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t ps_mode = 0;

    ret = wifi_config_get_power_save_mode(&ps_mode);

    LOG_FUNC("wifi_config_get_ps_mode_ex mode:%d, ret:%s, Code=%d\n",
             ps_mode, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

char *wifi_get_phy_mode_name(uint8_t phy_mode, char *phy_name)
{
    if (phy_name == NULL) {
        LOG_FUNC("phy name is null\n");
        return NULL;
    }
    switch (phy_mode) {
        case 0:
            strcpy(phy_name, "Legacy CCK");
            break;
        case 1:
            strcpy(phy_name, "Legacy OFDM");
            break;
        case 2:
            strcpy(phy_name, "HT Mixed mode");
            break;
        case 3:
            strcpy(phy_name, "HT Green field mode");
            break;
        case 4:
            strcpy(phy_name, "VHT mode");
            break;
        default:
            strcpy(phy_name, "unknown");
            break;
    }
    return phy_name;
}

/**
* @brief Example of Get Wlan statistic
* wifi config get wlanstat
*
* @return >=0 means success, <0 means fail
*/
uint8_t wifi_config_get_statistic_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t op_mode;
    uint32_t tx_fail_count;
    uint32_t rx_with_CRC;
    uint8_t tx_mode, rx_mode;
    uint8_t stbc, nsts, rate;
    wifi_statistic_t statistic = {0};
    char phy_name[32];

    ret = wifi_config_get_opmode(&op_mode);
    if (op_mode != WIFI_MODE_STA_ONLY) {
        LOG_FUNC("Get wlan statistics only support in STA mode");
        return -1;
    }

    ret = wifi_config_get_statistic(&statistic);

    tx_fail_count = statistic.Tx_Fail_Count;
    rx_with_CRC = statistic.Rx_with_CRC;
    tx_mode = statistic.REAL_TX_PHY_Mode;
    rx_mode = statistic.REAL_RX_PHY_Mode;

    LOG_FUNC("Get wlan statistics:");
    LOG_FUNC("CurrentTemperature = %u",
             (unsigned int)statistic.Current_Temperature);
    LOG_FUNC("Tx success count = %u",
             (unsigned int)statistic.Tx_Success_Count);
    LOG_FUNC("Tx retry count = %u",
             (unsigned int)statistic.Tx_Retry_Count);
    LOG_FUNC("Tx fail count = %u, PER = %u.%u %%",
             (unsigned int)tx_fail_count,
             (unsigned int)WIFI_CLI_CALC_PER_BEFORE_DOT(tx_fail_count,
                                                        tx_fail_count + statistic.Tx_Success_Count),
             (unsigned int)WIFI_CLI_CALC_PER_AFTER_DOT(tx_fail_count,
                                                       tx_fail_count + statistic.Tx_Success_Count));
    LOG_FUNC("Rx_with CRC = %u, PER = %u.%u %%",
             (unsigned int)rx_with_CRC,
             (unsigned int)WIFI_CLI_CALC_PER_BEFORE_DOT(rx_with_CRC,
                                                        rx_with_CRC + statistic.Rx_Success_Count),
             (unsigned int)WIFI_CLI_CALC_PER_AFTER_DOT(rx_with_CRC,
                                                       rx_with_CRC + statistic.Rx_Success_Count));
    LOG_FUNC("Rx drop due to out of resource = %u",
             (unsigned int)statistic.Rx_Drop_due_to_out_of_resources);
    LOG_FUNC("MIC error count = %d",
             (unsigned int)statistic.MIC_Error_Count);
    LOG_FUNC("Rssi = %d",
             (unsigned int)statistic.Rssi);
    LOG_FUNC("Tx AGG range_1 (1) = %u",
             (unsigned int)statistic.Tx_AGG_Range_1);
    LOG_FUNC("Tx AGG range_2 (2~9) = %u",
             (unsigned int)statistic.Tx_AGG_Range_2);
    LOG_FUNC("Tx AGG range_3 (10~18) = %u",
             (unsigned int)statistic.Tx_AGG_Range_3);
    LOG_FUNC("Tx AGG range_4 (19~27) = %u",
             (unsigned int)statistic.Tx_AGG_Range_4);
    LOG_FUNC("AMPDU Tx success = %u",
             (unsigned int)statistic.AMPDU_Tx_Success);
    LOG_FUNC("AMPDU Tx fail count = %u, PER = %u.%u %%",
             (unsigned int)statistic.AMPDU_Tx_Fail,
             (unsigned int)WIFI_CLI_CALC_PER_BEFORE_DOT(
                 statistic.AMPDU_Tx_Fail,
                 statistic.AMPDU_Tx_Fail + statistic.AMPDU_Tx_Success),
             (unsigned int)WIFI_CLI_CALC_PER_AFTER_DOT(
                 statistic.AMPDU_Tx_Fail,
                 statistic.AMPDU_Tx_Fail + statistic.AMPDU_Tx_Success));
    LOG_FUNC("SNR = %d", (int)statistic.SNR);
    LOG_FUNC("Current bandwidth = %s",
             (statistic.BBPCurrentBW) ? "BW 40" : "BW 20");
    if (tx_mode == TX_RATE_MODE_CCK || tx_mode == TX_RATE_MODE_OFDM)
        LOG_FUNC("Real tx phy rate = %dM", statistic.REAL_TX_PHY_Rate);
    else if (tx_mode == TX_RATE_MODE_HTMIX || tx_mode == TX_RATE_MODE_HTGF)
        LOG_FUNC("Real tx phy rate = MCS%d", statistic.REAL_TX_PHY_Rate);
    else {
        stbc = statistic.REAL_TX_PHY_Rate >> 9;
        nsts = (statistic.REAL_TX_PHY_Rate >> 7) & BITS(0, 1);
        rate = statistic.REAL_TX_PHY_Rate & BITS(0, 6);
        LOG_FUNC("Real tx phy rate = %s%d_MCS%d", stbc ? "NSTS" : "NSS",
                 nsts, rate);
    }
    LOG_FUNC("Real tx phy mode = %s",
             wifi_get_phy_mode_name(tx_mode, phy_name));
    LOG_FUNC("Real tx short GI = %s",
             (statistic.REAL_TX_ShortGI) ?
             "Short GI (400)" : "Normal GI (800)");
    LOG_FUNC("Real tx MCS = %u",
             (unsigned int)statistic.REAL_TX_MCS);
    if (rx_mode == TX_RATE_MODE_CCK || rx_mode == TX_RATE_MODE_OFDM)
        LOG_FUNC("Real rx phy rate = %dM", statistic.REAL_RX_PHY_Rate);
    else if (rx_mode == TX_RATE_MODE_HTMIX || rx_mode == TX_RATE_MODE_HTGF)
        LOG_FUNC("Real rx phy rate = MCS%d", statistic.REAL_RX_PHY_Rate);
    else {
        stbc = statistic.REAL_RX_PHY_Rate >> 10;
        nsts = (statistic.REAL_RX_PHY_Rate >> 7) & BITS(0, 2);
        rate = statistic.REAL_RX_PHY_Rate & BITS(0, 6);
        LOG_FUNC("Real rx phy rate = %s%d_MCS%d", stbc ? "NSTS" : "NSS",
                 nsts, rate);
    }
    LOG_FUNC("Real rx phy mode = %s",
             wifi_get_phy_mode_name(statistic.REAL_RX_PHY_Mode, phy_name));
    LOG_FUNC("Real rx short GI = %s",
             (statistic.REAL_RX_ShortGI) ?
             "Short GI (400)" : "Normal GI (800)");
    LOG_FUNC("Real rx MCS = %u",
             (unsigned int)statistic.REAL_RX_MCS);

    LOG_FUNC("wifi_config_get_statistic_ex, ret:%s, Code=%d",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret == 0 ? 0 : 1;
}

uint8_t wifi_config_set_single_sku_table_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
#ifdef MTK_MINISUPP_ENABLE
    singleSKU2G_t *sku_table_2g = kalMemAlloc(sizeof(singleSKU2G_t),
                                              VIR_MEM_TYPE);

    if (sku_table_2g == NULL)
        return -1;

    ret = wifi_config_set_single_sku_2G(sku_table_2g);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    LOG_FUNC("wifi_config_set_single_sku_table_ex, ret:%s, Code=%d\n", WIFI_CLI_RETURN_STRING(ret), (int)ret);
#ifdef MTK_MINISUPP_ENABLE
    kalMemFree(sku_table_2g, VIR_MEM_TYPE, sizeof(sku_table_2g));
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return ret;
}


/**
* @brief Example of Get the current STA port link up / link down status of
* the connection wifi connect get linkstatus
* @link status:
* WIFI_STATUS_LINK_DISCONNECTED(0)
* WIFI_STATUS_LINK_CONNECTED(1)
* @parameter None
* @return >=0 means success, <0 means fail
* @note WIFI_STATUS_LINK_DISCONNECTED indicates STA may in IDLE/ SCAN/ CONNECTING state
*/
uint8_t wifi_connect_get_link_status_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t link = 0;

    ret = wifi_connection_get_link_status(&link);

    LOG_FUNC("wifi_connect_get_link_status(link=%d), ret:%s, Code=%d\n", link,
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Get rssi of the connected AP
* wifi connect get rssi
* @parameter None
* @return =0 means success, >0 means fail
*/
uint8_t wifi_connect_get_rssi_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    int8_t rssi = 0;

    ret = wifi_connection_get_rssi(&rssi);

    LOG_FUNC("wifi_connect_get_rssi_ex(rssi=%d), ret:%s, Code=%d\n", rssi,
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Get WiFi Associated Station List
* wifi connect get stalist
* @param [OUT]station_list
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_connect_get_station_list_ex(uint8_t len, char *param[])
{
    int i;
    int32_t ret = 0;
    wifi_sta_list_t list[WIFI_MAX_NUMBER_OF_STA];
    uint8_t size = 0;
    uint8_t ucTxmode, ucTxrate = 0;
    uint8_t ucRxmode, ucRxrate = 0;

    ret = wifi_connection_get_sta_list(&size, list);

    LOG_FUNC("stalist size=%d\n", size);

    for (i = 0; i < size; i++) {
        LOG_FUNC("%d\n", i);

        ucTxmode = list[i].last_tx_rate.field.mode;
        ucTxrate = list[i].last_tx_rate.field.mcs;

        if (ucTxmode == TX_RATE_MODE_CCK) {
            LOG_FUNC("  TX: mode=%d(%s), rate=%d(%s)\n",
                     ucTxmode, RATE_V2_HW_TX_MODE_STR[ucTxmode],
                     ucTxrate, ucTxrate < 4 ?
                     HW_TX_RATE_CCK_STR[ucTxrate] : "N/A");
        } else if (ucTxmode == TX_RATE_MODE_OFDM) {
            LOG_FUNC("  TX: mode=%d(%s), rate=%d(%s)\n",
                     ucTxmode, RATE_V2_HW_TX_MODE_STR[ucTxmode],
                     ucTxrate, nicHwRateOfdmStr(ucTxrate));
        } else {
            LOG_FUNC("  TX: mode=%d(%s), rate=%d(MCS%d)\n",
                     ucTxmode, ucTxmode < ENUM_TX_MODE_NUM ?
                     RATE_V2_HW_TX_MODE_STR[ucTxmode] : "N/A",
                     ucTxrate, ucTxrate);
        }

        ucRxmode = list[i].last_rx_rate.field.mode;
        ucRxrate = list[i].last_rx_rate.field.mcs;

        if (ucRxmode == TX_RATE_MODE_CCK) {
            LOG_FUNC("  RX: mode=%d(%s), rate=%d(%s)\n",
                     ucRxmode, RATE_V2_HW_TX_MODE_STR[ucRxmode],
                     ucRxrate, ucRxrate < 4 ?
                     HW_TX_RATE_CCK_STR[ucRxrate] : "N/A");
        } else if (ucRxmode == TX_RATE_MODE_OFDM) {
            LOG_FUNC("  RX: mode=%d(%s), rate=%d(%s)\n",
                     ucRxmode, RATE_V2_HW_TX_MODE_STR[ucRxmode],
                     ucRxrate, nicHwRateOfdmStr(ucRxrate));
        } else {
            LOG_FUNC("  RX: mode=%d(%s), rate=%d(MCS%d)\n",
                     ucRxmode, ucRxmode < ENUM_TX_MODE_NUM ?
                     RATE_V2_HW_TX_MODE_STR[ucRxmode] : "N/A",
                     ucRxrate, ucRxrate);
        }

        LOG_FUNC("  rssi_sample.last_rssi=%d\n",
                 list[i].rssi_sample.last_rssi);
        LOG_FUNC("  mac_address=%02x:%02x:%02x:%02x:%02x:%02x\n",
                 list[i].mac_address[0], list[i].mac_address[1],
                 list[i].mac_address[2], list[i].mac_address[3],
                 list[i].mac_address[4], list[i].mac_address[5]);
        LOG_FUNC("  power_save_mode=%d\n", list[i].power_save_mode);
        LOG_FUNC("  bandwidth=%d(%s)\n",
                 list[i].bandwidth, HW_TX_RATE_BW[list[i].bandwidth]);
        LOG_FUNC("  keep_alive=%d\n", list[i].keep_alive);
    }

    LOG_FUNC("wifi_connection_get_sta_list, ret:%s, Code=%ld\n",
             WIFI_CLI_RETURN_STRING(ret), ret);
    return ret;
}

uint8_t wifi_connect_disconnect_ap_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;

    ret = wifi_connection_disconnect_ap();

    LOG_FUNC("wifi_connect_disconnect_ap(), ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of deauth some WiFi connection
* wifi connect set deauth <MAC>
* @param [IN]addr STA MAC Address
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_connect_deauth_station_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    unsigned char addr[WIFI_MAC_ADDRESS_LENGTH] = {0};

    if (len < 1) {
        LOG_USAGE("Usage: wifi connect set deauth <addr>\n");
        return -1;
    }

    wifi_conf_get_mac_from_str((char *)addr, param[0]);
    ret = wifi_connection_disconnect_sta(addr);

    LOG_FUNC("deauth (%02x:%02x:%02x:%02x:%02x:%02x), ret:%s, Code=%d\n",
             addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_connect_send_raw_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint32_t pkt_len = 0;
    uint8_t *raw_pkt = NULL;
    uint8_t pkt_type = 0;
    uint8_t raw_data_pkt[] = {
        0x08, 0x02, 0x00, 0x00, 0x00, 0x11, 0x22, 0xaa, 0xbb, 0xcc,
        0x00, 0x11, 0x22, 0xaa, 0xbb, 0xcc, 0x00, 0x11, 0x22, 0xaa,
        0xbb, 0xcc, 0x30, 0x00, 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x45, 0x00, 0x00, 0x4e, 0x3d, 0x42, 0x00, 0x00,
        0x80, 0x11, 0x17, 0xba, 0xc0, 0xa5, 0x64, 0x58, 0xc0, 0xa5,
        0xff, 0xff, 0x00, 0x89, 0x00, 0x89, 0x00, 0x3a, 0x45, 0xf2,
        0x91, 0x64, 0x01, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x20, 0x46, 0x48, 0x46, 0x41, 0x45, 0x42, 0x45,
        0x45, 0x43, 0x41, 0x43, 0x41, 0x43, 0x41, 0x43, 0x41, 0x43,
        0x41, 0x43, 0x41, 0x43, 0x41, 0x43, 0x41, 0x43, 0x41, 0x43,
        0x41, 0x43, 0x41, 0x41, 0x41, 0x00, 0x00, 0x20, 0x00, 0x01,
        0xcc, 0x50, 0xd8, 0x53
    };
    uint8_t raw_action_pkt[] = {
        0xd0, 0x00, 0x3c, 0x00, 0x00, 0x11, 0x22, 0xaa,
        0xbb, 0xcc, 0x00, 0x11, 0x22, 0xaa, 0xbb, 0xcc,
        0x00, 0x11, 0x22, 0xaa, 0xbb, 0xcc, 0x90, 0x02,
        0x0a, 0x08, 0x01, 0x00, 0x00, 0xe8, 0x1d, 0xa8,
        0x35, 0x65, 0x1c, 0x34, 0x10, 0xe8, 0x1d, 0xa8,
        0x35, 0x65, 0x1c, 0x92, 0x04, 0x00, 0x00, 0x80,
        0x30, 0x09, 0x03, 0x01, 0xff
    };


    if (len < 1) {
        LOG_USAGE("Usage: wifi connect set send_raw <pkt_type>\n");
        return -1;
    }

    pkt_type = atoi(param[0]);

    switch (pkt_type) {
        case 0:
            raw_pkt = raw_data_pkt;
            pkt_len = sizeof(raw_data_pkt);
            break;
        case 1:
            raw_pkt = raw_action_pkt;
            pkt_len = sizeof(raw_action_pkt);
        default:
            break;
    }

    LOG_FUNC("wifi_connect_send_raw_ex(), type %d, len %d\n",
             (int)pkt_type, (int)pkt_len);
    ret = wifi_connection_send_raw_packet(raw_pkt, pkt_len);

    LOG_FUNC("wifi_connect_send_raw_ex(), ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}
/**
* @brief start scan with options
* wifi connect set scan <start> <scan mode> <scan_option> [ssid] [bssid]
* wifi connect set scan <stop>
* @parameter [IN] start/stop  0: stop, 1: start
* @parameter [IN] scan mode  0: full, 1: partial
* @parameter [IN] scan_option  0: active, 1: passive in all channel, 2:force active(not supported)
* @parameter [IN] ssid   "NULL" means not specified
* @parameter [IN] bssid   "NULL" means not specified
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_connect_set_scan_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
#ifdef MTK_MINISUPP_ENABLE
    uint8_t start = 0;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    if (len < 1) {
        LOG_USAGE("Usage: wifi connect set scan <start> <scan mode> <scan_option> [ssid] [bssid]\n"
                  "wifi connect set scan <stop>\n");
        return -1;
    }

#ifdef MTK_MINISUPP_ENABLE
    start = atoi(param[0]);
    if (!start)
        ret = wifi_connection_stop_scan();
    else {
        if (len < 3) {
            LOG_USAGE("Usage: wifi connect set scan <start> <scan mode> <scan_option> [ssid] [bssid]\n");
            return -1;
        }
        uint8_t scan_mode = atoi(param[1]);
        uint8_t scan_option = atoi(param[2]);

        char *ssid;
        uint8_t ssid_len = 0;
        uint8_t bssid_val[WIFI_MAX_NUMBER_OF_STA] = {0x0, 0x0, 0x0,
                                                     0x0, 0x0, 0x0
                                                    };
        uint8_t *bssid = (uint8_t *)bssid_val;

        if (len >= 4) {
            ssid = param[3];
            ssid_len = os_strlen(ssid);
            if (len == 5)
                wifi_conf_get_mac_from_str((char *)bssid_val, param[4]);
            else
                bssid = (uint8_t *)(NULL);
        } else {
            ssid = NULL;
            ssid_len = 0;
            bssid = (uint8_t *)(NULL);
        }
        /*reset scan list*/
        wifi_connection_scan_init(g_ap_list, 8);
        LOG_FUNC("size = %d\n", sizeof(wifi_scan_list_item_t));
        ret = wifi_connection_start_scan((uint8_t *)ssid, ssid_len,
                                         (uint8_t *)bssid, scan_mode, scan_option);
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    LOG_FUNC("wifi_connect_set_scan_ex(), ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Set scheduled scan ON/OFF
* wifi connect set sched_scan <on/off>
* @param [IN]on/off
*       0 OFF
*       1 ON
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_connect_set_sched_scan_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t flag = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi connect set sched_scan <onoff>");
        return -1;
    }

    flag = atoi(param[0]);
    ret = wifi_connnect_set_sched_scan(flag);

    LOG_FUNC("wifi_connnect_set_sched_scan(onoff=%d), ret:%s, Code=%d\n", flag,
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

int mtk_event_handler_sample(wifi_event_t event_id, unsigned char *payload,
                             unsigned int len)
{
    int handled = 0;
#ifdef MTK_MINISUPP_ENABLE
    uint16_t i = 0;

    switch (event_id) {
        case WIFI_EVENT_IOT_CONNECTED:
            handled = 1;
            if ((len == WIFI_MAC_ADDRESS_LENGTH + 1) && (payload))
                LOG_FUNC("[MTK Event Callback Sample]: LinkUp! CONNECTED MAC = %02x:%02x:%02x:%02x:%02x:%02x\n",
                         payload[0], payload[1], payload[2],
                         payload[3], payload[4], payload[5]);
            else
                LOG_FUNC("[MTK Event Callback Sample]: LinkUp!\n");
            break;
        case WIFI_EVENT_IOT_SCAN_COMPLETE:
            handled = 1;
            if (!g_scan_list) {
                LOG_FUNC("scan list is null!\n");
                break;
            }
            for (i = 0; i < g_scan_list_size; i++) {
                LOG_FUNC("\n%-4s%-20s%-20s%-7s%-7s%-7s%-7s%-7s%-7s%-7s\n", "Ch",
                         "SSID", "BSSID", "Valid", "Hidden", "Auth", "Cipher", "RSSI",
                         "bcn", "capInfo");
                LOG_FUNC("%-4d", g_scan_list[i].channel);
                LOG_FUNC("%-20s", g_scan_list[i].ssid);
                LOG_FUNC("%02x:%02x:%02x:%02x:%02x:%02x	",
                         g_scan_list[i].bssid[0],
                         g_scan_list[i].bssid[1],
                         g_scan_list[i].bssid[2],
                         g_scan_list[i].bssid[3],
                         g_scan_list[i].bssid[4],
                         g_scan_list[i].bssid[5]);
                LOG_FUNC("%-7d", g_scan_list[i].is_valid);
                LOG_FUNC("%-7d", g_scan_list[i].is_hidden);
                LOG_FUNC("%-7d", g_scan_list[i].auth_mode);
                LOG_FUNC("%-7d", g_scan_list[i].encrypt_type);
                LOG_FUNC("%-7d", g_scan_list[i].rssi);
                /*printf("%-7d", g_scan_list[i].is_wps_supported);
                LOG_FUNC("%-7d", g_scan_list[i].wps_element.configuration_methods);
                LOG_FUNC("%-7d", g_scan_list[i].wps_element.device_password_id);
                LOG_FUNC("%-7d", g_scan_list[i].wps_element.selected_registrar);*/
                LOG_FUNC("%-7d", g_scan_list[i].beacon_interval);
                LOG_FUNC("%-7d", g_scan_list[i].capability_info);
                LOG_FUNC("\n");
            }
            LOG_FUNC("[MTK Event Callback Sample]: Scan Done!\n");
            break;
        case WIFI_EVENT_IOT_DISCONNECTED:
            handled = 1;
            for (i = 0; i < len; i++)
                LOG_FUNC("[MTK Event Callback Sample], p[i]=%02x\n", payload[i]);
            if ((len == WIFI_MAC_ADDRESS_LENGTH + 1) && (payload))
                LOG_FUNC("[MTK Event Callback Sample]: Disconnect! DISCONNECTED MAC = %02x:%02x:%02x:%02x:%02x:%02x\n",
                         payload[0], payload[1], payload[2],
                         payload[3], payload[4], payload[5]);
            else
                LOG_FUNC("[MTK Event Callback Sample]: Disconnect!\n");
            break;
        case WIFI_EVENT_IOT_PORT_SECURE:
            handled = 1;
            if ((len == WIFI_MAC_ADDRESS_LENGTH + 1) && (payload))
                LOG_FUNC("[MTK Event Callback Sample]: Port Secure! CONNECTED MAC = %02x:%02x:%02x:%02x:%02x:%02x\n",
                         payload[0], payload[1], payload[2],
                         payload[3], payload[4], payload[5]);
            else
                LOG_FUNC("[MTK Event Callback Sample]: Port Secure!\n");
            break;
        case WIFI_EVENT_IOT_CONNECTION_FAILED:
            handled = 1;
            if ((len == WIFI_REASON_CODE_LENGTH + 1) && (payload)) {
                LOG_FUNC("[MTK Event Callback Sample]: CONNECTION_FAILED!\n");
                LOG_FUNC("Port=%d, Reason code=%d\n", payload[2],
                         (payload[1] << 8) + payload[0]);
            } else
                LOG_FUNC("[MTK Event Callback Sample]: CONNECTION_FAILED!\n");
            break;
        case WIFI_EVENT_IOT_ROAM_RUNNING:
            handled = 1;
            if ((len == WIFI_ROAM_TYPE_LENGTH + 1) && (payload)) {
                LOG_FUNC("[MTK Event Callback Sample]: ROAMING!\n");
                LOG_FUNC("Port=%d, Roam type=%d\n",
                         payload[WIFI_ROAM_TYPE_LENGTH],
                         payload[0]);
            } else
                LOG_FUNC("[MTK Event Callback Sample]: ROAMING!\n");
            break;
        case WIFI_EVENT_IOT_CSI_DATA_NOTIFICATION:
            handled = 1;
            if (payload)
                DBGLOG(REQ, STATE,
                       "[MTK Event Callback Sample]: CSI Data Received! Please refer to CSI document for the detail\n");
            else
                LOG_FUNC("[MTK Event Callback Sample]: CSI Data Received!\n");
            break;
        default:
            handled = 0;
            LOG_FUNC("[MTK Event Callback Sample]: Unknown event(%d)\n",
                     event_id);
            break;
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return handled;
}


/**
* @brief Example of Register WiFi Event Notifier
* wifi connect set eventcb <enable> <event ID>
* @param [IN]evt
* @param evt Event ID, More Event ID @sa #wifi_event_t
* @param [IN]enable 0: register, 1: unregister
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_connect_set_event_callback_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t enable = 0;
    uint8_t event_id = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi connect set eventcb <enable> <event ID>\n");
        return -1;
    }

    enable = atoi(param[0]);
    event_id = atoi(param[1]);
    if (enable == 0)
        ret = wifi_connection_unregister_event_handler((wifi_event_t)event_id,
                                                       (wifi_event_handler_t) mtk_event_handler_sample);
    else
        ret = wifi_connection_register_event_handler((wifi_event_t)event_id,
                                                     (wifi_event_handler_t) mtk_event_handler_sample);

    LOG_FUNC("wifi_connect_set_event_callback(), ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of configure rx filter for packets wanted to be received
* wifi config set rxfilter <flag>
* @parameter
*    [IN]flag defined in  wifi_rx_filter_t
* @return =0 means success, >0 means fail
* @note Default value will be WIFI_DEFAULT_IOT_RX_FILTER
*/
uint8_t wifi_config_set_rx_filter_ex(uint8_t len, char *param[])
{

    int32_t ret = 0;
    int32_t i4Ret = 0;
    uint32_t flag = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set rxfilter <flag> \n");
        return -1;
    }

    i4Ret = kalkStrtos32(param[0], 0, &flag);
    if (i4Ret)
        LOG_FUNC("parse aucValue error i4Ret=%d\n", i4Ret);
    ret = wifi_config_set_rx_filter(flag);

    LOG_FUNC("wifi_config_set_rxfilter: 0x%x, ret:%s, Code=%d\n",
             (unsigned int) flag & g_rx_filter_mask,
             WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

/**
* @brief Example of configure rx filter for packets wanted to be received
* wifi config set rxfilter <flag>
* @parameter
*    [IN]flag defined in  wifi_rx_filter_t
* @return =0 means success, >0 means fail
* @note Default value will be WIFI_DEFAULT_IOT_RX_FILTER
*/
uint8_t wifi_config_set_mc_address_ex(uint8_t len, char *param[])
{

    int32_t ret = 0;
    uint32_t i, j;
    char *s, *e;
    const int8_t delim[] = "-:";
    uint8_t McAddrList[MAX_NUM_CLI_GROUP_ADDR][MAC_ADDR_LEN];
    if (len > 8) {
        LOG_USAGE("Usage: wifi config set rxfilter <IP1> [<IP2> <IP3> ...], support %d addresses\n",
                  MAX_NUM_CLI_GROUP_ADDR);
        return -1;
    }

    LOG_FUNC("set %d mc addresses\n", len);
    for (i = 0; i < len; i++) {
        s = param[i];
        for (j = 0; (j < MAC_ADDR_LEN) && (s != NULL); j++) {
            e = (char *)kalStrtokR((int8_t *)s, delim, (int8_t **)&s);
            if (e)
                McAddrList[i][j] = ((char)kalAtoi(e[0]) << 4) |
                                   (char)kalAtoi(e[1]);
        }
        LOG_FUNC(MACSTR,
                 MAC2STR(McAddrList[i]));
    }
    LOG_FUNC("\n");

    ret = wifi_config_set_mc_address(len, (void *)McAddrList);

    LOG_FUNC("wifi_config_set_mc_address ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

/**
* @brief Example of get rx filter for packets format wanted to be received
* wifi config get rxfilter
* @parameter
*    [OUT]flag defined in  wifi_rx_filter_t
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_get_rx_filter_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint32_t flag = 0;

    ret = wifi_config_get_rx_filter(&flag);

    LOG_FUNC("wifi_config_get_rxfilter: 0x%x, ret:%s, Code=%d\n",
             (unsigned int) flag, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

int wlan_raw_pkt_rx_filter_sample(uint8_t *payload, unsigned int len)
{
    struct wifi_data_parse_t data_info = {0};
    uint8_t sub_type = 0;
    uint8_t retry = 0;
    uint8_t frame_type = 0;

    if (wifi_connection_parse_data_descriptor(payload, &data_info) < 0)
        return 0;/* Not handled */

    retry = ((data_info.mac_header[1]) & BIT(3)) >> 3;
    if (retry)
        LOG_FUNC("Duplicate frames\r\n");

    frame_type = (*(uint8_t *)(data_info.mac_header) &
                  MASK_FC_TYPE) >> 2;
    sub_type = (*(uint8_t *)(data_info.mac_header) &
                MASK_FC_SUBTYPE) >> OFFSET_OF_FC_SUBTYPE;

    if (frame_type == 0) { /*Management frame*/
        switch (sub_type) {
            case 0:
                LOG_FUNC("Management, Association request");
                break;
            case 1:
                LOG_FUNC("Management, Association response");
                break;
            case 2:
                LOG_FUNC("Management, Reassociation request");
                break;
            case 3:
                LOG_FUNC("Management, Reassociation response");
                break;
            case 4:
                LOG_FUNC("Management, Probe request");
                break;
            case 5:
                LOG_FUNC("Management, Probe response");
                break;
            case 8:
                LOG_FUNC("Management, Beacon");
                break;
            case 9:
                LOG_FUNC("Management, ATIM");
                break;
            case 10:
                LOG_FUNC("Management, Disassociation");
                break;
            case 11:
                LOG_FUNC("Management, Authentication");
                break;
            case 12:
                LOG_FUNC("Management, Deauthentication");
                break;
            case 13:
                LOG_FUNC("Management, Action Frame");
                break;
            case 14:
                LOG_FUNC("Management, NACK");
                break;
            default:
                LOG_FUNC("wrong management frame");
                break;
        }
    } else if (frame_type == 1) { /*Control frame*/
        switch (sub_type) {
            case 11:
                LOG_FUNC("Control, RTS");
                break;
            case 12:
                LOG_FUNC("Control, CTS");
                break;
            default:
                break;
        }
    }

    return 1;/* handled */
}

uint8_t wifi_config_set_rx_handler_mode_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t mode = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set rx_mode <mode>");
        return -1;
    }

    mode = (uint8_t)atoi(param[0]);
    ret =  wifi_config_set_rx_handler_mode(mode);

    LOG_FUNC("wifi_config_set_rx_handler_mode(%d), ret:%s, Code=%d\n",
             (unsigned int)mode, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

/**
* @brief Example of Set WiFi Raw Packet Receiver
* wifi config set rxraw <enable>
* @param [IN]enable 0: unregister, 1: register
*
* @return  =0 means success, <0 means fail
*/
uint8_t wifi_config_set_rx_raw_pkt_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t enable = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set rxraw <enable> \n");
        return 1;
    }

    enable = atoi(param[0]);
    if (enable == 0)
        ret = wifi_config_unregister_rx_handler();
    else
        ret = wifi_config_register_rx_handler((wifi_rx_handler_t) wlan_raw_pkt_rx_filter_sample);

    LOG_FUNC("wifi_config_set_rx_raw_pkt, enable=%d, ret:%s, Code=%d\n",
             enable, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}


int wlan_wow_magic_pkt_handler_sample(uint8_t *ether_frame, unsigned int len)
{
    uint8_t *pCur = ether_frame;
    uint8_t *pIpHdr;
    uint16_t u2IpTotalLen, u2UdpTotallen;
    uint32_t u4IPv4HdrLen = 0;

    LOG_FUNC("EtherFrame[%p] Len[%d]", ether_frame, len);
    if (pCur == NULL || len <= 0)
        return -1;

    if (len < ETH_HLEN + IP_HLEN)
        return -1;

    LOG_FUNC("RA: "MACSTR, MAC2STR(pCur));
    pCur += MAC_ADDR_LEN;
    len -= MAC_ADDR_LEN;
    LOG_FUNC("TA: "MACSTR, MAC2STR(pCur));
    pCur += MAC_ADDR_LEN;
    len -= MAC_ADDR_LEN;
    LOG_FUNC("Ether Type: [%04x]", (((uint16_t)pCur[0] << 8) | (uint16_t)pCur[1]));
    pCur += ETHER_TYPE_LEN;
    len -= ETHER_TYPE_LEN;

    pIpHdr = pCur;
    u4IPv4HdrLen = (pIpHdr[0] & 0xf) << 2;
    LOG_FUNC("Protocol: [%x]", pIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET]);

    u2IpTotalLen = (((uint16_t)pIpHdr[2] << 8) | (uint16_t)pIpHdr[3]);
    if (len < u2IpTotalLen)
        return -1;

    pCur += u4IPv4HdrLen;
    /* dump payload of UDP type magic packet */
    if (pIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET] == IP_PROTO_UDP) {
        u2UdpTotallen = (((uint16_t)pCur[4] << 8) | (uint16_t)pCur[5]);
        LOG_FUNC("UDP length: [%d]", u2UdpTotallen);
        LOG_FUNC("Src IP: [%d:%d:%d:%d]",
                 pIpHdr[12], pIpHdr[13], pIpHdr[14], pIpHdr[15]);

        pCur += UDP_HDR_LEN;
        pCur += MAC_ADDR_LEN;
        LOG_FUNC("Payload MAC: "MACSTR, MAC2STR(pCur));
    }
    return 1;/* handled */
}

/**
* @brief Set WiFi Magic Packet default handler enable
* wifi config set wow_handler <enable>
* @param [IN]enable 0: unregister, 1: register
*
* @return  =0 means success, <0 means fail
*/
uint8_t wifi_config_set_wow_handler_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t enable = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set wow_handler <enable> \n");
        return -1;
    }

    enable = atoi(param[0]);
    if (enable == 0)
        ret = wifi_config_unregister_wow_handler(WIFI_WOW_TYPE_MAGIC);
    else
        ret = wifi_config_register_wow_handler((wifi_rx_handler_t) wlan_wow_magic_pkt_handler_sample, WIFI_WOW_TYPE_MAGIC);

    LOG_FUNC("%s, enable=%d, ret:%s, Code=%d\n",
             __func__, enable, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

/**
* @brief Example of Reload configuration
* wifi config set reload
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_config_set_reload_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;

    ret = wifi_config_reload_setting();

    LOG_FUNC("WiFi reload configuration, ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_clear_bcn_lost_cnt_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;

    ret = wifi_config_clear_bcn_lost_cnt();

    LOG_FUNC("wifi_config_clear_bcn_lost_cnt_ex ret:%s, Code=%d\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}


uint8_t wifi_config_get_bcn_lost_cnt_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint16_t bcn_lost_cnt = 0;

    ret = wifi_config_get_bcn_lost_cnt(&bcn_lost_cnt);

    LOG_FUNC("wifi_config_get_bcn_lost_cnt_ex: value=%d, ret:%s, Code=%d\n",
             bcn_lost_cnt, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

uint8_t wifi_config_set_pretbtt_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t value = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set pretbtt <value>\n");
        return -1;
    }

    value = atoi(param[0]);
    ret = wifi_config_set_pretbtt(value);

    LOG_FUNC("wifi_config_set_pretbtt_ex: value=%d, ret:%s, Code=%d\n",
             value, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_get_pretbtt_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t pretbtt_value = 0;

    ret = wifi_config_get_pretbtt(&pretbtt_value);

    LOG_FUNC("wifi_config_get_pretbtt_ex: value=%d, ret:%s, Code=%d\n",
             pretbtt_value, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

uint8_t wifi_config_set_ps_log_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t value = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set ps_log <on/off>\n");
        return -1;
    }

    value = atoi(param[0]);
    ret = wifi_config_set_ps_log(value);

    LOG_FUNC("wifi_config_set_ps_log_ex(value=%d), ret:%s, Code=%u\n",
             value, ret == WLAN_STATUS_SUCCESS ? "Success" : "Error", ret);
    return ret == 0 ? 0 : 1;
}

uint8_t wifi_config_get_ps_log_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t value = 0;

    ret = wifi_config_get_ps_log(&value);

    LOG_FUNC("wifi_config_get_ps_log_ex status:(value=%d), ret:%s, Code=%d\n",
             value, WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;
}

uint8_t wifi_config_get_sys_temperature_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t value = 0;

    ret = wifi_config_get_sys_temperature(&value);

    LOG_FUNC("wifi_config_get_sys_temperature, ret:%d, temperature=%d\n",
             ret, (int)value);

    return ret;
}

uint8_t wifi_config_set_tx_rate_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t fixed = atoi(param[0]);
    uint8_t mode = atoi(param[1]);
    uint8_t bw = atoi(param[2]);
    uint8_t mcs = atoi(param[3]);

    if (len != 1 && len != 4) {
        LOG_FUNC("Option support 1 or 4 parameters");
        LOG_FUNC("Option:");
        LOG_FUNC("wifi config set tx_rate 0 - auto");
        LOG_FUNC("wifi config set tx_rate 1 <mode> <bw> <mcs> - fixed");
        LOG_FUNC("[Mode]CCK=0, OFDM=1, HT=2, VHT=4,");
        LOG_FUNC("HE_SU=8, HE_ER_SU=9");
        LOG_FUNC("[BW]BW20=0 BW40=1 BW80=2 BW160=3");
        LOG_FUNC(
            "[MCS]CCK=0~3, OFDM=0~7, HT=0~32, VHT=0~9, HE=0~11");
        return -1;
    }

    if (len == 1) {
        if (fixed == 0) {
            ret = wifi_config_set_tx_rate(0, 0, 0, 0);
        } else {
            LOG_FUNC("Wrong params: only support 0 for auto");
            LOG_FUNC("Or enter 4 length for fixed rate");
            LOG_FUNC(
                "wifi config set tx_rate 1 <mode> <bw> <mcs> - fixed");
            return -1;
        }
    } else {
        ret = wifi_config_set_tx_rate(fixed,
                                      mode, bw, mcs);
    }

    LOG_FUNC("wifi_config_set_tx_rate, ret:%s, fixed=%d",
             WIFI_CLI_RETURN_STRING(ret), fixed);
    if (len > 1) {
        LOG_FUNC("mode=%d, bw=%d, mcs=%d",
                 mode, bw, mcs);
    }

    return ret;
}

#if IP_NAPT
uint8_t wifi_config_set_napt_entry_timer_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t timer;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set napt_timer <sec>");
        return -1;
    }

    timer = atoi(param[0]);
    ret = wifi_config_set_napt_entry_timer(timer);

    LOG_FUNC("wifi_config_set_napt_entry_timer(timer=%d), ret:%s, Code=%d\n",
             timer, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}
#endif /* #if IP_NAPT */

#if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1)
uint8_t wifi_config_set_arp_offload_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t enable;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set arp offload <enable/disable>");
        return -1;
    }

    enable = atoi(param[0]);
    ret = wifi_config_set_arp_offload(enable);

    LOG_FUNC("wifi_config_set_arp_offload(enable=%d), ret:%s, Code=%d\n",
             enable, WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}
#endif /* #if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1) */

int32_t wifi_config_get_ip_addr(uint8_t port, uint8_t *ip_dst, uint8_t type)
{
    struct netif *prNetif = wlan_sta_netif;
    if (type > 1) {
        LOG_FUNC("%s: error type: %d", __func__, type);
        return -1;
    }
#if LWIP_DHCP
    if (port == WIFI_PORT_STA) {
        if (lwip_get_ipmode() == REQ_IP_MODE_DHCP) {
            LOG_FUNC("mode: dhcp\r\n");
            if (dhcp_supplied_address(prNetif)) {
                struct dhcp *d = netif_dhcp_data(prNetif);
                if (type == 0) {
                    LOG_FUNC("ip: %s\r\n",
                             ip4addr_ntoa(&d->offered_ip_addr));
                    strncpy((char *)ip_dst,
                            ip4addr_ntoa(&d->offered_ip_addr),
                            WIFI_IP_BUFFER_LENGTH);
                    return 0;
                } else if (type == 1) {
                    LOG_FUNC("netmask: %s\r\n",
                             ip4addr_ntoa(&d->offered_sn_mask));
                    strncpy((char *)ip_dst,
                            ip4addr_ntoa(&d->offered_sn_mask),
                            WIFI_IP_BUFFER_LENGTH);
                    return 0;
                }

            } else {
                LOG_FUNC("%s: dhcp on going\r\n", __func__);
                return -1;
            }
        } else
#endif /* #if LWIP_DHCP */
        {
            LOG_FUNC("mode: static\r\n");
            if (type == 0) {
                LOG_FUNC("ip: %s\r\n",
                         ip4addr_ntoa(ip_2_ip4(&prNetif->ip_addr)));
                strncpy((char *)ip_dst,
                        ip4addr_ntoa(ip_2_ip4(&prNetif->ip_addr)),
                        WIFI_IP_BUFFER_LENGTH);
                return 0;
            } else if (type == 1) {
                LOG_FUNC("netmask: %s\r\n",
                         ip4addr_ntoa(ip_2_ip4(&prNetif->netmask)));
                strncpy((char *)ip_dst,
                        ip4addr_ntoa(ip_2_ip4(&prNetif->netmask)),
                        WIFI_IP_BUFFER_LENGTH);
                return 0;
            }
        }
    } else {
        LOG_FUNC("%s: not support port: %d\n", __func__, port);
        return -1;
    }
    return -1;
}
int32_t wifi_config_set_ip_addr(uint8_t port, uint8_t *address, uint8_t type)
{
    if (address == NULL) {
        LOG_FUNC("input is null.");
        return -1;
    }

#ifdef MTK_NVDM_ENABLE
    char buf[128] = {0};
    int8_t status;
    LOG_FUNC("%s: type: %d ip address = %d.%d.%d.%d.\n",
             __func__, type, address[0], address[1], address[2], address[3]);

    status = kalSnprintf(buf, WIFI_IP_BUFFER_LENGTH,
                         "%d.%d.%d.%d",
                         address[0], address[1], address[2], address[3]);
    if (status < 0)
        return -1;
    if (type == 0) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_NETWORK, "IpAddr",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)buf, kalStrLen(buf)) ==
            NVDM_STATUS_OK)
            return 0;
    } else if (type == 1) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_NETWORK, "IpNetmask",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)buf, kalStrLen(buf)) ==
            NVDM_STATUS_OK)
            return 0;
    } else if (type == 2) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_NETWORK, "IpGateway",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)buf, kalStrLen(buf)) ==
            NVDM_STATUS_OK)
            return 0;
    }
#endif /* #ifdef MTK_NVDM_ENABLE */

    return -1;
}
uint8_t wifi_config_set_ip_addr_ex(uint8_t len, char *param[])
{
    uint8_t status = -1;
    int32_t port = port_sanity_check(param[0]);
    uint8_t ip[4] = {0};
    uint8_t cert_def_netmask[4] = {255, 255, 255, 0};

    if (len < 2 || len > 4) {
        LOG_USAGE("usage: wifi config set ip_addr <port> <ip> [<netmask> <gateway>]\n");
        return -1;
    }
    if (port < 0)
        return -1;

    if (port == WIFI_PORT_STA) {
        wifi_conf_get_ip_from_str(ip, param[1]);
        status = wifi_config_set_ip_addr((uint8_t)port, ip, 0);
        if (status != 0)
            goto set_ip_end;

        if (len >= 3) {

            if (len == 4) {
                wifi_conf_get_ip_from_str(ip, param[3]);
                status = wifi_config_set_ip_addr((uint8_t)port,
                                                 ip, 2);
            } else {
                ip[3] = (uint8_t)254;
                status = wifi_config_set_ip_addr((uint8_t)port,
                                                 ip, 2);
            }
            if (status != 0)
                goto set_ip_end;

            wifi_conf_get_ip_from_str(ip, param[2]);
            status = wifi_config_set_ip_addr((uint8_t)port, ip, 1);
            if (status != 0)
                goto set_ip_end;
        } else {
            status = wifi_config_set_ip_addr((uint8_t)port,
                                             cert_def_netmask, 1);
            if (status != 0)
                goto set_ip_end;

            ip[3] = (uint8_t)254;
            status = wifi_config_set_ip_addr((uint8_t)port, ip, 2);
            if (status != 0)
                goto set_ip_end;
        }
    } else {
        LOG_FUNC("%s: error port: %d, current suppot STA, port = 0\n",
                 __func__, port);
        status = -1;
    }

set_ip_end:
    return status;
}
uint8_t wifi_config_set_ip_mode_ex(uint8_t len, char *param[])
{
    int32_t port = port_sanity_check(param[0]);

    if (len < 2) {
        LOG_USAGE("usage: wifi config set ip_mode <port> <mode>");
        return -1;
    }
    if (port < 0)
        return -1;

    if (port == WIFI_PORT_STA) {
#ifdef MTK_NVDM_ENABLE
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_NETWORK, "IpMode",
                                 0x02, (uint8_t *)param[1], kalStrLen(param[1])) == 0) {

            LOG_FUNC("set ip_mode %s success\n", param[1]);
            return 0;
        }
#endif /* #ifdef MTK_NVDM_ENABLE */
    } else {
        LOG_FUNC("%s: not support port: %d, current suppot STA(0)\n",
                 __func__, port);
        return -1;
    }
    LOG_FUNC("set ip_mode %s fail\n", param[1]);
    return -1;
}

uint8_t wifi_config_set_retry_limit_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;

    if (atoi(param[0]) > 31) {
        LOG_FUNC("The limit must be less than or equal to 31");
        return -1;
    }
    ret = wifi_config_set_retry_limit(atoi(param[0]));

    LOG_FUNC("wifi_config_set_retry_limit, ret:%s, linit=%d",
             WIFI_CLI_RETURN_STRING(ret), atoi(param[0]));

    return ret;
}


uint8_t wifi_config_get_twt_param_ex(uint8_t len, char *param[])
{
    int ret = 0;
    struct wifi_twt_params_t rTwt;

    ret = wifi_config_get_twt_param(&rTwt);

    if (ret < 0) {
        LOG_FUNC("get TWT Section fail, status[%d]", ret);
    } else {
        LOG_FUNC("TWT Flow ID [%d]", rTwt.ucTWTFlowID);
        LOG_FUNC("Setup[%d] Trig[%d] Unann[%d] Exp[%d] Prot[%d] WDur[%d] WIntvMant[%d]",
                 rTwt.ucTWTSetup,
                 rTwt.ucTWTTrigger,
                 rTwt.ucTWTUnannounced,
                 rTwt.ucTWTWakeIntervalExponent,
                 rTwt.ucTWTProtection,
                 rTwt.ucTWTMinWakeDuration,
                 rTwt.u2TWTWakeIntervalMantissa);
    }
    return 0;
}
/**
* @brief Example of WiFi TWT ON/OFF
* wifi config set twt <params> <...> ..., len = 2 or 9
* @param [IN] Wake_Dutation
*        For example, Nominal Minimum TWT Wake Duration = 126
*        126 means (126 * 256) us = 32.256 ms (322us)
* @param [IN] Wake_Interval
*        For example, TWT Wake Interval Mantissa = 64
*        64 * 2^10 = 64 * 1024 us (65536 us)
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_set_twt_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    struct wifi_twt_params_t rTwt;

    if (g_u4HaltFlag) {
        LOG_FUNC("wlan is halt, skip priv_driver_cmds");
        return -1;
    }

    kalMemZero(&rTwt, sizeof(struct wifi_twt_params_t));
    if ((len == 2 && 5 <= atoi(param[0]) && atoi(param[0]) <= 7) ||
        (len == 9 && atoi(param[0]) < 5)) {
        if (len == 2) {
            rTwt.ucTWTNego = atoi(param[0]);
            rTwt.ucTWTFlowID = atoi(param[1]);
        } else {
            rTwt.ucTWTNego = atoi(param[0]);
            rTwt.ucTWTFlowID = atoi(param[1]);
            rTwt.ucTWTSetup = atoi(param[2]);
            rTwt.ucTWTTrigger = atoi(param[3]);
            rTwt.ucTWTUnannounced = atoi(param[4]);
            rTwt.ucTWTWakeIntervalExponent = atoi(param[5]);
            rTwt.ucTWTProtection = atoi(param[6]);
            rTwt.ucTWTMinWakeDuration = atoi(param[7]);
            rTwt.u2TWTWakeIntervalMantissa = atoi(param[8]);
        }
    } else {
        LOG_FUNC("Invalid TWT Params\n");
        return -1;
    }

    ret = wifi_config_set_twt(&rTwt);

    LOG_FUNC("wifi_config_set_twt, ret:%s, Code=%d",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);

    return ret;

}

uint8_t wifi_config_set_bc_drop_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;

    if (len < 1) {
        LOG_FUNC("Usage: wifi config set bmcdrop <0/1>\n");
        return -1;
    }

    ret = wifi_config_set_bc_drop((uint8_t)atoi(param[0]));

    LOG_FUNC("wifi_config_set_BMCDrop [%s], ret:%s, Code=%d\n",
             (uint8_t)atoi(param[0]) ? "Enable" : "Disable",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_set_wow_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set wow <0/1>\n");
        return -1;
    }

    ret = wifi_config_set_wow((uint8_t)atoi(param[0]));

    LOG_FUNC("wifi_config_set_wow [%s], ret:%s, Code=%x\n",
             (uint8_t)atoi(param[0]) ? "Enable" : "Disable",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_get_wow_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t mode = 0;

    ret = wifi_config_get_wow(&mode);

    LOG_FUNC("wifi_config_get_wow [%s], ret:%s, Code=%x\n",
             (unsigned int)mode ? "Enable" : "Disable",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_get_wow_reason_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t reason = 0;

    ret = wifi_config_get_wow_reason(&reason);

    LOG_FUNC("wifi_config_get_wow_reason [%u], ret:%s, Code=%x\n",
             reason,
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_set_wow_udp_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t i;
    struct wifi_wow_ports_t arWowPorts;
    uint8_t op = ((WOW_PORT_IPPROTO_UDP << WOW_PORT_IPPROTO_SHFT) |
                  (WOW_PORT_OP_SET << WOW_PORT_OP_SHFT));

    if (len < 1 || len > MAX_TCP_UDP_PORT) {
        LOG_USAGE("Usage: wifi config set wow_udp <port0> <port1> ...\n");
        LOG_FUNC("Support %d UDP IPv4 ports\n", MAX_TCP_UDP_PORT);
        return -1;
    }

    for (i = 0; i < len; i++) {
        arWowPorts.ports[i] = (uint16_t)atoi(param[i]);
    }
    arWowPorts.len = len;

    ret = wifi_config_wow_port(&arWowPorts, op);

    LOG_FUNC("wifi_config_set_wow_udp, ret:%s, Code=%x\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_set_wow_udp_del_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    struct wifi_wow_ports_t arWowPorts;
    uint8_t op = ((WOW_PORT_IPPROTO_UDP << WOW_PORT_IPPROTO_SHFT) |
                  (WOW_PORT_OP_UNSET << WOW_PORT_OP_SHFT));

    ret = wifi_config_wow_port(&arWowPorts, op);

    LOG_FUNC("wifi_config_set_wow_udp_del, ret:%s, Code=%x\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_get_wow_udp_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t i;
    struct wifi_wow_ports_t arWowPorts;
    uint8_t op = ((WOW_PORT_IPPROTO_UDP << WOW_PORT_IPPROTO_SHFT) |
                  (WOW_PORT_OP_GET << WOW_PORT_OP_SHFT));

    arWowPorts.len = MAX_TCP_UDP_PORT;
    ret = wifi_config_wow_port(&arWowPorts, op);

    LOG_FUNC("WOW UDP IPv4 Port List\n");
    for (i = 0; i < arWowPorts.len; i++) {
        LOG_FUNC("%d ", arWowPorts.ports[i]);
    }
    LOG_FUNC("\n%d ports enable!\n\n", arWowPorts.len);

    LOG_FUNC("wifi_config_get_wow_udp, ret:%s, Code=%x\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_set_wow_tcp_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t i;
    struct wifi_wow_ports_t arWowPorts;
    uint8_t op = ((WOW_PORT_IPPROTO_TCP << WOW_PORT_IPPROTO_SHFT) |
                  (WOW_PORT_OP_SET << WOW_PORT_OP_SHFT));

    if (len < 1 || len > MAX_TCP_UDP_PORT) {
        LOG_USAGE("Usage: wifi config set wow_tcp <port0> <port1> ...\n");
        LOG_FUNC("Support %d TCP IPv4 ports\n", MAX_TCP_UDP_PORT);
        return -1;
    }

    for (i = 0; i < len; i++) {
        arWowPorts.ports[i] = (uint16_t)atoi(param[i]);
    }
    arWowPorts.len = len;

    ret = wifi_config_wow_port(&arWowPorts, op);

    LOG_FUNC("wifi_config_set_wow_tcp, ret:%s, Code=%x\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_set_wow_tcp_del_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    struct wifi_wow_ports_t arWowPorts;
    uint8_t op = ((WOW_PORT_IPPROTO_TCP << WOW_PORT_IPPROTO_SHFT) |
                  (WOW_PORT_OP_UNSET << WOW_PORT_OP_SHFT));

    ret = wifi_config_wow_port(&arWowPorts, op);

    LOG_FUNC("wifi_config_set_wow_tcp_del, ret:%s, Code=%x\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_get_wow_tcp_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    uint8_t i;
    struct wifi_wow_ports_t arWowPorts;
    uint8_t op = ((WOW_PORT_IPPROTO_TCP << WOW_PORT_IPPROTO_SHFT) |
                  (WOW_PORT_OP_GET << WOW_PORT_OP_SHFT));

    arWowPorts.len = MAX_TCP_UDP_PORT;
    ret = wifi_config_wow_port(&arWowPorts, op);

    LOG_FUNC("WOW TCP IPv4 Port List\n");
    for (i = 0; i < arWowPorts.len; i++) {
        LOG_FUNC("%d ", arWowPorts.ports[i]);
    }
    LOG_FUNC("\n%d ports enable!\n\n", arWowPorts.len);

    LOG_FUNC("wifi_config_get_wow_tcp, ret:%s, Code=%x\n",
             WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

#if CFG_SUPPORT_ROAMING_CUSTOMIZED
uint8_t wifi_config_set_roam_delta_ex(uint8_t len, char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    uint8_t roamingDelta = 0;;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set rssidelta <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    roamingDelta = (uint8_t)atoi(param[1]);
    status = wifi_config_set_roam_delta(port, roamingDelta);

    LOG_FUNC("save Roaming Delta = %d %s.\n", roamingDelta,
             status ? "fail" : "done");

    return status;
}

uint8_t wifi_config_get_roam_delta_ex(uint8_t len, char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    uint8_t roamingDelta = 0;


    if (len < 1) {
        LOG_USAGE("Usage: wifi config get rssidelta <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    status = wifi_config_get_roam_delta(port, &roamingDelta);

    LOG_FUNC("fetch Roaming Delta = %d done.\n", status ? -1 : roamingDelta,
             status ? "fail" : "done");

    return status;
}

uint8_t wifi_config_set_roam_scan_channel_ex(uint8_t len,
                                             char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    uint8_t fgScanFullCh = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set roamscanch <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    fgScanFullCh = (uint8_t)atoi(param[1]);
    status = wifi_config_set_roam_scan_channel(port, fgScanFullCh);

    LOG_FUNC("save roaming scan channel = %d %s.\n", fgScanFullCh,
             status ? "fail" : "done");

    return status;
}

uint8_t wifi_config_get_roam_scan_channel_ex(uint8_t len,
                                             char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    uint8_t fgScanFullCh = 0;


    if (len < 1) {
        LOG_USAGE("Usage: wifi config get roamscanch <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    status = wifi_config_get_roam_scan_channel(port, &fgScanFullCh);

    LOG_FUNC("fetch roaming scan channel = %d %s.\n",
             status ? -1 : fgScanFullCh, status ? "fail" : "done");

    return status;
}

uint8_t wifi_config_get_roam_type_ex(uint8_t len,
                                     char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    uint8_t type = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get roamtype <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    status = wifi_config_get_roam_type(port, &type);

    if (!status)
        LOG_FUNC("fetch roaming type = 0x%x done.\n", type);
    else
        LOG_FUNC("fetch roaming type fail.\n");

    return status;
}

uint8_t wifi_config_is_connect_ft_ap_ex(uint8_t len,
                                        char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int value = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get isconnectftap <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    status = wifi_config_is_connect_ft_ap(port, &value);

    if (!status)
        LOG_FUNC("is connected ft ap = %d done.\n", value);
    else
        LOG_FUNC("fetch connected type fail.\n");

    return status;
}

uint8_t wifi_config_get_roam_statistic_ex(uint8_t len,
                                          char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    struct roam_statistic_t roam_stat;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get roamstatistic <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    status = wifi_config_get_roam_statistic(port, &roam_stat);

    if (!status)
        LOG_FUNC("roam_by_rssi = %d, roam_by_bcnmiss = %d done.\n",
                 roam_stat.num_rssi_roam, roam_stat.num_bcnMiss_roam);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_clear_roam_statistic_ex(uint8_t len,
                                            char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set clearroamstatistic <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    status = wifi_config_clear_roam_statistic(port);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}


uint8_t wifi_config_set_roam_by_rssi_ex(uint8_t len,
                                        char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set RoamingByRssi <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_roam_by_rssi(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_get_roam_by_rssi_ex(uint8_t len,
                                        char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get RoamingByRssi <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (uint8_t)atoi(param[1]);
    status = wifi_config_get_roam_by_rssi(port, &u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_set_roam_rssithreshold_ex(uint8_t len,
                                              char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set RoamingRssiValue <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_roam_rssithreshold(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
uint8_t wifi_config_get_roam_rssithreshold_ex(uint8_t len,
                                              char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get RoamingRssiValue <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (uint8_t)atoi(param[1]);
    status = wifi_config_get_roam_rssithreshold(port, &u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_set_roam_by_bcnmiss_ex(uint8_t len,
                                           char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set RoamingByBcn <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_roam_by_bcnmiss(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
uint8_t wifi_config_get_roam_by_bcnmiss_ex(uint8_t len,
                                           char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get RoamingByBcn <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (uint8_t)atoi(param[1]);
    status = wifi_config_get_roam_by_bcnmiss(port, &u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_set_roam_by_bcnmissthreshold_ex(uint8_t len,
                                                    char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set ScreenOnBeaconTimeoutCount <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_roam_by_bcnmissthreshold(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
uint8_t wifi_config_get_roam_by_bcnmissthreshold_ex(uint8_t len,
                                                    char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get ScreenOnBeaconTimeoutCount <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (uint8_t)atoi(param[1]);
    status = wifi_config_get_roam_by_bcnmissthreshold(port, &u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_set_roam_block_time_ex(uint8_t len,
                                           char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set RoamingBlockTimeSec <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_roam_block_time(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
uint8_t wifi_config_get_roam_block_time_ex(uint8_t len,
                                           char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get RoamingBlockTimeSec <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (uint8_t)atoi(param[1]);
    status = wifi_config_get_roam_block_time(port, &u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_set_roam_lock_time_ex(uint8_t len,
                                          char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set RoamingRetryTimeSec <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_roam_lock_time(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
uint8_t wifi_config_get_roam_lock_time_ex(uint8_t len,
                                          char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get RoamingRetryTimeSec <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (uint8_t)atoi(param[1]);
    status = wifi_config_get_roam_lock_time(port, &u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_set_roam_maxlock_count_ex(uint8_t len,
                                              char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set RoamingRetryLimit <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_roam_maxlock_count(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
uint8_t wifi_config_get_roam_maxlock_count_ex(uint8_t len,
                                              char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get RoamingRetryLimit <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (uint8_t)atoi(param[1]);
    status = wifi_config_get_roam_maxlock_count(port, &u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_set_bto_time_ex(uint8_t len,
                                    char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set RoamingBeaconTimeSec <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_bto_time(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
uint8_t wifi_config_get_bto_time_ex(uint8_t len,
                                    char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get RoamingBeaconTimeSec <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (uint8_t)atoi(param[1]);
    status = wifi_config_get_bto_time(port, &u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_set_stable_time_ex(uint8_t len,
                                       char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set RoamingStableTimeSec <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_stable_time(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_set_split_scan_ex(uint8_t len,
                                      char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set splitscan <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_split_scan(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_get_stable_time_ex(uint8_t len,
                                       char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get RoamingStableTimeSec <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (uint8_t)atoi(param[1]);
    status = wifi_config_get_stable_time(port, &u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_set_autoroam_ex(uint8_t len,
                                    char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set autoroam <port> <value>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (int32_t)atoi(param[1]);
    status = wifi_config_set_autoroam(port, u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
uint8_t wifi_config_get_autoroam_ex(uint8_t len,
                                    char *param[])
{
    int32_t port = 0;
    int32_t status = 0;
    int32_t u4Param = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get autoroam <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    u4Param = (uint8_t)atoi(param[1]);
    status = wifi_config_get_autoroam(port, &u4Param);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, u4Param);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

#endif /* #if CFG_SUPPORT_ROAMING_CUSTOMIZED */

#if CFG_CHIP_RESET_SUPPORT
uint8_t wifi_config_set_ser_ex(uint8_t len, char *param[])
{
    int32_t status = 0;
    uint8_t op = 0;;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set ser <port> <op>\n\n");
        LOG_USAGE("op:\n");
        LOG_USAGE("     0 | trigger N10 coredump\n");
        LOG_USAGE("     1 | check bus hang\n");
        LOG_USAGE("     2 | trigger SER\n");
        LOG_USAGE("others | reserved\n");
        return -1;
    }

    op = (uint8_t)atoi(param[0]);
    status = wifi_config_set_ser(op);

    if (!status)
        LOG_FUNC("%s %d done.\n", __func__, op);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
#endif /* #if CFG_CHIP_RESET_SUPPORT */

#if CFG_SUPPORT_NON_PREF_CHAN
uint8_t wifi_config_set_non_pref_chan_ex(uint8_t len, char *param[])
{
    int32_t ret = 0;
    int32_t port = 0;

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

#ifdef MTK_MINISUPP_ENABLE
    ret = wifi_config_set_non_pref_chan(port, len < 2 ? NULL : (uint8_t *)param[1],
                                        len < 2 ? 0 : os_strlen(param[1]));
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    LOG_FUNC("wifi_config_set_non_pref_chan(port:%d), [%s], ret:%s, Code=%d\n",
             (int)port, param[1], WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}
#endif /* #if CFG_SUPPORT_NON_PREF_CHAN */

#if CFG_SUPPORT_11KV_SWITCH
uint8_t wifi_config_set_disable_11k_ex(uint8_t len, char *param[])
{
    int32_t ret;
    int32_t port;
    uint8_t val;

    if (len != 2) {
        LOG_USAGE("Usage: wifi config set disable_11k <port> <val>\n");
        LOG_USAGE("     0 | enable 11k\n");
        LOG_USAGE("     1 | disable 11k\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    val = (uint8_t)atoi(param[1]);

    ret = wifi_config_set_disable_11k(port, val);

    LOG_FUNC("wifi_config_set_disable_11k(port:%d), [%s], ret:%s, Code=%d\n",
             (int)port, param[1], WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_set_disable_11v_ex(uint8_t len, char *param[])
{
    int32_t ret;
    int32_t port;
    uint8_t val;

    if (len != 2) {
        LOG_USAGE("Usage: wifi config set disable_11v <port> <val>\n");
        LOG_USAGE("     0 | enable 11v\n");
        LOG_USAGE("     1 | disable 11v\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    val = (uint8_t)atoi(param[1]);
    if (val != 0 && val != 1) {
        LOG_FUNC("invalid value\n");
        return -1;
    }

    ret = wifi_config_set_disable_11v(port, val);

    LOG_FUNC("wifi_config_set_disable_11v(port:%d), [%s], ret:%s, Code=%d\n",
             (int)port, param[1], WIFI_CLI_RETURN_STRING(ret), (int)ret);
    return ret;
}

uint8_t wifi_config_get_disable_11k_ex(uint8_t len, char *param[])
{
    int32_t status;
    int32_t port;
    uint8_t uParam = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get disable_11k <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    status = wifi_config_get_disable_11k(port, &uParam);
    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, uParam);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_get_disable_11v_ex(uint8_t len, char *param[])
{
    int32_t status;
    int32_t port;
    uint8_t uParam = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config get disable_11v <port>\n");
        return -1;
    }

    port = port_sanity_check(param[0]);
    if (port < 0)
        return -1;

    status = wifi_config_get_disable_11v(port, &uParam);
    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, uParam);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
#endif /* #if CFG_SUPPORT_11KV_SWITCH */

#ifdef MTK_WIFI_PROFILE_ENABLE
uint8_t wifi_profile_set_opmode_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint8_t mode = atoi(param[0]);

    if (len < 1) {
        LOG_USAGE("Usage: wifi profile set opmode <mode>");
        return -1;
    }

    status = wifi_profile_set_opmode(mode);

    LOG_FUNC("save opmode = %d %s.", mode, status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_get_opmode_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint8_t mode;

    status = wifi_profile_get_opmode(&mode);

    LOG_FUNC("fetch opmode = %d %s.\n", status ? -1 : mode, status ? "fail" : "done");

    return status;
}

/**
* @brief Store SSID to the profile in the Flash memory.
* wifi profile set ssid <port> <ssid>
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [IN]ssid SSID
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_profile_set_ssid_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    char *ssid = NULL;
    char *section = (port == WIFI_PORT_STA ?
                     WIFI_PROFILE_BUFFER_STA : WIFI_PROFILE_BUFFER_AP);

    if (port < 0)
        return -1;

    if (len < 2) {
        LOG_USAGE("Usage: wifi profile set ssid <port> <ssid>");
        return -1;
    }

    ssid = param[1];
    status = wifi_profile_set_ssid((uint8_t)port,
                                   (uint8_t *)ssid, strlen(ssid));
    LOG_FUNC("[%s] save ssid = %s %s.", section, ssid, status ? "fail" : "done");

    return status;
}

/**
* @brief Get SSID from the profile in NVRAM.
* wifi profile get ssid <port>
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [OUT]ssid SSID
* @param [OUT]ssid_length Length of SSID
*
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_profile_get_ssid_ex(uint8_t length, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    uint8_t ssid[32] = {0};
    uint8_t len;

    if (port < 0)
        return -1;

    kalMemSet(ssid, 0x0, sizeof(ssid));
    status = wifi_profile_get_ssid((uint8_t)port, ssid, &len);

    if (!status)
        LOG_FUNC("fetch ssid = %s, len = %d done.\n", ssid, len);
    else
        LOG_FUNC("fetch ssid fail.\n");
    return status;
}

/**
* @brief Store WiFi Wireless Mode to the profile in the Flash memory.
* wifi profile set wirelessmode <port> <mode>
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [IN]mode
* @param 1 legacy 11B only
* @param 2 legacy 11A only
* @param 3 legacy 11A/B/G mixed
* @param 4 legacy 11G only
* @param 5 11ABGN mixed
* @param 6 11N only in 2.4G
* @param 7 11GN mixed
* @param 8 11AN mixed
* @param 9 11BGN mixed
* @param 10 11AGN mixed
*
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_profile_set_wireless_mode_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    wifi_phy_mode_t mode = (wifi_phy_mode_t)atoi(param[1]);
    char *section = (port == WIFI_PORT_STA ?
                     WIFI_PROFILE_BUFFER_STA : WIFI_PROFILE_BUFFER_AP);

    if (port < 0)
        return -1;

    if (len < 2) {
        LOG_USAGE("Usage: wifi profile set wirelessmode <port> <mode>");
        return -1;
    }

    status = wifi_profile_set_wireless_mode((uint8_t)port, mode);

    LOG_FUNC("[%s] save wireless mode =%d %s.", section, mode,
             status ? "fail" : "done");

    return status;
}

/**
* @brief Get WiFi Wireless Mode from the profile in NVRAM.
* wifi profile get wirelessmode <port>
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [OUT]mode
* @param 1 legacy 11B only
* @param 2 legacy 11A only
* @param 3 legacy 11A/B/G mixed
* @param 4 legacy 11G only
* @param 5 11ABGN mixed
* @param 6 11N only in 2.4G
* @param 7 11GN mixed
* @param 8 11AN mixed
* @param 9 11BGN mixed
* @param 10 11AGN mixed
*/
uint8_t wifi_profile_get_wireless_mode_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    uint8_t mode;

    if (port < 0)
        return -1;

    status = wifi_profile_get_wireless_mode((uint8_t)port,
                                            (wifi_phy_mode_t *)&mode);

    LOG_FUNC("fetch wireless mode =%d %s.\n", status ? -1 : mode,
             status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_set_security_mode_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    wifi_auth_mode_t auth = (wifi_auth_mode_t)atoi(param[1]);
    wifi_encrypt_type_t encryp = (wifi_encrypt_type_t)atoi(param[2]);
    char *section = (port == WIFI_PORT_STA ?
                     WIFI_PROFILE_BUFFER_STA : WIFI_PROFILE_BUFFER_AP);

    if (port < 0)
        return -1;

    if (len < 3) {
        LOG_USAGE("Usage: wifi profile set sec <port> <auth> <encrypt>");
        return -1;
    }

    status = wifi_profile_set_security_mode((uint8_t)port, auth, encryp);

    LOG_FUNC("[%s] save auth mode = %d, encrypt type = %d %s.",
             section, auth, encryp, status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_get_security_mode_ex(uint8_t length, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    uint8_t auth;
    uint8_t encryp;

    if (port < 0)
        return -1;

    status = wifi_profile_get_security_mode((uint8_t)port,
                                            (wifi_auth_mode_t *)&auth, (wifi_encrypt_type_t *)&encryp);

    LOG_FUNC("fetch auth mode = %d, encrypt type = %d %s.\n",
             status ? -1 : auth, status ? -1 : encryp, status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_set_psk_ex(uint8_t len, char *param[])
{
    int8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    char *password = NULL;
    char *section = (port == WIFI_PORT_STA ?
                     WIFI_PROFILE_BUFFER_STA : WIFI_PROFILE_BUFFER_AP);

    if (port < 0)
        return -1;

    if (len < 2) {
        LOG_USAGE("Usage: wifi profile set psk <port> <password>");
        return -1;
    }

    password = param[1];
    status = wifi_profile_set_wpa_psk_key((uint8_t)port,
                                          (uint8_t *)password, kalStrLen(password));

    LOG_FUNC("[%s] save password = %s len = %d %s.",
             section, password, kalStrLen(password), status ? "fail" : "done");

    return status < 0 ? -1 : 0;
}

uint8_t wifi_profile_get_psk_ex(uint8_t length, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    uint8_t password[65];
    uint8_t len;

    if (port < 0)
        return -1;

    kalMemSet(password, 0, sizeof(password));
    status = wifi_profile_get_wpa_psk_key((uint8_t)port,
                                          password, &len);

    LOG_FUNC("fetch password =%s len=%d %s.\n", password,
             status ? 0 : len, status ? "fail" : "done");

    return status;
}

/**
* @brief Store WiFi WEP Keys to the profile in NVRAM.
* wifi profile set wep <port> <key id> <key>
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [IN]wifi_wep_key_t
*
* @return  >=0 means success, <0 means fail
*/
uint8_t wifi_profile_set_wep_key_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint8_t index = 0;
    wifi_wep_key_t wep_key;
    char *ptr = NULL;
    int32_t port = port_sanity_check(param[0]);
    char *keys = param[2];
    char *section = (port == WIFI_PORT_STA ?
                     WIFI_PROFILE_BUFFER_STA : WIFI_PROFILE_BUFFER_AP);

    if (port < 0)
        return -1;

    kalMemSet(&wep_key, 0, sizeof(wep_key));
    wep_key.wep_tx_key_index = atoi(param[1]);

    index = 0;
    for (ptr = rstrtok((char *)keys, ","); (ptr);
         ptr = rstrtok(NULL, ",")) {
        LOG_FUNC("[%d] key=%s\n", index, ptr);
        if (kalStrLen(ptr) == 1 || kalStrLen(ptr) == 5 ||
            kalStrLen(ptr) == 13 || kalStrLen(ptr) == 10 ||
            kalStrLen(ptr) == 26) {
            kalMemCopy(wep_key.wep_key[index], ptr, kalStrLen(ptr));
            wep_key.wep_key_length[index] = kalStrLen(ptr);
        } else {
            LOG_FUNC("invalid length of value.\n");
        }
        index++;
        if (index >= WIFI_NUMBER_WEP_KEYS)
            break;
    }

    status = wifi_profile_set_wep_key((uint8_t)port, &wep_key);

    if (status) {
        wep_key.wep_key[0][wep_key.wep_key_length[0]] = '\0';
        wep_key.wep_key[1][wep_key.wep_key_length[1]] = '\0';
        wep_key.wep_key[2][wep_key.wep_key_length[2]] = '\0';
        wep_key.wep_key[3][wep_key.wep_key_length[3]] = '\0';
    }
    LOG_FUNC("[%s] save wep key =(%s, %s, %s, %s)"
             "key id=%d, len=(%d, %d, %d, %d) %s.\n",
             section,
             wep_key.wep_key[0],
             wep_key.wep_key[1],
             wep_key.wep_key[2],
             wep_key.wep_key[3],
             wep_key.wep_tx_key_index,
             wep_key.wep_key_length[0],
             wep_key.wep_key_length[1],
             wep_key.wep_key_length[2],
             wep_key.wep_key_length[3],
             status ? "fail" : "done");

    return status;
}

/**
* @brief Get WiFi WEP Keys from the profile in NVRAM.
* wifi profile get wep <port>
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [OUT]wifi_wep_key_t
*
* @return >=0 means success, <0 means fail
*/
uint8_t wifi_profile_get_wep_key_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    wifi_wep_key_t wep_key;
    int index, i;

    if (port < 0)
        return -1;

    kalMemSet(&wep_key, 0, sizeof(wep_key));
    status = wifi_profile_get_wep_key((uint8_t)port, &wep_key);

    wep_key.wep_key[0][wep_key.wep_key_length[0]] = '\0';
    wep_key.wep_key[1][wep_key.wep_key_length[1]] = '\0';
    wep_key.wep_key[2][wep_key.wep_key_length[2]] = '\0';
    wep_key.wep_key[3][wep_key.wep_key_length[3]] = '\0';

    if (!status) {
        LOG_FUNC("fetch wep key id =%d, len = (%d, %d, %d, %d) done.\n",
                 wep_key.wep_tx_key_index,
                 wep_key.wep_key_length[0],
                 wep_key.wep_key_length[1],
                 wep_key.wep_key_length[2],
                 wep_key.wep_key_length[3]);
        for (index = 0; index < WIFI_NUMBER_WEP_KEYS; index++) {
            LOG_FUNC("[%d]: ", index);
            for (i = 0; i < wep_key.wep_key_length[index]; i++)
                LOG_FUNC("%02x", wep_key.wep_key[index][i]);
            LOG_FUNC("\n");
        }
    } else
        LOG_FUNC("fetch wep key fail.\n");
    return status;
}

uint8_t wifi_profile_set_mac_address_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    uint8_t mac[6] = {0};
    /* Use STA MAC/IP as AP MAC/IP for the time being, due to N9 dual
    interface not ready yet */
    char *section = WIFI_PROFILE_BUFFER_STA;

    if (port < 0)
        return -1;

    if (len < 2) {
        LOG_USAGE("Usage: wifi profile set mac <port> <address>");
        return -1;
    }

    wifi_conf_get_mac_from_str((char *)mac, param[1]);

    status = wifi_profile_set_mac_address((uint8_t)port, mac);

    LOG_FUNC("[%s] save mac addr = %02x:%02x:%02x:%02x:%02x:%02x %s.",
             section, mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5], status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_get_mac_address_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    uint8_t mac[6] = {0};

    if (port < 0)
        return -1;

    status = wifi_profile_get_mac_address((uint8_t)port, mac);

    if (!status)
        LOG_FUNC("fetch mac addr = %02x:%02x:%02x:%02x:%02x:%02x done.\n",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    else
        LOG_FUNC("fetch mac addr fail.\n");
    return status;
}

/**
 * @brief Store channel to the profile in the Flash memory.
 * wifi profile set ch <port> <ch>
 * @param [IN]port
 * @param 0 STA / AP Client
 * @param 1 AP
 * @param [IN]channel    1~14 are supported for 2.4G only product.
 *
 * @return  >=0 means success, <0 means fail
 */
uint8_t wifi_profile_set_channel_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    uint8_t ch = atoi(param[1]);
    char *section = (port == WIFI_PORT_STA ?
                     WIFI_PROFILE_BUFFER_STA : WIFI_PROFILE_BUFFER_AP);

    if (port < 0)
        return -1;

    if (ch < 1) {
        LOG_FUNC("Invalid channel number\n");
        return -1;
    }

    status = wifi_profile_set_channel((uint8_t)port, ch);

    LOG_FUNC("[%s] save ch =%d %s.\n", section, ch, status ? "fail" : "done");

    return status;
}

/**
 * @brief Get channel from the profile in NVRAM.
 * wifi profile get ch <port>
 * @param [IN]port
 * @param 0 STA / AP Client
 * @param 1 AP
 * @param [OUT]channel    1~14 are supported for 2.4G only product.
 *
 * @return  >=0 means success, <0 means fail
 */
uint8_t wifi_profile_get_channel_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    uint8_t ch;

    if (port < 0)
        return -1;

    status = wifi_profile_get_channel((uint8_t)port, &ch);

    LOG_FUNC("fetch ch =%d %s.\n", status ? -1 : ch, status ? "fail" : "done");

    return status;
}

/**
 * @brief Store bandwidth to the profile in the Flash memory.
 * wifi profile set bw <port> <bw>
 * @param [IN]port
 * @param 0 STA / AP Client
 * @param 1 AP
 * @param [IN]bandwidth Bandwidth IOT_CMD_CBW_20MHZ, IOT_CMD_CBW_40MHZ,
 *                  IOT_CMD_CBW_2040MHZ are supported.
 * @return  >=0 means success, <0 means fail
 */
uint8_t wifi_profile_set_bandwidth_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    uint8_t bw = atoi(param[1]);
    char *section = (port == WIFI_PORT_STA ?
                     WIFI_PROFILE_BUFFER_STA : WIFI_PROFILE_BUFFER_AP);

    if (len < 2) {
        LOG_USAGE("Usage: wifi profile set bw <port> <bw>");
        return -1;
    }

    if (port < 0)
        return -1;

    status = wifi_profile_set_bandwidth((uint8_t)port, bw);

    LOG_FUNC("[%s] save bw =%d %s.\n", section, bw, status ? "fail" : "done");

    return status;
}

/**
 * @brief  Get bandwidth from the profile in NVRAM.
 * wifi profile get bw <port>
 * @param [IN]port
 * @param 0 STA / AP Client
 * @param 1 AP
 * @param [OUT]bandwidth The wirelss bandwidth.
 *                       IOT_CMD_CBW_20MHZ,
 *                       IOT_CMD_CBW_40MHZ, and
 *                       IOT_CMD_CBW_2040MHZ are supported.
 *
 * @return  >=0 means success, <0 means fail
 *
 * @note    Default value is HT_20
 */
uint8_t wifi_profile_get_bandwidth_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t port = port_sanity_check(param[0]);
    uint8_t bw;

    if (len < 1) {
        LOG_USAGE("Usage: wifi profile get bw <port>");
        return -1;
    }

    if (port < 0)
        return -1;

    status = wifi_profile_get_bandwidth((uint8_t)port, &bw);

    LOG_FUNC("fetch bw =%d %s.\n", status ? -1 : bw, status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_set_dtim_interval_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint8_t dtim = atoi(param[0]);

    status = wifi_profile_set_dtim_interval(dtim);

    LOG_FUNC("save dtim interval =%d %s.\n", dtim,
             status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_get_dtim_interval_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint8_t dtim;

    status = wifi_profile_get_dtim_interval(&dtim);

    LOG_FUNC("fetch dtim = %d %s.\n", status ? -1 : dtim,
             status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_set_listen_interval_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint8_t interval = atoi(param[0]);

    status = wifi_profile_set_listen_interval(interval);

    LOG_FUNC("save listen interval =%d %s.\n", interval,
             status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_get_listen_interval_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint8_t interval;

    status = wifi_profile_get_listen_interval(&interval);

    LOG_FUNC("fetch listen interval = %d %s.\n", status ? -1 : interval,
             status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_set_power_save_mode_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint8_t ps_mode = atoi(param[0]);

    status = wifi_profile_set_power_save_mode(ps_mode);

    LOG_FUNC("save PS mode =%d %s.\n", ps_mode, status ? "fail" : "done");

    return status;
}

uint8_t wifi_profile_get_power_save_mode_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint8_t ps_mode;

    status = wifi_profile_get_power_save_mode(&ps_mode);

    LOG_FUNC("fetch PS mode = %d %s.\n", status ? -1 : ps_mode,
             status ? "fail" : "done");

    return status;
}

#if IP_NAPT
uint8_t wifi_profile_set_napt_tcp_entry_num_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint32_t num = atoi(param[0]);

    if (num > WIFI_MAX_NUMBER_OF_NAPT_ENTRY) {
        LOG_FUNC("num must be less than or eqaul to 255");
        return -1;
    }
    status = wifi_profile_set_napt_tcp_entry_num((uint8_t)num);

    LOG_FUNC("save num of tcp entries = %d %s.", num,
             status ? "fail" : "done");

    return status;

}

uint8_t wifi_profile_set_napt_udp_entry_num_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    uint32_t num = atoi(param[0]);

    if (num > WIFI_MAX_NUMBER_OF_NAPT_ENTRY) {
        LOG_FUNC("num must be less than or eqaul to 255");
        return -1;
    }

    status = wifi_profile_set_napt_udp_entry_num((uint8_t)num);

    LOG_FUNC("save num of udp entries = %d %s.", num,
             status ? "fail" : "done");

    return status;

}
#endif /* #if IP_NAPT */

#endif /* #ifdef MTK_WIFI_PROFILE_ENABLE */

#if CFG_SUPPORT_ANT_DIV
uint8_t wifi_config_set_antdiv_mode_ex(uint8_t len, char *param[])
{
    int32_t status = 0;
    uint8_t mode = 0;

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set antmode <value>\n");
        return -1;
    }

    mode = atoi(param[0]);
    status = wifi_config_set_antdiv_mode(mode);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, mode);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_get_antdiv_mode_ex(uint8_t len, char *param[])
{
    int32_t status = 0;
    uint8_t mode = 0;

    status = wifi_config_get_antdiv_mode(&mode);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, mode);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}

uint8_t wifi_config_get_antdiv_cur_idx_ex(uint8_t len, char *param[])
{
    int32_t status = 0;
    uint8_t index = 0;

    status = wifi_config_get_antdiv_cur_idx(&index);

    if (!status)
        LOG_FUNC("%s = %d done.\n", __func__, index);
    else
        LOG_FUNC("%s fail.\n", __func__);

    return status;
}
#endif /* #if CFG_SUPPORT_ANT_DIV */

#if CFG_SUPPORT_SCAN_CH_TIME
uint8_t wifi_config_set_dwll_time_ex(uint8_t len,
                                     char *param[])
{
    int32_t status = 0;
    uint8_t scan_option = 0;
    uint8_t dwell_time = 0;

    scan_option = (uint8_t)atoi(param[0]);
    dwell_time = (uint8_t)atoi(param[1]);

    if (len < 2) {
        LOG_USAGE("Usage: wifi config set scnChTime <0:active,1:passive> <dwell>\n");
        return -1;
    }

    status = wifi_config_set_dwll_time(scan_option, dwell_time);

    if (!status)
        LOG_FUNC("%s : Set scan type[%d] Dwell time[%d] done.\n",
                 __func__, scan_option, dwell_time);
    else
        LOG_FUNC("%s: Set scan type[%d] Dwell time[%d] fail.\n",
                 __func__, scan_option, dwell_time);

    return status;
}

uint8_t wifi_config_set_DFS_dwll_time_ex(uint8_t len,
                                         char *param[])
{
    int32_t status = 0;
    uint8_t scan_option = 0;

    scan_option = (uint8_t)atoi(param[0]);

    if (len < 1) {
        LOG_USAGE("Usage: wifi config set scnDfsChTime <0:active,1:passive>\n");
        return -1;
    }

    status = wifi_config_set_DFS_dwll_time(scan_option);

    if (!status)
        LOG_FUNC("%s : Set DFS dwell time follow scan type[%d] done.\n",
                 __func__, scan_option);
    else
        LOG_FUNC("%s: Set DFS dwell time follow scan type[%d] fail.\n",
                 __func__, scan_option);

    return status;
}
#endif /* #if CFG_SUPPORT_SCAN_CH_TIME */

#endif /* #if defined(MTK_MINICLI_ENABLE) */
