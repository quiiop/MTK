/* Copyright Statement:
 *
 * (C) 2022-2022  MediaTek Inc. All rights reserved.
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


#include <FreeRTOS.h>


#include <hal.h>


#include "filogic_button.h"


#define EINT_CHK(statement) assert(HAL_EINT_STATUS_OK == statement)
#define PMUX_CHK(statement) assert(HAL_PINMUX_STATUS_OK == statement)
#define GPIO_CHK(statement) assert(HAL_GPIO_STATUS_OK == statement)


// TODO: can we reduce the time to make light on/off faster?
#define KEY_DEBOUNCE_TIME (10)     /* millisecond */


typedef struct filogic_button_info
{
    filogic_button_id_t     id;
    hal_gpio_pin_t          pin;
    uint8_t                 pin_func;
    hal_eint_number_t       eint;
} filogic_button_info_t;


static const filogic_button_info_t _g_button_info[] =
{
    { FILOGIC_BUTTON_0,
      HAL_GPIO_24,
      MT7933_PIN_24_FUNC_CM33_GPIO_EINT16,
      HAL_EINT_NUMBER_16 },
};
#define BUTTON_COUNT (sizeof(_g_button_info)/sizeof(filogic_button_info_t))


static filogic_button_event_callback _g_app_callback;


static void _filogic_button_set_pol(hal_eint_number_t eint,
                                    bool              trig_high)
{
    hal_eint_config_t cfg;
    cfg.trigger_mode = trig_high ? HAL_EINT_LEVEL_HIGH :
                                   HAL_EINT_LEVEL_LOW;
    EINT_CHK(hal_eint_init(eint, &cfg));

    EINT_CHK(hal_eint_set_debounce_count(eint, 0));

    EINT_CHK(hal_eint_set_debounce_time(eint, KEY_DEBOUNCE_TIME));
}


static void _button_isr(void *data)
{
    static bool _g_pressed = false;
    const filogic_button_info_t *b;

    b = (const filogic_button_info_t *)data;

#ifdef HAL_EINT_FEATURE_MASK
    EINT_CHK(hal_eint_mask(b->eint));
#endif

    _g_pressed = !_g_pressed;
    _filogic_button_set_pol(b->eint, _g_pressed);

    if (_g_app_callback) {
        filogic_button_t button = { .id = b->id, .press = _g_pressed };
        _g_app_callback(&button);
    }

#ifdef HAL_EINT_FEATURE_MASK
    EINT_CHK(hal_eint_unmask(b->eint));
#endif
}


bool filogic_button_init(void)
{
    const filogic_button_info_t *button;
    unsigned int                i;

    for (i = 0; i < BUTTON_COUNT; i++)
    {
        button = &_g_button_info[i];

        GPIO_CHK(hal_gpio_init(button->pin));

        PMUX_CHK(hal_pinmux_set_function(button->pin, button->pin_func));

#ifdef HAL_EINT_FEATURE_MASK
        EINT_CHK(hal_eint_mask(button->eint));
#endif
        _filogic_button_set_pol(button->eint, false);
        EINT_CHK(hal_eint_set_debounce_time(button->eint, KEY_DEBOUNCE_TIME));
        EINT_CHK(hal_eint_register_callback(button->eint, _button_isr,
                                            (void *)button));
#ifdef HAL_EINT_FEATURE_MASK
        EINT_CHK(hal_eint_unmask(button->eint));
#endif
    }

    return true;
}

bool filogic_button_set_callback(filogic_button_event_callback callback)
{
    _g_app_callback = callback;
    return true;
}
