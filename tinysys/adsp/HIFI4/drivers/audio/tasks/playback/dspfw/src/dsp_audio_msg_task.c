/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2019. All rights reserved.
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
* have been modified by MediaTek Inc. All revisions are subject to any receiver\'s
* applicable license agreements with MediaTek Inc.
*/

#include <string.h>
#include "audio_rtos_header_group.h"

#include "audio_task.h"
#include "audio_shared_info.h"
#include "audio_drv_log.h"
#include "dsp_audio_msg_task.h"
#include "dsp_scenario.h"
#ifdef DSP_STATE_SUPPORT
#include "dsp_state.h"
#endif

#define PLAYBACK_MAX_MSG_QUEUE_SIZE 20
static bool playback_stop_done = false;

void set_playback_trigger_done(uint16_t msg_id)
{
    PRINTF_I("%s, msg_id = %d\n", __func__, msg_id);
    if (msg_id == MSG_TO_DSP_PCM_PLAYBACK_STOP)
        playback_stop_done = true;
}

static void playback_msg_task_constructor(AUDIO_TASK* this)
{
    this->task_priv = NULL; //TBD
    this->msg_queue = aud_create_msg_queue(PLAYBACK_MAX_MSG_QUEUE_SIZE);
}

static void playback_msg_task_destructor(AUDIO_TASK* this)
{
    vQueueDelete(this->msg_queue);
}

static void playback_msg_msg_handler(AUDIO_TASK* this, struct ipi_msg_t* ipi_msg)
{
    BaseType_t ret;
    AUDIO_TASK* task;

    AUD_DRV_LOG_D("+%s : ipi_msg->msg_id 0x%x\n", __func__, ipi_msg->msg_id);

    task = aud_get_audio_task(TASK_SCENE_PLAYBACK_MSG);

    ret = xQueueSendToBack(task->msg_queue, ipi_msg, 0);
    if (ret != pdPASS) {
        PRINTF_E("%s, send msg failed\n", __func__);
        return;
    }

    xTaskNotifyGive(task->thread_handler);
}

static void playback_msg_task_handle_ipc_msg(AUDIO_TASK* this, ipi_msg_t* msg_ptr)
{
    AUD_DRV_LOG_D("+%s : msg_ptr->msg_id 0x%x\n", __func__, msg_ptr->msg_id);

    switch (msg_ptr->msg_id) {
#ifdef CFG_CM4_PLAYBACK_ENABLE
    case MSG_TO_DSP_PCM_PLAYBACK_OPEN:
        CB_CM4_PLAYBACK_OPEN(msg_ptr);
        break;
    case MSG_TO_DSP_PCM_PLAYBACK_START:
        CB_CM4_PLAYBACK_START(msg_ptr);
        break;
    case MSG_TO_DSP_PCM_PLAYBACK_STOP:
        playback_stop_done = false;
        CB_CM4_PLAYBACK_STOP(msg_ptr);
        break;
    case MSG_TO_DSP_PCM_PLAYBACK_CLOSE:
        CB_CM4_PLAYBACK_CLOSE(msg_ptr);
        break;
    case MSG_TO_DSP_PCM_PLAYBACK_SUSPEND:
        CB_CM4_PLAYBACK_SUSPEND(msg_ptr);
        break;
    case MSG_TO_DSP_PCM_PLAYBACK_RESUME:
        CB_CM4_PLAYBACK_RESUME(msg_ptr);
        break;
#endif
    case MSG_TO_DSP_PCM_PLAYBACK_GET_SINK_MEM:
        CB_PLAYBACK_GET_SINK_MEM(msg_ptr);
        break;
#ifdef MTK_PROMPT_SOUND_ENABLE
    case MSG_TO_DSP_PROMPT_OPEN:
        CB_CM4_VP_PLAYBACK_OPEN(msg_ptr);
        break;
    case MSG_TO_DSP_PROMPT_START:
        CB_CM4_VP_PLAYBACK_START(msg_ptr);
        break;
    case MSG_TO_DSP_PROMPT_STOP:
        CB_CM4_VP_PLAYBACK_STOP(msg_ptr);
        break;
    case MSG_TO_DSP_PROMPT_CLOSE:
        CB_CM4_VP_PLAYBACK_CLOSE(msg_ptr);
        break;
    case MSG_TO_DSP_PROMPT_CONFIG:
        CB_CM4_VP_PLAYBACK_CONFIG(msg_ptr);
        break;
#endif
    default:
        PRINTF_E("%s, msg_id %d not implement yet,thread_id %d\n", __func__,
                 msg_ptr->msg_id, this->thread_id);
        break;
    }
}

static void playback_msg_task_loop(void* void_this)
{
    AUDIO_TASK* this = void_this;
    BaseType_t ret;
    ipi_msg_t ipi_msg;
    while (1) {
        //wait for queue
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

        ret = xQueueReceive(this->msg_queue, &ipi_msg, 0);
        configASSERT(ret==pdTRUE);
        /* process message */
        playback_msg_task_handle_ipc_msg(this, &ipi_msg);

	if (ipi_msg.msg_id == MSG_TO_DSP_PCM_PLAYBACK_STOP) {
            while (!playback_stop_done)
                vTaskDelay(10 * portTICK_PERIOD_MS);
	    PRINTF_I("%s, playback trigger stop done!\n", __func__);
        }

        /* send ack back if need */
        audio_send_ipi_msg_ack_back(&ipi_msg);

    }
}

const AUDIO_TASK_OPS g_aud_task_playback_msg_ops = {
    .constructor = playback_msg_task_constructor,
    .destructor = playback_msg_task_destructor,
    .create_task_loop = aud_create_task_loop_common,
    .destroy_task_loop = aud_destroy_task_loop_common,
    .task_loop_func = playback_msg_task_loop,
    .recv_message = playback_msg_msg_handler,
};

