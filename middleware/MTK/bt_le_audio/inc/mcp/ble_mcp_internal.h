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


#ifndef __BLE_MCP_INTERNAL_H__
#define __BLE_MCP_INTERNAL_H__

#include "ble_mcp.h"
#include "ble_mcp_discovery.h"
//#define BT_OTP
#define BLE_MCP_OBJECT_SIZE                      (6)

/**
 *  @brief Defines the MCP actions.
 */
#define BLE_MCP_ACTION_READ_MEDIA_ICON_OBJECT_ID                            (0x00000102)    /**< This action sends a request to read the Media Player Icon Object ID. */
#define BLE_MCP_ACTION_READ_CURRENT_TRACK_SEGMENTS_OBJECT_ID                (0x00000109)    /**< This action sends a request to read the Track Segments Object ID. */
#define BLE_MCP_ACTION_READ_CURRENT_TRACK_OBJECT_ID                         (0x0000010A)    /**< This action sends a request to read the Current Track Object ID. */
#define BLE_MCP_ACTION_READ_NEXT_TRACK_OBJECT_ID                            (0x0000010B)    /**< This action sends a request to read the Next Track Object ID. */
#define BLE_MCP_ACTION_READ_PARENT_GROUP_OBJECT_ID                          (0x0000010C)    /**< This action sends a request to read the Parent Group Object ID. */
#define BLE_MCP_ACTION_READ_CURRENT_GROUP_OBJECT_ID                         (0x0000010D)    /**< This action sends a request to read the Current Group Object ID. */
#define BLE_MCP_ACTION_READ_SEARCH_RESULTS_OBJECT_ID                        (0x00000112)    /**< This action sends a request to read the Search Results Object ID. */
#define BLE_MCP_ACTION_READ_CURRENT_TRACK_OBJECT                            (0x00000115)    /**< This action sends a request to read the Current Track Object. */

#define BLE_MCP_ACTION_SET_CURRENT_TRACK_OBJECT_ID_NOTIFICATION             (0x00000308)    /**< This action sends a request to set notification of the Current Track Object ID characteristic. */
#define BLE_MCP_ACTION_SET_NEXT_TRACK_OBJECT_ID_NOTIFICATION                (0x00000309)    /**< This action sends a request to set notification of the Next Track Object ID characteristic. */
#define BLE_MCP_ACTION_SET_PARENT_GROUP_OBJECT_ID_NOTIFICATION              (0x0000030A)    /**< This action sends a request to set notification of the Parent Group Object ID characteristic. */
#define BLE_MCP_ACTION_SET_CURRENT_GROUP_OBJECT_ID_NOTIFICATION             (0x0000030B)    /**< This action sends a request to set notification of the Current Group Object ID characteristic. */
#define BLE_MCP_ACTION_SET_MEDIA_CONTROL_POINT_NOTIFICATION                 (0x0000030E)    /**< This action sends a request to set notification of the Media Control Point characteristic. */
#define BLE_MCP_ACTION_SET_SEARCH_CONTROL_POINT_NOTIFICATION                (0x00000310)    /**< This action sends a request to set notification of the Search Control Point characteristic. */
#define BLE_MCP_ACTION_SET_SEARCH_RESULTS_OBJECT_ID_NOTIFICATION            (0x00000311)    /**< This action sends a request to set notification of the Search Results Object ID characteristic. */

/**
 *  @brief Defines the MCP events.
 */
#define BLE_MCP_DISCOVER_SERVICE_COMPLETE_IND                   (BT_LE_AUDIO_MODULE_MCP | 0x0000)    /**< This event indicates the MCP service discovery complete with payload #ble_mcp_discover_service_complete_t. */

#define BLE_MCP_CURRENT_TRACK_OBJECT_ID_IND             (BT_LE_AUDIO_MODULE_MCP | 0x0008)    /**< This event indicates the Current Track Object ID changes with payload #ble_mcp_current_track_object_id_ind_t. */
#define BLE_MCP_NEXT_TRACK_OBJECT_ID_IND                (BT_LE_AUDIO_MODULE_MCP | 0x0009)    /**< This event indicates the Next Track Object ID changes with payload #ble_mcp_next_track_object_id_ind_t. */
#define BLE_MCP_PARENT_GROUP_OBJECT_ID_IND              (BT_LE_AUDIO_MODULE_MCP | 0x000A)    /**< This event indicates the Parent Group Object ID changes with payload #ble_mcp_parent_group_object_id_ind_t. */
#define BLE_MCP_CURRENT_GROUP_OBJECT_ID_IND             (BT_LE_AUDIO_MODULE_MCP | 0x000B)    /**< This event indicates the Current Group Object ID changes with payload #ble_mcp_current_group_object_id_ind_t. */
#define BLE_MCP_MEDIA_CONTROL_POINT_IND                 (BT_LE_AUDIO_MODULE_MCP | 0x000E)    /**< This event indicates the Media Control Point changes with payload #ble_mcp_media_control_point_ind_t. */
#define BLE_MCP_SEARCH_CONTROL_POINT_IND                (BT_LE_AUDIO_MODULE_MCP | 0x0010)    /**< This event indicates the Search Control Point changes. */
#define BLE_MCP_SEARCH_RESULTS_OBJECT_ID_IND            (BT_LE_AUDIO_MODULE_MCP | 0x0011)    /**< This event indicates the Search Results Object ID haracteristic with payload #ble_mcp_search_results_object_ind_t. */

#define BLE_MCP_READ_MEDIA_PLAYER_ICON_OBJECT_ID_CNF                        (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_READ_MEDIA_ICON_OBJECT_ID)    /**< The result of reading Media Player Icon Object ID with payload #ble_mcp_read_media_player_icon_object_id_cnf_t. */
#define BLE_MCP_READ_CURRENT_TRACK_SEGMENTS_OBJECT_ID_CNF                   (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_READ_CURRENT_TRACK_SEGMENTS_OBJECT_ID)    /**< The result of reading Track Segments Object ID with payload #. */
#define BLE_MCP_READ_CURRENT_TRACK_OBJECT_ID_CNF                            (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_READ_CURRENT_TRACK_OBJECT_ID)    /**< The result of reading Current Track Object ID with payload #ble_mcp_read_current_track_object_id_cnf_t. */
#define BLE_MCP_READ_NEXT_TRACK_OBJECT_ID_CNF                               (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_READ_NEXT_TRACK_OBJECT_ID)    /**< The result of reading Next Track Object ID with payload #ble_mcp_read_next_track_object_id_cnf_t. */
#define BLE_MCP_READ_PARENT_GROUP_OBJECT_ID_CNF                             (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_READ_PARENT_GROUP_OBJECT_ID)    /**< The result of reading Parent Group Object ID with payload #. */
#define BLE_MCP_READ_CURRENT_GROUP_OBJECT_ID_CNF                            (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_READ_CURRENT_GROUP_OBJECT_ID)    /**< The result of reading Current Group Object ID with payload #. */
#define BLE_MCP_READ_SEARCH_RESULTS_OBJECT_ID_CNF                           (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_READ_SEARCH_RESULTS_OBJECT_ID)    /**< The result of reading Search Results Object ID with payload #. */

#define BLE_MCP_SET_CURRENT_TRACK_OBJECT_ID_NOTIFICATION_CNF             (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_SET_CURRENT_TRACK_OBJECT_ID_NOTIFICATION)    /**< The result of setting notification of the Current Track Object ID characteristic with payload #ble_mcp_event_parameter_t. */
#define BLE_MCP_SET_NEXT_TRACK_OBJECT_ID_NOTIFICATION_CNF                (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_SET_NEXT_TRACK_OBJECT_ID_NOTIFICATION)    /**< The result of setting notification of the Next Track Object ID characteristic with payload #ble_mcp_event_parameter_t. */
#define BLE_MCP_SET_PARENT_GROUP_OBJECT_ID_NOTIFICATION_CNF              (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_SET_PARENT_GROUP_OBJECT_ID_NOTIFICATION)    /**< The result of setting notification of the Parent Group Object ID characteristic with payload #ble_mcp_event_parameter_t. */
#define BLE_MCP_SET_CURRENT_GROUP_OBJECT_ID_NOTIFICATION_CNF             (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_SET_CURRENT_GROUP_OBJECT_ID_NOTIFICATION)    /**< The result of setting notification of the Current Group Object ID characteristic with payload #ble_mcp_event_parameter_t. */
#define BLE_MCP_SET_MEDIA_CONTROL_POINT_NOTIFICATION_CNF                 (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_SET_MEDIA_CONTROL_POINT_NOTIFICATION)    /**< The result of setting notification of the Media Control Point characteristic with payload #ble_mcp_event_parameter_t. */
#define BLE_MCP_SET_SEARCH_CONTROL_POINT_NOTIFICATION_CNF                (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_SET_SEARCH_CONTROL_POINT_NOTIFICATION)    /**< The result of setting notification of the Search Control Point characteristic with payload #ble_mcp_event_parameter_t. */
#define BLE_MCP_SET_SEARCH_RESULTS_OBJECT_ID_NOTIFICATION_CNF            (BT_LE_AUDIO_MODULE_MCP | BLE_MCP_ACTION_SET_SEARCH_RESULTS_OBJECT_ID_NOTIFICATION)    /**< The result of setting notification of the Search Results Object ID characteristic with payload #ble_mcp_event_parameter_t. */

#define BLE_MCP_PLAY_CURRENT_TRACK_CNF                                   (BT_LE_AUDIO_MODULE_MCP | 0x0201)    /**< The result of #ble_mcp_play_current_track_req(). */
#define BLE_MCP_PAUSE_CURRENT_TRACK_CNF                                  (BT_LE_AUDIO_MODULE_MCP | 0x0202)    /**< The result of #ble_mcp_pause_current_track_req(). */
#define BLE_MCP_STOP_CURRENT_TRACK_CNF                                   (BT_LE_AUDIO_MODULE_MCP | 0x0203)    /**< The result of #ble_mcp_stop_current_track_req(). */
#define BLE_MCP_MOVE_TO_NEXT_TRACK_CNF                                   (BT_LE_AUDIO_MODULE_MCP | 0x0204)    /**< The result of #ble_mcp_move_to_next_track_req(). */
#define BLE_MCP_MOVE_TO_PREVIOUS_TRACK_CNF                               (BT_LE_AUDIO_MODULE_MCP | 0x0205)    /**< The result of #ble_mcp_move_to_previous_track_req(). */
#define BLE_MCP_FAST_FORWARD_CNF                                         (BT_LE_AUDIO_MODULE_MCP | 0x0206)    /**< The result of #ble_mcp_fast_forward_req(). */
#define BLE_MCP_FAST_REWIND_CNF                                          (BT_LE_AUDIO_MODULE_MCP | 0x0207)    /**< The result of #ble_mcp_fast_rewind_req(). */
#define BLE_MCP_WRITE_PLAYBACK_SPEED_CNF                                 (BT_LE_AUDIO_MODULE_MCP | 0x0208)    /**< The result of #ble_mcp_write_playback_speed_req(). */
#define BLE_MCP_WRITE_PLAYING_ORDER_CNF                                  (BT_LE_AUDIO_MODULE_MCP | 0x0209)    /**< The result of #ble_mcp_write_playing_order_req(). */
#define BLE_MCP_MOVE_TO_FIRST_TRACK_CNF                                  (BT_LE_AUDIO_MODULE_MCP | 0x020A)    /**< The result of #ble_mcp_move_to_first_track_req(). */
#define BLE_MCP_MOVE_TO_LAST_TRACK_CNF                                   (BT_LE_AUDIO_MODULE_MCP | 0x020B)    /**< The result of #ble_mcp_move_to_last_track_req(). */
#define BLE_MCP_MOVE_TO_TRACK_NUMBER_CNF                                 (BT_LE_AUDIO_MODULE_MCP | 0x020C)    /**< The result of #ble_mcp_move_to_track_number_req(). */
#define BLE_MCP_MOVE_TO_NEXT_GROUP_CNF                                   (BT_LE_AUDIO_MODULE_MCP | 0x020D)    /**< The result of #ble_mcp_move_to_track_number_req(). */
#define BLE_MCP_MOVE_TO_PREVIOUS_GROUP_CNF                               (BT_LE_AUDIO_MODULE_MCP | 0x020E)    /**< The result of #ble_mcp_move_to_previous_group_req(). */
#define BLE_MCP_MOVE_TO_FIRST_GROUP_CNF                                  (BT_LE_AUDIO_MODULE_MCP | 0x020F)    /**< The result of #ble_mcp_move_to_first_group_req(). */
#define BLE_MCP_MOVE_TO_LAST_GROUP_CNF                                   (BT_LE_AUDIO_MODULE_MCP | 0x0210)    /**< The result of #ble_mcp_move_to_last_group_req(). */
#define BLE_MCP_MOVE_TO_GROUP_NUMBER_CNF                                 (BT_LE_AUDIO_MODULE_MCP | 0x0211)    /**< The result of #ble_mcp_move_to_group_number_req(). */
#define BLE_MCP_MOVE_TO_NEXT_SEGMENT_CNF                                 (BT_LE_AUDIO_MODULE_MCP | 0x0212)    /**< The result of #ble_mcp_move_to_next_segment_req(). */
#define BLE_MCP_MOVE_TO_PREVIOUS_SEGMENT_CNF                             (BT_LE_AUDIO_MODULE_MCP | 0x0213)    /**< The result of #ble_mcp_move_to_previous_segment_req(). */
#define BLE_MCP_MOVE_TO_FIRST_SEGMENT_CNF                                (BT_LE_AUDIO_MODULE_MCP | 0x0214)    /**< The result of #ble_mcp_move_to_first_segment_req(). */
#define BLE_MCP_MOVE_TO_LAST_SEGMENT_CNF                                 (BT_LE_AUDIO_MODULE_MCP | 0x0215)    /**< The result of #ble_mcp_move_to_last_segment_req(). */
#define BLE_MCP_MOVE_TO_SEGMENT_NUMBER_CNF                               (BT_LE_AUDIO_MODULE_MCP | 0x0216)    /**< The result of #ble_mcp_move_to_segment_number_req(). */
#define BLE_MCP_SET_ABSOLUTE_TRACK_POSITION_CNF                          (BT_LE_AUDIO_MODULE_MCP | 0x0217)    /**< The result of #ble_mcp_set_absolute_track_position_req(). */
#define BLE_MCP_SET_RELATIVE_TRACK_POSITION_CNF                          (BT_LE_AUDIO_MODULE_MCP | 0x0218)    /**< The result of #ble_mcp_set_relative_track_position_req(). */

#define BLE_MCP_WRITE_SEARCH_CNF                                         (BT_LE_AUDIO_MODULE_MCP | 0x0219)
#define BLE_MCP_TRACK_DISCOVERY_CNF                                      (BT_LE_AUDIO_MODULE_MCP | 0x021A)
#define BLE_MCP_WRITE_MCP_UNSUPPORTED_OPCODE_CNF                         (BT_LE_AUDIO_MODULE_MCP | 0x021B)
#define BLE_MCP_WRITE_TRACK_POSITION_CNF                                 (BT_LE_AUDIO_MODULE_MCP | 0x021C)
#define BLE_MCP_WRITE_NEXT_TRACK_OBJECT_ID_CNF                           (BT_LE_AUDIO_MODULE_MCP | 0x021D)
#define BLE_MCP_WRITE_CURRENT_GROUP_OBJECT_ID_CNF                        (BT_LE_AUDIO_MODULE_MCP | 0x021E)
#define BLE_MCP_WRITE_CURRENT_TRACK_OBJECT_ID_CNF                        (BT_LE_AUDIO_MODULE_MCP | 0x021F)
/**
 * @brief The MCP pre action state.
 */
#define BLE_MCP_STATE_NONE                                   0x00
#define BLE_MCP_STATE_CONFIG_CCCD_START                      0x01
#define BLE_MCP_STATE_CONFIG_CCCD_TRACK_CHANGED              BLE_MCP_STATE_CONFIG_CCCD_START
#define BLE_MCP_STATE_CONFIG_CCCD_MEDIA_STATE                0x02
#define BLE_MCP_STATE_CONFIG_CCCD_MEDIA_CONTROL              0x03
#define BLE_MCP_STATE_CONFIG_CCCD_CONTROL_OPCODE_SUPPORTED   0x04
#define BLE_MCP_STATE_CONFIG_CCCD_PLAYER_NAME                0x05 // player name must be placed last
#define BLE_MCP_STATE_CONFIG_CCCD_COMPLETE                   0x06
#define BLE_MCP_STATE_READ_START                             BLE_MCP_STATE_CONFIG_CCCD_COMPLETE
#define BLE_MCP_STATE_READ_MEDIA_CONTROL_OPCODE_SUPPORTED    BLE_MCP_STATE_READ_START
#define BLE_MCP_STATE_READ_MEDIA_STATE                       0x07
#define BLE_MCP_STATE_READ_PLAYING_ORDER_SUPPORTED           0x08
#define BLE_MCP_STATE_PRE_ACTION_COMPLETE                    0x09
#define BLE_MCP_STATE_ACTION_IDLE                                   BLE_MCP_STATE_PRE_ACTION_COMPLETE
#define BLE_MCP_STATE_OTP_ACTION_WAIT_READ_OBJECT_ID_RSP            (BLE_MCP_STATE_ACTION_IDLE + 1)
#define BLE_MCP_STATE_OTP_ACTION_WAIT_READ_OBJECT_INFORMATION_RSP   (BLE_MCP_STATE_OTP_ACTION_WAIT_READ_OBJECT_ID_RSP + 1)
typedef uint8_t ble_mcp_state_t;

typedef struct {
    uint16_t value_handle;          /**< The handle of characteristic value.*/
    uint16_t desc_handle;           /**< The handle of descriptor.*/
    ble_mcs_uuid_t uuid_type;
} ble_mcp_charc_t;

typedef struct {
    struct ble_mcp_service_t *front;
    uint8_t *long_charc_value;
    uint16_t long_charc_value_length;
    ble_mcs_media_control_opcodes_supported_t supported_opcodes;
    ble_mcs_playing_order_supported_t supported_playing_order;
    ble_mcs_media_state_t media_state;
    uint16_t start_handle;
    uint16_t end_handle;
    uint8_t idx;
    uint8_t ots_idx;
    uint8_t charc_num;
    ble_mcp_charc_t charc[1];
} ble_mcp_service_t;

typedef struct ble_mcp_linknode_t {
    struct ble_mcp_linknode_t *front;
} ble_mcp_linknode_t;

typedef struct {
    ble_mcp_event_t current_evt;
    uint8_t current_srv_idx;
    uint8_t mcs_num;
    ble_mcp_state_t state;
    bool mcs_discovery_complete;
    bool gmcs_discovery_complete;
    ble_mcp_service_t *p_gmcs;
    ble_mcp_linknode_t mcs_list;
    ble_mcp_set_attribute_callback_t set_attr_callback;
} ble_mcp_info_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_DISCOVER_SERVICE_COMPLETE_IND.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t mcs_num;                /**< The number of service instances.*/
    bool gmcs_supported;
} ble_mcp_discover_service_complete_ind_t;

typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    uint8_t object[BLE_MCS_OBJECT_ID_SIZE];
} ble_mcp_current_track_object_id_ind_t;

typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    uint8_t object[BLE_MCS_OBJECT_ID_SIZE];
} ble_mcp_next_track_object_id_ind_t;

typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    uint8_t object[BLE_MCS_OBJECT_ID_SIZE];
} ble_mcp_parent_group_object_id_ind_t;
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    uint8_t object[BLE_MCS_OBJECT_ID_SIZE];
} ble_mcp_current_group_object_id_ind_t;
/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MEDIA_CONTROL_POINT_IND.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    ble_mcs_media_control_point_t requested_opcode;
    ble_mcs_media_control_point_result_t result_code;
} ble_mcp_media_control_point_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_SEARCH_RESULTS_OBJECT_IND.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    uint8_t object[BLE_MCS_OBJECT_ID_SIZE];
} ble_mcp_search_results_object_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MEDIA_PLAYER_ICON_OBJECT_ID_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    uint8_t object[BLE_MCS_OBJECT_ID_SIZE];
} ble_mcp_read_media_player_icon_object_id_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_READ_CURRENT_TRACK_OBJECT_ID_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    uint8_t object[BLE_MCS_OBJECT_ID_SIZE];
} ble_mcp_read_current_track_object_id_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_READ_NEXT_TRACK_OBJECT_ID_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    uint8_t object[BLE_MCS_OBJECT_ID_SIZE];
} ble_mcp_read_next_track_object_id_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_READ_PARENT_GROUP_OBJECT_ID_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    uint8_t object[BLE_MCS_OBJECT_ID_SIZE];
} ble_mcp_read_parent_group_object_id_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_READ_CURRENT_GROUP_OBJECT_ID_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
    uint8_t object[BLE_MCS_OBJECT_ID_SIZE];
} ble_mcp_read_current_group_object_id_cnf_t;

typedef void (*ble_mcp_callback_t)(ble_mcp_event_t event, bt_status_t status, void *msg);

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_WRITE_SEARCH_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_search_cnf_t;

typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_track_discovery_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_WRITE_PLAYBACK_SPEED_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_write_playback_speed_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_WRITE_PLAYING_ORDER_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_write_playing_order_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_PLAY_CURRENT_TRACK_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_play_current_track_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_PAUSE_CURRENT_TRACK_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_pause_current_track_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_STOP_CURRENT_TRACK_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_stop_current_track_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_NEXT_TRACK_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_next_track_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_PREVIOUS_TRACK_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_previous_track_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_FIRST_TRACK_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_first_track_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_LAST_TRACK_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_last_track_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_TRACK_NUMBER_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_track_number_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_NEXT_GROUP_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_next_group_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_PREVIOUS_GROUP_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_previous_group_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_FIRST_GROUP_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_first_group_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_LAST_GROUP_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_last_group_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_GROUP_NUMBER_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_group_number_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_NEXT_SEGMENT_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_next_segment_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_PREVIOUS_SEGMENT_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_previous_segment_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_FIRST_SEGMENT_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_first_segment_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_LAST_SEGMENT_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_last_segment_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_MOVE_TO_SEGMENT_NUMBER_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_move_to_segment_number_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_FAST_FORWARD_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_fast_forward_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_FAST_REWIND_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_fast_rewind_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_SET_ABSOLUTE_TRACK_POSITION_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_set_absolute_track_position_cnf_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_MCP_SET_RELATIVE_TRACK_POSITION_CNF.
 */
typedef struct {
    bt_status_t status;             /**< Event status. */
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service index. */
} ble_mcp_set_relative_track_position_cnf_t;


/* Unsupported*/
bt_status_t ble_mcp_set_xxx_req(bt_handle_t handle, uint8_t service_idx);

bt_status_t ble_mcp_event_handler(bt_msg_type_t msg, bt_status_t status, void *buff);

bt_status_t ble_mcp_otp_event_handler(bt_msg_type_t msg, bt_status_t status, void *buff);

/**
 * @brief                       This function reset the service attribute of TBS.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_mcp_reset_service_attribute(bt_handle_t handle);

bt_status_t ble_mcp_deinit(void);

bt_status_t ble_mcp_init(ble_mcp_callback_t callback);


/**
 * @brief                       This function write media playback speed.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] value             is the playback speed.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_write_playback_speed_req(bt_handle_t handle, uint8_t service_idx, uint8_t value);

/**
 * @brief                       This function write media playing order.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] playing_order     is the playing order type.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_write_playing_order_req(bt_handle_t handle, uint8_t service_idx, ble_mcs_playing_order_t playing_order);

/**
 * @brief                       This function set media player play current track.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_play_current_track_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player pause current track.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_pause_current_track_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player stop current track.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_stop_current_track_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to next track.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_next_track_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to previous track.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_previous_track_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to first track.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_first_track_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to last track.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_last_track_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move track byt the track number.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] track_number      is the track number.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_track_number_req(bt_handle_t handle, uint8_t service_idx, int32_t track_number);

/**
 * @brief                       This function set media player move to the next group.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_next_group_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to the previous group.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_previous_group_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to the first group.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_first_group_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to the last group.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_last_group_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to the group by group number.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] group_number      is the group number.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_group_number_req(bt_handle_t handle, uint8_t service_idx, int32_t group_number);

/**
 * @brief                       This function set media player move to next segment.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_next_segment_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to previous segment.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_previous_segment_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to first segment.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_first_segment_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to last segment.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_last_segment_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player move to the segment by segment number.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] segment_number    is the segment number.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_move_to_segment_number_req(bt_handle_t handle, uint8_t service_idx, int32_t segment_number);

/**
 * @brief                       This function set media player fast forward.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_fast_forward_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player fast rewind.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_fast_rewind_req(bt_handle_t handle, uint8_t service_idx);

/**
 * @brief                       This function set media player absolute track position.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] track_position    is the absolute track position.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_set_absolute_track_position_req(bt_handle_t handle, uint8_t service_idx, int32_t track_position);

/**
 * @brief                       This function set media player relative track position.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] offset            is the offset.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_CONNECTION_NOT_FOUND, link not exist.
 *                              #BT_STATUS_CONNECTION_IN_USE, other command in process.
 */
bt_status_t ble_mcp_set_relative_track_position_req(bt_handle_t handle, uint8_t service_idx, int32_t offset);


ble_mcs_media_state_t ble_mcp_get_service_media_state(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_play_current_track_cmd(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_read_media_player_icon_object_id_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_read_track_duration_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_read_track_position_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_write_track_position_cmd(bt_handle_t handle, uint8_t service_idx, uint8_t value);
bt_status_t ble_mcp_write_track_position_req(bt_handle_t handle, uint8_t service_idx, uint8_t value);
bt_status_t ble_mcp_read_playback_speed_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_write_playback_speed_cmd(bt_handle_t handle, uint8_t service_idx, uint8_t value);
bt_status_t ble_mcp_read_seeking_speed_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_write_current_track_object_id_cmd(bt_handle_t handle, uint8_t service_idx, uint8_t value[BLE_MCS_OBJECT_ID_SIZE]);
bt_status_t ble_mcp_write_current_track_object_id_req(bt_handle_t handle, uint8_t service_idx, uint8_t value[BLE_MCS_OBJECT_ID_SIZE]);
bt_status_t ble_mcp_read_next_track_object_id_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_write_next_track_object_id_cmd(bt_handle_t handle, uint8_t service_idx, uint8_t value[BLE_MCS_OBJECT_ID_SIZE]);
bt_status_t ble_mcp_write_next_track_object_id_req(bt_handle_t handle, uint8_t service_idx, uint8_t value[BLE_MCS_OBJECT_ID_SIZE]);
bt_status_t ble_mcp_read_parent_group_object_id_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_read_current_group_object_id_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_write_current_group_object_id_cmd(bt_handle_t handle, uint8_t service_idx, uint8_t value[BLE_MCS_OBJECT_ID_SIZE]);
bt_status_t ble_mcp_write_current_group_object_id_req(bt_handle_t handle, uint8_t service_idx, uint8_t value[BLE_MCS_OBJECT_ID_SIZE]);
bt_status_t ble_mcp_read_playing_order_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_write_playing_order_cmd(bt_handle_t handle, uint8_t service_idx, ble_mcs_playing_order_t playing_order);
bt_status_t ble_mcp_read_media_state_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_read_media_control_opcodes_supported_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_read_media_player_icon_url_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_read_media_player_name_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_read_playing_order_supported_req(bt_handle_t handle, uint8_t service_idx);
bt_status_t ble_mcp_set_absolute_track_position_cmd(bt_handle_t handle, uint8_t service_idx, int32_t track_position);

#endif  /* __BLE_MCP_INTERNAL_H__ */
