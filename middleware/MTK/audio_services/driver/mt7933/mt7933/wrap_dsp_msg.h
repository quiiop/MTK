#ifndef __WRAP_DSP_MSG_H__
#define __WRAP_DSP_MSG_H__

#include "audio_messenger_ipi.h"
#ifdef AUDIO_FREERTOS_SUPPORT
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#endif /* #ifdef AUDIO_FREERTOS_SUPPORT */

#ifdef AUDIO_ALIOS_SUPPORT
#include "freertos_to_aos.h"
#endif /* #ifdef AUDIO_ALIOS_SUPPORT */

typedef struct wrap_msg_task {
    const char thread_name[32];
    const unsigned int thread_stack_dep;
    const uint8_t  thread_priority;
    TaskHandle_t thread_handler;
    void (*task_loop_func)(void *void_this);

    QueueHandle_t msg_queue;
    SemaphoreHandle_t sema;
} WRAP_MSG_TASK;


int wrap_dsp_msg_probe(void);
void wrap_dsp_msg_remove(void);
int capture_wrap_msg_hdl(struct ipi_msg_t *p_ipi_msg);

#endif /* #ifndef __WRAP_DSP_MSG_H__ */
