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
#include "ci_cli.h"
#include "hal.h"
#include "ci.h"
#include "hal_pwm.h"

#define FREQ_SET_A  1000
/*The range from 0 to 1000, 750 means duty 75%*/
#define DUTY_SET_A  750

#define PWM_GPIO_CH_9       38


ci_status_t ci_pwm_test_sample(void)
{
    int i = 0;

    uint32_t pwm_freq_cnt_1 = 0;
    uint32_t frequency = 0;
    uint32_t duty_cycle = 0;
    hal_pwm_running_status_t running_status = HAL_PWM_IDLE;

    EXPECT_VAL(hal_gpio_init(PWM_GPIO_CH_9), HAL_ADC_STATUS_OK);
    EXPECT_VAL(hal_pinmux_set_function(PWM_GPIO_CH_9, MT7933_PIN_38_FUNC_PWM_9), HAL_PINMUX_STATUS_OK);
    EXPECT_VAL(hal_pwm_init(HAL_PWM_9, HAL_PWM_CLOCK_26MHZ), HAL_PWM_STATUS_OK);

    while (i < 2) {
        printf("[ADC_PWM] case 1 running\n");
        EXPECT_VAL(hal_pwm_set_frequency(HAL_PWM_9, FREQ_SET_A, &pwm_freq_cnt_1), HAL_PWM_STATUS_OK);
        EXPECT_VAL(hal_pwm_set_duty_cycle(HAL_PWM_9, DUTY_SET_A), HAL_PWM_STATUS_OK);
        EXPECT_VAL(hal_pwm_start(HAL_PWM_9), HAL_PWM_STATUS_OK);

        vTaskDelay(3000 / portTICK_PERIOD_MS);

        EXPECT_VAL(hal_pwm_get_frequency(HAL_PWM_9, &frequency), HAL_PWM_STATUS_OK);
        printf("[PWM] get frequency : %ld\n", frequency);
        EXPECT_VAL(hal_pwm_get_duty_cycle(HAL_PWM_9, &duty_cycle), HAL_PWM_STATUS_OK);
        printf("[PWM] get duty_cycle : %ld\n", duty_cycle);

        EXPECT_VAL(hal_pwm_get_running_status(HAL_PWM_9, &running_status), HAL_PWM_STATUS_OK);
        printf("[PWM] running_status : %d\n", running_status);

        EXPECT_VAL(hal_pwm_stop(HAL_PWM_9), HAL_PWM_STATUS_OK);

        EXPECT_VAL(hal_pwm_get_running_status(HAL_PWM_9, &running_status), HAL_PWM_STATUS_OK);
        printf("[PWM] running_status : %d\n", running_status);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        i++;
    }

    EXPECT_VAL(hal_pwm_deinit(HAL_PWM_9), HAL_PWM_STATUS_OK);
    return CI_PASS;
}


ci_status_t ci_pwm_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Sample Code: pwm test", ci_pwm_test_sample},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
