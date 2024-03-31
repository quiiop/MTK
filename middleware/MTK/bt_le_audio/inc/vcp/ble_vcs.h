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

#ifndef __BLE_VCS_H__
#define __BLE_VCS_H__

#include "ble_vcs_def.h"
#include "bt_le_audio_type.h"

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothLeaudio LE_AUDIO
 * @{
 * @addtogroup BluetoothLeAudioVCS VCS
 * @{
 * This section introduces the VCS server events, event payload structures and API prototypes.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b VCS                        | Volume Control Service. |
 *
 * @section ble_vcs_api_usage How to use this module
 *   - Implement the event callback to process the related VCS events.
 *   - Sample code:
 *     @code
 *          static ble_vcs_volume_t volume;            // The volume value of the VCS.
 *          static ble_vcs_mute_t mute;                // The mute state of the VCS.
 *          static ble_vcs_volume_flags_t flags;       // The volume flags of the VCS.
 *
 *          bt_status_t bt_sink_srv_le_volume_vcp_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
 *          {
 *              switch (msg) {
 *                  case BLE_VCS_SET_RELATIVE_VOLUME_DOWN_IND:
 *                      ble_vcs_set_relative_volume_down_ind_t *ind = (ble_vcs_set_relative_volume_down_ind_t *)buffer;
 *                      printf("[VCS] RELATIVE_VOLUME_DOWN_IND, handle:%x volume:%x mute:%x", ind->handle, volume, mute);
 *
 *                      // handle volume down
 *                      if (volume > 0) {
 *                          volume--;
 *                          // send volume state change notification with the volume value and the mute state
 *                          ble_vcs_send_volume_state_notification(ind->handle, volume, mute);
 *                      }
 *                      break;
 *                  }
 *
 *                  case BLE_VCS_SET_UNMUTE_RELATIVE_VOLUME_UP_IND: {
 *                      ble_vcs_set_unmute_relative_volume_up_ind_t *ind = (ble_vcs_set_unmute_relative_volume_up_ind_t *)buffer;
 *                      bool volume_state_change = false;
 *                      printf("[VCS] UNMUTE_RELATIVE_VOLUME_UP_IND, handle:%x volume:%x mute:%x", ind->handle, volume, mute);
 *
 *                      // handle volume up
 *                      if (volume < 0xFF) {
 *                          volume++;
 *                          volume_state_change = true;
 *                      }
 *                      if (BLE_VCS_MUTE == mute) {
 *                          mute = BLE_VCS_UNMUTE;
 *                          volume_state_change = true;
 *                      }
 *                      if (volume_state_change) {
 *                          // send volume state change notification with the volume value and the mute state
 *                          ble_vcs_send_volume_state_notification(ind->handle, volume, mute);
 *                      }
 *                      break;
 *                  }
 *
 *                  case BLE_VCS_SET_ABSOLUTE_VOLUME_IND: {
 *                      ble_vcs_set_absolute_volume_ind_t *ind = (ble_vcs_set_absolute_volume_ind_t *)buffer;
 *                      printf("[VCS] SET_ABSOLUTE_VOLUME_IND, handle:%x volume:%x mute:%x", ind->handle, volume, mute);
 *
 *                      // handle set absolute volume
 *                      if (volume != ind->volume) {
 *                          volume = ind->volume;
 *                          // send volume state change notification with the volume value and the mute state
 *                          ble_vcs_send_volume_state_notification(ind->handle, volume, mute);
 *                      }
 *                      break;
 *                  }
 *
 *                  case BLE_VCS_SET_UNMUTE_IND: {
 *                      ble_vcs_set_unmute_ind_t *ind = (ble_vcs_set_unmute_ind_t *)buffer;
 *                      printf("[VCS] SET_UNMUTE_IND, handle:%x volume:%x mute:%x", ind->handle, volume, mute);
 *
 *                      // handle set unmute
 *                      if (BLE_VCS_MUTE == mute) {
 *                          mute = BLE_VCS_UNMUTE;
 *                          // send volume state change notification with the volume value and the mute state
 *                          ble_vcs_send_volume_state_notification(ind->handle, volume, mute);
 *                      }
 *                      break;
 *                  }
 *
 *                  case BLE_VCS_SET_MUTE_IND: {
 *                      ble_vcs_set_mute_ind_t *ind = (ble_vcs_set_mute_ind_t *)buffer;
 *                      printf("[VCS] SET_MUTE_IND, handle:%x volume:%x mute:%x", ind->handle, volume, mute);
 *
 *                      // handle set mute
 *                      if (BLE_VCS_UNMUTE == mute) {
 *                          mute = BLE_VCS_MUTE;
 *                          // send volume state change notification with the volume value and the mute state
 *                          ble_vcs_send_volume_state_notification(ind->handle, volume, mute);
 *                      }
 *                      break;
 *                  }
 *
 *                  case BLE_VCS_READ_VOLUME_STATE_REQ: {
 *                      ble_vcs_read_volume_state_req_t *req = (ble_vcs_read_volume_state_req_t *)buffer;
 *                      printf("[VCS] READ_VOLUME_STATE_REQ, handle:%x msg:%x", handle, msg);
 *
 *                      // handle read volume state and send read volume state response with the volume value and the mute state
 *                      ble_vcs_send_volume_state_read_response(req->handle, 0, volume, mute);
 *                      break;
 *                  }
 *
 *                  case BLE_VCS_READ_VOLUME_FLAGS_REQ: {
 *                      ble_vcs_read_volume_flags_req_t *req = (ble_vcs_read_volume_flags_req_t *)buffer;
 *                      printf("[VCS] READ_VOLUME_FLAGS_REQ, handle:%x flags:%x", handle, flags);
 *
 *                      // handle read volume flags and send read volume flags response
 *                      ble_vcs_send_volume_flags_read_response(req->handle, 0, flags);
 *                      break;
 *                  }
 *
 *                  default:
 *                      break;
 *              }
 *
 *              return BT_STATUS_SUCCESS;
 *          }
 *
 *     @endcode
 */

/**
 * @defgroup Bluetoothble_VCS_define Define
 * @{
 * This section defines the VCS event IDs.
 */
/**
 *  @brief Defines the VCS event IDs for upper layer.
 */
#define BLE_VCS_SET_RELATIVE_VOLUME_DOWN_IND            (BT_LE_AUDIO_MODULE_VCP_VCS | 0x0000)   /**< This event indicates that VCS client attempt to set relative volume down with #ble_vcs_set_relative_volume_down_ind_t as payload. */
#define BLE_VCS_SET_RELATIVE_VOLUME_UP_IND              (BT_LE_AUDIO_MODULE_VCP_VCS | 0x0001)   /**< This event indicates that VCS client attempt to set relative volume up with #ble_vcs_set_relative_volume_up_ind_t as payload. */
#define BLE_VCS_SET_UNMUTE_RELATIVE_VOLUME_DOWN_IND     (BT_LE_AUDIO_MODULE_VCP_VCS | 0x0002)   /**< This event indicates that VCS client attempt to set unmute relative volume down with #ble_vcs_set_unmute_relative_volume_down_ind_t as payload. */
#define BLE_VCS_SET_UNMUTE_RELATIVE_VOLUME_UP_IND       (BT_LE_AUDIO_MODULE_VCP_VCS | 0x0003)   /**< This event indicates that VCS client attempt to set unmute relative volume up with #ble_vcs_set_unmute_relative_volume_up_ind_t as payload. */
#define BLE_VCS_SET_ABSOLUTE_VOLUME_IND                 (BT_LE_AUDIO_MODULE_VCP_VCS | 0x0004)   /**< This event indicates that VCS client attempt to set absolute volume with #ble_vcs_set_absolute_volume_ind_t as payload. */
#define BLE_VCS_SET_UNMUTE_IND                          (BT_LE_AUDIO_MODULE_VCP_VCS | 0x0005)   /**< This event indicates that VCS client attempt to set unmute with #ble_vcs_set_unmute_ind_t as payload. */
#define BLE_VCS_SET_MUTE_IND                            (BT_LE_AUDIO_MODULE_VCP_VCS | 0x0006)   /**< This event indicates that VCS client attempt to set mute with #ble_vcs_set_mute_ind_t as payload. */
#define BLE_VCS_READ_VOLUME_STATE_REQ                   (BT_LE_AUDIO_MODULE_VCP_VCS | 0x0007)   /**< This event indicates that VCS client request to read volume state with #ble_vcs_read_volume_state_req_t as payload. */
#define BLE_VCS_READ_VOLUME_FLAGS_REQ                   (BT_LE_AUDIO_MODULE_VCP_VCS | 0x0008)   /**< This event indicates that VCS client request to read volume flags with #ble_vcs_read_volume_flags_req_t as payload. */
/**
 * @}
 */

/**
 * @defgroup Bluetoothble_VCS_struct Struct
 * @{
 * This section defines basic data structures for the VCS.
 */
/**
 *  @brief This structure defines the parameter data type for event #BLE_VCS_SET_RELATIVE_VOLUME_DOWN_IND.
 */
typedef struct {
    bt_handle_t handle;     /**< Connection handle.*/
} ble_vcs_set_relative_volume_down_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_VCS_SET_RELATIVE_VOLUME_UP_IND.
 */
typedef ble_vcs_set_relative_volume_down_ind_t ble_vcs_set_relative_volume_up_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_VCS_SET_UNMUTE_RELATIVE_VOLUME_DOWN_IND.
 */
typedef ble_vcs_set_relative_volume_down_ind_t ble_vcs_set_unmute_relative_volume_down_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_VCS_SET_UNMUTE_RELATIVE_VOLUME_UP_IND.
 */
typedef ble_vcs_set_relative_volume_down_ind_t ble_vcs_set_unmute_relative_volume_up_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_VCS_SET_ABSOLUTE_VOLUME_IND.
 */
typedef struct {
    bt_handle_t handle;         /**< Connection handle.*/
    ble_vcs_volume_t volume;    /**< Volume.*/
} ble_vcs_set_absolute_volume_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_VCS_SET_UNMUTE_IND.
 */
typedef ble_vcs_set_relative_volume_down_ind_t ble_vcs_set_unmute_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_VCS_SET_MUTE_IND.
 */
typedef ble_vcs_set_relative_volume_down_ind_t ble_vcs_set_mute_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_VCS_READ_VOLUME_STATE_REQ.
 */
typedef ble_vcs_set_relative_volume_down_ind_t ble_vcs_read_volume_state_req_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_VCS_READ_VOLUME_FLAGS_REQ.
 */
typedef ble_vcs_set_relative_volume_down_ind_t ble_vcs_read_volume_flags_req_t;
/**
 * @}
 */

/**
 * @brief                       This function send volume state notification to VCS client.
 * @param[in] handle            is the connection handle of the LE link.
 * @param[in] volume            is the volume value.
 * @param[in] mute              is the mute state refer to #ble_vcs_mute_t.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_UNSUPPORTED, the operation is unsupported due to the VCS client does not configure to receive notification.
 */
bt_status_t ble_vcs_send_volume_state_notification(bt_handle_t handle, ble_vcs_volume_t volume, ble_vcs_mute_t mute);

/**
 * @brief                       This function send volume flags notification to VCS client.
 * @param[in] handle            is the connection handle of the LE link.
 * @param[in] volume_flags      is the volume flags refer to #ble_vcs_volume_flags_t.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_UNSUPPORTED, the operation is not supported because of the VCS client has not configured to receive notifications from the VCS server.
 */
bt_status_t ble_vcs_send_volume_flags_notification(bt_handle_t handle, ble_vcs_volume_flags_t volume_flags);


/**
 * @brief                       This function send read volume state response to VCS client.
 * @param[in] handle            is the connection handle of the LE link.
 * @param[in] result            is the result of reading the volume state.
 * @param[in] volume            is the volume value.
 * @param[in] mute              is the mute state refer to #ble_vcs_mute_t.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vcs_send_volume_state_read_response(bt_handle_t handle, uint8_t result, ble_vcs_volume_t volume, ble_vcs_mute_t mute);

/**
 * @brief                       This function send read volume flags response to VCS client.
 * @param[in] handle            is the connection handle of the LE link.
 * @param[in] result            is the result of reading the volume flags.
 * @param[in] volume_flags      is the volume flags refer to #ble_vcs_volume_flags_t.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_vcs_send_volume_flags_read_response(bt_handle_t handle, uint8_t result, ble_vcs_volume_flags_t volume_flags);

/**
 * @brief                       This function send volume state notification to all VCS clients.
 * @param[in] volume            is the volume value.
 * @param[in] mute              is the mute state refer to #ble_vcs_mute_t.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_UNSUPPORTED, the operation is unsupported due to the VCS client does not configure to receive notification.
 */
bt_status_t ble_vcs_send_all_volume_state_notification(ble_vcs_volume_t volume, ble_vcs_mute_t mute);

/**
 * @}
 * @}
 * @}
 */

#endif  /* __BLE_VCS_H__ */

