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



#ifndef __BL_TEST_H__
#define __BL_TEST_H__


//#define MTK_BL_TEST

#ifndef MTK_BL_TEST

    #define EMU_TRISTATE_RUN_IF_TRUE(...)   do{}while(0)
    #define EMU_TRISTATE_RETURN(...)        do{}while(0)
    #define EMU_TRISTATE_UPDATE(...)        do{}while(0)
    #define EMU_TRISTATE_BOOL(...)          do{}while(0)

#else

    typedef enum
    {
        NO_OP,
        EMU_TRUE,
        EMU_FALSE
    } emu_tristate_t;

    #define EMU_TRISTATE_RUN_IF_TRUE( _switch, _stmt ) \
        do { \
            if ( _switch == EMU_TRUE )  _stmt; \
        } while (0)

    #define EMU_TRISTATE_RETURN( _switch, _v0, _v1 ) \
        do { \
            if ( _switch == EMU_TRUE )  return _v0; \
            if ( _switch == EMU_FALSE ) return _v1; \
        } while (0)

    #define EMU_TRISTATE_UPDATE( _bool, _switch ) \
        do { \
            if ( _switch == EMU_TRUE )  (_bool) = true; \
            if ( _switch == EMU_FALSE ) (_bool) = false; \
        } while (0)

    #define EMU_TRISTATE_BOOL( _switch ) \
        do { \
            if ( _switch == EMU_TRUE )  return true; \
            if ( _switch == EMU_FALSE ) return false; \
        } while (0)


    extern emu_tristate_t g_emu_tri_sbc_enable;
    extern emu_tristate_t g_emu_tri_verify;
    extern emu_tristate_t g_emu_tri_ram;
    extern emu_tristate_t g_emu_tri_verified;
    extern emu_tristate_t g_emu_tri_strict;


    void bl_test_dump( void );


    void bl_test_toggle( int tp );


    void bl_test_cmd( char tp );

#endif /* MTK_BL_TEST */



#endif /* __BL_TEST_H__ */
