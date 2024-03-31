/*
 * Amazon FreeRTOS POSIX V1.1.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * @file pthread.h
 * @brief Threads.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/pthread.h.html
 */

#ifndef _FREERTOS_POSIX_PTHREAD_LOCAL_STORAGE_H_
#define _FREERTOS_POSIX_PTHREAD_LOCAL_STORAGE_H_

/* preserve index 0 of FreeRTOS thread-local array for pthread tls implementation*/
#define PTHREAD_TLS_INDEX 0

typedef void (*pthread_key_destructor_t)(void*);


int pthread_key_create (pthread_key_t *__key,
                pthread_key_destructor_t __destructor);

int pthread_setspecific (pthread_key_t __key, const void *__value);

void *  pthread_getspecific (pthread_key_t __key);

int pthread_key_delete (pthread_key_t __key);

/*
* Cleanup will be performed in pthread_exit() or pthread_join()
*/
void pthread_tls_cleanup(TaskHandle_t xTaskHandle);

#endif /* _FREERTOS_POSIX_PTHREAD_LOCAL_STORAGE_H_ */
