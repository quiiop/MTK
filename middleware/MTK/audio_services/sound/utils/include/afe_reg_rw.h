#ifndef _AFE_REG_RW_H_
#define _AFE_REG_RW_H_

#include <stdint.h>

enum {
    AUDIO_REGMAP_AFE_BASE = 0,
    AUDIO_REGMAP_APMIXEDSYS,
    AUDIO_REGMAP_EEF_GRP2,
    AUDIO_REGMAP_BASE_MAX_COUNT,
    AUDIO_REGMAP_NO_BASE_TYPE,
};

struct mtk_reg_sequence {
    uint32_t reg;
    uint32_t val;
};

/* register R/W API */
void aud_drv_regmap_set_size(int size);
void aud_drv_regmap_set_addr_range(unsigned int type, void *ioremap_reg, uint32_t size);
uint32_t aud_drv_get_reg(unsigned int type, uintptr_t addr_offset);
void aud_drv_set_reg_addr_val(unsigned int type, uintptr_t addr_offset, uint32_t val);
void aud_drv_multi_set_reg_addr_val(int type, const struct mtk_reg_sequence *tables, int num);
void aud_drv_set_reg_addr_val_mask(unsigned int type, uintptr_t addr_offset, uint32_t val, uint32_t mask);
void aud_drv_set_reg_addr_mask_val(int type, uintptr_t addr_offset, uint32_t mask, uint32_t val);
void aud_drv_set_reg_check_addr_val(int type, int32_t addr_offset, uint32_t val);
void aud_drv_set_reg_check_addr_val_mask(int type, int32_t addr_offset, uint32_t val, uint32_t mask);
void aud_drv_set_reg_check_addr_mask_val(int type, int32_t addr_offset, uint32_t mask, uint32_t val);
int aud_drv_read_reg(int type, uintptr_t addr_affset, uint32_t *val);
#endif /* #ifndef _AFE_REG_RW_H_ */

