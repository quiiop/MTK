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

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>


#if defined(MTK_MINICLI_ENABLE)


#include "toi.h"


#include "minicli_cmd_table.h"

#ifdef MTK_UT_ENABLE
#include "ut.h"
#endif /* #ifdef MTK_UT_ENABLE */

static cli_t *_cli_ptr;


#ifdef MTK_CLI_TEST_MODE_ENABLE
static uint8_t _sdk_cli_test_mode(uint8_t len, char *param[]);
static uint8_t _sdk_cli_normal_mode(uint8_t len, char *param[]);
#endif /* #ifdef MTK_CLI_TEST_MODE_ENABLE */

#ifdef MTK_CLI_TEST_MODE_ENABLE
#define GOTO_TEST_MODE_CLI_ENTRY    { "en",   "enter test mode",     _sdk_cli_test_mode    ,NULL},
#define GOTO_NORMAL_MODE_CLI_ENTRY  { "back", "back to normal mode", _sdk_cli_normal_mode ,NULL },
#endif /* #ifdef MTK_CLI_TEST_MODE_ENABLE */


#ifndef GOTO_TEST_MODE_CLI_ENTRY
#define GOTO_TEST_MODE_CLI_ENTRY
#endif /* #ifndef GOTO_TEST_MODE_CLI_ENTRY */

#ifndef GOTO_NORMAL_MODE_CLI_ENTRY
#define GOTO_NORMAL_MODE_CLI_ENTRY
#endif /* #ifndef GOTO_NORMAL_MODE_CLI_ENTRY */


/**
 * ADSP_CLI_ENTRY
 */
#if defined(BRINGUP_DSP_ENABLE) && defined(MTK_HIFI4DSP_ENABLE)
#include "adsp_debug.h"
#else /* #if defined(BRINGUP_DSP_ENABLE) && defined(MTK_HIFI4DSP_ENABLE) */
#define ADSP_CLI_ENTRY
#endif /* #if defined(BRINGUP_DSP_ENABLE) && defined(MTK_HIFI4DSP_ENABLE) */

/****************************************************************************
 *
 * Integer conversion
 *
 ****************************************************************************/




/****************************************************************************
 *
 * TEST commands
 *
 ****************************************************************************/



#ifdef HAL_WDT_MODULE_ENABLED
#include "hal_wdt.h"

static uint8_t _cmd_wdt(uint8_t len, char *param[])
{
    if (len < 1) {
        cli_puts("wdt [0:disable, 1:enable]\n");
    }

    if (atoi(param[0]) == 1) {
        hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
        cli_puts("wdt enabled\n");
    } else {
        hal_wdt_disable(HAL_WDT_DISABLE_MAGIC);
        cli_puts("wdt disabled\n");
    }
    return 0;
}
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */

/****************************************************************************
 *
 * TEST MODE
 *
 ****************************************************************************/


#ifdef MTK_CLI_TEST_MODE_ENABLE

static cmd_t   _cmds_test[] = {
    GOTO_NORMAL_MODE_CLI_ENTRY
    MINICLI_TEST_MODE_CLI_CMDS
#ifdef HAL_WDT_MODULE_ENABLED
    { "wdt", "WDT control", _cmd_wdt, NULL },
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
    { NULL, NULL, NULL, NULL }
};

#endif /* #ifdef MTK_CLI_TEST_MODE_ENABLE */


/****************************************************************************
 *
 * Wi-Fi commands
 *
 ****************************************************************************/


/****************************************************************************
 *
 * BT commands
 *
 ****************************************************************************/

#ifdef BRINGUP_BT_ENABLE
#ifdef MTK_MT7933_BT_ENABLE
extern cmd_t bt_driver_cli[];
#endif /* #ifdef MTK_MT7933_BT_ENABLE */
#ifdef MTK_BT_BOOTS_ENABLE
extern uint8_t btpriv_cli(uint8_t len, char *param[]);
#endif /* #ifdef MTK_BT_BOOTS_ENABLE */
static cmd_t   _cmds_bt[] = {
#ifdef MTK_MT7933_BT_ENABLE
    { "btdrv", "bt driver cli cmd", NULL, bt_driver_cli },
#endif /* #ifdef MTK_MT7933_BT_ENABLE */
#ifdef MTK_BT_BOOTS_ENABLE
    { "btpriv", "bt command", btpriv_cli, NULL },
#endif /* #ifdef MTK_BT_BOOTS_ENABLE */
    { NULL, NULL, NULL, NULL }
};
#endif /* #ifdef BRINGUP_BT_ENABLE */


/****************************************************************************
 *
 * NORMAL MODE
 *
 ****************************************************************************/

#ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE
extern cmd_t audio_drv_debug_cmds[];
#endif /* #ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE */

static cmd_t   _cmds_normal[] = {
#ifdef BRINGUP_BT_ENABLE
    { "bt",   "BT commands",                   NULL,       &_cmds_bt[0] },
#endif /* #ifdef BRINGUP_BT_ENABLE */
#ifdef BRINGUP_DSP_ENABLE
    ADSP_CLI_ENTRY
#endif /* #ifdef BRINGUP_DSP_ENABLE */
    GOTO_TEST_MODE_CLI_ENTRY
    MINICLI_NORMAL_MODE_CLI_CMDS
    OS_CLI_ENTRY
#ifdef MTK_UT_ENABLE
    UT_CLI_ENTRY
#endif /* #ifdef MTK_UT_ENABLE */
#ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE
    { "aud_dbg", "audio driver debug common", NULL, &audio_drv_debug_cmds[0]},
#endif /* #ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE */
    { NULL, NULL, NULL, NULL }
};


/****************************************************************************
 *
 * TOGGLE commands
 *
 ****************************************************************************/


#ifdef MTK_CLI_TEST_MODE_ENABLE
static uint8_t _sdk_cli_test_mode(uint8_t len, char *param[])
{
    _cli_ptr->cmd = &_cmds_test[0];
    return 0;
}
#endif /* #ifdef MTK_CLI_TEST_MODE_ENABLE */




#if defined(MTK_CLI_TEST_MODE_ENABLE)
static uint8_t _sdk_cli_normal_mode(uint8_t len, char *param[])
{
    _cli_ptr->cmd = &_cmds_normal[0];
    return 0;
}
#endif /* #if defined(MTK_CLI_TEST_MODE_ENABLE) */


/****************************************************************************
 *
 * PUBLIC functions
 *
 ****************************************************************************/


void cli_cmds_init(cli_t *cli)
{
    _cli_ptr = cli;
    _cli_ptr->cmd = &_cmds_normal[0];
}


#endif /* #if defined(MTK_MINICLI_ENABLE) */
