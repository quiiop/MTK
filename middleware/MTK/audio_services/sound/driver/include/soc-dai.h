#ifndef _SOUND_INCLUDE_SOC_DAI_H_
#define _SOUND_INCLUDE_SOC_DAI_H_

#include "sound/utils/include/list.h"
#include "sound/driver/include/soc.h"

#define MSD_SOC_DAIFMT_I2S          1
#define MSD_SOC_DAIFMT_RIGHT_J      2
#define MSD_SOC_DAIFMT_LEFT_J       3
#define MSD_SOC_DAIFMT_DSP_A        4
#define MSD_SOC_DAIFMT_DSP_B        5
#define MSD_SOC_DAIFMT_AC97         6
#define MSD_SOC_DAIFMT_PDM          7
#define MSD_SOC_DAIFMT_MSB          SND_SOC_DAIFMT_LEFT_J
#define MSD_SOC_DAIFMT_LSB          SND_SOC_DAIFMT_RIGHT_J
#define MSD_SOC_DAIFMT_NB_NF        (0 << 8)
#define MSD_SOC_DAIFMT_NB_IF        (2 << 8)
#define MSD_SOC_DAIFMT_IB_NF        (3 << 8)
#define MSD_SOC_DAIFMT_IB_IF        (4 << 8)
#define MSD_SOC_DAIFMT_CBM_CFM      (1 << 12)
#define MSD_SOC_DAIFMT_CBS_CFM      (2 << 12)
#define MSD_SOC_DAIFMT_CBM_CFS      (3 << 12)
#define MSD_SOC_DAIFMT_CBS_CFS      (4 << 12)
#define MSD_SOC_DAIFMT_FORMAT_MASK  0x000f
#define MSD_SOC_DAIFMT_CLOCK_MASK   0x00f0
#define MSD_SOC_DAIFMT_INV_MASK     0x0f00
#define MSD_SOC_DAIFMT_MASTER_MASK  0xf000

#define MSD_SOC_CLOCK_IN        0
#define MSD_SOC_CLOCK_OUT       1

struct snd_pcm_stream;
struct snd_compr_stream;
struct snd_soc_dai_driver;
struct snd_soc_dai;
struct snd_pcm_constraint;

struct snd_soc_dai_driver {
    const char *name;
    unsigned int id;

    int (*probe)(struct snd_soc_dai *dai);
    int (*remove)(struct snd_soc_dai *dai);
    int (*compress_new)(struct snd_soc_pcm_runtime *rtd, int num);
    const struct snd_soc_dai_pcm_ops *pcm_ops;
    const struct snd_soc_dai_compr_ops *compr_ops;
    struct snd_pcm_constraint constr;
};

enum snd_widget_type {
    WIDGET_SWITCH = 0x1,
    WIDGET_DAI = 0x2,
    WIDGET_DAI_LINK = 0x4,
    WIDGET_MUX = 0x8,
    WIDGET_END = 0x10,
};

#define SWITCH_IO_WIDGET(id, entry)    {#id, id, WIDGET_SWITCH, entry, {NULL, NULL}, NULL}
#define SWITCH_WIDGET(name, id, entry) {name, id, WIDGET_SWITCH, entry, {NULL, NULL}, NULL}
#define DAI_WIDGET(name)               {name, -1, WIDGET_DAI, NULL, {NULL, NULL}, NULL}
#define END_WIDGET(name)               {name, -1, WIDGET_END, NULL, {NULL, NULL}, NULL}

struct soc_ctl_entry;

struct snd_widget {
    const char *name;
    int id;
    enum snd_widget_type type;
    struct soc_ctl_entry *ctl_entry;
    struct list_head list;
    void *private_data;
};

struct snd_widget_route {
    const char *source;
    const char *sink;
    int def_value;
    struct list_head list;
};

struct snd_widget_path_list {
    struct snd_widget_path *path;
    struct list_head list;
};

struct snd_widget_path {
    struct snd_widget *source;
    struct snd_widget *sink;
    int old_value;
    int new_value;
    struct list_head list;
    struct list_head slave_path;
};

struct snd_soc_dai {
    const char *name;
    struct snd_soc_dai_driver *driver;

    unsigned int rate;
    unsigned int channels;
    unsigned int bit_width;
    int format;
    unsigned int active;
    unsigned int probed;
    struct list_head slave_path;
    struct list_head list;
    void *private_data;
};

struct snd_soc_dai_pcm_ops {
    int (*set_fmt)(struct snd_soc_dai *dai, unsigned int fmt);

    int (*startup)(struct snd_pcm_stream *, struct snd_soc_dai *);
    void (*shutdown)(struct snd_pcm_stream *, struct snd_soc_dai *);
    int (*hw_params)(struct snd_pcm_stream *, struct msd_hw_params *, struct snd_soc_dai *);
    int (*hw_free)(struct snd_pcm_stream *, struct snd_soc_dai *);
    int (*prepare)(struct snd_pcm_stream *, struct snd_soc_dai *);
    int (*trigger)(struct snd_pcm_stream *, int, struct snd_soc_dai *);
    int (*delay)(struct snd_pcm_stream *, struct snd_soc_dai *);
    unsigned long (*pointer)(struct snd_pcm_stream *, struct snd_soc_dai *);
    int (*copy)(struct snd_pcm_stream *stream, void *buf, unsigned int bytes, struct snd_soc_dai *dai);
    int (*reg_dump)(struct snd_pcm_stream *);
    int (*set_tdm_slot)(struct snd_soc_dai *dai, unsigned int tx_mask, unsigned int rx_mask, int slots, int slot_width);
    int (*set_sysclk)(struct snd_soc_dai *dai, int clk_id, unsigned int freq, int dir);
    int (*set_pll)(struct snd_soc_dai *dai, int pll_id, int source, unsigned int freq_in, unsigned int freq_out);
};

struct snd_soc_dai_compr_ops {
    int (*open)(struct snd_compr_stream *stream, struct snd_soc_dai *);
    int (*free)(struct snd_compr_stream *stream, struct snd_soc_dai *);
    int (*set_params)(struct snd_compr_stream *stream, struct snd_compr_params *params, struct snd_soc_dai *);
    int (*get_params)(struct snd_compr_stream *stream, struct snd_codec *params, struct snd_soc_dai *);
    int (*set_metadata)(struct snd_compr_stream *stream, struct snd_compr_metadata *metadata, struct snd_soc_dai *);
    int (*get_metadata)(struct snd_compr_stream *stream, struct snd_compr_metadata *metadata, struct snd_soc_dai *);
    int (*trigger)(struct snd_compr_stream *stream, int cmd, struct snd_soc_dai *dai);
    int (*pointer)(struct snd_compr_stream *stream, struct snd_compr_tstamp *tstamp, struct snd_soc_dai *);
    int (*copy)(struct snd_compr_stream *stream, char *buf, unsigned int count, struct snd_soc_dai *);
    int (*ack)(struct snd_compr_stream *stream, unsigned int count, struct snd_soc_dai *);
    int (*get_caps)(struct snd_compr_stream *stream, struct snd_compr_caps *caps, struct snd_soc_dai *);
    int (*get_codec_caps)(struct snd_compr_stream *stream, struct snd_compr_codec_caps *codec, struct snd_soc_dai *);
};

int snd_soc_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt);
int snd_soc_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id, unsigned int freq, int dir);
int snd_soc_dai_set_tdm_slot(struct snd_soc_dai *dai, unsigned int tx_mask, unsigned int rx_mask, int slots, int slot_width);
int snd_soc_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source, unsigned int freq_in, unsigned int freq_out);

#endif /* #ifndef _SOUND_INCLUDE_SOC_DAI_H_ */
