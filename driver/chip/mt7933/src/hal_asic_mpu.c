/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2020
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
#include <hal_asic_mpu.h>
#include <hal_asic_mpu_internal.h>

#ifdef HAL_NVIC_MODULE_ENABLED
#include "hal_nvic.h"
#endif /* #ifdef HAL_NVIC_MODULE_ENABLED */

static char mpu_type_str[MPU_NUM][10] = {
    "FLASH",
    "SYSRAM",
    "PSRAM",
    "TCM"
};
/******************************
 * Function
 *******************************/
hal_asic_mpu_status_t hal_asic_mpu_set_region_apc(hal_asic_mpu_type_t mpu_type, unsigned int region, int domain, unsigned int apc)
{
    unsigned int addr;
    unsigned int apc_shift;
    unsigned int val;

    if (mpu_type >= MPU_NUM || !(region < REGION_NUM)) {
        return -HAL_ASIC_MPU_STATUS_INVALID_PARAMETER;
    }

    addr = MPU_OFFSET(mpu_type, R0_APC_OFFSET) + (region * 0x4);

#ifdef MTK_DEBUG_LEVEL_DEBUG
    log_hal_debug("Set apc to MPU#%d region %u (0x%x)...\n", mpu_type, region, addr);
    log_hal_debug("(to-set) domain = %d, apc = %u\n", domain, apc);
    log_hal_debug("(pre-set) apc setting (0x%x) = 0x%x\n", addr, READ_REG(addr));
#endif /* #ifdef MTK_DEBUG_LEVEL_DEBUG */

    if (domain == -1) {
        WRITE_REG(apc, addr);
    } else if (domain >= 0 && domain < DOMAIN_NUM) {
        apc_shift = (domain * 4);
        val = (READ_REG(addr) & ~(0x7 << apc_shift)) | (apc << apc_shift);

        WRITE_REG(val, addr);
    } else {
        log_hal_error("WARNING: domain out of bound!!\n");
        return -HAL_ASIC_MPU_STATUS_INVALID_PARAMETER;
    }

#ifdef MTK_DEBUG_LEVEL_DEBUG
    log_hal_debug("(post-set) apc setting (0x%x) = 0x%x\n", addr, READ_REG(addr));
#endif /* #ifdef MTK_DEBUG_LEVEL_DEBUG */

    return HAL_ASIC_MPU_STATUS_OK;
}

hal_asic_mpu_status_t hal_asic_mpu_set_region_cfg(hal_asic_mpu_type_t mpu_type, unsigned int region, unsigned int en, unsigned int start_addr)
{
    unsigned int RX_CFG;
    unsigned int val;

    if (mpu_type >= MPU_NUM || !(region < REGION_NUM)) {
        return -HAL_ASIC_MPU_STATUS_INVALID_PARAMETER;
    }

    RX_CFG = MPU_OFFSET(mpu_type, R0_CFG_OFFSET) + (region * 0x4);

    val = ((en ? 1 : 0) << 31) | (start_addr >> 12);
    WRITE_REG(val, RX_CFG);

    return HAL_ASIC_MPU_STATUS_OK;
}

hal_asic_mpu_status_t hal_asic_mpu_set_region_apc_lock(hal_asic_mpu_type_t mpu_type)
{
    unsigned int RX_CFG;
    unsigned int val;

    if (mpu_type >= MPU_NUM) {
        return -HAL_ASIC_MPU_STATUS_INVALID_PARAMETER;
    }

    if (mpu_type == HAL_ASIC_MPU_TYPE_SYSRAM)
        val = 0x10001;
    else
        val = 0x1;

    RX_CFG = MPU_OFFSET(mpu_type, CFG_OFFSET);

    WRITE_REG(val, RX_CFG);

    return HAL_ASIC_MPU_STATUS_OK;
}

hal_asic_mpu_status_t hal_asic_mpu_get_region_apc(hal_asic_mpu_type_t mpu_type, unsigned int region, int domain, unsigned int *apc)
{
    unsigned int addr;
    unsigned int apc_shift;
    unsigned int val;

    if (mpu_type >= MPU_NUM || !(region < REGION_NUM) || ((domain < 0) || (domain > DOMAIN_NUM))) {
        return -HAL_ASIC_MPU_STATUS_INVALID_PARAMETER;
    }

    addr = MPU_OFFSET(mpu_type, R0_APC_OFFSET) + (region * 0x4);
    apc_shift = (domain * 4);
    val = READ_REG(addr) & (0x7 << apc_shift);
    *apc = val >> apc_shift;

#ifdef MTK_DEBUG_LEVEL_DEBUG
    log_hal_debug("Get apc to MPU#%d region %u (0x%x) = 0x%x\n", mpu_type, region, addr, READ_REG(addr));
#endif /* #ifdef MTK_DEBUG_LEVEL_DEBUG */

    return HAL_ASIC_MPU_STATUS_OK;
}

hal_asic_mpu_status_t hal_asic_mpu_get_region_cfg(hal_asic_mpu_type_t mpu_type, unsigned int region, unsigned int *start_addr)
{
    unsigned int RX_CFG;
    unsigned int val;

    if (mpu_type >= MPU_NUM || !(region < REGION_NUM)) {
        return -HAL_ASIC_MPU_STATUS_INVALID_PARAMETER;
    }

    RX_CFG = MPU_OFFSET(mpu_type, R0_CFG_OFFSET) + (region * 0x4);

    val = READ_REG(RX_CFG);
    *start_addr = val & 0xFFFFF;

    return HAL_ASIC_MPU_STATUS_OK;
}

hal_asic_mpu_status_t hal_asic_mpu_asic_mpu_init(void)
{
    hal_nvic_irq_t irq_num;
    int ret = 0;

#ifdef HAL_NVIC_MODULE_ENABLED
    // request IRQ
    irq_num = MPU_L2_PWR_IRQn;
    ret += hal_nvic_disable_irq(irq_num);
    ret += hal_nvic_register_isr_handler(irq_num, asic_mpu_irq_handler);
    ret += hal_nvic_irq_set_type(irq_num, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    ret += hal_nvic_enable_irq(irq_num);

    if (ret) {
        log_hal_info("[ASIC_MPU] request IRQ for MPU_L2 FAIL\n");
    }

    irq_num = MPU_PSRAM_PWR_IRQn;
    ret += hal_nvic_disable_irq(irq_num);
    ret += hal_nvic_register_isr_handler(irq_num, asic_mpu_irq_handler);
    ret += hal_nvic_irq_set_type(irq_num, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    ret += hal_nvic_enable_irq(irq_num);

    if (ret) {
        log_hal_info("[ASIC_MPU] request IRQ for MPU_PSRAM FAIL\n");
    }

    if (!ret) {
        // Enable asic_mpu IRQ
        WRITE_REG(0xFF, MPU_IRQ_EA);
    }
#endif /* #ifdef HAL_NVIC_MODULE_ENABLED */

    if (!ret)
        return HAL_ASIC_MPU_STATUS_OK;
    else
        return HAL_ASIC_MPU_STATUS_GENERIC_ERROR;
}

#ifdef HAL_NVIC_MODULE_ENABLED
static void update_vio_info(unsigned int id, struct mpu_unit_vio_info *p_info)
{
    unsigned int val;

    // irq sta
    val = READ_REG(MPU_IRQ_STA);
    p_info->irq_sta[0] = GET_BIT(val, (2 * id) + 1);
    p_info->irq_sta[1] = GET_BIT(val, (2 * id));

    // RABN_0
    val = READ_REG(MPU_OFFSET(id, RABN_0_OFFSET));
    p_info->abn_id[0] = val & 0x1FFF;

    // RABN_1
    val = READ_REG(MPU_OFFSET(id, RABN_1_OFFSET));
    p_info->irq_overflow[0] = GET_BIT(val, 28);
    p_info->abn_apc[0] = (val & (0x7 << 24)) >> 24;
    p_info->abn_prot[0] = (val & (0x7 << 20)) >> 20;
    p_info->abn_domain[0] = (val & (0x7 << 16)) >> 16;
    p_info->abn_region[0] = val & 0xFFFF;

    // RABN_2
    val = READ_REG(MPU_OFFSET(id, RABN_2_OFFSET));
    p_info->abn_addr[0] = val;

    // WABN_0
    val = READ_REG(MPU_OFFSET(id, WABN_0_OFFSET));
    p_info->abn_id[1] = val & 0x1FFF;

    // WABN_1
    val = READ_REG(MPU_OFFSET(id, WABN_1_OFFSET));
    p_info->irq_overflow[1] = GET_BIT(val, 28);
    p_info->abn_apc[1] = (val & (0x7 << 24)) >> 24;
    p_info->abn_prot[1] = (val & (0x7 << 20)) >> 20;
    p_info->abn_domain[1] = (val & (0x7 << 16)) >> 16;
    p_info->abn_region[1] = val & 0xFFFF;

    // WABN_2
    val = READ_REG(MPU_OFFSET(id, WABN_2_OFFSET));
    p_info->abn_addr[1] = val;

    // clear registers
    WRITE_REG(0x3 << (2 * id), MPU_IRQ_STA);
    WRITE_REG(0x3 << (2 * id), MPU_ERR_STS_CLR);
    WRITE_REG(0x0, MPU_ERR_STS_CLR);   // 0->0 does not affect, we ment to make 1->0
}

static void dump_vio_info(struct mpu_unit_vio_info *p_info, int set)
{
    int is_privileged_access;
    int is_NS_access;
    int is_instruction_access;

    is_privileged_access = (p_info->abn_prot[set] >> 0) & 0x1;
    is_NS_access = (p_info->abn_prot[set] >> 1) & 0x1;
    is_instruction_access = (p_info->abn_prot[set] >> 2) & 0x1;

    if (p_info->irq_sta[set]) {
        log_hal_error("[ASIC_MPU] (%s Violation) %s: 0x%x, %s: 0x%x, %s: 0x%x, %s: 0x%x\n",
                      set ? "W" : "R",
                      "Permission", p_info->abn_apc[set],
                      "Domain", p_info->abn_domain[set],
                      "Region", p_info->abn_region[set],
                      "Addr", p_info->abn_addr[set]);

        log_hal_error("[ASIC_MPU] Access type: %s, %s, %s\n",
                      is_privileged_access ? "Privileged" : "Normal",
                      is_NS_access ? "Non-secure" : "Secure",
                      is_instruction_access ? "Instruction" : "Data");

        log_hal_error("[ASIC_MPU] ABN ID: 0x%x\n", p_info->abn_id[set]);
    }

}

void asic_mpu_irq_handler(hal_nvic_irq_t irq_number)
{
    //unsigned int state;
    unsigned int irq_sta;
    int mpu_id, bit;
    struct mpu_unit_vio_info vio_info;

    // TODO: Grab spin_lock
    //state = MTEE_SpinLockMaskIrq(&g_dapc_lock);

    hal_nvic_disable_irq(irq_number);

    irq_sta = READ_REG(MPU_IRQ_STA);

    log_hal_error("[ASIC_MPU] IRQ_STA: 0x%x\n", irq_sta);

    // check vio sta of each mpu and update vio info
    for (mpu_id = 0, bit = 0; mpu_id < MPU_NUM; mpu_id++, bit += 2) {
        if (GET_BIT(irq_sta, bit) || GET_BIT(irq_sta, (bit + 1))) {
            log_hal_error("[ASIC_MPU] %s MPU Violation!!\n", mpu_type_str[mpu_id]);
            log_hal_error("[ASIC_MPU] Dumping Vio Info...\n");

            update_vio_info(mpu_id, &vio_info);

            dump_vio_info(&vio_info, 0);
            dump_vio_info(&vio_info, 1);
        }
    }

    hal_nvic_enable_irq(irq_number);

    // TODO: Drop spin_lock
    //MTEE_SpinUnlockRestoreIrq(&g_dapc_lock, state);
}

void hal_asic_mpu_vio_ut(void)
{
    hal_nvic_irq_t irq_num;
    int ret = 0;

    // request IRQ
    irq_num = MPU_L2_PWR_IRQn;
    ret += hal_nvic_disable_irq(irq_num);
    ret += hal_nvic_register_isr_handler(irq_num, asic_mpu_irq_handler);
    ret += hal_nvic_irq_set_type(irq_num, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    ret += hal_nvic_enable_irq(irq_num);

    if (ret) {
        log_hal_info("[ASIC_MPU] request IRQ for MPU_L2 FAIL\n");
        ret = 0;
    }

    irq_num = MPU_PSRAM_PWR_IRQn;
    ret += hal_nvic_disable_irq(irq_num);
    ret += hal_nvic_register_isr_handler(irq_num, asic_mpu_irq_handler);
    ret += hal_nvic_irq_set_type(irq_num, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    ret += hal_nvic_enable_irq(irq_num);

    if (ret) {
        log_hal_info("[ASIC_MPU] request IRQ for MPU_PSRAM FAIL\n");
    }

    // Enable asic_mpu IRQ
    WRITE_REG(0xFF, 0x30000014);

    // set SYSRAM region 1: 0x800A0000 ~ 0x800C0000
    hal_asic_mpu_set_region_cfg(HAL_ASIC_MPU_TYPE_SYSRAM, 1, 1, 0x800A0000);
    hal_asic_mpu_set_region_cfg(HAL_ASIC_MPU_TYPE_SYSRAM, 2, 0, 0x800C0000);

    // access OK
    WRITE_REG(0xAABBCCDD, 0x800A0004);
    log_hal_info("%s read 0x800A0004: 0x%x\n", __func__, READ_REG(0x800A0004));
    //asic_mpu_irq_handler(54);

    // protect SYSRAM region 1: ACCESS_DENIED
    hal_asic_mpu_set_region_apc(HAL_ASIC_MPU_TYPE_SYSRAM, 1, -1, 0x77777777);
    WRITE_REG(0xFFFFFFFF, 0x800A0004);
    log_hal_info("%s read 0x800A0004: 0x%x\n", __func__, READ_REG(0x800A0004));
    //asic_mpu_irq_handler(54);

    // Reset setting
    hal_asic_mpu_set_region_apc(HAL_ASIC_MPU_TYPE_SYSRAM, 1, -1, 0x0);
    hal_asic_mpu_set_region_cfg(HAL_ASIC_MPU_TYPE_SYSRAM, 1, 0, 0x800A0000);
}
#endif /* #ifdef HAL_NVIC_MODULE_ENABLED */
