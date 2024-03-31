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

#if defined(MTK_LP_DVT_CLI_ENABLE) && defined(HAL_RTC_MODULE_ENABLED)

#include <stdio.h>
#include <stdlib.h>
#include "cli.h"
#include "hal.h"
#include "hal_rtc.h"
#include "hal_rtc_internal.h"
#include "hal_platform.h"
#include "string.h"

static uint8_t _cli_lp_dvt_rtc_timer(uint8_t len, char *param[])
{
    if (len < 1) {
        printf("Usage: set_timer [period]\r\n");
        return 1;
    }

    int period = atoi(param[0]);

    hal_rtc_status_t ret =  hal_rtc_init();
    rtc_set_timer(period);

    printf("rtc_set_timer: period=%d, ret=%d\r\n", period, ret);

    return 0;
}
static uint8_t _cli_lp_dvt_rtc_set_data(uint8_t len, char *param[])
{
    if (len < 1) {
        printf("Usage: set_data [offset]\r\n");
        return 1;
    }

    char data = 0x5a;
    int offset = atoi(param[0]);
    hal_rtc_set_data(offset, &data, 1);
    printf("hal_rtc_set_data: offset=%d, data=0x%x\r\n", offset, data);

    return 0;
}

static uint8_t _cli_lp_dvt_rtc_mode(uint8_t len, char *param[])
{
    hal_rtc_status_t ret = hal_rtc_enter_rtc_mode();
    printf("hal_rtc_enter_rtc_mode: ret=%d\r\n", ret);

    return 0;
}

static uint8_t _cli_lp_dvt_rtc_get_data(uint8_t len, char *param[])
{
    if (len < 1) {
        printf("Usage: get_data [offset]\r\n");
        return 1;
    }

    char data = 0xa5;
    int offset = atoi(param[0]);
    hal_rtc_get_data(offset, &data, 1);
    printf("hal_rtc_get_data: offset=%d, data=0x%x\r\n", offset, data);

    return 0;
}


cmd_t rtc_lp_dvt_cli_cmds[] = {
    { "set_timer",       "set rtc timer",                       _cli_lp_dvt_rtc_timer, NULL},
    { "set_data",        "set rtc data",                        _cli_lp_dvt_rtc_set_data, NULL},
    { "rtc_mode",        "start to enter sleep mode",           _cli_lp_dvt_rtc_mode, NULL},
    { "get_data",        "get rtc data",                        _cli_lp_dvt_rtc_get_data, NULL},
    { NULL, NULL, NULL, NULL }
};
#endif /* MTK_LP_DVT_CLI_ENABLE */

