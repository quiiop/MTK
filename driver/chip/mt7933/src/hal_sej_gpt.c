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
 */
#include "hal.h"

#ifdef HAL_SEJ_GPT_MODULE_ENABLED
#include <common.h>
#include "hal_sej_gpt.h"
#include "driver_api.h"
#include "hal_nvic_internal.h"
#include "memory_attribute.h"

#ifdef HAL_SLEEP_MANAGER_ENABLED
const  char *sej_gpt_lock_sleep_name[HAL_SEJ_GPT_MAX_PORT] = {"GPT0", "GPT1"};
static uint8_t sej_gpt_lock_sleep_handle[HAL_SEJ_GPT_MAX_PORT];

#define SEJ_GPT_GET_SLEEP_HANDLE(gpt_port) \
do{ \
    sej_gpt_lock_sleep_handle[gpt_port] = hal_sleep_manager_set_sleep_handle(sej_gpt_lock_sleep_name[gpt_port]); \
}while(0)

#define SEJ_GPT_RELEASE_SLEEP_HANDLE(gpt_port) \
do{ \
    hal_sleep_manager_release_sleep_handle(sej_gpt_lock_sleep_handle[gpt_port]); \
}while(0)

#define SEJ_GPT_LOCK_SLEEP(gpt_port) \
do{ \
    hal_sleep_manager_lock_sleep(sej_gpt_lock_sleep_handle[gpt_port]); \
}while(0)

#define SEJ_GPT_UNLOCK_SLEEP(gpt_port) \
do{ \
    hal_sleep_manager_unlock_sleep(sej_gpt_lock_sleep_handle[gpt_port]); \
}while(0)

#else /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
#define SEJ_GPT_GET_SLEEP_HANDLE(gpt_port)
#define SEJ_GPT_RELEASE_SLEEP_HANDLE(gpt_port)
#define SEJ_GPT_LOCK_SLEEP(gpt_port)
#define SEJ_GPT_UNLOCK_SLEEP(gpt_port)
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

ATTR_ZIDATA_IN_TCM sej_gpt_context_t g_sej_gpt_context[HAL_SEJ_GPT_MAX_PORT];

static bool hal_sej_gpt_is_port_valid(hal_sej_gpt_port_t gpt_port)
{
    if ((gpt_port < HAL_SEJ_GPT_MAX_PORT) && (gpt_port >= 0))
        return true;
    else
        return false;
}

ATTR_TEXT_IN_SYSRAM uint32_t convert_ms_to_13m_count(uint32_t ms)
{
    return (uint32_t)(ms * 13000);
}

ATTR_TEXT_IN_TCM uint32_t sej_gpt_curr_count_l(hal_sej_gpt_port_t gpt_port)
{
    if (gpt_port == HAL_SEJ_GPT0)
        return DRV_Reg32(SEJ_GPT0_COUNTER);
    else if (gpt_port == HAL_SEJ_GPT1)
        return DRV_Reg32(SEJ_GPT1_COUNTER0);
    else {
        log_hal_info("[SEJ_GPT%d] cnt_l read failed\r\n", (int)gpt_port);
        return 0;
    }
}

ATTR_TEXT_IN_TCM uint32_t sej_gpt_curr_count_h(hal_sej_gpt_port_t gpt_port)
{
    if (gpt_port != HAL_SEJ_GPT1) {
        log_hal_info("[SEJ_GPT%d] cnt_h read failed\r\n", (int)gpt_port);
        return 0;
    }

    return DRV_Reg32(SEJ_GPT1_COUNTER1);
}

static void reset_sej_timer0(void)
{
    /*clr counter & disable GPT*/
    DRV_WriteReg32(SEJ_GPT0_CON, BIT(1));
    /*clr irq*/
    DRV_SetReg32(SEJ_GPT_IRQACK, BIT(0));
    /*clr compare value*/
    DRV_WriteReg32(SEJ_GPT0_COMPARE, 0x0);
}

static void reset_sej_timer1(void)
{
    /*clr counter & disable GPT*/
    DRV_WriteReg32(SEJ_GPT1_CON, BIT(1));
    /*clr irq*/
    DRV_SetReg32(SEJ_GPT_IRQACK, BIT(1));
    /*clr compare value*/
    DRV_WriteReg32(SEJ_GPT1_COMPARE0, 0x0);
    DRV_WriteReg32(SEJ_GPT1_COMPARE1, 0x0);
}

static void sej_gpt_reset_timer(hal_sej_gpt_port_t gpt_port)
{
    if (gpt_port == HAL_SEJ_GPT0)
        reset_sej_timer0();
    else if (gpt_port == HAL_SEJ_GPT1)
        reset_sej_timer1();
    else {
        log_hal_info("[SEJ_GPT%d]reset failed\r\n", (int)gpt_port);
    }
}
#if 0
static int32_t gpt_get_irq_number_from_port(hal_gpt_port_t port)
{
    switch (port) {
        case HAL_SEJ_GPT0:
            return APXGPT0_IRQn;
        case HAL_SEJ_GPT1:
            return APXGPT1_IRQn;
        default:
            return HAL_GPT_STATUS_ERROR_PORT;
    }
}

uint32_t gpt_get_port_from_irq_number(hal_nvic_irq_t irq)
{
    switch (irq) {
        case APXGPT0_IRQn: //SEJ_APXGPT_IRQn
            return HAL_SEJ_GPT0;
        case APXGPT1_IRQn: //SEJ_APXGPT_IRQn
            return HAL_SEJ_GPT1;
        default:
            return HAL_GPT_STATUS_INVALID_PARAMETER;
    }
}
#endif /* #if 0 */

void sej_gpt_interrupt_handler(hal_nvic_irq_t irq_number)
{
    volatile uint32_t state;
    volatile uint32_t enable;
    //uint32_t port = gpt_get_port_from_irq_number(irq_number);
    uint32_t port = HAL_SEJ_GPT0;

    /* get gpt irq status */
    state  = DRV_Reg32(SEJ_GPT_IRQSTA);
    enable = DRV_Reg32(SEJ_GPT_IRQEN);

    /* Mask/disable irq */
    DRV_ClrReg32(SEJ_GPT_IRQEN, BIT(port));

    /* clr irq */
    DRV_SetReg32(SEJ_GPT_IRQACK, BIT(port));

    /* if irq pending & enable */
    if ((state & BIT(port)) && (enable & BIT(port))) {
#ifdef SEJ_GPT_DEBUG_LOG
        log_hal_info("[SEJ_GPT%d]IRQ_STA = 0x%x\r\n", (int)port, (int)state);
#endif /* #ifdef SEJ_GPT_DEBUG_LOG */

        if (g_sej_gpt_context[port].callback_context.callback != NULL) {
            g_sej_gpt_context[port].callback_context.callback(g_sej_gpt_context[port].callback_context.user_data);
        }
    }
    DRV_WriteReg32(SEJ_GPT_IRQEN, enable);
}

static void sej_gpt_nvic_register(hal_sej_gpt_port_t port)
{
    //int32_t irq = sej_gpt_get_irq_number_from_port(port);
    int32_t irq = SEJ_APXGPT_IRQn;
    hal_nvic_disable_irq(irq);
    hal_nvic_register_isr_handler(irq, sej_gpt_interrupt_handler);
    hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    hal_nvic_enable_irq(irq);
}

hal_sej_gpt_status_t hal_sej_gpt_init(hal_sej_gpt_port_t gpt_port)
{

    if ((hal_sej_gpt_is_port_valid(gpt_port) != true) || (gpt_port < 0))
        return HAL_SEJ_GPT_ERROR_PORT;

    if ((g_sej_gpt_context[gpt_port].running_status == HAL_SEJ_GPT_RUNNING) ||
        (g_sej_gpt_context[gpt_port].has_initilized == true)) {
        return HAL_SEJ_GPT_ERROR;
    }

    /*Enable clock */
    if (hal_clock_enable(HAL_CLOCK_CG_CRYPTO_DIV2_XTAL) != HAL_CLOCK_STATUS_OK) {
        return HAL_SEJ_GPT_ERROR_CLOCK;
    }

    /*set structure to 0 */
    memset(&g_sej_gpt_context[gpt_port], 0, sizeof(sej_gpt_context_t));

    /*set flag respect this port has initlized */
    g_sej_gpt_context[gpt_port].has_initilized = true;

    /*disable interrupt */
    DRV_ClrReg32(SEJ_GPT_IRQEN, BIT(gpt_port));

    SEJ_GPT_GET_SLEEP_HANDLE(gpt_port);
#ifdef SEJ_GPT_DEBUG_LOG
    log_hal_info("[SEJ_GPT%d] init OK\r\n", (int)gpt_port);
#endif /* #ifdef SEJ_GPT_DEBUG_LOG */
    return HAL_SEJ_GPT_OK;
}

hal_sej_gpt_status_t hal_sej_gpt_deinit(hal_sej_gpt_port_t gpt_port)
{
    uint8_t clock_disable = 1;

    if ((hal_sej_gpt_is_port_valid(gpt_port) != true) || (gpt_port < 0))
        return HAL_SEJ_GPT_ERROR_PORT;

    if (g_sej_gpt_context[gpt_port].running_status == HAL_SEJ_GPT_RUNNING)
        return HAL_SEJ_GPT_ERROR;

    /* set structure to 0 */
    memset(&g_sej_gpt_context[gpt_port], 0, sizeof(sej_gpt_context_t));

    /* set flag indicate this port has deinitlized */
    g_sej_gpt_context[gpt_port].has_initilized = false;

    sej_gpt_reset_timer(gpt_port);

    SEJ_GPT_RELEASE_SLEEP_HANDLE(gpt_port);

    for (uint32_t gpt_idx = 0; gpt_idx < HAL_SEJ_GPT_MAX_PORT; ++gpt_idx) {
        if (g_sej_gpt_context[gpt_port].has_initilized) {
            // one of the gpt is not deinit, so the clock cannot be disabled.
            clock_disable = 0;
        }
    }

    if (clock_disable) {
        hal_clock_disable(HAL_CLOCK_CG_CRYPTO_DIV2_XTAL);
    }

#ifdef SEJ_GPT_DEBUG_LOG
    log_hal_info("[SEJ_GPT%d] deinit OK\r\n", (int)gpt_port);
#endif /* #ifdef SEJ_GPT_DEBUG_LOG */

    return HAL_SEJ_GPT_OK;
}

#if 0
/* API to manage over-run of 32bit HW counter */
static uint64_t _last_tick_count = 0;    /* use last-tick-count to detect roll-over */
static uint64_t _accumulated_tick_count = 0; /* this is accumulated tick counts include roll-over */
static uint32_t  _first_time = 1;

static uint64_t sej_get_accumulated_count(hal_sej_gpt_port_t gpt_port)
{
    uint32_t delta;
    uint64_t result;
    uint32_t mask;
    uint64_t count, count_h;

    if ((gpt_port >= HAL_SEJ_GPT_MAX_PORT) || (gpt_port < 0))
        return 0;

    /* disable interrupt */
    hal_nvic_save_and_set_interrupt_mask(&mask);
    count = sej_gpt_curr_count_l(gpt_port);

    if (gpt_port == HAL_SEJ_GPT1) {
        /* GPT_1 is 64bit timer */
        count_h = sej_gpt_curr_count_h(gpt_port);
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

ATTR_TEXT_IN_SYSRAM hal_sej_gpt_status_t hal_sej_gpt_get_free_run_count_64(hal_gpt_clock_source_t clock_source, uint64_t *count)
{
    /* millisecond free run timer */
    if (clock_source == HAL_GPT_CLOCK_SOURCE_32K) {
        if (g_sej_gpt_context[HAL_GPT_MS_PORT].running_status != HAL_SEJ_GPT_RUNNING) {

            /* set clock source to 32khz, and start timer */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_MS_PORT], GPT_CLOCK_32KHZ, GPT_DIVIDE_1);

            g_sej_gpt_context[HAL_GPT_MS_PORT].running_status = HAL_SEJ_GPT_RUNNING;
        }

        *count = sej_get_accumulated_count(HAL_GPT_MS_PORT);
    } /* microsecond free rum timer */
    else if (clock_source == HAL_GPT_CLOCK_SOURCE_1M) {
        if (g_sej_gpt_context[HAL_GPT_US_PORT].running_status != HAL_SEJ_GPT_RUNNING) {
#ifdef MTK_FPGA_ENABLE
            /* set clcok source to 1mhz, and start timer, gpt clk is 6mhz on fpga */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_6MHZ, GPT_DIVIDE_6);
#else /* #ifdef MTK_FPGA_ENABLE */
            /* set clcok source to 1mhz, and start timer */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_13MHZ, GPT_DIVIDE_13);
#endif /* #ifdef MTK_FPGA_ENABLE */

            g_sej_gpt_context[HAL_GPT_US_PORT].running_status = HAL_SEJ_GPT_RUNNING;
        }
        *count = sej_get_accumulated_count(HAL_GPT_US_PORT);
    } else {
        return HAL_SEJ_GPT_INVALID_PARAMETER;
    }

    return HAL_SEJ_GPT_OK;
}

ATTR_TEXT_IN_TCM hal_sej_gpt_status_t hal_sej_gpt_get_free_run_count(hal_gpt_clock_source_t clock_source, uint32_t *count)
{
    /* millisecond free run timer */
    if (clock_source == HAL_GPT_CLOCK_SOURCE_32K) {
        if (g_sej_gpt_context[HAL_GPT_MS_PORT].running_status != HAL_SEJ_GPT_RUNNING) {

            /* set clock source to 32khz, and start timer */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_MS_PORT], GPT_CLOCK_32KHZ, GPT_DIVIDE_1);

            g_sej_gpt_context[HAL_GPT_MS_PORT].running_status = HAL_SEJ_GPT_RUNNING;
        }

        *count = sej_gpt_curr_count_l(HAL_GPT_MS_PORT);
    } /* microsecond free rum timer */
    else if (clock_source == HAL_GPT_CLOCK_SOURCE_1M) {
        if (g_sej_gpt_context[HAL_GPT_US_PORT].running_status != HAL_SEJ_GPT_RUNNING) {
#ifdef MTK_FPGA_ENABLE
            /* set clcok source to 1mhz, and start timer, gpt clk is 6mhz on fpga */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_6MHZ, GPT_DIVIDE_6);
#else /* #ifdef MTK_FPGA_ENABLE */
            /* set clcok source to 1mhz, and start timer */
            gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_13MHZ, GPT_DIVIDE_13);
#endif /* #ifdef MTK_FPGA_ENABLE */

            g_sej_gpt_context[HAL_GPT_US_PORT].running_status = HAL_SEJ_GPT_RUNNING;

        }
        *count = sej_gpt_curr_count_l(HAL_GPT_US_PORT);
    } else {
        return HAL_SEJ_GPT_INVALID_PARAMETER;
    }

    return HAL_SEJ_GPT_OK;
}

ATTR_TEXT_IN_TCM hal_sej_gpt_status_t hal_sej_gpt_get_duration_count(uint32_t start_count, uint32_t end_count, uint32_t *duration_count)
{
    if (duration_count == NULL)
        return HAL_SEJ_GPT_INVALID_PARAMETER;

    if (end_count > start_count)
        *duration_count = end_count - start_count;
    else
        *duration_count = (0xffffffff - (start_count - end_count)) + 1;

    return HAL_SEJ_GPT_OK;
}

hal_sej_gpt_status_t hal_sej_gpt_get_running_status(hal_sej_gpt_port_t gpt_port, hal_gpt_running_status_t *running_status)
{
    if ((gpt_port >= HAL_SEJ_GPT_MAX_PORT) || (gpt_port < 0))
        return HAL_SEJ_GPT_ERROR_PORT;

    *running_status = g_sej_gpt_context[gpt_port].running_status;

    return HAL_SEJ_GPT_OK;
}
#endif /* #if 0 */
hal_sej_gpt_status_t hal_sej_gpt_register_callback(hal_sej_gpt_port_t gpt_port,
                                                   hal_gpt_callback_t   callback,
                                                   void                *user_data)
{
    if ((hal_sej_gpt_is_port_valid(gpt_port) != true) || (gpt_port < 0))
        return HAL_SEJ_GPT_ERROR_PORT;

    if ((g_sej_gpt_context[gpt_port].running_status == HAL_SEJ_GPT_RUNNING) ||
        (g_sej_gpt_context[gpt_port].has_initilized != true)) {
        return HAL_SEJ_GPT_ERROR;
    }

    if (callback == NULL)
        return HAL_SEJ_GPT_INVALID_PARAMETER;

#ifdef SEJ_GPT_DEBUG_LOG
    log_hal_info("[SEJ_GPT%d] register callback:0x%.8x\r\n", (unsigned int)gpt_port, (unsigned int)callback);
#endif /* #ifdef SEJ_GPT_DEBUG_LOG */
    g_sej_gpt_context[gpt_port].callback_context.callback  = callback;
    g_sej_gpt_context[gpt_port].callback_context.user_data = user_data;

    sej_gpt_nvic_register(gpt_port);

    return HAL_SEJ_GPT_OK;
}

static void start_timer0_ms(uint32_t timeout_time_ms, sej_gpt_mode_t timer_type)
{
    /*disable irq*/
    DRV_ClrReg32(SEJ_GPT_IRQEN, BIT(0));
    /*stop timer*/
    DRV_ClrReg32(SEJ_GPT0_CON, BIT(0));
    /* set to 13M clock and 1 division*/
    DRV_ClrReg32(SEJ_GPT0_PRESCALE, 0x1F);
    /* set timeout value */
    DRV_WriteReg32(SEJ_GPT0_COMPARE, convert_ms_to_13m_count(timeout_time_ms));
    /* clear counter */
    DRV_SetReg32(SEJ_GPT0_CON, BIT(1));
    /*clr irq*/
    DRV_SetReg32(SEJ_GPT_IRQACK, BIT(0));
    while (sej_gpt_curr_count_l(HAL_SEJ_GPT0));

    /* set mode*/
    switch (timer_type) {
        case HAL_GPT_TIMER_TYPE_ONE_SHOT:
            DRV_ClrReg32(SEJ_GPT0_CON, 0x30);
            break;
        case HAL_GPT_TIMER_TYPE_REPEAT:
            DRV_ClrReg32(SEJ_GPT0_CON, BIT(5));
            DRV_SetReg32(SEJ_GPT0_CON, BIT(4));
            break;
        default:
            log_hal_info("%s: timer_type err\n", __func__);
            break;
    }
    /* start counter */
    DRV_SetReg32(SEJ_GPT0_CON, BIT(0));
    /*enable irq*/
    DRV_SetReg32(SEJ_GPT_IRQEN, BIT(0));
}

hal_sej_gpt_status_t hal_sej_gpt_start_timer_ms(hal_sej_gpt_port_t gpt_port,
                                                uint32_t timeout_time_ms,
                                                sej_gpt_mode_t timer_type)
{
    volatile uint32_t mask;

    if ((hal_sej_gpt_is_port_valid(gpt_port) != true) || (gpt_port < 0))
        return HAL_SEJ_GPT_ERROR_PORT;

    if ((g_sej_gpt_context[gpt_port].running_status == HAL_SEJ_GPT_RUNNING)
        || (g_sej_gpt_context[gpt_port].has_initilized != true)) {

        return HAL_SEJ_GPT_ERROR_PORT_USED;
    }

    if (timeout_time_ms > HAL_SEJ_GPT_MAX_MS)
        return HAL_SEJ_GPT_INVALID_PARAMETER;

#ifdef SEJ_GPT_DEBUG_LOG
    log_hal_info("[SEJ_GPT%d]hal_gpt_start_timer_ms, time=%d ms,type=%d\r\n",
                 (int)gpt_port, (int)timeout_time_ms, (int)timer_type);
#endif /* #ifdef SEJ_GPT_DEBUG_LOG */

    mask = save_and_set_interrupt_mask();

    if (gpt_port == HAL_SEJ_GPT0)
        start_timer0_ms(timeout_time_ms, timer_type);
    else {
        log_hal_info("[SEJ_GPT%d]hal_gpt_start_timer_ms not support yet\r\n", (int)gpt_port);
    }

    g_sej_gpt_context[gpt_port].running_status = HAL_SEJ_GPT_RUNNING;
    restore_interrupt_mask(mask);

    return HAL_SEJ_GPT_OK;
}
#if 0
ATTR_TEXT_IN_SYSRAM hal_sej_gpt_status_t hal_sej_gpt_delay_ms(uint32_t ms)
{
    return hal_sej_gpt_delay_us(ms * 1000);
}

#ifdef HAL_GPT_FEATURE_US_TIMER
hal_sej_gpt_status_t hal_sej_gpt_start_timer_30us(hal_sej_gpt_port_t gpt_port,
                                                  uint32_t timeout_time_us, sej_gpt_mode_t timer_type)
{
    uint32_t aligned_timeout;

    aligned_timeout = (timeout_time_us + 29) / 30 * 30;

    return hal_gpt_start_timer_us(gpt_port, aligned_timeout, timer_type);
}

hal_sej_gpt_status_t hal_sej_pt_start_timer_us(hal_sej_gpt_port_t gpt_port,
                                               uint32_t timeout_time_us, sej_gpt_mode_t timer_type)
{
    volatile uint32_t mask;

    if ((hal_sej_gpt_is_port_valid(gpt_port) != true) || (gpt_port < 0)) {
        //log_hal_error("Invalid port: %d. Only port 0 or 1 works as timer.", gpt_port);
        return HAL_SEJ_GPT_ERROR_PORT;
    }

    if ((g_sej_gpt_context[gpt_port].running_status == HAL_SEJ_GPT_RUNNING)
        || (g_sej_gpt_context[gpt_port].has_initilized != true)) {
        return HAL_SEJ_GPT_ERROR_PORT_USED;
    }

#ifdef SEJ_GPT_DEBUG_LOG
    if (gpt_port !=  HAL_GPT_SW_PORT) {
        log_hal_info("[SEJ_GPT%d]hal_gpt_start_timer_us, time=%d us,type=%d\r\n",
                     (int)gpt_port, (int)timeout_time_us, (int)timer_type);
    }
#endif /* #ifdef SEJ_GPT_DEBUG_LOG */

    SEJ_GPT_LOCK_SLEEP(gpt_port);
    g_sej_gpt_context[gpt_port].is_gpt_locked_sleep = true;

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

    restore_interrupt_mask(mask);

    g_sej_gpt_context[gpt_port].running_status = HAL_SEJ_GPT_RUNNING;

    return HAL_SEJ_GPT_OK;
}

ATTR_TEXT_IN_TCM hal_sej_gpt_status_t hal_sej_gpt_delay_us(uint32_t us)
{
    /* if free run timer is not open, open it */
    if (g_sej_gpt_context[HAL_GPT_US_PORT].running_status != HAL_SEJ_GPT_RUNNING) {

#ifdef MTK_FPGA_ENABLE
        /* fpga clk source is 6mhz, set clcok source to 1mhz, and start timer */
        gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_6MHZ, GPT_DIVIDE_6);
#else /* #ifdef MTK_FPGA_ENABLE */
        /* set clcok source to 1mhz, and start timer */
        gpt_start_free_run_timer(gp_gpt[HAL_GPT_US_PORT], GPT_CLOCK_13MHZ, GPT_DIVIDE_13);
#endif /* #ifdef MTK_FPGA_ENABLE */
        g_sej_gpt_context[HAL_GPT_US_PORT].running_status = HAL_SEJ_GPT_RUNNING;
    }
    gpt_delay_time(gp_gpt[HAL_GPT_US_PORT], us);

    return HAL_SEJ_GPT_OK;
}
#endif /* #ifdef HAL_GPT_FEATURE_US_TIMER */

hal_sej_gpt_status_t hal_sej_gpt_start_timer(hal_sej_gpt_port_t gpt_port)
{
    volatile uint32_t mask;

    if ((hal_sej_gpt_is_port_valid(gpt_port) != true) || (gpt_port < 0))
        return HAL_SEJ_GPT_ERROR_PORT;

#ifdef SEJ_GPT_DEBUG_LOG
    if (gpt_port !=  HAL_GPT_SW_PORT)
        log_hal_info("[SEJ_GPT%d]hal_gpt_start_timer\r\n", (int)gpt_port);
#endif /* #ifdef SEJ_GPT_DEBUG_LOG */

    if (g_sej_gpt_context[gpt_port].running_status != HAL_GPT_STOPPED)
        return HAL_SEJ_GPT_ERROR;

    mask = save_and_set_interrupt_mask();

    /*Enable interrupt*/
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQCLR = GPT_IRQ_FLAG_ACK;
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.IRQEN = GPT_IRQ_ENABLE;

    /* start timer */
    gp_gpt[gpt_port]->GPT_CON_UNION.GPT_CON_CELLS.EN = GPT_COUNT_START;

    g_sej_gpt_context[gpt_port].running_status = HAL_SEJ_GPT_RUNNING;

    restore_interrupt_mask(mask);

    if (g_sej_gpt_context[gpt_port].is_gpt_locked_sleep == false) {
        SEJ_GPT_LOCK_SLEEP(gpt_port);
        g_sej_gpt_context[gpt_port].is_gpt_locked_sleep = true;
    }

    return HAL_SEJ_GPT_OK;
}

#endif /* #if 0 */
hal_sej_gpt_status_t hal_sej_gpt_stop_timer(hal_sej_gpt_port_t gpt_port)
{
    volatile uint32_t mask;

    if ((hal_sej_gpt_is_port_valid(gpt_port) != true) || (gpt_port < 0))
        return HAL_SEJ_GPT_ERROR_PORT;

#ifdef SEJ_GPT_DEBUG_LOG
    log_hal_info("[SEJ_GPT%d]hal_gpt_stop_timer\r\n", (int)gpt_port);
#endif /* #ifdef SEJ_GPT_DEBUG_LOG */

    mask = save_and_set_interrupt_mask();

    /*diable interrupt*/
    DRV_ClrReg32(SEJ_GPT_IRQEN, BIT(gpt_port));

    sej_gpt_reset_timer(gpt_port);
    g_sej_gpt_context[gpt_port].running_status = (hal_sej_gpt_running_status_t)HAL_GPT_STOPPED;
    restore_interrupt_mask(mask);

    if (g_sej_gpt_context[gpt_port].is_gpt_locked_sleep == true) {
        SEJ_GPT_UNLOCK_SLEEP(gpt_port);
        g_sej_gpt_context[gpt_port].is_gpt_locked_sleep = false;
    }
    return HAL_SEJ_GPT_OK;
}
#endif /* #ifdef HAL_SEJ_GPT_MODULE_ENABLED */

