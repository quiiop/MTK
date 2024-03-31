/******************************************************************************
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
 *****************************************************************************/
/*
** Id: @(#) gl_cfg80211.c@@
*/

/*! \file   gl_cfg80211.c
*    \brief  Main routines for supporintg MT6620 cfg80211 control interface
*
*    This file contains the support routines of Linux driver for MediaTek Inc. 802.11
*    Wireless LAN Adapters.
*/


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "precomp.h"
#include "gl_upperlayer.h"
#include "mtk_wireless.h"
#include "gl_init.h"
#ifdef MTK_MINISUPP_ENABLE
#include "wifi_api_ex.h"
#endif /* #ifdef MTK_MINISUPP_ENABLE */

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
/*For mini supplicant*/
#if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P)
SemaphoreHandle_t g_wait_drv_ready;
#endif /* #if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P) */
#if CFG_PSRAM_ENABLE
ATTR_ZIDATA_IN_RAM
#endif /* #if CFG_PSRAM_ENABLE */
struct wpa_scan_results *results;
#if CFG_PSRAM_ENABLE
ATTR_ZIDATA_IN_RAM
#endif /* #if CFG_PSRAM_ENABLE */
struct wpa_scan_res *scan_res[CFG_MAX_NUM_BSS_LIST];

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
const uint8_t bcast_addr[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

#if !CFG_SUPPORT_NO_SUPPLICANT_OPS /*For mini supplicant*/
int mtk_freertos_wpa_connect(struct wpa_driver_associate_params *sme);
#endif /* #if !CFG_SUPPORT_NO_SUPPLICANT_OPS (For mini supplicant) */
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
int mtk_freertos_wpa_get_ssid(uint8_t *ssid, uint32_t *ssid_len)
{
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
    struct ADAPTER *prAdapter = NULL;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex = 0;
    int ret = -1;

    prAdapter = prGlueInfo->prAdapter;

    ucBssIndex = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

    if (prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
        *ssid_len = prBssInfo->ucSSIDLen;
        kalMemCopy(ssid, prBssInfo->aucSSID, prBssInfo->ucSSIDLen);
        LOG_FUNC("Ssid: %s\r\n", prBssInfo->aucSSID);
        ret = 0;
    }

    return ret;
}

/* get connected bssid */
int mtk_freertos_wpa_get_bssid(uint8_t *bssid)
{
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
    struct ADAPTER *prAdapter = NULL;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex = 0;
    int ret = -1;

    prAdapter = prGlueInfo->prAdapter;

    ucBssIndex = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

    if (prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
        kalMemCopy(bssid, prBssInfo->aucBSSID, MAC_ADDR_LEN);
        LOG_FUNC("Ssid: %s\r\n", prBssInfo->aucSSID);
        LOG_FUNC("BssId: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                 prBssInfo->aucBSSID[0],
                 prBssInfo->aucBSSID[1],
                 prBssInfo->aucBSSID[2],
                 prBssInfo->aucBSSID[3],
                 prBssInfo->aucBSSID[4],
                 prBssInfo->aucBSSID[5]);
        LOG_FUNC("BssIndex: %u\r\n", ucBssIndex);
        ret = 0;
    }

    return ret;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to connect to
 *        the ESS with the specified parameters
 *
 * @param
 *  sme -> connect paramter
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_freertos_connect(uint16_t center_freq, uint8_t *bssid, uint8_t *ssid, uint32_t ssid_len)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen;
    enum ENUM_WEP_STATUS eEncStatus;
    enum ENUM_PARAM_AUTH_MODE eAuthMode;
    struct PARAM_CONNECT rNewSsid;
#if (CFG_SUPPORT_WPA3 == 0)
    uint8_t fgCarryWPSIE = FALSE;
    struct CONNECTION_SETTINGS *prConnSettings;
#endif /* #if (CFG_SUPPORT_WPA3 == 0) */
    struct PARAM_OP_MODE rOpMode;
    uint32_t i, u4AkmSuite = 0;
    struct DOT11_RSNA_CONFIG_AUTHENTICATION_SUITES_ENTRY *prEntry;
#if CFG_SUPPORT_REPLAY_DETECTION
    struct GL_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;
#endif /* #if CFG_SUPPORT_REPLAY_DETECTION */

    struct ieee80211_channel ch_info;
    struct cfg80211_connect_params sme_static;
    struct cfg80211_connect_params *sme = &sme_static;

    sme->channel = &ch_info;
    sme->channel->center_freq = center_freq;
    memcpy(sme->ssid, ssid, ssid_len);
    memcpy(sme->bssid, bssid, ETH_ALEN);
    sme->ssid[ssid_len] = '\0';
    sme->ssid_len = ssid_len;

    DBGLOG(REQ, LOUD, "> freq %d bssid [%02x:%02x:%02x:%02x:%02x:%02x] ssid %s ssid_len %d\n",
           center_freq, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
           sme->ssid, ssid_len);
    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    /* printk("[wlan]mtk_cfg80211_connect\n"); */
    if ((prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].eOPMode) > NET_TYPE_AUTO_SWITCH)
        rOpMode.eOpMode = NET_TYPE_AUTO_SWITCH;
    else
        rOpMode.eOpMode =
            prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].eOPMode;

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSetInfrastructureMode,
                       &rOpMode, sizeof(rOpMode),
                       FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(INIT, INFO, "wlanoidSetInfrastructureMode fail 0x%lx\n", rStatus);
        return -14;/* EFAULT; */
    }

    /* after set operation mode, key table are cleared */

    /* reset wpa info */
    prGlueInfo->rWpaInfo[0].u4WpaVersion = (uint32_t)IW_AUTH_WPA_VERSION_DISABLED;
    prGlueInfo->rWpaInfo[0].u4KeyMgmt = 0;
    prGlueInfo->rWpaInfo[0].u4CipherGroup = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo[0].u4CipherPairwise = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo[0].u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;

#if CFG_SUPPORT_REPLAY_DETECTION
    /* reset Detect replay information */
    prDetRplyInfo = aisGetDetRplyInfo(prGlueInfo->prAdapter, prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex);
    kalMemZero(prDetRplyInfo, sizeof(struct GL_DETECT_REPLAY_INFO));
#endif /* #if CFG_SUPPORT_REPLAY_DETECTION */

#if CFG_SUPPORT_802_11W
    DBGLOG(INIT, ERROR, "> MFP not supported\n");
#endif /* #if CFG_SUPPORT_802_11W */

    DBGLOG(INIT, ERROR, "> WPA not supported\n");
    prGlueInfo->rWpaInfo[0].u4WpaVersion = IW_AUTH_WPA_VERSION_DISABLED;

    DBGLOG(INIT, ERROR, "> SECUERITY not supported defalut OPEN\n");
    prGlueInfo->rWpaInfo[0].u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;

    if (prGlueInfo->rWpaInfo[0].u4WpaVersion == IW_AUTH_WPA_VERSION_DISABLED) {
        eAuthMode = (prGlueInfo->rWpaInfo[0].u4AuthAlg == IW_AUTH_ALG_OPEN_SYSTEM) ?
                    AUTH_MODE_OPEN : AUTH_MODE_AUTO_SWITCH;
    }

    prGlueInfo->rWpaInfo[0].fgPrivacyInvoke = sme->privacy;
    //prGlueInfo->fgWpsActive = FALSE;

#if CFG_SUPPORT_PASSPOINT
    prGlueInfo->fgConnectHS20AP = FALSE;
#endif /* #if CFG_SUPPORT_PASSPOINT */

    DBGLOG(SEC, WARN, "IE from upperlayer not supported\n");

#if (CFG_SUPPORT_WPA3 == 0)
    /* clear WSC Assoc IE buffer in case WPS IE is not detected */
    prConnSettings =
        aisGetConnSettings(prGlueInfo->prAdapter, prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex);
    if (fgCarryWPSIE == FALSE) {
        kalMemZero(&prConnSettings->aucWSCAssocInfoIE, 200);
        prConnSettings->u2WSCAssocInfoIELen = 0;
    }
#endif /* #if (CFG_SUPPORT_WPA3 == 0) */

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSetAuthMode, &eAuthMode, sizeof(eAuthMode), FALSE, FALSE, FALSE, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(REQ, WARN, "set auth mode error:%lx\n", rStatus);

    /* Enable the specific AKM suite only. */
    for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++) {
        prEntry = &prGlueInfo->prAdapter->rMib[0].dot11RSNAConfigAuthenticationSuitesTable[i];

        if (prEntry->dot11RSNAConfigAuthenticationSuite == u4AkmSuite) {
            prEntry->dot11RSNAConfigAuthenticationSuiteEnabled = TRUE;
            /* printk("match AuthenticationSuite = 0x%x", u4AkmSuite); */
        } else {
            prEntry->dot11RSNAConfigAuthenticationSuiteEnabled = FALSE;
        }
    }

    DBGLOG(REQ, WARN, "disable encryption\n");
    eEncStatus = ENUM_ENCRYPTION_DISABLED;

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSetEncryptionStatus, &eEncStatus, sizeof(eEncStatus), FALSE, FALSE, FALSE, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(REQ, WARN, "set encryption mode error:%lx\n", rStatus);

    DBGLOG(REQ, WARN, "Security not supported (no key)\n");

    rNewSsid.u4CenterFreq = sme->channel->center_freq;
    rNewSsid.pucBssid = (uint8_t *)sme->bssid;
    rNewSsid.pucSsid = (uint8_t *)sme->ssid;
    rNewSsid.u4SsidLen = sme->ssid_len;
    rNewSsid.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;

    rStatus = kalIoctl(prGlueInfo, wlanoidSetConnect, (void *)&rNewSsid, sizeof(struct PARAM_CONNECT),
                       FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set SSID:%x\n", rStatus);
        return -22;/* -EINVAL; */
    }

    DBGLOG(INIT, INFO, "> exit\n");
    return 0;
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to do a scan
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_freertos_scan(uint16_t *ch_list, int num_ch)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus;
    int i = 0;
    uint32_t u4BufLen;
    struct PARAM_SCAN_REQUEST_ADV *rScanRequest;

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);
    /* check if there is any pending scan/sched_scan not yet finished */
    if (prGlueInfo->prScanRequest != NULL)
        return -EBUSY;

    rScanRequest = vmalloc(sizeof(*rScanRequest));
    if (!rScanRequest)
        return -ENOMEM;

    kalMemZero((uint8_t *)rScanRequest, sizeof(*rScanRequest));

    DBGLOG(REQ, WARN, "scan channel num = %d\n", num_ch);

    if (num_ch > MAXIMUM_OPERATION_CHANNEL_LIST) {
        DBGLOG(REQ, WARN, "scan channel num (%d) exceeds %d, do a full scan instead\n",
               num_ch, MAXIMUM_OPERATION_CHANNEL_LIST);
        rScanRequest->u4ChannelNum = 0;
    } else if (num_ch > 0) {
        rScanRequest->u4ChannelNum = num_ch;
        for (i = 0; i < num_ch; i++) {
            rScanRequest->arChannel[i].eBand = (ch_list[i] < 13) ? BAND_2G4 : BAND_5G;
            rScanRequest->arChannel[i].u4CenterFreq1 = (ch_list[i] < 13) ? (2412 + (ch_list[i] - 1) * 5) : (5180 + (ch_list[i] - 1) * 5);
            rScanRequest->arChannel[i].u4CenterFreq2 = 0;
            rScanRequest->arChannel[i].u2PriChnlFreq = (ch_list[i] < 13) ? (2412 + (ch_list[i] - 1) * 5) : (5180 + (ch_list[i] - 1) * 5);
            rScanRequest->arChannel[i].ucChannelNum = ch_list[i];
            DBGLOG(REQ, WARN, "band %d add ch %d @ freq %d\n",
                   rScanRequest->arChannel[i].eBand,
                   rScanRequest->arChannel[i].ucChannelNum,
                   rScanRequest->arChannel[i].u4CenterFreq1);
        }
    }

    /* 2018/04/18 frog: The point should be ready before doing IOCTL. */
    prGlueInfo->prScanRequest = rScanRequest;

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSetBssidListScanAdv,
                       rScanRequest, sizeof(struct PARAM_SCAN_REQUEST_ADV), FALSE, FALSE, FALSE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "scan error:%lx\n", rStatus);
        /* 2018/04/18 frog: Remove pointer if IOCTL fail. */
        /* prGlueInfo->prScanRequest = NULL; */
        return -EINVAL;
    }

    return 0;
}

WLAN_ATTR_TEXT_IN_MEM_TX
err_t mtk_freertos_wlan_send(struct netif *netif, struct pbuf *p)
{
    return mtk_wlan_tx(p, netif);
}

int mtk_freertos_disconnect(struct GLUE_INFO *prGlueInfo, uint16_t reason_code)
{
    /* P_GLUE_INFO_T prGlueInfo = NULL; */
    uint32_t rStatus;
    uint32_t u4BufLen;

    DBGLOG(REQ, WARN, "reason code[%d]\n", reason_code);
    if (!prGlueInfo)
        prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetDisassociate, NULL, 0, FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "disassociate error:%lx\n", rStatus);
        return -EFAULT;
    }

    return 0;
}

/*For mini supplicant*/
#if !CFG_SUPPORT_NO_SUPPLICANT_OPS
/* check including dfs channel or not */
int mtk_freertos_wpa_get_channel_list_full(
    uint8_t ucSpecificBand,
    uint8_t ucMaxChannelNum,
    uint8_t ucNoDfs,
    uint8_t *pucNumOfChannel,
    uint8_t *paucChannelList)
{
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
    uint32_t i;
    struct RF_CHANNEL_INFO *aucChannelList;

    aucChannelList = (struct RF_CHANNEL_INFO *) kalMemAlloc(
                         sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM,
                         VIR_MEM_TYPE);
    if (!aucChannelList) {
        DBGLOG(P2P, ERROR,
               "Allocate buffer for channel list fail\n");
        return -ENOMEM;
    }

    rlmDomainGetChnlList(prGlueInfo->prAdapter, ucSpecificBand,
                         ucNoDfs, ucMaxChannelNum, pucNumOfChannel, aucChannelList);

    for (i = 0; i < *pucNumOfChannel; i++) {
        *(paucChannelList + i) = aucChannelList[i].ucChannelNum;
    }

    kalMemFree(aucChannelList, VIR_MEM_TYPE,
               sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM);

    return 0;
}

int mtk_freertos_wpa_set_favor_ssid(void *priv, uint8_t *ssid, uint8_t len)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    struct ADAPTER *prAdapter = NULL;
    struct CONNECTION_SETTINGS *prConnSettings;
    uint8_t ucBssIndex = 0;

    LOG_FUNC("set favor ssid(%u): %s\r\n", len, ssid);
    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    prAdapter = prGlueInfo->prAdapter;

    ucBssIndex = prAdapter->prAisBssInfo[0]->ucBssIndex;
    prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

    kalMemCopy(prConnSettings->aucSSID, ssid, len);
    prConnSettings->ucSSIDLen = len;

    return 0;
}

#ifdef MTK_MINISUPP_ENABLE
int mtk_freertos_wpa_scan(void *priv,
                          struct wpa_driver_scan_params *request)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus;
    uint32_t i, u4BufLen;
    struct PARAM_SCAN_REQUEST_ADV rScanRequest;

    if (!(g_prGlueInfo->u4ReadyFlag)) {
        DBGLOG(REQ, ERROR, "wlan is halt, skip\n");
        return WLAN_STATUS_FAILURE;
    }

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);
    kalMemZero(&rScanRequest, sizeof(rScanRequest));

    /* check if there is any pending scan/sched_scan not yet finished */
    if (prGlueInfo->prScanRequest != NULL)
        return -EBUSY;

    DBGLOG(REQ, INFO, "%s num_ssids %d\r\n", __func__, request->num_ssids);
    if (request->num_ssids == 0) {
        rScanRequest.u4SsidNum = 0;
    } else if (request->num_ssids <= SCN_SSID_MAX_NUM) {
        rScanRequest.u4SsidNum = request->num_ssids;

        for (i = 0; i < request->num_ssids; i++) {
            COPY_SSID(rScanRequest.rSsid[i].aucSsid,
                      rScanRequest.rSsid[i].u4SsidLen,
                      request->ssids[i].ssid,
                      request->ssids[i].ssid_len);
        }
    } else {
        return -EINVAL;
    }

    rScanRequest.u4IELength = request->extra_ies_len;
    if (request->extra_ies_len > 0)
        rScanRequest.pucIE = (u8 *)(request->extra_ies);

    if (!request->freqs) {
        DBGLOG(REQ, STATE, "scan channel num (0), do a full scan\n");
        rScanRequest.u4ChannelNum = 0;

    } else {
#if CFG_SCAN_CHANNEL_SPECIFIED
        uint32_t n_channels = 0;
        int channel_num;

        for (i = 0; request->freqs[i] != 0; i++)
            n_channels++;

        if (n_channels > MAXIMUM_OPERATION_CHANNEL_LIST) {
            DBGLOG(REQ, STATE,
                   "scan ch num(%d) exceeds %d, do a full scan\n",
                   n_channels, MAXIMUM_OPERATION_CHANNEL_LIST);
            rScanRequest.u4ChannelNum = 0;
        } else {
            scanRemoveBssDescsByPolicy(prGlueInfo->prAdapter, SCN_RM_POLICY_ENTIRE);
            rScanRequest.u4ChannelNum = n_channels;
            for (i = 0; i < n_channels; i++) {
                rScanRequest.arChannel[i].u4CenterFreq1 =
                    request->freqs[i];
                rScanRequest.arChannel[i].u4CenterFreq2 =
                    0;
                rScanRequest.arChannel[i].u2PriChnlFreq =
                    request->freqs[i];
                channel_num =
                    nicFreq2ChannelNum((request->freqs[i]) * 1000);
                rScanRequest.arChannel[i].ucChannelNum =
                    channel_num;
                rScanRequest.arChannel[i].eBand =
                    (channel_num < 13) ? BAND_2G4 : BAND_5G;
                DBGLOG(REQ, STATE,
                       "scan freq = %d ch num = %d\n",
                       request->freqs[i], channel_num);
            }
        }
#endif /* #if CFG_SCAN_CHANNEL_SPECIFIED */
    }

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSetBssidListScanAdv,
                       &rScanRequest, sizeof(struct PARAM_SCAN_REQUEST_ADV),
                       FALSE, FALSE, FALSE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "scan error:%lx\n", rStatus);
        return -EINVAL;
    }

    /* prGlueInfo->prScanRequest = request; */

    return 0;
}

int mtk_freertos_wpa_associate(void *priv,
                               struct wpa_driver_associate_params *req)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint8_t arBssid[PARAM_MAC_ADDR_LEN];
#if CFG_SUPPORT_PASSPOINT
    uint8_t *prDesiredIE = NULL;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
#endif /* #if CFG_SUPPORT_PASSPOINT */
    uint32_t u4BufLen;
    int ret;

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    if (req->mode == IEEE80211_MODE_AP) {
        DBGLOG(REQ, WARN, "need fix AP mode\n");
        return WLAN_STATUS_FAILURE;
        /*return wpa_driver_nl80211_ap(drv, params);*/
    }

    if (req->mode == IEEE80211_MODE_IBSS) {
        DBGLOG(REQ, WARN, "need fix IBSS mode\n");
        return WLAN_STATUS_FAILURE;
        /*return wpa_driver_nl80211_ibss(drv, params);*/
    }

    /*if (!msg)*/
    /*  return -1;*/

    kalMemZero(arBssid, PARAM_MAC_ADDR_LEN);
    wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid,
                         &arBssid[0], sizeof(arBssid), &u4BufLen);

    DBGLOG(REQ, INFO,
           "prefer bssid: [%02x:%02x:%02x:%02x:%02x:%02x]\n",
           req->bssid[0], req->bssid[1], req->bssid[2], req->bssid[3],
           req->bssid[4], req->bssid[5]);


    if (req->wpa_ie && req->wpa_ie_len > 0) {
#if CFG_SUPPORT_PASSPOINT
        if (wextSrchDesiredHS20IE((uint8_t *) req->ie, req->ie_len,
                                  (uint8_t **) &prDesiredIE)) {

            rStatus = kalIoctl(prGlueInfo,
                               wlanoidSetHS20Info,
                               prDesiredIE, IE_SIZE(prDesiredIE),
                               FALSE, FALSE, TRUE, &u4BufLen);
            if (rStatus != WLAN_STATUS_SUCCESS) {
                /* DBGLOG(REQ, TRACE,
                 *  ("[HS20] set HS20 assoc info error:%lx\n",
                 * rStatus));
                 */
            }
        }
#if (CFG_SUPPORT_WPA3 == 0)
        if (wextSrchDesiredInterworkingIE((PUINT_8) req->ie,
                                          req->ie_len, (uint8_t **) &prDesiredIE)) {

            rStatus = kalIoctl(prGlueInfo,
                               wlanoidSetInterworkingInfo,
                               prDesiredIE, IE_SIZE(prDesiredIE),
                               FALSE, FALSE, TRUE, &u4BufLen);
            if (rStatus != WLAN_STATUS_SUCCESS) {
                /* DBGLOG(REQ, TRACE,
                 *  ("[HS20] set %s error:%lx\n",
                 *  "Interworking assoc info", rStatus));
                 */
            }
        }
        if (wextSrchDesiredRoamingConsortiumIE((uint8_t *) req->ie,
                                               req->ie_len, (uint8_t **) &prDesiredIE)) {
            rStatus = kalIoctl(prGlueInfo,
                               wlanoidSetRoamingConsortiumIEInfo,
                               prDesiredIE, IE_SIZE(prDesiredIE),
                               FALSE, FALSE, TRUE, &u4BufLen);
            if (rStatus != WLAN_STATUS_SUCCESS) {
                /* DBGLOG(REQ, TRACE,
                 *  ("[HS20] set %s error:%lx\n",
                 *  "RoamingConsortium assoc info", rStatus));
                 */
            }
        }
#endif /* #if (CFG_SUPPORT_WPA3 == 0) */
#endif /* #if CFG_SUPPORT_PASSPOINT */
    }

    ret = mtk_freertos_wpa_connect(req);
    if (ret)
        return WLAN_STATUS_FAILURE;

    return 0;
}

int mtk_freertos_wpa_set_power(uint8_t listen_int_present, uint8_t listen_int,
                               uint8_t ps_mode_present, uint8_t ps_mode)
{
    int ret;

    if (listen_int_present) {
        ret = wifi_config_set_listen_interval(listen_int);
        if (ret) {
            DBGLOG(REQ, ERROR, "Set listen from nvdm fail\n");
            return -1;
        }
    }

    if (ps_mode_present) {
        ret = wifi_config_set_power_save_mode(ps_mode);
        if (ret) {
            DBGLOG(REQ, ERROR, "Set ps mode from nvdm fail\n");
            return -1;
        }
    }

    return 0;
}

#endif /* #ifdef MTK_MINISUPP_ENABLE */

#if CFG_SUPPORT_WPS
/*----------------------------------------------------------------------------*/
/*!
* \brief Find the desired WPS Information Element according to desiredElemID.
*
* \param[in] pucIEStart IE starting address.
* \param[in] i4TotalIeLen Total length of all the IE.
* \param[in] ucDesiredElemId Desired element ID.
* \param[out] ppucDesiredIE Pointer to the desired IE.
*
* \retval TRUE Find the desired IE.
* \retval FALSE Desired IE not found.
*
* \note
*/
/*----------------------------------------------------------------------------*/
bool wextSrchDesiredWPSIE(IN uint8_t *pucIEStart,
                          IN int32_t i4TotalIeLen, IN uint8_t  ucDesiredElemId,
                          OUT uint8_t **ppucDesiredIE)
{
    int32_t i4InfoElemLen;

    ASSERT(pucIEStart);
    ASSERT(ppucDesiredIE);

    while (i4TotalIeLen >= 2) {
        i4InfoElemLen = (int32_t) pucIEStart[1] + 2;

        if (pucIEStart[0] == ucDesiredElemId &&
            i4InfoElemLen <= i4TotalIeLen) {
            if (ucDesiredElemId != 0xDD) {
                /* Non 0xDD, OK! */
                *ppucDesiredIE = &pucIEStart[0];
                return TRUE;
            }
            /* EID == 0xDD, check WPS IE */
            if (pucIEStart[1] >= 4) {
                if (memcmp(&pucIEStart[2],
                           "\x00\x50\xf2\x04", 4) == 0) {
                    *ppucDesiredIE = &pucIEStart[0];
                    return TRUE;
                }
            }   /* check WPS IE length */
            /* check EID == 0xDD */
        }

        /* check desired EID */
        /* Select next information element. */
        i4TotalIeLen -= i4InfoElemLen;
        pucIEStart += i4InfoElemLen;
    }

    return FALSE;
} /* parseSearchDesiredWPSIE */
#endif /* #if CFG_SUPPORT_WPS */
/*----------------------------------------------------------------------------*/
/*!
* \brief Find the desired WPA/RSN Information Element
*  according to desiredElemID.
*
* \param[in] pucIEStart IE starting address.
* \param[in] i4TotalIeLen Total length of all the IE.
* \param[in] ucDesiredElemId Desired element ID.
* \param[out] ppucDesiredIE Pointer to the desired IE.
*
* \retval TRUE Find the desired IE.
* \retval FALSE Desired IE not found.
*
* \note
*/
/*----------------------------------------------------------------------------*/
bool wextSrchDesiredWPAIE(IN uint8_t *pucIEStart, IN int32_t i4TotalIeLen,
                          IN uint8_t  ucDesiredElemId, OUT uint8_t **ppucDesiredIE)
{
    int32_t i4InfoElemLen;

    ASSERT(pucIEStart);
    ASSERT(ppucDesiredIE);

    while (i4TotalIeLen >= 2) {
        i4InfoElemLen = (int32_t) pucIEStart[1] + 2;

        if (pucIEStart[0] == ucDesiredElemId &&
            i4InfoElemLen <= i4TotalIeLen) {
            if (ucDesiredElemId != 0xDD) {
                /* Non 0xDD, OK! */
                *ppucDesiredIE = &pucIEStart[0];
                return TRUE;
            } /* EID == 0xDD, check WPA IE */
            if (pucIEStart[1] >= 4) {
                if (memcmp(&pucIEStart[2],
                           "\x00\x50\xf2\x01", 4) == 0) {
                    *ppucDesiredIE = &pucIEStart[0];
                    return TRUE;
                }
            }   /* check WPA IE length */
            /* check EID == 0xDD */
        }

        /* check desired EID */
        /* Select next information element. */
        i4TotalIeLen -= i4InfoElemLen;
        pucIEStart += i4InfoElemLen;
    }

    return FALSE;
} /* parseSearchDesiredWPAIE */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for reporting scan results
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
struct wpa_scan_results *
mtk_freertos_wpa_scan_results(void *priv, int *num)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    struct SCAN_INFO *prScanInfo;
    struct LINK *prBSSDescList;
    struct BSS_DESC *prBSSDescNext;
    struct BSS_DESC *prBssDesc;
    uint8_t *ptr = NULL;
    uint8_t cnt;
    uint32_t i;
    uint8_t ucMixedSec = 0;
    uint8_t ucMixedEncr = 0;

    if (!(g_prGlueInfo->u4ReadyFlag)) {
        DBGLOG(REQ, ERROR, "wlan is halt, skip\n");
        return NULL;
    }

    *num = CFG_MAX_NUM_BSS_LIST;
    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    if (results) {
        for (int j = 0; j < CFG_MAX_NUM_BSS_LIST; ++j)
            kalMemZero(scan_res[j], sizeof(struct wpa_scan_res) +
                       CFG_IE_BUFFER_SIZE);
        results->num = 0;
        /* Reset fetch time to 0 so that wpa_supplicant will
         * update after fetching.
         */
        kalMemZero(&results->fetch_time, sizeof(results->fetch_time));
    } else {
#if CFG_PSRAM_ENABLE
        results = malloc(sizeof(*results));

        if (!results)
            return results;
        kalMemZero(results, sizeof(*results));

        results->res = malloc(sizeof(*results->res) * (*num));
        if (!results->res) {
            free(results);
            return NULL;
        }
        kalMemZero(results->res, sizeof(*results->res) * (*num));

        for (int j = 0; j < CFG_MAX_NUM_BSS_LIST; ++j) {
            scan_res[j] = malloc(sizeof(struct wpa_scan_res) +
                                 CFG_IE_BUFFER_SIZE);

            if (!scan_res[results->num])
                break;
            kalMemZero(scan_res[j], sizeof(struct wpa_scan_res) +
                       CFG_IE_BUFFER_SIZE);
        }
#else
        results = vmalloc(sizeof(*results));

        if (!results)
            return results;
        kalMemZero(results, sizeof(*results));

        results->res = vmalloc(sizeof(*results->res) * (*num));
        if (!results->res) {
            vfree(results);
            return NULL;
        }
        kalMemZero(results->res, sizeof(*results->res) * (*num));

        for (int j = 0; j < CFG_MAX_NUM_BSS_LIST; ++j) {
            scan_res[j] = vmalloc(sizeof(struct wpa_scan_res) +
                                 CFG_IE_BUFFER_SIZE);

            if (!scan_res[results->num])
                break;
            kalMemZero(scan_res[j], sizeof(struct wpa_scan_res) +
                       CFG_IE_BUFFER_SIZE);
        }

#endif
    }

    prScanInfo = &(prGlueInfo->prAdapter->rWifiVar.rScanInfo);
    prBSSDescList = &prScanInfo->rBSSDescList;

    LINK_FOR_EACH_ENTRY_SAFE(prBssDesc, prBSSDescNext,
                             prBSSDescList, rLinkEntry, struct BSS_DESC) {
        if (!prBssDesc || (results->num >= g_scan_list_size))
            break;

        results->res[results->num] = scan_res[results->num];
        results->num += 1;
        cnt = results->num - 1;

        kalMemCopy(scan_res[cnt]->bssid, prBssDesc->aucBSSID,
                   MAC_ADDR_LEN);
        if (prBssDesc->ucChannelNum <= 13)
            scan_res[cnt]->freq = prBssDesc->ucChannelNum * 5
                                  + 2407;
        else if (prBssDesc->ucChannelNum == 14)
            scan_res[cnt]->freq = 2484;
        else
            scan_res[cnt]->freq = prBssDesc->ucChannelNum * 5
                                  + 5000;
        scan_res[cnt]->beacon_int = prBssDesc->u2BeaconInterval;
        scan_res[cnt]->caps = prBssDesc->u2CapInfo;
        scan_res[cnt]->level = RCPI_TO_dBm(prBssDesc->ucRCPI);
        /* scan_res->tsf = beacon.Timestamp.QuadPart; */
        scan_res[cnt]->beacon_ie_len = 0;
        if (prBssDesc->u2IELength >= CFG_IE_BUFFER_SIZE)
            scan_res[cnt]->ie_len = CFG_IE_BUFFER_SIZE;
        else
            scan_res[cnt]->ie_len = prBssDesc->u2IELength;
        if ((prBssDesc->u2CapInfo & CAP_INFO_PRIVACY) == 0)
            scan_res[cnt]->auth_mode = AUTH_MODE_OPEN;
        else {
            scan_res[cnt]->auth_mode = AUTH_MODE_SHARED;
            prBssDesc->ucEncLevel = WIFI_ENCLEVEL_WEP;
        }
        if (prBssDesc->fgIEWPA) {
            scan_res[cnt]->auth_mode = WIFI_AUTH_MODE_WPA_PSK;
            for (i = 0; i < prBssDesc->rWPAInfo.u4PairwiseKeyCipherSuiteCount; i++) {
                if (prBssDesc->rWPAInfo.au4PairwiseKeyCipherSuite[i] ==
                    WPA_CIPHER_SUITE_TKIP)
                    ucMixedEncr |= BIT(0);
                if (prBssDesc->rWPAInfo.au4PairwiseKeyCipherSuite[i] ==
                    WPA_CIPHER_SUITE_CCMP)
                    ucMixedEncr |= BIT(1);
            }
            if (ucMixedEncr == 1)
                prBssDesc->ucEncLevel = WIFI_ENCLEVEL_TKIP;
            else if (ucMixedEncr == 2)
                prBssDesc->ucEncLevel = WIFI_ENCLEVEL_CCMP;
            else if (ucMixedEncr == 3)
                prBssDesc->ucEncLevel = WIFI_ENCLEVEL_TKIPCCMP;
            ucMixedEncr = 0;
        }
        if (prBssDesc->fgIERSN) {
            for (i = 0; i < prBssDesc->rRSNInfo.u4AuthKeyMgtSuiteCount; i++) {
                if (prBssDesc->rRSNInfo.au4AuthKeyMgtSuite[i] ==
                    RSN_AKM_SUITE_PSK)
                    ucMixedSec |= BIT(0);
                if (prBssDesc->rRSNInfo.au4AuthKeyMgtSuite[i] ==
                    RSN_AKM_SUITE_SAE)
                    ucMixedSec |= BIT(1);
            }
            if (ucMixedSec == 1)
                scan_res[cnt]->auth_mode = WIFI_AUTH_MODE_WPA2_PSK;
            else if (ucMixedSec == 2)
                scan_res[cnt]->auth_mode = WIFI_AUTH_MODE_WPA3_PSK;
            else
                scan_res[cnt]->auth_mode = WIFI_AUTH_MODE_WPA2_PSK_WPA3_PSK;
            ucMixedSec = 0;
            for (i = 0; i < prBssDesc->rRSNInfo.u4PairwiseKeyCipherSuiteCount; i++) {
                if (prBssDesc->rRSNInfo.au4PairwiseKeyCipherSuite[i] ==
                    RSN_CIPHER_SUITE_TKIP)
                    ucMixedEncr |= BIT(0);
                if (prBssDesc->rRSNInfo.au4PairwiseKeyCipherSuite[i] ==
                    RSN_CIPHER_SUITE_CCMP)
                    ucMixedEncr |= BIT(1);
            }
            if (ucMixedEncr == 1)
                prBssDesc->ucEncLevel = WIFI_ENCLEVEL_TKIP;
            else if (ucMixedEncr == 2)
                prBssDesc->ucEncLevel = WIFI_ENCLEVEL_CCMP;
            else if (ucMixedEncr == 3)
                prBssDesc->ucEncLevel = WIFI_ENCLEVEL_TKIPCCMP;
            ucMixedEncr = 0;
        }
        if (prBssDesc->fgIERSN && prBssDesc->fgIEWPA)
            scan_res[cnt]->auth_mode = WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK;
        scan_res[cnt]->encrypt_type = prBssDesc->ucEncLevel;
        ptr = (uint8_t *)(scan_res[cnt] + 1);
        kalMemCopy(ptr, prBssDesc->aucIEBuf,  scan_res[cnt]->ie_len);

        DBGLOG(SCN, INFO, "result[%d] %x:%x:%x:%x:%x:%x@freq:%d %s ie_len %d\n",
               results->num,
               scan_res[cnt]->bssid[0], scan_res[cnt]->bssid[1],
               scan_res[cnt]->bssid[2], scan_res[cnt]->bssid[3],
               scan_res[cnt]->bssid[4], scan_res[cnt]->bssid[5],
               scan_res[cnt]->freq, prBssDesc->aucSSID,
               scan_res[cnt]->ie_len);

#ifdef MTK_MINISUPP_ENABLE
        if (g_scan_list != NULL) {
            kalMemCopy(g_scan_list[cnt].bssid, prBssDesc->aucBSSID,
                       MAC_ADDR_LEN);
            kalMemCopy(g_scan_list[cnt].ssid, prBssDesc->aucSSID,
                       prBssDesc->ucSSIDLen);
            g_scan_list[cnt].ssid_length = prBssDesc->ucSSIDLen;
            g_scan_list[cnt].channel = prBssDesc->ucChannelNum;
            g_scan_list[cnt].auth_mode = scan_res[cnt]->auth_mode;
            if (scan_res[cnt]->encrypt_type == WIFI_ENCLEVEL_DISABLE)
                g_scan_list[cnt].encrypt_type =
                    WIFI_ENCRYPT_TYPE_ENCRYPT_DISABLED;
            else if (scan_res[cnt]->encrypt_type == WIFI_ENCLEVEL_WEP)
                g_scan_list[cnt].encrypt_type =
                    WIFI_ENCRYPT_TYPE_ENCRYPT1_ENABLED;
            else if (scan_res[cnt]->encrypt_type == WIFI_ENCLEVEL_TKIP)
                g_scan_list[cnt].encrypt_type =
                    WIFI_ENCRYPT_TYPE_ENCRYPT2_ENABLED;
            else if (scan_res[cnt]->encrypt_type == WIFI_ENCLEVEL_CCMP)
                g_scan_list[cnt].encrypt_type =
                    WIFI_ENCRYPT_TYPE_ENCRYPT3_ENABLED;
            else
                g_scan_list[cnt].encrypt_type =
                    WIFI_ENCRYPT_TYPE_ENCRYPT4_ENABLED;
            g_scan_list[cnt].beacon_interval = prBssDesc->u2BeaconInterval;
            g_scan_list[cnt].capability_info = prBssDesc->u2CapInfo;
            g_scan_list[cnt].rssi = RCPI_TO_dBm(prBssDesc->ucRCPI);
            if (prBssDesc->eBSSType == BSS_TYPE_IBSS)
                g_scan_list[cnt].bss_type = 0;
            else if (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE)
                g_scan_list[cnt].bss_type = 1;
            g_scan_list[cnt].is_hidden = prBssDesc->fgIsHiddenSSID;
#if CFG_SUPPORT_MBO
            g_scan_list[cnt].is_valid = !prBssDesc->fgIsDisallowed;
#else /* #if CFG_SUPPORT_MBO */
            g_scan_list[cnt].is_valid = prBssDesc->fgIsValidSSID;
#endif /* #if CFG_SUPPORT_MBO */
        }
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    }

    return results;
}

int mtk_freertos_wpa_connect(struct wpa_driver_associate_params *sme)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen;
    enum ENUM_WEP_STATUS eEncStatus;
    enum ENUM_PARAM_AUTH_MODE eAuthMode;
    int32_t cipher;
    struct PARAM_CONNECT rNewSsid;
#if (CFG_SUPPORT_WPA3 == 0)
    uint8_t  fgCarryWPSIE = FALSE;
#endif /* #if (CFG_SUPPORT_WPA3 == 0) */
    struct PARAM_OP_MODE eOpMode;
    uint32_t i, u4AkmSuite = 0;
    struct CONNECTION_SETTINGS *prConnSettings;

    struct DOT11_RSNA_CONFIG_AUTHENTICATION_SUITES_ENTRY *prEntry;
#if CFG_SUPPORT_REPLAY_DETECTION
    struct GL_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;
#endif /* #if CFG_SUPPORT_REPLAY_DETECTION */
    int algs = 0;
    int privacy = 0;


    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    /* printk("[wlan]mtk_cfg80211_connect\n"); */
    if (prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].eOPMode >
        NET_TYPE_AUTO_SWITCH)
        eOpMode.eOpMode = NET_TYPE_AUTO_SWITCH;
    else
        eOpMode.eOpMode =
            prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].eOPMode;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetInfrastructureMode, &eOpMode,
                       sizeof(eOpMode), FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(INIT, INFO,
               "wlanoidSetInfrastructureMode fail 0x%lx\n", rStatus);
        return -14;/* EFAULT; */
    }

    /* after set operation mode, key table are cleared */

    /* reset wpa info */
    prGlueInfo->rWpaInfo[0].u4WpaVersion = IW_AUTH_WPA_VERSION_DISABLED;
    prGlueInfo->rWpaInfo[0].u4KeyMgmt = 0;
    prGlueInfo->rWpaInfo[0].u4CipherGroup = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo[0].u4CipherPairwise = IW_AUTH_CIPHER_NONE;
    prGlueInfo->rWpaInfo[0].u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;

#if CFG_SUPPORT_REPLAY_DETECTION
    /* reset Detect replay information */
    prDetRplyInfo = aisGetDetRplyInfo(prGlueInfo->prAdapter,
                                      prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex);
    kalMemZero(prDetRplyInfo, sizeof(struct GL_DETECT_REPLAY_INFO));
#endif /* #if CFG_SUPPORT_REPLAY_DETECTION */

#if CFG_SUPPORT_802_11W
    prGlueInfo->rWpaInfo[0].u4Mfp = IW_AUTH_MFP_DISABLED;
    if (sme->mgmt_frame_protection == MGMT_FRAME_PROTECTION_REQUIRED)
        prGlueInfo->rWpaInfo[0].u4Mfp = IW_AUTH_MFP_REQUIRED;

#endif /* #if CFG_SUPPORT_802_11W */

    if (sme->wpa_proto & NL80211_WPA_VERSION_1)
        prGlueInfo->rWpaInfo[0].u4WpaVersion = IW_AUTH_WPA_VERSION_WPA;
    else if (sme->wpa_proto & NL80211_WPA_VERSION_2)
        prGlueInfo->rWpaInfo[0].u4WpaVersion = IW_AUTH_WPA_VERSION_WPA2;
    else
        prGlueInfo->rWpaInfo[0].u4WpaVersion =
            IW_AUTH_WPA_VERSION_DISABLED;

    algs = 0;
    if (sme->auth_alg & WPA_AUTH_ALG_OPEN)
        algs++;
    if (sme->auth_alg & WPA_AUTH_ALG_SHARED)
        algs++;
    if (sme->auth_alg & WPA_AUTH_ALG_LEAP)
        algs++;
#if (CFG_SUPPORT_WPA3 == 1)
    if (sme->auth_alg & WPA_AUTH_ALG_SAE)
        algs++;
#endif /* #if (CFG_SUPPORT_WPA3 == 1) */

    if (algs > 1) {
        DBGLOG(REQ, ERROR,
               "Leave out Auth Type for automatic selection, skip\n");
    } else {

        if (sme->auth_alg & WPA_AUTH_ALG_OPEN)
            prGlueInfo->rWpaInfo[0].u4AuthAlg =
                IW_AUTH_ALG_OPEN_SYSTEM;
        else if (sme->auth_alg & WPA_AUTH_ALG_SHARED)
            prGlueInfo->rWpaInfo[0].u4AuthAlg =
                IW_AUTH_ALG_SHARED_KEY;
        else if ((sme->auth_alg & WPA_AUTH_ALG_LEAP) ||
                 (sme->auth_alg & WPA_AUTH_ALG_FT))
            prGlueInfo->rWpaInfo[0].u4AuthAlg =
                IW_AUTH_ALG_OPEN_SYSTEM | IW_AUTH_ALG_SHARED_KEY;
#if (CFG_SUPPORT_WPA3 == 1)
        else if (sme->auth_alg & WPA_AUTH_ALG_SAE)
            prGlueInfo->rWpaInfo[0].u4AuthAlg =
                IW_AUTH_ALG_SAE;
#endif /* #if (CFG_SUPPORT_WPA3 == 1) */
        else {
            DBGLOG(REQ, ERROR, "Unknown auth type\n");
            return WLAN_STATUS_FAILURE;
        }
    }

    if (sme->pairwise_suite != WPA_CIPHER_NONE) {
        prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
        au4PairwiseKeyCipherSuite[0] = CIPHER_SUITE_CCMP;
        switch (sme->pairwise_suite) {
            case WPA_CIPHER_WEP40:
                prGlueInfo->rWpaInfo[0].
                u4CipherPairwise = IW_AUTH_CIPHER_WEP40;
                prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
                au4PairwiseKeyCipherSuite[0] =
                    CIPHER_SUITE_WEP40;
                break;
            case WPA_CIPHER_WEP104:
                prGlueInfo->rWpaInfo[0].
                u4CipherPairwise = IW_AUTH_CIPHER_WEP104;
                prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
                au4PairwiseKeyCipherSuite[0] =
                    CIPHER_SUITE_WEP104;
                break;
            case WPA_CIPHER_TKIP:
                prGlueInfo->rWpaInfo[0].
                u4CipherPairwise = IW_AUTH_CIPHER_TKIP;
                prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
                au4PairwiseKeyCipherSuite[0] =
                    CIPHER_SUITE_TKIP;
                break;
            case WPA_CIPHER_CCMP:
                prGlueInfo->rWpaInfo[0].
                u4CipherPairwise = IW_AUTH_CIPHER_CCMP;
                prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
                au4PairwiseKeyCipherSuite[0] =
                    CIPHER_SUITE_CCMP;
                break;
            case WPA_CIPHER_AES_128_CMAC:
                prGlueInfo->rWpaInfo[0].
                u4CipherPairwise = IW_AUTH_CIPHER_CCMP;
                prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
                au4PairwiseKeyCipherSuite[0] =
                    CIPHER_SUITE_CCMP;
                break;
            default:
                DBGLOG(REQ, WARN, "invalid cipher pairwise (%d)\n",
                       sme->pairwise_suite);
                return -EINVAL;
        }

    }

    DBGLOG(REQ, INFO, "sme->group_suite [%d]\n", sme->group_suite);

    if (sme->group_suite == WPA_CIPHER_GTK_NOT_USED
        /*&& !(drv->capa.enc & WPA_DRIVER_CAPA_ENC_GTK_NOT_USED)*/) {
        /*
         * This is likely to work even though many drivers do not
         * advertise support for operations without GTK.
         */
    } else if (sme->group_suite != WPA_CIPHER_NONE) {
        prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
        u4GroupKeyCipherSuite = CIPHER_SUITE_CCMP;
        switch (sme->group_suite) {
            case WPA_CIPHER_WEP40:
                prGlueInfo->rWpaInfo[0].
                u4CipherGroup = IW_AUTH_CIPHER_WEP40;
                prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
                u4GroupKeyCipherSuite = CIPHER_SUITE_WEP40;
                break;
            case WPA_CIPHER_WEP104:
                prGlueInfo->rWpaInfo[0].
                u4CipherGroup = IW_AUTH_CIPHER_WEP104;
                prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
                u4GroupKeyCipherSuite = CIPHER_SUITE_WEP104;
                break;
            case WPA_CIPHER_TKIP:
                prGlueInfo->rWpaInfo[0].
                u4CipherGroup = IW_AUTH_CIPHER_TKIP;
                prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
                u4GroupKeyCipherSuite = CIPHER_SUITE_TKIP;
                break;
            case WPA_CIPHER_CCMP:
                prGlueInfo->rWpaInfo[0].
                u4CipherGroup = IW_AUTH_CIPHER_CCMP;
                prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
                u4GroupKeyCipherSuite =
                    CIPHER_SUITE_CCMP;
                break;
            case WPA_CIPHER_AES_128_CMAC:
                prGlueInfo->rWpaInfo[0].
                u4CipherGroup = IW_AUTH_CIPHER_CCMP;
                prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
                u4GroupKeyCipherSuite =
                    CIPHER_SUITE_CCMP;
                break;
            default:
                DBGLOG(REQ, WARN, "invalid cipher group (%d)\n",
                       sme->group_suite);
                return -EINVAL;
        }
    }

    /* DBGLOG(SCN, INFO, ("akm_suites=%x\n", sme->crypto.akm_suites[0])); */
    if (sme->key_mgmt_suite == WPA_KEY_MGMT_IEEE8021X ||
        sme->key_mgmt_suite == WPA_KEY_MGMT_PSK ||
        sme->key_mgmt_suite == WPA_KEY_MGMT_FT_IEEE8021X ||
        sme->key_mgmt_suite == WPA_KEY_MGMT_FT_PSK ||
        sme->key_mgmt_suite == WPA_KEY_MGMT_CCKM ||
        sme->key_mgmt_suite == WPA_KEY_MGMT_OSEN ||
        sme->key_mgmt_suite == WPA_KEY_MGMT_IEEE8021X_SHA256 ||
        sme->key_mgmt_suite == WPA_KEY_MGMT_PSK_SHA256 ||
        sme->key_mgmt_suite == WPA_KEY_MGMT_IEEE8021X_SUITE_B ||
        sme->key_mgmt_suite == WPA_KEY_MGMT_IEEE8021X_SUITE_B_192
#if (CFG_SUPPORT_WPA3 ==  1)
        || sme->key_mgmt_suite == WPA_KEY_MGMT_SAE
        || sme->key_mgmt_suite == WPA_KEY_MGMT_FT_SAE
#endif /* #if (CFG_SUPPORT_WPA3 ==  1) */
#if CFG_SUPPORT_DPP
        || sme->key_mgmt_suite == WPA_KEY_MGMT_DPP
#endif /* #if CFG_SUPPORT_DPP */
       ) {
        if (prGlueInfo->rWpaInfo[0].u4WpaVersion ==
            IW_AUTH_WPA_VERSION_WPA) {
            switch (sme->key_mgmt_suite) {
                case WPA_KEY_MGMT_IEEE8021X:
                    eAuthMode = AUTH_MODE_WPA;
                    u4AkmSuite = WPA_AKM_SUITE_802_1X;
                    break;
                case WPA_KEY_MGMT_PSK:
                    eAuthMode = AUTH_MODE_WPA_PSK;
                    u4AkmSuite = WPA_AKM_SUITE_PSK;
                    break;
                default:
                    DBGLOG(REQ, WARN, "invalid Akm Suite (%d)\n",
                           sme->key_mgmt_suite);
                    return -EINVAL;
            }
        } else if (prGlueInfo->rWpaInfo[0].u4WpaVersion ==
                   IW_AUTH_WPA_VERSION_WPA2) {
            switch (sme->key_mgmt_suite) {

                case WPA_KEY_MGMT_IEEE8021X:
                    eAuthMode = AUTH_MODE_WPA2;
                    u4AkmSuite = RSN_AKM_SUITE_802_1X;
                    break;
#if CFG_SUPPORT_802_11W
                /* Notice:: Need kernel patch!! */
                case WPA_KEY_MGMT_IEEE8021X_SHA256:
                    eAuthMode = AUTH_MODE_WPA2;
                    u4AkmSuite = RSN_AKM_SUITE_802_1X_SHA256;
                    break;
                case WPA_KEY_MGMT_PSK_SHA256:
                    eAuthMode = AUTH_MODE_WPA2_PSK;
                    u4AkmSuite = RSN_AKM_SUITE_PSK_SHA256;
                    break;
#endif /* #if CFG_SUPPORT_802_11W */
                case WPA_KEY_MGMT_PSK:
                    eAuthMode = AUTH_MODE_WPA2_PSK;
                    u4AkmSuite = RSN_AKM_SUITE_PSK;
                    break;
#if (CFG_SUPPORT_WPA3 == 1)
                case WPA_KEY_MGMT_SAE:
                    eAuthMode = AUTH_MODE_WPA3_SAE;
                    u4AkmSuite = RSN_CIPHER_SUITE_SAE;
                    break;
                case WPA_KEY_MGMT_FT_SAE:
                    eAuthMode = AUTH_MODE_WPA3_FT_SAE;
                    u4AkmSuite = RSN_AKM_SUITE_FT_OVER_SAE;
                    break;
#endif /* #if (CFG_SUPPORT_WPA3 == 1) */
#if CFG_SUPPORT_DPP
                case WPA_KEY_MGMT_DPP:
                    eAuthMode = AUTH_MODE_WPA2_PSK;
                    u4AkmSuite = RSN_AKM_SUITE_DPP;
                    break;
#endif /* #if CFG_SUPPORT_DPP */
                case WPA_KEY_MGMT_FT_IEEE8021X:
                    eAuthMode = AUTH_MODE_WPA2_FT;
                    u4AkmSuite = RSN_AKM_SUITE_FT_802_1X;
                    break;
                case WPA_KEY_MGMT_FT_PSK:
                    eAuthMode = AUTH_MODE_WPA2_FT_PSK;
                    u4AkmSuite = RSN_AKM_SUITE_FT_PSK;
                    break;
#if CFG_SUPPORT_PASSPOINT
                case WPA_KEY_MGMT_OSEN:
                    eAuthMode = AUTH_MODE_WPA_OSEN;
                    u4AkmSuite = WFA_AKM_SUITE_OSEN;
                    break;
#endif /* #if CFG_SUPPORT_PASSPOINT */
                case WPA_KEY_MGMT_IEEE8021X_SUITE_B:
                    eAuthMode = AUTH_MODE_WPA2_PSK;
                    u4AkmSuite = RSN_AKM_SUITE_8021X_SUITE_B_192;
                    break;
                case WPA_KEY_MGMT_IEEE8021X_SUITE_B_192:
                    eAuthMode = AUTH_MODE_WPA2_PSK;
                    u4AkmSuite = RSN_AKM_SUITE_8021X_SUITE_B_192;
                    break;
                default:
                    DBGLOG(REQ, WARN, "invalid Akm Suite (%d)\n",
                           sme->key_mgmt_suite);
                    return -EINVAL; /*supplicant default*/
            }
        }
        prGlueInfo->prAdapter->rWifiVar.rConnSettings[0].rRsnInfo.
        au4AuthKeyMgtSuite[0] = GET_SELECTOR_TYPE(u4AkmSuite);
    }

    if (prGlueInfo->rWpaInfo[0].u4WpaVersion ==
        IW_AUTH_WPA_VERSION_DISABLED) {
        eAuthMode =
            (prGlueInfo->rWpaInfo[0].u4AuthAlg ==
             IW_AUTH_ALG_OPEN_SYSTEM)
            ? AUTH_MODE_OPEN : AUTH_MODE_AUTO_SWITCH;
    }

    for (i = 0; i < 4; i++) {
        if (!sme->wep_key[i])
            continue;
        privacy = 1;
        break;
    }
    if (sme->wps == WPS_MODE_PRIVACY)
        privacy = 1;
    if (sme->pairwise_suite &&
        sme->pairwise_suite != WPA_CIPHER_NONE)
        privacy = 1;

    prGlueInfo->rWpaInfo[0].fgPrivacyInvoke = privacy;
    /* prGlueInfo->fgWpsActive = FALSE; */

#if CFG_SUPPORT_PASSPOINT
    prGlueInfo->fgConnectHS20AP = FALSE;
#endif /* #if CFG_SUPPORT_PASSPOINT */

    DBGLOG(REQ, INFO, "sme->wpa_ie_len (%d)\n",
           sme->wpa_ie_len);

    /* clear WSC Assoc IE buffer in case WPS IE is not detected */
    prConnSettings =
        aisGetConnSettings(prGlueInfo->prAdapter,
                           prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex);

    if (sme->wpa_ie && sme->wpa_ie_len > 0) {
        uint8_t *prDesiredIE = NULL;
        uint8_t *pucIEStart = (uint8_t *)sme->wpa_ie;

#if (CFG_SUPPORT_WPA3 == 1)
        prConnSettings->assocIeLen = sme->wpa_ie_len;
        DBGLOG(REQ, INFO, "copy wpa_ie_len %d assocIeLen %d\n",
               sme->wpa_ie_len, prConnSettings->assocIeLen);
        if (prConnSettings->assocIeLen <= 200) {
            kalMemCopy(prConnSettings->pucAssocIEs,
                       sme->wpa_ie, prConnSettings->assocIeLen);
        } else {
            DBGLOG(REQ, INFO,
                   "allocate pucAssocIEs failed!\n");
            prConnSettings->assocIeLen = 0;
        }
#endif /* #if (CFG_SUPPORT_WPA3 == 1) */

#if CFG_SUPPORT_WAPI
        rStatus = kalIoctl(prGlueInfo, wlanoidSetWapiAssocInfo,
                           pucIEStart, sme->wpa_ie_len, FALSE, FALSE, FALSE,
                           &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS)
            DBGLOG(SEC, WARN,
                   "[wapi] set wapi assoc info error:%lx\n",
                   rStatus);
#endif /* #if CFG_SUPPORT_WAPI */

#if (CFG_SUPPORT_WPA3 == 0)
#if CFG_SUPPORT_WPS2
        if (wextSrchDesiredWPSIE(pucIEStart, sme->wpa_ie_len, 0xDD,
                                 (uint8_t **) &prDesiredIE)) {
            /*prGlueInfo->fgWpsActive = TRUE;*/
            fgCarryWPSIE = TRUE;

            rStatus = kalIoctl(prGlueInfo, wlanoidSetWSCAssocInfo,
                               prDesiredIE, IE_SIZE(prDesiredIE),
                               FALSE, FALSE, FALSE, &u4BufLen);
            if (rStatus != WLAN_STATUS_SUCCESS)
                DBGLOG(SEC, WARN,
                       "[WSC] set WSC assoc info error:%lx\n",
                       rStatus);
        }
#endif /* #if CFG_SUPPORT_WPS2 */
#endif /* #if (CFG_SUPPORT_WPA3 == 0) */

        if (wextSrchDesiredWPAIE(pucIEStart, sme->wpa_ie_len, 0x30,
                                 (uint8_t **) &prDesiredIE)) {
            struct RSN_INFO rRsnInfo;

            if (rsnParseRsnIE(prGlueInfo->prAdapter,
                              (struct RSN_INFO_ELEM *)prDesiredIE, &rRsnInfo)) {
#if CFG_SUPPORT_802_11W
                if (rRsnInfo.u2RsnCap & ELEM_WPA_CAP_MFPC) {
                    prGlueInfo->rWpaInfo[0].
                    u4CipherGroupMgmt =
                        rRsnInfo.
                        u4GroupMgmtKeyCipherSuite;
                    prGlueInfo->rWpaInfo[0].ucRSNMfpCap =
                        RSN_AUTH_MFP_OPTIONAL;
                    if (rRsnInfo.u2RsnCap &
                        ELEM_WPA_CAP_MFPR)
                        prGlueInfo->rWpaInfo[0].
                        ucRSNMfpCap
                            = RSN_AUTH_MFP_REQUIRED;
                } else
                    prGlueInfo->rWpaInfo[0].ucRSNMfpCap
                        = 0;

                DBGLOG(REQ, INFO, "MPF(%d)\n",
                       prGlueInfo->rWpaInfo[0].ucRSNMfpCap);

                /* Fill RSNE PMKID Count and List */
                prConnSettings->rRsnInfo.u2PmkidCnt =
                    rRsnInfo.u2PmkidCnt;
                if (rRsnInfo.u2PmkidCnt > 0)
                    kalMemCopy(
                        prConnSettings->rRsnInfo.
                        aucPmkidList,
                        rRsnInfo.aucPmkidList,
                        (rRsnInfo.u2PmkidCnt *
                         RSN_PMKID_LEN));
#endif /* #if CFG_SUPPORT_802_11W */
            }
        }
    }

#if (CFG_SUPPORT_WPA3 == 0)
    if (fgCarryWPSIE == FALSE) {
        kalMemZero(&prConnSettings->aucWSCAssocInfoIE, 200);
        prConnSettings->u2WSCAssocInfoIELen = 0;
    }
#endif /* #if (CFG_SUPPORT_WPA3 == 0) */

    rStatus = kalIoctl(prGlueInfo, wlanoidSetAuthMode, &eAuthMode,
                       sizeof(eAuthMode), FALSE, FALSE, FALSE, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(REQ, WARN, "set auth mode error:%lx\n", rStatus);

    /* Enable the specific AKM suite only. */
    for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++) {
        prEntry = &prGlueInfo->prAdapter->rMib[0].
                  dot11RSNAConfigAuthenticationSuitesTable[i];

        if (prEntry->dot11RSNAConfigAuthenticationSuite == u4AkmSuite) {
            prEntry->dot11RSNAConfigAuthenticationSuiteEnabled
                = TRUE;
        } else {
            prEntry->dot11RSNAConfigAuthenticationSuiteEnabled
                = FALSE;
        }
    }

    cipher = prGlueInfo->rWpaInfo[0].u4CipherGroup |
             prGlueInfo->rWpaInfo[0].u4CipherPairwise;

    if (prGlueInfo->rWpaInfo[0].fgPrivacyInvoke) {
        if (cipher & IW_AUTH_CIPHER_CCMP)
            eEncStatus = ENUM_ENCRYPTION3_ENABLED;
        else if (cipher & IW_AUTH_CIPHER_TKIP)
            eEncStatus = ENUM_ENCRYPTION2_ENABLED;
        else if (cipher &
                 (IW_AUTH_CIPHER_WEP104 | IW_AUTH_CIPHER_WEP40))
            eEncStatus = ENUM_ENCRYPTION1_ENABLED;
        else if (cipher & IW_AUTH_CIPHER_NONE) {
            if (prGlueInfo->rWpaInfo[0].fgPrivacyInvoke)
                eEncStatus = ENUM_ENCRYPTION1_ENABLED;
            else
                eEncStatus = ENUM_ENCRYPTION_DISABLED;
        } else
            eEncStatus = ENUM_ENCRYPTION_DISABLED;
    } else
        eEncStatus = ENUM_ENCRYPTION_DISABLED;

    rStatus = kalIoctl(prGlueInfo, wlanoidSetEncryptionStatus, &eEncStatus,
                       sizeof(eEncStatus), FALSE, FALSE, FALSE, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(SEC, WARN, "set encryption mode error:%lx\n", rStatus);

    if (sme->wep_tx_keyidx >= 0 &&
        sme->wep_key_len[sme->wep_tx_keyidx] != 0) {
        DBGLOG(SEC, WARN, "WEP key len: %d, tx_idx: %d, key: %s\n",
               sme->wep_key_len[sme->wep_tx_keyidx],
               sme->wep_tx_keyidx,
               sme->wep_key[sme->wep_tx_keyidx]);
    }

    rNewSsid.u4CenterFreq = sme->freq.freq;
    rNewSsid.pucBssid = (uint8_t *)sme->bssid;
    rNewSsid.pucSsid = (uint8_t *)sme->ssid;
    rNewSsid.u4SsidLen = sme->ssid_len;
    rNewSsid.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;

    DBGLOG(INIT, INFO, "connect freq = %d\n", rNewSsid.u4CenterFreq);
    DBGLOG(INIT, INFO, "connect bssid = %02x:%02x:%02x:%02x:%02x:%02x\n",
           rNewSsid.pucBssid[0], rNewSsid.pucBssid[1],
           rNewSsid.pucBssid[2], rNewSsid.pucBssid[3],
           rNewSsid.pucBssid[4], rNewSsid.pucBssid[5]);
    DBGLOG(INIT, INFO, "connect ssid = %s\n", rNewSsid.pucSsid);
    DBGLOG(INIT, INFO, "connect ssid_len = %d\n", rNewSsid.u4SsidLen);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetConnect, (void *)&rNewSsid,
                       sizeof(struct PARAM_CONNECT), FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "set SSID:%x\n", rStatus);
        return -22;/* -EINVAL; */
    }

    DBGLOG(INIT, INFO, "> exit\n");
    return 0;
}

#if CFG_SUPPORT_WPA3
int mtk_freertos_wpa_external_auth(
    struct external_auth *params)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus = WLAN_STATUS_FAILURE;
    uint32_t u4BufLen;
    struct PARAM_EXTERNAL_AUTH auth;

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    DBGLOG(REQ, TRACE, "\n");

    COPY_MAC_ADDR(auth.bssid, params->bssid);
    auth.status = params->status;
    auth.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    rStatus = kalIoctl(prGlueInfo, wlanoidExternalAuthDone, (void *)&auth,
                       sizeof(auth), FALSE, FALSE, FALSE, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(OID, INFO, "SAE-confirm failed with: %d\n", rStatus);

    return 0;
}

int mtk_freertos_wpa_send_mlme(void *priv, const u8 *buf, size_t len)
{

    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus = WLAN_STATUS_FAILURE;
    uint32_t u4BufLen;
    struct EXTERNAL_AUTH_PKT auth_pkt;

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    DBGLOG(REQ, TRACE, "\n");

    kalMemZero(&auth_pkt, sizeof(struct EXTERNAL_AUTH_PKT));
    kalMemCopy(auth_pkt.ucFrame, buf, len);
    auth_pkt.usFrameLen = len;
    rStatus = kalIoctl(prGlueInfo, wlanoidSendExternalAuth,
                       (void *)&auth_pkt, sizeof(struct EXTERNAL_AUTH_PKT),
                       FALSE, FALSE, FALSE, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(OID, INFO, "send externAuth: %d\n", rStatus);

    return 0;
}

#endif /* #if CFG_SUPPORT_WPA3 */

int mtk_freertos_wpa_add_pmkid(void *priv, const u8 *bssid, const u8 *pmkid)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus;
    uint32_t u4BufLen;
    struct PARAM_PMKID pmksa;
    uint8_t ucBssIndex = 0;

    DBGLOG(REQ, TRACE, "set_pmksa " MACSTR " pmk\n",
           MAC2STR(bssid));

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ucBssIndex = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    if (!IS_BSS_INDEX_VALID(ucBssIndex))
        return -EINVAL;

    COPY_MAC_ADDR(pmksa.arBSSID, bssid);
    kalMemCopy(pmksa.arPMKID, pmkid, IW_PMKID_LEN);
    pmksa.ucBssIdx = ucBssIndex;
    rStatus = kalIoctl(prGlueInfo, wlanoidSetPmkid, &pmksa,
                       sizeof(struct PARAM_PMKID),
                       FALSE, FALSE, FALSE, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(INIT, INFO, "add pmkid error:%x\n", rStatus);

    return 0;
}

int mtk_freertos_wpa_remove_pmkid(void *priv, const u8 *bssid, const u8 *pmkid)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus;
    uint32_t u4BufLen;
    struct PARAM_PMKID pmksa;
    uint8_t ucBssIndex = 0;

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;

    DBGLOG(REQ, TRACE, "del_pmksa " MACSTR "\n",
           MAC2STR(bssid));

    ucBssIndex = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    if (!IS_BSS_INDEX_VALID(ucBssIndex))
        return -EINVAL;

    COPY_MAC_ADDR(pmksa.arBSSID, bssid);
    kalMemCopy(pmksa.arPMKID, pmkid, IW_PMKID_LEN);
    pmksa.ucBssIdx = ucBssIndex;
    rStatus = kalIoctl(prGlueInfo, wlanoidDelPmkid, &pmksa,
                       sizeof(struct PARAM_PMKID),
                       FALSE, FALSE, FALSE, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(INIT, INFO, "add pmkid error:%x\n", rStatus);

    return 0;

}

int mtk_freertos_wpa_flush_pmkid(void *priv)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus;
    uint32_t u4BufLen;
    uint8_t ucBssIndex = 0;

    prGlueInfo = (struct GLUE_INFO *)g_prGlueInfo;
    ASSERT(prGlueInfo);

    ucBssIndex = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    if (!IS_BSS_INDEX_VALID(ucBssIndex))
        return -EINVAL;

    DBGLOG(REQ, INFO, "ucBssIndex = %d\n", ucBssIndex);

    rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidFlushPmkid, NULL, 0,
                               FALSE, FALSE, FALSE, &u4BufLen,
                               ucBssIndex);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(INIT, INFO, "flush pmkid error:%x\n", rStatus);

    return 0;

}


/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to disconnect from
 *        currently connected ESS
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_freertos_wpa_disconnect(void *priv, const uint8_t *addr,
                                int reason_code)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus;
    uint32_t u4BufLen;
    uint32_t u4DisconnectReason = reason_code;

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    DBGLOG(REQ, WARN, "%02x:%02x:%02x:%02x:%02x:%02x, reson code:%d\n",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
           reason_code);
    rStatus = kalIoctl(prGlueInfo, wlanoidSetDisassociate, &u4DisconnectReason, 0,
                       FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, WARN, "disassociate error:%lx\n", rStatus);
        return -EFAULT;
    }

    return 0;
}

int mtk_freertos_mgmt_tx(void *priv, int freq, const uint8_t *buf,
                         uint32_t len, u64 *cookie, int no_cck, int offchan)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    int i4Rslt = -EINVAL;
    struct MSG_MGMT_TX_REQUEST *prMsgTxReq = NULL;
    struct MSDU_INFO *prMgmtFrame = NULL;
    uint8_t *pucFrameBuf = NULL;

    do {
        if (buf == NULL || len == 0 || cookie == NULL)
            break;

        prGlueInfo = (struct GLUE_INFO *)g_prGlueInfo;
        ASSERT(prGlueInfo);

        *cookie = prGlueInfo->u8Cookie++;

        /* Channel & Channel Type & Wait time are ignored. */
        prMsgTxReq = cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
                                 sizeof(struct MSG_MGMT_TX_REQUEST));

        if (prMsgTxReq == NULL) {
            ASSERT(FALSE);
            i4Rslt = -ENOMEM;
            break;
        }

#if CONFIG_WIFI_OFFCHAN_TX
        if (offchan) {
            DBGLOG(REQ, TRACE, "Off channel TRUE\n");
            prMsgTxReq->fgIsOffChannel = TRUE;

            mtk_p2p_channel_format_switch(NULL,
                                          nicFreq2ChannelNum(freq * 1000),
                                          &prMsgTxReq->rChannelInfo);
            prMsgTxReq->eChnlExt = CHNL_EXT_SCN;
        } else {
            prMsgTxReq->fgIsOffChannel = FALSE;
        }
#else /* #if CONFIG_WIFI_OFFCHAN_TX */
        prMsgTxReq->fgIsOffChannel = FALSE;
#endif /* #if CONFIG_WIFI_OFFCHAN_TX */

        if (no_cck)
            prMsgTxReq->fgNoneCckRate = TRUE;
        else
            prMsgTxReq->fgNoneCckRate = FALSE;

        prMsgTxReq->fgIsWaitRsp = TRUE;

        prMgmtFrame = cnmMgtPktAlloc(prGlueInfo->prAdapter,
                                     (uint32_t)(len + MAC_TX_RESERVED_FIELD));
        prMsgTxReq->prMgmtMsduInfo = prMgmtFrame;
        if (prMsgTxReq->prMgmtMsduInfo == NULL) {
            ASSERT(FALSE);
            i4Rslt = -ENOMEM;
            break;
        }

        prMsgTxReq->u8Cookie = *cookie;
        prMsgTxReq->rMsgHdr.eMsgId = MID_MNY_AIS_MGMT_TX;
        prMsgTxReq->ucBssIdx =
            prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;

        pucFrameBuf = (uint8_t *)((unsigned long)prMgmtFrame->prPacket +
                                  MAC_TX_RESERVED_FIELD);

        kalMemCopy(pucFrameBuf, buf, len);

        prMgmtFrame->u2FrameLength = len;
        prMgmtFrame->ucBssIndex =
            prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (struct MSG_HDR *)prMsgTxReq, MSG_SEND_METHOD_BUF);

        i4Rslt = 0;
    } while (FALSE);

    if (i4Rslt != 0 && prMsgTxReq != NULL) {
        if (prMsgTxReq->prMgmtMsduInfo != NULL)
            cnmMgtPktFree(prGlueInfo->prAdapter, prMsgTxReq->prMgmtMsduInfo);

        cnmMemFree(prGlueInfo->prAdapter, prMsgTxReq);
    }

    return i4Rslt;
}

struct key_params {
    const uint8_t *key;
    const uint8_t *seq;
    int key_len;
    int seq_len;
    uint32_t cipher;
};

struct key_parse {
    struct key_params p;
    int idx;
    int type;
    bool def, defmgmt;
    bool def_uni, def_multi;
};

enum nl80211_key_type {
    NL80211_KEYTYPE_GROUP,
    NL80211_KEYTYPE_PAIRWISE,
    NL80211_KEYTYPE_PEERKEY,

    NUM_NL80211_KEYTYPES
};

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for adding key
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/

int mtk_freertos_wpa_add_key(uint8_t key_index, bool pairwise,
                             const uint8_t *mac_addr, struct key_params *params, int bssidx)
{
    struct PARAM_KEY rKey;
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    int i4Rslt = -EINVAL;
    uint32_t u4BufLen = 0;
    uint8_t tmp1[8], tmp2[8];
#if CFG_SUPPORT_REPLAY_DETECTION
    uint8_t ucCheckZeroKey = 0;
    uint8_t i = 0;
#endif /* #if CFG_SUPPORT_REPLAY_DETECTION */
    const uint8_t aucBCAddr[] = BC_MAC_ADDR;
    /* struct PARAM_GTK_REKEY_DATA *prGtkData; */
    /* const UINT_8 aucZeroMacAddr[] = NULL_MAC_ADDR; */

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

#if DBG
    DBGLOG(RSN, WARN, "mtk_cfg80211_add_key\n");
    if (mac_addr) {
        DBGLOG(RSN, WARN,
               "keyIdx = %d pairwise = %d mac = " MACSTR "\n",
               key_index, pairwise, MAC2STR(mac_addr));
    } else {
        DBGLOG(RSN, WARN, "keyIdx = %d pairwise = %d null mac\n",
               key_index, pairwise);
    }
    DBGLOG(RSN, WARN, "Cipher = %x\n", params->cipher);
    DBGLOG_MEM8(RSN, WARN, params->key, params->key_len);
#endif /* #if DBG */

    kalMemZero(&rKey, sizeof(struct PARAM_KEY));

    rKey.u4KeyIndex = key_index;

    if (params->cipher) {
        DBGLOG(RSN, WARN, "req cipher = %d\n", params->cipher);
        switch (params->cipher) {
            case WPA_ALG_WEP:
                if (params->key_len == 5)
                    rKey.ucCipher = CIPHER_SUITE_WEP40;
                else if (params->key_len == 13)
                    rKey.ucCipher = CIPHER_SUITE_WEP104;
                else
                    rKey.ucCipher = CIPHER_SUITE_WEP128;
                break;
            case WPA_ALG_TKIP:
                rKey.ucCipher = CIPHER_SUITE_TKIP;
                DBGLOG(RSN, INFO, "alg: CIPHER_SUITE_TKIP\n");
                break;
            case WPA_ALG_CCMP:
                rKey.ucCipher = CIPHER_SUITE_CCMP;
                DBGLOG(RSN, INFO, "alg: CIPHER_SUITE_CCMP\n");
                break;
            case WPA_ALG_BIP_CMAC_128:
                rKey.ucCipher = CIPHER_SUITE_BIP;
                DBGLOG(RSN, INFO, "alg: CIPHER_SUITE_BIP\n");
                break;
            default:
                DBGLOG(RSN, WARN, "not support cipher type [%d]\n",
                       params->cipher);
                ASSERT(FALSE);
        }
    }

    if (pairwise) {
        ASSERT(mac_addr);
        rKey.u4KeyIndex |= BIT(31);
        rKey.u4KeyIndex |= BIT(30);
        COPY_MAC_ADDR(rKey.arBSSID, mac_addr);
#if 0
        /* reset KCK, KEK, EAPOL Replay counter */
        kalMemZero(prGtkData->aucKek, NL80211_KEK_LEN);
        kalMemZero(prGtkData->aucKck, NL80211_KCK_LEN);
        kalMemZero(prGtkData->aucReplayCtr,
                   NL80211_REPLAY_CTR_LEN);
#endif /* #if 0 */
    } else {        /* Group key */
        COPY_MAC_ADDR(rKey.arBSSID, aucBCAddr);
    }

    if (params->key) {

#if CFG_SUPPORT_REPLAY_DETECTION
        for (i = 0; i < params->key_len; i++) {
            if (params->key[i] == 0x00)
                ucCheckZeroKey++;
        }

        if (ucCheckZeroKey == params->key_len)
            return 0;
#endif /* #if CFG_SUPPORT_REPLAY_DETECTION */

        kalMemCopy(rKey.aucKeyMaterial, params->key, params->key_len);
        if (rKey.ucCipher == CIPHER_SUITE_TKIP) {
            kalMemCopy(tmp1, &params->key[16], 8);
            kalMemCopy(tmp2, &params->key[24], 8);
            kalMemCopy(&rKey.aucKeyMaterial[16], tmp2, 8);
            kalMemCopy(&rKey.aucKeyMaterial[24], tmp1, 8);
        }
    }


    /*  rKey.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex;*/
    rKey.ucBssIdx = bssidx;

    rKey.u4KeyLength = params->key_len;
    rKey.u4Length = ((unsigned long) & (((struct PARAM_KEY *) 0)->aucKeyMaterial)) +
                    rKey.u4KeyLength;

    rStatus = kalIoctl(prGlueInfo, wlanoidSetAddKey, &rKey, rKey.u4Length,
                       FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus == WLAN_STATUS_SUCCESS)
        i4Rslt = 0;

    return i4Rslt;
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for setting default key on an interface
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_freertos_wpa_set_default_key(uint8_t key_index,
                                     bool unicast, bool multicast)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    struct PARAM_DEFAULT_KEY rDefaultKey;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    int i4Rst = -EINVAL;
    uint32_t u4BufLen = 0;

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    /* For STA, should wep set the default key !! */
    DBGLOG(RSN, INFO, "mtk_cfg80211_set_default_key\n");
    DBGLOG(RSN, INFO, "keyIdx = %d unicast = %d multicast = %d\n",
           key_index, unicast, multicast);

    rDefaultKey.ucKeyID = key_index;
    rDefaultKey.ucUnicast = unicast;
    rDefaultKey.ucMulticast = multicast;
    if (rDefaultKey.ucUnicast && !rDefaultKey.ucMulticast)
        return WLAN_STATUS_SUCCESS;

    rDefaultKey.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSetDefaultKey,
                       &rDefaultKey, sizeof(struct PARAM_DEFAULT_KEY), FALSE,
                       FALSE, TRUE, &u4BufLen);
    if (rStatus == WLAN_STATUS_SUCCESS)
        i4Rst = 0;

    return i4Rst;
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for removing key for specified STA
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_freertos_wpa_del_key(uint8_t key_index, bool pairwise,
                             const uint8_t *mac_addr)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    struct PARAM_REMOVE_KEY rRemoveKey;
    uint32_t u4BufLen = 0;
    int i4Rslt = -EINVAL;

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    if (g_u4HaltFlag) {
        DBGLOG(RSN, WARN, "wlan is halt, skip key deletion\n");
        return WLAN_STATUS_FAILURE;
    }

#if DBG
    DBGLOG(RSN, TRACE, "mtk_cfg80211_del_key\n");
    if (mac_addr) {
        DBGLOG(RSN, TRACE,
               "keyIdx = %d pairwise = %d mac = " MACSTR "\n",
               key_index, pairwise, MAC2STR(mac_addr));
    } else {
        DBGLOG(RSN, TRACE, "keyIdx = %d pairwise = %d null mac\n",
               key_index, pairwise);
    }
#endif /* #if DBG */

    kalMemZero(&rRemoveKey, sizeof(struct PARAM_REMOVE_KEY));
    rRemoveKey.u4KeyIndex = key_index;
    rRemoveKey.u4Length = sizeof(struct PARAM_REMOVE_KEY);
    if (mac_addr) {
        COPY_MAC_ADDR(rRemoveKey.arBSSID, mac_addr);
        rRemoveKey.u4KeyIndex |= BIT(30);
    }

    rRemoveKey.ucBssIdx = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSetRemoveKey, &rRemoveKey,
                       rRemoveKey.u4Length, FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(RSN, WARN, "remove key error:%lx\n", rStatus);
    else
        i4Rslt = 0;

    return i4Rslt;
}

#ifdef MTK_MINISUPP_ENABLE
int mtk_freertos_wpa_set_key(const char *ifname, void *priv,
                             enum wpa_alg alg, const uint8_t *addr, int key_idx, int set_tx,
                             const uint8_t *seq, uint32_t seq_len, const uint8_t *key, uint32_t key_len)
{
    struct key_parse k;
    k.type = NL80211_KEYTYPE_GROUP;
    int ret = -1;
    int bssidx = 0;

    DBGLOG(REQ, INFO, "addr[%02x:%02x:%02x:%02x:%02x:%02x]\n",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    DBGLOG(REQ, INFO, "alg[%d], key_idx[%d], seq_len[%d], key_len[%d]\n",
           alg, key_idx, seq_len, key_len);

    if (os_strstr(ifname, "p2p0") || os_strstr(ifname, "ra0"))
        bssidx = 1;
    else
        bssidx = 0;

    if (alg == CIPHER_SUITE_NONE) {
        DBGLOG(REQ, INFO, "WPA_ALG_NONE, need del key\n");
        ret = 0;
        ret = mtk_freertos_wpa_del_key(key_idx,
                                       k.type == NL80211_KEYTYPE_PAIRWISE, addr);
        if (ret != 0)
            return -WLAN_STATUS_FAILURE;
    } else {
        DBGLOG(REQ, INFO, "Check alg if get not support\n");
        if ((alg != WPA_ALG_WEP) && (alg != WPA_ALG_TKIP) &&
            (alg != WPA_ALG_CCMP) && (alg != WPA_ALG_GCMP) &&
            (alg != WPA_ALG_CCMP_256) && (alg != WPA_ALG_GCMP_256) &&
            (alg != WPA_ALG_BIP_GMAC_128) &&
            (alg != WPA_ALG_BIP_CMAC_128) &&
            (alg != WPA_ALG_BIP_GMAC_256) &&
            (alg != WPA_ALG_BIP_CMAC_256) &&
            (alg != WPA_ALG_SMS4) && (alg != WPA_ALG_KRK)) {
            DBGLOG(REQ, WARN, "not support key type %d\n", alg);
            return WLAN_STATUS_FAILURE;
        }

        ret = -1;
        memset(&k, 0, sizeof(k));
        if (key) {
            k.def_uni = TRUE;
            k.def_multi = FALSE;
            k.idx = key_idx;
            k.p.key = key;
            k.p.key_len = key_len;
            k.p.seq = seq;
            k.p.seq_len = seq_len;
            k.p.cipher = alg;
            /* k.p.cipher = wpa_alg_to_cipher_suite(alg, key_len);*/
            if (!addr ||
                kalMemCmp(bcast_addr, addr, MAC_ADDR_LEN) == 0 ||
                alg == WPA_ALG_WEP
               )
                k.type = NL80211_KEYTYPE_GROUP;
            else
                k.type = NL80211_KEYTYPE_PAIRWISE;
            ret = mtk_freertos_wpa_add_key(key_idx,
                                           k.type == NL80211_KEYTYPE_PAIRWISE,
                                           addr, &k.p, bssidx);
        }
        if (ret != 0)
            return -WLAN_STATUS_FAILURE;
        ret = 0;
        ret = mtk_freertos_wpa_set_default_key(key_idx, k.def_uni,
                                               k.def_multi);
    }
    return WLAN_STATUS_SUCCESS;
}
#endif /* #ifdef MTK_MINISUPP_ENABLE */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for setting the rekey data
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_freertos_wpa_set_rekey_data(void *priv, const u8 *kek, size_t kek_len,
                                    const u8 *kck, size_t kck_len, const u8 *replay_ctr)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    struct PARAM_GTK_REKEY_DATA *prGtkData;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen;
    int i4Rslt = -EINVAL;

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    if (g_u4HaltFlag) {
        DBGLOG(RSN, WARN, "wlan is halt, skip key deletion\n");
        return WLAN_STATUS_FAILURE;
    }

    if (!kek || !kck || !replay_ctr)
        return -EINVAL;
    if ((kek_len != NL80211_KEK_LEN) || (kck_len != NL80211_KCK_LEN))
        return -ERANGE;

    /* if disable offload, we store key data here,
    * and enable rekey offload when enter wow.
    */
    if (!prGlueInfo->prAdapter->rWifiVar.ucEapolOffload) {

        DBGLOG(RSN, INFO, "mtk_freertos_set_rekey keep\n");
        kalMemZero(prGlueInfo->rWpaInfo[0].aucKek, NL80211_KEK_LEN);
        kalMemZero(prGlueInfo->rWpaInfo[0].aucKck, NL80211_KCK_LEN);
        kalMemZero(prGlueInfo->rWpaInfo[0].aucReplayCtr,
                   NL80211_REPLAY_CTR_LEN);
        kalMemCopy(prGlueInfo->rWpaInfo[0].aucKek, kek,
                   NL80211_KEK_LEN);
        kalMemCopy(prGlueInfo->rWpaInfo[0].aucKck, kck,
                   NL80211_KCK_LEN);
        kalMemCopy(prGlueInfo->rWpaInfo[0].aucReplayCtr,
                   replay_ctr, NL80211_REPLAY_CTR_LEN);
        if (prGlueInfo->prAdapter->fgIsWowSuspend)
            setRekeyOffloadEnterWow(prGlueInfo);
        return 0;
    }

    prGtkData =
        (struct PARAM_GTK_REKEY_DATA *)
        kalMemAlloc(sizeof(struct PARAM_GTK_REKEY_DATA),
                    VIR_MEM_TYPE);

    if (!prGtkData)
        return WLAN_STATUS_SUCCESS;

    DBGLOG(RSN, INFO, "kek\n");
    DBGLOG_MEM8(RSN, INFO, (uint8_t *)kek,
                NL80211_KEK_LEN);
    DBGLOG(RSN, INFO, "kck\n");
    DBGLOG_MEM8(RSN, INFO, (uint8_t *)kck,
                NL80211_KCK_LEN);
    DBGLOG(RSN, INFO, "replay count\n");
    DBGLOG_MEM8(RSN, INFO, (uint8_t *)replay_ctr,
                NL80211_REPLAY_CTR_LEN);

    kalMemCopy(prGtkData->aucKek, kek, NL80211_KEK_LEN);
    kalMemCopy(prGtkData->aucKck, kck, NL80211_KCK_LEN);
    kalMemCopy(prGtkData->aucReplayCtr, replay_ctr, NL80211_REPLAY_CTR_LEN);

    prGtkData->u4Proto = NL80211_WPA_VERSION_2;
    if (prGlueInfo->rWpaInfo[0].u4WpaVersion == IW_AUTH_WPA_VERSION_WPA)
        prGtkData->u4Proto = NL80211_WPA_VERSION_1;

    if (prGlueInfo->rWpaInfo[0].u4CipherPairwise == IW_AUTH_CIPHER_TKIP)
        prGtkData->u4PairwiseCipher = BIT(3);
    else if (prGlueInfo->rWpaInfo[0].u4CipherPairwise ==
             IW_AUTH_CIPHER_CCMP)
        prGtkData->u4PairwiseCipher = BIT(4);
    else {
        kalMemFree(prGtkData, VIR_MEM_TYPE,
                   sizeof(struct PARAM_GTK_REKEY_DATA));
        return WLAN_STATUS_SUCCESS;
    }

    if (prGlueInfo->rWpaInfo[0].u4CipherGroup == IW_AUTH_CIPHER_TKIP)
        prGtkData->u4GroupCipher    = BIT(3);
    else if (prGlueInfo->rWpaInfo[0].u4CipherGroup == IW_AUTH_CIPHER_CCMP)
        prGtkData->u4GroupCipher    = BIT(4);
    else {
        kalMemFree(prGtkData, VIR_MEM_TYPE,
                   sizeof(struct PARAM_GTK_REKEY_DATA));
        return WLAN_STATUS_SUCCESS;
    }

    prGtkData->u4KeyMgmt = prGlueInfo->rWpaInfo[0].u4KeyMgmt;
    prGtkData->u4MgmtGroupCipher = 0;

    prGtkData->ucRekeyMode = GTK_REKEY_CMD_MODE_OFFLOAD_ON;

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSetGtkRekeyData,
                       prGtkData, sizeof(struct PARAM_GTK_REKEY_DATA),
                       FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(INIT, INFO, "set GTK rekey data error:0x%x\n", rStatus);
    else
        i4Rslt = 0;

    kalMemFree(prGtkData, VIR_MEM_TYPE,
               sizeof(struct PARAM_GTK_REKEY_DATA));

    return i4Rslt;
}
#endif /* #if !CFG_SUPPORT_NO_SUPPLICANT_OPS */

#if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P)
void mtk_freertos_wpa_global_deinit(void *priv)
{
    int ret = 0;

    if (g_wait_drv_ready) {
        ret = xSemaphoreGive(g_wait_drv_ready);
        if (ret != TRUE)
            LOG_FUNC("give drv ready failed\r\n");
    } else
        LOG_FUNC("drv ready not initiated\r\n");
}

void *mtk_freertos_wpa_global_init(void *ctx)
{
    int ret = pdFALSE;

    if (!g_wait_drv_ready)
        g_wait_drv_ready = xSemaphoreCreateBinary();

    if (g_wait_drv_ready) {
        LOG_FUNC("wait drv ready\r\n");
        ret = xSemaphoreTake(g_wait_drv_ready, portMAX_DELAY);
        if (ret != TRUE)
            LOG_FUNC("take drv ready failed\r\n");
        else
            xSemaphoreGive(g_wait_drv_ready);
    }
    return g_prGlueInfo;
}
#endif /* #if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P) */

const uint8_t *mtk_cfg80211_find_ie_match_mask(uint8_t eid,
                                               const uint8_t *ies, int len, const uint8_t *match, int match_len,
                                               int match_offset, const uint8_t *match_mask)
{
    /* match_offset can't be smaller than 2, unless match_len is
     * zero, in which case match_offset must be zero as well.
     */
    while (len >= 2 && len >= ies[1] + 2) {
        if ((ies[0] == eid) &&
            (ies[1] + 2 >= match_offset + match_len) &&
            !kalMaskMemCmp(ies + match_offset,
                           match, match_mask, match_len))
            return ies;
        len -= ies[1] + 2;
        ies += ies[1] + 2;
    }
    return NULL;
}

int mtk_freertos_update_ft_ies(u16 mdid, const u8 *ies, size_t ies_len)
{
#if CFG_SUPPORT_802_11R
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
    struct cfg80211_update_ft_ies_params ftie;
    uint32_t u4InfoBufLen = 0;
    uint32_t rStatus = WLAN_STATUS_FAILURE;

    DBGLOG(REQ, INFO, "ft ies_len = %d\n", ies_len);

    ftie.md = mdid;
    ftie.ie = ies;
    ftie.ie_len = ies_len;
    rStatus = kalIoctl(prGlueInfo, wlanoidUpdateFtIes, (void *)&ftie,
                       sizeof(struct cfg80211_update_ft_ies_params), FALSE, FALSE, FALSE,
                       &u4InfoBufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(OID, INFO, "FT: update Ft IE failed\n");
#else /* #if CFG_SUPPORT_802_11R */
    DBGLOG(OID, INFO, "FT: 802.11R is not enabled\n");
#endif /* #if CFG_SUPPORT_802_11R */

    return 0;
}

int mtk_freertos_abort_scan(void)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen;

    prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    ASSERT(prGlueInfo);

    kalIoctl(prGlueInfo, wlanoidAisPreSuspend, NULL, 0,
             TRUE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(INIT, INFO, "mtk_freertos_abort_scan fail 0x%lx\n", rStatus);
        return -14;/* EFAULT; */
    } else {
        DBGLOG(INIT, INFO, "mtk_freertos_abort_scan done 0x%lx\n", rStatus);
        return 0;
    }
}

