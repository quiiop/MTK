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

#include "adsp_reg.h"
#include <driver_api.h>
#include <errno.h>
#include "FreeRTOS.h"
#include "hal_log.h"
#include "hal_nvic.h"
#include "hal_nvic_internal.h"
#include "mt7933.h"

#define DRV_NAME        "mtk-dsp_wdt"

#define DSP_WDT_MODE_EN         (1U << 0)
#define DSP_WDT_MODE_EXT_POL_LOW    (0U << 1)
#define DSP_WDT_MODE_EXT_POL_HIGH   (1U << 1)
#define DSP_WDT_MODE_EXRST_EN       (1U << 2)
#define DSP_WDT_MODE_IRQ_EN     (1U << 3)
#define DSP_WDT_MODE_IRQ_LVL        (1U << 5)
#define DSP_WDT_MODE_DUAL_EN        (1U << 6)
#define DSP_WDT_MODE_KEY        0x22000000U

struct mtk_dsp_wdt_dev {
    void *dsp_wdt_base;
    hal_nvic_irq_t dsp_wdt_irq_id;
};

static struct mtk_dsp_wdt_dev *mtk_dsp_wdt;

static void adsp_wdt_stop(void)
{
    uint32_t reg;

    reg = readl(DSP_WDT_MODE);
    reg &= ~DSP_WDT_MODE_EN;
    reg |= DSP_WDT_MODE_KEY;
    writel(reg, DSP_WDT_MODE);

    log_hal_info("%s wdt mode:0x%x\n", __func__, readl(DSP_WDT_MODE));
}

static void prv_mtk_dsp_wdt_isr(hal_nvic_irq_t irq)
{
    uint32_t reg;

    /*clear irq */
    reg = readl(DSP_WDT_MODE);
    reg |= DSP_WDT_MODE_KEY;
    writel(reg, DSP_WDT_MODE);

    log_hal_error("%s enter\n", __func__);
}

int adsp_wdt_device_init(void)
{
    hal_nvic_status_t err;

    mtk_dsp_wdt = pvPortMalloc(sizeof(*mtk_dsp_wdt));
    if (!mtk_dsp_wdt)
        return -ENOMEM;

    mtk_dsp_wdt->dsp_wdt_irq_id = DSP_WDT_IRQn;

    //err = request_irq(mtk_dsp_wdt->dsp_wdt_irq_id,
    //  prv_mtk_dsp_wdt_isr, IRQ_TYPE_LEVEL_LOW,
    //  DRV_NAME, mtk_dsp_wdt);
    // Set IRQ Priority
    hal_nvic_set_priority(mtk_dsp_wdt->dsp_wdt_irq_id, WDT_IRQ_PRIORITY);

    // Register ISR Handler for IRQn
    err = hal_nvic_register_isr_handler(mtk_dsp_wdt->dsp_wdt_irq_id, prv_mtk_dsp_wdt_isr);
    if (err != HAL_NVIC_STATUS_OK) {
        log_hal_error("%s : failed to request irq (%d)\n",
                      __func__, err);
        return err;
    }

    // Enable IRQn
    hal_nvic_enable_irq(mtk_dsp_wdt->dsp_wdt_irq_id);

    adsp_wdt_stop();

    return 0;
}

int adsp_wdt_device_remove(void)
{
    adsp_wdt_stop();
    //disable IRQn
    hal_nvic_disable_irq(mtk_dsp_wdt->dsp_wdt_irq_id);
    vPortFree(mtk_dsp_wdt);
    mtk_dsp_wdt = NULL;

    return 0;
}
