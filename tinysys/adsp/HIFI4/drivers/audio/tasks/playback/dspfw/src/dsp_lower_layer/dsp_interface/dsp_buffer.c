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

//#include "config.h"
////#include "os.h"
////#include "os_memory.h"
#include "dsp_task.h"
////#include "rc.h"
#include "dsp_buffer.h"
#include "dsp_temp.h"
//#include "dsp_memory.h"
#include "dsp_utilities.h"
#include <string.h>


/******************************************************************************
 * Function Declaration
 ******************************************************************************/
VOID DSP_C2C_BufferCopy(VOID* DestBuf,
                        VOID* SrcBuf,
                        U16 CopySize,
                        VOID* DestCBufStart,
                        U16 DestCBufSize,
                        VOID* SrcCBufStart,
                        U16 SrcCBufSize);

VOID DSP_D2C_BufferCopy(VOID* DestBuf,
                        VOID* SrcBuf,
                        U16 CopySize,
                        VOID* CBufStart,
                        U16 DestCBufSize);

VOID DSP_C2D_BufferCopy(VOID* DestBuf,
                        VOID* SrcBuf,
                        U32 CopySize,
                        VOID* CBufStart,
                        U32 SrcCBufSize);


/******************************************************************************
 * Variables
 ******************************************************************************/


/**
 * DSP_C2D_BufferCopy
 */
ATTR_TEXT_IN_IRAM VOID DSP_C2D_BufferCopy(VOID* DestBuf,
                        VOID* SrcBuf,
                        U32 CopySize,
                        VOID* SrcCBufStart,
                        U32 SrcCBufSize)
{
    U8* SrcCBufEnd     	=  (U8*)((U8*)SrcCBufStart +  SrcCBufSize);
    U32 UnwrapSize      = (U8*)SrcCBufEnd - (U8*)SrcBuf; /* Remove + 1 to sync more common usage */
    S32 WrapSize        = CopySize - UnwrapSize;

    configASSERT((SrcCBufEnd >= (U8*)SrcBuf) && (SrcBuf >= SrcCBufStart));

    if (WrapSize > 0)
    {
        memcpy(DestBuf, SrcBuf, UnwrapSize);

        while ((U32)WrapSize > SrcCBufSize)
        {
            memcpy((U8*)DestBuf + UnwrapSize, SrcCBufStart, SrcCBufSize);
            WrapSize -= SrcCBufSize;
        }

        memcpy((U8*)DestBuf + UnwrapSize, SrcCBufStart, WrapSize);
    }
    else
    {
        memcpy(DestBuf, SrcBuf, CopySize);
    }
}


/**
 * DSP_D2C_BufferCopy
 */
ATTR_TEXT_IN_IRAM VOID DSP_D2C_BufferCopy(VOID* DestBuf,
                        VOID* SrcBuf,
                        U16 CopySize,
                        VOID* DestCBufStart,
                        U16 DestCBufSize)
{
	U8* DestCBufEnd		= (U8*)((U8*)DestCBufStart + DestCBufSize);
    U16 UnwrapSize      = (U8*)DestCBufEnd - (U8*)DestBuf; /* Remove + 1 to sync more common usage */
    S16 WrapSize        = CopySize - UnwrapSize;

    configASSERT((DestCBufEnd >= (U8*)DestBuf) && (DestBuf >= DestCBufStart));

    if (WrapSize > 0)
    {
        memcpy(DestBuf, SrcBuf, UnwrapSize);

        while (WrapSize > DestCBufSize)
        {
            memcpy(DestCBufStart, (U8*)SrcBuf + UnwrapSize, DestCBufSize);
            WrapSize -= DestCBufSize;
        }

        memcpy(DestCBufStart, (U8*)SrcBuf + UnwrapSize, WrapSize);
    }
    else
    {
        memcpy(DestBuf, SrcBuf, CopySize);
    }
}




/**
 * DSP_C2C_BufferCopy
 */
VOID DSP_C2C_BufferCopy(VOID* DestBuf,
                        VOID* SrcBuf,
                        U16 CopySize,
                        VOID* DestCBufStart,
                        U16 DestCBufSize,
                        VOID* SrcCBufStart,
                        U16 SrcCBufSize)
{
	U8* DestCBufEnd     = (U8*)((U8*)DestCBufStart + DestCBufSize);
    U16 DestUnwrapSize  = (U8*)DestCBufEnd - (U8*)DestBuf; /* Remove + 1 to sync more common usage */
    S16 DestWrapSize    = CopySize - DestUnwrapSize;

	U8* SrcCBufEnd		= (U8*)((U8*)SrcCBufStart + SrcCBufSize);
    U16 SrcUnwrapSize   = (U8*)SrcCBufEnd - (U8*)SrcBuf; /* Remove + 1 to sync more common usage */
    S16 SrcWrapSize     = CopySize - SrcUnwrapSize;

    configASSERT((DestCBufEnd >= (U8*)DestBuf) && (DestBuf >= DestCBufStart));
    configASSERT((SrcCBufEnd >= (U8*)SrcBuf) && (SrcBuf >= SrcCBufStart));

    if ((DestWrapSize > 0) && (SrcWrapSize > 0))
    {
        if (DestUnwrapSize > SrcUnwrapSize)
        {
            /* Src Buf wrap first */
            memcpy(DestBuf, SrcBuf, SrcUnwrapSize);

            while (SrcWrapSize > SrcCBufSize)
            {
                DSP_D2C_BufferCopy((U8*)DestBuf + SrcUnwrapSize, SrcCBufStart, SrcCBufSize, DestCBufStart, DestCBufSize);
                SrcWrapSize -= SrcCBufSize;
            }

            DSP_D2C_BufferCopy((U8*)DestBuf + SrcUnwrapSize, SrcCBufStart, CopySize - SrcUnwrapSize, DestCBufStart, DestCBufSize);
        }
        else
        {
            /* Dest Buf wrap first */
            memcpy(DestBuf, SrcBuf, DestUnwrapSize);

            while (DestWrapSize > DestCBufSize)
            {
                DSP_C2D_BufferCopy(DestCBufStart, (U8*)SrcBuf + DestUnwrapSize, DestCBufSize, SrcCBufStart, SrcCBufSize);
                DestWrapSize -= DestCBufSize;
            }

            DSP_C2D_BufferCopy(DestCBufStart, (U8*)SrcBuf + DestUnwrapSize, CopySize - DestUnwrapSize, SrcCBufStart, SrcCBufSize);
        }
    }
    else if (DestWrapSize > 0) /* Actual D2C */
    {
        DSP_D2C_BufferCopy(DestBuf, SrcBuf, CopySize, DestCBufStart, DestCBufSize);
    }
    else if (SrcWrapSize > 0) /* Actual C2D */
    {
        DSP_C2D_BufferCopy(DestBuf, SrcBuf, CopySize, SrcCBufStart, SrcCBufSize);
    }
    else
    {
        memcpy(DestBuf, SrcBuf, CopySize);
    }
}

/**
 * DSP_D2I_BufferCopy
 *
 * Direct buffer copy to interleaved buffer
 */
ATTR_TEXT_IN_IRAM VOID DSP_D2I_BufferCopy(U8* DestBuf,
                        U8* SrcBuf1,
                        U8* SrcBuf2,
                        U8* DestIBufStart,
                        U16 DestIBufSize,
                        U16 CopySize,
                        U16 FormatBytes)
{
	U8* DestIBufEnd		= (U8*)((U8*)DestIBufStart + DestIBufSize);
    uint32_t i, j, sample;
    uint32_t wpt, rpt, src_offset, dst_offset, toggle = 0;
    src_offset = 0;
    dst_offset = DestBuf - DestIBufStart;
    toggle = 0;
    configASSERT((DestIBufEnd >= (U8*)DestBuf) && (DestBuf >= DestIBufStart));

    //format_bytes
    sample = (CopySize<<1)/FormatBytes;

    for (i=0 ; i<sample ; i++) {
        for (j=0 ; j<FormatBytes ; j++ ) {
            wpt = (dst_offset + j) % DestIBufSize;
            rpt = src_offset + j;
            if ((toggle == 1) && (SrcBuf2 != NULL)) {
                *(DestIBufStart+wpt) = *(SrcBuf2+rpt);
            } else {
                *(DestIBufStart+wpt) = *(SrcBuf1+rpt);
            }
        }

        dst_offset = (dst_offset + FormatBytes)%DestIBufSize;
        toggle ^= 1;
        if (toggle==0) {
            src_offset = src_offset + FormatBytes;
        }

    }

}


/**
 * DSP_I2D_BufferCopy
 *
 * Interleaved buffer copy to direct buffer
 */
ATTR_TEXT_IN_IRAM VOID DSP_I2D_BufferCopy(U8* DestBuf1,
                        U8* DestBuf2,
                        U8* SrcBuf,
                        U8* SrcIBufStart,
                        U16 SrcIBufSize,
                        U16 CopySize,
                        U16 FormatBytes)
{
	U8* SrcIBufEnd		= (U8*)((U8*)SrcIBufStart + SrcIBufSize);
    uint32_t i, j, sample;
    uint32_t rpt, wpt, src_offset, dst_offset, toggle = 0;
    dst_offset = 0;
    src_offset = SrcBuf - SrcIBufStart;
    toggle = 0;
    configASSERT((SrcIBufEnd >= (U8*)SrcBuf) && (SrcBuf >= SrcIBufStart));

    //format_bytes
    sample = (CopySize<<1)/FormatBytes;

    for (i=0 ; i<sample ; i++) {
        for (j=0 ; j<FormatBytes ; j++ ) {
            rpt = (src_offset + j) % SrcIBufSize;
            wpt = dst_offset+j;
            if ((toggle == 1) && (DestBuf2 != NULL)) {
                *(DestBuf2+wpt) = *(SrcIBufStart+rpt);
            } else if (toggle == 0){
                *(DestBuf1+wpt) = *(SrcIBufStart+rpt);
            }
        }

        src_offset = (src_offset + FormatBytes)%SrcIBufSize;
        toggle ^= 1;
        if (toggle==0) {
            dst_offset = dst_offset + FormatBytes;
        }
    }
}

