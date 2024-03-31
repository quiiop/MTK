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
#include "ci.h"
#include "ci_cli.h"
#include "hal.h"
#include "hal_gpt.h"

#define TIMER_TMO_MS    100
#define TOLERANCE_MS    10
#define TOLERANCE_US    50
#ifdef HAL_GPT_SW_GPT_FEATURE
#define SW_TIMER_TMO_MS    3000
#endif /* #ifdef HAL_GPT_SW_GPT_FEATURE */

uint32_t start_cnt = 0, end_cnt = 0;
static uint32_t irq_flag = 0;
hal_gpt_status_t sta_ok = HAL_GPT_STATUS_OK;


ci_status_t ci_gpt_delay_ms_sample(void)
{
    uint32_t time_ms = 0, tmo_ms = TIMER_TMO_MS / 10;
    uint32_t duration;

    EXPECT_VAL(hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &start_cnt), sta_ok);
    EXPECT_VAL(hal_gpt_delay_ms(tmo_ms), sta_ok);
    EXPECT_VAL(hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &end_cnt), sta_ok);

    EXPECT_VAL(hal_gpt_get_duration_count(start_cnt, end_cnt, &duration), sta_ok);

    /* convert 32K counter to ms */
    time_ms = (duration * 1000) / 32768;

    if ((time_ms < tmo_ms - TOLERANCE_MS) || (time_ms > tmo_ms + TOLERANCE_MS))
        return CI_FAIL;

    return CI_PASS;
}

ci_status_t ci_gpt_delay_us_sample(void)
{
    uint32_t duration;
    uint32_t tmo_ms = TIMER_TMO_MS / 100, tmo_us;
    tmo_us = tmo_ms * 1000;

    EXPECT_VAL(hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &start_cnt), sta_ok);
    EXPECT_VAL(hal_gpt_delay_us(tmo_us), sta_ok);
    EXPECT_VAL(hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &end_cnt), sta_ok);

    EXPECT_VAL(hal_gpt_get_duration_count(start_cnt, end_cnt, &duration), sta_ok);
    if ((duration < tmo_us - TOLERANCE_US) || (duration > tmo_us + TOLERANCE_US)) {
        printf("%s test failed, duration = %luus\n", __func__, duration);
        return CI_FAIL;
    }

    return CI_PASS;
}

void gpt1_callback(void *user_data)
{
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &end_cnt);
    irq_flag = 1;
    hal_gpt_stop_timer(HAL_GPT_1);
    printf("%s exit\n", __func__);
}

ci_status_t ci_gpt_start_timer_ms_sample(void)
{
    hal_gpt_running_status_t running_sta;
    uint32_t duration;
    irq_flag = 0;

    EXPECT_VAL(hal_gpt_get_running_status(HAL_GPT_1, &running_sta), sta_ok);
    EXPECT_VAL(hal_gpt_init(HAL_GPT_1), sta_ok);

    printf("%s start, tmo = %dms, tol = %dms, mode = oneshot.\n",
           __func__, TIMER_TMO_MS, TOLERANCE_MS);

    EXPECT_VAL(hal_gpt_register_callback(HAL_GPT_1, gpt1_callback, NULL), sta_ok);
    EXPECT_VAL(hal_gpt_start_timer_ms(HAL_GPT_1, TIMER_TMO_MS, HAL_GPT_TIMER_TYPE_ONE_SHOT), sta_ok);
    EXPECT_VAL(hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &start_cnt), sta_ok);

    /* wait for timer irq triggered */
    while (!irq_flag)
        EXPECT_VAL(hal_gpt_delay_us(10), sta_ok);
    //printf("%s wait irq...\n", __func__);

    //EXPECT_VAL(hal_gpt_deinit(HAL_GPT_1), sta_ok);

    EXPECT_VAL(hal_gpt_get_duration_count(start_cnt, end_cnt, &duration), sta_ok);
    duration /= 1000;
    if ((duration < TIMER_TMO_MS - TOLERANCE_MS) || (duration > TIMER_TMO_MS + TOLERANCE_MS)) {
        printf("%s test failed, duration = %lums\n", __func__, duration);
        return CI_FAIL;
    }

    return CI_PASS;
}

#ifdef HAL_GPT_FEATURE_US_TIMER
void gpt2_callback(void *user_data)
{
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &end_cnt);
    irq_flag = 1;
    hal_gpt_stop_timer(HAL_GPT_2);
    printf("%s exit\n", __func__);
}

ci_status_t ci_gpt_start_timer_us_sample(void)
{
    hal_gpt_running_status_t running_sta;
    uint32_t duration;
    irq_flag = 0;

    EXPECT_VAL(hal_gpt_get_running_status(HAL_GPT_2, &running_sta), sta_ok);
    EXPECT_VAL(hal_gpt_init(HAL_GPT_2), sta_ok);

    printf("%s start, tmo = %dus, tol = %dus, mode = oneshot.\n",
           __func__, TIMER_TMO_MS * 1000, TOLERANCE_MS * 1000);

    EXPECT_VAL(hal_gpt_register_callback(HAL_GPT_2, gpt2_callback, NULL), sta_ok);
    EXPECT_VAL(
        hal_gpt_start_timer_us(HAL_GPT_2, TIMER_TMO_MS * 1000, HAL_GPT_TIMER_TYPE_ONE_SHOT),
        sta_ok
    );
    EXPECT_VAL(hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &start_cnt), sta_ok);

    /* wait for timer irq triggered */
    while (!irq_flag)
        EXPECT_VAL(hal_gpt_delay_us(10), sta_ok);
    //printf("%s wait irq...\n", __func__);
    //EXPECT_VAL(hal_gpt_deinit(HAL_GPT_2), sta_ok);

    EXPECT_VAL(hal_gpt_get_duration_count(start_cnt, end_cnt, &duration), sta_ok);
    duration /= 1000;
    if ((duration < TIMER_TMO_MS - TOLERANCE_MS) || (duration > TIMER_TMO_MS + TOLERANCE_MS)) {
        printf("%s test failed, duration = %lums\n", __func__, duration);
        return CI_FAIL;
    }

    return CI_PASS;
}
#endif /* #ifdef HAL_GPT_FEATURE_US_TIMER */

#ifdef HAL_GPT_SW_GPT_FEATURE
void sw_gpt_callback(void *user_data)
{
    uint32_t handle;

    handle = *(uint32_t *)user_data;

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &end_cnt);

    printf("sw_gpt callback come, handle = 0x%lX  \r\n", handle);

    irq_flag = 1;
    hal_gpt_sw_stop_timer_ms(handle);
    hal_gpt_sw_free_timer(handle);
}

ci_status_t ci_sw_gpt_start_timer_ms_sample(void)
{
    static uint32_t sw_gpt_handle;
    uint32_t duration, remain_time = 0;
    irq_flag = 0;

    EXPECT_VAL(hal_gpt_sw_get_timer(&sw_gpt_handle), sta_ok);
    printf("%s: SW_GPT get handle = 0x%lX\n", __func__, sw_gpt_handle);

    EXPECT_VAL(
        hal_gpt_sw_start_timer_ms(sw_gpt_handle, SW_TIMER_TMO_MS, sw_gpt_callback, &sw_gpt_handle),
        sta_ok
    );
    EXPECT_VAL(hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &start_cnt), sta_ok);

    /* test hal_gpt_sw_get_remaining_time_ms before timeout */
    EXPECT_VAL(hal_gpt_delay_ms(1000), sta_ok);
    EXPECT_VAL(
        hal_gpt_sw_get_remaining_time_ms(sw_gpt_handle, &remain_time),
        sta_ok
    );
    printf("%s remain_time = %lums\n", __func__, remain_time);

    if ((remain_time > (SW_TIMER_TMO_MS - 1000)) ||
        (remain_time < SW_TIMER_TMO_MS - 1000 - TOLERANCE_MS)) {
        printf("%s remain time failed.\n", __func__);
        return CI_FAIL;
    }

    /* wait for sw timer timeout */
    while (!irq_flag)
        EXPECT_VAL(hal_gpt_delay_us(10), sta_ok);

    EXPECT_VAL(hal_gpt_get_duration_count(start_cnt, end_cnt, &duration), sta_ok);
    duration /= 1000;
    if ((duration < SW_TIMER_TMO_MS - TOLERANCE_MS) || (duration > SW_TIMER_TMO_MS + TOLERANCE_MS)) {
        printf("%s test failed, duration = %lums\n", __func__, duration);
        return CI_FAIL;
    }

    return CI_PASS;
}
#endif /* #ifdef HAL_GPT_SW_GPT_FEATURE */

ci_status_t ci_gpt_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Sample Code: gpt mdelay test", ci_gpt_delay_ms_sample},
        {"Sample Code: gpt udelay test", ci_gpt_delay_us_sample},
        {"Sample Code: gpt start_timer_ms test", ci_gpt_start_timer_ms_sample},
#ifdef HAL_GPT_FEATURE_US_TIMER
        {"Sample Code: gpt start_timer_us test", ci_gpt_start_timer_us_sample},
#endif /* #ifdef HAL_GPT_FEATURE_US_TIMER */
#ifdef HAL_GPT_SW_GPT_FEATURE
        {"Sample Code: gpt sw start_timer test", ci_sw_gpt_start_timer_ms_sample},
#endif /* #ifdef HAL_GPT_SW_GPT_FEATURE */
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
