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
#include <string.h>
#include "hal_cache.h"

#ifdef HAL_CACHE_MODULE_ENABLED

#include "hal_cache_internal.h"
#include "hal_log.h"
#include "assert.h"
#include "memory_attribute.h"

#include "mt7933.h"


#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */


#define REG_BEGIN(r)    (CACHE->CACHE_ENTRY_N[r] & ~CACHE_ENTRY_N_C_MASK)
#define REG_END(r)      (CACHE->CACHE_END_ENTRY_N[r])
#define IN_REG(addr, r) ((REG_BEGIN(r) <= addr) && (addr < REG_END(r)))
/*
 *  Notice:
 *  For speeding up SDK boot time, CM33 cache must be initiated as early as possible. In MTK SDK, developers must aware the following APIs
 *  is called in BL: ResetISR() and RTOS:startup_mt7933.s. Therefore, developers must NOT use or declare any global variable or declare any
 *  local variable with "static" in those APIs to prevent system hang up.
 */
static hal_cache_size_t _hal_cache_size(void)
{
    uint32_t            cache_con;
    hal_cache_size_t    cache_size;

    cache_con = CACHE->CACHE_CON & CACHE_CON_CACHESIZE_MASK;
    cache_size = (hal_cache_size_t)(cache_con >> CACHE_CON_CACHESIZE_OFFSET);

    return cache_size;
}


hal_cache_status_t hal_cache_init(void)
{
    hal_cache_region_t  region;
    uint32_t            irq_flag;
    hal_cache_status_t  status;

    /*
     * In order to prevent race condition, interrupt should be disabled when
     * query and update global variable which indicates the module status
     */
    irq_flag = save_and_set_interrupt_mask();

    do {
        /* return busy if cache is already on */
        if ((CACHE->CACHE_CON & CACHE_CON_MCEN_MASK) != 0) {
            status = HAL_CACHE_STATUS_ERROR_BUSY;
            break;
        }

        /*
         * Flush and invalidate all cache lines before use, no matter cache
         * is enabled or not.
         */
        hal_cache_invalidate_all_cache_lines();

        /* Set CACHE related registers to default value*/
        CACHE->CACHE_CON = 0;
        CACHE->CACHE_REGION_EN = 0;

        /* Set Cache hit count to zero */
        CACHE->CACHE_HCNT0L = 0;
        CACHE->CACHE_HCNT0U = 0;
        CACHE->CACHE_HCNT1L = 0;
        CACHE->CACHE_HCNT1U = 0;

        /* Set Cache access count to zero */
        CACHE->CACHE_CCNT0L = 0;
        CACHE->CACHE_CCNT0U = 0;
        CACHE->CACHE_CCNT1L = 0;
        CACHE->CACHE_CCNT1U = 0;

        /* Set CACHE region registers to default value and update the global variable */
        for (region = HAL_CACHE_REGION_0; region < HAL_CACHE_REGION_MAX; region ++) {
            /* Set CACHE related registers to default value */
            CACHE->CACHE_ENTRY_N[region] = 0;
            CACHE->CACHE_END_ENTRY_N[region] = 0;
        }

        status = HAL_CACHE_STATUS_OK;
    } while (0);

    /* Restore the previous status of interrupt */
    restore_interrupt_mask(irq_flag);

    return status;
}


hal_cache_status_t hal_cache_deinit(void)
{
    hal_cache_region_t region;

    /* flush and invalidate all cache lines */
    /* this function will flush and invalidate all cache lines */
    hal_cache_invalidate_all_cache_lines();

    /* Set CACHE related registers to default value */
    CACHE->CACHE_CON = 0;
    CACHE->CACHE_REGION_EN = 0;

    /* Set CACHE region registers to default value and update the global variable */
    for (region = HAL_CACHE_REGION_0; region < HAL_CACHE_REGION_MAX; region ++) {
        /* Set CACHE related registers to default value */
        CACHE->CACHE_ENTRY_N[region] = 0;
        CACHE->CACHE_END_ENTRY_N[region] = 0;
    }

    return HAL_CACHE_STATUS_OK;
}


hal_cache_status_t hal_cache_enable(void)
{
    /* CACHE should not be enabled when the CACHE size is 0KB */
    if (HAL_CACHE_SIZE_0KB == _hal_cache_size()) {
        return HAL_CACHE_STATUS_ERROR_CACHE_SIZE;
    }

    /* Enable CACHE */
    CACHE->CACHE_CON |= CACHE_CON_MCEN_MASK | CACHE_CON_CNTEN0_MASK | CACHE_CON_CNTEN1_MASK;

    return HAL_CACHE_STATUS_OK;
}


hal_cache_status_t hal_cache_disable(void)
{
    uint32_t irq_flag;

    /* In order to prevent race condition, interrupt should be disabled when flush cache */
    irq_flag = save_and_set_interrupt_mask();

    /* If CACHE is enabled, flush and invalidate all cache lines */
    if (CACHE->CACHE_CON & CACHE_CON_MCEN_MASK) {
        /* this function will flush and invalidate all cache lines */
        hal_cache_invalidate_all_cache_lines();
    }

    /* Disable CACHE */
    CACHE->CACHE_CON &= ~CACHE_CON_MCEN_MASK;

    /* Set Cache hit count to zero */
    CACHE->CACHE_HCNT0L = 0;
    CACHE->CACHE_HCNT0U = 0;
    CACHE->CACHE_HCNT1L = 0;
    CACHE->CACHE_HCNT1U = 0;

    /* Set Cache access count to zero */
    CACHE->CACHE_CCNT0L = 0;
    CACHE->CACHE_CCNT0U = 0;
    CACHE->CACHE_CCNT1L = 0;
    CACHE->CACHE_CCNT1U = 0;

    /* Restore the previous status of interrupt */
    restore_interrupt_mask(irq_flag);

    return HAL_CACHE_STATUS_OK;
}


hal_cache_status_t hal_cache_region_enable(hal_cache_region_t region)
{
    /* Region is invalid */
    if (region >= HAL_CACHE_REGION_MAX) {
        return HAL_CACHE_STATUS_ERROR_REGION;
    }

    /* The region should be configured before region is enabled */
    if (CACHE->CACHE_ENTRY_N[region] & CACHE_ENTRY_N_C_MASK) {
        CACHE->CACHE_REGION_EN |= (1 << region);
    } else {
        return HAL_CACHE_STATUS_ERROR;
    }

    return HAL_CACHE_STATUS_OK;
}


hal_cache_status_t hal_cache_region_disable(hal_cache_region_t region)
{
    /* Region is invalid */
    if (region >= HAL_CACHE_REGION_MAX) {
        return HAL_CACHE_STATUS_ERROR_REGION;
    }

    /* Disable the corresponding region */
    CACHE->CACHE_REGION_EN &= ~(1 << region);

    return HAL_CACHE_STATUS_OK;
}


hal_cache_status_t hal_cache_set_size(hal_cache_size_t cache_size)
{
    /* CACHE size is invalid */
    if (cache_size >= HAL_CACHE_SIZE_MAX) {
        return HAL_CACHE_STATUS_ERROR_CACHE_SIZE;
    }

    /* Set CACHE size */
    CACHE->CACHE_CON &= ~CACHE_CON_CACHESIZE_MASK;
    CACHE->CACHE_CON |= (cache_size << CACHE_CON_CACHESIZE_OFFSET);

    /* When CACHE size is 0KB, make sure the CACHE is disabled */
    if (cache_size == HAL_CACHE_SIZE_0KB) {
        CACHE->CACHE_CON = 0;
    }

    return HAL_CACHE_STATUS_OK;
}


hal_cache_status_t hal_cache_region_config(hal_cache_region_t region, const hal_cache_region_config_t *region_config)
{
    uint32_t region_start, region_end;

    /* Region is invalid */
    if (region >= HAL_CACHE_REGION_MAX) {
        return HAL_CACHE_STATUS_ERROR_REGION;
    }

    /* Parameter check */
    if (region_config == NULL) {
        return HAL_CACHE_STATUS_INVALID_PARAMETER;
    }

    region_start = region_config->cache_region_address;
    region_end = region_start + region_config->cache_region_size;

    /* The region address must be 4KB aligned */
    if (region_start & (MTK_CACHE_REGION_SIZE_UNIT - 1)) {
        assert(0);
        return HAL_CACHE_STATUS_ERROR_REGION_ADDRESS;
    }

    /* The region end must be 4KB aligned */
    if (region_end & (MTK_CACHE_REGION_SIZE_UNIT - 1)) {
        assert(0);
        return HAL_CACHE_STATUS_ERROR_REGION_SIZE;
    }

    /* Write the region setting to corresponding register */
    CACHE->CACHE_ENTRY_N[region] = region_start;
    CACHE->CACHE_END_ENTRY_N[region] = region_end;

    /* Set this bit when region is configured, and this bit will be double checked in hal_cache_region_enable() function */
    CACHE->CACHE_ENTRY_N[region] |= CACHE_ENTRY_N_C_MASK;

    return HAL_CACHE_STATUS_OK;
}


ATTR_TEXT_IN_TCM hal_cache_status_t hal_cache_invalidate_one_cache_line(uint32_t address)
{
    uint32_t irq_flag;

    /* Make sure address is cache line size aligned */
    if (address & (HAL_CACHE_LINE_SIZE - 1)) {
        return HAL_CACHE_STATUS_INVALID_PARAMETER;
    }
    /* In order to prevent race condition, interrupt should be disabled when query and update global variable which indicates the module status */
    irq_flag = save_and_set_interrupt_mask();

    /* Invalidate CACHE line by address */
    CACHE->CACHE_OP = (address & CACHE_OP_TADDR_MASK);
    CACHE->CACHE_OP |= ((CACHE_INVALIDATE_ONE_LINE_BY_ADDRESS << CACHE_OP_OP_OFFSET) | CACHE_OP_EN_MASK);

    /* flush pipeline */
    __ISB();

    /* Restore the previous status of interrupt */
    restore_interrupt_mask(irq_flag);

    return HAL_CACHE_STATUS_OK;
}


ATTR_TEXT_IN_TCM hal_cache_status_t hal_cache_invalidate_multiple_cache_lines(uint32_t address, uint32_t length)
{
    uint32_t irq_flag;
    uint32_t end_address = address + length;
    uint32_t addr_before_align;

#if 0
    /* Make sure address and length are both cache line size aligned */
    if ((address & (HAL_CACHE_LINE_SIZE - 1)) || (length & (HAL_CACHE_LINE_SIZE - 1))) {
        return HAL_CACHE_STATUS_INVALID_PARAMETER;
    }
#endif /* #if 0 */

    addr_before_align = address;

    //printf("\t[cache] Before alignment: Virtual(%lu), Length(%lu)\r\n", address, length);
    /* Address alignment */
    address &= ~(HAL_CACHE_LINE_SIZE - 1);

    length += (addr_before_align - address);
    /* Length alignment */
    if (length & (HAL_CACHE_LINE_SIZE - 1)) {
        length &= ~(HAL_CACHE_LINE_SIZE - 1);
        length += HAL_CACHE_LINE_SIZE;
    }
    //printf("\t[cache] After alignment: Virtual(%lu), Length(%lu)\r\n", address, length);

    /* In order to prevent race condition, interrupt should be disabled when query and update global variable which indicates the module status */
    irq_flag = save_and_set_interrupt_mask();

    /* Invalidate CACHE lines by address and length */
    while (address < end_address) {
        hal_cache_invalidate_one_cache_line(address);
        address += HAL_CACHE_LINE_SIZE;
    }

    /* flush pipeline*/
    __ISB();

    /* Restore the previous status of interrupt */
    restore_interrupt_mask(irq_flag);

    return HAL_CACHE_STATUS_OK;
}


ATTR_TEXT_IN_TCM hal_cache_status_t hal_cache_invalidate_all_cache_lines(void)
{
    uint32_t irq_flag;

    /* In order to prevent race condition, interrupt should be disabled when query and update global variable which indicates the module status */
    irq_flag = save_and_set_interrupt_mask();

    /* Flush all CACHE lines before invalidate */
    CACHE->CACHE_OP &= ~CACHE_OP_OP_MASK;
    CACHE->CACHE_OP |= ((CACHE_FLUSH_ALL_LINES << CACHE_OP_OP_OFFSET) | CACHE_OP_EN_MASK);

    /* Invalidate all CACHE lines */
    CACHE->CACHE_OP &= ~CACHE_OP_OP_MASK;
    CACHE->CACHE_OP |= ((CACHE_INVALIDATE_ALL_LINES << CACHE_OP_OP_OFFSET) | CACHE_OP_EN_MASK);

    /* flush pipeline*/
    __ISB();

    /* Restore the previous status of interrupt */
    restore_interrupt_mask(irq_flag);

    return HAL_CACHE_STATUS_OK;
}


ATTR_TEXT_IN_TCM hal_cache_status_t hal_cache_flush_one_cache_line(uint32_t address)
{
    uint32_t irq_flag;

    /* Make sure address is cache line size aligned */
    if (address & (HAL_CACHE_LINE_SIZE - 1)) {
        return HAL_CACHE_STATUS_INVALID_PARAMETER;
    }

    /* Interrupt is masked to make sure flush or invalidate operation can not be interrupted */
    irq_flag = save_and_set_interrupt_mask();

    /* Flush CACHE line by address */
    CACHE->CACHE_OP = (address & CACHE_OP_TADDR_MASK);
    CACHE->CACHE_OP |= ((CACHE_FLUSH_ONE_LINE_BY_ADDRESS << CACHE_OP_OP_OFFSET) | CACHE_OP_EN_MASK);

    /* flush pipeline*/
    __ISB();

    /* Restore the previous status of interrupt */
    restore_interrupt_mask(irq_flag);

    return HAL_CACHE_STATUS_OK;
}


ATTR_TEXT_IN_TCM hal_cache_status_t hal_cache_flush_multiple_cache_lines(uint32_t address, uint32_t length)
{
    uint32_t irq_flag;
    uint32_t end_address = address + length;

#if 0
    /* Make sure address and length are both cache line size aligned */
    if ((address & (HAL_CACHE_LINE_SIZE - 1)) || (length & (HAL_CACHE_LINE_SIZE - 1))) {
        return HAL_CACHE_STATUS_INVALID_PARAMETER;
    }
#endif /* #if 0 */
    uint32_t addr_before_align = address;
    //printf("\t[cache] Before alignment: Virtual(%lu), Length(%lu)\r\n", address, length);
    /* Address alignment */
    address &= ~(HAL_CACHE_LINE_SIZE - 1);

    length += (addr_before_align - address);

    /* Length alignment */
    if (length & (HAL_CACHE_LINE_SIZE - 1)) {
        length &= ~(HAL_CACHE_LINE_SIZE - 1);
        length += HAL_CACHE_LINE_SIZE;
    }
    //printf("\t[cache] After alignment: Virtual(%lu), Length(%lu)\r\n", address, length);
    /* Interrupt is masked to make sure flush or invalidate operation can not be interrupted */
    irq_flag = save_and_set_interrupt_mask();

    /* Flush CACHE lines by address and length */
    while (address < end_address) {
        hal_cache_flush_one_cache_line(address);
        address += HAL_CACHE_LINE_SIZE;
    }

    /* flush pipeline*/
    __ISB();

    /* Restore the previous status of interrupt */
    restore_interrupt_mask(irq_flag);

    return HAL_CACHE_STATUS_OK;
}


ATTR_TEXT_IN_TCM hal_cache_status_t hal_cache_flush_all_cache_lines(void)
{
    uint32_t irq_flag;

    /* Interrupt is masked to make sure flush or invalidate operation can not be interrupted */
    irq_flag = save_and_set_interrupt_mask();

    /* Flush all CACHE lines */
    CACHE->CACHE_OP &= ~CACHE_OP_OP_MASK;
    CACHE->CACHE_OP |= ((CACHE_FLUSH_ALL_LINES << CACHE_OP_OP_OFFSET) | CACHE_OP_EN_MASK);

    /* flush pipeline*/
    __ISB();

    /* Restore the previous status of interrupt */
    restore_interrupt_mask(irq_flag);

    return HAL_CACHE_STATUS_OK;
}


ATTR_TEXT_IN_TCM hal_cache_status_t hal_cache_get_hit_count(uint32_t countIdx, uint32_t *hitCnt_h, uint32_t *hitCnt_l, uint32_t *accCnt_h, uint32_t *accCnt_l)
{
    uint32_t irq_flag;

    /* Make sure count index is valid */
    if (((int32_t)countIdx < 0) || (countIdx > 1)) {
        return HAL_CACHE_STATUS_INVALID_PARAMETER;
    }

    /* Check Null Pointer */
    if (!hitCnt_h || !hitCnt_l || !accCnt_h || !accCnt_l) {
        return HAL_CACHE_STATUS_INVALID_PARAMETER;
    }

    /* Interrupt is masked to make sure flush or invalidate operation can not be interrupted */
    irq_flag = save_and_set_interrupt_mask();

    /* Get Cache hit count and access count */
    *hitCnt_h = countIdx ? CACHE->CACHE_HCNT1U : CACHE->CACHE_HCNT0U;
    *hitCnt_l = countIdx ? CACHE->CACHE_HCNT1L : CACHE->CACHE_HCNT0L;
    *accCnt_h = countIdx ? CACHE->CACHE_CCNT1U : CACHE->CACHE_CCNT0U;
    *accCnt_l = countIdx ? CACHE->CACHE_CCNT1L : CACHE->CACHE_CCNT0L;

    /* flush pipeline*/
    //__ISB();

    /* Restore the previous status of interrupt */
    restore_interrupt_mask(irq_flag);

    return HAL_CACHE_STATUS_OK;
}


uint32_t hal_cache_get_region_en(void)
{
    return CACHE->CACHE_REGION_EN;
}


bool hal_cache_is_cacheable(uint32_t address)
{
    hal_cache_region_t reg;

    /* CACHE is disabled, all memorys are non-cacheable */
    if (!(CACHE->CACHE_CON & CACHE_CON_MCEN_MASK)) {
        return false;
    }

    /* check address is in any enabled region or not */
    for (reg = HAL_CACHE_REGION_0; reg < HAL_CACHE_REGION_MAX; reg ++) {
        /* Only compare with region that is enabled */
        if (CACHE->CACHE_REGION_EN & (1 << reg)) {
            if (IN_REG(address, reg)) {
                return true;
            }
        }
    }
    return false;
}


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifdef HAL_CACHE_MODULE_ENABLED */

