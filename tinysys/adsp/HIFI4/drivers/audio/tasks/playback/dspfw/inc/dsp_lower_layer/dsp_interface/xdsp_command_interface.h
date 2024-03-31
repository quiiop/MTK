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

 
#ifndef _DSP_COMMAND_INTERFACE_H_
#define _DSP_COMMAND_INTERFACE_H_

#include "audio_types.h"
#include "transform.h"
#include "stream_sco.h"
#include "dsp_update_para.h"

/*
 *
 * DSP Command Messages Section
 *
 */
#define DSP_UPDATE_COMMAND_FEATURE_PARA_SEQUENCE_AUTODETECT  0xFFFFFFFF
#warning "Machi wait for confirm"
typedef enum
{
    /* DTM */
    DSP_MSG_TM_ENUM_BEGIN,
    DSP_AUDIO_BIST,
    DSP_TEST_MODE,
    DSP_MSG_VOL_SET,
    DSP_MSG_PARA_CHANGE,

    DSP_MSG_LM_ENUM_BEGIN,
    /*Sreaming Ctrl*/
    DSP_MSG_SCO_Tx_READY,
    DSP_MSG_SCO_Rx_READY,
    DSP_MSG_SCO_Tx_DISABLE,
    DSP_MSG_SCO_Rx_DISABLE,

    DSP_MSG_TM_ENUM_END,

    DSP_UPDATE_PARAMETER,

    /* DPRT Start */
    DSP_MSG_PRT_START_ENUM_BEGIN,
    DSP_MSG_START_RT,
    DSP_MSG_START_VP,
    DSP_MSG_PRT_START_ENUM_END,

    /* DPRT Stop */
    DSP_MSG_PRT_STOP_ENUM_BEGIN,
    DSP_MSG_STOP_PRT,
    DSP_MSG_PRT_STOP_ENUM_END,

    /* DPRT Ready */
    DSP_MSG_PRT_READY_TO_START,
    DSP_MSG_PRT_READY_TO_STOP,

    /* DAVT Start */
    DSP_MSG_AVT_START_ENUM_BEGIN,
    DSP_MSG_START_LINE,
    DSP_MSG_AVT_START_ENUM_END,

    /* DAVT Stop */
    DSP_MSG_AVT_STOP_ENUM_BEGIN,
    DSP_MSG_STOP_AVT,
    DSP_MSG_AVT_STOP_ENUM_END,

    /* DAVT Ready */
    DSP_MSG_AVT_READY_TO_START,
    DSP_MSG_AVT_READY_TO_STOP,
} DSP_CMD_MSG_t;




/*
 *
 * DSP Command Parameters Section
 *
 */

typedef struct VP_PARA_s
{
    U8 Para0;
} VP_PARA_t;

typedef struct RT_PARA_s
{
    U8 Para0;
} RT_PARA_t;

typedef struct LINE_PARA_s
{
    U8 Para0;
} LINE_PARA_t;


typedef union DSP_COMMAND_PARA_u
{
    VP_PARA_t           VpPara;
    RT_PARA_t           RtPara;
    LINE_PARA_t         LinePara;
    DSP_UPDATE_PARA     UpdatePara;
    SCO_SETUP_TX_PARA_t TxPara;
    SCO_SETUP_RX_PARA_t RxPara;
} DSP_CMD_PARA_t, *DSP_CMD_PARA_PTR_t;

/*
 *
 * DSP Command Structure
 *
 */

typedef struct DSP_COMMAND_s
{
    DSP_CMD_MSG_t           DspMsg;
    U8                      TxTaskID;
    U8                      TaskID;
    U8                      CmdSeqNo;
    DSP_CMD_PARA_t          DspCmdPara;
} DSP_COMMAND_t, *DSP_CMD_PTR_t;


#endif /* _DSP_COMMAND_INTERFACE_H_ */

