#include "hal_sleep_manager.h"
#include "hal_gpio.h"
#include "common.h"
#include "low_pwr.h"
#include "hal_boot.h"

#define IOCFG_IES_CFG0 0x304030B0
#define IOCFG_IES_CFG1 0x304030C0

#define IOCFG_0_SLP_BASE 0x30403130

#define SET 0x4
#define CLR 0x8
#define MOD 0xC
#define E_CFG_0 0x00
#define E_CFG_1 0x10
#define HWCTL_CFG_0 0x20
#define HWCTL_CFG_1 0x30
#define PUPD_CFG_0 0x60
#define PUPD_CFG_1 0x70
#define R0_CFG_0 0x80
#define R0_CFG_1 0x90
#define R1_CFG_0 0xA0
#define R1_CFG_1 0xB0

void user_gpio_suspend(void *data)
{
    if (hal_boot_get_hw_ver() == 0x8A10) {
        WRITE_REG(0x00800000, 0x30404330 + MOD);
        WRITE_REG(0x88888000, 0x30404340 + MOD);
        WRITE_REG(0x00000808, 0x30404350 + MOD);
        WRITE_REG(0x88888880, 0x30404360 + MOD);
    } else {
        /* E1 workaround: before enter sleep, gpio mode register need to set
         * in gpio mode */
        static hal_gpio_pin_t setting_info[] = {
            HAL_GPIO_0,
            HAL_GPIO_1,
            HAL_GPIO_2,
            HAL_GPIO_3,
            HAL_GPIO_4,
            HAL_GPIO_5,
#ifdef MTK_HIFI4DSP_ENABLE
            HAL_GPIO_29,
            HAL_GPIO_30,
#endif /* #ifdef MTK_HIFI4DSP_ENABLE */
            HAL_GPIO_31,
            HAL_GPIO_33,
            HAL_GPIO_34,
            HAL_GPIO_42,
            HAL_GPIO_44,
            HAL_GPIO_48,
            HAL_GPIO_50,
            HAL_GPIO_MAX /* avoiding pointer access illegal address */
        };
        uint8_t pin_reserved = 0;
        for (hal_gpio_pin_t pin = HAL_GPIO_0; pin < HAL_GPIO_MAX; ++pin) {
            if (pin != setting_info[pin_reserved]) {
                hal_pinmux_set_function(pin, 0U);
            } else {
                ++pin_reserved;
            }
        }
    }
    return;
}

static void low_power_gpio_init(void)
{
    if (hal_boot_get_hw_ver() == 0x8A10) {
        /* Set GPIO pin 0 ~ pin 52 SLP setting */
        WRITE_REG(0x5E1A01A1, IOCFG_0_SLP_BASE + HWCTL_CFG_0);
        WRITE_REG(0x001F8008, IOCFG_0_SLP_BASE + HWCTL_CFG_1);

        /* Set SLP PUPD */
        WRITE_REG(0xFFB7FFFF, IOCFG_0_SLP_BASE + PUPD_CFG_0);
        WRITE_REG(0x003EFFFF, IOCFG_0_SLP_BASE + PUPD_CFG_1);

        /* Set SLP R0 R1 */
        WRITE_REG(0x40400000, IOCFG_0_SLP_BASE + R0_CFG_0);
        WRITE_REG(0x00010000, IOCFG_0_SLP_BASE + R0_CFG_1);
        WRITE_REG(0xFFFFFFFF, IOCFG_0_SLP_BASE + R1_CFG_0);
        WRITE_REG(0x003FFFFF, IOCFG_0_SLP_BASE + R1_CFG_1);

        /* Set SLP E */
        WRITE_REG(0x1, IOCFG_0_SLP_BASE + E_CFG_0);
        WRITE_REG(0x0, IOCFG_0_SLP_BASE + E_CFG_1);
    } else {
        /* E1 IC */
        /* Set GPIO pin 0 ~ pin 52 SLP setting */
        WRITE_REG(0xFEF9FFBF, IOCFG_0_SLP_BASE + HWCTL_CFG_0);
        WRITE_REG(0x003FFFFE, IOCFG_0_SLP_BASE + HWCTL_CFG_1);
        WRITE_REG(0x00000000, IOCFG_0_SLP_BASE + E_CFG_0);
        WRITE_REG(0x00000000, IOCFG_0_SLP_BASE + E_CFG_1);

        WRITE_REG(0x00000000, IOCFG_0_SLP_BASE + R0_CFG_0);
        WRITE_REG(0x00000000, IOCFG_0_SLP_BASE + R0_CFG_1);
        WRITE_REG(0x00000000, IOCFG_0_SLP_BASE + R1_CFG_0);
        WRITE_REG(0x00000000, IOCFG_0_SLP_BASE + R1_CFG_1);

        /* Set SLP pupd setting */
        WRITE_REG(0x00A00200, IOCFG_0_SLP_BASE + PUPD_CFG_0);
        WRITE_REG(0x00000000, IOCFG_0_SLP_BASE + PUPD_CFG_1);
    }

    /* All GPIO pin are input enabled */
    WRITE_REG(0xFFFFFFFF, (IOCFG_IES_CFG0));
    WRITE_REG(0x003FFFFF, (IOCFG_IES_CFG1));
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_register_suspend_callback(user_gpio_suspend, NULL);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    return;
}

void low_power_init(void)
{
    low_power_gpio_init();
    return;
}
