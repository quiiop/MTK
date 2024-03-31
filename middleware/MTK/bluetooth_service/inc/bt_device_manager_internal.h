/* Copyright Statement:
 *
 * (C) 2005-2021  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#ifndef __BT_DEVICE_MANAGER_INTERNAL_H__
#define __BT_DEVICE_MANAGER_INTERNAL_H__

#include <stdio.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "bt_type.h"
#include "bt_gap.h"
#include "bt_sink_srv.h"
#include "bt_device_manager.h"
#include "bt_connection_manager_internal.h"

#ifdef __cplusplus
extern "C"
{
#endif /* #ifdef __cplusplus */

typedef struct {
    bt_gap_link_key_notification_ind_t paired_key;
    char name[BT_GAP_MAX_DEVICE_NAME_LENGTH + 1];
} bt_device_manager_db_remote_paired_info_t;

typedef struct {
    uint8_t     version;
    uint16_t    manufacturer_id;
    uint16_t    subversion;
} bt_device_manager_db_remote_version_info_t;

typedef struct {
    uint16_t product_id;
    uint16_t vender_id;
#ifdef MTK_AP_DEFAULT_TYPE
    uint16_t version;
#endif /* #ifdef MTK_AP_DEFAULT_TYPE */
} bt_device_manager_db_remote_pnp_info_t;

typedef struct {
#ifdef BT_SINK_SRV_HFP_STORAGE_SIZE
    uint8_t hfp_info[BT_SINK_SRV_HFP_STORAGE_SIZE];
#endif /* #ifdef BT_SINK_SRV_HFP_STORAGE_SIZE */
#ifdef BT_SINK_SRV_A2DP_STORAGE_SIZE
    uint8_t a2dp_info[BT_SINK_SRV_A2DP_STORAGE_SIZE];
#endif /* #ifdef BT_SINK_SRV_A2DP_STORAGE_SIZE */
#ifdef BT_SINK_SRV_AVRCP_STORAGE_SIZE
    uint8_t avrcp_info[BT_SINK_SRV_AVRCP_STORAGE_SIZE];
#endif /* #ifdef BT_SINK_SRV_AVRCP_STORAGE_SIZE */
#ifdef BT_SINK_SRV_PBAP_STORAGE_SIZE
    uint8_t pbap_info[BT_SINK_SRV_PBAP_STORAGE_SIZE];
#endif /* #ifdef BT_SINK_SRV_PBAP_STORAGE_SIZE */
} bt_device_manager_db_remote_profile_info_t;

#define BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED   0x01
#define BT_DEVICE_MANAGER_REMOTE_INFO_MASK_VERSION  0x02
#define BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PROFILE  0x04
#define BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PNP      0x08
typedef uint8_t bt_device_manager_remote_info_mask_t;

#define BT_DEVICE_MANAGER_LOCAL_INFO_ADDRESS_LOCAL          (0x00)
#define BT_DEVICE_MANAGER_LOCAL_INFO_ADDRESS_LOCAL_RANDOM   (0x01)
#define BT_DEVICE_MANAGER_LOCAL_INFO_ADDRESS_REMOTE         (0x02)
#define BT_DEVICE_MANAGER_LOCAL_INFO_ADDRESS_SPEAKER_FIXED  (0x03)
typedef uint8_t bt_device_manager_local_info_address_type_t;

#ifndef LOG_MSGID_I
#define LOG_MSGID_I(module, message, arg_cnt, ...) LOG_I(module, message, ##__VA_ARGS__)
#endif /* #ifndef LOG_MSGID_I */
#ifndef LOG_MSGID_W
#define LOG_MSGID_W(module, message, arg_cnt, ...) LOG_W(module, message, ##__VA_ARGS__)
#endif /* #ifndef LOG_MSGID_W */
#ifndef LOG_MSGID_E
#define LOG_MSGID_E(module, message, arg_cnt, ...) LOG_E(module, message, ##__VA_ARGS__)
#endif /* #ifndef LOG_MSGID_E */

#define bt_device_manager_assert(x) configASSERT(x)

#define __BT_DEVICE_MANAGER_DEBUG_INFO__
#ifdef __BT_DEVICE_MANAGER_DEBUG_INFO__
//#define bt_dmgr_report_id(_message, arg_cnt, ...) printf(_message, ##__VA_ARGS__)
#define bt_dmgr_report_id(_message, arg_cnt, ...) LOG_MSGID_I(BT_DM_EDR, _message, arg_cnt, ##__VA_ARGS__)
#else /* #ifdef __BT_DEVICE_MANAGER_DEBUG_INFO__ */
#define bt_dmgr_report_id(_message, arg_cnt, ...);
#endif /* #ifdef __BT_DEVICE_MANAGER_DEBUG_INFO__ */

void bt_device_manager_dump_link_key(uint8_t *linkkey);
void bt_device_manager_dump_bt_address(bt_device_manager_local_info_address_type_t addr_type, uint8_t *address);

bt_status_t bt_device_manager_remote_delete_info(bt_bd_addr_t *addr, bt_device_manager_remote_info_mask_t info_mask);//Todo

void bt_device_manager_remote_get_paired_list(bt_device_manager_paired_infomation_t *info, uint32_t *read_count);
bt_status_t bt_device_manager_remote_find_paired_info(bt_bd_addr_t addr, bt_device_manager_db_remote_paired_info_t *info);
bt_status_t bt_device_manager_remote_find_paired_info_by_seq_num(uint8_t sequence, bt_device_manager_db_remote_paired_info_t *info);
bt_status_t bt_device_manager_remote_update_paired_info(bt_bd_addr_t addr, bt_device_manager_db_remote_paired_info_t *info);
bt_status_t bt_device_manager_remote_find_version_info(bt_bd_addr_t addr, bt_device_manager_db_remote_version_info_t *info);
bt_status_t bt_device_manager_remote_update_version_info(bt_bd_addr_t addr, bt_device_manager_db_remote_version_info_t *info);
bt_status_t bt_device_manager_remote_find_profile_info(bt_bd_addr_t addr, bt_device_manager_db_remote_profile_info_t *info);
bt_status_t bt_device_manager_remote_update_profile_info(bt_bd_addr_t addr, bt_device_manager_db_remote_profile_info_t *info);
bt_status_t bt_device_manager_remote_find_pnp_info(bt_bd_addr_t addr, bt_device_manager_db_remote_pnp_info_t *info);
bt_status_t bt_device_manager_remote_update_pnp_info(bt_bd_addr_t addr, bt_device_manager_db_remote_pnp_info_t *info);
uint32_t bt_device_manager_remote_find_cod(bt_bd_addr_t addr);
bt_status_t bt_device_manager_remote_update_cod(bt_bd_addr_t addr, uint32_t cod);
void bt_device_manager_remote_info_init(void);

void bt_device_manager_set_device_mode(bt_device_manager_mode_type_t mode);

void bt_device_manager_power_on_cnf(void);
void bt_device_manager_power_off_cnf(void);
void bt_device_manager_test_mode_init(void);


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifndef __BT_DEVICE_MANAGER_INTERNAL_H__ */

