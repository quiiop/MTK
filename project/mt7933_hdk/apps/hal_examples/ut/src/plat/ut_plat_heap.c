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

#if defined(UT_PLATFORM_ENABLE) && defined (UT_PLATFORM_HEAP_ENABLE)
#include "FreeRTOS.h"
#include "task.h"
#include "memory_attribute.h"

#if defined(MTK_OS_HEAP_EXTEND) && (MTK_OS_HEAP_EXTEND == HEAP_EXTEND_MULTI)

#define utAPP_HEAP_SIZE                     ((size_t)(20 * 1024))

ATTR_ZIDATA_IN_RAM static uint8_t  appUtHeap[ utAPP_HEAP_SIZE ];

HeapRegion_t  appUtHeapRegion = { appUtHeap, utAPP_HEAP_SIZE, (uint8_t *)"utApp"};

BaseType_t    appUtRegionId = 0;

#define UT_MALLOC(size)                 pvPortMallocExt(appUtRegionId, size)
#define UT_FREE(addr)                   vPortFreeExt(appUtRegionId, addr)

#endif /* #if defined(MTK_OS_HEAP_EXTEND) && (MTK_OS_HEAP_EXTEND == HEAP_EXTEND_MULTI) */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if defined(MTK_OS_HEAP_EXTEND) && (MTK_OS_HEAP_EXTEND == HEAP_EXTEND_MULTI)
/* Basic Allocate/Free Test */
ut_status_t heap_multi_tc1(void)
{
    char *pc   = NULL;
    int   msize = 1024;

    printf("\r\n<<TestCase - %s>>\r\n", __FUNCTION__);

    pc = (char *)SYS_MALLOC(msize);
    strcpy(pc, "UT(strcpy): System Heap Access - OK\r\n");
    printf("pc(%p - %lx):%s\r\n", pc, HAL_CACHE_VIRTUAL_TO_PHYSICAL((uint32_t)pc), pc);
    SYS_FREE(pc);

    // Allocate from SYSTEM Region (Cache)
    pc = SYS_MALLOC(msize);
    strcpy(pc, "( SYS_MALLOC )");
    printf("System Alloc (C)    : %p %s ...", pc, pc);
    SYS_FREE(pc);
    printf("Free (Done)\r\n");

    // Allocate from SYSTEM Region (Non-Cache)
    pc = SYS_MALLOC_NC(msize);
    strcpy(pc, "( SYS_MALLOC_NC )");
    printf("System Alloc (NC)   : %p %s ...", pc, pc);
    SYS_FREE_NC(pc);
    printf("Free (Done)\r\n");

    // Allocate from Platform Region (Cache)
    pc = PLAT_MALLOC(msize);
    strcpy(pc, "( PLAT_MALLOC )");
    printf("Platform Alloc (C)  : %p %s ...", pc, pc);
    PLAT_FREE(pc);
    printf("Free (Done)\r\n");

    // Allocate from Platform Region (Non-Cache)
    pc = PLAT_MALLOC_NC(msize);
    strcpy(pc, "( PLAT_MALLOC_NC )");
    printf("Platform Alloc (NC) : %p %s ...", pc, pc);
    PLAT_FREE_NC(pc);
    printf("Free (Done)\r\n");

    // Allocate from default API
    pc = malloc(msize);
    strcpy(pc, "( malloc )");
    printf("malloc Alloc        : %p %s ...", pc, pc);
    free(pc);
    printf("Free (Done)\r\n");

    return UT_STATUS_OK;
}


/* Continue Allocate & Free Test */
ut_status_t heap_multi_tc2(void)
{
    char *pc1, *pc2, *pc3, *pc4, *pc5;
    int   msize = 1024;

    pc1 = pc2 = pc3 = pc4 = pc5 = NULL;

    printf("\r\n<<TestCase - %s>>\r\n", __FUNCTION__);

    // Allocate from SYSTEM Region (Cache)
    pc1 = SYS_MALLOC(msize);
    printf("System Alloc (C)    : %p ...\r\n", pc1);

    // Allocate from SYSTEM Region (Non-Cache)
    pc2 = SYS_MALLOC_NC(msize);
    printf("System Alloc (NC)   : %p ...\r\n", pc2);

    // Allocate from Platform Region (Cache)
    pc3 = PLAT_MALLOC(msize);
    printf("Platform Alloc (C)  : %p ...\r\n", pc3);

    // Allocate from Platform Region (Non-Cache)
    pc4 = PLAT_MALLOC_NC(msize);
    printf("Platform Alloc (NC) : %p ...\r\n", pc4);

    // Allocate from default API
    pc5 = malloc(msize);
    printf("malloc Alloc        : %p ...\r\n", pc5);

    // Free
    SYS_FREE(pc1);
    printf("System Free (C)(Done)\r\n");

    SYS_FREE_NC(pc2);
    printf("System Free (NC)(Done)\r\n");

    PLAT_FREE(pc3);
    printf("Platform Free (C)(Done)\r\n");

    PLAT_FREE_NC(pc4);
    printf("Platform Free (NC)(Done)\r\n");

    free(pc5);
    printf("free (Done)\r\n");

    return UT_STATUS_OK;
}


ut_status_t heap_multi_tc3(void)
{
    char *pc = NULL;
    int   msize = 1024;

    printf("\r\n<<TestCase - %s>>\r\n", __FUNCTION__);

    // Allocate from SYSTEM Region (Cache)
    pc = SYS_MALLOC(msize);
    printf("System Alloc (C)    : %p\r\n", pc);

    printf("Try Platform Free...");
    PLAT_FREE(pc);
    printf("System Free...");
    SYS_FREE(pc);
    printf("Free (Done)\r\n");


    // Allocate from Platform Region (Cache)
    pc = PLAT_MALLOC(msize);
    printf("Platform Alloc (C)  : %p\r\n", pc);

    printf("Try System Free...");
    SYS_FREE(pc);
    printf("Platform Free...");
    PLAT_FREE(pc);
    printf("Free (Done)\r\n");

    return UT_STATUS_OK;
}

ut_status_t heap_multi_tc4(void)
{
    char *pc = NULL;
    int   msize = 1024;

    printf("\r\n<<TestCase - %s>>\r\n", __FUNCTION__);

    // Allocate from SYSTEM Region (Cache)
    pc = SYS_MALLOC(msize);
    printf("System Alloc (C)    : %p\r\n", pc);

    printf("\tTry System (NC) Free...\r\n");
    SYS_FREE_NC(pc);  //it will cause assert - configASSERT(pdFALSE == hal_cache_is_cacheable(xAddr)); , ignore this case first
    printf("Free (Done)\r\n");


    // Allocate from Platform Region (Cache)
    pc = PLAT_MALLOC(msize);
    printf("Platform Alloc (C)  : %p ...", pc);

    printf("\tTry Platform (NC) Free...\r\n");
    PLAT_FREE_NC(pc);
    printf("Free (Done)\r\n");

    return UT_STATUS_OK;
}

//User/APP
ut_status_t heap_multi_tc5(void)
{
    char *pc = NULL;
    int   msize = 1024;

    printf("\r\n<<TestCase - %s>>\r\n", __FUNCTION__);

    appUtRegionId = vPortRegisterHeapRegions(&appUtHeapRegion);
    if (!appUtRegionId) { //Id:0 is belong to system region
        printf("User/App Heap Register Fail\r\n");
        return UT_STATUS_ERROR;
    }
    printf("UT Heap Region Registerd - ID(%lu)\r\n", appUtRegionId);

    // Memory Allocate/Free With appUtRegionId
    printf("Try Allocate with ID(%lu)...\r\n", appUtRegionId);
    pc = pvPortMallocExt(appUtRegionId, msize);
    printf("pc(%p - %lx):%s\r\n", pc, HAL_CACHE_VIRTUAL_TO_PHYSICAL((uint32_t)pc), pc);

    printf("Try Free with ID(%lu)...\r\n", appUtRegionId);
    vPortFreeExt(appUtRegionId, pc);
    printf("Free Done.\r\n");

    // Memory Allocate/Free With Pre-Define
    printf("Try Allocate Pre-Define...\r\n");
    pc = UT_MALLOC(msize);
    printf("pc(%p - %lx):%s\r\n", pc, HAL_CACHE_VIRTUAL_TO_PHYSICAL((uint32_t)pc), pc);

    printf("Try Free Pre-Define...\r\n");
    UT_FREE(pc);
    printf("Free Done.\r\n");

    return UT_STATUS_OK;
}

//Error Test
ut_status_t heap_multi_tc6(void)
{
    char *pc = NULL;
    int   msize = 1024 * 1024;

    printf("\r\n<<TestCase - %s>>\r\n", __FUNCTION__);

    printf("Try Over-Size Allocate...\r\n");
    pc = SYS_MALLOC(msize);
    printf("pc(%p)\r\n", pc);
    //printf("pc(%p - %lx):%s\r\n", pc,HAL_CACHE_VIRTUAL_TO_PHYSICAL((uint32_t)pc), pc);

    return UT_STATUS_OK;
}

ut_status_t plat_heap_multi(void)
{
    ut_status_t ret = UT_STATUS_OK;

    ret = heap_multi_tc1();

    ret = heap_multi_tc2();

    ret = heap_multi_tc3();

    //ret = heap_multi_tc4();

    ret = heap_multi_tc5();

    ret = heap_multi_tc6();

    return ret;
}
#endif /* #if defined(MTK_OS_HEAP_EXTEND) && (MTK_OS_HEAP_EXTEND == HEAP_EXTEND_MULTI) */

ut_status_t ut_plat_heap(void)
{
    printf("Heap UT - Start\r\n");

#if defined(MTK_OS_HEAP_EXTEND) && (MTK_OS_HEAP_EXTEND == HEAP_EXTEND_MULTI)
    plat_heap_multi();
#endif /* #if defined(MTK_OS_HEAP_EXTEND) && (MTK_OS_HEAP_EXTEND == HEAP_EXTEND_MULTI) */

    printf("Heap UT - End\r\n");

    return UT_STATUS_OK;
}

#endif /* #if defined(UT_PLATFORM_ENABLE) && defined (UT_PLATFORM_HEAP_ENABLE) */
