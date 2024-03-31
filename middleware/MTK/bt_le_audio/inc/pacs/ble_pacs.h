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


#ifndef __BLE_PACS_H__
#define __BLE_PACS_H__

#include "bt_le_audio_util.h"
#include "ble_pacs_def.h"

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothLeaudio LE_AUDIO
 * @{
 * @addtogroup BluetoothLeAudioPACS PACS
 * @{
 * This section introduces the PACS operation codes, request and response structures, API prototypes.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b PACS                       | Published Audio Capabilities Service. |
 * |\b PAC record                 | A set of parameter values that denote server audio capabilities. |
 */

/**
 *  @brief This structure defines the PAC record detail.
 */
typedef struct {
    uint8_t codec_id[AUDIO_CODEC_ID_SIZE];              /**< The codec id. */
    uint8_t codec_specific_capabilities_length;         /**< Length of the Codec_Specific_capabilities value for this PAC record. */
    uint8_t *codec_specific_capabilities;               /**< Codec_Specific_capabilities value for this PAC record. */
    uint8_t metadata_length;                            /**< Length of the Metadata parameter for this PAC record. */
    uint8_t *metadata;                                  /**< Metadata applicable to this PAC record. */
} PACKED ble_pacs_pac_record_t;

/**
 *  @brief This structure defines the PAC characteristic detail.
 */
typedef struct {
    uint8_t num_of_record;                              /**< Number of PAC records in this characteristic. */
    ble_pacs_pac_record_t *record;                       /**< PAC records. */
} PACKED ble_pacs_pac_t;

/**
 * @}
 */


BT_EXTERN_C_BEGIN

/**
 * @brief                   This function sets audio location.
 * @param[in] direction     is Sink or Source.
 * @param[in] location      is Device-wide bitmap of supported Audio Location values for all PAC records where the server supports reception of audio data.
 * @return                  #BT_STATUS_SUCCESS, the operation completed successfully.
 *                          #BT_STATUS_FAIL, the operation has failed.
 */
uint32_t ble_pacs_get_audio_location(bt_le_audio_direction_t direction);


/**
 * @brief                   This function is used to get Available_Audio_Contexts.
 * @param[in] sink          is bitmask of audio data Context Type values available for reception.
 * @param[in] source        is bitmask of audio data Context Type values available for transmission.
 * @return                  #BT_STATUS_SUCCESS, the operation completed successfully.
 *                          #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_pacs_get_available_audio_contexts(bt_le_audio_content_type_t *sink, bt_le_audio_content_type_t *source);

/**
 * @brief                   This function sets audio location.
 * @param[in] direction     is Sink or Source.
 * @param[in] location      is Device-wide bitmap of supported Audio Location values for all PAC records where the server supports reception of audio data.
 * @return                  #BT_STATUS_SUCCESS, the operation completed successfully.
 *                          #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_pacs_set_audio_location(bt_le_audio_direction_t direction, uint32_t location);

/**
 * @brief                   This function sets Available_Audio_Contexts.
 * @param[in] sink          is bitmask of audio data Context Type values available for reception.
 * @param[in] source        is bitmask of audio data Context Type values available for transmission.
 * @return                  #BT_STATUS_SUCCESS, the operation completed successfully.
 *                          #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_pacs_set_available_audio_contexts(bt_le_audio_content_type_t sink, bt_le_audio_content_type_t source);

/**
 * @brief                   This function sets Supported_Audio_Contexts.
 * @param[in] sink          is bitmask of audio data Context Type values supported for reception.
 * @param[in] source        is bitmask of audio data Context Type values supported for transmission.
 * @return                  #BT_STATUS_SUCCESS, the operation completed successfully.
 *                          #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_pacs_set_supported_audio_contexts(bt_le_audio_content_type_t sink, bt_le_audio_content_type_t source);

/**
 * @brief                   This function sets PAC record.
 * @param[in] direction     is Sink or Source.
 * @param[in] charc_idx     is characteristic index correspond to each PAC characteristic.
 * @param[in] p_data        is PAC characteristic data.
 * @return                  #BT_STATUS_SUCCESS, the operation completed successfully.
 *                          #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_pacs_set_pac(bt_le_audio_direction_t direction, uint8_t charc_idx, ble_pacs_pac_t *p_data);

/**
 * @brief                   This function send sink PAC record notification to remote device.
 * @param[in] handle        is BLE connection handle.
 * @param[in] charc_idx     is characteristic index correspond to each sink PAC characteristic.
 * @return                  #true, the operation completed successfully.
 *                          #false, the operation has failed.
 */
bool ble_pacs_send_sink_pac_notify(bt_handle_t handle, uint8_t charc_idx);

/**
 * @brief                   This function send sink location notification to remote device.
 * @param[in] handle        is BLE connection handle.
 * @return                  #true, the operation completed successfully.
 *                          #false, the operation has failed.
 */
bool ble_pacs_send_sink_location_notify(bt_handle_t handle);

/**
 * @brief                   This function send source PAC record notification to remote device.
 * @param[in] handle        is BLE connection handle.
 * @param[in] charc_idx     is characteristic index correspond to each source PAC characteristic.
 * @return                  #true, the operation completed successfully.
 *                          #false, the operation has failed.
 */
bool ble_pacs_send_source_pac_notify(bt_handle_t handle, uint8_t charc_idx);

/**
 * @brief                   This function send source location notification to remote device.
 * @param[in] handle        is BLE connection handle.
 * @return                  #true, the operation completed successfully.
 *                          #false, the operation has failed.
 */
bool ble_pacs_send_source_location_notify(bt_handle_t handle);

/**
 * @brief                   This function send available audio contexts notification to remote device.
 * @param[in] handle        is BLE connection handle.
 * @return                  #true, the operation completed successfully.
 *                          #false, the operation has failed.
 */
bool ble_pacs_send_available_audio_contexts_notify(bt_handle_t handle);

/**
 * @brief                   This function send supported audio contexts notification to remote device.
 * @param[in] handle        is BLE connection handle.
 * @return                  #true, the operation completed successfully.
 *                          #false, the operation has failed.
 */
bool ble_pacs_send_supported_audio_contexts_notify(bt_handle_t handle);

/**
 * @brief                   This function sets availability of PAC record.
 * @param[in] direction     is Sink or Source.
 * @param[in] available     is available or unavailable.
 * @return                  #BT_STATUS_SUCCESS, the operation completed successfully.
 *                          #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_pacs_set_pac_record_availability(bt_le_audio_direction_t direction, bool available);

bool ble_pacs_set_cccd_with_att_handle(bt_handle_t handle, uint16_t attr_handle, uint16_t value);
bt_le_audio_cccd_record_t* ble_pacs_get_cccd_with_att_handle(bt_handle_t handle, uint32_t *num);

BT_EXTERN_C_END
/**
 * @}
 * @}
 * @}
 */

#endif  /* __BLE_PACS_H__ */

