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
 * MediaTek Inc. (C) 2019. All rights reserved.
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

#include <adsp_excep.h>
#ifdef CFG_IPC_SUPPORT
#include <adsp_ipi.h>
#endif
#ifdef CFG_TRAX_SUPPORT
#include <adsp_trax.h>
#endif
#include <driver_api.h>
#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <mt_reg_base.h>
#include <stdio.h>
#include <task.h>
#include <unwind.h>
#ifdef CFG_WDT_SUPPORT
#include <wdt.h>
#endif
#include <xtensa_api.h>
#include <xtensa/corebits.h>
#include <xtensa/tie/xt_debug.h>

#ifdef CFG_HW_BREAKPOINT_SUPPORT
#include <hw_breakpoint.h>
#endif

#ifdef CFG_TRAX_SUPPORT
extern  trax_context context;
#endif

extern unsigned char _memmap_mem_sram_start;
extern unsigned char _memmap_mem_sram_end;
extern unsigned char _memmap_mem_psram_start;
extern unsigned char _memmap_mem_psram_end;
#define ALIGNED_4(x)    ((x + 3) / 4 * 4)

struct adsp_excep_desc
{
    int id;
    const char *desc;
};

static const struct adsp_excep_desc excep_desc[] =
{
    /* defined in xtensa/corebits.h */
    {EXCCAUSE_ILLEGAL,                  "Illegal Instruction "},
    {EXCCAUSE_INSTR_ERROR,              "Instruction Fetch Error"},
    {EXCCAUSE_LOAD_STORE_ERROR,         "Load Store Error"},
    {EXCCAUSE_DIVIDE_BY_ZERO,           "Integer Divide by Zero"},
    {EXCCAUSE_PRIVILEGED,               "Privileged Instruction"},
    {EXCCAUSE_UNALIGNED,                "Unaligned Load or Store"},
    {EXCCAUSE_EXCLUSIVE_ERROR,          "Load exclusive to unsupported memory type or unaligned address"},
    {EXCCAUSE_INSTR_DATA_ERROR,         "PIF Data Error on Instruction Fetch"},
    {EXCCAUSE_LOAD_STORE_DATA_ERROR,    "PIF Data Error on Load or Store"},
    {EXCCAUSE_INSTR_ADDR_ERROR,         "PIF Address Error on Instruction Fetch"},
    {EXCCAUSE_LOAD_STORE_ADDR_ERROR,    "PIF Address Error on Load or Store"},
    {EXCCAUSE_ITLB_MISS,                "ITLB Miss"},
    {EXCCAUSE_ITLB_MULTIHIT,            "ITLB Multihit"},
    {EXCCAUSE_INSTR_RING,               "Ring Privilege Violation on Instruction Fetch"},
    {EXCCAUSE_INSTR_PROHIBITED,         "Cache Attribute does not allow Instruction Fetch"},
    {EXCCAUSE_DTLB_MISS,                "DTLB Miss"},
    {EXCCAUSE_DTLB_MULTIHIT,            "DTLB Multihit"},
    {EXCCAUSE_LOAD_STORE_RING,          "Ring Privilege Violation on Load or Store"},
    {EXCCAUSE_LOAD_PROHIBITED,          "Cache Attribute does not allow Load"},
    {EXCCAUSE_STORE_PROHIBITED,         "Cache Attribute does not allow Store"},
};

static unsigned int last_pc = 0x0;
static unsigned int current_pc = 0x0;

static _Unwind_Reason_Code trace_func(struct _Unwind_Context *ctx, void *d)
{
    current_pc = _Unwind_GetIP(ctx);
    if (current_pc && (current_pc != last_pc))
    {
        /* address is return address (LR) */
        PRINTF_E("  0x%08x\n", current_pc);
        last_pc = current_pc;
    }
    return _URC_NO_REASON;
}

void print_backtrace(void)
{
    int depth = 0;

    PRINTF_E("Backtrace:\n");
    _Unwind_Backtrace(&trace_func, &depth);
}

static int check_valid_pc(uint32_t pc)
{
    if (pc < CFG_HIFI4_SRAM_ADDRESS || pc >= (CFG_HIFI4_DRAM_ADDRESS + CFG_HIFI4_DRAM_SIZE))
        return -1;
    if (pc >= (CFG_HIFI4_SRAM_ADDRESS + CFG_HIFI4_SRAM_SIZE) && pc < CFG_HIFI4_DRAM_ADDRESS)
        return -1;

    return 0;
}

void print_excep_backtrace(XtExcFrame *frame)
{
    uint32_t i = 0, pc = frame->pc, lr = frame->a0, sp = frame->a1;
    uint32_t pc_bit_31_30 = pc & 0xC0000000;

    PRINTF_E("Backtrace:\n");
    PRINTF_E("  0x%08x\n", pc);
    pc = (lr & (~0xC0000000)) | pc_bit_31_30;
    while (i++ < 100) {
        if (check_valid_pc(pc))
            return;
        /* stack frame addresses are return addresses, so subtract 3 to get the CALL address */
        PRINTF_E("  0x%08x\n", pc - 3);
        pc = (*((uint32_t *) (sp - 0x10)) & (~0xC0000000)) | pc_bit_31_30;
        sp = *((uint32_t *) (sp - 0x10 + 4));
    }
}

const char* get_excep_desc(int excep_id)
{
    int i;
    int cnt = sizeof(excep_desc) / sizeof(struct adsp_excep_desc);

    for (i = 0; i < cnt; i++)
        if (excep_desc[i].id == excep_id)
            return excep_desc[i].desc;

    return NULL;
}

void dump_mem_around_register(char *reg_name, uint32_t reg_value)
{
    int i;
    int dump_size = 64;
    uint32_t aligned_addr, dump_start, dump_end;
    uint32_t sram_start = (uint32_t)&_memmap_mem_sram_start;
    uint32_t sram_end   = (uint32_t)&_memmap_mem_sram_end;
    uint32_t dram_start = (uint32_t)&_memmap_mem_psram_start;
    uint32_t dram_end   = (uint32_t)&_memmap_mem_psram_end;

    if (reg_value < sram_start ||
        (reg_value >= sram_end && reg_value < dram_start) ||
        reg_value >= dram_end)
        return;

    aligned_addr = ALIGNED_4(reg_value);
    dump_start = aligned_addr - 16 * 4;
    if (dump_start < sram_start)
        dump_start = sram_start;
    else if (dump_start >= sram_end && dump_start < dram_start)
        dump_start = dram_start;

    dump_end = dump_start + dump_size * 4;
    if (dump_end > sram_end && dump_end < dram_start)
        dump_end = sram_end;
    else if (dump_end > dram_end)
        dump_end = dram_end;

    dump_size = (dump_end - dump_start) / 4;

    PRINTF_E("%s: 0x%08x\n", reg_name, reg_value);
    for (i = 0; i < dump_size; i++) {
        if (i % 8 == 0) {
            if (i == 0) {
                PRINTF_E(" ");
            } else {
                FreeRTOS_CLIPrintf("\r\n");
                PRINTF_E(" ");
            }
            FreeRTOS_CLIPrintf("%08x:", dump_start + i * 4);
        }
        FreeRTOS_CLIPrintf(" %08x", *((uint32_t *)(dump_start + i * 4)));
    }
    FreeRTOS_CLIPrintf("\r\n\r\n");
}

int g_double_exception_flag = 0;
void* adsp_excep_handler(XtExcFrame *frame)
{
    const char *cause = NULL;
    uint32_t epc = XT_RSR_EPC1();
    uint32_t depc = XT_RSR_DEPC();

    /* Prohibit task scheduling */
    taskENTER_CRITICAL();

    PRINTF_E("Oops...\n");
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        PRINTF_E("Task: %s\n", pcTaskGetTaskName(NULL));

    cause = get_excep_desc(frame->exccause);
    if (g_double_exception_flag == 1) {
        PRINTF_E("Double Exception!\n");
        PRINTF_E("Double Exception Cause: %ld (%s)\n", frame->exccause,
                cause != NULL ? cause : "UNKNOWN");
        PRINTF_E("Double Exception PC: 0x%08x\n", depc);
        PRINTF_E("Double Exception Virtual Address: 0x%08lx\n", frame->excvaddr);
        PRINTF_E("First  Exception PC: 0x%08x\n", epc);
        g_double_exception_flag = 0;
    } else {
        PRINTF_E("Exception Cause: %ld (%s)\n", frame->exccause,
                cause != NULL ? cause : "UNKNOWN");
        PRINTF_E("Exception PC: 0x%08x\n", epc);
        PRINTF_E("Exception Virtual Address: 0x%08lx\n", frame->excvaddr);
    }

    print_backtrace();

    PRINTF_E("Registers:\n");
    PRINTF_E("   PC: 0x%08lx\n", frame->pc);
    PRINTF_E("   PS: 0x%08lx\n", frame->ps);
    PRINTF_E("   A0: 0x%08lx\n", frame->a0);
    PRINTF_E("   A1: 0x%08lx\n", frame->a1);
    PRINTF_E("   A2: 0x%08lx\n", frame->a2);
    PRINTF_E("   A3: 0x%08lx\n", frame->a3);
    PRINTF_E("   A4: 0x%08lx\n", frame->a4);
    PRINTF_E("   A5: 0x%08lx\n", frame->a5);
    PRINTF_E("   A6: 0x%08lx\n", frame->a6);
    PRINTF_E("   A7: 0x%08lx\n", frame->a7);
    PRINTF_E("   A8: 0x%08lx\n", frame->a8);
    PRINTF_E("   A9: 0x%08lx\n", frame->a9);
    PRINTF_E("  A10: 0x%08lx\n", frame->a10);
    PRINTF_E("  A11: 0x%08lx\n", frame->a11);
    PRINTF_E("  A12: 0x%08lx\n", frame->a12);
    PRINTF_E("  A13: 0x%08lx\n", frame->a13);
    PRINTF_E("  A14: 0x%08lx\n", frame->a14);
    PRINTF_E("  A15: 0x%08lx\n", frame->a15);
    PRINTF_E("  SAR: 0x%08lx\n", frame->sar);

    PRINTF_E("\n");
    dump_mem_around_register("PC", frame->pc);
    dump_mem_around_register("A0", frame->a0);
    dump_mem_around_register("A1", frame->a1);
    dump_mem_around_register("A2", frame->a2);
    dump_mem_around_register("A3", frame->a3);
    dump_mem_around_register("A4", frame->a4);
    dump_mem_around_register("A5", frame->a5);
    dump_mem_around_register("A6", frame->a6);
    dump_mem_around_register("A7", frame->a7);
    dump_mem_around_register("A8", frame->a8);
    dump_mem_around_register("A9", frame->a9);
    dump_mem_around_register("A10", frame->a10);
    dump_mem_around_register("A11", frame->a11);
    dump_mem_around_register("A12", frame->a12);
    dump_mem_around_register("A13", frame->a13);
    dump_mem_around_register("A14", frame->a14);
    dump_mem_around_register("A15", frame->a15);

    xthal_dcache_all_writeback_inv();

#ifdef CFG_WDT_SUPPORT
    /* watchdog irq only mode */
    mtk_wdt_irq_trigger();
#endif

    while (1) {
        __asm(
            "memw;\n"
            "WAITI 15;\n"
        );
    }
}

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void vAssertCalled(char *file, unsigned int line)
{
    printf("Assertion failed in %s:%d\n", file, line);

    __asm__ (
        "movi.n  a7,0\n"
        "quos    a6,a6,a7\n"
    );
}


/** init exception handler
*  @returns
*    no return
*/
NORMAL_SECTION_FUNC void adsp_excep_init(void)
{
    int i;
    int cnt = XEA2_EXCEPTION_COUNT;

    for (i = 0; i < cnt; i++) {
        xt_set_exception_handler(i, (xt_exc_handler)adsp_excep_handler);
    }

#ifdef CFG_HW_BREAKPOINT_SUPPORT
    xt_set_exception_handler(EXCCAUSE_DEBUG, hwbp_dbg_handler);
#endif
}
