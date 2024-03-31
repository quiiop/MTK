#include "mt7933-afe-utils.h"
#include "mt7933-reg.h"
#include "sound/utils/include/aud_log.h"
#include "sound/utils/include/afe_reg_rw.h"

#include "hal_clock.h"
#include "hal_platform.h"
//#include "mt7933_clk.h"

static const int aud_clks[MT7933_CLK_NUM] = {
    [MT7933_CLK_TOP_AUD_26M] = HAL_CLOCK_CG_AUDIO_XTAL,
    [MT7933_CLK_TOP_AUD_BUS] = HAL_CLOCK_CG_AUDSYS_BUS,
    [MT7933_CLK_FAPLL2] = HAL_CLOCK_CG_XPLL,
    [MT7933_CLK_APLL12_DIV0] = HAL_CLOCK_CG_APLL12_CK_DIV0,
    [MT7933_CLK_APLL12_DIV1] = HAL_CLOCK_CG_APLL12_CK_DIV6,
    [MT7933_CLK_APLL12_DIV2] = HAL_CLOCK_CG_APLL12_CK_DIV3,
    [MT7933_CLK_AUD_ADC0_XTAL] = HAL_CLOCK_CG_AUD_ADC0_XTAL,
    [MT7933_CLK_AUD_ADC1_XTAL] = HAL_CLOCK_CG_AUD_ADC1_XTAL,
    [MT7933_CLK_AUD_DAC_XTAL] = HAL_CLOCK_CG_AUD_DAC_XTAL,
    [MT7933_CLK_TOP_PLL] = HAL_CLOCK_CG_TOP_PLL,
};

static const int aud_clks_sel[MT7933_CLK_SEL_NUM] = {
    [MT7933_CLK_SEL_APLL12_DIV0] = HAL_CLOCK_SEL_APLL12_CK_DIV0,
    [MT7933_CLK_SEL_APLL12_DIV1] = HAL_CLOCK_SEL_APLL12_CK_DIV6,
    [MT7933_CLK_SEL_APLL12_DIV2] = HAL_CLOCK_SEL_APLL12_CK_DIV3,
};

int mt7933_afe_init_audio_clk(struct mtk_base_afe *afe)
{
    unsigned int i;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;

#ifdef PINMUX_QFN_DEFAULT
    hal_clock_mux_select(HAL_CLOCK_SEL_AUDIO_FAUD_INTBUS, CLK_AUDIO_FAUD_INTBUS_CLKSEL_133M);
    hal_clock_mux_select(HAL_CLOCK_SEL_AUDSYS_BUS, CLK_AUDSYS_BUS_CLKSEL_DIV_120M);
#elif defined PINMUX_BGA_DEFAULT
    hal_clock_mux_select(HAL_CLOCK_SEL_AUDIO_FAUD_INTBUS, CLK_AUDIO_FAUD_INTBUS_CLKSEL_266M);
    hal_clock_mux_select(HAL_CLOCK_SEL_AUDSYS_BUS, CLK_AUDSYS_BUS_CLKSEL_266M);
#endif /* #ifdef PINMUX_QFN_DEFAULT */
    hal_clock_mux_select(HAL_CLOCK_SEL_AUDIO_FASYS, CLK_AUDIO_FASYS_CLKSEL_XPLL_DIV4);
    hal_clock_mux_select(HAL_CLOCK_SEL_AUDIO_HAPLL, CLK_AUDIO_HAPLL_CLKSEL_XPLL_DIV4);

    for (i = 0; i < sizeof(aud_clks) / sizeof(aud_clks[0]); i++) {
        afe_priv->clocks[i] = aud_clks[i];
    }

    for (i = 0; i < sizeof(aud_clks_sel) / sizeof(aud_clks_sel[0]); i++) {
        afe_priv->clocks_sel[i] = aud_clks_sel[i];
    }
    return 0;
}

static int hal_cg_id_to_aud_id(hal_clock_cg_id clk)
{
    int i;
    for (i = 0; i < (int)sizeof(aud_clks) / (int)sizeof(aud_clks[0]); i++) {
        if (clk == aud_clks[i])
            return i;
    }
    return -1;
}

int mt7933_afe_enable_clk(struct mtk_base_afe *afe, hal_clock_cg_id clk)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    spin_lock_flags_define(flags);
    int need_update = 0;
    int ret = 0;
    int aud_id = hal_cg_id_to_aud_id(clk);
    if (aud_id < 0) {
        aud_error("clk error %d", clk);
        return ret;
    }

    spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

    afe_priv->hal_clock_ref_cnt[aud_id]++;
    if (afe_priv->hal_clock_ref_cnt[aud_id] == 1)
        need_update = 1;

    spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

    if (need_update) {
        aud_dbg("hal_clock_enable clk %d", clk);
        ret = hal_clock_enable(clk);
        if (ret) {
            aud_error("failed to enable clk");
            return ret;
        }

    }
    return ret;
}

void mt7933_afe_disable_clk(struct mtk_base_afe *afe, hal_clock_cg_id clk)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    spin_lock_flags_define(flags);
    bool need_update = 0;
    int aud_id = hal_cg_id_to_aud_id(clk);
    if (aud_id < 0) {
        aud_error("clk error %d", clk);
        return;
    }

    spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

    afe_priv->hal_clock_ref_cnt[aud_id]--;
    if (afe_priv->hal_clock_ref_cnt[aud_id] == 0)
        need_update = 1;
    else if (afe_priv->hal_clock_ref_cnt[aud_id] < 0)
        afe_priv->hal_clock_ref_cnt[aud_id] = 0;

    spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

    if (need_update) {
        aud_dbg("hal_clock_disable clk %d", clk);
        hal_clock_disable(clk);
    }
}

static unsigned int get_top_cg_reg(unsigned int cg_type)
{
    switch (cg_type) {
        case MT7933_TOP_CG_AFE:
        case MT7933_TOP_CG_APLL2:
        case MT7933_TOP_CG_DAC:
        case MT7933_TOP_CG_DAC_PREDIS:
        case MT7933_TOP_CG_ADC:
        case MT7933_TOP_CG_ADC2:
        case MT7933_TOP_CG_TML:
        case MT7933_TOP_CG_UPLINK_TML:
        case MT7933_TOP_CG_APLL2_TUNER:
            return AUDIO_TOP_CON0;
        case MT7933_TOP_CG_A1SYS_HOPPING:
        case MT7933_TOP_CG_DMIC_TML:
        case MT7933_TOP_CG_DMIC0:
        case MT7933_TOP_CG_DMIC1:
            return AUDIO_TOP_CON1;
        case MT7933_TOP_CG_I2S_IN:
        case MT7933_TOP_CG_TDM_IN:
        case MT7933_TOP_CG_I2S_OUT:
        case MT7933_TOP_CG_A1SYS:
        case MT7933_TOP_CG_A2SYS:
        case MT7933_TOP_CG_AFE_CONN:
        case MT7933_TOP_CG_GASRC0:
            return AUDIO_TOP_CON4;
        case MT7933_TOP_CG_A1SYS_TIMING:
        case MT7933_TOP_CG_A2SYS_TIMING:
        case MT7933_TOP_CG_26M_TIMING:
        case MT7933_TOP_CG_LP_MODE:
            return ASYS_TOP_CON;
        default:
            return 0;
    }
}

static unsigned int get_top_cg_mask(unsigned int cg_type)
{
    switch (cg_type) {
        case MT7933_TOP_CG_AFE:
            return AUD_TCON0_PDN_AFE;
        case MT7933_TOP_CG_APLL2:
            return AUD_TCON0_PDN_APLL1;
        case MT7933_TOP_CG_DAC:
            return AUD_TCON0_PDN_DAC;
        case MT7933_TOP_CG_DAC_PREDIS:
            return AUD_TCON0_PDN_DAC_PREDIS;
        case MT7933_TOP_CG_ADC:
            return AUD_TCON0_PDN_ADC;
        case MT7933_TOP_CG_ADC2:
            return AUD_TCON0_PDN_ADC2;
        case MT7933_TOP_CG_TML:
            return AUD_TCON0_PDN_TML;
        case MT7933_TOP_CG_UPLINK_TML:
            return AUD_TCON0_PDN_UPLINK_TML;
        case MT7933_TOP_CG_APLL2_TUNER:
            return AUD_TCON0_PDN_APLL1_TUNER;
        case MT7933_TOP_CG_A1SYS_HOPPING:
            return AUD_TCON1_PDN_A1SYS_HOPPING_CK;
        case MT7933_TOP_CG_I2S_IN:
            return AUD_TCON4_PDN_I2S_IN;
        case MT7933_TOP_CG_TDM_IN:
            return AUD_TCON4_PDN_TDM_IN;
        case MT7933_TOP_CG_I2S_OUT:
            return AUD_TCON4_PDN_I2S_OUT;
        case MT7933_TOP_CG_A1SYS:
            return AUD_TCON4_PDN_A1SYS;
        case MT7933_TOP_CG_A2SYS:
            return AUD_TCON4_PDN_A2SYS;
        case MT7933_TOP_CG_AFE_CONN:
            return AUD_TCON4_PDN_AFE_CONN;
        case MT7933_TOP_CG_GASRC0:
            return AUD_TCON4_PDN_GASRC0;
        case MT7933_TOP_CG_DMIC0:
            return AUD_TCON1_PDN_DMIC1_BCLK;
        case MT7933_TOP_CG_DMIC1:
            return AUD_TCON1_PDN_DMIC2_BCLK;
        case MT7933_TOP_CG_DMIC_TML:
            return AUD_TCON1_PDN_DMIC_TML;
        case MT7933_TOP_CG_A1SYS_TIMING:
            return ASYS_TCON_A1SYS_TIMING_ON;
        case MT7933_TOP_CG_A2SYS_TIMING:
            return ASYS_TCON_A2SYS_TIMING_ON;
        case MT7933_TOP_CG_26M_TIMING:
            return ASYS_TCON_LP_26M_ENGEN_ON;
        case MT7933_TOP_CG_LP_MODE:
            return ASYS_TCON_LP_MODE_ON;
        default:
            return 0;
    }
}

static unsigned int get_top_cg_on_val(unsigned int cg_type)
{
    switch (cg_type) {
        case MT7933_TOP_CG_A1SYS_TIMING:
        case MT7933_TOP_CG_A2SYS_TIMING:
        case MT7933_TOP_CG_26M_TIMING:
        case MT7933_TOP_CG_LP_MODE:
            return get_top_cg_mask(cg_type);
        default:
            return 0;
    }
}

static unsigned int get_top_cg_off_val(unsigned int cg_type)
{
    switch (cg_type) {
        case MT7933_TOP_CG_A1SYS_TIMING:
        case MT7933_TOP_CG_A2SYS_TIMING:
        case MT7933_TOP_CG_26M_TIMING:
        case MT7933_TOP_CG_LP_MODE:
            return 0;
        default:
            return get_top_cg_mask(cg_type);
    }
}

int mt7933_afe_enable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    unsigned int reg = get_top_cg_reg(cg_type);
    unsigned int mask = get_top_cg_mask(cg_type);
    unsigned int val = get_top_cg_on_val(cg_type);
    spin_lock_flags_define(flags);
    int need_update = 0;

    spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

    afe_priv->top_cg_ref_cnt[cg_type]++;
    if (afe_priv->top_cg_ref_cnt[cg_type] == 1)
        need_update = 1;

    spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

    if (need_update)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, reg, mask, val);
    //regmap_update_bits(afe->regmap, reg, mask, val);

    return 0;
}

int mt7933_afe_enable_afe_on(struct mtk_base_afe *afe)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    spin_lock_flags_define(flags);
    int need_update = 0;

    spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

    afe_priv->afe_on_ref_cnt++;
    if (afe_priv->afe_on_ref_cnt == 1)
        need_update = 1;

    spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

    if (need_update)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DAC_CON0, 0x1, 0x1);
    //regmap_update_bits(afe->regmap, AFE_DAC_CON0, 0x1, 0x1);

    return 0;
}

int mt7933_afe_change_top_reg_point(struct mtk_base_afe *afe)
{
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON,
                                  ASYS_TCON_REG_START_POINT_MASK,
                                  ASYS_TCON_REG_START_POINT_VAL(0xf));

    return 0;
}

int mt7933_afe_enable_main_clk(struct mtk_base_afe *afe)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;

    aud_dbg("begin");
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_26M]);
    //  mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_PLL]);
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_FAPLL2]);
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_BUS]);

    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A1SYS);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A2SYS);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_AFE);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A1SYS_HOPPING);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_AFE_CONN);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A1SYS_TIMING);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A2SYS_TIMING);
    mt7933_afe_change_top_reg_point(afe);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_26M_TIMING);
    //  mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_LP_MODE);

    mt7933_afe_enable_afe_on(afe);

    return 0;
}

int mt7933_afe_disable_afe_on(struct mtk_base_afe *afe)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    spin_lock_flags_define(flags);
    bool need_update = 0;

    spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

    afe_priv->afe_on_ref_cnt--;
    if (afe_priv->afe_on_ref_cnt == 0)
        need_update = 1;
    else if (afe_priv->afe_on_ref_cnt < 0)
        afe_priv->afe_on_ref_cnt = 0;

    spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

    if (need_update)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DAC_CON0, 0x1, 0x0);

    return 0;
}

int mt7933_afe_disable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    unsigned int reg = get_top_cg_reg(cg_type);
    unsigned int mask = get_top_cg_mask(cg_type);
    unsigned int val = get_top_cg_off_val(cg_type);
    spin_lock_flags_define(flags);
    bool need_update = 0;

    spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

    afe_priv->top_cg_ref_cnt[cg_type]--;
    if (afe_priv->top_cg_ref_cnt[cg_type] == 0)
        need_update = 1;
    else if (afe_priv->top_cg_ref_cnt[cg_type] < 0)
        afe_priv->top_cg_ref_cnt[cg_type] = 0;

    spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

    if (need_update)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, reg, mask, val);

    return 0;
}

int mt7933_afe_disable_main_clk(struct mtk_base_afe *afe)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;

    aud_dbg("begin");

    mt7933_afe_disable_afe_on(afe);

    //  mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_LP_MODE);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_26M_TIMING);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_A2SYS_TIMING);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_A1SYS_TIMING);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_AFE_CONN);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_A1SYS_HOPPING);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_AFE);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_A2SYS);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_A1SYS);

    mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_BUS]);
    mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_FAPLL2]);
    //  mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_PLL]);
    mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_26M]);

    return 0;
}

int mt7933_afe_disable_apll_tuner_cfg(struct mtk_base_afe *afe,
                                      unsigned int apll)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;

    mutex_lock(afe_priv->afe_clk_mutex);

    afe_priv->apll_tuner_ref_cnt[apll]--;
    if (afe_priv->apll_tuner_ref_cnt[apll] == 0) {
        if (apll == MT7933_AFE_APLL2)
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_APLL_TUNER_CFG,
                                          AFE_APLL_TUNER_CFG_EN_MASK, 0x0);
        else
            aud_error("APLL1 don't work at 7933");

    } else if (afe_priv->apll_tuner_ref_cnt[apll] < 0) {
        afe_priv->apll_tuner_ref_cnt[apll] = 0;
    }

    mutex_unlock(afe_priv->afe_clk_mutex);
    return 0;
}

int mt7933_afe_enable_apll_tuner_cfg(struct mtk_base_afe *afe,
                                     unsigned int apll)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;

    mutex_lock(afe_priv->afe_clk_mutex);

    afe_priv->apll_tuner_ref_cnt[apll]++;
    if (afe_priv->apll_tuner_ref_cnt[apll] != 1) {
        mutex_unlock(afe_priv->afe_clk_mutex);
        return 0;
    }

    if (apll == MT7933_AFE_APLL2) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_APLL_TUNER_CFG,
                                      AFE_APLL_TUNER_CFG_MASK, 0x212);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_APLL_TUNER_CFG,
                                      AFE_APLL_TUNER_CFG_EN_MASK, 0x1);
    } else {
        aud_error("APLL1 don't work at 7933");
    }

    mutex_unlock(afe_priv->afe_clk_mutex);
    return 0;
}

int mt7933_afe_enable_apll_associated_cfg(struct mtk_base_afe *afe,
                                          unsigned int apll)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;

    if (apll == MT7933_AFE_APLL1) {
        //      hal_clock_enable(afe_priv->clocks[MT7933_CLK_FAPLL1]);
        //      mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_APLL);
        //      mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_APLL_TUNER);
        mt7933_afe_enable_apll_tuner_cfg(afe, MT7933_AFE_APLL1);
    } else {
        hal_clock_enable(afe_priv->clocks[MT7933_CLK_FAPLL2]);
        mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_APLL2);
        mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_APLL2_TUNER);
        mt7933_afe_enable_apll_tuner_cfg(afe, MT7933_AFE_APLL2);
    }

    return 0;
}

int mt7933_afe_set_clk_rate(struct mtk_base_afe *afe, hal_clock_sel_id clk_sel,
                            unsigned int rate)
{
    int ret = 0;

    switch (clk_sel) {
        case HAL_CLOCK_SEL_APLL12_CK_DIV0:
            aud_msg("");
            ret = hal_clock_set_rate(clk_sel, rate);
            if (ret) {
                aud_error("Failed to set rate, ret = %d", ret);
                return ret;
            }
            ret = hal_clock_enable(HAL_CLOCK_CG_APLL12_CK_DIV0);
            if (ret) {
                aud_error("Failed to enable clock %d, ret = %d", HAL_CLOCK_CG_APLL12_CK_DIV0, ret);
                return ret;
            }
            break;

        case HAL_CLOCK_SEL_APLL12_CK_DIV3:
            aud_msg("");
            ret = hal_clock_set_rate(clk_sel, rate);
            if (ret) {
                aud_error("Failed to set rate, ret = %d", ret);
                return ret;
            }
            ret = hal_clock_enable(HAL_CLOCK_CG_APLL12_CK_DIV3);
            if (ret) {
                aud_error("Failed to enable clock %d, ret = %d", HAL_CLOCK_CG_APLL12_CK_DIV3, ret);
                return ret;
            }
            break;

        case HAL_CLOCK_SEL_APLL12_CK_DIV6:
            aud_msg("");
            ret = hal_clock_set_rate(clk_sel, rate);
            if (ret) {
                aud_error("Failed to set rate, ret = %d", ret);
                return ret;
            }
            ret = hal_clock_enable(HAL_CLOCK_CG_APLL12_CK_DIV6);
            if (ret) {
                aud_error("Failed to enable clock %d, ret = %d", HAL_CLOCK_CG_APLL12_CK_DIV6, ret);
                return ret;
            }
            break;

        default:
            aud_error("clk id error: clk = %d", clk_sel);
            break;
    }

    return ret;
}

