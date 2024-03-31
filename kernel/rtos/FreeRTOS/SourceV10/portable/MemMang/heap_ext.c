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
 * A sample implementation of pvPortMalloc() that allows the heap to be defined
 * across multiple non-contigous blocks and combines (coalescences) adjacent
 * memory blocks as they are freed.
 *
 * See heap_1.c, heap_2.c, heap_3.c and heap_4.c for alternative
 * implementations, and the memory management pages of https://www.FreeRTOS.org
 * for more information.
 *
 * Usage notes:
 *
 * vPortDefineHeapRegions() ***must*** be called before pvPortMalloc().
 * pvPortMalloc() will be called if any task objects (tasks, queues, event
 * groups, etc.) are created, therefore vPortDefineHeapRegions() ***must*** be
 * called before any other objects are defined.
 *
 * vPortDefineHeapRegions() takes a single parameter.  The parameter is an array
 * of HeapRegion_t structures.  HeapRegion_t is defined in portable.h as
 *
 * typedef struct HeapRegion
 * {
 *	uint8_t *pucStartAddress; << Start address of a block of memory that will be part of the heap.
 *	size_t xSizeInBytes;	  << Size of the block of memory.
 * } HeapRegion_t;
 *
 * The array is terminated using a NULL zero sized region definition, and the
 * memory regions defined in the array ***must*** appear in address order from
 * low address to high address.  So the following is a valid example of how
 * to use the function.
 *
 * HeapRegion_t xHeapRegions[] =
 * {
 *  { ( uint8_t * ) 0x80000000UL, 0x10000 }, << Defines a block of 0x10000 bytes starting at address 0x80000000
 *  { ( uint8_t * ) 0x90000000UL, 0xa0000 }, << Defines a block of 0xa0000 bytes starting at address of 0x90000000
 *  { NULL, 0 }                << Terminates the array.
 * };
 *
 * vPortDefineHeapRegions( xHeapRegions ); << Pass the array into vPortDefineHeapRegions().
 *
 * Note 0x80000000 is the lower address so appears in the array first.
 *
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

/* Exception Mask for malloc */
#define SCB_SHCSR_ACTIVE_MASK (SCB_SHCSR_MONITORACT_Msk | SCB_SHCSR_USGFAULTACT_Msk | SCB_SHCSR_HARDFAULTACT_Msk | SCB_SHCSR_BUSFAULTACT_Msk | SCB_SHCSR_MEMFAULTACT_Msk)

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
static void prvInsertBlockIntoFreeList( BaseType_t xRegion, BlockLink_t *pxBlockToInsert );

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
 * block must by correctly byte aligned. */
static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart[configMAX_HEAP_REGION], *pxEnd[configMAX_HEAP_REGION] = {NULL};

/* Keeps track of the number of calls to allocate and free memory as well as the
 * number of free bytes remaining, but says nothing about fragmentation. */
static size_t xFreeBytesRemaining[configMAX_HEAP_REGION] = {0};
static size_t xMinimumEverFreeBytesRemaining[configMAX_HEAP_REGION] = {0};
static size_t xTotalHeapSize[configMAX_HEAP_REGION] = {0U};
const HeapRegion_t *pxHeapRegionCfg[configMAX_HEAP_REGION] = {NULL};
static size_t xNumberOfSuccessfulAllocations[configMAX_HEAP_REGION] = {0};
static size_t xNumberOfSuccessfulFrees[configMAX_HEAP_REGION] = {0};

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
 * member of an BlockLink_t structure is set then the block belongs to the
 * application.  When the bit is free the block is still part of the free heap
 * space. */
static size_t xBlockAllocatedBit = 0;

/*-----------------------------------------------------------*/

void *pvPortMallocExt( BaseType_t xRegion, size_t xWantedSize )
{
    configASSERT( !(SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) || (SCB->SHCSR & SCB_SHCSR_ACTIVE_MASK) );
    BlockLink_t * pxBlock, * pxPreviousBlock, * pxNewBlockLink;
    void * pvReturn = NULL;
    #if( configUSE_MALLOC_FAILED_HOOK == 1 )
    size_t xOrgSize = xWantedSize;
    #endif

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

    /*Avoid to be at negative offset of an array*/
    if( xRegion < 0 )
    {
        return pvReturn;
    }

    /* The heap must be initialised before the first call to
     * prvPortMalloc(). */
    configASSERT( pxEnd[xRegion] );

    vTaskSuspendAll();
    {
        /* Check the requested block size is not so large that the top bit is
         * set.  The top bit of the block size member of the BlockLink_t structure
         * is used to determine who owns the block - the application or the
         * kernel, so it must be free. */
        if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
        {
            /* The wanted size is increased so it can contain a BlockLink_t
             * structure in addition to the requested amount of bytes. */
            if( ( xWantedSize > 0 ) && 
                ( ( xWantedSize + xHeapStructSize ) >  xWantedSize ) ) /* Overflow check */
            {
                xWantedSize += xHeapStructSize;

                /* Ensure that blocks are always aligned */
                if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
                {
                    /* Byte alignment required. Check for overflow */
                    if( ( xWantedSize + ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) ) ) >
                         xWantedSize )
                    {
                        xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
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

            if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining[xRegion] ) )
            {
                /* Traverse the list from the start	(lowest address) block until
                 * one of adequate size is found. */
                pxPreviousBlock = &xStart[xRegion];
                pxBlock = xStart[xRegion].pxNextFreeBlock;

                while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                /* If the end marker was reached then a block of adequate size
                 * was not found. */
                if( pxBlock != pxEnd[xRegion] )
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

                        /* Calculate the sizes of two blocks split from the
                         * single block. */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList( xRegion, ( pxNewBlockLink ) );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining[xRegion] -= pxBlock->xBlockSize;

                    if( xFreeBytesRemaining[xRegion] < xMinimumEverFreeBytesRemaining[xRegion] )
                    {
                        xMinimumEverFreeBytesRemaining[xRegion] = xFreeBytesRemaining[xRegion];
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* The block is being returned - it is allocated and owned
                     * by the application and has no "next" block. */
                    pxBlock->xBlockSize |= xBlockAllocatedBit;
                    pxBlock->pxNextFreeBlock = NULL;
                    xNumberOfSuccessfulAllocations[xRegion]++;
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
                extern void vApplicationMallocFailedHook( BaseType_t xRegion, size_t xWantedSize);
                vApplicationMallocFailedHook(xRegion, xOrgSize);
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    #endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

    return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFreeExt( BaseType_t xRegion, void *pv )
{
    configASSERT( !(SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) || (SCB->SHCSR & SCB_SHCSR_ACTIVE_MASK) );
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

    if (( pv != NULL ) && isValidRegion(xRegion))
    {
        const HeapRegion_t *pxHeapRegion = pxHeapRegionCfg[ xRegion ];

        /* Check Free Address is valid for the specified region */
        if ( (puc < pxHeapRegion->pucStartAddress)
            || (puc > (uint8_t*)(pxHeapRegion->pucStartAddress + pxHeapRegion->xSizeInBytes)) )
        {
            /* Free pv Not in this region */
            #if( configUSE_FREE_FAILED_HOOK == 1 )
            {
                extern void vApplicationFreeFailedHook( BaseType_t xRegion, void *pv, size_t xBlockSize );

                puc -= xHeapStructSize;
                pxLink = ( void * ) puc;
                vApplicationFreeFailedHook(xRegion, pv, pxLink->xBlockSize );
            }
            #endif

            return;
        }

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
                    xFreeBytesRemaining[xRegion] += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                    prvInsertBlockIntoFreeList( xRegion, ( ( BlockLink_t * ) pxLink ) );
                    xNumberOfSuccessfulFrees[xRegion]++;
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

uint8_t *xPortGetRegionName( BaseType_t xRegion )
{
	/*Avoid to be at negative offset of an array*/
	if( xRegion < 0 )
	{
		return NULL;
	}
	else
	{
		return (pxHeapRegionCfg[xRegion]?pxHeapRegionCfg[xRegion]->name:NULL);
	}
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSizeExt( BaseType_t xRegion )
{
	return xFreeBytesRemaining[ xRegion ];
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSizeExt( BaseType_t xRegion )
{
	return xMinimumEverFreeBytesRemaining[ xRegion ];
}
/*-----------------------------------------------------------*/

size_t xPortGetTotalHeapSize(  BaseType_t xRegion )
{
	return xTotalHeapSize[ xRegion ];
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BaseType_t xRegion, BlockLink_t *pxBlockToInsert )
{
    BlockLink_t * pxIterator;
    uint8_t * puc;

    /*Avoid to be at negative offset of an array*/
    if( xRegion < 0 )
    {
        return;
    }

    /* Iterate through the list until a block is found that has a higher address
     * than the block being inserted. */
    for( pxIterator = &xStart[xRegion]; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
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
        if( pxIterator->pxNextFreeBlock != pxEnd[xRegion] )
        {
            /* Form one big block from the two blocks. */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pxNextFreeBlock = pxEnd[xRegion];
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
static BaseType_t prvIsRegionValid( const HeapRegion_t *pxRegion )
{
    int                 regIdx       = 0;
    BaseType_t          xReturn      = pdTRUE;;
    const HeapRegion_t *pHeapCfg     = NULL;
    uint8_t            *heapStart    = pxRegion->pucStartAddress;
    uint8_t            *heapEnd      = (uint8_t*)(pxRegion->pucStartAddress + pxRegion->xSizeInBytes);
    uint8_t            *regionS      = NULL;
    uint8_t            *regionE      = NULL;

    /* Check Region Size */
    if (pxRegion->xSizeInBytes <= 0)
    {
        return pdFALSE;
    }

    /* Check Region Range */
    for (regIdx=0; regIdx<configMAX_HEAP_REGION; regIdx++)
    {
        pHeapCfg = pxHeapRegionCfg[regIdx];
        if (!pHeapCfg->pucStartAddress || !pHeapCfg->xSizeInBytes) break;

        regionS = pHeapCfg->pucStartAddress;
        regionE = (uint8_t*)(pHeapCfg->pucStartAddress + pHeapCfg->xSizeInBytes);

        if ( (heapStart>=regionS && heapStart<regionE) || (heapStart<=regionS && heapEnd>=regionE) || (heapEnd>regionS && heapEnd<=regionE) )
        {
            xReturn = pdFALSE;
            break;
        }
    }

	return xReturn;
}

void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions )
{
    BlockLink_t * pxFirstFreeBlockInRegion = NULL, * pxPreviousFreeBlock;
    size_t xAlignedHeap;
    size_t xTotalRegionSize;
    BaseType_t xDefinedRegions = 0;
    size_t xAddress;
    const HeapRegion_t * pxHeapRegion;

    /* Can only call once! */
    configASSERT( pxEnd[0] == NULL );

    while ( (xDefinedRegions >= 0) && (xDefinedRegions < configMAX_HEAP_REGION) )
    {
        pxHeapRegion = pxHeapRegionCfg[xDefinedRegions] = &( pxHeapRegions[ xDefinedRegions ] );
        if ( !pxHeapRegion || (pxHeapRegion->xSizeInBytes <= 0 ))
        {
            break;
        }

        xTotalRegionSize = pxHeapRegion->xSizeInBytes;

        /* Ensure the heap region starts on a correctly aligned boundary. */
        xAddress = ( size_t ) pxHeapRegion->pucStartAddress;

        if( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
        {
            xAddress += ( portBYTE_ALIGNMENT - 1 );
            xAddress &= ~portBYTE_ALIGNMENT_MASK;

            /* Adjust the size for the bytes lost to alignment. */
            xTotalRegionSize -= xAddress - ( size_t ) pxHeapRegion->pucStartAddress;
        }

        xAlignedHeap = xAddress;

        /* Set xStart if it has not already been set. */
        //if( xDefinedRegions == 0 )
        //{
            /* xStart is used to hold a pointer to the first item in the list of
             *  free blocks.  The void cast is used to prevent compiler warnings. */
            xStart[ xDefinedRegions ].pxNextFreeBlock = ( BlockLink_t * ) xAlignedHeap;
            xStart[ xDefinedRegions ].xBlockSize = ( size_t ) 0;
        //}
        //else
        //{
        //    /* Should only get here if one region has already been added to the
        //     * heap. */
        //    configASSERT( pxEnd != NULL );

        //    /* Check blocks are passed in with increasing start addresses. */
        //    configASSERT( xAddress > ( size_t ) pxEnd );
        //}

        /* Remember the location of the end marker in the previous region, if
         * any. */
        pxPreviousFreeBlock = pxEnd[ xDefinedRegions ];

        /* pxEnd is used to mark the end of the list of free blocks and is
         * inserted at the end of the region space. */
        xAddress = xAlignedHeap + xTotalRegionSize;
        xAddress -= xHeapStructSize;
        xAddress &= ~portBYTE_ALIGNMENT_MASK;
        pxEnd[ xDefinedRegions ] = ( BlockLink_t * ) xAddress;
        pxEnd[ xDefinedRegions ]->xBlockSize = 0;
        pxEnd[ xDefinedRegions ]->pxNextFreeBlock = NULL;

        /* To start with there is a single free block in this region that is
         * sized to take up the entire heap region minus the space taken by the
         * free block structure. */
        pxFirstFreeBlockInRegion = ( BlockLink_t * ) xAlignedHeap;
        pxFirstFreeBlockInRegion->xBlockSize = xAddress - ( size_t ) pxFirstFreeBlockInRegion;
        pxFirstFreeBlockInRegion->pxNextFreeBlock = pxEnd[ xDefinedRegions ];

        /* If this is not the first region that makes up the entire heap space
         * then link the previous region to this region. */
        if( pxPreviousFreeBlock != NULL )
        {
            pxPreviousFreeBlock->pxNextFreeBlock = pxFirstFreeBlockInRegion;
        }

        xTotalHeapSize[ xDefinedRegions ]                 = pxFirstFreeBlockInRegion->xBlockSize;
        xMinimumEverFreeBytesRemaining[ xDefinedRegions ] = pxFirstFreeBlockInRegion->xBlockSize;
        xFreeBytesRemaining[ xDefinedRegions ]            = pxFirstFreeBlockInRegion->xBlockSize;

        /* Move onto the next HeapRegion_t structure. */
        xDefinedRegions++;
    }

    /* Check something was actually defined before it is accessed. */
    //configASSERT( xTotalHeapSize );

    /* Work out the position of the top bit in a size_t variable. */
    xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}
/*-----------------------------------------------------------*/

void vPortGetHeapStats( HeapStats_t * pxHeapStats )
{
    BlockLink_t * pxBlock;
    size_t xBlocks = 0, xMaxSize = 0, xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

    vTaskSuspendAll();
    {
        pxBlock = xStart[0].pxNextFreeBlock;

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

                /* Heap five will have a zero sized block at the end of each
                 * each region - the block is only used to link to the next
                 * heap region so it not a real block. */
                if( pxBlock->xBlockSize != 0 )
                {
                    if( pxBlock->xBlockSize < xMinSize )
                    {
                        xMinSize = pxBlock->xBlockSize;
                    }
                }

                /* Move to the next block in the chain until the last block is
                 * reached. */
                pxBlock = pxBlock->pxNextFreeBlock;
            } while( pxBlock != pxEnd[0] );
        }
    }
    ( void ) xTaskResumeAll();

    pxHeapStats->xSizeOfLargestFreeBlockInBytes = xMaxSize;
    pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
    pxHeapStats->xNumberOfFreeBlocks = xBlocks;

    taskENTER_CRITICAL();
    {
        pxHeapStats->xAvailableHeapSpaceInBytes = xFreeBytesRemaining[0];
        pxHeapStats->xNumberOfSuccessfulAllocations = xNumberOfSuccessfulAllocations[0];
        pxHeapStats->xNumberOfSuccessfulFrees = xNumberOfSuccessfulFrees[0];
        pxHeapStats->xMinimumEverFreeBytesRemaining = xMinimumEverFreeBytesRemaining[0];
    }
    taskEXIT_CRITICAL();
}

BaseType_t vPortRegisterHeapRegions( const HeapRegion_t *pxRegion )
{
    BlockLink_t *pxFirstFreeBlockInRegion = NULL, *pxPreviousFreeBlock;
    size_t xAlignedHeap;
    size_t xTotalRegionSize;
    BaseType_t xDefinedRegions = 0;
    size_t     xAddress;
    const HeapRegion_t *pHeapCfg = NULL;


    /* Find Empty Region Slot */
    for (xDefinedRegions=0; xDefinedRegions<configMAX_HEAP_REGION; xDefinedRegions++)
    {
        pHeapCfg = pxHeapRegionCfg[xDefinedRegions];
        if (!pHeapCfg->pucStartAddress && !pHeapCfg->xSizeInBytes) break;
    }

	/* System heap need to be confiured before APP's Customized Heap */
    if ( (xDefinedRegions==0) || (xDefinedRegions >= configMAX_HEAP_REGION) ) {
        return pdFALSE; //0
    }

    /* Check if the new heap region is valid */
    if ( prvIsRegionValid(pxRegion) == pdFALSE ) {
        return pdFALSE; //0
    }


	xTotalRegionSize = pxRegion->xSizeInBytes;

	/* Ensure the heap region starts on a correctly aligned boundary. */
	xAddress = ( size_t ) pxRegion->pucStartAddress;
	if( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
	{
		xAddress += ( portBYTE_ALIGNMENT - 1 );
		xAddress &= ~portBYTE_ALIGNMENT_MASK;

		/* Adjust the size for the bytes lost to alignment. */
		xTotalRegionSize -= xAddress - ( size_t ) pxRegion->pucStartAddress;
	}

	xAlignedHeap = xAddress;

	/* xStart is used to hold a pointer to the first item in the list of
	free blocks.  The void cast is used to prevent compiler warnings. */
	xStart[ xDefinedRegions ].pxNextFreeBlock = ( BlockLink_t * ) xAlignedHeap;
	xStart[ xDefinedRegions ].xBlockSize = ( size_t ) 0;

	/* Remember the location of the end marker in the previous region, if any. */
	pxPreviousFreeBlock = pxEnd[ xDefinedRegions ];

	/* pxEnd is used to mark the end of the list of free blocks and is
	inserted at the end of the region space. */
	xAddress = xAlignedHeap + xTotalRegionSize;
	xAddress -= xHeapStructSize;
	xAddress &= ~portBYTE_ALIGNMENT_MASK;
	pxEnd[ xDefinedRegions ] = ( BlockLink_t * ) xAddress;
	pxEnd[ xDefinedRegions ]->xBlockSize = 0;
	pxEnd[ xDefinedRegions ]->pxNextFreeBlock = NULL;

	/* To start with there is a single free block in this region that is
	sized to take up the entire heap region minus the space taken by the
	free block structure. */
	pxFirstFreeBlockInRegion = ( BlockLink_t * ) xAlignedHeap;
	pxFirstFreeBlockInRegion->xBlockSize = xAddress - ( size_t ) pxFirstFreeBlockInRegion;
	pxFirstFreeBlockInRegion->pxNextFreeBlock = pxEnd[ xDefinedRegions ];

	/* If this is not the first region that makes up the entire heap space
	then link the previous region to this region. */
	if( pxPreviousFreeBlock != NULL )
	{
		pxPreviousFreeBlock->pxNextFreeBlock = pxFirstFreeBlockInRegion;
	}

    xTotalHeapSize[ xDefinedRegions ]                 = pxFirstFreeBlockInRegion->xBlockSize;
	xMinimumEverFreeBytesRemaining[ xDefinedRegions ] = pxFirstFreeBlockInRegion->xBlockSize;
	xFreeBytesRemaining[ xDefinedRegions ]            = pxFirstFreeBlockInRegion->xBlockSize;

    /* Save New Heap Region Info */
    pxHeapRegionCfg[xDefinedRegions] = pxRegion;

    return  xDefinedRegions;
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
	void		*pvReturn   = NULL;
	size_t	   xBlockSize = 0;
	uint8_t	 *puc		= ( uint8_t * ) pv;
	BlockLink_t *pxLink	 = NULL;

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
void *pvPortMallocNCExt( BaseType_t xRegion, size_t xWantedSize )
{
#ifdef HAL_CACHE_REMAP_ENABLE

/*
	  head		res			xBlockAlignWantedSize		 res
	|_____|________|______________________|________|
	p1	 p2	 p3	 p4

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
	void *pvReturn = NULL;		  /* p2*/
	uint32_t xCacheAlignAddr;	   /* p3 */
	uint32_t xUserAddr;			 /* p4 */
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
			pvReturn = pvPortMallocExt(xRegion, xBlockAlignWantedSize + xResSize * 2);
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
	xUserAddr = (uint32_t)pvReturn + xResSize;	  /* p4 = p2 + res */
	configASSERT(xCacheAlignAddr <= xUserAddr);	 /* p3 <= p4 */

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
    void *pvReturn = NULL;          /* p2*/

    pvReturn = pvPortMallocExt(xRegion, xWantedSize);
    /* directly return if allocate fail */
    if(pvReturn == NULL)
    {
        return pvReturn;
    }

    pvReturn = HAL_CACHE_VIRTUAL_TO_PHYSICAL(pvReturn);

    return pvReturn;
#endif /* HAL_CACHE_REMAP_ENABLE */
}
void vPortFreeNCExt( BaseType_t xRegion, void *pv )
{
#ifdef HAL_CACHE_REMAP_ENABLE
/*
	  head		res		xBlockAlignWantedSize		 res
	|_____|________|______________________|________|
	p1	 p2	 p3	 p4

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
		vPortFreeExt(xRegion, (void*)xAddr);
	}

#else
	vPortFreeExt(xRegion, pv);
#endif /* HAL_CACHE_REMAP_ENABLE*/
}

/*-----------------------------------------------------------*/
void *pvPortMalloc( size_t xWantedSize )
{
    return pvPortMallocExt(configUSE_HEAP_REGION_DEFAULT, xWantedSize);
}

void vPortFree( void *pv )
{
    vPortFreeExt(configUSE_HEAP_REGION_DEFAULT, pv);
}

size_t xPortGetFreeHeapSize()
{
    return xPortGetFreeHeapSizeExt(configUSE_HEAP_REGION_DEFAULT);
}

size_t xPortGetMinimumEverFreeHeapSize()
{
    return xPortGetMinimumEverFreeHeapSizeExt( configUSE_HEAP_REGION_DEFAULT );
}

void *pvPortMallocNC( size_t xWantedSize )
{
    return pvPortMallocNCExt(configUSE_HEAP_REGION_DEFAULT, xWantedSize);
}

void vPortFreeNC( void *pv )
{
    vPortFreeNCExt(configUSE_HEAP_REGION_DEFAULT, pv);
}

/*-----------------------------------------------------------*/

/* Wrap c stand library malloc family, include malloc/calloc/realloc/free to FreeRTOS heap service */
#if defined(__GNUC__)
#include <reent.h>
#if( configLIBC_EXT == 0 )
#endif

void *_malloc_r(struct _reent *rptr, size_t size)
{
    (void)rptr;
    return pvPortMalloc(size);
}

void _free_r(struct _reent *rptr,void *pv)
{
	(void)rptr;
	vPortFree(pv);
}

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
