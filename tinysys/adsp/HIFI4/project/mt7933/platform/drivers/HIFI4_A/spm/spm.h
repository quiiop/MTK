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

#ifndef __SPM_H__
#define __SPM_H__

#include <mt_reg_base.h>
#include <stdbool.h>
#define DSP_SPM_CLI_MODE 1

enum {
    sys_timer_irq_b0 = 0,
    sys_timer_irq_b1,
    sys_timer_irq_b2,
    sys_timer_irq_b3,
    cq_dma_irq_b0,
    cq_dma_irq_b1,
    cq_dma_irq_b2,
    uart_irq_b,
    audio2dsp_irq_b,
    cpu2dsp_irq_b,
    cm33_to_dsp_irq_b,
    ap_dma_i2c_irq_b0,
    ap_dma_i2c_irq_b1,
    ap_dma_irq_b2,
    ap_dma_irq_b3,
    ap_dma_uart0_tx_irq_b4,
    ap_dma_uart0_rx_irq_b5,
    ap_dma_uart1_tx_irq_b6,
    ap_dma_uart1_rx_irq_b7,
    ap_dma_btif_tx_irq_b8,
    ap_dma_btif_rx_irq_b9,
    ap_dma_irq_b10,
    ap_dma_irq_adc_rx_b11,
    bt_cvsd_int_b,
    bt2ap_isoch_irq_b,
    connsys2dsp_btofld_irq_b,
    i2c0_irq_b0,
    i2c0_irq_b1,
    pwm_irq_b0
};

enum {
	EDGE_TRIGGER = 0,
	LEVEL_TRIGGER,
};

enum {
	NEGE_EDGE = 0,
	POS_EDGE,
};

enum {
	INT_DISABLE = 0,
	INT_ENABLE,
};

enum {
	ULTRA_LOW_POWER_MODE = 0,
	BYPASS_ULTRA_LOW_POWER_MODE,
};


#define SPM_TRIGGER_TYPE		0x1FFFFFFF
#define SPM_POL_TYPE			0x60F


#define  HIGH_LEVEL_TRIGGER_BIT		(BIT(sys_timer_irq_b0)  |  \
					 BIT(sys_timer_irq_b1)  |  \
					 BIT(sys_timer_irq_b2)  |  \
					 BIT(sys_timer_irq_b3)  |  \
					 BIT(cpu2dsp_irq_b)  |  \
					 BIT(cm33_to_dsp_irq_b))

#define  ALL_TRIGGER_BIT \
            (BIT(sys_timer_irq_b0)  |  \
            BIT(sys_timer_irq_b1)  |  \
            BIT(sys_timer_irq_b2)  |  \
            BIT(sys_timer_irq_b3)  |  \
            BIT(cq_dma_irq_b0)  |  \
            BIT(cq_dma_irq_b1)  |  \
            BIT(cq_dma_irq_b2)  |  \
            BIT(uart_irq_b)  |  \
            BIT(audio2dsp_irq_b)  |  \
            BIT(cpu2dsp_irq_b)  |  \
            BIT(cm33_to_dsp_irq_b)  |  \
            BIT(ap_dma_i2c_irq_b0)  |  \
            BIT(ap_dma_i2c_irq_b1)  |  \
            BIT(ap_dma_irq_b2)  |  \
            BIT(ap_dma_irq_b3)  |  \
            BIT(ap_dma_uart0_tx_irq_b4)  |  \
            BIT(ap_dma_uart0_rx_irq_b5)  |  \
            BIT(ap_dma_uart1_tx_irq_b6)  |  \
            BIT(ap_dma_uart1_rx_irq_b7)  |  \
            BIT(ap_dma_btif_tx_irq_b8)  |  \
            BIT(ap_dma_btif_rx_irq_b9)  |  \
            BIT(ap_dma_irq_b10)  |  \
            BIT(ap_dma_irq_adc_rx_b11)  |  \
            BIT(bt_cvsd_int_b)  |  \
            BIT(bt2ap_isoch_irq_b)  |  \
            BIT(connsys2dsp_btofld_irq_b)  |  \
            BIT(i2c0_irq_b0)  |  \
            BIT(i2c0_irq_b1)  |  \
            BIT(pwm_irq_b0))


#define EDGE_TRIGGER_INT_BITS



/* do not need in 7933
struct spm_sleep_ctrl {
	bool dsp0_cache_en;
	unsigned int down_time;
	unsigned int up_time;
	unsigned int down_en;
};
*/

/* Interrupt Status
 * [14:0]:   {spis1_irq_b, spis0_irq_b, ptp_therm_int_b,
 * lowbattery_irq_b, sys_timer_irq_b[3:0], wdt_irq_b, afe_irq_b[1:0],
 * eint_irq_b, uart1_irq_b, uart0_irq_b, i2cs_irq_b}
 */
#define SPM_INT_STA			(TOP_SPM_REG_BASE + 0x504)

/* Interrupt Level
 * 0: edge triggher, 1: level trigger.
 * [14:0]:   {spis1_irq_b, spis0_irq_b, ptp_therm_int_b,
 * lowbattery_irq_b, sys_timer_irq_b[3:0], wdt_irq_b, afe_irq_b[1:0],
 * eint_irq_b, uart1_irq_b, uart0_irq_b, i2cs_irq_b}
 */
#define SPM_INT_LEV			(TOP_SPM_REG_BASE + 0x508)

/* Interrupt Polarity
 * 0: negedge triggher, 1: posedge trigger.
 * [14:0]:   {spis1_irq_b, spis0_irq_b, ptp_therm_int_b,
 * lowbattery_irq_b, sys_timer_irq_b[3:0], wdt_irq_b, afe_irq_b[1:0],
 * eint_irq_b, uart1_irq_b, uart0_irq_b, i2cs_irq_b}
 */
#define SPM_INT_POL			(TOP_SPM_REG_BASE + 0x50C)

/* Interrupt State Clear
 * [14:0]:   {spis1_irq_b, spis0_irq_b, ptp_therm_int_b,
 * lowbattery_irq_b, sys_timer_irq_b[3:0], wdt_irq_b, afe_irq_b[1:0],
 * eint_irq_b, uart1_irq_b, uart0_irq_b, i2cs_irq_b}
 */
#define SPM_INT_CLR			(TOP_SPM_REG_BASE + 0x510)

/* Interrupt Enable
 * [14:0]:   {spis1_irq_b, spis0_irq_b, ptp_therm_int_b,
 * lowbattery_irq_b, sys_timer_irq_b[3:0], wdt_irq_b, afe_irq_b[1:0],
 * eint_irq_b, uart1_irq_b, uart0_irq_b, i2cs_irq_b}
 */
#define SPM_INT_EN			(TOP_SPM_REG_BASE + 0x514)

/* Sleep Mode
 * [0] 0  GO TO ULTRA LOW POWER MODE
 * [0] 1  WFI ONLY
 * clock gating or not
 * [1] 0  do not gating
 * [1] 1  gating
 * DSP SRAM BANK SLEEP ENABLE
 * [11:8]  0 bank do not sleep
 * [11:8]  1 bank  sleep
 */
#define SPM_SLEEP_CTL			(TOP_SPM_REG_BASE + 0x500)
#define BIT(nr)             (1 << (nr))


#define DSP_SPM_SLEEP_MODE		BIT(0)
#define WFI_MODE                        1
#define ULTRA_LOW_POWER_MODE            0

#define DSP_SPM_CLK_GATING_EN           BIT(1)
#define SPM_CLK_GATING			0x2
#define SPM_CLK_NOT_GATING		0

#define SPM_SRAM_BANK0_SLEEP_EN		BIT(8)
#define SPM_SRAM_BANK1_SLEEP_EN		BIT(9)
#define SPM_SRAM_BANK2_SLEEP_EN		BIT(10)
#define SPM_SRAM_BANK3_SLEEP_EN		BIT(11)

#define DSP_SPM_SRAM_ALL_BANK_SLEEP_EN	(SPM_SRAM_BANK0_SLEEP_EN |	\
					 SPM_SRAM_BANK1_SLEEP_EN |	\
					 SPM_SRAM_BANK2_SLEEP_EN |	\
					 SPM_SRAM_BANK3_SLEEP_EN )


extern void dsp0_wfi(void);

extern void dump_spm_reg(void);

extern void register_spm_wake_up_source(unsigned int int_lev,
	unsigned int int_pol, unsigned int int_en);

extern void spm_clear_interrupt(int val);

extern int spm_init(void);

extern void spm_int_trigger(bool trigger_way, int wakeup_source);

extern void spm_int_enable(bool enable, int wakeup_source);

extern void spm_sleep_clk_setting(bool gating);

extern void spm_sleep_mode(int sleep_mode);

extern void spm_sram_poll_bank_sleep(int sleep_control) ;

extern void spm_wakeup_trigger(bool trigger_way, int wakeup_source);

#endif

