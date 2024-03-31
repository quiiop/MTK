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

#ifndef __BLE_VCP_H__
#define __BLE_VCP_H__

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothVCP VCP
 * @{
 * This section introduces the VCP operation codes, request and response structures, API prototypes.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b VCP                        | Volume Control Profile. This profile enables a device to adjust the volume of audio devices that expose the Volume Control Service. |
 *
 */

#include "bt_type.h"

/**
 * @brief                       This function triggers volume up action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vcp_volume_up(bt_handle_t handle);

/**
 * @brief                       This function triggers volume down action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vcp_volume_down(bt_handle_t handle);

/**
 * @brief                       This function triggers unmute and volume up action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vcp_unmute_volume_up(bt_handle_t handle);

/**
 * @brief                       This function triggers unmute and volume up action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vcp_unmute_volume_down(bt_handle_t handle);

/**
 * @brief                       This function sets a absolute volume value.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] value             is the value of volume, valid range 0~255.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vcp_set_volume(bt_handle_t handle, uint8_t volume);

/**
 * @brief                       This function triggers unmute action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vcp_unmute(bt_handle_t handle);

/**
 * @brief                       This function triggers mute action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vcp_mute(bt_handle_t handle);

/**
 * @brief                       This function gets current valume value.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
uint8_t ble_vcp_get_current_volume(void);

/**
 * @brief                       This function initialize the default volume.
 * @param[in] volume            is the default volume value.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vcp_set_default_volume(uint8_t volume);

/**
 * @}
 * @}
 */

#endif  /* __BLE_VCP_H__ */
