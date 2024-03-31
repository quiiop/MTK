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
#include <string.h>
#ifdef MTK_BLE_SMTCN_ENABLE
#include "ble_smtcn.h"
#endif /* #ifdef MTK_BLE_SMTCN_ENABLE */

#include "ut_app.h"
#ifdef __MTK_BT_MESH_ENABLE__
#include "mesh_app_util.h"
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */

#define MAX_GATTS_INDICATE_SIZE         256
#define BTAPP_GATTS_IND_TIMER_PERIOD    3000 //ms
#if defined(MTK_BLE_SMTCN_ENABLE) && defined(__BT_GATTS_EH__)
#define BTAPP_GATTS_DATA_BUF_SIZE       32
#endif /* #if defined(MTK_BLE_SMTCN_ENABLE) && defined(__BT_GATTS_EH__) */

#ifdef MTK_BT_BAS_SERVICE_ENABLE
bool g_bas_service_changed_tmp = false;
int g_bas_service_changed_tmp_num = 0;
uint16_t g_bas_ccc_value_tmp = 0;
#endif /* #ifdef MTK_BT_BAS_SERVICE_ENABLE */

bt_handle_t g_connect_handle = 0;

static bt_gattc_charc_value_notification_indication_t *gp_gatts_ind_rsp;
static uint16_t g_gatts_ind_conn_hdl = 0;
static TimerHandle_t g_gatts_test_timer_id = NULL;


static void _bt_ut_gatts_ind_test_timer_cb(TimerHandle_t xTimer)
{
    BT_LOGW("gatts", "set indicate at timer conn_hdl = 0x%x, attr_hdl = 0x%x (len %d)",
            g_gatts_ind_conn_hdl, gp_gatts_ind_rsp->att_req.handle, gp_gatts_ind_rsp->attribute_value_length);
    bt_gatts_send_charc_value_notification_indication(g_gatts_ind_conn_hdl, gp_gatts_ind_rsp);

}

/**
 * @brief   This function is a static callback to listen the stack event. Provide a user-defined callback.
 * @param[in] msg     is the callback message type.
 * @param[in] status  is the status of the callback message.
 * @param[in] buf     is the payload of the callback message.
 * @return            The status of this operation returned from the callback.
 */
bt_status_t bt_ut_gatts_service_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{

    switch (msg) {
        case BT_GAP_LE_CONNECT_IND:
            if (status == BT_STATUS_SUCCESS) {
                bt_gap_le_connection_ind_t *connection_ind = (bt_gap_le_connection_ind_t *)buff;

                BT_LOGI("gatts", "set connHdl=0x%04x", connection_ind->connection_handle);
                g_connect_handle = connection_ind->connection_handle;;
            }
            break;
        default:
            break;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_app_gatts_io_callback(void *input, void *output)
{
    const char *cmd = (const char *)input;

#ifdef MTK_BT_BAS_SERVICE_ENABLE

    /* Usage: test gatts bas level notify--cmd:ble gatts bas notify [level] level:1~100 */
    if (UT_APP_CMP("gatts bas notify")) {
        uint8_t buf[32] = {0};
        bt_gattc_charc_value_notification_indication_t *bas_noti_rsp;
        bas_noti_rsp = (bt_gattc_charc_value_notification_indication_t *) buf;

        uint8_t battery_level = (uint8_t)strtoul(cmd + 17, NULL, 10);
        if (battery_level <= 0 || battery_level > 100) {
            BT_LOGW("gatts", "value 1~100");
            return BT_STATUS_SUCCESS;
        }

        if (g_bas_ccc_value_tmp != 0x01) {
            BT_LOGW("gatts", "need client config CCC -- notify");
            return BT_STATUS_SUCCESS;
        }

        bas_noti_rsp->att_req.opcode = BT_ATT_OPCODE_HANDLE_VALUE_NOTIFICATION;
        bas_noti_rsp->att_req.handle = 0x0033;
        memcpy((void *)(bas_noti_rsp->att_req.attribute_value), (void *)(&battery_level), sizeof(uint8_t));

        bas_noti_rsp->attribute_value_length = 3 + sizeof(uint8_t);
        BT_LOGW("gatts", "set bas notify");
        bt_gatts_send_charc_value_notification_indication(g_connect_handle, bas_noti_rsp);
    }

    /* Usage: test gatts bas level indicate--cmd:ble gatts bas indicate [level] level:1~100 */
    else if (UT_APP_CMP("gatts bas indicate")) {
        uint8_t buf[32] = {0};
        bt_gattc_charc_value_notification_indication_t *bas_noti_rsp;
        bas_noti_rsp = (bt_gattc_charc_value_notification_indication_t *) buf;
        uint8_t battery_level = (uint8_t)strtoul(cmd + 19, NULL, 10);
        if (battery_level <= 0 || battery_level > 100) {
            BT_LOGW("gatts", "value 1~100");
            return BT_STATUS_SUCCESS;
        }

        if (g_bas_ccc_value_tmp != 0x02) {
            BT_LOGW("gatts", "need client config CCC -- indicate");
            return BT_STATUS_SUCCESS;
        }

        bas_noti_rsp->att_req.opcode = BT_ATT_OPCODE_HANDLE_VALUE_INDICATION;
        bas_noti_rsp->att_req.handle = 0x0033;
        memcpy((void *)(bas_noti_rsp->att_req.attribute_value), (void *)(&battery_level), sizeof(uint8_t));

        bas_noti_rsp->attribute_value_length = 3 + sizeof(uint8_t);
        BT_LOGW("gatts", "set bas indicate");
        bt_gatts_send_charc_value_notification_indication(g_connect_handle, bas_noti_rsp);
    }

    /* Usage: test gatts service changeed--cmd:ble gatts bas serice_changed, nRF APK will start discovery
    after receive twice cmd*/
    else if (UT_APP_CMP("gatts bas service_changed")) {
        BT_LOGW("gatts", "bas sevice changed");
        g_bas_service_changed_tmp_num++;

        if (g_bas_service_changed_tmp_num > 2) {
            ut_gatts_service_change(true, g_connect_handle);
            g_bas_service_changed_tmp = false;
            if (g_bas_service_changed_tmp_num == 4)
                g_bas_service_changed_tmp_num = 0;
        } else {
            ut_gatts_service_change(false, g_connect_handle);
            g_bas_service_changed_tmp = true;
        }
#if 0 //apk find service after notify service change twice, and no update GUI
        if (g_bas_service_changed_tmp) {
            bg_bas_service_changed_tmp = false;
            ut_gatts_service_change(true);
        } else {
            ut_gatts_service_change(false);
            bg_bas_service_changed_tmp = true;
        }
#endif /* #if 0 //apk find service after notify service change twice, and no update GUI */
    }
#else /* #ifdef MTK_BT_BAS_SERVICE_ENABLE */
    if (0) {

    }
#endif /* #ifdef MTK_BT_BAS_SERVICE_ENABLE */
    /* Usage: test gatts indicate [conn hdl] [att hdl] [value] */
    else if (UT_APP_CMP("gatts indicate")) {
        bt_gattc_charc_value_notification_indication_t *ind_rsp = gp_gatts_ind_rsp;
        uint16_t conn_hdl = 0;
        uint16_t attr_hdl = 0;

        conn_hdl = strtoul(cmd + 15, NULL, 16);
        if (conn_hdl >= 0x1000) {
            BT_LOGE("APP", "conn_hdl must be 0000~0EFF");
            return BT_STATUS_FAIL;
        }

        attr_hdl = strtoul(cmd + 20, NULL, 16);
        if (attr_hdl == 0) {
            BT_LOGE("APP", "attr_hdl should > 0");
            return BT_STATUS_FAIL;
        }

        const char *attr_value = cmd + 25;
        uint32_t attr_value_len = strlen(attr_value);
        uint32_t max_buf_len = MAX_GATTS_INDICATE_SIZE - sizeof(bt_gattc_charc_value_notification_indication_t);
        bool isAutoGenMode = false;
        bool isLoopMode = false;
        uint8_t i, *pValue = NULL;

        if (strncmp("-dummy", attr_value, strlen("-dummy")) == 0) {
            BT_LOGI("gatts", "Auto gen dummy data");
            isAutoGenMode = true;
        } else if (strncmp("-loop", attr_value, strlen("-loop")) == 0) { //start loop test
            BT_LOGI("gatts", "Auto gen dummy data and auto loop");
            if (g_gatts_test_timer_id) {
                BT_LOGE("gatts", "timer already exist!!");
                return BT_STATUS_SUCCESS;
            }
            isAutoGenMode = true;
            isLoopMode = true;
        } else if (strncmp("-lstop", attr_value, strlen("-lstop")) == 0) { //stop loop test
            BT_LOGE("gatts", "stop timer (%p)", g_gatts_test_timer_id);
            if (g_gatts_test_timer_id) {
                BaseType_t ret;
                ret = xTimerDelete(g_gatts_test_timer_id, 0);
                if (ret != pdPASS)
                    BT_LOGE("gatts", "g_gatts_test_timer_id delete fail!");
                g_gatts_test_timer_id = NULL;
            }
            return BT_STATUS_SUCCESS;
        }

        if (isAutoGenMode && (max_buf_len < 128)) {
            BT_LOGE("gatts", "No space to auto gen dummy data!!");
            return BT_STATUS_SUCCESS;
        }

        if (attr_value_len > max_buf_len) {
            BT_LOGE("gatts", "input string too long (exp: %d, cur =%d)", max_buf_len, attr_value_len);
            return BT_STATUS_SUCCESS;
        }

        if (!ind_rsp) {
            ind_rsp = (bt_gattc_charc_value_notification_indication_t *) pvPortMalloc(MAX_GATTS_INDICATE_SIZE);
            if (!ind_rsp) {
                BT_LOGE("gatts", "Fail to alloc buf(%d)", MAX_GATTS_INDICATE_SIZE);
                return BT_STATUS_SUCCESS;
            }
            gp_gatts_ind_rsp = ind_rsp;
        }

        ind_rsp->att_req.opcode = BT_ATT_OPCODE_HANDLE_VALUE_INDICATION;
        ind_rsp->att_req.handle = attr_hdl;

        //a special test case to force output 128bytes dummy data to peer due to cli cannot input over 128bytes
        if (isAutoGenMode) {
            pValue = (uint8_t *)ind_rsp->att_req.attribute_value;
            attr_value_len = 128;
            for (i = 0; i < attr_value_len; i++)
                *(pValue + i) = i;
        } else
            memcpy((void *)(ind_rsp->att_req.attribute_value), (void *)attr_value, attr_value_len);

        ind_rsp->attribute_value_length = 3 + attr_value_len;
        BT_LOGW("gatts", "set indicate conn_hdl = 0x%x, attr_hdl = 0x%x (len %d)", conn_hdl, attr_hdl, attr_value_len);
        bt_gatts_send_charc_value_notification_indication(conn_hdl, ind_rsp);

        if (isLoopMode) {
            //a special test case to send 128bytes every 3sec
            g_gatts_ind_conn_hdl = conn_hdl;
            g_gatts_test_timer_id = xTimerCreate("GATTS_IND_TIMER",
                                                 BTAPP_GATTS_IND_TIMER_PERIOD / portTICK_PERIOD_MS,
                                                 pdTRUE, (void *)0, _bt_ut_gatts_ind_test_timer_cb);
            BT_LOGI("gatts", "create timer = %p", g_gatts_test_timer_id);
            xTimerStart(g_gatts_test_timer_id, 1);
        } else {
            vPortFree((void *)gp_gatts_ind_rsp);
            gp_gatts_ind_rsp = NULL;
        }
    }
#ifdef MTK_GATT_JITTER_TEST
    /* Usage: add jitter service */
    else if (UT_APP_CMP("gatts add jitter srv")) {
        bt_gatts_add_jitter_test_srv();
    }
#endif /* #ifdef MTK_GATT_JITTER_TEST */
#ifdef __MTK_BT_MESH_ENABLE__
    else if (UT_APP_CMP("gatts switch mesh")) {
        BT_LOGI("gatts", "switch to mesh");
        bt_gatts_switch_init_mode(BT_BOOT_INIT_MODE_MESH);
    }

    else if (UT_APP_CMP("gatts switch gatts")) {
        if (!bt_app_mesh_is_enabled()) {
#if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE))
            BT_LOGI("gatts", "switch to gatts");
            bt_gatts_switch_init_mode(BT_BOOT_INIT_MODE_GATTS);
#endif /* #if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE)) */
        } else {
            BT_LOGI("APP", "can't switch gatts when mesh is enabled!");
        }
    }
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */
    else if (UT_APP_CMP("gatts notify srv change")) {
        const char *handle = cmd + strlen("gatts notify srv change ");
        uint16_t hdl_tmp = strtoul(handle, NULL, 16);
        if (hdl_tmp >= 0xFFFF) {
            BT_LOGE("gatts", "invalid handle!");
            return BT_STATUS_FAIL;
        }
        ut_gatts_service_change_notify((bt_handle_t)hdl_tmp);
    }
#if defined(MTK_BLE_SMTCN_ENABLE) && defined(__BT_GATTS_EH__)
    else if (UT_APP_CMP("gatts set smtcn_manual_rsp")) {
        uint16_t flag = 0;
        flag = (uint16_t)strtoul(cmd + 27, NULL, 16);
        ble_smtcn_set_server_response_flag(flag);
    } else if (UT_APP_CMP("gatts wr_rsp")) {
        uint16_t rsp = 0;
        rsp = (uint16_t)strtoul(cmd + 13, NULL, 16);
        ble_smtcn_send_write_response(rsp);
    }
    /* ex: gatts rd_rsp [size] [offset] [status] [data] */
    else if (UT_APP_CMP("gatts rd_rsp")) {
        uint16_t size = 0, offset = 0, st = 0;
        const char *data = cmd + 22;
        uint8_t buf[BTAPP_GATTS_DATA_BUF_SIZE];

        size = (uint16_t)strtoul(cmd + 13, NULL, 16);
        if (size > 0xFFFE) {
            BT_LOGE("APP", "size should be 2bytes");
            return BT_STATUS_FAIL;
        }
        offset = (uint16_t)strtoul(cmd + 16, NULL, 16);
        if (offset > 0xFFFE) {
            BT_LOGE("APP", "offset should be 2bytes");
            return BT_STATUS_FAIL;
        }
        st = (uint16_t)strtoul(cmd + 19, NULL, 16);
        if (st > 0x3F) {
            BT_LOGE("APP", "status should < 0x3f");
            return BT_STATUS_FAIL;
        }
        memcpy(buf, data, sizeof(buf));
        ble_smtcn_send_read_response(buf, size, offset, st);
    }
#endif /* #if defined(MTK_BLE_SMTCN_ENABLE) && defined(__BT_GATTS_EH__) */
#ifdef MTK_BT_EXTRA_CLI_TEST
    /* Usage: ble gatts check_auth [conn hdl] [att hdl] [read or write] */
    else if (UT_APP_CMP("gatts check_auth")) {
        bt_gatts_authorization_check_req_t auth_req_check;
        uint16_t hdl = 0;
        uint8_t rw = 0;

        hdl = strtoul(cmd + 24, NULL, 16);
        if (hdl >= 0x0EFF) {
            BT_LOGE("APP", "conn_hdl must be 0~0x0EFF");
            return BT_STATUS_FAIL;
        }
        auth_req_check.connection_handle = hdl;

        hdl = strtoul(cmd + 29, NULL, 16);
        if (hdl == 0) {
            BT_LOGE("APP", "attr_hdl should > 0");
            return BT_STATUS_FAIL;
        }
        auth_req_check.attribute_handle = hdl;

        rw = strtoul(cmd + 34, NULL, 16);
        if (rw > 1) {
            BT_LOGE("APP", "read write flag 0 or 1");
            return BT_STATUS_FAIL;
        }
        auth_req_check.read_write = (rw + 1) % 2;
        BT_LOGI("APP", "con_hdl: 0x%04x, attr_hdl: 0x%04x, rw: %d", auth_req_check.connection_handle,
                auth_req_check.attribute_handle, auth_req_check.read_write);

        if (BT_STATUS_SUCCESS == bt_gatts_get_authorization_check_result(&auth_req_check))
            BT_LOGI("APP", "gatts auth ok");
        else
            BT_LOGE("APP", "gatts auth fail");
    }
#endif /* #ifdef MTK_BT_EXTRA_CLI_TEST */
    else
        BT_LOGW("gatts", "cmd not support");


    return BT_STATUS_SUCCESS;
}

#ifdef MTK_GATT_JITTER_TEST
static uint32_t ble_jitter_charc_value_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    char *pData = (char *) data;
    uint32_t max_buf_len = MAX_GATTS_INDICATE_SIZE - sizeof(bt_gattc_charc_value_notification_indication_t);
    uint16_t conn_hdl = 0x0200;
    uint16_t attr_hdl = 0x0015;
    uint32_t attr_value_len = 0;
    bool isAutoGenMode = false;
    bool isLoopMode = false;
    uint8_t i, *pValue = NULL;
    bt_gattc_charc_value_notification_indication_t *ind_rsp = gp_gatts_ind_rsp;

    BT_LOGI("gatts", "jitter charc cb,rw = %d, handle = 0x%x, size = %d\n", rw, handle, size);
    BT_LOGI("gatts", "jitter charc cb, data = 0x%x %x\n", pData[0], pData[1]);

    if (max_buf_len < 128) {
        //please set a bigger size for MAX_GATTS_INDICATE_SIZE
        return (uint32_t)size;
    }

    if (strncmp("-dummy", pData, strlen("-dummy")) == 0) {
        BT_LOGI("gatts", "Auto gen dummy data to indication");
        isAutoGenMode = true;
    } else if (strncmp("-loop", pData, strlen("-loop")) == 0) { //start loop test
        BT_LOGI("gatts", "Auto gen dummy data and auto loop");
        if (g_gatts_test_timer_id) {
            BT_LOGE("gatts", "timer already exist!!");
            return (uint32_t)size;
        }
        isAutoGenMode = true;
        isLoopMode = true;
    } else if (strncmp("-lstop", pData, strlen("-lstop")) == 0) { //stop loop test
        BT_LOGE("gatts", "stop timer (%p)", g_gatts_test_timer_id);
        if (g_gatts_test_timer_id) {
            BaseType_t ret;
            ret = xTimerDelete(g_gatts_test_timer_id, 0);
            if (ret != pdPASS)
                BT_LOGE("gatts", "g_gatts_test_timer_id delete fail!");
            g_gatts_test_timer_id = NULL;
        }
        return (uint32_t)size;
    } else
        return (uint32_t)size;

    if (!ind_rsp) {
        ind_rsp = (bt_gattc_charc_value_notification_indication_t *) pvPortMalloc(MAX_GATTS_INDICATE_SIZE);
        if (!ind_rsp) {
            BT_LOGE("gatts", "Fail to alloc buf(%d)", MAX_GATTS_INDICATE_SIZE);
            return BT_STATUS_SUCCESS;
        }
        gp_gatts_ind_rsp = ind_rsp;
    }

    ind_rsp->att_req.opcode = BT_ATT_OPCODE_HANDLE_VALUE_INDICATION;
    ind_rsp->att_req.handle = attr_hdl;

    //a special test case to force output 128bytes dummy data to peer due to cli cannot input over 128bytes
    if (isAutoGenMode) {
        pValue = (uint8_t *)ind_rsp->att_req.attribute_value;
        attr_value_len = 128;
        for (i = 0; i < attr_value_len; i++)
            *(pValue + i) = i;
    }
    //else
    //    memcpy((void*)(ind_rsp->att_req.attribute_value), (void *)attr_value, attr_value_len);

    ind_rsp->attribute_value_length = 3 + attr_value_len;
    BT_LOGW("gatts", "set indicate conn_hdl = 0x%x, attr_hdl = 0x%x (len %d)", conn_hdl, attr_hdl, attr_value_len);
    bt_gatts_send_charc_value_notification_indication(conn_hdl, ind_rsp);

    if (isLoopMode) {
        //a special test case to send 128bytes every 3sec
        g_gatts_ind_conn_hdl = conn_hdl;
        g_gatts_test_timer_id = xTimerCreate("GATTS_IND_TIMER",
                                             BTAPP_GATTS_IND_TIMER_PERIOD / portTICK_PERIOD_MS,
                                             pdTRUE, (void *)0, _bt_ut_gatts_ind_test_timer_cb);
        BT_LOGI("gatts", "create timer = %p", g_gatts_test_timer_id);
        xTimerStart(g_gatts_test_timer_id, 1);
    } else {
        vPortFree((void *)gp_gatts_ind_rsp);
        gp_gatts_ind_rsp = NULL;
    }

    return (uint32_t)size;
}

static uint32_t ble_jitter_config_callback(const uint8_t rw, uint16_t handle, void *data, uint16_t size, uint16_t offset)
{
    char *pData = (char *) data;
    bt_hci_cmd_le_set_data_length_t data_length;

    BT_LOGI("gatts", "jitter cfg cb, rw = %d, handle = 0x%x, size = %d", rw, handle, size);
    BT_LOGI("gatts", "jitter cfg cb, data = 0x%x %x", pData[0], pData[1]);

    //ble gap update data length 0200 00F0 0500
    if (strncmp("set_data_length", pData, strlen("set_data_length")) == 0) {
        BT_LOGI("gatts", "set data length to 240(0xF0)");
        data_length.connection_handle = g_connect_handle;
        data_length.tx_octets = 0x00F0;
        data_length.tx_time = 0x0500;
        BT_LOGI("gatts", "update data length handle(%04x) tx_octets(%04x) tx_time(%04x)",
                data_length.connection_handle, data_length.tx_octets, data_length.tx_time);
        bt_gap_le_update_data_length(&data_length);
    }
    return (uint32_t)size;
}

#define BLE_JITTER_SERVICE_UUID        (0x18BB)
#define BLE_JITTER_CHAR_UUID           (0x2AAA)

const bt_uuid_t BLE_JITTER_CHAR_UUID128 = BT_UUID_INIT_WITH_UUID16(BLE_JITTER_CHAR_UUID);

BT_GATTS_NEW_PRIMARY_SERVICE_16(bt_if_jitter_primary_service, BLE_JITTER_SERVICE_UUID);

BT_GATTS_NEW_CHARC_16(bt_if_jitter_char,
                      BT_GATT_CHARC_PROP_WRITE | BT_GATT_CHARC_PROP_INDICATE, 0x001A, BLE_JITTER_CHAR_UUID);

BT_GATTS_NEW_CHARC_VALUE_CALLBACK(bt_if_jitter_char_value, BLE_JITTER_CHAR_UUID128,
                                  BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE, ble_jitter_charc_value_callback);

BT_GATTS_NEW_CLIENT_CHARC_CONFIG(bt_if_jitter_client_config,
                                 BT_GATTS_REC_PERM_READABLE | BT_GATTS_REC_PERM_WRITABLE,
                                 ble_jitter_config_callback);

static const bt_gatts_service_rec_t *bt_if_ble_jitter_service_rec[] = {
    (const bt_gatts_service_rec_t *) &bt_if_jitter_primary_service,
    (const bt_gatts_service_rec_t *) &bt_if_jitter_char,
    (const bt_gatts_service_rec_t *) &bt_if_jitter_char_value,
    (const bt_gatts_service_rec_t *) &bt_if_jitter_client_config
};

const bt_gatts_service_t bt_if_ble_jitter_service = {
    .starting_handle = 0x0018,
    .ending_handle = 0x001B,
    .required_encryption_key_size = 0,
    .records = bt_if_ble_jitter_service_rec
};
#endif /* #ifdef MTK_GATT_JITTER_TEST */



