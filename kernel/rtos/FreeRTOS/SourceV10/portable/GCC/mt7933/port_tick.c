/*
    FreeRTOS V8.2.0 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the ARM CM33 port.
 *----------------------------------------------------------*/

#include "FreeRTOS.h"
#include "port_tick.h"
#include "hal_clock.h"
#include "hal_clk.h"
#include "timers.h"

#include "hal_spm.h"
#include "hal_gpt.h"
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"

#if configUSE_TICKLESS_IDLE == 2

#define GPT_1M_DIVIDER (1000000U)

struct tickless_setting_t xDefaultTicklessConfig[] = {
    {
        .xMode = HAL_SLEEP_MODE_SLEEP,
        .ulMax_tick = 60000,
    },
    {
        .xMode = HAL_SLEEP_MODE_LEGACY_SLEEP,
        .ulMax_tick = 60000,
    },
    {
        .xMode = HAL_SLEEP_MODE_IDLE,
        .ulMax_tick = 1000,
    },
    {
        .xMode = HAL_SLEEP_MODE_NONE,
        .ulMax_tick = 0,
    },
};

void tickless_handler(uint32_t xExpectedIdleTime)
{
    int32_t xModifiableIdleTime;
    uint32_t before_sleep_time, after_sleep_time, before_sleep_time_1m, after_sleep_time_1m, after_cal_time_1m, sleep_time;
    uint32_t tick_compensation = 0U, systick_compensation;
    uint32_t ulCost_tick = 0U;
    uint32_t clk_f32k_divider = hal_clk_get_f32k_divider();
    static uint32_t cal_dur_1m = 0U;
    bool need_compensation = false;
    bool systick_irq_is_pending = false;
    hal_sleep_mode_t enter_sleep = HAL_SLEEP_MODE_NONE;
    hal_sleep_manager_status_t slp_ret = HAL_SLEEP_MANAGER_OK;
    uint32_t enter_sleep_ret = 0;
    uint32_t systick_irq_compensate = 0;

    /* Enter a critical section but don't use the taskENTER_CRITICAL()
    method as that will mask interrupts that should exit sleep mode. */
    __disable_irq();

    for (struct tickless_setting_t *s = &TICKLESS_CONFIG[0];
         enter_sleep == HAL_SLEEP_MODE_NONE && s->xMode != HAL_SLEEP_MODE_NONE; s++) {

        slp_ret = hal_sleep_manager_get_sleep_criteria(s->xMode, &ulCost_tick);
        configASSERT(slp_ret == HAL_SLEEP_MANAGER_OK);

        if (hal_sleep_manager_is_sleep_locked(s->xMode)) {
            continue;
        }

        xModifiableIdleTime = (xExpectedIdleTime > s->ulMax_tick ? s->ulMax_tick : xExpectedIdleTime);
        xModifiableIdleTime = ((xModifiableIdleTime - ulCost_tick) * (1000 / configTICK_RATE_HZ));

        if (xModifiableIdleTime <= 0) {
            continue;
        }

        /* Stop the SysTick momentarily.  */
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, (uint32_t *)&before_sleep_time);
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, (uint32_t *)&before_sleep_time_1m);

        need_compensation = true;
        systick_irq_is_pending = SCB->ICSR & SCB_ICSR_PENDSTSET_Msk;

        if (eTaskConfirmSleepModeStatus() == eAbortSleep) {
            break;
        }

        if (hal_sleep_manager_set_sleep_time(xModifiableIdleTime) != HAL_SLEEP_MANAGER_OK) {
            sleep_management_debug_sleep_log(SLEEP_MANAGEMENT_LOG_ID_ERROR, 0, 2, 0);
            break;
        }

        enter_sleep_ret = hal_sleep_manager_enter_sleep_mode(s->xMode);
        if (enter_sleep_ret < 0x10000000) { // check successfully enter sleep xMode
            enter_sleep = s->xMode;
        }
        // systick irq pending bit will be clear under following situations
        // 1. cm33 successfully enter deep sleep
        // 2. sleep abort at check point 2, 3
        if (s->xMode != HAL_SLEEP_MODE_IDLE && (enter_sleep_ret == 0xE0000002 || enter_sleep_ret == 0xE0000003 || enter_sleep_ret < 0x10000000)) {
            systick_irq_compensate = (systick_irq_is_pending ? 1 : 0);
        }
    }

    if (enter_sleep != HAL_SLEEP_MODE_NONE) {
        configPOST_SLEEP_PROCESSING(xExpectedIdleTime);
    }
    if (need_compensation) {
        //calculate time(systick) to jump
        uint32_t ref_clk_divider;

        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &after_sleep_time);
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &after_sleep_time_1m);

        if (enter_sleep == HAL_SLEEP_MODE_IDLE) {
            hal_gpt_get_duration_count(before_sleep_time_1m, after_sleep_time_1m, &sleep_time);
            ref_clk_divider = GPT_1M_DIVIDER;
        } else {
            hal_gpt_get_duration_count(before_sleep_time, after_sleep_time, &sleep_time);
            ref_clk_divider = clk_f32k_divider;
        }

        tick_compensation = (sleep_time * configTICK_RATE_HZ) / ref_clk_divider;
        systick_compensation = ((sleep_time * configTICK_RATE_HZ - tick_compensation * ref_clk_divider) * (configCPU_CLOCK_HZ / configTICK_RATE_HZ / ref_clk_divider)) + (cal_dur_1m * configCPU_CLOCK_HZ / GPT_1M_DIVIDER);
        tick_compensation += systick_irq_compensate;

        if (SysTick->VAL < systick_compensation) {
            vTaskStepTick(tick_compensation + 1);
            SysTick->LOAD = (configCPU_CLOCK_HZ / configTICK_RATE_HZ) - systick_compensation + SysTick->VAL;
        } else {
            vTaskStepTick(tick_compensation);
            SysTick->LOAD = SysTick->VAL - systick_compensation;
        }

        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &after_cal_time_1m);
        hal_gpt_get_duration_count(after_sleep_time_1m, after_cal_time_1m, &cal_dur_1m);

        SysTick->VAL = 0;
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
        SysTick->LOAD = (configCPU_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;
    }

    if (enter_sleep >= HAL_SLEEP_MODE_SLEEP) {
        sleep_management_debug_sleep_log(SLEEP_MANAGEMENT_LOG_ID_TICKLESS_WFI + (enter_sleep-HAL_SLEEP_MODE_IDLE), xExpectedIdleTime, tick_compensation, enter_sleep_ret);
    }

    __enable_irq();
}
#endif /* #if configUSE_TICKLESS_IDLE == 2 */
