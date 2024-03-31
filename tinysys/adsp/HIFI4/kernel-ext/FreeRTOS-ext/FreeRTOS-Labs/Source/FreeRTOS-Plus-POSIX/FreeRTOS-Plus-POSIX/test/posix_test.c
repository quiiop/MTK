/* Copyright Statement:
 *
 * (C) 2005-2020  MediaTek Inc. All rights reserved.
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
#include <assert.h>
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#include "FreeRTOS_POSIX/unistd.h"
#include "FreeRTOS_POSIX/time.h"
#include "FreeRTOS_POSIX/semaphore.h"
#include "FreeRTOS_POSIX/sched.h"
#include "FreeRTOS_POSIX/utils.h"

static pthread_mutex_t test_mutex;
static pthread_cond_t test_cond;
static struct timespec start_time, end_time;
static sem_t semaphore;

static void *thread_loop(void *arg)
{
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    printf("thread enter %ld(s) %ld(ns)\n", (long)start_time.tv_sec, start_time.tv_nsec);

    pthread_mutex_lock(&test_mutex);

    pthread_cond_wait(&test_cond, &test_mutex);

    printf("thread waked up\n");

    sleep(5);

    printf("sleep done, release lock\n");

    pthread_mutex_unlock(&test_mutex);

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    printf("thread quit %ld(s) %ld(ns)\n", (long)end_time.tv_sec, end_time.tv_nsec);

    pthread_exit(NULL);

    return NULL;
}

static void timer_callback(union sigval sigtype)
{
    printf("in timer_callback, trigger main\n");

    /* Post to the semaphore and unblock the main test thread. */
    assert(0 == sem_post(&semaphore));
}

static void timer_test(void)
{
    timer_t timer = {0};
    struct itimerspec timeout = { { 0 }, { 0 } };
    struct sigevent sig_event = {0};

    assert(0 == sem_init( &semaphore, 0, 0));

    sig_event.sigev_notify = SIGEV_THREAD;
    sig_event.sigev_notify_function = timer_callback;

    /* Create a timer. */
    assert(0 == timer_create(CLOCK_MONOTONIC, &sig_event, &timer));

    /* Set an absolute timeout. */
    assert(0 == clock_gettime(CLOCK_MONOTONIC, &timeout.it_value));


    assert(0 == UTILS_TimespecAddNanoseconds(&timeout.it_value,
                                            (int64_t)5000000000LL,
                                            &timeout.it_value));

    printf("set timer\n");

    assert(0 == timer_settime(timer, TIMER_ABSTIME, &timeout, NULL));

    printf("wait timer to trigger\n");

    assert(0 == sem_wait(&semaphore));

    assert(0 == sem_destroy(&semaphore));

    assert(0 == timer_delete(timer));
}

void posix_test(void)
{
    pthread_attr_t attr = {0};
    struct sched_param  param = {0};
    pthread_t test_thread = {0};

    //assert(0 == pthread_mutex_init(&test_mutex, NULL));
    printf("before pthread_cond_init\n");
    assert(0 == pthread_cond_init(&test_cond, NULL));
    printf("before pthread_attr_init\n");

    assert(0 == pthread_attr_init(&attr));
    printf("before pthread_attr_setschedparam\n");

    param.sched_priority = 4;

    assert(0 == pthread_attr_setschedparam(&attr, &param));
    printf("before pthread_create\n");

    assert(0 == pthread_create(&test_thread, &attr, thread_loop, NULL));

    printf("posix test start, main sleep\n");

    sleep(3);

    printf("main sleep done, trigger thread\n");

    assert(0 == pthread_cond_signal(&test_cond));

    sleep(2);

    printf("main lock...\n");

    assert(0 == pthread_mutex_lock(&test_mutex));

    printf("main lock done\n");

    assert(0 == pthread_join(test_thread, NULL));

    printf("thread joined, start test timer\n");

    assert(0 == pthread_mutex_unlock(&test_mutex));

    timer_test();
}

void posix_test_thread(void *arg)
{
    printf("enter posix_test_thread\n");

    posix_test();

    printf("posix test done\n");

    vTaskDelete(NULL);
}

