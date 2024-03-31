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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#include <driver_api.h>
//#include <eint.h>
#include "FreeRTOS.h"
#include <FreeRTOSConfig.h>
#include <interrupt.h>
#include <spm.h>
#include <stdio.h>
#include <systimer.h>
#include <task.h>


/* Dump All SPM Register. */
void dump_spm_reg(void) {
	unsigned int i = 0;
	for (i = SPM_SLEEP_CTL; i <= SPM_INT_EN; i = i + 4)
		PRINTF_I("SPM: [0x%x]: 0x%x\n", i, DRV_Reg32(i));
}

/* SPM register write */
static void spm_reg_write(char *name, unsigned int spm_reg_address,
	unsigned int reg_value) {
	PRINTF_D("SPM: origin %s: 0x%x\n", name, DRV_Reg32(spm_reg_address));
	DRV_WriteReg32(spm_reg_address, reg_value);
	PRINTF_D("SPM: set %s: 0x%x\n", name, DRV_Reg32(spm_reg_address));
}

/* Clear Interrupt Status */
void spm_clear_interrupt(int val) {
	PRINTF_D("SPM: SPM_SLEEP_CLT: 0x%x\n", DRV_Reg32(SPM_SLEEP_CTL));
	PRINTF_D("SPM: STA: 0x%x\n", DRV_Reg32(SPM_INT_STA));
	spm_reg_write("SPM_INT_CLR", SPM_INT_CLR, DRV_Reg32(SPM_INT_CLR) | val);
	spm_reg_write("SPM_INT_CLR", SPM_INT_CLR, 0x0);
}

/* Control SRAM POLL BANK during DSP go to WFI
 * DSP SRAM BANK SLEEP ENABLE
 * [11:8]  0 bank do not sleep
 * [11:8]  1 bank  sleep
 */
void spm_sram_poll_bank_sleep(int sleep_control) {
	int sleep_ctl = DRV_Reg32(SPM_SLEEP_CTL) & ~(DSP_SPM_SRAM_ALL_BANK_SLEEP_EN);
	spm_reg_write("SPM_SLEEP_DSP_SRAM_BANK_CTRL_EN",
		SPM_SLEEP_CTL,
		sleep_ctl |
		sleep_control);
}

/* Control DSP CLK during DSP go to WFI
 * clock gating or not
 * [1] 0  do not gating
 * [1] 1  gating
 */
void spm_clk_control(int clock_control) {
	int sleep_ctl = DRV_Reg32(SPM_SLEEP_CTL) & ~(DSP_SPM_CLK_GATING_EN);
	spm_reg_write("SPM_SLEEP_DSP_CLK_CTRL",
		SPM_SLEEP_CTL,
		sleep_ctl |
		clock_control);
}


/* Choice wake up source as either level trigger or edge trigger
 * trigger_way
 * 0: edge trigger
 * 1: level trigger
 */
void spm_wakeup_trigger(bool trigger_way, int wakeup_source) {
	if (trigger_way)
		spm_reg_write("SPM_INT_LEV",
			SPM_INT_LEV,
			DRV_Reg32(SPM_INT_LEV) | wakeup_source);
	else
		spm_reg_write("SPM_INT_LEV",
			SPM_INT_LEV,
			DRV_Reg32(SPM_INT_LEV) & ~wakeup_source);
}

/* edge trigger, choice falling edge or raising edge
 * trigger_way
 * 0: negedge trigger
 * 1: posedge trigger
 * level trigger, chose which should be invert
 * 0: inverted polarity
 * 1: non-inverted polarity
 */
void spm_int_trigger(bool trigger_way, int wakeup_source) {
	if (trigger_way)
		spm_reg_write("SPM_INT_POL",
			SPM_INT_POL,
			DRV_Reg32(SPM_INT_POL) | wakeup_source);
	else
		spm_reg_write("SPM_INT_POL",
			SPM_INT_POL,
			DRV_Reg32(SPM_INT_POL) & ~wakeup_source);
}

/* Enable/Disable SPM Wake up source */
void spm_int_enable(bool enable, int wakeup_source) {
	if (enable)
		spm_reg_write("SPM_INT_EN",
			SPM_INT_EN,
			DRV_Reg32(SPM_INT_EN) | wakeup_source);
	else
		spm_reg_write("SPM_INT_EN",
			SPM_INT_EN,
			DRV_Reg32(SPM_INT_EN) & ~wakeup_source);
}


NORMAL_SECTION_FUNC void spm_int_init(void)
{

	spm_reg_write("SPM_INT_LEV",
		SPM_INT_LEV,
		SPM_TRIGGER_TYPE);

	spm_reg_write("SPM_INT_LEV",
		SPM_INT_POL,
		SPM_POL_TYPE);
}

/* SPM sleep ctrl */
/*
void spm_sleep_ctrl(struct spm_sleep_ctrl *sleep_ctrl) {
}
*/
/* SPM sleep setting */
/*void spm_sleep_setting(struct spm_sleep_ctrl *sleep_ctrl) {
	sleep_ctrl->dsp0_cache_en = DSP0_SLEEP_CACHE_DISABLE;
	sleep_ctrl->down_time = SPM_DOWN_T1 | SPM_DOWN_T2;
	sleep_ctrl->up_time = SPM_UP_T1 | SPM_UP_T2 | SPM_UP_T3;
	sleep_ctrl->down_en = SPM_SLEEP_CLK26M_EN | SPM_SLEEP_VSRAM_EN | SPM_SLEEP_VCORE_EN;
}*/

/* Enable SPM clocks */
/*static void spm_clock_enable(void) {
	struct clk *clk;

	printf("SPM: spm_clock_enable!\n");

	clk = clk_get(CLK_TOP_SPM_32K);
	if (!clk)
		PRINTF_E("SPM: Fail to get CLK_TOP_SPM_32K\n");
	else
		clk_enable(clk);

	clk = clk_get(CLK_TOP_SPM_MD);
	if (!clk)
		PRINTF_E("SPM: Fail to get CLK_TOP_SPM_MD\n");
	else
		clk_enable(clk);
}*/

/* Sleep Mode
 * [0] 0  GO TO ULTRA LOW POWER MODE
 * [0] 1  WFI ONLY
*/
void spm_sleep_mode(int sleep_mode) {
	int sleep_ctl = DRV_Reg32(SPM_SLEEP_CTL) & ~(DSP_SPM_SLEEP_MODE);
	spm_reg_write("SPM_SLEEP_MODE",
	SPM_SLEEP_CTL,
	sleep_ctl |
	sleep_mode);
}


/* 7933 spm init */
NORMAL_SECTION_FUNC int spm_init(void) {
	//struct spm_sleep_ctrl sleep_ctrl;
	printf("SPM: SPM INIT!\n");

	/*Because of HW Design, we must clear Interrupt Status first*/
	spm_int_init();

	/* Enable SPM clocks 7933 SPM is always on */
	// spm_clock_enable();
	/* setting spm parameters
	It's for 8512 PMIC, 7933 have PMU only, do not need this */
	//spm_sleep_setting(&sleep_ctrl);

	/* SPM sleep control register setting
	 * 1. Default cache always on.
	 * 2. Switch internal 26M to external 32K clock
	 * 3. Set low voltage during sleep mode.
	 */
	//spm_sleep_ctrl(&sleep_ctrl);

	/* Control SRAM POLL BANK During DSP0 WFI */
	spm_sram_poll_bank_sleep(
		SPM_SRAM_BANK0_SLEEP_EN	|
		SPM_SRAM_BANK1_SLEEP_EN	|
		SPM_SRAM_BANK2_SLEEP_EN |
		SPM_SRAM_BANK3_SLEEP_EN
	);
	spm_clk_control(SPM_CLK_GATING);
	#ifdef CFG_SUSPEND_SUPPORT
	/* 7933 RTOS do not have EINT driver, curently*/
	/*spm_reg_write("SPM_INT_EN", SPM_INT_EN, 0);
	spm_wakeup_trigger(LEVEL_TRIGGER, EINT_IRQ_B0);
	spm_int_enable(INT_ENABLE, EINT_IRQ_B0);
	*/
	/* Default go to ultra low power mode when DSP0 WFI.*/
	//spm_sleep_mode(WFI_MODE);
	#endif
	/* Dump All SPM Register after init done. */
	dump_spm_reg();

	return 0;
}
