//#include "FreeRTOS_POSIX.h"
//#include "FreeRTOS_POSIX/pthread.h"
//#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h> /* For memset */
#include <math.h>
#include <errno.h>

#include <FreeRTOS.h>

#include "pa_util.h" // melloc、free、initialize clock     
//#include "pa_unix_util.h" // Unix thread
#include "pa_debugprint.h"

/*
   Track memory allocations to avoid leaks.
 */


#if PA_TRACK_MEMORY
static int numAllocations_  = 0;
static int sizeAllocations_ = 0;
static int maxAllocations_  = 0;
#endif /* #if PA_TRACK_MEMORY */

void *PaUtil_AllocateMemory(long size)
{
#if PA_TRACK_MEMORY
    int *result = (int *)pvPortMalloc(size + 4);

    if (result != NULL) {
        sizeAllocations_ += size;
        numAllocations_  += 1;
        *result           = size;
        result ++;
    }

    maxAllocations_ = sizeAllocations_ > maxAllocations_ ?
                      sizeAllocations_ : maxAllocations_;

    return (void *)(result);
#else /* #if PA_TRACK_MEMORY */
    void *result = pvPortMalloc(size);

    if (result != NULL)
        numAllocations_ += 1;
    return result;
#endif /* #if PA_TRACK_MEMORY */
}


void PaUtil_FreeMemory(void *block)
{
#if PA_TRACK_MEMORY
    int *buf = block;
    if (buf != NULL) {
        buf --;
        sizeAllocations_ += *buf;
        numAllocations_  -= 1;
        vPortFree(buf);
    }
#else /* #if PA_TRACK_MEMORY */
    if (block != NULL) {
        vPortFree(block);
        numAllocations_ -= 1;
    }
#endif /* #if PA_TRACK_MEMORY */
}

void PaUtil_InitializeClock(void)
{
#ifdef HAVE_MACH_ABSOLUTE_TIME
    mach_timebase_info_data_t info;
    kern_return_t err = mach_timebase_info(&info);
    if (err == 0)
        machSecondsConversionScaler_ = 1e-9 * (double) info.numer / (double) info.denom;
#endif /* #ifdef HAVE_MACH_ABSOLUTE_TIME */
}