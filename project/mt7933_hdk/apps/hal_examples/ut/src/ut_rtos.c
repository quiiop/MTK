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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ut.h"


#ifdef UT_RTOS_ENABLE
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#define UT_MEM_SIZE  (1024)

/* Task priorities. */
#define helloTask_PRIORITY (configMAX_PRIORITIES - 1)

// Software timer callback.
#define SW_TIMER_PERIOD_MS (1000 / portTICK_PERIOD_MS)
#define UT_SW_TIMER_COUNT   10


static int rtos_task_done = 0;

static int sw_timer_cnt = 0;

#ifdef UT_POSIX_ENABLE
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#include "FreeRTOS_POSIX/pthread_local_storage.h"

pthread_key_t key;
#endif /* #ifdef UT_POSIX_ENABLE */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
int ut_rtos_mem(void)
{
    char  memstr[] = "RTOS_MEM_UT";
    char *pmem_vsysram;
    char *pmem_sysram;

    printf("\r\n<<Memory>>\r\n");

    pmem_vsysram = (char *)pvPortMalloc(UT_MEM_SIZE);
    if (!pmem_vsysram) {
        printf("VSYSRAM allocate fail!\r\n");
        return FALSE;
    }

    if (((uint32_t)pmem_vsysram < (uint32_t)VSYSRAM_BASE) || ((uint32_t)pmem_vsysram >= ((uint32_t)VSYSRAM_BASE + VSYSRAM_LENGTH))) {
        printf("[VSYSRAM]: Test Fail - pmem(%p)\r\n", pmem_vsysram);
        return FALSE;
    } else {
        printf("[VSYSRAM]: Test (pmem:%p) - ", pmem_vsysram);
        memcpy(pmem_vsysram, memstr, sizeof(memstr));
        vPortFree(pmem_vsysram);
        printf("Pass\r\n");
    }

    pmem_sysram = (char *)pvPortMallocNC(UT_MEM_SIZE);
    if (!pmem_sysram) {
        printf("SYSRAM allocate fail!\r\n");
        return FALSE;
    }

    if (((uint32_t)pmem_sysram < SYSRAM_BASE) || ((uint32_t)pmem_sysram >= (SYSRAM_BASE + SYSRAM_LENGTH))) {
        printf("[SYSRAM]: Test Fail - pmem(%p)\r\n", pmem_sysram);
        return FALSE;
    } else {
        printf("[VSYSRAM]: Test (pmem:%p) - ", pmem_sysram);
        memcpy(pmem_sysram, memstr, sizeof(memstr));
        vPortFreeNC(pmem_sysram);
        printf("Pass\r\n");

    }

    return TRUE;
}

static void ut_rtos_task_hello(void *pvParameters)
{
    rtos_task_done = 1;
    printf("Hello Done (%d)...", rtos_task_done);

    vTaskDelete(NULL);
}

int ut_rtos_task(void)
{
    int idx = 0;

    printf("\r\n<<Task>>\r\n");
    if (xTaskCreate(ut_rtos_task_hello, "utHelloTask", configMINIMAL_STACK_SIZE + 10, NULL, helloTask_PRIORITY, NULL) != pdPASS) {
        printf("Task creation failed!.\r\n");
        return FALSE;
    }

    for (idx = 0; idx < 10; idx++) {
        if (rtos_task_done)
            break;
        vTaskDelay(100);
    }

    if (rtos_task_done) {   // Pass
        printf("Pass\r\n\r\n");
        return TRUE;
    } else { // Fail
        printf("Fail\r\n\r\n");
        return FALSE;
    }
}


static void ut_rtos_sw_timer_cb(TimerHandle_t xTimer)
{
    sw_timer_cnt++;
    printf("%02d. ", sw_timer_cnt);
}


int ut_rtos_timer(void)
{
    int idx = 0;

    TimerHandle_t SwTimerHandle = NULL;

    // Create the software timer
    SwTimerHandle = xTimerCreate("SwTimer",          /* Text name. */
                                 SW_TIMER_PERIOD_MS, /* Timer period. */
                                 pdTRUE,             /* Enable auto reload. */
                                 0,                  /* ID is not used. */
                                 ut_rtos_sw_timer_cb);   /* The callback function. */

    // Start timer
    xTimerStart(SwTimerHandle, 0);

    printf("\r\n<<Timer>>\r\n");
    // UT - Wait Software timer via Task Delay
    for (idx = 0; idx < (UT_SW_TIMER_COUNT + 1); idx++) {
        vTaskDelay(SW_TIMER_PERIOD_MS);
        //vTaskDelay(100);
    }
    printf("\r\nTimer Count(%d)...", sw_timer_cnt);

    // Stop Software Timer
    xTimerStop(SwTimerHandle, 0);

    if (sw_timer_cnt >= UT_SW_TIMER_COUNT) {  // Pass
        printf("Pass\r\n\r\n");
        return TRUE;
    } else { // Fail
        printf("Fail\r\n\r\n");
        return FALSE;
    }
}

int ut_rtos_semaphore(void)
{
    printf("\r\n<<Semaphore>>\r\n");
    SemaphoreHandle_t xSemaphore = xSemaphoreCreateBinary();
    if (xSemaphore != NULL) {
        printf("Semaphore Create Pass!\n");
    } else {
        printf("Semaphore Create Fail!\n");
        return FALSE;
    }

#if defined(configQUEUE_REGISTRY_SIZE) && configQUEUE_REGISTRY_SIZE > 0
    vQueueAddToRegistry((QueueHandle_t)xSemaphore, "test_sem");
#endif /* #if defined(configQUEUE_REGISTRY_SIZE) && configQUEUE_REGISTRY_SIZE > 0 */

    if (xSemaphoreGive(xSemaphore) == pdTRUE) {
        printf("Semaphore Give Pass!\n");
    } else {
        printf("Semaphore Give Fail!\n");
        return FALSE;
    }

    if (xSemaphoreTake(xSemaphore, (TickType_t) 10) == pdTRUE) {
        printf("Semaphore Take Pass!\n");
    } else {
        printf("Semaphore Take Fail!\n");
        return FALSE;
    }

    return TRUE;
}


#ifdef UT_POSIX_ENABLE
uint32_t ut_posix_addr1, ut_posix_addr2;
void *thread1(void *arg)
{
    int temp = 10;
    printf("thread1--temp's address is 0x%p\n", &temp);
    pthread_setspecific(key, &temp);
    int *addr1 = (int *)pthread_getspecific(key);
    ut_posix_addr1 = (uint32_t)addr1;
    printf("thread1 - pthread_getspecific(key) : 0x%p, temp:%d\n", addr1, *addr1);
    return NULL;
}

void *thread2(void *arg)
{
    int temp = 20;
    vTaskDelay(2000);
    printf("thread2--temp's address is 0x%p\n", &temp);
    pthread_setspecific(key, &temp);
    int *addr2 = (int *)pthread_getspecific(key);
    ut_posix_addr2 = (uint32_t)addr2;
    printf("thread2-- pthread_getspecific(key) : 0x%p, temp:%d\n", addr2, *addr2);
    return NULL;
}

int ut_posix_pthread(void)
{
    printf("\r\n<<Posix - pthread key>>\r\n");
    pthread_t tid1, tid2;
    if (pthread_key_create(&key, NULL)) {
        printf("pthread key create failed!.\r\n");
        return FALSE;
    }
    if (pthread_create(&tid1, NULL, thread1, NULL)) {
        printf("pthread1 create failed!.\r\n");
        return FALSE;
    }
    if (pthread_create(&tid2, NULL, thread2, NULL)) {
        printf("pthread2 create failed!.\r\n");
        return FALSE;
    }
    if (pthread_join(tid1, NULL)) {
        printf("pthread1 join failed!.\r\n");
        return FALSE;
    }
    if (pthread_join(tid2, NULL)) {
        printf("pthread2 join failed!.\r\n");
        return FALSE;
    }
    pthread_key_delete(key);
    if (ut_posix_addr1 == ut_posix_addr2) {
        printf("Test Fail\r\n\r\n");
        return FALSE;
    } else {
        printf("Test Pass\r\n\r\n");
    }
    return TRUE;
}

int ut_rtos_posix(void)
{
    printf("\r\n<<Posix>>\r\n");
    int ret = 0;
    ret = ut_posix_pthread();
    return ret;
}
#endif /* #ifdef UT_POSIX_ENABLE */

ut_status_t ut_rtos(void)
{
    int ret = 0;

    ret = ut_rtos_mem();

    ret = ut_rtos_task();

    ret = ut_rtos_timer();

    ret = ut_rtos_semaphore();

#ifdef UT_POSIX_ENABLE
    ret = ut_rtos_posix();
#endif /* #ifdef UT_POSIX_ENABLE */
    return ret ? UT_STATUS_OK : UT_STATUS_ERROR;
}

#endif /* #ifdef UT_RTOS_ENABLE */
