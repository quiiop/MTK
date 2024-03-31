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

 
#ifndef _DSP_BUFFER_H_
#define _DSP_BUFFER_H_

#include "audio_types.h"


/*
 *
 * DSP Command Structure
 *
 */
typedef struct DSP_C_BUF_s
{
    VOID*             BufStart;
    VOID*             BufEnd;
} DSP_C_BUF_t, *DSP_C_BUF_PTR_t;

typedef struct DSP_BUFFER_HEADER_s
{
    DSP_C_BUF_t             Circular;
    U16                     DmaOffset;
    U16                     OutputThreshold;
    U16                     WriteOffset;
    U16                     ReadOffset;
    U16                     BufferCount;
    U16                     BufferSize;
} DSP_BUFFER_HEADER_t, *DSP_BUFFER_HEADER_PTR_t;

typedef struct DSP_BUFFER_s
{
    DSP_BUFFER_HEADER_t     Header;
    U8                      Buffer[];
} DSP_BUFFER_t, *DSP_BUFFER_PTR_t;

typedef enum
{
    DFE_ID_SRC_A_IN_L,
    DFE_ID_SRC_A_IN_R,
    DFE_ID_SRC_A_OUT_L,
    DFE_ID_SRC_A_OUT_R,
    DFE_ID_SRC_B_IN_L,
    DFE_ID_SRC_B_IN_R,
    DFE_ID_SRC_B_OUT_L,
    DFE_ID_SRC_B_OUT_R,
    DFE_ID_ODFE_AU_L,
    DFE_ID_ODFE_AU_R,
    DFE_ID_ODFE_VP,
    DFE_ID_ODFE_DBG_L,
    DFE_ID_ODFE_DBG_R,
    DFE_ID_IDFE_EC,
    DFE_ID_IDFE_DBG,
    DFE_ID_IDFE_AU_0,
    DFE_ID_IDFE_AU_1,
    DFE_ID_IDFE_AU_2,
    DFE_ID_IDFE_AU_3,
    DFE_ID_TOTAL_NO,
} DSP_BUFFER_ID_t;

/******************************************************************************
 * External Global Variables
 ******************************************************************************/


/******************************************************************************
 * External Functions
 ******************************************************************************/
extern VOID DSP_C2D_BufferCopy(VOID* DestBuf,
                               VOID* SrcBuf,
                               U32 CopySize,
                               VOID* SrcCBufStart,
                               U32 SrcCBufSize);

extern VOID DSP_D2C_BufferCopy(VOID* DestBuf,
                               VOID* SrcBuf,
                               U16 CopySize,
                               VOID* DestCBufStart,
                               U16 DestCBufSize);

extern VOID DSP_C2C_BufferCopy(VOID* DestBuf,
	 	                       VOID* SrcBuf,
	 	                       U16 CopySize,
	 	                       VOID* DestCBufStart,
	 	                       U16 DestCBufSize,
	 	                       VOID* SrcCBufStart,
	 	                       U16 SrcCBufSize);

extern VOID DSP_D2I_BufferCopy(U8* DestBuf,
                                U8* SrcBuf1,
                                U8* SrcBuf2,
                                U8* DestIBufStart,
                                U16 DestIBufSize,
                                U16 CopySize,
                                U16 FormatBytes);

extern VOID DSP_I2D_BufferCopy(U8* DestBuf1,
                               U8* DestBuf2,
                               U8* SrcBuf,
                               U8* SrcIBufStart,
                               U16 SrcIBufSize,
                               U16 CopySize,
                               U16 FormatBytes);

extern VOID DSP_BUF_SetOutputThreshold(DSP_BUFFER_ID_t DfeID, U16 Threshold);

extern VOID DSP_AudioBufferInit(U8 TaskID, DSP_BUFFER_ID_t DfeID, U16 BufSize);
extern VOID DSP_AudioBufferDeInit(DSP_BUFFER_ID_t DfeID);
extern VOID DSP_AudioDmaInit(U8 TaskID, DSP_BUFFER_ID_t DfeID, U16 BufSize, U16 ThresholdSize);

extern VOID DSP_IdfePushSamplesToAuInBuf(DSP_BUFFER_ID_t DfeID);
extern BOOL DSP_GetSampleFromAuInBuf(DSP_BUFFER_ID_t DfeID, U16 Samples, VOID* DestBuf);
extern VOID DSP_OdfeGetSamplesFromAuOutBuf(DSP_BUFFER_ID_t DfeID);
extern BOOL DSP_PushSampleToAuOutBuf(DSP_BUFFER_ID_t DfeID, U16 Samples, VOID* SrcBuf);

#endif /* _DSP_BUFFER_H_ */

