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

#ifndef __BT_MESH_KEYTOOL_INTERNAL_H__
#define __BT_MESH_KEYTOOL_INTERNAL_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "bt_mesh_crypto_internal.h"

/**
 * @brief 	This function composes network nonce based on Mesh Profile Specification v1.0 3.8.5.1\n
 * @param[out] out is a memory buffer to store network nonce
 * @param[in] ctlttl is the CTL and TTL value.
 * @param[in] seq is the Sequence Number.
 * @param[in] src_addr is the source address.
 * @param[in] ividx is current IV Index.
 * @return NONE
 */
void mesh_keytool_get_net_nonce(uint8_t *out, uint8_t ctlttl, uint8_t *seq, uint16_t src_addr, uint32_t ividx);

/**
 * @brief 	This function composes application nonce based on Mesh Profile Specification v1.0 3.8.5.2\n
 * @param[out] out is a memory buffer to store network nonce.
 * @param[in] aszmic is 1 if a Segmented Access message or 0 for all other message formats.
 * @param[in] seq is Sequence Number of the Access message.
 * @param[in] src_addr is the source address.
 * @param[in] dst_addr is the destination address.
 * @param[in] ividx is current IV Index.
 * @return NONE
 */
void mesh_keytool_get_app_nonce(uint8_t *out, uint8_t aszmic, uint8_t *seq, uint16_t src_addr, uint16_t dst_addr, uint32_t ividx);

/**
 * @brief 	This function composes device nonce based on Mesh Profile Specification v1.0 3.8.5.3\n
 * @param[out] out is a memory buffer to store network nonce.
 * @param[in] aszmic is 1 if a Segmented Access message or 0 for all other message formats.
 * @param[in] seq is Sequence Number of the Access message.
 * @param[in] src_addr is the source address.
 * @param[in] dst_addr is the destination address.
 * @param[in] ividx is current IV Index.
 * @return NONE
 */
void mesh_keytool_get_device_nonce(uint8_t *out, uint8_t aszmic, uint8_t *seq, uint16_t src_addr, uint16_t dst_addr, uint32_t ividx);

/**
 * @brief 	This function composes proxy nonce based on Mesh Profile Specification v1.0 3.8.5.4\n
 * @param[out] out is a memory buffer to store network nonce.
 * @param[in] seq is the Sequence Number.
 * @param[in] src_addr is the source address.
 * @param[in] ividx is current IV Index.
 * @return NONE
 */
void mesh_keytool_get_proxy_nonce(uint8_t *out, uint8_t *seq, uint16_t src_addr, uint32_t ividx);


void mesh_keytool_application_key( uint8_t *appkey, mesh_crypto_cb_t *cb );
void mesh_keytool_device_key( uint8_t *ecdh_secret, uint8_t *salt, mesh_crypto_cb_t *cb );
void mesh_keytool_encryption_and_privacy_keys(uint8_t *netkey, uint8_t *lpn_addr, uint8_t *friend_addr,
        uint8_t *lpn_counter, uint8_t *friend_counter, mesh_crypto_cb_t *cb);
void mesh_keytool_network_id( uint8_t *netkey, mesh_crypto_cb_t *cb );
void mesh_keytool_identity_key( uint8_t *netkey, mesh_crypto_cb_t *cb );
void mesh_keytool_beacon_key( uint8_t *netkey, mesh_crypto_cb_t *cb );
void mesh_keytool_confirmation_key( uint8_t *ecdh_secret, uint8_t *salt, mesh_crypto_cb_t *cb );
void mesh_keytool_session_key( uint8_t *ecdh_secret, uint8_t *salt, mesh_crypto_cb_t *cb );
void mesh_keytool_session_nonce( uint8_t *ecdh_secret, uint8_t *salt, mesh_crypto_cb_t *cb );
/*NOTE: the following apis in cryptotool operation is little endian
 *            should use mesh_utils_byte_revert before calling all apis
 *            should use mesh_utils_byte_revert after getting result
*/
bool mesh_keytool_get_dhkey(uint8_t *pk, mesh_crypto_cb_t *cb);

bool mesh_keytool_read_local_p256_public_key(void);

uint32_t mesh_keytool_crc32(uint32_t crc, const uint8_t *buf, int size);

#endif  // __BT_MESH_KEYTOOL_INTERNAL_H__
