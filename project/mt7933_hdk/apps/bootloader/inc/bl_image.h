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

#ifndef __BL_IMAGE_H__
#define __BL_IMAGE_H__


#include <stdint.h>


#include "bl_region.h"
#include "bl_status.h"
#include "memory_map.h"


/***************************************************************************
 * DATA STRUCTURES
 ***************************************************************************/


typedef struct {
    uint32_t        sp;
    uint32_t        pc;
} bl_boot_vector_t;


/**
 * This is used to keep boot up information for reboot loader.
 *
 * Although this is a global variable, but it ought to be accessed only by
 * macros and functions from bl_image.c and bl_image.h.
 *
 * @see bl_last_image_boot().
 */
extern bl_boot_vector_t  g_bl_boot_vector;


bl_status_t bl_image_get_boot_vector(uint32_t           addr,
                                     bl_boot_vector_t  *vector);


bl_status_t bl_image_bootable_add(const bl_region_t *region);


void bl_image_jump(uint32_t sp, uint32_t pc);


bl_status_t bl_image_boot(rom_region_id_t id);


void bl_image_addr_boot(uint32_t pc_addr, uint32_t sp_addr);


bl_status_t bl_image_erase_4kb_aligned(rom_region_id_t id);



/**
 * This macros boots into last image.
 *
 * This macro jumps to last image during reboot and never return.
 *
 * @note    uses zero-footprint, does not use stack, does not write to memory.
 */
#define bl_last_image_boot()                                    \
    __asm volatile(                                             \
        "    isb                        \n\t"                   \
        "    dsb                        \n\t"                   \
        "    msr   msp,    %0           \n\t"                   \
        "    msr   msplim, %0           \n\t"                   \
        "    mov   r0,     %1           \n\t"                   \
        "    bx    r0                   \n\t"                   \
        :: "r" (g_bl_boot_vector.sp), "r" (g_bl_boot_vector.pc) : \
    )


#endif /* __BL_IMAGE_H__ */

