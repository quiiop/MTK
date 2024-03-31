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

#include "filogic_led_cli.h"


typedef struct led_cmd {
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
static bool                 spi_master_0_initialized = false;

void _spi_cmd_send(led_cmd_t *_cmd)
{
    assert(HAL_SPI_MASTER_STATUS_OK == hal_spi_master_send_polling(
               HAL_SPI_MASTER_0, (uint8_t *)_cmd, sizeof(*_cmd)));
#if 0
    int i;
    for (i = 0; i < sizeof(*_cmd); i++)
        printf("%02x ", ((const char *)_cmd)[i] & 0xFF);
    printf("\n");
#endif /* #if 0 */
}


static void spi_init(void)
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



static void cmd_init(led_cmd_t *cmd)
{
    cmd->start       = 0x00000000;
    cmd->led1_global = 0xE0;
    cmd->led1_b      = 0x00;
    cmd->led1_g      = 0x00;
    cmd->led1_r      = 0x00;
    cmd->led2_global = 0xE0;
    cmd->led2_b      = 0x00;
    cmd->led2_g      = 0x00;
    cmd->led2_r      = 0x00;
    cmd->end         = 0xFFFFFFFF;
}

static unsigned char led_init(uint8_t len, char *param[])
{
    spi_init();
    cmd_init(&led_cmd);
    _spi_cmd_send(&led_cmd);
    return 0;
}

static unsigned char led_deinit(uint8_t len, char *param[])
{
    if ( spi_master_0_initialized )
        {
        cmd_init(&led_cmd);
        _spi_cmd_send(&led_cmd);
        assert(HAL_SPI_MASTER_STATUS_OK == hal_spi_master_deinit(HAL_SPI_MASTER_0));
        }
    spi_master_0_initialized = false;
    return 0;
}

static unsigned char led_set(uint8_t len, char *param[])
{
    uint8_t led_sn, dim, blue, green, red;

    led_sn = atoi(param[0]);
    red = atoi(param[1]);
    green = atoi(param[2]);
    blue = atoi(param[3]);
    dim = atoi(param[4]);

    if ( led_sn != FILOGIC_LED_1 && led_sn != FILOGIC_LED_2 )
        {
        led_print("%s: led sn incorrect\n", __func__);
        return -1;
        }
    //if ( red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255 || dim < 0 || dim > 31 )
    //    {
    //    led_print("%s: led param incorrect\n", __func__);
    //    return -1;
    //    }

    switch(led_sn)
        {
        case FILOGIC_LED_1:
            led_cmd.start       = 0x00000000;
            led_cmd.led1_global = 0xE0 + dim;
            led_cmd.led1_b      = blue;
            led_cmd.led1_g      = green;
            led_cmd.led1_r      = red;
            led_cmd.end         = 0xFFFFFFFF;
            break;
        case FILOGIC_LED_2:
            led_cmd.start       = 0x00000000;
            led_cmd.led2_global = 0xE0 + dim;
            led_cmd.led2_b      = blue;
            led_cmd.led2_g      = green;
            led_cmd.led2_r      = red;
            led_cmd.end         = 0xFFFFFFFF;
            break;
        default:
            break;
        }
    return 0;
}

static unsigned char led_get(uint8_t len, char *param[])
{
    led_print("led1 : \n");
    led_print("  dim   : %d\n", led_cmd.led1_global - 0xE0);
    led_print("  red   : %d\n", led_cmd.led1_r);
    led_print("  green : %d\n", led_cmd.led1_g);
    led_print("  blue  : %d\n", led_cmd.led1_b);
    led_print("led2 : \n");
    led_print("  dim   : %d\n", led_cmd.led2_global - 0xE0);
    led_print("  red   : %d\n", led_cmd.led2_r);
    led_print("  green : %d\n", led_cmd.led2_g);
    led_print("  blue  : %d\n", led_cmd.led2_b);
    return 0;
}

static unsigned char led_on(uint8_t len, char *param[])
{
    if ( spi_master_0_initialized )
        _spi_cmd_send(&led_cmd);
    else
        {
        led_print("%s: led spi not ready\n", __func__);
        return -1;
        }
    return 0;
}

static unsigned char led_off(uint8_t len, char *param[])
{
    if ( spi_master_0_initialized )
        {
        cmd_init(&led_cmd);
        _spi_cmd_send(&led_cmd);
        }
    else
        {
        led_print("%s: led spi not ready\n", __func__);
        return -1;
        }
    return 0;
}

cmd_t led_cli_cmds[] = {
    {"init", "init led spi", led_init, NULL },
    {"deinit", "deinit led spi", led_deinit, NULL },
    {"set", "set LEDx R G B DIM", led_set, NULL },
    {"get", "get led info", led_get, NULL },
    {"on", "led on", led_on, NULL },
    {"off", "led off", led_off, NULL },
    { NULL, NULL, NULL, NULL }
};
