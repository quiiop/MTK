/* Copyright Statement:
 *
 * (C) 2022-2022  MediaTek Inc. All rights reserved.
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


#include <hal.h>


#include "filogic_led.h"

#define LIGHT_LEVEL_MIN     (0)
#define LIGHT_LEVEL_MAX     (31)

typedef struct led_cmd
{
    uint32_t    start;
    uint8_t     led1_global;
    uint8_t     led1_b;
    uint8_t     led1_g;
    uint8_t     led1_r;
    uint8_t     led2_global;
    uint8_t     led2_b;
    uint8_t     led2_g;
    uint8_t     led2_r;
    uint32_t    end;
} led_cmd_t;


static led_cmd_t            led_cmd;
static filogic_led_color_t  led1_color = {
        .r = 0,
        .b = 0,
        .g = LIGHT_LEVEL_MAX // default green
    };
static filogic_led_color_t  led2_color = {
        .r = 0,
        .b = 0,
        .g = LIGHT_LEVEL_MAX // default green
    };
static uint8_t              led1_dim = LIGHT_LEVEL_MAX;
static uint8_t              led2_dim = LIGHT_LEVEL_MAX;
static bool                 spi_master_0_initialized;


void _cmd_send(led_cmd_t *_cmd)
{
    assert(HAL_SPI_MASTER_STATUS_OK == hal_spi_master_send_polling(
           HAL_SPI_MASTER_0, (uint8_t *)_cmd, sizeof(*_cmd)));
#if 0
    int i;
    for (i = 0; i < sizeof(*_cmd); i++)
        printf("%02x ", ((const char *)_cmd)[i] & 0xFF);
    printf("\n");
#endif
}


static void _filogic_spi_init(void)
{
    if (spi_master_0_initialized)
        return;

    assert(HAL_PINMUX_STATUS_OK == hal_pinmux_set_function(HAL_GPIO_6, 3));
    assert(HAL_PINMUX_STATUS_OK == hal_pinmux_set_function(HAL_GPIO_9, 3));

    hal_spi_master_config_t spi_config;
    spi_config.bit_order        = HAL_SPI_MASTER_MSB_FIRST;
    spi_config.clock_frequency  = 1000000;
    spi_config.phase            = HAL_SPI_MASTER_CLOCK_PHASE0;
    spi_config.polarity         = HAL_SPI_MASTER_CLOCK_POLARITY0;

    assert(HAL_SPI_MASTER_STATUS_OK == hal_spi_master_init(HAL_SPI_MASTER_0, &spi_config));

    spi_master_0_initialized = true;
}


void filogic_led_status_dim(uint8_t dim)
{
    led1_dim = dim;

    filogic_led_status_toggle(true);
}


void filogic_led_light_dim(uint8_t dim)
{
    led2_dim = dim;

    filogic_led_light_toggle(true);
}


void filogic_led_status_toggle(bool on_off)
{
    led_cmd.led1_r = led1_color.r;
    led_cmd.led1_g = led1_color.g;
    led_cmd.led1_b = led1_color.b;

    if(on_off && led1_dim == 0) led1_dim = 1;

    led_cmd.led1_global = 0xE0 | (on_off?led1_dim:0);

    _cmd_send(&led_cmd);
}


uint8_t filogic_led_light_get_cur_dim_level(void)
{
    return led2_dim;
}


uint8_t filogic_led_get_max_dim_level(void)
{
    return LIGHT_LEVEL_MAX;
}


uint8_t filogic_led_get_min_dim_level(void)
{
    return LIGHT_LEVEL_MIN;
}


uint8_t filogic_led_status_get_cur_dim_level(void)
{
    return led1_dim;
}


void filogic_led_light_toggle(bool on_off)
{
    led_cmd.led2_r = led2_color.r;
    led_cmd.led2_g = led2_color.g;
    led_cmd.led2_b = led2_color.b;

    if(on_off && led2_dim == 0) led2_dim = 1;

    led_cmd.led2_global = 0xE0 | (on_off?led2_dim:0);

    _cmd_send(&led_cmd);
}


void filogic_led_status_color(filogic_led_color_t color)
{
    led1_color = color;

    filogic_led_status_toggle(true);
}


void filogic_led_light_color(filogic_led_color_t color)
{
    led2_color = color;

    filogic_led_light_toggle(true);
}


static void _filogic_cmd_init(led_cmd_t *cmd)
{
    cmd->start       = 0x00000000;
    cmd->led1_global = 0xE3;
    cmd->led1_b      = 0x00;
    cmd->led1_g      = 0x00;
    cmd->led1_r      = 0x00;
    cmd->led2_global = 0xE3;
    cmd->led2_b      = 0x00;
    cmd->led2_g      = 0x00;
    cmd->led2_r      = 0x00;
    cmd->end         = 0xFFFFFFFF;

    _cmd_send(&led_cmd);
}


void filogic_led_init(void)
{
    _filogic_spi_init();

    _filogic_cmd_init(&led_cmd);

    _cmd_send(&led_cmd);
}
