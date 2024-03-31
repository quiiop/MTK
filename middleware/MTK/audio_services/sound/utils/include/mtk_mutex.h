#ifndef _SOUND_UTILS_INC_MTK_MUTEX_H_
#define _SOUND_UTILS_INC_MTK_MUTEX_H_

//#include "freertos_to_aos.h"
#include "FreeRTOS.h"
#include "semphr.h"

typedef void *mutex_t;

#define mutex_init()            xSemaphoreCreateMutex()
#define mutex_lock(mutex)       xSemaphoreTake(mutex, portMAX_DELAY)
#define mutex_unlock(mutex)     xSemaphoreGive(mutex)

#endif /* #ifndef _SOUND_UTILS_INC_MTK_MUTEX_H_ */
