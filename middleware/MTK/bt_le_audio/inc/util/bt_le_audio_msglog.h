/* Copyright Statement:
 *
 * (C) 2020  MediaTek Inc. All rights reserved.
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

#ifndef __BT_LE_AUDIO_MSGLOG_H__
#define __BT_LE_AUDIO_MSGLOG_H__

#include "syslog.h"

#if 1 //Used by MT793X
/** @brief This macro calls the LOG_MSGID_D function to print the user's log string with an debug level.
 *  @param[in] msg is the user's log string.
 *  @param[in] ... is the parameter list corresponding with the message string.
*/
#define LE_AUDIO_MSGLOG_D(msg, cnt, arg...) LOG_I(LE_AUDIO, msg, ##arg)

/** @brief This macro calls the LOG_MSGID_D function to print the user's log string with an information level.
 *  @param[in] msg is the user's log string.
 *  @param[in] ... is the parameter list corresponding with the message string.
*/
#define LE_AUDIO_MSGLOG_I(msg, cnt, arg...) LOG_I(LE_AUDIO, msg, ##arg)

/** @brief This macro calls the LOG_MSGID_D function to print the user's log string with an warning level.
 *  @param[in] msg is the user's log string.
 *  @param[in] ... is the parameter list corresponding with the message string.
*/
#define LE_AUDIO_MSGLOG_W(msg, cnt, arg...) LOG_W(LE_AUDIO, msg, ##arg)

/** @brief This macro calls the LOG_MSGID_D function to print the user's log string with an error level.
 *  @param[in] msg is the user's log string.
 *  @param[in] ... is the parameter list corresponding with the message string.
*/
#define LE_AUDIO_MSGLOG_E(msg, cnt, arg...) LOG_E(LE_AUDIO, msg, ##arg)

/** @brief This macro calls the LE_AUDIO_MSGLOG_I function to print the user's log string with an information level.
 *  @param[in] msg is the user's log string.
 *  @param[in] ... is the parameter list corresponding with the message string.
*/
#define le_audio_log(msg, cnt, ...) LOG_I(LE_AUDIO, msg, ##__VA_ARGS__)

#else
/** @brief This macro calls the LOG_MSGID_D function to print the user's log string with an debug level.
 *  @param[in] msg is the user's log string.
 *  @param[in] ... is the parameter list corresponding with the message string.
*/
#define LE_AUDIO_MSGLOG_D(msg, cnt, arg...) LOG_MSGID_D(LE_AUDIO, msg, cnt, ##arg)

/** @brief This macro calls the LOG_MSGID_D function to print the user's log string with an information level.
 *  @param[in] msg is the user's log string.
 *  @param[in] ... is the parameter list corresponding with the message string.
*/
#define LE_AUDIO_MSGLOG_I(msg, cnt, arg...) LOG_MSGID_I(LE_AUDIO, msg, cnt, ##arg)

/** @brief This macro calls the LOG_MSGID_D function to print the user's log string with an warning level.
 *  @param[in] msg is the user's log string.
 *  @param[in] ... is the parameter list corresponding with the message string.
*/
#define LE_AUDIO_MSGLOG_W(msg, cnt, arg...) LOG_MSGID_W(LE_AUDIO, msg, cnt, ##arg)

/** @brief This macro calls the LOG_MSGID_D function to print the user's log string with an error level.
 *  @param[in] msg is the user's log string.
 *  @param[in] ... is the parameter list corresponding with the message string.
*/
#define LE_AUDIO_MSGLOG_E(msg, cnt, arg...) LOG_MSGID_E(LE_AUDIO, msg, cnt, ##arg)

/** @brief This macro calls the LE_AUDIO_MSGLOG_I function to print the user's log string with an information level.
 *  @param[in] msg is the user's log string.
 *  @param[in] ... is the parameter list corresponding with the message string.
*/
#define le_audio_log(msg, cnt, ...) LOG_MSGID_I(LE_AUDIO, msg, cnt, ##__VA_ARGS__)
#endif

#endif  /* __BT_LE_AUDIO_MSGLOG_H__ */

