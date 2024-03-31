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

#ifndef __BT_DEVICE_MANAGER_DB_H__
#define __BT_DEVICE_MANAGER_DB_H__

#include "bt_type.h"
#include "bt_device_manager_internal.h"
#include "bt_device_manager_config.h"

#ifdef __cplusplus
extern "C"
{
#endif /* #ifdef __cplusplus */

/* When this macro enabled, local info would write to NVDM. */
//#define MTK_BLE_DM_LOCAL_INFO_IN_DB

typedef struct {
    bt_bd_addr_t local_address;
    bt_bd_addr_t local_random_address;
#ifdef MTK_BT_DEVICE_MANAGER_DB_EXTENSION
    uint8_t      reserved[64];
#endif /* #ifdef MTK_BT_DEVICE_MANAGER_DB_EXTENSION */
} bt_device_manager_db_local_info_t;

// restructure
typedef enum {
#ifdef MTK_BLE_DM_LOCAL_INFO_IN_DB
    BT_DEVICE_MANAGER_DB_TYPE_LOCAL_INFO,
#endif /* #ifdef MTK_BLE_DM_LOCAL_INFO_IN_DB */
    BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO,
    BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO,
    /* Depend on max. paired number to reserve enum ID.
      (ex: max. pair num = 3, so we must reserve 3 DEVICE ID in enum.) */
    BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE_MAX = \
                                                  BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + BT_DEVICE_MANAGER_MAX_PAIR_NUM - 1,

    BT_DEVICE_MANAGER_DB_TYPE_MAX
} bt_device_manager_db_type_t;

#define BT_DEVICE_MANAGER_DB_STORAGE_TYPE_NVDM      (0x00)
#define BT_DEVICE_MANAGER_DB_STORAGE_TYPE_NVKEY     (0x01)
typedef uint8_t bt_device_manager_db_storage_type_t;

typedef struct {
    bool auto_gen;
    bt_device_manager_db_storage_type_t storage_type;
    union {
        uint32_t nvkey_id;
        struct {
            char *nvdm_group_str;
            char *nvdm_item_str;
        };
    };
} bt_device_manager_db_storage_t;

void bt_device_manager_db_init(bt_device_manager_db_type_t db_type,
                               bt_device_manager_db_storage_t *storage, void *db_buffer, uint32_t buffer_size);
void bt_device_manager_db_open(bt_device_manager_db_type_t db_type);
void bt_device_manager_db_close(bt_device_manager_db_type_t db_type);
void bt_device_manager_db_update(bt_device_manager_db_type_t db_type);
void bt_device_manager_db_flush(bt_device_manager_db_type_t db_type, bt_device_manager_db_flush_t block);
void bt_device_manager_db_mutex_take(void);
void bt_device_manager_db_mutex_give(void);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifndef __BT_DEVICE_MANAGER_DB_H__ */

