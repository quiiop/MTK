#ifndef _SOUND_INCLUDE_CONTROL_H_
#define _SOUND_INCLUDE_CONTROL_H_
#include "sound/driver/include/core.h"
struct snd_control;

struct snd_control_ops {
    int(*set)(struct snd_control *ctl, struct msd_ctl_value *value);
    int(*get)(struct snd_control *ctl, struct msd_ctl_value *value);
};

struct snd_control {
    struct snd_card *card;
    int device;
    char name[64];
    void *private_data;
    struct snd_control_ops *ops;
};

int snd_control_new(struct snd_card *card, int device, struct snd_control **pctl);

#endif /* #ifndef _SOUND_INCLUDE_CONTROL_H_ */
