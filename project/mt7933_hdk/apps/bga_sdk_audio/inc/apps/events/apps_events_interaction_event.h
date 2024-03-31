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


#ifndef __APPS_EVENTS_INTERACTION_EVENT_H__
#define __APPS_EVENTS_INTERACTION_EVENT_H__

enum {
    /* The events below is used to support power off, reboot or disable/enable BT */
    APPS_EVENTS_INTERACTION_POWER_OFF_WAIT_TIMEOUT, /* app_home_screen_idle_activity send the event to itself with a short delay time when do power off */
    APPS_EVENTS_INTERACTION_REQUEST_POWER_OFF,      /* Any APPs can send the event to app_home_screen_idle_activity to trigger power off */
    APPS_EVENTS_INTERACTION_REQUEST_REBOOT,         /* Any APPs can send the event to app_home_screen_idle_activity to trigger reboot */
    APPS_EVENTS_INTERACTION_REQUEST_ON_OFF_BT,      /* Any APPs can send the event to app_home_screen_idle_activity to enable or disable BT */

    /* The events below is used to support sleep after no connection and wait long enough */
    APPS_EVENTS_INTERACTION_REFRESH_SLEEP_TIME, /* app_home_screen_idle_activity in Partner send the event to Agent for trigger it reset sleep timer */
    APPS_EVENTS_INTERACTION_GO_TO_SLEEP,        /* app_home_screen_idle_activity send the event to itself with a delay time to trigger sleep flow.
                                                   The agent sends the event to partner to make partner sleep at the same time. */
    APPS_EVENTS_INTERACTION_SLEEP_WAIT_TIMEOUT, /* app_home_screen_idle_activity send the event to itself with a short delay time to sleep. */

    /* The events below is used to support update states */
    APPS_EVENTS_INTERACTION_UPDATE_MMI_STATE,   /* Activities update mmi state when it receive the event. If an activity want to set MMI state, it should
                                                   returns true when receives the event to avoid next activity receives the event. */

    /* The events below is used to support BT update states */
    APPS_EVENTS_INTERACTION_INCREASE_BLE_ADV_INTERVAL, /**< Use a small interval in first 30 seconds, and bt_app_common sends the interval to ask APPs to stop ble adv
                                                            when timeout. */
    APPS_EVENTS_INTERACTION_BT_VISIBLE_STATE_CHANGE,   /**< Agent send current bt visible state to partner */
    APPS_EVENTS_INTERACTION_SET_BT_VISIBLE,            /**< Partner send the aws event to agent to start bt visible */
    APPS_EVENTS_INTERACTION_AUTO_START_BT_VISIBLE,     /**< Send the event to start auto bt visible with a delay time */
    APPS_EVENTS_INTERACTION_BT_VISIBLE_TIMEOUT,        /**< Send the event to stop bt visible with a delay time */
    APPS_EVENTS_INTERACTION_BT_RECONNECT_DEVICE,       /**< Send the event to reconnect remote device */
    APPS_EVENTS_INTERACTION_BT_RECONNECT_TIMEOUT,      /**< Send the event to stop bt reconnect with a delay time */
    APPS_EVENTS_INTERACTION_BT_RETRY_POWER_ON_OFF,     /**< Send the event when call BT standby or active fail, need retry. */
};

#endif /* #ifndef __APPS_EVENTS_INTERACTION_EVENT_H__ */
