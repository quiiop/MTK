#ifndef _MTK_BASE_AFE_H_
#define _MTK_BASE_AFE_H_

#include <stdint.h>

#ifndef ARRAY_SIZE
//#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif /* #ifndef ARRAY_SIZE */

struct mtk_base_memif_data {
    int id;
    const char *name;
    int reg_ofs_base;
    int reg_ofs_cur;
    int fs_reg;
    int fs_shift;
    int fs_maskbit;
    int mono_reg;
    int mono_shift;
    int enable_reg;
    int enable_shift;
    int hd_reg;
    int hd_shift;
    int msb_reg;
    int msb_shift;
    int msb2_reg;
    int msb2_shift;
    int agent_disable_reg;
    int agent_disable_shift;
    int ch_config_reg;
    int ch_config_shift;
    int hd_align_reg;
    int hd_align_shift;
    int int_odd_reg;
    int int_odd_shift;
    int buffer_bytes_align;
    int buffer_end_shift;
};

struct mtk_base_irq_data {
    int id;
    int irq_cnt_reg;
    int irq_cnt_shift;
    int irq_cnt_maskbit;
    int irq_fs_reg;
    int irq_fs_shift;
    int irq_fs_maskbit;
    int irq_en_reg;
    int irq_en_shift;
    int irq_clr_reg;
    int irq_clr_shift;
    int irq_status_shift;
    int (*custom_handler)(int, void *);
};

struct mtk_base_afe_memif {
    unsigned int phys_buf_addr;
    int buffer_size;
    struct snd_pcm_stream *stream;
    const struct mtk_base_memif_data *data;
    int irq_usage;
    int const_irq;
};

struct mtk_base_afe_irq {
    const struct mtk_base_irq_data *irq_data;
    int irq_occupyed;
};

struct mtk_base_afe {
    struct mtk_base_afe_memif *memif;
    int memif_size;
    struct mtk_base_afe_irq *irqs;
    int irqs_size;
    void *platform_priv;

    int suspended;

    unsigned int const *reg_back_up_list;
    uint32_t *reg_back_up;
    unsigned int reg_back_up_list_num;

    unsigned int const *codec_reg_back_up_list;
    uint32_t *codec_reg_back_up;
    unsigned int codec_reg_back_up_list_num;

    unsigned int const *clk_mux_back_up_list;
    uint32_t *clk_mux_back_up;
    unsigned int clk_mux_back_up_list_num;
};

#endif /* #ifndef _MTK_BASE_AFE_H_ */

