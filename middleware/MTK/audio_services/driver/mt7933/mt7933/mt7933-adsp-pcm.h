
#ifndef __MT7933_ADSP_PCM_H__
#define __MT7933_ADSP_PCM_H__

#include "mtk_base_afe.h"
#include "mt7933-adsp-utils.h"

struct adsp_dma_ring_buf {
    unsigned char *start_addr;
    unsigned int size_bytes;
    unsigned int hw_offset_bytes;
    unsigned int appl_offset_bytes;
};

struct cpu_dma_ring_buf {
    unsigned char *dma_buf_vaddr;
    size_t dma_buf_size;
    size_t dma_offset;
    size_t dma_period_size_bytes;
};

struct mt7933_adsp_dai_memory {
    struct snd_pcm_stream *stream;
    struct adsp_dma_ring_buf adsp_dma;
    //struct cpu_dma_ring_buf  cpu_dma;
    struct io_ipc_ring_buf_shared *adsp_dma_control;
    unsigned int adsp_dma_control_paddr;
    unsigned int is_first_write;
};


struct mt7933_adsp_be_dai_data {
    //struct snd_pcm_substream *substream;
    int mem_type;
};

struct mt7933_adsp_private {
    unsigned char dsp_boot_run;
    unsigned char dsp_ready;
    unsigned char dsp_loading;
    unsigned char dsp_suspend;
    struct mt7933_adsp_dai_memory dai_mem[MT7933_ADSP_FE_CNT];
    struct mt7933_adsp_be_dai_data be_data[MT7933_ADSP_BE_CNT];
    struct mtk_base_afe *afe;
#ifdef DSP_DEBUG_DUMP
    void *dbg_data;
    EventGroupHandle_t wakeup;
    QueueHandle_t msg_queue;
#endif /* #ifdef DSP_DEBUG_DUMP */
    int cyberon_model_type; //0: hey siri  1:zhinengguanjia  2:Hello Aircon
    const void *cyberon_model_pack_withTxt;
    size_t cyberon_model_pack_withTxt_size;
};

#endif /* #ifndef __MT7933_ADSP_PCM_H__ */

