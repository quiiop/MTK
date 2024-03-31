/* Copyright Statement:
 *
 * (C) 2005-2030  MediaTek Inc. All rights reserved.
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ut.h"

#if defined(UT_PLATFORM_ENABLE) && defined (UT_PLATFORM_LAYOUT_ENABLE)

#include "memory_attribute.h"

#define  UT_PLAT_LAYOUT_SYSRAM   1
#define  UT_PLAT_LAYOUT_RAM      0
#define  UT_PLAT_LAYOUT_TCM      1

#if (UT_PLAT_LAYOUT_SYSRAM == 1)
extern unsigned long _sysram_code_start;
extern unsigned long _sysram_code_end;
extern unsigned long _sysram_data_start;
extern unsigned long _sysram_data_end;
extern unsigned long _sysram_bss_start;
extern unsigned long _sysram_bss_end;

extern unsigned long _noncached_sysram_code_start;
extern unsigned long _noncached_sysram_code_end;
extern unsigned long _noncached_sysram_data_start;
extern unsigned long _noncached_sysram_data_end;
extern unsigned long _noncached_sysram_bss_start;
extern unsigned long _noncached_sysram_bss_end;
#endif /* #if (UT_PLAT_LAYOUT_SYSRAM == 1) */

#if (UT_PLAT_LAYOUT_RAM == 1)
extern unsigned long _ram_code_start;
extern unsigned long _ram_code_end;
extern unsigned long _ram_data_start;
extern unsigned long _ram_data_end;
extern unsigned long _ram_bss_start;
extern unsigned long _ram_bss_end;

extern unsigned long _noncached_ram_code_start;
extern unsigned long _noncached_ram_code_end;
extern unsigned long _noncached_ram_data_start;
extern unsigned long _noncached_ram_data_end;
extern unsigned long _noncached_ram_bss_start;
extern unsigned long _noncached_ram_bss_end;
#endif /* #if (UT_PLAT_LAYOUT_RAM == 1) */

#if (UT_PLAT_LAYOUT_TCM == 1)
extern unsigned long _tcm_code_start;
extern unsigned long _tcm_code_end;
extern unsigned long _tcm_data_start;
extern unsigned long _tcm_data_end;
extern unsigned long _tcm_bss_start;
extern unsigned long _tcm_bss_end;
#endif /* #if (UT_PLAT_LAYOUT_TCM == 1) */


#define UT_PLAT_CONST_VAR 10
#if (UT_PLAT_LAYOUT_SYSRAM == 1)
ATTR_RWDATA_IN_SYSRAM int g_sram_rw = 20;
ATTR_ZIDATA_IN_SYSRAM int g_sram_zi;

ATTR_RWDATA_IN_NONCACHED_SYSRAM int g_noncached_sram_rw = 20;
ATTR_ZIDATA_IN_NONCACHED_SYSRAM int g_noncached_sram_zi;
#endif /* #if (UT_PLAT_LAYOUT_SYSRAM == 1) */

#if (UT_PLAT_LAYOUT_RAM == 1)
ATTR_RWDATA_IN_RAM int g_ram_rw = 20;
ATTR_ZIDATA_IN_RAM int g_ram_zi;

ATTR_RWDATA_IN_NONCACHED_RAM int g_noncached_ram_rw = 20;
ATTR_ZIDATA_IN_NONCACHED_RAM int g_noncached_ram_zi;
#endif /* #if (UT_PLAT_LAYOUT_RAM == 1) */

#if (UT_PLAT_LAYOUT_TCM == 1)
ATTR_RWDATA_IN_TCM int g_tcm_rw = 20;
ATTR_ZIDATA_IN_TCM int g_tcm_zi;
#endif /* #if (UT_PLAT_LAYOUT_TCM == 1) */


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

#if (UT_PLAT_LAYOUT_SYSRAM == 1)
ATTR_TEXT_IN_SYSRAM int plat_fun_in_sysram(int v1)
{
    int a = 30;
    int b = 40;
    int val;

    g_sram_zi = b;
    val = (a * g_sram_zi * g_sram_rw) + UT_PLAT_CONST_VAR + v1;

    printf("\t Oper: (%d*%d*%d)+%d+%d = %d \r\n", a, g_sram_zi, g_sram_rw, UT_PLAT_CONST_VAR, v1, val);

    return val;
}

ATTR_TEXT_IN_NONCACHED_SYSRAM int plat_fun_in_noncached_sysram(int v1)
{
    int a = 30;
    int b = 40;
    int val;

    g_noncached_sram_zi = b;
    val = (a * g_noncached_sram_zi * g_noncached_sram_rw) + UT_PLAT_CONST_VAR + v1;

    printf("\t Oper: (%d*%d*%d)+%d+%d = %d \r\n", a, g_noncached_sram_zi, g_noncached_sram_rw, UT_PLAT_CONST_VAR, v1, val);

    return val;
}
#endif /* #if (UT_PLAT_LAYOUT_SYSRAM == 1) */


#if (UT_PLAT_LAYOUT_RAM == 1)
ATTR_TEXT_IN_RAM int plat_fun_in_ram(int v1)
{
    int a = 30;
    int b = 40;
    int val;

    g_ram_zi = b;
    val = (a * g_ram_zi * g_ram_rw) + UT_PLAT_CONST_VAR + v1;

    printf("\t Oper: (%d*%d*%d)+%d+%d = %d \r\n", a, g_ram_zi, g_ram_rw, UT_PLAT_CONST_VAR, v1, val);

    return val;
}

ATTR_TEXT_IN_NONCACHED_RAM int plat_fun_in_noncached_ram(int v1)
{
    int a = 30;
    int b = 40;
    int val;

    g_noncached_ram_zi = b;
    val = (a * g_noncached_ram_zi * g_noncached_ram_rw) + UT_PLAT_CONST_VAR + v1;

    printf("\t Oper: (%d*%d*%d)+%d+%d = %d \r\n", a, g_noncached_ram_zi, g_noncached_ram_rw, UT_PLAT_CONST_VAR, v1, val);

    return val;
}
#endif /* #if (UT_PLAT_LAYOUT_RAM == 1) */


#if (UT_PLAT_LAYOUT_TCM == 1)
ATTR_TEXT_IN_TCM int plat_fun_in_tcm(int v1)
{
    int a = 30;
    int b = 40;
    int val;

    g_tcm_zi = b;
    val = (a * g_tcm_zi * g_tcm_rw) + UT_PLAT_CONST_VAR + v1;

    printf("\t Oper: (%d*%d*%d)+%d+%d = %d \r\n", a, g_tcm_zi, g_tcm_rw, UT_PLAT_CONST_VAR, v1, val);

    return val;
}
#endif /* #if (UT_PLAT_LAYOUT_TCM == 1) */


ut_status_t ut_plat_layout(void)
{
    int ret = 1;
    int val = 0;

#if (UT_PLAT_LAYOUT_SYSRAM == 1) // SYSRAM Test
    //Sysram Code Test (VSYSRAM, Cached)
    printf("[SYSRAM]\r\n");
    val = (((long unsigned *)plat_fun_in_sysram >= &_sysram_code_start) && ((long unsigned *)plat_fun_in_sysram <= &_sysram_code_end));
    ret &= val;
    printf("\t Code: Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_sysram_code_start, (uint32_t)&_sysram_code_end, (uint32_t)plat_fun_in_sysram, val ? "Valid" : "Invalid");

    //Sysram Data Test
    val = (((long unsigned *)&g_sram_rw >= &_sysram_data_start) && ((long unsigned *)&g_sram_rw <= &_sysram_data_end));
    ret &= val;
    printf("\t Data: Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_sysram_data_start, (uint32_t)&_sysram_data_end, (uint32_t)&g_sram_rw, val ? "Valid" : "Invalid");

    //Sysram BSS Test
    val = (((long unsigned *)&g_sram_zi >= &_sysram_bss_start) && ((long unsigned *)&g_sram_zi <= &_sysram_bss_end));
    ret &= val;
    printf("\t BSS : Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_sysram_bss_start, (uint32_t)&_sysram_bss_end, (uint32_t)&g_sram_zi, val ? "Valid" : "Invalid");

    val = plat_fun_in_sysram(1);
    ret &= (val == 24011);
    printf("\t Funtion Test == %s\r\n", (val == 24011) ? "OK" : "Fail");


    //Sysram Code Test (SYSRAM, Non-Cached)
    printf("[SYSRAM](Non-Cached)\r\n");
    val = (((long unsigned *)plat_fun_in_noncached_sysram >= &_noncached_sysram_code_start) && ((long unsigned *)plat_fun_in_noncached_sysram <= &_noncached_sysram_code_end));
    ret &= val;
    printf("\t Code: Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_noncached_sysram_code_start, (uint32_t)&_noncached_sysram_code_end, (uint32_t)plat_fun_in_noncached_sysram, val ? "Valid" : "Invalid");

    //Sysram Data Test
    val = (((long unsigned *)&g_noncached_sram_rw >= &_noncached_sysram_data_start) && ((long unsigned *)&g_noncached_sram_rw <= &_noncached_sysram_data_end));
    ret &= val;
    printf("\t Data: Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_noncached_sysram_data_start, (uint32_t)&_noncached_sysram_data_end, (uint32_t)&g_noncached_sram_rw, val ? "Valid" : "Invalid");

    //Sysram BSS Test
    val = (((long unsigned *)&g_noncached_sram_zi >= &_noncached_sysram_bss_start) && ((long unsigned *)&g_noncached_sram_zi <= &_noncached_sysram_bss_end));
    ret &= val;
    printf("\t BSS : Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_noncached_sysram_bss_start, (uint32_t)&_noncached_sysram_bss_end, (uint32_t)&g_noncached_sram_zi, val ? "Valid" : "Invalid");

    val = plat_fun_in_noncached_sysram(1);
    ret &= (val == 24011);
    printf("\t Funtion Test == %s\r\n", (val == 24011) ? "OK" : "Fail");
#endif /* #if (UT_PLAT_LAYOUT_SYSRAM == 1) // SYSRAM Test */


#if (UT_PLAT_LAYOUT_RAM == 1) // RAM Test
    //RAM Code Test (VRAM, Cached)
    printf("[RAM]\r\n");
    val = (((long unsigned *)plat_fun_in_ram >= &_ram_code_start) && ((long unsigned *)plat_fun_in_ram <= &_ram_code_end));
    ret &= val;
    printf("\t Code: Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_ram_code_start, (uint32_t)&_ram_code_end, (uint32_t)plat_fun_in_ram, val ? "Valid" : "Invalid");

    //RAM Data Test
    val = (((long unsigned *)&g_ram_rw >= &_ram_data_start) && ((long unsigned *)&g_ram_rw <= &_ram_data_end));
    ret &= val;
    printf("\t Data: Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_ram_data_start, (uint32_t)&_ram_data_end, (uint32_t)&g_ram_rw, val ? "Valid" : "Invalid");

    //RAM BSS Test
    val = (((long unsigned *)&g_ram_zi >= &_ram_bss_start) && ((long unsigned *)&g_ram_zi <= &_ram_bss_end));
    ret &= val;
    printf("\t BSS : Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_ram_bss_start, (uint32_t)&_ram_bss_end, (uint32_t)&g_ram_zi, val ? "Valid" : "Invalid");

    val = plat_fun_in_ram(1);
    ret &= (val == 24011);
    printf("\t Funtion Test == %s\r\n", (val == 24011) ? "OK" : "Fail");


    //RAM Code Test (RAM, Non-Cached)
    printf("[RAM](Non-Cached)\r\n");
    val = (((long unsigned *)plat_fun_in_noncached_ram >= &_noncached_ram_code_start) && ((long unsigned *)plat_fun_in_noncached_ram <= &_noncached_ram_code_end));
    ret &= val;
    printf("\t Code: Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_noncached_ram_code_start, (uint32_t)&_noncached_ram_code_end, (uint32_t)plat_fun_in_noncached_ram, val ? "Valid" : "Invalid");

    //RAM Data Test
    val = (((long unsigned *)&g_noncached_ram_rw >= &_noncached_ram_data_start) && ((long unsigned *)&g_noncached_ram_rw <= &_noncached_ram_data_end));
    ret &= val;
    printf("\t Data: Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_noncached_ram_data_start, (uint32_t)&_noncached_ram_data_end, (uint32_t)&g_noncached_ram_rw, val ? "Valid" : "Invalid");

    //RAM BSS Test
    val = (((long unsigned *)&g_noncached_ram_zi >= &_noncached_ram_bss_start) && ((long unsigned *)&g_noncached_ram_zi <= &_noncached_ram_bss_end));
    ret &= val;
    printf("\t BSS : Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_noncached_ram_bss_start, (uint32_t)&_noncached_ram_bss_end, (uint32_t)&g_noncached_ram_zi, val ? "Valid" : "Invalid");

    val = plat_fun_in_noncached_ram(1);
    ret &= (val == 24011);
    printf("\t Funtion Test == %s\r\n", (val == 24011) ? "OK" : "Fail");
#endif /* #if (UT_PLAT_LAYOUT_RAM == 1) // RAM Test */


#if (UT_PLAT_LAYOUT_TCM == 1) // TCM Test
    //TCM Code Test
    printf("[TCM]\r\n");
    val = (((long unsigned *)plat_fun_in_tcm >= &_tcm_code_start) && ((long unsigned *)plat_fun_in_tcm <= &_tcm_code_end));
    ret &= val;
    printf("\t Code: Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_tcm_code_start, (uint32_t)&_tcm_code_end, (uint32_t)plat_fun_in_tcm, val ? "Valid" : "Invalid");

    //TCM Data Test
    val = (((long unsigned *)&g_tcm_rw >= &_tcm_data_start) && ((long unsigned *)&g_tcm_rw <= &_tcm_data_end));
    ret &= val;
    printf("\t Data: Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_tcm_data_start, (uint32_t)&_tcm_data_end, (uint32_t)&g_tcm_rw, val ? "Valid" : "Invalid");

    //TCM BSS Test
    val = (((long unsigned *)&g_tcm_zi >= &_tcm_bss_start) && ((long unsigned *)&g_tcm_zi <= &_tcm_bss_end));
    ret &= val;
    printf("\t BSS : Range(0x%lx - 0x%lx), Test(0x%lx) - %s\r\n", (uint32_t)&_tcm_bss_start, (uint32_t)&_tcm_bss_end, (uint32_t)&g_tcm_zi, val ? "Valid" : "Invalid");

    val = plat_fun_in_tcm(1);
    ret &= (val == 24011);
    printf("\t Funtion Test == %s\r\n", (val == 24011) ? "OK" : "Fail");
#endif /* #if (UT_PLAT_LAYOUT_TCM == 1) // TCM Test */

    return ret ? UT_STATUS_OK : UT_STATUS_ERROR;
}

#endif /* #if defined(UT_PLATFORM_ENABLE) && defined (UT_PLATFORM_LAYOUT_ENABLE) */
