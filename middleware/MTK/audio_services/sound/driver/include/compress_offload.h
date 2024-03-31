#ifndef _SOUND_INCLUDE_COMPRESS_OFFLOAD_H_
#define _SOUND_INCLUDE_COMPRESS_OFFLOAD_H_

#include "sound/include/asound.h"
#include "sound/driver/include/compress_params.h"


/*
fragment_size: size of buffer fragment in bytes
fragments: number of such fragments
*/
#define MAX_FRAGMENTS (1024)
struct snd_compressed_buffer {
    unsigned int fragment_size;
    unsigned int fragments;
};

struct snd_compr_params {
    struct snd_compressed_buffer buffer;// buffer description
    struct snd_codec codec;// codec parameters
    unsigned int no_wake_mode; // don't wake on fragment elapsed
};

struct snd_compr_tstamp {
    unsigned int byte_offset;// byte offset in ring buffer to DSP
    unsigned int copied_total;// total number of bytes copied from/to ringbuffer to/by DSP
    unsigned int pcm_frames;// frames decoded or encoded by DSP. This field will evolve by large steps and should only be used to monitor encoding/decoding process. It shall not be used for timing estimates.
    unsigned int pcm_io_frames;// frames rendered or received by DSP into a mixer or an audio output/input. This field should be used for A/V sync or time estimates.
    unsigned int sampling_rate;// sampling rate of audio
};

/*
avail: Number of bytes available in ring buffer for writing/ reading
tstamp: timestamp information
*/
struct snd_compr_avail {
    unsigned long long avail;
    struct snd_compr_tstamp tstamp;
};

enum snd_compr_direction {
    SND_COMPRESS_PLAYBACK = 0,
    SND_COMPRESS_CAPTURE
};

struct snd_compr_caps {
    unsigned int num_codecs;
    unsigned int direction;
    unsigned int min_fragment_size;
    unsigned int max_fragment_size;
    unsigned int min_fragments;
    unsigned int max_fragments;
    unsigned int codecs[MAX_NUM_CODECS];
};

struct snd_compr_codec_caps {
    unsigned int codec;
    unsigned int num_descriptons;
    struct snd_codec_desc descriptor[MAX_NUM_CODEC_DESCRIPTORS];
};

enum {
    SNDRV_COMPRESS_ENCODER_PADDING = 1,
    SNDRV_COMPRESS_ENCODER_DELAY = 2,
};

struct snd_compr_metadata {
    unsigned int key;
    unsigned int value[8];
};


#define IO_CMD 0
#define IO_ARGS_SIZE 5

#define IO_CMD_TYPE(type) sizeof(type)
#define _IOR_(io_cmd, io_cmd_type) (io_cmd << IO_CMD | IO_CMD_TYPE(io_cmd_type) << IO_ARGS_SIZE)
#define _IO_(io_cmd) io_cmd

typedef enum snd_ioctl_comp {
    SND_IOCTL_COMPRESS_VERSION,
    SND_IOCTL_COMPRESS_GET_CAPS,
    SND_IOCTL_COMPRESS_GET_CODEC_CAPS,
    SND_IOCTL_COMPRESS_SET_PARAMS,
    SND_IOCTL_COMPRESS_GET_PARAMS,
    SND_IOCTL_COMPRESS_SET_METADATA,
    SND_IOCTL_COMPRESS_GET_METADATA,
    SND_IOCTL_COMPRESS_TSTAMP,
    SND_IOCTL_COMPRESS_AVAIL,
    SND_IOCTL_COMPRESS_PAUSE,
    SND_IOCTL_COMPRESS_RESUME,
    SND_IOCTL_COMPRESS_START,
    SND_IOCTL_COMPRESS_STOP,
    SND_IOCTL_COMPRESS_DRAIN,
    SND_IOCTL_COMPRESS_NEXT_TRACK,
    SND_IOCTL_COMPRESS_PARTIAL_DRAIN,
} snd_ioctl_comp_t;

#endif /* #ifndef _SOUND_INCLUDE_COMPRESS_OFFLOAD_H_ */
