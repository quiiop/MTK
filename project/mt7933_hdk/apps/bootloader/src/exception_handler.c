/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include <stdint.h>
#include "hw_uart.h"
#include "mt7933.h"

#define EXCEPTION_STACK_WORDS   64
static unsigned int ExceptionStack[EXCEPTION_STACK_WORDS] = {0};
unsigned int *pxExceptionStack = &ExceptionStack[EXCEPTION_STACK_WORDS - 1];
unsigned int *pxExceptionStackLimit = &ExceptionStack[0];

typedef enum {
    NMI_FAULT = 2,
    HARD_FAULT,
    MPU_FAULT,
    BUS_FAULT,
    USAGE_FAULT,
    SECURE_FAULT
} fault_type_t;

typedef struct fault_stack_frame
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} fault_stack_frame_t;

static void get_ufsr_info(uint32_t cfsr)
{
    if (cfsr & SCB_CFSR_UNDEFINSTR_Msk)
        hw_uart_puts("Undefined instruction\n");
    if (cfsr & SCB_CFSR_INVSTATE_Msk)
        hw_uart_puts("Invalid state\n");
    if (cfsr & SCB_CFSR_INVPC_Msk)
        hw_uart_puts("Invalid PC\n");
    if (cfsr & SCB_CFSR_NOCP_Msk)
        hw_uart_puts("No coprocessor\n");
    if (cfsr & SCB_CFSR_STKOF_Msk)
        hw_uart_puts("Stack overflow\n");
    if (cfsr & SCB_CFSR_UNALIGNED_Msk)
        hw_uart_puts("Unaligned access\n");
    if (cfsr & SCB_CFSR_DIVBYZERO_Msk)
        hw_uart_puts("Divide by zero\n");
}

static void get_bfsr_info(uint32_t cfsr)
{
    if (cfsr & SCB_CFSR_IBUSERR_Msk)
        hw_uart_puts("Instruction bus error\n");
    if (cfsr & SCB_CFSR_PRECISERR_Msk)
        hw_uart_puts("Precise data access error\n");
    if (cfsr & SCB_CFSR_IMPRECISERR_Msk)
        hw_uart_puts("Imprecise data access error\n");
    if (cfsr & SCB_CFSR_UNSTKERR_Msk)
        hw_uart_puts("Unstacking error\n");
    if (cfsr & SCB_CFSR_STKERR_Msk)
        hw_uart_puts("Stacking error\n");
    if (cfsr & SCB_CFSR_LSPERR_Msk)
        hw_uart_puts("FP lazy error\n");
    if (cfsr & SCB_CFSR_BFARVALID_Msk)
        hw_uart_puts("BFAR valid, see BFAR\n");
}

static void get_mmfsr_info(uint32_t cfsr)
{
    if (cfsr & SCB_CFSR_IACCVIOL_Msk)
        hw_uart_puts("Instruction access violation\n");
    if (cfsr & SCB_CFSR_DACCVIOL_Msk)
        hw_uart_puts("Data access violation\n");
    if (cfsr & SCB_CFSR_MUNSTKERR_Msk)
        hw_uart_puts("Unstacking error\n");
    if (cfsr & SCB_CFSR_MSTKERR_Msk)
        hw_uart_puts("Stacking error\n");
    if (cfsr & SCB_CFSR_MLSPERR_Msk)
        hw_uart_puts("FP lazy error\n");
    if (cfsr & SCB_CFSR_MMARVALID_Msk)
        hw_uart_puts("MMFAR valid, see MMFAR\n");
}

void CommonFault_Handler(fault_stack_frame_t *stack_frame, uint32_t exc_return)
{
    uint32_t cfsr, bus_fault_addr, mem_fault_addr;
    uint32_t exception_num = 0;

    bus_fault_addr = SCB->BFAR;
    mem_fault_addr = SCB->MMFAR;
    cfsr = SCB->CFSR;

    hw_uart_printf("Oops... PSR: 0x%x\n", __get_xPSR());
    exception_num = __get_xPSR()& 0x1FF;

    switch (exception_num) {
    case NMI_FAULT:
        hw_uart_puts("NMI FAULT:\n");
        break;
    case HARD_FAULT:
        hw_uart_puts("HARD FAULT:\n");
        break;
    case MPU_FAULT:
        hw_uart_puts("MPU FAULT:\n");
        get_mmfsr_info(cfsr);
        if (cfsr & SCB_CFSR_MMARVALID_Msk)
            hw_uart_printf("  MMFAR = 0x%x\n", mem_fault_addr);
        break;
    case BUS_FAULT:
        hw_uart_puts("BUS FAULT:\n");
        get_bfsr_info(cfsr);
        if (cfsr & SCB_CFSR_BFARVALID_Msk)
            hw_uart_printf("  BFAR  = 0x%x\n", bus_fault_addr);
        break;
    case USAGE_FAULT:
        hw_uart_puts("USAGE FAULT:\n");
        get_ufsr_info(cfsr);
        break;
    case SECURE_FAULT:
        hw_uart_puts("SECURE FAULT:\n");
        hw_uart_printf("  SFSR  = 0x%x\n", SAU->SFSR);
        if (SAU->SFSR & SAU_SFSR_SFARVALID_Msk)
            hw_uart_printf("  SFAR  = 0x%x\n", SAU->SFAR);
        break;
    default:
        hw_uart_puts("UNKNOWN FAULT:\n");
        break;
    }
    hw_uart_printf("  PC    = 0x%x\n", stack_frame->pc);
    hw_uart_printf("  LR    = 0x%x\n", stack_frame->lr);
    hw_uart_printf("  R0    = 0x%x\n", stack_frame->r0);
    hw_uart_printf("  R1    = 0x%x\n", stack_frame->r1);
    hw_uart_printf("  R2    = 0x%x\n", stack_frame->r2);
    hw_uart_printf("  R3    = 0x%x\n", stack_frame->r3);
    hw_uart_printf("  R12   = 0x%x\n", stack_frame->r12);
    hw_uart_printf("  PSR   = 0x%x\n", stack_frame->psr);
    hw_uart_printf("  CFSR  = 0x%x\n", cfsr);
    hw_uart_printf("  HFSR  = 0x%x\n", SCB->HFSR);
    hw_uart_printf("  DFSR  = 0x%x\n", SCB->DFSR);
    hw_uart_printf("  AFSR  = 0x%x\n", SCB->AFSR);
    hw_uart_printf("  EXC_RETURN = 0x%x\n", exc_return);
    while (1);
}
