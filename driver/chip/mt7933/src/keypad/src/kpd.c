/*
 * Copyright (c) 2019 MediaTek Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
//#include "FreeRTOS.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "kpd.h"
#include "hal_gpio.h"
#include "hal_eint.h"
#include "hal_keypad.h"
#include "hal_nvic_internal.h"
#include <common.h>

static unsigned int kpd_keymap[] = {
    1, 2, 3, 0, 0, 0, 0, 0, 0,
    4, 5, 6, 0, 0, 0, 0, 0, 0,
    7, 8, 9, 0, 0, 0, 0, 0, 0
};
static unsigned short kpd_keymap_state[KPD_NUM_MEMS] = {0xffff, 0xffff};
static unsigned int pio_keypad_col_pin_queue[COL_PIN_NUM] = {
    COL0_PIN,
    COL1_PIN,
    COL2_PIN
};

static unsigned int pio_keypad_row_pin_queue[ROW_PIN_NUM] = {
    ROW0_PIN,
    ROW1_PIN,
    ROW2_PIN
};
static int ut_pressed = 0;
static unsigned short ut_linux_keycode = 0;

static void keypad_set_gpio(void)
{
    int i = 0;

    for (i = 0; i < COL_PIN_NUM; i++) {
        hal_gpio_init(pio_keypad_col_pin_queue[i]);
        hal_pinmux_set_function(pio_keypad_col_pin_queue[i], 3);
        hal_gpio_set_direction(pio_keypad_col_pin_queue[i], HAL_GPIO_DIRECTION_INPUT);
        hal_gpio_pull_up(pio_keypad_col_pin_queue[i]);
    }

    for (i = 0; i < ROW_PIN_NUM; i++) {
        hal_gpio_init(pio_keypad_row_pin_queue[i]);
        hal_pinmux_set_function(pio_keypad_row_pin_queue[i], 3);
        hal_gpio_set_direction(pio_keypad_row_pin_queue[i], HAL_GPIO_DIRECTION_OUTPUT);
        hal_gpio_set_output(pio_keypad_row_pin_queue[i], HAL_GPIO_DATA_LOW);
    }
    return;
}

static void keypad_set_debounce(void)
{
    DRV_WriteReg(KEY_DEBOUNCE & KPD_DEBOUNCE_MASK, KP_DEBOUNCE);
    return;
}

static void kpd_get_keymap_state(unsigned short state[])
{
    state[0] = DRV_Reg(KP_MEM1);
    state[1] = DRV_Reg(KP_MEM2);
    printf("kpd_get_keymap_state done: %x %x!\n", state[0],
           state[1]);
    return;
}

static void keypad_interrupt_handler(void)
{
    int i, j;
    int pressed;
    unsigned short new_state[KPD_NUM_MEMS], change, mask;
    unsigned short hw_keycode, linux_keycode;

    printf("keypad_interrupt_handler start!\n");
    kpd_get_keymap_state(new_state);

    for (i = 0; i < KPD_NUM_MEMS; i++) {
        change = new_state[i] ^ kpd_keymap_state[i];
        if (change == 0U)
            continue;

        for (j = 0; j < 16U; j++) {
            mask = (u16) 1 << j;
            if ((change & mask) == 0U)
                continue;

            hw_keycode = (i << 4) + j;

            if (hw_keycode >= KPD_NUM_KEYS)
                continue;

            /* bit is 1: not pressed, 0: pressed */
            pressed = ((new_state[i] & mask) == 0U) ? 1 : 0;
            printf("(%s) HW keycode = %d\n",
                   (pressed == 1) ? "pressed" : "released",
                   hw_keycode);

            linux_keycode = kpd_keymap[hw_keycode];
            if (linux_keycode == 0U)
                continue;

            ut_pressed = pressed;
            ut_linux_keycode = linux_keycode;

            printf("report Linux keycode = %d\n", linux_keycode);

        }
    }

    memcpy(kpd_keymap_state, new_state, sizeof(new_state));
    return;
}

hal_keypad_status_t hal_keypad_get_key(hal_keypad_event_t *keypad_event)
{
    if (ut_pressed) {
        keypad_event->key_data = (uint32_t)ut_linux_keycode;
        return HAL_KEYPAD_STATUS_OK;
    }
    return HAL_KEYPAD_STATUS_ERROR;
}

static void keypad_request_irq(void)
{
    hal_nvic_disable_irq(KEY_INT_NUM);
    hal_nvic_irq_set_type(KEY_INT_NUM, HAL_NVIC_IRQ_TYPE_EDGE_RISING);
    hal_nvic_set_priority(KEY_INT_NUM, KP_IRQ_PRIORITY);
    hal_nvic_register_isr_handler(KEY_INT_NUM, (hal_nvic_isr_t)keypad_interrupt_handler);
    hal_nvic_enable_irq(KEY_INT_NUM);
    return;
}

void keypad_register_dump(void)
{
    unsigned short state[7] = {0};

    state[0] = DRV_Reg(KP_STA);
    state[1] = DRV_Reg(KP_MEM1);
    state[2] = DRV_Reg(KP_MEM2);
    state[3] = DRV_Reg(KP_DEBOUNCE);
    state[4] = DRV_Reg(KP_SCAN_TIMING);
    state[5] = DRV_Reg(KP_SEL);
    state[6] = DRV_Reg(KP_EN);

    printf("dump kpd register: %x %x %x %x %x %x %x!\n", state[0],
           state[1], state[2], state[3], state[4], state[5], state[6]);
}

hal_keypad_status_t hal_keypad_init(const hal_keypad_config_t *keypad_config)
{
    printf("hal_keypad_init \n");
    keypad_set_gpio();
    printf("keypad_set_gpio done \n");
    keypad_set_debounce();
    printf("keypad_set_debounce done \n");
    keypad_request_irq();
    printf("keypad_request_irq done \n");
    return HAL_KEYPAD_STATUS_OK;
}
