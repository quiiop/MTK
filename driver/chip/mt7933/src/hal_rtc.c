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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hal_gpt.h"
#include "hal_nvic.h"
#include "hal_nvic_internal.h"
#include "hal_rtc_internal.h"
#include "hal_efuse_get.h"
#include "hal_log.h"
#include "hal_clock.h"

#ifdef HAL_RTC_MODULE_ENABLED

/* #define RTC_DEBUG
 */

typedef struct {
    hal_rtc_time_callback_t func;
    void *arg;
} rtc_time_callback_t;

typedef struct {
    hal_rtc_alarm_callback_t func;
    void *arg;
} rtc_alarm_callback_t;


static rtc_time_callback_t   g_hal_rtc_time_callback;
static rtc_alarm_callback_t   g_hal_rtc_alarm_callback;
static uint32_t g_rtc_gpt_handle;
static uint8_t g_rtc_time_sec = 0;

#define F32K_TEST_DUR (2000)
#define RTC_INIT_POLLING  (100)

#ifdef RTC_DEBUG
void rtc_dump_rg(void)
{
    UINT32 rtc_base = RTC_BASE_ADDR;
    int index = 0, reg_cnt = RTC_REGISTER_CNT;
    unsigned int StartReg = rtc_base;
    unsigned int Reg_values[RTC_REGISTER_CNT] = {0};

    for (index = 0; index < reg_cnt; index++) {
        Reg_values[index] = DRV_Reg32(StartReg + index * 4);
    }
    for (index = 0; index < reg_cnt; index++) {
        if (index % 4 == 0)
            dbg_print("\n0X%x0    ", index / 4);
        dbg_print("0x%x    ", Reg_values[index]);
    }
    dbg_print("\n");
}
#endif /* #ifdef RTC_DEBUG */

void rtc_set_debounce_time(UINT32 debnce)
{
    UINT32 rtc_debnce;

    rtc_debnce = DRV_Reg32(RTC_DEBNCE);
    DRV_WriteReg32(RTC_DEBNCE, rtc_debnce & (~RTC_DEBOUNCE_MASK));
    rtc_debnce = DRV_Reg32(RTC_DEBNCE);
    DRV_WriteReg32(RTC_DEBNCE, rtc_debnce | debnce);
}

void rtc_set_enable(bool ucEnable)
{
    UINT32 rtc_ctl;
    rtc_ctl = DRV_Reg32(RTC_CTL);

    if (!ucEnable)
        DRV_WriteReg32(RTC_CTL, rtc_ctl | RC_STOP);
    else {
        rtc_set_debounce_time(DEBNCE_INITIAL);
        DRV_WriteReg32(RTC_CTL, rtc_ctl & (~RC_STOP));
    }
}

void rtc_interrupt_handler(hal_nvic_irq_t irq_number)
{
    UINT32 irq_sta, mask;

    hal_rtc_alarm_callback_t alm_callback;
    hal_rtc_time_callback_t time_callback;
    void *alm_arg;
    void *time_arg;

    irq_sta = DRV_Reg32(RTC_PMU_EN);
#ifdef RTC_DEBUG
    sys_print("RG rtc_pmu_en:0x%x\n", irq_sta);
#endif /* #ifdef RTC_DEBUG */
    /*ensure this section not to be interrupt*/
    mask = save_and_set_interrupt_mask();

    if (irq_sta & RG_ALARM_STA) {
#ifdef RTC_DEBUG
        sys_print("%s: RG_ALARM_STA\n", __func__);
#endif /* #ifdef RTC_DEBUG */

        /* STA WC */
        DRV_WriteReg32(RTC_PMU_EN, DRV_Reg32(RTC_PMU_EN) | RG_ALARM_STA);
        alm_callback = g_hal_rtc_alarm_callback.func;
        alm_arg = g_hal_rtc_alarm_callback.arg;

        if (alm_callback == NULL)
            goto out;

        alm_callback(alm_arg);
    }

    if (irq_sta & RG_TIME_STA) {
#ifdef RTC_DEBUG
        sys_print("%s: RG_TIME_STA\n", __func__);
#endif /* #ifdef RTC_DEBUG */
        /* STA WC */
        DRV_WriteReg32(RTC_PMU_EN, DRV_Reg32(RTC_PMU_EN) | RG_TIME_STA);
        time_callback = g_hal_rtc_time_callback.func;
        time_arg = g_hal_rtc_time_callback.arg;

        if (time_callback == NULL)
            goto out;

        time_callback(time_arg);
    }

    if (irq_sta & RG_EXT_EVENT_STA) {
#ifdef RTC_DEBUG
        sys_print("%s: RG_EXT_EVENT_STA\n", __func__);
#endif /* #ifdef RTC_DEBUG */
        /* STA WC */
        DRV_WriteReg32(RTC_PMU_EN, DRV_Reg32(RTC_PMU_EN) | RG_EXT_EVENT_STA);
    }

    irq_sta = DRV_Reg32(RTC_PMU_EN);
#ifdef RTC_DEBUG
    sys_print("RG rtc_pmu_en:0x%x\n", irq_sta);
#endif /* #ifdef RTC_DEBUG */

out:
    restore_interrupt_mask(mask);
}

void rtc_nvic_register(void)
{
    unsigned int irq = RTC_IRQ_ID;
    hal_nvic_disable_irq(irq);
    hal_nvic_register_isr_handler(irq, rtc_interrupt_handler);
    hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    hal_nvic_enable_irq(irq);
}

/*
* @param[in] counter is the count down number of rtc timer.
*  When TIMER_CNT equals to 0, asset internal interrupt.
*  unit of counter is second.
*/

void rtc_set_timer(UINT32 counter)
{
    /* rtc timer count down every 1/32 sec */
    DRV_WriteReg32(RTC_TIMER_CNT, 32 * counter);
    DRV_WriteReg32(RTC_TIMER_CTL, TR_INTEN);
}

/* en: true/false - external event enable / disable
     pol: 0 active low; 1 active high */
void rtc_ext_event_ctl(bool en, UINT32 pol)
{
    UINT32 ext_ctl;

    ext_ctl = DRV_Reg32(RTC_EXT_EV_CTL);
    if (en)
        DRV_WriteReg32(RTC_EXT_EV_CTL, ext_ctl | EXT_EV_EN);
    else
        DRV_WriteReg32(RTC_EXT_EV_CTL, ext_ctl & (~EXT_EV_EN));

    ext_ctl = DRV_Reg32(RTC_EXT_EV_CTL);
    if (pol == 1)
        DRV_WriteReg32(RTC_EXT_EV_CTL, ext_ctl | EXT_EV_POL);
    else if ((pol == 0))
        DRV_WriteReg32(RTC_EXT_EV_CTL, ext_ctl & (~EXT_EV_POL));
    else
        dbg_print("wrong external polarity setting\n");
}

void rtc_set_data(U32 off, U32 size, const char *buf, U32 buf_off, enum mem_type type)
{
    UINT32 val, front, data, tmp = 0, mask, shift = 0;

    front = (off / 4) * 4;
    val = DRV_Reg32(RTC_MEMORY_ADDR + front);

    if (type == RTC_MEM_NO_SPAN)
        shift = (off - front) * 8;

    switch (size) {
        case 1:
            mask = 0xFF;
            if (type == RTC_MEM_SPAN)
                shift = 24;

            data = (UINT32)buf[buf_off];

            tmp = (val & ~(mask << shift)) | (data << shift);
            break;
        case 2:
            mask = 0xFFFF;
            if (type == RTC_MEM_SPAN)
                shift = 16;
            data = (UINT32)buf[buf_off] | (UINT32)(buf[buf_off + 1] << 8);

            tmp = (val & ~(mask << shift)) | (data << shift);
            break;
        case 3:
            mask = 0xFFFFFF;
            if (type == RTC_MEM_SPAN)
                shift = 8;
            data = (UINT32)buf[buf_off] | ((UINT32)buf[buf_off + 1] << 8) | \
                   ((UINT32)buf[buf_off + 2] << 16);

            tmp = (val & ~(mask << shift)) | (data << shift);
            break;
        case 4:
            data = (UINT32)buf[buf_off] | ((UINT32)buf[buf_off + 1] << 8) | \
                   ((UINT32)buf[buf_off + 2] << 16) | ((UINT32)buf[buf_off + 3] << 24);
            tmp = data;
            break;
        default:
            dbg_print("unexpected size for mem set\n");
            return;
            break;
    }
    DRV_WriteReg32(RTC_MEMORY_ADDR + front, tmp);
}

void rtc_get_data(UINT32 off, UINT32 size, char *buf, U32 buf_off, enum mem_type type)
{
    char reg[4];
    UINT32 len = size, val, i, front, avail;

    front = (off / 4) * 4;
    avail = front + 4 - off;
    val = DRV_Reg32(RTC_MEMORY_ADDR + front);

    reg[0] = (char)val & 0xFF;
    reg[1] = (char)(val >> 8) & 0xFF;
    reg[2] = (char)(val >> 16) & 0xFF;
    reg[3] = (char)(val >> 24) & 0xFF;

    if (type == RTC_MEM_SPAN_LAST) {
        for (i = 0; len > 0; len--)
            buf[buf_off++] = reg[i++];
    } else if (type == RTC_MEM_SPAN || type == RTC_MEM_NO_SPAN) {
        for (i = 4 - avail; len > 0; len--)
            buf[buf_off++] = reg[i++];
    } else {
        dbg_print("type mismatch!\n");
    }
}

void rtc_clear_data(U32 off, U32 size, enum mem_type type)
{
    UINT32 val, front, tmp = 0, mask, shift = 0;

    front = (off / 4) * 4;
    val = DRV_Reg32(RTC_MEMORY_ADDR + front);

    if (type == RTC_MEM_NO_SPAN)
        shift = (off - front) * 8;

    switch (size) {
        case 1:
            mask = 0xFF;
            if (type == RTC_MEM_SPAN)
                shift = 24;

            tmp = (val & ~(mask << shift));
            break;
        case 2:
            mask = 0xFFFF;
            if (type == RTC_MEM_SPAN)
                shift = 16;

            tmp = (val & ~(mask << shift));
            break;
        case 3:
            mask = 0xFFFFFF;
            if (type == RTC_MEM_SPAN)
                shift = 8;

            tmp = (val & ~(mask << shift));
            break;
        case 4:
            tmp = 0;
            break;
        default:
            dbg_print("unexpected size for mem set\n");
            return;
            break;
    }
    DRV_WriteReg32(RTC_MEMORY_ADDR + front, tmp);
}

static void rtc_32k_2s_callback(void *data)
{
    hal_rtc_status_t rtc_ret;
    hal_clock_status_t clk_ret;
    hal_gpt_status_t gpt_ret;

    hal_rtc_time_t rtc_time;
    rtc_ret = hal_rtc_get_time(&rtc_time);
    ASSERT(rtc_ret == HAL_RTC_STATUS_OK);

#ifdef RTC_DEBUG
    log_hal_info("2s callback RTC time sec: %d, %d\n", g_rtc_time_sec, rtc_time.rtc_sec);
#endif /* #ifdef RTC_DEBUG */

    if (rtc_time.rtc_sec == g_rtc_time_sec) {
        log_hal_warning("32k XOSC not exist, switch to internal xtal");
        rtc_ret = hal_rtc_osc32_sel(RTC_OSC_SRC_EOSC);

        ASSERT(rtc_ret == HAL_RTC_STATUS_OK);
    } else {
        clk_ret = hal_clock_mux_select(HAL_CLOCK_SEL_F32K, CLK_F32K_CLKSEL_RTC);
        ASSERT(clk_ret == HAL_CLOCK_STATUS_OK);
    }

    gpt_ret = hal_gpt_sw_free_timer(g_rtc_gpt_handle);
    ASSERT(gpt_ret == HAL_GPT_STATUS_OK);
}

static void rtc_32k_clock_src_init(void)
{
    hal_rtc_status_t rtc_ret;
    hal_gpt_status_t gpt_ret;
    hal_clock_status_t clk_ret;
    hal_rtc_time_t rtc_time;
    enum rtc_osc_src_t rtc_clk_src = RTC_OSC_SRC_EOSC;

    hal_efuse_f32k_mode_t rtc_efuse_mode = hal_efuse_get_f32k_mode();

    switch (rtc_efuse_mode) {
        case HAL_EFUSE_F32K_MODE_INTERNAL:
            rtc_ret = hal_rtc_osc32_sel(rtc_clk_src);
            ASSERT(rtc_ret == HAL_RTC_STATUS_OK);
            break;

        case HAL_EFUSE_F32K_MODE_RTC_EOSC:
            rtc_ret = hal_rtc_osc32_sel(rtc_clk_src);
            ASSERT(rtc_ret == HAL_RTC_STATUS_OK);

            clk_ret = hal_clock_mux_select(HAL_CLOCK_SEL_F32K, CLK_F32K_CLKSEL_RTC);
            ASSERT(clk_ret == HAL_CLOCK_STATUS_OK);
            break;

        default:
            rtc_clk_src = RTC_OSC_SRC_XOSC;

            rtc_ret = hal_rtc_osc32_sel(rtc_clk_src);
            ASSERT(rtc_ret == HAL_RTC_STATUS_OK);

            gpt_ret = hal_gpt_sw_get_timer(&g_rtc_gpt_handle);
            ASSERT(gpt_ret == HAL_GPT_STATUS_OK);

            rtc_ret = hal_rtc_get_time(&rtc_time);
            ASSERT(rtc_ret == HAL_RTC_STATUS_OK);
            g_rtc_time_sec = rtc_time.rtc_sec;
#ifdef RTC_DEBUG
            log_hal_info("RTC time sec: %d, %d\n", g_rtc_time_sec, rtc_time.rtc_sec);
#endif /* #ifdef RTC_DEBUG */
            gpt_ret = hal_gpt_sw_start_timer_ms(g_rtc_gpt_handle, F32K_TEST_DUR, rtc_32k_2s_callback, NULL);
            ASSERT(gpt_ret == HAL_GPT_STATUS_OK);
            break;
    }
}

hal_rtc_status_t hal_rtc_init(void)
{
    UINT32 i;
#ifdef RTC_DEBUG
    UINT32 pwr1, pwr2, rtc_key, prot1, prot2, prot3, prot4;
#endif /* #ifdef RTC_DEBUG */

    // poll status for maximum 100ms
    for (i = 0; HAL_REG_32(RTC_COREPDN) != 0x2 && i < RTC_INIT_POLLING; i++) {
        hal_gpt_delay_us(1000);
    }

    if (i == RTC_INIT_POLLING) {
#ifdef RTC_DEBUG
        dbg_print("RTC is not ready!\n");
#endif /* #ifdef RTC_DEBUG */
        return HAL_RTC_STATUS_ERROR;
    }

    /* pwrkey & RTC key check  */
    DRV_WriteReg32(RTC_PWRCHK1, RTC_PWRCHK1_VAL);
    DRV_WriteReg32(RTC_PWRCHK2, RTC_PWRCHK2_VAL);
    DRV_WriteReg32(RTC_KEY, RTC_KEY_VAL);

    /* OSC32 initial & Prot check  */
    DRV_WriteReg32(RTC_PROT1, RTC_PROT1_VAL);
    DRV_WriteReg32(RTC_PROT2, RTC_PROT2_VAL);
    DRV_WriteReg32(RTC_PROT3, RTC_PROT3_VAL);
    DRV_WriteReg32(RTC_PROT4, RTC_PROT4_VAL);

    DRV_WriteReg32(RTC_PRDY_CNT, PRDY_CNT_DEFAULT);
    /* stop rtc*/
    //rtc_set_enable(false);
    rtc_ext_event_ctl(true, 1);
    // set IO EXT_EV pull-down
    DRV_WriteReg32(RTC_PAD_CTL, 0x00001203);
    /* irq handler */
    rtc_nvic_register();
    g_hal_rtc_time_callback.func = NULL;
    g_hal_rtc_time_callback.arg = NULL;
    g_hal_rtc_alarm_callback.func = NULL;
    g_hal_rtc_alarm_callback.arg = NULL;

#ifdef RTC_DEBUG
    pwr1 = DRV_Reg32(RTC_PWRCHK1);
    pwr2 = DRV_Reg32(RTC_PWRCHK2);
    rtc_key = DRV_Reg32(RTC_KEY);
    prot1 = DRV_Reg32(RTC_PROT1);
    prot2 = DRV_Reg32(RTC_PROT2);
    prot3 = DRV_Reg32(RTC_PROT3);
    prot4 = DRV_Reg32(RTC_PROT4);

    dbg_print("Dump rtc setting, pwr1:%x, pwr2:%x, rtc_key:%x, prot1:%x, prot2:%x, prot3:%x, prot4:%x\n",
              pwr1, pwr2, rtc_key, prot1, prot2, prot3, prot4);
#endif /* #ifdef RTC_DEBUG */

    rtc_32k_clock_src_init();

    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_deinit(void)
{
    /* disable rtc */
    rtc_set_enable(false);
    g_hal_rtc_time_callback.func = NULL;
    g_hal_rtc_time_callback.arg = NULL;
    g_hal_rtc_alarm_callback.func = NULL;
    g_hal_rtc_alarm_callback.arg = NULL;

    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_osc32_sel(enum rtc_osc_src_t osc32)
{
    UINT32 osc_cfg;
    if (osc32 >= RTC_OSC_SRC_END)
        return HAL_RTC_STATUS_INVALID_PARAM;

    switch (osc32) {
        case RTC_OSC_SRC_EOSC:
            osc_cfg = DRV_Reg32(RTC_XOSC_CFG);
            DRV_WriteReg32(RTC_XOSC_CFG, osc_cfg & (~RG_OSC_CK_SEL)); //eosc
            break;

        case RTC_OSC_SRC_XOSC:
            osc_cfg = DRV_Reg32(RTC_XOSC_CFG);
            DRV_WriteReg32(RTC_XOSC_CFG, osc_cfg | RG_OSC_CK_SEL); //xosc
            break;

        default:
            break;
    }

    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_enter_rtc_mode(void)
{
    /* Shutdown PMU to allow platform go into RTC mode*/
    DRV_WriteReg32(RTC_PMU_EN, DRV_Reg32(RTC_PMU_EN) & (~RG_PMU_EN));

    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_set_time(const hal_rtc_time_t *time)
{
    if (!time)
        return HAL_RTC_STATUS_INVALID_PARAM;

    DRV_WriteReg32(RTC_TC_YEA, time->rtc_year);
    DRV_WriteReg32(RTC_TC_MON, time->rtc_mon);
    DRV_WriteReg32(RTC_TC_DOM, time->rtc_day);
    DRV_WriteReg32(RTC_TC_DOW, time->rtc_week);
    DRV_WriteReg32(RTC_TC_HOU, time->rtc_hour);
    DRV_WriteReg32(RTC_TC_MIN, time->rtc_min);
    DRV_WriteReg32(RTC_TC_SEC, time->rtc_sec);

    /* start rtc*/
    rtc_set_enable(true);

    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_get_time(hal_rtc_time_t *time)
{
    if (!time)
        return HAL_RTC_STATUS_INVALID_PARAM;

    time->rtc_year = DRV_Reg32(RTC_TC_YEA);
    time->rtc_mon = DRV_Reg32(RTC_TC_MON);
    time->rtc_day = DRV_Reg32(RTC_TC_DOM);
    time->rtc_week = DRV_Reg32(RTC_TC_DOW);
    time->rtc_hour = DRV_Reg32(RTC_TC_HOU);
    time->rtc_min = DRV_Reg32(RTC_TC_MIN);
    time->rtc_sec = DRV_Reg32(RTC_TC_SEC);

#ifdef RTC_DEBUG
    dbg_print("rtc read time, yea:%d, mon:%d, dom:%d, dow:%d, hou:%d, min:%d, sec:%d\n",
              time->rtc_year, time->rtc_mon, time->rtc_day, time->rtc_week,
              time->rtc_hour, time->rtc_min, time->rtc_sec);
#endif /* #ifdef RTC_DEBUG */

    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_set_time_callback(hal_rtc_time_callback_t callback_function, void *user_data)
{
    if (callback_function == NULL) {
        return HAL_RTC_STATUS_INVALID_PARAM;
    }

    g_hal_rtc_time_callback.func = callback_function;
    g_hal_rtc_time_callback.arg = user_data;
    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_set_alarm(const hal_rtc_time_t *time)
{
    if (!time)
        return HAL_RTC_STATUS_INVALID_PARAM;

    DRV_WriteReg32(RTC_AL_YEA, time->rtc_year);
    DRV_WriteReg32(RTC_AL_MON, time->rtc_mon);
    DRV_WriteReg32(RTC_AL_DOM, time->rtc_day);
    DRV_WriteReg32(RTC_AL_HOU, time->rtc_hour);
    DRV_WriteReg32(RTC_AL_MIN, time->rtc_min);
    DRV_WriteReg32(RTC_AL_SEC, time->rtc_sec);

    /*set alm compare without weekday */
    DRV_WriteReg32(RTC_AL_CTL, RTC_MASK_OFF_WEEK);
    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_get_alarm(hal_rtc_time_t *time)
{
    if (!time)
        return HAL_RTC_STATUS_INVALID_PARAM;

    time->rtc_year = DRV_Reg32(RTC_AL_YEA);
    time->rtc_mon = DRV_Reg32(RTC_AL_MON);
    time->rtc_day = DRV_Reg32(RTC_AL_DOM);
    time->rtc_hour = DRV_Reg32(RTC_AL_HOU);
    time->rtc_min = DRV_Reg32(RTC_AL_MIN);
    time->rtc_sec = DRV_Reg32(RTC_AL_SEC);

#ifdef RTC_DEBUG
    dbg_print("rtc read alarm, yea:%d, mon:%d, dom:%d, hou:%d, min:%d, sec:%d\n",
              time->rtc_year, time->rtc_mon, time->rtc_day,
              time->rtc_hour, time->rtc_min, time->rtc_sec);
#endif /* #ifdef RTC_DEBUG */

    return HAL_RTC_STATUS_OK;


}

hal_rtc_status_t hal_rtc_enable_alarm(void)
{
    UINT32 alm_ctl;
    alm_ctl = DRV_Reg32(RTC_AL_CTL);

    DRV_WriteReg32(RTC_AL_CTL, alm_ctl | RTC_ALMEN);
    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_disable_alarm(void)
{
    UINT32 alm_ctl;
    alm_ctl = DRV_Reg32(RTC_AL_CTL);

    DRV_WriteReg32(RTC_AL_CTL, alm_ctl  & (~RTC_ALMEN));
    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_set_alarm_callback(const hal_rtc_alarm_callback_t callback_function, void *user_data)
{
    if (callback_function == NULL) {
        return HAL_RTC_STATUS_INVALID_PARAM;
    }

    g_hal_rtc_alarm_callback.func = callback_function;
    g_hal_rtc_alarm_callback.arg = user_data;
    return HAL_RTC_STATUS_OK;
}


/*
* @param[in] offset is the position of RTC spare registers to store data. The unit is in bytes.
* @param[in] buf is the address of buffer to store the data write to the RTC spare registers.
* @param[in] len is the datalength stored in the RTC spare registers. The unit is in bytes.
*/
hal_rtc_status_t hal_rtc_set_data(uint16_t offset, const char *buf, uint16_t len)
{
    U32 tmp, front, rear, size, remain, buf_off;

    if (!buf || (RTC_MEMORY_ADDR + offset > RTC_MEMORY_END) ||
        (RTC_MEMORY_ADDR + offset + len > RTC_MEMORY_END))
        return HAL_RTC_STATUS_INVALID_PARAM;

    buf_off = 0;
    tmp = offset;
    remain = len;

    front = (tmp / 4) * 4;
    rear = front + 3;
    size = rear - tmp + 1;

    /*less then one reg */
    if (len <= size) {
        rtc_set_data(tmp, len, buf, buf_off, RTC_MEM_NO_SPAN);
        return HAL_RTC_STATUS_OK;
    }

    /*more then one reg */
    do {
        front = (tmp / 4) * 4;
        rear = front + 3;
        size = rear - tmp + 1;
#ifdef RTC_DEBUG
        dbg_print("front: %d, rear: %d, size: %d, remain: %d\n", front, rear, size, remain);
#endif /* #ifdef RTC_DEBUG */

        if (size <= remain) {
            rtc_set_data(tmp, size, buf, buf_off, RTC_MEM_SPAN);
            tmp += size;
            remain -= size;
            buf_off += size;
        } else {    /* last package */
            rtc_set_data(tmp, remain, buf, buf_off, RTC_MEM_SPAN_LAST);
            remain = 0;
        }
    } while (remain);

    return HAL_RTC_STATUS_OK;
}

/*
* @param[in] offset is the position of RTC spare registers to store data. The unit is in bytes.
* @param[in] buf is the address of buffer to store the data received from the RTC spare registers.
* @param[in] len is the datalength read from the RTC spare registers. The unit is in bytes.
*/
hal_rtc_status_t hal_rtc_get_data(uint16_t offset, char *buf, uint16_t len)
{
    U32 tmp, front, rear, size, remain, buf_off;

    if (!buf || (RTC_MEMORY_ADDR + offset > RTC_MEMORY_END) ||
        (RTC_MEMORY_ADDR + offset + len > RTC_MEMORY_END))
        return HAL_RTC_STATUS_INVALID_PARAM;

    buf_off = 0;
    tmp = offset;
    remain = len;

    front = (tmp / 4) * 4;
    rear = front + 3;
    size = rear - tmp + 1;

    /*less then one reg */
    if (len <= size) {
        rtc_get_data(tmp, len, buf, buf_off, RTC_MEM_NO_SPAN);
        return HAL_RTC_STATUS_OK;
    }

    do {
        front = (tmp / 4) * 4;
        rear = front + 3;
        size = rear - tmp + 1;
#ifdef RTC_DEBUG
        dbg_print("front: %d, rear: %d, size: %d, remain: %d\n", front, rear, size, remain);
#endif /* #ifdef RTC_DEBUG */
        if (size <= remain) {
            rtc_get_data(tmp, size, buf, buf_off, RTC_MEM_SPAN);
            tmp += size;
            remain -= size;
            buf_off += size;
        } else {    /* last package */
            rtc_get_data(tmp, remain, buf, buf_off, RTC_MEM_SPAN_LAST);
            remain = 0;
        }
    } while (remain);

    return HAL_RTC_STATUS_OK;
}

hal_rtc_status_t hal_rtc_clear_data(uint16_t offset, uint16_t len)
{
    U32 tmp, front, rear, size, remain;

    if ((RTC_MEMORY_ADDR + offset > RTC_MEMORY_END) ||
        (RTC_MEMORY_ADDR + offset + len > RTC_MEMORY_END))
        return HAL_RTC_STATUS_INVALID_PARAM;

    tmp = offset;
    remain = len;

    front = (tmp / 4) * 4;
    rear = front + 3;
    size = rear - tmp + 1;

    /*less then one reg */
    if (len <= size) {
        rtc_clear_data(tmp, len, RTC_MEM_NO_SPAN);
        return HAL_RTC_STATUS_OK;
    }

    /*more then one reg */
    do {
        front = (tmp / 4) * 4;
        rear = front + 3;
        size = rear - tmp + 1;
#ifdef RTC_DEBUG
        dbg_print("front: %d, rear: %d, size: %d, remain: %d\n", front, rear, size, remain);
#endif /* #ifdef RTC_DEBUG */

        if (size <= remain) {
            rtc_clear_data(tmp, size, RTC_MEM_SPAN);
            tmp += size;
            remain -= size;
        } else {    /* last package */
            rtc_clear_data(tmp, remain, RTC_MEM_SPAN_LAST);
            remain = 0;
        }
    } while (remain);

    return HAL_RTC_STATUS_OK;
}

ATTR_HAL_DEPRECATED hal_rtc_status_t hal_rtc_get_f32k_frequency(uint32_t *frequency)
{
    return HAL_RTC_STATUS_OK;
}

#endif /* #ifdef HAL_RTC_MODULE_ENABLED */
