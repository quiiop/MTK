/* Copyright Statement:
 *
 * @2015 MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek Inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
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
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE.
 */

#include "hal_gdma.h"
#include "hal_gdma_internal.h"
#include "hal_log.h"

#ifndef __UBL__
#include "assert.h"
#else /* #ifndef __UBL__ */
#define assert(expr) log_hal_error("assert\r\n")
#endif /* #ifndef __UBL__ */

#ifdef HAL_GDMA_MODULE_ENABLED

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

/*general dma base address array */
GDMA_REGISTER_T  *gdma[GDMA_NUMBER] = {GDMA1, GDMA2, GDMA3};

volatile uint32_t gdma_status = 0;

/*general dma's  callback function array*/
gdma_user_callback_t g_gdma_callback[GDMA_NUMBER] = {
    {NULL, NULL}, {NULL, NULL}, {NULL, NULL}
};

void gdma_reset(uint32_t channel)
{
    /*reset gdma default setting*/
    gdma[channel]->GDMA_INT_FLAG = 0x0;
    gdma[channel]->GDMA_INT_EN = 0x0;
    gdma[channel]->GDMA_START = 0x0;
    gdma[channel]->GDMA_CON = 0x0;
    gdma[channel]->GDMA_SRC = 0x0;
    gdma[channel]->GDMA_DST = 0x0;
    gdma[channel]->GDMA_LEN1 = 0x0;
    gdma[channel]->GDMA_LEN2 = 0x0;
    gdma[channel]->GDMA_JUMP = 0x0;
    gdma[channel]->GDMA_SRC_ADDR2 = 0x0;
    gdma[channel]->GDMA_DST_ADDR2 = 0x0;
    gdma[channel]->GDMA_JUMP_ADDR2 = 0x0;
}

void gdma_register_callback(uint32_t channel, hal_gdma_callback_t callback, void *user_data)

{
    if (g_gdma_callback[channel].func == NULL) {
        g_gdma_callback[channel].func = callback;
        g_gdma_callback[channel].argument   = user_data;
    }
}

uint32_t gdma_get_working_status(uint32_t channel)
{
    uint32_t global_status = 0 ;

    /* read gdma running  status */
    global_status = gdma[channel]->GDMA_START;

    return  global_status;
}

void gdma_stop(uint32_t channel)
{
    gdma[channel]->GDMA_STOP = GDMA_STOP_BIT_MASK;

    while (gdma[channel]->GDMA_START);
    while (gdma[channel]->GDMA_STOP);
    gdma[channel]->GDMA_STOP = GDMA_STOP_CLR_BIT_MASK;
    gdma[channel]->GDMA_INT_FLAG = GDMA_INT_FLAG_CLR_BIT_MASK;
}

void gdma_start(uint32_t channel)
{
    gdma[channel]->GDMA_INT_FLAG = GDMA_INT_FLAG_CLR_BIT_MASK;
    gdma[channel]->GDMA_START = GDMA_START_BIT_MASK;
}

void gdma_set_len(uint32_t channel, uint32_t size)
{
    gdma[channel]->GDMA_LEN1 = size;
    //log_hal_info("[GDMA INTERNAL]len:0x%x\r\n", gdma[channel]->GDMA_LEN1);
}

void gdma_set_iten(uint32_t channel, bool enable)
{
    gdma[channel]->GDMA_INT_EN = enable;
    //log_hal_info("[GDMA INTERNAL]iten:0x%x\r\n", gdma[channel]->GDMA_INT_EN);
}

void gdma_set_address(uint32_t channel, uint32_t destination, uint32_t source)
{
    gdma[channel]->GDMA_SRC = source;
    gdma[channel]->GDMA_DST = destination;
    //log_hal_info("[GDMA INTERNAL]SRC:0x%x   DST:0x%x\r\n", gdma[channel]->GDMA_SRC, gdma[channel]->GDMA_DST);
}

void gdma_clear_irq(uint32_t channel)
{
    gdma[channel]->GDMA_INT_FLAG = GDMA_INT_FLAG_CLR_BIT_MASK;
}

void gdma0_interrupt_hander(hal_nvic_irq_t irq_number)
{
    void *argument;
    hal_gdma_callback_t gdma_callback;

    gdma_callback = g_gdma_callback[0].func;
    argument = g_gdma_callback[0].argument;
    if (gdma_callback != NULL) {
        gdma_callback(HAL_GDMA_EVENT_TRANSACTION_SUCCESS, argument);
    } else {
        assert(0);
    }
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_DMA);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
}

void gdma1_interrupt_hander(hal_nvic_irq_t irq_number)
{
    void *argument;
    hal_gdma_callback_t gdma_callback;

    gdma_callback = g_gdma_callback[1].func;
    argument = g_gdma_callback[1].argument;
    if (gdma_callback != NULL) {
        gdma_callback(HAL_GDMA_EVENT_TRANSACTION_SUCCESS, argument);
    } else {
        assert(0);
    }
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_DMA);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
}

void gdma2_interrupt_hander(hal_nvic_irq_t irq_number)
{
    void *argument;
    hal_gdma_callback_t gdma_callback;

    gdma_callback = g_gdma_callback[2].func;
    argument = g_gdma_callback[2].argument;
    if (gdma_callback != NULL) {
        gdma_callback(HAL_GDMA_EVENT_TRANSACTION_SUCCESS, argument);
    } else {
        assert(0);
    }
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_DMA);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
}

void gdma_init(uint32_t channel)
{
    /* keep driver default setting */
    gdma_reset(channel);
    /*register irq handler*/
    if (0 == channel) {
        hal_nvic_register_isr_handler(CQDMA0_IRQn, gdma0_interrupt_hander);
        hal_nvic_enable_irq(CQDMA0_IRQn);
    } else if (1 == channel) {
        hal_nvic_register_isr_handler(CQDMA1_IRQn, gdma1_interrupt_hander);
        hal_nvic_enable_irq(CQDMA1_IRQn);
    } else if (2 == channel) {
        hal_nvic_register_isr_handler(CQDMA2_IRQn, gdma2_interrupt_hander);
        hal_nvic_enable_irq(CQDMA2_IRQn);
    }
}

void gdma_deinit(uint32_t channel)
{
    /* stop gdma */
    gdma_stop(channel);
}
#endif /* #ifdef HAL_GDMA_MODULE_ENABLED */
