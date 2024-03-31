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

#ifndef __BLE_MCS_H__
#define __BLE_MCS_H__

#include "ble_mcs_def.h"

/**
 *  @brief Define events of MCS and GMCS.
 */
#define BLE_MCS_EVENT_PLAYING                  0x0001  /**< Media playing indication with #ble_mcs_event_parameter_t as the payload. */
#define BLE_MCS_EVENT_PAUSED                   0x0002  /**< Media paused indication with #ble_mcs_event_parameter_t as the payload. */
#define BLE_MCS_EVENT_PREVIOUS_TRACK           0x0003  /**< Media previous track indication with #ble_mcs_event_parameter_t as the payload. */
#define BLE_MCS_EVENT_NEXT_TRACK               0x0004  /**< Media next track indication with #ble_mcs_event_parameter_t as the payload. */
typedef uint16_t ble_mcs_event_t;                      /**< The type of the MCS events. */

/**
 *  @brief This structure defines the parameter data type for events.
 */
typedef struct {
    uint8_t service_idx;    /**< Service index. */
} ble_mcs_event_parameter_t;

/**
 * @brief     This function start playing media.
 * @param[in] service_idx       is the service index of MCS.
 * @return    #BT_STATUS_SUCCESS, the operation completed successfully.
 *            #BT_STATUS_FAIL, an error occurred, the operation has terminated.
 */
bt_status_t ble_mcs_play_media(uint8_t service_idx);

/**
 * @brief     This function stop playing media.
 * @param[in] service_idx       is the service index of MCS.
 * @return    #BT_STATUS_SUCCESS, the operation completed successfully.
 *            #BT_STATUS_FAIL, an error occurred, the operation has terminated.
 */
bt_status_t ble_mcs_stop_media(uint8_t service_idx);

/**
 * @brief     This function pause media.
 * @param[in] service_idx       is the service index of MCS.
 * @return    #BT_STATUS_SUCCESS, the operation completed successfully.
 *            #BT_STATUS_FAIL, an error occurred, the operation has terminated.
 */
bt_status_t ble_mcs_pause_media(uint8_t service_idx);

/**
 * @brief     This function start playing next track.
 * @param[in] service_idx       is the service index of MCS.
 * @return    #BT_STATUS_SUCCESS, the operation completed successfully.
 *            #BT_STATUS_FAIL, an error occurred, the operation has terminated.
 */
bt_status_t ble_mcs_next_track(uint8_t service_idx);

/**
 * @brief     This function start playing previous track.
 * @param[in] service_idx       is the service index of MCS.
 * @return    #BT_STATUS_SUCCESS, the operation completed successfully.
 *            #BT_STATUS_FAIL, an error occurred, the operation has terminated.
 */
bt_status_t ble_mcs_prev_track(uint8_t service_idx);

/**
 * @brief     This function set the media state.
 * @param[in] service_idx       is the service index of MCS.
 * @param[in] state       is the media state of MCS.
 */

void ble_mcs_set_media_state(uint8_t service_idx, ble_mcs_media_state_t state);

/**
 * @brief     This function get the media state.
 * @param[in] service_idx       is the service index of MCS.
 * @return    is the media state.
 */
ble_mcs_media_state_t ble_mcs_get_media_state(uint8_t service_idx);

/**
 * @brief     This function set the track duration.
 * @param[in] service_idx       is the service index of MCS.
 * @param[in] duration       is the track duration of MCS.
 */

void ble_mcs_set_track_duration(uint8_t service_idx, int32_t duration);

/**
 * @brief     This function get the track duration.
 * @param[in] service_idx       is the service index of MCS.
 * @return    is the track duration.
 */

uint32_t ble_mcs_get_track_duration(uint8_t service_idx);

/**
 * @brief     This function set the track position.
 * @param[in] service_idx       is the service index of MCS.
 * @param[in] position       is the track position of MCS.
 */

void ble_mcs_set_track_position(uint8_t service_idx, int32_t position);

/**
 * @brief     This function get the track position.
 * @param[in] service_idx       is the service index of MCS.
 * @return    is the track position.
 */

uint32_t ble_mcs_get_track_position(uint8_t service_idx);

/**
 * @brief     This function get the GMCS service index.
 * @return    is the service index of GMCS.
 */
uint8_t ble_mcs_get_gmcs_service_idx(void);

#endif  /* __BLE_MCS_H__ */
