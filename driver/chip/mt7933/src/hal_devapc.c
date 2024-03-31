/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2021
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#include <common.h>
#include "hal.h"
#include <hal_devapc.h>
#include <hal_devapc_internal.h>

#ifdef HAL_NVIC_MODULE_ENABLED
#include "hal_nvic.h"
#endif /* #ifdef HAL_NVIC_MODULE_ENABLED */

/******************************
 * Static Function
 *******************************/
#ifdef HAL_NVIC_MODULE_ENABLED
static void set_device_mask(struct DEVAPC_INFO *dapc, int mask)
{
    unsigned int reg;
    unsigned int max_reg_idx, max_reg_off;
    unsigned int val;
    unsigned int i;

    max_reg_idx = (dapc->vio_device_num - 1) / 32;
    max_reg_off = (dapc->vio_device_num - 1) % 32;

    if (mask)
        val = 0xFFFFFFFFU;
    else
        val = 0U;

    for (i = 0; i < max_reg_idx; i++) {
        reg = dapc->pd_base + PD_D0_VIO_MASK_0 + (i * 0x4);
        WRITE_REG(val, reg);
    }

    reg = dapc->pd_base + PD_D0_VIO_MASK_0 + (i * 0x4);
    val = READ_REG(reg);

    if (mask)
        WRITE_REG(val | GEN_MASK(max_reg_off, 0), reg);
    else {
        WRITE_REG(val & ~GEN_MASK(max_reg_off, 0), reg);
    }
}

static int get_clr_vio_sta(struct DEVAPC_INFO *dapc, unsigned int vio_index, int clr)
{
    unsigned int vio_reg_idx, vio_reg_off;
    unsigned long vio_sta_x;
    unsigned int vio_sta_x_val;
    unsigned int vio;

    vio_reg_idx = vio_index / (2 * MOD_NO_IN_1_APC);
    vio_reg_off = vio_index % (2 * MOD_NO_IN_1_APC);

    vio_sta_x = dapc->pd_base + PD_D0_VIO_STA_0 + (vio_reg_idx * 0x4);

    /* Get vio_sta */
    vio_sta_x_val = READ_REG(vio_sta_x);
    vio = ((vio_sta_x_val >> vio_reg_off) & 0x1U);

    /* If vio_sta is 1, try to clear it */
    if (vio && clr) {
        WRITE_REG((1U << vio_reg_off), vio_sta_x);
    }

    return vio;
}

static int get_shift_bit(hal_devapc_type_t dapc_type, unsigned int vio_idx)
{
    if (dapc_type == HAL_DEVAPC_TYPE_INFRA) {
        if (vio_idx <= 3)
            return 0;
        else if (vio_idx >= 4 && vio_idx <= 17)
            return 1;
        else if (vio_idx >= 18 && vio_idx <= 22)
            return 2;
        else if (vio_idx >= 23 && vio_idx <= 28)
            return 3;
        else if (vio_idx >= 29 && vio_idx <= 43)
            return 4;
        else if (vio_idx >= 44 && vio_idx <= 47)
            return 5;
        else if (vio_idx >= 48 && vio_idx <= 54)
            return 6;
        else if (vio_idx >= 55 && vio_idx <= 80)
            return 7;
        else if (vio_idx == 81)
            return 0;
        else if (vio_idx >= 82 && vio_idx <= 95)
            return 1;
        else if (vio_idx >= 96 && vio_idx <= 100)
            return 2;
        else if (vio_idx >= 101 && vio_idx <= 107)
            return 3;
        else if (vio_idx >= 108 && vio_idx <= 123)
            return 4;
        else if (vio_idx >= 124 && vio_idx <= 130)
            return 6;
        else if (vio_idx >= 131 && vio_idx <= 157)
            return 7;
        else if (vio_idx == 158)
            return 1;
        else if (vio_idx == 159)
            return 2;
        else if (vio_idx == 160)
            return 3;
        else if (vio_idx == 161)
            return 4;
        else if (vio_idx == 162)
            return 6;
        else if (vio_idx == 163)
            return 7;
        else
            return -1;
    } else if (dapc_type == HAL_DEVAPC_TYPE_AUD) {
        if (vio_idx <= 6)
            return 0;
        else if (vio_idx == 7)
            return 1;
        else if (vio_idx >= 8 && vio_idx <= 11)
            return 2;
        else if (vio_idx >= 12 && vio_idx <= 15)
            return 3;
        else if (vio_idx >= 16 && vio_idx <= 19)
            return 4;
        else if (vio_idx >= 20 && vio_idx <= 23)
            return 5;
        else if (vio_idx >= 24 && vio_idx <= 32)
            return 6;
        else if (vio_idx >= 33 && vio_idx <= 34)
            return 7;
        else if (vio_idx == 35)
            return 0;
        else if (vio_idx == 36)
            return 1;
        else if (vio_idx == 37)
            return 2;
        else if (vio_idx == 38)
            return 3;
        else if (vio_idx == 39)
            return 4;
        else if (vio_idx == 40)
            return 5;
        else if (vio_idx >= 41 && vio_idx <= 50)
            return 6;
        else if (vio_idx >= 51 && vio_idx <= 52)
            return 7;
        else if (vio_idx == 53)
            return 6;
        else if (vio_idx == 54)
            return 7;
        else
            return -1;
    }

    return -1;
}

/* return true if sync done */
static int sync_vio_dbg(unsigned int base, unsigned int shift_bit)
{
    unsigned long vio_shift_sta, vio_shift_sel, vio_shift_con;
    int shift_count, sync_done;

    vio_shift_sta = base + PD_VIO_SHIFT_STA;
    vio_shift_sel = base + PD_VIO_SHIFT_SEL;
    vio_shift_con = base + PD_VIO_SHIFT_CON;

    WRITE_REG(0x1 << shift_bit, vio_shift_sel);
    WRITE_REG(0x1, vio_shift_con);

    for (shift_count = 0;
         (shift_count < 100) && ((READ_REG(vio_shift_con) & 0x3) != 0x3);
         ++shift_count);

    if ((READ_REG(vio_shift_con) & 0x3) == 0x3)
        sync_done = 1;
    else {
        sync_done = 0;
        log_hal_error("[DEVAPC] sync failed, shift_bit: 0x%x\n", shift_bit);
    }

    /* Disable shift mechanism */
    WRITE_REG(0x0, vio_shift_con);
    WRITE_REG(0x0, vio_shift_sel);
    WRITE_REG(0x1 << shift_bit, vio_shift_sta);

    return sync_done;
}

static void extract_vio_dbg(struct DEVAPC_INFO *dapc)
{
    unsigned long vio_dbg0, vio_dbg1, vio_dbg2;
    unsigned int dbg0_val;
    struct devapc_vio_data dbg_data;

    vio_dbg0 = dapc->pd_base + PD_VIO_DBG_0;
    vio_dbg1 = dapc->pd_base + PD_VIO_DBG_1;
    vio_dbg2 = dapc->pd_base + PD_VIO_DBG_2;

    /* Extract violation information */
    dbg0_val = READ_REG(vio_dbg0);
    dbg_data.vio_addr_high = (dbg0_val >> 8) & 0xF;
    dbg_data.r_vio = (dbg0_val >> 7) & 0x1;
    dbg_data.w_vio = (dbg0_val >> 6) & 0x1;
    dbg_data.domain_id = (dbg0_val >> 0) & 0x3F;

    dbg_data.vio_id = READ_REG(vio_dbg1);

    dbg_data.vio_addr = READ_REG(vio_dbg2);

    /* Print vio info */
    log_hal_error("[DEVAPC] %s Violation (%s%s) - %s: 0x%x, %s: 0x%x, %s: 0x%x, %s: 0x%x\n",
                  dapc->name,
                  dbg_data.r_vio == 1 ? "R" : "",
                  dbg_data.w_vio == 1 ? "W" : "",
                  "Vio Addr", dbg_data.vio_addr,
                  "High", dbg_data.vio_addr_high,
                  "Bus ID", dbg_data.vio_id,
                  "Domain ID", dbg_data.domain_id);
}

//TFM IRQ call this function directly, this function cannot be `static`.
void devapc_irq_handler(hal_nvic_irq_t irq_number)
{
    struct DEVAPC_INFO *dapc;
    //unsigned int state;
    unsigned int i;
    int shift_bit;

    // TODO: Grab spin_lock
    //state = MTEE_SpinLockMaskIrq(&g_dapc_lock);

    hal_nvic_disable_irq(irq_number);

    dapc = IRQ_NUM_TO_P_DAPC(irq_number);

    /* Mask violaion */
    set_device_mask(dapc, 1);

    /* Check the violation status of each device */
    for (i = 0; i < dapc->vio_device_num; i++) {

        /* Check vio_sta */
        if (!get_clr_vio_sta(dapc, i, 0))
            continue;

        /* Get shift_bit */
        shift_bit = get_shift_bit(DAPC_NO(dapc), i);
        log_hal_info("[DEVAPC] %s vio_sta found: %u, shift_bit: %d\n", dapc->name, i, shift_bit);

        /* Should not happen, check it again in case */
        if (shift_bit < 0)
            continue;

        /* Sync vio info */
        if (sync_vio_dbg(dapc->pd_base, shift_bit)) {
            /* Extract & print vio info */
            extract_vio_dbg(dapc);
        }

        /* Clear vio_sta */
        get_clr_vio_sta(dapc, i, 1);
    }

    /* Unmask violaion */
    set_device_mask(dapc, 0);

    hal_nvic_enable_irq(irq_number);

    // TODO: Drop spin_lock
    //MTEE_SpinUnlockRestoreIrq(&g_dapc_lock, state);
}

static int start_devapc(struct DEVAPC_INFO *dapc)
{
    hal_nvic_irq_t irq_num;
    int ret = 0;

    irq_num = dapc->irq_num;

    ret += hal_nvic_disable_irq(irq_num);

    ret += hal_nvic_register_isr_handler(irq_num, devapc_irq_handler);

    ret += hal_nvic_irq_set_type(irq_num, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);

    ret += hal_nvic_enable_irq(irq_num);

    if (!ret) {
        set_device_mask(dapc, 0);
    }

    return ret;
}

#endif /* #ifdef HAL_NVIC_MODULE_ENABLED */

/******************************
 * Utils
 *******************************/
hal_devapc_status_t hal_devapc_set_master_domain(hal_devapc_type_t dapc_type, unsigned int master, hal_devapc_dom_t dom)
{
    struct DEVAPC_INFO *dapc;
    unsigned int dom_index = 0;
    unsigned int dom_bit_index = 0;
    unsigned long reg;
    unsigned int val;

    /* input check */
    if (dapc_type >= HAL_DEVAPC_TYPE_MAX)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    if (dom >= HAL_DEVAPC_DOMAIN_MAX)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    dapc = P_DAPC(dapc_type);
    if (master >= dapc->master_dom_num)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    dom_index = master / MOD_NO_IN_1_DOM;
    dom_bit_index = master % MOD_NO_IN_1_DOM;

    reg = dapc->ao_base + AO_MAS_DOM_0 + (dom_index * 0x4);

    val = READ_REG(reg);
    val &= ~(0x7 << (dom_bit_index * 8));
    val |= (dom & 0x7) << (dom_bit_index * 8);
    WRITE_REG(val, reg);

#ifdef MTK_DEBUG_LEVEL_DEBUG
    log_hal_info("[DEVAPC] Set %s master (%u) to domain %d: set 0x%x to 0x%x\n",
                 dapc->name, master, dom, reg, READ_REG(reg));
#endif /* #ifdef MTK_DEBUG_LEVEL_DEBUG */

    return HAL_DEVAPC_STATUS_OK;
}

hal_devapc_status_t hal_devapc_set_module_apc(hal_devapc_type_t dapc_type, unsigned int module, hal_devapc_dom_t dom, hal_devapc_apc_t perm)
{
    struct DEVAPC_INFO *dapc;
    unsigned int apc_index = 0;
    unsigned int apc_bit_index = 0;
    unsigned long reg;
    unsigned int val;

    /* input check */
    if (dapc_type >= HAL_DEVAPC_TYPE_MAX)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    if (dom >= HAL_DEVAPC_DOMAIN_MAX)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    if (perm >= HAL_DEVAPC_APC_MAX)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    dapc = P_DAPC(dapc_type);
    if (module >= dapc->apc_device_num)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    apc_index = module / MOD_NO_IN_1_APC;
    apc_bit_index = module % MOD_NO_IN_1_APC;

    reg = dapc->ao_base + AO_D0_APC_0 + (apc_index * 0x4) + (dom * 0x40);

    val = READ_REG(reg);
    val &= ~(0x3 << (2 * apc_bit_index));
    val |= (perm << (2 * apc_bit_index));
    WRITE_REG(val, reg);

#ifdef MTK_DEBUG_LEVEL_DEBUG
    log_hal_info("[DEVAPC] Set %s module (%u) D%d apc: set 0x%x to 0x%x\n",
                 dapc->name, module, dom, reg, READ_REG(reg));
#endif /* #ifdef MTK_DEBUG_LEVEL_DEBUG */

    return HAL_DEVAPC_STATUS_OK;
}

hal_devapc_status_t hal_devapc_set_apc_lock(hal_devapc_type_t dapc_type, unsigned int module)
{
    struct DEVAPC_INFO *dapc;
    unsigned int apc_index = 0;
    unsigned int apc_bit_index = 0;
    unsigned long reg;
    unsigned int val;

    /* input check */
    if (dapc_type >= HAL_DEVAPC_TYPE_MAX)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    dapc = P_DAPC(dapc_type);
    if (module >= dapc->apc_device_num)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    apc_index = module / MOD_NO_IN_1_LOCK;
    apc_bit_index = module % MOD_NO_IN_1_LOCK;

    reg = dapc->ao_base + AO_APC_LOCK_0 + (apc_index * 0x4);

    val = READ_REG(reg);
    val &= ~(0x1 << apc_bit_index);
    val |= (0x1 << apc_bit_index);
    WRITE_REG(val, reg);

#ifdef MTK_DEBUG_LEVEL_DEBUG
    log_hal_info("[DEVAPC] Set %s module (%u) lock: set 0x%x to 0x%x\n",
                 dapc->name, module, reg, READ_REG(reg));
#endif /* #ifdef MTK_DEBUG_LEVEL_DEBUG */

    return HAL_DEVAPC_STATUS_OK;
}

hal_devapc_status_t hal_devapc_get_master_domain(hal_devapc_type_t dapc_type, unsigned int master, hal_devapc_dom_t *dom)
{
    struct DEVAPC_INFO *dapc;
    unsigned int dom_index = 0;
    unsigned int dom_bit_index = 0;
    unsigned long reg;
    unsigned int val;

    /* input check */
    if (dapc_type >= HAL_DEVAPC_TYPE_MAX)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    if (dom == NULL)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    dapc = P_DAPC(dapc_type);
    if (master >= dapc->master_dom_num)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    dom_index = master / MOD_NO_IN_1_DOM;
    dom_bit_index = master % MOD_NO_IN_1_DOM;

    reg = dapc->ao_base + AO_MAS_DOM_0 + (dom_index * 0x4);

    val = READ_REG(reg);
    val &= (0x7 << (dom_bit_index * 8));
    *dom = val >> (dom_bit_index * 8);

#ifdef MTK_DEBUG_LEVEL_DEBUG
    log_hal_info("[DEVAPC] Get domain %d from the %s master (%u): get 0x%x from 0x%x\n",
                 *dom, dapc->name, master, READ_REG(reg), reg);
#endif /* #ifdef MTK_DEBUG_LEVEL_DEBUG */

    return HAL_DEVAPC_STATUS_OK;
}

hal_devapc_status_t hal_devapc_get_module_apc(hal_devapc_type_t dapc_type, unsigned int module, hal_devapc_dom_t dom, hal_devapc_apc_t *perm)
{
    struct DEVAPC_INFO *dapc;
    unsigned int apc_index = 0;
    unsigned int apc_bit_index = 0;
    unsigned long reg;
    unsigned int val;

    /* input check */
    if (dapc_type >= HAL_DEVAPC_TYPE_MAX)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    if (dom >= HAL_DEVAPC_DOMAIN_MAX)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    if (perm == NULL)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    dapc = P_DAPC(dapc_type);
    if (module >= dapc->apc_device_num)
        return HAL_DEVAPC_STATUS_INVALID_PARAMETER;

    apc_index = module / MOD_NO_IN_1_APC;
    apc_bit_index = module % MOD_NO_IN_1_APC;

    reg = dapc->ao_base + AO_D0_APC_0 + (apc_index * 0x4) + (dom * 0x40);

    val = READ_REG(reg);
    val &= 0x3 << (2 * apc_bit_index);
    val = (val >> (2 * apc_bit_index));
    *perm = val;

#ifdef MTK_DEBUG_LEVEL_DEBUG
    log_hal_info("[DEVAPC] get %s module (%u) D%d apc: set 0x%x to 0x%x\n",
                 dapc->name, module, dom, reg, READ_REG(reg));
#endif /* #ifdef MTK_DEBUG_LEVEL_DEBUG */

    return HAL_DEVAPC_STATUS_OK;
}

hal_devapc_status_t hal_devapc_infra_devapc_init(void)
{
    struct DEVAPC_INFO *dapc;
    int ret_sta = 0;
    int shift_bit = 0;

    dapc = P_DAPC(HAL_DEVAPC_TYPE_INFRA);

    WRITE_REG(0x0, dapc->ao_base + AO_APC_CON);
    WRITE_REG(0x0, dapc->pd_base + PD_PDN_APC_CON);

    if (get_clr_vio_sta(&g_devapc_info[HAL_DEVAPC_TYPE_INFRA], 158, 0)) {
        //Ignore DAPC violations 158
        //clear DAPC TX violation status, if DAPC doesn't clear DAPC TX violation, DAPC RX get the violation from TX immediately the violation status bit will keep raise
        shift_bit = get_shift_bit(HAL_DEVAPC_TYPE_INFRA, 158);
        if (!sync_vio_dbg(g_devapc_info[HAL_DEVAPC_TYPE_INFRA].pd_base, shift_bit)) {
            log_hal_error("[DEVAPC] clear DAPC TX violation status fail!!\n");
        }
        //clear DAPC RX violation status
        get_clr_vio_sta(&g_devapc_info[HAL_DEVAPC_TYPE_INFRA], 158, 1);
    }

#ifdef HAL_NVIC_MODULE_ENABLED
    if (start_devapc(dapc)) {
        log_hal_error("[DEVAPC] Failed to start %s devapc!!\n", dapc->name);
        ret_sta++;
    }
#endif /* #ifdef HAL_NVIC_MODULE_ENABLED */

    if (!ret_sta)
        return HAL_DEVAPC_STATUS_OK;
    else
        return HAL_DEVAPC_STATUS_GENERIC_ERROR;
}

hal_devapc_status_t hal_devapc_aud_devapc_init(void)
{
    struct DEVAPC_INFO *dapc;
    int ret_sta = 0;

    dapc = P_DAPC(HAL_DEVAPC_TYPE_AUD);

    WRITE_REG(0x0, dapc->ao_base + AO_APC_CON);
    WRITE_REG(0x0, dapc->pd_base + PD_PDN_APC_CON);

#ifdef HAL_NVIC_MODULE_ENABLED
    if (start_devapc(dapc)) {
        log_hal_error("[DEVAPC] Failed to start %s devapc!!\n", dapc->name);
        ret_sta++;
    }
#endif /* #ifdef HAL_NVIC_MODULE_ENABLED */

    if (!ret_sta)
        return HAL_DEVAPC_STATUS_OK;
    else
        return HAL_DEVAPC_STATUS_GENERIC_ERROR;
}

void hal_devapc_trigger_ut_vio(void)
{
    // test INFRA DEVAPC
    log_hal_info("%s: Trigger INFRA DEVAPC violation...\n", __func__);
    hal_devapc_set_module_apc(HAL_DEVAPC_TYPE_INFRA, 44, HAL_DEVAPC_DOMAIN_0, HAL_DEVAPC_APC_FORBIDDEN);
    hal_devapc_set_module_apc(HAL_DEVAPC_TYPE_INFRA, 63, HAL_DEVAPC_DOMAIN_0, HAL_DEVAPC_APC_FORBIDDEN);

    READ_REG(0x30485000U);
    WRITE_REG(0x12345678, 0x30403000U);

    hal_devapc_set_module_apc(HAL_DEVAPC_TYPE_INFRA, 44, HAL_DEVAPC_DOMAIN_0, HAL_DEVAPC_APC_NO_PROTECTION);
    hal_devapc_set_module_apc(HAL_DEVAPC_TYPE_INFRA, 63, HAL_DEVAPC_DOMAIN_0, HAL_DEVAPC_APC_NO_PROTECTION);

    // test AUD DEVAPC
    log_hal_info("%s: Trigger AUD DEVAPC violation...\n", __func__);
    hal_devapc_set_module_apc(HAL_DEVAPC_TYPE_AUD, 25, HAL_DEVAPC_DOMAIN_0, HAL_DEVAPC_APC_FORBIDDEN);
    hal_devapc_set_module_apc(HAL_DEVAPC_TYPE_AUD, 31, HAL_DEVAPC_DOMAIN_0, HAL_DEVAPC_APC_FORBIDDEN);

    READ_REG(0x41001000U);
    WRITE_REG(0x33333333, 0x4100A000U);

    hal_devapc_set_module_apc(HAL_DEVAPC_TYPE_AUD, 25, HAL_DEVAPC_DOMAIN_0, HAL_DEVAPC_APC_NO_PROTECTION);
    hal_devapc_set_module_apc(HAL_DEVAPC_TYPE_AUD, 31, HAL_DEVAPC_DOMAIN_0, HAL_DEVAPC_APC_NO_PROTECTION);

    log_hal_info("%s: Done. Please check if violation is handled...\n", __func__);
}
