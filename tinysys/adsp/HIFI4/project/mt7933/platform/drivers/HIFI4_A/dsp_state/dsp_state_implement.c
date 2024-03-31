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
#include "dsp_state.h"
#include "dsp_state_implement.h"
#ifdef CFG_HW_RES_MGR
#include "hw_res_mgr.h"
#endif
#include "timers.h"

#include "mt_printf.h"
#include "systimer.h"
#include "dsp_clk.h"

#ifdef CFG_DSP_SWITCH_STATE_EN
struct switch_timer {
    TimerHandle_t timer;
    int timer_count;
};

static struct switch_timer g_timer;

static void dsp_state_switch_timer_cb(TimerHandle_t timer)
{
    /* TODO check polling flag */

    g_timer.timer_count--;
    /* check poll OK count */
    if (g_timer.timer_count == 0)
        dsp_state_event_proc(DSP_EVT_AP_SWTICH_DONE);
}

static int dsp_state_start_switch_timer(int time, int count)
{
    int tickcount;

    g_timer.timer_count = count;
    tickcount = time * configTICK_RATE_HZ / 1000;
    g_timer.timer = xTimerCreate("Switch Timer",
                       tickcount, pdTRUE, (void *)0,
                       dsp_state_switch_timer_cb);
    xTimerStart(g_timer.timer, 0);
    return 0;
}

static int dsp_state_stop_switch_timer(void)
{
    if (xTimerIsTimerActive(g_timer.timer)) {
        xTimerStop(g_timer.timer, 0);
        xTimerDelete(g_timer.timer, 0);
    }
    g_timer.timer_count = 0;
    return 0;
}
#endif

static void dsp_state_hw_res_set(int clk, int hw_req)
{
#ifdef CFG_HW_RES_MGR
    dsp_hw_res_lock();
    dsp_hw_res_request(DSP_HW_RES_PSRAM_REQ, DSP_HW_USER_TOP_CTRL, hw_req);
    dsp_hw_res_request(DSP_HW_RES_CLK, DSP_HW_USER_TOP_CTRL, clk);
//    dsp_hw_res_request(DSP_HW_RES_PSRAM_REQ, DSP_HW_USER_TOP_CTRL, hw_req);
    dsp_hw_res_unlock();
#endif
}

static void dsp_state_hw_res_enable(int clk_en, int hw_req_en)
{
#ifdef CFG_HW_RES_MGR
    dsp_hw_res_lock();
    dsp_hw_res_enable(DSP_HW_RES_PSRAM_REQ, hw_req_en);
    dsp_hw_res_enable(DSP_HW_RES_CLK, clk_en);
    dsp_hw_res_unlock();
#endif
}

static int dsp_state_idle_event_proc(int event)
{
    if (event == DSP_EVT_INIT)
        dsp_state_switch(DSP_STAT_NORMAL);

    PRINTF_D("[time:%llu]dsp idle ev:%d\n", read_systimer_stamp_ns(), event);
    return 0;
}

static int dsp_state_idle_exit(void)
{
     dsp_state_hw_res_set(DSP_CLK_13M, DSP_PSRAM_NONEED);
     PRINTF_D("[time:%llu]dsp idle exit\n", read_systimer_stamp_ns());
    return 0;
}

static int dsp_state_normal_enter(void)
{
    dsp_state_hw_res_enable(1, 1);
    /* 4. Extra control */
    PRINTF_D("[time:%llu]dsp normal enter\n", read_systimer_stamp_ns());
    return 0;
}

static int dsp_state_normal_exit(void)
{
    PRINTF_D("[time:%llu]dsp normal exit\n", read_systimer_stamp_ns());
    return 0;
}

static int dsp_state_normal_event_proc(int event)
{
    int next_state = DSP_STAT_NORMAL;
    int ret = 0;

    switch(event) {
    case DSP_EVT_AP_SUSPEND:
#ifdef CFG_DSP_SWITCH_STATE_EN
        next_state = DSP_STAT_SWITCH;
#else
        next_state = DSP_STAT_LOWPOWER;
#endif
        break;
    case DSP_EVT_AP_RESUME:
    case DSP_EVT_AP_SWTICH_DONE:
        break;
    default:
        break;
    }
    if (next_state != DSP_STAT_NORMAL)
        ret = dsp_state_switch(next_state);
    PRINTF_D("[time:%llu]dsp normal ev:%d, nst:%d\n", read_systimer_stamp_ns(), event, next_state);
    return ret;
}

static int dsp_state_lowpower_enter(void)
{
    dsp_state_hw_res_enable(1, 1);
    /* 4. force update va status */
    PRINTF_D("[time:%llu]dsp lowpower enter\n", read_systimer_stamp_ns());
    return 0;
}

static int dsp_state_lowpower_exit(void)
{
     PRINTF_D("[time:%llu]dsp lowpower exit\n", read_systimer_stamp_ns());
    return 0;
}

static int dsp_state_lowpower_event_proc(int event)
{
    int next_state = DSP_STAT_LOWPOWER;
    int ret = 0;

    switch(event) {
    case DSP_EVT_AP_SUSPEND:
    case DSP_EVT_AP_SWTICH_DONE:
        break;
    case DSP_EVT_AP_RESUME:
        next_state = DSP_STAT_NORMAL;
        break;
    default:
        break;
    }
    if (next_state != DSP_STAT_LOWPOWER)
        ret = dsp_state_switch(next_state);
    PRINTF_D("[time:%llu]dsp lowpower ev:%d, nst:%d\n", read_systimer_stamp_ns(), event, next_state);
    return ret;
}
#ifdef CFG_DSP_SWITCH_STATE_EN
static int dsp_state_switch_enter(void)
{
#ifdef CFG_HW_RES_MGR
    //TODO
    dsp_hw_res_lock();
    /* 1. set DSP_HW_RES_SPM_REQ to ON and DSP_HW_RES_CLK to DSPPLL level */
    dsp_hw_res_request(DSP_HW_RES_SPM_REQ, DSP_HW_USER_TOP_CTRL, CFG_DSP_SWITCH_SYS_HW);
    dsp_hw_res_request(DSP_HW_RES_CLK, DSP_HW_USER_TOP_CTRL, CFG_DSP_SWITCH_DSP_CLK);

    /* 2. set DSP_HW_RES_SPM_REQ disable */
    dsp_hw_res_enable(DSP_HW_RES_SPM_REQ, 0);
    /* 3. set DSP_HW_RES_CLK     disable */
    dsp_hw_res_enable(DSP_HW_RES_CLK, 0);
    /* 4. set DSP_HW_RES_SPM_INT disable */
    dsp_hw_res_enable(DSP_HW_RES_SPM_INT, 0);
    dsp_hw_res_unlock();
#endif
    /* 5. create 10ms period timer to poll spm status */
    /* TODO, now use 1 second do the test */
    dsp_state_start_switch_timer(10, CFG_DSP_SWITCH_TIMEOUT_LEN / 10);
    PRINTF_D("[time:%llu]dsp switch enter\n", read_systimer_stamp_ns());
    return 0;
}

static int dsp_state_switch_exit(void)
{
     /* 1. Destroy 10ms period timer to poll spm status */
    dsp_state_stop_switch_timer();
#ifdef CFG_HW_RES_MGR
    /* 2. Set DSP_HW_RES_CLK to 26M */
    dsp_hw_res_lock();
    dsp_hw_res_request(DSP_HW_RES_CLK, DSP_HW_USER_TOP_CTRL, DSP_CLK_26M);
    dsp_hw_res_request(DSP_HW_RES_SPM_REQ, DSP_HW_USER_TOP_CTRL, DSP_SYS_HW_NONE);
    dsp_hw_res_unlock();
#endif
    PRINTF_D("[time:%llu]dsp switch exit\n", read_systimer_stamp_ns());
    return 0;
}

static int dsp_state_switch_event_proc(int event)
{
    int next_state = DSP_STAT_SWITCH;
    int ret = 0;

    switch(event) {
    case DSP_EVT_AP_SUSPEND:
        break;
    case DSP_EVT_AP_RESUME:
        next_state = DSP_STAT_NORMAL;
        break;
    case DSP_EVT_AP_SWTICH_DONE:
        next_state = DSP_STAT_LOWPOWER;
        break;
    default:
        break;
    }
    if (next_state != DSP_STAT_SWITCH)
        ret = dsp_state_switch(next_state);
    PRINTF_D("[time:%llu]dsp switch ev:%d, nst:%d\n", read_systimer_stamp_ns(), event, next_state);
    return ret;
}
#endif

struct virt_state_ops g_dsp_idle_ops = {
    .exit = dsp_state_idle_exit,
    .event_proc = dsp_state_idle_event_proc,
};

struct virt_state_ops g_dsp_normal_ops = {
    .enter = dsp_state_normal_enter,
    .exit = dsp_state_normal_exit,
    .event_proc = dsp_state_normal_event_proc,
};

struct virt_state_ops g_dsp_lowpower_ops = {
    .enter = dsp_state_lowpower_enter,
    .exit = dsp_state_lowpower_exit,
    .event_proc = dsp_state_lowpower_event_proc,
};

#ifdef CFG_DSP_SWITCH_STATE_EN
struct virt_state_ops g_dsp_switch_ops = {
    .enter = dsp_state_switch_enter,
    .exit = dsp_state_switch_exit,
    .event_proc = dsp_state_switch_event_proc,
};
#endif

NORMAL_SECTION_FUNC int dsp_state_implement_init(void)
{
    dsp_state_init();
    dsp_state_register_ops(DSP_STAT_IDLE, &g_dsp_idle_ops);
    dsp_state_register_ops(DSP_STAT_NORMAL, &g_dsp_normal_ops);
#ifdef CFG_DSP_SWITCH_STATE_EN
    dsp_state_register_ops(DSP_STAT_SWITCH, &g_dsp_switch_ops);
#endif
    dsp_state_register_ops(DSP_STAT_LOWPOWER, &g_dsp_lowpower_ops);
    dsp_state_enable(DSP_STAT_IDLE, 1);
    dsp_state_enable(DSP_STAT_NORMAL, 1);
    /* Dyanmic control the switch state enable */
#ifdef CFG_DSP_SWITCH_STATE_EN
    dsp_state_enable(DSP_STAT_SWITCH, 1);
#endif
    dsp_state_enable(DSP_STAT_LOWPOWER, 1);
    return 0;
}

NORMAL_SECTION_FUNC int dsp_state_implement_uninit(void)
{
    dsp_state_uninit();
    return 0;
}
