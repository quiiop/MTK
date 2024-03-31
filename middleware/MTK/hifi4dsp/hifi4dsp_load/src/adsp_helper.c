/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "adsp_helper.h"
#include "adsp_clk.h"
#include "adsp_ipi.h"
#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
#include "mtk_hifixdsp_common.h"
#include "hal_spm.h"
#include <string.h>

#define IPI_WAKEUP_TIME 1000
static struct adsp_chip_info *adsp_info = NULL;
unsigned int is_from_suspend;

void *get_adsp_chip_data(void)
{
    return (void *)adsp_info;
}

int adsp_device_probe(void)
{
    int ret = 0;
    void *data;

    if (adsp_info) {
        log_hal_error("%s is called twice %p\n", __func__, adsp_info);
        vPortFree(adsp_info);
        adsp_info = NULL;
    }

    data = pvPortMalloc(sizeof(struct adsp_chip_info));
    if (!data) {
        log_hal_error("%s malloc fail\n", __func__);
        return -1;
    }

    memset(data, 0, sizeof(struct adsp_chip_info));

    adsp_info = data;

    ret = platform_parse_resource(data);
    if (ret) {
        log_hal_error("platform_parse_resource failed\n");
        goto tail;
    }

    ret = platform_parse_clock(data); /* clk get */
    if (ret) {
        log_hal_error("platform_parse_clock failed\n");
        goto tail;
    }

    ret = adsp_must_setting_early(); /* clk_parent switch,  dram remap setting, dsp uart pinmux */
    if (ret) {
        log_hal_error("adsp_must_setting_early failed\n");
        goto tail;
    }

    ret = adsp_shared_base_ioremap(data);
    if (ret) {
        log_hal_error("adsp_shared_base_ioremap failed\n");
        goto tail;
    }

    ret = adsp_wdt_device_init();
    if (ret) {
        log_hal_error("adsp_wdt_device_init failed.\n");
        goto tail;
    }

    hifixdsp_boot_sequence(DTCM_PHYS_BASE_FROM_DSP_VIEW);

tail:
    if (ret) {
        vPortFree(data);
        adsp_info = NULL;
    }
    return ret;
}

int adsp_device_remove(void)
{
    /* Release ADSP reset-pin */
    hifixdsp_shutdown();

    /* Close ADSP clock and power-domains */
    adsp_clock_power_off();

    adsp_wdt_device_remove();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_DSP);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    MTCMOS_PWR_DOWN_DSP;

    if (adsp_info) {
        vPortFree(adsp_info);
        adsp_info = NULL;
    }

    return 0;
}

void adsp_init(void)
{
    uint32_t val;

    log_hal_info("%s enter and lock\n", __func__);

    MTCMOS_PWR_ON_DSP;
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_DSP);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    /* init */
    adsp_device_probe();

    /* Set dsp as secure master */
    writel(0x0, 0x41004F00);
    val = readl(0x41004A00);
    val |= (0x1 << 1);
    writel(val, 0x41004a00);

    /* bootup dsp */
}
