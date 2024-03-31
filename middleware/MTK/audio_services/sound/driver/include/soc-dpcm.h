#ifndef _SOUND_INCLUDE_SOC_DPCM_H_
#define _SOUND_INCLUDE_SOC_DPCM_H_
#include "sound/utils/include/list.h"

enum snd_soc_dpcm_trigger {
    SND_SOC_DPCM_TRIGGER_PRE = 0,
    SND_SOC_DPCM_TRIGGER_POST,
};

#define DPCM_ACTIVE_STARTUP       0x1
#define DPCM_ACTIVE_SHUTDOWN      0x2
#define DPCM_ACTIVE_HW_FREE       0x4
#define DPCM_ACTIVE_HW_PARAMS     0x8
#define DPCM_ACTIVE_PREPARE       0x10

enum dapm_prepare_event {
    SND_SOC_PRE_PREPARE,
    SND_SOC_PREPARE,
    SND_SOC_POST_PREPARE,
    SND_SOC_FORCE_OFF,
};

struct snd_soc_dpcm {
    struct snd_soc_pcm_runtime *be;
    struct snd_soc_pcm_runtime *fe;
    struct list_head list;
};

struct snd_soc_dpcm_runtime {
    struct list_head be_clients;
};

int dpcm_be_dai_startup(struct snd_soc_pcm_runtime *fe_rtd);
int dpcm_be_dai_shutdown(struct snd_soc_pcm_runtime *fe_rtd);
int dpcm_be_dai_hw_free(struct snd_soc_pcm_runtime *fe_rtd);
int dpcm_be_dai_hw_params(struct snd_soc_pcm_runtime *fe_rtd, struct msd_hw_params *params);
int dpcm_be_dai_trigger(struct snd_soc_pcm_runtime *fe_rtd, int cmd);
int dpcm_be_dai_prepare(struct snd_soc_pcm_runtime *fe_rtd, enum dapm_prepare_event event);

#endif /* #ifndef _SOUND_INCLUDE_SOC_DPCM_H_ */

