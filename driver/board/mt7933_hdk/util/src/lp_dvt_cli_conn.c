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
#include "hal_spm.h"

enum _CONN_INFRA_LP_STATE {
    CONN_INFRA_LP_STATE_ACTIVE,
    CONN_INFRA_LP_STATE_STANDBY,
    CONN_INFRA_LP_STATE_DEEPSLEEP,
    CONN_INFRA_LP_STATE_END
};

enum _CONN_INFRA_CLK_SRC {
    CONN_INFRA_CLK_SRC_XTAL,
    CONN_INFRA_CLK_SRC_2XTAL,
    CONN_INFRA_CLK_SRC_WPLL,
    CONN_INFRA_CLK_SRC_BTPLL,
    CONN_INFRA_CLK_SRC_END,
};

static uint8_t _cli_lp_dvt_conn_lpstate(uint8_t len, char *param[])
{
    uint32_t mode = atoi(param[0]);
    switch (mode) {
        case CONN_INFRA_LP_STATE_ACTIVE:
            break;
        case CONN_INFRA_LP_STATE_STANDBY:
            break;
        case CONN_INFRA_LP_STATE_DEEPSLEEP:
            break;
        default:
            break;
    }
    return 0;
}
static uint8_t _cli_lp_dvt_conn_clk(uint8_t len, char *param[])
{
    uint32_t clk = atoi(param[0]);
    switch (clk) {
        case CONN_INFRA_CLK_SRC_XTAL:
            break;
        case CONN_INFRA_CLK_SRC_2XTAL:
            break;
        case CONN_INFRA_CLK_SRC_WPLL:
            break;
        case CONN_INFRA_CLK_SRC_BTPLL:
            break;
        default:
            break;
    }
    return 0;
}

static uint8_t _cli_lp_dvt_conn_pos(uint8_t len, char *param[])
{
    hal_spm_conninfra_pos();
    return 0;
}
static uint8_t _cli_lp_dvt_conn_off(uint8_t len, char *param[])
{
    hal_spm_conninfra_off();
    return 0;
}
static uint8_t _cli_lp_dvt_conn_wakeup(uint8_t len, char *param[])
{
    hal_spm_conninfra_wakeup();
    return 0;
}
static uint8_t _cli_lp_dvt_conn_sleep(uint8_t len, char *param[])
{
    hal_spm_conninfra_sleep();
    return 0;
}

cmd_t conn_lp_dvt_cli_cmds[] = {
    { "pos",    "power on sequence of conn_infra",  _cli_lp_dvt_conn_pos, NULL},
    { "off",    "power off conn_infra",             _cli_lp_dvt_conn_off, NULL},
    { "wakeup", "wake up conn_infra",               _cli_lp_dvt_conn_wakeup, NULL},
    { "sleep",  "sleep conn_infra",                 _cli_lp_dvt_conn_sleep, NULL},
    { "lp_state",  "enter conninfra low power state: active/standby/deep sleep",  _cli_lp_dvt_conn_lpstate, NULL},
    { "clk",    "conn_infra dcm clock source:osc/osc2x/wpll/bpll",   _cli_lp_dvt_conn_clk, NULL},
    { NULL, NULL, NULL, NULL }
};
#endif /* MTK_LP_DVT_CLI_ENABLE */
