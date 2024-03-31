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
#include "adsp_reg.h"
#include <driver_api.h>
#include <errno.h>
#include "mtk_hifixdsp_common.h"
#ifdef MTK_DSP_SIGN_ENABLE
#include "scott.h"
#endif /* #ifdef MTK_DSP_SIGN_ENABLE */

int platform_parse_resource(void *data)
{
    struct adsp_chip_info *adsp = data;

    adsp->pa_dram = ADSP_DRAM_PHYSICAL_BASE;
    adsp->dramsize = ADSP_DRAM_SIZE;         /* 0x12E000(1208KB) */
    if (((uint32_t)adsp->pa_dram) & 0x1000) {
        log_hal_error("adsp memory(0x%x) is not 4K-aligned\n",
                      (uint32_t)adsp->pa_dram);
        return -1;
    }
    if (adsp->dramsize < TOTAL_SIZE_SHARED_DRAM_FROM_TAIL) {
        log_hal_error("adsp memroy(0x%x) is not enough for share\n",
                      (uint32_t)adsp->dramsize);
        return -1;
    }

    log_hal_info("[ADSP] pa_dram=0x%x, dramsize=0x%x\n",
                 (uint32_t)adsp->pa_dram,
                 adsp->dramsize);

    /* Parse CFG base */
    adsp->va_cfgreg = (void *)DSP_REG_BASE;
    adsp->pa_cfgreg = (uint32_t)DSP_REG_BASE;
    adsp->cfgregsize = 0x1000;
    log_hal_info("[ADSP] va_cfgreg=%p, cfgregsize=0x%x\n",
                 adsp->va_cfgreg,
                 adsp->cfgregsize);

    /* Parse DTCM */
    adsp->pa_dtcm = DSP_DTCM_BASE;
    adsp->dtcmsize = 0x4C000;
    if (adsp->dtcmsize < TOTAL_SIZE_SHARED_DTCM_FROM_TAIL) {
        log_hal_error("adsp DTCM(0x%x) is not enough for share\n",
                      (uint32_t)adsp->dtcmsize);
        return -1;
    }

    log_hal_info("[ADSP] pa_itcm=0x%x,0x%x, pa_dtcm=0x%x,0x%x\n",
                 (uint32_t)adsp->pa_itcm, adsp->itcmsize,
                 (uint32_t) adsp->pa_dtcm, adsp->dtcmsize);

    return 0;
}

int adsp_shared_base_ioremap(void *data)
{
    int ret = 0;
    phys_addr_t phy_base;
    uint32_t shared_size;
    struct adsp_chip_info *adsp = data;

    /* remap shared-dtcm base */
    shared_size = TOTAL_SIZE_SHARED_DTCM_FROM_TAIL;
    phy_base = adsp->pa_dtcm + adsp->dtcmsize - shared_size;
    adsp->shared_dtcm = (void *)phy_base; //this should be virtual address of dtcm /*devm_ioremap_nocache(dev, phy_base, shared_size);
    if (!adsp->shared_dtcm) {
        log_hal_error("ioremap failed at line %d\n", __LINE__);
        ret = -ENOMEM;
        goto tail;
    }
    log_hal_info("[ADSP] shared_dtcm=%p, shared_size=0x%x\n",
                 adsp->shared_dtcm, shared_size);

    /* remap shared-memory base */
    shared_size = TOTAL_SIZE_SHARED_DRAM_FROM_TAIL;
    phy_base = adsp->pa_dram + adsp->dramsize - shared_size;
    adsp->shared_dram = (void *)phy_base;/* va == pa in rtos */
    log_hal_info("[ADSP] shared_dram vbase=%p, shared_size=0x%x\n",
                 adsp->shared_dram, shared_size);

    /* split shared-dram mblock for details used */
    init_adsp_sysram_reserve_mblock(phy_base, adsp->shared_dram);
tail:
    return ret;
}

/*  Init the basic DSP DRAM address */
static int adsp_memory_remap_init(void)
{
    int err = 0;
    struct adsp_chip_info *adsp;
    int offset;

    adsp = get_adsp_chip_data();
    if (!adsp) {
        err = -1;
        goto TAIL;
    }

    /* Assume: pa_dram >= DSP_PHY_BASE */
    offset = adsp->pa_dram - DRAM_PHYS_BASE_FROM_DSP_VIEW;
    if (offset < 0) {
        log_hal_error("error, dram offset < 0\n");
        err = -1;
        goto TAIL;
    }
    writel(offset, DSP_EMI_BASE_ADDR);

TAIL:
    return err;
}

int adsp_must_setting_early(void)
{
    int ret = 0;

    /* support adsp-power at 'probe' phase */
    ret = adsp_pm_register_early();
    if (ret) {
        log_hal_info("clk set parent fail\n");
        goto TAIL;
    }

    /*
     * HIFIxDSP remap setting
     */
    ret = adsp_memory_remap_init();
    if (ret) {
        log_hal_info("memory remap fail\n");
        goto TAIL;
    }
TAIL:
    return ret;
}

int adsp_clock_power_on(void)
{
    int ret = 0;

    /* Step1: Open ADSP clock */
    ret = adsp_default_clk_init(1);
    if (ret)
        goto TAIL;

    /* Step2: Open ADSP power */
    ret = adsp_power_enable();

TAIL:
    return ret;
}

int adsp_clock_power_off(void)
{
    int ret = 0;

    /* Step1: Close ADSP power-domains */
    adsp_power_disable();

    /* Step2: Close ADSP clock */
    ret = adsp_default_clk_init(0);

    return ret;
}

void hifixdsp_boot_sequence(uint32_t boot_addr)
{
    uint32_t val;

    /* ADSP bootup base */
    writel(boot_addr, DSP_ALTRESETVEC);
    val = readl(DSP_ALTRESETVEC);
    log_hal_info("[ADSP] HIFIxDSP boot from base : 0x%x\n", val);

    /*
     * bit[4]: STATVECTOR_SEL pull high to select external reset vector : altReserVec
     * bit[3]: RunStall pull high
     */
    val = readl(DSP_RESET_SW);
    val |= (0x1 << STATVECTOR_SEL) | (0x1 << RUNSTALL);
    writel(val, DSP_RESET_SW);

    /* bit[1] DReset & bit[0] BReset pull high */
    val = readl(DSP_RESET_SW);
    val |= (0x1 << BRESET_SW) | (0x1 << DRESET_SW);
    writel(val, DSP_RESET_SW);

    /* bit[1] DReset & bit[0] BReset  pull low */
    val = readl(DSP_RESET_SW);
    val &= ~((uint32_t)((0x1 << BRESET_SW) | (0x1 << DRESET_SW)));
    writel(val, DSP_RESET_SW);
}

void hifixdsp_release_sequence(void)
{
    uint32_t val;
    /* set bit[0] to 1: Enable PDebug */
    val = readl(DSP_PDEBUGBUS0);
    val |= (0x1 << PDEBUG_ENABLE);
    writel(val, DSP_PDEBUGBUS0);

    /* clear bit[3] to 0: pull down RUN_STALL */
    val = readl(DSP_RESET_SW);
    val &= ~((uint32_t)(0x1 << RUNSTALL));
    writel(val, DSP_RESET_SW);
}

void hifixdsp_shutdown(void)
{
    uint32_t val;

    /* Clear to 0 firstly */
    val = 0x0;
    writel(val, DSP_RESET_SW);

    /* RUN_STALL pull high again to reset */
    val = readl(DSP_RESET_SW);
    val |= (0x1 << RUNSTALL);
    writel(val, DSP_RESET_SW);
}

static
int adsp_update_memory_protect_info(void)
{
    /* code will be added later */
    return 0;
}

int adsp_misc_setting_after_poweron(void)
{
    int ret = 0;

    ret = adsp_update_memory_protect_info();
    return ret;
}

int adsp_remove_setting_after_shutdown(void)
{
    int ret = 0;

    writel(0x00, DSP_AUDIO_DSP2SPM_INT);

    return ret;
}

int adsp_shutdown_notify_check(void)
{
    int ret = 0;

    return ret;
}

void get_adsp_firmware_size_addr(void **addr, size_t *size)
{
    uint32_t fw_addr = DSP_ROM_BASE;
    uint32_t fw_size = DSP_ROM_SIZE;

#ifdef MTK_DSP_SIGN_ENABLE
    if (!IS_ADDR_IN_FLASH(fw_addr)) {
        /*
        * Covert a flash storage address to a CM33 local bus address.
        * Strip using CM33 local bus address.
        * Convert a CM33 local bus address to a flash storage address.
        */
        fw_addr = _TO_VFLASH_(fw_addr);
        (void)scott_image_strip(&fw_addr, &fw_size);
        fw_addr = _TO_STORAGE_(fw_addr);
    } else {
        /* strip using a bus address */
        (void)scott_image_strip(&fw_addr, &fw_size);
    }
#endif /* #ifdef MTK_DSP_SIGN_ENABLE */

    *addr = (void *)fw_addr;
    *size = (size_t)fw_size;
}

void *get_adsp_reg_base(void)
{
    struct adsp_chip_info *adsp;

    adsp = get_adsp_chip_data();
    if (!adsp)
        return NULL;

    /*
     * adsp->va_cfgreg : DSP-CFG virtual base,
     * which must be ioremapped before first-use.
     */
    return adsp->va_cfgreg;
}

