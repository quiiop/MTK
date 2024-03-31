#ifndef _SOUND_INCLUDE_PCM_H_
#define _SOUND_INCLUDE_PCM_H_
#include "sound/include/asound.h"

#ifdef AUDIO_ALIOS_SUPPORT
#include "freertos_to_aos.h"
#endif /* #ifdef AUDIO_ALIOS_SUPPORT */

#ifdef AUDIO_FREERTOS_SUPPORT
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "timers.h"
#include "freertos/snd_portable.h"
#endif /* #ifdef AUDIO_FREERTOS_SUPPORT */

#include <errno.h>

typedef unsigned int                     snd_pcm_rate_t;
#define SNDRV_PCM_RATE_5512             (1<<0)      /* 5512Hz */
#define SNDRV_PCM_RATE_7350             (1<<1)      /* 7350Hz */
#define SNDRV_PCM_RATE_8000             (1<<2)      /* 8000Hz */
#define SNDRV_PCM_RATE_11025            (1<<3)      /* 11025Hz */
#define SNDRV_PCM_RATE_12000            (1<<4)      /* 12000Hz */
#define SNDRV_PCM_RATE_14700            (1<<5)      /* 14700Hz */
#define SNDRV_PCM_RATE_16000            (1<<6)      /* 16000Hz */
#define SNDRV_PCM_RATE_22050            (1<<7)      /* 22050Hz */
#define SNDRV_PCM_RATE_24000            (1<<8)      /* 24000Hz */
#define SNDRV_PCM_RATE_29400            (1<<9)      /* 29400Hz */
#define SNDRV_PCM_RATE_32000            (1<<10)     /* 32000Hz */
#define SNDRV_PCM_RATE_44100            (1<<11)     /* 44100Hz */
#define SNDRV_PCM_RATE_48000            (1<<12)     /* 48000Hz */
#define SNDRV_PCM_RATE_64000            (1<<13)     /* 64000Hz */
#define SNDRV_PCM_RATE_88200            (1<<14)     /* 88200Hz */
#define SNDRV_PCM_RATE_96000            (1<<15)     /* 96000Hz */
#define SNDRV_PCM_RATE_176400           (1<<16)     /* 176400Hz */
#define SNDRV_PCM_RATE_192000           (1<<17)     /* 192000Hz */
#define SNDRV_PCM_RATE_352800           (1<<18)     /* 352800Hz */
#define SNDRV_PCM_RATE_384000           (1<<19)     /* 384000Hz */
#define SNDRV_PCM_RATE_COUNT            (20)

#define SNDRV_PCM_RATE_8000_44100       (SNDRV_PCM_RATE_8000|SNDRV_PCM_RATE_11025|\
                                         SNDRV_PCM_RATE_16000|SNDRV_PCM_RATE_22050|\
                                         SNDRV_PCM_RATE_24000|SNDRV_PCM_RATE_32000|\
                                         SNDRV_PCM_RATE_44100)
#define SNDRV_PCM_RATE_8000_48000       (SNDRV_PCM_RATE_8000_44100|SNDRV_PCM_RATE_48000)
#define SNDRV_PCM_RATE_8000_96000       (SNDRV_PCM_RATE_8000_48000|SNDRV_PCM_RATE_64000|\
                                         SNDRV_PCM_RATE_88200|SNDRV_PCM_RATE_96000)
#define SNDRV_PCM_RATE_8000_192000      (SNDRV_PCM_RATE_8000_96000|SNDRV_PCM_RATE_176400|\
                                         SNDRV_PCM_RATE_192000)
#define SNDRV_PCM_RATE_8000_384000      (SNDRV_PCM_RATE_8000_192000|\
                                         SNDRV_PCM_RATE_352800|\
                                        SNDRV_PCM_RATE_384000)

enum {
    SND_PCM_TRIGGER_STOP = 0,
    SND_PCM_TRIGGER_START = 1,
    SND_PCM_TRIGGER_PAUSE_PUSH,
    SND_PCM_TRIGGER_PAUSE_RELEASE,
};

struct snd_pcm_stream;
struct snd_pcm_ops {
    int(*open)(struct snd_pcm_stream *stream);
    int(*close)(struct snd_pcm_stream *stream);
    int(*ioctl)(struct snd_pcm_stream *stream, unsigned int cmd, void *arg);
    int(*hw_params)(struct snd_pcm_stream *stream, struct msd_hw_params *params);
    int(*hw_free)(struct snd_pcm_stream *stream);
    int(*prepare)(struct snd_pcm_stream *stream);
    int(*trigger)(struct snd_pcm_stream *stream, int cmd);
    unsigned long (*pointer)(struct snd_pcm_stream *stream);
    int(*copy)(struct snd_pcm_stream *stream, void *buf, unsigned int bytes);
    int(*ack)(struct snd_pcm_stream *stream);
};

#define RUNTIME_EVENT_PERIOD_ELAPSE 0x1
#define RUNTIME_EVENT_DRAIN_DONE 0x2


struct snd_dma_buffer {
    char *base;
    char *aligned_base;
    unsigned int size;
};

struct snd_pcm_runtime {
    //status
    msd_state_t state;
    msd_uframes_t hw_ptr;
    msd_uframes_t old_hw_ptr;
    msd_uframes_t appl_ptr;

    msd_uframes_t xrun_frames;
    //hw params
    unsigned int rate;
    unsigned int channels;
    unsigned int bytes_per_frame;
    unsigned int bit_width;
    int format;
    msd_uframes_t period_size;
    msd_uframes_t buffer_size;
    msd_uframes_t min_align;
    //sw params
    msd_uframes_t start_threshold;
    msd_uframes_t stop_threshold;
    msd_uframes_t boundary;
    //lock/scheduing
    msd_uframes_t twake;
    EventGroupHandle_t wakeup;

    struct snd_dma_buffer dma_buf;
};

struct snd_pcm {
    struct snd_card *card;
    int device;
    char name[64];
    void *private_data;
    unsigned int direction;
    struct snd_pcm_stream *stream;
    int internal;
};

struct stream_status {
    int startup;
    int hw_params;
    int prepare;
    int trigger_start;
    int trigger_pause;
};

struct snd_pcm_stream {
    struct snd_pcm *pcm;
    const struct snd_pcm_ops *pcm_ops;
    struct snd_pcm_runtime *runtime;
    int direction;
    void *private_data;
    unsigned int hw_opened;
    struct stream_status status;
    spinlock_t stream_lock;
};

// Get the available(writable) space for playback
static inline msd_uframes_t snd_pcm_playback_avail(struct snd_pcm_runtime *runtime)
{
    msd_sframes_t avail = runtime->hw_ptr + runtime->buffer_size - runtime->appl_ptr;

    if (avail < 0)
        avail += runtime->boundary;
    else if ((msd_uframes_t)avail > runtime->boundary)
        avail -= runtime->boundary;
    return avail;
}

//Get the available(readable) space for capture
static inline msd_uframes_t snd_pcm_capture_avail(struct snd_pcm_runtime *runtime)
{
    msd_sframes_t avail = runtime->hw_ptr - runtime->appl_ptr;
    if (avail < 0)
        avail += runtime->boundary;
    return avail;
}

static inline int snd_pcm_running(struct snd_pcm_stream *stream)
{
    return (stream->runtime->state == MSD_STATE_RUNNING ||
            (stream->runtime->state == MSD_STATE_DRAINING &&
             stream->direction == MSD_STREAM_PLAYBACK));
}

typedef int(*transfer_f)(struct snd_pcm_stream *, char *, unsigned int, unsigned int);
int snd_pcm_new(struct snd_card *card, const char *id, int device, int dir, int internal, struct snd_pcm **rpcm);

unsigned int snd_pcm_format_width(msd_format_t format);
unsigned int snd_pcm_format_physical_width(msd_format_t format);
unsigned int snd_pcm_id_to_rate(snd_pcm_rate_t id);
snd_pcm_rate_t snd_pcm_rate_to_id(unsigned int rate);
void snd_pcm_period_elapsed(struct snd_pcm_stream *stream, int in_interrupt);

#endif /* #ifndef _SOUND_INCLUDE_PCM_H_ */
