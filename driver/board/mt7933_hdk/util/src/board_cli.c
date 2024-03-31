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


#include <stdio.h>
#include <stdint.h>
#include <string.h>
//#include "os.h"

#include "toi.h"
#include "cli.h"
#include "hal.h"
#include "hal_sys.h"
#include "hal_cache.h"
//#include "connsys_util.h"
#include "verno.h"

#ifdef HAL_CLOCK_MODULE_ENABLED
#include "hal_clk.h"
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */

#ifdef HAL_GPT_MODULE_ENABLED
#include "hal_gpt.h"
#endif /* #ifdef HAL_GPT_MODULE_ENABLED */
#include "FreeRTOS.h"
#include "task.h"
#if defined(MTK_RELEASE_MODE) && (MTK_RELEASE_MODE==MTK_M_MFG)
extern int _rom_mfg_start[];
extern int _xip_mfg_addr[];
#ifndef MFG_MAGIC
#define MFG_MAGIC                   0x4D464742
#endif /* #ifndef MFG_MAGIC */
#define IS_MFG_MAGIC( _v ) ( ( (*(volatile uint32_t *)(_v + 2)) & (*(volatile uint32_t *)(_v + 3)) ) == MFG_MAGIC )
#endif /* #if defined(MTK_RELEASE_MODE) && (MTK_RELEASE_MODE==MTK_M_MFG) */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

uint8_t board_cli_ver(uint8_t len, char *param[])
{
#if 0
    char fw_ver[32];
    char patch_ver[32];

    os_memset(fw_ver, 0, 32);
    os_memset(patch_ver, 0, 32);
#endif /* #if 0 */

    cli_puts("SDK Ver: ");
    cli_puts(sw_verno_str);
#if defined(MTK_RELEASE_MODE)
#if (MTK_RELEASE_MODE == MTK_M_DEBUG)
    cli_puts(" (Debug)");
#elif  (MTK_RELEASE_MODE == MTK_M_MFG)
    cli_puts(" (MFG)");
#endif /* #if (MTK_RELEASE_MODE == MTK_M_DEBUG) */
#endif /* #if defined(MTK_RELEASE_MODE) */
    cli_putln();

    cli_puts("Build Time    : ");
    cli_puts(build_date_time_str);
    cli_putln();

    cli_puts("Official Build  Time : ");
    cli_puts(official_ver_str);
    cli_putln();

#if 0
    connsys_util_get_n9_fw_ver(fw_ver);
#if (PRODUCT_VERSION == 7687 || PRODUCT_VERSION == 7697)
    connsys_util_get_ncp_patch_ver(patch_ver);
#endif /* #if (PRODUCT_VERSION == 7687 || PRODUCT_VERSION == 7697) */

    cli_puts("N9 Image  Ver: ");
    cli_puts(fw_ver);
    cli_putln();
#if (PRODUCT_VERSION == 7687 || PRODUCT_VERSION == 7697)
    cli_puts("HW Patch  Ver: ");
    cli_puts(patch_ver);
    cli_putln();
#endif /* #if (PRODUCT_VERSION == 7687 || PRODUCT_VERSION == 7697) */
#endif /* #if 0 */

    return 0;
}


uint8_t board_cli_reboot(uint8_t len, char *param[])
{
    cli_puts("Reboot Bye Bye Bye!!!!\n");
#ifdef CFG_AUDIO_DSP_LEAUDIO_EN
    hal_audio_dsp_off();
    vTaskDelay(50); // for syslog task print time
#endif /* #ifdef CFG_AUDIO_DSP_LEAUDIO_EN */
    __disable_irq();
    hal_cache_disable();
    hal_cache_deinit();

    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);

    return 0;
}


uint8_t board_cli_reg_read(uint8_t len, char *param[])
{
    uint32_t reg;
    uint32_t val;
    uint8_t  type;

    if (len != 1) {
        printf("reg#\n");
        return 0;
    }

    reg = toi(param[0], &type);

    if (type == TOI_ERR) {
        printf("reg#\n");
    } else {
        val = *((volatile uint32_t *)reg);
        printf("read register 0x%08x (%u) got 0x%08x\n", (unsigned int)reg, (unsigned int)reg, (unsigned int)val);
    }

    return 0;
}


uint8_t board_cli_reg_write(uint8_t len, char *param[])
{
    uint32_t reg;
    uint32_t val;
    uint8_t  type;

    if (len == 2) {
        reg = toi(param[0], &type);
        if (type == TOI_ERR) {
            printf("reg#\n");
            return 0;
        }
        val = toi(param[1], &type);
        if (type == TOI_ERR) {
            printf("val#\n");
            return 0;
        }

        *((volatile uint32_t *)reg) = val;
        printf("written register 0x%08x (%u) as 0x%08x\n", (unsigned int)reg, (unsigned int)reg, (unsigned int)val);
    }

    return 0;
}


uint8_t board_cli_mcu(uint8_t len, char *param[])
{
#ifdef HAL_CLOCK_MODULE_ENABLED
    uint32_t hz;

    hz = hal_clock_get_mcu_clock_frequency();

    printf("MCU Clock Frequency = %lu\r\n", hz);
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */

    return 0;
}

/****************************************************************************
 *
 * Memory Debug Commands
 *
 ****************************************************************************/

#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) && defined(MEM_DEBUG_CLI_ENABLE))

#define MEM_C(a)  (*(volatile unsigned char *)(a))
#define MEM_I(a)  (*(volatile unsigned int  *)(a))

uint8_t board_cli_find_mem(uint8_t len, char *param[])
{
#define PAT_MAX     (32)
    uint8_t type;
    int addr, size;
    int i, j;
    uint8_t pattern[PAT_MAX];

    if (len < 3 || len > PAT_MAX + 2)
        return 1;

    addr = toi(param[0], &type);
    if (type == TOI_ERR)
        return 1;

    size = toi(param[1], &type);
    if (type == TOI_ERR)
        return 1;

    for (j = 2; j < len; j++) {
        pattern[j -  2] = toi(param[j], &type);
        if (type == TOI_ERR)
            return 1;
    }

    for (i = addr; i < addr + size + 2 - len; i++) {
        for (j = 0; j < len - 2; j++)
            if (pattern[j] != MEM_C(i + j))
                break;
        if (j == len - 2) {
            cli_puts("found at ");
            cli_put0x(i);
            cli_putln();
        }
    }

    return 0;
}


uint8_t board_cli_dump_mem(uint8_t len, char *param[])
{
    uint8_t type, c;
    int addr, size;
    int i = 0, j;

    while (i < len) {
        addr = toi(param[i], &type);
        if (type == TOI_ERR)
            break;

        if (i + 1 < len) {
            size = toi(param[i + 1], &type);
        } else
            size = 64; // default 64 bytes
        i += 2;

        // dump memory
        for (j = (addr & ~15); j < addr + size; j++) {
            // print address
            if ((j & 15) == 0) {
                cli_putx(j);
                cli_putc(' ');
            }

            // print 1 byte or 'blank'
            if (j >= addr) {
                c = MEM_C(j);
                cli_putx(c >> 4);
                cli_putx(c & 15);
            } else {
                cli_putc(' ');
                cli_putc(' ');
            }

            if ((j & 15) == 15)
                cli_putln();
            else if ((j & 15) == 7) {
                cli_putc(' ');
                cli_putc(' ');
            } else
                cli_putc(' ');
#ifdef HAL_GPT_MODULE_ENABLED
            /* Slow down dump cli for CI MSP tool */
            hal_gpt_delay_ms(1);
#endif /* #ifdef HAL_GPT_MODULE_ENABLED */
        }

        cli_putc('\n');
    }
    return 1;
}


uint8_t board_cli_fill_mem(uint8_t len, char *param[])
{
    uint8_t type, c;
    int addr, size;

    if (len == 3) {
        addr = toi(param[0], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        c = (uint8_t)toi(param[1], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        size = toi(param[2], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        memset((void *)addr, c, (size_t)size);
    }

    return 0;
}

#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) && defined(MEM_DEBUG_CLI_ENABLE)) */

#if defined(MTK_RELEASE_MODE) && (MTK_RELEASE_MODE==MTK_M_MFG)
uint8_t _mfg_cli_destory(uint8_t len, char *param[])
{
    hal_flash_status_t status;

    //printf("[Bef] MFG Magic = %p 0x%08lx 0x%08lx\r\n", _xip_mfg_addr, *(((volatile uint32_t *)_xip_mfg_addr) + 2), *(((volatile uint32_t *)_xip_mfg_addr) + 3));
    if (IS_MFG_MAGIC(_xip_mfg_addr)) {
        status = hal_flash_write((uint32_t)(_rom_mfg_start + 3), (const uint8_t *)"done", 4);
        if (status != HAL_FLASH_STATUS_OK)
            printf("MFG Destory Fail\r\n");
        else
            printf("MFG Destory Done\r\n");
    } else {
        printf("Invalid MFG\r\n");
    }
    return 0;
}

cmd_t mfg_cli[] = {
    { "destory",   "destory mfg image",           _mfg_cli_destory,                   NULL },
    { NULL, NULL, NULL, NULL}
};
#endif /* #if defined(MTK_RELEASE_MODE) && (MTK_RELEASE_MODE==MTK_M_MFG) */

