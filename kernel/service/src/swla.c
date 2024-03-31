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

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "swla.h"
#include "memory_attribute.h"

#ifdef MTK_SWLA_ENABLE
#include "exception_handler.h"
#include "hal.h"
#include "io_def.h"
#include "verno.h"
#include "kernel_service_config.h"

#ifdef MTK_MEMORY_EXTRACTOR_ENABLE
#include "memextract.h"
#endif /* #ifdef MTK_MEMORY_EXTRACTOR_ENABLE */

#define SWLA_USE_SYSLOG
#ifdef SWLA_USE_SYSLOG
#include "syslog.h"
log_create_module(SWLA, PRINT_LEVEL_INFO);
#define SWLA_LOG_I(fmt, args...)        LOG_I(SWLA, fmt, ##args)
#define SWLA_LOG_W(fmt, args...)        LOG_W(SWLA, fmt, ##args)
#define SWLA_LOG_E(fmt, args...)        LOG_E(SWLA, fmt, ##args)
#define SWLA_EXCEP_LOG(fmt, args...)    do { \
        char cbuf[SLA_LOCAL_BUFFER_SIZE]; \
        uint32_t xLogLen = 0; \
        xLogLen = snprintf(cbuf, SLA_LOCAL_BUFFER_SIZE, fmt, ##args); \
        log_write(cbuf, xLogLen); \
    } while(0)
#else /* #ifdef SWLA_USE_SYSLOG */
#define SWLA_LOG_I(fmt, args...)        platform_printf(fmt, ##args)
#define SWLA_LOG_W(fmt, args...)        platform_printf(fmt, ##args)
#define SWLA_LOG_E(fmt, args...)        platform_printf(fmt, ##args)
#define SWLA_EXCEP_LOG(fmt, args...)    platform_printf(fmt, ##args)
#endif /* #ifdef SWLA_USE_SYSLOG */

/**
 * @brief SWLA_OVERHEAD_MEASURE: do SWLA overhead measurment in the begining
 */
//#define SWLA_OVERHEAD_MEASURE

#ifdef SWLA_OVERHEAD_MEASURE
/**
 * @brief SWLA_OVERHEAD_MEASURE_IN_TCM: place the stack of measuring task in TCM.
 */
#define SWLA_OVERHEAD_MEASURE_IN_TCM
#ifdef SWLA_OVERHEAD_MEASURE_IN_TCM
#define SLA_TEST_TASK_STACK_SIZE (800)
ATTR_ZIDATA_IN_TCM static uint8_t g_pxSLA_Stack[SLA_TEST_TASK_STACK_SIZE];
ATTR_ZIDATA_IN_TCM StaticTask_t xTaskTCBBuffer;
#endif /* #ifdef SWLA_OVERHEAD_MEASURE_IN_TCM */
/**
 * @brief SWLA_OVERHEAD_MEASURE_BY_DWT: use DWT cycle count instead of GPT_1M.
 */
#define SWLA_OVERHEAD_MEASURE_BY_DWT
#ifdef SWLA_OVERHEAD_MEASURE_BY_DWT
#define _core_debug (*(volatile int*)((unsigned int)&CoreDebug->DEMCR))
#define _dwt_ctrl (*(volatile int*)((unsigned int)&(DWT->CTRL)))
#define _dwt_cyc (*(volatile int*)((unsigned int)&(DWT->CYCCNT)))
#define dwt_on() do{ _core_debug |= 1<<24; _dwt_ctrl |=1 ; _dwt_cyc = 0;}while(0)
#define dwt_off() do{ _dwt_ctrl &= ~1; _core_debug &= ~(1<<24); }while(0)
#define dwt_cyc() (_dwt_cyc)
#else /* #ifdef SWLA_OVERHEAD_MEASURE_BY_DWT */
#define dwt_on()
#define dwt_off()
#endif /* #ifdef SWLA_OVERHEAD_MEASURE_BY_DWT */

#include "task.h"
#include "task_def.h"
#endif /* #ifdef SWLA_OVERHEAD_MEASURE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SLA_STREAM_MODE 0xa0 /* swla data will be output by log service */
#define SLA_DUMP_MODE 0xb0  /* swla data will be dumped when exception occure, only support dump mode on IOT */
#define SLA_CORE_ID 0x0     /* only single core, cm4/cm33 core */

#define SLA_NODE_SIZE 0x8   /* SWLA node size, currently is 8byte, include [context, time stamp] */

#define MAIN_VER '1'
#define SUB_VER '0'        /* SWLA implementation on IOT */

#define SLA_INIT_PATTERN 0x414C5753
#define SLA_LOCAL_BUFFER_SIZE (128)

// for check PLL enable
#if PRODUCT_VERSION == 7933
#define WAKEUP_AO_REG           ((P_U32)0x2106000C)
#define WAKEUP_TOP_PLL_ENABLE   (0x1 << 1)
#else /* #if PRODUCT_VERSION == 7933 */
#error "SWLA not supported this chip."
#endif /* #if PRODUCT_VERSION == 7933 */

/* Private macro -------------------------------------------------------------*/
#define _DEFINE_TO_STR(a) #a
#define DEFINE_TO_STR(a) _DEFINE_TO_STR(a)

/* Private variables ---------------------------------------------------------*/
const SLA_IMAGE_HEADER gSLA_DefaultHeader = {
    MAIN_VER,
    SUB_VER,
    sizeof(SLA_IMAGE_HEADER),
    0, // main part desc len
    0, // addon desc len
    SLA_DUMP_MODE,
    SLA_CORE_ID,
    {0, 0}, //res[2]
    0, //xDumpWrapCount
    DEFINE_TO_STR(PRODUCT_VERSION),
    {0},//MTK_FW_VERSION, //todo: defined in project's makefile, prefer a global/chip level definition
    0, //xStartPosition
    0, //xCurPosition
    0, //xBufLen
    0, //xWrapCount
};

ATTR_ZIDATA_IN_TCM static uint32_t gxSLA_Init = 0;
ATTR_ZIDATA_IN_TCM static SA_CONTROL_t gxSLA_EnableFlag = SA_DISABLE;
ATTR_ZIDATA_IN_TCM static SA_NODE_t *gpxSLA_Base;
ATTR_ZIDATA_IN_TCM static uint32_t gxSLA_CurIndex;
ATTR_ZIDATA_IN_TCM static uint32_t gxSLA_MaxIndex;
ATTR_ZIDATA_IN_TCM static uint32_t gxSLA_WrapFlag;

#ifdef MTK_MEMORY_EXTRACTOR_ENABLE
// Using in exception dump to flash flow
static region_list_t gSLA_DumpToFlashRegion[3];
#endif /* #ifdef MTK_MEMORY_EXTRACTOR_ENABLE */

/* Private functions ---------------------------------------------------------*/
#ifdef MTK_SWLA_USE_SYSRAM_BUFFER
#define ATTR_ZIDATA_IN_SYSRAM_SWLA __attribute__ ((__section__(".sysram_swla_zidata")))
// Use SYSRAM as ring buffer
ATTR_ZIDATA_IN_SYSRAM_SWLA ATTR_4BYTE_ALIGN static uint8_t _g_sla_ram_buffer[SWLA_BUFFER_SIZE];
#else /* #ifdef MTK_SWLA_USE_SYSRAM_BUFFER */
// Use PSRAM as ring buffer
ATTR_ZIDATA_IN_RAM ATTR_4BYTE_ALIGN static uint8_t _g_sla_ram_buffer[SWLA_BUFFER_SIZE];
#endif /* #ifdef MTK_SWLA_USE_SYSRAM_BUFFER */

static void SLA_get_region(uint8_t **pxBase, uint32_t *pxLen)
{
    *pxBase = _g_sla_ram_buffer;
    *pxLen = (SWLA_BUFFER_SIZE / 16) * 16; // align up to 16 bytes
}

static void SLA_Init(SA_CONTROL_t xOperation)
{
    SLA_IMAGE_HEADER *pxSLA_Header = NULL;
    uint8_t *pxBase = NULL;
    uint32_t xLen;
    uint32_t xSavedMask;

    if ((xOperation != SA_ENABLE) && (xOperation != SA_DISABLE)) {
        SWLA_LOG_E("SWLA invalid operation:%d.\r\n", (unsigned int)xOperation);
        return;
    } else if ((gxSLA_EnableFlag == SA_ENABLE) && (xOperation == SA_ENABLE)) {
        SWLA_LOG_W("SWLA is already enabled.\r\n");
        return;
    } else if ((gxSLA_EnableFlag == SA_DISABLE) && (xOperation == SA_DISABLE)) {
        SWLA_LOG_W("SWLA is already disabled.\r\n");
        return;
    }

    /* get swla region location and length according to layout */
    SLA_get_region(&pxBase, &xLen);
    /**
     * check the length of swla memory, ensure the buffer can record at least the header (80B), and the 20's user data
     * that's total need at least 80 + 20 * 8 = 240Byte working buffer
     **/
    if ((pxBase == NULL) ||
        (xLen <= sizeof(SLA_IMAGE_HEADER) + sizeof(SA_NODE_t) * 20)) {
        /* the swla cannot be enabled, keep gxSLA_EnableFlag is 0 */
        SWLA_LOG_E("SWLA working memory is not enough to start SWLA.\r\n");
        return;
    }

    /* critical section start */
    hal_nvic_save_and_set_interrupt_mask(&xSavedMask);
    if (xOperation == SA_ENABLE) {
        // assign header pointer
        pxSLA_Header = (SLA_IMAGE_HEADER *)pxBase;
        /* copy SA_IMAGE_HEADER to the begin of the SWLA buffer */
        memcpy(pxBase, &gSLA_DefaultHeader, sizeof(SLA_IMAGE_HEADER));
        /* only copy the front 32byte of the firmware version */
        strncpy((char *)(((SLA_IMAGE_HEADER *)pxBase)->xFlavormName), (const char *)MTK_SDK_VERSION, sizeof(((SLA_IMAGE_HEADER *)0)->xFlavormName));

        pxSLA_Header->xStartPosition = (uint32_t)(pxBase + sizeof(SLA_IMAGE_HEADER));
        pxSLA_Header->xCurPosition = (uint32_t)(pxBase + sizeof(SLA_IMAGE_HEADER));
        pxSLA_Header->xBufLen = xLen - gSLA_DefaultHeader.xImageHeaderLen; //Raw data length
        pxSLA_Header->xWrapCount = 0; //wrap count

        /* point to the raw data area */
        gpxSLA_Base = (SA_NODE_t *)(pxBase + sizeof(SLA_IMAGE_HEADER));
        gxSLA_MaxIndex = ((xLen - gSLA_DefaultHeader.xImageHeaderLen) / sizeof(SA_NODE_t)) - 1;
        gxSLA_CurIndex = 0;
        gxSLA_WrapFlag = 0;
    }

    gxSLA_EnableFlag = xOperation;
    gxSLA_Init = (xOperation == SA_ENABLE) ? SLA_INIT_PATTERN : 0;
    /* critical section end */
    hal_nvic_restore_interrupt_mask(xSavedMask);
    if (gxSLA_EnableFlag == SA_ENABLE) {
        SWLA_LOG_I("SWLA is enabled.\r\n");
    } else {
        SWLA_LOG_I("SWLA is disabled.\r\n");
    }

    return;
}

static void SLA_MemoryCallbackInit(char *buf, unsigned int bufsize)
{
    /* add a record for exception, not show on UI */
    const uint8_t ucExceptionRec[5] = "excp";
    const uint32_t xExceptionRec = (uint32_t)(ucExceptionRec[0] | (ucExceptionRec[1] << 8) | (ucExceptionRec[2] << 16) | (ucExceptionRec[3] << 24));
    SLA_RamLogging(xExceptionRec);
}

#if 1
// Use busy loop to avoid GPT not working and causes exception/WDT REST flow hang
static void SLA_DumpBusyDelayMs(uint32_t ms)
{
    // TODO: calculate with MPU Hz.
    uint32_t count = ms * 1024 * 1024;
    do {
        // nop to prevent this function gone after compiler optimization
        asm("");
        count--;
    } while (count != 0);
}
#else /* #if 1 */
static void SLA_ExceptDelayMs(uint32_t ms)
{
    uint32_t xStart, xEnd;
    uint32_t xPast = 0;
    uint32_t xTimeoutCnt = (ms * 1000) / 30 /* us */;
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &xStart);
    while (xPast < xTimeoutCnt) {
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &xEnd);
        hal_gpt_get_duration_count(xStart, xEnd, &xPast);
    }
}
#endif /* #if 1 */
static void SLA_MemoryCallbackDump(char *buf, unsigned int bufsize)
{
    SLA_IMAGE_HEADER *pxSLA_Header = NULL;
    uint8_t *pxBase = NULL;
    uint32_t *pxCurrent, *pxEnd, *pxHeaderEnd;
    uint32_t xLen;

    /* if SWLA is not enabled, do nothing */
    if (!gxSLA_EnableFlag) {
        return;
    }

    /* get swla region location and length according to layout */
    SLA_get_region(&pxBase, &xLen);
    if (pxBase == NULL) {
        return;
    }

    SWLA_EXCEP_LOG("####SWLA enabled[0x%08lX,0x%08lX]####\r\n", (uint32_t)pxBase, (uint32_t)xLen);

    pxSLA_Header = (SLA_IMAGE_HEADER *)pxBase;
    pxHeaderEnd = (uint32_t *)(pxBase + sizeof(SLA_IMAGE_HEADER));

    /* update SWLA header */
    pxSLA_Header->xCurPosition = (gxSLA_CurIndex == 0) ?
                                 ((uint32_t)(gpxSLA_Base + gxSLA_CurIndex)) :
                                 ((uint32_t)(gpxSLA_Base + gxSLA_CurIndex - 1));
    pxSLA_Header->xWrapCount = gxSLA_WrapFlag; /* wrap count */

    /* SWLA buffer valid length */
    if (gxSLA_WrapFlag) {
        pxEnd = (uint32_t *)(pxBase + xLen);
        pxSLA_Header->xBufLen = xLen - gSLA_DefaultHeader.xImageHeaderLen; //Raw data length
    } else {
        pxEnd = (uint32_t *)(gpxSLA_Base + gxSLA_CurIndex);
        pxSLA_Header->xBufLen = gxSLA_CurIndex * sizeof(SA_NODE_t); /* raw data length */
    }

    /* output swla region content */
    for (pxCurrent = (uint32_t *)pxBase; pxCurrent < pxEnd; pxCurrent += 4) {
        if (*(pxCurrent + 0) == 0 &&
            *(pxCurrent + 1) == 0 &&
            *(pxCurrent + 2) == 0 &&
            *(pxCurrent + 3) == 0 &&
            pxCurrent > pxHeaderEnd
           ) {
            continue;
        }

        SWLA_EXCEP_LOG("0x%08lx: %08lx %08lx %08lx %08lx\n\r",
                        (uint32_t)pxCurrent,
                        *(pxCurrent + 0),
                        *(pxCurrent + 1),
                        *(pxCurrent + 2),
                        *(pxCurrent + 3));
        /* add delay to avoid uart log corruption in putty or teraterm */
        SLA_DumpBusyDelayMs(1);
#ifdef HAL_WDT_MODULE_ENABLED
        /* kick WDT every 4K */
        if ((((uint32_t)pxCurrent) & 0xFFFFFFF0) % 0x1000 == 0)
            hal_wdt_feed(HAL_WDT_FEED_MAGIC);
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
    }

    SWLA_EXCEP_LOG("####SWLA end####\n\r");
}

#ifdef MTK_MEMORY_EXTRACTOR_ENABLE
static void SLA_DumpToFlash(region_list_t **region_list, int *num, uint32_t *total_size)
{
    SLA_IMAGE_HEADER *pxSLA_Header = NULL;
    uint8_t *pxBase = NULL;
    uint32_t xLen;
    uint32_t xDumpRegionRemainSize = ((SWLA_LENGTH / 16) * 16); //16 bytes alignment
    uint32_t xRegionCount = 0;

    /* get swla region location and length according to layout */
    SLA_get_region(&pxBase, &xLen);
    /* if SWLA is not enabled, do nothing */
    if (gxSLA_EnableFlag && pxBase != NULL) {
        pxSLA_Header = (SLA_IMAGE_HEADER *)pxBase;
        /* update SWLA header */
        pxSLA_Header->xCurPosition = (gxSLA_CurIndex == 0) ?
                                     ((uint32_t)(gpxSLA_Base + gxSLA_CurIndex)) :
                                     ((uint32_t)(gpxSLA_Base + gxSLA_CurIndex - 1));
        // Save wrap count to another field for debugging usage
        pxSLA_Header->xDumpWrapCount = gxSLA_WrapFlag;
        // Already order the data in dump mode, wrap count set to 0 for parsing script
        pxSLA_Header->xWrapCount = 0;

        // header region
        gSLA_DumpToFlashRegion[xRegionCount].start_addr = (char *)pxBase;
        gSLA_DumpToFlashRegion[xRegionCount].len = gSLA_DefaultHeader.xImageHeaderLen;
        xDumpRegionRemainSize -= gSLA_DumpToFlashRegion[xRegionCount].len;
        SWLA_EXCEP_LOG("[SWLA] Dump header: 0x%08lX, %lu\r\n",
                        (uint32_t)gSLA_DumpToFlashRegion[xRegionCount].start_addr,
                        gSLA_DumpToFlashRegion[xRegionCount].len);
        xRegionCount++;

        if ((gxSLA_CurIndex * sizeof(SA_NODE_t)) >= xDumpRegionRemainSize) {
            uint32_t xAvaliableCount = xDumpRegionRemainSize / sizeof(SA_NODE_t);
            uint8_t *pxStartCopyAddr = (uint8_t *)(gpxSLA_Base + (gxSLA_CurIndex - xAvaliableCount));
            gSLA_DumpToFlashRegion[xRegionCount].start_addr = (char *)pxStartCopyAddr;
            gSLA_DumpToFlashRegion[xRegionCount].len = xAvaliableCount * sizeof(SA_NODE_t);
            SWLA_EXCEP_LOG("[SWLA] Dump s1: 0x%08lX, %lu\r\n",
                            (uint32_t)gSLA_DumpToFlashRegion[xRegionCount].start_addr,
                            gSLA_DumpToFlashRegion[xRegionCount].len);
            // calculate remain size
            xDumpRegionRemainSize -= gSLA_DumpToFlashRegion[xRegionCount].len;
            // update swla content size
            pxSLA_Header->xBufLen = gSLA_DumpToFlashRegion[xRegionCount].len;
            xRegionCount++;
        } else if (gxSLA_WrapFlag) {
            // small than xDumpRegionRemainSize and buffer with wrap
            uint8_t *pxStartCopyAddr2 = (uint8_t *)(gpxSLA_Base);
            uint32_t xCopySize2 = gxSLA_CurIndex * sizeof(SA_NODE_t);
            xDumpRegionRemainSize -= xCopySize2;
            uint32_t xAvaliableCount = xDumpRegionRemainSize / sizeof(SA_NODE_t);
            uint8_t *pxStartCopyAddr1 = (uint8_t *)(gpxSLA_Base + (gxSLA_MaxIndex - xAvaliableCount + 1));
            uint32_t xCopySize1 = xAvaliableCount * sizeof(SA_NODE_t);
            xDumpRegionRemainSize -= xCopySize2;
            gSLA_DumpToFlashRegion[xRegionCount].start_addr = (char *)pxStartCopyAddr1;
            gSLA_DumpToFlashRegion[xRegionCount].len = xCopySize1;
            SWLA_EXCEP_LOG("[SWLA] Dump s2-1: 0x%08lX, %lu\r\n",
                            (uint32_t)gSLA_DumpToFlashRegion[xRegionCount].start_addr,
                            gSLA_DumpToFlashRegion[xRegionCount].len);
            xRegionCount++;
            gSLA_DumpToFlashRegion[xRegionCount].start_addr = (char *)pxStartCopyAddr2;
            gSLA_DumpToFlashRegion[xRegionCount].len = xCopySize2;
            SWLA_EXCEP_LOG("[SWLA] Dump s2-2: 0x%08lX, %lu\r\n",
                            (uint32_t)gSLA_DumpToFlashRegion[xRegionCount].start_addr,
                            gSLA_DumpToFlashRegion[xRegionCount].len);
            xRegionCount++;
            // update swla content size
            pxSLA_Header->xBufLen = xCopySize1 + xCopySize2;
        } else {
            // small than xDumpRegionRemainSize and buffer without wrap
            gSLA_DumpToFlashRegion[xRegionCount].start_addr = (char *)gpxSLA_Base;
            gSLA_DumpToFlashRegion[xRegionCount].len = gxSLA_CurIndex * sizeof(SA_NODE_t);
            SWLA_EXCEP_LOG("[SWLA] Dump s3: 0x%08lX, %lu\r\n",
                            (uint32_t)gSLA_DumpToFlashRegion[xRegionCount].start_addr,
                            gSLA_DumpToFlashRegion[xRegionCount].len);
            xDumpRegionRemainSize -= gSLA_DumpToFlashRegion[xRegionCount].len;
            // update swla content size
            pxSLA_Header->xBufLen = gSLA_DumpToFlashRegion[xRegionCount].len;
            xRegionCount++;
        }
    } else {
        // let total_size = 0
        xDumpRegionRemainSize = SWLA_LENGTH;
        xRegionCount = 0;
    } /* if(gxSLA_EnableFlag && pxBase != NULL) */

    // assign return values
    *region_list = gSLA_DumpToFlashRegion;
    *num = xRegionCount;
    *total_size = SWLA_LENGTH - xDumpRegionRemainSize;
}
#endif /* #ifdef MTK_MEMORY_EXTRACTOR_ENABLE */

static void SLA_MemoryDumpInit(void)
{
#ifndef MTK_DAPLINK_ENABLED
    uint32_t ret;
    exception_config_type callback_config;

    callback_config.init_cb = SLA_MemoryCallbackInit;
    callback_config.dump_cb = SLA_MemoryCallbackDump;

    ret = exception_register_callbacks(&callback_config);
    if (!ret) {
        configASSERT(0);
    }
#endif /* #ifndef MTK_DAPLINK_ENABLED */

#ifdef MTK_MEMORY_EXTRACTOR_ENABLE
    {
        // Register dump to flash callback
        memextract_entry xDumptToFlashEntry;
        xDumptToFlashEntry.cb = SLA_DumpToFlash;
        xDumptToFlashEntry.type = MEMORY_EXTRACT_SWLA;
        memextract_register_callbacks(&xDumptToFlashEntry);
    }
#endif /* #ifdef MTK_MEMORY_EXTRACTOR_ENABLE */
}

#ifdef SWLA_OVERHEAD_MEASURE

#define SWLA_OVERHEAD_MEASURE_CNT (100)

#define _SWLA_TAG2STR(tag) #tag
#define SWLA_TAG2STR(tag)  _SWLA_TAG2STR(tag)
#define SWLA_MEASURE_VAR(tag) uint32_t  tag##_max = 0; uint32_t  tag##_min = 0xffffffff; uint32_t tag##_avg = 0;
#define SWLA_MEASURE_PRINT(tag, cnt) do{ \
        SWLA_LOG_I("[SWLA] %s:\r\n", SWLA_TAG2STR(tag)); \
        SWLA_LOG_I("[SWLA] Max cycle: %lu\r\n", tag##_max); \
        SWLA_LOG_I("[SWLA] Min cycle: %lu\r\n", tag##_min); \
        SWLA_LOG_I("[SWLA] Avg cycle: %lu\r\n", tag##_avg / cnt); \
    } while(0)

#define SWLA_MEASURE_STATICAL(tag) do{ \
        tag##_avg += xOverHeadTime; \
        if(xOverHeadTime > tag##_max) tag##_max = xOverHeadTime; \
        if(xOverHeadTime < tag##_min) tag##_min = xOverHeadTime; \
    } while(0)

#ifndef SWLA_OVERHEAD_MEASURE_BY_DWT
#define SWLA_MEASURE(tag, func) do{ \
        hal_nvic_save_and_set_interrupt_mask(&xSavedMask); \
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &xTimeStart); \
        (func); \
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &xTimeEnd); \
        hal_nvic_restore_interrupt_mask(xSavedMask); \
        hal_gpt_get_duration_count(xTimeStart, xTimeEnd, &xOverHeadTime); \
        SWLA_MEASURE_STATICAL(tag); \
    } while(0)
#else /* #ifndef SWLA_OVERHEAD_MEASURE_BY_DWT */
#define SWLA_MEASURE(tag, func) do{ \
        hal_nvic_save_and_set_interrupt_mask(&xSavedMask); \
        xTimeStart = dwt_cyc(); \
        (func); \
        xTimeEnd = dwt_cyc(); \
        hal_nvic_restore_interrupt_mask(xSavedMask); \
        xOverHeadTime = (xTimeEnd > xTimeStart)? (xTimeEnd - xTimeStart):0; \
        SWLA_MEASURE_STATICAL(tag); \
    } while(0)
#endif /* #ifndef SWLA_OVERHEAD_MEASURE_BY_DWT */

static void SLA_ProfileTask(void *parameters)
{
    uint32_t xSavedMask;
    uint32_t xTimeStart, xTimeEnd, xOverHeadTime;
    const uint8_t ucMeasure[5] = "meas";
    const uint32_t xMeas = (uint32_t)(ucMeasure[0] | (ucMeasure[1] << 8) | (ucMeasure[2] << 16) | (ucMeasure[3] << 24));
    int i;
    // Block for 100ms.
    const TickType_t xDelay = 100 / portTICK_PERIOD_MS;
    SWLA_MEASURE_VAR(Task);
    SWLA_MEASURE_VAR(IRQ_S);
    SWLA_MEASURE_VAR(IRQ_E);
    SWLA_MEASURE_VAR(LABEL);

    SWLA_LOG_I("[SWLA] Profiling task start\r\n");
    for (i = 0; i < SWLA_OVERHEAD_MEASURE_CNT; ++i) {
        SWLA_LOG_I("[SWLA] %d/%d\r\n", i, SWLA_OVERHEAD_MEASURE_CNT);
        dwt_on();
        SWLA_MEASURE(Task, SLA_RamLogging(xMeas));
        SWLA_MEASURE(IRQ_S, SLA_RamLogging((uint32_t)(IRQ_START | 0xFF)));
        SWLA_MEASURE(IRQ_E, SLA_RamLogging((uint32_t)(IRQ_END)));
        SWLA_MEASURE(LABEL, SLA_CustomLogging("lab", SA_LABEL));
        dwt_off();
        vTaskDelay(xDelay);
    }
    SWLA_MEASURE_PRINT(Task, SWLA_OVERHEAD_MEASURE_CNT);
    SWLA_MEASURE_PRINT(IRQ_S, SWLA_OVERHEAD_MEASURE_CNT);
    SWLA_MEASURE_PRINT(IRQ_E, SWLA_OVERHEAD_MEASURE_CNT);
    SWLA_MEASURE_PRINT(LABEL, SWLA_OVERHEAD_MEASURE_CNT);
    SWLA_LOG_I("[SWLA] Profiling task exit\r\n");
    vTaskDelete(NULL);
}

void SLA_StartProfileTask(void)
{
    BaseType_t ret = pdPASS;
    TaskHandle_t init_handler;

#ifdef SWLA_OVERHEAD_MEASURE_IN_TCM // stack in TCM
    StaticTask_t *pxTaskTCBBuffer = (StaticTask_t *) &xTaskTCBBuffer;
    ret = xTaskCreateStatic(SLA_ProfileTask, "slap"
                            , SLA_TEST_TASK_STACK_SIZE, NULL, TASK_PRIORITY_LOW,
                            g_pxSLA_Stack, pxTaskTCBBuffer);
#else /* #ifdef SWLA_OVERHEAD_MEASURE_IN_TCM // stack in TCM */
    ret = xTaskCreate(SLA_ProfileTask, "slap"
                      , 800, NULL
                      , TASK_PRIORITY_LOW, &init_handler);
#endif /* #ifdef SWLA_OVERHEAD_MEASURE_IN_TCM // stack in TCM */
    if (ret != pdPASS)
        SWLA_LOG_E("[SWLA] Profiling task create failed\r\n");
    else
        SWLA_LOG_I("[SWLA] Profiling task create success\r\n");
}

#endif /* #ifdef SWLA_OVERHEAD_MEASURE */

/* Public functions ---------------------------------------------------------*/

/**
 * @brief  swla enable
 * @param[in]  none.
 * @return none
 */
void SLA_Enable(void)
{
    //uint32_t xSleepMode = 0;
    uint32_t xTimerStamp;

    /* register callback in exception handling flow to dump SWLA region */
    SLA_MemoryDumpInit();

    /* dummy read to make gpt is enabled */
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &xTimerStamp);

    /* Init buffer and global variables */
    SLA_Init(SA_ENABLE);

#ifdef SWLA_OVERHEAD_MEASURE
    /* dummy read to make gpt is enabled */
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &xTimerStamp);
    SLA_StartProfileTask();
#endif /* #ifdef SWLA_OVERHEAD_MEASURE */
}

/**
 * @brief  swla logging
 * @param[in]       *Context points to the input buffer, include swla label and action
 * @return none
 */
ATTR_TEXT_IN_TCM void SLA_RamLogging(uint32_t xContext)
{
    uint32_t xTimerStamp, xSavedMask;

    /* if SWLA is not enabled, do nothing */
    hal_nvic_save_and_set_interrupt_mask(&xSavedMask);
    if (!gxSLA_EnableFlag) {
        hal_nvic_restore_interrupt_mask(xSavedMask);
        return;
    }

    /* get time stamp */
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &xTimerStamp);

    gpxSLA_Base[gxSLA_CurIndex].xContext = xContext;
    gpxSLA_Base[gxSLA_CurIndex].xTimeStamp = xTimerStamp;

    if (gxSLA_CurIndex == gxSLA_MaxIndex) {
        gxSLA_WrapFlag ++;
        gxSLA_CurIndex = 0;
    } else {
        gxSLA_CurIndex++;
    }

    hal_nvic_restore_interrupt_mask(xSavedMask);
}

/**
 * @brief customer swla logging
 * @param[in]       *customLabel points to the input buffer
 * @param[in]       saAction swla operation, include start, stop and one-shot mode
 * @return none
 */
ATTR_TEXT_IN_TCM void SLA_CustomLogging(const char *pxCustomLabel, SA_ACTION_t xAction)
{
    uint32_t xTimerStamp;
    uint32_t xSavedMask, xContext;

    /* if SWLA is not enabled, do nothing */
    hal_nvic_save_and_set_interrupt_mask(&xSavedMask);
    if (!gxSLA_EnableFlag) {
        hal_nvic_restore_interrupt_mask(xSavedMask);
        return;
    }

    /* check action */
    if ((xAction != SA_START) && (xAction != SA_STOP) && (xAction != SA_LABEL)) {
        SWLA_LOG_E("[parameter error]invalid xAction:%d.\r\n", (unsigned int)xAction);
        configASSERT(0);
    }

    xContext = ((uint8_t)xAction |
                ((uint8_t)pxCustomLabel[0] << 8)  |
                ((uint8_t)pxCustomLabel[1] << 16) |
                ((uint8_t)pxCustomLabel[2] << 24));

    /* get time stamp */
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &xTimerStamp);

    gpxSLA_Base[gxSLA_CurIndex].xContext = xContext;
    gpxSLA_Base[gxSLA_CurIndex].xTimeStamp = xTimerStamp;

    if (gxSLA_CurIndex == gxSLA_MaxIndex) {
        gxSLA_WrapFlag ++;
        gxSLA_CurIndex = 0;
    } else {
        gxSLA_CurIndex++;
    }

    hal_nvic_restore_interrupt_mask(xSavedMask);
}

/**
 * @brief swla control
 * @param[in]  xOperation enable or disable swla
 * @return[out] ret return control ok or not
 */
void SLA_Control(SA_CONTROL_t xOperation)
{
    /* Init buffer and global variables */
    SLA_Init(xOperation);
}

/**
 * @brief swla relevantoperations before enter deep sleep mode
 */
void SLA_EnterDeepSleep(void *data)
{
#ifndef MTK_SWLA_USE_SYSRAM_BUFFER
    uint32_t xSavedMask;
    /* critical section start */
    hal_nvic_save_and_set_interrupt_mask(&xSavedMask);
    if (gxSLA_Init == SLA_INIT_PATTERN) {
        gxSLA_EnableFlag = SA_DISABLE;
    }
    hal_nvic_restore_interrupt_mask(xSavedMask);
#endif /* #ifndef MTK_SWLA_USE_SYSRAM_BUFFER */
}

/**
 * @brief swla relevant operations after exit deep sleep mode
 */
void SLA_ExitDeepSleep(void *data)
{
#ifndef MTK_SWLA_USE_SYSRAM_BUFFER
    uint32_t xSavedMask;
    /* critical section start */
    hal_nvic_save_and_set_interrupt_mask(&xSavedMask);
    if (gxSLA_Init == SLA_INIT_PATTERN) {
        gxSLA_EnableFlag = SA_ENABLE;
    }
    hal_nvic_restore_interrupt_mask(xSavedMask);
#endif /* #ifndef MTK_SWLA_USE_SYSRAM_BUFFER */
}
#else /* #ifdef MTK_SWLA_ENABLE */
/**
 * Some of libraries release to customer enabled SWLA.
 * For these libraries swla.c must be built and provides dummy functions.
 **/
void SLA_RamLogging(uint32_t xContext)
{

}

void SLA_CustomLogging(const char *pxCustomLabel, SA_ACTION_t xAction)
{

}

void SLA_EnterDeepSleep(void *data)
{

}

void SLA_ExitDeepSleep(void *data)
{

}
#endif /* #ifdef MTK_SWLA_ENABLE */

#if defined(MTK_SWLA_ENABLE) && defined(MTK_SWLA_WDT_RESET_TRACE)

#if !defined(MTK_SWLA_USE_SYSRAM_BUFFER) || !defined(MTK_SYSTEM_HANG_CHECK_ENABLE)
#error "MTK_SWLA_WDT_RESET_TRACE is only applied if MTK_SWLA_USE_SYSRAM_BUFFER and MTK_SYSTEM_HANG_CHECK_ENABLE are enabled."
#endif /* #if !defined(MTK_SWLA_USE_SYSRAM_BUFFER) || !defined(MTK_SYSTEM_HANG_CHECK_ENABLE) */

#define SWLA_EARLY_LOG(fmt, args...)    do { \
        char cbuf[SLA_LOCAL_BUFFER_SIZE]; \
        uint32_t xLogLen = 0; \
        xLogLen = snprintf(cbuf, SLA_LOCAL_BUFFER_SIZE, fmt, ##args); \
        output_log(cbuf, xLogLen); \
    } while(0)

static void output_log(const char *buf, uint32_t xSize)
{
    int left;
    int bytes_written;
    left = xSize;
    while (left > 0) {
        bytes_written = hal_uart_send_polling(CONSOLE_UART, (const uint8_t *)buf, left);
        left -= bytes_written;
        buf += bytes_written;
    }
}

void SLA_WDTResetDump(void)
{
    SLA_IMAGE_HEADER *pxSLA_Header = NULL;
    uint8_t *pxBase = NULL;
    uint32_t xLen;
    uint32_t *pxCurrent, *pxEnd;
    uint8_t ucDelayFlag = 0;
    /* get swla region location and length according to layout */
    SLA_get_region(&pxBase, &xLen);
    pxSLA_Header = (SLA_IMAGE_HEADER *)pxBase;
    /* Cannot use WDT STA to determine reset reason due to WDT is also used in BROM and BL */
    if (gxSLA_Init == SLA_INIT_PATTERN &&
        pxSLA_Header->xMainVer == '1' &&
        pxSLA_Header->xSubVer == '0' &&
        pxSLA_Header->xImageHeaderLen == sizeof(SLA_IMAGE_HEADER)) {

        SWLA_EARLY_LOG("** Detected WDT HW reset, dump SWLA buffer **\r\n");

        if (HAL_REG_32(WAKEUP_AO_REG) & WAKEUP_TOP_PLL_ENABLE) {
            ucDelayFlag = 1;
        }

        SWLA_EARLY_LOG("####SWLA enabled[0x%08lX,0x%08lX]####\r\n", (uint32_t)pxBase, (uint32_t)xLen);
        pxSLA_Header = (SLA_IMAGE_HEADER *)pxBase;

        /* update SWLA header */
        pxSLA_Header->xCurPosition = (gxSLA_CurIndex == 0) ?
                                     ((uint32_t)(gpxSLA_Base + gxSLA_CurIndex)) :
                                     ((uint32_t)(gpxSLA_Base + gxSLA_CurIndex - 1));
        pxSLA_Header->xWrapCount = gxSLA_WrapFlag; /* wrap count */

        if (gxSLA_WrapFlag) {
            pxEnd = (uint32_t *)(pxBase + xLen);
            pxSLA_Header->xBufLen = xLen - gSLA_DefaultHeader.xImageHeaderLen; //Raw data length
        } else {
            pxEnd = (uint32_t *)(gpxSLA_Base + gxSLA_CurIndex);
            pxSLA_Header->xBufLen = gxSLA_CurIndex * sizeof(SA_NODE_t); /* raw data length */
        }
        for (pxCurrent = (uint32_t *)pxBase; pxCurrent < pxEnd; pxCurrent += 4) {
            SWLA_EARLY_LOG("0x%08lx: %08lx %08lx %08lx %08lx\n\r",
                     (uint32_t)pxCurrent,
                     *(pxCurrent + 0),
                     *(pxCurrent + 1),
                     *(pxCurrent + 2),
                     *(pxCurrent + 3));
            /* add delay to avoid uart log corruption in putty or teraterm */
            if (ucDelayFlag) {
                // If PLL is not enabled in the BROM, skip this delay
                SLA_DumpBusyDelayMs(1);
            }
        }
        SWLA_EARLY_LOG("####SWLA end\r\n");
    }
}
#else /* #if defined(MTK_SWLA_ENABLE) && defined(MTK_SWLA_WDT_RESET_TRACE) */
void SLA_WDTResetDump(void)
{
    // empty function for mt7933_startup.s
}
#endif /* #if defined(MTK_SWLA_ENABLE) && defined(MTK_SWLA_WDT_RESET_TRACE) */

