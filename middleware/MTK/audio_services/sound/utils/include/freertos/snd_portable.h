#ifndef _SND_PORTABLE_H_
#define _SND_PORTABLE_H_

/***********posix part begin****************/
#ifdef FREERTOS_POSIX_SUPPORT
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#define PTHREAD_ONCE_INIT (0)
typedef int pthread_once_t;

int pthread_once(pthread_once_t *once_control, void(*init_routine)(void));

#endif /* #ifdef FREERTOS_POSIX_SUPPORT */

int pthread_once_lock_init(void);
int pthread_once_lock_destroy(void);
void snd_msg_lock(void);
void snd_msg_unlock(void);

/***********posix part end****************/

/***********Lock part end****************/

typedef void *spinlock_t;
#define spin_lock_flags_define(flag)            _init_lock_from_isr(flag)
#define spin_lock_irqsave(lock, flag)           _set_lock_from_isr(flag)
#define spin_unlock_irqrestore(lock, flag)      _set_unlock_from_isr(flag)
#define spin_lock_init(lock)                    (void)(lock)
#define spin_lock(lock)                         _set_lock()
#define spin_unlock(lock)                       _set_unlock()

/* Can Use in ISR */
#define _init_lock_from_isr(Y)     \
    UBaseType_t Y

/* Can Use in ISR */
#define _set_lock_from_isr(Y)   \
    do { \
        Y = taskENTER_CRITICAL_FROM_ISR(); \
    } while (0)

/* Can Use in ISR */
#define _set_unlock_from_isr(Y) \
    do { \
        taskEXIT_CRITICAL_FROM_ISR(Y); \
    } while (0)

/* DO Not use in ISR */
#define _set_lock()     \
    do { \
        taskENTER_CRITICAL(); \
        vTaskSuspendAll(); \
    } while (0)

/* DO Not use in ISR */
#define _set_unlock()   \
    do { \
        xTaskResumeAll(); \
        taskEXIT_CRITICAL(); \
    } while (0)

/***********Lock part end****************/

/***********TickCount part start****************/

#ifndef aud_mdelay
#define aud_mdelay(T) vTaskDelay(pdMS_TO_TICKS(T))
#endif /* #ifndef aud_mdelay */

/***********TickCount part end****************/

int snd_portable_init(void);
int snd_portable_uninit(void);

#endif /* #ifndef _SND_PORTABLE_H_ */
