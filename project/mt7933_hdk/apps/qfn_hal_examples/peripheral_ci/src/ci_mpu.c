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
#include "ci.h"
#include "ci_cli.h"
#include "hal.h"

ci_status_t ci_mpu_region_configure_sample(void)
{
    hal_mpu_config_t mpu_config = {
        /* PRIVDEFENA, HFNMIENA */
        TRUE, TRUE
    };

    EXPECT_VAL(hal_mpu_init(&mpu_config), HAL_MPU_STATUS_OK);

    hal_mpu_region_t region = HAL_MPU_REGION_0;
    hal_mpu_region_config_t region_config;

    memset(&region_config, 0, sizeof(hal_mpu_region_config_t)); // Note, call memset() to make sure all memory regions are set to 0.
    region_config.mpu_region_address = 0x10000000; // The start address of the region that is configured.
    region_config.mpu_region_size = HAL_MPU_REGION_SIZE_1KB; // The size of the region.
    region_config.mpu_region_access_permission = HAL_MPU_FULL_ACCESS; // Access permission is full access.
    region_config.mpu_subregion_mask = 0x00; // Unmask all sub regions.
    region_config.mpu_xn = FALSE; // Enables the instruction access.
    EXPECT_VAL(hal_mpu_region_configure(region, &region_config), HAL_MPU_STATUS_OK);

    EXPECT_VAL(hal_mpu_region_enable(region), HAL_MPU_STATUS_OK); // Enables the region.
    EXPECT_VAL(hal_mpu_enable(), HAL_MPU_STATUS_OK); // Enables the MPU.

    EXPECT_VAL(hal_mpu_deinit(), HAL_MPU_STATUS_OK);
    return CI_PASS;
}


ci_status_t ci_mpu_sample_main(unsigned int portnum)
{
    hal_mpu_deinit();

    struct test_entry test_entry_list[] = {
        {"Sample Code: MPU set location, size and access permissions for each region", ci_mpu_region_configure_sample},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
