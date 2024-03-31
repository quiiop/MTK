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
#ifndef __MT_GPIO_KEYS_H
#define __MT_GPIO_KEYS_H
#include "hal_eint.h"
//#include "FreeRTOS.h"
//#include "queue.h"
#include "hal_platform.h"
#include "hal_gpio.h"
#include "hal_nvic.h"
#include "hal_gpt.h"
#include "hal_platform.h"
#define GPIO_NUM    116
#define KEY_NUM     9
#define COL_PIN_NUM     3
#define ROW_PIN_NUM     3
#define IRQ_NUM 42
#define KEY_DEBOUNCE    50000
#define KEY_INT_NUM     42
#define KPD_NUM_MEMS    2
#define KPD_NUM_KEYS    48

#define COL0_PIN        50
#define COL1_PIN        51
#define COL2_PIN        52
#define ROW0_PIN        47
#define ROW1_PIN        48
#define ROW2_PIN        49

#define KP_BASE         0x30485000
#define KP_STA          (KP_BASE + 0x0000)
#define KP_MEM1         (KP_BASE + 0x0004)
#define KP_MEM2         (KP_BASE + 0x0008)
#define KP_DEBOUNCE     (KP_BASE + 0x0018)
#define KP_SCAN_TIMING      (KP_BASE + 0x001C)
#define KP_SEL          (KP_BASE + 0x0020)
#define KP_EN           (KP_BASE + 0x0024)

#define KPD_DEBOUNCE_MASK   ((1U << 14) - 1)

#endif /* #ifndef __MT_GPIO_KEYS_H */
