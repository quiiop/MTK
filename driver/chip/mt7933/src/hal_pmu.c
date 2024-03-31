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

#ifdef HAL_PMU_MODULE_ENABLED
#include <common.h>
#include "hal_boot.h"
#include "hal_log.h"
#include "hal_pmu.h"
#include "hal_pmu_wrap_interface.h"
#include "hal_gpt.h"
#include "hal_sleep_manager_internal.h"
#include "memory_attribute.h"

void pmu_buck_norm_slp_vol(uint32_t norm_vol, uint32_t slp_vol)
{
    uint32_t reg;

    reg = DRV_Reg32(PMU_RG_SPM_1);
    DRV_WriteReg32(PMU_RG_SPM_1, ((reg & 0xfe0fe0e0) | (norm_vol << 20) | (norm_vol << 8) | slp_vol)); // set mldo vosel
}

void pmu_mldo_norm_slp_vol(uint32_t norm_vol, uint32_t slp_vol)
{
    uint32_t reg;

    reg = DRV_Reg32(PMU_RG_SPM_3);
    DRV_WriteReg32(PMU_RG_SPM_3, ((reg & 0xfe0fe0e0) | (norm_vol << 20) | (norm_vol << 8) | slp_vol)); // set mldo vosel
}

void pmu_mldo_norm_dvs(uint32_t vosel)
{
    uint32_t reg;

    reg = DRV_Reg32(PMU_RG_SPM_5);
    DRV_WriteReg32(PMU_RG_SPM_5, reg | 0x00000004);                   // set dvs sw ctrl

    reg = DRV_Reg32(PMU_RG_MLDO5);
    DRV_WriteReg32(PMU_RG_MLDO5, ((reg & 0xffe0ffff) | (vosel << 16))); // set mldo vosel
    reg = DRV_Reg32(PMU_RG_MLDO3);
    DRV_WriteReg32(PMU_RG_MLDO3, ((reg & 0xefffffff) | 0x10000000));   // set mldo dvs_en

    delay_us(60);   // delay 60us (2T 32K)

    reg = DRV_Reg32(PMU_RG_MLDO5);
    while ((reg & 0x80000000) != 0x80000000) {                        // wait mldo normal_volse_rdy
        log_hal_debug("%s Wait PMU MLDO DVS done!\n", __FUNCTION__);
        delay_us(60);
        reg = DRV_Reg32(PMU_RG_MLDO5);
    }

    reg = DRV_Reg32(PMU_RG_MLDO3);
    DRV_WriteReg32(PMU_RG_MLDO3, ((reg & 0xefffffff) | 0x00000000));   // release mldo dvs_en

    reg = DRV_Reg32(PMU_RG_SPM_5);
    DRV_WriteReg32(PMU_RG_SPM_5, reg & 0xfffffffb);                   // release dvs sw ctrl
}

void pmu_buck_norm_dvs(uint32_t vosel)
{
    uint32_t reg;

    reg = DRV_Reg32(PMU_RG_SPM_5);
    DRV_WriteReg32(PMU_RG_SPM_5, reg | 0x00000004);                   // set dvs sw ctrl

    reg = DRV_Reg32(PMU_VS1_5);
    DRV_WriteReg32(PMU_VS1_5, ((reg & 0xffe0ffff) | (vosel << 16))); // set buck vosel
    reg = DRV_Reg32(PMU_VS1_3);
    DRV_WriteReg32(PMU_VS1_3, ((reg & 0xefffffff) | 0x10000000));   // set buck dvs_en

    delay_us(60);   // delay 60us (2T 32K)

    reg = DRV_Reg32(PMU_VS1_5);
    while ((reg & 0x80000000) != 0x80000000) {                        // wait buck normal_volse_rdy
        log_hal_debug("%s Wait PMU BUCKD DVS done!\n", __FUNCTION__);
        delay_us(60);
        reg = DRV_Reg32(PMU_VS1_5);
    }

    reg = DRV_Reg32(PMU_VS1_3);
    DRV_WriteReg32(PMU_VS1_3, ((reg & 0xefffffff) | 0x00000000));   // release buck dvs_en

    reg = DRV_Reg32(PMU_RG_SPM_5);
    DRV_WriteReg32(PMU_RG_SPM_5, reg & 0xfffffffb);                   // release dvs sw ctrl
}

void pmu_mldo_buck_norm_dvs_parallel(uint32_t vosel_buck, uint32_t vosel_mldo)
{
    uint32_t reg;

    reg = DRV_Reg32(PMU_RG_SPM_5);
    DRV_WriteReg32(PMU_RG_SPM_5, reg | 0x00000004);                   // set dvs sw ctrl

    reg = DRV_Reg32(PMU_VS1_5);
    DRV_WriteReg32(PMU_VS1_5, ((reg & 0xffe0ffff) | (vosel_buck << 16))); // set buck vosel
    reg = DRV_Reg32(PMU_VS1_3);
    DRV_WriteReg32(PMU_VS1_3, ((reg & 0xefffffff) | 0x10000000));   // set buck dvs_en

    reg = DRV_Reg32(PMU_RG_MLDO5);
    DRV_WriteReg32(PMU_RG_MLDO5, ((reg & 0xffe0ffff) | (vosel_mldo << 16))); // set mldo vosel
    reg = DRV_Reg32(PMU_RG_MLDO3);
    DRV_WriteReg32(PMU_RG_MLDO3, ((reg & 0xefffffff) | 0x10000000));   // set mldo dvs_en

    delay_us(60);   // delay 60us (2T 32K)

    reg = DRV_Reg32(PMU_RG_MLDO5);
    while ((reg & 0x80000000) != 0x80000000) {                        // wait mldo normal_volse_rdy
        log_hal_debug("%s Wait PMU MLDO DVS done!\n", __FUNCTION__);
        delay_us(60);
        reg = DRV_Reg32(PMU_RG_MLDO5);
    }

    reg = DRV_Reg32(PMU_VS1_5);
    while ((reg & 0x80000000) != 0x80000000) {                        // wait buck normal_volse_rdy
        log_hal_debug("%s Wait PMU BUCKD DVS done!\n", __FUNCTION__);
        delay_us(60);
        reg = DRV_Reg32(PMU_VS1_5);
    }

    reg = DRV_Reg32(PMU_VS1_3);
    DRV_WriteReg32(PMU_VS1_3, ((reg & 0xefffffff) | 0x00000000));   // release buck dvs_en

    reg = DRV_Reg32(PMU_RG_MLDO3);
    DRV_WriteReg32(PMU_RG_MLDO3, ((reg & 0xefffffff) | 0x00000000));   // release mldo dvs_en

    reg = DRV_Reg32(PMU_RG_SPM_5);
    DRV_WriteReg32(PMU_RG_SPM_5, reg & 0xfffffffb);                   // release dvs sw ctrl
}

hal_pmu_status_t pmu_set_mldo_voltage_mt7933(hal_pmu_mldo_vosel_t vol)
{
    if (vol == HAL_PMU_MLDO_0p65V) {
        pmu_mldo_norm_dvs(0x3);
    } else if (vol == HAL_PMU_MLDO_0p7V) {
        pmu_mldo_norm_dvs(0x5);
    } else if (vol == HAL_PMU_MLDO_0p8V) {
        pmu_mldo_norm_dvs(0x9);
    } else if (vol == HAL_PMU_MLDO_0p85V) {
        pmu_mldo_norm_dvs(0xB);
    } else {
        log_hal_error("[Error used]\r\n");
        return HAL_PMU_STATUS_ERROR;
    }
    return HAL_PMU_STATUS_OK;
}

hal_pmu_status_t pmu_set_vcore_voltage_mt7933(hal_pmu_vcore_vosel_t vol)
{
    if (vol == HAL_PMU_VCORE_0p65V) {
        pmu_buck_norm_dvs(0x4);
    } else if (vol == HAL_PMU_VCORE_0p7V) {
        pmu_buck_norm_dvs(0x9);
    } else if (vol == HAL_PMU_VCORE_0p8V) {
        pmu_buck_norm_dvs(0x13);
    } else {
        log_hal_error("[Error used]\r\n");
        return HAL_PMU_STATUS_ERROR;
    }
    return HAL_PMU_STATUS_OK;
}

hal_pmu_status_t pmu_set_mldo_slp_vol_mt7933(hal_pmu_mldo_vosel_t vol)
{
    pmu_set_mldo_voltage_mt7933(vol);

    if (vol == HAL_PMU_MLDO_0p65V) {
        pmu_mldo_norm_slp_vol(0x3, 0x03);
    } else if (vol == HAL_PMU_MLDO_0p7V) {
        pmu_mldo_norm_slp_vol(0x5, 0x03);
    } else if (vol == HAL_PMU_MLDO_0p8V) {
        pmu_mldo_norm_slp_vol(0x9, 0x03);
    } else if (vol == HAL_PMU_MLDO_0p85V) {
        pmu_mldo_norm_slp_vol(0xB, 0x03);
    } else {
        log_hal_error("[Error used]\r\n");
        return HAL_PMU_STATUS_ERROR;
    }
    return HAL_PMU_STATUS_OK;
}

hal_pmu_status_t pmu_set_vcore_slp_vol_mt7933(hal_pmu_vcore_vosel_t vol)
{
    pmu_set_vcore_voltage_mt7933(vol);

    if (vol == HAL_PMU_VCORE_0p65V) {
        pmu_buck_norm_slp_vol(0x4, 0x04);
    } else if (vol == HAL_PMU_VCORE_0p7V) {
        pmu_buck_norm_slp_vol(0x9, 0x04);
    } else if (vol == HAL_PMU_VCORE_0p8V) {
        pmu_buck_norm_slp_vol(0x13, 0x04);
    } else {
        log_hal_error("[Error used]\r\n");
        return HAL_PMU_STATUS_ERROR;
    }
    return HAL_PMU_STATUS_OK;
}

ATTR_TEXT_IN_TCM hal_pmu_status_t pmu_mldo_psw_en(void)
{
    uint32_t reg;

    reg = DRV_Reg32(PMU_RG_SPM_1);
    DRV_WriteReg32(PMU_RG_SPM_1, ((reg & 0xfe0fe0e0) | (0x09 << 20) | (0x09 << 8) | 0x04)); // set buck vosel

    reg = DRV_Reg32(PMU_RG_SPM_3);
    DRV_WriteReg32(PMU_RG_SPM_3, ((reg & 0xfe0fe0e0) | (0x09 << 20) | (0x09 << 8) | 0x05)); // set mldo vosel

    reg = DRV_Reg32(PMU_RG_MLDO4);
    DRV_WriteReg32(PMU_RG_MLDO4, ((reg & 0xfffff000) | 0x02));  // set mldo wake peroid

    reg = DRV_Reg32(PMU_RG_SPM_5);
    DRV_WriteReg32(PMU_RG_SPM_5, reg | 0x00000020);

    return HAL_PMU_STATUS_OK;
}

hal_pmu_status_t pmu_init_mt7933(void)
{
    uint32_t reg;

    log_hal_info("pmu init\r\n");

    reg = DRV_Reg32(PMU_RG_SPM_5);
    DRV_WriteReg32(PMU_RG_SPM_5, (reg & 0xfffffff7));  // set parallel mode

    pmu_set_mldo_voltage_mt7933(HAL_PMU_MLDO_0p85V);
    pmu_set_mldo_slp_vol_mt7933(HAL_PMU_MLDO_0p85V);
    log_hal_info("force VSRAM to 0.85V\r\n");

    if (hal_boot_get_hw_ver() == 0x8A10) {
#if defined(HAL_PMU_FORCE_VCORE_0P8V) || defined(PINMUX_BGA_DEFAULT)
        pmu_set_vcore_voltage_mt7933(HAL_PMU_VCORE_0p8V);
        pmu_set_vcore_slp_vol_mt7933(HAL_PMU_VCORE_0p8V);
        log_hal_info("MT7933 VCORE to 0.8V\r\n");
#else /* #if defined(HAL_PMU_FORCE_VCORE_0P8V) || defined(PINMUX_BGA_DEFAULT) */
        pmu_set_vcore_voltage_mt7933(HAL_PMU_VCORE_0p7V);
        pmu_set_vcore_slp_vol_mt7933(HAL_PMU_VCORE_0p7V);
        log_hal_info("MT7931 VCORE to 0.7V\r\n");
#endif /* #if defined(HAL_PMU_FORCE_VCORE_0P8V) || defined(PINMUX_BGA_DEFAULT) */
    } else {
        pmu_set_vcore_voltage_mt7933(HAL_PMU_VCORE_0p8V);
        pmu_set_vcore_slp_vol_mt7933(HAL_PMU_VCORE_0p8V);
        log_hal_info("force VCORE to 0.8V\r\n");
    }
    return HAL_PMU_STATUS_OK;
}

#endif /* #ifdef HAL_PMU_MODULE_ENABLED */
