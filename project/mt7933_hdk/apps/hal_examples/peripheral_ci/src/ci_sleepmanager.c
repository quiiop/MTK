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
#include "hal_sleep_manager.h"

#define EXPECT_VAL_MASK(_val, _mask, _expected) \
    { \
        unsigned int __val = (unsigned int)(_val); \
        unsigned int __expect = (unsigned int)(_expected); \
        if ((__val & (_mask)) != __expect) { \
            printf("FUNC:%s, LINE:%d, val:%08x, mask:%08x, expect:%08x\n", __func__, __LINE__, __val, (_mask), __expect); \
            return CI_FAIL; \
        } \
    }

#define EXPECT_VAL(_val, _expected) \
        { \
            unsigned int __val = (unsigned int)(_val); \
            unsigned int __expect = (unsigned int)(_expected); \
            if (__val != __expect) { \
                printf("FUNC:%s, LINE:%d, val:%08x, expect:%08x\n", __func__, __LINE__, __val, __expect); \
                return CI_FAIL; \
            } \
        }

#define EXPECT_NOT_VAL(_val, _not_expected) \
        { \
            unsigned int __val = (unsigned int)(_val); \
            unsigned int __not_expect = (unsigned int)(_not_expected); \
            if (__val == __not_expect) { \
                printf("FUNC:%s, LINE:%d, val:%08x, notexpect:%08x\n", __func__, __LINE__, __val, __not_expect); \
                return CI_FAIL; \
            } \
        }

void *ci_sleepmanager_suspend_callback_data = NULL;
void *ci_sleepmanager_resume_callback_data = NULL;

#define SUSPEND_CALLBACK_TEST_DATA_PTR ((void *)(0x12345678))
#define RESUME_CALLBACK_TEST_DATA_PTR ((void *)(0x55AA55AA))

void ci_sleepmanager_suspend_callback(void *data)
{
    ci_sleepmanager_suspend_callback_data = data;
}

void ci_sleepmanager_resume_callback(void *data)
{
    ci_sleepmanager_resume_callback_data = data;
}

ci_status_t ci_sleepmanager_sleep_lock_and_deep_sleep(void)
{
    EXPECT_VAL(hal_sleep_manager_init(), HAL_SLEEP_MANAGER_OK);

    hal_sleep_manager_register_suspend_callback(ci_sleepmanager_suspend_callback, SUSPEND_CALLBACK_TEST_DATA_PTR);
    hal_sleep_manager_register_resume_callback(ci_sleepmanager_resume_callback, RESUME_CALLBACK_TEST_DATA_PTR);
    ci_sleepmanager_suspend_callback_data = NULL;
    ci_sleepmanager_resume_callback_data = NULL;

    uint8_t handle_index = hal_sleep_manager_set_sleep_handle("test_lock");
    EXPECT_NOT_VAL(handle_index, 0xFF);

    EXPECT_VAL(hal_sleep_manager_is_sleep_handle_alive(handle_index), false);

    EXPECT_VAL(hal_sleep_manager_lock_sleep(handle_index), HAL_SLEEP_MANAGER_OK);
    EXPECT_VAL_MASK(hal_sleep_manager_get_lock_status(), BIT(handle_index), BIT(handle_index));
    EXPECT_VAL(hal_sleep_manager_is_sleep_handle_alive(handle_index), true);

    EXPECT_VAL(hal_sleep_manager_is_sleep_locked(HAL_SLEEP_MODE_SLEEP), true);

    // yield for uart message flush prior test
    vTaskDelay(200);

    uint32_t ret = hal_sleep_manager_enter_sleep_mode(HAL_SLEEP_MODE_SLEEP);

    if (ret < 0x10000000) {
        // always compare fail due to unexpected enter sleep
        EXPECT_VAL(ret, 0xFFFFFFFF);
    }
    EXPECT_VAL(ci_sleepmanager_suspend_callback_data, NULL);
    EXPECT_VAL(ci_sleepmanager_resume_callback_data, NULL);

    EXPECT_VAL(hal_sleep_manager_unlock_sleep(handle_index), HAL_SLEEP_MANAGER_OK);
    EXPECT_VAL_MASK(hal_sleep_manager_get_lock_status(), BIT(handle_index), 0);
    EXPECT_VAL(hal_sleep_manager_is_sleep_handle_alive(handle_index), false);

    // yield for uart message flush prior test
    vTaskDelay(200);

    EXPECT_VAL(hal_sleep_manager_set_sleep_time(300), HAL_SLEEP_MANAGER_OK);

    ret = hal_sleep_manager_enter_sleep_mode(HAL_SLEEP_MODE_SLEEP);
    if (ret >= 0x10000000) {
        // always compare fail due to unexpected not enter sleep
        EXPECT_VAL(ret, 0);
    }
    EXPECT_VAL(ci_sleepmanager_suspend_callback_data, SUSPEND_CALLBACK_TEST_DATA_PTR);
    EXPECT_VAL(ci_sleepmanager_resume_callback_data, RESUME_CALLBACK_TEST_DATA_PTR);

    EXPECT_VAL(hal_sleep_manager_release_sleep_handle(handle_index), HAL_SLEEP_MANAGER_OK);

    return CI_PASS;
}



ci_status_t ci_sleepmanager_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Sample Code: Sleep lock allocate & deep sleep test", ci_sleepmanager_sleep_lock_and_deep_sleep},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}

