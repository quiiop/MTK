/*
 * Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its
 * licensors. Without the prior written permission of MediaTek and/or its
 * licensors, any reproduction, modification, use or disclosure of MediaTek
 * Software, and information contained herein, in whole or in part, shall be
 * strictly prohibited. You may only use, reproduce, modify, or distribute
 * (as applicable) MediaTek Software if you have agreed to and been bound by
 * the applicable license agreement with MediaTek ("License Agreement") and
 * been granted explicit permission to do so within the License Agreement
 * ("Permitted User"). If you are not a Permitted User, please cease any
 * access or use of MediaTek Software immediately. BY OPENING THIS FILE,
 * RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREESTHAT MEDIATEK SOFTWARE
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO
 * LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY
 * TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED
 * HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK
 * SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE
 * PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */
/*
***************************************************************************
    Module Name:
    WiFi

    Abstract:
    WiFi processor configure / setting for STA operations

    Revision History:
    Who                     When                 What
    Michael Rong      2015/04/24       Initial
    --------            ----------      --------------------------------------
***************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "precomp.h"
#include "wifi_api_ex.h"
#include "gl_os.h"
#include "misc.h"
#include "wifi_event_gen4m.h"
#include "get_profile_string.h"
#include "hal_top_thermal.h"
#include "lwip/dhcp.h"

#ifdef MTK_MINISUPP_ENABLE
#include "drivers/driver.h"
#include "utils/eloop.h"
#include "wifi_os_api.h"
#include "wpa_supplicant_i.h"
#include "wpa_supplicant_task.h"
#endif /* #ifdef MTK_MINISUPP_ENABLE */

#include "wifi_netif.h"
#ifdef MTK_NVDM_ENABLE
#include "nvdm.h"
#endif /* #ifdef MTK_NVDM_ENABLE */

#if IP_NAPT
#include "tcpip_wrapper.h"
#endif /* #if IP_NAPT */

#if MTK_CSI_PSRAM_ENABLE
#include "xmodem.h"
#endif /* #if MTK_CSI_PSRAM_ENABLE */

#if CFG_SUPPORT_ANT_DIV
#include "hal_gpio.h"
#endif /* #if CFG_SUPPORT_ANT_DIV */

#define hex_isdigit(c) (('0' <= (c) && (c) <= '9') || ('a' <= (c) &&  \
            (c) <= 'f') || ('A' <= (c) && (c) <= 'F'))

/**
 * @brief temperary definition of the error code, this should be redesigned
 * in the future
 *
 * @note currently all errors are defined as -1
 */
#define WIFI_SUCC (0)

#define WIFI_FAIL (-1)

#define WIFI_ERR_PARA_INVALID (-1)

#define WIFI_ERR_NOT_SUPPORT (-1)

/*
Definition of the wireless mode, these are used in
wifi_config_get_wireless_mode function.
*/
#define WIFI_MODE_80211N BIT(0)
#define WIFI_MODE_80211AC BIT(1)
#define WIFI_MODE_80211AX BIT(2)

/*
Definition of the TWT mode, these are used in
wifi_set_twtparams function.
*/
#define CMD_TWT_ACTION_TEN_PARAMS 10
#define CMD_TWT_ACTION_THREE_PARAMS 3
#define CMD_TWT_MAX_PARAMS CMD_TWT_ACTION_TEN_PARAMS


/*
In lwip task ip_ready_callback function will call 3 WiFi APIs to get driver
status, but it have low rate to lead to dead lock, so disable the mutex
protected by defalut.
*/

os_semaphore_t opmode_switch_result_semphr;
static bool security_use_flag;
static bool bssid_use_flag;
uint8_t ps_log_onoff;
uint8_t wifi_inited;
uint8_t wifi_disabled;
#ifdef MTK_MINISUPP_ENABLE
wifi_scan_list_item_t *g_scan_list;
uint8_t g_scan_list_size;
uint8_t g_scan_inited;
#endif /* #ifdef MTK_MINISUPP_ENABLE */
wifi_rx_handler_t connsys_raw_handler = NULL;
#if CFG_SUPPORT_ROAMING_CUSTOMIZED
static int32_t g_roam_by_bcnmiss_threshold = 0;
#endif /* #if CFG_SUPPORT_ROAMING_CUSTOMIZED */

/**
 * @brief judge whether the opmode is valid
 */
bool wifi_is_opmode_valid(uint8_t mode)
{
    return (mode >= WIFI_MODE_STA_ONLY && mode <= WIFI_MODE_P2P_ONLY);
}


/**
 * @brief judge whether the port is valid
 */
bool wifi_is_port_valid(uint8_t port)
{
    return (port == WIFI_PORT_STA || port == WIFI_PORT_AP ||
            port == WIFI_PORT_APCLI);
}

/**
 * @brief judge whether the phy mode is valid
 */
bool wifi_is_phy_mode_valid(wifi_phy_mode_t mode)
{
    return ((mode < WIFI_PHY_MAX_NUMBER && mode >= WIFI_PHY_11NAC_MIXED) ||
            mode == WIFI_PHY_11ABG_MIXED || mode == WIFI_PHY_11ABGN_MIXED);
}

/**
 * @brief judge whether the band is valid
 */
bool wifi_is_band_valid(uint8_t band)
{
    return (band == WIFI_BAND_2_4_G || band == WIFI_BAND_5_G);
}

/**
 * @brief judge whether the bandwidth is valid
 */
bool wifi_is_bandwidth_valid(uint8_t bandwidth)
{
    return (bandwidth <= WIFI_IOT_COMMAND_CONFIG_BANDWIDTH_20MHZ);
}

int32_t wifi_is_mac_address_valid(uint8_t *mac_addr)
{
    uint32_t byte_sum = 0;

    for (uint32_t index = 0; index < WIFI_MAC_ADDRESS_LENGTH; index++)
        byte_sum += mac_addr[index];
    return (byte_sum != 0);
}

/**
 * @brief judge whether the channel is valid
 */
bool wifi_is_channel_valid(uint8_t channel)
{
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
    struct RF_CHANNEL_INFO *aucChannelList;
    uint8_t ucNumOfChannel;
    uint32_t i;
    bool result = FALSE;

    aucChannelList = (struct RF_CHANNEL_INFO *) kalMemAlloc(
                         sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM,
                         VIR_MEM_TYPE);
    if (!aucChannelList) {
        LOG_FUNC("Allocate buffer for channel list fail\n");
        return result;
    }

    rlmDomainGetChnlList(prGlueInfo->prAdapter, BAND_NULL,
                         FALSE, MAX_CHN_NUM, &ucNumOfChannel, aucChannelList);

    for (i = 0; i < ucNumOfChannel; i++) {
        if (channel == aucChannelList[i].ucChannelNum) {
            result = TRUE;
            break;
        }
    }

    kalMemFree(aucChannelList, VIR_MEM_TYPE,
               sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM);

    return result;
}

/**
 * @brief judge whether the channels are valid
 */
bool wifi_is_multi_channel_valid(uint8_t *channel, uint8_t channel_len)
{
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
    struct RF_CHANNEL_INFO *aucChannelList;
    uint8_t ucNumOfChannel;
    uint32_t i;
    bool result = FALSE;
    uint8_t len = 0;

    aucChannelList = (struct RF_CHANNEL_INFO *) kalMemAlloc(
                         sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM,
                         VIR_MEM_TYPE);
    if (!aucChannelList) {
        LOG_FUNC("Allocate buffer for channel list fail\n");
        return result;
    }

    rlmDomainGetChnlList(prGlueInfo->prAdapter, BAND_NULL,
                         FALSE, MAX_CHN_NUM, &ucNumOfChannel, aucChannelList);

    for (len = 0; len < channel_len; len++) {
        for (i = 0; i < ucNumOfChannel; i++) {
            if (channel[len] == aucChannelList[i].ucChannelNum) {
                result = TRUE;
                break;
            } else
                result = FALSE;
        }
    }

    kalMemFree(aucChannelList, VIR_MEM_TYPE,
               sizeof(struct RF_CHANNEL_INFO) * MAX_CHN_NUM);

    return result;
}

/**
 * @brief judge whether the auth mode is valid
 */
bool wifi_is_auth_mode_valid(uint8_t auth_mode)
{
    return (auth_mode < WIFI_AUTH_MODE_MAX);
}

/**
 * @brief judge whether the encrypt type is valid
 */
bool wifi_is_encrypt_type_valid(uint8_t encrypt_type)
{
#ifdef WAPI_SUPPORT
    return (encrypt_type <= WIFI_ENCRYPT_TYPE_ENCRYPT_SMS4_ENABLED);
#else /* #ifdef WAPI_SUPPORT */
    return (encrypt_type <= WIFI_ENCRYPT_TYPE_GROUP_WEP104_ENABLED);
#endif /* #ifdef WAPI_SUPPORT */
}

/**
 * @brief judge whether the power save mode is valid
 */
bool wifi_is_ps_mode_valid(uint8_t ps_mode)
{
    return (ps_mode <= 2);
}

/**
* @brief Get WiFi Operation Mode.
*
* @return  >=0 means success, <0 means fail
*/
static int32_t wifi_config_get_opmode_internal(uint8_t *mode)
{
    if (mode == NULL)
        return WIFI_ERR_PARA_INVALID;

#ifdef MTK_MINISUPP_ENABLE
    wpa_supplicant_entry_op_mode_get(mode);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    return WIFI_SUCC;
}

/**
* @brief Get the current STA port link up / link down status of the connection
*
* @return >=0 means success, <0 means fail
*
*/
static int32_t wifi_connection_get_link_status_internal(uint8_t *link_status)
{
    int32_t status = 0;

    if (link_status == NULL)
        return WIFI_ERR_PARA_INVALID;

#ifdef MTK_MINISUPP_ENABLE
    status = mtk_supplicant_get_state(link_status);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    return status;
}

/**
* @brief WiFi Radio ON/OFF
*
* @return  >=0 means success, <0 means fail
*
*/
static int32_t wifi_config_get_radio_interal(uint8_t *on_off)
{
    uint8_t opmode = 0;
    int32_t status = 0;

    if (wifi_config_get_opmode_internal(&opmode) < 0) {
        status = WIFI_FAIL;
        goto final;
    }

    if (on_off == NULL) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }

#ifdef MTK_MINISUPP_ENABLE
    if (WIFI_MODE_STA_ONLY == opmode || WIFI_MODE_AP_ONLY == opmode ||
        WIFI_MODE_REPEATER == opmode ||
        WIFI_MODE_MONITOR == opmode) {
        status = mtk_supplicant_get_radio(on_off);
        goto final;
    } else {
        status = WIFI_ERR_NOT_SUPPORT;
        goto final;
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}

/**
* @brief Set WiFi Operation Mode.
*
* @param [IN]mode
* @param 0x01 WIFI_MODE_STA_ONLY
* @param 0x02 WIFI_MODE_AP_ONLY
* @param 0x03 WIFI_MODE_REPEATER
* @param 0x04 WIFI_MODE_MONITOR
*
* @return  >=0 means success, <0 means fail
*
* @note Set WiFi Operation Mode will RESET all the configuration set by
*       previous WIFI-CONFIG APIs
* @note The mt7682 and mt7687 have different behavior. mt7687 use
*       opmode_switch_result_semphr to sync with minisupp
*/

int32_t wifi_config_set_opmode(uint8_t mode)
{
    int32_t status = 0;
    uint8_t current_mode = 0;

    if (!wifi_is_opmode_valid(mode))
        return WIFI_ERR_PARA_INVALID;

    if (mode > WIFI_MODE_REPEATER) {
        LOG_FUNC("The mode %d is not supported.\n", mode);
        return WIFI_ERR_PARA_INVALID;
    }

    wifi_config_get_opmode_internal(&current_mode);
#ifdef MTK_MINISUPP_ENABLE
    if (current_mode == WIFI_MODE_AP_ONLY && g_ap_created == 0) {
        LOG_FUNC("The AP is disable.\n");
        return WIFI_ERR_PARA_INVALID;
    }

    struct timeval wait_time = {5, 0};

    if (opmode_switch_mutex != NULL && (0 !=
                                        wifi_os_semphr_take(opmode_switch_mutex, &wait_time)))
        return  WIFI_FAIL;

    uint8_t driver_msg[5] = {0};
    int32_t ret_value = 0;
    uint32_t ret_addr = (uint32_t)&ret_value;

    driver_msg[0] = mode;
    os_memcpy(&driver_msg[1], (char *)(&ret_addr), 4);

    opmode_switch_result_semphr = wifi_os_semphr_create_binary();
    if (opmode_switch_result_semphr == NULL) {
        wifi_os_semphr_give(opmode_switch_mutex);
        return WIFI_FAIL;
    }

    if (__process_global_event[WIFI_PORT_STA].func != NULL)
        status = wifi_event_notification_wait(WIFI_PORT_STA,
                                              WIFI_EVENT_ID_IOT_OPMODE_SWITCH,
                                              (unsigned char *)&driver_msg, sizeof(driver_msg));
    else if (__process_global_event[WIFI_PORT_AP].func != NULL)
        status = wifi_event_notification_wait(WIFI_PORT_AP,
                                              WIFI_EVENT_ID_IOT_OPMODE_SWITCH,
                                              (unsigned char *)&driver_msg, sizeof(driver_msg));
    else
        status = WIFI_FAIL;

    if (status != 0) {
        if (opmode_switch_mutex != NULL)
            wifi_os_semphr_give(opmode_switch_mutex);
        return WIFI_FAIL;
    } else {
        if (wifi_os_semphr_take(opmode_switch_result_semphr, &wait_time) == 0)
            status = ret_value;
        else
            status = WIFI_FAIL;
        wifi_os_semphr_delete(opmode_switch_result_semphr);
        opmode_switch_result_semphr = NULL;
        wifi_os_semphr_give(opmode_switch_mutex);
    }

#endif /* #ifdef MTK_MINISUPP_ENABLE */
#if IP_NAPT
    if (mode == WIFI_MODE_REPEATER)
        mtk_tcpip_enable_napt_clean_entry_timer();
    else
        mtk_tcpip_disable_napt_clean_entry_timer();
#endif /* #if IP_NAPT */

    return status;
}


/**
* @brief Get WiFi Operation Mode.
*
* @param [OUT]mode
* @param 0x01 WIFI_MODE_AP_ONLY
* @param 0x02 WIFI_MODE_STA_ONLY
* @param 0x03 WIFI_MODE_REPEATER
* @param 0x04 WIFI_MODE_MONITOR
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_get_opmode(uint8_t *mode)
{
    int32_t status = 0;

#ifdef MTK_MINISUPP_ENABLE
    status = wifi_config_get_opmode_internal(mode);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    return status;
}

/**
* @brief get mac address from efuse
*/
int32_t wifi_get_mac_addr_from_efuse(uint8_t port, uint8_t *mac_addr)
{
#ifdef CFG_ENABLE_EFUSE_MAC_ADDR
    struct GLUE_INFO *prGlueInfo;

    prGlueInfo = &g_rGlueInfo;
    if (prGlueInfo->prAdapter->fgIsEmbbededMacAddrValid) {
        COPY_MAC_ADDR(mac_addr,
                      prGlueInfo->prAdapter->rWifiVar.aucMacAddress);
        return WIFI_SUCC;
    }
    return WIFI_FAIL;
#else /* #ifdef CFG_ENABLE_EFUSE_MAC_ADDR */
    return WIFI_FAIL;
#endif /* #ifdef CFG_ENABLE_EFUSE_MAC_ADDR */
}

/**
* @brief get mac address from nvdm
*/
int32_t wifi_get_mac_addr_from_nvdm(uint8_t port, uint8_t *mac_addr)
{
#ifdef MTK_NVDM_ENABLE
    uint8_t buff[PROFILE_BUF_LEN] = {0};
    uint32_t len = sizeof(buff);
    char *group_name = (port == 0) ? "STA" : "AP";

    if (nvdm_read_data_item(group_name, "MacAddr", buff, &len) !=
        NVDM_STATUS_OK)
        return WIFI_FAIL;

    wifi_conf_get_mac_from_str((char *)mac_addr, (char *)buff);
    if (!mac_addr[0] && !mac_addr[1] && !mac_addr[2] &&
        !mac_addr[3] && !mac_addr[4] && !mac_addr[5])
        return WIFI_FAIL;
    return WIFI_SUCC;
#else /* #ifdef MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #ifdef MTK_NVDM_ENABLE */
}

/**
* @brief Get WiFi Interface MAC Address.
*
*/
int32_t wifi_config_get_mac_address(uint8_t port, uint8_t *address)
{
    struct GLUE_INFO *prGlueInfo;

    prGlueInfo = &g_rGlueInfo;

    if (address == NULL) {
        LOG_FUNC("address is null.\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }

    if (port == WIFI_PORT_STA)
        COPY_MAC_ADDR(address, prGlueInfo->prAdapter->rWifiVar.aucMacAddress);
#if CFG_ENABLE_WIFI_DIRECT
    else if (port == WIFI_PORT_AP) {
        struct BSS_INFO *prBssInfo;
        uint8_t ucBssIndex;

        ucBssIndex = p2pFuncGetSapBssIndex(prGlueInfo);
        prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
        if (!prBssInfo)
            return WIFI_FAIL;

        COPY_MAC_ADDR(address, prBssInfo->aucOwnMacAddr);
    }
#endif /* #if CFG_ENABLE_WIFI_DIRECT */

    return WIFI_SUCC;
}

/**
* @brief This function initializes the Wi-Fi module.
*
* @param[in] config is the Wi-Fi configuration to be set, it should not be null.
*
* @param[in] config_ext is the extended Wi-Fi configuration to be set, it can be
* null if no extended features are expected.
*
* @note Call this function only once at the initialization stage.
*/
static uint8_t g_u1OpMode_backup = 0;
void wifi_init(wifi_config_t *config, wifi_config_ext_t *config_ext)
{
#ifdef MTK_MINISUPP_ENABLE
    int ret = pdFALSE;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    opmode = config->opmode;
    _wifi_on(0, NULL);
#ifdef MTK_MINISUPP_ENABLE
    if (wifi_inited == 1) {
        LOG_FUNC("wifi re-init.\n");
        if (wifi_disabled == 1) {
#if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P)
            if (g_wait_drv_ready) {
                LOG_FUNC("wait drv ready\r\n");
                ret = xSemaphoreTake(g_wait_drv_ready, WLAN_WAIT_LOCK_TIME);
                if (ret != TRUE)
                    LOG_FUNC("take drv ready failed\r\n");
                else
                    xSemaphoreGive(g_wait_drv_ready);
            }
#endif /* #if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P) */
            LOG_FUNC("radio on\n");
            wifi_config_set_radio(1);

            /* restore opmode if original op_mode is not support radio off */
            if (g_u1OpMode_backup != WIFI_MODE_STA_ONLY &&
                g_u1OpMode_backup != WIFI_MODE_AP_ONLY &&
                g_u1OpMode_backup != WIFI_MODE_REPEATER)
                wifi_config_set_opmode(g_u1OpMode_backup);

            /* if original is STA_only or repeater,
             * reload to trigger sta reconnect
             */
            if (g_u1OpMode_backup == WIFI_MODE_STA_ONLY ||
                g_u1OpMode_backup == WIFI_MODE_REPEATER)
                wifi_config_reload_setting();
            wifi_disabled = 0;
        }
    } else {
        wpa_supplicant_task_init(config, config_ext);
        wifi_inited = 1;
        wifi_disabled = 0;
        mtk_freertos_api2supp_semphr_take();
        wifi_api_event_trigger(WIFI_PORT_STA,
                               WIFI_EVENT_IOT_INIT_COMPLETE, NULL, 0);
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */
}

void wifi_deinit(uint8_t type_deinit)
{
    if (wifi_inited == 0) {
        LOG_FUNC("wifi deinited.\n");
        return;
    }

    wifi_config_get_opmode(&g_u1OpMode_backup);

    if (g_u1OpMode_backup != WIFI_MODE_STA_ONLY &&
        g_u1OpMode_backup != WIFI_MODE_AP_ONLY &&
        g_u1OpMode_backup != WIFI_MODE_REPEATER)
        wifi_config_set_opmode(WIFI_MODE_STA_ONLY);
    wifi_config_set_radio(0);
    wifi_disabled = 1;

    LOG_FUNC("radio off\n");

    if (type_deinit == WIFI_MODULE_REMOVAL) {
        if (xSemaphoreTake(g_halt_sem, 2000 / portTICK_PERIOD_MS) != true) {
            DBGLOG(INIT, ERROR,
                   "wifi deinit: g_halt_sem\n");
        } else {
            g_u4HaltFlag = 1;
            xSemaphoreGive(g_halt_sem);
        }
        _wifi_off(0, NULL);
    }

}


/**
* @brief Set WiFi SSID.
*
* @param [IN]port
* @param 0 AP
* @param 1 AP Client
* @param 2 STA
* @param [IN]ssid SSID
* @param [IN]ssid_length Length of SSID
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_ssid(uint8_t port, uint8_t *ssid, uint8_t ssid_length)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (ssid_length > 32) {
        status =  WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (ssid == NULL) {
        status =  WIFI_ERR_PARA_INVALID;
        goto final;
    }


#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[WIFI_MAX_LENGTH_OF_SSID + 2] = {0};

    driver_msg[0] = port;
    driver_msg[1] = ssid_length;
    os_memcpy((driver_msg + 2), ssid, ssid_length);
    if (wifi_event_notification_wait(port, WIFI_EVENT_ID_IOT_SET_SSID_WAIT,
                                     (unsigned char *)&driver_msg, sizeof(driver_msg)) != WIFI_SUCC)
        status = WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}

/**
* @brief Get WiFi SSID.
*
* @param [IN]port
* @param 0 AP
* @param 1 AP Client
* @param 2 STA
* @param [OUT]ssid SSID
* @param [OUT]ssid_length Length of SSID
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_get_ssid(uint8_t port, uint8_t *ssid, uint8_t *ssid_length)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if ((ssid == NULL) || (ssid_length == NULL)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }

#ifdef MTK_MINISUPP_ENABLE
    status = mtk_supplicant_get_ssid(port, ssid, ssid_length);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}

/**
* @brief set WiFi Authetication Mode and Encryption Type.
*
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [OUT]auth_mode
* @param OPEN
* @param SHARED In fact, it also support at MT7687/7697 STA mode
* @param AUTOSWITCH In MT7687/7697 STA mode, AUTO_WEP can suitable for
*        SHARE/OPEN, it will frist use SHARE to connect, if fail then use OPEN
* @param WPA
* @param WPAPSK
* @param WPA2
* @param WPA2PSK
* @param WPA1WPA2
* @param WPA1PSKWPA2PSK
* @param [OUT]encrypt_type
* @param WEP
* @param TKIP
* @param AES
* @param TKIP/AES Mixed Mode
*
* @return >=0 means success, <0 means fail
* @note   MT7687/7697, it is not support SHARED WEP in AP mode, but
*         support it in STA mode. Both support OPEN WEP.
*/
int32_t wifi_config_set_security_mode(uint8_t port,
                                      wifi_auth_mode_t auth_mode, wifi_encrypt_type_t encrypt_type)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (!wifi_is_auth_mode_valid(auth_mode)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (!wifi_is_encrypt_type_valid(encrypt_type)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (port == WIFI_PORT_STA)
        security_use_flag = true;

#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[3] = {0};

    driver_msg[0] = port;
    driver_msg[1] = auth_mode;
    driver_msg[2] = encrypt_type;
    if (wifi_event_notification_wait(port,
                                     WIFI_EVENT_ID_IOT_SET_SECURITY_WAIT, (unsigned char *)&driver_msg,
                                     sizeof(driver_msg)) != WIFI_SUCC)
        status = WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}

/**
* @brief set WiFi Multiple Authetication Modes and Encryption Type.
*
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [IN]auth_mode
* @param WPA_KEY_MGMT_IEEE8021X
* @param WPA_KEY_MGMT_PSK
* @param WPA_KEY_MGMT_NONE
* @param WPA_KEY_MGMT_IEEE8021X_NO_WPA
* @param WPA_KEY_MGMT_WPA_NONE
* @param [IN]encrypt_type
* @param WEP
* @param TKIP
* @param AES
* @param TKIP/AES Mixed Mode
*
* @return >=0 means success, <0 means fail
* @note   MT7687/7697, it is not support SHARED WEP in AP mode, but
*         support it in STA mode. Both support OPEN WEP.
*/
int32_t wifi_config_set_multi_security_mode(uint8_t port,
                                            uint32_t auth_mode,
                                            wifi_encrypt_type_t encrypt_type)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
#if 0
    if (!wifi_is_auth_mode_valid(auth_mode)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
#endif /* #if 0 */
    if (!wifi_is_encrypt_type_valid(encrypt_type)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (port == WIFI_PORT_STA)
        security_use_flag = true;

#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[6] = {0};

    driver_msg[0] = port;
    driver_msg[1] = auth_mode & 0xff;
    driver_msg[2] = (auth_mode >> 8) & 0xff;
    driver_msg[3] = (auth_mode >> 16) & 0xff;
    driver_msg[4] = (auth_mode >> 24) & 0xff;
    driver_msg[5] = encrypt_type;
    if (wifi_event_notification_wait(port,
                                     WIFI_EVENT_ID_IOT_SET_MULTI_SECURITY_WAIT,
                                     (unsigned char *)&driver_msg, sizeof(driver_msg)) != WIFI_SUCC)
        status = WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}


/**
* @brief Set WiFi ieee80211w.
*
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [IN]ieee80211w
* @param 0
* @param 1
* @param 2
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_ieee80211w(uint8_t port, uint8_t ieee80211w)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (port == WIFI_PORT_STA)
        security_use_flag = true;

#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[2] = {0};

    driver_msg[0] = port;
    driver_msg[1] = ieee80211w;
    if (wifi_event_notification_wait(port,
                                     WIFI_EVENT_ID_IOT_SET_IEEE80211W_WAIT,
                                     (unsigned char *)&driver_msg, sizeof(driver_msg)) != WIFI_SUCC)
        status = WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}


/**
* @brief Set WiFi proto.
*
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [IN]proto
* @param WPA_PROTO_WPA
* @param WPA_PROTO_RSN
* @param WPA_PROTO_WAPI
* @param WPA_PROTO_OSEN
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_proto(uint8_t port, uint8_t proto)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (port == WIFI_PORT_STA)
        security_use_flag = true;

#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[2] = {0};

    driver_msg[0] = port;
    driver_msg[1] = proto;
    if (wifi_event_notification_wait(port,
                                     WIFI_EVENT_ID_IOT_SET_PROTO_WAIT,
                                     (unsigned char *)&driver_msg, sizeof(driver_msg)) != WIFI_SUCC)
        status = WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}


#if CFG_SUPPORT_NON_PREF_CHAN
/**
* @brief Set non preferred channel.
*
* @param [IN]port
* @param 0 AP
* @param 1 AP Client
* @param 2 STA
* @param [IN]non preferred channel {OpClass}:{ChannelNum}:{Preference}:{ReasonCode}
* @param [IN]length of non preferred channel
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_non_pref_chan(uint8_t port, uint8_t *non_pref_chan,
                                      uint8_t non_pref_chan_length)
{
    if (!wifi_is_port_valid(port) || port != WIFI_PORT_STA ||
        non_pref_chan_length > WIFI_MAX_LENGTH_OF_NON_PREF_CHAN_BUF)
        return WIFI_ERR_PARA_INVALID;

#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[WIFI_MAX_LENGTH_OF_NON_PREF_CHAN_BUF + 2] = {0};

    driver_msg[0] = port;
    driver_msg[1] = non_pref_chan_length;
    if (non_pref_chan_length)
        os_memcpy((driver_msg + 2), non_pref_chan, non_pref_chan_length);
    if (wifi_event_notification_wait(port, WIFI_EVENT_ID_IOT_SET_NON_PREF_CAHN_WAIT,
                                     (unsigned char *)&driver_msg, sizeof(driver_msg)) != WIFI_SUCC)
        return WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    return 0;
}
#endif /* #if CFG_SUPPORT_NON_PREF_CHAN */


#if CFG_SUPPORT_11KV_SWITCH
/**
* @brief Set disable 11k config.
*
* @param [IN]port
* @param 0 AP
* @param 1 AP Client
* @param 2 STA
* @param [IN]val
* @param 0 enable
* @param 1 disable
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_disable_11k(uint8_t port, uint8_t val)
{
    struct GLUE_INFO *prGlueInfo;

    if (!wifi_is_port_valid(port) || port != WIFI_PORT_STA)
        return WIFI_ERR_PARA_INVALID;

    prGlueInfo = &g_rGlueInfo;

    if (val != 0 && val != 1) {
        LOG_FUNC("invalid value\n");
        return WIFI_ERR_PARA_INVALID;
    }

    prGlueInfo->prAdapter->rWifiVar.ucConfigDisable11K = val;

    return 0;
}


/**
* @brief Set disable 11v config.
*
* @param [IN]port
* @param 0 AP
* @param 1 AP Client
* @param 2 STA
* @param [IN]val
* @param 0 enable
* @param 1 disable
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_disable_11v(uint8_t port, uint8_t val)
{
    struct GLUE_INFO *prGlueInfo;

    if (!wifi_is_port_valid(port) || port != WIFI_PORT_STA)
        return WIFI_ERR_PARA_INVALID;

    prGlueInfo = &g_rGlueInfo;

    if (val != 0 && val != 1) {
        LOG_FUNC("invalid value\n");
        return WIFI_ERR_PARA_INVALID;
    }

    prGlueInfo->prAdapter->rWifiVar.ucConfigDisable11V = val;

    return 0;
}


/**
* @brief Get disable 11k.
*
* @param [IN]port
* @param 0 AP
* @param 1 AP Client
* @param 2 STA
* @param [OUT]prVal
* @param 0 enable
* @param 1 disable
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_get_disable_11k(uint8_t port, uint8_t *prVal)
{
    struct GLUE_INFO *prGlueInfo;

    if (!wifi_is_port_valid(port) || port != WIFI_PORT_STA || prVal == NULL) {
        LOG_FUNC("invalid parameter\n");
        return WIFI_ERR_PARA_INVALID;
    }

    prGlueInfo = &g_rGlueInfo;

    *prVal = prGlueInfo->prAdapter->rWifiVar.ucDisable11K;

    return 0;
}


/**
* @brief Get disable 11v.
*
* @param [IN]port
* @param 0 AP
* @param 1 AP Client
* @param 2 STA
* @param [OUT]prVal
* @param 0 enable
* @param 1 disable
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_get_disable_11v(uint8_t port, uint8_t *prVal)
{
    struct GLUE_INFO *prGlueInfo;

    if (!wifi_is_port_valid(port) || port != WIFI_PORT_STA || prVal == NULL) {
        LOG_FUNC("invalid parameter\n");
        return WIFI_ERR_PARA_INVALID;
    }

    prGlueInfo = &g_rGlueInfo;

    *prVal = prGlueInfo->prAdapter->rWifiVar.ucDisable11V;

    return 0;
}
#endif /* #if CFG_SUPPORT_11KV_SWITCH */


/**
* @brief Get WiFi Authetication Mode and Encryption Type.
*
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [OUT]auth_mode
* @param OPEN
* @param SHARED
* @param AUTOSWITCH
* @param WPA
* @param WPAPSK
* @param WPA2
* @param WPA2PSK
* @param WPA1WPA2
* @param WPA1PSKWPA2PSK
* @param [OUT]encrypt_type
* @param WEP
* @param TKIP
* @param AES
* @param TKIP/AES Mixed Mode
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_get_security_mode(uint8_t port,
                                      wifi_auth_mode_t *auth_mode, wifi_encrypt_type_t *encrypt_type)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if ((auth_mode == NULL) || (encrypt_type == NULL)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }

#ifdef MTK_MINISUPP_ENABLE
    status = mtk_supplicant_get_security(port, auth_mode, encrypt_type);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}


/**
* @brief set WIFI WPAPSK/WPA2PSK passphrase.
*
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [IN]passphrase 8 ~ 63 bytes ASCII or 64 bytes Hex
* @param [IN]passphrase_length 8 ~ 64
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_wpa_psk_key(uint8_t port, uint8_t *passphrase,
                                    uint8_t passphrase_length)
{
    int32_t status = 0;
    uint8_t i;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (passphrase == NULL) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if ((passphrase_length < 8) ||
        (passphrase_length > WIFI_LENGTH_PASSPHRASE)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (passphrase_length == WIFI_LENGTH_PASSPHRASE) {
        for (i = 0; i < WIFI_LENGTH_PASSPHRASE; i++) {
            if (!hex_isdigit(passphrase[i])) {
                status = WIFI_ERR_PARA_INVALID;
                goto final;
            }
        }
    }


#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[WIFI_LENGTH_PASSPHRASE + 2] = {0};

    driver_msg[0] = port;
    driver_msg[1] = passphrase_length;
    os_memcpy((driver_msg + 2), passphrase, passphrase_length);
    if (wifi_event_notification_wait(port, WIFI_EVENT_ID_IOT_SET_WPAPSK_WAIT,
                                     (unsigned char *)&driver_msg, WIFI_LENGTH_PASSPHRASE + 2) != WIFI_SUCC)
        status = WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}


/**
* @brief Get WIFI WPAPSK/WPA2PSK passphrase.
*
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [OUT]passphrase 8 ~ 63 bytes ASCII or 64 bytes Hex
* @param [OUT]passphrase_length 8 ~ 64
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_get_wpa_psk_key(uint8_t port, uint8_t *passphrase,
                                    uint8_t *passphrase_length)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if ((passphrase == NULL) || (passphrase_length == NULL)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }

#ifdef MTK_MINISUPP_ENABLE
    status = mtk_supplicant_get_wpa_psk_key(port, passphrase,
                                            passphrase_length);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}

/**
* @brief Set the PMK key.
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [IN]pmk
*
* @return >=0 means success, <0 means fail
*/
int32_t wifi_config_set_pmk(uint8_t port, uint8_t *pmk)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (pmk == NULL) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }

#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[WIFI_LENGTH_PMK + 1] = {0};

    driver_msg[0] = port;
    os_memcpy(driver_msg + 1, pmk, WIFI_LENGTH_PMK);
    if (wifi_event_notification_wait(port, WIFI_EVENT_ID_IOT_SET_PMK_WAIT,
                                     (unsigned char *)&driver_msg, sizeof(driver_msg)) != WIFI_SUCC)
        status = WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}


/**
* @brief Get the PMK key.
* @param [IN]port
* @param 0 STA / AP Client
* @param 1 AP
* @param [OUT]pmk
*
* @return >=0 means success, <0 means fail
*/
int32_t wifi_config_get_pmk(uint8_t port, uint8_t *pmk)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (pmk == NULL) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
#ifdef MTK_MINISUPP_ENABLE
    status = mtk_supplicant_get_pmk(port, pmk);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}

/**
* @brief Set WiFi WEP Keys
*
* @param [IN]wifi_wep_key_t
*
* @return >=0 means success, <0 means fail
*/
int32_t wifi_config_set_wep_key(uint8_t port, wifi_wep_key_t *wep_keys)
{
    int32_t status = 0;

    if (wep_keys == NULL) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (wep_keys->wep_tx_key_index >= WIFI_NUMBER_WEP_KEYS) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (wep_keys->wep_key_length[wep_keys->wep_tx_key_index] == 10 ||
        wep_keys->wep_key_length[wep_keys->wep_tx_key_index] == 26) {
        for (uint8_t index = 0; index < wep_keys->wep_key_length[wep_keys->wep_tx_key_index]; index++) {
            if (!hex_isdigit(wep_keys->wep_key[wep_keys->wep_tx_key_index][index])) {
                status = WIFI_ERR_PARA_INVALID;
                goto final;
            }
        }
        wep_keys->wep_key_length[wep_keys->wep_tx_key_index] >>= 1;
        AtoH((char *)wep_keys->wep_key[wep_keys->wep_tx_key_index],
             (char *)wep_keys->wep_key[wep_keys->wep_tx_key_index],
             (int)wep_keys->wep_key_length[wep_keys->wep_tx_key_index]);
    }

#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[sizeof(wifi_wep_key_t) +1] = {0};

    driver_msg[0] = port;
    os_memcpy(driver_msg + 1, wep_keys, sizeof(wifi_wep_key_t));
    if (wifi_event_notification_wait(port, WIFI_EVENT_ID_IOT_SET_WEPKEY_WAIT,
                                     (unsigned char *)&driver_msg, sizeof(driver_msg)) != WIFI_SUCC)
        status = WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}


/**
* @brief Get WiFi WEP Keys
*
* @param [OUT]wifi_wep_key_t
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_get_wep_key(uint8_t port, wifi_wep_key_t *wep_keys)
{
    int32_t status = 0;

    if (!wifi_is_port_valid(port)) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if (wep_keys == NULL) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }

#ifdef MTK_MINISUPP_ENABLE
    status = mtk_supplicant_get_wep_key(port, wep_keys);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}

/**
* @brief Set WiFi BSSID.
*
* @param [IN]bssid the targrt BSSID
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_bssid(uint8_t *bssid)
{
    int32_t status = 0;
    uint8_t mode = 0;

    if (bssid == NULL) {
        status = WIFI_ERR_PARA_INVALID;
        goto final;
    }
    if ((wifi_config_get_opmode_internal(&mode) < 0) ||
        (mode == WIFI_MODE_AP_ONLY) || (mode == WIFI_MODE_MONITOR)) {
        LOG_FUNC("Not support mode");
        status = WIFI_FAIL;
        goto final;
    }
    bssid_use_flag = true;

#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[WIFI_MAC_ADDRESS_LENGTH] = {0};

    os_memcpy(driver_msg, bssid, WIFI_MAC_ADDRESS_LENGTH);
    if (wifi_event_notification_wait(WIFI_PORT_STA,
                                     WIFI_EVENT_ID_IOT_SET_BSSID_WAIT, (unsigned char *)&driver_msg,
                                     sizeof(driver_msg)) != WIFI_SUCC)
        status = WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}

/**
* @brief Get WiFi BSSID.
*
* @param [OUT]bssid the targrt BSSID.
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_get_bssid(uint8_t *bssid)
{
    struct GLUE_INFO *prGlueInfo;
    struct ADAPTER *prAdapter = NULL;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex = 0;
#ifdef MTK_MINISUPP_ENABLE
    uint8_t opmode;

    if (wifi_config_get_opmode(&opmode) < 0)
        return WIFI_FAIL;

    if (opmode == WIFI_MODE_AP_ONLY)
        return WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    prGlueInfo = &g_rGlueInfo;
    prAdapter = prGlueInfo->prAdapter;

    ucBssIndex = prAdapter->prAisBssInfo[0]->ucBssIndex;
    prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

    if (prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) {
        bssid[0] = prBssInfo->aucBSSID[0];
        bssid[1] = prBssInfo->aucBSSID[1];
        bssid[2] = prBssInfo->aucBSSID[2];
        bssid[3] = prBssInfo->aucBSSID[3];
        bssid[4] = prBssInfo->aucBSSID[4];
        bssid[5] = prBssInfo->aucBSSID[5];
        return WIFI_SUCC;
    }
    return WIFI_FAIL;
}

/**
* @brief Configure bandwidth for STA wireless port.
*
* @parameter
*     [IN] bandwidth   IOT_CMD_CBW_20MHZ, IOT_CMD_CBW_40MHZ,
        IOT_CMD_CBW_2040MHZ are supported
* @return >=0 means success, <0 means fail
* @note Default value will be HT_20
*/
int32_t wifi_config_set_bandwidth(uint8_t port, uint8_t bandwidth)
{
    struct GLUE_INFO *prGlueInfo;
    struct ADAPTER *prAdapter = NULL;

    prGlueInfo = &g_rGlueInfo;
    prAdapter = prGlueInfo->prAdapter;

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d", port);
        return WIFI_ERR_PARA_INVALID;
    }

    if (!wifi_is_bandwidth_valid(bandwidth)) {
        LOG_FUNC("bandwidth is invalid: %d", bandwidth);
        return WIFI_ERR_PARA_INVALID;
    }

    if (port == WIFI_PORT_STA)
        prAdapter->rWifiVar.ucStaBandwidth = bandwidth;
    else
        prAdapter->rWifiVar.ucApBandwidth = bandwidth;

    return WIFI_SUCC;
}

/**
* @brief Configure bandwidth for STA wireless port.
*
* @parameter
*     [OUT] bandwidth  IOT_CMD_CBW_20MHZ, IOT_CMD_CBW_40MHZ,
        IOT_CMD_CBW_2040MHZ are supported
* @return >=0 means success, <0 means fail
* @note Default value will be HT_20
*/
int32_t wifi_config_get_bandwidth(uint8_t port, uint8_t *bandwidth)
{
    struct GLUE_INFO *prGlueInfo;
    struct ADAPTER *prAdapter = NULL;

    prGlueInfo = &g_rGlueInfo;
    prAdapter = prGlueInfo->prAdapter;

    if (bandwidth == NULL) {
        LOG_FUNC("bandwidth is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (port == WIFI_PORT_STA)
        *bandwidth = prAdapter->rWifiVar.ucStaBandwidth;
    else
        *bandwidth = prAdapter->rWifiVar.ucApBandwidth;

    return WIFI_SUCC;
}

/**
* @brief This function sets the channel number that the Wi-Fi driver uses
*        for a specific wireless port.
* @parameter
*     [IN] channel  1~14 are supported for 2.4G
*                   36~165 are supported for 5G
* @return >=0 means success, <0 means fail
*/
int32_t wifi_config_set_channel(uint8_t port, uint8_t channel)
{
    int32_t status = 0;
#ifdef MTK_MINISUPP_ENABLE
    uint8_t link_status;
    uint8_t opmode;

    if (wifi_config_get_opmode(&opmode) < 0)
        return WIFI_FAIL;
    if (!wifi_is_port_valid(port))
        return WIFI_ERR_PARA_INVALID;
    if (!wifi_is_channel_valid(channel)) {
        LOG_FUNC("Channel %d in not allowed!", channel);
        return WIFI_ERR_PARA_INVALID;
    }
    if ((opmode == WIFI_MODE_STA_ONLY || opmode == WIFI_MODE_REPEATER)
        && port == WIFI_PORT_STA) {
        wifi_connection_get_link_status_internal(&link_status);
        if (link_status == 1) {
            wifi_connection_disconnect_ap();
            mtk_supplicant_set_channel(port, channel);
            status = wifi_connection_start_scan(NULL, 0,
                                                NULL, 0, 0);
        } else {
            mtk_supplicant_set_channel(port, channel);
            status = wifi_connection_start_scan(NULL, 0,
                                                NULL, 0, 0);
        }
    }
    if ((opmode == WIFI_MODE_AP_ONLY || opmode == WIFI_MODE_REPEATER)
        && port == WIFI_PORT_AP) {
        mtk_supplicant_set_channel(port, channel);
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return status;
}


/**
* @brief Get the current channel for STA wireless port.
*
* @parameter
*     [OUT] channel I1~14 are supported for 2.4G only product
* @return >=0 means success, <0 means fail
*/
int32_t wifi_config_get_channel(uint8_t port, uint8_t *channel)
{
    struct GLUE_INFO *prGlueInfo;
    struct ADAPTER *prAdapter = NULL;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex = 0;

    prGlueInfo = &g_rGlueInfo;
    prAdapter = prGlueInfo->prAdapter;

    ucBssIndex = prAdapter->prAisBssInfo[0]->ucBssIndex;
    prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

    if (prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED)
        *channel = prBssInfo->ucPrimaryChannel;

    return WIFI_SUCC;
}

/**
* @brief This function sets the channel number that the Wi-Fi driver uses
*        for STA port.
* @parameter
*     [IN] channel  1~14 are supported for 2.4G
*                   36~165 are supported for 5G
* @return >=0 means success, <0 means fail
*/
int32_t wifi_config_set_multi_channel(uint8_t port, uint8_t *channel, uint8_t channel_len)
{
    int32_t status = 0;
#ifdef MTK_MINISUPP_ENABLE
    uint8_t link_status;
    uint8_t opmode;

    if (wifi_config_get_opmode(&opmode) < 0)
        return WIFI_FAIL;
    if (!wifi_is_port_valid(port))
        return WIFI_ERR_PARA_INVALID;
    if (!wifi_is_multi_channel_valid(channel, channel_len)) {
        LOG_FUNC("Channel not allowed!");
        return WIFI_ERR_PARA_INVALID;
    }
    if ((opmode == WIFI_MODE_STA_ONLY || opmode == WIFI_MODE_REPEATER)
        && port == WIFI_PORT_STA) {
        wifi_connection_get_link_status_internal(&link_status);
        if (link_status == 1) {
            wifi_connection_disconnect_ap();
            mtk_supplicant_set_multi_channel(port, channel, channel_len);
            status = wifi_connection_start_scan(NULL, 0,
                                                NULL, 0, 0);
        } else {
            mtk_supplicant_set_multi_channel(port, channel, channel_len);
            status = wifi_connection_start_scan(NULL, 0,
                                                NULL, 0, 0);
        }
    } else {
        LOG_FUNC("Not support.");
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return status;
}

/**
* @brief Get the channel bandwidth of connecting channel.
*
* @parameter
*     [OUT] channel bandwidth
* @return >=0 means success, <0 means fail
*/
int32_t wifi_config_get_chbw(uint8_t port, uint8_t *chbw)
{
    struct GLUE_INFO *prGlueInfo;
    struct ADAPTER *prAdapter = NULL;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex = 0;

    prGlueInfo = &g_rGlueInfo;
    prAdapter = prGlueInfo->prAdapter;

    ucBssIndex = prAdapter->prAisBssInfo[0]->ucBssIndex;
    prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

    if (chbw == NULL) {
        LOG_FUNC("bandwidth is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (prBssInfo->eConnectionState != MEDIA_STATE_CONNECTED) {
        return WIFI_FAIL;
    }

    *chbw = cnmGetBssMaxBw(prAdapter, ucBssIndex);

    return WIFI_SUCC;
}

/**
* @brief Function to map the wireless mode.
* @params[in] flag: 0:get, 1:set
* @return 0 11ABGN default
* @return 1 11N
* @return 3 11N/AC mixed
* @return 4 11AX
* @return 5 11N/AX mixed
* @return 7 11N/AC/AX mixed
*/
uint8_t wifi_wireless_mode_mapping(uint8_t flag, uint8_t mode)
{
    if (flag) {
        switch (mode) {
            case WIFI_PHY_11ABG_MIXED:
                mode = 0;
                break;
            case WIFI_PHY_11ABGN_MIXED:
                mode = 1;
                break;
            case WIFI_PHY_11NAC_MIXED:
                mode = 3;
                break;
            case WIFI_PHY_11AX:
                mode = 4;
                break;
            case WIFI_PHY_11NAX_MIXED:
                mode = 5;
                break;
            case WIFI_PHY_11NACAX_MIXED:
                mode = 7;
                break;
            default:
                LOG_FUNC("Not supported.");
        }
    } else {
        switch (mode) {
            case 0:
                mode = WIFI_PHY_11ABG_MIXED;
                break;
            case 1:
                mode = WIFI_PHY_11ABGN_MIXED;
                break;
            case 3:
                mode = WIFI_PHY_11NAC_MIXED;
                break;
            case 4:
                mode = WIFI_PHY_11AX;
                break;
            case 5:
                mode = WIFI_PHY_11NAX_MIXED;
                break;
            case 7:
                mode = WIFI_PHY_11NACAX_MIXED;
                break;
            default:
                LOG_FUNC("Not matched.");
        }
    }
    return mode;
}

/*
Function to set the wireless mode.
*/
void wifi_set_wireless_mode(uint8_t port, uint8_t mode)
{
    struct GLUE_INFO *prGlueInfo;
    struct WIFI_VAR *prWifiVar;

    prGlueInfo = &g_rGlueInfo;
    prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

    mode = wifi_wireless_mode_mapping(1, mode);
    if (port == WIFI_PORT_STA) {
        if ((mode & WIFI_MODE_80211N) == 0x1)
            prWifiVar->ucStaHt = 1;
        else
            prWifiVar->ucStaHt = 0;
        if ((mode & WIFI_MODE_80211AC) == 0x2)
            prWifiVar->ucStaVht = 1;
        else
            prWifiVar->ucStaVht = 0;
        if ((mode & WIFI_MODE_80211AX) == 0x4)
            prWifiVar->ucStaHe = 1;
        else
            prWifiVar->ucStaHe = 0;
    } else {
        if ((mode & WIFI_MODE_80211N) == 0x1)
            prWifiVar->ucApHt = 1;
        else
            prWifiVar->ucApHt = 0;
        if ((mode & WIFI_MODE_80211AC) == 0x2)
            prWifiVar->ucApVht = 1;
        else
            prWifiVar->ucApVht = 0;
    }
}

/*
Function to get the wireless mode.
*/
uint8_t wifi_get_wireless_mode(uint8_t port)
{
    struct GLUE_INFO *prGlueInfo;
    struct WIFI_VAR *prWifiVar;
    uint8_t mode = 0;

    prGlueInfo = &g_rGlueInfo;
    prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

    if (port == WIFI_PORT_STA) {
        if (prWifiVar->ucStaHt == 1)
            mode |= WIFI_MODE_80211N;
        if (prWifiVar->ucStaVht == 1)
            mode |= WIFI_MODE_80211AC;
        if (prWifiVar->ucStaHe == 1)
            mode |= WIFI_MODE_80211AX;
    } else {
        if (prWifiVar->ucApHt == 1)
            mode |= WIFI_MODE_80211N;
        if (prWifiVar->ucApVht == 1)
            mode |= WIFI_MODE_80211AC;
    }

    mode = wifi_wireless_mode_mapping(0, mode);
    return mode;
}

/**
* @brief Set WiFi Wireless Mode
*
* @param [IN]mode
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_wireless_mode(uint8_t port, wifi_phy_mode_t mode)
{
    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (!wifi_is_phy_mode_valid(mode)) {
        LOG_FUNC("mode is invalid: %d", mode);
        return WIFI_ERR_PARA_INVALID;
    }

    uint8_t link_status = 0;
    wifi_connection_get_link_status(&link_status);

    if (port == WIFI_PORT_STA && link_status == 1) {
        wifi_connection_disconnect_ap();
        wifi_connection_stop_scan();

        mdelay(100);
        wifi_set_wireless_mode(port, mode);

        wifi_connection_start_scan(NULL, 0, NULL, 0, 0);
    } else {
        wifi_set_wireless_mode(port, mode);
    }
#ifdef MTK_MINISUPP_ENABLE
    if (port == WIFI_PORT_AP)
        wifi_event_notification_wait(port,
                                     WIFI_EVENT_ID_IOT_RELOAD_CONFIGURATION, NULL, 0);
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return WIFI_SUCC;
}

/**
* @brief Get WiFi Wireless Mode
*
* @return 0 11ABGN default
* @return 1 11N
* @return 2 11AC
* @return 3 11N/AC mixed
* @return 4 11AX
* @return 5 11N/AX mixed
* @return 6 11AC/AX mixed
* @return 7 11N/AC/AX mixed
*
*/
int32_t wifi_config_get_wireless_mode(uint8_t port, wifi_phy_mode_t *mode)
{
    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (mode == NULL) {
        LOG_FUNC("mode is null");
        return WIFI_ERR_PARA_INVALID;
    }

    *mode = wifi_get_wireless_mode(port);

    return WIFI_SUCC;
}

/**
* @brief Set Country code
*
* @param [IN]wifi_country_code
*
* @return  >=0 means success, <0 means fail
*
* @note The prototype of this API may will be changed in future.
*       Please be carefull with this API for the future migration.
*/
int32_t wifi_config_set_country_code(wifi_country_code_t *wifi_country_code)
{
    struct GLUE_INFO *prGlueInfo;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    uint8_t aucCountry[2];

    prGlueInfo = &g_rGlueInfo;

    if (regd_is_single_sku_en()) {
        uint8_t aucCountry_code[4] = {0, 0, 0, 0};
        uint8_t /*i,*/ count;

        /* command like "COUNTRY US", "COUNTRY US1" and
         * "COUNTRY US01"
        */
        count = kalStrLen((char *)wifi_country_code->country_code);
        kalMemCopy(aucCountry_code,
                   (char *)wifi_country_code->country_code, count);

        rStatus = kalIoctl(prGlueInfo, wlanoidSetCountryCode,
                           &aucCountry_code[0], count,
                           FALSE, FALSE, TRUE, &u4BufLen);
        if (rStatus != WIFI_SUCC)
            return WIFI_FAIL;

        return WIFI_SUCC;
    }

    /* command like "COUNTRY US", "COUNTRY EU" and "COUNTRY JP" */
    kalMemCopy(aucCountry, wifi_country_code, 2);

    rStatus = kalIoctl(prGlueInfo, wlanoidSetCountryCode,
                       &aucCountry[0], 2, FALSE, FALSE, TRUE,
                       &u4BufLen);
    if (rStatus != WIFI_SUCC)
        return WIFI_FAIL;

    return WIFI_SUCC;

}

/**
* @brief Get Country code
*
* @param [OUT]wifi_country_code
*
* @return  >=0 means success, <0 means fail
*
* @note The prototype of this API may will be changed in future.
        Please be carefull with this API for the future migration.
*/
int32_t wifi_config_get_country_code(wifi_country_code_t *wifi_country_code)
{
    uint32_t country = 0;
    char acCountryStr[MAX_COUNTRY_CODE_LEN + 1] = {0};
    uint8_t count;

    if (!regd_is_single_sku_en()) {
        LOG_FUNC("Not Supported.");
        return WIFI_FAIL;
    }

    country = rlmDomainGetCountryCode();
    rlmDomainU32ToAlpha(country, acCountryStr);

    count = kalStrLen(acCountryStr);
    kalMemCopy(wifi_country_code->country_code, acCountryStr, count);

    return  WIFI_SUCC;
}

#if CFG_SUPPORT_REG_RULES
/**
* @brief Check the allowed channel base on the specific country code.
*
* @param [IN] channel, contry code
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_reg_rules_allow_channel(uint8_t channel, uint8_t *country_code)
{
    char acCountryCodeStr[MAX_COUNTRY_CODE_LEN] = {0};
    const struct mtk_regdomain *prRegdomain = NULL;
    const struct ieee80211_regdomain *prRegdRules = NULL;
    uint8_t ucCode_idx = 0;
    uint8_t i;
    uint16_t center_freq;

    if (!wifi_is_channel_valid(channel)) {
        LOG_FUNC("Channel %d in not allowed!", channel);
        return WIFI_ERR_PARA_INVALID;
    }
    if (!regd_is_single_sku_en()) {
        LOG_FUNC("Not Supported.");
        return WIFI_FAIL;
    }
    if (channel >= 1 && channel <= 13)
        center_freq = 2412 + (channel - 1) * 5;
    else if (channel == 14)
        center_freq = 2484;
    else if (channel >= 36 && channel <= 165)
        center_freq = 5000 + 5 * channel;
    else
        return WIFI_FAIL;

    kalMemCopy(acCountryCodeStr, country_code, MAX_COUNTRY_CODE_LEN);
    while (g_prRegRuleTable[ucCode_idx]) {
        prRegdomain = g_prRegRuleTable[ucCode_idx];
        if (kalStrnCmp(acCountryCodeStr, prRegdomain->country_code, MAX_COUNTRY_CODE_LEN) == 0) {
            prRegdRules = prRegdomain->prRegdRules;
            break;
        }
        ucCode_idx++;
    }
    if (g_prRegRuleTable[ucCode_idx] == NULL) {
        LOG_FUNC("Country Code was not Found.");
        return WIFI_FAIL;
    }
    for (i = 0; i < prRegdRules->n_reg_rules; i++) {
        if ((center_freq >=
             KHZ_TO_MHZ(prRegdRules->reg_rules[i].freq_range.start_freq_khz) + 10)
            && (center_freq <=
                KHZ_TO_MHZ(prRegdRules->reg_rules[i].freq_range.end_freq_khz) - 10)) {
            LOG_FUNC("Channel %d is supported in %s!.", channel, country_code);
            return WIFI_SUCC;
        }
    }
    LOG_FUNC("Channel was not Found!.");
    return WIFI_FAIL;
}
#endif /* #if CFG_SUPPORT_REG_RULES */

/**
* @brief Set WiFi beacon Interval
*
* @param [IN]interval 1 ~ 255
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_bcn_interval(uint8_t interval)
{
    int32_t status = 0;

    if (interval == 0)
        return WIFI_FAIL;
#ifdef MTK_MINISUPP_ENABLE
    uint8_t opmode;

    if (wifi_config_get_opmode(&opmode) < 0)
        return WIFI_FAIL;

    if (opmode == WIFI_MODE_AP_ONLY || opmode == WIFI_MODE_REPEATER)
        status = mtk_supplicant_set_bcn_interval(interval);
    else
        LOG_FUNC("Not support.");

#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return status;
}

/**
* @brief Set WiFi Dtim Interval
*
* @param [IN]interval 1 ~ 255
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_dtim_interval(uint8_t interval)
{
    int32_t status = 0;

    if (interval == 0)
        return WIFI_FAIL;
#ifdef MTK_MINISUPP_ENABLE
    uint8_t opmode;

    if (wifi_config_get_opmode(&opmode) < 0)
        return WIFI_FAIL;

    if (opmode == WIFI_MODE_AP_ONLY || opmode == WIFI_MODE_REPEATER)
        status =  mtk_supplicant_set_dtim_interval(interval);
    else
        LOG_FUNC("Not support.");

#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return status;
}

/**
* @brief Get WiFi DTIM Interval
*
* @return interval: 1~255
*/
int32_t wifi_config_get_dtim_interval(uint8_t *interval)
{
    int32_t status = WIFI_FAIL;
#ifdef MTK_MINISUPP_ENABLE
    struct ADAPTER *prAdapter;
    struct BSS_DESC *prBssDesc;
    uint8_t aucBSSID[MAC_ADDR_LEN];
    uint8_t opmode;

    if (wifi_config_get_opmode(&opmode) < 0)
        return WIFI_FAIL;

    if (opmode == WIFI_MODE_AP_ONLY || opmode == WIFI_MODE_REPEATER) {
        status = mtk_supplicant_get_dtim_interval(interval);
        return status;
    } else {
        status = wifi_config_get_bssid((uint8_t *)aucBSSID);
        prAdapter = &g_rAdapter;

        if (status >= 0) {
            prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
                                                        aucBSSID, FALSE, NULL);
            if (prBssDesc == NULL)
                return WIFI_FAIL;
            *interval = prBssDesc->ucDTIMPeriod;
        }
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return status;
}

/**
* @brief Set WiFi Listen Interval
*
* @param [IN]interval 1 ~ 255
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_listen_interval(uint8_t interval)
{
    int32_t status = 0;
    struct GLUE_INFO *prGlueInfo;
    struct ADAPTER *prAdapter = NULL;
    uint8_t ucDTIMPeriod;

    prGlueInfo = &g_rGlueInfo;
    prAdapter = prGlueInfo->prAdapter;

    if (interval == 0)
        return WIFI_ERR_PARA_INVALID;

#ifdef MTK_MINISUPP_ENABLE
    mtk_supplicant_update_listen_interval(interval);
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    status = wifi_config_get_dtim_interval(&ucDTIMPeriod);
    if (status != WIFI_SUCC)
        return WIFI_FAIL;

    if (interval < ucDTIMPeriod)
        interval = ucDTIMPeriod;

    prAdapter->rWifiVar.ucWowOnMdtim = interval / ucDTIMPeriod;

#if CFG_SUPPORT_ROAMING_CUSTOMIZED
    wifi_config_get_roam_by_bcnmissthreshold(WIFI_PORT_STA, &g_roam_by_bcnmiss_threshold);
    wifi_config_set_roam_by_bcnmissthreshold(WIFI_PORT_STA, g_roam_by_bcnmiss_threshold);
#endif /* #if CFG_SUPPORT_ROAMING_CUSTOMIZED */

    return WIFI_SUCC;

}


/**
* @brief Get WiFi Listen Interval
*
* @return interval: 1~255
*/
int32_t wifi_config_get_listen_interval(uint8_t *interval)
{
    int32_t status = 0;
    struct GLUE_INFO *prGlueInfo;
    struct ADAPTER *prAdapter = NULL;
    uint8_t ucDTIMPeriod;

    prGlueInfo = &g_rGlueInfo;
    prAdapter = prGlueInfo->prAdapter;

    status = wifi_config_get_dtim_interval(&ucDTIMPeriod);
    if (status != WIFI_SUCC)
        return WIFI_FAIL;

    *interval = ucDTIMPeriod * prAdapter->rWifiVar.ucWowOnMdtim;

    return WIFI_SUCC;
}

/**
 * @brief Get the current STA port link up / link down
 * status of the connection
 *
 * @param [OUT]link_status
 * @param 0 WIFI_STATUS_LINK_DISCONNECTED
 * @param 1 WIFI_STATUS_LINK_CONNECTED
 *
 * @return  >=0 means success, <0 means fail
 *
 * @note WIFI_STATUS_LINK_DISCONNECTED indicates STA may be in
 *       IDLE/ SCAN/ CONNECTING state.
 */
int32_t wifi_connection_get_link_status(uint8_t *link_status)
{
    int32_t status = 0;

    status = wifi_connection_get_link_status_internal(link_status);

    return status;
}

/**
* @brief WiFi Radio ON/OFF
*
* @param [IN]on_off
* @param 0 OFF
* @param 1 ON
*
* @return  >=0 means success, <0 means fail
*
* @note For MT7687/7697, this API is only supported in the station mode,
*       AP mode or monitor mode.
* For MT7682/7686/5932, this API is only supported in the station mode.
*
*/
int32_t wifi_config_set_radio(uint8_t on_off)
{
    int32_t status = 0;
    uint8_t opmode = 0;
    uint8_t u1IsApExisted = 0, u1IsStaExisted = 0;
#ifdef MTK_MINISUPP_ENABLE
    uint8_t radio_status = 0;
    uint8_t driver_msg = on_off;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    if (wifi_config_get_opmode_internal(&opmode) < 0) {
        status =  WIFI_FAIL;
        goto final;
    }

    if ((on_off != 0) && (on_off != 1)) {
        status =  WIFI_ERR_PARA_INVALID;
        goto final;
    }

    u1IsApExisted = (opmode == WIFI_MODE_AP_ONLY) ||
                    (opmode == WIFI_MODE_REPEATER);
    u1IsStaExisted = (opmode == WIFI_MODE_STA_ONLY) ||
                     (opmode == WIFI_MODE_REPEATER);

#ifdef MTK_MINISUPP_ENABLE
    wifi_config_get_radio_interal(&radio_status);
    if ((radio_status == 0 && on_off == 0) ||
        (radio_status == 1 && on_off == 1)) {
        LOG_FUNC("radio already on/off, it is not necessary do again.");
        status = WIFI_SUCC;
        goto final;
    }

    if (u1IsStaExisted) {
        if (on_off == 0)
            wifi_event_notification_wait(WIFI_PORT_STA,
                                         WIFI_EVENT_ID_IOT_DISCONNECT_AP_WAIT, NULL, 0);

        if (wifi_event_notification_wait(WIFI_PORT_STA,
                                         WIFI_EVENT_ID_IOT_STA_RADIO_ONOFF,
                                         &driver_msg, 1) == WIFI_SUCC) {
        } else {
            status = WIFI_FAIL;
            goto final;
        }
        if (on_off == 0) {
            g_scanning = 0;
            g_scan_by_app = 0;
        }
        status = WIFI_SUCC;
    }
#ifdef MTK_WIFI_AP_ENABLE
    if (u1IsApExisted) {
        mtk_supplicant_ap_onoff(on_off);
        status = WIFI_SUCC;
    }
#endif /* #ifdef MTK_WIFI_AP_ENABLE */
    if (u1IsStaExisted == 0 && u1IsApExisted == 0) {
        LOG_FUNC("Invalid mode. The running opmode is %d",
                 opmode);
        status = WIFI_ERR_NOT_SUPPORT;
        goto final;
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return status;
}

/**
* @brief This functions set power save mode,this only support in mt7682/mt7686
*        /mt7933 currently.
*
* @param[in] power_save_mode indicates three power save mode below.
*
* Value | Definition                                                        |
* ------|-------------------------------------------------------------------|
* \b 0  | CAM: CAM (Constantly Awake Mode) is a power save mode that keeps
*              the radio powered up continuously to ensure there is a minimal
*              lag in response time. This power save setting consumes the most
*              power but offers the highest throughput.|
* \b 1  | LEGACY_POWERSAVE: the access point buffers incoming messages for the
*              radio. The radio occasionally 'wakes up' to determine if any
*              buffered messages are waiting and then returns to sleep mode
*              after it requests each message. This setting conserves the most
*              power but also provides the lowest throughput. It is
*              recommended for radios in which power consumption is the most
*              important (such as small battery-operating devices).|
* \b 2  | FAST_POWERSAVE: Fast is a power save mode that switches between
*              power saving and CAM modes, depending on the network traffic.
*              For example, it switches to CAM when receiving a large number
*              of packets and switches back to PS mode after the packets have
*              been retrieved. Fast is recommended when power consumption and
*              throughput are a concern.|
*
* @return  >=0 the operation completed successfully, <0 the operation failed.
*
* @note  Only supported in STA mode.
*/
int32_t wifi_config_set_power_save_mode(uint8_t ps_mode)
{
    struct GLUE_INFO *prGlueInfo;
    enum PARAM_POWER_MODE ePowerMode;
    struct PARAM_POWER_MODE_ rPowerMode;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;

    if (ps_mode > 2) {
        LOG_FUNC("mode: 0:CAM, 1:LEGACY, 2:FAST");
        return WIFI_FAIL;
    }

    prGlueInfo = &g_rGlueInfo;
    rPowerMode.ucBssIdx =
        prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    rPowerMode.ePowerMode = Param_PowerModeCAM;

    ePowerMode = ps_mode;
    if (ePowerMode < Param_PowerModeMax)
        rPowerMode.ePowerMode = ePowerMode;

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSet802dot11PowerSaveProfile,
                       &rPowerMode, sizeof(struct PARAM_POWER_MODE_),
                       FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS)
        return WIFI_FAIL;

#ifdef MTK_MINISUPP_ENABLE
    mtk_supplicant_update_ps_mode(ps_mode);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    return WIFI_SUCC;
}

/**
* @brief This functions get power save mode, this only support in mt7682/mt7686
*        /mt7933 currently.
*
* @param[in] power_save_mode indicates three power save mode below.
*
* Value | Definition                                                        |
* ------|-------------------------------------------------------------------|
* \b 0  | CAM: CAM (Constantly Awake Mode) is a power save mode that keeps
*              the radio powered up continuously to ensure there is a minimal
*              lag in response time. This power save setting consumes the most
*              power but offers the highest throughput.|
* \b 1  | LEGACY_POWERSAVE: the access point buffers incoming messages for the
*              radio. The radio occasionally 'wakes up' to determine if any
*              buffered messages are waiting and then returns to sleep mode
*              after it requests each message. This setting conserves the most
*              power but also provides the lowest throughput. It is
*              recommended for radios in which power consumption is the most
*              important (such as small battery-operating devices).|
* \b 2  | FAST_POWERSAVE: Fast is a power save mode that switches between
*              power saving and CAM modes, depending on the network traffic.
*              For example, it switches to CAM when receiving a large number
*              of packets and switches back to PS mode after the packets have
*              been retrieved. Fast is recommended when power consumption and
*              throughput are a concern.|
*
* @return  >=0 the operation completed successfully, <0 the operation failed.
*
* @note  Only supported in STA mode.
*/
int32_t wifi_config_get_power_save_mode(uint8_t *ps_mode)
{
    struct GLUE_INFO *prGlueInfo;
    enum PARAM_POWER_MODE ePowerMode;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;

    prGlueInfo = &g_rGlueInfo;

    rStatus = wlanQueryInformation(prGlueInfo->prAdapter,
                                   wlanoidQuery802dot11PowerSaveProfile,
                                   &ePowerMode, sizeof(ePowerMode),
                                   &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS)
        return WIFI_FAIL;

    *ps_mode = ePowerMode;

    return WIFI_SUCC;

}

/**
 * @brief Set Bss Preference when scanning
 *
 * @return >=0 means success, <0 means fail
 *
 * @params [Mode]off=0, prefer_2G=1, prefer_5G=2
 */
int32_t wifi_config_set_bss_preference(uint8_t mode)
{
    int32_t status = WIFI_SUCC;

    if (mode > 2) {
        LOG_FUNC("[Mode]off=0, prefer_2G=1, prefer_5G=2");
        status =  WIFI_ERR_PARA_INVALID;
        return WIFI_FAIL;
    }

#ifdef MTK_MINISUPP_ENABLE
    uint8_t driver_msg[1] = {0};

    driver_msg[0] = mode;

    if (wifi_event_notification_wait(WIFI_PORT_STA,
                                     WIFI_EVENT_ID_IOT_SET_BSS_PREFERENCE, (unsigned char *)&driver_msg,
                                     sizeof(driver_msg)) != WIFI_SUCC)
        status = WIFI_FAIL;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    return status;
}

/**
 * @brief Disconnect the current connection
 *
 * @return  >=0 means success, <0 means fail
 *
 * @note1  STA will back to scan state once disconnect from AP.
 * @note2  After connected to AP, if AP power off at once and do not disassoc
 *         the connected station.
 *         For MT7687/7697/MT7682/MT7686, it will wait 5 mins to enter
 *         disconnected state.
 */
int32_t wifi_connection_disconnect_ap(void)
{
    uint8_t opmode;
    int32_t status = WIFI_FAIL;

    if (wifi_config_get_opmode_internal(&opmode) < 0)
        LOG_FUNC("invalid opmode: %d", opmode);
#ifdef MTK_MINISUPP_ENABLE
    uint8_t bssid[6] = {0};
    uint8_t link;

    /*Pengfei Add: for disable auto connect feature*/
    if (opmode == WIFI_MODE_STA_ONLY || opmode == WIFI_MODE_REPEATER) {
        wifi_config_get_bssid((uint8_t *)bssid);
        status = wifi_event_notification_wait(WIFI_PORT_STA,
                                              WIFI_EVENT_ID_IOT_DISCONNECT_AP_WAIT, NULL, 0);

        g_scanning = 0;
        g_scan_by_app = 0;
        if (wifi_connection_get_link_status_internal(&link) == 0) {
            if (link == 0)
                wifi_api_event_trigger(WIFI_PORT_STA,
                                       WIFI_EVENT_IOT_DISCONNECTED, (uint8_t *)bssid,
                                       WIFI_MAC_ADDRESS_LENGTH);
        }
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return status;
}

/**
* @brief Trigger WiFi connection
*
* @param [IN]address STA MAC Address
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_connection_disconnect_sta(uint8_t *address)
{
    int ret = 0;

    LOG_FUNC("wifi_connection_disconnect_sta\n");

#if defined(MTK_MINISUPP_ENABLE) && (CFG_ENABLE_WIFI_DIRECT == 1)
    ret = mtk_p2p_freertos_wpa_del_station(NULL, address);
#endif /* #if defined(MTK_MINISUPP_ENABLE) && (CFG_ENABLE_WIFI_DIRECT == 1) */

    return ret;
}

/**
* @brief WiFi Radio ON/OFF
*
* @param [OUT]on_off
* @param 0 OFF
* @param 1 ON
*
* @return  >=0 means success, <0 means fail
*
* @note For MT7687/7697, this API is only supported in the station mode,
* AP mode or monitor mode. \n
* For MT7682/7686/5932, this API is only supported in the station mode.
*
*/
int32_t wifi_config_get_radio(uint8_t *on_off)
{
    int32_t status = 0;

    status = wifi_config_get_radio_interal(on_off);

    return status;
}

/**
* @brief Get wlan statistic info, include Temperature,
*        Tx_Success_Count/Retry_Count/Fail_Count,
*        Rx_Success_Count/Rx_with_CRC/Rx_Drop,
*        also more detail please refer to #wifi_statistic_t
*
* @param [IN]wifi_statistic
*
* @return  >=0 means success, <0 means fail
*
* @note only used in STA mode. If it connected to an AP router,
*       PHY rate also will be supported.
*/
int32_t wifi_config_get_statistic(wifi_statistic_t *wifi_statistic)
{
    int32_t status = WIFI_FAIL;
    struct GLUE_INFO *prGlueInfo;

    struct PARAM_HW_WLAN_INFO *prHwWlanInfo = NULL;
    struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics = NULL;
    struct PARAM_HW_MIB_INFO *prHwMibInfo = NULL;
    uint32_t u4BufLen = 0;
    uint8_t fgResetCnt = FALSE;
    uint8_t ucWlanIndex = 0;
    uint8_t *pucMacAddr = NULL;
    struct BSS_INFO *prAisBssInfo = NULL;
    struct BSS_INFO *prBssInfo = NULL;
    struct PARAM_GET_CNM_T *prCnmInfo = NULL;
    int32_t rRssi;
    uint32_t rStatus = 0;
    uint16_t u2Idx = 0;
    uint8_t ucDbdcIdx, ucSkipAr;
    static uint32_t u4TotalTxCnt[CFG_STAT_DBG_PEER_NUM] = {0};
    static uint32_t u4TotalFailCnt[CFG_STAT_DBG_PEER_NUM] = {0};
    static uint32_t u4Rate1TxCnt[CFG_STAT_DBG_PEER_NUM] = {0};
    static uint32_t u4Rate1FailCnt[CFG_STAT_DBG_PEER_NUM] = {0};
    static uint32_t au4RxMpduCnt[ENUM_BAND_NUM] = {0};
    static uint32_t au4FcsError[ENUM_BAND_NUM] = {0};
    static uint32_t au4RxFifoCnt[ENUM_BAND_NUM] = {0};
    static uint32_t au4AmpduTxSfCnt[ENUM_BAND_NUM] = {0};
    static uint32_t au4AmpduTxAckSfCnt[ENUM_BAND_NUM] = {0};
    struct RX_CTRL *prRxCtrl;
    int32_t snr = 0;
    uint8_t ucBssIndex = AIS_DEFAULT_INDEX;
    enum ENUM_CNM_NETWORK_TYPE_T eNetworkType;
    uint32_t u4TxRetry = 0;
    uint32_t u4RxVector = 0;
    uint8_t ucStaIdx;
    u_int8_t fgWlanIdxFound = TRUE, fgSkipRxV = FALSE;
    struct STA_RECORD *prStaRecOfAP;
    uint16_t ucTxphy = 0, ucRxphy = 0;
    uint8_t ucTxmode, ucTxrate, ucTxsgi;
    uint8_t ucTxdcm, ucTxersu106t, ucTxNsts, ucTxStbc;
    uint32_t u4RxVector0 = 0, u4RxVector1 = 0, u4RxVector2 = 0;
    uint8_t ucRxmode, ucRxrate, ucRxsgi, ucRxNsts, ucRxStbc;
    uint8_t ucGroupid, ucMu;
    int HW_TX_RATE_CCK_STR[] = {1, 2, 5.5, 11, 0};
    int HW_TX_RATE_OFDM_STR[] = {6, 9, 12, 18, 24, 36, 48, 54, 0};


    if (wifi_statistic == NULL)
        return WIFI_FAIL;

    prGlueInfo = &g_rGlueInfo;

    /**get prHwWlanInfo*/
    prAisBssInfo = aisGetAisBssInfo(
                       prGlueInfo->prAdapter, prGlueInfo->prDevHandler->bss_idx);

    if (prAisBssInfo->prStaRecOfAP)
        ucWlanIndex = prAisBssInfo->prStaRecOfAP
                      ->ucWlanIndex;
    else if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter,
                                      NULL, &ucWlanIndex))
        goto out;

    prHwWlanInfo = (struct PARAM_HW_WLAN_INFO *)kalMemAlloc(
                       sizeof(struct PARAM_HW_WLAN_INFO), VIR_MEM_TYPE);
    if (!prHwWlanInfo)
        goto out;

    prHwWlanInfo->u4Index = ucWlanIndex;
    prHwWlanInfo->rWtblRxCounter.fgRxCCSel = FALSE;

    status = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
                      sizeof(struct PARAM_HW_WLAN_INFO),
                      TRUE, TRUE, TRUE, &u4BufLen);

    /**get prQueryStaStatistics*/
    prQueryStaStatistics =
        (struct PARAM_GET_STA_STATISTICS *)kalMemAlloc(
            sizeof(struct PARAM_GET_STA_STATISTICS), VIR_MEM_TYPE);
    if (!prQueryStaStatistics)
        goto out;

    prQueryStaStatistics->ucResetCounter = fgResetCnt;

    pucMacAddr = wlanGetStaAddrByWlanIdx(prGlueInfo->prAdapter,
                                         ucWlanIndex);

    if (!pucMacAddr) {
        LOG_FUNC("Addr of WlanIndex %d is not found!\n",
                 ucWlanIndex);
        goto out;
    }
    COPY_MAC_ADDR(prQueryStaStatistics->aucMacAddr, pucMacAddr);

    status = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
                      prQueryStaStatistics,
                      sizeof(struct PARAM_GET_STA_STATISTICS),
                      TRUE, TRUE, TRUE, &u4BufLen);

    /**get prHwMibInfo*/
    prHwMibInfo = (struct PARAM_HW_MIB_INFO *)kalMemAlloc(
                      sizeof(struct PARAM_HW_MIB_INFO), VIR_MEM_TYPE);
    if (!prHwMibInfo)
        goto out;

    prHwMibInfo->u4Index = 0;

    status = kalIoctl(prGlueInfo, wlanoidQueryMibInfo, prHwMibInfo,
                      sizeof(struct PARAM_HW_MIB_INFO),
                      TRUE, TRUE, TRUE, &u4BufLen);

    /**get rest*/
    ucSkipAr = prQueryStaStatistics->ucSkipAr;
    prRxCtrl = &prGlueInfo->prAdapter->rRxCtrl;

    if (ucSkipAr) {
        u2Idx = nicGetStatIdxInfo(prGlueInfo->prAdapter,
                                  (uint8_t)(prHwWlanInfo->u4Index));
        u4TotalTxCnt[u2Idx] += prQueryStaStatistics->u4TransmitCount;
        u4TotalFailCnt[u2Idx] +=
            prQueryStaStatistics->u4TransmitFailCount;
        u4Rate1TxCnt[u2Idx] += prQueryStaStatistics->u4Rate1TxCnt;
        u4Rate1FailCnt[u2Idx] += prQueryStaStatistics->u4Rate1FailCnt;
    }

    for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM; ucDbdcIdx++) {
        au4RxMpduCnt[ucDbdcIdx] += g_arMibInfo[ucDbdcIdx].u4RxMpduCnt;
        au4FcsError[ucDbdcIdx] += g_arMibInfo[ucDbdcIdx].u4FcsError;
        au4RxFifoCnt[ucDbdcIdx] += g_arMibInfo[ucDbdcIdx].u4RxFifoFull;
        au4AmpduTxSfCnt[ucDbdcIdx] +=
            g_arMibInfo[ucDbdcIdx].u4AmpduTxSfCnt;
        au4AmpduTxAckSfCnt[ucDbdcIdx] +=
            g_arMibInfo[ucDbdcIdx].u4AmpduTxAckSfCnt;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryRssi, &rRssi,
                       sizeof(rRssi), TRUE, TRUE, TRUE, &u4BufLen);
    if (rStatus != 0)
        LOG_FUNC("unable to retrieve rssi\n");

    prStaRecOfAP =
        aisGetStaRecOfAP(prGlueInfo->prAdapter, ucBssIndex);

    if (prStaRecOfAP)
        ucWlanIndex = prStaRecOfAP->ucWlanIndex;
    else if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter, NULL,
                                      &ucWlanIndex))
        fgWlanIdxFound = FALSE;
    if (fgWlanIdxFound) {
        if (wlanGetStaIdxByWlanIdx(prGlueInfo->prAdapter, ucWlanIndex,
                                   &ucStaIdx) == WLAN_STATUS_SUCCESS)
            u4RxVector =
                prGlueInfo->prAdapter->
                arStaRec[ucStaIdx].u4RxVector4;
        else
            fgSkipRxV = TRUE;
    } else
        fgSkipRxV = TRUE;

    if (!fgSkipRxV)
        snr = (uint32_t)(((u4RxVector & BITS(26, 31)) >> 26) - 16);

    struct TX_VECTOR_BBP_LATCH *prTxV = &prQueryStaStatistics->rTxVector[0];

    ucTxrate = TX_VECTOR_GET_TX_RATE(prTxV);
    ucTxmode = TX_VECTOR_GET_TX_MODE(prTxV);
    ucTxsgi = TX_VECTOR_GET_TX_SGI(prTxV);
    ucTxNsts = TX_VECTOR_GET_TX_NSTS(prTxV) + 1;
    ucTxStbc = TX_VECTOR_GET_TX_STBC(prTxV);
#if (CFG_SUPPORT_CONNAC2X == 1)
    ucTxdcm = TX_VECTOR_GET_TX_DCM(prTxV);
    ucTxersu106t = TX_VECTOR_GET_TX_106T(prTxV);

    if (ucTxdcm)
        ucTxrate = CONNAC2X_TXV_GET_TX_RATE_UNMASK_DCM(ucTxrate);
    if (ucTxersu106t)
        ucTxrate = CONNAC2X_TXV_GET_TX_RATE_UNMASK_106T(ucTxrate);
#endif /* #if (CFG_SUPPORT_CONNAC2X == 1) */
    if (ucTxmode == TX_RATE_MODE_CCK)
        ucTxphy = ucTxrate < 4 ? HW_TX_RATE_CCK_STR[ucTxrate] :
                  (ucTxrate < 8 ? HW_TX_RATE_CCK_STR[ucTxrate - 4] :
                   HW_TX_RATE_CCK_STR[4]);
    else if (ucTxmode == TX_RATE_MODE_OFDM) {
        switch (ucTxrate) {
            case 11: /* 6M */
                ucTxphy = HW_TX_RATE_OFDM_STR[0];
                break;
            case 15: /* 9M */
                ucTxphy = HW_TX_RATE_OFDM_STR[1];
                break;
            case 10: /* 12M */
                ucTxphy = HW_TX_RATE_OFDM_STR[2];
                break;
            case 14: /* 18M */
                ucTxphy = HW_TX_RATE_OFDM_STR[3];
                break;
            case 9: /* 24M */
                ucTxphy = HW_TX_RATE_OFDM_STR[4];
                break;
            case 13: /* 36M */
                ucTxphy = HW_TX_RATE_OFDM_STR[5];
                break;
            case 8: /* 48M */
                ucTxphy = HW_TX_RATE_OFDM_STR[6];
                break;
            case 12: /* 54M */
                ucTxphy = HW_TX_RATE_OFDM_STR[7];
                break;
            default:
                ucTxphy = HW_TX_RATE_OFDM_STR[8];
        }
    } else if ((ucTxmode == TX_RATE_MODE_HTMIX) ||
               (ucTxmode == TX_RATE_MODE_HTGF))
        ucTxphy = ucTxrate;
    else {
        ucTxphy = (ucTxphy | (ucTxStbc ? 1 : 0)) << 2;
        ucTxphy = (ucTxphy | ucTxNsts) << 7;
        ucTxphy = (ucTxphy | ucTxrate);
    }

    if (wlanGetStaIdxByWlanIdx(prGlueInfo->prAdapter, prHwWlanInfo->u4Index,
                               &ucStaIdx) == WIFI_SUCC) {
        if (CONNAC2X_RX_DBG_INFO_GRP3(prGlueInfo->prAdapter)) {
            /* Group3 PRXV0[0:31] */
            u4RxVector0 =
                prGlueInfo->prAdapter->
                arStaRec[ucStaIdx].u4RxVector0;

            /* P-RXV0 */
            ucRxrate = (u4RxVector0 & CONNAC2X_RX_VT_RX_RATE_MASK)
                       >> CONNAC2X_RX_VT_RX_RATE_OFFSET;
            ucRxsgi = (u4RxVector0 &
                       CONNAC2X_RX_VT_SHORT_GI_MASK_V2)
                      >> CONNAC2X_RX_VT_SHORT_GI_OFFSET_V2;
            ucRxmode = (u4RxVector0 &
                        CONNAC2X_RX_VT_RX_MODE_MASK_V2)
                       >> CONNAC2X_RX_VT_RX_MODE_OFFSET_V2;
            ucRxNsts = ((u4RxVector0 & CONNAC2X_RX_VT_NSTS_MASK)
                        >> CONNAC2X_RX_VT_NSTS_OFFSET);
            ucRxStbc = (u4RxVector0 & CONNAC2X_RX_VT_STBC_MASK_V2)
                       >> CONNAC2X_RX_VT_STBC_OFFSET_V2;
            ucMu = (u4RxVector0 & CONNAC2X_RX_VT_MU) >> 21;

            if (ucMu == 0)
                ucRxNsts += 1;
        } else {
            /* Group3 PRXV0[0:31] */
            u4RxVector0 =
                prGlueInfo->prAdapter->
                arStaRec[ucStaIdx].u4RxVector0;
            /* Group5 C-B-0[0:31] */
            u4RxVector1 =
                prGlueInfo->prAdapter->
                arStaRec[ucStaIdx].u4RxVector1;
            /* Group5 C-B-1[0:31] */
            u4RxVector2 =
                prGlueInfo->prAdapter->
                arStaRec[ucStaIdx].u4RxVector2;

            /* P-RXV0 */
            ucRxrate = (u4RxVector0 & CONNAC2X_RX_VT_RX_RATE_MASK)
                       >> CONNAC2X_RX_VT_RX_RATE_OFFSET;
            ucRxNsts = ((u4RxVector0 & CONNAC2X_RX_VT_NSTS_MASK)
                        >> CONNAC2X_RX_VT_NSTS_OFFSET);

            /* C-B-0 */
            ucRxmode = (u4RxVector1 & CONNAC2X_RX_VT_RX_MODE_MASK)
                       >> CONNAC2X_RX_VT_RX_MODE_OFFSET;
            ucRxsgi = (u4RxVector1 & CONNAC2X_RX_VT_SHORT_GI_MASK)
                      >> CONNAC2X_RX_VT_SHORT_GI_OFFSET;
            ucRxStbc = (u4RxVector1 & CONNAC2X_RX_VT_STBC_MASK)
                       >> CONNAC2X_RX_VT_STBC_OFFSET;

            /* C-B-1 */
            ucGroupid = (u4RxVector2 & CONNAC2X_RX_VT_GROUP_ID_MASK)
                        >> CONNAC2X_RX_VT_GROUP_ID_OFFSET;

            if (ucGroupid && ucGroupid != 63) {
                ucMu = 1;
            } else {
                ucMu = 0;
                ucRxNsts += 1;
            }
        }
    } else
        goto out;

    if (ucRxmode == TX_RATE_MODE_CCK)
        ucRxphy = ucRxrate < 4 ? HW_TX_RATE_CCK_STR[ucRxrate] :
                  (ucRxrate < 8 ? HW_TX_RATE_CCK_STR[ucRxrate - 4] :
                   HW_TX_RATE_CCK_STR[4]);
    else if (ucRxmode == TX_RATE_MODE_OFDM) {
        switch (ucRxrate) {
            case 11: /* 6M */
                ucRxphy = HW_TX_RATE_OFDM_STR[0];
                break;
            case 15: /* 9M */
                ucRxphy = HW_TX_RATE_OFDM_STR[1];
                break;
            case 10: /* 12M */
                ucRxphy = HW_TX_RATE_OFDM_STR[2];
                break;
            case 14: /* 18M */
                ucRxphy = HW_TX_RATE_OFDM_STR[3];
                break;
            case 9: /* 24M */
                ucRxphy = HW_TX_RATE_OFDM_STR[4];
                break;
            case 13: /* 36M */
                ucRxphy = HW_TX_RATE_OFDM_STR[5];
                break;
            case 8: /* 48M */
                ucRxphy = HW_TX_RATE_OFDM_STR[6];
                break;
            case 12: /* 54M */
                ucRxphy = HW_TX_RATE_OFDM_STR[7];
                break;
            default:
                ucRxphy = HW_TX_RATE_OFDM_STR[8];
        }
    } else if ((ucRxmode == TX_RATE_MODE_HTMIX) ||
               (ucRxmode == TX_RATE_MODE_HTGF))
        ucRxphy = ucRxrate;
    else {
        ucRxphy = (ucRxphy | (ucRxStbc ? 1 : 0)) << 3;
        ucRxphy = (ucRxphy | ucRxNsts) << 7;
        ucRxphy = (ucRxphy | ucRxrate);
    }

    prCnmInfo = (struct PARAM_GET_CNM_T *)kalMemAlloc(
                    sizeof(struct PARAM_GET_CNM_T), VIR_MEM_TYPE);
    if (prCnmInfo == NULL)
        goto out;

    kalMemZero(prCnmInfo, sizeof(struct PARAM_GET_CNM_T));

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryCnm, prCnmInfo,
                       sizeof(struct PARAM_GET_CNM_T),
                       TRUE, TRUE, TRUE, &u4BufLen);

    for (ucBssIndex = BSSID_0; ucBssIndex < BSSID_NUM; ucBssIndex++) {

        prBssInfo = prGlueInfo->prAdapter->aprBssInfo[ucBssIndex];
        if (!prBssInfo)
            continue;

        eNetworkType = cnmGetBssNetworkType(prBssInfo);
        if (prCnmInfo->ucBssInuse[ucBssIndex] &&
            prCnmInfo->ucBssActive[ucBssIndex] &&
            eNetworkType == ENUM_CNM_NETWORK_TYPE_AIS &&
            prCnmInfo->ucBssConnectState[ucBssIndex] == MEDIA_STATE_CONNECTED) {
            u4TxRetry = prHwMibInfo->rHwMibCnt.au4FrameRetryCnt[ucBssIndex];
        }
    }


    /**assign variable*/
    wifi_statistic->Current_Temperature =
        prQueryStaStatistics->ucTemperature;
    wifi_statistic->Tx_Success_Count = ucSkipAr ? u4TotalTxCnt[u2Idx] -
                                       u4TotalFailCnt[u2Idx] : prQueryStaStatistics->u4TransmitCount -
                                       prQueryStaStatistics->u4TransmitFailCount;
    wifi_statistic->Tx_Retry_Count = u4TxRetry;
    wifi_statistic->Tx_Fail_Count = ucSkipAr ? u4TotalFailCnt[u2Idx] :
                                    prQueryStaStatistics->u4TransmitFailCount;
    wifi_statistic->Rx_Success_Count = au4RxMpduCnt[0];
    wifi_statistic->Rx_with_CRC = prHwMibInfo->rHwMibCnt.u4RxFcsErrCnt;
    wifi_statistic->Rx_Drop_due_to_out_of_resources =
        prHwMibInfo->rHwMibCnt.u4RxFifoFullCnt;
    wifi_statistic->Rssi = rRssi;
    wifi_statistic->MIC_Error_Count =
        RX_GET_CNT(prRxCtrl, RX_MIC_ERROR_DROP_COUNT);
    wifi_statistic->AMPDU_Tx_Success =
        prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt;
    wifi_statistic->AMPDU_Tx_Fail = prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt -
                                    prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt;
    wifi_statistic->SNR = snr;
    wifi_statistic->BBPCurrentBW =
        prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability;
    wifi_statistic->REAL_TX_PHY_Rate = ucTxphy;
    wifi_statistic->REAL_TX_PHY_Mode = ucTxmode;
    wifi_statistic->REAL_TX_ShortGI = ucTxsgi;
    wifi_statistic->REAL_TX_MCS = ucTxrate;
    wifi_statistic->REAL_RX_PHY_Rate = ucRxphy;
    wifi_statistic->REAL_RX_PHY_Mode = ucRxmode;
    wifi_statistic->REAL_RX_ShortGI = ucRxsgi;
    wifi_statistic->REAL_RX_MCS = ucRxrate;
    wifi_statistic->Tx_AGG_Range_1 =
        prHwMibInfo->rHwTxAmpduMts.u2TxRange1AmpduCnt;
    wifi_statistic->Tx_AGG_Range_2 =
        prHwMibInfo->rHwTxAmpduMts.u2TxRange2AmpduCnt;
    wifi_statistic->Tx_AGG_Range_3 =
        prHwMibInfo->rHwTxAmpduMts.u2TxRange3AmpduCnt;
    wifi_statistic->Tx_AGG_Range_4 =
        prHwMibInfo->rHwTxAmpduMts.u2TxRange4AmpduCnt;

out:
    if (prHwWlanInfo)
        kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_HW_WLAN_INFO));
    if (prQueryStaStatistics)
        kalMemFree(prQueryStaStatistics, VIR_MEM_TYPE,
                   sizeof(struct PARAM_GET_STA_STATISTICS));
    if (prHwMibInfo)
        kalMemFree(prHwMibInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_HW_MIB_INFO));
    if (prCnmInfo)
        kalMemFree(prCnmInfo, VIR_MEM_TYPE,
                sizeof(struct PARAM_GET_CNM_T));
    return status;
}

/**
* @brief Get wlan statistic info, include Temperature,
*        Tx_Success_Count/Retry_Count/Fail_Count,
*        Rx_Success_Count/Rx_with_CRC/Rx_Drop,
*        also more detail please refer to #wifi_statistic_t
*
* @param [IN]wifi_statistic
*
* @param [IN]pucMacAddr The MAC address of STA to query
*
* @return  >=0 means success, <0 means fail
*
* @note only used in softAP mode.
*/
int32_t wifi_config_get_statistic_per_sta(wifi_statistic_t *wifi_statistic,
                                          uint8_t *pucMacAddr)
{
    int32_t status = WIFI_SUCC;
    struct GLUE_INFO *prGlueInfo;
    struct PARAM_HW_WLAN_INFO *prHwWlanInfo = NULL;
    struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics = NULL;
    struct PARAM_HW_MIB_INFO *prHwMibInfo = NULL;
    uint32_t u4BufLen = 0;
    uint8_t fgResetCnt = FALSE;
    uint8_t ucWlanIndex = 0;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint16_t u2Idx = 0;
    uint8_t ucDbdcIdx, ucSkipAr;
    static uint32_t u4TotalTxCnt[CFG_STAT_DBG_PEER_NUM] = {0};
    static uint32_t u4TotalFailCnt[CFG_STAT_DBG_PEER_NUM] = {0};
    static uint32_t u4Rate1TxCnt[CFG_STAT_DBG_PEER_NUM] = {0};
    static uint32_t u4Rate1FailCnt[CFG_STAT_DBG_PEER_NUM] = {0};
    static uint32_t au4RxMpduCnt[ENUM_BAND_NUM] = {0};
    static uint32_t au4FcsError[ENUM_BAND_NUM] = {0};
    static uint32_t au4RxFifoCnt[ENUM_BAND_NUM] = {0};
    static uint32_t au4AmpduTxSfCnt[ENUM_BAND_NUM] = {0};
    static uint32_t au4AmpduTxAckSfCnt[ENUM_BAND_NUM] = {0};
    struct RX_CTRL *prRxCtrl;
    int32_t snr = 0;
    uint32_t u4RxVector = 0;
    uint8_t ucStaIdx;
    u_int8_t fgSkipRxV = FALSE;
    uint8_t ucTxmode, ucTxrate, ucTxsgi, ucTxphy = 0;
    uint8_t ucTxdcm, ucTxersu106t;
    uint32_t u4RxVector0 = 0, u4RxVector1 = 0;
    uint8_t ucRxmode, ucRxrate, ucRxsgi, ucRxphy = 0;
    uint8_t ucTxldpc, ucTxStbc;
    uint8_t ucRxldpc, ucRxStbc;
    int HW_TX_RATE_CCK_STR[] = {1, 2, 5.5, 11, 0};
    int HW_TX_RATE_OFDM_STR[] = {6, 9, 12, 18, 24, 36, 48, 54, 0};

    kalMemZero(wifi_statistic, sizeof(wifi_statistic_t));

    if ((wifi_statistic == NULL) || (pucMacAddr == NULL))
        goto out;

    prGlueInfo = &g_rGlueInfo;

    if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter,
                                 pucMacAddr, &ucWlanIndex))
        goto out;
    LOG_FUNC("STA " MACSTR ", ucWlanIndex: %u\n",
             MAC2STR(pucMacAddr), ucWlanIndex);

    prHwWlanInfo = (struct PARAM_HW_WLAN_INFO *)kalMemAlloc(
                       sizeof(struct PARAM_HW_WLAN_INFO), VIR_MEM_TYPE);
    if (!prHwWlanInfo)
        goto out;

    prHwWlanInfo->u4Index = ucWlanIndex;
    prHwWlanInfo->rWtblRxCounter.fgRxCCSel = FALSE;

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryWlanInfo, prHwWlanInfo,
                       sizeof(struct PARAM_HW_WLAN_INFO),
                       TRUE, TRUE, TRUE, &u4BufLen);
    if (rStatus != 0) {
        LOG_FUNC("Failed to query Wlan info (0x%08x)\n", rStatus);
        goto out;
    }

    /**get prQueryStaStatistics*/
    prQueryStaStatistics =
        (struct PARAM_GET_STA_STATISTICS *)kalMemAlloc(
            sizeof(struct PARAM_GET_STA_STATISTICS), VIR_MEM_TYPE);
    if (!prQueryStaStatistics)
        goto out;

    prQueryStaStatistics->ucResetCounter = fgResetCnt;
    COPY_MAC_ADDR(prQueryStaStatistics->aucMacAddr, pucMacAddr);

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
                       prQueryStaStatistics,
                       sizeof(struct PARAM_GET_STA_STATISTICS),
                       TRUE, TRUE, TRUE, &u4BufLen);
    if (rStatus != 0) {
        LOG_FUNC("Failed to query STA stat (0x%08x)\n", rStatus);
        goto out;
    }

    /**get prHwMibInfo*/
    prHwMibInfo = (struct PARAM_HW_MIB_INFO *)kalMemAlloc(
                      sizeof(struct PARAM_HW_MIB_INFO), VIR_MEM_TYPE);
    if (!prHwMibInfo)
        goto out;

    prHwMibInfo->u4Index = 0;

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryMibInfo, prHwMibInfo,
                       sizeof(struct PARAM_HW_MIB_INFO),
                       TRUE, TRUE, TRUE, &u4BufLen);
    if (rStatus != 0) {
        LOG_FUNC("Failed to query MIB info (0x%08x)\n", rStatus);
        goto out;
    }

    /**get rest*/
    ucSkipAr = prQueryStaStatistics->ucSkipAr;
    prRxCtrl = &prGlueInfo->prAdapter->rRxCtrl;

    if (ucSkipAr) {
        u2Idx = nicGetStatIdxInfo(prGlueInfo->prAdapter,
                                  (uint8_t)(prHwWlanInfo->u4Index));
        u4TotalTxCnt[u2Idx] += prQueryStaStatistics->u4TransmitCount;
        u4TotalFailCnt[u2Idx] +=
            prQueryStaStatistics->u4TransmitFailCount;
        u4Rate1TxCnt[u2Idx] += prQueryStaStatistics->u4Rate1TxCnt;
        u4Rate1FailCnt[u2Idx] += prQueryStaStatistics->u4Rate1FailCnt;
    }

    for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM; ucDbdcIdx++) {
        au4RxMpduCnt[ucDbdcIdx] += g_arMibInfo[ucDbdcIdx].u4RxMpduCnt;
        au4FcsError[ucDbdcIdx] += g_arMibInfo[ucDbdcIdx].u4FcsError;
        au4RxFifoCnt[ucDbdcIdx] += g_arMibInfo[ucDbdcIdx].u4RxFifoFull;
        au4AmpduTxSfCnt[ucDbdcIdx] +=
            g_arMibInfo[ucDbdcIdx].u4AmpduTxSfCnt;
        au4AmpduTxAckSfCnt[ucDbdcIdx] +=
            g_arMibInfo[ucDbdcIdx].u4AmpduTxAckSfCnt;
    }

    if (wlanGetStaIdxByWlanIdx(prGlueInfo->prAdapter, ucWlanIndex,
                               &ucStaIdx) == WLAN_STATUS_SUCCESS)
        u4RxVector =
            prGlueInfo->prAdapter->arStaRec[ucStaIdx].u4RxVector4;
    else
        fgSkipRxV = TRUE;

    if (!fgSkipRxV)
        snr = (uint32_t)(((u4RxVector & BITS(26, 31)) >> 26) - 16);

    struct TX_VECTOR_BBP_LATCH *prTxV = &prQueryStaStatistics->rTxVector[0];

    ucTxrate = TX_VECTOR_GET_TX_RATE(prTxV);
    ucTxmode = TX_VECTOR_GET_TX_MODE(prTxV);
    ucTxsgi = TX_VECTOR_GET_TX_SGI(prTxV);
    ucTxldpc = TX_VECTOR_GET_TX_LDPC(prTxV);
    ucTxStbc = TX_VECTOR_GET_TX_STBC(prTxV);
#if (CFG_SUPPORT_CONNAC2X == 1)
    ucTxdcm = TX_VECTOR_GET_TX_DCM(prTxV);
    ucTxersu106t = TX_VECTOR_GET_TX_106T(prTxV);

    if (ucTxdcm)
        ucTxrate = CONNAC2X_TXV_GET_TX_RATE_UNMASK_DCM(ucTxrate);
    if (ucTxersu106t)
        ucTxrate = CONNAC2X_TXV_GET_TX_RATE_UNMASK_106T(ucTxrate);
#endif /* #if (CFG_SUPPORT_CONNAC2X == 1) */
    if (ucTxmode == TX_RATE_MODE_CCK)
        ucTxphy = ucTxrate < 4 ? HW_TX_RATE_CCK_STR[ucTxrate] :
                  (ucTxrate < 8 ? HW_TX_RATE_CCK_STR[ucTxrate - 4] :
                   HW_TX_RATE_CCK_STR[4]);
    else if (ucTxmode == TX_RATE_MODE_OFDM) {
        switch (ucTxrate) {
            case 11: /* 6M */
                ucTxphy = HW_TX_RATE_OFDM_STR[0];
                break;
            case 15: /* 9M */
                ucTxphy = HW_TX_RATE_OFDM_STR[1];
                break;
            case 10: /* 12M */
                ucTxphy = HW_TX_RATE_OFDM_STR[2];
                break;
            case 14: /* 18M */
                ucTxphy = HW_TX_RATE_OFDM_STR[3];
                break;
            case 9: /* 24M */
                ucTxphy = HW_TX_RATE_OFDM_STR[4];
                break;
            case 13: /* 36M */
                ucTxphy = HW_TX_RATE_OFDM_STR[5];
                break;
            case 8: /* 48M */
                ucTxphy = HW_TX_RATE_OFDM_STR[6];
                break;
            case 12: /* 54M */
                ucTxphy = HW_TX_RATE_OFDM_STR[7];
                break;
            default:
                ucTxphy = HW_TX_RATE_OFDM_STR[8];
        }
    }

    if (wlanGetStaIdxByWlanIdx(prGlueInfo->prAdapter, prHwWlanInfo->u4Index,
                               &ucStaIdx) == WIFI_SUCC) {
        if (CONNAC2X_RX_DBG_INFO_GRP3(prGlueInfo->prAdapter)) {
            /* Group3 PRXV0[0:31] */
            u4RxVector0 =
                prGlueInfo->prAdapter->
                arStaRec[ucStaIdx].u4RxVector0;

            /* P-RXV0 */
            ucRxrate = (u4RxVector0 & CONNAC2X_RX_VT_RX_RATE_MASK)
                       >> CONNAC2X_RX_VT_RX_RATE_OFFSET;
            ucRxldpc = (u4RxVector0 &
                        CONNAC2X_RX_VT_LDPC)
                       >> CONNAC2X_RX_VT_LDPC_OFFSET;
            ucRxsgi = (u4RxVector0 &
                       CONNAC2X_RX_VT_SHORT_GI_MASK_V2)
                      >> CONNAC2X_RX_VT_SHORT_GI_OFFSET_V2;
            ucRxStbc = (u4RxVector0 & CONNAC2X_RX_VT_STBC_MASK_V2)
                       >> CONNAC2X_RX_VT_STBC_OFFSET_V2;
            ucRxmode = (u4RxVector0 &
                        CONNAC2X_RX_VT_RX_MODE_MASK_V2)
                       >> CONNAC2X_RX_VT_RX_MODE_OFFSET_V2;
        } else {
            /* Group3 PRXV0[0:31] */
            u4RxVector0 =
                prGlueInfo->prAdapter->
                arStaRec[ucStaIdx].u4RxVector0;
            /* Group5 C-B-0[0:31] */
            u4RxVector1 =
                prGlueInfo->prAdapter->
                arStaRec[ucStaIdx].u4RxVector1;

            /* P-RXV0 */
            ucRxrate = (u4RxVector0 & CONNAC2X_RX_VT_RX_RATE_MASK)
                       >> CONNAC2X_RX_VT_RX_RATE_OFFSET;
            ucRxldpc = (u4RxVector0 &
                        CONNAC2X_RX_VT_LDPC)
                       >> CONNAC2X_RX_VT_LDPC_OFFSET;

            /* C-B-0 */
            ucRxStbc = (u4RxVector1 & CONNAC2X_RX_VT_STBC_MASK)
                       >> CONNAC2X_RX_VT_STBC_OFFSET;
            ucRxmode = (u4RxVector1 & CONNAC2X_RX_VT_RX_MODE_MASK)
                       >> CONNAC2X_RX_VT_RX_MODE_OFFSET;
            ucRxsgi = (u4RxVector1 & CONNAC2X_RX_VT_SHORT_GI_MASK)
                      >> CONNAC2X_RX_VT_SHORT_GI_OFFSET;
        }
    } else
        goto out;

    if (ucRxmode == TX_RATE_MODE_CCK)
        ucRxphy = ucRxrate < 4 ? HW_TX_RATE_CCK_STR[ucRxrate] :
                  (ucRxrate < 8 ? HW_TX_RATE_CCK_STR[ucRxrate - 4] :
                   HW_TX_RATE_CCK_STR[4]);
    else if (ucRxmode == TX_RATE_MODE_OFDM) {
        switch (ucRxrate) {
            case 11: /* 6M */
                ucRxphy = HW_TX_RATE_OFDM_STR[0];
                break;
            case 15: /* 9M */
                ucRxphy = HW_TX_RATE_OFDM_STR[1];
                break;
            case 10: /* 12M */
                ucRxphy = HW_TX_RATE_OFDM_STR[2];
                break;
            case 14: /* 18M */
                ucRxphy = HW_TX_RATE_OFDM_STR[3];
                break;
            case 9: /* 24M */
                ucRxphy = HW_TX_RATE_OFDM_STR[4];
                break;
            case 13: /* 36M */
                ucRxphy = HW_TX_RATE_OFDM_STR[5];
                break;
            case 8: /* 48M */
                ucRxphy = HW_TX_RATE_OFDM_STR[6];
                break;
            case 12: /* 54M */
                ucRxphy = HW_TX_RATE_OFDM_STR[7];
                break;
            default:
                ucRxphy = HW_TX_RATE_OFDM_STR[8];
        }
    }

    /**assign variable*/
    wifi_statistic->Current_Temperature =
        prQueryStaStatistics->ucTemperature;
    wifi_statistic->Tx_Success_Count = ucSkipAr ? u4TotalTxCnt[u2Idx] -
                                       u4TotalFailCnt[u2Idx] : prQueryStaStatistics->u4TransmitCount -
                                       prQueryStaStatistics->u4TransmitFailCount;
    wifi_statistic->Tx_Fail_Count = ucSkipAr ? u4TotalFailCnt[u2Idx] :
                                    prQueryStaStatistics->u4TransmitFailCount;
    wifi_statistic->Rx_Success_Count = au4RxMpduCnt[0];
    wifi_statistic->Rx_with_CRC = prHwMibInfo->rHwMibCnt.u4RxFcsErrCnt;
    wifi_statistic->Rx_Drop_due_to_out_of_resources =
        prHwMibInfo->rHwMibCnt.u4RxFifoFullCnt;
    wifi_statistic->Rssi =
        RCPI_TO_dBm(prQueryStaStatistics->ucRcpi);
    wifi_statistic->MIC_Error_Count =
        RX_GET_CNT(prRxCtrl, RX_MIC_ERROR_DROP_COUNT);
    wifi_statistic->AMPDU_Tx_Success =
        prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt;
    wifi_statistic->AMPDU_Tx_Fail = prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt -
                                    prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt;
    wifi_statistic->SNR = snr;
    wifi_statistic->BBPCurrentBW =
        prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability;
    wifi_statistic->REAL_TX_PHY_Rate = ucTxphy;
    wifi_statistic->REAL_TX_PHY_Mode = ucTxmode;
    wifi_statistic->REAL_TX_ShortGI = ucTxsgi;
    wifi_statistic->REAL_TX_MCS = ucTxrate;
    wifi_statistic->REAL_TX_LDPC = ucTxldpc;
    wifi_statistic->REAL_TX_STBC = ucTxStbc;
    wifi_statistic->REAL_RX_PHY_Rate = ucRxphy;
    wifi_statistic->REAL_RX_PHY_Mode = ucRxmode;
    wifi_statistic->REAL_RX_ShortGI = ucRxsgi;
    wifi_statistic->REAL_RX_MCS = ucRxrate;
    wifi_statistic->REAL_RX_LDPC = ucRxldpc;
    wifi_statistic->REAL_RX_STBC = ucRxStbc;
    wifi_statistic->Tx_AGG_Range_1 =
        prHwMibInfo->rHwTxAmpduMts.u2TxRange1AmpduCnt;
    wifi_statistic->Tx_AGG_Range_2 =
        prHwMibInfo->rHwTxAmpduMts.u2TxRange2AmpduCnt;
    wifi_statistic->Tx_AGG_Range_3 =
        prHwMibInfo->rHwTxAmpduMts.u2TxRange3AmpduCnt;
    wifi_statistic->Tx_AGG_Range_4 =
        prHwMibInfo->rHwTxAmpduMts.u2TxRange4AmpduCnt;

out:
    if (prHwWlanInfo)
        kalMemFree(prHwWlanInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_HW_WLAN_INFO));
    if (prQueryStaStatistics)
        kalMemFree(prQueryStaStatistics, VIR_MEM_TYPE,
                   sizeof(struct PARAM_GET_STA_STATISTICS));
    if (prHwMibInfo)
        kalMemFree(prHwMibInfo, VIR_MEM_TYPE,
                   sizeof(struct PARAM_HW_MIB_INFO));

    if (rStatus != WLAN_STATUS_SUCCESS)
        status = WIFI_FAIL;

    return status;
}
/**
* @brief Set single sku power table
*
* @param [IN]14 channle * 19 power per channel, total 266 bytes power
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_single_sku_2G(singleSKU2G_t *sku_table_2g)
{
    int32_t status = WIFI_FAIL;

    LOG_FUNC("Not support wifi_config_set_single_sku_2G!\n");
    return status;
}

/**
* @brief Get the rssi of the connected AP.
*
* @param [OUT]rssi The rssi of the connected AP will be returned
*
* @return  >=0 means success, <0 means fail
*
* @note  Only used for STA mode and the station has connected to the AP.
*
*/
int32_t wifi_connection_get_rssi(int8_t *rssi)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    int32_t temp;
    uint32_t u4BufLen;
    int32_t status;

    prGlueInfo = &g_rGlueInfo;
    status = kalIoctl(prGlueInfo, wlanoidQueryRssi, &temp,
                      sizeof(temp), TRUE, FALSE, FALSE, &u4BufLen);
    if (status != WIFI_SUCC)
        LOG_FUNC("unable to retrieve rssi\n");
    else
        *rssi = temp;
    return status;
}

int32_t wifi_config_get_connected_ap_info(connected_ap_info_s *ap_info)
{
    int32_t status = 0;
    struct ADAPTER *prAdapter;
    struct BSS_DESC *prBssDesc;
    uint8_t aucBSSID[MAC_ADDR_LEN];

    prAdapter = &g_rAdapter;
    status = wifi_config_get_bssid((uint8_t *)aucBSSID);
    if (status >= 0) {
        prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
                                                    aucBSSID, FALSE, NULL);
        if (prBssDesc == NULL)
            return WIFI_FAIL;
        ap_info->beacon_interval = prBssDesc->u2BeaconInterval;
        ap_info->dtim_period = prBssDesc->ucDTIMPeriod;
    }
    return status;
}

/**
 * @brief This function sends a raw Wi-Fi packet over the air.
 *
 * @param[in] raw_packet is a pointer to the raw packet which is a complete
 * 802.11 packet including the 802.11 header and the payload. The FCS will be
 * automatically computed and attached to the end of the raw packet by hardware.
 *            Note that some fields are controlled by hardware, such as power
 * management, sequence number and duration. So the value of these fields set by
 * software will be ignored.
 * @param[in] length is the length of the raw packet.
 *
 * @return  >=0 the operation completed successfully, <0 the operation failed.
 */
int32_t wifi_connection_send_raw_packet(uint8_t *raw_packet, uint32_t length)
{
#if (CFG_ENABLE_WIFI_DIRECT && MTK_MINISUPP_ENABLE)
    uint64_t cookie = 0;
    int32_t status = 0;

    /* int mtk_p2p_freertos_wpa_mgmt_tx(void *priv,
     * int freq, unsigned int wait_time,
     * const void *data, uint32_t len, uint64_t *cookie,
     * int no_cck, int no_ack, int offchan)
     */
    status = (int32_t) mtk_p2p_freertos_wpa_mgmt_tx(NULL, 0, 0,
                                                    (void *)raw_packet, length,
                                                    &cookie, 1, 1, 0);

    return status;
#else /* #if (CFG_ENABLE_WIFI_DIRECT && MTK_MINISUPP_ENABLE) */
    LOG_FUNC("send raw 802.11 packet is not supported.\n");

    return -1;
#endif /* #if (CFG_ENABLE_WIFI_DIRECT && MTK_MINISUPP_ENABLE) */
}

/**
* @brief This function initializes the scan table in the Wi-Fi driver.
*
* @param[in] ap_list is a pointer of the user buffer where the further scanned
* AP list will be stored.
* @param[in] max_count is the maximum number of ap_list can be stored.
*
* @return   >=0 the operation completed successfully, <0 the operation failed.
*
* @note #wifi_connection_scan_init() should be called before calling
* #wifi_connection_start_scan(), and it should be called only once to
* initialize one scan.
*       When the scan is done, the scanned AP list is already stored in
* parameter ap_list with descending order of the RSSI values.
*/
int32_t wifi_connection_scan_init(wifi_scan_list_item_t *ap_list,
                                  uint32_t max_count)
{
    if (ap_list == NULL)
        return WIFI_FAIL;
#ifdef MTK_MINISUPP_ENABLE
    wifi_os_task_enter_critical();
    if ((g_scan_inited != 1) && (g_scan_list == NULL)) {
        g_scan_list = ap_list;
        g_scan_list_size = max_count;
        os_memset(g_scan_list, 0, g_scan_list_size * sizeof(wifi_scan_list_item_t));
        g_scan_inited = 1;
        wifi_os_task_exit_critical();
    } else {
        wifi_os_task_exit_critical();
        return WIFI_FAIL;
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return 0;
}

/**
* @brief This function deinitializes the scan table.
*
* @return   >=0 the operation completed successfully, <0 the operation failed.
*
* @note When the scan is finished, #wifi_connection_scan_deinit() should be
* called to unload the buffer from the Wi-Fi driver. After that, the data
* in the parameter ap_list can be safely processed by user applications,
* and then another scan can be initialized by calling
* #wifi_connection_scan_init().
*/
int32_t wifi_connection_scan_deinit(void)
{
#ifdef MTK_MINISUPP_ENABLE
    if (g_scan_inited == 0)
        return WIFI_FAIL;
    g_scan_inited = 0;
    g_scan_list = NULL;
    g_scan_list_size = 0;
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return 0;
}

int32_t wifi_connection_start_scan(uint8_t *ssid, uint8_t ssid_length,
                                   uint8_t *bssid, uint8_t scan_mode,
                                   uint8_t scan_option)
{
#ifdef MTK_MINISUPP_ENABLE
    uint8_t opmode;
    int status;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    if ((ssid != NULL) && (ssid_length > WIFI_MAX_LENGTH_OF_SSID))
        return WIFI_ERR_PARA_INVALID;
    if (scan_mode > 1)
        return WIFI_ERR_PARA_INVALID;
    if (scan_option > 2)
        return WIFI_ERR_PARA_INVALID;

#ifdef MTK_MINISUPP_ENABLE
    status = wifi_config_get_opmode_internal(&opmode);
    if (status >= 0) {
        if (opmode == WIFI_MODE_STA_ONLY && scan_mode == 0) {
            mtk_supplicant_set_passive_scan(scan_option);
            if (g_scanning) {
                wifi_connection_stop_scan();
                g_scanning = 0;
            }
            g_scan_by_app = 1;

            mtk_supplicant_start_scan();
            return WIFI_SUCC;
        } else
            LOG_FUNC("Not support.\n");
    }
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    return WIFI_FAIL;
}


/**
* @brief Stop WiFi Scanning
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_connection_stop_scan(void)
{
#ifdef MTK_MINISUPP_ENABLE
    g_scanning = 0;
    mtk_supplicant_stop_scan();
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return WIFI_SUCC;
}

/**
 * @brief Turn on\off the scheduled scan
 *
 * @return >=0 means success, <0 means fail
 *
 * @params [Mode]off=0, on=1
 */
int32_t wifi_connnect_set_sched_scan(uint8_t mode)
{
    int32_t status = WIFI_SUCC;

    if (mode > 1) {
        LOG_FUNC("[Mode]off=0, on=1");
        status =  WIFI_ERR_PARA_INVALID;
        return WIFI_FAIL;
    }

#ifdef MTK_MINISUPP_ENABLE
    mtk_supplicant_set_sched_scan(mode);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    return status;
}

/**
* @brief Get WiFi Associated Station List
*
* @param [OUT]sta_list
* @param [OUT]number Number of associated stations will be returned
*
* @return  >=0 means success, <0 means fail
*
*/
int32_t wifi_connection_get_sta_list(uint8_t *number, wifi_sta_list_t *sta_list)
{
#if CFG_ENABLE_WIFI_DIRECT
    struct GLUE_INFO *prGlueInfo = NULL;
    struct BSS_INFO *prBssInfo = NULL;
    struct LINK *prClientList;
    struct STA_RECORD *prCurrStaRec, *prNextStaRec;
    wifi_sta_list_t *list = NULL;
    wifi_statistic_t statistic;
    int32_t i = 0;
    int32_t status = WIFI_SUCC;
    uint8_t ucBssIndex;

    prGlueInfo = &g_rGlueInfo;
    ucBssIndex = p2pFuncGetSapBssIndex(prGlueInfo);
    prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
    if (!prBssInfo) {
        LOG_FUNC("bss is not active\n");
        return WIFI_FAIL;
    }

    *number = 0;
    prClientList = &prBssInfo->rStaRecOfClientList;

    LINK_FOR_EACH_ENTRY_SAFE(prCurrStaRec,
                             prNextStaRec, prClientList, rLinkEntry, struct STA_RECORD) {
        if (!prCurrStaRec)
            break;

        if (!prCurrStaRec->fgIsInUse) {
            LOG_FUNC("StaRec is not in use\n");
            continue;
        }

        list = sta_list + i;

        status = wifi_config_get_statistic_per_sta(&statistic,
                                                   prCurrStaRec->aucMacAddr);
        if (status != WIFI_SUCC) {
            LOG_FUNC("fail to get sta stat " MACSTR "\n",
                     MAC2STR(prCurrStaRec->aucMacAddr));
            break;
        }

        COPY_MAC_ADDR(list->mac_address, prCurrStaRec->aucMacAddr);

        list->last_tx_rate.field.mcs = statistic.REAL_TX_MCS;
        list->last_tx_rate.field.ldpc = statistic.REAL_TX_LDPC;
        list->last_tx_rate.field.short_gi = statistic.REAL_TX_ShortGI;
        list->last_tx_rate.field.stbc = statistic.REAL_TX_STBC;
        list->last_tx_rate.field.mode = statistic.REAL_TX_PHY_Mode;

        list->last_rx_rate.field.mcs = statistic.REAL_RX_MCS;
        list->last_rx_rate.field.ldpc = statistic.REAL_RX_LDPC;
        list->last_rx_rate.field.short_gi = statistic.REAL_RX_ShortGI;
        list->last_rx_rate.field.stbc = statistic.REAL_RX_STBC;
        list->last_rx_rate.field.mode = statistic.REAL_RX_PHY_Mode;

        list->rssi_sample.last_rssi = statistic.Rssi;
        list->power_save_mode = prCurrStaRec->fgIsInPS;
        list->bandwidth = statistic.BBPCurrentBW;
        list->keep_alive = 0;

        *number = ++i;
    }

    return status;
#else /* #if CFG_ENABLE_WIFI_DIRECT */
    *number = 0;

    return WIFI_SUCC;
#endif /* #if CFG_ENABLE_WIFI_DIRECT */
}

/**
* @brief Register WiFi Event Notifier
*
* @param [IN]event
* @param event Event ID
* @param LinkUP (0)
* @param SCAN COMPLETE(1)
* @param DISCONNECT(2)
* @param PortSecured Event(3)
* @param Report Beacon/ProbeResponse(4)
* @param Report WPS Credential(5)
* @param [IN]notifier
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_connection_register_event_handler(wifi_event_t event,
                                               wifi_event_handler_t handler)
{
#ifdef MTK_MINISUPP_ENABLE
    return wifi_api_set_event_handler(1, event, handler);
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return WIFI_FAIL;
}

int32_t wifi_connection_register_event_notifier(uint8_t event,
                                                wifi_event_handler_t notifier)
{
    return wifi_connection_register_event_handler((wifi_event_t)event,
                                                  notifier);
}


/**
* @brief Register WiFi Event Notifier
*
* @param [IN]event
* @param [IN]notifier
* @param event Event ID
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_connection_unregister_event_handler(wifi_event_t event,
                                                 wifi_event_handler_t handler)
{
#ifdef MTK_MINISUPP_ENABLE
    return wifi_api_set_event_handler(0, event, handler);
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return WIFI_FAIL;
}

int32_t wifi_connection_unregister_event_notifier(uint8_t event,
                                                  wifi_event_handler_t notifier)
{
    return wifi_connection_unregister_event_handler((wifi_event_t)event,
                                                    notifier);
}

/*
* @brief rx filter mask, generated by all the open bits
*/
uint32_t g_rx_filter_mask = (
                                BIT(WIFI_RX_FILTER_DROP_STBC_BCN_BC_MC) |
                                BIT(WIFI_RX_FILTER_DROP_FCS_ERR) |
                                BIT(WIFI_RX_FILTER_DROP_VER_NOT_0) |
                                BIT(WIFI_RX_FILTER_DROP_PROBE_REQ) |
                                BIT(WIFI_RX_FILTER_DROP_MC_FRAME) |
                                BIT(WIFI_RX_FILTER_DROP_BC_FRAME) |
                                BIT(WIFI_RX_FILTER_DROP_BSSID_BCN) |
                                BIT(WIFI_RX_FILTER_RM_FRAME_REPORT_EN) |
                                BIT(WIFI_RX_FILTER_DROP_CTRL_RSV) |
                                BIT(WIFI_RX_FILTER_DROP_CTS) |
                                BIT(WIFI_RX_FILTER_DROP_RTS) |
                                BIT(WIFI_RX_FILTER_DROP_DUPLICATE) |
                                BIT(WIFI_RX_FILTER_DROP_NOT_MY_BSSID) |
                                BIT(WIFI_RX_FILTER_DROP_NOT_UC2ME) |
                                BIT(WIFI_RX_FILTER_DROP_DIFF_BSSID_BTIM) |
                                BIT(WIFI_RX_FILTER_DROP_NDPA)
                            );

/**
* @brief Configure packet format wanted to be received
*
* @param flag [IN] flag defined in wifi_rx_filter_t.
*
* @return >=0 means success, <0 means fail
*
* @note Default value will be WIFI_DEFAULT_IOT_RX_FILTER
*/
int32_t wifi_config_set_rx_filter(uint32_t flag)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    struct ADAPTER *prAdapter = NULL;
    uint32_t rStatus = WIFI_FAIL;
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
    uint32_t u4BufLen = 0;
    char *cCmd = "RxFilter";
    char *cRxFilter;

    uint32_t u4PacketFilter = (flag & g_rx_filter_mask);

    if (u4PacketFilter != flag) {
        LOG_FUNC("Unsupported bits have been masked. origin: 0x%x to: 0x%x",
                 flag, u4PacketFilter);
    }

    prGlueInfo = &g_rGlueInfo;
    prAdapter = prGlueInfo->prAdapter;
    if (prAdapter == NULL)
        return WIFI_FAIL;

    cRxFilter = (char *)kalMemAlloc(sizeof(int), VIR_MEM_TYPE);
    if (cRxFilter == NULL)
        goto out;;

    kalSprintf(cRxFilter, " %d", u4PacketFilter);

    kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
    rChipConfigInfo.u2MsgSize = kalStrLen(cCmd) + kalStrLen(cRxFilter) + 1;
    kalStrnCpy((char *)rChipConfigInfo.aucCmd, cCmd,
               CHIP_CONFIG_RESP_SIZE - 1);
    kalStrnCpy((char *)rChipConfigInfo.aucCmd + kalStrLen(cCmd),
               cRxFilter, kalStrLen(cRxFilter));
    rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

    rStatus = kalIoctl(prGlueInfo, wlanoidSetChipConfig,
                       &rChipConfigInfo, sizeof(rChipConfigInfo),
                       FALSE, FALSE, TRUE, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(REQ, INFO, "%s: kalIoctl ret=%d\n", __func__, rStatus);

out:
    if (cRxFilter)
        kalMemFree(cRxFilter, VIR_MEM_TYPE, sizeof(*cRxFilter));

    return rStatus;
}

/**
* @brief Configure MAR table wanted to be received
*
* @param flag [IN] flag defined in wifi_rx_filter_t.
*
* @return >=0 means success, <0 means fail
*
* @note Default value will be WIFI_DEFAULT_IOT_RX_FILTER
*/
int32_t wifi_config_set_mc_address(uint32_t len, void *AddrList)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus = WIFI_FAIL;
    uint32_t i, total_len, u4SetInfoLen;
    uint8_t arAddress[MAX_NUM_GROUP_ADDR][MAC_ADDR_LEN] = {0};

    prGlueInfo = &g_rGlueInfo;

    if (len > MAX_NUM_GROUP_ADDR)
        return rStatus;

    total_len = sizeof(uint8_t) * len * MAC_ADDR_LEN;
    if (len > 0)
        kalMemCopy(arAddress, AddrList,
                   total_len);

    DBGLOG(OID, ERROR,
           "MCAST white list: total=%d", len);
    for (i = 0; i < len; i++) {
        DBGLOG(OID, ERROR,
               "MAC%d="MACSTR"\n",
               i,
               MAC2STR(arAddress[i])
              );
    }

    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSetMulticastList, arAddress, total_len,
                       FALSE, FALSE, TRUE, &u4SetInfoLen);

    return rStatus;
}




/**
* @brief Configure packet format wanted to be received
*
* @param   flag [OUT] flag defined in  wifi_rx_filter_t
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_get_rx_filter(uint32_t *flag)
{
    struct GLUE_INFO *prGlueInfo;
    int32_t rStatus = WIFI_FAIL;
    uint32_t u4BufLen = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    char *cCmd = "RxFilter";
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

    prGlueInfo = &g_rGlueInfo;

    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
    rChipConfigInfo.u2MsgSize = kalStrLen(cCmd) + 1;
    kalStrnCpy((char *)rChipConfigInfo.aucCmd, cCmd,
               CHIP_CONFIG_RESP_SIZE - 1);
    rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryChipConfig,
                       &rChipConfigInfo, sizeof(rChipConfigInfo),
                       TRUE, TRUE, TRUE, &u4BufLen);
    if (rStatus != WIFI_SUCC)
        return WIFI_FAIL;

    wlanCfgParseArgument((char *)rChipConfigInfo.aucCmd, &i4Argc, apcArgv);
    rStatus = kalkStrtou32(apcArgv[0], 0, flag);
    if (rStatus)
        LOG_FUNC("Parse error rStatus=%d\n", rStatus);

    return rStatus;
}

/**
* @brief This function is for mt7933 rx handler.
*
* @param[in] mode, mt7933 must set this mode to 1 before register rx handler.
*
* @return  >=0 the operation completed successfully, <0 the operation failed.
*/
int32_t wifi_config_set_rx_handler_mode(uint8_t mode)
{
    int32_t status = WIFI_FAIL;
#ifdef MTK_MINISUPP_ENABLE
    g_rx_handler_mode = mode;
    status = WIFI_SUCC;
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return status;

}

void connsys_set_rxraw_handler(wifi_rx_handler_t handler)
{
    connsys_raw_handler = handler;
}

uint8_t raw_packet_handler_enabled(void)
{
    return (connsys_raw_handler != NULL);
}

int32_t raw_packet_handler(uint8_t *payload, unsigned int len)
{
    if (connsys_raw_handler != NULL)
        return (*connsys_raw_handler)(payload, len);
    return -1;
}

/**
* @brief This function parses the data descriptor in raw packet.
*
* @param[in]  payload is the raw date that be recieved from RX filter handler.
* @param[out] data is the descriptor information parse from payload, please refer to #wifi_data_parse_t.
*
* @return  >=0 the operation completed successfully, <0 the operation failed.
*
*/
int32_t wifi_connection_parse_data_descriptor(uint8_t *payload,
                                              struct wifi_data_parse_t *data)
{
    struct HW_MAC_CONNAC2X_RX_DESC *prRxStatus;
    uint8_t ucMacHeaderLength;
    uint16_t u2PayloadLength;
    uint8_t *pucMacHeader;       /* 802.11 header  */
    uint8_t *pucPayload;         /* 802.11 payload */
    uint16_t u2RxStatusOffset;
    uint32_t u4HeaderOffset;
    uint8_t ucGroupVLD;

    prRxStatus = (struct HW_MAC_CONNAC2X_RX_DESC *)(payload);
    u4HeaderOffset = (uint32_t)(
                         HAL_MAC_CONNAC2X_RX_STATUS_GET_HEADER_OFFSET(prRxStatus));
    u2RxStatusOffset = sizeof(struct HW_MAC_CONNAC2X_RX_DESC);
    ucGroupVLD =
        (uint8_t) HAL_MAC_CONNAC2X_RX_STATUS_GET_GROUP_VLD(prRxStatus);
    if (ucGroupVLD & BIT(RX_GROUP_VLD_4)) {
        u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_4);
    }
    if (ucGroupVLD & BIT(RX_GROUP_VLD_1)) {
        u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_1);
    }
    if (ucGroupVLD & BIT(RX_GROUP_VLD_2)) {
        u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_2);
    }
    if (ucGroupVLD & BIT(RX_GROUP_VLD_3)) {
        u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_3_V2);
    }
    if (ucGroupVLD & BIT(RX_GROUP_VLD_5)) {
        u2RxStatusOffset += sizeof(struct HW_MAC_RX_STS_GROUP_5);
    }

    pucMacHeader = (uint8_t *) prRxStatus +
                   u2RxStatusOffset + u4HeaderOffset;
    ucMacHeaderLength = (uint16_t)
                        HAL_MAC_CONNAC2X_RX_STATUS_GET_HEADER_LEN(prRxStatus);
    pucPayload = (uint8_t *)((uint32_t)(pucMacHeader + ucMacHeaderLength));
    u2PayloadLength =
        (uint16_t)((uint32_t) HAL_MAC_CONNAC2X_RX_STATUS_GET_RX_BYTE_CNT(
                       prRxStatus) - (u2RxStatusOffset + u4HeaderOffset));

    data->data_rate = 0;
    data->rssi = 0; /*asicConnac2xRxGetRcpiValueFromRxv*/
    data->mac_header = pucMacHeader;
    data->mac_header_len = ucMacHeaderLength;
    data->packet_payload = pucPayload;
    data->packet_payload_len = u2PayloadLength;

    return WIFI_SUCC;
}


/**
* @brief Register the handler to receive 802.11 raw packet from network processor
* @brief and the network processor begin to indicate 802.11 raw packet with RXWI
*
* @param [IN]wifi_rx_handler: handler routine
*
* @return >=0 means success, <0 means fail
*
*/
uint8_t raw_sleep_lock = 0xff;
int32_t wifi_config_register_rx_handler(wifi_rx_handler_t wifi_rx_handler)
{
    int32_t status = 0;
    //uint8_t enable = 0;

    wifi_event_notification(WIFI_PORT_STA, WIFI_EVENT_ID_IOT_REGISTER_RX_HANDLER, NULL, 0);

    connsys_set_rxraw_handler(wifi_rx_handler);


    /*if (wifi_rx_handler != NULL) {
        enable = 1;
    }

    raw_sleep_lock = wifi_set_sleep_handle("raw_pkt");
    if(raw_sleep_lock != 0xff)
        wifi_lock_sleep(raw_sleep_lock);

    status = wifi_inband_set_raw_pkt(enable);*/

    return status;
}

/**
* @brief Unregister the handler to receive 802.11 raw packet from network processor
* @brief and the network processor begin to indicate 802.11 raw packet with RXWI
*
* @param [IN]wifi_rx_handler: handler routine
*
* @return >=0 means success, <0 means fail
*
*/
int32_t wifi_config_unregister_rx_handler()
{
    int32_t status = 0;
    //uint8_t enable = 0;

    connsys_set_rxraw_handler(NULL);

    wifi_event_notification(WIFI_PORT_STA,  WIFI_EVENT_ID_IOT_UNREGISTER_RX_HANDLER, NULL, 0);


    /*if ((status = wifi_inband_set_raw_pkt(enable)) < 0) {
        return status;
    }

    if(raw_sleep_lock != 0xff){
        wifi_unlock_sleep(raw_sleep_lock);
        wifi_release_sleep_handle(raw_sleep_lock);
        raw_sleep_lock = 0xff;
    }*/

    return status;
}

/**
* @brief Register the handler to handle wakeup packet when wow enable
*
* @param [IN]wifi_rx_handler: handler routine
*
* @return >=0 means success, <0 means fail
*
*/
int32_t wifi_config_register_wow_handler(
    wifi_rx_handler_t wifi_rx_handler,
    uint8_t type)
{
    return nic_rx_set_wow_handler(wifi_rx_handler, type);
}

/**
* @brief Unregister the handler to handle wakeup packet when wow enable
*
* @param [IN]wifi_rx_handler: handler routine
*
* @return >=0 means success, <0 means fail
*
*/
int32_t wifi_config_unregister_wow_handler(uint8_t type)
{
    return nic_rx_set_wow_handler(NULL, type);
}

/**
* @brief Inform wpa_supplicant to reload configuraion.
*
* @return  >=0 means success, <0 means fail
*
* @note The mt7682 and mt7687 have different behavior. mt7687 use
*       reload_result_semphr to sync with minisupp
*/
int32_t wifi_config_reload_setting(void)
{
    int32_t ret = WIFI_FAIL;
    uint8_t opmode;
#ifdef MTK_MINISUPP_ENABLE
    uint8_t port;
    uint8_t zero_mac[WIFI_MAC_ADDRESS_LENGTH] = {0x00, 0x00, 0x00,
                                                 0x00, 0x00, 0x00
                                                };
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    /*
    if(security_use_flag == true) {
        wifi_set_security_valid(true);
    } else {
        wifi_set_security_valid(false);
    }
    */
    security_use_flag = false;

    if (wifi_config_get_opmode_internal(&opmode) < 0) {
        ret = WIFI_FAIL;
        goto final;
    }

#ifdef MTK_MINISUPP_ENABLE
    port = (opmode == WIFI_MODE_AP_ONLY) ? WIFI_PORT_AP : WIFI_PORT_STA;
#endif /* #ifdef MTK_MINISUPP_ENABLE */

    if (bssid_use_flag == false) {
#ifdef MTK_MINISUPP_ENABLE
        uint8_t driver_msg[WIFI_MAC_ADDRESS_LENGTH] = {0};

        os_memcpy(driver_msg, zero_mac, WIFI_MAC_ADDRESS_LENGTH);
        if (wifi_event_notification_wait(port,
                                         WIFI_EVENT_ID_IOT_SET_BSSID_WAIT,
                                         (unsigned char *)&driver_msg,
                                         sizeof(driver_msg)) != WIFI_SUCC) {
            ret = WIFI_FAIL;
            goto final;
        }
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    }
    bssid_use_flag = false;

#ifdef MTK_MINISUPP_ENABLE
    ret = wifi_event_notification_wait(port,
                                       WIFI_EVENT_ID_IOT_RELOAD_CONFIGURATION, NULL, 0);
    if (opmode == WIFI_MODE_REPEATER && port == WIFI_PORT_APCLI)
        ret = wifi_event_notification_wait(WIFI_PORT_AP,
                                           WIFI_EVENT_ID_IOT_RELOAD_CONFIGURATION, NULL, 0);
#endif /* #ifdef MTK_MINISUPP_ENABLE */

final:
    return ret;
}

int32_t wifi_config_get_bcn_lost_cnt(uint16_t *value)
{
    struct GLUE_INFO *prGlueInfo;
    int32_t rStatus;
    uint32_t u4BufLen = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    char *cmd = "BcnTimeoutCnt";
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

    prGlueInfo = &g_rGlueInfo;
    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
    rChipConfigInfo.u2MsgSize = kalStrLen(" BcnTimeoutCnt");

    kalStrnCpy((char *)rChipConfigInfo.aucCmd, cmd,
               CHIP_CONFIG_RESP_SIZE - 1);
    rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryChipConfig,
                       &rChipConfigInfo, sizeof(rChipConfigInfo),
                       TRUE, TRUE, TRUE, &u4BufLen);

    if (rStatus != WIFI_SUCC)
        goto out;

    wlanCfgParseArgument((char *)rChipConfigInfo.aucCmd, &i4Argc, apcArgv);
    rStatus = kalkStrtou16(apcArgv[2], 0, value);
    if (rStatus)
        LOG_FUNC("Parse error rStatus=%d\n", rStatus);
out:
    return rStatus;
}

int32_t wifi_config_clear_bcn_lost_cnt(void)
{
    struct GLUE_INFO *prGlueInfo;
    uint32_t rStatus;
    uint32_t u4BufLen = 0;
    char *cmd = "BcnTimeoutCnt 0";
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

    prGlueInfo = &g_rGlueInfo;
    kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;

    rChipConfigInfo.u2MsgSize = kalStrLen(" BcnTimeoutCnt 0");

    kalStrnCpy((char *)rChipConfigInfo.aucCmd, cmd,
               CHIP_CONFIG_RESP_SIZE - 1);
    rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

    rStatus = kalIoctl(prGlueInfo, wlanoidSetChipConfig,
                       &rChipConfigInfo, sizeof(rChipConfigInfo),
                       FALSE, FALSE, TRUE, &u4BufLen);

    return rStatus;
}

void wifi_get_connection_get_rssi_load_registers(uint32_t *register_list,
                                                 uint32_t max_size)
{
    LOG_FUNC("Not support wifi_get_connection_get_rssi_load_registers!");
}
#ifdef MTK_MINISUPP_ENABLE
void register_print_wifi_cb(pPrint_wifi __print_wifi)
{
    LOG_FUNC("Not support register_print_wifi_cb!");
}
#endif /* #ifdef MTK_MINISUPP_ENABLE */
int32_t wifi_config_set_ps_dynmic(uint8_t on_off)
{
    LOG_FUNC("Not support wifi_config_set_ps_dynmic!");
    return WIFI_FAIL;
}

int32_t wifi_config_get_ps_dynmic(uint8_t *on_off)
{
    LOG_FUNC("Not support wifi_config_get_ps_dynmic!");
    return WIFI_FAIL;
}

int32_t wifi_config_set_pretbtt(uint8_t value)
{
    struct GLUE_INFO *prGlueInfo;
    uint8_t ucBssIndex = 0;
    int32_t rStatus = WIFI_FAIL;
    uint32_t u4BufLen = 0;
    char *cCmd = "pretbtt";
    char *cBssIdx;
    char *cValue;
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

    prGlueInfo = &g_rGlueInfo;
    ucBssIndex = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    cBssIdx = (char *)kalMemAlloc(sizeof(int), VIR_MEM_TYPE);
    cValue = (char *)kalMemAlloc(sizeof(int), VIR_MEM_TYPE);
    if (cBssIdx == NULL || cValue == NULL)
        goto out;

    rStatus = kalSprintf(cBssIdx, " %d", ucBssIndex);
    if (rStatus < 0)
        goto out;
    rStatus = kalSprintf(cValue, " %d", value);
    if (rStatus < 0)
        goto out;

    kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;

    rChipConfigInfo.u2MsgSize = kalStrLen(cCmd) + kalStrLen(cBssIdx)
                                + kalStrLen(cValue) + 1; /**plus 1 for space*/

    kalStrnCpy((char *)rChipConfigInfo.aucCmd, cCmd,
               CHIP_CONFIG_RESP_SIZE - 1);
    kalStrnCpy((char *)rChipConfigInfo.aucCmd + kalStrLen(cCmd),
               cBssIdx, kalStrLen(cBssIdx));
    kalStrnCpy((char *)rChipConfigInfo.aucCmd + kalStrLen(cCmd) +
               kalStrLen(cBssIdx), cValue, kalStrLen(cValue));
    rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

    rStatus = kalIoctl(prGlueInfo, wlanoidSetChipConfig,
                       &rChipConfigInfo, sizeof(rChipConfigInfo),
                       FALSE, FALSE, TRUE, &u4BufLen);
out:
    if (cBssIdx)
        kalMemFree(cBssIdx, VIR_MEM_TYPE, sizeof(*cBssIdx));
    if (cValue)
        kalMemFree(cValue, VIR_MEM_TYPE, sizeof(*cValue));

    return rStatus;
}

int32_t wifi_config_get_pretbtt(uint8_t *value)
{
    struct GLUE_INFO *prGlueInfo;
    uint8_t ucBssIndex = 0;
    int32_t rStatus = WIFI_FAIL;
    uint32_t u4BufLen = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    char *cCmd = "pretbtt";
    char *cBssIdx;
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

    prGlueInfo = &g_rGlueInfo;
    ucBssIndex = prGlueInfo->prAdapter->prAisBssInfo[0]->ucBssIndex;
    cBssIdx = (char *)kalMemAlloc(sizeof(int), VIR_MEM_TYPE);
    if (cBssIdx == NULL)
        goto out;

    rStatus = kalSprintf(cBssIdx, " %d", ucBssIndex);
    if (rStatus < 0)
        goto out;

    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
    rChipConfigInfo.u2MsgSize = kalStrLen(cCmd) + kalStrLen(cBssIdx) + 1;

    kalStrnCpy((char *)rChipConfigInfo.aucCmd, cCmd,
               CHIP_CONFIG_RESP_SIZE - 1);
    kalStrnCpy((char *)rChipConfigInfo.aucCmd + kalStrLen(cCmd),
               cBssIdx, kalStrLen(cBssIdx));

    rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryChipConfig,
                       &rChipConfigInfo, sizeof(rChipConfigInfo),
                       TRUE, TRUE, TRUE, &u4BufLen);
    if (rStatus != WIFI_SUCC)
        goto out;

    wlanCfgParseArgument((char *)rChipConfigInfo.aucCmd, &i4Argc, apcArgv);
    rStatus = kalkStrtou8(apcArgv[0], 0, value);
    if (rStatus)
        LOG_FUNC("Parse error rStatus=%d\n", rStatus);
out:
    if (cBssIdx)
        kalMemFree(cBssIdx, VIR_MEM_TYPE, sizeof(*cBssIdx));

    return rStatus;
}

int32_t wifi_config_set_ps_log(uint8_t on_off)
{
    struct GLUE_INFO *prGlueInfo;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    uint8_t ucHostCtrl;
    struct CMD_FW_LOG_2_HOST_CTRL *prFwLog2HostCtrl;

    prGlueInfo = &g_rGlueInfo;
    prFwLog2HostCtrl = (struct CMD_FW_LOG_2_HOST_CTRL *)kalMemAlloc(
                           sizeof(struct CMD_FW_LOG_2_HOST_CTRL), VIR_MEM_TYPE);
    if (prFwLog2HostCtrl == NULL)
        goto out;

    prFwLog2HostCtrl->ucMcuDest = 0;
    if (on_off)
        ucHostCtrl = 2;
    else
        ucHostCtrl = 0;
    prFwLog2HostCtrl->ucFwLog2HostCtrl = ucHostCtrl;

    rStatus = kalIoctl(prGlueInfo, wlanoidSetFwLog2Host,
                       prFwLog2HostCtrl,
                       sizeof(struct CMD_FW_LOG_2_HOST_CTRL),
                       TRUE, TRUE, TRUE, &u4BufLen);
    if (!rStatus)
        ps_log_onoff = on_off;
out:
    if (prFwLog2HostCtrl)
        kalMemFree(prFwLog2HostCtrl, VIR_MEM_TYPE,
                   sizeof(struct CMD_FW_LOG_2_HOST_CTRL));
    return rStatus;
}

int32_t wifi_config_get_ps_log(uint8_t *on_off)
{
    *on_off = ps_log_onoff;
    return WIFI_SUCC;
}

int32_t wifi_config_set_ps_dynmic_pretbtt(uint8_t step, uint8_t value)
{
    LOG_FUNC("Not support wifi_config_set_ps_dynmic_pretbtt!");
    return WIFI_FAIL;
}

int32_t wifi_config_get_ps_dynmic_pretbtt(uint8_t step, uint8_t *value)
{
    LOG_FUNC("Not support wifi_config_get_ps_dynmic_pretbtt!");
    return WIFI_FAIL;
}

int32_t wifi_config_get_sys_temperature(uint8_t *value)
{
    uint32_t rStatus = WIFI_FAIL;
#if MTK_HAL_TOP_THERMAL_MODULE_ENABLE
    int temp;

    rStatus = connsys_thermal_query(&temp);
    *value = (uint8_t)temp;
#endif /* #if MTK_HAL_TOP_THERMAL_MODULE_ENABLE */
    return rStatus;
}

/**
* @brief Set retry limit for STA
*
* @param [IN]limit, which must be less than or equal to 31
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_retry_limit(uint8_t limit)
{
    struct GLUE_INFO *prGlueInfo;
    struct ADAPTER *prAdapter = NULL;

    if (limit > 31) {
        LOG_FUNC("The limit must be less than or equal to 31");
        return WIFI_FAIL;
    }

    prGlueInfo = &g_rGlueInfo;
    prAdapter = prGlueInfo->prAdapter;

    prAdapter->ucRetryLimit = limit;

    return WIFI_SUCC;
}

/**
* @brief Set tx rate (fixed, auto)
* @Param
* Fixed: 0:auto, 1:UseWCID
* Mode: 0:CCK 1:Legacy OFDM 2:MM 4:VHT
*       8:HE_SU 9:HE_ER, other reserved
* BW: 0:BW20 1:BW40 2:BW80 3:BW160
* MCS: CCK:0-3 OFDM:0-7 HT:0-32 VHT:0-9 HE:0-11
*/
int32_t wifi_config_set_tx_rate(uint8_t fixed, uint8_t mode,
                                uint8_t bw, uint8_t mcs)
{
    int8_t rStatus;

    rStatus = wlanSetTxRate(fixed, mode, bw, mcs, NULL);

    return rStatus;
}

int32_t wifi_config_get_twt_param(struct wifi_twt_params_t *prTwt)
{
    struct ADAPTER *prAdapter = NULL;
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
    struct _TWT_PLANNER_T *prTWTPlanner;
    struct _TWT_AGRT_T *prTWTAgrt;
    uint32_t u4TwtStatus = twtReqGetStatus();

    prAdapter = prGlueInfo->prAdapter;
    prTWTPlanner = &(prAdapter->rTWTPlanner);
    prTWTAgrt = &(prTWTPlanner->arTWTAgrtTbl[0]);

    if (prTWTAgrt->fgValid == FALSE || prTwt == NULL) {
        if ((int32_t)u4TwtStatus >= 0) {
            u4TwtStatus = TWT_REQ_STATUS_UNKNOWN_ERROR;
            twtReqSetStatus(u4TwtStatus);
        }
    } else {
        prTwt->ucTWTNego = 0;
        prTwt->ucTWTFlowID = prTWTAgrt->ucFlowId;
        prTwt->ucTWTSetup = prTWTAgrt->rTWTAgrt.fgReq;
        prTwt->ucTWTTrigger = prTWTAgrt->rTWTAgrt.fgTrigger;
        prTwt->ucTWTUnannounced = prTWTAgrt->rTWTAgrt.fgUnannounced;
        prTwt->ucTWTWakeIntervalExponent =
            prTWTAgrt->rTWTAgrt.ucWakeIntvalExponent;
        prTwt->ucTWTProtection = prTWTAgrt->rTWTAgrt.fgProtect;
        prTwt->ucTWTMinWakeDuration = prTWTAgrt->rTWTAgrt.ucMinWakeDur;
        prTwt->u2TWTWakeIntervalMantissa =
            prTWTAgrt->rTWTAgrt.u2WakeIntvalMantiss;
    }

    return (int32_t) - ((u4TwtStatus & BITS(0, 15)));
}

int32_t wifi_config_set_twt(struct wifi_twt_params_t *prTWTInput)
{
    struct ADAPTER *prAdapter = NULL;
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
    struct _TWT_CTRL_T rTWTCtrl;
    struct _TWT_PARAMS_T *prTWTParams;
    struct _MSG_TWT_PARAMS_SET_T *prTWTParamSetMsg;

    prAdapter = prGlueInfo->prAdapter;

    if ((IS_TWT_PARAM_ACTION_DEL(prTWTInput->ucTWTNego)
#if (CFG_SUPPORT_TWT_SUSPEND_RESUME == 1)
         || IS_TWT_PARAM_ACTION_SUSPEND(prTWTInput->ucTWTNego)
         || IS_TWT_PARAM_ACTION_RESUME(prTWTInput->ucTWTNego)
#endif /* #if (CFG_SUPPORT_TWT_SUSPEND_RESUME == 1) */
        )) {

        if (prTWTInput->ucTWTFlowID >= TWT_MAX_FLOW_NUM) {
            LOG_FUNC("Invalid TWT Params\n");
            twtReqSetStatus(TWT_REQ_STATUS_INVALID_PARAMETER);
            return -1;
        }

        rTWTCtrl.ucBssIdx = prGlueInfo->prDevHandler->bss_idx;
        rTWTCtrl.ucCtrlAction = prTWTInput->ucTWTNego;
        rTWTCtrl.ucTWTFlowId = prTWTInput->ucTWTFlowID;

    } else {
        if (prTWTInput->ucTWTFlowID >= TWT_MAX_FLOW_NUM ||
            prTWTInput->ucTWTSetup > TWT_SETUP_CMD_DEMAND ||
            prTWTInput->ucTWTWakeIntervalExponent > TWT_MAX_WAKE_INTVAL_EXP) {
            LOG_FUNC("Invalid TWT Params\n");
            twtReqSetStatus(TWT_REQ_STATUS_INVALID_PARAMETER);
            return -1;
        }

        prTWTParams = &(rTWTCtrl.rTWTParams);
        kalMemSet(prTWTParams, 0, sizeof(struct _TWT_PARAMS_T));
        prTWTParams->fgReq = TRUE;
        prTWTParams->ucSetupCmd = prTWTInput->ucTWTSetup;
        prTWTParams->fgTrigger = (prTWTInput->ucTWTTrigger) ? TRUE : FALSE;
        prTWTParams->fgUnannounced = (prTWTInput->ucTWTUnannounced) ? TRUE : FALSE;
        prTWTParams->ucWakeIntvalExponent = prTWTInput->ucTWTWakeIntervalExponent;
        prTWTParams->fgProtect = (prTWTInput->ucTWTProtection) ? TRUE : FALSE;
        prTWTParams->ucMinWakeDur = prTWTInput->ucTWTMinWakeDuration;
        prTWTParams->u2WakeIntvalMantiss = prTWTInput->u2TWTWakeIntervalMantissa;

        rTWTCtrl.ucBssIdx = prGlueInfo->prDevHandler->bss_idx;
        rTWTCtrl.ucCtrlAction = prTWTInput->ucTWTNego;
        rTWTCtrl.ucTWTFlowId = prTWTInput->ucTWTFlowID;
    }

    prTWTParamSetMsg = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
                                   sizeof(struct _MSG_TWT_PARAMS_SET_T));
    if (prTWTParamSetMsg) {
        prTWTParamSetMsg->rMsgHdr.eMsgId =
            MID_TWT_PARAMS_SET;
        kalMemCopy(&prTWTParamSetMsg->rTWTCtrl,
                   &rTWTCtrl, sizeof(rTWTCtrl));
        mboxSendMsg(prAdapter, MBOX_ID_0,
                    (struct MSG_HDR *) prTWTParamSetMsg,
                    MSG_SEND_METHOD_BUF);
    } else {
        twtReqSetStatus(TWT_REQ_STATUS_UNKNOWN_ERROR);
        return -1;
    }
    return 0;
}

int32_t wifi_config_set_bc_drop(uint8_t value)
{
    struct GLUE_INFO *prGlueInfo;
    int32_t rStatus = WIFI_FAIL;
    uint32_t u4BufLen = 0;
    char *cCmd = "CoalescingFilterForBC";
    char *cValue;
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

    if (value > 1)
        return WIFI_FAIL;

    prGlueInfo = &g_rGlueInfo;
    cValue = (char *)kalMemAlloc(sizeof(int), VIR_MEM_TYPE);
    if (cValue == NULL)
        goto out;

    rStatus = kalSprintf(cValue, " %d", value);
    if (rStatus < 0)
        goto out;

    kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;

    rChipConfigInfo.u2MsgSize = kalStrLen(cCmd) +
                                kalStrLen(cValue) + 1; /**plus 1 for space*/

    kalStrnCpy((char *)rChipConfigInfo.aucCmd, cCmd,
               CHIP_CONFIG_RESP_SIZE - 1);
    kalStrnCpy((char *)rChipConfigInfo.aucCmd + kalStrLen(cCmd),
               cValue, kalStrLen(cValue));
    rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

    rStatus = kalIoctl(prGlueInfo, wlanoidSetChipConfig,
                       &rChipConfigInfo, sizeof(rChipConfigInfo),
                       FALSE, FALSE, TRUE, &u4BufLen);
out:
    if (cValue)
        kalMemFree(cValue, VIR_MEM_TYPE, sizeof(*cValue));

    return rStatus;
}

#if IP_NAPT
/**
* @brief Set the timer of NAPT router to clean TCP/UDP table
*
* @Param timer: reset cycle in seconds.
*
* @return  >=0 means success, <0 means fail

*/
int32_t wifi_config_set_napt_entry_timer(uint32_t timer)
{
    mtk_tcpip_set_napt_clean_entry_timer(timer * 1000);

    return WIFI_SUCC;
}
#endif /* #if IP_NAPT */

#if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1)
/**
* @brief Set arp offload
*
* @Param enable: 0:disable, 1:enable
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_set_arp_offload(uint8_t enable)
{
    struct GLUE_INFO *prGlueInfo;
    uint8_t fgEnable;
    uint32_t u4Idx = 0;
    struct net_device *prDev = NULL;

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    if (enable == 1)
        fgEnable = TRUE;
    else if (enable == 0)
        fgEnable = FALSE;
    else {
        LOG_FUNC("wrong param: 0:disable, 1:enable");
        return WIFI_FAIL;
    }

    prGlueInfo->prAdapter->rWifiVar.ucArpOffload = fgEnable;

    DBGLOG(REQ, INFO, "%s: Set Arp offload [%u] [%u]\n", __func__,
           fgEnable, prGlueInfo->fgIsInSuspendMode);

    for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
        prDev = wlanGetNetDev(prGlueInfo, u4Idx);
        if (!prDev)
            continue;

        kalSetNetAddressFromInterface(prGlueInfo, prDev, fgEnable);
    }
    return WIFI_SUCC;

}

/**
* @brief Set arp offload
*
* @Param enable: 0:disable, 1:enable
*
* @return  >=0 means success, <0 means fail
*/
int32_t wifi_config_get_arp_offload(uint8_t *enable)
{
    struct GLUE_INFO *prGlueInfo;

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    if (enable != NULL)
        *enable = prGlueInfo->prAdapter->rWifiVar.ucArpOffload;

    return WIFI_SUCC;
}

#endif /* #if (CFG_SUPPORT_ARP_OFFLOAD_CMD == 1) */

int32_t wifi_config_set_wow(uint8_t enable)
{
    struct GLUE_INFO *prGlueInfo;
    uint8_t current_mode = 0;
    uint8_t idx, i;
    struct STA_RECORD *prStaRec;
    struct RX_BA_ENTRY *prRxBaEntry;
    struct netif *prNetif = &sta_netif;
    struct dhcp *prDhcp;

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    /* not support wow under AP + STA con-current.
     * after enable WOW, STA traffic will be block.
     * so wow under AP + STA con-current is meaningless
     */
    wifi_config_get_opmode_internal(&current_mode);
    if (current_mode != WIFI_MODE_STA_ONLY) {
        DBGLOG(REQ, ERROR, "Not support WOW when NAPT enable\n");
        return WIFI_ERR_NOT_SUPPORT;
    }

    if (enable == 0) {
        disableFWOffloadLeaveWow(prGlueInfo);
        kalWowProcess(prGlueInfo, FALSE);
        prDhcp = netif_dhcp_data(prNetif);
        if ((prDhcp != NULL) && ip_addr_isany_val(prNetif->ip_addr))
            dhcp_start(prNetif);
    } else {
        setRekeyOffloadEnterWow(prGlueInfo);
        kalWowProcess(prGlueInfo, TRUE);

        for (idx = 0; idx < CFG_STA_REC_NUM; idx++) {
            prStaRec = cnmGetStaRecByIndex(prGlueInfo->prAdapter, idx);
            if (!prStaRec)
                continue;
            for (i = 0; i < CFG_RX_MAX_BA_TID_NUM; i++) {
                prRxBaEntry = prStaRec->aprRxReorderParamRefTbl[i];
                if (!prRxBaEntry || !(prRxBaEntry->fgIsValid))
                    continue;

                prRxBaEntry->fgFirstSnToWinStart = TRUE;
            }
        }
    }

    DBGLOG(REQ, INFO, "%s: Set wow mode [%u]\n", __func__,
           prGlueInfo->prAdapter->fgIsWowSuspend);

    return WIFI_SUCC;

}

int32_t wifi_config_get_wow(uint8_t *mode)
{
    struct GLUE_INFO *prGlueInfo;

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    *mode = prGlueInfo->prAdapter->fgIsWowSuspend;

    return WIFI_SUCC;

}

int32_t wifi_config_get_wow_reason(uint8_t *reason)
{
    struct GLUE_INFO *prGlueInfo;

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    *reason = prGlueInfo->prAdapter->rWowCtrl.ucReason;
    /*
     * 0:  MAGIC PACKET
     * 8:  DISCONNECT
     * 9:  IPV4_UDP PACKET
     * 10: IPV4_TCP PACKET
     * 13: BEACON LOST
     * 20: ROAMING_EVENT
     */

    return WIFI_SUCC;

}

static
int32_t wifi_update_wow_Ipv4_Port(
    uint8_t *pu1Len,
    uint16_t aucPorts[],
    uint8_t op)
{
    int32_t i4Ret = WIFI_SUCC;
    uint8_t u1IpProto = ((op & WOW_PORT_IPPROTO_MASK) >>
                         WOW_PORT_IPPROTO_SHFT);
    uint8_t u1op = ((op & WOW_PORT_OP_MASK) >> WOW_PORT_OP_SHFT);
    uint8_t i;
    struct GLUE_INFO *prGlueInfo;
    struct WOW_PORT *prWowPort;
    uint16_t *pau2PortArry = NULL;
    uint8_t *pu1ArryLen = 0;
    uint16_t u2NewNumTotal = 0;

    prGlueInfo = &g_rGlueInfo;
    if (prGlueInfo == NULL || pu1Len == NULL || aucPorts == NULL) {
        DBGLOG(INIT, ERROR, "prGlueInfo in NULL!\n");
        i4Ret = WIFI_FAIL;
        goto END;
    }

    prWowPort = &prGlueInfo->prAdapter->rWowCtrl.stWowPort;

    if (u1IpProto == WOW_PORT_IPPROTO_UDP) {
        /* update udp port */
        pau2PortArry = prWowPort->ausIPv4UdpPort;
        pu1ArryLen = &prWowPort->ucIPv4UdpPortCnt;
        u2NewNumTotal = *pu1Len + prWowPort->ucIPv4TcpPortCnt;
    } else if (u1IpProto == WOW_PORT_IPPROTO_TCP) {
        /* update tcp port */
        pau2PortArry = prWowPort->ausIPv4TcpPort;
        pu1ArryLen = &prWowPort->ucIPv4TcpPortCnt;
        u2NewNumTotal = *pu1Len + prWowPort->ucIPv4UdpPortCnt;
    } else {
        i4Ret = WLAN_STATUS_INVALID_DATA;
        goto END;
    }

    /* <1> get */
    if (u1op == WOW_PORT_OP_GET) {
        if (*pu1Len < *pu1ArryLen) {
            i4Ret = WIFI_FAIL;
            goto END;
        }

        *pu1Len = *pu1ArryLen;
        for (i = 0; i < *pu1ArryLen; i++)
            aucPorts[i] = pau2PortArry[i];

        goto END;
    }

    /* <2> set */
    if (u1op == WOW_PORT_OP_SET) {
        if (*pu1Len < 1 ||
            *pu1Len > MAX_TCP_UDP_PORT ||
            u2NewNumTotal > MAX_TCP_UDP_PORT) {
            i4Ret = WIFI_FAIL;
            goto END;
        }

        *pu1ArryLen = *pu1Len;
        for (i = 0; i < *pu1Len; i++)
            pau2PortArry[i] = aucPorts[i];
    }

    /* <3> unset */
    if (u1op == WOW_PORT_OP_UNSET) {
        *pu1ArryLen = 0;
    }

END:
    return i4Ret;
}

int32_t wifi_config_wow_port(struct wifi_wow_ports_t *prWowPorts, uint8_t op)
{
    uint8_t *pu1Len;
    uint16_t *pu2Ports;

    if (prWowPorts != NULL) {
        pu1Len = &(prWowPorts->len);
        pu2Ports = prWowPorts->ports;
        return wifi_update_wow_Ipv4_Port(pu1Len, pu2Ports, op);
    } else
        return WIFI_ERR_PARA_INVALID;
}

#if CFG_SUPPORT_802_11K
int32_t wifi_neighbor_rep_request(uint8_t *ssid, uint8_t ssid_length)
{
    struct GLUE_INFO *prGlueInfo;
    uint32_t rStatus;
    uint32_t u4BufLen = 0;

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    if (ssid_length > IEEE80211_MAX_SSID_LEN) {
        DBGLOG(REQ, ERROR, "ERR: invalid ssid len (%d)", ssid_length);
        return WIFI_FAIL;
    }

    if (ssid == NULL)
        rStatus = kalIoctl(prGlueInfo, wlanoidSendNeighborRequest,
                           NULL, 0, FALSE, FALSE, TRUE, &u4BufLen);
    else
        rStatus = kalIoctl(prGlueInfo, wlanoidSendNeighborRequest,
                           ssid, ssid_length, FALSE, FALSE,
                           TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)", rStatus);
        return WIFI_FAIL;
    }

    return WIFI_SUCC;
}
#endif /* #if CFG_SUPPORT_802_11K */

#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
int32_t wifi_wnm_bss_query(uint8_t value)
{
    struct GLUE_INFO *prGlueInfo;
    uint32_t u4BufLen = 0;
    uint32_t rStatus;
    char pucQueryReason[4] = {};

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    if (snprintf(pucQueryReason, 4, "%u", value) < 0) {
        DBGLOG(REQ, ERROR, "ERR: snprint fail");
        return WIFI_FAIL;
    }

    rStatus = kalIoctl(prGlueInfo, wlanoidSendBTMQuery,
                       (void *)pucQueryReason, strlen(pucQueryReason),
                       FALSE, FALSE, TRUE, &u4BufLen);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "ERR: kalIoctl fail (%d)", rStatus);
        return WIFI_FAIL;
    }

    return WIFI_SUCC;
}
#endif /* #if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT */

#if CFG_SUPPORT_ROAMING_CUSTOMIZED
static int32_t wifi_config_set_roam_enable(uint32_t setRoaming)
{
    struct ADAPTER *prAdapter;
    uint32_t u4BufLen = 0;

    prAdapter = &g_rAdapter;

    return kalIoctl(prAdapter->prGlueInfo,
                    wlanoidSetDrvRoamingPolicy,
                    &setRoaming, sizeof(uint32_t), FALSE, FALSE, TRUE,
                    &u4BufLen);
}

int32_t wifi_config_set_roam_delta(uint8_t port, uint8_t value)

{
    struct ADAPTER *prAdapter;

    prAdapter = &g_rAdapter;

    if (!wifi_is_port_valid(port))
        return WIFI_ERR_PARA_INVALID;

    if (port != WIFI_PORT_STA) {
        LOG_FUNC("Not support mode(%d) %d\n", port);
        return WIFI_FAIL;
    }

    prAdapter->rWifiVar.ucRoamingDelta = value;

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return WIFI_SUCC;
}

int32_t wifi_config_get_roam_delta(uint8_t port, uint8_t *value)

{
    struct ADAPTER *prAdapter;

    if (!wifi_is_port_valid(port))
        return WIFI_ERR_PARA_INVALID;

    if (port != WIFI_PORT_STA) {
        LOG_FUNC("Not support mode(%d) %d\n", port);
        return WIFI_FAIL;
    }

    prAdapter = &g_rAdapter;
    *value = prAdapter->rWifiVar.ucRoamingDelta;

    return WIFI_SUCC;
}

int32_t wifi_config_set_roam_scan_channel(uint8_t port,
                                          uint8_t value)

{
    struct ADAPTER *prAdapter;

    prAdapter = &g_rAdapter;

    if (!wifi_is_port_valid(port))
        return WIFI_ERR_PARA_INVALID;

    if (port != WIFI_PORT_STA) {
        LOG_FUNC("Not support mode(%d) %d\n", port);
        return WIFI_FAIL;
    }

    prAdapter->rWifiVar.fgRoamingScanFullCh = value;

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return WIFI_SUCC;
}

int32_t wifi_config_get_roam_scan_channel(uint8_t port,
                                          uint8_t *value)

{
    struct ADAPTER *prAdapter;

    if (!wifi_is_port_valid(port))
        return WIFI_ERR_PARA_INVALID;

    if (port != WIFI_PORT_STA) {
        LOG_FUNC("Not support mode(%d) %d\n", port);
        return WIFI_FAIL;
    }

    prAdapter = &g_rAdapter;
    *value = prAdapter->rWifiVar.fgRoamingScanFullCh;

    return WIFI_SUCC;
}

static int32_t wifi_config_set_roam_chip_cfg(
    const char *cmd,
    int32_t value
)
{
    struct ADAPTER *prAdapter = &g_rAdapter;
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4TotalLen = CHIP_CONFIG_RESP_SIZE;
    uint32_t u4CmdLen = 0;
    char pcCommand[CHIP_CONFIG_RESP_SIZE] = {"set_chip "};
    const char *prMainCmd = cmd;

    kalSprintf((pcCommand + CMD_ROAM_PREFIX_LEN), "%s ", prMainCmd);
    u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);

    kalSprintf((pcCommand + u4CmdLen), "%d", value);
    u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));
    {
        rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
        rChipConfigInfo.u2MsgSize = u4CmdLen - CMD_ROAM_PREFIX_LEN;
        kalStrnCpy((char *)rChipConfigInfo.aucCmd,
                   pcCommand + CMD_ROAM_PREFIX_LEN,
                   CHIP_CONFIG_RESP_SIZE - 1);
        rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

        rStatus = kalIoctl(prAdapter->prGlueInfo, wlanoidSetChipConfig,
                           &rChipConfigInfo, sizeof(rChipConfigInfo),
                           FALSE, FALSE, TRUE, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS)
            DBGLOG(REQ, LOUD, "%s: kalIoctl ret=%d\n", __func__,
                   rStatus);
    }

    return rStatus;

}

static int32_t wifi_config_get_roam_chip_cfg(
    const char *cmd,
    int32_t *value
)
{
    int32_t i4BytesWritten = 0;
    uint32_t u4BufLen = 0;
    uint32_t u2MsgSize = 0;
    uint32_t u4CmdLen = 0;
    uint32_t u4MainCmdLen = 0;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    int32_t i4TotalLen = CHIP_CONFIG_RESP_SIZE;
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
    struct ADAPTER *prAdapter = &g_rAdapter;
    char pcCommand[CHIP_CONFIG_RESP_SIZE] = {"get_chip "};
    const char *prMainCmd = cmd;
    int32_t i4Value = 0;

    u4MainCmdLen = kalStrnLen(prMainCmd, i4TotalLen);
    kalSprintf((pcCommand + CMD_ROAM_PREFIX_LEN), "%s", prMainCmd);
    u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);

    DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

    u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);

    kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));
    {
        rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
        rChipConfigInfo.u2MsgSize = u4CmdLen - CMD_ROAM_PREFIX_LEN;
        kalStrnCpy((char *)rChipConfigInfo.aucCmd, pcCommand + CMD_ROAM_PREFIX_LEN,
                   CHIP_CONFIG_RESP_SIZE - 1);
        rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';
        rStatus = kalIoctl(prAdapter->prGlueInfo, wlanoidQueryChipConfig,
                           &rChipConfigInfo, sizeof(rChipConfigInfo),
                           TRUE, TRUE, TRUE, &u4BufLen);

        if (rStatus != WLAN_STATUS_SUCCESS) {
            DBGLOG(REQ, ERROR, "%s: kalIoctl ret=%d\n", __func__,
                   rStatus);
            return -1;
        }

        /* Check respType */
        u2MsgSize = rChipConfigInfo.u2MsgSize;
        DBGLOG(REQ, LOUD, "%s: RespTyep  %u\n", __func__,
               rChipConfigInfo.ucRespType);
        DBGLOG(REQ, LOUD, "%s: u2MsgSize %u\n", __func__,
               rChipConfigInfo.u2MsgSize);

        if (u2MsgSize > sizeof(rChipConfigInfo.aucCmd)) {
            DBGLOG(REQ, ERROR, "%s: u2MsgSize error ret=%u\n",
                   __func__, rChipConfigInfo.u2MsgSize);
            return -1;
        }

        if (u2MsgSize > 0) {
            if (rChipConfigInfo.ucRespType ==
                CHIP_CONFIG_TYPE_ASCII) {
                i4BytesWritten =
                    snprintf(pcCommand + i4BytesWritten,
                             i4TotalLen, "%s",
                             rChipConfigInfo.aucCmd);
                i4Value = atoi(pcCommand + u4MainCmdLen + 3);
                *value = i4Value;
                DBGLOG(REQ, LOUD, "%s: %s %d\n",
                       __func__, pcCommand, i4Value);
            }
            return 0;
        }
    }
    return -1;
}

int32_t wifi_config_set_roam_by_rssi(
    uint8_t port,
    int32_t value
)
{
    int32_t ret = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    ret = wifi_config_set_roam_chip_cfg(CMD_ROAM_BY_RSSI, value);

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return ret;
}
int32_t wifi_config_get_roam_by_rssi(
    uint8_t port,
    int32_t *value
)
{
    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    return wifi_config_get_roam_chip_cfg(CMD_ROAM_BY_RSSI, value);
}


int32_t wifi_config_set_roam_rssithreshold(uint8_t port,
                                           int32_t value
                                          )
{
    int32_t ret = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    value = dBm_TO_RCPI(value);
    ret = wifi_config_set_roam_chip_cfg(CMD_ROAM_RSSI_THRESHOLD, value);

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return ret;
}
int32_t wifi_config_get_roam_rssithreshold(uint8_t port,
                                           int32_t *value
                                          )
{
    int32_t ret = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    ret = wifi_config_get_roam_chip_cfg(CMD_ROAM_RSSI_THRESHOLD, value);
    *value = RCPI_TO_dBm(*value);

    return ret;
}

int32_t wifi_config_set_roam_by_bcnmiss(uint8_t port,
                                        int32_t value
                                       )
{
    int32_t ret = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    ret = wifi_config_set_roam_chip_cfg(CMD_ROAM_BY_BCN_MISS, value);

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return ret;
}
int32_t wifi_config_get_roam_by_bcnmiss(uint8_t port,
                                        int32_t *value
                                       )
{
    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    return wifi_config_get_roam_chip_cfg(CMD_ROAM_BY_BCN_MISS, value);
}

int32_t wifi_config_set_roam_by_bcnmissthreshold(uint8_t port,
                                                 int32_t value
                                                )
{
    int32_t ret = 0;
    uint8_t interval;
    uint8_t link_status = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    g_roam_by_bcnmiss_threshold = value;

    wifi_connection_get_link_status(&link_status);

    if (wifi_config_get_listen_interval(&interval) != WIFI_SUCC) {
        if (link_status == WIFI_STATUS_LINK_CONNECTED) {
            LOG_FUNC("Set bcnmissthreshold failed, interval=%d.\n",
                     interval);
            return WIFI_FAIL;
        } else {
            LOG_FUNC("Update bcnmissthreshold after link connected.\n");
            return ret;
        }
    }

    /* value is in unit of Beacon interval,
     * and HW Bcn timeout cnt is in unit of listen interval
     * so have to do unit conversion here
     */
    value = value / interval +
            (value % interval ? 1 : 0);
    ret = wifi_config_set_roam_chip_cfg(CMD_ROAM_BY_BCN_MISS_THR, value);

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return ret;
}
int32_t wifi_config_get_roam_by_bcnmissthreshold(uint8_t port,
                                                 int32_t *value
                                                )
{
    int32_t ret = 0;
    uint8_t interval = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    if (g_roam_by_bcnmiss_threshold != 0) {
        *value = g_roam_by_bcnmiss_threshold;
    } else {
        ret = wifi_config_get_roam_chip_cfg(CMD_ROAM_BY_BCN_MISS_THR, value);
        if (ret != WIFI_SUCC)
            return ret;
        ret = wifi_config_get_listen_interval(&interval);
        if (ret != WIFI_SUCC)
            return ret;
        g_roam_by_bcnmiss_threshold = *value * interval;
        *value = g_roam_by_bcnmiss_threshold;
    }

    return ret;
}

int32_t wifi_config_set_roam_block_time(uint8_t port,
                                        int32_t value
                                       )
{
    int32_t ret = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    ret = wifi_config_set_roam_chip_cfg(CMD_ROAM_BLOCK_TIME, value);

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return ret;
}
int32_t wifi_config_get_roam_block_time(uint8_t port,
                                        int32_t *value
                                       )
{
    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    return wifi_config_get_roam_chip_cfg(CMD_ROAM_BLOCK_TIME, value);
}

int32_t wifi_config_set_roam_lock_time(uint8_t port,
                                       int32_t value
                                      )
{
    int32_t ret = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    ret = wifi_config_set_roam_chip_cfg(CMD_ROAM_LOCK_TIME, value);

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return ret;
}
int32_t wifi_config_get_roam_lock_time(uint8_t port,
                                       int32_t *value
                                      )
{
    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    return wifi_config_get_roam_chip_cfg(CMD_ROAM_LOCK_TIME, value);
}

int32_t wifi_config_set_roam_maxlock_count(uint8_t port,
                                           int32_t value
                                          )
{
    int32_t ret = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    ret = wifi_config_set_roam_chip_cfg(CMD_ROAM_MAXLOCK_TIME, value);

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return ret;
}
int32_t wifi_config_get_roam_maxlock_count(uint8_t port,
                                           int32_t *value
                                          )
{
    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    return wifi_config_get_roam_chip_cfg(CMD_ROAM_MAXLOCK_TIME, value);
}

int32_t wifi_config_set_bto_time(uint8_t port,
                                 int32_t value
                                )
{
    int32_t ret = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    ret = wifi_config_set_roam_chip_cfg(CMD_ROAM_BTO_TIME, value);

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return ret;
}
int32_t wifi_config_get_bto_time(uint8_t port,
                                 int32_t *value
                                )
{
    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    return wifi_config_get_roam_chip_cfg(CMD_ROAM_BTO_TIME, value);
}

int32_t wifi_config_set_stable_time(uint8_t port,
                                    int32_t value
                                   )
{
    int32_t ret = 0;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    ret = wifi_config_set_roam_chip_cfg(CMD_ROAM_STABLE_TIME, value);

    /* restart roaming */
    wifi_config_set_roam_enable(0);
    wifi_config_set_roam_enable(1);

    return ret;
}
int32_t wifi_config_get_stable_time(uint8_t port,
                                    int32_t *value
                                   )
{
    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    return wifi_config_get_roam_chip_cfg(CMD_ROAM_STABLE_TIME, value);
}

int32_t wifi_config_set_split_scan(uint8_t port,
                                   int32_t value
                                  )
{
    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    return wifi_config_set_roam_chip_cfg(CMD_ROAM_SPLIT_SCAN, value);
}

int32_t wifi_config_set_autoroam(uint8_t port,
                                 int32_t value
                                )
{
    struct ADAPTER *prAdapter = g_prAdpater;
    struct ROAMING_INFO *prRoamingFsmInfo;
    struct CONNECTION_SETTINGS *prConnSettings;
    uint32_t setRoaming;
    uint8_t ucBssIndex = AIS_DEFAULT_INDEX;
    struct BSS_INFO *prBssInfo = NULL;
    uint32_t u4SetInfoLen;

    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    prBssInfo = aisGetConnectedBssInfo(prAdapter);
    if (prBssInfo != NULL)
        ucBssIndex = prBssInfo->ucBssIndex;

    prRoamingFsmInfo =
        aisGetRoamingInfo(prAdapter, ucBssIndex);
    prConnSettings =
        aisGetConnSettings(prAdapter, ucBssIndex);

    if (value == 0) {
        prAdapter->rWifiVar.fgDisRoaming = TRUE;
        prConnSettings->fgIsEnableRoaming = FALSE;
        setRoaming = 0;
    } else {
        prAdapter->rWifiVar.fgDisRoaming = FALSE;
        prConnSettings->fgIsEnableRoaming = TRUE;
        setRoaming = 1;
    }

    prRoamingFsmInfo->fgIsEnableRoaming =
        prConnSettings->fgIsEnableRoaming;

    wifi_config_set_roam_by_rssi(port, setRoaming);
    wifi_config_set_roam_enable(setRoaming);
    if (setRoaming == 0) {
        kalIoctlByBssIdx(prAdapter->prGlueInfo,
                         wlanoidAbortScan,
                         NULL, 1, FALSE, FALSE, TRUE, &u4SetInfoLen,
                         ucBssIndex);
    }

    return 0;

}
int32_t wifi_config_get_autoroam(uint8_t port,
                                 int32_t *value
                                )
{
    if (!g_prGlueInfo || !g_prAdpater)
        return -1;

    *value = !(g_prAdpater->rWifiVar.fgDisRoaming);
    return 0;
}

int32_t wifi_config_get_roam_type(uint8_t port,
                                  uint8_t *value)
{
    int32_t roam_by_rssi = 0;
    int32_t roam_by_bcnmiss = 0;

    wifi_config_get_roam_by_rssi(port, &roam_by_rssi);
    wifi_config_get_roam_by_bcnmiss(port, &roam_by_bcnmiss);

    *value =
        (roam_by_rssi ? BIT(ROAM_TYPE_BY_RSSI) : 0) |
        (roam_by_bcnmiss ? BIT(ROAM_TYPE_BY_BCNMISS) : 0);

    return WIFI_SUCC;
}

int32_t wifi_config_is_connect_ft_ap(uint8_t port,
                                     int *value)
{
#ifdef MTK_MINISUPP_ENABLE
    *value = mtk_supplicant_is_connect_ft_ap();
#endif /* #ifdef MTK_MINISUPP_ENABLE */
    return WIFI_SUCC;
}

int32_t wifi_config_get_roam_statistic(uint8_t port,
                                       struct roam_statistic_t *roam_stat)
{
    uint32_t u4Uknown = 0;
    struct ADAPTER *prAdapter = g_prAdpater;
    uint8_t ucBssIndex = AIS_DEFAULT_INDEX;
    struct BSS_INFO *prBssInfo = NULL;

    if (!prAdapter)
        return -1;

    prBssInfo = aisGetConnectedBssInfo(prAdapter);
    if (prBssInfo != NULL)
        ucBssIndex = prBssInfo->ucBssIndex;

    roamingFsmQueryStatic(
        prAdapter,
        ucBssIndex,
        &roam_stat->num_rssi_roam,
        &roam_stat->num_bcnMiss_roam,
        &u4Uknown);

    if (u4Uknown != 0)
        DBGLOG(REQ, WARN, "%s: RSSI[%d] BTO[%d] UNKNOWN[%d]\n", __func__,
               roam_stat->num_rssi_roam,
               roam_stat->num_bcnMiss_roam,
               u4Uknown);

    return WIFI_SUCC;
}

int32_t wifi_config_clear_roam_statistic(uint8_t port)
{
    struct ADAPTER *prAdapter = g_prAdpater;
    uint8_t ucBssIndex = AIS_DEFAULT_INDEX;
    struct BSS_INFO *prBssInfo = NULL;

    if (!prAdapter)
        return -1;

    prBssInfo = aisGetConnectedBssInfo(prAdapter);
    if (prBssInfo != NULL)
        ucBssIndex = prBssInfo->ucBssIndex;

    roamingFsmClearStatic(prAdapter, ucBssIndex);
    return WIFI_SUCC;
}

#endif /* #if CFG_SUPPORT_ROAMING_CUSTOMIZED */

int32_t wifi_config_update_tx_policy(uint8_t u1TxPolicy, uint8_t u1TxSkbQLen)
{
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;

    if (u1TxPolicy != 0xff)
        prGlueInfo->prAdapter->rWifiVar.u1TxPolicy = u1TxPolicy;
    if (u1TxSkbQLen != 0xff)
        prGlueInfo->prAdapter->rWifiVar.u1TxSkbQLen = u1TxSkbQLen;

    if (u1TxPolicy == 0xff && u1TxSkbQLen == 0xff)
        return -1;
    else
        return 0;
}

int32_t wifi_config_set_ser(uint8_t value)
{
    struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
    struct mt66xx_chip_info *prChipInfo;

    if (!prGlueInfo || !prGlueInfo->prAdapter)
        return -1;

    prChipInfo = prGlueInfo->prAdapter->chip_info;
    if (!prChipInfo)
        return -1;

    if (value == 0) {
        if (prChipInfo->triggerfwassert) {
            prChipInfo->triggerfwassert();
            return WIFI_SUCC;
        } else
            return WIFI_FAIL;
    } else if (value == 1) {
        if (prChipInfo->checkbushang) {
            prChipInfo->checkbushang(prGlueInfo->prAdapter, FALSE);
            return WIFI_SUCC;
        } else {
            return WIFI_FAIL;
        }
    } else if (value == 2) {
#if CFG_CHIP_RESET_SUPPORT
        GL_RESET_TRIGGER(prGlueInfo->prAdapter, 0);
#endif /* #if CFG_CHIP_RESET_SUPPORT */
    }

    return WIFI_FAIL;
}

#if CFG_SUPPORT_SCAN_CH_TIME
int32_t wifi_config_set_dwll_time(uint8_t scan_option,
                                  uint8_t dwell_time)
{
    int32_t ret = WLAN_STATUS_SUCCESS;

    if (scan_option == WIFI_PASSIVE_SCAN) {
        ret = wifi_config_set_roam_chip_cfg(CMD_SCAN_DFS_TIME,
                                            dwell_time);
    } else {
        g_prAdpater->rWifiVar.uChannelDwellTime = dwell_time;
    }

    return ret;
}

int32_t wifi_config_set_DFS_dwll_time(uint8_t scan_option)
{
    int32_t ret = WLAN_STATUS_SUCCESS;

    ret = wifi_config_set_roam_chip_cfg(CMD_SCAN_DFS_IN_ACTIVE,
                                        scan_option);

    return ret;
}
#endif /* #if CFG_SUPPORT_SCAN_CH_TIME */


#ifdef MTK_WIFI_PROFILE_ENABLE
int32_t wifi_profile_set_opmode(uint8_t mode)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    if (!wifi_is_opmode_valid(mode)) {
        LOG_FUNC("mode is invalid: %d\n", mode);
        return WIFI_ERR_PARA_INVALID;
    }

    if (mode > WIFI_MODE_REPEATER) {
        LOG_FUNC("The mode %d is not supported.\n", mode);
        return WIFI_ERR_PARA_INVALID;
    }

    kalSprintf(buf, "%d", mode);

    if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_COMMON, "OpMode",
                             NVDM_DATA_ITEM_TYPE_STRING, (uint8_t *)buf, kalStrLen(buf))
        != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_get_opmode(uint8_t *mode)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t len = sizeof(buf);

    if (mode == NULL) {
        LOG_FUNC("mode is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_COMMON, "OpMode",
                            (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    *mode = atoi(buf);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_ssid(uint8_t port, uint8_t *ssid, uint8_t ssid_length)
{
#if MTK_NVDM_ENABLE
    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (ssid_length > WIFI_MAX_LENGTH_OF_SSID) {
        LOG_FUNC("incorrect length(=%d)\n", ssid_length);
        return WIFI_ERR_PARA_INVALID;
    }
    if (ssid == NULL) {
        LOG_FUNC("ssid is null.\n");
        return WIFI_ERR_PARA_INVALID;
    }

    char ssid_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    char ssid_len_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    kalMemCopy(ssid_buf, ssid, ssid_length);
    ssid_buf[ssid_length] = '\0';

    kalSprintf(ssid_len_buf, "%d", ssid_length);
    if (port == WIFI_PORT_AP) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_HAPD, "ssid",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)ssid_buf, kalStrLen(ssid_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "SsidLen",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)ssid_len_buf, kalStrLen(ssid_len_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "Ssid",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)ssid_buf, kalStrLen(ssid_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "SsidLen",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)ssid_len_buf, kalStrLen(ssid_len_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    }
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}



int32_t wifi_profile_get_ssid(uint8_t port, uint8_t *ssid, uint8_t *ssid_length)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH];
    uint32_t len;

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (ssid_length == NULL) {
        LOG_FUNC("ssid_length is null.\n");
        return WIFI_ERR_PARA_INVALID;
    }
    if (ssid == NULL) {
        LOG_FUNC("ssid is null.\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (port == WIFI_PORT_AP) {
        kalMemSet(buf, 0, WIFI_PROFILE_BUFFER_LENGTH);
        len = sizeof(buf);
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_HAPD, "ssid",
                                (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        kalStrCpy((char *)ssid, buf);

        kalMemSet(buf, 0, WIFI_PROFILE_BUFFER_LENGTH);
        len = sizeof(buf);
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "SsidLen",
                                (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        *ssid_length = atoi(buf);
    } else {
        kalMemSet(buf, 0, WIFI_PROFILE_BUFFER_LENGTH);
        len = sizeof(buf);
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "Ssid",
                                (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        kalStrCpy((char *)ssid, buf);

        kalMemSet(buf, 0, WIFI_PROFILE_BUFFER_LENGTH);
        len = sizeof(buf);
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "SsidLen",
                                (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        *ssid_length = atoi(buf);
    }
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_wireless_mode(uint8_t port, wifi_phy_mode_t mode)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }

    kalSprintf(buf, "%d", mode);

    if (port == WIFI_PORT_AP) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "WirelessMode",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)buf, kalStrLen(buf)) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "WirelessMode",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)buf, kalStrLen(buf)) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    }

    mode = wifi_wireless_mode_mapping(1, mode);
    if (port == WIFI_PORT_STA) {
        if ((mode & WIFI_MODE_80211N) == 0x1)
            nvdm_write_data_item(WIFI_PROFILE_BUFFER_WIFI, "StaHT",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)"1", 1);
        else
            nvdm_write_data_item(WIFI_PROFILE_BUFFER_WIFI, "StaHT",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)"0", 1);
        if ((mode & WIFI_MODE_80211AC) == 0x2)
            nvdm_write_data_item(WIFI_PROFILE_BUFFER_WIFI, "StaVHT",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)"1", 1);
        else
            nvdm_write_data_item(WIFI_PROFILE_BUFFER_WIFI, "StaVHT",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)"0", 1);
        if ((mode & WIFI_MODE_80211AX) == 0x4)
            nvdm_write_data_item(WIFI_PROFILE_BUFFER_WIFI, "StaHE",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)"1", 1);
        else
            nvdm_write_data_item(WIFI_PROFILE_BUFFER_WIFI, "StaHE",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)"0", 1);
    } else {
        if ((mode & WIFI_MODE_80211N) == 0x1)
            nvdm_write_data_item(WIFI_PROFILE_BUFFER_WIFI, "ApHT",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)"1", 1);
        else
            nvdm_write_data_item(WIFI_PROFILE_BUFFER_WIFI, "ApHT",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)"0", 1);
        if ((mode & WIFI_MODE_80211AC) == 0x2)
            nvdm_write_data_item(WIFI_PROFILE_BUFFER_WIFI, "ApVHT",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)"1", 1);
        else
            nvdm_write_data_item(WIFI_PROFILE_BUFFER_WIFI, "ApVHT",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)"0", 1);
    }
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}


int32_t wifi_profile_get_wireless_mode(uint8_t port, wifi_phy_mode_t *mode)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t len = sizeof(buf);

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (mode == NULL) {
        LOG_FUNC("mode is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (port == WIFI_PORT_AP) {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "WirelessMode",
                                (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "WirelessMode",
                                (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    }

    *mode = (wifi_phy_mode_t)atoi(buf);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_security_mode(uint8_t port,
                                       wifi_auth_mode_t auth_mode, wifi_encrypt_type_t encrypt_type)
{
#if MTK_NVDM_ENABLE
    char auth_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    char encrypt_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (!wifi_is_auth_mode_valid(auth_mode)) {
        LOG_FUNC("auth_mode is invalid: %d\n", auth_mode);
        return WIFI_ERR_PARA_INVALID;
    }
    if (!wifi_is_encrypt_type_valid(encrypt_type)) {
        LOG_FUNC("encrypt_type is invalid: %d\n", encrypt_type);
        return WIFI_ERR_PARA_INVALID;
    }

    kalSprintf(auth_buf, "%d", auth_mode);
    kalSprintf(encrypt_buf, "%d", encrypt_type);

    if (port == WIFI_PORT_AP) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "AuthMode",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)auth_buf, kalStrLen(auth_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "EncrypType",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)encrypt_buf, kalStrLen(encrypt_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "AuthMode",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)auth_buf, kalStrLen(auth_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "EncrypType",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)encrypt_buf, kalStrLen(encrypt_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    }
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_get_security_mode(uint8_t port,
                                       wifi_auth_mode_t *auth_mode, wifi_encrypt_type_t *encrypt_type)
{
#if MTK_NVDM_ENABLE
    char auth_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    char encypt_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t auth_buf_len = sizeof(auth_buf);
    uint32_t encypt_buf_len = sizeof(encypt_buf);

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (auth_mode == NULL || encrypt_type == NULL) {
        LOG_FUNC("null input pointer\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (port == WIFI_PORT_AP) {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "AuthMode",
                                (uint8_t *)auth_buf, &auth_buf_len)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "EncrypType",
                                (uint8_t *)encypt_buf, &encypt_buf_len)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "AuthMode",
                                (uint8_t *)auth_buf, &auth_buf_len)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "EncrypType",
                                (uint8_t *)encypt_buf, &encypt_buf_len)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    }

    *auth_mode = (wifi_auth_mode_t)atoi(auth_buf);
    *encrypt_type = (wifi_encrypt_type_t)atoi(encypt_buf);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_wpa_psk_key(uint8_t port,
                                     uint8_t *passphrase, uint8_t passphrase_length)
{
#if MTK_NVDM_ENABLE
    char pass_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    char pass_len_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (passphrase == NULL) {
        LOG_FUNC("passphrase is null.\n");
        return WIFI_ERR_PARA_INVALID;
    }
    if ((passphrase_length < 8) ||
        (passphrase_length > WIFI_LENGTH_PASSPHRASE)) {
        LOG_FUNC("incorrect length(=%d)\n", passphrase_length);
        return WIFI_ERR_PARA_INVALID;
    }
    if (passphrase_length == WIFI_LENGTH_PASSPHRASE) {
        for (uint8_t index = 0; index < WIFI_LENGTH_PASSPHRASE;
             index++) {
            if (!hex_isdigit(passphrase[index])) {
                LOG_FUNC("length(=%d) but the strings"
                         " are not hex strings!\n",
                         passphrase_length);
                return WIFI_ERR_PARA_INVALID;
            }
        }
    }

    kalSprintf(pass_len_buf, "%d", passphrase_length);
    kalMemCopy(pass_buf, passphrase, passphrase_length);
    pass_buf[passphrase_length] = '\0';

    if (port == WIFI_PORT_AP) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "WpaPskLen",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)pass_len_buf, kalStrLen(pass_len_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "WpaPsk",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)pass_buf, kalStrLen(pass_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "WpaPskLen",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)pass_len_buf, kalStrLen(pass_len_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "WpaPsk",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)pass_buf, kalStrLen(pass_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    }
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}


int32_t wifi_profile_get_wpa_psk_key(uint8_t port,
                                     uint8_t *passphrase, uint8_t *passphrase_length)
{
#if MTK_NVDM_ENABLE
    char pass_len_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    char pass_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t pass_len_buf_size = sizeof(pass_len_buf);
    uint32_t pass_buf_size = sizeof(pass_buf);

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (passphrase == NULL || passphrase_length == NULL) {
        LOG_FUNC("null input pointer\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (port == WIFI_PORT_AP) {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "WpaPskLen",
                                (uint8_t *)pass_len_buf, &pass_len_buf_size)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "WpaPsk",
                                (uint8_t *)pass_buf, &pass_buf_size)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "WpaPskLen",
                                (uint8_t *)pass_len_buf, &pass_len_buf_size)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "WpaPsk",
                                (uint8_t *)pass_buf, &pass_buf_size)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    }

    if ((kalStrLen(pass_len_buf) == 0) || (kalStrLen(pass_buf) == 0)) {
        *passphrase_length = 0;
        passphrase[0] = '\0';
        return WIFI_SUCC;
    }

    *passphrase_length = atoi(pass_len_buf);
    if (*passphrase_length > WIFI_LENGTH_PASSPHRASE) {
        LOG_FUNC("passphrase_length is too big: %d\n",
                 *passphrase_length);
        return WIFI_FAIL;
    }

    kalMemCopy(passphrase, pass_buf, *passphrase_length);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_wep_key(uint8_t port, wifi_wep_key_t *wep_keys)
{
#if MTK_NVDM_ENABLE
    char key_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    char key_len_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    char def_key_id_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    char temp_str[4];
    char temp_str1[WIFI_MAX_WEP_KEY_LENGTH + 2];
    char *key_len_ptr = key_len_buf;
    char *key_ptr = key_buf;

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (wep_keys == NULL) {
        LOG_FUNC("wep_keys is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    for (uint8_t index = 0; index < WIFI_NUMBER_WEP_KEYS; index++) {
        if (index < WIFI_NUMBER_WEP_KEYS - 1) {
            kalSprintf(temp_str, "%d,",
                       wep_keys->wep_key_length[index]);
            kalMemCopy(temp_str1, wep_keys->wep_key[index],
                       wep_keys->wep_key_length[index]);
            temp_str1[wep_keys->wep_key_length[index]] = ',';
            temp_str1[wep_keys->wep_key_length[index] + 1] = '\0';
        } else {
            kalSprintf(temp_str, "%d",
                       wep_keys->wep_key_length[index]);
            kalMemCopy(temp_str1, wep_keys->wep_key[index],
                       wep_keys->wep_key_length[index]);
            temp_str1[wep_keys->wep_key_length[index]] = '\0';
        }
        kalStrCpy(key_len_ptr, temp_str);
        key_len_ptr += kalStrLen(temp_str);
        kalStrCpy(key_ptr, temp_str1);
        key_ptr += kalStrLen(temp_str1);
        kalMemSet(temp_str, 0, 4);
        kalMemSet(temp_str1, 0, WIFI_MAX_WEP_KEY_LENGTH + 2);
    }

    kalSprintf(def_key_id_buf, "%d", wep_keys->wep_tx_key_index);
    LOG_FUNC("wifi_profile_set_wep_key: SharedKey =%s,"
             " SharedKeyLen=%s, DefaultKeyId=%s\n",
             key_buf, key_len_buf, temp_str);

    if (port == WIFI_PORT_AP) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "SharedKey",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)key_buf, kalStrLen(key_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "SharedKeyLen",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)key_len_buf, kalStrLen(key_len_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "DefaultKeyId",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)def_key_id_buf, kalStrLen(def_key_id_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "SharedKey",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)key_buf, kalStrLen(key_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA,
                                 "SharedKeyLen", NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)key_len_buf, kalStrLen(key_len_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA,
                                 "DefaultKeyId", NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)def_key_id_buf, kalStrLen(def_key_id_buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    }
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_get_wep_key(uint8_t port, wifi_wep_key_t *wep_keys)
{
#if MTK_NVDM_ENABLE
    char key_len_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t key_len_buf_size = sizeof(key_len_buf);

    uint8_t index = 0;
    char *ptr = NULL;

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (wep_keys == NULL) {
        LOG_FUNC("wep_keys is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    /* WEP KEY LEN */
    if (port == WIFI_PORT_AP) {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "SharedKeyLen",
                                (uint8_t *)key_len_buf, &key_len_buf_size) !=
            NVDM_STATUS_OK)
            return WIFI_FAIL;
    } else {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "SharedKeyLen",
                                (uint8_t *)key_len_buf, &key_len_buf_size) !=
            NVDM_STATUS_OK)
            return WIFI_FAIL;
    }

    for (index = 0, ptr = rstrtok((char *)key_len_buf, ",");
         (ptr); ptr = rstrtok(NULL, ",")) {
        wep_keys->wep_key_length[index] = atoi(ptr);
        index++;
        if (index >= WIFI_NUMBER_WEP_KEYS)
            break;
    }

    /* WEP KEY */
    char key_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t key_buf_len = sizeof(key_buf);

    if (port == WIFI_PORT_AP) {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "SharedKey",
                                (uint8_t *)key_buf, &key_buf_len)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "SharedKey",
                                (uint8_t *)key_buf, &key_buf_len)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    }

    for (index = 0, ptr = rstrtok(key_buf, ",");
         (ptr); ptr = rstrtok(NULL, ",")) {
        if (wep_keys->wep_key_length[index] == 5 ||
            wep_keys->wep_key_length[index] == 13) {
            kalMemCopy(&wep_keys->wep_key[index], ptr,
                       wep_keys->wep_key_length[index]);
        } else if (wep_keys->wep_key_length[index] == 10 ||
                   wep_keys->wep_key_length[index] == 26) {
            wep_keys->wep_key_length[index] /= 2;
            AtoH(ptr, (char *)&wep_keys->wep_key[index],
                 (int)wep_keys->wep_key_length[index]);
        } else {
            LOG_FUNC("WEP Key Length(=%d) is incorrect.\n",
                     wep_keys->wep_key_length[index]);
        }
        index++;
        if (index >= WIFI_NUMBER_WEP_KEYS)
            break;
    }

    /* Default key ID */
    char def_key_id_buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t def_key_id_buf_size = sizeof(def_key_id_buf);

    if (port == WIFI_PORT_AP) {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "DefaultKeyId",
                                (uint8_t *)def_key_id_buf, &def_key_id_buf_size)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "DefaultKeyId",
                                (uint8_t *)def_key_id_buf, &def_key_id_buf_size)
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    }

    wep_keys->wep_tx_key_index = (uint8_t)atoi(def_key_id_buf);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_mac_address(uint8_t port, uint8_t *address)
{
    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (address == NULL) {
        LOG_FUNC("mac address is null.\n");
        return WIFI_ERR_PARA_INVALID;
    }

#ifdef MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    int8_t status;

    status = kalSprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
                        address[0], address[1], address[2],
                        address[3], address[4], address[5]);
    if (status < 0)
        return WIFI_FAIL;
    if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "MacAddr",
                             NVDM_DATA_ITEM_TYPE_STRING,
                             (uint8_t *)buf, kalStrLen(buf)) ==
        NVDM_STATUS_OK)
        return WIFI_SUCC;
#endif /* #ifdef MTK_NVDM_ENABLE */

    return WIFI_FAIL;
}

int32_t wifi_profile_get_mac_address(uint8_t port, uint8_t *address)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t len = sizeof(buf);

    if (!wifi_is_port_valid(port)) {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    if (address == NULL) {
        LOG_FUNC("address is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    /* Use STA MAC/IP as AP MAC/IP for the time being, due to N9
        dual interface not ready yet */
    if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "MacAddr",
                            (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    wifi_conf_get_mac_from_str((char *)address, buf);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_channel(uint8_t port, uint8_t channel)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    if (!wifi_is_channel_valid(channel)) {
        LOG_FUNC("Channel %d in not allowed!", channel);
        return WIFI_ERR_PARA_INVALID;
    }

    kalSprintf(buf, "%d", channel);

    if (port == WIFI_PORT_AP) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_HAPD, "channel",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)buf, kalStrLen(buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else if (port == WIFI_PORT_STA) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "Channel",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)buf, kalStrLen(buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_get_channel(uint8_t port, uint8_t *channel)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t len = sizeof(buf);

    if (channel == NULL)
        return WIFI_ERR_PARA_INVALID;

    if (port == WIFI_PORT_AP) {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_HAPD, "channel",
                                (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else if (port == WIFI_PORT_STA) {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "Channel",
                                (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }

    *channel = atoi(buf);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_bandwidth(uint8_t port, uint8_t bandwidth)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    if (!wifi_is_bandwidth_valid(bandwidth)) {
        LOG_FUNC("bandwidth is invalid: %d\n", bandwidth);
        return WIFI_ERR_PARA_INVALID;
    }

    kalSprintf(buf, "%d", bandwidth);

    if (port == WIFI_PORT_AP) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "BW",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)buf, kalStrLen(buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else if (port == WIFI_PORT_STA) {
        if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "BW",
                                 NVDM_DATA_ITEM_TYPE_STRING,
                                 (uint8_t *)buf, kalStrLen(buf))
            != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}


int32_t wifi_profile_get_bandwidth(uint8_t port, uint8_t *bandwidth)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t len = sizeof(buf);

    if (bandwidth == NULL) {
        LOG_FUNC("bandwidth is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (port == WIFI_PORT_AP) {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "BW",
                                (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else if (port == WIFI_PORT_STA) {
        if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "BW",
                                (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
            return WIFI_FAIL;
        }
    } else {
        LOG_FUNC("port is invalid: %d\n", port);
        return WIFI_ERR_PARA_INVALID;
    }

    *bandwidth = atoi(buf);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_dtim_interval(uint8_t interval)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    if (interval == 0) {
        LOG_FUNC("interval is invalid: %d\n", interval);
        return WIFI_ERR_PARA_INVALID;
    }

    kalSprintf(buf, "%d", interval);
    if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "DtimPeriod",
                             NVDM_DATA_ITEM_TYPE_STRING,
                             (const uint8_t *)buf, kalStrLen(buf)) != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_get_dtim_interval(uint8_t *interval)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t len = sizeof(buf);

    if (interval == NULL) {
        LOG_FUNC("interval is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "DtimPeriod",
                            (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    *interval = atoi(buf);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_listen_interval(uint8_t interval)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    if (interval == 0) {
        LOG_FUNC("interval is invalid: %d\n", interval);
        return WIFI_ERR_PARA_INVALID;
    }

    kalSprintf(buf, "%d", interval);

    if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "ListenInterval",
                             NVDM_DATA_ITEM_TYPE_STRING,
                             (const uint8_t *)buf, kalStrLen(buf)) != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_get_listen_interval(uint8_t *interval)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t len = sizeof(buf);

    if (interval == NULL) {
        LOG_FUNC("interval is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "ListenInterval",
                            (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    *interval = atoi(buf);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_power_save_mode(uint8_t power_save_mode)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    if (!wifi_is_ps_mode_valid(power_save_mode)) {
        LOG_FUNC("power_save_mode is invalid: %d\n", power_save_mode);
        return WIFI_ERR_PARA_INVALID;
    }

    kalSprintf(buf, "%d", power_save_mode);
    if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_STA, "PSMode",
                             NVDM_DATA_ITEM_TYPE_STRING,
                             (const uint8_t *)buf, kalStrLen(buf)) != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_get_power_save_mode(uint8_t *power_save_mode)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};
    uint32_t len = sizeof(buf);

    if (power_save_mode == NULL) {
        LOG_FUNC("power_save_mode is null\n");
        return WIFI_ERR_PARA_INVALID;
    }

    if (nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "PSMode",
                            (uint8_t *)buf, &len) != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    *power_save_mode = atoi(buf);
    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

#if IP_NAPT
int32_t wifi_profile_set_napt_tcp_entry_num(uint8_t num)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    kalSprintf(buf, "%d", num);
    if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "NatpTcpEntry",
                             NVDM_DATA_ITEM_TYPE_STRING,
                             (const uint8_t *)buf, kalStrLen(buf)) != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}

int32_t wifi_profile_set_napt_udp_entry_num(uint8_t num)
{
#if MTK_NVDM_ENABLE
    char buf[WIFI_PROFILE_BUFFER_LENGTH] = {0};

    kalSprintf(buf, "%d", num);
    if (nvdm_write_data_item(WIFI_PROFILE_BUFFER_AP, "NatpUdpEntry",
                             NVDM_DATA_ITEM_TYPE_STRING,
                             (const uint8_t *)buf, kalStrLen(buf)) != NVDM_STATUS_OK) {
        return WIFI_FAIL;
    }

    return WIFI_SUCC;
#else /* #if MTK_NVDM_ENABLE */
    return WIFI_FAIL;
#endif /* #if MTK_NVDM_ENABLE */
}
#endif /* #if IP_NAPT */

#endif /* #ifdef MTK_WIFI_PROFILE_ENABLE */


#if (CFG_ENABLE_WIFI_DIRECT == 1) && (CFG_SUPPORT_APPEND_VENDOR_IE == 1)
void *wifi_add_vendor_ie(enum wifi_pkt_type_t pkt_type, char *vie,
                         uint16_t vie_len)
{
    struct GLUE_INFO *prGlueInfo;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex;
    struct LINK *prVendorIEList;
    struct SAP_VENDOR_IE *prVendorIE;

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    ucBssIndex = p2pFuncGetSapBssIndex(prGlueInfo);
    prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
    if (!prBssInfo || vie_len == 0)
        return (void *)0;

    if (pkt_type == BEACON)
        prVendorIEList = &prBssInfo->rVendorIEBeaconList;
    else if (pkt_type == PROBE_RESP)
        prVendorIEList = &prBssInfo->rVendorIEProbRespList;
    else
        return (void *)0;

    prVendorIE = (struct SAP_VENDOR_IE *)kalMemAlloc(sizeof(struct SAP_VENDOR_IE),
                                                     VIR_MEM_TYPE);
    if (prVendorIE == NULL)
        return (void *)0;
    prVendorIE->prVendorIeBuffer = (char *)kalMemAlloc(vie_len * sizeof(char),
                                                       VIR_MEM_TYPE);
    if (prVendorIE->prVendorIeBuffer == NULL) {
        kalMemFree(prVendorIE, VIR_MEM_TYPE, sizeof(struct SAP_VENDOR_IE));
        return (void *)0;
    }

    kalMemCopy(prVendorIE->prVendorIeBuffer, vie, vie_len);
    prVendorIE->u2VendorIeLen = vie_len;
    prVendorIE->id = ++prBssInfo->u4VendorIEIndex;
    LINK_INSERT_TAIL(prVendorIEList, &prVendorIE->rLinkEntry);
    bssUpdateBeaconContent(prGlueInfo->prAdapter, ucBssIndex);

    return (void *)prVendorIE->id;
}

int wifi_delete_vendor_ie(void *handle)
{
    struct GLUE_INFO *prGlueInfo;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex;
    struct LINK *prVendorIEList;
    struct SAP_VENDOR_IE *prCurrVendorIE;

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    ucBssIndex = p2pFuncGetSapBssIndex(prGlueInfo);
    prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
    if (!prBssInfo)
        return -1;

    prVendorIEList = &prBssInfo->rVendorIEBeaconList;
    LINK_FOR_EACH_ENTRY(prCurrVendorIE, prVendorIEList, rLinkEntry,
                        struct SAP_VENDOR_IE) {
        if (!prCurrVendorIE)
            break;
        if (prCurrVendorIE->id == (int)handle) {
            LINK_REMOVE_KNOWN_ENTRY(prVendorIEList, &prCurrVendorIE->rLinkEntry);
            kalMemFree(prCurrVendorIE->prVendorIeBuffer, VIR_MEM_TYPE,
                       sizeof(char) * prCurrVendorIE->u2VendorIeLen);
            kalMemFree(prCurrVendorIE, VIR_MEM_TYPE,
                       sizeof(struct SAP_VENDOR_IE));
            bssUpdateBeaconContent(prGlueInfo->prAdapter, ucBssIndex);
            return 0;
        }
    }
    prVendorIEList = &prBssInfo->rVendorIEProbRespList;
    LINK_FOR_EACH_ENTRY(prCurrVendorIE, prVendorIEList, rLinkEntry,
                        struct SAP_VENDOR_IE) {
        if (!prCurrVendorIE)
            break;
        if (prCurrVendorIE->id == (int)handle) {
            LINK_REMOVE_KNOWN_ENTRY(prVendorIEList, &prCurrVendorIE->rLinkEntry);
            kalMemFree(prCurrVendorIE->prVendorIeBuffer, VIR_MEM_TYPE,
                       sizeof(char) * prCurrVendorIE->u2VendorIeLen);
            kalMemFree(prCurrVendorIE, VIR_MEM_TYPE,
                       sizeof(struct SAP_VENDOR_IE));
            bssUpdateBeaconContent(prGlueInfo->prAdapter, ucBssIndex);
            return 0;
        }
    }
    return -1;
}

static int update_vendor_ie(struct GLUE_INFO *prGlueInfo,
                            struct SAP_VENDOR_IE *prVendorIE, uint8_t ucBssIndex,
                            char *vie, uint16_t vie_len)
{
    if (vie_len != prVendorIE->u2VendorIeLen) {
        char *tmp_vie = (char *)kalMemAlloc(vie_len * sizeof(char),
                                            VIR_MEM_TYPE);
        if (tmp_vie == NULL)
            return -1;
        kalMemFree(prVendorIE->prVendorIeBuffer, VIR_MEM_TYPE,
                   prVendorIE->u2VendorIeLen * sizeof(char));
        prVendorIE->prVendorIeBuffer = tmp_vie;
        prVendorIE->u2VendorIeLen = vie_len;
    }

    kalMemCopy(prVendorIE->prVendorIeBuffer, vie, vie_len);
    bssUpdateBeaconContent(prGlueInfo->prAdapter, ucBssIndex);
    return 0;
}

int wifi_update_vendor_ie(void *handle, char *vie, uint16_t vie_len)
{
    struct GLUE_INFO *prGlueInfo;
    struct BSS_INFO *prBssInfo;
    uint8_t ucBssIndex;
    struct LINK *prVendorIEList;
    struct SAP_VENDOR_IE *prCurrVendorIE;

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    ucBssIndex = p2pFuncGetSapBssIndex(prGlueInfo);
    prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
    if (!prBssInfo || vie_len == 0)
        return -1;

    prVendorIEList = &prBssInfo->rVendorIEBeaconList;
    LINK_FOR_EACH_ENTRY(prCurrVendorIE, prVendorIEList, rLinkEntry,
                        struct SAP_VENDOR_IE) {
        if (!prCurrVendorIE)
            break;
        if (prCurrVendorIE->id == (int)handle)
            return update_vendor_ie(prGlueInfo, prCurrVendorIE, ucBssIndex,
                                    vie, vie_len);
    }
    prVendorIEList = &prBssInfo->rVendorIEProbRespList;
    LINK_FOR_EACH_ENTRY(prCurrVendorIE, prVendorIEList, rLinkEntry,
                        struct SAP_VENDOR_IE) {
        if (!prCurrVendorIE)
            break;
        if (prCurrVendorIE->id == (int)handle)
            return update_vendor_ie(prGlueInfo, prCurrVendorIE, ucBssIndex,
                                    vie, vie_len);
    }
    return -1;
}
#endif /* #if (CFG_ENABLE_WIFI_DIRECT == 1) && (CFG_SUPPORT_APPEND_VENDOR_IE == 1) */

#if CFG_SUPPORT_CSI
/**
* @brief This function sets the CSI mode and it takes effect
* immediately.
*
* @param[in] mode the CSI mode to set.
*
* Value                         |Definition |
* ------------------------------|------------------------------------------------------------------------|
* \b 0                          | Stop Capturing CSI Data |
* \b 1                          | Start Capturing CSI Data |
* \b 2 3 0 32                   | Frame Type: Beacon |
* \b 2 3 0 34                   | Frame Type: QoS Data |
* \b 2 5 0                      | Output: Original format |
* \b 2 5 2                      | Output: Apply tone mask and reorder tones |
* \b 3 0                        | Disable PSRAM for CSI Data |
* \b 3 1 0 34                   | Enable PSRAM for CSI Data |
* \b 3 2                        | Transmit CSI Data using xmodem protocol |
*
* @return  >=0 the operation completed successfully, <0 the operation failed.
*/
int wifi_config_set_csi(char *pcCommand, int i4TotalLen)
{
    struct GLUE_INFO *prGlueInfo;
    uint32_t rStatus = WLAN_STATUS_SUCCESS;
    uint32_t u4BufLen = 0;
    int32_t i4BytesWritten = 0;
    int32_t i4Argc = 0;
    signed char *apcArgv[WLAN_CFG_ARGV_MAX];
    struct ADAPTER *prAdapter = NULL;
    struct CMD_CSI_CONTROL_T *prCSICtrl = NULL;
    uint32_t u4Ret = 0;
    struct CSI_INFO_T *prCSIInfo = NULL;

    prGlueInfo = &g_rGlueInfo;
    ASSERT(prGlueInfo);

    DBGLOG(RSN, LOUD, "[CSI] command is %s\n", pcCommand);
    wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
    DBGLOG(REQ, LOUD, "[CSI] argc is %i\n", i4Argc);

    DBGLOG(RSN, INFO, "[CSI] priv_driver_csi_control\n");

    prAdapter = prGlueInfo->prAdapter;
    prCSIInfo = &(prGlueInfo->prAdapter->rCSIInfo);

    prCSICtrl = (struct CMD_CSI_CONTROL_T *)kalMemAlloc(
                    sizeof(struct CMD_CSI_CONTROL_T), VIR_MEM_TYPE);
    if (!prCSICtrl) {
        DBGLOG(REQ, ERROR,
               "[CSI] allocate memory for prCSICtrl failed\n");
        i4BytesWritten = -1;
        goto out;
    }

    if (i4Argc < 2 || i4Argc > 5) {
        DBGLOG(REQ, ERROR, "[CSI] argc %i is invalid\n", i4Argc);
        i4BytesWritten = -1;
        goto out;
    }

    u4Ret = kalkStrtou8(apcArgv[1], 0, &(prCSICtrl->ucMode));
    if (u4Ret) {
        DBGLOG(REQ, LOUD, "[CSI] parse ucMode error u4Ret=%d\n", u4Ret);
        goto out;
    }

    if (prCSICtrl->ucMode >= CSI_CONTROL_MODE_NUM) {
        DBGLOG(REQ, LOUD, "[CSI] Invalid ucMode %d, should be 0~3\n",
               prCSICtrl->ucMode);
        goto out;
    }

    prCSICtrl->ucBandIdx = ENUM_BAND_0;
    prCSIInfo->ucMode = prCSICtrl->ucMode;

    if (prCSICtrl->ucMode == CSI_CONTROL_MODE_STOP ||
        prCSICtrl->ucMode == CSI_CONTROL_MODE_START) {
        prCSIInfo->bIncomplete = FALSE;
        prCSIInfo->u4CopiedDataSize = 0;
        prCSIInfo->u4RemainingDataSize = 0;
        goto send_cmd;
    }

    u4Ret = kalkStrtou8(apcArgv[2], 0, &(prCSICtrl->ucCfgItem));
    if (u4Ret) {
        DBGLOG(REQ, LOUD,
               "[CSI] parse cfg item error u4Ret=%d\n", u4Ret);
        goto out;
    }

    if (prCSICtrl->ucCfgItem >= CSI_CONFIG_ITEM_NUM) {
        DBGLOG(REQ, LOUD, "[CSI] Invalid csi cfg_item %u\n",
               prCSICtrl->ucCfgItem);
        goto out;
    }

    u4Ret = kalkStrtou8(apcArgv[3], 0, &(prCSICtrl->ucValue1));
    if (u4Ret) {
        DBGLOG(REQ, LOUD,
               "[CSI] parse csi cfg value1 error u4Ret=%d\n", u4Ret);
        goto out;
    }
    prCSIInfo->ucValue1[prCSICtrl->ucCfgItem] = prCSICtrl->ucValue1;

    if (i4Argc == 5) {
        u4Ret = kalkStrtou8(apcArgv[4], 0, &(prCSICtrl->ucValue2));
        if (u4Ret) {
            DBGLOG(REQ, LOUD,
                   "[CSI] parse csi cfg value2 error u4Ret=%d\n",
                   u4Ret);
            goto out;
        }
        prCSIInfo->ucValue2[prCSICtrl->ucCfgItem] = prCSICtrl->ucValue2;
    }

#ifdef MTK_CSI_PSRAM_ENABLE
    if (prCSICtrl->ucMode == CSI_CONTROL_MODE_PSRAM && prCSICtrl->ucCfgItem == 2)
        xmodem_send_csi_data((uint32_t)prAdapter->ram_csi_data,
                             prAdapter->u4CSIBufferCount);
#else /* #ifdef MTK_CSI_PSRAM_ENABLE */
    if (prCSICtrl->ucMode == CSI_CONTROL_MODE_PSRAM)
        goto out;
#endif /* #ifdef MTK_CSI_PSRAM_ENABLE */

    DBGLOG(REQ, STATE,
           "[CSI][DEBUG] Set mode %d, csi cfg item %d, value1 %d, value2 %d",
           prCSICtrl->ucMode, prCSICtrl->ucCfgItem,
           prCSICtrl->ucValue1, prCSICtrl->ucValue2);

send_cmd:
    rStatus = kalIoctl(prGlueInfo, wlanoidSetCSIControl, prCSICtrl,
                       sizeof(struct CMD_CSI_CONTROL_T), FALSE, FALSE, TRUE, &u4BufLen);

    DBGLOG(REQ, INFO, "[CSI] %s: command result is %s\n",
           __func__, pcCommand);
    DBGLOG(REQ, STATE,
           "[CSI] mode %d, csi cfg item %d, value1 %d, value2 %d",
           prCSICtrl->ucMode, prCSICtrl->ucCfgItem,
           prCSICtrl->ucValue1, prCSICtrl->ucValue2);
    DBGLOG(REQ, LOUD, "[CSI] rStatus %u\n", rStatus);

    if (rStatus != WLAN_STATUS_SUCCESS) {
        DBGLOG(REQ, ERROR, "[CSI] send CSI control cmd failed\n");
        i4BytesWritten = -1;
    }

out:
    if (prCSICtrl)
        kalMemFree(prCSICtrl, VIR_MEM_TYPE,
                   sizeof(struct CMD_CSI_CONTROL_T));

    return i4BytesWritten;
}
#endif /* #if CFG_SUPPORT_CSI */

#if CFG_SUPPORT_ANT_DIV
/**
* @brief This function sets the antenna diversity mode and it takes effect
* immediately.
*
* @param[in] mode the antenna diversity mode to set.
*
* Value                         |Definition |
* ------------------------------|----------------------------------------------|
* \b 0                          | force antenna 0 |
* \b 1                          | force antenna 1 |
* \b 2                          | enable diversity |
* \b 3                          | disable diversity |
*
* @return  >=0 the operation completed successfully, <0 the operation failed.
*
* @note Value 0~2 works only when the HW configuration support. For example,
* 1) 7933 RFB with an external SPDT module to control antenna by GPIO40
* 2) And eFuse[0x3DA]=0x01 meaning Wi-Fi diversity with Bluetooth in use
* If the HW does not support, please choose value 3 to disable it.
*/
int32_t wifi_config_set_antdiv_mode(uint8_t mode)
{
    struct GLUE_INFO *prGlueInfo = NULL;
    uint32_t rStatus = WIFI_FAIL;
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
    uint32_t u4BufLen = 0;
    char *cmd = "AntDivMode";
    char *cMode = NULL;
    struct ADAPTER *prAdapter = NULL;

    prGlueInfo = &g_rGlueInfo;
    prAdapter = prGlueInfo->prAdapter;

    if (mode >= ENUM_ANTDIV_MODE_NUM) {
        goto out;
    }

    /* pinmux for antenna diversity */
    if (mode < ENUM_ANTDIV_MODE_DISABLE) {
        hal_pinmux_set_function(HAL_GPIO_40, MT7933_PIN_40_FUNC_ANT_SEL1);
    }

    prAdapter->rWifiVar.ucAntDivMode = mode;

    cMode = (char *)kalMemAlloc(sizeof(int), VIR_MEM_TYPE);
    if (cMode == NULL)
        goto out;

    kalSprintf(cMode, " %d", mode);

    kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
    rChipConfigInfo.u2MsgSize = kalStrLen(cmd) + kalStrLen(cMode) + 1;
    kalStrnCpy((char *)rChipConfigInfo.aucCmd, cmd,
               CHIP_CONFIG_RESP_SIZE - 1);
    kalStrnCpy((char *)rChipConfigInfo.aucCmd + kalStrLen(cmd),
               cMode, kalStrLen(cMode));
    rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

    rStatus = kalIoctl(prGlueInfo, wlanoidSetChipConfig,
                       &rChipConfigInfo, sizeof(rChipConfigInfo),
                       FALSE, FALSE, TRUE, &u4BufLen);
    if (rStatus != WLAN_STATUS_SUCCESS)
        LOG_FUNC("Failed to set chip config (0x%08x)\n", rStatus);

out:
    if (cMode)
        kalMemFree(cMode, VIR_MEM_TYPE, sizeof(*cMode));

    return rStatus;
}

/**
* @brief This function gets the antenna diversity mode of the Wi-Fi driver.
*
* @param[out] mode indicates the antenna diversity mode.
*
* Value                         |Definition |
* ------------------------------|----------------------------------------------|
* \b 0                          | force antenna 0 |
* \b 1                          | force antenna 1 |
* \b 2                          | enable diversity |
* \b 3                          | disable diversity |
*
* @return  >=0 the operation completed successfully, <0 the operation failed.
*/
int32_t wifi_config_get_antdiv_mode(uint8_t *mode)
{
    struct GLUE_INFO *prGlueInfo;
    int32_t rStatus;
    uint32_t u4BufLen = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    char *cmd = "AntDivMode";
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

    prGlueInfo = &g_rGlueInfo;
    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
    rChipConfigInfo.u2MsgSize = kalStrLen(cmd) + 1;

    kalStrnCpy((char *)rChipConfigInfo.aucCmd, cmd,
               CHIP_CONFIG_RESP_SIZE - 1);
    rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryChipConfig,
                       &rChipConfigInfo, sizeof(rChipConfigInfo),
                       TRUE, TRUE, TRUE, &u4BufLen);

    if (rStatus != WIFI_SUCC)
        goto out;

    wlanCfgParseArgument((char *)rChipConfigInfo.aucCmd, &i4Argc, apcArgv);
    rStatus = kalkStrtou8(apcArgv[2], 0, mode);
    if (rStatus)
        LOG_FUNC("Parse error rStatus=%d\n", rStatus);
out:
    return rStatus;
}

/**
* @brief This function gets the current antenna index in use.
*
* @param[out] index indicates the antenna index.
*
* Value                         |Definition |
* ------------------------------|------------------------------------------------------------------------|
* \b 0                          | force antenna 0|
* \b 1                          | force antenna 1|
*
* @return  >=0 the operation completed successfully, <0 the operation failed.
*/
int32_t wifi_config_get_antdiv_cur_idx(uint8_t *index)
{
    struct GLUE_INFO *prGlueInfo;
    int32_t rStatus;
    uint32_t u4BufLen = 0;
    int32_t i4Argc = 0;
    int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
    char *cmd = "AntDivCurIdx";
    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

    prGlueInfo = &g_rGlueInfo;
    rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
    rChipConfigInfo.u2MsgSize = kalStrLen(cmd) + 1;

    kalStrnCpy((char *)rChipConfigInfo.aucCmd, cmd,
               CHIP_CONFIG_RESP_SIZE - 1);
    rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

    rStatus = kalIoctl(prGlueInfo, wlanoidQueryChipConfig,
                       &rChipConfigInfo, sizeof(rChipConfigInfo),
                       TRUE, TRUE, TRUE, &u4BufLen);

    if (rStatus != WIFI_SUCC)
        goto out;

    wlanCfgParseArgument((char *)rChipConfigInfo.aucCmd, &i4Argc, apcArgv);
    rStatus = kalkStrtou8(apcArgv[2], 0, index);
    if (rStatus)
        LOG_FUNC("Parse error rStatus=%d\n", rStatus);
out:
    return rStatus;
}
#endif /* #if CFG_SUPPORT_ANT_DIV */

