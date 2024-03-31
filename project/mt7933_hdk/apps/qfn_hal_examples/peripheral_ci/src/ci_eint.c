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
#include "FreeRTOS.h"
#include "task.h"
#include "ci.h"

static hal_gpio_pin_t _Tst_GPIO_PIN = HAL_GPIO_20;
static hal_gpio_pin_t _Trigger_GPIO_PIN = HAL_GPIO_19;
static hal_eint_number_t _Tst_EINT_num = HAL_EINT_NUMBER_12;
static const uint32_t debounce_time_ms = 64;
static uint32_t eint_debounce_start;
static uint32_t eint_debounce_end;
static uint32_t eint_int_count = 0;

void callback(void *data)
{
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &eint_debounce_end);

    hal_eint_set_debounce_count(_Tst_EINT_num, 0); //close debounce

    hal_gpio_set_output(_Trigger_GPIO_PIN, HAL_GPIO_DATA_HIGH); //clear source

    eint_int_count++;

    printf("eint %d be triggered %ld times \n", _Tst_EINT_num, eint_int_count);
}

ci_status_t ci_eint_test(hal_eint_config_t config)
{
    uint32_t eint_debounce_duration = 0;

    hal_eint_mask(_Tst_EINT_num);

    if (HAL_EINT_STATUS_OK != hal_eint_register_callback(_Tst_EINT_num, callback, NULL)) {
        printf("ut_hal_eint register callback error \r\n");
        return CI_FAIL;
    }

    if (HAL_EINT_STATUS_OK != hal_eint_init(_Tst_EINT_num, &config)) {
        printf("ut_hal_eint eint init error \r\n");
        return CI_FAIL;
    }

    hal_eint_set_trigger_mode(_Tst_EINT_num, config.trigger_mode);

    if (config.trigger_mode == HAL_EINT_LEVEL_LOW) {
        // Change debounce time at runtime
        if (HAL_EINT_STATUS_OK != hal_eint_set_debounce_time(_Tst_EINT_num, debounce_time_ms)) {
            printf("ut_hal_eint set debounce time error \r\n");
            return CI_FAIL;
        }
    }

    hal_eint_unmask(_Tst_EINT_num);

    eint_int_count = 0;
    hal_gpio_set_output(_Trigger_GPIO_PIN, HAL_GPIO_DATA_HIGH);
    hal_gpt_delay_ms(1);

    if (config.trigger_mode == HAL_EINT_LEVEL_LOW) {
        eint_debounce_start = 0;
        eint_debounce_end = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &eint_debounce_start);
    }

    hal_gpio_set_output(_Trigger_GPIO_PIN, HAL_GPIO_DATA_LOW);
    hal_gpt_delay_ms(1000);
    if (!eint_int_count) {
        printf("eint num %d can not be trigger\n", _Tst_EINT_num);
        return CI_FAIL;
    }

    if (config.trigger_mode == HAL_EINT_LEVEL_LOW) {
        hal_gpt_get_duration_count(eint_debounce_start, eint_debounce_end, &eint_debounce_duration);
        eint_debounce_duration /= 1000;
        printf("eint_debounce_duration:%ldms, debounce_time:%ldms\n",
               eint_debounce_duration, debounce_time_ms);
    }

    printf("software_trigger start\n\n");
    eint_int_count = 0;

    hal_eint_clear_software_trigger(_Tst_EINT_num);

    hal_gpt_delay_ms(1000);
    hal_eint_set_software_trigger(_Tst_EINT_num);

    if (!eint_int_count) {
        printf("software_trigger eint num %d can not be trigger\n", _Tst_EINT_num);
        return CI_FAIL;
    }

    hal_eint_deinit(_Tst_EINT_num);

    return CI_PASS;
}

ci_status_t ci_eint_level_low_mode_sample(void)
{
    hal_eint_config_t eint_config = {0};

    printf("LEVEL_LOW mode test\n");
    eint_config.trigger_mode = HAL_EINT_LEVEL_LOW;
    EXPECT_VAL(ci_eint_test(eint_config), CI_PASS);

    return CI_PASS;
}

ci_status_t ci_eint_edge_falling_mode_sample(void)
{
    hal_eint_config_t eint_config = {0};

    printf("EDGE_FALLING mode test\n");
    eint_config.trigger_mode = HAL_EINT_EDGE_FALLING;
    EXPECT_VAL(ci_eint_test(eint_config), CI_PASS);

    return CI_PASS;
}

ci_status_t ci_eint_init_sample(void)
{
    hal_pinmux_status_t pinmux_status;

    // Init test GPIO
    hal_gpio_init(_Tst_GPIO_PIN);
    pinmux_status = hal_pinmux_set_function(_Tst_GPIO_PIN, 7);
    if (HAL_PINMUX_STATUS_OK != pinmux_status) {
        printf("ut_hal_eint set pinmux function error, pin = %d \r\n", _Tst_GPIO_PIN);
        return CI_FAIL;
    }
    hal_gpio_pull_up(_Tst_GPIO_PIN);

    // Init trigger gpio
    hal_gpio_init(_Trigger_GPIO_PIN);
    hal_pinmux_set_function(_Trigger_GPIO_PIN, 0);
    hal_gpio_set_direction(_Trigger_GPIO_PIN, HAL_GPIO_DIRECTION_OUTPUT);

    return CI_PASS;
}

ci_status_t ci_eint_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Sample Code: eint test init", ci_eint_init_sample},
        {"Sample Code: eint edge falling mode test sample", ci_eint_edge_falling_mode_sample},
        {"Sample Code: einit level low mode test sample", ci_eint_level_low_mode_sample},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}

