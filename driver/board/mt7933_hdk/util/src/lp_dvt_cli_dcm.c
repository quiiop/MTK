/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include <stdio.h>
#include <stdlib.h>
#include "cli.h"

#ifdef MTK_LP_DVT_CLI_ENABLE
#include "common.h"
#include "reg_base.h"
#define TOP_DCM_CTL                (INFRA_BCRM_AON_BASE + 0x70)
#define HAL_DCM_STATUS_SUCCESS (0)
#define HAL_DCM_STATUS_FAIL    (0xFF)

enum {
    DCM_AXICLK_1_div_32,
    DCM_AXICLK_1_div_16,
    DCM_AXICLK_1_div_8,
    DCM_AXICLK_1_div_4,
    DCM_AXICLK_1_div_2,
    DCM_AXICLK_FULL,
    DCM_AXICLK_END
};

static uint8_t _cli_lp_dvt_dcm_axi_normal(uint8_t len, char *param[])
{
    uint32_t clk = atoi(param[0]);
    if (clk >= DCM_AXICLK_END)
        return HAL_DCM_STATUS_FAIL;

    HAL_REG_32(TOP_DCM_CTL) &= ~(BITS(8, 12));

    if (clk == DCM_AXICLK_1_div_32)
        return HAL_DCM_STATUS_SUCCESS;

    HAL_REG_32(TOP_DCM_CTL) |= (clk << 8);

    return HAL_DCM_STATUS_SUCCESS;
}
static uint8_t _cli_lp_dvt_dcm_axi_idle(uint8_t len, char *param[])
{
    uint32_t clk = atoi(param[0]);
    if (clk >= DCM_AXICLK_END)
        return HAL_DCM_STATUS_FAIL;

    HAL_REG_32(TOP_DCM_CTL) &= ~(BITS(13, 17));

    if (clk == DCM_AXICLK_1_div_32)
        return HAL_DCM_STATUS_SUCCESS;

    HAL_REG_32(TOP_DCM_CTL) |= (clk << 13);

    return HAL_DCM_STATUS_SUCCESS;
}

static uint8_t _cli_lp_dvt_dcm_axi_enable(uint8_t len, char *param[])
{
    uint32_t en = atoi(param[0]);

    if (en) {
        HAL_REG_32(TOP_DCM_CTL) &= ~(BIT(0)); // dcm enable
        HAL_REG_32(TOP_DCM_CTL) |= BIT(1);    // clock enable
    } else {
        HAL_REG_32(TOP_DCM_CTL) |= BIT(0);    // dcm disable
        HAL_REG_32(TOP_DCM_CTL) &= ~(BIT(1)); // clock disable
    }
    return HAL_DCM_STATUS_SUCCESS;
}

static uint8_t _cli_lp_dvt_dcm_axi_clkoff(uint8_t len, char *param[])
{
    uint32_t en = atoi(param[0]);

    if (en)
        HAL_REG_32(TOP_DCM_CTL) |= BIT(3);
    else
        HAL_REG_32(TOP_DCM_CTL) &= ~(BIT(3));

    return HAL_DCM_STATUS_SUCCESS;
}
static uint8_t _cli_lp_dvt_dcm_axi_clkslow(uint8_t len, char *param[])
{
    uint32_t en = atoi(param[0]);

    if (en)
        HAL_REG_32(TOP_DCM_CTL) |= BIT(4);
    else
        HAL_REG_32(TOP_DCM_CTL) &= ~(BIT(4));

    return HAL_DCM_STATUS_SUCCESS;
}

static uint8_t _cli_lp_dvt_dcm_dsp_normal(uint8_t len, char *param[])
{
    // TODO: not ready
    return HAL_DCM_STATUS_SUCCESS;
}
static uint8_t _cli_lp_dvt_dcm_dsp_idle(uint8_t len, char *param[])
{
    // TODO: not ready
    return HAL_DCM_STATUS_SUCCESS;
}

static uint8_t _cli_lp_dvt_dcm_dsp_enable(uint8_t len, char *param[])
{
    // TODO: not ready
    return HAL_DCM_STATUS_SUCCESS;
}

static uint8_t _cli_lp_dvt_dcm_dsp_clkoff(uint8_t len, char *param[])
{
    // TODO: not ready
    return HAL_DCM_STATUS_SUCCESS;
}
static uint8_t _cli_lp_dvt_dcm_dsp_clkslow(uint8_t len, char *param[])
{
    // TODO: not ready
    return HAL_DCM_STATUS_SUCCESS;
}


cmd_t dcm_lp_dvt_cli_cmds[] = {
    { "dcm_axi_normal_set",  "AXI DCM configure in normal mode: 1/2/4/16/32",  _cli_lp_dvt_dcm_axi_normal, NULL},
    { "dcm_axi_idle_set",    "AXI DCM configure in idle   mode: 1/2/4/16/32",  _cli_lp_dvt_dcm_axi_idle, NULL},
    { "dcm_axi_en",          "AXI DCM enable(1) / disable(0)",                 _cli_lp_dvt_dcm_axi_enable, NULL},
    { "dcm_axi_clkoff",      "AXI DCM turn on clock OFF when IDLE",            _cli_lp_dvt_dcm_axi_clkoff, NULL},
    { "dcm_axi_clkslow",     "AXI DCM turn on clock slow when IDLE",           _cli_lp_dvt_dcm_axi_clkslow, NULL},

    { "dcm_dsp_normal_set",  "DSP DCM configure in normal mode: 1/2/4/16/32",  _cli_lp_dvt_dcm_dsp_normal, NULL},
    { "dcm_dsp_idle_set",    "DSP DCM configure in idle   mode: 1/2/4/16/32",  _cli_lp_dvt_dcm_dsp_idle, NULL},
    { "dcm_dsp_en",          "DSP DCM enable(1) / disable(0)",                 _cli_lp_dvt_dcm_dsp_enable, NULL},
    { "dcm_dsp_clkoff",      "DSP DCM turn on clock OFF when IDLE",            _cli_lp_dvt_dcm_dsp_clkoff, NULL},
    { "dcm_dsp_clkslow",     "DSP DCM turn on clock slow when IDLE",           _cli_lp_dvt_dcm_dsp_clkslow, NULL},

    { NULL, NULL, NULL, NULL }
};
#endif /* MTK_LP_DVT_CLI_ENABLE */
