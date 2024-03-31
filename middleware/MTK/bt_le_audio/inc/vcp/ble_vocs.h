/*
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

#ifndef __BLE_VOCS_H__
#define __BLE_VOCS_H__

#include "bt_type.h"

/**
 * @brief                           This function is used to set volume offset.
 * @param[in] handle                is the connection handle of the Bluetooth link.
 * @param[in] channel               is the audio channel.
 * @param[in] offset                is the volume offset value.
 * @return                          #BT_STATUS_SUCCESS, the operation completed successfully.
 *                                  #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vocs_set_volume_offset(bt_handle_t handle, uint8_t channel, int16_t offset);

/**
 * @brief                           This function is used to set audio output description.
 * @param[in] handle                is the connection handle of the Bluetooth link.
 * @param[in] channel               is the audio channel.
 * @param[in] output_description    is the output description string.
 * @param[in] length                is the length of input description string.
 * @return                          #BT_STATUS_SUCCESS, the operation completed successfully.
 *                                  #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vocs_set_audio_output_description(bt_handle_t handle, uint8_t channel, uint8_t *output_description, uint8_t length);

/**
 * @brief                           This function is used to set audio output description by channel.
 * @param[in] channel               is the audio channel.
 * @param[in] output_description    is the input description string.
 * @param[in] length                is the length of input description string.
 * @return                          #BT_STATUS_SUCCESS, the operation completed successfully.
 *                                  #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vocs_set_audio_output_description_by_channel(uint8_t channel, uint8_t *output_description, uint8_t length);

#endif  /* __BLE_VOCS_H__ */

