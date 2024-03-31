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


#ifndef __BLE_CCP_INTERNAL_H__
#define __BLE_CCP_INTERNAL_H__

#include "ble_ccp.h"
#include "ble_ccp_discovery.h"

/**
 * @brief The CCP event IDs.
 */
#define BLE_CCP_ACTION_WRITE_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL  (0x00000201)    /**< This action sends a request to write the bearer signal strength reporting interval with parameter #ble_ccp_action_write_bearer_signal_strength_reporting_interval_t. */
#define BLE_CCP_ACTION_WRITE_CALL_CONTROL_POINT                         (0x00000202)    /**< This action sends a request to write the call control point with parameter #ble_ccp_write_call_control_point_param_t. */

#define BLE_CCP_ACTION_SET_CALL_CONTROL_POINT_NOTIFICATION              (0x00000309)    /**< This action sends a request to set notification of the call control point characteristic with parameter #ble_ccp_action_set_notification_t. */

/**
 *  @brief Defines the CCP events.
 */
#define BLE_CCP_DISCOVER_SERVICE_COMPLETE_IND                           (BT_LE_AUDIO_MODULE_CCP | 0x0000)    /**< This event indicates the GTBS and TBS service discovery complete with payload #ble_ccp_discover_service_complete_ind_t. */
#define BLE_CCP_CALL_CONTROL_POINT_IND                                  (BT_LE_AUDIO_MODULE_CCP | 0x0009)    /**< This event indicates the call control point result with payload #ble_ccp_call_control_point_result_list_ind_t. */

#define BLE_CCP_WRITE_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL_CNF     (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_WRITE_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL)    /**< The result of writing bearer signal strength reporting interval with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_WRITE_CALL_CONTROL_POINT_CNF                            (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_WRITE_CALL_CONTROL_POINT)    /**< The result of writing call control point with payload #ble_ccp_event_parameter_t. */
#define BLE_CCP_WRITE_LONG_CALL_CONTROL_POINT_CNF                       (BT_LE_AUDIO_MODULE_CCP | 0x0F01)    /**< The event type of write long characteristic call control point. */

#define BLE_CCP_SET_CALL_CONTROL_POINT_NOTIFICATION_CNF                 (BT_LE_AUDIO_MODULE_CCP | BLE_CCP_ACTION_SET_CALL_CONTROL_POINT_NOTIFICATION)    /**< The result of setting notification of the call control point characteristic with payload #ble_ccp_event_parameter_t. */

/**
 * @brief The CCP state type.
 */
#define BLE_CCP_STATE_NONE                                       0x00
#define BLE_CCP_STATE_CONFIG_CCCD_START                          0x01
#define BLE_CCP_STATE_CONFIG_CCCD_BEARER_PROVIDER_NAME           BLE_CCP_STATE_CONFIG_CCCD_START
#define BLE_CCP_STATE_CONFIG_CCCD_BEARER_TECHNOLOGY              0x02
#define BLE_CCP_STATE_CONFIG_CCCD_BEARER_LIST_CURRENT_CALLS      0x03
#define BLE_CCP_STATE_CONFIG_CCCD_STATUS_FLAGS                   0x04
#define BLE_CCP_STATE_CONFIG_CCCD_CALL_STATE                     0x05
#define BLE_CCP_STATE_CONFIG_CCCD_CALL_CONTROL_POINT             0x06
#define BLE_CCP_STATE_CONFIG_CCCD_TERMINATION_REASON             0x07
#define BLE_CCP_STATE_CONFIG_CCCD_INCOMING_CALL                  0x08
#define BLE_CCP_CONFIG_STATE_READ_START                          0x09
#define BLE_CCP_CONFIG_STATE_READ_CALL_STATE                     BLE_CCP_CONFIG_STATE_READ_START
#define BLE_CCP_CONFIG_STATE_READ_STATUS_FLAGS                   0x0A
#define BLE_CCP_CONFIG_STATE_CCCD_COMPLETE                       0x0B

typedef uint8_t ble_ccp_state_t;


#define BLE_CCP_MAX_CALL_NUM    4

/**
 *  @brief This structure defines the conditions for TBS characteristic.
 */
typedef struct {
    uint16_t value_handle;          /**< The handle of characteristic value.*/
    uint16_t desc_handle;           /**< The handle of descriptor.*/
    ble_tbs_uuid_t uuid_type;   /**< characteristic uuid type.*/
} ble_ccp_charc_t;
/**
 *  @brief This structure defines the conditions for TBS service instance.
 */
typedef struct node {
    struct node *next;              /**< The next instance of TBS.*/
    uint8_t *long_charc_value;
    uint16_t long_charc_value_length;
    uint16_t start_handle;          /**< The start attribute handle of the service instance. */
    uint16_t end_handle;            /**< The end attribute handle of the service instance. */
    uint8_t charc_num;              /**< The number of characteristic in the TBS service instance.*/
    ble_ccp_charc_t charc[1];       /**< The handle of characteristic value.*/
} ble_ccp_service_t;

/**
 *  @brief This structure defines the information of TBS.
 */
typedef struct {
    ble_ccp_event_t current_evt;            /**< The event type of current action, refer to ble_ccp_event_t.*/
    uint8_t current_srv_idx;                /**< The service idx of the event type. */
    uint8_t tbs_num;                        /**< The TBS instance count of the CCP server.*/
    ble_ccp_state_t state: 6;               /**< The current state, refer to ble_ccp_state_t.*/
    uint8_t tbs_discovery_complete: 1;      /**< The TBS discovery complete flag.*/
    uint8_t gtbs_discovery_complete: 1;     /**< The GTBS discovery complete flag.*/
    uint8_t *data_ptr;                      /**< The data pointer for write long characteristic. */
    ble_ccp_service_t *tbs;                 /**< The TBS instances of the ccp server.*/
    ble_ccp_service_t *gtbs;
    ble_ccp_call_state_t call_state[BLE_CCP_MAX_CALL_NUM];
    ble_ccp_set_attribute_callback_t set_attr_callback;
} ble_ccp_info_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_DISCOVER_SERVICE_COMPLETE_IND.
 */
typedef struct {
    uint8_t ccid;                   /**< The content control ID. */
} ble_ccp_content_control_id_t;

typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t tbs_num;                /**< The number of service instances.*/
    bool gtbs_supported;            /**< The GTBS supported flag.*/
} ble_ccp_discover_service_complete_ind_t;

/**
 *  @brief This structure defines the data type of the parameter in function #ble_ccp_write_call_control_point_req().
 */
typedef struct {
    uint16_t length;                                    /**< The call control list length. */
    ble_tbs_call_control_point_t *call_control_point;    /**< The call control list. */
} ble_ccp_write_call_control_point_param_t;

/**
 *  @brief This structure defines the data type of the call control response.
 */
typedef struct {
    ble_tbs_call_control_opcode_t opcode;        /**< The call control opcode type. */
    uint8_t call_index;                         /**< The call index. */
    ble_tbs_call_control_result_t result;        /**< The call control result. */
} ble_ccp_call_control_point_result_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_CCP_CALL_CONTROL_POINT_NOTIFY.
 */
typedef struct {
    bt_handle_t handle;             /**< Connection handle. */
    uint8_t service_idx;            /**< Service idx. */
    uint16_t length;                /**< The length of call control response list. */
    ble_ccp_call_control_point_result_t *list; /**< The call control response list. */
} ble_ccp_call_control_point_result_list_ind_t;


typedef void (*ble_ccp_callback_t)(ble_ccp_event_t event, bt_status_t status, void *msg);

/**
 * @brief                       This function get specific call state of handle.
 * @return                      return call state
 */
ble_ccp_call_state_t *ble_ccp_get_call_state_list(bt_handle_t handle);

bt_status_t ble_ccp_event_handler(bt_msg_type_t msg, bt_status_t status, void *buff);

/**
 * @brief                       This function reset the service attribute of TBS.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_ccp_reset_service_attribute(bt_handle_t handle);

/**
 * @brief                       This function de-initializes the call control profile.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_ccp_deinit(void);

/**
 * @brief                       This function initializes the call control profile.
 * @param[in] callback          is the event callback @ref ble_ccp_callback_t.
 * @param[in] max_link_num      is the maximum number of links.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_ccp_init(ble_ccp_callback_t callback);

/**
 * @brief                       This function writes bearer signal strength reporting interval, the #BLE_CCP_WRITE_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL_CNF event is reported to
 *                              to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] interval          is the interval of reporting signal strength in second.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_OUT_OF_MEMORY, out of memory.
 */
bt_status_t ble_ccp_write_bearer_signal_strength_reporting_interval_req(bt_handle_t handle, uint8_t service_idx, uint8_t interval);

/**
 * @brief                       This function writes bearer signal strength reporting interval without an event.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] interval          is the interval of reporting signal strength in second.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_OUT_OF_MEMORY, out of memory.
 */
bt_status_t ble_ccp_write_bearer_signal_strength_reporting_interval_cmd(bt_handle_t handle, uint8_t service_idx, uint8_t interval);


/**
 * @brief                       This function writes call control point, the #BLE_CCP_WRITE_CALL_CONTROL_POINT_CFM event is reported to
 *                              to indicate the result of the action.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] list              is the call control point list.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_OUT_OF_MEMORY, out of memory.
 */
bt_status_t ble_ccp_write_call_control_point_req(bt_handle_t handle, uint8_t service_idx, ble_ccp_write_call_control_point_param_t *param);

/**
 * @brief                       This function writes call control point without an  event.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] service_idx       is the service idx.
 * @param[in] list              is the call control point list.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 *                              #BT_STATUS_OUT_OF_MEMORY, out of memory.
 */
bt_status_t ble_ccp_write_call_control_point_cmd(bt_handle_t handle, uint8_t service_idx, ble_ccp_write_call_control_point_param_t *param);

#endif  /* __BLE_CCP_INTERNAL_H__ */

