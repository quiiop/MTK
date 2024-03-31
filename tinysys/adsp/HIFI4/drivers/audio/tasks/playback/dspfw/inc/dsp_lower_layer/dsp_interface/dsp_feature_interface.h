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


#ifndef _DSP_FEATUREINTERFACE_H_
#define _DSP_FEATUREINTERFACE_H_

#include <string.h>
#include "audio_types.h"
#include "dsp_task.h"
#include "dsp_sdk.h"
#include "dsp_drv_dfe.h"
#ifdef AIR_VOICE_PLC_ENABLE
#include "voice_plc_interface.h"
#endif
#include "dsp_temp.h"


/******************************************************************************
 * CONSTANT DEFINITIONS
 ******************************************************************************/
#define CODEC_ALLOW_SEQUENCE          1

#ifdef MTK_TDM_ENABLE
#define CALLBACK_INPUT_PORT_MAX_NUM   8
#define CALLBACK_OUTPUT_PORT_MAX_NUM  8
#else
#define CALLBACK_INPUT_PORT_MAX_NUM   8
#define CALLBACK_OUTPUT_PORT_MAX_NUM  8
#endif
#define PIC_LOGPRINT                  0

#define DSP_SIZE_FOR_CLK_SKEW  (8) // 8-byte alignment for resolution converter

/******************************************************************************
 * DSP Command Structure
 ******************************************************************************/
typedef union {
    struct DSP_FEATURE_NUMBERING_s
    {
        U8 feature_number;
        U8 source_type;
        U8 sink_type;
        U8 process_sequence;
    } field;
    U32  reg;
} DSP_FEATURE_NUMBERING;

typedef struct DSP_FEATURE_RESOLUTION_u
{
    U8 source_in_res;   //
    U8 feature_res;     // codec out resolution;
    U8 sink_out_res;    //
    U8 process_res;     // streaming resolution
} DSP_FEATURE_RESOLUTION;

typedef struct DSP_AIROSTEREO_INFO_s {

    U16     frame_count;

	U16     remote_time_stamp;
    U16     remote_frame_count;
	U16     remote_sample_count;
    U16     remote_tuning_value;
    U16     remote_tuning_time;

}DSP_AIROSTEREO_INFO, *DSP_AIROSTEREO_INFO_PTR;

#ifdef AIR_VOICE_PLC_ENABLE
typedef union {
    VOICE_RX_PKT_STRU_t PlcInfo;
    DSP_AIROSTEREO_INFO AiroInfo;
    U32  Reg[5];
} DSP_PKT_INFO_STRU, *DSP_PKT_INFO_STRU_PTR;
#endif


#if 0
typedef struct DSP_Codec_Para_s {
    VOID*                   mem_ptr;
    TaskHandle_t *              DSPTask;

    U8                      in_channel_num;
    U16                     in_size;
    VOID*                   in_ptr[CALLBACK_INPUT_PORT_MAX_NUM];
    U8                      in_sampling_rate;

    U8                      out_channel_num;
    U16                     codec_out_size;
    VOID*                   out_ptr[CALLBACK_OUTPUT_PORT_MAX_NUM];
    U8                      codec_out_sampling_rate;

    U8                      with_encoder;
    U16                     encoder_out_size;

    U8                      with_src;
    U8                      src_out_sampling_rate;
    U16                     src_out_size;

    DSP_FEATURE_NUMBERING   number;
} DSP_CODEC_PARA, *DSP_CODEC_PARA_PTR;

typedef struct Dsp_Func_Para_s {
    VOID*                   mem_ptr;
    TaskHandle_t *              DSPTask;
    U8                      _reserved_1;
    U16                     _reserved_2;
    VOID*                   _reserved[CALLBACK_INPUT_PORT_MAX_NUM];
    U8                      _reserved_3;
    U8                      buf_channel_num;
    U16                     buf_size;
    VOID*                   buf_ptr[CALLBACK_OUTPUT_PORT_MAX_NUM];
    U8                      buf_sampling_rate;
    U8                      _reserved_4;
    U16                     _reserved_5;
    DSP_FEATURE_NUMBERING   number;
} DSP_FUNC_PARA, *DSP_FUNC_PARA_PTR;

typedef union {
    DSP_CODEC_PARA codec;
    DSP_FUNC_PARA  func;
} DSP_ENTRY_PARA, *DSP_ENTRY_PARA_PTR;
#else

typedef struct DSP_Entry_Para_s {
    VOID*                   mem_ptr;

    U16                     in_malloc_size;
    U16                     in_size;
    VOID*                   in_ptr[CALLBACK_INPUT_PORT_MAX_NUM];

    U16                     out_malloc_size;
    U16                     codec_out_size;
    VOID*                   out_ptr[CALLBACK_OUTPUT_PORT_MAX_NUM];

    U8                      in_channel_num;
    U8                      in_sampling_rate;
    U8                      out_channel_num;
    U8                      device_out_channel_num;
    U8                      software_handled_channel_num;
    U8                      codec_out_sampling_rate;

    TaskHandle_t            DSPTask;

    U8                      with_encoder;
    U16                     encoder_out_size;

    U8                      with_src;
    U8                      src_out_sampling_rate;
    U16                     src_out_size;

    U16                     pre_codec_out_size;
    U8                      pre_codec_out_sampling_rate;
    U8                      skip_process;
    U8                      bypass_mode;
    bool                    force_resume;
    DSP_FEATURE_RESOLUTION  resolution;
    DSP_FEATURE_NUMBERING   number;
#ifdef AIR_VOICE_PLC_ENABLE
    DSP_PKT_INFO_STRU       pkt_info;
#endif
#ifdef PRELOADER_ENABLE
    VOID*                   feature_ptr;
#endif
} DSP_ENTRY_PARA, *DSP_ENTRY_PARA_PTR;
#endif




/******************************************************************************
 * External Global Variables
 ******************************************************************************/
#if 0
extern stream_feature_list_t AudioFeatureList_BranchJoint[];
extern stream_feature_list_t AudioFeatureList_Source2Sink[];
extern stream_feature_list_t AudioFeatureList_CVSD_Source2Sink[];
extern stream_feature_list_t AudioFeatureList_RT[];
extern stream_feature_list_t gA2dpAudioRxFeatureList[];
#endif
extern stream_feature_table_t stream_feature_table[DSP_FEATURE_MAX_NUM];
#ifdef PRELOADER_ENABLE
extern stream_feature_ctrl_entry_t DSP_FeatureControl[DSP_FEATURE_MAX_NUM];
#if !PIC_LOGPRINT
extern uint32_t fake_printf( const char * format, ... );
#endif
#endif

/******************************************************************************
 * External Functions
 ******************************************************************************/
extern bool stream_function_end_process     (void *para);
extern bool stream_pcm_copy_initialize  (void *para);
extern bool stream_function_end_initialize   (void *para);


#endif /* _DSP_FEATUREINTERFACE_H_ */

