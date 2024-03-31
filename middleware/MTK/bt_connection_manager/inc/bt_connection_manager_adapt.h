/* Copyright Statement:
 *
 * (C) 2005-2021  MediaTek Inc. All rights reserved.
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

#ifndef __BT_CONNECTION_MANAGER_ADAPT_H__
#define __BT_CONNECTION_MANAGER_ADAPT_H__

#include "bt_type.h"
#include "bt_gap.h"
#include "bt_custom_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

#define BT_SINK_SRV_CM_MAX_DEVICE_NUMBER            3
#define BT_SINK_SRV_CM_MAX_TRUSTED_DEV              BT_DEVICE_MANAGER_MAX_PAIR_NUM
#define BT_SINK_SRV_CM_REASON_CONNECTION_TIMEOUT    0x08
#define BT_SINK_SRV_CM_REASON_ROLE_SWITCH_PENDING   0x32
#define BT_SINK_SRV_CM_REASON_LMP_RESPONSE_TIMEOUT  0x22
#define BT_SINK_SRV_CM_END_AIR_PAIRING_TIMER_DUR    (5000)

//typedef uint8_t bt_cm_power_state_t;

/**
 *  @brief Define for the profile type.
 */
#define BT_SINK_SRV_PROFILE_NONE           (0x00)  /**< Profile type: None. */
#define BT_SINK_SRV_PROFILE_HFP            (0x01)  /**< Profile type: Hands-free Profile(HFP). */
#define BT_SINK_SRV_PROFILE_A2DP_SINK      (0x02)  /**< Profile type: Advanced Audio Distribution Profile(A2DP) as sink. */
#define BT_SINK_SRV_PROFILE_AVRCP          (0x04)  /**< Profile type: Audio/Video Remote Control Profile(AVRCP). */
#define BT_SINK_SRV_PROFILE_PBAPC          (0x08)  /**< Profile type: Audio/Video Remote Control Profile(PBAPC). */
#define BT_SINK_SRV_PROFILE_HSP            (0x10)  /**< Profile type: Advanced Audio Distribution Profile(A2DP)  as source. */
typedef uint8_t bt_sink_srv_profile_type_t;     /**<The feature configuration of sink service. */

/**
 *  @brief Define for the profile connection state.
 */
#define BT_SINK_SRV_PROFILE_CONNECTION_STATE_DISCONNECTED  (0x00)
#define BT_SINK_SRV_PROFILE_CONNECTION_STATE_CONNECTING    (0x01)
#define BT_SINK_SRV_PROFILE_CONNECTION_STATE_CONNECTED     (0x02)
#define BT_SINK_SRV_PROFILE_CONNECTION_STATE_DISCONNECTING (0x03)
typedef uint8_t bt_sink_srv_profile_connection_state_t;    /**<The profile connection state. */

/**
 * @brief Define the module for the connection manager.
 */
#define BT_CONNECTION_MANAGER_MODULE_OFFSET   16            /**< Module range: 0xF8200000 ~ 0xF82F0000. The maximum number of modules: 16. */
#define BT_CONNECTION_MANAGER_MODULE_MASK     0x0000FFFFU   /**< Mask for Bluetooth custom module. 0x0000FFFFU */

#define BT_CONNECTION_MANAGER_STATUS    (0x00U << BT_CONNECTION_MANAGER_MODULE_OFFSET) /**< Prefix of the Status.  0xF8200000*/
#define BT_CONNECTION_MANAGER_EVENT     (0x02U << BT_CONNECTION_MANAGER_MODULE_OFFSET) /**< Prefix of the event.  0xF8220000*/
#define BT_CONNECTION_MANAGER_ACTION    (0x01U << BT_CONNECTION_MANAGER_MODULE_OFFSET) /**< Prefix of the action.  0xF8210000*/

#define BT_SINK_SRV_EVENT_PROFILE_CONNECTION_UPDATE         (BT_CONNECTION_MANAGER_EVENT | 0x0000U)  /**< This event indicates the profile connection information of current link. 0xF82200000*/
#define BT_SINK_SRV_EVENT_CONNECTION_INFO_UPDATE            (BT_CONNECTION_MANAGER_EVENT | 0x0002U)  /**< This event indicates the connection information of current link. 0xF82200002*/
typedef uint32_t bt_connection_manager_event_t;

#define BT_CONNECTION_MANAGER_STATUS_STATE_ALREADY_EXIST    (BT_CONNECTION_MANAGER_STATUS | 0x000CU)  /**< The connection manager status: the opreation state is exist already. 0xF8210000C*/

/**
 *  @brief This structure is the callback parameters type of event(#BT_SINK_SRV_EVENT_PROFILE_CONNECTION_UPDATE) which indicates connection status and information.
 */
typedef struct {
    bt_bd_addr_t address;                           /**<  The remote device address */
    bt_sink_srv_profile_type_t profile;             /**< Profile type*/
    bt_sink_srv_profile_connection_state_t state;   /**<Profile connection state */
    bt_status_t reason;                             /**<The disconnect or connect fail reason */
} bt_sink_srv_profile_connection_state_update_t;

/**
 *  @brief This structure is the parameters of #BT_SINK_SRV_ACTION_PROFILE_CONNECT or #BT_SINK_SRV_ACTION_PROFILE_DISCONNECT
 *            which indicates the profile to create or disconnect connection.
 */
typedef struct {
    bt_bd_addr_t                address;                    /**<  The remote device address. */
    bt_sink_srv_profile_type_t  profile_connection_mask;    /**<  The mask indicates which profile the device will connect. */
} bt_sink_srv_profile_connection_action_t;

/**
 *  @brief This structure is the callback parameters type of event(#BT_SINK_SRV_EVENT_CONNECTION_INFO_UPDATE) which indicates connection status and information.
 */
typedef struct {
    bt_bd_addr_t               bt_addr;       /**<  The remote device address. */
    bt_sink_srv_profile_type_t profile_type;  /**<  The connected profile type of this device. */
} bt_sink_srv_connection_information_t;

#define BT_SINK_SRV_HFP_STORAGE_SIZE    (32)
#define BT_SINK_SRV_A2DP_STORAGE_SIZE   (32)
#define BT_SINK_SRV_PBAP_STORAGE_SIZE   (32)

bt_bd_addr_t    *bt_sink_srv_cm_last_connected_device(void);
uint32_t        bt_sink_srv_cm_get_connected_device(bt_sink_srv_profile_type_t profile, bt_bd_addr_t addr_list[BT_SINK_SRV_CM_MAX_DEVICE_NUMBER]);
uint32_t        bt_sink_srv_cm_get_connected_device_list(bt_sink_srv_profile_type_t profile, bt_bd_addr_t *addr_list, uint32_t list_num);
bt_sink_srv_profile_type_t bt_sink_srv_cm_get_connected_profiles(bt_bd_addr_t *address);
bt_gap_connection_handle_t bt_sink_srv_cm_get_gap_handle(bt_bd_addr_t *address_p);
void            bt_sink_srv_cm_profile_status_notify(bt_bd_addr_t *addr, bt_sink_srv_profile_type_t profile, bt_sink_srv_profile_connection_state_t state, bt_status_t reason);
void            bt_connection_manager_device_local_info_store_local_address(bt_bd_addr_t *addr);
bt_bd_addr_t   *bt_connection_manager_device_local_info_get_local_address(void);
void            bt_connection_manager_write_scan_enable_mode(bt_gap_scan_mode_t mode);
//bt_cm_power_state_t
//              bt_connection_manager_power_get_state();

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifndef __BT_CONNECTION_MANAGER_ADAPT_H__ */

