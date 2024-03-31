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
#include "driver_api.h"
#include "hal_log.h"
#include "hal_gpt.h"
#include "hal_wdt_internal.h"

static uint32_t wdt_last_write_time = 0;
#define WDT_HW_WRITE_MIN_INTERVAL_US (185)

void WDTCRWrite(uint32_t u4Value, volatile uint32_t *pu4CRAddr)
{
    uint32_t _now, _dur;

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &_now);
    hal_gpt_get_duration_count(wdt_last_write_time, _now, &_dur);

    if (_dur < WDT_HW_WRITE_MIN_INTERVAL_US) {
        hal_gpt_delay_us(WDT_HW_WRITE_MIN_INTERVAL_US - _dur);
    }
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &wdt_last_write_time);

    writel(u4Value, pu4CRAddr);
}

uint32_t WDTCRRead(volatile uint32_t *pu4CRAddr)
{
    uint32_t u4Value = 0;

    uint32_t _now, _dur;

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &_now);
    hal_gpt_get_duration_count(wdt_last_write_time, _now, &_dur);

    if (_dur < WDT_HW_WRITE_MIN_INTERVAL_US) {
        hal_gpt_delay_us(WDT_HW_WRITE_MIN_INTERVAL_US - _dur);
    }

    u4Value = readl(pu4CRAddr);
    return (uint32_t)(u4Value);
}

void wdt_set_counter(uint32_t value)
{

    uint32_t val = 0;

    val &= (~WDT_LENGTH_KEY_MASK);
    val |= (WDT_LENGTH_KEY << WDT_LENGTH_KEY_OFFSET);

    val &= (~WDT_LENGTH_TIMEOUT_MASK);
    val |= (value << WDT_LENGTH_TIMEOUT_OFFSET);
    P_WDT_TypeDef pWDTTypeDef = (P_WDT_TypeDef)(WDT_REG_BASE + WDT_MODE_OFFSET);
    WDTCRWrite(val, &(pWDTTypeDef->WDT_LENGTH));
}

void wdt_set_interval(uint32_t value)
{

    uint32_t val = 0;
    val &= (~WDT_INTERVAL_LENGTH_MASK);
    val |= (value << WDT_INTERVAL_LENGTH_OFFSET);
    P_WDT_TypeDef pWDTTypeDef = (P_WDT_TypeDef)(WDT_REG_BASE + WDT_MODE_OFFSET);
    WDTCRWrite(val, &(pWDTTypeDef->WDT_INTERVAL));
}

/* get wdt hw and sw timeout status */
uint32_t wdt_get_status(void)
{
    uint32_t u4Value = 0;

    uint32_t *pWDTSta = (uint32_t *)(WDT_SW_STA_BASE + WDT_SW_STA_OFFSET);
    u4Value = WDTCRRead(pWDTSta);
    return (uint32_t)(u4Value);
}

/* wdt irq or reset mode select */
void wdt_set_mode(uint32_t irq)
{
    uint32_t val = 0;
    P_WDT_TypeDef pWDTTypeDef = (P_WDT_TypeDef)(WDT_REG_BASE + WDT_MODE_OFFSET);
    val = WDTCRRead(&(pWDTTypeDef->WDT_MODE));
    val &= (~WDT_MODE_KEY_MASK);
    val |= (WDT_MODE_KEY << WDT_MODE_KEY_OFFSET);
    if (0 == irq) {
        val &= (~BIT(WDT_MODE_IRQ_OFFSET));
    } else {
        val |= (BIT(WDT_MODE_IRQ_OFFSET));
    }
    WDTCRWrite(val, &(pWDTTypeDef->WDT_MODE));
}

/* set hw status for mt7933 only*/
void wdt_sw_set_hw_reboot(void)
{

    uint32_t u4Value = 0;

    uint32_t *pWDTSta = (uint32_t *)(WDT_SW_STA_BASE + WDT_SW_STA_OFFSET);
    u4Value = WDTCRRead(pWDTSta);
    u4Value |= (0x1 << WDT_SW_STA_WDT_OFFSET);
    WDTCRWrite(u4Value, pWDTSta);
}

/* clr sw status for mt7933 only */
void wdt_sw_clr_sw_reboot(void)
{
    uint32_t u4Value = 0;

    uint32_t *pWDTSta = (uint32_t *)(WDT_SW_STA_BASE + WDT_SW_STA_OFFSET);
    u4Value = WDTCRRead(pWDTSta);
    u4Value &= ~(0x1 << WDT_SW_STA_SW_WDT_OFFSET);
    WDTCRWrite(u4Value, pWDTSta);
}

/* wdt hw count enable */
void wdt_set_enable(uint32_t ucEnable)
{
    P_WDT_TypeDef pWDTTypeDef = NULL;
    uint32_t u4Val = 0;

    pWDTTypeDef = (P_WDT_TypeDef)(WDT_REG_BASE + WDT_MODE_OFFSET);

    u4Val = WDTCRRead(&(pWDTTypeDef->WDT_MODE));

    u4Val &= (~WDT_MODE_KEY_MASK);
    u4Val |= (WDT_MODE_KEY << WDT_MODE_KEY_OFFSET);

    if (0 == ucEnable) {
        u4Val &= (~BIT(WDT_MODE_ENABLE_OFFSET));
    } else {
        u4Val |= (BIT(WDT_MODE_ENABLE_OFFSET));
    }

    WDTCRWrite(u4Val, &(pWDTTypeDef->WDT_MODE));
}

/* wdt hw count restart */
void wdt_restart(void)
{
    P_WDT_TypeDef pWDTTypeDef = (P_WDT_TypeDef)(WDT_REG_BASE + WDT_MODE_OFFSET);
    WDTCRWrite(WDT_RESTART_KEY, &(pWDTTypeDef->WDT_RESTART));
}

/* wdt sw timeout trigger */
void wdt_swrst(void)
{

    uint32_t u4Value = 0;

    uint32_t *pWDTSta = (uint32_t *)(WDT_SW_STA_BASE + WDT_SW_STA_OFFSET);
    u4Value = WDTCRRead(pWDTSta);
    u4Value |= (0x1 << WDT_SW_STA_SW_WDT_OFFSET);
    WDTCRWrite(u4Value, pWDTSta);

    P_WDT_TypeDef pWDTTypeDef = (P_WDT_TypeDef)(WDT_REG_BASE + WDT_MODE_OFFSET);

    wdt_restart();
    WDTCRWrite(WDT_SWRST_KEY, &(pWDTTypeDef->WDT_SWRST));

}
void wdt_dump_info(void)
{
#if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG))
    P_WDT_TypeDef pWDTTypeDef = (P_WDT_TypeDef)(WDT_REG_BASE + WDT_MODE_OFFSET);

    /* Dump WDT regisers */
    log_hal_info("MODE: 0x%lx\r\n", WDTCRRead(&(pWDTTypeDef->WDT_MODE)));
    log_hal_info("STA:  0x%lx\r\n", wdt_get_status());
    log_hal_info("LEN:  0x%lx\r\n", WDTCRRead(&(pWDTTypeDef->WDT_LENGTH)));
#endif /* #if !defined(MTK_RELEASE_MODE) || (defined(MTK_RELEASE_MODE) &&(MTK_RELEASE_MODE == MTK_M_DEBUG)) */
}
