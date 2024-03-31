#ifndef __VA_H__
#define __VA_H__

#include "FreeRTOS.h"
#include "task.h"

struct va_task {
    unsigned short thread_stack_dep;
    UBaseType_t thread_priority;

    TaskHandle_t thread_handler;
    TaskHandle_t alarm_handler;
    void *priv;
};

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

void va_main(int argc, char *argv[]);
void va_stop(int argc, char *argv[]);
void va_dump(int argc, char *argv[]);
void mtask(void);

#endif /* #ifndef __VA_H__ */
