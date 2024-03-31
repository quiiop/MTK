/*
 * (C) 2005-2020 MediaTek Inc. All rights reserved.
 *
 * Copyright Statement:
 *
 * This MT7933 driver software/firmware and related documentation
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
#include "hal_eint.h"

#ifdef HAL_EINT_MODULE_ENABLED
#include "hal_nvic.h"
#include "hal_gpt.h"
#include "hal_nvic_internal.h"
#include "mhal_eint.h"
#include "hal_platform.h"

#define EINT_DEBOUNCE_BASE_ADDR     0x21000000
#define TABLE_SIZE          31
void __iomem *eint_base = (void __iomem *)EINT_DEBOUNCE_BASE_ADDR;

typedef struct {
    hal_eint_number_t eint_number;
    IRQn_Type irq;
} eint_irq_map_t;

typedef struct {
    hal_eint_callback_t eint_callback;
    void *user_data;
} eint_function_t;

eint_function_t eint_function_table[HAL_EINT_NUMBER_MAX];

eint_irq_map_t eint_irq_table[TABLE_SIZE] = {
    {HAL_EINT_NUMBER_0, GPIO_IRQ0n},
    {HAL_EINT_NUMBER_1, GPIO_IRQ1n},
    {HAL_EINT_NUMBER_2, GPIO_IRQ2n},
    {HAL_EINT_NUMBER_3, GPIO_IRQ3n},
    {HAL_EINT_NUMBER_4, GPIO_IRQ4n},
    {HAL_EINT_NUMBER_5, GPIO_IRQ5n},
    {HAL_EINT_NUMBER_6, GPIO_IRQ6n},
    {HAL_EINT_NUMBER_7, GPIO_IRQ7n},
    {HAL_EINT_NUMBER_8, GPIO_IRQ8n},
    {HAL_EINT_NUMBER_9, GPIO_IRQ9n},
    {HAL_EINT_NUMBER_10, GPIO_IRQ10n},
    {HAL_EINT_NUMBER_11, GPIO_IRQ11n},
    {HAL_EINT_NUMBER_12, GPIO_IRQ12n},
    {HAL_EINT_NUMBER_13, GPIO_IRQ13n},
    {HAL_EINT_NUMBER_14, GPIO_IRQ14n},
    {HAL_EINT_NUMBER_15, GPIO_IRQ15n},
    {HAL_EINT_NUMBER_16, GPIO_IRQ16n},
    {HAL_EINT_NUMBER_17, GPIO_IRQ17n},
    {HAL_EINT_NUMBER_18, GPIO_IRQ18n},
    {HAL_EINT_NUMBER_19, GPIO_IRQ19n},
    {HAL_EINT_NUMBER_20, GPIO_IRQ20n},
    {HAL_EINT_NUMBER_21, GPIO_IRQ21n},
    {HAL_EINT_NUMBER_22, GPIO_IRQ22n},
    {HAL_EINT_NUMBER_23, GPIO_IRQ23n},
    {HAL_EINT_NUMBER_24, GPIO_IRQ24n},
    {HAL_EINT_NUMBER_25, GPIO_IRQ25n},
    {HAL_EINT_NUMBER_26, GPIO_IRQ26n},
    {HAL_EINT_NUMBER_27, GPIO_IRQ27n},
    {HAL_EINT_NUMBER_28, GPIO_IRQ28n},
    {HAL_EINT_NUMBER_29, GPIO_IRQ29n},
    {HAL_EINT_NUMBER_30, GPIO_IRQ30n}
};

static int hal_eint_convert_gpio_irq(hal_eint_number_t eint_number, IRQn_Type *irq)
{
    uint32_t index = 0;

    for (index = 0; index < TABLE_SIZE; index++) {
        if (eint_irq_table[index].eint_number == eint_number) {
            *irq = (IRQn_Type)eint_irq_table[index].irq;
            return 0;
        }
    }

    return -EINT_EINVAL;
}

static hal_eint_number_t hal_eint_convert_irq_to_eint(IRQn_Type irq)
{
    uint32_t index = 0;

    for (index = 0; index < TABLE_SIZE; index++) {
        if (eint_irq_table[index].irq == irq)
            return eint_irq_table[index].eint_number;
    }

    return HAL_EINT_NUMBER_MAX;
}

hal_eint_status_t hal_eint_init(hal_eint_number_t eint_number, const hal_eint_config_t *eint_config)
{
    hal_eint_status_t status;
    IRQn_Type irq;
    uint32_t sens;

    if (hal_eint_convert_gpio_irq(eint_number, &irq) != 0)
        return HAL_EINT_STATUS_INVALID_PARAMETER;

    if (eint_number >= HAL_EINT_NUMBER_MAX || eint_config == NULL) {
        return HAL_EINT_STATUS_INVALID_PARAMETER;
    }

    hal_nvic_disable_irq(irq);

    if ((eint_config->trigger_mode == HAL_EINT_LEVEL_LOW) ||
        (eint_config->trigger_mode == HAL_EINT_EDGE_FALLING) ||
        (eint_config->trigger_mode == HAL_EINT_EDGE_FALLING_AND_RISING))
        status = mtk_mhal_eint_set_polarity(eint_number, 1, eint_base);
    else
        status = mtk_mhal_eint_set_polarity(eint_number, 0, eint_base);

    if (status != 0) {
        hal_nvic_enable_irq(irq);
        return HAL_EINT_STATUS_ERROR;
    }

    if (eint_config->trigger_mode == HAL_EINT_EDGE_FALLING_AND_RISING)
        status = mtk_mhal_eint_set_dual(eint_number, 1, eint_base);
    else
        status = mtk_mhal_eint_set_dual(eint_number, 0, eint_base);

    if (status != 0) {
        hal_nvic_enable_irq(irq);
        return HAL_EINT_STATUS_ERROR;
    }

    if ((eint_config->trigger_mode == HAL_EINT_LEVEL_LOW) ||
        (eint_config->trigger_mode == HAL_EINT_LEVEL_HIGH))
        sens = 1;
    else
        sens = 0;

    hal_nvic_clear_pending_irq(irq);
    hal_nvic_set_priority(irq, DEFAULT_PRI);
    if (sens == 1)
        hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    else
        hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_EDGE_RISING);
    hal_nvic_enable_irq(irq);

    return HAL_EINT_STATUS_OK;
}

hal_eint_status_t hal_eint_deinit(hal_eint_number_t eint_number)
{
    uint32_t mask;
    if ((eint_number >= HAL_EINT_NUMBER_MAX) || (eint_number < 0)) {
        return HAL_EINT_STATUS_ERROR_EINT_NUMBER;
    }

    mask = save_and_set_interrupt_mask();
    eint_function_table[eint_number].eint_callback = NULL;
    eint_function_table[eint_number].user_data = NULL;
    restore_interrupt_mask(mask);

    return HAL_EINT_STATUS_OK;
}

static void eint_isr(hal_nvic_irq_t irq)
{
    hal_eint_number_t eint_number;
    hal_eint_callback_t callback;

    eint_number = hal_eint_convert_irq_to_eint(irq);
    if ((eint_number == HAL_EINT_NUMBER_MAX) || (eint_number < 0))
        return;

    callback = eint_function_table[eint_number].eint_callback;
    if (callback)
        callback(eint_function_table[eint_number].user_data);
}

hal_eint_status_t hal_eint_register_callback(hal_eint_number_t eint_number,
                                             hal_eint_callback_t eint_callback,
                                             void *user_data)
{
    uint32_t mask;
    IRQn_Type irq;

    if ((eint_number >= HAL_EINT_NUMBER_MAX) || (eint_callback == NULL) || (eint_number < 0)) {
        return HAL_EINT_STATUS_INVALID_PARAMETER;
    }

    if (hal_eint_convert_gpio_irq(eint_number, &irq) != 0)
        return HAL_EINT_STATUS_INVALID_PARAMETER;
    mask = save_and_set_interrupt_mask();
    hal_nvic_register_isr_handler(irq, eint_isr);
    eint_function_table[eint_number].eint_callback = eint_callback;
    eint_function_table[eint_number].user_data = user_data;
    restore_interrupt_mask(mask);

    return HAL_EINT_STATUS_OK;
}

/***************************************** *******************************
    sensitivity:
                            1         level  (default)
                            0         edge
    polarity:               0         negative polarity  (default)
                            1         positive polarity

 *************************************************************************/
hal_eint_status_t hal_eint_set_trigger_mode(hal_eint_number_t eint_number,
                                            hal_eint_trigger_mode_t trigger_mode)
{
    uint32_t mask;
    IRQn_Type irq;

    if (eint_number >= HAL_EINT_NUMBER_MAX) {
        return HAL_EINT_STATUS_ERROR_EINT_NUMBER;
    }

    if (hal_eint_convert_gpio_irq(eint_number, &irq) != 0)
        return HAL_EINT_STATUS_INVALID_PARAMETER;

    if (trigger_mode > HAL_EINT_EDGE_FALLING_AND_RISING) {
        return HAL_EINT_STATUS_INVALID_PARAMETER;
    }

    mask = save_and_set_interrupt_mask();
    if (trigger_mode == HAL_EINT_LEVEL_LOW) {
        mtk_mhal_eint_set_polarity(eint_number, 1, eint_base);
        hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    } else if (trigger_mode == HAL_EINT_LEVEL_HIGH) {
        mtk_mhal_eint_set_polarity(eint_number, 0, eint_base);
        hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    } else if (trigger_mode == HAL_EINT_EDGE_FALLING) {
        mtk_mhal_eint_set_polarity(eint_number, 1, eint_base);
        hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_EDGE_RISING);
    } else if (trigger_mode == HAL_EINT_EDGE_RISING) {
        mtk_mhal_eint_set_polarity(eint_number, 0, eint_base);
        hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_EDGE_RISING);
    } else if (trigger_mode == HAL_EINT_EDGE_FALLING_AND_RISING) {
        mtk_mhal_eint_set_dual(eint_number, 1, eint_base);
    } else {
        restore_interrupt_mask(mask);
        return HAL_EINT_STATUS_INVALID_PARAMETER;
    }
    restore_interrupt_mask(mask);

    return HAL_EINT_STATUS_OK;
}

hal_eint_status_t hal_eint_set_debounce_count(hal_eint_number_t eint_number, uint32_t count)
{
    int state = 0;

    if (eint_number >= HAL_EINT_NUMBER_MAX) {
        return HAL_EINT_STATUS_ERROR_EINT_NUMBER;
    }

    if (count == 0) {
        /*disenable debounce bit*/
        state = mtk_mhal_eint_disable_debounce(eint_number, eint_base);
    } else {
        state = mtk_mhal_eint_enable_debounce(eint_number, eint_base);
    }

    if (state != 0)
        return HAL_EINT_STATUS_ERROR;

    return HAL_EINT_STATUS_OK;
}

hal_eint_status_t hal_eint_set_debounce_time(hal_eint_number_t eint_number, uint32_t time_ms)
{
    IRQn_Type irq;
    int state = 0;

    if (hal_eint_convert_gpio_irq(eint_number, &irq) != 0)
        return HAL_EINT_STATUS_INVALID_PARAMETER;

    hal_nvic_disable_irq(irq);
    state = mtk_mhal_eint_set_debounce(eint_number, (u32)irq, time_ms, eint_base);
    if (state != 0) {
        hal_nvic_enable_irq(irq);
        return HAL_EINT_STATUS_ERROR;
    }

    hal_gpt_delay_us(125);
    state = mtk_mhal_eint_enable_debounce(eint_number, eint_base);
    if (state != 0) {
        hal_nvic_enable_irq(irq);
        return HAL_EINT_STATUS_ERROR;
    }

    hal_nvic_enable_irq(irq);

    return HAL_EINT_STATUS_OK;
}

hal_eint_status_t hal_eint_set_software_trigger(hal_eint_number_t eint_number)
{
    uint32_t mask;
    IRQn_Type irq;

    if (hal_eint_convert_gpio_irq(eint_number, &irq) != 0)
        return HAL_EINT_STATUS_INVALID_PARAMETER;

    mask = save_and_set_interrupt_mask();
    hal_nvic_set_pending_irq(irq);
    restore_interrupt_mask(mask);

    return HAL_EINT_STATUS_OK;
}

hal_eint_status_t hal_eint_clear_software_trigger(hal_eint_number_t eint_number)
{
    uint32_t mask;
    IRQn_Type irq;

    if (hal_eint_convert_gpio_irq(eint_number, &irq) != 0)
        return HAL_EINT_STATUS_INVALID_PARAMETER;

    mask = save_and_set_interrupt_mask();
    hal_nvic_clear_pending_irq(irq);
    restore_interrupt_mask(mask);

    return HAL_EINT_STATUS_OK;
}

hal_eint_status_t eint_mask_wakeup_source(hal_eint_number_t eint_number)
{
    return HAL_EINT_STATUS_INVALID_PARAMETER;
}

hal_eint_status_t eint_unmask_wakeup_source(hal_eint_number_t eint_number)
{
    return HAL_EINT_STATUS_INVALID_PARAMETER;
}

uint32_t eint_get_status(void)
{
    uint32_t index = 0;
    uint32_t status = 0;
    IRQn_Type irq;

    for (index = 0; index < TABLE_SIZE; index++) {
        if (hal_eint_convert_gpio_irq(index, &irq) != 0)
            return HAL_EINT_STATUS_INVALID_PARAMETER;
        else
            status |= (hal_nvic_get_pending_irq(irq) << index);
    }

    return status;
}

void eint_ack_interrupt(uint32_t eint_number)
{
    hal_eint_clear_software_trigger(eint_number);
}

#ifdef HAL_EINT_FEATURE_MASK
hal_eint_status_t hal_eint_mask(hal_eint_number_t eint_number)
{
    IRQn_Type irq;

    if (hal_eint_convert_gpio_irq(eint_number, &irq) != 0)
        return HAL_EINT_STATUS_INVALID_PARAMETER;

    hal_nvic_disable_irq(irq);

    return HAL_EINT_STATUS_OK;
}

hal_eint_status_t hal_eint_unmask(hal_eint_number_t eint_number)
{
    IRQn_Type irq;

    if (hal_eint_convert_gpio_irq(eint_number, &irq) != 0)
        return HAL_EINT_STATUS_INVALID_PARAMETER;

    hal_nvic_enable_irq(irq);

    return HAL_EINT_STATUS_OK;
}
#endif /* #ifdef HAL_EINT_FEATURE_MASK */


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifdef HAL_EINT_MODULE_ENABLED */
