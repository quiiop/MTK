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


/*!
 *@file   stream_audio.c
 *@brief  Defines the audio stream
 *
 */

//-
#include "audio_types.h"
#include "stream_audio_setting.h"
#include "dsp_sdk.h"
#include "dsp_audio_ctrl.h"
#include "dsp_drv_afe.h"
#include "sink_inter.h"
#include "source_inter.h"
#include "audio_common.h"
#include <string.h>

Stream_audio_Config_Ptr Audio_setting;

VOID Audio_Default_setting_init(VOID)
{
#if 0
    AUDIO_PARAMETER *pSink;
    pSink = &Sink_blks[SINK_TYPE_AUDIO]->param.audio;
#endif

    DSP_MW_LOG_I("Audio_Default_setting_init\r\n", 0);

    if (Audio_setting != NULL)
    {return;}

    Audio_setting = (Stream_audio_Config_t *)pvPortMalloc(sizeof(Stream_audio_Config_t));

    memset(Audio_setting,0,sizeof(Stream_audio_Config_t));

#if 1
    Audio_setting->Audio_source.Frame_Size             = AUDIO_SOURCE_DEFAULT_FRAME_SIZE;
#else
    Audio_setting->Audio_source.Frame_Size             = AUDIO_SBC_FRAME_SAMPLES * pSource->format_bytes
                                                            * pSource->channel_num;
#endif
    Audio_setting->Audio_source.Buffer_Frame_Num       = AUDIO_SOURCE_DEFAULT_FRAME_NUM;
    Audio_setting->Rate.Source_Input_Sampling_Rate     = FS_RATE_16K;   //FS_RATE_96K;
    Audio_setting->Rate.Source_DownSampling_Ratio      = DOWNSAMPLE_BY1;
    Audio_setting->Audio_source.FetchFormatPerSample   = AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA;   // for LE

#if 0
    Audio_setting->Audio_sink.Frame_Size               = AUDIO_SBC_FRAME_SAMPLES * pSink->format_bytes;
//                                                            * pSink->channel_num;
#else
    Audio_setting->Audio_sink.Frame_Size               = AUDIO_SINK_DEFAULT_FRAME_SIZE;
#endif

    Audio_setting->Audio_sink.Buffer_Frame_Num         = AUDIO_SINK_DEFAULT_FRAME_NUM;
    Audio_setting->Audio_sink.Target_Q_Frame_Num       = 0;
    Audio_setting->Audio_sink.alc_enable               = TRUE;

    if (gAudioCtrl.Afe.OperationMode == AU_AFE_OP_ESCO_VOICE_MODE) {
        Audio_setting->Rate.Sink_Output_Sampling_Rate      = FS_RATE_16K;
    }
    else {
        Audio_setting->Rate.Sink_Output_Sampling_Rate      = FS_RATE_48K;
    }

    Audio_setting->Rate.Sink_UpSampling_Ratio          = UPSAMPLE_BY1;
    Audio_setting->Audio_sink.Mute_Flag                = FALSE;
    Audio_setting->Audio_sink.Pause_Flag               = FALSE;
    Audio_setting->resolution.AudioInRes               = RESOLUTION_16BIT;
    Audio_setting->resolution.AudioOutRes              = RESOLUTION_16BIT;
    Audio_setting->resolution.SRCInRes                 = RESOLUTION_16BIT;
    #if (FEA_SUPP_DSP_AUDIO_LOOPBACK_TEST)
    Audio_setting->Audio_sink.SRC_Out_Enable           = FALSE;
    #else
    Audio_setting->Audio_sink.SRC_Out_Enable           = TRUE;
    #endif

    Audio_setting->Rate.SRC_Sampling_Rate              = AudioSourceSamplingRate_Get();
    Audio_setting->Audio_sink.Zero_Padding_Cnt         = AUDIO_SINK_ZEROPADDING_FRAME_NUM;
    Audio_setting->Audio_VP.Frame_Size                 = AUDIO_SINK_DEFAULT_FRAME_SIZE;
    Audio_setting->Audio_VP.Buffer_Frame_Num           = AUDIO_SINK_DEFAULT_FRAME_NUM;
    Audio_setting->Audio_VP.Target_Q_Frame_Num         = AUDIO_SINK_DEFAULT_FRAME_NUM/2;
    Audio_setting->Rate.VP_UpSampling_Ratio            = UPSAMPLE_BY6;
    Audio_setting->Audio_VP.Mute_Flag                  = FALSE;
    Audio_setting->Audio_sink.FetchFormatPerSample      = AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA;   // for LE

}


U16 AudioSinkFrameSize_Get(VOID)
{
    return Audio_setting->Audio_sink.Frame_Size;
}


U8 AudioSinkFrameNum_Get(VOID)
{
    return Audio_setting->Audio_sink.Buffer_Frame_Num;
}


stream_samplerate_t AudioSourceSamplingRate_Get(VOID)
{
    return (Audio_setting->Rate.Source_Input_Sampling_Rate /
             DSP_UpDownRate2Value(Audio_setting->Rate.Source_DownSampling_Ratio));
}


U16 AudioSourceFrameSize_Get(VOID)
{
    return Audio_setting->Audio_source.Frame_Size;
}


U8 AudioSourceFrameNum_Get(VOID)
{
    return Audio_setting->Audio_source.Buffer_Frame_Num;
}


U8 AudioSRCSamplingRate_Get(VOID)
{
    return Audio_setting->Rate.SRC_Sampling_Rate;
}


U8 AudioSRCSamplingRate_Set(stream_samplerate_t rate)
{
    Audio_setting->Rate.SRC_Sampling_Rate = rate;
    return Audio_setting->Rate.SRC_Sampling_Rate;
}


stream_samplerate_t AudioSinkSamplingRate_Get(VOID)
{
    return (Audio_setting->Rate.Sink_Output_Sampling_Rate /
             DSP_UpDownRate2Value(Audio_setting->Rate.Sink_UpSampling_Ratio));
}

stream_samplerate_t AudioVpSinkSamplingRate_Get(VOID)
{
    return (Audio_setting->Rate.Sink_Output_Sampling_Rate /
             DSP_UpDownRate2Value(Audio_setting->Rate.VP_UpSampling_Ratio));
}


