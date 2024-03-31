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

#ifndef __APPS_EVENT_LIST_H__
#define __APPS_EVENT_LIST_H__

typedef enum {
    KEY_ACTION_INVALID = 0,                         /**< invalid key action id. */

    KEY_ACTION_BASE  = 0x0001,                      /**< Declare event base, all active events must larger than it. */


    KEY_DISCOVERABLE = 0x0002,                      /**< Start BT discoverable. */
    KEY_HIDDEN = 0x0003,                            /**< Disable visibility and connectivity */
    KEY_VOICE_UP = 0x000A,                          /**< Volume up. */
    KEY_VOICE_DN = 0x000B,                          /**< Volume down. */

    KEY_POWER_ON = 0x0017,                          /**< It's a SW power on, so power on BT. */
    KEY_POWER_OFF = 0x0018,                         /**< System power off, but if device is not support power key, may do BT off only. */
    //KEY_SYSTEM_REBOOT = 0x0019,                     /**< Trigger system reboot. use cli: reboot */
    KEY_RESET_PAIRED_DEVICES = 0x001A,              /**< Unpair all devices. */
    KEY_DELETE_PAIRED_DEVICES = 0x001B,             /**< Delete paired devices.by address */
    KEY_PAIRED_DEVICES_LIST = 0x001C,               /**< List all paired devices. */
    KEY_SHOW_STATUS = 0x001D,                       /**< Show device status. */
    KEY_RECONNECT_LAST_DEVICE = 0x001E,             /**< Actively reconnect last device. */

#if 0 //reserved for HFP
    KEY_REDIAL_LAST_CALL = 0x0034,          /**< Redail last call. */
    KEY_CANCEL_OUT_GOING_CALL = 0x0037,     /**< Cancel outgoing call. */
    KEY_REJCALL = 0x0038,                   /**< Reject incoming call. */
    KEY_REJCALL_SECOND_PHONE,               /**< Reject the new incoming call in 3-way calling. */
    KEY_ONHOLD_CALL = 0x003A,               /**< Hold current call or resume from hold status. */
    KEY_ACCEPT_CALL = 0x003B,               /**< Accept incoming call. */
    KEY_END_CALL = 0x003C,                  /**< End current call. */

    KEY_SWITCH_AUDIO_PATH,                  /**< Switch the audio path of call through Smart phone speaker or headset. */

    KEY_3WAY_HOLD_ACTIVE_ACCEPT_OTHER = 0x0040, /**< Accept the new incoming call and hold current call. */
#endif /* #if 0 //reserved for HFP */

    KEY_SINK_CONNECT = 0x0041,                  /**< Trigger A2DP Sink Connection. */
    KEY_SINK_DISCONN = 0x0042,                  /**< Trigger A2DP Sink Disconnection. */
#ifdef MTK_BT_AVRCP_ENABLE
    KEY_AVRCP_PLAY = 0x0053,                    /**< Start or resume music playing. */
    KEY_AVRCP_PAUSE = 0x0055,                   /**< Pause music playing. */
    KEY_AVRCP_GET_ATTRIBUTE = 0x0056,           /**< Get element attribue. */
    KEY_AVRCP_GET_PLAY_STATUS = 0x0057,         /**< Get play status. */
    KEY_AVRCP_GET_CAPABILITY = 0x0058,          /**< Get capability. */
    KEY_AVRCP_REGISTER_EVENT = 0x0059,          /**< Register Event. */
    KEY_AVRCP_FORWARD = 0x005A,                 /**< When playing music, play next track. */
    KEY_AVRCP_BACKWARD = 0x005B,                /**< When playing music, play last track. */
    KEY_AVRCP_FAST_FORWARD_PRESS = 0x005C,      /**< When playing music, fast forward start. */
    KEY_AVRCP_FAST_FORWARD_RELEASE = 0x005D,    /**< When playing music, fast forward end. */
    KEY_AVRCP_FAST_REWIND_PRESS = 0x005E,       /**< When playing music, fast rewind start. */
    KEY_AVRCP_FAST_REWIND_RELEASE = 0x005F,     /**< When playing music, fast rewind end. */
#endif /* #ifdef MTK_BT_AVRCP_ENABLE */
    /* 0x007X Group is for A2DP sink testing only*/
    KEY_ROLE_SWITCH = 0x0070,                   /**< Set Role of ACL Link. */
    KEY_GET_ROLE = 0x0071,                      /**< Get Role of ACL Link. */
#ifdef MTK_BT_A2DP_ENABLE
    KEY_SBC_MAX_BIT_POOL = 0x0072,              /**< Set SBC Max Bit Pool size. */
#ifdef MTK_BT_AUDIO_PR
    KEY_AUDIO_PR = 0x0073,                      /**< Dump decode path record. */
#endif /* #ifdef MTK_BT_AUDIO_PR */
#endif /* #ifdef MTK_BT_A2DP_ENABLE */
    //KEY_PASS_THROUGH = 0x0090, /**< Passthrough on and off */
#ifdef AIR_LE_AUDIO_ENABLE
    KEY_LE_AUDIO_START_ADV = 0x0080,           /**< Start LE Audio ADV. */
    KEY_LE_AUDIO_STOP_ADV = 0x0081,            /**< Stop LE Audio ADV. */
    KEY_LE_AUDIO_DISCONNECT = 0x0082,          /**< Disconnect LE Dev. */
    KEY_LE_AUDIO_UNPAIR = 0x0083,              /**< Unpair LE Dev. */
    KEY_LE_AUDIO_RESET_PAIRED_LIST = 0x0084,   /**< Rest LE paired list. */
    KEY_LE_AUDIO_PAIRED_LIST = 0x0085,         /**< Show LE paired list. */
    KEY_LE_AUDIO_CONN_LIST = 0x0086,           /**< Show LEA connection list. */

    KEY_LE_MCP_PLAY = 0x0090,                  /**< Start or resume LE music playing. */
    KEY_LE_MCP_PAUSE = 0x0091,                 /**< Pause LE music playing. */
    KEY_LE_MCP_FAST_FORWARD = 0x0092,          /**< Fast forward LE music (Resend can adjust seeking speed). */
    KEY_LE_MCP_FAST_REWIND = 0x0093,           /**< Fast rewind LE music (Resend can adjust seeking speed). */
    KEY_LE_MCP_NEXT_TRACK = 0x0094,            /**< Go to next track. */
    KEY_LE_MCP_PRE_TRACK = 0x0095,             /**< Go to previous track. */

    KEY_LE_MCP_GET_SUPPORTED_CTRL_OPCODE = 0x00A1, /**< Get supported media control opcodes on the server. */
    KEY_LE_MCP_GET_MEDIA_STATE = 0x00A2,       /**< Get media state. */
    KEY_LE_MCP_GET_MEDIA_PLAYER_NAME = 0x00A3, /**< Get media player name. */
    KEY_LE_MCP_GET_TRACK_TITLE = 0x00A4,       /**< Get track title. */
    KEY_LE_MCP_GET_TRACK_DURATION = 0x00A5,    /**< Get track duration. */
    KEY_LE_MCP_GET_TRACK_POSITION = 0x00A6,    /**< Get track position. */

    KEY_LE_VCP_GET_VOLUME_INFO = 0x00B1,       /**< Get Volume Information. */

#endif /* #ifdef AIR_LE_AUDIO_ENABLE */
    //KEY_FACTORY_RESET = 0x0095,             /**< Do factory reset and reboot. */
    //KEY_FACTORY_RESET_AND_POWEROFF = 0x96,  /**< Do factory reset and power off. */

    KEY_RESET_LINK_KEY = 0x010B,        /**< Clear link key. */

    KEY_CUSTOMER_DEFINE_ACTIONS = 0xF000,   /**< Value larger than it is for customization. */
} apps_config_key_action_t;

#endif /* #ifndef __APPS_EVENT_LIST_H__ */
