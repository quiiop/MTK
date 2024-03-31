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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

 
#ifndef _DSP_UTILITIES_H_
#define _DSP_UTILITIES_H_

#include "audio_types.h"
#include "FreeRTOS.h"



#ifndef DSP_ALIGN2
#define     DSP_ALIGN2      ALIGN(2)
#endif
#ifndef DSP_ALIGN4
#define     DSP_ALIGN4      ALIGN(4)
#endif
#ifndef DSP_ALIGN8
#define     DSP_ALIGN8      ALIGN(8)
#endif
/**
 * DSP_WrapSubtraction
 */
static inline U16 DSP_WrapSubtraction(U16 Minuend, U16 Subtrahend, U16 WrapSize)
{
    configASSERT((WrapSize >= Minuend) && (WrapSize >= Subtrahend));

    return (Minuend >= Subtrahend) ? (Minuend - Subtrahend) : (Minuend + WrapSize - Subtrahend);
}


/**
 * DSP_WrapSubtraction
 */
static inline U16 DSP_WrapAddition(U16 Augend, U16 Addend, U16 WrapSize)
{
    configASSERT((WrapSize >= Augend) && (WrapSize >= Addend));

    return (Augend + Addend >= WrapSize) ? (Augend + Addend - WrapSize) : (Augend + Addend);
}


/**
 * DSP_CheckBit
 *
 * Check Bit Field is Set in Variable
 *
 */
static inline BOOL DSP_CheckBit(U32 Var, U32 Pos)
{
    return ((Var & (1 << Pos)) != 0);
}


/**
 * DSP_ByteSwap16
 *
 * Endian swap of 16-bit value
 *
 */
static inline U16 DSP_ByteSwap16 (U16 value)
{
    U16 byte0 = value>>8;
    U16 byte1 = value<<8;

    return (byte0|byte1);
}


/**
 * DSP_ByteSwap32
 *
 * Endian swap of 32-bit value
 *
 */
static inline U32 DSP_ByteSwap32 (U32 value)
{
    value = ((value<<8) & 0xFF00FF00) | ((value>>8) & 0x00FF00FF);

    U32 word0   = value<<16;
    U32 word1   = (value>>16) & 0x0000FFFF;

    return (word0|word1);
}


#endif /* _DSP_UTILITIES_H_ */

