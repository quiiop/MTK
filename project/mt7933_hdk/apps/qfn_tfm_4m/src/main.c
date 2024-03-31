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
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "sys_init.h"
#if defined(MTK_MINICLI_ENABLE)
#include "cli_def.h"
#endif

#ifdef MTK_UT_ENABLE
#include "ut.h"
#endif

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#endif

#ifdef HAL_WDT_MODULE_ENABLED
#include "hal_wdt.h"
#endif
#include "common.h"
#include "mt7933_pos.h"
#include "hal_nvic.h"

#ifdef MTK_MT7933_CONSYS_WIFI_ENABLE
#include "gl_init.h"
#endif

#ifdef MTK_MT7933_BT_ENABLE
#include "bt_driver.h"
#endif

#ifdef MTK_BT_ENABLE
//#include "bt_init.h"
//#include "hal_psram.h"
#endif

#ifdef MTK_TFM_ENABLE
#include "tfm_ns_interface_iotsdk_init.h"
#endif

#ifdef HAL_GCPU_MODULE_ENABLED
#include "hal_gcpu_internal.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
#include "bsp_gpio_ept_config.h"
#endif

/* WiFi auto connection. */
#include "wifi_api_ex.h"
#ifdef MTK_NVDM_ENABLE
#include "nvdm.h"
#endif
#include "ping.h"
#include "wifi_netif.h"
/* Task priorities. */

#ifdef MTK_UT_ENABLE
#define utTask_PRIORITY (configMAX_PRIORITIES - 1)
#endif

#ifdef MTK_MT7933_CONSYS_ENABLE
SemaphoreHandle_t gConnsysCalLock = NULL;
SemaphoreHandle_t gConnsysRadioOnLock = NULL;
#endif

#ifdef MTK_TFM_ENABLE
#ifdef MTK_OS_HEAP_EXTEND
#if (MTK_OS_HEAP_EXTEND == HEAP_EXTEND_MULTI)
#ifdef MTK_NON_INIT_HEAP
ATTR_ZIDATA_IN_NON_INIT_SYSRAM  static uint8_t sysHeap[ configTOTAL_HEAP_SIZE ];
ATTR_ZIDATA_IN_NON_INIT_RAM     static uint8_t platHeap[ configPlatform_HEAP_SIZE ];
#else
ATTR_ZIDATA_IN_SYSRAM static uint8_t sysHeap[ configTOTAL_HEAP_SIZE ];
ATTR_ZIDATA_IN_RAM    static uint8_t platHeap[ configPlatform_HEAP_SIZE ];
#endif

HeapRegion_t xHeapRegions[] = {
    { (uint8_t *) sysHeap,  configTOTAL_HEAP_SIZE, (uint8_t *)"System"   },          // System Heap Region
    { (uint8_t *) platHeap, configPlatform_HEAP_SIZE, (uint8_t *)"Platform" },       // Platform Heap Region
    { NULL, 0, NULL }
};
#elif (MTK_OS_HEAP_EXTEND == HEAP_EXTEND_HEAP5)
static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];

HeapRegion_t xHeapRegions[] = {
    { (uint8_t *) ucHeap,      configTOTAL_HEAP_SIZE },       // Enable SYSRAM Heap
    { (uint8_t *) 0xA0500000, (size_t)0x300000  },            // Enable PSRAM  Heap
    { NULL, 0 }
};
#endif
#endif
#endif

#ifdef HAL_WDT_MODULE_ENABLED
void wdt_timeout_handle(hal_wdt_reset_status_t wdt_reset_status)
{
    printf("%s: status:%u\n", __FUNCTION__, (unsigned int)wdt_reset_status);
    /* assert 0 to trigger exception hanling flow */
    configASSERT(0);
}

static void wdt_init(void)
{
    hal_wdt_config_t wdt_init;
#if defined(MTK_SWLA_ENABLE) && defined(MTK_SWLA_WDT_RESET_TRACE)
    /**
     * Dump SWLA trace after watch dog reset, if system hang.
     **/
    wdt_init.mode = HAL_WDT_MODE_RESET;
#else
    /**
     * WDT interrupt to trigger exception flow, if system hang.
     **/
    wdt_init.mode = HAL_WDT_MODE_INTERRUPT;
#endif
    wdt_init.seconds = 60;
    hal_wdt_init(&wdt_init);
    hal_wdt_register_callback(wdt_timeout_handle);
#ifdef MTK_SYSTEM_HANG_CHECK_ENABLE
    hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
#else
    hal_wdt_disable(HAL_WDT_DISABLE_MAGIC); //Disable WDT when MTK_SYSTEM_HANG_CHECK_ENABLE = n
#endif
}
#endif


/* for idle task feed wdt (DO NOT enter sleep mode)*/
void vApplicationIdleHook(void)
{
#ifdef HAL_WDT_MODULE_ENABLED
    hal_wdt_feed(HAL_WDT_FEED_MAGIC);
#endif
}

#if( configSUPPORT_STATIC_ALLOCATION == 1 )

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
    *ppxTimerTaskTCBBuffer = (StaticTask_t *) pvPortMalloc(sizeof(StaticTask_t));
    if (*ppxTimerTaskTCBBuffer != NULL) {
        *ppxTimerTaskStackBuffer = (StackType_t *) pvPortMalloc((((size_t) * pulTimerTaskStackSize) * sizeof(StackType_t)));
    }
}
/*-----------------------------------------------------------*/

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
    *ppxIdleTaskTCBBuffer = (StaticTask_t *) pvPortMalloc(sizeof(StaticTask_t));
    if (*ppxIdleTaskTCBBuffer != NULL) {
        *ppxIdleTaskStackBuffer = (StackType_t *) pvPortMalloc((((size_t) * pulIdleTaskStackSize) * sizeof(StackType_t)));
    }
}

#endif

/*******************************************************************************
 * WiFi autoConnection
 ******************************************************************************/
uint8_t wifi_init_done_handle = 0;

bool iot_IsIpReady(struct netif *netif)
{
    return ip_addr_isany_val(netif->ip_addr) ? 0 : 1;
}

int32_t wifi_init_done_handler_test(wifi_event_t event,
                                    uint8_t *payload,
                                    uint32_t length)
{
    wifi_init_done_handle = 1;
    LOG_I(common, "WiFi Init Done: port = %d", payload[6]);

    return 0;
}

void wifi_auto_init_task(void *para)
{

    struct wifi_cfg wifi_config = {0};
    if (0 != wifi_config_init(&wifi_config)) {
        LOG_E(common, "wifi config init fail");
    }
    /*  load setting NVDM setting
        //FW need known the target ssid and BGchannel table(1 channel), to do STA fast link scan before WIFI_EVENT_IOT_INIT_COMPLETE.
        uint8_t test_opmode = WIFI_MODE_STA_ONLY;
        char *  test_ssid = "TEST";
        char *  password_AP = "QAZWSXEDC";
        //char *  test_bg_ch_table = "6,1,0|";  //first_channel=6, number_of_channel=1, scan_mode=0
        wifi_config.opmode = test_opmode;
        wifi_config.sta_ssid_len = strlen(test_ssid);
        wifi_config.sta_wpa_psk_len = strlen(password_AP);

        memcpy(wifi_config.sta_ssid, test_ssid, wifi_config.sta_ssid_len);
        memcpy(wifi_config.sta_wpa_psk, password_AP, wifi_config.sta_wpa_psk_len);

    */

    wifi_config_t config = {0};
    wifi_config_ext_t config_ext = {0};
    memset(&config, 0, sizeof(config));
    memset(&config_ext, 0, sizeof(config_ext));

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

    config_ext.sta_auto_connect_present = 1;
    config_ext.sta_auto_connect = 1;

    wifi_init(&config, &config_ext);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE, (wifi_event_handler_t)wifi_init_done_handler_test);
    wifi_connection_scan_init(g_ap_list, MTK_SCAN_LIST_SIZE);

    struct netif *sta_if = netif_find("st1");

    while (sta_if != NULL) {
        if ((pdTRUE == lwip_net_ready()) && (iot_IsIpReady(sta_if))) {
            /*ping_request(3, "8.8.8.8", PING_IP_ADDR_V4, 64, NULL);*/
            break;
        }
    }
    vTaskDelete(NULL);
}

int wifi_task_create(void)
{
    if (xTaskCreate(wifi_auto_init_task,
                    wifi_auto_init_TASK_NAME,
                    wifi_auto_init_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                    NULL,
                    wifi_auto_init_TASK_PRIO,
                    NULL) != pdPASS) {
        LOG_E(common, "xTaskCreate fail");
        return -1;
    }

    return 0;
}


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
#ifdef MTK_TFM_ENABLE
    /* os extend heap init */
#ifdef MTK_OS_HEAP_EXTEND
    vPortDefineHeapRegions(xHeapRegions);
#endif

    tfm_ns_interface_iotsdk_init();
#endif

    /* Do system initialization, eg: hardware, nvdm, logging and random seed. */
    system_init();

#ifdef HAL_GPIO_MODULE_ENABLED
    bsp_ept_gpio_setting_init();
#endif

#if defined(MTK_MINICLI_ENABLE)
    /* Initialize cli task to enable user input cli command from uart port.*/
    cli_def_create();
    cli_task_create();
#endif

    /* Call this function to indicate the system initialize done. */
    //SysInitStatus_Set();

#ifdef HAL_WDT_MODULE_ENABLED
    wdt_init();
#endif
    printf("Hello Mediatek!!\r\n\r\n");

#ifdef MTK_MT7933_CONSYS_ENABLE
    gConnsysCalLock = xSemaphoreCreateMutex();
    gConnsysRadioOnLock = xSemaphoreCreateMutex();
#if defined(BRINGUP_WIFI_ENABLE) || defined(BRINGUP_BT_ENABLE)
    mt7933_conninfra_init();
#ifdef MTK_BT_ENABLE
    //bt_driver_init();
#endif
#endif
#ifdef MTK_MT7933_CONSYS_WIFI_ENABLE
    /* mask out connsys on by default beofre ES for build sanity check */
    /* wifi_init_task(); */
#endif
#endif // MTK_MT7933_CONSYS_ENABLE

#ifdef HAL_GCPU_MODULE_ENABLED
    gcpu_init();
#endif

    //#ifdef MTK_UT_ENABLE
    //    if (xTaskCreate((TaskFunction_t)ut_main, "utTask", 1024, NULL, utTask_PRIORITY, NULL) != pdPASS)
    //    {
    //        printf("Task creation failed!.\r\n");
    //        while (1);
    //    }
    //#endif

#ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE
    printf("AUDIO DRIVER INIT\n");
    extern void audio_init(void);
    audio_init();
#endif
    wifi_task_create();
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for (;;);
}


