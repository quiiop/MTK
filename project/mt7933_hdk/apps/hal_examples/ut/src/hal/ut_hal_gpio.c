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

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_GPIO_MODULE_ENABLE)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
ut_status_t ut_hal_gpio(void)
{
    hal_gpio_direction_t dir;
    hal_gpio_data_t data;
    hal_gpio_driving_current_t drv;

    printf("ut_hal_gpio beign.\r\n");
    hal_pinmux_set_function(HAL_GPIO_43, 0);
    printf("gpio pinmux set pass.\r\n");
    hal_gpio_set_direction(HAL_GPIO_43, HAL_GPIO_DIRECTION_OUTPUT);
    hal_gpio_get_direction(HAL_GPIO_43, &dir);
    if (dir != HAL_GPIO_DIRECTION_OUTPUT)
        printf("gpio(%d) set dir1(%d) fail.\r\n", HAL_GPIO_43, dir);

    printf("gpio dir output set pass.\r\n");
    hal_gpio_set_output(HAL_GPIO_43, HAL_GPIO_DATA_HIGH);
    hal_gpio_get_output(HAL_GPIO_43, &data);
    if (data != HAL_GPIO_DATA_HIGH)
        printf("gpio(%d) get output data(%d) fail.\r\n", HAL_GPIO_43, data);

    printf("gpio output high set pass.\r\n");
    hal_gpio_get_input(HAL_GPIO_43, &data);
    if (data != HAL_GPIO_DATA_HIGH)
        printf("gpio(%d) get input data1(%d) fail.\n", HAL_GPIO_43, data);

    printf("gpio set output high pass.\r\n");
    hal_gpio_set_direction(HAL_GPIO_43, HAL_GPIO_DIRECTION_INPUT);
    hal_gpio_get_direction(HAL_GPIO_43, &dir);
    if (dir != HAL_GPIO_DIRECTION_INPUT)
        printf("gpio(%d) set dir2(%d) fail.\n", HAL_GPIO_43, dir);

    printf("gpio dir input set pass.\r\n");
    hal_gpio_pull_up(HAL_GPIO_43);
    hal_gpio_get_input(HAL_GPIO_43, &data);
    if (data != HAL_GPIO_DATA_HIGH)
        printf("gpio(%d) get input data2(%d) fail.\r\n", HAL_GPIO_43, data);

    printf("gpio set input pull up pass.\r\n");
    hal_gpio_pull_down(HAL_GPIO_43);
    hal_gpio_get_input(HAL_GPIO_43, &data);

    printf("gpio set input pull down pass.\r\n");
    hal_gpio_disable_pull(HAL_GPIO_43);
    hal_gpio_set_pupd_register(HAL_GPIO_43, 1, 1, 1);
    hal_gpio_set_high_impedance(HAL_GPIO_44);
    hal_gpio_clear_high_impedance(HAL_GPIO_44);

    hal_gpio_set_driving_current(HAL_GPIO_43, HAL_GPIO_DRIVING_CURRENT_6MA);
    hal_gpio_get_driving_current(HAL_GPIO_43, &drv);
    if (drv != HAL_GPIO_DRIVING_CURRENT_6MA)
        printf("gpio(%d) get drv(0x%x) fail.\r\n", HAL_GPIO_43, drv);

    printf("ut_hal_gpio end.\n");
    return UT_STATUS_OK;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_GPIO_MODULE_ENABLE) */
