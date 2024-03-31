#ifndef _SOUND_INCLUDE_CORE_H_
#define _SOUND_INCLUDE_CORE_H_
#include "sound/driver/include/snd_local.h"

#define SND_DEVICE_TYPE_PCM_PLAYBACK 16
#define SND_DEVICE_TYPE_PCM_CAPTURE 24
#define SND_DEVICE_TYPE_COMPRESS 2
#define SND_DEVICE_TYPE_CONTROL 3

enum snd_device_state {
    SND_DEV_BUILD,
    SND_DEV_REGISTERED,
    SND_DEV_DISSCONNECTED,
};

enum snd_device_type {
    SND_DEV_PCM,
    SND_DEV_COMPRESS,
    SND_DEV_CONTROL,
};

struct snd_device;

struct snd_device_ops {
    int(*dev_free)(struct snd_device *);
    int(*dev_register)(struct snd_device *);
};

struct snd_device {
    enum snd_device_state state;
    enum snd_device_type type;
    void *device_data;
    struct snd_device_ops *dev_ops;
};

struct snd_card {
    char name[80];
    struct snd_device *devices[256];
    int device_num;
    void *private_data;
    void(*private_free)(struct snd_card *card);

};

struct snd_reg {
    int type;
    int device;
    const char *name;
    const struct snd_operations *f_ops;
    void *private_data;
    struct snd_card *card_ptr;
};

int snd_card_new(struct snd_card **card_ret);

int snd_register_device(int type, struct snd_card *card, unsigned int dev, const char *name, const struct snd_operations *f_ops, void *private_data);
int snd_unregister_device(unsigned int device);
int snd_device_new(struct snd_card *card, enum snd_device_type type, void *device_data, struct snd_device_ops *pcm_ops);
int snd_device_register_all(struct snd_card *card);
void snd_device_free(struct snd_card *card, void *device_data);
void snd_device_free_all(struct snd_card *card);

void *snd_lookup_minor_data(struct snd_reg *preg, int type);
struct snd_widget_path *snd_soc_find_widget_route(const char *src, const char *sink);

#endif /* #ifndef _SOUND_INCLUDE_CORE_H_ */
