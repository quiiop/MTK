#include "sound/driver/include/soc.h"
#include "sound/utils/include/afe_reg_rw.h"
#include "freertos/snd_portable.h"
#include "mt7933-codec.h"
#include "hal_clk.h"
#include "time.h"
#include <sys/time.h>
#include "unistd.h"
#include "hal_gpt.h"

#include <string.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)                 (sizeof(arr) / sizeof((arr)[0]))
#endif /* #ifndef ARRAY_SIZE */

enum codec_adda_gain_enum_id {
    MUTE_GAIN = 0,
    ANALOG_GAIN,
    DIGITAL_GAIN,
    GAIN_MAX,
};

enum {
    MT7933_ADC0,
    MT7933_ADC1,
    MT7933_ADC_NUM,
};

enum {
    MT7933_CODEC_AUDSYS_BUS_CLK = 0,
    MT7933_CODEC_AUD_ADC0_XTAL_CLK,
    MT7933_CODEC_AUD_ADC1_XTAL_CLK,
    MT7933_CODEC_AUD_DAC_XTAL_CLK,
    MT7933_CODEC_AUD_CLK_NUM,
};

struct mt7933_codec_priv {
    int adda_afe_on_ref_cnt;
    spinlock_t adda_afe_on_lock;
    spinlock_t adda_26mclk_lock;
    hal_clock_cg_id clk[MT7933_CODEC_AUD_CLK_NUM];
    unsigned int micbias0_setup_time_us;
    unsigned int micbias0_val;
    unsigned int ul_sgen_en;
    unsigned int dl_lpbk_en;
    unsigned int dl_nle_support;
    unsigned int dl_vol_mode;/*fix or manual*/
    unsigned int dl_analog_digital_gain;/*1:digital,2:analog*/
    unsigned int dl_gain_val;
    unsigned int ul_analog_digital_gain;/*1:digital,2:analog*/
    unsigned int ul_gain_val;
    unsigned int adc_code; /*bit[0]: ADC0, bit[1]: ADC1*/
    unsigned int dac_power_on; // 1: power on ; 0: power down
    unsigned int micbias_always_on;
};

static const int codec_clks[MT7933_CODEC_AUD_CLK_NUM] = {
    [MT7933_CODEC_AUDSYS_BUS_CLK] = HAL_CLOCK_CG_AUDSYS_BUS,
    [MT7933_CODEC_AUD_ADC0_XTAL_CLK] = HAL_CLOCK_CG_AUD_ADC0_XTAL,
    [MT7933_CODEC_AUD_ADC1_XTAL_CLK] = HAL_CLOCK_CG_AUD_ADC1_XTAL,
    [MT7933_CODEC_AUD_DAC_XTAL_CLK] = HAL_CLOCK_CG_AUD_DAC_XTAL,
};

static int mt7933_codec_uplink_sgen_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;

    ctl_value->integer.value[0] = codec_data->ul_sgen_en;
    return 0;
}

static int mt7933_codec_uplink_sgen_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;
    uint32_t value = ctl_value->integer.value[0];

    if (!codec_data->ul_sgen_en && value) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      ABB_ULAFE_CON1,
                                      ABB_ULAFE_CON1_UL_SINEGEN_AMPDIV_CH1_MASK,
                                      ABB_ULAFE_CON1_UL_SINEGEN_AMPDIV_CH1_VAL(5));

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      ABB_ULAFE_CON1,
                                      ABB_ULAFE_CON1_UL_SINEGEN_AMPDIV_CH2_MASK,
                                      ABB_ULAFE_CON1_UL_SINEGEN_AMPDIV_CH2_VAL(5));

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      ABB_ULAFE_CON1,
                                      ABB_ULAFE_CON1_UL_SINEGEN_FREQDIV_CH1_MASK,
                                      ABB_ULAFE_CON1_UL_SINEGEN_FREQDIV_CH1_VAL(1));

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      ABB_ULAFE_CON1,
                                      ABB_ULAFE_CON1_UL_SINEGEN_FREQDIV_CH2_MASK,
                                      ABB_ULAFE_CON1_UL_SINEGEN_FREQDIV_CH2_VAL(2));

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      ABB_ULAFE_CON1,
                                      ABB_ULAFE_CON1_UL_SINEGEN_OUTPUT_MASK,
                                      ABB_ULAFE_CON1_UL_SINEGEN_OUTPUT);

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AUDIO_TOP_CON0,
                                      AUD_TCON0_PDN_UPLINK_TML,
                                      0);
    } else if (codec_data->ul_sgen_en && !value) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      ABB_ULAFE_CON1,
                                      ABB_ULAFE_CON1_UL_SINEGEN_OUTPUT_MASK,
                                      0x0);
    }

    codec_data->ul_sgen_en = ctl_value->integer.value[0];
    return 0;
}

static int mt7933_codec_downlink_vol_mode_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;

    ctl_value->integer.value[0] = codec_data->dl_vol_mode;
    return 0;
}

/* fix mode    :0  hardware auto adjust volume */
/* manual mode :1  software adjust volume */
static int mt7933_codec_downlink_vol_mode_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;
    uint32_t value = ctl_value->integer.value[0];

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  AFE_NLE_GAIN_IMP_LCH_CFG0,
                                  AFE_NLE_DIGITAL_GAIN_FIX_MANUAL_MODE,
                                  AFE_NLE_DIGITAL_GAIN_FIX_MANUAL_VAL(value));
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  AFE_NLE_GAIN_IMP_LCH_CFG0,
                                  AFE_NLE_ANALOG_GAIN_FIX_MANUAL_MODE,
                                  AFE_NLE_ANALOG_GAIN_FIX_MANUAL_VAL(value));
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  AFE_NLE_GAIN_IMP_RCH_CFG0,
                                  AFE_NLE_DIGITAL_GAIN_FIX_MANUAL_MODE,
                                  AFE_NLE_DIGITAL_GAIN_FIX_MANUAL_VAL(value));
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  AFE_NLE_GAIN_IMP_RCH_CFG0,
                                  AFE_NLE_ANALOG_GAIN_FIX_MANUAL_MODE,
                                  AFE_NLE_ANALOG_GAIN_FIX_MANUAL_VAL(value));

    codec_data->dl_vol_mode = ctl_value->integer.value[0];

    return 0;
}

static int mt7933_codec_downlink_vol_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;

    ctl_value->integer.value[0] = codec_data->dl_analog_digital_gain;
    ctl_value->integer.value[1] = codec_data->dl_gain_val;
    return 0;
}

/* Digital  gain: 0-> 0db  ; 1->  1db .......32->32db */
/* 'Audio Downlink Vol' 1 32 */
/* Analog gain: 0->-20db; 1->-19db .....32->12db */
/* 'Audio Downlink Vol' 2 32 */
/* Mute    gain: 0:mute ; 1:unmute */
/* 'Audio Downlink Vol' 3 0 */
/* notes: only for samll signal -40db,normal use dg 0db ag -20~2db */
static int mt7933_codec_downlink_vol_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;
    uint32_t value1 = ctl_value->integer.value[0];
    uint32_t value2 = ctl_value->integer.value[1];

    aud_msg("value1 %u, value2 %u\n", value1, value2);
    if (value1 == DIGITAL_GAIN) { /*digtial gain*/
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AFE_NLE_GAIN_IMP_LCH_CFG0,
                                      AFE_NLE_DIGITAL_GAIN_MANUAL_VAL_MASK,
                                      AFE_NLE_DIGITAL_GAIN_MANUAL_VAL(value2));
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AFE_NLE_GAIN_IMP_RCH_CFG0,
                                      AFE_NLE_DIGITAL_GAIN_MANUAL_VAL_MASK,
                                      AFE_NLE_DIGITAL_GAIN_MANUAL_VAL(value2));
    } else if (value1 == ANALOG_GAIN) { /*analog gain*/
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AFE_NLE_GAIN_IMP_LCH_CFG0,
                                      AFE_NLE_ANALOG_GAIN_MANUAL_VAL_MASK,
                                      AFE_NLE_ANALOG_GAIN_MANUAL_VAL(value2));
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AFE_NLE_GAIN_IMP_RCH_CFG0,
                                      AFE_NLE_ANALOG_GAIN_MANUAL_VAL_MASK,
                                      AFE_NLE_ANALOG_GAIN_MANUAL_VAL(value2));
    } else if (value1 == MUTE_GAIN) { /*mute gain*/
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_AFE_CON0,
                                      ABB_AFE_CON0_DL_EN_MASK,
                                      value2);
    } else {
        // todo analog gain &digital gain
        aud_msg("%s no this case!\n", __FUNCTION__);
    }

    codec_data->dl_analog_digital_gain = ctl_value->integer.value[0];
    codec_data->dl_gain_val = ctl_value->integer.value[1];

    return 0;
}

static int mt7933_codec_uplink_vol_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;

    ctl_value->integer.value[0] = codec_data->ul_analog_digital_gain;
    ctl_value->integer.value[1] = codec_data->ul_gain_val;
    return 0;
}

static int mt7933_codec_uplink_vol_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;
    uint32_t value1 = ctl_value->integer.value[0];
    uint32_t value2 = ctl_value->integer.value[1];
    int adc_id;
    aud_msg("value1 %u,value2 %u\n", value1, value2);

    if (value1 == DIGITAL_GAIN) { /*digtial gain*/
        /*ul gain not bypass*/
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      ABB_ULAFE_CON1,
                                      ABB_ULAFE_CON1_UL_GAIN_BYPASS_MASK,
                                      0x0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      ABB_UL2AFE_CON1,
                                      ABB_ULAFE_CON1_UL_GAIN_BYPASS_MASK,
                                      0x0);
        /*downstep 0.25*/
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AMIC_GAIN_CON2,
                                      AMIC_GAIN_CON2_DOWN_STEP_MASK,
                                      AMIC_GAIN_CON2_DOWN_STEP_VAL(0xF8BD));
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      UL2_AMIC_GAIN_CON2,
                                      AMIC_GAIN_CON2_DOWN_STEP_MASK,
                                      AMIC_GAIN_CON2_DOWN_STEP_VAL(0xF8BD));
        /*upstep 0.25*/
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AMIC_GAIN_CON3,
                                      AMIC_GAIN_CON3_UP_STEP_MASK,
                                      AMIC_GAIN_CON2_DOWN_STEP_VAL(0x1077A));
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      UL2_AMIC_GAIN_CON3,
                                      AMIC_GAIN_CON3_UP_STEP_MASK,
                                      AMIC_GAIN_CON2_DOWN_STEP_VAL(0x1077A));
        /*sample per step 200*/
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AMIC_GAIN_CON0,
                                      AMIC_GAIN_CON0_SAMPLE_PER_STEP_MASK,
                                      AMIC_GAIN_CON0_SAMPLE_PER_STEP_VAL(0xC8));
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      UL2_AMIC_GAIN_CON0,
                                      AMIC_GAIN_CON0_SAMPLE_PER_STEP_MASK,
                                      AMIC_GAIN_CON0_SAMPLE_PER_STEP_VAL(0xC8));
        /*target gain 20log(x/0x10000)*/
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AMIC_GAIN_CON1,
                                      AMIC_GAIN_CON1_TARGET_MASK,
                                      AMIC_GAIN_CON1_TARGET_VAL(value2));
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      UL2_AMIC_GAIN_CON1,
                                      AMIC_GAIN_CON1_TARGET_MASK,
                                      AMIC_GAIN_CON1_TARGET_VAL(value2));
        /*enable*/
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AMIC_GAIN_CON0,
                                      AMIC_GAIN_CON0_GAIN_EN_MASK,
                                      AMIC_GAIN_CON0_GAIN_ON);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      UL2_AMIC_GAIN_CON0,
                                      AMIC_GAIN_CON0_GAIN_EN_MASK,
                                      AMIC_GAIN_CON0_GAIN_ON);
    } else if (value1 == ANALOG_GAIN) { /*analog gain*/
        for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
            //          if (!(codec_data->adc_code & BIT(adc_id))) {
            //              continue;
            //          }
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_LCH_CON03 + adc_id * 0x100,
                                          RG_ADC_AUDULL_PGA_GAIN_MASK,
                                          RG_ADC_AUDULL_PGA_GAIN_VAL(value2));
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_RCH_CON03 + adc_id * 0x100,
                                          RG_ADC_AUDULR_PGA_GAIN_MASK,
                                          RG_ADC_AUDULR_PGA_GAIN_VAL(value2));
        }
    } else {
        // todo analog gain &digital gain
        aud_msg("%s no this case!\n", __FUNCTION__);
    }

    codec_data->ul_analog_digital_gain = ctl_value->integer.value[0];
    codec_data->ul_gain_val = ctl_value->integer.value[1];

    return 0;
}

static int mt7933_codec_downlink_loopback_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;

    ctl_value->integer.value[0] = codec_data->dl_lpbk_en;
    return 0;
}

static int mt7933_codec_downlink_loopback_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;
    uint32_t value = ctl_value->integer.value[0];

    if (!codec_data->dl_lpbk_en && value) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AFE_ADDA_UL_DL_CON0,
                                      AFE_ADDA_UL_DL_CON0_DL_LOOPBACK_MASK,
                                      AFE_ADDA_UL_DL_CON0_DL_LOOPBACK_ON);
    } else if (codec_data->dl_lpbk_en && !value) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AFE_ADDA_UL_DL_CON0,
                                      AFE_ADDA_UL_DL_CON0_DL_LOOPBACK_MASK,
                                      0x0);
    }

    codec_data->dl_lpbk_en = ctl_value->integer.value[0];
    return 0;
}

static int mt7933_codec_downlink_poweron_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;

    ctl_value->integer.value[0] = codec_data->dac_power_on;
    return 0;
}

int mt7933_codec_dac_depop_setup(void *priv);

int mt7933_codec_dac_depop_setdown(void *priv);

static int mt7933_codec_downlink_poweron_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;
    uint32_t value = ctl_value->integer.value[0];
    value = !!value;

    if (value) {
        mt7933_codec_dac_depop_setup(NULL);
    } else {
        mt7933_codec_dac_depop_setdown(NULL);
    }

    codec_data->dac_power_on = value;
    return 0;
}

static int mt7933_codec_micbias_alwayson_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;

    ctl_value->integer.value[0] = codec_data->micbias_always_on;
    return 0;
}

static int mt7933_codec_micbias_alwayson_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mt7933_codec_priv *codec_data = (struct mt7933_codec_priv *)priv;
    uint32_t value = ctl_value->integer.value[0];
    value = !!value;
    int adc_id = 0;

    if (value) {

        for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                          RG_ADC_LDO08_PWDB_AUDULL_MASK |
                                          RG_ADC_LDO_L_PWDB_MASK |
                                          RG_ADC_LDO08_VOSEL_AUDULL_MASK,
                                          RG_ADC_LDO08_PWDB_AUDULL_MASK |
                                          RG_ADC_LDO_L_PWDB_MASK |
                                          RG_ADC_LDO08_VOSEL_AUDULL_VAL(0x01));

            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                          RG_ADC_LDO08_PWDB_AUDULR_MASK |
                                          RG_ADC_LDO_R_PWDB_MASK |
                                          RG_ADC_LDO08_VOSEL_AUDULR_MASK,
                                          RG_ADC_LDO08_PWDB_AUDULR_MASK |
                                          RG_ADC_LDO_R_PWDB_MASK |
                                          RG_ADC_LDO08_VOSEL_AUDULR_VAL(0x01));

            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_COMMON + adc_id * 0x100,
                                          RG_ADC_AUDPWDBMICBIAS_MASK | RG_ADC_GLBIAS_EN_AUDUL_MASK,
                                          RG_ADC_AUDPWDBMICBIAS_MASK | RG_ADC_GLBIAS_EN_AUDUL_MASK);

        }
    } else {
        for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_COMMON + adc_id * 0x100,
                                          RG_ADC_AUDPWDBMICBIAS_MASK | RG_ADC_GLBIAS_EN_AUDUL_MASK,
                                          0);

            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                          RG_ADC_LDO08_PWDB_AUDULL_MASK |
                                          RG_ADC_LDO_L_PWDB_MASK,
                                          0);

            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                          RG_ADC_LDO08_PWDB_AUDULR_MASK |
                                          RG_ADC_LDO_R_PWDB_MASK,
                                          0);
        }
    }

    codec_data->micbias_always_on = value;
    return 0;
}

static int mt7933_codec_micbias_vref_get(struct msd_ctl_value *ctl_value, void *priv)
{
    uint32_t value = aud_drv_get_reg(AUDIO_REGMAP_APMIXEDSYS, AUD_ADC0_COMMON);

    value = (value >> 16) & 0x3;
    ctl_value->integer.value[0] = value;
    return 0;
}

static int mt7933_codec_micbias_vref_put(struct msd_ctl_value *ctl_value, void *priv)
{
    uint32_t value = ctl_value->integer.value[0];
    int adc_id;

    for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_COMMON + adc_id * 0x100,
                                      RG_ADC_AUDMICBIASVREF_MASK,
                                      RG_ADC_AUDMICBIASVREF_VAL(value));
    }
    return 0;
}

static int mt7933_codec_reg_dump_get(struct msd_ctl_value *ctl_value, void *priv)
{
    ctl_value->integer.value[0] = 0;
    return 0;
}

static void mt7933_codec_debug_read_msg(void);

static int mt7933_codec_reg_dump_put(struct msd_ctl_value *ctl_value, void *priv)
{
    mt7933_codec_debug_read_msg();
    return 0;
}

#if 0
/* PGA Capture Volume
 * {-6, 0, +6, +12, +18, +24} dB
 */
static const DECLARE_TLV_DB_SCALE(ul_pga_gain_tlv, -600, 600, 0);
#endif /* #if 0 */

static struct soc_ctl_entry mt7933_codec_snd_controls[] = {
    {
        /* for uplink debug */
        .name = "Audio_Uplink_SGEN",
        .get = mt7933_codec_uplink_sgen_get,
        .set = mt7933_codec_uplink_sgen_put,
    },
    {
        /* for downlink debug */
        .name = "Audio_Downlink_Loopback",
        .get = mt7933_codec_downlink_loopback_get,
        .set = mt7933_codec_downlink_loopback_put,
    },
    {
        /* Audio_DAC_AMP Vol mode */
        .name = "Audio_Downlink_Vol_Mode",
        .get = mt7933_codec_downlink_vol_mode_get,
        .set = mt7933_codec_downlink_vol_mode_put,
    },
    {
        /* Audio_DAC_AMP Vol */
        .name = "Audio_Downlink_Vol",
        .get = mt7933_codec_downlink_vol_get,
        .set = mt7933_codec_downlink_vol_put,
    },
    {
        /* Audio_ADC Vol :655360:20db*/
        .name = "Audio_Uplink_Vol",
        .get = mt7933_codec_uplink_vol_get,
        .set = mt7933_codec_uplink_vol_put,
    },
    {
        .name = "Audio_Downlink_Power",
        .get = mt7933_codec_downlink_poweron_get,
        .set = mt7933_codec_downlink_poweron_put,
    },
    {
        .name = "Audio_MICBIAS_ALWAYSON",
        .get = mt7933_codec_micbias_alwayson_get,
        .set = mt7933_codec_micbias_alwayson_put,
    },
    {
        .name = "Audio_Micbias_VREF",
        .get = mt7933_codec_micbias_vref_get,
        .set = mt7933_codec_micbias_vref_put,
    },
    {
        .name = "Audio_Codec_Reg_Dump",
        .get = mt7933_codec_reg_dump_get,
        .set = mt7933_codec_reg_dump_put,
    }
};

static int mt7933_codec_enable_adda_afe_on(void *priv)
{
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    spin_lock_flags_define(flags);

    spin_lock_irqsave(&mt7933_codec->adda_afe_on_lock, flags);

    mt7933_codec->adda_afe_on_ref_cnt++;
    if (mt7933_codec->adda_afe_on_ref_cnt == 1)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AFE_ADDA_UL_DL_CON0, 0x1, 0x1);

    spin_unlock_irqrestore(&mt7933_codec->adda_afe_on_lock, flags);

    return 0;
}

//static int mt7933_codec_disable_adda_afe_on(void* priv)
//{
//  struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv*)priv;
//  spin_lock_flags_define(flags);

//  spin_lock_irqsave(&mt7933_codec->adda_afe_on_lock, flags);

//  mt7933_codec->adda_afe_on_ref_cnt--;
//  if (mt7933_codec->adda_afe_on_ref_cnt == 0)
//      aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
//              AFE_ADDA_UL_DL_CON0, 0x1, 0x0);
//  else if (mt7933_codec->adda_afe_on_ref_cnt < 0)
//      mt7933_codec->adda_afe_on_ref_cnt = 0;

//  spin_unlock_irqrestore(&mt7933_codec->adda_afe_on_lock, flags);

//  return 0;
//}

static int mt7933_codec_dac_powerdown(void *priv)
{
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_CLK26MHZ_DIV4_EN_MASK,
                                  0x0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_CK_6P5M_FIFO_EN_MASK,
                                  0x0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON02,
                                  RG_ENDP_MASK,
                                  RG_ENDP_MASK);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON02,
                                  RG_DP_S1_MASK,
                                  RG_DP_S1_MASK);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON03,
                                  RG_DP_RAMP_SEL_MASK,
                                  RG_DP_RAMP_SEL_VAL(0x1));
    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON02,
                                  RG_ENVO_MASK,
                                  0x0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON02,
                                  RG_DP_S0_MASK,
                                  0x0);
    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_V2I_PWDB_MASK,
                                  0x0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_IDACR_PWDB_MASK,
                                  0x0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_IDACL_PWDB_MASK,
                                  0x0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_RELATCH_EN_MASK,
                                  0x0);

    return 0;
}

static int mt7933_codec_adc_powerdown(void *priv)
{
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    int adc_id = 0;

    for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
        if (!(mt7933_codec->adc_code & BIT(adc_id))) {
            continue;
        }
        aud_msg("adc_id = %d", adc_id);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_DITHER_EN_MASK | RG_ADC_AUDULL_DITHER_RESETB_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_DITHER_EN_MASK | RG_ADC_AUDULR_DITHER_RESETB_MASK,
                                      0);

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_VREF_VCM_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_VREF_VCM_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_VREF_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_VREF_EN_MASK,
                                      0);

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_SARADC_RSTB_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_SARADC_RSTB_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_SARADC_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_SARADC_EN_MASK,
                                      0);

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_PGA_PWDB_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_PGA_PWDB_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON03 + adc_id * 0x100,
                                      RG_ADC_AUDULL_PGA_CHP_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON03 + adc_id * 0x100,
                                      RG_ADC_AUDULR_PGA_CHP_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON03 + adc_id * 0x100,
                                      RG_ADC_AUDULL_PGA_NEGR_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON03 + adc_id * 0x100,
                                      RG_ADC_AUDULR_PGA_NEGR_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON03 + adc_id * 0x100,
                                      RG_ADC_AUDULL_PGA_SDR_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON03 + adc_id * 0x100,
                                      RG_ADC_AUDULR_PGA_SDR_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_PGAOUTPUT_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_PGAOUTPUT_EN_MASK,
                                      0);

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_INT_CHP_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_INT_CHP_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_PDN_INT1OP_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_PDN_INT1OP_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_INT2_RESETB_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_INT2_RESETB_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULL_INT1_RESETB_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                      RG_ADC_AUDULR_INT1_RESETB_MASK,
                                      0);

        if (!mt7933_codec->micbias_always_on) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_COMMON + adc_id * 0x100,
                                          RG_ADC_AUDPWDBMICBIAS_MASK,
                                          0);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_COMMON + adc_id * 0x100,
                                          RG_ADC_GLBIAS_EN_AUDUL_MASK,
                                          0);
        }
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_COMMON + adc_id * 0x100,
                                      RG_ADC_CLK_SEL_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_COMMON + adc_id * 0x100,
                                      RG_ADC_CLK_EN_MASK,
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                      AUD_ADC0_RSVD + adc_id * 0x100,
                                      RG_ADC_AUDUL_REV_MASK,
                                      0);
        if (!mt7933_codec->micbias_always_on) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                          RG_ADC_LDO_L_PWDB_MASK,
                                          0);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                          RG_ADC_LDO_R_PWDB_MASK,
                                          0);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                          RG_ADC_LDO08_PWDB_AUDULL_MASK,
                                          0);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                          RG_ADC_LDO08_PWDB_AUDULR_MASK,
                                          0);
        }

    }

    return 0;
}

int mt7933_codec_dac_depop_setup(void *priv)
{
    /*LDO Power Low avoid pop noise*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON04,
                                  RG_DP_PL_EN_MASK,
                                  RG_DP_PL_EN_MASK);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON04,
                                  RG_DP_PL_SEL_MASK,
                                  RG_DP_PL_SEL_MASK);
    hal_gpt_delay_us(11);

    /*LDO Enable*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_LDO1P8_ADAC_EN_MASK,
                                  RG_LDO1P8_ADAC_EN_MASK);

    uint32_t efuse;
    uint32_t vosel_val;

    aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUD_DAC, &efuse);

    if (efuse & RG_AUD_DAC_VALID_MASK) {
        vosel_val = (efuse & AUD_DAC_RG_LDO1P8_ADAC_VOSEL_MASK) >> 4;
    } else {
        vosel_val = 0x4;
    }

    aud_msg("vosel_val = %x", vosel_val);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_LDO1P8_ADAC_VOSEL_MASK,
                                  RG_LDO1P8_ADAC_VOSEL_VAL(vosel_val));
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON04,
                                  RG_LDO_LAT_EN_MASK,
                                  RG_LDO_LAT_EN_MASK);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON04,
                                  RG_LDO_LAT_IQSEL_MASK,
                                  RG_LDO_LAT_IQSEL_VAL(0x2)); /*60uA*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON04,
                                  RG_LDO_LAT_VOSEL_MASK,
                                  RG_LDO_LAT_VOSEL_VAL(0x0)); /*0.9v*/
    hal_gpt_delay_us(11);

    /* VCM RAMP UP*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_VMID_PWDB_MASK,
                                  RG_VMID_PWDB_MASK);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_VMID_FASTUP_EN_MASK,
                                  RG_VMID_FASTUP_EN_MASK);
    hal_gpt_delay_us(51000); /*50ms*/

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_VMID_FASTUP_EN_MASK,
                                  0x0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_VMID_RSEL_MASK,
                                  RG_VMID_RSEL_VAL(0x2));
    hal_gpt_delay_us(11);

    /*depop*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_GLBIAS_ADAC_EN_MASK,
                                  RG_GLBIAS_ADAC_EN_MASK);
    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON02,
                                  RG_DP_S0_MASK,
                                  0x0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON02,
                                  RG_DP_S1_MASK,
                                  RG_DP_S1_MASK);
    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON02,
                                  RG_ENDP_MASK,
                                  RG_ENDP_MASK);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON02,
                                  RG_ENVO_MASK,
                                  0x0);
    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON01,
                                  RG_I2VL_PWDB_MASK,
                                  RG_I2VL_PWDB_MASK);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON01,
                                  RG_I2VR_PWDB_MASK,
                                  RG_I2VR_PWDB_MASK);
    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_CLK_INV_MASK,
                                  RG_CLK_INV_MASK); /* 0.5T */
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON04,
                                  RG_REV_ADAC1_MASK,
                                  RG_REV_ADAC1_MASK);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON03,
                                  RG_DEPOP_RAMPGEN_CAP_RSTB_MASK,
                                  RG_DEPOP_RAMPGEN_CAP_RSTB_MASK);
    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON03,
                                  RG_DEPOP_RAMPGEN_EN_MASK,
                                  RG_DEPOP_RAMPGEN_EN_MASK);

    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON04,
                                  RG_DP_PL_SEL_MASK,
                                  0x0);

    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_CLK26MHZ_EN_MASK,
                                  RG_CLK26MHZ_EN_MASK);

    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON03,
                                  RG_DEPOP_CLK_EN_MASK,
                                  RG_DEPOP_CLK_EN_MASK);

    hal_gpt_delay_us(11);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON03,
                                  RG_DEPOP_RAMPGEN_START_MASK,
                                  0x0);

    hal_gpt_delay_us(2001000);

    return 0;
}

int mt7933_codec_dac_depop_setdown(void *priv)
{
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_LDO1P8_ADAC_EN_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON04,
                                  RG_LDO_LAT_EN_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_VMID_PWDB_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_GLBIAS_ADAC_EN_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON01,
                                  RG_I2VL_PWDB_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON01,
                                  RG_I2VR_PWDB_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_CLK26MHZ_EN_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON04,
                                  RG_DP_PL_EN_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON03,
                                  RG_DEPOP_RAMPGEN_EN_MASK,
                                  0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON03,
                                  RG_DEPOP_CLK_EN_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_V2I_PWDB_MASK,
                                  0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_IDACL_PWDB_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_IDACR_PWDB_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_RELATCH_EN_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_CK_6P5M_FIFO_EN_MASK,
                                  0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                  AUD_DAC_CON00,
                                  RG_CLK26MHZ_DIV4_EN_MASK,
                                  0);

    return 0;
}

static int mt7933_codec_adc_setup(void *priv)
{
    /*7933 must disable loopback(DAC to ADC)*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_ULAFE_CON0,
                                  ABB_ULAFE_CON0_AD_DA_LOOPBACK_DIS_MASK,
                                  ABB_ULAFE_CON0_AD_DA_LOOPBACK_DIS);

    /*for performance,need bypass digtial gain*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_ULAFE_CON1,
                                  ABB_ULAFE_CON1_UL_GAIN_BYPASS_MASK,
                                  ABB_ULAFE_CON1_UL_GAIN_NO_BYPASS);

    /*for remove DC offset*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_ULAFE_CON0,
                                  ABB_ULAFE_CON0_UL_IIR_ON_MASK,
                                  ABB_ULAFE_CON0_UL_IIR_ON);

    // for ADC1
    /*7933 must disable loopback(DAC to ADC)*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_UL2AFE_CON0,
                                  ABB_ULAFE_CON0_AD_DA_LOOPBACK_DIS_MASK,
                                  ABB_ULAFE_CON0_AD_DA_LOOPBACK_DIS);

    /*for performance,need bypass digtial gain*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_UL2AFE_CON1,
                                  ABB_ULAFE_CON1_UL_GAIN_BYPASS_MASK,
                                  ABB_ULAFE_CON1_UL_GAIN_NO_BYPASS);

    /*for remove DC offset*/
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_UL2AFE_CON0,
                                  ABB_ULAFE_CON0_UL_IIR_ON_MASK,
                                  ABB_ULAFE_CON0_UL_IIR_ON);

    return 0;
}

static int mt7933_codec_ana_dac_clk_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    aud_dbg("%d", event);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON02,
                                          RG_DP_S0_MASK,
                                          RG_DP_S0_MASK);
            hal_gpt_delay_us(11);

            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON02,
                                          RG_ENVO_MASK,
                                          RG_ENVO_MASK);
            hal_gpt_delay_us(11);

            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON02,
                                          RG_ENDP_MASK,
                                          0x0);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON02,
                                          RG_DP_S1_MASK,
                                          0x0);
            hal_gpt_delay_us(11);

            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON01,
                                          RG_EN_VCMBUF_MASK,
                                          RG_EN_VCMBUF_MASK);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON01,
                                          RG_VCMB_SEL_MASK,
                                          RG_VCMB_SEL_MASK);

            hal_gpt_delay_us(11);

            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON04,
                                          RG_DP_PL_EN_MASK,
                                          0x0);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON03,
                                          RG_DEPOP_RAMPGEN_EN_MASK,
                                          0x0);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON03,
                                          RG_DEPOP_CLK_EN_MASK,
                                          0x0);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON03,
                                          RG_DP_RAMP_SEL_MASK,
                                          RG_DP_RAMP_SEL_VAL(0x0));

            hal_gpt_delay_us(11);

            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON00,
                                          RG_RELATCH_EN_MASK,
                                          RG_RELATCH_EN_MASK);

            hal_gpt_delay_us(11);

            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON00,
                                          RG_CLK26MHZ_DIV4_EN_MASK,
                                          RG_CLK26MHZ_DIV4_EN_MASK);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON00,
                                          RG_CK_6P5M_FIFO_EN_MASK,
                                          RG_CK_6P5M_FIFO_EN_MASK);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON00,
                                          RG_V2I_PWDB_MASK,
                                          RG_V2I_PWDB_MASK);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON00,
                                          RG_IDACR_PWDB_MASK,
                                          RG_IDACR_PWDB_MASK);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                          AUD_DAC_CON00,
                                          RG_IDACL_PWDB_MASK,
                                          RG_IDACL_PWDB_MASK);

            break;

        default:
            return 0;
    }

    return 0;
}

static int mt7933_codec_ana_adc_clk_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    int adc_id = 0;
    aud_dbg("event: %d, adc_code = %x", event, mt7933_codec->adc_code);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
                if (!(mt7933_codec->adc_code & BIT(adc_id))) {
                    continue;
                }
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RSVD + adc_id * 0x100,
                                              RG_ADC_AUDUL_REV_MASK,
                                              RG_ADC_AUDUL_REV_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_COMMON + adc_id * 0x100,
                                              RG_ADC_CLK_EN_MASK,
                                              RG_ADC_CLK_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_COMMON + adc_id * 0x100,
                                              RG_ADC_CLK_SEL_MASK,
                                              RG_ADC_CLK_SEL_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_COMMON + adc_id * 0x100,
                                              RG_ADC_GLBIAS_EN_AUDUL_MASK,
                                              RG_ADC_GLBIAS_EN_AUDUL_MASK);
            }
            break;
        default:
            return 0;
    }

    return 0;
}

static int mt7933_codec_ana_adc_int1_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    int adc_id = 0;
    aud_dbg("event: %d, adc_code = %x", event, mt7933_codec->adc_code);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
                if (!(mt7933_codec->adc_code & BIT(adc_id))) {
                    continue;
                }
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_INT1_RESETB_MASK,
                                              RG_ADC_AUDULL_INT1_RESETB_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_INT1_RESETB_MASK,
                                              RG_ADC_AUDULR_INT1_RESETB_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_INT_CHP_EN_MASK,
                                              RG_ADC_AUDULL_INT_CHP_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_INT_CHP_EN_MASK,
                                              RG_ADC_AUDULR_INT_CHP_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_PDN_INT1OP_MASK,
                                              RG_ADC_AUDULL_PDN_INT1OP_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_PDN_INT1OP_MASK,
                                              RG_ADC_AUDULR_PDN_INT1OP_MASK);
            }
            break;
        default:
            return 0;
    }

    return 0;
}

static int mt7933_codec_ana_adc_int2_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    int adc_id = 0;
    aud_dbg("event: %d, adc_code = %x", event, mt7933_codec->adc_code);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
                if (!(mt7933_codec->adc_code & BIT(adc_id))) {
                    continue;
                }
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_INT2_RESETB_MASK,
                                              RG_ADC_AUDULL_INT2_RESETB_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_INT2_RESETB_MASK,
                                              RG_ADC_AUDULR_INT2_RESETB_MASK);
            }
            break;
        default:
            return 0;
    }

    return 0;
}

static int mt7933_codec_ana_adc_saradc_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    int adc_id = 0;
    aud_dbg("event: %d, adc_code = %x", event, mt7933_codec->adc_code);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
                if (!(mt7933_codec->adc_code & BIT(adc_id))) {
                    continue;
                }
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON05 + adc_id * 0x100,
                                              RG_ADC_AUDULL_SARADC_CTRL_SEL_MASK,
                                              RG_ADC_AUDULL_SARADC_CTRL_SEL_VAL(0x3));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON05 + adc_id * 0x100,
                                              RG_ADC_AUDULR_SARADC_CTRL_SEL_MASK,
                                              RG_ADC_AUDULR_SARADC_CTRL_SEL_VAL(0x3));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON05 + adc_id * 0x100,
                                              RG_ADC_AUDULL_SARADC_DEC_DLY_SEL_MASK,
                                              RG_ADC_AUDULL_SARADC_DEC_DLY_SEL_VAL(0x1));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON05 + adc_id * 0x100,
                                              RG_ADC_AUDULR_SARADC_DEC_DLY_SEL_MASK,
                                              RG_ADC_AUDULR_SARADC_DEC_DLY_SEL_VAL(0x1));

                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_SARADC_EN_MASK,
                                              RG_ADC_AUDULL_SARADC_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_SARADC_EN_MASK,
                                              RG_ADC_AUDULR_SARADC_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_SARADC_RSTB_MASK,
                                              RG_ADC_AUDULL_SARADC_RSTB_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_SARADC_RSTB_MASK,
                                              RG_ADC_AUDULR_SARADC_RSTB_MASK);

                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON05 + adc_id * 0x100,
                                              RG_ADC_AUDULL_SARADC_SA_DLY_SEL_MASK,
                                              RG_ADC_AUDULL_SARADC_SA_DLY_SEL_VAL(0x1));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON05 + adc_id * 0x100,
                                              RG_ADC_AUDULR_SARADC_SA_DLY_SEL_MASK,
                                              RG_ADC_AUDULR_SARADC_SA_DLY_SEL_VAL(0x1));

                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON05 + adc_id * 0x100,
                                              RG_ADC_AUDULL_SARADC_SEL_MASK,
                                              RG_ADC_AUDULL_SARADC_SEL_VAL(0x0));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON05 + adc_id * 0x100,
                                              RG_ADC_AUDULR_SARADC_SEL_MASK,
                                              RG_ADC_AUDULR_SARADC_SEL_VAL(0x0));

                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON05 + adc_id * 0x100,
                                              RG_ADC_AUDULL_SARADC_VREF_ISEL_MASK,
                                              RG_ADC_AUDULL_SARADC_VREF_ISEL_VAL(0x3));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON05 + adc_id * 0x100,
                                              RG_ADC_AUDULR_SARADC_VREF_ISEL_MASK,
                                              RG_ADC_AUDULR_SARADC_VREF_ISEL_VAL(0x3));
            }
            break;
        default:
            return 0;
    }

    return 0;
}

static int mt7933_codec_ana_adc_left_pga_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    int adc_id = 0;
    aud_dbg("event: %d, adc_code = %x", event, mt7933_codec->adc_code);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
                if (!(mt7933_codec->adc_code & BIT(adc_id))) {
                    continue;
                }
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_PGAOUTPUT_EN_MASK,
                                              RG_ADC_AUDULL_PGAOUTPUT_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON03 + adc_id * 0x100,
                                              RG_ADC_AUDULL_PGA_CHP_EN_MASK,
                                              RG_ADC_AUDULL_PGA_CHP_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON03 + adc_id * 0x100,
                                              RG_ADC_AUDULL_PGA_NEGR_EN_MASK,
                                              0);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON03 + adc_id * 0x100,
                                              RG_ADC_AUDULL_PGA_SDR_EN_MASK,
                                              RG_ADC_AUDULL_PGA_SDR_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_PGA_PWDB_MASK,
                                              RG_ADC_AUDULL_PGA_PWDB_MASK);
                /* EFUSE */
                uint32_t efuse;
                uint32_t valid;
                switch (adc_id) {
                    case MT7933_ADC0:
                        aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC0_RC_TRIM_L, &valid);
                        if (valid & BIT(7)) {
                            efuse = (valid & GENMASK(5, 0)) >> 0;
                        } else {
                            efuse = 0x20;
                        }
                        break;

                    case MT7933_ADC1:
                        aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC1_RC_TRIM_L, &valid);
                        if (valid & BIT(7)) {
                            efuse = (valid & GENMASK(5, 0)) >> 0;
                        } else {
                            efuse = 0x20;
                        }
                        break;

                    default:
                        aud_error("adc_ic error: %d", adc_id);
                }

                aud_msg("RC_TRIM_VAL = %x", efuse);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_RC_TRIM_MASK,
                                              RG_ADC_AUDULL_RC_TRIM_VAL(efuse));
            }
            break;
        default:
            return 0;
    }

    return 0;
}

static int mt7933_codec_ana_adc_right_pga_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    int adc_id = 0;
    aud_msg("event: %d, adc_code = %x", event, mt7933_codec->adc_code);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
                if (!(mt7933_codec->adc_code & BIT(adc_id))) {
                    continue;
                }
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_PGAOUTPUT_EN_MASK,
                                              RG_ADC_AUDULR_PGAOUTPUT_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON03 + adc_id * 0x100,
                                              RG_ADC_AUDULR_PGA_CHP_EN_MASK,
                                              RG_ADC_AUDULR_PGA_CHP_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON03 + adc_id * 0x100,
                                              RG_ADC_AUDULR_PGA_NEGR_EN_MASK,
                                              0);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON03 + adc_id * 0x100,
                                              RG_ADC_AUDULR_PGA_SDR_EN_MASK,
                                              RG_ADC_AUDULR_PGA_SDR_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_PGA_PWDB_MASK,
                                              RG_ADC_AUDULR_PGA_PWDB_MASK);
                /* EFUSE */
                uint32_t efuse;
                uint32_t valid;
                switch (adc_id) {
                    case MT7933_ADC0:
                        aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC0_RC_TRIM_R, &valid);
                        if (valid & BIT(7)) {
                            efuse = (valid & GENMASK(5, 0)) >> 0;
                        } else {
                            efuse = 0x20;
                        }
                        break;

                    case MT7933_ADC1:
                        efuse = 0x20;
                        break;

                    default:
                        aud_error("adc_ic error: %d", adc_id);
                }

                aud_msg("RC_TRIM_VAL = %x", efuse);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_RC_TRIM_MASK,
                                              RG_ADC_AUDULR_RC_TRIM_VAL(efuse));
            }
            break;
        default:
            return 0;
    }

    return 0;
}

static int mt7933_codec_ana_adc_ldo_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    int adc_id = 0;
    aud_dbg("event: %d, adc_code = %x", event, mt7933_codec->adc_code);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
                if (!(mt7933_codec->adc_code & BIT(adc_id))) {
                    continue;
                }
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_LDO08_PWDB_AUDULL_MASK,
                                              RG_ADC_LDO08_PWDB_AUDULL_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_LDO08_PWDB_AUDULR_MASK,
                                              RG_ADC_LDO08_PWDB_AUDULR_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_LDO_L_PWDB_MASK,
                                              RG_ADC_LDO_L_PWDB_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_LDO_R_PWDB_MASK,
                                              RG_ADC_LDO_R_PWDB_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_LDO08_VOSEL_AUDULL_MASK,
                                              RG_ADC_LDO08_VOSEL_AUDULL_VAL(0x1));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_LDO08_VOSEL_AUDULR_MASK,
                                              RG_ADC_LDO08_VOSEL_AUDULR_VAL(0x1));
                /* EFUSE */
                uint32_t efuse;
                uint32_t valid;
                switch (adc_id) {
                    case MT7933_ADC0:
                        aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC1_VREF_VCM1P4_SEL, &valid);
                        if (valid & BIT(3)) {
                            aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC_LDO_VOSEL, &efuse);
                            efuse = (efuse & GENMASK(7, 4)) >> 4;
                        } else {
                            efuse = 0x6;
                        }
                        break;

                    case MT7933_ADC1:
                        aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC1_VREF_VCM1P4_SEL, &valid);
                        if (valid & BIT(3)) {
                            aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC_LDO_VOSEL, &efuse);
                            efuse = (efuse & GENMASK(3, 0)) >> 0;
                        } else {
                            efuse = 0x6;
                        }
                        break;
                    default:
                        aud_error("adc_id error %d", adc_id);
                }

                aud_msg("LDO_VOSEL = %x", efuse);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_COMMON + adc_id * 0x100,
                                              RG_ADC_LDO_VOSEL_MASK,    /*1.7v*/
                                              RG_ADC_LDO_VOSEL_VAL(efuse));
            }
            break;
        default:
            return 0;
    }

    return 0;
}

static int mt7933_codec_ana_adc_vref_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    int adc_id = 0;
    aud_dbg("event: %d, adc_code = %x", event, mt7933_codec->adc_code);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
                if (!(mt7933_codec->adc_code & BIT(adc_id))) {
                    continue;
                }
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULL_VREF_VCM08_SEL_MASK,
                                              RG_ADC_AUDULL_VREF_VCM08_SEL_VAL(0x2));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULR_VREF_VCM08_SEL_MASK,
                                              RG_ADC_AUDULR_VREF_VCM08_SEL_VAL(0x2));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULL_VREF_VCM09_SEL_MASK,
                                              RG_ADC_AUDULL_VREF_VCM09_SEL_VAL(0x0));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULR_VREF_VCM09_SEL_MASK,
                                              RG_ADC_AUDULR_VREF_VCM09_SEL_VAL(0x0));
                /* EFUSE */
                uint32_t efuse;
                uint32_t valid;
                switch (adc_id) {
                    case MT7933_ADC0:
                        aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC1_VREF_VCM1P4_SEL, &valid);
                        if (valid & BIT(2)) {
                            aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC0_VREF_VCM1P4_SEL, &efuse);
                            efuse = (efuse & GENMASK(7, 4)) >> 4;
                        } else {
                            efuse = 0x8;
                        }
                        break;

                    case MT7933_ADC1:
                        aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC1_VREF_VCM1P4_SEL, &valid);
                        if (valid & BIT(0)) {
                            aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC1_VREF_VCM1P4_SEL, &efuse);
                            efuse = (efuse & GENMASK(7, 4)) >> 4;
                        } else {
                            efuse = 0x8;
                        }
                        break;

                    default:
                        aud_error("adc_id error: %d", adc_id);
                }

                aud_msg("AUDULL_VREF_VCM = %x", efuse);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULL_VREF_VCM1P4_SEL_MASK,
                                              RG_ADC_AUDULL_VREF_VCM1P4_SEL_VAL(efuse));

                switch (adc_id) {
                    case MT7933_ADC0:
                        aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC1_VREF_VCM1P4_SEL, &valid);
                        if (valid & BIT(1)) {
                            aud_drv_read_reg(AUDIO_REGMAP_EEF_GRP2, AUDADC0_VREF_VCM1P4_SEL, &efuse);
                            efuse = (efuse & GENMASK(3, 0)) >> 0;
                        } else {
                            efuse = 0x8;
                        }
                        break;

                    case MT7933_ADC1:
                        efuse = 0x8;
                        break;

                    default:
                        aud_error("adc_id error: %d", adc_id);
                }

                aud_msg("AUDULR_VREF_VCM = %x", efuse);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULR_VREF_VCM1P4_SEL_MASK,
                                              RG_ADC_AUDULR_VREF_VCM1P4_SEL_VAL(efuse));

                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_VREF_VCM_EN_MASK,
                                              RG_ADC_AUDULL_VREF_VCM_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_VREF_VCM_EN_MASK,
                                              RG_ADC_AUDULR_VREF_VCM_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_VREF_EN_MASK,
                                              RG_ADC_AUDULL_VREF_EN_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_VREF_EN_MASK,
                                              RG_ADC_AUDULR_VREF_EN_MASK);

                /* disable VCM chopper */
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULL_VREF_VCM_CHP_EN_MASK,
                                              0x0);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULR_VREF_VCM_CHP_EN_MASK,
                                              0x0);

                /* disable VREF chopper */
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULL_VREF_CHP_EN_MASK,
                                              0x0);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULR_VREF_CHP_EN_MASK,
                                              0x0);

                /* VREF current adjust */
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULL_VREF_CURRENT_ADJUST_MASK,
                                              RG_ADC_AUDULL_VREF_CURRENT_ADJUST_VAL(0x0));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULR_VREF_CURRENT_ADJUST_MASK,
                                              RG_ADC_AUDULR_VREF_CURRENT_ADJUST_VAL(0x0));

                /* VREF outputstage current adjust */
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULL_VREF_OUTPUT_CURRENT_SEL_MASK,
                                              RG_ADC_AUDULL_VREF_OUTPUT_CURRENT_SEL_VAL(0x0));
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON04 + adc_id * 0x100,
                                              RG_ADC_AUDULR_VREF_OUTPUT_CURRENT_SEL_MASK,
                                              RG_ADC_AUDULR_VREF_OUTPUT_CURRENT_SEL_VAL(0x0));

                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_LCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULL_DITHER_EN_MASK | RG_ADC_AUDULL_DITHER_RESETB_MASK,
                                              RG_ADC_AUDULL_DITHER_EN_MASK | RG_ADC_AUDULL_DITHER_RESETB_MASK);
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_RCH_CON00 + adc_id * 0x100,
                                              RG_ADC_AUDULR_DITHER_EN_MASK | RG_ADC_AUDULR_DITHER_RESETB_MASK,
                                              RG_ADC_AUDULR_DITHER_EN_MASK | RG_ADC_AUDULR_DITHER_RESETB_MASK);
            }
            break;
        default:
            return 0;
    }

    return 0;
}

static int mt7933_codec_dig_adda_clk_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    aud_dbg("event: %d", event);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            mt7933_codec_enable_adda_afe_on(priv);
            break;
        default:
            return 0;
    }

    return 0;
}

static int mt7933_codec_micbias0_event(struct msd_ctl_value *ctl_value, void *priv)
{
    int event = ctl_value->integer.value[3];
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    int adc_id = 0;
    aud_dbg("event: %d, adc_code = %x", event, mt7933_codec->adc_code);

    switch (event) {
        case SND_SOC_PRE_PREPARE:
            for (adc_id = MT7933_ADC0; adc_id < MT7933_ADC_NUM; adc_id ++) {
                if (!(mt7933_codec->adc_code & BIT(adc_id))) {
                    continue;
                }
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_APMIXEDSYS,
                                              AUD_ADC0_COMMON + adc_id * 0x100,
                                              RG_ADC_AUDPWDBMICBIAS_MASK,
                                              RG_ADC_AUDPWDBMICBIAS_MASK);
            }
            break;
        default:
            return 0;
    }

    return 0;
}

struct mt7933_reg_info {
    uint32_t reg;
    uint32_t mask;
};

enum {
    LEFT_PGA = 0,
    RIGHT_PGA,
    INT1,
    SARADC,
    INT2,
    ANA_DAC_CLK,
    DIG_ADDA_CLK,
    ANA_ADC_LDO,
    ANA_ADC_CLK,
    ANA_ADC_VREF,
    AU_MICBIAS0,
    ID_MAX,
};

static struct soc_ctl_entry mt7933_codec_snd_dapm_controls[] = {
    {
        .name = "Left PGA",
        .set = mt7933_codec_ana_adc_left_pga_event,
    },
    {
        .name = "Right PGA",
        .set = mt7933_codec_ana_adc_right_pga_event,
    },
    {
        .name = "INT1",
        .set = mt7933_codec_ana_adc_int1_event,
    },
    {
        .name = "SARADC",
        .set = mt7933_codec_ana_adc_saradc_event,
    },
    {
        .name = "INT2",
        .set = mt7933_codec_ana_adc_int2_event,
    },
    {
        .name = "ANA_DAC_CLK",
        .set = mt7933_codec_ana_dac_clk_event,
    },
    {
        .name = "DIG_ADDA_CLK",
        .set = mt7933_codec_dig_adda_clk_event,
    },
    {
        .name = "ANA_ADC_LDO",
        .set = mt7933_codec_ana_adc_ldo_event,
    },
    {
        .name = "ANA_ADC_CLK",
        .set = mt7933_codec_ana_adc_clk_event,
    },
    {
        .name = "ANA_ADC_VREF",
        .set = mt7933_codec_ana_adc_vref_event,
    },
    {
        .name = "AU_MICBIAS0",
        .set = mt7933_codec_micbias0_event,
    },
};

static struct snd_widget mt7933_codec_dapm_widgets[] = {
    SWITCH_WIDGET("AIF RX", -1, NULL),
    SWITCH_WIDGET("Left DAC", -1, NULL),
    SWITCH_WIDGET("Right DAC", -1, NULL),
    SWITCH_WIDGET("AIF TX", -1, NULL),

    SWITCH_WIDGET("Left ADC", -1, NULL),
    SWITCH_WIDGET("Right ADC", -1, NULL),
    SWITCH_WIDGET("Left PGA", -1, &mt7933_codec_snd_dapm_controls[LEFT_PGA]),
    SWITCH_WIDGET("Right PGA", -1, &mt7933_codec_snd_dapm_controls[RIGHT_PGA]),
    SWITCH_WIDGET("INT1", -1, &mt7933_codec_snd_dapm_controls[INT1]),
    SWITCH_WIDGET("SARADC", -1, &mt7933_codec_snd_dapm_controls[SARADC]),
    SWITCH_WIDGET("INT2", -1, &mt7933_codec_snd_dapm_controls[INT2]),
    SWITCH_WIDGET("DIG_ADC_CLK", -1, NULL),
    SWITCH_WIDGET("DIG_DAC_CLK", -1, NULL),
    SWITCH_WIDGET("ANA_DAC_CLK", -1, &mt7933_codec_snd_dapm_controls[ANA_DAC_CLK]),

    SWITCH_WIDGET("DIG_ADDA_CLK", -1, &mt7933_codec_snd_dapm_controls[DIG_ADDA_CLK]),
    SWITCH_WIDGET("ANA_ADC_LDO", -1, &mt7933_codec_snd_dapm_controls[ANA_ADC_LDO]),
    SWITCH_WIDGET("ANA_ADC_CLK", -1, &mt7933_codec_snd_dapm_controls[ANA_ADC_CLK]),
    SWITCH_WIDGET("ANA_ADC_VREF", -1, &mt7933_codec_snd_dapm_controls[ANA_ADC_VREF]),
    SWITCH_WIDGET("AU_MICBIAS0", -1, &mt7933_codec_snd_dapm_controls[AU_MICBIAS0]),

    DAI_WIDGET("mt7933-codec-dai-playback"),
    DAI_WIDGET("mt7933-codec-dai-capture"),

    END_WIDGET("AU_VIN0"),
    END_WIDGET("AU_VIN1"),
    END_WIDGET("AU_VOL"),
    END_WIDGET("AU_VOR"),
    SWITCH_WIDGET("SWITCH_DUMMY", -1, NULL),
    END_WIDGET("AU_END_DUMMY"),
};

static struct snd_widget_route mt7933_codec_intercon[] = {
    {"mt7933-codec-dai-playback", "AIF RX", 1, {NULL, NULL}},
    { "mt7933-codec-dai-capture", "AIF TX", 1, {NULL, NULL}},

    { "AIF RX", "ANA_DAC_CLK", 1, {NULL, NULL}},
    { "AIF RX", "DIG_ADDA_CLK", 1, {NULL, NULL}},
    { "AIF RX", "DIG_DAC_CLK", 1, {NULL, NULL}},

    { "ANA_DAC_CLK", "SWITCH_DUMMY", 1, {NULL, NULL}},
    { "DIG_ADDA_CLK", "SWITCH_DUMMY", 1, {NULL, NULL}},
    { "DIG_DAC_CLK", "SWITCH_DUMMY", 1, {NULL, NULL}},

    { "SWITCH_DUMMY", "AU_END_DUMMY", 1, {NULL, NULL}},

    { "AIF TX", "ANA_ADC_CLK", 1, {NULL, NULL}},
    { "AIF TX", "DIG_ADDA_CLK", 1, {NULL, NULL}},
    { "AIF TX", "ANA_ADC_LDO", 1, {NULL, NULL}},
    { "AIF TX", "ANA_ADC_VREF", 1, {NULL, NULL}},
    { "AIF TX", "DIG_ADC_CLK", 1, {NULL, NULL}},
    { "AIF TX", "Left ADC", 1, {NULL, NULL}},
    { "AIF TX", "Right ADC", 1, {NULL, NULL}},


    { "ANA_ADC_CLK", "SWITCH_DUMMY", 1, {NULL, NULL}},
    { "ANA_ADC_LDO", "SWITCH_DUMMY", 1, {NULL, NULL}},
    { "ANA_ADC_VREF", "SWITCH_DUMMY", 1, {NULL, NULL}},
    { "DIG_ADC_CLK", "SWITCH_DUMMY", 1, {NULL, NULL}},

    { "Left ADC", "Left PGA", 1, {NULL, NULL}},
    { "Right ADC", "Right PGA", 1, {NULL, NULL}},

    {"AIF RX", "Left DAC", 1, {NULL, NULL}},
    {"AIF RX", "Right DAC", 1, {NULL, NULL}},
    {"Left DAC", "AU_VOL", 1, {NULL, NULL}},
    {"Right DAC", "AU_VOR", 1, {NULL, NULL}},

    { "Left PGA", "INT1", 1, {NULL, NULL}},
    { "Right PGA", "INT1", 1, {NULL, NULL}},

    { "INT1", "SARADC", 1, {NULL, NULL}},
    { "SARADC", "INT2", 1, {NULL, NULL}},

    { "INT2", "AU_MICBIAS0", 1, {NULL, NULL}},
    { "AU_MICBIAS0", "SWITCH_DUMMY", 1, {NULL, NULL}},
};

struct mt7933_codec_rate {
    unsigned int rate;
    unsigned int regvalue;
};

static const struct mt7933_codec_rate mt7933_codec_ul_voice_modes[] = {
    {.rate = 8000, .regvalue = 0, },
    {.rate = 16000, .regvalue = 1, },
    {.rate = 32000, .regvalue = 2, },
    {.rate = 48000, .regvalue = 3, },
};

//static const struct mt7933_codec_rate mt7933_codec_ul_rates[] = {
//  {.rate = 8000, .regvalue = 0, },
//  {.rate = 16000, .regvalue = 0, },
//  {.rate = 32000, .regvalue = 0, },
//  {.rate = 48000, .regvalue = 1, },
//};

//static const struct mt7933_codec_rate mt7933_codec_dl_voice_modes[] = {
//  {.rate = 8000, .regvalue = 1, },
//  {.rate = 11025, .regvalue = 0, },
//  {.rate = 12000, .regvalue = 0, },
//  {.rate = 16000, .regvalue = 1, },
//  {.rate = 22050, .regvalue = 0, },
//  {.rate = 24000, .regvalue = 0, },
//  {.rate = 32000, .regvalue = 0, },
//  {.rate = 44100, .regvalue = 0, },
//  {.rate = 48000, .regvalue = 0, },
//};

static const struct mt7933_codec_rate mt7933_codec_dl_input_modes[] = {
    {.rate = 8000, .regvalue = 0, },
    {.rate = 11025, .regvalue = 1, },
    {.rate = 12000, .regvalue = 2, },
    {.rate = 16000, .regvalue = 3, },
    {.rate = 22050, .regvalue = 4, },
    {.rate = 24000, .regvalue = 5, },
    {.rate = 32000, .regvalue = 6, },
    {.rate = 44100, .regvalue = 7, },
    {.rate = 48000, .regvalue = 8, },
};

static const struct mt7933_codec_rate mt7933_codec_dl_rates[] = {
    {.rate = 8000, .regvalue = 0, },
    {.rate = 11025, .regvalue = 1, },
    {.rate = 12000, .regvalue = 2, },
    {.rate = 16000, .regvalue = 3, },
    {.rate = 22050, .regvalue = 4, },
    {.rate = 24000, .regvalue = 5, },
    {.rate = 32000, .regvalue = 6, },
    {.rate = 44100, .regvalue = 7, },
    {.rate = 48000, .regvalue = 8, },
};

static int mt7933_codec_rate_to_val(const struct mt7933_codec_rate *table,
                                    int table_nums, unsigned int rate, unsigned int *val)
{
    int i;

    for (i = 0; i < table_nums; i++) {
        if (table[i].rate == rate) {
            *val = table[i].regvalue;
            return 0;
        }
    }

    return -EINVAL;
}

struct mt7933_codec_rate_ctrl {
    unsigned int top_ctrl_reg;
    unsigned int top_ctrl_w_val;
    unsigned int top_ctrl_r_val;
};

static const struct mt7933_codec_rate_ctrl top_ctrl_regs[] = {
    [MSD_STREAM_PLAYBACK] = {
        .top_ctrl_reg = ABB_AFE_CON11,
        .top_ctrl_w_val = ABB_AFE_CON11_TOP_CTRL,
        .top_ctrl_r_val = ABB_AFE_CON11_TOP_CTRL_STATUS,
    },
    [MSD_STREAM_CAPTURE] = {
        .top_ctrl_reg = -1,
        .top_ctrl_w_val = -1,
        .top_ctrl_r_val = -1,
    },
};

static int mt7933_codec_valid_new_rate(void *priv, int stream)
{
    if (stream == MSD_STREAM_PLAYBACK) {
        unsigned int reg = top_ctrl_regs[stream].top_ctrl_reg;
        unsigned int w_val = top_ctrl_regs[stream].top_ctrl_w_val;
        unsigned int r_val = top_ctrl_regs[stream].top_ctrl_r_val;
        unsigned int reg_val;

        /* toggle top_ctrl status */
        reg_val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, reg);

        if (reg_val & r_val) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, reg,
                                          w_val, 0x0);
        } else {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, reg,
                                          w_val, w_val);
        }
    }

    return 0;
}

static int mt7933_codec_setup_ul_rate(void *priv, unsigned int rate)
{
    unsigned int val = 0;
    int ret;

    ret = mt7933_codec_rate_to_val(mt7933_codec_ul_voice_modes,
                                   ARRAY_SIZE(mt7933_codec_ul_voice_modes), rate, &val);
    if (ret < 0)
        goto err;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  ABB_ULAFE_CON0,
                                  ABB_ULAFE_CON0_UL_VOICE_MODE_MASK,
                                  ABB_ULAFE_CON0_UL_VOICE_MODE(val));

    // for ADC1
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  ABB_UL2AFE_CON0,
                                  ABB_ULAFE_CON0_UL_VOICE_MODE_MASK,
                                  ABB_ULAFE_CON0_UL_VOICE_MODE(val));

    return 0;

err:
    aud_error("error to setup ul rate\n");
    return -EINVAL;
}

static int mt7933_codec_setup_dl_rate(void *priv, int rate)
{
    unsigned int val = 0;
    int ret;

    ret = mt7933_codec_rate_to_val(mt7933_codec_dl_input_modes,
                                   ARRAY_SIZE(mt7933_codec_dl_input_modes), rate, &val);
    if (ret < 0)
        goto err;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  AFE_ADDA_DL_SRC2_CON0,
                                  AFE_ADDA_DL_SRC2_CON0_DL_INPUT_MODE_MASK,
                                  AFE_ADDA_DL_SRC2_CON0_DL_INPUT_MODE_VAL(val));

    ret = mt7933_codec_rate_to_val(mt7933_codec_dl_rates,
                                   ARRAY_SIZE(mt7933_codec_dl_rates), rate, &val);
    if (ret < 0)
        goto err;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  ABB_AFE_CON1,
                                  ABB_AFE_CON1_DL_RATE_MASK,
                                  ABB_AFE_CON1_DL_RATE(val));

    mt7933_codec_valid_new_rate(priv, MSD_STREAM_PLAYBACK);

    return 0;

err:
    aud_error("error to setup dl rate\n");
    return -EINVAL;
}

static int mt7933_codec_configure_ul(void *priv, struct snd_soc_dai *dai)
{
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;

    /* Set up a few ADC to start up in this place */
    mt7933_codec->adc_code = 0x3; // enable ADC0 & ADC1

    return 0;
}

static int mt7933_codec_configure_dl_nle(void *priv, int rate)
{
    static struct {
        unsigned int reg;
        unsigned int mask;
        unsigned int val;
    } config_regs[] = {
        {
            AFE_NLE_ZCD_LCH_CFG,
            AFE_NLE_ZCD_CH_CFG_ZCD_CHECK_MODE_MASK,
            AFE_NLE_ZCD_CH_CFG_ZCD_CHECK_MODE_VAL(0x0),
        },
        {
            AFE_NLE_ZCD_LCH_CFG,
            AFE_NLE_ZCD_CH_CFG_ZCD_MODE_SEL_MASK,
            AFE_NLE_ZCD_CH_CFG_ZCD_MODE_SEL_VAL(0x0),
        },
        {
            AFE_NLE_PWR_DET_LCH_CFG,
            AFE_NLE_PWR_DET_CH_CFG_H2L_HOLD_TIME_MASK,
            AFE_NLE_PWR_DET_CH_CFG_H2L_HOLD_TIME_DEF_VAL,
        },
        {
            AFE_NLE_PWR_DET_LCH_CFG,
            AFE_NLE_PWR_DET_CH_CFG_NLE_VTH_MASK,
            /* -40dB: 10^(-40/20) * 2^20 */
            AFE_NLE_PWR_DET_CH_CFG_NLE_VTH_VAL(0x28F6),
        },
        {
            AFE_NLE_GAIN_ADJ_LCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_ADJ_BYPASS_ZCD_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_ADJ_BYPASS_ZCD_VAL(0x0),
        },
        {
            AFE_NLE_GAIN_ADJ_LCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_TIME_OUT_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_TIME_OUT_VAL(0x1),
        },
        {
            AFE_NLE_GAIN_ADJ_LCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_HOLD_TIME_PER_JUMP_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_HOLD_TIME_PER_JUMP_VAL(0x0),
        },
        {
            AFE_NLE_GAIN_ADJ_LCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_JUMP_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_JUMP_VAL(0x0),
        },
        {
            AFE_NLE_GAIN_ADJ_LCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_ZCD_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_ZCD_VAL(0x3),
        },
        {
            AFE_NLE_GAIN_ADJ_LCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MIN_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MIN_VAL(0xA),
        },
        {
            AFE_NLE_GAIN_ADJ_LCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MAX_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MAX_VAL(0x2),
        },
        {
            AFE_NLE_GAIN_IMP_LCH_CFG0,
            AFE_NLE_GAIN_IMP_CH_CFG0_AG_DELAY_MASK,
            AFE_NLE_GAIN_IMP_CH_CFG0_AG_DELAY_VAL(0xC),
        },
        {
            AFE_NLE_ZCD_RCH_CFG,
            AFE_NLE_ZCD_CH_CFG_ZCD_CHECK_MODE_MASK,
            AFE_NLE_ZCD_CH_CFG_ZCD_CHECK_MODE_VAL(0x0),
        },
        {
            AFE_NLE_ZCD_RCH_CFG,
            AFE_NLE_ZCD_CH_CFG_ZCD_MODE_SEL_MASK,
            AFE_NLE_ZCD_CH_CFG_ZCD_MODE_SEL_VAL(0x0),
        },
        {
            AFE_NLE_PWR_DET_RCH_CFG,
            AFE_NLE_PWR_DET_CH_CFG_H2L_HOLD_TIME_MASK,
            AFE_NLE_PWR_DET_CH_CFG_H2L_HOLD_TIME_DEF_VAL,
        },
        {
            AFE_NLE_PWR_DET_RCH_CFG,
            AFE_NLE_PWR_DET_CH_CFG_NLE_VTH_MASK,
            /* -40dB: 10^(-40/20) * 2^20 */
            AFE_NLE_PWR_DET_CH_CFG_NLE_VTH_VAL(0x28F6),
        },
        {
            AFE_NLE_GAIN_ADJ_RCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_ADJ_BYPASS_ZCD_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_ADJ_BYPASS_ZCD_VAL(0x0),
        },
        {
            AFE_NLE_GAIN_ADJ_RCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_TIME_OUT_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_TIME_OUT_VAL(0x1),
        },
        {
            AFE_NLE_GAIN_ADJ_RCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_HOLD_TIME_PER_JUMP_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_HOLD_TIME_PER_JUMP_VAL(0x0),
        },
        {
            AFE_NLE_GAIN_ADJ_RCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_JUMP_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_JUMP_VAL(0x0),
        },
        {
            AFE_NLE_GAIN_ADJ_RCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_ZCD_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_ZCD_VAL(0x3),
        },
        {
            AFE_NLE_GAIN_ADJ_RCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MIN_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MIN_VAL(0xA),
        },
        {
            AFE_NLE_GAIN_ADJ_RCH_CFG0,
            AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MAX_MASK,
            AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MAX_VAL(0x2),
        },
        {
            AFE_NLE_GAIN_IMP_RCH_CFG0,
            AFE_NLE_GAIN_IMP_CH_CFG0_AG_DELAY_MASK,
            AFE_NLE_GAIN_IMP_CH_CFG0_AG_DELAY_VAL(0xC),
        },
    };
    unsigned int i;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_NLE_PRE_BUF_CFG,
                                  AFE_NLE_PRE_BUF_CFG_POINT_END_MASK,
                                  (rate * (H2L_HOLD_TIME_MS - 1) / 1000) - 1);

    for (i = 0; i < ARRAY_SIZE(config_regs); i++)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, config_regs[i].reg,
                                      config_regs[i].mask, config_regs[i].val);
    return 0;
}

static int mt7933_codec_configure_dl(void *priv, int rate)
{
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_DL_SRC2_CON1,
                                  AFE_ADDA_DL_SRC2_CON1_GAIN_CTRL_MASK,
                                  AFE_ADDA_DL_SRC2_CON1_GAIN_CTRL_VAL(0xf74f));

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_DL_SRC2_CON0,
                                  AFE_ADDA_DL_SRC2_CON0_MUTE_OFF_CTRL_MASK,
                                  AFE_ADDA_DL_SRC2_CON0_MUTE_OFF);

    mt7933_codec_valid_new_rate(priv, MSD_STREAM_PLAYBACK);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_AFE_CON5,
                                  ABB_AFE_CON5_SDM_GAIN_VAL_MASK,
                                  ABB_AFE_CON5_SDM_GAIN_VAL(0x34));

    if (mt7933_codec->dl_nle_support)
        mt7933_codec_configure_dl_nle(priv, rate);

    return 0;
}

static int mt7933_codec_enable_ul(void *priv)
{

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_AD_UL2_SRC_CON1,
                                  BIT(11),
                                  BIT(11));

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_ULAFE_CON0,
                                  ABB_ULAFE_CON0_UL_SRC_ON_MASK,
                                  ABB_ULAFE_CON0_UL_SRC_ON);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_ULAFE_CON0,
                                  ABB_ULAFE_CON0_UL_FIFO_ON_MASK,
                                  ABB_ULAFE_CON0_UL_FIFO_ON);

    // for ADC1
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_UL2AFE_CON0,
                                  ABB_ULAFE_CON0_UL_SRC_ON_MASK,
                                  ABB_ULAFE_CON0_UL_SRC_ON);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_UL2AFE_CON0,
                                  ABB_ULAFE_CON0_UL_FIFO_ON_MASK,
                                  ABB_ULAFE_CON0_UL_FIFO_ON);

    return 0;
}

static int mt7933_codec_enable_dl_nle(void *priv)
{
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_NLE_CFG,
                                  AFE_NLE_CFG_SW_RSTB_MASK,
                                  AFE_NLE_CFG_SW_RSTB_VAL(0x1));

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_NLE_CFG,
                                  AFE_NLE_CFG_AFE_NLE_ON_MASK,
                                  AFE_NLE_CFG_AFE_NLE_ON);

    return 0;
}

static int mt7933_codec_enable_dl(void *priv)
{
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;

    if (mt7933_codec->dl_nle_support)
        mt7933_codec_enable_dl_nle(priv);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_DL_SCR_SPL_CON0,
                                  BIT(10) | BIT(12) | BIT(20), BIT(10) | BIT(12) | BIT(20));

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_UL_DL_CON0,
                                  AFE_ADDA_UL_DL_CON0_ADDA_INTF_ON_MASK,
                                  AFE_ADDA_UL_DL_CON0_ADDA_INTF_ON);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_DL_SRC2_CON0,
                                  AFE_ADDA_DL_SRC2_CON0_GAIN_ON_MASK,
                                  AFE_ADDA_DL_SRC2_CON0_GAIN_ON);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_DL_SRC2_CON0,
                                  AFE_ADDA_DL_SRC2_CON0_DL_SRC_ON_MASK,
                                  AFE_ADDA_DL_SRC2_CON0_DL_SRC_ON);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_AFE_CON0,
                                  ABB_AFE_CON0_DL_EN_MASK,
                                  ABB_AFE_CON0_DL_EN);

    return 0;
}

static int mt7933_codec_disable_ul(void *priv)
{
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_ULAFE_CON0,
                                  ABB_ULAFE_CON0_UL_FIFO_ON_MASK,
                                  0x0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_ULAFE_CON0,
                                  ABB_ULAFE_CON0_UL_SRC_ON_MASK,
                                  0x0);

    // for ADC1
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_UL2AFE_CON0,
                                  ABB_ULAFE_CON0_UL_FIFO_ON_MASK,
                                  0x0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_UL2AFE_CON0,
                                  ABB_ULAFE_CON0_UL_SRC_ON_MASK,
                                  0x0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_AD_UL2_SRC_CON1,
                                  BIT(11),
                                  0x0);

    return 0;
}

static int mt7933_codec_disable_dl_nle(void *priv)
{
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_NLE_CFG,
                                  AFE_NLE_CFG_AFE_NLE_ON_MASK,
                                  0x0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_NLE_CFG,
                                  AFE_NLE_CFG_SW_RSTB_MASK,
                                  AFE_NLE_CFG_SW_RSTB_VAL(0x0));

    return 0;
}

static int mt7933_codec_disable_dl(void *priv)
{
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_DL_SCR_SPL_CON0,
                                  BIT(10) | BIT(12), 0x0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ABB_AFE_CON0,
                                  ABB_AFE_CON0_DL_EN_MASK,
                                  0x0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_DL_SRC2_CON0,
                                  AFE_ADDA_DL_SRC2_CON0_GAIN_ON_MASK,
                                  0x0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_DL_SRC2_CON0,
                                  AFE_ADDA_DL_SRC2_CON0_DL_SRC_ON_MASK,
                                  0x0);

    if (mt7933_codec->dl_nle_support)
        mt7933_codec_disable_dl_nle(priv);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_UL_DL_CON0,
                                  AFE_ADDA_UL_DL_CON0_ADDA_INTF_ON_MASK,
                                  0x0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_UL_DL_CON0,
                                  AFE_ADDA_UL_DL_CON0_DL_SW_RESET_MASK,
                                  0x0);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_ADDA_UL_DL_CON0,
                                  AFE_ADDA_UL_DL_CON0_DL_SW_RESET_MASK,
                                  0x1 << 15);

    return 0;
}

#define MT7933_CODEC_DL_RATES (SNDRV_PCM_RATE_8000_48000 | \
                               SNDRV_PCM_RATE_12000 | \
                               SNDRV_PCM_RATE_24000)
#define MT7933_CODEC_UL_RATES (SNDRV_PCM_RATE_8000 | \
                               SNDRV_PCM_RATE_16000 | \
                               SNDRV_PCM_RATE_32000 | \
                               SNDRV_PCM_RATE_48000)

#define MT7933_CODEC_FORMATS (MSD_PCM_FMT_S16_LE | \
                              MSD_PCM_FMT_S32_LE)

struct mt7933_codec_reg_attr {
    uint32_t offset;
    char *name;
};

#define DUMP_REG_ENTRY(reg) {reg, #reg}

static const struct mt7933_codec_reg_attr mt7933_codec_dump_digital_reg_list[] = {
    /* audio_top_sys */
    DUMP_REG_ENTRY(AFE_ADDA_DL_SRC2_CON0),
    DUMP_REG_ENTRY(AFE_ADDA_DL_SRC2_CON1),
    DUMP_REG_ENTRY(AFE_ADDA_TOP_CON0),
    DUMP_REG_ENTRY(AFE_ADDA_UL_DL_CON0),
    DUMP_REG_ENTRY(AFE_NLE_CFG),
    DUMP_REG_ENTRY(AFE_NLE_PRE_BUF_CFG),
    DUMP_REG_ENTRY(AFE_NLE_PWR_DET_LCH_CFG),
    DUMP_REG_ENTRY(AFE_NLE_ZCD_LCH_CFG),
    DUMP_REG_ENTRY(AFE_NLE_GAIN_ADJ_LCH_CFG0),
    DUMP_REG_ENTRY(AFE_NLE_GAIN_IMP_LCH_CFG0),
    DUMP_REG_ENTRY(AFE_NLE_PWE_DET_LCH_MON),
    DUMP_REG_ENTRY(AFE_NLE_GAIN_ADJ_LCH_MON0),
    DUMP_REG_ENTRY(AFE_NLE_LCH_MON0),
    DUMP_REG_ENTRY(AFE_NLE_PWR_DET_RCH_CFG),
    DUMP_REG_ENTRY(AFE_NLE_ZCD_RCH_CFG),
    DUMP_REG_ENTRY(AFE_NLE_GAIN_ADJ_RCH_CFG0),
    DUMP_REG_ENTRY(AFE_NLE_GAIN_IMP_RCH_CFG0),
    DUMP_REG_ENTRY(AFE_NLE_PWE_DET_RCH_MON),
    DUMP_REG_ENTRY(AFE_NLE_GAIN_ADJ_RCH_MON0),
    DUMP_REG_ENTRY(AFE_NLE_RCH_MON0),
    DUMP_REG_ENTRY(ABB_AFE_CON0),
    DUMP_REG_ENTRY(ABB_AFE_CON1),
    DUMP_REG_ENTRY(ABB_AFE_CON2),
    DUMP_REG_ENTRY(ABB_AFE_CON3),
    DUMP_REG_ENTRY(ABB_AFE_CON4),
    DUMP_REG_ENTRY(ABB_AFE_CON5),
    DUMP_REG_ENTRY(ABB_AFE_CON6),
    DUMP_REG_ENTRY(ABB_AFE_CON7),
    DUMP_REG_ENTRY(ABB_AFE_CON10),
    DUMP_REG_ENTRY(ABB_AFE_CON11),
    DUMP_REG_ENTRY(ABB_AFE_SDM_TEST),
    DUMP_REG_ENTRY(ABB_ULAFE_CON0),
    DUMP_REG_ENTRY(ABB_ULAFE_CON1),
    DUMP_REG_ENTRY(ABB_UL2AFE_CON0),
    DUMP_REG_ENTRY(ABB_UL2AFE_CON1),
    DUMP_REG_ENTRY(AFE_AD_UL2_SRC_CON1),
};

static const struct mt7933_codec_reg_attr mt7933_codec_dump_analog_reg_list[] = {
    /* apmixedsys */
    DUMP_REG_ENTRY(AUD_ADC0_COMMON),
    DUMP_REG_ENTRY(AUD_ADC0_LCH_CON00),
    DUMP_REG_ENTRY(AUD_ADC0_LCH_CON01),
    DUMP_REG_ENTRY(AUD_ADC0_LCH_CON02),
    DUMP_REG_ENTRY(AUD_ADC0_LCH_CON03),
    DUMP_REG_ENTRY(AUD_ADC0_LCH_CON04),
    DUMP_REG_ENTRY(AUD_ADC0_LCH_CON05),
    DUMP_REG_ENTRY(AUD_ADC0_RCH_CON00),
    DUMP_REG_ENTRY(AUD_ADC0_RCH_CON01),
    DUMP_REG_ENTRY(AUD_ADC0_RCH_CON02),
    DUMP_REG_ENTRY(AUD_ADC0_RCH_CON03),
    DUMP_REG_ENTRY(AUD_ADC0_RCH_CON04),
    DUMP_REG_ENTRY(AUD_ADC0_RCH_CON05),
    DUMP_REG_ENTRY(AUD_ADC0_RSVD),
    DUMP_REG_ENTRY(AUD_ADC1_COMMON),
    DUMP_REG_ENTRY(AUD_ADC1_LCH_CON00),
    DUMP_REG_ENTRY(AUD_ADC1_LCH_CON01),
    DUMP_REG_ENTRY(AUD_ADC1_LCH_CON02),
    DUMP_REG_ENTRY(AUD_ADC1_LCH_CON03),
    DUMP_REG_ENTRY(AUD_ADC1_LCH_CON04),
    DUMP_REG_ENTRY(AUD_ADC1_LCH_CON05),
    DUMP_REG_ENTRY(AUD_ADC1_RCH_CON00),
    DUMP_REG_ENTRY(AUD_ADC1_RCH_CON01),
    DUMP_REG_ENTRY(AUD_ADC1_RCH_CON02),
    DUMP_REG_ENTRY(AUD_ADC1_RCH_CON03),
    DUMP_REG_ENTRY(AUD_ADC1_RCH_CON04),
    DUMP_REG_ENTRY(AUD_ADC1_RCH_CON05),
    DUMP_REG_ENTRY(AUD_ADC1_RSVD),
    DUMP_REG_ENTRY(AUD_DAC_CON00),
    DUMP_REG_ENTRY(AUD_DAC_CON01),
    DUMP_REG_ENTRY(AUD_DAC_CON02),
    DUMP_REG_ENTRY(AUD_DAC_CON03),
    DUMP_REG_ENTRY(AUD_DAC_CON04),
    DUMP_REG_ENTRY(AUD_DAC_CON05),
    DUMP_REG_ENTRY(AUD_ANAREG_CON),
    DUMP_REG_ENTRY(XTAL_CLK_CON),
};

static void mt7933_codec_debug_read(void)
{
    unsigned int i = 0;
    uint32_t val = 0;
    aud_dbg("dump_codec_msg_begin");
    for (i = 0; i < ARRAY_SIZE(mt7933_codec_dump_digital_reg_list); i++) {
        val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, mt7933_codec_dump_digital_reg_list[i].offset);
        aud_dbg("%s = 0x%x", mt7933_codec_dump_digital_reg_list[i].name, val);
    }
    for (i = 0; i < ARRAY_SIZE(mt7933_codec_dump_analog_reg_list); i++) {
        val = aud_drv_get_reg(AUDIO_REGMAP_APMIXEDSYS, mt7933_codec_dump_analog_reg_list[i].offset);
        aud_dbg("%s = 0x%x", mt7933_codec_dump_analog_reg_list[i].name, val);
    }
    aud_dbg("dump_codec_msg_end");
}

static void mt7933_codec_debug_read_msg(void)
{
    unsigned int i = 0;
    uint32_t val = 0;
    aud_msg("dump_codec_msg_begin");
    for (i = 0; i < ARRAY_SIZE(mt7933_codec_dump_digital_reg_list); i++) {
        val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, mt7933_codec_dump_digital_reg_list[i].offset);
        aud_msg("%s = 0x%x", mt7933_codec_dump_digital_reg_list[i].name, val);
    }
    for (i = 0; i < ARRAY_SIZE(mt7933_codec_dump_analog_reg_list); i++) {
        val = aud_drv_get_reg(AUDIO_REGMAP_APMIXEDSYS, mt7933_codec_dump_analog_reg_list[i].offset);
        aud_msg("%s = 0x%x", mt7933_codec_dump_analog_reg_list[i].name, val);
    }
    aud_msg("dump_codec_msg_end");
}

static int mt7933_codec_reg_dump(struct snd_pcm_stream *stream)
{
    mt7933_codec_debug_read();
    return 0;
}

void mt7933_codec_shutdown(struct snd_pcm_stream *substream,
                           struct snd_soc_dai *dai)
{
    aud_dbg("shutdown");

    if (substream->direction == MSD_STREAM_CAPTURE)
        mt7933_codec_disable_ul(dai->private_data);
    else if (substream->direction == MSD_STREAM_PLAYBACK)
        mt7933_codec_disable_dl(dai->private_data);

    if (substream->direction == MSD_STREAM_PLAYBACK)
        mt7933_codec_dac_powerdown(dai->private_data);
    else
        mt7933_codec_adc_powerdown(dai->private_data);
}

static int mt7933_codec_hw_params(struct snd_pcm_stream *substream, struct msd_hw_params *params, struct snd_soc_dai *dai)
{
    int ret = 0;
    unsigned int rate = dai->rate;

    aud_dbg("rate = %dHz", rate);

    if (substream->direction == MSD_STREAM_CAPTURE)
        ret = mt7933_codec_setup_ul_rate(dai->private_data, rate);
    else if (substream->direction == MSD_STREAM_PLAYBACK)
        ret = mt7933_codec_setup_dl_rate(dai->private_data, rate);

    if (ret < 0) {
        aud_error("error to setup rate");
        return ret;
    }

    if (substream->direction == MSD_STREAM_CAPTURE)
        mt7933_codec_configure_ul(dai->private_data, dai);
    else if (substream->direction == MSD_STREAM_PLAYBACK) {
        mt7933_codec_configure_dl(dai->private_data, rate);
    }
    if (substream->direction == MSD_STREAM_CAPTURE)
        mt7933_codec_enable_ul(dai->private_data);
    else if (substream->direction == MSD_STREAM_PLAYBACK) {
        mt7933_codec_enable_dl(dai->private_data);
    }

    return ret;
}

static const struct snd_soc_dai_pcm_ops mt7933_codec_dai_ops = {
    .hw_params = mt7933_codec_hw_params,
    .shutdown = mt7933_codec_shutdown,
    .reg_dump = mt7933_codec_reg_dump,
};

static struct snd_soc_dai_driver mt7933_codec_dais[] = {
    {
        .name = "mt7933-codec-dai-playback",
        .pcm_ops = &mt7933_codec_dai_ops,
        .constr = {
            .stream_name = "mt7933-codec-dai-playback",
            .formats = MT7933_CODEC_FORMATS,
            .rates = MT7933_CODEC_DL_RATES,
            .channels_min = 1,
            .channels_max = 2,
        },
    },
    {
        .name = "mt7933-codec-dai-capture",
        .pcm_ops = &mt7933_codec_dai_ops,
        .constr = {
            .stream_name = "mt7933-codec-dai-capture",
            .formats = MT7933_CODEC_FORMATS,
            .rates = MT7933_CODEC_UL_RATES,
            .channels_min = 1,
            .channels_max = 2,
        },
    },
};

static void mt7933_codec_init_regs(void *priv)
{
    mt7933_codec_dac_depop_setup(priv);

    mt7933_codec_adc_setup(priv);
}

static int mt7933_afe_init_codec_clk(struct mt7933_codec_priv *codec)
{
    unsigned int i;

    for (i = 0; i < sizeof(codec_clks) / sizeof(codec_clks[0]); i++) {
        codec->clk[i] = codec_clks[i];
    }
    return 0;
}

static int mt7933_codec_clk_probe(void *priv)
{
    int index = 0;
    int ret = 0;
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;

    mt7933_afe_init_codec_clk(mt7933_codec);

    for (index = 0; index < MT7933_CODEC_AUD_CLK_NUM; index ++) {
        ret = hal_clock_enable(mt7933_codec->clk[index]);
        if (ret)
            return ret;
    }
    return ret;
}

static int mt7933_codec_probe(void *priv)
{
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;
    //  int data[4] = {0, 0, 0, 0};
    //  int rawdata = 0;
    int ret = 0;

    aud_dbg("%s", "mt7933_codec_probe");

#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    mt7933_codec->dl_nle_support = 0;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    mt7933_codec->dl_nle_support = 1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    mt7933_codec->micbias0_setup_time_us = 0;

    spin_lock_init(&mt7933_codec->adda_afe_on_lock);

    ret = mt7933_codec_clk_probe(priv);
    if (ret < 0)
        return ret;

    mt7933_codec_init_regs(priv);
    return ret;
}

static int mt7933_codec_remove(void *priv)
{
    struct mt7933_codec_priv *mt7933_codec = (struct mt7933_codec_priv *)priv;;
    int index = 0;
    int ret = 0;

    for (index = 0; index < MT7933_CODEC_AUD_CLK_NUM; index ++) {
        ret = hal_clock_disable(mt7933_codec->clk[index]);
        if (ret)
            aud_error("hal clock disable error: %d, ret = %d", index, ret);
    }
    free(mt7933_codec);

    snd_soc_unregister_dais(mt7933_codec_dais);
    snd_soc_del_widget_routes(mt7933_codec_intercon);
    return 0;
}

void *codec_probe(void)
{
    struct mt7933_codec_priv *codec_priv = malloc(sizeof(*codec_priv));

    if (codec_priv == NULL)
        return NULL;

    memset(codec_priv, 0, sizeof(*codec_priv));
    mt7933_codec_probe(codec_priv);
    snd_soc_add_widgets(mt7933_codec_dapm_widgets, ARRAY_SIZE(mt7933_codec_dapm_widgets), codec_priv);
    snd_soc_add_controls(mt7933_codec_snd_controls, ARRAY_SIZE(mt7933_codec_snd_controls), codec_priv);
    snd_soc_add_widget_routes(mt7933_codec_intercon, ARRAY_SIZE(mt7933_codec_intercon));
    snd_soc_register_dais(mt7933_codec_dais, sizeof(mt7933_codec_dais) / sizeof(mt7933_codec_dais[0]), codec_priv);
    return codec_priv;
}

int codec_remove(void *priv)
{
    return mt7933_codec_remove(priv);
}
