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

#ifndef __BT_MESH_PROVISION_INTERNAL_H__
#define __BT_MESH_PROVISION_INTERNAL_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "bt_type.h"
#include "bt_mesh_beacon_internal.h"
#include "bt_mesh_config.h"

#define MESH_VALIDATE_PUBLIC_KEY 0

/* Provisining encryption */
#define BT_MESH_PROV_NONCE_SIZE 13
#define BT_MESH_PROV_MIC_SIZE 8

/* Provisioning PDU Type */
typedef enum {
    PROVISIONING_INVITE = 0x00,
    PROVISIONING_CAPABILITIES = 0x01,
    PROVISIONING_START = 0x02,
    PROVISIONING_PUBLIC_KEY = 0x03,
    PROVISIONING_INPUT_COMPLETE = 0x04,
    PROVISIONING_CONFIRMATION = 0x05,
    PROVISIONING_RANDOM = 0x06,
    PROVISIONING_DATA = 0x07,
    PROVISIONING_COMPLETE = 0x08,
    PROVISIONING_FAILED = 0x09
} bt_mesh_prov_pdu_t;

/* Provisioning PDU Type */
typedef enum {
    ALI_PROVISIONING_DISCOVERY = 0x00,
    ALI_PROVISIONING_START = 0x01,
    ALI_PROVISIONING_RESPONSE = 0x02,
    ALI_PROVISIONING_CONFIRMATION_CLOUD = 0x03,
    ALI_PROVISIONING_CONFIRMATION_DEVICE = 0x04,
    ALI_PROVISIONING_AUTHENTICATION_RESULT = 0x05,
    ALI_PROVISIONING_PROVISIONING_COMPLETE = 0x06,
} bt_mesh_prov_ali_evt_id_t;

/* Provisioning Public Key structure */
typedef struct {
    uint8_t key_x[32];               /**< The X component of public key for the FIPS P-256 algorithm */
    uint8_t key_y[32];               /**< The Y component of public key for the FIPS P-256 algorithm */
} prov_public_key_t;

/* Provisioning Confirmation structure */
typedef struct {
    uint8_t confirmation[16];        /**< The values exchanged so far including the OOB Authentication value */
} prov_confirmation_t;

/* Provisioning Random structure */
typedef struct {
    uint8_t random[16];              /**< The final input to the confirmation */
} prov_random_t;

/* Provisioning Failed structure */
typedef struct {
    bt_mesh_prov_failed_error_t error_code;  /**< This represents a specific error in the provisioning protocol encountered by a device */
} prov_failed_t;

/* The secure network information */
typedef struct {
    uint8_t flags;
    uint8_t network_id[MESH_BEACON_NETWORK_ID_SIZE];
    uint16_t key_index;
    uint32_t iv_index;
    uint8_t auth_value[8];
    uint8_t beacon_key[BT_MESH_BEACONKEY_SIZE];
    void *key_list;
    bool proxy;
} prov_secure_network_t;

typedef struct {
    bool is_provisioner;
    bt_mesh_prov_capabilities_t cap;
} prov_init_params_t;

/* The link close reason */
typedef enum {
    BEARER_LINK_CLOSE_REASON_SUCCESS = 0x00,
    BEARER_LINK_CLOSE_REASON_TIMEOUT = 0x01,
    BEARER_LINK_CLOSE_REASON_FAIL = 0x02,
} prov_link_close_reason_t;

#if MESH_LIB_APP_LAYER
typedef struct {
    uint8_t confirmation_key[16];
    uint8_t provisioner_random[16];
} bt_mesh_evt_prov_ali_response;

typedef struct {
    uint8_t confirmation_key[16];
    uint8_t device_confirmation[16];
    uint8_t device_random[16];
} bt_mesh_evt_prov_ali_confirmation_device;

typedef struct bt_mesh_prov_ali_evt_t {
    bt_mesh_prov_ali_evt_id_t evt_id;
    union {
        bt_mesh_evt_prov_ali_response response;
        bt_mesh_evt_prov_ali_confirmation_device confirmation_device;
    } event;
} bt_mesh_prov_ali_evt_t;

typedef void (*bt_mesh_prov_request_confirmation_callback) (bt_mesh_prov_ali_evt_t *event);
typedef void (*bt_mesh_prov_evt_callback) (bt_mesh_prov_pdu_t type, uint8_t length, uint8_t *payload, uint8_t trans_num);
#endif

/*!
    @brief Provision module initialization
    @param[in] params are the parameters for provision module.
*/
void mesh_provision_init(const prov_init_params_t *params);

/*!
    @brief Provision module deinitialization
*/
void mesh_provision_deinit(void);

/*!
    @brief Start to listen for link establishment
    @param[in] uri is a optional device URI string. May be NULL.
    @param[in] oobinfo means OOB information sources, see #Provision OOB Info
    @note This api is only for provisionee role.
*/
void mesh_provision_listen(const char *uri, uint16_t oobinfo);

/*!
    @brief Enable provisioning server. It will start to scan unprovisioned device.
    @note This api is only for provisioner role.
 */
void mesh_provision_enable_server(void);

/*!
    @brief Disable provisioning server.
    @note This api is only for provisioner role.
*/
void mesh_provision_disable_server(void);

void mesh_provision_receive_from_bearer(uint8_t len, uint8_t *buf);
void mesh_provision_receive_from_gatt(uint8_t *data, uint16_t len);
void mesh_provision_update_gatt_status(bool connected);
void mesh_provision_update_provisioning_status(bool isProvisioned);
void mesh_provision_add_unprovisioned_device(bt_mesh_prov_unprovisioned_device_t *param);

/*!
    @brief Check if this device is provisioner or not
    @return
    true means this device act as a provisioner. \n
    false means this device act as a node. \n
*/
bool mesh_provision_is_provisioner(void);

void bt_mesh_prov_provide_authentication_result(bool success);

#if MESH_LIB_APP_LAYER
void bt_mesh_prov_register_confirmation_callback(bt_mesh_prov_request_confirmation_callback cb);
void bt_mesh_prov_provide_confirmation(uint8_t *confirmation);
#endif

#endif // __BT_MESH_PROVISION_INTERNAL_H__
