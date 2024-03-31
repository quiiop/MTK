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
 *      /gl_init.c#11
 */

/*! \file   gl_init.c
 *    \brief  Main routines of Linux driver
 *
 *    This file contains the main routines of Linux driver for MediaTek Inc.
 *    802.11 Wireless LAN Adapters.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_hook_api.h"
#include "precomp.h"
#include "gl_init.h"
#include "gl_rst.h"

#if CFG_WLAN_LWIP_ZERO_COPY || CFG_WLAN_CACHE_HIT_DBG
#include "hal_cache.h"
#endif /* #if CFG_WLAN_LWIP_ZERO_COPY || CFG_WLAN_CACHE_HIT_DBG */
#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
#ifdef MTK_SLT_ENABLE
#include "slt_wifi.h"
#endif /* #ifdef MTK_SLT_ENABLE */

#if CFG_SUPPORT_ANT_DIV
#include "hal_gpio.h"
#endif /* #if CFG_SUPPORT_ANT_DIV */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
EventGroupHandle_t g_init_wait;
SemaphoreHandle_t g_halt_sem;
int g_u4HaltFlag;
uint8_t g_fgNvramAvailable;
uint8_t g_aucNvram[CFG_FILE_WIFI_REC_SIZE];

uint8_t g_first_boot = 1;
#if CFG_STATIC_MEM_ALLOC
/* In GlueInfo */
struct completion g_rScanComp;
struct completion g_rHaltComp;
struct completion g_rPendComp;
#if CFG_SUPPORT_MULTITHREAD
struct completion g_rHifHaltComp;
struct completion g_rRxHaltComp;
#endif /* #if CFG_SUPPORT_MULTITHREAD */
#if CFG_SUPPORT_NCHO
/* indicate Ais channel grant complete */
struct completion g_rAisChGrntComp;
#endif /* #if CFG_SUPPORT_NCHO */
SemaphoreHandle_t g_rSpinLock[SPIN_LOCK_NUM];
#if (configSUPPORT_STATIC_ALLOCATION == 1)
StaticSemaphore_t g_rStaticSpinLock[SPIN_LOCK_NUM];
#endif /* #if (configSUPPORT_STATIC_ALLOCATION == 1) */
SemaphoreHandle_t g_ioctl_sem;

uint8_t *g_pucRxCached;
uint8_t *g_pucTxCached;
uint8_t *g_pucCoalescingBufCached;
uint8_t *g_pucMgtBufCached;
WLAN_ATTR_ZIDATA_IN_MEM struct GLUE_INFO g_rGlueInfo;

#if CFG_PSRAM_ENABLE
ATTR_ZIDATA_IN_RAM struct ADAPTER g_rAdapter;
#else /* #if CFG_PSRAM_ENABLE */
ATTR_ZIDATA_IN_SYSRAM struct ADAPTER g_rAdapter;
#endif /* #if CFG_PSRAM_ENABLE */

struct net_device g_netdev;
struct wiphy g_rWiphy;
struct wireless_dev grWdev[KAL_AIS_NUM];
#endif /* #if CFG_STATIC_MEM_ALLOC */

/* Keep allocated memory not free */
struct GLUE_INFO *g_prGlueInfo;
struct ADAPTER *g_prAdpater;
uint8_t *g_pucSecBuf;


wlan_netif_input_fn wlan_ap_input;
wlan_netif_input_fn wlan_sta_input;

struct netif *wlan_sta_netif;
struct netif *wlan_ap_netif;


#if CFG_SUPPORT_SNIFFER
wlan_netif_input_fn wlan_sniffer_input;
struct netif *wlan_sniffer_netif;
#endif /* #if CFG_SUPPORT_SNIFFER */

struct wireless_dev *gprWdev[KAL_AIS_NUM];
struct wiphy *g_prWiphy;

u_int8_t g_fgArpOffloadStatus;

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
/* Tasklet mechanism is like buttom-half in Linux. We just want to
 * send a signal to OS for interrupt defer processing. All resources
 * are NOT allowed reentry, so txPacket, ISR-DPC and ioctl must avoid preempty.
 */
struct WLANDEV_INFO {
    struct net_device *prDev;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#ifdef MTK_SLT_ENABLE
int32_t g_i4SltConnsysWifiOn;
#endif /* #ifdef MTK_SLT_ENABLE */

#if DBG /* TODO: temp masked out */
uint32_t gu4DbgLevl = DBG_CLASS_MASK;
#else /* #if DBG ( TODO: temp masked out ) */
#ifdef CFG_DEFAULT_DBG_LEVEL
uint32_t gu4DbgLevl = CFG_DEFAULT_DBG_LEVEL;
#else /* #ifdef CFG_DEFAULT_DBG_LEVEL */
uint32_t gu4DbgLevl = DBG_LOG_LEVEL_DEFAULT;
#endif /* #ifdef CFG_DEFAULT_DBG_LEVEL */
#endif /* #if DBG ( TODO: temp masked out ) */

uint8_t aucDebugModule[DBG_MODULE_NUM];
uint32_t au4LogLevel[ENUM_WIFI_LOG_MODULE_NUM] = {ENUM_WIFI_LOG_LEVEL_DEFAULT};

#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
uint8_t g_fgIsCalDataBackuped = FALSE;
#endif /* #if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST */

#define CFG_EEPRM_FILENAME    "EEPROM"
#define FILE_NAME_MAX     64

#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
static uint8_t *apucEepromName[] = {
    (uint8_t *) CFG_EEPRM_FILENAME "_MT",
    NULL
};
#endif /* #if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1) */

extern struct DOMAIN_INFO_ENTRY defaultRegDomain;

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

#define CHAN2G(_channel, _freq, _flags)     \
{                       \
    .band               = KAL_BAND_2GHZ,    \
    .center_freq        = (_freq),      \
    .hw_value           = (_channel),   \
    .flags              = (_flags),     \
    .max_antenna_gain   = 0,        \
    .max_power          = 30,       \
}
static struct ieee80211_channel mtk_2ghz_channels[] = {
    CHAN2G(1, 2412, 0),
    CHAN2G(2, 2417, 0),
    CHAN2G(3, 2422, 0),
    CHAN2G(4, 2427, 0),
    CHAN2G(5, 2432, 0),
    CHAN2G(6, 2437, 0),
    CHAN2G(7, 2442, 0),
    CHAN2G(8, 2447, 0),
    CHAN2G(9, 2452, 0),
    CHAN2G(10, 2457, 0),
    CHAN2G(11, 2462, 0),
    CHAN2G(12, 2467, 0),
    CHAN2G(13, 2472, 0),
    CHAN2G(14, 2484, 0),
};

#define CHAN5G(_channel, _flags)                    \
{                                   \
    .band               = KAL_BAND_5GHZ,                \
    .center_freq        =                       \
        (((_channel >= 182) && (_channel <= 196)) ?     \
        (4000 + (5 * (_channel))) : (5000 + (5 * (_channel)))), \
    .hw_value           = (_channel),               \
    .flags              = (_flags),                 \
    .max_antenna_gain   = 0,                    \
    .max_power          = 30,                   \
}
static struct ieee80211_channel mtk_5ghz_channels[] = {
    /* UNII-1 */
    CHAN5G(36, 0),
    CHAN5G(40, 0),
    CHAN5G(44, 0),
    CHAN5G(48, 0),
    /* UNII-2 */
    CHAN5G(52, IEEE80211_CHAN_RADAR),
    CHAN5G(56, IEEE80211_CHAN_RADAR),
    CHAN5G(60, IEEE80211_CHAN_RADAR),
    CHAN5G(64, IEEE80211_CHAN_RADAR),
    /* UNII-2e */
    CHAN5G(100, IEEE80211_CHAN_RADAR),
    CHAN5G(104, IEEE80211_CHAN_RADAR),
    CHAN5G(108, IEEE80211_CHAN_RADAR),
    CHAN5G(112, IEEE80211_CHAN_RADAR),
    CHAN5G(116, IEEE80211_CHAN_RADAR),
    CHAN5G(120, IEEE80211_CHAN_RADAR),
    CHAN5G(124, IEEE80211_CHAN_RADAR),
    CHAN5G(128, IEEE80211_CHAN_RADAR),
    CHAN5G(132, IEEE80211_CHAN_RADAR),
    CHAN5G(136, IEEE80211_CHAN_RADAR),
    CHAN5G(140, IEEE80211_CHAN_RADAR),
    CHAN5G(144, IEEE80211_CHAN_RADAR),
    /* UNII-3 */
    CHAN5G(149, 0),
    CHAN5G(153, 0),
    CHAN5G(157, 0),
    CHAN5G(161, 0),
    CHAN5G(165, 0)
};

/* public for both Legacy Wi-Fi / P2P access */
struct ieee80211_supported_band mtk_band_2ghz = {
    .channels = mtk_2ghz_channels,
    .n_channels = ARRAY_SIZE(mtk_2ghz_channels),
};

/* public for both Legacy Wi-Fi / P2P access */
struct ieee80211_supported_band mtk_band_5ghz = {
    .channels = mtk_5ghz_channels,
    .n_channels = ARRAY_SIZE(mtk_5ghz_channels),
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */


/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, a primary SOCKET interface to configure
 *        the interface lively. Handle an ioctl call on one of our devices.
 *        Everything Linux ioctl specific is done here. Then we pass the
 *    contents of the ifr->data to the request message handler.
 *
 * \param[in] prDev      Linux kernel netdevice
 *
 * \param[in] prIFReq    Our private ioctl request structure, typed for the
 *           generic
 *                       struct ifreq so we can use ptr to function
 *
 * \param[in] cmd        Command ID
 *
 * \retval WLAN_STATUS_SUCCESS The IOCTL command is executed successfully.
 * \retval OTHER The execution of IOCTL command is failed.
 */
/*----------------------------------------------------------------------------*/
int wlanDoIOCTL(unsigned int i4Cmd, unsigned int  flag, void *data)

{
    return 0;
}               /* end of wlanDoIOCTL() */

/*----------------------------------------------------------------------------*/
/*
* \brief This function is TX entry point of NET DEVICE.
*
* \param[in] prSkb  Pointer of the sk_buff to be sent
* \param[in] prDev  Pointer to struct net_device
*
* \retval NETDEV_TX_OK - on success.
* \retval NETDEV_TX_BUSY - on failure, packet will be discarded by upper layer.
*/
/*----------------------------------------------------------------------------*/
int wlanHardStartXmit(struct pbuf *p, struct netif *netif, struct GLUE_INFO *prGlueInfo)
{
    struct pbuf *q;
    uint16_t total_len = 0;
    uint8_t  bss_idx = 0;
    int i = 0;
    struct pkt_buf *pkt = NULL;
    kalSLA_CustomLogging_Start_Label(SLA_LABEL_wlanHardStartXmit);

    for (bss_idx = 0; bss_idx < HW_BSSID_NUM; bss_idx++) {
        if (!prGlueInfo->arNetInterfaceInfo[bss_idx].pvNetInterface)
            continue;

        if (((struct net_device *)(prGlueInfo->arNetInterfaceInfo[bss_idx].pvNetInterface))->netif == netif)
            break;
    }

    if (bss_idx == HW_BSSID_NUM)
        return ERR_VAL;

    for (q = p; q != NULL; q = q->next) {
        total_len = total_len + (q->len);
        DBGLOG(TX, INFO, "add num %d, len %d, %p\n",
               i++, total_len, q->payload);
    }

    DBGLOG(TX, INFO, "> total packet size %d\n", total_len);
    kalSLA_CustomLogging_Start(SLA_LABEL_wlanHardStartXmit_alloc_internal_packet);

#if CFG_WLAN_LWIP_ZERO_COPY
#if CFG_WLAN_LWIP_ZERO_COPY_DBG
    DBGLOG(TX, INFO, "total_len %d p %p p->payload %p\n",
           total_len, p, p->payload);
#endif /* #if CFG_WLAN_LWIP_ZERO_COPY_DBG */
    if (!pbuf_header(p, NIC_TX_HEAD_ROOM))
        pkt = alloc_internal_packet(0, total_len, 0, p);
    else
        DBGLOG(TX, INFO, "GET HDR room failed\r\n");
#else /* #if CFG_WLAN_LWIP_ZERO_COPY */
    pkt = alloc_internal_packet(0, total_len, NIC_TX_HEAD_ROOM, p);
#endif /* #if CFG_WLAN_LWIP_ZERO_COPY */

    if (!pkt) {
        return ERR_MEM;
    }

    pbuf_ref(p);
    DBGLOG(TX, INFO, "> porting bss_idx of netif %d, len %d\n",
           bss_idx, total_len);
    kalSLA_CustomLogging_Stop(SLA_LABEL_wlanHardStartXmit_alloc_internal_packet);
    kalSLA_CustomLogging_Start(SLA_LABEL_wlanHardStartXmit_kalResetPacket);
    kalResetPacket(prGlueInfo, (void *) pkt);
    kalSLA_CustomLogging_Stop(SLA_LABEL_wlanHardStartXmit_kalResetPacket);

    if (kalHardStartXmit(pkt, netif, prGlueInfo, bss_idx)
        != WLAN_STATUS_SUCCESS) {
        if (pkt->pbuf)
            pbuf_free(pkt->pbuf);
        vfree(pkt);
        DBGLOG(TX, INFO, "> sent failed return ERR_MEM\n");
        return ERR_MEM;
    }
    kalSLA_CustomLogging_Stop(SLA_LABEL_wlanHardStartXmit);

    return 0;
}               /* end of wlanHardStartXmit() */

void wlanDebugInit(void)
{
    wlanSetDriverDbgLevel(DBG_ALL_MODULE_IDX, gu4DbgLevl);
    LOG_FUNC("%s Reset ALL DBG module log level to DEFAULT!\r\n", __func__);
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief Update Channel table for cfg80211 for Wi-Fi Direct based on current
 *        country code
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   none
 */
/*----------------------------------------------------------------------------*/
void wlanUpdateChannelTable(struct GLUE_INFO *prGlueInfo)
{
    uint8_t i, j;
    uint8_t ucNumOfChannel;
    struct RF_CHANNEL_INFO aucChannelList[ARRAY_SIZE(
                                              mtk_2ghz_channels) + ARRAY_SIZE(mtk_5ghz_channels)];

    /* 1. Disable all channels */
    for (i = 0; i < ARRAY_SIZE(mtk_2ghz_channels); i++) {
        mtk_2ghz_channels[i].flags |= IEEE80211_CHAN_DISABLED;
        mtk_2ghz_channels[i].orig_flags |= IEEE80211_CHAN_DISABLED;
    }

    for (i = 0; i < ARRAY_SIZE(mtk_5ghz_channels); i++) {
        mtk_5ghz_channels[i].flags |= IEEE80211_CHAN_DISABLED;
        mtk_5ghz_channels[i].orig_flags |= IEEE80211_CHAN_DISABLED;
    }

    /* 2. Get current domain channel list */
    rlmDomainGetChnlList(prGlueInfo->prAdapter,
                         BAND_NULL, FALSE,
                         ARRAY_SIZE(mtk_2ghz_channels) + ARRAY_SIZE(
                             mtk_5ghz_channels),
                         &ucNumOfChannel, aucChannelList);

    /* 3. Enable specific channel based on domain channel list */
    for (i = 0; i < ucNumOfChannel; i++) {
        switch (aucChannelList[i].eBand) {
            case BAND_2G4:
                for (j = 0; j < ARRAY_SIZE(mtk_2ghz_channels); j++) {
                    if (mtk_2ghz_channels[j].hw_value ==
                        aucChannelList[i].ucChannelNum) {
                        mtk_2ghz_channels[j].flags &=
                            ~IEEE80211_CHAN_DISABLED;
                        mtk_2ghz_channels[j].orig_flags &=
                            ~IEEE80211_CHAN_DISABLED;
                        break;
                    }
                }
                break;

            case BAND_5G:
                for (j = 0; j < ARRAY_SIZE(mtk_5ghz_channels); j++) {
                    if (mtk_5ghz_channels[j].hw_value ==
                        aucChannelList[i].ucChannelNum) {
                        mtk_5ghz_channels[j].flags &=
                            ~IEEE80211_CHAN_DISABLED;
                        mtk_5ghz_channels[j].orig_flags &=
                            ~IEEE80211_CHAN_DISABLED;
                        break;
                    }
                }
                break;

            default:
                DBGLOG(INIT, WARN, "Unknown band %d\n",
                       aucChannelList[i].eBand);
                break;
        }
    }

}

#if CFG_SUPPORT_SAP_DFS_CHANNEL
static uint8_t wlanIsAdjacentChnl(struct GL_P2P_INFO *prGlueP2pInfo,
                                  uint32_t u4CenterFreq, uint8_t ucBandWidth,
                                  enum ENUM_CHNL_EXT eBssSCO, uint8_t ucAdjacentChannel)
{
    uint32_t u4AdjacentFreq = 0;
    uint32_t u4BandWidth = 20;
    uint32_t u4StartFreq, u4EndFreq;
    struct ieee80211_channel *chnl = NULL;

    u4AdjacentFreq = nicChannelNum2Freq(ucAdjacentChannel) / 1000;

    DBGLOG(INIT, TRACE,
           "p2p: %p, center_freq: %d, bw: %d, sco: %d, ad_freq: %d",
           prGlueP2pInfo, u4CenterFreq, ucBandWidth, eBssSCO,
           u4AdjacentFreq);

    if (!prGlueP2pInfo)
        return FALSE;

    if (ucBandWidth == VHT_OP_CHANNEL_WIDTH_20_40 &&
        eBssSCO == CHNL_EXT_SCN)
        return FALSE;

    if (!u4CenterFreq)
        return FALSE;

    if (!u4AdjacentFreq)
        return FALSE;

    switch (ucBandWidth) {
        case VHT_OP_CHANNEL_WIDTH_20_40:
            u4BandWidth = 40;
            break;
        case VHT_OP_CHANNEL_WIDTH_80:
            u4BandWidth = 80;
            break;
        default:
            DBGLOG(INIT, WARN, "unsupported bandwidth: %d", ucBandWidth);
            return FALSE;
    }
    u4StartFreq = u4CenterFreq - u4BandWidth / 2 + 10;
    u4EndFreq = u4CenterFreq + u4BandWidth / 2 - 10;
    DBGLOG(INIT, TRACE, "bw: %d, s_freq: %d, e_freq: %d",
           u4BandWidth, u4StartFreq, u4EndFreq);
    if (u4AdjacentFreq < u4StartFreq || u4AdjacentFreq > u4EndFreq)
        return FALSE;

    /* check valid channel */
    chnl = ieee80211_get_channel(prGlueP2pInfo->prWdev->wiphy,
                                 u4AdjacentFreq);
    if (!chnl) {
        DBGLOG(INIT, WARN, "invalid channel for freq: %d",
               u4AdjacentFreq);
        return FALSE;
    }
    return TRUE;
}

void wlanUpdateDfsChannelTable(struct GLUE_INFO *prGlueInfo,
                               uint8_t ucRoleIdx, uint8_t ucChannel, uint8_t ucBandWidth,
                               enum ENUM_CHNL_EXT eBssSCO, uint32_t u4CenterFreq)
{
    struct GL_P2P_INFO *prGlueP2pInfo = NULL;
    uint8_t i, j;
    uint8_t ucNumOfChannel;
    struct RF_CHANNEL_INFO aucChannelList[ARRAY_SIZE(mtk_5ghz_channels)];

    DBGLOG(INIT, INFO, "r: %d, chnl %u, b: %d, s: %d, freq: %d\n",
           ucRoleIdx, ucChannel, ucBandWidth, eBssSCO,
           u4CenterFreq);

    /* 1. Get current domain DFS channel list */
    rlmDomainGetDfsChnls(prGlueInfo->prAdapter,
                         ARRAY_SIZE(mtk_5ghz_channels),
                         &ucNumOfChannel, aucChannelList);

    if (ucRoleIdx >= 0 && ucRoleIdx < KAL_P2P_NUM)
        prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

    /* 2. Enable specific channel based on domain channel list */
    for (i = 0; i < ucNumOfChannel; i++) {
        for (j = 0; j < ARRAY_SIZE(mtk_5ghz_channels); j++) {
            if (aucChannelList[i].ucChannelNum !=
                mtk_5ghz_channels[j].hw_value)
                continue;

            if ((aucChannelList[i].ucChannelNum == ucChannel) ||
                wlanIsAdjacentChnl(prGlueP2pInfo,
                                   u4CenterFreq,
                                   ucBandWidth,
                                   eBssSCO,
                                   aucChannelList[i].ucChannelNum)) {
                mtk_5ghz_channels[j].dfs_state
                    = NL80211_DFS_AVAILABLE;
                mtk_5ghz_channels[j].flags &=
                    ~IEEE80211_CHAN_RADAR;
                mtk_5ghz_channels[j].orig_flags &=
                    ~IEEE80211_CHAN_RADAR;
                DBGLOG(INIT, INFO,
                       "ch (%d), force NL80211_DFS_AVAILABLE.\n",
                       aucChannelList[i].ucChannelNum);
            } else {
                mtk_5ghz_channels[j].dfs_state
                    = NL80211_DFS_USABLE;
                mtk_5ghz_channels[j].flags |=
                    IEEE80211_CHAN_RADAR;
                mtk_5ghz_channels[j].orig_flags |=
                    IEEE80211_CHAN_RADAR;
                DBGLOG(INIT, TRACE,
                       "ch (%d), force NL80211_DFS_USABLE.\n",
                       aucChannelList[i].ucChannelNum);
            }
        }
    }
}
#endif /* #if CFG_SUPPORT_SAP_DFS_CHANNEL */

static struct ADAPTER *wlanNetCreate(void *pvData, void *pvDriverData)
{
    struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)pvData;
    struct ADAPTER *prAdapter = NULL;
    uint32_t i;
    struct connsys_wf_hif_op *hif_op =
        (struct connsys_wf_hif_op *)pvDriverData;
    struct mt66xx_chip_info *prChipInfo;
    struct net_device *net_device = NULL;

    if (!pvData || !pvDriverData)
        return prAdapter;

    kalMemZero(prGlueInfo, sizeof(struct GLUE_INFO));

#if CFG_STATIC_MEM_ALLOC
    prGlueInfo->prDevHandler = &g_netdev;
#else /* #if CFG_STATIC_MEM_ALLOC */
    prGlueInfo->prDevHandler = kmalloc(sizeof(struct net_device));
#endif /* #if CFG_STATIC_MEM_ALLOC */

    if (prGlueInfo->prDevHandler == NULL)
        DBGLOG(INIT, ERROR, "Alloc net device failed\n");

    net_device = prGlueInfo->prDevHandler;

    hif_op = (struct connsys_wf_hif_op *)pvDriverData;
    prChipInfo = ((struct mt66xx_hif_driver_data *)
                  hif_op->driver_data)->chip_info;

    /*  Create Adapter structure */
    prAdapter = (struct ADAPTER *)wlanAdapterCreate(prGlueInfo);
    if (!prAdapter) {
        DBGLOG(INIT, ERROR,
               "Allocating memory to adapter failed\n");
        glClearHifInfo(prGlueInfo);
        goto netcreate_err;
    }

    prAdapter->chip_info = prChipInfo;
    prGlueInfo->prAdapter = prAdapter;
    /* 4 <3> Initialize Glue structure */
    /* 4 <3.1> Create net device */
    net_device->netif = wlan_sta_netif;
    net_device->netif_rxcb = wlan_sta_input;
    net_device->gl_info = prGlueInfo;
    if (!prGlueInfo->prDevHandler->netif || !prGlueInfo->prDevHandler->netif_rxcb) {
        DBGLOG(INIT, ERROR, "get netif/rxcb from lwip failed\n");
        goto netcreate_err;
    }

    prGlueInfo->ePowerState = ParamDeviceStateD0;
    prGlueInfo->fgIsRegistered = FALSE;
    prGlueInfo->prScanRequest = NULL;

#if CFG_SUPPORT_PASSPOINT
    /* Init DAD */
    prGlueInfo->fgIsDad = FALSE;
    prGlueInfo->fgIs6Dad = FALSE;
    kalMemZero(prGlueInfo->aucDADipv4, 4);
    kalMemZero(prGlueInfo->aucDADipv6, 16);
#endif /* #if CFG_SUPPORT_PASSPOINT */

#if CONFIG_WIFI_MEM_DBG
    LOG_FUNC("st semaphore free heap(%d)\n", xPortGetFreeHeapSize());
#endif /* #if CONFIG_WIFI_MEM_DBG */

#if CFG_STATIC_MEM_ALLOC
    if (g_first_boot) {
        init_completion(&g_rScanComp);
        init_completion(&g_rHaltComp);
        init_completion(&g_rPendComp);
#if CFG_SUPPORT_MULTITHREAD
        init_completion(&g_rHifHaltComp);
        init_completion(&g_rRxHaltComp);
#endif /* #if CFG_SUPPORT_MULTITHREAD */
#if CFG_SUPPORT_NCHO
        init_completion(&g_rAisChGrntComp);
#endif /* #if CFG_SUPPORT_NCHO */
    }
    dup_completion(&g_rScanComp, &prGlueInfo->rScanComp);
    dup_completion(&g_rHaltComp, &prGlueInfo->rHaltComp);
    dup_completion(&g_rPendComp, &prGlueInfo->rPendComp);
#if CFG_SUPPORT_MULTITHREAD
    dup_completion(&g_rHifHaltComp, &prGlueInfo->rHifHaltComp);
    dup_completion(&g_rRxHaltComp, &prGlueInfo->rRxHaltComp);
#endif /* #if CFG_SUPPORT_MULTITHREAD */
#if CFG_SUPPORT_NCHO
    dup_completion(&g_rAisChGrntComp, &prGlueInfo->rAisChGrntComp);
#endif /* #if CFG_SUPPORT_NCHO */
#else /* #if CFG_STATIC_MEM_ALLOC */
    init_completion(&prGlueInfo->rScanComp);
    init_completion(&prGlueInfo->rHaltComp);
    init_completion(&prGlueInfo->rPendComp);

#if CFG_SUPPORT_MULTITHREAD
    init_completion(&prGlueInfo->rHifHaltComp);
    init_completion(&prGlueInfo->rRxHaltComp);
#endif /* #if CFG_SUPPORT_MULTITHREAD */

#if CFG_SUPPORT_NCHO
    init_completion(&prGlueInfo->rAisChGrntComp);
#endif /* #if CFG_SUPPORT_NCHO */
#endif /* #if CFG_STATIC_MEM_ALLOC */

    /* initialize timer for OID timeout checker */
    kalOsTimerInitialize(prGlueInfo, kalTimeoutHandler);

    for (i = 0; i < SPIN_LOCK_NUM; i++) {
#if CFG_STATIC_MEM_ALLOC
        if (!g_rSpinLock[i]) {
#endif /* #if CFG_STATIC_MEM_ALLOC */

#if (configSUPPORT_STATIC_ALLOCATION == 1)
            prGlueInfo->rSpinLock[i] =
                xSemaphoreCreateMutexStatic(&g_rStaticSpinLock[i]);
#else /* #if (configSUPPORT_STATIC_ALLOCATION == 1) */
            prGlueInfo->rSpinLock[i] = xSemaphoreCreateMutex();
#endif /* #if (configSUPPORT_STATIC_ALLOCATION == 1) */
            if (!prGlueInfo->rSpinLock[i]) {
                DBGLOG(INIT, ERROR, "cannot get mutex %d as spinlock\n", i);
            }
#if CFG_STATIC_MEM_ALLOC
            g_rSpinLock[i] = prGlueInfo->rSpinLock[i];
        } else
            prGlueInfo->rSpinLock[i] = g_rSpinLock[i];
#endif /* #if CFG_STATIC_MEM_ALLOC */
    }

#if CFG_STATIC_MEM_ALLOC
    if (!g_ioctl_sem) {
#endif /* #if CFG_STATIC_MEM_ALLOC */
        /* initialize semaphore for ioctl */
        prGlueInfo->ioctl_sem = xSemaphoreCreateMutex();
#if CFG_STATIC_MEM_ALLOC
        g_ioctl_sem = prGlueInfo->ioctl_sem;
    } else
        prGlueInfo->ioctl_sem = g_ioctl_sem;
#endif /* #if CFG_STATIC_MEM_ALLOC */

    /* initialize semaphore for halt control */
    if (!g_halt_sem)
        g_halt_sem = xSemaphoreCreateMutex();

#if CONFIG_WIFI_MEM_DBG
    LOG_FUNC("end semaphore free heap(%d)\n", xPortGetFreeHeapSize());
#endif /* #if CONFIG_WIFI_MEM_DBG */

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
    /* initialize SDIO read-write pattern control */
    prGlueInfo->fgEnSdioTestPattern = FALSE;
    prGlueInfo->fgIsSdioTestInitialized = FALSE;
#endif /* #if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN */
    QUEUE_INITIALIZE(&prGlueInfo->rCmdQueue);
    prGlueInfo->i4TxPendingCmdNum = 0;
    QUEUE_INITIALIZE(&prGlueInfo->rTxQueue);
    glSetHifInfo(prGlueInfo, (unsigned long)NULL);

    /* main thread is created in this function */
#if CFG_SUPPORT_MULTITHREAD
    init_waitqueue_head(&prGlueInfo->waitq_rx);
    init_waitqueue_head(&prGlueInfo->waitq_hif);

    prGlueInfo->u4TxThreadPid = 0xffffffff;
    prGlueInfo->u4RxThreadPid = 0xffffffff;
    prGlueInfo->u4HifThreadPid = 0xffffffff;
#endif /* #if CFG_SUPPORT_MULTITHREAD */

    return prAdapter;

netcreate_err:
    if (prAdapter != NULL) {
        wlanAdapterDestroy(prAdapter);
        prAdapter = NULL;
    }
    prGlueInfo->prDevHandler = NULL;

    return prAdapter;
}               /* end of wlanNetCreate() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Destroying the struct net_device object and the private data.
 *
 * \param[in] prWdev      Pointer to struct wireless_dev.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static void wlanNetDestroy(void *prWdev)
{
    struct GLUE_INFO *prGlueInfo = NULL;

    ASSERT(prWdev);

    if (!prWdev) {
        DBGLOG(INIT, ERROR, "The device context is NULL\n");
        return;
    }

    /* prGlueInfo is allocated with net_device */
    prGlueInfo = (struct GLUE_INFO *)prWdev;
    ASSERT(prGlueInfo);

    /* destroy kal OS timer */
    kalCancelTimer(prGlueInfo);

    glClearHifInfo(prGlueInfo);

#if CFG_STATIC_MEM_ALLOC
#else /* #if CFG_STATIC_MEM_ALLOC */
    kalMemFree(prGlueInfo->prDevHandler,
               PHY_MEM_TYPE, sizeof(struct net_device));
#endif /* #if CFG_STATIC_MEM_ALLOC */

    wlanAdapterDestroy(prGlueInfo->prAdapter);
    prGlueInfo->prAdapter = NULL;

}               /* end of wlanNetDestroy() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Register the device to the kernel and return the index.
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0     The execution of wlanNetRegister succeeds.
 * \retval < 0   The execution of wlanNetRegister failed.
 */
/*----------------------------------------------------------------------------*/
static int wlanNetRegister(struct GLUE_INFO *prGlueInfo)
{
    int ret = 0;
    struct net_device *device_handler = prGlueInfo->prDevHandler;
    do {
        ret = (device_handler->netif) ? 0 :  -1;
        if (ret < 0) {
            DBGLOG(INIT, ERROR, "No netif in device handler\n");
            break;
        }

        device_handler->bss_idx = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
        wlanBindBssIdxToNetInterface(prGlueInfo,
                                     prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex, (void *) device_handler);
        if (ret != -1)
            prGlueInfo->fgIsRegistered = TRUE;
    } while (FALSE);

    return ret; /* success */
}               /* end of wlanNetRegister() */



/*----------------------------------------------------------------------------*/
/*!
 * \brief Unregister the device from the kernel
 *
 * \param[in] prWdev      Pointer to struct net_device.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static void wlanNetUnregister(struct GLUE_INFO *prGlueInfo)
{
    prGlueInfo->fgIsRegistered = FALSE;
}               /* end of wlanNetUnregister() */

void wlanSetSuspendMode(struct GLUE_INFO *prGlueInfo,
                        uint8_t fgEnable)
{
    struct net_device *prDev = NULL;
    uint32_t u4PacketFilter = 0;
    uint32_t u4SetInfoLen = 0;
    uint32_t u4Idx = 0;

    if (!prGlueInfo)
        return;


    for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
        prDev = wlanGetNetDev(prGlueInfo, u4Idx);
        if (!prDev)
            continue;

        /* new filter should not include p2p mask */
        /* hard-code in wlanAdapterStart */
        u4PacketFilter =
            prGlueInfo->prAdapter->u4OsPacketFilter &
            (~PARAM_PACKET_FILTER_P2P_MASK);

        if (kalIoctl(prGlueInfo,
                     wlanoidSetCurrentPacketFilter,
                     &u4PacketFilter,
                     sizeof(u4PacketFilter), FALSE, FALSE, TRUE,
                     &u4SetInfoLen) != WLAN_STATUS_SUCCESS)
            DBGLOG(INIT, ERROR, "set packet filter failed.\n");
        /* To Be Implement: get all MC address and not drop all MC packet */

        if (g_fgArpOffloadStatus != prGlueInfo->prAdapter->rWifiVar.ucArpOffload) {
            kalSetNetAddressFromInterface(prGlueInfo, prDev,
                                          prGlueInfo->prAdapter->rWifiVar.ucArpOffload);
            g_fgArpOffloadStatus = prGlueInfo->prAdapter->rWifiVar.ucArpOffload;
        }

        wlanNotifyFwSuspend(prGlueInfo, prDev, fgEnable);
    }
}

#if CFG_SUPPORT_CFG
void wlanGetConfig(struct ADAPTER *prAdapter)
{
#if CFG_SUPPORT_CFG_FILE
    uint8_t *pucConfigBuf;
    uint32_t u4ConfigReadLen;
#ifndef CFG_WIFI_CFG_FN
#define WIFI_CFG_FN "wifi.cfg"
#else /* #ifndef CFG_WIFI_CFG_FN */
#define WIFI_CFG_FN CFG_WIFI_CFG_FN
#endif /* #ifndef CFG_WIFI_CFG_FN */
    wlanCfgInit(prAdapter, NULL, 0, 0);
    pucConfigBuf = (uint8_t *) kalMemAlloc(
                       WLAN_CFG_FILE_BUF_SIZE, VIR_MEM_TYPE);
    kalMemZero(pucConfigBuf, WLAN_CFG_FILE_BUF_SIZE);
    u4ConfigReadLen = 0;
    if (pucConfigBuf) {
        if (kalRequestFirmware(WIFI_CFG_FN, pucConfigBuf,
                               WLAN_CFG_FILE_BUF_SIZE, &u4ConfigReadLen,
                               prAdapter->prGlueInfo->prDev) == 0) {
            /* ToDo:: Nothing */
        } else if (kalReadToFile("/data/misc/wifi/wifi.cfg",
                                 pucConfigBuf, WLAN_CFG_FILE_BUF_SIZE,
                                 &u4ConfigReadLen) == 0) {
            /* ToDo:: Nothing */
        } else if (kalReadToFile("/storage/sdcard0/wifi.cfg",
                                 pucConfigBuf, WLAN_CFG_FILE_BUF_SIZE,
                                 &u4ConfigReadLen) == 0) {
            /* ToDo:: Nothing */
        }

        if (pucConfigBuf[0] != '\0' && u4ConfigReadLen > 0)
            wlanCfgInit(prAdapter,
                        pucConfigBuf, u4ConfigReadLen, 0);

        kalMemFree(pucConfigBuf, VIR_MEM_TYPE,
                   WLAN_CFG_FILE_BUF_SIZE);
    }           /* pucConfigBuf */
#else /* #if CFG_SUPPORT_CFG_FILE */
    wlanCfgInit(prAdapter, NULL, 0, 0);
#endif /* #if CFG_SUPPORT_CFG_FILE */
}
#endif /* #if CFG_SUPPORT_CFG */
#if CFG_SUPPORT_CFG_FILE
#if CFG_SUPPORT_EASY_DEBUG
/*----------------------------------------------------------------------------*/
/*!
 * \brief parse config from wifi.cfg
 *
 * \param[in] prAdapter
 *
 * \retval VOID
 */
/*----------------------------------------------------------------------------*/
void wlanGetParseConfig(struct ADAPTER *prAdapter)
{
    uint8_t *pucConfigBuf;
    uint32_t u4ConfigReadLen;

    wlanCfgInit(prAdapter, NULL, 0, 0);
    pucConfigBuf = (uint8_t *) kmalloc(
                       WLAN_CFG_FILE_BUF_SIZE);
    kalMemZero(pucConfigBuf, WLAN_CFG_FILE_BUF_SIZE);
    u4ConfigReadLen = 0;
    if (pucConfigBuf) {
        if (kalRequestFirmware("wifi.cfg", pucConfigBuf,
                               WLAN_CFG_FILE_BUF_SIZE, &u4ConfigReadLen,
                               prAdapter->prGlueInfo->prDev) == 0) {
            /* ToDo:: Nothing */
        } else if (kalReadToFile("/data/misc/wifi.cfg",
                                 pucConfigBuf, WLAN_CFG_FILE_BUF_SIZE,
                                 &u4ConfigReadLen) == 0) {
            /* ToDo:: Nothing */
        } else if (kalReadToFile("/data/misc/wifi/wifi.cfg",
                                 pucConfigBuf, WLAN_CFG_FILE_BUF_SIZE,
                                 &u4ConfigReadLen) == 0) {
            /* ToDo:: Nothing */
        } else if (kalReadToFile("/storage/sdcard0/wifi.cfg",
                                 pucConfigBuf, WLAN_CFG_FILE_BUF_SIZE,
                                 &u4ConfigReadLen) == 0) {
            /* ToDo:: Nothing */
        }

        if (pucConfigBuf[0] != '\0' && u4ConfigReadLen > 0)
            wlanCfgParse(prAdapter, pucConfigBuf, u4ConfigReadLen,
                         TRUE);

        kalMemFree(pucConfigBuf, VIR_MEM_TYPE,
                   WLAN_CFG_FILE_BUF_SIZE);
    }           /* pucConfigBuf */
}


#endif /* #if CFG_SUPPORT_EASY_DEBUG */

/*----------------------------------------------------------------------------*/
/*!
 * \brief get config from wifi.cfg
 *
 * \param[in] prAdapter
 *
 * \retval VOID
 */
/*----------------------------------------------------------------------------*/

#endif /* #if CFG_SUPPORT_CFG_FILE */


#if CFG_SUPPORT_QA_TOOL == 0 && (CFG_SUPPORT_BUFFER_MODE_HDR == 0)
/* if not reference chip default array */
uint8_t uacEEPROMImage[MAX_EEPROM_BUFFER_SIZE];
#endif /* #if CFG_SUPPORT_QA_TOOL == 0 && (CFG_SUPPORT_BUFFER_MODE_HDR == 0) */
/*----------------------------------------------------------------------------*/
/*!
 * \brief this function send buffer bin EEPROB_MTxxxx.bin to FW.
 *
 * \param[in] prAdapter
 *
 * \retval WLAN_STATUS_SUCCESS Success
 * \retval WLAN_STATUS_FAILURE Failed
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanDownloadBufferBin(struct ADAPTER *prAdapter)
{
    struct GLUE_INFO *prGlueInfo = NULL;
#if (CFG_FW_Report_Efuse_Address)
    uint16_t u2InitAddr = prAdapter->u4EfuseStartAddress;
#else /* #if (CFG_FW_Report_Efuse_Address) */
    uint16_t u2InitAddr = EFUSE_CONTENT_BUFFER_START;
#endif /* #if (CFG_FW_Report_Efuse_Address) */
    uint32_t u4BufLen = 0;
    struct PARAM_CUSTOM_EFUSE_BUFFER_MODE *prSetEfuseBufModeInfo
            = NULL;
    uint32_t u4ContentLen;
    uint8_t *pucConfigBuf = NULL;
    struct mt66xx_chip_info *prChipInfo;
    uint32_t chip_id;
    uint8_t aucEeprom[32];
    uint32_t retWlanStat = WLAN_STATUS_FAILURE;
    uint32_t ret;
#if CFG_EFUSE_AUTO_MODE_SUPPORT
    uint32_t u4Efuse_addr = 0;
    struct PARAM_CUSTOM_ACCESS_EFUSE *prAccessEfuseInfo
            = NULL;
#endif /* #if CFG_EFUSE_AUTO_MODE_SUPPORT */
#if (CFG_SUPPORT_BUFFER_MODE) && (CFG_SUPPORT_BUFFER_MODE_HDR == 1)
    uint16_t uEEPROM_BUFFER_LIMIT = u4LenEEPROMImage;
#else /* #if (CFG_SUPPORT_BUFFER_MODE) && (CFG_SUPPORT_BUFFER_MODE_HDR == 1) */
    uint16_t uEEPROM_BUFFER_LIMIT = MAX_EEPROM_BUFFER_SIZE;
#endif /* #if (CFG_SUPPORT_BUFFER_MODE) && (CFG_SUPPORT_BUFFER_MODE_HDR == 1) */

    if (prAdapter->fgIsSupportPowerOnSendBufferModeCMD ==
        TRUE) {
        DBGLOG(INIT, INFO, "Start Efuse Buffer Mode ..\n");
        DBGLOG(INIT, INFO, "ucEfuseBUfferModeCal is %x\n",
               prAdapter->rWifiVar.ucEfuseBufferModeCal);

        prChipInfo = prAdapter->chip_info;
        chip_id = prChipInfo->chip_id;
        prGlueInfo = prAdapter->prGlueInfo;
        /*
                if (prGlueInfo == NULL || prGlueInfo->prDev == NULL)
                    goto label_exit;
        */
        /* allocate memory for buffer mode info */
        prSetEfuseBufModeInfo =
            (struct PARAM_CUSTOM_EFUSE_BUFFER_MODE *)
            kmalloc(sizeof(
                        struct PARAM_CUSTOM_EFUSE_BUFFER_MODE));
        if (prSetEfuseBufModeInfo == NULL)
            goto label_exit;
        kalMemZero(prSetEfuseBufModeInfo,
                   sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE));
#if CFG_EFUSE_AUTO_MODE_SUPPORT
        /* allocate memory for Access Efuse Info */
        prAccessEfuseInfo =
            (struct PARAM_CUSTOM_ACCESS_EFUSE *)
            kalMemAlloc(sizeof(
                            struct PARAM_CUSTOM_ACCESS_EFUSE),
                        VIR_MEM_TYPE);
        if (prAccessEfuseInfo == NULL)
            goto label_exit;
        kalMemZero(prAccessEfuseInfo,
                   sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));

        if (prAdapter->rWifiVar.ucEfuseBufferModeCal == LOAD_AUTO) {
            prAccessEfuseInfo->u4Address = (u4Efuse_addr /
                                            EFUSE_BLOCK_SIZE) * EFUSE_BLOCK_SIZE;
            kalIoctl(prGlueInfo,
                     wlanoidQueryProcessAccessEfuseRead,
                     prAccessEfuseInfo,
                     sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE),
                     TRUE, TRUE, TRUE, &u4BufLen);
            if (prGlueInfo->prAdapter->aucEepromVaule[1]
                == (chip_id >> 8)) {
                prAdapter->rWifiVar.ucEfuseBufferModeCal
                    = LOAD_EFUSE;
                DBGLOG(INIT, STATE,
                       "[EFUSE AUTO] EFUSE Mode\n");
            } else {
                prAdapter->rWifiVar.ucEfuseBufferModeCal
                    = LOAD_EEPROM_BIN;
                DBGLOG(INIT, STATE,
                       "[EFUSE AUTO] Buffer Mode\n");
            }
        }
#endif /* #if CFG_EFUSE_AUTO_MODE_SUPPORT */
        if (prAdapter->rWifiVar.ucEfuseBufferModeCal
            == LOAD_EEPROM_BIN) {
            /* Buffer mode */
            /* Only in buffer mode need to access bin file */
            /* 1 <1> Load bin file*/
            pucConfigBuf = (uint8_t *) kmalloc(
                               uEEPROM_BUFFER_LIMIT);
            if (pucConfigBuf == NULL)
                goto label_exit;

            kalMemZero(pucConfigBuf, uEEPROM_BUFFER_LIMIT);

            /* 1 <2> Construct EEPROM binary name */
            kalMemZero(aucEeprom, sizeof(aucEeprom));

            ret = kalSnprintf(aucEeprom,
                              sizeof(aucEeprom), "%s%x.bin",
                              apucEepromName[0], chip_id);
            if (ret == 0 || ret >= sizeof(aucEeprom)) {
                DBGLOG(INIT, ERROR,
                       "[%u] snprintf failed, ret: %d\n",
                       __LINE__, ret);
                goto label_exit;
            }

            /* 1 <3> Request buffer bin */
            if (kalRequestFirmware(aucEeprom, pucConfigBuf,
                                   uEEPROM_BUFFER_LIMIT, &u4ContentLen,
                                   NULL) == 0) {
                DBGLOG(INIT, INFO, "request file done\n");
            } else {
                DBGLOG(INIT, INFO, "can't find file\n");
                goto label_exit;
            }

            /* 1 <4> Send CMD with bin file content */
            prGlueInfo = prAdapter->prGlueInfo;

            /* Update contents in local table */
            kalMemCopy(uacEEPROMImage, pucConfigBuf,
                       uEEPROM_BUFFER_LIMIT);

            /* copy to the command buffer */
#if (CFG_FW_Report_Efuse_Address)
            u4ContentLen = (prAdapter->u4EfuseEndAddress) -
                           (prAdapter->u4EfuseStartAddress) + 1;
#else /* #if (CFG_FW_Report_Efuse_Address) */
            u4ContentLen = EFUSE_CONTENT_BUFFER_SIZE;
#endif /* #if (CFG_FW_Report_Efuse_Address) */
            if (u4ContentLen > uEEPROM_BUFFER_LIMIT)
                goto label_exit;
            kalMemCopy(prSetEfuseBufModeInfo->aBinContent,
                       &pucConfigBuf[u2InitAddr], u4ContentLen);

            prSetEfuseBufModeInfo->ucSourceMode = 1;
        } else {
            /* eFuse mode */
            /* Only need to tell FW the content from, contents are
             * directly from efuse
             */
            prSetEfuseBufModeInfo->ucSourceMode = 0;
        }
        prSetEfuseBufModeInfo->ucCmdType = 0x1 |
                                           (prAdapter->rWifiVar.ucCalTimingCtrl << 4);
        prSetEfuseBufModeInfo->ucCount   =
            0xFF; /* ucCmdType 1 don't care the ucCount */

        retWlanStat = kalIoctl(prGlueInfo, wlanoidSetEfusBufferMode,
                               (void *)prSetEfuseBufModeInfo,
                               sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE),
                               FALSE, TRUE, TRUE, &u4BufLen);
    }

    retWlanStat = WLAN_STATUS_SUCCESS;

label_exit:

    /* free memory */
    if (prSetEfuseBufModeInfo != NULL)
        kalMemFree(prSetEfuseBufModeInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE));

    if (pucConfigBuf != NULL)
        kalMemFree(pucConfigBuf, VIR_MEM_TYPE,
                   uEEPROM_BUFFER_LIMIT);
#if CFG_EFUSE_AUTO_MODE_SUPPORT
    if (prAccessEfuseInfo != NULL)
        kalMemFree(prAccessEfuseInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));
#endif /* #if CFG_EFUSE_AUTO_MODE_SUPPORT */

    return retWlanStat;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief this function send buffer bin EEPROB_MTxxxx.bin to FW.
 *
 * \param[in] prAdapter
 *
 * \retval WLAN_STATUS_SUCCESS Success
 * \retval WLAN_STATUS_FAILURE Failed
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanConnacDownloadBufferBin(struct ADAPTER
                                     *prAdapter)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t u4BufLen = 0;
    struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T
        *prSetEfuseBufModeInfo = NULL;
    uint32_t u4ContentLen;
    uint8_t *pucConfigBuf = NULL;
    struct mt66xx_chip_info *prChipInfo;
    uint32_t chip_id;
    uint8_t aucEeprom[32];
    uint32_t retWlanStat = WLAN_STATUS_FAILURE;
    uint32_t ret;
#if CFG_EFUSE_AUTO_MODE_SUPPORT
    uint32_t u4Efuse_addr = 0;
    struct PARAM_CUSTOM_ACCESS_EFUSE *prAccessEfuseInfo
            = NULL;
#endif /* #if CFG_EFUSE_AUTO_MODE_SUPPORT */
#if (CFG_SUPPORT_BUFFER_MODE) && (CFG_SUPPORT_BUFFER_MODE_HDR == 1)
    uint16_t uEEPROM_BUFFER_LIMIT = u4LenEEPROMImage;
#else /* #if (CFG_SUPPORT_BUFFER_MODE) && (CFG_SUPPORT_BUFFER_MODE_HDR == 1) */
    uint16_t uEEPROM_BUFFER_LIMIT = MAX_EEPROM_BUFFER_SIZE;
#endif /* #if (CFG_SUPPORT_BUFFER_MODE) && (CFG_SUPPORT_BUFFER_MODE_HDR == 1) */

    if (prAdapter->fgIsSupportPowerOnSendBufferModeCMD == FALSE)
        return WLAN_STATUS_SUCCESS;

    DBGLOG(INIT, INFO, "Start Efuse Buffer Mode ..\n");
    DBGLOG(INIT, INFO, "ucEfuseBUfferModeCal is %x\n",
           prAdapter->rWifiVar.ucEfuseBufferModeCal);

    prChipInfo = prAdapter->chip_info;
    chip_id = prChipInfo->chip_id;
    prGlueInfo = prAdapter->prGlueInfo;
    /*
        if (prGlueInfo == NULL || prGlueInfo->prDev == NULL)
            goto label_exit;
    */
    /* allocate memory for buffer mode info */
    prSetEfuseBufModeInfo =
        (struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T *)
        kmalloc(sizeof(
                    struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));
    if (prSetEfuseBufModeInfo == NULL)
        goto label_exit;
    kalMemZero(prSetEfuseBufModeInfo,
               sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));
#if CFG_EFUSE_AUTO_MODE_SUPPORT
    /* allocate memory for Access Efuse Info */
    prAccessEfuseInfo =
        (struct PARAM_CUSTOM_ACCESS_EFUSE *)
        kalMemAlloc(sizeof(
                        struct PARAM_CUSTOM_ACCESS_EFUSE),
                    VIR_MEM_TYPE);
    if (prAccessEfuseInfo == NULL)
        goto label_exit;
    kalMemZero(prAccessEfuseInfo,
               sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));

    if (prAdapter->rWifiVar.ucEfuseBufferModeCal == LOAD_AUTO) {
        prAccessEfuseInfo->u4Address = (u4Efuse_addr /
                                        EFUSE_BLOCK_SIZE) * EFUSE_BLOCK_SIZE;
        kalIoctl(prGlueInfo,
                 wlanoidQueryProcessAccessEfuseRead,
                 prAccessEfuseInfo,
                 sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE),
                 TRUE, TRUE, TRUE, &u4BufLen);
        if (prGlueInfo->prAdapter->aucEepromVaule[1]
            == (chip_id >> 8)) {
            prAdapter->rWifiVar.ucEfuseBufferModeCal
                = LOAD_EFUSE;
            DBGLOG(INIT, STATE,
                   "[EFUSE AUTO] EFUSE Mode\n");
        } else {
            prAdapter->rWifiVar.ucEfuseBufferModeCal
                = LOAD_EEPROM_BIN;
            DBGLOG(INIT, STATE,
                   "[EFUSE AUTO] Buffer Mode\n");
        }
    }
#endif /* #if CFG_EFUSE_AUTO_MODE_SUPPORT */

    if (prAdapter->rWifiVar.ucEfuseBufferModeCal
        == LOAD_EEPROM_BIN) {
        /* Buffer mode */
        /* Only in buffer mode need to access bin file */
        /* 1 <1> Load bin file*/
        pucConfigBuf = (uint8_t *) kmalloc(
                           uEEPROM_BUFFER_LIMIT);
        if (pucConfigBuf == NULL)
            goto label_exit;

        kalMemZero(pucConfigBuf, uEEPROM_BUFFER_LIMIT);

        /* 1 <2> Construct EEPROM binary name */
        kalMemZero(aucEeprom, sizeof(aucEeprom));

        ret = kalSnprintf(aucEeprom, sizeof(aucEeprom),
                          "%s%x.bin",
                          apucEepromName[0], chip_id);
        if (ret == 0 || ret >= sizeof(aucEeprom)) {
            DBGLOG(INIT, ERROR,
                   "[%u] kalSnprintf failed, ret: %d\n",
                   __LINE__, ret);
            goto label_exit;
        }


        /* 1 <3> Request buffer bin */
        if (kalRequestFirmware(aucEeprom, pucConfigBuf,
                               uEEPROM_BUFFER_LIMIT, &u4ContentLen, NULL)
            == 0) {
            DBGLOG(INIT, INFO, "request file done\n");
        } else {
            DBGLOG(INIT, INFO, "can't find file\n");
            goto label_exit;
        }
        DBGLOG(INIT, INFO, "u4ContentLen = %d\n", u4ContentLen);

        /* 1 <4> Send CMD with bin file content */
        prGlueInfo = prAdapter->prGlueInfo;

        /* Update contents in local table */
        kalMemCopy(uacEEPROMImage, pucConfigBuf,
                   uEEPROM_BUFFER_LIMIT);

        if (u4ContentLen > uEEPROM_BUFFER_LIMIT)
            goto label_exit;

        kalMemCopy(prSetEfuseBufModeInfo->aBinContent, pucConfigBuf,
                   u4ContentLen);

        prSetEfuseBufModeInfo->ucSourceMode = 1;
    } else {
        /* eFuse mode */
        /* Only need to tell FW the content from, contents are directly
         * from efuse
         */
        prSetEfuseBufModeInfo->ucSourceMode = 0;
        u4ContentLen = 0;
    }
    prSetEfuseBufModeInfo->ucContentFormat = 0x1 |
                                             (prAdapter->rWifiVar.ucCalTimingCtrl << 4);
    prSetEfuseBufModeInfo->u2Count = u4ContentLen;

    retWlanStat = kalIoctl(prGlueInfo, wlanoidConnacSetEfusBufferMode,
                           (void *)prSetEfuseBufModeInfo,
                           OFFSET_OF(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T,
                                     aBinContent) + u4ContentLen,
                           FALSE, TRUE, TRUE, &u4BufLen);

    retWlanStat = WLAN_STATUS_SUCCESS;

label_exit:

    /* free memory */
    if (prSetEfuseBufModeInfo != NULL)
        kalMemFree(prSetEfuseBufModeInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));

    if (pucConfigBuf != NULL)
        kalMemFree(pucConfigBuf, VIR_MEM_TYPE,
                   uEEPROM_BUFFER_LIMIT);
#if CFG_EFUSE_AUTO_MODE_SUPPORT
    if (prAccessEfuseInfo != NULL)
        kalMemFree(prAccessEfuseInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));
#endif /* #if CFG_EFUSE_AUTO_MODE_SUPPORT */

    return retWlanStat;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief this function send buffer bin EEPROB_MTxxxx.bin to FW.
 *
 * \param[in] prAdapter
 *
 * \retval WLAN_STATUS_SUCCESS Success
 * \retval WLAN_STATUS_FAILURE Failed
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanConnac2XDownloadBufferBin(struct ADAPTER *prAdapter)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    struct GLUE_INFO *prGlueInfo = NULL;
#if CFG_STATIC_MEM_ALLOC
    struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T pSetEfuseBufModeInfo;
    struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T
        *prSetEfuseBufModeInfo = &pSetEfuseBufModeInfo;
#if CFG_EFUSE_AUTO_MODE_SUPPORT
    uint32_t u4Efuse_addr = 0;
    struct PARAM_CUSTOM_ACCESS_EFUSE pAccessEfuseInfo;
    struct PARAM_CUSTOM_ACCESS_EFUSE *prAccessEfuseInfo
            = &pAccessEfuseInfo;
#endif /* #if CFG_EFUSE_AUTO_MODE_SUPPORT */
#else /* #if CFG_STATIC_MEM_ALLOC */
    struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T
        *prSetEfuseBufModeInfo = NULL;
#if CFG_EFUSE_AUTO_MODE_SUPPORT
    uint32_t u4Efuse_addr = 0;
    struct PARAM_CUSTOM_ACCESS_EFUSE *prAccessEfuseInfo
            = NULL;
#endif /* #if CFG_EFUSE_AUTO_MODE_SUPPORT */
#endif /* #if CFG_STATIC_MEM_ALLOC */
    uint8_t *pucConfigBuf = NULL;
    uint32_t u4ContentLen = 0;
    uint8_t uTotalPage = 0;
    uint8_t uPageIdx = 0;
    uint32_t u4BufLen = 0;
    uint32_t retWlanStat = WLAN_STATUS_FAILURE;
    struct mt66xx_chip_info *prChipInfo = NULL;
    uint32_t chip_id;
#if (CFG_BUFFER_BIN_FROM_FLASH == 1)
    uint16_t u2EepromIdCheck = 0;
#endif /* #if (CFG_BUFFER_BIN_FROM_FLASH == 1) */
#if (CFG_SUPPORT_BUFFER_MODE) && (CFG_SUPPORT_BUFFER_MODE_HDR == 1)
    uint16_t uEEPROM_BUFFER_LIMIT = u4LenEEPROMImage;
#else /* #if (CFG_SUPPORT_BUFFER_MODE) && (CFG_SUPPORT_BUFFER_MODE_HDR == 1) */
    uint16_t uEEPROM_BUFFER_LIMIT = MAX_EEPROM_BUFFER_SIZE;
#endif /* #if (CFG_SUPPORT_BUFFER_MODE) && (CFG_SUPPORT_BUFFER_MODE_HDR == 1) */
#if (CFG_BUFFER_BIN_FROM_FLASH == 1)
    uint32_t wifi_flash_addr = getBufBinAddr();
#endif /* #if (CFG_BUFFER_BIN_FROM_FLASH == 1) */

    if (prAdapter == NULL) {
        DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
        goto label_exit;
    }

    if (prAdapter->fgIsSupportPowerOnSendBufferModeCMD == FALSE)
        return WLAN_STATUS_SUCCESS;

    DBGLOG(INIT, INFO, "Start Efuse Buffer Mode ..\n");
    DBGLOG(INIT, INFO, "ucEfuseBUfferModeCal is %x\n",
           prAdapter->rWifiVar.ucEfuseBufferModeCal);

    prGlueInfo = prAdapter->prGlueInfo;
    if (prGlueInfo == NULL)
        goto label_exit;

    prChipInfo = prAdapter->chip_info;
    chip_id = prChipInfo->chip_id;

#if !CFG_STATIC_MEM_ALLOC
    /* allocate memory for buffer mode info */
    prSetEfuseBufModeInfo =
        (struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T *)
        kalMemAlloc(sizeof(
                        struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T),
                    VIR_MEM_TYPE);
    if (prSetEfuseBufModeInfo == NULL)
        goto label_exit;
#endif /* #if !CFG_STATIC_MEM_ALLOC */
    kalMemZero(prSetEfuseBufModeInfo,
               sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));
#if CFG_EFUSE_AUTO_MODE_SUPPORT
#if !CFG_STATIC_MEM_ALLOC
    /* allocate memory for Access Efuse Info */
    prAccessEfuseInfo =
        (struct PARAM_CUSTOM_ACCESS_EFUSE *)
        kalMemAlloc(sizeof(
                        struct PARAM_CUSTOM_ACCESS_EFUSE),
                    VIR_MEM_TYPE);
    if (prAccessEfuseInfo == NULL)
        goto label_exit;
#endif /* #if !CFG_STATIC_MEM_ALLOC */
    kalMemZero(prAccessEfuseInfo,
               sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));
    if (prAdapter->rWifiVar.ucEfuseBufferModeCal == LOAD_AUTO) {
        prAccessEfuseInfo->u4Address = (u4Efuse_addr /
                                        EFUSE_BLOCK_SIZE) * EFUSE_BLOCK_SIZE;
        rStatus = kalIoctl(prGlueInfo,
                           wlanoidQueryProcessAccessEfuseRead,
                           prAccessEfuseInfo,
                           sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE),
                           TRUE, TRUE, TRUE, &u4BufLen);
        if (prGlueInfo->prAdapter->aucEepromVaule[1]
            == (chip_id >> 8) &&
            prGlueInfo->prAdapter->aucEepromVaule[2]
            > 0) {
            prAdapter->rWifiVar.ucEfuseBufferModeCal
                = LOAD_EFUSE;
            DBGLOG(INIT, STATE,
                   "[EFUSE AUTO] EFUSE Mode\n");
        } else {
            prAdapter->rWifiVar.ucEfuseBufferModeCal
                = LOAD_EEPROM_BIN;
            DBGLOG(INIT, STATE,
                   "[EFUSE AUTO] Buffer Mode\n");
        }
    }
#endif /* #if CFG_EFUSE_AUTO_MODE_SUPPORT */
    /* 1 <1> Load bin file*/
    /* 1 <2> Construct EEPROM binary name */
    /* 1 <3> Request buffer bin */
#if (CFG_SUPPORT_BUFFER_MODE_HDR == 1)
    u4ContentLen = u4LenEEPROMImage;
#if (CFG_BUFFER_BIN_FROM_FLASH == 1)
    /* read from header in flash/ nvdm */
    hal_flash_read(wifi_flash_addr,
                   (uint8_t *)&u2EepromIdCheck, 2);
    DBGLOG(INIT, INFO,
           "Get Id 0x%x from flash\n", u2EepromIdCheck);
#endif /* #if (CFG_BUFFER_BIN_FROM_FLASH == 1) */
#else /* #if (CFG_SUPPORT_BUFFER_MODE_HDR == 1) */
#endif /* #if (CFG_SUPPORT_BUFFER_MODE_HDR == 1) */
    DBGLOG(INIT, INFO, "u4ContentLen = %d\n", u4ContentLen);

    /* 1 <4> Send CMD with bin file content */
    if (u4ContentLen == 0 || u4ContentLen > uEEPROM_BUFFER_LIMIT)
        goto label_exit;

    /* Update contents in local table */
#if (CFG_SUPPORT_BUFFER_MODE_HDR == 1)
    /* use uacEEPROMImage array in $CHIP_eeprom.c */
    pucConfigBuf = uacEEPROMImage;
#if (CFG_BUFFER_BIN_FROM_FLASH == 1)
    prChipInfo = prAdapter->chip_info;
    if (u2EepromIdCheck == prChipInfo->chip_id) {
        hal_flash_read(wifi_flash_addr,
                       pucConfigBuf, u4ContentLen);
        DBGLOG(INIT, INFO,
               "Read buffer bin comtent from flash done!\n");
    } else {
        DBGLOG(INIT, WARN,
               "Id(0x%x) not match (0x%x), use default\n",
               prChipInfo->chip_id, u2EepromIdCheck);
    }
#endif /* #if (CFG_BUFFER_BIN_FROM_FLASH == 1) */
#else /* #if (CFG_SUPPORT_BUFFER_MODE_HDR == 1) */
    /* update from flash/ nvdm & COPY to driver bin */
    kalMemCopy(uacEEPROMImage, pucConfigBuf,
               uEEPROM_BUFFER_LIMIT);
#endif /* #if (CFG_SUPPORT_BUFFER_MODE_HDR == 1) */

    uTotalPage = u4ContentLen / BUFFER_BIN_PAGE_SIZE;
    if ((u4ContentLen % BUFFER_BIN_PAGE_SIZE) == 0)
        uTotalPage--;

    if (prAdapter->rWifiVar.ucEfuseBufferModeCal
        == LOAD_EEPROM_BIN)
        prSetEfuseBufModeInfo->ucSourceMode = 1;
    else {
        prSetEfuseBufModeInfo->ucSourceMode = 0;
        u4ContentLen = 0;
        uTotalPage = 0;
    }

    for (uPageIdx = 0; uPageIdx <= uTotalPage; uPageIdx++) {
        /* set format */
        prSetEfuseBufModeInfo->ucContentFormat = (
                                                     CONTENT_FORMAT_WHOLE_CONTENT |
                                                     ((uTotalPage << BUFFER_BIN_TOTAL_PAGE_SHIFT)
                                                      & BUFFER_BIN_TOTAL_PAGE_MASK) |
                                                     ((uPageIdx << BUFFER_BIN_PAGE_INDEX_SHIFT)
                                                      & BUFFER_BIN_PAGE_INDEX_MASK)
                                                 );
        /* set buffer size */
        prSetEfuseBufModeInfo->u2Count =
            (u4ContentLen < BUFFER_BIN_PAGE_SIZE ?
             u4ContentLen : BUFFER_BIN_PAGE_SIZE);
        /* set buffer */
        kalMemZero(prSetEfuseBufModeInfo->aBinContent,
                   BUFFER_BIN_PAGE_SIZE);
        if (prSetEfuseBufModeInfo->u2Count != 0)
            kalMemCopy(prSetEfuseBufModeInfo->aBinContent,
                       pucConfigBuf + uPageIdx * BUFFER_BIN_PAGE_SIZE,
                       prSetEfuseBufModeInfo->u2Count);
        /* send buffer */
        DBGLOG(INIT, INFO, "[%d/%d] load buffer size: 0x%x\n",
               uPageIdx, uTotalPage, prSetEfuseBufModeInfo->u2Count);
        rStatus = kalIoctl(prGlueInfo, wlanoidConnacSetEfusBufferMode,
                           (void *) prSetEfuseBufModeInfo, OFFSET_OF(
                               struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T,
                               aBinContent) + prSetEfuseBufModeInfo->u2Count,
                           FALSE, TRUE, TRUE, &u4BufLen);
        /* update remain size */
        u4ContentLen -= prSetEfuseBufModeInfo->u2Count;

        if (rStatus == WLAN_STATUS_FAILURE)
            DBGLOG(INIT, ERROR, "rStatus %x\n", rStatus);
    }
    retWlanStat = WLAN_STATUS_SUCCESS;

label_exit:
#if !CFG_STATIC_MEM_ALLOC
    /* free memory */
    if (prSetEfuseBufModeInfo != NULL)
        kalMemFree(prSetEfuseBufModeInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));

#if CFG_EFUSE_AUTO_MODE_SUPPORT
    if (prAccessEfuseInfo != NULL)
        kalMemFree(prAccessEfuseInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));
#endif /* #if CFG_EFUSE_AUTO_MODE_SUPPORT */
#endif /* #if !CFG_STATIC_MEM_ALLOC */

    return retWlanStat;
}

#if (CONFIG_WLAN_SERVICE == 1)
uint32_t wlanServiceInit(struct GLUE_INFO *prGlueInfo)
{

    struct service_test *prServiceTest;
    struct test_wlan_info *winfos;

    struct ATE_OPS_T *prAteOps = NULL;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;

    struct mt66xx_chip_info *prChipInfo;

    prChipInfo = mt66xx_driver_data_mt7933.chip_info;

    if (prGlueInfo == NULL)
        return WLAN_STATUS_FAILURE;

    prGlueInfo->rService.serv_id = SERV_HANDLE_TEST;
    prGlueInfo->rService.serv_handle
        = kmalloc(sizeof(struct service_test));
    if (prGlueInfo->rService.serv_handle == NULL) {
        DBGLOG(INIT, WARN,
               "prGlueInfo->rService.serv_handle memory alloc fail! %d\n",
               sizeof(struct service_test));
        return WLAN_STATUS_FAILURE;
    }

    prServiceTest = (struct service_test *)prGlueInfo->rService.serv_handle;
    prServiceTest->test_winfo
        = kmalloc(sizeof(struct test_wlan_info));
    if (prServiceTest->test_winfo == NULL) {
        DBGLOG(INIT, WARN,
               "prServiceTest->test_winfo memory alloc fail!\n");
        goto label_exit;
    }
    winfos = prServiceTest->test_winfo;

    prServiceTest->test_winfo->net_dev = NULL;

    if (prChipInfo->asicGetChipID)
        prServiceTest->test_winfo->chip_id =
            prChipInfo->asicGetChipID(prGlueInfo->prAdapter);
    else
        prServiceTest->test_winfo->chip_id = prChipInfo->chip_id;

    DBGLOG(INIT, WARN,
           "%s chip_id = 0x%x\n", __func__,
           prServiceTest->test_winfo->chip_id);

    DBGLOG(RFTEST, WARN, "Platform doesn't support EMI address\n");

    if (prGlueInfo->prAdapter->rWifiVar.ucEfuseBufferModeCal == TRUE) {
        /* buffermode */
        winfos->e2p_cur_mode = E2P_EEPROM_MODE;
        winfos->e2p_access_mode = E2P_EEPROM_MODE;
        DBGLOG(INIT, WARN, "INIT wlan_service to buffer mode\n");
    } else {
        /* efuse */
        winfos->e2p_cur_mode = E2P_EFUSE_MODE;
        winfos->e2p_access_mode = E2P_EFUSE_MODE;
        DBGLOG(INIT, WARN, "INIT wlan_service to efuse mode\n");
    }

    prServiceTest->test_op
        = kmalloc(sizeof(struct test_operation));
    if (prServiceTest->test_op == NULL) {
        DBGLOG(INIT, WARN,
               "prServiceTest->test_op memory alloc fail!\n");
        goto label_exit;
    }

    /*icap setting*/
    prAteOps = prChipInfo->prAteOps;
    if (prAteOps != NULL) {
        prServiceTest->test_winfo->icap_arch
            = prAteOps->u4Architech;
        prServiceTest->test_winfo->icap_bitwidth
            = prAteOps->u4EnBitWidth;
        prServiceTest->test_winfo->icap_phy_idx
            = prAteOps->u4PhyIdx;
#if (CFG_MTK_ANDROID_EMI == 1)
        prServiceTest->test_winfo->icap_emi_start_addr
            = prAteOps->u4EmiStartAddress;
        prServiceTest->test_winfo->icap_emi_end_addr
            = prAteOps->u4EmiEndAddress;
        prServiceTest->test_winfo->icap_emi_msb_addr
            = prAteOps->u4EmiMsbAddress;
#else /* #if (CFG_MTK_ANDROID_EMI == 1) */
        prServiceTest->test_winfo->icap_emi_start_addr = 0;
        prServiceTest->test_winfo->icap_emi_end_addr = 0;
        prServiceTest->test_winfo->icap_emi_msb_addr = 0;
#endif /* #if (CFG_MTK_ANDROID_EMI == 1) */
    } else {
        DBGLOG(INIT, WARN, "prAteOps is null!\n");
    }

    DBGLOG(INIT, INFO, "%s enter arch:%d, bw:%d!\n",
           __func__,
           prServiceTest->test_winfo->icap_arch,
           prServiceTest->test_winfo->icap_bitwidth);


    prServiceTest->engine_offload = true;
    winfos->oid_funcptr = (wlan_oid_handler_t) ServiceWlanOid;

    if (prChipInfo->initWlanServiceSetting)
        prChipInfo->initWlanServiceSetting(prGlueInfo->prAdapter);
    rStatus = mt_agent_init_service(&prGlueInfo->rService);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(INIT, WARN, "%s init fail err:%d\n", __func__, rStatus);

    return rStatus;

label_exit:

    /* free memory */
    if (prGlueInfo->rService.serv_handle != NULL)
        kalMemFree(prGlueInfo->rService.serv_handle, VIR_MEM_TYPE,
                   sizeof(struct service_test));

    if (prServiceTest->test_winfo != NULL)
        kalMemFree(prServiceTest->test_winfo, VIR_MEM_TYPE,
                   sizeof(struct test_wlan_info));

    if (prServiceTest->test_op != NULL)
        kalMemFree(prServiceTest->test_op, VIR_MEM_TYPE,
                   sizeof(struct test_operation));

    return WLAN_STATUS_FAILURE;
}
uint32_t wlanServiceExit(struct GLUE_INFO *prGlueInfo)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    struct service_test *prServiceTest;

    DBGLOG(INIT, TRACE, "%s enter\n", __func__);

    if (prGlueInfo == NULL || prGlueInfo->rService.serv_handle == NULL)
        return WLAN_STATUS_FAILURE;

    rStatus = mt_agent_exit_service(&prGlueInfo->rService);

    prServiceTest = (struct service_test *)prGlueInfo->rService.serv_handle;

    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(INIT, WARN, "wlanServiceExit fail err:%d\n", rStatus);

    if (prServiceTest->test_winfo)
        kalMemFree(prServiceTest->test_winfo,
                   VIR_MEM_TYPE, sizeof(struct test_wlan_info));

    if (prServiceTest->test_op)
        kalMemFree(prServiceTest->test_op,
                   VIR_MEM_TYPE, sizeof(struct test_operation));

    if (prGlueInfo->rService.serv_handle)
        kalMemFree(prGlueInfo->rService.serv_handle,
                   VIR_MEM_TYPE, sizeof(struct service_test));

    prGlueInfo->rService.serv_id = 0;

    return rStatus;
}
#endif /* #if (CONFIG_WLAN_SERVICE == 1) */

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH

#define FW_LOG_CMD_ON_OFF        0
#define FW_LOG_CMD_SET_LEVEL     1
static uint32_t u4LogOnOffCache = -1;

struct CMD_CONNSYS_FW_LOG {
    int32_t fgCmd;
    int32_t fgValue;
};

uint32_t connsysFwLogControl(struct ADAPTER *prAdapter, void *pvSetBuffer,
                             uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
    struct CMD_CONNSYS_FW_LOG *prCmd;
    struct CMD_HEADER rCmdV1Header;
    struct CMD_FORMAT_V1 rCmd_v1;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;

    if ((prAdapter == NULL) || (pvSetBuffer == NULL)
        || (pu4SetInfoLen == NULL))
        return WLAN_STATUS_FAILURE;

    /* init */
    *pu4SetInfoLen = sizeof(struct CMD_CONNSYS_FW_LOG);
    prCmd = (struct CMD_CONNSYS_FW_LOG *) pvSetBuffer;

    if (prCmd->fgCmd == FW_LOG_CMD_ON_OFF) {

        /*EvtDrvnLogEn 0/1*/
        uint8_t onoff[1] = {'0'};

        DBGLOG(INIT, TRACE, "FW_LOG_CMD_ON_OFF\n");

        rCmdV1Header.cmdType = CMD_TYPE_SET;
        rCmdV1Header.cmdVersion = CMD_VER_1;
        rCmdV1Header.cmdBufferLen = 0;
        rCmdV1Header.itemNum = 0;

        kalMemSet(rCmdV1Header.buffer, 0, MAX_CMD_BUFFER_LENGTH);
        kalMemSet(&rCmd_v1, 0, sizeof(struct CMD_FORMAT_V1));

        rCmd_v1.itemType = ITEM_TYPE_STR;

        /*send string format to firmware */
        rCmd_v1.itemStringLength = kalStrLen("EnableDbgLog");
        kalMemZero(rCmd_v1.itemString, MAX_CMD_NAME_MAX_LENGTH);
        kalMemCopy(rCmd_v1.itemString, "EnableDbgLog",
                   rCmd_v1.itemStringLength);

        if (prCmd->fgValue == 1) /* other cases, send 'OFF=0' */
            onoff[0] = '1';
        rCmd_v1.itemValueLength = 1;
        kalMemZero(rCmd_v1.itemValue, MAX_CMD_VALUE_MAX_LENGTH);
        kalMemCopy(rCmd_v1.itemValue, &onoff, 1);

        DBGLOG(INIT, INFO, "Send key word (%s) WITH (%s) to firmware\n",
               rCmd_v1.itemString, rCmd_v1.itemValue);

        kalMemCopy(((struct CMD_FORMAT_V1 *)rCmdV1Header.buffer),
                   &rCmd_v1,  sizeof(struct CMD_FORMAT_V1));

        rCmdV1Header.cmdBufferLen += sizeof(struct CMD_FORMAT_V1);
        rCmdV1Header.itemNum = 1;

        rStatus = wlanSendSetQueryCmd(
                      prAdapter, /* prAdapter */
                      CMD_ID_GET_SET_CUSTOMER_CFG, /* 0x70 */
                      TRUE,  /* fgSetQuery */
                      FALSE, /* fgNeedResp */
                      FALSE, /* fgIsOid */
                      NULL,  /* pfCmdDoneHandler*/
                      NULL,  /* pfCmdTimeoutHandler */
                      sizeof(struct CMD_HEADER),
                      (uint8_t *)&rCmdV1Header, /* pucInfoBuffer */
                      NULL,  /* pvSetQueryBuffer */
                      0      /* u4SetQueryBufferLen */
                  );

        /* keep in cache */
        u4LogOnOffCache = prCmd->fgValue;
    } else if (prCmd->fgCmd == FW_LOG_CMD_SET_LEVEL) {
        /*ENG_LOAD_OFFSET 1*/
        /*USERDEBUG_LOAD_OFFSET 2 */
        /*USER_LOAD_OFFSET 3 */
        DBGLOG(INIT, INFO, "FW_LOG_CMD_SET_LEVEL\n");
    } else {
        DBGLOG(INIT, INFO, "command can not parse\n");
    }
    return WLAN_STATUS_SUCCESS;
}

static void consys_log_event_notification(int cmd, int value)
{
    struct CMD_CONNSYS_FW_LOG rFwLogCmd;
    struct GLUE_INFO *prGlueInfo = NULL;
    struct ADAPTER *prAdapter = NULL;
    struct net_device *prDev = gPrDev;
    uint32_t rStatus = WLAN_STATUS_FAILURE;
    uint32_t u4BufLen;

    DBGLOG(INIT, INFO, "gPrDev=%p, cmd=%d, value=%d\n",
           gPrDev, cmd, value);

    if (kalIsHalted()) { /* power-off */
        u4LogOnOffCache = value;
        DBGLOG(INIT, INFO,
               "Power off return, u4LogOnOffCache=%d\n",
               u4LogOnOffCache);
        return;
    }

    prGlueInfo = (prDev != NULL) ?
                 *((struct GLUE_INFO **) netdev_priv(prDev)) : NULL;
    DBGLOG(INIT, TRACE, "prGlueInfo=%p\n", prGlueInfo);
    if (!prGlueInfo) {
        u4LogOnOffCache = value;
        DBGLOG(INIT, INFO,
               "prGlueInfo == NULL return, u4LogOnOffCache=%d\n",
               u4LogOnOffCache);
        return;
    }
    prAdapter = prGlueInfo->prAdapter;
    DBGLOG(INIT, TRACE, "prAdapter=%p\n", prAdapter);
    if (!prAdapter) {
        u4LogOnOffCache = value;
        DBGLOG(INIT, INFO,
               "prAdapter == NULL return, u4LogOnOffCache=%d\n",
               u4LogOnOffCache);
        return;
    }

    kalMemZero(&rFwLogCmd, sizeof(rFwLogCmd));
    rFwLogCmd.fgCmd = cmd;
    rFwLogCmd.fgValue = value;

    rStatus = kalIoctl(prGlueInfo,
                       connsysFwLogControl,
                       &rFwLogCmd,
                       sizeof(struct CMD_CONNSYS_FW_LOG),
                       FALSE, FALSE, FALSE,
                       &u4BufLen);
}
#endif /* #ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH */

void wlanOnPreAdapterStart(struct GLUE_INFO *prGlueInfo,
                           struct ADAPTER *prAdapter,
                           struct REG_INFO **pprRegInfo,
                           struct mt66xx_chip_info **pprChipInfo)
{
    uint32_t u4Idx = 0;

    DBGLOG(INIT, TRACE, "start.\n");
    prGlueInfo->u4ReadyFlag = 0;

#if CFG_TCP_IP_CHKSUM_OFFLOAD
    prAdapter->fgIsSupportCsumOffload = TRUE;
    prAdapter->u4CSUMFlags = CSUM_OFFLOAD_EN_ALL;
#endif /* #if CFG_TCP_IP_CHKSUM_OFFLOAD */

#if CFG_SUPPORT_CFG
    wlanGetConfig(prAdapter);
#endif /* #if CFG_SUPPORT_CFG */

    /* Init Chip Capability */
    *pprChipInfo = prAdapter->chip_info;
    if ((*pprChipInfo)->asicCapInit)
        (*pprChipInfo)->asicCapInit(prAdapter);

#if (CFG_WMT_WIFI_PATH_SUPPORT)
    /* Default support 2.4/5G MIMO */
    prAdapter->rWifiFemCfg.u2WifiPath = (
                                            WLAN_FLAG_2G4_WF0 | WLAN_FLAG_5G_WF0 |
                                            WLAN_FLAG_2G4_WF1 | WLAN_FLAG_5G_WF1);
#endif /* #if (CFG_WMT_WIFI_PATH_SUPPORT) */

    /* 4 <5> Start Device */
    *pprRegInfo = &prGlueInfo->rRegInfo;

    /* P_REG_INFO_T prRegInfo = (P_REG_INFO_T) kmalloc(
     *              sizeof(REG_INFO_T), GFP_KERNEL);
     */
    kalMemSet(*pprRegInfo, 0, sizeof(struct REG_INFO));

    /* Trigger the action of switching Pwr state to drv_own */
    prAdapter->fgIsFwOwn = TRUE;

    nicpmWakeUpWiFi(prAdapter);


    /* The Init value of u4WpaVersion/u4AuthAlg shall be
     * DISABLE/OPEN, not zero!
     */
    /* The Init value of u4CipherGroup/u4CipherPairwise shall be
     * NONE, not zero!
     */
    for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
        struct GL_WPA_INFO *prWpaInfo =
            aisGetWpaInfo(prAdapter, u4Idx);

        if (!prWpaInfo)
            continue;

        prWpaInfo->u4WpaVersion =
            IW_AUTH_WPA_VERSION_DISABLED;
        prWpaInfo->u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;
        prWpaInfo->u4CipherGroup = IW_AUTH_CIPHER_NONE;
        prWpaInfo->u4CipherPairwise = IW_AUTH_CIPHER_NONE;
    }
}

static
void wlanOnPostAdapterStart(struct ADAPTER *prAdapter,
                            struct GLUE_INFO *prGlueInfo)
{
    DBGLOG(INIT, TRACE, "start.\n");
    if (HAL_IS_TX_DIRECT(prAdapter)) {
        if (!prAdapter->fgTxDirectInited) {
            QUEUE_INITIALIZE(&prAdapter->rTxDirectSkbQueue);
            cnmTimerInitTimer(prAdapter,
                              &prAdapter->rTxDirectSkbTimer, (PFN_MGMT_TIMEOUT_FUNC)
                              nicTxDirectTimerCheckSkbQ, (unsigned long)prGlueInfo);

            cnmTimerInitTimer(prAdapter,
                              &prAdapter->rTxDirectHifTimer, (PFN_MGMT_TIMEOUT_FUNC)
                              nicTxDirectTimerCheckHifQ, (unsigned long)prGlueInfo);

            prAdapter->fgTxDirectInited = TRUE;
        }
    }
}

static int32_t wlanOnPreNetRegister(struct GLUE_INFO *prGlueInfo,
                                    struct ADAPTER *prAdapter,
                                    struct mt66xx_chip_info *prChipInfo,
                                    struct WIFI_VAR *prWifiVar,
                                    const uint8_t bAtResetFlow)
{

    if (!bAtResetFlow) {
        g_u4HaltFlag = 0;

#if CFG_SUPPORT_BUFFER_MODE && (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
#if CFG_MTK_ANDROID_WMT
        if (!bAtResetFlow) {
#else /* #if CFG_MTK_ANDROID_WMT */
        {
#endif /* #if CFG_MTK_ANDROID_WMT */
            if (prChipInfo->downloadBufferBin)
            {
                if (prChipInfo->downloadBufferBin(prAdapter) !=
                    WLAN_STATUS_SUCCESS)
                    return -1;
            }
        }
#endif /* #if CFG_SUPPORT_BUFFER_MODE && (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1) */


#if CFG_SUPPORT_DBDC
        /* Update DBDC default setting */
        cnmInitDbdcSetting(prAdapter);
#endif /* #if CFG_SUPPORT_DBDC */
    }

    /* send regulatory information to firmware */
    rlmDomainSendInfoToFirmware(prAdapter);

    return 0;
}

static void wlanOnPostNetRegister(void)
{
    g_fgArpOffloadStatus = FALSE;
}

static
int32_t wlanOnWhenProbeSuccess(struct GLUE_INFO *prGlueInfo,
                               struct ADAPTER *prAdapter,
                               const uint8_t bAtResetFlow)
{
    uint32_t u4LogLevel = ENUM_WIFI_LOG_LEVEL_DEFAULT;

    if (prGlueInfo == NULL) {
        DBGLOG(INIT, ERROR, "%s, !prGlueInfo fail\n", __func__);
        return -1;
    }

    DBGLOG(INIT, TRACE, "start.\n");
#if CFG_SUPPORT_CFG
#if CFG_SUPPORT_EASY_DEBUG
    /* move before reading file
     * wlanLoadDefaultCustomerSetting(prAdapter);
     */
    wlanFeatureToFw(prGlueInfo->prAdapter);
#endif /* #if CFG_SUPPORT_EASY_DEBUG */
#endif /* #if CFG_SUPPORT_CFG */

#if CFG_SUPPORT_IOT_AP_BLACKLIST
    wlanCfgLoadIotApRule(prAdapter);
    wlanCfgDumpIotApRule(prAdapter);
#endif /* #if CFG_SUPPORT_IOT_AP_BLACKLIST */
    if (!bAtResetFlow) {

#if CFG_SUPPORT_CFG_FILE
        wlanCfgSetSwCtrl(prGlueInfo->prAdapter);
        wlanCfgSetChip(prGlueInfo->prAdapter);
        wlanCfgSetCountryCode(prGlueInfo->prAdapter);
#endif /* #if CFG_SUPPORT_CFG_FILE */

#if CFG_MET_TAG_SUPPORT
        if (met_tag_init() != 0)
            DBGLOG(INIT, ERROR, "MET_TAG_INIT error!\n");
#endif /* #if CFG_MET_TAG_SUPPORT */
    }

#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
    /* Calibration Backup Flow */
    if (!g_fgIsCalDataBackuped) {
        if (rlmTriggerCalBackup(prGlueInfo->prAdapter,
                                g_fgIsCalDataBackuped) == WLAN_STATUS_FAILURE) {
            DBGLOG(RFTEST, INFO,
                   "Error : Boot Time Wi-Fi Enable Fail........\n");
            return -1;
        }

        g_fgIsCalDataBackuped = TRUE;
    } else {
        if (rlmTriggerCalBackup(prGlueInfo->prAdapter,
                                g_fgIsCalDataBackuped) == WLAN_STATUS_FAILURE) {
            DBGLOG(RFTEST, INFO,
                   "Error : Normal Wi-Fi Enable Fail........\n");
            return -1;
        }
    }
#endif /* #if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST */

    /* card is ready */
    prGlueInfo->u4ReadyFlag = 1;

    wlanDbgGetGlobalLogLevel(ENUM_WIFI_LOG_MODULE_FW,
                             &u4LogLevel);
    if (u4LogLevel > ENUM_WIFI_LOG_LEVEL_DEFAULT)
        wlanDbgSetLogLevelImpl(prAdapter,
                               ENUM_WIFI_LOG_LEVEL_VERSION_V1,
                               ENUM_WIFI_LOG_MODULE_FW,
                               u4LogLevel);

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
    /* sync log status with firmware */
    if (u4LogOnOffCache != -1) /* -1: connsysD does not set */
        consys_log_event_notification((int)FW_LOG_CMD_ON_OFF,
                                      u4LogOnOffCache);
#endif /* #ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH */

    /*Send default county code to firmware*/
    prAdapter->rWifiVar.u2CountryCode = COUNTRY_CODE_WW;
#if (CFG_SUPPORT_PWR_LIMIT_COUNTRY == 1)
    prAdapter->prDomainInfo = &defaultRegDomain;
#endif /* #if (CFG_SUPPORT_PWR_LIMIT_COUNTRY == 1) */
    rlmDomainSendCmd(prAdapter);

    return 0;
}

void wlanOffStopWlanThreads(IN struct GLUE_INFO *prGlueInfo)
{
    DBGLOG(INIT, TRACE, "start.\n");

    if (prGlueInfo->main_thread == NULL
#if CFG_SUPPORT_MULTITHREAD
        && prGlueInfo->hif_thread == NULL
        && prGlueInfo->rx_thread == NULL
#endif /* #if CFG_SUPPORT_MULTITHREAD */
       ) {
        DBGLOG(INIT, INFO,
               "Threads are already NULL, skip stop and free\n");
        return;
    }

    vTaskDelete(prGlueInfo->main_thread);

    DBGLOG(INIT, INFO, "wlan thread stopped\n");

    /* prGlueInfo->rHifInfo.main_thread = NULL; */
    prGlueInfo->main_thread = NULL;
#if CFG_SUPPORT_MULTITHREAD
    prGlueInfo->hif_thread = NULL;
    prGlueInfo->rx_thread = NULL;

    prGlueInfo->u4TxThreadPid = 0xffffffff;
    prGlueInfo->u4HifThreadPid = 0xffffffff;
#endif /* #if CFG_SUPPORT_MULTITHREAD */

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Wlan probe function. This function probes and initializes the device.
 *
 * \param[in] pvData     data passed by bus driver init function
 *                           _HIF_EHPI: NULL
 *                           _HIF_SDIO: sdio bus driver handle
 *
 * \retval 0 Success
 * \retval negative value Failed
 */
/*----------------------------------------------------------------------------*/
int32_t wlanProbe(void *pvData, void *pvDriverData)
{
    enum ENUM_PROBE_FAIL_REASON {
        BUS_INIT_FAIL,
        BUS_SET_IRQ_FAIL,
        ADAPTER_START_FAIL,
        NET_REGISTER_FAIL,
        FAIL_MET_INIT_PROCFS,
        FAIL_REASON_NUM
    } eFailReason;
    int32_t i4DevIdx = 0;
    struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)pvData;
    struct ADAPTER *prAdapter = NULL;
    int32_t i4Status = 0;
    struct REG_INFO *prRegInfo;
    struct mt66xx_chip_info *prChipInfo;
    struct WIFI_VAR *prWifiVar;
    uint8_t wait_event_bits = 0;
#if CFG_SUPPORT_802_11R
    uint32_t u4Idx = 0;
#endif /* #if CFG_SUPPORT_802_11R */
    EventBits_t ret;

    eFailReason = FAIL_REASON_NUM;
    do {
        if (prGlueInfo == NULL) {
            DBGLOG(INIT, ERROR, "!prGlueInfo\n");
            return -1;
        }

        /* 4 <2> Create network device, Adapter, KalInfo,
         *       prDevHandler(netdev)
         */
        wlanNetCreate(prGlueInfo, pvDriverData);
        prAdapter = prGlueInfo->prAdapter;

        DBGLOG(INIT, INFO, "wlanNetCreated prAdapter %p %p\n",
               prGlueInfo->prAdapter, prAdapter);

        /* workaround for mockingbird DRV_OWN before access WFDMA CR */
        prAdapter->fgIsFwOwn = TRUE;
        ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
        DBGLOG(INIT, INFO, "ACQUIRE_POWER_CONTROL_FROM_PM\n");

        g_init_wait = xEventGroupCreate();

        wait_event_bits = WLAN_THREAD_INIT_BIT;
#if CFG_SUPPORT_RXTHREAD
        wait_event_bits |= RX_THREAD_INIT_BIT;
#endif /* #if CFG_SUPPORT_RXTHREAD */

        /* init main thread */
        if (pdPASS != xTaskCreate(main_thread, "wifi_main_task",
                                  WLAN_THREAD_STACK_SIZE, prGlueInfo,
                                  WLAN_THREAD_TASK_PRI, &prGlueInfo->main_thread)) {
            DBGLOG(INIT, ERROR, "create main_thread task failed!\n");
            return -1;
        }
#if CFG_SUPPORT_RXTHREAD
        /* init rx indicate thread */
        if (pdPASS != xTaskCreate(rx_thread, "rx_thread",
                                  WLAN_THREAD_STACK_SIZE, prGlueInfo,
                                  RX_THREAD_TASK_PRI, &prGlueInfo->rx_thread)) {
            DBGLOG(INIT, ERROR, "create rx_thread task failed!\n");
            return -1;
        }
#endif /* #if CFG_SUPPORT_RXTHREAD */

        ret = xEventGroupSync(g_init_wait, WF_THREAD_INIT_DONE_BIT,
                              wait_event_bits, portMAX_DELAY);

        DBGLOG(INIT, INFO, "thread initialized! %x\n", ret);

        /* Setup IRQ */
        i4Status = glBusSetIrq(NULL, NULL, prGlueInfo);
        if (i4Status != WLAN_STATUS_SUCCESS) {
            DBGLOG(INIT, ERROR, "wlanProbe: Set IRQ error\n");
            eFailReason = BUS_SET_IRQ_FAIL;
            break;
        }

        //prGlueInfo->i4DevIdx = i4DevIdx;
        prWifiVar = &prAdapter->rWifiVar;

#ifdef HAL_SLEEP_MANAGER_ENABLED
        sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_WIFI);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
        wlanOnPreAdapterStart(prGlueInfo,
                              prAdapter,
                              &prRegInfo,
                              &prChipInfo);

        if (wlanAdapterStart(prAdapter,
                             prRegInfo, FALSE) != WLAN_STATUS_SUCCESS)
            i4Status = -ADAPTER_START_FAIL;

#ifdef HAL_SLEEP_MANAGER_ENABLED
        sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_WIFI);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

        wlanOnPostAdapterStart(prAdapter, prGlueInfo);

        /* kfree(prRegInfo); */

        if (i4Status < 0) {
            eFailReason = ADAPTER_START_FAIL;
            break;
        }

        wlanOnPreNetRegister(prGlueInfo, prAdapter, prChipInfo,
                             prWifiVar, FALSE);

        /* 4 <3> Register the card */
        i4DevIdx = wlanNetRegister(prGlueInfo);
        if (i4DevIdx < 0) {
            i4Status = -6;
            DBGLOG(INIT, ERROR,
                   "wlanProbe: Cannot register the net_device context to the kernel\n");
            eFailReason = NET_REGISTER_FAIL;
            break;
        }

        wlanOnPostNetRegister();
#if (CONFIG_WLAN_SERVICE == 1)
        wlanServiceInit(prGlueInfo);
#endif /* #if (CONFIG_WLAN_SERVICE == 1) */

#if CFG_SUPPORT_802_11R
        for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
            struct FT_IES *prFtIEs =
                aisGetFtIe(prAdapter, u4Idx);
            kalMemZero(prFtIEs,
                       sizeof(*prFtIEs));
        }
#endif /* #if CFG_SUPPORT_802_11R */

#if CFG_SUPPORT_ANT_DIV
        /* pinmux for antenna diversity */
        if (prAdapter->rWifiVar.ucAntDivMode < ENUM_ANTDIV_MODE_DISABLE) {
            hal_pinmux_set_function(HAL_GPIO_40, MT7933_PIN_40_FUNC_ANT_SEL1);
        }
#endif /* #if CFG_SUPPORT_ANT_DIV */

    } while (FALSE);

    if (i4Status == 0) {
        wlanOnWhenProbeSuccess(prGlueInfo, prAdapter, FALSE);
        DBGLOG(INIT, INFO,
               "wlanProbe: probe success, feature set: 0x%llx\n",
               wlanGetSupportedFeatureSet(prGlueInfo));
    } else {
        DBGLOG(INIT, ERROR, "wlanProbe: probe failed, reason:%d\n",
               eFailReason);
        switch (eFailReason) {
            case NET_REGISTER_FAIL:
                wlanAdapterStop(prAdapter);
            /* fallthrough */
            case ADAPTER_START_FAIL:
            /* fallthrough */
            case BUS_SET_IRQ_FAIL:
                /* prGlueInfo->prAdapter is released in
                 * wlanNetDestroy
                 */
                /* Set NULL value for local prAdapter as well */
                prAdapter = NULL;
                break;
            default:
                break;
        }
    }

    return i4Status;
}               /* end of wlanProbe() */

static int32_t wlanOffAtReset(struct ADAPTER *prAdapter)
{
    uint32_t u4Status = WLAN_STATUS_SUCCESS;
    struct GL_HIF_INFO *prHifInfo = NULL;

    ASSERT(prAdapter);

    wlanOffClearAllQueues(prAdapter);

    /* Hif power off wifi */
    prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
    DBGLOG(INIT, ERROR, "Power off Wi-Fi!\n");
    nicDisableInterrupt(prAdapter);
    prHifInfo->fgIsPowerOff = true;
    prAdapter->fgIsCr4FwDownloaded = FALSE;

    wlanOffUninitNicModule(prAdapter, FALSE);

    fgSimplifyResetFlow = FALSE;

    return u4Status;
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief A method to stop driver operation and release all resources. Following
 *        this call, no frame should go up or down through this interface.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void wlanRemoveAfterMainThread(struct GLUE_INFO *prGlueInfo)
{
    struct ADAPTER *prAdapter = NULL;
    uint8_t fgResult = FALSE;
#if CFG_CHIP_RESET_SUPPORT
    uint8_t fgIsNotifyRstDone = fgSimplifyResetFlow;
#endif /* #if CFG_CHIP_RESET_SUPPORT */

    prAdapter = prGlueInfo->prAdapter;

    DBGLOG(INIT, ERROR, "Remove wlan 2!\n");

    if (HAL_IS_TX_DIRECT(prAdapter)) {
        if (prAdapter->fgTxDirectInited) {
            cnmTimerStopTimer(prAdapter,
                              &prAdapter->rTxDirectSkbTimer);
            cnmTimerStopTimer(prAdapter,
                              &prAdapter->rTxDirectHifTimer);
        }
    }

    kalMemSet(&(prGlueInfo->prAdapter->rWlanInfo), 0,
              sizeof(struct WLAN_INFO));

#if CFG_ENABLE_WIFI_DIRECT
    if (prGlueInfo->prAdapter->fgIsP2PRegistered) {
        /*DBGLOG(INIT, INFO, "p2pNetUnregister...\n");*/
        /*p2pNetUnregister(prGlueInfo, FALSE);*/
        DBGLOG(INIT, INFO, "p2pRemove...\n");
        /*p2pRemove must before wlanAdapterStop */
        p2pRemove(prGlueInfo);
    }
#endif /* #if CFG_ENABLE_WIFI_DIRECT */

#if CFG_ENABLE_BT_OVER_WIFI
    if (prGlueInfo->rBowInfo.fgIsRegistered)
        glUnregisterAmpc(prGlueInfo);
#endif /* #if CFG_ENABLE_BT_OVER_WIFI */

#if (CFG_MET_PACKET_TRACE_SUPPORT == 1)
    kalMetRemoveProcfs();
#endif /* #if (CFG_MET_PACKET_TRACE_SUPPORT == 1) */

#if CFG_MET_TAG_SUPPORT
    if (GL_MET_TAG_UNINIT() != 0)
        DBGLOG(INIT, ERROR, "MET_TAG_UNINIT error!\n");
#endif /* #if CFG_MET_TAG_SUPPORT */

    if (fgSimplifyResetFlow) {
        wlanOffAtReset(prAdapter);
    } else {
        wlanAdapterStop(prAdapter);
    }

    HAL_LP_OWN_SET(prAdapter, &fgResult);
    DBGLOG(INIT, INFO, "HAL_LP_OWN_SET(%d)\n",
           (uint32_t) fgResult);

    /* 4 <x> Stopping handling interrupt and free IRQ */
    /* To Be implement */
    glBusFreeIrq(NULL, prGlueInfo);

    /* 4 <5> Release the Bus */
    glBusRelease(NULL);

#if (CFG_SUPPORT_TRACE_TC4 == 1)
    wlanDebugTC4Uninit();
#endif /* #if (CFG_SUPPORT_TRACE_TC4 == 1) */
    /* 4 <6> Unregister the card */
    wlanNetUnregister(prGlueInfo);

    /* 4 <7> Destroy the device */
    wlanNetDestroy(prGlueInfo);

#if CFG_CHIP_RESET_SUPPORT & !CFG_WMT_RESET_API_SUPPORT
    fgIsResetting = FALSE;
#endif /* #if CFG_CHIP_RESET_SUPPORT & !CFG_WMT_RESET_API_SUPPORT */

    /* exitWlan */
#if CFG_PRE_ALLOCATION_IO_BUFFER
    kalUninitIOBuffer();
#endif /* #if CFG_PRE_ALLOCATION_IO_BUFFER */

#if CFG_STATIC_MEM_ALLOC
    g_prGlueInfo = NULL;
#else /* #if CFG_STATIC_MEM_ALLOC */
    if (g_prGlueInfo)
        kalMemFree(g_prGlueInfo, VIR_MEM_TYPE,
                   sizeof(struct GLUE_INFO));
    g_prGlueInfo = NULL;
#endif /* #if CFG_STATIC_MEM_ALLOC */

    LOG_FUNC("exitWlan>\r\n");

    LOG_FUNC("wifi exit success\r\n");

    vEventGroupDelete(g_init_wait);

    _wsys_off(0, NULL);

#if CFG_CHIP_RESET_SUPPORT
    if (fgIsNotifyRstDone) {
        if (!g_wait_wifi_main_down)
            g_wait_wifi_main_down = xSemaphoreCreateBinary();
        xSemaphoreGive(g_wait_wifi_main_down);
    }
#endif /* #if CFG_CHIP_RESET_SUPPORT */
}

static void wlanRemove(void *pvData)
{
    struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)pvData;
    int ret = pdFALSE;
    uint32_t u4Idx = 0;

    DBGLOG(INIT, ERROR, "Remove wlan!\n");

    if (prGlueInfo == NULL) {
        DBGLOG(INIT, INFO, "prGlueInfo is NULL\n");
        return;
    }
    /* Need to get A-DIE ver anytime when device plug in,
    * or will fail on the case with insert different A-DIE card.
    */
    prGlueInfo->prAdapter->chip_info->u4ADieVer = 0xFFFFFFFF;

#if (CONFIG_WLAN_SERVICE == 1)
    wlanServiceExit(prGlueInfo);
#endif /* #if (CONFIG_WLAN_SERVICE == 1) */

    /* to avoid that wpa_supplicant/hostapd triogger new cfg80211 command */
    prGlueInfo->u4ReadyFlag = 0;

    /* Have tried to do scan done here, but the exception occurs for */
    /* the P2P scan. Keep the original design that scan done in the  */
    /* p2pStop/wlanStop.                         */


    nicSerDeInit(prGlueInfo->prAdapter);

#if CFG_ENABLE_BT_OVER_WIFI
    if (prGlueInfo->rBowInfo.fgIsNetRegistered) {
        bowNotifyAllLinkDisconnected(prGlueInfo->prAdapter);
        /* wait 300ms for BoW module to send deauth */
        kalMsleep(300);
    }
#endif /* #if CFG_ENABLE_BT_OVER_WIFI */


    ret = xSemaphoreTake(g_halt_sem, 0);
    if (ret == pdTRUE) {
        g_u4HaltFlag = 1;
        xSemaphoreGive(g_halt_sem);
    }

#if CFG_SUPPORT_802_11R
    for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
        struct FT_IES *prFtIEs =
            aisGetFtIe(prGlueInfo->prAdapter, u4Idx);
        if (prFtIEs->pucIEBuf != NULL) {
            kalMemFree(prFtIEs->pucIEBuf, VIR_MEM_TYPE,
                       prFtIEs->u4IeLength);
            prFtIEs->pucIEBuf = NULL;
        }
    }
#endif /* #if CFG_SUPPORT_802_11R */

    /* 4 <2> Mark HALT, notify main thread to stop, and clean up queued
     *       requests
     */
    if (!(xEventGroupGetBits(g_init_wait) & WF_THREAD_INIT_DONE_BIT)) {
        DBGLOG(INIT, ERROR, "main_thread not ready yet\n");
        return;
    }
    xEventGroupSetBits(prGlueInfo->event_main_thread, GLUE_FLAG_HALT);
}               /* end of wlanRemove() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Driver entry point when the driver is configured as a Linux Module,
 *        and is called once at module load time, by the user-level modutils
 *        application: insmod or modprobe.
 *
 * \retval 0     Success
 */
/*----------------------------------------------------------------------------*/
/* 1 Module Entry Point */
static int initWlan(struct GLUE_INFO *prGlueInfo)
{
    int ret = 0;
    struct connsys_wf_hif_op *hif_op = NULL;
    struct wiphy *prWiphy = NULL;
    struct wireless_dev *prWdev[KAL_AIS_NUM] = {NULL};
    uint32_t u4Idx = 0;

    if (!prGlueInfo) {
        DBGLOG(INIT, ERROR, "!prGlueInfo\n");
        return -1;
    }

    wlanDebugInit();

    /* memory pre-allocation */
#if CFG_PRE_ALLOCATION_IO_BUFFER
    kalInitIOBuffer(TRUE);
#else /* #if CFG_PRE_ALLOCATION_IO_BUFFER */
    kalInitIOBuffer(FALSE);
#endif /* #if CFG_PRE_ALLOCATION_IO_BUFFER */

    /* Create wiphy */
#if CFG_STATIC_MEM_ALLOC
    prWiphy = &g_rWiphy;
#else /* #if CFG_STATIC_MEM_ALLOC */
    prWiphy = (struct wiphy *)
              kalMemAlloc(sizeof(struct wiphy), VIR_MEM_TYPE);

    if (!prWiphy) {
        DBGLOG(INIT, ERROR,
               "Allocating memory to wiphy device failed\n");
        return -1;
    }
#endif /* #if CFG_STATIC_MEM_ALLOC */
    kalMemZero(prWiphy, sizeof(struct wiphy));
    prWiphy->bands[KAL_BAND_2GHZ] = &mtk_band_2ghz;
    prWiphy->bands[KAL_BAND_5GHZ] = &mtk_band_5ghz;
    g_prWiphy = prWiphy;

    /* Create wireless_dev */
    for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
#if CFG_STATIC_MEM_ALLOC
        prWdev[u4Idx] = &grWdev[u4Idx];
        kalMemZero(prWdev[u4Idx], sizeof(struct wireless_dev));
        prWdev[u4Idx]->wiphy = g_prWiphy;
        gprWdev[u4Idx] = prWdev[u4Idx];
#else /* #if CFG_STATIC_MEM_ALLOC */
        prWdev[u4Idx] = (struct wireless_dev *)
                        kalMemAlloc(sizeof(struct wireless_dev), VIR_MEM_TYPE);
        if (prWdev[u4Idx]) {
            prWdev[u4Idx]->wiphy = g_prWiphy;
            gprWdev[u4Idx] = prWdev[u4Idx];
        } else {
            DBGLOG(INIT, ERROR,
                   "Allocating memory to wireless_dev context failed\n");
            /* free wiphy */
            vfree(prWiphy);
            prWiphy = NULL;
            return -1;
        }
#endif /* #if CFG_STATIC_MEM_ALLOC */
    }

#if (CFG_SUPPORT_SINGLE_SKU == 1) || (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
    if (rlmDomainGetCtrlState() == REGD_STATE_UNDEFINED)
        rlmDomainResetCtrlInfo(TRUE);
#endif /* #if (CFG_SUPPORT_SINGLE_SKU == 1) || (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1) */

    hif_op = glRegisterBus();

    if (!hif_op) {
        kalUninitIOBuffer();
        return -1;
    }

#if (CFG_CHIP_RESET_SUPPORT)
    glResetInit(prGlueInfo);
#endif /* #if (CFG_CHIP_RESET_SUPPORT) */

    hif_op->probe();
    ret = wlanProbe(prGlueInfo, hif_op);
#ifdef MTK_WF_IWPRIV_CLI_ENABLE
    struct CHIP_DBG_OPS *prChipDbg;
    prChipDbg = prGlueInfo->prAdapter->chip_info->prDebugOps;
    if (prChipDbg) {
        prChipDbg->showTxdInfo = connac2x_show_txd_Info;
        prChipDbg->showWtblInfo = connac2x_show_wtbl_info;
        prChipDbg->showUmacFwtblInfo = connac2x_show_umac_wtbl_info;
        prChipDbg->show_rx_rate_info = connac2x_show_rx_rate_info;
        prChipDbg->show_rx_rssi_info = connac2x_show_rx_rssi_info;
        prChipDbg->show_stat_info = connac2x_show_stat_info;
    }
#endif /* #ifdef MTK_WF_IWPRIV_CLI_ENABLE */

    /* Register P2P interface in initWlan */
#if CFG_ENABLE_WIFI_DIRECT
    struct MSG_P2P_SWITCH_OP_MODE *prSwitchModeMsg =
        (struct MSG_P2P_SWITCH_OP_MODE *) NULL;
    struct MSG_P2P_NETDEV_REGISTER *prNetDevRegisterMsg =
        (struct MSG_P2P_NETDEV_REGISTER *) NULL;
    uint8_t ucRoleIdx = 0;
    struct net_device *dev = NULL;
    uint8_t ucChkCnt = 255;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex;

    prGlueInfo = g_prGlueInfo;
    ucBssIndex = p2pFuncGetSapBssIndex(prGlueInfo);
    prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);

    if (prBssInfo->eCurrentOPMode != OP_MODE_ACCESS_POINT) {
        /* change drv if to GO/AP first if its not AP mode */
        prNetDevRegisterMsg =
            cnmMemAlloc(prGlueInfo->prAdapter,
                        RAM_TYPE_MSG,
                        sizeof(struct MSG_P2P_NETDEV_REGISTER));

        if (prNetDevRegisterMsg == NULL) {
            DBGLOG(P2P, ERROR, "prNetDevRegisterMsg is NULL\n");
            return -1;
        }

        prNetDevRegisterMsg->rMsgHdr.eMsgId =
            MID_MNY_P2P_NET_DEV_REGISTER;
        prNetDevRegisterMsg->fgIsEnable = 1;
        prNetDevRegisterMsg->ucMode = RUNNING_AP_MODE;

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (struct MSG_HDR *) prNetDevRegisterMsg,
                    MSG_SEND_METHOD_BUF);

        /* Polling until P2P is registered */
        while (!prGlueInfo->prAdapter->fgIsP2PRegistered &&
               ucChkCnt) {
            if (--ucChkCnt == 0) {
                DBGLOG(P2P, ERROR,
                       "Failed to register P2P net dev\n");
                return -1;
            }
            kalMsleep(1);
        }

        dev = wlanGetNetInterfaceByBssIdx(prGlueInfo, ucBssIndex);

        if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx)
            != 0) {
            DBGLOG(P2P, ERROR, "RoleIdx %d return\n",
                   ucRoleIdx);
            return 0;
        }
        /* Switch OP MOde. */
        prSwitchModeMsg =
            (struct MSG_P2P_SWITCH_OP_MODE *)
            cnmMemAlloc(prGlueInfo->prAdapter,
                        RAM_TYPE_MSG,
                        sizeof(struct MSG_P2P_SWITCH_OP_MODE));
        if (prSwitchModeMsg == NULL) {
            DBGLOG(P2P, ERROR, "prSwitchModeMsg is NULL\n");
            return -1;
        }

        prSwitchModeMsg->rMsgHdr.eMsgId =
            MID_MNY_P2P_FUN_SWITCH;
        prSwitchModeMsg->ucRoleIdx = ucRoleIdx;
        prSwitchModeMsg->eOpMode = OP_MODE_ACCESS_POINT;
        kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);

        mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
                    (struct MSG_HDR *) prSwitchModeMsg,
                    MSG_SEND_METHOD_BUF);
    }
#endif /* #if CFG_ENABLE_WIFI_DIRECT */

    return ret;
}               /* end of initWlan() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Driver exit point when the driver as a Linux Module is removed. Called
 *        at module unload time, by the user level modutils application: rmmod.
 *        This is our last chance to clean up after ourselves.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
/* 1 Module Leave Point */
static void exitWlan(void)
{
#if CFG_STATIC_MEM_ALLOC
#else /* #if CFG_STATIC_MEM_ALLOC */
    uint32_t u4Idx = 0;
#endif /* #if CFG_STATIC_MEM_ALLOC */
    glUnregisterBus();

#if CFG_CHIP_RESET_SUPPORT
    glResetUninit();
#endif /* #if CFG_CHIP_RESET_SUPPORT */

    wlanRemove(g_prGlueInfo);

#if CFG_STATIC_MEM_ALLOC
#else /* #if CFG_STATIC_MEM_ALLOC */
    /* free AIS wdev */
    if (gprWdev[0]) {
        if (gprWdev[0]->wiphy) {
            vPortFree(gprWdev[0]->wiphy);
            gprWdev[0]->wiphy = NULL;
        }

        for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
            vPortFree(gprWdev[u4Idx]);
            gprWdev[u4Idx] = NULL;
        }
    }
#endif /* #if CFG_STATIC_MEM_ALLOC */
    DBGLOG(INIT, INFO, "exitWlan\n");
}               /* end of exitWlan() */

int mtk_wlan_tx(struct pbuf *p, struct netif *netif)
{
    int ret = 0;
#if CFG_WLAN_CACHE_HIT_DBG
    uint32_t hitCnt_h = 0, hitCnt_l = 0;
    uint32_t accCnt_h = 0, accCnt_l = 0;

    /* hal_cache_disable(); */
    /* hal_cache_enable(); */
#endif /* #if CFG_WLAN_CACHE_HIT_DBG */

    ret = wlanHardStartXmit(p, netif, g_prGlueInfo);

#if CFG_WLAN_CACHE_HIT_DBG
    hal_cache_get_hit_count(0, &hitCnt_h,
                            &hitCnt_l, &accCnt_h, &accCnt_l);
    LOG_FUNC("countIdx 0, hit Cnt %lu.%lu, access Cnt %lu.%lu\r\n",
             hitCnt_h, hitCnt_l, accCnt_h, accCnt_l);
    hal_cache_get_hit_count(1, &hitCnt_h,
                            &hitCnt_l, &accCnt_h, &accCnt_l);
    LOG_FUNC("countIdx 1, hit Cnt %lu.%lu, access Cnt %lu.%lu\r\n",
             hitCnt_h, hitCnt_l, accCnt_h, accCnt_l);
#endif /* #if CFG_WLAN_CACHE_HIT_DBG */
    return ret;
}

void wlan_register_callback(wlan_netif_input_fn input, struct netif *netif, int opmode)
{
    DBGLOG(INIT, INFO, ">\n");
    switch (opmode) {
        case LWIP_STA_MODE:
            wlan_sta_input = input;
            wlan_sta_netif = netif;
            break;
#if CFG_SUPPORT_SNIFFER
        case LWIP_SNIFFER_MODE:
            wlan_sniffer_input = input;
            wlan_sniffer_netif = netif;
            break;
#endif /* #if CFG_SUPPORT_SNIFFER */
        case LWIP_AP_MODE:
            wlan_ap_input = input;
            wlan_ap_netif = netif;
            break;
        default:
            DBGLOG(INIT, ERROR, "un-supported opmode %d\n", opmode);
    }
    return;
}

static void wifi_initialization(void *hdl)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    int ret = 0;

    /* LOG_FUNC("wifi init start\r\n"); */

    DBGLOG(INIT, INFO, "alloc prGlueInfo %u\n", sizeof(struct GLUE_INFO));
#if CFG_STATIC_MEM_ALLOC
    prGlueInfo = &g_rGlueInfo;
    if (g_first_boot) {
        /* In GlueInfo */
        kalMemSet(&g_rScanComp, 0, sizeof(g_rScanComp));
        kalMemSet(&g_rHaltComp, 0, sizeof(g_rHaltComp));
        kalMemSet(&g_rPendComp, 0, sizeof(g_rPendComp));
#if CFG_SUPPORT_MULTITHREAD
        kalMemSet(&g_rHifHaltComp, 0, sizeof(g_rHifHaltComp));
        kalMemSet(&g_rRxHaltComp, 0, sizeof(g_rRxHaltComp));
#endif /* #if CFG_SUPPORT_MULTITHREAD */
#if CFG_SUPPORT_NCHO
        kalMemSet(&g_rAisChGrntComp, 0, sizeof(g_rAisChGrntComp));
#endif /* #if CFG_SUPPORT_NCHO */
        for (ret = 0; ret < SPIN_LOCK_NUM; ret++)
            g_rSpinLock[ret] = NULL;
        g_ioctl_sem = NULL;
        g_pucRxCached = NULL;
        g_pucTxCached = NULL;
        g_pucCoalescingBufCached = NULL;
        g_pucMgtBufCached = NULL;
    }
#else /* #if CFG_STATIC_MEM_ALLOC */
    prGlueInfo = (struct GLUE_INFO *)
                 kalMemAlloc(sizeof(struct GLUE_INFO), VIR_MEM_TYPE);
    if (!prGlueInfo) {
        DBGLOG(INIT, ERROR, "ERROR: !prGlueInfo %u\n",
               sizeof(struct GLUE_INFO));
        DBGLOG(INIT, ERROR, "wifi init failed\n");
        vTaskDelete(NULL);
        return;
    }
#endif /* #if CFG_STATIC_MEM_ALLOC */
    kalMemZero(prGlueInfo, sizeof(struct GLUE_INFO));

    /*For mini supplicant*/
#if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P)
    if (!g_wait_drv_ready)
        g_wait_drv_ready = xSemaphoreCreateBinary();
#endif /* #if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P) */
    g_prGlueInfo = prGlueInfo;

    ret = initWlan(g_prGlueInfo);

    if (!ret) {
        /*For mini supplicant*/
#if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P)
        ret = xSemaphoreGive(g_wait_drv_ready);
        if (ret != TRUE)
            DBGLOG(INIT, ERROR, "already gave drv ready\n");
#endif /* #if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P) */
        g_first_boot = 0;
        LOG_FUNC("wifi init success\r\n");
    } else
        LOG_FUNC("wifi init failed %d\r\n", ret);

#ifdef MTK_SLT_ENABLE
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    g_i4SltConnsysWifiOn = ret;
    xTaskNotifyFromISR(sltwifi_task_handler,
                       1,
                       eSetBits,
                       &xHigherPriorityTaskWoken);
#endif /* #ifdef MTK_SLT_ENABLE */
    vTaskDelete(NULL);
}

static void wifi_exit(void)
{
#if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P)
    int8_t ret = 0;
#endif /* #if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P) */

    LOG_FUNC("wifi exit start\r\n");
    /* main_thread */
    exitWlan();
#if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P)
    if (g_wait_drv_ready)
        ret = xSemaphoreTake(g_wait_drv_ready, 0);
    LOG_FUNC("wifi exit take back drv ready %d\r\n", ret);
#endif /* #if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P) */
}

BaseType_t wifi_init_task(void)
{
    BaseType_t ret = pdPASS;
    TaskHandle_t init_handler;

    /* LOG_FUNC("wifi task create\r\n"); */
    ret = xTaskCreate(wifi_initialization, "wlan_init"
                      , WLAN_INIT_STACK_SIZE, NULL
                      , WLAN_INIT_TASK_PRI, &init_handler);

    if (ret != pdPASS)
        LOG_FUNC("[wifi]create init task failed\r\n");

    /* LOG_FUNC("wifi task create success\r\n"); */
    return ret;
}

BaseType_t wifi_exit_task(void)
{
    BaseType_t ret = pdPASS;

    wifi_exit();
    return ret;
}

