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


#ifndef _VOICE_PLC_INTERFACE_H_
#define _VOICE_PLC_INTERFACE_H_

//#include "dsp_feature_interface.h"
#include "dsp_utilities.h"
#include "plcpitch_sim.h"

/**
 *
 *  Definition
 *
 */
#define PLC_USE_STATIC_INSTANCE			(0)
#define PLC_USE_FAKE_PARA				(1)
#define eSCO_MOST_PROCSS_PACKET         (2)

#define MAX_PLC_PKT_SIZE                (360)
#define MAX_PLC_OFFSET_SIZE             (60)

#define DSP_VOICE_PLC_MEMSIZE  (sizeof(VOICE_PLC_SCRATCH_t))


typedef struct VOCIE_PLC_PARA_s
{
	U16 PktSize;
	U16 PktType;
	U16 CodecType;

	U16 ModemDataType;
	U16 SnrReportFrameSize;
	U16 ActualFrameSize;
	U16 ScoType;

	DSP_ALIGN8 S16 PLC_IN_FRAME[MAX_PLC_PKT_SIZE]; // Applied when not using as callback
	DSP_ALIGN8 S16 PLC_BUF[MAX_PLC_PKT_SIZE+MAX_PLC_OFFSET_SIZE];
	DSP_ALIGN8 PLC_state PLC_INSTANCE;
} VOICE_PLC_SCRATCH_t, *VOICE_PLC_SCRATCH_PTR_t;

typedef struct VOICE_RX_INBAND_INFO_s
{
	U32 Offset		:16;
	U32 RxEd 		:1;
	U32 IsMuted 	:1;
	U32 HecErr		:1;
	U32 HecForceOk	:1;
	U32 CrcErr		:1;
	U32 SnrErr		:1;
	U32 _Rsvd_		:10;
} VOICE_RX_INBAND_INFO_t, *VOICE_RX_INBAND_INFO_PTR_t;


typedef struct VOICE_RX_PICO_CLK_s
{
	U32 Value		:28;
	U32 _Rsvd_		:4;
} VOICE_RX_PICO_CLK_t;


typedef struct VOICE_RX_PKT_STRU_s
{
    VOICE_RX_INBAND_INFO_t InbandInfo[eSCO_MOST_PROCSS_PACKET];
    VOICE_RX_PICO_CLK_t PicoClk[eSCO_MOST_PROCSS_PACKET];
} PACKED VOICE_RX_PKT_STRU_t, *VOICE_RX_PKT_STRU_PTR_t;

typedef struct VOICE_RX_SINGLE_PKT_STRU_s
{
    VOICE_RX_INBAND_INFO_t InbandInfo;
    VOICE_RX_PICO_CLK_t PicoClk;
} PACKED VOICE_RX_SINGLE_PKT_STRU_t, *VOICE_RX_SINGLE_PKT_STRU_PTR_t;


bool stream_function_plc_initialize (void *para);
bool stream_function_plc_process (void *para);
VOID Voice_PLC_UpdateInbandInfo(VOICE_RX_INBAND_INFO_t* Ptr, U32 Lengh, U8 PktIdx);
VOID Voice_PLC_UpdatePacketInfo(U16 CodecType, U16 PktSize, U16 PktType);
VOID Voice_PLC_PushDataToProcess(U8* Ptr, U32 Lengh);
VOID Voice_PLC_PopDataFromProcess(U8* Ptr, U32 Lengh);
extern VOID Voice_PLC_CheckAndFillZeroResponse(S16* DataPtr,U8 codec_type);
extern VOID Voice_PLC_CleanInfo(VOICE_RX_SINGLE_PKT_STRU_PTR_t info);
extern BOOL Voice_PLC_CheckInfoValid(VOICE_RX_SINGLE_PKT_STRU_PTR_t InfoPtr);

#endif /* _VOICE_PLC_INTERFACE_H_ */
