/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include "FreeRTOS.h"
#include "autobt.h"
#include "bttool_hw_test.h"

static UINT8 g_bttool_log_lvl = BTTOOL_LOG_LVL_I;
TaskHandle_t bt_tool_task_hdl = NULL;
static BT_HW_TEST_CMD_T curr_test_cmd;
BOOL waiting_stop_cmd = FALSE;
xSemaphoreHandle curr_cmd_mutex = NULL;

char nsrx_result[256] = {0};


void bt_tool_set_log_lvl(UINT8 lvl)
{
    if (lvl > BTTOOL_LOG_LVL_V)
        g_bttool_log_lvl = BTTOOL_LOG_LVL_V;
    else
        g_bttool_log_lvl = lvl;
}

UINT8 bt_tool_get_log_lvl()
{
    return g_bttool_log_lvl;
}

void bt_tool_dump_buffer(char *label, const unsigned char *buf, unsigned int length)
{
    char string[256] = {0};
    unsigned char temp_buf[16] = {0};
    int i = 0;
    int cycle = length/16 + 1;

    if (label == NULL || buf == NULL)
        return;

    for (i = 0; i < cycle; i++) {
        memset(temp_buf, 0, 16);
        memset(string, 0, 128);
        unsigned char cp_len = (i+1)*16 < length ? 16 : (length - i*16);
        memcpy(temp_buf, &buf[i*16], cp_len);
        snprintf(string, 128, "%s: [%d][%d~%d] %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", label, length, i*16, i*16+cp_len-1, \
            temp_buf[0], temp_buf[1], temp_buf[2], temp_buf[3], temp_buf[4], temp_buf[5], temp_buf[6], temp_buf[7], \
            temp_buf[8], temp_buf[9], temp_buf[10], temp_buf[11], temp_buf[12], temp_buf[13], temp_buf[14], temp_buf[15]);
        if (cp_len < 16) {
            unsigned int len = strlen(string);
            string[len - 3*(16 - cp_len)] = '\0';
        }
        BTTOOL_LOGD("%s", string);
        if (i*16+cp_len >= length)
            break;
    }
}

static void bt_tool_data_ready(void) //rx data ready callback
{
    BTTOOL_LOGD("%s\n", __func__);
    if (bt_tool_task_hdl)
        xTaskNotifyGive(bt_tool_task_hdl);
}

static int bt_tool_test_start(void)
{
    BOOL result = FALSE;

    BTTOOL_LOGI("%s, curr_cmd: %d", __func__, curr_test_cmd.cmd);

    if (HW_TEST_BT_init() < 0)
        goto cmd_start_fail;

    bt_driver_register_event_cb(bt_tool_data_ready, FALSE);

    switch (curr_test_cmd.cmd) {
        case BT_HW_TEST_CMD_TX:
            result = HW_TEST_BT_TxOnlyTest_start(&curr_test_cmd);
            break;
        case BT_HW_TEST_CMD_NSRX:
            result = HW_TEST_BT_NonSignalRx_start(&curr_test_cmd);
            if (result) {
                BTTOOL_LOGI("reset nsrx result info");
                memset(nsrx_result, 0, sizeof(nsrx_result));
                snprintf(nsrx_result, sizeof(nsrx_result), "%s", "No nsrx test result, please do the test first!!!");
            }
            break;
         case BT_HW_TEST_CMD_TEST_MODE:
            result = HW_TEST_BT_TestMode_enter(&curr_test_cmd);
            if (result)
            {
                BTTOOL_LOGI("---Test Start Success---");
                waiting_stop_cmd = TRUE;
                //Receive packet syncly until test stop cmd is issued
                result = HW_TEST_BT_TestMode_Rx_loop();
                if (result)
                    goto success_end;
            }
            break;
        case BT_HW_TEST_CMD_INQUIRY:
            result = HW_TEST_BT_Inquiry();  //sync test procedure
            if (result) {
                BTTOOL_LOGI("---Test Start Success---");
                //Receive and parse inquiry response syncly
                result = HW_TEST_BT_parse_inq_resp();
                if (result)
                    goto success_end;
            }
            break;
        case BT_HW_TEST_CMD_BLE_TX:
            result = HW_TEST_BT_LE_Tx_start(&curr_test_cmd);
            break;
        case BT_HW_TEST_CMD_BLE_RX:
            result = HW_TEST_BT_LE_Rx_start(&curr_test_cmd);
            break;
        //case BT_HW_TEST_CMD_STOP:
        //    break;
        case BT_HW_TEST_CMD_RELAYER:
            break;
        case BT_HW_TEST_CMD_GET_RX:
            //shall read the result from file  TBD
            BTTOOL_LOGI("%s", nsrx_result);
            goto success_end;
        case BT_HW_TEST_CMD_TX_PWR_OFFSET:
            break;
        default:
            result = FALSE;
            break;
    }
    if (result == FALSE) {
        BTTOOL_LOGE("---Test Start Fail!!!---");
        goto cmd_start_fail;
    } else {
        BTTOOL_LOGI("---Test Start Success---");
        waiting_stop_cmd = TRUE;
    }
    return 0;

cmd_start_fail:
    HW_TEST_BT_deinit();

drv_open_fail:
    BTTOOL_LOGE("%s FAIL", __func__);
    bt_driver_register_event_cb(NULL, TRUE);
    memset(&curr_test_cmd, 0, sizeof(BT_HW_TEST_CMD_T));
    return -1;

success_end:
    BTTOOL_LOGI("---Test End success---");
    HW_TEST_BT_deinit();
    bt_driver_register_event_cb(NULL, TRUE);
    memset(&curr_test_cmd, 0, sizeof(BT_HW_TEST_CMD_T));
    return 0;
}


static int bt_tool_test_stop(void)
{
    BOOL result = FALSE;

    BTTOOL_LOGI("%s, curr_cmd: %d, waiting_stop: %s", \
        __func__, curr_test_cmd.cmd, waiting_stop_cmd ? "Yes" : "No");

    //if (waiting_stop_cmd == FALSE)
    //    goto test_stop_fail;

    bt_driver_register_event_cb(bt_tool_data_ready, FALSE);

    switch (curr_test_cmd.cmd) {
        case BT_HW_TEST_CMD_TX:
            result = HW_TEST_BT_TxOnlyTest_end();
            break;
        case BT_HW_TEST_CMD_NSRX:
        {
            unsigned int pkt_count = 0;
            unsigned int byte_count = 0;
            float PER, BER;
            result = HW_TEST_BT_NonSignalRx_end(&pkt_count, &PER, &byte_count, &BER);
            if (TRUE == result) {
                char *p_str = nsrx_result;
                /*BTTOOL_LOGI("Total received packet: %d", pkt_count);
                BTTOOL_LOGI("Packet Error Rate: %f%%", PER);
                BTTOOL_LOGI("Total received payload byte: %d", byte_count);
                BTTOOL_LOGI("Bit Error Rate: %f%%", BER);*/
                //shall store the result info into a file for the getrx cmd
                memset(nsrx_result, 0, sizeof(nsrx_result));
                p_str += sprintf(p_str, "Total received packet: %d\n", pkt_count);
                p_str += sprintf(p_str, "Packet Error Rate: %f%%\n", PER);
                p_str += sprintf(p_str, "Total received payload byte: %d\n", byte_count);
                p_str += sprintf(p_str, "Bit Error Rate: %f%%\n", BER);
                BTTOOL_LOGI("%s", nsrx_result);
            }
            break;
        }
        case BT_HW_TEST_CMD_TEST_MODE:
            result = HW_TEST_BT_TestMode_exit();
            break;
        case BT_HW_TEST_CMD_BLE_TX:
            result = HW_TEST_BT_LE_Tx_end();
            break;
        case BT_HW_TEST_CMD_BLE_RX:
        {
            unsigned short pkt_count = 0;
            result = HW_TEST_BT_LE_Rx_end(&pkt_count);
            if (TRUE == result) {
                BTTOOL_LOGI("Total received packet: %d\n", pkt_count);
            }
            break;
        }
        case BT_HW_TEST_CMD_RELAYER:
            break;
        case BT_HW_TEST_CMD_INQUIRY:
        case BT_HW_TEST_CMD_STOP:
        case BT_HW_TEST_CMD_GET_RX:
        case BT_HW_TEST_CMD_TX_PWR_OFFSET:
        default:
            result = FALSE;
            break;
    }
    if (result == FALSE) {
        BTTOOL_LOGE("---Test Stop Fail---");
    } else {
        BTTOOL_LOGI("---Test Stop success---");
    }

test_stop_fail:
    HW_TEST_BT_deinit();
    bt_driver_register_event_cb(NULL, TRUE);
    memset(&curr_test_cmd, 0, sizeof(BT_HW_TEST_CMD_T));
    return 0;
}

static void bt_tool_task(void * arg)
{
    int res = 0;
    BTTOOL_LOGI("%s is running", __func__);

    /* main loop, handle cmd and receive data from driver */
    do {
        if ((curr_test_cmd.cmd > BT_HW_TEST_CMD_NONE)
            && (curr_test_cmd.cmd < BT_HW_TEST_CMD_MAX)) {
            res = bt_tool_test_start();
            if (res == 0) { //success
                if ((curr_test_cmd.cmd == BT_HW_TEST_CMD_TX)
                    || (curr_test_cmd.cmd == BT_HW_TEST_CMD_NSRX)
                    || (curr_test_cmd.cmd == BT_HW_TEST_CMD_BLE_TX)
                    || (curr_test_cmd.cmd == BT_HW_TEST_CMD_BLE_RX))
                {
                    int timeout_s = curr_test_cmd.test_duration_s;
                    //Async test cmd, shall wait for stop command or timeout
                    if (timeout_s > 0) {
                        BTTOOL_LOGW("Exec [autobt stop] to stop test or wait for %ds timeout...", timeout_s);
                        while (waiting_stop_cmd && (timeout_s > 0)) {
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            BTTOOL_LOGW("test cmd %d remain %ds...", curr_test_cmd.cmd, --timeout_s);
                        }
                    } else {
                        BTTOOL_LOGW("Exec [autobt stop] to stop test...");
                        while (waiting_stop_cmd) {
                            vTaskDelay(pdMS_TO_TICKS(1000));
                        }
                    }
                    waiting_stop_cmd = FALSE;
                    BTTOOL_LOGW("Go to stop test...");
                    bt_tool_test_stop();
                }
            }
        }
        else {
            //wait for cmd notify from CLI/ADB or other terminal
            BTTOOL_LOGW("Waiting for test cmd ...");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    } while (1);
}

void bt_tool_cmd_submit(BT_HW_TEST_CMD_T *target_cmd)
{
    if (NULL == target_cmd) {
        BTTOOL_LOGE("%s, cmd is NULL", __func__);
        return;
    }
    BTTOOL_LOGI("%s, curr_cmd:%d, target_cmd:%d", __func__, curr_test_cmd.cmd, target_cmd->cmd);

    //xSemaphoreTake(curr_cmd_mutex, portMAX_DELAY);
    if ((curr_test_cmd.cmd != BT_HW_TEST_CMD_NONE)
        && (target_cmd->cmd != BT_HW_TEST_CMD_STOP)){
        BTTOOL_LOGW("cmd %d is running, stop the current test first!", curr_test_cmd.cmd);
        //xSemaphoreGive(curr_cmd_mutex);
        return;
    }

    if (target_cmd->cmd == BT_HW_TEST_CMD_STOP) {
        if ((curr_test_cmd.cmd == BT_HW_TEST_CMD_NONE)
            || (curr_test_cmd.cmd == BT_HW_TEST_CMD_INQUIRY)
            || (curr_test_cmd.cmd == BT_HW_TEST_CMD_GET_RX)) {
            BTTOOL_LOGW("No need stop action for test %d", curr_test_cmd.cmd);
            return;
        }
        //stop current test
        waiting_stop_cmd = FALSE;
        BTTOOL_LOGW("Clear waiting_stop_cmd flag");
        return;
    }
    memcpy(&curr_test_cmd, target_cmd, sizeof(BT_HW_TEST_CMD_T));
    BTTOOL_LOGI("Submit cmd %d", curr_test_cmd.cmd);
    //xSemaphoreGive(curr_cmd_mutex);

    xTaskNotifyGive(bt_tool_task_hdl);
}

int bt_tool_init(void)
{
    BTTOOL_LOGI("%s", __func__);

    memset(&curr_test_cmd, 0, sizeof(curr_test_cmd));
    curr_cmd_mutex = xSemaphoreCreateMutex();
    if (NULL == curr_cmd_mutex) {
        BTTOOL_LOGE("%s create mutex fail.", __func__);
        return -1;
    }

    if (pdPASS != xTaskCreate(bt_tool_task,
                            "BT_TOOL",
                            (1024*4)/sizeof(StackType_t),
                            NULL,
                            configMAX_PRIORITIES-2,
                            &bt_tool_task_hdl)) {
        BTTOOL_LOGE("%s cannot create bt_tool_task.", __func__);
        vSemaphoreDelete(curr_cmd_mutex);
        return -1;
    }
    return 0;
}

