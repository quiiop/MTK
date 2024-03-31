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

#ifndef _AU_I2S_SFR_H_
#define _AU_I2S_SFR_H_
#include "audio_types.h"


/******************************************************************************
 * Enumerations
 ******************************************************************************/
typedef enum AU_I2S_MODE_enum
{
    reserved_i2s,
    // I2S_TX_MODE     = 0,
    // I2S_RX_MODE     = 1,
    // I2S_TRX_MODE    = 2,
    // I2S_DISABLE     = 3,

}AU_I2S_MODE;



/******************************************************************************
 * I2S
 * Base Address = 0x42840000
 ******************************************************************************/

 /* offset 0x00 */
typedef union I2S_CTL0_u
{
    struct I2S_CTL0_s
    {
        U32 EN_I2S0_MOD	        : 1;
        U32 _RSVD_0_         	: 1;
        U32 EN_I2S1_MOD	        : 1;
        U32 _RSVD_1_         	: 11;
        U32 DBG_PORT_SEL        : 2;
        U32 _RSVD_2_       		: 16;
    } field;
    U32 reg;
} I2S_CTL0_t;

 /* offset 0x04 */
typedef union I2S_CTL1_u
{
    struct I2S_CTL1_s
    {
        U32 RST_I2S0_MOD	    : 1;
        U32 _RSVD_0_         	: 1;
        U32 RST_I2S1_MOD	    : 1;
        U32 _RSVD_1_         	: 29;
    } field;
    U32 reg;
} I2S_CTL1_t;

 /* offset 0x08 */
typedef union I2S_CTL2_u
{
    struct I2S_CTL2_s
    {
        U32 MUTE_I2S0_DATA	    : 1;
        U32 _RSVD_0_         	: 1;
        U32 MUTE_I2S1_DATA	    : 1;
        U32 _RSVD_1_         	: 29;
    } field;
    U32 reg;
} I2S_CTL2_t;

 /* offset 0x0C */
typedef union I2S0_SET_u
{
    struct I2S0_SET_s
    {
        U32         EX_TX_LR_DATA		: 1;
        U32         EX_RX_LR_DATA       : 1;
        U32         I2S_BCLK_24B_MODE 	: 1;
        U32         SET_I2S_M_FS 	    : 1;
        U32         RX_SDI_LATCH_PHASE 	: 1;
        U32         _RSVD_0_       		: 1;
		U32         I2S_AUTX_WORD_LEN   : 2;
		U32         AURX_I2S_WORD_LEN   : 2;
		U32         I2S_DATA_WORD_LEN   : 2;
		U32         I2S_BIT_FORMAT      : 2;
		AU_I2S_MODE I2S_TR_MODE_CTL     : 2;
		U32         _RSVD_1_       		: 16;
    } field;
    U32 reg;
} I2S0_SET_t;

 /* offset 0x10 */
typedef union I2S1_SET_u
{
    struct I2S1_SET_s
    {
        U32         EX_TX_LR_DATA		: 1;
        U32         EX_RX_LR_DATA       : 1;
        U32         I2S_SDI_TIME 	    : 1;
        U32         I2S_SDO_TIME 	    : 1;
        U32         _RSVD_0_       		: 2;
		U32         I2S_AUTX_WORD_LEN   : 2;
		U32         AURX_I2S_WORD_LEN   : 2;
		U32         I2S_DATA_WORD_LEN   : 2;
		U32         I2S_BIT_FORMAT      : 2;
		AU_I2S_MODE I2S_TR_MODE_CTL     : 2;
		U32 _RSVD_1_       		: 16;
    } field;
    U32 reg;
} I2S1_SET_t;

typedef struct
{
	I2S_CTL0_t				        CTL0;		   	//offset 0x00
	I2S_CTL1_t					    CTL1;			//offset 0x04
	I2S_CTL2_t					    CTL2;			//offset 0x08
	I2S0_SET_t					    SET0;	        //offset 0x0C
	I2S1_SET_t					    SET1;	        //offset 0x10
} I2S_s;

extern volatile I2S_s I2S;

#endif
