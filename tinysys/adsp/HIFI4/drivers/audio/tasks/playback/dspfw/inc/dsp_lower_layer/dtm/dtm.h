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

 
#ifndef _DTM_H_
#define _DTM_H_

#include "audio_types.h"
#define DspIpcEnable (0)


/* Audio verification related*/
#define AudioTestMode (1)
#ifdef AudioTestMode
#define Mp3Audio_USB     (0)
#define Mp3Audio_SDC     (0)
#define Mp3Audio_FILE_STREAM   (0)
#define eSCO_CVSD      (0)
#define eSCO_mSBC      (0)
#define CDC2Audio      (0)
#define USBAudioClass  (0)
#define Memory2VP 	   (0)
#define CVSD_USB       (0)
#define VC_VIRTUAL     (0)
#define RF_TESTDSPOPEN (0)
#define AB155X_TEST    (0)
#define AB155X_SCO_TX_Test (0)
#define AB155X_SCO_RX_Test (0)
#define AB155X_AFE_DL_TEST (0)
#define AB155X_AFE_UL_TEST (0)
#define AB155X_AFE_A2DP_UT  (0)
#define AB155X_AFE_ESCO_UT  (0)
#endif
/* Audio verification related*/


#define MP3FORCEFILEDECODE


typedef enum
{
    DSP_CMD_CLEAR,
    DSP_CMD_PUTBACK,
    DSP_CMD_FORWARD_TO_DPRT,
    DSP_CMD_FORWARD_TO_DAVT,
} DSP_CMD_ACT_t;

typedef enum
{
    DSP_JOB_SUSPEND,
    DSP_JOB_INIT,
    DSP_JOB_START,
    DSP_JOB_DEINIT,
} DSP_JOB_STAT_t;

typedef enum
{
    DTM_EVENT_ID_SIDETONE_START,
    DTM_EVENT_ID_SIDETONE_STOP_RAMP,
    DTM_EVENT_ID_SIDETONE_STOP,
    DTM_EVENT_ID_GSENSOR_WATERMARK_TRIGGER,
    DTM_EVENT_ID_AUDIO_DUMP,
    DTM_EVENT_ID_VOW_ENABLE,
    DTM_EVENT_ID_VOW_DISABLE,
    DTM_EVENT_ID_GAMING_MODE_VOLUME_SMART_BALANCE_ENABLE,
    DTM_EVENT_ID_GAMING_MODE_VOLUME_SMART_BALANCE_PROCESS,
    DTM_EVENT_ID_GAMING_MODE_VOLUME_SMART_BALANCE_DISABLE,
    DTM_EVENT_ID_WIRED_AUDIO_USB_VOLUME_SMART_BALANCE_ENABLE,
    DTM_EVENT_ID_WIRED_AUDIO_USB_VOLUME_SMART_BALANCE_PROCESS,
    DTM_EVENT_ID_WIRED_AUDIO_USB_VOLUME_SMART_BALANCE_DISABLE,
    DTM_EVENT_ID_AUDIO_SYNC_END,
    DTM_EVENT_ID_NUM,
} DTM_EVENT_ID_t;

typedef struct DSP_TASK_LIST_s
{
    DSP_JOB_STAT_t AV;
    DSP_JOB_STAT_t PR;
} DSP_TASK_LIST_t;

typedef struct audio_Status_Tag
{
    U16 AUDIO_BUSY;                    /* audio routing currently in progress */
    U16 audio_in_use:1;                /* audio currently being routed */
    U16 tone_playing:1;                /* tone currently being played */
    U16 vp_playing:1;                  /* voice prompt currently being played */
    U16 asr_running:1;                 /* asr is currently running/listening */
    U16 content_protection:1;
    U16 unused:11;
}AUDIO_STATUS_t;


typedef struct audio_sink_Status_Tag
{
    BOOL DSP_Audio_busy;
    BOOL DSP_vp_path_busy;
}AUDIO_SINK_STATUS_t;

typedef struct DTM_QUEUE_s
{
    DTM_EVENT_ID_t event_id;
    U32 arg;
}DTM_QUEUE_t;

/*
 * External Global Variables
 */


/*
 * External Function Prototypes
 */
extern VOID DTM(VOID);
extern AUDIO_STATUS_t Audio_Status;
extern AUDIO_SINK_STATUS_t Audio_Sink_Status;
extern void DTM_enqueue(DTM_EVENT_ID_t id, U32 arg, BOOL isFromISR);
#endif /* _DTM_H_ */

