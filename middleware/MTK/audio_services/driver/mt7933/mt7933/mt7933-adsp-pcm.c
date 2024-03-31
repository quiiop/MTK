#include "mt7933-afe-common.h"
#include "mt7933-adsp-pcm.h"
#include "mt7933-adsp-utils.h"
#include "mt7933-afe-utils.h"

#include "sound/utils/include/afe_reg_rw.h"
#include "memory_attribute.h"

#include "audio_shared_info.h"
#include "sound/driver/include/soc.h"
#include "sound/driver/include/pcm.h"
#include "audio_task_manager.h"
#include "audio_ipi/audio_ipi_driver.h"
#include "mtk_hifixdsp_common.h"
#include <string.h>
#include "audio_test_utils.h"
#include "hal_psram.h"
#include "hal_clock.h"
#include "hal_platform.h"
#include "hal_sleep_manager_internal.h"
#include "queue.h"

#ifdef CFG_DSPOTTER_SUPPORT
#include "CybDSpotter.h"
//#include "CybServer_MTK_Trial_little.h"

#include "HeySiri_OpenCamera_pack_withTxt.h"
#include "Demo_ZhiNengGuanJia_pack_withTxt.h"
#include "EngAircon_pack_withTxt.h"

#endif /* #ifdef CFG_DSPOTTER_SUPPORT */

#ifdef CFG_WRAP_PCM_SUPPORT
#include "wrap_dsp_msg.h"
#endif /* #ifdef CFG_WRAP_PCM_SUPPORT */

#ifdef FILE_SYS_SUPPORT
#include "ff.h"
#endif /* #ifdef FILE_SYS_SUPPORT */

#define lower_32_bits(n) ((unsigned int)(n))
#define AFE_BASE_END_OFFSET 8

#ifndef ARRAYSIZE
#define ARRAYSIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif /* #ifndef ARRAYSIZE */

struct mt7933_adsp_private *g_adsp_priv;
#define IS_ADSP_READY (g_adsp_priv->dsp_ready)
#define ADSP_IS_LOADING 2

TaskHandle_t wait_dsp_handler = NULL;

extern unsigned long bytes_to_frames(struct snd_pcm_runtime *runtime, unsigned int size);

/* ==========================  utils  ============================ */

static void mt7933_reset_dai_memory(struct mt7933_adsp_dai_memory *dai_mem)
{
    memset(dai_mem, 0, sizeof(struct mt7933_adsp_dai_memory));
}

static void mt7933_reset_dai_dma_offset(struct mt7933_adsp_dai_memory *dai_mem)
{
    dai_mem->adsp_dma.hw_offset_bytes = 0;
    dai_mem->adsp_dma.appl_offset_bytes = 0;
}

/* ==========================  probe  ============================ */

static void audio_ipi_ul_irq_handler(int id)
{
    struct mt7933_adsp_dai_memory *dai_mem = &g_adsp_priv->dai_mem[id];
    struct snd_pcm_stream *stream = dai_mem->stream;

    snd_pcm_period_elapsed(stream, 0);
}

#ifdef DSP_DEBUG_DUMP
//#include "memory_attribute.h"

#define DQUEUE_SIZE 20
TaskHandle_t dump_thread_handler = NULL;

#define DEFAULT_DUMP_CHANNEL_NUMBER     (4)
#define DEFAULT_DUMP_BITWIDTH           (32)
#define DEFAULT_DUMP_PERIOD_COUNT       (14)
#define DEFAULT_DUMP_PERIOD_SIZE        (160)
//#define DEFAULT_DUMP_FILE_SIZE        (4*4*16000*4)
//ATTR_RWDATA_IN_NONCACHED_RAM unsigned char ddump_buf[DEFAULT_DUMP_FILE_SIZE] = {0};

#define EVENT_DUMP_POSITION_1  (0x1 << 0)
#define EVENT_DUMP_POSITION_2  (0x1 << 1)
#define EVENT_DUMP_POSITION_3  (0x1 << 2)
#define EVENT_DUMP_POSITION_4  (0x1 << 3)
#define EVENT_DUMP_POSITION_ALL  \
    (EVENT_DUMP_POSITION_1|EVENT_DUMP_POSITION_2|EVENT_DUMP_POSITION_3|EVENT_DUMP_POSITION_4)


struct mt7933_adsp_debugfs_data {
    unsigned long size;
    int dump_position;
    unsigned int buffer_size;
    unsigned int timeout;
    unsigned int chnum;
    unsigned int bitwidth;
    unsigned int period_size;
    unsigned int period_count;
    bool enable;
    struct adsp_dma_ring_buf adsp_dma;
    struct io_ipc_ring_buf_shared *adsp_dma_control;
    void *vbuf;
    void *pbuf;
#ifdef FILE_SYS_SUPPORT
    FIL fid;
#endif /* #ifdef FILE_SYS_SUPPORT */
    unsigned char *buf_cur;
    int dump_over;
};

static unsigned int dump_data_from_dsp(struct mt7933_adsp_debugfs_data *dump)
{
    struct adsp_dma_ring_buf *adsp_dma = &dump->adsp_dma;
    struct io_ipc_ring_buf_shared *adsp_dma_control = dump->adsp_dma_control;

    unsigned int remain = ((unsigned char *)dump->pbuf + (unsigned int)dump->size) - dump->buf_cur;

    unsigned char *adsp_dma_buf_vaddr = adsp_dma->start_addr;
    unsigned int adsp_dma_buf_size = adsp_dma->size_bytes;
    unsigned int adsp_dma_hw_off = 0;
    unsigned int adsp_dma_appl_off = adsp_dma->appl_offset_bytes;

    unsigned int copy_bytes;
    unsigned int copied = 0;

    adsp_dma_hw_off = adsp_dma_control->ptr_to_hw_offset_bytes;

    if (adsp_dma_hw_off >= adsp_dma_appl_off)
        copy_bytes = adsp_dma_hw_off - adsp_dma_appl_off;
    else
        copy_bytes = adsp_dma_buf_size - adsp_dma_appl_off + adsp_dma_hw_off;

    /* calculate remain bytes and copy bytes */
    if (copy_bytes >= remain) {
        copy_bytes = remain;
    }

    while (copy_bytes > 0) {
        unsigned int count = 0;

        if (adsp_dma_hw_off >= adsp_dma_appl_off)
            count = adsp_dma_hw_off - adsp_dma_appl_off;
        else
            count = adsp_dma_buf_size - adsp_dma_appl_off;

        if (count > copy_bytes)
            count = copy_bytes;
#ifdef FILE_SYS_SUPPORT
        FRESULT f_ret;
        UINT f_bw;
        f_ret = f_write(&dump->fid, adsp_dma_buf_vaddr + adsp_dma_appl_off, count, &f_bw);
        if (f_ret || f_bw != count) {
            aud_error("f_write error: %d, %u", f_ret, f_bw);
        }
        f_ret = f_sync(&dump->fid);
        if (f_ret != FR_OK) {
            aud_error("f_sync error: %d", f_ret);
        }

#else /* #ifdef FILE_SYS_SUPPORT */
        memcpy(dump->buf_cur, adsp_dma_buf_vaddr + adsp_dma_appl_off, count);
#endif /* #ifdef FILE_SYS_SUPPORT */

        //printf("To:%p[0x%x], From:%p + 0x%x[0x%x], Bytes:0x%x\n", dump->buf_cur, *(unsigned int *)dump->buf_cur, adsp_dma_buf_vaddr, adsp_dma_appl_off, *(unsigned int*)(adsp_dma_buf_vaddr+adsp_dma_appl_off), count);
        copy_bytes -= count;
        dump->buf_cur += count;
        copied += count;
        adsp_dma_appl_off = (adsp_dma_appl_off + count) % adsp_dma_buf_size;

        remain -= count;
        if (remain == 0) {
            struct host_debug_param param;
            dump->dump_over = 1;
#ifdef FILE_SYS_SUPPORT
            f_close(&dump->fid);
#else /* #ifdef FILE_SYS_SUPPORT */
            free(dump->vbuf);
#endif /* #ifdef FILE_SYS_SUPPORT */

            param.dump_position = dump->dump_position;
            mt7933_adsp_send_ipi_cmd(NULL,
                                     TASK_SCENE_AUDIO_CONTROLLER,
                                     AUDIO_IPI_LAYER_TO_DSP,
                                     AUDIO_IPI_PAYLOAD,
                                     AUDIO_IPI_MSG_NEED_ACK,
                                     MSG_TO_DSP_DEBUG_STOP,
                                     sizeof(param),
                                     0,
                                     (char *)&param);
            printf("[%s] dump over. start address %p, size %ld.\n", __func__, dump->pbuf, dump->size);
        }
    }

    adsp_dma_control->ptr_to_appl_offset_bytes = adsp_dma_appl_off;
    adsp_dma->appl_offset_bytes = adsp_dma_appl_off;
    adsp_dma->hw_offset_bytes = adsp_dma_hw_off;

    return copied;
}

void dump_process(void *void_this)
{
    int i;
    uint16_t dump_postion;

    struct mt7933_adsp_debugfs_data *dump;

    for (;;) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        xQueueReceive(g_adsp_priv->msg_queue, &dump_postion, 0);
        for (i = DSP_DUMP1; i < DSP_DATA_DUMP_NUM; i++) {
            if (dump_postion & (0x1 << i)) {
                dump = (struct mt7933_adsp_debugfs_data *)g_adsp_priv->dbg_data + dump_postion;
                if (dump->dump_over != 1)
                    dump_data_from_dsp(dump);
            }
        }
    }

    vTaskDelete(NULL);
}

int dump_open(int dump_position, int *pcm_info)
{
    struct ipi_msg_t ipi_msg;
    struct host_debug_param param;
    struct host_debug_param ack_param;
    struct io_ipc_ring_buf_shared *shared_buf;
    struct adsp_dma_ring_buf *adsp_dma;
    struct mt7933_adsp_debugfs_data *dump;
    unsigned long paddr;
    void *vaddr;
    int ret;

    dump = (struct mt7933_adsp_debugfs_data *)g_adsp_priv->dbg_data + dump_position;
    printf("dump struct addr %p\n", dump);
    dump->dump_over = 0;
    adsp_dma = &dump->adsp_dma;

    if (pcm_info[1] != 0)
        dump->chnum = pcm_info[1];

    if (pcm_info[2] != 0)
        dump->bitwidth = pcm_info[2];

    if (pcm_info[0] != 0) {
        unsigned long tmp = pcm_info[0];
        tmp = tmp * dump->chnum;
        tmp = tmp * dump->bitwidth;
        tmp = tmp * 2;
        dump->size = tmp;

    }

#ifdef FILE_SYS_SUPPORT
    FRESULT f_ret;
    char file_name[256] = { 0 };
    static int dump_count = 0;

    snprintf(file_name, sizeof(file_name),
             "SD:/data/va_dump_pos_%d_cnt_%d.pcm",
             dump_position, dump_count++);

    aud_msg("dump data to %s", file_name);
    f_ret = f_open(&dump->fid, file_name, FA_CREATE_ALWAYS | FA_WRITE);
    if (f_ret) {
        aud_error("f_open %s error %d", file_name, f_ret);
        return -EPERM;
    }
    dump->vbuf = 0;
    dump->pbuf = 0;
    dump->buf_cur = (unsigned char *)dump->pbuf;

#else /* #ifdef FILE_SYS_SUPPORT */
    /* malloc memory */
    dump->vbuf = malloc(dump->size);
    if (!dump->vbuf) {
        aud_error("Dump buffer malloc fail!!\n");
        return -EPERM;
    }
    dump->pbuf = (void *)HAL_CACHE_VIRTUAL_TO_PHYSICAL((unsigned int)dump->vbuf);
    dump->buf_cur = (unsigned char *)dump->pbuf;

#endif /* #ifdef FILE_SYS_SUPPORT */

    printf("[%s] dump begin. start address %p, size %ld.\n", __func__, dump->pbuf, dump->size);
    dump->buffer_size = dump->chnum
                        * dump->bitwidth
                        * dump->period_count
                        * dump->period_size
                        / 8;
    printf("[%s] dump size[%ld], channel[%d], bits[%d]\n", __func__, dump->size, dump->chnum, dump->bitwidth);

    param.dump_position = dump->dump_position;
    param.request_bytes = dump->size;
    param.chnum = dump->chnum;
    param.bitwidth = dump->bitwidth;
    param.period_cnt = dump->period_count;
    param.period_size = dump->period_size;
    param.buffer_size = dump->buffer_size;

    ret = mt7933_adsp_send_ipi_cmd(&ipi_msg,
                                   TASK_SCENE_AUDIO_CONTROLLER,
                                   AUDIO_IPI_LAYER_TO_DSP,
                                   AUDIO_IPI_PAYLOAD,
                                   AUDIO_IPI_MSG_NEED_ACK,
                                   MSG_TO_DSP_DEBUG_START,
                                   sizeof(param),
                                   0,
                                   (char *)&param);
    if (ret)
        return ret;

    memcpy((void *)(&ack_param), (void *)(&ipi_msg.payload), sizeof(struct host_debug_param));
    if (ack_param.inited == 0) {
        aud_error("%s, dsp dump not inited.\n", __func__);
        return -EPERM;
    }

    /* get adsp dma control block between adsp pcm driver and adsp */
    paddr = ack_param.shared_base;
    paddr = adsp_hal_phys_addr_dsp2cpu(paddr);
    vaddr = adsp_get_shared_sysram_phys2virt(paddr);
    shared_buf = (struct io_ipc_ring_buf_shared *)vaddr;
    dump->adsp_dma_control = shared_buf;
    /* config dma between adsp pcm driver and adsp */
    paddr = shared_buf->start_addr;
    paddr = adsp_hal_phys_addr_dsp2cpu(paddr);
    vaddr = adsp_get_shared_sysram_phys2virt(paddr);
    adsp_dma->start_addr = (unsigned char *)vaddr;
    adsp_dma->size_bytes = shared_buf->size_bytes;
    adsp_dma->hw_offset_bytes = shared_buf->ptr_to_hw_offset_bytes;
    adsp_dma->appl_offset_bytes = shared_buf->ptr_to_appl_offset_bytes;

    dump->enable = true;

    return 0;
}

#endif /* #ifdef DSP_DEBUG_DUMP */

static void mt7933_adsp_handle_debug_dump_irq(struct ipi_msg_t *p_ipi_msg)
{
#ifdef DSP_DEBUG_DUMP
    struct dsp_debug_irq_param param;
    int i;
    int debugfs_count = DSP_DATA_DUMP_NUM;
    struct mt7933_adsp_debugfs_data *dump_data_base;
    struct mt7933_adsp_debugfs_data *dump_data;
    BaseType_t ret;

    memcpy((void *)(&param), (void *)(p_ipi_msg->payload),
           sizeof(struct dsp_debug_irq_param));

    dump_data_base = (struct mt7933_adsp_debugfs_data *)g_adsp_priv->dbg_data;
    if (!dump_data_base)
        return;
    for (i = 0; i < debugfs_count; i++) {
        dump_data = dump_data_base + i;
        if (dump_data && (dump_data->dump_position == param.dump_position))
            break;
    }

    if (!dump_data || (i == debugfs_count) || (dump_data->enable == 0)) {
        aud_error("%s dump invalid\n", __func__);
        return;
    }

    ret = xQueueSendToBack(g_adsp_priv->msg_queue, &(param.dump_position), 0);
    if (ret != pdPASS)
        aud_error("%s, send msg failed\n", __func__);

    ret = xTaskNotifyGive(dump_thread_handler);
    if (ret != pdTRUE) {
        aud_error("%s, wake thread failed\n", __func__);
        return;
    }
#endif /* #ifdef DSP_DEBUG_DUMP */
}

static void mt7933_adsp_init_debug_dump(struct mt7933_adsp_private *priv)
{
#ifdef DSP_DEBUG_DUMP
    size_t i;
    size_t dump_count = DSP_DATA_DUMP_NUM;
    size_t data_size = sizeof(struct mt7933_adsp_debugfs_data) * dump_count;
    struct mt7933_adsp_debugfs_data *dump_data_base;
    struct mt7933_adsp_debugfs_data *dump_data;

    priv->dbg_data = malloc(data_size);
    if (!priv->dbg_data)
        return;

    dump_data_base = (struct mt7933_adsp_debugfs_data *)priv->dbg_data;

    for (i = 0; i < dump_count; i++) {
        dump_data = dump_data_base + i;
        dump_data->chnum = DEFAULT_DUMP_CHANNEL_NUMBER;
        dump_data->bitwidth = DEFAULT_DUMP_BITWIDTH;
        dump_data->period_size = DEFAULT_DUMP_PERIOD_SIZE;
        dump_data->period_count = DEFAULT_DUMP_PERIOD_COUNT;
        dump_data->dump_position = DSP_DUMP1 + i;
        dump_data->enable = false;
        dump_data->buffer_size = dump_data->chnum
                                 * dump_data->bitwidth
                                 * dump_data->period_count
                                 * dump_data->period_size
                                 / 8;
        dump_data->size = dump_data->buffer_size;
        //dump_data->size = DEFAULT_DUMP_FILE_SIZE;
    }
    priv->msg_queue = xQueueCreate(DQUEUE_SIZE, sizeof(int));

    xTaskCreate(dump_process,
                "dsp_dump",
                configMINIMAL_STACK_SIZE * 4,
                NULL,
                configMAX_PRIORITIES - 1,
                &dump_thread_handler);

#endif /* #ifdef DSP_DEBUG_DUMP */
}

static void mt7933_adsp_cleanup_debug_dump(struct mt7933_adsp_private *priv)
{
#ifdef DSP_DEBUG_DUMP
    if (priv->dbg_data)
        free(priv->dbg_data);
#endif /* #ifdef DSP_DEBUG_DUMP */
}

void mt7933_adsp_record_notify(struct dsp_ipc_va_notify *info)
{
    // TODO:
    aud_msg("mt7933_adsp_record_notify\n");
}

static void mt7933_adsp_generic_recv_msg(struct ipi_msg_t *p_ipi_msg)
{
    if (!p_ipi_msg)
        return;

    if (p_ipi_msg->task_scene == TASK_SCENE_AUDIO_CONTROLLER) {
        switch (p_ipi_msg->msg_id) {
            case MSG_TO_HOST_DSP_AUDIO_READY:
                aud_msg("%s, DSP Audio ready!\n", __func__);
                g_adsp_priv->dsp_loading = 0;
#ifdef AUDIO_FREERTOS_SUPPORT
                if (wait_dsp_handler != NULL)
                    xTaskNotifyGive(wait_dsp_handler);
                else
                    aud_msg("%s, wait_dsp_handler is NULL\n", __func__);
#endif /* #ifdef AUDIO_FREERTOS_SUPPORT */
                break;
            case MSG_TO_HOST_DSP_DEBUG_IRQ:
                // TODO:
                //          aud_msg("DSP debug IRQ!\n");
                mt7933_adsp_handle_debug_dump_irq(p_ipi_msg);
                break;
            case MSG_TO_HOST_IRQ:
                aud_msg("%s, Got it!\n", __func__);
                break;
            default:
                break;
        }
    } else {
        aud_msg("%s. Not a support task scene: ID = %d\n", __func__, p_ipi_msg->task_scene);
    }
}

extern void notify_va_upload(int type, int value);

static void mt7933_adsp_pcm_va_notify(struct dsp_ipc_va_notify *info)
{
    if (info->type == VA_NOTIFY_LOCAL_CMD_TRIGGER_PASS || info->type == VA_NOTIFY_LOCAL_CMD_COMMAND_PASS) {
#ifdef CFG_DSPOTTER_SUPPORT
        aud_msg("[%s]get local command type(%d) idx(%d) mapID(%d) notify.\n",
                __func__, info->type, info->local_cmd_idx, info->local_cmd_mapID);
#else /* #ifdef CFG_DSPOTTER_SUPPORT */
        aud_msg("[%s]get local command type(%d) idx(%d) notify.\n",
                __func__, info->type, info->local_cmd_idx);
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */
    } else if (info->type == VA_NOTIFY_LOCAL_CMD_COMMAND_TIMEOUT) {
        aud_msg("[%s]VAD local command timeout!\n", __func__);
    } else if (info->type == VA_NOTIFY_VAD_PASS) {
        aud_msg("[%s]get vad ok notify.\n", __func__);
    } else if (info->type == VA_NOTIFY_WAKEWORD_PASS) {
        aud_msg("[%s]get wakeword(%s) ok notify.\n", __func__, info->wakeword);
    } else {
        aud_msg("[%s] Unhandled info type(%d).\n", __func__, info->type);
        return;
    }
    notify_va_upload(info->type, info->local_cmd_idx);
}

void mt7933_adsp_ul_recv_msg(struct ipi_msg_t *p_ipi_msg)
{
    if (!p_ipi_msg)
        return;

    switch (p_ipi_msg->msg_id) {
        case MSG_TO_HOST_DSP_IRQUL: {
                struct dsp_ipc_msg_irq ipc_irq;
                memcpy((void *)&ipc_irq,
                       (void *)(p_ipi_msg->payload),
                       sizeof(struct dsp_ipc_msg_irq));
                audio_ipi_ul_irq_handler(ipc_irq.dai_id);
            }
            break;
#ifdef CFG_WRAP_PCM_SUPPORT
        case MSG_TO_HOST_RECORD_OPEN_CAPTURE:
        case MSG_TO_HOST_RECORD_START_CAPTURE:
        case MSG_TO_HOST_RECORD_STOP_CAPTURE:
        case MSG_TO_HOST_RECORD_CLOSE_CAPTURE: {
                int ret = 0;
                ret = capture_wrap_msg_hdl(p_ipi_msg);
                if (ret != 0)
                    aud_error("Capure wrap msg handler return error: %d", ret);
            }
            break;
#endif /* #ifdef CFG_WRAP_PCM_SUPPORT */
        case MSG_TO_HOST_VA_NOTIFY: {
                struct dsp_ipc_va_notify info;
                memcpy((void *)&info,
                       (void *)(p_ipi_msg->payload),
                       sizeof(struct dsp_ipc_va_notify));
                mt7933_adsp_pcm_va_notify(&info);
            }
            break;
        default:
            break;
    }
}

static const struct {
    unsigned int scene;
    unsigned int msg_id;
} init_tasks[] = {
#ifdef CFG_VA_TASK_SUPPORT
#ifdef CFG_DSPOTTER_SUPPORT
    {TASK_SCENE_AUDIO_CONTROLLER, MSG_TO_DSP_MALLOC_CYBERON_BUFFER},
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */
    {TASK_SCENE_AUDIO_CONTROLLER, MSG_TO_DSP_CREATE_VA_T},
#endif /* #ifdef CFG_VA_TASK_SUPPORT */
#ifdef CFG_RECORD_TASK_SUPPORT
    {TASK_SCENE_AUDIO_CONTROLLER, MSG_TO_DSP_CREATE_RECORD_T},
#endif /* #ifdef CFG_RECORD_TASK_SUPPORT */
    /* fix build warning */
    {TASK_SCENE_INVALID, MSG_TO_DSP_NUM},
};

static const struct {
    unsigned int scene;
    recv_message_t recv;
} task_callbacks[] = {
    {TASK_SCENE_AUDIO_CONTROLLER, mt7933_adsp_generic_recv_msg},
#ifdef CFG_VA_TASK_SUPPORT
    {TASK_SCENE_VA, mt7933_adsp_ul_recv_msg},
#endif /* #ifdef CFG_VA_TASK_SUPPORT */
#ifdef CFG_RECORD_TASK_SUPPORT
    {TASK_SCENE_RECORD, mt7933_adsp_ul_recv_msg},
#endif /* #ifdef CFG_RECORD_TASK_SUPPORT */
};

#ifdef AUDIO_FREERTOS_SUPPORT
static void wait_dsp_task_loop(void *void_this)
{
    struct mt7933_adsp_private *priv = (struct mt7933_adsp_private *)void_this;
    struct mt7933_afe_private *afe_priv = (struct mt7933_afe_private *)(priv->afe->platform_priv);
    struct mt7933_adsp_data *adsp_data = &(afe_priv->adsp_data);
    int ret;
    unsigned int i;

#ifdef CFG_DSPOTTER_SUPPORT
    int nRet = 1;
    DSpotterControl oDSpotterControl;
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */
    //  aud_msg("+%s\n", __func__);

    while (1) {
        ret = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (ret != pdTRUE) {
            aud_error("%s, receive msg failed\n", __func__);
            continue;
        }


        if (ARRAYSIZE(init_tasks) != 0) {
            aud_msg("%s dsp loading done, start init audio tasks\n", __func__);
            //mt7933_adsp_low_power_init(priv);

            for (i = 0; i < ARRAYSIZE(init_tasks); i++) {

                if (init_tasks[i].scene == TASK_SCENE_INVALID) {
                    continue;
                }
#ifdef CFG_DSPOTTER_SUPPORT
                if (!(init_tasks[i].scene == TASK_SCENE_AUDIO_CONTROLLER &&
                      init_tasks[i].msg_id == MSG_TO_DSP_MALLOC_CYBERON_BUFFER)) {
#else /* #ifdef CFG_DSPOTTER_SUPPORT */
                if (init_tasks[i].scene != TASK_SCENE_INVALID) {
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */
                    mt7933_adsp_send_ipi_cmd(NULL, init_tasks[i].scene,
                                             AUDIO_IPI_LAYER_TO_DSP,
                                             AUDIO_IPI_MSG_ONLY,
                                             AUDIO_IPI_MSG_NEED_ACK,
                                             init_tasks[i].msg_id, 0, 0, NULL);
                } else {
#ifdef CFG_DSPOTTER_SUPPORT
                    nRet = CybDSpotterCheckLicense();
                    if (nRet != CYB_SUCCESS) {
                        printf("License do not existed.Pls request first.\n");
                        //         vTaskDelete(NULL);
                    }
                    if (nRet == CYB_SUCCESS) {
                        oDSpotterControl.nCommandStageFlowControl = 1;
                        oDSpotterControl.nMaxCommandTime = MAX_COMMAND_TIME;
                        oDSpotterControl.nCommandStageTimeout = COMMAND_STAGE_TIMEOUT;
                        oDSpotterControl.byAGC_Gain = 32;
                        nRet = CybDSpotterInit(&oDSpotterControl);
                        if (nRet == CYB_SUCCESS)
                            printf("CybDSpotterInit() OK.\n");
                        else
                            printf("CybDSpotterInit() fail! ret = %d.\n", nRet);
                    }
                    uint32_t addr_info = priv->cyberon_model_pack_withTxt_size;
                    struct ipi_msg_t ipi_msg;
                    mt7933_adsp_send_ipi_cmd(&ipi_msg, init_tasks[i].scene,
                                             AUDIO_IPI_LAYER_TO_DSP,
                                             AUDIO_IPI_PAYLOAD,
                                             AUDIO_IPI_MSG_NEED_ACK,
                                             init_tasks[i].msg_id,
                                             sizeof(uint32_t),
                                             0,
                                             (char *)&addr_info);
                    memcpy(&addr_info, ipi_msg.payload, sizeof(uint32_t));
                    if (addr_info == 0)
                        continue;
                    addr_info = adsp_hal_phys_addr_dsp2cpu(addr_info);
                    aud_dbg("addr_info = %x", addr_info);
                    memcpy((void *)addr_info, priv->cyberon_model_pack_withTxt,
                           priv->cyberon_model_pack_withTxt_size);
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */
                }
            }
        }
        priv->dsp_ready = true;
        adsp_data->adsp_on = 1;
    }
}
#endif /* #ifdef AUDIO_FREERTOS_SUPPORT */

static void load_hifi4dsp_callback(void *arg)
{
#ifndef AUDIO_FREERTOS_SUPPORT
    struct mt7933_adsp_private *priv = (struct mt7933_adsp_private *)arg;
    struct mt7933_afe_private afe_priv = (struct mt7933_afe_private *)(priv->afe->platform_priv);
    struct mt7933_adsp_data *adsp_data = &(afe_priv->adsp_data);

#ifdef CFG_DSPOTTER_SUPPORT
    int nRet = 1;
    DSpotterControl oDSpotterControl;
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */

    if (!hifixdsp_run_status())
        aud_msg("%s hifi4dsp_run_status not done\n", __func__);

    if (ARRAYSIZE(init_tasks) != 0) {
        int i;
        while (priv->dsp_loading) {
            usleep(10000);
        }
        aud_msg("%s dsp loading done, start init audio tasks\n", __func__);
        //mt7933_adsp_low_power_init(priv);

        for (i = 0; i < ARRAYSIZE(init_tasks); i++) {
#ifdef CFG_DSPOTTER_SUPPORT
            if (!(init_tasks[i].scene == TASK_SCENE_AUDIO_CONTROLLER &&
                  init_tasks[i].msg_id == MSG_TO_DSP_MALLOC_CYBERON_BUFFER)) {
#else /* #ifdef CFG_DSPOTTER_SUPPORT */
            if (init_tasks[i].scene != TASK_SCENE_INVALID) {
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */
                mt7933_adsp_send_ipi_cmd(NULL, init_tasks[i].scene,
                                         AUDIO_IPI_LAYER_TO_DSP,
                                         AUDIO_IPI_MSG_ONLY,
                                         AUDIO_IPI_MSG_NEED_ACK,
                                         init_tasks[i].msg_id, 0, 0, NULL);
            } else {
#ifdef CFG_DSPOTTER_SUPPORT
                nRet = CybDSpotterCheckLicense();
                if (nRet != CYB_SUCCESS) {
                    printf("License do not existed.Pls request it first.\n");
                    //           return CYB_LICENSE_NOT_EXIST;
                }
                if (nRet == CYB_SUCCESS) {
                    oDSpotterControl.nCommandStageFlowControl = 1;
                    oDSpotterControl.nMaxCommandTime = MAX_COMMAND_TIME;
                    oDSpotterControl.nCommandStageTimeout = COMMAND_STAGE_TIMEOUT;
                    oDSpotterControl.byAGC_Gain = 32;
                    nRet = CybDSpotterInit(&oDSpotterControl);
                    if (nRet == CYB_SUCCESS)
                        printf("CybDSpotterInit() OK.\n");
                    else
                        printf("CybDSpotterInit() fail! ret = %d.\n", nRet);
                }
                uint32_t addr_info = priv->cyberon_model_pack_withTxt_size;

                struct ipi_msg_t ipi_msg;
                mt7933_adsp_send_ipi_cmd(&ipi_msg, init_tasks[i].scene,
                                         AUDIO_IPI_LAYER_TO_DSP,
                                         AUDIO_IPI_PAYLOAD,
                                         AUDIO_IPI_MSG_NEED_ACK,
                                         init_tasks[i].msg_id,
                                         sizeof(uint32_t),
                                         0,
                                         (char *)&addr_info);
                memcpy(&addr_info, ipi_msg.payload, sizeof(uint32_t));
                addr_info = adsp_hal_phys_addr_dsp2cpu(addr_info);
                aud_dbg("addr_info = %x", addr_info);
                memcpy((void *)addr_info, priv->cyberon_model_pack_withTxt,
                       priv->cyberon_model_pack_withTxt_size);
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */
            }
        }
    }
    priv->dsp_ready = true;
    adsp_data->adsp_on = 1;
#endif /* #ifndef AUDIO_FREERTOS_SUPPORT */
}

static int mt7933_adsp_fe_dai_active(int id)
{
    struct snd_pcm_stream *stream;
    struct snd_soc_pcm_runtime *rtd;
    struct snd_soc_dai *dai;

    if (id < 0 || id >= MT7933_ADSP_FE_CNT) {
        aud_error("%s, wrong dai_id: %d\n", __func__, id);
        return 0;
    }

    stream = g_adsp_priv->dai_mem[id].stream;
    if (stream == NULL)
        return 0;

    rtd = (struct snd_soc_pcm_runtime *)(stream->private_data);
    dai = rtd->cpu_dai;
    if (dai != NULL)
        return dai->active;
    else
        return 0;
}


static int mt7933_adsp_hostless_active(void)
{
    int active = 0;

    active += mt7933_adsp_fe_dai_active(MT7933_ADSP_FE_HOSTLESS_VA);
    active += mt7933_adsp_fe_dai_active(MT7933_ADSP_FE_HOSTLESS_PLAYBACK0);
    active += mt7933_adsp_fe_dai_active(MT7933_ADSP_FE_HOSTLESS_PLAYBACK1);

    return active;
}

static void mt7933_adsp_suspend_callback(void *arg)
{
    bool hostless;

    /* if dsp is not ready or on, ignore suspend */
    if (!IS_ADSP_READY)
        return;

    if (g_adsp_priv->dsp_suspend)
        return;

    hostless = (mt7933_adsp_hostless_active() > 0);
    if (hostless) {
        ap2dsp_suspend_notify();
    }
    /* If there is no hostless active */
    /* Whatever other DAIs is active or not, do nothing now */
    /* DSP may in WFI mode, or clock off? */

    g_adsp_priv->dsp_suspend = true;

}

static void mt7933_adsp_resume_callback(void *arg)
{
    bool hostless;

    /* if dsp is not ready or on, ignore resume */
    if (!IS_ADSP_READY)
        return;

    if (!g_adsp_priv->dsp_suspend)
        return;

    hostless = (mt7933_adsp_hostless_active() > 0);
    if (hostless) {
        ap2dsp_resume_notify();
    }
    /* If there is no hostless active */
    /* Whatever other DAIs is active or not, do nothing now */
    /* DSP may in WFI mode */

    g_adsp_priv->dsp_suspend = false;
    return;
}

void mt7933_adsp_sleep_init(void)
{
    sleep_management_register_suspend_callback(SLEEP_BACKUP_RESTORE_DSP, mt7933_adsp_suspend_callback, NULL);
    sleep_management_register_resume_callback(SLEEP_BACKUP_RESTORE_DSP, mt7933_adsp_resume_callback, NULL);
}

static int mt7933_adsp_probe(struct mt7933_adsp_private *adsp_priv)
{
    int ret = 0;
    struct mt7933_adsp_data *adsp_data;
    struct mt7933_afe_private *afe_priv;
    unsigned int i = 0;

    aud_msg("mt7933_adsp_probe\n");

    adsp_priv->afe = mt7933_afe_pcm_get_info();
    afe_priv = (struct mt7933_afe_private *)(adsp_priv->afe->platform_priv);
    adsp_data = &(afe_priv->adsp_data);
    adsp_data->hostless_active = mt7933_adsp_hostless_active;

    for (i = 0; i < ARRAYSIZE(task_callbacks); i++)
        audio_task_register_callback(task_callbacks[i].scene, task_callbacks[i].recv, NULL);

    if (ret) {
        aud_msg("%s register callback for audio controller fail %d\n", __func__, ret);
        return ret;
    }
#ifdef AUDIO_FREERTOS_SUPPORT
    /**************
     * AP will not receive interrupt before task schedule.
     * So cannot use while loop to wait interrupt in func load_hifi4dsp_callback.
     */
    if (xTaskCreate(
            wait_dsp_task_loop,
            "wait_dsp_task",
            configMINIMAL_STACK_SIZE * 4,
            adsp_priv,
            configMAX_PRIORITIES - 2,
            &wait_dsp_handler) != pdPASS) {
        aud_error("[%s] wait_dsp_task create error!\n", __func__);
    }
#endif /* #ifdef AUDIO_FREERTOS_SUPPORT */

    mt7933_adsp_init_debug_dump(adsp_priv);

    aud_msg("%s dsp boot run = %d\n", __func__, adsp_priv->dsp_boot_run);
    if (!adsp_priv->dsp_boot_run)
        return 0;

    hal_sleep_manager_lock_sleep(SLEEP_LOCK_DEEP_SLEEP);
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_AUDIO);
    adsp_priv->dsp_loading = 1;
    ret = async_load_hifixdsp_bin_and_run(load_hifi4dsp_callback, adsp_priv);
    if (ret) {
        aud_msg("%s async_load_hifi4dsp_bin_and_run fail %d\n", __func__, ret);
        adsp_priv->dsp_loading = 0;
        return ret;
    }

    return 0;
}

static int mt7933_adsp_remove(void)
{
    int ret = 0;
    aud_msg("mt7933_adsp_remove\n");

    ret = hifixdsp_stop_run();
    if (ret) {
        aud_error("%s hifixdsp_stop_run fail %d\n", __func__, ret);
    }
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_DEEP_SLEEP);
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_AUDIO);
    return 0;
}

/* ==========================  Controls  ============================ */

static int adsp_enable_get(struct msd_ctl_value *value, void *priv)
{
    value->integer.value[0] = IS_ADSP_READY;
    if (g_adsp_priv->dsp_loading)
        value->integer.value[0] = ADSP_IS_LOADING;
    return 0;
}

static int adsp_enable_put(struct msd_ctl_value *value, void *priv)
{
    struct mt7933_adsp_private *adsp_priv = (struct mt7933_adsp_private *)priv;
    struct mt7933_afe_private *afe_priv = (struct mt7933_afe_private *)(adsp_priv->afe->platform_priv);
    struct mt7933_adsp_data *adsp_data = &(afe_priv->adsp_data);
    int enable = value->integer.value[0];
    aud_msg("set_adsp_enable\n");
    int ret = 0;

    /* already loading */
    if (adsp_priv->dsp_loading)
        return -EBUSY;

    if ((enable && IS_ADSP_READY) || (!enable && !IS_ADSP_READY))
        return 0;

    if (enable) {
        hal_sleep_manager_lock_sleep(SLEEP_LOCK_DEEP_SLEEP);
        hal_sleep_manager_lock_sleep(SLEEP_LOCK_AUDIO);
        adsp_priv->dsp_loading = 1;
        ret = async_load_hifixdsp_bin_and_run(load_hifi4dsp_callback, adsp_priv);
        if (ret) {
            aud_msg("%s async_load_hifi4dsp_bin_and_run fail %d\n", __func__, ret);
            adsp_priv->dsp_loading = 0;
        }
    }  else {
        hal_sleep_manager_unlock_sleep(SLEEP_LOCK_DEEP_SLEEP);
        hal_sleep_manager_unlock_sleep(SLEEP_LOCK_AUDIO);
        ret = hifixdsp_stop_run();
        if (ret) {
            aud_msg("%s hifixdsp_stop_run fail %d\n", __func__, ret);
        } else {
            adsp_priv->dsp_ready = 0;
            adsp_data->adsp_on = 0;
            //mt7933_adsp_low_power_uninit(priv);
        }
    }
    return ret;
}

static int adsp_cyberon_model_get(struct msd_ctl_value *value, void *priv)
{
    struct mt7933_adsp_private *adsp_priv = (struct mt7933_adsp_private *)priv;
    value->integer.value[0]  = adsp_priv->cyberon_model_type;
    return 0;
}

static int adsp_cyberon_model_put(struct msd_ctl_value *value, void *priv)
{
#ifdef CFG_DSPOTTER_SUPPORT
    struct mt7933_adsp_private *adsp_priv = (struct mt7933_adsp_private *)priv;
    int cyberon_module_type = value->integer.value[0];

    aud_msg("set_cyberon_mode = %d\n", cyberon_module_type);

    switch (cyberon_module_type) {
        case 0:
            adsp_priv->cyberon_model_pack_withTxt = (void *)g_lpdwHeySiri_OpenCamera_pack_withTxt;
            adsp_priv->cyberon_model_pack_withTxt_size = sizeof(g_lpdwHeySiri_OpenCamera_pack_withTxt);
            adsp_priv->cyberon_model_type = cyberon_module_type;
            break;
        case 1:
            adsp_priv->cyberon_model_pack_withTxt = g_lpdwDemo_ZhiNengGuanJia_pack_withTxt;
            adsp_priv->cyberon_model_pack_withTxt_size = sizeof(g_lpdwDemo_ZhiNengGuanJia_pack_withTxt);
            adsp_priv->cyberon_model_type = cyberon_module_type;
            break;
        case 2:
            adsp_priv->cyberon_model_pack_withTxt = g_lpdwEngAircon_pack_withTxt;
            adsp_priv->cyberon_model_pack_withTxt_size = sizeof(g_lpdwEngAircon_pack_withTxt);
            adsp_priv->cyberon_model_type = cyberon_module_type;
            break;
        default:
            aud_error("cyberon mode %d is not supported", cyberon_module_type);
    }
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */
    return 0;
}

static struct soc_ctl_entry mt7933_adsp_controls[] = {
    {
        .name = "ADSP_Enable",
        .get = adsp_enable_get,
        .set = adsp_enable_put,
    },
    {
        .name = "Cyberon_Model",
        .get = adsp_cyberon_model_get,
        .set = adsp_cyberon_model_put,
    },
};

/* ======================  Widgets & Route  ========================= */

static struct snd_widget mt7933_adsp_widgets[] = {
    DAI_WIDGET("FE_LOCAL_RECORD"),
    DAI_WIDGET("FE_AP_RECORD"),

    DAI_WIDGET("FE_HOSTLESS_VA"),
    DAI_WIDGET("FE_VA"),

    DAI_WIDGET("BE_UL9_IN Capture"),
    END_WIDGET("VA UL9 In"),

    DAI_WIDGET("BE_UL2_IN Capture"),
    END_WIDGET("VA UL2 In"),

    SWITCH_WIDGET("VA_UL9_IO", -1, NULL),

    SWITCH_WIDGET("VA_UL2_IO", -1, NULL),

#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    DAI_WIDGET("BE_DL2_OUT Playback"),
    DAI_WIDGET("BE_DL3_OUT Playback"),
    DAI_WIDGET("BE_DLM_OUT Playback"),
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
};


static struct snd_widget_route mt7933_adsp_route[] = {
    {"O_26", "VA_UL9_IO", 1, {NULL, NULL}},
    {"O_27", "VA_UL9_IO", 1, {NULL, NULL}},
    {"O_28", "VA_UL9_IO", 1, {NULL, NULL}},
    {"O_29", "VA_UL9_IO", 1, {NULL, NULL}},

    {"O_26", "VA_UL2_IO", 1, {NULL, NULL}},
    {"O_27", "VA_UL2_IO", 1, {NULL, NULL}},
    {"O_28", "VA_UL2_IO", 1, {NULL, NULL}},
    {"O_29", "VA_UL2_IO", 1, {NULL, NULL}},

    {"VA_UL9_IO", "FE_HOSTLESS_VA", 1, {NULL, NULL}},
    {"VA_UL9_IO", "FE_VA", 1, {NULL, NULL}},
    {"VA_UL9_IO", "FE_AP_RECORD", 1, {NULL, NULL}},

    {"VA_UL2_IO", "FE_HOSTLESS_VA", 1, {NULL, NULL}},
    {"VA_UL2_IO", "FE_VA", 1, {NULL, NULL}},
    {"VA_UL2_IO", "FE_AP_RECORD", 1, {NULL, NULL}},

    {"BE_UL9_IN Capture", "FE_HOSTLESS_VA", 1, {NULL, NULL}},
    {"BE_UL9_IN Capture", "FE_VA", 1, {NULL, NULL}},
    {"BE_UL9_IN Capture", "FE_AP_RECORD", 1, {NULL, NULL}},

    {"BE_UL2_IN Capture", "FE_HOSTLESS_VA", 1, {NULL, NULL}},
    {"BE_UL2_IN Capture", "FE_VA", 1, {NULL, NULL}},
    {"BE_UL2_IN Capture", "FE_AP_RECORD", 1, {NULL, NULL}},

    {"VA UL9 In", "BE_UL9_IN Capture", 1, {NULL, NULL}},

    {"VA UL2 In", "BE_UL9_IN Capture", 1, {NULL, NULL}},

#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    {"BE_DL2_OUT Playback", "I_40", 1, {NULL, NULL}},
    {"BE_DL2_OUT Playback", "I_41", 1, {NULL, NULL}},

    {"BE_DL3_OUT Playback", "I_20", 1, {NULL, NULL}},
    {"BE_DL3_OUT Playback", "I_21", 1, {NULL, NULL}},

    {"BE_DLM_OUT Playback", "I_22", 1, {NULL, NULL}},
    {"BE_DLM_OUT Playback", "I_23", 1, {NULL, NULL}},
    {"BE_DLM_OUT Playback", "I_24", 1, {NULL, NULL}},
    {"BE_DLM_OUT Playback", "I_25", 1, {NULL, NULL}},
    {"BE_DLM_OUT Playback", "I_26", 1, {NULL, NULL}},
    {"BE_DLM_OUT Playback", "I_27", 1, {NULL, NULL}},
    {"BE_DLM_OUT Playback", "I_28", 1, {NULL, NULL}},
    {"BE_DLM_OUT Playback", "I_29", 1, {NULL, NULL}},
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
};

/* ============================  DAI  =============================== */

static int mt7933_adsp_fe_startup(struct snd_pcm_stream *stream,
                                  struct snd_soc_dai *dai)
{
    struct mt7933_adsp_private *priv = dai->private_data;
    int scene = mt7933_adsp_get_scene_by_dai_id(dai->driver->id);
    int ret = 0;

    aud_msg("mt7933_adsp_fe_startup\n");

    if (!IS_ADSP_READY)
        return -ENODEV;

    ret = mt7933_adsp_send_ipi_cmd(NULL,
                                   scene,
                                   AUDIO_IPI_LAYER_TO_DSP,
                                   AUDIO_IPI_MSG_ONLY,
                                   AUDIO_IPI_MSG_NEED_ACK,
                                   MSG_TO_DSP_HOST_PORT_STARTUP,
                                   0, 0, NULL);

    priv->dai_mem[dai->driver->id].stream = stream;

    return ret;
}

static int mt7933_adsp_fe_hw_params(struct snd_pcm_stream *stream,
                                    struct msd_hw_params *params, struct snd_soc_dai *dai)
{
    struct mt7933_adsp_private *priv = dai->private_data;
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);
    struct mt7933_adsp_dai_memory *dai_mem = &priv->dai_mem[id];
    struct adsp_dma_ring_buf *adsp_dma = &dai_mem->adsp_dma;

    struct host_ipc_msg_hw_param ipc_hw_param, ack_hw_param;
    struct ipi_msg_t ipi_msg;

    unsigned long paddr;
    void *vaddr;
    int ret = 0;

    aud_msg("mt7933_adsp_fe_hw_params\n");

    if (!IS_ADSP_READY)
        return -ENODEV;

    if (scene < 0)
        return -EINVAL;

    memset(&ipi_msg, 0, sizeof(struct ipi_msg_t));

    ipc_hw_param.dai_id = mt7933_adsp_dai_id_pack(id);
    ipc_hw_param.sample_rate = params->rate;
    ipc_hw_param.channel_num = params->channels;
    ipc_hw_param.bitwidth = snd_pcm_format_width(params->format);
    ipc_hw_param.period_size = params->period_size;
    ipc_hw_param.period_count = params->period_count;

    ret = mt7933_adsp_send_ipi_cmd(&ipi_msg,
                                   scene,
                                   AUDIO_IPI_LAYER_TO_DSP,
                                   AUDIO_IPI_PAYLOAD,
                                   AUDIO_IPI_MSG_NEED_ACK,
                                   MSG_TO_DSP_HOST_HW_PARAMS,
                                   sizeof(ipc_hw_param),
                                   0,
                                   (char *)&ipc_hw_param);

    if (ret != 0)
        return ret;

    if (ipi_msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        aud_error("unexpected ack type %u\n", ipi_msg.ack_type);
        return -EINVAL;
    }


    AUDIO_IPC_COPY_DSP_HW_PARAM(ipi_msg.payload, &ack_hw_param);
    /* NOTICE: now only support DRAM share buffer */
    if (ack_hw_param.adsp_dma.mem_type != AFE_MEM_TYPE_PSRAM) {
        return -ENOMEM;
    }
#ifdef CFG_DSPOTTER_SUPPORT
    if (!ack_hw_param.success) {
        aud_error("ack error, %u", ack_hw_param.success);
        return -EINVAL;
    }
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */
    params->period_size = ack_hw_param.period_size;
    params->period_count = ack_hw_param.period_count;

    aud_dbg("ADSP FE HW Params: rate(%d), channels(%d), bitwidth(%d), period_size(%d), period_count(%d)\n",
            params->rate, params->channels, ipc_hw_param.bitwidth, params->period_size, params->period_count);

    if (id == MT7933_ADSP_FE_AP_RECORD || id == MT7933_ADSP_FE_VA) {
        paddr = ack_hw_param.adsp_dma.dma_paddr;
        paddr = adsp_hal_phys_addr_dsp2cpu(paddr);
        vaddr = adsp_get_shared_sysram_phys2virt(paddr);
        dai_mem->adsp_dma_control_paddr = (uint32_t)paddr;
        dai_mem->adsp_dma_control = (struct io_ipc_ring_buf_shared *)vaddr;

        /* config dma between adsp pcm driver and adsp */
        paddr = dai_mem->adsp_dma_control->start_addr;
        paddr = adsp_hal_phys_addr_dsp2cpu(paddr);
        vaddr = adsp_get_shared_sysram_phys2virt(paddr);
        adsp_dma->start_addr = (unsigned char *)vaddr;
        adsp_dma->size_bytes = dai_mem->adsp_dma_control->size_bytes;
        adsp_dma->hw_offset_bytes = dai_mem->adsp_dma_control->ptr_to_hw_offset_bytes;
        adsp_dma->appl_offset_bytes = dai_mem->adsp_dma_control->ptr_to_appl_offset_bytes;
    }

    return ret;
}

static int mt7933_adsp_fe_prepare(struct snd_pcm_stream *stream,
                                  struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);
    int ret = 0;

    aud_dbg("mt7933_adsp_fe_prepare\n");

    if (!IS_ADSP_READY)
        return -ENODEV;

    if (scene < 0)
        return -EINVAL;

    ret = mt7933_adsp_send_ipi_cmd(NULL,
                                   scene,
                                   AUDIO_IPI_LAYER_TO_DSP,
                                   AUDIO_IPI_MSG_ONLY,
                                   AUDIO_IPI_MSG_NEED_ACK,
                                   MSG_TO_DSP_HOST_PREPARE,
                                   0, 0, NULL);

    return ret;
}

static int mt7933_adsp_fe_trigger(struct snd_pcm_stream *stream,
                                  int cmd, struct snd_soc_dai *dai)
{
    struct mt7933_adsp_private *priv = dai->private_data;
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);
    struct mt7933_adsp_dai_memory *dai_mem = &priv->dai_mem[id];
    struct host_ipc_msg_trigger ipc_trigger;
    int ret = 0;

    aud_msg("mt7933_adsp_fe_trigger\n");

    if (!IS_ADSP_READY)
        return -ENODEV;

    ipc_trigger.dai_id = mt7933_adsp_dai_id_pack(id);

    switch (cmd) {
        case SND_PCM_TRIGGER_START:
            mt7933_adsp_send_ipi_cmd(NULL,
                                     scene,
                                     AUDIO_IPI_LAYER_TO_DSP,
                                     AUDIO_IPI_PAYLOAD,
                                     AUDIO_IPI_MSG_DIRECT_SEND,
                                     MSG_TO_DSP_HOST_TRIGGER_START,
                                     sizeof(ipc_trigger),
                                     0,
                                     (char *)&ipc_trigger);
            break;
        case SND_PCM_TRIGGER_STOP:
            mt7933_adsp_send_ipi_cmd(NULL,
                                     scene,
                                     AUDIO_IPI_LAYER_TO_DSP,
                                     AUDIO_IPI_PAYLOAD,
                                     AUDIO_IPI_MSG_DIRECT_SEND,
                                     MSG_TO_DSP_HOST_TRIGGER_STOP,
                                     sizeof(ipc_trigger),
                                     0,
                                     (char *)&ipc_trigger);
            mt7933_reset_dai_dma_offset(dai_mem);
            break;
        default:
            break;
    }
    return ret;
}

static int mt7933_adsp_fe_hw_free(struct snd_pcm_stream *stream,
                                  struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);
    struct host_ipc_msg_hw_free ipc_hw_free;
    int ret = 0;

    aud_msg("mt7933_adsp_fe_hw_free\n");

    if (!IS_ADSP_READY)
        return -ENODEV;

    ipc_hw_free.dai_id = mt7933_adsp_dai_id_pack(id);

    mt7933_adsp_send_ipi_cmd(NULL,
                             scene,
                             AUDIO_IPI_LAYER_TO_DSP,
                             AUDIO_IPI_PAYLOAD,
                             AUDIO_IPI_MSG_NEED_ACK,
                             MSG_TO_DSP_HOST_HW_FREE,
                             sizeof(ipc_hw_free),
                             0,
                             (char *)&ipc_hw_free);

    //snd_pcm_lib_free_pages(substream);

    return ret;
}

static void mt7933_adsp_fe_shutdown(struct snd_pcm_stream *stream,
                                    struct snd_soc_dai *dai)
{
    struct mt7933_adsp_private *priv = dai->private_data;
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);
    aud_msg("mt7933_adsp_fe_shutdown\n");

    if (!IS_ADSP_READY)
        return;

    if (scene < 0)
        return;

    mt7933_adsp_send_ipi_cmd(NULL,
                             scene,
                             AUDIO_IPI_LAYER_TO_DSP,
                             AUDIO_IPI_MSG_ONLY,
                             AUDIO_IPI_MSG_NEED_ACK,
                             MSG_TO_DSP_HOST_CLOSE,
                             0, 0, NULL);

    mt7933_reset_dai_memory(&priv->dai_mem[id]);
}

static int mt7933_adsp_fe_copy(struct snd_pcm_stream *stream, void *buf,
                               unsigned int bytes, struct snd_soc_dai *dai)
{
    struct snd_pcm_runtime *runtime = stream->runtime;
    struct mt7933_adsp_private *priv = dai->private_data;
    struct mt7933_adsp_dai_memory *dai_mem = &priv->dai_mem[dai->driver->id];

    unsigned char *pstart = (unsigned char *)(dai_mem->adsp_dma_control->start_addr);
    unsigned int pstart_ap_view = (unsigned int)adsp_hal_phys_addr_dsp2cpu((unsigned int)pstart);
    unsigned int buf_bytes = dai_mem->adsp_dma_control->size_bytes;
    unsigned int *pnew_a_ptr = (unsigned int *) & (dai_mem->adsp_dma_control->ptr_to_appl_offset_bytes);

    unsigned int a_ptr = (runtime->appl_ptr % runtime->buffer_size) * (runtime->channels * runtime->bit_width >> 3);

    if (a_ptr != *pnew_a_ptr) {
        aud_error("SW pointer offset(%d), SW pointer share with DSP(%d)\n", a_ptr, *pnew_a_ptr);
        return -1;
    }

    volatile_memcpy(buf, (void *)(pstart_ap_view + a_ptr), bytes);
    __asm__ __volatile__("dsb" : : : "memory");
    *pnew_a_ptr = (*pnew_a_ptr + bytes) % buf_bytes;

    dai_mem->adsp_dma.hw_offset_bytes = dai_mem->adsp_dma_control->ptr_to_hw_offset_bytes;
    dai_mem->adsp_dma.appl_offset_bytes = dai_mem->adsp_dma_control->ptr_to_appl_offset_bytes;
    return 0;
}

unsigned long mt7933_adsp_fe_pointer(struct snd_pcm_stream *stream, struct snd_soc_dai *dai)
{
    struct mt7933_adsp_private *priv = dai->private_data;
    struct mt7933_adsp_dai_memory *dai_mem = &priv->dai_mem[dai->driver->id];

    unsigned int adsp_dma_hw_off = 0;

    adsp_dma_hw_off = dai_mem->adsp_dma_control->ptr_to_hw_offset_bytes;
    aud_dev("id: %d, control addr = %p, adsp_dma_hw_off = %d\n",
            dai->driver->id, dai_mem->adsp_dma_control, adsp_dma_hw_off / 4);

    return bytes_to_frames(stream->runtime, adsp_dma_hw_off);
}

static const struct snd_soc_dai_pcm_ops mt7933_adsp_fe_ops = {
    .startup = mt7933_adsp_fe_startup,
    .hw_params = mt7933_adsp_fe_hw_params,
    .prepare = mt7933_adsp_fe_prepare,
    .trigger = mt7933_adsp_fe_trigger,
    .hw_free = mt7933_adsp_fe_hw_free,
    .shutdown = mt7933_adsp_fe_shutdown,
    .copy = mt7933_adsp_fe_copy,
    .pointer = mt7933_adsp_fe_pointer,
};

static int mt7933_adsp_be_startup(struct snd_pcm_stream *stream,
                                  struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);
    int ret = 0;
    aud_msg("mt7933_adsp_be_startup\n");

    ret = mt7933_adsp_send_ipi_cmd(NULL,
                                   scene,
                                   AUDIO_IPI_LAYER_TO_DSP,
                                   AUDIO_IPI_MSG_ONLY,
                                   AUDIO_IPI_MSG_NEED_ACK,
                                   MSG_TO_DSP_DSP_PORT_STARTUP,
                                   0, 0, NULL);

    aud_msg("mt7933_adsp_be_startup end\n");
    return ret;
}

static int mt7933_adsp_be_hw_params(struct snd_pcm_stream *stream,
                                    struct msd_hw_params *params, struct snd_soc_dai *dai)
{
    struct mt7933_adsp_private *priv = dai->private_data;
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);
    struct mt7933_adsp_be_dai_data *adsp_be =
            &priv->be_data[id - MT7933_ADSP_BE_START];

    int memif_id = mt7933_adsp_get_afe_memif_id(id);
    struct mtk_base_afe *afe = mt7933_afe_pcm_get_info();
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mtk_base_afe_memif *memif = &afe->memif[memif_id];
    struct mt7933_adsp_data *afe_adsp = &(afe_priv->adsp_data);

    struct host_ipc_msg_hw_param ipc_hw_param;
    struct dsp_ipc_msg_hw_param ack_hw_param;
    struct ipi_msg_t ipi_msg;
    int ret = 0;
    struct snd_pcm_runtime *runtime = stream->runtime;

    if (stream->runtime == NULL) {
        runtime = malloc(sizeof(struct snd_pcm_runtime));
        if (runtime == NULL) {
            aud_error("not enough memroy.");
            return -ENOMEM;
        }
        memset(runtime, 0, sizeof(*runtime));
        stream->runtime = runtime;
    }

    aud_msg("mt7933_adsp_be_hw_params\n");

    if (!IS_ADSP_READY)
        return -ENODEV;

    memset(&ipi_msg, 0, sizeof(ipi_msg));

    ipc_hw_param.dai_id = id - MT7933_ADSP_BE_START;
    ipc_hw_param.sample_rate = params->rate;
    ipc_hw_param.channel_num = params->channels;
    ipc_hw_param.bitwidth = snd_pcm_format_width(params->format);
    ipc_hw_param.period_size = params->period_size;
    ipc_hw_param.period_count = params->period_count;

    /* Config AFE DMA buffer */
    unsigned int bytes_per_frame = snd_pcm_format_width(params->format) * params->channels / 8;
    memif->buffer_size = params->period_size * params->period_count * bytes_per_frame;
    ipc_hw_param.adsp_dma.dma_paddr = 0;

    if (adsp_be->mem_type == AFE_MEM_TYPE_AFE_SRAM) {
        unsigned int paddr, size;

        afe_adsp->get_afe_memif_sram(afe, memif_id, &paddr, &size);
        if (paddr != 0 && memif->buffer_size < (int)size) {
            ipc_hw_param.adsp_dma.dma_paddr = paddr;
            ipc_hw_param.adsp_dma.mem_type = AFE_MEM_TYPE_AFE_SRAM;
        } else {
            ipc_hw_param.adsp_dma.mem_type = AFE_MEM_TYPE_PSRAM;
        }
    } else {
        ipc_hw_param.adsp_dma.mem_type = (uint32_t)adsp_be->mem_type;
    }

    ret = mt7933_adsp_send_ipi_cmd(&ipi_msg,
                                   scene,
                                   AUDIO_IPI_LAYER_TO_DSP,
                                   AUDIO_IPI_PAYLOAD,
                                   AUDIO_IPI_MSG_NEED_ACK,
                                   MSG_TO_DSP_DSP_HW_PARAMS,
                                   sizeof(ipc_hw_param),
                                   0,
                                   (char *)&ipc_hw_param);

    if (ret != 0)
        return ret;

    if (ipi_msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        aud_msg("unexpected ack type %u\n", ipi_msg.ack_type);
        return -EINVAL;
    }

    AUDIO_IPC_COPY_DSP_HW_PARAM(ipi_msg.payload, &ack_hw_param);
    if (ack_hw_param.adsp_dma.dma_paddr == 0) {
        return -ENOMEM;
    }

    runtime->buffer_size = ack_hw_param.period_size * ack_hw_param.period_count;
    runtime->period_size = ack_hw_param.period_size;
    runtime->rate = ack_hw_param.sample_rate;
    runtime->channels = ack_hw_param.channel_num;
    runtime->bit_width = ack_hw_param.bitwidth;
    runtime->bytes_per_frame = ack_hw_param.channel_num * ack_hw_param.bitwidth >> 3;

    aud_dbg("runtime info: rate(%d), channels(%d), bit_width(%d), bytes_per_frame(%d), period_size(%d), buffer_size(%d)\n",
            runtime->rate, runtime->channels, runtime->bit_width, runtime->bytes_per_frame, runtime->period_size, runtime->buffer_size);

    /* Notice: for AFE & DTCM do not need address convert */
    if (ack_hw_param.adsp_dma.mem_type != AFE_MEM_TYPE_AFE_SRAM)
        memif->phys_buf_addr =
            adsp_hal_phys_addr_dsp2cpu(ack_hw_param.adsp_dma.dma_paddr);
    else
        memif->phys_buf_addr = ack_hw_param.adsp_dma.dma_paddr;

    afe_adsp->set_afe_memif(afe,
                            memif_id,
                            ack_hw_param.sample_rate,
                            ack_hw_param.channel_num,
                            ack_hw_param.bitwidth);

    return 0;
}

static int mt7933_adsp_be_prepare(struct snd_pcm_stream *stream,
                                  struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);
    int ret = 0;

    aud_dbg("mt7933_adsp_be_prepare\n");
    aud_dbg("rate = %d, period_size = %d\n", stream->runtime->rate, stream->runtime->period_size);

    if (!IS_ADSP_READY)
        return -ENODEV;

    if (scene < 0)
        return -EINVAL;

    ret = mt7933_adsp_send_ipi_cmd(NULL,
                                   scene,
                                   AUDIO_IPI_LAYER_TO_DSP,
                                   AUDIO_IPI_MSG_ONLY,
                                   AUDIO_IPI_MSG_NEED_ACK,
                                   MSG_TO_DSP_DSP_PREPARE,
                                   0, 0, NULL);
    return ret;
}

static int mt7933_adsp_be_trigger(struct snd_pcm_stream *stream,
                                  int cmd, struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);
    int memif_id = mt7933_adsp_get_afe_memif_id(id);
    struct mtk_base_afe *afe = mt7933_afe_pcm_get_info();
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_adsp_data *afe_adsp = &(afe_priv->adsp_data);

    struct host_ipc_msg_trigger ipc_trigger;

    aud_msg("mt7933_adsp_be_trigger\n");
    aud_msg("rate = %d, period_size = %d\n", stream->runtime->rate, stream->runtime->period_size);

    if (!IS_ADSP_READY)
        return -ENODEV;

    if (scene < 0)
        return 0;

    ipc_trigger.dai_id = id - MT7933_ADSP_BE_START;
    switch (cmd) {
        case SND_PCM_TRIGGER_START:
            afe_adsp->set_afe_memif_enable(afe,
                                           memif_id,
                                           stream->runtime->rate,
                                           stream->runtime->period_size,
                                           1);
            mt7933_adsp_send_ipi_cmd(NULL,
                                     scene,
                                     AUDIO_IPI_LAYER_TO_DSP,
                                     AUDIO_IPI_PAYLOAD,
                                     AUDIO_IPI_MSG_DIRECT_SEND,
                                     MSG_TO_DSP_DSP_TRIGGER_START,
                                     sizeof(ipc_trigger),
                                     0,
                                     (char *)&ipc_trigger);
            break;
        case SND_PCM_TRIGGER_STOP:
            afe_adsp->set_afe_memif_enable(afe,
                                           memif_id,
                                           stream->runtime->rate,
                                           stream->runtime->period_size,
                                           0);
            mt7933_adsp_send_ipi_cmd(NULL,
                                     scene,
                                     AUDIO_IPI_LAYER_TO_DSP,
                                     AUDIO_IPI_PAYLOAD,
                                     AUDIO_IPI_MSG_DIRECT_SEND,
                                     MSG_TO_DSP_DSP_TRIGGER_STOP,
                                     sizeof(ipc_trigger),
                                     0,
                                     (char *)&ipc_trigger);
            break;
        default:
            break;
    }

    return 0;
}

static int mt7933_adsp_be_hw_free(struct snd_pcm_stream *stream,
                                  struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);

    if (!IS_ADSP_READY)
        return -ENODEV;

    if (scene < 0)
        return -EINVAL;

    mt7933_adsp_send_ipi_cmd(NULL,
                             scene,
                             AUDIO_IPI_LAYER_TO_DSP,
                             AUDIO_IPI_MSG_ONLY,
                             AUDIO_IPI_MSG_NEED_ACK,
                             MSG_TO_DSP_DSP_HW_FREE,
                             0, 0, NULL);

    aud_msg("mt7933_adsp_be_hw_free\n");
    return 0;
}

static void mt7933_adsp_be_shutdown(struct snd_pcm_stream *stream,
                                    struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int scene = mt7933_adsp_get_scene_by_dai_id(id);

    aud_msg("mt7933_adsp_be_shutdown\n");

    if (!IS_ADSP_READY)
        return;

    if (scene < 0)
        return;

    if (stream->runtime) {
        free(stream->runtime);
        stream->runtime = NULL;
    }

    mt7933_adsp_send_ipi_cmd(NULL,
                             scene,
                             AUDIO_IPI_LAYER_TO_DSP,
                             AUDIO_IPI_MSG_ONLY,
                             AUDIO_IPI_MSG_NEED_ACK,
                             MSG_TO_DSP_DSP_CLOSE,
                             0, 0, NULL);

}

static const struct snd_soc_dai_pcm_ops mt7933_adsp_be_ops = {
    .startup = mt7933_adsp_be_startup,
    .hw_params = mt7933_adsp_be_hw_params,
    .prepare = mt7933_adsp_be_prepare,
    .trigger = mt7933_adsp_be_trigger,
    .hw_free = mt7933_adsp_be_hw_free,
    .shutdown = mt7933_adsp_be_shutdown,
};

#ifdef CFG_AUDIO_DSP_PLAYBACK_EN

static int mt7933_adsp_playback_fe_startup(struct snd_pcm_stream *stream,
                                           struct snd_soc_dai *dai)
{
    struct mt7933_adsp_private *priv = dai->private_data;

    aud_msg("mt7933_adsp_playback_fe_startup\n");

    if (!IS_ADSP_READY) {
        aud_error("ADSP not ready!\n");
        return -ENODEV;
    }

    priv->dai_mem[dai->driver->id].stream = stream;

    return 0;
}

static const struct snd_soc_dai_pcm_ops mt7933_adsp_playback_fe_ops = {
    .startup    = mt7933_adsp_playback_fe_startup,
    //    .shutdown   = mt7933_adsp_playback_fe_shutdown,
    //    .hw_params  = mt7933_adsp_playback_fe_hw_params,
    //    .hw_free    = mt7933_adsp_playback_fe_hw_free,
    //    .prepare    = mt7933_adsp_playback_fe_prepare,
    //    .trigger    = mt7933_adsp_playback_fe_trigger,
};

static int mt7933_adsp_playback_be_startup(struct snd_pcm_stream *stream,
                                           struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int memif_id = mt7933_adsp_get_afe_memif_id(id);
    struct mtk_base_afe *afe = mt7933_afe_pcm_get_info();
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_adsp_data *afe_adsp = &(afe_priv->adsp_data);
    struct mtk_base_afe_memif *memif = NULL;

    if (memif_id < 0)
        return -EINVAL;

    memif = &afe->memif[memif_id];
    memif->stream = stream;

    mt7933_afe_enable_main_clk(afe);

    /* enable agent */
    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                  memif->data->agent_disable_reg,
                                  1 << memif->data->agent_disable_shift,
                                  0 << memif->data->agent_disable_shift);


    afe_adsp->irq_direction_enable(afe,
                                   memif->irq_usage,
                                   MT7933_AFE_IRQ_DIR_DSP);

    aud_dbg("%s", memif->data->name);
    aud_dbg("memif->irq_usage = %d", memif->irq_usage);
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);
    return 0;
}

static void mt7933_adsp_playback_be_shutdown(struct snd_pcm_stream *stream,
                                             struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int memif_id = mt7933_adsp_get_afe_memif_id(id);
    struct mtk_base_afe *afe = mt7933_afe_pcm_get_info();
    struct mtk_base_afe_memif *memif = NULL;

    if (memif_id < 0)
        return;

    memif = &afe->memif[memif_id];
    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
    aud_dbg("rate:%d, channel:%d, bit_width:%d", dai->rate, dai->channels, dai->bit_width);

    if (stream->runtime) {
        free(stream->runtime);
        stream->runtime = NULL;
    }

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->agent_disable_reg,
                                  1 << memif->data->agent_disable_shift,
                                  1 << memif->data->agent_disable_shift);

    mt7933_afe_disable_main_clk(afe);
}

static int mtk_dsp_playback_hw_params(struct snd_pcm_stream *stream,
                                      struct msd_hw_params *params,
                                      struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int memif_id = mt7933_adsp_get_afe_memif_id(id);
    struct mtk_base_afe *afe = mt7933_afe_pcm_get_info();
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_adsp_data *afe_adsp = &(afe_priv->adsp_data);
    struct mtk_base_afe_memif *memif = NULL;

    if (memif_id < 0)
        return -EINVAL;

    memif = &afe->memif[memif_id];

    struct snd_pcm_runtime *runtime = stream->runtime;

    if (stream->runtime == NULL) {
        runtime = malloc(sizeof(struct snd_pcm_runtime));
        if (runtime == NULL) {
            aud_error("not enough memroy.");
            return -ENOMEM;
        }
        memset(runtime, 0, sizeof(*runtime));
        stream->runtime = runtime;
    }

    runtime->period_size = params->period_size;

    struct snd_dma_buffer *dma_buf = &stream->runtime->dma_buf;
    unsigned int bytes_per_frame = snd_pcm_format_width(params->format) * params->channels / 8;
    dma_buf->size = params->period_size * params->period_count * bytes_per_frame;

#if 0
    unsigned int paddr_size;
    unsigned int paddr;
    unsigned int request_bytes = dma_buf->size + dai->driver->constr.buffer_bytes_align;
    afe_adsp->get_afe_memif_sram(afe, memif_id, &paddr, &paddr_size);

    dma_buf->base = (char *)paddr;
    if (!dma_buf->base || request_bytes > paddr_size)
        return -ENOMEM;

    unsigned int phy_base = (unsigned int)dma_buf->base;
    dma_buf->aligned_base = (char *)(((phy_base + request_bytes) & (~(dai->driver->constr.buffer_bytes_align - 1))) - dma_buf->size);

#else /* #if 0 */
    uint32_t paddr = memif_id;
    struct ipi_msg_t msg;
    int ret = 0;

    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_PAYLOAD,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PCM_PLAYBACK_GET_SINK_MEM,
              sizeof(uint32_t),
              0,
              &paddr);

    if (ret != 0)
        return ret;

    if (msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        aud_error("unexpected ack type %u", msg.ack_type);
        return -EINVAL;
    }

    memcpy(&paddr, msg.payload, sizeof(uint32_t));
    paddr = adsp_hal_phys_addr_dsp2cpu(paddr);
    dma_buf->base = (char *)paddr;

    dma_buf->aligned_base = dma_buf->base;

#endif /* #if 0 */

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

    fs = afe_adsp->get_afe_fs_timing(params->rate);

    if (fs < 0)
        return -EINVAL;

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->fs_reg,
                                  memif->data->fs_maskbit << memif->data->fs_shift,
                                  fs << memif->data->fs_shift);

    return 0;
}

static int mt7933_adsp_playback_be_hw_params(struct snd_pcm_stream *stream,
                                             struct msd_hw_params *params,
                                             struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int memif_id = mt7933_adsp_get_afe_memif_id(id);
    struct mtk_base_afe *afe = mt7933_afe_pcm_get_info();
    struct mtk_base_afe_memif *memif = NULL;

    if (memif_id < 0)
        return -EINVAL;

    memif = &afe->memif[memif_id];
    const struct mtk_base_memif_data *data = memif->data;
    unsigned int channels = params->channels;

    aud_dbg("dai_name:%s, dai_id:%d", dai->driver->name, dai->driver->id);
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

    return mtk_dsp_playback_hw_params(stream, params, dai);
}

static int mt7933_adsp_playback_be_prepare(struct snd_pcm_stream *stream,
                                           struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int memif_id = mt7933_adsp_get_afe_memif_id(id);
    struct mtk_base_afe *afe = mt7933_afe_pcm_get_info();
    struct mtk_base_afe_memif *memif = NULL;
    int hd_audio = 0;

    if (memif_id < 0)
        return -EINVAL;

    memif = &afe->memif[memif_id];

    aud_dbg("format = %d", dai->format);
    /* set hd mode */
    switch (dai->format) {
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
            aud_error("unsupported format %d\n", dai->format);
            break;
    }

    if (memif->data->hd_reg >= 0)
        aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE, memif->data->hd_reg,
                                      1 << memif->data->hd_shift,
                                      hd_audio << memif->data->hd_shift);

    return 0;
}

static int mt7933_adsp_playback_be_trigger(struct snd_pcm_stream *stream,
                                           int cmd,
                                           struct snd_soc_dai *dai)
{
    int id = dai->driver->id;
    int memif_id = mt7933_adsp_get_afe_memif_id(id);
    struct mtk_base_afe *afe = mt7933_afe_pcm_get_info();
    struct mt7933_afe_private *afe_priv = afe->platform_priv;
    struct mt7933_adsp_data *afe_adsp = &(afe_priv->adsp_data);
    struct mtk_base_afe_memif *memif = NULL;

    if (memif_id < 0)
        return -EINVAL;

    memif = &afe->memif[memif_id];

    struct mtk_base_afe_irq *irqs = &afe->irqs[memif->irq_usage];
    const struct mtk_base_irq_data *irq_data = irqs->irq_data;

    unsigned int counter = stream->runtime->period_size;
    int fs;

    aud_dbg("%s cmd = %d, counter = %u", memif->data->name, cmd, counter);

    switch (cmd) {
        case SND_PCM_TRIGGER_START:
            /* set irq counter */
            if (irq_data->irq_cnt_reg >= 0)
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                              irq_data->irq_cnt_reg,
                                              irq_data->irq_cnt_maskbit << irq_data->irq_cnt_shift,
                                              counter << irq_data->irq_cnt_shift);

            /* set irq fs */
            fs = afe_adsp->get_afe_fs_timing(dai->rate);

            if (fs < 0)
                return -EINVAL;

            if (irq_data->irq_fs_reg >= 0)
                aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_AFE_BASE,
                                              irq_data->irq_fs_reg,
                                              irq_data->irq_fs_maskbit << irq_data->irq_fs_shift,
                                              fs << irq_data->irq_fs_shift);

            return 0;
        case SND_PCM_TRIGGER_STOP:

            /* and clear pending IRQ */
            aud_drv_set_reg_addr_val(AUDIO_REGMAP_AFE_BASE, irq_data->irq_clr_reg,
                                     1 << irq_data->irq_clr_shift);
            return 0;
        default:
            return -EINVAL;
    }
}

static const struct snd_soc_dai_pcm_ops mt7933_adsp_playback_be_ops = {
    .startup    = mt7933_adsp_playback_be_startup,
    .hw_params  = mt7933_adsp_playback_be_hw_params,
    .prepare    = mt7933_adsp_playback_be_prepare,
    .shutdown   = mt7933_adsp_playback_be_shutdown,
    .trigger    = mt7933_adsp_playback_be_trigger,
};
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */

ATTR_RWDATA_IN_RAM static struct snd_soc_dai_driver mt7933_adsp_dais[] = {
    /* FE */
    {
        .name = "FE_LOCAL_RECORD",
        .id = MT7933_ADSP_FE_LOCAL_RECORD,
        .pcm_ops = &mt7933_adsp_fe_ops,
        .constr = {
            .stream_name = "FE_LOCAL_RECORD",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 16,
        },
    },
    {
        .name = "FE_AP_RECORD",
        .id = MT7933_ADSP_FE_AP_RECORD,
        .pcm_ops = &mt7933_adsp_fe_ops,
        .constr = {
            .stream_name = "FE_AP_RECORD",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 16,
        },
    },
    {
        .name = "FE_HOSTLESS_VA",
        .id = MT7933_ADSP_FE_HOSTLESS_VA,
        .pcm_ops = &mt7933_adsp_fe_ops,
        .constr = {
            .stream_name = "FE_HOSTLESS_VA",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 16,
        },
    },
    {
        .name = "FE_VA",
        .id = MT7933_ADSP_FE_VA,
        .pcm_ops = &mt7933_adsp_fe_ops,
        .constr = {
            .stream_name = "FE_VA",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 16,
        },
    },
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    {
        .name = "FE_HOSTLESS_PLAYBACK0",
        .id = MT7933_ADSP_FE_HOSTLESS_PLAYBACK0,
        .pcm_ops = &mt7933_adsp_playback_fe_ops,
        .constr = {
            .stream_name = "FE_HOSTLESS_PLAYBACK0",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 8,
        },
    },
    {
        .name = "FE_HOSTLESS_PLAYBACK1",
        .id = MT7933_ADSP_FE_HOSTLESS_PLAYBACK1,
        .pcm_ops = &mt7933_adsp_playback_fe_ops,
        .constr = {
            .stream_name = "FE_HOSTLESS_PLAYBACK1",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 8,
        },
    },
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    /* BE */
    {
        .name = "BE_UL9_IN Capture",
        .id = MT7933_ADSP_BE_UL9,
        .pcm_ops = &mt7933_adsp_be_ops,
        .constr = {
            .stream_name = "BE_UL9_IN Capture",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 16,
        },
    },
    {
        .name = "BE_UL2_IN Capture",
        .id = MT7933_ADSP_BE_UL2,
        .pcm_ops = &mt7933_adsp_be_ops,
        .constr = {
            .stream_name = "BE_UL2_IN Capture",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 16,
        },
    },
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    {
        .name = "BE_DL2_OUT Playback",
        .id = MT7933_ADSP_BE_DL2,
        .pcm_ops = &mt7933_adsp_playback_be_ops,
        .constr = {
            .stream_name = "BE_DL2_OUT Playback",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "BE_DL3_OUT Playback",
        .id = MT7933_ADSP_BE_DL3,
        .pcm_ops = &mt7933_adsp_playback_be_ops,
        .constr = {
            .stream_name = "BE_DL3_OUT Playback",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
    {
        .name = "BE_DLM_OUT Playback",
        .id = MT7933_ADSP_BE_DLM,
        .pcm_ops = &mt7933_adsp_playback_be_ops,
        .constr = {
            .stream_name = "BE_DLM_OUT Playback",
            .formats = MSD_PCM_FMT_S16_LE | MSD_PCM_FMT_S32_LE,
            .rates = SNDRV_PCM_RATE_8000_192000,
            .channels_min = 1,
            .channels_max = 2,
            .buffer_bytes_align = 64,
        },
    },
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
};

static int mt7933_adsp_parse_of(struct mt7933_adsp_private *priv)
{
    unsigned int i;
    aud_dbg("[%s] %p\n", __func__, priv);
#ifdef DSP_BOOT_RUN
    priv->dsp_boot_run = 1;
#else /* #ifdef DSP_BOOT_RUN */
    priv->dsp_boot_run = 0;
#endif /* #ifdef DSP_BOOT_RUN */
    aud_dbg("[%s] %d\n", __func__, priv->dsp_boot_run);

    struct {
        char *name;
        unsigned int val;
    } of_be_table[] = {
        { "ul9",    MT7933_ADSP_BE_UL9 },
    };

    for (i = 0; i < ARRAYSIZE(of_be_table); i++) {
        priv->be_data[i].mem_type = AFE_MEM_TYPE_AFE_SRAM;
    }

    return 0;
}

void *adsp_probe(void)
{
    int ret = 0;
    struct mt7933_adsp_private *adsp_priv = (struct mt7933_adsp_private *)malloc(sizeof(struct mt7933_adsp_private));
    if (adsp_priv == NULL)
        return NULL;

    aud_msg("[%s]\n", __func__);
    memset((void *)adsp_priv, 0x0, sizeof(struct mt7933_adsp_private));
#ifdef CFG_DSPOTTER_SUPPORT
    adsp_priv->cyberon_model_type = CYB_DSPOTTER_BASE_VERSION;

    switch (adsp_priv->cyberon_model_type) {
        case 0:
            adsp_priv->cyberon_model_pack_withTxt = (void *)g_lpdwHeySiri_OpenCamera_pack_withTxt;
            adsp_priv->cyberon_model_pack_withTxt_size = sizeof(g_lpdwHeySiri_OpenCamera_pack_withTxt);
            break;
        case 1:
            adsp_priv->cyberon_model_pack_withTxt = g_lpdwDemo_ZhiNengGuanJia_pack_withTxt;
            adsp_priv->cyberon_model_pack_withTxt_size = sizeof(g_lpdwDemo_ZhiNengGuanJia_pack_withTxt);
            break;
        case 2:
            adsp_priv->cyberon_model_pack_withTxt = g_lpdwEngAircon_pack_withTxt;
            adsp_priv->cyberon_model_pack_withTxt_size = sizeof(g_lpdwEngAircon_pack_withTxt);
            break;
        default:
            aud_error("cyberon mode %d is not supported", adsp_priv->cyberon_model_type);
            adsp_priv->cyberon_model_pack_withTxt = (void *)g_lpdwHeySiri_OpenCamera_pack_withTxt;
            adsp_priv->cyberon_model_pack_withTxt_size = sizeof(g_lpdwHeySiri_OpenCamera_pack_withTxt);
            adsp_priv->cyberon_model_type = 0;
            break;
    }
#endif /* #ifdef CFG_DSPOTTER_SUPPORT */
    audio_ipi_driver_init();
    mt7933_adsp_parse_of(adsp_priv);
    dsp2ap_irq_init();

    ret = mt7933_adsp_probe(adsp_priv);
    if (ret != 0) {
        aud_error("mt7933_adsp_probe error\n");
        goto error_exit;
    }

#ifdef CFG_WRAP_PCM_SUPPORT
    ret = wrap_dsp_msg_probe();
    if (ret != 0) {
        aud_error("wrap_dsp_msg_probe error\n");
        goto error_exit;
    }
#endif /* #ifdef CFG_WRAP_PCM_SUPPORT */

    snd_soc_add_widgets(mt7933_adsp_widgets, ARRAYSIZE(mt7933_adsp_widgets), adsp_priv);
    snd_soc_add_controls(mt7933_adsp_controls, ARRAYSIZE(mt7933_adsp_controls), adsp_priv);
    snd_soc_add_widget_routes(mt7933_adsp_route, ARRAYSIZE(mt7933_adsp_route));
    snd_soc_register_dais(mt7933_adsp_dais, ARRAYSIZE(mt7933_adsp_dais), adsp_priv);

#ifdef CFG_AUDIO_SUSPEND_EN
    mt7933_adsp_sleep_init();
#endif /* #ifdef CFG_AUDIO_SUSPEND_EN */

    g_adsp_priv = adsp_priv;

    return adsp_priv;

error_exit:
    audio_ipi_driver_exit();
    free(adsp_priv);
    return NULL;
}

void adsp_remove(void *priv)
{
    struct mt7933_adsp_private *adsp_priv = (struct mt7933_adsp_private *)priv;
    mt7933_adsp_cleanup_debug_dump(adsp_priv);
#ifdef CFG_WRAP_PCM_SUPPORT
    wrap_dsp_msg_remove();
#endif /* #ifdef CFG_WRAP_PCM_SUPPORT */
    mt7933_adsp_remove();
    dsp2ap_irq_uninit();
    audio_ipi_driver_exit();
    free(adsp_priv);

    snd_soc_unregister_dais(mt7933_adsp_dais);
    snd_soc_del_widget_routes(mt7933_adsp_route);
}

void aud_wdt_adsp_close(void)
{
    aud_msg("!!!ADSP Exception Trigger ADSP Close Start!!!\n");
    // close
#ifdef CFG_WRAP_PCM_SUPPORT
    wrap_dsp_msg_remove();
#endif /* #ifdef CFG_WRAP_PCM_SUPPORT */
    mt7933_adsp_remove();
    audio_ipi_driver_exit();
    aud_msg("!!!ADSP Exception Trigger ADSP Close Done!!!\n");
}

void aud_wdt_adsp_reboot(void)
{
    aud_msg("!!!ADSP Exception Trigger ADSP Reboot Start!!!\n");
    // re-open
    audio_ipi_driver_init();
#ifdef CFG_WRAP_PCM_SUPPORT
    wrap_dsp_msg_probe();
#endif /* #ifdef CFG_WRAP_PCM_SUPPORT */
    mt7933_adsp_probe(g_adsp_priv);
    aud_msg("!!!ADSP Exception Trigger ADSP Reboot Done!!!\n");
}

