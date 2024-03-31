#ifndef __ENCODE_H__
#define __ENCODE_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const char* name;
    void (*encode_init)();
    uint32_t (*encode)(char*, uint32_t, char*, uint32_t);
    void (*encode_deinit)();
    int (*calc_input_len)(int);
    unsigned int (*get_sample_rate)();
    unsigned int (*get_audio_format)();
    unsigned int (*get_num_channels)();
    int (*encodec_pcm_buffer_len)();
    void (*decode_init)();
    uint32_t (*decode)(char*, uint32_t, char*, uint32_t, int, bool);
    void (*decode_deinit)();
    int (*calc_output_len)(int);
} codec_st;

enum {
    LC3_CODEC = 0,
    MAX_CODEC_SUPPORT
};

enum {
    ENCODE_SCENARIO = 0,
    DECODE_SCENARIO,
    MAX_SCENARIO_SUPPORT
};

enum {
    MONO_CHANNEL= 1,
    STEREO_CHANNEL,
    MAX_NUM_CHANNELS
};

//encoder function
void leaudio_encodec_init(int codec_type);
void leaudio_encodec_deinit(int codec_type);
uint32_t leaudio_do_encode(int codec_type, char* in_buff, uint32_t in_buffer_size, char* out_buff, uint32_t out_buffer_size);
int leaudio_calc_input_length(int codec_type,int scenario);
unsigned int leaudio_get_sample_rate(int codec_type);
unsigned int leaudio_get_audio_format(int codec_type);
unsigned int leaudio_get_num_channels(int codec_type);
int leaudio_encodec_pcm_buffer_len(int codec_type);

//decoder function
void leaudio_decoder_init(int codec_type);
void leaudio_decoder_deinit(int codec_type);
uint32_t leaudio_do_decode(int codec_type, char* in_buff, uint32_t in_buffer_size, char* out_buff, uint32_t out_buffer_size, int bfi_ext, bool is_left);
int leaudio_calc_output_length(int codec_type, int scenario);


#endif
