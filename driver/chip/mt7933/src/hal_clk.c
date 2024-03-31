/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
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
#include "common.h"
#include "hal_log.h"
#include "hal_gpt.h"
#include "hal.h"
#include "hal_clk.h"
#include "hal_clock.h"
#include "hal_platform.h"
#include "memory_attribute.h"
#include "hal_boot.h"
#include "hal_efuse_get.h"
#include "hal_spm.h"

#define MHZ(_x_) (_x_##000000)
#define KHZ(_x_) (_x_##000)
#define EXTERNAL_32K_DIVIDER (32768U)
#define INTERNAL_32K_DIVIDER (32746U)

#define CM33_CLK_CTL                (TOP_CFG_AON_BASE + 0x220)
#define F32K_CLK_CTL                (TOP_CFG_AON_BASE + 0x210)
#define PLL_CLK_CG                  (TOP_CLK_OFF_BASE + 0x200)
#define XTAL_CLK_CTRL               (TOP_CLK_OFF_BASE + 0x214)
#define AUX_ADC_CLK_CTL             (TOP_CLK_OFF_BASE + 0x218)
#define PWM_CLK_DIV_CTL             (TOP_CLK_OFF_BASE + 0x21C)
#define DSP_CLK_CTL                 (TOP_CLK_OFF_BASE + 0x230)
#define CLK_AUDDIV_0                (TOP_CLK_OFF_BASE + 0x238)
#define CLK_AUDDIV_1                (TOP_CLK_OFF_BASE + 0x23C)
#define AUDIO_FAUDIO_CLK_SEL        (TOP_CLK_OFF_BASE + 0x240)
#define AUDIO_FAUD_INTBUS_CLK_SEL   (TOP_CLK_OFF_BASE + 0x244)
#define AUDIO_HAPLL_CLK_SEL         (TOP_CLK_OFF_BASE + 0x248)
#define AUDIO_FASYS_CLK_SEL         (TOP_CLK_OFF_BASE + 0x24C)
#define AUDIO_FASM_CLK_SEL          (TOP_CLK_OFF_BASE + 0x250)
#define AUDIO_FAUD_CLK_SEL          (TOP_CLK_OFF_BASE + 0x254)
#define PSRAM_CLK_CTL               (TOP_CLK_OFF_BASE + 0x260)
#define FLASH_CLK_CTL               (TOP_CLK_OFF_BASE + 0x26C)
#define AXI_CLK_CTL                 (TOP_CLK_OFF_BASE + 0x270)
#define AUDSYS_BUS_CLK_CTL          (TOP_CLK_OFF_BASE + 0x274)
#define PSRAMAXI_CLK_CTL            (TOP_CLK_OFF_BASE + 0x278)
#define SPIM0_CLK_CTL               (TOP_CLK_OFF_BASE + 0x280)
#define SPIM1_CLK_CTL               (TOP_CLK_OFF_BASE + 0x284)
#define SPIS_CLK_CTL                (TOP_CLK_OFF_BASE + 0x288)
#define I2C_CLK_CTL                 (TOP_CLK_OFF_BASE + 0x290)
#define USB_CLK_CTL                 (TOP_CLK_OFF_BASE + 0x2A0)
#define SDIOM_CLK_CTL               (TOP_CLK_OFF_BASE + 0x2A4)
#define TRACE_CLK_CTL               (TOP_CLK_OFF_BASE + 0x2B0)
#define DBG_CLK_CTL                 (TOP_CLK_OFF_BASE + 0x2B4)
#define GCPU_CLK_CTL                (TOP_CLK_OFF_BASE + 0x300)
#define ECC_CLK_CTL                 (TOP_CLK_OFF_BASE + 0x304)

#define FREDETCR0                   (TOP_CFG_OFF_BASE + 0x300)
#define FREDETCR1                   (TOP_CFG_OFF_BASE + 0x304)

#define PLL_EN_CTRL                 (PLL_CTRL_BASE    + 0x000)
#define PLL_EN_CTRL_TOPPLL_EN_OFFSET          1
#define PLL_EN_CTRL_USBPLL_EN_OFFSET          3
#define PLL_EN_CTRL_DSPPLL_EN_OFFSET          5
#define PLL_EN_CTRL_XPLL_EN_OFFSET            7
#define PLL_TOPPLL_0                (PLL_CTRL_BASE    + 0x010)
#define PLL_TOPPLL_1                (PLL_CTRL_BASE    + 0x014)
#define PLL_TOPPLL_2                (PLL_CTRL_BASE    + 0x018)
#define PLL_TOPPLL_3                (PLL_CTRL_BASE    + 0x01C)
#define PLL_USBPLL_0                (PLL_CTRL_BASE    + 0x020)
#define PLL_USBPLL_1                (PLL_CTRL_BASE    + 0x024)
#define PLL_USBPLL_2                (PLL_CTRL_BASE    + 0x028)
#define PLL_USBPLL_3                (PLL_CTRL_BASE    + 0x02C)
#define PLL_DSPPLL_0                (PLL_CTRL_BASE    + 0x030)
#define PLL_DSPPLL_1                (PLL_CTRL_BASE    + 0x034)
#define PLL_DSPPLL_2                (PLL_CTRL_BASE    + 0x038)
#define PLL_DSPPLL_3                (PLL_CTRL_BASE    + 0x03C)
#define PLL_TUNER_0                 (PLL_CTRL_BASE    + 0x050)
#define PLL_TUNER_1                 (PLL_CTRL_BASE    + 0x054)
#define TOPPLL_FLOWCTRL             (PLL_CTRL_BASE    + 0x084)
#define TOPPLL_FLOWCTRL_TOPPLL_RDY_OFFSET     1
#define USBPLL_FLOWCTRL             (PLL_CTRL_BASE    + 0x0A4)
#define USBPLL_FLOWCTRL_USBPLL_RDY_OFFSET     1
#define DSPPLL_FLOWCTRL             (PLL_CTRL_BASE    + 0x0C4)
#define DSPPLL_FLOWCTRL_DSPPLL_RDY_OFFSET     1
#define XPLL_FLOWCTRL               (PLL_CTRL_BASE    + 0x0E4)
#define XPLL_FLOWCTRL_XPLL_RDY_OFFSET         1

#define PLL_CK_CG                   (TOP_CLK_OFF_BASE + 0x0200)
#define PLL_CK_CG_PLL_CG_EN_MASK             0x1
#define PLL_CK_CG_TOPPLL_CG_EN_OFFSET        0
#define PLL_CK_CG_DSPPLL_CG_EN_OFFSET        1
#define PLL_CK_CG_USBPLL_CG_EN_OFFSET        2
#define PLL_CK_CG_XPLL_CG_EN_OFFSET          3
#define PLL_CK_CG_TOPPLL_MON_CG_EN_OFFSET    4
#define PLL_CK_CG_DSPPLL_MON_CG_EN_OFFSET    5
#define PLL_CK_CG_USBPLL_MON_CG_EN_OFFSET    6
#define PLL_CK_CG_XPLL_MON_CG_EN_OFFSET      7

#define XTAL_XO_TOP_1           (XTAL_CTRL_BASE + 0x0B00)
#define XTAL_XO_POS_EN          (XTAL_CTRL_BASE + 0x0B10)
#define XTAL_XO_TRANSITION      (XTAL_CTRL_BASE + 0x0B18)
#define XTAL_XO_C1_TRIM         (XTAL_CTRL_BASE + 0x0B28)
#define XTAL_XO_C2_TRIM         (XTAL_CTRL_BASE + 0x0B38)
#define RG_XO_CTRL_39           (XTAL_CTRL_BASE + 0x0BC0)

#define XTAL_LPM_EN             (BIT(5))
#define RG_TOP_XO_7             XTAL_XO_TRANSITION

#define GRP2_EEF_TOP_BASE_ADDR  (0x30406000)
#define GRP2_EEF_EFS_BLK12      (GRP2_EEF_TOP_BASE_ADDR + 0xB0)
#define GRP2_EEF_EFS_BLK13      (GRP2_EEF_TOP_BASE_ADDR + 0xB4)

#define INFRA_BUS_DCM_CTRL      (INFRA_BCRM_AON_BASE + 0x070)

#define REG32_READ(_r) HAL_REG_32(_r)
#define REG32_WRITE(_r, _v) HAL_REG_32(_r) = (_v)

#define ABS_DIFF(a, b)  ((a) > (b)? (a) - (b) : (b) - (a))

#define FREDETCR0_SW_DEFAULT (0x3F00000)
#ifdef HAL_CLOCK_MODULE_ENABLED

/* ----------------------------------------------------------------------------
   -- Core clock
   ---------------------------------------------------------------------------- */
#ifdef MTK_FPGA_ENABLE
#define XTAL_FREQ       MHZ(24)
#else /* #ifdef MTK_FPGA_ENABLE */
#define XTAL_FREQ       MHZ(26)
#endif /* #ifdef MTK_FPGA_ENABLE */

uint32_t SystemCoreClock = MHZ(26);
/* ----------------------------------------------------------------------------
   -- Internal functions
   ---------------------------------------------------------------------------- */

/*----------------------------------------------------------------------------
  Clock functions
 *----------------------------------------------------------------------------*/
ATTR_TEXT_IN_SYSRAM void SystemCoreClockUpdate(void)             /* Get Core Clock Frequency      */
{
#ifdef MTK_FPGA_ENABLE
    SystemCoreClock = hal_clock_get_mcu_clock_frequency() / 2;
#else /* #ifdef MTK_FPGA_ENABLE */
    SystemCoreClock = hal_clock_get_mcu_clock_frequency();
#endif /* #ifdef MTK_FPGA_ENABLE */
}

void hal_clk_hclk_prot_en(void)
{
    REG32_WRITE(CM33_CLK_CTL, REG32_READ(CM33_CLK_CTL) | BIT(24));
}

ATTR_TEXT_IN_SYSRAM uint32_t hal_clk_get_timeout_by_cm33_clk(uint32_t us)
{
    return hal_clock_get_mcu_clock_frequency() * us / 1000000;
}

struct clk_cg_pll_field_tbl_t {
    uint32_t fc_reg;
    uint32_t pll_en_offset: 16;
    uint32_t ck_cg_offset: 16;
};

ATTR_RODATA_IN_SYSRAM static const struct clk_cg_pll_field_tbl_t clk_cg_pll_field_tbl[] = {
    {TOPPLL_FLOWCTRL, PLL_EN_CTRL_TOPPLL_EN_OFFSET, PLL_CK_CG_TOPPLL_CG_EN_OFFSET},     // HAL_CLOCK_CG_TOP_PLL
    {DSPPLL_FLOWCTRL, PLL_EN_CTRL_DSPPLL_EN_OFFSET, PLL_CK_CG_DSPPLL_CG_EN_OFFSET},     // HAL_CLOCK_CG_DSP_PLL
    {USBPLL_FLOWCTRL, PLL_EN_CTRL_USBPLL_EN_OFFSET, PLL_CK_CG_USBPLL_CG_EN_OFFSET},     // HAL_CLOCK_CG_USB_PLL
    {XPLL_FLOWCTRL,   PLL_EN_CTRL_XPLL_EN_OFFSET,   PLL_CK_CG_XPLL_CG_EN_OFFSET},       // HAL_CLOCK_CG_XPLL
};
STATIC_ASSERT(sizeof(clk_cg_pll_field_tbl) / sizeof(struct clk_cg_pll_field_tbl_t) == (HAL_CLOCK_CG_PLL_END - HAL_CLOCK_CG_PLL_BEGIN + 1));

struct clk_cg_ctl_field_tbl_t {
    uint32_t reg;
    uint8_t  en_bit;
    uint8_t  is_inverse: 1;
};

ATTR_RODATA_IN_SYSRAM static const struct clk_cg_ctl_field_tbl_t clk_cg_ctl_field_tbl[] = {
    {DSP_CLK_CTL,          31, FALSE},  // HAL_CLOCK_CG_DSP_HCLK
    {AXI_CLK_CTL,          31, FALSE},  // HAL_CLOCK_CG_INFRA_BUS,
    {AUDSYS_BUS_CLK_CTL,   31, FALSE},  // HAL_CLOCK_CG_AUDSYS_BUS,
    {PSRAM_CLK_CTL,        31, FALSE},  // HAL_CLOCK_CG_PSRAM,
    {TRACE_CLK_CTL,        31, FALSE},  // HAL_CLOCK_CG_TRACE,
    {PSRAMAXI_CLK_CTL,     31, FALSE},  // HAL_CLOCK_CG_PSRAM_AXI,
    {FLASH_CLK_CTL,        31, FALSE},  // HAL_CLOCK_CG_FLASH,
    {GCPU_CLK_CTL,         31, FALSE},  // HAL_CLOCK_CG_GCPU,
    {ECC_CLK_CTL,          31, FALSE},  // HAL_CLOCK_CG_ECC,
    {SPIM0_CLK_CTL,        31, FALSE},  // HAL_CLOCK_CG_SPIM0,
    {SPIM1_CLK_CTL,        31, FALSE},  // HAL_CLOCK_CG_SPIM1,
    {SPIS_CLK_CTL,         31, FALSE},  // HAL_CLOCK_CG_SPIS,
    {I2C_CLK_CTL,          29, FALSE},  // HAL_CLOCK_CG_I2C0,
    {I2C_CLK_CTL,          28, FALSE},  // HAL_CLOCK_CG_I2C1,
    {DBG_CLK_CTL,          31, FALSE},  // HAL_CLOCK_CG_DBG,
    {SDIOM_CLK_CTL,        31, FALSE},  // HAL_CLOCK_CG_SDIOM,
    {USB_CLK_CTL,          30, FALSE},  // HAL_CLOCK_CG_USB_SYS,
    {USB_CLK_CTL,          31, FALSE},  // HAL_CLOCK_CG_USB_XHCI,
    {AUX_ADC_CLK_CTL,      30, FALSE},  // HAL_CLOCK_CG_AUX_ADC_DIV,
    {PWM_CLK_DIV_CTL,      30, FALSE},  // HAL_CLOCK_CG_PWM_DIV,
    {CLK_AUDDIV_0,         00, TRUE},  // HAL_CLOCK_CG_APLL12_CK_DIV0,
    {CLK_AUDDIV_0,         01, TRUE},  // HAL_CLOCK_CG_APLL12_CK_DIV3,
    {CLK_AUDDIV_0,         02, TRUE},  // HAL_CLOCK_CG_APLL12_CK_DIV6,
};
STATIC_ASSERT(sizeof(clk_cg_ctl_field_tbl) / sizeof(struct clk_cg_ctl_field_tbl_t) == (HAL_CLOCK_CG_CTL_END - HAL_CLOCK_CG_CTL_BEGIN + 1));

STATIC_ASSERT(HAL_CLOCK_CG_XTAL_END < HAL_CLOCK_CG_PLL_END);
STATIC_ASSERT(HAL_CLOCK_CG_PLL_END < HAL_CLOCK_CG_CTL_END);
/**
 * @brief       This function enables a specific CG clock.
 * @param[in]   clock_id is a unique clock identifier.
 * @return      #HAL_CLOCK_STATUS_OK, if the operation completed successfully.\n
 *              #HAL_CLOCK_STATUS_UNINITIALIZED, if the clock driver is not initialized.\n
 *              #HAL_CLOCK_STATUS_INVALID_PARAMETER, if the input parameter is invalid.\n
 *              #HAL_CLOCK_STATUS_ERROR, if the clock function detected a common error.\n
 */

static const uint32_t clk_xtal_bitmap = 0x307FFFFF;
static bool clk_i2c_0_is_enable = false;

#define BIT_SETCLEAR(val, bit, set)  ((set)?((val) | BIT(bit)):((val) & ~(BIT(bit))))

ATTR_TEXT_IN_SYSRAM hal_clock_status_t hal_clock_enable(hal_clock_cg_id clock_id)
{
    // clock already enabled
    if (hal_clock_is_enabled(clock_id)) {
        return HAL_CLOCK_STATUS_OK;
    }

    if (clock_id <= HAL_CLOCK_CG_XTAL_END && (clk_xtal_bitmap & BIT(clock_id))) {
        REG32_WRITE(XTAL_CLK_CTRL, REG32_READ(XTAL_CLK_CTRL) | BIT(clock_id));

    } else if (clock_id >= HAL_CLOCK_CG_PLL_BEGIN && clock_id <= HAL_CLOCK_CG_PLL_END) {
        const struct clk_cg_pll_field_tbl_t *f = &clk_cg_pll_field_tbl[clock_id - HAL_CLOCK_CG_PLL_BEGIN];
        uint32_t timeout = hal_clk_get_timeout_by_cm33_clk(100); // 0.1ms

        /* TODO: USB VCO frquency PLL setting workaround
         * USB need to set some magic number to PLL controller
         * currently clock driver did not support PLL control
         * therefore hardcode the USB PLL setting
         */
        if (clock_id == HAL_CLOCK_CG_USB_PLL) {
            REG32_WRITE(PLL_USBPLL_3, 0x76276276);
            REG32_WRITE(PLL_USBPLL_0, 0x2);
            REG32_WRITE(PLL_USBPLL_0, 0x3);
            REG32_WRITE(PLL_USBPLL_1, 0x4000FF00);
        }

        // Enable PLL
        REG32_WRITE(PLL_EN_CTRL, REG32_READ(PLL_EN_CTRL) | BIT(f->pll_en_offset));

        // polling PLL ready
        while (!(REG32_READ(f->fc_reg) & BIT(1))) {
            if (timeout-- == 0) { // 1ms timeout @ 26MHz
                log_hal_error("%s: PLL enable timeout!\n", __func__);
                break;
            }
        }

        REG32_WRITE(PLL_CK_CG, REG32_READ(PLL_CK_CG) & ~(BIT(f->ck_cg_offset)));
        REG32_WRITE(PLL_CK_CG, REG32_READ(PLL_CK_CG) | BIT(f->ck_cg_offset));
    } else if (clock_id >= HAL_CLOCK_CG_CTL_BEGIN && clock_id <= HAL_CLOCK_CG_CTL_END) {
        const struct clk_cg_ctl_field_tbl_t *f = &clk_cg_ctl_field_tbl[clock_id - HAL_CLOCK_CG_CTL_BEGIN];

        REG32_WRITE(f->reg, BIT_SETCLEAR(REG32_READ(f->reg), f->en_bit, (f->is_inverse == FALSE)));

        // i2c_0 and i2c_1 use same clock source, let clock api compatiable with E1 and others
        if (hal_boot_get_hw_ver() != 0x8A00) {
            switch (clock_id) {
                case HAL_CLOCK_CG_I2C0:
                    clk_i2c_0_is_enable = true;
                    break;
                case HAL_CLOCK_CG_I2C1:
                    if (!hal_clock_is_enabled(HAL_CLOCK_CG_I2C0)) {
                        HAL_REG_32(0x30020290) |= BIT(29);
                    }
                    break;
                default:
                    break;
            }
        }
    } else {
        return HAL_CLOCK_STATUS_INVALID_PARAMETER;
    }
    return HAL_CLOCK_STATUS_OK;
}

/**
 * @brief       This function disables a specific CG clock.
 * @param[in]   clock_id is a unique clock identifier.
 * @return      #HAL_CLOCK_STATUS_OK, if the operation completed successfully.\n
 *              #HAL_CLOCK_STATUS_UNINITIALIZED, if the clock driver is not initialized.\n
 *              #HAL_CLOCK_STATUS_INVALID_PARAMETER, if the input parameter is invalid.\n
 *              #HAL_CLOCK_STATUS_ERROR, if the clock function detected a common error.\n
 */
ATTR_TEXT_IN_SYSRAM hal_clock_status_t hal_clock_disable(hal_clock_cg_id clock_id)
{
    if (clock_id <= HAL_CLOCK_CG_XTAL_END && (clk_xtal_bitmap & BIT(clock_id))) {
        REG32_WRITE(XTAL_CLK_CTRL, REG32_READ(XTAL_CLK_CTRL) & ~(BIT(clock_id)));

    } else if (clock_id >= HAL_CLOCK_CG_PLL_BEGIN && clock_id <= HAL_CLOCK_CG_PLL_END) {
        const struct clk_cg_pll_field_tbl_t *f = &clk_cg_pll_field_tbl[clock_id - HAL_CLOCK_CG_PLL_BEGIN];

        REG32_WRITE(PLL_CK_CG, REG32_READ(PLL_CK_CG) & ~(BIT(f->ck_cg_offset)));
        REG32_WRITE(PLL_EN_CTRL, REG32_READ(PLL_EN_CTRL) & ~(BIT(f->pll_en_offset)));

    } else if (clock_id >= HAL_CLOCK_CG_CTL_BEGIN && clock_id <= HAL_CLOCK_CG_CTL_END) {
        const struct clk_cg_ctl_field_tbl_t *f = &clk_cg_ctl_field_tbl[clock_id - HAL_CLOCK_CG_CTL_BEGIN];

        // i2c_0 and i2c_1 use same clock source, let clock api compatiable with E1 and E2 IC
        if (hal_boot_get_hw_ver() != 0x8A00) {
            switch (clock_id) {
                case HAL_CLOCK_CG_I2C0:
                    clk_i2c_0_is_enable = false;
                    if (hal_clock_is_enabled(HAL_CLOCK_CG_I2C1)) {
                        return HAL_CLOCK_STATUS_OK;
                    }
                    break;
                case HAL_CLOCK_CG_I2C1:
                    if (!clk_i2c_0_is_enable) {
                        HAL_REG_32(0x30020290) &= ~BIT(29);
                    }
                    break;
                default:
                    break;
            }
        }
        REG32_WRITE(f->reg, BIT_SETCLEAR(REG32_READ(f->reg), f->en_bit, (f->is_inverse == TRUE)));

    } else {
        return HAL_CLOCK_STATUS_INVALID_PARAMETER;
    }
    return HAL_CLOCK_STATUS_OK;
}

/**
 * @brief       This function queries the status of a specific CG clock.
 * @param[in]   clock_id is a unique clock identifier.
 * @return      true, if the specified clock is enabled.\n
 *              false, if the specified clock is disabled or the clock driver is not initialized.\n
 */
ATTR_TEXT_IN_SYSRAM bool hal_clock_is_enabled(hal_clock_cg_id clock_id)
{
    if (clock_id <= HAL_CLOCK_CG_XTAL_END && (clk_xtal_bitmap & BIT(clock_id))) {
        return ((REG32_READ(XTAL_CLK_CTRL) & BIT((uint32_t)clock_id)) != 0);

    } else if (clock_id >= HAL_CLOCK_CG_PLL_BEGIN && clock_id <= HAL_CLOCK_CG_PLL_END) {
        const struct clk_cg_pll_field_tbl_t *f = &clk_cg_pll_field_tbl[clock_id - HAL_CLOCK_CG_PLL_BEGIN];

        return ((REG32_READ(PLL_EN_CTRL) & BIT(f->pll_en_offset)) != 0);
    } else if (clock_id >= HAL_CLOCK_CG_CTL_BEGIN && clock_id <= HAL_CLOCK_CG_CTL_END) {
        const struct clk_cg_ctl_field_tbl_t *f = &clk_cg_ctl_field_tbl[clock_id - HAL_CLOCK_CG_CTL_BEGIN];

        return (((REG32_READ(f->reg) >> f->en_bit) & 0x1) ^ f->is_inverse);
    }

    return FALSE;
}


struct clk_sel_pll_cr_val_t {
    uint32_t sdm_pcw;
    uint32_t pll_cr1;
};

// ref doc: MockingBird_PLL_setting.xlsx
ATTR_RODATA_IN_SYSRAM static const struct clk_sel_pll_cr_val_t clk_sel_pll_cr_val[] = {
    {0x2E276276, 0x20000000 }, // 300 MHz
    {0x3D89D89D, 0x20000000 }, // 400 MHz
    {0x26762762, 0x10000000 }, // 500 MHz
    {0x2E276276, 0x10000000 }, // 600 MHz
    {0x5c4ec4ec, 0x10000000 }, // 1200 MHz
    {0x5bd89d89, 0x10000000 }  // 1194 MHz
};

#define NUM_CLKSEL_PLL_CR_VAL (sizeof(clk_sel_pll_cr_val)/sizeof(struct clk_sel_pll_cr_val_t))
#define PLL_CR1_CTL_BITMASK (0x73000000)
#define PLL_CTRL_0_OFFSET (0x00)
#define PLL_CTRL_1_OFFSET (0x04)
#define PLL_CTRL_2_OFFSET (0x08)
#define PLL_CTRL_3_OFFSET (0x0C)


ATTR_TEXT_IN_SYSRAM hal_clock_status_t clk_pll_select(hal_clock_cg_id cg_id, uint32_t pll_ctrl_base, uint32_t clksel)
{
    bool is_enable = hal_clock_is_enabled(cg_id);
    hal_clock_sel_id clock_id = CLKSEL_FIELD_CLOCK_ID(clksel);
    uint32_t sel = CLKSEL_FIELD_CLKSEL(clksel);
    uint32_t reg_data;
    uint32_t cm33_clksel = 0, infra_bus_clksel = 0;

    if (sel >= NUM_CLKSEL_PLL_CR_VAL) {
        return HAL_CLOCK_STATUS_INVALID_PARAMETER;
    }

    if (is_enable) {
        if (clock_id == HAL_CLOCK_SEL_TOP_PLL) {
            hal_clock_get_selected_mux(HAL_CLOCK_SEL_CM33_HCLK, &cm33_clksel);
            hal_clock_get_selected_mux(HAL_CLOCK_SEL_INFRA_BUS, &infra_bus_clksel);
            hal_clock_mux_select(HAL_CLOCK_SEL_CM33_HCLK, CLK_CM33_HCLK_CLKSEL_XTAL);
            hal_clock_mux_select(HAL_CLOCK_SEL_INFRA_BUS, CLK_INFRA_BUS_CLKSEL_XTAL);
        }
        hal_clock_disable(cg_id);
    }

    const struct clk_sel_pll_cr_val_t *f = &clk_sel_pll_cr_val[sel];
    REG32_WRITE((pll_ctrl_base + PLL_CTRL_3_OFFSET), f->sdm_pcw);

    reg_data = REG32_READ((pll_ctrl_base + PLL_CTRL_1_OFFSET));
    reg_data &= ~PLL_CR1_CTL_BITMASK;
    reg_data |= f->pll_cr1;
    REG32_WRITE((pll_ctrl_base + PLL_CTRL_1_OFFSET), reg_data);

    if (is_enable) {
        hal_clock_enable(cg_id);
        if (clock_id == HAL_CLOCK_SEL_TOP_PLL) {
            hal_clock_mux_select(HAL_CLOCK_SEL_INFRA_BUS, infra_bus_clksel);
            hal_clock_mux_select(HAL_CLOCK_SEL_CM33_HCLK, cm33_clksel);
        }
    }

    return HAL_CLOCK_STATUS_OK;
}

ATTR_TEXT_IN_SYSRAM hal_clock_status_t clk_pll_get_selected_mux(hal_clock_sel_id clock_id, uint32_t pll_ctrl_base, uint32_t *clksel)
{
    ASSERT(clksel);

    uint32_t sdm_pcw = REG32_READ((pll_ctrl_base + PLL_CTRL_3_OFFSET));
    uint32_t pll_cr1 = REG32_READ((pll_ctrl_base + PLL_CTRL_1_OFFSET)) & PLL_CR1_CTL_BITMASK;
    uint32_t i;

    for (i = 0; i < NUM_CLKSEL_PLL_CR_VAL; i++) {
        if (sdm_pcw == clk_sel_pll_cr_val[i].sdm_pcw && pll_cr1 == clk_sel_pll_cr_val[i].pll_cr1) {
            *clksel = CLKSEL_FIELD(clock_id, 0, i);
            return HAL_CLOCK_STATUS_OK;
        }
    }

    *clksel = 0;
    return HAL_CLOCK_STATUS_ERROR;
}

#define CLKSEL_NO_DIV   (0xFF)

typedef enum  {
    CLK_SRC_OTHERS,
    CLK_SRC_XTAL,
    CLK_SRC_F32K,
    CLK_SRC_TOP_PLL,
    CLK_SRC_USB_PLL,
    CLK_SRC_DSP_PLL,
    CLK_SRC_X_PLL,

    CLK_SRC_MAX
} clk_src_t;

struct clk_sel_field_tbl_t {
    uint32_t reg;
    uint16_t clksel_offset: 5;
    uint16_t stactive_offset: 5; // offset=0 means no active bit (don't care)
    uint16_t divsel_bits: 4;
    uint16_t clksel_bits: 2;
    uint8_t clksel_max;
    uint8_t clksel_div;
    uint8_t clksrc_tbl[8];
};

ATTR_RODATA_IN_SYSRAM static const struct clk_sel_field_tbl_t clk_sel_field_tbl[] = {
    {AUDIO_FAUDIO_CLK_SEL,      0, 16, 0, 2, 3, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_TOP_PLL, CLK_SRC_X_PLL}},                     // HAL_CLOCK_SEL_AUDIO_FAUDIO
    {AUDIO_FASM_CLK_SEL,        0, 16, 0, 1, 2, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_TOP_PLL}},                                    // HAL_CLOCK_SEL_AUDIO_FASM
    {AUDIO_FASYS_CLK_SEL,       0, 16, 0, 2, 4, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_X_PLL, CLK_SRC_X_PLL, CLK_SRC_X_PLL}},        // HAL_CLOCK_SEL_AUDIO_FASYS
    {AUDIO_HAPLL_CLK_SEL,       0, 16, 0, 2, 4, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_X_PLL, CLK_SRC_X_PLL, CLK_SRC_X_PLL}},        // HAL_CLOCK_SEL_AUDIO_HAPLL
    {AUDIO_FAUD_INTBUS_CLK_SEL, 0, 16, 0, 2, 4, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL, CLK_SRC_X_PLL}},    // HAL_CLOCK_SEL_AUDIO_FAUD_INTBUS
    {DSP_CLK_CTL,               0, 16, 0, 3, 8, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_XTAL, CLK_SRC_F32K, CLK_SRC_DSP_PLL, CLK_SRC_DSP_PLL, CLK_SRC_DSP_PLL, CLK_SRC_DSP_PLL, CLK_SRC_X_PLL}},     // HAL_CLOCK_SEL_DSP_HCLK
    {AXI_CLK_CTL,               0, 16, 4, 2, 4, 3,             {CLK_SRC_XTAL, CLK_SRC_F32K, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL}},     // HAL_CLOCK_SEL_INFRA_BUS
    {AUDSYS_BUS_CLK_CTL,        0, 12, 4, 2, 4, 3,             {CLK_SRC_XTAL, CLK_SRC_F32K, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL}},     // HAL_CLOCK_SEL_AUDSYS_BUS
    {PSRAM_CLK_CTL,             0, 0,  4, 1, 2, 1,             {CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL}},                                 // HAL_CLOCK_SEL_PSRAM
    {TRACE_CLK_CTL,             0, 16, 4, 1, 2, 1,             {CLK_SRC_XTAL, CLK_SRC_TOP_PLL}},                                    // HAL_CLOCK_SEL_TRACE
    {PSRAMAXI_CLK_CTL,          0, 16, 0, 1, 2, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_TOP_PLL}},                                    // HAL_CLOCK_SEL_PSRAM_AXI
    {FLASH_CLK_CTL,             0, 16, 4, 1, 2, 1,             {CLK_SRC_XTAL, CLK_SRC_TOP_PLL}},                                    // HAL_CLOCK_SEL_FLASH
    {GCPU_CLK_CTL,              0, 16, 3, 1, 2, 1,             {CLK_SRC_XTAL, CLK_SRC_TOP_PLL}},                                    // HAL_CLOCK_SEL_GCPU
    {ECC_CLK_CTL,               0, 16, 3, 1, 2, 1,             {CLK_SRC_XTAL, CLK_SRC_TOP_PLL}},                                    // HAL_CLOCK_SEL_ECC
    {SPIM0_CLK_CTL,             0, 16, 4, 2, 4, 1,             {CLK_SRC_XTAL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL}},  // HAL_CLOCK_SEL_SPIM0
    {SPIM1_CLK_CTL,             0, 16, 4, 2, 4, 1,             {CLK_SRC_XTAL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL}},  // HAL_CLOCK_SEL_SPIM1
    {SPIS_CLK_CTL,              0, 16, 4, 1, 2, 1,             {CLK_SRC_XTAL, CLK_SRC_TOP_PLL}},                                    // HAL_CLOCK_SEL_SPIS
    {I2C_CLK_CTL,               0, 16, 3, 1, 2, 1,             {CLK_SRC_XTAL, CLK_SRC_TOP_PLL}},                                    // HAL_CLOCK_SEL_I2C
    {DBG_CLK_CTL,               0, 16, 0, 2, 4, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL}},  // HAL_CLOCK_SEL_DBG
    {SDIOM_CLK_CTL,             0, 16, 0, 1, 2, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_TOP_PLL}},                                    // HAL_CLOCK_SEL_SDIOM
    {USB_CLK_CTL,               0, 16, 0, 2, 4, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL}},  // HAL_CLOCK_SEL_USB_SYS
    {USB_CLK_CTL,               2, 20, 0, 2, 4, CLKSEL_NO_DIV, {CLK_SRC_XTAL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL, CLK_SRC_TOP_PLL}},  // HAL_CLOCK_SEL_USB_XHCI
    {AUX_ADC_CLK_CTL,           0, 0,  5, 0, 1, 0,             {CLK_SRC_XTAL}},                                                     // HAL_CLOCK_SEL_AUX_ADC_DIV
    {PWM_CLK_DIV_CTL,           0, 0,  5, 0, 1, 0,             {CLK_SRC_XTAL}},                                                     // HAL_CLOCK_SEL_PWM_DIV
    {F32K_CLK_CTL,              0, 16, 11, 2, 3, 2,             {CLK_SRC_OTHERS, CLK_SRC_OTHERS, CLK_SRC_XTAL}},                    // HAL_CLOCK_SEL_F32K,
    {CM33_CLK_CTL,              0, 16, 4, 1, 2, 1,             {CLK_SRC_XTAL, CLK_SRC_TOP_PLL}},                                    // HAL_CLOCK_SEL_CM33_HCLK
};
STATIC_ASSERT(sizeof(clk_sel_field_tbl) / sizeof(struct clk_sel_field_tbl_t) == (HAL_CLOCK_SEL_TBL_END - HAL_CLOCK_SEL_TBL_BEGIN + 1));


ATTR_TEXT_IN_SYSRAM hal_clock_status_t hal_clock_mux_select(hal_clock_sel_id clock_id, uint32_t clk_sel_field)
{
    if (clock_id < 1 || clock_id >= HAL_CLOCK_SEL_MAX || CLKSEL_FIELD_CLOCK_ID(clk_sel_field) != clock_id) {
        return HAL_CLOCK_STATUS_INVALID_PARAMETER;
    }

    if (clock_id == HAL_CLOCK_SEL_DSP_PLL) {
        return clk_pll_select(HAL_CLOCK_CG_DSP_PLL, PLL_DSPPLL_0, clk_sel_field);
    } else if (clock_id == HAL_CLOCK_SEL_TOP_PLL) {
        return clk_pll_select(HAL_CLOCK_CG_TOP_PLL, PLL_TOPPLL_0, clk_sel_field);
    }

    if (clock_id > HAL_CLOCK_SEL_TBL_END) {
        return HAL_CLOCK_STATUS_NOT_SUPPORT;
    }

    uint32_t clk_sel = CLKSEL_FIELD_CLKSEL(clk_sel_field);
    uint32_t div_sel = CLKSEL_FIELD_DIVSEL(clk_sel_field);
    const struct clk_sel_field_tbl_t *sf = &clk_sel_field_tbl[clock_id - 1];

    if (clk_sel >= sf->clksel_max) {
        return HAL_CLOCK_STATUS_INVALID_PARAMETER;
    }

    uint32_t reg_data = REG32_READ(sf->reg);

    // set div_en=1 and div_sel if clock source is div
    if (clk_sel == sf->clksel_div) {
        if (div_sel >= (uint32_t)(1 << sf->divsel_bits)) {
            return HAL_CLOCK_STATUS_INVALID_PARAMETER;
        }

        reg_data |= BIT(30);
        REG_FLD_SET(REG_FLD(sf->divsel_bits, 4), reg_data, div_sel);

        REG32_WRITE(sf->reg, reg_data);

        hal_gpt_delay_us(1);
    }

    // handle special case CM33
    if (clock_id == HAL_CLOCK_SEL_CM33_HCLK) {
        if (clk_sel_field == CLK_CM33_HCLK_CLKSEL_F32K) {
            reg_data |= BIT(27);
        } else {
            reg_data &= ~(BIT(27));
        }
    }

    if (clock_id == HAL_CLOCK_SEL_F32K) {
        /* Checking F32K clock source, if clock source is using internal 32K
         * then VAD request need to be pull high.
         * */
        if (clk_sel_field == CLK_F32K_CLKSEL_XTAL_DIV_32K) {
            HAL_REG_32(XTAL_XO_TOP_1) |= XTAL_LPM_EN;
        } else {
            HAL_REG_32(XTAL_XO_TOP_1) &= ~XTAL_LPM_EN;
        }
    }
    // set clk_sel
    if (sf->clksel_bits) {
        REG_FLD_SET(REG_FLD(sf->clksel_bits, sf->clksel_offset), reg_data, clk_sel);
        REG32_WRITE(sf->reg, reg_data);

        // wait for st_active ready
        if (sf->stactive_offset) {
            for (reg_data = REG32_READ(sf->reg); !(REG_FLD_GET(REG_FLD(sf->clksel_max, sf->stactive_offset), reg_data) & BIT(clk_sel)); reg_data = REG32_READ(sf->reg));
        }
    }

    // set div_en=0 if clock source is not div
    if (sf->clksel_div != CLKSEL_NO_DIV && clk_sel != sf->clksel_div) {
        reg_data &= ~(BIT(30));
        REG32_WRITE(sf->reg, reg_data);
    }

    if (clock_id == HAL_CLOCK_SEL_CM33_HCLK) {
        SystemCoreClockUpdate();
    }

    return HAL_CLOCK_STATUS_OK;
}


ATTR_TEXT_IN_SYSRAM hal_clock_status_t hal_clock_get_selected_mux(hal_clock_sel_id clock_id, uint32_t *clk_sel_field)
{
    if (clock_id < 1 || clock_id >= HAL_CLOCK_SEL_MAX) {
        return HAL_CLOCK_STATUS_INVALID_PARAMETER;
    }

    ASSERT(clk_sel_field);

    if (clock_id == HAL_CLOCK_SEL_DSP_PLL) {
        return clk_pll_get_selected_mux(HAL_CLOCK_SEL_DSP_PLL, PLL_DSPPLL_0, clk_sel_field);
    } else if (clock_id == HAL_CLOCK_SEL_TOP_PLL) {
        return clk_pll_get_selected_mux(HAL_CLOCK_SEL_TOP_PLL, PLL_TOPPLL_0, clk_sel_field);
    }

    if (clock_id > HAL_CLOCK_SEL_TBL_END) {
        return HAL_CLOCK_STATUS_NOT_SUPPORT;
    }

    const struct clk_sel_field_tbl_t *sf = &clk_sel_field_tbl[clock_id - 1];

    uint32_t reg_data = REG32_READ(sf->reg);

    // handle special case
    if (clock_id == HAL_CLOCK_SEL_CM33_HCLK && (reg_data & BIT(27))) {
        *clk_sel_field = CLK_CM33_HCLK_CLKSEL_F32K;
        return HAL_CLOCK_STATUS_OK;
    }

    uint32_t clk_sel = REG_FLD_GET(REG_FLD(sf->clksel_bits, sf->clksel_offset), reg_data);
    uint32_t div_sel = (clk_sel == sf->clksel_div ? REG_FLD_GET(REG_FLD(sf->divsel_bits, 4), reg_data) : 0);

    *clk_sel_field = CLKSEL_FIELD(clock_id, div_sel, clk_sel);

    return HAL_CLOCK_STATUS_OK;
}

STATIC_ASSERT(HAL_CLOCK_SEL_APLL12_CK_DIV3 == HAL_CLOCK_SEL_APLL12_CK_DIV0 + 1);
STATIC_ASSERT(HAL_CLOCK_SEL_APLL12_CK_DIV6 == HAL_CLOCK_SEL_APLL12_CK_DIV3 + 1);

static uint32_t pll_sel_mux_rate[] = {MHZ(300), MHZ(400), MHZ(500), MHZ(600), MHZ(1200), MHZ(1194)};
static uint32_t apll_div_sel_mux_rate[] = {KHZ(196608)};
static uint32_t cm33_sel_mux_rate[] = {XTAL_FREQ, MHZ(600)};


ATTR_TEXT_IN_SYSRAM hal_clock_status_t hal_clock_set_rate(hal_clock_sel_id clock_id, uint32_t hz)
{
    if (clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV0 ||
        clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV3 ||
        clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV6) {

        if (hz == 0) {
            return HAL_CLOCK_STATUS_INVALID_PARAMETER;
        }
        uint32_t div = ((apll_div_sel_mux_rate[0] - 1) / hz);
        if (div > 0 && hz > (apll_div_sel_mux_rate[0] / (div + 1) + apll_div_sel_mux_rate[0] / div) / 2) {
            div -= 1;
        }
        uint32_t shift = 8 * (clock_id - HAL_CLOCK_SEL_APLL12_CK_DIV0);

        if (div >= (1 << 8)) {
            return HAL_CLOCK_STATUS_INVALID_PARAMETER;
        }

        uint32_t reg_data = REG32_READ(CLK_AUDDIV_1);
        reg_data &= ~(0xFF << shift);
        reg_data |= (div << shift);
        REG32_WRITE(CLK_AUDDIV_1, reg_data);
    } else {
        return HAL_CLOCK_STATUS_NOT_SUPPORT;
    }
    return HAL_CLOCK_STATUS_OK;
}


ATTR_TEXT_IN_SYSRAM hal_clock_status_t hal_clock_get_rate(hal_clock_sel_id clock_id, uint32_t *hz)
{
    uint32_t clk_sel_field;
    uint32_t *sel_mux_rate_tbl;

    if (!hz) {
        return HAL_CLOCK_STATUS_INVALID_PARAMETER;
    }

    if (clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV0 ||
        clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV3 ||
        clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV6) {

        clk_sel_field = CLKSEL_FIELD(HAL_CLOCK_SEL_APLL12_CK_DIV0, (REG32_READ(CLK_AUDDIV_1) >> (8 * (clock_id - HAL_CLOCK_SEL_APLL12_CK_DIV0))) & 0xFF, 0);
        sel_mux_rate_tbl = apll_div_sel_mux_rate;

    } else {
        hal_clock_status_t ret = hal_clock_get_selected_mux(clock_id, &clk_sel_field);
        if (ret != HAL_CLOCK_STATUS_OK) {
            return ret;
        }

        if (clock_id == HAL_CLOCK_SEL_DSP_PLL || clock_id == HAL_CLOCK_SEL_TOP_PLL) {
            sel_mux_rate_tbl = pll_sel_mux_rate;

        } else if (clock_id == HAL_CLOCK_SEL_CM33_HCLK) {
            if (clk_sel_field == CLK_CM33_HCLK_CLKSEL_F32K) {
                *hz = KHZ(32);
                return HAL_CLOCK_STATUS_OK;
            }

            sel_mux_rate_tbl = cm33_sel_mux_rate;

        } else {
            return HAL_CLOCK_STATUS_NOT_SUPPORT;
        }
    }

    *hz = sel_mux_rate_tbl[CLKSEL_FIELD_CLKSEL(clk_sel_field)] / (CLKSEL_FIELD_DIVSEL(clk_sel_field) + 1);

    // Adjust frequency according to TOP PLL frequency shift
    if (clock_id <= HAL_CLOCK_SEL_TBL_END) {
        const struct clk_sel_field_tbl_t *sf = &clk_sel_field_tbl[clock_id - 1];
        if (sf->clksrc_tbl[CLKSEL_FIELD_CLKSEL(clk_sel_field)] == CLK_SRC_TOP_PLL) {
            uint32_t top_pll_hz = 0;
            hal_clock_get_rate(HAL_CLOCK_SEL_TOP_PLL, &top_pll_hz);
            *hz = (((*hz) / 1000) * (top_pll_hz / 1000000) / 1200) * 1000;
        }
    }


    return HAL_CLOCK_STATUS_OK;
}

void clk_dcxo_trim(void)
{
    // ref doc: MockingBird_XO_Trim_apply_to_ATOP_flow_20200629.docx
    // ref doc: MockingBird_auto-load check list.xlsx
    uint32_t xo_c12_sel_trim1, xo_c12_sel_trim2, xo_c12_sel_trim3, xo_c12_sel_trim4, xo_trim_flow;
    uint32_t trim_result_ax;
    uint32_t reg_data;

    // read xo_trim data from efuse auto load CR
    xo_c12_sel_trim1 = REG_FLD_GET(REG_FLD(8, 24), REG32_READ(GRP2_EEF_EFS_BLK12));
    xo_c12_sel_trim2 = REG_FLD_GET(REG_FLD(8,  0), REG32_READ(GRP2_EEF_EFS_BLK13));
    xo_c12_sel_trim3 = REG_FLD_GET(REG_FLD(8,  8), REG32_READ(GRP2_EEF_EFS_BLK13));
    xo_c12_sel_trim4 = REG_FLD_GET(REG_FLD(8, 16), REG32_READ(GRP2_EEF_EFS_BLK13));
    xo_trim_flow     = REG_FLD_GET(REG_FLD(8, 24), REG32_READ(GRP2_EEF_EFS_BLK13));

    if ((xo_trim_flow & BIT(0)) == 0) {
        /* Apply default value, tuned by SA */
        /* wr 0x300E0BC0 0xcc220000 */
        /* wr 0x300E0B18 0xafaf0000 */
        REG32_WRITE(RG_TOP_XO_7, 0xafaf0000);
        hal_gpt_delay_us(100);
        REG32_WRITE(RG_XO_CTRL_39, 0xcc220000);
        hal_gpt_delay_us(100);
        return;
    }

    if (xo_c12_sel_trim1 & BIT(7)) {
        trim_result_ax = (xo_c12_sel_trim1 & BITS(0, 6));
    } else if (xo_c12_sel_trim2 & BIT(7)) {
        trim_result_ax = (xo_c12_sel_trim2 & BITS(0, 6));
    } else if (xo_c12_sel_trim3 & BIT(7)) {
        trim_result_ax = (xo_c12_sel_trim3 & BITS(0, 6));
    } else if (xo_c12_sel_trim4 & BIT(7)) {
        trim_result_ax = (xo_c12_sel_trim4 & BITS(0, 6));
    } else {
        trim_result_ax = 50;
    }

    reg_data = REG32_READ(XTAL_XO_C1_TRIM);
    REG_FLD_SET(REG_FLD(8, 24), reg_data, trim_result_ax);
    REG32_WRITE(XTAL_XO_C1_TRIM, reg_data);

    reg_data = REG32_READ(XTAL_XO_C2_TRIM);
    REG_FLD_SET(REG_FLD(8, 24), reg_data, trim_result_ax);
    REG32_WRITE(XTAL_XO_C2_TRIM, reg_data);

    reg_data = REG32_READ(XTAL_XO_POS_EN);
    REG_FLD_SET(REG_FLD(2, 28), reg_data, 0x3);
    REG32_WRITE(XTAL_XO_POS_EN, reg_data);

    reg_data = REG32_READ(XTAL_XO_TRANSITION);
    REG_FLD_SET(REG_FLD(2, 0), reg_data, 0x2);
    REG32_WRITE(XTAL_XO_TRANSITION, reg_data);
    hal_gpt_delay_us(100);
    REG_FLD_SET(REG_FLD(2, 0), reg_data, 0x3);
    REG32_WRITE(XTAL_XO_TRANSITION, reg_data);
    hal_gpt_delay_us(100);
    REG_FLD_SET(REG_FLD(2, 0), reg_data, 0x2);
    REG32_WRITE(XTAL_XO_TRANSITION, reg_data);
    hal_gpt_delay_us(100);
    REG_FLD_SET(REG_FLD(2, 0), reg_data, 0x0);
    REG32_WRITE(XTAL_XO_TRANSITION, reg_data);
}

ATTR_RODATA_IN_SYSRAM static const hal_clock_sel_id clk_clksel_backup_list[] = {
    HAL_CLOCK_SEL_INFRA_BUS,
    HAL_CLOCK_SEL_CM33_HCLK,
    HAL_CLOCK_SEL_FLASH,
    HAL_CLOCK_SEL_F32K,
    HAL_CLOCK_SEL_TRACE,
    HAL_CLOCK_SEL_GCPU,
    HAL_CLOCK_SEL_ECC,
    HAL_CLOCK_SEL_SPIM0,
    HAL_CLOCK_SEL_SPIM1,
    HAL_CLOCK_SEL_SPIS,
    HAL_CLOCK_SEL_I2C,
    HAL_CLOCK_SEL_DBG,
    HAL_CLOCK_SEL_USB_SYS,
    HAL_CLOCK_SEL_USB_XHCI,
    HAL_CLOCK_SEL_AUX_ADC_DIV,
    HAL_CLOCK_SEL_PWM_DIV,
};

#define NUM_CLKSEL_BACKUP_LIST_ENTRY (sizeof(clk_clksel_backup_list) / sizeof(hal_clock_sel_id))
#define NUM_CLKSEL_BACKUP_PHASE1 (4)
#define F32K_BACKUP_LIST_INDEX (3)
STATIC_ASSERT(NUM_CLKSEL_BACKUP_PHASE1 <= NUM_CLKSEL_BACKUP_LIST_ENTRY);

struct clk_ds_backup_t {
    uint32_t xtal_clk_ctrl;
    uint32_t clk_en; // bitmap starts from HAL_CLOCK_CG_PLL_BEGIN
    bool dcm_en;
    uint32_t clksel[NUM_CLKSEL_BACKUP_LIST_ENTRY];
};

// ensure xtal is at the beginning part of clock_id
STATIC_ASSERT(HAL_CLOCK_CG_XTAL_END + 1 == HAL_CLOCK_CG_PLL_BEGIN);
// ensure clk_en 32 bits can store all remain clock status
STATIC_ASSERT(HAL_CLOCK_CG_MAX - HAL_CLOCK_CG_PLL_BEGIN < 32);

// Shall be put in TCM due to deep sleep cache optimization
ATTR_RWDATA_IN_TCM static struct clk_ds_backup_t clk_ds_backup;

ATTR_TEXT_IN_SYSRAM void clk_ds_backup_and_set(void *data)
{
    clk_ds_backup.xtal_clk_ctrl = REG32_READ(XTAL_CLK_CTRL);

    clk_ds_backup.clk_en = 0;
    for (hal_clock_cg_id clock_id = HAL_CLOCK_CG_PLL_BEGIN; clock_id < HAL_CLOCK_CG_MAX; clock_id++) {
        if (hal_clock_is_enabled(clock_id)) {
            clk_ds_backup.clk_en |= BIT(clock_id - HAL_CLOCK_CG_PLL_BEGIN);
        }
    }

    for (uint32_t i = 0; i < NUM_CLKSEL_BACKUP_LIST_ENTRY; i++) {
        hal_clock_sel_id clock_id = clk_clksel_backup_list[i];
        if (clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV0 ||
            clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV3 ||
            clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV6) {
            hal_clock_get_rate(clock_id, &clk_ds_backup.clksel[i]);
        } else {
            hal_clock_get_selected_mux(clock_id, &clk_ds_backup.clksel[i]);
        }
    }

    hal_clock_mux_select(HAL_CLOCK_SEL_FLASH, CLK_FLASH_CLKSEL_XTAL);
    // 7933 E1 workaround - 20201204.JJ Chang - Boot rom did not set DIV_EN in FLASH_CLK_CTL
    HAL_REG_32(0x3002026C) |= BITS(30, 31);  // FLASH_CLK_CTL
    // 7933 E1 workaround - 20210118.JJ Chang - Boot rom did not set DIV_EN in GCPU_CLK_CTL & ECC_CLK_CTL
    HAL_REG_32(0x30020300) |= BITS(30, 31);  // GCPU_CLK_CTL
    HAL_REG_32(0x30020304) |= BITS(30, 31);  // ECC_CLK_CTL

    hal_clock_mux_select(HAL_CLOCK_SEL_INFRA_BUS, CLK_INFRA_BUS_CLKSEL_XTAL);
    hal_clock_mux_select(HAL_CLOCK_SEL_CM33_HCLK, CLK_CM33_HCLK_CLKSEL_XTAL);

    hal_clock_disable(HAL_CLOCK_CG_TOP_PLL);
    hal_clock_disable(HAL_CLOCK_CG_USB_PLL);

}

ATTR_TEXT_IN_SYSRAM void clk_ds_restore_phase1(void *data)
{
    hal_clock_status_t ret;

    if (clk_ds_backup.clk_en & (1 << (HAL_CLOCK_CG_TOP_PLL - HAL_CLOCK_CG_PLL_BEGIN))) {
        ret = hal_clock_enable(HAL_CLOCK_CG_TOP_PLL);
        ASSERT(ret == HAL_CLOCK_STATUS_OK);
    }

    for (uint32_t i = 0; i < NUM_CLKSEL_BACKUP_PHASE1; i++) {
        ret = hal_clock_mux_select(clk_clksel_backup_list[i], clk_ds_backup.clksel[i]);
        ASSERT(ret == HAL_CLOCK_STATUS_OK);
    }
}

void clk_ds_restore_phase2(void *data)
{
    REG32_WRITE(XTAL_CLK_CTRL, clk_ds_backup.xtal_clk_ctrl);

    for (hal_clock_cg_id clock_id = HAL_CLOCK_CG_PLL_BEGIN; clock_id < HAL_CLOCK_CG_MAX; clock_id++) {
        if (clk_ds_backup.clk_en & BIT(clock_id - HAL_CLOCK_CG_PLL_BEGIN)) {
            hal_clock_enable(clock_id);
        } else {
            hal_clock_disable(clock_id);
        }
    }


    for (uint32_t i = NUM_CLKSEL_BACKUP_PHASE1; i < NUM_CLKSEL_BACKUP_LIST_ENTRY; i++) {
        hal_clock_sel_id clock_id = clk_clksel_backup_list[i];
        if (clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV0 ||
            clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV3 ||
            clock_id == HAL_CLOCK_SEL_APLL12_CK_DIV6) {
            hal_clock_status_t ret = hal_clock_set_rate(clock_id, clk_ds_backup.clksel[i]);
            ASSERT(ret == HAL_CLOCK_STATUS_OK);
        } else {
            hal_clock_status_t ret = hal_clock_mux_select(clock_id, clk_ds_backup.clksel[i]);
            ASSERT(ret == HAL_CLOCK_STATUS_OK);
        }
    }

    hal_clock_set_dcm(clk_ds_backup.dcm_en);

    // Workaround: forced freq-meter select largest clk ID
    // which can dealt with problem leakage, and also can compatiable
    // with E1.
    HAL_REG_32(FREDETCR0) = FREDETCR0_SW_DEFAULT;
}

#ifndef MTK_1ST_LINK_SRAM_BOOT
static const uint8_t CG_enable_list[] = {
    HAL_CLOCK_CG_CM33_XTAL,
    HAL_CLOCK_CG_UART0_XTAL,
    HAL_CLOCK_CG_UART1_XTAL,
    HAL_CLOCK_CG_CONNAC_XTAL,
    HAL_CLOCK_CG_EFUSE1_XTAL,
    HAL_CLOCK_CG_EFUSE2_XTAL,
    HAL_CLOCK_CG_EFUSE3_XTAL,
    HAL_CLOCK_CG_RTC_XTAL,
    HAL_CLOCK_CG_FLASH_XTAL,
    HAL_CLOCK_CG_TOP_AON_XTAL,
    HAL_CLOCK_CG_CM33_DIV2_XTAL,
    HAL_CLOCK_CG_TOP_PLL,
    HAL_CLOCK_CG_INFRA_BUS,
    HAL_CLOCK_CG_FLASH,
    HAL_CLOCK_CG_SPIS,
    HAL_CLOCK_CG_DBG,
    HAL_CLOCK_CG_MAX
};
#endif /* #ifndef MTK_1ST_LINK_SRAM_BOOT */

/**
 * @brief       This function initializes the clock driver and CG before using any Clock API.
 * @return      #HAL_CLOCK_STATUS_OK, if the operation completed successfully.\n
 *              #HAL_CLOCK_STATUS_UNINITIALIZED, if the clock driver is not initialized.\n
 *              #HAL_CLOCK_STATUS_INVALID_PARAMETER, if the input parameter is invalid.\n
 *              #HAL_CLOCK_STATUS_ERROR, if the clock function detected a common error.\n
 */
hal_clock_status_t hal_clock_init(void)
{
    hal_clock_status_t ret;

    if (HAL_REG_32(FREDETCR0) == FREDETCR0_SW_DEFAULT) {
        log_hal_warning("Clock is reinitialized, skip setting!");
        return HAL_CLOCK_STATUS_OK;
    }

#ifdef MTK_FPGA_ENABLE
    ret = hal_clock_mux_select(HAL_CLOCK_SEL_CM33_HCLK, CLK_CM33_HCLK_CLKSEL_XTAL);
    ASSERT(ret == HAL_CLOCK_STATUS_OK);

    ret = hal_clock_mux_select(HAL_CLOCK_SEL_INFRA_BUS, CLK_INFRA_BUS_CLKSEL_XTAL);
    ASSERT(ret == HAL_CLOCK_STATUS_OK);

    ret = hal_clock_mux_select(HAL_CLOCK_SEL_FLASH, CLK_FLASH_CLKSEL_XTAL);
    ASSERT(ret == HAL_CLOCK_STATUS_OK);

    ret = hal_clock_mux_select(HAL_CLOCK_SEL_F32K, CLK_F32K_CLKSEL_XTAL_DIV_32K);
    ASSERT(ret == HAL_CLOCK_STATUS_OK);
#else /* #ifdef MTK_FPGA_ENABLE */

    hal_clock_mux_select(HAL_CLOCK_SEL_F32K, CLK_F32K_CLKSEL_XTAL_DIV_32K);

    /* Check the clock select backup list F32K entry is the HAL_CLOCK_SEL_F32K */
    ASSERT(clk_clksel_backup_list[F32K_BACKUP_LIST_INDEX] == HAL_CLOCK_SEL_F32K);
#ifndef MTK_1ST_LINK_SRAM_BOOT
    uint8_t list_idx = 0U;
    for (hal_clock_cg_id clock_id = 0; clock_id < HAL_CLOCK_CG_MAX; ++clock_id) {
        if (CG_enable_list[list_idx] != clock_id) {
            ret = hal_clock_disable(clock_id);
        } else {
            ret = hal_clock_enable(clock_id);
            ++list_idx;
        }
    }
#else /* #ifndef MTK_1ST_LINK_SRAM_BOOT */
    for (hal_clock_cg_id clock_id = 0; clock_id < HAL_CLOCK_CG_MAX; ++clock_id) {
        ret = hal_clock_enable(clock_id);
    }
#endif /* #ifndef MTK_1ST_LINK_SRAM_BOOT */

    ret = hal_clock_mux_select(HAL_CLOCK_SEL_INFRA_BUS, CLK_INFRA_BUS_CLKSEL_133M);
    ASSERT(ret == HAL_CLOCK_STATUS_OK);

    uint32_t cm33_clk = CLK_CM33_HCLK_CLKSEL_DIV_300M;
#ifndef PINMUX_BGA_DEFAULT
    // Set CM33 clock rate to 200MHz for 7933 E2 QFN IC
    if (hal_boot_get_hw_ver() == 0x8A10) {
        cm33_clk = CLK_CM33_HCLK_CLKSEL_DIV_200M;
    }
#endif /* #ifndef PINMUX_BGA_DEFAULT */
    ret = hal_clock_mux_select(HAL_CLOCK_SEL_CM33_HCLK, cm33_clk);
    ASSERT(ret == HAL_CLOCK_STATUS_OK);

    hal_clock_mux_select(HAL_CLOCK_SEL_FLASH, CLK_FLASH_CLKSEL_DIV_60M);

    clk_dcxo_trim();
#endif /* #ifdef MTK_FPGA_ENABLE */

    // Workaround: forced freq-meter select largest clk ID
    // which can dealt with problem leakage, and also can compatiable
    // with E1.
    HAL_REG_32(FREDETCR0) = FREDETCR0_SW_DEFAULT;

    return HAL_CLOCK_STATUS_OK;
}


/**
 * @brief       This function gets the MCU clock frequency.
 * @return      the MCU clock frequency in Hz.\n
 */
ATTR_TEXT_IN_SYSRAM uint32_t hal_clock_get_mcu_clock_frequency(void)
{
    uint32_t hz;
    hal_clock_get_rate(HAL_CLOCK_SEL_CM33_HCLK, &hz);
    return hz;
}

void hal_clock_set_pll_dcm_init(void)
{
    hal_clock_enable(HAL_CLOCK_CG_TOP_PLL);
    hal_clock_set_dcm(true);
}

void hal_clock_set_dcm(bool enable)
{
    clk_ds_backup.dcm_en = enable;
    if (enable) {
        REG32_WRITE(INFRA_BUS_DCM_CTRL, 0x40401012);
    } else {
        REG32_WRITE(INFRA_BUS_DCM_CTRL, 0x40401010);
    }
}

ATTR_TEXT_IN_SYSRAM uint32_t hal_clk_get_f32k_divider(void)
{
    const struct clk_sel_field_tbl_t *sf = &clk_sel_field_tbl[HAL_CLOCK_SEL_F32K - 1];

    uint32_t reg_data = REG32_READ(sf->reg);

    uint32_t clk_sel = REG_FLD_GET(REG_FLD(sf->clksel_bits, sf->clksel_offset), reg_data);
    uint32_t div_sel = (clk_sel == sf->clksel_div ? REG_FLD_GET(REG_FLD(sf->divsel_bits, 4), reg_data) : 0);

    uint32_t ret = 0U;
    if (CLKSEL_FIELD(HAL_CLOCK_SEL_F32K, div_sel, clk_sel) == CLK_F32K_CLKSEL_RTC) {
        ret = EXTERNAL_32K_DIVIDER;
    } else {
        ret = INTERNAL_32K_DIVIDER;
    }
    return ret;
}

// for compatibility reason, these shall be removed in future
#if 1
void hal_clk_top_pll_disable(void)
{
    hal_clock_disable(HAL_CLOCK_CG_TOP_PLL);
}

void hal_clk_xpll_disable(void)
{

    hal_clock_disable(HAL_CLOCK_CG_XPLL);
}

#endif /* #if 1 */

#ifdef HAL_CLK_CTP_SUPPORT

static const char *ckgen_clk[] = {
    [FM_TOP_PLL_DIV2_CK]    = "TOP_PLL_DIV2_CK",    // [0] 600MHz
    [FM_DSPPLL_CK]          = "DSPPLL_CK",          // [1] 600MHz
    [FM_USBPLL_CK]          = "USBPLL_CK",          // [2] 192MHz
    [FM_XPLL_CK]            = "XPLL_CK",            // [3] 196.608MHz
    [FM_DSP_CK]             = "FM_DSP_CK",          // [4] 600MHz
    [FM_PSRAM_CK]           = "PSRAM_CK",           // [5] 400MHz
    [FM_SPIS_CK]            = "SPIS_CK",            // [6] 400MHz
    [FM_CM33_HCLK_PRE_CK]   = "CM33_HCLK_PRE_CK",   // [7] 300MHz
    [FM_TRACE_CK]           = "TRACE_CK",           // [8] 300MHz
    [FM_GCPU_CK]            = "GCPU_CK",            // [9] 300MHz
    [FM_ECC_CK]             = "ECC_CK",             // [10] 300MHz
    [FM_AUX_ADC_CK]         = "AUX_ADC_CK",         // [11] 2MHz
    [FM_AUDSYS_BUS_CK]      = "AUDSYS_BUS_CK",      // [12] 266.67MHz
    [FM_FAUD_INTBUS_CK]     = "FAUD_INTBUS_CK",     // [13] 266.67MHz
    [FM_PSRAM_AXI_CK]       = "PSRAM_AXI_CK",       // [14] 266.67MHz
    [FM_SPIM0_CK]           = "SPIM0_CK",           // [15] 200MHz
    [FM_SPIM1_CK]           = "SPIM1_CK",           // [16] 200MHz
    [FM_HAPLL_CK]           = "HAPLL_CK",           // [17] 196.608MHz
    [FM_FASYS_CK]           = "FASYS_CK",           // [18] 196.608MHz
    [FM_FAUD_CK]            = "FAUD_CK",            // [19] 196.608MHz
    [FM_AXI_BUS_FREE_CK]    = "AXI_BUS_FREE_CK",    // [20] 133.34MHz
    [FM_USB_XHCI_CK]        = "USB_XHCI_CK",        // [21] 133.34MHz
    [FM_DBG_CK]             = "DBG_CK",             // [22] 133.34MHz
    [FM_I2C_CK]             = "I2C_CK",             // [23] 120MHz
    [FM_FASM_CK]            = "FASM_CK",            // [24] 120MHz
    [FM_USB_SYS_CK]         = "USB_SYS_CK",         // [25] 120MHz
    [FM_FLASH_CK]           = "FLASH_CK",           // [26] 60MHz
    [FM_SDIOM_CK]           = "SDIOM_CK",           // [27] 50MHz
    [FM_FAUDIO_CK]          = "FAUDIO_CK",          // [28] 49.152MHz
    [FM_SSUSB_FRMCNT_CK]    = "SSUSB_FRMCNT_CK",    // [29] 60MHz
    [FM_XTAL_CK]            = "XTAL_CK",            // [30] 26MHz
    [FM_AUD_DAC_26M_MON_CK] = "AUD_DAC_26M_MON_CK", // [31] 26MHz
    [FM_UHS_PSRAM_CK]       = "UHS_PSRAM_CK",       // [32] 259.983MHz
    [FM_NON_UHS_PSRAM_CK]   = "NON_UHS_PSRAM_CK",   // [33] 400MHz
    [FM_NON_UHS_PSRAM_DIV2_CK] = "NON_UHS_PSRAM_DIV2_CK", // [34] 200MHz
};

#define DEFAULT_FREQ_METER_CYCLE_COUNT   1
/** @brief Enable pll tuner
*   @param en : enable(1), disable(0)
*   @return None
**/
void mt_pll_tuner_enable(unsigned int en)
{
    if (en)
        HAL_REG_32(PLL_TUNER_0) |= BIT(0);
    else
        HAL_REG_32(PLL_TUNER_0) &= ~(BIT(0));
}

/** @brief Configure pll tuner by SW
*          In general case, HW default value is no need to change
*          Provided here only for special purpose
*   @param pcw : upper and lower bound for pll tuner
*   @return None
**/
void mt_pll_tuner_config(unsigned int pcw_hi, unsigned int pcw_lo)
{
    pcw_hi &= BITS(0, 15);
    pcw_lo &= BITS(0, 15);
    HAL_REG_32(PLL_TUNER_1) = (pcw_hi << 16) | (pcw_lo);
}

static uint32_t __f32k_orignal_setting = 0;

static void __before_fmeter(void)
{
    hal_clock_get_selected_mux(HAL_CLOCK_SEL_F32K, &__f32k_orignal_setting);
    // switch f32K clock source to internal
    hal_clock_mux_select(HAL_CLOCK_SEL_F32K, CLK_F32K_CLKSEL_XTAL_DIV_32K);
}

static void __after_fmeter(void)
{
    // switch f32k clock source back to original
    hal_clock_mux_select(HAL_CLOCK_SEL_F32K, __f32k_orignal_setting);
}

/** @brief Measure clock source frequence by
*          32.745KHz internal f32K clock
*   @param ID        : refer to enum FM_CKGEN_CKID
*   @param cycle_cnt : 1~6, F32K cycle count
*   @return clock frequency measured (unit in KHz)
**/
unsigned int mt_get_ckgen_freq(unsigned int ID, unsigned int cycle_cnt)
{
    int output = 0, i = 0;

    __before_fmeter();

    // 0. Disable freq meter at first
    HAL_REG_32(FREDETCR0) &= ~(BIT(19));
    hal_gpt_delay_ms(1);

    // 1. configure clk ID
    HAL_REG_32(FREDETCR0) &= 0xFC0FFFFF;
    HAL_REG_32(FREDETCR0) |= (ID << 20);
    hal_gpt_delay_ms(1);

    // 2. configure 32K cycle count = 1~6
    HAL_REG_32(FREDETCR0) &= ~(BITS(16, 18));
    HAL_REG_32(FREDETCR0) |= (cycle_cnt << 16);
    hal_gpt_delay_ms(1);

    // 3. Enable freq meter
    HAL_REG_32(FREDETCR0) |= BIT(19);
    hal_gpt_delay_ms(1);

    // 4. polling until completed

    while ((HAL_REG_32(FREDETCR0) & BIT(15)) == 0) {
        hal_gpt_delay_ms(1);
        i++;
        if (i > 10) {
            // Workaround: forced freq-meter select largest clk ID
            // which can dealt with problem leakage, and also can compatiable
            // with E1.
            HAL_REG_32(FREDETCR0) = FREDETCR0_SW_DEFAULT;
            PR("%d: %s: timeout! (no this device?)\r\n", ID, ckgen_clk[ID]);
            __after_fmeter();
            return 0;
        }
    }

    // 5. read result
    output = ((HAL_REG_32(FREDETCR1) & BITS(0, 19)) / cycle_cnt) * 32745 / 1000; // count by 32.745KHz, unit KHz

    // Ditto
    HAL_REG_32(FREDETCR0) = FREDETCR0_SW_DEFAULT;

    __after_fmeter();
    return output;
}

unsigned int mt_measure_stable_fmeter_freq(int clk, unsigned int cycle_cnt)
{
    u32 last_freq = 0;
    u32 freq = mt_get_ckgen_freq(clk, cycle_cnt);
    u32 maxfreq = max(freq, last_freq);

    while (maxfreq > 0 && ABS_DIFF(freq, last_freq) * 100 / maxfreq > 10) {
        last_freq = freq;
        freq = mt_get_ckgen_freq(clk, cycle_cnt);
        maxfreq = max(freq, last_freq);
    }

    return freq;
}

void mt_measure_all_clocks(unsigned int cycle_cnt)
{
    unsigned int temp, results;

    PR("ckegen:\r\n");
    for (temp = 0; temp < FM_CKGEN_CLK_END; temp++) {
        if (!ckgen_clk[temp])
            continue;
        results = mt_measure_stable_fmeter_freq(temp, cycle_cnt);
        if (results)
            PR("%d: %s: %d KHz\r\n", temp, ckgen_clk[temp], results);
    }
}

unsigned int mt_get_cpu_freq(void)
{
    return mt_get_ckgen_freq(FM_CM33_HCLK_PRE_CK, DEFAULT_FREQ_METER_CYCLE_COUNT);
}

unsigned int mt_get_bus_freq(void)
{
    return mt_get_ckgen_freq(FM_AXI_BUS_FREE_CK, DEFAULT_FREQ_METER_CYCLE_COUNT);
}

extern void power_on_sysram_dsp_psram_conn(void);

int mt_clkmgr_init(void)
{
    hal_clock_mux_select(HAL_CLOCK_SEL_F32K, CLK_F32K_CLKSEL_XTAL_DIV_32K);

#if ALL_CLK_ON
    hal_clock_init();

    /*************
     * for MTCMOS
     *************/
    power_on_sysram_dsp_psram_conn();

    /*************
     * for Subsys CG
     *************/

#endif /* #if ALL_CLK_ON */

#if DEBUG_FQMTR
    mt_measure_all_clocks(DEFAULT_FREQ_METER_CYCLE_COUNT);
#endif /* #if DEBUG_FQMTR */

    return 0;
}
#endif /* #ifdef HAL_CLK_CTP_SUPPORT */
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */


