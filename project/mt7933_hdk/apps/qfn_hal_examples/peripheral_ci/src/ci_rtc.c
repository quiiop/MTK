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
#include "hal_rtc.h"

#define BASE_YEAR 2000
ci_status_t set_current_time(uint16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
    hal_rtc_time_t time;

    // The user has to define the base year and the RTC year is defined
    // as an offset. For example, define the base year as 2000 and assign 15 to the RTC year to represent the year of 2015.
    time.rtc_year = year - BASE_YEAR;
    time.rtc_mon = mon;
    time.rtc_day = day;
    time.rtc_hour = hour;
    time.rtc_min = min;
    time.rtc_sec = sec;

    // Set the RTC current time.
    if (HAL_RTC_STATUS_OK != hal_rtc_set_time(&time)) {
        // Error handler
        return CI_FAIL;
    }
    return CI_PASS;
}

ci_status_t get_current_time(uint16_t *year, uint8_t *mon, uint8_t *day, uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    hal_rtc_time_t time;

    if (HAL_RTC_STATUS_OK != hal_rtc_get_time(&time)) {
        // Error handler
        return CI_FAIL;
    }
    // The user has to define the base year and the RTC year is defined
    // as an offset. For example, define the base year as 2000 and assign 15 to the RTC year to represent the year of 2015.
    *year = time.rtc_year + BASE_YEAR;
    *mon = time.rtc_mon;
    *day = time.rtc_day;
    *hour = time.rtc_hour;
    *min = time.rtc_min;
    *sec = time.rtc_sec;

    return CI_PASS;
}

ci_status_t set_alarm_time(uint16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
    hal_rtc_time_t alarm;

    // The user has to define the base year and the RTC year is defined
    // as an offset. For example, define the base year as 2000 and assign 15 to the RTC year to represent the year of 2015.
    alarm.rtc_year = year - BASE_YEAR;
    alarm.rtc_mon = mon;
    alarm.rtc_day = day;
    alarm.rtc_hour = hour;
    alarm.rtc_min = min;
    alarm.rtc_sec = sec;

    // Set the RTC alarm time.
    if (HAL_RTC_STATUS_OK != hal_rtc_set_alarm(&alarm)) {
        // Error handler
        return CI_FAIL;
    }
    if (HAL_RTC_STATUS_OK != hal_rtc_enable_alarm()) {
        // Error handler
        return CI_FAIL;
    }

    return CI_PASS;
}

ci_status_t get_alarm_time(uint16_t *year, uint8_t *mon, uint8_t *day, uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    hal_rtc_time_t alarm;

    if (HAL_RTC_STATUS_OK != hal_rtc_get_alarm(&alarm)) {
        // Error handler
        return CI_FAIL;
    }
    // The user has to define the base year and the RTC year is defined
    // as an offset. For example, define the base year as 2000 and assign 15 to the RTC year to represent the year of 2015.
    *year = alarm.rtc_year + BASE_YEAR;
    *mon = alarm.rtc_mon;
    *day = alarm.rtc_day;
    *hour = alarm.rtc_hour;
    *min = alarm.rtc_min;
    *sec = alarm.rtc_sec;

    return CI_PASS;
}


bool g_rtc_alarm_triggered = false;

void alarm_handle_cb(void *user_data)

{
    // Apply the RTC functionality, for example, get the RTC current time and update it on the user interface.
    // Please note, that this callback runs in an interrupt service routine.
    printf("rtc alarm_handle_cb triggered\n");
    g_rtc_alarm_triggered = true;
}

ci_status_t set_alarm_handle_cb(hal_rtc_alarm_callback_t alarm_handle_cb)
{
    // Register a callback function to handle the RTC alarm.
    if (HAL_RTC_STATUS_OK != hal_rtc_set_alarm_callback(alarm_handle_cb, NULL)) {
        // Error handler
        return CI_FAIL;
    }
    // Enable an alarm notification.
    if (HAL_RTC_STATUS_OK != hal_rtc_enable_alarm()) {
        // Error handler
        return CI_FAIL;
    }

    return CI_PASS;
}

ci_status_t ci_rtc_get_set_current_time_sample(void)
{
    uint16_t year;
    uint8_t mon, day, hour, min, sec;

    EXPECT_VAL(hal_rtc_init(), HAL_RTC_STATUS_OK);

    EXPECT_VAL(set_current_time(2022, 12, 31, 23, 59, 59), CI_PASS);

    EXPECT_VAL(get_current_time(&year, &mon, &day, &hour, &min, &sec), CI_PASS);

    EXPECT_VAL(year, 2022);
    EXPECT_VAL(mon, 12);
    EXPECT_VAL(day, 31);
    EXPECT_VAL(hour, 23);
    EXPECT_VAL(min, 59);
    EXPECT_VAL(sec, 59);

    vTaskDelay(1100);

    EXPECT_VAL(get_current_time(&year, &mon, &day, &hour, &min, &sec), CI_PASS);

    EXPECT_VAL(year, 2023);
    EXPECT_VAL(mon, 01);
    EXPECT_VAL(day, 01);
    EXPECT_VAL(hour, 00);
    EXPECT_VAL(min, 00);
    EXPECT_VAL(sec, 00);

    EXPECT_VAL(hal_rtc_deinit(), HAL_RTC_STATUS_OK);

    return CI_PASS;
}

ci_status_t ci_rtc_alarm_time_sample(void)
{
    uint16_t year;
    uint8_t mon, day, hour, min, sec;

    EXPECT_VAL(hal_rtc_init(), HAL_RTC_STATUS_OK);

    g_rtc_alarm_triggered = false;
    EXPECT_VAL(set_alarm_handle_cb(alarm_handle_cb), CI_PASS);
    EXPECT_VAL(set_current_time(2022, 05, 01, 10, 00, 00), CI_PASS);
    EXPECT_VAL(set_alarm_time(2022, 05, 01, 10, 00, 01), CI_PASS);

    EXPECT_VAL(g_rtc_alarm_triggered, false);
    vTaskDelay(2100);
    EXPECT_VAL(g_rtc_alarm_triggered, true);

    EXPECT_VAL(get_alarm_time(&year, &mon, &day, &hour, &min, &sec), CI_PASS);

    EXPECT_VAL(year, 2022);
    EXPECT_VAL(mon, 05);
    EXPECT_VAL(day, 01);
    EXPECT_VAL(hour, 10);
    EXPECT_VAL(min, 00);
    EXPECT_VAL(sec, 01);

    EXPECT_VAL(hal_rtc_deinit(), HAL_RTC_STATUS_OK);

    return CI_PASS;
}

ci_status_t ci_rtc_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Sample Code: Get/set RTC current time", ci_rtc_get_set_current_time_sample},
        {"Sample Code: RTC set alarm", ci_rtc_alarm_time_sample},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
