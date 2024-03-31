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

/**
 * Include stdlib to check __NEWLIB_H__ existence
 **/
#include <stdlib.h>
#if defined(__NEWLIB_H__) && !defined(__DYNAMIC_REENT__)
#if __NEWLIB__ < 3
#error "Newlib's reentrant functions are verified from version 3 only!"
#endif /* #if __NEWLIB__ < 3 */
#endif /* #if defined(__NEWLIB_H__) && !defined(__DYNAMIC_REENT__) */

#include "hal_nvic.h"

#ifdef HAL_NVIC_MODULE_ENABLED
#include "hal_nvic_internal.h"
#include "memory_attribute.h"
#include "hal_flash.h"
#include "hal_log.h"
#ifdef MTK_SWLA_ENABLE
#include "swla.h"
#endif /* #ifdef MTK_SWLA_ENABLE */
#include <assert.h>
#ifdef HAL_NVIC_IRQ_SET_TYPE_FEATURE
#include "reg_base.h"
#include "common.h"
#endif /* #ifdef HAL_NVIC_IRQ_SET_TYPE_FEATURE */

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

#ifdef HAL_NVIC_IRQ_SET_TYPE_FEATURE
#define IRQ_SENS0_BASE  (IRQ_CFG_BASE + 0x200)
#endif /* #ifdef HAL_NVIC_IRQ_SET_TYPE_FEATURE */

#define _IS_ALIGNED(PTR, BYTE) \
    ((((uint32_t)(PTR)) & ((BYTE) - 1U)) == 0)

typedef struct {
    void (*nvic_callback)(hal_nvic_irq_t irq_number);
    uint32_t irq_pending;
} nvic_function_t;

static const uint8_t default_irq_priority[IRQ_NUMBER_MAX] = {
    WIC_INT_IRQ_PRIORITY,                   /* 0, WIC_INT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 1, RESERVED1_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 2, RESERVED2_IRQn */
    WDT_IRQ_PRIORITY,                       /* 3, WDT_B0_IRQn */
    UART_IRQ_PRIORITY,                      /* 4, UART_IRQn */
    INFRA_BUS_IRQ_PRIORITY,                 /* 5, INFRA_BUS_IRQn */
    CDBGPWRUPREQ_IRQ_PRIORITY,              /* 6, CDBGPWRUPREG_IRQn */
    CDBGPWRUPACK_IRQ_PRIORITY,              /* 7, CDBGPWRUPACK_IRQn */
    WDT_IRQ_PRIORITY,                       /* 8, WDT_B1_IRQn */
    DSP_UART_IRQ_PRIORITY,                  /* 9, DSP_TO_CM33_IRQn */
    GPT_IRQ_PRIORITY,                       /* 10, APXGPT0_IRQn */
    GPT_IRQ_PRIORITY,                       /* 11, APXGPT1_IRQn */
    GPT_IRQ_PRIORITY,                       /* 12, APXGPT2_IRQn */
    GPT3_IRQ_PRIORITY,                      /* 13, APXGPT3_IRQn */
    GPT_IRQ_PRIORITY,                       /* 14, APXGPT4_IRQn */
    GPT_IRQ_PRIORITY,                       /* 15, APXGPT5_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 16, DEVAC_INFRA_AON_SECURE_VIO_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 17, DEVAPC_AUD_BUS_SECURE_VIO_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 18, CONN_AP_BUS_REQ_RISE_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 19, CONN_AP_BUS_REQ_FULL_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 20, CONN_APSRC_REQ_RISE_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 21, CONN_APSRC_REQ_FALL_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 22, CONN_AP_BUS_REQ_HIGH_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 23, CONN_AP_BUS_REQ_LOW_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 24, CONN_APSRC_REQ_HIGH_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 25, CONN_APSRC_REQ_LOW_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 26, INFRA_BUS_TIMEOUT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 27, CM33_LOCAL_BUS_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 28, ADSP_INFRA_BUS_TIMEOUT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 29, RESERVED29_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 30, RESERVED30_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 31, RESERVED31_IRQn */
    DSP_UART_IRQ_PRIORITY,                  /* 32, DSP_UART_IRQn */
    TOP_UART0_IRQ_PRIORITY,                 /* 33, TOP_UART0_IRQn */
    TOP_UART1_IRQ_PRIORITY,                 /* 34, TOP_UART1_IRQn */
    I2C0_IRQ_PRIORITY,                      /* 35, I2C0_IRQn */
    I2C1_IRQ_PRIORITY,                      /* 36, I2C1_IRQn */
    SDCTL_TOP_FW_IRQ_PRIORITY,              /* 37, SDCTL_TOP_FW_IRQn */
    SDCTL_TOP_FW_QOUT_IRQ_PRIORITY,         /* 38, SDCTL_TOP_FW_QOUT_IRQn */
    SPIM0_IRQ_PRIORITY,                     /* 39, SPIM0_IRQn */
    SPIM1_IRQ_PRIORITY,                     /* 40, SPIM1_IRQn */
    SPIS_IRQ_PRIORITY,                      /* 41, SPIS_IRQn */
    KP_IRQ_PRIORITY,                        /* 42, KP_IRQn */
    IRRX_IRQ_PRIORITY,                      /* 43, IRRX_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 44, RESERVED44_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 45, RESERVED45_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 46, RESERVED46_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 47, RESERVED47_IRQn */
    SSUSB_XHCI_IRQ_PRIORITY,                /* 48, SSUSB_XHCI_IRQn */
    SSUSB_OTG_IRQ_PRIORITY,                 /* 49, SSUSB_OTG_IRQn */
    SSUSB_DEV_IRQ_PRIORITY,                 /* 50, SSUSB_DEV_IRQn */
    AFE_MCU_IRQ_PRIORITY,                   /* 51, AFE_MCU_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 52, RTC_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 53, SYSRAM_TOP_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 54, MPU_L2_PWR_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 55, MPU_PSRAM_PWR_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 56, CQDMA0_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 57, CQDMA1_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 58, CQDMA2_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 59, MSDC_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 60, MSDC_WAKEUP_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 61, DSP_WDT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 62, DSP_TO_CPU_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 63, APDMA0_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 64, APDMA1_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 65, APDMA2_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 66, APDMA3_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 67, APDMA4_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 68, APDMA5_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 69, APDMA6_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 70, APDMA7_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 71, APDMA8_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 72, APDMA9_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 73, APDMA10_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 74, APDMA11_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 75, BTIF_HOST_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 76, SF_TOP_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 77, CONN2AP_WFDMA_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 78, BGF2AP_WDT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 79, BGF2AP_BTIF0_WAKEUP_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 80, CONN2AP_SW_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 81, BT2AP_ISOCH_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 82, BT_CVSD_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 83, CCIF_WF2AP_SW_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 84, CCIF_BGF2AP_SW_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 85, WM_CONN2AP_WDT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 86, SEMA_RELEASE_INFORM_M2_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 87, SEMA_RELEASE_INFORM_M3_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 88, SEMA_M2_TIMEOUT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 89, SEMA_M3_TIMEOUT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 90, CONN_BGF_HIF_ON_HOST_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 91, CONN_GPS_HIF_ON_HOST_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 92, SSUSB_SPM_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 93, WF2AP_SW_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 94, CQDMA_SEC_ABORT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 95, APDMA_SEC_ABORT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 96, SDIO_CMD_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 97, RESERVED97_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 98, ADC_COMP_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 99, ADC_FIFO_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 100, GCPU_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 101, ECC_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 102, TRNG_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 103, SEJ_APXGPT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 104, SEJ_WDT_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 105, RESERVED105_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 106, RESERVED106_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 107, RESERVED107_IRQn */
    RESERVED_IRQ_PRIORITY,                  /* 108, GPIO_IRQ0n */
    RESERVED_IRQ_PRIORITY,                  /* 109, GPIO_IRQ1n */
    RESERVED_IRQ_PRIORITY,                  /* 110, GPIO_IRQ2n */
    RESERVED_IRQ_PRIORITY,                  /* 111, GPIO_IRQ3n */
    RESERVED_IRQ_PRIORITY,                  /* 112, GPIO_IRQ4n */
    RESERVED_IRQ_PRIORITY,                  /* 113, GPIO_IRQ5n */
    RESERVED_IRQ_PRIORITY,                  /* 114, GPIO_IRQ6n */
    RESERVED_IRQ_PRIORITY,                  /* 115, GPIO_IRQ7n */
    RESERVED_IRQ_PRIORITY,                  /* 116, GPIO_IRQ8n */
    RESERVED_IRQ_PRIORITY,                  /* 117, GPIO_IRQ9n */
    RESERVED_IRQ_PRIORITY,                  /* 118, GPIO_IRQ10n */
    RESERVED_IRQ_PRIORITY,                  /* 119, GPIO_IRQ11n */
    RESERVED_IRQ_PRIORITY,                  /* 120, GPIO_IRQ12n */
    RESERVED_IRQ_PRIORITY,                  /* 121, GPIO_IRQ13n */
    RESERVED_IRQ_PRIORITY,                  /* 122, GPIO_IRQ14n */
    RESERVED_IRQ_PRIORITY,                  /* 123, GPIO_IRQ15n */
    RESERVED_IRQ_PRIORITY,                  /* 124, GPIO_IRQ16n */
    RESERVED_IRQ_PRIORITY,                  /* 125, GPIO_IRQ17n */
    RESERVED_IRQ_PRIORITY,                  /* 126, GPIO_IRQ18n */
    RESERVED_IRQ_PRIORITY,                  /* 127, GPIO_IRQ19n */
    RESERVED_IRQ_PRIORITY,                  /* 128, GPIO_IRQ20n */
    RESERVED_IRQ_PRIORITY,                  /* 129, GPIO_IRQ21n */
    RESERVED_IRQ_PRIORITY,                  /* 130, GPIO_IRQ22n */
    RESERVED_IRQ_PRIORITY,                  /* 131, GPIO_IRQ23n */
    RESERVED_IRQ_PRIORITY,                  /* 132, GPIO_IRQ24n */
    RESERVED_IRQ_PRIORITY,                  /* 133, GPIO_IRQ25n */
    RESERVED_IRQ_PRIORITY,                  /* 134, GPIO_IRQ26n */
    RESERVED_IRQ_PRIORITY,                  /* 135, GPIO_IRQ27n */
    RESERVED_IRQ_PRIORITY,                  /* 136, GPIO_IRQ28n */
    RESERVED_IRQ_PRIORITY,                  /* 137, GPIO_IRQ29n */
    RESERVED_IRQ_PRIORITY,                  /* 138, GPIO_IRQ30n */
    UART_IRQ_PRIORITY,                      /* 139, CM33_UART_RX_IRQn */
};


nvic_function_t nvic_function_table[IRQ_NUMBER_MAX];


static uint32_t get_pending_irq(void)
{
    return ((SCB->ICSR & SCB_ICSR_ISRPENDING_Msk) >> SCB_ICSR_ISRPENDING_Pos);
}

hal_nvic_status_t hal_nvic_init(void)
{
    static uint32_t priority_set = 0;
    uint32_t i;

    /* Check vector table alignment
     * Currenlty cm33 have 139 irq, the table size is
     * 139 + 16 (preserve by fault) = 155 words, according armv8m doc
     * the alignment must be the power of 2, therefore the next power of 2
     * is 256 words, 256 words = 1024 bytes
     * */
    ASSERT(_IS_ALIGNED((void *)SCB->VTOR, 1024U));

    if (priority_set == 0) {
        /* Init Default ISR Handler */
        /* Set defualt priority only one time */
        for (i = 0; i < IRQ_NUMBER_MAX; i++) {
            NVIC_SetPriority((hal_nvic_irq_t)i, default_irq_priority[i]);
        }
        priority_set = 1;
    }
    return HAL_NVIC_STATUS_OK;
}

hal_nvic_status_t hal_nvic_enable_irq(hal_nvic_irq_t irq_number)
{
    hal_nvic_status_t status;

    if (irq_number < (hal_nvic_irq_t)0 || irq_number >= IRQ_NUMBER_MAX) {
        status = HAL_NVIC_STATUS_ERROR_IRQ_NUMBER;
    } else {
        NVIC_EnableIRQ(irq_number);
        status = HAL_NVIC_STATUS_OK;
    }

    return status;
}

hal_nvic_status_t hal_nvic_disable_irq(hal_nvic_irq_t irq_number)
{
    hal_nvic_status_t status;

    if (irq_number < (hal_nvic_irq_t)0 || irq_number >= IRQ_NUMBER_MAX) {
        status = HAL_NVIC_STATUS_ERROR_IRQ_NUMBER;
    } else {
        NVIC_DisableIRQ(irq_number);
        status = HAL_NVIC_STATUS_OK;
    }

    return status;
}

uint32_t hal_nvic_get_pending_irq(hal_nvic_irq_t irq_number)
{
    uint32_t ret = 0xFF;

    if (irq_number < (hal_nvic_irq_t)0 || irq_number >= IRQ_NUMBER_MAX) {
        return ret;
    } else {
        ret = NVIC_GetPendingIRQ(irq_number);
    }

    return ret;
}

hal_nvic_status_t hal_nvic_set_pending_irq(hal_nvic_irq_t irq_number)
{
    hal_nvic_status_t status;

    if (irq_number < (hal_nvic_irq_t)0 || irq_number >= IRQ_NUMBER_MAX) {
        status = HAL_NVIC_STATUS_ERROR_IRQ_NUMBER;
    } else {
        NVIC_SetPendingIRQ(irq_number);
        status = HAL_NVIC_STATUS_OK;
    }

    return status;
}

hal_nvic_status_t hal_nvic_clear_pending_irq(hal_nvic_irq_t irq_number)
{
    hal_nvic_status_t status;

    if (irq_number < (hal_nvic_irq_t)0 || irq_number >= IRQ_NUMBER_MAX) {
        status = HAL_NVIC_STATUS_ERROR_IRQ_NUMBER;
    } else {
        NVIC_ClearPendingIRQ(irq_number);
        status = HAL_NVIC_STATUS_OK;
    }

    return status;
}

hal_nvic_status_t hal_nvic_set_priority(hal_nvic_irq_t irq_number, uint32_t priority)
{
    hal_nvic_status_t status;

    if (priority > RESERVED_IRQ_PRIORITY) {
        return HAL_NVIC_STATUS_INVALID_PARAMETER;
    }

    if (irq_number < (hal_nvic_irq_t)0 || irq_number >= IRQ_NUMBER_MAX) {
        status = HAL_NVIC_STATUS_ERROR_IRQ_NUMBER;
        return status;
    } else {
        NVIC_SetPriority(irq_number, priority);
        status = HAL_NVIC_STATUS_OK;
    }

    return status;
}

uint32_t hal_nvic_get_priority(hal_nvic_irq_t irq_number)
{
    uint32_t ret = RESERVED_IRQ_PRIORITY;

    if (irq_number < (hal_nvic_irq_t)0 || irq_number >= IRQ_NUMBER_MAX) {
        return ret;
    } else {
        ret = NVIC_GetPriority(irq_number);
    }

    return ret;
}

static uint32_t get_current_irq(void)
{
    uint32_t irq_num = ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos);
    return (irq_num - 16);
}


ATTR_TEXT_IN_TCM hal_nvic_status_t isrC_main(void)
{
    hal_nvic_status_t status;
    hal_nvic_irq_t irq_number;

#if defined(__NEWLIB_H__) && !defined(__DYNAMIC_REENT__)
    struct  _reent *backup_reent = _impure_ptr;
    _impure_ptr = _global_impure_ptr;
#endif /* #if defined(__NEWLIB_H__) && !defined(__DYNAMIC_REENT__) */

    Flash_ReturnReady();
    irq_number = (hal_nvic_irq_t)(get_current_irq());
#ifdef MTK_SWLA_ENABLE
    SLA_RamLogging((uint32_t)(IRQ_START | irq_number));
#endif /* #ifdef MTK_SWLA_ENABLE */
    if (irq_number < (hal_nvic_irq_t)0 || irq_number >= IRQ_NUMBER_MAX) {
        status = HAL_NVIC_STATUS_ERROR_IRQ_NUMBER;
    } else if (nvic_function_table[irq_number].nvic_callback == NULL) {
        status = HAL_NVIC_STATUS_ERROR_NO_ISR;
#ifdef READY
        log_hal_error("ERROR: no IRQ handler! \n");
#endif /* #ifdef READY */
    } else {
        nvic_function_table[irq_number].irq_pending = get_pending_irq();
        nvic_function_table[irq_number].nvic_callback(irq_number);
        status = HAL_NVIC_STATUS_OK;
    }

#ifdef MTK_SWLA_ENABLE
    SLA_RamLogging((uint32_t)IRQ_END);
#endif /* #ifdef MTK_SWLA_ENABLE */

#if defined(__NEWLIB_H__) && !defined(__DYNAMIC_REENT__)
    _impure_ptr = backup_reent;
#endif /* #if defined(__NEWLIB_H__) && !defined(__DYNAMIC_REENT__) */

    return status;
}

hal_nvic_status_t hal_nvic_register_isr_handler(hal_nvic_irq_t irq_number, hal_nvic_isr_t callback)
{
    uint32_t mask;

    if (irq_number < (hal_nvic_irq_t)0 || irq_number >= IRQ_NUMBER_MAX || callback == NULL) {
        return HAL_NVIC_STATUS_INVALID_PARAMETER;
    }

    mask = save_and_set_interrupt_mask();
    NVIC_ClearPendingIRQ(irq_number);
    nvic_function_table[irq_number].nvic_callback = callback;
    nvic_function_table[irq_number].irq_pending = 0;
    restore_interrupt_mask(mask);

    return HAL_NVIC_STATUS_OK;
}

ATTR_TEXT_IN_TCM hal_nvic_status_t hal_nvic_save_and_set_interrupt_mask(uint32_t *mask)
{
    *mask = save_and_set_interrupt_mask();
    return HAL_NVIC_STATUS_OK;
}

ATTR_TEXT_IN_TCM hal_nvic_status_t hal_nvic_restore_interrupt_mask(uint32_t mask)
{
    restore_interrupt_mask(mask);
    return HAL_NVIC_STATUS_OK;
}

#ifdef HAL_NVIC_IRQ_SET_TYPE_FEATURE
hal_nvic_status_t hal_nvic_irq_set_type(hal_nvic_irq_t irq_number, hal_nvic_irq_type_t type)
{
    int index = irq_number % 32 ;

    switch (type) {
        case HAL_NVIC_IRQ_TYPE_LEVEL_HIGH:
            DRV_SetReg32(IRQ_SENS0_BASE + irq_number / 32 * 4, 1 << index);
            return HAL_NVIC_STATUS_OK;
        case HAL_NVIC_IRQ_TYPE_EDGE_RISING:
            DRV_ClrReg32(IRQ_SENS0_BASE + irq_number / 32 * 4, 1 << index);
            return HAL_NVIC_STATUS_OK;
        default:
            return HAL_NVIC_STATUS_INVALID_PARAMETER;
    }
}
#endif /* #ifdef HAL_NVIC_IRQ_SET_TYPE_FEATURE */

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifdef HAL_NVIC_MODULE_ENABLED */

