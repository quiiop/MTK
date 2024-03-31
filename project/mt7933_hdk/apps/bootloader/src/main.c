/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */


/****************************************************************************
 *
 * HEADER FILES
 *
 ****************************************************************************/


/* standard header */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* HAL */
#include "hal_gcpu_internal.h"
#ifdef HAL_GPIO_MODULE_ENABLED
#include "bsp_gpio_ept_config.h"
#endif
#ifdef HAL_GPT_MODULE_ENABLED
#include "hal_gpt_internal.h"
#endif

/* private header */
#include "bl_cache_ops.h"
#include "bl_fota.h"
#include "bl_image.h"
#include "bl_mperf.h"
#include "bl_sec.h"
#include "bl_util.h"
#include "driver_api.h"
#include "sys_init.h"

#ifdef MTK_FOTA_V3_ENABLE
#include "fota_flash_config.h"
#endif
#include "hal.h"
#include "hw_uart.h"
#include "memory_map.h"
#include "ps_loadsave.h"
#include "xmodem.h"

#include "hal_boot.h"
#include "hal_spm.h"
#include "memory_attribute.h"

#ifdef MTK_BL_PSRAM_ENABLE
#include "hal_asic_mpu.h"
#include "hal_asic_mpu_internal.h"
#include "hal_psram_internal.h"

extern p_asic_mpu_config_callbak g_psram_cb;
#endif


#if defined(MTK_RELEASE_MODE) && defined(MTK_MFG_VERIFY)
#include "bl_mfg.h"
#endif

/****************************************************************************
 *
 * FORWARD DECLARATIONS
 *

 ****************************************************************************/


int  main(void);
void ResetISR(void);
void ExceptionHandler(void);


/****************************************************************************
 *
 * TYPE DECLARATION
 *
 ****************************************************************************/


typedef struct copy_table {
    uint32_t const *src;
    uint32_t *dest;
    uint32_t  wlen;
} copy_table_t;


typedef struct zero_table {
    uint32_t *dest;
    uint32_t  wlen;
} zero_table_t;


#if defined(__GNUC__)

typedef void(*pFunc)(void);


/****************************************************************************
 *
 * GLOBAL VARIABLES
 *
 ****************************************************************************/


/*
 * Exception Vectors
 */
__attribute__((section(".except_vectors"))) const pFunc __isr_vector[32] = {
    (pFunc)STACK_TOP,                   // Master stack pointer(MSP)
    ResetISR,                           // Reset handler
    ExceptionHandler,                   // -14, NMI
    ExceptionHandler,                   // -13, Hard fault
    ExceptionHandler,                   // -12, MPU fault
    ExceptionHandler,                   // -11, Bus fault
    ExceptionHandler,                   // -10, Usage fault
    ExceptionHandler,                   // -9, Secure fault
    0,                                  // Reserved
    0,                                  // Reserved
    0,                                  // Reserved
    ExceptionHandler,                   // -5, SVCall
    ExceptionHandler,                   // -4, Debug monitor
    0,                                  // Reserved
    ExceptionHandler,                   // -2, PendSV
    0,                                  // -1, Systick
    /* ... */
};


/****************************************************************************
 *
 * PRIVATE FUNCTIONS
 *
 ****************************************************************************/


static void _cache_init(void)
{
#ifdef MTK_BL_CACHE_ENABLE
    unsigned int i;

    static const hal_cache_region_config_t regions[] = {
        { 0x08000000, 0x08000000 }, // SYSRAM 128MB
        { 0x10000000, 0x08000000 }, // PSRAM  128MB
        { 0x18000000, 0x08000000 }  // FLASH  128MB
    };
#define REGIONS (sizeof(regions)/sizeof(hal_cache_region_config_t))

    (void)hal_cache_deinit();
    hal_cache_init();
    hal_cache_set_size(HAL_CACHE_SIZE_32KB);

    for (i = 0; i < REGIONS; i++) {
        hal_cache_region_config(i, &regions[i]);
        hal_cache_region_enable(i);
    }

    hal_cache_enable();
#endif
}


/****************************************************************************
 *
 * CORTEX-M33 MPU
 *
 ****************************************************************************/


static void cpu_mpu_init(void)
{
    const ARM_MPU_Region_t mpu_table[] = {
        //                     BASE          SH              RO   NP   XN
        //                     LIMIT         ATTR
        {
            .RBAR = ARM_MPU_RBAR(0x00000000UL, ARM_MPU_SH_NON, 1UL, 0UL, 1UL),
            .RLAR = ARM_MPU_RLAR(0x00010000UL, 0UL)
        },
        {
            .RBAR = ARM_MPU_RBAR(0x08000000UL, ARM_MPU_SH_NON, 0UL, 1UL, 0UL),
            .RLAR = ARM_MPU_RLAR(0x1FFFFFE0UL, 0UL)
        },
        {
            .RBAR = ARM_MPU_RBAR(0x20000000UL, ARM_MPU_SH_NON, 0UL, 1UL, 1UL),
            .RLAR = ARM_MPU_RLAR(0x7FFFFFE0UL, 1UL)
        },
        {
            .RBAR = ARM_MPU_RBAR(0x80000000UL, ARM_MPU_SH_NON, 0UL, 1UL, 0UL),
            .RLAR = ARM_MPU_RLAR(0xAFFFFFE0UL, 0UL)
        }
    };
#define MPU_TABLE_COUNT (sizeof(mpu_table) / sizeof(ARM_MPU_Region_t))

    __disable_irq();

    // secure mode

    ARM_MPU_Disable();

    // Set Attr 0
    ARM_MPU_SetMemAttr(0UL,
                       ARM_MPU_ATTR( /* Normal memory */
                           ARM_MPU_ATTR_MEMORY_(1UL, 0UL, 1UL, 1UL),
                           ARM_MPU_ATTR_MEMORY_(1UL, 0UL, 1UL, 1UL)));

    // Set Attr 1
    ARM_MPU_SetMemAttr(1UL,
                       ARM_MPU_ATTR( /* Device memory */
                           ARM_MPU_ATTR_DEVICE,
                           ARM_MPU_ATTR_DEVICE_nGnRnE));

    ARM_MPU_Load(0, &mpu_table[0], MPU_TABLE_COUNT);

    MPU->CTRL |= MPU_CTRL_PRIVDEFENA_Msk;

    ARM_MPU_Enable(MPU->CTRL);

    // non-secure mode

    ARM_MPU_Disable_NS();

    // Set Attr 0
    ARM_MPU_SetMemAttr_NS(0UL,
                          ARM_MPU_ATTR( /* Normal memory */
                              ARM_MPU_ATTR_MEMORY_(1UL, 0UL, 1UL, 1UL),
                              ARM_MPU_ATTR_MEMORY_(1UL, 0UL, 1UL, 1UL)));

    // Set Attr 1
    ARM_MPU_SetMemAttr_NS(1UL,
                          ARM_MPU_ATTR( /* Device memory */
                              ARM_MPU_ATTR_DEVICE,
                              ARM_MPU_ATTR_DEVICE_nGnRnE));

    ARM_MPU_Load_NS(0, &mpu_table[0], MPU_TABLE_COUNT);

    MPU_NS->CTRL |= MPU_CTRL_PRIVDEFENA_Msk;

    ARM_MPU_Enable_NS(MPU_NS->CTRL);

    __enable_irq();
}


/*
 * Specify section (.reset_isr) of this routine and keep this section right
 * behind vector table in linker script.
 *
 * Specify attribute naked to not generate function prologue because this
 * function should never return.
 */
__attribute__((section(".reset_isr"), naked)) void ResetISR(void)
{
    /* Set stack range */
    __set_MSP(STACK_TOP);
    __set_MSPLIM(STACK_LIMIT);

    __disable_irq();

#ifdef HAL_GPT_MODULE_ENABLED
    volatile uint64_t current_count = gpt_get_current_count();
    hal_boot_set_bootrom_duration(bl_util_gpt_to_ms(current_count));
#endif

    cpu_mpu_init();

    if (SPM_IS_REBOOT()) {
        bl_last_image_boot();
    }

    /* Set VTOR */
    SCB->VTOR = (uint32_t) & (__isr_vector[0]);

    _cache_init();

    /* copy code/data if XIP */
    for (copy_table_t const *p = (copy_table_t const *)COPY_TABLE_START;
         p < (copy_table_t const *)COPY_TABLE_END; p++) {
        for (uint32_t i = 0; i < (p->wlen >> 2); i++) {
            p->dest[i] = p->src[i];
        }
    }

    /* clear bss */
    for (zero_table_t const *p = (zero_table_t const *)ZERO_TABLE_START;
         p < (zero_table_t const *)ZERO_TABLE_END; p++) {
        for (uint32_t i = 0; i < (p->wlen >> 2); i++) {
            p->dest[i] = 0;
        }
    }

    /* init uart as early as possible */
    hw_uart_init();

    /* Capture Exceptions  */
    SCB->SHCSR = SCB_SHCSR_USGFAULTENA_Msk |
                 SCB_SHCSR_BUSFAULTENA_Msk |
                 SCB_SHCSR_MEMFAULTENA_Msk;

    __enable_irq();

    /*
     * Branch to main() which is on SYSRAM. but we can't just call main() directly.
     *
     * That is because compiler will generate PIC code (mentioned as above comment) for main() call statment,
     * and it will jump to main() in flash(XIP).
     *
     * So we force a long jump by assembly here.
     */

    __asm volatile(
        "       ldr   r0, =main \n\t"
        "       bx    r0        \n\t");

}
#endif


#ifdef MTK_BL_RESTORE_DEFAULT_ENABLE
static int sys_restore_default_request(void)
{
#define RTC_INT_GPIO_PIN    HAL_GPIO_0       /* HDK board: WS3367, MT7687 Main Board-V22 */
#define RTC_INT_GPIO_PMUX   HAL_GPIO_0_GPIO0 /* HDK board: WS3367, MT7687 Main Board-V22 */

    hal_gpio_status_t   r_gpio;
    hal_gpio_data_t     data;

    r_gpio = hal_gpio_init(RTC_INT_GPIO_PIN);
    r_gpio = hal_gpio_set_direction(RTC_INT_GPIO_PIN, HAL_GPIO_DIRECTION_INPUT);
    r_gpio = hal_gpio_pull_down(RTC_INT_GPIO_PIN);
    r_gpio = hal_gpio_deinit(RTC_INT_GPIO_PIN);

    r_gpio = hal_gpio_get_input(RTC_INT_GPIO_PIN, &data);

    return (data != 0);
}

static int sys_restore_default_do(fota_flash_t *t)
{
    int                 addr;
    int                 i;

    hal_flash_init();

    for (i = 0; i < t->table_entries; i++) {
        if (t->table[i].id != FOTA_PARITION_NV) {
            continue;
        }

        addr = t->table[i].address;

        while ((addr + 4096) <= (t->table[i].address + t->table[i].length)) {
            hal_flash_erase(addr, HAL_FLASH_BLOCK_4K);
            addr += 4096;
        }
    }

    hal_flash_deinit();
}
#else
#define sys_restore_default_request()   (0)
#define sys_restore_default_do(__p__)
#endif /* MTK_BL_RESTORE_DEFAULT_ENABLE */


#ifdef MTK_BL_BOOT_MENU_ENABLE
#define WAIT_USER_INPUT_SEC (2)    /* wait 2 secs for user input */

#include "ps_loadsave_tables.c"

static void user_select(void)
{
    int loop_count;
    int ch;
    int delay_ms = 100;
#if 0
    int i;
    uint32_t dest = 0;
#endif

    while (1) {
#if 0
        hw_uart_printf("1. Burn  Bootloader(0x%X)\n", LOADER_BASE);
        hw_uart_printf("2. Burn  RTOS      (0x%X)\n", RTOS_BASE);
        hw_uart_printf("3. Burn  WIFI PATCH(0x%X)\n", WIFI_PATCH_BASE);
        hw_uart_printf("4. Burn  WIFI FW   (0x%X)\n", WIFI_BASE);
        hw_uart_printf("5. Burn  WIFI_EXT  (0x%X)\n", WIFI_EXT_BASE);
        hw_uart_printf("6. Burn  BT        (0x%X)\n", BT_BASE);
        hw_uart_printf("7. Burn  DSP       (0x%X)\n", DSP_BASE);
        hw_uart_printf("8. Burn  TFM       (0x%X)\n", TFM_BASE);
        hw_uart_printf("9. Erase NVDM      (0x%X)\n", NVDM_BASE);
        hw_uart_printf("c. Boot  RTOS      (0x%X)\n", RTOS_BASE);
        hw_uart_printf("d. Burn  OTA image (0x%X)\n", FOTA_BASE);
        hw_uart_printf("z. CLI\n");
#endif

        loop_count = WAIT_USER_INPUT_SEC * 1000 / delay_ms;

        /* wait for user input */
        while (loop_count > 0 && (ch = hw_uart_getc()) == -1) {
            hal_gpt_delay_ms(100);
            loop_count--;
            if (!(loop_count % 10)) {
                hw_uart_printf("\r  %d   ", loop_count / 10);
            }
            ch = 'c';
        }

        hw_uart_printf("\r\n");
        hw_uart_printf("Your choose %c\r\n", ch);

        /* dispatch user's input */
        switch (ch) {
            case 'z': {
                    void cli_create(void);
                    extern void cli_task(void);
                    cli_create();
                    cli_task();
                    break;
                }
#if 0
            case '1':
                // TODO:
                //bl_fota_xmodem_download(FOTA_PARITION_LOADER, LOADER_LENGTH);
                hw_uart_puts("Erase flash firstly, please wait\n");
                /* erase is slow, put here temporarily  */
                for (i = 0; i < (LOADER_LENGTH / 0x1000); i++) {
                    hw_uart_printf("\r %d", i + 1);
                    hal_flash_erase(LOADER_BASE + i * 0x1000, HAL_FLASH_BLOCK_4K);
                }
                hw_uart_puts("\nPlease select the firmware to send.\n");
                dest = LOADER_BASE;
                xmodem_block_rx(NULL, (uint8_t *)&dest, LOADER_LENGTH, NULL);
                break;

            case '2':
                // TODO:
                //bl_fota_xmodem_download(FOTA_PARITION_CM4, CM4_CODE_LENGTH);
                hw_uart_puts("Erase flash firstly, please wait\n");
                bl_image_erase_4kb_aligned(ROM_REGION_RTOS);
                hw_uart_puts("\nPlease select the firmware to send.\n");
                dest = RTOS_BASE;
                xmodem_block_rx(NULL, (uint8_t *)&dest, RTOS_LENGTH, NULL);
                break;
            case '3': //WIFI PATCH
                //fotu_do(FOTA_PARITION_CM4, CM4_CODE_LENGTH);
                // TODO:
                hw_uart_puts("Erase flash firstly, please wait\n");
                /* erase is slow, put here temporarily  */
                for (i = 0; i < (WIFI_PATCH_LENGTH / 0x1000); i++) {
                    hw_uart_printf("\r %d", i + 1);
                    hal_flash_erase(WIFI_PATCH_BASE + i * 0x1000, HAL_FLASH_BLOCK_4K);
                }
                hw_uart_puts("\nPlease select the firmware to send.\n");
                dest = WIFI_PATCH_BASE;
                xmodem_block_rx(NULL, (uint8_t *)&dest, WIFI_PATCH_LENGTH, NULL);
                break;
            case '4': //WIFI
                //bl_fota_xmodem_download(FOTA_PARITION_NCP, N9_RAMCODE_LENGTH);
                hw_uart_puts("Erase flash firstly, please wait\n");
                /* erase is slow, put here temporarily  */
                for (i = 0; i < (WIFI_LENGTH / 0x1000); i++) {
                    hw_uart_printf("\r %d", i + 1);
                    hal_flash_erase(WIFI_BASE + i * 0x1000, HAL_FLASH_BLOCK_4K);
                }
                hw_uart_puts("\nPlease select the firmware to send.\n");
                dest = WIFI_BASE;
                xmodem_block_rx(NULL, (uint8_t *)&dest, WIFI_LENGTH, NULL);
                break;
            case '5': //WIFI_EXT
                hw_uart_puts("Erase flash firstly, please wait\n");
                /* erase is slow, put here temporarily  */
                for (i = 0; i < (WIFI_EXT_LENGTH / 0x1000); i++) {
                    hw_uart_printf("\r %d", i + 1);
                    hal_flash_erase(WIFI_EXT_BASE + i * 0x1000, HAL_FLASH_BLOCK_4K);
                }
                hw_uart_puts("\nPlease select the firmware to send.\n");
                dest = WIFI_EXT_BASE;
                xmodem_block_rx(NULL, (uint8_t *)&dest, WIFI_EXT_LENGTH, NULL);
                break;
            case '6': //BT
                hw_uart_puts("Erase flash firstly, please wait\n");
                /* erase is slow, put here temporarily  */
                for (i = 0; i < (BT_LENGTH / 0x1000); i++) {
                    hw_uart_printf("\r %d", i + 1);
                    hal_flash_erase(BT_BASE + i * 0x1000, HAL_FLASH_BLOCK_4K);
                }
                hw_uart_puts("\nPlease select the firmware to send.\n");
                dest = BT_BASE;
                xmodem_block_rx(NULL, (uint8_t *)&dest, BT_LENGTH, NULL);
                break;

            case '7': //DSP
                hw_uart_puts("Erase flash firstly, please wait\n");
                /* erase is slow, put here temporarily  */
                for (i = 0; i < (DSP_LENGTH / 0x1000); i++) {
                    hw_uart_printf("\r %d", i + 1);
                    hal_flash_erase(DSP_BASE + i * 0x1000, HAL_FLASH_BLOCK_4K);
                }
                hw_uart_puts("\nPlease select the firmware to send.\n");
                dest = DSP_BASE;
                xmodem_block_rx(NULL, (uint8_t *)&dest, DSP_LENGTH, NULL);
                break;

            case '8': //TFM
                hw_uart_puts("Erase flash firstly, please wait\n");
                /* erase is slow, put here temporarily  */
                for (i = 0; i < (TFM_LENGTH / 0x1000); i++) {
                    hw_uart_printf("\r %d", i + 1);
                    hal_flash_erase(TFM_BASE + i * 0x1000, HAL_FLASH_BLOCK_4K);
                }
                hw_uart_puts("\nPlease select the firmware to send.\n");
                dest = TFM_BASE;
                xmodem_block_rx(NULL, (uint8_t *)&dest, TFM_LENGTH, NULL);
                break;
            case '9': //NVDM
                hw_uart_puts("Erase flash firstly, please wait\n");
                /* erase is slow, put here temporarily  */
                for (i = 0; i < (NVDM_LENGTH / 0x1000); i++) {
                    hw_uart_printf("\r %d", i + 1);
                    hal_flash_erase(NVDM_BASE + i * 0x1000, HAL_FLASH_BLOCK_4K);
                }
                //hw_uart_puts("\nPlease select the firmware to send.\n");
                //xmodem_block_rx(NULL, (uint8_t *)&NVDM_BASE, NVDM_LENGTH, NULL);
                break;

            case 'D':
            case 'd':
                hw_uart_puts("Erase flash firstly, please wait\n");
                /* erase is slow, put here temporarily  */
                for (i = 0; i < (FOTA_LENGTH / 0x1000); i++) {
                    hw_uart_printf("\r %d", i + 1);
                    hal_flash_erase(FOTA_BASE + i * 0x1000, HAL_FLASH_BLOCK_4K);
                }
                hw_uart_puts("\nPlease select the firmware to send.\n");
                dest = FOTA_BASE;
                xmodem_block_rx(NULL, (uint8_t *)&dest, FOTA_LENGTH, NULL);
                break;
#endif
            case 'C':
            case 'c':
                return;
                break;

            default:
                break;
        }
    }
}
#else
#define user_select() do { } while (0)
#endif /* ifndef MTK_BL_BOOT_MENU_ENABLE */


static bl_status_t _bl_watchdog_init(void)
{
    hal_wdt_config_t    wdt_config;
    hal_wdt_status_t    ws;
    bl_status_t         status = BL_STATUS_OK;

    wdt_config.mode    = HAL_WDT_MODE_RESET;
    wdt_config.seconds = 30;

    ws = hal_wdt_init(&wdt_config);
    exit_with_status(ws != HAL_WDT_STATUS_OK, BL_STATUS_WDOG_HW_FAIL, ws, 0);

#ifdef MTK_BL_WDT_ENABLE
    /* enable 30 seconds watchdog timeout. */
    ws = hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
    exit_with_status(ws != HAL_WDT_STATUS_OK, BL_STATUS_WDOG_EN_FAIL, ws, 0);
#else
    /* disable 30 seconds watchdog timeout. */
    ws = hal_wdt_disable(HAL_WDT_DISABLE_MAGIC);
    exit_with_status(ws != HAL_WDT_STATUS_OK, BL_STATUS_WDOG_DIS_FAIL, ws, 0);
#endif

_exit:
    return status;
}


static bl_status_t _bl_flash_init(void)
{
    hal_flash_status_t  fs;
    bl_status_t         status = BL_STATUS_OK;

    fs = hal_flash_init();
    exit_with_status(fs != HAL_FLASH_STATUS_OK, BL_STATUS_FLASH_HW_FAIL, fs, 0);

_exit:
    return status;
}


static void _boot_download_mode(hal_boot_source_t boot_source)
{
    switch (boot_source) {
        case HAL_BOOT_SOURCE_UART:
            // init uart - download agent
            break;
        case HAL_BOOT_SOURCE_USB:
            // init usb - download agent
            break;
        case HAL_BOOT_SOURCE_SDIO:
        // SDIO is xboot
        default:
            //while (1) ;
            break;
    }
}


static bl_status_t _bl_crypto_init(void)
{
    bl_status_t         status = BL_STATUS_OK;

#ifdef MTK_SECURE_BOOT_ENABLE
    hal_clock_status_t  cs;
    int                 gs;

    cs = hal_clock_enable(HAL_CLOCK_CG_GCPU);
    exit_with_status(cs != HAL_CLOCK_STATUS_OK, BL_STATUS_SEC_CLK_FAIL, cs, 0);

    cs = hal_clock_enable(HAL_CLOCK_CG_ECC);
    exit_with_status(cs != HAL_CLOCK_STATUS_OK, BL_STATUS_ECC_CLK_FAIL, cs, 0);

    gs = gcpu_init();
    exit_with_status(gs < 0, BL_STATUS_SEC_HW_FAIL, gs, 0);

_exit:
#endif

    return status;
}


static bl_status_t _bl_pinmux_init(void)
{
#ifdef HAL_GPIO_MODULE_ENABLED
    bsp_ept_gpio_setting_init();
#endif
    return BL_STATUS_OK;
}

#ifdef MTK_BL_PSRAM_ENABLE
static void asic_mpu_psram_init_cb(void)
{
    unsigned int region;
    unsigned int addr;

    // disable psram protection
    for (region = 0; region < REGION_NUM; region++) {
        addr = MPU_OFFSET(HAL_ASIC_MPU_TYPE_PSRAM, R0_APC_OFFSET) + (region * 0x4);
        WRITE_REG(0, addr);
    }
}
#endif


static bl_status_t _bl_extmem_init(void)
{
    bl_status_t         status = BL_STATUS_OK;
#ifdef MTK_BL_PSRAM_ENABLE
    g_psram_cb = asic_mpu_psram_init_cb;
    hal_psram_status_t  ps = hal_psram_init();
    exit_with_status(ps != HAL_PSRAM_STATUS_SUCCESS, BL_STATUS_MEM_HW_FAIL, ps, 0);

_exit:
#endif
    return status;
}


static bl_status_t _bl_firmware_check(void)
{
    bl_status_t status = BL_STATUS_OK;

#ifdef MTK_SECURE_BOOT_ENABLE
    status = bl_sec_verify_all(bl_region_get_table(),
                               bl_image_bootable_add);
#endif

    return status;
}


static bl_status_t _bl_app_boot(void)
{
#ifdef MTK_TFM_ENABLE
    return bl_image_boot(ROM_REGION_TFM_A);
#else
    return bl_image_boot(ROM_REGION_RTOS);
#endif
}


ATTR_USED int main(void)
{
    rom_region_id_t     image_id;
    hal_boot_source_t   boot_source;

    bl_status_init();

    system_init();

    hal_spm_init();

    if (BL_STATUS_OK != _bl_crypto_init()  ) goto _exit;

    bl_mperf_init();

    bl_mperf_profile_tag(&g_mperf_profile, "00");

    if (BL_STATUS_OK != _bl_pinmux_init()  ) goto _exit;

    boot_source = hal_boot_get_boot_source();
    if (boot_source != HAL_BOOT_SOURCE_NORMAL)
        _boot_download_mode(boot_source);

    if (BL_STATUS_OK != _bl_extmem_init()  ) goto _exit;

    bl_mperf_profile_tag(&g_mperf_profile, "ua");

    if (BL_STATUS_OK != _bl_watchdog_init()) goto _exit;

    if (BL_STATUS_OK != _bl_flash_init())    goto _exit;

    hw_uart_printf("loader init%c\n", is_in_ram(main, 1) ? '.' : ' ');

    bl_mperf_profile_tag(&g_mperf_profile, "mu");

    if (sys_restore_default_request()) {
        hw_uart_printf("restore default\n");
        sys_restore_default_do(&fota_flash_default_config);
    }

    user_select();

    bl_fota_boot_status_sync();

    bl_mperf_profile_tag(&g_mperf_profile, "bx");

    if (BL_STATUS_OK != bl_fota_query_active_image(&image_id)) goto _exit;

    if (BL_STATUS_OK != _bl_firmware_check()) goto _exit;

#if defined(MTK_RELEASE_MODE) && defined(MTK_MFG_VERIFY)
    bl_mfg_boot();
#endif

    _bl_app_boot();

_exit:

    bl_status_print();
    while (1) ;

    return 0;
}


