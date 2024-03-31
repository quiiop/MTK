/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2020-2021. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */


#include <string.h>


#include "hal.h"
#include "hal_boot.h"
#include "driver_api.h"


#ifdef HAL_GPT_MODULE_ENABLED
#include "hal_gpt_internal.h"
#endif


#ifdef MTK_FOTA_V3_ENABLE
#include "fota_flash_config.h"
#endif


#include "bl_sec.h"
#include "bl_cache_ops.h"
#include "bl_image.h"
#include "bl_region.h"
#include "bl_util.h"
#include "hw_uart.h"
#include "memory_map.h"


/***************************************************************************
 * DATA STRUCTURES
 ***************************************************************************/


/***************************************************************************
 * GLOBAL DATA
 ***************************************************************************/


/**
 * This is used to keep boot up information for reboot loader.
 *
 * Although this is a global variable, but it ought to be accessed only by
 * macros and functions from bl_image.c and bl_image.h.
 *
 * @see bl_last_image_boot().
 */
bl_boot_vector_t    g_bl_boot_vector;


/***************************************************************************
 * GENERATED ACTUAL BOOT VECTORS DATA
 ***************************************************************************/


#define MAX_BOOTABLE_REGIONS    (4)


typedef struct bl_boot_region
{
    uint8_t             count;
    bl_region_t         regions[ MAX_BOOTABLE_REGIONS ];
} bl_boot_region_t;


static bl_boot_region_t _g_boot_regions;


/***************************************************************************
 * PRIVATE FUNCTIONS
 ***************************************************************************/


#define IS_EMPTY( _v )  ( ( _v ) == (uint32_t)0 || ( _v ) == (uint32_t)~0 )


#ifdef MTK_SECURE_BOOT_ENABLE
static bl_status_t _get_sec_boot_region(rom_region_id_t id, bl_region_t *region)
{
    bl_status_t                 status = BL_STATUS_OK;
    uint8_t                     i;

    exit_with_status(region == NULL, BL_STATUS_INVALID_PARAM, id, 0);

    for (i = 0; i < _g_boot_regions.count; i++)
        if (_g_boot_regions.regions[i].id == id)
            break;

    exit_with_status(i >= _g_boot_regions.count, BL_STATUS_NOT_FOUND, id, 0);

    memcpy(region, &_g_boot_regions.regions[i], sizeof(*region));

_exit:

    return status;
}
#endif


#if !defined(MTK_SECURE_BOOT_ENABLE)
static bl_status_t _get_nonsec_boot_region(rom_region_id_t id, bl_region_t *region)
{
    const bl_region_t           *ptr;
    bl_status_t                 status = BL_STATUS_OK;
    uint8_t                     i;

    exit_with_status(region == NULL, BL_STATUS_INVALID_PARAM, id, 0);

    ptr = bl_region_get_table();

    while (ptr->id != id && ptr->id != ROM_REGION_MAX)
        ptr++;

    exit_with_status( ptr->id == ROM_REGION_MAX, BL_STATUS_NOT_FOUND, id, 0 );

    memcpy( region, ptr, sizeof( *ptr ) );

_exit:

    return status;
}
#endif


static bl_status_t _get_boot_region(rom_region_id_t id, bl_region_t *region)
{
#ifdef MTK_SECURE_BOOT_ENABLE
    return _get_sec_boot_region(id, region);
#else
    return _get_nonsec_boot_region(id, region);
#endif
}


/***************************************************************************
 * PUBLIC FUNCTIONS
 ***************************************************************************/


bl_status_t bl_image_bootable_add(const bl_region_t *region)
{
    if ( region->bootable && _g_boot_regions.count < MAX_BOOTABLE_REGIONS )
    {
        memcpy( &_g_boot_regions.regions[ _g_boot_regions.count ],
                region, sizeof( *region ) );
        _g_boot_regions.count++;

        return BL_STATUS_OK;
    }

    return BL_STATUS_NO_SPACE;
}


bl_status_t bl_image_get_boot_vector(uint32_t addr, bl_boot_vector_t *vector)
{
    bl_status_t     status;
    uint32_t        offset = 0;

    status = bl_sec_get_image_offset(addr, &offset);
    exit_with_status(status != BL_STATUS_OK,
                     BL_STATUS_BOOT_VECT_FAIL, status, addr);

    /*
     * If imgtool header not found and current offset is 0, see if there
     * is candidate of sp and pc.
     * If not, fallback to offset 0x80 and check for sp and pc again.
     */

    if (offset == 0) {
        vector->sp = DRV_Reg32(addr);
        vector->pc = DRV_Reg32(addr + 4) | 0x1;

        if (! IS_EMPTY(vector->sp) && ! IS_EMPTY(vector->pc))
            return BL_STATUS_OK;
        offset = 0x80;
    }

    vector->sp = DRV_Reg32(addr + offset);
    vector->pc = DRV_Reg32(addr + offset + 4) | 0x1;

    exit_with_status(IS_EMPTY(vector->sp) || IS_EMPTY(vector->pc),
                     BL_STATUS_BOOT_VECT_FAIL, vector->sp, vector->pc);

    status = BL_STATUS_OK;

_exit:

    return status;
}


void bl_image_jump(uint32_t sp, uint32_t pc)
{
#if defined(__GNUC__)
    hw_uart_printf("jump pc 0x%x, sp 0x%x\r\n", pc, sp);
    if (sp % 4 != 0) {
        hw_uart_puts("sp is not aligned!\n");
        while (1);
    }

    cache_flush_invalidate_all();
    cache_disable();
    bl_image_addr_boot(pc | 1, sp);
    while (1);
#else
#error "FIXME: porting may be needed"
#endif
}


void bl_image_addr_boot(uint32_t pc_addr, uint32_t sp_addr)
{
    __asm volatile(
        "    isb                        \n\t"
        "    dsb                        \n\t"
        /* update MSP and MSPL */
        "    msr   msp, %0              \n\t"
        "    msr   msplim, %0           \n\t"
        /* jump to Reset ISR and never return */
        "    mov   r0, %1               \n\t"
        "    bx    r0                   \n\t"
        :: "r"(sp_addr), "r"(pc_addr) :
    );
}


bl_status_t bl_image_boot(rom_region_id_t id)
{
    bl_status_t         status;
    bl_region_t         region;

    if (BL_STATUS_OK != (status = _get_boot_region(id, &region))) goto _exit;

    if ( ! region.bootable ) goto _exit;

    status = bl_image_get_boot_vector(region.addr, &g_bl_boot_vector);

    if (status == BL_STATUS_OK) {
#ifdef HAL_GPT_MODULE_ENABLED
        volatile uint64_t current_count = gpt_get_current_count();
        hal_boot_set_bootloader_duration(bl_util_gpt_to_ms(current_count));
#endif
        bl_image_jump(g_bl_boot_vector.sp, g_bl_boot_vector.pc);
    }

_exit:

    return status;
}


static bool _is_flash_block_empty(uint32_t start, uint32_t end)
{
    while (start < end) {
        if ((*(uint32_t *)start) != (uint32_t)~0)
            return false;
        start += 4;
    }

    return true;
}


bl_status_t bl_image_erase_4kb_aligned(rom_region_id_t id)
{
    bl_region_t         region = {0};
    bl_status_t         status;
    uint32_t            start, end;
    hal_flash_status_t  hstatus;
    uint32_t            i;

    /* fetch image range information */

    status  = bl_region_get(id, &region);
    if (status != BL_STATUS_OK)
        return status;

    /* calculate star and end addresses */

#define TRIM_4KB (0x00000FFF)
    start = (region.addr & (~TRIM_4KB));
    end   = start + region.size + (region.addr & TRIM_4KB);

    /* erase operation is skipped if the block is clean */

    hw_uart_printf("erase blocks: %d\n", (end - start) >> 12);

    for (i = start; i < end; i += 0x1000) {

        if ((i << 16) == 0)
            hw_uart_printf(".");

        /* detect whether we can skip erasing this block */
        if (_is_flash_block_empty(i, i + 0x1000))
            continue;

        hstatus = hal_flash_erase(i - LOADER_BASE, HAL_FLASH_BLOCK_4K);
        if (hstatus != HAL_FLASH_STATUS_OK) {
            hw_uart_printf("\r\nerase fail 0x% (%d)\r\n", i, hstatus);
            status = BL_STATUS_IMG_ERASE_FAIL;
            break;
        }
    }

    hw_uart_printf("\ndone\n");

    return status;
}

