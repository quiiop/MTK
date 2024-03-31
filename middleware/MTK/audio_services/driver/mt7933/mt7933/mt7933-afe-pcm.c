#include "sound/driver/include/soc.h"
#include "sound/include/asound.h"
#include "sound/driver/include/pcm.h"
#include "sound/utils/include/afe_reg_rw.h"
#include "sound/utils/include/aud_memory.h"
#include "../common/mtk_base_afe.h"
#include "mt7933-afe-common.h"
#include "mt7933-reg.h"
#include "mt7933-afe-utils.h"
#include "mt7933-codec.h"

#include "hal_nvic.h"
#include "hal_clock.h"
#include "memory_map.h"
#include <stdbool.h>
#include <string.h>
#include "audio_test_utils.h"
#include "hal_sleep_manager_internal.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif /* #ifndef ARRAY_SIZE */

/**
 * upper_32_bits - return bits 32-63 of a number
 * @n: the number we're accessing
 *
 * A basic shift-right of a 64- or 32-bit quantity.  Use this to suppress
 * the "right shift count >= width of type" warning when that quantity is
 * 32-bits.
 */
#define upper_32_bits(n) ((unsigned int)(((n) >> 16) >> 16))

/**
 * lower_32_bits - return bits 0-31 of a number
 * @n: the number we're accessing
 */
#define lower_32_bits(n) ((unsigned int)(n))
#define MT7933_ETDM1_IN_MCLK_MULTIPLIER 256
#define MT7933_ETDM2_OUT_MCLK_MULTIPLIER 256
#define MT7933_ETDM2_IN_MCLK_MULTIPLIER 256
#define MT7933_ETDM_NORMAL_MAX_BCK_RATE 24576000

#define AFE_BASE_END_OFFSET 8

#ifdef CONFIG_MTK_ADSP_SUPPORT
struct mtk_base_afe *g_priv;
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */

static struct mtk_base_afe *mt7933_afe;

static const unsigned int mt7933_afe_backup_list[] = {
    AUDIO_TOP_CON0,
    AUDIO_TOP_CON1,
    AUDIO_TOP_CON3,
    AUDIO_TOP_CON4,
    AUDIO_TOP_CON5,
    ASYS_TOP_CON,
    AFE_DAC_CON0,
    PWR2_TOP_CON0,
    //  AFE_CONN4,
    //  AFE_CONN5,
    //  AFE_CONN12,
    //  AFE_CONN13,
    //  AFE_CONN14,
    //  AFE_CONN15,
    //  AFE_CONN16,
    //  AFE_CONN17,
    //  AFE_CONN18,
    //  AFE_CONN19,
    //  AFE_CONN20,
    //  AFE_CONN21,
    //  AFE_CONN26,
    //  AFE_CONN27,
    //  AFE_CONN28,
    //  AFE_CONN29,
    //  AFE_CONN30,
    //  AFE_CONN31,
    //  AFE_CONN32,
    //  AFE_CONN33,
    //  AFE_CONN34,
    //  AFE_CONN35,
    //  AFE_CONN36,
    //  AFE_CONN37,
    //  AFE_CONN38,
    //  AFE_CONN39,
    //  AFE_CONN40,
    //  AFE_CONN41,
    //  AFE_CONN42,
    //  AFE_CONN43,
    //  AFE_CONN4_1,
    //  AFE_CONN5_1,
    //  AFE_CONN12_1,
    //  AFE_CONN13_1,
    //  AFE_CONN14_1,
    //  AFE_CONN15_1,
    //  AFE_CONN16_1,
    //  AFE_CONN17_1,
    //  AFE_CONN18_1,
    //  AFE_CONN19_1,
    //  AFE_CONN20_1,
    //  AFE_CONN21_1,
    //  AFE_CONN22_1,
    //  AFE_CONN23_1,
    //  AFE_CONN26_1,
    //  AFE_CONN27_1,
    //  AFE_CONN28_1,
    //  AFE_CONN29_1,
    //  AFE_CONN30_1,
    //  AFE_CONN31_1,
    //  AFE_CONN32_1,
    //  AFE_CONN33_1,
    //  AFE_CONN34_1,
    //  AFE_CONN35_1,
    //  AFE_CONN36_1,
    //  AFE_CONN37_1,
    //  AFE_CONN38_1,
    //  AFE_CONN39_1,
    //  AFE_CONN40_1,
    //  AFE_CONN41_1,
    //  AFE_CONN42_1,
    //  AFE_CONN43_1,
    //  AFE_CONN_RS,
    //  AFE_CONN_RS_1,
    //  AFE_CONN_16BIT,
    //  AFE_CONN_24BIT,
    //  AFE_CONN_16BIT_1,
    //  AFE_CONN_24BIT_1,
    //  AFE_DL2_BASE,
    //  AFE_DL2_END,
    //  AFE_DL3_BASE,
    //  AFE_DL3_END,
    //  AFE_DL10_BASE,
    //  AFE_DL10_END,
    //  AFE_UL2_BASE,
    //  AFE_UL2_END,
    //  AFE_UL3_BASE,
    //  AFE_UL3_END,
    //  AFE_UL4_BASE,
    //  AFE_UL4_END,
    //  AFE_UL8_BASE,
    //  AFE_UL8_END,
    //  AFE_UL9_BASE,
    //  AFE_UL9_END,
    //  AFE_UL10_BASE,
    //  AFE_UL10_END,
    //  AFE_DAC_CON0,
    //  AFE_IRQ_MASK,
    //  ETDM_IN2_CON0,
    AFE_SINEGEN_CON0,
    AFE_SINEGEN_CON1,
    ABB_ULAFE_CON0,
    ABB_ULAFE_CON1,
    ABB_UL2AFE_CON0,
    ABB_UL2AFE_CON1,
    AFE_NLE_GAIN_IMP_LCH_CFG0,
    AFE_NLE_GAIN_IMP_RCH_CFG0,
    ABB_AFE_CON0,
    AFE_ADDA_UL_DL_CON0,
    AMIC_GAIN_CON0,
    AMIC_GAIN_CON1,
    AMIC_GAIN_CON2,
    AMIC_GAIN_CON3,
    UL2_AMIC_GAIN_CON0,
    UL2_AMIC_GAIN_CON1,
    UL2_AMIC_GAIN_CON2,
    UL2_AMIC_GAIN_CON3,
};

static const unsigned int mt7933_clk_mux_backup_list[] = {
    HAL_CLOCK_SEL_AUDIO_FAUD_INTBUS,
    HAL_CLOCK_SEL_AUDSYS_BUS,
    HAL_CLOCK_SEL_AUDIO_FASYS,
    HAL_CLOCK_SEL_AUDIO_HAPLL,
    HAL_CLOCK_SEL_AUDIO_FAUDIO,
    HAL_CLOCK_SEL_AUDIO_FASM,
};

static const unsigned int mt7933_codec_backup_list[] = {
    AUD_DAC_CON00,
    AUD_DAC_CON01,
    AUD_DAC_CON02,
    AUD_DAC_CON03,
    AUD_DAC_CON04,
    AUD_DAC_CON05,
    AUD_ADC0_COMMON,
    AUD_ADC0_RSVD,
    AUD_ADC0_LCH_CON00,
    AUD_ADC0_LCH_CON01,
    AUD_ADC0_LCH_CON02,
    AUD_ADC0_LCH_CON03,
    AUD_ADC0_LCH_CON04,
    AUD_ADC0_LCH_CON05,
    AUD_ADC0_RCH_CON00,
    AUD_ADC0_RCH_CON01,
    AUD_ADC0_RCH_CON02,
    AUD_ADC0_RCH_CON03,
    AUD_ADC0_RCH_CON04,
    AUD_ADC0_RCH_CON05,

    AUD_ADC1_COMMON,
    AUD_ADC1_RSVD,
    AUD_ADC1_LCH_CON00,
    AUD_ADC1_LCH_CON01,
    AUD_ADC1_LCH_CON02,
    AUD_ADC1_LCH_CON03,
    AUD_ADC1_LCH_CON04,
    AUD_ADC1_LCH_CON05,
    AUD_ADC1_RCH_CON00,
    AUD_ADC1_RCH_CON01,
    AUD_ADC1_RCH_CON02,
    AUD_ADC1_RCH_CON03,
    AUD_ADC1_RCH_CON04,
    AUD_ADC1_RCH_CON05,
};

struct io_port {
    char *name;
    int port;
};
struct io_port_route {
    struct io_port source;
    struct io_port sink;
    int old_value;
    int new_value;
    struct soc_ctl_entry *ctl_entry;
};

typedef enum {
    I_00, I_01, I_02, I_03, I_04, I_05, I_06, I_07,
    I_08, I_09, I_10, I_11, I_12, I_13, I_14, I_15,
    I_16, I_17, I_18, I_19, I_20, I_21, I_22, I_23,
    I_24, I_25, I_26, I_27, I_28, I_29, I_30, I_31,
    I_32, I_33, I_34, I_35, I_36, I_37, I_38, I_39,
    I_40, I_41, I_42, I_43, I_44, I_45, I_46, I_47,
    I_48, I_49, I_50, I_51, I_52, I_53, I_54, I_55,
    I_56, I_57, I_58, I_59, I_60, I_61, I_MAX,
} IO_IN_PORT;

typedef enum {
    O_00, O_01, O_02, O_03, O_04, O_05, O_06, O_07,
    O_08, O_09, O_10, O_11, O_12, O_13, O_14, O_15,
    O_16, O_17, O_18, O_19, O_20, O_21, O_22, O_23,
    O_24, O_25, O_26, O_27, O_28, O_29, O_30, O_31,
    O_32, O_33, O_34, O_35, O_36, O_37, O_38, O_39,
    O_40, O_41, O_42, O_43, O_44, O_45, O_46, O_47,
    O_48, O_49, O_MAX,
} IO_OUT_PORT;

struct afe_interconn {
    unsigned int reg;    /* register addr */
    unsigned int offset; /* start bit of Ix_Oy_S or Ix_Oy_S_H */
};

struct afe_dump_reg_attr {
    unsigned int offset;
    char *name;
};

#define DUMP_REG_ENTRY(reg) {reg, #reg}

static const struct afe_dump_reg_attr memif_dump_regs[] = {
    DUMP_REG_ENTRY(ASYS_TOP_CON),
    DUMP_REG_ENTRY(AUDIO_TOP_CON5),
    DUMP_REG_ENTRY(AFE_DAC_CON0),
    DUMP_REG_ENTRY(AFE_DAC_CON1),
    DUMP_REG_ENTRY(AFE_DL2_BASE),
    DUMP_REG_ENTRY(AFE_DL2_CUR),
    DUMP_REG_ENTRY(AFE_DL2_END),
    DUMP_REG_ENTRY(AFE_DL2_CON0),
    DUMP_REG_ENTRY(AFE_DL3_BASE),
    DUMP_REG_ENTRY(AFE_DL3_CUR),
    DUMP_REG_ENTRY(AFE_DL3_END),
    DUMP_REG_ENTRY(AFE_DL3_CON0),
    DUMP_REG_ENTRY(AFE_DL10_BASE),
    DUMP_REG_ENTRY(AFE_DL10_CUR),
    DUMP_REG_ENTRY(AFE_DL10_END),
    DUMP_REG_ENTRY(AFE_DL10_CON0),
    DUMP_REG_ENTRY(AFE_UL2_BASE),
    DUMP_REG_ENTRY(AFE_UL2_CUR),
    DUMP_REG_ENTRY(AFE_UL2_END),
    DUMP_REG_ENTRY(AFE_UL2_CON0),
    DUMP_REG_ENTRY(AFE_UL3_BASE),
    DUMP_REG_ENTRY(AFE_UL3_CUR),
    DUMP_REG_ENTRY(AFE_UL3_END),
    DUMP_REG_ENTRY(AFE_UL3_CON0),
    DUMP_REG_ENTRY(AFE_UL4_BASE),
    DUMP_REG_ENTRY(AFE_UL4_CUR),
    DUMP_REG_ENTRY(AFE_UL4_END),
    DUMP_REG_ENTRY(AFE_UL4_CON0),
    DUMP_REG_ENTRY(AFE_UL8_BASE),
    DUMP_REG_ENTRY(AFE_UL8_CUR),
    DUMP_REG_ENTRY(AFE_UL8_END),
    DUMP_REG_ENTRY(AFE_UL8_CON0),
    DUMP_REG_ENTRY(AFE_UL9_BASE),
    DUMP_REG_ENTRY(AFE_UL9_CUR),
    DUMP_REG_ENTRY(AFE_UL9_END),
    DUMP_REG_ENTRY(AFE_UL9_CON0),
    DUMP_REG_ENTRY(AFE_UL10_BASE),
    DUMP_REG_ENTRY(AFE_UL10_CUR),
    DUMP_REG_ENTRY(AFE_UL10_END),
    DUMP_REG_ENTRY(AFE_UL10_CON0),
    DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON0),
    DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON1),
    DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON2),
    DUMP_REG_ENTRY(AFE_MEMIF_AGENT_FS_CON3),
    DUMP_REG_ENTRY(AFE_I2S_UL9_REORDER),
    DUMP_REG_ENTRY(AFE_I2S_UL2_REORDER),
};

static const struct afe_dump_reg_attr irq_dump_regs[] = {
    DUMP_REG_ENTRY(ASYS_TOP_CON),
    DUMP_REG_ENTRY(ASYS_IRQ1_CON),
    DUMP_REG_ENTRY(ASYS_IRQ2_CON),
    DUMP_REG_ENTRY(ASYS_IRQ3_CON),
    DUMP_REG_ENTRY(ASYS_IRQ4_CON),
    DUMP_REG_ENTRY(ASYS_IRQ5_CON),
    DUMP_REG_ENTRY(ASYS_IRQ6_CON),
    DUMP_REG_ENTRY(ASYS_IRQ7_CON),
    DUMP_REG_ENTRY(ASYS_IRQ8_CON),
    DUMP_REG_ENTRY(ASYS_IRQ9_CON),
    DUMP_REG_ENTRY(ASYS_IRQ10_CON),
    DUMP_REG_ENTRY(ASYS_IRQ11_CON),
    DUMP_REG_ENTRY(ASYS_IRQ12_CON),
    DUMP_REG_ENTRY(ASYS_IRQ13_CON),
    DUMP_REG_ENTRY(ASYS_IRQ14_CON),
    DUMP_REG_ENTRY(ASYS_IRQ15_CON),
    DUMP_REG_ENTRY(ASYS_IRQ16_CON),
    DUMP_REG_ENTRY(ASYS_IRQ_CLR),
    DUMP_REG_ENTRY(ASYS_IRQ_STATUS),
    DUMP_REG_ENTRY(AFE_IRQ_MCU_CLR),
    DUMP_REG_ENTRY(AFE_IRQ_STATUS),
    DUMP_REG_ENTRY(AFE_IRQ_MASK),
    DUMP_REG_ENTRY(ASYS_IRQ_MASK),
};

static const struct afe_dump_reg_attr conn_dump_regs[] = {
    DUMP_REG_ENTRY(AFE_CONN4),
    DUMP_REG_ENTRY(AFE_CONN5),
    DUMP_REG_ENTRY(AFE_CONN12),
    DUMP_REG_ENTRY(AFE_CONN13),
    DUMP_REG_ENTRY(AFE_CONN14),
    DUMP_REG_ENTRY(AFE_CONN15),
    DUMP_REG_ENTRY(AFE_CONN16),
    DUMP_REG_ENTRY(AFE_CONN17),
    DUMP_REG_ENTRY(AFE_CONN18),
    DUMP_REG_ENTRY(AFE_CONN19),
    DUMP_REG_ENTRY(AFE_CONN20),
    DUMP_REG_ENTRY(AFE_CONN21),
    DUMP_REG_ENTRY(AFE_CONN26),
    DUMP_REG_ENTRY(AFE_CONN27),
    DUMP_REG_ENTRY(AFE_CONN28),
    DUMP_REG_ENTRY(AFE_CONN29),
    DUMP_REG_ENTRY(AFE_CONN30),
    DUMP_REG_ENTRY(AFE_CONN31),
    DUMP_REG_ENTRY(AFE_CONN32),
    DUMP_REG_ENTRY(AFE_CONN33),
    DUMP_REG_ENTRY(AFE_CONN34),
    DUMP_REG_ENTRY(AFE_CONN35),
    DUMP_REG_ENTRY(AFE_CONN36),
    DUMP_REG_ENTRY(AFE_CONN37),
    DUMP_REG_ENTRY(AFE_CONN38),
    DUMP_REG_ENTRY(AFE_CONN39),
    DUMP_REG_ENTRY(AFE_CONN40),
    DUMP_REG_ENTRY(AFE_CONN41),
    DUMP_REG_ENTRY(AFE_CONN42),
    DUMP_REG_ENTRY(AFE_CONN43),
    DUMP_REG_ENTRY(AFE_CONN4_1),
    DUMP_REG_ENTRY(AFE_CONN5_1),
    DUMP_REG_ENTRY(AFE_CONN12_1),
    DUMP_REG_ENTRY(AFE_CONN13_1),
    DUMP_REG_ENTRY(AFE_CONN14_1),
    DUMP_REG_ENTRY(AFE_CONN15_1),
    DUMP_REG_ENTRY(AFE_CONN16_1),
    DUMP_REG_ENTRY(AFE_CONN17_1),
    DUMP_REG_ENTRY(AFE_CONN18_1),
    DUMP_REG_ENTRY(AFE_CONN19_1),
    DUMP_REG_ENTRY(AFE_CONN20_1),
    DUMP_REG_ENTRY(AFE_CONN21_1),
    DUMP_REG_ENTRY(AFE_CONN26_1),
    DUMP_REG_ENTRY(AFE_CONN27_1),
    DUMP_REG_ENTRY(AFE_CONN28_1),
    DUMP_REG_ENTRY(AFE_CONN29_1),
    DUMP_REG_ENTRY(AFE_CONN30_1),
    DUMP_REG_ENTRY(AFE_CONN31_1),
    DUMP_REG_ENTRY(AFE_CONN32_1),
    DUMP_REG_ENTRY(AFE_CONN33_1),
    DUMP_REG_ENTRY(AFE_CONN34_1),
    DUMP_REG_ENTRY(AFE_CONN35_1),
    DUMP_REG_ENTRY(AFE_CONN36_1),
    DUMP_REG_ENTRY(AFE_CONN37_1),
    DUMP_REG_ENTRY(AFE_CONN38_1),
    DUMP_REG_ENTRY(AFE_CONN39_1),
    DUMP_REG_ENTRY(AFE_CONN40_1),
    DUMP_REG_ENTRY(AFE_CONN41_1),
    DUMP_REG_ENTRY(AFE_CONN42_1),
    DUMP_REG_ENTRY(AFE_CONN43_1),
    DUMP_REG_ENTRY(AFE_CONN_16BIT),
    DUMP_REG_ENTRY(AFE_CONN_24BIT),
    DUMP_REG_ENTRY(AFE_CONN_16BIT_1),
    DUMP_REG_ENTRY(AFE_CONN_24BIT_1),
    DUMP_REG_ENTRY(AFE_CONN_RS),
    DUMP_REG_ENTRY(AFE_CONN_RS_1),
};

static const struct afe_dump_reg_attr adda_dump_regs[] = {
    DUMP_REG_ENTRY(AFE_SINEGEN_CON0),
    DUMP_REG_ENTRY(AFE_SINEGEN_CON1),
    DUMP_REG_ENTRY(AUDIO_TOP_CON0),
    DUMP_REG_ENTRY(AUDIO_TOP_CON3),
    DUMP_REG_ENTRY(AUDIO_TOP_CON4),
    DUMP_REG_ENTRY(ASMO_TIMING_CON0),
    DUMP_REG_ENTRY(PWR2_TOP_CON0),
    DUMP_REG_ENTRY(PWR2_TOP_CON1),
    DUMP_REG_ENTRY(AFE_DMIC0_UL_SRC_CON0),
    DUMP_REG_ENTRY(AFE_DMIC0_UL_SRC_CON1),
    DUMP_REG_ENTRY(AFE_DMIC0_IIR_COEF_02_01),
    DUMP_REG_ENTRY(AFE_DMIC0_IIR_COEF_04_03),
    DUMP_REG_ENTRY(AFE_DMIC0_IIR_COEF_06_05),
    DUMP_REG_ENTRY(AFE_DMIC0_IIR_COEF_08_07),
    DUMP_REG_ENTRY(AFE_DMIC0_IIR_COEF_10_09),
    DUMP_REG_ENTRY(AFE_DMIC1_UL_SRC_CON0),
    DUMP_REG_ENTRY(AFE_DMIC1_UL_SRC_CON1),
    DUMP_REG_ENTRY(AFE_DMIC1_IIR_COEF_02_01),
    DUMP_REG_ENTRY(AFE_DMIC1_IIR_COEF_04_03),
    DUMP_REG_ENTRY(AFE_DMIC1_IIR_COEF_06_05),
    DUMP_REG_ENTRY(AFE_DMIC1_IIR_COEF_08_07),
    DUMP_REG_ENTRY(AFE_DMIC1_IIR_COEF_10_09),
};

static const struct afe_dump_reg_attr gasrc_dump_regs[] = {
    DUMP_REG_ENTRY(AUDIO_TOP_CON0),
    DUMP_REG_ENTRY(AUDIO_TOP_CON4),
    DUMP_REG_ENTRY(ASMO_TIMING_CON0),
    DUMP_REG_ENTRY(PWR1_ASM_CON1),
    DUMP_REG_ENTRY(GASRC_CFG0),
    DUMP_REG_ENTRY(GASRC_TIMING_CON0),
    DUMP_REG_ENTRY(GASRC_TIMING_CON1),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON0),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON1),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON2),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON3),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON4),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON6),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON7),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON8),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON9),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON10),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON11),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON13),
    DUMP_REG_ENTRY(AFE_GASRC0_NEW_CON14),
};

static const struct afe_dump_reg_attr etdm_dump_regs[] = {
    DUMP_REG_ENTRY(AUDIO_TOP_CON0),
    DUMP_REG_ENTRY(AUDIO_TOP_CON4),
    DUMP_REG_ENTRY(AUDIO_TOP_CON5),
    DUMP_REG_ENTRY(ASYS_TOP_CON),
    DUMP_REG_ENTRY(AFE_DAC_CON0),
    DUMP_REG_ENTRY(ETDM_IN1_CON0),
    DUMP_REG_ENTRY(ETDM_IN1_CON1),
    DUMP_REG_ENTRY(ETDM_IN1_CON2),
    DUMP_REG_ENTRY(ETDM_IN1_CON3),
    DUMP_REG_ENTRY(ETDM_IN1_CON4),
    DUMP_REG_ENTRY(ETDM_IN2_CON0),
    DUMP_REG_ENTRY(ETDM_IN2_CON1),
    DUMP_REG_ENTRY(ETDM_IN2_CON2),
    DUMP_REG_ENTRY(ETDM_IN2_CON3),
    DUMP_REG_ENTRY(ETDM_IN2_CON4),
    DUMP_REG_ENTRY(ETDM_OUT2_CON0),
    DUMP_REG_ENTRY(ETDM_OUT2_CON1),
    DUMP_REG_ENTRY(ETDM_OUT2_CON2),
    DUMP_REG_ENTRY(ETDM_OUT2_CON3),
    DUMP_REG_ENTRY(ETDM_OUT2_CON4),
    DUMP_REG_ENTRY(ETDM_COWORK_CON0),
    DUMP_REG_ENTRY(ETDM_COWORK_CON1),
    DUMP_REG_ENTRY(AFE_APLL_TUNER_CFG),
};

static const struct mtk_base_memif_data memif_data[MT7933_AFE_MEMIF_NUM] = {
    {
        .name = "DLM",
        .id = MT7933_AFE_MEMIF_DLM,
        .reg_ofs_base = AFE_DL10_BASE,
        .reg_ofs_cur = AFE_DL10_CUR,
        .fs_reg = AFE_MEMIF_AGENT_FS_CON1,
        .fs_shift = 20,
        .fs_maskbit = 0x1f,
        .mono_reg = -1,
        .mono_shift = -1,
        .hd_reg = AFE_DL10_CON0,
        .hd_shift = 5,
        .enable_reg = AFE_DAC_CON0,
        .enable_shift = 26,
        .msb_reg = AFE_NORMAL_BASE_ADR_MSB,
        .msb_shift = 26,
        .msb2_reg = AFE_NORMAL_END_ADR_MSB,
        .msb2_shift = 26,
        .agent_disable_reg = AUDIO_TOP_CON5,
        .agent_disable_shift = 26,
        .ch_config_reg = AFE_DL10_CON0,
        .ch_config_shift = 0,
        .int_odd_reg = -1,
        .int_odd_shift = -1,
        .buffer_bytes_align = 64,
    }, {
        .name = "DL2",
        .id = MT7933_AFE_MEMIF_DL2,
        .reg_ofs_base = AFE_DL2_BASE,
        .reg_ofs_cur = AFE_DL2_CUR,
        .fs_reg = AFE_MEMIF_AGENT_FS_CON0,
        .fs_shift = 10,
        .fs_maskbit = 0x1f,
        .mono_reg = -1,
        .mono_shift = -1,
        .hd_reg = AFE_DL2_CON0,
        .hd_shift = 5,
        .enable_reg = AFE_DAC_CON0,
        .enable_shift = 18,
        .msb_reg = AFE_NORMAL_BASE_ADR_MSB,
        .msb_shift = 18,
        .msb2_reg = AFE_NORMAL_END_ADR_MSB,
        .msb2_shift = 18,
        .agent_disable_reg = AUDIO_TOP_CON5,
        .agent_disable_shift = 18,
        .ch_config_reg = AFE_DL2_CON0,
        .ch_config_shift = 0,
        .int_odd_reg = -1,
        .int_odd_shift = -1,
        .buffer_bytes_align = 64,
    }, {
        .name = "DL3",
        .id = MT7933_AFE_MEMIF_DL3,
        .reg_ofs_base = AFE_DL3_BASE,
        .reg_ofs_cur = AFE_DL3_CUR,
        .fs_reg = AFE_MEMIF_AGENT_FS_CON0,
        .fs_shift = 15,
        .fs_maskbit = 0x1f,
        .mono_reg = -1,
        .mono_shift = -1,
        .hd_reg = AFE_DL3_CON0,
        .hd_shift = 5,
        .enable_reg = AFE_DAC_CON0,
        .enable_shift = 19,
        .msb_reg = AFE_NORMAL_BASE_ADR_MSB,
        .msb_shift = 19,
        .msb2_reg = AFE_NORMAL_END_ADR_MSB,
        .msb2_shift = 19,
        .agent_disable_reg = AUDIO_TOP_CON5,
        .agent_disable_shift = 19,
        .ch_config_reg = AFE_DL3_CON0,
        .ch_config_shift = 0,
        .int_odd_reg = -1,
        .int_odd_shift = -1,
        .buffer_bytes_align = 64,
    }, {
        .name = "UL2",
        .id = MT7933_AFE_MEMIF_UL2,
        .reg_ofs_base = AFE_UL2_BASE,
        .reg_ofs_cur = AFE_UL2_CUR,
        .fs_reg = AFE_MEMIF_AGENT_FS_CON2,
        .fs_shift = 5,
        .fs_maskbit = 0x1f,
        .mono_reg = AFE_UL2_CON0,
        .mono_shift = 16,
        .hd_reg = AFE_UL2_CON0,
        .hd_shift = 5,
        .enable_reg = AFE_DAC_CON0,
        .enable_shift = 2,
        .msb_reg = AFE_NORMAL_BASE_ADR_MSB,
        .msb_shift = 1,
        .msb2_reg = AFE_NORMAL_END_ADR_MSB,
        .msb2_shift = 1,
        .agent_disable_reg = AUDIO_TOP_CON5,
        .agent_disable_shift = 1,
        .ch_config_reg = -1,
        .ch_config_shift = -1,
        .int_odd_reg = AFE_UL2_CON0,
        .int_odd_shift = 0,
        .buffer_bytes_align = 64,
    }, {
        .name = "UL3",
        .id = MT7933_AFE_MEMIF_UL3,
        .reg_ofs_base = AFE_UL3_BASE,
        .reg_ofs_cur = AFE_UL3_CUR,
        .fs_reg = AFE_MEMIF_AGENT_FS_CON2,
        .fs_shift = 10,
        .fs_maskbit = 0x1f,
        .mono_reg = AFE_UL3_CON0,
        .mono_shift = 16,
        .hd_reg = AFE_UL3_CON0,
        .hd_shift = 5,
        .enable_reg = AFE_DAC_CON0,
        .enable_shift = 3,
        .msb_reg = AFE_NORMAL_BASE_ADR_MSB,
        .msb_shift = 2,
        .msb2_reg = AFE_NORMAL_END_ADR_MSB,
        .msb2_shift = 2,
        .agent_disable_reg = AUDIO_TOP_CON5,
        .agent_disable_shift = 2,
        .ch_config_reg = -1,
        .ch_config_shift = -1,
        .int_odd_reg = AFE_UL3_CON0,
        .int_odd_shift = 0,
        .buffer_bytes_align = 64,
    }, {
        .name = "UL4",
        .id = MT7933_AFE_MEMIF_UL4,
        .reg_ofs_base = AFE_UL4_BASE,
        .reg_ofs_cur = AFE_UL4_CUR,
        .fs_reg = AFE_MEMIF_AGENT_FS_CON2,
        .fs_shift = 15,
        .fs_maskbit = 0x1f,
        .mono_reg = AFE_UL4_CON0,
        .mono_shift = 16,
        .hd_reg = AFE_UL4_CON0,
        .hd_shift = 5,
        .enable_reg = AFE_DAC_CON0,
        .enable_shift = 4,
        .msb_reg = AFE_NORMAL_BASE_ADR_MSB,
        .msb_shift = 3,
        .msb2_reg = AFE_NORMAL_END_ADR_MSB,
        .msb2_shift = 3,
        .agent_disable_reg = AUDIO_TOP_CON5,
        .agent_disable_shift = 3,
        .ch_config_reg = -1,
        .ch_config_shift = -1,
        .int_odd_reg = AFE_UL4_CON0,
        .int_odd_shift = 0,
        .buffer_bytes_align = 64,
    }, {
        .name = "UL8",
        .id = MT7933_AFE_MEMIF_UL8,
        .reg_ofs_base = AFE_UL8_BASE,
        .reg_ofs_cur = AFE_UL8_CUR,
        .fs_reg = AFE_MEMIF_AGENT_FS_CON3,
        .fs_shift = 5,
        .fs_maskbit = 0x1f,
        .mono_reg = AFE_UL8_CON0,
        .mono_shift = 16,
        .hd_reg = AFE_UL8_CON0,
        .hd_shift = 5,
        .enable_reg = AFE_DAC_CON0,
        .enable_shift = 8,
        .msb_reg = AFE_NORMAL_BASE_ADR_MSB,
        .msb_shift = 7,
        .msb2_reg = AFE_NORMAL_END_ADR_MSB,
        .msb2_shift = 7,
        .agent_disable_reg = AUDIO_TOP_CON5,
        .agent_disable_shift = 7,
        .ch_config_reg = -1,
        .ch_config_shift = -1,
        .int_odd_reg = AFE_UL8_CON0,
        .int_odd_shift = 0,
        .buffer_bytes_align = 64,
    }, {
        .name = "UL9",
        .id = MT7933_AFE_MEMIF_UL9,
        .reg_ofs_base = AFE_UL9_BASE,
        .reg_ofs_cur = AFE_UL9_CUR,
        .fs_reg = AFE_MEMIF_AGENT_FS_CON3,
        .fs_shift = 10,
        .fs_maskbit = 0x1f,
        .mono_reg = AFE_UL9_CON0,
        .mono_shift = 16,
        .hd_reg = AFE_UL9_CON0,
        .hd_shift = 5,
        .enable_reg = AFE_DAC_CON0,
        .enable_shift = 9,
        .msb_reg = AFE_NORMAL_BASE_ADR_MSB,
        .msb_shift = 8,
        .msb2_reg = AFE_NORMAL_END_ADR_MSB,
        .msb2_shift = 8,
        .agent_disable_reg = AUDIO_TOP_CON5,
        .agent_disable_shift = 8,
        .ch_config_reg = -1,
        .ch_config_shift = -1,
        .int_odd_reg = AFE_UL9_CON0,
        .int_odd_shift = 0,
        .buffer_bytes_align = 64,
    }, {
        .name = "UL10",
        .id = MT7933_AFE_MEMIF_UL10,
        .reg_ofs_base = AFE_UL10_BASE,
        .reg_ofs_cur = AFE_UL10_CUR,
        .fs_reg = AFE_MEMIF_AGENT_FS_CON3,
        .fs_shift = 15,
        .fs_maskbit = 0x1f,
        .mono_reg = AFE_UL10_CON0,
        .mono_shift = 16,
        .hd_reg = AFE_UL10_CON0,
        .hd_shift = 5,
        .enable_reg = AFE_DAC_CON0,
        .enable_shift = 10,
        .msb_reg = AFE_NORMAL_BASE_ADR_MSB,
        .msb_shift = 10,
        .msb2_reg = AFE_NORMAL_END_ADR_MSB,
        .msb2_shift = 10,
        .agent_disable_reg = AUDIO_TOP_CON5,
        .agent_disable_shift = 9,
        .ch_config_reg = -1,
        .ch_config_shift = -1,
        .int_odd_reg = AFE_UL10_CON0,
        .int_odd_shift = 0,
        .buffer_bytes_align = 64,
    },
};


static const int memif_specified_irqs[MT7933_AFE_MEMIF_NUM] = {
    [MT7933_AFE_MEMIF_DLM] = MT7933_AFE_IRQ10,
    [MT7933_AFE_MEMIF_DL2] = MT7933_AFE_IRQ11,
    [MT7933_AFE_MEMIF_DL3] = MT7933_AFE_IRQ12,
    [MT7933_AFE_MEMIF_UL2] = MT7933_AFE_IRQ15,
    [MT7933_AFE_MEMIF_UL3] = MT7933_AFE_IRQ16,
    [MT7933_AFE_MEMIF_UL4] = MT7933_AFE_IRQ17,
    [MT7933_AFE_MEMIF_UL8] = MT7933_AFE_IRQ19,
    [MT7933_AFE_MEMIF_UL9] = MT7933_AFE_IRQ20,
    [MT7933_AFE_MEMIF_UL10] = MT7933_AFE_IRQ21,
};

int mt7933_afe_is_int_1x_en_low_power(struct mtk_base_afe *afe);

static int mt7933_afe_irq_direction_enable(struct mtk_base_afe *afe, int irq_id, int direction)
{

    struct mtk_base_afe_irq *irq;

    if (irq_id >= MT7933_AFE_IRQ_NUM)
        return -1;

    irq = &afe->irqs[irq_id];

    if (direction == MT7933_AFE_IRQ_DIR_MCU) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_IRQ_MASK,
                                      (1 << irq->irq_data->irq_clr_shift),
                                      0);
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_IRQ_MASK,
                                      (1 << irq->irq_data->irq_clr_shift),
                                      (1 << irq->irq_data->irq_clr_shift));
    } else if (direction == MT7933_AFE_IRQ_DIR_DSP) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_IRQ_MASK,
                                      (1 << irq->irq_data->irq_clr_shift),
                                      (1 << irq->irq_data->irq_clr_shift));
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_IRQ_MASK,
                                      (1 << irq->irq_data->irq_clr_shift),
                                      0);
    } else {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_IRQ_MASK,
                                      (1 << irq->irq_data->irq_clr_shift),
                                      (1 << irq->irq_data->irq_clr_shift));
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_IRQ_MASK,
                                      (1 << irq->irq_data->irq_clr_shift),
                                      (1 << irq->irq_data->irq_clr_shift));
    }
    return 0;
}

static const struct mtk_base_irq_data irq_data[MT7933_AFE_IRQ_NUM] = {
    {
        .id = MT7933_AFE_IRQ10,
        .irq_cnt_reg = ASYS_IRQ1_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ1_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ1_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 0,
        .irq_status_shift = 0,
    }, {
        .id = MT7933_AFE_IRQ11,
        .irq_cnt_reg = ASYS_IRQ2_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ2_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ2_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 1,
        .irq_status_shift = 1,
    }, {
        .id = MT7933_AFE_IRQ12,
        .irq_cnt_reg = ASYS_IRQ3_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ3_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ3_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 2,
        .irq_status_shift = 2,
    }, {
        .id = MT7933_AFE_IRQ13,
        .irq_cnt_reg = ASYS_IRQ4_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ4_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ4_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 3,
        .irq_status_shift = 3,
    }, {
        .id = MT7933_AFE_IRQ14,
        .irq_cnt_reg = ASYS_IRQ5_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ5_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ5_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 4,
        .irq_status_shift = 4,
    }, {
        .id = MT7933_AFE_IRQ15,
        .irq_cnt_reg = ASYS_IRQ6_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ6_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ6_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 5,
        .irq_status_shift = 5,
    }, {
        .id = MT7933_AFE_IRQ16,
        .irq_cnt_reg = ASYS_IRQ7_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ7_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ7_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 6,
        .irq_status_shift = 6,
    }, {
        .id = MT7933_AFE_IRQ17,
        .irq_cnt_reg = ASYS_IRQ8_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ8_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ8_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 7,
        .irq_status_shift = 7,
    }, {
        .id = MT7933_AFE_IRQ18,
        .irq_cnt_reg = ASYS_IRQ9_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ9_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ9_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 8,
        .irq_status_shift = 8,
    }, {
        .id = MT7933_AFE_IRQ19,
        .irq_cnt_reg = ASYS_IRQ10_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ10_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ10_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 9,
        .irq_status_shift = 9,
    }, {
        .id = MT7933_AFE_IRQ20,
        .irq_cnt_reg = ASYS_IRQ11_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ11_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ11_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 10,
        .irq_status_shift = 10,
    }, {
        .id = MT7933_AFE_IRQ21,
        .irq_cnt_reg = ASYS_IRQ12_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ12_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ12_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 11,
        .irq_status_shift = 11,
    }, {
        .id = MT7933_AFE_IRQ22,
        .irq_cnt_reg = ASYS_IRQ13_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ13_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ13_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 12,
        .irq_status_shift = 12,
    }, {
        .id = MT7933_AFE_IRQ23,
        .irq_cnt_reg = ASYS_IRQ14_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ14_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ14_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 13,
        .irq_status_shift = 13,
    }, {
        .id = MT7933_AFE_IRQ24,
        .irq_cnt_reg = ASYS_IRQ15_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ15_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ15_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 14,
        .irq_status_shift = 14,
    }, {
        .id = MT7933_AFE_IRQ25,
        .irq_cnt_reg = ASYS_IRQ16_CON,
        .irq_cnt_shift = 0,
        .irq_cnt_maskbit = 0xffffff,
        .irq_en_reg = ASYS_IRQ16_CON,
        .irq_en_shift = 31,
        .irq_fs_reg = ASYS_IRQ16_CON,
        .irq_fs_shift = 24,
        .irq_fs_maskbit = 0x1f,
        .irq_clr_reg = ASYS_IRQ_CLR,
        .irq_clr_shift = 15,
        .irq_status_shift = 15,
    },
};

//static const int aux_irqs[] = {
//  MT7933_AFE_IRQ2,
//};

static int mt7933_afe_fe_copy(struct snd_pcm_stream *stream, void *buf, unsigned int bytes, struct snd_soc_dai *dai)
{
    //  aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;
    unsigned int dai_id = dai->driver->id;
    struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
    struct snd_pcm_runtime *runtime = stream->runtime;
    unsigned int direction = stream->direction;

    unsigned int cur_app_ptr = runtime->appl_ptr % runtime->buffer_size;
    unsigned int cur_app_byte = cur_app_ptr * runtime->bytes_per_frame;
    void *dest = (void *)(memif->phys_buf_addr + cur_app_byte);

    if ((unsigned int)dest < memif->phys_buf_addr ||
        (unsigned int)dest > (memif->phys_buf_addr + memif->buffer_size) ||
        (unsigned int)(dest + bytes) < memif->phys_buf_addr ||
        (unsigned int)(dest + bytes) > (memif->phys_buf_addr + memif->buffer_size)) {
        aud_msg("dest_addr = %p", dest);
        aud_msg("dest_end = %p", dest + bytes);

    }
    if (direction == MSD_STREAM_PLAYBACK) {
        volatile_memcpy(dest, buf, bytes);
        __asm__ __volatile__("dsb" : : : "memory");
    } else if (direction == MSD_STREAM_CAPTURE) {
        volatile_memcpy(buf, dest, bytes);
        __asm__ __volatile__("dsb" : : : "memory");
    }

    return 0;
}

unsigned long bytes_to_frames(struct snd_pcm_runtime *runtime, unsigned int size)
{
    return size / runtime->bytes_per_frame;
}

unsigned long mt7933_afe_fe_pointer(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;
    unsigned int dai_id = dai->driver->id;
    struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
    const struct mtk_base_memif_data *memif_data = memif->data;
    int reg_ofs_base = memif_data->reg_ofs_base;
    int reg_ofs_cur = memif_data->reg_ofs_cur;
    uint32_t hw_ptr = 0, hw_base = 0;
    unsigned int ret, pcm_ptr_bytes;

    ret = aud_drv_read_reg(AUDIO_REGMAP_AFE_BASE, reg_ofs_cur, &hw_ptr);
    //  aud_msg("hw_ptr = 0x%x", hw_ptr);
    if (ret || hw_ptr == 0) {
        aud_error("hw_ptr err");
        pcm_ptr_bytes = 0;
        goto POINTER_RETURN_FRAMES;
    }

    ret = aud_drv_read_reg(AUDIO_REGMAP_AFE_BASE, reg_ofs_base, &hw_base);
    if (ret || hw_base == 0) {
        aud_error("hw_ptr err");
        pcm_ptr_bytes = 0;
        goto POINTER_RETURN_FRAMES;
    }

    pcm_ptr_bytes = hw_ptr - hw_base;

POINTER_RETURN_FRAMES:
    return bytes_to_frames(stream->runtime, pcm_ptr_bytes);
}

static int mt7933_afe_fe_startup(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;
    int memif_num = dai->driver->id;
    struct mtk_base_afe_memif *memif = &afe->memif[memif_num];

    memif->stream = stream;

    mt7933_afe_enable_main_clk(afe);
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_AUDIO);
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_CM33);

    /* enable agent */
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->agent_disable_reg,
                                  1 << memif->data->agent_disable_shift, 0 << memif->data->agent_disable_shift);


    mt7933_afe_irq_direction_enable(afe,
                                    memif->irq_usage,
                                    MT7933_AFE_IRQ_DIR_MCU);

    aud_dbg("%s", memif->data->name);
    aud_dbg("memif->irq_usage = %d", memif->irq_usage);
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

static int is_ul3_in_direct_mode(struct mtk_base_afe *afe)
{
    uint32_t val = 0;

    aud_drv_read_reg(AUDIO_REGMAP_AFE_BASE, ETDM_COWORK_CON1, &val);

    if (val & ETDM_COWORK_CON1_TDM_IN2_BYPASS_INTERCONN)
        return 1;
    else
        return 0;
}

struct mt7933_afe_rate {
    unsigned int rate;
    unsigned int reg_val;
};

static const struct mt7933_afe_rate mt7933_afe_fs_rates[] = {
    { .rate = 8000, .reg_val = MT7933_FS_8K },
    { .rate = 12000, .reg_val = MT7933_FS_12K },
    { .rate = 16000, .reg_val = MT7933_FS_16K },
    { .rate = 24000, .reg_val = MT7933_FS_24K },
    { .rate = 32000, .reg_val = MT7933_FS_32K },
    { .rate = 48000, .reg_val = MT7933_FS_48K },
    { .rate = 96000, .reg_val = MT7933_FS_96K },
    { .rate = 192000, .reg_val = MT7933_FS_192K },
    { .rate = 384000, .reg_val = MT7933_FS_384K },
    { .rate = 7350, .reg_val = MT7933_FS_7P35K },
    { .rate = 11025, .reg_val = MT7933_FS_11P025K },
    { .rate = 14700, .reg_val = MT7933_FS_14P7K },
    { .rate = 22050, .reg_val = MT7933_FS_22P05K },
    { .rate = 29400, .reg_val = MT7933_FS_29P4K },
    { .rate = 44100, .reg_val = MT7933_FS_44P1K },
    { .rate = 88200, .reg_val = MT7933_FS_88P2K },
    { .rate = 176400, .reg_val = MT7933_FS_176P4K },
    { .rate = 352800, .reg_val = MT7933_FS_352P8K },
};

static int mt7933_afe_fs_timing(unsigned int rate)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(mt7933_afe_fs_rates); i++)
        if (mt7933_afe_fs_rates[i].rate == rate)
            return mt7933_afe_fs_rates[i].reg_val;

    return -EINVAL;
}

static int mt7933_memif_fs(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;
    //  int memif_num = dai->driver->id;
    //  struct mtk_base_afe_memif *memif = &afe->memif[memif_num];

    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    const int dai_id = rtd->cpu_dai->driver->id;
    struct mt7933_fe_dai_data *fe_data = &afe_priv->fe_data[dai_id];
    int fs;
    unsigned int rate = dai->rate;

    if (dai_id == MT7933_AFE_MEMIF_UL8)
        return MT7933_FS_ETDMIN1_NX_EN;

    if (dai_id == MT7933_AFE_MEMIF_UL3 && is_ul3_in_direct_mode(afe))
        return MT7933_FS_ETDMIN2_NX_EN;

    fs = mt7933_afe_fs_timing(rate);

    if (!fe_data->slave_mode)
        return fs;

    // slave mode
    aud_msg("slave mode");
    //  switch (dai_id) {
    //  case MT7933_AFE_MEMIF_UL8:
    //      fs = MT7933_FS_ETDMIN1_NX_EN;
    //      break;
    //  case MT7933_AFE_MEMIF_UL2:
    //  case MT7933_AFE_MEMIF_UL3:
    //  case MT7933_AFE_MEMIF_UL4:
    //  case MT7933_AFE_MEMIF_UL9:
    //  case MT7933_AFE_MEMIF_UL10:
    //      if (mt7933_match_1st_be_cpu_dai(substream, "ETDM1_IN"))
    //          fs = MT7933_FS_ETDMIN1_1X_EN;
    //      else if (mt7933_match_1st_be_cpu_dai(substream, "ETDM2_IN"))
    //          fs = MT7933_FS_ETDMIN2_1X_EN;
    //      break;
    //  case MT7933_AFE_MEMIF_DLM:
    //  case MT7933_AFE_MEMIF_DL2:
    //  case MT7933_AFE_MEMIF_DL3:
    //      if (mt7933_match_1st_be_cpu_dai(substream, "ETDM2_OUT"))
    //          fs = MT7933_FS_ETDMOUT2_1X_EN;
    //      break;
    //  default:
    //      break;
    //  }

    return fs;
}

int mtk_afe_fe_hw_params(struct snd_pcm_stream *stream, struct msd_hw_params *params, struct snd_soc_dai *dai)
{
    struct mtk_base_afe *afe = dai->private_data;
    const int dai_id = dai->driver->id;
    struct mtk_base_afe_memif *memif = &afe->memif[dai_id];

    struct snd_dma_buffer *dma_buf = &stream->runtime->dma_buf;
    unsigned int bytes_per_frame = snd_pcm_format_width(params->format) * params->channels / 8;
    dma_buf->size = params->period_size * params->period_count * bytes_per_frame;
    unsigned int request_bytes = dma_buf->size + dai->driver->constr.buffer_bytes_align;

#ifdef PINMUX_QFN_DEFAULT
    dma_buf->base = malloc(request_bytes);
    if (!dma_buf->base)
        return -ENOMEM;
    unsigned int phy_base = HAL_CACHE_VIRTUAL_TO_PHYSICAL((unsigned int)dma_buf->base);
#elif defined PINMUX_BGA_DEFAULT
    dma_buf->base = aud_memory_malloc(request_bytes);
    if (!dma_buf->base)
        return -ENOMEM;
    unsigned int phy_base = (unsigned int)dma_buf->base;
#endif /* #ifdef PINMUX_QFN_DEFAULT */
    dma_buf->aligned_base = (char *)(((phy_base + request_bytes) & (~(dai->driver->constr.buffer_bytes_align - 1))) - dma_buf->size);

    int msb_at_bit33 = 0;
    int msb2_at_bit33 = 0;
    int fs = 0;

    //  msb_at_bit33 = upper_32_bits((unsigned int)dma_buf->aligned_base) ? 1 : 0;
    memif->phys_buf_addr = lower_32_bits((unsigned int)dma_buf->aligned_base);
    memif->buffer_size = dma_buf->size;
    //  msb2_at_bit33 = upper_32_bits((unsigned int)(dma_buf->aligned_base + memif->buffer_size - 1)) ? 1 : 0;

    aud_msg("phys_buf_addr = 0x%x", memif->phys_buf_addr);
    aud_msg("buffer_size = 0x%x", memif->buffer_size);

    /* start */
    aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, memif->data->reg_ofs_base,
                             memif->phys_buf_addr);
    /* end */
    aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE,
                             memif->data->reg_ofs_base + AFE_BASE_END_OFFSET,
                             memif->phys_buf_addr + memif->buffer_size - 1 +
                             memif->data->buffer_end_shift);

    /* set MSB to 33-bit */
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->msb_reg,
                                  1 << memif->data->msb_shift,
                                  msb_at_bit33 << memif->data->msb_shift);

    /* set buf end MSB to 33-bit */
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->msb2_reg,
                                  1 << memif->data->msb2_shift,
                                  msb2_at_bit33 << memif->data->msb2_shift);

    /* set channel */
    if (memif->data->mono_shift >= 0) {
        unsigned int mono = (dai->channels == 1) ? 1 : 0;

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->mono_reg,
                                      1 << memif->data->mono_shift,
                                      mono << memif->data->mono_shift);
    }

    /* set rate */
    if (memif->data->fs_shift < 0)
        return 0;

    fs = mt7933_memif_fs(stream, dai);

    if (fs < 0)
        return -EINVAL;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->fs_reg,
                                  memif->data->fs_maskbit << memif->data->fs_shift,
                                  fs << memif->data->fs_shift);

    return 0;
}

static int mt7933_afe_fe_hw_params(struct snd_pcm_stream *stream, struct msd_hw_params *params, struct snd_soc_dai *dai)
{
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);

    struct mtk_base_afe *afe = dai->private_data;
    const int dai_id = dai->driver->id;
    struct mtk_base_afe_memif *memif = &afe->memif[dai_id];

    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    const struct mtk_base_memif_data *data = memif->data;
    struct mt7933_control_data *ctrl_data = &afe_priv->ctrl_data;
    unsigned int channels = params->channels;

    aud_dbg("[%s] rate %u ch %u bit %u period %u-%u\n",
            data->name, params->rate, channels,
            snd_pcm_format_width(params->format), params->period_size, params->period_count);

    if (data->ch_config_shift >= 0)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      data->ch_config_reg,
                                      0x1f << data->ch_config_shift,
                                      channels << data->ch_config_shift);

    if (data->int_odd_shift >= 0) {
        unsigned int odd_en = (channels == 1) ? 1 : 0;

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, data->int_odd_reg,
                                      1 << (data->int_odd_shift + 1),
                                      1 << (data->int_odd_shift + 1));
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, data->int_odd_reg,
                                      1 << data->int_odd_shift,
                                      odd_en << data->int_odd_shift);
    }

    if (dai_id == MT7933_AFE_MEMIF_UL2) {
        unsigned int val = 0;

        if (!ctrl_data->bypass_cm1) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON,
                                          ASYS_TCON_O34_O41_1X_EN_MASK,
                                          ASYS_TCON_O34_O41_1X_EN_UL2);

            val |= UL_REORDER_START_DATA(8) |
                   UL_REORDER_CHANNEL(channels) |
                   UL_REORDER_NO_BYPASS;
        }

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_I2S_UL2_REORDER,
                                      UL_REORDER_CTRL_MASK, val);
    } else if (dai_id == MT7933_AFE_MEMIF_UL9) {
        unsigned int val = 0;

        if (!ctrl_data->bypass_cm0) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON,
                                          ASYS_TCON_O26_O33_1X_EN_MASK,
                                          ASYS_TCON_O26_O33_1X_EN_UL9);

            if (channels > 8)
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON,
                                              ASYS_TCON_O34_O41_1X_EN_MASK,
                                              ASYS_TCON_O34_O41_1X_EN_UL9);

            val |= UL_REORDER_START_DATA(0) |
                   UL_REORDER_CHANNEL(channels) |
                   UL_REORDER_NO_BYPASS;
        }

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_I2S_UL9_REORDER,
                                      UL_REORDER_CTRL_MASK, val);
    }

    return mtk_afe_fe_hw_params(stream, params, dai);
}

int mtk_afe_fe_prepare(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;
    const int dai_id = dai->driver->id;
    struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
    int hd_audio = 0;

    /* set hd mode */
    switch (stream->runtime->format) {
        case MSD_PCM_FMT_S16_LE:
            hd_audio = 0;
            break;
        case MSD_PCM_FMT_S32_LE:
            hd_audio = 1;
            break;
        case MSD_PCM_FMT_S24_LE:
            hd_audio = 1;
            break;
        default:
            aud_error("unsupported format %d\n", stream->runtime->format);
            break;
    }

    if (memif->data->hd_reg >= 0)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->hd_reg,
                                      1 << memif->data->hd_shift,
                                      hd_audio << memif->data->hd_shift);

    return 0;
}

static int mt7933_afe_fe_prepare(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    return mtk_afe_fe_prepare(stream, dai);
}

static int mt7933_irq_fs(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    int irq_fs = mt7933_memif_fs(stream, dai);

    if (irq_fs == MT7933_FS_ETDMIN1_NX_EN)
        irq_fs = MT7933_FS_ETDMIN1_1X_EN;
    else if (irq_fs == MT7933_FS_ETDMIN2_NX_EN)
        irq_fs = MT7933_FS_ETDMIN2_1X_EN;

    return irq_fs;
}

int mtk_afe_fe_trigger(struct snd_pcm_stream *stream, int cmd, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;
    const int dai_id = dai->driver->id;
    struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
    struct mtk_base_afe_irq *irqs = &afe->irqs[memif->irq_usage];
    const struct mtk_base_irq_data *irq_data = irqs->irq_data;
    unsigned int counter = stream->runtime->period_size;
    int fs;

    aud_dbg("%s cmd = %d\n", memif->data->name, cmd);

    switch (cmd) {
        case SND_PCM_TRIGGER_START:
            if (memif->data->enable_shift >= 0)
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                              memif->data->enable_reg,
                                              1 << memif->data->enable_shift,
                                              1 << memif->data->enable_shift);

            /* set irq counter */
            if (irq_data->irq_cnt_reg >= 0)
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                              irq_data->irq_cnt_reg,
                                              irq_data->irq_cnt_maskbit << irq_data->irq_cnt_shift,
                                              counter << irq_data->irq_cnt_shift);

            /* set irq fs */
            fs = mt7933_irq_fs(stream, dai);

            if (fs < 0)
                return -EINVAL;

            if (irq_data->irq_fs_reg >= 0)
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                              irq_data->irq_fs_reg,
                                              irq_data->irq_fs_maskbit << irq_data->irq_fs_shift,
                                              fs << irq_data->irq_fs_shift);

            /* enable interrupt */
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, irq_data->irq_en_reg,
                                          1 << irq_data->irq_en_shift,
                                          1 << irq_data->irq_en_shift);

            return 0;
        case SND_PCM_TRIGGER_STOP:
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->enable_reg,
                                          1 << memif->data->enable_shift, 0);
            /* disable interrupt */
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, irq_data->irq_en_reg,
                                          1 << irq_data->irq_en_shift,
                                          0 << irq_data->irq_en_shift);
            /* and clear pending IRQ */
            aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, irq_data->irq_clr_reg,
                                     1 << irq_data->irq_clr_shift);
            return 0;
        default:
            return -EINVAL;
    }
}

static int mt7933_afe_fe_trigger(struct snd_pcm_stream *stream, int cmd, struct snd_soc_dai *dai)
{
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    const int dai_id = dai->driver->id;

    aud_dbg("%d cmd %d\n", dai_id, cmd);

    switch (cmd) {
        case SND_PCM_TRIGGER_START:
            /* enable channel merge */
            if (dai_id == MT7933_AFE_MEMIF_UL2) {
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                              AFE_I2S_UL2_REORDER,
                                              UL_REORDER_EN, UL_REORDER_EN);
            } else if (dai_id == MT7933_AFE_MEMIF_UL9) {
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                              AFE_I2S_UL9_REORDER,
                                              UL_REORDER_EN, UL_REORDER_EN);
            }
            break;
        case SND_PCM_TRIGGER_STOP:
            /* disable channel merge */
            if (dai_id == MT7933_AFE_MEMIF_UL2) {
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                              AFE_I2S_UL2_REORDER,
                                              UL_REORDER_EN, 0x0);
            } else if (dai_id == MT7933_AFE_MEMIF_UL9) {
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                              AFE_I2S_UL9_REORDER,
                                              UL_REORDER_EN, 0x0);
            }
            break;
        default:
            break;
    }

    return mtk_afe_fe_trigger(stream, cmd, dai);
}
static int mt7933_afe_fe_hw_free(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    struct snd_dma_buffer *dma_buf = &stream->runtime->dma_buf;

#ifdef PINMUX_QFN_DEFAULT
    free(dma_buf->base);
#elif defined PINMUX_BGA_DEFAULT
    aud_memory_free(dma_buf->base);
#endif /* #ifdef PINMUX_QFN_DEFAULT */
    dma_buf->base = NULL;
    dma_buf->aligned_base = NULL;
    dma_buf->size = 0;
    return 0;
}

int mtk_dynamic_irq_release(struct mtk_base_afe *afe, int irq_id)
{
    //  mutex_lock(&afe->irq_alloc_lock);
    if (irq_id >= 0 && irq_id < afe->irqs_size) {
        afe->irqs[irq_id].irq_occupyed = 0;
        //      mutex_unlock(&afe->irq_alloc_lock);
        return 0;
    }
    //  mutex_unlock(&afe->irq_alloc_lock);
    return -EINVAL;
}

void mtk_afe_fe_shutdown(struct snd_pcm_stream *stream,
                         struct snd_soc_dai *dai)
{
    struct mtk_base_afe *afe = dai->private_data;
    const int dai_id = dai->driver->id;
    struct mtk_base_afe_memif *memif = &afe->memif[dai_id];
    int irq_id;

    irq_id = memif->irq_usage;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->agent_disable_reg,
                                  1 << memif->data->agent_disable_shift,
                                  1 << memif->data->agent_disable_shift);

    if (!memif->const_irq) {
        mtk_dynamic_irq_release(afe, irq_id);
        memif->irq_usage = -1;
        memif->stream = NULL;
    }
}

static void mt7933_afe_fe_shutdown(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    struct mtk_base_afe *afe = dai->private_data;

    mtk_afe_fe_shutdown(stream, dai);

    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_AUDIO);
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_CM33);
    mt7933_afe_disable_main_clk(afe);
}

struct mt7933_adsp_compr_stream {
    struct snd_compr_caps compr_caps;
};

#if 0
static int mt7933_afe_fe_compr_open(struct snd_compr_stream *cstream, struct snd_soc_dai *dai)
{
    struct mt7933_adsp_compr_stream *stream;
    //  struct snd_soc_pcm_runtime *rtd = cstream->private_data;
    struct snd_compr_runtime *runtime = cstream->runtime;

    stream = calloc(1, sizeof(*stream));
    if (!stream)
        return -ENOMEM;

    stream->compr_caps.min_fragment_size = 128;
    stream->compr_caps.max_fragments = 10;
    runtime->private_data = stream;


    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

int mt7933_afe_fe_compr_free(struct snd_compr_stream *cstream, struct snd_soc_dai *dai)
{
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

int mt7933_afe_fe_compr_set_params(struct snd_compr_stream *cstream, struct snd_compr_params *params, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = cstream->private_data;
    //  struct snd_compr_runtime *runtime = cstream->runtime;
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

int mt7933_afe_fe_compr_get_params(struct snd_compr_stream *cstream, struct snd_codec *params, struct snd_soc_dai *dai)
{
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

int mt7933_afe_fe_compr_set_metadata(struct snd_compr_stream *cstream, struct snd_compr_metadata *metadata, struct snd_soc_dai *dai)
{
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

int mt7933_afe_fe_compr_get_metadata(struct snd_compr_stream *cstream, struct snd_compr_metadata *metadata, struct snd_soc_dai *dai)
{
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

int mt7933_afe_fe_compr_trigger(struct snd_compr_stream *cstream, int cmd, struct snd_soc_dai *dai)
{
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

int mt7933_afe_fe_compr_pointer(struct snd_compr_stream *cstream, struct snd_compr_tstamp *tstamp, struct snd_soc_dai *dai)
{
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

int mt7933_afe_fe_compr_copy(struct snd_compr_stream *cstream, char *buf, unsigned int count, struct snd_soc_dai *dai)
{
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return count;
}

int mt7933_afe_fe_compr_ack(struct snd_compr_stream *cstream, unsigned int count, struct snd_soc_dai *dai)
{
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

int mt7933_afe_fe_compr_get_caps(struct snd_compr_stream *cstream, struct snd_compr_caps *caps, struct snd_soc_dai *dai)
{
    struct mt7933_adsp_compr_stream *stream = cstream->runtime->private_data;
    memcpy(caps, &stream->compr_caps, sizeof(struct snd_compr_caps));
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

int mt7933_afe_fe_compr_get_codec_caps(struct snd_compr_stream *cstream, struct snd_compr_codec_caps *codec, struct snd_soc_dai *dai)
{
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}
#endif /* #if 0 */

static struct snd_soc_pcm_runtime *find_soc_pcm_be_runtime(struct snd_soc_pcm_runtime *fe_rtd)
{
    struct snd_soc_dpcm *dpcm;
    struct list_head *be_list;
    struct snd_soc_pcm_runtime *be_rtd = NULL;

    list_for_each(be_list, &fe_rtd->dpcm.be_clients) {
        dpcm = list_entry(be_list, struct snd_soc_dpcm, list);
        // find first be;
        be_rtd = dpcm->be;
        break;
    }
    return be_rtd;
}

#define PCM_STREAM_STR(x) \
    (((x) == MSD_STREAM_CAPTURE) ? "capture" : "playback")

static int mt7933_dai_num_to_gasrc(int num)
{
    int val = num - MT7933_AFE_IO_GASRC0;

    if (val < 0 || val >= MT7933_GASRC_NUM)
        return -EINVAL;

    return val;
}

static const struct mt7933_gasrc_ctrl_reg gasrc_ctrl_reg[MT7933_GASRC_NUM] = {
    [MT7933_GASRC0] = {
        .con0 = AFE_GASRC0_NEW_CON0,
        .con1 = AFE_GASRC0_NEW_CON1,
        .con2 = AFE_GASRC0_NEW_CON2,
        .con3 = AFE_GASRC0_NEW_CON3,
        .con4 = AFE_GASRC0_NEW_CON4,
        .con6 = AFE_GASRC0_NEW_CON6,
        .con7 = AFE_GASRC0_NEW_CON7,
        .con10 = AFE_GASRC0_NEW_CON10,
        .con11 = AFE_GASRC0_NEW_CON11,
        .con13 = AFE_GASRC0_NEW_CON13,
        .con14 = AFE_GASRC0_NEW_CON14,
    },
};

static void mt7933_afe_gasrc_enable_iir(struct mtk_base_afe *afe,
                                        int gasrc_id)
{
    unsigned int ctrl_reg = 0;

    if (gasrc_id < MT7933_GASRC0 || gasrc_id >= MT7933_GASRC_NUM) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return;
    }

    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_CHSET0_IIR_STAGE_MASK,
                                  GASRC_NEW_CON0_CHSET0_IIR_STAGE(8));

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_CHSET0_CLR_IIR_HISTORY,
                                  GASRC_NEW_CON0_CHSET0_CLR_IIR_HISTORY);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_CHSET0_IIR_EN,
                                  GASRC_NEW_CON0_CHSET0_IIR_EN);
}

static void mt7933_afe_gasrc_disable_iir(struct mtk_base_afe *afe,
                                         int gasrc_id)
{
    unsigned int ctrl_reg = 0;

    if (gasrc_id < MT7933_GASRC0 || gasrc_id >= MT7933_GASRC_NUM) {
        aud_error("can not find gasrc_id: %d", gasrc_id);
        return;
    }

    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_CHSET0_IIR_EN, 0);
}

static int mt7933_afe_enable_gasrc(struct snd_soc_dai *dai, int stream)
{
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_gasrc_data *gasrc_data;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);
    spin_lock_flags_define(flags);
    unsigned int ctrl_reg = 0;
    unsigned int val = 0;
    int ret = 0, counter;
    bool re_enable = false;

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return -EINVAL;
    }

    gasrc_data = &afe_priv->gasrc_data[gasrc_id];

    if (gasrc_data->re_enable[stream]) {
        re_enable = true;
        gasrc_data->re_enable[stream] = false;
    }

    spin_lock_irqsave(&gasrc_data->gasrc_lock, flags);
    gasrc_data->ref_cnt++;
    counter = gasrc_data->ref_cnt;
    spin_unlock_irqrestore(&gasrc_data->gasrc_lock, flags);

    if (counter != 1 && !re_enable)
        return 0;

    aud_msg("[%d] one_heart %d re_enable %d", gasrc_id, gasrc_data->one_heart, re_enable);

    if (gasrc_data->cali_tx || gasrc_data->cali_rx) {
        if (gasrc_data->one_heart)
            ctrl_reg = gasrc_ctrl_reg[MT7933_GASRC0].con6;
        else
            ctrl_reg = gasrc_ctrl_reg[gasrc_id].con6;

        val = GASRC_NEW_CON6_CALI_EN;
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, val, val);

        val = GASRC_NEW_CON6_AUTO_TUNE_FREQ2 |
              GASRC_NEW_CON6_AUTO_TUNE_FREQ3;
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, val, val);
    }

    if (gasrc_data->one_heart)
        ctrl_reg = gasrc_ctrl_reg[MT7933_GASRC0].con0;
    else
        ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

    val = GASRC_NEW_CON0_ASM_ON;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, val, val);

    return ret;
}

static int mt7933_afe_disable_gasrc(struct snd_soc_dai *dai, bool directly)
{
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_gasrc_data *gasrc_data;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);
    unsigned int ctrl_reg = 0;
    unsigned int val = 0;
    int ret = 0, counter;
    spin_lock_flags_define(flags);

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return -EINVAL;
    }

    gasrc_data = &afe_priv->gasrc_data[gasrc_id];

    if (!directly) {
        spin_lock_irqsave(&gasrc_data->gasrc_lock, flags);
        gasrc_data->ref_cnt--;
        counter = gasrc_data->ref_cnt;
        if (counter < 0) {
            gasrc_data->ref_cnt++;
        }
        spin_unlock_irqrestore(&gasrc_data->gasrc_lock, flags);
        if (counter != 0) {
            return 0;
        }
    }

    aud_msg("[%d] one_heart %d directly %d", gasrc_id, gasrc_data->one_heart, directly);

    if (gasrc_data->one_heart)
        ctrl_reg = gasrc_ctrl_reg[MT7933_GASRC0].con0;
    else
        ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

    val = GASRC_NEW_CON0_ASM_ON;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, val, 0);

    if (gasrc_data->cali_tx || gasrc_data->cali_rx) {
        if (gasrc_data->one_heart)
            ctrl_reg = gasrc_ctrl_reg[MT7933_GASRC0].con6;
        else
            ctrl_reg = gasrc_ctrl_reg[gasrc_id].con6;

        val = GASRC_NEW_CON6_CALI_EN;
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, val, 0);
    }

    if (gasrc_data->iir_on)
        mt7933_afe_gasrc_disable_iir(afe, gasrc_id);

    return ret;
}

static void mt7933_afe_reset_gasrc(int gasrc_id)
{
    unsigned int val = 0;

    switch (gasrc_id) {
        case MT7933_GASRC0:
            val = GASRC_CFG0_GASRC0_SOFT_RST;
            break;
        default:
            return;
    }

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, GASRC_CFG0, val, val);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, GASRC_CFG0, val, 0);

}

static void mt7933_afe_clear_gasrc(int gasrc_id)
{
    unsigned int ctrl_reg = 0;

    if (gasrc_id < MT7933_GASRC0 || gasrc_id >= MT7933_GASRC_NUM) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return;
    }

    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_CHSET_STR_CLR, GASRC_NEW_CON0_CHSET_STR_CLR);
}

static void mt7933_afe_gasrc_use_sel(int gasrc_id, bool no_bypass)
{
    unsigned int mask = 0;
    unsigned int val = 0;

    switch (gasrc_id) {
        case MT7933_GASRC0:
            mask = GASRC_CFG0_GASRC0_USE_SEL_MASK;
            val = GASRC_CFG0_GASRC0_USE_SEL(no_bypass);
            break;
        default:
            aud_error("can not find gasrc_id: %d", gasrc_id);
            break;
    }

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, GASRC_CFG0, mask, val);
}

struct mt7933_afe_gasrc_freq {
    unsigned int rate;
    unsigned int freq_val;
};

static const
struct mt7933_afe_gasrc_freq
    mt7933_afe_gasrc_freq_palette_49m_45m_64_cycles[] = {
    { .rate = 8000, .freq_val = 0x050000 },
    { .rate = 12000, .freq_val = 0x078000 },
    { .rate = 16000, .freq_val = 0x0A0000 },
    { .rate = 24000, .freq_val = 0x0F0000 },
    { .rate = 32000, .freq_val = 0x140000 },
    { .rate = 48000, .freq_val = 0x1E0000 },
    { .rate = 96000, .freq_val = 0x3C0000 },
    { .rate = 192000, .freq_val = 0x780000 },
    { .rate = 384000, .freq_val = 0xF00000 },
    { .rate = 7350, .freq_val = 0x049800 },
    { .rate = 11025, .freq_val = 0x06E400 },
    { .rate = 14700, .freq_val = 0x093000 },
    { .rate = 22050, .freq_val = 0x0DC800 },
    { .rate = 29400, .freq_val = 0x126000 },
    { .rate = 44100, .freq_val = 0x1B9000 },
    { .rate = 88200, .freq_val = 0x372000 },
    { .rate = 176400, .freq_val = 0x6E4000 },
    { .rate = 352800, .freq_val = 0xDC8000 },
};

static const
struct mt7933_afe_gasrc_freq
    mt7933_afe_gasrc_period_palette_49m_64_cycles[] = {
    { .rate = 8000, .freq_val = 0x060000 },
    { .rate = 12000, .freq_val = 0x040000 },
    { .rate = 16000, .freq_val = 0x030000 },
    { .rate = 24000, .freq_val = 0x020000 },
    { .rate = 32000, .freq_val = 0x018000 },
    { .rate = 48000, .freq_val = 0x010000 },
    { .rate = 96000, .freq_val = 0x008000 },
    { .rate = 192000, .freq_val = 0x004000 },
    { .rate = 384000, .freq_val = 0x002000 },
    { .rate = 7350, .freq_val = 0x0687D8 },
    { .rate = 11025, .freq_val = 0x045A90 },
    { .rate = 14700, .freq_val = 0x0343EC },
    { .rate = 22050, .freq_val = 0x022D48 },
    { .rate = 29400, .freq_val = 0x01A1F6 },
    { .rate = 44100, .freq_val = 0x0116A4 },
    { .rate = 88200, .freq_val = 0x008B52 },
    { .rate = 176400, .freq_val = 0x0045A9 },
    { .rate = 352800, .freq_val = 0x0022D4 },
};

static const
struct mt7933_afe_gasrc_freq
    mt7933_afe_gasrc_period_palette_45m_64_cycles[] = {
    { .rate = 8000, .freq_val = 0x058332 },
    { .rate = 12000, .freq_val = 0x03ACCC },
    { .rate = 16000, .freq_val = 0x02C199 },
    { .rate = 24000, .freq_val = 0x01D666 },
    { .rate = 32000, .freq_val = 0x0160CC },
    { .rate = 48000, .freq_val = 0x00EB33 },
    { .rate = 96000, .freq_val = 0x007599 },
    { .rate = 192000, .freq_val = 0x003ACD },
    { .rate = 384000, .freq_val = 0x001D66 },
    { .rate = 7350, .freq_val = 0x060000 },
    { .rate = 11025, .freq_val = 0x040000 },
    { .rate = 14700, .freq_val = 0x030000 },
    { .rate = 22050, .freq_val = 0x020000 },
    { .rate = 29400, .freq_val = 0x018000 },
    { .rate = 44100, .freq_val = 0x010000 },
    { .rate = 88200, .freq_val = 0x008000 },
    { .rate = 176400, .freq_val = 0x004000 },
    { .rate = 352800, .freq_val = 0x002000 },
};

/* INT_ADDA ADC (RX Tracking of 26m) */
static const
struct mt7933_afe_gasrc_freq
    mt7933_afe_gasrc_freq_palette_49m_45m_48_cycles[] = {
    { .rate = 8000, .freq_val = 0x06AAAA },
    { .rate = 12000, .freq_val = 0x0A0000 },
    { .rate = 16000, .freq_val = 0x0D5555 },
    { .rate = 24000, .freq_val = 0x140000 },
    { .rate = 32000, .freq_val = 0x1AAAAA },
    { .rate = 48000, .freq_val = 0x280000 },
    { .rate = 96000, .freq_val = 0x500000 },
    { .rate = 192000, .freq_val = 0xA00000 },
    { .rate = 384000, .freq_val = 0x1400000 },
    { .rate = 11025, .freq_val = 0x093000 },
    { .rate = 22050, .freq_val = 0x126000 },
    { .rate = 44100, .freq_val = 0x24C000 },
    { .rate = 88200, .freq_val = 0x498000 },
    { .rate = 176400, .freq_val = 0x930000 },
    { .rate = 352800, .freq_val = 0x1260000 },
};

/* INT_ADDA DAC (TX Tracking of 26m) */
static const
struct mt7933_afe_gasrc_freq
    mt7933_afe_gasrc_period_palette_49m_48_cycles[] = {
    { .rate = 8000, .freq_val = 0x048000 },
    { .rate = 12000, .freq_val = 0x030000 },
    { .rate = 16000, .freq_val = 0x024000 },
    { .rate = 24000, .freq_val = 0x018000 },
    { .rate = 32000, .freq_val = 0x012000 },
    { .rate = 48000, .freq_val = 0x00C000 },
    { .rate = 96000, .freq_val = 0x006000 },
    { .rate = 192000, .freq_val = 0x003000 },
    { .rate = 384000, .freq_val = 0x001800 },
    { .rate = 11025, .freq_val = 0x0343EB },
    { .rate = 22050, .freq_val = 0x01A1F6 },
    { .rate = 44100, .freq_val = 0x00D0FB },
    { .rate = 88200, .freq_val = 0x00687D },
    { .rate = 176400, .freq_val = 0x00343F },
    { .rate = 352800, .freq_val = 0x001A1F },
};

/* INT_ADDA DAC (TX Tracking of 26m) */
static const
struct mt7933_afe_gasrc_freq
    mt7933_afe_gasrc_period_palette_45m_441_cycles[] = {
    { .rate = 8000, .freq_val = 0x25FC0D },
    { .rate = 12000, .freq_val = 0x1952B3 },
    { .rate = 16000, .freq_val = 0x12FE06 },
    { .rate = 24000, .freq_val = 0x0CA95A },
    { .rate = 32000, .freq_val = 0x097F03 },
    { .rate = 48000, .freq_val = 0x0654AD },
    { .rate = 96000, .freq_val = 0x032A56 },
    { .rate = 192000, .freq_val = 0x01952B },
    { .rate = 384000, .freq_val = 0x00CA96 },
    { .rate = 11025, .freq_val = 0x1B9000 },
    { .rate = 22050, .freq_val = 0x0DC800 },
    { .rate = 44100, .freq_val = 0x06E400 },
    { .rate = 88200, .freq_val = 0x037200 },
    { .rate = 176400, .freq_val = 0x01B900 },
    { .rate = 352800, .freq_val = 0x00DC80 },
};

static int mt7933_afe_gasrc_get_freq_val(unsigned int rate,
                                         unsigned int cali_cycles)
{
    int i;
    const struct mt7933_afe_gasrc_freq *freq_palette = NULL;
    int tbl_size = 0;

    if (cali_cycles == 48) {
        freq_palette =
            mt7933_afe_gasrc_freq_palette_49m_45m_48_cycles;
        tbl_size = ARRAY_SIZE(
                       mt7933_afe_gasrc_freq_palette_49m_45m_48_cycles);
    } else {
        freq_palette =
            mt7933_afe_gasrc_freq_palette_49m_45m_64_cycles;
        tbl_size = ARRAY_SIZE(
                       mt7933_afe_gasrc_freq_palette_49m_45m_64_cycles);
    }

    //  if (freq_palette == NULL)
    //      return -EINVAL;

    for (i = 0; i < tbl_size; i++)
        if (freq_palette[i].rate == rate)
            return freq_palette[i].freq_val;

    return -EINVAL;
}

static int mt7933_afe_gasrc_get_period_val(unsigned int rate,
                                           bool op_freq_45m, unsigned int cali_cycles)
{
    int i;
    const struct mt7933_afe_gasrc_freq *period_palette = NULL;
    int tbl_size = 0;

    if (cali_cycles == 48) {
        period_palette =
            mt7933_afe_gasrc_period_palette_49m_48_cycles;
        tbl_size = ARRAY_SIZE(
                       mt7933_afe_gasrc_period_palette_49m_48_cycles);
    } else if (cali_cycles == 441) {
        period_palette =
            mt7933_afe_gasrc_period_palette_45m_441_cycles;
        tbl_size = ARRAY_SIZE(
                       mt7933_afe_gasrc_period_palette_45m_441_cycles);
    } else {
        if (op_freq_45m) {
            period_palette =
                mt7933_afe_gasrc_period_palette_45m_64_cycles;
            tbl_size = ARRAY_SIZE(
                           mt7933_afe_gasrc_period_palette_45m_64_cycles);
        } else {
            period_palette =
                mt7933_afe_gasrc_period_palette_49m_64_cycles;
            tbl_size = ARRAY_SIZE(
                           mt7933_afe_gasrc_period_palette_49m_64_cycles);
        }
    }

    //  if (period_palette == NULL)
    //      return -EINVAL;

    for (i = 0; i < tbl_size; i++) {
        if (period_palette[i].rate == rate)
            return period_palette[i].freq_val;
    }

    return -EINVAL;
}


static void mt7933_afe_gasrc_set_tx_mode_fs(struct mtk_base_afe *afe,
                                            struct snd_soc_dai *dai, int input_rate, int output_rate)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);
    bool gasrc_op_freq_45m;
    unsigned int cali_cycles;
    unsigned int ctrl_reg = 0;
    int val = 0;

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return;
    }

    gasrc_op_freq_45m = afe_priv->gasrc_data[gasrc_id].op_freq_45m;
    cali_cycles = afe_priv->gasrc_data[gasrc_id].cali_cycles;

    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_CHSET0_OFS_SEL_MASK,
                                  GASRC_NEW_CON0_CHSET0_OFS_SEL_TX);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_CHSET0_IFS_SEL_MASK,
                                  GASRC_NEW_CON0_CHSET0_IFS_SEL_TX);

    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con4;
    val = mt7933_afe_gasrc_get_period_val(output_rate,
                                          gasrc_op_freq_45m, cali_cycles);
    if (val > 0)
        aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, val);

    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con1;
    val = mt7933_afe_gasrc_get_period_val(input_rate,
                                          gasrc_op_freq_45m, cali_cycles);
    if (val > 0)
        aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, val);
}


static void mt7933_afe_gasrc_set_rx_mode_fs(struct mtk_base_afe *afe,
                                            struct snd_soc_dai *dai, int input_rate, int output_rate)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return;
    }

    unsigned int cali_cycles = afe_priv->gasrc_data[gasrc_id].cali_cycles;
    unsigned int ctrl_reg = 0;
    int val = 0;

    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_CHSET0_OFS_SEL_MASK,
                                  GASRC_NEW_CON0_CHSET0_OFS_SEL_RX);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_CHSET0_IFS_SEL_MASK,
                                  GASRC_NEW_CON0_CHSET0_IFS_SEL_RX);

    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con2;
    val = mt7933_afe_gasrc_get_freq_val(output_rate, cali_cycles);
    if (val > 0)
        aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, val);

    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con3;
    val = mt7933_afe_gasrc_get_freq_val(input_rate, cali_cycles);
    if (val > 0)
        aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, val);
}

static void mt7933_afe_gasrc_sel_cali_clk(
    struct mtk_base_afe *afe,
    struct snd_soc_dai *dai, bool gasrc_op_freq_45m)
{
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);
    unsigned int mask = 0;
    unsigned int val = 0;

    val = gasrc_op_freq_45m;

    switch (gasrc_id) {
        case MT7933_GASRC0:
            mask = PWR1_ASM_CON1_GASRC0_CALI_CK_SEL_MASK;
            val = PWR1_ASM_CON1_GASRC0_CALI_CK_SEL(val);
            break;
        default:
            break;
    }

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, PWR1_ASM_CON1, mask, val);
}

struct mt7933_afe_gasrc_lrck_sel {
    int fs_timing;
    unsigned int lrck_sel_val;
};

static const
struct mt7933_afe_gasrc_lrck_sel mt7933_afe_gasrc_lrck_sels[] = {
    {
        .fs_timing = MT7933_FS_ETDMIN2_1X_EN,
        .lrck_sel_val = MT7933_AFE_GASRC_LRCK_SEL_ETDM_IN2,
    },
    {
        .fs_timing = MT7933_FS_ETDMIN1_1X_EN,
        .lrck_sel_val = MT7933_AFE_GASRC_LRCK_SEL_ETDM_IN1,
    },
    {
        .fs_timing = MT7933_FS_ETDMOUT2_1X_EN,
        .lrck_sel_val = MT7933_AFE_GASRC_LRCK_SEL_ETDM_OUT2,
    },
    {
        .fs_timing = MT7933_FS_AMIC_1X_EN_ASYNC,
        .lrck_sel_val = MT7933_AFE_GASRC_LRCK_SEL_UL_VIRTUAL,
    },
};

static int mt7933_afe_gasrc_get_lrck_sel_val(int fs_timing, unsigned int *val)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(mt7933_afe_gasrc_lrck_sels); i++) {
        if (mt7933_afe_gasrc_lrck_sels[i].fs_timing == fs_timing) {
            *val = mt7933_afe_gasrc_lrck_sels[i].lrck_sel_val;
            return 0;
        }
    }
    return -EINVAL;
}

static void mt7933_afe_gasrc_sel_lrck(
    struct mtk_base_afe *afe,
    struct snd_soc_dai *dai, int fs_timing)
{
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);
    unsigned int mask = 0;
    unsigned int val = 0;
    int ret;

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return;
    }

    ret = mt7933_afe_gasrc_get_lrck_sel_val(fs_timing, &val);
    if (ret < 0)
        return;

    switch (gasrc_id) {
        case MT7933_GASRC0:
            mask = GASRC_CFG0_GASRC0_LRCK_SEL_MASK;
            val = GASRC_CFG0_GASRC0_LRCK_SEL(val);
            break;
        default:
            break;
    }

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, GASRC_CFG0, mask, val);
}

static bool mt7933_afe_gasrc_is_tx_tracking(int fs_timing)
{
    if (fs_timing == MT7933_FS_ETDMOUT2_1X_EN)
        return true;
    else
        return false;
}

static bool mt7933_afe_gasrc_is_rx_tracking(int fs_timing)
{
    if ((fs_timing == MT7933_FS_ETDMIN1_1X_EN) ||
        (fs_timing == MT7933_FS_ETDMIN2_1X_EN) ||
        (fs_timing == MT7933_FS_AMIC_1X_EN_ASYNC))
        return true;
    else
        return false;
}

static void mt7933_afe_gasrc_set_input_fs(int gasrc_id, int fs_timing)
{
    unsigned int val = 0;
    unsigned int mask = 0;

    switch (gasrc_id) {
        case MT7933_GASRC0:
            mask = GASRC_TIMING_CON0_GASRC0_IN_MODE_MASK;
            val = GASRC_TIMING_CON0_GASRC0_IN_MODE(fs_timing);
            break;
        default:
            aud_error("can not find gasrc_id: %d", gasrc_id);
            return;
    }

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, GASRC_TIMING_CON0, mask, val);
}

static void mt7933_afe_gasrc_set_output_fs(int gasrc_id, int fs_timing)
{
    unsigned int val = 0;
    unsigned int mask = 0;

    switch (gasrc_id) {
        case MT7933_GASRC0:
            mask = GASRC_TIMING_CON1_GASRC0_OUT_MODE_MASK;
            val = GASRC_TIMING_CON1_GASRC0_OUT_MODE(fs_timing);
            break;
        default:
            aud_error("can not find gasrc_id: %d", gasrc_id);
            return;
    }

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, GASRC_TIMING_CON1, mask, val);
}

static void mt7933_afe_adjust_gasrc_cali_cycles(struct mtk_base_afe *afe,
                                                struct snd_soc_dai *dai, int fs_timing, unsigned int rate)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);
    unsigned int *cali_cycles;

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return;
    }

    cali_cycles = &(afe_priv->gasrc_data[gasrc_id].cali_cycles);

    if (fs_timing == MT7933_FS_AMIC_1X_EN_ASYNC) {
        switch (rate) {
            case 8000:
            /* FALLTHROUGH */
            case 16000:
            /* FALLTHROUGH */
            case 32000:
                *cali_cycles = 64;
                break;
            case 48000:
                *cali_cycles = 48;
                break;
            default:
                *cali_cycles = 64;
                break;
        }
    }

    aud_dbg("[%d] cali_cycles %u", gasrc_id, *cali_cycles);
}

enum afe_gasrc_iir_coeff_table_id {
    MT7933_AFE_GASRC_IIR_COEFF_384_to_352 = 0,
    MT7933_AFE_GASRC_IIR_COEFF_256_to_192,
    MT7933_AFE_GASRC_IIR_COEFF_352_to_256,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_256,
    MT7933_AFE_GASRC_IIR_COEFF_352_to_192,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_176,
    MT7933_AFE_GASRC_IIR_COEFF_256_to_96,
    MT7933_AFE_GASRC_IIR_COEFF_352_to_128,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_128,
    MT7933_AFE_GASRC_IIR_COEFF_352_to_96,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_88,
    MT7933_AFE_GASRC_IIR_COEFF_256_to_48,
    MT7933_AFE_GASRC_IIR_COEFF_352_to_64,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_64,
    MT7933_AFE_GASRC_IIR_COEFF_352_to_48,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_48,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_44,
    MT7933_AFE_GASRC_IIR_COEFF_352_to_32,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_32,
    MT7933_AFE_GASRC_IIR_COEFF_352_to_24,
    MT7933_AFE_GASRC_IIR_COEFF_384_to_24,
    MT7933_AFE_GASRC_IIR_TABLES,
};

struct mt7933_afe_gasrc_iir_coeff_table_id {
    int input_rate;
    int output_rate;
    int table_id;
};

static const struct mt7933_afe_gasrc_iir_coeff_table_id
    mt7933_afe_gasrc_iir_coeff_table_ids[] = {
    {
        .input_rate = 8000,
        .output_rate = 7350,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_352,
    },
    {
        .input_rate = 12000,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_256,
    },
    {
        .input_rate = 12000,
        .output_rate = 11025,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_352,
    },
    {
        .input_rate = 16000,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 16000,
        .output_rate = 12000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_256_to_192,
    },
    {
        .input_rate = 16000,
        .output_rate = 7350,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_176,
    },
    {
        .input_rate = 16000,
        .output_rate = 14700,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_352,
    },
    {
        .input_rate = 24000,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_128,
    },
    {
        .input_rate = 24000,
        .output_rate = 12000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 24000,
        .output_rate = 16000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_256,
    },
    {
        .input_rate = 24000,
        .output_rate = 11025,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_176,
    },
    {
        .input_rate = 24000,
        .output_rate = 22050,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_352,
    },
    {
        .input_rate = 32000,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    },
    {
        .input_rate = 32000,
        .output_rate = 12000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_256_to_96,
    },
    {
        .input_rate = 32000,
        .output_rate = 16000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 32000,
        .output_rate = 24000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_256_to_192,
    },
    {
        .input_rate = 32000,
        .output_rate = 7350,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_88,
    },
    {
        .input_rate = 32000,
        .output_rate = 14700,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_176,
    },
    {
        .input_rate = 32000,
        .output_rate = 29400,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_352,
    },
    {
        .input_rate = 48000,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_64,
    },
    {
        .input_rate = 48000,
        .output_rate = 12000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    },
    {
        .input_rate = 48000,
        .output_rate = 16000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_128,
    },
    {
        .input_rate = 48000,
        .output_rate = 24000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 48000,
        .output_rate = 32000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_256,
    },
    {
        .input_rate = 48000,
        .output_rate = 11025,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_88,
    },
    {
        .input_rate = 48000,
        .output_rate = 22050,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_176,
    },
    {
        .input_rate = 48000,
        .output_rate = 44100,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_352,
    },
    {
        .input_rate = 96000,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_32,
    },
    {
        .input_rate = 96000,
        .output_rate = 12000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_48,
    },
    {
        .input_rate = 96000,
        .output_rate = 16000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_64,
    },
    {
        .input_rate = 96000,
        .output_rate = 24000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    },
    {
        .input_rate = 96000,
        .output_rate = 32000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_128,
    },
    {
        .input_rate = 96000,
        .output_rate = 48000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 96000,
        .output_rate = 11025,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_44,
    },
    {
        .input_rate = 96000,
        .output_rate = 22050,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_88,
    },
    {
        .input_rate = 96000,
        .output_rate = 44100,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_176,
    },
    {
        .input_rate = 96000,
        .output_rate = 88200,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_352,
    },
    {
        .input_rate = 192000,
        .output_rate = 12000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_24,
    },
    {
        .input_rate = 192000,
        .output_rate = 16000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_32,
    },
    {
        .input_rate = 192000,
        .output_rate = 24000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_48,
    },
    {
        .input_rate = 192000,
        .output_rate = 32000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_64,
    },
    {
        .input_rate = 192000,
        .output_rate = 48000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    },
    {
        .input_rate = 192000,
        .output_rate = 96000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 192000,
        .output_rate = 22050,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_44,
    },
    {
        .input_rate = 192000,
        .output_rate = 44100,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_88,
    },
    {
        .input_rate = 192000,
        .output_rate = 88200,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_176,
    },
    {
        .input_rate = 192000,
        .output_rate = 176400,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_352,
    },
    {
        .input_rate = 384000,
        .output_rate = 24000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_24,
    },
    {
        .input_rate = 384000,
        .output_rate = 32000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_32,
    },
    {
        .input_rate = 384000,
        .output_rate = 48000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_48,
    },
    {
        .input_rate = 384000,
        .output_rate = 96000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    },
    {
        .input_rate = 384000,
        .output_rate = 192000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 384000,
        .output_rate = 44100,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_44,
    },
    {
        .input_rate = 384000,
        .output_rate = 88200,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_88,
    },
    {
        .input_rate = 384000,
        .output_rate = 176400,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_176,
    },
    {
        .input_rate = 384000,
        .output_rate = 352800,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_352,
    },
    {
        .input_rate = 11025,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_256,
    },
    {
        .input_rate = 11025,
        .output_rate = 7350,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_256,
    },
    {
        .input_rate = 14700,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_192,
    },
    {
        .input_rate = 14700,
        .output_rate = 7350,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 14700,
        .output_rate = 11025,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_256_to_192,
    },
    {
        .input_rate = 22050,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_128,
    },
    {
        .input_rate = 22050,
        .output_rate = 12000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_192,
    },
    {
        .input_rate = 22050,
        .output_rate = 16000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_256,
    },
    {
        .input_rate = 22050,
        .output_rate = 7350,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_128,
    },
    {
        .input_rate = 22050,
        .output_rate = 11025,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 22050,
        .output_rate = 14700,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_256,
    },
    {
        .input_rate = 29400,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_96,
    },
    {
        .input_rate = 29400,
        .output_rate = 16000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_192,
    },
    {
        .input_rate = 29400,
        .output_rate = 7350,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    },
    {
        .input_rate = 29400,
        .output_rate = 11025,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_256_to_96,
    },
    {
        .input_rate = 29400,
        .output_rate = 14700,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 29400,
        .output_rate = 22050,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_256_to_192,
    },
    {
        .input_rate = 44100,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_64,
    },
    {
        .input_rate = 44100,
        .output_rate = 12000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_96,
    },
    {
        .input_rate = 44100,
        .output_rate = 16000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_128,
    },
    {
        .input_rate = 44100,
        .output_rate = 24000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_192,
    },
    {
        .input_rate = 44100,
        .output_rate = 32000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_256,
    },
    {
        .input_rate = 44100,
        .output_rate = 7350,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_64,
    },
    {
        .input_rate = 44100,
        .output_rate = 11025,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    },
    {
        .input_rate = 44100,
        .output_rate = 14700,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_128,
    },
    {
        .input_rate = 44100,
        .output_rate = 22050,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 44100,
        .output_rate = 29400,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_256,
    },
    {
        .input_rate = 88200,
        .output_rate = 8000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_32,
    },
    {
        .input_rate = 88200,
        .output_rate = 12000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_48,
    },
    {
        .input_rate = 88200,
        .output_rate = 16000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_64,
    },
    {
        .input_rate = 88200,
        .output_rate = 24000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_96,
    },
    {
        .input_rate = 88200,
        .output_rate = 32000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_128,
    },
    {
        .input_rate = 88200,
        .output_rate = 48000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_192,
    },
    {
        .input_rate = 88200,
        .output_rate = 7350,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_32,
    },
    {
        .input_rate = 88200,
        .output_rate = 11025,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_48,
    },
    {
        .input_rate = 88200,
        .output_rate = 14700,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_64,
    },
    {
        .input_rate = 88200,
        .output_rate = 22050,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    },
    {
        .input_rate = 88200,
        .output_rate = 29400,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_128,
    },
    {
        .input_rate = 88200,
        .output_rate = 44100,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 176400,
        .output_rate = 12000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_24,
    },
    {
        .input_rate = 176400,
        .output_rate = 16000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_32,
    },
    {
        .input_rate = 176400,
        .output_rate = 24000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_48,
    },
    {
        .input_rate = 176400,
        .output_rate = 32000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_64,
    },
    {
        .input_rate = 176400,
        .output_rate = 48000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_96,
    },
    {
        .input_rate = 176400,
        .output_rate = 96000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_192,
    },
    {
        .input_rate = 176400,
        .output_rate = 11025,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_24,
    },
    {
        .input_rate = 176400,
        .output_rate = 14700,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_32,
    },
    {
        .input_rate = 176400,
        .output_rate = 22050,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_48,
    },
    {
        .input_rate = 176400,
        .output_rate = 29400,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_64,
    },
    {
        .input_rate = 176400,
        .output_rate = 44100,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    },
    {
        .input_rate = 176400,
        .output_rate = 88200,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
    {
        .input_rate = 352800,
        .output_rate = 24000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_24,
    },
    {
        .input_rate = 352800,
        .output_rate = 32000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_32,
    },
    {
        .input_rate = 352800,
        .output_rate = 48000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_48,
    },
    {
        .input_rate = 352800,
        .output_rate = 96000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_96,
    },
    {
        .input_rate = 352800,
        .output_rate = 192000,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_352_to_192,
    },
    {
        .input_rate = 352800,
        .output_rate = 22050,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_24,
    },
    {
        .input_rate = 352800,
        .output_rate = 29400,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_32,
    },
    {
        .input_rate = 352800,
        .output_rate = 44100,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_48,
    },
    {
        .input_rate = 352800,
        .output_rate = 88200,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_96,
    },
    {
        .input_rate = 352800,
        .output_rate = 176400,
        .table_id = MT7933_AFE_GASRC_IIR_COEFF_384_to_192,
    },
};

#define IIR_NUMS (48)

static const unsigned int
mt7933_afe_gasrc_iir_coeffs[MT7933_AFE_GASRC_IIR_TABLES][IIR_NUMS] = {
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_352] = {
        0x10bea3af, 0x2007e9be, 0x10bea3af,
        0xe2821372, 0xf0848d58, 0x00000003,
        0x08f9d435, 0x113d1a1f, 0x08f9d435,
        0xe31a73c5, 0xf1030af1, 0x00000003,
        0x09dd37b9, 0x13106967, 0x09dd37b9,
        0xe41398e1, 0xf1c98ae5, 0x00000003,
        0x0b55c74b, 0x16182d46, 0x0b55c74b,
        0xe5bce8cb, 0xf316f594, 0x00000003,
        0x0e02cb05, 0x1b950f07, 0x0e02cb05,
        0xf44d829a, 0xfaa9876b, 0x00000004,
        0x13e0e18e, 0x277f6d77, 0x13e0e18e,
        0xf695efae, 0xfc700da4, 0x00000004,
        0x0db3df0d, 0x1b6240b3, 0x0db3df0d,
        0xf201ce8e, 0xfca24567, 0x00000003,
        0x06b31e0f, 0x0cca96d1, 0x06b31e0f,
        0xc43a9021, 0xe051c370, 0x00000002,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_256_to_192] = {
        0x0de3c667, 0x137bf0e3, 0x0de3c667,
        0xd9575388, 0xe0d4770d, 0x00000002,
        0x0e54ed46, 0x1474090f, 0x0e54ed46,
        0xdb1c8213, 0xe2a7b6b7, 0x00000002,
        0x0d58713b, 0x13bde364, 0x05d8713b,
        0xde0a3770, 0xe5183cde, 0x00000002,
        0x0bdcfce3, 0x128ef355, 0x0bdcfce3,
        0xe2be28af, 0xe8affd19, 0x00000002,
        0x139091b3, 0x20f20a8e, 0x139091b3,
        0xe9ed58af, 0xedff795d, 0x00000002,
        0x0e68e9cd, 0x1a4cb00b, 0x0e68e9cd,
        0xf3ba2b24, 0xf5275137, 0x00000002,
        0x13079595, 0x251713f9, 0x13079595,
        0xf78c204d, 0xf227616a, 0x00000000,
        0x00000000, 0x2111eb8f, 0x2111eb8f,
        0x0014ac5b, 0x00000000, 0x00000006,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_352_to_256] = {
        0x0db45c84, 0x1113e68a, 0x0db45c84,
        0xdf58fbd3, 0xe0e51ba2, 0x00000002,
        0x0e0c4d8f, 0x11eaf5ef, 0x0e0c4d8f,
        0xe11e9264, 0xe2da4b80, 0x00000002,
        0x0cf2558c, 0x1154c11a, 0x0cf2558c,
        0xe41c6288, 0xe570c517, 0x00000002,
        0x0b5132d7, 0x10545ecd, 0x0b5132d7,
        0xe8e2e944, 0xe92f8dc6, 0x00000002,
        0x1234ffbb, 0x1cfba5c7, 0x1234ffbb,
        0xf00653e0, 0xee9406e3, 0x00000002,
        0x0cfd073a, 0x170277ad, 0x0cfd073a,
        0xf96e16e7, 0xf59562f9, 0x00000002,
        0x08506c2b, 0x1011cd72, 0x08506c2b,
        0x164a9eae, 0xe4203311, 0xffffffff,
        0x00000000, 0x3d58af1e, 0x3d58af1e,
        0x001bee13, 0x00000000, 0x00000007,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_256] = {
        0x0eca2fa9, 0x0f2b0cd3, 0x0eca2fa9,
        0xf50313ef, 0xf15857a7, 0x00000003,
        0x0ee239a9, 0x1045115c, 0x0ee239a9,
        0xec9f2976, 0xe5090807, 0x00000002,
        0x0ec57a45, 0x11d000f7, 0x0ec57a45,
        0xf0bb67bb, 0xe84c86de, 0x00000002,
        0x0e85ba7e, 0x13ee7e9a, 0x0e85ba7e,
        0xf6c74ebb, 0xecdba82c, 0x00000002,
        0x1cba1ac9, 0x2da90ada, 0x1cba1ac9,
        0xfecba589, 0xf2c756e1, 0x00000002,
        0x0f79dec4, 0x1c27f5e0, 0x0f79dec4,
        0x03c44399, 0xfc96c6aa, 0x00000003,
        0x1104a702, 0x21a72c89, 0x1104a702,
        0x1b6a6fb8, 0xfb5ee0f2, 0x00000001,
        0x0622fc30, 0x061a0c67, 0x0622fc30,
        0xe88911f2, 0xe0da327a, 0x00000002,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_352_to_192] = {
        0x1c012b9a, 0x09302bd9, 0x1c012b9a,
        0x0056c6d0, 0xe2b7f35c, 0x00000002,
        0x1b60cee5, 0x0b59639b, 0x1b60cee5,
        0x045dc965, 0xca2264a0, 0x00000001,
        0x19ec96ad, 0x0eb20aa9, 0x19ec96ad,
        0x0a6789cd, 0xd08944ba, 0x00000001,
        0x17c243aa, 0x1347e7fc, 0x17c243aa,
        0x131e03a8, 0xd9241dd4, 0x00000001,
        0x1563b168, 0x1904032f, 0x1563b168,
        0x0f0d206b, 0xf1d7f8e1, 0x00000002,
        0x14cd0206, 0x2169e2af, 0x14cd0206,
        0x14a5d991, 0xf7279caf, 0x00000002,
        0x0aac4c7f, 0x14cb084b, 0x0aac4c7f,
        0x30bc41c6, 0xf5565720, 0x00000001,
        0x0cea20d5, 0x03bc5f00, 0x0cea20d5,
        0xfeec800a, 0xc1b99664, 0x00000001,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_192] = {
        0x1bd356f3, 0x012e014f, 0x1bd356f3,
        0x081be0a6, 0xe28e2407, 0x00000002,
        0x0d7d8ee8, 0x01b9274d, 0x0d7d8ee8,
        0x09857a7b, 0xe4cae309, 0x00000002,
        0x0c999cbe, 0x038e89c5, 0x0c999cbe,
        0x0beae5bc, 0xe7ded2a4, 0x00000002,
        0x0b4b6e2c, 0x061cd206, 0x0b4b6e2c,
        0x0f6a2551, 0xec069422, 0x00000002,
        0x13ad5974, 0x129397e7, 0x13ad5974,
        0x13d3c166, 0xf11cacb8, 0x00000002,
        0x126195d4, 0x1b259a6c, 0x126195d4,
        0x184cdd94, 0xf634a151, 0x00000002,
        0x092aa1ea, 0x11add077, 0x092aa1ea,
        0x3682199e, 0xf31b28fc, 0x00000001,
        0x0e09b91b, 0x0010b76f, 0x0e09b91b,
        0x0f0e2575, 0xc19d364a, 0x00000001,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_176] = {
        0x1b4feb25, 0xfa1874df, 0x1b4feb25,
        0x0fc84364, 0xe27e7427, 0x00000002,
        0x0d22ad1f, 0xfe465ea8, 0x0d22ad1f,
        0x10d89ab2, 0xe4aa760e, 0x00000002,
        0x0c17b497, 0x004c9a14, 0x0c17b497,
        0x12ba36ef, 0xe7a11513, 0x00000002,
        0x0a968b87, 0x031b65c2, 0x0a968b87,
        0x157c39d1, 0xeb9561ce, 0x00000002,
        0x11cea26a, 0x0d025bcc, 0x11cea26a,
        0x18ef4a32, 0xf05a2342, 0x00000002,
        0x0fe5d188, 0x156af55c, 0x0fe5d188,
        0x1c6234df, 0xf50cd288, 0x00000002,
        0x07a1ea25, 0x0e900dd7, 0x07a1ea25,
        0x3d441ae6, 0xf0314c15, 0x00000001,
        0x0dd3517a, 0xfc7f1621, 0x0dd3517a,
        0x1ee4972a, 0xc193ad77, 0x00000001,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_256_to_96] = {
        0x0bad1c6d, 0xf7125e39, 0x0bad1c6d,
        0x200d2195, 0xe0e69a20, 0x00000002,
        0x0b7cc85d, 0xf7b2aa2b, 0x0b7cc85d,
        0x1fd4a137, 0xe2d2e8fc, 0x00000002,
        0x09ad4898, 0xf9f3edb1, 0x09ad4898,
        0x202ffee3, 0xe533035b, 0x00000002,
        0x073ebe31, 0xfcd552f2, 0x073ebe31,
        0x2110eb62, 0xe84975f6, 0x00000002,
        0x092af7cc, 0xff2b1fc9, 0x092af7cc,
        0x2262052a, 0xec1ceb75, 0x00000002,
        0x09655d3e, 0x04f0939d, 0x09655d3e,
        0x47cf219d, 0xe075904a, 0x00000001,
        0x021b3ca5, 0x03057f44, 0x021b3ca5,
        0x4a5c8f68, 0xe72b7f7b, 0x00000001,
        0x00000000, 0x389ecf53, 0x358ecf53,
        0x04b60049, 0x00000000, 0x00000004,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_352_to_128] = {
        0x0c4deacd, 0xf5b3be35, 0x0c4deacd,
        0x20349d1f, 0xe0b9a80d, 0x00000002,
        0x0c5dbbaa, 0xf6157998, 0x0c5dbbaa,
        0x200c143d, 0xe25209ea, 0x00000002,
        0x0a9de1bd, 0xf85ee460, 0x0a9de1bd,
        0x206099de, 0xe46a166c, 0x00000002,
        0x081f9a34, 0xfb7ffe47, 0x081f9a34,
        0x212dd0f7, 0xe753c9ab, 0x00000002,
        0x0a6f9ddb, 0xfd863e9e, 0x0a6f9ddb,
        0x226bd8a2, 0xeb2ead0b, 0x00000002,
        0x05497d0e, 0x01ebd7f0, 0x05497d0e,
        0x23eba2f6, 0xef958aff, 0x00000002,
        0x008e7c5f, 0x00be6aad, 0x008e7c5f,
        0x4a74b30a, 0xe6b0319a, 0x00000001,
        0x00000000, 0x38f3c5aa, 0x38f3c5aa,
        0x012e1306, 0x00000000, 0x00000006,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_128] = {
        0x0cf188aa, 0xf37845cc, 0x0cf188aa,
        0x126b5cbc, 0xf10e5785, 0x00000003,
        0x0c32c481, 0xf503c49b, 0x0c32c481,
        0x24e5a686, 0xe3edcb35, 0x00000002,
        0x0accda0f, 0xf7ad602d, 0x0accda0f,
        0x2547ad4f, 0xe65c4390, 0x00000002,
        0x08d6d7fb, 0xfb56b002, 0x08d6d7fb,
        0x25f3f39f, 0xe9860165, 0x00000002,
        0x0d4b1ceb, 0xff189a5d, 0x0d4b1ceb,
        0x26d3a3a5, 0xed391db5, 0x00000002,
        0x0a060fcf, 0x07a2d23a, 0x0a060fcf,
        0x27b2168e, 0xf0c10173, 0x00000002,
        0x040b6e8c, 0x0742638c, 0x040b6e8c,
        0x5082165c, 0xe5f8f032, 0x00000001,
        0x067a1ae1, 0xf98acf04, 0x067a1ae1,
        0x2526b255, 0xe0ab23e6, 0x00000002,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_352_to_96] = {
        0x0ba3aaf1, 0xf0c12941, 0x0ba3aaf1,
        0x2d8fe4ae, 0xe097f1ad, 0x00000002,
        0x0be92064, 0xf0b1f1a9, 0x0be92064,
        0x2d119d04, 0xe1e5fe1b, 0x00000002,
        0x0a1220de, 0xf3a9aff8, 0x0a1220de,
        0x2ccb18cb, 0xe39903cf, 0x00000002,
        0x07794a30, 0xf7c2c155, 0x07794a30,
        0x2ca647c8, 0xe5ef0ccd, 0x00000002,
        0x0910b1c4, 0xf84c9886, 0x0910b1c4,
        0x2c963877, 0xe8fbcb7a, 0x00000002,
        0x041d6154, 0xfec82c8a, 0x041d6154,
        0x2c926893, 0xec6aa839, 0x00000002,
        0x005b2676, 0x0050bb1f, 0x005b2676,
        0x5927e9f4, 0xde9fd5bc, 0x00000001,
        0x00000000, 0x2b1e5dc1, 0x2b1e5dc1,
        0x0164aa09, 0x00000000, 0x00000006,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_96] = {
        0x0481f41d, 0xf9c1b194, 0x0481f41d,
        0x31c66864, 0xe0581a1d, 0x00000002,
        0x0a3e5a4c, 0xf216665d, 0x0a3e5a4c,
        0x31c3de69, 0xe115ebae, 0x00000002,
        0x0855f15c, 0xf5369aef, 0x0855f15c,
        0x323c17ad, 0xe1feed04, 0x00000002,
        0x05caeeeb, 0xf940c54b, 0x05caeeeb,
        0x33295d2b, 0xe3295c94, 0x00000002,
        0x0651a46a, 0xfa4d6542, 0x0651a46a,
        0x3479d138, 0xe49580b2, 0x00000002,
        0x025e0ccb, 0xff36a412, 0x025e0ccb,
        0x35f517d7, 0xe6182a82, 0x00000002,
        0x0085eff3, 0x0074e0ca, 0x0085eff3,
        0x372ef0de, 0xe7504e71, 0x00000002,
        0x00000000, 0x29b76685, 0x29b76685,
        0x0deab1c3, 0x00000000, 0x00000003,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_88] = {
        0x0c95e01f, 0xed56f8fc, 0x0c95e01f,
        0x191b8467, 0xf0c99b0e, 0x00000003,
        0x0bbee41a, 0xef0e8160, 0x0bbee41a,
        0x31c02b41, 0xe2ef4cd9, 0x00000002,
        0x0a2d258f, 0xf2225b96, 0x0a2d258f,
        0x314c8bd2, 0xe4c10e08, 0x00000002,
        0x07f9e42a, 0xf668315f, 0x07f9e42a,
        0x30cf47d4, 0xe71e3418, 0x00000002,
        0x0afd6fa9, 0xf68f867d, 0x0afd6fa9,
        0x3049674d, 0xe9e0cf4b, 0x00000002,
        0x06ebc830, 0xffaa9acd, 0x06ebc830,
        0x2fcee1bf, 0xec81ee52, 0x00000002,
        0x010de038, 0x01a27806, 0x010de038,
        0x2f82d453, 0xee2ade9b, 0x00000002,
        0x064f0462, 0xf68a0d30, 0x064f0462,
        0x32c81742, 0xe07f3a37, 0x00000002,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_256_to_48] = {
        0x02b72fb4, 0xfb7c5152, 0x02b72fb4,
        0x374ab8ef, 0xe039095c, 0x00000002,
        0x05ca62de, 0xf673171b, 0x05ca62de,
        0x1b94186a, 0xf05c2de7, 0x00000003,
        0x09a9656a, 0xf05ffe29, 0x09a9656a,
        0x37394e81, 0xe1611f87, 0x00000002,
        0x06e86c29, 0xf54bf713, 0x06e86c29,
        0x37797f41, 0xe24ce1f6, 0x00000002,
        0x07a6b7c2, 0xf5491ea7, 0x07a6b7c2,
        0x37e40444, 0xe3856d91, 0x00000002,
        0x02bf8a3e, 0xfd2f5fa6, 0x02bf8a3e,
        0x38673190, 0xe4ea5a4d, 0x00000002,
        0x007e1bd5, 0x000e76ca, 0x007e1bd5,
        0x38da5414, 0xe61afd77, 0x00000002,
        0x00000000, 0x2038247b, 0x2038247b,
        0x07212644, 0x00000000, 0x00000004,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_352_to_64] = {
        0x05c89f29, 0xf6443184, 0x05c89f29,
        0x1bbe0f00, 0xf034bf19, 0x00000003,
        0x05e47be3, 0xf6284bfe, 0x05e47be3,
        0x1b73d610, 0xf0a9a268, 0x00000003,
        0x09eb6c29, 0xefbc8df5, 0x09eb6c29,
        0x365264ff, 0xe286ce76, 0x00000002,
        0x0741f28e, 0xf492d155, 0x0741f28e,
        0x35a08621, 0xe4320cfe, 0x00000002,
        0x087cdc22, 0xf3daa1c7, 0x087cdc22,
        0x34c55ef0, 0xe6664705, 0x00000002,
        0x038022af, 0xfc43da62, 0x038022af,
        0x33d2b188, 0xe8e92eb8, 0x00000002,
        0x001de8ed, 0x0001bd74, 0x001de8ed,
        0x33061aa8, 0xeb0d6ae7, 0x00000002,
        0x00000000, 0x3abd8743, 0x3abd8743,
        0x032b3f7f, 0x00000000, 0x00000005,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_64] = {
        0x05690759, 0xf69bdff3, 0x05690759,
        0x392fbdf5, 0xe032c3cc, 0x00000002,
        0x05c3ff7a, 0xf60d6b05, 0x05c3ff7a,
        0x1c831a72, 0xf052119a, 0x00000003,
        0x0999efb9, 0xefae71b0, 0x0999efb9,
        0x3900fd02, 0xe13a60b9, 0x00000002,
        0x06d5aa46, 0xf4c1d0ea, 0x06d5aa46,
        0x39199f34, 0xe20c15e1, 0x00000002,
        0x077f7d1d, 0xf49411e4, 0x077f7d1d,
        0x394b3591, 0xe321be50, 0x00000002,
        0x02a14b6b, 0xfcd3c8a5, 0x02a14b6b,
        0x398b4c12, 0xe45e5473, 0x00000002,
        0x00702155, 0xffef326c, 0x00702155,
        0x39c46c90, 0xe56c1e59, 0x00000002,
        0x00000000, 0x1c69d66c, 0x1c69d66c,
        0x0e76f270, 0x00000000, 0x00000003,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_352_to_48] = {
        0x05be8a21, 0xf589fb98, 0x05be8a21,
        0x1d8de063, 0xf026c3d8, 0x00000003,
        0x05ee4f4f, 0xf53df2e5, 0x05ee4f4f,
        0x1d4d87e2, 0xf07d5518, 0x00000003,
        0x0a015402, 0xee079bc7, 0x0a015402,
        0x3a0a0c2b, 0xe1e16c40, 0x00000002,
        0x07512c6a, 0xf322f651, 0x07512c6a,
        0x394e82c2, 0xe326def2, 0x00000002,
        0x087a5316, 0xf1d3ba1f, 0x087a5316,
        0x385bbd4a, 0xe4dbe26b, 0x00000002,
        0x035bd161, 0xfb2b7588, 0x035bd161,
        0x37464782, 0xe6d6a034, 0x00000002,
        0x00186dd8, 0xfff28830, 0x00186dd8,
        0x365746b9, 0xe88d9a4a, 0x00000002,
        0x00000000, 0x2cd02ed1, 0x2cd02ed1,
        0x035f6308, 0x00000000, 0x00000005,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_48] = {
        0x0c68c88c, 0xe9266466, 0x0c68c88c,
        0x1db3d4c3, 0xf0739c07, 0x00000003,
        0x05c69407, 0xf571a70a, 0x05c69407,
        0x1d6f1d3b, 0xf0d89718, 0x00000003,
        0x09e8d133, 0xee2a68df, 0x09e8d133,
        0x3a32d61b, 0xe2c2246a, 0x00000002,
        0x079233b7, 0xf2d17252, 0x079233b7,
        0x3959a2c3, 0xe4295381, 0x00000002,
        0x09c2822e, 0xf0613d7b, 0x09c2822e,
        0x385c3c48, 0xe5d3476b, 0x00000002,
        0x050e0b2c, 0xfa200d5d, 0x050e0b2c,
        0x37688f21, 0xe76fc030, 0x00000002,
        0x006ddb6e, 0x00523f01, 0x006ddb6e,
        0x36cd234d, 0xe8779510, 0x00000002,
        0x0635039f, 0xf488f773, 0x0635039f,
        0x3be42508, 0xe0488e99, 0x00000002,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_44] = {
        0x0c670696, 0xe8dc1ef2, 0x0c670696,
        0x1e05c266, 0xf06a9f0d, 0x00000003,
        0x05c60160, 0xf54b9f4a, 0x05c60160,
        0x1dc3811d, 0xf0c7e4db, 0x00000003,
        0x09e74455, 0xeddfc92a, 0x09e74455,
        0x3adfddda, 0xe28c4ae3, 0x00000002,
        0x078ea9ae, 0xf28c3ba7, 0x078ea9ae,
        0x3a0a98e8, 0xe3d93541, 0x00000002,
        0x09b32647, 0xefe954c5, 0x09b32647,
        0x3910a244, 0xe564f781, 0x00000002,
        0x04f0e9e4, 0xf9b7e8d5, 0x04f0e9e4,
        0x381f6928, 0xe6e5316c, 0x00000002,
        0x006303ee, 0x003ae836, 0x006303ee,
        0x37852c0e, 0xe7db78c1, 0x00000002,
        0x06337ac0, 0xf46665c5, 0x06337ac0,
        0x3c818406, 0xe042df81, 0x00000002,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_352_to_32] = {
        0x07d25973, 0xf0fd68ae, 0x07d25973,
        0x3dd9d640, 0xe02aaf11, 0x00000002,
        0x05a0521d, 0xf5390cc4, 0x05a0521d,
        0x1ec7dff7, 0xf044be0d, 0x00000003,
        0x04a961e1, 0xf71c730b, 0x04a961e1,
        0x1e9edeee, 0xf082b378, 0x00000003,
        0x06974728, 0xf38e3bf1, 0x06974728,
        0x3cd69b60, 0xe1afd01c, 0x00000002,
        0x072d4553, 0xf2c1e0e2, 0x072d4553,
        0x3c54fdc3, 0xe28e96b6, 0x00000002,
        0x02802de3, 0xfbb07dd5, 0x02802de3,
        0x3bc4f40f, 0xe38a3256, 0x00000002,
        0x000ce31b, 0xfff0d7a8, 0x000ce31b,
        0x3b4bbb40, 0xe45f55d6, 0x00000002,
        0x00000000, 0x1ea1b887, 0x1ea1b887,
        0x03b1b27d, 0x00000000, 0x00000005,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_32] = {
        0x0c5074a7, 0xe83ee090, 0x0c5074a7,
        0x1edf8fe7, 0xf04ec5d0, 0x00000003,
        0x05bbb01f, 0xf4fa20a7, 0x05bbb01f,
        0x1ea87e16, 0xf093b881, 0x00000003,
        0x04e8e57f, 0xf69fc31d, 0x04e8e57f,
        0x1e614210, 0xf0f1139e, 0x00000003,
        0x07756686, 0xf1f67c0b, 0x07756686,
        0x3c0a3b55, 0xe2d8c5a6, 0x00000002,
        0x097212e8, 0xeede0608, 0x097212e8,
        0x3b305555, 0xe3ff02e3, 0x00000002,
        0x0495d6c0, 0xf8bf1399, 0x0495d6c0,
        0x3a5c93a1, 0xe51e0d14, 0x00000002,
        0x00458b2d, 0xfffdc761, 0x00458b2d,
        0x39d4793b, 0xe5d6d407, 0x00000002,
        0x0609587b, 0xf456ed0f, 0x0609587b,
        0x3e1d20e1, 0xe0315c96, 0x00000002,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_352_to_24] = {
        0x062002ee, 0xf4075ac9, 0x062002ee,
        0x1f577599, 0xf0166280, 0x00000003,
        0x05cdb68c, 0xf4ab2e81, 0x05cdb68c,
        0x1f2a7a17, 0xf0484eb7, 0x00000003,
        0x04e3078b, 0xf67b954a, 0x04e3078b,
        0x1ef25b71, 0xf08a5bcf, 0x00000003,
        0x071fc81e, 0xf23391f6, 0x071fc81e,
        0x3d4bc51b, 0xe1cdf67e, 0x00000002,
        0x08359c1c, 0xf04d3910, 0x08359c1c,
        0x3c80bf1e, 0xe2c6cf99, 0x00000002,
        0x0331888d, 0xfa1ebde6, 0x0331888d,
        0x3b94c153, 0xe3e96fad, 0x00000002,
        0x00143063, 0xffe1d1af, 0x00143063,
        0x3ac672e3, 0xe4e7f96f, 0x00000002,
        0x00000000, 0x2d7cf831, 0x2d7cf831,
        0x074e3a4f, 0x00000000, 0x00000004,
    },
    [MT7933_AFE_GASRC_IIR_COEFF_384_to_24] = {
        0x0c513993, 0xe7dbde26, 0x0c513993,
        0x1f4e3b98, 0xf03b6bee, 0x00000003,
        0x05bd9980, 0xf4c4fb19, 0x05bd9980,
        0x1f21aa2b, 0xf06fa0e5, 0x00000003,
        0x04eb9c21, 0xf6692328, 0x04eb9c21,
        0x1ee6fb2f, 0xf0b6982c, 0x00000003,
        0x07795c9e, 0xf18d56cf, 0x07795c9e,
        0x3d345c1a, 0xe229a2a1, 0x00000002,
        0x096d3d11, 0xee265518, 0x096d3d11,
        0x3c7d096a, 0xe30bee74, 0x00000002,
        0x0478f0db, 0xf8270d5a, 0x0478f0db,
        0x3bc96998, 0xe3ea3cf8, 0x00000002,
        0x0037d4b8, 0xffdedcf0, 0x0037d4b8,
        0x3b553ec9, 0xe47a2910, 0x00000002,
        0x0607e296, 0xf42bc1d7, 0x0607e296,
        0x3ee67cb9, 0xe0252e31, 0x00000002,
    },
};

static bool mt7933_afe_gasrc_found_iir_coeff_table_id(int input_rate,
                                                      int output_rate, int *table_id)
{
    unsigned int i;
    const struct mt7933_afe_gasrc_iir_coeff_table_id *table =
            mt7933_afe_gasrc_iir_coeff_table_ids;

    if (!table_id)
        return false;

    /* no need to apply iir for up-sample */
    if (input_rate <= output_rate)
        return false;

    for (i = 0; i < ARRAY_SIZE(mt7933_afe_gasrc_iir_coeff_table_ids); i++) {
        if ((table[i].input_rate == input_rate) &&
            (table[i].output_rate == output_rate)) {
            *table_id = table[i].table_id;
            return true;
        }
    }

    return false;
}

static bool mt7933_afe_gasrc_fill_iir_coeff_table(struct mtk_base_afe *afe,
                                                  int gasrc_id, int table_id)
{
    const unsigned int *table;
    unsigned int ctrl_reg;
    int i;

    if ((table_id < 0) ||
        (table_id >= MT7933_AFE_GASRC_IIR_TABLES))
        return false;

    if (gasrc_id < 0)
        return false;

    aud_dbg("[%d] table_id %d", gasrc_id, table_id);

    table = &mt7933_afe_gasrc_iir_coeffs[table_id][0];

    /* enable access for iir sram */
    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_COEFF_SRAM_CTRL,
                                  GASRC_NEW_CON0_COEFF_SRAM_CTRL);

    /* fill coeffs from addr 0 */
    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con11;
    aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, 0);

    /* fill all coeffs */
    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con10;
    for (i = 0; i < IIR_NUMS; i++)
        aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, table[i]);

    /* disable access for iir sram */
    ctrl_reg = gasrc_ctrl_reg[gasrc_id].con0;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  GASRC_NEW_CON0_COEFF_SRAM_CTRL, 0);

    return true;
}

static bool mt7933_afe_load_gasrc_iir_coeff_table(struct mtk_base_afe *afe,
                                                  int gasrc_id, int input_rate, int output_rate)
{
    int table_id;

    if (mt7933_afe_gasrc_found_iir_coeff_table_id(input_rate,
                                                  output_rate, &table_id)) {
        return mt7933_afe_gasrc_fill_iir_coeff_table(afe,
                                                     gasrc_id, table_id);
    }

    return false;
}

static int mt7933_afe_configure_gasrc(struct mtk_base_afe *afe,
                                      struct snd_pcm_stream *stream,
                                      struct snd_soc_dai *dai,
                                      int gasrc_id,
                                      struct mt7933_gasrc_data *gasrc_data)
{
    int input_fs = 0, output_fs = 0;
    int input_rate = 0, output_rate = 0;
    unsigned int val = 0;
    unsigned int mask = 0;
    bool *gasrc_op_freq_45m;
    unsigned int *cali_cycles;

    gasrc_op_freq_45m = &(gasrc_data->op_freq_45m);
    cali_cycles = &(gasrc_data->cali_cycles);

    /* rate to timing */
    output_fs = mt7933_afe_fs_timing(gasrc_data->output_rate);
    if (output_fs < 0)
        aud_error("output timing error: %d", output_fs);
    output_rate = gasrc_data->output_rate;

    input_fs = mt7933_afe_fs_timing(gasrc_data->input_rate);
    if (input_fs < 0)
        aud_error("input timing error: %d", input_fs);
    input_rate = gasrc_data->input_rate;


    mt7933_afe_gasrc_set_input_fs(gasrc_id, input_fs);
    mt7933_afe_gasrc_set_output_fs(gasrc_id, output_fs);

    /* if it is slave mode, you neet to set track id */
    // function

    if (stream->direction == MSD_STREAM_PLAYBACK) {
        *gasrc_op_freq_45m = (input_rate % 8000);
    } else if (stream->direction == MSD_STREAM_CAPTURE) {
        *gasrc_op_freq_45m = (output_rate % 8000);
    }

    if (mt7933_afe_load_gasrc_iir_coeff_table(afe, gasrc_id,
                                              input_rate, output_rate)) {
        mt7933_afe_gasrc_enable_iir(afe, gasrc_id);
        gasrc_data->iir_on = true;
    } else {
        mt7933_afe_gasrc_disable_iir(afe, gasrc_id);
        gasrc_data->iir_on = false;
    }

    /* INT_ADDA ADC (RX Tracking of 26m) */
    if (stream->direction == MSD_STREAM_CAPTURE)
        mt7933_afe_adjust_gasrc_cali_cycles(afe, dai,
                                            input_fs, output_rate);


    gasrc_data->cali_tx = false;
    gasrc_data->cali_rx = false;
    if (stream->direction == MSD_STREAM_PLAYBACK) {
        if (mt7933_afe_gasrc_is_tx_tracking(output_fs))
            gasrc_data->cali_tx = true;
        else
            gasrc_data->cali_tx = false;
    } else if (stream->direction == MSD_STREAM_CAPTURE) {
        if (mt7933_afe_gasrc_is_rx_tracking(input_fs))
            gasrc_data->cali_rx = true;
        else
            gasrc_data->cali_rx = false;

        gasrc_data->cali_tx = false;
    }

    aud_dbg("[%d] %s cali_tx %d, cali_rx %d",
            gasrc_id, PCM_STREAM_STR(stream->direction),
            gasrc_data->cali_tx, gasrc_data->cali_rx);

    if (gasrc_data->channel_num > 2) {
        gasrc_data->one_heart = true;
    } else {
        gasrc_data->one_heart = false;
    }

    if (gasrc_data->one_heart && (gasrc_id != MT7933_GASRC0))
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, gasrc_ctrl_reg[(unsigned int)gasrc_id].con0,
                                      GASRC_NEW_CON0_ONE_HEART, GASRC_NEW_CON0_ONE_HEART);

    //  aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, gasrc_ctrl_reg[gasrc_id].con0,
    //      1 << 19, 1 << 19);

    if (stream->direction == MSD_STREAM_PLAYBACK) {
        mt7933_afe_gasrc_set_tx_mode_fs(afe,
                                        dai, input_rate, output_rate);
        if (gasrc_data->cali_tx) {
            mt7933_afe_gasrc_sel_cali_clk(afe,
                                          dai, *gasrc_op_freq_45m);
            mt7933_afe_gasrc_sel_lrck(afe, dai, output_fs);
        }
    } else if (stream->direction == MSD_STREAM_CAPTURE) {
        mt7933_afe_gasrc_set_rx_mode_fs(afe,
                                        dai, input_rate, output_rate);
        if (gasrc_data->cali_rx) {
            mt7933_afe_gasrc_sel_cali_clk(afe,
                                          dai, *gasrc_op_freq_45m);
            mt7933_afe_gasrc_sel_lrck(afe, dai, input_fs);
        }
    }

    if (gasrc_data->cali_tx || gasrc_data->cali_rx) {

        val = (*gasrc_op_freq_45m) ?
              GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_45M :
              GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_49M;
        mask = GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_MASK;
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, gasrc_ctrl_reg[(unsigned int)gasrc_id].con7,
                                      mask, val);

        val = GASRC_NEW_CON6_FREQ_CALI_CYCLE(*cali_cycles) |
              GASRC_NEW_CON6_COMP_FREQ_RES_EN |
              GASRC_NEW_CON6_FREQ_CALI_BP_DGL |
              GASRC_NEW_CON6_CALI_USE_FREQ_OUT |
              GASRC_NEW_CON6_FREQ_CALI_AUTO_RESTART;
        mask = GASRC_NEW_CON6_FREQ_CALI_CYCLE_MASK |
               GASRC_NEW_CON6_COMP_FREQ_RES_EN |
               GASRC_NEW_CON6_FREQ_CALI_BP_DGL |
               GASRC_NEW_CON6_CALI_USE_FREQ_OUT |
               GASRC_NEW_CON6_FREQ_CALI_AUTO_RESTART;

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, gasrc_ctrl_reg[(unsigned int)gasrc_id].con6,
                                      mask, val);
    }

    return 0;
}

static int mt7933_afe_gasrc_prepare(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_gasrc_data *gasrc_data;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return -EINVAL;
    }

    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    gasrc_data = &afe_priv->gasrc_data[gasrc_id];

    if (!gasrc_data->input_rate || !gasrc_data->output_rate) {
        aud_dbg("input_rate = %u, output_rate = %u", gasrc_data->input_rate, gasrc_data->output_rate);
        return 0;
    }

    mt7933_afe_reset_gasrc(gasrc_id);
    mt7933_afe_clear_gasrc(gasrc_id);
    mt7933_afe_gasrc_use_sel(gasrc_id, true);
    mt7933_afe_configure_gasrc(afe, stream, dai, gasrc_id, gasrc_data);

    return 0;

}

static int mt7933_afe_gasrc_startup(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    struct mtk_base_afe *afe = dai->private_data;
    //  struct mt7933_afe_private *afe_priv = afe->platform_priv;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);

    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    aud_msg("[%d] %s", gasrc_id, PCM_STREAM_STR(stream->direction));

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return -EINVAL;
    }

    mt7933_afe_enable_main_clk(afe);

    switch (gasrc_id) {
        case MT7933_GASRC0:
            mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_GASRC0);
            break;
        default:
            break;
    }

    return 0;
}

static void mt7933_afe_gasrc_shutdown(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_gasrc_data *gasrc_data;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return;
    }

    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    aud_msg("[%d] %s", gasrc_id, PCM_STREAM_STR(stream->direction));

    gasrc_data = &afe_priv->gasrc_data[gasrc_id];
    gasrc_data->re_enable[stream->direction] = false;

    switch (gasrc_id) {
        case MT7933_GASRC0:
            mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_GASRC0);
            break;
        default:
            break;
    }

    mt7933_afe_disable_main_clk(afe);
}

static int mt7933_afe_gasrc_trigger(struct snd_pcm_stream *stream, int cmd, struct snd_soc_dai *dai)
{

    int ret = 0;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return -EINVAL;
    }

    aud_msg("[%d] %s cmd %d", gasrc_id, PCM_STREAM_STR(stream->direction), cmd);
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    switch (cmd) {
        case SND_PCM_TRIGGER_START:
            ret = mt7933_afe_enable_gasrc(dai, stream->direction);
            break;
        case SND_PCM_TRIGGER_STOP:
            ret = mt7933_afe_disable_gasrc(dai, false);
            break;
        default:
            break;
    }

    return ret;
}

static int mt7933_afe_gasrc_hw_params(struct snd_pcm_stream *stream, struct msd_hw_params *params, struct snd_soc_dai *dai)
{
    struct snd_soc_dai *be_cpu_dai = NULL;
    struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct snd_soc_pcm_runtime *be_rtd = find_soc_pcm_be_runtime(rtd);
    if (!be_rtd) {
        be_cpu_dai = dai;
        aud_error("no valid be");
        return -EINVAL;
    }

    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_gasrc_data *gasrc_data;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);
    int direction = be_rtd->pcm->direction;

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return -EINVAL;
    }

    gasrc_data = &afe_priv->gasrc_data[gasrc_id];
    be_cpu_dai = be_rtd->cpu_dai;

    aud_msg("[%d] %s", gasrc_id, PCM_STREAM_STR(stream->direction));
    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    aud_msg("%s, be:%s, rate:%d, channel:%d, bit_width:%d", direction ? "capture" : "playback", be_cpu_dai->name, be_cpu_dai->rate, be_cpu_dai->channels, be_cpu_dai->bit_width);
    aud_msg("current hw param rate:%d, channel:%d", params->rate, params->channels);

    if (stream->direction == MSD_STREAM_PLAYBACK) {
        if (be_cpu_dai->driver->id > MT7933_AFE_BACKEND_END) {
            gasrc_data->input_rate = params->rate;
        } else {
            if (params->rate)
                gasrc_data->input_rate = params->rate;
            if (be_cpu_dai->rate) {
                gasrc_data->output_rate = be_cpu_dai->rate;
            }
        }
        gasrc_data->channel_num = params->channels;
    } else {
        if (be_cpu_dai->driver->id > MT7933_AFE_BACKEND_END) {
            gasrc_data->output_rate = params->rate;
        } else {
            if (params->rate)
                gasrc_data->output_rate = params->rate;
            if (be_cpu_dai->rate) {
                gasrc_data->input_rate = be_cpu_dai->rate;
            }
            gasrc_data->channel_num = params->channels;
        }
    }
    aud_msg("input_rate:%u, output_rate:%u", gasrc_data->input_rate, gasrc_data->output_rate);
    aud_dbg("channel_num = %u", gasrc_data->channel_num);
    return 0;
}

static int mt7933_afe_gasrc_hw_free(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_gasrc_data *gasrc_data;
    const int gasrc_id = mt7933_dai_num_to_gasrc(dai->driver->id);

    if (gasrc_id < 0) {
        aud_error("can not find gasrc_id:%d", gasrc_id);
        return -EINVAL;
    }

    gasrc_data = &afe_priv->gasrc_data[gasrc_id];
    gasrc_data->input_rate = 0;
    gasrc_data->output_rate = 0;

    aud_msg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_msg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    return 0;
}

static int mt7933_afe_int_adda_startup(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    unsigned int direction = stream->direction;

    mt7933_afe_enable_main_clk(afe);
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_26M]);

    if (direction == MSD_STREAM_PLAYBACK) {
        mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_DAC);
        mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_DAC_PREDIS);
        //      mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_DL_ASRC);
    } else if (direction == MSD_STREAM_CAPTURE) {
        mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_ADC);
        mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_ADC2);
    }

    return 0;
}

static int mt7933_afe_int_adda_prepare(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    struct mtk_base_afe *afe = dai->private_data;
    unsigned int direction = stream->direction;
    unsigned int rate = dai->rate;

    aud_dbg("stream %d, rate = %d\n", direction, rate);

    if (!mt7933_afe_is_int_1x_en_low_power(afe)) {
        if (rate % 8000)
            mt7933_afe_enable_apll_associated_cfg(afe,
                                                  MT7933_AFE_APLL1);
        else
            mt7933_afe_enable_apll_associated_cfg(afe,
                                                  MT7933_AFE_APLL2);
    }

    if (direction == MSD_STREAM_CAPTURE)
        return 0;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASMO_TIMING_CON0,
                                  ASMO_TIMING_CON0_ASMO0_MODE_MASK,
                                  ASMO_TIMING_CON0_ASMO0_MODE_VAL(mt7933_afe_fs_timing(rate)));

    return 0;
}

int mt7933_afe_is_int_1x_en_low_power(struct mtk_base_afe *afe)
{
    uint32_t val;
    int lp_mode;

    aud_drv_read_reg(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON, &val);

    lp_mode = (ASYS_TCON_LP_MODE_ON & val) ? 1 : 0;

    return lp_mode;
}

int mt7933_afe_disable_apll_associated_cfg(struct mtk_base_afe *afe,
                                           unsigned int apll)
{
    //  struct mt7933_afe_private *afe_priv = afe->platform_priv;

    if (apll == MT7933_AFE_APLL1) {
        mt7933_afe_disable_apll_tuner_cfg(afe, MT7933_AFE_APLL1);
        //      mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_APLL_TUNER);
        //      mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_APLL);
        //      clk_disable(afe_priv->clocks[MT7933_CLK_FAPLL1]);
    } else {
        mt7933_afe_disable_apll_tuner_cfg(afe, MT7933_AFE_APLL2);
        mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_APLL2_TUNER);
        mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_APLL2);
        //      clk_disable(afe_priv->clocks[MT7933_CLK_FAPLL2]);
    }

    return 0;
}

static void mt7933_afe_int_adda_shutdown(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    unsigned int direction = stream->direction;
    unsigned int rate = stream->runtime->rate;

    aud_dbg("rate:%u", rate);

    if (direction == MSD_STREAM_PLAYBACK) {
        mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_DAC_PREDIS);
        mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_DAC);
    } else if (direction == MSD_STREAM_CAPTURE) {
        mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_ADC);
        mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_ADC2);
    }

    if (!mt7933_afe_is_int_1x_en_low_power(afe)) {
        if (rate % 8000)
            mt7933_afe_disable_apll_associated_cfg(afe,
                                                   MT7933_AFE_APLL1);
        else
            mt7933_afe_disable_apll_associated_cfg(afe,
                                                   MT7933_AFE_APLL2);
    }

    mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_26M]);
    mt7933_afe_disable_main_clk(afe);
}

static void mt7933_afe_dec_etdm_occupy(struct mtk_base_afe *afe,
                                       unsigned int etdm_set, unsigned int stream)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
    spin_lock_flags_define(flags);

    spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

    etdm_data->occupied[stream]--;
    if (etdm_data->occupied[stream] < 0)
        etdm_data->occupied[stream] = 0;

    spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);
}

static void mt7933_afe_inc_etdm_occupy(struct mtk_base_afe *afe,
                                       unsigned int etdm_set, unsigned int stream)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
    spin_lock_flags_define(flags);

    spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

    etdm_data->occupied[stream]++;

    spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);
}

static unsigned int mt7933_afe_tdm_ch_fixup(unsigned int channels,
                                            unsigned int etdm_set)
{
    if ((channels > 8) && (!etdm_set))
        return 16;
    else if (channels > 4)
        return 8;
    else if (channels > 2)
        return 4;
    else
        return 2;
}

static const struct mt7933_etdm_ctrl_reg etdm_ctrl_reg[MT7933_ETDM_SETS][2] = {
    {
        {
            .con0 = -1,
            .con1 = -1,
            .con2 = -1,
            .con3 = -1,
            .con4 = -1,
        },
        {
            .con0 = ETDM_IN1_CON0,
            .con1 = ETDM_IN1_CON1,
            .con2 = ETDM_IN1_CON2,
            .con3 = ETDM_IN1_CON3,
            .con4 = ETDM_IN1_CON4,
        },
    },
    {
        {
            .con0 = ETDM_OUT2_CON0,
            .con1 = ETDM_OUT2_CON1,
            .con2 = ETDM_OUT2_CON2,
            .con3 = ETDM_OUT2_CON3,
            .con4 = ETDM_OUT2_CON4,
        },
        {
            .con0 = ETDM_IN2_CON0,
            .con1 = ETDM_IN2_CON1,
            .con2 = ETDM_IN2_CON2,
            .con3 = ETDM_IN2_CON3,
            .con4 = ETDM_IN2_CON4,
        },
    },
};

static int mt7933_afe_configure_etdm_out(struct mtk_base_afe *afe,
                                         unsigned int etdm_set, unsigned int rate,
                                         unsigned int channels, unsigned int bit_width)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
    unsigned int tdm_channels;
    unsigned int stream = MSD_STREAM_PLAYBACK;
    unsigned int clk_mode = etdm_data->clock_mode;
    unsigned int data_mode = etdm_data->data_mode[stream];
    unsigned int lrck_width = etdm_data->lrck_width[stream];
    bool slave_mode = etdm_data->slave_mode[stream];
    bool lrck_inv = etdm_data->lrck_inv[stream];
    bool bck_inv = etdm_data->bck_inv[stream];
    unsigned int fmt = etdm_data->format[stream];
    unsigned int ctrl_reg;
    unsigned int ctrl_mask;
    unsigned int val = 0;
    unsigned int bck;

    aud_dbg("%u rate:%u ch:%u bits:%u slave:%d data:%u",
            etdm_set + 1, rate, channels, bit_width,
            slave_mode, data_mode);

    tdm_channels = (data_mode == MT7933_ETDM_DATA_ONE_PIN) ?
                   mt7933_afe_tdm_ch_fixup(channels, etdm_set) : 2;

    bck = rate * tdm_channels * bit_width;

    val |= ETDM_CON0_BIT_LEN(bit_width);
    val |= ETDM_CON0_WORD_LEN(bit_width);
    val |= ETDM_CON0_FORMAT(fmt);
    val |= ETDM_CON0_CH_NUM(tdm_channels);

    if (clk_mode == MT7933_ETDM_SHARED_CLOCK)
        val |= ETDM_CON0_SYNC_MODE;

    if (slave_mode) {
        val |= ETDM_CON0_SLAVE_MODE;
        if (lrck_inv)
            val |= ETDM_CON0_SLAVE_LRCK_INV;
        if (bck_inv)
            val |= ETDM_CON0_SLAVE_BCK_INV;
    } else {
        if (lrck_inv)
            val |= ETDM_CON0_MASTER_LRCK_INV;
        if (bck_inv)
            val |= ETDM_CON0_MASTER_BCK_INV;
    }

    ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
    ctrl_mask = ETDM_OUT_CON0_CTRL_MASK;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg,
                                  ctrl_mask, val);

    val = 0;

    val |= ETDM_CON1_LRCK_AUTO_MODE;

    if (!slave_mode) {
        val |= ETDM_CON1_MCLK_OUTPUT;

        if (bck > MT7933_ETDM_NORMAL_MAX_BCK_RATE)
            val |= ETDM_CON1_BCK_FROM_DIVIDER;
    }

    if (lrck_width > 0)
        val |= ETDM_OUT_CON1_LRCK_WIDTH(lrck_width);
    else
        val |= ETDM_OUT_CON1_LRCK_WIDTH(bit_width);

    ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con1;
    ctrl_mask = ETDM_OUT_CON1_CTRL_MASK;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, ctrl_mask, val);

    val = ETDM_OUT_CON4_FS(mt7933_afe_fs_timing(rate));
    ctrl_mask = ETDM_OUT_CON4_CTRL_MASK;

    if (etdm_set == MT7933_ETDM2) {
        if (slave_mode)
            val |= ETDM_OUT_CON4_CONN_FS(MT7933_FS_ETDMOUT2_1X_EN);
        else
            val |= ETDM_OUT_CON4_CONN_FS(
                       mt7933_afe_fs_timing(rate));
        ctrl_mask |= ETDM_OUT_CON4_INTERCONN_EN_SEL_MASK;
    }

    ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con4;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, ctrl_mask, val);

    return 0;
}

static int mt7933_afe_configure_etdm_in(struct mtk_base_afe *afe,
                                        unsigned int etdm_set, unsigned int rate,
                                        unsigned int channels, unsigned int bit_width)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data =
            &afe_priv->etdm_data[etdm_set];
    unsigned int tdm_channels;
    unsigned int stream = MSD_STREAM_CAPTURE;
    unsigned int data_mode = etdm_data->data_mode[stream];
    unsigned int lrck_width = etdm_data->lrck_width[stream];
    bool slave_mode = etdm_data->slave_mode[stream];
    bool lrck_inv = etdm_data->lrck_inv[stream];
    bool bck_inv = etdm_data->bck_inv[stream];
    unsigned int fmt = etdm_data->format[stream];
    unsigned int ctrl_reg;
    unsigned int ctrl_mask;
    unsigned int val = 0;
    unsigned int bck;

    aud_dbg("%u rate:%u ch:%u bits:%u slave:%d data:%u",
            etdm_set + 1, rate, channels, bit_width,
            slave_mode, data_mode);

    tdm_channels = (data_mode == MT7933_ETDM_DATA_ONE_PIN) ?
                   mt7933_afe_tdm_ch_fixup(channels, etdm_set) : 2;

    val |= ETDM_CON0_BIT_LEN(bit_width);
    val |= ETDM_CON0_WORD_LEN(bit_width);
    val |= ETDM_CON0_FORMAT(fmt);
    val |= ETDM_CON0_CH_NUM(tdm_channels);

    bck = rate * tdm_channels * bit_width;

    if (slave_mode) {
        val |= ETDM_CON0_SLAVE_MODE;
        if (lrck_inv)
            val |= ETDM_CON0_SLAVE_LRCK_INV;
        if (bck_inv)
            val |= ETDM_CON0_SLAVE_BCK_INV;
    } else {
        if (lrck_inv)
            val |= ETDM_CON0_MASTER_LRCK_INV;
        if (bck_inv)
            val |= ETDM_CON0_MASTER_BCK_INV;
    }

    ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con0;
    ctrl_mask = ETDM_IN_CON0_CTRL_MASK;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, ctrl_mask, val);

    val = 0;

    val |= ETDM_CON1_LRCK_MANUAL_MODE;

    val |= ETDM_CON1_MCLK_OUTPUT;

    if (bck > MT7933_ETDM_NORMAL_MAX_BCK_RATE)
        val |= ETDM_CON1_BCK_FROM_DIVIDER;

    if (lrck_width > 0)
        val |= ETDM_IN_CON1_LRCK_WIDTH(lrck_width);
    else
        val |= ETDM_IN_CON1_LRCK_WIDTH(bit_width);

    ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con1;
    ctrl_mask = ETDM_IN_CON1_CTRL_MASK;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, ctrl_mask, val);

    val = ETDM_IN_CON3_FS(mt7933_afe_fs_timing(rate));

    ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con3;
    ctrl_mask = ETDM_IN_CON3_CTRL_MASK;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, ctrl_mask, val);

    if (slave_mode) {
        if (etdm_set == MT7933_ETDM1)
            val = ETDM_IN_CON4_CONN_FS(MT7933_FS_ETDMIN1_1X_EN);
        else
            val = ETDM_IN_CON4_CONN_FS(MT7933_FS_ETDMIN2_1X_EN);
    } else
        val = ETDM_IN_CON4_CONN_FS(
                  mt7933_afe_fs_timing(rate));

    ctrl_mask = ETDM_IN_CON4_CTRL_MASK;

    ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con4;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, ctrl_mask, val);

    val = 0;
    if (data_mode == MT7933_ETDM_DATA_MULTI_PIN) {
        val |= ETDM_IN_CON2_MULTI_IP_2CH_MODE |
               ETDM_IN_CON2_MULTI_IP_ONE_DATA |
               ETDM_IN_CON2_MULTI_IP_CH(tdm_channels);
    }

    val |= ETDM_IN_CON2_UPDATE_POINT_AUTO_DIS |
           ETDM_IN_CON2_UPDATE_POINT(1);

    ctrl_reg = etdm_ctrl_reg[etdm_set][stream].con2;
    ctrl_mask = ETDM_IN_CON2_CTRL_MASK;
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ctrl_reg, ctrl_mask, val);

    return 0;
}


static bool mt7933_afe_is_etdm_low_power(struct mtk_base_afe *afe,
                                         unsigned int etdm_set, unsigned int stream)
{
    unsigned int lp_mode_reg = etdm_ctrl_reg[etdm_set][stream].con0;
    uint32_t val;
    bool lp_mode;

    aud_drv_read_reg(AUDIO_REGMAP_AFE_BASE, lp_mode_reg, &val);
    lp_mode = (ETDM_CON0_LOW_POWER_MODE & val) ? true : false;

    return lp_mode;
}

static void mt7933_afe_enable_etdm(struct mtk_base_afe *afe,
                                   unsigned int etdm_set, unsigned int stream)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data =
            &afe_priv->etdm_data[etdm_set];
    unsigned int en_reg = etdm_ctrl_reg[etdm_set][stream].con0;
    spin_lock_flags_define(flags);
    bool need_update = false;

    aud_dbg("etdm_set %u", etdm_set);

    spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

    etdm_data->active[stream]++;
    if (etdm_data->active[stream] == 1)
        need_update = true;

    spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

    if (need_update)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, en_reg, 0x1, 0x1);
}

static void mt7933_afe_disable_etdm(struct mtk_base_afe *afe,
                                    unsigned int etdm_set, unsigned int stream)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data =
            &afe_priv->etdm_data[etdm_set];
    bool slave_mode = etdm_data->slave_mode[stream];
    unsigned int reg;
    spin_lock_flags_define(flags);
    bool need_update = false;

    aud_dbg("etdm_set %u", etdm_set);

    spin_lock_irqsave(&afe_priv->afe_ctrl_lock, flags);

    etdm_data->active[stream]--;
    if (etdm_data->active[stream] == 0)
        need_update = true;
    else if (etdm_data->active[stream] < 0)
        etdm_data->active[stream] = 0;

    spin_unlock_irqrestore(&afe_priv->afe_ctrl_lock, flags);

    if (need_update) {
        reg = etdm_ctrl_reg[etdm_set][stream].con0;
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, reg, 0x1, 0x0);

        if (slave_mode) {
            reg = etdm_ctrl_reg[etdm_set][stream].con4;
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, reg,
                                          ETDM_CON4_ASYNC_RESET,
                                          ETDM_CON4_ASYNC_RESET);
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, reg,
                                          ETDM_CON4_ASYNC_RESET,
                                          0);
        }
    }
}

static void mt7933_afe_etdm1_in_force_on(struct mtk_base_afe *afe)
{
    struct mt7933_afe_private *priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm1 = &priv->etdm_data[MT7933_ETDM1];
    unsigned int mclk, rate;
    bool need_tuner = false;

    mclk = etdm1->mclk_freq[MSD_STREAM_CAPTURE];
    rate = etdm1->force_rate[MSD_STREAM_CAPTURE];

    //  if (rate % 8000)
    //      mt7933_afe_enable_clk(afe, priv->clocks[MT7933_CLK_FA2SYS]);

    if (mclk == 0)
        mclk = MT7933_ETDM1_IN_MCLK_MULTIPLIER * rate;

    //  mt7933_afe_enable_clk(afe, priv->clocks[MT7933_CLK_TDMIN_MCK]);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_TDM_IN);

    mt7933_afe_configure_etdm_in(afe,
                                 MT7933_ETDM1,
                                 rate,
                                 etdm1->force_channels[MSD_STREAM_CAPTURE],
                                 etdm1->force_bit_width[MSD_STREAM_CAPTURE]);

    need_tuner = (mt7933_afe_is_int_1x_en_low_power(afe)
                  && !mt7933_afe_is_etdm_low_power(afe, MT7933_ETDM1,
                                                   MSD_STREAM_CAPTURE))
                 || (!mt7933_afe_is_int_1x_en_low_power(afe)
                     && mt7933_afe_is_etdm_low_power(afe, MT7933_ETDM1,
                                                     MSD_STREAM_CAPTURE));

    if (need_tuner) {
        if (rate % 8000) {
            mt7933_afe_enable_apll_associated_cfg(afe,
                                                  MT7933_AFE_APLL1);
        } else {
            mt7933_afe_enable_apll_associated_cfg(afe,
                                                  MT7933_AFE_APLL2);
        }
    }

    mt7933_afe_set_clk_rate(afe, priv->clocks_sel[MT7933_CLK_SEL_APLL12_DIV1],
                            mclk);

    mt7933_afe_enable_etdm(afe, MT7933_ETDM1, MSD_STREAM_CAPTURE);

    etdm1->force_on_status[MSD_STREAM_CAPTURE] = true;
}

static void mt7933_afe_etdm2_out_force_on(struct mtk_base_afe *afe)
{
    struct mt7933_afe_private *priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm2 = &priv->etdm_data[MT7933_ETDM2];
    unsigned int mclk, rate;
    bool need_tuner = false;

    mclk = etdm2->mclk_freq[MSD_STREAM_PLAYBACK];
    rate = etdm2->force_rate[MSD_STREAM_PLAYBACK];

    //  if (rate % 8000)
    //      mt7933_afe_enable_clk(afe, priv->clocks[MT7933_CLK_FA2SYS]);

    if (mclk == 0)
        mclk = MT7933_ETDM2_OUT_MCLK_MULTIPLIER * rate;

    //  mt7933_afe_enable_clk(afe, priv->clocks[MT7933_CLK_I2SOUT_MCK]);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_I2S_OUT);

    mt7933_afe_configure_etdm_out(afe,
                                  MT7933_ETDM2,
                                  rate,
                                  etdm2->force_channels[MSD_STREAM_PLAYBACK],
                                  etdm2->force_bit_width[MSD_STREAM_PLAYBACK]);

    need_tuner = (mt7933_afe_is_int_1x_en_low_power(afe)
                  && !mt7933_afe_is_etdm_low_power(afe, MT7933_ETDM2,
                                                   MSD_STREAM_PLAYBACK))
                 || (!mt7933_afe_is_int_1x_en_low_power(afe)
                     && mt7933_afe_is_etdm_low_power(afe, MT7933_ETDM2,
                                                     MSD_STREAM_PLAYBACK));

    if (need_tuner) {
        if (rate % 8000) {
            mt7933_afe_enable_apll_associated_cfg(afe,
                                                  MT7933_AFE_APLL1);
        } else {
            mt7933_afe_enable_apll_associated_cfg(afe,
                                                  MT7933_AFE_APLL2);
        }
    }

    mt7933_afe_set_clk_rate(afe,
                            priv->clocks_sel[MT7933_CLK_SEL_APLL12_DIV2],
                            mclk);

    mt7933_afe_enable_etdm(afe, MT7933_ETDM2, MSD_STREAM_PLAYBACK);

    etdm2->force_on_status[MSD_STREAM_PLAYBACK] = true;
}

static void mt7933_afe_etdm2_in_force_on(struct mtk_base_afe *afe)
{
    struct mt7933_afe_private *priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm2 = &priv->etdm_data[MT7933_ETDM2];
    unsigned int mclk, rate;
    bool need_tuner = false;

    mclk = etdm2->mclk_freq[MSD_STREAM_CAPTURE];
    rate = etdm2->force_rate[MSD_STREAM_CAPTURE];

    //  if (rate % 8000)
    //      mt7933_afe_enable_clk(afe, priv->clocks[MT7933_CLK_FA2SYS]);

    if (mclk == 0)
        mclk = MT7933_ETDM2_IN_MCLK_MULTIPLIER * rate;

    //  mt7933_afe_enable_clk(afe, priv->clocks[MT7933_CLK_I2SIN_MCK]);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_I2S_IN);

    mt7933_afe_configure_etdm_in(afe,
                                 MT7933_ETDM2,
                                 rate,
                                 etdm2->force_channels[MSD_STREAM_CAPTURE],
                                 etdm2->force_bit_width[MSD_STREAM_CAPTURE]);

    need_tuner = (mt7933_afe_is_int_1x_en_low_power(afe)
                  && !mt7933_afe_is_etdm_low_power(afe, MT7933_ETDM2,
                                                   MSD_STREAM_CAPTURE))
                 || (!mt7933_afe_is_int_1x_en_low_power(afe)
                     && mt7933_afe_is_etdm_low_power(afe, MT7933_ETDM2,
                                                     MSD_STREAM_CAPTURE));

    if (need_tuner) {
        if (rate % 8000) {
            mt7933_afe_enable_apll_associated_cfg(afe,
                                                  MT7933_AFE_APLL1);
        } else {
            mt7933_afe_enable_apll_associated_cfg(afe,
                                                  MT7933_AFE_APLL2);
        }
    }

    mt7933_afe_set_clk_rate(afe,
                            priv->clocks_sel[MT7933_CLK_SEL_APLL12_DIV0],
                            mclk);

    mt7933_afe_enable_etdm(afe, MT7933_ETDM2, MSD_STREAM_CAPTURE);

    etdm2->force_on_status[MSD_STREAM_CAPTURE] = true;
}

static int mt7933_afe_prepare_etdm_in(struct mtk_base_afe *afe,
                                      struct snd_soc_dai *dai, unsigned int etdm_set)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data =
            &afe_priv->etdm_data[etdm_set];
    unsigned int rate = dai->rate;
    unsigned int channels = dai->channels;
    unsigned int bit_width = dai->bit_width;
    unsigned int stream = MSD_STREAM_CAPTURE;

    aud_dbg("%u rate:%u ch:%u bits:%u slave:%d data:%u\n",
            etdm_set + 1, rate, channels, bit_width,
            etdm_data->slave_mode[stream],
            etdm_data->data_mode[stream]);

    mt7933_afe_configure_etdm_in(afe, etdm_set, rate,
                                 channels, bit_width);

    if (etdm_data->low_power_mode) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ETDM_OUT2_CON0, BIT(2), BIT(2));
    }
    return 0;
}

static int mt7933_afe_prepare_etdm_out(struct mtk_base_afe *afe,
                                       struct snd_soc_dai *dai,
                                       unsigned int etdm_set)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data =
            &afe_priv->etdm_data[etdm_set];
    unsigned int rate = dai->rate;
    unsigned int channels = dai->channels;
    unsigned int bit_width = dai->bit_width;
    unsigned int stream = MSD_STREAM_PLAYBACK;

    aud_dbg("%s#%u rate:%u ch:%u bits:%u slave:%d data:%u\n",
            __func__, etdm_set + 1, rate, channels, bit_width,
            etdm_data->slave_mode[stream],
            etdm_data->data_mode[stream]);

    mt7933_afe_configure_etdm_out(afe, etdm_set, rate,
                                  channels, bit_width);

    if (etdm_data->low_power_mode) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ETDM_OUT2_CON0, BIT(2), BIT(2));
    }
    return 0;
}

static int mt7933_afe_etdm1_startup(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data =
            &afe_priv->etdm_data[MT7933_ETDM1];
    unsigned int clk_mode = etdm_data->clock_mode;
    unsigned int direction = stream->direction;

    aud_dbg("stream %u clk_mode %u", direction, clk_mode);
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    if (etdm_data->force_on[direction]) {
        bool force_apply_in_change =
            !etdm_data->force_on_status[MSD_STREAM_CAPTURE]
            &&
            (etdm_data->force_on_policy[MSD_STREAM_CAPTURE]
             == MT7933_ETDM_FORCE_ON_1ST_TRIGGER);

        if (force_apply_in_change) {
            mt7933_afe_enable_main_clk(afe);

            if (force_apply_in_change)
                mt7933_afe_etdm1_in_force_on(afe);
        }

        return 0;
    }

    mt7933_afe_enable_main_clk(afe);

    if (direction == MSD_STREAM_PLAYBACK)
        aud_msg("stream %u is not supported!", direction);

    if (direction == MSD_STREAM_CAPTURE ||
        clk_mode == MT7933_ETDM_SHARED_CLOCK) {
        //      mt7933_afe_enable_clk(afe,
        //          afe_priv->clocks[MT7933_CLK_TDMIN_MCK]);
        mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_TDM_IN);
    }

    return 0;
}

static void mt7933_afe_etdm1_shutdown(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    const int dai_id = dai->driver->id;
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_be_dai_data *be =
            &afe_priv->be_data[dai_id - MT7933_AFE_BACKEND_BASE];
    unsigned int etdm_set = MT7933_ETDM1;
    struct mt7933_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
    unsigned int clk_mode = etdm_data->clock_mode;
    unsigned int direction = stream->direction;
    bool reset_in_change = (direction == MSD_STREAM_CAPTURE) ||
                           (clk_mode == MT7933_ETDM_SHARED_CLOCK);
    unsigned int rate = dai->rate;
    bool need_tuner = false;

    aud_dbg("stream %u clk_mode %u", direction, clk_mode);
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    if (reset_in_change) {
        if ((etdm_data->occupied[direction] == 1)
            && !etdm_data->force_on[direction]) {
            need_tuner = (mt7933_afe_is_int_1x_en_low_power(afe)
                          && !mt7933_afe_is_etdm_low_power(afe, etdm_set,
                                                           MSD_STREAM_CAPTURE))
                         || (!mt7933_afe_is_int_1x_en_low_power(afe)
                             && mt7933_afe_is_etdm_low_power(afe, etdm_set,
                                                             MSD_STREAM_CAPTURE));

            if (need_tuner) {
                if (rate % 8000) {
                    mt7933_afe_disable_apll_associated_cfg(
                        afe, MT7933_AFE_APLL1);
                } else {
                    mt7933_afe_disable_apll_associated_cfg(
                        afe, MT7933_AFE_APLL2);
                }
            }
        }
        mt7933_afe_dec_etdm_occupy(afe, etdm_set,
                                   MSD_STREAM_CAPTURE);
        hal_clock_disable(afe_priv->clocks[MT7933_CLK_APLL12_DIV1]);
    }

    if (etdm_data->force_on[direction])
        return;

    if (etdm_data->occupied[direction] == 0) {
        if (reset_in_change)
            mt7933_afe_disable_etdm(afe, etdm_set,
                                    MSD_STREAM_CAPTURE);
    }

    if (reset_in_change) {
        mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_TDM_IN);
        //      mt7933_afe_disable_clk(afe,
        //          afe_priv->clocks[MT7933_CLK_TDMIN_MCK]);
    }

    if (be->prepared[direction]) {
        be->prepared[direction] = false;
        if (rate % 8000) {
            //          mt7933_afe_disable_clk(afe,
            //                  afe_priv->clocks[MT7933_CLK_FA2SYS]);
        }
    }

    mt7933_afe_disable_main_clk(afe);
}

static int mt7933_afe_etdm1_prepare(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    const int dai_id = dai->driver->id;
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_be_dai_data *be =
            &afe_priv->be_data[dai_id - MT7933_AFE_BACKEND_BASE];
    unsigned int etdm_set = MT7933_ETDM1;
    struct mt7933_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
    unsigned int clk_mode = etdm_data->clock_mode;
    unsigned int direction = stream->direction;
    bool apply_in_change = (direction == MSD_STREAM_CAPTURE) ||
                           (clk_mode == MT7933_ETDM_SHARED_CLOCK);
    bool need_tuner = false;
    unsigned int rate = dai->rate;
    unsigned int mclk = afe_priv->etdm_data[etdm_set].mclk_freq[direction];
    int ret;

    aud_dbg("stream %u clk_mode %u occupied %d",
            direction, clk_mode, etdm_data->occupied[direction]);
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    if (etdm_data->occupied[direction] || be->prepared[direction]) {
        aud_msg("stream %u prepared %d, occupied %d\n", direction,
                be->prepared[direction], etdm_data->occupied[direction]);
        return 0;
    }

    if (apply_in_change)
        mt7933_afe_inc_etdm_occupy(afe, etdm_set,
                                   MSD_STREAM_CAPTURE);

    if (etdm_data->force_on[direction])
        return 0;

    if (apply_in_change) {
        ret = mt7933_afe_prepare_etdm_in(afe, dai, etdm_set);
        if (ret)
            return ret;
    }

    //  if (rate % 8000)
    //      mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_FA2SYS]);

    if (apply_in_change) {
        if (mclk == 0)
            mclk = MT7933_ETDM1_IN_MCLK_MULTIPLIER * rate;

        need_tuner =
            (mt7933_afe_is_int_1x_en_low_power(afe)
             && !mt7933_afe_is_etdm_low_power(afe, etdm_set,
                                              MSD_STREAM_CAPTURE))
            || (!mt7933_afe_is_int_1x_en_low_power(afe)
                && mt7933_afe_is_etdm_low_power(afe, etdm_set,
                                                MSD_STREAM_CAPTURE));

        if (need_tuner) {
            if (rate % 8000) {
                mt7933_afe_enable_apll_associated_cfg(afe,
                                                      MT7933_AFE_APLL1);
            } else {
                mt7933_afe_enable_apll_associated_cfg(afe,
                                                      MT7933_AFE_APLL2);
            }
        }

        mt7933_afe_set_clk_rate(afe,
                                afe_priv->clocks_sel[MT7933_CLK_SEL_APLL12_DIV1],
                                mclk);
        mt7933_afe_enable_etdm(afe, etdm_set,
                               MSD_STREAM_CAPTURE);
    }

    be->prepared[direction] = true;

    return 0;
}

static int mt7933_afe_etdm2_startup(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data =
            &afe_priv->etdm_data[MT7933_ETDM2];
    unsigned int clk_mode = etdm_data->clock_mode;
    unsigned int direction = stream->direction;

    aud_dbg("stream %u clk_mode %u occupied %d",
            direction, clk_mode, etdm_data->occupied[direction]);
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    if (etdm_data->force_on[direction]) {
        bool force_apply_out_change =
            !etdm_data->force_on_status[MSD_STREAM_PLAYBACK]
            &&
            (etdm_data->force_on_policy[MSD_STREAM_PLAYBACK]
             == MT7933_ETDM_FORCE_ON_1ST_TRIGGER);
        bool force_apply_in_change =
            !etdm_data->force_on_status[MSD_STREAM_CAPTURE]
            &&
            (etdm_data->force_on_policy[MSD_STREAM_CAPTURE]
             == MT7933_ETDM_FORCE_ON_1ST_TRIGGER);

        if (force_apply_out_change || force_apply_in_change) {
            if (clk_mode == MT7933_ETDM_SHARED_CLOCK) {
                force_apply_out_change = true;
                force_apply_in_change = true;
            }

            mt7933_afe_enable_main_clk(afe);

            if (force_apply_out_change)
                mt7933_afe_etdm2_out_force_on(afe);

            if (force_apply_in_change)
                mt7933_afe_etdm2_in_force_on(afe);
        }

        return 0;
    }

    mt7933_afe_enable_main_clk(afe);

    if (direction == MSD_STREAM_PLAYBACK ||
        clk_mode == MT7933_ETDM_SHARED_CLOCK) {
        //      mt7933_afe_enable_clk(afe,
        //          afe_priv->clocks[MT7933_CLK_I2SOUT_MCK]);
        mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_I2S_OUT);
    }

    if (direction == MSD_STREAM_CAPTURE ||
        clk_mode == MT7933_ETDM_SHARED_CLOCK) {
        //      mt7933_afe_enable_clk(afe,
        //          afe_priv->clocks[MT7933_CLK_I2SIN_MCK]);
        mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_I2S_IN);
    }

    return 0;
}

static int mt7933_afe_etdm_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    struct mtk_base_afe *afe = dai->private_data;
    const int dai_id = dai->driver->id;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    unsigned int etdm_set = (dai_id == MT7933_AFE_IO_ETDM1_IN) ?
                            MT7933_ETDM1 : MT7933_ETDM2;
    unsigned int stream = (dai_id == MT7933_AFE_IO_ETDM2_OUT) ?
                          MSD_STREAM_PLAYBACK :
                          MSD_STREAM_CAPTURE;

    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    aud_dbg("fmt:%u", fmt);

    if (afe_priv->etdm_data[etdm_set].force_on[stream])
        return 0;

    switch (fmt & MSD_SOC_DAIFMT_FORMAT_MASK) {
        case MSD_SOC_DAIFMT_I2S:
            afe_priv->etdm_data[etdm_set].format[stream] =
                MT7933_ETDM_FORMAT_I2S;
            break;
        case MSD_SOC_DAIFMT_DSP_A:
            afe_priv->etdm_data[etdm_set].format[stream] =
                MT7933_ETDM_FORMAT_DSPA;
            break;
        case MSD_SOC_DAIFMT_DSP_B:
            afe_priv->etdm_data[etdm_set].format[stream] =
                MT7933_ETDM_FORMAT_DSPB;
            break;
        case MSD_SOC_DAIFMT_LEFT_J:
            afe_priv->etdm_data[etdm_set].format[stream] =
                MT7933_ETDM_FORMAT_LJ;
            break;
        case MSD_SOC_DAIFMT_RIGHT_J:
            afe_priv->etdm_data[etdm_set].format[stream] =
                MT7933_ETDM_FORMAT_RJ;
            break;
        default:
            return -1;
    }

    switch (fmt & MSD_SOC_DAIFMT_INV_MASK) {
        case MSD_SOC_DAIFMT_NB_NF:
            afe_priv->etdm_data[etdm_set].bck_inv[stream] = false;
            afe_priv->etdm_data[etdm_set].lrck_inv[stream] = false;
            break;
        case MSD_SOC_DAIFMT_NB_IF:
            afe_priv->etdm_data[etdm_set].bck_inv[stream] = false;
            afe_priv->etdm_data[etdm_set].lrck_inv[stream] = true;
            break;
        case MSD_SOC_DAIFMT_IB_NF:
            afe_priv->etdm_data[etdm_set].bck_inv[stream] = true;
            afe_priv->etdm_data[etdm_set].lrck_inv[stream] = false;
            break;
        case MSD_SOC_DAIFMT_IB_IF:
            afe_priv->etdm_data[etdm_set].bck_inv[stream] = true;
            afe_priv->etdm_data[etdm_set].lrck_inv[stream] = true;
            break;
        default:
            return -1;
    }

    switch (fmt & MSD_SOC_DAIFMT_MASTER_MASK) {
        case MSD_SOC_DAIFMT_CBM_CFM:
            afe_priv->etdm_data[etdm_set].slave_mode[stream] = true;
            break;
        case MSD_SOC_DAIFMT_CBS_CFS:
            afe_priv->etdm_data[etdm_set].slave_mode[stream] = false;
            break;
        default:
            return -1;
    }

    return 0;
}

static int mt7933_afe_etdm_set_tdm_slot(struct snd_soc_dai *dai,
                                        unsigned int tx_mask,
                                        unsigned int rx_mask,
                                        int slots,
                                        int slot_width)
{
    struct mtk_base_afe *afe = dai->private_data;
    const int dai_id = dai->driver->id;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    unsigned int etdm_set = (dai_id == MT7933_AFE_IO_ETDM1_IN) ?
                            MT7933_ETDM1 : MT7933_ETDM2;
    unsigned int stream = (dai_id == MT7933_AFE_IO_ETDM2_OUT) ?
                          MSD_STREAM_PLAYBACK :
                          MSD_STREAM_CAPTURE;

    aud_dbg("%d etdm %u stream %u slot_width %d",
            dai_id, etdm_set, stream, slot_width);
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    if (afe_priv->etdm_data[etdm_set].force_on[stream])
        return 0;

    afe_priv->etdm_data[etdm_set].lrck_width[stream] = slot_width;

    return 0;
}

static int mt7933_afe_etdm_set_sysclk(struct snd_soc_dai *dai,
                                      int clk_id,
                                      unsigned int freq,
                                      int dir)
{
    struct mtk_base_afe *afe = dai->private_data;
    const int dai_id = dai->driver->id;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    unsigned int etdm_set = (dai_id == MT7933_AFE_IO_ETDM1_IN) ?
                            MT7933_ETDM1 : MT7933_ETDM2;
    unsigned int stream = (dai_id == MT7933_AFE_IO_ETDM2_OUT) ?
                          MSD_STREAM_PLAYBACK :
                          MSD_STREAM_CAPTURE;

    aud_dbg("%d etdm %u stream %u freq %u", dai_id, etdm_set, stream, freq);
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    if (afe_priv->etdm_data[etdm_set].force_on[stream])
        return 0;

    afe_priv->etdm_data[etdm_set].mclk_freq[stream] = freq;

    return 0;
}

static int mt7933_afe_etdm2_prepare(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    const int dai_id = dai->driver->id;
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_be_dai_data *be =
            &afe_priv->be_data[dai_id - MT7933_AFE_BACKEND_BASE];
    unsigned int etdm_set = MT7933_ETDM2;
    struct mt7933_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
    unsigned int clk_mode = etdm_data->clock_mode;
    unsigned int direction = stream->direction;
    bool apply_out_change = (direction == MSD_STREAM_PLAYBACK) ||
                            (clk_mode == MT7933_ETDM_SHARED_CLOCK);
    bool apply_in_change = (direction == MSD_STREAM_CAPTURE) ||
                           (clk_mode == MT7933_ETDM_SHARED_CLOCK);
    bool need_tuner = false;
    unsigned int rate = dai->rate;
    unsigned int mclk = afe_priv->etdm_data[etdm_set].mclk_freq[direction];
    int ret;

    aud_dbg("stream %u clk_mode %u occupied %d",
            direction, clk_mode, etdm_data->occupied[direction]);
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    if (etdm_data->occupied[direction] || be->prepared[direction]) {
        aud_msg("stream %u prepared %d, occupied %d\n",
                direction, be->prepared[direction], etdm_data->occupied[direction]);
        return 0;
    }

    if (apply_out_change)
        mt7933_afe_inc_etdm_occupy(afe, etdm_set,
                                   MSD_STREAM_PLAYBACK);

    if (apply_in_change)
        mt7933_afe_inc_etdm_occupy(afe, etdm_set,
                                   MSD_STREAM_CAPTURE);

    if (etdm_data->force_on[direction])
        return 0;

    if (apply_out_change) {
        ret = mt7933_afe_prepare_etdm_out(afe, dai,
                                          etdm_set);
        if (ret)
            return ret;
    }

    if (apply_in_change) {
        ret = mt7933_afe_prepare_etdm_in(afe, dai,
                                         etdm_set);
        if (ret)
            return ret;
    }

    //  if (rate % 8000)
    //      mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_FA2SYS]);

    if (apply_out_change) {
        if (mclk == 0)
            mclk = MT7933_ETDM2_OUT_MCLK_MULTIPLIER * rate;

        need_tuner = (mt7933_afe_is_int_1x_en_low_power(afe)
                      && !mt7933_afe_is_etdm_low_power(afe,
                                                       etdm_set, MSD_STREAM_PLAYBACK))
                     || (!mt7933_afe_is_int_1x_en_low_power(afe)
                         && mt7933_afe_is_etdm_low_power(afe,
                                                         etdm_set, MSD_STREAM_PLAYBACK));

        if (need_tuner) {
            if (rate % 8000) {
                mt7933_afe_enable_apll_associated_cfg(afe,
                                                      MT7933_AFE_APLL1);
            } else {
                mt7933_afe_enable_apll_associated_cfg(afe,
                                                      MT7933_AFE_APLL2);
            }
        }

        mt7933_afe_set_clk_rate(afe,
                                afe_priv->clocks_sel[MT7933_CLK_SEL_APLL12_DIV2],
                                mclk);

        mt7933_afe_enable_etdm(afe, etdm_set,
                               MSD_STREAM_PLAYBACK);
    }

    if (apply_in_change) {
        if (mclk == 0)
            mclk = MT7933_ETDM1_IN_MCLK_MULTIPLIER * rate;

        need_tuner =
            (mt7933_afe_is_int_1x_en_low_power(afe)
             && !mt7933_afe_is_etdm_low_power(afe, etdm_set,
                                              MSD_STREAM_CAPTURE))
            || (!mt7933_afe_is_int_1x_en_low_power(afe)
                && mt7933_afe_is_etdm_low_power(afe, etdm_set,
                                                MSD_STREAM_CAPTURE));

        if (need_tuner) {
            if (rate % 8000) {
                mt7933_afe_enable_apll_associated_cfg(afe,
                                                      MT7933_AFE_APLL1);
            } else {
                mt7933_afe_enable_apll_associated_cfg(afe,
                                                      MT7933_AFE_APLL2);
            }
        }

        mt7933_afe_set_clk_rate(afe,
                                afe_priv->clocks_sel[MT7933_CLK_SEL_APLL12_DIV0],
                                mclk);
        mt7933_afe_enable_etdm(afe, etdm_set,
                               MSD_STREAM_CAPTURE);
    }

    be->prepared[direction] = true;

    return 0;
}

static void mt7933_afe_etdm2_shutdown(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    const int dai_id = dai->driver->id;
    struct mtk_base_afe *afe = dai->private_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_be_dai_data *be =
            &afe_priv->be_data[dai_id - MT7933_AFE_BACKEND_BASE];
    unsigned int etdm_set = MT7933_ETDM2;
    struct mt7933_etdm_data *etdm_data = &afe_priv->etdm_data[etdm_set];
    unsigned int clk_mode = etdm_data->clock_mode;
    unsigned int direction = stream->direction;
    bool reset_out_change = (direction == MSD_STREAM_PLAYBACK) ||
                            (clk_mode == MT7933_ETDM_SHARED_CLOCK);
    bool reset_in_change = (direction == MSD_STREAM_CAPTURE) ||
                           (clk_mode == MT7933_ETDM_SHARED_CLOCK);
    bool need_tuner = false;
    unsigned int rate = dai->rate;

    aud_dbg("stream %u clk_mode %u occupied %d\n",
            direction, clk_mode, etdm_data->occupied[direction]);
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    if (reset_out_change) {
        if ((etdm_data->occupied[direction] == 1)
            && !etdm_data->force_on[direction]) {
            need_tuner = (mt7933_afe_is_int_1x_en_low_power(afe)
                          && !mt7933_afe_is_etdm_low_power(afe, etdm_set,
                                                           MSD_STREAM_PLAYBACK))
                         || (!mt7933_afe_is_int_1x_en_low_power(afe)
                             && mt7933_afe_is_etdm_low_power(afe, etdm_set,
                                                             MSD_STREAM_PLAYBACK));

            if (need_tuner) {
                if (rate % 8000) {
                    mt7933_afe_disable_apll_associated_cfg(
                        afe, MT7933_AFE_APLL1);
                } else {
                    mt7933_afe_disable_apll_associated_cfg(
                        afe, MT7933_AFE_APLL2);
                }
            }
        }
        mt7933_afe_dec_etdm_occupy(afe, etdm_set,
                                   MSD_STREAM_PLAYBACK);
        hal_clock_disable(afe_priv->clocks[MT7933_CLK_APLL12_DIV2]);
    }

    if (reset_in_change) {
        if ((etdm_data->occupied[direction] == 1)
            && !etdm_data->force_on[direction]) {
            need_tuner = (mt7933_afe_is_int_1x_en_low_power(afe)
                          && !mt7933_afe_is_etdm_low_power(afe, etdm_set,
                                                           MSD_STREAM_CAPTURE))
                         || (!mt7933_afe_is_int_1x_en_low_power(afe)
                             && mt7933_afe_is_etdm_low_power(afe, etdm_set,
                                                             MSD_STREAM_CAPTURE));

            if (need_tuner) {
                if (rate % 8000) {
                    mt7933_afe_disable_apll_associated_cfg(
                        afe, MT7933_AFE_APLL1);
                } else {
                    mt7933_afe_disable_apll_associated_cfg(
                        afe, MT7933_AFE_APLL2);
                }
            }
        }
        mt7933_afe_dec_etdm_occupy(afe, etdm_set,
                                   MSD_STREAM_CAPTURE);
        hal_clock_disable(afe_priv->clocks[MT7933_CLK_APLL12_DIV0]);
    }

    if (etdm_data->force_on[direction])
        return;

    if (etdm_data->occupied[direction] == 0) {
        if (reset_out_change)
            mt7933_afe_disable_etdm(afe, etdm_set,
                                    MSD_STREAM_PLAYBACK);

        if (reset_in_change)
            mt7933_afe_disable_etdm(afe, etdm_set,
                                    MSD_STREAM_CAPTURE);
    }

    if (reset_out_change) {
        mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_I2S_OUT);
        //      mt7933_afe_disable_clk(afe,
        //          afe_priv->clocks[MT7933_CLK_I2SOUT_MCK]);
    }

    if (reset_in_change) {
        mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_I2S_IN);
        //      mt7933_afe_disable_clk(afe,
        //          afe_priv->clocks[MT7933_CLK_I2SIN_MCK]);
    }
    if (be->prepared[direction]) {
        be->prepared[direction] = false;
        if (rate % 8000) {
            //          mt7933_afe_disable_clk(afe,
            //                  afe_priv->clocks[MT7933_CLK_FA2SYS]);
        }
    }
    mt7933_afe_disable_main_clk(afe);

    //  return 0;
}

static void mtk_afe_fe_reg_dump(const struct afe_dump_reg_attr *regs, unsigned int regs_num)
{
    unsigned int index;
    for (index = 0; index < regs_num; index ++) {
        aud_dbg("%s[0x%08x] = 0x%08x", regs[index].name, regs[index].offset,
                aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, regs[index].offset));
    }

}

static void mtk_afe_fe_reg_dump_msg(const struct afe_dump_reg_attr *regs, unsigned int regs_num)
{
    unsigned int index;
    for (index = 0; index < regs_num; index ++) {
        aud_msg("%s[0x%08x] = 0x%08x", regs[index].name, regs[index].offset,
                aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, regs[index].offset));
    }

}

struct afe_dump_clock_attr {
    int clock_id;
    char *name;
};

#define DUMP_CLOCK_ENTRY(clock_id) {clock_id, #clock_id}

static const struct afe_dump_clock_attr memif_dump_clocks[] = {
    DUMP_CLOCK_ENTRY(HAL_CLOCK_CG_AUDIO_XTAL),
    DUMP_CLOCK_ENTRY(HAL_CLOCK_CG_AUDSYS_BUS),
    DUMP_CLOCK_ENTRY(HAL_CLOCK_CG_XPLL),
    DUMP_CLOCK_ENTRY(HAL_CLOCK_SEL_APLL12_CK_DIV0),
    DUMP_CLOCK_ENTRY(HAL_CLOCK_SEL_APLL12_CK_DIV6),
    DUMP_CLOCK_ENTRY(HAL_CLOCK_SEL_APLL12_CK_DIV3),
    DUMP_CLOCK_ENTRY(HAL_CLOCK_CG_AUD_ADC0_XTAL),
    DUMP_CLOCK_ENTRY(HAL_CLOCK_CG_AUD_ADC1_XTAL),
    DUMP_CLOCK_ENTRY(HAL_CLOCK_CG_AUD_DAC_XTAL),
    DUMP_CLOCK_ENTRY(HAL_CLOCK_CG_TOP_PLL),
};

static int mt7933_afe_fe_reg_dump(struct snd_pcm_stream *stream)
{
    mtk_afe_fe_reg_dump(memif_dump_regs, ARRAY_SIZE(memif_dump_regs));
    mtk_afe_fe_reg_dump(irq_dump_regs, ARRAY_SIZE(irq_dump_regs));
    mtk_afe_fe_reg_dump(conn_dump_regs, ARRAY_SIZE(conn_dump_regs));
    mtk_afe_fe_reg_dump(adda_dump_regs, ARRAY_SIZE(adda_dump_regs));
    mtk_afe_fe_reg_dump(gasrc_dump_regs, ARRAY_SIZE(gasrc_dump_regs));
    mtk_afe_fe_reg_dump(etdm_dump_regs, ARRAY_SIZE(etdm_dump_regs));

    unsigned int index;
    for (index = 0; index < ARRAY_SIZE(memif_dump_clocks); index ++) {
        aud_dbg("%s[%d] = %d", memif_dump_clocks[index].name, memif_dump_clocks[index].clock_id,
                hal_clock_is_enabled(memif_dump_clocks[index].clock_id));
    }
    return 0;
}

static int mt7933_afe_dmic_startup(struct snd_pcm_stream *stream,
                                   struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;

    aud_msg("");

    mt7933_afe_enable_main_clk(afe);

    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_DMIC0);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_DMIC1);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_DMIC_TML);

    return 0;
}

static void mt7933_afe_disable_dmic(struct mtk_base_afe *afe)
{
    unsigned int msk = 0;

    msk |= DMIC_UL_CON0_MODE_3P25M_CH1_CTL;
    msk |= DMIC_UL_CON0_MODE_3P25M_CH2_CTL;
    msk |= DMIC_UL_CON0_SRC_ON_TMP_CTL;

    //  aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC3_UL_SRC_CON0,
    //      msk, 0x0);
    //  aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC2_UL_SRC_CON0,
    //      msk, 0x0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC1_UL_SRC_CON0,
                                  msk, 0x0);
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC0_UL_SRC_CON0,
                                  msk, 0x0);

#if 0
    /*DE said 7933 dmic no need set this,8168 need */
    regmap_update_bits(afe->regmap, AFE_ADDA_UL_DL_CON0,
                       ADDA_UL_DL_CON0_ADDA_INTF_ON, 0x0);

    regmap_update_bits(afe->regmap, AFE_ADDA_UL_DL_CON0,
                       ADDA_UL_DL_CON0_DMIC_CLKDIV_ON, 0x0);
#endif /* #if 0 */

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, PWR2_TOP_CON1,
                                  PWR2_TOP_CON1_DMIC_PDM_INTF_ON, 0x0);
}

static void mt7933_afe_dmic_shutdown(struct snd_pcm_stream *stream,
                                     struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;

    aud_msg("");

    mt7933_afe_disable_dmic(afe);

    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_DMIC_TML);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_DMIC1);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_DMIC0);

    mt7933_afe_disable_main_clk(afe);
}

static void mt7933_afe_enable_dmic(struct mtk_base_afe *afe,
                                   struct snd_soc_dai *dai)
{
    unsigned int channels = dai->channels;
    unsigned int val = 0;
    unsigned int msk = 0;
    aud_msg("channels = %d", channels);

    msk |= DMIC_UL_CON0_MODE_3P25M_CH1_CTL;
    msk |= DMIC_UL_CON0_MODE_3P25M_CH2_CTL;
    msk |= DMIC_UL_CON0_SRC_ON_TMP_CTL;
    val = msk;

    //  if (channels > 6)
    //      aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC3_UL_SRC_CON0,
    //          msk, val);

    //  if (channels > 4)
    //      aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC2_UL_SRC_CON0,
    //          msk, val);

    if (channels > 2)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC1_UL_SRC_CON0,
                                      msk, val);

    if (channels > 0) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC0_UL_SRC_CON0,
                                      msk, val);
    }

#if 0
    /*DE said 7933 dmic no need set this,8168 need */
    /* adda afe on */
    regmap_update_bits(afe->regmap, AFE_ADDA_UL_DL_CON0,
                       ADDA_UL_DL_CON0_ADDA_INTF_ON,
                       ADDA_UL_DL_CON0_ADDA_INTF_ON);

    /* dmic clkdiv on */
    regmap_update_bits(afe->regmap, AFE_ADDA_UL_DL_CON0,
                       ADDA_UL_DL_CON0_DMIC_CLKDIV_ON,
                       ADDA_UL_DL_CON0_DMIC_CLKDIV_ON);
#endif /* #if 0 */

    /* dmic ck div on */
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, PWR2_TOP_CON1,
                                  PWR2_TOP_CON1_DMIC_PDM_INTF_ON,
                                  PWR2_TOP_CON1_DMIC_PDM_INTF_ON);
}

static int mt7933_afe_configure_dmic_array(struct mtk_base_afe *afe)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_dmic_data *dmic_data = &afe_priv->dmic_data;
    unsigned int *dmic_src_sel = dmic_data->dmic_src_sel;
    unsigned int mask =
        PWR2_TOP_CON_DMIC4_SRC_SEL_MASK |
        PWR2_TOP_CON_DMIC3_SRC_SEL_MASK |
        PWR2_TOP_CON_DMIC2_SRC_SEL_MASK |
        PWR2_TOP_CON_DMIC1_SRC_SEL_MASK;
    unsigned int val =
        PWR2_TOP_CON_DMIC4_SRC_SEL_VAL(dmic_src_sel[3]) |
        PWR2_TOP_CON_DMIC3_SRC_SEL_VAL(dmic_src_sel[2]) |
        PWR2_TOP_CON_DMIC2_SRC_SEL_VAL(dmic_src_sel[1]) |
        PWR2_TOP_CON_DMIC1_SRC_SEL_VAL(dmic_src_sel[0]);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, PWR2_TOP_CON0, mask, val);

    return 0;
}

static const struct mtk_reg_sequence mt7933_afe_dmic_iir_coeff_reg_defaults[] = {
    { AFE_DMIC0_IIR_COEF_02_01, 0x00000000 },
    { AFE_DMIC0_IIR_COEF_04_03, 0x00003FB8 },
    { AFE_DMIC0_IIR_COEF_06_05, 0x3FB80000 },
    { AFE_DMIC0_IIR_COEF_08_07, 0x3FB80000 },
    { AFE_DMIC0_IIR_COEF_10_09, 0x0000C048 },
    { AFE_DMIC1_IIR_COEF_02_01, 0x00000000 },
    { AFE_DMIC1_IIR_COEF_04_03, 0x00003FB8 },
    { AFE_DMIC1_IIR_COEF_06_05, 0x3FB80000 },
    { AFE_DMIC1_IIR_COEF_08_07, 0x3FB80000 },
    { AFE_DMIC1_IIR_COEF_10_09, 0x0000C048 },
};

static int mt7933_afe_load_dmic_iir_coeff_table(struct mtk_base_afe *afe)
{
    aud_drv_multi_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE,
                                   mt7933_afe_dmic_iir_coeff_reg_defaults,
                                   ARRAY_SIZE(mt7933_afe_dmic_iir_coeff_reg_defaults));
    return 0;
}

static int mt7933_afe_configure_dmic(struct mtk_base_afe *afe,
                                     struct snd_pcm_stream *stream,
                                     struct snd_soc_dai *dai)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_dmic_data *dmic_data = &afe_priv->dmic_data;
    unsigned int rate = dai->rate;
    unsigned int channels = dai->channels;
    bool two_wire_mode = dmic_data->two_wire_mode;
    unsigned int clk_phase_sel_ch1 = dmic_data->clk_phase_sel_ch1;
    unsigned int clk_phase_sel_ch2 = dmic_data->clk_phase_sel_ch2;
    bool iir_on = dmic_data->iir_on;
    unsigned int dmic_ul_mode = dmic_data->ul_mode;
    unsigned int val = 0;

    val |= DMIC_UL_CON0_SDM_3_LEVEL_CTL;

    if (two_wire_mode) {
        val |= DMIC_UL_CON0_TWO_WIRE_MODE_CTL;
        val |= DMIC_UL_CON0_PHASE_SEL_CH1(clk_phase_sel_ch1);
        val |= DMIC_UL_CON0_PHASE_SEL_CH2(clk_phase_sel_ch2);
    } else {
        val |= DMIC_UL_CON0_PHASE_SEL_CH1(clk_phase_sel_ch1);
        val |= DMIC_UL_CON0_PHASE_SEL_CH2((clk_phase_sel_ch1 + 4)
                                          & 0x7);
    }

    mt7933_afe_configure_dmic_array(afe);

    switch (rate) {
        case 48000:
            val |= DMIC_UL_CON0_VOCIE_MODE_48K;
            break;
        case 32000:
            val |= DMIC_UL_CON0_VOCIE_MODE_32K;
            break;
        case 16000:
            val |= DMIC_UL_CON0_VOCIE_MODE_16K;
            break;
        case 8000:
            val |= DMIC_UL_CON0_VOCIE_MODE_8K;
            break;
        default:
            return -EINVAL;
    }

    val |= DMIC_UL_CON0_3P25M_1P625M_SEL(dmic_ul_mode & 0x1);
    val |= DMIC_UL_CON0_LOW_POWER_MODE_SEL(dmic_ul_mode);

    if (iir_on) {
        mt7933_afe_load_dmic_iir_coeff_table(afe);
        val |= DMIC_UL_CON0_IIR_MODE_SEL(0); /* SW mode */
        val |= DMIC_UL_CON0_IIR_ON_TMP_CTL;
    }

    //  if (channels > 6)
    //      aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC3_UL_SRC_CON0,
    //          DMIC_UL_CON0_CONFIG_MASK, val);

    //  if (channels > 4)
    //      aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC2_UL_SRC_CON0,
    //          DMIC_UL_CON0_CONFIG_MASK, val);

    if (channels > 2)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC1_UL_SRC_CON0,
                                      DMIC_UL_CON0_CONFIG_MASK, val);

    if (channels > 0) {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_DMIC0_UL_SRC_CON0,
                                      DMIC_UL_CON0_CONFIG_MASK, val);
    }

    return 0;
}

static int mt7933_afe_dmic_prepare(struct snd_pcm_stream *stream,
                                   struct snd_soc_dai *dai)
{
    //  struct snd_soc_pcm_runtime *rtd = stream->private_data;
    struct mtk_base_afe *afe = dai->private_data;

    aud_msg("");

    mt7933_afe_configure_dmic(afe, stream, dai);
    mt7933_afe_enable_dmic(afe, dai);

    return 0;
}

static int mt7933_afe_hw_gain1_startup(struct snd_pcm_stream *stream,
                                       struct snd_soc_dai *dai)
{
    struct mtk_base_afe *afe = dai->private_data;

    mt7933_afe_enable_main_clk(afe);
    return 0;
}

static void mt7933_afe_hw_gain1_shutdown(struct snd_pcm_stream *stream,
                                         struct snd_soc_dai *dai)
{
    struct mtk_base_afe *afe = dai->private_data;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  AFE_GAIN1_CON0, AFE_GAIN1_CON0_EN_MASK, 0);

    mt7933_afe_disable_main_clk(afe);
}

static int mt7933_afe_hw_gain1_prepare(struct snd_pcm_stream *stream,
                                       struct snd_soc_dai *dai)
{
    unsigned int rate = dai->rate;
    int fs;
    unsigned int val1 = 0, val2 = 0;

    aud_msg("rate = %d", rate);

    fs = mt7933_afe_fs_timing(rate);
    if (fs < 0)
        return -EINVAL;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  AFE_GAIN1_CON0,
                                  AFE_GAIN1_CON0_MODE_MASK,
                                  AFE_GAIN1_CON0_MODE_VAL((unsigned int)fs));

    val1 = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_GAIN1_CON1);
    val2 = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_GAIN1_CUR);

    if ((val1 & AFE_GAIN1_CON1_MASK) != (val2 & AFE_GAIN1_CUR_MASK))
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                      AFE_GAIN1_CUR,
                                      AFE_GAIN1_CUR_MASK, val1);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  AFE_GAIN1_CON0,
                                  AFE_GAIN1_CON0_EN_MASK, 1);

    return 0;
}

static const struct snd_soc_dai_pcm_ops mt7933_afe_fe_dai_pcm_ops = {
    .startup = mt7933_afe_fe_startup,
    .hw_params = mt7933_afe_fe_hw_params,
    .prepare = mt7933_afe_fe_prepare,
    .trigger = mt7933_afe_fe_trigger,
    .hw_free = mt7933_afe_fe_hw_free,
    .shutdown = mt7933_afe_fe_shutdown,
    .copy = mt7933_afe_fe_copy,
    .pointer = mt7933_afe_fe_pointer,
    .reg_dump = mt7933_afe_fe_reg_dump,
};

static const struct snd_soc_dai_pcm_ops mt7933_afe_etdm1_ops = {
    .startup = mt7933_afe_etdm1_startup,
    .shutdown = mt7933_afe_etdm1_shutdown,
    .prepare = mt7933_afe_etdm1_prepare,
    .set_fmt = mt7933_afe_etdm_set_fmt,
    .set_tdm_slot = mt7933_afe_etdm_set_tdm_slot,
    .set_sysclk = mt7933_afe_etdm_set_sysclk,
};

static const struct snd_soc_dai_pcm_ops mt7933_afe_etdm2_ops = {
    .startup = mt7933_afe_etdm2_startup,
    .set_fmt = mt7933_afe_etdm_set_fmt,
    .set_tdm_slot = mt7933_afe_etdm_set_tdm_slot,
    .set_sysclk = mt7933_afe_etdm_set_sysclk,
    .prepare = mt7933_afe_etdm2_prepare,
    .shutdown = mt7933_afe_etdm2_shutdown,
};

static const struct snd_soc_dai_pcm_ops mt7933_afe_gasrc_pcm_ops = {
    .startup = mt7933_afe_gasrc_startup,
    .hw_params = mt7933_afe_gasrc_hw_params,
    .prepare = mt7933_afe_gasrc_prepare,
    .trigger = mt7933_afe_gasrc_trigger,
    .hw_free = mt7933_afe_gasrc_hw_free,
    .shutdown = mt7933_afe_gasrc_shutdown,
};

static const struct snd_soc_dai_pcm_ops mt7933_afe_hw_gain_pcm_ops = {
    .startup    = mt7933_afe_hw_gain1_startup,
    .shutdown   = mt7933_afe_hw_gain1_shutdown,
    .prepare    = mt7933_afe_hw_gain1_prepare,
};

static const struct snd_soc_dai_pcm_ops mt7933_afe_int_adda_pcm_ops = {
    .startup = mt7933_afe_int_adda_startup,
    .prepare = mt7933_afe_int_adda_prepare,
    .shutdown = mt7933_afe_int_adda_shutdown,
};

#if 0
static const struct snd_soc_dai_compr_ops mt7933_afe_fe_dai_compr_ops = {
    .open = mt7933_afe_fe_compr_open,
    .free = mt7933_afe_fe_compr_free,
    .set_params = mt7933_afe_fe_compr_set_params,
    .get_params = mt7933_afe_fe_compr_get_params,
    .set_metadata = mt7933_afe_fe_compr_set_metadata,
    .get_metadata = mt7933_afe_fe_compr_get_metadata,
    .trigger = mt7933_afe_fe_compr_trigger,
    .pointer = mt7933_afe_fe_compr_pointer,
    .copy = mt7933_afe_fe_compr_copy,
    .ack = mt7933_afe_fe_compr_ack,
    .get_caps = mt7933_afe_fe_compr_get_caps,
    .get_codec_caps = mt7933_afe_fe_compr_get_codec_caps,
};
#endif /* #if 0 */

static const struct snd_soc_dai_pcm_ops mt7933_afe_dmic_pcm_ops = {
    .startup    = mt7933_afe_dmic_startup,
    .shutdown   = mt7933_afe_dmic_shutdown,
    .prepare    = mt7933_afe_dmic_prepare,
};

static struct snd_soc_dai_driver mt7933_afe_pcm_dais[] = {
    //FE
    {
        .name = "dlm-cpu-dai",
        .id = MT7933_AFE_MEMIF_DLM,
        .pcm_ops = &mt7933_afe_fe_dai_pcm_ops,
        .constr = {
            .stream_name = "dlm-cpu-dai",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000 |
            SNDRV_PCM_RATE_12000 | SNDRV_PCM_RATE_24000,
            .channels_min = 1,
            .channels_max = 8,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "dl2-cpu-dai",
        .id = MT7933_AFE_MEMIF_DL2,
        .pcm_ops = &mt7933_afe_fe_dai_pcm_ops,
        .constr = {
            .stream_name = "dl2-cpu-dai",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000 |
            SNDRV_PCM_RATE_12000 | SNDRV_PCM_RATE_24000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "dl3-cpu-dai",
        .id = MT7933_AFE_MEMIF_DL3,
        .pcm_ops = &mt7933_afe_fe_dai_pcm_ops,
        .constr = {
            .stream_name = "dl3-cpu-dai",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000 |
            SNDRV_PCM_RATE_12000 | SNDRV_PCM_RATE_24000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "cm9-cpu-dai",
        .id = MT7933_AFE_MEMIF_UL9,
        .pcm_ops = &mt7933_afe_fe_dai_pcm_ops,
        .constr = {
            .stream_name = "cm9-cpu-dai",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 16,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "ul2-cpu-dai",
        .id = MT7933_AFE_MEMIF_UL2,
        .pcm_ops = &mt7933_afe_fe_dai_pcm_ops,
        .constr = {
            .stream_name = "ul2-cpu-dai",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 8,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "ul3-cpu-dai",
        .id = MT7933_AFE_MEMIF_UL3,
        .pcm_ops = &mt7933_afe_fe_dai_pcm_ops,
        .constr = {
            .stream_name = "ul3-cpu-dai",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "ul4-cpu-dai",
        .id = MT7933_AFE_MEMIF_UL4,
        .pcm_ops = &mt7933_afe_fe_dai_pcm_ops,
        .constr = {
            .stream_name = "ul4-cpu-dai",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "ul8-cpu-dai",
        .id = MT7933_AFE_MEMIF_UL8,
        .pcm_ops = &mt7933_afe_fe_dai_pcm_ops,
        .constr = {
            .stream_name = "ul8-cpu-dai",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 8,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "ul10-cpu-dai",
        .id = MT7933_AFE_MEMIF_UL10,
        .pcm_ops = &mt7933_afe_fe_dai_pcm_ops,
        .constr = {
            .stream_name = "ul10-cpu-dai",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
    //BE
    {
        .name = "GASRC0",
        .id = MT7933_AFE_IO_GASRC0,
        .pcm_ops = &mt7933_afe_gasrc_pcm_ops,
        .constr = {
            .stream_name = "GASRC0",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "HW_GAIN1",
        .id = MT7933_AFE_IO_HW_GAIN1,
        .pcm_ops = &mt7933_afe_hw_gain_pcm_ops,
        .constr = {
            .stream_name = "HW_GAIN1",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "etdm1-in-cpu-dai",
        .id = MT7933_AFE_IO_ETDM1_IN,
        .pcm_ops = &mt7933_afe_etdm1_ops,
        .constr = {
            .stream_name = "ETDM1 Capture",
            .channels_min = 1,
            .channels_max = 8,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .buffer_bytes_align = 64,
        }
    },
    {
        .name = "etdm2-out-cpu-dai",
        .id = MT7933_AFE_IO_ETDM2_OUT,
        .pcm_ops = &mt7933_afe_etdm2_ops,
        .constr = {
            .stream_name = "ETDM2 Playback",
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .buffer_bytes_align = 64,
        }
    },
    {
        .name = "etdm2-in-cpu-dai",
        .id = MT7933_AFE_IO_ETDM2_IN,
        .pcm_ops = &mt7933_afe_etdm2_ops,
        .constr = {
            .stream_name = "ETDM2 Capture",
            .channels_min = 1,
            .channels_max = 2,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
        }
    },
    {
        .name = "INT ADDA Playback",
        .id = MT7933_AFE_IO_INT_ADDA,
        .pcm_ops = &mt7933_afe_int_adda_pcm_ops,
        .constr = {
            .stream_name = "INT ADDA Playback",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_48000 |
            SNDRV_PCM_RATE_12000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "INT ADDA Capture",
        .id = MT7933_AFE_IO_INT_ADDA,
        .pcm_ops = &mt7933_afe_int_adda_pcm_ops,
        .constr = {
            .stream_name = "INT ADDA Capture",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000,
            .channels_min = 1,
            .channels_max = 3,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "DMIC",
        .id = MT7933_AFE_IO_DMIC,
        .pcm_ops = &mt7933_afe_dmic_pcm_ops,
        .constr = {
            .stream_name = "DMIC Capture",
            .channels_min = 1,
            .channels_max = 4,
            .rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000,
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "snd-soc-dummy",
        .id = -1,
        .constr = {
            .stream_name = "snd-soc-dummy",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000 |
            SNDRV_PCM_RATE_12000 | SNDRV_PCM_RATE_24000,
            .channels_min = 1,
            .channels_max = 384,
        },
    },
};

/* register of Ix_Oy_S, x: 0 ~ 31, y: 0 ~ 49 */
/* bit position of Ix_Oy_S, x: 0 ~ 31, y: 0 ~ 49*/
static struct afe_interconn afe_intercon_L[] = {
    [O_00] = {.reg = -1,  .offset = 0},
    [O_01] = {.reg = -1,  .offset = 0},
    [O_02] = {.reg = -1,  .offset = 0},
    [O_03] = {.reg = -1,  .offset = 0},
    [O_04] = {.reg = AFE_CONN4,  .offset = 0},
    [O_05] = {.reg = AFE_CONN5,  .offset = 0},
    [O_06] = {.reg = -1,  .offset = 0},
    [O_07] = {.reg = -1,  .offset = 0},
    [O_08] = {.reg = -1,  .offset = 0},
    [O_09] = {.reg = -1,  .offset = 0},
    [O_10] = {.reg = -1, .offset = 0},
    [O_11] = {.reg = -1, .offset = 0},
    [O_12] = {.reg = AFE_CONN12, .offset = 0},
    [O_13] = {.reg = AFE_CONN13, .offset = 0},
    [O_14] = {.reg = AFE_CONN14, .offset = 0},
    [O_15] = {.reg = AFE_CONN15, .offset = 0},
    [O_16] = {.reg = AFE_CONN16, .offset = 0},
    [O_17] = {.reg = AFE_CONN17, .offset = 0},
    [O_18] = {.reg = AFE_CONN18, .offset = 0},
    [O_19] = {.reg = AFE_CONN19, .offset = 0},
    [O_20] = {.reg = AFE_CONN20, .offset = 0},
    [O_21] = {.reg = AFE_CONN21, .offset = 0},
    [O_22] = {.reg = AFE_CONN22, .offset = 0},
    [O_23] = {.reg = AFE_CONN23, .offset = 0},
    [O_24] = {.reg = -1, .offset = 0},
    [O_25] = {.reg = -1, .offset = 0},
    [O_26] = {.reg = AFE_CONN26, .offset = 0},
    [O_27] = {.reg = AFE_CONN27, .offset = 0},
    [O_28] = {.reg = AFE_CONN28, .offset = 0},
    [O_29] = {.reg = AFE_CONN29, .offset = 0},
    [O_30] = {.reg = AFE_CONN30, .offset = 0},
    [O_31] = {.reg = AFE_CONN31, .offset = 0},
    [O_32] = {.reg = AFE_CONN32, .offset = 0},
    [O_33] = {.reg = AFE_CONN33, .offset = 0},
    [O_34] = {.reg = AFE_CONN34, .offset = 0},
    [O_35] = {.reg = AFE_CONN35, .offset = 0},
    [O_36] = {.reg = AFE_CONN36, .offset = 0},
    [O_37] = {.reg = AFE_CONN37, .offset = 0},
    [O_38] = {.reg = AFE_CONN38, .offset = 0},
    [O_39] = {.reg = AFE_CONN39, .offset = 0},
    [O_40] = {.reg = AFE_CONN40, .offset = 0},
    [O_41] = {.reg = AFE_CONN41, .offset = 0},
    [O_42] = {.reg = AFE_CONN42, .offset = 0},
    [O_43] = {.reg = AFE_CONN43, .offset = 0},
    [O_44] = {.reg = -1, .offset = 0},
    [O_45] = {.reg = -1, .offset = 0},
    [O_46] = {.reg = -1, .offset = 0},
    [O_47] = {.reg = -1, .offset = 0},
    [O_48] = {.reg = -1, .offset = 0},
    [O_49] = {.reg = -1, .offset = 0},
};

/* register of Ix_Oy_S_H, x: 32 ~ 47, y: 0 ~ 49 */
/* bit position of Ix_Oy_S, x: 32 ~ 47, y: 0 ~ 49 */
static struct afe_interconn afe_intercon_H[] = {
    [O_00] = {.reg = -1,  .offset = 0},
    [O_01] = {.reg = -1,  .offset = 0},
    [O_02] = {.reg = -1,  .offset = 0},
    [O_03] = {.reg = -1,  .offset = 0},
    [O_04] = {.reg = AFE_CONN4_1,  .offset = 0},
    [O_05] = {.reg = AFE_CONN5_1,  .offset = 0},
    [O_06] = {.reg = -1,  .offset = 0},
    [O_07] = {.reg = -1,  .offset = 0},
    [O_08] = {.reg = -1,  .offset = 0},
    [O_09] = {.reg = -1,  .offset = 0},
    [O_10] = {.reg = -1, .offset = 0},
    [O_11] = {.reg = -1, .offset = 0},
    [O_12] = {.reg = AFE_CONN12_1, .offset = 0},
    [O_13] = {.reg = AFE_CONN13_1, .offset = 0},
    [O_14] = {.reg = AFE_CONN14_1, .offset = 0},
    [O_15] = {.reg = AFE_CONN15_1, .offset = 0},
    [O_16] = {.reg = AFE_CONN16_1, .offset = 0},
    [O_17] = {.reg = AFE_CONN17_1, .offset = 0},
    [O_18] = {.reg = AFE_CONN18_1, .offset = 0},
    [O_19] = {.reg = AFE_CONN19_1, .offset = 0},
    [O_20] = {.reg = AFE_CONN20_1, .offset = 0},
    [O_21] = {.reg = AFE_CONN21_1, .offset = 0},
    [O_22] = {.reg = AFE_CONN22_1, .offset = 0},
    [O_23] = {.reg = AFE_CONN23_1, .offset = 0},
    [O_24] = {.reg = -1, .offset = 0},
    [O_25] = {.reg = -1, .offset = 0},
    [O_26] = {.reg = AFE_CONN26_1, .offset = 0},
    [O_27] = {.reg = AFE_CONN27_1, .offset = 0},
    [O_28] = {.reg = AFE_CONN28_1, .offset = 0},
    [O_29] = {.reg = AFE_CONN29_1, .offset = 0},
    [O_30] = {.reg = AFE_CONN30_1, .offset = 0},
    [O_31] = {.reg = AFE_CONN31_1, .offset = 0},
    [O_32] = {.reg = AFE_CONN32_1, .offset = 0},
    [O_33] = {.reg = AFE_CONN33_1, .offset = 0},
    [O_34] = {.reg = AFE_CONN34_1, .offset = 0},
    [O_35] = {.reg = AFE_CONN35_1, .offset = 0},
    [O_36] = {.reg = AFE_CONN36_1, .offset = 0},
    [O_37] = {.reg = AFE_CONN37_1, .offset = 0},
    [O_38] = {.reg = AFE_CONN38_1, .offset = 0},
    [O_39] = {.reg = AFE_CONN39_1, .offset = 0},
    [O_40] = {.reg = AFE_CONN40_1, .offset = 0},
    [O_41] = {.reg = AFE_CONN41_1, .offset = 0},
    [O_42] = {.reg = AFE_CONN42_1, .offset = 0},
    [O_43] = {.reg = AFE_CONN43_1, .offset = 0},
    [O_44] = {.reg = -1, .offset = 0},
    [O_45] = {.reg = -1, .offset = 0},
    [O_46] = {.reg = -1, .offset = 0},
    [O_47] = {.reg = -1, .offset = 0},
    [O_48] = {.reg = -1, .offset = 0},
    [O_49] = {.reg = -1, .offset = 0},
};

int itrcon_connect(IO_IN_PORT in, IO_OUT_PORT out, int connect)
{
    struct afe_interconn *conn = NULL;
    unsigned int bit_pos;

    connect = !!connect;

    if ((out >= O_MAX) || (in >= I_MAX)) {
        aud_error("bad output(%d) or input(%d) port \n", out, in);
        return -EINVAL;
    }

    if (in <= I_31) {   /* Low */
        conn = &afe_intercon_L[out];
        bit_pos = in;
    } else if ((in >= I_32) && (in < I_MAX)) {  /* High */
        conn = &afe_intercon_H[out];
        bit_pos = in - I_32 + conn->offset;
    }

    aud_dbg("%s I_%d -> O_%d\n", connect ? "connect" : "disconnect", in, out);
    if (conn->reg == (unsigned int) -1) {
        aud_msg("skip output(%d)\n", out);
        return 0;
    }
    aud_drv_set_reg_addr_val_mask(AUDIO_REGMAP_AFE_BASE, conn->reg,
                                  connect << bit_pos, 0x1 << bit_pos);

    return 0;
}

static int get_widget_route(struct msd_ctl_value *value, void *priv)
{
    return 0;
}
static int set_widget_route(struct msd_ctl_value *value, void *priv)
{
    aud_dbg("src_id:%d, sink_id:%d, value:%d, event:%d",
            value->integer.value[0],
            value->integer.value[1],
            value->integer.value[2],
            value->integer.value[3]);
    IO_IN_PORT in = value->integer.value[0];
    IO_OUT_PORT out = value->integer.value[1];
    int connect = value->integer.value[2];

    mt7933_afe_enable_main_clk(priv);
    itrcon_connect(in, out, connect);
    mt7933_afe_disable_main_clk(priv);

    return !!connect;
}

#define IO_PORT_ENTRY(port) {#port, port}
#define IO_NAME(name) #name

static struct soc_ctl_entry io_route_ctl = {
    .name = "io_route_ctl",
    .get = get_widget_route,
    .set = set_widget_route,
};

static struct snd_widget mt7933_widget_list[] = {
    SWITCH_IO_WIDGET(I_00, &io_route_ctl),
    SWITCH_IO_WIDGET(I_01, &io_route_ctl),
    SWITCH_IO_WIDGET(I_02, &io_route_ctl),
    SWITCH_IO_WIDGET(I_03, &io_route_ctl),
    SWITCH_IO_WIDGET(I_04, &io_route_ctl),
    SWITCH_IO_WIDGET(I_05, &io_route_ctl),
    SWITCH_IO_WIDGET(I_06, &io_route_ctl),
    SWITCH_IO_WIDGET(I_07, &io_route_ctl),
    SWITCH_IO_WIDGET(I_08, &io_route_ctl),
    SWITCH_IO_WIDGET(I_09, &io_route_ctl),
    SWITCH_IO_WIDGET(I_10, &io_route_ctl),
    SWITCH_IO_WIDGET(I_11, &io_route_ctl),
    SWITCH_IO_WIDGET(I_12, &io_route_ctl),
    SWITCH_IO_WIDGET(I_13, &io_route_ctl),
    SWITCH_IO_WIDGET(I_14, &io_route_ctl),
    SWITCH_IO_WIDGET(I_15, &io_route_ctl),
    SWITCH_IO_WIDGET(I_16, &io_route_ctl),
    SWITCH_IO_WIDGET(I_17, &io_route_ctl),
    SWITCH_IO_WIDGET(I_18, &io_route_ctl),
    SWITCH_IO_WIDGET(I_19, &io_route_ctl),
    SWITCH_IO_WIDGET(I_20, &io_route_ctl),
    SWITCH_IO_WIDGET(I_21, &io_route_ctl),
    SWITCH_IO_WIDGET(I_22, &io_route_ctl),
    SWITCH_IO_WIDGET(I_23, &io_route_ctl),
    SWITCH_IO_WIDGET(I_24, &io_route_ctl),
    SWITCH_IO_WIDGET(I_25, &io_route_ctl),
    SWITCH_IO_WIDGET(I_26, &io_route_ctl),
    SWITCH_IO_WIDGET(I_27, &io_route_ctl),
    SWITCH_IO_WIDGET(I_28, &io_route_ctl),
    SWITCH_IO_WIDGET(I_29, &io_route_ctl),
    SWITCH_IO_WIDGET(I_30, &io_route_ctl),
    SWITCH_IO_WIDGET(I_31, &io_route_ctl),
    SWITCH_IO_WIDGET(I_32, &io_route_ctl),
    SWITCH_IO_WIDGET(I_33, &io_route_ctl),
    SWITCH_IO_WIDGET(I_34, &io_route_ctl),
    SWITCH_IO_WIDGET(I_35, &io_route_ctl),
    SWITCH_IO_WIDGET(I_36, &io_route_ctl),
    SWITCH_IO_WIDGET(I_37, &io_route_ctl),
    SWITCH_IO_WIDGET(I_38, &io_route_ctl),
    SWITCH_IO_WIDGET(I_39, &io_route_ctl),
    SWITCH_IO_WIDGET(I_40, &io_route_ctl),
    SWITCH_IO_WIDGET(I_41, &io_route_ctl),
    SWITCH_IO_WIDGET(I_42, &io_route_ctl),
    SWITCH_IO_WIDGET(I_43, &io_route_ctl),
    SWITCH_IO_WIDGET(I_44, &io_route_ctl),
    SWITCH_IO_WIDGET(I_45, &io_route_ctl),
    SWITCH_IO_WIDGET(I_46, &io_route_ctl),
    SWITCH_IO_WIDGET(I_47, &io_route_ctl),
    SWITCH_IO_WIDGET(I_48, &io_route_ctl),
    SWITCH_IO_WIDGET(I_49, &io_route_ctl),
    SWITCH_IO_WIDGET(I_50, &io_route_ctl),
    SWITCH_IO_WIDGET(I_51, &io_route_ctl),
    SWITCH_IO_WIDGET(I_52, &io_route_ctl),
    SWITCH_IO_WIDGET(I_53, &io_route_ctl),
    SWITCH_IO_WIDGET(I_54, &io_route_ctl),
    SWITCH_IO_WIDGET(I_55, &io_route_ctl),
    SWITCH_IO_WIDGET(I_56, &io_route_ctl),
    SWITCH_IO_WIDGET(I_57, &io_route_ctl),
    SWITCH_IO_WIDGET(I_58, &io_route_ctl),
    SWITCH_IO_WIDGET(I_59, &io_route_ctl),
    SWITCH_IO_WIDGET(I_60, &io_route_ctl),
    SWITCH_IO_WIDGET(I_61, &io_route_ctl),
    SWITCH_IO_WIDGET(O_00, NULL),
    SWITCH_IO_WIDGET(O_01, NULL),
    SWITCH_IO_WIDGET(O_02, NULL),
    SWITCH_IO_WIDGET(O_03, NULL),
    SWITCH_IO_WIDGET(O_04, NULL),
    SWITCH_IO_WIDGET(O_05, NULL),
    SWITCH_IO_WIDGET(O_06, NULL),
    SWITCH_IO_WIDGET(O_07, NULL),
    SWITCH_IO_WIDGET(O_08, NULL),
    SWITCH_IO_WIDGET(O_09, NULL),
    SWITCH_IO_WIDGET(O_10, NULL),
    SWITCH_IO_WIDGET(O_11, NULL),
    SWITCH_IO_WIDGET(O_12, NULL),
    SWITCH_IO_WIDGET(O_13, NULL),
    SWITCH_IO_WIDGET(O_14, NULL),
    SWITCH_IO_WIDGET(O_15, NULL),
    SWITCH_IO_WIDGET(O_16, NULL),
    SWITCH_IO_WIDGET(O_17, NULL),
    SWITCH_IO_WIDGET(O_18, NULL),
    SWITCH_IO_WIDGET(O_19, NULL),
    SWITCH_IO_WIDGET(O_20, NULL),
    SWITCH_IO_WIDGET(O_21, NULL),
    SWITCH_IO_WIDGET(O_22, NULL),
    SWITCH_IO_WIDGET(O_23, NULL),
    SWITCH_IO_WIDGET(O_24, NULL),
    SWITCH_IO_WIDGET(O_25, NULL),
    SWITCH_IO_WIDGET(O_26, NULL),
    SWITCH_IO_WIDGET(O_27, NULL),
    SWITCH_IO_WIDGET(O_28, NULL),
    SWITCH_IO_WIDGET(O_29, NULL),
    SWITCH_IO_WIDGET(O_30, NULL),
    SWITCH_IO_WIDGET(O_31, NULL),
    SWITCH_IO_WIDGET(O_32, NULL),
    SWITCH_IO_WIDGET(O_33, NULL),
    SWITCH_IO_WIDGET(O_34, NULL),
    SWITCH_IO_WIDGET(O_35, NULL),
    SWITCH_IO_WIDGET(O_36, NULL),
    SWITCH_IO_WIDGET(O_37, NULL),
    SWITCH_IO_WIDGET(O_38, NULL),
    SWITCH_IO_WIDGET(O_39, NULL),
    SWITCH_IO_WIDGET(O_40, NULL),
    SWITCH_IO_WIDGET(O_41, NULL),
    SWITCH_IO_WIDGET(O_42, NULL),
    SWITCH_IO_WIDGET(O_43, NULL),
    SWITCH_IO_WIDGET(O_44, NULL),
    SWITCH_IO_WIDGET(O_45, NULL),
    SWITCH_IO_WIDGET(O_46, NULL),
    SWITCH_IO_WIDGET(O_47, NULL),
    SWITCH_IO_WIDGET(O_48, NULL),
    SWITCH_IO_WIDGET(O_49, NULL),
    DAI_WIDGET("dlm-cpu-dai"),
    DAI_WIDGET("dl2-cpu-dai"),
    DAI_WIDGET("dl3-cpu-dai"),
    DAI_WIDGET("ul2-cpu-dai"),
    DAI_WIDGET("ul3-cpu-dai"),
    DAI_WIDGET("ul4-cpu-dai"),
    DAI_WIDGET("ul8-cpu-dai"),
    DAI_WIDGET("cm9-cpu-dai"),
    DAI_WIDGET("ul10-cpu-dai"),
    DAI_WIDGET("INT ADDA Playback"),
    DAI_WIDGET("INT ADDA Capture"),
    DAI_WIDGET("DMIC"),
    DAI_WIDGET("etdm1-in-cpu-dai"),
    DAI_WIDGET("etdm2-in-cpu-dai"),
    DAI_WIDGET("etdm2-out-cpu-dai"),
    DAI_WIDGET("GASRC0"),
    DAI_WIDGET("HW_GAIN1"),

};

static struct snd_widget_route mt7933_widget_routes[] = {
    {"dlm-cpu-dai", IO_NAME(I_22), 1, {NULL, NULL}},
    {"dlm-cpu-dai", IO_NAME(I_23), 1, {NULL, NULL}},

    {"dlm-cpu-dai", IO_NAME(I_24), 1, {NULL, NULL}},
    {"dlm-cpu-dai", IO_NAME(I_25), 1, {NULL, NULL}},
    {"dlm-cpu-dai", IO_NAME(I_26), 1, {NULL, NULL}},
    {"dlm-cpu-dai", IO_NAME(I_27), 1, {NULL, NULL}},
    {"dlm-cpu-dai", IO_NAME(I_28), 1, {NULL, NULL}},
    {"dlm-cpu-dai", IO_NAME(I_29), 1, {NULL, NULL}},

    {"dl2-cpu-dai", IO_NAME(I_40), 1, {NULL, NULL}},
    {"dl2-cpu-dai", IO_NAME(I_41), 1, {NULL, NULL}},

    {IO_NAME(I_40), IO_NAME(O_04), 0, {NULL, NULL}},
    {IO_NAME(I_41), IO_NAME(O_05), 0, {NULL, NULL}},
    {IO_NAME(I_40), IO_NAME(O_20), 0, {NULL, NULL}},
    {IO_NAME(I_41), IO_NAME(O_21), 0, {NULL, NULL}},
    {IO_NAME(I_40), IO_NAME(O_42), 0, {NULL, NULL}},
    {IO_NAME(I_41), IO_NAME(O_43), 0, {NULL, NULL}},

    {"dl3-cpu-dai", IO_NAME(I_20), 1, {NULL, NULL}},
    {"dl3-cpu-dai", IO_NAME(I_21), 1, {NULL, NULL}},

    {IO_NAME(I_20), IO_NAME(O_04), 0, {NULL, NULL}},
    {IO_NAME(I_21), IO_NAME(O_05), 0, {NULL, NULL}},
    {IO_NAME(I_20), IO_NAME(O_20), 0, {NULL, NULL}},
    {IO_NAME(I_21), IO_NAME(O_21), 0, {NULL, NULL}},
    {IO_NAME(I_20), IO_NAME(O_42), 0, {NULL, NULL}},
    {IO_NAME(I_21), IO_NAME(O_43), 0, {NULL, NULL}},

    { "INT ADDA Capture", IO_NAME(I_60), 1, {NULL, NULL}},
    { "INT ADDA Capture", IO_NAME(I_61), 1, {NULL, NULL}},
    { "INT ADDA Capture", IO_NAME(I_08), 1, {NULL, NULL}},
    { "INT ADDA Capture", IO_NAME(I_09), 1, {NULL, NULL}},
    { "INT ADDA Capture", IO_NAME(I_32), 1, {NULL, NULL}},
    { "INT ADDA Capture", IO_NAME(I_33), 1, {NULL, NULL}},

    { "DMIC", IO_NAME(I_04), 1, {NULL, NULL} },
    { "DMIC", IO_NAME(I_05), 1, {NULL, NULL} },
    { "DMIC", IO_NAME(I_06), 1, {NULL, NULL} },
    { "DMIC", IO_NAME(I_07), 1, {NULL, NULL} },

    { IO_NAME(O_26), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_27), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_28), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_29), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_30), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_31), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_32), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_33), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_34), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_35), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_36), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_37), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_38), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_39), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_40), "cm9-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_41), "cm9-cpu-dai", 1, {NULL, NULL} },

    { IO_NAME(I_60), IO_NAME(O_12), 0, {NULL, NULL} },
    { IO_NAME(I_61), IO_NAME(O_13), 0, {NULL, NULL} },
    { IO_NAME(I_60), IO_NAME(O_14), 0, {NULL, NULL} },
    { IO_NAME(I_61), IO_NAME(O_15), 0, {NULL, NULL} },
    { IO_NAME(I_60), IO_NAME(O_15), 0, {NULL, NULL} },
    { IO_NAME(I_08), IO_NAME(O_14), 0, {NULL, NULL} },

    { IO_NAME(I_08), IO_NAME(O_15), 0, {NULL, NULL} },

    { IO_NAME(I_12), IO_NAME(O_26), 0, {NULL, NULL} },
    { IO_NAME(I_13), IO_NAME(O_27), 0, {NULL, NULL} },

    { IO_NAME(I_60), IO_NAME(O_26), 0, {NULL, NULL} },
    { IO_NAME(I_61), IO_NAME(O_27), 0, {NULL, NULL} },
    { IO_NAME(I_60), IO_NAME(O_28), 0, {NULL, NULL} },
    { IO_NAME(I_61), IO_NAME(O_29), 0, {NULL, NULL} },
    { IO_NAME(I_22), IO_NAME(O_28), 0, {NULL, NULL} },
    { IO_NAME(I_23), IO_NAME(O_29), 0, {NULL, NULL} },
    { IO_NAME(I_32), IO_NAME(O_28), 0, {NULL, NULL} },
    { IO_NAME(I_33), IO_NAME(O_29), 0, {NULL, NULL} },

    { IO_NAME(I_42), IO_NAME(O_26), 0, {NULL, NULL} },
    { IO_NAME(I_43), IO_NAME(O_27), 0, {NULL, NULL} },
    { IO_NAME(I_44), IO_NAME(O_28), 0, {NULL, NULL} },
    { IO_NAME(I_45), IO_NAME(O_29), 0, {NULL, NULL} },
    { IO_NAME(I_46), IO_NAME(O_30), 0, {NULL, NULL} },
    { IO_NAME(I_47), IO_NAME(O_31), 0, {NULL, NULL} },
    { IO_NAME(I_48), IO_NAME(O_32), 0, {NULL, NULL} },
    { IO_NAME(I_49), IO_NAME(O_33), 0, {NULL, NULL} },

    { IO_NAME(I_04), IO_NAME(O_26), 0, {NULL, NULL} },
    { IO_NAME(I_05), IO_NAME(O_27), 0, {NULL, NULL} },
    { IO_NAME(I_06), IO_NAME(O_28), 0, {NULL, NULL} },
    { IO_NAME(I_07), IO_NAME(O_29), 0, {NULL, NULL} },
    { IO_NAME(I_08), IO_NAME(O_30), 0, {NULL, NULL} },
    { IO_NAME(I_09), IO_NAME(O_31), 0, {NULL, NULL} },
    { IO_NAME(I_10), IO_NAME(O_32), 0, {NULL, NULL} },
    { IO_NAME(I_11), IO_NAME(O_33), 0, {NULL, NULL} },
    { IO_NAME(I_04), IO_NAME(O_34), 0, {NULL, NULL} },
    { IO_NAME(I_05), IO_NAME(O_35), 0, {NULL, NULL} },
    { IO_NAME(I_06), IO_NAME(O_36), 0, {NULL, NULL} },
    { IO_NAME(I_07), IO_NAME(O_37), 0, {NULL, NULL} },
    { IO_NAME(I_08), IO_NAME(O_38), 0, {NULL, NULL} },
    { IO_NAME(I_09), IO_NAME(O_39), 0, {NULL, NULL} },
    { IO_NAME(I_08), IO_NAME(O_28), 0, {NULL, NULL} },
    { IO_NAME(I_09), IO_NAME(O_29), 0, {NULL, NULL} },
    { IO_NAME(I_10), IO_NAME(O_40), 0, {NULL, NULL} },
    { IO_NAME(I_11), IO_NAME(O_41), 0, {NULL, NULL} },

    { IO_NAME(I_08), IO_NAME(O_27), 0, {NULL, NULL} },

    { IO_NAME(O_14), "ul3-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_15), "ul3-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(I_32), IO_NAME(O_14), 0, {NULL, NULL} },
    { IO_NAME(I_33), IO_NAME(O_15), 0, {NULL, NULL} },

    { IO_NAME(O_16), "ul4-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(O_17), "ul4-cpu-dai", 1, {NULL, NULL} },
    { IO_NAME(I_60), IO_NAME(O_16), 0, {NULL, NULL} },
    { IO_NAME(I_61), IO_NAME(O_17), 0, {NULL, NULL} },
    { IO_NAME(I_32), IO_NAME(O_16), 0, {NULL, NULL} },
    { IO_NAME(I_33), IO_NAME(O_17), 0, {NULL, NULL} },
    { IO_NAME(I_12), IO_NAME(O_16), 0, {NULL, NULL} },
    { IO_NAME(I_13), IO_NAME(O_17), 0, {NULL, NULL} },
    { IO_NAME(I_04), IO_NAME(O_16), 0, {NULL, NULL} },
    { IO_NAME(I_05), IO_NAME(O_17), 0, {NULL, NULL} },

    { IO_NAME(I_22), IO_NAME(O_12), 0, {NULL, NULL} },
    { IO_NAME(I_23), IO_NAME(O_13), 0, {NULL, NULL} },

    { IO_NAME(O_12), "ul10-cpu-dai", 1, {NULL, NULL}},
    { IO_NAME(O_13), "ul10-cpu-dai", 1, {NULL, NULL}},

    {"GASRC0", IO_NAME(I_32), 1, {NULL, NULL}},
    {"GASRC0", IO_NAME(I_33), 1, {NULL, NULL}},
    { IO_NAME(I_22), IO_NAME(O_20), 0, {NULL, NULL} },
    { IO_NAME(I_23), IO_NAME(O_21), 0, {NULL, NULL} },
    { IO_NAME(I_22), IO_NAME(O_21), 0, {NULL, NULL} },
    { IO_NAME(I_23), IO_NAME(O_20), 0, {NULL, NULL} },
    { IO_NAME(I_22), IO_NAME(O_42), 0, {NULL, NULL} },
    { IO_NAME(I_23), IO_NAME(O_43), 0, {NULL, NULL} },
    { IO_NAME(I_32), IO_NAME(O_20), 0, {NULL, NULL} },
    { IO_NAME(I_33), IO_NAME(O_21), 0, {NULL, NULL} },

    { IO_NAME(O_20), "INT ADDA Playback", 1, {NULL, NULL} },
    { IO_NAME(O_21), "INT ADDA Playback", 1, {NULL, NULL} },
    { IO_NAME(O_42), "GASRC0", 1, {NULL, NULL} },
    { IO_NAME(O_43), "GASRC0", 1, {NULL, NULL} },

    {"HW_GAIN1", IO_NAME(I_30), 1, {NULL, NULL}},
    {"HW_GAIN1", IO_NAME(I_31), 1, {NULL, NULL}},
    { IO_NAME(O_22), "HW_GAIN1", 1, {NULL, NULL} },
    { IO_NAME(O_23), "HW_GAIN1", 1, {NULL, NULL} },
    {"etdm1-in-cpu-dai", "ul8-cpu-dai", 1, {NULL, NULL}},

    {"etdm1-in-cpu-dai", IO_NAME(I_42), 1, {NULL, NULL}},
    {"etdm1-in-cpu-dai", IO_NAME(I_43), 1, {NULL, NULL}},
    {"etdm1-in-cpu-dai", IO_NAME(I_44), 1, {NULL, NULL}},
    {"etdm1-in-cpu-dai", IO_NAME(I_45), 1, {NULL, NULL}},
    {"etdm1-in-cpu-dai", IO_NAME(I_46), 1, {NULL, NULL}},
    {"etdm1-in-cpu-dai", IO_NAME(I_47), 1, {NULL, NULL}},
    {"etdm1-in-cpu-dai", IO_NAME(I_48), 1, {NULL, NULL}},
    {"etdm1-in-cpu-dai", IO_NAME(I_49), 1, {NULL, NULL}},

    {IO_NAME(I_30), IO_NAME(O_26), 0, {NULL, NULL}},
    {IO_NAME(I_31), IO_NAME(O_27), 0, {NULL, NULL}},

    {"etdm2-in-cpu-dai", IO_NAME(I_12), 1, {NULL, NULL}},
    {"etdm2-in-cpu-dai", IO_NAME(I_13), 1, {NULL, NULL}},

    {IO_NAME(I_12), IO_NAME(O_26), 0, {NULL, NULL}},
    {IO_NAME(I_13), IO_NAME(O_27), 0, {NULL, NULL}},

    {IO_NAME(I_12), IO_NAME(O_34), 0, {NULL, NULL}},
    {IO_NAME(I_13), IO_NAME(O_35), 0, {NULL, NULL}},

    {IO_NAME(O_34), "ul2-cpu-dai", 1, {NULL, NULL}},
    {IO_NAME(O_35), "ul2-cpu-dai", 1, {NULL, NULL}},
    {IO_NAME(O_36), "ul2-cpu-dai", 1, {NULL, NULL}},
    {IO_NAME(O_37), "ul2-cpu-dai", 1, {NULL, NULL}},
    {IO_NAME(O_38), "ul2-cpu-dai", 1, {NULL, NULL}},
    {IO_NAME(O_39), "ul2-cpu-dai", 1, {NULL, NULL}},
    {IO_NAME(O_40), "ul2-cpu-dai", 1, {NULL, NULL}},
    {IO_NAME(O_41), "ul2-cpu-dai", 1, {NULL, NULL}},

    {IO_NAME(O_18), "ul2-cpu-dai", 1, {NULL, NULL}},
    {IO_NAME(O_19), "ul2-cpu-dai", 1, {NULL, NULL}},

    {IO_NAME(O_04), "etdm2-out-cpu-dai", 1, {NULL, NULL}},
    {IO_NAME(O_05), "etdm2-out-cpu-dai", 1, {NULL, NULL}},

    {IO_NAME(I_22), IO_NAME(O_04), 0, {NULL, NULL}},
    {IO_NAME(I_24), IO_NAME(O_04), 0, {NULL, NULL}},
    {IO_NAME(I_26), IO_NAME(O_04), 0, {NULL, NULL}},
    {IO_NAME(I_28), IO_NAME(O_04), 0, {NULL, NULL}},
    {IO_NAME(I_23), IO_NAME(O_05), 0, {NULL, NULL}},
    {IO_NAME(I_25), IO_NAME(O_05), 0, {NULL, NULL}},
    {IO_NAME(I_27), IO_NAME(O_05), 0, {NULL, NULL}},
    {IO_NAME(I_29), IO_NAME(O_05), 0, {NULL, NULL}},

    {IO_NAME(I_04), IO_NAME(O_14), 0, {NULL, NULL}},
    {IO_NAME(I_05), IO_NAME(O_15), 0, {NULL, NULL}},

    {IO_NAME(I_32), IO_NAME(O_22), 0, {NULL, NULL}},
    {IO_NAME(I_33), IO_NAME(O_23), 0, {NULL, NULL}},
    {IO_NAME(I_22), IO_NAME(O_22), 0, {NULL, NULL}},
    {IO_NAME(I_23), IO_NAME(O_23), 0, {NULL, NULL}},
    {IO_NAME(I_40), IO_NAME(O_22), 0, {NULL, NULL}},
    {IO_NAME(I_41), IO_NAME(O_23), 0, {NULL, NULL}},
    {IO_NAME(I_20), IO_NAME(O_22), 0, {NULL, NULL}},
    {IO_NAME(I_21), IO_NAME(O_23), 0, {NULL, NULL}},
    {IO_NAME(I_60), IO_NAME(O_22), 0, {NULL, NULL}},
    {IO_NAME(I_61), IO_NAME(O_23), 0, {NULL, NULL}},
    {IO_NAME(I_30), IO_NAME(O_20), 0, {NULL, NULL}},
    {IO_NAME(I_31), IO_NAME(O_21), 0, {NULL, NULL}},
    {IO_NAME(I_30), IO_NAME(O_04), 0, {NULL, NULL}},
    {IO_NAME(I_31), IO_NAME(O_05), 0, {NULL, NULL}},
    {IO_NAME(I_30), IO_NAME(O_42), 0, {NULL, NULL}},
    {IO_NAME(I_31), IO_NAME(O_43), 0, {NULL, NULL}},
    {IO_NAME(I_30), IO_NAME(O_12), 0, {NULL, NULL}},
    {IO_NAME(I_31), IO_NAME(O_13), 0, {NULL, NULL}},
    {IO_NAME(I_30), IO_NAME(O_14), 0, {NULL, NULL}},
    {IO_NAME(I_31), IO_NAME(O_15), 0, {NULL, NULL}},
    {IO_NAME(I_30), IO_NAME(O_16), 0, {NULL, NULL}},
    {IO_NAME(I_31), IO_NAME(O_17), 0, {NULL, NULL}},

    {IO_NAME(I_60), IO_NAME(O_42), 0, {NULL, NULL}},
    {IO_NAME(I_61), IO_NAME(O_43), 0, {NULL, NULL}},
    {IO_NAME(I_04), IO_NAME(O_12), 0, {NULL, NULL}},
    {IO_NAME(I_05), IO_NAME(O_13), 0, {NULL, NULL}},
    {IO_NAME(I_12), IO_NAME(O_12), 0, {NULL, NULL}},
    {IO_NAME(I_13), IO_NAME(O_13), 0, {NULL, NULL}},
    {IO_NAME(I_32), IO_NAME(O_12), 0, {NULL, NULL}},
    {IO_NAME(I_33), IO_NAME(O_13), 0, {NULL, NULL}},
    {IO_NAME(I_32), IO_NAME(O_26), 0, {NULL, NULL}},
    {IO_NAME(I_33), IO_NAME(O_27), 0, {NULL, NULL}},
};

enum sinegen_loopback_mode {
    SINEGEN_I00_I01 = 0,
    SINEGEN_I02_I03,
    SINEGEN_I04_I11,
    SINEGEN_I12_I19,
    SINEGEN_I20_I21,
    SINEGEN_I22_I29,
    SINEGEN_I30_I33,
    SINEGEN_I34_I35,
    SINEGEN_I36_I37,
    SINEGEN_I38_I39,
    SINEGEN_I40_I41,
    SINEGEN_I42_I57,
    SINEGEN_O00_O01,
    SINEGEN_O02_O03,
    SINEGEN_O04_O11,
    SINEGEN_O12_O13,
    SINEGEN_O14_O15,
    SINEGEN_O16_O17,
    SINEGEN_O20_O21,
    SINEGEN_O42_O43,
    SINEGEN_O44_O45,
    SINEGEN_O46_O47,
    SINEGEN_O48_O49,
    SINEGEN_O26_O41,
    SINEGEN_UL9,
    SINEGEN_UL2,
    SINEGEN_I60_I61 = 28,
    SINEGEN_I58_I59,
    SINEGEN_NONE,
};

enum sinegen_timing {
    SINEGEN_8K = 0,
    SINEGEN_12K,
    SINEGEN_16K,
    SINEGEN_24K,
    SINEGEN_32K,
    SINEGEN_48K,
    SINEGEN_96K,
    SINEGEN_192K,
    SINEGEN_384K,
    SINEGEN_7P35K,
    SINEGEN_11P025K,
    SINEGEN_14P7K,
    SINEGEN_22P05K,
    SINEGEN_29P4K,
    SINEGEN_44P1K,
    SINEGEN_88P2K,
    SINEGEN_176P4K,
    SINEGEN_352P8K,
    SINEGEN_DL_1X_EN,
    SINEGEN_SGEN_EN,
};

static int mt7933_afe_singen_enable_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val = 0;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0);

    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] = (val & AFE_SINEGEN_CON0_EN) >> 26;

    return 0;
}


static int mt7933_afe_singen_enable_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    mt7933_afe_enable_main_clk(afe);

    if (ctl_value->integer.value[0]) {
        mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_TML);

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0,
                                      AFE_SINEGEN_CON0_EN,
                                      AFE_SINEGEN_CON0_EN);
    } else {
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0,
                                      AFE_SINEGEN_CON0_EN,
                                      0x0);

        mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_TML);
    }

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

static int mt7933_afe_sinegen_loopback_mode_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int mode;
    unsigned int val;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0);

    mt7933_afe_disable_main_clk(afe);

    mode = (val & AFE_SINEGEN_CON0_MODE_MASK) >> 27;

    if (mode >= SINEGEN_NONE)
        mode = SINEGEN_NONE;

    ctl_value->integer.value[0] = mode;

    return 0;
}

static int mt7933_afe_sinegen_loopback_mode_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int mode;
    unsigned int val;

    mode = ctl_value->integer.value[0];

    val = (mode << 27) & AFE_SINEGEN_CON0_MODE_MASK;

    mt7933_afe_enable_main_clk(afe);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0,
                                  AFE_SINEGEN_CON0_MODE_MASK, val);

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

static int mt7933_afe_sinegen_timing_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int timing;
    unsigned int val;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON1);

    mt7933_afe_disable_main_clk(afe);

    val = (val & AFE_SINEGEN_CON1_TIMING_CH1_MASK) >> 16;

    switch (val) {
        case AFE_SINEGEN_CON1_TIMING_8K:
            timing = SINEGEN_8K;
            break;
        case AFE_SINEGEN_CON1_TIMING_12K:
            timing = SINEGEN_12K;
            break;
        case AFE_SINEGEN_CON1_TIMING_16K:
            timing = SINEGEN_16K;
            break;
        case AFE_SINEGEN_CON1_TIMING_24K:
            timing = SINEGEN_24K;
            break;
        case AFE_SINEGEN_CON1_TIMING_32K:
            timing = SINEGEN_32K;
            break;
        case AFE_SINEGEN_CON1_TIMING_48K:
            timing = SINEGEN_48K;
            break;
        case AFE_SINEGEN_CON1_TIMING_96K:
            timing = SINEGEN_96K;
            break;
        case AFE_SINEGEN_CON1_TIMING_192K:
            timing = SINEGEN_192K;
            break;
        case AFE_SINEGEN_CON1_TIMING_384K:
            timing = SINEGEN_384K;
            break;
        case AFE_SINEGEN_CON1_TIMING_7P35K:
            timing = SINEGEN_7P35K;
            break;
        case AFE_SINEGEN_CON1_TIMING_11P025K:
            timing = SINEGEN_11P025K;
            break;
        case AFE_SINEGEN_CON1_TIMING_14P7K:
            timing = SINEGEN_14P7K;
            break;
        case AFE_SINEGEN_CON1_TIMING_22P05K:
            timing = SINEGEN_22P05K;
            break;
        case AFE_SINEGEN_CON1_TIMING_29P4K:
            timing = SINEGEN_29P4K;
            break;
        case AFE_SINEGEN_CON1_TIMING_44P1K:
            timing = SINEGEN_44P1K;
            break;
        case AFE_SINEGEN_CON1_TIMING_88P2K:
            timing = SINEGEN_88P2K;
            break;
        case AFE_SINEGEN_CON1_TIMING_176P4K:
            timing = SINEGEN_176P4K;
            break;
        case AFE_SINEGEN_CON1_TIMING_352P8K:
            timing = SINEGEN_352P8K;
            break;
        case AFE_SINEGEN_CON1_TIMING_DL_1X_EN:
            timing = SINEGEN_DL_1X_EN;
            break;
        case AFE_SINEGEN_CON1_TIMING_SGEN_EN:
            timing = SINEGEN_SGEN_EN;
            break;
        default:
            timing = SINEGEN_8K;
            break;
    }

    ctl_value->integer.value[0] = timing;

    return 0;
}

static int mt7933_afe_sinegen_timing_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int timing;
    unsigned int val;

    timing = ctl_value->integer.value[0];

    switch (timing) {
        case SINEGEN_8K:
            val = AFE_SINEGEN_CON1_TIMING_8K;
            break;
        case SINEGEN_12K:
            val = AFE_SINEGEN_CON1_TIMING_12K;
            break;
        case SINEGEN_16K:
            val = AFE_SINEGEN_CON1_TIMING_16K;
            break;
        case SINEGEN_24K:
            val = AFE_SINEGEN_CON1_TIMING_24K;
            break;
        case SINEGEN_32K:
            val = AFE_SINEGEN_CON1_TIMING_32K;
            break;
        case SINEGEN_48K:
            val = AFE_SINEGEN_CON1_TIMING_48K;
            break;
        case SINEGEN_96K:
            val = AFE_SINEGEN_CON1_TIMING_96K;
            break;
        case SINEGEN_192K:
            val = AFE_SINEGEN_CON1_TIMING_192K;
            break;
        case SINEGEN_384K:
            val = AFE_SINEGEN_CON1_TIMING_384K;
            break;
        case SINEGEN_7P35K:
            val = AFE_SINEGEN_CON1_TIMING_7P35K;
            break;
        case SINEGEN_11P025K:
            val = AFE_SINEGEN_CON1_TIMING_11P025K;
            break;
        case SINEGEN_14P7K:
            val = AFE_SINEGEN_CON1_TIMING_14P7K;
            break;
        case SINEGEN_22P05K:
            val = AFE_SINEGEN_CON1_TIMING_22P05K;
            break;
        case SINEGEN_29P4K:
            val = AFE_SINEGEN_CON1_TIMING_29P4K;
            break;
        case SINEGEN_44P1K:
            val = AFE_SINEGEN_CON1_TIMING_44P1K;
            break;
        case SINEGEN_88P2K:
            val = AFE_SINEGEN_CON1_TIMING_88P2K;
            break;
        case SINEGEN_176P4K:
            val = AFE_SINEGEN_CON1_TIMING_176P4K;
            break;
        case SINEGEN_352P8K:
            val = AFE_SINEGEN_CON1_TIMING_352P8K;
            break;
        case SINEGEN_DL_1X_EN:
            val = AFE_SINEGEN_CON1_TIMING_DL_1X_EN;
            break;
        case SINEGEN_SGEN_EN:
            val = AFE_SINEGEN_CON1_TIMING_SGEN_EN;
            break;
        default:
            val = AFE_SINEGEN_CON1_TIMING_8K;
            break;
    }

    mt7933_afe_enable_main_clk(afe);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON1,
                                  AFE_SINEGEN_CON1_TIMING_CH1_MASK |
                                  AFE_SINEGEN_CON1_TIMING_CH2_MASK,
                                  AFE_SINEGEN_CON1_TIMING_CH1(val) |
                                  AFE_SINEGEN_CON1_TIMING_CH2(val));

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

#if 0
static int mt7933_afe_ul8_sinegen_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val = 0;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON);

    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] = (val & ASYS_TCON_UL8_USE_SINEGEN);

    return 0;
}

static int mt7933_afe_ul8_sinegen_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    mt7933_afe_enable_main_clk(afe);

    if (ctl_value->integer.value[0])
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON,
                                      ASYS_TCON_UL8_USE_SINEGEN,
                                      ASYS_TCON_UL8_USE_SINEGEN);
    else
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON,
                                      ASYS_TCON_UL8_USE_SINEGEN,
                                      0x0);

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

static int mt7933_afe_dmic_sinegen_enable_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val = 0;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_DMIC0_UL_SRC_CON1);

    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] =
        (val & DMIC_UL_CON1_SGEN_EN) ? 1 : 0;

    return 0;
}

static int mt7933_afe_dmic_sinegen_enable_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    unsigned int val_cfg = 0;
    unsigned int val_en = 0;
    unsigned int read_val = 0;
    unsigned int regs[] = {
        AFE_DMIC0_UL_SRC_CON1,
        AFE_DMIC1_UL_SRC_CON1,
        AFE_DMIC2_UL_SRC_CON1,
        AFE_DMIC3_UL_SRC_CON1,
    };
    int i;

    mt7933_afe_enable_main_clk(afe);

    val_cfg |= DMIC_UL_CON1_SGEN_CH2_AMP_DIV(6) |
               DMIC_UL_CON1_SGEN_CH2_FREQ_DIV(1) |
               DMIC_UL_CON1_SGEN_CH1_AMP_DIV(6) |
               DMIC_UL_CON1_SGEN_CH1_FREQ_DIV(1);

    read_val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_DMIC0_UL_SRC_CON0);

    switch (read_val & (0x7 << 17)) {
        case DMIC_UL_CON0_VOCIE_MODE_8K:
            val_cfg |= DMIC_UL_CON1_SGEN_CH2_SINE_MODE(0) |
                       DMIC_UL_CON1_SGEN_CH1_SINE_MODE(0);
            break;
        case DMIC_UL_CON0_VOCIE_MODE_16K:
            val_cfg |= DMIC_UL_CON1_SGEN_CH2_SINE_MODE(3) |
                       DMIC_UL_CON1_SGEN_CH1_SINE_MODE(3);
            break;
        case DMIC_UL_CON0_VOCIE_MODE_32K:
            val_cfg |= DMIC_UL_CON1_SGEN_CH2_SINE_MODE(6) |
                       DMIC_UL_CON1_SGEN_CH1_SINE_MODE(6);
            break;
        case DMIC_UL_CON0_VOCIE_MODE_48K:
            val_cfg |= DMIC_UL_CON1_SGEN_CH2_SINE_MODE(8) |
                       DMIC_UL_CON1_SGEN_CH1_SINE_MODE(8);
            break;
        default:
            val_cfg |= DMIC_UL_CON1_SGEN_CH2_SINE_MODE(0) |
                       DMIC_UL_CON1_SGEN_CH1_SINE_MODE(0);
            break;
    }

    val_en |= DMIC_UL_CON1_SGEN_EN;

    if (ctl_value->integer.value[0]) {
        for (i = 0; i < ARRAY_SIZE(regs); i++)
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, regs[i],
                                          val_cfg | val_en,
                                          val_cfg | val_en);
    } else {
        for (i = 0; i < ARRAY_SIZE(regs); i++)
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, regs[i],
                                          val_en,
                                          0x0);
    }

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

static int mt7933_afe_gasrc_in_sinegen_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    unsigned int val = 0;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON1);

    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] =
        (val & AFE_SINEGEN_CON1_GASRC_IN_SGEN);

    return 0;
}

static int mt7933_afe_gasrc_in_sinegen_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    mt7933_afe_enable_main_clk(afe);

    if (ctl_value->integer.value[0])
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON1,
                                      AFE_SINEGEN_CON1_GASRC_IN_SGEN,
                                      AFE_SINEGEN_CON1_GASRC_IN_SGEN);
    else
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON1,
                                      AFE_SINEGEN_CON1_GASRC_IN_SGEN,
                                      0x0);

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

static int mt7933_afe_gasrc_out_sinegen_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val = 0;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON1);


    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] =
        (val & AFE_SINEGEN_CON1_GASRC_OUT_SGEN);

    return 0;
}

static int mt7933_afe_gasrc_out_sinegen_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    mt7933_afe_enable_main_clk(afe);

    if (ctl_value->integer.value[0])
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON1,
                                      AFE_SINEGEN_CON1_GASRC_OUT_SGEN,
                                      AFE_SINEGEN_CON1_GASRC_OUT_SGEN);
    else
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON1,
                                      AFE_SINEGEN_CON1_GASRC_OUT_SGEN,
                                      0x0);

    mt7933_afe_disable_main_clk(afe);

    return 0;
}
#endif /* #if 0 */

static int mt7933_afe_sinegen_amp_ch1_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val = 0;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0);

    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] = (val & GENMASK(7, 5)) >> 5;

    return 0;
}

static int mt7933_afe_sinegen_amp_ch1_set(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    mt7933_afe_enable_main_clk(afe);

    unsigned int val = ctl_value->integer.value[0];
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0,
                                  GENMASK(7, 5),
                                  val << 5);

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

static int mt7933_afe_sinegen_amp_ch2_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val = 0;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0);

    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] = (val & GENMASK(19, 17)) >> 17;

    return 0;
}

static int mt7933_afe_sinegen_amp_ch2_set(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    mt7933_afe_enable_main_clk(afe);

    unsigned int val = ctl_value->integer.value[0];
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0,
                                  GENMASK(19, 17),
                                  val << 17);

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

static int mt7933_afe_sinegen_freq_ch2_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val = 0;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0);

    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] = (val & GENMASK(16, 12)) >> 12;

    return 0;
}

static int mt7933_afe_sinegen_freq_ch2_set(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    mt7933_afe_enable_main_clk(afe);

    unsigned int val = ctl_value->integer.value[0];
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0,
                                  GENMASK(16, 12),
                                  val << 12);

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

static int mt7933_afe_reg_dump_get(struct msd_ctl_value *ctl_value, void *priv)
{
    ctl_value->integer.value[0] = 0;

    return 0;
}

static int mt7933_afe_reg_dump_set(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    mt7933_afe_enable_main_clk(afe);

    mtk_afe_fe_reg_dump_msg(memif_dump_regs, ARRAY_SIZE(memif_dump_regs));
    mtk_afe_fe_reg_dump_msg(irq_dump_regs, ARRAY_SIZE(irq_dump_regs));
    mtk_afe_fe_reg_dump_msg(conn_dump_regs, ARRAY_SIZE(conn_dump_regs));
    mtk_afe_fe_reg_dump_msg(adda_dump_regs, ARRAY_SIZE(adda_dump_regs));
    mtk_afe_fe_reg_dump_msg(gasrc_dump_regs, ARRAY_SIZE(gasrc_dump_regs));
    mtk_afe_fe_reg_dump_msg(etdm_dump_regs, ARRAY_SIZE(etdm_dump_regs));

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

static int mt7933_afe_sinegen_freq_ch1_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val = 0;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0);

    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] = (val & GENMASK(4, 0));

    return 0;
}

static int mt7933_afe_sinegen_freq_ch1_set(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;

    mt7933_afe_enable_main_clk(afe);

    unsigned int val = ctl_value->integer.value[0];
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_SINEGEN_CON0,
                                  GENMASK(4, 0),
                                  val);

    mt7933_afe_disable_main_clk(afe);

    return 0;
}

//static int connect_io_port(int src, int sink, int enable)
//{
//  return 0;
//}

static int get_io_port_route(struct msd_ctl_value *value, void *priv)
{
    aud_msg("done");
    return 0;
}

static int set_io_port_route(struct msd_ctl_value *value, void *priv)
{
    aud_msg("done");
    return 0;
}

static int mt7933_afe_hw_gain1_vol_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val = 0;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_GAIN1_CON1);

    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] = val & AFE_GAIN1_CON1_MASK;

    return 0;
}

static int mt7933_afe_hw_gain1_vol_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val = ctl_value->integer.value[0];

    mt7933_afe_enable_main_clk(afe);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  AFE_GAIN1_CON1,
                                  AFE_GAIN1_CON1_MASK,
                                  val);

    mt7933_afe_disable_main_clk(afe);
    return 0;
}

static int mt7933_afe_hw_gain1_sampleperstep_get(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val;

    mt7933_afe_enable_main_clk(afe);

    val = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE, AFE_GAIN1_CON0);

    mt7933_afe_disable_main_clk(afe);

    ctl_value->integer.value[0] = (val & AFE_GAIN1_CON0_SAMPLE_PER_STEP_MASK) >> 8;

    return 0;
}

static int mt7933_afe_hw_gain1_sampleperstep_put(struct msd_ctl_value *ctl_value, void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    unsigned int val;

    val = ctl_value->integer.value[0];

    mt7933_afe_enable_main_clk(afe);

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  AFE_GAIN1_CON0,
                                  AFE_GAIN1_CON0_SAMPLE_PER_STEP_MASK,
                                  AFE_GAIN1_CON0_SAMPLE_PER_STEP_VAL(val));

    mt7933_afe_disable_main_clk(afe);
    return 0;
}

static struct soc_ctl_entry mt7933_afe_controls[] = {
    {
        .name = "io_route_ctl",
        .get = get_io_port_route,
        .set = set_io_port_route,
    },
    {
        .name = "SineGen_Enable_Switch",
        .get = mt7933_afe_singen_enable_get,
        .set = mt7933_afe_singen_enable_put,
    },
    {
        .name = "SineGen_Loopback_Mode_Select",
        .get = mt7933_afe_sinegen_loopback_mode_get,
        .set = mt7933_afe_sinegen_loopback_mode_put,
    },
    {
        .name = "SineGen_Timing_Select",
        .get = mt7933_afe_sinegen_timing_get,
        .set = mt7933_afe_sinegen_timing_put,
    },
    {
        .name = "SineGen_Amp_Div_Ch1",
        .get = mt7933_afe_sinegen_amp_ch1_get,
        .set = mt7933_afe_sinegen_amp_ch1_set,
    },
    {
        .name = "SineGen_Amp_Div_Ch2",
        .get = mt7933_afe_sinegen_amp_ch2_get,
        .set = mt7933_afe_sinegen_amp_ch2_set,
    },
    {
        .name = "SineGen_Freq_Div_Ch1",
        .get = mt7933_afe_sinegen_freq_ch1_get,
        .set = mt7933_afe_sinegen_freq_ch1_set,
    },
    {
        .name = "SineGen_Freq_Div_Ch2",
        .get = mt7933_afe_sinegen_freq_ch2_get,
        .set = mt7933_afe_sinegen_freq_ch2_set,
    },
    {
        .name = "Audio_AFE_Reg_Dump",
        .get = mt7933_afe_reg_dump_get,
        .set = mt7933_afe_reg_dump_set,
    },
    {
        .name = "HW_Gain1_Volume",
        .get = mt7933_afe_hw_gain1_vol_get,
        .set = mt7933_afe_hw_gain1_vol_put,
    },
    {
        .name = "HW Gain1 SamplePerStep",
        .get = mt7933_afe_hw_gain1_sampleperstep_get,
        .set = mt7933_afe_hw_gain1_sampleperstep_put,
    },
    //  {
    //      .name = "UL8_SineGen_Select",
    //      .get = mt7933_afe_ul8_sinegen_get,
    //      .set = mt7933_afe_ul8_sinegen_put,
    //  },
    //  {
    //      .name = "DMIC_SineGen_Enable_Switch",
    //      .get = mt7933_afe_dmic_sinegen_enable_get,
    //      .set = mt7933_afe_dmic_sinegen_enable_put,
    //  },
    //  {
    //      .name = "GASRC_In_SineGen_Select",
    //      .get = mt7933_afe_gasrc_in_sinegen_get,
    //      .set = mt7933_afe_gasrc_in_sinegen_put,
    //  },
    //  {
    //      .name = "GASRC_Out_SineGen_Select",
    //      .get = mt7933_afe_gasrc_out_sinegen_get,
    //      .set = mt7933_afe_gasrc_out_sinegen_put,
    //  },
};

static void mt7933_afe_irq_handler(hal_nvic_irq_t irq)
{
    struct mtk_base_afe *afe = mt7933_afe;
    uint32_t val = 0;
    unsigned int asys_irq_clr_bits = 0;
    unsigned int afe_irq_clr_bits = 0;
    unsigned int irq_status_bits;
    unsigned int irq_clr_bits;
    uint32_t mcu_irq_mask;
    int i, ret;

    //  aud_msg("");

    //ret = regmap_read(afe->regmap, AFE_IRQ_STATUS, &val);
    ret = aud_drv_read_reg(AUDIO_REGMAP_AFE_BASE, AFE_IRQ_STATUS, &val);
    if (ret) {
        aud_error("irq status err");
        afe_irq_clr_bits = AFE_IRQ_MCU_CLR_BITS;
        asys_irq_clr_bits = ASYS_IRQ_CLR_BITS;
        goto err_irq;
    }

    //ret = regmap_read(afe->regmap, AFE_IRQ_MASK, &mcu_irq_mask);
    ret = aud_drv_read_reg(AUDIO_REGMAP_AFE_BASE, AFE_IRQ_MASK, &mcu_irq_mask);
    if (ret) {
        aud_error("read irq mask err");
        afe_irq_clr_bits = AFE_IRQ_MCU_CLR_BITS;
        asys_irq_clr_bits = ASYS_IRQ_CLR_BITS;
        goto err_irq;
    }

    /* only clr cpu irq */
    val &= mcu_irq_mask;

    for (i = 0; i < MT7933_AFE_MEMIF_NUM; i++) {
        struct mtk_base_afe_memif *memif = &afe->memif[i];
        struct mtk_base_irq_data const *irq_data;

        if (memif->irq_usage < 0)
            continue;

        irq_data = afe->irqs[memif->irq_usage].irq_data;

        irq_status_bits = BIT(irq_data->irq_status_shift);
        irq_clr_bits = BIT(irq_data->irq_clr_shift);

        if (!(val & irq_status_bits))
            continue;

        if (irq_data->irq_clr_reg == ASYS_IRQ_CLR)
            asys_irq_clr_bits |= irq_clr_bits;
        else
            afe_irq_clr_bits |= irq_clr_bits;

        if (irq_data->custom_handler)
            irq_data->custom_handler(irq, (void *)afe);
        else
            snd_pcm_period_elapsed(memif->stream, 1);
    }

    //  for (i = 0; i < ARRAY_SIZE(aux_irqs); i++) {
    //      struct mtk_base_irq_data const *irq_data;
    //      unsigned int irq_id = aux_irqs[i];

    //      irq_data = afe->irqs[irq_id].irq_data;

    //      irq_status_bits = BIT(irq_data->irq_status_shift);
    //      irq_clr_bits = BIT(irq_data->irq_clr_shift);

    //      if (!(val & irq_status_bits))
    //          continue;

    //      if (irq_data->irq_clr_reg == ASYS_IRQ_CLR)
    //          asys_irq_clr_bits |= irq_clr_bits;
    //      else
    //          afe_irq_clr_bits |= irq_clr_bits;

    //      if (irq_data->custom_handler)
    //          irq_data->custom_handler(irq, (void *)afe);
    //  }

err_irq:
    /* clear irq */
    if (asys_irq_clr_bits)
        aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, ASYS_IRQ_CLR, asys_irq_clr_bits);
    //regmap_write(afe->regmap, ASYS_IRQ_CLR, asys_irq_clr_bits);
    if (afe_irq_clr_bits)
        aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, AFE_IRQ_MCU_CLR, afe_irq_clr_bits);
    //regmap_write(afe->regmap, AFE_IRQ_MCU_CLR, afe_irq_clr_bits);

    return;
}

/*====================  For ADSP ======================*/
#ifdef CONFIG_MTK_ADSP_SUPPORT

struct mtk_base_afe *mt7933_afe_pcm_get_info(void)
{
    return g_priv;
}

static void mt7933_adsp_get_afe_memif_sram(struct mtk_base_afe *afe,
                                           int memif_id,
                                           unsigned int *paddr,
                                           unsigned int *size)
{
    if (memif_id == MT7933_AFE_MEMIF_UL9) {
        *paddr = AFE_SRAM_BASE;
        *size = AFE_SRAM_SIZE;
    } else if (memif_id == MT7933_AFE_MEMIF_UL2) {
        *paddr = 0;
        *size = 0;
    } else if (memif_id == MT7933_AFE_MEMIF_DLM) {
        *paddr = AFE_SRAM_BASE;
        *size = AFE_SRAM_SIZE;
    } else if (memif_id == MT7933_AFE_MEMIF_DL2) {
        *paddr = AFE_SRAM_BASE;
        *size = AFE_SRAM_SIZE;
    } else if (memif_id == MT7933_AFE_MEMIF_DL3) {
        *paddr = AFE_SRAM_BASE;
        *size = AFE_SRAM_SIZE;
    }
}

static int mt7933_adsp_set_afe_memif(struct mtk_base_afe *afe,
                                     int memif_id,
                                     unsigned int rate,
                                     unsigned int channels,
                                     unsigned int bitwidth)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mtk_base_afe_memif *memif = &afe->memif[memif_id];
    struct mt7933_control_data *ctrl_data = &afe_priv->ctrl_data;
    int fs, hd_audio = 0;

    if (memif_id == MT7933_AFE_MEMIF_UL2) {
        unsigned int val = 0;

        if (!ctrl_data->bypass_cm1) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON,
                                          ASYS_TCON_O34_O41_1X_EN_MASK,
                                          ASYS_TCON_O34_O41_1X_EN_UL2);

            val |= UL_REORDER_START_DATA(8) |
                   UL_REORDER_CHANNEL(channels) |
                   UL_REORDER_NO_BYPASS;
        }

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_I2S_UL2_REORDER,
                                      UL_REORDER_CTRL_MASK, val);
    } else if (memif_id == MT7933_AFE_MEMIF_UL9) {
        unsigned int val = 0;

        if (!ctrl_data->bypass_cm0) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON,
                                          ASYS_TCON_O26_O33_1X_EN_MASK,
                                          ASYS_TCON_O26_O33_1X_EN_UL9);

            if (channels > 8)
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, ASYS_TOP_CON,
                                              ASYS_TCON_O34_O41_1X_EN_MASK,
                                              ASYS_TCON_O34_O41_1X_EN_UL9);

            val |= UL_REORDER_START_DATA(0) |
                   UL_REORDER_CHANNEL(channels) |
                   UL_REORDER_NO_BYPASS;
        }

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, AFE_I2S_UL9_REORDER,
                                      UL_REORDER_CTRL_MASK, val);
    }

    aud_msg("phys_buf_addr = 0x%x", memif->phys_buf_addr);
    aud_msg("buffer_size = 0x%x", memif->buffer_size);

    /* start */
    aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, memif->data->reg_ofs_base,
                             memif->phys_buf_addr);
    /* end */
    aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE,
                             memif->data->reg_ofs_base + AFE_BASE_END_OFFSET,
                             memif->phys_buf_addr + memif->buffer_size - 1 +
                             memif->data->buffer_end_shift);

    /* set MSB to 33-bit, fix 33-bit to 0 for adsp */
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->msb_reg,
                                  1 << memif->data->msb_shift,
                                  0 << memif->data->msb_shift);

    /* set channel */
    if (memif->data->mono_shift >= 0) {
        unsigned int mono = (channels == 1) ? 1 : 0;

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->mono_reg,
                                      1 << memif->data->mono_shift,
                                      mono << memif->data->mono_shift);
    }

    /* set rate */
    if (memif->data->fs_shift < 0)
        return 0;

    fs = mt7933_afe_fs_timing(rate);

    if (fs < 0)
        return -EINVAL;

    if (memif->data->id == MT7933_AFE_MEMIF_UL8)
        fs = MT7933_FS_ETDMIN1_NX_EN;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->fs_reg,
                                  memif->data->fs_maskbit << memif->data->fs_shift,
                                  fs << memif->data->fs_shift);

    /* set hd mode */
    if (bitwidth > 16)
        hd_audio = 1;
    else
        hd_audio = 0;

    if (memif->data->hd_reg >= 0)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->hd_reg,
                                      1 << memif->data->hd_shift,
                                      hd_audio << memif->data->hd_shift);

    mt7933_afe_irq_direction_enable(afe,
                                    memif->irq_usage,
                                    MT7933_AFE_IRQ_DIR_DSP);

    return 0;
}

static int mt7933_adsp_set_afe_memif_enable(struct mtk_base_afe *afe,
                                            int memif_id,
                                            unsigned int rate,
                                            unsigned int period_size,
                                            int enable)
{
    struct mtk_base_afe_memif *memif = &afe->memif[memif_id];
    struct mtk_base_afe_irq *irqs = &afe->irqs[memif->irq_usage];
    const struct mtk_base_irq_data *irq_data = irqs->irq_data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_control_data *ctrl_data = &afe_priv->ctrl_data;
    unsigned int counter = period_size;
    int fs;

    aud_msg("%s memif %d %s\n", __func__, memif_id, enable ? "enable" : "disable");
    aud_msg("irq_data->id = %d, irq_data->irq_cnt_reg = 0x%x\n", irq_data->id, irq_data->irq_cnt_reg);


    /* TODO IRQ which side? */
    if (enable) {
        if (memif->data->agent_disable_shift >= 0)
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                          memif->data->agent_disable_reg,
                                          1 << memif->data->agent_disable_shift,
                                          0 << memif->data->agent_disable_shift);

        /* enable channel merge */
        if (memif_id == MT7933_AFE_MEMIF_UL2 &&
            !ctrl_data->bypass_cm1) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                          AFE_I2S_UL2_REORDER,
                                          UL_REORDER_EN, UL_REORDER_EN);
        } else if (memif_id == MT7933_AFE_MEMIF_UL9 &&
                   !ctrl_data->bypass_cm0) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                          AFE_I2S_UL9_REORDER,
                                          UL_REORDER_EN, UL_REORDER_EN);
        }

        if (memif->data->enable_shift >= 0)
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                          memif->data->enable_reg,
                                          1 << memif->data->enable_shift,
                                          1 << memif->data->enable_shift);
        aud_msg("[Yajun Test] counter = %d\n", counter);
        /* set irq counter */
        if (irq_data->irq_cnt_reg >= 0)
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                          irq_data->irq_cnt_reg,
                                          irq_data->irq_cnt_maskbit
                                          << irq_data->irq_cnt_shift,
                                          counter << irq_data->irq_cnt_shift);

        /* set irq fs */
        fs = mt7933_afe_fs_timing(rate);

        if (fs < 0)
            return -EINVAL;

        if (irq_data->irq_fs_reg >= 0)
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                          irq_data->irq_fs_reg,
                                          irq_data->irq_fs_maskbit << irq_data->irq_fs_shift,
                                          fs << irq_data->irq_fs_shift);

        /* enable interrupt */
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, irq_data->irq_en_reg,
                                      1 << irq_data->irq_en_shift,
                                      1 << irq_data->irq_en_shift);
    } else {
        if (memif_id == MT7933_AFE_MEMIF_UL2 &&
            !ctrl_data->bypass_cm1) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                          AFE_I2S_UL2_REORDER,
                                          UL_REORDER_EN, 0x0);
        } else if (memif_id == MT7933_AFE_MEMIF_UL9 &&
                   !ctrl_data->bypass_cm0) {
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                          AFE_I2S_UL9_REORDER,
                                          UL_REORDER_EN, 0x0);
        }

        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->enable_reg,
                                      1 << memif->data->enable_shift, 0);
        /* disable interrupt */
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, irq_data->irq_en_reg,
                                      1 << irq_data->irq_en_shift,
                                      0 << irq_data->irq_en_shift);
        /* and clear pending IRQ */
        aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, irq_data->irq_clr_reg,
                                 1 << irq_data->irq_clr_shift);

        if (memif->data->agent_disable_shift >= 0)
            aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                          memif->data->agent_disable_reg,
                                          1 << memif->data->agent_disable_shift,
                                          1 << memif->data->agent_disable_shift);
    }

    return 0;
}


static void mt7933_adsp_set_afe_init(struct mtk_base_afe *afe)
{
    //  struct mt7933_afe_private *afe_priv = afe->platform_priv;

    aud_msg("%s\n", __func__);

    /* enable audio power always on */
    //device_init_wakeup(afe->dev, true);

    //  mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_AUDIO_CG]);
    //  mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_AUD_26M_CG]);
    mt7933_afe_enable_main_clk(afe);
}

static void mt7933_adsp_set_afe_uninit(struct mtk_base_afe *afe)
{
    //  struct mt7933_afe_private *afe_priv = afe->platform_priv;

    aud_msg("%s\n", __func__);

    mt7933_afe_disable_main_clk(afe);
    //  mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_AUD_26M_CG]);
    //  mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_AUDIO_CG]);

    /* disable audio power always on */
    //device_init_wakeup(afe->dev, false);
}
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */
/*=====================================================*/

static void mt7933_afe_parse_of(struct mtk_base_afe *afe)
{
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_etdm_data *etdm_data;

    // mediatek,etdm-clock-modes
    afe_priv->etdm_data[MT7933_ETDM1].clock_mode = MT7933_ETDM_SEPARATE_CLOCK;
    afe_priv->etdm_data[MT7933_ETDM2].clock_mode = MT7933_ETDM_SEPARATE_CLOCK;

    // mediatek,etdm-out-data-modes
    afe_priv->etdm_data[MT7933_ETDM1].data_mode[MSD_STREAM_PLAYBACK] = MT7933_ETDM_DATA_ONE_PIN;
    afe_priv->etdm_data[MT7933_ETDM2].data_mode[MSD_STREAM_PLAYBACK] = MT7933_ETDM_DATA_ONE_PIN;

    // mediatek,etdm-in-data-modes
    afe_priv->etdm_data[MT7933_ETDM1].data_mode[MSD_STREAM_CAPTURE] = MT7933_ETDM_DATA_ONE_PIN;
    afe_priv->etdm_data[MT7933_ETDM2].data_mode[MSD_STREAM_CAPTURE] = MT7933_ETDM_DATA_ONE_PIN;

    // set etdm1-in
    {
        unsigned int stream = MSD_STREAM_CAPTURE;
        //      bool force_on = false;
        //      bool force_on_1st_trigger = false;

        etdm_data = &afe_priv->etdm_data[MT7933_ETDM1];

        //      if (force_on) {
        //          etdm_data->force_on[stream] = true;
        //          etdm_data->force_on_policy[stream] = MT7933_ETDM_FORCE_ON_DEFAULT;
        //      } else if (force_on_1st_trigger) {
        //          etdm_data->force_on[stream] = true;
        //          etdm_data->force_on_policy[stream] = MT7933_ETDM_FORCE_ON_1ST_TRIGGER;
        //      }

        /* skip related force-on properties parsing */
        if (etdm_data->force_on[stream]) {
            // force-format
            etdm_data->format[stream] = MT7933_ETDM_FORMAT_I2S;
            // force-mclk-freq
            etdm_data->mclk_freq[stream] = 0;
            // force-lrck-inverse
            etdm_data->lrck_inv[stream] = false;
            // force-bck-inverse
            etdm_data->bck_inv[stream] = false;
            // force-lrck-width
            etdm_data->lrck_width[stream] = 32;
            // force-rate
            etdm_data->force_rate[stream] = 48000;
            // force-channels
            etdm_data->force_channels[stream] = 2;
            // force-bit-width
            etdm_data->force_bit_width[stream] = 32;
        }
    }

    // set etdm2-out
    {
        unsigned int stream = MSD_STREAM_PLAYBACK;
        //      bool force_on = false;
        //      bool force_on_1st_trigger = false;

        etdm_data = &afe_priv->etdm_data[MT7933_ETDM2];

        //      if (force_on) {
        //          etdm_data->force_on[stream] = true;
        //          etdm_data->force_on_policy[stream] = MT7933_ETDM_FORCE_ON_DEFAULT;
        //      } else if (force_on_1st_trigger) {
        //          etdm_data->force_on[stream] = true;
        //          etdm_data->force_on_policy[stream] = MT7933_ETDM_FORCE_ON_1ST_TRIGGER;
        //      }

        /* skip related force-on properties parsing */
        if (etdm_data->force_on[stream]) {
            // force-format
            etdm_data->format[stream] = MT7933_ETDM_FORMAT_I2S;
            // force-mclk-freq
            etdm_data->mclk_freq[stream] = 0;
            // force-lrck-inverse
            etdm_data->lrck_inv[stream] = false;
            // force-bck-inverse
            etdm_data->bck_inv[stream] = false;
            // force-lrck-width
            etdm_data->lrck_width[stream] = 32;
            // force-rate
            etdm_data->force_rate[stream] = 48000;
            // force-channels
            etdm_data->force_channels[stream] = 2;
            // force-bit-width
            etdm_data->force_bit_width[stream] = 32;
        }
    }

    //set etdm2-in
    {
        unsigned int stream = MSD_STREAM_CAPTURE;
        //      bool force_on = false;
        //      bool force_on_1st_trigger = false;

        etdm_data = &afe_priv->etdm_data[MT7933_ETDM2];

        //      if (force_on) {
        //          etdm_data->force_on[stream] = true;
        //          etdm_data->force_on_policy[stream] = MT7933_ETDM_FORCE_ON_DEFAULT;
        //      } else if (force_on_1st_trigger) {
        //          etdm_data->force_on[stream] = true;
        //          etdm_data->force_on_policy[stream] = MT7933_ETDM_FORCE_ON_1ST_TRIGGER;
        //      }

        /* skip related force-on properties parsing */
        if (etdm_data->force_on[stream]) {
            // force-format
            etdm_data->format[stream] = MT7933_ETDM_FORMAT_I2S;
            // force-mclk-freq
            etdm_data->mclk_freq[stream] = 0;
            // force-lrck-inverse
            etdm_data->lrck_inv[stream] = false;
            // force-bck-inverse
            etdm_data->bck_inv[stream] = false;
            // force-lrck-width
            etdm_data->lrck_width[stream] = 32;
            // force-rate
            etdm_data->force_rate[stream] = 48000;
            // force-channels
            etdm_data->force_channels[stream] = 2;
            // force-bit-width
            etdm_data->force_bit_width[stream] = 32;
        }
    }

    // mediatek,dmic-two-wire-mode
    afe_priv->dmic_data.two_wire_mode = false;

    // mediatek,dmic-clk-phases
    afe_priv->dmic_data.clk_phase_sel_ch1 = 0;
    afe_priv->dmic_data.clk_phase_sel_ch2 = 4;

    // diatek,dmic-src-sels
    if (afe_priv->dmic_data.two_wire_mode) {
        afe_priv->dmic_data.dmic_src_sel[0] = 0;
        afe_priv->dmic_data.dmic_src_sel[1] = 1;
        afe_priv->dmic_data.dmic_src_sel[2] = 2;
        afe_priv->dmic_data.dmic_src_sel[3] = 3;
        afe_priv->dmic_data.dmic_src_sel[4] = 4;
        afe_priv->dmic_data.dmic_src_sel[5] = 5;
        afe_priv->dmic_data.dmic_src_sel[6] = 6;
        afe_priv->dmic_data.dmic_src_sel[7] = 7;
    } else {
        afe_priv->dmic_data.dmic_src_sel[0] = 0;
        afe_priv->dmic_data.dmic_src_sel[2] = 1;
        afe_priv->dmic_data.dmic_src_sel[4] = 2;
        afe_priv->dmic_data.dmic_src_sel[6] = 3;
    }

    // mediatek,dmic-iir-on
    afe_priv->dmic_data.iir_on = 1;

    // diatek,dmic-setup-time-us
    afe_priv->dmic_data.setup_time_us = 0;

}

int mtk_afe_dai_suspend(struct mtk_base_afe *afe)
{
    // backup clock mux
    unsigned int index;
    for (index = 0; index < afe->clk_mux_back_up_list_num; index++) {
        hal_clock_get_selected_mux(afe->clk_mux_back_up_list[index],
                                   &(afe->clk_mux_back_up[index]));
    }

    for (index = 0; index < afe->codec_reg_back_up_list_num; index++) {
        uint32_t tmp = aud_drv_get_reg(AUDIO_REGMAP_APMIXEDSYS,
                                       afe->codec_reg_back_up_list[index]);
        afe->codec_reg_back_up[index] = tmp;
    }

    for (index = 0; index < afe->reg_back_up_list_num; index++) {
        uint32_t tmp = aud_drv_get_reg(AUDIO_REGMAP_AFE_BASE,
                                       afe->reg_back_up_list[index]);
        afe->reg_back_up[index] = tmp;
    }

    return 0;
}

static void mt7933_afe_dai_suspend(void *data)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;

#ifdef CONFIG_MTK_ADSP_SUPPORT
    struct mt7933_adsp_data *adsp_data = &(afe_priv->adsp_data);
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */

    aud_dbg("suspend %d", afe->suspended);

    if (afe->suspended)
        return;

#ifdef CONFIG_MTK_ADSP_SUPPORT
    /* if ADSP HOSTLESS is active, not do register coverage */
    if (adsp_data->adsp_on) {
        if (adsp_data->hostless_active()) {
            aud_dbg("");
            hal_clock_mux_select(HAL_CLOCK_SEL_AUDIO_FAUD_INTBUS,
                                 CLK_AUDIO_FAUD_INTBUS_CLKSEL_AUDIO_XTAL);
            hal_clock_mux_select(HAL_CLOCK_SEL_AUDSYS_BUS,
                                 CLK_AUDSYS_BUS_CLKSEL_XTAL);

            afe->suspended = true;
            return;
        }
    }
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */

    mt7933_afe_enable_main_clk(afe);

    /* do afe suspend */
    mtk_afe_dai_suspend(afe);

    mt7933_afe_disable_main_clk(afe);

    //  mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_LP_MODE);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_26M_TIMING);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_A2SYS_TIMING);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_A1SYS_TIMING);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_A1SYS_HOPPING);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_AFE);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_A2SYS);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_A1SYS);
    mt7933_afe_disable_top_cg(afe, MT7933_TOP_CG_AFE_CONN);

    //  mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_BUS]);
    mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_26M]);
    mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_AUD_ADC0_XTAL]);
    mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_AUD_ADC1_XTAL]);
    mt7933_afe_disable_clk(afe, afe_priv->clocks[MT7933_CLK_AUD_DAC_XTAL]);
    afe->suspended = true;
    return;
}

int mtk_afe_dai_resume(struct mtk_base_afe *afe)
{
    // resume clock mux
    unsigned int index;
    for (index = 0; index < afe->clk_mux_back_up_list_num; index++) {
        hal_clock_mux_select(afe->clk_mux_back_up_list[index],
                             afe->clk_mux_back_up[index]);
    }

    for (index = 0; index < afe->codec_reg_back_up_list_num; index++) {
        aud_drv_set_reg_addr_val(AUDIO_REGMAP_APMIXEDSYS,
                                 afe->codec_reg_back_up_list[index],
                                 afe->codec_reg_back_up[index]);
    }

    for (index = 0; index < afe->reg_back_up_list_num; index++) {
        aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE,
                                 afe->reg_back_up_list[index],
                                 afe->reg_back_up[index]);
    }

    return 0;
}

static void mt7933_afe_dai_resume(void *data)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)data;
    struct mt7933_afe_private *afe_priv = afe->platform_priv;

#ifdef CONFIG_MTK_ADSP_SUPPORT
    struct mt7933_adsp_data *adsp_data = &(afe_priv->adsp_data);
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */

    aud_dbg("suspend %d", afe->suspended);

    if (!afe->suspended)
        return;


#ifdef CONFIG_MTK_ADSP_SUPPORT
    /* if ADSP HOSTLESS is active, not do register coverage */
    if (adsp_data->adsp_on) {

        if (adsp_data->hostless_active()) {
#ifdef PINMUX_QFN_DEFAULT
            hal_clock_mux_select(HAL_CLOCK_SEL_AUDIO_FAUD_INTBUS,
                                 CLK_AUDIO_FAUD_INTBUS_CLKSEL_133M);
            hal_clock_mux_select(HAL_CLOCK_SEL_AUDSYS_BUS,
                                 CLK_AUDSYS_BUS_CLKSEL_DIV_120M);
#elif defined PINMUX_BGA_DEFAULT
            hal_clock_mux_select(HAL_CLOCK_SEL_AUDIO_FAUD_INTBUS,
                                 CLK_AUDIO_FAUD_INTBUS_CLKSEL_266M);
            hal_clock_mux_select(HAL_CLOCK_SEL_AUDSYS_BUS,
                                 CLK_AUDSYS_BUS_CLKSEL_266M);
#endif /* #ifdef PINMUX_QFN_DEFAULT */
            afe->suspended = false;
            return;
        }
    }
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */

    mt7933_afe_enable_main_clk(afe);

    /* do afe suspend */
    mtk_afe_dai_resume(afe);

    mt7933_afe_disable_main_clk(afe);

    /* keep these cg open for dapm to read/write audio register */
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_AUD_ADC0_XTAL]);
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_AUD_ADC1_XTAL]);
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_AUD_DAC_XTAL]);
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_26M]);
    //  mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_BUS]);

    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A1SYS);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A2SYS);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_AFE);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A1SYS_HOPPING);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_AFE_CONN);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A1SYS_TIMING);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A2SYS_TIMING);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_26M_TIMING);
    //  mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_LP_MODE);

    afe->suspended = false;
    return;
}

void mt7933_afe_sleep_init(void *data)
{
    sleep_management_register_suspend_callback(SLEEP_BACKUP_RESTORE_AUDIO,
                                               mt7933_afe_dai_suspend, data);
    sleep_management_register_resume_callback(SLEEP_BACKUP_RESTORE_AUDIO,
                                              mt7933_afe_dai_resume, data);
}

void *afe_probe(void)
{
    int i, sel_irq;
    int ret;
    struct mt7933_afe_private *afe_priv;
    struct mtk_base_afe *afe = malloc(sizeof(*afe));
    if (!afe)
        return NULL;
    memset(afe, 0, sizeof(*afe));
    mt7933_afe = afe;

    afe_priv = malloc(sizeof(*afe_priv));
    if (!afe_priv)
        goto err_0;
    memset(afe_priv, 0, sizeof(*afe_priv));
    afe->platform_priv = afe_priv;

    spin_lock_init(&afe_priv->afe_ctrl_lock);

    afe_priv->afe_clk_mutex = mutex_init();
#ifdef CONFIG_MTK_ADSP_SUPPORT
    afe_priv->adsp_data.get_afe_memif_sram = mt7933_adsp_get_afe_memif_sram;
    afe_priv->adsp_data.set_afe_memif = mt7933_adsp_set_afe_memif;
    afe_priv->adsp_data.set_afe_memif_enable = mt7933_adsp_set_afe_memif_enable;
    afe_priv->adsp_data.set_afe_init = mt7933_adsp_set_afe_init;
    afe_priv->adsp_data.set_afe_uninit = mt7933_adsp_set_afe_uninit;
    afe_priv->adsp_data.irq_direction_enable = mt7933_afe_irq_direction_enable;
    afe_priv->adsp_data.get_afe_fs_timing = mt7933_afe_fs_timing;
    g_priv = afe;
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */

    /* set etdm low power mode*/
    afe_priv->etdm_data[MT7933_ETDM2].low_power_mode = false;

    /* register irq*/
    ret = hal_nvic_register_isr_handler(AFE_MCU_IRQn, mt7933_afe_irq_handler);
    if (ret) {
        aud_error("could not request_irq");
        goto err_1;
    }
    ret = hal_nvic_enable_irq(AFE_MCU_IRQn);
    if (ret) {
        aud_error("could not enable_irq");
        goto err_1;
    }

    /* initial audio related clock */
    ret = mt7933_afe_init_audio_clk(afe);
    if (ret) {
        aud_error("mt7933_afe_init_audio_clk fail");
        goto err_1;
    }
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_26M]);
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_PLL]);
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_FAPLL2]);
    mt7933_afe_enable_clk(afe, afe_priv->clocks[MT7933_CLK_TOP_AUD_BUS]);

    /* memif and irq initialize */
    afe->memif_size = MT7933_AFE_MEMIF_NUM;
    afe->memif = malloc(sizeof(*afe->memif) * afe->memif_size);
    if (!afe->memif)
        goto err_1;

    afe->irqs_size = MT7933_AFE_IRQ_NUM;
    afe->irqs = malloc(sizeof(*afe->irqs) * afe->irqs_size);
    if (!afe->irqs)
        goto err_2;

    for (i = 0; i < afe->irqs_size; i++) {
        afe->irqs[i].irq_data = &irq_data[i];
    }

    for (i = 0; i < afe->memif_size; i++) {
        afe->memif[i].data = &memif_data[i];
        sel_irq = memif_specified_irqs[i];
        if (sel_irq >= 0) {
            afe->memif[i].irq_usage = sel_irq;
            afe->memif[i].const_irq = 1;
            afe->irqs[sel_irq].irq_occupyed = 1;
        } else {
            afe->memif[i].irq_usage = -1;
        }
    }

    afe->reg_back_up_list = mt7933_afe_backup_list;
    afe->reg_back_up_list_num = ARRAY_SIZE(mt7933_afe_backup_list);

    afe->clk_mux_back_up_list = mt7933_clk_mux_backup_list;
    afe->clk_mux_back_up_list_num = ARRAY_SIZE(mt7933_clk_mux_backup_list);
    afe->clk_mux_back_up = (uint32_t *)malloc(afe->clk_mux_back_up_list_num * sizeof(uint32_t));
    if (!afe->clk_mux_back_up) {
        goto err_3;
    }

    afe->codec_reg_back_up_list = mt7933_codec_backup_list;
    afe->codec_reg_back_up_list_num = ARRAY_SIZE(mt7933_codec_backup_list);
    afe->codec_reg_back_up = (uint32_t *)malloc(afe->codec_reg_back_up_list_num * sizeof(uint32_t));
    if (!afe->codec_reg_back_up) {
        goto err_4;
    }

    afe->reg_back_up_list = mt7933_afe_backup_list;
    afe->reg_back_up_list_num = ARRAY_SIZE(mt7933_afe_backup_list);
    afe->reg_back_up = (uint32_t *)malloc(afe->reg_back_up_list_num * sizeof(uint32_t));
    if (!afe->reg_back_up) {
        goto err_5;
    }

    /* keep these cg open for dapm to read/write audio register */
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A1SYS);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A2SYS);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_AFE);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A1SYS_HOPPING);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_AFE_CONN);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A1SYS_TIMING);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_A2SYS_TIMING);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_26M_TIMING);
    mt7933_afe_enable_top_cg(afe, MT7933_TOP_CG_LP_MODE);

    mt7933_afe_parse_of(afe);

    snd_soc_add_widgets(mt7933_widget_list, sizeof(mt7933_widget_list) / sizeof(mt7933_widget_list[0]), afe);
    snd_soc_add_controls(mt7933_afe_controls, sizeof(mt7933_afe_controls) / sizeof(mt7933_afe_controls[0]), afe);
    snd_soc_add_widget_routes(mt7933_widget_routes, sizeof(mt7933_widget_routes) / sizeof(mt7933_widget_routes[0]));
    snd_soc_register_dais(mt7933_afe_pcm_dais, sizeof(mt7933_afe_pcm_dais) / sizeof(mt7933_afe_pcm_dais[0]), afe);

#ifdef CFG_AUDIO_SUSPEND_EN
    mt7933_afe_sleep_init((void *)afe);
#endif /* #ifdef CFG_AUDIO_SUSPEND_EN */

    aud_msg("MT7933 AFE driver initialized.");
    return afe;

err_5:
    free(afe->codec_reg_back_up);
err_4:
    free(afe->clk_mux_back_up);
err_3:
    free(afe->irqs);
err_2:
    free(afe->memif);
err_1:
    free(afe_priv);
err_0:
    free(afe);
    return NULL;
}

int afe_remove(void *priv)
{
    struct mtk_base_afe *afe = (struct mtk_base_afe *)priv;
    free(afe->reg_back_up);
    free(afe->codec_reg_back_up);
    free(afe->clk_mux_back_up);
    free(afe->irqs);
    free(afe->memif);
    free(afe);

    snd_soc_unregister_dais(mt7933_afe_pcm_dais);
    snd_soc_del_widget_routes(mt7933_widget_routes);
    return 0;
}


