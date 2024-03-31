/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "main.h"
#include "cli.h"
#include "stdlib.h"
#include <string.h>
#include <types.h>
#include <systimer.h>
#include <driver_api.h>
#include <mt_printf.h>
#include "spm.h"
#include "interrupt.h"
#include <xtensa/tie/xt_interrupt.h>

#ifdef CFG_CLI_SUPPORT
static void cpu_irq_interrupt(void)
{
    DRV_WriteReg32(0x41003114, DRV_Reg32(0x41003114) & (~0x1) );
    PRINTF_D("IRQ : 0x%x\n",  DRV_Reg32(0x41003114));
}

NORMAL_SECTION_FUNC static int cli_spm_init(char *pcWriteBuffer,
    size_t xWriteBufferLen, const char *pcCommandString )
{
    int param_cnt;

    FreeRTOS_CLIPutString("CLI SPM INIT\r\n");
    param_cnt = FreeRTOS_CLIGetNumberOfParameters(pcCommandString);
    if (param_cnt > 0) {
        FreeRTOS_CLIPutString("over range\r\n");
        return 0;
    }
    spm_init();

    return 0;
}
NORMAL_SECTION_FUNC static void print_vcore_dvfs_set_usage(void)
{
    FreeRTOS_CLIPutString("usage: vcore_dvfs_set [type] [opp index] ...\r\n");
    FreeRTOS_CLIPutString("  type:\r\n");
    FreeRTOS_CLIPutString("    0: VCORE\r\n");
    FreeRTOS_CLIPutString("    1: DDR\r\n");
    FreeRTOS_CLIPutString("  opp index:\r\n");
    FreeRTOS_CLIPutString("    0: type 0: LPDDR3/4 1200/1200Mhz, type 1: VCORE voltage 0.65V\r\n");
    FreeRTOS_CLIPutString("    1: type 0: LPDDR3/4 1400/2400Mhz, type 1: VCORE voltage 0.7V\r\n");
    FreeRTOS_CLIPutString("    2: type 0: LPDDR3/4 1866/3200Mhz, type 1: VCORE voltage 0.8V\r\n");
    FreeRTOS_CLIPutString("  example: 'vcore_dvfs_set 0 1' means set VCORE voltage to 0.7V\r\n");
}

NORMAL_SECTION_FUNC static void print_vcore_dvfs_stress_usage(void)
{
    FreeRTOS_CLIPutString("usage: vcore_dvfs_stress [stress count] ...\r\n");
    FreeRTOS_CLIPutString("  example: 'vcore_dvfs_stress 1000' means set vcore dvfs to random level 1000 times\r\n");
}


NORMAL_SECTION_FUNC static int cli_spm_cpu_to_DSP(char *pcWriteBuffer,
    size_t xWriteBufferLen, const char *pcCommandString )
{
    int param_cnt;

    FreeRTOS_CLIPutString("cli_spm_cpu_to_DSP\r\n");
    param_cnt = FreeRTOS_CLIGetNumberOfParameters(pcCommandString);
    if (param_cnt > 0) {
        print_vcore_dvfs_set_usage();
        return 0;
    }

    PRINTF_D("IRQ Setting \r\n");
    request_irq(LX_MCU_IRQ_B, cpu_irq_interrupt, "cm33 int");
    spm_int_enable(1,  BIT(cpu2dsp_irq_b));
    return 0;
}

static  NORMAL_SECTION_FUNC int cli_spm_ultra_low_power(char *pcWriteBuffer,
    size_t xWriteBufferLen, const char *pcCommandString )
{

    int param_cnt;

    FreeRTOS_CLIPutString("cli_spm_wfi\r\n");
    param_cnt = FreeRTOS_CLIGetNumberOfParameters(pcCommandString);
    if (param_cnt > 0) {
        print_vcore_dvfs_stress_usage();
        return 0;
    }
    spm_init();
    spm_sleep_mode(ULTRA_LOW_POWER_MODE);
    PRINTF_D("IRQ Setting \r\n");
    request_irq(LX_MCU_IRQ_B, cpu_irq_interrupt, "cm33 int");
    spm_int_enable(1, BIT(cpu2dsp_irq_b));
    dump_spm_reg();
    PRINTF_D("Sleeping ZZZ\r\n");
    XT_WAITI(0);
    PRINTF_D("Wakeup!\r\n");
    spm_sleep_mode(WFI_MODE);
    return 0;
}

static NORMAL_SECTION_FUNC int cli_spm_wfi(char *pcWriteBuffer,
    size_t xWriteBufferLen, const char *pcCommandString )
{
    int param_cnt;

    FreeRTOS_CLIPutString("cli_spm_wfi\r\n");
    param_cnt = FreeRTOS_CLIGetNumberOfParameters(pcCommandString);
    if (param_cnt > 0) {
        print_vcore_dvfs_stress_usage();
        return 0;
    }
    spm_init();
    PRINTF_D("IRQ Setting \r\n");
    request_irq(LX_MCU_IRQ_B, cpu_irq_interrupt, "cm33 int");
    spm_int_enable(1, BIT(cpu2dsp_irq_b));
    dump_spm_reg();
    PRINTF_D("Sleeping ZZZ\r\n");
    XT_WAITI(0);
    PRINTF_D("Wakeup!\r\n");

    return 0;
}

static NORMAL_SECTION_FUNC int cli_spm_dump(char *pcWriteBuffer,
    size_t xWriteBufferLen, const char *pcCommandString )
{
    int param_cnt;

    FreeRTOS_CLIPutString("cli_spm dump\r\n");
    param_cnt = FreeRTOS_CLIGetNumberOfParameters(pcCommandString);
    if (param_cnt > 0) {
        print_vcore_dvfs_stress_usage();
        return 0;
    }
    dump_spm_reg();
    return 0;
}

static const CLI_Command_Definition_t cli_cmd_spm_wfi =
{
    "spm_wfi",
    "\r\nspm enter wfi mode \r\n",
    cli_spm_wfi,
    -1
};

static const CLI_Command_Definition_t cli_cmd_spm_init =
{
    "spm_init",
    "\r\nspm init \r\n",
    cli_spm_init,
    -1
};

static const CLI_Command_Definition_t cli_cmd_spm_cpu_to_dsp_int =
{
    "spm_cpu_to_dsp",
    "\r\nspm systimer int\r\n",
    cli_spm_cpu_to_DSP,
    -1
};

static const CLI_Command_Definition_t cli_cmd_spm_ultra_low_power =
{
    "spm_ultra",
    "\r\nspm enter ultra low power mode \r\n",
    cli_spm_ultra_low_power,
    -1
};

static const CLI_Command_Definition_t cli_cmd_spm_dump =
{
    "spm_dump",
    "\r\nspm CR dump \r\n",
    cli_spm_dump,
    -1
};

NORMAL_SECTION_FUNC void spm_cli_register(void)
{
	FreeRTOS_CLIRegisterCommand(&cli_cmd_spm_init);
	FreeRTOS_CLIRegisterCommand(&cli_cmd_spm_cpu_to_dsp_int);
	FreeRTOS_CLIRegisterCommand(&cli_cmd_spm_ultra_low_power);
	FreeRTOS_CLIRegisterCommand(&cli_cmd_spm_wfi);
	FreeRTOS_CLIRegisterCommand(&cli_cmd_spm_dump);
}
#endif

NORMAL_SECTION_FUNC void spm_cli_init()
{
#ifdef CFG_CLI_SUPPORT
	spm_cli_register();
#endif
}
