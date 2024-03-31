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

//MTK_COMMON: for getting connection num
#include "project_config.h"
//(end) MTK_COMMON

#include "bt_gattc_discovery_internal.h"
#include "bt_callback_manager.h"
#include "syslog.h"
#include "FreeRTOS.h"
#include "bt_init.h"

#ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE
#include "bt_role_handover.h"
#include "multi_ble_adv_manager.h"
#include "bt_gap_le_service.h"
#endif /* #ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE */

log_create_module(BT_GATTC, PRINT_LEVEL_INFO);



/**************************Macro and Global*************************/
#define SRV_DISC_INVALID_HANDLE     0x0000   /**< The invalid handle. */
#define SRV_DISC_START_HANDLE       0x0001   /* The start handle value used during service discovery. */
#define SRV_DISC_END_HANDLE         0xFFFF   /* The start handle value used during service discovery. */
#define BT_GATTC_DISCOVERY_MAX_USERS BT_GATTC_DISCOVERY_MAX_SRVS  /* The maximum number of users/registrations allowed by this module. */

//MTK_COMMON: Fix build error
#define BT_LE_CONNECTION_NUM BT_LE_CONNECTION_MAX
//(end) MTK_COMMON

#define BT_GATT_DISCOVERY_UUID_16BTS_SIZE                       2
#define BT_GATT_DISCOVERY_UUID_32BTS_SIZE                       4
#define BT_GATT_DISCOVERY_UUID_128BTS_SIZE                      16

static bt_gattc_registered_handlers g_registered_handlers[BT_GATTC_DISCOVERY_MAX_USERS];         /* The discovery GATT service storage variable. */
static bt_gattc_discovery_context_t g_discovery_context[BT_LE_CONNECTION_NUM];   /* The discovery context for each conn handle. */
static uint32_t g_num_of_handlers_reg;      /* The number of handlers registered with the GATTC discovery module. */
static bool     g_initialized = false;      /* This variable indicates if the module is initialized or not. */

#ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE
static uint8_t g_discovery_in_progress_rho = 0;
static uint8_t g_discovery_rho_state = 0;
#endif /* #ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE */

/************************************************
*   Utilities
*************************************************/
static void bt_gattc_discovery_reset_context(void);
static bt_status_t bt_gattc_discovery_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff);
static bt_status_t  bt_gattc_discovery_save_connection_info(void *buff);
static bt_status_t bt_gattc_discovery_delete_connection_info(void *buff);
bt_gattc_discovery_context_t *bt_gattc_discovery_get_cntx_by_handle(uint16_t conn_handle);
static void bt_gattc_discovery_change_discovery_state(bt_gattc_discovery_context_t *cntx, bt_gattc_conn_discovery_state_t state);
static bt_status_t bt_gattc_discovery_add_user_triggered(uint16_t conn_handle, bt_gattc_discovery_user_t user);
static bt_gattc_discovery_user_t bt_gattc_discovery_get_user_triggered(uint16_t conn_handle);
static bt_status_t bt_gattc_discovery_get_discovery_buffer(bt_gattc_discovery_service_t **discovery_buffer, bt_gattc_discovery_service_t *user_buffer);
static void bt_gattc_discovery_free_discovery_buffer(bt_gattc_discovery_service_t *discovery_buffer);
static void bt_gattc_discovery_copy_discovery_buffer(bt_gattc_discovery_service_t *user_buffer, bt_gattc_discovery_service_t *discovery_buffer);
static bt_status_t bt_gattc_discovery_copy_service_uuid(ble_uuid_t *dest, ble_uuid_t *src);

bt_gattc_discovery_status_t bt_gattc_discovery_start_internal(bt_gattc_discovery_context_t *cntx);
static void bt_gattc_discover_included_service(bt_gattc_discovery_context_t *cntx);
static void bt_gattc_discover_characteristic(bt_gattc_discovery_context_t *cntx);
static void bt_gattc_discover_descriptor(bt_gattc_discovery_context_t *cntx);
static bt_status_t bt_gattc_discovery_all_descriptor(bt_gattc_discovery_context_t *cntx, uint8_t index);

static void bt_gattc_discovery_result_evt_trigger(bt_gattc_discovery_context_t *cntx, bool is_complete, int32_t err_code);
static void bt_gattc_discovery_primary_service_cnf(bt_status_t status, void *buff);
static void bt_gattc_discovery_charactiristics_cnf(bt_status_t status, void *buff);
static void bt_gattc_discovery_descriptors_cnf(bt_status_t status, void *buff);
static void bt_gattc_discovery_find_included_services_cnf(bt_status_t status, void *buff);
static void bt_gattc_discovery_included_service_completed(bt_gattc_discovery_context_t *cntx);
static void bt_gattc_discovery_primary_service_completed(bt_gattc_discovery_context_t *cntx);

#ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE
extern bt_gap_le_srv_link_attribute_t bt_gap_le_srv_get_link_attribute_by_handle(bt_handle_t handle);
static bt_status_t bt_gattc_discovery_rho_init(void);
static bool bt_gattc_discovery_check_all_conn_in_progress(void);
#endif /* #ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE */
extern bool bt_gattc_is_flow_control_ongoing(bt_handle_t conneciton_handle);
/************************************************
*   Utilities
*************************************************/

static void bt_gattc_discovery_reset_context(void)
{
    uint8_t i;
    LOG_MSGID_I(BT_GATTC, "reset context", 0);
    for (i = 0; i < BT_LE_CONNECTION_NUM; i++) {
        g_discovery_context[i].conn_handle = BT_HANDLE_INVALID;
        g_discovery_context[i].conn_discovery_state = BT_GATTC_CONN_DISCOVERY_STATE_NONE;
        g_discovery_context[i].user_triggered_list = NULL;
        g_discovery_context[i].user_triggered_count = 0;
        g_discovery_context[i].discovering_serv_index = 0;
        g_discovery_context[i].discovering_inc_serv_index = 0;
        g_discovery_context[i].discovering_char_index = 0;
        bt_utils_memset(&g_discovery_context[i].multi_instance, 0x00, sizeof(bt_gattc_discovery_multiple_instance_t));
        if (g_discovery_context[i].discovery_buffer != NULL) {
            bt_gattc_discovery_free_discovery_buffer(g_discovery_context[i].discovery_buffer);
            g_discovery_context[i].discovery_buffer = NULL;
        }
    }
}

#ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE
static bool bt_gattc_discovery_check_all_conn_in_progress(void)
{
    uint8_t i;
    for (i = 0; i < BT_LE_CONNECTION_NUM; i++) {
        if (g_discovery_context[i].conn_discovery_state > BT_GATTC_CONN_DISCOVERY_STATE_IDLE) {
            return true;
        }
    }
    return false;
}
#endif /* #ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE */

static bt_status_t bt_gattc_discovery_add_user_triggered(uint16_t conn_handle, bt_gattc_discovery_user_t user)
{
    LOG_MSGID_I(BT_GATTC, "add_user_triggered, conn_handle: 0x%04x, user: %d", 2, conn_handle, user);
    bt_gattc_discovery_context_t *cntx = bt_gattc_discovery_get_cntx_by_handle(conn_handle);
    if (cntx == NULL) {
        return BT_STATUS_FAIL;
    }

    user_triggered_t *user_add = (user_triggered_t *)bt_utils_memory_alloc(sizeof(user_triggered_t));
    if (user_add == NULL) {
        LOG_MSGID_E(BT_GATTC, "add_user_triggered: failed malloc size: %d", 1, sizeof(user_triggered_t));
        return BT_STATUS_OUT_OF_MEMORY;
    }

    user_add->user = user;
    user_add->next = NULL;
    if (cntx->user_triggered_count == 0) {
        cntx->user_triggered_list = user_add;
    } else {
        user_triggered_t *curr_node;
        curr_node = cntx->user_triggered_list;
        while (curr_node->next) {
            if (curr_node->user == user) {
                LOG_MSGID_I(BT_GATTC, "add_user_triggered: user(0x%x) already trigger.", 1, user);
                bt_utils_memory_free(user_add);
                return BT_STATUS_SUCCESS;
            }
            curr_node = curr_node->next;
        }
        curr_node->next = user_add;
    }
    cntx->user_triggered_count++;

    LOG_MSGID_I(BT_GATTC, "add_user_triggered, user_triggered_count: %d", 1, cntx->user_triggered_count);
    return BT_STATUS_SUCCESS;
}

static bt_gattc_discovery_user_t bt_gattc_discovery_get_user_triggered(uint16_t conn_handle)
{
    user_triggered_t *user_temp;
    bt_gattc_discovery_user_t ret;
    bt_gattc_discovery_context_t *cntx = bt_gattc_discovery_get_cntx_by_handle(conn_handle);
    LOG_MSGID_I(BT_GATTC, "get_user_triggered, conn_handle: 0x%04x", 1, conn_handle);

    if (NULL == cntx) {
        return BT_GATTC_DISCOVERY_USER_NONE;
    }

    if (cntx->user_triggered_count == 0 || cntx->user_triggered_list == NULL) {
        LOG_MSGID_I(BT_GATTC, "get_user_triggered: no user triggered", 0);
        return BT_GATTC_DISCOVERY_USER_NONE;
    }

    user_temp = cntx->user_triggered_list;
    cntx->user_triggered_list = user_temp->next;
    cntx->user_triggered_count--;
    ret = user_temp->user;
    bt_utils_memory_free(user_temp);

    LOG_MSGID_I(BT_GATTC, "get_user_triggered, conn_handle: 0x%04x, get_user: %d, user_triggered_count: %d", 3, conn_handle, ret, cntx->user_triggered_count);
    return ret;
}

static bt_status_t  bt_gattc_discovery_save_connection_info(void *buff)
{
    uint8_t i;
    bt_status_t status = BT_STATUS_SUCCESS;
    bt_gap_le_connection_ind_t *conn_ind = (bt_gap_le_connection_ind_t *)buff;
    for (i = 0; i < BT_LE_CONNECTION_NUM; i++) {
        /**< first connect, to save connection info. */
        if (BT_HANDLE_INVALID == g_discovery_context[i].conn_handle) {
            LOG_MSGID_I(BT_GATTC, "first connect, conn handle=0x%04x", 1, conn_ind->connection_handle);

            g_discovery_context[i].conn_handle = conn_ind->connection_handle;
            g_discovery_context[i].conn_discovery_state = BT_GATTC_CONN_DISCOVERY_STATE_IDLE;
            break;
            /**< Reconnect. */
        } else if (conn_ind->connection_handle == g_discovery_context[i].conn_handle) {
            LOG_MSGID_I(BT_GATTC, "reconnect, conn handle=0x%04x", 1, g_discovery_context[i].conn_handle);
            break;
        }
    }
    if (i == BT_LE_CONNECTION_NUM) {
        LOG_MSGID_W(BT_GATTC, "Reach max. conn, no empty buf!", 0);
        status = BT_STATUS_OUT_OF_MEMORY;
    }
    return status;
}

static bt_status_t bt_gattc_discovery_delete_connection_info(void *buff)
{
    uint8_t i;
    bt_status_t status = BT_STATUS_SUCCESS;
    bt_hci_evt_disconnect_complete_t *disconn_ind;
    //LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_delete_connection_info", 0);
    disconn_ind = (bt_hci_evt_disconnect_complete_t *) buff;
    for (i = 0; i < BT_LE_CONNECTION_NUM ; i++) {
        if (disconn_ind->connection_handle == g_discovery_context[i].conn_handle) {
            LOG_MSGID_I(BT_GATTC, "disconn context:%02x, conn_handle = %02x", 2, &g_discovery_context[i], g_discovery_context[i].conn_handle);
            bt_utils_memset(&(g_discovery_context[i]), 0, sizeof(bt_gattc_discovery_context_t));
            g_discovery_context[i].conn_handle = BT_HANDLE_INVALID;
            g_discovery_context[i].conn_discovery_state = BT_GATTC_CONN_DISCOVERY_STATE_NONE;
            g_discovery_context[i].user_triggered_list = NULL;
            g_discovery_context[i].user_triggered_count = 0;

            break;
        }
    }
    if (i == BT_LE_CONNECTION_NUM) {
        LOG_MSGID_W(BT_GATTC, "No conn info to delete!", 0);
        status = BT_STATUS_FAIL;
    }
    return status;
}

bt_gattc_discovery_context_t *bt_gattc_discovery_get_cntx_by_handle(uint16_t conn_handle)
{
    uint8_t i = 0;
    for (i = 0; i < BT_LE_CONNECTION_NUM; i++) {
        if ((conn_handle != BT_HANDLE_INVALID) && (conn_handle == g_discovery_context[i].conn_handle)) {
            return &(g_discovery_context[i]);
        }
    }
    if (i == BT_LE_CONNECTION_NUM) {
        LOG_MSGID_I(BT_GATTC, "get_cntx_by_handle, not connected!", 0);
    }
    return NULL;
}

static bt_gattc_discovery_buffer_size_t bt_gattc_discovery_get_buffer_size(bt_gattc_discovery_service_t *user_buffer)
{
    bt_gattc_discovery_buffer_size_t buffer_size = {0};

    buffer_size.serv_szie = sizeof(bt_gattc_discovery_service_t);
    buffer_size.inc_serv_size = sizeof(bt_gattc_discovery_included_service_t) * user_buffer->included_srv_count;
    buffer_size.chara_size = sizeof(bt_gattc_discovery_characteristic_t) * user_buffer->characteristic_count;

    for (uint8_t i = 0; i < user_buffer->characteristic_count; i++) {
        buffer_size.desc_size += sizeof(bt_gattc_discovery_descriptor_t) * user_buffer->charateristics[i].descriptor_count;
    }

    for (uint8_t i = 0; i < user_buffer->included_srv_count; i++) {
        buffer_size.inc_chara_size += sizeof(bt_gattc_discovery_characteristic_t) * user_buffer->included_service[i].char_count;
        for (uint8_t j = 0; j < user_buffer->included_service[i].char_count; j++) {
            buffer_size.inc_desc_size += sizeof(bt_gattc_discovery_descriptor_t) * user_buffer->included_service[i].charateristics[j].descriptor_count;
        }
    }

    buffer_size.all_size = buffer_size.serv_szie + buffer_size.chara_size + buffer_size.desc_size
                           + buffer_size.inc_serv_size + buffer_size.inc_chara_size + buffer_size.inc_desc_size;
    LOG_MSGID_I(BT_GATTC, "get_buffer_size, size: %d, buffer: 0x%04x", 2, buffer_size.all_size, user_buffer);
    return buffer_size;
}

static void bt_gattc_discovery_get_discovery_buffer_service_info(
    bt_gattc_discovery_service_t *user_serv_buffer,
    bt_gattc_discovery_characteristic_t *p_chara,
    bt_gattc_discovery_descriptor_t *p_desc)
{
    uint8_t i, j;

    for (i = 0; i < user_serv_buffer->characteristic_count; i++) {
        bt_utils_memcpy(&p_chara[i], &user_serv_buffer->charateristics[i], sizeof(bt_gattc_discovery_characteristic_t));
        p_chara[i].descr_count_found = 0;
        p_chara[i].descriptor = p_desc;
        for (j = 0; j < p_chara[i].descriptor_count; j ++) {
            bt_utils_memcpy(&p_chara[i].descriptor[j], &user_serv_buffer->charateristics[i].descriptor[j], sizeof(bt_gattc_discovery_descriptor_t));
        }
        p_desc += p_chara[i].descriptor_count;
    }
}

static void bt_gattc_discovery_get_discovery_buffer_inc_service_info(
    bt_gattc_discovery_included_service_t *user_inc_serv_buffer,
    bt_gattc_discovery_characteristic_t *p_inc_chara,
    bt_gattc_discovery_descriptor_t *p_inc_desc)
{
    uint8_t i, j;

    for (i = 0; i < user_inc_serv_buffer->char_count; i++) {
        bt_utils_memcpy(&p_inc_chara[i], &user_inc_serv_buffer->charateristics[i], sizeof(bt_gattc_discovery_characteristic_t));
        p_inc_chara[i].descr_count_found = 0;
        p_inc_chara[i].descriptor = p_inc_desc;
        for (j = 0; j < p_inc_chara[i].descriptor_count; j ++) {
            bt_utils_memcpy(&p_inc_chara[i].descriptor[j], &user_inc_serv_buffer->charateristics[i].descriptor[j], sizeof(bt_gattc_discovery_descriptor_t));
        }
        p_inc_desc += p_inc_chara[i].descriptor_count;
    }
}

static bt_status_t bt_gattc_discovery_get_discovery_buffer(bt_gattc_discovery_service_t **discovery_buffer, bt_gattc_discovery_service_t *user_buffer)
{
    bt_status_t ret = BT_STATUS_SUCCESS;
    bt_gattc_discovery_buffer_size_t buffer_size;
    bt_gattc_discovery_service_t *p_discovery_buffer = NULL;

    // MTK add, should check pointer before use it
    if (!user_buffer)
        return BT_STATUS_FAIL;
    buffer_size = bt_gattc_discovery_get_buffer_size(user_buffer);

    p_discovery_buffer = (bt_gattc_discovery_service_t *)bt_utils_memory_alloc(buffer_size.all_size);
    if (p_discovery_buffer == NULL) {
        LOG_MSGID_E(BT_GATTC, "get_disc_buffer fail malloc size: %d", 1, buffer_size);
        return BT_STATUS_OUT_OF_MEMORY;
    }

    /* MTK_David: memset after confirmed that malloc success */
    bt_utils_memset(p_discovery_buffer, 0x00, sizeof(buffer_size.all_size));

    bt_gattc_discovery_characteristic_t *p_chara = (bt_gattc_discovery_characteristic_t *)((uint8_t *)p_discovery_buffer + buffer_size.serv_szie);
    bt_gattc_discovery_descriptor_t *p_desc = (bt_gattc_discovery_descriptor_t *)((uint8_t *)p_chara + buffer_size.chara_size);
    bt_gattc_discovery_included_service_t *p_inc_serv = (bt_gattc_discovery_included_service_t *)((uint8_t *)p_desc + buffer_size.desc_size);
    bt_gattc_discovery_characteristic_t *p_inc_chara = (bt_gattc_discovery_characteristic_t *)((uint8_t *)p_inc_serv + buffer_size.inc_serv_size);
    bt_gattc_discovery_descriptor_t *p_inc_desc = (bt_gattc_discovery_descriptor_t *)((uint8_t *)p_inc_chara + buffer_size.inc_chara_size);
    // get primery service chara & desc info from user buffer to discovery buffer.
    bt_gattc_discovery_get_discovery_buffer_service_info(user_buffer, p_chara, p_desc);
    // get included service chara & desc info from user buffer to discovery buffer.
    for (uint8_t i = 0; i < user_buffer->included_srv_count; i++) {
        bt_utils_memcpy(&p_inc_serv[i], &user_buffer->included_service[i], sizeof(bt_gattc_discovery_included_service_t));
        p_inc_serv[i].char_count_found = 0;
        p_inc_serv[i].multi_instance_count = 0;
        p_inc_serv[i].start_handle = 0;
        p_inc_serv[i].end_handle = 0;
        p_inc_serv[i].charateristics = p_inc_chara;
        bt_gattc_discovery_get_discovery_buffer_inc_service_info(&user_buffer->included_service[i], p_inc_chara, p_inc_desc);
        p_inc_chara += p_inc_serv[i].char_count;
    }
    // get discovery buffer from user buffer.
    bt_utils_memcpy(p_discovery_buffer, user_buffer, sizeof(bt_gattc_discovery_service_t));
    p_discovery_buffer->charateristics = p_chara;
    p_discovery_buffer->included_service = p_inc_serv;

    p_discovery_buffer->included_srv_count_found = 0;
    p_discovery_buffer->char_count_found = 0;

    *discovery_buffer = p_discovery_buffer;
    LOG_MSGID_I(BT_GATTC, "get_disc_buffer, buf: 0x%04x", 1, *discovery_buffer);

    return ret;
}

static void bt_gattc_discovery_copy_charc_buffer(bt_gattc_discovery_characteristic_t *dest, bt_gattc_discovery_characteristic_t *src, uint8_t charac_count)
{
    uint8_t i, j;

    for (i = 0; i < charac_count; i++) {
        //LOG_MSGID_I(BT_GATTC, "copy_charc_buffer, index: %d, desc_found:%d", 2, i, src[i].descr_count_found); //MTK_COMMON: David - reduce log
        dest[i].descr_count_found = src[i].descr_count_found;
        dest[i].handle = src[i].handle;
        dest[i].value_handle = src[i].value_handle;
        dest[i].property = src[i].property;
        bt_gattc_discovery_copy_service_uuid(&dest[i].char_uuid, &src[i].char_uuid);
        for (j = 0; j < dest[i].descr_count_found; j++) {
            bt_utils_memcpy(&dest[i].descriptor[j], &src[i].descriptor[j], sizeof(bt_gattc_discovery_descriptor_t));
        }
    }
}

static void bt_gattc_discovery_copy_inc_serv_buffer(bt_gattc_discovery_included_service_t *dest, bt_gattc_discovery_included_service_t *src, uint8_t inc_serv_count)
{
    uint8_t i;

    for (i = 0; i < inc_serv_count; i++) {
        LOG_MSGID_I(BT_GATTC, "copy_inc_serv_buffer, inc_serv_index: %d, char_count_found:%d", 2, i, src[i].char_count_found);
        dest[i].char_count_found = src[i].char_count_found;
        dest[i].start_handle = src[i].start_handle;
        dest[i].end_handle = src[i].end_handle;
        bt_gattc_discovery_copy_service_uuid(&dest[i].service_uuid, &src[i].service_uuid);
        bt_gattc_discovery_copy_charc_buffer(dest[i].charateristics, src[i].charateristics, dest[i].char_count_found);
    }
}

static void bt_gattc_discovery_copy_discovery_buffer(bt_gattc_discovery_service_t *user_buffer, bt_gattc_discovery_service_t *discovery_buffer)
{
    uint8_t chara_count_found;
    uint8_t inc_serv_count_found;

    /* MTK_David: should check pointer before use it */
    if (!discovery_buffer)
        return;

    chara_count_found = discovery_buffer->char_count_found;
    inc_serv_count_found = discovery_buffer->included_srv_count_found;

    user_buffer->included_srv_count_found = inc_serv_count_found;
    user_buffer->char_count_found = chara_count_found;
    user_buffer->start_handle = discovery_buffer->start_handle;
    user_buffer->end_handle = discovery_buffer->end_handle;
    bt_gattc_discovery_copy_service_uuid(&user_buffer->service_uuid, &discovery_buffer->service_uuid);

    LOG_MSGID_I(BT_GATTC, "copy_discovery_buffer, chara_count_found: %d, inc_serv_count_found:%d", 2, chara_count_found, inc_serv_count_found);
    bt_gattc_discovery_copy_charc_buffer(user_buffer->charateristics, discovery_buffer->charateristics, chara_count_found);
    bt_gattc_discovery_copy_inc_serv_buffer(user_buffer->included_service, discovery_buffer->included_service, inc_serv_count_found);
}

static void bt_gattc_discovery_free_discovery_buffer(bt_gattc_discovery_service_t *discovery_buffer)
{
    LOG_MSGID_I(BT_GATTC, "free_discovery_buffer, buf: 0x%04x", 1, discovery_buffer);
    bt_utils_memory_free(discovery_buffer);
}

static uint8_t bt_gattc_discovery_find_next_resigtered_serv_index_by_user(uint8_t serv_index_start, bt_gattc_discovery_user_t user)
{
    uint8_t i;

    //(MTK ReduceLog)LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_find_next_resigtered_serv_index_by_user, start_serv_index:%d, user:%d", 2, serv_index_start, user);
    for (i = serv_index_start; i < g_num_of_handlers_reg; i++) {
        if (g_registered_handlers[i].user_registered == user) {
            //(MTK ReduceLog)LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_find_next_resigtered_serv_index_by_user, next discovery serv index:%d", 1, i);
            return i;
        }
    }
    if (i == g_num_of_handlers_reg) {
        LOG_MSGID_I(BT_GATTC, "find next service_index failed, i:%d", 1, i);
        i = 0xff;
    }
    return i;
}

static void bt_gattc_discovery_change_discovery_state(bt_gattc_discovery_context_t *cntx, bt_gattc_conn_discovery_state_t state)
{
    if (state >= BT_GATTC_CONN_DISCOVERY_STATE_INVALID) {
        LOG_MSGID_I(BT_GATTC, "change_discovery_state, invalid state:%d", 1, state);
        return;
    }
    LOG_MSGID_I(BT_GATTC, "change_discovery_state, conn_handle:0x%04x, state:%d", 2, cntx->conn_handle, state);
    cntx->conn_discovery_state = state;
    return;
}

static bt_status_t bt_gattc_discovery_copy_service_uuid(ble_uuid_t *dest, ble_uuid_t *src)
{
    bt_utils_memset(dest, 0x00, sizeof(ble_uuid_t));
    dest->type = src->type;

    if (BLE_UUID_TYPE_16BIT == dest->type) {
        /* uuid is 16 bit. */
        dest->uuid.uuid16 = src->uuid.uuid16;
        //MTK_COMMON: David - reduce log
        //LOG_MSGID_I(BT_GATTC, "copy_service_uuid, uuid16 = 0x%04x", 1, dest->uuid.uuid16);
    } else if (BLE_UUID_TYPE_32BIT == dest->type) {
        /* uuid is 32 bit. */
        dest->uuid.uuid32 = src->uuid.uuid32;
        //MTK_COMMON: David - reduce log
        //LOG_MSGID_I(BT_GATTC, "copy_service_uuid, uuid32 = 0x%04x", 1, dest->uuid.uuid32);
    } else if (BLE_UUID_TYPE_128BIT == dest->type) {
        /* uuid is 128 bit. */
        bt_utils_memcpy(dest->uuid.uuid, src->uuid.uuid, 16);
        //MTK_COMMON: David - reduce log
        /*for (int i = 15; i >= 0; i--) {
            LOG_MSGID_I(BT_GATTC, "copy_service_uuid, uuid128[%d] = 0x%04x", 2, i, dest->uuid.uuid[i]);
        }*/
    } else {
        LOG_MSGID_I(BT_GATTC, "copy_service_uuid error uuid! \r\n", 0);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t bt_gattc_discovery_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{
    //(MTK ReduceLog)LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_event_callback: status(0x%04x), msg(0x%04x)", 2, status, msg);
    if (!g_initialized) {
        return BT_STATUS_SUCCESS;
    }
    switch (msg) {
        case BT_GAP_LE_CONNECT_IND:
            bt_gattc_discovery_save_connection_info(buff);

            break;
        case BT_GAP_LE_DISCONNECT_IND: {
                /* LE disconnected complete. */
                bt_gap_le_disconnect_ind_t *dis_ind = (bt_gap_le_disconnect_ind_t *)buff;
                bt_gattc_discovery_context_t *cntx = bt_gattc_discovery_get_cntx_by_handle(dis_ind->connection_handle);
                if (cntx == NULL) {
                    return BT_STATUS_FAIL;
                }

                if (cntx->conn_discovery_state > BT_GATTC_CONN_DISCOVERY_STATE_IDLE) {
                    bt_gattc_discovery_free_discovery_buffer(cntx->discovery_buffer);
                    cntx->discovery_buffer = NULL;
                }
                bt_gattc_discovery_delete_connection_info(buff);

#ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE
                if (g_discovery_in_progress_rho == 1) {
                    if (!bt_gattc_discovery_check_all_conn_in_progress()) {
                        LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_event_callback, all conn no in progress, call bt_role_handover_reply_prepare_request", 0);
                        g_discovery_in_progress_rho = 0;
                        bt_role_handover_reply_prepare_request(BT_ROLE_HANDOVER_MODULE_GATT_DISCOVERY);
                    }
                }
#endif /* #ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE */
                break;
            }
        case BT_GATTC_DISCOVER_PRIMARY_SERVICE_BY_UUID: {
                /* GATTC discovery primary service by uuid response event. */
                bt_gattc_discovery_primary_service_cnf(status, buff);
                break;
            }
        case BT_GATTC_FIND_INCLUDED_SERVICES: {
                bt_gattc_discovery_find_included_services_cnf(status, buff);
                break;
            }
        case BT_GATTC_DISCOVER_CHARC: {
                /* GATTC discovery charc response event. */
                bt_gattc_discovery_charactiristics_cnf(status, buff);
                break;
            }
        case BT_GATTC_DISCOVER_CHARC_DESCRIPTOR: {
                /* GATTC discovery charc descriptor response event. */
                bt_gattc_discovery_descriptors_cnf(status, buff);
                break;
            }
        default:
            break;
    }
    return BT_STATUS_SUCCESS;
}

static void bt_gattc_discovery_parse_primary_service(bt_gattc_find_by_type_value_rsp_t *rsp, bt_gattc_discovery_context_t *cntx)
{
    uint8_t *attribute_data_list = (uint8_t *) & (rsp->att_rsp->handles_info_list);
    uint8_t num_of_data = (rsp->length - 1) / 4;

    // MTK add, should check pointer before use it
    if (!cntx || !cntx->discovery_buffer) {
        LOG_MSGID_E(BT_GATTC, "context or buf is NULL", 0);
        return;
    }

    bt_gattc_discovery_service_t *serv_buffer = cntx->discovery_buffer;
    bt_gattc_discovery_multiple_instance_t *multi_instance = &cntx->multi_instance;
    LOG_MSGID_I(BT_GATTC, "DiscoveryServiceParse serviceNum: %d, multi_inst_serv_cnt: %d, ", 2, num_of_data, multi_instance->service_count);

    for (uint8_t i = 0; i < num_of_data; i++) {
        LOG_MSGID_I(BT_GATTC, "serv_num: %d, start_hdle = 0x%04x, end_hdle = 0x%04x", 3, i, *(uint16_t *)(attribute_data_list + i * 4), *(uint16_t *)(attribute_data_list + i * 4 + 2));
        LOG_MSGID_I(BT_GATTC, "multi_inst_serv_cnt: %d ", 1, multi_instance->service_count);
        if ((i == 0) && (multi_instance->service_count == 0)) {
            cntx->discovery_buffer->start_handle = *(uint16_t *)(attribute_data_list + i * 4);
            cntx->discovery_buffer->end_handle = *(uint16_t *)(attribute_data_list + i * 4 + 2);
        } else {
            bt_utils_memcpy(&(multi_instance->service_handles[multi_instance->service_count].start_handle), attribute_data_list + i * 4, sizeof(uint16_t));
            bt_utils_memcpy(&(multi_instance->service_handles[multi_instance->service_count].end_handle), attribute_data_list + i * 4 + 2, sizeof(uint16_t));
            bt_gattc_discovery_copy_service_uuid(&(multi_instance->uuid), &(serv_buffer->service_uuid));
            multi_instance->service_count++;
        }
    }
}

static void bt_gattc_discovery_primary_service_cnf(bt_status_t status, void *buff)
{
    uint16_t conn_handle = 0;
    bt_gattc_discovery_context_t *cntx = NULL;
    bt_gattc_find_by_type_value_rsp_t *p_event_data = NULL;
    bt_gattc_error_rsp_t *p_event_error_data = NULL;

    LOG_MSGID_I(BT_GATTC, "primary_service_cnf: status = %d", 1, status);
    if (buff == NULL) {
        LOG_MSGID_E(BT_GATTC, "DiscoveryServiceCnf buff NULL!", 0);
        return;
    }

    if ((status != BT_STATUS_SUCCESS) && (status != BT_ATT_ERRCODE_CONTINUE)) {
        p_event_error_data = (bt_gattc_error_rsp_t *)buff;
        conn_handle = p_event_error_data->connection_handle;
    } else {
        p_event_data = (bt_gattc_find_by_type_value_rsp_t *)buff;
        conn_handle = p_event_data->connection_handle;
    }
    cntx = bt_gattc_discovery_get_cntx_by_handle(conn_handle);
    // MTK add, should check pointer before use it
    if (!cntx || !cntx->discovery_buffer)
        return;

    switch (status) {
        case BT_STATUS_SUCCESS:
        case BT_ATT_ERRCODE_CONTINUE: {
                if (p_event_data->att_rsp) {
                    bt_gattc_discovery_parse_primary_service(p_event_data, cntx);
                }

                if (status == BT_ATT_ERRCODE_CONTINUE) {
                    return;
                }
                break;
            }
        case BT_ATT_ERRCODE_ATTRIBUTE_NOT_FOUND: {
                if (cntx->discovery_buffer->start_handle >= cntx->discovery_buffer->end_handle) {
                    bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_READY);
                    bt_gattc_discovery_result_evt_trigger(cntx, false, BT_GATTC_DISCOVERY_ERROR_SERVICE_NOT_FOUND);
                    // bt_gattc_discovery_primary_service_completed(cntx);
                    return;
                }
                break;
            }
        default: {
                bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_READY);
                bt_gattc_discovery_result_evt_trigger(cntx, false, BT_GATTC_DISCOVERY_ERROR_SERVICE_NOT_FOUND);
                // bt_gattc_discovery_primary_service_completed(cntx);
                return;
            }
    }
    // found success
    if (cntx->discovery_buffer->included_srv_count) {
        bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_SERV);
        bt_gattc_discover_included_service(cntx);
    } else if (cntx->discovery_buffer->characteristic_count) {
        bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_PRIMERY_CHARC);
        bt_gattc_discover_characteristic(cntx);
    } else {
        /**< Trigger Service Not Found event to the application. */
        bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_READY);
        bt_gattc_discovery_result_evt_trigger(cntx, true, 0);
    }
}

static void bt_gattc_discovery_parse_inc_service(bt_gattc_read_by_type_rsp_t *rsp, bt_gattc_discovery_context_t *cntx)
{
    uint32_t num_of_data;
    uint32_t uuid_size;
    uint32_t pair_len;
    ble_uuid_type_t type;;
    uint8_t *attribute_data_list;

    pair_len = rsp->att_rsp->length;
    if (pair_len < 8) {
        return;
    }
    uuid_size = pair_len - 6;
    LOG_MSGID_I(BT_GATTC, "parse_included_service uuid_size: %d", 1, uuid_size);
    if (BT_GATT_DISCOVERY_UUID_16BTS_SIZE == uuid_size) {
        type = BLE_UUID_TYPE_16BIT;
    } else if (BT_GATT_DISCOVERY_UUID_32BTS_SIZE == uuid_size) {
        type = BLE_UUID_TYPE_32BIT;
    } else if (BT_GATT_DISCOVERY_UUID_128BTS_SIZE == uuid_size) {
        type = BLE_UUID_TYPE_128BIT;
    } else {
        return;
    }
    attribute_data_list = rsp->att_rsp->attribute_data_list;
    num_of_data = (rsp->length - 2) / pair_len;
    bt_gattc_discovery_service_t *serv_buffer = cntx->discovery_buffer;
    bt_gattc_discovery_included_service_t *inc_serv_buffer = serv_buffer->included_service;

    LOG_MSGID_I(BT_GATTC, "DiscoveryIncServiceParse inc_serv_num:%d, inc_serv_count: %d, inc_serv_count_found: %d", 3,
                num_of_data, serv_buffer->included_srv_count, serv_buffer->included_srv_count_found);
#if 0
    if (inc_serv_buffer->char_count < num_of_data + inc_serv_buffer->char_count_found) {
        num_of_data = inc_serv_buffer->char_count - inc_serv_buffer->char_count_found;
        LOG_MSGID_I(BT_GATTC, "inc_serv_count not enouge, cut inc_serv_num to: %d", 1, num_of_data);
    }

    for (uint8_t i = 0 ; i < num_of_data; i++) {
        uint16_t *start_handle = &inc_serv_buffer[serv_buffer->included_srv_count_found].start_handle;
        uint16_t *end_handle = &inc_serv_buffer[serv_buffer->included_srv_count_found].end_handle;
        ble_uuid_t *serv_uuid = &inc_serv_buffer[serv_buffer->included_srv_count_found].service_uuid;
        serv_uuid->type = type;
        bt_utils_memcpy(start_handle, attribute_data_list + (i * pair_len) + 2, sizeof(uint16_t));
        bt_utils_memcpy(end_handle, attribute_data_list + (i * pair_len) + 4, sizeof(uint16_t));
        bt_utils_memcpy(&serv_uuid->uuid, attribute_data_list + (i * pair_len) + 6, uuid_size);
        LOG_MSGID_I(BT_GATTC, "inc_serv_num: %d start_handle = 0x%04x, end_handle = 0x%04x, uuid:0x%04x", 4,
                    i, *start_handle, *end_handle, serv_uuid->uuid);
        serv_buffer->included_srv_count_found++;
    }
#endif /* #if 0 */
    for (uint8_t i = 0 ; i < num_of_data; i++) {
        LOG_MSGID_I(BT_GATTC, "inc_serv_num: %d, uuid:0x%04x, start_handle = 0x%04x, end_handle = 0x%04x, ", 4,
                    i, *(uint16_t *)(attribute_data_list + (i * pair_len) + 6), *(uint16_t *)(attribute_data_list + (i * pair_len) + 2), *(uint16_t *)(attribute_data_list + (i * pair_len) + 4));
        for (uint8_t j = 0; j < serv_buffer->included_srv_count; j++) {
            uint16_t *start_handle = &inc_serv_buffer[j].start_handle;
            uint16_t *end_handle = &inc_serv_buffer[j].end_handle;
            ble_uuid_t *serv_uuid = &inc_serv_buffer[j].service_uuid;
            uint8_t *instance_count = &inc_serv_buffer[j].multi_instance_count;
            serv_uuid->type = type;

            if (bt_utils_memcmp(&serv_uuid->uuid, attribute_data_list + (i * pair_len) + 6, uuid_size) == 0) {
                LOG_MSGID_I(BT_GATTC, "found inc_serv uuid = 0x%04x, inc_serv_buffer_index: %d, start_handle = 0x%04x, end_handle = 0x%04x, multi_instance_count = %d", 5,
                            *(uint16_t *)(attribute_data_list + (i * pair_len) + 6), j, *start_handle, *end_handle, *instance_count);
                *instance_count += 1;
                if (*start_handle >= *end_handle) {
                    LOG_MSGID_I(BT_GATTC, "copy inc_serv start handle & end handle", 0);
                    bt_utils_memcpy(start_handle, attribute_data_list + (i * pair_len) + 2, sizeof(uint16_t));
                    bt_utils_memcpy(end_handle, attribute_data_list + (i * pair_len) + 4, sizeof(uint16_t));
                    serv_buffer->included_srv_count_found++;
                    break;
                }
            }
        }
    }
}

static void bt_gattc_discovery_find_included_services_cnf(bt_status_t status, void *buff)
{
    uint16_t conn_handle = 0;
    bt_gattc_discovery_context_t *cntx = NULL;
    bt_gattc_read_by_type_rsp_t *p_event_data = NULL;
    bt_gattc_error_rsp_t *p_event_error_data = NULL;

    LOG_MSGID_I(BT_GATTC, "find_included_services_cnf: status = %d", 1, status);
    if (buff == NULL) {
        LOG_MSGID_E(BT_GATTC, "DiscoveryIncServiceCnf buff NULL!", 0);
        return;
    }

    if ((status != BT_STATUS_SUCCESS) && (status != BT_ATT_ERRCODE_CONTINUE)) {
        p_event_error_data = (bt_gattc_error_rsp_t *)buff;
        conn_handle = p_event_error_data->connection_handle;
    } else {
        p_event_data = (bt_gattc_read_by_type_rsp_t *)buff;
        conn_handle = p_event_data->connection_handle;
    }
    cntx = bt_gattc_discovery_get_cntx_by_handle(conn_handle);
    if (NULL == cntx) {
        return;
    }

    switch (status) {
        case BT_STATUS_SUCCESS:
        case BT_ATT_ERRCODE_CONTINUE: {
                if (p_event_data->att_rsp) {
                    bt_gattc_discovery_parse_inc_service(p_event_data, cntx);
                }

                if (status == BT_ATT_ERRCODE_CONTINUE) {
                    return;
                }
                break;
            }
        case BT_ATT_ERRCODE_ATTRIBUTE_NOT_FOUND: {
                LOG_MSGID_I(BT_GATTC, "disc_inc_serv_cnf: not found, included_srv_count_found = %d", 1, cntx->discovery_buffer->included_srv_count_found);

                break;
            }
        default: {
                bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_READY);
                bt_gattc_discovery_result_evt_trigger(cntx, false, BT_GATTC_DISCOVERY_ERROR_INC_SERV_FOUND_FAIL);
                return;
            }
    }
    // found success
    if (cntx->discovery_buffer->included_srv_count_found) {
        bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_CHARC);
        bt_gattc_discover_characteristic(cntx);
    } else {
        /**< Trigger Service Not Found event to the application. */
        bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_READY);
        bt_gattc_discover_characteristic(cntx);
    }
}

static void bt_gattc_discovery_parse_char_command_data(bt_gattc_discovery_characteristic_t *char_info, uint8_t *attribute_data_list)
{
    if (attribute_data_list) {
        bt_utils_memcpy(&(char_info->handle), attribute_data_list, 2);
        bt_utils_memcpy(&(char_info->property), attribute_data_list + 2, 1);
        bt_utils_memcpy(&(char_info->value_handle), attribute_data_list + 3, 2);
        //MTK_COMMON: David - reduce log
        //LOG_MSGID_I(BT_GATTC, "char_handle = 0x%04x, value_handle = 0x%04x, property = %d", 3, char_info->handle,
        //            char_info->value_handle, char_info->property);
    }
}

static void bt_gattc_discovery_parse_characteristic(bt_gattc_read_by_type_rsp_t *data, bt_gattc_discovery_context_t *cntx)
{
    bt_gattc_read_by_type_rsp_t *rsp = (bt_gattc_read_by_type_rsp_t *)data;
    bt_gattc_discovery_characteristic_t *char_info, *p_charateristics;
    uint8_t *attribute_data_list = rsp->att_rsp->attribute_data_list;
    uint8_t num_of_data = (rsp->length - 2) / rsp->att_rsp->length;
    uint8_t characteristic_count, *p_char_count_found;

    /* MTK_David: should check pointer before use it */
    if (!cntx || !cntx->discovery_buffer)
        return;

    if (cntx->conn_discovery_state == BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_CHARC) {
        bt_gattc_discovery_included_service_t *inc_serv_buffer = cntx->discovery_buffer->included_service + cntx->discovering_inc_serv_index;
        p_charateristics = inc_serv_buffer->charateristics;
        characteristic_count = inc_serv_buffer->char_count;
        p_char_count_found = &inc_serv_buffer->char_count_found;
    } else {
        bt_gattc_discovery_service_t *serv_buffer = cntx->discovery_buffer;
        p_charateristics = serv_buffer->charateristics;
        characteristic_count = serv_buffer->characteristic_count;
        p_char_count_found = &serv_buffer->char_count_found;
    }

    LOG_MSGID_I(BT_GATTC, "parse_char : char_num = %d, char_count = %d, char_count_found = %d", 3, num_of_data,
                characteristic_count, *p_char_count_found);

    if (characteristic_count < (*p_char_count_found + num_of_data)) {
        num_of_data = characteristic_count - *p_char_count_found;
        LOG_MSGID_W(BT_GATTC, "parse_char char_count not enough, cut num to: %d", 1, num_of_data);
    }

    if (rsp->att_rsp->length < 20) {
        /* 16+2 + 1 +1,uuid is 16 bit. */
        for (uint8_t i = 0 ; i < num_of_data; i++) {
            char_info = p_charateristics + i + *p_char_count_found;
            bt_gattc_discovery_parse_char_command_data(char_info, (rsp->att_rsp->attribute_data_list + i * rsp->att_rsp->length));
            char_info->char_uuid.type = BLE_UUID_TYPE_16BIT;
            bt_utils_memcpy(&(char_info->char_uuid.uuid.uuid16), attribute_data_list + i * rsp->att_rsp->length + 5, 2);
            //MTK_COMMON: David - reduce log
            //LOG_MSGID_I(BT_GATTC, "char_index = %d, char_uuid16 = 0x%04x", 2, i, char_info->char_uuid.uuid.uuid16);
        }
        LOG_MSGID_I(BT_GATTC, "char parser done total %d", 1, num_of_data);
    } else {
        /* uuid is 128 bit. */
        for (uint8_t j = 0 ; j < num_of_data; j++) {
            char_info = p_charateristics + j + *p_char_count_found;
            bt_gattc_discovery_parse_char_command_data(char_info, (rsp->att_rsp->attribute_data_list + j * rsp->att_rsp->length));
            char_info->char_uuid.type = BLE_UUID_TYPE_128BIT;
            bt_utils_memcpy(&(char_info->char_uuid.uuid.uuid), attribute_data_list + j * rsp->att_rsp->length + 5, 16);
            LOG_MSGID_I(BT_GATTC, "char_uuid1128 is", 0);
            /* (MTK ReduceLog)
            for (int i = 15; i >= 0; i--) {
                LOG_MSGID_I(BT_GATTC, "0x%02x", 1, char_info->char_uuid.uuid.uuid[i]);
            }
            LOG_MSGID_I(BT_GATTC, "\n", 0);*/
        }
    }
    *p_char_count_found += num_of_data;
    //LOG_MSGID_I(BT_GATTC, "ble_gattc_parse_characteristic : char_count_found = %d", 1, *p_char_count_found);
}

static void bt_gattc_discovery_charactiristics_cnf(bt_status_t status, void *buff)
{
    uint8_t *p_char_count_found;
    uint16_t conn_handle = 0;
    bt_gattc_discovery_context_t *cntx = NULL;
    bt_gattc_read_by_type_rsp_t *p_event_data = NULL;
    bt_gattc_error_rsp_t *p_event_error_data = NULL;

    LOG_MSGID_I(BT_GATTC, "char_cnf: status = %d", 1, status);
    if (buff == NULL) {
        LOG_MSGID_E(BT_GATTC, "Discovery charactiristics Cnf  buff NULL!", 0);
        return;
    }

    if ((status != BT_STATUS_SUCCESS) && (status != BT_ATT_ERRCODE_CONTINUE)) {
        p_event_error_data = (bt_gattc_error_rsp_t *)buff;
        conn_handle = p_event_error_data->connection_handle;
    } else {
        p_event_data = (bt_gattc_read_by_type_rsp_t *)buff;
        conn_handle = p_event_data->connection_handle;
    }
    cntx = bt_gattc_discovery_get_cntx_by_handle(conn_handle);
    // MTK add, should check pointer before use it
    if (!cntx || !cntx->discovery_buffer)
        return;

    if (cntx->conn_discovery_state == BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_CHARC) {
        p_char_count_found = &cntx->discovery_buffer->included_service[cntx->discovering_inc_serv_index].char_count_found;
    } else {
        p_char_count_found = &cntx->discovery_buffer->char_count_found;
    }

    switch (status) {
        case BT_STATUS_SUCCESS:
        case BT_ATT_ERRCODE_CONTINUE: {
                if (p_event_data->att_rsp) {
                    bt_gattc_discovery_parse_characteristic(p_event_data, cntx);
                }

                if (status == BT_ATT_ERRCODE_CONTINUE) {
                    return;
                }
                break;
            }
        case BT_ATT_ERRCODE_ATTRIBUTE_NOT_FOUND: {

                LOG_MSGID_I(BT_GATTC, "disc_char_cnf: not found, char_num_found = %d", 1, *p_char_count_found);
                break;
            }
        default: {
                bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_READY);
                bt_gattc_discovery_result_evt_trigger(cntx, false, BT_GATTC_DISCOVERY_ERROR_CHAR_FOUND_FAIL);
                return;
            }
    }

    if (*p_char_count_found) {
        if (cntx->conn_discovery_state == BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_CHARC) {
            bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_DESCR);
        } else {
            bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_PRIMERY_DESCR);
        }
        bt_gattc_discover_descriptor(cntx);
    } else {
        /**< Trigger Service Not Found event to the application. */
        if (cntx->conn_discovery_state == BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_CHARC) {
            bt_gattc_discovery_included_service_completed(cntx);
        } else {
            bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_READY);
            bt_gattc_discovery_result_evt_trigger(cntx, true, 0);
        }
    }
}

static void bt_gattc_discovery_parse_descriptor(bt_gattc_find_info_rsp_t *data, bt_gattc_discovery_characteristic_t *chara)
{
    uint8_t format = 0;
    uint8_t attribute_length = 0;
    uint8_t i;
    bt_gattc_find_info_rsp_t rsp = *data;
    bt_gattc_discovery_descriptor_t *descr_info;
    if (rsp.att_rsp) {
        format = rsp.att_rsp->format;
    }
    if (format == 0x02) {
        attribute_length = 18;
    } else {
        attribute_length = 4;
    }
    uint8_t num_of_attribute = (rsp.length - 2) / attribute_length;
    LOG_MSGID_I(BT_GATTC, "parse_desc, descr_num = %d,  descr_count = %d, descr_count_found = %d", 3,
                num_of_attribute, chara->descriptor_count, chara->descr_count_found);
    if (chara->descriptor_count < chara->descr_count_found + num_of_attribute) {
        num_of_attribute = chara->descriptor_count - chara->descr_count_found;
        LOG_MSGID_I(BT_GATTC, "parse_descriptor descr_count not enough, cut descr_num to: %d", 1, num_of_attribute);
    }

    for (i = 0; i < num_of_attribute; i++) {
        descr_info = chara->descriptor + i + chara->descr_count_found;
        if (format == 0x02) {
            /* uuid 128 */
            bt_utils_memcpy(&(descr_info->handle), rsp.att_rsp->info_data + i * attribute_length, 2);
            descr_info->descriptor_uuid.type = BLE_UUID_TYPE_128BIT;
            bt_utils_memcpy(&(descr_info->descriptor_uuid.uuid.uuid), rsp.att_rsp->info_data + i * attribute_length + 2, 16);
        } else {
            /* uuid 16 */
            bt_utils_memcpy(&(descr_info->handle), rsp.att_rsp->info_data + i * attribute_length, 2);
            descr_info->descriptor_uuid.type = BLE_UUID_TYPE_16BIT;
            bt_utils_memcpy(&(descr_info->descriptor_uuid.uuid.uuid16), rsp.att_rsp->info_data + i * attribute_length + 2, 2);
            LOG_MSGID_I(BT_GATTC, "descr_uuid16 = 0x%04x", 1, descr_info->descriptor_uuid.uuid.uuid16);
        }
        LOG_MSGID_I(BT_GATTC, "attribute handle = 0x%04x", 1, descr_info->handle);
    }
    chara->descr_count_found += num_of_attribute;
    //LOG_MSGID_I(BT_GATTC, "parse_desc, descr_count_found: %d", 1, chara->descr_count_found); //MTK_COMMON: David - reduce log
}

static void bt_gattc_discovery_descriptors_cnf(bt_status_t status, void *buff)
{
    uint16_t conn_handle = 0;
    bt_gattc_discovery_context_t *cntx = NULL;
    bt_gattc_find_info_rsp_t *p_event_data = NULL;
    bt_gattc_error_rsp_t *p_event_error_data = NULL;
    bt_gattc_discovery_characteristic_t *chara = NULL;

    LOG_MSGID_I(BT_GATTC, "desc_cnf: status = %d", 1, status);
    if (buff == NULL) {
        LOG_MSGID_E(BT_GATTC, "Discovery charactiristics Cnf  buff NULL!\r\n", 0);
        return;
    }

    if ((status != BT_STATUS_SUCCESS) && (status != BT_ATT_ERRCODE_CONTINUE)) {
        p_event_error_data = (bt_gattc_error_rsp_t *)buff;
        conn_handle = p_event_error_data->connection_handle;
    } else {
        p_event_data = (bt_gattc_find_info_rsp_t *)buff;
        conn_handle = p_event_data->connection_handle;
    }
    cntx = bt_gattc_discovery_get_cntx_by_handle(conn_handle);
    // MTK add, should check pointer before use it
    if (!cntx || !cntx->discovery_buffer)
        return;

    bt_gattc_discovery_service_t *serv_buffer = cntx->discovery_buffer;
    bt_gattc_discovery_included_service_t *inc_serv_buffer = serv_buffer->included_service;

    if (cntx->conn_discovery_state == BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_DESCR) {
        if (cntx->discovering_char_index < inc_serv_buffer->char_count_found) {
            chara = inc_serv_buffer->charateristics + cntx->discovering_char_index;
        }
    } else {
        if (cntx->discovering_char_index < serv_buffer->char_count_found) {
            chara = serv_buffer->charateristics + cntx->discovering_char_index;
        }
    }

    switch (status) {
        case BT_STATUS_SUCCESS: {
                LOG_MSGID_I(BT_GATTC, "desc_cnf: succsee:index = %d", 1, cntx->discovering_char_index);
                if ((p_event_data->att_rsp) && (chara)) {
                    bt_gattc_discovery_parse_descriptor(p_event_data, chara);
                }
                cntx->discovering_char_index++;
                bt_gattc_discovery_all_descriptor(cntx, cntx->discovering_char_index);
                break;
            }
        case BT_ATT_ERRCODE_CONTINUE: {
                LOG_MSGID_I(BT_GATTC, "desc_cnf: continue:index = %d", 1, cntx->discovering_char_index);
                if (chara) {
                    bt_gattc_discovery_parse_descriptor(p_event_data, chara);
                }
                break;
            }
        case BT_ATT_ERRCODE_ATTRIBUTE_NOT_FOUND: {
                LOG_MSGID_I(BT_GATTC, "desc_cnf: fail:index = %d", 1, cntx->discovering_char_index);
                cntx->discovering_char_index++;
                bt_gattc_discovery_all_descriptor(cntx, cntx->discovering_char_index);
                break;
            }
        default: {
                bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_READY);
                bt_gattc_discovery_result_evt_trigger(cntx, false, BT_GATTC_DISCOVERY_ERROR_DESCRIPTOR_FOUND_FAIL);
                return;
            }
    }
}

static bt_status_t bt_gattc_discovery_start_discover_service(uint16_t conn_handle, ble_uuid_t *srv_uuid)
{
    uint8_t buff[40];
    bt_gattc_discover_primary_service_by_uuid_req_t req;
    LOG_MSGID_I(BT_GATTC, "start_disc_srv: conn_handle is 0x%04x, uuid: 0x%04x", 2, conn_handle, srv_uuid->uuid.uuid16);
    req.att_req = (bt_att_find_by_type_value_req_t *)buff;
    if (BLE_UUID_TYPE_16BIT == srv_uuid->type) {
        req.attribute_value_length = 2;
        bt_utils_memcpy(&req.att_req->attribute_value, &(srv_uuid->uuid.uuid16), 2);
    } else if (BLE_UUID_TYPE_32BIT == srv_uuid->type) {
        req.attribute_value_length = 4;
        bt_utils_memcpy(&req.att_req->attribute_value, &(srv_uuid->uuid.uuid32), 4);
    } else if (BLE_UUID_TYPE_128BIT == srv_uuid->type) {
        req.attribute_value_length = 16;
        bt_utils_memcpy(&req.att_req->attribute_value, &(srv_uuid->uuid.uuid), 16);
    }
    req.att_req->opcode = BT_ATT_OPCODE_FIND_BY_TYPE_VALUE_REQUEST;
    req.att_req->starting_handle = SRV_DISC_START_HANDLE;
    req.att_req->ending_handle = SRV_DISC_END_HANDLE;
    req.att_req->uuid = BT_GATT_UUID16_PRIMARY_SERVICE;
    return bt_gattc_discover_primary_service_by_uuid(conn_handle, &req);
}

bt_gattc_discovery_status_t bt_gattc_discovery_start_internal(bt_gattc_discovery_context_t *cntx)
{
    int32_t ret;
    bt_gattc_registered_handlers serv_registered = g_registered_handlers[cntx->discovering_serv_index];
    //LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_start_internal: uuid = 0x%04x, user = %d", 2, serv_registered.service_uuid.uuid.uuid16, serv_registered.user_registered);

    if (BT_STATUS_SUCCESS != bt_gattc_discovery_get_discovery_buffer(&cntx->discovery_buffer, serv_registered.service_info)) {
        ret = BT_GATTC_DISCOVERY_STATUS_FAIL;
        return ret;
    }
    bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_PRIMERY_SERV);
    ret = bt_gattc_discovery_start_discover_service(cntx->conn_handle, &serv_registered.service_uuid);
    return ret;
}

static bt_status_t bt_gattc_discovery_start_discover_included_service(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
    bt_status_t status;
    LOG_MSGID_I(BT_GATTC, "start_disc_included_service: start~end handle = 0x%04x ~ 0x%04x", 2, start_handle, end_handle);
    if (SRV_DISC_INVALID_HANDLE == start_handle || SRV_DISC_INVALID_HANDLE == end_handle) {
        return BT_STATUS_FAIL;
    }

    BT_GATTC_NEW_FIND_INCLUDED_SERVICE_REQ(req, start_handle, end_handle);
    status = bt_gattc_find_included_services(conn_handle, &req);
    LOG_MSGID_I(BT_GATTC, "dis_inc_serv status: %d", 1, status);
    return status;
}

static void bt_gattc_discover_included_service(bt_gattc_discovery_context_t *cntx)
{
    bt_status_t sta;
    uint16_t conn_handle = cntx->conn_handle;
    bt_gattc_discovery_service_t *discovery_buffer = cntx->discovery_buffer;

    sta = bt_gattc_discovery_start_discover_included_service(conn_handle, discovery_buffer->start_handle, discovery_buffer->end_handle);
    if (BT_STATUS_SUCCESS != sta) {
        /**< Error with discovering the service. Indicate the error to the registered user application. */
        bt_gattc_discovery_change_discovery_state(cntx,  BT_GATTC_CONN_DISCOVERY_STATE_READY);
        bt_gattc_discovery_result_evt_trigger(cntx, false, BT_GATTC_DISCOVERY_ERROR_INC_SERV_FOUND_FAIL);
    }
}

static bt_status_t bt_gattc_discovery_start_discover_characteristics(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
    bt_status_t status;
    bt_gattc_discover_charc_req_t req;
    LOG_MSGID_I(BT_GATTC, "disc_char_start: start~end handle = 0x%04x ~ 0x%04x", 2, start_handle, end_handle);
    uint16_t uuid_16 = BT_GATT_UUID16_CHARC;
    req.opcode = BT_ATT_OPCODE_READ_BY_TYPE_REQUEST;
    req.starting_handle = start_handle;
    req.ending_handle = end_handle;
    bt_uuid_load(&(req.type), (void *)&uuid_16, 2);
    status = bt_gattc_discover_charc(conn_handle, &req);
    //LOG_MSGID_I(BT_GATTC, "disc_char end: status = %d", 1, status);
    return status;
}

static void bt_gattc_discover_characteristic(bt_gattc_discovery_context_t *cntx)
{
    bt_status_t sta;
    uint16_t conn_handle = cntx->conn_handle;
    bt_gattc_discovery_service_t *serv_buffer = NULL;
    uint16_t start_handle, end_handle;

    // MTK add, should check pointer before use it
    if (!cntx || !cntx->discovery_buffer)
        return;

    serv_buffer = cntx->discovery_buffer;
    if (cntx->conn_discovery_state == BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_CHARC) {
        bt_gattc_discovery_included_service_t *inc_serv_buffer = serv_buffer->included_service + cntx->discovering_inc_serv_index;
        start_handle = inc_serv_buffer->start_handle;
        end_handle = inc_serv_buffer->end_handle;
    } else {
        start_handle = serv_buffer->start_handle;
        end_handle = serv_buffer->end_handle;
    }
    sta = bt_gattc_discovery_start_discover_characteristics(conn_handle, start_handle, end_handle);
    if (BT_STATUS_SUCCESS != sta) {
        /**< Error with discovering the service. Indicate the error to the registered user application. */
        bt_gattc_discovery_change_discovery_state(cntx,  BT_GATTC_CONN_DISCOVERY_STATE_READY);
        bt_gattc_discovery_result_evt_trigger(cntx, false, BT_GATTC_DISCOVERY_ERROR_CHAR_FOUND_FAIL);
    }
}

static bt_status_t bt_gattc_discovery_descriptor_of_characteristic(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
    bt_status_t status;
    bt_gattc_discover_charc_descriptor_req_t req;
    LOG_MSGID_I(BT_GATTC, "disc_desc_start: start~end handle = 0x%04x ~ 0x%04x", 2, start_handle, end_handle);
    req.opcode = BT_ATT_OPCODE_FIND_INFORMATION_REQUEST;
    req.starting_handle = start_handle;
    req.ending_handle = end_handle;
    status = bt_gattc_discover_charc_descriptor(conn_handle, &req);
    //LOG_MSGID_I(BT_GATTC, "dis_descr end: status = %d\r\n", 1, status);
    return status;
}

static bt_status_t bt_gattc_discovery_all_descriptor(bt_gattc_discovery_context_t *cntx, uint8_t index)
{
    bt_status_t status = 0;
    bt_gattc_discovery_characteristic_t *cur_char, *next_char, *p_charateristics;
    bt_gattc_discovery_service_t *service = cntx->discovery_buffer;
    uint8_t char_num;
    uint16_t conn_handle = cntx->conn_handle;
    cntx->discovering_char_index = index;

    if (cntx->conn_discovery_state == BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_DESCR) {
        p_charateristics = service->included_service->charateristics;
        char_num = service->included_service->char_count_found;
    } else {
        p_charateristics = service->charateristics;
        char_num = service->char_count_found;
    }

    if (index < char_num) {
        uint16_t start_handle;
        uint16_t end_handle;
        cur_char = p_charateristics + index;
        start_handle = cur_char->value_handle + 1;
        LOG_MSGID_I(BT_GATTC, "find_all_desc conn_handle: 0x%04x, char_index = %d, char_num = %d", 3, conn_handle, index, char_num);
        if (0 == cur_char->descriptor_count) {
            status = bt_gattc_discovery_all_descriptor(cntx, index + 1);
            return status;
        }
        /* This one might be the last characteristic in service. */
        if (index == char_num - 1) {
            end_handle = service->end_handle;
        } else {
            next_char = p_charateristics + index + 1;
            end_handle = next_char->handle - 1;
        }
        LOG_MSGID_I(BT_GATTC, "find_all_desc start~end handle = 0x%04x ~ 0x%04x", 2, start_handle, end_handle);
        if (start_handle <= end_handle) {
            status = bt_gattc_discovery_descriptor_of_characteristic(conn_handle, start_handle, end_handle);
        } else {
            status = bt_gattc_discovery_all_descriptor(cntx, index + 1);
        }
    } else {/*all descriptor is complete done */
        if (cntx->conn_discovery_state == BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_DESCR) {
            LOG_MSGID_I(BT_GATTC, "find_all_desc done, discover included service idx:%d, completed!", 1, cntx->discovering_inc_serv_index);
            bt_gattc_discovery_included_service_completed(cntx);
        } else {
            LOG_MSGID_I(BT_GATTC, "find_all_desc, discover primary service idx:%d completed!", 1, cntx->discovering_serv_index);
            bt_gattc_discovery_change_discovery_state(cntx,  BT_GATTC_CONN_DISCOVERY_STATE_READY);
            bt_gattc_discovery_result_evt_trigger(cntx, true, 0);
            // bt_gattc_discovery_primary_service_completed(cntx);
        }
    }
    return status;
}

static void bt_gattc_discover_descriptor(bt_gattc_discovery_context_t *cntx)
{
    bt_status_t sta;

    sta = bt_gattc_discovery_all_descriptor(cntx, 0);
    if (BT_STATUS_SUCCESS != sta) {
        /**< Error with discovering the service. Indicate the error to the registered user application. */
        bt_gattc_discovery_change_discovery_state(cntx,  BT_GATTC_CONN_DISCOVERY_STATE_READY);
        bt_gattc_discovery_result_evt_trigger(cntx, false, BT_GATTC_DISCOVERY_ERROR_DESCRIPTOR_FOUND_FAIL);
    }
}

static void bt_gattc_discovery_included_service_completed(bt_gattc_discovery_context_t *cntx)
{
    cntx->discovering_inc_serv_index++;
    cntx->discovering_char_index = 0;

    if (cntx->discovering_inc_serv_index < cntx->discovery_buffer->included_srv_count_found) {
        LOG_MSGID_I(BT_GATTC, "included_service_completed, start discover next included service idx:%d!", 1, cntx->discovering_inc_serv_index);
        bt_gattc_discovery_change_discovery_state(cntx,  BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_CHARC);
        bt_gattc_discover_characteristic(cntx);
    } else {
        LOG_MSGID_I(BT_GATTC, "included_service_completed, all include services discover done, next dicover primary charac!", 0);
        bt_gattc_discovery_change_discovery_state(cntx,  BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_PRIMERY_CHARC);
        bt_gattc_discover_characteristic(cntx);
    }
}

static void bt_gattc_discovery_primary_service_completed(bt_gattc_discovery_context_t *cntx)
{
    bt_gattc_discovery_user_t user;
    uint8_t serv_index_next;
    uint8_t serv_index = cntx->discovering_serv_index;
    uint16_t conn_handle = cntx->conn_handle;
    bt_gattc_discovery_multiple_instance_t *multi_instance = &cntx->multi_instance;
    bt_gattc_registered_handlers serv_registered = g_registered_handlers[cntx->discovering_serv_index];
    cntx->discovering_inc_serv_index = 0;
    cntx->discovering_char_index = 0;
    //multi_instance
    if (multi_instance->service_count != 0) {
        LOG_MSGID_I(BT_GATTC, "primary_service_completed, conn_hanlde: 0x%04x, service uuid:0x%04x have multi instance! now dicover multi serv idex:%d", 3,
                    cntx->conn_handle, serv_registered.service_uuid.uuid.uuid16, multi_instance->discovering_multi_serv_index);
        if (BT_STATUS_SUCCESS != bt_gattc_discovery_get_discovery_buffer(&cntx->discovery_buffer, serv_registered.service_info)) {
            LOG_MSGID_E(BT_GATTC, "primary_service_completed, get buffer fail!", 0);
            return;
        }
        cntx->discovery_buffer->start_handle = multi_instance->service_handles[multi_instance->discovering_multi_serv_index].start_handle;
        cntx->discovery_buffer->end_handle = multi_instance->service_handles[multi_instance->discovering_multi_serv_index].end_handle;
        bt_gattc_discovery_copy_service_uuid(&cntx->discovery_buffer->service_uuid, &multi_instance->uuid);
        multi_instance->service_count--;
        if (multi_instance->service_count != 0) {
            multi_instance->discovering_multi_serv_index++;
        }
        bt_gattc_discovery_change_discovery_state(cntx,  BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_PRIMERY_CHARC);
        bt_gattc_discover_characteristic(cntx);
        return;
    }
    //get new uuid
    bt_utils_memset(multi_instance, 0x00, sizeof(bt_gattc_discovery_multiple_instance_t));
    user = g_registered_handlers[cntx->discovering_serv_index].user_registered;
    serv_index_next = bt_gattc_discovery_find_next_resigtered_serv_index_by_user(serv_index + 1, user);
    if (serv_index_next != 0xff) {
        cntx->discovering_serv_index = serv_index_next;
        LOG_MSGID_I(BT_GATTC, "primary_service_completed, conn_hanlde: 0x%04x, now dicover next service idex:%d!", 2,
                    cntx->conn_handle, cntx->discovering_serv_index);
        if (bt_gattc_discovery_start_internal(cntx) != BT_STATUS_SUCCESS) {
            LOG_MSGID_E(BT_GATTC, "primary_service_completed, assert!!", 0);
            BT_ASSERT(0 && "discovery fail in event response");
        }
        return;
    }
    //get new trigger user
    user = bt_gattc_discovery_get_user_triggered(conn_handle);
    if (BT_GATTC_DISCOVERY_USER_NONE != user) {
        serv_index_next = bt_gattc_discovery_find_next_resigtered_serv_index_by_user(0, user);
        if (serv_index_next != 0xff) {
            cntx->discovering_serv_index = serv_index_next;
            LOG_MSGID_I(BT_GATTC, "primary_service_completed, conn_hanlde: 0x%04x, now dicover service idex:%d by new trigger user: %d", 3,
                        cntx->conn_handle, cntx->discovering_serv_index, user);
            if (bt_gattc_discovery_start_internal(cntx) != BT_STATUS_SUCCESS) {
                LOG_MSGID_E(BT_GATTC, "primary_service_completed, assert!!", 0);
                BT_ASSERT(0 && "discovery fail in event response");
            }
            return;
        }
    }

    bt_gattc_discovery_change_discovery_state(cntx,  BT_GATTC_CONN_DISCOVERY_STATE_IDLE);
    LOG_MSGID_I(BT_GATTC, "primary_service_completed, conn_hanlde: 0x%04x, all discover done and no pending trigger user!", 1, cntx->conn_handle);

#ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE
    if (g_discovery_in_progress_rho == 1) {
        if (!bt_gattc_discovery_check_all_conn_in_progress()) {
            LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_primary_service_completed, all conn no in progress, call bt_role_handover_reply_prepare_request", 0);
            g_discovery_in_progress_rho = 0;
            bt_role_handover_reply_prepare_request(BT_ROLE_HANDOVER_MODULE_GATT_DISCOVERY);
        }
    }
#endif /* #ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE */
    return;
}

static void bt_gattc_discovery_result_evt_trigger(bt_gattc_discovery_context_t *cntx, bool is_complete, int32_t err_code)
{
    bt_gattc_registered_handlers serv_registered = g_registered_handlers[cntx->discovering_serv_index];
    bt_gattc_discovery_event_t result_event;
    LOG_MSGID_I(BT_GATTC, "result_evt_trigger, conn: 0x%04x, is_complete: %d, err_code: %d", 3, cntx->conn_handle, is_complete, err_code);

    if (!is_complete) {
        result_event.event_type = BT_GATTC_DISCOVERY_EVENT_FAIL;
        result_event.conn_handle = cntx->conn_handle;
        result_event.last_instance = TRUE;
        result_event.params.error_code = err_code;
        goto SEND_EVENT;
    }
    //is comlpeted
    bt_gattc_discovery_copy_discovery_buffer(serv_registered.service_info, cntx->discovery_buffer);
    result_event.event_type = BT_GATTC_DISCOVERY_EVENT_COMPLETE;
    result_event.conn_handle = cntx->conn_handle;
    result_event.params.discovered_db = serv_registered.service_info;
    if (cntx->multi_instance.service_count == 0) {
        result_event.last_instance = TRUE;
    } else {
        result_event.last_instance = FALSE;
    }

SEND_EVENT:
    LOG_MSGID_I(BT_GATTC, "bresult_evt_trigger, send event: 0x%x to user: %d", 2, result_event.event_type, serv_registered.user_registered);
    bt_gattc_discovery_free_discovery_buffer(cntx->discovery_buffer);
    cntx->discovery_buffer = NULL;
    serv_registered.event_handler(&result_event);
}

#ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE
static bt_status_t bt_gattc_discovery_rho_is_allowed(const bt_bd_addr_t *addr)
{
    uint8_t i;

    for (i = 0; i < BT_LE_CONNECTION_NUM; i++) {
        if (g_discovery_context[i].conn_discovery_state > BT_GATTC_CONN_DISCOVERY_STATE_IDLE) {
            LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_rho_is_allowed, discovery is in progress, conn_handle: 0x%04x", 1, g_discovery_context[i].conn_handle);
#ifdef AIR_LE_AUDIO_ENABLE
            if (bt_gap_le_srv_get_link_attribute_by_handle(g_discovery_context[i].conn_handle) == BT_GAP_LE_SRV_LINK_ATTRIBUTE_NOT_NEED_RHO) {
                LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_rho_is_allowed, connection handle not need rho", 0);
                continue;
            }
#endif /* #ifdef AIR_LE_AUDIO_ENABLE */
            g_discovery_in_progress_rho = 1;
            return BT_STATUS_PENDING;
        }
    }
    return BT_STATUS_SUCCESS;
}

static void bt_gattc_discovery_rho_status_notify(const bt_bd_addr_t *addr, bt_aws_mce_role_t role,
                                                 bt_role_handover_event_t event, bt_status_t status)
{
    switch (event) {
        case BT_ROLE_HANDOVER_COMPLETE_IND: {
                g_discovery_rho_state = 0;
                LOG_MSGID_I(BT_GATTC, "bt gattc discovery rho complete, status = %02x, role = %02x", 2, status, role);
            }
            break;
        case BT_ROLE_HANDOVER_START_IND: {
                g_discovery_rho_state = 1;
            }
        default:
            break;
    }
}
static bt_role_handover_callbacks_t role_cb = {
    &bt_gattc_discovery_rho_is_allowed,
    NULL,
    NULL,
    NULL,
    &bt_gattc_discovery_rho_status_notify
};

static bt_status_t bt_gattc_discovery_rho_init(void)
{
    LOG_MSGID_I(BT_GATTC, "bt gattc discovery rho register", 0);
    bt_status_t status = bt_role_handover_register_callbacks(BT_ROLE_HANDOVER_MODULE_GATT_DISCOVERY, &role_cb);
    if (status != BT_STATUS_SUCCESS) {
        LOG_MSGID_I(BT_GATTC, "bt gattc discovery rho register fail = %02x", 1, status);
    }
    return BT_STATUS_SUCCESS;
}
#endif /* #ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE */

/************************************************
*   Public
*************************************************/
bt_status_t bt_gattc_discovery_init(void)
{
    LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_init", 0);
    if (g_initialized) {
        return BT_STATUS_FAIL;
    }
    g_initialized = true;
    bt_gattc_discovery_reset_context();
#ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE
    bt_gattc_discovery_rho_init();
#endif /* #ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE */

    return bt_callback_manager_register_callback(bt_callback_type_app_event, MODULE_MASK_GAP | MODULE_MASK_GATT, (void *)bt_gattc_discovery_event_callback);
}

bt_gattc_discovery_status_t bt_gattc_discovery_register_service(bt_gattc_discovery_user_t user, bt_gattc_discovery_user_data_t *user_data)
{
    LOG_MSGID_I(BT_GATTC, "register_service: user: %d, uuid: 0x%04x, need_cache: %d, g_num_of_handlers_reg: %d", 4, user, user_data->uuid.uuid.uuid16, user_data->need_cache, g_num_of_handlers_reg);
    if (g_num_of_handlers_reg < BT_GATTC_DISCOVERY_MAX_USERS) {
        if (BT_STATUS_SUCCESS != bt_gattc_discovery_copy_service_uuid(&(g_registered_handlers[g_num_of_handlers_reg].service_uuid), &(user_data->uuid))) {
            LOG_MSGID_E(BT_GATTC, "registered_handler_set error uuid!", 0);
            return BT_GATTC_DISCOVERY_STATUS_INVALID_UUID;
        }
        g_registered_handlers[g_num_of_handlers_reg].service_info = user_data->srv_info;
        g_registered_handlers[g_num_of_handlers_reg].event_handler = user_data->handler;
        g_registered_handlers[g_num_of_handlers_reg].user_registered = user;

        g_num_of_handlers_reg++;
        return BT_GATTC_DISCOVERY_STATUS_SUCCESS;
    } else {
        return BT_GATTC_DISCOVERY_STATUS_OUT_OF_MEMORY;
    }
}

bt_gattc_discovery_status_t bt_gattc_discovery_start(bt_gattc_discovery_user_t user, uint16_t conn_handle, bool refresh)
{
    int32_t err_code;
    bt_gattc_discovery_context_t *cntx;

    LOG_MSGID_I(BT_GATTC, "disc_start: user: 0x%x, conn_handle: 0x%04x", 2, user, conn_handle);
    if (!g_initialized) {
        /* Please call bt_gattc_discovery_init first. */
        return BT_GATTC_DISCOVERY_STATUS_INVALID_STATE;
    }
    if (g_num_of_handlers_reg == 0) {
        /* No user modules were registered. There are no services to discover. */
        return BT_GATTC_DISCOVERY_STATUS_INVALID_STATE;
    }

#ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE
    if (g_discovery_rho_state == 1) {
        LOG_MSGID_I(BT_GATTC, "bt_gattc_discovery_start discovery in rho, g_discovery_rho_state = %d\r\n", 1, g_discovery_rho_state);
        if (bt_gap_le_srv_get_link_attribute_by_handle(conn_handle) == BT_GAP_LE_SRV_LINK_ATTRIBUTE_NOT_NEED_RHO) {
            LOG_MSGID_I(BT_APP, "[BT][GATT] conenction handle = %02x is le audio, no need rho, continue discovery", 1, conn_handle);
            return BT_GATTC_DISCOVERY_STATUS_SUCCESS;
        }
        return BT_GATTC_DISCOVERY_STATUS_FAIL;
    }
#endif /* #ifdef BT_ROLE_HANDOVER_WITH_SPP_BLE */

    cntx = bt_gattc_discovery_get_cntx_by_handle(conn_handle);
    if (cntx == NULL) {
        /* not connection established */
        return BT_GATTC_DISCOVERY_STATUS_INVALID_STATE;
    }

    if (cntx->conn_discovery_state > BT_GATTC_CONN_DISCOVERY_STATE_IDLE) {
        LOG_MSGID_I(BT_GATTC, "disc_start in progress, state: %d, conn: 0x%x", 2, cntx->conn_discovery_state, cntx->conn_handle);
        if (BT_STATUS_SUCCESS != bt_gattc_discovery_add_user_triggered(conn_handle, user)) {
            return BT_GATTC_DISCOVERY_STATUS_INVALID_STATE;
        }
        return BT_GATTC_DISCOVERY_STATUS_BUSY;
    }

    uint8_t serv_index = bt_gattc_discovery_find_next_resigtered_serv_index_by_user(0x00, user);
    if (serv_index == 0xff) {
        LOG_MSGID_W(BT_GATTC, "disc_start, not found registered serv service by user:%d", 1, user);
        return BT_GATTC_DISCOVERY_STATUS_INVALID_STATE;
    } else {
        cntx->discovering_serv_index = serv_index;
    }

    /* TODO:
    if (refresh) {
        get discovery cache
        ...
    }
    */
    bt_gattc_discovery_change_discovery_state(cntx, BT_GATTC_CONN_DISCOVERY_STATE_READY);
    err_code = bt_gattc_discovery_start_internal(cntx);
    if (err_code != BT_STATUS_SUCCESS) {
        LOG_MSGID_E(BT_GATTC, "bt_gattc_discovery_start_interval: err_code = 0x%04x", 1, err_code);
        return BT_GATTC_DISCOVERY_STATUS_FAIL;
    }

    return BT_GATTC_DISCOVERY_STATUS_SUCCESS;
}

void bt_gattc_discovery_continue(uint16_t conn_handle)
{
    uint8_t i;
    bt_gattc_discovery_context_t *cntx = NULL;

    LOG_MSGID_I(BT_GATTC, "disc_continue, conn_handle = 0x%04x", 1, conn_handle);
    if (conn_handle == BT_HANDLE_INVALID) {
        for (i = 0; i < BT_LE_CONNECTION_NUM; i++) {
            cntx = &g_discovery_context[i];
            if (cntx->conn_discovery_state < BT_GATTC_CONN_DISCOVERY_STATE_READY) {
                continue;
            }
            if (!bt_gattc_is_flow_control_ongoing(cntx->conn_handle)) {
                bt_gattc_discovery_primary_service_completed(cntx);
                return;
            }
        }
    } else {
        cntx = bt_gattc_discovery_get_cntx_by_handle(conn_handle);
        if (NULL == cntx) {
            return;
        }
        bt_gattc_discovery_primary_service_completed(cntx);
    }

    return;
}