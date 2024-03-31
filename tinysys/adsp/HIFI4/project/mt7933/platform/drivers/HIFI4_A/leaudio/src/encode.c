#include "le_main.h"
#include "encode.h"
#include "leaudio_debug.h"

extern const codec_st lc3_encode_op;
LE_SECTION_DATA const codec_st *leaudio_codec_table[MAX_CODEC_SUPPORT] = {&lc3_encode_op};

LE_SECTION_FUNC static const codec_st* find(int codec_type)
{
    const codec_st* ret = NULL;
    if(codec_type >= 0 && codec_type < MAX_CODEC_SUPPORT) {
        ret = leaudio_codec_table[codec_type];
    }
    if(ret) {
        return ret;
    }
    LE_LOG_E("unsupported leaudio_codec_type=%d !", codec_type);
    return NULL;
}

LE_SECTION_FUNC void leaudio_encodec_init(int codec_type)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        codec->encode_init();
    }
}

LE_SECTION_FUNC void leaudio_encodec_deinit(int codec_type)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        codec->encode_deinit();
    }
}

LE_SECTION_FUNC uint32_t leaudio_do_encode(int codec_type,
        char* in_buff,
        uint32_t in_buffer_size,
        char* out_buff,
        uint32_t out_buffer_size)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        return codec->encode(in_buff, in_buffer_size, out_buff, out_buffer_size);
    }
    return 0;
}

LE_SECTION_FUNC int leaudio_calc_input_length(int codec_type,int scenario)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        return codec->calc_input_len(scenario);
    }
    return 0;
}

LE_SECTION_FUNC unsigned int leaudio_get_sample_rate(int codec_type)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        return codec->get_sample_rate();
    }
    return 0;
}

LE_SECTION_FUNC unsigned int leaudio_get_audio_format(int codec_type)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        return codec->get_audio_format();
    }
    return 0;
}

LE_SECTION_FUNC unsigned int leaudio_get_num_channels(int codec_type)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        return codec->get_num_channels();
    }
    return 0;
}

LE_SECTION_FUNC int leaudio_encodec_pcm_buffer_len(int codec_type)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        return codec->encodec_pcm_buffer_len();
    }
    return 0;
}

LE_SECTION_FUNC void leaudio_decoder_init(int codec_type)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        codec->decode_init();
    }
}

LE_SECTION_FUNC void leaudio_decoder_deinit(int codec_type)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        codec->decode_deinit();
    }
}

LE_SECTION_FUNC uint32_t leaudio_do_decode(int codec_type,
        char* in_buff,
        uint32_t in_buffer_size,
        char* out_buff,
        uint32_t out_buffer_size,
        int bfi_ext,
        bool is_left)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        return codec->decode(in_buff, in_buffer_size, out_buff, out_buffer_size, bfi_ext, is_left);
    }
    return 0;
}

LE_SECTION_FUNC int leaudio_calc_output_length(int codec_type, int scenario)
{
    const codec_st* codec = find(codec_type);
    if(codec) {
        return codec->calc_output_len(scenario);
    }
    return 0;
}

