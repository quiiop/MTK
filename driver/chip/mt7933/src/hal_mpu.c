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

#include "hal_mpu.h"

#ifdef HAL_MPU_MODULE_ENABLED

#include "hal_mpu_internal.h"
#include "hal_log.h"
#include "hal_nvic.h"
#include "hal_nvic_internal.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */


extern uint8_t g_mpu_status;
hal_mpu_status_t hal_mpu_init(const hal_mpu_config_t *mpu_config)
{
    hal_mpu_region_t region;
    uint32_t irq_flag;

    /* Parameter check */
    if (mpu_config == NULL) {
        return HAL_MPU_STATUS_INVALID_PARAMETER;
    }

    /* In order to prevent race condition, interrupt should be disabled when query and update global variable which indicates the module status */
    irq_flag = save_and_set_interrupt_mask();

    /* Check module status */
    if (g_mpu_status == MPU_BUSY) {
        /* Restore the previous status of interrupt */
        restore_interrupt_mask(irq_flag);

        return HAL_MPU_STATUS_ERROR_BUSY;
    } else {
        /* Change status to busy */
        g_mpu_status = MPU_BUSY;

        /* Restore the previous status of interrupt */
        restore_interrupt_mask(irq_flag);
    }

    /* Set CTRL register to default value */
    MPU->CTRL = 0;

    /* Update PRIVDEFENA and HFNMIENA bit of CTRL register */
    if (mpu_config->privdefena == TRUE) {
        MPU->CTRL |= MPU_CTRL_PRIVDEFENA_Msk;
    }
    if (mpu_config->hfnmiena == TRUE) {
        MPU->CTRL |= MPU_CTRL_HFNMIENA_Msk;
    }

    /* Update the global variable */
    g_mpu_ctrl.w = MPU->CTRL;
    g_mpu_region_en = 0;

    /* Init global variable for each region */
    for (region = HAL_MPU_REGION_0; region < HAL_CM33_MPU_REGION_MAX; region++) {
        g_mpu_entry[region].RBAR = 0;
        g_mpu_entry[region].RLAR = 0;
    }

    return HAL_MPU_STATUS_OK;
}

hal_mpu_status_t hal_mpu_deinit(void)
{
    hal_mpu_region_t region;

    /* Set CTRL register to default value */
    MPU->CTRL = 0;

    /* Update the global variable */
    g_mpu_ctrl.w = 0;
    g_mpu_region_en = 0;

    /* Reset MPU setting as well as global variables to default value */
    for (region = 0; region < HAL_CM33_MPU_REGION_MAX; region++) {
        ARM_MPU_ClrRegion(region);
        MPU->RBAR = 0;

        /* Update the global variable */
        g_mpu_entry[region].RBAR = 0;
        g_mpu_entry[region].RLAR = 0;
    }

    /* Change status to idle */
    g_mpu_status = MPU_IDLE;

    return HAL_MPU_STATUS_OK;
}

hal_mpu_status_t hal_mpu_enable(void)
{
    /* Enable MPU */
    //MPU->CTRL |= MPU_CTRL_ENABLE_Msk;
    ARM_MPU_Enable(MPU->CTRL);

    /* Update the global variable */
    g_mpu_ctrl.w = MPU->CTRL;

    return HAL_MPU_STATUS_OK;
}

hal_mpu_status_t hal_mpu_disable(void)
{
    /* Disable MPU */
    //MPU->CTRL &= ~MPU_CTRL_ENABLE_Msk;
    ARM_MPU_Disable();

    /* Update the global variable */
    g_mpu_ctrl.w = MPU->CTRL;

    return HAL_MPU_STATUS_OK;
}

hal_mpu_status_t hal_mpu_region_enable(hal_mpu_region_t region)
{
    /* Region is invalid */
    if ((region >= HAL_CM33_MPU_REGION_MAX) || (region < 0)) {
        return HAL_MPU_STATUS_ERROR_REGION;
    }

    /* Enable corresponding region */
    MPU->RNR = (region << MPU_RNR_REGION_Pos);
    MPU->RLAR |= MPU_RLAR_EN_Msk;

    /* Update the global variable */
    g_mpu_entry[region].RLAR = MPU->RLAR;
    g_mpu_region_en |= (1 << region);

    return HAL_MPU_STATUS_OK;
}

hal_mpu_status_t hal_mpu_region_disable(hal_mpu_region_t region)
{
    /* Region is invalid */
    if ((region >= HAL_CM33_MPU_REGION_MAX) || (region < 0)) {
        return HAL_MPU_STATUS_ERROR_REGION;
    }

    /* Disable corresponding region */
    MPU->RNR = (region << MPU_RNR_REGION_Pos);
    MPU->RLAR &= ~MPU_RLAR_EN_Msk;

    /* Update the global variable */
    g_mpu_entry[region].RLAR = MPU->RLAR;
    g_mpu_region_en &= ~(1 << region);

    return HAL_MPU_STATUS_OK;
}

hal_mpu_status_t hal_mpu_region_configure(hal_mpu_region_t region, const hal_mpu_region_config_t *region_config)
{
    uint8_t ap_bit;
    uint32_t region_size;

    /* Region is invalid */
    if ((region >= HAL_CM33_MPU_REGION_MAX) || (region < 0)) {
        return HAL_MPU_STATUS_ERROR_REGION;
    }

    /* Parameter check */
    if (region_config == NULL) {
        return HAL_MPU_STATUS_INVALID_PARAMETER;
    }

    /* The MPU region size is invalid */
    if ((region_config->mpu_region_size <= HAL_MPU_REGION_SIZE_MIN) || (region_config->mpu_region_size >= HAL_MPU_REGION_SIZE_MAX)) {
        return HAL_MPU_STATUS_ERROR_REGION_SIZE;
    }

    /* The MPU region address must be size aligned */
    if (region_config->mpu_region_size < 31) {
        if (region_config->mpu_region_address != (region_config->mpu_region_address & (0xFFFFFFFFUL << (region_config->mpu_region_size + 1)))) {
            return HAL_MPU_STATUS_ERROR_REGION_ADDRESS;
        }
    } else {
        if (region_config->mpu_region_address != 0)
            return HAL_MPU_STATUS_ERROR_REGION_ADDRESS;
    }

    /* The MPU access permission must be valid */
    switch (region_config->mpu_region_access_permission) {
        case HAL_MPU_PRIVILEGED_ACCESS_ONLY:
            ap_bit = 0;
            break;
        case HAL_MPU_FULL_ACCESS:
            ap_bit = 1;
            break;
        case HAL_MPU_PRIVILEGED_READ_ONLY:
            ap_bit = 2;
            break;
        case HAL_MPU_READONLY:
            ap_bit = 3;
            break;
        default:
            return HAL_MPU_STATUS_INVALID_PARAMETER;
    }


    /* Write the region setting to corresponding register */
    MPU->RNR  = (region << MPU_RNR_REGION_Pos);
    MPU->RBAR = (region_config->mpu_region_address & MPU_RBAR_BASE_Msk) | ((ap_bit << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk);
    if (region_config->mpu_region_size < 31) {
        region_size = 1UL << (region_config->mpu_region_size + 1);
        MPU->RLAR = (region_config->mpu_region_address + region_size - 1) & MPU_RLAR_LIMIT_Msk;
    } else {
        MPU->RLAR = (region_config->mpu_region_address + 0xFFFFFFFFUL) & MPU_RLAR_LIMIT_Msk;
    }


    //MPU->RASR = ((region_config->mpu_region_size << MPU_RASR_SIZE_Pos) | (region_config->mpu_region_access_permission << MPU_RASR_AP_Pos) | (region_config->mpu_subregion_mask << MPU_RASR_SRD_Pos));

    /* Set the XN(execution never) bit of RASR if mpu_xn is true */
    if (region_config->mpu_xn == TRUE) {
        MPU->RBAR |= MPU_RBAR_XN_Msk;
    }

    /* Update the global variable */
    g_mpu_entry[region].RBAR = MPU->RBAR;
    g_mpu_entry[region].RLAR = MPU->RLAR;

    return HAL_MPU_STATUS_OK;
}

hal_mpu_status_t hal_mpu_region_ex_configure(hal_mpu_region_t region, const hal_mpu_region_ex_config_t *region_config)
{
    uint8_t ap_bit;

    /* Region is invalid */
    if ((region >= HAL_CM33_MPU_REGION_MAX) || (region < 0)) {
        return HAL_MPU_STATUS_ERROR_REGION;
    }

    /* Parameter check */
    if (region_config == NULL) {
        return HAL_MPU_STATUS_INVALID_PARAMETER;
    }


    /* The MPU region address must be 32 bytes aligned */
    if (region_config->mpu_region_address & (0x1Ful)) {
        return HAL_MPU_STATUS_ERROR_REGION_ADDRESS;
    }

    if (region_config->mpu_region_size & (0x1Ful)) {
        return HAL_MPU_STATUS_ERROR_REGION_SIZE;
    }

    /* The MPU access permission must be valid */
    switch (region_config->mpu_region_access_permission) {
        case HAL_MPU_PRIVILEGED_ACCESS_ONLY:
            ap_bit = 0;
            break;
        case HAL_MPU_FULL_ACCESS:
            ap_bit = 1;
            break;
        case HAL_MPU_PRIVILEGED_READ_ONLY:
            ap_bit = 2;
            break;
        case HAL_MPU_READONLY:
            ap_bit = 3;
            break;
        default:
            return HAL_MPU_STATUS_INVALID_PARAMETER;
    }


    /* Write the region setting to corresponding register */
    MPU->RNR  = (region << MPU_RNR_REGION_Pos);
    MPU->RBAR = (region_config->mpu_region_address & MPU_RBAR_BASE_Msk) | ((ap_bit << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk);
    MPU->RLAR = (region_config->mpu_region_address + region_config->mpu_region_size - 1) & MPU_RLAR_LIMIT_Msk;

    //MPU->RASR = ((region_config->mpu_region_size << MPU_RASR_SIZE_Pos) | (region_config->mpu_region_access_permission << MPU_RASR_AP_Pos) | (region_config->mpu_subregion_mask << MPU_RASR_SRD_Pos));

    /* Set the XN(execution never) bit of RASR if mpu_xn is true */
    if (region_config->mpu_xn == TRUE) {
        MPU->RBAR |= MPU_RBAR_XN_Msk;
    }

    /* Update the global variable */
    g_mpu_entry[region].RBAR = MPU->RBAR;
    g_mpu_entry[region].RLAR = MPU->RLAR;

    return HAL_MPU_STATUS_OK;
}

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifdef HAL_MPU_MODULE_ENABLED */

