/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include "adsp_clk.h"
#include "adsp_helper.h"
#include "hal_clk.h"
#include "hal_clock.h"
#include "hal_spm.h"

#if 1
struct mtk_pd *adsp_pd;

static int adsp_enable_clock(void)
{
    int ret = 0;

    ret = hal_clock_enable(HAL_CLOCK_CG_DSP_XTAL);
    if (ret != HAL_CLOCK_STATUS_OK) {
        log_hal_error("clk_enable(HAL_CLOCK_CG_DSP_XTAL) fail %d \n", ret);
        goto err;
    }

    ret = hal_clock_enable(HAL_CLOCK_CG_DSP_PLL);
    if (ret != HAL_CLOCK_STATUS_OK) {
        log_hal_error("clk_enable(HAL_CLOCK_CG_DSP_PLL) fail %d \n", ret);
        goto err1;
    }

    ret = hal_clock_enable(HAL_CLOCK_CG_DSP_HCLK);
    if (ret != HAL_CLOCK_STATUS_OK) {
        log_hal_error("clk_enable(HAL_CLOCK_CG_DSP_HCLK) fail %d \n", ret);
        goto err0;
    }

    hal_clock_mux_select(HAL_CLOCK_SEL_DSP_HCLK,  CLK_DSP_HCLK_CLKSEL_DSPPLL);

    return ret;

err0:
    hal_clock_disable(HAL_CLOCK_CG_DSP_PLL);
err1:
    hal_clock_disable(HAL_CLOCK_CG_DSP_XTAL);
err:
    return ret;
}

static void adsp_disable_clock(void)
{
    hal_clock_disable(HAL_CLOCK_CG_DSP_HCLK);
    hal_clock_disable(HAL_CLOCK_CG_DSP_PLL);
    hal_clock_disable(HAL_CLOCK_CG_DSP_XTAL);
}

int platform_parse_clock(void *data)
{
    return 0;
}

int adsp_default_clk_init(int enable)
{
    int ret = 0;

    log_hal_info("line%d %s (%x)\n", __LINE__, __func__, enable);

    if (enable) {
        ret = adsp_enable_clock();
        if (ret) {
            log_hal_error("failed to adsp_enable_clock: %d\n", ret);
            goto TAIL;
        }
    } else
        adsp_disable_clock();

TAIL:
    return ret;
}

int adsp_pm_register_early()
{
    return 0;
}

int adsp_power_enable(void)
{
    int i;

    /* DSP MTCMOS */
    MTCMOS_PWR_ON_DSP;

    /* SRAM MTCMOS */
    for (i = 0 ; i < 4 ; i++) {
        SRAM_PWR_ON_DSP(i);
        SRAM_PWR_ON_DSP_POOL(i);
    }

    return 0;
}

void adsp_power_disable(void)
{
    int i;

    for (i = 0 ; i < 4 ; i++) {
        SRAM_PWR_DOWN_DSP_POOL(i);
        SRAM_PWR_DOWN_DSP(i);
    }

    MTCMOS_PWR_DOWN_DSP;
}
#else /* #if 1 */
int platform_parse_clock(void *data)
{
    return 0;
}
int adsp_default_clk_init(int enable)
{
    return 0;
}
int adsp_pm_register_early(void)
{
    return 0;
}
int adsp_power_enable(void)
{
    return 0;
}
void adsp_power_disable(void) { ; }
#endif /* #if 1 */
