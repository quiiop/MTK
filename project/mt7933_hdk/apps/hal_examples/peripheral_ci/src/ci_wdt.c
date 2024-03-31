/* Copyright Statement:
 *
 * (C) 2005-2030  MediaTek Inc. All rights reserved.
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
#include <stdio.h>
#include <string.h>
#include "ci.h"
#include "ci_cli.h"
#include "hal.h"
#include "hal_wdt.h"

hal_wdt_reset_status_t wdt_ut_callback_status = HAL_WDT_NONE_RESET;

void wdt_ci_callback(hal_wdt_reset_status_t wdt_reset_status)
{
    wdt_ut_callback_status = wdt_reset_status;
    printf("enter wdt_ci_callback, status=%d\n", wdt_ut_callback_status);
}

void wdt_ci2_callback(hal_wdt_reset_status_t wdt_reset_status)
{
    wdt_ut_callback_status = wdt_reset_status;
    printf("enter wdt_ci2_callback, status=%d\n", wdt_ut_callback_status);
}

ci_status_t ci_wdt_feed_timeout_reset_irq_callback_sample(void)
{
    EXPECT_VAL(hal_wdt_deinit(), HAL_WDT_STATUS_OK);

    hal_wdt_config_t cfg = {0};
    cfg.mode = HAL_WDT_MODE_INTERRUPT;
    cfg.seconds = 1;
    EXPECT_VAL(hal_wdt_init(&cfg), HAL_WDT_STATUS_OK);

    EXPECT_VAL(hal_wdt_register_callback(wdt_ci_callback), NULL);
    EXPECT_VAL(hal_wdt_register_callback(wdt_ci2_callback), wdt_ci_callback);

    wdt_ut_callback_status = HAL_WDT_NONE_RESET;
    EXPECT_VAL(hal_wdt_enable(HAL_WDT_ENABLE_MAGIC), HAL_WDT_STATUS_OK);

    hal_gpt_delay_ms(600);
    EXPECT_VAL(wdt_ut_callback_status, HAL_WDT_NONE_RESET);

    EXPECT_VAL(hal_wdt_feed(HAL_WDT_FEED_MAGIC), HAL_WDT_STATUS_OK);
    EXPECT_VAL(hal_wdt_feed(0), HAL_WDT_STATUS_INVALID_MAGIC);

    hal_gpt_delay_ms(300);
    EXPECT_VAL(wdt_ut_callback_status, HAL_WDT_NONE_RESET);

    EXPECT_VAL(hal_wdt_feed(HAL_WDT_FEED_MAGIC), HAL_WDT_STATUS_OK);

    hal_gpt_delay_ms(600);
    EXPECT_VAL(wdt_ut_callback_status, HAL_WDT_NONE_RESET);

    hal_gpt_delay_ms(410);
    EXPECT_VAL(wdt_ut_callback_status, HAL_WDT_TIMEOUT_RESET);
    EXPECT_VAL(hal_wdt_get_reset_status(), HAL_WDT_TIMEOUT_RESET);

    EXPECT_VAL(hal_wdt_deinit(), HAL_WDT_STATUS_OK);

    return CI_PASS;
}

ci_status_t ci_wdt_sw_rst_sample(void)
{
    EXPECT_VAL(hal_wdt_deinit(), HAL_WDT_STATUS_OK);

    wdt_ut_callback_status = HAL_WDT_NONE_RESET;

    hal_wdt_config_t cfg = {0};
    cfg.mode = HAL_WDT_MODE_INTERRUPT;
    cfg.seconds = 10;
    EXPECT_VAL(hal_wdt_init(&cfg), HAL_WDT_STATUS_OK);

    EXPECT_VAL(hal_wdt_register_callback(wdt_ci_callback), NULL);

    EXPECT_VAL(wdt_ut_callback_status, HAL_WDT_NONE_RESET);

    EXPECT_VAL(hal_wdt_software_reset(), HAL_WDT_STATUS_OK);

    hal_gpt_delay_ms(10);
    EXPECT_VAL(wdt_ut_callback_status, HAL_WDT_SOFTWARE_RESET);

    EXPECT_VAL(hal_wdt_get_reset_status(), HAL_WDT_SOFTWARE_RESET);

    EXPECT_VAL(hal_wdt_deinit(), HAL_WDT_STATUS_OK);

    return CI_PASS;
}



ci_status_t ci_wdt_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Sample Code: WDT feed and interrupt mode timeout", ci_wdt_feed_timeout_reset_irq_callback_sample},
        {"Sample Code: WDT interrupt mode software Reset", ci_wdt_sw_rst_sample},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
