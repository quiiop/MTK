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

#ifndef __BT_MESH_PROXY_INTERNAL_H__
#define __BT_MESH_PROXY_INTERNAL_H__

#include <stdbool.h>
#include <stdint.h>

#include "bt_mesh_config_internal.h"

#define CHECK_PROXY_SUPPORTED \
    if (!mesh_config_is_feature_supported(BT_MESH_FEATURE_PROXY)) { \
        MESH_DEBUG_PRINTF(BT_MESH_DEBUG_PROXY, "Proxy Feature not supported\n"); \
        return; \
    }

typedef enum {
    MESH_PROXY_ADV_TYPE_NODE_IDENTITY,
    MESH_PROXY_ADV_TYPE_NETWORK_ID
} mesh_proxy_adv_type_t;

/**
 * @brief 	This function is used for proxy module initialization
 * @return NONE
 * @note Please use #bt_mesh_init instead of using this api individually.
 */
void mesh_proxy_init(void);

/**
 * @brief 	Advertising mesh proxy service with all known network id.
 * @return NONE
 */
void mesh_proxy_server_network_id_all(void);

/**
 * @brief 	Advertising mesh proxy service with all known node idenetity.
 * @return NONE
 */
void mesh_proxy_server_node_identity_all(void);

/**
 * @brief 	Advertising mesh proxy service with specified node idenetity.
 * @param[in] netkeyidx means which the network key index should be used.
 * @return NONE
 */
bool mesh_proxy_server_node_identity(uint16_t netkeyidx);

/**
 * @brief 	Disable advertising mesh proxy service with node idenetity.
 * @return NONE
 */
void mesh_proxy_server_node_identity_disable(void);

/**
 * @brief 	Start advertising with mesh proxy service data.
 * @return NONE
 */
void mesh_proxy_server_start(void);

/**
 * @brief 	Stop advertising with mesh proxy service data.
 * @return NONE
 */
void mesh_proxy_server_stop(void);

/**
 * @brief 	Send a message to report the status of the proxy filter.
 * @param[in] netkeyidx specifies the target network.
 * @return NONE
 * @note This api is used by a Proxy Server.
 */
void mesh_proxy_filter_status(uint16_t netkeyidx);

#if MESH_PROXY
void mesh_proxy_add_valid_message_addr(uint16_t addr);

void mesh_proxy_configuration(uint8_t len, uint8_t *buf, uint16_t netkeyidx);

bool mesh_proxy_check_filter(uint16_t dst_addr);
#endif

void mesh_beacon_receive_from_bearer(uint8_t len, uint8_t *buf);
void mesh_proxy_start_node_identity_timer(uint16_t key_index);
void mesh_proxy_receive_from_gatt(uint8_t len, uint8_t *buf);
void mesh_proxy_dump(void);
#endif // __BT_MESH_PROXY_INTERNAL_H__

