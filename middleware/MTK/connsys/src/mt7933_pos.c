/*
 * Copyright (C) 2015 MediaTek Inc.
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

//---------------------------------------------------------------------------
#include <string.h>
#include "FreeRTOS.h"
#include <task.h>
#include "mt7933_pos.h"
#include "hal_spm.h"
#include "hal_gpt.h"
#include "hal_log.h"
#include "hal_nvic.h"

//---------------------------------------------------------------------------
int consys_polling_chipid(void)
{
#define TIMEOUT_US        20000
    uint32_t __start = 0, __dur = 0, __now = 0;
    uint32_t ver = 0;

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__start);
    do {
        ver = CONSYS_REG_READ(CONN_INFRA_CFG_START + CONN_HW_VER_OFFSET);
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__now);
        hal_gpt_get_duration_count(__start, __now, &__dur);
        if (ver == CONN_HW_VER_E1 || ver == CONN_HW_VER_E2)
            break;
    } while (__dur <= TIMEOUT_US);

    if (__dur > TIMEOUT_US) {
        log_hal_info("error connsys ver: %08x, expect: %08x or %08x\r\n",
                     ver, CONN_HW_VER_E1, CONN_HW_VER_E2);
        return -1;
    }

    log_hal_info("connsys ver: 0x%08x(%s)\r\n", ver, ver == CONN_HW_VER_E1 ? "E1" : "E2");
    return 0;
}

//---------------------------------------------------------------------------
void consys_emi_set_remapping_reg(void)
{
    unsigned int addr = 0;

    // remap to CBTOP(efuse) address
    addr = CONN2AP_REMAP_MCU_EMI_BASE_ADDR;
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, CONN2AP_REMAP_MCU_EMI_ADDR_VAL, 0, 12, 20);
    // remap to flash address
    addr = CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR;
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, CONN2AP_REMAP_MD_SHARE_EMI_ADDR_VAL, 0, 16, 20);
    // remap to psram address
    addr = CONN2AP_REMAP_GPS_EMI_BASE_ADDR;
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, CONN2AP_REMAP_GPS_EMI_ADDR_VAL, 0, 12, 20);
    // related to ready bit
    addr = CONN2AP_REMAP_WF_PERI_BASE_ADDR;
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, CONN2AP_REMAP_WF_PERI_ADDR_VAL, 0, 12, 20);
    // remap to sysram
    addr = CONN2AP_REMAP_BT_PERI_BASE_ADDR;
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, CONN2AP_REMAP_BT_PERI_ADDR_VAL, 0, 12, 20);
    //related to XO_CTL_TOP_REG
    addr = CONN2AP_REMAP_GPS_PERI_BASE_ADDR;
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, CONN2AP_REMAP_GPS_PERI_ADDR_VAL, 0, 12, 20);
    // related to 32k clock ctl
    addr = CONN2AP_REMAP_SCPSYS_SRAM_BASE_ADDR;
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, CONN2AP_REMAP_SCPSYS_SRAM_ADDR_VAL, 0, 12, 20);
}

/*---------------------------------------------------------------------------*/
void consys_set_emi_addr_range(void)
{
    unsigned int addr = 0;

    addr = CONN_INFRA_CONN2AP_EMI_DET_START_ADDR;
    CONSYS_REG_WRITE(addr, CONN_INFRA_CONN2AP_EMI_DET_START_VAL);
    addr = CONN_INFRA_CONN2AP_EMI_DET_END_ADDR;
    CONSYS_REG_WRITE(addr, CONN_INFRA_CONN2AP_EMI_DET_END_VAL);
}

/*---------------------------------------------------------------------------*/
void connsys_sysram_hwctl(void)
{
    unsigned int addr = 0;

    addr = CONN_INFRA_RGU_SYSRAM_HWCTL_PDN_ADDR;
    CONSYS_REG_WRITE(addr, CONN_INFRA_RGU_SYSRAM_HWCTL_PDN_VAL);
    addr = CONN_INFRA_RGU_SYSRAM_HWCTL_SLP_ADDR;
    CONSYS_REG_WRITE(addr, CONN_INFRA_RGU_SYSRAM_HWCTL_SLP_VAL);
}

/*---------------------------------------------------------------------------*/
void consys_bus_protect_timeout(void)
{
#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG))
    unsigned int addr = 0, val = 0;
#else /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */
    unsigned int addr = 0;
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */

    // bus access protector
    addr = CONN_INFRA_LIGHT_SECURITY_CTRL;
#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG))
    val = CONSYS_REG_READ(addr);
    log_hal_info("%s(before): addr = 0x%x, value = 0x%x\r\n", __func__, addr, val);
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */
    SET_BIT(addr, BIT(4));
    SET_BIT(addr, BIT(3));
    SET_BIT(addr, BIT(1));
    SET_BIT(addr, BIT(0));
#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG))
    val = CONSYS_REG_READ(addr);
    log_hal_info("%s(after ): addr = 0x%x, value = 0x%x\r\n", __func__, addr, val);
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */
    // set conn_infra_off bus timeout
    addr = CONN_INFRA_OFF_TIMEOUT_SETTING;
#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG))
    val = CONSYS_REG_READ(addr);
    log_hal_info("%s(before): addr = 0x%x, value = 0x%x\r\n", __func__, addr, val);
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, 0x31, 7, 0, 8);
#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG))
    val = CONSYS_REG_READ(addr);
    log_hal_info("%s(after ): addr = 0x%x, value = 0x%x\r\n", __func__, addr, val);
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */
    // enable conn_infra_off bus timeout feature
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, 0xf, 0, 0, 4);
#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG))
    val = CONSYS_REG_READ(addr);
    log_hal_info("%s(after ): addr = 0x%x, value = 0x%x\r\n", __func__, addr, val);
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */
    // set conn_infra_on bus timeout
    addr = CONN_INFRA_ON_TIMEOUT_SETTING;
#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG))
    val = CONSYS_REG_READ(addr);
    log_hal_info("%s(before): addr = 0x%x, value = 0x%x\r\n", __func__, addr, val);
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, 0x0c, 7, 0, 8);
#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG))
    val = CONSYS_REG_READ(addr);
    log_hal_info("%s(after ): addr = 0x%x, value = 0x%x\r\n", __func__, addr, val);
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */

    // enable conn_infra_on bus timeout feature
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, 0xf, 0, 0, 4);
#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG))
    val = CONSYS_REG_READ(addr);
    log_hal_info("%s(after ): addr = 0x%x, value = 0x%x\r\n", __func__, addr, val);
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */
}

//---------------------------------------------------------------------------
void consys_enable_bus_hang_detect(void)
{
    unsigned int addr = 0;

    // enable conn_infra bus detect function & bus timeout value (use common API)
    addr = CONN_INFRA_BUS_HANG_DETECT;
    CONSYS_REG_WRITE(addr, 0x32C8001C);
}

//---------------------------------------------------------------------------
void consys_select_cfg_selection(void)
{
    unsigned int addr = 0;

    // select conn_infra_cfg debug_sel to low power related
    addr = CONN_INFRA_CFG_DBG_MUX_SEL_CONN_INFRA_DBG_SELECTION;
    CONSYS_REG_WRITE_OFFSET_RANGE(addr, 0x0, 0, 0, 3);
}

//---------------------------------------------------------------------------
void ap_bus_req_rising_handler(hal_nvic_irq_t irq)
{
    spm_ctrl(SPM_CTRL_SET_AP_BUS_REQ);

    hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    hal_nvic_enable_irq(irq);
}

//---------------------------------------------------------------------------
void apsrc_req_rising_handler(hal_nvic_irq_t irq)
{
    spm_ctrl(SPM_CTRL_SET_AP_SRC_REQ);

    hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    hal_nvic_enable_irq(irq);
}

//---------------------------------------------------------------------------
void mt7933_conninfra_init(void)
{
    ap_bus_req_rising_handler(CONN_AP_BUS_REQ_HIGH_IRQn);
    // apsrc_req_rising_handler(CONN_APSRC_REQ_HIGH_IRQn);

    // set N10 SWD mode
    SET_BIT(0x30030040, BIT(1));

    // ROW 10~19
    MTCMOS_PWR_ON_CONNSYS_ON;
    // ROW 20~21
    hal_spm_conninfra_sw_reset(true);
    // ROW 23~29
    hal_spm_conninfra_slp_prot_disable();
    // ROW 31
    consys_polling_chipid();
    // ROW 36
    consys_emi_set_remapping_reg();
    // ROW 37
    consys_set_emi_addr_range();
    // ROW 39~40
    connsys_sysram_hwctl();
    // ROW 42
    hal_spm_conninfra_a_die_cfg();
    // ROW 44
    hal_spm_conninfra_afe_wbg_init();
    // ROW 45 [AFE WBG CR]
    hal_spm_conninfra_afe_wbg_cal();
    // ROW 46~47
    hal_spm_conninfra_pll_init();
    // ROW 49~78
    hal_spm_conninfra_osc_init();

    // ROW 77~80 Enable DCM (conn_infra on/off bus clock)
    hal_spm_conninfra_dcm_on();

    // conn_infra OSC wake up speed up
    hal_spm_conninfra_speedup_wakeup();
    log_hal_info("%s: success\r\n", __func__);
}

void connsys_power_on(void)
{
    mt7933_conninfra_init();
}

void connsys_power_off(void)
{
    hal_spm_conninfra_off();
}



