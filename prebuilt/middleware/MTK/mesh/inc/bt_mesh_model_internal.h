/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#ifndef __BT_MESH_MODEL_INTERNAL_H__
#define __BT_MESH_MODEL_INTERNAL_H__

#include "bt_mesh_access.h"
#include "bt_mesh_config.h"
#include "bt_mesh_common.h"
#include "bt_mesh_model.h"

typedef struct {
    uint16_t loc;
    uint8_t numS;
    uint8_t numV;
    void *sig_models;  // double-linked list
    void *vendor_models;  // double-lined list
} mesh_composition_element_t;

typedef struct {
    uint8_t secureNetworkBeacon;
    bt_mesh_composition_data_t compositionData;
    uint8_t defaultTTL;
    bt_mesh_feature_state_t gattProxy;
    bt_mesh_feature_state_t friend;
    bt_mesh_feature_state_t relay;
    bt_mesh_node_identity_t nodeIdentity;
    uint8_t keyRefreshPhase;
    bt_mesh_heartbeat_publication_t heartbeatPublication;
    bt_mesh_heartbeat_subscription_t heartbeatSubscription;
    bt_mesh_network_transmit_t networkTransmit;
    bt_mesh_relay_retransmit_t relayRetransmit;
} mesh_configuration_server_model_t;

typedef struct {
    uint8_t testID;
    uint16_t companyID;
    uint8_t *faultArray;
} mesh_health_fault_t;

typedef struct {
    uint8_t fastPeriod;
} mesh_health_period_t;

typedef struct {
    mesh_health_fault_t healthFault;
    mesh_health_period_t healthPeriod;
    uint8_t attention;
    uint16_t appkeyIdx;
} mesh_health_server_model_t;

typedef struct {
    uint32_t model_id;
    uint16_t modelHandle;
    void *modelData;
    void *appkey_list;
    bt_mesh_model_publication_t model_publication;
    void *subscriptionList;
    void *virtual_subscriptionList;
    const bt_mesh_access_opcode_handler_t *opcode_handlers;
    uint8_t opcode_count;
    bt_mesh_access_msg_handler callback;
    bt_mesh_access_publish_timeout_cb_t publish_timeout_cb;
} mesh_model_t;

typedef struct {
    uint16_t element_index;
    uint16_t unicastAddr;
    mesh_model_t *models;
    bool isPrimary;
} mesh_element_t;

typedef struct {
    mesh_element_t *elements; /**< a dlist storing elements */
} mesh_node_data_t;

void Mesh_Health_Model_LoadData(uint16_t addr, mesh_health_server_model_t serverData);

void Mesh_Config_Model_LoadData(uint16_t addr, mesh_configuration_server_model_t serverData);

/*!
    @brief Dump the composition data and model information about this node
    @return NONE
*/
void mesh_model_dump(void);

/*!
    @brief Bind a appkey with a model
    @param appidx the appkey index
    @param id the model id
    @param idLength the model id bytes, 2 or 4.
    @param unicastAddr the target element unicast address
    @return the errcode, ref 'Status codes' defined in bt_mesh_access.h. MESH_MSG_STATUS_SUCCESS means binding successfully
*/
uint8_t mesh_model_app_bind(uint16_t appidx, uint32_t id, uint8_t idLength, uint16_t unicastAddr);

void Mesh_Model_ClearAppkeyBinding(uint16_t appidx);

/*!
    @brief Unbind a appkey with a model
    @param appidx the appkey index
    @param id the model id
    @param idLength the model id bytes, 2 or 4.
    @param unicastAddr the target element unicast address
    @return the errcode, ref 'Status codes' defined in bt_mesh_access.h. MESH_MSG_STATUS_SUCCESS means binding successfully
*/
uint8_t mesh_model_app_unbind(uint16_t appidx, uint32_t id, uint8_t idLength, uint16_t unicastAddr);

/*!
    @brief clear fault list
    @param fault fault
*/
void mesh_health_model_clear_fault_list(uint16_t model_handle);

/*!
    @brief get health model publication
    @param model_publication [out]model publication
*/
bool mesh_health_model_get_publication(bt_mesh_model_publication_t *model_publication);

/*!
    @brief get fault list
    @param faultCount [out]fault count
*/
uint8_t *mesh_health_model_get_fault_list(uint8_t *faultCount);

/*!
    @brief Add fault to list in the health server model
    @param fault fault to be added into list
*/
void mesh_health_model_add_fault_to_list(uint8_t fault);

/*!
    @brief Get the element object of specififed address
    @param unicastAddr the target element address
    @return the target element object
*/
mesh_element_t *Mesh_Model_Get_Element(uint16_t unicastAddr);

/*!
    @brief Get the element object of specififed address by element index
    @param index the target element index
    @return the target element object
*/
mesh_element_t *Mesh_Model_Get_Element_By_Index(uint16_t index);

/*!
    @brief Get the model object by model id
    @param model_id the target model id
    @param element the element you want to search the model
    @return the target model object
*/
mesh_model_t *Mesh_Model_Get_Model_by_ID(uint32_t model_id, mesh_element_t *element);

/*!
    @brief Get pointer of foundation model by model ID
    @param model_id model ID
*/
void *Mesh_Model_Get_Foundation_Model_by_ID(uint32_t model_id);

/*!
    @brief Get the unicast address assigned for this node
    @param start a uint16_t pointer to pass the first address
    @param end a uint16_t pointer to pass the last address
*/

void mesh_model_get_element_addr_range(uint16_t *start, uint16_t *end);

uint8_t *Mesh_Model_getModelSubscriptionList(uint16_t unicastAddr, uint32_t id, uint8_t *addrCount);

/*!
    @brief Get all appkey index bound with the specified model
    @param unicastAddr the target element unicast address
    @param id the target model id
    @param keyCount a uint8_t pointer to pass the number of bound appkeys
    @return a uint8_t array containing all bound appkey in a little endian order
*/
uint8_t *Mesh_Model_getModelAppList(uint16_t unicastAddr, uint32_t id, uint32_t *keyCount, uint8_t *status);

/*!
    @brief Add a publication address to model
    @param[in] model_publication is the publication information
    @param[in] model_id is the target model id
    @param[in] unicast_addr is the target element unicast address
    @return NONE
*/
void mesh_model_add_model_publication(bt_mesh_model_publication_t model_publication, uint32_t model_id, uint16_t unicast_addr);

/*!
    @brief Add a subscription address to model
    @param subscriptionAddr the address to be subscribed
    @param model_id the target model id
    @param unicast_addr the target element unicast address
*/
void mesh_model_add_model_subscription(uint16_t subscriptionAddr, uint32_t model_id, uint16_t unicast_addr);

void mesh_model_add_virtual_subscription(const uint8_t *virtual_uuid, uint32_t model_id, uint16_t unicast_addr);

bool Mesh_Model_isOnHeartbeatSubscriptionList(uint16_t addr);

bool Mesh_Model_checkUUIDModelSubscription(uint16_t element_addr, uint32_t model_id, uint8_t *uuid);

bool Mesh_Model_checkModelSubscription(uint16_t element_addr, uint32_t model_id, uint16_t sub_addr);

void *Mesh_Model_checkSubscriptionListWithUUID(uint8_t *uuid);

/*!
    @brief Check if the addr is on any subscription list of this node
    @param addr the target address need to be checked
    @return a double-linked list stored with subscribed element unicast addr, all entries are uint16_t*.
    @note uint16_t *elementAddr = (uint16_t *)bt_mesh_os_layer_ds_dlist_first(list);
*/
void *Mesh_Model_checkSubscriptionList(uint16_t addr);

void *Mesh_Model_checkVirtualSubscriptionList(uint16_t addr);

/*!
    @brief Check if the appkey and model is bound
    @param appidx the appkey index
    @param id the model id
    @param idLength the model id bytes, 2 or 4.
    @param unicastAddr the target element unicast address
    @return true: bound, false: not bound
*/

bool Mesh_Model_checkAppBinded_byId(uint16_t appidx, uint32_t id, uint8_t idLength, uint16_t unicastAddr);

/*!
    @brief Check if the application key and model is bound
    @param[in] appidx is the target application key index
    @param[in] model_handle is the target model handle
    @return
    true means the application key is bound with specified model.\n
    false means the application key is not bound with specified model.
*/
bool mesh_model_check_app_binding_by_model_handle(uint16_t appidx, uint16_t model_handle);

/*!
    @brief Get pointer of health server model
    @return mesh_health_server_model_t pointer to health server model
*/
mesh_health_server_model_t *Mesh_Model_Get_Health_Server_Model(void);

/*!
    @brief Get pointer of configuration server model
    @return mesh_configuration_server_model_t pointer to configuration server model
*/
mesh_configuration_server_model_t *Mesh_Model_Get_Configuration_Server_Model(void);

mesh_model_t *mesh_model_find_model_by_handle(uint16_t *element_index, uint16_t model_handle);

void *mesh_model_get_opcode_handler(uint16_t addr, bt_mesh_access_opcode_t opcode);

bool bt_mesh_model_add_model_ex(uint16_t *model_handle, const bt_mesh_model_add_params_t *model_params, bt_mesh_access_msg_handler callback);

void mesh_model_schedule_publication(mesh_model_t *model);

void mesh_model_unschedule_publication(mesh_model_t *model);

uint8_t *mesh_model_get_composition_raw_data(uint16_t *length);

uint32_t mesh_model_get_model_id_by_opcode(bt_mesh_access_opcode_t *access_opcode);

bt_mesh_access_msg_status_code_t mesh_model_add_publication(
    uint16_t element_addr, uint32_t model_id, bt_mesh_model_publication_t *params);

#endif // __BT_MESH_MODEL_INTERNAL_H__
