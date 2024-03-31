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

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_EINT_MODULE_ENABLE)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static hal_gpio_pin_t _Tst_GPIO_PIN = HAL_GPIO_8;
static hal_gpio_pin_t _Trigger_GPIO_PIN = HAL_GPIO_6;
static hal_eint_number_t _Tst_EINT_num = HAL_EINT_NUMBER_0;
static const uint32_t debounce_time_ms = 64;
static uint32_t u32_trigger_time;
static uint32_t u32_eint_time;
static unsigned int eint_number = 0;

typedef enum {
    SLEEP_MODE = 0,
    NORMAL_MODE,
} EINT_TEST_MODE_T;

extern void vTaskDelay(const uint16_t);

static void wakeup_gpt_callback(void *user_data)
{
    hal_gpt_stop_timer(HAL_GPT_1);
}

void callback(hal_nvic_irq_t irq)
{
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &u32_eint_time);

    hal_eint_set_debounce_count(_Tst_EINT_num, 0); //close debounce

    hal_gpio_set_output(_Trigger_GPIO_PIN, HAL_GPIO_DATA_HIGH); //clear source

    hal_eint_mask(_Tst_EINT_num);

    eint_number++;

    hal_eint_unmask(_Tst_EINT_num);

    printf("eint %d be triggered %d times \n", _Tst_EINT_num, eint_number);
}

ut_status_t ut_hal_eint_mode_test(EINT_TEST_MODE_T mode,
                                  hal_eint_config_t config)
{
    hal_pinmux_status_t pinmux_status;
    uint32_t duration_count;
    uint32_t ret;

    if (mode == SLEEP_MODE) {
        hal_gpt_init(HAL_GPT_1);
        hal_gpt_register_callback(HAL_GPT_1, wakeup_gpt_callback, NULL);
        hal_gpt_stop_timer(HAL_GPT_1);
    }

    // Init test GPIO
    hal_gpio_init(_Tst_GPIO_PIN);
    pinmux_status = hal_pinmux_set_function(_Tst_GPIO_PIN, MT7933_PIN_8_FUNC_CM33_GPIO_EINT0);
    if (HAL_PINMUX_STATUS_OK != pinmux_status) {
        printf("ut_hal_eint set pinmux function error, pin = %d \r\n", _Tst_GPIO_PIN);
        return UT_STATUS_ERROR;
    }
    hal_gpio_pull_up(_Tst_GPIO_PIN);

    // Init trigger source pin
    hal_gpio_init(_Trigger_GPIO_PIN);
    hal_pinmux_set_function(_Trigger_GPIO_PIN, MT7933_PIN_6_FUNC_GPIO6);
    hal_gpio_set_direction(_Trigger_GPIO_PIN, HAL_GPIO_DIRECTION_OUTPUT);

    hal_eint_mask(_Tst_EINT_num);
    if (HAL_EINT_STATUS_OK != hal_eint_init(_Tst_EINT_num, &config)) {
        printf("ut_hal_eint eint init error \r\n");
        return UT_STATUS_ERROR;
    }
    if (HAL_EINT_STATUS_OK != hal_eint_register_callback(_Tst_EINT_num, callback, NULL)) {
        printf("ut_hal_eint register callback error \r\n");
        return UT_STATUS_ERROR;
    }
    if (config.trigger_mode == HAL_EINT_LEVEL_LOW) {
        // Change debounce time at runtime
        if (HAL_EINT_STATUS_OK != hal_eint_set_debounce_time(_Tst_EINT_num, debounce_time_ms)) {
            printf("ut_hal_eint set debounce time error \r\n");
            return UT_STATUS_ERROR;
        }
    }
    hal_eint_unmask(_Tst_EINT_num);

    if (mode == SLEEP_MODE) {
        hal_gpt_start_timer_ms(HAL_GPT_1, 5000, HAL_GPT_TIMER_TYPE_ONE_SHOT);
        vTaskDelay(50);
        ret = hal_sleep_manager_enter_sleep_mode(HAL_SLEEP_MODE_SLEEP);
        if (ret != 11) {
            printf("can not enter sleep\n");
            return UT_STATUS_ERROR;
        }
    }

    eint_number = 0;
    hal_gpio_set_output(_Trigger_GPIO_PIN, HAL_GPIO_DATA_HIGH);
    hal_gpt_delay_ms(1);

    if (config.trigger_mode == HAL_EINT_LEVEL_LOW) {
        u32_eint_time = 0;
        u32_trigger_time = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &u32_trigger_time);
    }

    hal_gpio_set_output(_Trigger_GPIO_PIN, HAL_GPIO_DATA_LOW);
    hal_gpt_delay_ms(1000);
    if (!eint_number) {
        printf("eint num %d can not be trigger\n", _Tst_EINT_num);
        return UT_STATUS_ERROR;
    }

    if (config.trigger_mode == HAL_EINT_LEVEL_LOW) {
        hal_gpt_get_duration_count(u32_trigger_time, u32_eint_time, &duration_count);
        duration_count /= 1000;
        printf("duration_count:%ldms, below debounce_time:%ldms\n",
               duration_count, debounce_time_ms);
    }

    if (mode == SLEEP_MODE)
        hal_gpt_deinit(HAL_GPT_1);

    hal_eint_deinit(_Tst_EINT_num);

    return UT_STATUS_OK;
}

ut_status_t ut_hal_eint(void)
{
    uint32_t ret = UT_STATUS_OK;

    hal_eint_config_t eint_config = {0};

    printf("ut_hal_eint enter\r\n");

    printf("\r\n");
    printf("ut_hal_eint HAL_EINT_EDGE_FALLING test\r\n");
    eint_config.trigger_mode = HAL_EINT_EDGE_FALLING;
    printf("NORMAL MODE test\n");
    ret = ut_hal_eint_mode_test(NORMAL_MODE, eint_config);
    printf("SLEEP MODE test\n");
    ret = ut_hal_eint_mode_test(SLEEP_MODE, eint_config);

    printf("\r\n");
    printf("ut_hal_eint HAL_EINT_LEVEL_LOW test\r\n");
    eint_config.trigger_mode = HAL_EINT_LEVEL_LOW;
    printf("NORMAL MODE test\n");
    ret = ut_hal_eint_mode_test(NORMAL_MODE, eint_config);
    printf("SLEEP MODE test\n");
    ret = ut_hal_eint_mode_test(SLEEP_MODE, eint_config);

    if (ret == UT_STATUS_ERROR)
        return UT_STATUS_ERROR;

    return UT_STATUS_OK;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_EINT_MODULE_ENABLE) */
