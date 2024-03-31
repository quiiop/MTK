/* Copyright Statement:
 *
 * (C) 2005-2021  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/**
 * File: apps_init.c
 *
 * Description: This file is used to init APP modules.
 *
 */

#include "ui_shell_manager.h"

#include "apps_init.h"

#include "apps_events_bt_event.h"

#include "apps_debug.h"

/*Include registered activities*/
#include "app_preproc_activity.h"
#include "app_home_screen_idle_activity.h"
#include "app_music_idle_activity.h"
#ifdef AIR_LE_AUDIO_ENABLE
#include "app_le_audio.h"
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */


static void apps_init_events_senders(void)
{
    //apps_event_key_event_init();

    apps_events_bt_event_init();
}


static void apps_init_applications(void)
{
    ui_shell_set_pre_proc_func(app_preproc_activity_proc);

    /*
     * Register all idle activities here. The last registered activity is the foreground activity.
     */
    ui_shell_start_activity(NULL, app_home_screen_idle_activity_proc, ACTIVITY_PRIORITY_IDLE_TOP, NULL, 0);

    //ui_shell_start_activity(NULL, app_event_state_report_activity_proc, ACTIVITY_PRIORITY_IDLE_BACKGROUND, NULL, 0);

    //ui_shell_start_activity(NULL, app_multi_va_idle_activity_proc, ACTIVITY_PRIORITY_IDLE_BACKGROUND, NULL, 0);

    //ui_shell_start_activity(NULL, app_hfp_idle_activity_proc, ACTIVITY_PRIORITY_IDLE_BACKGROUND, NULL, 0);
    ui_shell_start_activity(NULL, app_music_idle_activity_proc, ACTIVITY_PRIORITY_IDLE_BACKGROUND, NULL, 0);

#ifdef AIR_LE_AUDIO_ENABLE
    ui_shell_start_activity(NULL, app_le_audio_idle_activity_proc, ACTIVITY_PRIORITY_IDLE_BACKGROUND, NULL, 0);
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */

#ifdef MTK_BT_AUDIO_PR
    app_bt_dbg_audio_pr_init();
#endif /* #ifdef MTK_BT_AUDIO_PR */

}

/*
static void apps_init_multi_va(void)
{
    //multi_va_manager_start();
}*/

void apps_init(void)
{

    /* init APP event sender, such as bt event, etc. */
    apps_init_events_senders();

    /* All VA register to multi VA manager and start. */
    //apps_init_multi_va();
    /* Init all APP activities. */
    apps_init_applications();

    /* Start UI shell. */
    ui_shell_start();
}
