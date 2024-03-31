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
#include "hw_res_mgr.h"
#include "audio_rtos_header_group.h"
#include "semphr.h"

#include "mt_printf.h"
#include "systimer.h"
#ifdef CFG_DSP_CLK_SUPPORT
#include "dsp_clk.h"
#endif

#ifdef CFG_VCORE_DVFS_SUPPORT
#include "dvfsrc.h"
#endif

#ifdef CFG_DSP_AP_IRQ_SUPPORT
#include "dsp_ap_irq.h"
#endif

int dsp_clk_res_ops_set(int value)
{
    int ret = 0;

#ifdef CFG_DSP_CLK_SUPPORT
    dsp_clk_select(value);
#endif

    return ret;
}

int psram_res_ops_set(int value)
{
    int ret = 0;
#ifdef CFG_DSP_AP_IRQ_SUPPORT
    /* convert res manger value to driver map */
    switch (value) {
    case DSP_PSRAM_NONEED:
        value = INT_SLEEP_PSRAM;
        break;
    case DSP_PSRAM_NEED:
        value = INT_WAKEUP_PSRAM;
        break;
    default:
        break;
    }

    ret = ap_req_set(value);
    if (ret != 0)
        PRINTF_E("psram res set to %d fail(%d)\n", value, ret);
#endif

    return ret;
}

NORMAL_SECTION_FUNC int hw_res_implement_init(void)
{
    dsp_hw_res_init();

#ifdef CFG_DSP_AP_IRQ_SUPPORT
    ap_req_init();
#endif /* CFG_DSP_AP_IRQ_SUPPORT */

#ifdef CFG_DSP_CLK_SUPPORT
    dsp_clk_init();
#endif

    dsp_hw_res_register_ops(DSP_HW_RES_CLK, dsp_clk_res_ops_set);
    dsp_hw_res_register_ops(DSP_HW_RES_PSRAM_REQ, psram_res_ops_set);

    return 0;
}

NORMAL_SECTION_FUNC int hw_res_implement_uninit(void)
{
#ifdef CFG_DSP_AP_IRQ_SUPPORT
    ap_req_init();
#endif /* CFG_DSP_AP_IRQ_SUPPORT */

    dsp_hw_res_uninit();
    return 0;
}



