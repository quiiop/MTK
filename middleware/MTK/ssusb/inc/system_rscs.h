/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/*
 * MediaTek Inc. (C) 2018. All rights reserved.
 *
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


#ifndef SYSTEM_RSCS_H
#define SYSTEM_RSCS_H

#include <driver_api.h>
#include <typedefs.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#ifndef NULL
#define NULL ((void *)0)
#endif /* #ifndef NULL */

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* #ifndef MIN */

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* #ifndef MAX */

#undef udelay
#undef mdelay

#ifndef udelay
#define udelay(u) hal_gpt_delay_us(u)
#endif /* #ifndef udelay */

#ifndef mdelay
#define mdelay(u) hal_gpt_delay_ms(u)
#endif /* #ifndef mdelay */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif /* #ifndef ARRAY_SIZE */

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif /* #ifndef DIV_ROUND_UP */

#ifndef dsb
#define dsb() __asm__ __volatile__ ("dsb" : : : "memory")
#endif /* #ifndef dsb */

#ifndef isb
#define isb() __asm__ __volatile__ ("isb" : : : "memory")
#endif /* #ifndef isb */

#ifndef mb
#define mb() \
    do { \
        dsb(); \
    } while (0)
#endif /* #ifndef mb */

#ifndef readw
#define readw(addr) (*(volatile unsigned short *)(addr))
#endif /* #ifndef readw */

#ifndef readb
#define readb(addr) (*(volatile unsigned char *)(addr))
#endif /* #ifndef readb */

#ifndef writew

#define writew(value, addr) \
    do { \
        ((*(volatile unsigned short *)(addr)) = (unsigned short)value); \
        mb(); \
    } while (0)
#endif /* #ifndef writew */

#ifndef writeb
#define writeb(value, addr) \
    do { \
        ((*(volatile unsigned char *)(addr)) = (unsigned char)value); \
        mb(); \
    } while (0)
#endif /* #ifndef writeb */

#endif /* #ifndef SYSTEM_RSCS_H */

