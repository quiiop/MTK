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
#include <string.h>
#include "audio_rtos_header_group.h"
#include "mtk_heap.h"
#include "afe_drv_pcm.h"
#include "audio_task.h"
#include "audio_task_top_ctrl.h"
#include "audio_shared_info.h"
#ifdef DSP_STATE_SUPPORT
#include "dsp_state.h"
#endif
#ifdef DUMP_TO_HOST_SUPPORT
#include "audio_dump_helper.h"
#endif

#define QUEUE_SIZE 20

void top_ctrl_task_recv_message(AUDIO_TASK *this,
    struct ipi_msg_t *ipi_msg)
{
    BaseType_t ret;
    ret = xQueueSendToBack(aud_get_audio_task(TASK_SCENE_AUDIO_CONTROLLER)->msg_queue,
                   ipi_msg, 0);
    if (ret != pdPASS) {
        PRINTF_E("%s, send msg failed\n", __func__);
    }
    xTaskNotifyGive(aud_get_audio_task(TASK_SCENE_AUDIO_CONTROLLER)->thread_handler);

    if (ret != pdTRUE) {
        PRINTF_E("%s, wake thread failed\n", __func__);
        return;
    }

}

static void top_ctrl_task_constructor(AUDIO_TASK* this)
{
    this->task_priv = NULL; //TBD
    this->msg_queue = aud_create_msg_queue(QUEUE_SIZE);
    //TODO: register ipc_cb_for_top_ctrl_task to IPC
}

#ifdef DUMP_TO_HOST_SUPPORT
static void top_ctrl_task_handle_debug(ipi_msg_t *msg)
{
    int ret;

    PRINTF_D("%s, get id 0x%x\n", __func__, msg->msg_id);
    ret = audio_dump_handle_ipc_msg(msg);
    if (ret != 0) {
        PRINTF_D("[%s] audio_dump_handle_ipc_msg fail\n", __func__);
    }
 }
#endif

static void top_ctrl_task_handle_ipc_msg(AUDIO_TASK* this, ipi_msg_t *msg)
{
    switch (msg->msg_id) {
#ifdef CFG_TASK_VA_SUPPORT
#ifdef CFG_DSPOTTER_SUPPORT
	case MSG_TO_DSP_CYBERON_LICENSE:
	{
		int ret = 0;
		void *cyberon_license_addr = MTK_pvPortMalloc(256, MTK_eMemDramNormal);
		if (cyberon_license_addr == NULL) {
			ret = -1;
			memcpy(msg->payload, &ret, sizeof(int));
		} else {
			extern void set_cyberon_license_addr(char *addr);
			set_cyberon_license_addr((char *)cyberon_license_addr);
			memcpy(cyberon_license_addr, msg->payload, 256);
			ret = 0;
			memcpy(msg->payload, &ret, sizeof(int));
		}

		PRINTF_I("%s, get cyberon license addr = %p\n", __func__, cyberon_license_addr);
	}
		break;
	case MSG_TO_DSP_MALLOC_CYBERON_BUFFER:
	{
		uint32_t addr_info = 0;
		memcpy(&addr_info, msg->payload, sizeof(uint32_t));
		void *malloc_addr = MTK_pvPortMalloc(addr_info, MTK_eMemDramNormalNC);

		extern void set_cyberon_model_addr(void *addr);
		set_cyberon_model_addr(malloc_addr);

		addr_info = (uint32_t)malloc_addr;
		memcpy(msg->payload, &addr_info, sizeof(uint32_t));
		PRINTF_I("%s, malloc cyberon buffer, addr_info = %x\n", __func__, addr_info);
	}
		break;
#endif
    case MSG_TO_DSP_CREATE_VA_T:
        init_audio_task(TASK_SCENE_VA);
        PRINTF_D("%s, initialize Voice Assistant task\n", __func__);
        break;
    case MSG_TO_DSP_DESTROY_VA_T:
        deinit_audio_task(TASK_SCENE_VA);
        PRINTF_D("%s, deinit Voice Assistant task\n", __func__);
        break;
#endif
#ifdef DSP_STATE_SUPPORT
    case MSG_TO_DSP_AP_SUSPEND_T:
        dsp_state_event_proc(DSP_EVT_AP_SUSPEND);
        break;
    case MSG_TO_DSP_AP_RESUME_T:
        dsp_state_event_proc(DSP_EVT_AP_RESUME);
        break;
#endif
#ifdef DUMP_TO_HOST_SUPPORT
    case MSG_TO_DSP_DEBUG_START:
    case MSG_TO_DSP_DEBUG_STOP:
        top_ctrl_task_handle_debug(msg);
        break;
#endif
    default:
        PRINTF_E("%s, msgID not implement yet.\n", __func__);
        PRINTF_E("    msgID: %d, msg name: %s, thread_id: %u\n",
             msg->msg_id, msg_string[msg->msg_id], this->thread_id);
        break;
    }
}

static void top_ctrl_task_loop(void* void_this)
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
        top_ctrl_task_handle_ipc_msg(this, &ipi_msg);

        /* send ack back if need */
        audio_send_ipi_msg_ack_back(&ipi_msg);

    }
}

static void top_ctrl_task_destructor(AUDIO_TASK* this)
{
     vQueueDelete(this->msg_queue);
}

const AUDIO_TASK_OPS g_aud_task_top_ctrl_ops = {
    .constructor = top_ctrl_task_constructor,
    .destructor = top_ctrl_task_destructor,
    .create_task_loop = aud_create_task_loop_common,
    .destroy_task_loop = aud_destroy_task_loop_common,
    .task_loop_func = top_ctrl_task_loop,
    .recv_message = top_ctrl_task_recv_message,
};
