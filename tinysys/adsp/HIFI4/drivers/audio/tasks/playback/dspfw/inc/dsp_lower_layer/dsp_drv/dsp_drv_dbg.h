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

 
#ifndef _DSP_DRV_DBG_H_
#define _DSP_DRV_DBG_H_

#include "audio_types.h"
#include "dsp_drv_dfe.h"


/******************************************************************************
 * Constant define
 ******************************************************************************/


/******************************************************************************
 * Constant define
 ******************************************************************************/
    
#define DBG_BUF_SIZE (7)
#define DUMP_HALF_BUFSIZE (9)





/******************************************************************************
 * Enumerations
 ******************************************************************************/


typedef struct
{
    BOOL DbgOutEnable;
    U32 Mask[2];

    U32 DUMP_ID;
    U32 DUMP_IDRemainSize;
    U32 BufWo;
    U32 BufRo;
    U32 BufCnt;
}   DBG_CTRL_STRU;

typedef union S32_OUTDATA_TYPE_union
{
    struct S32_OUTDATA_TYPE_stru
    {
        U16 _rsvd_      :8;
        U16 DebugID     :8;
        S16 OutData;
    } Field;

    S32 Reg;

} S32_OUTDATA_t;

typedef enum 
{
    Dump_DA_Out = 0,
    Dump_I2S_MS_Out,
    Dump_I2S_SL_Out,
    Dump_SPDIF_Out,
    Dump_USB__Out
}Dump_output_sel_enum_s;



/******************************************************************************
 * External Global Variables
 ******************************************************************************/


/******************************************************************************
 * External Function Prototypes
 ******************************************************************************/
   extern VOID DUMP_DBG_SFR_INIT( Dump_output_sel_enum_s DUMP_OUTPUT_SEL); 
   extern VOID DUMP_DBG_BUFFER_INIT (VOID);
   extern VOID Debug_DataIn (S16* InBuf, U16 DataLength, U16 DebugID);
   extern VOID Debug_DataOut (VOID);
   extern VOID DSP_oDFE_DBGIsrHandler(VOID);


#endif /* _DSP_DRV_DBG_H_ */

