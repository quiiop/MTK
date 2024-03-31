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
 * MediaTek Inc. (C) 2022-2022. All rights reserved.
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


#include "bl_test.h"


#ifdef MTK_BL_TEST


#include "hw_uart.h"


typedef struct tristate
{
    char            *name;
    emu_tristate_t  *tri;
} tristate_t;

const char *tristate_name[] = { "NO_OP", "EMU_TRUE", "EMU_FALSE" };


/***************************************************************************
 * EMULATION CONTROL VARIABLES
 ***************************************************************************/


emu_tristate_t g_emu_tri_sbc_enable;
emu_tristate_t g_emu_tri_verify;
emu_tristate_t g_emu_tri_ram;
emu_tristate_t g_emu_tri_verified;
emu_tristate_t g_emu_tri_strict;


const tristate_t g_tristate[] = 
{
    { "sbc enable",     &g_emu_tri_sbc_enable },
    { "verify",         &g_emu_tri_verify },
    { "ram",            &g_emu_tri_ram },
    { "sec verified",   &g_emu_tri_verified },
    { "non-permissive", &g_emu_tri_strict },
};
#define g_tristate_count ( sizeof(g_tristate) / sizeof(tristate_t) )


/***************************************************************************
 * EMULATION
 ***************************************************************************/


void bl_test_dump( void )
{
    unsigned int i;
    for (i = 0; i < g_tristate_count; i++)
        hw_uart_printf("%s %s\n", g_tristate[ i ].name,
                                  tristate_name[ *g_tristate[ i ].tri ]);
}


void bl_tristate_toggle( unsigned int i )
{
    if (i >= g_tristate_count)
        return;
    
    *g_tristate[i].tri = ( *g_tristate[i].tri + 1 ) % 3;
    hw_uart_printf("%s %s\n", g_tristate[ i ].name,
                              tristate_name[ *g_tristate[i].tri ]);
}


void bl_test_cmd( char tp )
{
    unsigned int i = 0xFFFFFFFF;

    if ( tp == 'd' ) {
        bl_test_dump();
        return;
    }
    
    if ( tp >= '0' && tp <= '9' )
        i = tp - '0';
    else if ( tp >= 'A' && tp <= 'Z' )
        i = tp - 'A' + 10;
    
    bl_tristate_toggle( i );
}


#endif /* MTK_BL_TEST */
