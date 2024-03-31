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

#ifndef __BT_MESH_NETWORK_INTERNAL_H__
#define __BT_MESH_NETWORK_INTERNAL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "bt_mesh_common.h"
#include "bt_mesh_utils.h"

#define NETWORK_PDU_SEQ_NUMBER_LEN    3
#define NETWORK_PDU_SRC_ADDR_LEN    2
#define NETWORK_PDU_DST_ADDR_LEN    2

/* Network PDU parameter */
typedef struct {
    /*** keep this order, must fit network_layer_decrypt_params (Begin) ****/
    uint8_t ivi : 1;
    uint8_t nid : 7;
    uint8_t ctl : 1;
    uint8_t ttl : 7;
    uint8_t seq_num[NETWORK_PDU_SEQ_NUMBER_LEN];
    bt_mesh_address_t src_addr;
    bt_mesh_address_t dst_addr;
    uint32_t ivindex;
    uint8_t *transport_pdu;
    uint8_t *network_mic;
    uint8_t transport_pdu_len;
    uint8_t network_mic_len; /* 32 or 64 bits, depends on CTL field */
    int8_t rssi;
    /*** keep this order, must fit network_layer_decrypt_params (End) ****/
} __attribute__((packed)) mesh_network_pdu_params;

/*!
    @brief Network Layer initialization
*/
void mesh_network_init( void );

void mesh_network_receive_from_bearer(uint8_t len, uint8_t *buf, int8_t rssi, bool isGatt, bool isProxy);

#if MESH_PROXY
void mesh_network_receive_from_proxy_out(uint8_t len, uint8_t *buf, uint16_t netkeyIdx);

void mesh_network_receive_from_proxy_in(uint8_t len, uint8_t *buf);
#endif

/*!
    @brief Dump network layer message cache and relay cache
*/
void mesh_network_dump( void );

void Mesh_Network_ReceiveFromTransport_ex(uint8_t len, uint8_t *buf,
        uint16_t src_addr, uint16_t dst_addr, bool ctl, uint8_t ttl,
        uint16_t netkeyIdx, uint32_t ivindex, uint8_t credential_flag, uint8_t retry, uint8_t is_from_gatt);

void Mesh_Network_ReceiveFromTransport(uint8_t len, uint8_t *buf,
        uint16_t src_addr, uint16_t dst_addr, bool ctl, uint8_t ttl,
        uint16_t netkeyIdx, uint32_t ivindex, uint8_t retry, uint8_t is_from_gatt);

#endif // __BT_MESH_NETWORK_INTERNAL_H__

