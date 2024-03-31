#ifndef __SND_LOCAL_H_
#define __SND_LOCAL_H_

#include "sound/utils/include/aud_log.h"

#ifdef SND_THREAD_SAFE_API
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#endif /* #ifdef SND_THREAD_SAFE_API */

struct sound {
    struct snd_reg *preg;
    int flag;
    void *plug;
    void *private_data;
#ifdef SND_THREAD_SAFE_API
    int lock_enabled;
    pthread_mutex_t lock;
#endif /* #ifdef SND_THREAD_SAFE_API */
};

struct snd_operations {
    int(*open)(struct sound *snd);
    int(*close)(struct sound *snd);
    int(*read)(struct sound *snd, char *buf, unsigned int count);
    int(*write)(struct sound *snd, char *buf, unsigned int count);
    int(*ioctl)(struct sound *snd, int cmd, void *args);
};

int snd_find_device(const char *name);
int snd_open(struct sound **psnd, unsigned int device, int mode);
int snd_close(struct sound *snd);
int snd_ioctl(struct sound *snd, int cmd, void *params);
int snd_read(struct sound *snd, void *buf, unsigned int size);
int snd_write(struct sound *snd, void *buf, unsigned int size);
int snd_drop(struct sound *snd);
int snd_start(struct sound *snd);

#ifdef SND_THREAD_SAFE_API

#ifndef SND_LOCK_ENABLED
#define SND_LOCK_ENABLED (1)
#endif /* #ifndef SND_LOCK_ENABLED */

static inline void snd_lock_init(struct sound *snd)
{
    snd->lock_enabled = SND_LOCK_ENABLED;
    if (snd->lock_enabled)
        pthread_mutex_init(&snd->lock, NULL);
}

static inline int snd_trylock(struct sound *snd)
{
    int err = 0;
    if (snd->lock_enabled)
        err = pthread_mutex_trylock(&snd->lock);
    return err;
}

static inline void snd_lock(struct sound *snd)
{
    if (snd->lock_enabled)
        pthread_mutex_lock(&snd->lock);
}

static inline void snd_unlock(struct sound *snd)
{
    if (snd->lock_enabled)
        pthread_mutex_unlock(&snd->lock);
}

static inline void snd_lock_destroy(struct sound *snd)
{
    if (snd->lock_enabled)
        pthread_mutex_destroy(&snd->lock);
}
#else /* #ifdef SND_THREAD_SAFE_API */
#define snd_lock_init(snd)  do {} while (0)
#define snd_trylock(snd)    do {} while (0)
#define snd_lock(snd)       do {} while (0)
#define snd_unlock(snd)     do {} while (0)
#define snd_lock_destroy(snd)   do {} while (0)
#endif /* #ifdef SND_THREAD_SAFE_API */

#endif /* #ifndef __SND_LOCAL_H_ */

