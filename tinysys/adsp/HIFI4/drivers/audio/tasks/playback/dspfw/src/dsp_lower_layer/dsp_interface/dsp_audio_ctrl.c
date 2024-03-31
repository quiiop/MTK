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


//#include "config.h"
#include "audio_types.h"
#include "dsp_audio_ctrl.h"
#include "dsp_drv_afe.h"
#include "dsp_gain_control.h"
#include "sink.h"
#include "source.h"
#include "dsp_temp.h"
#ifdef MTK_ANC_ENABLE
#include "anc_api.h"
#endif
#include "stream_n9sco.h"


/**
 *
 *  Definition
 *
 */


/**
 *
 *  Type Definition
 *
 */
typedef void (*AUDIO_CONTROL_ENTRY)(void);

typedef struct AUDIO_CONTROL_STRU_s
{
    AUDIO_CONTROL_ENTRY Entry;
} AUDIO_CONTROL_STRU_t;


/**
 *
 *  Buffer & Control
 *
 */
DSP_AUDIO_CTRL_t gAudioCtrl;

VOID DSP_CTRL_Initialization (VOID);
VOID DSP_CTRL_Deinitialization (VOID);
VOID DSP_CTRL_ChangeStatus (AU_DSP_STATUS_LIST_t Component, AU_DSP_STATUS_CH_t Stat);
VOID DSP_CTRL_AudioHandleProcess (AU_DSP_STATUS_LIST_t Component, AU_DSP_STATUS_CH_t Stat);
VOID DSP_CTRL_AudioOffProcedure_DspInput(VOID);
VOID DSP_CTRL_AudioOnProcedure_DspInput(VOID);
VOID DSP_CTRL_AudioOffProcedure_DspOutput(VOID);
VOID DSP_CTRL_AudioOnProcedure_DspOutput(VOID);
VOID DSP_CTRL_AudioOffProcedure(VOID);
VOID DSP_CTRL_AudioOnProcedure(VOID);
VOID DSP_CTRL_DummyOff(VOID);
VOID DSP_CTRL_DummyOn(VOID);

static CODE AUDIO_CONTROL_ENTRY pAudioOnCtrlEntry[] =
{
    DSP_CTRL_AudioOffProcedure_DspInput,
    DSP_CTRL_DummyOn,
    DSP_CTRL_DummyOn,
    DSP_CTRL_AudioOnProcedure_DspOutput,
    DSP_CTRL_DummyOn,
    DSP_CTRL_DummyOn,
    DSP_CTRL_DummyOn,
    DSP_CTRL_AudioOnProcedure,
};


static CODE AUDIO_CONTROL_ENTRY pAudioOffCtrlEntry[] =
{
    DSP_CTRL_AudioOffProcedure_DspInput,
    DSP_CTRL_DummyOff,
    DSP_CTRL_DummyOff,
    DSP_CTRL_AudioOffProcedure_DspOutput,
    DSP_CTRL_DummyOff,
    DSP_CTRL_DummyOff,
    DSP_CTRL_DummyOff,
    DSP_CTRL_AudioOffProcedure,
};


/**
 * DSP_CTRL_Initialization
 *
 * This function is used to initialize control status of all audio components
 */
VOID DSP_CTRL_Initialization (VOID)
{
    DSP_CTRL_Deinitialization();
#ifdef CFG_AUDIO_HARDWARE_ENABLE
#ifdef CFG_DSP_GAIN_CONTROL_ENABLE
    DSP_GC_Init();                  //[ToDo] gain control
#endif
    DSP_DRV_AFE_Init();
#endif
#ifdef MTK_ANC_ENABLE
    dsp_anc_init();
#endif
#ifdef FORWARDER_DEBUG
    Forwarder_IRQ_init(true);
#endif
}


/**
 * DSP_CTRL_Deinitialization
 *
 * This function is used to de-initialize control status of all audio components
 */
VOID DSP_CTRL_Deinitialization(VOID)
{
    U32 Idx;
    for (Idx = 0 ; Idx <= AU_STATUS_ALL ; Idx++)
    {
    	if (gAudioCtrl.Status[Idx] == AU_DSP_STATUS_ON)
    	{
    		DSP_CTRL_ChangeStatus(Idx, AU_DSP_STATUS_OFF);
    	}
    	gAudioCtrl.Status[Idx] = AU_DSP_STATUS_OFF;
    }
}


/**
 * DSP_CTRL_ChangeStatus
 *
 * This function is used to change control status of specific component,
 * which should always be accompanied by operation of audio source/sink
 *
 * @Stat : Status to be set
 *
 */
VOID DSP_CTRL_ChangeStatus(AU_DSP_STATUS_LIST_t Component, AU_DSP_STATUS_CH_t Stat)
{
    U32 Idx = 0;
    U32 ComponentExist = 0;

    UNUSED(ComponentExist);

    if (Stat == AU_DSP_STATUS_ON)
    {
        DSP_CTRL_AudioHandleProcess(Component, AU_DSP_STATUS_ON);

        if (Component < AU_STATUS_INPUT)
        {
            if (gAudioCtrl.Status[AU_STATUS_INPUT] == AU_DSP_STATUS_OFF)
            {
                DSP_CTRL_AudioHandleProcess(AU_STATUS_INPUT, AU_DSP_STATUS_ON);
            }
        }
        else if (Component < AU_STATUS_OUTPUT)
        {
            if (gAudioCtrl.Status[AU_STATUS_OUTPUT] == AU_DSP_STATUS_OFF)
            {
                DSP_CTRL_AudioHandleProcess(AU_STATUS_OUTPUT, AU_DSP_STATUS_ON);
            }
        }

        if (gAudioCtrl.Status[AU_STATUS_ALL] == AU_DSP_STATUS_OFF)
        {
            DSP_CTRL_AudioHandleProcess(AU_STATUS_ALL, AU_DSP_STATUS_ON);
        }
    }
    else
    {
        DSP_CTRL_AudioHandleProcess(Component, AU_DSP_STATUS_OFF);

        if (Component < AU_STATUS_INPUT)
        {
            for (Idx = 0 ; Idx < AU_STATUS_INPUT ; Idx++)
            {
                if (gAudioCtrl.Status[Idx] != AU_DSP_STATUS_OFF)
                {
                    return;
                }
            }
            DSP_CTRL_AudioHandleProcess(AU_STATUS_INPUT, AU_DSP_STATUS_OFF);
        }
        else if (Component < AU_STATUS_OUTPUT)
        {
            for (Idx = (AU_STATUS_INPUT + 1) ; Idx < AU_STATUS_OUTPUT ; Idx++)
            {
                if (gAudioCtrl.Status[Idx] != AU_DSP_STATUS_OFF)
                {
                    return;
                }
            }
            DSP_CTRL_AudioHandleProcess(AU_STATUS_OUTPUT, AU_DSP_STATUS_OFF);
        }

        for (Idx = 0 ; Idx < AU_STATUS_ALL ; Idx++)
        {
            if (gAudioCtrl.Status[Idx] != AU_DSP_STATUS_OFF)
            {
                return;
            }
        }
        DSP_CTRL_AudioHandleProcess(AU_STATUS_ALL, AU_DSP_STATUS_OFF);
    }
}


/**
 * DSP_CTRL_AudioHandleProcess
 *
 * This function is used to control audio compenent and correspoding behavior
 */
VOID DSP_CTRL_AudioHandleProcess(AU_DSP_STATUS_LIST_t Component, AU_DSP_STATUS_CH_t Stat)
{
	  gAudioCtrl.Status[Component] = Stat;

    switch (Stat)
    {
    	case AU_DSP_STATUS_ON:
    		pAudioOnCtrlEntry[Component]();
    		break;

    	case AU_DSP_STATUS_OFF:
    		pAudioOffCtrlEntry[Component]();
    		break;

    	default:
    		break;
    }
}

VOID DSP_CTRL_AudioOffProcedure_DspInput(VOID)
{

}

VOID DSP_CTRL_AudioOnProcedure_DspInput(VOID)
{

}

VOID DSP_CTRL_AudioOffProcedure_DspOutput(VOID)
{

}

VOID DSP_CTRL_AudioOnProcedure_DspOutput(VOID)
{

}

VOID DSP_CTRL_AudioOffProcedure(VOID)
{

}

VOID DSP_CTRL_AudioOnProcedure(VOID)
{

}

VOID DSP_CTRL_DummyOff(VOID)
{

}

VOID DSP_CTRL_DummyOn(VOID)
{

}
