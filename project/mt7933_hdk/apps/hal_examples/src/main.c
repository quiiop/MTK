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
#endif /* #if defined(MTK_MINICLI_ENABLE) */

#ifdef MTK_UT_ENABLE
#include "ut.h"
#endif /* #ifdef MTK_UT_ENABLE */

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

#ifdef HAL_WDT_MODULE_ENABLED
#include "hal_wdt.h"
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
#include "common.h"
#include "mt7933_pos.h"
#include "hal_nvic.h"
#include "hal_efuse_get.h"


#ifdef MTK_MT7933_CONSYS_WIFI_ENABLE
#include "gl_init.h"
#endif /* #ifdef MTK_MT7933_CONSYS_WIFI_ENABLE */

#ifdef MTK_MT7933_BT_ENABLE
#include "bt_driver.h"
#endif /* #ifdef MTK_MT7933_BT_ENABLE */

#ifdef MTK_SSUSB_GADGET_ENABLE
#include "ssusb_gadget_export.h"
#endif /* #ifdef MTK_SSUSB_GADGET_ENABLE */

#ifdef MTK_SSUSB_HOST_ENABLE
#include "ssusb_host_export.h"
#endif /* #ifdef MTK_SSUSB_HOST_ENABLE */

#ifdef MTK_BT_ENABLE
//#include "bt_init.h"
//#include "hal_psram.h"
#endif /* #ifdef MTK_BT_ENABLE */

#ifdef MTK_TFM_ENABLE
#include "tfm_ns_interface_iotsdk_init.h"
#endif /* #ifdef MTK_TFM_ENABLE */

#ifdef HAL_GCPU_MODULE_ENABLED
#include "hal_gcpu_internal.h"
#endif /* #ifdef HAL_GCPU_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
#include "bsp_gpio_ept_config.h"
#endif /* #ifdef HAL_GPIO_MODULE_ENABLED */

#ifdef MTK_HIFI4DSP_ENABLE
#include "mtk_hifixdsp_common.h"
#endif /* #ifdef MTK_HIFI4DSP_ENABLE */

/* Task priorities. */

#ifdef MTK_UT_ENABLE
#define utTask_PRIORITY (configMAX_PRIORITIES - 1)
#endif /* #ifdef MTK_UT_ENABLE */

#ifdef MTK_MT7933_CONSYS_ENABLE
SemaphoreHandle_t gConnsysCalLock = NULL;
SemaphoreHandle_t gConnsysRadioOnLock = NULL;
#endif /* #ifdef MTK_MT7933_CONSYS_ENABLE */
#ifdef HAL_WDT_MODULE_ENABLED
void wdt_timeout_handle(hal_wdt_reset_status_t wdt_reset_status)
{
    printf("%s: stattus:%u\r\n", __FUNCTION__, (unsigned int)wdt_reset_status);
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
#else /* #if defined(MTK_SWLA_ENABLE) && defined(MTK_SWLA_WDT_RESET_TRACE) */
    /**
     * WDT interrupt to trigger exception flow, if system hang.
     **/
    wdt_init.mode = HAL_WDT_MODE_INTERRUPT;
#endif /* #if defined(MTK_SWLA_ENABLE) && defined(MTK_SWLA_WDT_RESET_TRACE) */
    wdt_init.seconds = 15;
    hal_wdt_init(&wdt_init);
    hal_wdt_register_callback(wdt_timeout_handle);
#ifdef MTK_SYSTEM_HANG_CHECK_ENABLE
    hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
#else /* #ifdef MTK_SYSTEM_HANG_CHECK_ENABLE */
    hal_wdt_disable(HAL_WDT_DISABLE_MAGIC); //Disable WDT when MTK_SYSTEM_HANG_CHECK_ENABLE = n
#endif /* #ifdef MTK_SYSTEM_HANG_CHECK_ENABLE */
}
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */


/* for idle task feed wdt (DO NOT enter sleep mode)*/
void vApplicationIdleHook(void)
{
#ifdef MTK_SYSTEM_HANG_CHECK_ENABLE
#ifdef HAL_WDT_MODULE_ENABLED
    hal_wdt_feed(HAL_WDT_FEED_MAGIC);
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
#endif /* #ifdef MTK_SYSTEM_HANG_CHECK_ENABLE */
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

#endif /* #if( configSUPPORT_STATIC_ALLOCATION == 1 ) */

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
    /* Do system initialization, eg: hardware, nvdm, logging and random seed. */
    system_init();

#ifdef HAL_GPIO_MODULE_ENABLED
    bsp_ept_gpio_setting_init();
#endif /* #ifdef HAL_GPIO_MODULE_ENABLED */


#if defined(MTK_MINICLI_ENABLE)
    /* Initialize cli task to enable user input cli command from uart port.*/
    cli_def_create();
    cli_task_create();
#endif /* #if defined(MTK_MINICLI_ENABLE) */

    /* Call this function to indicate the system initialize done. */
    //SysInitStatus_Set();

#ifdef HAL_WDT_MODULE_ENABLED
    wdt_init();
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
    printf("Hello Mediatek!!\r\n\r\n");


    if (HAL_EFUSE_PKG_MODE_WBBGA == hal_efuse_get_pkg_mode()) {
#ifdef MTK_SSUSB_HOST_ENABLE
        /* Disable usb, use the cli command to init usb function */
        //mtk_usb_host_init(0);
#endif /* #ifdef MTK_SSUSB_HOST_ENABLE */

#ifdef MTK_SSUSB_GADGET_ENABLE
        /* Disable usb, use the cli command to init usb function */
        //serial_usb_init();
#endif /* #ifdef MTK_SSUSB_GADGET_ENABLE */
    }

#ifdef MTK_TFM_ENABLE
    tfm_ns_interface_iotsdk_init();
#endif /* #ifdef MTK_TFM_ENABLE */

#ifdef MTK_MT7933_CONSYS_ENABLE
    gConnsysCalLock = xSemaphoreCreateMutex();
    gConnsysRadioOnLock = xSemaphoreCreateMutex();
#if defined(BRINGUP_WIFI_ENABLE) || defined(BRINGUP_BT_ENABLE)
    mt7933_conninfra_init();
#ifdef MTK_BT_ENABLE
    //bt_driver_init();
#endif /* #ifdef MTK_BT_ENABLE */
#endif /* #if defined(BRINGUP_WIFI_ENABLE) || defined(BRINGUP_BT_ENABLE) */
#ifdef MTK_MT7933_CONSYS_WIFI_ENABLE
    /* mask out connsys on by default beofre ES for build sanity check */
    /* wifi_init_task(); */
#endif /* #ifdef MTK_MT7933_CONSYS_WIFI_ENABLE */
#endif /* #ifdef MTK_MT7933_CONSYS_ENABLE */

#ifdef HAL_GCPU_MODULE_ENABLED
    gcpu_init();
#endif /* #ifdef HAL_GCPU_MODULE_ENABLED */

    //#ifdef MTK_UT_ENABLE
    //    if (xTaskCreate((TaskFunction_t)ut_main, "utTask", 1024, NULL, utTask_PRIORITY, NULL) != pdPASS)
    //    {
    //        printf("Task creation failed!.\r\n");
    //        while (1);
    //    }
    //#endif

#ifdef MTK_HIFI4DSP_ENABLE
    extern void adsp_ipi_init(void);
    adsp_ipi_init();
#endif /* #ifdef MTK_HIFI4DSP_ENABLE */

#ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE
    printf("AUDIO DRIVER INIT\n");
    extern void audio_init(void);
    audio_init();
#endif /* #ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE */

#ifdef MTK_HIFI4DSP_ENABLE
    /*
    * remove adsp_init into async_load_hifixdsp_bin_and_run.
    * Because adsp starts to work only when audio need it.
    */
    //adsp_init();
#endif /* #ifdef MTK_HIFI4DSP_ENABLE */

#ifdef MTK_BT_ENABLE
    extern void bt_app_common_init(void);
    bt_app_common_init();
#endif /* #ifdef MTK_BT_ENABLE */

    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for (;;);
}


