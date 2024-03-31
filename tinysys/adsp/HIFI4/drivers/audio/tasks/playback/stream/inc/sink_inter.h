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


#ifndef _SINK_INTER_H
#define _SINK_INTER_H
/*!
 *@file   Sink_private.h
 *@brief  defines the detail structure of sink
 *
 @verbatim
 @endverbatim
 */

//-
#include "audio_types.h"
#include "dsp_task.h"
#include "stream_config.h"
#include "transform_.h"
#include "sink_.h"
#include "audio_common.h"

////////////////////////////////////////////////////////////////////////////////
// CONSTANT DEFINITIONS ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define SINK_TYPE_L2CAP_NUM   8
#define SINK_TYPE_DSP_NUM     4
#define SINK_TYPE_RFCOMM_NUM 2


////////////////////////////////////////////////////////////////////////////////
// Global Variables ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
extern SINK Sink_blks[];

////////////////////////////////////////////////////////////////////////////////
// TYPE DEFINITIONS ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
typedef BOOL (*sink_write_buffer_entry)(SINK sink, U8 *src_addr, U32 length);

typedef enum {
    SINK_TYPE_HOST = (U8)0,
    SINK_TYPE_UART,
    SINK_TYPE_AUDIO,
    SINK_TYPE_VP_AUDIO,
    SINK_TYPE_SCO,
    SINK_TYPE_N9SCO,
    SINK_TYPE_RFCOMM_MIN,
    SINK_TYPE_RFCOMM_0 = SINK_TYPE_RFCOMM_MIN,
    SINK_TYPE_RFCOMM_1,
    SINK_TYPE_RFCOMM_MAX = SINK_TYPE_RFCOMM_MIN + SINK_TYPE_RFCOMM_NUM - 1,
    SINK_TYPE_L2CAP_MIN,
    SINK_TYPE_L2CAP_0 = SINK_TYPE_L2CAP_MIN,
    SINK_TYPE_L2CAP_1,
    SINK_TYPE_L2CAP_2,
    SINK_TYPE_L2CAP_3,
    SINK_TYPE_L2CAP_4,
    SINK_TYPE_L2CAP_5,
    SINK_TYPE_L2CAP_6,
    SINK_TYPE_L2CAP_7,
    SINK_TYPE_L2CAP_MAX = SINK_TYPE_L2CAP_MIN + SINK_TYPE_L2CAP_NUM - 1,
    SINK_TYPE_DSP_MIN,
    SINK_TYPE_DSP_VIRTUAL = SINK_TYPE_DSP_MIN,
    SINK_TYPE_DSP_AUDIOQ,
    SINK_TYPE_DSP_JOINT,
    SINK_TYPE_DSP_JOINT_0 = SINK_TYPE_DSP_JOINT,
    SINK_TYPE_DSP_JOINT_1,
    SINK_TYPE_DSP_JOINT_MAX = SINK_TYPE_DSP_JOINT_1,
    SINK_TYPE_DSP_MAX = SINK_TYPE_DSP_MIN + SINK_TYPE_DSP_NUM - 1,
    SINK_TYPE_USBAUDIOCLASS,
    SINK_TYPE_USBCDCCLASS,
    SINK_TYPE_HCI,
    SINK_TYPE_AIRAPP,
    SINK_TYPE_MEMORY,
    SINK_TYPE_CM4RECORD,//share memory
    SINK_TYPE_GSENSOR,
#ifdef AIR_BT_CODEC_BLE_ENABLED
    SINK_TYPE_N9BLE,
#endif
#ifdef MTK_AUDIO_TRANSMITTER_ENABLE
    SINK_TYPE_AUDIO_TRANSMITTER_MIN,
    SINK_TYPE_AUDIO_TRANSMITTER_0 = SINK_TYPE_AUDIO_TRANSMITTER_MIN,
    SINK_TYPE_AUDIO_TRANSMITTER_1,
    SINK_TYPE_AUDIO_TRANSMITTER_2,
    SINK_TYPE_AUDIO_TRANSMITTER_3,
    SINK_TYPE_AUDIO_TRANSMITTER_4,
    SINK_TYPE_AUDIO_TRANSMITTER_5,
    SINK_TYPE_AUDIO_TRANSMITTER_6,
    SINK_TYPE_AUDIO_TRANSMITTER_7,
    SINK_TYPE_AUDIO_TRANSMITTER_MAX = SINK_TYPE_AUDIO_TRANSMITTER_7,
#endif
#ifdef AIR_AUDIO_I2S_SLAVE_TDM_ENABLE
    SINK_TYPE_TDMAUDIO,
#endif
#ifdef MTK_AUDIO_BT_COMMON_ENABLE
    SINK_TYPE_BT_COMMON_MIN,
    SINK_TYPE_BT_COMMON_0 = SINK_TYPE_BT_COMMON_MIN,
    SINK_TYPE_BT_COMMON_1,
    SINK_TYPE_BT_COMMON_2,
    SINK_TYPE_BT_COMMON_3,
    SINK_TYPE_BT_COMMON_4,
    SINK_TYPE_BT_COMMON_5,
    SINK_TYPE_BT_COMMON_6,
    SINK_TYPE_BT_COMMON_7,
    SINK_TYPE_BT_COMMON_MAX = SINK_TYPE_BT_COMMON_7,
#endif
    SINK_TYPE_AUDIO_DL3,
    SINK_TYPE_AUDIO_DL12,
    SINK_TYPE_ADV_PASSTRU,
    SINK_TYPE_MAX,
} SINK_TYPE;

typedef struct __SINK_IF {
    U32  (*SinkSlack)(SINK sink);
    U32  (*SinkClaim)(SINK sink, U32 extra);
    U8*  (*SinkMap)(SINK sink);
    BOOL (*SinkFlush)(SINK sink, U32 amount);
    VOID (*SinkFlushAll)(SINK sink);
    BOOL (*SinkConfigure)(SINK sink, stream_config_type type, U32 value);
    BOOL (*SinkClose)(SINK sink);

    //Mediatek
    BOOL (*SinkWriteBuf)(SINK sink, U8 *src_addr, U32 length);
} SINK_IF, * SINKIF;

typedef struct __OSMEMBLK {
    U8*  addr;
    U32  length;
} OSMEMBLK;

struct __SINK {

    SINK_TYPE   type;

    BUFFER_TYPE buftype;

    TaskHandle_t   taskid;

    TRANSFORM   transform;

    STREAM_BUFFER  streamBuffer;

    SINK_PARAMETER param;

    SINK_IF     sif;
};


#endif
