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
#include "ut.h"
#include "hal_gpt.h"

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_ADC_MODULE_ENABLE)
extern void vTaskDelay(const uint16_t);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void wakeup_gpt_callback(void *user_data)
{
    hal_gpt_stop_timer(HAL_GPT_1);
}

ut_status_t ut_hal_adc(void)
{
    uint32_t val, voltage;
    int ret, channel;

    printf("ut_hal_adc beign.\r\n");
    ret = hal_adc_init();
    if (ret < 0)
        printf("hal_adc_init fail.\r\n");
    printf("hal_adc_init pass.\r\n");

    hal_gpt_init(HAL_GPT_1);
    hal_gpt_register_callback(HAL_GPT_1, wakeup_gpt_callback, NULL);
    hal_gpt_stop_timer(HAL_GPT_1);

    hal_gpt_start_timer_ms(HAL_GPT_1, 500, HAL_GPT_TIMER_TYPE_ONE_SHOT);
    vTaskDelay(50);
    ret = hal_sleep_manager_enter_sleep_mode(HAL_SLEEP_MODE_SLEEP);

    for (channel = 0; channel < HAL_ADC_CHANNEL_MAX; channel++) {
        hal_gpio_set_analog(17 + channel, true);
        ret = hal_adc_get_data_polling(channel, &val);
        if (ret < 0)
            printf("hal_adc_get_data_polling fail.\r\n");
        voltage = val * 1800 / 4096;
        printf("hal_adc_get_data_polling pass. channel(%d), val(0x%lx), voltage(%ld mv)\r\n", channel, val, voltage);
        hal_gpio_set_analog(17 + channel, false);
    }

    hal_gpt_deinit(HAL_GPT_1);

    ret = hal_adc_deinit();
    if (ret < 0)
        printf("hal_adc_deinit fail.\r\n");
    printf("hal_adc_deinit pass.\r\n");

    printf("ut_hal_adc end.\r\n");
    return UT_STATUS_OK;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_ADC_MODULE_ENABLE) */
