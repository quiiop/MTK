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

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_MPU_MODULE_ENABLE)

#include "hal_mpu_internal.h"
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
typedef struct {
    uint32_t mpu_region_base_address;   /**< MPU region start address */
    uint32_t mpu_region_end_address;    /**< MPU region end address */
    hal_mpu_access_permission_t mpu_region_access_permission; /**< MPU region access permission */
    uint8_t mpu_subregion_mask;         /**< MPU sub region mask*/
    bool mpu_xn;                        /**< XN attribute of MPU, if set TRUE, execution of an instruction fetched from the corresponding region is not permitted */
} mpu_region_information_t;

/**
* @brief       caculate actual bit value of region size.
* @param[in]   region_size: actual region size.
* @return      corresponding bit value of region size for MPU setting.
*/
static uint32_t caculate_mpu_region_size(uint32_t region_size)
{
    uint32_t count;

    if (region_size < 32) {
        return 0;
    }
    for (count = 0; ((region_size  & 0x80000000) == 0); count++, region_size  <<= 1);
    return 30 - count;
}

ut_status_t ut_hal_mpu(void)
{
    hal_mpu_status_t ret_status;
    hal_mpu_region_t region, region_number;
    hal_mpu_region_config_t region_config;

    mpu_region_information_t region_information[] = {
        /* mpu_region_start_address, mpu_region_end_address, mpu_region_access_permission, mpu_subregion_mask, mpu_xn */
        {(uint32_t)0x80000000UL + (512UL * 1024UL), (uint32_t) 0x80000000UL + (512UL * 1024UL) + 1024UL, HAL_MPU_PRIVILEGED_READ_ONLY, 0x0, FALSE},
    };

    hal_mpu_config_t mpu_config = {
        /* PRIVDEFENA, HFNMIENA */
        TRUE, TRUE
    };

    region_number = (hal_mpu_region_t)(sizeof(region_information) / sizeof(region_information[0]));

    printf("\r\nMPU test enter ...\r\n");

    ret_status = hal_mpu_init(&mpu_config);
    if (HAL_MPU_STATUS_OK != ret_status) {
        printf("MPU test mpu_init fail, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }

    for (region = HAL_MPU_REGION_0; region < region_number; region++) {
        /* Updata region information to be configured */
        region_config.mpu_region_address = region_information[region].mpu_region_base_address;
        region_config.mpu_region_size = (hal_mpu_region_size_t) caculate_mpu_region_size(region_information[region].mpu_region_end_address - region_information[region].mpu_region_base_address);
        region_config.mpu_region_access_permission = region_information[region].mpu_region_access_permission;
        region_config.mpu_subregion_mask = region_information[region].mpu_subregion_mask;
        region_config.mpu_xn = region_information[region].mpu_xn;

        printf("Region config base_addr = 0x%8lX, region_size = %d \r\n",
               region_config.mpu_region_address,
               region_config.mpu_region_size);



        ret_status = hal_mpu_region_configure(region, &region_config);
        if (HAL_MPU_STATUS_OK != ret_status) {
            printf("MPU test region configure fail, ret = %d \r\n", ret_status);

            return UT_STATUS_ERROR;
        }
        ret_status = hal_mpu_region_enable(region);
        if (HAL_MPU_STATUS_OK != ret_status) {
            printf("MPU test region[%d] enable fail, ret = %d \r\n", region, ret_status);

            return UT_STATUS_ERROR;
        }
    }

    /* make sure unused regions are disabled */
    for (; region < HAL_MPU_REGION_MAX; region++) {
        ret_status = hal_mpu_region_disable(region);
        if (HAL_MPU_STATUS_OK != ret_status) {
            printf("MPU test region[%d] disable fail, ret = %d \r\n", region, ret_status);

            return UT_STATUS_ERROR;
        }
    }

    ret_status = hal_mpu_enable();
    if (HAL_MPU_STATUS_OK != ret_status) {
        printf("MPU test mpu_enable fail, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }

#if 0 /* write to readonly address */
    /* write here will cause MemManage Fault */
    *(volatile uint32_t *)(region_config.mpu_region_address) = 0xdeadbeaf;
#endif /* #if 0 ( write to readonly address ) */

    ret_status = hal_mpu_disable();
    if (HAL_MPU_STATUS_OK != ret_status) {
        printf("MPU test mpu_disable, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }
    /* after mpu disable , can write here normally */
    *(volatile uint32_t *)(region_config.mpu_region_address) = 0xdeadbeaf;

    /* restore mpu setting */
    mpu_status_restore();
    hal_mpu_enable();
#if 0 /* write to readonly address */
    /* write here will cause MemManage Fault */
    *(volatile uint32_t *)(region_config.mpu_region_address) = 0xdeadbeaf;
#endif /* #if 0 ( write to readonly address ) */
    hal_mpu_disable();

    ret_status = hal_mpu_deinit();
    if (HAL_MPU_STATUS_OK != ret_status) {
        printf("MPU test mpu_deinit, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }

    printf("\r\nhal_mpu UT OK\r\n");

    return UT_STATUS_OK;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_MPU_MODULE_ENABLE) */
