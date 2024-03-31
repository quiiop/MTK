#ifndef __AUDIO_TEST_UTILS_H__
#define __AUDIO_TEST_UTILS_H__

typedef struct complex_ {
    float  real;
    float  image;
} Complex;

int connect_route(const char *src, const char *sink, int value, int type);
int control_cget(const char *control_name, int value_count);
int control_cget_v2(const char *control_name, int value_count, void *out_value);
int control_cset(const char *control_name, int value_count, void *value);
void volatile_memcpy(volatile void *dest, volatile void *src, unsigned int size);
int frequency_check(void *data, unsigned int data_size, unsigned int bitdepth, unsigned int channel_num,
                    unsigned int *frequency, unsigned int sample_rate);

#endif /* #ifndef __AUDIO_TEST_UTILS_H__ */

