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
#include "hal_gpt_internal.h"
#include "hal_nvic_internal.h"
#include "memory_attribute.h"

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"

#define GPT_GET_SLEEP_HANDLE(gpt_port)
#define GPT_RELEASE_SLEEP_HANDLE(gpt_port)
#define GPT_LOCK_SLEEP(gpt_port) \
do{ \
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_GPT); \
}while(0)

#define GPT_UNLOCK_SLEEP(gpt_port) \
do{ \
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_GPT); \
}while(0)

#else /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
#define GPT_GET_SLEEP_HANDLE(gpt_port)
#define GPT_RELEASE_SLEEP_HANDLE(gpt_port)
#define GPT_LOCK_SLEEP(gpt_port)
#define GPT_UNLOCK_SLEEP(gpt_port)
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

static bool hal_gpt_is_port_valid(hal_gpt_port_t gpt_port)
{
    /*make sure this port just for sw gpt*/
    if ((gpt_sw_context.is_sw_gpt == false) && (gpt_port == HAL_GPT_SW_PORT))
        return false;

    if ((gpt_port < HAL_GPT_MAX_PORT) && (gpt_port != HAL_GPT_MS_PORT) &&
        (gpt_port != HAL_GPT_US_PORT) && (gpt_port != HAL_GPT_PORT_INVALID))
        return true;
    else
        return false;
}

hal_gpt_status_t hal_gpt_init(hal_gpt_port_t gpt_port)
{

    if (hal_gpt_is_port_valid(gpt_port) != true)
        return HAL_GPT_STATUS_ERROR_PORT;

    if ((g_gpt_context[gpt_port].running_status == HAL_GPT_RUNNING) ||
        (g_gpt_context[gpt_port].has_initilized == true)) {
        return HAL_GPT_STATUS_ERROR;
    }

    /*set structure to 0 */
    memset(&g_gpt_context[gpt_port], 0, sizeof(gpt_context_t));

    /*enable pdn power, open clock source*/
    gpt_open_clock_source();

    /*set flag respect this port has initlized */
    g_gpt_context[gpt_port].has_initilized = true;

    /*disable interrupt */
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQEN = 0;

    GPT_GET_SLEEP_HANDLE(gpt_port);
#ifdef GPT_DEBUG_LOG
    log_hal_info("[GPT%d] init OK\r\n", (int)gpt_port);
#endif /* #ifdef GPT_DEBUG_LOG */
    return HAL_GPT_STATUS_OK;
}

hal_gpt_status_t hal_gpt_deinit(hal_gpt_port_t gpt_port)
{

    if (hal_gpt_is_port_valid(gpt_port) != true)
        return HAL_GPT_STATUS_ERROR_PORT;

    if (g_gpt_context[gpt_port].running_status == HAL_GPT_RUNNING)
        return HAL_GPT_STATUS_ERROR;

    /* set structure to 0 */
    memset(&g_gpt_context[gpt_port], 0, sizeof(gpt_context_t));

    /* set flag indicate this port has deinitlized */
    g_gpt_context[gpt_port].has_initilized = false;

    gpt_reset_default_timer(gp_gpt[gpt_port]);

    GPT_RELEASE_SLEEP_HANDLE(gpt_port);
#ifdef GPT_DEBUG_LOG
    log_hal_info("[GPT%d] deinit OK\r\n", (int)gpt_port);
#endif /* #ifdef GPT_DEBUG_LOG */

    return HAL_GPT_STATUS_OK;
}

/* API to manage over-run of 32bit HW counter */
static uint64_t _last_tick_count = 0;    /* use last-tick-count to detect roll-over */
static uint64_t _accumulated_tick_count = 0; /* this is accumulated tick counts include roll-over */
static uint32_t  _first_time = 1;

#define OVERFLOW_MAX_VAL     (0xFFFFFFFFUL)

static uint64_t get_accumulated_count(hal_gpt_port_t gpt_port)
{
    uint32_t delta;
    uint64_t result;
    uint32_t mask;
    uint64_t count, count_h;

    if (gpt_port >= HAL_GPT_MAX_PORT || gpt_port == HAL_GPT_PORT_INVALID)
        return 0;

    /* disable interrupt */
    hal_nvic_save_and_set_interrupt_mask(&mask);
    count = gpt_current_count(gp_gpt[gpt_port]);

    if (gpt_port == HAL_GPT_5) {
        /* GPT_5 is 64bit timer */
        count_h = gpt_current_count_high(gp_gpt[gpt_port]);
        result = ((uint64_t)count_h << 32) | count;

        hal_nvic_restore_interrupt_mask(mask);

        return result;
    }

    /* check for over-flow */
    if ((count) < _last_tick_count) {
        /* compensate for roll-ver: compute the differences between MAX_OVER_FLOW value
           and last tick value, and add to the current count value, we also add 1 to include
           the `0` tick when counter roll-over */

        delta = (OVERFLOW_MAX_VAL - _last_tick_count) + (count + 1);
    } else {
        delta = (count) - _last_tick_count;
    }
    /* update the last_tick_count and accumulated tick counts */
    _last_tick_count = count;
    /* If this is first time, then need to update accumlated tick count */
    if (_first_time) {
        _accumulated_tick_count = _last_tick_count;

        _first_time = 0;
    } else {
        _accumulated_tick_count = _accumulated_tick_count + delta;
    }
    result = _accumulated_tick_count;

    /* enable interrupt */
    hal_nvic_restore_interrupt_mask(mask);

    return result;
}

ATTR_TEXT_IN_SYSRAM hal_gpt_status_t hal_gpt_get_free_run_count_64(hal_gpt_clock_source_t clock_source, uint64_t *count)
{
    /* millisecond free run timer */
    if (clock_source == HAL_GPT_CLOCK_SOURCE_32K) {
        if (g_gpt_context[HAL_GPT_MS_PORT].running_status != HAL_GPT_RUNNING) {

            /* set clock source to 32khz, and start timer */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_MS_PORT], GPT_CLOCK_32KHZ, GPT_DIVIDE_1);

            g_gpt_context[HAL_GPT_MS_PORT].running_status = HAL_GPT_RUNNING;
        }

        *count = get_accumulated_count(HAL_GPT_MS_PORT);
    } /* microsecond free rum timer */
    else if (clock_source == HAL_GPT_CLOCK_SOURCE_1M) {
        if (g_gpt_context[HAL_GPT_US_PORT].running_status != HAL_GPT_RUNNING) {
#ifdef MTK_FPGA_ENABLE
            /* set clcok source to 1mhz, and start timer, gpt clk is 6mhz on fpga */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_6MHZ, GPT_DIVIDE_6);
#else /* #ifdef MTK_FPGA_ENABLE */
            /* set clcok source to 1mhz, and start timer */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_13MHZ, GPT_DIVIDE_13);
#endif /* #ifdef MTK_FPGA_ENABLE */

            g_gpt_context[HAL_GPT_US_PORT].running_status = HAL_GPT_RUNNING;
        }
        *count = get_accumulated_count(HAL_GPT_US_PORT);
    } else {
        return HAL_GPT_STATUS_INVALID_PARAMETER;
    }

    return HAL_GPT_STATUS_OK;
}

ATTR_TEXT_IN_TCM hal_gpt_status_t hal_gpt_get_free_run_count(hal_gpt_clock_source_t clock_source, uint32_t *count)
{
    /* millisecond free run timer */
    if (clock_source == HAL_GPT_CLOCK_SOURCE_32K) {
        if (g_gpt_context[HAL_GPT_MS_PORT].running_status != HAL_GPT_RUNNING) {

            /* set clock source to 32khz, and start timer */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_MS_PORT], GPT_CLOCK_32KHZ, GPT_DIVIDE_1);

            g_gpt_context[HAL_GPT_MS_PORT].running_status = HAL_GPT_RUNNING;
        }

        *count = gpt_current_count(gp_gpt[HAL_GPT_MS_PORT]);
    } /* microsecond free rum timer */
    else if (clock_source == HAL_GPT_CLOCK_SOURCE_1M) {
        if (g_gpt_context[HAL_GPT_US_PORT].running_status != HAL_GPT_RUNNING) {
#ifdef MTK_FPGA_ENABLE
            /* set clcok source to 1mhz, and start timer, gpt clk is 6mhz on fpga */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_6MHZ, GPT_DIVIDE_6);
#else /* #ifdef MTK_FPGA_ENABLE */
            /* set clcok source to 1mhz, and start timer */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_13MHZ, GPT_DIVIDE_13);
#endif /* #ifdef MTK_FPGA_ENABLE */

            g_gpt_context[HAL_GPT_US_PORT].running_status = HAL_GPT_RUNNING;

        }
        *count = gpt_current_count(gp_gpt[HAL_GPT_US_PORT]);
    } else {
        return HAL_GPT_STATUS_INVALID_PARAMETER;
    }

    return HAL_GPT_STATUS_OK;
}

ATTR_TEXT_IN_TCM hal_gpt_status_t hal_gpt_get_duration_count(uint32_t start_count, uint32_t end_count, uint32_t *duration_count)
{
    if (duration_count == NULL)
        return HAL_GPT_STATUS_INVALID_PARAMETER;

    if (end_count > start_count)
        *duration_count = end_count - start_count;
    else
        *duration_count = (0xffffffff - (start_count - end_count)) + 1;

    return HAL_GPT_STATUS_OK;
}

hal_gpt_status_t hal_gpt_get_running_status(hal_gpt_port_t gpt_port, hal_gpt_running_status_t *running_status)
{
    if (gpt_port >= HAL_GPT_MAX_PORT || gpt_port == HAL_GPT_PORT_INVALID)
        return HAL_GPT_STATUS_ERROR_PORT;

    *running_status = g_gpt_context[gpt_port].running_status;

    return HAL_GPT_STATUS_OK;
}

hal_gpt_status_t hal_gpt_register_callback(hal_gpt_port_t    gpt_port,
                                           hal_gpt_callback_t   callback,
                                           void                *user_data)
{
    if (hal_gpt_is_port_valid(gpt_port) != true)
        return HAL_GPT_STATUS_ERROR_PORT;

    if ((g_gpt_context[gpt_port].running_status == HAL_GPT_RUNNING) ||
        (g_gpt_context[gpt_port].has_initilized != true)) {

        return HAL_GPT_STATUS_ERROR;
    }

    if (callback == NULL)
        return HAL_GPT_STATUS_INVALID_PARAMETER;

#ifdef GPT_DEBUG_LOG
    log_hal_info("[GPT%d] register callback:0x%.8x\r\n", (unsigned int)gpt_port, (unsigned int)callback);
#endif /* #ifdef GPT_DEBUG_LOG */
    g_gpt_context[gpt_port].callback_context.callback  = callback;
    g_gpt_context[gpt_port].callback_context.user_data = user_data;

    gpt_nvic_register(gpt_port);

    return HAL_GPT_STATUS_OK;
}

hal_gpt_status_t hal_gpt_start_timer_ms(hal_gpt_port_t gpt_port, uint32_t timeout_time_ms, hal_gpt_timer_type_t timer_type)
{
    volatile uint32_t mask;

    if (hal_gpt_is_port_valid(gpt_port) != true)
        return HAL_GPT_STATUS_ERROR_PORT;

    if ((g_gpt_context[gpt_port].running_status == HAL_GPT_RUNNING)
        || (g_gpt_context[gpt_port].has_initilized != true)) {

        return HAL_GPT_STATUS_ERROR_PORT_USED;
    }

    if (timeout_time_ms > HAL_GPT_MAXIMUM_MS_TIMER_TIME)
        return HAL_GPT_STATUS_INVALID_PARAMETER;

#ifdef GPT_DEBUG_LOG
    if (gpt_port !=  HAL_GPT_SW_PORT) {
        log_hal_info("[GPT%d]hal_gpt_start_timer_ms, time=%d ms,type=%d\r\n", (int)gpt_port, (int)timeout_time_ms, (int)timer_type);
    }
#endif /* #ifdef GPT_DEBUG_LOG */

    mask = save_and_set_interrupt_mask();

    gpt_open_clock_source();

    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQEN = 0; /* disable interrupt */
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.EN = 0;    /* stop timer */

    /* set to 32K clock and 1 division,clear counter */
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.CLKSRC = GPT_CLOCK_32KHZ;
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.CLKDIV = GPT_DIVIDE_1;
    gp_gpt[gpt_port]->GPT_COMPARE = gpt_convert_ms_to_32k_count(timeout_time_ms) ;
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQCLR = GPT_IRQ_FLAG_ACK;
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.CNTCLR = GPT_COUNT_CLEAR;
    while (gp_gpt[gpt_port]->GPT_COUNT);

    /* set to mode, open clock source and start counter */
    switch (timer_type) {
        case HAL_GPT_TIMER_TYPE_ONE_SHOT:
            gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.MODE = GPT_MODE_ONE_SHOT;
            gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.EN = GPT_COUNT_START;
            break;
        case HAL_GPT_TIMER_TYPE_REPEAT:
            gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.MODE = GPT_MODE_REPEAT;
            gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.EN = GPT_COUNT_START;
            break;
    }

    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQEN = GPT_IRQ_ENABLE;
    g_gpt_context[gpt_port].running_status = HAL_GPT_RUNNING;
    restore_interrupt_mask(mask);

    return HAL_GPT_STATUS_OK;
}

ATTR_TEXT_IN_SYSRAM hal_gpt_status_t hal_gpt_delay_ms(uint32_t ms)
{
    return hal_gpt_delay_us(ms * 1000);
}

#ifdef HAL_GPT_FEATURE_US_TIMER
hal_gpt_status_t hal_gpt_start_timer_30us(hal_gpt_port_t gpt_port, uint32_t timeout_time_us, hal_gpt_timer_type_t timer_type)
{
    uint32_t aligned_timeout;

    aligned_timeout = (timeout_time_us + 29) / 30 * 30;

    return hal_gpt_start_timer_us(gpt_port, aligned_timeout, timer_type);
}

hal_gpt_status_t hal_gpt_start_timer_us(hal_gpt_port_t gpt_port, uint32_t timeout_time_us, hal_gpt_timer_type_t timer_type)
{
    volatile uint32_t mask;

    if (hal_gpt_is_port_valid(gpt_port) != true) {
        //log_hal_error("Invalid port: %d. Only port 0 or 1 works as timer.", gpt_port);
        return HAL_GPT_STATUS_ERROR_PORT;
    }

    if ((g_gpt_context[gpt_port].running_status == HAL_GPT_RUNNING)
        || (g_gpt_context[gpt_port].has_initilized != true)) {
        return HAL_GPT_STATUS_ERROR_PORT_USED;
    }

#ifdef GPT_DEBUG_LOG
    if (gpt_port !=  HAL_GPT_SW_PORT) {
        log_hal_info("[GPT%d]hal_gpt_start_timer_us, time=%d us,type=%d\r\n", (int)gpt_port, (int)timeout_time_us, (int)timer_type);
    }
#endif /* #ifdef GPT_DEBUG_LOG */

    if (g_gpt_context[gpt_port].is_gpt_locked_sleep == false) {
        GPT_LOCK_SLEEP(gpt_port);
        g_gpt_context[gpt_port].is_gpt_locked_sleep = true;
    }

    mask = save_and_set_interrupt_mask();

    gpt_open_clock_source();

    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQEN  = 0;   /* disable interrupt */
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.EN = 0;       /* stop timer */

#ifdef MTK_FPGA_ENABLE
    // gpt clk is 6MHz on fpga
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.CLKSRC = GPT_CLOCK_6MHZ;
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.CLKDIV = GPT_DIVIDE_6;
#else /* #ifdef MTK_FPGA_ENABLE */
    //set to 13MHz clock and 13 division, clear counter,1 us per tick
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.CLKSRC = GPT_CLOCK_13MHZ;
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.CLKDIV = GPT_DIVIDE_13;
#endif /* #ifdef MTK_FPGA_ENABLE */
    gp_gpt[gpt_port]->GPT_COMPARE   = timeout_time_us;
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQCLR = GPT_IRQ_FLAG_ACK;
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.CNTCLR = GPT_COUNT_CLEAR;
    while (gp_gpt[gpt_port]->GPT_COUNT);

    /* set to mode, open clock source and start counter */
    switch (timer_type) {
        case HAL_GPT_TIMER_TYPE_ONE_SHOT:
            gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.MODE = GPT_MODE_ONE_SHOT;
            gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.EN = GPT_COUNT_START;
            break;
        case HAL_GPT_TIMER_TYPE_REPEAT:
            gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.MODE = GPT_MODE_REPEAT;
            gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.EN = GPT_COUNT_START;
            break;
    }

    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQEN = GPT_IRQ_ENABLE;

    g_gpt_context[gpt_port].running_status = HAL_GPT_RUNNING;

    restore_interrupt_mask(mask);

    return HAL_GPT_STATUS_OK;
}

ATTR_TEXT_IN_TCM hal_gpt_status_t hal_gpt_delay_us(uint32_t us)
{
    /* if free run timer is not open, open it */
    if (g_gpt_context[HAL_GPT_US_PORT].running_status != HAL_GPT_RUNNING) {

#ifdef MTK_FPGA_ENABLE
        /* fpga clk source is 6mhz, set clcok source to 1mhz, and start timer */
        gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_6MHZ, GPT_DIVIDE_6);
#else /* #ifdef MTK_FPGA_ENABLE */
        /* set clcok source to 1mhz, and start timer */
        gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_13MHZ, GPT_DIVIDE_13);
#endif /* #ifdef MTK_FPGA_ENABLE */
        g_gpt_context[HAL_GPT_US_PORT].running_status = HAL_GPT_RUNNING;
    }
    gpt_delay_time(gp_gpt[HAL_GPT_US_PORT], us);

    return HAL_GPT_STATUS_OK;
}
#endif /* #ifdef HAL_GPT_FEATURE_US_TIMER */

hal_gpt_status_t hal_gpt_start_timer(hal_gpt_port_t gpt_port)
{
    volatile uint32_t mask;

    if (hal_gpt_is_port_valid(gpt_port) != true)
        return HAL_GPT_STATUS_ERROR_PORT;

#ifdef GPT_DEBUG_LOG
    if (gpt_port !=  HAL_GPT_SW_PORT)
        log_hal_info("[GPT%d]hal_gpt_start_timer\r\n", (int)gpt_port);
#endif /* #ifdef GPT_DEBUG_LOG */

    if (g_gpt_context[gpt_port].running_status != HAL_GPT_STOPPED)
        return HAL_GPT_STATUS_ERROR;

    mask = save_and_set_interrupt_mask();

    /*Enable interrupt*/
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQCLR = GPT_IRQ_FLAG_ACK;
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQEN = GPT_IRQ_ENABLE;

    /* start timer */
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.EN = GPT_COUNT_START;

    g_gpt_context[gpt_port].running_status = HAL_GPT_RUNNING;

    restore_interrupt_mask(mask);

    if (g_gpt_context[gpt_port].is_gpt_locked_sleep == false) {
        GPT_LOCK_SLEEP(gpt_port);
        g_gpt_context[gpt_port].is_gpt_locked_sleep = true;
    }

    return HAL_GPT_STATUS_OK;
}

hal_gpt_status_t hal_gpt_stop_timer(hal_gpt_port_t gpt_port)
{
    volatile uint32_t mask;

    if (hal_gpt_is_port_valid(gpt_port) != true)
        return HAL_GPT_STATUS_ERROR_PORT;

#ifdef GPT_DEBUG_LOG
    if (gpt_port !=  HAL_GPT_SW_PORT)
        log_hal_info("[GPT%d]hal_gpt_stop_timer\r\n", (int)gpt_port);
#endif /* #ifdef GPT_DEBUG_LOG */

    mask = save_and_set_interrupt_mask();

    /*diable interrupt*/
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQEN = 0;

    /* stop timer */
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.EN = 0;
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQCLR = GPT_IRQ_FLAG_ACK;

    g_gpt_context[gpt_port].running_status = HAL_GPT_STOPPED;
    restore_interrupt_mask(mask);

    if (g_gpt_context[gpt_port].is_gpt_locked_sleep == true) {
        GPT_UNLOCK_SLEEP(gpt_port);
        g_gpt_context[gpt_port].is_gpt_locked_sleep = false;
    }
    return HAL_GPT_STATUS_OK;
}

/**************** software timer for  multiple user *************************/
hal_gpt_status_t hal_gpt_sw_get_timer(uint32_t *handle)
{
    uint32_t timer;
    uint32_t mask;

    if (gpt_sw_context.used_timer_count >= HAL_GPT_SW_NUMBER)
        return HAL_GPT_STATUS_ERROR;

    if (handle == NULL)
        return HAL_GPT_STATUS_INVALID_PARAMETER;

    /*ensure this section not to be interrupt*/
    mask = save_and_set_interrupt_mask();
    timer   = gpt_sw_get_free_timer();
    *handle = timer | HAL_GPT_SW_MAGIC;

    gpt_sw_context.timer[timer].is_used = true;

    gpt_sw_context.used_timer_count++;

    restore_interrupt_mask(mask);
    return HAL_GPT_STATUS_OK;
}

hal_gpt_status_t hal_gpt_sw_free_timer(uint32_t handle)
{
    volatile uint32_t timer;

    if ((handle & HAL_GPT_SW_MAGIC) != HAL_GPT_SW_MAGIC)
        return HAL_GPT_STATUS_INVALID_PARAMETER;

    timer = handle & HAL_GPT_SW_HANDLE_MASK;

    if (gpt_sw_context.used_timer_count == 0)
        return HAL_GPT_STATUS_ERROR;

    if (gpt_sw_context.timer[timer].is_running == true)
        return HAL_GPT_STATUS_ERROR;

    if (gpt_sw_context.timer[timer].is_used != true)
        return HAL_GPT_STATUS_ERROR;

    gpt_sw_context.timer[timer].is_used = false;
    gpt_sw_context.used_timer_count--;

    return HAL_GPT_STATUS_OK;
}

hal_gpt_status_t hal_gpt_sw_start_timer_ms(uint32_t handle, uint32_t timeout_time_ms, hal_gpt_callback_t callback, void *user_data)
{
    uint32_t current_time;
    uint32_t mask;
    uint32_t timer;

    if ((handle & HAL_GPT_SW_MAGIC) != HAL_GPT_SW_MAGIC)
        return HAL_GPT_STATUS_INVALID_PARAMETER;

    timer = handle & HAL_GPT_SW_HANDLE_MASK;

    if (callback == NULL)
        return HAL_GPT_STATUS_INVALID_PARAMETER;

    if (gpt_sw_context.timer[timer].is_running == true)
        return HAL_GPT_STATUS_ERROR;

    if (gpt_sw_context.timer[timer].is_used != true)
        return HAL_GPT_STATUS_ERROR;

    if (timeout_time_ms > HAL_GPT_MAXIMUM_MS_TIMER_TIME)
        return HAL_GPT_STATUS_INVALID_PARAMETER;

    /*ensure this section not to be interrupt*/
    mask = save_and_set_interrupt_mask();

    /* hal api to control timer*/
    gpt_sw_context.is_sw_gpt = true;

    if (gpt_sw_context.is_first_init == false) {
        hal_gpt_init(HAL_GPT_SW_PORT);
        hal_gpt_register_callback(HAL_GPT_SW_PORT, (hal_gpt_callback_t)gpt_sw_handler, NULL);
        gpt_sw_context.is_first_init = true;
    }
    hal_gpt_stop_timer(HAL_GPT_SW_PORT);

    if (gpt_sw_context.is_start_from_isr == false)
        current_time = gpt_sw_get_current_time_ms(gp_gpt[HAL_GPT_SW_PORT]);
    else
        current_time = 0;

    gpt_sw_context.absolute_time +=  current_time;
    gpt_sw_context.running_timer_count++;

    gpt_sw_context.timer[timer].is_running      = true;
    gpt_sw_context.timer[timer].time_out_ms     = gpt_sw_context.absolute_time + timeout_time_ms;
    gpt_sw_context.timer[timer].callback_context.callback   = callback;
    gpt_sw_context.timer[timer].callback_context.user_data  = user_data;

    gpt_sw_start_timer();

    /*restore mask*/
    gpt_sw_context.is_sw_gpt = false;
    restore_interrupt_mask(mask);
    return HAL_GPT_STATUS_OK;
}

hal_gpt_status_t hal_gpt_sw_stop_timer_ms(uint32_t handle)
{
    uint32_t current_time;
    uint32_t mask;
    uint32_t timer;

    if ((handle & HAL_GPT_SW_MAGIC) != HAL_GPT_SW_MAGIC)
        return HAL_GPT_STATUS_INVALID_PARAMETER;

    timer = handle & HAL_GPT_SW_HANDLE_MASK;

    if (gpt_sw_context.timer[timer].is_running != true)
        return HAL_GPT_STATUS_ERROR;

    if (gpt_sw_context.timer[timer].is_used != true)
        return HAL_GPT_STATUS_ERROR;

    /*ensure this section not to be interrupt*/
    mask = save_and_set_interrupt_mask();

    gpt_sw_context.is_sw_gpt = true;
    hal_gpt_stop_timer(HAL_GPT_SW_PORT);
    gpt_sw_context.is_sw_gpt = false;

    if (gpt_sw_context.is_start_from_isr == false)
        current_time = gpt_sw_get_current_time_ms(gp_gpt[HAL_GPT_SW_PORT]);
    else
        current_time = 0;

    gpt_sw_context.absolute_time +=  current_time;
    gpt_sw_context.running_timer_count--;

    gpt_sw_context.timer[timer].is_running = false;

    if (gpt_sw_context.running_timer_count != 0)
        gpt_sw_start_timer();

    gpt_sw_context.is_sw_gpt = false;
    restore_interrupt_mask(mask);

    return HAL_GPT_STATUS_OK;
}

hal_gpt_status_t hal_gpt_sw_get_remaining_time_ms(uint32_t handle, uint32_t *remaing_time)
{
    uint32_t current_time;
    uint32_t mask;
    uint32_t timer;

    if ((handle & HAL_GPT_SW_MAGIC) != HAL_GPT_SW_MAGIC)
        return HAL_GPT_STATUS_INVALID_PARAMETER;

    timer = handle & HAL_GPT_SW_HANDLE_MASK;

    if (gpt_sw_context.timer[timer].is_used != true)
        return HAL_GPT_STATUS_ERROR;

    /*ensure this section not to be interrupt*/
    mask = save_and_set_interrupt_mask();
    current_time = gpt_sw_context.absolute_time + gpt_get_current_time_ms(gp_gpt[HAL_GPT_SW_PORT]);

    if (gpt_sw_context.timer[timer].time_out_ms >  current_time)
        *remaing_time = gpt_sw_context.timer[timer].time_out_ms - current_time;
    else
        *remaing_time = 0;

    restore_interrupt_mask(mask);

    return HAL_GPT_STATUS_OK;

}

#endif /* #ifdef HAL_GPT_MODULE_ENABLED */

