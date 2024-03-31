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

#ifndef __BT_MESH_CONFIG_INTERNAL_H__
#define __BT_MESH_CONFIG_INTERNAL_H__

#include "bt_mesh_config.h"
#include "bt_mesh_beacon_internal.h"
#include "bt_mesh_access.h"

#define MESH_LPN 1
#define MESH_FRIEND 1
#define MESH_PROXY 1

#define BT_MESH_CONFIG_NETWORK_TRANSMIT_COUNT (3)
#define BT_MESH_CONFIG_NETWORK_TRANSMIT_INTERVAL_STEPS (0)
#define BT_MESH_CONFIG_DEFAULT_TTL (4)

typedef struct {
    uint16_t keyidx;
    uint16_t src_addr;
    uint32_t seqnum;
    uint32_t ivindex; /* for LastAck */
    uint32_t seq; /* for LastAck */
    uint32_t block_ack; /* for LastAck */
} replay_protection_entry_t;

typedef struct {
    uint16_t keyidx;
    uint8_t updated;
    uint8_t aid;
    uint8_t key[BT_MESH_APPKEY_SIZE];
} appkey_entry_t;

typedef struct {
    uint8_t nid;
    uint8_t encryption_key[BT_MESH_NETKEY_SIZE];
    uint8_t privacy_key[BT_MESH_NETKEY_SIZE];
} encryption_info_t;

typedef struct {
    uint8_t lpn_addr[2];
    uint8_t friend_addr[2];
    uint8_t lpn_counter[2];
    uint8_t friend_counter[2];
    encryption_info_t encryption_data;
} friend_sec_entry_t;

typedef struct {
    uint16_t keyidx;
    friend_sec_entry_t *friend_security;
} friend_update_entry_t;

typedef struct {
    uint8_t *privacy_key;
    uint8_t *encryption_key;
    uint16_t keyidx: 15;
    uint16_t is_friend_security: 1;
    uint8_t nid;
} encryption_entry_t;

typedef struct {
    uint16_t keyidx;
    uint8_t key[BT_MESH_NETKEY_SIZE];
} netkey_t;

typedef struct {
    uint8_t key[BT_MESH_NETKEY_SIZE];
    uint8_t network_id[MESH_BEACON_NETWORK_ID_SIZE];
    encryption_info_t master_security;
    void *friend_security_list;
} netkey_data_t;

typedef struct {
    uint16_t keyidx;
    uint8_t state : 4;
    uint8_t node_identity : 4;
    uint8_t rest_retry : 6; /*rest retry count of node identity*/
    uint8_t flag : 2;
    netkey_data_t *netkey;
    netkey_data_t *temp_netkey;
    void *appkey_list;
    void *temp_appkey_list;
} netkey_entry_t;

/** @brief  This defines the callback function prototype.
 *          User should register a callback function while calling #mesh_config_init
 *  @param [in] success: true means the init process is done.
 */
typedef void (*init_done_handler) (bool success);

/**
 * @brief 	Set maximum netkey count
 * @param[in] keyCount maximum key count to be set.
 * @return NONE
 * @note for PTS usage
 */
void mesh_config_set_max_netkey_count(uint8_t keyCount);

/**
 * @brief 	Initialize configuration module
 * @param[in] init_params is the parameters for initialization.
 * @param[in] callback will be called when configuration module initialization is complete.
 * @return NONE
 * @note Please use #bt_mesh_init instead of using this api individually.
 */
void mesh_config_init(bt_mesh_config_init_params_t *init_params, init_done_handler callback);

void mesh_config_deinit(void);

void mesh_config_dump(void);

/*!
    @brief Set the supported feature of device.
    @param[in] feature is a bit field indicating the device features.
    @return NONE
*/
void mesh_config_set_feature_supported(uint16_t feature);

/*!
    @brief Get the supported feature of device.
    @return a bit field indicating the device features.
*/
uint16_t mesh_config_get_feature_supported(void);

/*!
    @brief Check the specified feature is supported or not.
    @param[in] feature is a bit field indicating the device features.
    @return
    true means the feature is supported.\n
    false means the feature is not supported.
*/
bool mesh_config_is_feature_supported(uint16_t feature);

/*!
    @brief    This function is used to set uuid for this device.
    @param[in] uuid is the 16-byte UUID
    @return NULL
 */
void mesh_config_set_uuid( uint8_t *uuid );

#if MESH_FRIEND
uint16_t mesh_config_get_friend_counter(bool increase);
void mesh_config_set_friend_counter(uint16_t counter);
#endif

#if MESH_LPN
uint16_t mesh_config_get_lpn_counter(bool increase);
void mesh_config_set_lpn_counter(uint16_t counter);
#endif

netkey_entry_t *Mesh_Config_getNetKeyEntryByIndex( uint16_t netkeyidx );
uint8_t Mesh_Config_deleteApplicationKey( uint8_t *keyidx );

void *Mesh_Config_getApplicationKeyByAid( uint8_t aid );
void *Mesh_Config_getNetworkKeyByNetworkID( uint8_t *network_id );
void *Mesh_Config_getNetworkKeyByNid( uint8_t nid );
void *Mesh_Config_getDeviceKeyList(uint16_t src_addr);

uint8_t *mesh_config_get_device_key( void );

/*!
    @brief Add application key
    @param keyidx netkey index and appkey index
    @param appkey 16-byte appkey
*/
uint8_t Mesh_Config_addApplicationKeyEx(uint8_t *keyidx, uint8_t *appkey);

/*!
    @brief Update application key
    @param keyidx netkey index and appkey index
    @param appkey 16-byte new appkey
*/
uint8_t Mesh_Config_updateApplicationKey(uint8_t *keyidx, uint8_t *appkey);

/*!
    @brief Add friend security
    @param key_idx the assigned key index for this key
    @param lpn_addr
    @param friend_addr
    @param lpn_counter
    @param friend_counter
    @note will generate nid, encryption key, privacy key automatically
*/
uint8_t mesh_config_add_friendship_security(
    uint16_t key_idx,
    uint8_t *lpn_addr,
    uint8_t *friend_addr,
    uint8_t *lpn_counter,
    uint8_t *friend_counter);

void mesh_config_delete_friendship_security(uint16_t key_idx, uint16_t lpn_addr);

/*!
    @brief Add network key
    @param netkey is the 16-byte key
    @param keyidx means the assigned key index for this key
    @param state means key refresh state
    @param node_identity indicates node identity status
    @param flag means the assigned flag for this key
    @note will generate nid, encryption key, privacy key automatically
*/
uint8_t Mesh_Config_addNetworkKeyEx(const uint8_t *netkey, uint16_t keyidx,
                                    bt_mesh_key_refresh_state_t state, uint8_t node_identity, uint8_t flag );

/*!
    @brief Get key refresh state for specified key index
    @param keyidx key index
*/
uint8_t Mesh_Config_getKeyState( uint16_t keyidx );

/*!
    @brief Start key refresh procedure phase 2
    @param keyidx key index
*/
uint8_t Mesh_Config_useNewNetkey( uint16_t keyidx );

/*!
    @brief Start key refresh procedure phase 3
    @param keyidx key index
*/
uint8_t Mesh_Config_revokeTempKey( uint16_t keyidx );

/*!
    @brief Get count of network keys.
    @return total key count of network keys
*/
uint32_t Mesh_Config_getNetkeyCount(void);

/*!
    @brief Get network key index list
    @param[out] keyCount (output)key count
    @param[in] node_identity means need to check node identity or not.
*/
uint16_t *Mesh_Config_getNetkeyIndexList( uint32_t *keyCount, bool node_identity );

/*!
    @brief find related application key by application key index
    @param appkeyidx the request key index
*/
appkey_entry_t *Mesh_Config_getAppKeyByIndex( uint16_t appkeyidx );

/*!
    @brief find related network key by network key index
    @param netkeyidx the request key index
*/
netkey_data_t *Mesh_Config_getNetKeyByIndex( uint16_t netkeyidx );

/*!
    @brief find related network key by application key index
    @param appkeyidx the bounded appkey index
*/
netkey_entry_t *Mesh_Config_getNetKeyByAppIndex( uint16_t appkeyidx );

uint32_t mesh_config_get_sequence_number( bool increase );
uint8_t mesh_config_get_iv_index(uint32_t *ivindex);
uint8_t mesh_config_get_temp_iv_index(uint32_t *ivindex);
void mesh_config_set_iv_index(uint32_t ivindex);
void mesh_config_set_temp_iv_index(uint32_t ivindex);
void mesh_config_set_primary_network(bool isPrimaryNetwork);
bool mesh_config_is_primary_network(void);
uint16_t mesh_config_get_address(uint8_t *addr);
void mesh_config_set_address(uint16_t addr);
const char *mesh_config_get_uri(void);
uint16_t mesh_config_get_oob_info(void);
void *mesh_config_get_replay_protection(void);
uint16_t bt_mesh_config_get_replay_protection_size(void);
bt_mesh_access_msg_status_code_t bt_mesh_config_add_device_key(const uint8_t *device_key, uint16_t address);
void mesh_config_set_node_identity_retry(uint16_t key_index, uint8_t retry_count, bool decrement_other_node);
uint8_t mesh_config_get_node_identity_retry(uint16_t key_index);
void mesh_config_check_startup_setting(void);
void mesh_config_check_stop_setting(void);
#endif // __BT_MESH_CONFIG_INTERNAL_H__

