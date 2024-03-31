#ifndef _ASOUND_H_
#define _ASOUND_H_

typedef unsigned int                msd_format_t;
#define MSD_PCM_FMT_S8              (1<<0)
#define MSD_PCM_FMT_U8              (1<<1)
#define MSD_PCM_FMT_S16_LE          (1<<2)
#define MSD_PCM_FMT_S16_BE          (1<<3)
#define MSD_PCM_FMT_U16_LE          (1<<4)
#define MSD_PCM_FMT_U16_BE          (1<<5)
#define MSD_PCM_FMT_S24_LE          (1<<6)
#define MSD_PCM_FMT_S24_BE          (1<<7)
#define MSD_PCM_FMT_U24_LE          (1<<8)
#define MSD_PCM_FMT_U24_BE          (1<<9)
#define MSD_PCM_FMT_S32_LE          (1<<10)
#define MSD_PCM_FMT_S32_BE          (1<<11)
#define MSD_PCM_FMT_U32_LE          (1<<12)
#define MSD_PCM_FMT_U32_BE          (1<<13)
#define MSD_PCM_FMT_COUNT           (14)

typedef unsigned long msd_uframes_t;
typedef signed long msd_sframes_t;

enum {
    MSD_STREAM_PLAYBACK = 0,
    MSD_STREAM_CAPTURE,
    MSD_STREAM_NUM,
};

typedef enum msd_state {
    MSD_STATE_OPEN,
    MSD_STATE_SETUP,
    MSD_STATE_PREPARED,
    MSD_STATE_RUNNING,
    MSD_STATE_XRUN,
    MSD_STATE_DRAINING,
    MSD_STATE_PAUSED,
} msd_state_t;

struct msd_hw_params {
    unsigned int rate;
    unsigned int channels;
    unsigned int period_size;
    unsigned int period_count;
    msd_format_t format;
};

struct msd_sw_params {
    msd_uframes_t start_threshold;
    msd_uframes_t stop_threshold;
    unsigned int tstamp_type;
};

enum msd_open_mode {
    MSD_BLOCK = 0,
    MSD_NONBLOCK,
};

#define CONNECT_FE_BE (0X1)
#define CONNECT_IO_PORT (0X2)


#define MSD_CTL_ID_NAME_MAX_LEN 64
struct msd_ctl_id {
    int id;
    char name[MSD_CTL_ID_NAME_MAX_LEN];
};

struct msd_ctl_value {
    struct msd_ctl_id ctl_id;
    int type;
    union {
        int value[128];
    } integer;
    union {
        char data[512];
    } bytes;
};

struct route {
    const char *src;
    const char *sink;
    int value;
};

struct msd_info {
    int device;
    char device_name[64];
    int direction;
    int type;
};

typedef enum msd_ioctl_pcm {
    MSD_IOCTL_PCM_INFO,
    MSD_IOCTL_HW_PARAMS,
    MSD_IOCTL_HW_FREE,
    MSD_IOCTL_SW_PARAMS,
    MSD_IOCTL_PCM_DRAIN,
    MSD_IOCTL_PCM_START,
    MSD_IOCTL_PCM_AVAIL,
    MSD_IOCTL_PCM_DELAY,
    MSD_IOCTL_PCM_PREPARE,
    MSD_IOCTL_PCM_DROP,
} msd_ioctl_pcm_t;

typedef struct sound sound_t;

#endif /* #ifndef _ASOUND_H_ */
