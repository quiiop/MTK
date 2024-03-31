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

 
#ifndef _DPRT_VP_H_
#define _DPRT_VP_H_

#include "audio_types.h"
#include "dsp_callback.h"

#define VP_ODFE_BUFFER_SIZE     (256)
#define VP_GENERATE_SIZE        (256)
#define VP_STREAM_SIZE_MAX      (31)
#define VP_BUFFER_SIZE          (VP_GENERATE_SIZE<<2)


typedef struct VP_PARA_CTL_s
{
    U16 AmrMode;
    U16 AmrStreamSize;
    U16 RemainingLength;
    U16* PatternAddress;
} VP_PARA_CTL_t;

typedef struct VP_STATUS_s
{
    U16 output_enable;
    U16 count;
    U16 wo;
    U16 ro;

    U32 IntrSignal;
    U16 PaddingCount;
} VP_STATUS_t;

typedef struct VP_BUFFER_s
{
    S16 ODFE_BUF[VP_ODFE_BUFFER_SIZE<<1];
    S16 OUT_BUF[VP_BUFFER_SIZE];
    S16 STREAM_BUF[VP_STREAM_SIZE_MAX];
    S16 DECODE_BUF[VP_GENERATE_SIZE];
} VP_BUFFER_t;

typedef struct VP_CTL_s
{
    VP_BUFFER_t     BUF;
    VP_PARA_CTL_t   PARA;
    VP_STATUS_t     STATUS;
} VP_CTL_t;



/******************************************************************************
 * External Global Variables
 ******************************************************************************/


/******************************************************************************
 * External Function Prototypes
 ******************************************************************************/
extern BOOL             AMRInit                     (VOID* para);
extern BOOL             AMRCodec                    (VOID* para);



#endif /* _DPVP_VP_H_ */

