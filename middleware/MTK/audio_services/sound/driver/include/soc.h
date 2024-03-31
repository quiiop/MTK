#ifndef _SOUND_INCLUDE_SOC_H_
#define _SOUND_INCLUDE_SOC_H_
#include "sound/driver/include/core.h"
#include "sound/driver/include/compress_driver.h"
#include "sound/include/asound.h"
#include "sound/driver/include/pcm.h"
#include "sound/driver/include/soc-dpcm.h"
#include "sound/driver/include/control.h"

struct snd_soc_card;
struct snd_soc_ops;
struct snd_soc_dai;
struct snd_soc_dai_driver;
struct snd_soc_dai_link;
struct snd_widget_path;
struct snd_widget_route;
struct snd_widget;

struct snd_pcm_constraint {
    const char *stream_name;
    msd_format_t formats;
    snd_pcm_rate_t rates;
    unsigned int rate_min;
    unsigned int rate_max;
    unsigned int channels_min;
    unsigned int channels_max;
    int buffer_bytes_align;
};

struct snd_soc_ops {
    int (*startup)(struct snd_pcm_stream *);
    void (*shutdown)(struct snd_pcm_stream *);
    int (*hw_params)(struct snd_pcm_stream *, struct msd_hw_params *);
    int (*hw_free)(struct snd_pcm_stream *);
    int (*prepare)(struct snd_pcm_stream *);
    int (*trigger)(struct snd_pcm_stream *, int);
};

struct snd_soc_compr_ops {
    int (*startup)(struct snd_compr_stream *);
    void (*shutdown)(struct snd_compr_stream *);
    int (*set_params)(struct snd_compr_stream *);
    int (*trigger)(struct snd_compr_stream *);
};

#define END_POINTER (0x2)

struct snd_soc_dai_link {
    const char *name;
    const char *stream_name;
    const char *cpu_dai_name;
    const char *codec_dai_name;

    unsigned int dai_fmt;
    enum snd_soc_dpcm_trigger trigger;
    int (*init)(struct snd_soc_pcm_runtime *rtd);
    int (*be_hw_params_fixup)(struct snd_soc_pcm_runtime *rtd, struct msd_hw_params *params);
    const struct snd_soc_ops *pcm_ops;
    const struct snd_soc_compr_ops *compr_ops;

    int stream_dir;
    unsigned int no_pcm;
};

struct snd_soc_control {
    struct snd_control *ctl;
    struct list_head *control_list;
};

struct snd_soc_card {
    const char *name;
    struct snd_card *snd_card;
    int instantiated;

    /* CPU<-->Codec DAI links */
    struct snd_soc_dai_link *dai_link;
    int num_links;
    struct snd_soc_pcm_runtime *rtd;
    int num_rtd;

    struct snd_soc_control *soc_ctl;

    void *drvdata;
};

struct soc_ctl_entry {
    int id;
    char *name;
    int (*get)(struct msd_ctl_value *value, void *priv);
    int (*set)(struct msd_ctl_value *value, void *priv);
    struct list_head list;
    void *private_data;
};

struct snd_soc_pcm_runtime {
    struct snd_soc_card *card;
    struct snd_soc_dai_link *dai_link;
    struct snd_soc_dpcm_runtime dpcm;

    struct snd_pcm *pcm;
    struct snd_compr *compr;
    struct snd_soc_dai *codec_dai;
    struct snd_soc_dai *cpu_dai;
};

int snd_soc_register_card(struct snd_soc_card *card);
int snd_soc_unregister_card(struct snd_soc_card *card);
int soc_new_pcm(struct snd_soc_pcm_runtime *rtd, int num);
int snd_soc_new_compress(struct snd_soc_pcm_runtime *rtd, int num);

int soc_new_control(struct snd_soc_card *card, struct list_head *ctl_list);

int snd_soc_register_dais(struct snd_soc_dai_driver *dai_drv, unsigned int num, void *priv);
int snd_soc_unregister_dais(struct snd_soc_dai_driver *dai_drv);
int snd_soc_add_controls(struct soc_ctl_entry *ctl_entry, unsigned int num, void *priv);
int snd_soc_add_widgets(struct snd_widget *widgets, unsigned int num, void *priv);
int snd_soc_add_widget_routes(struct snd_widget_route *routes, unsigned int route_num);
int snd_soc_del_widget_routes(struct snd_widget_route *routes);
void snd_soc_update_widget_route(struct snd_widget_path *path, enum dapm_prepare_event event);
void snd_soc_update_all_widget_route(int value, int event);
#include "sound/driver/include/soc-dai.h"

#endif /* #ifndef _SOUND_INCLUDE_SOC_H_ */
