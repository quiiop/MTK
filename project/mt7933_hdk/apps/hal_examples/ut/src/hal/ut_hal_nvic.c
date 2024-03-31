/* Copyright Statement:
 *
 * (C) 2005-2030  MediaTek Inc. All rights reserved.
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ut.h"

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_NVIC_MODULE_ENABLE)
#include "FreeRTOS.h"
#include "task.h"
#include "hal_nvic_internal.h"

#define UT_ISR_IRQn       RESERVED1_IRQn
#define UT_ISR_IRQn_PRI   10
#define UT_ISR_COUNT      10

static int isr_cnt = 0;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void ut_hal_nvic_isr_handler(void)
{
    isr_cnt++;
    printf("%02d. ", isr_cnt);
}

ut_status_t ut_hal_nvic(void)
{
    int idx = 0;
    uint32_t irq_pri, irq_mask, irq_pending;
    hal_nvic_status_t ret_status;
    isr_cnt = 0;

    // Set IRQ Priority
    ret_status = hal_nvic_set_priority(UT_ISR_IRQn, UT_ISR_IRQn_PRI);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        printf("hal_nvic set priority fail, ret = %d, pri = %d \r\n", ret_status, UT_ISR_IRQn_PRI);

        return UT_STATUS_ERROR;
    }

    irq_pri = hal_nvic_get_priority(UT_ISR_IRQn);
    printf("hal_nvic get priority, pri = %ld , __NVIC_PRIO_BITS = %d \r\n", irq_pri, __NVIC_PRIO_BITS);

    // Register ISR Handler for IRQn
    ret_status = hal_nvic_register_isr_handler(UT_ISR_IRQn, (hal_nvic_isr_t)ut_hal_nvic_isr_handler);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        printf("hal_nvic register_isr_handler fail, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }

    // Enable IRQn Priority
    ret_status = hal_nvic_enable_irq(UT_ISR_IRQn);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        printf("hal_nvic enable_irq fail, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }

    // UT - Software Trigger IRQn
    printf("\r\n<<ISR>>\r\n");
    for (idx = 0; idx < UT_ISR_COUNT; idx++) {
        if (HAL_NVIC_STATUS_OK != nvic_irq_software_trigger(UT_ISR_IRQn)) {
            printf("hal_nvic software trigger fail. \r\n");

            return UT_STATUS_ERROR;
        }

        vTaskDelay(100);
    }
    printf("\r\nISR Count (%d)...", isr_cnt);

    // Disable IRQn
    ret_status = hal_nvic_disable_irq(UT_ISR_IRQn);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        printf("hal_nvic disable_irq fail, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }

    if (isr_cnt == UT_ISR_COUNT) {  // Pass
        printf("Pass\r\n\r\n");
    } else { // Fail
        printf("Fail\r\n\r\n");
        return UT_STATUS_ERROR;
    }

    // save and restore irq mask
    ret_status = hal_nvic_save_and_set_interrupt_mask(&irq_mask);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        printf("hal_nvic save_and_set_interrupt_mask fail, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }

    printf("hal_nvic irq_mask = 0x%lX \r\n", irq_mask);
    ret_status = hal_nvic_restore_interrupt_mask(irq_mask);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        printf("hal_nvic restore_interrupt_mask fail, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }

    // pending irq
    ret_status = hal_nvic_set_pending_irq(GPIO_IRQ0n);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        printf("hal_nvic set_pending_irq fail, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }

    irq_pending = hal_nvic_get_pending_irq(GPIO_IRQ0n);
    printf("hal_nvic get pending irq = 0x%lX, irq = %d \r\n", irq_pending, GPIO_IRQ0n);

    ret_status = hal_nvic_clear_pending_irq(GPIO_IRQ0n);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        printf("hal_nvic clear_pending_irq fail, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }

#ifdef HAL_NVIC_IRQ_SET_TYPE_FEATURE
    ret_status = hal_nvic_irq_set_type(RESERVED1_IRQn, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    if (HAL_NVIC_STATUS_OK != ret_status) {
        printf("hal_nvic irq_set_type fail, ret = %d \r\n", ret_status);

        return UT_STATUS_ERROR;
    }
#endif /* #ifdef HAL_NVIC_IRQ_SET_TYPE_FEATURE */

    printf("\r\nhal_nvic UT OK\r\n\r\n");

    return UT_STATUS_OK;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_NVIC_MODULE_ENABLE) */
