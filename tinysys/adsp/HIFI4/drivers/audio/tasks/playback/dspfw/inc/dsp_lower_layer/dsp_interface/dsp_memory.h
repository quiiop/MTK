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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

 
#ifndef _DSP_MEMORY_H_
#define _DSP_MEMORY_H_

#include <string.h>
#include "audio_types.h"
#include "dlist.h"
#include "dsp_task.h"




////////////////////////////////////////////////////////////////////////////////
// CONSTANT DEFINITIONS ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define DSP_MEMBLK_EOB_SIZE (sizeof(DSPMEM_BLKTAL))
#define DSP_ADDITIONAL_BYTES_FOR_MEMORY_ARRAY (sizeof(DSPMEM_BLKHDR)+DSP_MEMBLK_EOB_SIZE)

#define DSP_MARK_SRC_PTR    (2)


////////////////////////////////////////////////////////////////////////////////
// TYPE DEFINITIONS ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/* DSPMEM is DSP MEMORY control */
typedef struct DSPMEM_stru
{
    //- 8B, Relative DSP memory head
    DLIST_HEAD          head;

    //- 4B, Relative DSP memory top ptr
    VOID*               mem_top_ptr;

    //- 4B, Relative DSP memory bottom ptr
    VOID*               mem_bottom_ptr;

    //- 1B, sequence of memory block
    U8                  seqMB;

    //- 1B, number of memory block
    U8                  noMB;

    //- 1B, initial flag
    U8                 init_flag;

    //- 1B, reserved
    U8                 _reserved;
    //- 4B, remained memory size
    U32                 rm_size;
} DSPMEM, * DSPMEM_PTR;

/* DSPMEM_BLKHDR is the header of MEMORY BLOCK */
typedef struct DSPMEM_blk_hdr_stru
{
    //- 8B, Link list
    DLIST              list;

    //- 1B, memory block sequence
    U8                     blk_seQ;

    //- 1B, task
    U8                     task;

    //- 2B, memory block size
    U16                     blk_size;

    //- 4B, pointer which get this MEMORY BLOCK
    VOID*  using_ptr;
} DSPMEM_BLKHDR, * DSPMEM_BLKHDR_PTR;

/* DSPMEM_BLK is OS MEMORY BLOCK */
typedef struct DSPMEM_blk_stru
{
    //- 16B, Header of memory block
    DSPMEM_BLKHDR        header;
    //- ?B, Free data space with unknown size
    U8                  data_space[1];

} DSPMEM_BLK, * DSPMEM_BLK_PTR;

/* DSPMEM_BLKTAL is the tail of MEMORY BLOCK */
typedef struct DSPMEM_blk_tail_stru
{
    //- 4B, Top of memory block
    DSPMEM_BLK_PTR tob_ptr;
} DSPMEM_BLKTAL, * DSPMEM_BLKTAL_PTR;

typedef struct stru_dsp_mem_entries
{
    SIZE DSP_DSP_MEMSize;
    SIZE DSP_DPRT_MEMSize;
    SIZE DSP_DAVT_MEMSize;
    SIZE DSP_DHPT_MEMSize;
} DSP_MEMORY_STRU;

typedef enum
{
    DSP_MEMORY_CHECK_FRONT = 0,
    DSP_MEMORY_CHECK_BEHIND,
}DSP_MEMORY_CHECK_TIMING, *DSP_MEMORY_CHECK_TIMING_PTR;

////////////////////////////////////////////////////////////////////////////////
// External Variables //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////////////////////////
// Macro ///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
enum dsp_memory_allocation_e
{
    DPRT_MEM,
    DAVT_MEM,
    DHPT_MEM,
};


////////////////////////////////////////////////////////////////////////////////
// DSPMEM FUNCTION DECLARATIONS /////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
extern VOID             DSPMEM_Init         (TaskHandle_t   DSPTask);
extern U8               DSPMEM_Flush        (TaskHandle_t   DSPTask);
extern U8               DSPMEM_Free         (TaskHandle_t  DSPTask, VOID* usingPtr);
extern VOID*            DSPMEM_tmalloc      (TaskHandle_t   DSPTask, SIZE Size, VOID* usingPtr);
extern U8               DSPMEM_Check        (TaskHandle_t   DSPTask);
extern U16              DSPMEM_Remain       (TaskHandle_t   DSPTask);

#if AIR_PROMPT_SOUND_MEMORY_DEDICATE_ENABLE
extern VOID*            DSPMEM_tpremalloc   (TaskHandle_t   DSPTask, SIZE Size, VOID* usingPtr);
#endif

#endif /* _DSP_MEMORY_H_ */

