/*
 * (C) 2005-2020 MediaTek Inc. All rights reserved.
 *
 * Copyright Statement:
 *
 * This MT3620 driver software/firmware and related documentation
 * ("MediaTek Software") are protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. ("MediaTek"). You may only use, reproduce, modify, or
 * distribute (as applicable) MediaTek Software if you have agreed to and been
 * bound by this Statement and the applicable license agreement with MediaTek
 * ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User"). If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
 * PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS
 * ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO
 * LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED
 * HEREUNDER WILL BE ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
 * RECEIVER TO MEDIATEK DURING THE PRECEDING TWELVE (12) MONTHS FOR SUCH
 * MEDIATEK SOFTWARE AT ISSUE.
 */

#include "hal_platform.h"

#ifdef HAL_ADC_MODULE_ENABLED
#include "os_hal_adc.h"
#include "hal_adc.h"
#include "hal_gpio.h"
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#include "hal_clock.h"

hal_adc_status_t hal_adc_init(void)
{
    hal_clock_enable((hal_clock_cg_id)HAL_CLOCK_CG_AUX_ADC_DIV);
    hal_clock_mux_select(HAL_CLOCK_SEL_AUX_ADC_DIV, CLK_AUX_ADC_DIV_CLKSEL_XTAL_DIV_2M);

    return HAL_ADC_STATUS_OK;
}

hal_adc_status_t hal_adc_deinit(void)
{
    hal_clock_disable((hal_clock_cg_id)HAL_CLOCK_CG_AUX_ADC_DIV);

    return HAL_ADC_STATUS_OK;
}

hal_adc_status_t hal_adc_get_data_polling(hal_adc_channel_t channel, uint32_t *data)
{
    hal_adc_status_t ret;
    u32 adc_analog_pins[ADC_CHANNEL_MAX] = { 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 };

    if (channel >= HAL_ADC_CHANNEL_MAX)
        return HAL_ADC_STATUS_ERROR_CHANNEL;

    if (data == NULL)
        return HAL_ADC_STATUS_INVALID_PARAMETER;

    hal_gpio_set_analog(adc_analog_pins[channel], true);

    mtk_os_hal_adc_ctlr_init(ADC_PMODE_ONE_TIME,
                             ADC_FIFO_DIRECT, 1 << channel);

    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_ADC);

    mtk_os_hal_adc_start_ch(1 << channel);

    ret = mtk_os_hal_adc_one_shot_get_data((adc_channel)channel, (u32 *) data);
    if (ret < 0) {
        printf("mtk_os_hal_adc_one_shot_get_data fail.\n");
        return ret;
    }

    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_ADC);

    mtk_os_hal_adc_ctlr_deinit();

    return ret;
}

#endif /* #ifdef HAL_ADC_MODULE_ENABLED */
