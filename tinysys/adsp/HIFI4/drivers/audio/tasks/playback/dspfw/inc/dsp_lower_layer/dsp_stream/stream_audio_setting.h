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

#ifndef __STREAM_AUDIO_SETTING_H__
#define __STREAM_AUDIO_SETTING_H__

/*!
 *@file   stream_audio_setting.h
 *@brief  defines the setting of audio stream
*/

//#include "config.h"
#include "audio_types.h"
#include "dsp_drv_dfe.h"
#ifdef HEL_AUDIO_READY
#include "hal_audio_afe_define.h"
#else
typedef enum {
    AFE_WLEN_16_BIT = 0,
    AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA = 1,
    AFE_WLEN_32_BIT_ALIGN_24BIT_DATA_8BIT_0 = 3,
} afe_fetch_format_per_sampel_t;
#endif

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

#define AUDIO_SOURCE_DEFAULT_FRAME_SIZE 480
#define AUDIO_SOURCE_DEFAULT_FRAME_NUM 6
#define AUDIO_SINK_DEFAULT_FRAME_SIZE 480
#define AUDIO_SINK_DEFAULT_FRAME_NUM 8// 4 //8
#define AUDIO_SINK_ZEROPADDING_FRAME_NUM 0
#define AUDIO_SBC_FRAME_SAMPLES         120
#define AUDIO_AAC_FRAME_SAMPLES         1024
#define AUDIO_SOURCE_PREFILL_FRAME_NUM  0

#define AUDIO_SOURCE_DEFAULT_ANALOG_VOICE_RATE    16000
#define AUDIO_SOURCE_DEFAULT_ANALOG_AUDIO_RATE    48000
#define AUDIO_SINK_DEFAULT_OUTPUT_RATE            48000

typedef struct Audio_Fade_Ctrl_u
{
    U16  Target_Level;
    U16  Current_Level;
    S16  Step;
    U16  Resolution;
} PACKED Audio_Fade_Ctrl_t, *Audio_Fade_Ctrl_Ptr;

typedef struct Audio_Source_Config_u
{
    U16  Frame_Size;
    U8   Buffer_Frame_Num;
    BOOL Mute_Flag;
    afe_fetch_format_per_sampel_t FetchFormatPerSample;
#ifdef MTK_AUDIO_LOOPBACK_TEST_ENABLE
    BOOL Pga_mux;
#endif
} Audio_Source_config_t;

typedef struct Audio_Sink_Config_u
{
    U16  Frame_Size;
    U8   Buffer_Frame_Num;
    U8   Target_Q_Frame_Num;
    BOOL Output_Enable;
    BOOL Mute_Flag;
    BOOL SRC_Out_Enable;
    BOOL Pause_Flag;
    BOOL alc_enable;
    U8   Zero_Padding_Cnt;
    U8   Software_Channel_Num;
    afe_fetch_format_per_sampel_t FetchFormatPerSample;
#ifdef ENABLE_HWSRC_CLKSKEW
    clkskew_mode_t clkskew_mode;
#endif
} Audio_Sink_config_t;

typedef struct Audio_VP_Config_u
{
    U16  Frame_Size;
    U8   Buffer_Frame_Num;
    U8   Target_Q_Frame_Num;
    BOOL Output_Enable;
    BOOL Mute_Flag;
} Audio_VP_config_t;


typedef struct Rate_Config_u
{
    U8 Source_Input_Sampling_Rate;
    U8 Source_DownSampling_Ratio;
    U8 Sink_UpSampling_Ratio;
    U8 Sink_Output_Sampling_Rate;
    U8 SRC_Sampling_Rate;
    U8 VP_UpSampling_Ratio;
} Rate_config_t;

typedef struct Audio_Resolution_u
{
    stream_resolution_t AudioInRes;
    stream_resolution_t AudioOutRes;
    stream_resolution_t SRCInRes;
} Audio_Resolution_t;

typedef struct Audio_Interface_GPIO_u
{
    U8 I2S_Master_Group;
    U8 I2S_Slave_Group;
} Audio_Interface_GPIO_t;


typedef struct Stream_audio_Config_s
{
    Audio_Source_config_t Audio_source;
    Audio_Sink_config_t Audio_sink;
    Audio_VP_config_t Audio_VP;
    Rate_config_t  Rate;
    Audio_Resolution_t resolution;
    Audio_Interface_GPIO_t Audio_interface;
}Stream_audio_Config_t, *Stream_audio_Config_Ptr;


extern Stream_audio_Config_Ptr Audio_setting;
extern VOID Audio_Default_setting_init(VOID);
extern U16 AudioSinkFrameSize_Get(VOID);
extern U8 AudioSinkFrameNum_Get(VOID);
extern stream_samplerate_t AudioSourceSamplingRate_Get(VOID);
extern U16 AudioSourceFrameSize_Get(VOID);
extern U8 AudioSourceFrameNum_Get(VOID);
extern U8 AudioSRCSamplingRate_Get(VOID);
extern U8 AudioSRCSamplingRate_Set(stream_samplerate_t rate);
extern stream_samplerate_t AudioSinkSamplingRate_Get(VOID);
extern stream_samplerate_t AudioVpSinkSamplingRate_Get(VOID);


#endif /* __STREAM_AUDIO_SETTING_H__ */

