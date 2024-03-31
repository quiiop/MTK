/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2018. All rights reserved.
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
 */
#include "va_state.h"
#include "va_state_implement.h"
#ifdef CFG_HW_RES_MGR
#include "hw_res_mgr.h"
#endif
#include "timers.h"
#include "mt_printf.h"
#include "systimer.h"
#include "dsp_state.h"
#include "dsp_clk.h"

struct ww_timer {
    TimerHandle_t timer;
    int timer_count;
};

static struct ww_timer g_ww_timer;

static void va_state_ww_timeout_callback(TimerHandle_t timer)
{
    if ((--(g_ww_timer.timer_count)) == 0){
        va_state_set_ww_timeout();
        PRINTF_D("[time:%llu]WW TIMER TIMEOUT\n", read_systimer_stamp_ns());
    }
}

static int va_state_start_ww_timeout(/*int time, */int count)
{
    if (!va_state_get_ww_timeout_en())
        return 0;

    g_ww_timer.timer_count = count;

    xTimerStart(g_ww_timer.timer, 0);
    PRINTF_D("[time:%llu]WW TIMER START\n", read_systimer_stamp_ns());
    return 0;
}

static int va_state_stop_ww_timeout(void)
{
    if (!va_state_get_ww_timeout_en())
        return 0;

    //if (xTimerIsTimerActive(g_ww_timer.timer))
        xTimerStop(g_ww_timer.timer, 0);
    return 0;
}

static void va_state_hw_res_set(int clk, int hw_req)
{
#ifdef CFG_HW_RES_MGR
    dsp_hw_res_lock();
    dsp_hw_res_request(DSP_HW_RES_PSRAM_REQ, DSP_HW_USER_VA_TASK, hw_req);
    dsp_hw_res_request(DSP_HW_RES_CLK, DSP_HW_USER_VA_TASK, clk);
//    dsp_hw_res_request(DSP_HW_RES_PSRAM_REQ, DSP_HW_USER_VA_TASK, hw_req);
    dsp_hw_res_unlock();
#endif
}

static int va_state_idle_enter(void)
{
    va_state_hw_res_set(CFG_VA_IDLE_DSP_CLK, CFG_VA_IDLE_SYS_HW);
    PRINTF_D("[time:%llu]va idle enter\n", read_systimer_stamp_ns());
    return 0;
}

static int va_state_idle_event_proc(int event)
{
    int next_state = VA_STAT_IDLE;
    int ret = 0;

    switch(event) {
    case VA_EVT_START:
        if (va_state_is_valid(VA_STAT_VAD))
            next_state = VA_STAT_VAD;
        else if ((!va_state_is_valid(VA_STAT_VAD)) && va_state_is_valid(VA_STAT_WAKEWORD))
            next_state = VA_STAT_WAKEWORD;
        else if ((!va_state_is_valid(VA_STAT_VAD)) && (!va_state_is_valid(VA_STAT_WAKEWORD)))
            next_state = VA_STAT_VOICEUPLOAD;
        break;
    default:
        break;
    }
    if (next_state != VA_STAT_IDLE)
        ret = va_state_switch(next_state);
    PRINTF_D("[time:%llu]va idle ev:%d, nst:%d\n", read_systimer_stamp_ns(), event, next_state);
    return ret;
}

static int va_state_vad_enter(void)
{
    int dsp_state;

    dsp_state_lock();
    dsp_state = dsp_state_get_without_lock();
    /* Before setting dram off, need do cache flush */
    /* need improve */
    if (dsp_state == DSP_STAT_LOWPOWER) {
        xthal_dcache_region_writeback_inv((void*)CFG_HIFI4_DRAM_ADDRESS, CFG_HIFI4_DRAM_SIZE);
        xthal_dcache_region_writeback_inv((void*)CFG_HIFI4_DRAM_RESERVE_CACHE_START, CFG_HIFI4_DRAM_RESERVE_CACHE_SIZE);
    }
    va_state_hw_res_set(CFG_VA_VAD_DSP_CLK, CFG_VA_VAD_SYS_HW);
    dsp_state_unlock();

    PRINTF_I("[time:%llu]vad enter\n", read_systimer_stamp_ns());
    return 0;
}

static int va_state_vad_event_proc(int event)
{
    int next_state = VA_STAT_VAD;
    int ret = 0;

    switch(event) {
    case VA_EVT_STOP:
        next_state = VA_STAT_IDLE;
        break;
    case VA_EVT_VAD_TOONOISY:
    case VA_EVT_VAD_SUCCESS:
    case VA_EVT_VAD_DISABLE:
        if (va_state_is_valid(VA_STAT_WAKEWORD))
            next_state = VA_STAT_WAKEWORD;
        else
            next_state = VA_STAT_VOICEUPLOAD;
        break;
    case VA_EVT_FORCE_VOICE_UPLOAD:
        next_state = VA_STAT_VOICEUPLOAD;
        break;
    default:
        break;
    }
    if (next_state != VA_STAT_VAD) {
        ret = va_state_switch(next_state);
        PRINTF_D("[time:%llu]vad ev:%d, nst:%d\n", read_systimer_stamp_ns(), event, next_state);
    }
    return ret;
}

static int va_state_ww_enter(void)
{
    va_state_hw_res_set(CFG_VA_WW_DSP_CLK, CFG_VA_WW_SYS_HW);
    /* 5. Add VA_EVT_WW_TIMEOUT event timer */
    va_state_clr_ww_timeout();
    va_state_start_ww_timeout(1);
    PRINTF_I("[time:%llu]ww enter\n", read_systimer_stamp_ns());
    return 0;
}

static int va_state_ww_exit(void)
{
    //TODO
    /* 1. delete VA_EVT_WW_TIMEOUT event timer */
    va_state_stop_ww_timeout();
    PRINTF_D("[time:%llu]ww exit\n", read_systimer_stamp_ns());
    return 0;
}

static int va_state_ww_event_proc(int event)
{
    int next_state = VA_STAT_WAKEWORD;
    int ret = 0;

    switch(event) {
    case VA_EVT_STOP:
        next_state = VA_STAT_IDLE;
        break;
    case VA_EVT_WW_SUCCESS:
    case VA_EVT_FORCE_VOICE_UPLOAD:
        next_state = VA_STAT_VOICEUPLOAD;
        break;
    case VA_EVT_WW_FAIL:
    case VA_EVT_WW_TIMEOUT:
        if (va_state_is_valid(VA_STAT_VAD))
            next_state = VA_STAT_VAD;
        else
            next_state = VA_STAT_WAKEWORD;
        break;
    default:
        break;
    }
    if (next_state != VA_STAT_WAKEWORD) {
        ret = va_state_switch(next_state);
        PRINTF_D("[time:%llu]ww ev:%d, nst:%d\n", read_systimer_stamp_ns(), event, next_state);
    }
    return ret;
}

static int va_state_voiceupload_enter(void)
{
    va_state_hw_res_set(CFG_VA_VOICE_UPLOAD_DSP_CLK, CFG_VA_VOICE_UPLOAD_SYS_HW);
    PRINTF_I("[time:%llu]upload enter\n", read_systimer_stamp_ns());
    return 0;
}

static int va_state_voiceupload_event_proc(int event)
{
    int next_state = VA_STAT_VOICEUPLOAD;
    int ret = 0;

    switch(event) {
    case VA_EVT_STOP:
        next_state = VA_STAT_IDLE;
        break;
    case VA_EVT_VOICE_UPLOAD_DONE:
        if (va_state_is_valid(VA_STAT_VAD))
            next_state = VA_STAT_VAD;
        else if ((!va_state_is_valid(VA_STAT_VAD)) && va_state_is_valid(VA_STAT_WAKEWORD))
            next_state = VA_STAT_WAKEWORD;
        else
            next_state = VA_STAT_IDLE;
        break;
    default:
        break;
    }
    if (next_state != VA_STAT_VOICEUPLOAD)
        ret = va_state_switch(next_state);
    PRINTF_D("[time:%llu]upload ev:%d, nst:%d\n", read_systimer_stamp_ns(), event, next_state);
    return ret;
}

struct virt_state_ops g_va_idle_ops = {
    .enter = va_state_idle_enter,
    .event_proc = va_state_idle_event_proc,
};

struct virt_state_ops g_va_vad_ops = {
    .enter = va_state_vad_enter,
    .event_proc = va_state_vad_event_proc,
};

struct virt_state_ops g_va_ww_ops = {
    .enter = va_state_ww_enter,
    .exit = va_state_ww_exit,
    .event_proc = va_state_ww_event_proc,
};

struct virt_state_ops g_va_voiceupload_ops = {
    .enter = va_state_voiceupload_enter,
    .event_proc = va_state_voiceupload_event_proc,
};

NORMAL_SECTION_FUNC int va_state_implement_init(void)
{
    va_state_init();
    va_state_register_ops(VA_STAT_IDLE, &g_va_idle_ops);
    va_state_register_ops(VA_STAT_VAD, &g_va_vad_ops);
    va_state_register_ops(VA_STAT_WAKEWORD, &g_va_ww_ops);
    va_state_register_ops(VA_STAT_VOICEUPLOAD, &g_va_voiceupload_ops);

    va_state_set_ww_timeout_en(CFG_VA_WW_TIMEOUT_EN);
    if (CFG_VA_WW_TIMEOUT_EN == 1) {
        int tickcount;
        PRINTF_D("Timer Create init\n");
        tickcount = CFG_VA_WW_TIMEOUT_LEN * configTICK_RATE_HZ / 1000;
        g_ww_timer.timer = xTimerCreate("WW Timeout",
                           tickcount, pdTRUE, (void *)0,
                           va_state_ww_timeout_callback);
    }

    /* default enable IDLE and VOICEUPLOAD state, others need user config */
    va_state_enable(VA_STAT_IDLE, 1);
    va_state_enable(VA_STAT_VOICEUPLOAD, 1);

    /* default state is IDLE */
    va_state_idle_enter();
    return 0;
}

NORMAL_SECTION_FUNC int va_state_implement_uninit(void)
{
    va_state_uninit();
    if (g_ww_timer.timer != NULL && CFG_VA_WW_TIMEOUT_EN == 1) {
        xTimerDelete(g_ww_timer.timer, 0);
        g_ww_timer.timer = NULL;
    }

    return 0;
}
