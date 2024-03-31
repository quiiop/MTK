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

 
#ifndef _DSP_STREAMCONNECT_H_
#define _DSP_STREAMCONNECT_H_

#include "audio_types.h"
#include "source_inter.h"
#include "sink_inter.h"
#include "dsp_feature_interface.h"

/******************************************************************************
 * Constants
 ******************************************************************************/
#define DSP_STREAM_BRANCH_FUNC_MEMSIZE  (sizeof(DSP_STREAM_BRANCH_FUNC_t))
#define DSP_STREAM_JOINT_FUNC_MEMSIZE   (sizeof(DSP_STREAM_JOINT_FUNC_t))



/******************************************************************************
 * DSP Command Structure
 ******************************************************************************/
typedef struct DSP_STREAM_BRANCH_FUNC_s
{
    SOURCE_TYPE     type;
} DSP_STREAM_BRANCH_FUNC_t, *DSP_STREAM_BRANCH_FUNC_PTR_t;

typedef struct DSP_STREAM_JOINT_FUNC_s
{
    S32             buftemp[2048];
    SINK_TYPE       type;
} DSP_STREAM_JOINT_FUNC_t, *DSP_STREAM_JOINT_FUNC_PTR_t;



/******************************************************************************
 * External Global Variables
 ******************************************************************************/


/******************************************************************************
 * External Functions
 ******************************************************************************/
extern BOOL DSP_StreamJointInit(VOID* para);
extern BOOL DSP_StreamJoint(VOID* para);
extern BOOL DSP_StreamBranchInit(VOID* para);
extern BOOL DSP_StreamBranch(VOID* para);



#endif /* _DSP_STREAMCONNECT_H_ */

