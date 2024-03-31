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
 ** Id: @(#) gl_p2p.c@@
 */

/*! \file   gl_p2p.c
 *    \brief  Main routines of Linux driver interface for Wi-Fi Direct
 *
 *    This file contains the main routines of Linux driver
 *    for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */


/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"

#include "gl_p2p_ioctl.h"

#include "precomp.h"

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */
#define ARGV_MAX_NUM        (4)

/*For CFG80211 - wiphy parameters*/
#define MAX_SCAN_LIST_NUM   (1)
#define MAX_SCAN_IE_LEN     (512)

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */


/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */

struct net_device *g_P2pPrDev;
/* struct wireless_dev *gprP2pWdev; */
/* struct wireless_dev *gprP2pRoleWdev[KAL_P2P_NUM]; */
struct net_device *gPrP2pDev[KAL_P2P_NUM];

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */


/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

/*---------------------------------------------------------------------------*/
/*!
 * \brief Allocate memory for P2P_INFO, GL_P2P_INFO, P2P_CONNECTION_SETTINGS
 *                                          P2P_SPECIFIC_BSS_INFO, P2P_FSM_INFO
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   TRUE
 *           FALSE
 */
/*---------------------------------------------------------------------------*/
uint8_t p2PAllocInfo(IN struct GLUE_INFO *prGlueInfo, IN uint8_t ucIdex)
{
    struct ADAPTER *prAdapter = NULL;
    struct WIFI_VAR *prWifiVar = NULL;
    /* UINT_32 u4Idx = 0; */

    ASSERT(prGlueInfo);

    prAdapter = prGlueInfo->prAdapter;
    prWifiVar = &(prAdapter->rWifiVar);

    ASSERT(prAdapter);
    ASSERT(prWifiVar);

    do {
        if (prGlueInfo->prP2PInfo[ucIdex] == NULL) {
            /*alloc memory for p2p info */
            prGlueInfo->prP2PInfo[ucIdex] =
                kmalloc(sizeof(struct GL_P2P_INFO));

            if (prGlueInfo->prP2PDevInfo == NULL) {
                prGlueInfo->prP2PDevInfo =
                    kmalloc(sizeof(struct GL_P2P_DEV_INFO));
                if (prGlueInfo->prP2PDevInfo) {
                    kalMemZero(prGlueInfo->prP2PDevInfo,
                               sizeof(struct GL_P2P_DEV_INFO));
                }
            }

            if (prAdapter->prP2pInfo == NULL) {
                prAdapter->prP2pInfo = kmalloc(sizeof(struct P2P_INFO));
                if (prAdapter->prP2pInfo) {
                    kalMemZero(prAdapter->prP2pInfo,
                               sizeof(struct P2P_INFO));
                }
            }

            if (prWifiVar->prP2pDevFsmInfo == NULL) {
                /* Don't only create P2P device for ucIdex 0.
                 * Avoid the exception that mtk_init_ap_role
                 * called without p2p0.
                 */
                prWifiVar->prP2pDevFsmInfo = kmalloc(sizeof(struct P2P_DEV_FSM_INFO));
                if (prWifiVar->prP2pDevFsmInfo) {
                    kalMemZero(prWifiVar->prP2pDevFsmInfo,
                               sizeof(struct
                                      P2P_DEV_FSM_INFO));
                }
            }

            prWifiVar->prP2PConnSettings[ucIdex] = kmalloc(sizeof(struct P2P_CONNECTION_SETTINGS));
            prWifiVar->prP2pSpecificBssInfo[ucIdex] = kmalloc(sizeof(struct P2P_SPECIFIC_BSS_INFO));
#if CFG_ENABLE_PER_STA_STATISTICS_LOG
            prWifiVar->prP2pQueryStaStatistics[ucIdex] = kmalloc(sizeof(struct PARAM_GET_STA_STATISTICS));
#endif /* #if CFG_ENABLE_PER_STA_STATISTICS_LOG */
            /* TODO: It can be moved
             * to the interface been created.
             */
        } else {
            ASSERT(prAdapter->prP2pInfo != NULL);
            ASSERT(prWifiVar->prP2PConnSettings[ucIdex] != NULL);
            /* ASSERT(prWifiVar->prP2pFsmInfo != NULL); */
            ASSERT(prWifiVar->prP2pSpecificBssInfo[ucIdex] != NULL);
        }
        /*MUST set memory to 0 */
        kalMemZero(prGlueInfo->prP2PInfo[ucIdex],
                   sizeof(struct GL_P2P_INFO));
        kalMemZero(prWifiVar->prP2PConnSettings[ucIdex],
                   sizeof(struct P2P_CONNECTION_SETTINGS));
        /* kalMemZero(prWifiVar->prP2pFsmInfo, sizeof(P2P_FSM_INFO_T)); */
        kalMemZero(prWifiVar->prP2pSpecificBssInfo[ucIdex],
                   sizeof(struct P2P_SPECIFIC_BSS_INFO));
#if CFG_ENABLE_PER_STA_STATISTICS_LOG
        if (prWifiVar->prP2pQueryStaStatistics[ucIdex])
            kalMemZero(prWifiVar->prP2pQueryStaStatistics[ucIdex],
                       sizeof(struct PARAM_GET_STA_STATISTICS));
#endif /* #if CFG_ENABLE_PER_STA_STATISTICS_LOG */
    } while (FALSE);

    if (!prGlueInfo->prP2PDevInfo)
        DBGLOG(P2P, ERROR, "prP2PDevInfo error\n");
    else
        DBGLOG(P2P, TRACE, "prP2PDevInfo ok\n");

    if (!prGlueInfo->prP2PInfo[ucIdex])
        DBGLOG(P2P, ERROR, "prP2PInfo error\n");
    else
        DBGLOG(P2P, TRACE, "prP2PInfo ok\n");



    /* chk if alloc successful or not */
    if (prGlueInfo->prP2PInfo[ucIdex] &&
        prGlueInfo->prP2PDevInfo &&
        prAdapter->prP2pInfo &&
        prWifiVar->prP2PConnSettings[ucIdex] &&
        /* prWifiVar->prP2pFsmInfo && */
        prWifiVar->prP2pSpecificBssInfo[ucIdex])
        return TRUE;


    DBGLOG(P2P, ERROR, "[fail!]p2PAllocInfo :fail\n");

    if (prWifiVar->prP2pSpecificBssInfo[ucIdex]) {
        kalMemFree(prWifiVar->prP2pSpecificBssInfo[ucIdex],
                   VIR_MEM_TYPE,
                   sizeof(struct P2P_SPECIFIC_BSS_INFO));

        prWifiVar->prP2pSpecificBssInfo[ucIdex] = NULL;
    }

#if CFG_ENABLE_PER_STA_STATISTICS_LOG
    if (prWifiVar->prP2pQueryStaStatistics[ucIdex]) {
        kalMemFree(prWifiVar->prP2pQueryStaStatistics[ucIdex],
                   VIR_MEM_TYPE,
                   sizeof(struct PARAM_GET_STA_STATISTICS));
        prWifiVar->prP2pQueryStaStatistics[ucIdex] = NULL;
    }
#endif /* #if CFG_ENABLE_PER_STA_STATISTICS_LOG */

    /* if (prWifiVar->prP2pFsmInfo) { */
    /* kalMemFree(prWifiVar->prP2pFsmInfo,
     * VIR_MEM_TYPE, sizeof(P2P_FSM_INFO_T));
     */

    /* prWifiVar->prP2pFsmInfo = NULL; */
    /* } */
    if (prWifiVar->prP2PConnSettings[ucIdex]) {
        kalMemFree(prWifiVar->prP2PConnSettings[ucIdex],
                   VIR_MEM_TYPE, sizeof(struct P2P_CONNECTION_SETTINGS));

        prWifiVar->prP2PConnSettings[ucIdex] = NULL;
    }
    if (prGlueInfo->prP2PDevInfo) {
        kalMemFree(prGlueInfo->prP2PDevInfo,
                   VIR_MEM_TYPE, sizeof(struct GL_P2P_DEV_INFO));

        prGlueInfo->prP2PDevInfo = NULL;
    }
    if (prGlueInfo->prP2PInfo[ucIdex]) {
        kalMemFree(prGlueInfo->prP2PInfo[ucIdex],
                   VIR_MEM_TYPE, sizeof(struct GL_P2P_INFO));

        prGlueInfo->prP2PInfo[ucIdex] = NULL;
    }
    if (prAdapter->prP2pInfo) {
        kalMemFree(prAdapter->prP2pInfo,
                   VIR_MEM_TYPE, sizeof(struct P2P_INFO));

        prAdapter->prP2pInfo = NULL;
    }
    return FALSE;

}

/*---------------------------------------------------------------------------*/
/*!
 * \brief Free memory for P2P_INFO, GL_P2P_INFO, P2P_CONNECTION_SETTINGS
 *                                          P2P_SPECIFIC_BSS_INFO, P2P_FSM_INFO
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *  [in] ucIdx       The BSS with the idx will be freed.
 *               "ucIdx == 0xff" will free all BSSs.
 *               Only has meaning for "CFG_ENABLE_UNIFY_WIPHY == 1"
 *
 * \return   TRUE
 *           FALSE
 */
/*---------------------------------------------------------------------------*/
uint8_t p2PFreeInfo(struct GLUE_INFO *prGlueInfo, uint8_t ucIdx)
{
    struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

    ASSERT(prGlueInfo);
    ASSERT(prAdapter);

    if (ucIdx >= KAL_P2P_NUM) {
        DBGLOG(P2P, ERROR, "ucIdx=%d is invalid\n", ucIdx);
        return FALSE;
    }

    /* Expect that prAdapter->prP2pInfo must be existing. */
    if (prAdapter->prP2pInfo == NULL) {
        DBGLOG(P2P, ERROR, "prAdapter->prP2pInfo is NULL\n");
        return FALSE;
    }

    /* TODO: how can I sure that the specific P2P device can be freed?
     * The original check is that prGlueInfo->prAdapter->fgIsP2PRegistered.
     * For one wiphy feature, this func may be called without
     * (fgIsP2PRegistered == FALSE) condition.
     */

    if (prGlueInfo->prP2PInfo[ucIdx] != NULL) {
        kalMemFree(prAdapter->rWifiVar.prP2PConnSettings[ucIdx],
                   VIR_MEM_TYPE,
                   sizeof(struct P2P_CONNECTION_SETTINGS));
        prAdapter->rWifiVar.prP2PConnSettings[ucIdx] = NULL;

        kalMemFree(prAdapter->rWifiVar.prP2pSpecificBssInfo[ucIdx],
                   VIR_MEM_TYPE,
                   sizeof(struct P2P_SPECIFIC_BSS_INFO));
        prAdapter->rWifiVar.prP2pSpecificBssInfo[ucIdx] = NULL;

#if CFG_ENABLE_PER_STA_STATISTICS_LOG
        kalMemFree(prAdapter->rWifiVar.prP2pQueryStaStatistics[ucIdx],
                   VIR_MEM_TYPE,
                   sizeof(struct PARAM_GET_STA_STATISTICS));
        prAdapter->rWifiVar.prP2pQueryStaStatistics[ucIdx] = NULL;
#endif /* #if CFG_ENABLE_PER_STA_STATISTICS_LOG */

#if (CFG_SUPPORT_DFS_MASTER == 1)
        if (prGlueInfo->prP2PInfo[ucIdx]->chandef) {
            if (prGlueInfo->prP2PInfo[ucIdx]->chandef->chan) {
                cnmMemFree(prGlueInfo->prAdapter,
                           prGlueInfo->prP2PInfo[ucIdx]
                           ->chandef->chan);
                prGlueInfo->prP2PInfo[ucIdx]
                ->chandef->chan = NULL;
            }
            cnmMemFree(prGlueInfo->prAdapter,
                       prGlueInfo->prP2PInfo[ucIdx]->chandef);
            prGlueInfo->prP2PInfo[ucIdx]->chandef = NULL;
        }
#endif /* #if (CFG_SUPPORT_DFS_MASTER == 1) */

        kalMemFree(prGlueInfo->prP2PInfo[ucIdx],
                   VIR_MEM_TYPE,
                   sizeof(struct GL_P2P_INFO));
        prGlueInfo->prP2PInfo[ucIdx] = NULL;

        prAdapter->prP2pInfo->u4DeviceNum--;
    }

    if (prAdapter->prP2pInfo->u4DeviceNum == 0) {
        /* all prP2PInfo are freed, and free the general part now */

        kalMemFree(prAdapter->prP2pInfo, VIR_MEM_TYPE,
                   sizeof(struct P2P_INFO));
        prAdapter->prP2pInfo = NULL;

        if (prGlueInfo->prP2PDevInfo) {
            kalMemFree(prGlueInfo->prP2PDevInfo, VIR_MEM_TYPE,
                       sizeof(struct GL_P2P_DEV_INFO));
            prGlueInfo->prP2PDevInfo = NULL;
        }
        if (prAdapter->rWifiVar.prP2pDevFsmInfo) {
            kalMemFree(prAdapter->rWifiVar.prP2pDevFsmInfo,
                       VIR_MEM_TYPE, sizeof(struct P2P_DEV_FSM_INFO));
            prAdapter->rWifiVar.prP2pDevFsmInfo = NULL;
        }

        /* Reomve p2p bss scan list */
        scanRemoveAllP2pBssDesc(prAdapter);
    }

    return TRUE;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief Register for cfg80211 for Wi-Fi Direct
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   TRUE
 *           FALSE
 */
/*---------------------------------------------------------------------------*/
uint8_t glRegisterP2P(struct GLUE_INFO *prGlueInfo, const char *prDevName,
                      const char *prDevName2, uint8_t ucApMode)
{
    struct ADAPTER *prAdapter = NULL;
    uint8_t  ucRegisterNum = 1, i = 0;
    struct net_device *dev_handler;
    uint8_t fgIsApMode = FALSE;

    ASSERT(prGlueInfo);

    prAdapter = prGlueInfo->prAdapter;
    ASSERT(prAdapter);

    if ((ucApMode == RUNNING_AP_MODE) ||
        (ucApMode == RUNNING_DUAL_AP_MODE) ||
        (ucApMode == RUNNING_P2P_AP_MODE)) {
        fgIsApMode = TRUE;
        if ((ucApMode == RUNNING_DUAL_AP_MODE) ||
            (ucApMode == RUNNING_P2P_AP_MODE)) {
            ucRegisterNum = 2;
            glP2pCreateWirelessDevice(prGlueInfo);
        }
    }

    do {
        DBGLOG(INIT, INFO, "cur/total_dev %d/%d\n",
               i, ucRegisterNum);
        /* allocate p2pinfo */
        if (!p2PAllocInfo(prGlueInfo, i)) {
            DBGLOG(INIT, ERROR, "Allocate memory for p2p FAILED\n");
            return FALSE;
        }

        /* allocate netdev */
        prGlueInfo->prP2PInfo[i]->prDevHandler =
            kalMemAlloc(
                sizeof(*prGlueInfo->prP2PInfo[i]->prDevHandler),
                VIR_MEM_TYPE);

        if (!prGlueInfo->prP2PInfo[i]->prDevHandler) {
            DBGLOG(INIT, ERROR, "unable to alloc netdev for p2p\n");
            goto err_alloc_netdev;
        }

        dev_handler = prGlueInfo->prP2PInfo[i]->prDevHandler;
        dev_handler->netif = wlan_ap_netif;
        dev_handler->netif_rxcb = wlan_ap_input;
        dev_handler->bss_idx = 0;
        dev_handler->gl_info = prGlueInfo;

        /* set p2p net device register state */
        /* p2pNetRegister() will check prAdapter->rP2PNetRegState. */
        prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERED;

#if CFG_P2P_DEV_FSM
        /* bind netdev pointer to netdev index */
        wlanBindBssIdxToNetInterface(prGlueInfo,
                                     p2pDevFsmInit(prAdapter),
                                     (void *)prGlueInfo->prP2PInfo[i]->prDevHandler);
#endif

        prGlueInfo->prP2PInfo[i]->aprRoleHandler =
            prGlueInfo->prP2PInfo[i]->prDevHandler;

        DBGLOG(P2P, INFO, "check prDevHandler = %p\n",
               prGlueInfo->prP2PInfo[i]->prDevHandler);
        DBGLOG(P2P, INFO, "aprRoleHandler = %p\n",
               prGlueInfo->prP2PInfo[i]->aprRoleHandler);

        dev_handler->bss_idx = p2pRoleFsmInit(prAdapter, i);
        /* Currently wpasupplicant can't support create interface.*/
        /* so initial the corresponding data structure here. */
        wlanBindBssIdxToNetInterface(prGlueInfo,
                                     dev_handler->bss_idx,
                                     (void *)prGlueInfo->prP2PInfo[i]->aprRoleHandler);

        DBGLOG(P2P, INFO, "p2pRole bss_idx %d\n",
               dev_handler->bss_idx);

        /* setup running mode */
        p2pFuncInitConnectionSettings(prAdapter,
                                      prAdapter->rWifiVar.prP2PConnSettings[i], fgIsApMode);

        /* Active network too early would cause HW not able to sleep.
         * Defer the network active time.
         */

        i++;
    } while (i < ucRegisterNum);

    if ((ucApMode == RUNNING_DUAL_AP_MODE) ||
        (ucApMode == RUNNING_P2P_AP_MODE))
        prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum = 2;
    else
        prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum = 1;

    DBGLOG(P2P, STATE, "P2P[%d] device num = %d\n",
           i, prAdapter->prP2pInfo->u4DeviceNum);

    return TRUE;

err_alloc_netdev:
    return FALSE;
}               /* end of glRegisterP2P() */

uint8_t glP2pCreateWirelessDevice(struct GLUE_INFO *prGlueInfo)
{
    DBGLOG(INIT, ERROR, "no support...please fix me\n");
    return TRUE;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief Unregister Net Device for Wi-Fi Direct
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *  [in] ucIdx       The BSS with the idx will be freed.
 *               "ucIdx == 0xff" will free all BSSs.
 *               Only has meaning for "CFG_ENABLE_UNIFY_WIPHY == 1"
 *
 * \return   TRUE
 *           FALSE
 */
/*---------------------------------------------------------------------------*/
uint8_t glUnregisterP2P(struct GLUE_INFO *prGlueInfo, uint8_t ucIdx)
{
    uint8_t  ucRegisterNum = 1, i = 0;

    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prAdapter);

    if ((ucIdx == RUNNING_DUAL_AP_MODE) ||
        (ucIdx == RUNNING_P2P_AP_MODE))
        ucRegisterNum = 2;

#if CFG_P2P_DEV_FSM
    p2pDevFsmUninit(prGlueInfo->prAdapter);
#endif

    do {
        DBGLOG(INIT, INFO, "cur/total_dev %d/%d\n",
               i, ucRegisterNum);

        /* allocate netdev */
        if (prGlueInfo->prP2PInfo[i]->prDevHandler)
            kalMemFree(prGlueInfo->prP2PInfo[i]->prDevHandler,
                       sizeof(*prGlueInfo->prP2PInfo[i]->prDevHandler),
                       VIR_MEM_TYPE);

        p2pRoleFsmUninit(prGlueInfo->prAdapter, i);

        if (!p2PFreeInfo(prGlueInfo, i))
            DBGLOG(INIT, ERROR, "p2PFreeInfo FAILED [%d]\n", i);

        i++;
    } while (i < ucRegisterNum);

    return TRUE;
}

