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

#include <string.h>
#include "syslog.h"
#include <stdlib.h>

#include "hal_trng.h"
#include "bt_system.h"
#include "bt_debug.h"

#include "bt_device_manager_le_config.h"
#include "bt_device_manager_le.h"
#include "bt_device_manager.h"
#include "bt_device_manager_internal.h"
#include "bt_callback_manager.h"

#ifdef MTK_NVDM_ENABLE
#include "nvdm.h"
#endif /* #ifdef MTK_NVDM_ENABLE */


#ifdef MTK_NVDM_ENABLE
#define NVDM_GROUP_BT_DM "BT_DM"
/*should not over 20bytes at this prefix string*/
#define NVDM_BT_DM_ITEM_BONDED_INFO_PREFIX  "BT_DM_Info[%02x]"
#endif /* #ifdef MTK_NVDM_ENABLE */

log_create_module(BT_DM, PRINT_LEVEL_INFO);

#define BT_ADDR_TYPE_UNKNOW (0xFF)

/**
 *  @brief Connection Structure, internal use.
 */
typedef struct {
    bt_handle_t                             connection_handle; /**< Connection handle. */
    bt_addr_t                               peer_addr; /**< peer addr. */
    bt_device_manager_le_connection_param_t connection_params; /**< Connection parameters. */
} bt_device_manager_le_connection_struct_t;

typedef struct {
    bool in_use;
    bt_device_manager_le_bonded_event_callback callback;
} ble_dm_callback_node_t;

typedef enum {
    BLE_DM_RSL_MODE_DISABLED,
    BLE_DM_RSL_MODE_ADD_ONE, /**< to add just one RPA data */
    BLE_DM_RSL_MODE_ADD_ALL, /**< to add all RPA data in the bonded info */
    BLE_DM_RSL_MODE_ENABLED,
} ble_dm_rsl_mode_e;

typedef struct {
    uint8_t add_index;      /**< indicate adding index while adding all */
    uint8_t list_cnt;       /**< indicate total number of added RPA */
    ble_dm_rsl_mode_e mode; /**< rpa operation mode */
} ble_dm_rsl_ctrl_t;

#define BT_DM_LE_BOND_INFO_VERSION     (0x11)

#define BT_DM_LE_FLAG_NONE             (0x00)
#define BT_DM_LE_FLAG_NOT_SYNC         (0x01)
typedef uint8_t bt_dm_le_flag_t;
BT_PACKED(
typedef struct {
    bt_device_manager_le_bonded_info_t  bond_info;
    uint8_t         version;
    bt_dm_le_flag_t flag;
    uint8_t         reserve[5];
}) bt_device_manager_le_bonded_info_ext_t;

static ble_dm_callback_node_t ble_dm_cb_list[BT_DEVICE_MANAGER_LE_CB_MAX_NUM] = {{0}};

static uint8_t g_next_resolved = 0;
static bool g_next_rsl_run_flag = false;
static ble_dm_rsl_ctrl_t g_ble_dm_rsl_ctrl;
static bool g_nvram_read_flag = false;
static bool bt_dm_le_initialized = false;
static bool bt_dm_clear_flag = false;
static const bt_addr_t default_bt_addr = {
    .type = BT_ADDR_TYPE_UNKNOW,
    .addr = {0}
};

bt_device_manager_le_connection_struct_t dm_info[BT_DEVICE_MANAGER_LE_CONNECTION_MAX] = {{0}};
bt_device_manager_le_bonded_info_t bonded_info[BT_DEVICE_MANAGER_LE_BONDED_MAX] = {0};

static bt_dm_le_flag_t bond_info_flag[BT_DEVICE_MANAGER_LE_BONDED_MAX] = {0x00};
#define BT_DM_LE_SET_FLAG(index,flag)         (bond_info_flag[index] |= flag)
#define BT_DM_LE_CANCEL_FLAG(index,flag)      (bond_info_flag[index] &= ~flag)
#define BT_DM_LE_CLEAR_FLAG(index)            (bond_info_flag[index] &= BT_DM_LE_FLAG_NONE)
#define BT_DM_LE_GET_FLAG(index)              (bond_info_flag[index])


/** default secure configuration. */
static bool g_sc_only_default = false;
static bt_gap_le_local_config_req_ind_t g_local_config_default;
static bt_gap_le_smp_pairing_config_t g_pairing_config_req_default = {
    .io_capability = BT_GAP_LE_SMP_NO_INPUT_NO_OUTPUT,
    .auth_req = BT_GAP_LE_SMP_AUTH_REQ_NO_BONDING,
    .maximum_encryption_key_size = 16,
};
static bt_gap_le_local_key_t g_local_key_req_default = {
    .encryption_info = {{0}},
    .master_id = {0},
    .identity_info = {{0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x19, 0x28, 0x55, 0x33, 0x68, 0x33, 0x56, 0xde}},
    .signing_info = {{0}}
};


static void bt_device_manager_le_reset_bonded_infos(void);
static void bt_device_manager_le_reset_connection_infos(void);
static void bt_device_manager_le_save_connection_params(void *buff);
static void bt_device_manager_le_delete_connection_params(void *buff);
static void bt_device_manager_le_event_callback_register(void);
//static bt_status_t bt_device_manager_le_add_resolving_list(uint8_t index);
static void bt_device_manager_le_store_bonded_info_to_nvdm(uint8_t index);
static uint8_t bt_device_manager_le_get_index_by_address(bt_addr_t *address);
static void bt_device_manager_le_restore_bonded_info_from_nvdm(uint8_t index);
static void bt_device_manager_le_update_current_conn_interval(void *conn_params);
static bt_gap_le_local_config_req_ind_t *bt_device_manager_le_get_local_config(void);
static bt_status_t bt_device_manager_le_get_pairing_config(bt_gap_le_bonding_start_ind_t *ind);
static bt_gap_le_bonding_info_t *bt_device_manager_le_get_bonding_info(const bt_addr_t remote_addr);
static bt_status_t bt_device_manager_le_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff);
bt_device_manager_le_bonded_info_t *bt_device_manager_le_get_bonded_info_by_handle(bt_handle_t conn_handle);
static bt_device_manager_le_bonded_info_t *bt_device_manager_le_get_or_new_bonded_info(const bt_addr_t *peer_addr, bool new_flag);

/* A debug API to show all bonding info
 *  Note: Use a shorter naming to prevent log lose
 */
static void _ble_dm_show_bonding_info(void)
{
    uint8_t i;

    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        LOG_MSGID_I(BT_DM, "bonded_info[%d] ===> bt_addr(%02x) %x:%x:%x:%x:%x:%x", 2, i,
                    bonded_info[i].bt_addr.type,
                    bonded_info[i].bt_addr.addr[5], bonded_info[i].bt_addr.addr[4],
                    bonded_info[i].bt_addr.addr[3], bonded_info[i].bt_addr.addr[2],
                    bonded_info[i].bt_addr.addr[1], bonded_info[i].bt_addr.addr[0]);
        LOG_MSGID_I(BT_DM, "bonded_info[%d].info.id_addr(%02x) %x:%x:%x:%x:%x:%x", 2, i,
                    bonded_info[i].info.identity_addr.address.type,
                    bonded_info[i].info.identity_addr.address.addr[5], bonded_info[i].info.identity_addr.address.addr[4],
                    bonded_info[i].info.identity_addr.address.addr[3], bonded_info[i].info.identity_addr.address.addr[2],
                    bonded_info[i].info.identity_addr.address.addr[1], bonded_info[i].info.identity_addr.address.addr[0]);
        LOG_MSGID_I(BT_DM, "bonded_info[%d].info.s_mode 0x%x, irk = 0x%x %x %x ..", 2, i,
                    bonded_info[i].info.key_security_mode,
                    bonded_info[i].info.identity_info.irk[0],
                    bonded_info[i].info.identity_info.irk[1],
                    bonded_info[i].info.identity_info.irk[2]);
    }
}

static void _ble_dm_show_conn_info(void)
{
    uint8_t i;

    for (i = 0; i < BT_DEVICE_MANAGER_LE_CONNECTION_MAX; i++) {
        LOG_MSGID_I(BT_DM, "conn_info[%d] ===> hdl: 0x%x, addr(%02x) %x:%x:%x:%x:%x:%x", 9, i,
                    dm_info[i].connection_handle, dm_info[i].peer_addr.type,
                    dm_info[i].peer_addr.addr[5], dm_info[i].peer_addr.addr[4],
                    dm_info[i].peer_addr.addr[3], dm_info[i].peer_addr.addr[2],
                    dm_info[i].peer_addr.addr[1], dm_info[i].peer_addr.addr[0]);
    }
}

static void _ble_dm_update_conn_info_addr(bt_handle_t conn_hdl, bt_addr_t *addr)
{
    uint8_t i;

    for (i = 0; i < BT_DEVICE_MANAGER_LE_CONNECTION_MAX; i++) {
        if (dm_info[i].connection_handle == conn_hdl &&
            dm_info[i].peer_addr.type != addr->type) {
            LOG_MSGID_I(BT_DM, "update conn_info[%d] hdl: 0x%x, addr(%02x)%x%x%x%x%x%x->(%02x)%x%x%x%x%x%x",
                        16, i, dm_info[i].connection_handle, dm_info[i].peer_addr.type,
                        dm_info[i].peer_addr.addr[5], dm_info[i].peer_addr.addr[4],
                        dm_info[i].peer_addr.addr[3], dm_info[i].peer_addr.addr[2],
                        dm_info[i].peer_addr.addr[1], dm_info[i].peer_addr.addr[0],
                        addr->type, addr->addr[5], addr->addr[4], addr->addr[3],
                        addr->addr[2], addr->addr[1], addr->addr[0]);

            memcpy(&dm_info[i].peer_addr, addr, sizeof(bt_addr_t));
        }

    }

}

static void bt_device_manager_le_bonded_event_cb(bt_device_manager_le_bonded_event_t event, bt_addr_t *address)
{
    uint8_t i = 0;
    for (i = 0; i < BT_DEVICE_MANAGER_LE_CB_MAX_NUM; i++) {
        if (ble_dm_cb_list[i].in_use && ble_dm_cb_list[i].callback != NULL) {
            ble_dm_cb_list[i].callback(event, address);
        }
    }
}

static bt_status_t bt_device_manager_le_cb_register_int(bt_device_manager_le_bonded_event_callback callback)
{
    uint8_t i = 0;
    bt_status_t status = 0;
    for (i = 0; i < BT_DEVICE_MANAGER_LE_CB_MAX_NUM; i++) {
        if (!ble_dm_cb_list[i].in_use) {
            ble_dm_cb_list[i].callback = callback;
            ble_dm_cb_list[i].in_use = true;
            break;
        }
    }
    if (i == BT_DEVICE_MANAGER_LE_CB_MAX_NUM) {
        LOG_MSGID_I(BT_DM, "all are in use, please extend the value of BT_DEVICE_MANAGER_LE_CB_MAX_NUM!\r\n", 0);
        status = BT_STATUS_FAIL;
    }
    return status;
}

static bt_status_t bt_device_manager_le_cb_deregister_int(bt_device_manager_le_bonded_event_callback callback)
{
    uint8_t i = 0;
    bt_status_t status = 0;

    for (i = 0; i < BT_DEVICE_MANAGER_LE_CB_MAX_NUM; i++) {
        if (ble_dm_cb_list[i].in_use && ble_dm_cb_list[i].callback == callback) {
            ble_dm_cb_list[i].callback = NULL;
            ble_dm_cb_list[i].in_use = false;
            break;
        }
    }
    if (i == BT_DEVICE_MANAGER_LE_CB_MAX_NUM) {
        LOG_MSGID_I(BT_DM, "deregister fail, because of not find the callback\r\n", 0);
        status = BT_STATUS_FAIL;
    }
    return status;
}

void bt_device_manager_le_show_bonding_info(void)
{
    _ble_dm_show_bonding_info();
}

void bt_device_manager_le_show_conn_info(void)
{
    _ble_dm_show_conn_info();
}

void bt_device_manager_le_init(void)
{
    if (bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Others application had initialized BT DM LE module\r\n", 0);
        return;
    } else {
        bt_device_manager_le_reset_bonded_infos();
        bt_device_manager_le_reset_connection_infos();
        bt_device_manager_le_event_callback_register();
        if (!g_nvram_read_flag) {
            g_nvram_read_flag = true;
            bt_device_manager_le_restore_bonded_info_from_nvdm(0);
        }
        bt_dm_le_initialized = true;
        LOG_MSGID_I(BT_DM, "bt_device_manager_le_init done\r\n", 0);
    }
}

static void bt_device_manager_le_event_callback_register(void)
{
    LOG_MSGID_I(BT_DM, "bt_device_manager_le_event_callback_register\r\n", 0);
    bt_callback_manager_register_callback(bt_callback_type_gap_le_get_bonding_info, 0, (void *)bt_device_manager_le_get_bonding_info);
    bt_callback_manager_register_callback(bt_callback_type_gap_le_get_local_cofig, 0, (void *)bt_device_manager_le_get_local_config);
    bt_callback_manager_register_callback(bt_callback_type_gap_le_get_pairing_config, 0, (void *)bt_device_manager_le_get_pairing_config);
    bt_callback_manager_register_callback(bt_callback_type_app_event, MODULE_MASK_SYSTEM | MODULE_MASK_GAP, (void *)bt_device_manager_le_event_callback);
}

static bt_status_t bt_device_manager_le_add_dev_2_resolving_list(const bt_gap_le_bonding_info_t *bonded_info)
{
    bt_status_t st = BT_STATUS_FAIL;
    if ((BT_ADDR_TYPE_UNKNOW != bonded_info->identity_addr.address.type) &&
        (0 != memcmp(&(default_bt_addr.addr), &(bonded_info->identity_addr.address.addr), BT_BD_ADDR_LEN))) {
        bt_hci_cmd_le_add_device_to_resolving_list_t dev;
        dev.peer_identity_address = bonded_info->identity_addr.address;
        LOG_MSGID_I(BT_DM, "peer identity addr: %02X:%02X:%02X:%02X:%02X:%02X,type = %d", 7,
                    dev.peer_identity_address.addr[5],
                    dev.peer_identity_address.addr[4],
                    dev.peer_identity_address.addr[3],
                    dev.peer_identity_address.addr[2],
                    dev.peer_identity_address.addr[1],
                    dev.peer_identity_address.addr[0],
                    dev.peer_identity_address.type
                   );
        memcpy(dev.peer_irk, &(bonded_info->identity_info), sizeof(dev.peer_irk));
        memcpy(dev.local_irk, &(bonded_info->local_key.identity_info), sizeof(dev.local_irk));
        st = bt_gap_le_set_resolving_list(BT_GAP_LE_ADD_TO_RESOLVING_LIST, (void *)&dev);
        if (BT_STATUS_OUT_OF_MEMORY == st) {
            LOG_MSGID_I(BT_DM, "Add device to rsl Failed [OOM]", 0);
        }
    }
    if (BT_STATUS_SUCCESS != st && BT_STATUS_OUT_OF_MEMORY != st)
        LOG_MSGID_I(BT_DM, "add_dev_2_rsl fail status = 0x%4x", 1, st);
    return st;
}

static bt_status_t bt_device_manager_le_delete_dev_from_resolving_list(const bt_gap_le_bonding_info_t *bonded_info)
{
    bt_status_t st = BT_STATUS_SUCCESS;
    if ((BT_ADDR_TYPE_UNKNOW != bonded_info->identity_addr.address.type) &&
        (0 != memcmp(&(default_bt_addr.addr), &(bonded_info->identity_addr.address.addr), BT_BD_ADDR_LEN))) {
        bt_hci_cmd_le_remove_device_from_resolving_list_t dev;
        dev.peer_identity_address = bonded_info->identity_addr.address;
        g_ble_dm_rsl_ctrl.list_cnt --;
        st = bt_gap_le_set_resolving_list(BT_GAP_LE_REMOVE_FROM_RESOLVING_LIST, (void *)&dev);
        if (BT_STATUS_OUT_OF_MEMORY == st) {
            LOG_MSGID_I(BT_DM, "Delete device from Resolving List Failed [OOM]\r\n", 0);
        }
    }
    LOG_MSGID_I(BT_DM, "bt_device_manager_le_delete_dev_from_resolving_list status is 0x%4x\r\n", 1, st);
    return st;
}

static bt_status_t bt_device_manager_le_add_resolving_list_by_index(uint8_t index)
{
    LOG_MSGID_I(BT_DM, "[LE_DM] add_rsl at index(%d). mode(%d)\r\n", 2, index, g_ble_dm_rsl_ctrl.mode);

    if (g_ble_dm_rsl_ctrl.mode != BLE_DM_RSL_MODE_DISABLED &&
        g_ble_dm_rsl_ctrl.mode != BLE_DM_RSL_MODE_ENABLED) {
        LOG_MSGID_E(BT_DM, "[LE_DM] add_rsl not at correct mode(%d)\r\n", 1, g_ble_dm_rsl_ctrl.mode);
        return BT_STATUS_FAIL;
    }

    if (index > BT_DEVICE_MANAGER_LE_BONDED_MAX) {
        LOG_MSGID_E(BT_DM, "[LE_DM] index not in the range\r\n", 0);
        return BT_STATUS_FAIL;
    }

    g_ble_dm_rsl_ctrl.mode = BLE_DM_RSL_MODE_ADD_ONE;

    bt_device_manager_le_bonded_info_t *bond_info;
    bond_info = &(bonded_info[index]);
    if ((BT_ADDR_TYPE_UNKNOW != bond_info->info.identity_addr.address.type) &&
        (0 != memcmp(&(default_bt_addr.addr), &(bond_info->info.identity_addr.address.addr), BT_BD_ADDR_LEN))) {
        //update resolving list
        bt_device_manager_le_add_dev_2_resolving_list(&(bond_info->info));
        g_ble_dm_rsl_ctrl.list_cnt ++;
        g_ble_dm_rsl_ctrl.mode = BLE_DM_RSL_MODE_ENABLED;
        bt_gap_le_set_address_resolution_enable(1);
        LOG_MSGID_I(BT_DM, "[LE_DM] add_rsl success, index = %d (total = %d)\r\n", 1, index, g_ble_dm_rsl_ctrl.list_cnt);
        return BT_STATUS_SUCCESS;
    }

    LOG_MSGID_I(BT_DM, "[LE_DM] add_rsl fail\r\n", 0);
    return BT_STATUS_FAIL;
}


static bt_status_t bt_device_manager_le_add_resolving_list_all(void)
{
    if (g_ble_dm_rsl_ctrl.mode == BLE_DM_RSL_MODE_DISABLED ||
        g_ble_dm_rsl_ctrl.mode == BLE_DM_RSL_MODE_ENABLED) {
        g_ble_dm_rsl_ctrl.add_index = 0;
        g_ble_dm_rsl_ctrl.list_cnt = 0;
        g_ble_dm_rsl_ctrl.mode = BLE_DM_RSL_MODE_ADD_ALL;
    }

    if (g_ble_dm_rsl_ctrl.mode != BLE_DM_RSL_MODE_ADD_ALL) {
        LOG_MSGID_E(BT_DM, "[LE_DM] add_rsl_all Fail: Not in correct mode(%d)!!", 1, g_ble_dm_rsl_ctrl.mode);
        return BT_STATUS_FAIL;
    }

    uint8_t index = g_ble_dm_rsl_ctrl.add_index;
    uint8_t i;

    for (i = index; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        bt_device_manager_le_bonded_info_t *bond_info;
        bond_info = &(bonded_info[i]);

        index ++;
        if (0 == memcmp(&default_bt_addr, &(bond_info->bt_addr), sizeof(bt_addr_t))) {
            //LOG_MSGID_I(BT_DM, "[LE_DM] add_rsl_all ignore, i = %d\r\n", 1, i);
            //return BT_STATUS_FAIL;
        } else {
            if ((BT_ADDR_TYPE_UNKNOW != bond_info->info.identity_addr.address.type) &&
                (0 != memcmp(&(default_bt_addr.addr), &(bond_info->info.identity_addr.address.addr), BT_BD_ADDR_LEN))) {
                //update resolving list
                bt_device_manager_le_add_dev_2_resolving_list(&(bond_info->info));
                g_ble_dm_rsl_ctrl.list_cnt ++;
                //LOG_MSGID_I(BT_DM, "[LE_DM] add_rsl_all success, i = %d, %d\r\n", 2, i, index);
                break; //break and wait confirm from controller
            }
        }
    }

    //update index
    g_ble_dm_rsl_ctrl.add_index = index;

    if (index >= BT_DEVICE_MANAGER_LE_BONDED_MAX &&
        g_ble_dm_rsl_ctrl.list_cnt != 0) {
        //it means all addr has been added
        g_ble_dm_rsl_ctrl.mode = BLE_DM_RSL_MODE_ENABLED;
        LOG_MSGID_I(BT_DM, "[LE_DM] add_rsl_all all added (total = %d)", 1, g_ble_dm_rsl_ctrl.list_cnt);
        bt_gap_le_set_address_resolution_enable(1);
    }

    LOG_MSGID_I(BT_DM, "[LE_DM] add_rsl_all done index =%d", 1, index);
    return BT_STATUS_SUCCESS;
}


static bt_status_t bt_device_manager_le_bond_info_remove_by_index(uint8_t index)
{
    if (index > BT_DEVICE_MANAGER_LE_BONDED_MAX) {
        LOG_MSGID_I(BT_DM, "[DM][LE] Remove bonded info index over max = %d\r\n", 1, index);
        return BT_STATUS_FAIL;
    }

    bt_device_manager_le_bonded_info_t *p_bond_info = &bonded_info[index];
#ifdef MTK_BLE_GAP_SRV_ENABLE
    bt_handle_t connection_handle = bt_gap_le_srv_get_conn_handle_by_addr(&(p_bond_info->bt_addr), false);
    LOG_MSGID_I(BT_DM, "[DM][LE] Remove bonded info connection handle = %02x\r\n", 1, connection_handle);
    if (!(bt_gap_le_srv_get_link_attribute_by_handle(connection_handle) & BT_GAP_LE_SRV_LINK_ATTRIBUTE_NOT_NEED_RHO)) {
        bt_device_manager_le_bonded_event_cb(BT_DEVICE_MANAGER_LE_BONDED_REMOVE, &p_bond_info->bt_addr);
    } else {
        LOG_MSGID_I(BT_DM, "[DM][LE] not need sync bond info to agent\r\n", 0);
    }
#else /* #ifdef MTK_BLE_GAP_SRV_ENABLE */
    bt_device_manager_le_bonded_event_cb(BT_DEVICE_MANAGER_LE_BONDED_REMOVE, &p_bond_info->bt_addr);
#endif /* #ifdef MTK_BLE_GAP_SRV_ENABLE */
    bt_device_manager_le_delete_dev_from_resolving_list((&(p_bond_info->info)));
    memset(p_bond_info, 0x00, sizeof(bt_device_manager_le_bonded_info_t));
    p_bond_info->bt_addr.type = BT_ADDR_TYPE_UNKNOW;
    p_bond_info->info.identity_addr.address.type = BT_ADDR_TYPE_UNKNOW;
    BT_DM_LE_CLEAR_FLAG(index);
    LOG_MSGID_I(BT_DM, "[DM][LE] Remove bonded info by index  %d\r\n", 1, index);
    bt_device_manager_le_store_bonded_info_to_nvdm(index);
    return BT_STATUS_SUCCESS;
}

static bt_status_t bt_device_manager_le_bond_info_duplicate_remove(uint8_t index)
{
    uint8_t i = 0;
    bt_device_manager_le_bonded_info_t *new_bond_info = NULL;
    bt_device_manager_le_bonded_info_t *old_bond_info = NULL;
    new_bond_info = &(bonded_info[index]);
    //LOG_MSGID_I(BT_DM, "[DM][LE]bond info is duplicate index = %d\r\n", 1, index);
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        old_bond_info = &(bonded_info[i]);
        if ((0 == memcmp(&(new_bond_info->info.identity_addr.address.addr), &(old_bond_info->info.identity_addr.address.addr), BT_BD_ADDR_LEN)) &&
            (i != index) && (0 != memcmp(&(default_bt_addr.addr), &(old_bond_info->info.identity_addr.address.addr), BT_BD_ADDR_LEN))) {
            LOG_MSGID_I(BT_DM, "[DM][LE]rm duplicated info at 0x%02x (new = 0x%02x),addr: %02X:%02X:%02X:%02X:%02X:%02X,type %d", 9,
                        i, index,
                        old_bond_info->bt_addr.addr[5],
                        old_bond_info->bt_addr.addr[4],
                        old_bond_info->bt_addr.addr[3],
                        old_bond_info->bt_addr.addr[2],
                        old_bond_info->bt_addr.addr[1],
                        old_bond_info->bt_addr.addr[0],
                        old_bond_info->bt_addr.type
                       );
            bt_device_manager_le_bond_info_remove_by_index(i);
            break;
        }
    }
    //if (i == BT_DEVICE_MANAGER_LE_BONDED_MAX) {
    //    LOG_MSGID_I(BT_DM, "[DM][LE]bond info not have duplicate remove\r\n", 0);
    //}
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_device_manager_le_update_resolving_list(void)
{
    //bt_device_manager_le_add_resolving_list(0);
    bt_device_manager_le_add_resolving_list_all();
    return BT_STATUS_SUCCESS;
}

static bt_status_t bt_device_manager_le_write_nvdm(bt_device_manager_le_bonded_info_t *p_bond_info, uint8_t index)
{
#ifdef MTK_NVDM_ENABLE
    nvdm_status_t write_status;
    char data_item_name[20] = {0};
    snprintf((char *)data_item_name, sizeof(data_item_name), NVDM_BT_DM_ITEM_BONDED_INFO_PREFIX, (unsigned int)index);
    bt_device_manager_le_bonded_info_ext_t bond_info_nvdm;
    memcpy(&bond_info_nvdm.bond_info, p_bond_info, sizeof(bt_device_manager_le_bonded_info_t));
    bond_info_nvdm.version = BT_DM_LE_BOND_INFO_VERSION;
    bond_info_nvdm.flag = BT_DM_LE_GET_FLAG(index);
    LOG_MSGID_I(BT_DM, "[DM][LE] Write nvdm index = %d, flag = %02x (%s)\r\n", 3,
                index, bond_info_nvdm.flag, data_item_name);
    write_status = nvdm_write_data_item(NVDM_GROUP_BT_DM, (const char *)data_item_name, NVDM_DATA_ITEM_TYPE_RAW_DATA, (const uint8_t *)&bond_info_nvdm, sizeof(bt_device_manager_le_bonded_info_ext_t));
    if (write_status != NVDM_STATUS_OK) {
        LOG_MSGID_I(BT_DM, "Write nvdm fail, bonded_info[%d], write nvdm status = %d.\r\n", 2, index, write_status);
    }
    LOG_MSGID_I(BT_DM, "Write nvdm success, bonded_info[%d].\r\n", 1, index);
#else /* #ifdef MTK_NVDM_ENABLE */
    LOG_MSGID_E(BT_DM, "NVDM is not enabled at le write", 0);
#endif /* #ifdef MTK_NVDM_ENABLE */
    return BT_STATUS_SUCCESS;
}

static bt_status_t bt_device_manager_le_read_nvdm(bt_device_manager_le_bonded_info_t *p_bond_info, uint8_t index)
{
#ifdef MTK_NVDM_ENABLE
    nvdm_status_t read_status;
    char data_item_name[20] = {0};
    if (p_bond_info == NULL) {
        return BT_STATUS_FAIL;
    }
    uint32_t size = sizeof(bt_device_manager_le_bonded_info_ext_t);
    bt_device_manager_le_bonded_info_ext_t bond_info_nvdm;
    snprintf(data_item_name, sizeof(data_item_name), NVDM_BT_DM_ITEM_BONDED_INFO_PREFIX, (unsigned int)index);
    read_status = nvdm_read_data_item(NVDM_GROUP_BT_DM, (const char *)data_item_name, (uint8_t *)&bond_info_nvdm, &size);
    if (read_status != NVDM_STATUS_OK) {
        if (read_status == NVDM_STATUS_ITEM_NOT_FOUND) {
            LOG_MSGID_I(BT_DM, "[DM][LE] Read nvdm[%d] %s not found!\r\n", 2, index, data_item_name);
        } else
            LOG_MSGID_I(BT_DM, "[DM][LE] Read nvdm[%d] fail, %s, read nvdm status = %d.\r\n", 2, index, data_item_name, read_status);
        return BT_STATUS_FAIL;
    }
    memcpy(p_bond_info, &bond_info_nvdm.bond_info, sizeof(bt_device_manager_le_bonded_info_t));
    if (0 == memcmp(&default_bt_addr, &(p_bond_info->bt_addr), sizeof(default_bt_addr))) {
        //LOG_MSGID_I(BT_DM, "[DM][LE] bonded_info[%d] addr is NULL.\r\n", 1, index);
    }
    //MTK_David : reduce log
    /*LOG_MSGID_I(BT_DM, "[DM][LE] Read bond info[%d] type = %02x, addr: %02x-%02x-%02x-%02x-%02x-%02x \r\n", 8,
                index, p_bond_info->bt_addr.type,
                p_bond_info->bt_addr.addr[0], p_bond_info->bt_addr.addr[1],
                p_bond_info->bt_addr.addr[2], p_bond_info->bt_addr.addr[3],
                p_bond_info->bt_addr.addr[4], p_bond_info->bt_addr.addr[5]);*/
    /* MTK_Titan: To prevent disclousre too many information
        uint8_t *key_ptr = (uint8_t *)&p_bond_info->info.encryption_info.ltk;
        LOG_MSGID_I(BT_DM, "[DM][LE]local ltk:%x%x%x%x %x%x%x%x %x%x%x%x %x%x%x%x", 16,
                    key_ptr[0], key_ptr[1], key_ptr[2], key_ptr[3],
                    key_ptr[4], key_ptr[5], key_ptr[6], key_ptr[7],
                    key_ptr[8], key_ptr[9], key_ptr[10], key_ptr[11],
                    key_ptr[12], key_ptr[13], key_ptr[14], key_ptr[15]);
        LOG_MSGID_I(BT_DM, "[DM][LE] Read nvdm success, index = %d, version = %02x, flag = %02x.\r\n", 3,
                    index, bond_info_nvdm.version, bond_info_nvdm.flag);
    */
    if (bond_info_nvdm.version == BT_DM_LE_BOND_INFO_VERSION) {
        BT_DM_LE_SET_FLAG(index, bond_info_nvdm.flag);
    }
#else /* #ifdef MTK_NVDM_ENABLE */
    LOG_MSGID_E(BT_DM, "NVDM is not enabled at le read", 0);
#endif /* #ifdef MTK_NVDM_ENABLE */
    return BT_STATUS_SUCCESS;
}

static bt_status_t bt_device_manager_le_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{

    // remove to reduce log
    //if (msg != BT_GAP_LE_ADVERTISING_REPORT_IND)
    //    LOG_MSGID_I(BT_DM, "event callback, status(0x%04x), msg(0x%04x)\r\n", 2, status, msg);

    //if (BT_STATUS_SUCCESS != status)
    //    return BT_STATUS_SUCCESS;

    switch (msg) {
        case BT_POWER_ON_CNF: {
                //bt_device_manager_le_add_resolving_list(0);
                bt_device_manager_le_add_resolving_list_all();
            }
            break;
        case BT_POWER_OFF_CNF: {
                if (0 != g_next_resolved) {
                    g_next_resolved = 0;
                }
            }
            break;
        case BT_GAP_LE_SET_RESOLVING_LIST_CNF: {
                LOG_MSGID_I(BT_DM, "evt cb: set RPA cnf", 0);
                // Check to add next if still in adding all mode.
                if (g_ble_dm_rsl_ctrl.mode == BLE_DM_RSL_MODE_ADD_ALL)
                    bt_device_manager_le_add_resolving_list_all();

            }
            break;
        case BT_GAP_LE_CONNECT_IND: {
                //const bt_gap_le_connection_ind_t *connect_ind = (bt_gap_le_connection_ind_t *)buff;
                bt_device_manager_le_save_connection_params(buff);
                //LOG_MSGID_I(BT_DM, "connected, role = %d", 1, connect_ind->role);
            }
            break;
        case BT_GAP_LE_CONNECTION_UPDATE_IND:
            bt_device_manager_le_update_current_conn_interval(buff);
            break;
        case BT_GAP_LE_DISCONNECT_IND: {
                bt_hci_evt_disconnect_complete_t *disc_ind = (bt_hci_evt_disconnect_complete_t *)buff;
                bt_device_manager_le_bonded_info_t *info;
                bt_device_manager_le_delete_connection_params(buff);

                // If alloc a bonded info but not used, we should free it too
                info = bt_device_manager_le_get_bonded_info_by_handle(disc_ind->connection_handle);
                if (info &&
                    info->info.identity_addr.address.type == BT_ADDR_TYPE_UNKNOW) {
                    LOG_MSGID_I(BT_DM, "Free unused bonded info %s\n", 1, bt_debug_addr2str(&info->bt_addr));
                    bt_device_manager_le_remove_bonded_device(&info->bt_addr);
                }

            }
            break;
        case BT_GAP_LE_BONDING_COMPLETE_IND: {
                bt_gap_le_bonding_complete_ind_t *ind = (bt_gap_le_bonding_complete_ind_t *)buff;
                LOG_MSGID_I(BT_DM, "Bond complete, conn_handle(0x%04x)status (0x%04x)", 2, ind->handle, status);
                bt_device_manager_le_bonded_info_t *bond_info = bt_device_manager_le_get_bonded_info_by_handle(ind->handle);
                if (NULL != bond_info) {
                    uint8_t i = 255;
                    if (BT_STATUS_SUCCESS != status) {
                        if (0x06 == status) {// means key or pin missing
                            LOG_MSGID_I(BT_DM, "Bond fail because remote dev key missing, conn_handle(0x%04x)", 1, ind->handle);
                            bt_gap_le_bond(ind->handle, &g_pairing_config_req_default);
                        }
                        return BT_STATUS_SUCCESS;
                    }

                    //remove duplicated bond info before adding new one
                    i = bt_device_manager_le_get_index_by_address(&(bond_info->bt_addr));
                    bt_device_manager_le_bond_info_duplicate_remove(i);

#if 1
                    /* If we got IRK/Identity address from peer, we have to change
                     * bonding info's bd address; bt_device_manager_le_bonded_info_t.
                     */
                    if (BT_ADDR_TYPE_UNKNOW != bond_info->info.identity_addr.address.type) {
                        /* Because value of bonded_info->info.identity_addr.address_type is 0[Public Identity] or 1[Random Identity],
                         *but Identity address type were definied 2 or 3 in spec.
                         *We have to "+2" for synchronization.
                         */
                        _ble_dm_update_conn_info_addr(ind->handle, &(bond_info->info.identity_addr.address));

                        //bond_info->bt_addr = bond_info->info.identity_addr.address;
                        //update resolving list
                        if (BT_STATUS_SUCCESS == bt_device_manager_le_add_dev_2_resolving_list(&(bond_info->info))) {
                            //g_next_resolved ++;
                            g_next_rsl_run_flag = true;
                            bt_gap_le_set_address_resolution_enable(1);
                        }
                    }
#endif /* #if 1 */

                    //i = bt_device_manager_le_get_index_by_address(&(bond_info->bt_addr));
                    //bt_device_manager_le_bond_info_duplicate_remove(i);
                    if ((255 != i) && (bond_info->info.key_security_mode & BT_GAP_LE_SECURITY_BONDED_MASK)) {

                        bt_device_manager_le_write_nvdm(bond_info, i);
                    }
                    bt_device_manager_le_bonded_event_cb(BT_DEVICE_MANAGER_LE_BONDED_ADD, &(bond_info->bt_addr));
                }
            }
            break;
        default:
            break;
    }
    return BT_STATUS_SUCCESS;
}

static bt_gap_le_local_config_req_ind_t *bt_device_manager_le_get_local_config(void)
{
    LOG_MSGID_I(BT_DM, "bt_device_manager_le_get_local_config\r\n", 0);
    g_local_config_default.local_key_req = &g_local_key_req_default;
    g_local_config_default.sc_only_mode_req = g_sc_only_default;
    return &g_local_config_default;
}

static bt_status_t bt_device_manager_le_get_pairing_config(bt_gap_le_bonding_start_ind_t *ind)
{
    LOG_MSGID_I(BT_DM, "bt_device_manager_le_get_pairing_config, is_previously_bonded(%d)\r\n", 1, ind->is_previously_bonded);
    ind->pairing_config_req = g_pairing_config_req_default;
    return BT_STATUS_SUCCESS;
}

static bt_gap_le_bonding_info_t *bt_device_manager_le_get_bonding_info(const bt_addr_t remote_addr)
{
    //LOG_I(BT_DM, "%s: enter\r\n", __FUNCTION__);
    //return &(bt_device_manager_le_get_or_new_bonded_info(&remote_addr, true)->info);
    LOG_MSGID_I(BT_DM, "get bonding info address: %02X:%02X:%02X:%02X:%02X:%02X \r\n", 6,
                remote_addr.addr[5],
                remote_addr.addr[4],
                remote_addr.addr[3],
                remote_addr.addr[2],
                remote_addr.addr[1],
                remote_addr.addr[0]
               );
    bt_device_manager_le_bonded_info_t *dm_bonded_info = bt_device_manager_le_get_or_new_bonded_info(&remote_addr, true);
    if (dm_bonded_info) {
        return &(dm_bonded_info->info);
    }
    return NULL;
}

bt_gap_le_bonding_info_t *bt_device_manager_le_get_bonding_info_by_addr(bt_bd_addr_t *remote_addr)
{
    uint8_t i;
    bt_addr_t addr = {0};

    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Device bonded status check fail, please init BT DM LE first!\r\n", 0);
        return NULL;
    }
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        if (0 == memcmp(remote_addr, &(bonded_info[i].bt_addr.addr), sizeof(bt_bd_addr_t))) {
            memcpy(&addr, &(bonded_info[i].bt_addr), sizeof(bt_addr_t));
            LOG_MSGID_I(BT_DM, "bt_device_manager_le_get_bonding_info_by_addr use bt_addr.\r\n", 0);
            return bt_device_manager_le_get_bonding_info((const bt_addr_t)addr);
        } else if (0 == memcmp(remote_addr, &(bonded_info[i].info.identity_addr.address.addr), sizeof(bt_bd_addr_t))) {
            memcpy(&addr, &(bonded_info[i].info.identity_addr.address), sizeof(bt_addr_t));
            LOG_MSGID_I(BT_DM, "bt_device_manager_le_get_bonding_info_by_addr use identity addr.\r\n", 0);
            return bt_device_manager_le_get_bonding_info((const bt_addr_t)addr);
        }
    }
    return NULL;
}

bt_status_t bt_device_manager_le_gap_set_local_configuration(bt_gap_le_local_key_t *local_key, bool sc_only_mode)
{
    LOG_I(BT_DM, "set_local_config");

    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Set Local config fail, please init BT DM LE first!\r\n", 0);
        return BT_STATUS_FAIL;
    }
    memset(&g_local_key_req_default, 0x00, sizeof(bt_gap_le_local_key_t));
    memcpy(&g_local_key_req_default, local_key, sizeof(bt_gap_le_local_key_t));
    g_sc_only_default = sc_only_mode;
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_device_manager_le_gap_set_pairing_configuration(bt_gap_le_smp_pairing_config_t *pairing_config)
{
    LOG_I(BT_DM, "set_pairing_config");

    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Set Pairing config fail, please init BT DM LE first!\r\n", 0);
        return BT_STATUS_FAIL;
    }
    memset(&g_pairing_config_req_default, 0x00, sizeof(bt_gap_le_smp_pairing_config_t));
    g_pairing_config_req_default.io_capability = pairing_config->io_capability;
    g_pairing_config_req_default.oob_data_flag = pairing_config->oob_data_flag;
    g_pairing_config_req_default.auth_req = pairing_config->auth_req;
    g_pairing_config_req_default.maximum_encryption_key_size = pairing_config->maximum_encryption_key_size;
    g_pairing_config_req_default.initiator_key_distribution = pairing_config->initiator_key_distribution;
    g_pairing_config_req_default.responder_key_distribution = pairing_config->responder_key_distribution;
    return BT_STATUS_SUCCESS;
}

static void bt_device_manager_le_store_bonded_info_to_nvdm(uint8_t index)
{
    uint8_t i;
    for (i = index; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        /**< if it is the end unit.*/
        if (0 == memcmp(&default_bt_addr, &(bonded_info[i].bt_addr), sizeof(default_bt_addr))) {
            bt_device_manager_le_write_nvdm(&bonded_info[i], i);
            if (bt_dm_clear_flag) {
                if (i == (BT_DEVICE_MANAGER_LE_BONDED_MAX - 1)) {
                    bt_dm_clear_flag = false;
                    LOG_MSGID_I(BT_DM, "[DM]clear NVDM buffer success\r\n", 0);
                    return;
                }
                continue;
            }
        }
        if (0 != memcmp(&default_bt_addr, &(bonded_info[i].bt_addr), sizeof(default_bt_addr))) {
            bt_device_manager_le_write_nvdm(&bonded_info[i], i);
        }
    }
}

static void bt_device_manager_le_restore_bonded_info_from_nvdm(uint8_t index)
{
    uint8_t i;
    bt_device_manager_le_bonded_info_t *bond_info;
    for (i = index; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        bond_info = &(bonded_info[i]);
        bt_device_manager_le_read_nvdm(bond_info, i);
    }
}

static bt_device_manager_le_bonded_info_t *bt_device_manager_le_get_or_new_bonded_info(const bt_addr_t *peer_addr, bool new_flag)
{
    uint8_t i;
    LOG_MSGID_I(BT_DM, "get_or_new_bonded_info, flag(%d) addr (0x%02x):%02X:%02X:%02X:%02X:%02X:%02X", 8,
                new_flag, peer_addr->type,
                peer_addr->addr[5],
                peer_addr->addr[4],
                peer_addr->addr[3],
                peer_addr->addr[2],
                peer_addr->addr[1],
                peer_addr->addr[0]);
    if (!g_nvram_read_flag) {
        g_nvram_read_flag = true;
        bt_device_manager_le_restore_bonded_info_from_nvdm(0);
    }
    /** Check whether bonded? */
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX ; i++) {
        if ((0 == memcmp(peer_addr, &(bonded_info[i].bt_addr), sizeof(bt_addr_t))) ||
            (0 == memcmp(peer_addr, &(bonded_info[i].info.identity_addr.address), sizeof(bt_addr_t)))) {
            LOG_MSGID_I(BT_DM, "Bonded, return bonded_info[%d]", 1, i);
            return &(bonded_info[i]);
        }
    }
    /** unbonded, so need new a buffer. */
    if (new_flag) {
        for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX ; i++) {
            if (0 == memcmp(&default_bt_addr, &(bonded_info[i].bt_addr), sizeof(bt_addr_t))) {
                bonded_info[i].info.identity_addr.address.type = BT_ADDR_TYPE_UNKNOW;
                memcpy(&(bonded_info[i].bt_addr), peer_addr, sizeof(bt_addr_t));
                LOG_MSGID_I(BT_DM, "Un-Bonded, return new bonded_info[%d]", 1, i);
                return &(bonded_info[i]);
            }
        }
        /**have no empty buffer, so delete the oldest one, and return the last one buffer. */
        if (i == BT_DEVICE_MANAGER_LE_BONDED_MAX) {
            uint8_t j;
            LOG_MSGID_I(BT_DM, "No empty buffer, delete the oldest one!", 0);
            bt_device_manager_le_delete_dev_from_resolving_list((&(bonded_info[0].info)));
            memset(&(bonded_info[0]), 0x00, sizeof(bt_device_manager_le_bonded_info_t));
            bonded_info[0].bt_addr.type = BT_ADDR_TYPE_UNKNOW;
            bonded_info[0].info.identity_addr.address.type = BT_ADDR_TYPE_UNKNOW;
            BT_DM_LE_CLEAR_FLAG(0);
            for (j = 0; j < (BT_DEVICE_MANAGER_LE_BONDED_MAX - 1); j++) {
                bt_device_manager_le_delete_dev_from_resolving_list((&(bonded_info[j].info)));
                memcpy(&(bonded_info[j]), &(bonded_info[j + 1]), sizeof(bt_device_manager_le_bonded_info_t));
                memset(&(bonded_info[j + 1]), 0x00, sizeof(bt_device_manager_le_bonded_info_t));
                bonded_info[j + 1].bt_addr.type = BT_ADDR_TYPE_UNKNOW;
                bonded_info[j + 1].info.identity_addr.address.type = BT_ADDR_TYPE_UNKNOW;
            }
            bt_device_manager_le_store_bonded_info_to_nvdm(0);
            memcpy(&(bonded_info[BT_DEVICE_MANAGER_LE_BONDED_MAX - 1].bt_addr), peer_addr, sizeof(bt_addr_t));
            LOG_MSGID_I(BT_DM, "No empty buffer, return new bonded_info[%d]", 1, BT_DEVICE_MANAGER_LE_BONDED_MAX - 1);
            return &(bonded_info[BT_DEVICE_MANAGER_LE_BONDED_MAX - 1]);
        }
    }
    return NULL;
}

bt_device_manager_le_bonded_info_t *bt_device_manager_le_get_bonded_info_by_handle(bt_handle_t conn_handle)
{
    bt_status_t status;
    LOG_MSGID_I(BT_DM, "get_bonded_info_by_handle,conn_handle(0x%04x)", 1, conn_handle);
    bt_gap_le_connection_information_t con;
    memset(&(con), 0x00, sizeof(bt_gap_le_connection_information_t));
    status = bt_gap_le_get_connection_information(conn_handle, &con);
    if (BT_STATUS_SUCCESS == status) {
        return bt_device_manager_le_get_or_new_bonded_info(&(con.peer_addr), false);
    }
    return NULL;
}

static uint8_t bt_device_manager_le_get_index_by_address(bt_addr_t *address)
{
    uint8_t i;
    //LOG_I(BT_DM, "%s:enter\r\n", __FUNCTION__);

    if (0 == memcmp(&default_bt_addr, address, sizeof(default_bt_addr))) {
        LOG_MSGID_I(BT_DM, "empty address for find!\r\n", 0);
        return 255;
    }
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX ; i++) {
        if ((0 == memcmp(address, &(bonded_info[i].bt_addr), sizeof(bt_addr_t))) ||
            (0 == memcmp(address, &(bonded_info[i].info.identity_addr.address), sizeof(bt_addr_t)))) {
            return i;
        }
    }
    if (i == BT_DEVICE_MANAGER_LE_BONDED_MAX) {
        LOG_MSGID_I(BT_DM, "bt_device_manager_le_get_index_by_address, not find !\r\n", 0);
        return 255;
    }
    return 255;
}

void bt_device_manager_le_remove_bonded_device(bt_addr_t *peer_addr)
{
    uint8_t i;
    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Remove bonded device fail, please init BT DM LE first!\r\n", 0);
        return;
    }
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        if ((0 == memcmp(peer_addr, &(bonded_info[i].bt_addr), sizeof(bt_addr_t))) ||
            (0 == memcmp(peer_addr, &(bonded_info[i].info.identity_addr.address), sizeof(bt_addr_t)))) {
            bt_device_manager_le_bonded_event_cb(BT_DEVICE_MANAGER_LE_BONDED_REMOVE, peer_addr);
            bt_device_manager_le_delete_dev_from_resolving_list((&(bonded_info[i].info)));
            memset(&(bonded_info[i]), 0x00, sizeof(bt_device_manager_le_bonded_info_t));
            bonded_info[i].bt_addr.type = BT_ADDR_TYPE_UNKNOW;
            bonded_info[i].info.identity_addr.address.type = BT_ADDR_TYPE_UNKNOW;
            BT_DM_LE_CLEAR_FLAG(i);
            LOG_MSGID_I(BT_DM, "Remove bonded info for index  %d\r\n", 1, i);
            bt_device_manager_le_store_bonded_info_to_nvdm(i);
            break;
        }
    }
    if (i == BT_DEVICE_MANAGER_LE_BONDED_MAX) {
        LOG_MSGID_I(BT_DM, "Remove bonded info fail, because can not find it!\r\n", 0);
    }
}

static void bt_device_manager_le_reset_bonded_infos(void)
{
    uint8_t i;
    //LOG_I(BT_DM, "%s: enter\r\n", __FUNCTION__);
    g_nvram_read_flag = false;
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX ; i++) {
        memset(&(bonded_info[i]), 0x00, sizeof(bt_device_manager_le_bonded_info_t));
        bonded_info[i].bt_addr.type = BT_ADDR_TYPE_UNKNOW;
        bonded_info[i].info.identity_addr.address.type = BT_ADDR_TYPE_UNKNOW;
        BT_DM_LE_CLEAR_FLAG(i);
    }

    if (bt_dm_le_initialized) { //do below only when dm le is initialized
        bt_device_manager_le_bonded_event_cb(BT_DEVICE_MANAGER_LE_BONDED_CLEAR, NULL);
        bt_status_t st = bt_gap_le_set_resolving_list(BT_GAP_LE_CLEAR_RESOLVING_LIST, NULL);
        if (BT_STATUS_SUCCESS != st) {
            LOG_MSGID_I(BT_DM, "Clear device from Resolving List Failed\r\n", 0);
        }
    }
    LOG_MSGID_I(BT_DM, "bt_device_manager_le_reset_bonded_infos done!\r\n", 0);
}

void bt_device_manager_le_clear_all_bonded_info(void)
{
    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Clear bonded info fail, please init BT DM LE first!\r\n", 0);
        return;
    }
    LOG_MSGID_I(BT_DM, "[DM][LE] start clear bonded info\r\n", 0);
    bt_dm_clear_flag = true;
    bt_device_manager_le_reset_bonded_infos();
    bt_device_manager_le_store_bonded_info_to_nvdm(0);
}

bool bt_device_manager_le_is_bonded(bt_addr_t *address)
{
    uint8_t i;
    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Device bonded status check fail, please init BT DM LE first!\r\n", 0);
        return false;
    }
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        if ((0 == memcmp(address, &(bonded_info[i].bt_addr), sizeof(bt_addr_t))) ||
            (0 == memcmp(address, &(bonded_info[i].info.identity_addr.address), sizeof(bt_addr_t)))) {
            LOG_MSGID_I(BT_DM, "device have bonded! index is %d\r\n", 1, i);
            return true;
        }
    }
    return false;
}

uint8_t bt_device_manager_le_get_bonded_number(void)
{
    uint8_t i;
    uint8_t count = 0;
    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Get bonded device number fail, please init BT DM LE first!\r\n", 0);
        return 0;
    }
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        if (0 != memcmp(&default_bt_addr, &(bonded_info[i].bt_addr), sizeof(bt_addr_t))) {
            count++;
        }
    }
    LOG_MSGID_I(BT_DM, "get_bonded_number = %d\r\n", 1, count);
    return count;
}

void bt_device_manager_le_get_bonded_list(bt_bd_addr_t *list, uint8_t *count)
{
    uint8_t i;
    uint8_t buff_size = *count;
    bt_bd_addr_t *p = list;
    uint8_t bonded_num = 0;
    //LOG_MSGID_I(BT_DM, "bt_device_manager_le_get_bonded_list, want_read_count is %d\r\n", 1, *count);
    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Get bonded list fail, please init BT DM LE first!\r\n", 0);
        return;
    }
    if ((NULL == list) || (0 == buff_size)) {
        LOG_MSGID_I(BT_DM, "buffer is empty!\r\n", 0);
        return;
    }
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        //LOG_MSGID_I(BT_DM, "%d,%d,%d,%d!,index = %d\r\n", 5, default_bt_addr.type, default_bt_addr.addr[0], bonded_info[i].bt_addr.type, bonded_info[i].bt_addr.addr[0], i);
        if (0 != memcmp(&default_bt_addr, &(bonded_info[i].bt_addr), sizeof(bt_addr_t))) {
            memcpy(p, bonded_info[i].bt_addr.addr, sizeof(bt_bd_addr_t));
            bonded_num ++;
            if (buff_size == bonded_num) {
                break;
            }
            p++;
        }
    }
    *count = bonded_num;
    LOG_MSGID_I(BT_DM, "get_bonded_list, buff_size = %d, real_read = %d\r\n", 2, buff_size, *count);
}

static void bt_device_manager_le_save_connection_params(void *buff)
{
    uint8_t i;

    bt_gap_le_connection_ind_t *conn_ind = (bt_gap_le_connection_ind_t *)buff;
    for (i = 0; i < BT_DEVICE_MANAGER_LE_CONNECTION_MAX; i++) {
        if (0 == dm_info[i].connection_handle) {
            dm_info[i].connection_handle = conn_ind->connection_handle;
            memcpy(&dm_info[i].peer_addr, &conn_ind->peer_addr, sizeof(bt_addr_t));
            dm_info[i].connection_params.conn_interval = conn_ind->conn_interval;
            dm_info[i].connection_params.slave_latency = conn_ind->conn_latency;
            dm_info[i].connection_params.supervision_timeout = conn_ind->supervision_timeout;
            break;
        }
    }
    if (i == BT_DEVICE_MANAGER_LE_CONNECTION_MAX) {
        LOG_MSGID_I(BT_DM, "Reach max conn, no empty buff to save conn_info!\r\n", 0);
    }
}

static void bt_device_manager_le_delete_connection_params(void *buff)
{
    uint8_t i;
    bt_hci_evt_disconnect_complete_t *disc_ind;

    if (!buff)
        return;

    disc_ind = (bt_hci_evt_disconnect_complete_t *) buff;
    for (i = 0; i < BT_DEVICE_MANAGER_LE_CONNECTION_MAX ; i++) {
        if (disc_ind->connection_handle == dm_info[i].connection_handle) {
            memset(&(dm_info[i]), 0x00, sizeof(bt_device_manager_le_connection_struct_t));
            LOG_MSGID_I(BT_DM, "del conn param, hdl = 0x%x", 1, disc_ind->connection_handle);
            break;
        }
    }
    if (i == BT_DEVICE_MANAGER_LE_CONNECTION_MAX) {
        LOG_MSGID_I(BT_DM, "No conn info for deleting! hdl = 0x%x", 1, disc_ind->connection_handle);
    }
}

static void bt_device_manager_le_reset_connection_infos(void)
{
    uint8_t i;
    LOG_MSGID_I(BT_DM, "bt_device_manager_le_reset_connection_infos\r\n", 0);
    for (i = 0; i < BT_DEVICE_MANAGER_LE_CONNECTION_MAX ; i++) {
        memset(&(dm_info[i]), 0x00, sizeof(bt_device_manager_le_connection_struct_t));
    }
}

static void bt_device_manager_le_update_current_conn_interval(void *conn_params)
{
    uint8_t i;
    bt_gap_le_connection_update_ind_t *ind = (bt_gap_le_connection_update_ind_t *)conn_params;
    LOG_MSGID_I(BT_DM, "update_conn_interval, conn_handle(0x%04x)\r\n", 1, ind->conn_handle);
    for (i = 0; i < BT_DEVICE_MANAGER_LE_CONNECTION_MAX; i++) {
        if (dm_info[i].connection_handle == ind->conn_handle) {
            dm_info[i].connection_params.conn_interval = ind->conn_interval;
            dm_info[i].connection_params.slave_latency = ind->conn_latency;
            dm_info[i].connection_params.supervision_timeout = ind->supervision_timeout;
            break;
        }
    }
    if (i == BT_DEVICE_MANAGER_LE_CONNECTION_MAX) {
        LOG_MSGID_I(BT_DM, "Reach max conn, update conn params fail!\r\n", 0);
    }
}

bt_device_manager_le_connection_param_t *bt_device_manager_le_get_current_connection_param(bt_handle_t connection_handle)
{
    uint8_t i;
    LOG_MSGID_I(BT_DM, "bt_device_manager_le_get_current_connection_param, conn_handle(0x%04x)\r\n", 1, connection_handle);
    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Get Conn interval fail, please init BT DM LE first! \r\n", 0);
        return NULL;
    }
    for (i = 0; i < BT_DEVICE_MANAGER_LE_CONNECTION_MAX; i++) {
        if (connection_handle == dm_info[i].connection_handle) {
            return &(dm_info[i].connection_params);
        }
    }
    return NULL;
}

bt_device_manager_le_bonded_info_t *bt_device_manager_le_get_bonding_info_by_addr_ext(bt_bd_addr_t *remote_addr)
{
    uint8_t i;
    //bt_addr_t addr = {0};

    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Device bonded status check fail, please init BT DM LE first!\r\n", 0);
        return NULL;
    }
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        if ((0 != memcmp(&default_bt_addr.addr, remote_addr, BT_BD_ADDR_LEN)) &&
            ((0 == memcmp(remote_addr, &(bonded_info[i].bt_addr.addr), sizeof(bt_bd_addr_t))) ||
             (0 == memcmp(remote_addr, &(bonded_info[i].info.identity_addr.address.addr), sizeof(bt_bd_addr_t))))) {
            return &(bonded_info[i]);
        }
    }
    LOG_MSGID_I(BT_DM, "bt_device_manager_le_get_bonding_info_by_addr_ext Fail!\r\n", 0);
    return NULL;
}

bt_status_t bt_device_manager_le_get_all_bonding_infos(bt_device_manager_le_bonded_info_t *infos, uint8_t *count)
{
    uint8_t i;
    uint8_t buff_size = *count;
    bt_device_manager_le_bonded_info_t *p = infos;
    uint8_t bonded_num = 0;
    LOG_MSGID_I(BT_DM, "bt_device_manager_le_get_bonded_list, want_read_count is %d\r\n", 1, *count);
    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Get bonded list fail, please init BT DM LE first!\r\n", 0);
        return BT_STATUS_FAIL;
    }
    if ((NULL == infos) || (0 == buff_size)) {
        LOG_MSGID_I(BT_DM, "buffer is empty!\r\n", 0);
        return BT_STATUS_FAIL;
    }

    for (i = 0; ((i < BT_DEVICE_MANAGER_LE_BONDED_MAX) && (i < buff_size)); i++) {
        LOG_MSGID_I(BT_DM, "%d,%d,%d,%d!\r\n", 4, default_bt_addr.type, default_bt_addr.addr[0], bonded_info[i].bt_addr.type, bonded_info[i].bt_addr.addr[0]);
        if (0 != memcmp(&default_bt_addr, &(bonded_info[i].bt_addr), sizeof(bt_addr_t))) {
            memcpy(&(p->bt_addr), &(bonded_info[i].bt_addr), sizeof(bt_addr_t));
            memcpy(&(p->info), &(bonded_info[i].info), sizeof(bt_gap_le_bonding_info_t));
            bonded_num++;
            p++;
        }
    }
    *count = bonded_num;
    LOG_MSGID_I(BT_DM, "bt_device_manager_le_get_bonded_list, real_read_count is %d\r\n", 1, *count);
    return BT_STATUS_SUCCESS;
}

bt_handle_t bt_device_manager_le_get_hdl_by_addr(const bt_addr_t *bt_addr)
{
    uint8_t i;

    for (i = 0; i < BT_DEVICE_MANAGER_LE_CONNECTION_MAX; i++) {
        if (memcmp(&dm_info[i].peer_addr, bt_addr, sizeof(bt_addr_t)) == 0) {
            return dm_info[i].connection_handle;
        }
    }
    return BT_HANDLE_INVALID;
}

bt_status_t bt_device_manager_le_set_bonding_info_by_addr(bt_addr_t *remote_addr, bt_gap_le_bonding_info_t *info)
{
    uint8_t i;
    //bt_addr_t addr = {0};

    if (!bt_dm_le_initialized) {
        LOG_MSGID_I(BT_DM, "Device bonded status check fail, please init BT DM LE first!\r\n", 0);
        return BT_STATUS_FAIL;
    }
    if ((NULL == info) || (!(info->key_security_mode & BT_GAP_LE_SECURITY_BONDED_MASK))) {
        LOG_MSGID_I(BT_DM, "bt_device_manager_le_set_bonding_info_by_addr fail, empty info!\r\n", 0);
        return BT_STATUS_FAIL;
    }
    /** Check whether bonded? */
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX ; i++) {
        if (0 == memcmp(remote_addr, &(bonded_info[i].bt_addr), sizeof(bt_addr_t))) {
            LOG_MSGID_I(BT_DM, "Have Bonded, bonded_info[%d].\r\n", 1, i);
            //TODO, update anyway(compare all)
            //save the new keys
            if (0 != memcmp(info, &(bonded_info[i].info), sizeof(bt_gap_le_bonding_info_t))) {
                bt_device_manager_le_delete_dev_from_resolving_list((&(bonded_info[i].info)));
                memcpy(&(bonded_info[i].info), info, sizeof(bt_gap_le_bonding_info_t));
                BT_DM_LE_CANCEL_FLAG(i, BT_DM_LE_FLAG_NOT_SYNC);
                bt_device_manager_le_store_bonded_info_to_nvdm(i);
                //bt_device_manager_le_add_resolving_list(i);
                bt_device_manager_le_add_resolving_list_by_index(i);
                LOG_MSGID_I(BT_DM, "re-save success, bonded_info[%d]\r\n", 1, i);
            }
            return BT_STATUS_SUCCESS;
        }
    }
    /** unbonded, so need new a buffer. */
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX ; i++) {
        if (0 == memcmp(&default_bt_addr, &(bonded_info[i].bt_addr), sizeof(bt_addr_t))) {
            bonded_info[i].info.identity_addr.address.type = BT_ADDR_TYPE_UNKNOW;
            memcpy(&(bonded_info[i].bt_addr), remote_addr, sizeof(bt_addr_t));
            memcpy(&(bonded_info[i].info), info, sizeof(bt_gap_le_bonding_info_t));
            LOG_MSGID_I(BT_DM, "save success, bonded_info[%d]\r\n", 1, i);
            BT_DM_LE_CANCEL_FLAG(i, BT_DM_LE_FLAG_NOT_SYNC);
            bt_device_manager_le_store_bonded_info_to_nvdm(i);
            //bt_device_manager_le_add_resolving_list(i);
            bt_device_manager_le_add_resolving_list_by_index(i);
            return BT_STATUS_SUCCESS;
        }
    }

    /**have no empty buffer, so delete the oldest one, and return the last one buffer.
      * Delete the oldest one, at [0], and sorting the order, and then put the new one to the tail.
      */
    if (i == BT_DEVICE_MANAGER_LE_BONDED_MAX) {
        uint8_t j;
        LOG_MSGID_I(BT_DM, "No empty buffer, Need to delete the oldest one!\r\n", 0);

        bt_device_manager_le_delete_dev_from_resolving_list((&(bonded_info[0].info)));
        memset(&(bonded_info[0]), 0x00, sizeof(bt_device_manager_le_bonded_info_t));
        bonded_info[0].bt_addr.type = BT_ADDR_TYPE_UNKNOW;
        bonded_info[0].info.identity_addr.address.type = BT_ADDR_TYPE_UNKNOW;
        BT_DM_LE_CLEAR_FLAG(0);
        for (j = 0; j < (BT_DEVICE_MANAGER_LE_BONDED_MAX - 1); j++) {
            //bt_device_manager_le_delete_dev_from_resolving_list((&(bonded_info[j].info)));--only need delete oldest one.
            memcpy(&(bonded_info[j]), &(bonded_info[j + 1]), sizeof(bt_device_manager_le_bonded_info_t));
            memset(&(bonded_info[j + 1]), 0x00, sizeof(bt_device_manager_le_bonded_info_t));
            bonded_info[j + 1].bt_addr.type = BT_ADDR_TYPE_UNKNOW;
            bonded_info[j + 1].info.identity_addr.address.type = BT_ADDR_TYPE_UNKNOW;
        }
        memcpy(&(bonded_info[BT_DEVICE_MANAGER_LE_BONDED_MAX - 1].bt_addr), remote_addr, sizeof(bt_addr_t));
        memcpy(&(bonded_info[BT_DEVICE_MANAGER_LE_BONDED_MAX - 1].info), info, sizeof(bt_gap_le_bonding_info_t));
        bt_device_manager_le_add_resolving_list_by_index(BT_DEVICE_MANAGER_LE_BONDED_MAX - 1);
        bt_device_manager_le_store_bonded_info_to_nvdm(0);
        LOG_MSGID_I(BT_DM, "delete the oldest one to save the new one, return bonded_info[%d].\r\n", 1, BT_DEVICE_MANAGER_LE_BONDED_MAX - 1);
        return BT_STATUS_SUCCESS;
    }
    return BT_STATUS_SUCCESS;
}

/**
 * @brief Function for application main entry.
 */
bt_status_t bt_device_manager_le_register_callback(bt_device_manager_le_bonded_event_callback callback)
{
    if (NULL == callback) {
        return BT_STATUS_FAIL;
    } else {
        /**Initialize.*/
        LOG_MSGID_I(BT_DM, "init app_callback=0x%04x", 1, callback);
        return bt_device_manager_le_cb_register_int(callback);
    }
}

bt_status_t bt_device_manager_le_deregister_callback(bt_device_manager_le_bonded_event_callback callback)
{
    if (NULL == callback) {
        return BT_STATUS_FAIL;
    } else {
        /**Deinit.*/
        LOG_MSGID_I(BT_DM, "deinit app_callback=0x%04x", 1, callback);
        return bt_device_manager_le_cb_deregister_int(callback);
    }
    return BT_STATUS_FAIL;
}

bool bt_device_manager_le_is_sync_bond_info(bt_addr_t *bt_addr)
{
    uint8_t i = 0;
    bt_device_manager_le_bonded_info_t *p_bond_info = NULL;
    for (i = 0; i < BT_DEVICE_MANAGER_LE_BONDED_MAX; i++) {
        p_bond_info = &bonded_info[i];
        if ((0 == memcmp(&p_bond_info->bt_addr, bt_addr, sizeof(bt_device_manager_le_bonded_info_t))) &&
            (0 != memcmp(&default_bt_addr, bt_addr, sizeof(bt_device_manager_le_bonded_info_t)))) {
            if (BT_DM_LE_GET_FLAG(i) & BT_DM_LE_FLAG_NOT_SYNC) {
                LOG_MSGID_I(BT_DM, "[DM][LE] Not need sync bond info index = %d \r\n", 1, i);
                return false;
            }
        }
    }
    return true;
}
