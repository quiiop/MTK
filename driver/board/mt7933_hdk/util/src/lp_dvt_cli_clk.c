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
#include <stdlib.h>
#include "cli.h"
#include "hal.h"
#include "hal_clock.h"
#include "hal_platform.h"
#include "string.h"


#ifdef MTK_LP_DVT_CLI_ENABLE
static uint8_t _cli_lp_dvt_clk_enable(uint8_t len, char *param[])
{
    uint32_t clock_id = atoi(param[0]);

    hal_clock_status_t ret = hal_clock_enable((hal_clock_cg_id) clock_id);
    printf("hal_clock_enable: clock_id=%lu, ret=%d\r\n", clock_id, ret);

    return 0;
}
static uint8_t _cli_lp_dvt_clk_disable(uint8_t len, char *param[])
{
    uint32_t clock_id = atoi(param[0]);

    hal_clock_status_t ret = hal_clock_disable((hal_clock_cg_id) clock_id);
    printf("hal_clock_disable: clock_id=%lu, ret=%d\r\n", clock_id, ret);


    return 0;
}

static char *clock_cg_id_str_tbl[] = {
    [   HAL_CLOCK_CG_CM33_XTAL  ]   =   "HAL_CLOCK_CG_CM33_XTAL",
    [   HAL_CLOCK_CG_DSP_XTAL   ]   =   "HAL_CLOCK_CG_DSP_XTAL",
    [   HAL_CLOCK_CG_AUDIO_XTAL ]   =   "HAL_CLOCK_CG_AUDIO_XTAL",
    [   HAL_CLOCK_CG_USB_CTRL_XTAL  ]   =   "HAL_CLOCK_CG_USB_CTRL_XTAL",
    [   HAL_CLOCK_CG_USB_PHY_XTAL   ]   =   "HAL_CLOCK_CG_USB_PHY_XTAL",
    [   HAL_CLOCK_CG_UART0_XTAL ]   =   "HAL_CLOCK_CG_UART0_XTAL",
    [   HAL_CLOCK_CG_UART1_XTAL ]   =   "HAL_CLOCK_CG_UART1_XTAL",
    [   HAL_CLOCK_CG_PWM0_XTAL  ]   =   "HAL_CLOCK_CG_PWM0_XTAL",
    [   HAL_CLOCK_CG_PWM1_XTAL  ]   =   "HAL_CLOCK_CG_PWM1_XTAL",
    [   HAL_CLOCK_CG_PWM2_XTAL  ]   =   "HAL_CLOCK_CG_PWM2_XTAL",
    [   HAL_CLOCK_CG_CONNAC_XTAL    ]   =   "HAL_CLOCK_CG_CONNAC_XTAL",
    [   HAL_CLOCK_CG_EFUSE1_XTAL    ]   =   "HAL_CLOCK_CG_EFUSE1_XTAL",
    [   HAL_CLOCK_CG_EFUSE2_XTAL    ]   =   "HAL_CLOCK_CG_EFUSE2_XTAL",
    [   HAL_CLOCK_CG_EFUSE3_XTAL    ]   =   "HAL_CLOCK_CG_EFUSE3_XTAL",
    [   HAL_CLOCK_CG_RTC_XTAL   ]   =   "HAL_CLOCK_CG_RTC_XTAL",
    [   HAL_CLOCK_CG_UHS_PSRAM_XTAL ]   =   "HAL_CLOCK_CG_UHS_PSRAM_XTAL",
    [   HAL_CLOCK_CG_FLASH_XTAL ]   =   "HAL_CLOCK_CG_FLASH_XTAL",
    [   HAL_CLOCK_CG_AUD_ADC0_XTAL  ]   =   "HAL_CLOCK_CG_AUD_ADC0_XTAL",
    [   HAL_CLOCK_CG_AUD_ADC1_XTAL  ]   =   "HAL_CLOCK_CG_AUD_ADC1_XTAL",
    [   HAL_CLOCK_CG_AUD_DAC_XTAL   ]   =   "HAL_CLOCK_CG_AUD_DAC_XTAL",
    [   HAL_CLOCK_CG_TOP_AON_XTAL   ]   =   "HAL_CLOCK_CG_TOP_AON_XTAL",
    [   HAL_CLOCK_CG_TOP_MBIST_XTAL ]   =   "HAL_CLOCK_CG_TOP_MBIST_XTAL",
    [   HAL_CLOCK_CG_USB_MBIST_XTAL ]   =   "HAL_CLOCK_CG_USB_MBIST_XTAL",
    [   HAL_CLOCK_CG_CRYPTO_DIV2_XTAL   ]   =   "HAL_CLOCK_CG_CRYPTO_DIV2_XTAL",
    [   HAL_CLOCK_CG_CM33_DIV2_XTAL ]   =   "HAL_CLOCK_CG_CM33_DIV2_XTAL",
    [   HAL_CLOCK_CG_TOP_PLL    ]   =   "HAL_CLOCK_CG_TOP_PLL",
    [   HAL_CLOCK_CG_DSP_PLL    ]   =   "HAL_CLOCK_CG_DSP_PLL",
    [   HAL_CLOCK_CG_USB_PLL    ]   =   "HAL_CLOCK_CG_USB_PLL",
    [   HAL_CLOCK_CG_XPLL   ]   =   "HAL_CLOCK_CG_XPLL",
    [   HAL_CLOCK_CG_DSP_HCLK   ]   =   "HAL_CLOCK_CG_DSP_HCLK",
    [   HAL_CLOCK_CG_INFRA_BUS  ]   =   "HAL_CLOCK_CG_INFRA_BUS",
    [   HAL_CLOCK_CG_AUDSYS_BUS ]   =   "HAL_CLOCK_CG_AUDSYS_BUS",
    [   HAL_CLOCK_CG_PSRAM  ]   =   "HAL_CLOCK_CG_PSRAM",
    [   HAL_CLOCK_CG_TRACE  ]   =   "HAL_CLOCK_CG_TRACE",
    [   HAL_CLOCK_CG_PSRAM_AXI  ]   =   "HAL_CLOCK_CG_PSRAM_AXI",
    [   HAL_CLOCK_CG_FLASH  ]   =   "HAL_CLOCK_CG_FLASH",
    [   HAL_CLOCK_CG_GCPU   ]   =   "HAL_CLOCK_CG_GCPU",
    [   HAL_CLOCK_CG_ECC    ]   =   "HAL_CLOCK_CG_ECC",
    [   HAL_CLOCK_CG_SPIM0  ]   =   "HAL_CLOCK_CG_SPIM0",
    [   HAL_CLOCK_CG_SPIM1  ]   =   "HAL_CLOCK_CG_SPIM1",
    [   HAL_CLOCK_CG_SPIS   ]   =   "HAL_CLOCK_CG_SPIS",
    [   HAL_CLOCK_CG_I2C0   ]   =   "HAL_CLOCK_CG_I2C0",
    [   HAL_CLOCK_CG_I2C1   ]   =   "HAL_CLOCK_CG_I2C1",
    [   HAL_CLOCK_CG_DBG    ]   =   "HAL_CLOCK_CG_DBG",
    [   HAL_CLOCK_CG_SDIOM  ]   =   "HAL_CLOCK_CG_SDIOM",
    [   HAL_CLOCK_CG_USB_SYS    ]   =   "HAL_CLOCK_CG_USB_SYS",
    [   HAL_CLOCK_CG_USB_XHCI   ]   =   "HAL_CLOCK_CG_USB_XHCI",
    [   HAL_CLOCK_CG_AUX_ADC_DIV    ]   =   "HAL_CLOCK_CG_AUX_ADC_DIV",
    [   HAL_CLOCK_CG_PWM_DIV    ]   =   "HAL_CLOCK_CG_PWM_DIV",
    [   HAL_CLOCK_CG_APLL12_CK_DIV0 ]   =   "HAL_CLOCK_CG_APLL12_CK_DIV0",
    [   HAL_CLOCK_CG_APLL12_CK_DIV3 ]   =   "HAL_CLOCK_CG_APLL12_CK_DIV3",
    [   HAL_CLOCK_CG_APLL12_CK_DIV6 ]   =   "HAL_CLOCK_CG_APLL12_CK_DIV6",
};


static uint8_t _cli_lp_dvt_clk_is_enable(uint8_t len, char *param[])
{
    hal_clock_cg_id clock_id;
    bool ret;
    if (len > 0) {
        clock_id = atoi(param[0]);
        ret = hal_clock_is_enabled(clock_id);
        printf("hal_clock_is_enabled: clock_id=%d, ret=%u\r\n", clock_id, ret);

    } else {
        printf("hal_clock_is_enabled: List of all clock enable status:\r\n");
        for (clock_id = 0; clock_id < HAL_CLOCK_CG_MAX; clock_id++) {
            if (clock_cg_id_str_tbl[clock_id] == NULL)
                continue;
            ret = hal_clock_is_enabled(clock_id);
            printf("   clock:%s(%d), ret=%u\r\n", clock_cg_id_str_tbl[clock_id], clock_id, ret);
        }
    }

    return 0;
}

struct dvt_clk_str_to_val_t {
    char str[36];
    uint32_t val;
};

struct dvt_clk_str_to_val_t clksel_str_tbl[] = {
    {"AUDIO_FAUDIO_CLKSEL_AUDIO_XTAL",      CLK_AUDIO_FAUDIO_CLKSEL_AUDIO_XTAL},
    {"AUDIO_FAUDIO_CLKSEL_60M",             CLK_AUDIO_FAUDIO_CLKSEL_60M},
    {"AUDIO_FAUDIO_CLKSEL_XPLL_DIV4",       CLK_AUDIO_FAUDIO_CLKSEL_XPLL_DIV4},
    {"AUDIO_FASM_CLKSEL_AUDIO_XTAL",        CLK_AUDIO_FASM_CLKSEL_AUDIO_XTAL},
    {"AUDIO_FASM_CLKSEL_120M",              CLK_AUDIO_FASM_CLKSEL_120M},
    {"AUDIO_FASYS_CLKSEL_AUDIO_XTAL",       CLK_AUDIO_FASYS_CLKSEL_AUDIO_XTAL},
    {"AUDIO_FASYS_CLKSEL_XPLL_DIV2",        CLK_AUDIO_FASYS_CLKSEL_XPLL_DIV2},
    {"AUDIO_FASYS_CLKSEL_XPLL_DIV4",        CLK_AUDIO_FASYS_CLKSEL_XPLL_DIV4},
    {"AUDIO_HAPLL_CLKSEL_AUDIO_XTAL",       CLK_AUDIO_HAPLL_CLKSEL_AUDIO_XTAL},
    {"AUDIO_HAPLL_CLKSEL_XPLL_DIV2",        CLK_AUDIO_HAPLL_CLKSEL_XPLL_DIV2},
    {"AUDIO_HAPLL_CLKSEL_XPLL_DIV4",        CLK_AUDIO_HAPLL_CLKSEL_XPLL_DIV4},
    {"AUDIO_FAUD_INTBUS_CLKSEL_AUDIO_XTAL", CLK_AUDIO_FAUD_INTBUS_CLKSEL_AUDIO_XTAL},
    {"AUDIO_FAUD_INTBUS_CLKSEL_266M",       CLK_AUDIO_FAUD_INTBUS_CLKSEL_266M},
    {"AUDIO_FAUD_INTBUS_CLKSEL_133M",       CLK_AUDIO_FAUD_INTBUS_CLKSEL_133M},
    {"AUDIO_FAUD_INTBUS_CLKSEL_XPLL_DIV4",  CLK_AUDIO_FAUD_INTBUS_CLKSEL_XPLL_DIV4},
    {"DSP_HCLK_CLKSEL_XTAL",                CLK_DSP_HCLK_CLKSEL_XTAL},
    {"DSP_HCLK_CLKSEL_XTAL_DIV2",           CLK_DSP_HCLK_CLKSEL_XTAL_DIV2},
    {"DSP_HCLK_CLKSEL_F32K",                CLK_DSP_HCLK_CLKSEL_F32K},
    {"DSP_HCLK_CLKSEL_DSPPLL",              CLK_DSP_HCLK_CLKSEL_DSPPLL},
    {"DSP_HCLK_CLKSEL_DSPPLL_DIV2",         CLK_DSP_HCLK_CLKSEL_DSPPLL_DIV2},
    {"DSP_HCLK_CLKSEL_DSPPLL_DIV4",         CLK_DSP_HCLK_CLKSEL_DSPPLL_DIV4},
    {"DSP_HCLK_CLKSEL_DSPPLL_DIV8",         CLK_DSP_HCLK_CLKSEL_DSPPLL_DIV8},
    {"DSP_HCLK_CLKSEL_XPLL_DIV4",           CLK_DSP_HCLK_CLKSEL_XPLL_DIV4},
    {"INFRA_BUS_CLKSEL_XTAL",               CLK_INFRA_BUS_CLKSEL_XTAL},
    {"INFRA_BUS_CLKSEL_F32K",               CLK_INFRA_BUS_CLKSEL_F32K},
    {"INFRA_BUS_CLKSEL_133M",               CLK_INFRA_BUS_CLKSEL_133M},
    {"INFRA_BUS_CLKSEL_DIV_120M",           CLK_INFRA_BUS_CLKSEL_DIV_120M},
    {"INFRA_BUS_CLKSEL_DIV_100M",           CLK_INFRA_BUS_CLKSEL_DIV_100M},
    {"AUDSYS_BUS_CLKSEL_XTAL",              CLK_AUDSYS_BUS_CLKSEL_XTAL},
    {"AUDSYS_BUS_CLKSEL_F32K",              CLK_AUDSYS_BUS_CLKSEL_F32K},
    {"AUDSYS_BUS_CLKSEL_266M",              CLK_AUDSYS_BUS_CLKSEL_266M},
    {"AUDSYS_BUS_CLKSEL_DIV_200M",          CLK_AUDSYS_BUS_CLKSEL_DIV_200M},
    {"PSRAM_CLKSEL_400M",                   CLK_PSRAM_CLKSEL_400M},
    {"PSRAM_CLKSEL_DIV_300M",               CLK_PSRAM_CLKSEL_DIV_300M},
    {"PSRAM_CLKSEL_DIV_200M",               CLK_PSRAM_CLKSEL_DIV_200M},
    {"PSRAM_CLKSEL_DIV_150M",               CLK_PSRAM_CLKSEL_DIV_150M},
    {"PSRAM_CLKSEL_DIV_120M",               CLK_PSRAM_CLKSEL_DIV_120M},
    {"PSRAM_CLKSEL_DIV_100M",               CLK_PSRAM_CLKSEL_DIV_100M},
    {"PSRAM_CLKSEL_DIV_85p7M",              CLK_PSRAM_CLKSEL_DIV_85p7M},
    {"PSRAM_CLKSEL_DIV_75M",                CLK_PSRAM_CLKSEL_DIV_75M},
    {"TRACE_CLKSEL_XTAL",                   CLK_TRACE_CLKSEL_XTAL},
    {"TRACE_CLKSEL_DIV_300M",               CLK_TRACE_CLKSEL_DIV_300M},
    {"PSRAM_AXI_CLKSEL_XTAL",               CLK_PSRAM_AXI_CLKSEL_XTAL},
    {"PSRAM_AXI_CLKSEL_PSRAM",              CLK_PSRAM_AXI_CLKSEL_PSRAM},
    {"FLASH_CLKSEL_XTAL",                   CLK_FLASH_CLKSEL_XTAL},
    {"FLASH_CLKSEL_DIV_120M",               CLK_FLASH_CLKSEL_DIV_120M},
    {"FLASH_CLKSEL_DIV_60M",                CLK_FLASH_CLKSEL_DIV_60M},
    {"GCPU_CLKSEL_XTAL",                    CLK_GCPU_CLKSEL_XTAL},
    {"GCPU_CLKSEL_DIV_300M",                CLK_GCPU_CLKSEL_DIV_300M},
    {"ECC_CLKSEL_XTAL",                     CLK_ECC_CLKSEL_XTAL},
    {"ECC_CLKSEL_DIV_300M",                 CLK_ECC_CLKSEL_DIV_300M},
    {"SPIM0_CLKSEL_XTAL",                   CLK_SPIM0_CLKSEL_XTAL},
    {"SPIM0_CLKSEL_DIV_200M",               CLK_SPIM0_CLKSEL_DIV_200M},
    {"SPIM0_CLKSEL_171p43M",                CLK_SPIM0_CLKSEL_171p43M},
    {"SPIM0_CLKSEL_133M",                   CLK_SPIM0_CLKSEL_133M},
    {"SPIM1_CLKSEL_XTAL",                   CLK_SPIM1_CLKSEL_XTAL},
    {"SPIM1_CLKSEL_DIV_200M",               CLK_SPIM1_CLKSEL_DIV_200M},
    {"SPIM1_CLKSEL_171p43M",                CLK_SPIM1_CLKSEL_171p43M},
    {"SPIM1_CLKSEL_133M",                   CLK_SPIM1_CLKSEL_133M},
    {"SPIS_CLKSEL_XTAL",                    CLK_SPIS_CLKSEL_XTAL},
    {"SPIS_CLKSEL_DIV_400M",                CLK_SPIS_CLKSEL_DIV_400M},
    {"I2C_CLKSEL_XTAL",                     CLK_I2C_CLKSEL_XTAL},
    {"I2C_CLKSEL_DIV_120M",                 CLK_I2C_CLKSEL_DIV_120M},
    {"DBG_CLKSEL_XTAL",                     CLK_DBG_CLKSEL_XTAL},
    {"DBG_CLKSEL_F32K",                     CLK_DBG_CLKSEL_F32K},
    {"DBG_CLKSEL_100M",                     CLK_DBG_CLKSEL_100M},
    {"DBG_CLKSEL_133M",                     CLK_DBG_CLKSEL_133M},
    {"SDIOM_CLKSEL_XTAL",                   CLK_SDIOM_CLKSEL_XTAL},
    {"SDIOM_CLKSEL_50M",                    CLK_SDIOM_CLKSEL_50M},
    {"USB_SYS_CLKSEL_XTAL",                 CLK_USB_SYS_CLKSEL_XTAL},
    {"USB_SYS_CLKSEL_50M",                  CLK_USB_SYS_CLKSEL_50M},
    {"USB_SYS_CLKSEL_100M",                 CLK_USB_SYS_CLKSEL_100M},
    {"USB_SYS_CLKSEL_120M",                 CLK_USB_SYS_CLKSEL_120M},
    {"USB_XHCI_CLKSEL_XTAL",                CLK_USB_XHCI_CLKSEL_XTAL},
    {"USB_XHCI_CLKSEL_50M",                 CLK_USB_XHCI_CLKSEL_50M},
    {"USB_XHCI_CLKSEL_100M",                CLK_USB_XHCI_CLKSEL_100M},
    {"USB_XHCI_CLKSEL_133M",                CLK_USB_XHCI_CLKSEL_133M},
    {"AUX_ADC_DIV_CLKSEL_XTAL_DIV_2M",      CLK_AUX_ADC_DIV_CLKSEL_XTAL_DIV_2M},
    {"PWM_DIV_CLKSEL_XTAL_DIV_2M",          CLK_PWM_DIV_CLKSEL_XTAL_DIV_2M},
    {"F32K_CLKSEL_PMU",                     CLK_F32K_CLKSEL_PMU},
    {"F32K_CLKSEL_RTC",                     CLK_F32K_CLKSEL_RTC},
    {"F32K_CLKSEL_XTAL_DIV_32K",            CLK_F32K_CLKSEL_XTAL_DIV_32K},
    {"CM33_HCLK_CLKSEL_XTAL",               CLK_CM33_HCLK_CLKSEL_XTAL},
    {"CM33_HCLK_CLKSEL_DIV_300M",           CLK_CM33_HCLK_CLKSEL_DIV_300M},
    {"CM33_HCLK_CLKSEL_DIV_200M",           CLK_CM33_HCLK_CLKSEL_DIV_200M},
    {"CM33_HCLK_CLKSEL_F32K",               CLK_CM33_HCLK_CLKSEL_F32K},
    {"DSPPLL_CLKSEL_300M",                  CLK_DSPPLL_CLKSEL_300M},
    {"DSPPLL_CLKSEL_400M",                  CLK_DSPPLL_CLKSEL_400M},
    {"DSPPLL_CLKSEL_500M",                  CLK_DSPPLL_CLKSEL_500M},
    {"DSPPLL_CLKSEL_600M",                  CLK_DSPPLL_CLKSEL_600M},
    {"TOPPLL_CLKSEL_1200M",                 CLK_TOPPLL_CLKSEL_1200M},
    {"TOPPLL_CLKSEL_1194M",                 CLK_TOPPLL_CLKSEL_1194M},
};
#define CLKSEL_STR_TBL_SIZE  (sizeof(clksel_str_tbl) / sizeof(struct dvt_clk_str_to_val_t))

static char *clock_sel_id_str_tbl[] = {
    [HAL_CLOCK_SEL_AUDIO_FAUDIO] = "HAL_CLOCK_SEL_AUDIO_FAUDIO",
    [HAL_CLOCK_SEL_AUDIO_FASM] = "HAL_CLOCK_SEL_AUDIO_FASM",
    [HAL_CLOCK_SEL_AUDIO_FASYS] = "HAL_CLOCK_SEL_AUDIO_FASYS",
    [HAL_CLOCK_SEL_AUDIO_HAPLL] = "HAL_CLOCK_SEL_AUDIO_HAPLL",
    [HAL_CLOCK_SEL_AUDIO_FAUD_INTBUS] = "HAL_CLOCK_SEL_AUDIO_FAUD_INTBUS",
    [HAL_CLOCK_SEL_DSP_HCLK] = "HAL_CLOCK_SEL_DSP_HCLK",
    [HAL_CLOCK_SEL_INFRA_BUS] = "HAL_CLOCK_SEL_INFRA_BUS",
    [HAL_CLOCK_SEL_AUDSYS_BUS] = "HAL_CLOCK_SEL_AUDSYS_BUS",
    [HAL_CLOCK_SEL_PSRAM] = "HAL_CLOCK_SEL_PSRAM",
    [HAL_CLOCK_SEL_TRACE] = "HAL_CLOCK_SEL_TRACE",
    [HAL_CLOCK_SEL_PSRAM_AXI] = "HAL_CLOCK_SEL_PSRAM_AXI",
    [HAL_CLOCK_SEL_FLASH] = "HAL_CLOCK_SEL_FLASH",
    [HAL_CLOCK_SEL_GCPU] = "HAL_CLOCK_SEL_GCPU",
    [HAL_CLOCK_SEL_ECC] = "HAL_CLOCK_SEL_ECC",
    [HAL_CLOCK_SEL_SPIM0] = "HAL_CLOCK_SEL_SPIM0",
    [HAL_CLOCK_SEL_SPIM1] = "HAL_CLOCK_SEL_SPIM1",
    [HAL_CLOCK_SEL_SPIS] = "HAL_CLOCK_SEL_SPIS",
    [HAL_CLOCK_SEL_I2C] = "HAL_CLOCK_SEL_I2C",
    [HAL_CLOCK_SEL_DBG] = "HAL_CLOCK_SEL_DBG",
    [HAL_CLOCK_SEL_SDIOM] = "HAL_CLOCK_SEL_SDIOM",
    [HAL_CLOCK_SEL_USB_SYS] = "HAL_CLOCK_SEL_USB_SYS",
    [HAL_CLOCK_SEL_USB_XHCI] = "HAL_CLOCK_SEL_USB_XHCI",
    [HAL_CLOCK_SEL_AUX_ADC_DIV] = "HAL_CLOCK_SEL_AUX_ADC_DIV",
    [HAL_CLOCK_SEL_PWM_DIV] = "HAL_CLOCK_SEL_PWM_DIV",
    [HAL_CLOCK_SEL_F32K] = "HAL_CLOCK_SEL_F32K",
    [HAL_CLOCK_SEL_CM33_HCLK] = "HAL_CLOCK_SEL_CM33_HCLK",
    [HAL_CLOCK_SEL_APLL12_CK_DIV0] = "HAL_CLOCK_SEL_APLL12_CK_DIV0",
    [HAL_CLOCK_SEL_APLL12_CK_DIV3] = "HAL_CLOCK_SEL_APLL12_CK_DIV3",
    [HAL_CLOCK_SEL_APLL12_CK_DIV6] = "HAL_CLOCK_SEL_APLL12_CK_DIV6",
    [HAL_CLOCK_SEL_DSP_PLL] = "HAL_CLOCK_SEL_DSP_PLL",
    [HAL_CLOCK_SEL_TOP_PLL] = "HAL_CLOCK_SEL_TOP_PLL",
};

static uint8_t _cli_lp_dvt_clk_select(uint8_t len, char *param[])
{
    uint32_t clock_id = atoi(param[0]);
    uint32_t clksel = 0;

    if (strncmp(param[1], "CLK_", 4) == 0) {
        param[1] += 4;
    }

    for (unsigned int i = 0; i < CLKSEL_STR_TBL_SIZE; i++) {
        if (strcmp(param[1], clksel_str_tbl[i].str) == 0) {
            clksel = clksel_str_tbl[i].val;
            break;
        }
    }
    if (clksel == 0) {
        printf("hal_clock_mux_select: invalid clksel value '%s'!\r\n", param[1]);
        return 0;
    }

    hal_clock_status_t ret = hal_clock_mux_select((hal_clock_sel_id) clock_id, clksel);
    printf("hal_clock_mux_select: clock_id=%lu, clksel=%lu, ret=%d\r\n", clock_id, clksel, ret);

    return 0;
}


static char *clksel_to_str(uint32_t clksel)
{
    for (unsigned int i = 0; i < CLKSEL_STR_TBL_SIZE; i++) {
        if (clksel == clksel_str_tbl[i].val) {
            return clksel_str_tbl[i].str;
        }
    }
    return "UNDEF";
}

static uint8_t _cli_lp_dvt_clk_get_sel(uint8_t len, char *param[])
{
    hal_clock_sel_id clock_id;
    uint32_t clksel = 0;
    hal_clock_status_t ret;

    if (len > 0) {
        clock_id = atoi(param[0]);

        hal_clock_status_t ret = hal_clock_get_selected_mux((hal_clock_sel_id) clock_id, &clksel);
        if (ret == HAL_CLOCK_STATUS_OK) {
            printf("hal_clock_mux_select: clock_id=%d, clksel=%s(0x%08lX)\r\n", clock_id, clksel_to_str(clksel), clksel);
        } else {
            printf("hal_clock_mux_select: clock_id=%d, ret=%d\r\n", clock_id, ret);
        }
    } else {
        printf("hal_clock_mux_select: List of all clock mux status:\r\n");
        for (clock_id = 0; clock_id < HAL_CLOCK_SEL_MAX; clock_id++) {

            if (clock_sel_id_str_tbl[clock_id] == NULL)
                continue;

            ret = hal_clock_get_selected_mux(clock_id, &clksel);
            if (ret == HAL_CLOCK_STATUS_OK) {
                printf("    clock:%s(%d), clksel=%s(0x%08lX)\r\n", clock_sel_id_str_tbl[clock_id], clock_id, clksel_to_str(clksel), clksel);
            } else {
                printf("    clock:%s(%d), ret=%d\r\n", clock_sel_id_str_tbl[clock_id], clock_id, ret);
            }
        }
    }

    return 0;
}

static uint8_t _cli_lp_dvt_clk_get_rate(uint8_t len, char *param[])
{
    if (len < 1) {
        printf("Usage: get_rate [clock_id]\r\n");
        return 1;
    }

    hal_clock_sel_id clock_id = atoi(param[0]);
    uint32_t hz;

    hal_clock_status_t ret = hal_clock_get_rate(clock_id, &hz);

    printf("hal_clock_get_rate: clock_id:%d, ret=%d, hz=%lu\r\n", clock_id, ret, hz);

    return 0;
}

static uint8_t _cli_lp_dvt_clk_set_rate(uint8_t len, char *param[])
{
    if (len < 2) {
        printf("Usage: get_rate [clock_id] [hz]\r\n");
        return 1;
    }

    hal_clock_sel_id clock_id = atoi(param[0]);
    uint32_t hz = atoi(param[1]);

    hal_clock_status_t ret = hal_clock_set_rate(clock_id, hz);

    printf("hal_clock_set_rate: clock_id:%d, hz=%lu, ret=%d\r\n", clock_id, hz, ret);

    return 0;
}
#ifdef HAL_CLK_CTP_SUPPORT
extern void mt_measure_all_clocks(unsigned int cycle_cnt);
static uint8_t _cli_lp_dvt_clk_freq_meter(uint8_t len, char *param[])
{
    uint32_t cycle_cnt = atoi(param[0]);

    if (len < 1) {
        mt_measure_all_clocks(1);
        return 0;
    }

    if (cycle_cnt < 1 || cycle_cnt > 6) {
        printf("Usage: freq_meter <cycle_cnt>, cycle_cnt: 1~6, default:1\r\n");
        return 1;
    }
    mt_measure_all_clocks(cycle_cnt);
    return 0;
}
#endif /* HAL_CLK_CTP_SUPPORT */
cmd_t clk_lp_dvt_cli_cmds[] = {
    { "enable",       "enable clock",                       _cli_lp_dvt_clk_enable, NULL},
    { "disable",      "disable clock",                      _cli_lp_dvt_clk_disable, NULL},
    { "is_enable",    "list clock enable/disable status",   _cli_lp_dvt_clk_is_enable, NULL},
    { "clksel",       "select clock mux",                   _cli_lp_dvt_clk_select, NULL},
    { "get_clksel",   "get clock selected mux status",      _cli_lp_dvt_clk_get_sel, NULL},
    { "get_rate",     "get clock rate",                     _cli_lp_dvt_clk_get_rate, NULL},
    { "set_rate",     "set clock rate",                     _cli_lp_dvt_clk_set_rate, NULL},
#ifdef HAL_CLK_CTP_SUPPORT
    { "freq_meter",   "<cycle_cnt:1~6> measure all the freq_meter ID",      _cli_lp_dvt_clk_freq_meter, NULL},
#endif  /* HAL_CLK_CTP_SUPPORT */
    { NULL, NULL, NULL, NULL }
};
#endif /* MTK_LP_DVT_CLI_ENABLE */
