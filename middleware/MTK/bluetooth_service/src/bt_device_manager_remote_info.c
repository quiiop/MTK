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

#include "FreeRTOS.h"
#ifdef MTK_NVDM_ENABLE
#include "nvkey.h"
#include "nvkey_id_list.h"
#endif /* MTK_NVDM_ENABLE */
#include "bt_gap.h"
#include "bt_gap_le.h"
#include "bt_device_manager.h"
#include "bt_callback_manager.h"
#include "bt_device_manager_config.h"
#include "bt_device_manager_internal.h"
#include "bt_device_manager_db.h"
#include "bt_timer_external.h"

#ifndef PACKED
#define PACKED  __attribute__((packed))
#endif

#define UNUSED(x)  ((void)(x))


typedef struct {
    bt_gap_link_key_type_t   key_type;
    bt_key_t                 key;
    char name[BT_GAP_MAX_DEVICE_NAME_LENGTH + 1];
} PACKED bt_device_manager_db_remote_paired_info_internal_t;

typedef struct {
    bt_bd_addr_t address;
    bt_device_manager_remote_info_mask_t info_valid_flag;
    bt_device_manager_db_remote_paired_info_internal_t paired_info;
    bt_device_manager_db_remote_version_info_t version_info;
    bt_device_manager_db_remote_pnp_info_t pnp_info;
    bt_device_manager_db_remote_profile_info_t profile_info;
    uint32_t     cod;
    uint8_t      reserved[60];
} PACKED bt_device_manager_db_remote_info_t;

static uint8_t g_bt_dm_remote_sequence[BT_DEVICE_MANAGER_MAX_PAIR_NUM];
static bt_device_manager_db_remote_info_t g_bt_dm_remote_list_cnt[BT_DEVICE_MANAGER_MAX_PAIR_NUM];

#define BT_DEVICE_MANAGER_TEMP_BEFORE_PAIRING_LIST_CNT (2)
static struct {
    bt_bd_addr_t    address;
    bt_device_manager_db_remote_pnp_info_t pnp_info;
} g_bt_dm_temp_before_pairing_cnt[BT_DEVICE_MANAGER_TEMP_BEFORE_PAIRING_LIST_CNT];

static struct {
    bt_bd_addr_t    address;
    uint32_t        cod;
} g_bt_dm_temp_cod_set_cnt[BT_DEVICE_MANAGER_TEMP_BEFORE_PAIRING_LIST_CNT];

static void bt_device_manager_show_remote_info(void)
{
    bt_device_manager_db_remote_info_t *temp_remote = &g_bt_dm_remote_list_cnt[0];
    //bt_device_manager_db_remote_info_t *find_remote = NULL;
    uint8_t *addr_p;
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        addr_p = (uint8_t *)temp_remote->address;
        bt_dmgr_report_id("[BT_DM][REMOTE][I] show info [%d], seq = %d (flag = %d) addr = %02x-%02x-%02x-%02x-%02x-%02x", 9,
                          index, g_bt_dm_remote_sequence[index], temp_remote->info_valid_flag,
                          addr_p[5], addr_p[4], addr_p[3], addr_p[2], addr_p[1], addr_p[0]);
    }
}

void bt_device_manager_remote_info_init(void)
{
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Remote info init", 0);
    bt_device_manager_db_storage_t remote_storage = {
        .auto_gen = true,
        .storage_type = BT_DEVICE_MANAGER_DB_STORAGE_TYPE_NVKEY,
        .nvkey_id = NVKEYID_BT_HOST_LINK_KEY_RECORD_ID_01
    };
    if (BT_DEVICE_MANAGER_MAX_PAIR_NUM > 16) {
        bt_dmgr_report_id("[BT_DM][REMOTE][W] Storage can only support 16 remote devices now", 0);
        bt_device_manager_assert(0 && "Device maximum number exceed the storage can support");
    } else if (0 == BT_DEVICE_MANAGER_MAX_PAIR_NUM) {
        bt_dmgr_report_id("[BT_DM][REMOTE][W] Maximum pair num is 0", 0);
        return;
    }
    memset(g_bt_dm_remote_list_cnt, 0, sizeof(g_bt_dm_remote_list_cnt));
    memset(g_bt_dm_remote_sequence, 0, sizeof(g_bt_dm_remote_sequence));
    memset(g_bt_dm_temp_before_pairing_cnt, 0, sizeof(g_bt_dm_temp_before_pairing_cnt));
    memset(g_bt_dm_temp_cod_set_cnt, 0, sizeof(g_bt_dm_temp_cod_set_cnt));
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++) {
        remote_storage.nvkey_id = NVKEYID_BT_HOST_LINK_KEY_RECORD_ID_01 + index;
        bt_device_manager_db_init(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + index,
                                  &remote_storage, &g_bt_dm_remote_list_cnt[index], sizeof(g_bt_dm_remote_list_cnt[index]));
    }
    remote_storage.nvkey_id = NVKEYID_BT_HOST_LINK_KEY_RECORD_SEQUENCE;
    bt_device_manager_db_init(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO,
                              &remote_storage, g_bt_dm_remote_sequence, sizeof(g_bt_dm_remote_sequence));
    bt_device_manager_show_remote_info();

}

bt_status_t bt_device_manager_remote_delete_info(bt_bd_addr_t *addr, bt_device_manager_remote_info_mask_t info_mask)
{
    bt_device_manager_db_remote_info_t *temp_remote = &g_bt_dm_remote_list_cnt[0];
    uint8_t clear_flag[BT_DEVICE_MANAGER_MAX_PAIR_NUM + 1] = {0};
    if (NULL != addr) {
        bt_dmgr_report_id("[BT_DM][REMOTE][I] Del info addr%02x:%02x:%02x:%02x:%02x:%02x, info mask %d", 7,
                          (*addr)[5], (*addr)[4], (*addr)[3], (*addr)[2], (*addr)[1], (*addr)[0], info_mask);
    } else {
        bt_dmgr_report_id("[BT_DM][REMOTE][I] Del info addr is null, info mask %d", 1, info_mask);
    }
    bt_device_manager_db_mutex_take();
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; temp_remote++, index++) {
        if (!g_bt_dm_remote_sequence[index]) {
            continue;
        }
        if (NULL == addr || !memcmp(&(temp_remote->address), addr, sizeof(bt_bd_addr_t))) {
            if (info_mask == 0) {
                temp_remote->info_valid_flag = 0;
            } else {
                temp_remote->info_valid_flag &= (~info_mask);
                clear_flag[index + 1] = 1;
            }
        } else {
            continue;
        }
        if (0 == temp_remote->info_valid_flag) {
            for (uint8_t i = 0; i < BT_DEVICE_MANAGER_MAX_PAIR_NUM; i++) {
                if (g_bt_dm_remote_sequence[i] > g_bt_dm_remote_sequence[index]) {
                    g_bt_dm_remote_sequence[i]--;
                }
            }
            if (g_bt_dm_remote_sequence[index]) {
                bt_dmgr_report_id("[BT_DM][REMOTE][I] Del dev index:%d, seq:%d %02x:%02x:%02x:%02x:%02x:%02x", 8,
                                  index, g_bt_dm_remote_sequence[index],
                                  temp_remote->address[5], temp_remote->address[4], temp_remote->address[3],
                                  temp_remote->address[2], temp_remote->address[1], temp_remote->address[0]);
            }
            g_bt_dm_remote_sequence[index] = 0;
            clear_flag[0] = 1;
            clear_flag[index + 1] = 0;
        }
    }
    bt_device_manager_db_mutex_give();
    if (1 == clear_flag[0]) {
        bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO);
#ifdef MT7933_SUPPORT_NVKEY_NON_BLOCKING
        bt_device_manager_db_flush(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO, BT_DEVICE_MANAGER_DB_FLUSH_NON_BLOCK);
#endif
    }
    for (uint8_t index = 1; index < sizeof(clear_flag); index++) {
        if (1 == clear_flag[index]) {
            bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + index - 1);
#ifdef MT7933_SUPPORT_NVKEY_NON_BLOCKING
            bt_device_manager_db_flush(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + index - 1, BT_DEVICE_MANAGER_DB_FLUSH_NON_BLOCK);
#endif
        }
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_device_manager_remote_set_seq_num(bt_bd_addr_t addr, uint8_t sequence)
{
    uint8_t index = 0;
    uint8_t find_index = 0xFF;
    bt_device_manager_db_remote_info_t *temp_remote = &g_bt_dm_remote_list_cnt[0];
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Set seq(%d) to dev %02x:%02x:%02x:%02x:%02x:%02x", 7,
                      sequence, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    bt_device_manager_db_mutex_take();
    for (index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (g_bt_dm_remote_sequence[index] && !memcmp(temp_remote->address, addr, sizeof(bt_bd_addr_t))) {
            find_index = index;
            break;
        }
    }
    if (0xFF == find_index) {
        bt_device_manager_db_mutex_give();
        bt_dmgr_report_id("[BT_DM][REMOTE][W] Set seq fail not find dev", 0);
        return BT_STATUS_FAIL;
    }
    if (sequence == g_bt_dm_remote_sequence[find_index]) {
        bt_device_manager_db_mutex_give();
        return BT_STATUS_SUCCESS;
    } else if (sequence > g_bt_dm_remote_sequence[find_index]) {
        uint8_t max_seq = g_bt_dm_remote_sequence[find_index];
        for (index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++) {
            max_seq = max_seq < g_bt_dm_remote_sequence[index] ? g_bt_dm_remote_sequence[index] : max_seq;
            if (g_bt_dm_remote_sequence[index] && (g_bt_dm_remote_sequence[index] <= sequence) &&
                (g_bt_dm_remote_sequence[index] > g_bt_dm_remote_sequence[find_index])) {
                g_bt_dm_remote_sequence[index]--;
            }
        }
        sequence = (sequence > max_seq) ? max_seq : sequence;
    } else {
        for (index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++) {
            if (g_bt_dm_remote_sequence[index] && (g_bt_dm_remote_sequence[index] >= sequence) &&
                (g_bt_dm_remote_sequence[index] < g_bt_dm_remote_sequence[find_index])) {
                g_bt_dm_remote_sequence[index]++;
            }
        }
    }
    g_bt_dm_remote_sequence[find_index] = sequence;
    bt_device_manager_db_mutex_give();
    bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO);
#ifdef MT7933_SUPPORT_NVKEY_NON_BLOCKING
    bt_device_manager_db_flush(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO, BT_DEVICE_MANAGER_DB_FLUSH_NON_BLOCK);
#endif
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_device_manager_remote_top(bt_bd_addr_t addr)
{
    return bt_device_manager_remote_set_seq_num(addr, 1);
}

bt_bd_addr_t *bt_device_manager_remote_get_dev_by_seq_num(uint8_t sequence)
{
    bt_device_manager_db_remote_info_t *temp_remote = &g_bt_dm_remote_list_cnt[0];
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Get dev by seq num %d", 1, sequence);
    for (uint8_t index = 0; sequence && index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (sequence == g_bt_dm_remote_sequence[index]) {
            return &(temp_remote->address);
        }
    }
    return NULL;
}

uint32_t bt_device_manager_remote_get_paired_num(void)
{
    uint32_t ret = 0;
    bt_device_manager_db_remote_info_t *temp_remote = &g_bt_dm_remote_list_cnt[0];
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (g_bt_dm_remote_sequence[index] && (temp_remote->info_valid_flag & BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED)) {
            ret++;
        }
    }
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Get paiared num = 0x%x", 1, ret);
    return ret;
}

void bt_device_manager_remote_get_paired_list(bt_device_manager_paired_infomation_t *info, uint32_t *read_count)
{
    uint32_t count = 0;
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Get Paired list", 0);
    if (NULL == info || *read_count == 0) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Wrong param info: %p, read_count: %d", 2, info, *read_count);
        return;
    }
    bt_device_manager_db_remote_paired_info_t *temp = pvPortMalloc(sizeof(bt_device_manager_db_remote_paired_info_t));
    if (NULL == temp) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Alloc fail.", 0);
        return;
    }
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++) {
        if (count < *read_count) {
            if (BT_STATUS_SUCCESS == bt_device_manager_remote_find_paired_info_by_seq_num(index + 1, temp)) {
                memcpy(info[count].address, temp->paired_key.address, sizeof(bt_bd_addr_t));
                memcpy(info[count].name, temp->name, sizeof(info[count].name));
                count++;
            }
        } else {
            break;
        }
    }
    vPortFree((void *)temp);
    *read_count = count;

    if (count == 0) {
        bt_dmgr_report_id("[BT_DM][REMOTE][I] No data in paired list", 0);
    }

}

bt_status_t bt_device_manager_remote_find_paired_info_by_seq_num(uint8_t sequence, bt_device_manager_db_remote_paired_info_t *info)
{
    bt_device_manager_db_remote_info_t *temp_remote = &g_bt_dm_remote_list_cnt[0];
    //bt_dmgr_report_id("[BT_DM][REMOTE][I] Find paired info by sequence num %d", 1, sequence);
    if (NULL == info) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Find paired info buffer is null", 0);
        return BT_STATUS_FAIL;
    }
    for (uint8_t index = 0; sequence && index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (g_bt_dm_remote_sequence[index] == sequence) {
            if (temp_remote->info_valid_flag & BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED) {
                memcpy(info->name, temp_remote->paired_info.name, BT_GAP_MAX_DEVICE_NAME_LENGTH);
                memcpy(info->paired_key.address, temp_remote->address, sizeof(bt_bd_addr_t));
                memcpy(info->paired_key.key, temp_remote->paired_info.key, sizeof(bt_key_t));
                info->paired_key.key_type = temp_remote->paired_info.key_type;
                bt_dmgr_report_id("[BT_DM][REMOTE][I] Find paired info at seq (%d)", 1, sequence);
                return BT_STATUS_SUCCESS;
            }
            break;
        }
    }
    return BT_STATUS_FAIL;
}

static bt_status_t bt_device_manager_remote_info_update(bt_bd_addr_t addr, bt_device_manager_remote_info_mask_t type, void *data)
{
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Update info type 0x%x", 1, type);
    uint8_t temp_addr[6] = {0x00};
    if (0 == BT_DEVICE_MANAGER_MAX_PAIR_NUM) {
        return BT_STATUS_FAIL;
    }
    if (NULL == data || !memcmp(&temp_addr, (uint8_t *)addr, sizeof(bt_bd_addr_t))) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Update info prameter invalid", 0);
        return BT_STATUS_FAIL;
    }
    uint8_t index = 0;
    uint8_t item_index = 0xFF;
    bool flush_at_once = false;
    bt_device_manager_db_remote_info_t *temp_remote = &g_bt_dm_remote_list_cnt[0];
    bt_device_manager_db_remote_info_t *find_remote = temp_remote;
    bt_device_manager_db_mutex_take();
    for (index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (g_bt_dm_remote_sequence[index] && !memcmp(temp_remote->address, addr, sizeof(bt_bd_addr_t))) {
            find_remote = temp_remote;
            item_index = index;
            break;
        } else if (item_index == 0xFF && (!g_bt_dm_remote_sequence[index] || BT_DEVICE_MANAGER_MAX_PAIR_NUM == g_bt_dm_remote_sequence[index])) {
            find_remote = temp_remote;
            item_index = index;
        }
    }
    if (BT_DEVICE_MANAGER_MAX_PAIR_NUM == index) {
        bt_dmgr_report_id("[BT_DM][REMOTE][I] New addr %02x:%02x:%02x:%02x:%02x:%02x, index %d, index sequence %d",
                          8, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], item_index, g_bt_dm_remote_sequence[item_index]);
        if (g_bt_dm_remote_sequence[item_index]) {
            bt_dmgr_report_id("[BT_DM][REMOTE][I] New need delete device %02x:%02x:%02x:%02x:%02x:%02x", 6,
                              find_remote->address[5], find_remote->address[4], find_remote->address[3],
                              find_remote->address[2], find_remote->address[1], find_remote->address[0]);
        }

        memcpy(find_remote->address, addr, sizeof(bt_bd_addr_t));
        find_remote->info_valid_flag = type;
        g_bt_dm_remote_sequence[item_index] = 1;
        for (index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++) {
            if (index != item_index && g_bt_dm_remote_sequence[index]) {
                g_bt_dm_remote_sequence[index]++;
            }
        }
        bt_device_manager_db_mutex_give();
        flush_at_once = true;
        bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO);
#ifdef MT7933_SUPPORT_NVKEY_NON_BLOCKING
        bt_device_manager_db_flush(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO, BT_DEVICE_MANAGER_DB_FLUSH_NON_BLOCK);
#endif
        bt_device_manager_db_mutex_take();
    } else {
        find_remote->info_valid_flag |= type;
    }
    switch (type) {
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED: {
                bt_device_manager_db_remote_paired_info_t *info = (void *)data;
                flush_at_once = true;
                memcpy(find_remote->paired_info.name, info->name, BT_GAP_MAX_DEVICE_NAME_LENGTH);
                memcpy(find_remote->paired_info.key, info->paired_key.key, sizeof(bt_key_t));
                find_remote->paired_info.key_type = info->paired_key.key_type;
                break;
            }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_VERSION: {
                bt_device_manager_db_remote_version_info_t *info = (void *)data;
                memcpy(&(find_remote->version_info), info, sizeof(bt_device_manager_db_remote_version_info_t));
                break;
            }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PROFILE: {
                bt_device_manager_db_remote_profile_info_t *info = (void *)data;
                memcpy(&(find_remote->profile_info), info, sizeof(bt_device_manager_db_remote_profile_info_t));
                break;
            }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PNP: {
                bt_device_manager_db_remote_pnp_info_t *info = (void *)data;
                find_remote->pnp_info.product_id = info->product_id;
                find_remote->pnp_info.vender_id = info->vender_id;
                break;
            }
        default:
            bt_device_manager_db_mutex_give();
            bt_dmgr_report_id("[BT_DM][REMOTE][E] Update info type is invalid", 0);
            return BT_STATUS_FAIL;
    }
    bt_device_manager_db_mutex_give();
    bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + item_index);
    if (flush_at_once == true) {
#ifdef MT7933_SUPPORT_NVKEY_NON_BLOCKING
        bt_device_manager_db_flush(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + item_index, BT_DEVICE_MANAGER_DB_FLUSH_NON_BLOCK);
#endif
    }
    return BT_STATUS_SUCCESS;
}

static bt_status_t bt_device_manager_remote_info_find(bt_bd_addr_t addr, bt_device_manager_remote_info_mask_t type, void *data)
{
    //bt_dmgr_report_id("[BT_DM][REMOTE][I] Find find info type 0x%x", 1, type);
    if (0 == BT_DEVICE_MANAGER_MAX_PAIR_NUM) {
        return BT_STATUS_FAIL;
    }
    if (NULL == data) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Find info buffer is null", 0);
        return BT_STATUS_FAIL;
    }
    bt_device_manager_db_remote_info_t *temp_remote = &g_bt_dm_remote_list_cnt[0];
    bt_device_manager_db_remote_info_t *find_remote = NULL;

    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (g_bt_dm_remote_sequence[index] && !memcmp(temp_remote->address, addr, sizeof(bt_bd_addr_t)) &&
            (temp_remote->info_valid_flag & type)) {
            find_remote = temp_remote;
            break;
        }
    }
    if (NULL == find_remote) {
        bt_dmgr_report_id("[BT_DM][REMOTE][I] Find info fail: flag = 0x%x, type=0x%x, %02x-%02x-%02x-%02x-%02x-%02x",
                          8, temp_remote->info_valid_flag, type, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

        return BT_STATUS_FAIL;
    }
    switch (type) {
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED: {
                bt_device_manager_db_remote_paired_info_t *info = (void *)data;
                memcpy(info->name, find_remote->paired_info.name, BT_GAP_MAX_DEVICE_NAME_LENGTH);
                memcpy(info->paired_key.address, find_remote->address, sizeof(bt_bd_addr_t));
                memcpy(info->paired_key.key, find_remote->paired_info.key, sizeof(bt_key_t));
                info->paired_key.key_type = find_remote->paired_info.key_type;
                break;
            }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_VERSION: {
                bt_device_manager_db_remote_version_info_t *info = (void *)data;
                memcpy(info, &(find_remote->version_info), sizeof(bt_device_manager_db_remote_version_info_t));
                break;
            }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PROFILE: {
                bt_device_manager_db_remote_profile_info_t *info = (void *)data;
                memcpy(info, &(find_remote->profile_info), sizeof(bt_device_manager_db_remote_profile_info_t));
                break;
            }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PNP: {
                bt_device_manager_db_remote_pnp_info_t *info = (void *)data;
                info->product_id = find_remote->pnp_info.product_id;
                info->vender_id = find_remote->pnp_info.vender_id;
                break;
            }
        default:
            bt_dmgr_report_id("[BT_DM][REMOTE][E] find info type is invalid", 0);
            return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_device_manager_remote_find_paired_info(bt_bd_addr_t addr, bt_device_manager_db_remote_paired_info_t *info)
{
    return bt_device_manager_remote_info_find(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED, (void *)info);
}

bt_status_t bt_device_manager_remote_update_paired_info(bt_bd_addr_t addr, bt_device_manager_db_remote_paired_info_t *info)
{
    for (uint32_t index = 0; index < BT_DEVICE_MANAGER_TEMP_BEFORE_PAIRING_LIST_CNT; index++) {
        if (!memcmp(addr, g_bt_dm_temp_before_pairing_cnt[index].address, sizeof(bt_bd_addr_t))) {
            bt_device_manager_remote_info_update(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PNP, (void *) & (g_bt_dm_temp_before_pairing_cnt[index].pnp_info));
            memset(&g_bt_dm_temp_before_pairing_cnt[index], 0, sizeof(g_bt_dm_temp_before_pairing_cnt[0]));
        }
        if (!memcmp(addr, g_bt_dm_temp_cod_set_cnt[index].address, sizeof(bt_bd_addr_t))) {
            bt_device_manager_db_mutex_take();
            bt_device_manager_db_remote_info_t *temp_remote = &g_bt_dm_remote_list_cnt[0];
            for (uint8_t index_2 = 0; index_2 < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index_2++, temp_remote++) {
                if (!memcmp(temp_remote->address, addr, sizeof(bt_bd_addr_t))) {
                    temp_remote->cod = g_bt_dm_temp_cod_set_cnt[index].cod;
                }
            }
            bt_device_manager_db_mutex_give();
            memset(&g_bt_dm_temp_cod_set_cnt[index], 0, sizeof(g_bt_dm_temp_cod_set_cnt[0]));
        }
    }
    return bt_device_manager_remote_info_update(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED, (void *)info);
}

bt_status_t bt_device_manager_remote_find_version_info(bt_bd_addr_t addr, bt_device_manager_db_remote_version_info_t *info)
{
    return bt_device_manager_remote_info_find(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_VERSION, (void *)info);
}

bt_status_t bt_device_manager_remote_update_version_info(bt_bd_addr_t addr, bt_device_manager_db_remote_version_info_t *info)
{
    return bt_device_manager_remote_info_update(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_VERSION, (void *)info);
}

bt_status_t bt_device_manager_remote_find_profile_info(bt_bd_addr_t addr, bt_device_manager_db_remote_profile_info_t *info)
{
    return bt_device_manager_remote_info_find(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PROFILE, (void *)info);
}

bt_status_t bt_device_manager_remote_update_profile_info(bt_bd_addr_t addr, bt_device_manager_db_remote_profile_info_t *info)
{
    bt_status_t status = BT_STATUS_SUCCESS;
    bt_device_manager_db_remote_paired_info_t *temp_info = pvPortMalloc(sizeof(bt_device_manager_db_remote_paired_info_t));
    if (NULL == temp_info) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Alloc fail.", 0);
        return BT_STATUS_FAIL;
    }
    status = bt_device_manager_remote_find_paired_info((uint8_t *)addr, temp_info);
    vPortFree((void *)temp_info);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }
    return bt_device_manager_remote_info_update(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PROFILE, (void *)info);
}

uint32_t bt_device_manager_remote_find_cod(bt_bd_addr_t addr)
{
    for (uint32_t index = 0; index < BT_DEVICE_MANAGER_TEMP_BEFORE_PAIRING_LIST_CNT; index++) {
        if (!memcmp(addr, g_bt_dm_temp_cod_set_cnt[index].address, sizeof(bt_bd_addr_t))) {
            return g_bt_dm_temp_cod_set_cnt[index].cod;
        }
    }
    bt_device_manager_db_remote_info_t *temp_remote = &g_bt_dm_remote_list_cnt[0];
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (g_bt_dm_remote_sequence[index] && !memcmp(temp_remote->address, addr, sizeof(bt_bd_addr_t))) {
            return temp_remote->cod;
        }
    }
    return 0;
}

bt_status_t bt_device_manager_remote_update_cod(bt_bd_addr_t addr, uint32_t cod)
{
    bt_bd_addr_t temp_addr = {0};
    for (uint32_t index = 0; index < BT_DEVICE_MANAGER_TEMP_BEFORE_PAIRING_LIST_CNT; index++) {
        if (!memcmp(temp_addr, g_bt_dm_temp_cod_set_cnt[index].address, sizeof(bt_bd_addr_t))) {
            memcpy(g_bt_dm_temp_cod_set_cnt[index].address, addr, sizeof(bt_bd_addr_t));
            g_bt_dm_temp_cod_set_cnt[index].cod = cod;
            break;
        }
        if (BT_DEVICE_MANAGER_TEMP_BEFORE_PAIRING_LIST_CNT - 1 == index) {
            bt_dmgr_report_id("[BT_DM][REMOTE][E] Device addr:%02x:%02x:%02x:%02x:%02x:%02x temp context will lost", 6,
                              g_bt_dm_temp_cod_set_cnt[index].address[5], g_bt_dm_temp_cod_set_cnt[index].address[4],
                              g_bt_dm_temp_cod_set_cnt[index].address[3], g_bt_dm_temp_cod_set_cnt[index].address[2],
                              g_bt_dm_temp_cod_set_cnt[index].address[1], g_bt_dm_temp_cod_set_cnt[index].address[0]);
            memcpy(g_bt_dm_temp_cod_set_cnt[index].address, addr, sizeof(bt_bd_addr_t));
            g_bt_dm_temp_cod_set_cnt[index].cod = cod;
        }
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_device_manager_remote_find_pnp_info(bt_bd_addr_t addr, bt_device_manager_db_remote_pnp_info_t *info)
{
    if (NULL == info) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Find info buffer is null", 0);
        return BT_STATUS_FAIL;
    }
    for (uint32_t index = 0; index < BT_DEVICE_MANAGER_TEMP_BEFORE_PAIRING_LIST_CNT; index++) {
        if (!memcmp(addr, g_bt_dm_temp_before_pairing_cnt[index].address, sizeof(bt_bd_addr_t))) {
            info->product_id = g_bt_dm_temp_before_pairing_cnt[index].pnp_info.product_id;
            info->vender_id = g_bt_dm_temp_before_pairing_cnt[index].pnp_info.vender_id;
            return BT_STATUS_SUCCESS;
        }
    }
    return bt_device_manager_remote_info_find(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PNP, (void *)info);
}

bt_status_t bt_device_manager_remote_update_pnp_info(bt_bd_addr_t addr, bt_device_manager_db_remote_pnp_info_t *info)
{
    bt_bd_addr_t temp_addr = {0};
    if (NULL == info) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Update info buffer is null", 0);
        return BT_STATUS_FAIL;
    }
    for (uint32_t index = 0; index < BT_DEVICE_MANAGER_TEMP_BEFORE_PAIRING_LIST_CNT; index++) {
        if (!memcmp(temp_addr, g_bt_dm_temp_before_pairing_cnt[index].address, sizeof(bt_bd_addr_t))) {
            memcpy(g_bt_dm_temp_before_pairing_cnt[index].address, addr, sizeof(bt_bd_addr_t));
            g_bt_dm_temp_before_pairing_cnt[index].pnp_info.product_id = info->product_id;
            g_bt_dm_temp_before_pairing_cnt[index].pnp_info.vender_id = info->vender_id;
            break;
        }
        if (BT_DEVICE_MANAGER_TEMP_BEFORE_PAIRING_LIST_CNT - 1 == index) {
            bt_dmgr_report_id("[BT_DM][REMOTE][E] Device addr:%02x:%02x:%02x:%02x:%02x:%02x temp context will lost", 6,
                              g_bt_dm_temp_before_pairing_cnt[index].address[5], g_bt_dm_temp_before_pairing_cnt[index].address[4],
                              g_bt_dm_temp_before_pairing_cnt[index].address[3], g_bt_dm_temp_before_pairing_cnt[index].address[2],
                              g_bt_dm_temp_before_pairing_cnt[index].address[1], g_bt_dm_temp_before_pairing_cnt[index].address[0]);
            memcpy(g_bt_dm_temp_before_pairing_cnt[index].address, addr, sizeof(bt_bd_addr_t));
            g_bt_dm_temp_before_pairing_cnt[index].pnp_info.product_id = info->product_id;
            g_bt_dm_temp_before_pairing_cnt[index].pnp_info.vender_id = info->vender_id;
        }
    }
    return BT_STATUS_SUCCESS;
    //return bt_device_manager_remote_info_update(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PNP, (void*)info);
}


