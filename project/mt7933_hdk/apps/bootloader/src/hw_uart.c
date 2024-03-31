/* Copyright Statement:
 *
 * (C) 2020  MediaTek Inc. All rights reserved.
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


/****************************************************************************
 *
 * HEADER FILES
 *
 ****************************************************************************/


#include <common.h>
#include <hal_uart.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

// private
#include "hw_uart.h"


/****************************************************************************
 *
 * CONSTANTS AND MACROS
 *
 ****************************************************************************/


#define CAPSFLAG     0x00000040
#define BAUDRATE     921600


/****************************************************************************
 *
 * PRIVATE FUNCTIONS
 *
 ****************************************************************************/


#if defined(MTK_BL_DEBUG_ENABLE)


static unsigned long long div10(
	unsigned long long n,
	unsigned long long *rem)
{
    unsigned long long q;
    unsigned long long r;
    int i;

    q = r = 0;
    for (i = (sizeof(n) * 8 - 1); i >= 0; i--) {
        r <<= 1;
        r |= !!(n & ((unsigned long long)1 << i));
        if (r >= 10) {
            r = r - 10;
            q |= (1 << i);
        }
    }

    if (rem)
        *rem = r;

    return q;
}

static void unsigned_num_print(uint64_t unum, uint32_t radix, uint32_t flags)
{
    int i = 0;
    uint64_t rem;
    unsigned char num_buf[20];
    unsigned char c;
    bool cap;

    if (radix != 10 && radix != 16)
        return;

    cap = (flags & CAPSFLAG) == CAPSFLAG;

    do {
        if (radix == 16) {
            rem = unum & 0xF;
            unum >>= 4;
        } else {
            unum = div10(unum, &rem);
        }

        c = (unsigned char)rem;
        c += (c < 10) ? '0' : cap ? 'A' - 10 : 'a' - 10;
        num_buf[i++] = c;
    } while (unum);

    while (--i >= 0) {
        hw_uart_putc(num_buf[i]);
    }
}

/**********************************************************
Description : debug print used internally
Input       : Buffer pointer, value to convert.
Output      : None
***********************************************************/
static void bl_print_internal(const char *fmt, va_list ap)
{
    const char *p, *str;
    char c;
    unsigned int l_count;
    long long num;
    unsigned long long unum;
    unsigned int flags = 0;
    int i;

    for (i = 0, p = fmt; *p; p++, i++) {
        l_count = 0;

        if (*p == '%') {
            p++;
            i++;
loop:
            switch (*p) {
            case 'd':
                num = l_count > 1 ? va_arg(ap, long long) :
                    (l_count ? va_arg(ap, long) : va_arg(ap, int));
                if (num < 0) {
                    hw_uart_putc('-');
                    unum = (unsigned long long)-num;
                } else {
                    unum = (unsigned long long)num;
                }
                unsigned_num_print(unum, 10, 0);
                break;

            case 'u':
                unum = l_count > 1 ? va_arg(ap, unsigned long long) :
                    (l_count ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int));
                    unsigned_num_print(unum, 10, 0);
                break;

            case 'X':
                flags |= CAPSFLAG;
                /* fallthrough */
            case 'x':
                unum = l_count > 1 ? va_arg(ap, unsigned long long) :
                    (l_count ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int));

                unsigned_num_print(unum, 16, flags);
                break;

            case 'p':
                unum = (unsigned long long)va_arg(ap, void *);
                if (unum)
                    hw_uart_puts("0x");
                unsigned_num_print(unum, 16, 0);
                break;

            case 'c':
                c = va_arg(ap, int);
                hw_uart_putc(c);
                break;

            case 'l':
                l_count++;
                p++;
                i++;
                goto loop;

            case 's':
                str = va_arg(ap, char *);
                hw_uart_puts(str);
                break;

            case '%':
                hw_uart_putc('%');
                break;

            default:
                hw_uart_putc(*p);
                break;
            }
        } else if (*p == '\n') {
            if (i == 0 || *(p - 1) != '\r') {
                hw_uart_putc('\r');
                hw_uart_putc('\n');
            } else {
                hw_uart_putc('\n');
            }
        } else {
            hw_uart_putc(*p);
        }
    }
}


/****************************************************************************
 *
 * PUBLIC FUNCTIONS
 *
 ****************************************************************************/


int hw_uart_getc(void)
{
    return ReadDebugByte();
}

void hw_uart_putc(char c)
{
    WriteDebugByte(c);
}

void hw_uart_puts(const char *str)
{
    int i;

    if (str == NULL) {
        return;
    }

    for (i = 0; *str != '\0'; i++, str++) {
        if (*str == '\n') {
            if (i == 0 || *(str - 1) != '\r') {
                hw_uart_putc('\r');
                hw_uart_putc('\n');
            } else {
                hw_uart_putc('\n');
            }
        } else {
            hw_uart_putc(*str);
        }
    }
}

void hw_uart_init(void)
{
    InitDebugUart(BAUDRATE);
}

void hw_uart_printf( const char *str, ... )
{
    if ( str ) {
        va_list ap;
        va_start( ap, str );
        bl_print_internal( str, ap );
        va_end( ap );
    }
}

void hw_uart_vprintf( const char *format, va_list ap )
{
    if ( format ) {
        bl_print_internal( format, ap );
    }
}

#else

void hw_uart_init(void)
{
}


void hw_uart_puts(const char *str)
{
}

void hw_uart_putc(char c)
{
}

void hw_uart_printf(char *str, ...)
{
}

void hw_uart_vprintf( const char *format, va_list ap )
{
}

int hw_uart_getc(void)
{
    return -1;
}

#endif /* MTK_BL_DEBUG_ENABLE */
