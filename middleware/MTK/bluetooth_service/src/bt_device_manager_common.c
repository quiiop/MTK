/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
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

#ifdef MTK_NVDM_ENABLE
#include "nvkey.h"
#include "nvkey_id_list.h"
#endif /* #ifdef MTK_NVDM_ENABLE */
#include "syslog.h"
#include "bt_device_manager_db.h"

#define BT_DEVICE_MANAGER_LOCAL_INFO_NVDM_GROUP_INFO         "BT"
#define BT_DEVICE_MANAGER_LOCAL_INFO_NVDM_ITEM_LOCAL_ADDR    "local_addr"

static bt_device_manager_db_local_info_t local_info;

log_create_module(BT_DM_EDR, PRINT_LEVEL_INFO);

void bt_device_manager_common_init(void)
{
    static uint8_t is_init = 0;

    bt_dmgr_report_id("[BT_DM][I]local info init V_2.0", 0);
    if (is_init == 0) {
        memset((void *)&local_info, 0, sizeof(local_info));
#ifdef MTK_BLE_DM_LOCAL_INFO_IN_DB
        bt_device_manager_db_storage_t local_storage = {
            .auto_gen = true,
            .storage_type = BT_DEVICE_MANAGER_DB_STORAGE_TYPE_NVDM,
            .nvdm_group_str = BT_DEVICE_MANAGER_LOCAL_INFO_NVDM_GROUP_INFO,
            .nvdm_item_str = BT_DEVICE_MANAGER_LOCAL_INFO_NVDM_ITEM_LOCAL_ADDR
        };
        bt_device_manager_db_init(BT_DEVICE_MANAGER_DB_TYPE_LOCAL_INFO, &local_storage, &local_info, sizeof(local_info));
#endif /* #ifdef MTK_BLE_DM_LOCAL_INFO_IN_DB */
        is_init = 1;
    }
}

void bt_device_manager_store_local_address(bt_bd_addr_t *addr)
{
    //(reduce log)bt_dmgr_report_id("[BT_DM][I] Store local address:", 0);
    //(reduce log)bt_device_manager_dump_bt_address(BT_DEVICE_MANAGER_LOCAL_INFO_ADDRESS_LOCAL, (uint8_t *)addr);
    if (memcmp(&local_info.local_address, addr, sizeof(bt_bd_addr_t))) {
        bt_device_manager_db_mutex_take();
        memcpy(&local_info.local_address, addr, sizeof(bt_bd_addr_t));
        bt_device_manager_db_mutex_give();
#ifdef MTK_BLE_DM_LOCAL_INFO_IN_DB
        bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_LOCAL_INFO);
    }
    bt_device_manager_db_flush(BT_DEVICE_MANAGER_DB_TYPE_LOCAL_INFO, BT_DEVICE_MANAGER_DB_FLUSH_BLOCK);
#else /* #ifdef MTK_BLE_DM_LOCAL_INFO_IN_DB */
    }
#endif /* #ifdef MTK_BLE_DM_LOCAL_INFO_IN_DB */
}

bt_bd_addr_t *bt_device_manager_get_local_address(void)
{
    //(reduce log)bt_dmgr_report_id("[BT_DM][I] Get local address:", 0);
    //(reduce log)bt_device_manager_dump_bt_address(BT_DEVICE_MANAGER_LOCAL_INFO_ADDRESS_LOCAL, (uint8_t *) & (local_info.local_address));
    return &local_info.local_address;
}

void bt_device_manager_store_local_random_address(bt_bd_addr_t *addr)
{
    //(reduce log)bt_dmgr_report_id("[BT_DM][I] Store local random address:", 0);
    //(reduce log)bt_device_manager_dump_bt_address(BT_DEVICE_MANAGER_LOCAL_INFO_ADDRESS_LOCAL_RANDOM, (uint8_t *)addr);
    if (memcmp(&local_info.local_random_address, addr, sizeof(bt_bd_addr_t))) {
        bt_device_manager_db_mutex_take();
        memcpy(&local_info.local_random_address, addr, sizeof(bt_bd_addr_t));
        bt_device_manager_db_mutex_give();
#ifdef MTK_BLE_DM_LOCAL_INFO_IN_DB
        bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_LOCAL_INFO);
    }
    bt_device_manager_db_flush(BT_DEVICE_MANAGER_DB_TYPE_LOCAL_INFO, BT_DEVICE_MANAGER_DB_FLUSH_BLOCK);
#else /* #ifdef MTK_BLE_DM_LOCAL_INFO_IN_DB */
    }
#endif /* #ifdef MTK_BLE_DM_LOCAL_INFO_IN_DB */
}

bt_bd_addr_t *bt_device_manager_get_local_random_address(void)
{
    //(reduce log)bt_dmgr_report_id("[BT_DM][I] Get local random address:", 0);
    //(reduce log)bt_device_manager_dump_bt_address(BT_DEVICE_MANAGER_LOCAL_INFO_ADDRESS_LOCAL_RANDOM,
    //                                  (uint8_t *) & (local_info.local_random_address));
    return &local_info.local_random_address;
}

