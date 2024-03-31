/* Copyright Statement:
 *
 * (C) 2022  MediaTek Inc. All rights reserved.
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

/*
 * Description:
 *  Client role: Initiator
 *  Server role: Acceptor and Prvoide SPP SDP Info
 *  Client/Server Task: Process the RX data from Server/Client
 * Note:
 *  Depend on your application, if you just need client role, you can only reference
 *  client sample code and vice versa. If you need both, you can also combine two roles
 *  and use one task to instead.
*/

#include <stdlib.h>
#include <string.h>

#include "btut_spp_main.h"
#include "btut_spp_client.h"
#include "btut_spp_server.h"
#include "bt_callback_manager.h"

#include "FreeRTOS.h"
#include "task.h"
#include "task_def.h"
#include "queue.h"
#if (BTUT_SPP_PERF_TEST_SUPPORT)
#include "event_groups.h"
#endif /* #if (BTUT_SPP_PERF_TEST_SUPPORT) */

#ifdef HAL_GPT_MODULE_ENABLED
#include "hal_gpt.h"
#endif /* #ifdef HAL_GPT_MODULE_ENABLED */


log_create_module(BTUT_SPP, PRINT_LEVEL_INFO);

#define SPP_CLI_HEAD_STR    "spp"

#define BTUT_SPP_CLIENT_TASK_NAME "spp_client_task"
#define BTUT_SPP_CLIENT_TASK_STACK_SIZE (512*4)
#define BTUT_SPP_CLIENT_TASK_PRIO TASK_PRIORITY_NORMAL
#define BTUT_SPP_CLIENT_Q_LEN 5

#define BTUT_SPP_SRV_TASK_NAME "spp_srv_task"
#define BTUT_SPP_SRV_TASK_STACK_SIZE (512*4)
#define BTUT_SPP_SRV_TASK_PRIO TASK_PRIORITY_NORMAL
#define BTUT_SPP_SRV_Q_LEN 5

#if (BTUT_SPP_PERF_TEST_SUPPORT)
#define BTUT_SPP_PERF_TASK_NAME "spp_perf_task"
#define BTUT_SPP_PERF_TASK_STACK_SIZE (512*4)
#define BTUT_SPP_PERF_TASK_PRIO TASK_PRIORITY_NORMAL

#define BTUT_SPP_PERF_ACK_S "ACK PERF"
#define BTUT_SPP_PERF_ACK_S_LEN (strlen(BTUT_SPP_PERF_ACK_S))
#define BTUT_SPP_PERF_START_S "START PERF"
#define BTUT_SPP_PERF_START_S_LEN (strlen(BTUT_SPP_PERF_START_S))
#define BTUT_SPP_PERF_TEST_MAX_LEN (4) //use 4 bytes for len field
#define BTUT_SPP_PERF_HDR_LEN (BTUT_SPP_PERF_START_S_LEN + BTUT_SPP_PERF_TEST_MAX_LEN)
#define BTUT_SPP_PERF_BYTE (0xAB)
#define BTUT_SPP_PERF_MIN_TEST_SIZE (0x400)  //1KB
#ifdef HAL_GPT_MODULE_ENABLED
#define BTUT_SPP_CLK_TYPE  HAL_GPT_CLOCK_SOURCE_32K
#define BTUT_SPP_CLK_HZ    (32768) // 32kHZ => 1 tick = (1/32768) sec
#else /* #ifdef HAL_GPT_MODULE_ENABLED */
#error "Pls open HAL_GPT_MODULE_ENABLED at SPP perf test"
#endif /* #ifdef HAL_GPT_MODULE_ENABLED */
typedef struct {
    uint32_t handle;
    uint32_t exp_len;
    TaskHandle_t task_handle;
    EventGroupHandle_t task_event_group;
} btut_spp_perf_tx_t;
#endif /* #if (BTUT_SPP_PERF_TEST_SUPPORT) */

static TaskHandle_t g_sppClientTaskHdle = NULL;
static TaskHandle_t g_sppSrvTaskHdle = NULL;

static QueueHandle_t g_sppClientQ = NULL;
static QueueHandle_t g_sppServerQ = NULL;

static bool g_is_btut_spp_init = false;

#if (BTUT_SPP_PERF_TEST_SUPPORT)
static btut_spp_perf_tx_t g_spp_perf = {0};
#endif /* #if (BTUT_SPP_PERF_TEST_SUPPORT) */

#define SPP_IO_CMP(_cmd)    (strncmp((_cmd), cmd, strlen(_cmd)) == 0)
#define SPP_IO_GET_CMD_PARAM_BY_HEX(cmd, value, ret_ptr)\
    do { \
        if (cmd != NULL) { \
            ret_ptr = strstr(cmd," "); \
            if (ret_ptr != NULL) { \
                value = (uint32_t)strtoul(ret_ptr + 1, NULL, 16); \
                ret_ptr += 1; \
            } \
        } \
    } while(0)

#define SPP_IO_GET_CMD_PARAM_STR_PTR(cmd, str_ptr, ret_ptr)\
    do { \
        if (cmd != NULL) { \
            ret_ptr = strstr(cmd," "); \
            if (ret_ptr != NULL) { \
                str_ptr = ret_ptr + 1; \
                ret_ptr = str_ptr; \
            } \
        } \
    } while(0)


static void _btut_spp_print_help(void)
{
    printf("ble spp  - BT SPP cmds : Description\n"
           " init\n"
           " connect [addr]\n"
           " disconnect [handle]\n"
           " send [handle] [data]\n"
           " test_send [handle] [pattern(1byte)] [length(0x0001-0xFFFF)]\n"
#if (BTUT_SPP_PERF_TEST_SUPPORT)
           " perf_start [handle] [length(0x0400-0xFFFFFFFF)]\n"
           " perf_delete\n"
#endif /* #if (BTUT_SPP_PERF_TEST_SUPPORT) */
           " show info: show SPP connected dev info\n"
          );
}

static bool _btut_spp_str_to_addr(uint8_t *addr, const char *str)
{
    unsigned int i, value;
    int using_long_format = 0;
    int using_hex_sign = 0;
    int icn = 0;

    if (str[2] == ':' || str[2] == '-') {
        using_long_format = 1;
    }

    if (str[1] == 'x') {
        using_hex_sign = 2;
    }

    /*
     * For example :
     * 11:22:33:44:55:66             -> OK
     * 0x11:0x22:0x33:0x44:0x55:0x66 -> OK
     * 112233445566                  -> OK
     * 0x11:22:33:44:55:66           -> NG
     */
    if ((size_t)(6 * (using_hex_sign + 2 + using_long_format) - using_long_format) > strlen(str)) {
        LOG_E(BTUT_SPP, "Expect %d  Actual %d",
              (size_t)(6 * (using_hex_sign + 2 + using_long_format) - using_long_format), strlen(str));
        return false;
    }

    for (i = 0; i < 6; i++) {
        icn = sscanf(str + using_hex_sign + i * (2 + using_long_format), "%02x", &value);
        if (icn < 0)
            return false;
        addr[5 - i] = (uint8_t) value;
    }

    return true;
}

#if (BTUT_SPP_PERF_TEST_SUPPORT)
static uint32_t _btut_spp_get_hal_gpt_count(void)
{
    uint32_t count = 0;
    int32_t ret = 0;
#ifdef HAL_GPT_MODULE_ENABLED
    ret = (int32_t)hal_gpt_get_free_run_count(BTUT_SPP_CLK_TYPE, &count);
#endif /* #ifdef HAL_GPT_MODULE_ENABLED */
    if (ret < 0) {
        return 0;
    } else {
        return count;
    }
}

static double _btut_spp_get_hal_gpt_duratioin_ms(uint32_t start_count, uint32_t end_count)
{
    uint32_t duration_count = 0;
    if (end_count > start_count)
        duration_count = end_count - start_count;
    else
        duration_count = (0xffffffff - (start_count - end_count)) + 1;

    return (duration_count * 1000.0) / BTUT_SPP_CLK_HZ;
}

static btut_spp_role_t _btut_spp_get_role(uint32_t handle)
{
    return btut_spp_is_client(handle) ? BTUT_SPP_ROLE_CLIENT :
           btut_spp_is_server(handle) ? BTUT_SPP_ROLE_SERVER : BTUT_SPP_ROLE_NONE;
}

static char *_btut_spp_perf_search(const char *s1, const char *s2, uint32_t cnt)
{
    const size_t len = strlen(s2);
    while (*s1 && cnt > 0) {
        if (!memcmp(s1, s2, len)) {
            return (char *)s1;
        }
        ++s1;
        cnt--;
    }
    return NULL;
}

static bt_status_t _btut_spp_perf_send_ack(uint32_t handle)
{
    bt_status_t status = BT_STATUS_FAIL;
    btut_spp_tx_fptr tx_fptr = (BTUT_SPP_ROLE_CLIENT == _btut_spp_get_role(handle)) ?
                               btut_spp_client_send_data : btut_spp_server_send_data;

    status = tx_fptr(handle, (uint8_t *)BTUT_SPP_PERF_ACK_S, BTUT_SPP_PERF_ACK_S_LEN);
    if (BT_STATUS_SUCCESS == status) {
        LOG_I(BTUT_SPP, "perf send ack");
    } else {
        LOG_E(BTUT_SPP, "perf send ack fail(sts:%d)", status);
    }
    return status;
}

static void _btut_spp_perf_show_result(uint32_t handle, uint32_t rx_exp_len,
                                       uint32_t start_count, uint32_t end_count)
{
    double dr_KBS = 0.0;
    double interval_ms = _btut_spp_get_hal_gpt_duratioin_ms(start_count, end_count);

    dr_KBS = (rx_exp_len / interval_ms) * (1000.0 / 1024);

    LOG_I(BTUT_SPP, "perf result ==> hdl:0x%x, exp:%d, interval:%.3f(ms), %.3f KB/s",
          handle, rx_exp_len, interval_ms, dr_KBS);
}

static bool _btut_spp_perf_check(uint32_t handle, uint8_t *p_rx_buf, uint32_t rx_len)
{
    static uint32_t rx_exp_len = 0;
    static uint32_t rx_cur_len = 0;
    static uint32_t rx_start_count = 0;
    const char *p_rx_str = (const char *)p_rx_buf;
    uint32_t rx_end_count = 0;
    bool is_perf_running = FALSE;

    //perf test master
    if (g_spp_perf.task_handle && _btut_spp_perf_search(p_rx_str, BTUT_SPP_PERF_ACK_S, rx_len)) {
        if (g_spp_perf.task_event_group)
            xEventGroupSetBits(g_spp_perf.task_event_group, BTUT_SPP_PERF_TASK_EVENT_RX_ACK);
        is_perf_running = TRUE;
    }
    //perf test slave
    else {
        if (_btut_spp_perf_search(p_rx_str, BTUT_SPP_PERF_START_S, rx_len)) {
            rx_start_count = _btut_spp_get_hal_gpt_count();
            rx_exp_len = *((uint32_t *)(p_rx_str + BTUT_SPP_PERF_START_S_LEN));
            rx_cur_len = rx_len;
            is_perf_running = TRUE;
            LOG_I(BTUT_SPP, "perf rx start(exp_len:%d)", rx_exp_len);
            if (rx_exp_len == rx_cur_len) {
                LOG_E(BTUT_SPP, "perf rx stop(wrong len)"); //only send 1 packet is not enough to get correct result.
                rx_cur_len = 0;
                rx_exp_len = 0;
                rx_start_count = 0;
            }
        } else if (rx_exp_len) {
            rx_cur_len += rx_len;
            is_perf_running = TRUE;
            if (rx_exp_len == rx_cur_len) {
                LOG_I(BTUT_SPP, "perf rx end(total: %d)", rx_cur_len);
                rx_end_count = _btut_spp_get_hal_gpt_count();
                _btut_spp_perf_send_ack(handle);
                _btut_spp_perf_show_result(handle, rx_exp_len, rx_start_count, rx_end_count);
                rx_cur_len = 0;
                rx_exp_len = 0;
                rx_start_count = 0;
            }
        }
    }
    return is_perf_running;
}

static void _btut_spp_perf_task(void *arg)
{
    EventBits_t bits = 0;
    bool stop_task = FALSE;
    bt_status_t status = BT_STATUS_FAIL;
    uint8_t *p_buf = NULL;
    uint32_t cur_len = 0;
    uint32_t end_count = 0;
    uint32_t exp_len = g_spp_perf.exp_len;
    uint32_t send_len = 0;
    uint32_t spp_tx_handle =  g_spp_perf.handle;
    uint32_t start_count = 0;
    btut_spp_role_t role = _btut_spp_get_role(spp_tx_handle);
    uint16_t max_len = (BTUT_SPP_ROLE_CLIENT == role) ?
                       btut_spp_client_get_max_packet_len(spp_tx_handle) :
                       btut_spp_server_get_max_packet_len(spp_tx_handle);
    btut_spp_tx_fptr tx_fptr = (BTUT_SPP_ROLE_CLIENT == role) ?
                               btut_spp_client_send_data : btut_spp_server_send_data;

    if (BTUT_SPP_ROLE_NONE == _btut_spp_get_role(spp_tx_handle)) {
        LOG_E(BTUT_SPP, "perf tx fail(invalid handle)");
        goto spp_perf_task_error_exit;
    }

    if (BTUT_SPP_PERF_HDR_LEN > max_len) {
        LOG_E(BTUT_SPP, "perf tx fail(maxlen:%d)", max_len);
        goto spp_perf_task_error_exit;
    }

    if (!(p_buf = (uint8_t *)pvPortMalloc(max_len))) {
        LOG_E(BTUT_SPP, "perf tx fail(oom,exp:%d)", max_len);
        goto spp_perf_task_error_exit;
    }
    memcpy(p_buf, BTUT_SPP_PERF_START_S, BTUT_SPP_PERF_START_S_LEN);
    memcpy(p_buf + BTUT_SPP_PERF_START_S_LEN, &exp_len, BTUT_SPP_PERF_TEST_MAX_LEN);

    start_count = _btut_spp_get_hal_gpt_count();
    xEventGroupSetBits(g_spp_perf.task_event_group, BTUT_SPP_PERF_TASK_EVENT_TX_START);

    do {
        bits = xEventGroupWaitBits(g_spp_perf.task_event_group,
                                   BTUT_SPP_PERF_TASK_EVENT_ALL, pdTRUE, pdFALSE,
                                   portMAX_DELAY);

        if (bits & (BTUT_SPP_PERF_TASK_EVENT_TX_START | BTUT_SPP_PERF_TASK_EVENT_TX_CONTINUE)) {

            while (cur_len != exp_len) {
                send_len = (exp_len - cur_len) > max_len ? max_len : (exp_len - cur_len);
                if (0 == cur_len) {
                    memset(p_buf + BTUT_SPP_PERF_HDR_LEN, BTUT_SPP_PERF_BYTE, send_len - BTUT_SPP_PERF_HDR_LEN);
                } else {
                    memset(p_buf, BTUT_SPP_PERF_BYTE, send_len);
                }
                status = tx_fptr(spp_tx_handle, (uint8_t *)(p_buf), send_len);
                if (BT_STATUS_SUCCESS == status) {
                    cur_len += send_len;
                } else if (BT_STATUS_SPP_TX_NOT_AVAILABLE == status) {
                    break;
                } else {
                    LOG_E(BTUT_SPP, "perf tx fail(exp:%d,sent:%d)", exp_len, cur_len);
                    stop_task = TRUE;
                }
            }
        }

        if (bits & BTUT_SPP_PERF_TASK_EVENT_RX_ACK) {
            if (exp_len == cur_len) {
                LOG_I(BTUT_SPP, "perf tx done(receive ACK)");
                end_count = _btut_spp_get_hal_gpt_count();
                _btut_spp_perf_show_result(spp_tx_handle, exp_len, start_count, end_count);
            } else {
                LOG_E(BTUT_SPP, "perf tx fail(exp!=cur)");
            }
            stop_task = TRUE;
        }
    } while (!stop_task);

spp_perf_task_error_exit:
    if (p_buf)
        vPortFree(p_buf);
    if (g_spp_perf.task_event_group)
        vEventGroupDelete(g_spp_perf.task_event_group);
    memset(&g_spp_perf, 0x00, sizeof(btut_spp_perf_tx_t));
    vTaskDelete(NULL);
}

static void _btut_spp_perf_task_deinit(void)
{
    if (g_spp_perf.task_handle)
        vTaskDelete(g_spp_perf.task_handle);
    if (g_spp_perf.task_event_group)
        vEventGroupDelete(g_spp_perf.task_event_group);
    memset(&g_spp_perf, 0x00, sizeof(btut_spp_perf_tx_t));
}

static bool _btut_spp_perf_task_init(uint32_t handle, uint32_t test_len)
{
    if (g_spp_perf.task_handle) {
        LOG_E(BTUT_SPP, "perf fail(task is running)");
        return FALSE;
    }

    if (BTUT_SPP_ROLE_NONE == _btut_spp_get_role(handle)) {
        LOG_E(BTUT_SPP, "perf fail(invalid handle)");
        return FALSE;
    }

    if (!(g_spp_perf.task_event_group = xEventGroupCreate())) {
        LOG_E(BTUT_SPP, "perf fail(can't create event group)");
        return FALSE;
    }

    g_spp_perf.handle = handle;
    g_spp_perf.exp_len = test_len;

    if (pdPASS != xTaskCreate(_btut_spp_perf_task, BTUT_SPP_PERF_TASK_NAME,
                              BTUT_SPP_PERF_TASK_STACK_SIZE / sizeof(portSTACK_TYPE),
                              NULL,
                              TASK_PRIORITY_SOFT_REALTIME,
                              &g_spp_perf.task_handle)) {
        LOG_E(BTUT_SPP, "perf fail(can't create task)");
        _btut_spp_perf_task_deinit();
        return FALSE;
    }
    LOG_I(BTUT_SPP, "perf task init success!");
    return TRUE;
}
#endif /* #if (BTUT_SPP_PERF_TEST_SUPPORT) */

void _btut_spp_event_callback(bt_msg_type_t msg, bt_status_t status, void *param)
{
    LOG_I(BTUT_SPP, "msg:0x%x, status:0x%x", msg, status);
    btut_spp_msg_t message;
    uint32_t handle = BT_SPP_INVALID_HANDLE;

    switch (msg) {
        case BT_SPP_CONNECT_IND: { /*0x34000000*/
                bt_spp_connect_ind_t *conn_ind = (bt_spp_connect_ind_t *)param;
                memcpy(&message.param.conn_ind, param, sizeof(bt_spp_connect_ind_t));
                handle = conn_ind->handle;
            }
            break;

        case BT_SPP_CONNECT_CNF: { /*0x34000001*/
                bt_spp_connect_cnf_t *conn_cnf = (bt_spp_connect_cnf_t *)param;
                memcpy(&message.param.conn_cnf, param, sizeof(bt_spp_connect_cnf_t));
                handle = conn_cnf->handle;
            }
            break;

        case BT_SPP_DISCONNECT_IND: { /*0x34000002*/
                bt_spp_disconnect_ind_t *dis_ind = (bt_spp_disconnect_ind_t *)param;
                memcpy(&message.param.dis_ind, param, sizeof(bt_spp_disconnect_ind_t));
                handle = dis_ind->handle;
            }
            break;

        case BT_SPP_DATA_RECEIVED_IND: { /*0x34000003*/
                bt_spp_data_received_ind_t *data_ind = (bt_spp_data_received_ind_t *)param;
                uint16_t queued_data;
                memcpy(&message.param.data_ind, param, sizeof(bt_spp_data_received_ind_t));
                handle = data_ind->handle;

#if (BTUT_SPP_PERF_TEST_SUPPORT)
                if (_btut_spp_perf_check(handle, data_ind->packet, data_ind->packet_length)) {
                    return;
                }
#endif /* #if (BTUT_SPP_PERF_TEST_SUPPORT) */

                //Copy packet becuase the packet buffer will be released after this function return.
                if (btut_spp_is_client(handle)) {
                    queued_data = btut_spp_client_feed_rx_data(handle, data_ind->packet, data_ind->packet_length);
                    if (queued_data < data_ind->packet_length) {
                        LOG_E(BTUT_SPP, "data dropped!! U should enlarge ur client buf");
                    }
                } else {
                    queued_data = btut_spp_server_feed_rx_data(handle, data_ind->packet, data_ind->packet_length);
                    if (queued_data < data_ind->packet_length) {
                        LOG_E(BTUT_SPP, "data dropped!! U should enlarge ur server buf");
                    }
                }
            }
            break;

        case BT_SPP_READY_TO_SEND_IND: { /*0x34000004*/
                bt_spp_ready_to_send_ind_t *send_ind = (bt_spp_ready_to_send_ind_t *)param;
                memcpy(&message.param.send_ind, param, sizeof(bt_spp_ready_to_send_ind_t));
                handle = send_ind->handle;
            }
            break;

        default:
            break;
    }


    message.msg = msg;
    message.status = status;
    if (btut_spp_is_client(handle)) {
        if (g_sppClientQ)
            xQueueSend(g_sppClientQ, (void *)&message, 0);
    } else {
        if (g_sppServerQ)
            xQueueSend(g_sppServerQ, (void *)&message, 0);
    }

}

//server ready to send indicator
static void _btut_spp_srv_ready_to_send_ind(uint32_t handle)
{
    LOG_E(BTUT_SPP, "get srv ready to send (hdl =0x%x)", handle);
#if (BTUT_SPP_PERF_TEST_SUPPORT)
    if (g_spp_perf.task_handle && g_spp_perf.task_event_group) {
        xEventGroupSetBits(g_spp_perf.task_event_group, BTUT_SPP_PERF_TASK_EVENT_TX_CONTINUE);
    }
#endif /* #if (BTUT_SPP_PERF_TEST_SUPPORT) */
}

//client ready to send indicator
static void _btut_spp_clt_ready_to_send_ind(uint32_t handle)
{
    LOG_E(BTUT_SPP, "get clt ready to send (hdl =0x%x)", handle);
#if (BTUT_SPP_PERF_TEST_SUPPORT)
    if (g_spp_perf.task_handle && g_spp_perf.task_event_group) {
        xEventGroupSetBits(g_spp_perf.task_event_group, BTUT_SPP_PERF_TASK_EVENT_TX_CONTINUE);
    }
#endif /* #if (BTUT_SPP_PERF_TEST_SUPPORT) */
}

static void _btut_spp_client_task(void *arg)
{
    btut_spp_msg_t message;

    g_sppClientQ = xQueueCreate(BTUT_SPP_CLIENT_Q_LEN, sizeof(btut_spp_msg_t));
    if (g_sppClientQ == NULL) {
        LOG_E(BTUT_SPP, "create client queue fail!");
        return;
    }

    while (1) {
        if (xQueueReceive(g_sppClientQ, (void *)&message, portMAX_DELAY)) {
            btut_spp_client_event_handler(&message);
        }
    }
}

static void _btut_spp_server_task(void *arg)
{
    btut_spp_msg_t message;
    g_sppServerQ = xQueueCreate(BTUT_SPP_SRV_Q_LEN, sizeof(btut_spp_msg_t));
    if (g_sppServerQ == NULL) {
        LOG_E(BTUT_SPP, "create server queue fail!");
        return;
    }

    while (1) {
        if (xQueueReceive(g_sppServerQ, (void *)&message, portMAX_DELAY)) {
            btut_spp_server_event_handler(&message);
        }
    }
}


/**
 * @brief     This function is for spp example project main init function.
 * @return    void.
 */
void btut_spp_init(void)
{
    if (g_is_btut_spp_init) {
        LOG_E(BTUT_SPP, "already initialized");
        return;
    }

    //LOG_I(btut_spp,"create spp app task!");
    if (pdPASS != xTaskCreate(_btut_spp_client_task, BTUT_SPP_CLIENT_TASK_NAME,
                              BTUT_SPP_CLIENT_TASK_STACK_SIZE / sizeof(portSTACK_TYPE),
                              NULL,
                              BTUT_SPP_CLIENT_TASK_PRIO,
                              &g_sppClientTaskHdle)) {
        LOG_E(BTUT_SPP, "Create client task fail!");
    }
    if (pdPASS != xTaskCreate(_btut_spp_server_task, BTUT_SPP_SRV_TASK_NAME,
                              BTUT_SPP_SRV_TASK_STACK_SIZE / sizeof(portSTACK_TYPE),
                              NULL,
                              BTUT_SPP_SRV_TASK_PRIO,
                              &g_sppSrvTaskHdle)) {
        LOG_E(BTUT_SPP, "Create server task fail!");
    }

    btut_spp_client_init(_btut_spp_clt_ready_to_send_ind);
    btut_spp_server_init(_btut_spp_srv_ready_to_send_ind);


    bt_callback_manager_register_callback(bt_callback_type_app_event,
                                          //(uint32_t)(MODULE_MASK_GAP | MODULE_MASK_SYSTEM | MODULE_MASK_SPP),
                                          (uint32_t)(MODULE_MASK_SPP),
                                          (void *)_btut_spp_event_callback);

    g_is_btut_spp_init = true;
}

void btut_spp_deinit(void)
{
    if (!g_is_btut_spp_init) {
        LOG_E(BTUT_SPP, "Not initialized");
        return;
    }

    bt_callback_manager_deregister_callback(bt_callback_type_app_event, (void *)_btut_spp_event_callback);

    if (g_sppClientTaskHdle) {
        vTaskDelete(g_sppClientTaskHdle);
        g_sppClientTaskHdle = NULL;
    }

    if (g_sppSrvTaskHdle) {
        vTaskDelete(g_sppSrvTaskHdle);
        g_sppSrvTaskHdle = NULL;
    }

    if (g_sppClientQ) {
        vQueueDelete(g_sppClientQ);
        g_sppClientQ = NULL;
    }

    if (g_sppServerQ) {
        vQueueDelete(g_sppServerQ);
        g_sppServerQ = NULL;
    }

    btut_spp_client_deinit();
    btut_spp_server_deinit();

    g_is_btut_spp_init = false;

}

bt_status_t btut_spp_io_callback(void *input, void *output)
{
    const char *cmd = input;
    char *next_param = NULL;

    cmd += strlen(SPP_CLI_HEAD_STR) + 1; //+1 to shift space

    if (SPP_IO_CMP("?")) {
        _btut_spp_print_help();
    } else if (SPP_IO_CMP("init")) {
        btut_spp_init();
    } else if (SPP_IO_CMP("connect")) { //Initiator will be client role
        /* Usage: spp connect [address].
           Note:  N/A. */
        const uint8_t spp_uuid[16] = {BT_SPP_STANDARD_UUID};
        bt_bd_addr_t addr;
        if (!_btut_spp_str_to_addr(&addr[0], cmd + sizeof("connect")))
            return BT_STATUS_FAIL;
        LOG_I(BTUT_SPP, "connect: %02x:%02x:%02x:%02x:%02x:%02x",
              addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        btut_spp_client_connect_req(&addr, spp_uuid);
    } else if (SPP_IO_CMP("disconnect")) {
        /* Usage: spp disconnect [handle(hex)]
         */
        uint32_t handle = 0;

        SPP_IO_GET_CMD_PARAM_BY_HEX(cmd, handle, next_param);
        LOG_I(BTUT_SPP, "disconnect (input hdl = 0x%x)", handle);

        // Do basic sanity check on handle id
        if (btut_spp_is_client(handle) ||
            btut_spp_is_server(handle))
            bt_spp_disconnect(handle);
        else
            LOG_W(BTUT_SPP, "input hdl = 0x%x is not existed!!", handle);
    } else if (SPP_IO_CMP("send")) {
        /* Usage: spp send [handle(hex)] [data]
           Note:  Use default data if [data] == null */
        uint32_t handle = 0;
        uint16_t exp_len = 0;
        char *p_str = NULL;
        bt_status_t status = BT_STATUS_FAIL;

        SPP_IO_GET_CMD_PARAM_BY_HEX(cmd, handle, next_param);
        SPP_IO_GET_CMD_PARAM_STR_PTR(next_param, p_str, next_param);

        if (btut_spp_is_client(handle)) {
            const char *pHello = "Hello I'm SPP Client";
            if (p_str == NULL)
                p_str = (char *)pHello;
            exp_len = strlen(p_str);
            status = btut_spp_client_send_data(handle, (uint8_t *)p_str, exp_len);
        } else {
            const char *pHello = "Hello I'm SPP Server";
            if (p_str == NULL)
                p_str = (char *)pHello;
            exp_len = strlen(p_str);
            status = btut_spp_server_send_data(handle, (uint8_t *)p_str, exp_len);
        }

        if (status != BT_STATUS_SUCCESS)
            LOG_E(BTUT_SPP, "send Fail 0x%x (hdl = 0x%x, len = %d)", status, handle, exp_len);
        else
            LOG_E(BTUT_SPP, "send OK (hdl = 0x%x, len = %d)", handle, exp_len);

    } else if (SPP_IO_CMP("test_send")) {
        /* Usage: spp test_send [handle(hex)] [pattern(1byte)] [len(0x0001-0xFFFF)] */
        uint16_t exp_len = 0;
        uint16_t max_pkt_l = 0;
        uint16_t sent_len = 0;
        uint32_t handle = 0;
        char *p_pattern = NULL;
        char *p_send_data = NULL;
        bt_status_t status = BT_STATUS_FAIL;
        bt_status_t (*send_data_fptr)(uint32_t handle, uint8_t *buf, uint16_t len) = NULL;

        LOG_I(BTUT_SPP, "test_send start");

        // Input Data
        SPP_IO_GET_CMD_PARAM_BY_HEX(cmd, handle, next_param);
        SPP_IO_GET_CMD_PARAM_STR_PTR(next_param, p_pattern, next_param);
        SPP_IO_GET_CMD_PARAM_BY_HEX(next_param, exp_len, next_param);

        if (0 == handle || !p_pattern || 0 == exp_len) {
            LOG_E(BTUT_SPP, "test_send fail(invalid input)");
            return BT_STATUS_FAIL;
        }

        // Get Server/Client max packet length and tx function pointer
        max_pkt_l = btut_spp_is_client(handle) ? btut_spp_client_get_max_packet_len(handle) :
                    btut_spp_server_get_max_packet_len(handle);
        send_data_fptr = btut_spp_is_client(handle) ? btut_spp_client_send_data :
                         btut_spp_is_server(handle) ? btut_spp_server_send_data : NULL;

        if (0 == max_pkt_l || !send_data_fptr) {
            LOG_E(BTUT_SPP, "test_send fail(wrong handle)");
            return BT_STATUS_FAIL;
        }

        LOG_I(BTUT_SPP, "hdl:0x%x, pattern:%c, exp_len:%d, is_Client:%d",
              handle, *p_pattern, exp_len, btut_spp_is_client(handle));

        // Prepare Test Data
        if (!(p_send_data = (char *)pvPortMalloc((exp_len)))) {
            LOG_E(BTUT_SPP, "test_send fail(oom,exp:%d)", exp_len);
            return BT_STATUS_FAIL;
        }
        memset(p_send_data, *p_pattern, exp_len);

        // Send Test Data
        while (exp_len > sent_len) {
            uint16_t temp_len = (exp_len - sent_len) > max_pkt_l ? max_pkt_l : (exp_len - sent_len);
            status = send_data_fptr(handle, (uint8_t *)(p_send_data) + sent_len, temp_len);
            if (BT_STATUS_SUCCESS == status) {
                sent_len += temp_len;
            } else {
                break;
            }
        }

        // Check Result
        if (sent_len != exp_len)
            LOG_E(BTUT_SPP, "test_send fail(sent:%d != exp:%d) sts:0x%x", sent_len, exp_len, status);
        else
            LOG_I(BTUT_SPP, "test_send success(len = %d)", sent_len);
        vPortFree(p_send_data);
    } else if (SPP_IO_CMP("show info")) {
        LOG_I(BTUT_SPP, "=======start========");
        btut_spp_client_show_info();
        btut_spp_server_show_info();
        LOG_I(BTUT_SPP, "=======end========");
    }
#if (BTUT_SPP_PERF_TEST_SUPPORT)
    else if (SPP_IO_CMP("perf_start")) {
        /* Usage: spp perf [handle(hex)] [len(0x400-0xFFFFFFFF)] min:1KB */
        uint32_t length = 0;
        uint32_t handle = 0;

        LOG_I(BTUT_SPP, "perf start");

        SPP_IO_GET_CMD_PARAM_BY_HEX(cmd, handle, next_param);
        SPP_IO_GET_CMD_PARAM_BY_HEX(next_param, length, next_param);
        if (0 == handle || BTUT_SPP_PERF_MIN_TEST_SIZE > length) {
            LOG_E(BTUT_SPP, "perf invalid input(hdl:0x%x len:%d)", handle, length);
            return BT_STATUS_FAIL;
        }

        if (!_btut_spp_perf_task_init(handle, length)) {
            return BT_STATUS_FAIL;
        }
    } else if (SPP_IO_CMP("perf_delete")) {
        LOG_I(BTUT_SPP, "perf task force deinit");
        _btut_spp_perf_task_deinit();
    }
#endif /* #if (BTUT_SPP_PERF_TEST_SUPPORT) */
    else {
        LOG_I(BTUT_SPP, "not a SPP cli!!");
    }
    return BT_STATUS_SUCCESS;
}


