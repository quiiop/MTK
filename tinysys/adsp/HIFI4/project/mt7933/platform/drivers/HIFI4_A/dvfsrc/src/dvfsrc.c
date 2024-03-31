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
#ifdef CFG_VCORE_DVFS_SUPPORT
#include <dvfsrc.h>
#endif

#define dvfsrc_rmw(addr, val, mask, shift) \
	DRV_WriteReg32(addr, (DRV_Reg32(addr) & ~(mask << shift)) \
			| (val << shift))

SRAM_SECTION_FUNC static void dvfsrc_set_sw_req2(unsigned int type, unsigned int data)
{
	if (type == DVFSRC_VCORE_REQ)
		dvfsrc_rmw(DVFSRC_SW_REQ2, data, VCORE_SW_AP2_MASK, VCORE_SW_AP2_SHIFT);
	else
		dvfsrc_rmw(DVFSRC_SW_REQ2, data, EMI_SW_AP2_MASK, EMI_SW_AP2_SHIFT);
}

SRAM_SECTION_FUNC static int is_opp_forced(void)
{
	return DRV_Reg32(DVFSRC_BASIC_CONTROL) & 0xC000;
}

static SRAM_SECTION_FUNC int is_dvfsrc_ready(void)
{
	if(DRV_Reg32(SPM_SW_RSV_6) == PCM_DVFS_INI_CMD)
		return 1;
	else if (DRV_Reg32(SPM_SW_RSV_6) == PCM_SUSPEND_INI_CMD)
		return (DRV_Reg32(DSP2SPM_REQ_STA) != 0x1F) ? 0 : 1;
	else
		return 0;
	/*return (DRV_Reg32(SPM_SW_RSV_6) != PCM_DVFS_INI_CMD) ? 0 : 1;*/
}

SRAM_SECTION_FUNC static int get_target_level(void)
{
	return (DRV_Reg32(DVFSRC_LEVEL) & 0xFFFF);
}

static SRAM_SECTION_FUNC unsigned int get_cur_opp(unsigned int type)
{
	if (type == DVFSRC_VCORE_REQ)
		return __builtin_ffs(DRV_Reg32(SPM_DVS_LEVEL)) - 1;
	else
		return __builtin_ffs(DRV_Reg32(SPM_DFS_LEVEL)) - 1;
}

SRAM_SECTION_FUNC static int dvfsrc_wait_for_completion(unsigned int type, unsigned int opp_idx)
{
	int ret = 1;

	while ((get_target_level() != 0) && ret > 0) {
		if (ret++ >= DVFSRC_TIMEOUT) {
			PRINTF_E("Wait target timeout !!!\n");
			ret = -1;
		}
		udelay(1);
	}
	ret = 1;
	while ((get_cur_opp(type) < opp_idx) && ret > 0) {
		if (ret++ >= DVFSRC_TIMEOUT) {
			PRINTF_E("Wait type: %d req timeout: %d!!!\n", type, get_cur_opp(type));
			ret = -1;
		}
		udelay(1);
	}
	return 0;
}

SRAM_SECTION_FUNC int set_dvfs_opp(unsigned int type, unsigned int opp_idx)
{
	int level;
	int count;
	int ret = 0;

	if (opp_idx >= VCORE_OPP_NUM)
		level = VCORE_0P8;
	if (opp_idx < VCORE_0P65)
		level = VCORE_0P65;
	level = opp_idx;

	if (is_opp_forced()) {
		PRINTF_E("%s: dvfsrc is forced\n", __func__);
		return -1;
	}
	count = 500;
	while (count --) {
		if (is_dvfsrc_ready()) {
			dvfsrc_set_sw_req2(type, level);
			ret = dvfsrc_wait_for_completion(type,level);
			break;
		}
		PRINTF_I("%s: wait RC ready %d\n", __func__, 500 - count);
		ret = -1;
		udelay(3);
	}
	return ret;
}

NORMAL_SECTION_FUNC static int get_vcore_dvfs_level(void)
{
	return DRV_Reg32(DVFSRC_LEVEL) >> CURRENT_LEVEL_SHIFT;
}

NORMAL_SECTION_FUNC static int spm_get_dvfs_final_level(void)
{
	int rsv9 = DRV_Reg32(SPM_SW_RSV_9) & 0xFFFF;
	int event_sta = DRV_Reg32(SPM_DVFS_EVENT_STA) & 0xFFFF;

	if (event_sta != 0)
		return rsv9 > event_sta ? event_sta : rsv9;
	else
		return rsv9;
}

NORMAL_SECTION_FUNC static unsigned int get_cur_vcore_dvfs_opp(void)
{
	return __builtin_ffs(spm_get_dvfs_final_level()) - 1;
}

NORMAL_SECTION_FUNC static int is_dvfsrc_enabled(void)
{
	return DRV_Reg32(DVFSRC_BASIC_CONTROL) & 0x1;
}

NORMAL_SECTION_FUNC static int is_spm_enabled(void)
{
	return DRV_Reg32(PCM_REG15_DATA) != 0 ? 1 : 0;
}

#ifdef CFG_CLI_SUPPORT

NORMAL_SECTION_FUNC static int cli_dsp2spm_req_set(char *pcWriteBuffer,
    size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *param_id;
    int param_id_len, param_cnt;
    unsigned int value;

    FreeRTOS_CLIPutString("cli_vcore_dvfs_set\r\n");
    param_cnt = FreeRTOS_CLIGetNumberOfParameters(pcCommandString);
    if (param_cnt > 1) {
        FreeRTOS_CLIPutString("over range\r\n");
        return 0;
    }
    param_id = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_id_len);
    if (param_id == NULL || strncmp(param_id, "-h", param_id_len) == 0) {
        FreeRTOS_CLIPutString("Unknown input req value\r\n");
        return 0;
    }
    if (mt_str2ul(param_id, &value) != 0) {
        FreeRTOS_CLIPutString("Unknown input req type\r\n");
        return 0;
    }
    if (value < 0 || value > 0x1f) {
        FreeRTOS_CLIPutString("value over range\r\n");
        return 0;
    }
    if (DRV_Reg32(SPM_SW_RSV_6) == PCM_SUSPEND_INI_CMD) {
        while (value != DRV_Reg32(DSP2SPM_REQ_STA)) {
            FreeRTOS_CLIPrintf("try to set DSP2SPM flag %X\r\n", value);
            DRV_WriteReg32(DSP2SPM_REQ, value);
            udelay(10);
         }
    } else
        FreeRTOS_CLIPutString("AP not in suspend mode\r\n");

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

NORMAL_SECTION_FUNC static int cli_vcore_dvfs_set(char *pcWriteBuffer,
    size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *param_id;
    int param_id_len, param_cnt;
    unsigned int opp_idx;
    unsigned int type;

    FreeRTOS_CLIPutString("cli_vcore_dvfs_set\r\n");
    param_cnt = FreeRTOS_CLIGetNumberOfParameters(pcCommandString);
    if (param_cnt > 2) {
        print_vcore_dvfs_set_usage();
        return 0;
    }
    param_id = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_id_len);
    if (param_id == NULL || strncmp(param_id, "-h", param_id_len) == 0) {
        print_vcore_dvfs_set_usage();
        return 0;
    }
    if (mt_str2ul(param_id, &type) != 0) {
        FreeRTOS_CLIPutString("Unknown input req type\r\n");
        return 0;
    }
    if (type < 0 || type > 1) {
        FreeRTOS_CLIPutString("type over range\r\n");
        return 0;
    }
    param_id = FreeRTOS_CLIGetParameter(pcCommandString, 2, &param_id_len);
    if (param_id == NULL || strncmp(param_id, "-h", param_id_len) == 0) {
        print_vcore_dvfs_set_usage();
        return 0;
    }
    if (mt_str2ul(param_id, &opp_idx) != 0) {
        FreeRTOS_CLIPutString("Unknown input opp index\r\n");
        return 0;
    }
    if (opp_idx < 0 || opp_idx > 2) {
        FreeRTOS_CLIPutString("opp index over range\r\n");
        return 0;
    }
    if(set_dvfs_opp(type, opp_idx))
        FreeRTOS_CLIPutString("failed to set dvfs opp!\r\n");

    return 0;
}

NORMAL_SECTION_FUNC static int cli_vcore_dvfs_get(char *pcWriteBuffer,
    size_t xWriteBufferLen, const char *pcCommandString )
{
    FreeRTOS_CLIPutString("cli_vcore_dvfs_get\r\n");
    FreeRTOS_CLIPrintf("VCORE DVFS LEVEL: 0x%X, VCORE OPP: %d, DDR OPP: %d\r\n",
    	get_vcore_dvfs_level(),
    	get_cur_opp(DVFSRC_VCORE_REQ),
    	get_cur_opp(DVFSRC_DDR_REQ));
    return 0;
}


NORMAL_SECTION_FUNC static void print_vcore_dvfs_stress_usage(void)
{
    FreeRTOS_CLIPutString("usage: vcore_dvfs_stress [stress count] ...\r\n");
    FreeRTOS_CLIPutString("  example: 'vcore_dvfs_stress 1000' means set vcore dvfs to random level 1000 times\r\n");
}

static NORMAL_SECTION_FUNC int cli_vcore_dvfs_stress(char *pcWriteBuffer,
    size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *param_id;
    int param_id_len, param_cnt;
    unsigned int loop;
    unsigned int opp_idx;

    FreeRTOS_CLIPutString("cli_vcore_dvfs_stress\r\n");
    param_cnt = FreeRTOS_CLIGetNumberOfParameters(pcCommandString);
    if (param_cnt > 1) {
        print_vcore_dvfs_stress_usage();
        return 0;
    }
    param_id = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_id_len);
    if (param_id == NULL || strncmp(param_id, "-h", param_id_len) == 0) {
        print_vcore_dvfs_stress_usage();
        return 0;
    }

    if (mt_str2ul(param_id, &loop) != 0) {
        FreeRTOS_CLIPutString("Unknown input loop count\r\n");
        return 0;
    }
	while(loop --) {
		opp_idx = rand() % 3;
		if(set_dvfs_opp(DVFSRC_VCORE_REQ, opp_idx))
			FreeRTOS_CLIPutString("failed to set vcore opp!\r\n");
		opp_idx = rand() % 3;
		if(set_dvfs_opp(DVFSRC_DDR_REQ, opp_idx))
			FreeRTOS_CLIPutString("failed to set ddr opp!\r\n");
		PRINTF_I("success to run round %d !\r\n", loop);
	}
    return 0;
}


static const CLI_Command_Definition_t cli_cmd_dsp2spm_req_set =
{
    "dsp2spm_req_set",
    "\r\ndsp2spm_req_set \r\n",
    cli_dsp2spm_req_set,
    -1
};

static const CLI_Command_Definition_t cli_cmd_vcore_dvfs_set =
{
    "vcore_dvfs_set",
    "\r\nvcore_dvfs_set [id|-h]\r\n",
    cli_vcore_dvfs_set,
    -1
};

static const CLI_Command_Definition_t cli_cmd_vcore_dvfs_get =
{
    "vcore_dvfs_get",
    "\r\nvcore_dvfs_get \r\n",
    cli_vcore_dvfs_get,
    -1
};

static const CLI_Command_Definition_t cli_cmd_vcore_dvfs_stress =
{
    "vcore_dvfs_stress",
    "\r\nvcore_dvfs_stress \r\n",
    cli_vcore_dvfs_stress,
    -1
};

NORMAL_SECTION_FUNC void dvfsrc_cli_register(void)
{
	FreeRTOS_CLIRegisterCommand(&cli_cmd_dsp2spm_req_set);
	FreeRTOS_CLIRegisterCommand(&cli_cmd_vcore_dvfs_set);
	FreeRTOS_CLIRegisterCommand(&cli_cmd_vcore_dvfs_get);
	FreeRTOS_CLIRegisterCommand(&cli_cmd_vcore_dvfs_stress);
}
#endif

NORMAL_SECTION_FUNC void dvfsrc_init()
{
	PRINTF_I("%s\n", __func__);
	if(!is_spm_enabled())
		PRINTF_E("%s: spm is not enabled\n", __func__);
	if(!is_dvfsrc_enabled())
		PRINTF_E("%s: dvfsrc is not enabled\n", __func__);
	PRINTF_I("%s: success! Opp is %d, level is 0x%X\n", __func__,
		get_cur_vcore_dvfs_opp(), get_vcore_dvfs_level());
#ifdef CFG_CLI_SUPPORT
	dvfsrc_cli_register();
#endif
}
