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
#if defined(UT_HAL_ENABLE) && defined(UT_HAL_PWM_MODULE_ENABLE)
#include "hal_pwm.h"
#include <hal_gpio.h>
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#define GPIO_NUM_INDEX 29
/*
 *define for test suspend resume feartue, should keep pwm wavefor always on, no need
 * call pwm_stop & pwm_deinit.
 */
#define suspend_resume
uint8_t pwm_deinit(void)
{
    uint8_t start_channel_num = 0;
    uint8_t end_channel_num = 0;
    uint8_t channel_num = 0;
    int ret = 0;

    start_channel_num = HAL_PWM_0;
    end_channel_num = HAL_PWM_11;

    for (channel_num = start_channel_num; channel_num <= end_channel_num;
         channel_num++) {
#ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK
        ret = hal_pwm_deinit();
#else /* #ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK */
        ret = hal_pwm_deinit(channel_num);
#endif /* #ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK */
        if (ret)
            printf("channel_num%d deinit fail\n", channel_num);
        else
            printf("channel_num%d deinit success\n", channel_num);
    }

    return ret;
}

uint8_t pwm_set_frequency(uint8_t channel_num, uint32_t frequency, uint32_t *total_count)
{
    int ret = 0;


    ret = hal_pwm_set_frequency(channel_num,
                                frequency,
                                total_count
                               );
    if (ret)
        printf("set_frequency fail ret:%d\n", ret);
    else
        printf("set_frequency success\n");

    return ret;
}

uint8_t pwm_set_duty_cycle(uint8_t channel_num, uint32_t duty_cycle)
{
    int ret = 0;

    ret = hal_pwm_set_duty_cycle(channel_num,
                                 duty_cycle
                                );
    if (ret)
        printf("set duty_cycle fail\n");
    else
        printf("set duty_cycle success\n");

    return ret;
}

uint8_t pwm_get_frequency(uint8_t channel_num, uint32_t *frequency)
{
    int ret = 0;

    ret = hal_pwm_get_frequency(channel_num,
                                frequency
                               );
    if (ret)
        printf("get_frequency fail\n");
    else
        printf("get_frequency success\n");

    return ret;
}

uint8_t pwm_get_duty_cycle(uint8_t channel_num, uint32_t *duty_cycle)
{
    int ret = 0;

    ret = hal_pwm_get_duty_cycle(channel_num,
                                 duty_cycle
                                );
    if (ret)
        printf("get_duty_cycle fail\n");
    else
        printf("get_duty_cycle success\n");

    return ret;
}

uint8_t pwm_get_running_status(uint8_t channel_num, hal_pwm_running_status_t *running_status)
{
    int ret = 0;

    ret = hal_pwm_get_running_status(channel_num,
                                     running_status
                                    );
    if (ret)
        printf("get_running_status fail\n");
    else
        printf("get_running_status success, running_status:%d\n", *running_status);

    return ret;
}
uint8_t pwm_set_advanced_config(uint8_t channel_num, uint32_t advanced_config)
{
    int ret = 0;

#ifdef  HAL_PWM_FEATURE_ADVANCED_CONFIG
    ret = hal_pwm_set_advanced_config(channel_num,
                                      (hal_pwm_advanced_config_t)advanced_config
                                     );
#endif /* #ifdef  HAL_PWM_FEATURE_ADVANCED_CONFIG */
    if (ret)
        printf("set advanced_config fail\n");
    else
        printf("set advanced_config success\n");

    return ret;
}

uint8_t pwm_start(uint8_t channel_num)
{
    int ret = 0;

    ret = hal_pwm_start(channel_num);
    if (ret)
        printf("pwm%d fail\n", channel_num);
    else
        printf("pwm%d start success\n", channel_num);

    return ret;
}

uint8_t pwm_stop(uint8_t channel_num)
{
    int ret = 0;

    ret = hal_pwm_stop(channel_num);
    if (ret)
        printf("pwm%d fail\n", channel_num);
    else
        printf("pwm%d stop success\n", channel_num);

    return ret;
}

ut_status_t ut_hal_pwm(void)
{
    uint32_t frequency;
    uint32_t duty_cycle;
    uint32_t current_frequency;
    uint32_t current_duty_cycle;
    uint32_t total_count;
    uint8_t start_channel_num = 0;
    uint8_t end_channel_num = 0;
    uint8_t channel_num = 0;
    hal_pwm_source_clock_t source_clock;
    hal_pwm_running_status_t running_status = HAL_PWM_IDLE;
    int ret = 0;

    start_channel_num = HAL_PWM_0;
    end_channel_num = HAL_PWM_7;
    source_clock =  HAL_PWM_CLOCK_26MHZ;
    frequency = 20000;
    duty_cycle = 0xFF;

#ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK
    ret = hal_pwm_init((hal_pwm_source_clock_t)source_clock);
    printf("PWM init success, start_channel_num:%d, end_channel_num:%d\n",
           start_channel_num, end_channel_num);
#endif /* #ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK */
    for (channel_num = start_channel_num; channel_num <= end_channel_num;
         channel_num++) {

        hal_gpio_init(HAL_GPIO_29 + channel_num);
        hal_pinmux_set_function(HAL_GPIO_29 + channel_num, 3);

#ifndef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK
        ret = hal_pwm_init(channel_num, (hal_pwm_source_clock_t)source_clock);
        printf("ut_hal_pwm stg2!\n");
#endif /* #ifndef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK */

        if (ret) {
            printf("channel_num%d init pwm fail\n",
                   channel_num);
            return UT_STATUS_ERROR;
        } else
            printf("channel_num%d init pwm success\n",
                   channel_num);
        ret = pwm_set_frequency(channel_num, frequency, &total_count);
        if (ret)
            return UT_STATUS_ERROR;

        ret = pwm_set_duty_cycle(channel_num, duty_cycle);
        if (ret)
            return UT_STATUS_ERROR;

        ret = pwm_start(channel_num);
        if (ret)
            return UT_STATUS_ERROR;

        ret = pwm_get_frequency(channel_num, &current_frequency);
        if (ret)
            return UT_STATUS_ERROR;

        ret = pwm_get_duty_cycle(channel_num, &current_duty_cycle);
        if (ret)
            return UT_STATUS_ERROR;

        printf("channel_num%d current_frequency:%d, current_duty_cycle:%d!\n",
               channel_num, (int)current_frequency, (int)current_duty_cycle);

        ret = pwm_get_running_status(channel_num, &running_status);
        if (ret)
            return UT_STATUS_ERROR;
        printf("channel_num%d running_status:%d !\n", channel_num, (int)running_status);

    }
#ifdef suspend_resume
    return UT_STATUS_OK;
#endif /* #ifdef suspend_resume */

    for (channel_num = start_channel_num; channel_num <= end_channel_num;
         channel_num++) {
        ret = pwm_stop(channel_num);
        if (ret)
            return UT_STATUS_ERROR;
    }

#ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK
    ret = hal_pwm_deinit();
    if (ret) {
        printf("PWM deinit fail\n");
        return UT_STATUS_ERROR;
    } else
        printf("PWM deinit success\n");
#else /* #ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK */
    for (channel_num = start_channel_num; channel_num <= end_channel_num;
         channel_num++) {
        ret = hal_pwm_deinit(channel_num);
        if (ret) {
            printf("channel_num%d deinit fail\n", channel_num);
            return UT_STATUS_ERROR;
        } else
            printf("channel_num%d deinit success\n", channel_num);
    }
#endif /* #ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK */
    return UT_STATUS_OK;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined(UT_HAL_PWM_MODULE_ENABLE) */

