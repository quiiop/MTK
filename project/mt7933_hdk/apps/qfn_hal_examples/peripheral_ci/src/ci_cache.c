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
#include "ci_cli.h"
#include "hal.h"
#include "hal_cache.h"
#include "ci.h"
#include "memory_map.h"
#include "hal_gpt_internal.h"

ci_status_t ci_cache_set_region_sample(void)
{
    EXPECT_VAL(hal_cache_deinit(), HAL_CACHE_STATUS_OK);
    EXPECT_VAL(hal_cache_init(), HAL_CACHE_STATUS_OK);
    hal_cache_region_t region = HAL_CACHE_REGION_0;
    hal_cache_region_config_t region_config;
    EXPECT_VAL(hal_cache_set_size(HAL_CACHE_SIZE_32KB), HAL_CACHE_STATUS_OK); // Set the total CACHE size.
    region_config.cache_region_address = 0x10000000; // The start address of the region that is cacheable, which must be 4kB aligned.
    region_config.cache_region_size = 0x10000; // The size of the cacheable region.
    EXPECT_VAL(hal_cache_region_config(region, &region_config), HAL_CACHE_STATUS_OK);
    EXPECT_VAL(hal_cache_region_enable(region), HAL_CACHE_STATUS_OK);
    EXPECT_VAL(hal_cache_enable(), HAL_CACHE_STATUS_OK);

    return CI_PASS;
}

ci_status_t ci_cache_flush_invalidate_sample(void)
{
    EXPECT_VAL(hal_cache_deinit(), HAL_CACHE_STATUS_OK);
    EXPECT_VAL(hal_cache_init(), HAL_CACHE_STATUS_OK);
    hal_cache_region_t region = HAL_CACHE_REGION_0;
    hal_cache_region_config_t region_config;
    EXPECT_VAL(hal_cache_set_size(HAL_CACHE_SIZE_32KB), HAL_CACHE_STATUS_OK); // Set the total CACHE size.
    region_config.cache_region_address = 0x10000000; // The start address of the region that is cacheable, which must be 4kB aligned.
    region_config.cache_region_size = 0x10000; // The size of the cacheable region.
    EXPECT_VAL(hal_cache_region_config(region, &region_config), HAL_CACHE_STATUS_OK);
    EXPECT_VAL(hal_cache_region_enable(region), HAL_CACHE_STATUS_OK);
    EXPECT_VAL(hal_cache_enable(), HAL_CACHE_STATUS_OK);

    EXPECT_VAL(hal_cache_flush_multiple_cache_lines(region_config.cache_region_address, region_config.cache_region_size), HAL_CACHE_STATUS_OK);
    EXPECT_VAL(hal_cache_invalidate_multiple_cache_lines(region_config.cache_region_address, region_config.cache_region_size), HAL_CACHE_STATUS_OK);

    return CI_PASS;
}


ci_status_t ci_cache_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Sample Code: Cache set a cacheable region", ci_cache_set_region_sample},
        {"Sample Code: Cache flush or invalidate a cacheable region", ci_cache_flush_invalidate_sample},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
