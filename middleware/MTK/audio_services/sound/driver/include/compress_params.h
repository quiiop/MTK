#ifndef _SOUND_INCLUDE_COMPRESS_PARAMS_H_
#define _SOUND_INCLUDE_COMPRESS_PARAMS_H_

#define MAX_NUM_CODECS 32
#define MAX_NUM_CODEC_DESCRIPTORS 32
#define MAX_NUM_BITRATES 32
#define MAX_NUM_SAMPLE_RATES 32

/* codec list */

enum snd_codec_list {
    SND_AUDIOCODEC_MP3,
    SND_AUDIOCODEC_MAX,
};

struct snd_enc_flac {
    unsigned int num;
    unsigned int gain;
};

struct snd_enc_generic {
    unsigned int bw;// encoder bandwidth
};

union snd_codec_options {
    struct snd_enc_flac flac;
    struct snd_enc_generic generic;
};

/*
description of codec capabilities
*/
struct snd_codec_desc {
    unsigned int max_ch;
    unsigned int sample_rates[MAX_NUM_SAMPLE_RATES];
    unsigned int num_sample_rates;
    unsigned int bit_rate[MAX_NUM_BITRATES];
    unsigned int num_bitrates;
    unsigned int rate_control;
    unsigned int profiles;
    unsigned int mode;
    unsigned int format;
    unsigned int min_buffer;
};


struct snd_codec {
    unsigned int id;
    unsigned int ch_in;
    unsigned int ch_out;
    unsigned int sample_rate;
    unsigned int bit_rate;
    unsigned int rate_control;
    unsigned int profile;
    unsigned int level;
    unsigned int ch_mode;
    unsigned int format;
    unsigned int align;
    union snd_codec_options options;

};

#endif /* #ifndef _SOUND_INCLUDE_COMPRESS_PARAMS_H_ */
