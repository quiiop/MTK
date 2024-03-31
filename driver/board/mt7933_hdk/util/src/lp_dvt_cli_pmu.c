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
#ifdef HAL_PMU_MODULE_ENABLED
#include "hal_pmu.h"
#include "hal_pmu_wrap_interface.h"
#include "hal_log.h"

static uint8_t _cli_lp_dvt_pmu_mldo_psw_en(uint8_t len, char *param[])
{
    pmu_mldo_psw_en();
    log_hal_info("enable MLDO PSW Mode in LPM\r\n");

    return 0;
}

static uint8_t _cli_lp_dvt_pmu_normal_voltage(uint8_t len, char *param[])
{
    hal_pmu_set_vcore_voltage(HAL_PMU_VCORE_0p7V);
    log_hal_info("pmu set buckd to 0.7V.\r\n");
    hal_pmu_set_mldo_voltage(HAL_PMU_MLDO_0p8V);
    log_hal_info("pmu set mldo to 0.8V.\r\n");

    return 0;
}
static uint8_t _cli_lp_dvt_pmu_sleep_voltage(uint8_t len, char *param[])
{
    uint32_t mode = atoi(param[0]);

    if (mode) {
        log_hal_info("pmu set sleep voltage in normal mode.\r\n");
        pmu_set_vcore_slp_vol_mt7933(HAL_PMU_VCORE_0p7V);
        pmu_set_mldo_slp_vol_mt7933(HAL_PMU_MLDO_0p8V);
    } else {
        log_hal_info("pmu set sleep voltage in DVS mode.\r\n");
        pmu_set_vcore_slp_vol_mt7933(HAL_PMU_VCORE_0p8V);
        pmu_set_mldo_slp_vol_mt7933(HAL_PMU_MLDO_0p85V);
    }

    return 0;
}
static uint8_t _cli_lp_dvt_pmu_dvs_voltage(uint8_t len, char *param[])
{
    hal_pmu_set_mldo_voltage(HAL_PMU_MLDO_0p85V);
    log_hal_info("pmu set mldo to 0.85V.\r\n");
    hal_pmu_set_vcore_voltage(HAL_PMU_VCORE_0p8V);
    log_hal_info("pmu set buckd to 0.8V.\r\n");

    return 0;
}
cmd_t pmu_lp_dvt_cli_cmds[] = {
    { "normal_v", "set normal voltage: 0.8v/0.7v",  _cli_lp_dvt_pmu_normal_voltage, NULL},
    { "dvs_v",    "set DVS voltage: 0.85v/0.8v",    _cli_lp_dvt_pmu_dvs_voltage, NULL},
    { "sleep_v",  "set sleep voltage: 0.65v",       _cli_lp_dvt_pmu_sleep_voltage, NULL},
    { "mldo_psw",  "enable MLDO PSW Mode in LPM",   _cli_lp_dvt_pmu_mldo_psw_en, NULL},
    { NULL, NULL, NULL, NULL }
};
#endif /* HAL_PMU_MODULE_ENABLED */
#endif /* MTK_LP_DVT_CLI_ENABLE */
