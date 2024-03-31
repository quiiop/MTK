/* Copyright Statement:
 *
 * (C) 2020  MediaTek Inc. All rights reserved.
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

#ifndef __BL_CACHE_OPS_H__
#define __BL_CACHE_OPS_H__

#include "common.h"
#include "hal_cache.h"

#define CACHE_CTRL          (CACHE_BASE)
    #define CACHE_SIZE_8KB_BIT      (0x1 << 8)
    #define CACHE_SIZE_16KB_BIT     (0x2 << 8)
    #define CACHE_SIZE_32KB_BIT     (0x3 << 8)
    #define CACHE_SIZE_MASK         (0x3 << 8)
    #define CACHE_ENABLE_BIT        (0x1 << 0)
#define CACHE_OP            (CACHE_BASE + 0x4)
    #define CACHE_FLUSH_ALL_BIT     (0x9 << 1)
    #define CACHE_INV_ALL_BIT       (0x1 << 1)
    #define CACHE_OP_ENABLE_BIT     (0x1 << 0)
    #define CACHE_OP_MASK           (0xF << 1)
#define CACHE_REGION_EN     (CACHE_BASE + 0x2C)
#define CACHE_ENTRY_1       (CACHE_MPU_BASE)
    #define CACHE_ENTRY_CACAHEABLE_BIT  (0x1 << 8)
#define CACHE_END_ENTRY_1   (CACHE_MPU_BASE + 0x40)

typedef struct cache_region
{
    uint32_t start;
    uint32_t end;
} cache_region_t;

#ifdef MTK_BL_CACHE_ENABLE

__STATIC_FORCEINLINE void cache_size_set(hal_cache_size_t cache_size)
{
    switch (cache_size) {
    case HAL_CACHE_SIZE_8KB:
        DRV_ClrReg32(CACHE_CTRL, CACHE_SIZE_MASK);
        DRV_SetReg32(CACHE_CTRL, CACHE_SIZE_8KB_BIT);
        break;
    case HAL_CACHE_SIZE_16KB:
        DRV_ClrReg32(CACHE_CTRL, CACHE_SIZE_MASK);
        DRV_SetReg32(CACHE_CTRL, CACHE_SIZE_16KB_BIT);
        break;
    case HAL_CACHE_SIZE_32KB:
        DRV_SetReg32(CACHE_CTRL, CACHE_SIZE_32KB_BIT);
        break;
    case HAL_CACHE_SIZE_0KB:
        DRV_ClrReg32(CACHE_CTRL, CACHE_SIZE_MASK);
        break;
    default:
        break;
    }
}

__STATIC_FORCEINLINE void cache_disable(void)
{
    DRV_ClrReg32(CACHE_CTRL, CACHE_ENABLE_BIT);
    DRV_WriteReg32(CACHE_REGION_EN, 0);
}
__STATIC_FORCEINLINE void cache_enable(void)
{
    DRV_SetReg32(CACHE_CTRL, CACHE_ENABLE_BIT);
    __DSB();
    __ISB();
}

__STATIC_FORCEINLINE void cache_invalidate_all(void)
{
    DRV_ClrReg32(CACHE_OP, CACHE_OP_MASK);
    DRV_SetReg32(CACHE_OP, CACHE_INV_ALL_BIT);
    DRV_SetReg32(CACHE_OP, CACHE_OP_ENABLE_BIT);
    __DSB();
    __ISB();
}

__STATIC_FORCEINLINE void cache_flush_all(void)
{
    DRV_ClrReg32(CACHE_OP, CACHE_OP_MASK);
    DRV_SetReg32(CACHE_OP, CACHE_FLUSH_ALL_BIT);
    DRV_SetReg32(CACHE_OP, CACHE_OP_ENABLE_BIT);
    __DSB();
    __ISB();
}

__STATIC_FORCEINLINE void cache_flush_invalidate_all(void)
{
    cache_flush_all();
    cache_invalidate_all();
}

__STATIC_FORCEINLINE void cache_region_set
    (struct cache_region *cache_regions, uint32_t num_regions)
{
    for (uint32_t i = 0; i < num_regions; i++) {
        DRV_WriteReg32(CACHE_ENTRY_1 + (i << 2), cache_regions[i].start);
        DRV_WriteReg32(CACHE_END_ENTRY_1 + (i << 2), cache_regions[i].end);
        DRV_SetReg32(CACHE_ENTRY_1 + (i << 2), CACHE_ENTRY_CACAHEABLE_BIT);
        DRV_SetReg32(CACHE_REGION_EN, (0x1 << i));
    }
}

#else

__STATIC_FORCEINLINE void cache_size_set(hal_cache_size_t cache_size) {}
__STATIC_FORCEINLINE void cache_disable(void) {}
__STATIC_FORCEINLINE void cache_enable(void) {}
__STATIC_FORCEINLINE void cache_invalidate_all(void) {}
__STATIC_FORCEINLINE void cache_flush_all(void) {}
__STATIC_FORCEINLINE void cache_flush_invalidate_all(void) {}
__STATIC_FORCEINLINE void cache_region_set
    (struct cache_region *cache_regions, uint32_t num_regions) {}

#endif /* MTK_BL_CACHE_ENABLE */
#endif /* __BL_CACHE_OPS_H__ */
