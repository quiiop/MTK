/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */
#include "hal.h"

#ifdef HAL_UART_MODULE_ENABLED
#include <common.h>
#ifdef FREERTOS_API
#include "event_groups.h"
#include "FreeRTOS.h"
#include "task.h"
#endif /* #ifdef FREERTOS_API */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hal_uart_internal.h"
#include "hal_log.h"
#include "hal_nvic_internal.h"
#include "memory_attribute.h"

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_gpt.h"
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */


#define UART_BAUD_RATE 115200
#define INVAL_UART_BASE 0xFFFFFFFF

#define DIV_EXACTLY(x, div) ((x) / (div))
#define DIV_ROUND_CLOSEST(x, div)   (((x) + ((div) / 2)) / (div))
#define DIV_ROUND_UP(n, div)        (((n) + (div) - 1) / (div))

#define uart_console(port) ((port) == HAL_UART_0)

static unsigned int debug_uart_base = INVAL_UART_BASE;
static int g_dma_TX_no_flush = 0;

mtk_uart_register uart_rg[HAL_UART_MAX];
mtk_apdma_uart_register apdma_rg[HAL_UART_MAX * 2];

#ifdef HAL_SLEEP_MANAGER_ENABLED
static bool g_uart_send_lock_status[HAL_UART_MAX];
static sleep_management_lock_request_t uart_sleep_handle = SLEEP_LOCK_UART;
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */


typedef struct {
    hal_uart_callback_t func;
    void *arg;
} uart_callback_t;


#ifdef FREERTOS_API
EventGroupHandle_t g_uart_event_sync = NULL;
#else /* #ifdef FREERTOS_API */
static uart_callback_t g_uart_callback[HAL_UART_MAX];
#endif /* #ifdef FREERTOS_API */


const uint32_t UART_BaseAddr[DRV_SUPPORT_UART_PORTS] = {
    UART0_BASE_ADDR,    //cm33 uart
    UART1_BASE_ADDR,    //top uart0
    UART2_BASE_ADDR,    //top uart1
    UART3_BASE_ADDR,    //dsp uart
};

const uint32_t UART_IRQ_NUM[DRV_SUPPORT_UART_PORTS] = {
    UART0_IRQ_ID,
    UART1_IRQ_ID,
    UART2_IRQ_ID,
    UART3_IRQ_ID,
};

/* UART runtime information */
mtk_uart_setting uart_setting[DRV_SUPPORT_UART_PORTS];


void __attribute__((noreturn)) AssertionFailed(const char *function, const char *file, int line)
{
    sys_print("!!! ASSERTION FAILED !!!\n"
              "[Func] %s\n"
              "[File] %s\n"
              "[Line] %d\n", function, file, line);
    while (1) ;
}

void __attribute__((noreturn)) NotImplemented(const char *function, char *file, int line)
{
    sys_print("!!! NOT IMPLEMENTED !!!\n"
              "[Func] %s\n"
              "[File] %s\n"
              "[Line] %d\n", function, file, line);
    while (1) ;
}

/* Internal RingBuffer Function */
void Buff_init(BUFFER_INFO *Buf, uint8_t *Buffaddr, uint16_t uTotalSize)
{
    Buf->Read = 0;
    Buf->Write = 0;
    Buf->Length = uTotalSize;
    Buf->CharBuffer = Buffaddr;
}

void Buff_Push(BUFFER_INFO *Buf, const uint8_t *pushData)
{
    if (Buff_IsFull(Buf))
        return;
    *BuffWrite(Buf) = *pushData;
    BWrite(Buf)++;
    if (BWrite(Buf) >= BLength(Buf))
        BWrite(Buf) -= BLength(Buf);
}

void Buff_Pop(BUFFER_INFO *Buf, uint8_t *popData)
{
    if (Buff_IsEmpty(Buf))
        return;
    *popData = *BuffRead(Buf);
    BRead(Buf)++;
    if (BRead(Buf) >= BLength(Buf))
        BRead(Buf) -= BLength(Buf);
}

uint8_t Buff_IsEmpty(BUFFER_INFO *Buf)
{
    uint8_t status;
    if (BRead(Buf) == BWrite(Buf))
        status = Buff_isEmpty;
    else
        status = Buff_notEmpty;
    return status;
}

uint8_t Buff_IsFull(BUFFER_INFO *Buf)
{
    uint8_t  status;
    uint16_t tmp = BRead(Buf);

    if (tmp == 0)
        tmp = BLength(Buf);
    if ((tmp - BWrite(Buf)) == 1)
        status = Buff_isFull;
    else
        status = Buff_notFull;
    return status;
}

uint16_t Buff_GetRoomLeft(BUFFER_INFO *Buf)
{
    uint16_t  RoomLeft = 0;

    if (BRead(Buf) <= BWrite(Buf))
        RoomLeft = BLength(Buf) - BWrite(Buf) + BRead(Buf) - 1;
    else
        RoomLeft = BRead(Buf) - BWrite(Buf) - 1;

    return RoomLeft;
}

uint16_t Buff_GetBytesAvail(BUFFER_INFO *Buf)
{
    uint16_t BytesAvail = 0;

    if (BWrite(Buf) >= BRead(Buf))
        BytesAvail = BWrite(Buf) - BRead(Buf);
    else
        BytesAvail = BLength(Buf) - BRead(Buf) + BWrite(Buf);

    return BytesAvail;
}

uint16_t Buff_GetLength(BUFFER_INFO *Buf)
{
    return Buf->Length;
}

void Buff_Flush(BUFFER_INFO *Buf)
{
    Buf->Write = Buf->Read = 0;
}

ATTR_TEXT_IN_SYSRAM int WriteDebugByte(uint8_t ch)
{
    uint32_t LSR;

    if (debug_uart_base == INVAL_UART_BASE)
        return -1;

    while (1) {
        LSR = INREG32(UART_LSR(debug_uart_base));
        if (LSR & UART_LSR_THRE) {
            OUTREG32(UART_THR(debug_uart_base), (uint32_t)ch);
            break;
        }
    }
    return (int)ch;
}

int ReadDebugByte(void)
{
    uint32_t ch;
    uint32_t LSR;

    if (debug_uart_base == INVAL_UART_BASE)
        return DEBUG_SERIAL_READ_NODATA;

    LSR = INREG32(UART_LSR(debug_uart_base));
    if (LSR & UART_LSR_DR)
        ch = (uint32_t)INREG32(UART_RBR(debug_uart_base));
    else
        ch = DEBUG_SERIAL_READ_NODATA;
    return (int)ch;
}

#ifdef __UART_VIRTUAL_FIFO__
static int dma_INT_TX_Cnt = 0;
static int dma_INT_RX_Cnt = 0;

/* Virtual FIFO runtime information ------------*/
VFIFO_ctrl_struct vff_ctrl[DRV_SUPPORT_VFF_CHANNEL];

/* Virtual FIFO static information init */
VFIFO_static_info vff_info[] = {
    // reg_base,                IRQQ_ID                         direction,             channel,          bind_uart_port
    {VFF_BASE_CH_0, VFF_UART0_TX_IRQ_ID,  DIR_TX,  VFF_UART0_TX_ID, UART_PORT0},
    {VFF_BASE_CH_1, VFF_UART0_RX_IRQ_ID,  DIR_RX,  VFF_UART0_RX_ID, UART_PORT0},
    {VFF_BASE_CH_2, VFF_UART1_TX_IRQ_ID,  DIR_TX,  VFF_UART1_TX_ID, UART_PORT1},
    {VFF_BASE_CH_3, VFF_UART1_RX_IRQ_ID,  DIR_RX,  VFF_UART1_RX_ID, UART_PORT1},
    {VFF_BASE_CH_4, VFF_UART2_TX_IRQ_ID,  DIR_TX,  VFF_UART2_TX_ID, UART_PORT2},
    {VFF_BASE_CH_5, VFF_UART2_RX_IRQ_ID,  DIR_RX,  VFF_UART2_RX_ID, UART_PORT2},
    {VFF_BASE_CH_6, VFF_UART3_TX_IRQ_ID,  DIR_TX,  VFF_UART3_TX_ID, UART_PORT3},
    {VFF_BASE_CH_7, VFF_UART3_RX_IRQ_ID,  DIR_RX,  VFF_UART3_RX_ID, UART_PORT3},

};


/*UART APDMA Function*/
void VFIFO_pop_byte(void *info, unsigned char *ch)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)info;
    unsigned int base = head->reg_base;
    unsigned int rpt = __IO_READ32__(VFF_RPT(base));
    unsigned int addr = (head->mem_base + (rpt & 0xffff));

    *ch = __IO_READ8__(addr);
    if ((rpt & 0xffff) == (__IO_READ32__(VFF_LEN(base)) - 1))
        __IO_WRITE32__(VFF_RPT(base), (~rpt) & 0x10000);
    else
        __IO_WRITE32__(VFF_RPT(base), rpt + 1);
}

unsigned int VFIFO_pop(void *info, UART_read_buffer_struct *buff)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)info;
    unsigned int base = head->reg_base;
    unsigned int rpt = __IO_READ32__(VFF_RPT(base));
    unsigned int addr = (head->mem_base + (rpt & 0xffff));
    unsigned int avail_size = VFIFO_GET_AVAIL(base);
    unsigned int len = __IO_READ32__(VFF_LEN(base));
    unsigned int packetbytes = 0;
    unsigned int bytesreceived = 0;
    packetbytes = avail_size;

    if ((rpt & 0xffff) + avail_size >= len) {
        bytesreceived = len - (rpt & 0xffff);
        memcpy(buff->read_buff + buff->offset, (unsigned char *)addr, bytesreceived);
        avail_size -= bytesreceived;
        /*revert bit 16 of RPT when wrapped to head again*/
        __IO_WRITE32__(VFF_RPT(base), (~rpt) & 0x10000);
        rpt = __IO_READ32__(VFF_RPT(base));
        addr = (head->mem_base + (rpt & 0xffff));
    }
    if (avail_size) {
        memcpy(buff->read_buff + buff->offset + bytesreceived, (unsigned char *)addr, avail_size);
        __IO_WRITE32__(VFF_RPT(base), rpt + avail_size);
    }
    buff->offset += packetbytes;

    return packetbytes;
}

void VFIFO_push_byte(void *info, unsigned char ch)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)info;
    unsigned int base = head->reg_base;
    unsigned int wpt = __IO_READ32__(VFF_WPT(base));
    unsigned int addr = (head->mem_base + (wpt & 0xffff));

    __IO_WRITE8__(addr, ch);
    if ((wpt & 0xffff) == (__IO_READ32__(VFF_LEN(base)) - 1))
        __IO_WRITE32__(VFF_WPT(base), (~wpt) & 0x10000);
    else
        __IO_WRITE32__(VFF_WPT(base), wpt + 1);
}

unsigned int VFIFO_flush_data(void *info)
{
    unsigned int base = ((VFIFO_ctrl_struct *)info)->reg_base;
    __IO_WRITE32__(VFF_FLUSH(base), VFF_FLUSH_B);
    return 0;
}

unsigned int VFIFO_int_en(void *info, unsigned char en)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)info;
    unsigned int base = head->reg_base;
    unsigned int int_en_flag;

    if (head->direction == DIR_TX)
        int_en_flag = VFF_TX_INT_EN_B;
    else
        int_en_flag = (VFF_RX_INT_EN0_B | VFF_RX_INT_EN1_B);

    if (en)
        DRV_WriteReg32(VFF_INT_EN(base), int_en_flag);
    else
        DRV_WriteReg32(VFF_INT_EN(base), VFF_TX_INT_FLAG_CLR_B);
    return 0;
}

#if 0
void VFIFO_rx_handler(void *data)
{
    //BaseType_t xResult;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    VFIFO_pop(data, g_uart_read_buf);
    xEventGroupSetBitsFromISR(g_uart_event_sync, BIT_REV, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif /* #if 0 */

void VFIFO_rx_handler(void *data)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)data;
    mtk_uart_setting  *uart_head = head->uart_ctrl_head;
    unsigned int base = head->reg_base;
    unsigned int room_left;
    unsigned char rx_data;
    unsigned int dropped = 0, index = 0;
    BUFFER_INFO *vff_buffer = &(uart_head->rx_buffer);
#ifdef FREERTOS_API
    BaseType_t xResult;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#else /* #ifdef FREERTOS_API */
    unsigned int uart_port = uart_head->port_id;
    hal_uart_callback_t callback;
    void *arg;
#endif /* #ifdef FREERTOS_API */

    room_left = Buff_GetRoomLeft(vff_buffer);

    if (!room_left) {
        while (1) {
            if (VFIFO_GET_AVAIL(base) > 0) {
                VFIFO_pop_byte(data, &rx_data);
                dropped++;
            } else {
                uart_head->dropped += dropped;
                return;
            }
        }
    }
    while (room_left) {
        if (VFIFO_GET_AVAIL(base) > 0) {
            VFIFO_pop_byte(data, &rx_data);
            Buff_Push(vff_buffer, &rx_data);
            index++;
            room_left--;
        } else
            break;
    }
#ifdef FREERTOS_API
    xResult = xEventGroupSetBitsFromISR(g_uart_event_sync, BIT_REV, &xHigherPriorityTaskWoken);
    if (xResult != pdFAIL)
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#else /* #ifdef FREERTOS_API */
    callback = g_uart_callback[uart_port].func;
    arg = g_uart_callback[uart_port].arg;
    if (callback == NULL)
        return;
    callback(HAL_UART_EVENT_READY_TO_READ, arg);
#endif /* #ifdef FREERTOS_API */
}

void VFIFO_tx_handler(void *data)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)data;
    mtk_uart_setting  *uart_head = head->uart_ctrl_head;
    unsigned int base = head->reg_base;
    unsigned int uart_base = uart_head->uart_reg_base;
    unsigned int  byteCount, real_count, index, vff_len, lsr;
    unsigned char TX_DATA;
    BUFFER_INFO *vff_buffer = &(uart_head->tx_buffer);

#ifdef FREERTOS_API
    BaseType_t xResult;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#else /* #ifdef FREERTOS_API */
    unsigned int uart_port = uart_head->port_id;
    hal_uart_callback_t callback;
    void *arg;
#endif /* #ifdef FREERTOS_API */

    vff_len = DRV_Reg32(VFF_LEN(base));
    byteCount = VFIFO_GET_LEFT(base);

    real_count = Buff_GetBytesAvail(vff_buffer);

    if (real_count > byteCount)
        real_count = byteCount;

    for (index = 0; index < real_count; index++) {
        Buff_Pop(vff_buffer, &TX_DATA);
        VFIFO_push_byte(data, TX_DATA);
    }

    if (!g_dma_TX_no_flush)
        VFIFO_flush_data(data);

    lsr = DRV_Reg32(UART_LSR(uart_base));
    if ((Buff_IsEmpty(vff_buffer) == TRUE) && (VFIFO_GET_LEFT(base) == vff_len) && (lsr & UART_LSR_TEMT)) {// Switch off TXD interrupt if TX buffer empty
        VFIFO_int_en(data, 0);
        DRV_WriteReg32(UART_IER(uart_base), IER_HW_NORMALINTS);
#ifdef HAL_SLEEP_MANAGER_ENABLED
        if (g_uart_send_lock_status[uart_port] == true) {
            if (hal_sleep_manager_is_sleep_handle_alive(uart_sleep_handle) == true) {
                hal_sleep_manager_unlock_sleep(uart_sleep_handle);
            }
            g_uart_send_lock_status[uart_port] = false;
        }
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

        callback = g_uart_callback[uart_port].func;
        arg = g_uart_callback[uart_port].arg;
        if (callback == NULL)
            return;
        callback(HAL_UART_EVENT_READY_TO_WRITE, arg);
    } else
        VFIFO_int_en(data, 1);
}

void VFIFO_irq_call_back(void *data)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)data;
    unsigned int base = head->reg_base;

    if (head->direction == DIR_RX) {
        dma_INT_RX_Cnt++;
        __IO_WRITE32__(VFF_INT_FLAG(base), VFF_RX_INT_FLAG_CLR_B);
        VFIFO_rx_handler(head);
    } else {
        dma_INT_TX_Cnt++;
        __IO_WRITE32__(VFF_INT_FLAG(base), VFF_TX_INT_FLAG_CLR_B);
        VFIFO_tx_handler(head);
    }
}


unsigned int VFIFO_start(void *info)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)info;
    unsigned int base = head->reg_base;

    DRV_WriteReg32(VFF_INT_FLAG(base), VFF_TX_INT_FLAG_CLR_B);

    if (head->direction == DIR_TX) {
        DRV_WriteReg32(VFF_WPT(base), 0);
        dma_INT_TX_Cnt = 0;
    } else {
        DRV_WriteReg32(VFF_RPT(base), 0);
        dma_INT_RX_Cnt = 0;
    }
    DRV_WriteReg32(VFF_EN(base), 1);
    return 0;
}

unsigned int VFIFO_stop(void *info)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)info;
    unsigned int base = head->reg_base;
    volatile unsigned int retry;

    if (head->direction == DIR_TX) {
        /* Set flush as 1 -> wait until flush is 0 */
        __IO_WRITE32__(VFF_FLUSH(base), VFF_FLUSH_B);
        retry = 10000000;
        while (__IO_READ32__(VFF_FLUSH(base)) && retry--);
    }

    /* Stop */
    retry = 10000000;
    __IO_WRITE32__(VFF_STOP(base), VFF_STOP_B);
    while (__IO_READ32__(VFF_EN(base)) && retry--);
    __IO_WRITE32__(VFF_STOP(base), VFF_STOP_CLR_B);

    /* Clear int en */
    __IO_WRITE32__(VFF_INT_EN(base), VFF_INT_EN_CLR_B);
    __IO_WRITE32__(VFF_INT_FLAG(base), VFF_TX_INT_FLAG_CLR_B);

    //dbg_print("VFF_UART_RX_ID with DMA TX INT:%d, DMA RX INT:%d!\n", dma_INT_TX_Cnt, dma_INT_RX_Cnt);
    return 0;
}

unsigned int VFIFO_get_free_space(void *info)
{
    return VFIFO_GET_LEFT(((VFIFO_ctrl_struct *)info)->reg_base);
}

unsigned int VFIFO_get_data_num(void *info)
{
    return VFIFO_GET_AVAIL(((VFIFO_ctrl_struct *)info)->reg_base);
}

unsigned int VFIFO_is_full(void *info)
{
    return (VFIFO_GET_LEFT(((VFIFO_ctrl_struct *)info)->reg_base) == 0);
}

unsigned int VFIFO_is_empty(void *info)
{
    return (VFIFO_GET_AVAIL(((VFIFO_ctrl_struct *)info)->reg_base) == 0);
}

unsigned int VFIFO_config(void *info, unsigned int addr, unsigned int len, unsigned int thre)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)info;
    unsigned int base = head->reg_base;

    head->mem_base = addr;
    memset((void *)addr, 0x5A, len);
    DRV_WriteReg32(VFF_ADDR(base), addr);
    DRV_WriteReg32(VFF_LEN(base),  len);
    DRV_WriteReg32(VFF_THRE(base), thre);

    if (head->direction == DIR_RX)
        DRV_WriteReg32(VFF_RX_FLOWCTL_THRE(base), len - 2);//only for VFF_RX : FLOW_CTRL
    return 0;
}

static int VFIFO_get_channel_from_irq_number(hal_nvic_irq_t irq_number)
{
    /* 7933: only top uart port support DMA mode */
    switch (irq_number) {
        case VFF_UART1_TX_IRQ_ID:
            return VFF_UART1_TX_ID;
        case VFF_UART1_RX_IRQ_ID:
            return VFF_UART1_RX_ID;
        case VFF_UART2_TX_IRQ_ID:
            return VFF_UART2_TX_ID;
        case VFF_UART2_RX_IRQ_ID:
            return VFF_UART2_RX_ID;
        default:
            return HAL_UART_STATUS_ERROR_PARAMETER;
    }
}

void VFIFO_interrupt_handler(hal_nvic_irq_t irq_number)
{
    int channel = VFIFO_get_channel_from_irq_number(irq_number);
    if (channel != HAL_UART_STATUS_ERROR_PARAMETER)
        VFIFO_irq_call_back(&vff_ctrl[channel]);
}

unsigned int VFIFO_prepare_Intr(void *info)
{
    VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)info;
    unsigned int irq_num = head->irq_id;
    hal_nvic_disable_irq(irq_num);
    hal_nvic_register_isr_handler(irq_num, VFIFO_interrupt_handler);
    hal_nvic_irq_set_type(irq_num, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    hal_nvic_enable_irq(irq_num);
    return 0;
}


#endif /* #ifdef __UART_VIRTUAL_FIFO__ */


void __uart_clr_buffer(mtk_uart_setting *u_setting)
{
    __IO_WRITE32__(UART_FCR(u_setting->uart_reg_base), UART_FCR_NORMAL);
}

void __uart_close(mtk_uart_setting *u_setting)
{
    uint32_t uart_base = u_setting->uart_reg_base;

    DRV_WriteReg32(UART_IER(uart_base), UART_IER_ALLOFF);
    __uart_clr_buffer(u_setting);
}

static unsigned int uart_get_baudrate_from_enum(hal_uart_baudrate_t baud)
{
    switch (baud) {
        case HAL_UART_BAUDRATE_110:
            return 110;
        case HAL_UART_BAUDRATE_300:
            return 300;
        case HAL_UART_BAUDRATE_1200:
            return 1200;
        case HAL_UART_BAUDRATE_2400:
            return 2400;
        case HAL_UART_BAUDRATE_4800:
            return 4800;
        case HAL_UART_BAUDRATE_9600:
            return 9600;
        case HAL_UART_BAUDRATE_19200:
            return 19200;
        case HAL_UART_BAUDRATE_38400:
            return 38400;
        case HAL_UART_BAUDRATE_57600:
            return 57600;
        case HAL_UART_BAUDRATE_115200:
            return 115200;
        case HAL_UART_BAUDRATE_230400:
            return 230400;
        case HAL_UART_BAUDRATE_460800:
            return 460800;
        case HAL_UART_BAUDRATE_921600:
            return 921600;
#ifdef HAL_UART_FEATURE_3M_BAUDRATE
        case HAL_UART_BAUDRATE_3000000:
            return 3000000;
#endif /* #ifdef HAL_UART_FEATURE_3M_BAUDRATE */
        default:
            return HAL_UART_STATUS_ERROR_PARAMETER;
    }
}

void __uart_receive_handler(void *parameter)
{
    uint32_t RoomLeft;
    uint32_t RX_DATA;
    uint8_t  cRXChar;
    uint32_t LSR;
    uint32_t dropped = 0;
    mtk_uart_setting *UARTData = (mtk_uart_setting *)parameter;
    uint32_t uart_base = UARTData->uart_reg_base;
    BUFFER_INFO *uart_buf = &(UARTData->rx_buffer);
#ifdef FREERTOS_API
    BaseType_t xResult;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#else /* #ifdef FREERTOS_API */
    unsigned int uart_port = UARTData->port_id;
    hal_uart_callback_t callback;
    void *arg;
#endif /* #ifdef FREERTOS_API */

    RoomLeft = Buff_GetRoomLeft(uart_buf);
    if (!RoomLeft) { /* drop data */
        while (1) {
            LSR = DRV_Reg32(UART_LSR(uart_base));
            if (LSR & UART_LSR_DR)
                dropped++;
            else {
                UARTData->dropped += dropped;
                return;
            }
        }
    }
    while (RoomLeft) {
        LSR = DRV_Reg32(UART_LSR(uart_base));
        if (LSR & 0x2)
            log_hal_error("OE\r\n");

        if (LSR & UART_LSR_DR) {
            RX_DATA = DRV_Reg32(UART_RBR(uart_base));
            cRXChar = (uint8_t)RX_DATA;
            Buff_Push(uart_buf, &cRXChar);
            RoomLeft--;
        } else
            break;
    }
#ifdef FREERTOS_API
    xResult = xEventGroupSetBitsFromISR(g_uart_event_sync, BIT_REV, &xHigherPriorityTaskWoken);
    if (xResult != pdFAIL)
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#else /* #ifdef FREERTOS_API */
    callback = g_uart_callback[uart_port].func;
    arg = g_uart_callback[uart_port].arg;
    if (callback == NULL)
        return;
    callback(HAL_UART_EVENT_READY_TO_READ, arg);
#endif /* #ifdef FREERTOS_API */

}

void __uart_transmit_handler(void *parameter)
{
    uint32_t  byteCount, real_count, index, lsr;
    uint8_t   TX_DATA;
    mtk_uart_setting *UARTData = (mtk_uart_setting *)parameter;
    uint32_t uart_base = UARTData->uart_reg_base;
    BUFFER_INFO *uart_buf = &(UARTData->tx_buffer);
    unsigned int uart_port = UARTData->port_id;
    hal_uart_callback_t callback;
    void *arg;

    byteCount = UART_TX_FIFO_LENGTH - UART_TX_THRESHOLD_SETTING;//28 btye

    real_count = Buff_GetBytesAvail(uart_buf);
    if (real_count > byteCount)
        real_count = byteCount;

    for (index = 0; index < real_count; index++) {
        Buff_Pop(uart_buf, &TX_DATA);
        DRV_WriteReg32(UART_THR(uart_base), (uint32_t)TX_DATA);
    }

    lsr = DRV_Reg32(UART_LSR(uart_base));
    if (Buff_IsEmpty(uart_buf) == TRUE && (lsr & UART_LSR_TEMT)) { // Switch off TXD interrupt if TX buffer empty
        if (uart_console(uart_port))
            DRV_WriteReg32(UART_IER(uart_base), UART_IER_ERBFI);
        else
            DRV_WriteReg32(UART_IER(uart_base), IER_HW_NORMALINTS);
#ifdef HAL_SLEEP_MANAGER_ENABLED
        if (g_uart_send_lock_status[uart_port] == true) {
            if (hal_sleep_manager_is_sleep_handle_alive(uart_sleep_handle) == true) {
                hal_sleep_manager_unlock_sleep(uart_sleep_handle);
            }
            g_uart_send_lock_status[uart_port] = false;
        }
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
        callback = g_uart_callback[uart_port].func;
        arg = g_uart_callback[uart_port].arg;
        if (callback == NULL)
            return;
        callback(HAL_UART_EVENT_READY_TO_WRITE, arg);
    } else {
        if (uart_console(uart_port))
            DRV_WriteReg32(UART_IER(uart_base), UART_IER_ERBFI | UART_IER_ETBEI);
        else
            DRV_WriteReg32(UART_IER(uart_base), IER_HW_ALLINTS);
    }
}

void __uart_ls_handler(void *parameter)
{
    uint32_t LSR;
    mtk_uart_setting *UARTData = (mtk_uart_setting *)parameter;
    uint32_t uart_base = UARTData->uart_reg_base;
    unsigned int uart_port = UARTData->port_id;
    hal_uart_callback_t callback;
    void *arg;

    LSR = DRV_Reg32(UART_LSR(uart_base));
    log_hal_error("ls handler,lsr: %d\n", LSR);

    if (LSR & (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE)) {
        callback = g_uart_callback[uart_port].func;
        arg = g_uart_callback[uart_port].arg;
        if (callback == NULL)
            return;
        callback(HAL_UART_EVENT_TRANSACTION_ERROR, arg);
    }
}

void __uart_ms_handler(void *parameter)
{
}

void __uart_irq_callback(void *parameter)
{
    uint32_t IIR;
    mtk_uart_setting *UARTData;
    uint32_t uart_base, mask;
    UARTData = (mtk_uart_setting *)parameter;
    uart_base = UARTData->uart_reg_base;
    /*ensure this section not to be interrupt*/
    mask = save_and_set_interrupt_mask();

    IIR = DRV_Reg32(UART_IIR(uart_base));
    if (IIR & UART_IIR_INT_INVALID) {
        restore_interrupt_mask(mask);
        return;
    }

    switch (IIR & UART_IIR_INT_MASK) {
        case UART_IIR_HWFlowCtrl:
            break;

        case UART_IIR_RLS:
            __uart_ls_handler(parameter);
            break;

        case UART_IIR_CTI:
            __uart_receive_handler(parameter);
            break;

        case UART_IIR_RDA:
            __uart_receive_handler(parameter);
            break;

        case UART_IIR_THRE:
            __uart_transmit_handler(parameter);
            break;

        case UART_IIR_MS:
            __uart_ms_handler(parameter);
            break;
        default:
            break;
    }
    restore_interrupt_mask(mask);
}

void __uart_baudrate_config(unsigned int addr, unsigned int baudrate, unsigned int uartclk)
{
    int highspeed, quot, fraction;
    int sample, sample_count, sample_point;
    int dll, dlh;
    unsigned short fraction_L_mapping[] = { 0, 1, 0x5, 0x15, 0x55, 0x57, 0x57, 0x77, 0x7F, 0xFF, 0xFF };
    unsigned short fraction_M_mapping[] = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 3 };

    if (baudrate < HAL_UART_BAUDRATE_MAX)
        baudrate = uart_get_baudrate_from_enum(baudrate);
    /* set baud rate */
    if (baudrate < 115200) {
        highspeed = 0;
        quot = DIV_ROUND_CLOSEST(uartclk, 16 * baudrate);
    } else {
        highspeed = 3;
        //quot = DIV_ROUND_UP(uartclk, 256 * baudrate);
        quot = DIV_EXACTLY(uartclk, 256 * baudrate) + 1;
    }

    sample = DIV_EXACTLY(uartclk, quot * baudrate);
    //sample = DIV_ROUND_CLOSEST(uartclk, quot * baudrate);
    sample_count = sample - 1;
    sample_point = (sample_count >> 1) - 1;
    dll = quot & 0xff;
    dlh = quot >> 8;

#if UART_NEW_FEATURE_SWITCH_OPTION
    DRV_WriteReg32(UART_FEATURE_SEL(addr), UART_FEATURE_SEL_NEW_MAP | UART_FEATURE_SEL_NO_DOWNLOAD);
#else /* #if UART_NEW_FEATURE_SWITCH_OPTION */
    DRV_WriteReg32(UART_FEATURE_SEL(addr), 0x80); //Disable download mode
#endif /* #if UART_NEW_FEATURE_SWITCH_OPTION */
    DRV_WriteReg32(UART_HIGHSPEED(addr), highspeed);

    DRV_WriteReg32(UART_LCR(addr), UART_LCR_DLAB); // set LCR to DLAB to set DLL,DLH
    DRV_WriteReg32(UART_DLL(addr), dll);
    DRV_WriteReg32(UART_DLH(addr), dlh);
    DRV_WriteReg32(UART_LCR(addr), UART_WLS_8); //word length 8

    if (baudrate >= 115200) {
        DRV_WriteReg32(UART_SAMPLE_COUNT(addr), sample_count);
        DRV_WriteReg32(UART_SAMPLE_POINT(addr), sample_point);

        /*count fraction to set fractoin register */
        fraction = ((uartclk  * 100) / baudrate / quot) % 100;
        fraction = DIV_ROUND_CLOSEST(fraction, 10);
        DRV_WriteReg32(UART_FRACDIV_L(addr), fraction_L_mapping[fraction]);
        DRV_WriteReg32(UART_FRACDIV_M(addr), fraction_M_mapping[fraction]);
    } else {
        DRV_WriteReg32(UART_SAMPLE_COUNT(addr), 0x00);
        DRV_WriteReg32(UART_SAMPLE_POINT(addr), 0xff);
        DRV_WriteReg32(UART_FRACDIV_L(addr), 0x00);
        DRV_WriteReg32(UART_FRACDIV_M(addr), 0x00);
    }

    DRV_WriteReg32(UART_FCR(addr), UART_FCR_NORMAL); //Enable FIFO

    //log_hal_info("%x: dump baudrate config, baud:%d, HIGHSPEED:%d, DLL:%d, DLH:%d, sample_count:%d, sample_point:%d, fcr:%d\n",
    //    addr, baudrate, highspeed, dll, dlh, sample_count, sample_point, DRV_Reg32(UART_FCR_RD(addr)));
}


static void __uart_set_baud_rate(mtk_uart_setting *u_setting, uint32_t baudrate)
{
    uint32_t    uart_base = u_setting->uart_reg_base;
    uint32_t    uart_clock = 0;
    uart_clock = UART_CLOCK_RATE;
    __uart_baudrate_config(uart_base, baudrate, uart_clock);
}

static void __uart_set_dcb_config(mtk_uart_setting *u_setting, UART_dcb_struct *UART_Config)
{
    uint32_t   byte;
    uint32_t   IER;
    uint32_t   uart_base = u_setting->uart_reg_base;

    IER = DRV_Reg32(UART_IER(uart_base));
    DRV_WriteReg32(UART_IER(uart_base), UART_IER_ALLOFF);
    __uart_set_baud_rate(u_setting, UART_Config->baud);
    byte = DRV_Reg32(UART_LCR(uart_base)); /* DLAB start */

    byte &= ~UART_WLS_MASK;
    switch (UART_Config->dataBits) {
        case len_5:
            byte |= UART_WLS_5;
            break;

        case len_6:
            byte |= UART_WLS_6;
            break;

        case len_7:
            byte |= UART_WLS_7;
            break;

        case len_8:
            byte |= UART_WLS_8;
            break;

        default:
            break;
    }
    byte &= ~UART_STOP_MASK;
    switch (UART_Config->stopBits) {
        case sb_1:
            byte |= UART_1_STOP;
            break;

        case sb_2:
            byte |= UART_2_STOP;
            break;

        case sb_1_5:
            byte |= UART_1_5_STOP;
            break;

        default:
            break;
    }

    byte &= ~UART_PARITY_MASK;
    switch (UART_Config->parity) {
        case pa_none:
            byte |= UART_NONE_PARITY;
            break;

        case pa_odd:
            byte |= UART_ODD_PARITY;
            break;

        case pa_even:
            byte |= UART_EVEN_PARITY;
            break;

        case pa_space:
            byte |= UART_SPACE_PARITY;
            break;

        default:
            break;
    }
    DRV_WriteReg32(UART_LCR(uart_base), byte);           /* DLAB End */

    /* flowControl */
    byte = DRV_Reg32(UART_LCR(uart_base));
    DRV_WriteReg32(UART_LCR(uart_base), 0xbf);           /* Enchance setting */
    DRV_WriteReg32(UART_ESCAPE_EN(uart_base), 0x0);
    switch (UART_Config->flowControl) {
        case fc_none:
            DRV_WriteReg32(UART_EFR(uart_base), UART_EFR_ALLOFF);
            break;

        case fc_hw:
            DRV_WriteReg32(UART_EFR(uart_base), UART_EFR_AutoRTSCTS | UART_EFR_Enchance);
            IER |= (UART_IER_RTSI | UART_IER_CTSI);
            break;

        case fc_sw:
            DRV_WriteReg32(UART_EFR(uart_base), UART_EFR_SWFlowCtrlX1);
            DRV_WriteReg32(UART_ESCAPE_EN(uart_base), 0x0);
            break;
        case fc_sw_mtk:
            DRV_WriteReg32(UART_EFR(uart_base), UART_EFR_SWFlowCtrlX1);
            DRV_WriteReg32(UART_ESCAPE_DAT(uart_base), UART_Config->escapeChar);
            DRV_WriteReg32(UART_ESCAPE_EN(uart_base), 0x1);
            break;

        default:
            break;
    }

    DRV_WriteReg32(UART_XON1(uart_base), UART_Config->xonChar);
    DRV_WriteReg32(UART_XOFF1(uart_base), UART_Config->xoffChar);
    DRV_WriteReg32(UART_XON2(uart_base), UART_Config->xon1Char);
    DRV_WriteReg32(UART_XOFF2(uart_base), UART_Config->xoff1Char);
    DRV_WriteReg32(UART_LCR(uart_base), byte);         /* DLAB End */

    memcpy(&(u_setting->dcb), UART_Config, sizeof(UART_dcb_struct));
    __IO_WRITE32__(UART_IER(uart_base), IER); // Restore IER

}

static void __uart_config(uint32_t port, UART_dcb_struct *UART_Config)
{
    __uart_set_dcb_config(&(uart_setting[port]), UART_Config);
}

static void __uart_set_flowControl(uint32_t port, UART_dcb_struct *UART_Config)
{
    uint32_t   byte;
    uint32_t   IER;
    mtk_uart_setting *u_setting = &(uart_setting[port]);
    UART_dcb_struct *UART_Src_Config = &(u_setting->dcb);
    uint32_t   uart_base = u_setting->uart_reg_base;

    IER = DRV_Reg32(UART_IER(uart_base));
    DRV_WriteReg32(UART_IER(uart_base), UART_IER_ALLOFF);

    /* flowControl */
    byte = DRV_Reg32(UART_LCR(uart_base));
    DRV_WriteReg32(UART_LCR(uart_base), 0xbf);           /* Enchance setting */
    DRV_WriteReg32(UART_ESCAPE_EN(uart_base), 0x0);
    switch (UART_Config->flowControl) {
        case fc_none:
            DRV_WriteReg32(UART_EFR(uart_base), UART_EFR_ALLOFF);
            break;
        case fc_hw:
            DRV_WriteReg32(UART_EFR(uart_base), UART_EFR_AutoRTSCTS | UART_EFR_Enchance);
            IER |= (UART_IER_RTSI | UART_IER_CTSI);
            break;
        case fc_sw:
            DRV_WriteReg32(UART_EFR(uart_base), UART_EFR_SWFlowCtrlX1);
            DRV_WriteReg32(UART_ESCAPE_EN(uart_base), 0x0);
            break;
        case fc_sw_mtk:
            DRV_WriteReg32(UART_EFR(uart_base), UART_EFR_SWFlowCtrlX1);
            DRV_WriteReg32(UART_ESCAPE_DAT(uart_base), UART_Config->escapeChar);
            DRV_WriteReg32(UART_ESCAPE_EN(uart_base), 0x1);
            break;
        default:
            break;
    }

    DRV_WriteReg32(UART_XON1(uart_base), UART_Config->xonChar);
    DRV_WriteReg32(UART_XOFF1(uart_base), UART_Config->xoffChar);
    DRV_WriteReg32(UART_XON2(uart_base), UART_Config->xon1Char);
    DRV_WriteReg32(UART_XOFF2(uart_base), UART_Config->xoff1Char);
    DRV_WriteReg32(UART_LCR(uart_base), byte);         /* DLAB End */

    UART_Src_Config->flowControl = UART_Config->flowControl;
    UART_Src_Config->xonChar = UART_Config->xonChar;
    UART_Src_Config->xoffChar = UART_Config->xoffChar;
    UART_Src_Config->xon1Char = UART_Config->xon1Char;
    UART_Src_Config->xoff1Char = UART_Config->xoff1Char;
    UART_Src_Config->escapeChar = UART_Config->escapeChar;

    __IO_WRITE32__(UART_IER(uart_base), IER); // Restore IER

}

static unsigned int uart_get_irq_number_from_port(hal_uart_port_t port)
{
    switch (port) {
        case HAL_UART_0:
            return UART_IRQn;
        case HAL_UART_1:
            return TOP_UART0_IRQn;
        case HAL_UART_2:
            return TOP_UART1_IRQn;
        case HAL_UART_3:
            return DSP_UART_IRQn;
        default:
            return HAL_UART_STATUS_ERROR_PARAMETER;
    }
}

static int uart_get_port_from_irq_number(hal_nvic_irq_t irq_number)
{
    switch (irq_number) {
        case UART_IRQn:
            return HAL_UART_0;
        case TOP_UART0_IRQn:
            return HAL_UART_1;
        case TOP_UART1_IRQn:
            return HAL_UART_2;
        case DSP_UART_IRQn:
            return HAL_UART_3;
        default:
            return HAL_UART_STATUS_ERROR_PARAMETER;
    }
}

static uint32_t uart_get_baud_from_enum(hal_uart_baudrate_t baudrate)
{
    switch (baudrate) {
        case HAL_UART_BAUDRATE_110:
            return 110;
        case HAL_UART_BAUDRATE_300:
            return 300;
        case HAL_UART_BAUDRATE_1200:
            return 1200;
        case HAL_UART_BAUDRATE_2400:
            return 2400;
        case HAL_UART_BAUDRATE_4800:
            return 4800;
        case HAL_UART_BAUDRATE_9600:
            return 9600;
        case HAL_UART_BAUDRATE_19200:
            return 19200;
        case HAL_UART_BAUDRATE_38400:
            return 38400;
        case HAL_UART_BAUDRATE_57600:
            return 57600;
        case HAL_UART_BAUDRATE_115200:
            return 115200;
        case HAL_UART_BAUDRATE_230400:
            return 230400;
        case HAL_UART_BAUDRATE_460800:
            return 460800;
        case HAL_UART_BAUDRATE_921600:
            return 921600;
#ifdef HAL_UART_FEATURE_3M_BAUDRATE
        case HAL_UART_BAUDRATE_3000000:
            return 3000000;
#endif /* #ifdef HAL_UART_FEATURE_3M_BAUDRATE */
        default:
            return HAL_UART_STATUS_ERROR_PARAMETER;
    }
}


void uart_interrupt_handler(hal_nvic_irq_t irq_number)
{
    int port = uart_get_port_from_irq_number(irq_number);
    if (port != HAL_UART_STATUS_ERROR_PARAMETER)
        __uart_irq_callback(&uart_setting[port]);
}

void uart_nvic_register(hal_uart_port_t port)
{
    unsigned int irq = uart_get_irq_number_from_port(port);
    hal_nvic_disable_irq(irq);
    hal_nvic_register_isr_handler(irq, uart_interrupt_handler);
    hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    hal_nvic_enable_irq(irq);
}


static void __uart_hw_init(hal_uart_port_t port, mtk_uart_setting *u_setting)
{
    uint32_t uart_base;
    u_setting->uart_reg_base = UART_BaseAddr[port];
    u_setting->uart_irq_id = UART_IRQ_NUM[port];
    u_setting->port_id = port;

    uart_base = u_setting->uart_reg_base;
    DRV_WriteReg32(UART_MCR(uart_base), UART_MCR_Normal);
    DRV_WriteReg32(UART_IER(uart_base), UART_IER_ALLOFF);
    uart_nvic_register(port);
    u_setting->use_vfifo = 0;
    u_setting->initialized = TRUE;
}

static uint32_t uart_open(uint32_t port)
{
    mtk_uart_setting *u_setting = &(uart_setting[port]);
    if (u_setting->opened)
        return FALSE;
    uint32_t uart_base = u_setting->uart_reg_base;
    u_setting->opened = TRUE;
    u_setting->dropped = 0;
    DRV_WriteReg32(UART_FCR(uart_base), UART_FCR_NORMAL);
    if (uart_console(port))
        DRV_WriteReg32(UART_IER(uart_base), UART_IER_ERBFI);
    else
        DRV_WriteReg32(UART_IER(uart_base), IER_HW_NORMALINTS);

    return TRUE;
}

static void uart_init(hal_uart_port_t port)
{
    if (port >= HAL_UART_MAX || port < HAL_UART_0)
        return;

    /* uart internal ring buffer */
    uint8_t *uart_rx = (uint8_t *)malloc(UART_RING_BUFF_LEN);
    if (!uart_rx)
        return;

    uint8_t *uart_tx = (uint8_t *)malloc(UART_RING_BUFF_LEN);
    if (!uart_tx) {
        free(uart_rx);
        return;
    }

    memset(&uart_setting[port], 0x0, sizeof(uart_setting[port]));
    Buff_init(&(uart_setting[port].rx_buffer), uart_rx, UART_RING_BUFF_LEN);
    Buff_init(&(uart_setting[port].tx_buffer), uart_tx, UART_RING_BUFF_LEN);

    __uart_hw_init(port, &(uart_setting[port]));
    return;
}

#ifdef HAL_SLEEP_MANAGER_ENABLED
void uart_register_backup(hal_uart_port_t uart_port)
{
    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0)
        return;

    mtk_uart_register *reg = &uart_rg[uart_port];
    uint32_t base = uart_setting[uart_port].uart_reg_base;

    /* save when LCR = 0xBF */
    reg->lcr = DRV_Reg32(UART_LCR(base));
    DRV_WriteReg32(UART_LCR(base), 0xBF);
    reg->efr = DRV_Reg32(UART_EFR(base));
    DRV_WriteReg32(UART_LCR(base), reg->lcr);

    if (!uart_console(uart_port))
        reg->fcr = DRV_Reg32(UART_FCR_RD(base));

    /*save baudrate */
    reg->highspeed = DRV_Reg32(UART_HIGHSPEED(base));
    reg->fracdiv_l = DRV_Reg32(UART_FRACDIV_L(base));
    reg->fracdiv_m = DRV_Reg32(UART_FRACDIV_M(base));
    DRV_WriteReg32(UART_LCR(base), reg->lcr | UART_LCR_DLAB);
    reg->dll = DRV_Reg32(UART_DLL(base));
    reg->dlm = DRV_Reg32(UART_DLH(base));
    DRV_WriteReg32(UART_LCR(base), reg->lcr);
    reg->sample_count = DRV_Reg32(UART_SAMPLE_COUNT(base));
    reg->sample_point = DRV_Reg32(UART_SAMPLE_POINT(base));
    reg->guard = DRV_Reg32(UART_GUARD(base));

    /* save flow control */
    reg->mcr = DRV_Reg32(UART_MCR(base));
    reg->ier = DRV_Reg32(UART_IER(base));
    reg->lcr = DRV_Reg32(UART_LCR(base));
    DRV_WriteReg32(UART_LCR(base), 0xBF);
    reg->xon1 = DRV_Reg32(UART_XON1(base));
    reg->xon2 = DRV_Reg32(UART_XON2(base));
    reg->xoff1 = DRV_Reg32(UART_XOFF1(base));
    reg->xoff2 = DRV_Reg32(UART_XOFF2(base));
    DRV_WriteReg32(UART_LCR(base), reg->lcr);
    reg->escape_dat = DRV_Reg32(UART_ESCAPE_DAT(base));
    reg->sleep_en = DRV_Reg32(UART_SLEEP_EN(base));

    /* save others */
    reg->escape_en = DRV_Reg32(UART_ESCAPE_EN(base));
    reg->msr = DRV_Reg32(UART_MSR(base));
    reg->scr = DRV_Reg32(UART_SCR(base));
    reg->dma_en = DRV_Reg32(UART_DMA_EN(base));
    reg->rxtri_ad = DRV_Reg32(UART_RXTRI_AD(base));

    if (uart_setting[uart_port].init_flag & VFIFO_TX_ENABLE_FLAG) {
        uint32_t tx_base = uart_setting[uart_port].tx_dma_reg_base;
        mtk_apdma_uart_register *tx_rg = &apdma_rg[uart_port];

        tx_rg->vff_int_flag = DRV_Reg32(VFF_INT_FLAG(tx_base));
        tx_rg->vff_int_en = DRV_Reg32(VFF_INT_EN(tx_base));
        tx_rg->vff_addr = DRV_Reg32(VFF_ADDR(tx_base));
        tx_rg->vff_len = DRV_Reg32(VFF_LEN(tx_base));
        tx_rg->vff_thre = DRV_Reg32(VFF_THRE(tx_base));
        tx_rg->vff_en = DRV_Reg32(VFF_EN(tx_base));
    }

    if (uart_setting[uart_port].init_flag & VFIFO_RX_ENABLE_FLAG) {
        uint32_t rx_base = uart_setting[uart_port].rx_dma_reg_base;
        mtk_apdma_uart_register *rx_rg = &apdma_rg[uart_port + 1];

        rx_rg->vff_int_flag = DRV_Reg32(VFF_INT_FLAG(rx_base));
        rx_rg->vff_int_en = DRV_Reg32(VFF_INT_EN(rx_base));
        rx_rg->vff_addr = DRV_Reg32(VFF_ADDR(rx_base));
        rx_rg->vff_len = DRV_Reg32(VFF_LEN(rx_base));
        rx_rg->vff_thre = DRV_Reg32(VFF_THRE(rx_base));
        rx_rg->vff_rx_fc = DRV_Reg32(VFF_RX_FLOWCTL_THRE(rx_base));
        rx_rg->vff_en = DRV_Reg32(VFF_EN(rx_base));
    }
}


void uart_register_restore(hal_uart_port_t uart_port)
{
    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0)
        return;

    mtk_uart_register *reg = &uart_rg[uart_port];
    uint32_t base = uart_setting[uart_port].uart_reg_base;

    /* restore when LCR = 0xBF */
    DRV_WriteReg32(UART_LCR(base), 0xBF);
    DRV_WriteReg32(UART_EFR(base), reg->efr);
    DRV_WriteReg32(UART_LCR(base), reg->lcr);

    if (!uart_console(uart_port))
        DRV_WriteReg32(UART_FCR(base), reg->fcr);

    /*restore baudrate */
    DRV_WriteReg32(UART_HIGHSPEED(base), reg->highspeed);
    DRV_WriteReg32(UART_FRACDIV_L(base), reg->fracdiv_l);
    DRV_WriteReg32(UART_FRACDIV_M(base), reg->fracdiv_m);
    DRV_WriteReg32(UART_LCR(base), reg->lcr | UART_LCR_DLAB);
    DRV_WriteReg32(UART_DLL(base), reg->dll);
    DRV_WriteReg32(UART_DLH(base), reg->dlm);
    DRV_WriteReg32(UART_LCR(base), reg->lcr);
    DRV_WriteReg32(UART_SAMPLE_COUNT(base), reg->sample_count);
    DRV_WriteReg32(UART_SAMPLE_POINT(base), reg->sample_point);
    DRV_WriteReg32(UART_GUARD(base), reg->guard);

    /* restore flow control */
    DRV_WriteReg32(UART_MCR(base), reg->mcr);
    DRV_WriteReg32(UART_IER(base), reg->ier);
    DRV_WriteReg32(UART_LCR(base), 0xBF);
    DRV_WriteReg32(UART_XON1(base), reg->xon1);
    DRV_WriteReg32(UART_XON2(base), reg->xon2);
    DRV_WriteReg32(UART_XOFF1(base), reg->xoff1);
    DRV_WriteReg32(UART_XOFF2(base), reg->xoff2);
    DRV_WriteReg32(UART_LCR(base), reg->lcr);
    DRV_WriteReg32(UART_ESCAPE_DAT(base), reg->escape_dat);
    DRV_WriteReg32(UART_SLEEP_EN(base), reg->sleep_en);

    /* restore others */
    DRV_WriteReg32(UART_ESCAPE_EN(base), reg->escape_en);
    DRV_WriteReg32(UART_MSR(base), reg->msr);
    DRV_WriteReg32(UART_SCR(base), reg->scr);

    DRV_WriteReg32(UART_DMA_EN(base), reg->dma_en);
    DRV_WriteReg32(UART_RXTRI_AD(base), reg->rxtri_ad);

    if (uart_setting[uart_port].init_flag & VFIFO_TX_ENABLE_FLAG) {
        uint32_t tx_base = uart_setting[uart_port].tx_dma_reg_base;
        uint32_t rst;
        mtk_apdma_uart_register *tx_rg = &apdma_rg[uart_port];

        DRV_WriteReg32(VFF_RST(tx_base), DMA_WARM_RST);
        do {
            rst = DRV_Reg32(VFF_RST(tx_base));
        } while (rst & DMA_WARM_RST);
        DRV_WriteReg32(VFF_WPT(tx_base), 0);
        DRV_WriteReg32(VFF_INT_FLAG(tx_base), tx_rg->vff_int_flag);
        DRV_WriteReg32(VFF_ADDR(tx_base), tx_rg->vff_addr);
        DRV_WriteReg32(VFF_LEN(tx_base), tx_rg->vff_len);
        DRV_WriteReg32(VFF_THRE(tx_base), tx_rg->vff_thre);
        DRV_WriteReg32(VFF_INT_EN(tx_base), tx_rg->vff_int_en);
        DRV_WriteReg32(VFF_EN(tx_base), tx_rg->vff_en);
    }

    if (uart_setting[uart_port].init_flag & VFIFO_RX_ENABLE_FLAG) {
        uint32_t rx_base = uart_setting[uart_port].rx_dma_reg_base;
        uint32_t rst;
        mtk_apdma_uart_register *rx_rg = &apdma_rg[uart_port + 1];

        DRV_WriteReg32(VFF_RST(rx_base), DMA_WARM_RST);
        do {
            rst = DRV_Reg32(VFF_RST(rx_base));
        } while (rst & DMA_WARM_RST);
        DRV_WriteReg32(VFF_RPT(rx_base), 0);
        DRV_WriteReg32(VFF_INT_FLAG(rx_base), rx_rg->vff_int_flag);
        DRV_WriteReg32(VFF_ADDR(rx_base), rx_rg->vff_addr);
        DRV_WriteReg32(VFF_LEN(rx_base), rx_rg->vff_len);
        DRV_WriteReg32(VFF_THRE(rx_base), rx_rg->vff_thre);
        DRV_WriteReg32(VFF_RX_FLOWCTL_THRE(rx_base), rx_rg->vff_rx_fc);
        DRV_WriteReg32(VFF_INT_EN(rx_base), rx_rg->vff_int_en);
        DRV_WriteReg32(VFF_EN(rx_base), rx_rg->vff_en);
    }
}

void hal_uart_register_backup_callback(void *data)
{
    for (uint32_t port = 0; port < HAL_UART_MAX; port ++) {
        if (uart_setting[port].initialized)
            uart_register_backup((hal_uart_port_t)port);
    }
}

void hal_uart_register_restore_callback(void *data)
{
    for (uint32_t port = 0; port < HAL_UART_MAX; port ++) {
        if (uart_setting[port].initialized)
            uart_register_restore((hal_uart_port_t)port);
    }
}


#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

hal_uart_status_t hal_uart_set_format(hal_uart_port_t uart_port,
                                      const hal_uart_config_t *config)
{
    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0)
        return HAL_UART_STATUS_ERROR_PARAMETER;

    unsigned int baudrate = config->baudrate;
    if (baudrate >= HAL_UART_BAUDRATE_MAX)
        return HAL_UART_STATUS_ERROR_PARAMETER;

    unsigned int data_len = config->word_length;
    unsigned int stop_bits = config->stop_bit;
    unsigned int parity_bits = config->parity;
    unsigned int flow_ctl = fc_none;    /*flow control none  when init*/
    //unsigned int vff_mode = VFIFO_ENABLE_FLAG_NULL;   /*DMA none when init */


    UART_dcb_struct  Port_Config = {
        baudrate,       /* baud */
        data_len,       /* dataBits */
        stop_bits,      /* stopBits */
        parity_bits,    /* parity */
        flow_ctl,       /* flow control*/
        UART_CHAR_XON1,     /* xonChar1 */
        UART_CHAR_XOFF1,    /* xoffChar1 */
        UART_CHAR_XON2,     /* xonChar2 */
        UART_CHAR_XOFF2,    /* xoffChar2 */
        UART_CHAR_ESCAPE,   /* escape char */
    };

    /* configure N81, flow control, baudrate*/
    __uart_config(uart_port, &Port_Config);

    return HAL_UART_STATUS_OK;
}

hal_uart_status_t hal_uart_init(hal_uart_port_t uart_port, hal_uart_config_t *uart_config)
{
    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0)
        return HAL_UART_STATUS_ERROR_PARAMETER;

    unsigned int baudrate = uart_config->baudrate;
    if (baudrate >= HAL_UART_BAUDRATE_MAX)
        return HAL_UART_STATUS_ERROR_PARAMETER;

#ifdef HAL_SLEEP_MANAGER_ENABLED
    g_uart_send_lock_status[uart_port] = false;
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    unsigned int data_len = uart_config->word_length;
    unsigned int stop_bits = uart_config->stop_bit;
    unsigned int parity_bits = uart_config->parity;
    unsigned int flow_ctl = fc_none;    /*flow control none  when init*/
    //unsigned int vff_mode = VFIFO_ENABLE_FLAG_NULL;   /*DMA none when init */


    UART_dcb_struct  Port_Config = {
        baudrate,       /* baud */
        data_len,       /* dataBits */
        stop_bits,      /* stopBits */
        parity_bits,    /* parity */
        flow_ctl,       /* flow control*/
        UART_CHAR_XON1,     /* xonChar1 */
        UART_CHAR_XOFF1,    /* xoffChar1 */
        UART_CHAR_XON2,     /* xonChar2 */
        UART_CHAR_XOFF2,    /* xoffChar2 */
        UART_CHAR_ESCAPE,   /* escape char */
    };
    //uart_clk_pd_enable(port);
    uart_init(uart_port);
    if (!uart_open(uart_port)) {
        return HAL_UART_STATUS_ERROR_BUSY;
    }
    /* configure N81, flow control, baudrate*/
    __uart_config(uart_port, &Port_Config);

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_register_suspend_callback(SLEEP_BACKUP_RESTORE_UART, (hal_sleep_manager_callback_t)hal_uart_register_backup_callback, NULL);
    sleep_management_register_resume_callback(SLEEP_BACKUP_RESTORE_UART, (hal_sleep_manager_callback_t)hal_uart_register_restore_callback, NULL);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return HAL_UART_STATUS_OK;
}

hal_uart_status_t hal_uart_deinit(hal_uart_port_t uart_port)
{
    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0)
        return HAL_UART_STATUS_ERROR_PARAMETER;

    unsigned int irq = uart_get_irq_number_from_port(uart_port);
    mtk_uart_setting *u_setting = &(uart_setting[uart_port]);
    if (u_setting->initialized != TRUE)
        return HAL_UART_STATUS_ERROR_UNINITIALIZED;

    hal_nvic_disable_irq(irq);
    free(u_setting->tx_buffer.CharBuffer);
    free(u_setting->rx_buffer.CharBuffer);

    if (uart_setting[uart_port].init_flag & VFIFO_TX_ENABLE_FLAG) {
        VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)uart_setting[uart_port].tx_dma_run_time_info;
        hal_nvic_disable_irq(head->irq_id);
        VFIFO_stop(head);
    }

    if (uart_setting[uart_port].init_flag & VFIFO_RX_ENABLE_FLAG) {
        VFIFO_ctrl_struct *head = (VFIFO_ctrl_struct *)uart_setting[uart_port].rx_dma_run_time_info;
        hal_nvic_disable_irq(head->irq_id);
        VFIFO_stop(head);
    }

    memset(u_setting, 0x0, sizeof(uart_setting[uart_port]));
    return HAL_UART_STATUS_OK;
}

ATTR_TEXT_IN_SYSRAM void hal_uart_put_char(hal_uart_port_t uart_port, char byte)
{
    uint32_t base = UART_BaseAddr[uart_port];
    uint32_t LSR;

    while (1) {
        LSR = INREG32(UART_LSR(base));
        if (LSR & UART_LSR_THRE) {
            __IO_WRITE32__(UART_THR(base), (uint32_t)byte);
            break;
        }
    }
}

char hal_uart_get_char(hal_uart_port_t uart_port)
{
    uint32_t base = UART_BaseAddr[uart_port];
    uint32_t LSR;
    char ch;

    while (1) {
        LSR = INREG32(UART_LSR(base));
        if (LSR & UART_LSR_DR) {
            ch = __IO_READ32__(UART_RBR(base)) & 0xff;
            break;
        }
    }
    return ch;
}

uint32_t hal_uart_get_char_unblocking(hal_uart_port_t uart_port)
{
    uint32_t base = UART_BaseAddr[uart_port];
    uint32_t LSR;
    uint32_t ch;

    LSR = INREG32(UART_LSR(base));
    if (LSR & UART_LSR_DR)
        ch = __IO_READ32__(UART_RBR(base)) & 0xff;
    else
        ch = 0xffff;
    return ch;
}

ATTR_TEXT_IN_SYSRAM uint32_t hal_uart_send_polling(hal_uart_port_t uart_port, const uint8_t *data, uint32_t size)
{
    uint32_t idx;

    for (idx = 0; idx < size; idx++) {
        hal_uart_put_char(uart_port, *(data + idx));
    }
    return idx;
}

uint32_t hal_uart_send_dma(hal_uart_port_t uart_port, const uint8_t *data, uint32_t size)
{
    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0) {
        return 0;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    uint32_t irq_status;
    irq_status = save_and_set_interrupt_mask();
    if (g_uart_send_lock_status[uart_port] == false) {
        hal_sleep_manager_lock_sleep(uart_sleep_handle);
        g_uart_send_lock_status[uart_port] = true;
    }
    restore_interrupt_mask(irq_status);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */


    uint32_t  left_count, index;
    BUFFER_INFO *uart_buf = &(uart_setting[uart_port].tx_buffer);

    uint32_t uart_base = uart_setting[uart_port].uart_reg_base;
    left_count = Buff_GetRoomLeft(uart_buf);

    if (size > left_count)
        size = left_count;

    for (index = 0; index < size; index++)
        Buff_Push(uart_buf, data + index);

    if (uart_setting[uart_port].use_vfifo) { /* DMA mode */
        if (uart_setting[uart_port].flag & VFIFO_TX_ENABLE_FLAG) {
            if (size)
                VFIFO_int_en(uart_setting[uart_port].tx_dma_run_time_info, 1);
        }/* FIFO mode */
        else
            EnableTxIntr(uart_base);
    } else
        EnableTxIntr(uart_base);
    return size;
}

uint32_t hal_uart_receive_polling(hal_uart_port_t uart_port, uint8_t *buffer, uint32_t size)
{
    uint32_t idx;

    for (idx = 0; idx < size; idx++) {
        *(buffer + idx) = hal_uart_get_char(uart_port);
    }
    return idx;
}

uint32_t hal_uart_receive_dma(hal_uart_port_t uart_port, uint8_t *buffer, uint32_t size)
{
    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0) {
        log_hal_info("invalid port\n");
        return 0;
    }

    uint32_t real_count, index;
    uint32_t BytesReceive = 0;
    BUFFER_INFO *uart_buf = &(uart_setting[uart_port].rx_buffer);

    real_count = Buff_GetBytesAvail(uart_buf);
    for (index = 0; (index < real_count) && (BytesReceive < size) ; index++) {
        Buff_Pop(uart_buf, buffer + BytesReceive);
        BytesReceive++;
    }
    return BytesReceive;
}

uint32_t hal_uart_get_available_send_space(hal_uart_port_t uart_port)
{
    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0) {
        log_hal_info("invalid port\n");
        return 0;
    }

    BUFFER_INFO *uart_buf = &(uart_setting[uart_port].tx_buffer);
    return Buff_GetRoomLeft(uart_buf);
}

uint32_t hal_uart_get_available_receive_bytes(hal_uart_port_t uart_port)
{
    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0) {
        log_hal_info("invalid port\n");
        return 0;
    }

    BUFFER_INFO *uart_buf = &(uart_setting[uart_port].rx_buffer);
    return Buff_GetBytesAvail(uart_buf);
}


hal_uart_status_t hal_uart_register_callback(hal_uart_port_t uart_port,
                                             hal_uart_callback_t user_callback,
                                             void *user_data)
{

    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0)
        return HAL_UART_STATUS_ERROR_PARAMETER;

    if (user_callback == NULL) {
        return HAL_UART_STATUS_ERROR_PARAMETER;
    }

    mtk_uart_setting *u_setting = &(uart_setting[uart_port]);
    if (u_setting->initialized != TRUE)
        return HAL_UART_STATUS_ERROR_UNINITIALIZED;

    g_uart_callback[uart_port].func = user_callback;
    g_uart_callback[uart_port].arg = user_data;

    return HAL_UART_STATUS_OK;
}


hal_uart_status_t hal_uart_set_baudrate(hal_uart_port_t uart_port, hal_uart_baudrate_t baudrate)
{
    if (uart_port >= HAL_UART_MAX)
        return HAL_UART_STATUS_ERROR_PARAMETER;

    mtk_uart_setting *u_setting = &(uart_setting[uart_port]);

    if (u_setting->initialized != TRUE)
        return HAL_UART_STATUS_ERROR_UNINITIALIZED;

    uint32_t baud = uart_get_baud_from_enum(baudrate);
    __uart_set_baud_rate(u_setting, baud);
    return HAL_UART_STATUS_OK;
}


hal_uart_status_t hal_uart_set_dma(hal_uart_port_t uart_port, const hal_uart_dma_config_t *dma_config)
{
    if (uart_port >= HAL_UART_MAX || uart_port < HAL_UART_0)
        return HAL_UART_STATUS_ERROR_PARAMETER;

    mtk_uart_setting *u_setting = &(uart_setting[uart_port]);

    if (u_setting->initialized != TRUE)
        return HAL_UART_STATUS_ERROR_UNINITIALIZED;

    u_setting->use_vfifo = 1;
    uint32_t base = u_setting->uart_reg_base;
    uint32_t vff_en_setting = 0;
    unsigned int vff_mode = VFIFO_TX_ENABLE_FLAG | VFIFO_RX_ENABLE_FLAG;
    uint32_t vff_tx_len = dma_config->send_vfifo_buffer_size;
    uint32_t vff_tx_nc_mem = dma_config->send_vfifo_buffer;
    uint32_t vff_tx_threshold = dma_config->send_vfifo_threshold_size;
    uint32_t vff_rx_len = dma_config->receive_vfifo_buffer_size;
    uint32_t vff_rx_nc_mem = dma_config->receive_vfifo_buffer;
    uint32_t vff_rx_threshold = dma_config->receive_vfifo_threshold_size;

    //uint32_t vff_mem_addr = (uint32_t)pvPortMalloc(buffer_len * 2);/* 8 bytes align */
    uint32_t i, tx_channel = 0, rx_channel = 0;
    if (vff_mode & VFIFO_TX_ENABLE_FLAG) {
        if (0 == vff_tx_nc_mem)
            return HAL_UART_STATUS_ERROR_PARAMETER;
        for (i = 0; i < DRV_SUPPORT_VFF_CHANNEL; i++) {
            if ((vff_info[i].bind_uart_port == (unsigned int)uart_port)
                && (vff_info[i].direction == DIR_TX)) {
                uart_setting[uart_port].tx_dma_static_info = &(vff_info[i]);
                uart_setting[uart_port].tx_dma_run_time_info = &(vff_ctrl[i]);
                uart_setting[uart_port].tx_dma_reg_base = vff_info[i].reg_base;
                vff_ctrl[i].p_info = &(vff_info[i]);
                vff_ctrl[i].reg_base = vff_info[i].reg_base;
                vff_ctrl[i].irq_id = vff_info[i].irq_id;
                vff_ctrl[i].channel = vff_info[i].channel;
                vff_ctrl[i].mem_base = vff_tx_nc_mem;
                vff_ctrl[i].mem_length = vff_tx_len;
                vff_ctrl[i].direction = vff_info[i].direction;
                vff_ctrl[i].is_power_on = 0;
                vff_ctrl[i].uart_ctrl_head = &(uart_setting[uart_port]);
                tx_channel = vff_ctrl[i].channel;
                VFIFO_config(&vff_ctrl[i], vff_tx_nc_mem, vff_tx_len, vff_tx_threshold);
                uart_setting[uart_port].init_flag |= VFIFO_TX_ENABLE_FLAG;
                break;
            }
        }
    }
    if (vff_mode & VFIFO_RX_ENABLE_FLAG) {
        if (0 == vff_rx_nc_mem)
            return HAL_UART_STATUS_ERROR_PARAMETER;
        for (i = 0; i < DRV_SUPPORT_VFF_CHANNEL; i++) {
            if ((vff_info[i].bind_uart_port == (unsigned int)uart_port)
                && (vff_info[i].direction == DIR_RX)) {
                uart_setting[uart_port].rx_dma_static_info = &(vff_info[i]);
                uart_setting[uart_port].rx_dma_run_time_info = &(vff_ctrl[i]);
                uart_setting[uart_port].rx_dma_reg_base = vff_info[i].reg_base;
                vff_ctrl[i].p_info = &(vff_info[i]);
                vff_ctrl[i].reg_base = vff_info[i].reg_base;
                vff_ctrl[i].irq_id = vff_info[i].irq_id;
                vff_ctrl[i].channel = vff_info[i].channel;
                vff_ctrl[i].mem_base = vff_rx_nc_mem;
                vff_ctrl[i].mem_length = vff_rx_len;
                vff_ctrl[i].direction = vff_info[i].direction;
                vff_ctrl[i].is_power_on = 0;
                vff_ctrl[i].uart_ctrl_head = &(uart_setting[uart_port]);
                rx_channel = vff_ctrl[i].channel;
                VFIFO_config(&vff_ctrl[i], vff_rx_nc_mem, vff_rx_len, vff_rx_threshold);
                uart_setting[uart_port].init_flag |= VFIFO_RX_ENABLE_FLAG;
                break;
            }
        }
    }

#ifdef __ENABLE_INTR__
    /* Register Tx intr */
    if (vff_mode & VFIFO_TX_ENABLE_FLAG)
        VFIFO_prepare_Intr(&vff_ctrl[tx_channel]);

    /* Register Rx intr */
    if (vff_mode & VFIFO_RX_ENABLE_FLAG)
        VFIFO_prepare_Intr(&vff_ctrl[rx_channel]);
#endif /* #ifdef __ENABLE_INTR__ */

    vff_en_setting = __IO_READ32__(UART_DMA_EN(base));
    if (vff_mode & VFIFO_TX_ENABLE_FLAG) {
        VFIFO_start(uart_setting[uart_port].tx_dma_run_time_info);
        vff_en_setting = UART_TX_DMA_EN;
        uart_setting[uart_port].flag |= VFIFO_TX_ENABLE_FLAG;
    }
    if (vff_mode & VFIFO_RX_ENABLE_FLAG) {
        VFIFO_start(uart_setting[uart_port].rx_dma_run_time_info);
        VFIFO_int_en(uart_setting[uart_port].rx_dma_run_time_info, 1);
        vff_en_setting |= (UART_RX_DMA_EN | UART_TO_CNT_AUTORST);
        uart_setting[uart_port].flag |= VFIFO_RX_ENABLE_FLAG;
    }
    __IO_WRITE32__(UART_DMA_EN(base), vff_en_setting);
    return HAL_UART_STATUS_OK;
}

/* Add dummy API to fix TF build error */
hal_uart_status_t hal_uart_set_auto_baudrate(hal_uart_port_t uart_port, bool is_enable)
{
    return HAL_UART_STATUS_OK;
}

hal_uart_status_t hal_uart_set_hardware_flowcontrol(hal_uart_port_t uart_port)
{
    if (uart_port >= HAL_UART_MAX)
        return HAL_UART_STATUS_ERROR_PARAMETER;

    mtk_uart_setting *u_setting = &(uart_setting[uart_port]);

    if (u_setting->initialized != TRUE)
        return HAL_UART_STATUS_ERROR_UNINITIALIZED;

    UART_dcb_struct  Port_Config = {
        0,          /* baud */
        0,          /* dataBits */
        0,          /* stopBits */
        0,          /* parity */
        fc_hw,      /* flow control*/
        0,          /* xonChar1 */
        0,          /* xoffChar1 */
        0,          /* xonChar2 */
        0,          /* xoffChar2 */
        0,          /* escape char */
    };

    __uart_set_flowControl(uart_port, &Port_Config);

    return HAL_UART_STATUS_OK;
}

hal_uart_status_t hal_uart_set_software_flowcontrol(hal_uart_port_t uart_port,
                                                    uint8_t xon,
                                                    uint8_t xoff,
                                                    uint8_t escape_character)
{
    if (uart_port >= HAL_UART_MAX)
        return HAL_UART_STATUS_ERROR_PARAMETER;

    mtk_uart_setting *u_setting = &(uart_setting[uart_port]);

    if (u_setting->initialized != TRUE)
        return HAL_UART_STATUS_ERROR_UNINITIALIZED;

    UART_dcb_struct  Port_Config = {
        0,                  /* baud */
        0,                  /* dataBits */
        0,                  /* stopBits */
        0,                  /* parity */
        fc_sw_mtk,          /* flow control*/
        xon,                /* xonChar1 */
        xoff,               /* xoffChar1 */
        UART_CHAR_XON2,     /* xonChar2 */
        UART_CHAR_XOFF2,    /* xoffChar2 */
        escape_character,   /* escape char */
    };

    __uart_set_flowControl(uart_port, &Port_Config);

    return HAL_UART_STATUS_OK;
}

hal_uart_status_t hal_uart_disable_flowcontrol(hal_uart_port_t uart_port)
{
    if (uart_port >= HAL_UART_MAX)
        return HAL_UART_STATUS_ERROR_PARAMETER;

    mtk_uart_setting *u_setting = &(uart_setting[uart_port]);

    if (u_setting->initialized != TRUE)
        return HAL_UART_STATUS_ERROR_UNINITIALIZED;

    UART_dcb_struct  Port_Config = {
        0,          /* baud */
        0,          /* dataBits */
        0,          /* stopBits */
        0,          /* parity */
        fc_none,    /* flow control*/
        0,          /* xonChar1 */
        0,          /* xoffChar1 */
        0,          /* xonChar2 */
        0,          /* xoffChar2 */
        0,          /* escape char */
    };

    __uart_set_flowControl(uart_port, &Port_Config);

    return HAL_UART_STATUS_OK;
}

void InitDebugSerial(void)
{
    unsigned int uart_clock = UART_CLOCK_RATE;
    unsigned int baudrate = UART_BAUD_RATE;

    debug_uart_base = CM33_UART_BASE;
    __uart_baudrate_config(debug_uart_base, baudrate, uart_clock);
}

void InitDebugUart(unsigned int baudrate)
{
    unsigned int uart_clock = UART_CLOCK_RATE;

    debug_uart_base = CM33_UART_BASE;
    __uart_baudrate_config(debug_uart_base, baudrate, uart_clock);
}


#ifdef CFG_CLI_SUPPORT
static QueueHandle_t xRxedChars;

NORMAL_SECTION_FUNC void xSerialPortInitMinimal(unsigned portBASE_TYPE uxQueueLength)
{
    /* Create the queues used to hold Rx characters */
    xRxedChars = xQueueCreate(uxQueueLength, (unsigned portBASE_TYPE)sizeof(signed char));
}

signed portBASE_TYPE xSerialGetChar(signed char *pcRxedChar, TickType_t xBlockTime)
{
    /* Get the next character from the buffer. Return false if no characters
       are available, or arrive before xBlockTime expires. */
    if (xQueueReceive(xRxedChars, pcRxedChar, xBlockTime)) {
        return pdTRUE;
    } else {
        return pdFALSE;
    }
}

void uart0_interrupt_handler(hal_nvic_irq_t irq_number)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    char cChar;
    unsigned int iir;

    iir = INREG32(UART_IIR(UART0_BASE_ADDR));
    if (iir & UART_IIR_RDA || iir & UART_IIR_CTI) {
        while (INREG32(UART_LSR(UART0_BASE_ADDR)) & (UART_LSR_DR)) {
            cChar = (char)INREG32(UART_RBR(UART0_BASE_ADDR));
            if (!xRxedChars)
                continue;
            xQueueSendFromISR(xRxedChars, &cChar, &xHigherPriorityTaskWoken);
        }
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void uart0_nvic_register(hal_uart_port_t port)
{
    unsigned int irq = uart_get_irq_number_from_port(port);
    hal_nvic_disable_irq(irq);
    hal_nvic_register_isr_handler(irq, uart0_interrupt_handler);
    hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    hal_nvic_enable_irq(irq);
}

void uart0_register_irq(void)
{
    unsigned int uart_base = CM33_UART_BASE;

    DRV_WriteReg32(UART_IER(uart_base), UART_IER_ALLOFF);
    uart0_nvic_register(HAL_UART_0);
    DRV_WriteReg32(UART_IER(uart_base), UART_IER_ERBFI);
}

#endif /* #ifdef CFG_CLI_SUPPORT */

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_UART_MODULE_ENABLE)
#include "FreeRTOS.h"

int mtk_uart_init_ut(unsigned int port)
{
    hal_uart_status_t status_t;
    hal_uart_config_t basic_config;
    hal_uart_dma_config_t dma_config;
    int baud = HAL_UART_BAUDRATE_115200;
    const int vff_len = UART_VFF_LEN;//4kB
    unsigned int vff_mem_addr = (unsigned int)pvPortMallocNC(vff_len * 2);/* 8 bytes align */
    basic_config.baudrate = baud;
    basic_config.parity = HAL_UART_PARITY_NONE;
    basic_config.stop_bit = HAL_UART_STOP_BIT_1;
    basic_config.word_length = HAL_UART_WORD_LENGTH_8;
    status_t = hal_uart_init(port, &basic_config);
    if (status_t) {
        log_hal_error("%s: uart init error %d\n", __func__, status_t);
        vPortFreeNC((unsigned int *)vff_mem_addr);
        return status_t;
    }

    dma_config.receive_vfifo_buffer = (unsigned int)vff_mem_addr;
    dma_config.receive_vfifo_buffer_size = vff_len;
    dma_config.receive_vfifo_threshold_size = vff_len / 2;
    dma_config.send_vfifo_buffer = vff_mem_addr + vff_len;
    dma_config.send_vfifo_buffer_size = vff_len;
    dma_config.send_vfifo_threshold_size = vff_len * 7 / 8;
    status_t = hal_uart_set_dma(port, &dma_config);
    if (status_t) {
        log_hal_error("%s: uart set dma error %d\n", __func__, status_t);
        return status_t;
    }

    log_hal_info("top uart1 init done!\r\n");
    return 0;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_UART_MODULE_ENABLE) */

#endif /* #ifdef HAL_UART_MODULE_ENABLED */

