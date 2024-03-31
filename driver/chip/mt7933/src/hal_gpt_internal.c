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
 * MediaTek Inc. (C) 2020. All rights reserved.
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

#include "hal.h"

#ifdef HAL_GPT_MODULE_ENABLED
#include "hal_clk.h"
#include "hal_gpt_internal.h"
#include "hal_nvic_internal.h"
#include "memory_attribute.h"

#define GPT0    ((GPT_REGISTER_T *)(APXGPT_BASE))
#define GPT1    ((GPT_REGISTER_T *)(APXGPT_BASE + 0x20))
#define GPT2    ((GPT_REGISTER_T *)(APXGPT_BASE + 0x40))
#define GPT3    ((GPT_REGISTER_T *)(APXGPT_BASE + 0x60))
#define GPT4    ((GPT_REGISTER_T *)(APXGPT_BASE + 0x80))
#define GPT5    ((GPT_REGISTER_T *)(APXGPT_BASE + 0xA0))

ATTR_RWDATA_IN_TCM GPT_REGISTER_T *gp_gpt[HAL_GPT_MAX_PORT] = {GPT0, GPT1, GPT2, GPT3, GPT4, GPT5};
ATTR_ZIDATA_IN_TCM gpt_context_t g_gpt_context[HAL_GPT_MAX_PORT];
gpt_sw_context_t        gpt_sw_context;

/*
 * Get current counter of GPT3[called GPT4 in Brom] in free run mode. GPT3 will be re-used by
 * gpt basic functions, so use this function after calling any *start_timer* & *stop_timer*
 * functions maybe un-safe.
 *
 * Brom will enable GPT3 with freq = 1M.
 */
uint64_t gpt_get_current_count(void)
{
    return GPT3->GPT_COUNT;
}

ATTR_TEXT_IN_TCM uint32_t gpt_current_count(GPT_REGISTER_T *gpt)
{
    return gpt->GPT_COUNT;
}

ATTR_TEXT_IN_SYSRAM uint32_t gpt_current_count_high(GPT_REGISTER_T *gpt)
{
    return gpt->GPT_COUNTH;
}

ATTR_TEXT_IN_SYSRAM uint32_t gpt_convert_ms_to_32k_count(uint32_t ms)
{
    uint32_t clk_f32k_divider = hal_clk_get_f32k_divider();
    uint32_t ret = 0U;
    ret = (ms * 32) + ((ms * 7) / 10);
    if (clk_f32k_divider == 32746U) {
        ret += ((ms * 4) / 100) + ((ms * 6) / 1000);
    } else {
        ret += ((ms * 6) / 100) + ((ms * 8) / 1000);
    }
    return ret;
}

ATTR_TEXT_IN_TCM void gpt_open_clock_source(void)
{
    return;
}

ATTR_TEXT_IN_TCM void  gpt_start_free_run_timer(GPT_REGISTER_T *gpt, uint32_t clock_source, uint32_t divide)
{
    gpt_open_clock_source();

    gpt->GPT_CON_UNION.GPT_CON_CELLS.CLKSRC = clock_source;
    gpt->GPT_CON_UNION.GPT_CON_CELLS.CLKDIV = divide;

    /* set to free run mode, open clock source and start counter */
    gpt->GPT_CON_UNION.GPT_CON_CELLS.MODE = GPT_MODE_FREE_RUN;
    gpt->GPT_CON_UNION.GPT_CON_CELLS.EN = GPT_COUNT_START;
}

ATTR_TEXT_IN_TCM void gpt_delay_time(GPT_REGISTER_T *gpt, const uint32_t count)
{
    uint32_t current = 0, temp0 = 0, temp1 = 0;

    current = gpt_current_count(gpt);
    while (temp1 <= count) {
        temp0 = gpt_current_count(gpt);

        if (temp0 > current)
            temp1 = temp0 - current;
        else
            temp1 = (0xffffffff - (current - temp0)) + 1;
    }
}

void gpt_reset_default_timer(GPT_REGISTER_T *gpt)
{
    gpt->GPT_CON_UNION.GPT_CON = 0;
    gpt->GPT_CON_UNION.GPT_CON_CELLS.CNTCLR = GPT_COUNT_CLEAR;
    gpt->GPT_CON_UNION.GPT_CON_CELLS.IRQCLR = GPT_IRQ_FLAG_ACK;
    gpt->GPT_COMPARE = 0;
    if (gpt == GPT5) {
        gpt->GPT_COMPAREH = 0;
    }
}

uint32_t gpt_save_and_mask_interrupt(GPT_REGISTER_T *gpt)
{
    volatile uint32_t mask;

    mask = gpt->GPT_CON_UNION.GPT_CON_CELLS.IRQEN;

    gpt->GPT_CON_UNION.GPT_CON_CELLS.IRQEN = 0;

    return mask;
}

void gpt_restore_interrupt(GPT_REGISTER_T *gpt, uint32_t mask)
{
    gpt->GPT_CON_UNION.GPT_CON_CELLS.IRQEN = mask;
}

int32_t gpt_get_irq_number_from_port(hal_gpt_port_t port)
{
    switch (port) {
        case HAL_GPT_0:
            return APXGPT0_IRQn;
        case HAL_GPT_1:
            return APXGPT1_IRQn;
        case HAL_GPT_2:
            return APXGPT2_IRQn;
        case HAL_GPT_3:
            return APXGPT3_IRQn;
        case HAL_GPT_4:
            return APXGPT4_IRQn;
        case HAL_GPT_5:
            return APXGPT5_IRQn;
        default:
            return HAL_GPT_STATUS_ERROR_PORT;
    }
}

uint32_t gpt_get_port_from_irq_number(hal_nvic_irq_t irq)
{
    switch (irq) {
        case APXGPT0_IRQn:
            return HAL_GPT_0;
        case APXGPT1_IRQn:
            return HAL_GPT_1;
        case APXGPT2_IRQn:
            return HAL_GPT_2;
        case APXGPT3_IRQn:
            return HAL_GPT_3;
        case APXGPT4_IRQn:
            return HAL_GPT_4;
        case APXGPT5_IRQn:
            return HAL_GPT_5;
        default:
            return HAL_GPT_STATUS_INVALID_PARAMETER;
    }
}

void gpt_interrupt_handler(hal_nvic_irq_t irq_number)
{
    volatile uint32_t mask;
    volatile uint32_t state;
    volatile uint32_t enable;
    uint32_t port = gpt_get_port_from_irq_number(irq_number);

    if (port >= HAL_GPT_MAX_PORT) {
        return;
    }

    /* get gpt irq status */
    state  = gp_gpt[port]->GPT_CON_UNION.GPT_CON_CELLS.IRQSTA;
    enable = gp_gpt[port]->GPT_CON_UNION.GPT_CON_CELLS.IRQEN;

    mask = gpt_save_and_mask_interrupt(gp_gpt[port]);

    gp_gpt[port]->GPT_CON_UNION.GPT_CON_CELLS.IRQCLR = GPT_IRQ_FLAG_ACK;

    if ((state & GPT_IRQ_FLAG_STA) && (enable & GPT_IRQ_ENABLE)) {
        /* clear interrupt status */
#ifdef GPT_DEBUG_LOG
        if (i != HAL_GPT_SW_PORT) {
            //    log_hal_info("[GPT%d]GPT_IRQ_STA = 0x%x\r\n", (int)i, (int)gp_gpt[i]->GPT_IRQ_STA);
        }
#endif /* #ifdef GPT_DEBUG_LOG */

        if (g_gpt_context[port].callback_context.callback != NULL) {
            g_gpt_context[port].callback_context.callback(g_gpt_context[port].callback_context.user_data);
        }
    }
    gpt_restore_interrupt(gp_gpt[port], mask);
}

void gpt_nvic_register(hal_gpt_port_t port)
{
    int32_t irq = gpt_get_irq_number_from_port(port);
    hal_nvic_disable_irq(irq);
    hal_nvic_register_isr_handler(irq, gpt_interrupt_handler);
    hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    hal_nvic_enable_irq(irq);
}

uint32_t gpt_get_current_time_ms(GPT_REGISTER_T *gpt)
{
    volatile uint32_t time;
    uint32_t count, time_s, time_ms;

    count = gpt_current_count(gpt);
    time_s = count / 32768;
    time_ms = ((count % 32768) * 1000 + 16384) / 32768;
    time = time_s * 1000 + time_ms;

    return time;
}

uint32_t gpt_sw_get_current_time_ms(GPT_REGISTER_T *gpt)
{
    volatile uint32_t time = gpt_get_current_time_ms(gpt);

    /*clear count*/
    gp_gpt[HAL_GPT_SW_PORT]->GPT_CON_UNION.GPT_CON_CELLS.CNTCLR = GPT_COUNT_CLEAR;
    while (gp_gpt[HAL_GPT_SW_PORT]->GPT_COUNT);

    return time;
}

uint32_t gpt_sw_get_free_timer(void)
{
    uint32_t i;

    for (i = 0; i < HAL_GPT_SW_NUMBER; i++) {
        if (gpt_sw_context.timer[i].is_used != true) {
            return i;
        }
    }

    return HAL_GPT_SW_NUMBER;
}

void gpt_sw_start_timer(void)
{
    uint32_t minimum_time, current_timer;

    if (gpt_sw_context.is_start_from_isr == true) {
        return;
    }

    gpt_sw_get_minimum_left_time_ms(&minimum_time, &current_timer);

    /*restore this absolute time*/
    gpt_sw_context.last_absolute_time = gpt_sw_context.absolute_time;

    gpt_sw_context.is_sw_gpt = true;
    hal_gpt_start_timer_ms(HAL_GPT_SW_PORT, minimum_time, HAL_GPT_TIMER_TYPE_ONE_SHOT);
    gpt_sw_context.is_sw_gpt = false;
}

uint32_t gpt_sw_absolute_value(uint32_t timer_number)
{
    uint32_t time_out_delta;
    uint32_t current_delta;
    uint32_t ret_value;

    time_out_delta = gpt_sw_delta(gpt_sw_context.timer[timer_number].time_out_ms, gpt_sw_context.last_absolute_time);
    current_delta  = gpt_sw_delta(gpt_sw_context.absolute_time, gpt_sw_context.last_absolute_time);

    if (time_out_delta > current_delta) {
        /*timer has not been expired*/
        ret_value = gpt_sw_delta(gpt_sw_context.timer[timer_number].time_out_ms, gpt_sw_context.absolute_time);
    } else {
        /*timer has been expired*/
        ret_value =  0;
    }

    return ret_value;
}
void gpt_sw_get_minimum_left_time_ms(uint32_t *minimum_time, uint32_t *number)
{
    uint32_t i;
    uint32_t minimum = 0xffffffff;
    uint32_t data;

    for (i = 0; i < HAL_GPT_SW_NUMBER; i++) {
        if (gpt_sw_context.timer[i].is_running == true) {
            data = gpt_sw_absolute_value(i);

            if (data < minimum) {
                minimum = data;
                *number = i;
            }
        }
    }

    *minimum_time = minimum;
}


void gpt_sw_handler(void *parameter)
{
    uint32_t i;
    volatile uint32_t time_ms;
    volatile uint32_t data;


    if (gpt_sw_context.used_timer_count != 0) {

        gpt_sw_context.is_sw_gpt = true;
        hal_gpt_stop_timer(HAL_GPT_SW_PORT);
        gpt_sw_context.is_sw_gpt = false;

        time_ms = gpt_sw_get_current_time_ms(gp_gpt[HAL_GPT_SW_PORT]);

        gpt_sw_context.absolute_time += time_ms;
        gpt_sw_context.is_start_from_isr = true;

        for (i = 0; i < HAL_GPT_SW_NUMBER; i++) {
            if (gpt_sw_context.timer[i].is_running == true) {

                data = gpt_sw_absolute_value(i);

                if (data == 0) {
                    gpt_sw_context.timer[i].is_running = false;
                    gpt_sw_context.running_timer_count--;

                    gpt_sw_context.timer[i].callback_context.callback(gpt_sw_context.timer[i].callback_context.user_data);
                }

            }
        }

        gpt_sw_context.is_start_from_isr = false;

        if (gpt_sw_context.running_timer_count != 0) {
            gpt_sw_start_timer();
        }
    }

}


#endif /* #ifdef HAL_GPT_MODULE_ENABLED */


