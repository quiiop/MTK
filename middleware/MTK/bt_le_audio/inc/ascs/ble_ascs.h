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


#ifndef __BLE_ASCS_H__
#define __BLE_ASCS_H__

#include "bt_type.h"
#include "bt_le_audio_def.h"
#include "bt_le_audio_util.h"
#include "ble_ascs_def.h"

/**
 * @brief                           This function used to get specified type of configuration in codec specific configuration.
 * @param[in] type                  is the specified type for the codec specific configuration.
 * @param[in] configuration_len     is the length of codec specific configuration.
 * @param[in] configuration_len     is the codec specific configuration.
 * @return                          the address point to specified type.
 *
 */
uint8_t *ble_ascs_get_ltv_value_from_codec_specific_configuration(uint8_t type, uint8_t configuration_len, uint8_t *configuration);

/**
 * @brief                           This function used to get metadata type.
 * @param[in] type                  is the metadata type.
 * @param[in] metadata_len          is the length of metadata.
 * @param[in] metadata              is the codec metadata.
 * @return                          the address point to metadata value.
 *
 */
uint8_t *ble_ascs_get_ltv_value_from_metadata(uint8_t type, uint8_t metadata_len, uint8_t *metadata);

/**
 * @brief                           This function used to get ASCS qos preference parameters.
 * @param[in] target_latency        is the target latency.
 * @param[in] configuration_len     is the frame duration.
 * @param[in] configuration_len     is the configed octets per codec frame.
 * @return                          the pointer of qos preference.
 *
 */
ble_ascs_default_qos_parameters_t *ble_ascs_get_qos_preference(uint8_t target_latency, uint8_t frame_duration, uint16_t configed_octets_per_codec_frame);

/**
 * @brief                           This function used switch Sink/Source ASE availability.
 * @param[in] direction             is Sink or Source.
 * @param[in] available             is available or not.
 * @return                          #BT_STATUS_SUCCESS, the operation completed successfully.
 *                                  #BT_STATUS_FAIL, the operation has failed.
 *
 */
bt_status_t ble_ascs_set_ase_availability(bt_le_audio_direction_t direction, bool available);

bt_le_audio_cccd_record_t* ble_ascs_get_cccd(bt_handle_t handle, uint32_t *num);
bool ble_ascs_set_cccd(bt_handle_t handle, uint16_t attr_handle, uint16_t value);

#if 1 //MTK_COMMON
/**
 * @brief                           This function used switch Sink/Source ASE availability.
 * @param[in] handle                Connection handle.
 * @return                          Number of ASEs set in codec configuration.
 *
 */
uint8_t ble_ascs_get_num_of_ase(bt_handle_t handle);
#endif

#endif  /* __BLE_ASCS_H__ */

