/* Copyright Statement:
 *
 * (C) 2021  MediaTek Inc. All rights reserved.
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

#include "apps_debug.h"
#include "app_mcp.h"

#include "ble_mcp_discovery.h"
#include "bt_gattc_discovery.h"

/**************************************************************************************************
* Define
**************************************************************************************************/

#define APP_LOG_TAG "[APP_MCP]"


typedef struct {
    bt_gattc_discovery_characteristic_t charc[BLE_MCS_MAX_CHARC_NUMBER];
    bt_gattc_discovery_descriptor_t descrp[BLE_MCS_MAX_CHARC_NUMBER];
} app_mcp_discovery_charc_t;

/**************************************************************************************************
* Variable
**************************************************************************************************/
static bt_gattc_discovery_service_t g_mcp_service;
static app_mcp_discovery_charc_t g_mcp_charc;
static app_mcp_callback_t g_mcp_callback = NULL;
/**************************************************************************************************
* Prototype
**************************************************************************************************/
extern bool bt_le_audio_sink_is_link_valid(bt_handle_t handle);

/**************************************************************************************************
* Static function
**************************************************************************************************/
static void app_mcp_set_attribute_callback(bt_handle_t conn_handle)
{
    APPS_LOG_MSGID_I(APP_LOG_TAG", set attribute done conn_hd:0x%x", 1, conn_handle);
    bt_gattc_discovery_continue(conn_handle);
}

static bool app_mcp_discovery_callback(bt_gattc_discovery_event_t *event, bool is_gmcs)
{
    ble_mcp_characteristic_t charc[BLE_MCS_MAX_CHARC_NUMBER];
    ble_mcp_set_service_attribute_parameter_t param;
    bt_status_t status;
    uint8_t i = 0;

    if (NULL == event) {
        return false;
    }

    if ((false == bt_le_audio_sink_is_link_valid(event->conn_handle)) ||
        ((BT_GATTC_DISCOVERY_EVENT_COMPLETE != event->event_type) && (!event->last_instance))) {
        return false;
    }

    APPS_LOG_MSGID_I(APP_LOG_TAG", disc cb is_gmcs:%x charc_num:%d", 2, is_gmcs, g_mcp_service.char_count_found);


    memset(&param, 0, sizeof(ble_mcp_set_service_attribute_parameter_t));

    if (BT_GATTC_DISCOVERY_EVENT_COMPLETE == event->event_type) {

        if (0 != g_mcp_service.char_count_found) {

            /* Fill MCP charc table */
            for (i = 0; i < g_mcp_service.char_count_found; i++) {
                (charc + i)->uuid = g_mcp_service.charateristics[i].char_uuid.uuid.uuid16;
                (charc + i)->value_handle = g_mcp_service.charateristics[i].value_handle;
                if (g_mcp_service.charateristics[i].descr_count_found)
                    (charc + i)->desc_handle = g_mcp_service.charateristics[i].descriptor[0].handle;
            }

            param.charc = charc;
        }

        param.start_handle = g_mcp_service.start_handle;
        param.end_handle = g_mcp_service.end_handle;
        param.charc_num = g_mcp_service.char_count_found;
    }

    param.is_gmcs = is_gmcs;
    param.is_complete = event->last_instance;
    param.callback = app_mcp_set_attribute_callback;

    status = ble_mcp_set_service_attribute(event->conn_handle, &param);
    APPS_LOG_MSGID_I(APP_LOG_TAG", disc cb: set mcp srv ret status = 0x%x", 1, status);

    if (is_gmcs) {
        g_mcp_callback(event->conn_handle, (0 != g_mcp_service.char_count_found) ? true : false);
    }

    if (BT_STATUS_SUCCESS != status) {
        return false;
    }

    return true;
}

static void app_mcp_discovery_gmcs_callback(bt_gattc_discovery_event_t *event)
{
    if (!app_mcp_discovery_callback(event, true)) {
        bt_gattc_discovery_continue((event != NULL ? event->conn_handle : BT_HANDLE_INVALID));
    }
}

static void app_mcp_discovery_mcs_callback(bt_gattc_discovery_event_t *event)
{
    if (!app_mcp_discovery_callback(event, false)) {
        bt_gattc_discovery_continue((event != NULL ? event->conn_handle : BT_HANDLE_INVALID));
    }
}

/**************************************************************************************************
* Public function
**************************************************************************************************/
bt_status_t app_mcp_init(app_mcp_callback_t callback)
{
    uint8_t i = 0;

    if (NULL != g_mcp_callback) {
        return BT_STATUS_FAIL;
    }

    g_mcp_callback = callback;

    memset(&g_mcp_charc, 0, sizeof(app_mcp_discovery_charc_t));

    for (i = 0; i < BLE_MCS_MAX_CHARC_NUMBER; i++) {
        g_mcp_charc.charc[i].descriptor_count = 1;
        g_mcp_charc.charc[i].descriptor = &g_mcp_charc.descrp[i];
    }

    memset(&g_mcp_service, 0, sizeof(bt_gattc_discovery_service_t));
    g_mcp_service.characteristic_count = BLE_MCS_MAX_CHARC_NUMBER;
    g_mcp_service.charateristics = g_mcp_charc.charc;

    bt_gattc_discovery_user_data_t discovery_data = {
        .uuid.type = BLE_UUID_TYPE_16BIT,
        .uuid.uuid.uuid16 = BT_GATT_UUID16_GENERIC_MEDIA_CONTROL_SERVICE,
        .need_cache = FALSE,
        .srv_info = &g_mcp_service,
        .handler = app_mcp_discovery_gmcs_callback
    };
    bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &discovery_data);

    discovery_data.uuid.uuid.uuid16 = BT_GATT_UUID16_MEDIA_CONTROL_SERVICE;
    discovery_data.srv_info = &g_mcp_service;
    discovery_data.handler = app_mcp_discovery_mcs_callback;
    bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &discovery_data);

    return BT_STATUS_SUCCESS;
}
