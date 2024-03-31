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

#ifndef _STREAM_AUDIO_DRIVER_H_
#define _STREAM_AUDIO_DRIVER_H_

/*!
 *@file   stream_audio_driver.h
 *@brief  defines the setting of audio stream
 */

//#include "config.h"
#include "audio_types.h"
#include "source.h"
#include "sink.h"
#include "source_inter.h"
#include "sink_inter.h"
#include "transform_inter.h"
#include "dsp_drv_dfe.h"
#include "stream_config.h"
////////////////////////////////////////////////////////////////////////////////
// Constant Definitions ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//#define AUTO_RATE_DET

#define OFFSET_OVERFLOW_CHK(preOffset, nowOffset, comparedOffset) (\
                                ((preOffset==nowOffset) && (preOffset==comparedOffset))\
                                    ? TRUE\
                                    : (nowOffset>=preOffset)\
                                        ? (comparedOffset<=nowOffset && comparedOffset>preOffset)\
                                            ? TRUE\
                                            : FALSE\
                                        : (comparedOffset<=nowOffset || comparedOffset>preOffset)\
                                            ? TRUE\
                                            : FALSE)


////////////////////////////////////////////////////////////////////////////////
// Extern Function Declarations ////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
extern BOOL Stream_Audio_Handler(SOURCE source , SINK sink);
extern VOID Sink_Audio_Path_Init(SINK sink);
extern VOID Sink_Audio_Path_Init_AFE(SINK sink);

extern VOID Sink_Audio_Buffer_Ctrl(SINK sink, BOOL isEnabled);
extern VOID Sink_Audio_BufferInfo_Rst(SINK sink, U32 offset);
extern VOID Sink_Audio_UpdateReadOffset(SINK_TYPE type);
extern VOID Sink_Audio_SubPath_Init(SINK sink);
extern BOOL Sink_Audio_ZeroPadding();
extern BOOL Sink_Audio_ClosureSmooth(SINK sink);
extern VOID Sink_Audio_SRC_Ctrl(SINK sink, BOOL isEnabled, stream_samplerate_t value);
extern VOID Sink_Audio_Triger_SourceSRC(SOURCE source);
extern VOID Sink_Audio_SRC_A_CDM_SamplingRate_Reset(SINK sink, stream_samplerate_t rate);
extern VOID Sink_Audio_ADMAIsrHandler(VOID);
extern VOID Sink_Audio_SRCIsrHandler(VOID);
extern VOID Sink_Audio_VpRtIsrHandler(VOID);
extern VOID SinkBufferUpdateReadPtr(SINK sink, int32_t size);
extern VOID AudioCheckTransformHandle(TRANSFORM transform);
extern BOOL AudioAfeConfiguration(stream_config_type type, U32 value);

extern VOID SourceBufferUpdateWritePtr(SOURCE source, int32_t size);

extern VOID Source_Audio_Pattern_Init(SOURCE source);
extern VOID Source_Audio_Path_Init(SOURCE source);
extern VOID Source_Audio_SRC_Ctrl(SOURCE source, BOOL ctrl, stream_samplerate_t value);
extern VOID Source_Audio_Buffer_Ctrl(SOURCE source, BOOL ctrl);
extern VOID Source_Audio_BufferInfo_Rst(SOURCE source, U32 offset);
extern VOID Source_Audio_Triger_SinkSRC(SINK sink);
extern VOID Source_Audio_IsrHandler(VOID);

extern BOOL Sink_Audio_WriteBuffer (SINK sink, U8 *src_addr, U32 length);
extern BOOL Sink_Audio_FlushBuffer(SINK sink, U32 amount);
extern BOOL Sink_Audio_CloseProcedure(SINK sink);
extern BOOL Sink_Audio_Configuration(SINK sink, stream_config_type type, U32 value);
extern BOOL Source_Audio_ReadAudioBuffer(SOURCE source, U8* dst_addr, U32 length);
extern BOOL Source_Audio_CloseProcedure(SOURCE source);
extern BOOL Source_Audio_Configuration(SOURCE source, stream_config_type type, U32 value);
extern U32  Source_Audio_GetVoicePromptSize(SOURCE source);
extern BOOL Source_Audio_ReadVoicePromptBuffer(SOURCE source, U8 *dst_addr, U32 length);
extern VOID Source_Audio_DropVoicePrompt(SOURCE source, U32 amount);
extern U32  Source_Audio_GetRingtoneSize(SOURCE source);
extern BOOL Source_Audio_ReadRingtoneBuf(SOURCE source, U8 *dst_addr, U32 length);
extern VOID Source_Audio_DropRingtone(SOURCE source, U32 amount);

extern BOOL Sink_AudioQ_Configuration(SINK sink, stream_config_type type, U32 value);
extern BOOL Source_AudioQ_Configuration(SOURCE source, stream_config_type type, U32 value);
extern BOOL Sink_VirtualSink_Configuration(SINK sink, stream_config_type type, U32 value);
#ifdef HAL_AUDIO_ENABLE_PATH_MEM_DEVICE
extern hal_audio_interconn_selection_t stream_audio_convert_control_to_interconn(hal_audio_control_t audio_control, hal_audio_path_port_parameter_t port_parameter, uint32_t connection_sequence, bool is_input);
extern hal_audio_memory_selection_t hal_memory_convert_dl(hal_audio_memory_t memory);//for compatable ab1552
extern hal_audio_memory_selection_t hal_memory_convert_ul(hal_audio_memory_t memory);//for compatable ab1552

hal_audio_memory_selection_t stream_audio_convert_interconn_to_memory(hal_audio_interconn_selection_t interconn_selection);
#endif

extern void Sink_Audio_ExtAmpOff_Control_Init(void);
extern void Sink_Audio_ExtAmpOff_Control_Callback(BOOL SilenceFlag);

#endif /* _STREAM_AUDIO_DRIVER_H_ */

