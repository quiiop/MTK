/*
 * MediaTek Inc. (C) 2018. All rights reserved.
 *
 * Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
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

#include "va_process.h"
#include "ww_process.h"
#include "virt_state.h"
#include "mtk_heap.h"

#include "DSpotterSDKApi.h"
#include "DSpotterSDKApi_Const.h"
#include "CybModelInfor.h"
#include "CybDSpotter.h"
#include "DSpotterProprietary.h"

#define DSPOTTER_MEM_SIZE        50000  // Maxinum size of Cyberon Alog used

static void *cyberon_module_addr;

void *get_cyberon_module_addr(void)
{
    return cyberon_module_addr;
}

void set_cyberon_model_addr(void *addr)
{
    cyberon_module_addr =  addr;
}

static BYTE g_byaCybModelMem[CYBMODEL_GET_MEM_USAGE()];
static int trigger_num;
static int command_num;

static char *cyberon_license_addr;

char *get_cyberon_license_addr(void)
{
	return cyberon_license_addr;
}

void set_cyberon_license_addr(char *addr)
{
	cyberon_license_addr = addr;
}

/***************************************************************
                                                    Macro
***************************************************************/
#define COMMAND_RECOGNIZE_TIME                 6000
#define COMMAND_RECOGNIZE_TIME_MAX         8000
#define MAX_COMMAND_TIME                           500 /*unit 10ms*/

#define DSPOTTER_DETECT_TRIGGER_OK                 3
#define DSPOTTER_DETECT_COMMAND_OK              4
#define DSPOTTER_DETECT_RECOGINIZE_FAIL     5
#define DSPOTTER_DETECT_TIMEOUT_FAIL     6

/***************************************************************
                                                    Globle
***************************************************************/
enum DSPOTTER_STATE {
    DSPOTTER_STATE_TRIGGER,
    DSPOTTER_STATE_COMMAND,
    DSPOTTER_STATE_NUM,
};

enum DSPOTTER_EVENT {
    DSPOTTER_EVT_NEED_MORE_SAMPLE,
    DSPOTTER_EVT_TRIGGER_OK,
    DSPOTTER_EVT_COMMAND_OK,
    DSPOTTER_EVT_COMMAND_TIMEOUT,
    DSPOTTER_EVT_UNKNOWN_OK,
    DSPOTTER_EVT_UNKNOWN_FAIL,
    DSPOTTER_EVT_NUM,
};

struct dspotter_state {
    struct virt_state *cur_state;
    struct virt_state state_list[DSPOTTER_STATE_NUM];
};

HANDLE dspotter = NULL;
HANDLE hCybModel = NULL;
void *dspotter_mem_addr = NULL;
int g_dspotter_inited = 0;
static struct dspotter_state g_dspotter_state;
int proc_frames = 0;
int proc_rate = 0;
int proc_timeout = 0;

struct local_cmd_result g_result;
struct ww_private *g_priv;


/***************************************************************
                                                State Control
***************************************************************/
int dspotter_state_init(void)
{
    int i;

    for (i = 0; i < DSPOTTER_STATE_NUM; i++) {
        g_dspotter_state.state_list[i].state = i;
        g_dspotter_state.state_list[i].ops = NULL;
    }
    g_dspotter_state.cur_state = &(g_dspotter_state.state_list[DSPOTTER_STATE_TRIGGER]);

    return 0;
}

int dspotter_state_register_ops(int state, struct virt_state_ops *ops)
{
    if (state < 0 || state >= DSPOTTER_STATE_NUM)
        return -1;

    g_dspotter_state.state_list[state].ops = ops;

    return 0;
}

int dspotter_state_get(void)
{
    int state;

    if (g_dspotter_state.cur_state == NULL)
        state = -1;
    else
        state = g_dspotter_state.cur_state->state;
    return state;
}

int dspotter_state_switch(int next_state)
{
    struct virt_state *current = g_dspotter_state.cur_state;
    struct virt_state *next;
    int ret = 0;

    if (next_state < 0 || next_state >= DSPOTTER_STATE_NUM)
        return -1;

    next = &(g_dspotter_state.state_list[next_state]);

    if (current->ops->exit)
        current->ops->exit();

    g_dspotter_state.cur_state = next;

    if (next->ops->enter) {
        ret = next->ops->enter();
        if (ret != 0)
            return ret;
    }

    return 0;
}

int dspotter_state_event_proc(int event)
{
    int ret = 0;

    if (event < 0 || event > DSPOTTER_EVT_NUM)
        return -1;

    ret = g_dspotter_state.cur_state->ops->event_proc(event);

    return ret;
}

/***************************************************************
                                             Recognition Function
***************************************************************/

static void release_recognition(void)
{
    if (g_dspotter_inited == 0) {
        PRINTF_E("Warning: dspotter already released.\n");
        return;
    }

    if (dspotter != NULL) {
        DSpotter_Release(dspotter);
        dspotter = NULL;
    }

    MTK_vPortFree(dspotter_mem_addr);
    g_dspotter_inited = 0;
}

static int init_recognition(void)
{
    int ret = DSPOTTER_SUCCESS;
    int dspotter_mem_size = 0;
    int nModelCount;
    int nGroupCount;

    BYTE *p_models[2];

    if (g_dspotter_inited != 0) {
        PRINTF_E("Warning: dspotter already inited.\n");
        return -1;
    }

    DSpotterControl *control = (DSpotterControl *)cyberon_license_addr;
    hCybModel = CybModelInit((const BYTE *)cyberon_module_addr,
                             g_byaCybModelMem,
                             sizeof(g_byaCybModelMem),
                             &ret);
    if (ret)
        goto exit0;

    nGroupCount = CybModelGetGroupCount(hCybModel);
    nModelCount = 1;
    p_models[0] = (BYTE*)CybModelGetGroup(hCybModel, 0);
    // Maybe get more than 2 groups, but for IOT we only need 2 groups
    if (nGroupCount > 1)
    {
        nModelCount = 2;
        p_models[1] = (BYTE*)CybModelGetGroup(hCybModel, 1);
    }



    trigger_num = DSpotter_GetNumWord(p_models[0]);
    if (nModelCount == 2)
        command_num = DSpotter_GetNumWord(p_models[1]);
    else
        command_num = 0;

    dspotter_mem_size =  DSpotter_GetMemoryUsage_Multi((BYTE *)CybModelGetBase(hCybModel),
                                                       p_models,
                                                       nModelCount,
                                                       MAX_COMMAND_TIME);
    PRINTF_I("Needed working memory size = %d , reserved non-cache psram : %d \n", dspotter_mem_size, MTK_xPortGetFreeHeapSize(get_adsp_heap_type(ADSP_MEM_NORMAL_CACHE)));
    if (MTK_xPortGetFreeHeapSize(get_adsp_heap_type(ADSP_MEM_NORMAL_CACHE)) < DSPOTTER_MEM_SIZE ){
        PRINTF_E("Non-Cache Psram size is too low! \n " );
    }
    dspotter_mem_addr = MTK_pvPortMalloc(dspotter_mem_size, get_adsp_heap_type(ADSP_MEM_NORMAL_CACHE));
    if (!dspotter_mem_addr) {
        ret = -1;
        PRINTF_E("%s malloc error, ret = %d\n", __func__, ret);
        goto exit1;
    }
    if ((BYTE *)cyberon_license_addr == NULL) {
        PRINTF_E("Warning: cyberon_license_addr null.\n");
	ret = -1;
        goto exit2;
    }
    dspotter = DSpotter_Init_Multi((BYTE *)CybModelGetBase(hCybModel),
                                   p_models,
                                   nModelCount,
                                   control->nMaxCommandTime/10,  //unit 10ms
                                   dspotter_mem_addr,
                                   dspotter_mem_size,
                                   (BYTE *)cyberon_license_addr,
                                   256,
                                   &ret);
    if (dspotter == NULL) {
        PRINTF_E("Warning: dspotter init fail (%d).\n", ret);
        goto exit2;
    } else {
        DSpotterAGC_Enable(dspotter);
        DSpotterAGC_SetMaxGain(dspotter,(float)control->byAGC_Gain);
        EnableAllCommandWords(FALSE);
        SetTimeoutBoundary(control->nMaxCommandTime);
        PRINTF_I("\r\nDSpotter init done.\r\n");
        PRINTF_I("Group count : %d  \n", nModelCount  );
        PRINTF_I("GAIN : %d, Continuous command : %d  ,command Timeout : %d (10ms uint)   \n",control->byAGC_Gain, control->nCommandStageFlowControl , GetTimeoutBoundary());
    }


    g_dspotter_inited = 1;


    return ret;
exit2:
    MTK_vPortFree(dspotter_mem_addr);

exit1:
    CybModelRelease(hCybModel);

exit0:
    return ret;
}

/***************************************************************
                                                  States
***************************************************************/

static int dspotter_state_trigger_enter(void)
{
    PRINTF_D("Trigger state enter.\n");
    return 0;
}

static int dspotter_state_trigger_exit(void)
{
    PRINTF_D("Trigger state exit.\n");
    return 0;
}

static int dspotter_state_trigger_event_proc(int event)
{
    int next_state = DSPOTTER_STATE_TRIGGER;
    int ret = 0;

    switch(event) {
    case DSPOTTER_EVT_TRIGGER_OK:
        next_state = DSPOTTER_STATE_COMMAND;
        g_priv->detect_result = DSPOTTER_DETECT_TRIGGER_OK;
        break;
    default:
        break;
    }

    if (next_state != DSPOTTER_STATE_TRIGGER) {
        ret = dspotter_state_switch(next_state);
        PRINTF_D("Trigger state: event (%d), next_state (%d).\n", event, next_state);
    }
    return ret;
}

static int dspotter_state_command_enter(void)
{
    PRINTF_D("Command state enter.\n");
    return 0;
}

static int dspotter_state_command_exit(void)
{
    PRINTF_D("Command state exit.\n");
    return 0;
}

static int dspotter_state_command_timeout_handle(int event)
{
    static int cmd_record_sample = 0;
    DSpotterControl *control = (DSpotterControl *)cyberon_license_addr;

    if (event != DSPOTTER_EVT_NEED_MORE_SAMPLE) {
        //recoginze command during default timeout interval
        if (event != DSPOTTER_EVT_COMMAND_OK){
            proc_timeout = 0;
            cmd_record_sample = 0;
            return 0;
         }

    }

    cmd_record_sample += proc_frames;
     // Recoginze command during extend 2s,  we return to the time point which is 2s prior to Timeout
    if(GetTimeoutBoundary() > control->nMaxCommandTime){
        if (event == DSPOTTER_EVT_COMMAND_OK){
                cmd_record_sample = (control->nMaxCommandTime - 2000) > 0
		                          ? (control->nMaxCommandTime - 2000) * (proc_rate/1000)
		                          : 0;
                SetTimeoutBoundary(control->nMaxCommandTime);
                return 0;
         }
    }
    if (cmd_record_sample > proc_rate / 1000 * GetTimeoutBoundary()) {
        if (DSpotter_IsKeywordAlive(dspotter)) {
            //NEED_MORE_SAMPLE
            SetTimeoutBoundary(cmd_record_sample/(proc_rate/1000) + EXTENDUNIT);
            return 0;
        }
        else{
            //At least extend 2s, if we got keyword when Timeout
            if ( GetTimeoutBoundary() != control->nMaxCommandTime && GetTimeoutBoundary() <  control->nMaxCommandTime + 2000 ){
                SetTimeoutBoundary(cmd_record_sample/(proc_rate/1000) + EXTENDUNIT);
                return 0;
            }
        }
        PRINTF_I("DSpotter recognize timeout! \n");
        EnableAllCommandWords(FALSE);
        SetTimeoutBoundary(control->nMaxCommandTime);
        proc_timeout = 1;
        cmd_record_sample = 0;
        return -1;
    }

    return 0;
}

static int dspotter_state_command_event_proc(int event)
{
    int next_state = DSPOTTER_STATE_COMMAND;
    int ret = 0;

    if (dspotter_state_command_timeout_handle(event) != 0)
        event = DSPOTTER_EVT_COMMAND_TIMEOUT;

    switch(event) {
    case DSPOTTER_EVT_TRIGGER_OK:
        next_state = DSPOTTER_STATE_COMMAND;
        g_priv->detect_result = DSPOTTER_DETECT_TRIGGER_OK;
        break;
    case DSPOTTER_EVT_COMMAND_OK:
        next_state = DSPOTTER_STATE_COMMAND;
        g_priv->detect_result = DSPOTTER_DETECT_COMMAND_OK;
        break;
    case DSPOTTER_EVT_UNKNOWN_OK:
    case DSPOTTER_EVT_UNKNOWN_FAIL:
        next_state = DSPOTTER_STATE_TRIGGER;
        g_priv->detect_result = DSPOTTER_DETECT_RECOGINIZE_FAIL;
        break;
    case DSPOTTER_EVT_COMMAND_TIMEOUT:
        //for timeout case
        g_result.cmd_state = -1;
        next_state = DSPOTTER_STATE_TRIGGER;
        g_priv->detect_result = DSPOTTER_DETECT_TIMEOUT_FAIL;
        break;
    default:  //Need more sample
        break;
    }

    if (next_state != DSPOTTER_STATE_COMMAND) {
        ret = dspotter_state_switch(next_state);
        PRINTF_D("Command state: event (%d), next_state (%d).\n", event, next_state);
    }
    return ret;
}

struct virt_state_ops g_dspotter_trigger_ops = {
    .enter = dspotter_state_trigger_enter,
    .exit = dspotter_state_trigger_exit,
    .event_proc = dspotter_state_trigger_event_proc,
};

struct virt_state_ops g_dspotter_command_ops = {
    .enter = dspotter_state_command_enter,
    .exit = dspotter_state_command_exit,
    .event_proc = dspotter_state_command_event_proc,
};

/***************************************************************
                                                   Adaptor
***************************************************************/
static int dspotter_adaptor_init(struct va_proc_obj *obj, struct va_pcm_format in,
    struct va_pcm_format *out, int frames)
{
    struct ww_private *priv = (struct ww_private *)(obj->priv);
    struct ww_config *config = (struct ww_config *)(&(priv->config));
	PRINTF_E("dspotter adaptor init 1.\n");

    proc_rate = config->rate;
    g_priv = (struct ww_private *)(obj->priv);
    if (init_recognition() != DSPOTTER_SUCCESS) {
        PRINTF_E("dspotter adaptor init fail.\n");
        return -1;
    }
	PRINTF_E("dspotter adaptor init 2.\n");

    dspotter_state_init();
    dspotter_state_register_ops(DSPOTTER_STATE_TRIGGER, &g_dspotter_trigger_ops);
    dspotter_state_register_ops(DSPOTTER_STATE_COMMAND, &g_dspotter_command_ops);

    return 0;
}

static int dspotter_adaptor_uninit(struct va_proc_obj *obj)
{
    release_recognition();
    return 0;
}

static int dspotter_adaptor_reset(struct va_proc_obj *obj)
{
    DSpotter_Reset(dspotter);

    return 0;
}

static int dspotter_adaptor_get_params(struct va_proc_obj *obj, int cmd, void *data)
{
    struct local_cmd_result *result = (struct local_cmd_result *)data;

    switch(cmd) {
    case CMD_GET_DSPOTTER_RESULT:
        result->state = dspotter_state_get();
        result->cmd_state = g_result.cmd_state;
        result->cmd_idx = g_result.cmd_idx;
        result->cmd_score = g_result.cmd_score;
        result->cmd_mapID = g_result.cmd_mapID;
        break;
    default:
        break;
    }

    return 0;
}

static int dspotter_adaptor_process(struct va_proc_obj *obj, char *inbuf, char *outbuf, int frames)
{
    int ret = DSPOTTER_SUCCESS;
    char szCommand[64];
    int nMapID;
    DSpotterControl *control = (DSpotterControl *)cyberon_license_addr;

    // only support 16bit mono
    ret = DSpotter_AddSample(dspotter, (SHORT *)inbuf, frames);
    proc_frames = frames;

    int event = 0;
    const char *word = "Unknown";

    if (ret == DSPOTTER_ERR_NeedMoreSample) {

        event = DSPOTTER_EVT_NEED_MORE_SAMPLE;
    } else if (ret == DSPOTTER_SUCCESS) {
        int cmd_idx, cmd_score, cmd_sg, cmd_eg;
        int result_idx = 0;
        // There are two models here, the id will enumerated in order.
        cmd_idx = DSpotter_GetResult(dspotter);
        // Score: similar to command model. SG: different from silence/garbage. Fil: different from filter model.
        DSpotter_GetResultScore(dspotter, &cmd_score, &cmd_sg, NULL);
        // result in RMS
        cmd_eg = DSpotter_GetCmdEnergy(dspotter);
        if (cmd_idx < trigger_num) {
            /* It's a trigger word */
            result_idx = cmd_idx;
            CybModelGetCommandInfo(hCybModel, 0, cmd_idx, szCommand, sizeof(szCommand), &nMapID, NULL);
            event = DSPOTTER_EVT_TRIGGER_OK;
            g_result.cmd_state = DSPOTTER_STATE_TRIGGER;
            EnableAllCommandWords(TRUE);
        } else if (cmd_idx >= trigger_num && cmd_idx < (trigger_num + command_num)){
            /* It's a command word */
            result_idx = cmd_idx - trigger_num;
            CybModelGetCommandInfo(hCybModel, 1, cmd_idx, szCommand, sizeof(szCommand), &nMapID, NULL);
            event = DSPOTTER_EVT_COMMAND_OK;
            g_result.cmd_state = DSPOTTER_STATE_COMMAND;
            //If we do not support continuous command mode, just disable command words
            if (control->nCommandStageFlowControl == 0){
                EnableAllCommandWords(FALSE);
                event = DSPOTTER_EVT_TRIGGER_OK;
            }

        } else {
            /* It's a unknown word */
            word = "Unknown";
            event = DSPOTTER_EVT_UNKNOWN_OK;
            g_result.cmd_state = DSPOTTER_STATE_TRIGGER;
            EnableAllCommandWords(FALSE);
        }
        PRINTF_I("Get trigger/command :%s(%d/[%d+%d]), Score = %d, SG = %d, Energy = %d nMapID : %d, result_idx: %d .\n",
            word, cmd_idx, trigger_num, command_num, cmd_score, cmd_sg, cmd_eg, nMapID, result_idx);

        g_result.cmd_idx = result_idx;
        g_result.cmd_score = cmd_score;
	g_result.cmd_mapID = nMapID;

        // Support recognizing command words without 0.3s internal after trigger word
        DSpotter_Continue(dspotter);
    } else {
        PRINTF_I("DSpotter_AddSample() un-handle result %d\n", ret);
        event = DSPOTTER_EVT_UNKNOWN_FAIL;
    }

    dspotter_state_event_proc(event);

    return 0;
}

void cyberon_dspotter_adaptor_register(struct algo_ops *ops)
{
    ops->init = dspotter_adaptor_init;
    ops->uninit = dspotter_adaptor_uninit;
    ops->set_params = NULL;
    ops->get_params = dspotter_adaptor_get_params;
    ops->reset = dspotter_adaptor_reset;
    ops->process = dspotter_adaptor_process;
}

void cyberon_dspotter_adaptor_unregister(struct algo_ops *ops)
{
    ops->init = NULL;
    ops->uninit = NULL;
    ops->set_params = NULL;
    ops->get_params = NULL;
    ops->reset = NULL;
    ops->process = NULL;
}
