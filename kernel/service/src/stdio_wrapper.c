/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * Copyright  (C) 2022  MediaTek Inc. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include <stdio.h>
#include "memory_attribute.h"

extern int log_write(char *buf, int len);

#ifdef MPALAND_PRINTF
/**
 * https://github.com/eyalroz/printf
 */
#include <printf_config.h>
#include <printf/printf.h>

void putchar_(char c)
{
    log_write(&c, 1);
}

ATTR_USED int __wrap_fprintf(FILE *__restrict fp, const char *__restrict fmt, ...)
{
    va_list ap;
    int ret;
    (void)fp;

    va_start(ap, fmt);
    ret = vprintf_(fmt, ap);
    va_end(ap);

    return ret;
}

ATTR_USED int __wrap_printf(const char *__restrict fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vprintf_(fmt, ap);
    va_end(ap);

    return ret;
}

ATTR_USED int __wrap_fiprintf(FILE *fp, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vprintf_(fmt, ap);
    va_end(ap);

    return ret;
}

ATTR_USED int __wrap_vprintf(const char *fmt, __VALIST va)
{
    return vprintf_(fmt, va);
}

ATTR_USED int __wrap_sprintf(char *__restrict str, const char *__restrict fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vsprintf_(str, fmt, ap);
    va_end(ap);

    return ret;
}

ATTR_USED int __wrap_vsprintf(char *__restrict str, const char *__restrict fmt, __VALIST va)
{
    return vsprintf_(str, fmt, va);
}

ATTR_USED int __wrap_snprintf(char *__restrict str, size_t size, const char *__restrict fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vsnprintf_(str, size, fmt, ap);
    va_end(ap);

    return ret;
}

ATTR_USED int __wrap_vsnprintf(char *__restrict str, size_t size, const char *__restrict fmt, __VALIST va)
{
    return vsnprintf_(str, size, fmt, va);
}

#else /* #ifdef MPALAND_PRINTF */
/**
 * https://github.com/MaJerle/lwprintf
 */
#include "lwprintf/lwprintf.h"

ATTR_ZIDATA_IN_TCM static int g_stdio_wrapper_inited;

int lwprintf_output_cb(int ch, lwprintf_t *lw)
{
    return log_write((char *)&ch, 1);
}

void stdio_wrapper_init(void)
{
    if (!g_stdio_wrapper_inited) {
        lwprintf_init(lwprintf_output_cb);
        g_stdio_wrapper_inited = 1;
    }
}

ATTR_USED int __wrap_fprintf(FILE *__restrict fp, const char *__restrict fmt, ...)
{
    va_list ap;
    int ret;
    (void)fp;

    if (!g_stdio_wrapper_inited) {
        stdio_wrapper_init();
    }

    va_start(ap, fmt);
    ret = lwvprintf(fmt, ap);
    va_end(ap);

    return ret;
}

ATTR_USED int __wrap_printf(const char *__restrict fmt, ...)
{
    va_list ap;
    int ret;

    if (!g_stdio_wrapper_inited) {
        stdio_wrapper_init();
    }

    va_start(ap, fmt);
    ret = lwvprintf(fmt, ap);
    va_end(ap);

    return ret;
}

ATTR_USED int __wrap_fiprintf(FILE *fp, const char *fmt, ...)
{
    va_list ap;
    int ret;

    if (!g_stdio_wrapper_inited) {
        stdio_wrapper_init();
    }

    va_start(ap, fmt);
    ret = lwvprintf(fmt, ap);
    va_end(ap);

    return ret;
}

ATTR_USED int __wrap_vprintf(const char *fmt, __VALIST va)
{
    if (!g_stdio_wrapper_inited) {
        stdio_wrapper_init();
    }

    return lwvprintf(fmt, va);
}

ATTR_USED int __wrap_sprintf(char *__restrict str, const char *__restrict fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = lwvsnprintf(str, SIZE_MAX, fmt, ap);
    va_end(ap);

    return ret;
}

ATTR_USED int __wrap_vsprintf(char *__restrict str, const char *__restrict fmt, __VALIST va)
{
    return lwvsnprintf(str, SIZE_MAX, fmt, va);
}

ATTR_USED int __wrap_snprintf(char *__restrict str, size_t size, const char *__restrict fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = lwvsnprintf(str, size, fmt, ap);
    va_end(ap);

    return ret;
}

ATTR_USED int __wrap_vsnprintf(char *__restrict str, size_t size, const char *__restrict fmt, __VALIST va)
{
    return lwvsnprintf(str, size, fmt, va);
}
#endif /* #ifdef MPALAND_PRINTF */
