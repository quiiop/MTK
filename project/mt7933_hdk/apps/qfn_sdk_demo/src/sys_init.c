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

#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Hal includes. */
#include "hal.h"
#ifdef HAL_DCXO_MODULE_ENABLED
#include "hal_dcxo.h"
#endif

#ifdef MTK_NVDM_ENABLE
#include "nvdm.h"
#endif

#include "syslog.h"
#include "io_def.h"
//#include "bsp_gpio_ept_config.h"

#include "memory_map.h"
#include "memory_attribute.h"

#include "hal_asic_mpu.h"
#include "hal_spm.h"
#include "hal_psram_internal.h"
#include "hal_gpt.h"

#ifdef HAL_CACHE_MODULE_ENABLED
#include "hal_cache.h"
#endif

#ifdef HAL_CLOCK_MODULE_ENABLED
#include "hal_clock.h"
#endif /* HAL_CLOCK_MODULE_ENABLED */

#ifdef MTK_HIFI4DSP_ENABLE
#include "mtk_hifixdsp_common.h"
#endif

#ifdef HAL_RTC_MODULE_ENABLED
#include "hal_rtc.h"
#include "hal_rtc_internal.h"
#endif

extern uint32_t chip_hardware_code(void);
extern uint32_t chip_eco_version(void);

#if defined(MTK_MINICLI_ENABLE)
#include "cli_def.h"
#endif

#ifdef MTK_SWLA_ENABLE
#include "swla.h"
#endif /* MTK_SWLA_ENABLE */

#ifdef MTK_NVDM_ENABLE
void nvdm_module_init(void);
#endif

#ifdef MTK_MEMORY_EXTRACTOR_ENABLE
#include "memextract.h"
#endif /* MTK_MEMORY_EXTRACTOR_ENABLE */

void user_check_default_value(void);

#if defined(MTK_MINI_DUMP_ENABLE) && defined(MTK_NVDM_ENABLE)
void plat_exception_handler_init(void);
#endif

#ifndef MTK_DEBUG_LEVEL_NONE

log_create_module(main, PRINT_LEVEL_ERROR);
log_create_module(WIFI, PRINT_LEVEL_INFO);
log_create_module(minisupp, PRINT_LEVEL_INFO);

LOG_CONTROL_BLOCK_DECLARE(main);
LOG_CONTROL_BLOCK_DECLARE(common);
LOG_CONTROL_BLOCK_DECLARE(hal);
LOG_CONTROL_BLOCK_DECLARE(WIFI);
LOG_CONTROL_BLOCK_DECLARE(minisupp);

log_control_block_t *syslog_control_blocks[] = {
    &LOG_CONTROL_BLOCK_SYMBOL(main),
    &LOG_CONTROL_BLOCK_SYMBOL(common),
    &LOG_CONTROL_BLOCK_SYMBOL(hal),
    &LOG_CONTROL_BLOCK_SYMBOL(WIFI),
    &LOG_CONTROL_BLOCK_SYMBOL(minisupp),
    NULL
};
#endif

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
#if defined(MTK_F32K_SW_DETECT_ENABLED)
static bool prvF32kIsExist(void)
{
    uint32_t before_delay_32k = 0, after_delay_32k = 0, before_delay_1m = 0, after_delay_1m = 0, time_elapsed = 0;
    /* GPT timer's setting need to take about 2 ~ 3 T to wait for hardware sync after the setting had been changed */
    hal_gpt_delay_us(100);
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &before_delay_32k);
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &before_delay_1m);
    /* Polling 32k gpt timer conuter, if rtc XOSC exist, thus gpt counter would be increased */
    while (time_elapsed <= 100) {
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &after_delay_32k);
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &after_delay_1m);
        time_elapsed = after_delay_1m - before_delay_1m;

        if ((after_delay_32k - before_delay_32k))
            return true;
    }
    return false;
}
#endif // MTK_F32K_SW_DETECT_ENABLED

#ifdef HAL_CLOCK_MODULE_ENABLED
static void SystemClock_Config(void)
{
    hal_clock_init();
    SystemCoreClockUpdate();
}
#endif // HAL_CLOCK_MODULE_ENABLED

#ifdef HAL_CACHE_MODULE_ENABLED
/**
* @brief       This function is to initialize cache controller.
* @param[in]   None.
* @return      None.
*/
void cache_init(void)
{
    hal_cache_region_t         region, region_number;
    hal_cache_region_config_t  config, *p;

    /* Max region number is 16 */
    hal_cache_region_config_t region_cfg_tbl[] = {
        { XIP_RTOS_START,   RTOS_LENGTH    },
        { VSYSRAM_BASE,     VSYSRAM_LENGTH }, /* virtual sysram */
        { VRAM_BASE,        VRAM_LENGTH    }  /* virtual memory */
    };

    region_number = (hal_cache_region_t)(sizeof(region_cfg_tbl) / sizeof(region_cfg_tbl[0]));
    hal_cache_init();
    hal_cache_set_size(HAL_CACHE_SIZE_32KB);

    for (region = HAL_CACHE_REGION_0; region < region_number; region++) {

#ifdef MTK_UT_ENABLE
        if (region == HAL_CACHE_REGION_15) {
            printf("[Cache]: Region 15 is reserved for Cache UT. Ignore Current Configure...\r\n");
            continue;
        }
#endif

        /* cacheable address and size both MUST be 4KB aligned */
#define TRIM_4KB (0x00000FFF)
        p = &region_cfg_tbl[region];
        config.cache_region_address = p->cache_region_address & (~TRIM_4KB);
        config.cache_region_size    = p->cache_region_size +
                                      (p->cache_region_address & TRIM_4KB);

        hal_cache_region_config(region, &config);
        hal_cache_region_enable(region);
    }

    for (; region < HAL_CACHE_REGION_MAX; region++) {
        hal_cache_region_disable(region);
    }

    hal_cache_enable();
}
#endif


/**
* @brief       caculate actual bit value of region size.
* @param[in]   region_size: actual region size.
* @return      corresponding bit value of region size for MPU setting.
*/
//static uint32_t caculate_mpu_region_size(uint32_t region_size)
//{
//    uint32_t count;
//
//    if (region_size < 32) {
//        return 0;
//    }
//    for (count = 0; ((region_size  & 0x80000000) == 0); count++, region_size  <<= 1);
//    return 30 - count;
//}

/**
* @brief       This function is to initialize MPU.
* @param[in]   None.
* @return      None.
*/
ATTR_TEXT_IN_SYSRAM void mpu_init(void)
{
    //    hal_mpu_region_t region, region_number;
    //    hal_mpu_region_config_t region_config;
    //    typedef struct {
    //        uint32_t mpu_region_base_address;         /**< MPU region start address */
    //        uint32_t mpu_region_end_address;          /**< MPU region end address */
    //        hal_mpu_access_permission_t mpu_region_access_permission; /**< MPU region access permission */
    //        uint8_t mpu_subregion_mask;           /**< MPU sub region mask*/
    //        bool mpu_xn;                  /**< XN attribute of MPU, if set TRUE, execution of an instruction fetched from the corresponding region is not permitted */
    //    } mpu_region_information_t;
    //
    //#if defined (__GNUC__) || defined (__CC_ARM)
    //
    //    //TCM: VECTOR TABLE + CODE+RO DATA
    //    extern uint32_t Image$$TCM$$RO$$Base;
    //    extern uint32_t Image$$TCM$$RO$$Limit;
    //    //SYSRAM: CODE+RO DATA
    //    extern uint32_t Image$$CACHED_SYSRAM_TEXT$$Base;
    //    extern uint32_t Image$$CACHED_SYSRAM_TEXT$$Limit;
    //    //STACK END
    //    extern unsigned int Image$$STACK$$ZI$$Base[];
    //
    //#if (PRODUCT_VERSION == 7686)
    //    //RAM: CODE+RO DATA
    //    extern uint32_t Image$$CACHED_RAM_TEXT$$Base;
    //    extern uint32_t Image$$CACHED_RAM_TEXT$$Limit;
    //#endif
    //    /* MAX region number is 8 */
    //    mpu_region_information_t region_information[] = {
    //        /* mpu_region_start_address, mpu_region_end_address, mpu_region_access_permission, mpu_subregion_mask, mpu_xn */
    //        {(uint32_t) &Image$$TCM$$RO$$Base, (uint32_t) &Image$$TCM$$RO$$Limit, HAL_MPU_READONLY, 0x0, FALSE},//Vector table+TCM code+TCM rodata
    //        {(uint32_t) &Image$$CACHED_SYSRAM_TEXT$$Base, (uint32_t) &Image$$CACHED_SYSRAM_TEXT$$Limit, HAL_MPU_READONLY, 0x0, FALSE}, //SYSRAM code+SYSRAM rodata
    //        {(uint32_t) &Image$$CACHED_SYSRAM_TEXT$$Base - VRAM_BASE, (uint32_t) &Image$$CACHED_SYSRAM_TEXT$$Limit - VRAM_BASE, HAL_MPU_NO_ACCESS, 0x0, TRUE}, //Virtual memory
    //        {(uint32_t) &Image$$STACK$$ZI$$Base, (uint32_t) &Image$$STACK$$ZI$$Base + 32, HAL_MPU_READONLY, 0x0, TRUE}, //Stack end check for stack overflow
    //#if (PRODUCT_VERSION == 7686)
    //        {(uint32_t) &Image$$CACHED_RAM_TEXT$$Base, (uint32_t) &Image$$CACHED_RAM_TEXT$$Limit, HAL_MPU_READONLY, 0x0, FALSE}, //RAM code+RAM rodata
    //        {(uint32_t) &Image$$CACHED_RAM_TEXT$$Base - VRAM_BASE, (uint32_t) &Image$$CACHED_RAM_TEXT$$Limit - VRAM_BASE, HAL_MPU_NO_ACCESS, 0x0, TRUE}, //Virtual memory
    //#else
    //        {(uint32_t) 0, (uint32_t) 0x400000, HAL_MPU_NO_ACCESS, 0x0, TRUE} //Set EMI address range as no access
    //#endif
    //    };
    //
    //#elif defined (__ICCARM__)
    //
    //#pragma section = ".intvec"
    //#pragma section = ".tcm_rwdata"
    //#pragma section = ".sysram_code"
    //#pragma section = ".sysram_rodata"
    //#pragma section = "CSTACK"
    //#if (PRODUCT_VERSION == 7686)
    //#pragma section = ".ram_code"
    //#pragma section = ".ram_rodata"
    //#endif
    //
    //    /* MAX region number is 8 */
    //    mpu_region_information_t region_information[] = {
    //        /* mpu_region_start_address, mpu_region_end_address, mpu_region_access_permission, mpu_subregion_mask, mpu_xn */
    //        {(uint32_t)__section_begin(".intvec"), (uint32_t)__section_begin(".tcm_rwdata"), HAL_MPU_READONLY, 0x0, FALSE},//Vector table+TCM code+TCM rodata
    //        {(uint32_t)__section_begin(".sysram_code"), (uint32_t)__section_end(".sysram_code") + (uint32_t)__section_end(".sysram_rodata") - (uint32_t)__section_begin(".sysram_rodata"), HAL_MPU_READONLY, 0x0, FALSE},//SYSRAM code+SYSRAM rodata
    //        {(uint32_t)__section_begin(".sysram_code") - VRAM_BASE, (uint32_t)__section_end(".sysram_code") + (uint32_t)__section_end(".sysram_rodata") - (uint32_t)__section_begin(".sysram_rodata") - VRAM_BASE, HAL_MPU_NO_ACCESS, 0x0, TRUE}, //Virtual memory
    //        {(uint32_t)__section_begin("CSTACK"), (uint32_t)__section_begin("CSTACK") + 32, HAL_MPU_READONLY, 0x0, TRUE}, //Stack end check for stack overflow
    //#if (PRODUCT_VERSION == 7686)
    //        {(uint32_t)__section_begin(".ram_code"), (uint32_t)__section_end(".ram_code") + (uint32_t)__section_end(".ram_rodata") - (uint32_t)__section_begin(".ram_rodata"), HAL_MPU_READONLY, 0x0, FALSE},//RAM code+RAM rodata
    //        {(uint32_t)__section_begin(".ram_code") - VRAM_BASE, (uint32_t)__section_end(".ram_code") + (uint32_t)__section_end(".ram_rodata") - (uint32_t)__section_begin(".ram_rodata") - VRAM_BASE, HAL_MPU_NO_ACCESS, 0x0, TRUE}, //Virtual memory
    //#else
    //        {(uint32_t) 0, (uint32_t) 0x400000, HAL_MPU_NO_ACCESS, 0x0, TRUE} //Set EMI address range as no access
    //#endif
    //    };
    //
    //#endif
    //
    //    hal_mpu_config_t mpu_config = {
    //        /* PRIVDEFENA, HFNMIENA */
    //        TRUE, TRUE
    //    };
    //
    //    region_number = (hal_mpu_region_t)(sizeof(region_information) / sizeof(region_information[0]));
    //
    //    hal_mpu_init(&mpu_config);
    //    for (region = HAL_MPU_REGION_0; region < region_number; region++) {
    //        /* Updata region information to be configured */
    //        region_config.mpu_region_address = region_information[region].mpu_region_base_address;
    //        region_config.mpu_region_size = (hal_mpu_region_size_t) caculate_mpu_region_size(region_information[region].mpu_region_end_address - region_information[region].mpu_region_base_address);
    //        region_config.mpu_region_access_permission = region_information[region].mpu_region_access_permission;
    //        region_config.mpu_subregion_mask = region_information[region].mpu_subregion_mask;
    //        region_config.mpu_xn = region_information[region].mpu_xn;
    //
    //        hal_mpu_region_configure(region, &region_config);
    //        hal_mpu_region_enable(region);
    //    }
    //    /* make sure unused regions are disabled */
    //    for (; region < HAL_MPU_REGION_MAX; region++) {
    //        hal_mpu_region_disable(region);
    //    }
    //    hal_mpu_enable();
}


#ifdef HAL_ASIC_MPU_MODULE_ENABLED

#ifdef HAL_PSRAM_MODULE_ENABLED
extern p_asic_mpu_config_callbak g_psram_cb;
ATTR_TEXT_IN_SYSRAM void asic_mpu_init_psram(void)
{
    unsigned int region;

    // disable psram protection
    LOG_I(hal, "ASIC_MPU disable psram protection in callback\r\n");
    for (region = 0; region < REGION_NUM; region++) {
        hal_asic_mpu_set_region_apc(HAL_ASIC_MPU_TYPE_PSRAM, region, -1, 0);
    }
}
#endif

/**
* @brief       This function is to initialize ASIC MPU.
* @param[in]   None.
* @return      None.
*/
ATTR_TEXT_IN_SYSRAM void asic_mpu_init(void)
{
    unsigned int region;

    /*
        Because the system may not power on the psram, if the system configure asic_mpu for psram at this time, it will crash.
        The callback g_psram_cb will be called at psram power on to ensure system can configure asic_mpu for psram.
    */
#ifdef HAL_PSRAM_MODULE_ENABLED
    g_psram_cb = asic_mpu_init_psram;
#endif

    // disable all protection except for psram
    for (region = 0; region < REGION_NUM; region++) {
        hal_asic_mpu_set_region_apc(HAL_ASIC_MPU_TYPE_FLASH, region, -1, 0);
        hal_asic_mpu_set_region_apc(HAL_ASIC_MPU_TYPE_SYSRAM, region, -1, 0);
        hal_asic_mpu_set_region_apc(HAL_ASIC_MPU_TYPE_TCM, region, -1, 0);
    }
}
#endif


static void prvSetupHardware(void)
{
    /* os extend heap init */
#ifdef MTK_OS_HEAP_EXTEND
    vPortDefineHeapRegions(xHeapRegions);
#endif

    //    bsp_io_def_uart_init();

    /* cache module init */
#ifdef HAL_CACHE_MODULE_ENABLED
    //cache_init();
#endif

    /* mpu module init */
#ifdef HAL_MPU_MODULE_ENABLED
    mpu_init();
#endif

    /* flash module init */
#ifdef HAL_FLASH_MODULE_ENABLED
    hal_flash_init();
#ifdef MTK_TFM_ENABLE
    hal_flash_set_base_address(XIP_BL_START);
#endif
#endif

    /* NVIC module init */
#ifdef HAL_NVIC_MODULE_ENABLED
    hal_nvic_init();
#endif

    bsp_io_def_uart_init();

    /* PMU module init */
#ifdef HAL_PMU_MODULE_ENABLED
    hal_pmu_init();
#endif

#ifdef MTK_HIFI4DSP_ENABLE
    adsp_init();
#endif
    //#ifdef HAL_DCXO_MODULE_ENABLED
    //    hal_dcxo_init();
    //#endif
#ifdef HAL_RTC_MODULE_ENABLED
    hal_rtc_init();
#endif // HAL_RTC_MODULE_ENABLED
#if defined(MTK_F32K_SW_DETECT_ENABLED)
    if (!prvF32kIsExist()) {
        /* Due to F32K's clock mux is glitch-free clock mux, this mux need synchronize signal
         * before switch mux, therefore it need to switch RTC source back to EOSC, then F32K
         * can be switched to internal 32K.
         */
        hal_rtc_osc32_sel(RTC_OSC_SRC_EOSC);
        hal_clock_mux_select(HAL_CLOCK_SEL_F32K, CLK_F32K_CLKSEL_XTAL_DIV_32K);
    }
#endif // MTK_F32K_SW_DETECT_ENABLED
}

#if !defined(MTK_DEBUG_LEVEL_NONE) && !defined(MTK_HAL_PLAIN_LOG_ENABLE) && defined(MTK_NVDM_ENABLE)
static void syslog_config_save(const syslog_config_t *config)
{
    char *syslog_filter_buf;
    nvdm_status_t status;

    syslog_filter_buf = (char *)pvPortMalloc(SYSLOG_FILTER_LEN);
    configASSERT(syslog_filter_buf != NULL);
    syslog_convert_filter_val2str((const log_control_block_t **)config->filters, syslog_filter_buf);
    status = nvdm_write_data_item("common", "syslog_filters", \
                                  NVDM_DATA_ITEM_TYPE_STRING, (const uint8_t *)syslog_filter_buf, strlen(syslog_filter_buf));
    if (status != NVDM_STATUS_OK) {
        LOG_E(common, "[syslog]: syslog_filters write fail!\r\n");
    }
    vPortFree(syslog_filter_buf);
}

static uint32_t syslog_config_load(syslog_config_t *config)
{
    uint32_t sz = SYSLOG_FILTER_LEN;
    char *syslog_filter_buf;
    nvdm_status_t status;

    syslog_filter_buf = (char *)pvPortMalloc(SYSLOG_FILTER_LEN);
    configASSERT(syslog_filter_buf != NULL);
    status = nvdm_read_data_item("common", "syslog_filters", (uint8_t *)syslog_filter_buf, &sz);
    if (status != NVDM_STATUS_OK) {
        LOG_E(common, "[syslog]: syslog_filters read fail!\r\n");
        vPortFree(syslog_filter_buf);
        return -1;
    }

    syslog_convert_filter_str2val(config->filters, syslog_filter_buf);
    vPortFree(syslog_filter_buf);

    return 0;
}
#endif


/* Pre-Init Before PSRAM Data Move */
void system_preinit(void)
{
#ifdef HAL_CLOCK_MODULE_ENABLED
    /* SystemClock Config */
    SystemClock_Config();
#endif // HAL_CLOCK_MODULE_ENABLED

    /* asic mpu module init */
#ifdef HAL_ASIC_MPU_MODULE_ENABLED
    asic_mpu_init();
#endif

    /* psram module init */
#ifdef HAL_PSRAM_MODULE_ENABLED
    hal_psram_init();
#endif
}


void system_init(void)
{
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_init();
#endif

    /* Configure the hardware ready to run the test. */
    prvSetupHardware();

#ifdef MTK_NVDM_ENABLE
    /* nvdm init */
    nvdm_init();

    /* nvdm module init */
    nvdm_module_init();
#endif

#if !defined(MTK_DEBUG_LEVEL_NONE) && !defined(MTK_HAL_PLAIN_LOG_ENABLE)
#ifdef MTK_NVDM_ENABLE
    log_init(syslog_config_save, syslog_config_load, syslog_control_blocks);
#else
    log_init(NULL, NULL, syslog_control_blocks);
#endif
#endif

#if defined(MTK_MINI_DUMP_ENABLE) && defined(MTK_NVDM_ENABLE)
    plat_exception_handler_init();
#endif

#ifdef MTK_MEMORY_EXTRACTOR_ENABLE
    memextract_init();
#endif /* MTK_MEMORY_EXTRACTOR_ENABLE */

#ifdef MTK_SWLA_ENABLE
    SLA_Enable();
#endif /* MTK_SWLA_ENABLE */

    LOG_I(common, "system initialize done.\r\n");
}


