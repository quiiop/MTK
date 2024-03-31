/* Copyright Statement:
 *
 * (C) 2005-2030  MediaTek Inc. All rights reserved.
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
#include "ut.h"
#include "memory_attribute.h"
#include "driver_api.h"
#include "hal_spm.h"
#include "hal_clk.h"
#include "hal_gpt.h"

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_CLOCK_MODULE_ENABLE)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#define PLL_CLK_CG                  (TOP_CLK_OFF_BASE + 0x200)
#define XTAL_CLK_CTRL               (TOP_CLK_OFF_BASE + 0x214)
#define PWM_CLK_DIV_CTL             (TOP_CLK_OFF_BASE + 0x21C)
#define AUX_ADC_CLK_CTL             (TOP_CLK_OFF_BASE + 0x218)
#define PSRAM_CLK_CTL               (TOP_CLK_OFF_BASE + 0x260)
#define FLASH_CLK_CTL               (TOP_CLK_OFF_BASE + 0x26C)
#define AXI_CLK_CTL                 (TOP_CLK_OFF_BASE + 0x270)
#define AUDSYS_BUS_CLK_CTL          (TOP_CLK_OFF_BASE + 0x274)
#define PSRAMAXI_CLK_CTL            (TOP_CLK_OFF_BASE + 0x278)
#define I2C_CLK_CTL                 (TOP_CLK_OFF_BASE + 0x290)

unsigned int mt_measure_stable_fmeter_freq(int clk, unsigned int cycle_cnt);
bool __ut_clk_result = true;

void _clk_fmeter_check(const char *func_name, int clk_id, unsigned int expected_freq_khz)
{
    unsigned int f = mt_measure_stable_fmeter_freq(clk_id, 3);
    unsigned int min = expected_freq_khz * 0.992;
    unsigned int max = expected_freq_khz * 1.003;
    if (f < min || f > max) {
        printf("%s: Freq measure expected:%u-%u, measured:%u -- FAILED!\n", func_name, min, max, f);
        __ut_clk_result = false;
        return;
    }

    // printf("  Freq measure result - expected:%u-%u, measured:%u\n", min, max, f);
}

void _clk_mux_check(const char *func_name, hal_clock_sel_id clk, uint32_t expected_mux)
{
    uint32_t mux = 0;
    hal_clock_status_t ret;
    if ((ret = hal_clock_get_selected_mux(clk, &mux)) != HAL_CLOCK_STATUS_OK) {
        printf("%s: Call get mux return error %d -- FAILED!\n", func_name, ret);
        __ut_clk_result = false;
        return;
    }
    if (mux != expected_mux) {
        printf("%s: Unexpected mux value, expected:%08X, get:%08X -- FAILED!\n", func_name, (unsigned int)expected_mux, (unsigned int)mux);
        __ut_clk_result = false;
        return;
    }

    // printf("  Get mux result - expected:%08X, measured:%08X\n", (unsigned int)expected_mux, (unsigned int)mux);
}

bool ut_clk_dsppll_test()
{
    __ut_clk_result = true;
    printf("%s ", __func__);

    hal_clock_enable(HAL_CLOCK_CG_DSP_XTAL);
    hal_clock_enable(HAL_CLOCK_CG_DSP_PLL);

    hal_clock_mux_select(HAL_CLOCK_SEL_DSP_PLL, CLK_DSPPLL_CLKSEL_600M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_DSP_PLL, CLK_DSPPLL_CLKSEL_600M);
    _clk_fmeter_check(__func__, FM_DSPPLL_CK, 600000);

    hal_clock_mux_select(HAL_CLOCK_SEL_DSP_PLL, CLK_DSPPLL_CLKSEL_500M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_DSP_PLL, CLK_DSPPLL_CLKSEL_500M);
    _clk_fmeter_check(__func__, FM_DSPPLL_CK, 500000);

    hal_clock_mux_select(HAL_CLOCK_SEL_DSP_PLL, CLK_DSPPLL_CLKSEL_400M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_DSP_PLL, CLK_DSPPLL_CLKSEL_400M);
    _clk_fmeter_check(__func__, FM_DSPPLL_CK, 400000);

    hal_clock_mux_select(HAL_CLOCK_SEL_DSP_PLL, CLK_DSPPLL_CLKSEL_300M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_DSP_PLL, CLK_DSPPLL_CLKSEL_300M);
    _clk_fmeter_check(__func__, FM_DSPPLL_CK, 300000);

    hal_clock_disable(HAL_CLOCK_CG_DSP_PLL);
    _clk_fmeter_check(__func__, FM_DSPPLL_CK, 0);

    hal_clock_enable(HAL_CLOCK_CG_DSP_PLL);
    _clk_fmeter_check(__func__, FM_DSPPLL_CK, 300000);

    return __ut_clk_result;
}

bool ut_clk_usbpll_test()
{
    __ut_clk_result = true;
    printf("%s ", __func__);

    hal_clock_enable(HAL_CLOCK_CG_USB_CTRL_XTAL);
    hal_clock_enable(HAL_CLOCK_CG_USB_PHY_XTAL);
    hal_clock_enable(HAL_CLOCK_CG_USB_MBIST_XTAL);

    hal_clock_enable(HAL_CLOCK_CG_USB_PLL);
    _clk_fmeter_check(__func__, FM_USBPLL_CK, 192000);

    hal_clock_disable(HAL_CLOCK_CG_USB_PLL);
    _clk_fmeter_check(__func__, FM_USBPLL_CK, 0);

    return __ut_clk_result;
}

bool ut_clk_xpll_test()
{
    __ut_clk_result = true;
    printf("%s ", __func__);

    hal_clock_enable(HAL_CLOCK_CG_XPLL);
    _clk_fmeter_check(__func__, FM_XPLL_CK, 196608);

    hal_clock_disable(HAL_CLOCK_CG_XPLL);
    _clk_fmeter_check(__func__, FM_XPLL_CK, 0);

    return __ut_clk_result;
}

// this test is no longer executed in SLT due to already covered by SLT-wide voltage/clock setting adjustment
bool ut_clk_cm33_test()
{
    __ut_clk_result = true;
    printf("%s ", __func__);

    hal_clock_mux_select(HAL_CLOCK_SEL_CM33_HCLK, CLK_CM33_HCLK_CLKSEL_XTAL);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_CM33_HCLK, CLK_CM33_HCLK_CLKSEL_XTAL);
    _clk_fmeter_check(__func__, FM_CM33_HCLK_PRE_CK, 26000);

    hal_clock_mux_select(HAL_CLOCK_SEL_CM33_HCLK, CLK_CM33_HCLK_CLKSEL_DIV_200M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_CM33_HCLK, CLK_CM33_HCLK_CLKSEL_DIV_200M);
    _clk_fmeter_check(__func__, FM_CM33_HCLK_PRE_CK, 200000);

    hal_clock_mux_select(HAL_CLOCK_SEL_CM33_HCLK, CLK_CM33_HCLK_CLKSEL_DIV_300M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_CM33_HCLK, CLK_CM33_HCLK_CLKSEL_DIV_300M);
    _clk_fmeter_check(__func__, FM_CM33_HCLK_PRE_CK, 300000);

    return __ut_clk_result;
}

bool ut_clk_dsp_hclk_test()
{
    __ut_clk_result = true;
    printf("%s ", __func__);

    hal_clock_enable(HAL_CLOCK_CG_DSP_XTAL);
    hal_clock_enable(HAL_CLOCK_CG_DSP_PLL);
    hal_clock_enable(HAL_CLOCK_CG_XPLL);
    hal_clock_enable(HAL_CLOCK_CG_DSP_HCLK);

    hal_clock_mux_select(HAL_CLOCK_SEL_DSP_HCLK, CLK_DSP_HCLK_CLKSEL_XTAL);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_DSP_HCLK, CLK_DSP_HCLK_CLKSEL_XTAL);
    _clk_fmeter_check(__func__, FM_DSP_CK, 26000);

    hal_clock_disable(HAL_CLOCK_CG_DSP_XTAL);
    _clk_fmeter_check(__func__, FM_DSP_CK, 0);

    hal_clock_enable(HAL_CLOCK_CG_DSP_XTAL);
    _clk_fmeter_check(__func__, FM_DSP_CK, 26000);

    hal_clock_mux_select(HAL_CLOCK_SEL_DSP_HCLK, CLK_DSP_HCLK_CLKSEL_XTAL_DIV2);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_DSP_HCLK, CLK_DSP_HCLK_CLKSEL_XTAL_DIV2);
    _clk_fmeter_check(__func__, FM_DSP_CK, 13000);

    hal_clock_mux_select(HAL_CLOCK_SEL_DSP_HCLK, CLK_DSP_HCLK_CLKSEL_DSPPLL);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_DSP_HCLK, CLK_DSP_HCLK_CLKSEL_DSPPLL);
    _clk_fmeter_check(__func__, FM_DSP_CK, 300000);

    hal_clock_mux_select(HAL_CLOCK_SEL_DSP_HCLK, CLK_DSP_HCLK_CLKSEL_DSPPLL_DIV8);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_DSP_HCLK, CLK_DSP_HCLK_CLKSEL_DSPPLL_DIV8);
    _clk_fmeter_check(__func__, FM_DSP_CK, 37500);

    hal_clock_mux_select(HAL_CLOCK_SEL_DSP_HCLK, CLK_DSP_HCLK_CLKSEL_XPLL_DIV4);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_DSP_HCLK, CLK_DSP_HCLK_CLKSEL_XPLL_DIV4);
    _clk_fmeter_check(__func__, FM_DSP_CK, 49152);

    return __ut_clk_result;
}


bool ut_clk_axi_bus_test()
{
    __ut_clk_result = true;
    printf("%s ", __func__);

    hal_clock_mux_select(HAL_CLOCK_SEL_INFRA_BUS, CLK_INFRA_BUS_CLKSEL_XTAL);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_INFRA_BUS, CLK_INFRA_BUS_CLKSEL_XTAL);
    _clk_fmeter_check(__func__, FM_AXI_BUS_FREE_CK, 26000);

    hal_clock_mux_select(HAL_CLOCK_SEL_INFRA_BUS, CLK_INFRA_BUS_CLKSEL_DIV_120M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_INFRA_BUS, CLK_INFRA_BUS_CLKSEL_DIV_120M);
    _clk_fmeter_check(__func__, FM_AXI_BUS_FREE_CK, 120000);

    hal_clock_mux_select(HAL_CLOCK_SEL_INFRA_BUS, CLK_INFRA_BUS_CLKSEL_133M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_INFRA_BUS, CLK_INFRA_BUS_CLKSEL_133M);
    _clk_fmeter_check(__func__, FM_AXI_BUS_FREE_CK, 133340);

    return __ut_clk_result;
}

bool ut_clk_faudio_test()
{
    __ut_clk_result = true;
    printf("%s ", __func__);

    hal_clock_enable(HAL_CLOCK_CG_AUDIO_XTAL);

    hal_clock_mux_select(HAL_CLOCK_SEL_AUDIO_FAUDIO, CLK_AUDIO_FAUDIO_CLKSEL_AUDIO_XTAL);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_AUDIO_FAUDIO, CLK_AUDIO_FAUDIO_CLKSEL_AUDIO_XTAL);
    _clk_fmeter_check(__func__, FM_FAUDIO_CK, 26000);

    hal_clock_disable(HAL_CLOCK_CG_AUDIO_XTAL);
    _clk_fmeter_check(__func__, FM_FAUDIO_CK, 0);

    hal_clock_enable(HAL_CLOCK_CG_AUDIO_XTAL);
    _clk_fmeter_check(__func__, FM_FAUDIO_CK, 26000);

    hal_clock_mux_select(HAL_CLOCK_SEL_AUDIO_FAUDIO, CLK_AUDIO_FAUDIO_CLKSEL_60M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_AUDIO_FAUDIO, CLK_AUDIO_FAUDIO_CLKSEL_60M);
    _clk_fmeter_check(__func__, FM_FAUDIO_CK, 60000);

    hal_clock_mux_select(HAL_CLOCK_SEL_AUDIO_FAUDIO, CLK_AUDIO_FAUDIO_CLKSEL_XPLL_DIV4);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_AUDIO_FAUDIO, CLK_AUDIO_FAUDIO_CLKSEL_XPLL_DIV4);
    _clk_fmeter_check(__func__, FM_FAUDIO_CK, 49152);

    return __ut_clk_result;
}

bool ut_clk_audsys_bus_test()
{
    __ut_clk_result = true;
    printf("%s ", __func__);

    hal_clock_enable(HAL_CLOCK_CG_AUDSYS_BUS);

    hal_clock_mux_select(HAL_CLOCK_SEL_AUDSYS_BUS, CLK_AUDSYS_BUS_CLKSEL_XTAL);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_AUDSYS_BUS, CLK_AUDSYS_BUS_CLKSEL_XTAL);
    _clk_fmeter_check(__func__, FM_AUDSYS_BUS_CK, 26000);

    hal_clock_mux_select(HAL_CLOCK_SEL_AUDSYS_BUS, CLK_AUDSYS_BUS_CLKSEL_DIV_200M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_AUDSYS_BUS, CLK_AUDSYS_BUS_CLKSEL_DIV_200M);
    _clk_fmeter_check(__func__, FM_AUDSYS_BUS_CK, 200000);

    hal_clock_mux_select(HAL_CLOCK_SEL_AUDSYS_BUS, CLK_AUDSYS_BUS_CLKSEL_DIV_120M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_AUDSYS_BUS, CLK_AUDSYS_BUS_CLKSEL_DIV_120M);
    _clk_fmeter_check(__func__, FM_AUDSYS_BUS_CK, 120000);

    hal_clock_mux_select(HAL_CLOCK_SEL_AUDSYS_BUS, CLK_AUDSYS_BUS_CLKSEL_266M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_AUDSYS_BUS, CLK_AUDSYS_BUS_CLKSEL_266M);
    _clk_fmeter_check(__func__, FM_AUDSYS_BUS_CK, 266667);

    return __ut_clk_result;
}

bool ut_clk_spis_test()
{
    __ut_clk_result = true;
    printf("%s ", __func__);

    hal_clock_enable(HAL_CLOCK_CG_SPIS);

    hal_clock_mux_select(HAL_CLOCK_SEL_SPIS, CLK_SPIS_CLKSEL_XTAL);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_SPIS, CLK_SPIS_CLKSEL_XTAL);
    _clk_fmeter_check(__func__, FM_SPIS_CK, 26000);

    hal_clock_mux_select(HAL_CLOCK_SEL_SPIS, CLK_SPIS_CLKSEL_DIV_400M);
    _clk_mux_check(__func__, HAL_CLOCK_SEL_SPIS, CLK_SPIS_CLKSEL_DIV_400M);
    _clk_fmeter_check(__func__, FM_SPIS_CK, 400000);

    hal_clock_disable(HAL_CLOCK_CG_SPIS);
    _clk_fmeter_check(__func__, FM_SPIS_CK, 0);

    hal_clock_enable(HAL_CLOCK_CG_SPIS);
    _clk_fmeter_check(__func__, FM_SPIS_CK, 400000);

    return __ut_clk_result;
}

bool ut_clk_misc_test()
{
    __ut_clk_result = true;
    printf("%s ", __func__);

    _clk_fmeter_check(__func__, FM_TOP_PLL_DIV2_CK, 600000);
    _clk_fmeter_check(__func__, FM_XTAL_CK, 26000);

    return __ut_clk_result;
}

static bool clk_val_compare(uint32_t val_a, uint32_t val_b, char *clock)
{
    if (val_a != val_b) {
        printf("0x%lx != 0x%lx\n", val_a, val_b);
        printf("clock ut fail at %s !\n", clock);
        return false;
    }
    return true;
}

static void wakeup_gpt_callback(void *user_data)
{
    hal_gpt_stop_timer(HAL_GPT_1);
}



bool (*ut_clk_test_funcs[])(void) = {ut_clk_dsppll_test,
                                     ut_clk_usbpll_test,
                                     ut_clk_xpll_test,
                                     ut_clk_dsp_hclk_test,
                                     ut_clk_axi_bus_test,
                                     ut_clk_faudio_test,
                                     ut_clk_spis_test,
                                     ut_clk_audsys_bus_test,
                                     ut_clk_misc_test
                                    };

static const hal_clock_sel_id clk_clksel_list[] = {
    HAL_CLOCK_SEL_INFRA_BUS,
    HAL_CLOCK_SEL_CM33_HCLK,
    HAL_CLOCK_SEL_FLASH,
    HAL_CLOCK_SEL_F32K,
    HAL_CLOCK_SEL_TRACE,
    HAL_CLOCK_SEL_GCPU,
    HAL_CLOCK_SEL_ECC,
    HAL_CLOCK_SEL_SPIM0,
    HAL_CLOCK_SEL_SPIM1,
    HAL_CLOCK_SEL_SPIS,
    HAL_CLOCK_SEL_I2C,
    HAL_CLOCK_SEL_DBG,
    HAL_CLOCK_SEL_USB_SYS,
    HAL_CLOCK_SEL_USB_XHCI,
    HAL_CLOCK_SEL_AUX_ADC_DIV,
    HAL_CLOCK_SEL_PWM_DIV,
    HAL_CLOCK_SEL_DSP_PLL,
    HAL_CLOCK_SEL_DSP_HCLK,
    HAL_CLOCK_SEL_APLL12_CK_DIV0,
    HAL_CLOCK_SEL_APLL12_CK_DIV3,
    HAL_CLOCK_SEL_APLL12_CK_DIV6
};

char *clk_id[] = {
    "INFRA_BUS",
    "CM33_HCLK",
    "FLASH",
    "F32K",
    "TRACE",
    "GCPU",
    "ECC",
    "SPIM0",
    "SPIM1",
    "SPIS",
    "I2C",
    "DBG",
    "USB_SYS",
    "USB_XHCI",
    "AUX_ADC_DIV",
    "PWM_DIV",
    "DSP_PLL",
    "DSP_HCLK",
    "APLL12_CK_DIV0",
    "APLL12_CK_DIV3",
    "APLL12_CK_DIV6"
};

ut_status_t ut_hal_clock(void)
{
    unsigned int case_id = 0;
    uint32_t num_clk =  sizeof(clk_clksel_list) / sizeof(uint32_t);
    volatile uint32_t clk_sel_vals[num_clk];
    volatile uint32_t tmp, pll_val, xtal_val;
    uint32_t ret = 0U;
    ut_status_t ut_result = UT_STATUS_OK;

    for (case_id = 0; case_id < (sizeof(ut_clk_test_funcs) / sizeof(void *)); case_id++) {
        if (ut_clk_test_funcs[case_id]() == false) {
            ut_result = UT_STATUS_ERROR;
        }
    }

    hal_gpt_init(HAL_GPT_1);
    hal_gpt_register_callback(HAL_GPT_1, wakeup_gpt_callback, NULL);
    hal_gpt_stop_timer(HAL_GPT_1);

    pll_val = HAL_REG_32(PLL_CLK_CG);
    xtal_val = HAL_REG_32(XTAL_CLK_CTRL);
    for (uint32_t i = 0; i < num_clk; ++i) {
        hal_clock_get_selected_mux(clk_clksel_list[i], &clk_sel_vals[i]);
    }

    hal_gpt_start_timer_ms(HAL_GPT_1, 500, HAL_GPT_TIMER_TYPE_ONE_SHOT);

    vTaskDelay(50);
    ret = hal_sleep_manager_enter_sleep_mode(HAL_SLEEP_MODE_SLEEP);

    if (!clk_val_compare(pll_val, HAL_REG_32(PLL_CLK_CG), "PLL_CLK")) {
        ut_result = UT_STATUS_ERROR;
    }

    if (!clk_val_compare(xtal_val, HAL_REG_32(XTAL_CLK_CTRL), "XTAL")) {
        ut_result = UT_STATUS_ERROR;
    }

    for (uint32_t i = 0; i < num_clk; ++i) {
        hal_clock_get_selected_mux(clk_clksel_list[i], &tmp);
        if (!clk_val_compare(clk_sel_vals[i], tmp, clk_id[i])) {
            ut_result = UT_STATUS_ERROR;
        }
    }

    if (ut_result == UT_STATUS_OK) {
        printf("UT CLK OK...\n");
    } else {
        printf("UT CLK FAIL...\n");
    }
    hal_gpt_deinit(HAL_GPT_1);
    return ut_result;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_CLOCK_MODULE_ENABLE) */
