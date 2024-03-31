#ifndef _MT7933_AFE_UTILS_H_
#define _MT7933_AFE_UTILS_H_
#include "../common/mtk_base_afe.h"
#include "mt7933-afe-common.h"

int mt7933_afe_init_audio_clk(struct mtk_base_afe *afe);
int mt7933_afe_enable_clk(struct mtk_base_afe *afe, hal_clock_cg_id clk);
void mt7933_afe_disable_clk(struct mtk_base_afe *afe, hal_clock_cg_id clk);
int mt7933_afe_enable_main_clk(struct mtk_base_afe *afe);
int mt7933_afe_disable_main_clk(struct mtk_base_afe *afe);
int mt7933_afe_disable_apll_tuner_cfg(struct mtk_base_afe *afe, unsigned int apll);
int mt7933_afe_enable_apll_associated_cfg(struct mtk_base_afe *afe, unsigned int apll);
int mt7933_afe_set_clk_rate(struct mtk_base_afe *afe, hal_clock_sel_id clk,
                            unsigned int rate);
int mt7933_afe_enable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type);
int mt7933_afe_disable_top_cg(struct mtk_base_afe *afe, unsigned int cg_type);
#endif /* #ifndef _MT7933_AFE_UTILS_H_ */
