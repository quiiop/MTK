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


#ifndef _DSP_CALLBACK_H_
#define _DSP_CALLBACK_H_


#include "audio_types.h"
#include "dlist.h"
#include "source.h"
#include "sink.h"
#include "dsp_feature_interface.h"
#include "transform_.h"
#include "dsp_task.h"




////////////////////////////////////////////////////////////////////////////////
// CONSTANT DEFINITIONS ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define DSP_STREAMING_GET_FROM_PRAR(para) (DSP_CONTAINER_OF(DSP_CONTAINER_OF(para,      \
                                                           DSP_CALLBACK, EntryPara),  \
                                           DSP_STREAMING_PARA, callback))



#define ForceDSPCallback (1)

////////////////////////////////////////////////////////////////////////////////
// TYPE DEFINITIONS ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
typedef enum
{
    CALLBACK_DISABLE = 0,
    CALLBACK_MALLOC,
    CALLBACK_INIT,
    CALLBACK_SUSPEND,
    CALLBACK_HANDLER,
    CALLBACK_BYPASS_CODEC,
    CALLBACK_ZEROPADDING,
    CALLBACK_WAITEND,
}DSP_CALLBACK_STATUS, *DSP_CALLBACK_STATUS_PTR;

typedef enum
{
    STREAMING_DISABLE = 0,
    STREAMING_START,
    STREAMING_ACTIVE,
    STREAMING_DEINIT,
    STREAMING_END,
}DSP_STREAMING_STATUS, *DSP_STREAMING_STATUS_PTR;
typedef enum
{
    STREAMING_MODE = 1,
    BYPASS_CODEC_MODE = 2,
}DSP_CALLBACK_BYPASS_CODEC_MODE;

typedef struct Dsp_Feature_Table_s {
    U32   FeatureType   : 8;
    U32   MemSize       : 24;
    VOID* MemPtr;
    stream_feature_entry_t InitialEntry;
    stream_feature_entry_t ProcessEntry;
} DSP_FEATURE_TABLE, *DSP_FEATURE_TABLE_PTR;

typedef struct DSP_CALLBACK_SRC_s
{
    SRC_PTR_s volatile  src_ptr;
    U8                  inSRC_Full;
    U8                  outSRC_Full;
    U8                  _reserved;

    //User configure
    U8                  in_resolution;
    U8                  out_resolution;
    U8                  out_sampling_rate;
    U16                 out_frame_size;
} DSP_CALLBACK_SRC, *DSP_CALLBACK_SRC_PTR;

typedef struct DSP_CALLBACK_s {
    DSP_CALLBACK_STATUS volatile Status;
    DSP_FEATURE_TABLE_PTR        FeatureTablePtr;
    DSP_ENTRY_PARA               EntryPara;
    DSP_CALLBACK_SRC             Src;
    BOOL                         IsBusy;
} DSP_CALLBACK, *DSP_CALLBACK_PTR;

typedef struct Dsp_Streaming_Para_s {
    DSP_STREAMING_STATUS    streamingStatus ;
    SOURCE                  source;
    SINK                    sink;
    DSP_CALLBACK            callback;
    U16                     DspReportEndId;
} DSP_STREAMING_PARA, *DSP_STREAMING_PARA_PTR;

typedef struct DSP_CALLBACK_HANDLER_s {
    DSP_CALLBACK_STATUS         handlingStatus;
    DSP_CALLBACK_STATUS         nextStatus;
    DSP_STREAMING_PARA_PTR      stream;
} DSP_CALLBACK_HANDLER, *DSP_CALLBACK_HANDLER_PTR;

typedef struct DSP_CALLBACK_STREAM_CONFIG_s {
    BOOL                    is_enable;
    SOURCE                  source;
    SINK                    sink;

    VOID*                   feature_list_ptr;
    DSP_STREAMING_PARA_PTR  stream_ptr;
    U8                      stream_number;
    TaskHandle_t           task;
} DSP_CALLBACK_STREAM_CONFIG, *DSP_CALLBACK_STREAM_CONFIG_PTR;


typedef struct DSP_CALLBACK_STREAM_CLEAN_s {
    BOOL volatile           is_clean;
    DSP_STREAMING_PARA_PTR  stream_ptr;
} DSP_CALLBACK_STREAM_CLEAN, *DSP_CALLBACK_STREAM_CLEAN_PTR;


typedef BOOL (*CALLBACK_STATE_ENTRY)(DSP_ENTRY_PARA_PTR entry_para, DSP_FEATURE_TABLE_PTR feature_table_ptr);

////////////////////////////////////////////////////////////////////////////////
// DSP FUNCTION DECLARATIONS ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
extern TaskHandle_t  DSP_Callback_Config(SOURCE source, SINK sink, VOID* feature_list_ptr, BOOL isEnable);
extern VOID DSP_Callback_StreamingInit(DSP_STREAMING_PARA_PTR  stream_ptr, U8 stream_number, TaskHandle_t  task);
extern TaskHandle_t  DSP_Callback_StreamConfig(DSP_CALLBACK_STREAM_CONFIG_PTR stream_config_ptr);
extern VOID DSP_Callback_FeatureConfig(DSP_STREAMING_PARA_PTR stream, stream_feature_list_ptr_t feature_list_ptr);
#ifdef PRELOADER_ENABLE
extern VOID DSP_Callback_PreloaderConfig(stream_feature_list_ptr_t feature_list_ptr);
extern VOID DSP_Callback_FeatureDeinit(DSP_STREAMING_PARA_PTR stream);
extern VOID DSP_PIC_FeatureDeinit(stream_feature_type_ptr_t featureTypePtr);
#endif
extern TaskHandle_t  DSP_CallbackTask_Get(SOURCE source, SINK sink);
extern DSP_CALLBACK_PTR DSP_Callback_Get(SOURCE source, SINK sink);
extern DSP_STREAMING_PARA_PTR DSP_Streaming_Get(SOURCE source, SINK sink);
extern VOID DSP_Callback_Processing(DSP_STREAMING_PARA_PTR stream);
extern VOID* DSP_FuncMemPtr_Get(DSP_CALLBACK_PTR callback, stream_feature_function_entry_t entry, U32 feature_seq);
extern VOID DSP_Callback_ChangeStreaming2Deinit(VOID* para);

extern VOID DriftConfiguration(DSP_STREAMING_PARA_PTR stream, BOOL enable);
VOID DSP_Callback_ParaSetup(DSP_STREAMING_PARA_PTR stream);
extern DSP_CALLBACK_BYPASS_CODEC_MODE DSP_Callback_BypassModeGet(SOURCE source, SINK sink);
extern VOID DSP_Callback_BypassModeCtrl(SOURCE source, SINK sink , DSP_CALLBACK_BYPASS_CODEC_MODE mode);

extern VOID DSP_ChangeStreaming2Deinit (TRANSFORM transform);

#endif /* _DSP_CALLBACK_H_ */

