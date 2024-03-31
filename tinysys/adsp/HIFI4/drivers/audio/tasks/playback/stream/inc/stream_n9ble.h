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


#ifndef _STREAM_N9BLE_H_
#define _STREAM_N9BLE_H_

/*!
 *@file   stream.h
 *@brief  defines the heap management of system
 */

//#include "config.h"
#include "audio_types.h"
#include "source_inter.h"
#include "sink_inter.h"

#include "source.h"
#include "sink.h"
//#include "transform.h"

#include "transform_inter.h"

#include "audio_common.h"



////////////////////////////////////////////////////////////////////////////////
// Type Defintions /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


typedef struct N9Ble_Sink_Config_s {
    U16  N9_Ro_abnormal_cnt;
    U16  Buffer_Frame_Num;
    U16  Process_Frame_Num;
    U16  Frame_Size;
//    U16  Output_sample_rate;
//    BOOL isEnable;
} N9Ble_Sink_config_t;

typedef struct N9Ble_Source_Config_s {
    U16  Buffer_Frame_Num;
    U16  Process_Frame_Num;
    U16  Frame_Size;
    U16  Input_sample_rate;
//    BOOL isEnable;
} N9Ble_Source_config_t;

typedef enum {
    BLE_PKT_FREE,
    BLE_PKT_USED,
    BLE_PKT_LOST,
} ble_packet_state;

typedef struct Stream_n9ble_Config_s {
    N9Ble_Source_config_t N9Ble_source;
    N9Ble_Sink_config_t   N9Ble_sink;
} Stream_n9ble_Config_t, *Stream_n9ble_Config_Ptr;




////////////////////////////////////////////////////////////////////////////////
// Function Declarations ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
extern VOID SinkInitN9Ble(SINK sink);
extern VOID SourceInitN9Ble(SOURCE source);
extern Stream_n9ble_Config_Ptr N9BLE_setting;
extern bool ble_query_rx_packet_lost_status(uint32_t index);
ATTR_TEXT_IN_IRAM_LEVEL_2 VOID N9Ble_update_readoffset_share_information(SOURCE source, U32 ReadOffset);
ATTR_TEXT_IN_IRAM_LEVEL_2 VOID N9Ble_update_writeoffset_share_information(SINK sink, U32 WriteOffset);
extern VOID N9Ble_SourceUpdateLocalReadOffset(SOURCE source, U8 num);
#if defined(AIR_UL_FIX_SAMPLING_RATE_48K) && defined(AIR_FIXED_RATIO_SRC)
extern void N9Ble_UL_Fix_Sample_Rate_Init(void);
extern void N9Ble_UL_Fix_Sample_Rate_Deinit(void);
#endif


#ifdef AIR_SOFTWARE_GAIN_ENABLE
#include "sw_gain_interface.h"
extern sw_gain_port_t *g_call_sw_gain_port;
extern volatile int32_t g_call_target_gain;
extern void Call_UL_SW_Gain_Mute_control(bool mute);
extern void Call_UL_SW_Gain_Set(int32_t target_gain);
extern void Call_UL_SW_Gain_Deinit(void);
#endif

#endif /* _STREAM_N9SCO_H_ */

