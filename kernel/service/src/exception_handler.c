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

/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "stdarg.h"
#include "memory_attribute.h"
#include "exception_handler.h"
#include "hal_dwt.h"
#ifdef HAL_WDT_MODULE_ENABLED
#include "hal_wdt.h"
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */

#include "elf.h"

#if  defined ( __GNUC__ )
#ifndef __weak
#define __weak   __attribute__((weak))
#endif /* #ifndef __weak */
#endif /* #if  defined ( __GNUC__ ) */

#if defined(MTK_MINI_DUMP_ENABLE)
#include "string.h"
extern void *xTaskGetCurrentTaskHandle(void);
#endif /* #if defined(MTK_MINI_DUMP_ENABLE) */


//#define CHECK_EXCEPTION_STACK_USAGE 0
//#if (CHECK_EXCEPTION_STACK_USAGE == 1)
//#include <string.h>
//#endif

//Local Exception Feature Define
#define EXCEPTION_AUTO_REBOOT      0
#define MAX_EXCEPTION_CONFIGURATIONS 6
#define EXCEPTION_STACK_WORDS        384

#define ALIGN(x) ((x) - (x)%4)
#define DISTANCE(x, y) ((x) > (y) ? (x) - (y) : (y) - (x))

typedef struct {
    int items;
    exception_config_type configs[MAX_EXCEPTION_CONFIGURATIONS];
} exception_config_t;

typedef struct {
    bool        is_valid;
    const char *expr;
    const char *file;
    int         line;
} assert_expr_t;

//static unsigned int xExceptionStack[EXCEPTION_STACK_WORDS] = {0}; /* reserved as exception handler's stack */
//unsigned int *pxExceptionStack = &xExceptionStack[EXCEPTION_STACK_WORDS-1];
exception_config_t exception_config = {0};

static assert_expr_t assert_expr = {0};
static unsigned int reboot_flag = 0;

extern memory_region_type memory_regions[];

uint32_t exc_delay_ms = 0;

enum { r0, r1, r2, r3, r12, lr, pc, psr,
       s0, s1, s2, s3, s4, s5, s6, s7,
       s8, s9, s10, s11, s12, s13, s14, s15,
       fpscr
     };

typedef struct TaskContextType {
    unsigned int r0;
    unsigned int r1;
    unsigned int r2;
    unsigned int r3;
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
    unsigned int r8;
    unsigned int r9;
    unsigned int r10;
    unsigned int r11;
    unsigned int r12;
    unsigned int sp;              /* after pop r0-r3, lr, pc, xpsr                   */
    unsigned int lr;              /* lr before exception                             */
    unsigned int pc;              /* pc before exception                             */
    unsigned int psr;             /* xpsr before exeption                            */
    unsigned int control;         /* nPRIV bit & FPCA bit meaningful, SPSEL bit = 0  */
    unsigned int exc_return;      /* current lr                                      */
    unsigned int msp;             /* msp                                             */
    unsigned int msplim;          /* msp limit                                       */
    unsigned int psp;             /* psp                                             */
    unsigned int psplim;          /* psp limit                                       */
    unsigned int fpscr;
    unsigned int s0;
    unsigned int s1;
    unsigned int s2;
    unsigned int s3;
    unsigned int s4;
    unsigned int s5;
    unsigned int s6;
    unsigned int s7;
    unsigned int s8;
    unsigned int s9;
    unsigned int s10;
    unsigned int s11;
    unsigned int s12;
    unsigned int s13;
    unsigned int s14;
    unsigned int s15;
    unsigned int s16;
    unsigned int s17;
    unsigned int s18;
    unsigned int s19;
    unsigned int s20;
    unsigned int s21;
    unsigned int s22;
    unsigned int s23;
    unsigned int s24;
    unsigned int s25;
    unsigned int s26;
    unsigned int s27;
    unsigned int s28;
    unsigned int s29;
    unsigned int s30;
    unsigned int s31;
} TaskContext;


static TaskContext taskContext = {0};
ATTR_USED TaskContext *pTaskContext = &taskContext;

#define COREDUMP_EXTRA_INFO_SIZE 128
char g_coredump_info[COREDUMP_EXTRA_INFO_SIZE] = {0};
typedef struct ExtraInfoDumpType {
    unsigned int control;
    unsigned int msp;             /* msp                                             */
    unsigned int msplim;          /* msp limit                                       */
    unsigned int psp;             /* psp                                             */
    unsigned int psplim;          /* psp limit                                       */
} T_ExtraInfo_Dump;

#if (EXCEPTION_AUTO_REBOOT == 1)
static int auto_reboot_flag = 0;

#define EXCEPTION_AUTO_REBOOT_MAGIC  0x4F545541 //AUTO
static bool auto_reboot_check(void);

static void exception_config_wdt()
{
    if (auto_reboot_check() == true) {
#ifdef HAL_WDT_MODULE_ENABLED
        hal_wdt_config_t wdt_config;
        wdt_config.mode = HAL_WDT_MODE_RESET;
        wdt_config.seconds = 30;
        hal_wdt_disable(HAL_WDT_DISABLE_MAGIC);
        hal_wdt_init(&wdt_config);
        hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
    } else {
#ifdef HAL_WDT_MODULE_ENABLED
        hal_wdt_disable(HAL_WDT_DISABLE_MAGIC);
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
    }
}

static void exception_feed_wdt()
{
    if (auto_reboot_check() == true) {
#ifdef HAL_WDT_MODULE_ENABLED
        hal_wdt_feed(HAL_WDT_FEED_MAGIC);
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
    }
}

#endif /* #if (EXCEPTION_AUTO_REBOOT == 1) */


/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/
void abort(void)
{
    __asm("cpsid i");

#ifdef __ARM_FEATURE_UNALIGNED
    SCB->CCR |=  SCB_CCR_UNALIGN_TRP_Msk;
    *((volatile unsigned int *) 0xFFFFFFF1) = 1;
    for (;;);
#else /* #ifdef __ARM_FEATURE_UNALIGNED */
    __asm volatile("udf #255");
    for (;;);
#endif /* #ifdef __ARM_FEATURE_UNALIGNED */
}

void platform_assert(const char *expr, const char *file, int line)
{
    //For -mno-unaligned-access
#ifdef __ARM_FEATURE_UNALIGNED
    __asm("cpsid i");
    SCB->CCR |=  SCB_CCR_UNALIGN_TRP_Msk;
    assert_expr.is_valid = true;
    assert_expr.expr = expr;
    assert_expr.file = file;
    assert_expr.line = line;
    *((volatile unsigned int *) 0xFFFFFFF1) = 1;
#else /* #ifdef __ARM_FEATURE_UNALIGNED */
    __asm volatile("udf #255");
#endif /* #ifdef __ARM_FEATURE_UNALIGNED */
}

void exception_get_assert_expr(const char **expr, const char **file, int *line)
{
    if (assert_expr.is_valid) {
        if (assert_expr.expr == NULL)
            assert_expr.expr = "(NULL)";
        *expr = assert_expr.expr;
        *file = assert_expr.file;
        *line = assert_expr.line;
    } else {
        *expr = NULL;
        *file = NULL;
        *line = 0;
    }
}

void exception_dump_config(int cfg_type, int flag)
{

    switch (cfg_type) {
        case EXCEPT_CFG_REBOOT: {
                reboot_flag = (unsigned int)flag;
                break;
            }
        default:
            break;
    }

    return;
}

void exception_reboot_config(bool auto_reboot)
{
#if (EXCEPTION_AUTO_REBOOT == 1)
    if (auto_reboot)
        auto_reboot_flag = EXCEPTION_AUTO_REBOOT_MAGIC;
    else
        auto_reboot_flag = 0;
#endif /* #if (EXCEPTION_AUTO_REBOOT == 1) */
}

#if defined (__CC_ARM) || defined (__ICCARM__)

void __aeabi_assert(const char *expr, const char *file, int line)
{
    platform_assert(expr, file, line);
}

#endif /* #if defined (__CC_ARM) || defined (__ICCARM__) */

bool exception_register_callbacks(exception_config_type *cb)
{
    int i;

    if ((exception_config.items < 0) || (exception_config.items >= MAX_EXCEPTION_CONFIGURATIONS)) {
        return false;
    }

    /* check if it is already registered */
    for (i = 0; i < exception_config.items; i++) {
        if (exception_config.configs[i].init_cb == cb->init_cb
            && exception_config.configs[i].dump_cb == cb->dump_cb) {
            return false;
        }
    }
    exception_config.configs[exception_config.items].init_cb = cb->init_cb;
    exception_config.configs[exception_config.items].dump_cb = cb->dump_cb;
    //platform_printf("Regiter: item(%d), init_cb(%p), dump_cb(%p)\r\n\r\n", exception_config.items,
    //                 exception_config.configs[exception_config.items].init_cb,
    //                 exception_config.configs[exception_config.items].dump_cb);

    exception_config.items++;
    return true;
}



#ifdef MTK_MEMORY_EXTRACTOR_ENABLE

#include "memextract.h"
#include "memory_map.h"

#define DUMP_EXP_REGION_NUM (5 + 1) // Header + regions
#define DUMP_EXP_PSPSTACK_LENGTH (1024*3)
#define DUMP_EXP_MSPSTACK_LENGTH (1024*4)

#define DUMP_EXP_HEADER_FIELD_NUM (5 + 1) // field num + other fields

static region_list_t Exp_DumpToFlashRegion[ DUMP_EXP_REGION_NUM ];

static uint32_t DumpToFlashRegionHeader[ DUMP_EXP_HEADER_FIELD_NUM ];

static void Exp_DumpToFlash(region_list_t **region_list, int *num, uint32_t *total_size)
{
    uint32_t xExpDumpSize = 0;

    /*
    * Add a header to record info of regions
    *    1. sizeof(taskContext)
    *    2. DUMP_EXP_PSPSTACK_LENGTH
    *    3. DUMP_EXP_MSPSTACK_LENGTH
    */
    DumpToFlashRegionHeader[0] = DUMP_EXP_HEADER_FIELD_NUM - 1;
    DumpToFlashRegionHeader[1] = sizeof(taskContext);
    DumpToFlashRegionHeader[2] = DUMP_EXP_PSPSTACK_LENGTH;
    DumpToFlashRegionHeader[3] = DUMP_EXP_MSPSTACK_LENGTH;
    DumpToFlashRegionHeader[4] = 4;
    DumpToFlashRegionHeader[5] = 4 * 16;

    Exp_DumpToFlashRegion[0].start_addr = (void *)DumpToFlashRegionHeader;
    Exp_DumpToFlashRegion[0].len = sizeof(DumpToFlashRegionHeader);
    xExpDumpSize += sizeof(DumpToFlashRegionHeader);

    /*
    * add rergisters region
    */
    Exp_DumpToFlashRegion[1].start_addr = (void *)&taskContext;
    Exp_DumpToFlashRegion[1].len = sizeof(taskContext);
    xExpDumpSize += sizeof(taskContext);
    /*
    * add psp region
    */
    Exp_DumpToFlashRegion[2].start_addr = (void *)taskContext.psp;
    Exp_DumpToFlashRegion[2].len = DUMP_EXP_PSPSTACK_LENGTH;
    xExpDumpSize += DUMP_EXP_PSPSTACK_LENGTH;
    /*
    * add msp region
    */
    Exp_DumpToFlashRegion[3].start_addr = (void *)STACK_START;
    Exp_DumpToFlashRegion[3].len = DUMP_EXP_MSPSTACK_LENGTH;
    xExpDumpSize += DUMP_EXP_MSPSTACK_LENGTH;
    /*
    * add SCB registers
    */
    Exp_DumpToFlashRegion[4].start_addr = (void *)SCnSCB->ACTLR;
    Exp_DumpToFlashRegion[4].len = 4;
    xExpDumpSize += 4;
    /*
    * add SCB registers
    */
    Exp_DumpToFlashRegion[5].start_addr = (void *)SCS_BASE;
    Exp_DumpToFlashRegion[5].len = 4 * 16;
    xExpDumpSize += (4 * 16);


    *region_list = Exp_DumpToFlashRegion;
    *num = DUMP_EXP_REGION_NUM;
    *total_size = xExpDumpSize;
}
#endif /* #ifdef MTK_MEMORY_EXTRACTOR_ENABLE */

ATTR_USED ATTR_TEXT_IN_SYSRAM void exception_init(void)
{
    int i;

    SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk;

#if (EXCEPTION_AUTO_REBOOT == 1)
    exception_config_wdt();
#endif /* #if (EXCEPTION_AUTO_REBOOT == 1) */

#if (configUSE_FLASH_SUSPEND == 1)
    Flash_ReturnReady();
#endif /* #if (configUSE_FLASH_SUSPEND == 1) */


    // Dump to flash
#ifdef MTK_MEMORY_EXTRACTOR_ENABLE
    {
        // Register dump to flash callback
        memextract_entry xDumptToFlashEntry;
        xDumptToFlashEntry.cb = Exp_DumpToFlash;
        xDumptToFlashEntry.type = MEMORY_EXTRACT_OTHER;
        memextract_register_callbacks(&xDumptToFlashEntry);
    }
#endif /* #ifdef MTK_MEMORY_EXTRACTOR_ENABLE */


    //#if (CHECK_EXCEPTION_STACK_USAGE == 1)
    //    memset(xExceptionStack, (int)0xa5, (EXCEPTION_STACK_WORDS - 16)*4);
    //#endif

    for (i = 0; i < exception_config.items; i++) {
        if (exception_config.configs[i].init_cb) {
            exception_config.configs[i].init_cb(NULL, 0);
#if (EXCEPTION_AUTO_REBOOT == 1)
            exception_feed_wdt();
#endif /* #if (EXCEPTION_AUTO_REBOOT == 1) */
        }
    }

    if (assert_expr.is_valid) {
        platform_printf("assert failed: %s, file: %s, line: %d\n\r", assert_expr.expr, assert_expr.file,  assert_expr.line);
    }
}

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

void printUsageErrorMsg(uint32_t CFSRValue)
{
    platform_printf("Usage fault: ");

    CFSRValue >>= 16; /* right shift to lsb */
    if ((CFSRValue & (1 << 9)) != 0) {
        platform_printf("Divide by zero\n\r");
    }
    if ((CFSRValue & (1 << 8)) != 0) {
        platform_printf("Unaligned access\n\r");
    }
    if ((CFSRValue & (1 << 3)) != 0) {
        platform_printf("Coprocessor error\n\r");
    }
    if ((CFSRValue & (1 << 2)) != 0) {
        platform_printf("Invalid EXC_RETURN\n\r");
    }
    if ((CFSRValue & (1 << 1)) != 0) {
        platform_printf("Invalid state\n\r");
    }
    if ((CFSRValue & (1 << 0)) != 0) {
        platform_printf("Undefined instruction\n\r");
    }
}

void printMemoryManagementErrorMsg(uint32_t CFSRValue)
{
    platform_printf("Memory Management fault: ");

    CFSRValue &= 0x000000FF; /* mask mem faults */
    if ((CFSRValue & (1 << 5)) != 0) {
        platform_printf("A MemManage fault occurred during FP lazy state preservation\n\r");
    }
    if ((CFSRValue & (1 << 4)) != 0) {
        platform_printf("A derived MemManage fault occurred on exception entry\n\r");
    }
    if ((CFSRValue & (1 << 3)) != 0) {
        platform_printf("A derived MemManage fault occurred on exception return\n\r");
    }
    if ((CFSRValue & (1 << 1)) != 0) { /* Need to check valid bit (bit 7 of CFSR)? */
        platform_printf("Data access violation @0x%08x\n\r", (unsigned int)SCB->MMFAR);
    }
    if ((CFSRValue & (1 << 0)) != 0) {
        platform_printf("MPU or Execute Never (XN) default memory map access violation\n\r");
    }
    if ((CFSRValue & (1 << 7)) != 0) { /* To review: remove this if redundant */
        platform_printf("SCB->MMFAR = 0x%08x\n\r", (unsigned int)SCB->MMFAR);
    }
}

void printBusFaultErrorMsg(uint32_t CFSRValue)
{
    platform_printf("Bus fault: ");

    CFSRValue &= 0x0000FF00; /* mask bus faults */
    CFSRValue >>= 8;
    if ((CFSRValue & (1 << 5)) != 0) {
        platform_printf("A bus fault occurred during FP lazy state preservation\n\r");
    }
    if ((CFSRValue & (1 << 4)) != 0) {
        platform_printf("A derived bus fault has occurred on exception entry\n\r");
    }
    if ((CFSRValue & (1 << 3)) != 0) {
        platform_printf("A derived bus fault has occurred on exception return\n\r");
    }
    if ((CFSRValue & (1 << 2)) != 0) {
        platform_printf("Imprecise data access error has occurred\n\r");
    }
    if ((CFSRValue & (1 << 1)) != 0) { /* Need to check valid bit (bit 7 of CFSR)? */
        platform_printf("A precise data access error has occurred @x%08x\n\r", (unsigned int)SCB->BFAR);
    }
    if ((CFSRValue & (1 << 0)) != 0) {
        platform_printf("A bus fault on an instruction prefetch has occurred\n\r");
    }
    if ((CFSRValue & (1 << 7)) != 0) { /* To review: remove this if redundant */
        platform_printf("SCB->BFAR = 0x%08x\n\r", (unsigned int)SCB->BFAR);
    }
}


#if defined(MTK_MINI_DUMP_ENABLE)

#if defined(MTK_MINI_DUMP_ENABLE) || defined(HAL_DWT_MODULE_ENABLED) && defined (KI_RLG_ENABLE_EVENT_OVER_HSL)

#if !defined(MTK_GCC_LTO_ENABLE)
/**
 *  LTO choice weak definition in LTO object even a strong definition is existing in normal object.
 *  These weaks definiton will causes problem, if enabled LTO.
 */
__weak void *xTaskGetCurrentTaskHandle(void)
{
    return 0;
}

__weak char *pcTaskGetTaskName(void *taskHandle)
{
    return NULL;
}
#endif /* #if !defined(MTK_GCC_LTO_ENABLE) */

#endif /* #if defined(MTK_MINI_DUMP_ENABLE) || defined(HAL_DWT_MODULE_ENABLED) && defined (KI_RLG_ENABLE_EVENT_OVER_HSL) */


#endif /* #if defined(MTK_MINI_DUMP_ENABLE) */

/* It is defined as a weak function.
 * It needs to be implemented in project.
 * The default behvior is NOP, and the memory dump continues.
 */
__weak void exception_reboot(void)
{
#ifdef HAL_WDT_MODULE_ENABLED
    hal_wdt_config_t wdt_config;
    wdt_config.mode = HAL_WDT_MODE_RESET;
    wdt_config.seconds = 10;
    hal_wdt_disable(HAL_WDT_DISABLE_MAGIC);
    hal_wdt_init(&wdt_config);
    hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
    // hal_wdt_software_reset();
    while (1);
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
}

static unsigned int reboot_check(void)
{
    return reboot_flag;
}


#if (EXCEPTION_AUTO_REBOOT == 1)
static bool auto_reboot_check(void)
{
    return (auto_reboot_flag == EXCEPTION_AUTO_REBOOT_MAGIC);
}

//TODO: need log owner offer genie connect status
bool is_genie_connected(void)
{
    return true;
}
#endif /* #if (EXCEPTION_AUTO_REBOOT == 1) */

static void exception_infinite_loop(void)
{
    while (1) {
        // kick watch dog if enabled
#if defined(HAL_WDT_MODULE_ENABLED) && defined(MTK_SYSTEM_HANG_CHECK_ENABLE)
        // delay
        for (uint32_t count = (1 * 1024 * 1024); count != 0 ; --count) {
            asm("");
        }
        hal_wdt_feed(HAL_WDT_FEED_MAGIC);
#endif /* #if defined(HAL_WDT_MODULE_ENABLED) && defined(MTK_SYSTEM_HANG_CHECK_ENABLE) */
    }
}

void exception_dump_show(unsigned int show_fmt, unsigned int *buf, unsigned int bufsize)
{
}

/* To keep the output format on Teraterm */
static void dumpBusyDelayMs(uint32_t ms)
{
    // TODO: calculate with MPU Hz.
    uint32_t count = ms * 1024 * 1024;
    do {
        // nop to prevent this function gone after compiler optimization
        asm("");
        count--;
    } while (count != 0);
}

void memoryDumpAll(void)
{
    unsigned int *current, *end;
    unsigned int i;

    // static region dump
    platform_printf("\n\rmemory dump start.\n\r\n\r");

    for (i = 0; ; i++) {

        if (!memory_regions[i].region_name) {
            break;
        }

        if (!memory_regions[i].is_dumped) {
            continue;
        }

#if (EXCEPTION_AUTO_REBOOT == 1)
        exception_feed_wdt();
#endif /* #if (EXCEPTION_AUTO_REBOOT == 1) */

        current = memory_regions[i].start_address;
        end     = memory_regions[i].end_address;

        for (; current < end; current += 4) {

#if (EXCEPTION_AUTO_REBOOT == 1)
            //feed wdt every 64k
            if ((((unsigned int)current) & 0xFFFFFFF0) % 65536 == 0)
                exception_feed_wdt();
#endif /* #if (EXCEPTION_AUTO_REBOOT == 1) */

            if (*(current + 0) == 0 && *(current + 1) == 0 && *(current + 2) == 0 && *(current + 3) == 0) {
                continue;
            }
            platform_printf("0x%08x: %08x %08x %08x %08x\n\r", (unsigned int)current, *(current + 0), *(current + 1), *(current + 2), *(current + 3));
            if (exc_delay_ms != 0) {
                dumpBusyDelayMs(exc_delay_ms);
            }
        }

    }

    platform_printf("\n\rmemory dump completed.\n\r");
    platform_printf("\n===============================================\n");
    platform_printf("\rException Log: Copy End\r");
    platform_printf("\n===============================================\n");
    // dynamic region dump. Memory dump to flash is also executed here.
    for (i = 0; i < (unsigned int)exception_config.items; i++) {
        if (exception_config.configs[i].dump_cb) {
            exception_config.configs[i].dump_cb(NULL, 0);
        }
    }

    /* Genie complete message */
    platform_printf("<<<<<<<< LOG END LOG END LOG END LOG END LOG END <<<<<<<<\n");

    if ((reboot_check() == DISABLE_WHILELOOP_MAGIC)
#if (EXCEPTION_AUTO_REBOOT == 1)
        || (auto_reboot_check())
#endif /* #if (EXCEPTION_AUTO_REBOOT == 1) */
       ) {
        exception_reboot();
    }

}

void stackDump(uint32_t stack[])
{
    platform_printf("\n===============================================\n");
    platform_printf("\rException Log: Copy Start\r");
    platform_printf("\n===============================================\n");

    taskContext.r0   = stack[r0];
    taskContext.r1   = stack[r1];
    taskContext.r2   = stack[r2];
    taskContext.r3   = stack[r3];
    taskContext.r12  = stack[r12];
    taskContext.sp   = ((uint32_t)stack) + 0x20;
    taskContext.lr   = stack[lr];
    taskContext.pc   = stack[pc];
    taskContext.psr  = stack[psr];

    /* FPU context? */
    if ((taskContext.exc_return & 0x10) == 0) {
        taskContext.s0 = stack[s0];
        taskContext.s1 = stack[s1];
        taskContext.s2 = stack[s2];
        taskContext.s3 = stack[s3];
        taskContext.s4 = stack[s4];
        taskContext.s5 = stack[s5];
        taskContext.s6 = stack[s6];
        taskContext.s7 = stack[s7];
        taskContext.s8 = stack[s8];
        taskContext.s9 = stack[s9];
        taskContext.s10 = stack[s10];
        taskContext.s11 = stack[s11];
        taskContext.s12 = stack[s12];
        taskContext.s13 = stack[s13];
        taskContext.s14 = stack[s14];
        taskContext.s15 = stack[s15];
        taskContext.fpscr = stack[fpscr];
        taskContext.sp += 72; /* s0-s15, fpsr, reserved */
    }

#if (__CORTEX_M != (33U) )
    /* if CCR.STKALIGN=1, check PSR[9] to know if there is forced stack alignment */
    if ((SCB->CCR & SCB_CCR_STKALIGN_Msk) && (taskContext.psr & 0x200)) {
        taskContext.sp += 4;
    }
#endif /* #if (__CORTEX_M != (33U) ) */

    platform_printf("r0  = 0x%08x\n\r", taskContext.r0);
    platform_printf("r1  = 0x%08x\n\r", taskContext.r1);
    platform_printf("r2  = 0x%08x\n\r", taskContext.r2);
    platform_printf("r3  = 0x%08x\n\r", taskContext.r3);
    platform_printf("r4  = 0x%08x\n\r", taskContext.r4);
    platform_printf("r5  = 0x%08x\n\r", taskContext.r5);
    platform_printf("r6  = 0x%08x\n\r", taskContext.r6);
    platform_printf("r7  = 0x%08x\n\r", taskContext.r7);
    platform_printf("r8  = 0x%08x\n\r", taskContext.r8);
    platform_printf("r9  = 0x%08x\n\r", taskContext.r9);
    platform_printf("r10 = 0x%08x\n\r", taskContext.r10);
    platform_printf("r11 = 0x%08x\n\r", taskContext.r11);
    platform_printf("r12 = 0x%08x\n\r", taskContext.r12);
    platform_printf("lr  = 0x%08x\n\r", taskContext.lr);
    platform_printf("pc  = 0x%08x\n\r", taskContext.pc);
    platform_printf("psr = 0x%08x\n\r", taskContext.psr);
    platform_printf("EXC_RET = 0x%08x\n\r", taskContext.exc_return);

    /* update CONTROL.SPSEL and psp if returning to thread mode */
    if (taskContext.exc_return & 0x4) {
        taskContext.control |= 0x2; /* CONTROL.SPSel */
        taskContext.psp = taskContext.sp;
    } else { /* update msp if returning to handler mode */
        taskContext.msp = taskContext.sp;
    }

    /* FPU context? */
    if ((taskContext.exc_return & 0x10) == 0) {
        taskContext.control |= 0x4; /* CONTROL.FPCA */
        platform_printf("s0  = 0x%08x\n\r", taskContext.s0);
        platform_printf("s1  = 0x%08x\n\r", taskContext.s1);
        platform_printf("s2  = 0x%08x\n\r", taskContext.s2);
        platform_printf("s3  = 0x%08x\n\r", taskContext.s3);
        platform_printf("s4  = 0x%08x\n\r", taskContext.s4);
        platform_printf("s5  = 0x%08x\n\r", taskContext.s5);
        platform_printf("s6  = 0x%08x\n\r", taskContext.s6);
        platform_printf("s7  = 0x%08x\n\r", taskContext.s7);
        platform_printf("s8  = 0x%08x\n\r", taskContext.s8);
        platform_printf("s9  = 0x%08x\n\r", taskContext.s9);
        platform_printf("s10 = 0x%08x\n\r", taskContext.s10);
        platform_printf("s11 = 0x%08x\n\r", taskContext.s11);
        platform_printf("s12 = 0x%08x\n\r", taskContext.s12);
        platform_printf("s13 = 0x%08x\n\r", taskContext.s13);
        platform_printf("s14 = 0x%08x\n\r", taskContext.s14);
        platform_printf("s15 = 0x%08x\n\r", taskContext.s15);
        platform_printf("s16 = 0x%08x\n\r", taskContext.s16);
        platform_printf("s17 = 0x%08x\n\r", taskContext.s17);
        platform_printf("s18 = 0x%08x\n\r", taskContext.s18);
        platform_printf("s19 = 0x%08x\n\r", taskContext.s19);
        platform_printf("s20 = 0x%08x\n\r", taskContext.s20);
        platform_printf("s21 = 0x%08x\n\r", taskContext.s21);
        platform_printf("s22 = 0x%08x\n\r", taskContext.s22);
        platform_printf("s23 = 0x%08x\n\r", taskContext.s23);
        platform_printf("s24 = 0x%08x\n\r", taskContext.s24);
        platform_printf("s25 = 0x%08x\n\r", taskContext.s25);
        platform_printf("s26 = 0x%08x\n\r", taskContext.s26);
        platform_printf("s27 = 0x%08x\n\r", taskContext.s27);
        platform_printf("s28 = 0x%08x\n\r", taskContext.s28);
        platform_printf("s29 = 0x%08x\n\r", taskContext.s29);
        platform_printf("s30 = 0x%08x\n\r", taskContext.s30);
        platform_printf("s31 = 0x%08x\n\r", taskContext.s31);
        platform_printf("fpscr = 0x%08x\n\r", taskContext.fpscr);
    }

    platform_printf("CONTROL = 0x%08x\n\r", taskContext.control);
    platform_printf("MSP     = 0x%08x\n\r", taskContext.msp);
    platform_printf("MSPLIM  = 0x%08x\n\r", taskContext.msplim);
    platform_printf("PSP     = 0x%08x\n\r", taskContext.psp);
    platform_printf("PSPLIM  = 0x%08x\n\r", taskContext.psplim);
    platform_printf("sp      = 0x%08x\n\r", taskContext.sp);

    if (reboot_check() == DISABLE_MEMDUMP_MAGIC) {
        exception_reboot();
    }

#if (EXCEPTION_AUTO_REBOOT == 1)
    if (auto_reboot_check() && (is_genie_connected() == false)) {
        // don't dump if genie tool is not connect
        exception_reboot();
    }
#endif /* #if (EXCEPTION_AUTO_REBOOT == 1) */
}


/*
 * Debug scenarios:
 *
 * (1) debug with debugger, stop in first exception.
 *     Print the exception context, and halt cpu.
 *
 *     DEBUGGER_ON: 1
 *
 * (2) debug with uart, stop in first exception.
 *     Print the exception context, and enter an infinite loop.
 *
 *     DEBUGGER_ON: 0
 */

#define DEBUGGER_ON    0

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
ATTR_USED void Hard_Fault_Handler(uint32_t stack[])
{
    platform_printf("\n\rIn Hard Fault Handler\n\r");
    platform_printf("SCB->HFSR = 0x%08x\n\r", (unsigned int)SCB->HFSR);

    if ((SCB->HFSR & (1 << 30)) != 0) {
        platform_printf("Forced Hard Fault\n\r");
        platform_printf("SCB->CFSR = 0x%08x\n\r", (unsigned int)SCB->CFSR);
        if ((SCB->CFSR & 0xFFFF0000) != 0) {
            printUsageErrorMsg(SCB->CFSR);
        }
        if ((SCB->CFSR & 0x0000FF00) != 0) {
            printBusFaultErrorMsg(SCB->CFSR);
        }
        if ((SCB->CFSR & 0x000000FF) != 0) {
            printMemoryManagementErrorMsg(SCB->CFSR);
        }
    }

    stackDump(stack);

    memoryDumpAll();

#if DEBUGGER_ON
    __ASM volatile("BKPT #01");
#else /* #if DEBUGGER_ON */
    exception_infinite_loop();
#endif /* #if DEBUGGER_ON */
}

ATTR_USED void MemManage_Fault_Handler(uint32_t stack[])
{
    platform_printf("\n\rIn MemManage Fault Handler\n\r");
    platform_printf("SCB->CFSR = 0x%08x\n\r", (unsigned int)SCB->CFSR);

    if ((SCB->CFSR & 0xFF) != 0) {
        printMemoryManagementErrorMsg(SCB->CFSR);
    }

    stackDump(stack);

    memoryDumpAll();

#if DEBUGGER_ON
    __ASM volatile("BKPT #01");
#else /* #if DEBUGGER_ON */
    exception_infinite_loop();
#endif /* #if DEBUGGER_ON */
}

ATTR_USED void Bus_Fault_Handler(uint32_t stack[])
{

    platform_printf("\n\rIn Bus Fault Handler\n\r");
    platform_printf("SCB->CFSR = 0x%08x\n\r", (unsigned int)SCB->CFSR);

    if ((SCB->CFSR & 0xFF00) != 0) {
        printBusFaultErrorMsg(SCB->CFSR);
    }

    stackDump(stack);

    memoryDumpAll();

#if DEBUGGER_ON
    __ASM volatile("BKPT #01");
#else /* #if DEBUGGER_ON */
    exception_infinite_loop();
#endif /* #if DEBUGGER_ON */
}

ATTR_USED void Usage_Fault_Handler(uint32_t stack[])
{
    platform_printf("\n\rIn Usage Fault Handler\n\r");
    platform_printf("SCB->CFSR = 0x%08x\n\r", (unsigned int)SCB->CFSR);

    if ((SCB->CFSR & 0xFFFF0000) != 0) {
        printUsageErrorMsg(SCB->CFSR);
    }

    stackDump(stack);

    memoryDumpAll();

#if DEBUGGER_ON
    __ASM volatile("BKPT #01");
#else /* #if DEBUGGER_ON */
    exception_infinite_loop();
#endif /* #if DEBUGGER_ON */
}

#ifdef HAL_DWT_MODULE_ENABLED
ATTR_USED void Debug_Monitor_Handler(uint32_t stack[])
{
    uint32_t offset, stack_end, is_match;

    platform_printf("\n\rIn Debug Monitor Fault Handler\r\n");

    /* is task stack overflow? */
    {
        offset = (0x10 * HAL_DWT_3) / 4;
        is_match = ((*((uint32_t *)&DWT->FUNCTION0 + offset))& DWT_FUNCTION_MATCHED_Msk) >> DWT_FUNCTION_MATCHED_Pos;
        stack_end = *((uint32_t *)&DWT->COMP0 + offset);
        platform_printf("Task stack overflow:%c, stack end:0x%x \r\n", ((is_match) ? 'Y' : 'N'), (unsigned int)stack_end);
    }

    stackDump(stack);

    memoryDumpAll();

#if DEBUGGER_ON
    __ASM volatile("BKPT #01");
#else /* #if DEBUGGER_ON */
    exception_infinite_loop();
#endif /* #if DEBUGGER_ON */
}
#endif /* #ifdef HAL_DWT_MODULE_ENABLED */

ATTR_USED void Platform_Abort_Handler(uint32_t stack[])
{
    platform_printf("\n\rIn Platform Abort Handler\n\r");
    platform_printf("SCB->CFSR = 0x%08x\n\r", (unsigned int)SCB->CFSR);

    if ((SCB->CFSR & 0xFFFF0000) != 0) {
        printUsageErrorMsg(SCB->CFSR);
    }

    stackDump(stack);

    memoryDumpAll();

#if DEBUGGER_ON
    __ASM volatile("BKPT #01");
#else /* #if DEBUGGER_ON */
    while (1);
#endif /* #if DEBUGGER_ON */
}

ATTR_USED void Platform_Assert_Handler(uint32_t stack[])
{
    platform_printf("\n\rIn Platform Assert Handler\n\r");
    platform_printf("SCB->CFSR = 0x%08x\n\r", (unsigned int)SCB->CFSR);

    if ((SCB->CFSR & 0xFFFF0000) != 0) {
        printUsageErrorMsg(SCB->CFSR);
    }

    stackDump(stack);

    memoryDumpAll();

#if DEBUGGER_ON
    __ASM volatile("BKPT #01");
#else /* #if DEBUGGER_ON */
    exception_infinite_loop();
#endif /* #if DEBUGGER_ON */
}


/******************************************************************************/
/*                   Toolchain Dependent Part                                 */
/******************************************************************************/
#if defined(__GNUC__)

#define __EXHDLR_ATTR__ __attribute__((naked, used)) ATTR_TEXT_IN_SYSRAM

/**
  * @brief  This function is the common part of exception handlers.
  * @param  r3 holds EXC_RETURN value
  * @retval None
  */
__EXHDLR_ATTR__ void CommonFault_Handler(void)
{
    __asm volatile
    (
        "cpsid i                       \n"     /* disable irq                 */
        "push {lr}                     \n"
        "bl Flash_ReturnReady          \n"     /* must suspend flash before fetch code from flash. */
        "pop  {lr}                     \n"
        /*"ldr r3, =pxExceptionStack     \n"*/
        /*"ldr r3, [r3]                  \n"*/     /* r3 := pxExceptionStack      */
        "ldr r0, =pTaskContext         \n"
        "ldr r0, [r0]                  \n"     /* r0 := pTaskContext          */
        "add r0, r0, #16               \n"     /* point to context.r4         */
        "stmia r0!, {r4-r11}           \n"     /* store r4-r11                */
        "mov r5, r12                   \n"     /* r5 := EXC_RETURN            */
        "add r0, r0, #20               \n"     /* point to context.control    */
        "mrs r1, control               \n"     /* move CONTROL to r1          */
        "str r1, [r0], #4              \n"     /* store CONTROL               */
        "str r5, [r0], #4              \n"     /* store EXC_RETURN            */
        "mrs r4, msp                   \n"     /* r4 := MSP                   */
        "str r4, [r0], #4              \n"     /* store MSP                   */
        "mrs r4, msplim                \n"     /* r4 := MSPLIM                */
        "str r4, [r0], #4              \n"     /* store MSPLIM                */
        "mrs r1, psp                   \n"     /* move PSP to r1              */
        "str r1, [r0], #4              \n"     /* store PSP                   */
        "mrs r1, psplim                \n"     /* move PSPLIM to r1           */
        "str r1, [r0]                  \n"     /* store PSPLIM                */
        "tst r5, #0x10                 \n"     /* FPU context?                */
        "itt eq                        \n"
        "addeq r0, r0, #68             \n"     /* point to contex.s16         */
        "vstmeq r0, {s16-s31}          \n"     /* store r16-r31               */
        /*"cmp r3, #0                    \n"*/     /* if (!pxExceptionStack)      */
        /*"it ne                         \n"*/
        /*"movne sp, r3                  \n"*/     /* update msp                  */
        "push {lr}                     \n"
        "bl exception_init             \n"
        "pop {lr}                      \n"
        "tst r5, #4                    \n"     /* thread or handler mode?     */
        "ite eq                        \n"
        "mrseq r0, msp                 \n"
        "mrsne r0, psp                 \n"
        "bx lr                         \n"
    );
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
__EXHDLR_ATTR__ void HardFault_Handler(void)
{
    __asm volatile
    (
        "mov r12, lr                   \n\t"
        "bl CommonFault_Handler        \n\t"
        "bl Hard_Fault_Handler         \n\t"
    );
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
__EXHDLR_ATTR__ void MemManage_Handler(void)
{
    __asm volatile
    (
        "mov r12, lr                   \n"
        "bl CommonFault_Handler        \n"
        "bl MemManage_Fault_Handler    \n"
    );
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
__EXHDLR_ATTR__ void BusFault_Handler(void)
{
    __asm volatile
    (
        "mov r12, lr                   \n"
        "bl CommonFault_Handler        \n"
        "bl Bus_Fault_Handler          \n"
    );
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
__EXHDLR_ATTR__ void UsageFault_Handler(void)
{
    __asm volatile
    (
        "mov r12, lr                   \n"
        "bl CommonFault_Handler        \n"
        "bl Usage_Fault_Handler          \n"
    );
}

#ifdef HAL_DWT_MODULE_ENABLED
__EXHDLR_ATTR__  void DebugMon_Handler(void)
{
    __asm volatile
    (
        "mov r12, lr                   \n"
        "bl CommonFault_Handler        \n"
        "bl Debug_Monitor_Handler      \n"
    );
}
#endif /* #ifdef HAL_DWT_MODULE_ENABLED */

#endif /* #if defined(__GNUC__) */
