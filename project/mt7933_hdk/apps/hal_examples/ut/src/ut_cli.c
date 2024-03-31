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

#if defined(MTK_MINICLI_ENABLE) && defined(MTK_UT_ENABLE)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ut.h"

/****************************************************************************
 *
 * Constants.
 *
 ****************************************************************************/


/****************************************************************************
 *
 * Types.
 *
 ****************************************************************************/


/****************************************************************************
 *
 * Static variables.
 *
 ****************************************************************************/


/****************************************************************************
 *
 * Local functions.
 *
 ****************************************************************************/

#if defined(UT_RTOS_ENABLE)
static uint8_t _ut_cli_rtos(uint8_t len, char *param[])
{
    ut_status_t ret = UT_STATUS_NOT_SUPPORT;

    printf("RTOS UT Start...\r\n");
    ret = ut_rtos();
    if (ret != UT_STATUS_OK) {
        printf("RTOS UT Fail!\r\n");
        return 0;
    }

    printf("RTOS UT OK...\r\n");

    return 0;
}
#endif /* #if defined(UT_RTOS_ENABLE) */

#if defined(UT_HAL_ENABLE)
static uint8_t _ut_cli_hal(uint8_t len, char *param[])
{
    ut_status_t ret = UT_STATUS_NOT_SUPPORT;

    printf("Hal UT Start...\r\n");

    ret = ut_hal();
    if (ret != UT_STATUS_OK) {
        printf("Hal UT Fail!\r\n");
        return 0;
    }

    printf("Hal UT OK...\r\n");

    return 0;
}
#endif /* #if defined(UT_HAL_ENABLE) */

#if defined(UT_PLATFORM_ENABLE)
static uint8_t _ut_cli_platform(uint8_t len, char *param[])
{
    ut_status_t ret = UT_STATUS_NOT_SUPPORT;

    printf("Platform UT Start...\r\n");

    ret = ut_platform();
    if (ret != UT_STATUS_OK) {
        printf("Platform UT Fail!\r\n");
        return 0;
    }

    printf("Platform UT OK...\r\n");

    return 0;
}
#endif /* #if defined(UT_PLATFORM_ENABLE) */



/****************************************************************************
 *
 * API variable.
 *
 ****************************************************************************/


/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/

cmd_t ut_cli[] = {
#ifdef UT_RTOS_ENABLE
    { "os",   "rtos ut command", _ut_cli_rtos, NULL },
#endif /* #ifdef UT_RTOS_ENABLE */
#ifdef UT_HAL_ENABLE
    { "hal",   "hal ut command", _ut_cli_hal, NULL },
#endif /* #ifdef UT_HAL_ENABLE */
#ifdef UT_PLATFORM_ENABLE
    { "plat",   "platform ut command", _ut_cli_platform, NULL },
#endif /* #ifdef UT_PLATFORM_ENABLE */
    { NULL }
};

#endif /* #if defined(MTK_MINICLI_ENABLE) && defined(MTK_UT_ENABLE) */

