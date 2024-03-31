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

//#include "config.h"
#include "dsp.h"
#include "dsp_interface.h"
#include "dsp_audio_ctrl.h"
#include "dsp_drv_afe.h"
#include "dsp_sdk.h"
//#include "task_def.h"
#include "stream.h"
#include "dsp_task.h"

#if 0 /* it seems useless */
#ifdef PRELOADER_ENABLE
#include "preloader_pisplit.h"
#endif
#endif

typedef enum {
    TASK_PRIORITY_IDLE = 0,                                 /* lowest, special for idle task */
    TASK_PRIORITY_SYSLOG,                                   /* special for syslog task */

    /* User task priority begin, please define your task priority at this interval */
    TASK_PRIORITY_LOW,                                      /* low */
    TASK_PRIORITY_BELOW_NORMAL,                             /* below normal */
    TASK_PRIORITY_NORMAL,                                   /* normal */
    TASK_PRIORITY_ABOVE_NORMAL,                             /* above normal */
    TASK_PRIORITY_HIGH,                                     /* high */
    TASK_PRIORITY_SOFT_REALTIME,                            /* soft real time */
    TASK_PRIORITY_HARD_REALTIME,                            /* hard real time */
    /* User task priority end */

    /*Be careful, the max-priority number can not be bigger than configMAX_PRIORITIES - 1, or kernel will crash!!! */
    TASK_PRIORITY_TIMER = configMAX_PRIORITIES - 1,         /* highest, special for timer task to keep time accuracy */
} task_priority_type_t;


VOID* pvTaskParameter;
/**
 * DSP_Init
 *
 * Initialization for DSP Tasks
 */
VOID dsp_playback_init(VOID)
{
#if 0 /* not used */
    xTaskCreate( (TaskFunction_t)DTM, "DTM_TASK", 0xa00 / sizeof(StackType_t), pvTaskParameter, TASK_PRIORITY_NORMAL, &pDTM_TaskHandler );
#endif

#if 0 /* temporarily reuse DAVT for promt sound */
#ifdef MTK_PROMPT_SOUND_ENABLE
    xTaskCreate( (TaskFunction_t)DSP_PR_Task, "DPR_TASK", 0xc00 / sizeof(StackType_t), pvTaskParameter, TASK_PRIORITY_BELOW_NORMAL, &pDPR_TaskHandler );
#endif
#endif

#if defined(AIR_GAMING_MODE_DONGLE_ENABLE) || defined(AIR_BLE_AUDIO_DONGLE_ENABLE)
    xTaskCreate( (TaskFunction_t)DSP_AV_Task, "DAV_TASK", 0x1800 / sizeof(StackType_t), pvTaskParameter, TASK_PRIORITY_SOFT_REALTIME, &pDAV_TaskHandler );
#else
    xTaskCreate( (TaskFunction_t)DSP_AV_Task, "DAV_TASK", 0x1800 / sizeof(StackType_t), pvTaskParameter, TASK_PRIORITY_NORMAL, &pDAV_TaskHandler );
#endif

#if defined(AIR_WIRED_AUDIO_ENABLE) || defined(AIR_ADVANCED_PASSTHROUGH_ENABLE) || defined(AIR_PROMPT_SOUND_DUMMY_SOURCE_ENABLE) || defined(MTK_CM4_RECORD_ENABLE) || defined(MTK_BT_A2DP_AIRO_CELT_ENABLE)
#if defined(MTK_GAMING_MODE_HEADSET) || defined(AIR_GAMING_MODE_DONGLE_ENABLE) || defined(AIR_BLE_AUDIO_DONGLE_ENABLE) || defined(AIR_ADVANCED_PASSTHROUGH_ENABLE)
    xTaskCreate( (TaskFunction_t)DSP_HP_Task, "DHP_TASK", 0x1000 / sizeof(StackType_t), pvTaskParameter, TASK_PRIORITY_HARD_REALTIME, &pDHP_TaskHandler );
#else
    xTaskCreate( (TaskFunction_t)DSP_HP_Task, "DHP_TASK", 0x1000 / sizeof(StackType_t), pvTaskParameter, TASK_PRIORITY_ABOVE_NORMAL, &pDHP_TaskHandler );
#endif
#endif

#ifdef AIR_DONGLE_AFE_USAGE_CHECK_ENABLE
    DSP_MW_LOG_I("AFE USAGE CHECK enabled, 0xC0000000 disable", 0);
    xthal_set_cacheattr(0x2F222224);
#endif

    DSP_CTRL_Initialization();

#if 0 /* it seems useless */
#ifdef PRELOADER_ENABLE
    extern void pisplit_configure_static_pool();
    preloader_pisplit_init();
    pisplit_configure_static_pool();
#endif
#endif

    Stream_Init();
    dsp_sdk_initialize();
}
