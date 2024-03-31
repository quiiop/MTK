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

#ifndef __BT_MESH_ACCESS_INTERNAL_H__
#define __BT_MESH_ACCESS_INTERNAL_H__

#include "bt_mesh_access.h"

/******************************************************************************
 * Forward Declaration
 ******************************************************************************/
struct bt_mesh_security_t;

#define HEALTH_CLIENT_API 0

typedef struct {
    uint16_t modelHandle;
    bt_mesh_access_msg_handler 	handler;
} mesh_access_handler;

typedef struct {
    /*** keep this order, must fit lower_transport_decrypt_params (Begin) ****/
    uint16_t src_addr;
    uint16_t dst_addr;
    uint16_t appkeyidx;
    uint16_t netkeyIdx;
    uint16_t buf_len;
    uint8_t ttl : 7;
    uint8_t is_virtual : 1;
    int8_t rssi;
    uint8_t is_gatt;
    uint8_t seq_num[3];
    uint32_t ivindex;
    /*** keep this order, must fit lower_transport_decrypt_params (End) ****/
} __attribute__((packed)) mesh_access_rx_params;

void mesh_access_init(void);

void mesh_access_dump(void);

bt_mesh_status_t mesh_access_send_message(
    uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint8_t *payload, uint16_t payload_length);

bt_mesh_status_t mesh_access_send_message_ex(uint16_t model_handle, const bt_mesh_access_message_tx_meta_t *tx_meta, uint32_t len4check, char *format, ...);

bt_mesh_status_t bt_mesh_access_model_reply_ex(uint16_t model_handle, const bt_mesh_access_message_rx_t *msg, uint16_t opcode, uint16_t company_id, uint32_t len4check, char *format, ...);

void mesh_access_send_to_upper_transport_ex1(uint16_t buf_len, const uint8_t *buf, uint16_t src_addr, uint16_t dst_addr,
        uint8_t ttl, uint8_t from_bearer, const struct bt_mesh_security_t *security, const uint8_t *uuid, uint8_t credential_flag);

void mesh_access_send_to_upper_transport_ex(uint16_t buf_len, const uint8_t *buf, uint16_t src_addr, uint16_t dst_addr,
        uint8_t ttl, uint8_t from_bearer, const struct bt_mesh_security_t *security, const uint8_t *uuid);

void mesh_access_send_to_upper_transport(uint16_t buf_len, const uint8_t *buf, uint16_t src_addr, uint16_t dst_addr,
        uint8_t ttl, uint8_t from_bearer, uint16_t appidx, uint16_t netkeyIdx);

void mesh_access_receive_from_upper_transport_with_subscription(mesh_access_rx_params *params, uint8_t *buf,  uint16_t sub_element_addr, uint8_t *uuid);

void mesh_access_receive_from_upper_transport(mesh_access_rx_params *params, uint8_t *buf);

void mesh_access_rx_message_free(bt_mesh_access_message_rx_t *msg);

void mesh_access_set_opcode(uint8_t *out, uint16_t companyid, uint16_t opcode);

#endif // __BT_MESH_ACCESS_INTERNAL_H__
