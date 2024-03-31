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

#include <string.h>
#include "boots.h"
#if defined(BTMTK_PLATFORM_MT7933)
#include "btif_mt7933.h"
#include "memory_attribute.h"
#endif

#if defined(BTMTK_PLATFORM_NXP)
/*#define NXP_UART*/
#if defined(NXP_UART)
#include "fsl_usart.h"
#include "fsl_usart_dma.h"
#include "fsl_dma.h"
#endif
#define BOARD_UART_BAUDRATE 115200

#define USE_USART USART0
#define USE_USART_CLK_SRC kCLOCK_Flexcomm0
#define USE_USART_CLK_FREQ CLOCK_GetFreq(kCLOCK_Flexcomm0)
#define USART_RX_DMA_CHANNEL 0
#define USART_TX_DMA_CHANNEL 1
#define USE_UART_DMA_BASEADDR DMA0

#endif

#define LOG_TAG "boots_uart"

#define BOOTS_UART_WRITE_RETRY_CNT   3
#define BOOTS_UART_WRITE_RETRY_DELAY 10 // ms

static int g_inited_uart_port = -1;
extern TaskHandle_t boots_server_uart_handle;

//---------------------------------------------------------------------------
static void uart_read_from_input(hal_uart_callback_event_t event, void *user_data)
{
    //BPRINT_D("%s: evt = %d", __func__, event);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (event == HAL_UART_EVENT_READY_TO_READ) {
        if (boots_server_uart_handle)
            vTaskNotifyGiveFromISR(boots_server_uart_handle, &xHigherPriorityTaskWoken);
    }
}

//---------------------------------------------------------------------------
int boots_uart_init_freertos(int port, int speed)
{
    hal_uart_status_t status_t;
    status_t = bt_uart_init(port, speed);
    if (status_t != HAL_UART_STATUS_OK) {
        BPRINT_E("%s: init FAIL!", __func__);
        return (int)status_t;
    }
    g_inited_uart_port = port;

    status_t = hal_uart_register_callback(port, uart_read_from_input, NULL);

    BPRINT_D("%s: success", __func__);
    return 0;
}

//---------------------------------------------------------------------------
int boots_uart_deinit_freertos(void)
{
    int ret = 0;

    if (g_inited_uart_port < 0) return 0;

    hal_uart_register_callback(g_inited_uart_port, NULL, NULL);

    ret = (int)bt_uart_deinit(g_inited_uart_port);

    g_inited_uart_port = -1;
    return ret;
}

//---------------------------------------------------------------------------
/* Data is remained in vfifo buffer, but not in DMA buffer*/
int boots_uart_read_remained_bytes(int fd)
{
    uint32_t length;

    BPRINT_I("%s: fd = 0x%x", __func__, fd);

    length = hal_uart_get_available_receive_bytes(fd);
    BPRINT_I("%s: len = %d", __func__, (int)length);
    return length;
}

//---------------------------------------------------------------------------
int boots_uart_read_freertos(int fd, unsigned char *buf, int len)
{
    if (g_inited_uart_port < 0 ||
        g_inited_uart_port != fd) {
        BPRINT_W("fd = %d doesn't init or wrong. (exp: %d)", fd, g_inited_uart_port);
        return 0;
    }

    return bt_uart_read(fd, buf, len);
}

/*Call this API to read expected length data*/
int boots_uart_read_freertos_with_retry(int fd, unsigned char *buf, int len)
{
    uint32_t length = 0;
    uint32_t total_read = 0;
    uint8_t retry = 5;

    if (len <= 0) {
        BPRINT_E("%s: error, len < 0", __func__);
        return total_read;
    }
    BPRINT_D("%s: fd = 0x%x (%d)", __func__, fd, len);

    if (g_inited_uart_port < 0 ||
        g_inited_uart_port != fd) {
        BPRINT_W("fd = %d doesn't init or wrong. (exp: %d)", fd, g_inited_uart_port);
        return 0;
    }

    do {
        length = bt_uart_read(HAL_UART_2, buf + total_read, len - total_read);
        total_read += length;
        retry --;
        if (total_read < (uint32_t)len) {
            BPRINT_W("boots_uart_read: wait 10ms, total = %d/%d, retry (%d)", (int)total_read, len, retry);
            vTaskDelay(10/portTICK_PERIOD_MS); //wait 10ms for uart driver to move data
        }
    } while (total_read < (uint32_t)len && retry > 0);
    return total_read;
}

//---------------------------------------------------------------------------
uint32_t boots_uart_write_freertos(int fd, const void *buf, int len)
{
    if (g_inited_uart_port < 0 ||
        g_inited_uart_port != fd) {
        BPRINT_W("fd = %d doesn't init or wrong. (exp: %d)", fd, g_inited_uart_port);
        return 0;
    }

    return bt_uart_write(fd, (unsigned char *)buf, len);
}
