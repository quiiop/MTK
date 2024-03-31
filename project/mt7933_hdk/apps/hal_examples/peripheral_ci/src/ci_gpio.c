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

ci_status_t ci_gpio_sample(void)
{
    hal_gpio_pin_t gpio_pin = HAL_GPIO_36;
    hal_gpio_direction_t dir;
    hal_gpio_data_t data;
    hal_gpio_driving_current_t drv;

    EXPECT_VAL(hal_gpio_init(gpio_pin), HAL_GPIO_STATUS_OK);

    EXPECT_VAL(hal_pinmux_set_function(gpio_pin, 0), HAL_GPIO_STATUS_OK);

    EXPECT_VAL(hal_gpio_set_direction(gpio_pin, HAL_GPIO_DIRECTION_OUTPUT), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_get_direction(gpio_pin, &dir), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(dir, HAL_GPIO_DIRECTION_OUTPUT);

    EXPECT_VAL(hal_gpio_set_output(gpio_pin, HAL_GPIO_DATA_HIGH), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_get_output(gpio_pin, &data), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(data, HAL_GPIO_DATA_HIGH);

    EXPECT_VAL(hal_gpio_get_input(gpio_pin, &data), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(data, HAL_GPIO_DATA_HIGH);

    EXPECT_VAL(hal_gpio_set_output(gpio_pin, HAL_GPIO_DATA_LOW), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_get_output(gpio_pin, &data), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(data, HAL_GPIO_DATA_LOW);

    EXPECT_VAL(hal_gpio_get_input(gpio_pin, &data), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(data, HAL_GPIO_DATA_LOW);

    EXPECT_VAL(hal_gpio_toggle_pin(gpio_pin), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_get_output(gpio_pin, &data), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(data, HAL_GPIO_DATA_HIGH);

    EXPECT_VAL(hal_gpio_get_input(gpio_pin, &data), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(data, HAL_GPIO_DATA_HIGH);

    EXPECT_VAL(hal_gpio_set_direction(gpio_pin, HAL_GPIO_DIRECTION_INPUT), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_get_direction(gpio_pin, &dir), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(dir, HAL_GPIO_DIRECTION_INPUT);

    EXPECT_VAL(hal_gpio_pull_up(gpio_pin), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_get_input(gpio_pin, &data), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(data, HAL_GPIO_DATA_HIGH);

    EXPECT_VAL(hal_gpio_pull_down(gpio_pin), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_get_input(gpio_pin, &data), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(data, HAL_GPIO_DATA_LOW);

    EXPECT_VAL(hal_gpio_disable_pull(gpio_pin), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_set_pupd_register(gpio_pin, 1, 1, 1), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_set_high_impedance(gpio_pin), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_clear_high_impedance(gpio_pin), HAL_GPIO_STATUS_OK);

    EXPECT_VAL(hal_gpio_set_driving_current(gpio_pin, HAL_GPIO_DRIVING_CURRENT_6MA), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(hal_gpio_get_driving_current(gpio_pin, &drv), HAL_GPIO_STATUS_OK);
    EXPECT_VAL(drv, HAL_GPIO_DRIVING_CURRENT_6MA);

    EXPECT_VAL(hal_gpio_deinit(gpio_pin), HAL_GPIO_STATUS_OK);

    return CI_PASS;
}


ci_status_t ci_gpio_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Sample Code: GPIO sample code", ci_gpio_sample},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));

}
