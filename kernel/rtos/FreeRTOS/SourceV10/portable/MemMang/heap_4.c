/*
 * FreeRTOS Kernel V10.4.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of https://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>
#include <string.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
 * all the API functions to use the MPU wrappers.  That should only be done when
 * task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#include "hal.h"
#include "memory_attribute.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE         ( ( size_t ) 8 )

/* Allocate the memory for the heap. */
#if ( configAPPLICATION_ALLOCATED_HEAP == 1 )

/* The application writer has already defined the array used for the RTOS
* heap - probably so it can be placed in a special segment or address. */
    extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
    PRIVILEGED_DATA static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* configAPPLICATION_ALLOCATED_HEAP */

/* Define the linked list structure.  This is used to link free blocks in order
 * of their memory address. */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                     /*<< The size of the free block. */
#ifdef MTK_SUPPORT_HEAP_DEBUG
    uint32_t xLinkRegAddr;
#endif /* MTK_SUPPORT_HEAP_DEBUG */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert ) PRIVILEGED_FUNCTION;

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit( void ) PRIVILEGED_FUNCTION;

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
 * block must by correctly byte aligned. */
static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
PRIVILEGED_DATA static BlockLink_t xStart, * pxEnd = NULL;

/* Keeps track of the number of calls to allocate and free memory as well as the
 * number of free bytes remaining, but says nothing about fragmentation. */
PRIVILEGED_DATA static size_t xFreeBytesRemaining = 0U;
PRIVILEGED_DATA static size_t xMinimumEverFreeBytesRemaining = 0U;
PRIVILEGED_DATA static size_t xNumberOfSuccessfulAllocations = 0;
PRIVILEGED_DATA static size_t xNumberOfSuccessfulFrees = 0;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
 * member of an BlockLink_t structure is set then the block belongs to the
 * application.  When the bit is free the block is still part of the free heap
 * space. */
PRIVILEGED_DATA static size_t xBlockAllocatedBit = 0;

/*-----------------------------------------------------------*/

void * pvPortMalloc( size_t xWantedSize )
{
    BlockLink_t * pxBlock, * pxPreviousBlock, * pxNewBlockLink;
    void * pvReturn = NULL;

    #ifdef MTK_SUPPORT_HEAP_DEBUG
    /* Obtain the return address of caller from link register */
    #if defined(__GNUC__)
    uint32_t xLinkRegAddr = (uint32_t)__builtin_return_address(0);
    #elif defined(__CC_ARM)
    uint32_t xLinkRegAddr = __return_address();
    #elif defined(__ICCARM__)
    uint32_t xLinkRegAddr = __get_LR();
    #endif /* __GNUC__ */
    #endif /* MTK_SUPPORT_HEAP_DEBUG */

    vTaskSuspendAll();
    {
        /* If this is the first call to malloc then the heap will require
         * initialisation to setup the list of free blocks. */
        if( pxEnd == NULL )
        {
            prvHeapInit();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        /* Check the requested block size is not so large that the top bit is
         * set.  The top bit of the block size member of the BlockLink_t structure
         * is used to determine who owns the block - the application or the
         * kernel, so it must be free. */
        if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
        {
            /* The wanted size must be increased so it can contain a BlockLink_t
             * structure in addition to the requested amount of bytes. */
            if( ( xWantedSize > 0 ) && 
                ( ( xWantedSize + xHeapStructSize ) >  xWantedSize ) ) /* Overflow check */
            {
                xWantedSize += xHeapStructSize;

                /* Ensure that blocks are always aligned. */
                if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
                {
                    /* Byte alignment required. Check for overflow. */
                    if( ( xWantedSize + ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) ) ) 
                            > xWantedSize )
                    {
                        xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
                        configASSERT( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) == 0 );
                    }
                    else
                    {
                        xWantedSize = 0;
                    }  
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            } 
            else 
            {
                xWantedSize = 0;
            }

            if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
            {
                /* Traverse the list from the start	(lowest address) block until
                 * one of adequate size is found. */
                pxPreviousBlock = &xStart;
                pxBlock = xStart.pxNextFreeBlock;

                while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                /* If the end marker was reached then a block of adequate size
                 * was not found. */
                if( pxBlock != pxEnd )
                {
                    /* Return the memory space pointed to - jumping over the
                     * BlockLink_t structure at its start. */
                    pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

                    #ifdef MTK_SUPPORT_HEAP_DEBUG
                    pxPreviousBlock->pxNextFreeBlock->xLinkRegAddr = xLinkRegAddr;
                    #endif /* MTK_SUPPORT_HEAP_DEBUG */

                    /* This block is being returned for use so must be taken out
                     * of the list of free blocks. */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    /* If the block is larger than required it can be split into
                     * two. */
                    if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
                    {
                        /* This block is to be split into two.  Create a new
                         * block following the number of bytes requested. The void
                         * cast is used to prevent byte alignment warnings from the
                         * compiler. */
                        pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
                        configASSERT( ( ( ( size_t ) pxNewBlockLink ) & portBYTE_ALIGNMENT_MASK ) == 0 );

                        /* Calculate the sizes of two blocks split from the
                         * single block. */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList( pxNewBlockLink );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining -= pxBlock->xBlockSize;

                    if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
                    {
                        xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* The block is being returned - it is allocated and owned
                     * by the application and has no "next" block. */
                    pxBlock->xBlockSize |= xBlockAllocatedBit;
                    pxBlock->pxNextFreeBlock = NULL;
                    xNumberOfSuccessfulAllocations++;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceMALLOC( pvReturn, xWantedSize );
    }
    ( void ) xTaskResumeAll();

    #if ( configUSE_MALLOC_FAILED_HOOK == 1 )
        {
            if( pvReturn == NULL )
            {
                extern void vApplicationMallocFailedHook( void );
                vApplicationMallocFailedHook();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    #endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

    configASSERT( ( ( ( size_t ) pvReturn ) & ( size_t ) portBYTE_ALIGNMENT_MASK ) == 0 );
    return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void * pv )
{
    uint8_t * puc = ( uint8_t * ) pv;
    BlockLink_t * pxLink;

    #ifdef MTK_SUPPORT_HEAP_DEBUG
    /* Obtain the return address of caller from link register */
    #if defined(__GNUC__)
    uint32_t xLinkRegAddr = (uint32_t)__builtin_return_address(0);
    #elif defined(__CC_ARM)
    uint32_t xLinkRegAddr = __return_address();
    #elif defined(__ICCARM__)
    uint32_t xLinkRegAddr = __get_LR();
    #endif /* __GNUC__ */
    #endif /* MTK_SUPPORT_HEAP_DEBUG */

    if( pv != NULL )
    {
        /* The memory being freed will have an BlockLink_t structure immediately
         * before it. */
        puc -= xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = ( void * ) puc;

        /* Check the block is actually allocated. */
        configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == NULL );

        if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
        {
            if( pxLink->pxNextFreeBlock == NULL )
            {
                #ifdef MTK_SUPPORT_HEAP_DEBUG
                pxLink->xLinkRegAddr = xLinkRegAddr;
                #endif /* MTK_SUPPORT_HEAP_DEBUG */

                /* The block is being returned to the heap - it is no longer
                 * allocated. */
                pxLink->xBlockSize &= ~xBlockAllocatedBit;

                vTaskSuspendAll();
                {
                    /* Add this block to the list of free blocks. */
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                    prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
                    xNumberOfSuccessfulFrees++;
                }
                ( void ) xTaskResumeAll();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
    return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
    /* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

static void prvHeapInit( void ) /* PRIVILEGED_FUNCTION */
{
    BlockLink_t * pxFirstFreeBlock;
    uint8_t * pucAlignedHeap;
    size_t uxAddress;
    size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;

    /* Ensure the heap starts on a correctly aligned boundary. */
    uxAddress = ( size_t ) ucHeap;

    if( ( uxAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
    {
        uxAddress += ( portBYTE_ALIGNMENT - 1 );
        uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
        xTotalHeapSize -= uxAddress - ( size_t ) ucHeap;
    }

    pucAlignedHeap = ( uint8_t * ) uxAddress;

    /* xStart is used to hold a pointer to the first item in the list of free
     * blocks.  The void cast is used to prevent compiler warnings. */
    xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
    xStart.xBlockSize = ( size_t ) 0;

    /* pxEnd is used to mark the end of the list of free blocks and is inserted
     * at the end of the heap space. */
    uxAddress = ( ( size_t ) pucAlignedHeap ) + xTotalHeapSize;
    uxAddress -= xHeapStructSize;
    uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
    pxEnd = ( void * ) uxAddress;
    pxEnd->xBlockSize = 0;
    pxEnd->pxNextFreeBlock = NULL;

    /* To start with there is a single free block that is sized to take up the
     * entire heap space, minus the space taken by pxEnd. */
    pxFirstFreeBlock = ( void * ) pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = uxAddress - ( size_t ) pxFirstFreeBlock;
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

    /* Only one block exists - and it covers the entire usable heap space. */
    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

    /* Work out the position of the top bit in a size_t variable. */
    xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert ) /* PRIVILEGED_FUNCTION */
{
    BlockLink_t * pxIterator;
    uint8_t * puc;

    /* Iterate through the list until a block is found that has a higher address
     * than the block being inserted. */
    for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
    {
        /* Nothing to do here, just iterate to the right position. */
    }

    /* Do the block being inserted, and the block it is being inserted after
     * make a contiguous block of memory? */
    puc = ( uint8_t * ) pxIterator;

    if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
    {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /* Do the block being inserted, and the block it is being inserted before
     * make a contiguous block of memory? */
    puc = ( uint8_t * ) pxBlockToInsert;

    if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
    {
        if( pxIterator->pxNextFreeBlock != pxEnd )
        {
            /* Form one big block from the two blocks. */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pxNextFreeBlock = pxEnd;
        }
    }
    else
    {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /* If the block being inserted plugged a gab, so was merged with the block
     * before and the block after, then it's pxNextFreeBlock pointer will have
     * already been set, and should not be set here as that would make it point
     * to itself. */
    if( pxIterator != pxBlockToInsert )
    {
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}
/*-----------------------------------------------------------*/

void vPortGetHeapStats( HeapStats_t * pxHeapStats )
{
    BlockLink_t * pxBlock;
    size_t xBlocks = 0, xMaxSize = 0, xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

    vTaskSuspendAll();
    {
        pxBlock = xStart.pxNextFreeBlock;

        /* pxBlock will be NULL if the heap has not been initialised.  The heap
         * is initialised automatically when the first allocation is made. */
        if( pxBlock != NULL )
        {
            do
            {
                /* Increment the number of blocks and record the largest block seen
                 * so far. */
                xBlocks++;

                if( pxBlock->xBlockSize > xMaxSize )
                {
                    xMaxSize = pxBlock->xBlockSize;
                }

                if( pxBlock->xBlockSize < xMinSize )
                {
                    xMinSize = pxBlock->xBlockSize;
                }

                /* Move to the next block in the chain until the last block is
                 * reached. */
                pxBlock = pxBlock->pxNextFreeBlock;
            } while( pxBlock != pxEnd );
        }
    }
    ( void ) xTaskResumeAll();

    pxHeapStats->xSizeOfLargestFreeBlockInBytes = xMaxSize;
    pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
    pxHeapStats->xNumberOfFreeBlocks = xBlocks;

    taskENTER_CRITICAL();
    {
        pxHeapStats->xAvailableHeapSpaceInBytes = xFreeBytesRemaining;
        pxHeapStats->xNumberOfSuccessfulAllocations = xNumberOfSuccessfulAllocations;
        pxHeapStats->xNumberOfSuccessfulFrees = xNumberOfSuccessfulFrees;
        pxHeapStats->xMinimumEverFreeBytesRemaining = xMinimumEverFreeBytesRemaining;
    }
    taskEXIT_CRITICAL();
}


/*-----------------------------------------------------------*/

void *pvPortCalloc( size_t nmemb, size_t size )
{
    void *pvReturn;

    /* unsigned integer wrap around protection */
    #define __LIM ( 1 << ( sizeof( size_t ) * 8 / 2 ) )
    if( ( nmemb | size ) >= __LIM && size && ( nmemb > SIZE_MAX / size ) )
    {
        size = 0;
    }

#ifdef MTK_HEAP_SIZE_GUARD_ENABLE
#if defined( __GNUC__ )
    extern void *__wrap_pvPortMalloc( size_t );
    pvReturn = ( void * )__wrap_pvPortMalloc( nmemb * size );
#elif defined( __CC_ARM )
    pvReturn = pvPortMalloc( nmemb * size );
#endif /* __GNUC__ */
#else
    pvReturn = pvPortMalloc( nmemb * size );
#endif /* MTK_HEAP_SIZE_GUARD_ENABLE */
    if( pvReturn )
    {
        memset( pvReturn, 0, nmemb * size );
    }

    return pvReturn;
}
/*-----------------------------------------------------------*/


void *pvPortRealloc( void *pv, size_t size )
{
    void        *pvReturn   = NULL;
    size_t       xBlockSize = 0;
    uint8_t     *puc        = ( uint8_t * ) pv;
    BlockLink_t *pxLink     = NULL;

    pvReturn = pvPortCalloc( size, 1 );

    if( (pv != NULL) && (pvReturn != NULL) )
    {
        // The memory being freed will have an BlockLink_t structure immediately before it.
        puc -= xHeapStructSize;

        // This casting is to keep the compiler from issuing warnings.
        pxLink = ( void * ) puc;

        // Check the block is actually allocated
        configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == NULL );

        // Get Original Block Size
        xBlockSize = (pxLink->xBlockSize & ~xBlockAllocatedBit);

        // Get Original data length
        xBlockSize = (xBlockSize - xHeapStructSize);

        if(xBlockSize < size)
            memcpy(pvReturn, pv, xBlockSize);
        else
            memcpy(pvReturn, pv, size);

        // Free Original Ptr
        vPortFree(pv);
    }

    return pvReturn;
}

#ifdef HAL_CACHE_REMAP_ENABLE
#define portCacheline_ALIGNMENT HAL_CACHE_LINE_SIZE
#endif
void *pvPortMallocNC( size_t xWantedSize )
{
#ifdef HAL_CACHE_REMAP_ENABLE

/*
      head        res            xBlockAlignWantedSize         res
    |_____|________|______________________|________|
    p1     p2     p3     p4

    res is a const value: portCacheline_ALIGNMENT - portBYTE_ALIGNMENT, 
    the first res is to confirm this non-cacheable block is located at the different cache line compared with the front heap block
    the second res is to confirm this non-cacheable block is located at the differet cache line compared with the next heap block

    p1: block begin address
    p2: return address of pvPortMalloc
    p3: cache line align address, which is the begin of the cache line invalidate operation
    p4: user address,which is equal to p2 + res(portCacheline_ALIGNMENT - portBYTE_ALIGNMENT)
*/
    const size_t xResSize =  portCacheline_ALIGNMENT - portBYTE_ALIGNMENT; /* res */
    size_t xBlockAlignWantedSize = 0;
    void *pvReturn = NULL;          /* p2*/
    uint32_t xCacheAlignAddr;       /* p3 */
    uint32_t xUserAddr;             /* p4 */
    uint32_t xInvalidLength;
    if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
    {
        /* The wanted size is increased so it can contain a BlockLink_t
        structure in addition to the requested amount of bytes. */
        if( xWantedSize > 0 )
        {
            xBlockAlignWantedSize = xWantedSize;
        	/* Ensure that blocks are always aligned to the required number of bytes. */
        	if( ( xBlockAlignWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
        	{
        		/* Byte alignment required. */
        		xBlockAlignWantedSize += ( portBYTE_ALIGNMENT - ( xBlockAlignWantedSize & portBYTE_ALIGNMENT_MASK ) );
        		configASSERT( ( xBlockAlignWantedSize & portBYTE_ALIGNMENT_MASK ) == 0 );
        	}
        	else
        	{
        		mtCOVERAGE_TEST_MARKER();
        	}
            /* Allocate a block from heap memory */
            pvReturn = pvPortMalloc(xBlockAlignWantedSize + xResSize * 2);
        }
        else
        {
        	mtCOVERAGE_TEST_MARKER();
        }
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /* directly return if allocate fail */
    if(pvReturn == NULL) 
    {
        return pvReturn;
    }
    /* round up to cache line align size for invalidation */
    xCacheAlignAddr = ((uint32_t)pvReturn + portCacheline_ALIGNMENT - 1) & ~(portCacheline_ALIGNMENT - 1); /* p3 */
    xUserAddr = (uint32_t)pvReturn + xResSize;      /* p4 = p2 + res */
    configASSERT(xCacheAlignAddr <= xUserAddr);     /* p3 <= p4 */

    xInvalidLength = (xUserAddr - xCacheAlignAddr + xBlockAlignWantedSize + portCacheline_ALIGNMENT - 1) & ~(portCacheline_ALIGNMENT - 1); /* (p4 - p3 + xBlockAlignWantedSize) round up to cache line aligne size */
    configASSERT((xCacheAlignAddr + xInvalidLength) <= (xUserAddr + xBlockAlignWantedSize + xResSize)); /* (p3 + xInvalidLength) <= (p4 + xBlockAlignWantedSize + res) */

    /* do invalidation*/
    if(HAL_CACHE_STATUS_OK != hal_cache_invalidate_multiple_cache_lines(xCacheAlignAddr, xInvalidLength))
    {
        configASSERT(0);
    }

    /* change to non-cacheable address */
	xUserAddr = HAL_CACHE_VIRTUAL_TO_PHYSICAL(xUserAddr);

    return (void*)xUserAddr;
#else
    return pvPortMalloc(xWantedSize);
#endif /* HAL_CACHE_REMAP_ENABLE */
}
void vPortFreeNC( void *pv )
{
#ifdef HAL_CACHE_REMAP_ENABLE
/*
      head        res        xBlockAlignWantedSize         res
    |_____|________|______________________|________|
    p1     p2     p3     p4
    
    p2 = p4 - res
*/
    const uint32_t xResSize =  portCacheline_ALIGNMENT - portBYTE_ALIGNMENT; /* res */
    uint32_t xAddr;

    if(pv != NULL)
    {
        xAddr = (uint32_t)pv - xResSize; /* p2 */
        
        /* check address is cacheable or not, if yes, then assert */
		configASSERT(pdFALSE == hal_cache_is_cacheable(xAddr));

        /* change to virtual address */
		xAddr = HAL_CACHE_PHYSICAL_TO_VIRTUAL(xAddr);

        /* free */  
        vPortFree((void*)xAddr);
    }

#else
    vPortFree(pv);
#endif /* HAL_CACHE_REMAP_ENABLE*/
}

/* Wrap c stand library malloc family, include malloc/calloc/realloc/free to FreeRTOS heap service */
#if defined(__GNUC__)
#include <reent.h>
#if( configLIBC_EXT == 0 )
#endif

void _free_r(struct _reent *rptr,void *pv)
{
	(void)rptr;
	vPortFree(pv);
}

#if 0
void *_calloc_r(struct _reent *rptr,size_t nmemb, size_t size )
{
	(void)rptr;
	return pvPortCalloc(nmemb,size);
}

void *_realloc_r(struct _reent *rptr, void *pv, size_t size )
{
	(void)rptr;
	return pvPortRealloc(pv,size);
}
#endif

ATTR_USED void *__wrap_malloc(size_t size)
{
	return pvPortMalloc(size);
}

ATTR_USED void *__wrap_calloc(size_t nmemb, size_t size)
{
	return pvPortCalloc(nmemb,size);
}

ATTR_USED void *__wrap_realloc(void *pv, size_t size)
{
	return pvPortRealloc(pv,size);
}

ATTR_USED void __wrap_free(void *pv)
{
	vPortFree(pv);
}
#endif

