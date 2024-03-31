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
 * WARRANTIES OF MERCHANTABILITY, altNESS FOR A PARTICULAR PURPOSE OR
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


#include "memory_map.h"


#include "bl_mem.h"


#define MEM_PAIR_MAX    (10)



static struct mem_pair g_mem_pairs[ MEM_PAIR_MAX ] = { 
    { PHY_SYSRAM_BASE, SYSRAM_SIZE    },
    { PHY_PRAM_BASE,   MEM_BLOCK_SIZE },
    { PHY_FLASH_BASE,  MEM_BLOCK_SIZE },
};


#define NOT_USED( _p) \
    ( \
        (_p).addr == 0 && (_p).len == 0 \
    )


#define CONTAIN( _p, _a, _l ) \
    ( \
        (_p).addr <= (_a) &&   \
        ( (_a) + (_l) ) <= ( (_p).addr + (_p).len ) \
    )


#define ADDR_ALIGNED( _p, _a ) ( (_p).addr == _a )


#define TAIL_ALIGNED( _p, _e ) ( (_e) == ( (_p).addr + (_p).len ) )


#define D(_fmt...) do{}while(0)


#ifdef BL_MEM_PRINTF
#include <stdio.h>
#undef D
#define D(_fmt...) printf(_fmt)
#endif


static bool _bl_mem_ins(struct mem_pair *pair, int size, uint32_t addr, uint32_t len)
{
    int neu = -1, alt = -1;
    int i;

    D("insert 0x%x + 0x%x\n", addr, len);
    
    for (i = 0; i < size; i++)
    {
        // find an empty pair
        if (neu < 0 && NOT_USED(pair[i]))
            neu = i;

        // find a pair to insert into
        if (alt < 0 && CONTAIN(pair[i], addr, len))
            alt = i;

        if (alt >= 0) {
            if (ADDR_ALIGNED(pair[alt], addr)) {
                if (pair[alt].len != len) {
                    pair[alt].addr += len;
                    pair[alt].len  -= len;
                } else {
                    pair[alt].addr  = 0;
                    pair[alt].len   = 0;
                }
                break;
            } else if (TAIL_ALIGNED(pair[alt], addr + len)) {
                pair[alt].len  -= len;
                break;
            } else if (neu >= 0) {
                uint32_t alt_end = pair[alt].addr + pair[alt].len;
                pair[alt].len    = addr - pair[alt].addr;
                pair[neu].addr   = addr + len;
                pair[neu].len    = alt_end - addr - len;
                break;
            }             
        }
    }
    
    return i < size;
}


bool bl_mem_ins(uint32_t addr, uint32_t len)
{
    addr = HAL_CACHE_VIRTUAL_TO_PHYSICAL( addr );
    return _bl_mem_ins(&g_mem_pairs[0], MEM_PAIR_MAX, addr, len);
}


struct mem_pair *bl_mem_get(uint32_t *size)
{
    *size = MEM_PAIR_MAX;
    return &g_mem_pairs[0];
}

