/* Copyright Statement:
 *
 * (C) 2005-2020  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek Inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE.
 */
#include <common.h>
#include "hal_nvic.h"
#include "hal_nvic_internal.h"
#include "hal_wdt.h"
#include "hal_wdt_internal.h"
#include "hal_log.h"

#ifdef HAL_WDT_MODULE_ENABLED

static hal_wdt_callback_t   s_hal_wdt_callback = NULL;
static bool                 s_hal_enable = false;
static hal_wdt_mode_t       s_hal_mode = HAL_WDT_MODE_RESET;

static void hal_wdt_isr(void)
{
    uint32_t return_status;
    if (s_hal_wdt_callback) {
        return_status = wdt_get_status();
        if (0 != (return_status & (0x1 << WDT_SW_STA_SW_WDT_OFFSET))) {
            s_hal_wdt_callback(HAL_WDT_SOFTWARE_RESET);
        } else if (0 != (return_status & (0x1 << WDT_SW_STA_WDT_OFFSET))) {
            s_hal_wdt_callback(HAL_WDT_TIMEOUT_RESET);
        } else {
            s_hal_wdt_callback(HAL_WDT_NONE_RESET);
        }
    }
}

void wdt_default_callback(hal_wdt_reset_status_t wdt_reset_status)
{
    log_hal_warning("WDT Interrupt triggered\r\n");
}

hal_wdt_status_t hal_wdt_init(hal_wdt_config_t *config)
{
    if (!config) {
        return HAL_WDT_STATUS_INVALID_PARAMETER;
    }

    if (config->mode != HAL_WDT_MODE_RESET &&
        config->mode != HAL_WDT_MODE_INTERRUPT) {
        return HAL_WDT_STATUS_INVALID_PARAMETER;
    }

    s_hal_mode = (config->mode);
    wdt_set_mode((config->mode));
    /* wdt time-out period is a multiple of 1024* T32k=32ms*(TIMEOUT + 1) if T32K is ideal.*/
    wdt_set_counter((((config->seconds) * 1000) / 32) - 1);
    wdt_set_interval(WDT_INTERVAL_MIN);

    // Set IRQ Priority
    hal_nvic_set_priority(WDT_B0_IRQn, WDT_IRQ_PRIORITY);
    // Register ISR Handler for IRQn
    hal_nvic_register_isr_handler(WDT_B0_IRQn, (hal_nvic_isr_t)hal_wdt_isr);
    // Enable IRQn Priority
    hal_nvic_enable_irq(WDT_B0_IRQn);
    s_hal_wdt_callback = (hal_wdt_callback_t)wdt_default_callback;
    wdt_sw_set_hw_reboot();
    wdt_sw_clr_sw_reboot();
    return HAL_WDT_STATUS_OK;
}

hal_wdt_status_t hal_wdt_deinit(void)
{
    /* disable wdt */
    wdt_set_enable(0);
    s_hal_enable = false;
    s_hal_wdt_callback = NULL;
    return HAL_WDT_STATUS_OK;
}


hal_wdt_status_t hal_wdt_enable(uint32_t magic)
{
    if (magic != HAL_WDT_ENABLE_MAGIC) {
        return HAL_WDT_STATUS_INVALID_MAGIC;
    }

    s_hal_enable = true;
    wdt_set_enable(1);
    wdt_restart();
    return HAL_WDT_STATUS_OK;
}


hal_wdt_status_t hal_wdt_disable(uint32_t magic)
{
    if (magic != HAL_WDT_DISABLE_MAGIC) {
        return HAL_WDT_STATUS_INVALID_MAGIC;
    }

    s_hal_enable = false;
    wdt_set_enable(0);
    return HAL_WDT_STATUS_OK;
}


hal_wdt_status_t hal_wdt_feed(uint32_t magic)
{
    if (magic != HAL_WDT_FEED_MAGIC) {
        return HAL_WDT_STATUS_INVALID_MAGIC;
    }
    wdt_restart();
    /* Wait for 4T 32k cycle for HW limitation */
    //hal_gpt_delay_us(123);

    return HAL_WDT_STATUS_OK;
}


hal_wdt_callback_t hal_wdt_register_callback(const hal_wdt_callback_t wdt_callback)
{
    hal_wdt_callback_t current_wdt_callback;

    current_wdt_callback = s_hal_wdt_callback;
    if (wdt_callback)
        s_hal_wdt_callback = wdt_callback;

    if (current_wdt_callback == (hal_wdt_callback_t)wdt_default_callback)
        return NULL;

    return current_wdt_callback;
}

hal_wdt_status_t hal_wdt_software_reset(void)
{
    wdt_swrst();
    return HAL_WDT_STATUS_OK;
}


bool hal_wdt_get_enable_status(void)
{
    return s_hal_enable;
}


hal_wdt_mode_t hal_wdt_get_mode(void)
{
    return s_hal_mode;
}

void hal_wdt_set_mode(hal_wdt_mode_t mode)
{
    s_hal_mode = mode;
    wdt_set_mode(mode);
}

hal_wdt_reset_status_t hal_wdt_get_reset_status(void)
{
    uint32_t return_status;
    return_status = wdt_get_status();
    if (0 != (return_status & (0x1 << WDT_SW_STA_SW_WDT_OFFSET))) {
        return HAL_WDT_SOFTWARE_RESET;
    } else if (0 != (return_status & (0x1 << WDT_SW_STA_WDT_OFFSET))) {
        return HAL_WDT_TIMEOUT_RESET;
    }
    return HAL_WDT_NONE_RESET;
}

hal_wdt_status_t hal_wdt_get_retn_data5_reg(uint32_t *value)
{
    if (value == NULL) {
        return HAL_WDT_STATUS_INVALID_PARAMETER;
    }
    *value = 0;
    return HAL_WDT_STATUS_OK;
}

hal_wdt_status_t hal_wdt_set_retn_data5_reg(uint32_t value)
{
    return HAL_WDT_STATUS_OK;
}

#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
