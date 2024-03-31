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

 
#ifndef _STREAM_AUDIO_H_
#define _STREAM_AUDIO_H_

/*!
 *@file   stream_audio.h
 *@brief  defines the setting of audio stream
 *
 @verbatim
 @endverbatim
 */


#define SUPPORT_AUDIOQ (0)

//#include "config.h"
#include "audio_types.h"
#include "source_inter.h"
#include "sink_inter.h"


#include "source.h"
#include "sink.h"
#include "transform_inter.h"
#include "dsp_callback.h"

#include "stream_audio_hardware.h"
#include "stream_audio_setting.h"
#include "stream_audio_driver.h"

////////////////////////////////////////////////////////////////////////////////
// Type Defintions /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
typedef VOID (*Intrhandler)(VOID);
typedef BOOL (*Audiohandler)(SOURCE source,SINK sink);

typedef struct buffer_t{
    VOID*    startAddr;
    U32      length;
}BUFFER_s;

typedef struct audio_queue_stru{
    U16         header;
    U16         length;
    U32         _reserved[6];
    U8          data_space[1];
}AUDIO_QUEUE_STRU, *AUDIO_QUEUE_STRU_PTR;


////////////////////////////////////////////////////////////////////////////////
// Function Declarations ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
extern VOID Source_Audio_Path_Interface_Init(SOURCE source);
extern VOID Source_Audio_SubPath_Interface_Init(SOURCE source);
extern VOID Sink_Audio_Path_Interface_Init(SINK sink);
extern VOID Sink_Audio_Path_Init(SINK sink);
extern VOID Sink_Audio_Buffer_Ctrl(SINK sink, BOOL isEnabled);
extern BOOL SinkWriteBufAudio (SINK sink, U8 *src_addr, U32 length);
extern VOID Source_Audio_SelectInstance(audio_hardware hardware, audio_instance instance);
extern VOID Sink_VirtualSink_Interface_Init(SINK sink);
extern U32  SourceSize_AudioPattern_VP(SOURCE source);


#endif /* _STREAM_AUDIO_H_ */

