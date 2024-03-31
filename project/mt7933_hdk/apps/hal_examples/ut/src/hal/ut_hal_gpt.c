/* Copyright Statement:
 *
 * (C) 2005-2030  MediaTek Inc. All rights reserved.
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
#include <stdio.h>
#include <string.h>
#include "ut.h"

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_GPT_MODULE_ENABLE)
#include "hal_gpt.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
extern hal_gpt_status_t hal_gpt_start_timer(hal_gpt_port_t gpt_port);
extern hal_gpt_status_t hal_gpt_start_timer_30us(hal_gpt_port_t gpt_port, uint32_t timeout_time_us, hal_gpt_timer_type_t timer_type);
extern hal_gpt_status_t hal_gpt_get_free_run_count_64(hal_gpt_clock_source_t clock_source, uint64_t *count);
void user_gpt_1_callback(void *user_data)
{
    printf("GPT_1 callback come \r\n");

    hal_gpt_stop_timer(HAL_GPT_1);
    //hal_gpt_deinit(HAL_GPT_1);
}

void user_gpt_2_callback(void *user_data)
{
    printf("GPT_2 callback come \r\n");

    hal_gpt_stop_timer(HAL_GPT_2);
    hal_gpt_deinit(HAL_GPT_2);
}

void sw_gpt_callback(void *user_data)
{
    uint32_t handle;

    handle = *(uint32_t *)user_data;

    printf("SW_GPT callback come, handle = 0x%lX  \r\n", handle);

    hal_gpt_sw_stop_timer_ms(handle);
    hal_gpt_sw_free_timer(handle);
}

ut_status_t ut_hal_gpt(void)
{
    hal_gpt_running_status_t    running_status;
    hal_gpt_status_t            ret_status;
    uint32_t                    duration_count;
    static uint32_t             sw_gpt_handle;
    uint32_t                    remain_time;
    uint32_t                    count1, count2;
    uint32_t                    time;
    uint64_t                    count_64bit;
    //int ret = 0;

    if (HAL_GPT_STATUS_OK != hal_gpt_get_running_status(HAL_GPT_1, &running_status)) {
        printf("GPT test fail, get running status.\r\n");
        return UT_STATUS_ERROR;
    }

    if (HAL_GPT_STATUS_OK != hal_gpt_init(HAL_GPT_1)) {
        printf("GPT test fail, gpt init GPT_1 .\r\n");
        return UT_STATUS_ERROR;
    }

    printf("GPT test start, 100ms duration one shot. \r\n");
    hal_gpt_register_callback(HAL_GPT_1, user_gpt_1_callback, NULL);
    hal_gpt_start_timer_ms(HAL_GPT_1, 100, HAL_GPT_TIMER_TYPE_ONE_SHOT);

    ret_status = hal_gpt_delay_ms(100);
    printf("GPT delay 100 ms. ret = %d \r\n", ret_status);

    ret_status = hal_gpt_start_timer(HAL_GPT_1);

    if (HAL_GPT_STATUS_OK != ret_status) {
        printf("GPT test stop timer fail, ret = %d.\r\n", ret_status);

        return UT_STATUS_ERROR;
    }

    ret_status = hal_gpt_delay_ms(200);
    printf("GPT delay 200 ms. ret = %d \r\n", ret_status);

    hal_gpt_deinit(HAL_GPT_1);

#ifdef HAL_GPT_FEATURE_US_TIMER
    if (HAL_GPT_STATUS_OK != hal_gpt_init(HAL_GPT_2)) {
        printf("GPT test fail, gpt init GPT_2 .\r\n");
        return UT_STATUS_ERROR;
    }
    printf("GPT test start, 10*1000 us duration. \r\n");
    hal_gpt_register_callback(HAL_GPT_2, user_gpt_2_callback, NULL);
    hal_gpt_start_timer_us(HAL_GPT_2, 10 * 1000, HAL_GPT_TIMER_TYPE_ONE_SHOT);

    ret_status = hal_gpt_delay_us(10000);
    printf("GPT delay 10000 us. ret = %d \r\n", ret_status);
#endif /* #ifdef HAL_GPT_FEATURE_US_TIMER */

#ifdef HAL_GPT_FEATURE_US_TIMER
    if (HAL_GPT_STATUS_OK != hal_gpt_init(HAL_GPT_2)) {
        printf("GPT test fail, gpt init GPT_2 .\r\n");
        return UT_STATUS_ERROR;
    }
    printf("GPT test start, 30*1000 us duration. \r\n");
    hal_gpt_register_callback(HAL_GPT_2, user_gpt_2_callback, NULL);
    hal_gpt_start_timer_30us(HAL_GPT_2, 30 * 1000, HAL_GPT_TIMER_TYPE_ONE_SHOT);

    ret_status = hal_gpt_delay_us(30000);
    printf("GPT delay 10000 us. ret = %d \r\n", ret_status);
#endif /* #ifdef HAL_GPT_FEATURE_US_TIMER */

    ret_status = hal_gpt_get_duration_count(0xffff0000, 0x0000ffff, &duration_count);
    printf("GPT get duration = %ld, ret = %d \r\n", duration_count, ret_status);

#ifdef HAL_GPT_SW_GPT_FEATURE
    if (HAL_GPT_STATUS_OK != hal_gpt_sw_get_timer(&sw_gpt_handle)) {
        printf("GPT test fail, sw_get_timer .\r\n");
        return UT_STATUS_ERROR;
    }

    printf("SW_GPT get handle = 0x%lX \r\n", sw_gpt_handle);

    hal_gpt_sw_start_timer_ms(sw_gpt_handle, 3000, sw_gpt_callback, &sw_gpt_handle);

    //ret_status = hal_gpt_delay_ms(100);

    ret_status = hal_gpt_sw_get_remaining_time_ms(sw_gpt_handle, &remain_time);

    printf("SW_GPT get remaining time ms = %ld, ret = %d \r\n", remain_time, ret_status);
#endif /* #ifdef HAL_GPT_SW_GPT_FEATURE */

    if (HAL_GPT_STATUS_OK != hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &count1)) {
        printf("GPT test fail, get_free_run_count .\r\n");
        return UT_STATUS_ERROR;
    }

    hal_gpt_delay_ms(100);

    if (HAL_GPT_STATUS_OK != hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &count2)) {
        printf("GPT test fail, get_free_run_count .\r\n");
        return UT_STATUS_ERROR;
    }

    if (HAL_GPT_STATUS_OK != hal_gpt_get_duration_count(count1, count2, &duration_count)) {
        printf("GPT test fail, get_duration_count .\r\n");
        return UT_STATUS_ERROR;
    }

    time = (duration_count * 1000) / 32768;

    printf("GPT test free_run_count 32K clk source, time = %ld ms, duration_count = 0x%lX .\r\n", time, duration_count);

    if (HAL_GPT_STATUS_OK != hal_gpt_get_free_run_count_64(HAL_GPT_CLOCK_SOURCE_32K, &count_64bit)) {
        printf("GPT test fail, get_free_run_count_64 .\r\n");
        return UT_STATUS_ERROR;

    }
    count1 = (uint32_t)count_64bit;
    count2 = (uint32_t)(count_64bit >> 32);

    printf("GPT test free_run_count_64, clock = %d, count = 0x%lX, count_H = 0x%lX . \r\n", HAL_GPT_CLOCK_SOURCE_32K, count1, count2);

    if (HAL_GPT_STATUS_OK != hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &count1)) {
        printf("GPT test fail, get_free_run_count .\r\n");
        return UT_STATUS_ERROR;
    }

    hal_gpt_delay_us(10);

    if (HAL_GPT_STATUS_OK != hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &count2)) {
        printf("GPT test fail, get_free_run_count .\r\n");
        return UT_STATUS_ERROR;
    }

    if (HAL_GPT_STATUS_OK != hal_gpt_get_duration_count(count1, count2, &duration_count)) {
        printf("GPT test fail, get_duration_count .\r\n");
        return UT_STATUS_ERROR;
    }

    time = duration_count;

    printf("GPT test free_run_count 1M clk source, time = %ld ms, duration_count = 0x%lX .\r\n", time, duration_count);


    if (HAL_GPT_STATUS_OK != hal_gpt_get_free_run_count_64(HAL_GPT_CLOCK_SOURCE_1M, &count_64bit)) {
        printf("GPT test fail, get_free_run_count_64 .\r\n");
        return UT_STATUS_ERROR;

    }
    count1 = (uint32_t)count_64bit;
    count2 = (uint32_t)(count_64bit >> 32);

    printf("GPT test free_run_count_64, clock = %d, count = 0x%lX, count_H = 0x%lX . \r\n", HAL_GPT_CLOCK_SOURCE_1M, count1, count2);

    printf("\r\nhal_gpt UT OK\r\n\r\n");

    return UT_STATUS_NOT_SUPPORT;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_GPT_MODULE_ENABLE) */
