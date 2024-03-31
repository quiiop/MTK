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
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/****************************************************************************
 * INCLUDE
 ****************************************************************************/


#include <stdlib.h>
#include <stdbool.h>

#ifdef HAL_CLOCK_MODULE_ENABLED
#include "hal_clock.h"
#include "hal_clk.h"
#include "hal_spm.h"
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

#include "hal_ecc.h"
#include "hal_ecc_internal.h"

/****************************************************************************
 * ON/OFF OPTIONS
 ****************************************************************************/


#ifndef __ECC_P_192__
#define __ECC_P_192__ 1
#endif /* #ifndef __ECC_P_192__ */
#ifndef __ECC_P_224__
#define __ECC_P_224__ 1
#endif /* #ifndef __ECC_P_224__ */
#ifndef __ECC_P_256__
#define __ECC_P_256__ 1
#endif /* #ifndef __ECC_P_256__ */
#ifndef __ECC_P_384__
#define __ECC_P_384__ 1
#endif /* #ifndef __ECC_P_384__ */
#ifndef __ECC_P_521__
#define __ECC_P_521__ 1
#endif /* #ifndef __ECC_P_521__ */


//#define HAL_ECC_MODULE_DEBUG_ENABLED


#ifdef HAL_ECC_MODULE_DEBUG_ENABLED
#define HAL_ECC_MODULE_DEBUG_WITH_STDIO
#endif /* #ifdef HAL_ECC_MODULE_DEBUG_ENABLED */



/****************************************************************************
 * MACROS
 ****************************************************************************/

#ifndef WRITE_REG
#define WRITE_REG(val,addr)         ((*(volatile uint32_t *)(addr)) = (unsigned int)val)
#endif /* #ifndef WRITE_REG */
#ifndef READ_REG
#define READ_REG(addr)              (*(volatile uint32_t *)(addr))
#endif /* #ifndef READ_REG */
#define ECC_WRITE_REG(val, addr)    WRITE_REG(val, addr)
#define ECC_READ_REG(addr)          READ_REG(addr)


/****************************************************************************
 * STRUCTURES
 ****************************************************************************/


/****************************************************************************
 * CURVE CONSTANT PARAMETERS
 ****************************************************************************/


typedef struct _ecc_param_t {
    uint32_t             *p;
    uint32_t         *p_neg; /* p + p_neg = 1...000 */
    uint32_t             *n; /* order */
    uint32_t         *n_neg;
    uint32_t    *pow2n_modp; /* 2 ^ 2n Mod P */
    uint32_t    *pow2n_modn; /* 2 ^ 2n Mod N */
    uint32_t             *a;
    uint32_t           *one; /* num: 1 */
} ecc_param_t;


typedef struct _ecc_p_param_t {
    uint32_t           p[8];
    uint32_t       p_neg[8]; /* p + p_neg = 1...000 */
    uint32_t           n[8]; /* order */
    uint32_t       n_neg[8];
    uint32_t  pow2n_modp[8]; /* 2 ^ 2n Mod P */
    uint32_t  pow2n_modn[8]; /* 2 ^ 2n Mod N */
    uint32_t           a[8];
    uint32_t         one[8]; /* num: 1 */
} ecc_p_param_t;


typedef struct _ecc_p384_param_t {
    uint32_t           p[12];
    uint32_t           n[12]; /* order */
    uint32_t       p_neg[12]; /* p + p_neg = 1...000 */
    uint32_t       n_neg[12];
    uint32_t  pow2n_modp[12]; /* 2 ^ 2n Mod P */
    uint32_t  pow2n_modn[12]; /* 2 ^ 2n Mod N */
    uint32_t           a[12];
    uint32_t         one[12]; /* num: 1 */
} ecc_p384_param_t;


typedef struct _ecc_p521_param_t {
    uint32_t           p[17];
    uint32_t           n[17]; /* order */
    uint32_t       p_neg[17]; /* p + p_neg = 1...000 */
    uint32_t       n_neg[17];
    uint32_t  pow2n_modp[17]; /* 2 ^ 2n Mod P */
    uint32_t  pow2n_modn[17]; /* 2 ^ 2n Mod N */
    uint32_t           a[17];
    uint32_t         one[17]; /* num: 1 */
} ecc_p521_param_t;


#if __ECC_P_192__
/*
    data_p: | FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFF
data_p_neg: | 000000000000000000000000000000010000000000000001
    data_n: | FFFFFFFFFFFFFFFFFFFFFFFF99DEF836146BC9B1B4D22831
data_n_neg: | 000000000000000000000000662107C9EB94364E4B2DD7CF
 2^192modP: | 000000000000000100000000000000020000000000000001
 2^192modN: | 28BE5677EA0581A24696EA5BBB3A6BEECE66BACCDEB35961
    data_a: | fffffffffffffffffffffffffffffffbfffffffffffffffc
*/
static const ecc_p_param_t gt_ECC_P_192_const_param = {
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000},
    {0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0xB4D22831, 0x146BC9B1, 0x99DEF836, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000},
    {0x4B2DD7CF, 0xEB94364E, 0x662107C9, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0x00000001, 0x00000000, 0x00000002, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000},
    {0xDEB35961, 0xCE66BACC, 0xBB3A6BEE, 0x4696EA5B, 0xEA0581A2, 0x28BE5677, 0x00000000, 0x00000000},
    {0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFB, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000},
    {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
};
#endif /* #if __ECC_P_192__ */

#if __ECC_P_224__
/*
    data_p: | FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000000001
data_p_neg: | 00000000000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF
    data_n: | FFFFFFFFFFFFFFFFFFFFFFFFFFFF16A2E0B8F03E13DD29455C5C2A3D
data_n_neg: | 0000000000000000000000000000E95D1F470FC1EC22D6BAA3A3D5C3
 2^224modP: | 00000000FFFFFFFFFFFFFFFFFFFFFFFE000000000000000000000001
 2^224modN: | D4BAA4CF1822BC47B1E979616AD09D9197A545526BDAAE6C3AD01289
    data_a: | fffffffffffffffffffffffffffffffc000000000000000000000004
*/
static const ecc_p_param_t gt_ECC_P_224_const_param = {
    {0x00000001, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0x5C5C2A3D, 0x13DD2945, 0xE0B8F03E, 0xFFFF16A2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000},
    {0xA3A3D5C3, 0xEC22D6BA, 0x1F470FC1, 0x0000E95D, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0x00000001, 0x00000000, 0x00000000, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000},
    {0x3AD01289, 0x6BDAAE6C, 0x97A54552, 0x6AD09D91, 0xB1E97961, 0x1822BC47, 0xD4BAA4CF, 0x00000000},
    {0x00000004, 0x00000000, 0x00000000, 0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000},
    {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
};
#endif /* #if __ECC_P_224__ */

#if __ECC_P_256__
/*
    data_p: | FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF
data_p_neg: | 00000000FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000000001
    data_n: | FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
data_n_neg: | 00000000FFFFFFFF00000000000000004319055258E8617B0C46353D039CDAAF
 2^256modP: | 00000004FFFFFFFDFFFFFFFFFFFFFFFEFFFFFFFBFFFFFFFF0000000000000003
 2^256modN: | 66E12D94F3D956202845B2392B6BEC594699799C49BD6FA683244C95BE79EEA2
    data_a: | fffffffc00000004000000000000000000000003fffffffffffffffffffffffc
 */
static const ecc_p_param_t gt_ECC_P_256_const_param = {
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF},
    {0x00000001, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE, 0x00000000},
    {0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF},
    {0x039CDAAF, 0x0C46353D, 0x58E8617B, 0x43190552, 0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000},
    {0x00000003, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFB, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFD, 0x00000004},
    {0xBE79EEA2, 0x83244C95, 0x49BD6FA6, 0x4699799C, 0x2B6BEC59, 0x2845B239, 0xF3D95620, 0x66E12D94},
    {0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000003, 0x00000000, 0x00000000, 0x00000004, 0xFFFFFFFC},
    {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
};
#endif /* #if __ECC_P_256__ */

#if __ECC_P_384__
/*
    data_p: | FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFF
data_p_neg: | 000000000000000000000000000000000000000000000000000000000000000100000000FFFFFFFFFFFFFFFF00000001
    data_n: | FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52973
data_n_neg: | 000000000000000000000000000000000000000000000000389CB27E0BC8D220A7E5F24DB74F58851313E695333AD68D
 2^192modP: | 000000000000000000000000000000010000000200000000FFFFFFFE000000000000000200000000FFFFFFFE00000001
 2^192modN: | 0C84EE012B39BF213FB05B7A28266895D40D49174AAB1CC5BC3E483AFCB82947FF3D81E5DF1AA4192D319B2419B409A9
    data_a: | FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBFFFFFFFC0000000000000003FFFFFFFC
       one: | 000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001
*/
ecc_p384_param_t gt_ECC_P_384_const_param = {
    {0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
    {0xCCC52973, 0xECEC196A, 0x48B0A77A, 0x581A0DB2, 0xF4372DDF, 0xC7634D81, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
    {0x00000001, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0x333AD68D, 0x1313E695, 0xB74F5885, 0xA7E5F24D, 0x0BC8D220, 0x389CB27E, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0x00000001, 0xFFFFFFFE, 0x00000000, 0x00000002, 0x00000000, 0xFFFFFFFE, 0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000000, 0x00000000},
    {0x19B409A9, 0x2D319B24, 0xDF1AA419, 0xFF3D81E5, 0xFCB82947, 0xBC3E483A, 0x4AAB1CC5, 0xD40D4917, 0x28266895, 0x3FB05B7A, 0x2B39BF21, 0x0C84EE01},
    {0xFFFFFFFC, 0x00000003, 0x00000000, 0xFFFFFFFC, 0xFFFFFFFB, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
    {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
};
#endif /* #if __ECC_P_384__ */

#if __ECC_P_521__
/*
    data_p: | 01FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
data_p_neg: | 000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001
    data_n: | 01FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFA51868783BF2F966B7FCC0148F709A5D03BB5C9B8899C47AEBB6FB71E91386409
data_n_neg: | 00000000000000000000000000000000000000000000000000000000000000000005AE79787C40D069948033FEB708F65A2FC44A36477663B851449048E16EC79BF7
 2^192modP: | 000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001
 2^192modN: | 002047A80E468E696D68EBFA3110E0F4B6380F4524B435156F31B586A3959EF33FCEE957B70D566936E65B7AF2F36F2B21D61A3B8F1D34C4028CE06B3DDA1D070851
    data_a: | 01FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC
       one: | 000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001
*/
ecc_p521_param_t gt_ECC_P_521_const_param = {
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x000001FF},
    {0x91386409, 0xBB6FB71E, 0x899C47AE, 0x3BB5C9B8, 0xF709A5D0, 0x7FCC0148, 0xBF2F966B, 0x51868783, 0xFFFFFFFA, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x000001FF},
    {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0x6EC79BF7, 0x449048E1, 0x7663B851, 0xC44A3647, 0x08F65A2F, 0x8033FEB7, 0x40D06994, 0xAE79787C, 0x00000005, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0x1D070851, 0xE06B3DDA, 0x34C4028C, 0x1A3B8F1D, 0x6F2B21D6, 0x5B7AF2F3, 0x566936E6, 0xE957B70D, 0x9EF33FCE, 0xB586A395, 0x35156F31, 0x0F4524B4, 0xE0F4B638, 0xEBFA3110, 0x8E696D68, 0x47A80E46, 0x00000020},
    {0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x000001FF},
    {0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
};
#endif /* #if __ECC_P_521__ */


#define ecccpy(d, s) \
    do { \
        (d)->p          = (uint32_t *)(s)->p; \
        (d)->p_neg      = (uint32_t *)(s)->p_neg; \
        (d)->n          = (uint32_t *)(s)->n; \
        (d)->n_neg      = (uint32_t *)(s)->n_neg; \
        (d)->pow2n_modp = (uint32_t *)(s)->pow2n_modp; \
        (d)->pow2n_modn = (uint32_t *)(s)->pow2n_modn; \
        (d)->a          = (uint32_t *)(s)->a; \
        (d)->one        = (uint32_t *)(s)->one; \
    } while (0)


/****************************************************************************
 * FORWARD DECLARATION
 ****************************************************************************/


uint8_t _hal_ecc_curve_read(
    const   hal_ecc_curve_t     curve,
    const   uint32_t            u4_sram_addr,
    uint32_t            *pu4_output);


/****************************************************************************
 * DEBUG CODE
 ****************************************************************************/


#include <assert.h>
#define HAL_ECC_ASSERT(__expr__)


#ifndef HAL_ECC_MODULE_DEBUG_ENABLED

#define do_nothing      do {} while(0)
#define V(fmt...)       do_nothing
#define E(fmt...)       do_nothing
#define D(fmt...)       do_nothing
#define D_HEX(fmt...)   do_nothing

#elif defined(HAL_ECC_MODULE_DEBUG_WITH_STDIO)

#include <stdio.h>
#include <hal_log.h>
#define E log_hal_error
#define D log_hal_debug
#define V log_hal_debug

void _32_to_8(uint32_t i, uint8_t *o)
{
    o[0] = ((i >> 24) & 0x000000ff);
    o[1] = ((i >> 16) & 0x000000ff);
    o[2] = ((i >> 8) & 0x000000ff);
    o[3] = ((i) & 0x000000ff);
}

void D_HEX(char *tag, uint32_t vector_no)
{
    size_t i;
    uint32_t vector[17];
    uint8_t p[68];

    _hal_ecc_curve_read(HAL_ECC_CURVE_NIST_P_521,
                        vector_no, &vector[0]);

    for (i = 0; i < 17; i ++)
        _32_to_8(vector[16 - i], &p[4 * i]);

    log_hal_debug("%s: ", tag);
    for (i = 0; i < sizeof(p); i++)
        log_hal_debug("%02X ", 0xFF & p[i]);
    log_hal_debug("\n");
}

#else /* #ifndef HAL_ECC_MODULE_DEBUG_ENABLED */

void V(const char *fmt, ...)
{
}

void E(const char *fmt, ...)
{
}

void D(const char *fmt, ...)
{
}

void D_HEX(const char *tag, uint32_t data)
{
}

#endif /* #ifndef HAL_ECC_MODULE_DEBUG_ENABLED */


/****************************************************************************
 * INTERNAL API
 ****************************************************************************/

/*
    Write data to Destination ECC Sram.
*/
static void _hal_ecc_curve_write(
    const   hal_ecc_curve_t     curve,
    const   uint32_t            u4_sram_addr,
    const   uint32_t            *pu4_input)
{
    uint8_t i;
    uint8_t key_len = 0;

    switch (curve) {
#if __ECC_P_192__
        case HAL_ECC_CURVE_NIST_P_192:
            key_len = 6;
            break;
#endif /* #if __ECC_P_192__ */
#if __ECC_P_224__
        case HAL_ECC_CURVE_NIST_P_224:
            key_len = 7;
            break;
#endif /* #if __ECC_P_224__ */
#if __ECC_P_256__
        case HAL_ECC_CURVE_NIST_P_256:
            key_len = 8;
            break;
#endif /* #if __ECC_P_256__ */
#if __ECC_P_384__
        case HAL_ECC_CURVE_NIST_P_384:
            key_len = 12;
            break;
#endif /* #if __ECC_P_384__ */
#if __ECC_P_521__
        case HAL_ECC_CURVE_NIST_P_521:
            key_len = 17;
            break;
#endif /* #if __ECC_P_521__ */
        default:
            break;
    }

    if (key_len > 0) {
        /* write ECC memory address */
        ECC_WRITE_REG(u4_sram_addr, ECC_ADDR_RW_MEM_ADDR);

        /* Write data to register. register17 is top digit. */
        for (i = 0; i < key_len; i++)
            ECC_WRITE_REG(pu4_input[i], (ECC_ADDR_RW_WDATA_00 + 4 * i));

        for (i = key_len; i < 17; i ++)
            ECC_WRITE_REG(0, (ECC_ADDR_RW_WDATA_00 + 4 * i));

        /* Trigger HW */
        ECC_WRITE_REG(ECC_VALUE_ONE, ECC_ADDR_WO_SRAM_WR);
    }
}


/*
 * Read data from ECC memory.
 */
uint8_t _hal_ecc_curve_read(
    const   hal_ecc_curve_t     curve,
    const   uint32_t            u4_sram_addr,
    uint32_t            *pu4_output)
{
    uint8_t i;
    uint8_t key_len = 0;

    switch (curve) {
#if __ECC_P_192__
        case HAL_ECC_CURVE_NIST_P_192:
            key_len =  6;
            break;
#endif /* #if __ECC_P_192__ */
#if __ECC_P_224__
        case HAL_ECC_CURVE_NIST_P_224:
            key_len =  7;
            break;
#endif /* #if __ECC_P_224__ */
#if __ECC_P_256__
        case HAL_ECC_CURVE_NIST_P_256:
            key_len =  8;
            break;
#endif /* #if __ECC_P_256__ */
#if __ECC_P_384__
        case HAL_ECC_CURVE_NIST_P_384:
            key_len = 12;
            break;
#endif /* #if __ECC_P_384__ */
#if __ECC_P_521__
        case HAL_ECC_CURVE_NIST_P_521:
            key_len = 17;
            break;
#endif /* #if __ECC_P_521__ */
        default:
            break;
    }

    if (key_len) {
        /* Write sram address */
        ECC_WRITE_REG(u4_sram_addr, ECC_ADDR_RW_MEM_ADDR);

        /* Trigger HW */
        ECC_WRITE_REG(ECC_VALUE_ONE, ECC_ADDR_WO_SRAM_RD);

        /* read register value from ECC hardware. */
        for (i = 0; i < key_len; i++)
            pu4_output[i] = (uint32_t)ECC_READ_REG(ECC_ADDR_RO_RDATA_00 + 4 * i);
    }

    return key_len;
}


static inline void _hal_ecc_curve_write_param(
    const   hal_ecc_curve_t     curve,
    const   ecc_param_t         *param)
{
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_CON_P, param->p);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_CON_N, param->n);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_CON_P_NEG, param->p_neg);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_CON_N_NEG, param->n_neg);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_CON_22N_P, param->pow2n_modp);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_CON_22N_N, param->pow2n_modn);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_CON_A, param->a);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_CON_ONE, param->one);
}


static hal_ecc_status_t _hal_ecc_curve_write_common(
    const   hal_ecc_curve_t     curve,
    uint32_t            *config)
{
    uint32_t            ecc_config;
    hal_ecc_status_t    status = HAL_ECC_STATUS_HARDWARE_ERROR;
    ecc_param_t         param;

    V("%s: enter\n", __func__);

    HAL_ECC_ASSERT(NULL != config);

    ecc_config = *config ;

    switch (curve) {
#if __ECC_P_192__
        case HAL_ECC_CURVE_NIST_P_192:
            ecc_config |= ECC_CFG_NIST_P_192;
            ecccpy(&param, &gt_ECC_P_192_const_param);
            _hal_ecc_curve_write_param(curve, &param);
            status = HAL_ECC_STATUS_OK;
            break;
#endif /* #if __ECC_P_192__ */
#if __ECC_P_224__
        case HAL_ECC_CURVE_NIST_P_224:
            ecc_config |= ECC_CFG_NIST_P_224;
            ecccpy(&param, &gt_ECC_P_224_const_param);
            _hal_ecc_curve_write_param(curve, &param);
            status = HAL_ECC_STATUS_OK;
            break;
#endif /* #if __ECC_P_224__ */
#if __ECC_P_256__
        case HAL_ECC_CURVE_NIST_P_256:
            ecc_config |= ECC_CFG_NIST_P_256;
            ecccpy(&param, &gt_ECC_P_256_const_param);
            _hal_ecc_curve_write_param(curve, &param);
            status = HAL_ECC_STATUS_OK;
            break;
#endif /* #if __ECC_P_256__ */
#if __ECC_P_384__
        case HAL_ECC_CURVE_NIST_P_384:
            ecc_config |= ECC_CFG_NIST_P_384;
            ecccpy(&param, &gt_ECC_P_384_const_param);
            _hal_ecc_curve_write_param(curve, &param);
            status = HAL_ECC_STATUS_OK;
            break;
#endif /* #if __ECC_P_384__ */
#if __ECC_P_521__
        case HAL_ECC_CURVE_NIST_P_521:
            ecc_config |= ECC_CFG_NIST_P_521;
            ecccpy(&param, &gt_ECC_P_521_const_param);
            _hal_ecc_curve_write_param(curve, &param);
            status = HAL_ECC_STATUS_OK;
            break;
#endif /* #if __ECC_P_521__ */
        default:
            status = HAL_ECC_CURVE_NOT_SUPPORTED;
    }

    if (HAL_ECC_STATUS_OK == status) {
        *config = ecc_config;

        ECC_WRITE_REG(ecc_config, ECC_ADDR_RW_ECC_CFG);
    }

    V("%s: exit (%x)\n", __func__, status);

    return status;
}



#if __ECC_USE_IRQ__

/** TODO: IRQ supported TBD */

uint8_t g_ecc_lock = 0;

static hal_ecc_status_t _ecc_hw_check_status(void)
{
    uint32_t u4_ecc_st = 0;

    g_ecc_lock = 0;

    V("%s: enter\n", __func__);

    ECC_WRITE_REG(0x01, ECC_ADDR_WO_ECC_IRQ_CLR);
    ECC_WRITE_REG(ECC_VALUE_ONE, ECC_ADDR_WO_ECC_TRIG);

    do {
        if (0 != g_ecc_lock) {
            break;
        }
    } while (1);

    if (1 == g_ecc_lock)
        return HAL_ECC_STATUS_OK;
    else
        return HAL_ECC_STATUS_HARDWARE_ERROR;

#if 0
    u4_ecc_st = (uint32_t)ECC_READ_REG(ECC_ADDR_RO_ECC_STATUS);

    BOOT_LOG("[ecc_impl] %s[%X] \n", u4_ecc_st, __func__);

    if (u4_ecc_st & ECC_STATUS_MASK_ERROR)
        return HAL_ECC_STATUS_HARDWARE_ERROR;
    else
        return HAL_ECC_STATUS_OK;
#endif /* #if 0 */

}

#else /* #if __ECC_USE_IRQ__ */

/*
 * polling to check HW status.
 */
hal_ecc_status_t _ecc_hw_check_status(void)
{
    hal_ecc_status_t    status    = HAL_ECC_STATUS_OK;
    uint32_t            status_hw = HAL_ECC_STATUS_HARDWARE_ERROR;

    V("%s: enter\n", __func__);

#if 0
    /* For Test */
    uint32_t i;
    uint32_t au4_input[ECC_SRAM_SIZE_DWORD] = {0x96BF8549, 0xC379E404, 0xEDA108A5, 0x51F83623, 0x12D8D1B2, 0xA5FA5706, 0xE2CC225C, 0xF6F977C4};
    for (i = 16; i < 37; i ++)
        _hal_ecc_curve_write(i, au4_input);
#endif /* #if 0 */

    ECC_WRITE_REG(0x1, ECC_ADDR_WO_ECC_TRIG);

    do {
        status_hw = ECC_READ_REG(ECC_ADDR_RO_ECC_STATUS);

        if (!(status_hw & ECC_STATUS_MASK_BUSY)) {
            status = HAL_ECC_STATUS_OK;
            break;
        }

        if (status_hw & ECC_STATUS_MASK_ERROR)
            break;
    } while (1);

    ECC_WRITE_REG(0x01, ECC_ADDR_WO_ECC_IRQ_CLR);

    if (status_hw & ECC_STATUS_MASK_ERROR)
        E("%s: ecc fail (%x)\n", __func__, status_hw);

    V("%s: exit (%d)\n", __func__, status);

    return status;
}

#endif /* #if __ECC_USE_IRQ__ */


#if defined( HAL_CLOCK_MODULE_ENABLED ) && defined( MTK_HAL_CLK_CTP_SUPPORT )
static void _hal_ecc_clock_sel(top_cm33_ecc_clock_control_t select)
{
    if (select == ECC_CLK_SRC_DIV_300M)
        (*ECC_CLK_CTL) |= RG_ECC_CLK_SEL;
    else
        (*ECC_CLK_CTL) &= ~(RG_ECC_CLK_SEL);
}
#endif /* #if defined( HAL_CLOCK_MODULE_ENABLED ) && defined( MTK_HAL_CLK_CTP_SUPPORT ) */


static void _hal_ecc_clock_on(void)
{
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_ECC);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    ECC_WRITE_REG((ECC_CLK_CFG_ECLK_EN | ECC_CLK_CFG_PCLK_EN),
                  ECC_ADDR_RW_CLK_CFG);
    ECC_WRITE_REG(0x00, ECC_ADDR_RW_SWRST);
    ECC_WRITE_REG(0x11, ECC_ADDR_RW_SWRST);
}


static void _hal_ecc_clock_off(void)
{
    ECC_WRITE_REG(ECC_VALUE_ZERO, ECC_ADDR_RW_CLK_CFG);
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_ECC);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
}


/****************************************************************************
 * PUBLIC API
 ****************************************************************************/


bool hal_ecc_init(void)
{
    //hal_crypt_lock_take();

#if defined( HAL_CLOCK_MODULE_ENABLED )
    SRAM_PWR_ON_MSDC_ECC;
    if (hal_clock_enable(HAL_CLOCK_CG_ECC) != HAL_CLOCK_STATUS_OK)
        return false;

#if defined( MTK_HAL_CLK_CTP_SUPPORT )
    if (hal_clock_is_enabled(FM_TOP_PLL_DIV2_CK))
        _hal_ecc_clock_sel(ECC_CLK_SRC_DIV_300M);
#else /* #if defined( MTK_HAL_CLK_CTP_SUPPORT ) */
    if (hal_clock_is_enabled(HAL_CLOCK_CG_TOP_PLL))
        hal_clock_mux_select(HAL_CLOCK_SEL_ECC, CLK_ECC_CLKSEL_DIV_300M);
#endif /* #if defined( MTK_HAL_CLK_CTP_SUPPORT ) */
#endif /* #if defined( HAL_CLOCK_MODULE_ENABLED ) */

    //hal_crypt_lock_give();
    return true;
}


void hal_ecc_deinit(void)
{
    //hal_crypt_lock_take();

#if defined( HAL_CLOCK_MODULE_ENABLED )
#if defined( MTK_HAL_CLK_CTP_SUPPORT )
    _hal_ecc_clock_sel(ECC_CLK_SRC_XTAL);
#else /* #if defined( MTK_HAL_CLK_CTP_SUPPORT ) */
    hal_clock_mux_select(HAL_CLOCK_SEL_ECC, CLK_ECC_CLKSEL_XTAL);
#endif /* #if defined( MTK_HAL_CLK_CTP_SUPPORT ) */

    SRAM_PWR_DOWN_MSDC_ECC;
    hal_clock_disable(HAL_CLOCK_CG_ECC);
#endif /* #if defined( HAL_CLOCK_MODULE_ENABLED ) */

    //hal_crypt_lock_give();
}


hal_ecc_status_t hal_ecc_ecdsa_sign(
    const   hal_ecc_curve_t     curve,
    const   uint32_t            *pu4_d,
    const   uint32_t            *pu4_k,
    const   uint32_t            *pu4_e,
    uint32_t            *pu4_r,
    uint32_t            *pu4_s
)
{
    hal_ecc_status_t status    = HAL_ECC_STATUS_OK;
    uint32_t ecc_config        = ECC_CFG_MODE_NORMAL;

    V("%s: enter\n", __func__);

    //hal_crypt_lock_take();

    /* 2. turn on HW and reset */

    _hal_ecc_clock_on();

    /* 3. set constant */

    ecc_config |= ECC_CFG_CMD_ECDSA_SIGN;

    status = _hal_ecc_curve_write_common(curve, &ecc_config);

    if (HAL_ECC_STATUS_OK != status) {
        E("%s: fail (%d) \n", __func__, status);
        goto hal_ecc_ecdsa_sign_error;
    }

    /* 4. load k, d, e to hardware */

    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_SW_K, pu4_k);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_SW_D, pu4_d);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_E, pu4_e);

    /* 5. trigger and wait  */

    status = _ecc_hw_check_status();

    if (HAL_ECC_STATUS_OK != status) {
        E("%s: fail (%d) \n", __func__, status);
        goto hal_ecc_ecdsa_sign_error;
    }

    /* 6. read r, s from hardware */

    if (0 == _hal_ecc_curve_read(curve, ECC_SRAM_ADDR_R, pu4_r)) {
        goto hal_ecc_ecdsa_sign_error;
    }

    if (0 == _hal_ecc_curve_read(curve, ECC_SRAM_ADDR_S, pu4_s)) {
        goto hal_ecc_ecdsa_sign_error;
    }

    D("[ECC IMPL] %s >>>\n", __func__);
    D_HEX("k ", ECC_SRAM_ADDR_SW_K);
    D_HEX("d ", ECC_SRAM_ADDR_SW_D);
    D_HEX("e ", ECC_SRAM_ADDR_E);
    D_HEX("r ", ECC_SRAM_ADDR_R);
    D_HEX("s ", ECC_SRAM_ADDR_S);

hal_ecc_ecdsa_sign_error:

    _hal_ecc_clock_off();

    //hal_crypt_lock_give();

    V("%s: exit (%d)\n", __func__, status);

    return status;
}


hal_ecc_status_t hal_ecc_ecdsa_verify(
    const   hal_ecc_curve_t     curve,
    const   uint32_t            *pu4_r,
    const   uint32_t            *pu4_s,
    const   uint32_t            *pu4_e,
    const   uint32_t            *pu4_Qx,
    const   uint32_t            *pu4_Qy,
    uint32_t            *pu4_v)
{
    int32_t  status             = HAL_ECC_STATUS_INVALID_PARAMETER;
    uint32_t ecc_config         = ECC_CFG_MODE_NORMAL;

    V("%s: enter\n", __func__);

    //hal_crypt_lock_take();

    /* 1. turn on ECC hardware, reset to default. */

    _hal_ecc_clock_on();

    /* 2. prepare common curve parameters. */

    ecc_config |= ECC_CFG_CMD_ECDSA_VERIFY;

    status = _hal_ecc_curve_write_common(curve, &ecc_config);

    if (HAL_ECC_STATUS_OK != status) {
        E("%s: fail (%d) \n", __func__, status);
        goto hal_ecc_ecdsa_verify_error;
    }

    /* 3. write r, s, e, Qx, Qy to ECC hardware */

    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_R,   pu4_r);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_S,   pu4_s);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_E,   pu4_e);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_Q_X, pu4_Qx);
    _hal_ecc_curve_write(curve, ECC_SRAM_ADDR_Q_Y, pu4_Qy);

    /* 4. trigger hardware and wait for result. */

    status = _ecc_hw_check_status();

    if (HAL_ECC_STATUS_OK != status) {
        E("%s: fail (%d) \n", __func__, status);
        goto hal_ecc_ecdsa_verify_error;
    }

    /* 5. read v from ECC hardware. */

    if (0 == _hal_ecc_curve_read(curve, ECC_SRAM_ADDR_V, pu4_v)) {
        goto hal_ecc_ecdsa_verify_error;
    }

    D("%s: debug dump\n", __func__);
    D_HEX("e ", ECC_SRAM_ADDR_E);
    D_HEX("r ", ECC_SRAM_ADDR_R);
    D_HEX("s ", ECC_SRAM_ADDR_S);
    D_HEX("Qx", ECC_SRAM_ADDR_Q_X);
    D_HEX("Qy", ECC_SRAM_ADDR_Q_Y);
    D_HEX("v ", ECC_SRAM_ADDR_V);

hal_ecc_ecdsa_verify_error:

    /* 6. turn off ECC hardware */

    _hal_ecc_clock_off();

    //hal_crypt_lock_give();

    V("%s: exit (%d)\n", __func__, status);

    return status;
}
