/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */
/* Standard includes. */
#include "string.h"
#include "stdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Example includes. */
#include "FreeRTOS_CLI.h"
#include "mt_uart.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "task.h"
#include "types.h"
#ifdef CFG_LOGGER_SUPPORT
#include "logger.h"
#endif
#ifdef CFG_PRINT_TIMESTAMP
#include "systimer.h"
#endif

extern int __io_putchar(int ch) __attribute__((weak));
static inline void _dputc(char c)
{
#ifdef CFG_UART_SUPPORT
    __io_putchar(c);
#endif
}

int dputs(const char *str, int length)
{
    uint32_t ix = 0;
    while (str[ix] != '\0' && ix < length) {
        _dputc(str[ix]);
        ix++;
    }
#ifdef CFG_LOGGER_SUPPORT
    logger_puts(str, ix);
#endif
    return ix;
}

#define MAX_LOG_SIZE 256
static char printf_buf[MAX_LOG_SIZE + 4];

char *itoa(int i, char b[])
{
    char const digit[] = "0123456789";
    char *p = b;
    if (i < 0) {
        *p++ = '-';
        i *= -1;
    }
    int shifter = i;
    do { //Move to where representation ends
        ++p;
        shifter = shifter / 10;
    }
    while (shifter);
    *p = '\0';
    do { //Move back, inserting digits as u go
        *--p = digit[i % 10];
        i = i / 10;
    }
    while (i);
    return b;
}

#ifdef CFG_PRINT_TIMESTAMP
static void print_prefix_timestamp()
{
    char prefix_buf[26] = {0};
    char buff[12] = {0};
    int count = 0;
    unsigned long long time_ns ;
    unsigned long long second;
    unsigned long long mini_second;

    time_ns = read_systimer_stamp_ns();
    second = time_ns / 1000000000;
    mini_second = time_ns / 1000000 - second * 1000;

    strcpy(prefix_buf, "[");
    itoa(second, buff);
    strcat(prefix_buf, buff);
    strcat(prefix_buf, ".");
    itoa(mini_second, buff);
    if (mini_second == 0)
    {
        strcat(prefix_buf, "000");
    }
    else
    {
        /* count prefix 0 digit */
        while (mini_second) {
            mini_second /= 10;
            count ++;
        }
        /* print prefix 0 digit */
        for (; count < 3; count++) {
            strcat(prefix_buf, "0");
        }
        /* print the remaining numbers */
        strcat(prefix_buf, buff);
    }
    strcat(prefix_buf, "]");

    dputs(prefix_buf, sizeof(prefix_buf));
}
#endif

#ifdef CFG_HIFI4_DUAL_CORE
static void print_prefix_core_num()
{
#if defined(CFG_HIFI4_A)
    dputs("(0) ", 4);
#elif defined(CFG_HIFI4_B)
    dputs("(1) ", 4);
#endif
}
#endif

static void print_prefix_task_name()
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        dputs("[", 1);
        dputs(pcTaskGetTaskName(NULL), configMAX_TASK_NAME_LEN);
        dputs("]", 1);
    }
}

static void print_prefix()
{
#ifdef CFG_PRINT_TIMESTAMP
    print_prefix_timestamp();
#endif

#ifdef CFG_HIFI4_DUAL_CORE
    print_prefix_core_num();
#endif

    print_prefix_task_name();
}

int vprintf(const char *fmt, va_list ap)
{
    int len;
    int printed_len = 0;
    UBaseType_t uxSavedInterruptStatus;

    uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();

    print_prefix();

    memset(printf_buf, 0x0, sizeof(printf_buf));
    len = vsnprintf(printf_buf, MAX_LOG_SIZE, fmt, ap);
    if (printf_buf[len - 1] == '\n') {
        printf_buf[len - 1] = '\r';
        printf_buf[len++] = '\n';
    }
    printed_len += dputs(printf_buf, len);

    /* prevent from context switch when print out msg */
    /* NOTE: in ISR, there is no prevention */
    portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);

    return printed_len;
}

int __wrap_printf(const char *fmt, ...)
{
    va_list args;
    int len;

    va_start(args, fmt);
    len = vprintf(fmt, args);
    va_end(args);

    return len;
}

unsigned long simple_strtoul(const char *nptr, char **endptr, int base)
{
    const char *s;
    unsigned long acc, cutoff;
    int c;
    int neg, any, cutlim;

    s = nptr;
    do {
        c = (unsigned char) *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else {
        neg = 0;
        if (c == '+')
            c = *s++;
    }
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;

    cutoff = ULONG_MAX / (unsigned long)base;
    cutlim = ULONG_MAX % (unsigned long)base;
    for (acc = 0, any = 0;; c = (unsigned char) *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0)
            continue;
        if (acc > cutoff || (acc == cutoff && c > cutlim)) {
            any = -1;
            acc = ULONG_MAX;
            errno = ERANGE;
        } else {
            any = 1;
            acc *= (unsigned long)base;
            acc += c;
        }
    }
    if (neg && any > 0)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? s - 1 : nptr);
    return (acc);
}

/*
 * mt_str2ul - This function will convert a numeric string into
 *	an unsigned int value.
 *	Note: Prohibit sscanf(), which will eat too much text-size.
 * Parameters:
 *	arg: A character string representing a numeric value.
 * Outputs:
 *	*value: the unsigned int represntation of arg.
 * Returns:
 *	Zero on success, other value on failure.
 */
int mt_str2ul(const char *arg, unsigned int *value)
{
    char *endp;
    unsigned long val;

    val = simple_strtoul(arg, &endp, 0);
    if (endp == arg) {
        /*
         * Also try base 16, for us folks too lazy to type the
         * leading 0x...
         */
        val = simple_strtoul(arg, &endp, 16);
        if (endp == arg)
            return EINVAL;
    }

    *value = (unsigned int)val;
    return 0;
}

#ifdef CFG_DYNAMIC_DEBUG
void set_loglevel(int level)
{
    g_log_level = level;
}

int get_loglevel()
{
    return g_log_level;
}
#endif

void printf_common(int level, const char *fmt, ...)
{
    va_list args;
#ifdef CFG_DYNAMIC_DEBUG
    if (level <= get_loglevel())
{
#endif
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
#ifdef CFG_DYNAMIC_DEBUG
    }
#endif
}

