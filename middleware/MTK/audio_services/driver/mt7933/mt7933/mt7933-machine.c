#include <string.h>
#include "sound/driver/include/soc.h"
#include "sound/utils/include/afe_reg_rw.h"
#include "hal_gpio_internal.h"
#include "mt7933-reg.h"
#include "mt7933-codec.h"

#include "memory_attribute.h"
//#include "mtk_pd.h"
//#include "mt7933_power.h"
#include "mt7933-afe-common.h"
#include "mtk_base_afe.h"
#include "hal_spm.h"
#include "hal_clk.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)                 (sizeof(arr) / sizeof((arr)[0]))
#endif /* #ifndef ARRAY_SIZE */

extern void *afe_probe(void);
extern int afe_remove(void *priv);
extern void *codec_probe(void);
extern int codec_remove(void *priv);
extern void *adsp_probe(void);
extern void adsp_remove(void *priv);

struct mt7933_machine_priv {
    void *codec_priv;
    void *afe_priv;
    void *adsp_priv;
    void *machine_priv;
};

struct mt7933_evb_etdm_ctrl_data {
    unsigned int mck_multp_in;
    unsigned int mck_multp_out;
    unsigned int lrck_width_in;
    unsigned int lrck_width_out;
    unsigned int fix_rate_in;
    unsigned int fix_rate_out;
    unsigned int fix_bit_width_in;
    unsigned int fix_bit_width_out;
    unsigned int fix_channels_in;
    unsigned int fix_channels_out;
    unsigned int tlv320_clock_mux;
};

struct mt7933_evb_priv {
    struct mt7933_evb_etdm_ctrl_data etdm_data[MT7933_ETDM_SETS];

    int dac_fixup_rate;
    int dac_fixup_channels;

    int adc_fixup_rate;
    int adc_fixup_channels;

    int gasrc_fixup_rate;
    int gasrc_fixup_channels;

    int hw_gain1_fixup_rate;
    int hw_gain1_fixup_channels;
};

#ifdef AUD_CODEC_SUPPORT
static int mt7933_int_dac_fixup(struct snd_soc_pcm_runtime *rtd, struct msd_hw_params *params)
{
    struct mt7933_machine_priv *machine_priv = (struct mt7933_machine_priv *)rtd->card->drvdata;
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)machine_priv->machine_priv;

    aud_dbg("channels = %d, rate = %d", machine_data->dac_fixup_channels,
            machine_data->dac_fixup_rate);

    if (machine_data->dac_fixup_channels)
        params->channels = machine_data->dac_fixup_channels;

    if (machine_data->dac_fixup_rate)
        params->rate = machine_data->dac_fixup_rate;

    return 0;
}

static int mt7933_int_adc_fixup(struct snd_soc_pcm_runtime *rtd, struct msd_hw_params *params)
{
    struct mt7933_machine_priv *machine_priv = (struct mt7933_machine_priv *)rtd->card->drvdata;
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)machine_priv->machine_priv;

    aud_dbg("channels = %d, rate = %d", machine_data->adc_fixup_channels,
            machine_data->adc_fixup_rate);

    if (machine_data->adc_fixup_channels)
        params->channels = machine_data->adc_fixup_channels;

    if (machine_data->adc_fixup_rate)
        params->rate = machine_data->adc_fixup_rate;

    return 0;
}
#endif /* #ifdef AUD_CODEC_SUPPORT */

static int mt7933_gasrc_fixup(struct snd_soc_pcm_runtime *rtd, struct msd_hw_params *params)
{
    struct mt7933_machine_priv *machine_priv = (struct mt7933_machine_priv *)rtd->card->drvdata;
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)machine_priv->machine_priv;

    aud_dbg("channels = %d, rate = %d", machine_data->gasrc_fixup_channels,
            machine_data->gasrc_fixup_rate);

    if (machine_data->gasrc_fixup_channels)
        params->channels = machine_data->gasrc_fixup_channels;

    if (machine_data->gasrc_fixup_rate)
        params->rate = machine_data->gasrc_fixup_rate;

    return 0;
}

static int mt7933_hw_gain1_fixup(struct snd_soc_pcm_runtime *rtd, struct msd_hw_params *params)
{
    struct mt7933_machine_priv *machine_priv = (struct mt7933_machine_priv *)rtd->card->drvdata;
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)machine_priv->machine_priv;

    aud_dbg("channels = %d, rate = %d", machine_data->hw_gain1_fixup_channels,
            machine_data->hw_gain1_fixup_rate);

    if (machine_data->hw_gain1_fixup_channels)
        params->channels = machine_data->hw_gain1_fixup_channels;

    if (machine_data->hw_gain1_fixup_rate)
        params->rate = machine_data->hw_gain1_fixup_rate;

    return 0;
}

#ifdef AUD_CODEC_SUPPORT
static int pin_gpio_set(hal_gpio_pin_t pin, uint8_t mode, hal_gpio_direction_t dir, hal_gpio_data_t output)
{
#ifdef CFG_GPIO_SUPPORT
    hal_pinmux_set_function(pin, mode);
    hal_gpio_set_direction(pin, dir);
    hal_gpio_set_output(pin, output);
#endif /* #ifdef CFG_GPIO_SUPPORT */
    return 0;
}

static int mt7933_machine_gpio_trigger(struct snd_pcm_stream *stream, int cmd)
{
    int pin = 0;
#ifdef IOT_SDK_DEMO_PROJECT
    pin = HAL_GPIO_39;
#else /* #ifdef IOT_SDK_DEMO_PROJECT */
    pin = HAL_GPIO_13;
#endif /* #ifdef IOT_SDK_DEMO_PROJECT */

    aud_msg("mt7933_machine_gpio_trigger, cmd %d\n", cmd);

    switch (cmd) {
        case SND_PCM_TRIGGER_START:
            pin_gpio_set(pin, 0, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_HIGH);
            break;
        case SND_PCM_TRIGGER_STOP:
            pin_gpio_set(pin, 0, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_LOW);
            break;
        default:
            break;
    }

    //  aud_dbg("%d: %d %d %d %d %d %d %d %d %d\r\n",
    //          pin,
    //          gpio_get_mode(pin),
    //          gpio_get_dir(pin),
    //          gpio_get_out(pin),
    //          gpio_get_in(pin),
    //          gpio_get_pull_enable(pin),
    //          gpio_get_pull_select(pin),
    //          gpio_get_ies_enable(pin),
    //          gpio_get_smt_enable(pin),
    //          gpio_get_driving(pin));

    return 0;
}



static struct snd_soc_ops mt7933_machine_codec_ops = {
    .trigger = mt7933_machine_gpio_trigger,
};
#endif /* #ifdef AUD_CODEC_SUPPORT */

static int mt7933_etdm_hw_params(struct snd_pcm_stream *stream, struct msd_hw_params *params);

static struct snd_soc_ops mt7933_evb_etdm_ops = {
    .hw_params = mt7933_etdm_hw_params,
};

ATTR_RWDATA_IN_RAM static struct snd_soc_dai_link mt7933_dais[] = {
    //FE
    {
        .name = "track0",
        .cpu_dai_name = "dlm-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
        .trigger = SND_SOC_DPCM_TRIGGER_POST,
    },
    {
        .name = "track1",
        .cpu_dai_name = "dl2-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
    },
    {
        .name = "track2",
        .cpu_dai_name = "dl3-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
    },
    {
        .name = "UL9",
        .cpu_dai_name = "cm9-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
    },
    {
        .name = "UL10",
        .cpu_dai_name = "ul10-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
    },
    {
        .name = "UL2",
        .cpu_dai_name = "ul2-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
    },
    {
        .name = "UL3",
        .cpu_dai_name = "ul3-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
    },
    {
        .name = "UL4",
        .cpu_dai_name = "ul4-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
    },
    {
        .name = "UL8",
        .cpu_dai_name = "ul8-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
        .trigger = SND_SOC_DPCM_TRIGGER_POST,
    },
#ifdef CONFIG_MTK_ADSP_SUPPORT
    {
        .name = "ADSP_LOCAL_RECORD",
        .cpu_dai_name = "FE_LOCAL_RECORD",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
    },
    {
        .name = "ADSP_AP_RECORD",
        .cpu_dai_name = "FE_AP_RECORD",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
    },
    {
        .name = "ADSP_HOSTLESS_VA",
        .cpu_dai_name = "FE_HOSTLESS_VA",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
    },
    {
        .name = "ADSP_VA_FE",
        .cpu_dai_name = "FE_VA",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
    },
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    {
        .name = "ADSP_HOSTLESS_PLAYBACK0",
        .cpu_dai_name = "FE_HOSTLESS_PLAYBACK0",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
    },
    {
        .name = "ADSP_HOSTLESS_PLAYBACK1",
        .cpu_dai_name = "FE_HOSTLESS_PLAYBACK1",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
    },
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    //BE
    {
        .name = "GASRC0_P",
        .cpu_dai_name = "GASRC0",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
        .be_hw_params_fixup = mt7933_gasrc_fixup,
        .no_pcm = 1,
    },
    {
        .name = "GASRC0_C",
        .cpu_dai_name = "GASRC0",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
        .be_hw_params_fixup = mt7933_gasrc_fixup,
        .no_pcm = 1,
    },
#ifdef AUD_CODEC_SUPPORT
    {
        .name = "INTDAC_out",
        .stream_name = "INTDAC Playback",
        .cpu_dai_name = "INT ADDA Playback",
        .codec_dai_name = "mt7933-codec-dai-playback",
        .stream_dir = MSD_STREAM_PLAYBACK,
        .be_hw_params_fixup = mt7933_int_dac_fixup,
        .no_pcm = 1 | END_POINTER,
        .pcm_ops = &mt7933_machine_codec_ops,
    },
    {
        .name = "INTADC_in",
        .cpu_dai_name = "INT ADDA Capture",
        .codec_dai_name = "mt7933-codec-dai-capture",
        .stream_dir = MSD_STREAM_CAPTURE,
        .be_hw_params_fixup = mt7933_int_adc_fixup,
        .no_pcm = 1 | END_POINTER,
    },
#endif /* #ifdef AUD_CODEC_SUPPORT */
    {
        .name = "ETDM1_IN_BE",
        .cpu_dai_name = "etdm1-in-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
        .no_pcm = 1 | END_POINTER,
        .dai_fmt = MSD_SOC_DAIFMT_I2S |
        MSD_SOC_DAIFMT_NB_NF |
        MSD_SOC_DAIFMT_CBS_CFS,
        .pcm_ops = &mt7933_evb_etdm_ops
    },
    {
        .name = "ETDM2_OUT_BE",
        .cpu_dai_name = "etdm2-out-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
        .no_pcm = 1 | END_POINTER,
        .dai_fmt = MSD_SOC_DAIFMT_I2S |
        MSD_SOC_DAIFMT_NB_NF |
        MSD_SOC_DAIFMT_CBS_CFS,
        .pcm_ops = &mt7933_evb_etdm_ops
    },
    {
        .name = "ETDM2_IN_BE",
        .cpu_dai_name = "etdm2-in-cpu-dai",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
        .no_pcm = 1 | END_POINTER,
        .dai_fmt = MSD_SOC_DAIFMT_I2S |
        MSD_SOC_DAIFMT_NB_NF |
        MSD_SOC_DAIFMT_CBS_CFS,
        .pcm_ops = &mt7933_evb_etdm_ops,
    },
    {
        .name = "DMIC_BE",
        .cpu_dai_name = "DMIC",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
        .no_pcm = 1 | END_POINTER,
    },
    {
        .name = "HW_Gain1_P",
        .cpu_dai_name = "HW_GAIN1",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
        .be_hw_params_fixup = mt7933_hw_gain1_fixup,
        .no_pcm = 1,
    },
    {
        .name = "HW_Gain1_C",
        .cpu_dai_name = "HW_GAIN1",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
        .be_hw_params_fixup = mt7933_hw_gain1_fixup,
        .no_pcm = 1,
    },
    {
        .name = "dummy_end_p",
        .cpu_dai_name = "snd-soc-dummy",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
        .no_pcm = 1 | END_POINTER,
    },
    {
        .name = "dummy_end_c",
        .cpu_dai_name = "snd-soc-dummy",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
        .no_pcm = 1 | END_POINTER,
    },
#ifdef CONFIG_MTK_ADSP_SUPPORT
    {
        .name = "ADSP_UL9_IN_BE",
        .cpu_dai_name = "BE_UL9_IN Capture",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
        .no_pcm = 1,
    },
    {
        .name = "ADSP_UL2_IN_BE",
        .cpu_dai_name = "BE_UL2_IN Capture",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_CAPTURE,
        .no_pcm = 1,
    },
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    {
        .name = "ADSP_DLM_OUT_BE",
        .cpu_dai_name = "BE_DLM_OUT Playback",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
        .no_pcm = 1,
    },
    {
        .name = "ADSP_DL2_OUT_BE",
        .cpu_dai_name = "BE_DL2_OUT Playback",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
        .no_pcm = 1,
    },
    {
        .name = "ADSP_DL3_OUT_BE",
        .cpu_dai_name = "BE_DL3_OUT Playback",
        .codec_dai_name = "snd-soc-dummy",
        .stream_dir = MSD_STREAM_PLAYBACK,
        .no_pcm = 1,
    },
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
};

static int mt7933_intdac_fixup_param_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)priv;

    ctl_value->integer.value[0] = machine_data->dac_fixup_rate;
    ctl_value->integer.value[1] = machine_data->dac_fixup_channels;

    return 0;
}

static int mt7933_intdac_fixup_param_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)priv;
    machine_data->dac_fixup_rate = ctl_value->integer.value[0];
    machine_data->dac_fixup_channels = ctl_value->integer.value[1];

    aud_dbg("samplerate = %d, channels = %d", machine_data->dac_fixup_rate,
            machine_data->dac_fixup_channels);
    return 0;
}

static int mt7933_intadc_fixup_param_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)priv;

    ctl_value->integer.value[0] = machine_data->adc_fixup_rate;
    ctl_value->integer.value[1] = machine_data->adc_fixup_channels;

    return 0;
}

static int mt7933_intadc_fixup_param_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)priv;
    machine_data->adc_fixup_rate = ctl_value->integer.value[0];
    machine_data->adc_fixup_channels = ctl_value->integer.value[1];

    aud_dbg("samplerate = %d, channels = %d", machine_data->adc_fixup_rate,
            machine_data->adc_fixup_channels);
    return 0;
}

static int mt7933_gasrc_fixup_param_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)priv;

    ctl_value->integer.value[0] = machine_data->gasrc_fixup_rate;
    ctl_value->integer.value[1] = machine_data->gasrc_fixup_channels;

    return 0;
}

static int mt7933_gasrc_fixup_param_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)priv;
    machine_data->gasrc_fixup_rate = ctl_value->integer.value[0];
    machine_data->gasrc_fixup_channels = ctl_value->integer.value[1];

    aud_dbg("samplerate = %d, channels = %d", machine_data->gasrc_fixup_rate,
            machine_data->gasrc_fixup_channels);
    return 0;
}

static int mt7933_hw_gain1_fixup_param_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)priv;

    ctl_value->integer.value[0] = machine_data->hw_gain1_fixup_rate;
    ctl_value->integer.value[1] = machine_data->hw_gain1_fixup_channels;

    return 0;
}

static int mt7933_hw_gain1_fixup_param_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_evb_priv *machine_data = (struct mt7933_evb_priv *)priv;
    machine_data->hw_gain1_fixup_rate = ctl_value->integer.value[0];
    machine_data->hw_gain1_fixup_channels = ctl_value->integer.value[1];

    aud_dbg("samplerate = %d, channels = %d", machine_data->hw_gain1_fixup_rate,
            machine_data->hw_gain1_fixup_channels);
    return 0;
}

static struct soc_ctl_entry mt7933_machine_controls[] = {
    {
        .name = "INTDAC_Fixup_Param",
        .get = mt7933_intdac_fixup_param_get,
        .set = mt7933_intdac_fixup_param_put,
    },
    {
        .name = "INTADC_Fixup_Param",
        .get = mt7933_intadc_fixup_param_get,
        .set = mt7933_intadc_fixup_param_put,
    },
    {
        .name = "GASRC_Fixup_Param",
        .get = mt7933_gasrc_fixup_param_get,
        .set = mt7933_gasrc_fixup_param_put,
    },
    {
        .name = "HW_GAIN1_Fixup_Param",
        .get = mt7933_hw_gain1_fixup_param_get,
        .set = mt7933_hw_gain1_fixup_param_put,
    },
};

struct snd_soc_card mt7933_card = {
    .name = "mt-snd_card",
    .dai_link = mt7933_dais,
    .num_links = sizeof(mt7933_dais) / sizeof(mt7933_dais[0]),
};

static int mt7933_etdm_hw_params(struct snd_pcm_stream *stream, struct msd_hw_params *params)
{
    //  struct snd_soc_card *card = &mt7933_card;
    //  struct mt7933_machine_priv *machine_priv = (struct mt7933_machine_priv *)card->drvdata;
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    //  struct mt7933_evb_priv *priv = (struct mt7933_evb_priv *)machine_priv->machine_priv;
    //  struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    //  int id = cpu_dai->driver->id;
    //  struct mt7933_evb_etdm_ctrl_data *etdm;
    //  struct snd_soc_dai *codec_dai;

    //  unsigned int mclk_multiplier = 0;
    //  unsigned int mclk = 0;
    //  unsigned int lrck_width = 0;
    //  int slot = 0;
    //  int slot_width = 0;
    //  unsigned int slot_bitmask = 0;
    //  unsigned int tlv320_clock_source = 0;
    //  unsigned int idx;
    //  int ret;

    //  if (id == MT7933_AFE_IO_ETDM1_IN) {
    //      idx = MT7933_ETDM1;
    //      etdm = &priv->etdm_data[idx];
    //      mclk_multiplier = etdm->mck_multp_in;
    //      lrck_width = etdm->lrck_width_in;
    //      tlv320_clock_source = etdm->tlv320_clock_mux;
    //  } else if (id == MT7933_AFE_IO_ETDM2_OUT) {
    //      idx = MT7933_ETDM2;
    //      etdm = &priv->etdm_data[idx];
    //      mclk_multiplier = etdm->mck_multp_out;
    //      lrck_width = etdm->lrck_width_out;
    //  } else if (id == MT7933_AFE_IO_ETDM2_IN) {
    //      idx = MT7933_ETDM2;
    //      etdm = &priv->etdm_data[idx];
    //      mclk_multiplier = etdm->mck_multp_in;
    //      lrck_width = etdm->lrck_width_in;
    //  }

    //  if (mclk_multiplier > 0) {
    //      mclk = mclk_multiplier * params->rate;

    //      ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk,
    //                       MSD_SOC_CLOCK_OUT);
    //      if (ret)
    //          return ret;
    //  }

    //  slot_width = lrck_width;
    //  if (slot_width > 0) {
    //      slot = params->channels;
    //      slot_bitmask = GENMASK(slot - 1, 0);

    //      ret = snd_soc_dai_set_tdm_slot(cpu_dai,
    //                         slot_bitmask,
    //                         slot_bitmask,
    //                         slot,
    //                         slot_width);

    //      if (ret)
    //          return ret;
    //  }

    //  if (tlv320_clock_source) {

    //      codec_dai = rtd->codec_dai;

    //      if (codec_dai == NULL) {
    //          aud_error("tlv320: invalid dai parameter");
    //          return -EINVAL;
    //      }

    //      snd_soc_dai_set_pll(codec_dai, 0, tlv320_clock_source,
    //          snd_pcm_format_width(params->format)
    //          * params->channels * params->rate
    //          , params->rate);

    //      snd_soc_dai_set_fmt(codec_dai, MSD_SOC_DAIFMT_CBS_CFS |
    //              MSD_SOC_DAIFMT_DSP_B | MSD_SOC_DAIFMT_NB_NF);

    //  }
    return 0;
}

static void *machine_evb_probe(void)
{
    struct mt7933_evb_priv *machine_evb_priv = malloc(sizeof(*machine_evb_priv));

    if (machine_evb_priv == NULL) {
        return NULL;
    }

    memset(machine_evb_priv, 0, sizeof(struct mt7933_evb_priv));

    snd_soc_add_controls(mt7933_machine_controls,
                         ARRAY_SIZE(mt7933_machine_controls),
                         machine_evb_priv);

    return machine_evb_priv;
}

int machine_evb_remove(void *priv)
{
    struct mt7933_evb_priv *machine_evb_priv = (struct mt7933_evb_priv *)priv;
    free(machine_evb_priv);
    return 0;
}

struct gpio_table {
    hal_gpio_pin_t gpio_pin;
    uint8_t gpio_mod;
};

static const struct gpio_table gpio_tables[] = {
    {HAL_GPIO_15, 3},               // I2SO_MCK
    {HAL_GPIO_14, 3},               // I2SO_LRCK
    {HAL_GPIO_13, 3},               // I2SO_BCK
    {HAL_GPIO_11, 3},               // I2SO_DAT0

    {HAL_GPIO_16, 4},               // I2SIN_MCK
    {HAL_GPIO_19, 4},               // I2SIN_LRCK
    {HAL_GPIO_20, 4},               // I2SIN_BCK
    {HAL_GPIO_10, 3},               // I2SIN_DAT0

#if 1
    {HAL_GPIO_17, 3},               // TDMIN_MCK
    {HAL_GPIO_18, 3},               // TDMIN_BCK
    {HAL_GPIO_21, 3},               // TDMIN_LRCK
    {HAL_GPIO_22, 3},               // TDMIN_DI
#else /* #if 1 */
    {HAL_GPIO_17, 4},               // DMIC_CLK0
    {HAL_GPIO_18, 4},               // DMIC_DAT0
    {HAL_GPIO_21, 4},               // DMIC_DAT1
    {HAL_GPIO_24, 4},               // DMIC_CLK1
    {HAL_GPIO_22, 4},               // DMIC_DAT2
    {HAL_GPIO_23, 4},               // DMIC_DAT3
#endif /* #if 1 */
};

static void mt7933_machine_gpio_probe(const struct gpio_table *tables, unsigned int num)
{
    //  unsigned int index;
    //  for (index = 0; index < num; ++index) {
    //      hal_pinmux_set_function(tables[index].gpio_pin, tables[index].gpio_mod);
    //  }
#ifdef IOT_SDK_DEMO_PROJECT
#ifdef AUD_CODEC_SUPPORT
    int pin = HAL_GPIO_39;
    pin_gpio_set(pin, 0, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_LOW);
#endif /* #ifdef AUD_CODEC_SUPPORT */
#endif /* #ifdef IOT_SDK_DEMO_PROJECT */
}

int mt7933_dev_probe(void)
{
    int index;
    struct snd_soc_card *card = &mt7933_card;
    struct mt7933_machine_priv *priv = malloc(sizeof(*priv));
    if (priv == NULL) {
        aud_error("out of memory.");
        return -ENOMEM;
    }
    memset(priv, 0, sizeof(*priv));

    /* open audio power domain */
    MTCMOS_PWR_ON_AUDIO_AO;
    MTCMOS_PWR_ON_AUDIO_AFE;
    hal_clock_enable(HAL_CLOCK_CG_AUDIO_XTAL);
    hal_clock_enable(HAL_CLOCK_CG_TOP_PLL);
    hal_clock_enable(HAL_CLOCK_CG_XPLL);
    hal_clock_enable(HAL_CLOCK_CG_AUDSYS_BUS);

    for (index = 0; index < 4; index++) {
        SRAM_PWR_ON_AUDIO(index);
    }

    aud_drv_regmap_set_size(AUDIO_REGMAP_BASE_MAX_COUNT);
    aud_drv_regmap_set_addr_range(AUDIO_REGMAP_AFE_BASE, (void *)AFE_REG_BASE, AFE_REG_SIZE);

#ifdef AUD_CODEC_SUPPORT
    aud_drv_regmap_set_addr_range(AUDIO_REGMAP_EEF_GRP2, (void *)EEF_GRP2_REG_BASE, EEF_GRP2_REG_SIZE);
    aud_drv_regmap_set_addr_range(AUDIO_REGMAP_APMIXEDSYS, (void *)APMIXED_REG_BASE, APMIXED_REG_SIZE);
    priv ->codec_priv = codec_probe();
#endif /* #ifdef AUD_CODEC_SUPPORT */
    priv->afe_priv = afe_probe();
    priv->machine_priv = machine_evb_probe();
#ifdef CONFIG_MTK_ADSP_SUPPORT
    priv->adsp_priv = adsp_probe();
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */
    card->drvdata = priv;
    if (snd_soc_register_card(card)) {
        assert(0);
    }

    mt7933_machine_gpio_probe(gpio_tables, sizeof(gpio_tables) / sizeof(gpio_tables[0]));

    return 0;
}

int mt7933_dev_remove(void)
{
    struct snd_soc_card *card = &mt7933_card;
    struct mt7933_machine_priv *priv = (struct mt7933_machine_priv *)card->drvdata;
    adsp_remove(priv->adsp_priv);
    afe_remove(priv->afe_priv);
#ifdef AUD_CODEC_SUPPORT
    codec_remove(priv->codec_priv);
#endif /* #ifdef AUD_CODEC_SUPPORT */
    machine_evb_remove(priv->machine_priv);
    snd_soc_unregister_card(card);
    free(card->drvdata);

    /* close aduio power domain */
    //  struct mtk_pd *audafe_pd, *audsrc_pd;
    //  audafe_pd = mtk_pd_get(MT7933_POWER_DOMAIN_AUDAFE);
    //  if (is_err_or_null_pd(audafe_pd))
    //      aud_error("pd_test: failed to get pd!");
    //  mtk_pd_disable(audafe_pd);
    //  audsrc_pd = mtk_pd_get(MT7933_POWER_DOMAIN_AUDSRC);
    //  if (is_err_or_null_pd(audsrc_pd))
    //      aud_error("pd_test: failed to get pd!");
    //  mtk_pd_disable(audsrc_pd);
    return 0;
}

