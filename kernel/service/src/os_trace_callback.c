/*
    FreeRTOS V8.2.0 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#include "FreeRTOS.h"
#include "task.h"
#include "memory_attribute.h"
#include "timers.h"

#ifdef MTK_OS_SEMAPHORE_LIST_ENABLE
#include "queue.h"
#include "semphr.h"
#include <string.h>
#endif

#ifdef MTK_SWLA_ENABLE
#include "swla.h"
#endif

#ifdef MTK_OS_TIMER_LIST_ENABLE
#define TRACE_MAX_TIMERS                     (6)

typedef struct
{
    const char* pcOwnerTaskName;
    TickType_t xCreateTime;
}TimerInfo_t;

static TimerHandle_t xTraceTimerList[TRACE_MAX_TIMERS];
static TimerInfo_t xTraceTimerInfoList[TRACE_MAX_TIMERS];
static uint8_t ucTraceTimerCount = 0;
#endif

#if defined(MTK_SWLA_ENABLE)
ATTR_TEXT_IN_TCM void vTraceTaskSwitchIn(uint32_t pRio)
{
    uint8_t *pxTaskName;
    uint32_t xTaskName;
    void    *pxTask;
    (void)pRio;

    //to do: get the task tcb directly from the pxCurrentTCB
    pxTask     = xTaskGetCurrentTaskHandle();

    //to do: get the task name address directly from pxCurrentTCB
    pxTaskName = (uint8_t *)pcTaskGetTaskName(pxTask);
    /* only support 4-ascii character */
    xTaskName = (uint32_t)(pxTaskName[0] | (pxTaskName[1] << 8) | (pxTaskName[2] << 16) | (pxTaskName[3] << 24));

    SLA_RamLogging(xTaskName);
}
#else
void vTraceTaskSwitchIn(uint32_t pRio)
{
#if ((INCLUDE_xTaskGetCurrentTaskHandle == 1) && (INCLUDE_pcTaskGetTaskName == 1))
    printf("switch to %s\n", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
#endif
    (void)pRio;
}
#endif /* MTK_SWLA_ENABLE */

#ifdef MTK_OS_TIMER_LIST_ENABLE
void vTraceTimerCreate(void * pxNewTimer)
{
    if(ucTraceTimerCount >= TRACE_MAX_TIMERS){
        printf("The number of trace timers has reached the upper limit.\n");
        return;
    }
    TickType_t xTimeNow = xTaskGetTickCount();
    TaskStatus_t xTaskDetails;
    vTaskGetInfo( NULL,
                  &xTaskDetails,
                  pdTRUE,
                  eInvalid );

    xTraceTimerList[ucTraceTimerCount] = (TimerHandle_t) pxNewTimer;

    xTraceTimerInfoList[ucTraceTimerCount].pcOwnerTaskName = (char*)xTaskDetails.pcTaskName;
    xTraceTimerInfoList[ucTraceTimerCount].xCreateTime = xTimeNow;
    ucTraceTimerCount++;
}

void vShowTimerList(void)
{
    printf( "\n===== Current Time: %ld ======\n\n", xTaskGetTickCount() );
    printf( "%-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s\n",
    "[Timer]", "[Type]", "[Status]", "[Owner Task]", "[Create Time]", "[Timer Period]", "[Expiry Time]",  "[Remaining Time before Expiry]");
    printf( "------------------------------------------------------------------------------------------------------------------------------------\n" );
    for(int i = 0 ; i < ucTraceTimerCount ; i++) {
        if( xTraceTimerList[i] != NULL ) {

            printf( "%-16s", (char*)pcTimerGetName( xTraceTimerList[i] ) );
            printf( "%-16s", uxTimerGetReloadMode( xTraceTimerList[i] ) ? "auto-reload" : "one-shot" );
            printf( "%-16s", xTimerIsTimerActive( xTraceTimerList[i] ) ? "active" : "inactive" );
            printf( "%-16s", xTraceTimerInfoList[i].pcOwnerTaskName );
            printf( "%-16ld", xTraceTimerInfoList[i].xCreateTime );
            printf( "%-16ld", xTimerGetPeriod( xTraceTimerList[i] ) );
            printf( "%-16ld", xTimerGetExpiryTime( xTraceTimerList[i] ) );
            if( xTimerIsTimerActive( xTraceTimerList[i] ) ) {
                printf( "%-16ld", xTimerGetExpiryTime( xTraceTimerList[i] ) - xTaskGetTickCount() );
            }
            printf("\n");
        }
    }
}
#endif

#ifdef MTK_OS_SEMAPHORE_LIST_ENABLE
/* Queue size */
#define TRACE_QUEUE_MAX (64)
#define TRACE_QUEUE_HISTORY_MAX (512)
/*--------------------------------------*/
/* Filter */
#define TRACE_QUEUE_HISTORY_HIDE    (0) // Only show the semaphores specified in filter
#define TRACE_QUEUE_HISTORY_SHOW    (1) // Only hide the semaphores specified in filter

#define TRACE_QUEUE_HISTORY_FILTER_MODE TRACE_QUEUE_HISTORY_HIDE

#define TRACE_QUEUE_HISTORY_FILTER      "NOTASK_0", "NOTASK_1"  // do not record cli semaphore to save buffer
#define TRACE_QUEUE_HISTORY_FILTER_NUM  2

/* Enable failed record */
#define TRACE_QUEUE_HISTORY_FAILED_RECORD_ENABLE     0
/*--------------------------------------*/
/* Action Type */
#define TRACE_QUEUE_HISTORY_TAKE                   (0)
#define TRACE_QUEUE_HISTORY_TAKE_FROM_ISR          (1)
#define TRACE_QUEUE_HISTORY_GIVE                   (2)
#define TRACE_QUEUE_HISTORY_GIVE_FROM_ISR          (3)
#define TRACE_QUEUE_HISTORY_TAKE_FAILED            (4)
#define TRACE_QUEUE_HISTORY_TAKE_FROM_ISR_FAILED   (5)
#define TRACE_QUEUE_HISTORY_GIVE_FAILED            (6)
#define TRACE_QUEUE_HISTORY_GIVE_FROM_ISR_FAILED   (7)

/*--------------------------------------*/

typedef struct{
    QueueHandle_t xQueue;
    TaskHandle_t xLastTask;
    uint32_t ulInterruptNum;
    const char* pcQueueNameReserve; // it is used when there's no name registered
    uint32_t ulTakeSuccessCnt;
    uint32_t ulGiveSuccessCnt;
    uint32_t ulTakeFailedCnt;
    uint32_t ulGiveFailedCnt;
}QueueInfo_t;

typedef struct{
    QueueInfo_t xQueueInfo;
    uint8_t ucAction;
}QueueHistory_t;

uint8_t ucQueueCnt = 0;
uint16_t usHistoryCnt = 0;

#ifndef HAL_PSRAM_MODULE_ENABLED
#undef ATTR_RWDATA_IN_RAM
#define ATTR_RWDATA_IN_RAM
#endif

ATTR_RWDATA_IN_RAM QueueInfo_t xTraceQueueInfoList[TRACE_QUEUE_MAX];
ATTR_RWDATA_IN_RAM QueueHistory_t xTraceHistoryList[TRACE_QUEUE_HISTORY_MAX];

void vTraceQueueCreate(void * pxNewQueue)
{
    if( ucQueueCnt >= TRACE_QUEUE_MAX ) {
        return;
    }
    uint8_t ucQueueType = ucQueueGetQueueType( (QueueHandle_t)pxNewQueue );
    (void) ucQueueType;

    char* pcQueueName = (char*) pvPortMalloc(sizeof(char) * 16);
    if( pcQueueName == NULL ) {
        printf( "[Trace]: Failed when allocating queue name\n" );
        return;
    }
    int ret = snprintf( pcQueueName, 16, "%s_%d",
                        xTaskGetCurrentTaskHandle() != NULL && pcTaskGetTaskName( xTaskGetCurrentTaskHandle() ) != NULL ?
                        pcTaskGetTaskName( xTaskGetCurrentTaskHandle() ) : "NOTASK", ucQueueCnt );
    if( ret < 0 ) {
        printf( "[Trace]: Failed when assigning a name to a queue\n" );
        return;
    }

    QueueInfo_t xQueueInfo = {
                                .xQueue = (QueueHandle_t)pxNewQueue,
                                .xLastTask = xTaskGetCurrentTaskHandle(),
                                .ulInterruptNum = 0,
                                .pcQueueNameReserve = (const char*) pcQueueName,
                                .ulTakeSuccessCnt = 0,
                                .ulGiveSuccessCnt = 0,
                                .ulTakeFailedCnt = 0,
                                .ulGiveFailedCnt = 0
                             };
    xTraceQueueInfoList[ucQueueCnt++] = xQueueInfo;
}

static BaseType_t xTraceQueueFilter(QueueInfo_t xQueueInfo)
{
    char* pcQueueName = (char*)pcQueueGetName( xQueueInfo.xQueue ) != NULL ?
                                (char*)pcQueueGetName( xQueueInfo.xQueue ) :
                                (char*)xQueueInfo.pcQueueNameReserve;
    if( pcQueueName != NULL ) {
        char* pcFilters[TRACE_QUEUE_HISTORY_FILTER_NUM] = { TRACE_QUEUE_HISTORY_FILTER };
        for(int j = 0 ; j < TRACE_QUEUE_HISTORY_FILTER_NUM ; j++){
            if( TRACE_QUEUE_HISTORY_FILTER_MODE == TRACE_QUEUE_HISTORY_HIDE &&
                strcmp( pcQueueName, pcFilters[j] ) == 0 ){
                return pdTRUE;
            }
            if( TRACE_QUEUE_HISTORY_FILTER_MODE == TRACE_QUEUE_HISTORY_SHOW &&
                strcmp( pcQueueName, pcFilters[j] ) != 0 ){
                return pdTRUE;
            }
        }
    }

    return pdFALSE;
}

static void vTraceQueueAddToHistory(QueueInfo_t xQueueInfo, uint8_t ucQueueAction)
{
    QueueHistory_t xQueueHistory = {
                                        .xQueueInfo = xQueueInfo,
                                        .ucAction = ucQueueAction,
                                   };
    xTraceHistoryList[usHistoryCnt++] = xQueueHistory;
    if( usHistoryCnt >= TRACE_QUEUE_HISTORY_MAX ){
        usHistoryCnt = 0;
    }
}

static void vTraceQueueBehavior(void * xQueue, uint8_t ucQueueAction)
{
    for(int i = 0 ; i < ucQueueCnt ; i++) {
        if( xTraceQueueInfoList[i].xQueue == (QueueHandle_t)xQueue ) {
            /* Get the current task */
            xTraceQueueInfoList[i].xLastTask = xTaskGetCurrentTaskHandle();

            /* Add corresponding count */
            switch( ucQueueAction ) {
                case TRACE_QUEUE_HISTORY_TAKE:
                    xTraceQueueInfoList[i].ulTakeSuccessCnt++;
                    break;
                case TRACE_QUEUE_HISTORY_GIVE:
                    xTraceQueueInfoList[i].ulGiveSuccessCnt++;
                    break;
                case TRACE_QUEUE_HISTORY_TAKE_FAILED:
                    xTraceQueueInfoList[i].ulTakeFailedCnt++;
                    break;
                case TRACE_QUEUE_HISTORY_GIVE_FAILED:
                    xTraceQueueInfoList[i].ulGiveFailedCnt++;
                    break;
            }

            /* Filter out specific semaphores */
            if( xTraceQueueFilter( xTraceQueueInfoList[i] ) == pdTRUE ){
                return;
            } else {
                /* Add a record to history */
                vTraceQueueAddToHistory( xTraceQueueInfoList[i], ucQueueAction);
            }
            return;
        }
    }
}


static void vTraceQueueBehaviorFromISR(void * xQueue, uint8_t ucQueueAction)
{
    for(int i = 0 ; i < ucQueueCnt ; i++) {
        if( xTraceQueueInfoList[i].xQueue == (QueueHandle_t)xQueue ) {
            uint32_t ulCurrentInterrupt = __get_IPSR();
            if( ulCurrentInterrupt == 0 ){
                xTraceQueueInfoList[i].xLastTask = xTaskGetCurrentTaskHandle();
            } else {
                xTraceQueueInfoList[i].xLastTask = NULL;
                xTraceQueueInfoList[i].ulInterruptNum = ulCurrentInterrupt - 16; // ipsr start from 0, need an offset
            }

            /* Add corresponding count */
            switch( ucQueueAction ) {
                case TRACE_QUEUE_HISTORY_TAKE_FROM_ISR:
                    xTraceQueueInfoList[i].ulTakeSuccessCnt++;
                    break;
                case TRACE_QUEUE_HISTORY_GIVE_FROM_ISR:
                    xTraceQueueInfoList[i].ulGiveSuccessCnt++;
                    break;
                case TRACE_QUEUE_HISTORY_TAKE_FROM_ISR_FAILED:
                    xTraceQueueInfoList[i].ulTakeFailedCnt++;
                    break;
                case TRACE_QUEUE_HISTORY_GIVE_FROM_ISR_FAILED:
                    xTraceQueueInfoList[i].ulGiveFailedCnt++;
                    break;
            }


            /* Filter out specific semaphores */
            if( xTraceQueueFilter( xTraceQueueInfoList[i] ) == pdTRUE ){
                return;
            } else {
                /* Add a record to history */
                vTraceQueueAddToHistory( xTraceQueueInfoList[i], ucQueueAction);
            }
            return;
        }
    }
}

void vTraceQueueSend(void * xQueue)
{
    vTraceQueueBehavior(xQueue, TRACE_QUEUE_HISTORY_GIVE);
}

void vTraceQueueReceive(void * xQueue)
{
    vTraceQueueBehavior(xQueue, TRACE_QUEUE_HISTORY_TAKE);
}

void vTraceQueueSendFromISR(void * xQueue)
{
    vTraceQueueBehaviorFromISR(xQueue, TRACE_QUEUE_HISTORY_GIVE_FROM_ISR);
}

void vTraceQueueReceiveFromISR(void * xQueue)
{
    vTraceQueueBehaviorFromISR(xQueue, TRACE_QUEUE_HISTORY_TAKE_FROM_ISR);
}

void vTraceQueueSendFailed(void * xQueue)
{
#if TRACE_QUEUE_HISTORY_FAILED_RECORD_ENABLE == 1
    vTraceQueueBehavior(xQueue, TRACE_QUEUE_HISTORY_GIVE_FAILED);
#endif
}

void vTraceQueueReceiveFailed(void * xQueue)
{
#if TRACE_QUEUE_HISTORY_FAILED_RECORD_ENABLE == 1
    vTraceQueueBehavior(xQueue, TRACE_QUEUE_HISTORY_TAKE_FAILED);
#endif
}

void vTraceQueueSendFromISRFailed(void * xQueue)
{
#if TRACE_QUEUE_HISTORY_FAILED_RECORD_ENABLE == 1
    vTraceQueueBehaviorFromISR(xQueue, TRACE_QUEUE_HISTORY_GIVE_FROM_ISR_FAILED);
#endif
}

void vTraceQueueReceiveFromISRFailed(void * xQueue)
{
#if TRACE_QUEUE_HISTORY_FAILED_RECORD_ENABLE == 1
    vTraceQueueBehaviorFromISR(xQueue, TRACE_QUEUE_HISTORY_TAKE_FROM_ISR_FAILED);
#endif
}

void vTraceQueueDelete(void * xQueue)
{
    int i = 0;
    for( ; i < ucQueueCnt ; i++) {
        if( xTraceQueueInfoList[i].xQueue == (QueueHandle_t)xQueue ) {
            break;
        }
    }

    if( i >= ucQueueCnt ) {
        printf("[Trace]: The queue you want to delete doesn't exist.\n");
        return;
    }

    for( ; i < ucQueueCnt - 1 ; i++ ) {
        xTraceQueueInfoList[i] = xTraceQueueInfoList[i + 1];
    }

    ucQueueCnt--;
}

void vResetQueueCount(char* type, char* name)
{
    for(int i = 0 ; i < ucQueueCnt ; i++) {
        uint8_t pcQueueType = ucQueueGetQueueType( xTraceQueueInfoList[i].xQueue );
        if( ( strcmp(type, "SEMAPHORE") == 0 ) && ( pcQueueType == queueQUEUE_TYPE_BASE ) ){
            continue;
        }
        if( ( strcmp(type, "QUEUE") == 0 ) && ( pcQueueType != queueQUEUE_TYPE_BASE ) ){
            continue;
        }

        char* pcQueueName = (char*)pcQueueGetName( xTraceQueueInfoList[i].xQueue ) != NULL ?
                            (char*)pcQueueGetName( xTraceQueueInfoList[i].xQueue ) :
                            (char*)xTraceQueueInfoList[i].pcQueueNameReserve;

        if( ( pcQueueName != NULL && strcmp( pcQueueName, name ) == 0 ) || strcmp( "all", name ) == 0 ) {
            xTraceQueueInfoList[i].ulTakeSuccessCnt = 0;
            xTraceQueueInfoList[i].ulGiveSuccessCnt = 0;
            xTraceQueueInfoList[i].ulTakeFailedCnt = 0;
            xTraceQueueInfoList[i].ulGiveFailedCnt = 0;
        }
    }
}

void vShowQueueList(char* type)
{
    if( strcmp(type, "SEMAPHORE") == 0 ) {
        printf( "%-20s%-20s%-16s%-16s%-16s%-20s%-22s%-22s\n", "[Name]", "[Type]", "[Status]", "[Holder]", "[Max Count]", "[Current Count]", "[Take Failed Rate]", "[Give Failed Rate]");
        printf( "----------------------------------------------------------------------------------------------------------------------------------------------------\n" );
    } else {
        printf( "%-20s%-16s%-20s%-24s%-24s\n", "[Name]", "[Max Count]", "[Current Count]", "[Receive Failed Rate]", "[Send Failed Rate]");
        printf( "------------------------------------------------------------------------------------------------------\n" );
    }
    for(int i = 0 ; i < ucQueueCnt ; i++) {
        uint8_t pcQueueType = ucQueueGetQueueType( xTraceQueueInfoList[i].xQueue );
        if( ( strcmp(type, "SEMAPHORE") == 0 ) && ( pcQueueType == queueQUEUE_TYPE_BASE ) ){
            continue;
        }
        if( ( strcmp(type, "QUEUE") == 0 ) && ( pcQueueType != queueQUEUE_TYPE_BASE ) ){
            continue;
        }
        char* pcQueueName = (char*)pcQueueGetName( xTraceQueueInfoList[i].xQueue );
        printf( "%-20s", pcQueueName != NULL ? pcQueueName : xTraceQueueInfoList[i].pcQueueNameReserve );

        switch( pcQueueType ) {
            case queueQUEUE_TYPE_BINARY_SEMAPHORE:
                printf( "%-20s", "Binary Semaphore" );
                break;
            case queueQUEUE_TYPE_MUTEX:
                printf( "%-20s", "Mutex" );
                break;
            case queueQUEUE_TYPE_COUNTING_SEMAPHORE:
                printf( "%-20s", "Counting Semaphore" );
                break;
            case queueQUEUE_TYPE_RECURSIVE_MUTEX:
                printf( "%-20s", "Recursive Mutex" );
                break;
            case queueQUEUE_TYPE_BASE:
                break;
            default:
                printf( "%-20s", "Unknow Type" );
                break;
        }

        switch( ucQueueGetQueueType( xTraceQueueInfoList[i].xQueue ) ) {
            case queueQUEUE_TYPE_BINARY_SEMAPHORE:
            case queueQUEUE_TYPE_MUTEX:
            case queueQUEUE_TYPE_RECURSIVE_MUTEX:
                    if( uxQueueMessagesWaiting( xTraceQueueInfoList[i].xQueue ) == 1 ) {
                        printf( "%-16s%-16s", "Free", "None" );
                    } else {
                        if( xTraceQueueInfoList[i].xLastTask != NULL ) {
                            printf( "%-16s%-16s", "Hold",
                                    pcTaskGetTaskName( xTraceQueueInfoList[i].xLastTask ) != NULL ?
                                    pcTaskGetTaskName( xTraceQueueInfoList[i].xLastTask ) : "NO TASK NAME" );
                        } else {
                            printf( "%-16s%s%-12lu", "Hold", "IRQ_", xTraceQueueInfoList[i].ulInterruptNum );
                        }
                    }
                break;
            case queueQUEUE_TYPE_COUNTING_SEMAPHORE:
                printf( "%-16s%-16s", "----", "----" );
                break;
            default:
                break;
        }

        printf( "%-16lu", uxQueueMessagesWaiting( xTraceQueueInfoList[i].xQueue ) + uxQueueSpacesAvailable( xTraceQueueInfoList[i].xQueue ) );

        printf( "%-20lu", uxQueueMessagesWaiting( xTraceQueueInfoList[i].xQueue ) );

        if( xTraceQueueInfoList[i].ulTakeSuccessCnt + xTraceQueueInfoList[i].ulTakeFailedCnt != 0 ) {
            printf( "%2lu%-22s", ( xTraceQueueInfoList[i].ulTakeFailedCnt * 100 ) / ( xTraceQueueInfoList[i].ulTakeSuccessCnt + xTraceQueueInfoList[i].ulTakeFailedCnt ), "%" );
        } else {
            printf( "%-24s", "NaN" );
        }

        if( xTraceQueueInfoList[i].ulGiveSuccessCnt + xTraceQueueInfoList[i].ulGiveFailedCnt != 0 ) {
            printf( "%2lu%-22s", ( xTraceQueueInfoList[i].ulGiveFailedCnt * 100 ) / ( xTraceQueueInfoList[i].ulGiveSuccessCnt + xTraceQueueInfoList[i].ulGiveFailedCnt ), "%" );
        } else {
            printf( "%-24s", "NaN" );
        }

        printf( "\n" );
    }
}

void vShowHistory(char* type, char* queueName)
{
    printf( "            [%s]:    [%s]           [%s]", "Name", "Action Type", "Task Name or IRQ Number");
    printf( "\n------------------------------------------------------------------------------\n" );
    uint16_t usStartIdx = usHistoryCnt + 1;
    if( usStartIdx >= TRACE_QUEUE_HISTORY_MAX ) {
        usStartIdx = 0;
    }
    uint16_t usIdx = usStartIdx;
    do {
        uint8_t pcQueueType = ucQueueGetQueueType( xTraceHistoryList[usIdx].xQueueInfo.xQueue );
        if( ( ( strcmp(type, "SEMAPHORE") == 0 ) && ( pcQueueType != queueQUEUE_TYPE_BASE ) )
            || ( ( strcmp(type, "QUEUE") == 0 ) && ( pcQueueType == queueQUEUE_TYPE_BASE ) ) ){
            char* pcQueueName = (char*)pcQueueGetName( xTraceHistoryList[usIdx].xQueueInfo.xQueue ) != NULL ?
                            (char*)pcQueueGetName( xTraceHistoryList[usIdx].xQueueInfo.xQueue ) :
                            (char*)xTraceHistoryList[usIdx].xQueueInfo.pcQueueNameReserve;
            if( pcQueueName != NULL ) {
                if( strcmp( pcQueueName, queueName ) == 0 || strcmp( queueName, "all" ) == 0 ) {
                    printf( "[%16s]:    ", pcQueueName );
                    char* pcActionType = NULL;
                    switch( xTraceHistoryList[usIdx].ucAction ) {
                        case TRACE_QUEUE_HISTORY_TAKE:
                            pcActionType = "Take";
                            break;
                        case TRACE_QUEUE_HISTORY_TAKE_FROM_ISR:
                            pcActionType = "Take from ISR";
                            break;
                        case TRACE_QUEUE_HISTORY_GIVE:
                            pcActionType = "Give";
                            break;
                        case TRACE_QUEUE_HISTORY_GIVE_FROM_ISR:
                            pcActionType = "Give from ISR";
                            break;
                        case TRACE_QUEUE_HISTORY_TAKE_FAILED:
                            pcActionType = "Take Failed";
                            break;
                        case TRACE_QUEUE_HISTORY_TAKE_FROM_ISR_FAILED:
                            pcActionType = "Take from ISR Failed";
                            break;
                        case TRACE_QUEUE_HISTORY_GIVE_FAILED:
                            pcActionType = "Give Failed";
                            break;
                        case TRACE_QUEUE_HISTORY_GIVE_FROM_ISR_FAILED:
                            pcActionType = "Give from ISR Failed";
                            break;
                        default:
                            pcActionType = "Unknown";
                            break;
                    }
                    printf( "%-24s", pcActionType );
                    if( strcmp( pcActionType, "Take" ) == 0 || strcmp( pcActionType, "Give" ) == 0 ||
                        strcmp( pcActionType, "Take Failed" ) == 0 || strcmp( pcActionType, "Give Failed" ) == 0  ) {
                        printf( "%-16s", pcTaskGetTaskName( xTraceHistoryList[usIdx].xQueueInfo.xLastTask ) != NULL ?
                                                           pcTaskGetTaskName( xTraceHistoryList[usIdx].xQueueInfo.xLastTask ) :
                                                           "NO TASK NAME" );
                    } else {
                        if( xTraceHistoryList[usIdx].xQueueInfo.xLastTask == NULL ) {
                            printf( "%s%-12lu", "IRQ_", xTraceHistoryList[usIdx].xQueueInfo.ulInterruptNum );
                        } else {
                            printf( "%-16s", pcTaskGetTaskName( xTraceHistoryList[usIdx].xQueueInfo.xLastTask ) != NULL ?
                                                           pcTaskGetTaskName( xTraceHistoryList[usIdx].xQueueInfo.xLastTask ) :
                                                           "NO TASK NAME" );
                        }
                    }
                    printf( "\n" );
                }
            }
        }

        usIdx++;
        if( usIdx >= TRACE_QUEUE_HISTORY_MAX ) {
            usIdx = 0;
        }
    } while( usIdx != usStartIdx );
}

void vShowQueueWaitingList(char* type, char* queueName)
{
    if( strcmp(type, "SEMAPHORE") == 0 ) {
        printf( "\nShow tasks waiting to take semaphore or mutex: \n");
        printf( "          [%s]    [%s]\n", "Name", "Waiting Tasks" );
    } else {
        printf( "\nShow tasks waiting to send to queue or reveive from queue: \n");
        printf( "          [%s]    [%s]\n                    [%s]\n", "Name", "WaitingToReceive(<)", "WaitingToSend(>)" );
    }

    printf( "---------------------------------------------------------------------\n");
    for(uint8_t ucQueueToShow = 0; ucQueueToShow < ucQueueCnt ; ucQueueToShow++) {

        uint8_t pcQueueType = ucQueueGetQueueType( xTraceQueueInfoList[ucQueueToShow].xQueue );
        if( ( strcmp(type, "SEMAPHORE") == 0 ) && ( pcQueueType == queueQUEUE_TYPE_BASE ) ){
            continue;
        }
        if( ( strcmp(type, "QUEUE") == 0 ) && ( pcQueueType != queueQUEUE_TYPE_BASE ) ){
            continue;
        }

        char* pcQueueName = (char*)pcQueueGetName( xTraceQueueInfoList[ucQueueToShow].xQueue ) != NULL ?
                            (char*)pcQueueGetName( xTraceQueueInfoList[ucQueueToShow].xQueue ) :
                            (char*)xTraceQueueInfoList[ucQueueToShow].pcQueueNameReserve;
        if( pcQueueName != NULL && ( strcmp(queueName, "all") == 0 || strcmp(queueName, pcQueueName) == 0 ) ) {
            /* Access waiting lists with offset within xQUEUE structure*/
            List_t* xTasksWaitingToReceive = (List_t*)( (char*)( xTraceQueueInfoList[ucQueueToShow].xQueue ) + 36 );

            ListItem_t* pxIterator;
            TaskHandle_t xTask;

            /* Take waiting list*/
            printf( "%16s    ", pcQueueName );

            if( pcQueueType == queueQUEUE_TYPE_BASE ) {
                printf("< ");
            }
            if( listLIST_IS_EMPTY( xTasksWaitingToReceive ) == pdFALSE ) {
                for( pxIterator = listGET_HEAD_ENTRY( xTasksWaitingToReceive ) ; pxIterator != listGET_END_MARKER( xTasksWaitingToReceive ) ;
                        pxIterator = listGET_NEXT( pxIterator ) ) {
                    xTask = (TaskHandle_t)listGET_LIST_ITEM_OWNER( pxIterator );
                    printf( "%s, ", pcTaskGetTaskName( xTask ) );
                }
            }

            if( pcQueueType == queueQUEUE_TYPE_BASE ) { // For queue, maybe there are tasks waiting to send
                printf("\n                    > ");
                List_t* xTasksWaitingToSend = (List_t*)( (char*)( xTraceQueueInfoList[ucQueueToShow].xQueue ) + 16 );
                if( listLIST_IS_EMPTY( xTasksWaitingToSend ) == pdFALSE ) {
                    for( pxIterator = listGET_HEAD_ENTRY( xTasksWaitingToSend ) ; pxIterator != listGET_END_MARKER( xTasksWaitingToSend ) ;
                            pxIterator = listGET_NEXT( pxIterator ) ) {
                        xTask = (TaskHandle_t)listGET_LIST_ITEM_OWNER( pxIterator );
                        printf( "%s, ", pcTaskGetTaskName( xTask ) );
                    }
                }
            }
            printf("\n");
        }
    }
    printf("\n");
}

#endif /* MTK_OS_SEMAPHORE_LIST_ENABLE */
