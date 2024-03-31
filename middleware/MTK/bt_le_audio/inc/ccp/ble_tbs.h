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

#ifndef __BLE_TBS_H__
#define __BLE_TBS_H__

#include "ble_tbs_def.h"

/**
 *  @brief Define events of TBS and GTBS.
 */
#define BLE_TBS_EVENT_INCOMING_CALL     0x0001          /**< The incoming call event with #ble_tbs_event_incoming_call_t as the payload. */
#define BLE_TBS_EVENT_DIALING           0x0002          /**< The dialing Call (Outgoing call) event with #ble_tbs_event_dialing_t as the payload. */
#define BLE_TBS_EVENT_ALERTING          0x0003          /**< The alerting (Outgoing call) event with #ble_tbs_event_parameter_t as the payload. */
#define BLE_TBS_EVENT_ACTIVE            0x0004          /**< The call active event with #ble_tbs_event_parameter_t as the payload. */
#define BLE_TBS_EVENT_CALL_ENDED        0x0005          /**< The call ended event with #ble_tbs_event_parameter_t as the payload. */
#define BLE_TBS_EVENT_CALL_HELD         0x0006          /**< The call held event with #ble_tbs_event_parameter_t as the payload. */
#define BLE_TBS_EVENT_ACCEPT_CALL       0x0010          /**< The accept call event with #ble_tbs_event_parameter_t as the payload. */
#define BLE_TBS_EVENT_TERMINATE_CALL    0x0011          /**< The terminate call event with #ble_tbs_event_parameter_t as the payload. */
#define BLE_TBS_EVENT_HOLD_CALL         0x0012          /**< The hold call event with #ble_tbs_event_parameter_t as the payload. */
#define BLE_TBS_EVENT_UNHOLD_CALL       0x0013          /**< The unhold call event with #ble_tbs_event_parameter_t as the payload. */
typedef uint8_t ble_tbs_event_t;                        /**< The type of the TBS events. */

/**
 *  @brief This structure defines the parameter data type for event #BLE_TBS_EVENT_INCOMING_CALL and #BLE_TBS_EVENT_DIALING.
 */
typedef struct {
    uint8_t service_idx;    /**< Service index. */
    uint8_t call_idx;       /**< Call index. */
    uint8_t uri_len;        /**< The length of URI. */
    uint8_t *uri;           /**< URI. */
    uint8_t name_len;        /**< The length of call friendly name. */
    uint8_t *name;           /**< call friendly name. */
} ble_tbs_event_incoming_call_t, ble_tbs_event_dialing_t;

/**
 *  @brief This structure defines the parameter data type for events.
 */
typedef struct {
    uint8_t service_idx;    /**< Service index. */
    uint8_t call_idx;       /**< Call index. */
} ble_tbs_event_parameter_t;

/**
 * @brief                       This function get the GTBS service index.
 * @return                      is the service index of GTBS.
 */
uint8_t ble_tbs_get_gtbs_service_idx(void);
/**
 * @brief                       This function is ued to judge the connection handle whether is an active device.
 * @param[in] handle            is the connection handle.
 * @return                      is an active device.
 */
bool ble_tbs_is_active_group_by_handle(bt_handle_t handle);


#endif  /* __BLE_TBS_H__ */

