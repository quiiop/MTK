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
#ifdef HAL_EINT_MODULE_ENABLED

#include "mhal_eint.h"
#include "hdl_eint.h"

#define DEBOUNCE_MAX_TIME       128
#define IRQ_SENS_BASE  (IRQ_CFG_BASE + 0x200)

static void _mtk_mhal_eint_calc_deb_param(u32 debounce_time, u32 *prescaler, u32 *cnt)
{
    u32 const threshold = 10;
    u32 target_count = debounce_time * 8;
    u32 tmp = 0;

    for (tmp = 0; tmp < 7; tmp++) {
        if (target_count <= threshold)
            break;
        target_count = target_count / 2;
    }
    *prescaler = tmp;
    *cnt = target_count;
}

int mtk_mhal_eint_enable_debounce(hal_eint_number_t eint_num, void __iomem *eint_base)
{
    if (!eint_base) {
        log_hal_error("eint_base is NULL.\n");
        return -EINT_EFAULT;
    }

    if (eint_num >= HAL_EINT_NUMBER_MAX) {
        log_hal_error("eint_num is invaild.\n");
        return -EINT_EINVAL;
    }

    mtk_hdl_eint_enable_debounce((u32)eint_num, eint_base);

    return 0;
}

int mtk_mhal_eint_disable_debounce(hal_eint_number_t eint_num, void __iomem *eint_base)
{
    if (!eint_base) {
        log_hal_error("eint_base is NULL\n");
        return -EINT_EFAULT;
    }

    if (eint_num >= HAL_EINT_NUMBER_MAX) {
        log_hal_error("eint_num is invaild.\n");
        return -EINT_EINVAL;
    }

    mtk_hdl_eint_disable_debounce((u32)eint_num, eint_base);

    return 0;
}

int mtk_mhal_eint_set_polarity(hal_eint_number_t eint_num, u32 pol, void __iomem *eint_base)
{
    if (!eint_base) {
        log_hal_error("eint_base is NULL\n");
        return -EINT_EFAULT;
    }

    if (eint_num >= HAL_EINT_NUMBER_MAX) {
        log_hal_error("eint_num is invaild.\n");
        return -EINT_EINVAL;
    }

    if (pol > 1) {
        log_hal_error("pol is invaild.\n");
        return -EINT_EINVAL;
    }

    mtk_hdl_eint_set_polarity((u32)eint_num, pol, eint_base);

    return 0;

}

int mtk_mhal_eint_set_dual(hal_eint_number_t eint_num, u32 dual,
                           void __iomem *eint_base)
{
    if (!eint_base) {
        log_hal_error("eint_base is NULL\n");
        return -EINT_EFAULT;
    }

    if (eint_num >= HAL_EINT_NUMBER_MAX) {
        log_hal_error("eint_num is invaild.\n");
        return -EINT_EINVAL;
    }

    if (dual > 1) {
        log_hal_error("dual is invaild.\n");
        return -EINT_EINVAL;
    }

    mtk_hdl_eint_set_dual((u32)eint_num, dual, eint_base);

    return 0;
}

int mtk_mhal_eint_is_edge(hal_eint_number_t eint_num, u32 irq, void __iomem *eint_base)
{
    if (!eint_base) {
        log_hal_error("eint_base is NULL\n");
        return -EINT_EFAULT;
    }

    if (eint_num >= HAL_EINT_NUMBER_MAX) {
        log_hal_error("eint_num is invaild.\n");
        return -EINT_EINVAL;
    }

    return mtk_hdl_eint_is_edge((u32)eint_num, irq, eint_base, (void __iomem *)IRQ_SENS_BASE);
}

int mtk_mhal_eint_set_debounce(hal_eint_number_t eint_num, u32 irq, u32 debounce_time, void __iomem *eint_base)
{
    u32 prescaler, cnt;

    if (!eint_base) {
        log_hal_error("eint_base is NULL\n");
        return -EINT_EFAULT;
    }

    if (eint_num >= HAL_EINT_NUMBER_MAX) {
        log_hal_error("eint_num is invaild.\n");
        return -EINT_EINVAL;
    }

    if (mtk_mhal_eint_is_edge(eint_num, irq, eint_base)) {
        log_hal_error("eint_num %d is edge type, cannot enable debounce!!!\n", eint_num);
        return -EINT_EINVAL;
    }

    if (debounce_time > DEBOUNCE_MAX_TIME)
        debounce_time = DEBOUNCE_MAX_TIME;

    _mtk_mhal_eint_calc_deb_param(debounce_time, &prescaler, &cnt);
    mtk_hdl_eint_set_prescaler((u32)eint_num, prescaler, eint_base);
    mtk_hdl_eint_set_cnt((u32)eint_num, cnt, eint_base);

    return 0;
}

#endif /* #ifdef HAL_EINT_MODULE_ENABLED */
