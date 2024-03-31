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


//-
#include "dsp_sdk.h"
#include "dsp_update_para.h"

#ifndef UNUSED
#define UNUSED(p) ((void)(p))
#endif


////////////////////////////////////////////////////////////////////////////////
// Constant Definitions ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////
// Function Prototypes /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
BOOL    DSP_Update_DRCGain_Entry       (VOID* FuncMemPtr, VOID* dataBeginPtr, U32 dataLength);
BOOL    DSP_Update_PEQFilter_Entry     (VOID* FuncMemPtr, VOID* dataBeginPtr, U32 dataLength);
BOOL    DSP_Update_JointCh_Entry       (VOID* FuncMemPtr, VOID* dataBeginPtr, U32 dataLength);



////////////////////////////////////////////////////////////////////////////////
// Global Variables ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
UPDATE_PARA_ENTRY DSP_UpdateParaEntryTable[DSP_UPDATE_MAX_NUM] =
{
    DSP_Update_DRCGain_Entry,       /*DSP_UPDATE_DRC_GAIN*/
    DSP_Update_PEQFilter_Entry,     /*DSP_UPDATE_PEQ_FILTER*/
    DSP_Update_JointCh_Entry,       /*DSP_UPDATE_JOINT_CHANNEL*/
};

stream_feature_type_t DSP_UpdateCheckFeatureTypeTable[DSP_UPDATE_MAX_NUM] =
{
    FUNC_END,                       /*DSP_UPDATE_DRC_GAIN*/
    FUNC_END,                       /*DSP_UPDATE_PEQ_FILTER*/
    FUNC_JOINT,                     /*DSP_UPDATE_JOINT_CHANNEL*/
};

////////////////////////////////////////////////////////////////////////////////
// DSPMEM FUNCTION DECLARATIONS /////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/**
 * DSP_Update_DRCGain_Entry
 *
 * Update DRC digital gain value
 *
 * @FuncMemPtr   : Function memory pointer
 * @dataBeginPtr : data pointer
 * @dataLength   : data length
 *
 * @return FALSE if the streaming should not be initialized, TRUE otherwise.
 */
BOOL DSP_Update_DRCGain_Entry(VOID* FuncMemPtr, VOID* dataBeginPtr, U32 dataLength)
{
    UNUSED(FuncMemPtr);
    UNUSED(dataBeginPtr);
    UNUSED(dataLength);
    return FALSE;
}


/**
 * DSP_Update_PEQFilter_Entry
 *
 * Update PEQ Filter coefficient
 *
 * @FuncMemPtr   : Function memory pointer
 * @dataBeginPtr : data pointer
 * @dataLength   : data length
 *
 * @return FALSE if the streaming should not be initialized, TRUE otherwise.
 */
BOOL DSP_Update_PEQFilter_Entry(VOID* FuncMemPtr, VOID* dataBeginPtr, U32 dataLength)
{
    UNUSED(FuncMemPtr);
    UNUSED(dataBeginPtr);
    UNUSED(dataLength);
    return FALSE;
}


/**
 * DSP_Update_JointCh_Entry
 *
 * Joint Channel select
 *
 * @FuncMemPtr   : Function memory pointer
 * @dataBeginPtr : data pointer
 * @dataLength   : data length
 *
 * @return FALSE if the streaming should not be initialized, TRUE otherwise.
 */
BOOL DSP_Update_JointCh_Entry(VOID* FuncMemPtr, VOID* dataBeginPtr, U32 dataLength)
{
    UNUSED(FuncMemPtr);
    UNUSED(dataBeginPtr);
    UNUSED(dataLength);
    return FALSE;
}

