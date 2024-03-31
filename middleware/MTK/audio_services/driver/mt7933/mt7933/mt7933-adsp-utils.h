#ifndef __ADSP_UTILS_H__
#define __ADSP_UTILS_H__

#include "audio_messenger_ipi.h"

enum {
    MT7933_ADSP_FE_LOCAL_RECORD = 0,
    MT7933_ADSP_FE_AP_RECORD,
    MT7933_ADSP_FE_HOSTLESS_VA,
    MT7933_ADSP_FE_VA,
    MT7933_ADSP_FE_HOSTLESS_PLAYBACK0,
    MT7933_ADSP_FE_HOSTLESS_PLAYBACK1,
    MT7933_ADSP_FE_CNT,
    MT7933_ADSP_BE_START = MT7933_ADSP_FE_CNT,
    MT7933_ADSP_BE_UL9 = MT7933_ADSP_BE_START,
    MT7933_ADSP_BE_UL2,
    MT7933_ADSP_BE_DL2,
    MT7933_ADSP_BE_DL3,
    MT7933_ADSP_BE_DLM,
    MT7933_ADSP_BE_END,
    MT7933_ADSP_BE_CNT = MT7933_ADSP_BE_END - MT7933_ADSP_BE_START,
};

int mt7933_adsp_get_scene_by_dai_id(int id);
int mt7933_adsp_dai_id_pack(int dai_id);
int mt7933_adsp_get_afe_memif_id(int dai_id);
int mt7933_adsp_send_ipi_cmd(struct ipi_msg_t *p_msg,
                             uint8_t task_scene,
                             uint8_t target,
                             uint8_t data_type,
                             uint8_t ack_type,
                             uint16_t msg_id,
                             uint32_t param1,
                             uint32_t param2,
                             char *payload);
bool mt7933_adsp_need_ul_dma_copy(int id);
void set_execption_happen_flag(void);
void clear_execption_happen_flag(void);
void dsp2ap_irq_init(void);
void dsp2ap_irq_uninit(void);
int ap2dsp_suspend_notify(void);
int ap2dsp_resume_notify(void);

#endif /* #ifndef __ADSP_UTILS_H__ */

