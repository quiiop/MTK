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

#if defined(MTK_MINICLI_ENABLE)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#include "hal_gpt.h"
#include "hal_rtc.h"
#include "sleep_manager_cli.h"
#include "hal_rtc.h"
#include "hal_rtc_internal.h"
#include "hal_spm.h"

#include "syslog.h"

extern uint32_t cli_dtim_sleep_mode;
extern uint32_t MACLPSState;
extern uint32_t MACLPNum;
TimerHandle_t xTimer_dtim_sleep;
uint32_t cli_dtim_mode_tmp;


static char *sleep_mode_name_list[] = {
    [HAL_SLEEP_MODE_IDLE] = "CPU Idle",
    [HAL_SLEEP_MODE_SLEEP] = "Deep Sleep",
    [HAL_SLEEP_MODE_LEGACY_SLEEP] = "Legacy sleep",
};

static void _lp_enter_sleep(hal_sleep_mode_t mode)
{
    if (mode <= HAL_SLEEP_MODE_NONE || mode >= HAL_SLEEP_MODE_NUMBER) {
	printf("Input argument invalid, mode=%d\n", mode);
	return;
    }
    printf("Entering %s, press anykey in console to wakeup .......\n", sleep_mode_name_list[mode]);
    vTaskDelay(50);

    uint32_t ret = hal_sleep_manager_enter_sleep_mode(mode);
    if (ret & 0xF0000000) {
        printf("Sleep ABORT, did not enter %s, reason=0x%08x\n", sleep_mode_name_list[mode], (unsigned int)ret);
    } else {
        printf("Successfully awaken from %s, wakeup irq=%lu\n", sleep_mode_name_list[mode], ret);
    }
}
static uint8_t _cli_lp_deep_sleep(uint8_t len, char *param[])
{
    bool do_restore = false;

    if (len >= 1 && !strcmp(param[0], "floor")) {
        hal_spm_conninfra_off();
        do_restore = true;
        printf("Applied setting for floor power\n");
    }

    _lp_enter_sleep(HAL_SLEEP_MODE_SLEEP);

    if (do_restore) {
        hal_spm_conninfra_pos();
    }

    return 0;
}

static uint8_t _cli_lp_legacy_sleep(uint8_t len, char *param[])
{
    _lp_enter_sleep(HAL_SLEEP_MODE_LEGACY_SLEEP);

    return 0;
}

static uint8_t _cli_lp_wfi_mode(uint8_t len, char *param[])
{
    _lp_enter_sleep(HAL_SLEEP_MODE_IDLE);

    return 0;
}

static uint8_t _cli_lp_rtc_mode(uint8_t len, char *param[])
{
#ifdef HAL_RTC_MODULE_ENABLED
    uint32_t sec = atoi(param[0]);

    hal_rtc_init();

    if (sec != 0) {
        printf("Entering RTC mode, wait %lu seconds or press RTC_EINT to wakeup...\n", sec);
        rtc_set_timer(sec);
    } else {
        printf("Entering RTC mode, press RTC_EINT to wakeup...\n");
    }
    vTaskDelay(50);

    hal_rtc_enter_rtc_mode();
#else /* #ifdef HAL_RTC_MODULE_ENABLED */
    printf("RTC module disabled!\n");
#endif /* #ifdef HAL_RTC_MODULE_ENABLED */
    return 0;
}

static uint8_t _cli_lp_set_sleep_lock_handle(uint8_t len, char *param[])
{
    uint32_t handle_index = atoi(param[0]);

    printf("cli force lock sleep handle: %d\n", (int)handle_index);
    hal_sleep_manager_lock_sleep(handle_index);

    return 0;
}

static uint8_t _cli_lp_set_sleep_unlock_handle(uint8_t len, char *param[])
{
    uint32_t handle_index = atoi(param[0]);

    printf("cli force unlock sleep handle: %d\n", (int)handle_index);
    hal_sleep_manager_unlock_sleep(handle_index);

    return 0;
}

static uint8_t _cli_lp_sleep_status(uint8_t len, char *param[])
{
    hal_gpt_delay_ms(100);

    sleep_management_dump_status();

    return 0;
}

#ifdef  SLEEP_MANAGEMENT_DEBUG_ENABLE
static uint8_t _cli_lp_dump_debug_log(uint8_t len, char *param[])
{
    uint32_t mode = atoi(param[0]);
    if (mode == 0) {
        sleep_management_debug_dump_lock_sleep_time();
    } else if (mode == 1) {
        sleep_management_debug_dump_suspend_resume_time();
    } else if (mode == 2) {
        sleep_management_debug_dump_suspend_mtcmos_sram_status();
    } else {
        printf("unknown command\n");
    }
    return 0;
}

static uint8_t _cli_lp_reset_debug_log(uint8_t len, char *param[])
{
#ifdef  SLEEP_MANAGEMENT_DEBUG_ENABLE
    uint32_t mode = atoi(param[0]);
    if (mode == 0) {
        sleep_management_debug_reset_lock_sleep_time();
        printf("Reset sleep lock time and count history\n");
    }
#endif /* #ifdef  SLEEP_MANAGEMENT_DEBUG_ENABLE */
    return 0;
}

static uint8_t _cli_lp_suspend(uint8_t len, char *param[])
{
    sleep_management_module_suspend();
    return 0;
}

static uint8_t _cli_lp_resume(uint8_t len, char *param[])
{
    sleep_management_module_resume();
    return 0;
}

static uint8_t _cli_lp_syslog(uint8_t len, char *param[])
{
    uint32_t mode = atoi(param[0]);
    if (mode == 0) {
        syslog_deep_sleep_switch(0);
        printf("Disabled syslog in deep sleep flow.\n");
    } else {
        syslog_deep_sleep_switch(1);
        printf("Enabled syslog in deep sleep flow.\n");
    }
    return 0;
}
#endif /* #ifdef  SLEEP_MANAGEMENT_DEBUG_ENABLE */

static uint8_t _cli_lp_set_criteria(uint8_t len, char *param[])
{
    if (len != 2U) {
        printf("Usage: <sleep mode> <sleep criteria>\n");
        printf("\tSleep mode:\n");
        for (unsigned int i = HAL_SLEEP_MODE_IDLE; i < (sizeof(sleep_mode_name_list) / sizeof(char *)); i++) {

            printf("\t%u: %s\n", i, sleep_mode_name_list[i]);
        }
        return -1;
    }

    hal_sleep_manager_status_t ret = HAL_SLEEP_MANAGER_OK;
    uint32_t mode = atoi(param[0]);
    uint32_t criteria = atoi(param[1]);
    ret = hal_sleep_manager_set_sleep_criteria(mode, criteria);

    if (ret != HAL_SLEEP_MANAGER_OK) {
        printf("Check sleep mode & sleep criteria!!\n");
        printf("Your input: mode: %ld, criteria: %ld\n", mode, criteria);
        return -1;
    }
    return 0;
}

static uint8_t _cli_lp_get_criteria(uint8_t len, char *param[])
{
    hal_sleep_manager_status_t ret = HAL_SLEEP_MANAGER_OK;
    uint32_t mode = 0;
    uint32_t criteria = 0;

    if (len < 1) {
        for (hal_sleep_mode_t i = HAL_SLEEP_MODE_IDLE; i < HAL_SLEEP_MODE_NUMBER; i++) {
            ret = hal_sleep_manager_get_sleep_criteria(i, &criteria);
            printf("Sleep mode: %s, Criteria: %ld\n", sleep_mode_name_list[i], criteria);
        }
    } else {
        mode = atoi(param[0]);
        ret = hal_sleep_manager_get_sleep_criteria(mode, &criteria);

        if (ret != HAL_SLEEP_MANAGER_OK) {
            printf("Check sleep mode!!\n");
            printf("Your input: mode: %ld\n", mode);
            return -1;
        } else {
            printf("Sleep mode: %s, Criteria: %ld\n", sleep_mode_name_list[mode], criteria);
        }
    }

    return 0;
}

static uint8_t _cli_lp_bus_int_ctrl(uint8_t len, char *param[])
{
    uint32_t is_enable = atoi(param[0]);
    hal_spm_bus_dbg_int_ctrl(is_enable);
    return 0;
}

cmd_t slp_mgr_cli_cmds[] = {
    { "status",         "sleep status",         _cli_lp_sleep_status, NULL},
    { "ds",             "deep sleep",           _cli_lp_deep_sleep, NULL},
    { "legacy",         "legacy sleep",         _cli_lp_legacy_sleep, NULL},
    { "wfi",            "CPU IDLE (WFI)",       _cli_lp_wfi_mode, NULL},
    { "rtc",            "rtc mode (seconds)",   _cli_lp_rtc_mode, NULL},
    { "set_slplock",    "set sleep lock",       _cli_lp_set_sleep_lock_handle, NULL},
    { "set_slpunlock",  "set sleep unlock",     _cli_lp_set_sleep_unlock_handle, NULL},
#ifdef  SLEEP_MANAGEMENT_DEBUG_ENABLE
    { "debug",          "debug log",            _cli_lp_dump_debug_log, NULL},
    { "debug_reset",    "reset debug log",      _cli_lp_reset_debug_log, NULL},
    { "debug_suspend",  "force call all module suspend",  _cli_lp_suspend, NULL},
    { "debug_resume",   "force call all module resume (0:has sleep, 1:no sleep)",   _cli_lp_resume, NULL      },
    { "debug_log",      "Syslog output directly while deep deep flow. (0: disable, 1:enable)",   _cli_lp_syslog, NULL      },
#endif /* #ifdef  SLEEP_MANAGEMENT_DEBUG_ENABLE */
    { "set_criteria",   "set sleep criteria by sleep mode",   _cli_lp_set_criteria, NULL},
    { "get_criteria",   "get sleep criteria by sleep mode",   _cli_lp_get_criteria, NULL},
    { "bus_dbg",      "disable(0)/enable(1)",                 _cli_lp_bus_int_ctrl, NULL },
    { NULL, NULL, NULL, NULL }
};
#endif /* #if defined(MTK_MINICLI_ENABLE) */
