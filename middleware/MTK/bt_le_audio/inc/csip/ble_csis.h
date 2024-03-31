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

#ifndef __BLE_CSIS_H__
#define __BLE_CSIS_H__

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothLeaudio LE_AUDIO
 * @{
 * @addtogroup BluetoothLeAudioCSIS CSIS
 * @{
 * This section introduces the CSIS operation codes, request and response structures, API prototypes.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b CSIS                       | Coordinated Set Identification Service. |
 * |\b SIRK                       | Set Identity Resolving Key. |
 * |\b RSI                        | Random Set Identifier. |
 *
 * @section ble_csis_api_usage How to use this module
 *   - LE Audio sink device can decide set size and SIRK by #ble_csis_set_coordinated_set_size() and #ble_csis_set_sirk().
 *    - Sample code:
 *     @code
 *          #define BLE_CSIS_DEFAULT_SIZE    2
 *
 *          bt_key_t sirk = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x19, 0x28, 0x55, 0x33, 0x68, 0x33, 0x56, 0xde};
 *
 *          void ble_csis_init_parameter(void)
 *          {
 *              ble_csis_set_coordinated_set_size(BLE_CSIS_DEFAULT_SIZE);
 *              ble_csis_set_sirk(sirk);
 *          }
 *     @endcode
 *   - Since LE Audio sink device needs to advertise with PSRI data, this sample demonstrates how to get PSRI data by #ble_csis_get_psri().
 *    - Sample code:
 *     @code
 *          static uint32_t app_le_audio_get_adv_data(multi_ble_adv_info_t *adv_data)
 *          {
 *              if ((NULL != adv_data->adv_data) && (NULL != adv_data->adv_data->data)) {
 *                  uint16_t sink_conent, source_conent;
 *                  uint8_t rsi[6];
 *                  uint8_t len = 0;
 *
 *                  adv_data->adv_data->data[len] = 7;
 *                  adv_data->adv_data->data[len + 1] = 0xF0; // AD Type RSI
 *                  ble_csis_get_rsi(rsi);
 *                  memcpy(&adv_data->adv_data->data[len + 2], psri, sizeof(rsi));
 *                  len += 8;
 *
 *                  adv_data->adv_data->data_length = len;
 *              }
 *
 *              return 0;
 *          }
 *     @endcode
 */

#include "bt_type.h"

/**
 * @brief                       This function calculates the RSI.
 * @param[out] psri             is the RSI value.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csis_get_rsi(uint8_t *psri);

/**
 * @brief                       This function sets the SIRK value in CSIS.
 * @param[in] sirk              is the SIRK value.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csis_set_sirk(bt_key_t sirk);


/**
 * @brief                       This function initializes the coordinated set size.
 * @param[in] size              is the size of coordinated set.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csis_set_coordinated_set_size(uint8_t size);

/**
 * @brief                       This function initializes the coordinated set member rank.
 * @param[in] rank              is the member rank of coordinated set.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csis_set_coordinated_set_rank(uint8_t rank);

/**
 * @brief                       This function configures CSIS to expose the sirk in encrypted or clear form.
 * @param[in] encrypted_sirk    is the type of exposed sirk.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csis_config_encrypted_sirk(bool encrypted_sirk);

//MTK_COMMON: keep this declaration for fixing app build error
/**
 * @brief                       This function write the SIRK value in CSIS to NVDM.
 * @param[in] sirk              is the value of sirk.
 */
void ble_csis_write_nvkey_sirk(bt_key_t *sirk);

/**
 * @}
 * @}
 * @}
 */

#endif  /* __BLE_CSIS_H__ */

