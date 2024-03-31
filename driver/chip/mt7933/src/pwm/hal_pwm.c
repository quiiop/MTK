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

#include "mhal_pwm.h"
#include "hal_clk.h"
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"

#ifdef HAL_PWM_MODULE_ENABLED

#define GROUP0_PWM_BASE         0x30041000
#define GROUP1_PWM_BASE         0x30051000
#define GROUP2_PWM_BASE         0x30061000

#define TOP_PWM_CLK_DIV_BASE        0x3002021C

const unsigned long pwm_base_addr[MAX_GROUP_NUM] = {
    GROUP0_PWM_BASE,
    GROUP1_PWM_BASE,
    GROUP2_PWM_BASE,
};

static const pwm_clks pwm_clock_source[MAX_GROUP_NUM] = {
    PWM_CLOCK_32K,
    PWM_CLOCK_2M,
    PWM_CLOCK_XTAL
};

static const unsigned long group0_register[MAX_CHANNEL_NUM] = {
    0x100, 0x110, 0x120, 0x130
};

static const unsigned long group1_register[MAX_CHANNEL_NUM] = {
    0x100, 0x110, 0x120, 0x130
};

static const unsigned long group2_register[MAX_CHANNEL_NUM] = {
    0x100, 0x110, 0x120, 0x130
};

static struct mtk_com_pwm_data group0_pwm_data = {
    .pwm_register = &group0_register[0],
    .pwm_nums = 4,
    .index = 0,
};
static struct mtk_com_pwm_data group1_pwm_data = {
    .pwm_register = &group1_register[0],
    .pwm_nums = 4,
    .index = 4,
};
static struct mtk_com_pwm_data group2_pwm_data = {
    .pwm_register = &group2_register[0],
    .pwm_nums = 4,
    .index = 8,
};

struct mtk_com_pwm_data *pwm_common_data[MAX_GROUP_NUM] = {
    &group0_pwm_data, &group1_pwm_data, &group2_pwm_data
};

static struct mtk_pwm_controller g_pwm_controller[MAX_GROUP_NUM];

struct mtk_pwm_controller_rtos {
    struct mtk_pwm_controller *ctlr;
};

static struct mtk_pwm_controller_rtos g_pwm_ctlr_rtos[MAX_GROUP_NUM];

static struct mtk_pwm_controller_rtos *_mtk_os_hal_pwm_get_ctlr(
    pwm_groups group_num)
{
    return &g_pwm_ctlr_rtos[group_num];
}

static hal_pwm_status_t _hal_pwm_clock_enabe(pwm_groups group_num)
{
    switch (group_num) {
        case PWM_GROUP0:
            if (hal_clock_is_enabled(HAL_CLOCK_CG_PWM0_XTAL) != true) {
                if (HAL_CLOCK_STATUS_OK != hal_clock_enable(HAL_CLOCK_CG_PWM0_XTAL)) {
                    return -PWM_ECLK;
                }
            }
            break;
        case PWM_GROUP1:
            if (hal_clock_is_enabled(HAL_CLOCK_CG_PWM1_XTAL) != true) {
                if (HAL_CLOCK_STATUS_OK != hal_clock_enable(HAL_CLOCK_CG_PWM1_XTAL)) {
                    return -PWM_ECLK;
                }
            }
            break;
        case PWM_GROUP2:
            if (hal_clock_is_enabled(HAL_CLOCK_CG_PWM2_XTAL) != true) {
                if (HAL_CLOCK_STATUS_OK != hal_clock_enable(HAL_CLOCK_CG_PWM2_XTAL)) {
                    return -PWM_ECLK;
                }
            }
            break;
        default:
            if (hal_clock_is_enabled(HAL_CLOCK_CG_PWM0_XTAL) != true) {
                if (HAL_CLOCK_STATUS_OK != hal_clock_enable(HAL_CLOCK_CG_PWM0_XTAL)) {
                    return -PWM_ECLK;
                }
            }

            if (hal_clock_is_enabled(HAL_CLOCK_CG_PWM1_XTAL) != true) {
                if (HAL_CLOCK_STATUS_OK != hal_clock_enable(HAL_CLOCK_CG_PWM1_XTAL)) {
                    return -PWM_ECLK;
                }
            }

            if (hal_clock_is_enabled(HAL_CLOCK_CG_PWM2_XTAL) != true) {
                if (HAL_CLOCK_STATUS_OK != hal_clock_enable(HAL_CLOCK_CG_PWM2_XTAL)) {
                    return -PWM_ECLK;
                }
            }
            break;
    }

    return 0;
}

static hal_pwm_status_t  _hal_pwm_clock_disabe(pwm_groups group_num)
{
    switch (group_num) {
        case PWM_GROUP0:
            if (hal_clock_is_enabled(HAL_CLOCK_CG_PWM0_XTAL) == true) {
                if (HAL_CLOCK_STATUS_OK != hal_clock_disable(HAL_CLOCK_CG_PWM0_XTAL)) {
                    return -PWM_ECLK;
                }
            }
            break;
        case PWM_GROUP1:
            if (hal_clock_is_enabled(HAL_CLOCK_CG_PWM1_XTAL) == true) {
                if (HAL_CLOCK_STATUS_OK != hal_clock_disable(HAL_CLOCK_CG_PWM1_XTAL)) {
                    return -PWM_ECLK;
                }
            }
            break;
        case PWM_GROUP2:
            if (hal_clock_is_enabled(HAL_CLOCK_CG_PWM2_XTAL) == true) {
                if (HAL_CLOCK_STATUS_OK != hal_clock_disable(HAL_CLOCK_CG_PWM2_XTAL)) {
                    return -PWM_ECLK;
                }
            }
            break;
        default:
            break;
    }

    return 0;
}

#ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK
hal_pwm_status_t hal_pwm_init(hal_pwm_source_clock_t source_clock)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    struct mtk_pwm_controller *ctlr = NULL;
    pwm_groups group_num = PWM_GROUP0;
    uint32_t channel_num = 0;
    uint32_t bit_map = PWM_0_BIT | PWM_1_BIT | PWM_2_BIT | PWM_3_BIT;
    int ret = 0;

    for (group_num = PWM_GROUP0; group_num < PWM_GROUP_MAX;
         group_num++) {
        _hal_pwm_clock_enabe(group_num);
        ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);
        if (!ctlr_rtos)
            return -PWM_EPTR;

        ctlr_rtos->ctlr = &g_pwm_controller[group_num];

        if (!ctlr_rtos->ctlr)
            return -PWM_EPTR;

        ctlr = ctlr_rtos->ctlr;
        ctlr->base = (void __iomem *)pwm_base_addr[group_num];
        ctlr->top_clk_base = (void __iomem *)TOP_PWM_CLK_DIV_BASE;
        ctlr->group_number = group_num;
        switch (source_clock) {
            case HAL_PWM_CLOCK_32KHZ:
                ctlr->group_clock = PWM_CLOCK_32K;
                break;
            case HAL_PWM_CLOCK_2MHZ:
                ctlr->group_clock = PWM_CLOCK_2M;
                break;
            case HAL_PWM_CLOCK_26MHZ:
                ctlr->group_clock = PWM_CLOCK_XTAL;
                break;
            default:
                ctlr->group_clock = pwm_clock_source[group_num];
                break;
        }

        ctlr->data = pwm_common_data[group_num];

        ret = mtk_mhal_pwm_init(ctlr, bit_map);
        if (ret)
            return ret;

        ctlr->data->global_kick_enable = 0;
        ctlr->data->io_ctrl_sel = 0;
        ctlr->data->polarity_set = 0;
        for (channel_num = 0; channel_num < MAX_CHANNEL_NUM;
             channel_num++) {
            if (bit_map & BIT(channel_num)) {
                ret = mtk_mhal_pwm_feature_enable(ctlr_rtos->ctlr, channel_num);
                if (ret)
                    return ret;
            }
            ctlr->data->running_status[channel_num] = PWM_IDLE;
        }
        ctlr->crtl_status = PWM_INIT;
    }
#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_register_suspend_callback(SLEEP_BACKUP_RESTORE_PWM, (hal_sleep_manager_callback_t)pwm_backup_register_callback, NULL);
    sleep_management_register_resume_callback(SLEEP_BACKUP_RESTORE_PWM, (hal_sleep_manager_callback_t)pwm_restore_register_callback, NULL);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return 0;
}

hal_pwm_status_t hal_pwm_deinit(void)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    struct mtk_pwm_controller *ctlr = NULL;
    pwm_groups group_num = PWM_GROUP0;
    uint32_t bit_map = PWM_0_BIT | PWM_1_BIT | PWM_2_BIT | PWM_3_BIT;
    int ret = 0;

    for (group_num = PWM_GROUP0; group_num < PWM_GROUP_MAX;
         group_num++) {
        ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);
        if (!ctlr_rtos)
            return -PWM_EPTR;
        ctlr = ctlr_rtos->ctlr;
        ctlr->crtl_status = PWM_DEINIT;
        ret = mtk_mhal_pwm_deinit(ctlr, bit_map);
        if (ret)
            return ret;
        _hal_pwm_clock_disabe(group_num);
    }

    return 0;
}
#else /* #ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK */
hal_pwm_status_t hal_pwm_init(hal_pwm_channel_t pwm_num, hal_pwm_source_clock_t source_clock)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    struct mtk_pwm_controller *ctlr = NULL;
    pwm_groups group_num = PWM_GROUP0;
    pwm_channels pwm_index = (pwm_channels)HAL_PWM_0;
    uint32_t bit_map = PWM_0_BIT;
    int ret = 0;

    if (pwm_num >= HAL_PWM_MAX)
        return -PWM_EPTR;

    group_num = (pwm_groups)(pwm_num / 4);
    bit_map = BIT(pwm_num);
    pwm_index = (pwm_channels)(pwm_num % 4);

    ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);
    if (!ctlr_rtos)
        return -PWM_EPTR;

    ctlr_rtos->ctlr = &g_pwm_controller[group_num];

    if (!ctlr_rtos->ctlr)
        return -PWM_EPTR;

    ctlr = ctlr_rtos->ctlr;

    ctlr->base = (void __iomem *)pwm_base_addr[group_num];
    ctlr->top_clk_base = (void __iomem *)TOP_PWM_CLK_DIV_BASE;
    ctlr->group_number = group_num;
    switch (source_clock) {
        case HAL_PWM_CLOCK_32KHZ:
            ctlr->group_clock = PWM_CLOCK_32K;
            break;
        case HAL_PWM_CLOCK_2MHZ:
            ctlr->group_clock = PWM_CLOCK_2M;
            break;
        case HAL_PWM_CLOCK_26MHZ:
            ctlr->group_clock = PWM_CLOCK_XTAL;
            break;
        default:
            ctlr->group_clock = pwm_clock_source[(u8)group_num];
            break;
    }

    ctlr->data = pwm_common_data[(u8)group_num];

    if (ctlr->crtl_status == PWM_DEINIT) {
        _hal_pwm_clock_enabe(group_num);
        ret = mtk_mhal_pwm_init(ctlr, bit_map);
        if (ret)
            return ret;
#ifdef HAL_SLEEP_MANAGER_ENABLED
        sleep_management_register_suspend_callback(SLEEP_BACKUP_RESTORE_PWM, (hal_sleep_manager_callback_t)pwm_backup_register_callback, NULL);
        sleep_management_register_resume_callback(SLEEP_BACKUP_RESTORE_PWM, (hal_sleep_manager_callback_t)pwm_restore_register_callback, NULL);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
        ctlr->crtl_status = PWM_INIT;
    } else
        mtk_mhal_pwm_clock_select(ctlr);

    ctlr->data->global_kick_enable = 0;
    ctlr->data->io_ctrl_sel = 0;
    ctlr->data->polarity_set = 0;
    ctlr->data->running_status[pwm_index] = PWM_IDLE;
    ret = mtk_mhal_pwm_feature_enable(ctlr_rtos->ctlr, pwm_index);
    if (ret)
        return ret;

    return 0;
}
hal_pwm_status_t hal_pwm_deinit(hal_pwm_channel_t pwm_num)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    struct mtk_pwm_controller *ctlr = NULL;
    pwm_groups group_num = PWM_GROUP0;
    int ret = 0;
    uint32_t bit_map = 0;

    group_num = (pwm_groups)(pwm_num / 4);
    bit_map = BIT(pwm_num);

    ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);
    if (!ctlr_rtos)
        return -PWM_EPTR;

    ctlr = ctlr_rtos->ctlr;

    ret = mtk_mhal_pwm_deinit(ctlr, bit_map);
    if (ret)
        return ret;

    if (ctlr->data->pwm_nums == 0) {
        _hal_pwm_clock_disabe(group_num);
        ctlr->crtl_status = PWM_DEINIT;
    }

    return 0;
}
#endif /* #ifdef HAL_PWM_FEATURE_SINGLE_SOURCE_CLOCK */

hal_pwm_status_t hal_pwm_set_frequency(hal_pwm_channel_t pwm_num, uint32_t frequency, uint32_t *total_count)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    struct mtk_pwm_controller *ctlr = NULL;
    pwm_groups group_num = PWM_GROUP0;
    pwm_channels pwm_index = (pwm_channels)HAL_PWM_0;

    int ret = 0;

    group_num = (pwm_groups)(pwm_num / 4);
    pwm_index = (pwm_channels)(pwm_num % 4);
    ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);
    if (!ctlr_rtos)
        return -PWM_EPTR;

    ctlr = ctlr_rtos->ctlr;

    ctlr->data->frequency = frequency;

    ret = mtk_mhal_pwm_set_frequency(ctlr_rtos->ctlr, pwm_index);
    if (ret)
        return ret;

    *total_count = ctlr->data->total_count;

    return 0;
}
hal_pwm_status_t hal_pwm_set_duty_cycle(hal_pwm_channel_t pwm_num, uint32_t duty_cycle)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    struct mtk_pwm_controller *ctlr = NULL;
    pwm_groups group_num = PWM_GROUP0;
    pwm_channels pwm_index = (pwm_channels)HAL_PWM_0;
    int ret = 0;

    group_num = (pwm_groups)(pwm_num / 4);
    pwm_index = (pwm_channels)(pwm_num % 4);
    ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);
    if (!ctlr_rtos)
        return -PWM_EPTR;

    ctlr = ctlr_rtos->ctlr;

    ctlr->data->duty_cycle = duty_cycle;

    ret = mtk_mhal_pwm_set_duty_cycle(ctlr_rtos->ctlr, pwm_index);
    if (ret)
        return ret;

    return 0;
}
hal_pwm_status_t hal_pwm_start(hal_pwm_channel_t pwm_num)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    pwm_groups group_num = PWM_GROUP0;
    pwm_channels pwm_index = (pwm_channels)HAL_PWM_0;
    int ret = 0;

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_PWM);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    group_num = (pwm_groups)(pwm_num / 4);
    pwm_index = (pwm_channels)(pwm_num % 4);
    ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);
    if (!ctlr_rtos)
        return -PWM_EPTR;

    ret = mtk_mhal_pwm_start(ctlr_rtos->ctlr, pwm_index);
    ctlr_rtos->ctlr->data->running_status[pwm_index] = PWM_BUSY;

    return ret;
}
hal_pwm_status_t hal_pwm_stop(hal_pwm_channel_t pwm_num)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    pwm_groups group_num = PWM_GROUP0;
    pwm_channels pwm_index = (pwm_channels)HAL_PWM_0;
    int ret = 0;
    void *data = NULL;

    group_num = (pwm_groups)(pwm_num / 4);
    pwm_index = (pwm_channels)(pwm_num % 4);
    ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);
    if (!ctlr_rtos)
        return -PWM_EPTR;
#ifdef HAL_SLEEP_MANAGER_ENABLED
    pwm_backup_register_callback(data);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    ret = mtk_mhal_pwm_stop(ctlr_rtos->ctlr, pwm_index);
#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_PWM);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    ctlr_rtos->ctlr->data->running_status[pwm_index] = PWM_IDLE;

    return ret;
}
hal_pwm_status_t hal_pwm_get_frequency(hal_pwm_channel_t pwm_num, uint32_t *frequency)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    pwm_groups group_num = PWM_GROUP0;
    pwm_channels pwm_index = (pwm_channels)HAL_PWM_0;
    int ret = 0;

    group_num = (pwm_groups)(pwm_num / 4);
    pwm_index = (pwm_channels)(pwm_num % 4);
    ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);
    if (!ctlr_rtos)
        return -PWM_EPTR;

    ret = mtk_mhal_pwm_get_frequency(ctlr_rtos->ctlr, pwm_index);
    if (ret)
        return ret;
    *frequency = ctlr_rtos->ctlr->data->current_frequency[pwm_index];

    return 0;
}
hal_pwm_status_t hal_pwm_get_duty_cycle(hal_pwm_channel_t pwm_num, uint32_t *duty_cycle)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    pwm_groups group_num = PWM_GROUP0;
    pwm_channels pwm_index = (pwm_channels)HAL_PWM_0;
    int ret = 0;

    group_num = (pwm_groups)(pwm_num / 4);
    pwm_index = (pwm_channels)(pwm_num % 4);
    ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);

    if (!ctlr_rtos)
        return -PWM_EPTR;

    ret = mtk_mhal_pwm_get_duty_cycle(ctlr_rtos->ctlr, pwm_index);
    if (ret)
        return ret;
    *duty_cycle = ctlr_rtos->ctlr->data->current_duty_cycle[pwm_index];

    return 0;
}
hal_pwm_status_t hal_pwm_get_running_status(hal_pwm_channel_t pwm_num, hal_pwm_running_status_t *running_status)
{
    struct mtk_pwm_controller_rtos *ctlr_rtos = NULL;
    pwm_groups group_num = PWM_GROUP0;
    pwm_channels pwm_index = (pwm_channels)HAL_PWM_0;

    group_num = (pwm_groups)(pwm_num / 4);
    pwm_index = (pwm_channels)(pwm_num % 4);
    ctlr_rtos = _mtk_os_hal_pwm_get_ctlr(group_num);
    if (!ctlr_rtos)
        return -PWM_EPTR;

    *running_status = (hal_pwm_running_status_t)ctlr_rtos->ctlr->data->running_status[pwm_index] ;

    return 0;
}

#ifdef  HAL_PWM_FEATURE_ADVANCED_CONFIG
ATTR_HAL_DEPRECATED hal_pwm_status_t hal_pwm_set_advanced_config(hal_pwm_channel_t pwm_num, hal_pwm_advanced_config_t advanced_config)
{
    return 0;
}
#endif /* #ifdef  HAL_PWM_FEATURE_ADVANCED_CONFIG */

#endif /* #ifdef HAL_PWM_MODULE_ENABLED */
