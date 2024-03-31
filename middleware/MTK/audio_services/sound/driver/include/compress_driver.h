#ifndef _SOUND_INCLUDE_COMPRESS_DRIVER_H
#define _SOUND_INCLUDE_COMPRESS_DRIVER_H
#include "sound/driver/include/core.h"
#include "sound/driver/include/compress_offload.h"
#include "sound/include/asound.h"
#include "sound/driver/include/pcm.h"

struct snd_compr_stream;

struct snd_compr_ops {
    int(*open)(struct snd_compr_stream *stream);
    int(*free)(struct snd_compr_stream *stream);
    int(*set_params)(struct snd_compr_stream *stream, struct snd_compr_params *params);
    int(*get_params)(struct snd_compr_stream *stream, struct snd_codec *params);
    int(*set_metadata)(struct snd_compr_stream *stream, struct snd_compr_metadata *metadata);
    int(*get_metadata)(struct snd_compr_stream *stream, struct snd_compr_metadata *metadata);
    int(*trigger)(struct snd_compr_stream *stream, int cmd);
    int(*pointer)(struct snd_compr_stream *stream, struct snd_compr_tstamp *tstamp);
    int(*copy)(struct snd_compr_stream *stream, char *buf, unsigned int count);
    int(*ack)(struct snd_compr_stream *stream, unsigned int count);
    int(*get_caps)(struct snd_compr_stream *stream, struct snd_compr_caps *caps);
    int(*get_codec_caps)(struct snd_compr_stream *stream, struct snd_compr_codec_caps *codec);
};

#define SND_COMPR_NAME_MAX_LEN  64
struct snd_compr {
    struct snd_card *card;
    int device;
    char name[SND_COMPR_NAME_MAX_LEN];
    void *private_data;
    unsigned int direction;
    struct snd_compr_stream *stream;
};

struct snd_compr_runtime {
    msd_state_t state;
    void *buffer;
    unsigned long long buffer_size;
    unsigned int fragment_size;
    unsigned int fragments;
    uint64_t total_bytes_available;
    uint64_t total_bytes_transferrd;
    struct snd_dma_buffer dma_buf;
    EventGroupHandle_t wakeup;
    void *private_data;
};

struct snd_compr_stream {
    struct snd_compr *compr;
    struct snd_compr_ops *compr_ops;
    struct snd_compr_runtime *runtime;
    enum snd_compr_direction direction;
    void *private_data;
    unsigned int hw_opened;
    int metadata_set;
    int next_track;
};

int snd_compress_new(struct snd_card *card, const char *id, int device, int dir, struct snd_compr **pcompr);

#endif /* #ifndef _SOUND_INCLUDE_COMPRESS_DRIVER_H */
