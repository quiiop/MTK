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

#include <stdint.h>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

//#include <dma_sw.h>
#include <hal_uart.h>

#include "io_def.h"
#include "syslog.h"
#include "memory_attribute.h"
#include "hal_eint.h"
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#include "hal_nvic.h"


#if  defined ( __GNUC__ )
#ifndef __weak
#define __weak   __attribute__((weak))
#endif /* #ifndef __weak */
#endif /* #if  defined ( __GNUC__ ) */


#define VFIFO_TX_SIZE           512
#define VFIFO_RX_SIZE           128
#define VFIFO_RX_THRESHOLD      1
#define VFIFO_ALERT_LENGTH      0
#if defined (MTK_TF_WIFI_AUTO_TEST)
#define UART_RX_TIMEOUT         10000
#else /* #if defined (MTK_TF_WIFI_AUTO_TEST) */
#define UART_RX_TIMEOUT         3000
#endif /* #if defined (MTK_TF_WIFI_AUTO_TEST) */

#define IO_INTERRUPT_MODE

/* Block UART definition ----------------------------------------------------*/
#ifdef IO_INTERRUPT_MODE
static bool is_io_ready = false;

static SemaphoreHandle_t    _g_semaphore_tx = NULL;
static SemaphoreHandle_t    _g_semaphore_rx = NULL;

//ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint8_t              g_tx_vfifo[VFIFO_TX_SIZE];
//ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint8_t              g_rx_vfifo[VFIFO_RX_SIZE];
#endif /* #ifdef IO_INTERRUPT_MODE */

/****************************************************************************
 * Forward Declaration
 ****************************************************************************/

#ifdef IO_INTERRUPT_MODE
static void _uart_event(hal_uart_callback_event_t event, void *user_data);
#endif /* #ifdef IO_INTERRUPT_MODE */

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#ifdef IO_INTERRUPT_MODE

#ifdef HAL_SLEEP_MANAGER_ENABLED
static void io_def_uart_sleep_management_suspend_callback(void *data)
{
    hal_nvic_clear_pending_irq(CM33_UART_RX_IRQn);
}

static void io_def_uart_sleep_management_resume_callback(void *data)
{
    if (hal_nvic_get_pending_irq(CM33_UART_RX_IRQn)) {
        xSemaphoreGive(_g_semaphore_rx);
    }
}
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

static void _uart_event(hal_uart_callback_event_t event, void *user_data)
{
    BaseType_t  x_higher_priority_task_woken = pdFALSE;

    switch (event) {
        case HAL_UART_EVENT_READY_TO_READ:
            xSemaphoreGiveFromISR(_g_semaphore_rx, &x_higher_priority_task_woken);
            break;
        case HAL_UART_EVENT_READY_TO_WRITE:
            xSemaphoreGiveFromISR(_g_semaphore_tx, &x_higher_priority_task_woken);
            break;
        default:
            break;
    }

    /*
     * xSemaphoreGiveFromISR() will set *pxHigherPriorityTaskWoken to pdTRUE
     * if giving _g_semaphore_rx or _g_semaphore_tx caused a task to unblock,
     * and the unblocked task has a priority higher than the currently running
     * task. If xSemaphoreGiveFromISR() sets this value to pdTRUE then a
     * context switch should be requested before the interrupt exits.
     */

    portYIELD_FROM_ISR(x_higher_priority_task_woken);
}

/**
 * Check for conditions that blocking APIs can not be used.
 *
 * 1. Before OS starts.
 * 2. When interrupt is disabled.
 * 3. Currently in an ISR.
 */
static int8_t _uart_dma_blocking_is_safe(void)
{
#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING &&
        __get_PRIMASK() == 0                              &&
        ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos) == 0) {
        return 1;
    }
#endif /* #if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) ) */

    return 0;
}
#endif /* #ifdef IO_INTERRUPT_MODE */

/****************************************************************************
 * Public Functions
 ****************************************************************************/


__weak PUTCHAR_PROTOTYPE {
    return bsp_io_def_uart_putchar(ch);
}


__weak GETCHAR_PROTOTYPE {
    return bsp_io_def_uart_getchar();
}


/**
  * @brief  Retargets the C library printf function to the USART.
  *
  * This API correspond to __io_putchar() for many open source C library
  * implementations. Most commonly, it is called by printf series APIs.
  * It could be used before or after OS starts running (scheduler starts).
  *
  * The difference in phases before or after OS starts is the supporting of
  * blocking APIs.
  *
  * The implementation sends to UART using polling API if scheduler is not
  * running. If scheduler is running, it sends to TX VFIFO when there is space
  * in TX VFIFO. When there is no space in VFIFO, it waits for TX ready event
  * (which means there is now some space in TX VFIFO) from DMA engine before
  * continuing the transmission.
  *
  * @param  ch  the character to be sent.
  *
  * @retval None
  */
int bsp_io_def_uart_putchar(int ch)
{
    static int last_char;

#ifdef IO_INTERRUPT_MODE
    uint32_t n;

    if (!is_io_ready) {
        return 0;
    }

    if (_uart_dma_blocking_is_safe()) {
        if (ch == '\n' && last_char != '\r') {
            do {
                n = hal_uart_send_dma(CONSOLE_UART, (uint8_t *)"\r", 1);
            } while (!n && xSemaphoreTake(_g_semaphore_tx, 1000/*portMAX_DELAY*/) != pdTRUE);
        }

        do {
            n = hal_uart_send_dma(CONSOLE_UART, (uint8_t *)&ch, 1);
        } while (!n && xSemaphoreTake(_g_semaphore_tx, 1000/*portMAX_DELAY*/) != pdTRUE);
    } else {
        if (ch == '\n' && last_char != '\r') {
            do {
                n = hal_uart_send_dma(CONSOLE_UART, (uint8_t *)"\r", 1);
            } while (!n);
        }

        do {
            n = hal_uart_send_dma(CONSOLE_UART, (uint8_t *)&ch, 1);
        } while (!n);
    }

    last_char = ch;

    return ch;
#else /* #ifdef IO_INTERRUPT_MODE */

    if (ch == '\n' && last_char != '\r')
        WriteDebugByte('\r');

    last_char = ch;

    return WriteDebugByte(ch);
#endif /* #ifdef IO_INTERRUPT_MODE */
}

/**
  * @brief  Retargets the C library getchar function to the USART.
  * @param  None
  * @retval None
  */
int bsp_io_def_uart_getchar(void)
{
#ifdef IO_INTERRUPT_MODE
    /* Blocked UART Getchar */

#if configUSE_TICKLESS_IDLE == 2
    bool has_sleep_lock = true;
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_CONSOLE_UART);
#endif /* #if configUSE_TICKLESS_IDLE == 2 */

    while (1) {
        uint8_t     ret;
        uint32_t    len;

        len = hal_uart_receive_dma(CONSOLE_UART, &ret, 1);

        if (len > 0) {
#if configUSE_TICKLESS_IDLE == 2
            if (has_sleep_lock) {
                sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_CONSOLE_UART);
            }
#endif /* #if configUSE_TICKLESS_IDLE == 2 */
            return ret;
        } else {
#if configUSE_TICKLESS_IDLE == 2
            TickType_t xTicksToWait = has_sleep_lock ? UART_RX_TIMEOUT / portTICK_PERIOD_MS : portMAX_DELAY;

            if (xSemaphoreTake(_g_semaphore_rx, xTicksToWait) == pdTRUE) {
                if (!has_sleep_lock) {
                    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_CONSOLE_UART);
                    has_sleep_lock = true;
                }
                continue;
            }

            sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_CONSOLE_UART);
            has_sleep_lock = false;
#else /* #if configUSE_TICKLESS_IDLE == 2 */
            xSemaphoreTake(_g_semaphore_rx, portMAX_DELAY);
#endif /* #if configUSE_TICKLESS_IDLE == 2 */
        }
    }
#else /* #ifdef IO_INTERRUPT_MODE */
    return ReadDebugByte();
#endif /* #ifdef IO_INTERRUPT_MODE */
}

static xSemaphoreHandle sys_log_semaphore = NULL;
void sys_log_semaphore_take(void)
{
    BaseType_t priorityTaskWoken;

    if (!sys_log_semaphore) {
        return;
    }

    if (xPortIsInsideInterrupt()) {
        xSemaphoreTakeFromISR(sys_log_semaphore, &priorityTaskWoken);
    } else {
        xSemaphoreTake(sys_log_semaphore, portMAX_DELAY);
    }
}

void sys_log_semaphore_give(void)
{
    BaseType_t priorityTaskWoken;

    if (!sys_log_semaphore) {
        return;
    }

    if (xPortIsInsideInterrupt()) {
        xSemaphoreGiveFromISR(sys_log_semaphore, &priorityTaskWoken);
    } else {
        xSemaphoreGive(sys_log_semaphore);
    }
}

ATTR_TEXT_IN_SYSRAM int log_write(char *buf, int len)
{
    int left = len;
    int bytes_written;
    sys_log_semaphore_take();
    while (left > 0) {
#ifdef IO_INTERRUPT_MODE
        int send_len = 0;

        while (send_len < left && (buf[send_len] != '\n' || (send_len > 0 && buf[send_len - 1] == '\r')))
            send_len++;

        int replace_crlf = (send_len < left);

        if (_uart_dma_blocking_is_safe()) {
            bytes_written = hal_uart_send_dma(CONSOLE_UART, (uint8_t *)buf, send_len);
            if (bytes_written < send_len) {
                xSemaphoreTake(_g_semaphore_tx, 1000);
            }
        } else {
            bytes_written = hal_uart_send_polling(CONSOLE_UART, (uint8_t *)buf, send_len);
        }

        if (replace_crlf) {
            if (_uart_dma_blocking_is_safe()) {
                bytes_written = hal_uart_send_dma(CONSOLE_UART, (uint8_t *)"\r\n", 2);
                if (bytes_written < 2) {
                    xSemaphoreTake(_g_semaphore_tx, 1000);
                }
            } else {
                hal_uart_send_polling(CONSOLE_UART, (uint8_t *)"\r\n", 2);
            }

            bytes_written = send_len + 1;
        }
#else /* #ifdef IO_INTERRUPT_MODE */
        WriteDebugByte(*buf);
        bytes_written = 1;
#endif /* #ifdef IO_INTERRUPT_MODE */
        left -= bytes_written;
        buf += bytes_written;
    }
    sys_log_semaphore_give();
    return len;
}

int log_write_binary(char *buf, int len)
{
    int left = len;
    int bytes_written;
    sys_log_semaphore_take();
    while (left > 0) {
#ifdef IO_INTERRUPT_MODE
        if (_uart_dma_blocking_is_safe()) {
            bytes_written = hal_uart_send_dma(CONSOLE_UART, (uint8_t *)buf, left);
            if (bytes_written < left) {
                xSemaphoreTake(_g_semaphore_tx, 1000);
            }
        } else {
            bytes_written = hal_uart_send_polling(CONSOLE_UART, (uint8_t *)buf, left);
        }
#else /* #ifdef IO_INTERRUPT_MODE */
        WriteDebugByte(*buf);
        bytes_written = 1;
#endif /* #ifdef IO_INTERRUPT_MODE */
        left -= bytes_written;
        buf += bytes_written;
    }
    sys_log_semaphore_give();
    return len;
}

void bsp_io_def_uart_init(void)
{
#ifdef IO_INTERRUPT_MODE
    //hal_uart_dma_config_t   dma_config = {
    //    .send_vfifo_buffer              = g_tx_vfifo,
    //    .send_vfifo_buffer_size         = VFIFO_TX_SIZE,
    //    .send_vfifo_threshold_size      = VFIFO_TX_SIZE / 8,
    //    .receive_vfifo_buffer           = g_rx_vfifo,
    //    .receive_vfifo_buffer_size      = VFIFO_RX_SIZE,
    //    .receive_vfifo_threshold_size   = VFIFO_RX_THRESHOLD,
    //    .receive_vfifo_alert_size       = VFIFO_ALERT_LENGTH
    //};

    if (!sys_log_semaphore) {
        sys_log_semaphore = xSemaphoreCreateMutex();
    }

    log_uart_init(CONSOLE_UART);
    // Enable CM33 UART RX IRQ for wakeup
    hal_nvic_enable_irq(CM33_UART_RX_IRQn);

    is_io_ready = true;

    /* initialize Semephore */
    _g_semaphore_tx = xSemaphoreCreateBinary();
    _g_semaphore_rx = xSemaphoreCreateBinary();

    //hal_uart_set_dma(CONSOLE_UART, &dma_config);

    hal_uart_register_callback(CONSOLE_UART, _uart_event, NULL);

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_register_suspend_callback(SLEEP_BACKUP_RESTORE_IO_DEF, io_def_uart_sleep_management_suspend_callback, NULL);
    sleep_management_register_resume_callback(SLEEP_BACKUP_RESTORE_IO_DEF, io_def_uart_sleep_management_resume_callback, NULL);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    //_io_def_uart_sleep_init();
#else /* #ifdef IO_INTERRUPT_MODE */
    InitDebugSerial();
#endif /* #ifdef IO_INTERRUPT_MODE */
}

