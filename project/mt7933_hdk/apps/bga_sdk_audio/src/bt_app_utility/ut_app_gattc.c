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

#include "FreeRTOS.h"
#include "timers.h"

#include "ut_app.h"
#include "gatt_service.h"

#ifdef MTK_BLE_DM_SUPPORT
#include "bt_device_manager_le.h"
#endif /* #ifdef MTK_BLE_DM_SUPPORT */

#define MY_MODULE "DEMO"


#define MAX_UUID_INPUT_SIZE         64
#define MAX_ATTR_VALUE_INPUT_SIZE   256

#ifdef MTK_GATT_JITTER_TEST
#define MAX_JITTER_LINKS    6
#define JITTER_TIME_BASE    3000 //ms

typedef struct {
    bt_handle_t conn_handle;
    TickType_t  prev_tick;
    uint32_t    avg_jitter; //ms
    uint32_t    max_jitter; //ms
    uint32_t    min_jitter; //ms
    bool        new_node;
} btapp_jitter_element_t;

//for data latency of each link
typedef struct {
    btapp_jitter_element_t link[MAX_JITTER_LINKS];
    uint8_t connected_links;
} btapp_jitter_info_t;

static btapp_jitter_info_t g_jitter_info = {0};
static bool g_is_jitter_test_on = false;
#endif /* #ifdef MTK_GATT_JITTER_TEST */


//static uint8_t test_value[620] = {0};

static uint32_t g_connection_handle = 0, g_attribute_handle = 0, g_gatt_opcode = 0, g_offset = 0;
static uint32_t g_starting_handle = 0, g_ending_handle = 0, g_value_handle_1 = 0, g_value_handle_2 = 0;

static char g_uuid[MAX_UUID_INPUT_SIZE] = {0};
static uint8_t gatt_long_charc[MAX_ATTR_VALUE_INPUT_SIZE] = {0};
static char g_attribute_value[MAX_ATTR_VALUE_INPUT_SIZE] = {0};

bt_status_t bt_demo_app_gattc_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff);


#ifdef MTK_GATT_JITTER_TEST
static uint32_t _time_elapse(portTickType last, portTickType curr)
{
    portTickType elapse = 0;
    //this API works if the tick elapses less than 0xFFFFFFFF times
    if (curr >= last) { //no overflow
        elapse = curr - last;
    } else { //overflow
        elapse = curr + ((~last) & 0xFFFFFFFF);
    }

    return (elapse * portTICK_PERIOD_MS);
}

static btapp_jitter_element_t *_btapp_find_and_add_jitter_element(bt_handle_t conn_handle)
{
    uint8_t i;

    for (i = 0; i < MAX_JITTER_LINKS; i++) {
        if (g_jitter_info.link[i].conn_handle == conn_handle) {
            return &g_jitter_info.link[i];
        }
    }
    //if cann't find, we can alloc one.
    if (g_jitter_info.connected_links >= MAX_JITTER_LINKS) {
        BT_LOGW("gattc", "%s, no space (%d/%d)", __func__, g_jitter_info.connected_links, MAX_JITTER_LINKS);
        return NULL;
    }

    for (i = 0; i < MAX_JITTER_LINKS; i++) {
        if (g_jitter_info.link[i].conn_handle == 0) {
            g_jitter_info.link[i].conn_handle = conn_handle;
            g_jitter_info.link[i].new_node = true;
            g_jitter_info.connected_links ++;
            BT_LOGI("gattc", "%s, add 0x%x to index = %d", __func__, conn_handle, i);
            return &g_jitter_info.link[i];
        }
    }
    BT_LOGE("gattc", "%s, fail to find element", __func__);
    return NULL;
}

static void _btapp_delete_jitter_element(bt_handle_t conn_handle)
{
    uint8_t i;

    for (i = 0; i < MAX_JITTER_LINKS; i++) {
        if (g_jitter_info.link[i].conn_handle == conn_handle) {
            memset(&g_jitter_info.link[i], 0x0, sizeof(btapp_jitter_element_t));
            g_jitter_info.connected_links --;
            BT_LOGI("gattc", "%s, del 0x%x at index = %d", __func__, conn_handle, i);
            BT_LOGI("gattc", "%s, total links %d", __func__, g_jitter_info.connected_links);
            break;
        }
    }

}

static void _btapp_update_jitter_info(bt_gatt_handle_value_notification_t *p_ind_info)
{
    btapp_jitter_element_t *pNode = NULL;
    TickType_t cur_tick = xTaskGetTickCount();
    uint32_t jitter;

    pNode = _btapp_find_and_add_jitter_element(p_ind_info->connection_handle);
    if (!pNode) {
        return;
    }

    if (pNode->new_node) {
        pNode->new_node = false;
        pNode->prev_tick = cur_tick;
        return;
    }

    jitter = _time_elapse(pNode->prev_tick, cur_tick);

    pNode->prev_tick = cur_tick;
    if (jitter > pNode->max_jitter) {
        pNode->max_jitter = jitter;
    } else if (jitter < pNode->min_jitter || pNode->min_jitter == 0) {
        pNode->min_jitter = jitter;
    }
    if (pNode->avg_jitter == 0)
        pNode->avg_jitter = jitter; //to prevent count 0 to avg.
    else
        pNode->avg_jitter = (pNode->avg_jitter + jitter) / 2;

    BT_LOGI("gattc", "update jitter info: [conn = 0x%x] max %d, min %d, av %d", pNode->conn_handle,
            pNode->max_jitter, pNode->min_jitter, pNode->avg_jitter);
}

/* reset all info but keep connection handle*/
static void _btapp_reset_jitter_tick(void)
{
    uint8_t i;
    btapp_jitter_element_t *pNode = NULL;

    for (i = 0; i < MAX_JITTER_LINKS; i++) {
        pNode = &g_jitter_info.link[i];
        if (pNode->conn_handle != 0) {
            pNode->prev_tick = 0;
            pNode->avg_jitter = 0;
            pNode->max_jitter = 0;
            pNode->min_jitter = 0;
            pNode->new_node = true;
            continue;
        }
    }
    BT_LOGI("gattc", "_btapp_reset_jitter_tick done");
}

static void _btapp_show_all_jitter_info(void)
{
    uint8_t i;
    btapp_jitter_element_t *pNode = NULL;

    for (i = 0; i < MAX_JITTER_LINKS; i++) {
        pNode = &g_jitter_info.link[i];
        if (pNode->conn_handle != 0) {
            BT_LOGI("gattc", "jitter info [%d]: [conn = 0x%x] max %d, min %d, av %d", i,
                    pNode->conn_handle, pNode->max_jitter, pNode->min_jitter, pNode->avg_jitter);
            continue;
        }
    }
    BT_LOGI("gattc", "total connected links %d", g_jitter_info.connected_links);
}

static void _btapp_jitter_test_auto_setup(void)
{
    //set callback first before setup
    ut_app_callback = bt_demo_app_gattc_event_callback;

    _btapp_reset_jitter_tick();

    //1)setup data length
    uint8_t i;
    bt_handle_t connection_handle;
    const char *p_attr_value = "set_data_length";
    bt_gattc_write_charc_req_t req;
    uint8_t buffer[20] = {0};
    uint16_t attr_hdl = 0x001B;

    req.att_req = (bt_att_write_req_t *)buffer;
    req.att_req->opcode = BT_ATT_OPCODE_WRITE_REQUEST;
    req.att_req->attribute_handle = attr_hdl;
    req.attribute_value_length = strlen(p_attr_value);
    memcpy(req.att_req->attribute_value, p_attr_value, strlen(p_attr_value));

    for (i = 0; i < BT_LE_CONNECTION_MAX ; i++) {
        if (connection_cb[i].connection_handle) {
            connection_handle = connection_cb[i].connection_handle;
            BT_LOGD("gattc", "[%d]Set data len to Conn hdl :0x%x", i, connection_handle);
            bt_gattc_write_charc(connection_handle, &req);
        }
    }
}

static void _btapp_jitter_test_auto_run(void)
{
    //set callback first before setup
    ut_app_callback = bt_demo_app_gattc_event_callback;

    //start loop test
    uint8_t i;
    bt_handle_t connection_handle;
    const char *p_attr_value = "-loop";
    bt_gattc_write_charc_req_t req;
    uint8_t buffer[20] = {0};
    uint16_t attr_hdl = 0x001A;

    req.att_req = (bt_att_write_req_t *)buffer;
    req.att_req->opcode = BT_ATT_OPCODE_WRITE_REQUEST;
    req.att_req->attribute_handle = attr_hdl;
    req.attribute_value_length = strlen(p_attr_value);
    memcpy(req.att_req->attribute_value, p_attr_value, strlen(p_attr_value));

    for (i = 0; i < BT_LE_CONNECTION_MAX ; i++) {
        if (connection_cb[i].connection_handle) {
            connection_handle = connection_cb[i].connection_handle;
            BT_LOGD("gattc", "[%d]Set data len to Conn hdl :0x%x", i, connection_handle);
            bt_gattc_write_charc(connection_handle, &req);
        }
    }
}

static void _btapp_jitter_test_auto_stop(void)
{
    //set callback first before setup
    ut_app_callback = bt_demo_app_gattc_event_callback;

    //stop loop test
    uint8_t i;
    bt_handle_t connection_handle;
    const char *p_attr_value = "-lstop";
    bt_gattc_write_charc_req_t req;
    uint8_t buffer[20] = {0};
    uint16_t attr_hdl = 0x001A;

    req.att_req = (bt_att_write_req_t *)buffer;
    req.att_req->opcode = BT_ATT_OPCODE_WRITE_REQUEST;
    req.att_req->attribute_handle = attr_hdl;
    req.attribute_value_length = strlen(p_attr_value);
    memcpy(req.att_req->attribute_value, p_attr_value, strlen(p_attr_value));

    for (i = 0; i < BT_LE_CONNECTION_MAX ; i++) {
        if (connection_cb[i].connection_handle) {
            connection_handle = connection_cb[i].connection_handle;
            BT_LOGD("gattc", "[%d]Set data len to Conn hdl :0x%x", i, connection_handle);
            bt_gattc_write_charc(connection_handle, &req);
        }
    }
}
#endif /* #ifdef MTK_GATT_JITTER_TEST */

static bt_status_t ut_app_gattc_req_le_bond(void)
{
    pairing_config_req.auth_req |= BT_GAP_LE_SMP_AUTH_REQ_BONDING;
    pairing_config_req.io_capability = BT_GAP_LE_SMP_DISPLAY_ONLY;
    pairing_config_req.oob_data_flag = BT_GAP_LE_SMP_OOB_DATA_NOT_PRESENTED;
    pairing_config_req.initiator_key_distribution |= BT_GAP_LE_SMP_KEY_DISTRIBUTE_SIGN;
    pairing_config_req.initiator_key_distribution |= BT_GAP_LE_SMP_KEY_DISTRIBUTE_ENCKEY;
    pairing_config_req.responder_key_distribution |= BT_GAP_LE_SMP_KEY_DISTRIBUTE_SIGN;
    pairing_config_req.responder_key_distribution |= BT_GAP_LE_SMP_KEY_DISTRIBUTE_ENCKEY;
    pairing_config_req.maximum_encryption_key_size = 16;
#ifdef MTK_BLE_DM_SUPPORT
    bt_device_manager_le_gap_set_pairing_configuration(&pairing_config_req);
#endif /* #ifdef MTK_BLE_DM_SUPPORT */
    return bt_gap_le_bond(g_connection_handle, &pairing_config_req);
}

bt_status_t bt_demo_app_gattc_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{
    switch (msg) {

        case BT_GAP_LE_BONDING_COMPLETE_IND: {

                if (status != BT_STATUS_SUCCESS) {
                    return status;
                }

                switch (g_gatt_opcode) {

                    case BT_GATTC_READ_CHARC: {
                            bt_gattc_read_charc_req_t req;

                            req.opcode = BT_ATT_OPCODE_READ_REQUEST;
                            req.attribute_handle = g_attribute_handle;

                            bt_gattc_read_charc(g_connection_handle, &req);
                            ut_app_callback = bt_demo_app_gattc_event_callback;
                            break;
                        }

                    case BT_GATTC_READ_LONG_CHARC: {
                            bt_gattc_read_long_charc_value_req_t req;

                            req.opcode = BT_ATT_OPCODE_READ_BLOB_REQUEST;
                            req.attribute_handle = g_attribute_handle;
                            req.value_offset = g_offset;

                            bt_gattc_read_long_charc(g_connection_handle, &req);
                            ut_app_callback = bt_demo_app_gattc_event_callback;
                            break;
                        }
                    case BT_GATTC_READ_USING_CHARC_UUID: {

                            bt_gattc_read_using_charc_uuid_req_t req;

                            if (strlen(g_uuid) == 4) {
                                uint16_t uuid_16 = (uint16_t)strtoul(g_uuid, NULL, 16);
                                bt_uuid_load(&(req.type), (void *)&uuid_16, 2);
                            } else {
                                uint8_t attribute_value[16] = {0};

                                char tmpc[3] = {0};
                                uint32_t t;
                                for (uint16_t i = 0; i < 16; ++i) {
                                    tmpc[0] = g_uuid[2 * i];
                                    tmpc[1] = g_uuid[2 * i + 1];
                                    if (sscanf(tmpc, "%lx", &t) == 1)
                                        attribute_value[15 - i] =  t;
                                }
                                bt_uuid_load(&req.type, (void *)attribute_value, 16);

                            }

                            req.opcode = BT_ATT_OPCODE_READ_BY_TYPE_REQUEST;
                            req.starting_handle = g_starting_handle;
                            req.ending_handle = g_ending_handle;

                            bt_gattc_read_using_charc_uuid(g_connection_handle, &req);
                            ut_app_callback = bt_demo_app_gattc_event_callback;
                            break;
                        }
                    case BT_GATTC_READ_MULTI_CHARC_VALUES: {
                            bt_gattc_read_multi_charc_values_req_t req;

                            req.handle_length = 5;
                            uint8_t buffer[5] = {0};

                            req.att_req = (bt_att_read_multiple_req_t *)buffer;
                            req.att_req->opcode = BT_ATT_OPCODE_READ_MULTIPLE_REQUEST;

                            req.att_req->set_of_handles[0] = g_value_handle_1;
                            req.att_req->set_of_handles[1] = g_value_handle_2;

                            bt_gattc_read_multi_charc_values(g_connection_handle, &req);
                            ut_app_callback = bt_demo_app_gattc_event_callback;
                            break;
                        }

                    case BT_GATTC_WRITE_CHARC: {
                            bt_gattc_write_charc_req_t req;
                            req.attribute_value_length = strlen(g_attribute_value);
                            uint8_t buffer[20] = {0};

                            req.att_req = (bt_att_write_req_t *)buffer;
                            req.att_req->opcode = BT_ATT_OPCODE_WRITE_REQUEST;
                            req.att_req->attribute_handle = g_attribute_handle;
                            memcpy(req.att_req->attribute_value, g_attribute_value, strlen(g_attribute_value));

                            bt_gattc_write_charc(g_connection_handle, &req);
                            ut_app_callback = bt_demo_app_gattc_event_callback;
                            break;
                        }
                    case BT_GATTC_WRITE_LONG_CHARC: {
                            bt_gattc_prepare_write_charc_req_t req;
                            req.attribute_value_length = strlen(g_attribute_value);
                            uint8_t buffer_1[30] = {0};

                            memcpy(gatt_long_charc, g_attribute_value, strlen(g_attribute_value));

                            req.att_req = (bt_att_prepare_write_req_t *)buffer_1;
                            req.att_req->opcode = BT_ATT_OPCODE_PREPARE_WRITE_REQUEST;
                            req.att_req->attribute_handle = g_attribute_handle;
                            req.att_req->value_offset = g_offset;
                            req.att_req->part_attribute_value = gatt_long_charc;
                            bt_gattc_prepare_write_charc(g_connection_handle, 0, 0, &req);
                            ut_app_callback = bt_demo_app_gattc_event_callback;
                            break;
                        }
                    case BT_GATTC_RELIABLE_WRITE_CHARC: {
                            bt_gattc_prepare_write_charc_req_t req;
                            req.attribute_value_length = strlen(g_attribute_value);
                            uint8_t buffer_1[30] = {0};

                            memcpy(gatt_long_charc, g_attribute_value, strlen(g_attribute_value));

                            req.att_req = (bt_att_prepare_write_req_t *)buffer_1;
                            req.att_req->opcode = BT_ATT_OPCODE_PREPARE_WRITE_REQUEST;
                            req.att_req->attribute_handle = g_attribute_handle;
                            req.att_req->value_offset = g_offset;
                            req.att_req->part_attribute_value = gatt_long_charc;
                            bt_gattc_prepare_write_charc(g_connection_handle, 1, 0, &req);
                            ut_app_callback = bt_demo_app_gattc_event_callback;
                            break;
                        }

                    default:
                        break;

                }
                break;
            }
        case BT_GAP_LE_DISCONNECT_IND: {
#ifdef MTK_GATT_JITTER_TEST
                bt_hci_evt_disconnect_complete_t *disconnect_complete = (bt_hci_evt_disconnect_complete_t *)buff;
                if (g_is_jitter_test_on)
                    _btapp_delete_jitter_element(disconnect_complete->connection_handle);
#endif /* #ifdef MTK_GATT_JITTER_TEST */
            }
            break;
        case BT_GATTC_EXCHANGE_MTU:
            BT_COLOR_SET(BT_COLOR_RED);
            BT_LOGI("APP", "BT_GATTC_EXCHANGE_MTU");
            BT_COLOR_SET(BT_COLOR_BLUE);

            if (buff == NULL) {
                BT_LOGI("APP", "status = %d", status);
                BT_COLOR_SET(BT_COLOR_WHITE);
                break;
            }

            bt_gatt_exchange_mtu_rsp_t rsp = *((bt_gatt_exchange_mtu_rsp_t *)buff);

            BT_LOGI("APP", "mtu = %d", rsp.server_rx_mtu);
            BT_COLOR_SET(BT_COLOR_WHITE);
            break;

        case BT_GATTC_DISCOVER_PRIMARY_SERVICE: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_DISCOVER_PRIMARY_SERVICE");
                BT_COLOR_SET(BT_COLOR_BLUE);

                bt_gattc_read_by_group_type_rsp_t rsp = *((bt_gattc_read_by_group_type_rsp_t *)buff);

                if (status == BT_STATUS_SUCCESS && rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "BT_GATTC_DISCOVER_PRIMARY_SERVICE FINISHED!!");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                if (rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                uint16_t end_group_handle = 0, starting_handle = 0, uuid = 0;
                bt_uuid_t uuid128;
                uint8_t *attribute_data_list = rsp.att_rsp->attribute_data_list;
                uint8_t num_of_data = rsp.length / rsp.att_rsp->length;

                for (int i = 1; i <= num_of_data; i++) {

                    memcpy(&starting_handle, attribute_data_list + (i - 1) * rsp.att_rsp->length, 2);
                    memcpy(&end_group_handle, attribute_data_list + (i - 1) * rsp.att_rsp->length + 2, 2);
                    BT_LOGI("APP", "data : %d", i);
                    if (rsp.att_rsp->length == 6) {
                        memcpy(&uuid, attribute_data_list + (i - 1) * rsp.att_rsp->length + 4, rsp.att_rsp->length - 4);
                        BT_LOGI("APP", "starting_handle = 0x%08x, end_group_handle = 0x%08x, uuid = 0x%08x", starting_handle, end_group_handle, uuid);
                    } else {
                        memcpy(&uuid128.uuid, attribute_data_list + (i - 1) * rsp.att_rsp->length + 4, rsp.att_rsp->length - 4);
                        BT_LOGI("APP", "starting_handle = 0x%08x, end_group_handle = 0x%08x, uuid", starting_handle, end_group_handle);
                        for (int i = 15; i >= 0; i--) {
                            printf("0x%02x ", uuid128.uuid[i]);
                        }
                        printf("\n");
                    }
                }

                BT_COLOR_SET(BT_COLOR_WHITE);
            }
            break;

        case BT_GATTC_DISCOVER_PRIMARY_SERVICE_BY_UUID: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_DISCOVER_PRIMARY_SERVICE_BY_UUID");
                BT_COLOR_SET(BT_COLOR_BLUE);

                bt_gattc_find_by_type_value_rsp_t rsp = *((bt_gattc_find_by_type_value_rsp_t *)buff);

                if (status == BT_STATUS_SUCCESS && rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "BT_GATTC_DISCOVER_PRIMARY_SERVICE_BY_UUID FINISHED!!");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                if (rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                uint16_t attribute_handle = 0, end_group_handle = 0;
                uint8_t *handles_info_list = rsp.att_rsp->handles_info_list;
                uint8_t num_of_info = (rsp.length - 1) / 4;

                for (int i = 0 ; i < num_of_info; i++) {
                    memcpy(&attribute_handle, handles_info_list + 4 * i, 2);
                    memcpy(&end_group_handle, handles_info_list + 4 * i + 2, 2);
                    BT_LOGI("APP", "attribute_handle = 0x%08x, end_group_handle = 0x%08x", attribute_handle, end_group_handle);
                }

                BT_COLOR_SET(BT_COLOR_WHITE);
                break;
            }

        case BT_GATTC_FIND_INCLUDED_SERVICES: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_FIND_INCLUDED_SERVICES");
                BT_COLOR_SET(BT_COLOR_BLUE);

                bt_gattc_read_by_type_rsp_t rsp = *((bt_gattc_read_by_type_rsp_t *)buff);

                if (status == BT_STATUS_SUCCESS && rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "BT_GATTC_FIND_INCLUDED_SERVICES FINISHED!!");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                if (rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                uint8_t *attribute_data_list = rsp.att_rsp->attribute_data_list;
                uint16_t attribute_handle = 0, starting_handle = 0, ending_handle = 0, uuid = 0;

                if (rsp.att_rsp->length <= 8) {
                    uint8_t num_of_data = (rsp.length - 2) / rsp.att_rsp->length;

                    for (int i = 0 ; i < num_of_data; i++) {
                        memcpy(&attribute_handle, attribute_data_list + rsp.att_rsp->length * i, 2);
                        memcpy(&starting_handle, attribute_data_list + rsp.att_rsp->length * i + 2, 2);
                        memcpy(&ending_handle, attribute_data_list + rsp.att_rsp->length * i + 4, 2);
                        memcpy(&uuid, attribute_data_list + rsp.att_rsp->length * i + 6, 2);
                        BT_LOGI("APP", "attribute_handle = 0x%08x, starting_handle = 0x%08x, end_group_handle = 0x%08x, uuid = 0x%08x",
                                attribute_handle, starting_handle, ending_handle, uuid);
                    }
                } else {
                    bt_uuid_t uuid128;

                    memcpy(&attribute_handle, attribute_data_list, 2);
                    memcpy(&starting_handle, attribute_data_list + 2, 2);
                    memcpy(&ending_handle, attribute_data_list + 4, 2);
                    memcpy(&uuid128.uuid, attribute_data_list + 6, 16);

                    BT_LOGI("APP", "attribute_handle = 0x%08x, starting_handle = 0x%08x, end_group_handle = 0x%08x",
                            attribute_handle, starting_handle, ending_handle);
                    for (int i = 15; i >= 0; i--) {
                        printf("0x%02x ", uuid128.uuid[i]);
                    }
                    printf("\n");
                }
                BT_COLOR_SET(BT_COLOR_WHITE);
            }
            break;

        case BT_GATTC_DISCOVER_CHARC: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_DISCOVER_CHARC");
                BT_COLOR_SET(BT_COLOR_BLUE);

                bt_gattc_read_by_type_rsp_t rsp = *((bt_gattc_read_by_type_rsp_t *)buff);

                if (status == BT_STATUS_SUCCESS && rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "BT_GATTC_DISCOVER_CHARC FINISHED!!");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                if (rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                uint8_t *attribute_data_list = rsp.att_rsp->attribute_data_list;
                uint16_t attribute_handle = 0;
                uint8_t attribute_value[30] = {0};

                uint8_t num_of_data = (rsp.length - 2) / rsp.att_rsp->length;

                if (rsp.att_rsp->length < 20) {
                    for (int i = 0 ; i < num_of_data; i++) {
                        memcpy(&attribute_handle, attribute_data_list + i * rsp.att_rsp->length, 2);
                        memcpy(&attribute_value, attribute_data_list + i * rsp.att_rsp->length + 2, rsp.att_rsp->length - 2);
                        BT_LOGI("APP", "num_of_data = %d, attribute handle = 0x%08x", i, attribute_handle);
                        BT_LOGI("APP", "attribute_value:");
                        for (int j = 0; j < rsp.att_rsp->length - 2; j++) {
                            printf("0x%02x ", attribute_value[j]);
                        }
                        printf("\n");
                    }
                } else {
                    bt_uuid_t uuid128;

                    memcpy(&attribute_handle, attribute_data_list, 2);
                    memcpy(&attribute_value, attribute_data_list + 3, 2);
                    memcpy(&uuid128.uuid, attribute_data_list + 5, 16);
                    BT_LOGI("APP", "attr_hdl = 0x%08x, val_hdl = 0x%02x%02x",
                            attribute_handle, attribute_value[0], attribute_value[1]);
                    for (int i = 15; i >= 0; i--) {
                        printf("0x%02x ", uuid128.uuid[i]);
                    }
                    printf("\n");
                }

                BT_COLOR_SET(BT_COLOR_WHITE);
            }
            break;

        case BT_GATTC_DISCOVER_CHARC_DESCRIPTOR: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_DISCOVER_CHARC_DESCRIPTOR");
                BT_COLOR_SET(BT_COLOR_BLUE);

                bt_gattc_find_info_rsp_t rsp = *((bt_gattc_find_info_rsp_t *)buff);

                if (status == BT_STATUS_SUCCESS && rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "BT_GATTC_DISCOVER_CHARC_DESCRIPTOR FINISHED!!");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                if (rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                uint8_t format = rsp.att_rsp->format;

                uint16_t attribute_handle = 0, attribute_value = 0;
                uint8_t attribute_length = 0;

                if (format == 0x02) {
                    /* uuid 128 */
                    attribute_length = 18;
                    bt_uuid_t uuid128;

                    uint8_t num_of_attribute = (rsp.length - 2) / attribute_length;
                    memcpy(&attribute_handle, rsp.att_rsp->info_data + (num_of_attribute - 1) * attribute_length, 2);
                    memcpy(&uuid128, rsp.att_rsp->info_data + (num_of_attribute - 1) * attribute_length + 2, 16);

                    BT_LOGI("APP", "attribute handle = 0x%08x", attribute_handle);
                    for (int i = 15; i >= 0; i--) {
                        printf("0x%02x ", uuid128.uuid[i]);
                    }
                    printf("\n");

                    BT_COLOR_SET(BT_COLOR_WHITE);
                    return BT_STATUS_SUCCESS;

                } else {
                    /* uuid 16 */
                    attribute_length = 4;

                    uint8_t num_of_attribute = (rsp.length - 2) / attribute_length;
                    memcpy(&attribute_handle, rsp.att_rsp->info_data + (num_of_attribute - 1) * attribute_length, 2);
                    memcpy(&attribute_value, rsp.att_rsp->info_data + (num_of_attribute - 1) * attribute_length + 2, 2);


                    BT_LOGI("APP", "attribute handle = 0x%08x, attribute value = 0x%08x", attribute_handle, attribute_value);

                    BT_COLOR_SET(BT_COLOR_WHITE);
                    return BT_STATUS_SUCCESS;

                }

            }
            break;

        case BT_GATTC_READ_CHARC:
        case BT_GATTC_READ_LONG_CHARC: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_READ_CHARC");
                BT_COLOR_SET(BT_COLOR_BLUE);

                if (status == BT_ATT_ERRCODE_INSUFFICIENT_ENCRYPTION_KEY_SIZE ||
                    status == BT_ATT_ERRCODE_INSUFFICIENT_AUTHENTICATION ||
                    status == BT_ATT_ERRCODE_INSUFFICIENT_ENCRYPTION) {

                    ut_app_gattc_req_le_bond();
                    return BT_STATUS_SUCCESS;
                }

                bt_gattc_read_rsp_t rsp = *((bt_gattc_read_rsp_t *)buff);

                if (status == BT_STATUS_SUCCESS && rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "BT_GATTC_READ_CHARC FINISHED!!");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                if (rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }


                uint8_t op_code = rsp.att_rsp->opcode;

                BT_LOGI("APP", "op_code = 0x%02x", op_code);
                uint8_t length = rsp.length - 1;
                for (int i = 0; i < length ; i++)
                    printf("0x%02x ", rsp.att_rsp->attribute_value[i]);
                printf("\n");
                if (status == BT_STATUS_SUCCESS) {
                    BT_LOGI("APP", "bt_gattc_read_charc FINISHED!!");
                }
                BT_COLOR_SET(BT_COLOR_WHITE);
                return BT_STATUS_SUCCESS;

            }

        case BT_GATTC_READ_USING_CHARC_UUID: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_READ_USING_CHARC_UUID");
                BT_COLOR_SET(BT_COLOR_BLUE);

                if (status == BT_ATT_ERRCODE_INSUFFICIENT_ENCRYPTION_KEY_SIZE ||
                    status == BT_ATT_ERRCODE_INSUFFICIENT_AUTHENTICATION ||
                    status == BT_ATT_ERRCODE_INSUFFICIENT_ENCRYPTION) {

                    ut_app_gattc_req_le_bond();
                    return BT_STATUS_SUCCESS;
                }


                bt_gattc_read_by_type_rsp_t rsp = *((bt_gattc_read_by_type_rsp_t *)buff);

                if (status == BT_STATUS_SUCCESS && rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "BT_GATTC_READ_USING_CHARC_UUID FINISHED!!");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                if (rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                uint8_t *attribute_data_list = rsp.att_rsp->attribute_data_list;
                uint16_t attribute_handle = 0;
                uint8_t attribute_value[30] = {0};

                uint8_t num_of_data = (rsp.length - 2) / rsp.att_rsp->length;

                for (int i = 0 ; i < num_of_data; i++) {
                    memcpy(&attribute_handle, attribute_data_list + i * rsp.att_rsp->length, 2);
                    memcpy(&attribute_value, attribute_data_list + i * rsp.att_rsp->length + 2, rsp.att_rsp->length - 2);
                    BT_LOGI("APP", "num_of_data = %d, attribute handle = 0x%08x", i, attribute_handle);
                    BT_LOGI("APP", "attribute_value:");
                    for (int j = 0; j < rsp.att_rsp->length - 2; j++) {
                        printf("0x%02x ", attribute_value[j]);
                    }
                }
                BT_COLOR_SET(BT_COLOR_WHITE);
                return BT_STATUS_SUCCESS;
            }

        case BT_GATTC_READ_MULTI_CHARC_VALUES: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_READ_MULTI_CHARC_VALUES");
                BT_COLOR_SET(BT_COLOR_BLUE);
                if (status == BT_STATUS_SUCCESS) {
                    BT_LOGI("APP", "bt_gattc_read_multi_charc_values FINISHED!!");
                } else {
                    BT_LOGI("APP", "bt_gattc_read_multi_charc_values, status = %d!!", status);
                }

                if (status == BT_ATT_ERRCODE_INSUFFICIENT_ENCRYPTION_KEY_SIZE ||
                    status == BT_ATT_ERRCODE_INSUFFICIENT_AUTHENTICATION ||
                    status == BT_ATT_ERRCODE_INSUFFICIENT_ENCRYPTION) {

                    ut_app_gattc_req_le_bond();
                    return BT_STATUS_SUCCESS;
                }

                bt_gattc_read_multiple_rsp_t rsp = *((bt_gattc_read_multiple_rsp_t *)buff);

                if (status == BT_STATUS_SUCCESS && rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "BT_GATTC_READ_MULTI_CHARC_VALUES FINISHED!!");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                if (rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                uint8_t op_code = rsp.att_rsp->opcode;

                BT_LOGI("APP", "op_code = 0x%02x", op_code);

                uint8_t length = rsp.length - 1;
                for (int i = 0; i < length ; i++)
                    printf("0x%02x ", rsp.att_rsp->set_of_values[i]);


                BT_COLOR_SET(BT_COLOR_WHITE);
                return BT_STATUS_SUCCESS;
            }
        case BT_GATTC_WRITE_CHARC:
        case BT_GATTC_WRITE_LONG_CHARC:
        case BT_GATTC_RELIABLE_WRITE_CHARC: {

                if (status == BT_ATT_ERRCODE_INSUFFICIENT_ENCRYPTION_KEY_SIZE ||
                    status == BT_ATT_ERRCODE_INSUFFICIENT_AUTHENTICATION ||
                    status == BT_ATT_ERRCODE_INSUFFICIENT_ENCRYPTION) {

                    ut_app_gattc_req_le_bond();
                    return BT_STATUS_SUCCESS;
                }

                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_WRITE_CHARC");
                BT_COLOR_SET(BT_COLOR_BLUE);
                BT_LOGI("APP", "bt_gattc_write_charc, status = %d!!", status);
                BT_COLOR_SET(BT_COLOR_WHITE);
                return BT_STATUS_SUCCESS;
            }
        case BT_GATTC_CHARC_VALUE_NOTIFICATION: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_CHARC_VALUE_NOTIFICATION");
                BT_COLOR_SET(BT_COLOR_BLUE);
                if (status == BT_STATUS_SUCCESS) {
                    BT_LOGI("APP", "BT_GATTC_CHARC_VALUE_NOTIFICATION FINISHED!!");
                }

                bt_gatt_handle_value_notification_t rsp = *((bt_gatt_handle_value_notification_t *)buff);

                if (status == BT_STATUS_SUCCESS && rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "BT_GATTC_CHARC_VALUE_NOTIFICATION FINISHED!!");
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                if (rsp.att_rsp == NULL) {
                    BT_LOGI("APP", "status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                uint8_t op_code = rsp.att_rsp->opcode;
                uint16_t attribute_handle = rsp.att_rsp->handle;

                BT_LOGI("APP", "op_code = 0x%02x", op_code);
                BT_LOGI("APP", "attribute_handle = 0x%08x", attribute_handle);
                uint8_t length = rsp.length - 3;
                for (int i = 0; i < length ; i++)
                    printf("0x%02x ", rsp.att_rsp->attribute_value[i]);
                printf("\n");

                if (status == BT_STATUS_SUCCESS) {
                    BT_LOGI("APP", "BT_GATTC_CHARC_VALUE_NOTIFICATION FINISHED!!");
                }
                BT_COLOR_SET(BT_COLOR_WHITE);
                return BT_STATUS_SUCCESS;
            }
        case BT_GATTC_CHARC_VALUE_INDICATION: {
                BT_COLOR_SET(BT_COLOR_RED);
                BT_LOGI("APP", "BT_GATTC_CHARC_VALUE_INDICATION");
                BT_COLOR_SET(BT_COLOR_BLUE);

                bt_gatt_handle_value_notification_t rsp = *((bt_gatt_handle_value_notification_t *)buff);

                if (rsp.att_rsp == NULL) {
                    BT_LOGW("APP", "att_rsp == NULL!!, status = %d", status);
                    BT_COLOR_SET(BT_COLOR_WHITE);
                    break;
                }

                uint8_t op_code = rsp.att_rsp->opcode;
                uint16_t attribute_handle = rsp.att_rsp->handle;
                uint8_t length = rsp.length - 3;

                BT_LOGI("APP", "op_code = 0x%02x, attr_hdl = 0x%08x, len = %d", op_code, attribute_handle, length);
#if 0 //open to debu raw data
                for (int i = 0; i < length ; i++)
                    printf("0x%02x ", rsp.att_rsp->attribute_value[i]);
                printf("\n");
#endif /* #if 0 //open to debu raw data */
                BT_COLOR_SET(BT_COLOR_WHITE);

#ifdef MTK_GATT_JITTER_TEST
                if (g_is_jitter_test_on)
                    _btapp_update_jitter_info(&rsp);
#endif /* #ifdef MTK_GATT_JITTER_TEST */
                return BT_STATUS_SUCCESS;
            }

    }
    return BT_STATUS_SUCCESS;

}

void ut_app_gattc_print_help(void)
{
    printf("ble gattc  -Gattc cmds\n"
           "  exchange mtu [con_hdl][mtu_size]\n"
           "  primary service [con_hdl]\n"
           "  primary by UUID [con_hdl][UUID]\n"
           "  find included services [con_hdl][st_hdl][end_hdl]\n"
           "  discover all charc [con_hdl][st_hdl][end_hdl]\n"
           "  read charc value [con_hdl][attr_hdl]\n"
           "  set notification [con_hdl][attr_hdl]\n"
           "  set indication [con_hdl][attr_hdl]\n"
           "  write charc value [con_hdl][attr_hdl][value]\n"
           "  discover charc by UUID [con_hdl][UUID][st_hdl][end_hdl]\n"
           "  discover charc descriptor [con_hdl][st_hdl][end_hdl]\n"
           "  write long charc value [con_hdl][attr_hdl][offset][val]\n"
           "  read long charc value [con_hdl][attr_hdl][offset]\n"
#ifdef MTK_GATT_JITTER_TEST
           "  jitter test\n"
           "  show jitter info\n"
           "  jitter auto setup\n"
           "  jitter auto run\n"
           "  jitter auto stop\n"
#endif /* #ifdef MTK_GATT_JITTER_TEST */
          );
}


//bt_status_t bt_app_demo_io_callback(void *input, void *output) -- tt: rename to bt_app_gattc_io_callback
bt_status_t bt_app_gattc_io_callback(void *input, void *output)
{
    const char *cmd = (const char *)input;

    if (UT_APP_CMP("gattc ?")) {
        ut_app_gattc_print_help();
    } else if (UT_APP_CMP("gattc dump bonded info list")) {
        BT_COLOR_SET(BT_COLOR_RED);
        dump_bonded_info_list();
        BT_COLOR_SET(BT_COLOR_WHITE);
    }
    /* GATTC exchange MTU */
    else if (UT_APP_CMP("gattc exchange mtu")) {
        g_gatt_opcode = BT_GATTC_EXCHANGE_MTU;

        const char *handle = cmd + 19;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        bt_gattc_exchange_mtu_req_t req;
        uint16_t mtu_size = (uint16_t)strtoul(cmd + 24, NULL, 16);
        if (mtu_size > 0x0200 || mtu_size < 0x0017) {
            BT_LOGE("APP", "input mtu size should be 0017~0200");
            return BT_STATUS_FAIL;
        }
        req.opcode = BT_ATT_OPCODE_EXCHANGE_MTU_REQUEST;
        req.client_rx_mtu = mtu_size;

        bt_gattc_exchange_mtu(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    else if (UT_APP_CMP("gattc get mtu")) {
        uint16_t conn_hdl = 0, mtu = 0;
        conn_hdl = (uint16_t)strtoul(cmd + 14, NULL, 16);
        if (conn_hdl >= 0x0EFF) {
            BT_LOGE("APP", "input handle must be 0~0EFF");
            return BT_STATUS_FAIL;
        }
        mtu = bt_gattc_get_mtu(conn_hdl);
        BT_LOGI("APP", "gatt mtu = %d", mtu);
    }

    else if (UT_APP_CMP("gattc primary service")) {

        g_gatt_opcode = BT_GATTC_DISCOVER_PRIMARY_SERVICE;

        const char *handle = cmd + 21;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        bt_gattc_discover_primary_service_req_t req;
        req.opcode = BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST;
        req.starting_handle = 0x0001;
        req.ending_handle = 0xFFFF;
        req.type16 =  BT_GATT_UUID16_PRIMARY_SERVICE;

        bt_gattc_discover_primary_service(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC primary by UUID 0201 1800 */
    /*       connection handle, uuid */
    else if (UT_APP_CMP("gattc primary by UUID")) {

        g_gatt_opcode = BT_GATTC_DISCOVER_PRIMARY_SERVICE_BY_UUID;

        const char *handle = cmd + 22;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *uuid = cmd + 27;
        bt_gattc_discover_primary_service_by_uuid_req_t gatt_req;

        uint8_t req[30];
        memset(req, 0, sizeof(req));
        gatt_req.att_req = (bt_att_find_by_type_value_req_t *)req;

        if (strlen(uuid) == 4) {
            uint16_t attribute_value = (uint16_t)strtoul(uuid, NULL, 16);
            if (attribute_value == 0xFFFF)
                BT_LOGE("APP", "%s, primary by UUID, arrt value 0xFFFF", __func__);

            uint8_t attr_val[2];
            attr_val[0] = attribute_value & 0x00FF;
            attr_val[1] = (attribute_value & 0xFF00) >> 8;

            memcpy(gatt_req.att_req->attribute_value, attr_val, 2);
            gatt_req.attribute_value_length = 2;
        } else {
            uint8_t attribute_value[16] = {0};

            char tmpc[3] = {0};
            uint32_t t;
            for (uint16_t i = 0; i < 16; ++i) {
                tmpc[0] = uuid[2 * i];
                tmpc[1] = uuid[2 * i + 1];
                if (sscanf(tmpc, "%lx", &t) == 1)
                    attribute_value[15 - i] =  t;
            }

            memcpy(gatt_req.att_req->attribute_value, attribute_value, 16);
            gatt_req.attribute_value_length = 16;
        }

        gatt_req.att_req->opcode = BT_ATT_OPCODE_FIND_BY_TYPE_VALUE_REQUEST;
        gatt_req.att_req->starting_handle = 0x0001;
        gatt_req.att_req->ending_handle = 0xFFFF;
        gatt_req.att_req->uuid =  BT_GATT_UUID16_PRIMARY_SERVICE;

        bt_gattc_discover_primary_service_by_uuid(g_connection_handle, &gatt_req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC find included services 0201 xxxx xxxx */
    /*       connection handle, starting handle, ending handle */
    else if (UT_APP_CMP("gattc find included services")) {

        g_gatt_opcode = BT_GATTC_FIND_INCLUDED_SERVICES;

        const char *handle = cmd + 29;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *starting_handle = cmd + 34;
        const char *ending_handle = cmd + 39;

        bt_gattc_find_included_services_req_t req;

        req.opcode = BT_ATT_OPCODE_READ_BY_TYPE_REQUEST;
        req.starting_handle = (uint16_t)strtoul(starting_handle, NULL, 16);
        if (req.starting_handle == 0) {
            BT_LOGE("APP", "starting_handle should > 0");
            return BT_STATUS_FAIL;
        }

        req.ending_handle = (uint16_t)strtoul(ending_handle, NULL, 16);
        if (req.ending_handle == 0) {
            BT_LOGE("APP", "ending_handle should > 0");
            return BT_STATUS_FAIL;
        }

        req.type16 =  BT_GATT_UUID16_INCLUDE;

        bt_gattc_find_included_services(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC discover all charc 0201 xxxx xxxx*/
    /*       connection handle, starting handle, ending handle */
    else if (UT_APP_CMP("gattc discover all charc")) {

        g_gatt_opcode = BT_GATTC_DISCOVER_CHARC;

        const char *handle = cmd + 25;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *starting_handle = cmd + 30;
        const char *ending_handle = cmd + 35;

        bt_gattc_discover_charc_req_t req;

        req.opcode = BT_ATT_OPCODE_READ_BY_TYPE_REQUEST;
        req.starting_handle = (uint16_t)strtoul(starting_handle, NULL, 16);
        if (req.starting_handle == 0) {
            BT_LOGE("APP", "starting_handle should > 0");
            return BT_STATUS_FAIL;
        }

        req.ending_handle = (uint16_t)strtoul(ending_handle, NULL, 16);
        if (req.ending_handle == 0) {
            BT_LOGE("APP", "ending_handle should > 0");
            return BT_STATUS_FAIL;
        }

        uint16_t uuid_16 = BT_GATT_UUID16_CHARC;
        bt_uuid_load(&(req.type), (void *)&uuid_16, 2);

        bt_gattc_discover_charc(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC discover charc by UUID 0201 uuid xxxx xxxx*/
    /*       connection handle, uuid, starting handle, ending handle */
    else if (UT_APP_CMP("gattc discover charc by UUID")) {

        g_gatt_opcode = BT_GATTC_DISCOVER_CHARC;

        const char *handle = cmd + 29;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *uuid = cmd + 34;
        const char *starting_handle = cmd + 39;
        const char *ending_handle = cmd + 44;


        bt_gattc_discover_charc_req_t req;

        req.opcode = BT_ATT_OPCODE_READ_BY_TYPE_REQUEST;
        req.starting_handle = (uint16_t)strtoul(starting_handle, NULL, 16);
        if (req.starting_handle == 0) {
            BT_LOGE("APP", "starting_handle should > 0");
            return BT_STATUS_FAIL;
        }

        req.ending_handle = (uint16_t)strtoul(ending_handle, NULL, 16);
        if (req.ending_handle == 0) {
            BT_LOGE("APP", "ending_handle should > 0");
            return BT_STATUS_FAIL;
        }

        if (strlen(uuid) == 4) {
            uint16_t uuid_16 = (uint16_t)strtoul(uuid, NULL, 16);
            if (uuid_16 == 0) {
                BT_LOGE("APP", "uuid should > 0");
                return BT_STATUS_FAIL;
            }

            bt_uuid_load(&(req.type), (void *)&uuid_16, 2);
        } else {
            uint8_t attribute_value[16] = {0};

            char tmpc[3] = {0};
            uint32_t t;
            for (uint16_t i = 0; i < 16; ++i) {
                tmpc[0] = uuid[2 * i];
                tmpc[1] = uuid[2 * i + 1];
                if (sscanf(tmpc, "%lx", &t) == 1)
                    attribute_value[15 - i] =  t;
            }
            bt_uuid_load(&req.type, (void *)attribute_value, 16);
        }

        bt_gattc_discover_charc(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC discover charc descriptor 0201 xxxx xxxx*/
    /*       connection handle, starting handle, ending handle */
    else if (UT_APP_CMP("gattc discover charc descriptor")) {

        g_gatt_opcode = BT_GATTC_DISCOVER_CHARC_DESCRIPTOR;

        const char *handle = cmd + 32;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *starting_handle = cmd + 37;
        const char *ending_handle = cmd + 42;
        bt_gattc_discover_charc_descriptor_req_t req;

        req.opcode = BT_ATT_OPCODE_FIND_INFORMATION_REQUEST;
        req.starting_handle = (uint16_t)strtoul(starting_handle, NULL, 16);
        if (req.starting_handle == 0) {
            BT_LOGE("APP", "starting_handle should > 0");
            return BT_STATUS_FAIL;
        }

        req.ending_handle = (uint16_t)strtoul(ending_handle, NULL, 16);
        if (req.ending_handle == 0) {
            BT_LOGE("APP", "ending_handle should > 0");
            return BT_STATUS_FAIL;
        }

        bt_gattc_discover_charc_descriptor(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC read charc value 0201 xxxx*/
    /*       connection handle, attribute handle */
    else if (UT_APP_CMP("gattc read charc value")) {

        g_gatt_opcode = BT_GATTC_READ_CHARC;

        const char *handle = cmd + 23;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *attribute_handle = cmd + 28;
        g_attribute_handle = (uint16_t)strtoul(attribute_handle, NULL, 16);
        if (g_attribute_handle == 0) {
            BT_LOGE("APP", "attr handle should > 0");
            return BT_STATUS_FAIL;
        }

        bt_gattc_read_charc_req_t req;

        req.opcode = BT_ATT_OPCODE_READ_REQUEST;
        req.attribute_handle = g_attribute_handle;

        bt_gattc_read_charc(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /*GATTC read long charc value 0201 xxxx offset*/
    /*       connection handle, attribute handle, offset */
    else if (UT_APP_CMP("gattc read long charc value")) {

        g_gatt_opcode = BT_GATTC_READ_LONG_CHARC;

        const char *handle = cmd + 28;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *attribute_handle = cmd + 33;
        g_attribute_handle = (uint16_t)strtoul(attribute_handle, NULL, 16);
        if (g_attribute_handle == 0) {
            BT_LOGE("APP", "attr handle should > 0");
            return BT_STATUS_FAIL;
        }

        const char *offset = cmd + 38;
        g_offset = (uint16_t)strtoul(offset, NULL, 16);
        if (g_offset == 0xFFFF)
            BT_LOGE("APP", "offset reachs max val: 0xFFFF");

        bt_gattc_read_long_charc_value_req_t req;

        req.opcode = BT_ATT_OPCODE_READ_BLOB_REQUEST;
        req.attribute_handle = g_attribute_handle;
        req.value_offset = g_offset;

        bt_gattc_read_long_charc(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC read using charc uuid 0201 xxxx xxxx uuid */
    /*       connection handle, starting handle, ending handle, uuid */
    else if (UT_APP_CMP("gattc read using charc uuid")) {

        g_gatt_opcode = BT_GATTC_READ_USING_CHARC_UUID;

        const char *handle = cmd + 28;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *starting_handle = cmd + 33;
        g_starting_handle = (uint16_t)strtoul(starting_handle, NULL, 16);
        if (g_starting_handle == 0) {
            BT_LOGE("APP", "starting_handle should > 0");
            return BT_STATUS_FAIL;
        }

        const char *ending_handle = cmd + 38;
        g_ending_handle = (uint16_t)strtoul(ending_handle, NULL, 16);
        if (g_ending_handle == 0) {
            BT_LOGE("APP", "ending_handle should > 0");
            return BT_STATUS_FAIL;
        }

        const char *uuid = cmd + 43;
        if (strlen(uuid) > MAX_UUID_INPUT_SIZE) {
            BT_LOGE("gattc", "input uuid is too long!!");
            return BT_STATUS_SUCCESS;
        }

        memcpy(g_uuid, uuid, strlen(uuid));

        bt_gattc_read_using_charc_uuid_req_t req;

        if (strlen(uuid) == 4) {
            uint16_t uuid_16 = (uint16_t)strtoul(uuid, NULL, 16);
            if (uuid_16 == 0) {
                BT_LOGE("APP", "uuid should > 0");
                return BT_STATUS_FAIL;
            }

            bt_uuid_load(&(req.type), (void *)&uuid_16, 2);
        } else {
            uint8_t attribute_value[16] = {0};

            char tmpc[3] = {0};
            uint32_t t;
            for (uint16_t i = 0; i < 16; ++i) {
                tmpc[0] = uuid[2 * i];
                tmpc[1] = uuid[2 * i + 1];
                if (sscanf(tmpc, "%lx", &t) == 1)
                    attribute_value[15 - i] =  t;
            }
            bt_uuid_load(&req.type, (void *)attribute_value, 16);

        }

        req.opcode = BT_ATT_OPCODE_READ_BY_TYPE_REQUEST;
        req.starting_handle = g_starting_handle;
        req.ending_handle = g_ending_handle;

        bt_gattc_read_using_charc_uuid(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC read multi charc value 0201 xxxx xxxx */
    /*       connection handle, value handle 1, value handle 2 */
    else if (UT_APP_CMP("gattc read multi charc value")) {

        g_gatt_opcode = BT_GATTC_READ_MULTI_CHARC_VALUES;

        const char *handle = cmd + 29;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *value_handle_1 = cmd + 34;
        g_value_handle_1 = (uint16_t)strtoul(value_handle_1, NULL, 16);
        if (g_value_handle_1 == 0) {
            BT_LOGE("APP", "%s, read multi charc value, value handle 1 error", __func__);
            return BT_STATUS_FAIL;
        }

        const char *value_handle_2 = cmd + 39;
        g_value_handle_2 = (uint16_t)strtoul(value_handle_2, NULL, 16);
        if (g_value_handle_2 == 0) {
            BT_LOGE("APP", "%s, read multi charc value, value handle 2 error", __func__);
            return BT_STATUS_FAIL;
        }

        bt_gattc_read_multi_charc_values_req_t req;

        req.handle_length = 5;
        uint8_t buffer[5] = {0};

        req.att_req = (bt_att_read_multiple_req_t *)buffer;
        req.att_req->opcode = BT_ATT_OPCODE_READ_MULTIPLE_REQUEST;

        req.att_req->set_of_handles[0] = g_value_handle_1;
        req.att_req->set_of_handles[1] = g_value_handle_2;

        bt_gattc_read_multi_charc_values(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC write wo rsp 0201 xxxx value */
    /*       connection handle, attribute handle, attribute value */
    else if (UT_APP_CMP("gattc write wo rsp")) {

        g_gatt_opcode = 0;//NULL;

        const char *handle = cmd + 19;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *attribute_handle = cmd + 24;
        const char *attribute_value = cmd + 29;

        bt_gattc_write_without_rsp_req_t req;
        req.attribute_value_length = strlen(attribute_value);
        uint8_t buffer[30] = {0};

        req.att_req = (bt_att_write_command_t *)buffer;
        req.att_req->opcode = BT_ATT_OPCODE_WRITE_COMMAND;
        req.att_req->attribute_handle = (uint16_t)strtoul(attribute_handle, NULL, 16);
        if (req.att_req->attribute_handle == 0) {
            BT_LOGE("APP", "attr handle should > 0");
            return BT_STATUS_FAIL;
        }

        memcpy(req.att_req->attribute_value, attribute_value, strlen(attribute_value));

        bt_gattc_write_without_rsp(g_connection_handle, 0, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC signed write wo rsp 0201 xxxx value */
    /*       connection handle, attribute handle, attribute value */
    else if (UT_APP_CMP("gattc signed write wo rsp")) {

        g_gatt_opcode = 0;//NULL;

        pairing_config_req.initiator_key_distribution |= BT_GAP_LE_SMP_KEY_DISTRIBUTE_SIGN;
        pairing_config_req.responder_key_distribution |= BT_GAP_LE_SMP_KEY_DISTRIBUTE_SIGN;
#ifdef MTK_BLE_DM_SUPPORT
        bt_device_manager_le_gap_set_pairing_configuration(&pairing_config_req);
#endif /* #ifdef MTK_BLE_DM_SUPPORT */

        const char *handle = cmd + 26;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *attribute_handle = cmd + 31;
        const char *attribute_value = cmd + 36;

        bt_gattc_write_without_rsp_req_t req;
        req.attribute_value_length = strlen(attribute_value);
        uint8_t buffer[30] = {0};

        req.att_req = (bt_att_write_command_t *)buffer;
        req.att_req->opcode = BT_ATT_OPCODE_SIGNED_WRITE_COMMAND;
        req.att_req->attribute_handle = (uint16_t)strtoul(attribute_handle, NULL, 16);
        if (req.att_req->attribute_handle == 0) {
            BT_LOGE("APP", "attr handle should > 0");
            return BT_STATUS_FAIL;
        }

        memcpy(req.att_req->attribute_value, attribute_value, strlen(attribute_value));

        bt_gattc_write_without_rsp(g_connection_handle, 1, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC write charc value 0201 xxxx value */
    /*       connection handle, attribute handle, attribute value */
    else if (UT_APP_CMP("gattc write charc value")) {

        g_gatt_opcode = BT_GATTC_WRITE_CHARC;

        const char *handle = cmd + 24;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *attribute_handle = cmd + 29;
        g_attribute_handle = (uint16_t)strtoul(attribute_handle, NULL, 16);
        if (g_attribute_handle == 0) {
            BT_LOGE("APP", "attr handle should > 0");
            return BT_STATUS_FAIL;
        }

        const char *attribute_value = cmd + 34;
        if (strlen(attribute_value) > MAX_ATTR_VALUE_INPUT_SIZE) {
            BT_LOGE("gattc", "input attr value is too long!!");
            return BT_STATUS_SUCCESS;
        }
        memcpy(g_attribute_value, attribute_value, strlen(attribute_value));

        bt_gattc_write_charc_req_t req;
        req.attribute_value_length = strlen(attribute_value);
        uint8_t buffer[20] = {0};

        req.att_req = (bt_att_write_req_t *)buffer;
        req.att_req->opcode = BT_ATT_OPCODE_WRITE_REQUEST;
        req.att_req->attribute_handle = g_attribute_handle;
        if ((sizeof(buffer) - sizeof(req.att_req)) < strlen(attribute_value)) {
            BT_LOGE("gattc", "input attr value over the buf size!!");
            return BT_STATUS_SUCCESS;
        }
        memcpy(req.att_req->attribute_value, attribute_value, strlen(attribute_value));

        bt_gattc_write_charc(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC write long charc value 0201 xxxx xxxx value */
    /*       connection handle, attribute handle, offset, attribute value */
    else if (UT_APP_CMP("gattc write long charc value")) {

        g_gatt_opcode = BT_GATTC_WRITE_LONG_CHARC;

        const char *handle = cmd + 29;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *attribute_handle = cmd + 34;
        g_attribute_handle = (uint16_t)strtoul(attribute_handle, NULL, 16);
        if (g_attribute_handle == 0) {
            BT_LOGE("APP", "attr handle should > 0");
            return BT_STATUS_FAIL;
        }

        const char *offset = cmd + 39;
        g_offset = (uint16_t)strtoul(offset, NULL, 16);
        if (g_offset == 0xFFFF)
            BT_LOGE("APP", "offset reachs max val: 0xFFFF");

        const char *attribute_value = cmd + 44;
        if (strlen(attribute_value) > MAX_ATTR_VALUE_INPUT_SIZE) {
            BT_LOGE("gattc", "input attr value is too long!!");
            return BT_STATUS_SUCCESS;
        }
        memcpy(g_attribute_value, attribute_value, strlen(attribute_value));

        bt_gattc_prepare_write_charc_req_t req;
        req.attribute_value_length = strlen(attribute_value);
        uint8_t buffer_1[30] = {0};

        memcpy(gatt_long_charc, attribute_value, strlen(attribute_value));

        req.att_req = (bt_att_prepare_write_req_t *)buffer_1;
        req.att_req->opcode = BT_ATT_OPCODE_PREPARE_WRITE_REQUEST;
        req.att_req->attribute_handle = g_attribute_handle;
        req.att_req->value_offset = (uint16_t)strtoul(offset, NULL, 16);
        req.att_req->part_attribute_value = gatt_long_charc;
        bt_gattc_prepare_write_charc(g_connection_handle, 0, req.att_req->value_offset, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC reliable write charc value 0201 xxxx xxxx value */
    /*       connection handle, attribute handle, offset, attribute value */
    else if (UT_APP_CMP("gattc reliable write charc value")) {

        g_gatt_opcode = BT_GATTC_RELIABLE_WRITE_CHARC;

        const char *handle = cmd + 33;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *attribute_handle = cmd + 38;
        g_attribute_handle = (uint16_t)strtoul(attribute_handle, NULL, 16);
        if (g_attribute_handle == 0) {
            BT_LOGE("APP", "attr handle should > 0");
            return BT_STATUS_FAIL;
        }

        const char *offset = cmd + 43;
        g_offset = (uint16_t)strtoul(offset, NULL, 16);
        if (g_offset == 0xFFFF)
            BT_LOGE("APP", "offset reachs max val: 0xFFFF");

        const char *attribute_value = cmd + 48;
        if (strlen(attribute_value) > MAX_ATTR_VALUE_INPUT_SIZE) {
            BT_LOGE("gattc", "input attr value is too long!!");
            return BT_STATUS_SUCCESS;
        }
        memcpy(g_attribute_value, attribute_value, strlen(attribute_value));

        bt_gattc_prepare_write_charc_req_t req;
        req.attribute_value_length = 25;
        uint8_t buffer_1[30] = {0};

        memcpy(gatt_long_charc, attribute_value, strlen(attribute_value));

        req.att_req = (bt_att_prepare_write_req_t *)buffer_1;
        req.att_req->opcode = BT_ATT_OPCODE_PREPARE_WRITE_REQUEST;
        req.att_req->attribute_handle = g_attribute_handle;
        req.att_req->value_offset = (uint16_t)strtoul(offset, NULL, 16);
        req.att_req->part_attribute_value = gatt_long_charc;
        bt_gattc_prepare_write_charc(g_connection_handle, 1, 0, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC execute write req 0201 flag*/
    /*       connection handle, flag */
    else if (UT_APP_CMP("gattc execute write req")) {

        g_gatt_opcode = 0;//NULL;

        const char *handle = cmd + 24;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *flag = cmd + 29;

        bt_gattc_execute_write_req_t req;

        req.opcode = BT_ATT_OPCODE_EXECUTE_WRITE_REQUEST;
        req.flags = (uint8_t)strtoul(flag, NULL, 16);
        if (req.flags == 0xFF)
            BT_LOGE("APP", "%s, execute write req, flags 0xFF", __func__);

        bt_gattc_send_execute_write_req(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC set notification 0201 xxxx*/
    /*       connection handle, attribute handle */
    else if (UT_APP_CMP("gattc set notification")) {

        const char *handle = cmd + 23;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        const char *attribute_handle = cmd + 28;

        bt_gattc_write_charc_req_t req;
        req.attribute_value_length = 2;
        uint8_t buffer[20] = {0};

        req.att_req = (bt_att_write_req_t *)buffer;
        req.att_req->opcode = BT_ATT_OPCODE_WRITE_REQUEST;
        req.att_req->attribute_handle = (uint16_t)strtoul(attribute_handle, NULL, 16);
        if (req.att_req->attribute_handle == 0) {
            BT_LOGE("APP", "attr handle should > 0");
            return BT_STATUS_FAIL;
        }

        uint16_t notification = 0x0001;
        memcpy(req.att_req->attribute_value, &notification, 2);

        bt_gattc_write_charc(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }

    /* GATTC set indication 0201 xxxx*/
    /*       connection handle, attribute handle */
    else if (UT_APP_CMP("gattc set indication")) {

        const char *handle = cmd + 21;
        const char *attribute_handle = cmd + 26;
        g_connection_handle = (uint16_t)strtoul(handle, NULL, 16);
        if (g_connection_handle >= 0x1000) {
            BT_LOGE("APP", "input handle must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        bt_gattc_write_charc_req_t req;
        req.attribute_value_length = 2;
        uint8_t buffer[20] = {0};

        req.att_req = (bt_att_write_req_t *)buffer;
        req.att_req->opcode = BT_ATT_OPCODE_WRITE_REQUEST;
        req.att_req->attribute_handle = (uint16_t)strtoul(attribute_handle, NULL, 16);
        if (req.att_req->attribute_handle == 0) {
            BT_LOGE("APP", "attr handle should > 0");
            return BT_STATUS_FAIL;
        }

        uint16_t indication = 0x0002;
        memcpy(req.att_req->attribute_value, &indication, 2);

        bt_gattc_write_charc(g_connection_handle, &req);
        ut_app_callback = bt_demo_app_gattc_event_callback;
    }
#ifdef MTK_GATT_JITTER_TEST
    /* A GATT statistic test code on client side */
    else if (UT_APP_CMP("gattc jitter test")) {
        const char *value = cmd + 18;
        if (strncmp("on", value, strlen("on")) == 0) {
            g_is_jitter_test_on = true;
        } else
            g_is_jitter_test_on = false;
        BT_LOGW("gattc", "set jitter test to %s", g_is_jitter_test_on ? "on" : "off");
        ut_app_callback = bt_demo_app_gattc_event_callback;
    } else if (UT_APP_CMP("gattc show jitter info")) {
        _btapp_show_all_jitter_info();
        ut_app_callback = bt_demo_app_gattc_event_callback;
    } else if (UT_APP_CMP("gattc jitter auto setup")) {
        g_is_jitter_test_on = true;
        _btapp_jitter_test_auto_setup();
    } else if (UT_APP_CMP("gattc jitter auto run")) {
        _btapp_jitter_test_auto_run();
    } else if (UT_APP_CMP("gattc jitter auto stop")) {
        _btapp_jitter_test_auto_stop();
    }
#endif /* #ifdef MTK_GATT_JITTER_TEST */
    else {
        BT_LOGW("gattc", "cmd not support");
    }

    return BT_STATUS_SUCCESS;
}
