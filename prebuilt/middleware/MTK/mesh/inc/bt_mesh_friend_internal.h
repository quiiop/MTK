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

#ifndef __BT_MESH_FRIEND_INTERNAL_H__
#define __BT_MESH_FRIEND_INTERNAL_H__

#include "bt_mesh_friend.h"
#include "bt_mesh_model.h"
#include "bt_mesh_network_internal.h"

#if MESH_FRIEND
typedef enum {
    FRIEND_QUEUE_ENTRY_TYPE_NORMAL = 0,
    FRIEND_QUEUE_ENTRY_TYPE_FRIEND_UPADTE = 1,
    FRIEND_QUEUE_ENTRY_TYPE_SECURITY_UPADTE = 2
} friend_queue_entry_type;

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
    /*** keep this order, must fit network_layer_decrypt_params (End) ****/
    uint8_t *transport_pdu;
    uint8_t transport_pdu_len;
    friend_queue_entry_type type;
} __attribute__((packed)) friend_cache_entry_t;
#endif

void mesh_friend_poll_handler(
    uint8_t *buf,
    uint8_t buf_length,
    uint16_t src_addr,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl,
    int8_t rssi);

void mesh_friend_update_handler(
    uint8_t *buf,
    uint8_t buf_length,
    uint16_t src_addr,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl,
    int8_t rssi);

void mesh_friend_request_handler(
    uint8_t *buf,
    uint8_t buf_length,
    uint16_t src_addr,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl,
    int8_t rssi);

void mesh_friend_offer_handler(
    uint8_t *buf,
    uint8_t buf_length,
    uint16_t src_addr,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl,
    int8_t rssi);

void mesh_friend_clear_handler(
    uint8_t *buf,
    uint8_t buf_length,
    uint16_t src_addr,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl,
    int8_t rssi);

void mesh_friend_clear_confirm_handler(
    uint8_t *buf,
    uint8_t buf_length,
    uint16_t src_addr,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl,
    int8_t rssi);

void mesh_friend_subscription_list_add_handler(
    uint8_t *buf,
    uint8_t buf_length,
    uint16_t src_addr,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl,
    int8_t rssi);

void mesh_friend_subscription_list_remove_handler(
    uint8_t *buf,
    uint8_t buf_length,
    uint16_t src_addr,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl,
    int8_t rssi);

void mesh_friend_subscription_list_confirm_handler(
    uint8_t *buf,
    uint8_t buf_length,
    uint16_t src_addr,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl,
    int8_t rssi);

#if MESH_FRIEND
void mesh_friend_inform_lpn_security_update(void);

void mesh_friend_queue_entry_free(friend_cache_entry_t *entry);

void *mesh_friend_queue_get(uint16_t lpn_addr);

void mesh_friend_queue_add(mesh_network_pdu_params *net_pdu);

bool mesh_friend_candidate_queue_add(mesh_network_pdu_params *net_pdu);

void mesh_friend_candidate_queue_remove(uint16_t src_addr, uint16_t dst_addr, uint16_t seqZero);

void mesh_friend_candidate_queue_commit(uint16_t src_addr, uint16_t dst_addr, uint8_t dst_addrType, uint16_t seq_zero);

void mesh_friend_candidate_queue_deinit(void);

void mesh_friend_candidate_queue_init(void);

void mesh_friend_candidate_queue_dump(void);

#endif

#if MESH_LPN
void mesh_lpn_stop_timer(bool msgFromFriend, bool fsnToggle);

void mesh_lpn_poll_message(bool msgFromFriend);

void mesh_lpn_poll_message_with_fsn_toggle(bool msgFromFriend, uint8_t op_code);

bool mesh_lpn_is_friend_address(uint16_t friend_addr);
#endif

void mesh_friend_poll(
    uint8_t fsn,
    uint16_t dst_addr,
    uint16_t netkey_index);

void mesh_friend_update(
    uint8_t flags,
    uint32_t iv_index,
    uint8_t md,
    uint16_t dst_addr,
    uint16_t netkey_index);

void mesh_friend_request(
    uint8_t criteria,
    uint8_t receive_delay,
    uint32_t poll_timeout,
    uint16_t prev_addr,
    uint8_t num_elements,
    uint16_t lpn_counter,
    uint16_t netkey_index);

void mesh_friend_offer(
    uint8_t receive_window,
    uint8_t queue_size,
    uint8_t subscription_list_size,
    int8_t rssi,
    uint16_t friend_counter,
    uint16_t dst_addr,
    uint16_t netkey_index);

void mesh_friend_clear(
    uint16_t lpn_addr,
    uint16_t lpn_counter,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl);

void mesh_friend_clear_confirm(
    uint16_t lpn_addr,
    uint16_t lpn_counter,
    uint16_t dst_addr,
    uint16_t netkey_index,
    uint8_t ttl);

void mesh_friend_subscription_list_add(
    uint8_t transaction_number,
    const uint16_t *addr_list,
    uint8_t list_count,
    uint16_t dst_addr,
    uint16_t netkey_index);

void mesh_friend_subscription_list_confirm(
    uint8_t transaction_number,
    uint16_t dst_addr,
    uint16_t netkey_index);

uint32_t mesh_friend_poll_timeout_get(uint16_t lpn_addr);

bt_mesh_lpn_poll_timeout_t *mesh_friend_poll_timeout_list_get(uint32_t *count);

void mesh_friend_friendship_all_terminate(void);

bool mesh_friend_is_lpn_unicast_address(uint16_t lpn_addr);

bool mesh_friend_is_lpn_address(uint16_t addr, uint8_t addr_type);

bool mesh_friend_feature_in_use(void);

void mesh_friend_dump(void);

bt_mesh_status_t mesh_friend_init_parameter_set(bt_mesh_friend_init_params_t *parm);

bool mesh_lpn_is_from_friend(bool msgFromFriend);

bool mesh_lpn_feature_in_use(void);

void mesh_lpn_dump(void);

#endif //__BT_MESH_FRIEND_INTERNAL_H__

