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
 * Copyright(C) 2018 MediaTek Inc
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

//- vim: set ts=4 sts=4 sw=4 et: --------------------------------------------
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "boots.h"
#include "boots_srv.h"
#include "FreeRTOS.h"
#include "bt_driver.h"
#include "boots_pkt.h"
#include "boots_btif.h"
#include "boots_uart.h"
#include "boots_stress.h"
#include "queue.h"

//---------------------------------------------------------------------------
#define LOG_TAG "boots_srv"

//---------------------------------------------------------------------------
//A struct to control event buffer
typedef struct {
    unsigned char   *p_buf_head;
    uint32_t        write_offset;
    uint32_t        buf_max;
    bool            header_in_buf;    // Set true if hci header in buf
    uint32_t        wait_buf;         // Indicate how many buf we need for rest hci data
} boots_srv_buf_op;

//---------------------------------------------------------------------------
#if BUFFER_USING_HEAP
unsigned char remain[REMAIN_SIZE];
unsigned char *cmd_buffer = NULL;
#else
unsigned char cmd_buffer[CMD_BUF_LEN];
#endif
unsigned char event_buffer[EVENT_BUF_LEN];
TaskHandle_t boots_server_tx_handle = NULL;
TaskHandle_t boots_server_rx_handle = NULL;
TaskHandle_t boots_server_uart_handle = NULL;
xSemaphoreHandle boots_srv_buf_mutex = NULL;
xSemaphoreHandle boots_client_semaphore = NULL;
boots_srv_buf_op g_evt_buf_op;
static uint8_t boots_srv_running = 0;

#define BOOTS_SRV_TASK_PRI (configMAX_PRIORITIES - 3)

//---------------------------------------------------------------------------
extern int g_relay_mode;


//---------------------------------------------------------------------------
void boots_src_show_all_server_handle(void)
{
    BPRINT_D("boots_server_tx_handle = %p", boots_server_tx_handle);
    BPRINT_D("boots_server_rx_handle = %p", boots_server_rx_handle);
    BPRINT_D("boots_server_uart_handle = %p", boots_server_uart_handle);
}

//---------------------------------------------------------------------------
TaskHandle_t boots_srv_get_server_tx_handle(void)
{
    return boots_server_tx_handle;
}

TaskHandle_t boots_srv_get_server_rx_handle(void)
{
    return boots_server_rx_handle;
}

TaskHandle_t boots_srv_get_server_uart_handle(void)
{
    return boots_server_uart_handle;
}

//---------------------------------------------------------------------------
xSemaphoreHandle boots_srv_mutex_create(void)
{
    return xSemaphoreCreateMutex();
}

void boots_srv_mutex_delete(xSemaphoreHandle sema)
{
    vSemaphoreDelete(sema);
}

int boots_srv_mutex_lock(xSemaphoreHandle sema)
{
    if (sema != NULL)
    {
        return xSemaphoreTake(sema, portMAX_DELAY);
    }
    return 0;
}

void boots_srv_mutex_unlock(xSemaphoreHandle sema)
{
    if (sema != NULL)
    {
        xSemaphoreGive(sema);
    }
}

//---------------------------------------------------------------------------
void boots_client_semaphore_create(void)
{
    if (!boots_client_semaphore) {
        BPRINT_D("%s: Semaphore Create!", __func__);
        boots_client_semaphore = xSemaphoreCreateBinary();
    } else {
        BPRINT_W("%s: Already Created!", __func__);
    }
}

//---------------------------------------------------------------------------
void boots_client_semaphore_delete(void)
{
    if (boots_client_semaphore) {
        // BPRINT_D("%s: Semaphore Delete!", __func__);
        vSemaphoreDelete(boots_client_semaphore);
        boots_client_semaphore = NULL;
    } else {
        BPRINT_W("%s: Not Created!", __func__);
    }
}

//---------------------------------------------------------------------------
bool boots_client_semaphore_take(TickType_t xTicksToWait)
{
    if (boots_client_semaphore) {
        // BPRINT_D("%s: Semaphore Take!", __func__);
        return xSemaphoreTake(boots_client_semaphore, xTicksToWait);
    }
    BPRINT_W("%s: Semaphore not Created!", __func__);
    return pdFALSE;
}

//---------------------------------------------------------------------------
bool boots_client_semaphore_give(void)
{
    if (boots_client_semaphore) {
        BPRINT_D("%s: Semaphore Give!", __func__);
        return xSemaphoreGive(boots_client_semaphore);
    }
    BPRINT_W("%s: Semaphore not Created!", __func__);
    return pdFALSE;
}

//---------------------------------------------------------------------------
/*Get hci len position from HCI header*/
int boots_srv_get_hci_len_offset(unsigned char type)
{
    int len_offset = 0;

    switch (type) {
        case HCI_CMD_PKT:
            len_offset = 3;
            break;
        case HCI_EVENT_PKT:
            len_offset = 2;
            break;
        case HCI_ACL_PKT:
            len_offset = 4;
            break;
        case HCI_SCO_PKT:
            len_offset = 3;
            break;
        default:
            BPRINT_E("Receive unknown type %02X", type);
            break;
    }

    BPRINT_D("%s, offset = %d", __func__, len_offset);
    return len_offset;
}

int boots_srv_get_hci_len(unsigned char *buf, size_t len)
{
    int pkt_len = 0;

    if (buf[0] == HCI_CMD_PKT && len >= 4) {
        pkt_len = buf[3] + HCI_CMD_PKT_HDR_LEN;
    } else if (buf[0] == HCI_EVENT_PKT && len >= 3) {
        pkt_len = buf[2] + HCI_EVENT_PKT_HDR_LEN;
    } else if (buf[0] == HCI_ACL_PKT && len >= 5) {
        pkt_len = (buf[4] << 8) + buf[3] + HCI_ACL_PKT_HDR_LEN;
    } else if (buf[0] == HCI_SCO_PKT && len >= 4) {
        pkt_len = buf[3] + HCI_SCO_PKT_HDR_LEN;
    } else {
        BPRINT_E("Receive unknown type (len = %d) %02X %02X %02X %02X", len, buf[0],
                buf[1],buf[2], buf[3]);
        return 0;
    }

    BPRINT_D("%s, len = %d", __func__, pkt_len);
    return pkt_len;
}

//---------------------------------------------------------------------------
void boots_srv_get_event_buffer_content(unsigned char *buf, size_t *len)
{
    unsigned char *p_buf = NULL;
    int hci_len = 0;
    bool is_sleep = false;

    if (buf == NULL) {
        BPRINT_E("buf is NULL, return");
        return;
    }

    boots_srv_mutex_lock(boots_srv_buf_mutex);

    *len = 0;
    if (!g_evt_buf_op.write_offset) {
        BPRINT_D("get_evt_buf: event buf is empty");
        boots_srv_mutex_unlock(boots_srv_buf_mutex);
        return;
    }

    /* Only provide a completed hci data to upper layer.*/
    p_buf = g_evt_buf_op.p_buf_head;
    hci_len = boots_srv_get_hci_len(p_buf, g_evt_buf_op.write_offset);
    BPRINT_D("get_evt_buf:hci_len = %d, w_offset =%d", hci_len, (int)g_evt_buf_op.write_offset);
    if (hci_len <= 0) {
        BPRINT_W("get_evt_buf:hci_len (%d) is not valid!!", hci_len);
    } else if (hci_len > (int)g_evt_buf_op.write_offset) {
        BPRINT_W("get_evt_buf:buf (%d) is not enough!!(expected = %d)",
                 (int)g_evt_buf_op.write_offset, hci_len);
        //If data remained in uart dma, we should trigger boots server to rx data.
        if (boots_server_uart_handle)
            xTaskNotifyGive(boots_server_uart_handle);
        is_sleep = true;
    } else {
        *len = hci_len;
        memcpy(buf, p_buf, *len);
        g_evt_buf_op.write_offset -= hci_len;
        if (!g_evt_buf_op.write_offset) {
            memset(p_buf, 0, hci_len); //Only memset dirty data to reduce CPU time
        } else {
            memcpy(p_buf, p_buf + hci_len, g_evt_buf_op.write_offset);
            memset(p_buf + g_evt_buf_op.write_offset, 0, hci_len); //Only memset dirty data to reduce CPU time
        }
    }
    boots_srv_mutex_unlock(boots_srv_buf_mutex);

    if (is_sleep)
       vTaskDelay(1000/portTICK_PERIOD_MS); //to prevent too many logs

}

//---------------------------------------------------------------------------
bool boots_srv_rx_available(void)
{
    boots_srv_mutex_lock(boots_srv_buf_mutex);
    if (g_evt_buf_op.write_offset == 0) {
        boots_srv_mutex_unlock(boots_srv_buf_mutex);
        return false;
    }
    boots_srv_mutex_unlock(boots_srv_buf_mutex);
    return true;
}

//---------------------------------------------------------------------------
void boots_srv_set_cmd_buffer_content(const void *p, int len)
{
    uint8_t *buf = (uint8_t *)p;
    if (buf == NULL) {
        BPRINT_E("buf is NULL, return");
        return;
    }

    if (buf[0] == 0) {
        BPRINT_E("length is 0, return");
        return;
    }

    boots_srv_mutex_lock(boots_srv_buf_mutex);
    cmd_buffer[0] = len;
    cmd_buffer[1] = len >> 8;
    memcpy(&cmd_buffer[2], buf, len);
    boots_srv_mutex_unlock(boots_srv_buf_mutex);
}

//---------------------------------------------------------------------------
void boots_srv_main(void * parm)
{
    int ret = 0;
    int len = 0;
    int type = (int)parm;
    int hci_len = 0;
    int hci_len_offset = 0;
    uint32_t evt_buf_left = 0;
    uint32_t evt_buf_expect = 0;
    unsigned char *p_buf = NULL;
    bool ignore_task_take = false;

    BPRINT_I("%s: type = 0x%x", __func__, type);

    if (boots_srv_buf_mutex == NULL) {
        boots_srv_buf_mutex = boots_srv_mutex_create();
        BPRINT_D("boots_srv_buf_mutex created (%p) !!!", boots_srv_buf_mutex);
    }
    if (boots_srv_buf_mutex == NULL) {
        BPRINT_E("boots_srv_buf_mutex create fail !!!");
    }

    memset(cmd_buffer, 0, CMD_BUF_LEN);
    memset(event_buffer, 0, EVENT_BUF_LEN);

    memset(&g_evt_buf_op, 0, sizeof(boots_srv_buf_op));
    g_evt_buf_op.p_buf_head = event_buffer;
    g_evt_buf_op.buf_max = EVENT_BUF_LEN;

    if (type == BOOTS_SRV_TX)
        boots_server_tx_handle = xTaskGetCurrentTaskHandle();
    else if (type == BOOTS_SRV_RX)
        boots_server_rx_handle = xTaskGetCurrentTaskHandle();
    else if (type == BOOTS_SRV_UART) {
        boots_server_uart_handle = xTaskGetCurrentTaskHandle();
    }

    while (1){
        if (type == BOOTS_SRV_TX) {
            BPRINT_D("boots tx server sleep");
            if (!cmd_buffer[0])
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            boots_srv_mutex_lock(boots_srv_buf_mutex);
            if (cmd_buffer[0] != 0 || cmd_buffer[1] != 0) {
                len = cmd_buffer[0] + (cmd_buffer[1] << 8);
                bt_driver_tx(&cmd_buffer[2], len);
                memset(cmd_buffer, 0, CMD_BUF_LEN);
            }
            boots_srv_mutex_unlock(boots_srv_buf_mutex);

        } else if (type == BOOTS_SRV_RX){/*RX*/
            BPRINT_D("boots rx server sleep");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            /*check read*/
            boots_srv_mutex_lock(boots_srv_buf_mutex);
            evt_buf_left = EVENT_BUF_LEN - g_evt_buf_op.write_offset;
            if (g_evt_buf_op.write_offset) {
                BPRINT_D("%s: left (%d) for drv rx", __func__, (int)evt_buf_left);
            }
            p_buf = g_evt_buf_op.p_buf_head + g_evt_buf_op.write_offset;
            ret = bt_driver_rx(p_buf, evt_buf_left);
            if (ret > 0) {
                if (boots_client_semaphore) {
                    g_evt_buf_op.write_offset += ret;
                    BPRINT_I("DRV RX: write_offset = %d", (int)g_evt_buf_op.write_offset);
                    boots_client_semaphore_give();
                } else {
                    BPRINT_D("UART TX: len %d", ret);
                    SHOW_RAW(ret, p_buf);
                    boots_uart_write_freertos(HAL_UART_2, p_buf, ret);
                }
            }
            boots_srv_mutex_unlock(boots_srv_buf_mutex);

        } else if (type == BOOTS_SRV_UART) {
            if (g_evt_buf_op.wait_buf) {
                BPRINT_I("UART SRV: wait 10ms for buf rls");
                vTaskDelay(10/portTICK_PERIOD_MS);
            } else if (ignore_task_take){
                BPRINT_D("UART SRV: no wait check rx");
                ignore_task_take = false;
            } else {
                BPRINT_D("UART SRV sleep");
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            }

            BPRINT_D("UART SRV: get noti");

            if (g_relay_mode) {
                boots_srv_mutex_lock(boots_srv_buf_mutex);
                len = boots_uart_read_freertos(HAL_UART_2, cmd_buffer, CMD_BUF_LEN);
                BPRINT_D("UART RX: cmd len %d", len);
                SHOW_RAW(len, cmd_buffer);
                if (cmd_buffer[0] == 00) {
                    BPRINT_I("%s: SW workaround for HAPS, skip first 00 byte", __func__);
                    bt_driver_tx(cmd_buffer + 1, len - 1);
                } else {
                    bt_driver_tx(cmd_buffer, len);
                }
                boots_srv_mutex_unlock(boots_srv_buf_mutex);

            } else {
                boots_srv_mutex_lock(boots_srv_buf_mutex);
                evt_buf_left = EVENT_BUF_LEN - g_evt_buf_op.write_offset;
                //If we're waiting for buf, it means we had parsered the header
                if (!g_evt_buf_op.header_in_buf) {
                    // Read from hci header
                    if (evt_buf_left < 5) { //We at least need 5 bytes free spaces for parser HCI header
                        g_evt_buf_op.wait_buf = 5;
                        //No enough buf for header parser, so wait buf to be consumed.
                        BPRINT_W("UART RX: evt buf not enough (%d)", (int)evt_buf_left);
                        goto read_uart_end;
                    }
                    p_buf = g_evt_buf_op.p_buf_head + g_evt_buf_op.write_offset;
                    len = boots_uart_read_freertos(HAL_UART_2, p_buf, 1); //No need wait retry for 1st bytes
                    if (len <= 0) {
                        //BPRINT_E("UART RX: No data set to continue, len = %d", len);
                        goto read_uart_end;
                    }
                    //BPRINT_I("UART RX: evt %d ", len);
                    hci_len_offset = boots_srv_get_hci_len_offset(p_buf[0]);
                    if (hci_len_offset == 0) {
                        BPRINT_E("drop one byte and continue %02X", p_buf[0]);
                        p_buf[0] = 0;
                        goto read_uart_end;
                    }
                    len = boots_uart_read_freertos_with_retry(HAL_UART_2, &p_buf[1], hci_len_offset);
                    BPRINT_D("UART RX: event head len %d", len);
                    hci_len = boots_srv_get_hci_len(&p_buf[0], len + 1); //+1 is for including header type byte
                    if (hci_len == 0) {
                        BPRINT_E("drop bytes and continue %02X %02X %02X %02X",
                                 p_buf[0], p_buf[1], p_buf[2], p_buf[3]);
                        goto read_uart_end;
                    }
                    g_evt_buf_op.write_offset += hci_len_offset + 1;
                    evt_buf_expect = hci_len - (hci_len_offset + 1);
                    if (evt_buf_left < evt_buf_expect) {
                        BPRINT_E("UART RX: evt buf (%d) not enough.(exp = %d)",
                                  (int)evt_buf_left, (int)evt_buf_expect);
                        g_evt_buf_op.wait_buf = evt_buf_expect;
                        g_evt_buf_op.header_in_buf = true;
                        goto read_uart_end;
                    }
                } else {
                    evt_buf_expect = g_evt_buf_op.wait_buf;
                    if (evt_buf_left < g_evt_buf_op.wait_buf) {
                        BPRINT_E("UART RX: evt buf (%d) still not enough. (exp = %d)",
                                  (int)evt_buf_left, (int)evt_buf_expect);
                        goto read_uart_end;
                    }
                }
                p_buf = g_evt_buf_op.p_buf_head + g_evt_buf_op.write_offset;
                len = boots_uart_read_freertos_with_retry(HAL_UART_2, p_buf, evt_buf_expect);
                BPRINT_I("UART RX: event len %d", len);
                if (len < 0) {
                    BPRINT_E("UART RX: unexpected case!!! stop thread. (len = %d, exp = %d)",
                            len, (int)evt_buf_expect);
                    boots_srv_mutex_unlock(boots_srv_buf_mutex);
                    break;
                }
                SHOW_RAW(len, p_buf);
                g_evt_buf_op.write_offset += len;
                g_evt_buf_op.header_in_buf = false;
                g_evt_buf_op.wait_buf = 0;
                ignore_task_take = true; //We may have data remained in UART DMA, so set true to read again.
read_uart_end:
                boots_srv_mutex_unlock(boots_srv_buf_mutex);
                if (boots_client_semaphore && len > 0) {
                    BPRINT_D("%s: wakeup boots client!!", __func__);
                    boots_client_semaphore_give();
                }
            }
        }
    }

    vTaskDelete(xTaskGetCurrentTaskHandle());
    BPRINT_I("Service STOP!");
}

//---------------------------------------------------------------------------
static void boots_data_ready(void)
{
    BPRINT_D("%s", __func__);
    if (boots_server_rx_handle)
        xTaskNotifyGive(boots_server_rx_handle);
}

//---------------------------------------------------------------------------
bool create_boots_srv(void)
{
    BaseType_t xReturned = 0;
    //TaskHandle_t boots_main_thread_tx_Handle;
    //TaskHandle_t boots_main_thread_uart_Handle;
    int wait_cnt = 10;

    if (boots_srv_running) {
        BPRINT_D("%s: boots_srv is running, no need to create again", __func__);
        return true;
    }

#if BUFFER_USING_HEAP
    if (!cmd_buffer) {
        cmd_buffer = (unsigned char *)pvPortMalloc(CMD_BUF_LEN * sizeof(unsigned char));
        if (!cmd_buffer) {
            BPRINT_D("malloc command buffer fail!");
            return false;
        }
        memset(cmd_buffer, 0, CMD_BUF_LEN * sizeof(unsigned char));
        BPRINT_D("malloc command buffer success!");
    }
#endif

    /*-create uart boots tx task-*/
    xReturned = xTaskCreate(
        boots_srv_main,/* Function that implements the task. */
        "boots_S-tx_thread",      /* Text name for the task. */
        400,/* Stack size in words, not bytes. */
        NULL,/*TX*/
        BOOTS_SRV_TASK_PRI,/* Priority at which the task is created. */
        NULL);      /* Used to pass a handle to the created task out of the xTaskCreate(). */

    if (xReturned == pdPASS) {
        /* The task was created.  Use the task's handle to delete the task. */
        BPRINT_I("%s boots server main tx thread create success", __func__);
    } else {
        BPRINT_E("%s boots server main tx thread create fail", __func__);
        return false;
    }

    /*-create uart boots rx task-*/
    xReturned = xTaskCreate(
        boots_srv_main,/* Function that implements the task. */
        "boots_S-rx_thread",      /* Text name for the task. */
        512,/* Stack size in words, not bytes. */
        (int *)BOOTS_SRV_RX,/*RX*/
        BOOTS_SRV_TASK_PRI,/* Priority at which the task is created. */
        NULL);/* Used to pass a handle to the created task out of the xTaskCreate(). */

    if (xReturned == pdPASS) {
        /* The task was created.  Use the task's handle to delete the task. */
        bt_driver_register_event_cb(boots_data_ready, 0);
        BPRINT_I("%s boots server main rx thread create success", __func__);
    } else {
        BPRINT_E("%s boots server main rx thread create fail", __func__);
        return false;
    }

    /*-create uart boots server task-*/
    xReturned = xTaskCreate(
        boots_srv_main,/* Function that implements the task. */
        "boots_S-uart_thread",/* Text name for the task. */
        400,/* Stack size in words, not bytes. */
        (int *) BOOTS_SRV_UART,/*RX*/
        BOOTS_SRV_TASK_PRI,/* Priority at which the task is created. */
        NULL);/* Used to pass a handle to the created task out of the xTaskCreate(). */

    if (xReturned == pdPASS) {
        /* The task was created.  Use the task's handle to delete the task. */
        BPRINT_I("%s boots server main uart thread create success", __func__);
    } else {
        BPRINT_I("%s boots server main uart thread create fail", __func__);
        return false;
    }

    // In SLT RTOS project, it tooks 80ms for creating all tasks.
    do {
        if (!boots_srv_get_server_tx_handle() ||
            !boots_srv_get_server_rx_handle() ||
            !boots_srv_get_server_uart_handle()) {
            vTaskDelay(20 / portTICK_PERIOD_MS);
            BPRINT_D("wait 20ms for creating all tasks. (%d)", wait_cnt);
        }
    } while ((wait_cnt--) > 0);

    boots_src_show_all_server_handle();

    boots_srv_running = true;
    return true;
}

//---------------------------------------------------------------------------
bool kill_boots_srv(void)
{
    BPRINT_I("kill boots srv");
    if (boots_server_tx_handle) {
        vTaskDelete(boots_server_tx_handle);
        boots_server_tx_handle = NULL;
        BPRINT_D("delete boots tx task");
    }

    if (boots_server_rx_handle) {
        vTaskDelete(boots_server_rx_handle);
        boots_server_rx_handle = NULL;
        BPRINT_D("delete boots rx task");
    }

    if (boots_server_uart_handle) {
        vTaskDelete(boots_server_uart_handle);
        boots_server_uart_handle = NULL;
        BPRINT_D("delete boots uart task");
    }

#if BUFFER_USING_HEAP
    if (cmd_buffer) {
        vPortFree(cmd_buffer);
        cmd_buffer = NULL;
        BPRINT_D("free command buffer");
    }
#endif

    if (boots_srv_buf_mutex) {
        boots_srv_mutex_delete(boots_srv_buf_mutex);
        boots_srv_buf_mutex = NULL;
    }

    boots_srv_running = false;
    return true;
}
