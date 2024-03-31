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

#ifdef MTK_LP_DVT_CLI_ENABLE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#include "hal_gpt.h"
#include "hal_rtc.h"
#include "sleep_manager_cli.h"

#include "hal_spm.h"
#include "hal_psram.h"
#include "hal_psram_internal.h"
#include "hal_clk.h"
#include "hal_log.h"
#include "mt7933_pos.h"
#include "mt7933.h"
#include "hal_nvic.h"
#include "hal_nvic_internal.h"
#include "toi.h"
#include "memory_attribute.h"
#ifdef MTK_MT7933_BT_ENABLE
#include "bt_driver.h"
#ifdef MTK_BT_BOOTS_ENABLE
#include "boots.h"
#endif
#endif
#ifdef MTK_MT7933_CONSYS_WIFI_ENABLE
#include "gl_wifi_cli.h"
#endif

extern void consys_set_emi_addr_range(void);
extern void connsys_sysram_hwctl(void);

static const char *_wakeup_irq_name[] = {
    "WIC_INT_IRQ       (0)",
    "UART_IRQn         (4)",
    "DSP_TO_CM33_IRQn  (9)",
    "APXGPT0_IRQn      (10)",
    "CONN_AP_BUS_REQ_RISE_IRQn           (18)",
    "CONN_APSRC_REQ_RISE_IRQn            (20)",
    "CONN_AP_BUS_REQ_HIGH_IRQn           (22)",
    "CONN_APSRC_REQ_HIGH_IRQn            (24)",
    "DSP_UART_IRQn     (32)",
    "TOP_UART0_IRQn    (33)",
    "TOP_UART1_IRQn    (34)",
    "I2C0_IRQn         (35)",
    "I2C1_IRQn         (36)",
    "SPIM0_IRQn        (39)",
    "SPIM1_IRQn        (40)",
    "SPIS_IRQn         (41)",
    "IRRX_IRQn         (43)",
    "SSUSB_XHCI_IRQn   (48)",
    "SSUSB_OTG_IRQn    (49)",
    "SSUSB_DEV_IRQn    (50)",
    "RTC_IRQn          (52)",
    "CQDMA0_IRQn       (56)",
    "DSP_WDT_IRQn      (61)",
    "DSP_TO_CPU_IRQn   (62)",
    "APDMA0_IRQn       (63)",
    "BTIF_HOST_IRQn    (75)",
    "CONN2AP_WFDMA_IRQn(77)",
    "BGF2AP_BTIF0_WAKEUP_IRQn            (79)",
    "CONN2AP_SW_IRQn   (80)",
    "SSUSB_SPM_IRQn    (92)",
    "WF2AP_SW_IRQn     (93)",
    "GPIO_IRQ0n        (108)",
    "GPIO_IRQ30n       (138)",
    "CM33_UART_RX_IRQn (139)"
};

#define WAKEUP_IRQ_NUM  (sizeof(_wakeup_irq_name) / sizeof(char *))

static uint8_t _cli_lp_dvt_spm_status(uint8_t len, char *param[])
{
    uint32_t mtcmos_onoff_state, sram_onoff_state, sram_sleep_state;

    sleep_management_read_mtcmos_status(&mtcmos_onoff_state, &sram_onoff_state, &sram_sleep_state);
    sleep_management_dump_mtcmos_status(mtcmos_onoff_state, sram_onoff_state, sram_sleep_state);

    return 0;
}


static uint8_t _cli_lp_dvt_spm_sflash_power(uint8_t len, char *param[])
{
    uint32_t on = atoi(param[0]);

    if (on) {
        SRAM_PWR_ON_SF_DMA_SPIS;
        MTCMOS_PWR_ON_SFLASH_IOMACRO;
        // TODO:  sflash_wake_up_init((U32)(p_flash_init_settings->infos));?
        // TODO:  flashif_low_power_wakeup ?
        log_hal_info("SFlash power on done\r\n");
    } else {
        /* Note: TOP_OFF_L2 not turn off, due to InfraBus(AXI) will be off as well */

        // TODO: backup sflash parameters ?
        SRAM_PWR_DOWN_SF_DMA_SPIS;
        MTCMOS_PWR_DOWN_SFLASH_IOMACRO;
        log_hal_info("SFlash power off done\r\n");
    }
    return 0;
}

static uint8_t _cli_lp_dvt_spm_top_l1_power(uint8_t len, char *param[])
{
    uint32_t on = atoi(param[0]);
    //uint32_t idx = atoi(param[1]); // UART0/1
    if (on) {
        MTCMOS_PWR_ON_TOP_OFF_L1;
        log_hal_info("TOP_L1 power on done\r\n");
    } else {
        MTCMOS_PWR_DOWN_TOP_OFF_L1;
        log_hal_info("TOP_L1 power off done\r\n");
    }
    return 0;
}
static uint8_t _cli_lp_dvt_spm_gcpu_ecc_power(uint8_t len, char *param[])
{
    uint32_t on = atoi(param[0]);
    if (on) {
        SRAM_PWR_ON_MSDC_ECC;
        log_hal_info("ecc_gcpu power on done\r\n");
    } else {
        SRAM_PWR_DOWN_MSDC_ECC;
        log_hal_info("ecc_gcpu  power off done\r\n");
    }
    return 0;
}

static uint8_t _cli_lp_dvt_spm_gcpu_sram_power(uint8_t len, char *param[])
{
    uint32_t op = atoi(param[0]);
    int i = 0;
    switch (op) {
        case 0:
            for (i = 0; i < 2; i++)
                SRAM_PWR_DOWN_GCPU(i);
            log_hal_info("GCPU SRAM[0-1] power off done\r\n");
            break;
        case 1:
            for (i = 0; i < 2; i++)
                SRAM_PWR_ON_GCPU(i);
            log_hal_info("GCPU SRAM[0-1] power on done\r\n");
            break;
        case 2:
            for (i = 0; i < 2; i++)
                SRAM_SLP_GCPU(i);
            log_hal_info("GCPU SRAM[0-1] sleep done\r\n");
            break;
        case 3:
            for (i = 0; i < 2; i++)
                SRAM_WAKE_GCPU(i);
            log_hal_info("GCPU SRAM[0-1] wake up done\r\n");
            break;
        default:
            break;
    }
    return 0;
}

static uint8_t _cli_lp_dvt_spm_psram_power(uint8_t len, char *param[])
{
    uint32_t op = atoi(param[0]);
    switch (op) {
        case 0:
            hal_psram_off();
            log_hal_info("psram power off done\r\n");
            break;
        case 1:
            hal_psram_init();
            log_hal_info("psram on/pos done\r\n");
            break;
        case 2:
            hal_psram_power_hsleep();
            log_hal_info("psram slp done\r\n");
            break;
        case 3:
            hal_psram_power_wakeup();
            log_hal_info("psram wake up done\r\n");
            break;
        default:
            break;
    }
    return 0;
}
static uint8_t _cli_lp_dvt_spm_sdio_power(uint8_t len, char *param[])
{
    uint32_t on = atoi(param[0]);
    if (on) {
        SRAM_PWR_ON_SDCTL;
        log_hal_info("SDIO power on done\r\n");
    } else {
        SRAM_PWR_DOWN_SDCTL;
        log_hal_info("SDIO power off done\r\n");
    }
    return 0;
}
static uint8_t _cli_lp_dvt_spm_usb_power(uint8_t len, char *param[])
{
    uint32_t on = atoi(param[0]);
    if (on) {
        MTCMOS_PWR_ON_USB2;
        SRAM_PWR_ON_USB2;
        log_hal_info("USB power on done\r\n");
    } else {
        SRAM_PWR_DOWN_USB2;
        MTCMOS_PWR_DOWN_USB2;
        log_hal_info("USB power off done\r\n");
    }
    return 0;
}
static uint8_t _cli_lp_dvt_spm_audio_aon(uint8_t len, char *param[])
{
    uint32_t on = atoi(param[0]);
    if (on) {
        MTCMOS_PWR_ON_AUDIO_AO;
        log_hal_info("Audio aon power on done\r\n");
    } else {
        MTCMOS_PWR_DOWN_AUDIO_AO;
        log_hal_info("Audio aon power off done\r\n");
    }
    return 0;
}
static uint8_t _cli_lp_dvt_spm_audio_afe(uint8_t len, char *param[])
{
    uint32_t on = atoi(param[0]);
    if (on) {
        MTCMOS_PWR_ON_AUDIO_AFE;
        log_hal_info("Audio AFE power on done\r\n");
    } else {
        MTCMOS_PWR_DOWN_AUDIO_AFE;
        log_hal_info("Audio AFE power off done\r\n");
    }
    return 0;
}

static uint8_t _cli_lp_dvt_spm_audio_sram(uint8_t len, char *param[])
{
    uint32_t op = atoi(param[0]);
    int i = 0;
    switch (op) {
        case 0:
            for (i = 0; i < 4; i++)
                SRAM_PWR_DOWN_AUDIO(i);
            log_hal_info("Audio SRAM[0-3] power off done\r\n");
            break;
        case 1:
            for (i = 0; i < 4; i++)
                SRAM_PWR_ON_AUDIO(i);
            log_hal_info("Audio SRAM[0-3] power on done\r\n");
            break;
        case 2:
            log_hal_info("Audio SRAM[0-3] sleep not support\r\n");
            break;
        case 3:
            log_hal_info("Audio SRAM[0-3] wake up not support\r\n");
            break;
        default:
            break;
    }
    return 0;
}

static uint8_t _cli_lp_dvt_spm_sdctl_sram(uint8_t len, char *param[])
{
    uint32_t op = atoi(param[0]);
    switch (op) {
        case 0:
            SRAM_PWR_DOWN_SDCTL;
            log_hal_info("SDCTL SRAM power off done\r\n");
            break;
        case 1:
            SRAM_PWR_ON_SDCTL;
            log_hal_info("SDCTL SRAM power on done\r\n");
            break;
        case 2:
            log_hal_info("SDCTL SRAM sleep not support\r\n");
            break;
        case 3:
            log_hal_info("SDCTL SRAM wake up not support\r\n");
            break;
        default:
            break;
    }
    return 0;
}

static uint8_t _cli_lp_dvt_spm_dsp_power(uint8_t len, char *param[])
{
    uint32_t on = atoi(param[0]);
    if (on) {
        MTCMOS_PWR_ON_DSP;
        log_hal_info("DSP power on done\r\n");
    } else {
        MTCMOS_PWR_DOWN_DSP;
        log_hal_info("DSP power off done\r\n");
    }
    return 0;
}
static uint8_t _cli_lp_dvt_spm_dsp_sram(uint8_t len, char *param[])
{
    uint32_t op = atoi(param[0]);
    int i = 0;
    switch (op) {
        case 0:
            for (i = 0; i < 4; i++) {
                SRAM_PWR_DOWN_DSP(i);
            }
            log_hal_info("DSP SRAM[0-3] power off done\r\n");
            break;
        case 1:
            for (i = 0; i < 4; i++) {
                SRAM_PWR_ON_DSP(i);
            }
            log_hal_info("DSP SRAM[0-3] power on done\r\n");
            break;
        case 2:
            log_hal_info("DSP SRAM[0-3] sleep not support\r\n");
            break;
        case 3:
            for (i = 0; i < 4; i++) {
                SRAM_WAKE_DSP_POOL(i);
            }
            log_hal_info("DSP SRAM[0-3] wake up not support\r\n");
            break;
        default:
            break;
    }
    return 0;
}
static uint8_t _cli_lp_dvt_spm_dsp_pool(uint8_t len, char *param[])
{
    uint32_t op = atoi(param[0]);
    int i = 0;
    switch (op) {
        case 0:
            for (i = 0; i < 4; i++) {
                SRAM_PWR_DOWN_DSP_POOL(i);
            }
            log_hal_info("DSP POOL[0-3] power off done\r\n");
            break;
        case 1:
            for (i = 0; i < 4; i++) {
                SRAM_PWR_ON_DSP_POOL(i);
            }
            log_hal_info("DSP POOL[0-3] power on done\r\n");
            break;
        case 2:
            for (i = 0; i < 4; i++) {
                SRAM_SLP_DSP_POOL(i);
            }
            log_hal_info("DSP POOL[0-3] sleep done\r\n");
            break;
        case 3:
            for (i = 0; i < 4; i++) {
                SRAM_WAKE_DSP_POOL(i);
            }
            log_hal_info("DSP POOL[0-3] wake up done\r\n");
            break;
        default:
            break;
    }
    return 0;
}


static uint8_t _cli_lp_dvt_spm_conn_power(uint8_t len, char *param[])
{
    uint32_t op = atoi(param[0]);
    switch (op) {
        case 0:
            hal_spm_conninfra_off();
            log_hal_info("ConnInfra power off done\r\n");
            break;
        case 1:
            hal_spm_conninfra_on(); // no WF/BT PLL on
            log_hal_info("ConnInfra on done\r\n");
            break;
        case 11:
            log_hal_info("ConnInfra WPLL on done\r\n");
            break;
        case 12:
            log_hal_info("ConnInfra BPLL on done\r\n");
            break;
        case 2:
            hal_spm_conninfra_sleep();
            log_hal_info("ConnInfra slp done\r\n");
            break;
        case 3:
            hal_spm_conninfra_wakeup();
            log_hal_info("ConnInfra wake up done\r\n");
            break;
        case 4:
            mt7933_conninfra_init();
            //hal_spm_conninfra_pos(); // not support (1)N10 ICE SWD mode, (2)addr remapping
            log_hal_info("ConnInfra POS (all PLLs on) done\r\n");
            break;
        case 111:
            // ROW 36
            consys_emi_set_remapping_reg();
            // ROW 37
            consys_set_emi_addr_range();
            log_hal_info("EMI remapping done\r\n");
            break;
        case 112:
            // ROW 39~40
            connsys_sysram_hwctl();
            log_hal_info("sysram hwctl done\r\n");
            break;
        case 113:
            // ROW 42
            hal_spm_conninfra_a_die_cfg();
            log_hal_info("A Die CFG done\r\n");
            break;
        case 114:
            // ROW 44
            hal_spm_conninfra_afe_wbg_init();
            log_hal_info("AFE WBG init done\r\n");
            break;
        case 115:
            // ROW 45 [AFE WBG CR]
            hal_spm_conninfra_afe_wbg_cal();
            log_hal_info("AFE WBG cal done\r\n");
            break;
        case 116:
            // ROW 46~47
            hal_spm_conninfra_pll_init();
            // ROW 49~78
            hal_spm_conninfra_osc_init();
            log_hal_info("pll/osc init done\r\n");
            break;
        case 117:
            // conn_infra OSC wake up speed up
            hal_spm_conninfra_speedup_wakeup();
            log_hal_info("speedup done\r\n");
            break;
        default:
            break;
    }
    return 0;
}

static uint8_t _cli_lp_dvt_spm_conn_floor(uint8_t len, char *param[])
{
    uint32_t op = atoi(param[0]);
    switch (op) {
        case 0: // OSC floor
            hal_spm_conninfra_osc_floor();
            log_hal_info("ConnInfra OSC floor done\r\n");
            break;
        case 1: // OSC2X floor
            hal_spm_conninfra_osc2x_floor();
            log_hal_info("ConnInfra OSC2X floor done\r\n");
            break;
        case 2: // wpll floor
            hal_spm_conninfra_wpll_floor();
            log_hal_info("ConnInfra wpll floor done\r\n");
            break;
        case 3: // bpll floor
            hal_spm_conninfra_bpll_floor();
            log_hal_info("ConnInfra bpll floor done\r\n");
            break;
        case 4: // deep sleep floor
            hal_spm_conninfra_ds_floor();
            log_hal_info("ConnInfra ds floor done\r\n");
            break;
        case 5: // pll_tree floor
            hal_spm_conninfra_plltree_floor();
            log_hal_info("ConnInfra pll_tree floor done\r\n");
            break;
        default:
            break;
    }
    return 0;
}

static uint8_t _cli_lp_dvt_spm_rf_spi(uint8_t len, char *param[])
{
    uint32_t id     = atoi(param[0]);
    uint32_t op     = atoi(param[1]);
    uint32_t addr   = 0;
    uint32_t value  = 0;
    uint8_t  type;
    bool infra_wakeup = false;

    infra_wakeup = hal_spm_conninfra_is_wakeup();
    if (!infra_wakeup)
        hal_spm_conninfra_wakeup();

    switch (op) {
        case 0: // read
            addr = toi(param[2], &type);
            if (type == TOI_ERR)
                log_hal_error("param addr error!\r\n");
            else
                value = topspi_read(addr, id);
            log_hal_info("topspi read 0x%x = 0x%x done\r\n", addr, value);
            break;
        case 1: // write
            addr = toi(param[2], &type);
            if (type == TOI_ERR)
                log_hal_error("param addr error!\r\n");
            value = toi(param[3], &type);
            if (type == TOI_ERR)
                log_hal_error("param value error!\r\n");
            topspi_write(addr, value, id);
            log_hal_info("topspi write 0x%x=0x%x done\r\n", addr, value);
            break;
        default:
            break;
    }
    if (!infra_wakeup)
        hal_spm_conninfra_sleep();
    return 0;
}

#ifdef MTK_MT7933_BT_ENABLE
static uint8_t _cli_lp_dvt_spm_bt_power(uint8_t len, char *param[])
{
    uint32_t option = atoi(param[0]);

    switch (option) {
        case 0:
            log_hal_info("%s: BT off\r\n", __func__, option);
            bt_driver_power_off();
            break;
        case 1:
            log_hal_info("%s: BT CPU idle only\r\n", __func__, option);
            bt_driver_bgfsys_on();
            break;
        case 2:
            log_hal_info("%s: BT MAC+RF+PLL all turn on\r\n", __func__, option);
            bt_driver_power_on();
            break;
        default:
            log_hal_info("%s: invalid option(%ld)\r\n", __func__, option);
            break;
    }

    return 0;
}
#endif

#ifdef MTK_MT7933_CONSYS_WIFI_ENABLE
static uint8_t _cli_lp_dvt_spm_wf_power(uint8_t len, char *param[])
{
    uint32_t option = atoi(param[0]);
    static uint32_t state = 0;

    log_hal_info("%s: WiFi %d -> %d\r\n", __func__, state, option);
    switch (option) {
        case 0:
            log_hal_info("%s: WiFi off\r\n", __func__, option);
            if (state == 2) {
                _wifi_off(0, NULL);
                state = 1;
            }
            if (state == 1) {
                _wsys_off(0, NULL);
                state = 0;
            }
            break;
        case 1:
            log_hal_info("%s: WiFi CPU idle only\r\n", __func__, option);
            if (state == 0) {
                _wsys_on(0, NULL);
                state = 1;
            }
            if (state == 2) {
                _wifi_off(0, NULL);
                state = 1;
            }
            break;
        case 2:
            log_hal_info("%s: WiFi MAC+RF+PLL all turn on\r\n", __func__, option);
            if (state == 0) {
                _wsys_on(0, NULL);
                state = 1;
            }
            if (state == 1) {
                _wifi_on(0, NULL);
                state = 2;
            }
            break;
        default:
            log_hal_info("%s: invalid option(%ld)\r\n", __func__, option);
            break;
    }
    return 0;
}
#endif

ATTR_TEXT_IN_TCM
static uint8_t _cli_lp_dvt_spm_top_sram(uint8_t len, char *param[])
{
    uint32_t op = atoi(param[0]);
    int i = 0;
    switch (op) {
        case 0:
            for (i = 0; i < 8; i++)
                SRAM_PWR_DOWN_TOP_SYSRAM(i);
            log_hal_info("SRAM power off done\r\n");
            break;
        case 1:
            for (i = 0; i < 8; i++)
                SRAM_PWR_ON_TOP_SYSRAM(i);
            log_hal_info("SRAM power on done\r\n");
            break;
        case 2:
            for (i = 0; i < 8; i++)
                SRAM_SLP_TOP_SYSRAM(i);
            log_hal_info("SRAM slp done\r\n");
            break;
        case 3:
            for (i = 0; i < 8; i++)
                SRAM_WAKE_TOP_SYSRAM(i);
            log_hal_info("SRAM wake up done\r\n");
            break;
    }
    return 0;
}



void __dummy_irq_handler(hal_nvic_irq_t irq)
{
    log_hal_info("IRQ no:%d\r\n", (int)irq);
}


static uint8_t _cli_lp_dvt_spm_irq(uint8_t len, char *param[])
{
    uint32_t irq  = atoi(param[0]);
    uint32_t en   = atoi(param[1]);
    uint32_t edge = atoi(param[2]);
    unsigned int i = 0;

    if (len != 3 || \
        edge > 1 || \
        irq >= IRQ_NUMBER_MAX) {
        printf("Usage: [IRQ] [Disable(0)/Enable(1)] [Level(0)Edge(1)/]\r\n");
        printf("IRQs used most frequently:\r\n");
        for (i = 0; i < WAKEUP_IRQ_NUM; i++)
            printf("%s\r\n", _wakeup_irq_name[i]);
        return 0;
    }

    hal_nvic_irq_set_type(irq, edge);
    hal_nvic_register_isr_handler(irq, __dummy_irq_handler);
    if (en) {
        hal_nvic_enable_irq(irq);
        log_hal_info("Enable %d, edge(%d)\r\n", irq, edge);
    } else {
        hal_nvic_disable_irq(irq);
        log_hal_info("Disable %d\r\n", irq);
    }
    return 0;
}


cmd_t spm_lp_dvt_cli_cmds[] = {
    { "status",       "show all mtcmos/sram on/off status",                         _cli_lp_dvt_spm_status, NULL },
    { "sflash",       "power off(0)/on(1) SFlash",                                  _cli_lp_dvt_spm_sflash_power, NULL },
    { "top_l1",       "power off(0)/on(1) top_off_L1",                              _cli_lp_dvt_spm_top_l1_power, NULL },
    { "gcpu_ecc",     "power off(0)/on(1) gcpu_ecc",                                _cli_lp_dvt_spm_gcpu_ecc_power, NULL },
    { "gcpu_sram",    "power off(0)/on(1) slp(2)/wakeup(3) gcpu SRAM",              _cli_lp_dvt_spm_gcpu_sram_power, NULL },
    { "psram",        "power off(0)/on(1) slp(2)/wakeup(3) psram",                  _cli_lp_dvt_spm_psram_power, NULL },
    { "sdio",         "power off(0)/on(1) SDIO/SD",                                 _cli_lp_dvt_spm_sdio_power, NULL },
    { "sdctl_sram",   "power off(0)/on(1) slp(2)/wakeup(3) SDCTL SRAM",             _cli_lp_dvt_spm_sdctl_sram, NULL },
    { "usb",          "power off(0)/on(1) USB",                                     _cli_lp_dvt_spm_usb_power, NULL },
    { "audio_ao",     "power off(0)/on(1) Audio AON",                               _cli_lp_dvt_spm_audio_aon, NULL },
    { "audio_afe",    "power off(0)/on(1) Audio AFE",                               _cli_lp_dvt_spm_audio_afe, NULL },
    { "audio_sram",   "power off(0)/on(1) slp(2)/wakeup(3) Audio SRAM",             _cli_lp_dvt_spm_audio_sram, NULL },
    { "dsp",          "power off(0)/on(1) DSP",                                     _cli_lp_dvt_spm_dsp_power, NULL },
    { "dsp_sram",     "power off(0)/on(1) slp(2)/wakeup(3) DSP SRAM",               _cli_lp_dvt_spm_dsp_sram, NULL },
    { "dsp_pool",     "power off(0)/on(1) slp(2)/wakeup(3) DSP POOL",               _cli_lp_dvt_spm_dsp_pool, NULL },
    { "conn",         "power off(0)/on(1) WF_pll_on(11)/BT_Pll_on(12) slp(2)/wakeup(3) all_pll_on(4) ConnInfra",  _cli_lp_dvt_spm_conn_power, NULL         },
    { "conn_floor",   "floor state for osc(0)/osc2x(1)/wpll(2)/bpll(3)/deep_sleep(4)/pll_tree(5) of ConnInfra",   _cli_lp_dvt_spm_conn_floor, NULL         },
    { "rf_spi",       "rfspi wf(1)/bt(2)/atop(5) read(0)/write(1) <addr> <value>",  _cli_lp_dvt_spm_rf_spi, NULL },
#ifdef MTK_MT7933_CONSYS_WIFI_ENABLE
    { "wf",           "power off(0)/CPU-idle(1)/RF-PLL-MAC all on(2) WiFi",         _cli_lp_dvt_spm_wf_power, NULL },
#endif
    { "top_sram",     "power off(0)/on(1) slp(2)/wakeup(3) TOP SRAM",               _cli_lp_dvt_spm_top_sram, NULL },
    { "wakeup_irq",   "disable(0)/enable(1) wake up source IRQ: APBUS(0)/APSRC(1)/DSP(2)",  _cli_lp_dvt_spm_irq, NULL },
#ifdef MTK_MT7933_BT_ENABLE
    { "bt",           "power off(0)/CPU-idle(1)/RF-PLL-MAC all on(2) BT",           _cli_lp_dvt_spm_bt_power, NULL},
#endif
    { NULL, NULL, NULL, NULL }
};
#endif /* MTK_LP_DVT_CLI_ENABLE */
