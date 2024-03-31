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

#ifndef __BLE_AICS_H__
#define __BLE_AICS_H__

#include "ble_aics_def.h"

/**
 * @brief                       This function is used to set audio input gain setting.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] channel           is the audio channel.
 * @param[in] gain              is the gain setting.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_aics_set_audio_input_gain_setting(bt_handle_t handle, uint8_t channel, int8_t gain);

/**
 * @brief                       This function is used to set audio input to mute.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] channel           is the audio channel.
 * @param[in] mute              is the mute state.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_aics_set_audio_input_mute(bt_handle_t handle, uint8_t channel, ble_aics_mute_state_t mute);

/**
 * @brief                       This function is used to set audio input gain mode.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] channel           is the audio channel.
 * @param[in] gain_mode         is the gain mode.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_aics_set_audio_input_gain_mode(bt_handle_t handle, uint8_t channel, ble_aics_gain_mode_t gain_mode);

/**
 * @brief                       This function is used to set audio input status.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] channel           is the audio channel.
 * @param[in] input_status      is the input status.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_aics_set_audio_input_status(bt_handle_t handle, uint8_t channel, ble_aics_audio_input_status_t input_status);

/**
 * @brief                       This function is used to set audio input description.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] channel           is the audio channel.
 * @param[in] input_description is the input description string.
 * @param[in] length            is the length of input description string.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_aics_set_audio_input_description(bt_handle_t handle, uint8_t channel, uint8_t *input_description, uint8_t length);

/**
 * @brief                       This function is used to set audio input description by channel.
 * @param[in] channel           is the audio channel.
 * @param[in] input_description is the input description string.
 * @param[in] length            is the length of input description string.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_aics_set_audio_input_description_by_channel(uint8_t channel, uint8_t *input_description, uint8_t length);

#endif  /* __BLE_AICS_H__ */

