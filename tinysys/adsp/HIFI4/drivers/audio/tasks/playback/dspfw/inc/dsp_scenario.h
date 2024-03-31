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



#ifndef _DSP_SCENARIO_H_
#define _DSP_SCENARIO_H_

#include "audio_types.h"
//#include "config.h"
#include "dsp_interface.h"
#include "dtm.h"
#include "audio_config.h"
#include "stream_audio_setting.h"
#include "stream_audio.h"
#include "stream.h"
#include "dsp_sdk.h"
#include "audio_messenger_ipi.h"

//#include "hal_gpt.h"
//#include "dsp_audio_msg.h"
//#include "hal_resource_assignment.h"
//#include "dsp_audio_msg_define.h"
/******************************************************************************
 * External Global Variables
 ******************************************************************************/
typedef struct {
    SOURCE      source;
    SINK        sink;
    TRANSFORM   transform;
    stream_feature_list_ptr_t pfeature_table;
} CONNECTION_IF;
// For Audio Sync
#define AUDIO_DSP_SYNC_MIN_TIME_OUT                            (1000)   // 1ms
#define AUDIO_DSP_SYNC_FIXED_POLLING_TIME                      (600) // 600us
#define AUDIO_DSP_SYNC_REQUEST_MIN_REQUEST_INTERVAL            (800) // 800us
//#define MTK_AUDIO_DSP_SYNC_REQUEST_GPIO_DEBUG
typedef enum {
    SUBMSG_MCU2DSP_SYNC_START = 0,
    SUBMSG_MCU2DSP_SYNC_STOP = 1,
    SUBMSG_MCU2DSP_SYNC_SET_VOLUME = 2,
    SUBMSG_MCU2DSP_SYNC_SET_MUTE = 3,
    SUBMSG_MCU2DSP_SYNC_FADE_OUT,
    SUBMSG_MCU2DSP_SYNC_ACTION_MAX = 0xFFFFFFFF,
} dsp_sync_request_action_id_t;
typedef enum {
    MSG_MCU2DSP_HFP_SYNC_REQUEST = 0,
    MSG_MCU2DSP_A2DP_SYNC_REQUEST,
    MSG_MCU2DSP_ANC_SYNC_REQUEST,
    MSG_MCU2DSP_VP_SYNC_REQUEST,
    MSG_MCU2DSP_SYNC_SCENARIO_MAX = 0xFFFFFFFF,
} dsp_sync_request_scenario_id_t;

typedef enum {
    AUDIO_SYNC_GAIN_SELECT_CHANNEL_L = 0,
    AUDIO_SYNC_GAIN_SELECT_CHANNEL_R,
    AUDIO_SYNC_GAIN_SELECT_CHANNEL_DUAL,
    AUDIO_SYNC_GAIN_SELECT_CHANNEL_MAX = 0xFFFFFFFF
} gain_select_t;

typedef struct {
    uint32_t gain;
    gain_select_t gain_select;
} vol_gain_info_t;

typedef struct  {
    uint32_t gpt_count;        // the target gpt timer count of sync request
    vol_gain_info_t vol_gain_info; // downlink volume info
} cm4_dsp_sync_param_t;

typedef struct  {
    cm4_dsp_sync_param_t param;
    uint32_t  msg_id;
    uint64_t  mirror_count; // only used for sorting in timer list
} timer_list_info_t;

typedef struct timer_list_node_t {
    timer_list_info_t info;
    struct timer_list_node_t *next;
} timer_list_node_t;


typedef void(* dsp_sync_callback_t)(dsp_sync_request_action_id_t request_action_id, void *user_data);
/******************************************************************************
 * External Function Prototypes
 ******************************************************************************/
#ifdef MTK_BT_A2DP_ENABLE
extern void CB_N9_A2DP_OPEN(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_A2DP_START(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_A2DP_STOP(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_A2DP_CLOSE(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_A2DP_SUSPEND(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_N9_A2DP_RESUME(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#ifdef MTK_BT_HFP_ENABLE
extern void CB_N9_SCO_UL_OPEN(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_SCO_UL_START(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_SCO_UL_STOP(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_SCO_UL_CLOSE(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_SCO_UL_PLAY(hal_ccni_message_t , hal_ccni_message_t *);
extern void CB_N9_SCO_UL_SUSPEND(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_N9_SCO_UL_RESUME(hal_ccni_message_t msg, hal_ccni_message_t *ack);

extern void CB_N9_SCO_DL_OPEN(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_SCO_DL_START(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_SCO_DL_STOP(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_SCO_DL_CLOSE(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_SCO_DL_SUSPEND(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_N9_SCO_DL_RESUME(hal_ccni_message_t msg, hal_ccni_message_t *ack);

extern void CB_N9_SCO_ULIRQ(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_SCO_DLIRQ(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_SCO_MICIRQ(hal_ccni_message_t, hal_ccni_message_t*);
#endif

#ifdef AIR_VOICE_NR_ENABLE
#ifdef MTK_AIRDUMP_EN
extern void CB_CM4_SCO_AIRDUMP_EN(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif
extern void CB_CM4_SCO_DL_AVC_VOL_UPDATE(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void dsp_get_reference_gain(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#ifdef AIR_BT_CODEC_BLE_ENABLED
extern void CB_N9_BLE_UL_OPEN(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_BLE_UL_START(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_BLE_UL_STOP(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_BLE_UL_CLOSE(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_BLE_UL_PLAY(hal_ccni_message_t , hal_ccni_message_t *);
extern void CB_N9_BLE_UL_SUSPEND(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_N9_BLE_UL_RESUME(hal_ccni_message_t msg, hal_ccni_message_t *ack);

extern void CB_N9_BLE_DL_OPEN(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_BLE_DL_START(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_BLE_DL_STOP(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_BLE_DL_CLOSE(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_BLE_DL_SUSPEND(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_N9_BLE_DL_RESUME(hal_ccni_message_t msg, hal_ccni_message_t *ack);

extern void CB_N9_BLE_UL_INFO(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_N9_BLE_DL_INFO(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_N9_BLE_UL_PLAYBACK_DATA_INFO(hal_ccni_message_t msg, hal_ccni_message_t *ack);


extern void CB_CM4_BLE_DL_AVC_VOL_UPDATE(hal_ccni_message_t msg, hal_ccni_message_t *ack);

extern void CB_N9_BLE_ULIRQ(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_BLE_DLIRQ(hal_ccni_message_t, hal_ccni_message_t*);
extern void CB_N9_BLE_MICIRQ(hal_ccni_message_t, hal_ccni_message_t*);

extern void CB_N9_BLE_INIT_PLAY_INFO(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#ifdef CFG_CM4_PLAYBACK_ENABLE
extern void CB_CM4_PLAYBACK_OPEN(ipi_msg_t* msg_ptr);
extern void CB_CM4_PLAYBACK_START(ipi_msg_t* msg_ptr);
extern void CB_CM4_PLAYBACK_STOP(ipi_msg_t* msg_ptr);
extern void CB_CM4_PLAYBACK_CLOSE(ipi_msg_t* msg_ptr);
extern void CB_CM4_PLAYBACK_DATA_REQ_ACK(void);
extern void CB_CM4_PLAYBACK_SUSPEND(ipi_msg_t* msg_ptr);
extern void CB_CM4_PLAYBACK_RESUME(ipi_msg_t* msg_ptr);
void query_playback_shared_mem(uint32_t *share_addr);
void trigger_playback_sink_out(void);
#endif
extern void CB_PLAYBACK_GET_SINK_MEM(ipi_msg_t* msg_ptr);

#ifdef MTK_LINEIN_PLAYBACK_ENABLE
extern void CB_CM4_LINEIN_PLAYBACK_OPEN(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_LINEIN_PLAYBACK_START(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_LINEIN_PLAYBACK_STOP(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_LINEIN_PLAYBACK_CLOSE(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_LINEIN_PLAYBACK_SUSPEND(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_LINEIN_PLAYBACK_RESUME(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_TRULY_LINEIN_PLAYBACK_OPEN(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_TRULY_LINEIN_PLAYBACK_CLOSE(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#ifdef MTK_CM4_RECORD_ENABLE
extern void CB_CM4_RECORD_OPEN(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_RECORD_START(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_RECORD_STOP(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_RECORD_CLOSE(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_RECORD_SUSPEND(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_RECORD_RESUME(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#ifdef MTK_LEAKAGE_DETECTION_ENABLE
extern void CB_CM4_RECORD_LC_SET_PARAM_ACK(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif
#endif

#ifdef AIR_SIDETONE_ENABLE
extern void dsp_sidetone_start(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void dsp_sidetone_stop(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void dsp_sidetone_set_volume(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void dsp_sidetone_start_volume_set(void);
#endif

#if defined(MTK_PEQ_ENABLE) || defined(MTK_LINEIN_PEQ_ENABLE)
extern void dsp_peq_set_param(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#ifdef MTK_BT_A2DP_ENABLE
extern void dsp_alc_switch(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif


//extern void dsp_set_algorithm_param(hal_ccni_message_t msg, hal_ccni_message_t *ack);
//extern void CB_AUDIO_DUMP_INIT(hal_ccni_message_t msg, hal_ccni_message_t *ack);


#ifdef MTK_PROMPT_SOUND_ENABLE
extern void CB_CM4_VP_PLAYBACK_OPEN(ipi_msg_t* msg_ptr);
extern void CB_CM4_VP_PLAYBACK_START(ipi_msg_t* msg_ptr);
extern void CB_CM4_VP_PLAYBACK_STOP(ipi_msg_t* msg_ptr);
extern void CB_CM4_VP_PLAYBACK_CONFIG(ipi_msg_t* msg_ptr);
extern void CB_CM4_VP_PLAYBACK_CLOSE(ipi_msg_t* msg_ptr);
extern void CB_CM4_VP_PLAYBACK_TRIGGER(ipi_msg_t* msg_ptr);
extern void CB_CM4_VP_PLAYBACK_DATA_REQ_ACK(void);
#endif

#ifdef AIR_PROMPT_SOUND_DUMMY_SOURCE_ENABLE
extern void CB_CM4_VP_DUMMY_SOURCE_PLAYBACK_OPEN(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_VP_DUMMY_SOURCE_PLAYBACK_START(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_VP_DUMMY_SOURCE_PLAYBACK_STOP(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_VP_DUMMY_SOURCE_PLAYBACK_CLOSE(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_CM4_VP_DUMMY_SOURCE_PLAYBACK_CHANGE_FEATURE(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#ifdef AIR_BT_CLK_SKEW_ENABLE
extern void CB_N9_CLK_SKEW_LAG(hal_ccni_message_t msg, hal_ccni_message_t *ack);
extern void CB_N9_CLK_SKEW_LEAD(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void DSP_CLK_SKEW_DEBUG_CONTROL(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

//void CB_CM4_AUDIO_AMP_FORCE_CLOSE(hal_ccni_message_t msg, hal_ccni_message_t *ack);

#ifdef MTK_SENSOR_SOURCE_ENABLE
void CB_GSENSOR_DETECT_START(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void CB_GSENSOR_DETECT_STOP(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void CB_GSENSOR_DETECT_READ_RG(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void CB_GSENSOR_DETECT_WRITE_RG(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#ifdef MTK_AUDIO_PLC_ENABLE
void DSP_AUDIO_PLC_CONTROL(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#ifdef USE_CCNI
void DSP_GC_SetHWGainWithFadeTime(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#ifdef HAL_AUDIO_DSP_SHUTDOWN_SPECIAL_CONTROL_ENABLE
void DSP_DUMMY_SHUTDOWN(void);
#endif

#ifdef MTK_AUDIO_TRANSMITTER_ENABLE
void audio_transmitter_open(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void audio_transmitter_start(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void audio_transmitter_stop(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void audio_transmitter_close(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void audio_transmitter_config(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void audio_transmitter_suspend(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void audio_transmitter_resume(hal_ccni_message_t msg, hal_ccni_message_t *ack);
void audio_transmitter_send_message(audio_transmitter_scenario_t scenario_type, audio_transmitter_scenario_sub_id_t scenario_id, uint32_t message);
void gaming_headset_uplink_enable_irq(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#ifdef MTK_SLT_AUDIO_HW
void AUDIO_SLT_START(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif
#ifdef MTK_AUDIO_LOOPBACK_TEST_ENABLE
void DSP_AUDIO_LOOPBACK_TEST_CONFIG(hal_ccni_message_t msg, hal_ccni_message_t *ack);
#endif

#if 0 /* it seems useless */
extern void dsp_audio_request_sync_process(hal_ccni_message_t msg, hal_ccni_message_t *ack);
aud_msg_status_t dsp_audio_request_sync_register_callback(dsp_sync_request_scenario_id_t request_scenario_id, dsp_sync_callback_t *sync_callback);
void dsp_audio_request_sync_initialization(void);
#endif

#ifdef  AIR_AUDIO_HW_LOOPBACK_ENABLE
void audio_hw_loopback_echo_enable(bool enable);
#endif

#endif /* _DSP_SCENARIO_H_ */

