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
#include "common.h"
#include "hal_gpio.h"
#include "hal_nvic_internal.h"


#ifdef HAL_GPIO_MODULE_ENABLED
#include "hal_gpio_internal.h"
#include "hal_log.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

GPIO_BASE_REGISTER_T *gpio_base = (GPIO_BASE_REGISTER_T *)(GPIO_BASE_BASE);
GPIO_CFG0_REGISTER_T *gpio_cfg0 = (GPIO_CFG0_REGISTER_T *)(CHIP_PINMUX_BASE);


hal_gpio_status_t hal_gpio_init(hal_gpio_pin_t gpio_pin)
{
    return HAL_GPIO_STATUS_OK;
}


hal_gpio_status_t hal_gpio_deinit(hal_gpio_pin_t gpio_pin)
{
    return HAL_GPIO_STATUS_OK;
}



hal_gpio_status_t hal_gpio_set_direction(hal_gpio_pin_t gpio_pin, hal_gpio_direction_t gpio_direction)
{
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    uint32_t pos;
    uint32_t bit;

    pos = gpio_pin / GPIO_REG_CTRL_PIN_NUM_OF_32;
    bit = gpio_pin % GPIO_REG_CTRL_PIN_NUM_OF_32;

    if (gpio_direction == HAL_GPIO_DIRECTION_INPUT) {
        gpio_base->GPIO_DIR[pos].CLR = (GPIO_REG_ONE_BIT_SET_CLR << bit);
    } else {
        gpio_base->GPIO_DIR[pos].SET = (GPIO_REG_ONE_BIT_SET_CLR << bit);
    }
    return HAL_GPIO_STATUS_OK;
}

hal_gpio_status_t hal_gpio_get_direction(hal_gpio_pin_t gpio_pin, hal_gpio_direction_t *gpio_direction)
{
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    if (NULL == gpio_direction) {
        return HAL_GPIO_STATUS_INVALID_PARAMETER;
    }

    uint32_t pos;
    uint32_t bit;

    pos = gpio_pin / GPIO_REG_CTRL_PIN_NUM_OF_32;
    bit = gpio_pin % GPIO_REG_CTRL_PIN_NUM_OF_32;

    if (gpio_base->GPIO_DIR[pos].RW & (GPIO_REG_ONE_BIT_SET_CLR << bit)) {
        *gpio_direction = HAL_GPIO_DIRECTION_OUTPUT;
    } else {
        *gpio_direction = HAL_GPIO_DIRECTION_INPUT;
    }

    return HAL_GPIO_STATUS_OK;
}

hal_pinmux_status_t hal_pinmux_set_function(hal_gpio_pin_t gpio_pin, uint8_t function_index)
{
    uint32_t no;
    uint32_t remainder;
    uint32_t irq_status;
    uint32_t temp;

    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_PINMUX_STATUS_ERROR_PORT;
    }

    /* check whether the function index is right as one function is corresponding to 4 bits of oen pin */
    if (function_index >= GPIO_MODE_MAX_NUMBER) {
        return HAL_PINMUX_STATUS_INVALID_FUNCTION;
    }

    irq_status = save_and_set_interrupt_mask();

    /* get the register number corresponding to the pin as one register can control 8 pins*/
    no = gpio_pin / GPIO_MODE_REG_CTRL_PIN_NUM;

    /* get the bit offset within the register as one register can control 8 pins*/
    remainder = gpio_pin % GPIO_MODE_REG_CTRL_PIN_NUM;

    temp = gpio_base->GPIO_MODE[no].RW;
    temp &= ~(GPIO_REG_FOUR_BIT_SET_CLR << (remainder * GPIO_MODE_FUNCTION_CTRL_BITS));
    temp |= (function_index << (remainder * GPIO_MODE_FUNCTION_CTRL_BITS));
    gpio_base->GPIO_MODE[no].RW = temp;
    restore_interrupt_mask(irq_status);

    return HAL_PINMUX_STATUS_OK;
}

hal_gpio_status_t hal_gpio_get_function(hal_gpio_pin_t gpio_pin, hal_gpio_mode_t *gpio_mode)
{
    uint32_t no;
    uint32_t remainder;

    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    if (NULL == gpio_mode) {
        return HAL_GPIO_STATUS_INVALID_PARAMETER;
    }

    /* get the register number corresponding to the pin as one register can control 8 pins*/
    no = gpio_pin / GPIO_MODE_REG_CTRL_PIN_NUM;
    /* get the bit offset within the register as one register can control 8 pins*/
    remainder = gpio_pin % GPIO_MODE_REG_CTRL_PIN_NUM;

    *gpio_mode = gpio_base->GPIO_MODE[no].RW >> (remainder * GPIO_MODE_FUNCTION_CTRL_BITS);

    *gpio_mode &= 0xF;

    return HAL_GPIO_STATUS_OK;

}

hal_gpio_status_t hal_gpio_get_input(hal_gpio_pin_t gpio_pin, hal_gpio_data_t *gpio_data)
{
    uint32_t pos;
    uint32_t bit;

    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    if (NULL == gpio_data) {
        return HAL_GPIO_STATUS_INVALID_PARAMETER;
    }
    pos = gpio_pin / GPIO_REG_CTRL_PIN_NUM_OF_32;
    bit = gpio_pin % GPIO_REG_CTRL_PIN_NUM_OF_32;

    if (gpio_base->GPIO_DIN[pos].R & (GPIO_REG_ONE_BIT_SET_CLR << bit)) {
        *gpio_data = HAL_GPIO_DATA_HIGH;
    } else {
        *gpio_data = HAL_GPIO_DATA_LOW;
    }

    return HAL_GPIO_STATUS_OK;

}


hal_gpio_status_t hal_gpio_set_output(hal_gpio_pin_t gpio_pin, hal_gpio_data_t gpio_data)
{
    uint32_t pos;
    uint32_t bit;

    pos = gpio_pin / GPIO_REG_CTRL_PIN_NUM_OF_32;
    bit = gpio_pin % GPIO_REG_CTRL_PIN_NUM_OF_32;
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    if (gpio_data) {
        gpio_base->GPIO_DOUT[pos].SET = (GPIO_REG_ONE_BIT_SET_CLR << bit);
    } else {
        gpio_base->GPIO_DOUT[pos].CLR = (GPIO_REG_ONE_BIT_SET_CLR << bit);
    }

    return HAL_GPIO_STATUS_OK;
}

hal_gpio_status_t hal_gpio_get_output(hal_gpio_pin_t gpio_pin, hal_gpio_data_t *gpio_data)
{
    uint32_t pos;
    uint32_t bit;

    pos = gpio_pin / GPIO_REG_CTRL_PIN_NUM_OF_32;
    bit = gpio_pin % GPIO_REG_CTRL_PIN_NUM_OF_32;
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    if (NULL == gpio_data) {
        return HAL_GPIO_STATUS_INVALID_PARAMETER;
    }

    if (gpio_base->GPIO_DOUT[pos].RW & (GPIO_REG_ONE_BIT_SET_CLR << bit)) {
        *gpio_data = HAL_GPIO_DATA_HIGH;
    } else {
        *gpio_data = HAL_GPIO_DATA_LOW;
    }

    return HAL_GPIO_STATUS_OK;

}

hal_gpio_status_t hal_gpio_toggle_pin(hal_gpio_pin_t gpio_pin)
{
    uint32_t pos;
    uint32_t bit;

    pos = gpio_pin / GPIO_REG_CTRL_PIN_NUM_OF_32;
    bit = gpio_pin % GPIO_REG_CTRL_PIN_NUM_OF_32;
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    if (gpio_base->GPIO_DOUT[pos].RW & (GPIO_REG_ONE_BIT_SET_CLR << bit)) {
        gpio_base->GPIO_DOUT[pos].CLR = (GPIO_REG_ONE_BIT_SET_CLR << bit);
    } else {
        gpio_base->GPIO_DOUT[pos].SET = (GPIO_REG_ONE_BIT_SET_CLR << bit);
    }

    return HAL_GPIO_STATUS_OK;
}

struct mtk_pin_grp {
    uint16_t pin;
    uint16_t index;
    uint16_t bit;
};

static struct mtk_pin_grp mt7933_pin_info[] = {
    {0, 1, 15},
    {1, 1, 16},
    {2, 1, 17},
    {3, 1, 18},
    {4, 1, 19},
    {5, 1, 20},
    {6, 1, 9},
    {7, 1, 10},
    {8, 1, 11},
    {9, 1, 12},
    {10, 1, 13},
    {11, 1, 14},
    {12, 0, 0},
    {13, 0, 1},
    {14, 0, 9},
    {15, 0, 10},
    {16, 0, 11},
    {17, 0, 12},
    {18, 0, 13},
    {19, 0, 14},
    {20, 0, 15},
    {21, 0, 16},
    {22, 0, 2},
    {23, 0, 3},
    {24, 0, 4},
    {25, 0, 5},
    {26, 0, 6},
    {27, 0, 7},
    {28, 0, 8},
    {29, 0, 17},
    {30, 0, 18},
    {31, 0, 21},
    {32, 0, 22},
    {33, 0, 23},
    {34, 0, 24},
    {35, 0, 25},
    {36, 0, 26},
    {37, 0, 27},
    {38, 0, 28},
    {39, 0, 19},
    {40, 0, 20},
    {41, 0, 29},
    {42, 0, 30},
    {43, 0, 31},
    {44, 1, 0},
    {45, 1, 1},
    {46, 1, 2},
    {47, 1, 6},
    {48, 1, 7},
    {49, 1, 8},
    {50, 1, 3},
    {51, 1, 4},
    {52, 1, 5}
};

static struct mtk_pin_grp mt7933_pin_drv_info[] = {
    {0, 0, 10},
    {1, 0, 0},
    {2, 0, 10},
    {3, 0, 0},
    {4, 0, 12},
    {5, 0, 0},
    {6, 0, 30},
    {7, 1, 0},
    {8, 1, 2},
    {9, 1, 4},
    {10, 1, 6},
    {11, 1, 8},
    {12, 0, 14},
    {13, 0, 20},
    {14, 0, 14},
    {15, 0, 20},
    {16, 0, 14},
    {17, 0, 22},
    {18, 0, 14},
    {19, 0, 22},
    {20, 0, 16},
    {21, 0, 22},
    {22, 0, 16},
    {23, 0, 22},
    {24, 0, 16},
    {25, 0, 24},
    {26, 0, 16},
    {27, 0, 24},
    {28, 0, 20},
    {29, 0, 28},
    {30, 0, 24},
    {31, 0, 28},
    {32, 0, 24},
    {33, 0, 28},
    {34, 0, 26},
    {35, 0, 28},
    {36, 0, 26},
    {37, 0, 2},
    {38, 0, 26},
    {39, 0, 2},
    {40, 0, 26},
    {41, 0, 6},
    {42, 0, 2},
    {43, 0, 6},
    {44, 0, 2},
    {45, 0, 6},
    {46, 0, 4},
    {47, 0, 6},
    {48, 0, 4},
    {49, 0, 8},
    {50, 0, 4},
    {51, 0, 8},
    {52, 0, 4}
};

hal_gpio_status_t hal_gpio_set_analog(hal_gpio_pin_t gpio_pin, bool enable)
{
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    const struct mtk_pin_grp *gpio_g_ctrl;
    gpio_g_ctrl = &mt7933_pin_info[gpio_pin];
    if (enable)
        gpio_cfg0->GPIO_G[gpio_g_ctrl->index].CLR = (GPIO_REG_ONE_BIT_SET_CLR << gpio_g_ctrl->bit);
    else
        gpio_cfg0->GPIO_G[gpio_g_ctrl->index].SET = (GPIO_REG_ONE_BIT_SET_CLR << gpio_g_ctrl->bit);

    return HAL_GPIO_STATUS_OK;
}

hal_gpio_status_t hal_gpio_pull_up(hal_gpio_pin_t gpio_pin)
{
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    const struct mtk_pin_grp *pupd_ctrl;
    pupd_ctrl = &mt7933_pin_info[gpio_pin];
    gpio_cfg0->GPIO_PUPD[pupd_ctrl->index].CLR = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    gpio_cfg0->GPIO_R1[pupd_ctrl->index].SET = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);

    return HAL_GPIO_STATUS_OK;
}

hal_gpio_status_t hal_gpio_pull_down(hal_gpio_pin_t gpio_pin)
{
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    const struct mtk_pin_grp *pupd_ctrl;
    pupd_ctrl = &mt7933_pin_info[gpio_pin];
    gpio_cfg0->GPIO_PUPD[pupd_ctrl->index].SET = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    gpio_cfg0->GPIO_R1[pupd_ctrl->index].SET = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);

    return HAL_GPIO_STATUS_OK;
}

hal_gpio_status_t hal_gpio_disable_pull(hal_gpio_pin_t gpio_pin)
{
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    const struct mtk_pin_grp *pupd_ctrl;
    pupd_ctrl = &mt7933_pin_info[gpio_pin];
    gpio_cfg0->GPIO_R0[pupd_ctrl->index].CLR = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    gpio_cfg0->GPIO_R1[pupd_ctrl->index].CLR = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);

    return HAL_GPIO_STATUS_OK;
}

#ifdef HAL_GPIO_FEATURE_CLOCKOUT
hal_gpio_status_t hal_gpio_set_clockout(hal_gpio_clock_t gpio_clock_num, hal_gpio_clock_mode_t clock_mode)
{
    return HAL_GPIO_STATUS_OK;
}

#endif /* #ifdef HAL_GPIO_FEATURE_CLOCKOUT */

#ifdef HAL_GPIO_FEATURE_SET_IES
hal_gpio_status_t hal_gpio_set_ies_enable(hal_gpio_pin_t gpio_pin)
{
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    const struct mtk_pin_grp *ies_ctrl;
    ies_ctrl = &mt7933_pin_info[gpio_pin];
    gpio_cfg0->GPIO_IES[ies_ctrl->index].SET = (GPIO_REG_ONE_BIT_SET_CLR << ies_ctrl->bit);

    return HAL_GPIO_STATUS_OK;
}

hal_gpio_status_t hal_gpio_ies_disable(hal_gpio_pin_t gpio_pin)
{
    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    const struct mtk_pin_grp *ies_ctrl;
    ies_ctrl = &mt7933_pin_info[gpio_pin];
    gpio_cfg0->GPIO_IES[ies_ctrl->index].CLR = (GPIO_REG_ONE_BIT_SET_CLR << ies_ctrl->bit);

    return HAL_GPIO_STATUS_OK;
}
#endif /* #ifdef HAL_GPIO_FEATURE_SET_IES */

#ifdef HAL_GPIO_FEATURE_PUPD
hal_gpio_status_t hal_gpio_set_pupd_register(hal_gpio_pin_t gpio_pin, uint8_t gpio_pupd, uint8_t gpio_r0, uint8_t gpio_r1)
{
    const struct mtk_pin_grp *pupd_ctrl;
    pupd_ctrl = &mt7933_pin_info[gpio_pin];

    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    if (gpio_pupd) {
        gpio_cfg0->GPIO_PUPD[pupd_ctrl->index].CLR = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    } else {
        gpio_cfg0->GPIO_PUPD[pupd_ctrl->index].SET = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    }

    if (gpio_r0) {
        gpio_cfg0->GPIO_R0[pupd_ctrl->index].SET = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    } else {
        gpio_cfg0->GPIO_R0[pupd_ctrl->index].CLR = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    }

    if (gpio_r1) {
        gpio_cfg0->GPIO_R1[pupd_ctrl->index].SET = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    } else {
        gpio_cfg0->GPIO_R1[pupd_ctrl->index].CLR = (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    }

    return HAL_GPIO_STATUS_OK;
}

hal_gpio_status_t hal_gpio_get_pupd_register(hal_gpio_pin_t gpio_pin, gpio_pull_type_t *pull_type)
{
    uint32_t pupd, r1, r0, temp;
    const struct mtk_pin_grp *pupd_ctrl;

    pupd_ctrl = &mt7933_pin_info[gpio_pin];

    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    pupd = gpio_cfg0->GPIO_PUPD[pupd_ctrl->index].RW & (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    pupd >>= (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    pupd &= 0x1;
    r0 = gpio_cfg0->GPIO_R0[pupd_ctrl->index].RW & (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    r0 >>= (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    r0 &= 0x1;
    r1 = gpio_cfg0->GPIO_R1[pupd_ctrl->index].RW & (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    r1 >>= (GPIO_REG_ONE_BIT_SET_CLR << pupd_ctrl->bit);
    r1 &= 0x1;
    temp = (pupd << 8) + (r1 << 4) + r0;

    if (temp == 0x001) {
        *pull_type = GPIO_PU_10K;
    } else if (temp == 0x010) {
        *pull_type = GPIO_PU_50K;
    } else if (temp == 0x011) {
        *pull_type = GPIO_PU_50K_10K;
    } else if (temp == 0x101) {
        *pull_type = GPIO_PD_10K;
    } else if (temp == 0x110) {
        *pull_type = GPIO_PD_50K;
    } else if (temp == 0x111) {
        *pull_type = GPIO_PD_50K_10K;
    } else if ((temp == 0x100) || (temp == 0x000)) {
        *pull_type = GPIO_NO_PULL;
    } else
        *pull_type = GPIO_PUPD_ERR;

    return HAL_GPIO_STATUS_OK;
}

#endif /* #ifdef HAL_GPIO_FEATURE_PUPD */

#ifdef HAL_GPIO_FEATURE_HIGH_Z
hal_gpio_status_t hal_gpio_set_high_impedance(hal_gpio_pin_t gpio_pin)
{

    hal_pinmux_status_t ret1;
    hal_gpio_status_t   ret2;

    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    /* set GPIO mode of pin */
    ret1 = hal_pinmux_set_function(gpio_pin, 0);
    if (ret1 != HAL_PINMUX_STATUS_OK) {
        return HAL_GPIO_STATUS_ERROR;
    }

    /* set input direction of pin */
    ret2 = hal_gpio_set_direction(gpio_pin, HAL_GPIO_DIRECTION_INPUT);
    if (ret2 != HAL_GPIO_STATUS_OK) {
        return ret2;
    }

    /* disable input buffer enable function of pin */
    ret2 = hal_gpio_ies_disable(gpio_pin);
    if (ret2 != HAL_GPIO_STATUS_OK) {
        return ret2;
    }

    /* disable pull function of pin */
    ret2 = hal_gpio_disable_pull(gpio_pin);
    if (ret2 != HAL_GPIO_STATUS_OK) {
        return ret2;
    }

    return HAL_GPIO_STATUS_OK;
}

hal_gpio_status_t hal_gpio_clear_high_impedance(hal_gpio_pin_t gpio_pin)
{
    hal_pinmux_status_t ret1;
    hal_gpio_status_t   ret2;

    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    /* set GPIO mode of pin. */
    ret1 = hal_pinmux_set_function(gpio_pin, 0);
    if (ret1 != HAL_PINMUX_STATUS_OK) {
        return HAL_GPIO_STATUS_ERROR;
    }

    /* set input direction of pin. */
    ret2 = hal_gpio_set_direction(gpio_pin, HAL_GPIO_DIRECTION_INPUT);
    if (ret2 != HAL_GPIO_STATUS_OK) {
        return ret2;
    }

    /* enable input buffer enable function of pin */
    ret2 = hal_gpio_set_ies_enable(gpio_pin);
    if (ret2 != HAL_GPIO_STATUS_OK) {
        return ret2;
    }

    /* enable pull down of pin. */
    ret2 = hal_gpio_pull_up(gpio_pin);
    if (ret2 != HAL_GPIO_STATUS_OK) {
        return ret2;
    }

    return HAL_GPIO_STATUS_OK;
}
#endif /* #ifdef HAL_GPIO_FEATURE_HIGH_Z */

#ifdef HAL_GPIO_FEATURE_SET_DRIVING
hal_gpio_status_t hal_gpio_set_driving_current(hal_gpio_pin_t gpio_pin, hal_gpio_driving_current_t driving)
{
    const struct mtk_pin_grp *drv_ctrl;
    drv_ctrl = &mt7933_pin_drv_info[gpio_pin];

    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    gpio_cfg0->GPIO_DRV[drv_ctrl->index].CLR = (GPIO_REG_TWO_BIT_SET_CLR << drv_ctrl->bit);
    gpio_cfg0->GPIO_DRV[drv_ctrl->index].SET = (driving << drv_ctrl->bit);

    return HAL_GPIO_STATUS_OK;
}

hal_gpio_status_t hal_gpio_get_driving_current(hal_gpio_pin_t gpio_pin, hal_gpio_driving_current_t *driving)
{
    uint32_t temp;

    temp = 0;
    const struct mtk_pin_grp *drv_ctrl;
    drv_ctrl = &mt7933_pin_drv_info[gpio_pin];

    if (gpio_pin >= HAL_GPIO_MAX) {
        return HAL_GPIO_STATUS_ERROR_PIN;
    }

    temp = (gpio_cfg0->GPIO_DRV[drv_ctrl->index].RW >> drv_ctrl->bit) & GPIO_REG_TWO_BIT_SET_CLR;

    *driving = (hal_gpio_driving_current_t)(temp);

    return HAL_GPIO_STATUS_OK;
}
#endif /* #ifdef HAL_GPIO_FEATURE_SET_DRIVING */

void gpio_get_state(hal_gpio_pin_t gpio_pin, gpio_state_t *gpio_state)
{
    uint32_t mode;
    uint32_t dir;
    uint32_t din;
    uint32_t dout;

    gpio_pull_type_t pull_type;
    uint32_t reg_index;
    uint32_t bit_index;
    hal_gpio_driving_current_t driving_value;

#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE != MTK_M_RELEASE))
    const char *direct[2] = {"input", "output"};
    const char *pull_state[18] = {"disable_pull", "PU_10k", "PD_10k", "PU_50K", "PD_50K",
                                  "PU_75k", "PD_75k", "PU_2K", "PD_2K", "PU_200k", "PD_200k",
                                  "PU_50k//10K", "PD_50k//10K", "PU_2k//75K", "PD_2k//75K",
                                  "PU_200k//75K", "PD_200k//75K", "PUPD_Error"
                                 };
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE != MTK_M_RELEASE)) */
    reg_index = gpio_pin / 8;
    bit_index = (gpio_pin % 8) * 4;
    mode = (gpio_base->GPIO_MODE[reg_index].RW >> (bit_index) & 0xf);

    reg_index = gpio_pin / 32;
    bit_index = gpio_pin % 32;
    dir  = (gpio_base->GPIO_DIR[reg_index].RW >> (bit_index) & 0x1);
    din  = (gpio_base->GPIO_DIN[reg_index].R >> (bit_index) & 0x1);
    dout = (gpio_base->GPIO_DOUT[reg_index].RW >> (bit_index) & 0x1);

    hal_gpio_get_pupd_register(gpio_pin, &pull_type);

    hal_gpio_get_driving_current(gpio_pin, &driving_value);

    gpio_state->mode = mode;
    gpio_state->dir  = dir;
    gpio_state->din  = (hal_gpio_data_t)din;
    gpio_state->dout = (hal_gpio_data_t)dout;
    gpio_state->pull_type = pull_type;
    gpio_state->current_type = (hal_gpio_driving_current_t)((uint8_t)driving_value);

    log_hal_info("LOG: GPIO%d, mode=%d, %s, din=%d, dout=%d, %s\r\n", gpio_pin, mode, direct[dir], din, dout, pull_state[(uint32_t)pull_type]);

}


struct cr_set_val_t {
    uint32_t cr_addr;
    uint32_t val;
};

struct cr_set_val_t gpio_ls_cr_table[] = {
    {0x30404300, 0},
    {0x30404310, 0},
    {0x30404320, 0},
    {0x30404330, 0},
    {0x30404340, 0},
    {0x30404350, 0},
    {0x30404360, 0},

    {0x304030D0, 0},
    {0x304030E0, 0},
    {0x304030F0, 0},
    {0x30403100, 0},
    {0x30403110, 0},
    {0x30403120, 0},
};

void hal_gpio_suspend(void *data)
{
    uint32_t i;

    for (i = 0; i < sizeof(gpio_ls_cr_table) / sizeof(struct cr_set_val_t); i++) {
        gpio_ls_cr_table[i].val = HAL_REG_32(gpio_ls_cr_table[i].cr_addr);
    }
}

void hal_gpio_resume(void *data)
{
    uint32_t i;

    for (i = 0; i < sizeof(gpio_ls_cr_table) / sizeof(struct cr_set_val_t); i++) {
        HAL_REG_32(gpio_ls_cr_table[i].cr_addr) = gpio_ls_cr_table[i].val;
    }
}

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifdef HAL_GPIO_MODULE_ENABLED */

