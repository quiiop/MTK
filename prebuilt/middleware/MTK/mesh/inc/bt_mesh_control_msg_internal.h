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

#ifndef __BT_MESH_CONTROL_MSG_INTERNAL_H__
#define __BT_MESH_CONTROL_MSG_INTERNAL_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bt_mesh_model_internal.h"

#define MESH_DEFAULT_CLEAR_TTL 0

#define MESH_HEARTBEAT_PUBLICATION_FEATURE_NONE             0
#define MESH_HEARTBEAT_PUBLICATION_FEATURE_RELAY            0x01
#define MESH_HEARTBEAT_PUBLICATION_FEATURE_PROXY            0x02
#define MESH_HEARTBEAT_PUBLICATION_FEATURE_FRIEND           0x04
#define MESH_HEARTBEAT_PUBLICATION_FEATURE_LOW_POWER        0x08

void mesh_heartbeat_send(bool is_once, bt_mesh_heartbeat_publication_t *publish_params, uint16_t feature_check);

void bt_mesh_control_msg_set_retry_count(uint8_t retry);

uint8_t mesh_control_msg_get_retry_count(void);

void mesh_control_msg_ack(uint16_t seq_zero, uint32_t block_ack, uint16_t src_addr, uint16_t dst_addr, uint16_t netkey_Idx);

void mesh_control_msg_heartbeat(
    uint8_t init_ttl,
    uint16_t features,
    uint16_t dst_addr,
    uint16_t netkey_idx);

void mesh_control_msg_heartbeat_handler(
    uint8_t *buf,
    uint8_t buf_len,
    uint16_t src_addr,
    uint16_t dst_addr,
    uint16_t netkey_idx,
    uint8_t ttl,
    int8_t rssi);

#endif // __BT_MESH_CONTROL_MSG_INTERNAL_H__
