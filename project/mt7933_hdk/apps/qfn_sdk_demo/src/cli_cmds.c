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
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#if defined(MTK_MINICLI_ENABLE)


#include "toi.h"


#include "minicli_cmd_table.h"

#ifdef MTK_UT_ENABLE
#include "ut.h"
#endif /* #ifdef MTK_UT_ENABLE */

static cli_t *_cli_ptr;


#ifdef MTK_CLI_TEST_MODE_ENABLE
static uint8_t _sdk_cli_test_mode(uint8_t len, char *param[]);
static uint8_t _sdk_cli_normal_mode(uint8_t len, char *param[]);
#endif /* #ifdef MTK_CLI_TEST_MODE_ENABLE */

#ifdef MTK_CLI_TEST_MODE_ENABLE
#define GOTO_TEST_MODE_CLI_ENTRY    { "en",   "enter test mode",     _sdk_cli_test_mode    ,NULL},
#define GOTO_NORMAL_MODE_CLI_ENTRY  { "back", "back to normal mode", _sdk_cli_normal_mode ,NULL },
#endif /* #ifdef MTK_CLI_TEST_MODE_ENABLE */


#ifndef GOTO_TEST_MODE_CLI_ENTRY
#define GOTO_TEST_MODE_CLI_ENTRY
#endif /* #ifndef GOTO_TEST_MODE_CLI_ENTRY */

#ifndef GOTO_NORMAL_MODE_CLI_ENTRY
#define GOTO_NORMAL_MODE_CLI_ENTRY
#endif /* #ifndef GOTO_NORMAL_MODE_CLI_ENTRY */


#ifdef MTK_MT7933_WIFI_ENABLE
#include "gl_wifi_cli.h"
#ifdef MTK_ATED_ENABLE
#include "ated_init.h"
#endif /* #ifdef MTK_ATED_ENABLE */
#ifdef MTK_PING_OUT_ENABLE
#include "ping_cli.h"
#endif /* #ifdef MTK_PING_OUT_ENABLE */
#ifdef MTK_IPERF_ENABLE
/* IPERF_CLI_ENTRY */
#include "iperf_cli.h"
#endif /* #ifdef MTK_IPERF_ENABLE */
#endif /* #ifdef MTK_MT7933_WIFI_ENABLE */

/**
 * ADSP_CLI_ENTRY
 */
#if defined(BRINGUP_DSP_ENABLE) && defined(MTK_HIFI4DSP_ENABLE)
#include "adsp_debug.h"
#else /* #if defined(BRINGUP_DSP_ENABLE) && defined(MTK_HIFI4DSP_ENABLE) */
#define ADSP_CLI_ENTRY
#endif /* #if defined(BRINGUP_DSP_ENABLE) && defined(MTK_HIFI4DSP_ENABLE) */

/****************************************************************************
 *
 * Integer conversion
 *
 ****************************************************************************/




/****************************************************************************
 *
 * TEST commands
 *
 ****************************************************************************/


#define MEM_C(a)  (*(volatile unsigned char *)(a))
#define MEM_I(a)  (*(volatile unsigned int  *)(a))


struct mem_info {
    char        *desc;
    uint32_t    base;
    uint32_t    limit;
};


static const struct mem_info _g_mem_info[] = {
    { "CM33 LOCAL - ROM",                  0x00000000, 0x00010000 },
    { "CM33 LOCAL - TCM First (rsvd)",     0x0010b41c, 0x0010b42c },
    { "CM33 LOCAL - TCM First",            0x00100000, 0x00110000 },
    { "CM33 LOCAL - TCM_Next",             0x00110000, 0x00118000 },
    { "CM33 LOCAL - SYSRAM_ALIAS",         0x08000000, 0x10000000 },
    { "CM33 LOCAL - PSRAM_ALIAS",          0x10000000, 0x18000000 },
    { "CM33 LOCAL - FLASH_ALIAS",          0x18000000, 0x20000000 },
    { "CM33 LOCAL - CM33_CFG",             0x20000000, 0x20100000 },
    { "CM33 LOCAL - CM33_IRQ_CFG",         0x21000000, 0x21010000 },
    { "CM33 LOCAL - CM33_APXGPT",          0x21010000, 0x21011000 },
    { "CM33 LOCAL - WDT",                  0x21020000, 0x21030000 },
    { "CM33 LOCAL - UART",                 0x21040000, 0x21040100 },
    { "CM33 LOCAL - CM33_BUS_REG",         0x21050000, 0x21060000 },
    { "CM33 LOCAL - CHIP_MISC_CFG",        0x21060000, 0x21070000 },
    { "CM33 LOCAL - CM33_LOCAL_BUS_BRCM",  0x210A0000, 0x210A1000 },
    { "CM33 LOCAL - PPB",                  0xE0000000, 0xE0100000 },
    { "CM33 LOCAL - VENDOR_SYS",           0xE0100000, 0xFFFFFFFF },

    { "CM33&DSP - Firewall",               0x30000000, 0x30010000 },
    { "CM33&DSP - Audio ADC/DAC",          0x30010000, 0x30020000 },
    { "CM33&DSP - TOP_CLK_OFF",            0x30020000, 0x30030000 },
    { "CM33&DSP - TOP_CFG_AON",            0x30030000, 0x30040000 },
    { "CM33&DSP - PWM 0~2",                0x30040000, 0x30070000 },
    { "CM33&DSP - RTC",                    0x30070000, 0x30080000 },
    { "CM33&DSP - SEJ",                    0x30090000, 0x300A0000 },
    { "CM33&DSP - Security wrapper",       0x300A0000, 0x300B0000 },
    { "CM33&DSP - PMU",                    0x300B0000, 0x300C0000 },
    { "CM33&DSP - PLL",                    0x300C0000, 0x300D0000 },
    { "CM33&DSP - SPM",                    0x300D0000, 0x300E0000 },
    { "CM33&DSP - XTAL CTRL",              0x300E0000, 0x300F0000 },
    { "CM33&DSP - DEVAPC AON",             0x30300000, 0x30304000 },
    { "CM33&DSP - SYSRAM",                 0x80000000, 0x90000000 },
    { "CM33&DSP - PSRAM",                  0xA0000000, 0xB0000000 },
    { "CM33&DSP - FLASH",                  0x90000000, 0xA0000000 },

    { NULL, 0, 0 }
};


const struct mem_info *mem_type_find(const struct mem_info *info, uint32_t addr)
{
    info = !info ? &_g_mem_info[0] : (info + 1);

    while (info->desc) {
        if (info->base <= addr && info->limit > addr)
            return info;
        info++;
    }

    return NULL;
}

static uint8_t _cmd_mem_type(uint8_t len, char *param[])
{
    uint8_t     type;
    uint32_t    addr;
    int         i;
    const struct mem_info *info;

    if (len == 0) {
        info = &_g_mem_info[0];
        while (info->desc) {
            cli_puts(info->desc);
            cli_putc(' ');
            cli_put0x(info->base);
            cli_putc(' ');
            cli_put0x(info->limit);
            cli_putln();
            info++;
        }
        return 0;
    }

    for (i = 0; i < len; i++) {
        addr = toi(param[i], &type);
        if (type == TOI_ERR)
            return 1;

        info = mem_type_find(NULL, addr);

        do {
            cli_put0x(addr);
            cli_putc(' ');
            if (info == NULL) {
                cli_puts("unknown");
                cli_putln();
                break;
            }

            cli_puts((char *)info->desc);
            cli_putc(' '); // fixme
            cli_put0x(info->base);
            cli_putc(' ');
            cli_put0x(info->limit);
            cli_putln();
            info = mem_type_find(info, addr);
        } while (info);
    }

    return 0;
}


static uint8_t _cmd_find_mem(uint8_t len, char *param[])
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


static uint8_t _cmd_dump_mem(uint8_t len, char *param[])
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
        }
    }
    return 1;
}


static uint8_t _cmd_fill_mem(uint8_t len, char *param[])
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


static uint8_t _cmd_read_reg(uint8_t len, char *param[])
{
    uint8_t type;
    uint32_t addr;
    int i = 0;

    while (i < len) {
        addr = toi(param[i], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
        } else {
            cli_puts("rr ");
            cli_put0x(addr);
            cli_putln();
            cli_put0x(addr);
            cli_puts(": ");
            cli_put0x(MEM_I(addr));
            cli_putln();
        }
        i++;
    }

    return 0;
}


static uint8_t _cmd_write_reg(uint8_t len, char *param[])
{
    uint8_t type;
    uint32_t addr, data;
    int i = 0;

    if (len & 1) {
        cli_puts("<reg> <value> pairs expected\n");
        return 1;
    }

    while (i < len) {
        addr = toi(param[i], &type);
        if (type == TOI_ERR) {
            cli_puts("param error\n");
            return 2;
        }
        data = toi(param[i + 1], &type);
        if (type == TOI_ERR) {
            cli_puts("param error\n");
            return 2;
        }
        MEM_I(addr) = data;

        cli_puts("wr ");
        cli_put0x(addr);
        cli_putc(' ');
        cli_put0x(data);
        cli_putln();

        i += 2;
    }

    return 0;
}


#if defined(MTK_1ST_LINK_SRAM_BOOT) && defined(HAL_PSRAM_MODULE_ENABLED)
#include "hal_psram.h"
#include "task.h"

static uint8_t _cmd_psram_init(uint8_t len, char *param[])
{
    hal_psram_init();
    vTaskDelay(100);
    return 0;
}
#define PSRAM_CMDS { "psram", "psram init", _cmd_psram_init,    NULL },
#else /* #if defined(MTK_1ST_LINK_SRAM_BOOT) && defined(HAL_PSRAM_MODULE_ENABLED) */
#define PSRAM_CMDS
#endif /* #if defined(MTK_1ST_LINK_SRAM_BOOT) && defined(HAL_PSRAM_MODULE_ENABLED) */

#if defined(HAL_WDT_MODULE_ENABLED) && defined(MTK_CLI_TEST_MODE_ENABLE)
#include "hal_wdt.h"

static uint8_t _cmd_wdt(uint8_t len, char *param[])
{
    if (len < 1) {
        cli_puts("wdt [0:disable, 1:enable]\n");
    }

    if (atoi(param[0]) == 1) {
        hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
        cli_puts("wdt enabled\n");
    } else {
        hal_wdt_disable(HAL_WDT_DISABLE_MAGIC);
        cli_puts("wdt disabled\n");
    }
    return 0;
}
#endif /* #if defined(HAL_WDT_MODULE_ENABLED) && defined(MTK_CLI_TEST_MODE_ENABLE) */

/****************************************************************************
 *
 * TEST MODE
 *
 ****************************************************************************/


#ifdef MTK_CLI_TEST_MODE_ENABLE

static cmd_t   _cmds_test[] = {
    GOTO_NORMAL_MODE_CLI_ENTRY
    MINICLI_TEST_MODE_CLI_CMDS
#ifdef HAL_WDT_MODULE_ENABLED
    { "wdt", "WDT control", _cmd_wdt, NULL },
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
    { NULL, NULL, NULL, NULL }
};

#endif /* #ifdef MTK_CLI_TEST_MODE_ENABLE */


/****************************************************************************
 *
 * Wi-Fi commands
 *
 ****************************************************************************/

#ifdef MTK_MT7933_WIFI_ENABLE
#ifdef MTK_WIFI_DRV_CLI_ENABLE
extern cmd_t wifi_init_cli[];
#endif /* #ifdef MTK_WIFI_DRV_CLI_ENABLE */
#endif /* #ifdef MTK_MT7933_WIFI_ENABLE */

/****************************************************************************
 *
 * BT commands
 *
 ****************************************************************************/

#ifdef BRINGUP_BT_ENABLE
#ifdef MTK_MT7933_BT_ENABLE
extern cmd_t bt_driver_cli[];
#endif /* #ifdef MTK_MT7933_BT_ENABLE */
#ifdef MTK_BT_BOOTS_ENABLE
extern uint8_t btpriv_cli(uint8_t len, char *param[]);
#endif /* #ifdef MTK_BT_BOOTS_ENABLE */
static cmd_t   _cmds_bt[] = {
#ifdef MTK_MT7933_BT_ENABLE
    { "btdrv", "bt driver cli cmd", NULL, bt_driver_cli },
#endif /* #ifdef MTK_MT7933_BT_ENABLE */
#ifdef MTK_BT_BOOTS_ENABLE
    { "btpriv", "bt command", btpriv_cli, NULL },
#endif /* #ifdef MTK_BT_BOOTS_ENABLE */
    { NULL, NULL, NULL, NULL }
};
#endif /* #ifdef BRINGUP_BT_ENABLE */


/****************************************************************************
 *
 * NORMAL MODE
 *
 ****************************************************************************/

#ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE
extern cmd_t audio_drv_debug_cmds[];
#endif /* #ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE */

static cmd_t   _cmds_normal[] = {
    { "mem",   "show memory type of <addr>",   _cmd_mem_type,      NULL },
    { "s",     "search <addr> <len> <pat>",    _cmd_find_mem,      NULL },
    { "d",     "dump memory <addr> <len>",     _cmd_dump_mem,      NULL },
    { "f",     "fill memory",                  _cmd_fill_mem,      NULL },
    { "rr",    "read reg",                     _cmd_read_reg,      NULL },
    { "wr",    "write reg",                    _cmd_write_reg,     NULL },
    PSRAM_CMDS
#ifdef BRINGUP_BT_ENABLE
    { "bt",   "BT commands",                   NULL,       &_cmds_bt[0] },
#endif /* #ifdef BRINGUP_BT_ENABLE */
#ifdef BRINGUP_DSP_ENABLE
    ADSP_CLI_ENTRY
#endif /* #ifdef BRINGUP_DSP_ENABLE */
    GOTO_TEST_MODE_CLI_ENTRY
    MINICLI_NORMAL_MODE_CLI_CMDS
    OS_CLI_ENTRY
#ifdef MTK_UT_ENABLE
    UT_CLI_ENTRY
#endif /* #ifdef MTK_UT_ENABLE */
#ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE
    { "aud_dbg", "audio driver debug common", NULL, &audio_drv_debug_cmds[0]},
#endif /* #ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE */
    { NULL, NULL, NULL, NULL }
};


/****************************************************************************
 *
 * TOGGLE commands
 *
 ****************************************************************************/


#ifdef MTK_CLI_TEST_MODE_ENABLE
static uint8_t _sdk_cli_test_mode(uint8_t len, char *param[])
{
    _cli_ptr->cmd = &_cmds_test[0];
    return 0;
}
#endif /* #ifdef MTK_CLI_TEST_MODE_ENABLE */




#if defined(MTK_CLI_TEST_MODE_ENABLE)
static uint8_t _sdk_cli_normal_mode(uint8_t len, char *param[])
{
    _cli_ptr->cmd = &_cmds_normal[0];
    return 0;
}
#endif /* #if defined(MTK_CLI_TEST_MODE_ENABLE) */


/****************************************************************************
 *
 * PUBLIC functions
 *
 ****************************************************************************/


void cli_cmds_init(cli_t *cli)
{
    _cli_ptr = cli;
    _cli_ptr->cmd = &_cmds_normal[0];
}


#endif /* #if defined(MTK_MINICLI_ENABLE) */
