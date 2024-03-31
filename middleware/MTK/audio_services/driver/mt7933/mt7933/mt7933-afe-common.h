#ifndef _MT7933_AFE_COMMON_H_
#define _MT7933_AFE_COMMON_H_

#define AFE_MCU_IRQ_BIT     (0x45)
#define AFE_MCU_IRQ_ID      (AFE_MCU_IRQ_BIT + 0x20)    /* 53 + 47 + 1 = 101*/

#include "sound/utils/include/mtk_mutex.h"
#include "freertos/snd_portable.h"
#include "sound/include/asound.h"
#include "mtk_base_afe.h"
#include "hal_platform.h"

#include <stdbool.h>

#define DMIC_MAX_CH (8)

enum {
    MT7933_AFE_MEMIF_DLM,
    MT7933_AFE_MEMIF_DL2,
    MT7933_AFE_MEMIF_DL3,
    MT7933_AFE_MEMIF_UL2,
    MT7933_AFE_MEMIF_UL3,
    MT7933_AFE_MEMIF_UL4,
    MT7933_AFE_MEMIF_UL8,
    MT7933_AFE_MEMIF_UL9,
    MT7933_AFE_MEMIF_UL10,
    MT7933_AFE_MEMIF_NUM,
    MT7933_AFE_BACKEND_BASE = MT7933_AFE_MEMIF_NUM,
    MT7933_AFE_IO_ETDM1_IN = MT7933_AFE_BACKEND_BASE,
    MT7933_AFE_IO_ETDM2_OUT,
    MT7933_AFE_IO_ETDM2_IN,
    MT7933_AFE_IO_DMIC,
    MT7933_AFE_IO_INT_ADDA,
    MT7933_AFE_IO_GASRC0,
    MT7933_AFE_IO_HW_GAIN1,
    MT7933_AFE_BACKEND_END,
    MT7933_AFE_BACKEND_NUM = (MT7933_AFE_BACKEND_END - MT7933_AFE_BACKEND_BASE),
};

enum {
    MT7933_AFE_IRQ10,
    MT7933_AFE_IRQ11,
    MT7933_AFE_IRQ12,
    MT7933_AFE_IRQ13,
    MT7933_AFE_IRQ14,
    MT7933_AFE_IRQ15,
    MT7933_AFE_IRQ16,
    MT7933_AFE_IRQ17,
    MT7933_AFE_IRQ18,
    MT7933_AFE_IRQ19,
    MT7933_AFE_IRQ20,
    MT7933_AFE_IRQ21,
    MT7933_AFE_IRQ22,
    MT7933_AFE_IRQ23,
    MT7933_AFE_IRQ24,
    MT7933_AFE_IRQ25,
    MT7933_AFE_IRQ_NUM,
};

enum {
    MT7933_TOP_CG_AFE,
    MT7933_TOP_CG_APLL2,
    MT7933_TOP_CG_DAC,
    MT7933_TOP_CG_DAC_PREDIS,
    MT7933_TOP_CG_ADC,
    MT7933_TOP_CG_ADC2,
    MT7933_TOP_CG_TML,
    MT7933_TOP_CG_UPLINK_TML,
    MT7933_TOP_CG_APLL2_TUNER,
    MT7933_TOP_CG_A1SYS_HOPPING,
    MT7933_TOP_CG_I2S_IN,
    MT7933_TOP_CG_TDM_IN,
    MT7933_TOP_CG_I2S_OUT,
    MT7933_TOP_CG_A1SYS,
    MT7933_TOP_CG_A2SYS,
    MT7933_TOP_CG_AFE_CONN,
    MT7933_TOP_CG_GASRC0,
    MT7933_TOP_CG_DMIC_TML,
    MT7933_TOP_CG_DMIC0,
    MT7933_TOP_CG_DMIC1,
    MT7933_TOP_CG_A1SYS_TIMING,
    MT7933_TOP_CG_A2SYS_TIMING,
    MT7933_TOP_CG_26M_TIMING,
    MT7933_TOP_CG_LP_MODE,
    MT7933_TOP_CG_NUM
};

enum {
    MT7933_CLK_TOP_AUD_26M,
    MT7933_CLK_TOP_AUD_BUS,
    MT7933_CLK_FAPLL2,
    MT7933_CLK_APLL12_DIV0,
    MT7933_CLK_APLL12_DIV1,
    MT7933_CLK_APLL12_DIV2,
    MT7933_CLK_AUD_ADC0_XTAL,
    MT7933_CLK_AUD_ADC1_XTAL,
    MT7933_CLK_AUD_DAC_XTAL,
    MT7933_CLK_TOP_PLL,
    MT7933_CLK_NUM
};

enum {
    MT7933_CLK_SEL_APLL12_DIV0,
    MT7933_CLK_SEL_APLL12_DIV1,
    MT7933_CLK_SEL_APLL12_DIV2,
    MT7933_CLK_SEL_NUM
};

enum {
    MT7933_FS_8K = 0,
    MT7933_FS_12K,
    MT7933_FS_16K,
    MT7933_FS_24K,
    MT7933_FS_32K,
    MT7933_FS_48K,
    MT7933_FS_96K,
    MT7933_FS_192K,
    MT7933_FS_384K,
    MT7933_FS_ETDMOUT1_1X_EN,
    MT7933_FS_ETDMOUT2_1X_EN,
    MT7933_FS_ETDMIN1_1X_EN = 12,
    MT7933_FS_ETDMIN2_1X_EN,
    MT7933_FS_EXT_PCM_1X_EN = 15,
    MT7933_FS_7P35K,
    MT7933_FS_11P025K,
    MT7933_FS_14P7K,
    MT7933_FS_22P05K,
    MT7933_FS_29P4K,
    MT7933_FS_44P1K,
    MT7933_FS_88P2K,
    MT7933_FS_176P4K,
    MT7933_FS_352P8K,
    MT7933_FS_ETDMIN1_NX_EN,
    MT7933_FS_ETDMIN2_NX_EN,
    MT7933_FS_AMIC_1X_EN_ASYNC = 28,
    MT7933_FS_DL_1X_EN = 30,
};

enum {
    MT7933_AFE_IRQ_DIR_MCU = 0,
    MT7933_AFE_IRQ_DIR_DSP,
    MT7933_AFE_IRQ_DIR_BOTH,
};

enum {
    MT7933_ETDM_FORCE_ON_DEFAULT = 0,
    MT7933_ETDM_FORCE_ON_1ST_TRIGGER,
};

enum {
    MT7933_ETDM_SEPARATE_CLOCK = 0,
    MT7933_ETDM_SHARED_CLOCK,
};

enum {
    MT7933_ETDM_DATA_ONE_PIN = 0,
    MT7933_ETDM_DATA_MULTI_PIN,
};

struct mt7933_etdm_ctrl_reg {
    unsigned int con0;
    unsigned int con1;
    unsigned int con2;
    unsigned int con3;
    unsigned int con4;
};

struct mt7933_control_data {
    int bypass_cm0;
    int bypass_cm1;
    int spdif_output_iec61937;
};

enum {
    MT7933_AFE_APLL1 = 0,
    MT7933_AFE_APLL2,
    MT7933_AFE_APLL_NUM,
};

struct mt7933_fe_dai_data {
    int slave_mode;
    int use_sram;
    unsigned int sram_phy_addr;
    void *sram_vir_addr;
    unsigned int sram_size;
};

struct mt7933_adsp_data {
    /* information adsp supply */
    unsigned char adsp_on;
    int (*hostless_active)(void);
    /* information afe supply */
    int (*set_afe_memif)(struct mtk_base_afe *afe,
                         int memif_id,
                         unsigned int rate,
                         unsigned int channels,
                         unsigned int bitwidth);
    int (*set_afe_memif_enable)(struct mtk_base_afe *afe,
                                int memif_id,
                                unsigned int rate,
                                unsigned int period_size,
                                int enable);
    int (*irq_direction_enable)(struct mtk_base_afe *afe,
                                int irq_id,
                                int direction);
    int (*get_afe_fs_timing)(unsigned int rate);
    void (*get_afe_memif_sram)(struct mtk_base_afe *afe,
                               int memif_id,
                               unsigned int *paddr,
                               unsigned int *size);
    void (*set_afe_init)(struct mtk_base_afe *afe);
    void (*set_afe_uninit)(struct mtk_base_afe *afe);
};

enum mt7933_afe_gasrc_lrck_sel_src {
    MT7933_AFE_GASRC_LRCK_SEL_ETDM_IN2 = 0,
    MT7933_AFE_GASRC_LRCK_SEL_ETDM_IN1,
    MT7933_AFE_GASRC_LRCK_SEL_ETDM_OUT2,
    MT7933_AFE_GASRC_LRCK_SEL_ETDM_OUT1,
    MT7933_AFE_GASRC_LRCK_SEL_PCM_IF,
    MT7933_AFE_GASRC_LRCK_SEL_UL_VIRTUAL,
};

enum {
    MT7933_GASRC0 = 0,
    MT7933_GASRC_NUM,
};

struct mt7933_gasrc_ctrl_reg {
    unsigned int con0;
    unsigned int con1;
    unsigned int con2;
    unsigned int con3;
    unsigned int con4;
    unsigned int con6;
    unsigned int con7;
    unsigned int con10;
    unsigned int con11;
    unsigned int con13;
    unsigned int con14;
};

struct mt7933_gasrc_data {
    unsigned int input_rate;
    unsigned int output_rate;
    unsigned int channel_num;
    bool cali_tx;
    bool cali_rx;
    bool one_heart;
    bool iir_on;
    bool op_freq_45m;
    unsigned int cali_cycles;
    bool re_enable[MSD_STREAM_CAPTURE + 1];
    int ref_cnt;
    spinlock_t gasrc_lock;
};

enum {
    MT7933_ETDM1 = 0,
    MT7933_ETDM2,
    MT7933_ETDM_SETS,
};

enum {
    MT7933_ETDM_FORMAT_I2S = 0,
    MT7933_ETDM_FORMAT_LJ,
    MT7933_ETDM_FORMAT_RJ,
    MT7933_ETDM_FORMAT_EIAJ,
    MT7933_ETDM_FORMAT_DSPA,
    MT7933_ETDM_FORMAT_DSPB,
};

struct mt7933_be_dai_data {
    bool prepared[MSD_STREAM_NUM];
    unsigned int fmt_mode;
};

struct mt7933_etdm_data {
    int occupied[MSD_STREAM_CAPTURE + 1];
    int active[MSD_STREAM_CAPTURE + 1];
    bool slave_mode[MSD_STREAM_CAPTURE + 1];
    bool lrck_inv[MSD_STREAM_CAPTURE + 1];
    bool bck_inv[MSD_STREAM_CAPTURE + 1];
    bool enable_interlink[MSD_STREAM_CAPTURE + 1];
    unsigned int lrck_width[MSD_STREAM_CAPTURE + 1];
    unsigned int data_mode[MSD_STREAM_CAPTURE + 1];
    unsigned int format[MSD_STREAM_CAPTURE + 1];
    unsigned int mclk_freq[MSD_STREAM_CAPTURE + 1];
    unsigned int clock_mode;
    bool force_on[MSD_STREAM_CAPTURE + 1];
    bool force_on_status[MSD_STREAM_CAPTURE + 1];
    unsigned int force_on_policy[MSD_STREAM_CAPTURE + 1];
    unsigned int force_rate[MSD_STREAM_CAPTURE + 1];
    unsigned int force_channels[MSD_STREAM_CAPTURE + 1];
    unsigned int force_bit_width[MSD_STREAM_CAPTURE + 1];
    bool low_power_mode;
};

struct mt7933_dmic_data {
    bool two_wire_mode;     //bool
    unsigned int clk_phase_sel_ch1;
    unsigned int clk_phase_sel_ch2;
    unsigned int dmic_src_sel[DMIC_MAX_CH];
    bool iir_on;            //bool
    unsigned int setup_time_us;
    unsigned int ul_mode;
};

struct mt7933_afe_private {
    hal_clock_cg_id clocks[MT7933_CLK_NUM];
    hal_clock_sel_id clocks_sel[MT7933_CLK_SEL_NUM];
    int hal_clock_ref_cnt[MT7933_CLK_NUM];
    int top_cg_ref_cnt[MT7933_TOP_CG_NUM];
    struct mt7933_fe_dai_data fe_data[MT7933_AFE_MEMIF_NUM];
    struct mt7933_be_dai_data be_data[MT7933_AFE_BACKEND_NUM];
    int afe_on_ref_cnt;
    struct mt7933_control_data ctrl_data;
    int apll_tuner_ref_cnt[MT7933_AFE_APLL_NUM];
    struct mt7933_gasrc_data gasrc_data[MT7933_GASRC_NUM];
    struct mt7933_etdm_data etdm_data[MT7933_ETDM_SETS];
    struct mt7933_dmic_data dmic_data;
    /* lock */
    spinlock_t afe_ctrl_lock;
    mutex_t afe_clk_mutex;

    struct mt7933_adsp_data adsp_data;
};

#ifdef CONFIG_MTK_ADSP_SUPPORT
struct mtk_base_afe *mt7933_afe_pcm_get_info(void);
#endif /* #ifdef CONFIG_MTK_ADSP_SUPPORT */

#endif /* #ifndef _MT7933_AFE_COMMON_H_ */

