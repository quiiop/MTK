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

#ifndef __BT_MESH_FLASH_INTERNAL_H__
#define __BT_MESH_FLASH_INTERNAL_H__

#include "bt_mesh_config_internal.h"
#include "bt_mesh_model_internal.h"

#define BT_MESH_FRIEND_RECORD_NUMBER               45  /**< The max number of friend stored in flash.*/
#define BT_MESH_DEVICE_INFO_RECORD_NUMBER          20  /**< The max number of device information stored in flash.*/
#define BT_MESH_CONFIGURATION_SERVER_RECORD_NUMBER 15  /**< The max number of configuration server record stored in flash.*/
#define BT_MESH_SEQUENCE_NUMBER_RECORD_NUMBER      120 /**< The max number of sequence number stored in flash.*/
#define BT_MESH_MODEL_VIRTUAL_SUBS_RECORD_NUMBER   20  /**< The max number of virtual address subscription stored in flash.*/
#define BT_MESH_DEV_KEY_RECORD_NUMBER              20  /**< The max number of device keys stored in flash.*/
#define BT_MESH_OTA_BLOCK_SIZE                     8   /**< The max number of updater blocks*/

typedef enum {
    LOADING_DATA_FROM_FLASH_COMPLETE = 0,
    LOADING_NETKEY_FROM_FLASH,
    LOADING_FRIEND_FROM_FLASH,
    LOADING_TEMPNETKEY_FROM_FLASH,
    LOADING_APPKEY_FROM_FLASH,
    LOADING_TEMPAPPKEY_FROM_FLASH,
    LOADING_MODEL_FROM_FLASH,
    LOADING_DEVICE_INFO_FROM_FLASH,
} mesh_flash_loading_state_t;

typedef enum {
    SAVING_NETKEY_TO_FLASH = 0,
    SAVING_FRIEND_TO_FLASH,
    SAVING_APPKEY_TO_FLASH,
    SAVING_MODELINFO_TO_FLASH,
    SAVING_MODEL_PUBLISH_TO_FLASH,
    SAVING_MODEL_SUBSCRIBE_TO_FLASH,
    SAVING_VIRTUAL_SUBSCRIBE_TO_FLASH,
    SAVING_DEVICEINFO_TO_FLASH,
    SAVING_CONFIGDATA_TO_FLASH,
    SAVING_HEALTHDATA_TO_FLASH,
    SAVING_SEQUENCE_NUMBER_TO_FLASH,
} mesh_flash_saving_state_t;

/** @brief The capability of mesh storage, user can config when mesh init*/

typedef struct {
    uint16_t network_key_record;             /**< The max number of network keys stored in flash.*/
    uint16_t friend_record;                  /**< The max number of friend stored in flash.*/
    uint16_t application_key_record;         /**< The max number of application keys stored in flash.*/
    uint16_t model_record;                   /**< The max number of model record stored in flash.*/
    uint16_t publication_address_record;     /**< The max number of model publication record stored in flash.*/
    uint16_t subscription_address_record;    /**< The max number of model subscription record stored in flash.*/
    uint16_t virtual_subscription_record;    /**< The max number of model virtual subscription record stored in flash.*/
    uint16_t device_info_record;             /**< The max number of device information stored in flash.*/
    uint16_t configuration_server_record;    /**< The max number of configuration server record stored in flash.*/
    uint16_t health_server_record;	         /**< The max number of health server record stored in flash.*/
    uint16_t sequence_number;	             /**< The max number of sequence number stored in flash.*/
} mesh_record_config_internal_t;


/* flash record structure */
typedef struct {
    uint8_t isValidData;
    uint16_t unicast_addr;
    uint32_t model_id;
    uint16_t subscriptionAddr;
} mesh_model_flash_t;

typedef struct {
    uint8_t isValidData;
    uint8_t secureNetworkBeacon;
    uint8_t defaultTTL;
    uint8_t gattProxy;
    uint8_t friend;
    uint8_t relay;
    uint8_t nodeIdentity;
    uint8_t keyRefreshPhase;
    uint16_t unicastAddr;
    bt_mesh_heartbeat_publication_t heartbeatPublication;
    bt_mesh_heartbeat_subscription_t heartbeatSubscription;
    bt_mesh_network_transmit_t networkTransmit;
    bt_mesh_relay_retransmit_t relayRetransmit;
} __attribute__((packed)) configuration_server_flash_data_t;

typedef struct {
    uint8_t isValidData;
    mesh_health_period_t healthPeriod;
    uint8_t attention;
    uint16_t appkeyIdx;
    uint16_t unicastAddr;
} __attribute__((packed)) health_server_flash_data_t;

typedef struct {
    uint8_t isValidData;
    uint16_t unicast_addr;
    uint16_t cid;
    uint16_t pid;
    uint16_t crpl;
    uint16_t features;
    uint8_t ttl;
    uint8_t gattEnabled;
} __attribute__((packed)) composition_data_flash_t;

typedef struct {
    uint8_t isValidData;
    uint8_t ivindex_state;
    uint32_t ivindex;
    uint8_t deviceKey[BT_MESH_DEVKEY_SIZE];
    uint16_t unicast_addr;
} __attribute__((packed)) device_flash_data_t;

typedef struct {
    uint8_t isValidData;
    uint16_t appkeyIdx;
    uint8_t idLength;
    uint32_t model_id;
    uint16_t unicast_addr;
} __attribute__((packed)) model_flash_data_t;

typedef struct {
    uint8_t isValidData;
    uint32_t model_id;
    uint16_t unicast_addr;
    bt_mesh_model_publication_t model_publication;
    uint8_t virtual_uuid[BT_MESH_UUID_SIZE]; /* depends on model_publication.addr */
} __attribute__((packed)) model_publication_flash_data_t;

typedef struct {
    uint8_t isValidData;
    uint32_t model_id;
    uint16_t unicast_addr;
    uint16_t subscriptionAddr;
} __attribute__((packed)) model_subscription_flash_data_t;

typedef struct {
    uint32_t model_id;
    uint16_t unicast_addr;
    uint8_t subscription_uuid[BT_MESH_UUID_SIZE];
    uint8_t isValidData;
} __attribute__((packed)) model_virtual_subscription_flash_data_t;

typedef struct {
    uint8_t isValidData;
    uint8_t keyidx[3];
    uint8_t key[BT_MESH_APPKEY_SIZE];
    uint16_t netkeyIdx;
    uint8_t phase;
    uint8_t tmpkey[BT_MESH_APPKEY_SIZE];
} __attribute__((packed)) appkey_flash_data_t;

typedef struct {
    uint8_t isValidData;
    uint16_t netkeyIdx;
    uint8_t lpn_addr[2];
    uint8_t friend_addr[2];
    uint8_t lpn_counter[2];
    uint8_t friend_counter[2];
} __attribute__((packed)) friend_flash_data_t;

typedef struct {
    uint8_t isValidData;
    uint16_t keyidx;
    uint8_t key[BT_MESH_NETKEY_SIZE];
    uint8_t phase : 4;
    uint8_t node_identity : 4;
    uint8_t tmpkey[BT_MESH_NETKEY_SIZE];
} __attribute__((packed)) netkey_flash_data_t;
/* todo
typedef struct {
    uint8_t isValidData;
    uint8_t reserved;
    uint16_t numOfBlock;
    uint32_t object_size;
    uint32_t firmware_id;
    uint8_t object_id[8];
    uint8_t blk_mask[BT_MESH_OTA_BLOCK_SIZE];
}__attribute__((packed)) ota_flash_data_t;
*/
typedef struct {
    uint8_t is_valid_data;
    uint8_t seq_num[3];
} __attribute__((packed)) seqnum_flash_data_t;

/** @brief The nvdm config of mesh storage*/

typedef struct {
    uint16_t network_key_nvdm_num;             /**< The nvdm number of network keys stored in flash.*/
    uint16_t friend_nvdm_num;                  /**< The nvdm number of friend stored in flash.*/
    uint16_t application_key_nvdm_num;         /**< The nvdm number of application keys stored in flash.*/
    uint16_t model_nvdm_num;                   /**< The nvdm number of model record stored in flash.*/
    uint16_t publication_address_nvdm_num;     /**< The nvdm number of model publication record stored in flash.*/
    uint16_t subscription_address_nvdm_num;    /**< The nvdm number of model subscription record stored in flash.*/
    uint16_t device_info_nvdm_num;             /**< The nvdm number of device information stored in flash.*/
    uint16_t configuration_server_nvdm_num;    /**< The nvdm number of configuration server record stored in flash.*/
    uint16_t health_server_nvdm_num;	       /**< The nvdm number of health server record stored in flash.*/
} mesh_record_nvdm_config_t;


/** @brief  This defines the callback function prototype for loading flash.
 *          User should register a callback function while calling #mesh_flash_load
 *  @param [in] success: true means the flash loading process is done.
 */
typedef void (*load_done_handler) (bool success);

/**
 * @brief 	This function is used for flash module initialization
 * @return NONE
 * @note Please use #bt_mesh_init instead of using this api individually.
 */
void mesh_flash_init(mesh_record_config_t *mesh_record);

/**
 * @brief 	This function is used to save mesh data in current state.
 * @return
 * true means flash writing is done.\n
 * false means flash module is not initialized yet.
 */
bool mesh_flash_save(void);

/**
 * @brief 	This function is used to load mesh data stored in flash
 * @param[in] callback will be called when data loading is completed.
 * @return
 * true means flash is loading data, callback will be called when loading complete.\n
 * false means flash module is not initialized yet.
 */
bool mesh_flash_load(load_done_handler callback);

bool mesh_flash_load_data(void);
mesh_flash_loading_state_t mesh_flash_get_loading_state(void);

//void mesh_flash_delete_sequence_number_record(void);
//void mesh_flash_delete_netkey_record(uint16_t keyidx);
void mesh_flash_delete_friend_record(uint8_t *lpn_addr);
//void mesh_flash_delete_appkey_record(uint16_t keyidx);
void mesh_flash_delete_model_record(uint16_t appkeyIdx, uint32_t model_id, uint16_t unicastAddr);
//void mesh_flash_delete_model_publication_record(uint32_t model_id, uint16_t unicastAddr);
void mesh_flash_delete_model_subscription_record(uint16_t subscriptionAddr, uint32_t model_id, uint16_t unicastAddr);
void mesh_flash_delete_virtual_subscription_record(const uint8_t *virtual_uuid, uint32_t model_id, uint16_t unicastAddr);
//void mesh_flash_delete_device_record(void);
//void mesh_flash_delete_config_server_data_record(void);
//void mesh_flash_delete_health_server_record(uint16_t unicast_addr);
void mesh_flash_save_sequence_number(uint32_t seq);
void mesh_flash_save_netkey(netkey_entry_t *key );
void mesh_flash_save_friend_info(uint16_t keyidx, friend_sec_entry_t *key);
void mesh_flash_save_appkey(appkey_flash_data_t *appkeyData);
void mesh_flash_save_model_info(uint16_t appkeyIdx, uint32_t model_id, uint8_t idLength, uint16_t unicast_addr);
void mesh_flash_save_model_publication_ex(bt_mesh_model_publication_t model_publication, uint32_t model_id, uint16_t unicast_addr, uint8_t *virtual_uuid);
void mesh_flash_save_model_publication(bt_mesh_model_publication_t model_publication, uint32_t model_id, uint16_t unicast_addr);
void mesh_flash_save_model_subscription(uint16_t subscriptionAddr, uint32_t model_id, uint16_t unicast_addr);
void mesh_flash_save_virtual_subscription(const uint8_t *virtual_uuid, uint32_t model_id, uint16_t unicast_addr);
void mesh_flash_save_device_info();
void mesh_flash_save_config_data(void);
void mesh_flash_save_health_data(uint16_t unicast_addr);
/* to do
uint8_t bt_mesh_flash_ota_get_mask(uint16_t blk_idx);
void bt_mesh_flash_ota_set_mask(uint16_t blk_idx);
bool mesh_flash_save_ota_size(uint8_t obj_id[8], uint32_t obj_size);
void mesh_flash_setup_ota(uint32_t firmware_id, uint8_t obj_id[8], bool force_overwrite);
const uint8_t* mesh_flash_get_publication_virtual_uuid(uint32_t model_id, uint16_t unicast_addr);
*/
#endif // __BT_MESH_FLASH_INTERNAL_H__

