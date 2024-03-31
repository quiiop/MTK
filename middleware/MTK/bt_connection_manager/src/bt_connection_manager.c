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

#include "project_config.h"

#include "bt_os_layer_api.h"
#include "bt_gap_le.h"
#include "bt_di.h"
#include "bt_device_manager_config.h"
#include "bt_device_manager.h"
#include "bt_device_manager_db.h"

#include "bt_connection_manager_internal.h"
#include "bt_connection_manager_utils.h"

#include "bt_callback_manager.h"


#define BT_CM_CHECK_RET_WITH_VALUE_AND_LOG(CHECK_CONDITION, RET_VALUE, LOG_STRING, ...) \
    if (CHECK_CONDITION) {  \
        bt_cmgr_report_id(LOG_STRING, ##__VA_ARGS__); \
        return (RET_VALUE); \
    }

#define BT_CM_CHECK_RET_WITH_VALUE_NO_LOG(CHECK_CONDITION, RET_VALUE)   \
    if (CHECK_CONDITION) { \
        return (RET_VALUE); \
    }

#define BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(CHECK_CONDITION, LOG_STRING, ...) \
    if (CHECK_CONDITION) {  \
        bt_cmgr_report_id(LOG_STRING, ##__VA_ARGS__); \
        return; \
    }

#define BT_CM_CHECK_RET_NO_VALUE_NO_LOG(CHECK_CONDITION)    \
    if (CHECK_CONDITION) {  \
        return; \
    }

#define BT_CM_LOG_CONFIG_NULL   "[BT_CM][E] CM config is null"
#define BT_CM_LOG_CONTEXT_NULL  "[BT_CM][E] CM context is null"

#if _MSC_VER >= 1500
#pragma comment(linker, "/alternatename:_bt_cm_event_callback=_default_bt_connection_manager_event_callback")
#elif defined(__GNUC__) || defined(__ICCARM__) || defined(__CC_ARM)
#pragma weak bt_cm_event_callback = default_bt_cm_event_callback
#else /* #if _MSC_VER >= 1500 */
#error "Unsupported Platform"
#endif /* #if _MSC_VER >= 1500 */

/*-----------------
 * Static variables
 *------------------*/
/*
static const uint8_t g_bt_cm_search_di_pattern[] = {
    BT_SDP_ATTRIBUTE_HEADER_8BIT(3),                            // Data Element Sequence, 9 bytes
    BT_SDP_UUID_16BIT(BT_DI_SDP_SERVICE_CLASS_PNP_INFORMATION), // The device Identification UUID in big-endian.
    0x02, 0x00
};
*/
static const uint8_t g_bt_cm_search_di_attributes[] = {
    0x00, 0x64,                                                 /* 0x0064, max handle for attribute return */
    BT_SDP_ATTRIBUTE_HEADER_8BIT(6),
    BT_SDP_UINT_16BIT(BT_DI_SDP_ATTRIBUTE_VENDOR_ID),
    BT_SDP_UINT_16BIT(BT_DI_SDP_ATTRIBUTE_PRODUCT_ID)
};

/* Start Classic bt power related. <<<<<<<<<<<<<<< */
static struct {
    bt_cm_power_state_t cur_state;
    bt_cm_power_state_t target_state;
} g_bt_cm_edr_power_cnt = {
    .cur_state = BT_CM_POWER_STATE_OFF,
    .target_state = BT_CM_POWER_STATE_OFF,
};


/*-----------------
 * Global variables
 *------------------*/

bt_cm_cnt_t     *g_bt_cm_cnt = NULL;
bt_cm_config_t  *g_bt_cm_cfg = NULL;
bt_cm_remote_info_update_ind_t g_remote_info;

/*-----------------
 * Macros
 *------------------*/

#define BT_CM_ADD_MASK(MASK, POSITION)      ((MASK) |= (0x01U << (POSITION)))
#define BT_CM_REMOVE_MASK(MASK, POSITION)   ((MASK) &= ~(0x01U << (POSITION)))


/*-----------------
 * Functions
 *------------------*/

static void default_bt_cm_event_callback(bt_cm_event_t event_id, void *params, uint32_t params_len)
{

}
/*
static void BT_CM_LOG_CONNECTION_STATUS(bt_cm_remote_device_t *device_p)
{
    BT_CM_CHECK_RET_NO_VALUE_NO_LOG(NULL == device_p);
    bt_cmgr_report_id("[BT_CM][I] Device:0x%x, request_conn:0x%04x, request_disconn:0x%04x, "
        "connecting:0x%04x, disconnecting:0x%04x, connected:0x%04x, flags:0x%x, link_state:0x%x", 8,
        *(uint32_t *)(&device_p->link_info.addr), device_p->request_connect_mask, device_p->request_disconnect_mask,
        device_p->link_info.connecting_mask, device_p->link_info.disconnecting_mask, device_p->link_info.connected_mask,
        device_p->flags, device_p->link_info.link_state);
}
*/
static bt_sink_srv_profile_type_t bt_cm_profile_mask_to_sink_mask(bt_cm_profile_service_mask_t cm_mask)
{
    bt_sink_srv_profile_type_t ret_sink_mask = 0;

    if (cm_mask & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_HFP))
        ret_sink_mask |= BT_SINK_SRV_PROFILE_HFP;
    if (cm_mask & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_HSP))
        ret_sink_mask |= BT_SINK_SRV_PROFILE_HSP;
    if (cm_mask & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_PBAPC))
        ret_sink_mask |= BT_SINK_SRV_PROFILE_HSP;
    if (cm_mask & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_A2DP_SINK))
        ret_sink_mask |= BT_SINK_SRV_PROFILE_A2DP_SINK;
    if (cm_mask & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_AVRCP))
        ret_sink_mask |= BT_SINK_SRV_PROFILE_AVRCP;

    return ret_sink_mask;
}

static void bt_cm_list_add(bt_cm_list_t list_type, bt_cm_remote_device_t *device, bt_cm_list_add_t add_type)
{
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == g_bt_cm_cnt, BT_CM_LOG_CONTEXT_NULL, 0);
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == device, "[BT_CM][E] list remove device is NULL", 0);
    if (list_type < 0 || list_type >= BT_CM_LIST_TYPE_MAX)
        return;

    if (BT_CM_LIST_ADD_FRONT == add_type) {
        bt_cm_remote_device_t *temp = device;
        while (NULL != temp->next[list_type]) {
            temp = temp->next[list_type];
        }
        temp->next[list_type] = g_bt_cm_cnt->handle_list[list_type];
        g_bt_cm_cnt->handle_list[list_type] = device;
    } else if (BT_CM_LIST_ADD_BACK == add_type) {
        bt_cm_remote_device_t *temp = (bt_cm_remote_device_t *) & (g_bt_cm_cnt->handle_list);
        while (NULL != temp->next[list_type]) {
            temp = temp->next[list_type];
        }
        temp->next[list_type] = device;
    }
}

static void bt_cm_list_remove(bt_cm_list_t list_type, bt_cm_remote_device_t *device)
{
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == g_bt_cm_cnt, BT_CM_LOG_CONTEXT_NULL, 0);
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == device, "[BT_CM][E] list remove device is NULL", 0);
    if (list_type < 0 || list_type >= BT_CM_LIST_TYPE_MAX)
        return;

    bt_cm_remote_device_t *temp = (bt_cm_remote_device_t *) & (g_bt_cm_cnt->handle_list);
    while (NULL != temp->next[list_type]) {
        if (temp->next[list_type] == device) {
            temp->next[list_type] = ((bt_cm_remote_device_t *)(temp->next[list_type]))->next[list_type];
            device->next[list_type] = NULL;
            return;
        }
        temp = temp->next[list_type];
    }
}

bt_cm_remote_device_t *bt_cm_list_get_last(bt_cm_list_t list_type)
{
    BT_CM_CHECK_RET_WITH_VALUE_AND_LOG(NULL == g_bt_cm_cnt, NULL, BT_CM_LOG_CONTEXT_NULL, 0);
    if (list_type < 0 || list_type >= BT_CM_LIST_TYPE_MAX)
        return NULL;

    bt_cm_remote_device_t *temp = g_bt_cm_cnt->handle_list[list_type];
    while (NULL != temp && NULL != temp->next[list_type]) {
        temp = temp->next[list_type];
    }
    return temp;
}

#define PROFILE_PARSER_BUF_SIZE 32
static void bt_cm_profile_append_text_cpy(char *dest, char *src, int *index)
{
    int size = PROFILE_PARSER_BUF_SIZE - *index;
    int len = (int)strlen(src);

    len = len > size ? size : len;
    strncat(dest + *index, src, len);
    *index += len;
}

static void bt_cm_profile_append_text(char *dest, char *src, int *index)
{
    if (*index)
        bt_cm_profile_append_text_cpy(dest, " | ", index);

    bt_cm_profile_append_text_cpy(dest, src, index);
}

char *bt_cm_profile_parser(bt_cm_profile_service_t profile)
{
    int index = 0;
    static char ret[PROFILE_PARSER_BUF_SIZE] = {0};

    memset(ret, 0, PROFILE_PARSER_BUF_SIZE);

    if (profile & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_A2DP_SINK)) {
        bt_cm_profile_append_text(ret, "A2DP Sink", &index);
    }
    if (profile & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_AVRCP)) {
        bt_cm_profile_append_text(ret, "AVRCP", &index);
    }

    if (index == 0)
        memcpy(ret, "NULL", 4);
    return ret;
}

static void bt_cm_show_devices(void)
{
    uint8_t *addr_p;
    bt_cm_link_info_t *link_info_p;

    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == g_bt_cm_cnt, BT_CM_LOG_CONTEXT_NULL, 0);
    bt_cmgr_report_id("[BT_CM][I] g_bt_cm_cnt->max_conn_num = %d, dev_buf_num = %d", 2,
                      g_bt_cm_cnt->max_connection_num, g_bt_cm_cnt->devices_buffer_num);
    bt_cmgr_report_id("[BT_CM][I] g_bt_cm_cnt->connected_dev_num = %d", 1, g_bt_cm_cnt->connected_dev_num);
    bt_cmgr_report_id("[BT_CM][I] g_bt_cm_cnt->scan moe = %d", 1,
                      g_bt_cm_cnt->scan_mode);

    for (uint32_t i = 0; i < g_bt_cm_cfg->max_connection_num; ++i) {
        link_info_p = &(g_bt_cm_cnt->devices_list[i].link_info);
        addr_p = (uint8_t *)link_info_p->addr;
        bt_cmgr_report_id("[BT_CM][I] dev_list[%d] hdl = 0x%x (device_p = %p)", 3,
                          i, link_info_p->handle, &g_bt_cm_cnt->devices_list[i]);
        bt_cmgr_report_id("[BT_CM][I] dev_list[%d] addr = 0x%02x-%02x-%02x-%02x-%02x-%02x", 7, i,
                          addr_p[5], addr_p[4], addr_p[3], addr_p[2], addr_p[1], addr_p[0]);
        bt_cmgr_report_id("[BT_CM][I] dev_list[%d] Profile:%s  Role:%s", 3, i,
                          bt_cm_profile_parser(link_info_p->connected_mask),
                          link_info_p->local_role ? "Slave" : "Master");
    }
}

void bt_cm_protected_show_devices(void)
{
    bt_cm_mutex_lock();
    bt_cm_show_devices();
    bt_cm_mutex_unlock();
}

bt_cm_remote_device_t *bt_cm_find_device(bt_cm_find_t find_type, void *param)
{
    bt_cm_remote_device_t *device_p = NULL;
    BT_CM_CHECK_RET_WITH_VALUE_AND_LOG(NULL == param, NULL, "[BT_CM][E] Find device param is null", 0);
    BT_CM_CHECK_RET_WITH_VALUE_AND_LOG(NULL == g_bt_cm_cnt, NULL, BT_CM_LOG_CONTEXT_NULL, 0);

    for (uint32_t i = 0; i < g_bt_cm_cfg->max_connection_num; ++i) {
        if (BT_CM_FIND_BY_HANDLE == find_type &&
            g_bt_cm_cnt->devices_list[i].link_info.handle == *((bt_gap_connection_handle_t *)param)) {
            device_p = &g_bt_cm_cnt->devices_list[i];
            break;
        } else if (BT_CM_FIND_BY_ADDR == find_type &&
                   bt_cm_memcmp(param, &g_bt_cm_cnt->devices_list[i].link_info.addr, sizeof(bt_bd_addr_t)) == 0) {
            device_p = &g_bt_cm_cnt->devices_list[i];
            break;
        } else if (BT_CM_FIND_BY_REMOTE_FLAG == find_type &&
                   (g_bt_cm_cnt->devices_list[i].flags & ((uint32_t)param))) {
            device_p = &g_bt_cm_cnt->devices_list[i];
            break;
        } else if (BT_CM_FIND_BY_LINK_STATE == find_type &&
                   (g_bt_cm_cnt->devices_list[i].link_info.link_state == (uint32_t)param)) {
            device_p = &g_bt_cm_cnt->devices_list[i];
            break;
        } else if (g_bt_cm_cfg->max_connection_num == (i + 1)) {
            bt_cmgr_report_id("[BT_CM][I] Can't find device by type %d", 1, find_type);
        }
    }
    if (NULL != device_p) {
        bt_cmgr_report_id("[BT_CM][I] Find handle:0x%x (%p)", 2, device_p->link_info.handle, device_p);
    }
    return device_p;
}

static bt_cm_remote_device_t *bt_cm_find_new_device(const bt_bd_addr_t *addr)
{
    bt_cm_remote_device_t *device_p;
    bt_bd_addr_t temp_addr = {0};

    bt_cmgr_report_id("[BT_CM][I] find new device handle", 0);
    device_p = bt_cm_find_device(BT_CM_FIND_BY_ADDR, (void *)temp_addr);
    if (NULL == device_p) {
        bt_cmgr_report_id("[BT_CM][E] Connection already maximum", 0);
        return NULL;
    }
    bt_cm_memset(device_p, 0, sizeof(*device_p));
    bt_cm_memcpy(&device_p->link_info.addr, addr, sizeof(bt_bd_addr_t));
    device_p->link_info.link_state = BT_CM_ACL_LINK_DISCONNECTED;

    uint8_t *addr_p = (uint8_t *) addr;
    bt_cmgr_report_id("[BT_CM][I] add new dev(%p) addr = 0x%02x-%02x-%02x-%02x-%02x-%02x", 7, device_p,
                      addr_p[5], addr_p[4], addr_p[3], addr_p[2], addr_p[1], addr_p[0]);

    return device_p;
}

static bt_status_t bt_cm_clear_device(bt_cm_remote_device_t *device_p)
{
    BT_CM_CHECK_RET_WITH_VALUE_AND_LOG(NULL == device_p, BT_STATUS_FAIL, "[BT_CM][E] wrong param!!", 0);

    bt_cmgr_report_id("[BT_CM][I] clear device, hdl = 0x%x", 1, device_p->link_info.handle);

    bt_cm_list_remove(BT_CM_LIST_CONNECTING, device_p);
    bt_cm_list_remove(BT_CM_LIST_CONNECTED, device_p);
    bt_cm_memset(device_p, 0, sizeof(*device_p));

    return BT_STATUS_SUCCESS;
}

static void bt_cm_clear_all_devices(void)
{
    if (g_bt_cm_cnt == NULL)
        return;
    if (g_bt_cm_cnt->devices_buffer_num == 0) {
        bt_cmgr_report_id("[BT_CM][E] why devices_buffer_num == 0?", 0);
        return;
    }
    bt_cmgr_report_id("[BT_CM][I] clear all devices", 0);

    g_bt_cm_cnt->connected_dev_num = 0;
    g_bt_cm_cnt->scan_mode = 0;
    g_bt_cm_cnt->handle_list[BT_CM_LIST_CONNECTING] = NULL;
    g_bt_cm_cnt->handle_list[BT_CM_LIST_CONNECTED] = NULL;
    bt_cm_memset(&(g_bt_cm_cnt->devices_list), 0, sizeof(bt_cm_remote_device_t) * g_bt_cm_cnt->devices_buffer_num);
}

static void bt_cm_handle_link_update_ind(bt_status_t status, bt_gap_link_status_updated_ind_t *param)
{
    //MTK_Titan: (TODO) This part has huge different in new CM module
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == param, "[BT_CM][E] Param is null", 0);
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == g_bt_cm_cnt, BT_CM_LOG_CONTEXT_NULL, 0);

    bt_cm_remote_device_t *device_p = bt_cm_find_device(BT_CM_FIND_BY_ADDR, (bt_bd_addr_t *)(param->address));
    bt_cmgr_report_id("[BT_CM][I] Link update device:0x%x, link status:0x%x, status:0x%x, handle = 0x%x", 4,
                      device_p, param->link_status, status, param->handle);

    //bt_cmgr_report_id("[BT_CM][I] Link Status:0x%x", 1, param->link_status);
    bt_cm_acl_link_state_t cm_acl_state = BT_CM_ACL_LINK_DISCONNECTED;

    if (BT_GAP_LINK_STATUS_CONNECTED_0 <= param->link_status && BT_STATUS_SUCCESS == status) {

        if (NULL == device_p) {
            device_p = bt_cm_find_new_device(param->address);
            if (NULL == device_p) {
                bt_cmgr_report_id("[BT_CM][E] Fail to find free space", 0);
                //(TODO) do the error handling here if no space.
                return;
            }
        }

        bt_cm_acl_link_state_t pre_acl_state = device_p->link_info.link_state;
        device_p->link_info.handle = param->handle;

        if (BT_GAP_LINK_STATUS_CONNECTED_0 < param->link_status)
            cm_acl_state = BT_CM_ACL_LINK_ENCRYPTED;
        else
            cm_acl_state = BT_CM_ACL_LINK_CONNECTED;

        if (pre_acl_state < BT_CM_ACL_LINK_CONNECTED) {
            //this is 1st time connection or reconnection.

            if (g_bt_cm_cnt->connected_dev_num < g_bt_cm_cnt->max_connection_num)
                g_bt_cm_cnt->connected_dev_num ++;
            else
                bt_cmgr_report_id("[BT_CM][E] Why the connected_number greater than max_connection_num?", 0);
            bt_cm_list_remove(BT_CM_LIST_CONNECTING, device_p);
            bt_cm_list_add(BT_CM_LIST_CONNECTED, device_p, BT_CM_LIST_ADD_BACK);
            // Disable discoverable after connected.
#ifdef MTK_BT_DUO_ENABLE
            bt_cm_scan_mode(false, false);
#endif
        }
        device_p->link_info.link_state = cm_acl_state;
        //bt_cm_show_devices();

        if (cm_acl_state == BT_CM_ACL_LINK_ENCRYPTED) {
            /*
             * Update seq number for last connected device.
             * If it is a paired device, write info into nvdm.
             * If it's not paired, write info into nvdm later until it's bonded.
             */
            bt_device_manager_remote_top(*(bt_bd_addr_t *)(param->address));
            if (bt_device_manager_is_paired(*(param->address)))
                bt_device_manager_db_flush_all(BT_DEVICE_MANAGER_DB_FLUSH_BLOCK);
            bt_cm_show_devices();
        }
    } else if (BT_GAP_LINK_STATUS_DISCONNECTED == param->link_status) {
        /* Currently we will not do anything while disconnected. May re-enable discoverable. */
        bt_cm_clear_device(device_p);

        /* Should avoid under flow */
        if (g_bt_cm_cnt->connected_dev_num > 0)
            g_bt_cm_cnt->connected_dev_num --;
        else
            bt_cmgr_report_id("[BT_CM][E] Why get disconnect when connected_number equal to 0?", 0);
        bt_cm_show_devices();
    } else if (BT_GAP_LINK_STATUS_CONNECTION_FAILED == param->link_status) {
        bt_cm_clear_device(device_p);
        bt_cm_show_devices();
    }

    g_remote_info.pre_acl_state = g_remote_info.acl_state;
    g_remote_info.acl_state = cm_acl_state;
    g_remote_info.reason = status;
    memcpy(g_remote_info.address, param->address, sizeof(bt_bd_addr_t));
    bt_cm_event_callback(BT_CM_EVENT_REMOTE_INFO_UPDATE, &g_remote_info, sizeof(bt_cm_remote_info_update_ind_t));
}

#ifdef MTK_BT_DUO_ENABLE
static void bt_cm_sdp_search_service_cnf_handle(bt_status_t status, void *buffer)
{
    bt_sdpc_service_cnf_t *service_result = (bt_sdpc_service_cnf_t *)buffer;
    BT_CM_CHECK_RET_NO_VALUE_NO_LOG(NULL == service_result);
    bt_cm_remote_device_t *device_p = bt_cm_find_device(BT_CM_FIND_BY_ADDR, (void *)(service_result->user_data));
    BT_CM_CHECK_RET_NO_VALUE_NO_LOG(NULL == device_p);
    if (0 == service_result->handle_number) {
        bt_device_manager_db_remote_pnp_info_t pnp_info = {
            .vender_id = 0,
            .product_id = 0
        };
        bt_device_manager_remote_update_pnp_info(device_p->link_info.addr, &pnp_info);
        bt_cmgr_report_id("[BT_CM][I] Pnp info serach no result", 0);
    } else {
        uint32_t query_service_handle = bt_sdp_get_32bit(service_result->handle_list);
        bt_sdpc_attribute_request_t request;
        request.address = (void *) & (device_p->link_info.addr);
        request.search_handle = query_service_handle;
        request.attribute_pattern = g_bt_cm_search_di_attributes;
        request.pattern_length = sizeof(g_bt_cm_search_di_attributes);
        request.user_data = &(device_p->link_info.addr);
        bt_status_t return_result = bt_sdpc_search_attribute(&request);
        bt_cmgr_report_id("[BT_CM][I] Pnp info serach attribute result: %d", 1, return_result);
    }
}

static void bt_cm_sdp_search_attribute_cnf_handle(bt_status_t status, void *buffer)
{
    bt_sdpc_attribute_cnf_t *attr_result = (bt_sdpc_attribute_cnf_t *)buffer;
    uint8_t *parse_result = NULL, *data = NULL;
    uint16_t result_len = 0, data_len = 0;

    bt_device_manager_db_remote_pnp_info_t pnp_info = {
        .vender_id = 0,
        .product_id = 0
    };
    bt_sdpc_parse_attribute(&parse_result, &result_len, BT_DI_SDP_ATTRIBUTE_VENDOR_ID, 0, attr_result->length, attr_result->attribute_data);
    bt_sdpc_parse_next_value(&data, &data_len, parse_result, result_len);
    if (data_len) {
        pnp_info.vender_id = (data[0] << 8 | data[1]);
        bt_cmgr_report_id("[BT_CM][I] vender_id:0x%04x", 1, pnp_info.vender_id);
    }
    bt_sdpc_parse_attribute(&parse_result, &result_len, BT_DI_SDP_ATTRIBUTE_PRODUCT_ID, 0, attr_result->length, attr_result->attribute_data);
    bt_sdpc_parse_next_value(&data, &data_len, parse_result, result_len);
    if (data_len) {
        pnp_info.product_id = (data[0] << 8 | data[1]);
        bt_cmgr_report_id("[BT_CM][I] product_id:0x%04x", 1, pnp_info.product_id);
    }
    bt_device_manager_remote_update_pnp_info((void *)(attr_result->user_data), &pnp_info);
}

static void bt_cm_gap_role_event_handler(bt_msg_type_t msg, bt_status_t status, void *buffer)
{
    bt_cm_remote_device_t *device_p = NULL;
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == g_bt_cm_cfg, BT_CM_LOG_CONFIG_NULL, 0);
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == g_bt_cm_cnt, BT_CM_LOG_CONTEXT_NULL, 0);

    if (BT_GAP_GET_ROLE_CNF == msg) {
        bt_gap_get_role_cnf_t *get_role = (bt_gap_get_role_cnf_t *)buffer;
        BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == (device_p = bt_cm_find_device(BT_CM_FIND_BY_HANDLE, &(get_role->handle))),
                                          "[BT_CM][E] Get role cnf, can't find device", 0);
        bt_cmgr_report_id("[BT_CM][I] Get role cnf: %s", 1, get_role->local_role ? "Slave" : "Master");
        device_p->link_info.local_role = get_role->local_role;
        return;
    } else if (BT_GAP_SET_ROLE_CNF == msg) {
        device_p = bt_cm_find_device(BT_CM_FIND_BY_REMOTE_FLAG, (void *)BT_CM_REMOTE_FLAG_ROLE_SWITCHING);
        if (NULL != device_p)
            device_p->flags &= (~BT_CM_REMOTE_FLAG_ROLE_SWITCHING);
        BT_CM_CHECK_RET_NO_VALUE_NO_LOG(BT_STATUS_SUCCESS == status);
        bt_cmgr_report_id("[BT_CM][W] Set role fail.", 0);
    } else if (BT_GAP_ROLE_CHANGED_IND == msg) {
        bt_gap_role_changed_ind_t *role_change = (bt_gap_role_changed_ind_t *)buffer;
        BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == role_change || NULL == (device_p = bt_cm_find_device(BT_CM_FIND_BY_HANDLE, &(role_change->handle))),
                                          "[BT_CM][E] Can't find device by handle 0x%x", 1, role_change->handle);
        bt_cmgr_report_id("[BT_CM][I] Changed:0x%x,local:0x%x", 2, role_change->local_role, device_p->link_info.local_role);
        device_p->link_info.local_role = role_change->local_role;
        BT_CM_CHECK_RET_NO_VALUE_NO_LOG(BT_STATUS_SUCCESS == status);
    }
}

void bt_cm_profile_service_register(bt_cm_profile_service_t profile, bt_cm_profile_service_handle_callback_t cb)
{
    if (profile < 0 || profile >= BT_CM_PROFILE_SERVICE_MAX) {
        bt_cmgr_report_id("[BT_CM][E] Wrong param %d", 1, profile);
        return;
    }

    if (!cb) {
        bt_cmgr_report_id("[BT_CM][E]cb is NULL", 0);
        return;
    }

    bt_cmgr_report_id("[BT_CM][I] Register profile service callback, profile:%d", 1, profile);
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == g_bt_cm_cnt, BT_CM_LOG_CONTEXT_NULL, 0);
    bt_cm_mutex_lock();
    g_bt_cm_cnt->profile_service_cb[profile] = cb;
    bt_cm_mutex_unlock();
}
#endif /* #ifdef MTK_BT_DUO_ENABLE */

static void bt_cm_notify_profile_service_callback(bt_cm_profile_service_handle_t type, void *data)
{
    //MTK_Titan: This is an alternated method. No this API in new CM.
    uint8_t i;
    for (i = 0; i < BT_CM_PROFILE_SERVICE_MAX; i++) {
        if (g_bt_cm_cnt->profile_service_cb[i])
            g_bt_cm_cnt->profile_service_cb[i](type, data);
    }
}

static bt_status_t bt_cm_system_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
{
    /* MTK_David:New CM did not have this function. */
#if defined(__BT_SINK_SRV_DEBUG_INFO__ )
    if (msg >= BT_POWER_ON_CNF && msg <= BT_PANIC) {
        bt_cmgr_report("[BT_CM][I] SDK msg:%s", g_system_event_string[msg - BT_POWER_ON_CNF]);
    } else {
        bt_cmgr_report_id("[BT_CM][I] SDK msg:0x%x", 1, msg);
    }
#else /* #if defined(__BT_SINK_SRV_DEBUG_INFO__ ) */
    bt_cmgr_report_id("[BT_CM][I] SDK msg:0x%x", 1, msg);
#endif /* #if defined(__BT_SINK_SRV_DEBUG_INFO__ ) */

    switch (msg) {
        case BT_POWER_ON_CNF: {
                bt_cm_power_state_update_ind_t ind;
                ind.power_state = BT_CM_POWER_STATE_ON;
                //bt_connection_manager_power_event_handle(msg, status, buffer);
                //bt_connection_manager_handle_power_on_cnf();
                g_bt_cm_edr_power_cnt.cur_state = BT_CM_POWER_STATE_ON;
                bt_cm_notify_profile_service_callback(BT_CM_PROFILE_SERVICE_HANDLE_POWER_ON, NULL);
                bt_cm_event_callback(BT_CM_EVENT_POWER_STATE_UPDATE, &ind, sizeof(ind));
            }
            break;
        case BT_POWER_OFF_CNF: {
                bt_cm_power_state_update_ind_t ind;
                ind.power_state = BT_CM_POWER_STATE_OFF;
                //bt_connection_manager_handle_power_off_cnf();
                //bt_device_manager_db_flush_all(BT_DEVICE_MANAGER_DB_FLUSH_BLOCK);
                //bt_connection_manager_power_event_handle(msg, status, buffer);
                g_bt_cm_edr_power_cnt.cur_state = BT_CM_POWER_STATE_OFF;
                bt_cm_notify_profile_service_callback(BT_CM_PROFILE_SERVICE_HANDLE_POWER_OFF, NULL);
                bt_cm_event_callback(BT_CM_EVENT_POWER_STATE_UPDATE, &ind, sizeof(ind));
                bt_cm_clear_all_devices();
            }
            break;
        case BT_PANIC:
            //bt_sink_srv_cm_handle_bt_panic();
            break;
        case BT_DUT_MODE_ACTIVE_IND:
            //bt_cmgr_report_id("[BT_CM][I] BT enter DUT mode", 0);
            //bt_connection_manager_power_event_handle(msg, status, buffer);
            break;
        default:
            break;
    }
    return BT_STATUS_SUCCESS;
}

#ifdef MTK_BT_DUO_ENABLE
static bt_status_t bt_cm_sdp_event_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
{
    bt_status_t result = BT_STATUS_SUCCESS;
    bt_cmgr_report_id("[BT_CM][I] bt_connection_manager_sdp_callback, msg:0x%x", 1, msg);
    switch (msg) {
        case BT_SDPC_SEARCH_SERVICE_CNF: {
                bt_cm_sdp_search_service_cnf_handle(status, buffer);
                break;
            }
        case BT_SDPC_SEARCH_ATTRIBUTE_CNF: {
                bt_cm_sdp_search_attribute_cnf_handle(status, buffer);
                break;
            }
        case BT_SDPC_SEARCH_ERROR: {
                bt_cmgr_report_id("[BT_CM][I] BT_SDPC_SEARCH_ERROR", 0);
                break;
            }
        default:
            bt_cmgr_report_id("[BT_CM][I] Unexcepted msg:%x", 1, msg);
            break;
    }
    return result;
}

static bt_status_t bt_cm_gap_event_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
{
    bt_status_t result = BT_STATUS_SUCCESS;
    //bt_cmgr_report_id("[BT_CM][I] SDK gap msg:0x%x, status:0x%x", 2, msg, status);
    BT_CM_CHECK_RET_WITH_VALUE_AND_LOG(NULL == g_bt_cm_cfg, result, BT_CM_LOG_CONFIG_NULL, 0);
    BT_CM_CHECK_RET_WITH_VALUE_AND_LOG(NULL == g_bt_cm_cnt, result, BT_CM_LOG_CONTEXT_NULL, 0);

    switch (msg) {
        case BT_GAP_LINK_STATUS_UPDATED_IND:
            bt_cm_handle_link_update_ind(status, buffer);
            break;
        case BT_GAP_SET_SCAN_MODE_CNF: {
                bt_cm_visibility_state_update_ind_t ind;
                if (g_bt_cm_cnt->scan_mode & BT_GAP_SCAN_MODE_DISCOVERABLE_ONLY)
                    ind.visibility_state = true;
                else
                    ind.visibility_state = false;

                if (g_bt_cm_cnt->scan_mode & BT_GAP_SCAN_MODE_CONNECTABLE_ONLY)
                    ind.connectivity_state = true;
                else
                    ind.connectivity_state = false;

                bt_cm_event_callback(BT_CM_EVENT_VISIBILITY_STATE_UPDATE, &ind, sizeof(ind));
                break;
            }
        case BT_GAP_GET_ROLE_CNF:
        case BT_GAP_SET_ROLE_CNF:
        case BT_GAP_ROLE_CHANGED_IND: {
                bt_cm_gap_role_event_handler(msg, status, buffer);
                break;
            }
        default:
            //bt_cmgr_report_id("[BT_CM][I] Unexcepted msg:%x", 1, msg);
            break;
    }
    return result;
}
#endif /* #ifdef MTK_BT_DUO_ENABLE */

static bt_status_t bt_cm_common_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
{
    bt_status_t result = BT_STATUS_SUCCESS;
    uint32_t module = msg & 0xFF000000;

    /*MTK_Titan: Add BT_MODULE_SYSTEM for power control part to instead of linking to DM.*/
    switch (module) {
        case BT_MODULE_SYSTEM:
            result = bt_cm_system_callback(msg, status, buffer);
            break;
#ifdef MTK_BT_DUO_ENABLE
        case BT_MODULE_GAP:
            result = bt_cm_gap_event_callback(msg, status, buffer);
            break;
        case BT_MODULE_SDP:
            result = bt_cm_sdp_event_callback(msg, status, buffer);
            break;
#endif
        default:
            bt_cmgr_report_id("[BT_CM][E] Not expected bt event module 0x%x", 1, module);
            break;
    }
    return result;
}

bt_cm_power_state_t bt_cm_power_get_state(void)
{
    bt_cm_power_state_t ret;

    bt_cm_mutex_lock();
    ret = g_bt_cm_edr_power_cnt.cur_state;
    bt_cm_mutex_unlock();

    return ret;
}

bt_gap_connection_handle_t bt_cm_get_gap_handle(bt_bd_addr_t addr)
{
    bt_gap_connection_handle_t gap_handle = 0;
    bt_cm_remote_device_t *device_p;

    bt_cm_mutex_lock();
    device_p = bt_cm_find_device(BT_CM_FIND_BY_ADDR, (void *)addr);
    if (NULL != device_p) {
        gap_handle =  device_p->link_info.handle;
    } else {
        bt_cmgr_report_id("[BT_CM][E] Can't find device_p", 0);
    }
    bt_cm_mutex_unlock();

    bt_cmgr_report_id("[BT_CM][I] Get gap handle:0x%x", 1, gap_handle);
    return gap_handle;
}

uint32_t bt_cm_get_connected_devices(bt_cm_profile_service_mask_t profiles, bt_bd_addr_t *addr_list, uint32_t list_num)
{
    BT_CM_CHECK_RET_WITH_VALUE_AND_LOG(NULL == g_bt_cm_cnt, 0, BT_CM_LOG_CONTEXT_NULL, 0);
    uint32_t count = 0;
    bt_cm_remote_device_t *device_p;

    bt_cm_mutex_lock();
    device_p = g_bt_cm_cnt->handle_list[BT_CM_LIST_CONNECTED];

    while (NULL != device_p) {
        if (BT_CM_PROFILE_SERVICE_MASK_NONE == profiles || 0 != (profiles & device_p->link_info.connected_mask)) {
            count++;
            if (addr_list != NULL && 0 != list_num) {
                bt_cm_memcpy(&(addr_list[count - 1]), device_p->link_info.addr, sizeof(bt_bd_addr_t));
                if (count == list_num) {
                    break;
                }
            }
        }
        device_p = device_p->next[BT_CM_LIST_CONNECTED];
    }
    bt_cm_mutex_unlock();
    bt_cmgr_report_id("[BT_CM][I] Connected count:%d", 1, count);
    return count;
}

bt_cm_profile_service_mask_t bt_cm_get_connected_profile_services(bt_bd_addr_t addr)
{
    bt_cm_remote_device_t *device_p = bt_cm_find_device(BT_CM_FIND_BY_ADDR, addr);
    BT_CM_CHECK_RET_WITH_VALUE_AND_LOG(NULL == device_p || BT_CM_ACL_LINK_CONNECTED > device_p->link_info.link_state,
                                       BT_CM_PROFILE_SERVICE_MASK_NONE, "[BT_CM][E] Remote device not connected", 0);
    return device_p->link_info.connected_mask;
}

#ifdef MTK_BT_DUO_ENABLE
void bt_cm_scan_mode(bool visible, bool connectable)
{
    //MTK_Titan: New CM use bt_cm_discoverable()
    bt_gap_scan_mode_t mode_to_set = 0;

    if (visible)
        mode_to_set |= BT_GAP_SCAN_MODE_DISCOVERABLE_ONLY; //visible
    if (connectable)
        mode_to_set |= BT_GAP_SCAN_MODE_CONNECTABLE_ONLY; //connectable

    bt_cm_mutex_lock();
    if (mode_to_set != g_bt_cm_cnt->scan_mode) {
        g_bt_cm_cnt->scan_mode = mode_to_set;
        bt_gap_set_scan_mode(g_bt_cm_cnt->scan_mode);
    }
    bt_cm_mutex_unlock();
}
#endif

bt_status_t bt_cm_connect(const bt_cm_connect_t *param)
{
    //MTK_Titan: (TODO) This part has huge different in new CM module

    bt_sink_srv_profile_connection_action_t action;

    memcpy(&action.address, param->address, sizeof(bt_bd_addr_t));
    action.profile_connection_mask = bt_cm_profile_mask_to_sink_mask(param->profile);

    //Check device node
    bt_cm_mutex_lock();
    bt_cm_remote_device_t *device_p = bt_cm_find_device(BT_CM_FIND_BY_ADDR, (bt_bd_addr_t *) & (param->address));
    if (NULL == device_p) {
        device_p = bt_cm_find_new_device(&(param->address));
        if (NULL == device_p) {
            bt_cm_mutex_unlock();
            bt_cmgr_report_id("[BT_CM][E] Fail to find free space", 0);
            return BT_CM_STATUS_MAX_LINK;
        }
    }

    /* Add new link request to connecting list. */
    if (device_p->link_info.link_state == BT_CM_ACL_LINK_DISCONNECTED) {
        device_p->link_info.link_state = BT_CM_ACL_LINK_PENDING_CONNECT;
        bt_cm_list_add(BT_CM_LIST_CONNECTING, device_p, BT_CM_LIST_ADD_BACK);
    } else {
        bt_cmgr_report_id("[BT_CM][W] Already in linking state (%d) ", 1, device_p->link_info.link_state);
        bt_cm_mutex_unlock();
        return BT_CM_STATUS_LINK_EXIST;
    }
    /*  Record the expected connect mask. */
    device_p->expected_connect_mask |= param->profile;

    bt_cm_mutex_unlock();

    bt_sink_srv_send_action(BT_SINK_SRV_ACTION_PROFILE_CONNECT, &action);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_cm_disconnect(const bt_cm_connect_t *param)
{
    //MTK_Titan: (TODO) This part has huge different in new CM module
    //no handle profile mask now
    bt_sink_srv_profile_connection_action_t action;

    memcpy(&action.address, param->address, sizeof(bt_bd_addr_t));
    action.profile_connection_mask = bt_cm_profile_mask_to_sink_mask(param->profile);

    bt_sink_srv_send_action(BT_SINK_SRV_ACTION_PROFILE_DISCONNECT, &action);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_cm_force_disconnect(bt_bd_addr_t *remote_dev)
{
#ifdef MTK_BT_DUO_ENABLE
    bt_cm_remote_device_t *device_p = NULL;
    bt_cmgr_report_id("[BT_CM][I] To force disconnect the remote device", 0);
    bt_cm_mutex_lock();
    if (NULL != remote_dev && NULL == (device_p = bt_cm_find_device(BT_CM_FIND_BY_ADDR, (void *)remote_dev))) {
        bt_cmgr_report_id("[BT_CM][E] Can't find the remote device", 0);
        bt_cm_mutex_unlock();
        return BT_STATUS_FAIL;
    }
    if (NULL != device_p) {
        if (BT_CM_ACL_LINK_DISCONNECTED != device_p->link_info.link_state) {
            bt_cm_mutex_unlock();
            return bt_gap_disconnect_ext(device_p->link_info.handle, 1000);
        }
        bt_cm_mutex_unlock();
        return BT_STATUS_FAIL;
    } else {
        for (uint32_t i = 0; i < g_bt_cm_cfg->max_connection_num ; ++i) {
            device_p = &(g_bt_cm_cnt->devices_list[i]);
            if (BT_CM_ACL_LINK_DISCONNECTED != device_p->link_info.link_state) {
                if (BT_STATUS_SUCCESS == bt_gap_disconnect_ext(device_p->link_info.handle, 1000)) {
                    bt_cm_mutex_unlock();
                    return BT_STATUS_SUCCESS;
                }
            }
        }
    }
    bt_cm_mutex_unlock();
    return BT_STATUS_FAIL;
#else
    /* CM did not handle LE link, so we return it directly. */
    return BT_STATUS_SUCCESS;
#endif
}

void bt_cm_init(const bt_cm_config_t *config)
{
    bt_cmgr_report_id("[BT_CM][I] Init", 0);
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == config, "[BT_CM][E] Config param is null", 0);
    g_bt_cm_cfg = (bt_cm_config_t *)config;
    uint32_t support_device;

    bt_cm_mutex_lock();

    support_device = g_bt_cm_cfg->max_connection_num ? g_bt_cm_cfg->max_connection_num : 1;
    if (NULL != g_bt_cm_cnt && g_bt_cm_cnt->devices_buffer_num < g_bt_cm_cfg->max_connection_num) {
        bt_cm_memory_free(g_bt_cm_cnt);
        g_bt_cm_cnt = NULL;
    }
    if (NULL == g_bt_cm_cnt) {
        uint32_t temp_size = sizeof(bt_cm_cnt_t) + ((support_device - 1) * sizeof(bt_cm_remote_device_t));

        g_bt_cm_cnt = bt_cm_memory_alloc(temp_size);
        BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == g_bt_cm_cnt, "[BT_CM][E] Context memory allocate fail", 0);
        bt_cm_memset(g_bt_cm_cnt, 0, temp_size);
        g_bt_cm_cnt->devices_buffer_num = support_device;
    }
    g_bt_cm_cnt->max_connection_num = g_bt_cm_cfg->max_connection_num;

    bt_cm_mutex_unlock();

    bt_cm_timer_init();
    //bt_device_manager_dev_register_callback(BT_DEVICE_TYPE_CLASSIC, (bt_device_manager_power_callback_t)bt_cm_power_device_manager_callback);
    bt_callback_manager_register_callback(bt_callback_type_app_event,
                                          (uint32_t)(
#ifdef MTK_BT_DUO_ENABLE
                                            MODULE_MASK_SDP |
#endif
                                            MODULE_MASK_GAP | MODULE_MASK_SYSTEM),
                                          (void *)bt_cm_common_callback);
}

void bt_cm_deinit(void)
{
    bt_cmgr_report_id("[BT_CM][I] Deinit", 0);

    bt_cm_mutex_lock();
    if (g_bt_cm_cnt) {
        bt_cm_memory_free(g_bt_cm_cnt);
        g_bt_cm_cnt = NULL;
    }
    bt_cm_mutex_unlock();

    //bt_device_manager_dev_register_callback(BT_DEVICE_TYPE_CLASSIC, NULL);
    bt_callback_manager_deregister_callback(bt_callback_type_app_event, (void *)bt_cm_common_callback);
}

void bt_cm_switch_role(bt_bd_addr_t address, bt_role_t role)
{
    bt_cm_mutex_lock();
    bt_cm_remote_device_t *device_p = bt_cm_find_device(BT_CM_FIND_BY_ADDR, (void *)address);
    //bt_cmgr_report_id("[BT_CM][I] Set address:0x%x, role:0x%x", 2, *(uint32_t*)address, role);
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == device_p || (BT_CM_ACL_LINK_CONNECTED > device_p->link_info.link_state),
                                      "[BT_CM][E] Swtich role can't find connected device", 0);
    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG((device_p->flags & BT_CM_REMOTE_FLAG_ROLE_SWITCHING),
                                      "[BT_CM][W] Switch role is ongoing", 0);
    device_p->flags |= BT_CM_REMOTE_FLAG_ROLE_SWITCHING;
    bt_status_t ret = bt_gap_set_role(device_p->link_info.handle, role);
    if (BT_STATUS_SUCCESS != ret) {
        bt_cmgr_report_id("[BT_CM][W] Set role fail:0x%x", 1, ret);
        bt_cm_gap_role_event_handler(BT_GAP_SET_ROLE_CNF, BT_STATUS_FAIL, NULL);
    }
    bt_cm_mutex_unlock();
}

void bt_cm_get_role(bt_bd_addr_t address)
{
    bt_cm_mutex_lock();
    bt_cm_remote_device_t *device_p = bt_cm_find_device(BT_CM_FIND_BY_ADDR, (void *)address);

    BT_CM_CHECK_RET_NO_VALUE_WITH_LOG(NULL == device_p || (BT_CM_ACL_LINK_CONNECTED > device_p->link_info.link_state),
                                      "[BT_CM][E] Get role can't find connected device", 0);
    bt_status_t ret = bt_gap_get_role(device_p->link_info.handle);
    bt_cm_mutex_unlock();
    if (BT_STATUS_SUCCESS != ret) {
        bt_cmgr_report_id("[BT_CM][W] Get role fail:0x%x", 1, ret);
    }
}
