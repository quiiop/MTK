/*
 * Amazon FreeRTOS POSIX V1.1.0
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file FreeRTOS_POSIX_pthread_local_storage.c
 * @brief Implementation of thread functions in pthread_local_storage.h
 */

/* C standard library includes. */
#include <stdlib.h>
#include <sys/queue.h>

/* FreeRTOS+POSIX includes. */
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/errno.h"
#include "FreeRTOS_POSIX/pthread_local_storage.h"


typedef struct tls_key_t_ {
    pthread_key_t key;
    pthread_key_destructor_t destructor;
    SLIST_ENTRY(tls_key_t_) next;
} tls_key_t;

typedef struct tls_kv_t_ {
    pthread_key_t key;
    pthread_key_destructor_t destructor;
    void *value;
    SLIST_ENTRY(tls_kv_t_) next;
} tls_kv_t;

/* A global key record to keep all pthread_key_t */
SLIST_HEAD(tls_key_record_t, tls_key_t_) tls_key_record = SLIST_HEAD_INITIALIZER(tls_key_record);

/* A local key-value record for each thread to keep its own local value corresponding to key */
SLIST_HEAD(tls_kv_record_t_, tls_kv_t_);
typedef struct tls_kv_record_t_ tls_kv_record_t;

int pthread_key_create(pthread_key_t *__key, pthread_key_destructor_t __destructor)
{
    vTaskSuspendAll();

    tls_key_t *new_record = pvPortMalloc(sizeof(tls_key_t));
    if (new_record == NULL) {
        return ENOMEM;
    }

    tls_key_t *first_record = SLIST_FIRST(&tls_key_record);
    if( first_record == NULL ) {
        new_record->key = 1;
    } else {
        new_record->key = first_record->key + 1;
    }
    new_record->destructor = __destructor;
    *__key = new_record->key;

    SLIST_INSERT_HEAD(&tls_key_record, new_record, next);

    xTaskResumeAll();
    return 0;
}

int pthread_key_delete(pthread_key_t __key)
{
    vTaskSuspendAll();

    /* Search the key to delete */
    tls_key_t *record = NULL;
    SLIST_FOREACH(record, &tls_key_record, next) {
        if(record->key == __key) {
            break;
        }
    }
    if (record != NULL) {
        SLIST_REMOVE(&tls_key_record, record, tls_key_t_, next);
        vPortFree(record);
    } else {
        return EINVAL;
    }

    xTaskResumeAll();

    return 0;
}

void *pthread_getspecific(pthread_key_t __key)
{
    /* Get the current thread's key-value record */
    tls_kv_record_t *tls_kv_head = (tls_kv_record_t *) pvTaskGetThreadLocalStoragePointer(NULL, PTHREAD_TLS_INDEX);
    if (tls_kv_head == NULL) {
        return NULL;
    }

    /* Search the key-value with the key */
    tls_kv_t *kv = NULL;
    SLIST_FOREACH(kv, tls_kv_head, next) {
        if(kv->key == __key) {
            break;
        }
    }

    if(kv == NULL) {
        return NULL;
    }

    return kv->value;
}

int pthread_setspecific(pthread_key_t __key, const void *__value)
{
    vTaskSuspendAll();
    /* Make sure the key is valid */
    tls_key_t *record = NULL;
    SLIST_FOREACH(record, &tls_key_record, next) {
        if(record->key == __key) {
            break;
        }
    }

    if (record == NULL) {
        return EINVAL;
    }

    xTaskResumeAll();

    /* Create the first key-value if it has not been created before */
    tls_kv_record_t *tls_kv_head = pvTaskGetThreadLocalStoragePointer(NULL, PTHREAD_TLS_INDEX);
    if (tls_kv_head == NULL) {
        tls_kv_head = pvPortCalloc(1, sizeof(tls_kv_record_t));
        if (tls_kv_head == NULL) {
            return ENOMEM;
        }
        vTaskSetThreadLocalStoragePointer(NULL, PTHREAD_TLS_INDEX, tls_kv_head);
    }

    /* Search the key-value with the key */
    tls_kv_t *kv = NULL;
    SLIST_FOREACH(kv, tls_kv_head, next) {
        if(kv->key == __key) {
            break;
        }
    }

    if (kv != NULL) {
        if (__value != NULL) {
            kv->value = (void *) __value;
        } else { // set value to NULL means remove the key-value from record
            SLIST_REMOVE(tls_kv_head, kv, tls_kv_t_, next);
            vPortFree(kv);
        }
    } else  {
        if (__value != NULL) { // if there's no such a key-value, create one and insert it
            kv = pvPortMalloc(sizeof(tls_kv_t));
            if (kv == NULL) {
                return ENOMEM;
            }
            kv->key = __key;
            kv->value = (void *) __value;
            kv->destructor = record->destructor;
            SLIST_INSERT_HEAD(tls_kv_head, kv, next);
        } else {
            return 0;
        }
    }

    return 0;
}

void pthread_tls_cleanup(TaskHandle_t xTaskHandle)
{
    tls_kv_record_t *tls_kv_head = pvTaskGetThreadLocalStoragePointer(xTaskHandle, PTHREAD_TLS_INDEX);
    if (tls_kv_head != NULL) {

        tls_kv_t *kv = SLIST_FIRST(tls_kv_head);
        while(kv != NULL) {
            if (kv->destructor != NULL) {
                kv->destructor(kv->value);
            }
            tls_kv_t *next_entry = SLIST_NEXT(kv, next);
            vPortFree(kv);
            kv = next_entry;
        }
        vPortFree(tls_kv_head);

        vTaskSetThreadLocalStoragePointer(xTaskHandle, PTHREAD_TLS_INDEX, NULL);
    }
}

/*-----------------------------------------------------------*/
