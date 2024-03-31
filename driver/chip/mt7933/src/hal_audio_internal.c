#include <string.h>
#include "hal_log.h"
#include "FreeRTOS.h"
#include "task.h"

#if defined(HAL_AUDIO_MODULE_ENABLED)
#include "hal_audio_message_struct.h"
#include "audio_messenger_ipi.h"
#include "audio_shared_info.h"
#include "tinycontrol.h"
#include "tinypcm.h"
#include "mtk_hifixdsp_common.h"
#include "hal_audio_cm4_dsp_message.h"
//#include "hal_audio_internal_ex.h"

#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
#define SHARED_BUFFER_LEN 4096
static sound_t *g_snd[2];
static uint32_t g_dac_ref_count = 0;
#define LEAUDIO_PLAYBACK "ADSP_HOSTLESS_PLAYBACK0"
#define PROMPT_VOICE_PLAYBACK "ADSP_HOSTLESS_PLAYBACK1"

static n9_dsp_share_info_t *g_shared = NULL;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */

hal_audio_channel_number_t _channel_convert(uint32_t p_channels)
{
    hal_audio_channel_number_t channels = 0;
    switch (p_channels) {
        case 1:
            channels = HAL_AUDIO_MONO;
            break;
        case 2:
            channels = HAL_AUDIO_STEREO;
            break;
        case 3:
            channels = HAL_AUDIO_STEREO_BOTH_L_CHANNEL;
            break;
        case 4:
            channels = HAL_AUDIO_STEREO_BOTH_R_CHANNEL;
            break;
        case 5:
            channels = HAL_AUDIO_STEREO_BOTH_L_R_SWAP;
            break;
        default:
            log_hal_error("unknown channels!(%d)", p_channels);
            break;
    }

    return channels;
}

hal_audio_bits_per_sample_t _bit_type_convert(uint32_t p_type)
{
    hal_audio_bits_per_sample_t type = 0;
    switch (p_type) {
        case 16:
            type = HAL_AUDIO_BITS_PER_SAMPLING_16;
            break;
        case 24:
            type = HAL_AUDIO_BITS_PER_SAMPLING_24;
            break;
        default:
            log_hal_error("unknown bit type!(%d)", p_type);
            break;
    }

    return type;
}

hal_audio_sampling_rate_t _sample_rate_convert(uint32_t p_rate)
{
    hal_audio_sampling_rate_t rate = 0;
    switch (p_rate) {
        case 8000:
            rate = HAL_AUDIO_SAMPLING_RATE_8KHZ; /**< 8000Hz  */
            break;
        case 11025:
            rate = HAL_AUDIO_SAMPLING_RATE_11_025KHZ; /**< 11025Hz */
            break;
        case 12000:
            rate = HAL_AUDIO_SAMPLING_RATE_12KHZ; /**< 12000Hz */
            break;
        case 16000:
            rate = HAL_AUDIO_SAMPLING_RATE_16KHZ; /**< 16000Hz */
            break;
        case 22050:
            rate = HAL_AUDIO_SAMPLING_RATE_22_05KHZ; /**< 22050Hz */
            break;
        case 24000:
            rate = HAL_AUDIO_SAMPLING_RATE_24KHZ; /**< 24000Hz */
            break;
        case 32000:
            rate = HAL_AUDIO_SAMPLING_RATE_32KHZ; /**< 32000Hz */
            break;
        case 44100:
            rate = HAL_AUDIO_SAMPLING_RATE_44_1KHZ; /**< 44100Hz */
            break;
        case 48000:
            rate = HAL_AUDIO_SAMPLING_RATE_48KHZ; /**< 48000Hz */
            break;
        case 88200:
            rate = HAL_AUDIO_SAMPLING_RATE_88_2KHZ; /**< 88200Hz */
            break;
        case 96000:
            rate = HAL_AUDIO_SAMPLING_RATE_96KHZ;/**< 96000Hz */
            break;
        case 176400:
            rate = HAL_AUDIO_SAMPLING_RATE_176_4KHZ;/**< 176400Hz */
            break;
        case 192000:
            rate = HAL_AUDIO_SAMPLING_RATE_192KHZ;/**< 192000Hz */
            break;
        default:
            log_hal_error("unknown sampling rate!(%d)", p_rate);
            break;
    }
    return rate;
}

void hal_audio_lock(void) {};
void hal_audio_unlock(void) {};

int32_t _auddrv_cset(const char *ctrl_name, int32_t value_count, void *value)
{
    sound_t *snd = NULL;
    int32_t index;
    int32_t ret = 0;

    int32_t *pvalue = (int32_t *)value;

    struct msd_ctl_value ctl_value;
    memset(&ctl_value, 0, sizeof(ctl_value));
    strncpy(ctl_value.ctl_id.name, ctrl_name, MSD_CTL_ID_NAME_MAX_LEN - 1);
    for (index = 0; index < value_count; index++) {
        ctl_value.integer.value[index] = pvalue[index];
    }

    hal_audio_lock();

    ret = snd_ctl_open(&snd, "control");
    if (ret < 0) {
        log_hal_error("snd_ctl_open fail %d", ret);
        hal_audio_unlock();
        return ret;
    }

    snd_ctl_write(snd, &ctl_value);
    snd_ctl_close(snd);

    hal_audio_unlock();
    return 0;
}

int32_t _auddrv_cget(const char *ctrl_name, int32_t value_count, void *value)
{
    if (!ctrl_name) {
        log_hal_error("control_name is NULL");
        return -1;
    }

    sound_t *snd = NULL;
    int index;
    int ret = 0;

    int32_t *pvalue = (int32_t *)value;

    struct msd_ctl_value ctl_value;
    memset(&ctl_value, 0, sizeof(ctl_value));
    strncpy(ctl_value.ctl_id.name, ctrl_name, MSD_CTL_ID_NAME_MAX_LEN - 1);

    hal_audio_lock();

    ret = snd_ctl_open(&snd, "control");
    if (ret < 0) {
        log_hal_error("snd_ctl_open fail %d.", ret);
        hal_audio_unlock();
        return ret;
    }

    snd_ctl_read(snd, &ctl_value);
    snd_ctl_close(snd);

    hal_audio_unlock();

    for (index = 0; index < value_count; index ++) {
        pvalue[index] = ctl_value.integer.value[index];
    }
    return 0;
}

int32_t _auddrv_connect_route(const char *src, const char *sink, int32_t value, int32_t type)
{
    int32_t ret = 0;
    sound_t *snd = NULL;
    struct route r;
    struct msd_ctl_value ctl_value;

    r.src = src;
    r.sink = sink;
    r.value = value;
    memset(&ctl_value, 0, sizeof(ctl_value));
    ctl_value.ctl_id.id = type;
    strncpy(ctl_value.ctl_id.name, "io_route_ctl", MSD_CTL_ID_NAME_MAX_LEN - 1);
    memcpy(ctl_value.bytes.data, &r, sizeof(struct route));

    hal_audio_lock();

    ret = snd_ctl_open(&snd, "control");
    if (ret < 0) {
        log_hal_error("connect_route fail %d.\n", ret);
        hal_audio_unlock();
        return ret;
    }
    snd_ctl_write(snd, &ctl_value);
    snd_ctl_close(snd);

    hal_audio_unlock();
    return 0;
}

int32_t audio_drv_init(stream_params_t *stream_param, bool is_leaudio)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    int32_t ret = 0;

    taskENTER_CRITICAL();
    g_dac_ref_count++;

    if (is_leaudio) {
        _auddrv_connect_route(LEAUDIO_PLAYBACK, "ADSP_DL2_OUT_BE", 1, CONNECT_FE_BE);
        _auddrv_connect_route("ADSP_DL2_OUT_BE", "HW_Gain1_P", 1, CONNECT_FE_BE);
        _auddrv_connect_route("HW_Gain1_P", "INTDAC_out", 1, CONNECT_FE_BE);
        _auddrv_connect_route("I_40", "O_22", 1, CONNECT_IO_PORT);
        _auddrv_connect_route("I_41", "O_23", 1, CONNECT_IO_PORT);
        _auddrv_connect_route("I_30", "O_20", 1, CONNECT_IO_PORT);
        _auddrv_connect_route("I_31", "O_21", 1, CONNECT_IO_PORT);
    } else {
        _auddrv_connect_route("ADSP_HOSTLESS_PLAYBACK1", "ADSP_DL3_OUT_BE", 1, CONNECT_FE_BE);
        _auddrv_connect_route("ADSP_DL3_OUT_BE", "GASRC0_P", 1, CONNECT_FE_BE);
        _auddrv_connect_route("GASRC0_P", "HW_Gain1_P", 1, CONNECT_FE_BE);
        _auddrv_connect_route("HW_Gain1_P", "INTDAC_out", 1, CONNECT_FE_BE);
        _auddrv_connect_route("I_20", "O_42", 1, CONNECT_IO_PORT);
        _auddrv_connect_route("I_21", "O_43", 1, CONNECT_IO_PORT);
        _auddrv_connect_route("I_32", "O_22", 1, CONNECT_IO_PORT);
        _auddrv_connect_route("I_33", "O_23", 1, CONNECT_IO_PORT);
        _auddrv_connect_route("I_30", "O_20", 1, CONNECT_IO_PORT);
        _auddrv_connect_route("I_31", "O_21", 1, CONNECT_IO_PORT);
    }
    taskEXIT_CRITICAL();

    struct msd_hw_params hw_param = {0};

    hw_param.rate = stream_param->sample_rate;
    hw_param.channels = stream_param->channels;
    hw_param.period_size = stream_param->period_size;
    hw_param.period_count = stream_param->period_count;
    hw_param.format = stream_param->format;
    int8_t snd_index;
    char *name;

    if (is_leaudio) {
        snd_index = 0;
        name = LEAUDIO_PLAYBACK;
    } else {
        snd_index = 1;
        name = PROMPT_VOICE_PLAYBACK;
    }

    ret = snd_pcm_open(&g_snd[snd_index], name, 0, 0);
    if (ret)
        goto init_err_exit;
    ret = snd_pcm_hw_params(g_snd[snd_index], &hw_param);
    if (ret)
        goto init_err_exit;
    ret = snd_pcm_prepare(g_snd[snd_index]);
    if (ret)
        goto init_err_exit;
    ret = snd_pcm_start(g_snd[snd_index]);
    if (ret < 0)
        goto init_err_exit;

    return 0;

init_err_exit:
    log_hal_error("audio drv init fail(%d)", ret);
    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

int32_t audio_drv_deinit(bool is_leaudio)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    int32_t ret;
    int8_t snd_index;
    //uint32_t ref_count;

    if (is_leaudio) {
        snd_index = 0;
    } else {
        snd_index = 1;
    }

    ret = snd_pcm_drop(g_snd[snd_index]);
    if (ret)
        goto deinit_err_exit;
    ret = snd_pcm_hw_free(g_snd[snd_index]);
    if (ret)
        goto deinit_err_exit;
    ret = snd_pcm_close(g_snd[snd_index]);
    if (ret)
        goto deinit_err_exit;

    g_snd[snd_index] = NULL;

    taskENTER_CRITICAL();
    if (g_dac_ref_count)
        g_dac_ref_count--;
    if (is_leaudio) {
        _auddrv_connect_route("ADSP_HOSTLESS_PLAYBACK0", "ADSP_DL2_OUT_BE", 0, CONNECT_FE_BE);
        _auddrv_connect_route("ADSP_DL2_OUT_BE", "HW_Gain1_P", 0, CONNECT_FE_BE);
        if (!g_dac_ref_count)
            _auddrv_connect_route("HW_Gain1_P", "INTDAC_out", 0, CONNECT_FE_BE);
        _auddrv_connect_route("I_40", "O_22", 0, CONNECT_IO_PORT);
        _auddrv_connect_route("I_41", "O_23", 0, CONNECT_IO_PORT);
        if (!g_dac_ref_count) {
            _auddrv_connect_route("I_30", "O_20", 0, CONNECT_IO_PORT);
            _auddrv_connect_route("I_31", "O_21", 0, CONNECT_IO_PORT);
        }
    } else {
        _auddrv_connect_route("ADSP_HOSTLESS_PLAYBACK1", "ADSP_DL3_OUT_BE", 0, CONNECT_FE_BE);
        _auddrv_connect_route("ADSP_DL3_OUT_BE", "GASRC0_P", 0, CONNECT_FE_BE);
        _auddrv_connect_route("GASRC0_P", "HW_Gain1_P", 0, CONNECT_FE_BE);
        if (!g_dac_ref_count)
            _auddrv_connect_route("HW_Gain1_P", "INTDAC_out", 0, CONNECT_FE_BE);
        _auddrv_connect_route("I_20", "O_42", 0, CONNECT_IO_PORT);
        _auddrv_connect_route("I_21", "O_43", 0, CONNECT_IO_PORT);
        _auddrv_connect_route("I_32", "O_22", 0, CONNECT_IO_PORT);
        _auddrv_connect_route("I_33", "O_23", 0, CONNECT_IO_PORT);
        if (!g_dac_ref_count) {
            _auddrv_connect_route("I_30", "O_20", 0, CONNECT_IO_PORT);
            _auddrv_connect_route("I_31", "O_21", 0, CONNECT_IO_PORT);
        }
    }
    taskEXIT_CRITICAL();
    return ret;

deinit_err_exit:
    g_snd[snd_index] = NULL;
    log_hal_error("audio drv deinit fail(%d)", ret);
    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

int32_t audio_drv_set_volume(uint32_t volume)
{
    int32_t volm = volume;
    log_hal_error("volume:%d", volume);
    return _auddrv_cset("HW_Gain1_Volume", 1, &volm);
}

int32_t audio_ipi_dsp_open(stream_params_t *stream_param)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    int32_t ret = 0;

    unsigned long paddr;
    void *vaddr;

    mcu2dsp_open_param_t open_param;
    memset(&open_param, 0, sizeof(mcu2dsp_open_param_t));

    open_param.param.stream_in  = STREAM_IN_VP;
    open_param.param.stream_out = STREAM_OUT_AFE;

    open_param.stream_in_param.playback.channel_number = _channel_convert(stream_param->channels);
    open_param.stream_in_param.playback.bit_type = _bit_type_convert(stream_param->bit_type);
    open_param.stream_in_param.playback.sampling_rate = _sample_rate_convert(stream_param->sample_rate);
    open_param.stream_in_param.playback.codec_type = (uint8_t)AUDIO_DSP_CODEC_TYPE_PCM;

    open_param.stream_out_param.afe.audio_device = HAL_AUDIO_DEVICE_DAC_DUAL;
    open_param.stream_out_param.afe.stream_channel = HAL_AUDIO_DIRECT;
    open_param.stream_out_param.afe.audio_interface = HAL_AUDIO_INTERFACE_NONE;
    open_param.stream_out_param.afe.misc_parms   = DOWNLINK_PERFORMANCE_NORMAL;
    open_param.stream_out_param.afe.memory   = HAL_AUDIO_MEM2;
    open_param.stream_out_param.afe.format   = AFE_PCM_FORMAT_S16_LE;
    open_param.stream_out_param.afe.stream_out_sampling_rate   = stream_param->sample_rate;
    open_param.stream_out_param.afe.sampling_rate    = stream_param->sample_rate;
    open_param.stream_out_param.afe.irq_period   = 1000 * stream_param->period_size / stream_param->sample_rate;//frameDuration
    open_param.stream_out_param.afe.frame_size   = stream_param->period_size;//480
    open_param.stream_out_param.afe.frame_number     = stream_param->period_count;//2
    open_param.stream_out_param.afe.hw_gain  = false;

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_PAYLOAD,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PROMPT_OPEN,
              sizeof(mcu2dsp_open_param_t),
              0,
              &open_param);

    if (msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        log_hal_error("unexpected ack type %u", msg.ack_type);
        return -1;
    }

    memcpy(&open_param, msg.payload, sizeof(mcu2dsp_open_param_t));

    if (open_param.stream_in_param.playback.p_share_info == 0) {
        log_hal_error("shared mem err!");
        ret = -1;
    } else {
        log_hal_debug("p_share_info = 0x%08x!", (uint32_t)open_param.stream_in_param.playback.p_share_info);

        paddr = (unsigned long)open_param.stream_in_param.playback.p_share_info;
        paddr = adsp_hal_phys_addr_dsp2cpu(paddr);
        vaddr = adsp_get_shared_sysram_phys2virt(paddr);
        g_shared = (n9_dsp_share_info_t *)vaddr;

        /* config dma between adsp pcm driver and adsp */
        log_hal_debug("start_addr = 0x%08x!", g_shared->start_addr);

        log_hal_debug("length = %d!", g_shared->length);
        log_hal_debug("read_offset = 0x%08x, write_offset = 0x%08x!", g_shared->read_offset, g_shared->write_offset);
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

int32_t audio_ipi_dsp_close(void)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    int32_t ret = 0;

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PROMPT_CLOSE,
              0,
              0,
              NULL);

    if (ret) {
        log_hal_error("close failed!");
    }

    g_shared = NULL;
    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

int32_t audio_ipi_dsp_start(void)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    /* start */
    int32_t ret = 0;
    mcu2dsp_start_param_t start_param;
    start_param.param.stream_in = STREAM_IN_VP;
    start_param.param.stream_out = STREAM_OUT_AFE;

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_PAYLOAD,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PROMPT_START,
              sizeof(start_param),
              0,
              &start_param);

    if (ret) {
        log_hal_error("start failed!(%d)", ret);
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

/* ipi message */
int32_t audio_ipi_dsp_stop(void)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    /* stop */
    int32_t ret = 0;

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PROMPT_STOP,
              0,
              0,
              NULL);

    if (ret) {
        log_hal_error("stop failed!(%d)", ret);
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

#if 0//voice promp doesn't need to suspend and resume.
int32_t audio_ipi_dsp_suspend(void)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    /* suspend */
    int32_t ret = 0;
    log_hal_info("%s", __FUNCTION__);

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PCM_PLAYBACK_SUSPEND,
              0,
              0,
              NULL);

    if (ret) {
        log_hal_error("stop failed!(%d)", ret);
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

int32_t audio_ipi_dsp_resume(void)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    /* resume */
    int32_t ret = 0;
    log_hal_info("%s", __FUNCTION__);

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PCM_PLAYBACK_RESUME,
              0,
              0,
              NULL);

    if (ret) {
        log_hal_error("stop failed!(%d)", ret);
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}
#endif /* #if 0//voice promp doesn't need to suspend and resume. */

void shared_lock(void) {}
void shared_unlock(void) {}

uint16_t _copy_shared(void *buffer, uint32_t data_size)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    if (buffer == NULL)
        return -1;

    n9_dsp_share_info_t *shared = g_shared;
    if (shared->start_addr == 0) {
        return -1;
    }

    uint16_t free_size;
    uint16_t byte_count;
    uint16_t unwrap_size;
    uint16_t read_offset;

    shared_lock();
    read_offset = shared->read_offset;
    free_size = (shared->length - (shared->write_offset - read_offset)) % shared->length;

    if ((free_size == 0) && (shared->bBufferIsFull == 0))
        free_size = shared->length;

    if ((shared->bBufferIsFull == 1) && (shared->write_offset != read_offset)) {
        shared->bBufferIsFull = 0;
    }

    if (shared->bBufferIsFull || !free_size || (free_size < data_size)) {
        shared_unlock();
        return 0;
    }

    uint32_t p_start_addr = adsp_hal_phys_addr_dsp2cpu(shared->start_addr);

    byte_count = 0;
    if (read_offset > shared->write_offset) {
        memcpy((void *)(p_start_addr + shared->write_offset), buffer, data_size);
        shared->write_offset += data_size;
    } else {
        unwrap_size = shared->length - shared->write_offset;
        if (unwrap_size >= data_size) {
            memcpy((void *)(p_start_addr + shared->write_offset), buffer, data_size);
            shared->write_offset += data_size;
        } else {
            memcpy((void *)(p_start_addr + shared->write_offset), buffer, unwrap_size);
            memcpy((void *)p_start_addr, buffer + unwrap_size, data_size - unwrap_size);
            shared->write_offset = data_size - unwrap_size;
        }
    }

    byte_count += data_size;

    if (shared->write_offset == shared->length)
        shared->write_offset = 0;

    if (shared->write_offset == read_offset) {
        shared->bBufferIsFull = 1;
    }

    shared_unlock();

    return byte_count;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

/*****************DSP leaudio******************/
#ifdef CFG_AUDIO_DSP_LEAUDIO_EN
int32_t le_audio_ipi_dsp_open(stream_params_t *stream_param)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    int32_t ret = 0;

    mcu2dsp_open_param_t open_param;
    memset(&open_param, 0, sizeof(mcu2dsp_open_param_t));

    open_param.param.stream_in  = STREAM_IN_PLAYBACK;
    open_param.param.stream_out = STREAM_OUT_AFE;

    open_param.stream_in_param.playback.channel_number = _channel_convert(stream_param->channels);
    open_param.stream_in_param.playback.bit_type = _bit_type_convert(stream_param->bit_type);
    open_param.stream_in_param.playback.sampling_rate = _sample_rate_convert(stream_param->sample_rate);
    open_param.stream_in_param.playback.codec_type = (uint8_t)AUDIO_DSP_CODEC_TYPE_PCM;
    if (stream_param->channels == 1) {
        open_param.stream_out_param.afe.audio_device = HAL_AUDIO_DEVICE_DAC_L;
    } else {
        open_param.stream_out_param.afe.audio_device = HAL_AUDIO_DEVICE_DAC_DUAL;
    }
    open_param.stream_out_param.afe.stream_channel = HAL_AUDIO_DIRECT;
    open_param.stream_out_param.afe.audio_interface = HAL_AUDIO_INTERFACE_NONE;
    open_param.stream_out_param.afe.misc_parms   = DOWNLINK_PERFORMANCE_NORMAL;
    open_param.stream_out_param.afe.memory   = HAL_AUDIO_MEM1;
    open_param.stream_out_param.afe.format   = AFE_PCM_FORMAT_S16_LE;
    open_param.stream_out_param.afe.stream_out_sampling_rate   = stream_param->sample_rate;
    open_param.stream_out_param.afe.sampling_rate    = stream_param->sample_rate;
    open_param.stream_out_param.afe.irq_period   = 1000 * stream_param->period_size / stream_param->sample_rate;//frameDuration
    open_param.stream_out_param.afe.frame_size   = stream_param->period_size;//480
    open_param.stream_out_param.afe.frame_number     = stream_param->period_count;//4
    open_param.stream_out_param.afe.hw_gain  = false;

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_PAYLOAD,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PCM_PLAYBACK_OPEN,
              sizeof(mcu2dsp_open_param_t),
              0,
              &open_param);

    if (msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        log_hal_error("unexpected ack type: %u", msg.ack_type);
        return -1;
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

int32_t le_audio_ipi_dsp_close(void)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    int32_t ret = 0;

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PCM_PLAYBACK_CLOSE,
              0,
              0,
              NULL);

    if (ret) {
        log_hal_error("close failed!");
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

int32_t le_audio_ipi_dsp_start(void)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    /* start */
    int32_t ret = 0;
    mcu2dsp_start_param_t start_param;
    start_param.param.stream_in = STREAM_IN_PLAYBACK;
    start_param.param.stream_out = STREAM_OUT_AFE;

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_PAYLOAD,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PCM_PLAYBACK_START,
              sizeof(start_param),
              0,
              &start_param);

    if (ret) {
        log_hal_error("start failed!(%d)", ret);
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

/* ipi message */
int32_t le_audio_ipi_dsp_stop(void)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    /* stop */
    int32_t ret = 0;

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PCM_PLAYBACK_STOP,
              0,
              0,
              NULL);

    if (ret) {
        log_hal_error("stop failed!(%d)", ret);
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

int32_t le_audio_ipi_dsp_suspend(void)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    /* suspend */
    int32_t ret = 0;

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PCM_PLAYBACK_SUSPEND,
              0,
              0,
              NULL);

    if (ret) {
        log_hal_error("stop failed!(%d)", ret);
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}

int32_t le_audio_ipi_dsp_resume(void)
{
#ifdef CFG_AUDIO_DSP_PLAYBACK_EN
    /* resume */
    int32_t ret = 0;

    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_PLAYBACK_MSG,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_PCM_PLAYBACK_RESUME,
              0,
              0,
              NULL);

    if (ret) {
        log_hal_error("stop failed!(%d)", ret);
    }

    return ret;
#else /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
    return -1;
#endif /* #ifdef CFG_AUDIO_DSP_PLAYBACK_EN */
}
#endif /* #ifdef CFG_AUDIO_DSP_LEAUDIO_EN */

#endif /* #if defined(HAL_AUDIO_MODULE_ENABLED) */

