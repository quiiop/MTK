/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
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

#if defined(MTK_MINICLI_ENABLE)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cli.h"

#include "hal_gpio.h"
#include "hal_gpio_internal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "task_def.h"

#include "mt84g_uart_cli.h"
#include "mt84g_hid_cli.h"

#define TRIGGER_PIN HAL_GPIO_38

#define gpio_trigger_TASK_NAME        "gpio_trigger_task"
#define gpio_trigger_TASK_STACKSIZE   (512)
#define gpio_trigger_TASK_PRIO        TASK_PRIORITY_NORMAL
/****************************************************************************
 *
 * Static variables.
 *
 ****************************************************************************/
static bool pin_status = false;
static xTaskHandle gpio_trigger_task_hdl = NULL;
static bool hardware_trigger_status = false;
/****************************************************************************
 *
 * Private variables.
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Local functions.
 *
 ****************************************************************************/
uint8_t set_trigger_pin(bool trigger)
{
    if (pin_status)
        {
        if (trigger)
            {
            if (hal_gpio_set_output(TRIGGER_PIN, HAL_GPIO_DATA_LOW) != HAL_GPIO_STATUS_OK)           
                return -1;
            }
        else
            {
            if (hal_gpio_set_output(TRIGGER_PIN, HAL_GPIO_DATA_HIGH) != HAL_GPIO_STATUS_OK)           
                return -1;
            }
        }

    return 0;
}

void gpio_trigger_init_task(void *param)
{
    while(1)
        {
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
        hardware_trigger_status = true;
        set_trigger_pin(1);
        vTaskDelay(5000);
        set_trigger_pin(0);
        hardware_trigger_status = false;
        }
}

int gpio_trigger_task_create(void)
{
    if (xTaskCreate(gpio_trigger_init_task,
                    gpio_trigger_TASK_NAME,
                    gpio_trigger_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                    NULL,
                    gpio_trigger_TASK_PRIO,
                    &gpio_trigger_task_hdl) != pdPASS) {
        cli_puts("xTaskCreate fail\r\n");
        return -1;
    }

    return 0;
}

uint8_t trigger_pin_enable(void)
{
    if (!pin_status)
        {
        if (hal_gpio_init(TRIGGER_PIN) != HAL_GPIO_STATUS_OK)                                     return -1;
        if (hal_pinmux_set_function(TRIGGER_PIN, 0) != HAL_PINMUX_STATUS_OK)                      return -1;
        if (hal_gpio_set_direction(TRIGGER_PIN, HAL_GPIO_DIRECTION_OUTPUT) != HAL_GPIO_STATUS_OK) return -1;
        if (hal_gpio_set_output(TRIGGER_PIN, HAL_GPIO_DATA_HIGH) != HAL_GPIO_STATUS_OK)           return -1;
        gpio_trigger_task_create();
        pin_status = true;
        }

    return 0;
}

uint8_t trigger_pin_disable(void)
{
    if (pin_status)
        {
        if (hal_gpio_deinit(TRIGGER_PIN) != HAL_GPIO_STATUS_OK)                                     
            return -1;
        vTaskDelete(gpio_trigger_task_hdl);
        pin_status = false;
        }

    return 0;
}

uint8_t bs_cli_init(uint8_t len, char *param[])
{
    if (trigger_pin_enable() != 0)
        return -1;
    
    hid_interface_init();
    uart_interface_init();

    return 0;
}

uint8_t bs_cli_deinit(uint8_t len, char *param[])
{
    hid_interface_deinit();
    uart_interface_deinit();
    
    trigger_pin_disable();

    return 0;
}

uint8_t bs_cli_scan(uint8_t len, char *param[])
{
    uint32_t trigger_mode;

    if (len != 1)
        {
        cli_puts("Invalid parameter!\r");
        return -1;
        }
    trigger_mode = atoi(param[0]);

    if (trigger_mode == 0)       //uart command serial trigger, the scanner needs to be set to serial trigger mode
        scanner_serial_trigger_scan();
    else if (trigger_mode == 1)  //hardware pin trigger
        {
        if (hardware_trigger_status)
            cli_puts("Session is running!\r");
        else
            xTaskNotifyGive( gpio_trigger_task_hdl );
        }
    else
        cli_puts("Invalid parameter!\r");

    return 0;
}
/****************************************************************************
 *
 * API variable.
 *
 ****************************************************************************/


/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/

cmd_t barcode_scanner_cli[] = {
    { "init"  ,    "init barcode scanner interface"                                   ,bs_cli_init  ,    NULL  },
    { "deinit",    "deinit barcode scanner interface"                                 ,bs_cli_deinit,    NULL  },
    { "scan"  ,    "trigger scanner to scan 0:serial trigger mode, 1:hardware trigger",bs_cli_scan  ,    NULL  },
    { "hid"   ,    "hid interface"                                                    ,NULL         ,    &mt84g_hid_cli[0]   },
    { "uart"  ,    "uart interface"                                                   ,NULL         ,    &mt84g_uart_cli[0]  },
    { NULL }
};

#endif /* #if defined(MTK_MINICLI_ENABLE) */