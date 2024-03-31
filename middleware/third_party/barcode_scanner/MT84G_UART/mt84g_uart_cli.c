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

#if defined(MTK_MINICLI_ENABLE)

#include "hal_uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "task_def.h"

#include <stdlib.h>
/****************************************************************************
 *
 * Static variables.
 *
 ****************************************************************************/
static uint8_t *pbuf;
uint32_t snd_cnt, total;
bool is_io_ready = false;

SemaphoreHandle_t xSemaphore_rx = NULL;
SemaphoreHandle_t xSemaphore_tx = NULL;
static xTaskHandle uart1_rx_task_hdl = NULL;

static uint32_t rx_vfifo_buffer[512] __attribute__ ((section(".noncached_zidata")));
static uint32_t tx_vfifo_buffer[512] __attribute__ ((section(".noncached_zidata")));

#define uart1_rx_init_TASK_NAME        "uart1_rx_init_task"
#define uart1_rx_init_TASK_STACKSIZE   (512)
#define uart1_rx_init_TASK_PRIO        TASK_PRIORITY_NORMAL

#define UART_CMD_SIZE 9
#define UART_RX_MAX_BUFFER_SIZE 512
/****************************************************************************
 *
 * Private variables.
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Local functions.
 *
 ****************************************************************************/
uint8_t uart1_send(uint8_t *buffer, uint32_t size)
{
    if (is_io_ready)
        {
        do 
            {
            snd_cnt = hal_uart_send_dma(HAL_UART_2, (const uint8_t *)buffer, size);
            } 
        while (!snd_cnt && xSemaphoreTake(xSemaphore_tx, 1000) != pdTRUE);
        }
    else
        {
        cli_puts("uart io is not ready!\r");
        return -1;
        }

    return 0;
}

static uint8_t uart_trigger_cmd(void)
{
    uint8_t serial_trigger_enable[UART_CMD_SIZE] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x02, 0x01, 0xAB, 0xCD};

    if (uart1_send(&serial_trigger_enable, sizeof(serial_trigger_enable)) != 0)
        return -1;

    return 0;
}

static void user_uart_callback (hal_uart_callback_event_t status, void *user_data)
{
    BaseType_t  x_higher_priority_task_woken = pdFALSE;

    switch (status) {
        case HAL_UART_EVENT_READY_TO_READ:
            xSemaphoreGiveFromISR(xSemaphore_rx, &x_higher_priority_task_woken);
            break;
        case HAL_UART_EVENT_READY_TO_WRITE:
            xSemaphoreGiveFromISR(xSemaphore_tx, &x_higher_priority_task_woken);
            break;
        default:
            break;
    }

    portYIELD_FROM_ISR(x_higher_priority_task_woken);
}

uint8_t uart_interface_init(void)
{
    
    hal_uart_config_t uart_config;
    hal_uart_dma_config_t dma_config;

    uart_config.baudrate = HAL_UART_BAUDRATE_9600;
    uart_config.parity = HAL_UART_PARITY_NONE;
    uart_config.stop_bit = HAL_UART_STOP_BIT_1;
    uart_config.word_length = HAL_UART_WORD_LENGTH_8;
    hal_uart_init(HAL_UART_2, &uart_config);

    dma_config.receive_vfifo_alert_size = 50;
    dma_config.receive_vfifo_buffer = rx_vfifo_buffer;
    dma_config.receive_vfifo_buffer_size = 512;
    dma_config.receive_vfifo_threshold_size = 128;
    dma_config.send_vfifo_buffer = tx_vfifo_buffer;
    dma_config.send_vfifo_buffer_size = 512;
    dma_config.send_vfifo_threshold_size = 51;
    hal_uart_set_dma(HAL_UART_2, &dma_config);
    hal_uart_register_callback(HAL_UART_2, user_uart_callback, NULL);

    total = 0;
    pbuf = pvPortMalloc(UART_RX_MAX_BUFFER_SIZE);
    memset(pbuf, 0, UART_RX_MAX_BUFFER_SIZE);
    
    xSemaphore_rx = xSemaphoreCreateBinary();
    if (xSemaphore_rx == NULL)
        cli_putc("xSemaphore_rx create fail");

    xSemaphore_tx = xSemaphoreCreateBinary();
    if (xSemaphore_tx == NULL)
        cli_putc("xSemaphore_tx create fail");

    uart1_rx_task_create();

    is_io_ready = true;

    return 0;
}

uint8_t uart_barcode_write_register(uint8_t len, char *param[])
{
    uint8_t uart_cmd_buffer[UART_CMD_SIZE] = {0x7E, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0xAB, 0XCD};
    uint8_t type;

    if (len != 1)
        {
        cli_puts("Invalid parameter!\r");
        return -1;
        }

    uart_cmd_buffer[3] = (toi(param[0], &type) >> 24) & 0xFF;
    uart_cmd_buffer[4] = (toi(param[0], &type) >> 16) & 0xFF;
    uart_cmd_buffer[5] = (toi(param[0], &type) >> 8) & 0xFF;
    uart_cmd_buffer[6] = (toi(param[0], &type)) & 0xFF;

    if ( uart1_send(&uart_cmd_buffer, sizeof(uart_cmd_buffer)) != 0)
        return -1;

    cli_puts("setup finish\r");

    return 0;
}


uint8_t uart_interface_deinit(void)
{

    is_io_ready = false;
    
    if (xSemaphore_rx != NULL)
        vSemaphoreDelete(xSemaphore_rx);
    
    if (xSemaphore_tx != NULL)
        vSemaphoreDelete(xSemaphore_tx);

    vTaskDelete(uart1_rx_task_hdl);

    vPortFree(pbuf);

    hal_uart_deinit(HAL_UART_2);

    return 0;
}

uint8_t scanner_serial_trigger_scan(void)
{
    //uint8_t uart_cmd_buffer[UART_CMD_SIZE] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x00, 0xf5, 0xAB, 0XCD};
    
    //set to serial trigger mode
    //if ( uart1_send(&uart_cmd_buffer, sizeof(uart_cmd_buffer)) != 0)
    //    return -1;

    if (uart_trigger_cmd() != 0)
        return -1;

    return 0;
}

uint8_t scanner_cli_get(uint8_t len, char *param[])
{
    if ( total != 0 )
        {
        for (uint32_t i = 0; i < total; i++)
            {
            if (pbuf[i] == 0x02 && pbuf[i+1] == 0x00 && pbuf[i+2] == 0x00 && pbuf[i+3] == 0x01 && pbuf[i+4] == 0x00 && pbuf[i+5] == 0x33 && pbuf[i+6] == 0x31)
                i += 6;
            else
                cli_putc(pbuf[i]);
            }
        cli_putc('\r');
        }
        
    total = 0;

    return 0;
}

uint8_t uart1_getchar(void)
{
    while (1) 
        {
        uint8_t     ret;
        uint32_t    len;

        len = hal_uart_receive_dma(HAL_UART_2, &ret, 1);

        if (len > 0) 
            {
            return ret;
            } 
        else 
            {
            xSemaphoreTake(xSemaphore_rx, portMAX_DELAY);
            }
        }
}

void uart1_rx_init_task(void *param)
{
    uint8_t c;
   
    while(1)
       {
       c =  uart1_getchar();
       pbuf[total] = c;
       total += 1;
       }
}

int uart1_rx_task_create(void)
{
    if (xTaskCreate(uart1_rx_init_task,
                    uart1_rx_init_TASK_NAME,
                    uart1_rx_init_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                    NULL,
                    uart1_rx_init_TASK_PRIO,
                    &uart1_rx_task_hdl) != pdPASS) {
        cli_puts("xTaskCreate fail\r\n");
        return -1;
    }

    return 0;
}
/****************************************************************************
 *
 * API variable.
 *
 ****************************************************************************/


/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/

#endif /* #if defined(MTK_MINICLI_ENABLE) */