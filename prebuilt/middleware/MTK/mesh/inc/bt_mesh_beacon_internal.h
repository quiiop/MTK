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

#ifndef __BT_MESH_BEACON_INTERNAL_H__
#define __BT_MESH_BEACON_INTERNAL_H__

#include "bt_mesh_provision.h"

#define MESH_BEACON_NETWORK_ID_SIZE (8)    /**< Network ID size in mesh, can't be modified.*/

#define MESH_BEACON_GATT_PROVISIONING_ADV_DATA_SIZE             29
#define MESH_BEACON_GATT_NODE_IDENTITY_ADV_DATA_SIZE            28
#define MESH_BEACON_GATT_NETWORK_ID_ADV_DATA_SIZE               20

#define MESH_BEACON_TYPE_UNPROVISIONED_DEVICE_SIZE       19
#define MESH_BEACON_TYPE_SECURE_NETWORK_SIZE             22

#define MESH_BEACON_TYPE_UNPROVISIONED_DEVICE            0x00
#define MESH_BEACON_TYPE_SECURE_NETWORK                  0x01

#define IV_INDEX_UPDATE_INIT_STATE                    0x00
#define IV_INDEX_UPDATE_NORMAL_STATE_STAGE_1          0x01
#define IV_INDEX_UPDATE_NORMAL_STATE_STAGE_2          0x02
#define IV_INDEX_UPDATE_IN_PROGRESS_STATE_STAGE_1     0x03
#define IV_INDEX_UPDATE_IN_PROGRESS_STATE_STAGE_2     0x04

void mesh_beacon_init(void);
void mesh_beacon_set_ud_scan(bool enable);
uint8_t mesh_beacon_get_state(void);
void mesh_beacon_set_state(uint8_t state);
void mesh_beacon_receive_from_bearer(uint8_t len, uint8_t *buf);
void mesh_beacon_send_unprovisioned_device(bt_mesh_prov_unprovisioned_device_t *params, uint8_t retry);
void mesh_beacon_send_secure_network(uint16_t key_index, bool proxy);
void mesh_beacon_send_secure_network_all(uint8_t flags);
void mesh_beacon_send_secure_network_periodically(bool on);
void mesh_beacon_check_key_refresh(uint8_t flags, uint16_t key_index, uint8_t *network_id);
bool mesh_beacon_is_iv_update_in_progress(void);
bool mesh_beacon_check_iv_update(uint8_t flags, uint32_t iv_index, uint16_t key_index);
void mesh_beacon_start_iv_index_recovery(void);
void mesh_beacon_defer_iv_index_update(bool defercheck);

#endif // __BT_MESH_BEACON_INTERNAL_H__

