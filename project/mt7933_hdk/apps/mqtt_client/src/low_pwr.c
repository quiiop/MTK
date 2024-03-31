#include "hal_sleep_manager.h"
#include "hal_gpio.h"
#include "common.h"
#include "low_pwr.h"
#include "hal_boot.h"

#define IOCFG_0_SLP_BASE 0x30403130

#define MOD 0xC
#define E_CFG_0 0x00
#define E_CFG_1 0x10
#define HWCTL_CFG_0 0x20
#define HWCTL_CFG_1 0x30
#define I_CFG_0 0x40
#define I_CFG_1 0x50
#define PUPD_CFG_0 0x60
#define PUPD_CFG_1 0x70
#define R0_CFG_0 0x80
#define R0_CFG_1 0x90
#define R1_CFG_0 0xA0
#define R1_CFG_1 0xB0

void user_gpio_suspend(void *data)
{
    /* For E1 RFB due to DMIC and strap pin shared pin */
    if (hal_boot_get_hw_ver() == 0x8A00) {
        WRITE_REG(0x00000008, 0x30404330 + MOD);
    }

    WRITE_REG(0x00800080, 0x30404330 + MOD);
    WRITE_REG(0x88888000, 0x30404340 + MOD);
    WRITE_REG(0x00000808, 0x30404350 + MOD);
    WRITE_REG(0x88800800, 0x30404360 + MOD);
    return;
}

static void low_power_gpio_init(void)
{
    /* Set GPIO pin 0 ~ pin 52 SLP setting */
    WRITE_REG(0x5E1A0020, IOCFG_0_SLP_BASE + HWCTL_CFG_0);
    WRITE_REG(0x001F8008, IOCFG_0_SLP_BASE + HWCTL_CFG_1);

    /* Set SLP PUPD */
    WRITE_REG(0xFFB7FFFF, IOCFG_0_SLP_BASE + PUPD_CFG_0);
    WRITE_REG(0x003EFFFF, IOCFG_0_SLP_BASE + PUPD_CFG_1);

    /* Set SLP R0 R1 */
    WRITE_REG(0x00400000, IOCFG_0_SLP_BASE + R0_CFG_0);
    WRITE_REG(0x00010000, IOCFG_0_SLP_BASE + R0_CFG_1);
    WRITE_REG(0xFFFFFFFF, IOCFG_0_SLP_BASE + R1_CFG_0);
    WRITE_REG(0x003FFFFF, IOCFG_0_SLP_BASE + R1_CFG_1);

    /* Set SLP E (Set GPIO pin to output mode when ic is in sleep mode) */
    WRITE_REG(0x00000001, IOCFG_0_SLP_BASE + E_CFG_0);
    WRITE_REG(0x00010000, IOCFG_0_SLP_BASE + E_CFG_1);

    /* Set SLP I (Set GPIO pin to output high(1)/low(0) when ic is in sleep mode) */
    WRITE_REG(0x00000000, IOCFG_0_SLP_BASE + I_CFG_0);
    WRITE_REG(0x00010000, IOCFG_0_SLP_BASE + I_CFG_1);
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_register_suspend_callback(user_gpio_suspend, NULL);
#endif /* HAL_SLEEP_MANAGER_ENABLED */
    return;
}

void low_power_init(void)
{
    low_power_gpio_init();
    return;
}
