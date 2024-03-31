//#include "arch_timer.h"
#include <string.h>
#include <stdbool.h>
//#include <FreeRTOS.h>

#include "hal_gpio_internal.h"
#include "hal_gpt.h"

#include "cli.h"
#include "sound/utils/include/aud_log.h"
#include "sound/include/asound.h"
#include "sound/include/tinypcm.h"
#include "sound/include/tinycompress.h"
#include "sound/driver/include/compress_params.h"
#include "sound/utils/include/afe_reg_rw.h"
#include "sound/utils/include/aud_memory.h"
#include "va.h"
#include "audio_test_utils.h"
#include "hal_psram.h"
#include "audio_task.h"
#include "hal_boot.h"

#ifdef FILE_SYS_SUPPORT
#include "ff.h"
#endif /* #ifdef FILE_SYS_SUPPORT */

#ifdef CFG_DRV_MUSIC_DATA_EN
extern int holiday_model[];
extern int model_size;
#endif /* #ifdef CFG_DRV_MUSIC_DATA_EN */

extern int afe_sgen_golden_table_32bits[];
extern int table_size;
extern int afe_sgen_golden_table_16bits[];
extern int table_size_16bit;
extern short tone1k_48khz_1ch[];
extern short tone1k_48khz_2ch[];
extern int tone1k_table_size;

#define CONNETC_SRC_SINK_NAME_LEN   128
#define ALSA_DAI_NAME_LEN           128

struct audio_task_params {
    char fe_name[ALSA_DAI_NAME_LEN];
    int channel_num;
    int bitdepth;
    int sample_rate;
    int period_size;
    int period_count;
    int time_len;
    int dump_count;
    int disable_task;
};

#ifdef AUD_CODEC_SUPPORT
static void dlm_gsrc_intdac(int channel_num, int bitdepth, int sample_rate,
                            int period_size, int period_count, int time_len)
{
    sound_t *w_snd;
    int ret;
    int index;
    struct msd_hw_params params;

    if (bitdepth != 16 && bitdepth != 32) {
        aud_error("bitdepth error: %d", bitdepth);
        return;
    }

#ifndef CFG_DRV_MUSIC_DATA_EN
    int golden_size;
    void *golden_src;

    switch (bitdepth) {
        case 32:
            golden_src = afe_sgen_golden_table_32bits;
            golden_size = table_size;
            break;

        case 16:
            golden_src = afe_sgen_golden_table_16bits;
            golden_size = table_size_16bit;
            break;

        default:
            aud_error("bitdepth error: %d", bitdepth);
            return;
    }
#endif /* #ifndef CFG_DRV_MUSIC_DATA_EN */

#ifdef CFG_DRV_MUSIC_DATA_EN
    params.format = MSD_PCM_FMT_S16_LE;
    params.channels = 2;
    params.period_count = 4;
    params.period_size = 640;
    params.rate = 8000;

    int bytes_per_frame = 16 * params.channels / 8;
#else /* #ifdef CFG_DRV_MUSIC_DATA_EN */
    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    int bytes_per_frame = bitdepth * params.channels / 8;
#endif /* #ifdef CFG_DRV_MUSIC_DATA_EN */
    int total_transfer = 0;

#ifndef CFG_DRV_MUSIC_DATA_EN
    int w_data_size = bytes_per_frame * params.period_size;
    void *w_data_src = malloc(w_data_size);
    if (!w_data_src) {
        aud_error("malloc error: size = 0x%x", w_data_size);
        return;
    }
    memset(w_data_src, 0, w_data_size);
    aud_msg("data_src = %p, data_size = 0x%x", w_data_src, w_data_size);

    for (index = 0; index < w_data_size / golden_size; index++) {
        memcpy(w_data_src + index * golden_size, golden_src, golden_size);
    }
#endif /* #ifndef CFG_DRV_MUSIC_DATA_EN */

    connect_route("track0", "GASRC0_P", 1, CONNECT_FE_BE);
    connect_route("GASRC0_P", "INTDAC_out", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_42", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_43", 1, CONNECT_IO_PORT);
    connect_route("I_32", "O_20", 1, CONNECT_IO_PORT);
    connect_route("I_33", "O_21", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;
#ifdef CFG_DRV_MUSIC_DATA_EN
    for (index = 0; index < 2; index ++) {
        total_transfer = 0;
        while (total_transfer < (int)sizeof(int) * model_size - (int)params.period_size * bytes_per_frame) {
            aud_dbg("+snd_pcm_write %d; index %d", params.period_size, index);
            ret = snd_pcm_write(w_snd, (char *)holiday_model + total_transfer,
                                params.period_size);
            if (ret != (int)params.period_size)
                aud_error("ret: %d", ret);
            total_transfer += params.period_size * bytes_per_frame;
            aud_dbg("-snd_pcm_write %d; index %d", ret, index);
        }
    }
#else /* #ifdef CFG_DRV_MUSIC_DATA_EN */
    total_transfer = time_len * params.rate;
    while (total_transfer > 0) {
        ret = snd_pcm_write(w_snd, w_data_src, params.period_size);
        if (ret != (int)params.period_size)
            aud_error("ret: %d", ret);
        total_transfer = total_transfer - ret;
    }

#endif /* #ifdef CFG_DRV_MUSIC_DATA_EN */
    ret = snd_pcm_drop(w_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
#ifndef CFG_DRV_MUSIC_DATA_EN
    free(w_data_src);
#endif /* #ifndef CFG_DRV_MUSIC_DATA_EN */
    connect_route("track0", "GASRC0_P", 0, CONNECT_FE_BE);
    connect_route("GASRC0_P", "INTDAC_out", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_42", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_43", 0, CONNECT_IO_PORT);
    connect_route("I_32", "O_20", 0, CONNECT_IO_PORT);
    connect_route("I_33", "O_21", 0, CONNECT_IO_PORT);
}

void dlm_intdac(int channel_num, int bitdepth, int sample_rate,
                int period_size, int period_count, int time_len)
{
    sound_t *w_snd;
    int ret, index;
    void *golden_src;
    int golden_size;
    struct msd_hw_params params;

    switch (bitdepth) {
        case 32:
            golden_src = afe_sgen_golden_table_32bits;
            golden_size = table_size;
            break;

        case 16:
            golden_src = afe_sgen_golden_table_16bits;
            golden_size = table_size_16bit;
            break;

        default:
            aud_error("bitdepth error: %d", bitdepth);
            return;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    int bytes_per_frame = bitdepth * params.channels / 8;
    int data_size = bytes_per_frame * params.period_size * params.period_count / golden_size * golden_size;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }

    memset(data_src, 0, data_size);

    for (index = 0; index < data_size / golden_size; index++) {
        memcpy(data_src + index * golden_size, golden_src, golden_size);
    }

    aud_msg("data_src = %p, data_size = 0x%x", data_src, data_size);

    connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;

    int total_frames = time_len * params.rate;
    while (total_frames > 0) {
        ret = snd_pcm_write(w_snd, data_src, data_size / bytes_per_frame);
        if (ret != data_size / bytes_per_frame)
            aud_msg("ret: %d", ret);
        total_frames = total_frames - ret;
    }

    ret = snd_pcm_drain(w_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);

    connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
}

#ifdef FILE_SYS_SUPPORT
void dlm_intdac_file(int channel_num, int bitdepth, int sample_rate,
                     int period_size, int period_count, int time_len,
                     char *file_name)
{
    sound_t *w_snd;
    int ret;
    struct msd_hw_params params;

    FIL fid;
    FRESULT f_ret;
    UINT f_br;

    if (bitdepth != 32 && bitdepth != 16) {
        aud_error("bitdepth error: %d", bitdepth);
        return;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    int bytes_per_frame = bitdepth * params.channels / 8;
    int data_size = bytes_per_frame * params.period_size * params.period_count;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }

    memset(data_src, 0, data_size);

    f_ret = f_open(&fid, file_name, FA_READ);
    if (f_ret) {
        aud_error("f_open error: %d", f_ret);
        goto exit0;
    }
    aud_msg("data_src = %p, data_size = 0x%x", data_src, data_size);

    connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit1;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit2;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit2;

    while (!f_eof(&fid)) {
        f_read(&fid, data_src, data_size, &f_br);

        ret = snd_pcm_write(w_snd, data_src, f_br / bytes_per_frame);
        if (ret != (int)f_br / bytes_per_frame)
            aud_msg("ret: %d", ret);
    }

    ret = snd_pcm_drain(w_snd);
    if (ret)
        goto exit2;

exit2:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit1:
    f_close(&fid);

exit0:
    free(data_src);

    connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
}
#endif /* #ifdef FILE_SYS_SUPPORT */
#endif /* #ifdef AUD_CODEC_SUPPORT */

void dlm_etdmout2(int channel_num, int bitdepth, int sample_rate,
                  int period_size, int period_count, int time_len)
{
    sound_t *w_snd;
    int ret, index;
    const void *golden_src;
    int golden_size;
    struct msd_hw_params params;

    switch (bitdepth) {
        case 32:
            golden_src = afe_sgen_golden_table_32bits;
            golden_size = table_size;
            break;

        case 16:
            golden_src = afe_sgen_golden_table_16bits;
            golden_size = table_size_16bit;
            break;

        default:
            aud_error("bitdepth error: %d", bitdepth);
            return;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;
    int bytes_per_frame = bitdepth * params.channels / 8;
    int data_size = bytes_per_frame * params.period_size * params.period_count / golden_size * golden_size;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }

    memset(data_src, 0, data_size);

    for (index = 0; index < data_size / golden_size; index++) {
        memcpy(data_src + index * golden_size, golden_src, golden_size);
    }

    connect_route("track0", "ETDM2_OUT_BE", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_04", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_05", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit0;

    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;

    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;

    int total_frames = time_len * params.rate;
    while (total_frames > 0) {
        ret = snd_pcm_write(w_snd, data_src, data_size / bytes_per_frame);
        if (ret != data_size / bytes_per_frame)
            aud_msg("ret: %d", ret);
        total_frames = total_frames - ret;
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);
    connect_route("track0", "ETDM2_OUT_BE", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_04", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_05", 0, CONNECT_IO_PORT);
}

#ifdef AUD_CODEC_SUPPORT
void ul3_intadc(int channel_num, int bitdepth, int sample_rate,
                int period_size, int period_count, int time_len)
{
    sound_t *w_snd;
    int ret;
    struct msd_hw_params params;

    if (bitdepth != 16 && bitdepth != 32) {
        aud_error("bitdepth error: %d", bitdepth);
        return;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    int bytes_per_frame = bitdepth * params.channels / 8;
    int data_frames = 2 * params.rate;
    int data_size = bytes_per_frame * data_frames;
    int total_frames = time_len * params.rate;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("UL3", "INTADC_in", 1, CONNECT_FE_BE);
#ifdef PINMUX_QFN_DEFAULT
    connect_route("I_60", "O_14", 1, CONNECT_IO_PORT);
    connect_route("I_61", "O_15", 1, CONNECT_IO_PORT);
#elif defined PINMUX_BGA_DEFAULT
    connect_route("I_60", "O_14", 1, CONNECT_IO_PORT);
    if (hal_boot_get_hw_ver() == 0x8A00) {
        connect_route("I_08", "O_15", 1, CONNECT_IO_PORT);
    } else {
        connect_route("I_61", "O_15", 1, CONNECT_IO_PORT);
    }
#endif /* #ifdef PINMUX_QFN_DEFAULT */

    ret = snd_pcm_open(&w_snd, "UL3", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;

    while (total_frames > 0) {
        ret = snd_pcm_read(w_snd, data_src, data_frames);
        if (ret != data_frames)
            aud_msg("ret: %d", ret);
        total_frames -= ret;
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);
    connect_route("UL3", "INTADC_in", 0, CONNECT_FE_BE);
#ifdef PINMUX_QFN_DEFAULT
    connect_route("I_60", "O_14", 0, CONNECT_IO_PORT);
    connect_route("I_61", "O_15", 0, CONNECT_IO_PORT);
#elif defined PINMUX_BGA_DEFAULT
    connect_route("I_60", "O_14", 0, CONNECT_IO_PORT);
    if (hal_boot_get_hw_ver() != 0x8A00) {
        connect_route("I_61", "O_15", 0, CONNECT_IO_PORT);
    } else {
        connect_route("I_08", "O_15", 0, CONNECT_IO_PORT);
    }
#endif /* #ifdef PINMUX_QFN_DEFAULT */
}

#ifdef FILE_SYS_SUPPORT
void ul3_intadc_file(int channel_num, int bitdepth, int sample_rate,
                     int period_size, int period_count, int time_len,
                     char *file_name)
{
    sound_t *w_snd;
    int ret;
    struct msd_hw_params params;

    if (bitdepth != 16 && bitdepth != 32) {
        aud_error("bitdepth error: %d", bitdepth);
        return;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    int bytes_per_frame = bitdepth * params.channels / 8;
    int data_frames = params.period_count * params.period_size;
    int data_size = bytes_per_frame * data_frames;
    int total_frames = time_len * params.rate;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    FIL fid;
    FRESULT f_ret;
    UINT f_bw;

    f_ret = f_open(&fid, file_name, FA_CREATE_ALWAYS | FA_WRITE);
    if (f_ret) {
        aud_error("f_open %s error: %d", file_name, f_ret);
        goto exit0;
    }

    connect_route("UL3", "INTADC_in", 1, CONNECT_FE_BE);
#ifdef PINMUX_QFN_DEFAULT
    connect_route("I_60", "O_14", 1, CONNECT_IO_PORT);
    connect_route("I_61", "O_15", 1, CONNECT_IO_PORT);
#elif defined PINMUX_BGA_DEFAULT
    connect_route("I_60", "O_14", 1, CONNECT_IO_PORT);
    if (hal_boot_get_hw_ver() != 0x8A00) {
        connect_route("I_61", "O_15", 1, CONNECT_IO_PORT);
    } else {
        connect_route("I_08", "O_15", 1, CONNECT_IO_PORT);
    }
#endif /* #ifdef PINMUX_QFN_DEFAULT */

    ret = snd_pcm_open(&w_snd, "UL3", 0, 0);
    if (ret)
        goto exit1;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit2;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit2;

    while (total_frames > 0) {
        ret = snd_pcm_read(w_snd, data_src, data_frames);
        if (ret != data_frames)
            aud_msg("ret: %d", ret);
        f_write(&fid, data_src, ret * bytes_per_frame, &f_bw);
        if ((int)f_bw != ret * bytes_per_frame)
            aud_msg("f_write error");
        total_frames -= ret;
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        goto exit2;

exit2:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit1:
    f_close(&fid);

exit0:
    free(data_src);
    connect_route("UL3", "INTADC_in", 0, CONNECT_FE_BE);
#ifdef PINMUX_QFN_DEFAULT
    connect_route("I_60", "O_14", 0, CONNECT_IO_PORT);
    connect_route("I_61", "O_15", 0, CONNECT_IO_PORT);
#elif defined PINMUX_BGA_DEFAULT
    connect_route("I_60", "O_14", 0, CONNECT_IO_PORT);
    if (hal_boot_get_hw_ver() != 0x8A00) {
        connect_route("I_61", "O_15", 0, CONNECT_IO_PORT);
    } else {
        connect_route("I_08", "O_15", 0, CONNECT_IO_PORT);
    }
#endif /* #ifdef PINMUX_QFN_DEFAULT */
}
#endif /* #ifdef FILE_SYS_SUPPORT */
#endif /* #ifdef AUD_CODEC_SUPPORT */

static void ul3_dmic(void)
{
    sound_t *w_snd;
    int ret, index;
    struct msd_hw_params params;
    params.format = MSD_PCM_FMT_S16_LE;
    params.channels = 2;
    params.period_count = 4;
    params.period_size = 640;
    params.rate = 8000;
    int bytes_per_frame = 16 * params.channels / 8;
    int data_frames = 10 * params.rate;
    int data_size = bytes_per_frame * data_frames;
    //  unsigned int total_transfer = 0;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("UL3", "DMIC_BE", 1, CONNECT_FE_BE);
    connect_route("I_04", "O_14", 1, CONNECT_IO_PORT);
    connect_route("I_05", "O_15", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "UL3", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;
    for (index = 0; index < data_frames / (int)params.period_size; index ++) {
        ret = snd_pcm_read(w_snd, data_src + index * params.period_size * bytes_per_frame,
                           params.period_size);
        if (ret != (int)params.period_size)
            aud_msg("ret: %d", ret);
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);
    connect_route("UL3", "DMIC_BE", 0, CONNECT_FE_BE);
    connect_route("I_04", "O_14", 0, CONNECT_IO_PORT);
    connect_route("I_05", "O_15", 0, CONNECT_IO_PORT);
}

void ul9_dmic(int channel_num, int bitdepth, int sample_rate,
              int period_size, int period_count, int time_len)
{
    sound_t *w_snd;
    int ret;
    struct msd_hw_params params;

    if (bitdepth != 16 && bitdepth != 32) {
        aud_error("bitdepth error: %d", bitdepth);
        return;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;
    int bytes_per_frame = bitdepth * params.channels / 8;
    int data_frames = 2 * params.rate;
    int data_size = bytes_per_frame * data_frames;
    //  unsigned int total_transfer = 0;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("UL9", "DMIC_BE", 1, CONNECT_FE_BE);
    connect_route("I_04", "O_26", 1, CONNECT_IO_PORT);
    connect_route("I_05", "O_27", 1, CONNECT_IO_PORT);
    connect_route("I_06", "O_28", 1, CONNECT_IO_PORT);
    connect_route("I_07", "O_29", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "UL9", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;
    int total_frames = time_len * params.rate;
    while (total_frames > 0) {
        ret = snd_pcm_read(w_snd, data_src, data_frames);
        if (ret != data_frames)
            aud_msg("ret: %d", ret);
        total_frames -= ret;
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);

    connect_route("UL9", "DMIC_BE", 0, CONNECT_FE_BE);
    connect_route("I_04", "O_26", 0, CONNECT_IO_PORT);
    connect_route("I_05", "O_27", 0, CONNECT_IO_PORT);
    connect_route("I_06", "O_28", 0, CONNECT_IO_PORT);
    connect_route("I_07", "O_29", 0, CONNECT_IO_PORT);
}

#ifdef AUD_CODEC_SUPPORT
static void ul9_intadc(void)
{
    sound_t *r_snd;
    int ret = 0;
    int index;

    struct msd_hw_params r_params;
    r_params.format = MSD_PCM_FMT_S16_LE;
    r_params.channels = 4;
    r_params.period_count = 4;
    r_params.period_size = 640;
    r_params.rate = 8000;

    int bytes_per_frame = 16 * r_params.channels / 8;
    int data_frames = 2 * r_params.rate;
    int data_size = bytes_per_frame * data_frames;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("UL9", "INTADC_in", 1, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 1, CONNECT_IO_PORT);
    connect_route("I_61", "O_27", 1, CONNECT_IO_PORT);
    connect_route("I_08", "O_28", 1, CONNECT_IO_PORT);
    connect_route("I_09", "O_29", 1, CONNECT_IO_PORT);


    ret = snd_pcm_open(&r_snd, "UL9", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(r_snd, &r_params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit1;
    for (index = 0; index < data_frames / (int)r_params.period_size; index ++) {
        ret = snd_pcm_read(r_snd, (char *)data_src + index * r_params.period_size * bytes_per_frame,
                           r_params.period_size);
        if (ret != (int)r_params.period_size)
            aud_msg("ret: %d", ret);
    }

    ret = snd_pcm_drop(r_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);

    connect_route("UL9", "INTADC_in", 0, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 0, CONNECT_IO_PORT);
    connect_route("I_61", "O_27", 0, CONNECT_IO_PORT);
    connect_route("I_08", "O_28", 0, CONNECT_IO_PORT);
    connect_route("I_09", "O_29", 0, CONNECT_IO_PORT);

}
#endif /* #ifdef AUD_CODEC_SUPPORT */

void ul9_etdmin2(int channel_num, int bitdepth, int sample_rate,
                 int period_size, int period_count, int time_len)
{
    sound_t *r_snd;
    int ret = 0;

    if (bitdepth != 16 && bitdepth != 32) {
        aud_error("bitdepth error: %d", bitdepth);
        return;
    }

    struct msd_hw_params r_params;
    r_params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    r_params.channels = channel_num;
    r_params.period_count = period_count;
    r_params.period_size = period_size;
    r_params.rate = sample_rate;

    int bytes_per_frame = bitdepth * r_params.channels / 8;
    int data_frames = 1 * r_params.rate;
    int data_size = bytes_per_frame * data_frames;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("UL9", "ETDM2_IN_BE", 1, CONNECT_FE_BE);
    connect_route("I_12", "O_26", 1, CONNECT_IO_PORT);
    connect_route("I_13", "O_27", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&r_snd, "UL9", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(r_snd, &r_params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit1;
    int total_frames = time_len * r_params.rate;
    while (total_frames > 0) {
        ret = snd_pcm_read(r_snd, data_src, data_frames);
        if (ret != data_frames)
            aud_msg("ret: %d", ret);
        total_frames -= ret;
    }

    ret = snd_pcm_drop(r_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);
    connect_route("UL9", "ETDM2_IN_BE", 0, CONNECT_FE_BE);
    connect_route("I_12", "O_26", 0, CONNECT_IO_PORT);
    connect_route("I_13", "O_27", 0, CONNECT_IO_PORT);

}

void ul9_etdmin1(int channel_num, int bitdepth, int sample_rate,
                 int period_size, int period_count, int time_len)
{
    sound_t *r_snd;
    int ret = 0;
    struct msd_hw_params r_params;

    if (bitdepth != 16 && bitdepth != 32) {
        aud_error("bitdepth error: %d", bitdepth);
        return;
    }

    r_params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    r_params.channels = channel_num;
    r_params.period_count = period_count;
    r_params.period_size = period_size;
    r_params.rate = sample_rate;

    int bytes_per_frame = bitdepth * r_params.channels / 8;
    int data_frames = 1 * r_params.rate;
    int data_size = bytes_per_frame * data_frames;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("UL9", "ETDM1_IN_BE", 1, CONNECT_FE_BE);
    connect_route("I_42", "O_26", 1, CONNECT_IO_PORT);
    connect_route("I_43", "O_27", 1, CONNECT_IO_PORT);
    connect_route("I_44", "O_28", 1, CONNECT_IO_PORT);
    connect_route("I_45", "O_29", 1, CONNECT_IO_PORT);
    connect_route("I_46", "O_30", 1, CONNECT_IO_PORT);
    connect_route("I_47", "O_31", 1, CONNECT_IO_PORT);
    connect_route("I_48", "O_32", 1, CONNECT_IO_PORT);
    connect_route("I_49", "O_33", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&r_snd, "UL9", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(r_snd, &r_params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit1;

    int total_frames = time_len * r_params.rate;
    while (total_frames > 0) {
        ret = snd_pcm_read(r_snd, data_src, data_frames);
        if (ret != data_frames)
            aud_msg("ret: %d", ret);
        total_frames = total_frames - ret;
    }

    ret = snd_pcm_drop(r_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);
    connect_route("UL9", "ETDM1_IN_BE", 0, CONNECT_FE_BE);
    connect_route("I_42", "O_26", 0, CONNECT_IO_PORT);
    connect_route("I_43", "O_27", 0, CONNECT_IO_PORT);
    connect_route("I_44", "O_28", 0, CONNECT_IO_PORT);
    connect_route("I_45", "O_29", 0, CONNECT_IO_PORT);
    connect_route("I_46", "O_30", 0, CONNECT_IO_PORT);
    connect_route("I_47", "O_31", 0, CONNECT_IO_PORT);
    connect_route("I_48", "O_32", 0, CONNECT_IO_PORT);
    connect_route("I_49", "O_33", 0, CONNECT_IO_PORT);

}

#ifdef AUD_CODEC_SUPPORT
#ifdef CFG_DRV_MUSIC_DATA_EN

static void ul9_intadc_and_dlm(void)
{
    sound_t *r_snd;
    sound_t *w_snd;
    int ret = 0;
    int index;

    struct msd_hw_params r_params;
    r_params.format = MSD_PCM_FMT_S16_LE;
    r_params.channels = 4;
    r_params.period_count = 4;
    r_params.period_size = 640;
    r_params.rate = 8000;

    struct msd_hw_params w_params;
    w_params.format = MSD_PCM_FMT_S16_LE;
    w_params.channels = 2;
    w_params.period_count = 4;
    w_params.period_size = 640;
    w_params.rate = 8000;

    unsigned int total_transfer;

    int w_bytes_per_frame = 16 * w_params.channels / 8;
    int r_bytes_per_frame = 16 * r_params.channels / 8;

    int data_frames = 2 * r_params.rate;
    int data_size = r_bytes_per_frame * data_frames;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, sizeof(data_src));

    connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);
    connect_route("I_22", "O_28", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_29", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &w_params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;

    connect_route("UL9", "INTADC_in", 1, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 1, CONNECT_IO_PORT);
    connect_route("I_61", "O_27", 1, CONNECT_IO_PORT);


    ret = snd_pcm_open(&r_snd, "UL9", 0, 0);
    if (ret)
        goto exit2;
    ret = snd_pcm_hw_params(r_snd, &r_params);
    if (ret)
        goto exit3;
    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit3;

    unsigned int bytes_p = 0;

    for (index = 0; index < 2; index ++) {
        total_transfer = 0;
        while (total_transfer < sizeof(int) * model_size - w_params.period_size * w_bytes_per_frame) {
            ret = snd_pcm_write(w_snd, (char *)holiday_model + total_transfer,
                                w_params.period_size);
            if (ret != (int)w_params.period_size)
                aud_error("ret: %d", ret);
            total_transfer += w_params.period_size * w_bytes_per_frame;
            if ((int)bytes_p + r_bytes_per_frame * (int)r_params.period_size > data_size)
                goto exit4;
            ret = snd_pcm_read(r_snd, data_src + bytes_p, r_params.period_size);
            bytes_p += ret * r_bytes_per_frame;
            if (ret != (int)r_params.period_size)
                aud_error("ret: %d", ret);
        }
    }

exit4:

    ret = snd_pcm_drop(w_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);


    //  if (ret)
    //      return;
    //  for (index = 0; index < data_frames / r_params.period_size; index ++) {
    //      ret = snd_pcm_read(r_snd, (char *)data_src + index * r_params.period_size * bytes_per_frame,
    //                 r_params.period_size);
    //      if (ret != r_params.period_size)
    //          aud_msg("ret: %d", ret);
    //  }

    ret = snd_pcm_drop(r_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

exit3:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit2:
    connect_route("UL9", "INTADC_in", 0, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 0, CONNECT_IO_PORT);
    connect_route("I_61", "O_27", 0, CONNECT_IO_PORT);

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);
    connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
    connect_route("I_22", "O_28", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_29", 0, CONNECT_IO_PORT);

}
#else /* #ifdef CFG_DRV_MUSIC_DATA_EN */
static void ul9_intadc_and_dlm(void)
{
    aud_msg("");
}
#endif /* #ifdef CFG_DRV_MUSIC_DATA_EN */

#ifdef CFG_DRV_MUSIC_DATA_EN

static void ul9_intadc_and_gasrc(void)
{
    sound_t *r_snd;
    sound_t *w_snd;
    int ret = 0;
    int index;

    struct msd_hw_params r_params;
    r_params.format = MSD_PCM_FMT_S16_LE;
    r_params.channels = 4;
    r_params.period_count = 4;
    r_params.period_size = 3840;
    r_params.rate = 48000;

    struct msd_hw_params w_params;
    w_params.format = MSD_PCM_FMT_S16_LE;
    w_params.channels = 2;
    w_params.period_count = 4;
    w_params.period_size = 640;
    w_params.rate = 8000;

    unsigned int total_transfer;

    int w_bytes_per_frame = 16 * w_params.channels / 8;
    int r_bytes_per_frame = 16 * r_params.channels / 8;

    int data_frames = 2 * r_params.rate;
    int data_size = r_bytes_per_frame * data_frames;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, sizeof(data_src));

    connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
    connect_route("track0", "GASRC0_P", 1, CONNECT_FE_BE);
    connect_route("GASRC0_P", "dummy_end_p", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);
    connect_route("I_22", "O_42", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_43", 1, CONNECT_IO_PORT);

    connect_route("UL9", "INTADC_in", 1, CONNECT_FE_BE);
    connect_route("UL9", "GASRC0_C", 1, CONNECT_FE_BE);
    connect_route("GASRC0_C", "dummy_end_c", 1, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 1, CONNECT_IO_PORT);
    connect_route("I_61", "O_27", 1, CONNECT_IO_PORT);
    connect_route("I_32", "O_28", 1, CONNECT_IO_PORT);
    connect_route("I_33", "O_29", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &w_params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;

    ret = snd_pcm_open(&r_snd, "UL9", 0, 0);
    if (ret)
        goto exit1;
    ret = snd_pcm_hw_params(r_snd, &r_params);
    if (ret)
        goto exit2;
    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit2;

    unsigned int bytes_p = 0;

    for (index = 0; index < 2; index ++) {
        total_transfer = 0;
        while (total_transfer < sizeof(int) * model_size - w_params.period_size * w_bytes_per_frame) {
            ret = snd_pcm_write(w_snd, (char *)holiday_model + total_transfer,
                                w_params.period_size);
            if (ret != (int)w_params.period_size)
                aud_error("ret: %d", ret);
            total_transfer += w_params.period_size * w_bytes_per_frame;
            if ((int)bytes_p + r_bytes_per_frame * (int)r_params.period_size > data_size)
                goto exit3;
            ret = snd_pcm_read(r_snd, (char *)data_src + bytes_p, r_params.period_size);
            bytes_p += r_params.period_size * r_bytes_per_frame;
            if (ret != (int)r_params.period_size)
                aud_error("ret: %d", ret);
        }
    }

exit3:
    ret = snd_pcm_drop(w_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);
    ret = snd_pcm_drop(r_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

exit2:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);

    connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
    connect_route("track0", "GASRC0_P", 0, CONNECT_FE_BE);
    connect_route("GASRC0_P", "dummy_end_p", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
    connect_route("I_22", "O_42", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_43", 0, CONNECT_IO_PORT);

    connect_route("UL9", "INTADC_in", 0, CONNECT_FE_BE);
    connect_route("UL9", "GASRC0_C", 0, CONNECT_FE_BE);
    connect_route("GASRC0_C", "dummy_end_c", 0, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 0, CONNECT_IO_PORT);
    connect_route("I_61", "O_27", 0, CONNECT_IO_PORT);
    connect_route("I_32", "O_28", 0, CONNECT_IO_PORT);
    connect_route("I_33", "O_29", 0, CONNECT_IO_PORT);

}
#else /* #ifdef CFG_DRV_MUSIC_DATA_EN */
static void ul9_intadc_and_gasrc(void)
{
    aud_msg("");
}
#endif /* #ifdef CFG_DRV_MUSIC_DATA_EN */

static int ul10_intadc_from_intdac(void)
{
    sound_t *w_snd;
    sound_t *r_snd;
    int ret = 0;
    int index;
    struct msd_hw_params w_params;
    w_params.format = MSD_PCM_FMT_S32_LE;
    w_params.channels = 2;
    w_params.period_count = 4;
    w_params.period_size = 640;
    w_params.rate = 8000;

    int bytes_per_frame = 32 * w_params.channels / 8;

    int w_data_size = bytes_per_frame * w_params.period_size * 4;

    void *w_data_src = malloc(w_data_size);
    if (!w_data_src) {
        printf("%s: memory error\r\n", __FUNCTION__);
        return -1;
    }
    memset(w_data_src, 0, w_data_size);
    aud_msg("w_data_src = %p, w_data_size = %d", w_data_src, w_data_size);

    const void *golden_src = afe_sgen_golden_table_32bits;
    int golden_size = table_size;

    for (index = 0; index < w_data_size / golden_size; index++) {
        memcpy(w_data_src + index * golden_size, golden_src, golden_size);
    }
    aud_msg("golden_src = %p, golden_size = %d", golden_src, golden_size);

    struct msd_hw_params r_params;
    r_params.format = MSD_PCM_FMT_S32_LE;
    r_params.channels = 2;
    r_params.period_count = 4;
    r_params.period_size = 640;
    r_params.rate = 8000;

    int r_data_size = bytes_per_frame * r_params.period_size;

    void *r_data_src = malloc(r_data_size);
    if (!r_data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        ret = -1;
        goto exit0;
    }

    printf("r_data_src = %p, r_data_size = 0x%x\r\n", r_data_src, r_data_size);
    memset(r_data_src, 0, r_data_size);

    connect_route("UL10", "INTADC_in", 1, CONNECT_FE_BE);
    connect_route("I_60", "O_12", 1, CONNECT_IO_PORT);
    connect_route("I_61", "O_13", 1, CONNECT_IO_PORT);
    int value[1] = {1};
    control_cset("Audio_Downlink_Loopback", 1, value);
    control_cget("Audio_Downlink_Loopback", 1);

    connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);


    // timing: playback first, read second
    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit1;
    ret = snd_pcm_hw_params(w_snd, &w_params);
    if (ret)
        goto exit2;
    ret = snd_pcm_open(&r_snd, "UL10", 0, 0);
    if (ret)
        goto exit2;
    ret = snd_pcm_hw_params(r_snd, &r_params);
    if (ret)
        goto exit3;

    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit3;

    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit3;

    ret = snd_pcm_write(w_snd, w_data_src, w_params.period_size * 4);
    if (ret != (int)w_params.period_size * 4)
        aud_error("ret: %d", ret);

    for (index = 0; index < (int)r_params.period_size / (int)r_params.period_size * 1000; index ++) {
        ret = snd_pcm_write(w_snd, w_data_src, w_params.period_size * 4);
        if (ret != (int)w_params.period_size * 4)
            aud_error("ret: %d", ret);

        ret = snd_pcm_read(r_snd, r_data_src, r_params.period_size * 4);
        if (ret != (int)r_params.period_size * 4)
            aud_msg("ret: %d", ret);
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

    ret = snd_pcm_drop(r_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

exit3:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit2:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit1:
    connect_route("UL10", "INTADC_in", 0, CONNECT_FE_BE);
    connect_route("I_60", "O_12", 0, CONNECT_IO_PORT);
    connect_route("I_61", "O_13", 0, CONNECT_IO_PORT);

    int close_value[1] = {0};
    control_cset("Audio_Downlink_Loopback", 1, close_value);
    control_cget("Audio_Downlink_Loopback", 1);

    connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
    free(r_data_src);

exit0:
    free(w_data_src);

    return ret;
}

static int ul10_intadc_from_intdac_with_freq_check(int in_ch, int out_ch, int in_rate,
                                                   int out_rate, int period_size, int period_count, int bit_depth)
{
    sound_t *w_snd;
    sound_t *r_snd;
    int ret = 0;
    int index;
    void *golden_src = NULL;
    int golden_size = tone1k_table_size;
    unsigned int test_duration = (in_rate > 16000) ? 500 : 800; //ms
    unsigned int frequency[2] = {0, 0};

    if (in_ch != 1 && in_ch != 2) {
        aud_msg("%s: input channel error(%d)", __FUNCTION__, in_ch);
        return -1;
    }

    if (bit_depth != 16 && bit_depth != 32) {
        aud_msg("%s: input bit depth error(%d)", __FUNCTION__, bit_depth);
        return -1;
    }

    switch (out_ch) {
        case 1:
            golden_src = tone1k_48khz_1ch;
            golden_size = tone1k_table_size;
            break;

        case 2:
            golden_src = tone1k_48khz_2ch;
            golden_size = 2 * tone1k_table_size;
            break;

        default:
            aud_error("%s: output channel error(%d)", __FUNCTION__, out_ch);
            return -1;
    }

    struct msd_hw_params w_params;
    w_params.format = MSD_PCM_FMT_S16_LE;
    w_params.channels = out_ch;
    w_params.period_count = 4;
    w_params.period_size = 480;
    w_params.rate = out_rate;

    int bytes_per_frame = (w_params.format == MSD_PCM_FMT_S16_LE ? 16 : 32) * w_params.channels / 8;

    int w_data_size = bytes_per_frame * w_params.period_size;
    void *w_data_src = malloc(w_data_size);
    if (!w_data_src) {
        aud_error("%s: memory error", __FUNCTION__);
        return -1;
    }
    memset(w_data_src, 0, w_data_size);
    aud_msg("w_data_src = %p, w_data_size = %d", w_data_src, w_data_size);

    for (index = 0; index < w_data_size / golden_size; index++)
        memcpy(w_data_src + index * golden_size, golden_src, golden_size);

    aud_msg("golden_src = %p, golden_size = %d", golden_src, golden_size);

    struct msd_hw_params r_params;
    r_params.format = bit_depth == 32 ? MSD_PCM_FMT_S32_LE : MSD_PCM_FMT_S16_LE;
    r_params.channels = in_ch;
    r_params.period_count = period_count;
    r_params.period_size = period_size;
    r_params.rate = in_rate;

    int in_bytes_per_frame = bit_depth * r_params.channels / 8;

    int r_data_size = test_duration * r_params.rate * in_bytes_per_frame / 1000;

    void *r_data_src = malloc(r_data_size);
    if (!r_data_src) {
        aud_error("%s: memory error", __FUNCTION__);
        ret = -1;
        goto exit0;
    }

    aud_msg("r_data_src = %p, r_data_size = %d", r_data_src, r_data_size);
    memset(r_data_src, 0, r_data_size);

    connect_route("UL10", "INTADC_in", 1, CONNECT_FE_BE);
    connect_route("I_60", "O_12", 1, CONNECT_IO_PORT);
    connect_route("I_61", "O_13", 1, CONNECT_IO_PORT);

    connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);


    // timing: playback first, read second
    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit1;

    ret = snd_pcm_open(&r_snd, "UL10", 0, 0);
    if (ret)
        goto exit1;

    snd_pcm_hw_params(w_snd, &w_params);

    snd_pcm_hw_params(r_snd, &r_params);

    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit2;

    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit2;

    for (index = 0; index < (int)w_params.period_count; index++) {
        ret = snd_pcm_write(w_snd, w_data_src, w_params.period_size);
        if (ret != (int)w_params.period_size)
            aud_error("ret: %d", ret);
    }

    unsigned int total_read_size = 0;
    unsigned int read_frames = w_params.period_size * r_params.rate / w_params.rate;

    while (total_read_size + in_bytes_per_frame * read_frames <= (unsigned int)r_data_size) {
        ret = snd_pcm_write(w_snd, w_data_src, w_params.period_size);
        if (ret != (int)w_params.period_size)
            aud_error("ret: %d", ret);

        ret = snd_pcm_read(r_snd, r_data_src + total_read_size, read_frames);
        if (ret != (int)read_frames)
            aud_error("ret: %d", ret);

        total_read_size += in_bytes_per_frame * read_frames;
    }

    if (total_read_size < (unsigned int)r_data_size) {
        ret = snd_pcm_write(w_snd, w_data_src, w_params.period_size);
        if (ret != (int)w_params.period_size)
            aud_error("ret: %d", ret);

        read_frames = (r_data_size - total_read_size) / in_bytes_per_frame;
        ret = snd_pcm_read(r_snd, r_data_src + total_read_size, read_frames);
        if (ret != (int)read_frames)
            aud_error("ret: %d", ret);
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        goto exit2;

    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        goto exit2;

    ret = snd_pcm_drop(r_snd);
    if (ret)
        goto exit2;

    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        goto exit2;

    frequency[0] = w_params.rate / 48;
    frequency[1] = w_params.rate / 48;
    frequency_check(r_data_src, r_data_size, r_params.format == MSD_PCM_FMT_S16_LE ? 16 : 32, r_params.channels, frequency, r_params.rate);

exit2:
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit1:
    connect_route("UL10", "INTADC_in", 0, CONNECT_FE_BE);
    connect_route("I_60", "O_12", 0, CONNECT_IO_PORT);
    connect_route("I_61", "O_13", 0, CONNECT_IO_PORT);

    connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
    free(r_data_src);

exit0:
    free(w_data_src);

    return ret;
}

#endif /* #ifdef AUD_CODEC_SUPPORT */

static int ul10_from_dlm(void)
{
    sound_t *w_snd;
    sound_t *r_snd;
    int ret = 0;
    int index;
    struct msd_hw_params w_params;
    w_params.format = MSD_PCM_FMT_S32_LE;
    w_params.channels = 2;
    w_params.period_count = 4;
    w_params.period_size = 640;
    w_params.rate = 8000;

    int bytes_per_frame = 32 * w_params.channels / 8;

    int w_data_size = bytes_per_frame * w_params.period_size;

    void *w_data_src = malloc(w_data_size);
    if (!w_data_src) {
        printf("%s: memory error\r\n", __FUNCTION__);
        return -1;
    }
    memset(w_data_src, 0, w_data_size);
    aud_msg("w_data_src = %p, w_data_size = 0x%x", w_data_src, w_data_size);

    const void *golden_src = afe_sgen_golden_table_32bits;
    int golden_size = table_size;
    aud_msg("golden_src = %p, golden_size = %d", golden_src, golden_size);

    for (index = 0; index < w_data_size / golden_size; index++) {
        memcpy(w_data_src + index * golden_size, golden_src, golden_size);
    }

    struct msd_hw_params r_params;
    r_params.format = MSD_PCM_FMT_S32_LE;
    r_params.channels = 2;
    r_params.period_count = 4;
    r_params.period_size = 640;
    r_params.rate = 8000;

    int r_data_size = bytes_per_frame * r_params.period_size;

    void *r_data_src = malloc(r_data_size);
    if (!r_data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        goto exit0;
    }

    printf("r_data_src = %p, r_data_size = 0x%x\r\n", r_data_src, r_data_size);
    memset(r_data_src, 0, r_data_size);

    connect_route("UL10", "INTADC_in", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_12", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_13", 1, CONNECT_IO_PORT);

    connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);

    // timing: playback first, read second
    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit1;
    ret = snd_pcm_hw_params(w_snd, &w_params);
    if (ret)
        goto exit2;

    ret = snd_pcm_open(&r_snd, "UL10", 0, 0);
    if (ret)
        goto exit2;

    ret = snd_pcm_hw_params(r_snd, &r_params);
    if (ret)
        goto exit3;

    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit3;

    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit3;

    for (index = 0; index < (int)r_params.period_size / (int)r_params.period_size * 1000; index ++) {
        ret = snd_pcm_write(w_snd, w_data_src, w_params.period_size);
        if (ret != (int)w_params.period_size)
            aud_error("ret: %d", ret);

        ret = snd_pcm_read(r_snd, r_data_src, r_params.period_size);
        if (ret != (int)r_params.period_size)
            aud_msg("ret: %d", ret);
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);
    ret = snd_pcm_drop(r_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

exit3:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit2:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit1:
    free(r_data_src);

    connect_route("UL10", "INTADC_in", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_12", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_13", 0, CONNECT_IO_PORT);

    connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);

exit0:
    free(w_data_src);
    return ret;
}

#ifdef AUD_CODEC_SUPPORT
static int dac_from_adc_loopback(int channel_num, int bitdepth, int sample_rate,
                                 int period_size, int period_count, int time_len)
{
    sound_t *w_snd;
    sound_t *r_snd;
    int ret = 0;
    struct msd_hw_params params;

    if (bitdepth != 16 && bitdepth != 32) {
        aud_error("bitdepth error: %d", bitdepth);
        return 0;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    int bytes_per_frame = bitdepth * params.channels / 8;
    int total_frames = time_len * params.rate;
    int data_size = bytes_per_frame * params.period_size * params.period_count;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return -1;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("UL3", "INTADC_in", 1, CONNECT_FE_BE);
#if defined PINMUX_QFN_DEFAULT
    connect_route("I_60", "O_14", 1, CONNECT_IO_PORT);
    connect_route("I_61", "O_15", 1, CONNECT_IO_PORT);
#elif defined PINMUX_BGA_DEFAULT
    connect_route("I_60", "O_14", 1, CONNECT_IO_PORT);
    if (hal_boot_get_hw_ver() != 0x8A00) {
        connect_route("I_61", "O_15", 1, CONNECT_IO_PORT);
    } else {
        connect_route("I_08", "O_15", 1, CONNECT_IO_PORT);
    }
#endif /* #if defined PINMUX_QFN_DEFAULT */

    connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;

    ret = snd_pcm_open(&r_snd, "UL3", 0, 0);
    if (ret)
        goto exit1;

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    ret = snd_pcm_hw_params(r_snd, &params);
    if (ret)
        goto exit2;

    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit2;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit2;

    while (total_frames > 0) {
        ret = snd_pcm_read(r_snd, data_src, params.period_size * params.period_count);
        if (ret != (int)params.period_size * (int)params.period_count)
            aud_error("ret: %d", ret);
        ret = snd_pcm_write(w_snd, data_src, params.period_size * params.period_count);
        if (ret != (int)params.period_size * (int)params.period_count)
            aud_error("ret: %d", ret);
        total_frames -= ret;
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

    ret = snd_pcm_drop(r_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

exit2:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);

    connect_route("UL3", "INTADC_in", 0, CONNECT_FE_BE);
#if defined PINMUX_QFN_DEFAULT
    connect_route("I_60", "O_14", 0, CONNECT_IO_PORT);
    connect_route("I_61", "O_15", 0, CONNECT_IO_PORT);
#elif defined PINMUX_BGA_DEFAULT
    connect_route("I_60", "O_14", 0, CONNECT_IO_PORT);
    if (hal_boot_get_hw_ver() != 0x8A00) {
        connect_route("I_61", "O_15", 0, CONNECT_IO_PORT);
    } else {
        connect_route("I_08", "O_15", 0, CONNECT_IO_PORT);
    }
#endif /* #if defined PINMUX_QFN_DEFAULT */

    connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);

    return 0;
}

static int i2s_from_adc_loopback(int channel_num, int bitdepth, int sample_rate,
                                 int period_size, int period_count, int time_len)
{
    sound_t *w_snd;
    sound_t *r_snd;
    int ret = 0;
    struct msd_hw_params params;

    if (bitdepth != 16 && bitdepth != 32) {
        aud_error("bitdepth error: %d", bitdepth);
        return 0;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    int bytes_per_frame = bitdepth * params.channels / 8;
    int total_frames = time_len * params.rate;
    int data_size = bytes_per_frame * params.period_size * params.period_count;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return -1;
    }
    printf("data_src = %p, data_size = 0x%x", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("UL9", "INTADC_in", 1, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 1, CONNECT_IO_PORT);
    if (hal_boot_get_hw_ver() != 0x8A00) {
        connect_route("I_61", "O_27", 1, CONNECT_IO_PORT);
    } else {
        connect_route("I_08", "O_27", 1, CONNECT_IO_PORT);
    }

    connect_route("track0", "ETDM2_OUT_BE", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_04", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_05", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_open(&r_snd, "UL9", 0, 0);
    if (ret)
        goto exit1;

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    ret = snd_pcm_hw_params(r_snd, &params);
    if (ret)
        goto exit2;

    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit2;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit2;

    while (total_frames > 0) {
        ret = snd_pcm_read(r_snd, data_src, params.period_size * params.period_count);
        if (ret != (int)params.period_size * (int)params.period_count)
            aud_msg("ret: %d", ret);
        ret = snd_pcm_write(w_snd, data_src, params.period_size * params.period_count);
        if (ret != (int)params.period_size * (int)params.period_count)
            aud_error("ret: %d", ret);
        total_frames = total_frames - ret;
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

    ret = snd_pcm_drop(r_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

exit2:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);
    connect_route("UL9", "INTADC_in", 0, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 0, CONNECT_IO_PORT);
    if (hal_boot_get_hw_ver() != 0x8A00) {
        connect_route("I_61", "O_27", 0, CONNECT_IO_PORT);
    } else {
        connect_route("I_08", "O_27", 0, CONNECT_IO_PORT);
    }

    connect_route("track0", "ETDM2_OUT_BE", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_04", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_05", 0, CONNECT_IO_PORT);

    return 0;
}

static int dac_from_dmic_loopback(int channel_num, int bitdepth, int sample_rate,
                                  int period_size, int period_count, int time_len)
{
    sound_t *w_snd;
    sound_t *r_snd;
    int ret = 0;
    struct msd_hw_params params;

    if (bitdepth != 16 && bitdepth != 32) {
        aud_error("bitdepth error: %d", bitdepth);
        return 0;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    int bytes_per_frame = bitdepth * params.channels / 8;
    int total_frames = time_len * params.rate;
    int data_size = bytes_per_frame * params.period_size * params.period_count;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return -1;
    }
    printf("data_src = %p, data_size = 0x%x", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("UL3", "DMIC_BE", 1, CONNECT_FE_BE);
    connect_route("I_04", "O_14", 1, CONNECT_IO_PORT);
    connect_route("I_05", "O_15", 1, CONNECT_IO_PORT);

    connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_open(&r_snd, "UL3", 0, 0);
    if (ret)
        goto exit1;

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    ret = snd_pcm_hw_params(r_snd, &params);
    if (ret)
        goto exit2;

    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit2;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit2;

    while (total_frames > 0) {
        ret = snd_pcm_read(r_snd, data_src, params.period_size * params.period_count);
        if (ret != (int)params.period_size * (int)params.period_count)
            aud_error("ret: %d", ret);
        ret = snd_pcm_write(w_snd, data_src, params.period_size * params.period_count);
        if (ret != (int)params.period_size * (int)params.period_count)
            aud_error("ret: %d", ret);
        total_frames -= ret;
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

    ret = snd_pcm_drop(r_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

exit2:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);

    connect_route("UL3", "DMIC_BE", 0, CONNECT_FE_BE);
    connect_route("I_04", "O_14", 0, CONNECT_IO_PORT);
    connect_route("I_05", "O_15", 0, CONNECT_IO_PORT);

    connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);

    return 0;
}
#endif /* #ifdef AUD_CODEC_SUPPORT */

static int i2sin_from_i2sout_loopback(int channel_num, int bitdepth, int sample_rate,
                                      int period_size, int period_count, int time_len)
{
    int index;
    sound_t *w_snd;
    sound_t *r_snd;
    int ret = 0;
    struct msd_hw_params params;
    const void *golden_src;
    int golden_size;

    switch (bitdepth) {
        case 32:
            golden_src = afe_sgen_golden_table_32bits;
            golden_size = table_size;
            break;

        case 16:
            golden_src = afe_sgen_golden_table_16bits;
            golden_size = table_size_16bit;
            break;

        default:
            aud_error("bitdepth error: %d", bitdepth);
            return 0;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;

    int bytes_per_frame = bitdepth * params.channels / 8;
    int total_frames = time_len * params.rate;
    int data_size = bytes_per_frame * params.period_size * params.period_count;
    data_size = data_size / golden_size * golden_size;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return -1;
    }
    printf("data_src = %p, data_size = 0x%x", data_src, data_size);
    memset(data_src, 0, data_size);

    for (index = 0; index < data_size / golden_size; index++) {
        memcpy(data_src + index * golden_size, golden_src, golden_size);
    }

    connect_route("UL9", "ETDM2_IN_BE", 1, CONNECT_FE_BE);
    connect_route("I_12", "O_26", 1, CONNECT_IO_PORT);
    connect_route("I_13", "O_27", 1, CONNECT_IO_PORT);

    connect_route("track0", "ETDM2_OUT_BE", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_04", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_05", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_open(&r_snd, "UL9", 0, 0);
    if (ret)
        goto exit1;

    struct msd_hw_params r_params;

    r_params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    r_params.channels = channel_num;
    r_params.period_count = period_count;
    r_params.period_size = period_size;
    r_params.rate = sample_rate;

    ret = snd_pcm_hw_params(r_snd, &r_params);
    if (ret)
        goto exit2;

    int in_data_frames = period_count * period_size;
    int in_data_size = bytes_per_frame * in_data_frames;

    void *in_data_src = malloc(in_data_size);
    if (!in_data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        goto exit2;
    }
    printf("in_data_src = %p, in_data_size = 0x%x\n", in_data_src, in_data_size);
    memset(in_data_src, 0, in_data_size);


    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit3;
    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit3;

    ret = snd_pcm_write(w_snd, data_src, params.period_size * params.period_count);
    if (ret != (int)params.period_size * (int)params.period_count)
        aud_error("ret: %d", ret);

    ret = snd_pcm_write(w_snd, data_src, params.period_size * params.period_count);
    if (ret != (int)params.period_size * (int)params.period_count)
        aud_error("ret: %d", ret);
    while (total_frames > 0) {
        ret = snd_pcm_write(w_snd, data_src, params.period_size * params.period_count);
        if (ret != (int)params.period_size * (int)params.period_count)
            aud_error("ret: %d", ret);
        ret = snd_pcm_read(r_snd, in_data_src, r_params.period_size * r_params.period_count);
        if (ret != (int)r_params.period_size * (int)r_params.period_count)
            aud_msg("ret: %d", ret);
        total_frames = total_frames - ret;
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

    ret = snd_pcm_drop(r_snd);
    if (ret)
        aud_error("snd_pcm_drop error: %d", ret);

exit3:
    free(in_data_src);

exit2:
    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(r_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit1:
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        aud_error("snd_pcm_hw_free error: %d", ret);
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);
    connect_route("UL9", "ETDM2_IN_BE", 0, CONNECT_FE_BE);
    connect_route("I_12", "O_26", 0, CONNECT_IO_PORT);
    connect_route("I_13", "O_27", 0, CONNECT_IO_PORT);

    connect_route("track0", "ETDM2_OUT_BE", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_04", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_05", 0, CONNECT_IO_PORT);

    return 0;
}

#ifdef FILE_SYS_SUPPORT
#define FILE_NAME_MAX 256
#endif /* #ifdef FILE_SYS_SUPPORT */

static int dsp_test_r(void)
{
    sound_t *r_snd;
    int ret = 0;
    //  int index;
#ifdef FILE_SYS_SUPPORT
    FIL fd;
    FRESULT f_ret;
    UINT f_bw;
    char file_name[FILE_NAME_MAX];
#endif /* #ifdef FILE_SYS_SUPPORT */
    struct msd_hw_params params;
    unsigned int bitwidth = 16;
    params.format = MSD_PCM_FMT_S16_LE;
    params.channels = 4;
    params.period_count = 4;
    params.rate = 16000;
    params.period_size = params.rate / 100; //10ms;

#ifdef FILE_SYS_SUPPORT
    snprintf(file_name, sizeof(file_name), "SD:/data/AP_RECORD_%dHz_%dch_%dbits.pcm",
             params.rate, params.channels, bitwidth);

    f_ret = f_open(&fd, file_name, FA_CREATE_ALWAYS | FA_READ | FA_WRITE);
    if (f_ret) {
        printf("Failed to open %s, ret: %d\n", file_name, f_ret);
        return 0;
    }
#endif /* #ifdef FILE_SYS_SUPPORT */
    int bytes_per_frame = bitwidth * params.channels / 8;
    int data_frames = params.period_count * params.period_size;
    int data_size = bytes_per_frame * data_frames;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        goto exit;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("ADSP_HOSTLESS_VA", "ADSP_UL9_IN_BE", 1, CONNECT_FE_BE);
    connect_route("ADSP_VA_FE", "ADSP_UL9_IN_BE", 1, CONNECT_FE_BE);
    connect_route("ADSP_UL9_IN_BE", "INTADC_in", 1, CONNECT_FE_BE);
    connect_route("ADSP_UL9_IN_BE", "GASRC0_C", 1, CONNECT_FE_BE);
    connect_route("GASRC0_C", "dummy_end_c", 1, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 1, CONNECT_IO_PORT);
    if (hal_boot_get_hw_ver() != 0x8A00) {
        connect_route("I_61", "O_27", 1, CONNECT_IO_PORT);
    } else {
        connect_route("I_08", "O_27", 1, CONNECT_IO_PORT);
    }
    connect_route("I_22", "O_28", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_29", 1, CONNECT_IO_PORT);


    snd_pcm_open(&r_snd, "ADSP_AP_FE", MSD_STREAM_CAPTURE, 0);

    snd_pcm_hw_params(r_snd, &params);

    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit;

    int total_frames = 20 * params.rate;

    while (total_frames > 0) {
        ret = snd_pcm_read(r_snd, data_src, data_size / bytes_per_frame);
        if (ret != data_size / bytes_per_frame)
            aud_msg("ret: %d", ret);

        total_frames = total_frames - ret;
#ifdef FILE_SYS_SUPPORT
        aud_msg("dump data to file");
        f_write(&fd, data_src, data_size, &f_bw);
        if (data_size != (int)f_bw)
            aud_msg("f_write error %u", f_bw);
#endif /* #ifdef FILE_SYS_SUPPORT */
    }

    ret = snd_pcm_drop(r_snd);
    if (ret)
        goto exit;

    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        goto exit;
    ret = snd_pcm_close(r_snd);
    if (ret)
        goto exit;

exit:
#ifdef FILE_SYS_SUPPORT
    f_close(&fd);
#endif /* #ifdef FILE_SYS_SUPPORT */
    free(data_src);
    return ret;
}

static int dsp_test_rw(void)
{
    sound_t *w_snd;
    sound_t *r_snd;
    int ret = 0;
    int index;
#ifdef FILE_SYS_SUPPORT
    FIL fd;
    FRESULT f_ret;
    char file_name[FILE_NAME_MAX];
#endif /* #ifdef FILE_SYS_SUPPORT */

    struct msd_hw_params params;
    unsigned int bitwidth = 16;
    params.format = MSD_PCM_FMT_S16_LE;
    params.channels = 4;
    params.period_count = 4;//un-usefull
    params.rate = 16000;
    params.period_size = params.rate / 100; //10ms

    struct msd_hw_params w_params;
    //  unsigned int w_bitwidth = 16;
    w_params.format = MSD_PCM_FMT_S16_LE;
    w_params.channels = 4;
    w_params.period_count = 4;
    w_params.rate = 16000;
    w_params.period_size = params.rate / 100; //10ms

#ifdef FILE_SYS_SUPPORT
    snprintf(file_name, sizeof(file_name), "/data/AP_RECORD_%dHz_%dch_%dbits.pcm",
             params.rate, params.channels, bitwidth);

    f_ret = f_open(&fd, file_name, FA_CREATE_ALWAYS | FA_READ | FA_WRITE);
    if (f_ret) {
        printf("Failed to open %s, ret: %d\n", file_name, f_ret);
        return 0;
    }
#endif /* #ifdef FILE_SYS_SUPPORT */
    int bytes_per_frame = bitwidth * params.channels / 8;
    int data_frames = 10 * params.rate;
    int data_size = bytes_per_frame * data_frames;

    //  int w_bytes_per_frame = w_bitwidth * w_params.channels / 8;

    void *data_src = pvPortMalloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        goto exit;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    connect_route("ADSP_AP_RECORD", "BE_UL9", 1, CONNECT_FE_BE);
    connect_route("BE_UL9", "INTADC_in", 1, CONNECT_FE_BE);
    //connect_route("BE_UL9", "GASRC0_C", 1, CONNECT_FE_BE);
    //connect_route("GASRC0_C", "dummy_end_c", 1, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 1, CONNECT_IO_PORT);
    connect_route("I_61", "O_27", 1, CONNECT_IO_PORT);
    //connect_route("I_32", "O_28", 1, CONNECT_IO_PORT);
    //connect_route("I_33", "O_29", 1, CONNECT_IO_PORT);

    connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
    //connect_route("track0", "GASRC0_P", 1, CONNECT_FE_BE);
    //connect_route("GASRC0_P", "dummy_end_p", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);
    //connect_route("I_22", "O_42", 1, CONNECT_IO_PORT);
    //connect_route("I_23", "O_43", 1, CONNECT_IO_PORT);
    connect_route("I_22", "O_28", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_29", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&r_snd, "ADSP_AP_RECORD", 0, 0);
    if (ret)
        goto exit;

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret)
        goto exit;

    snd_pcm_hw_params(r_snd, &params);
    snd_pcm_hw_params(w_snd, &params);

    ret = snd_pcm_prepare(r_snd);
    if (ret)
        goto exit;

    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit;

    for (index = 0; index < data_frames / (int)params.period_size; index++) {
        ret = snd_pcm_read(r_snd, data_src + index * params.period_size * bytes_per_frame,
                           params.period_size);
        if (ret != (int)params.period_size)
            aud_msg("ret: %d", ret);
        ret = snd_pcm_write(w_snd, data_src + index * params.period_size * bytes_per_frame,
                            w_params.period_size);
        if (ret != (int)w_params.period_size)
            aud_error("ret: %d", ret);
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        goto exit;

    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        goto exit;

    ret = snd_pcm_close(w_snd);
    if (ret)
        goto exit;

    ret = snd_pcm_drop(r_snd);
    if (ret)
        goto exit;

    ret = snd_pcm_hw_free(r_snd);
    if (ret)
        goto exit;

    ret = snd_pcm_close(r_snd);
    if (ret)
        goto exit;

#ifdef FILE_SYS_SUPPORT
    printf("start dump file...\n");
    UINT f_bw;
    f_ret = f_write(&fd, data_src, data_size, &f_bw);
    if (f_ret) {
        printf("Failed to write to file %s, ret: %d\n", file_name, f_ret);
    }
    printf("end dump file...\n");
#endif /* #ifdef FILE_SYS_SUPPORT */
exit:
#ifdef FILE_SYS_SUPPORT
    f_close(&fd);
#endif /* #ifdef FILE_SYS_SUPPORT */
    vPortFree(data_src);
    return ret;
}

struct aud_cmd_list {
    const char *pname;
    void (*pfun)(int argc, const char *argv[]);
};

static uint8_t set_loglevel_cmd(uint8_t len, char *params[])
{
    if (len == 1 && (atoi(params[0]) >= 1 || atoi(params[0]) <= 5)) {
        int log_level = atoi(params[0]);
        set_loglevel(log_level);
        printf("New loglevel %d\r\n", get_loglevel());
        return 0;
    } else {
        printf("Wrong params!! %d, %d\r\n", len, atoi(params[0]));
        printf("Usage: loglevel [level]\r\n");
        printf("	[level] 1: error  2: warn 3: msg 4: debug\r\n");
        return 1;
    }
}

static void dump_process(void *void_this)
{
    while (1) {
#ifdef AUD_CODEC_SUPPORT
        dlm_intdac(2, 16, 8000, 960, 4, 4);
#else /* #ifdef AUD_CODEC_SUPPORT */
        aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
        vTaskDelay(10000 / portTICK_RATE_MS);
    }
}

static void audio_aplay_task(void *void_this)
{
    struct audio_task_params *audio_param = (struct audio_task_params *)void_this;

    sound_t *w_snd;
    int ret, index;
    void *golden_src;
    int golden_size;
    struct msd_hw_params params;

    switch (audio_param->bitdepth) {
        case 32:
            golden_src = afe_sgen_golden_table_32bits;
            golden_size = table_size;
            break;

        case 16:
            golden_src = afe_sgen_golden_table_16bits;
            golden_size = table_size_16bit;
            break;

        default:
            aud_error("bitdepth error: %d", audio_param->bitdepth);
            goto exit_task_delete;
    }

    params.format = audio_param->bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = audio_param->channel_num;
    params.period_count = audio_param->period_count;
    params.period_size = audio_param->period_size;
    params.rate = audio_param->sample_rate;

    int bytes_per_frame = audio_param->bitdepth * params.channels / 8;
    int data_size = bytes_per_frame * params.period_size * params.period_count / golden_size * golden_size;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        goto exit_task_delete;
    }

    memset(data_src, 0, data_size);

    for (index = 0; index < data_size / golden_size; index++) {
        memcpy(data_src + index * golden_size, golden_src, golden_size);
    }

    aud_msg("data_src = %p, data_size = 0x%x", data_src, data_size);

    ret = snd_pcm_open(&w_snd, audio_param->fe_name, 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;

    int total_frames = audio_param->time_len * params.rate;
    while (total_frames > 0) {
        ret = snd_pcm_write(w_snd, data_src, data_size / bytes_per_frame);
        if (ret != data_size / bytes_per_frame)
            aud_msg("ret: %d", ret);
        total_frames = total_frames - ret;
    }

    ret = snd_pcm_drain(w_snd);
    if (ret)
        goto exit1;
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(data_src);

exit_task_delete:
    free(audio_param);
    vTaskDelete(NULL);
}

static uint8_t audio_aplay_cmd(uint8_t len, char *params[])
{
    struct audio_task_params *audio_param = (struct audio_task_params *)malloc(sizeof(struct audio_task_params));

    if (!audio_param)
        return 0;

    memset(audio_param, 0, sizeof(struct audio_task_params));

    if (len > 6) {
        aud_msg("params[0] = %s", params[0]);
        strncpy(audio_param->fe_name, params[0], ALSA_DAI_NAME_LEN - 1);
        audio_param->channel_num = atoi(params[1]);
        audio_param->bitdepth = atoi(params[2]);
        audio_param->sample_rate = atoi(params[3]);
        audio_param->time_len = atoi(params[4]);
        audio_param->period_size = atoi(params[5]);
        audio_param->period_count = atoi(params[6]);
    } else {
        aud_msg("aplay [fe_name] [channel] [bitdepth] [rate] [time len] [period size] [period count]");
        free(audio_param);
        return 0;
    }

    if (len > 7) {
        audio_param->disable_task = atoi(params[7]);
    }

    aud_msg("fe_name = %s", audio_param->fe_name);
    aud_msg("channel_num = %d", audio_param->channel_num);
    aud_msg("bitdepth = %d", audio_param->bitdepth);
    aud_msg("sample_rate = %d", audio_param->sample_rate);
    aud_msg("time_len = %d", audio_param->time_len);
    aud_msg("period_size = %d", audio_param->period_size);
    aud_msg("period_count = %d", audio_param->period_count);
    aud_msg("disable_task = %d", audio_param->disable_task);

    if (audio_param->disable_task) {
        audio_aplay_task(audio_param);
    } else {
        xTaskCreate(audio_aplay_task,
                    audio_param->fe_name,
                    1024,
                    audio_param,
                    configMAX_PRIORITIES - 1,
                    NULL);
    }

    return 0;
}

static void audio_dsp_play_task(void *void_this)
{
    struct audio_task_params *audio_param = (struct audio_task_params *)void_this;

    sound_t *w_snd;
    int ret;
    struct msd_hw_params params;


    params.format = audio_param->bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = audio_param->channel_num;
    params.period_count = audio_param->period_count;
    params.period_size = audio_param->period_size;
    params.rate = audio_param->sample_rate;

    // hal audio: open
    ret = snd_pcm_open(&w_snd, audio_param->fe_name, 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;

    // hal audio: start
    ret = snd_pcm_start(w_snd);
    if (ret < 0)
        goto exit1;

    vTaskDelay(1000 * audio_param->time_len / portTICK_PERIOD_MS);

    // hal audio: stop
    ret = snd_pcm_drop(w_snd);
    if (ret)
        goto exit1;

    // hal audio: close
    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        goto exit1;

exit1:
    ret = snd_pcm_close(w_snd);
    if (ret)
        aud_error("snd_pcm_close error: %d", ret);

exit0:
    free(audio_param);
    vTaskDelete(NULL);
}

static uint8_t audio_dsp_play_cmd(uint8_t len, char *params[])
{
    struct audio_task_params *audio_param = (struct audio_task_params *)malloc(sizeof(struct audio_task_params));

    if (!audio_param)
        return 0;

    memset(audio_param, 0, sizeof(struct audio_task_params));

    if (len > 6) {
        aud_msg("params[0] = %s", params[0]);
        strncpy(audio_param->fe_name, params[0], ALSA_DAI_NAME_LEN - 1);
        audio_param->channel_num = atoi(params[1]);
        audio_param->bitdepth = atoi(params[2]);
        audio_param->sample_rate = atoi(params[3]);
        audio_param->time_len = atoi(params[4]);
        audio_param->period_size = atoi(params[5]);
        audio_param->period_count = atoi(params[6]);
    } else {
        aud_msg("aplay [fe_name] [channel] [bitdepth] [rate] [time len] [period size] [period count]");
        free(audio_param);
        return 0;
    }

    if (len > 7) {
        audio_param->disable_task = atoi(params[7]);
    }

    aud_msg("fe_name = %s", audio_param->fe_name);
    aud_msg("channel_num = %d", audio_param->channel_num);
    aud_msg("bitdepth = %d", audio_param->bitdepth);
    aud_msg("sample_rate = %d", audio_param->sample_rate);
    aud_msg("time_len = %d", audio_param->time_len);
    aud_msg("period_size = %d", audio_param->period_size);
    aud_msg("period_count = %d", audio_param->period_count);
    aud_msg("disable_task = %d", audio_param->disable_task);

    if (audio_param->disable_task) {
        audio_dsp_play_task(audio_param);
    } else {
        xTaskCreate(audio_dsp_play_task,
                    audio_param->fe_name,
                    1024,
                    audio_param,
                    configMAX_PRIORITIES - 1,
                    NULL);
    }

    return 0;
}

static uint8_t audio_connect_cmd(uint8_t len, char *params[])
{
    char src[CONNETC_SRC_SINK_NAME_LEN] = { 0 };
    char sink[CONNETC_SRC_SINK_NAME_LEN] = { 0 };
    int value;
    int type;

    if (len > 3) {
        aud_msg("params[0] = %s", params[0]);
        strncpy(src, params[0], CONNETC_SRC_SINK_NAME_LEN - 1);
        strncpy(sink, params[1], CONNETC_SRC_SINK_NAME_LEN - 1);
        value = atoi(params[2]);
        type = atoi(params[3]);
    } else {
        aud_msg("connect [src] [sink] [value] [type]");
        return 0;
    }

    aud_msg("src = %s", src);
    aud_msg("sink = %s", sink);
    aud_msg("value = %d", value);
    aud_msg("type = %d", type);

    connect_route(src, sink, value, type);

    return 0;
}

static uint8_t audio_play_cmd(uint8_t len, char *params[])
{
    int play_type;
    int channel_num;
    int bitdepth;
    int sample_rate;
    int period_size;
    int period_count;
    int time_len;
    TaskHandle_t xHandler = NULL;
    if (len > 6) {
        aud_msg("params[0] = %s", params[0]);
        play_type = atoi(params[0]);
        channel_num = atoi(params[1]);
        bitdepth = atoi(params[2]);
        sample_rate = atoi(params[3]);
        time_len = atoi(params[4]);
        period_size = atoi(params[5]);
        period_count = atoi(params[6]);
    } else {
        aud_msg("play [play_type] [channel] [bitdepth] [rate] [time len] [period size] [period count]");
        aud_msg("	0: dlm_gsrc_intdac");
        aud_msg("	1: dlm_intdac");
        aud_msg("	2: dlm_etdmout2");
        return 0;
    }
    aud_msg("play_type = %d", play_type);
    aud_msg("channel_num = %d", channel_num);
    aud_msg("bitdepth = %d", bitdepth);
    aud_msg("sample_rate = %d", sample_rate);
    aud_msg("time_len = %d", time_len);
    aud_msg("period_size = %d", period_size);
    aud_msg("period_count = %d", period_count);
    switch (play_type) {
        case 0:
#ifdef AUD_CODEC_SUPPORT
            dlm_gsrc_intdac(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;
        case 1:
#ifdef AUD_CODEC_SUPPORT
            dlm_intdac(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;
        case 2:
            dlm_etdmout2(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
            break;
        case 3:
            xTaskCreate(dump_process, "AUDIOTEST", 1024, NULL, 7, &xHandler);
            break;
        default:
            aud_error("play_type error");
            break;
    }
    return 0;
}

#ifdef FILE_SYS_SUPPORT
static uint8_t audio_play_file_cmd(uint8_t len, char *params[])
{
    int play_type;
    int channel_num;
    int bitdepth;
    int sample_rate;
    int period_size;
    int period_count;
    int time_len;
    char file_name[256] = {0};
    if (len > 7) {
        aud_msg("params[0] = %s", params[0]);
        play_type = atoi(params[0]);
        channel_num = atoi(params[1]);
        bitdepth = atoi(params[2]);
        sample_rate = atoi(params[3]);
        time_len = atoi(params[4]);
        period_size = atoi(params[5]);
        period_count = atoi(params[6]);
        snprintf(file_name, sizeof(file_name), "SD:/%s", params[7]);
    } else {
        aud_msg("play_file [play_type] [channel] [bitdepth] [rate] [time len] [period size] [period count] [file_name]");
        aud_msg("	0: dlm_gsrc_intdac");
        aud_msg("	1: dlm_intdac");
        aud_msg("	2: dlm_etdmout2");
        return 0;
    }
    aud_msg("play_type = %d", play_type);
    aud_msg("channel_num = %d", channel_num);
    aud_msg("bitdepth = %d", bitdepth);
    aud_msg("sample_rate = %d", sample_rate);
    aud_msg("time_len = %d", time_len);
    aud_msg("period_size = %d", period_size);
    aud_msg("period_count = %d", period_count);
    aud_msg("file_name = %s", file_name);
    switch (play_type) {
        case 0:
#ifdef AUD_CODEC_SUPPORT
            //      dlm_gsrc_intdac(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;
        case 1:
#ifdef AUD_CODEC_SUPPORT
            dlm_intdac_file(channel_num, bitdepth, sample_rate, period_size,
                            period_count, time_len, file_name);
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;
        case 2:
            //      dlm_etdmout2(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
            break;
        default:
            aud_error("play_type error");
    }
    return 0;
}
#endif /* #ifdef FILE_SYS_SUPPORT */

static uint8_t audio_loopback_cmd(uint8_t len, char *params[])
{
    int loopback_type;
    int channel_num;
    int bitdepth;
    int sample_rate;
    int period_size;
    int period_count;
    int time_len;
    if (len > 6) {
        aud_msg("params[0] = %s", params[0]);
        loopback_type = atoi(params[0]);
        channel_num = atoi(params[1]);
        bitdepth = atoi(params[2]);
        sample_rate = atoi(params[3]);
        time_len = atoi(params[4]);
        period_size = atoi(params[5]);
        period_count = atoi(params[6]);
    } else {
        aud_msg("loopback [loopback_type] [channel] [bitdepth] [rate] [time len] [period size] [period count]");
        aud_msg("	0: dac_from_adc_loopback");
        aud_msg("	1: i2s_from_adc_loopback");
        aud_msg("	2: dac_from_dmic_loopback");
        aud_msg("	3: i2sin_from_i2sout_loopback");
        return 0;
    }
    aud_msg("play_type = %d", loopback_type);
    aud_msg("channel_num = %d", channel_num);
    aud_msg("bitdepth = %d", bitdepth);
    aud_msg("sample_rate = %d", sample_rate);
    aud_msg("time_len = %d", time_len);
    aud_msg("period_size = %d", period_size);
    aud_msg("period_count = %d", period_count);
    switch (loopback_type) {
        case 0:
#ifdef AUD_CODEC_SUPPORT
            dac_from_adc_loopback(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;

        case 1:
#ifdef AUD_CODEC_SUPPORT
            i2s_from_adc_loopback(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;

        case 2:
#ifdef AUD_CODEC_SUPPORT
            dac_from_dmic_loopback(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;

        case 3:
            i2sin_from_i2sout_loopback(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
            break;

        default:
            aud_error("play_type error");

    }
    return 0;
}

static uint8_t amixer_cget_cmd(uint8_t len, char *params[])
{
    if (len >= 2) {
        control_cget(params[0], atoi(params[1]));
        return 0;
    } else {
        aud_msg("wrong num of params");
        aud_msg("cget [name] [param_num]");
        return 1;
    }
}

static uint8_t amixer_cset_cmd(uint8_t len, char *params[])
{
    if (len >= 3) {
        int param_num = atoi(params[1]);
        if (param_num <= len - 2) {
            int index;
            int *value = (int *)malloc(param_num * sizeof(int));
            if (!value) {
                aud_error("malloc error");
                return 1;
            }

            for (index = 0; index < param_num; index++) {
                value[index] = atoi(params[2 + index]);
                aud_msg("value[%d] = %d", value[index], index);
            }
            aud_msg("param_num = %d", param_num);
            control_cset(params[0], param_num, value);
            control_cget(params[0], param_num);
            free(value);
            return 0;
        } else {
            aud_error("wrong num of cset params, %d %d\n", param_num, len - 2);
            return 1;
        };
    } else {
        aud_error("wrong num of params");
        aud_error("cset [name] [param_num] [value0 value1 ...]");
        return 1;
    }
}

static uint8_t dsp_read_cmd(uint8_t len, char *params[])
{
    dsp_test_r();
    return 0;
}

static uint8_t audio_register_read_cmd(uint8_t len, char *params[])
{
    if (len < 3) {
        aud_error("reg_read [address] [bytes]");
        return 0;
    }
    char *p = NULL;
    unsigned long address = strtoul(params[0], &p, 0);
    if (address == 0 && p == params[0]) {
        printf("address error\n");
        return 0;
    }

    unsigned long bytes = strtoul(params[1], &p, 0);
    if (bytes == 0 && p == params[1]) {
        printf("bytes error\n");
        return 0;
    }

    unsigned long hide_addr = strtoul(params[2], &p, 0);
    if (hide_addr == 0 && p == params[2]) {
        printf("hide_addr error\n");
        return 0;
    }

    int i = 0;

    for (i = 0; i < (int)bytes / 4; i++) {
        if (hide_addr == 1)
            printf("0x%08x\r\n", (unsigned int)aud_drv_get_reg(AUDIO_REGMAP_NO_BASE_TYPE, (uintptr_t)(address + i * 4)));
        else
            printf("[0x%x] = 0x%08x\r\n", (unsigned int)(address + i * 4), (unsigned int)aud_drv_get_reg(AUDIO_REGMAP_NO_BASE_TYPE, (uintptr_t)(address + i * 4)));
    }

    return 0;
}

static uint8_t audio_register_write_cmd(uint8_t len, char *params[])
{
    if (len < 3) {
        aud_error("reg_write [address] [val] [mask]");
        return 0;
    }

    char *p = NULL;
    unsigned long address = strtoul(params[0], &p, 0);
    if (address == 0 && p == params[0]) {
        printf("address error\n");
        return 0;
    }

    unsigned long val = strtoul(params[1], &p, 0);
    if (val == 0 && p == params[1]) {
        printf("val error\n");
        return 0;
    }

    unsigned long mask = strtoul(params[2], &p, 0);
    if (mask == 0 && p == params[2]) {
        printf("mask error\n");
        return 0;
    }

    aud_drv_set_reg_addr_mask_val(AUDIO_REGMAP_NO_BASE_TYPE, (uintptr_t)address, (uint32_t)mask, (uint32_t)val);
    printf("[0x%x] = 0x%x\r\n", (unsigned int)address, (unsigned int)aud_drv_get_reg(AUDIO_REGMAP_NO_BASE_TYPE, (uintptr_t)address));
    return 0;
}

static uint8_t audio_gpio_set_cmd(uint8_t len, char *params[])
{
    if (len < 4) {
        aud_error("gpio set [pin] [mode] [dir] [output]");
        return 0;
    }

    hal_gpio_pin_t pin;
    uint8_t mode;
    hal_gpio_direction_t dir;
    hal_gpio_data_t output;

    pin = atoi(params[0]);
    mode = atoi(params[1]);
    dir = atoi(params[2]);
    output = atoi(params[3]);

    printf("pin = %d, mode = %u, dir = %d, output = %d\n", pin, mode, dir, output);

    hal_pinmux_set_function(pin, mode);
    hal_gpio_set_direction(pin, dir);
    hal_gpio_set_output(pin, output);

    hal_gpio_get_function(pin, &mode);
    printf("pin = %d, mode = %u\n", pin, mode);
    return 0;
}

static uint8_t audio_gpio_get_cmd(uint8_t len, char *params[])
{
    if (len < 1) {
        aud_error("gpio get [pin]");
        return 0;
    }

    hal_gpio_pin_t pin;
    uint8_t mode;
    hal_gpio_direction_t dir;
    hal_gpio_data_t output;
    hal_gpio_data_t input;

    pin = atoi(params[0]);

    hal_gpio_get_function(pin, &mode);
    hal_gpio_get_direction(pin, &dir);
    hal_gpio_get_output(pin, &output);
    hal_gpio_get_input(pin, &input);
    printf("pin = %d, mode = %u, dir = %d, output = %d, input = %d\n",
           pin, mode, dir, output, input);

    return 0;
}

static uint8_t dsp_read_write_cmd(uint8_t len, char *params[])
{
    dsp_test_rw();
    return 0;
}

static void audio_arecord_task(void *void_this)
{
    struct audio_task_params *audio_param = (struct audio_task_params *)void_this;

    sound_t *w_snd;
    int ret;
    struct msd_hw_params params;
    unsigned int *dump_src;

    if (audio_param->bitdepth != 16 && audio_param->bitdepth != 32) {
        aud_error("bitdepth error: %d", audio_param->bitdepth);
        goto exit_task_delete;
    }

    params.format = audio_param->bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = audio_param->channel_num;
    params.period_count = audio_param->period_count;
    params.period_size = audio_param->period_size;
    params.rate = audio_param->sample_rate;

    int bytes_per_frame = audio_param->bitdepth * params.channels / 8;
    int data_frames = 1 * params.rate;
    int data_size = bytes_per_frame * data_frames;
    int total_frames = audio_param->time_len * params.rate;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        goto exit_task_delete;
    }
    printf("data_src = %p, data_size = 0x%x\n", data_src, data_size);
    memset(data_src, 0, data_size);

    ret = snd_pcm_open(&w_snd, audio_param->fe_name, 0, 0);
    if (ret)
        goto exit0;
    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret)
        goto exit1;
    ret = snd_pcm_prepare(w_snd);
    if (ret)
        goto exit1;

    while (total_frames > 0) {
        ret = snd_pcm_read(w_snd, data_src, data_frames);
        if (ret != data_frames)
            aud_msg("ret: %d", ret);
        total_frames -= ret;
    }

    ret = snd_pcm_drop(w_snd);
    if (ret)
        goto exit1;

    ret = snd_pcm_hw_free(w_snd);
    if (ret)
        goto exit1;

    dump_src = (unsigned int *)data_src;
    for (; audio_param->dump_count > 0; audio_param->dump_count--) {
        printf("dump data begin %d\r\n", audio_param->dump_count);
        int i = 0;

        for (i = 0; i < (int)data_size / 4; i++) {
            printf("[0x%x] = 0x%08x\r\n", (unsigned int)(dump_src + i),
                   (unsigned int)(*(dump_src + i)));
        }

    }
exit1:
    ret = snd_pcm_close(w_snd);
    if (ret)
        goto exit0;

exit0:
    free(data_src);

exit_task_delete:
    free(audio_param);
    vTaskDelete(NULL);
}

static uint8_t audio_arecord_cmd(uint8_t len, char *params[])
{
    struct audio_task_params *audio_param = (struct audio_task_params *)malloc(sizeof(struct audio_task_params));

    if (!audio_param)
        return 0;

    memset(audio_param, 0, sizeof(struct audio_task_params));

    if (len > 6) {
        aud_msg("params[0] = %s", params[0]);
        strncpy(audio_param->fe_name, params[0], 127);
        audio_param->channel_num = atoi(params[1]);
        audio_param->bitdepth = atoi(params[2]);
        audio_param->sample_rate = atoi(params[3]);
        audio_param->time_len = atoi(params[4]);
        audio_param->period_size = atoi(params[5]);
        audio_param->period_count = atoi(params[6]);
    } else {
        aud_msg("aplay [fe_name] [channel] [bitdepth] [rate] [time len] [period size] [period count]");
        free(audio_param);
        return 0;
    }

    if (len > 7) {
        audio_param->dump_count = atoi(params[7]);
    }

    if (len > 8) {
        audio_param->disable_task = atoi(params[8]);
    }

    aud_msg("fe_name = %s", audio_param->fe_name);
    aud_msg("channel_num = %d", audio_param->channel_num);
    aud_msg("bitdepth = %d", audio_param->bitdepth);
    aud_msg("sample_rate = %d", audio_param->sample_rate);
    aud_msg("time_len = %d", audio_param->time_len);
    aud_msg("period_size = %d", audio_param->period_size);
    aud_msg("period_count = %d", audio_param->period_count);
    aud_msg("dump_count = %d", audio_param->dump_count);
    aud_msg("disable_task = %d", audio_param->disable_task);

    if (audio_param->disable_task) {
        audio_arecord_task(audio_param);
    } else {
        xTaskCreate(audio_arecord_task,
                    audio_param->fe_name,
                    1024,
                    audio_param,
                    configMAX_PRIORITIES - 1,
                    NULL);
    }

    return 0;
}

static uint8_t audio_capture_cmd(uint8_t len, char *params[])
{
    int capture_type;
    int channel_num;
    int bitdepth;
    int sample_rate;
    int period_size;
    int period_count;
    int time_len;
    if (len > 6) {
        aud_msg("params[0] = %s", params[0]);
        capture_type = atoi(params[0]);
        channel_num = atoi(params[1]);
        bitdepth = atoi(params[2]);
        sample_rate = atoi(params[3]);
        time_len = atoi(params[4]);
        period_size = atoi(params[5]);
        period_count = atoi(params[6]);
    } else {
        aud_msg("capture [capture_type] [channel] [bitdepth] [rate] [time len] [period size] [period count]");

        aud_msg("	0: ul3_intadc");
        aud_msg("	1: ul10_intadc_from_intdac");
        aud_msg("	2: ul3_dmic");
        aud_msg("	3: ul9_intadc");
        aud_msg("	4: ul9_etdmin2");
        aud_msg("	5: ul9_intadc_and_dlm");
        aud_msg("	6: ul9_intadc_and_intdac");
        aud_msg("	7: ul10_frome_dlm");
        aud_msg("	8: ul9_etdmin1");
        aud_msg("	9: ul9_dmic");
        return 0;
    }

    aud_msg("capture_type = %d", capture_type);
    aud_msg("channel_num = %d", channel_num);
    aud_msg("bitdepth = %d", bitdepth);
    aud_msg("sample_rate = %d", sample_rate);
    aud_msg("time_len = %d", time_len);
    aud_msg("period_size = %d", period_size);
    aud_msg("period_count = %d", period_count);
    switch (capture_type) {
        case 0:
#ifdef AUD_CODEC_SUPPORT
            ul3_intadc(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;

        case 1:
#ifdef AUD_CODEC_SUPPORT
            ul10_intadc_from_intdac();
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;

        case 2:
            ul3_dmic();
            break;

        case 3:
#ifdef AUD_CODEC_SUPPORT
            ul9_intadc();
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;

        case 4:
            ul9_etdmin2(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
            break;

        case 5:
#ifdef AUD_CODEC_SUPPORT
            ul9_intadc_and_dlm();
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;

        case 6:
#ifdef AUD_CODEC_SUPPORT
            ul9_intadc_and_gasrc();
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;

        case 7:
            ul10_from_dlm();
            break;

        case 8:
            ul9_etdmin1(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
            break;

        case 9:
            ul9_dmic(channel_num, bitdepth, sample_rate, period_size, period_count, time_len);
            break;

        default:
            aud_error("capture_type error");
    }
    return 0;
}

static uint8_t audio_dsp_freq_check_cmd(uint8_t len, char *params[])
{
    int test_type;
    int in_ch;
    int out_ch;
    int in_rate;
    int out_rate;
    int bitdepth;
    int period_size;
    int period_count;

    if (len == 8) {
        aud_msg("params[0] = %s", params[0]);
        test_type = atoi(params[0]);
        in_ch = atoi(params[1]);
        out_ch = atoi(params[2]);
        in_rate = atoi(params[3]);
        out_rate = atoi(params[4]);
        period_size = atoi(params[5]);
        period_count = atoi(params[6]);
        bitdepth = atoi(params[7]);
    } else {
        aud_msg("freq_check [test_type] [in_ch] [out_ch] [in_rate] [out_rate] [period size] [period count] [bitdepth]");
        aud_msg("	0: ul10_intadc_from_intdac");
        return 0;
    }

    aud_msg("test_type = %d", test_type);
    aud_msg("input channel = %d", in_ch);
    aud_msg("output channel = %d", out_ch);
    aud_msg("input sample rate = %d", in_rate);
    aud_msg("output sample rate = %d", out_rate);
    aud_msg("period_size = %d", period_size);
    aud_msg("period_count = %d", period_count);
    aud_msg("bit depth = %d", bitdepth);

    switch (test_type) {
        case 0:
#ifdef AUD_CODEC_SUPPORT
            ul10_intadc_from_intdac_with_freq_check(in_ch, out_ch, in_rate, out_rate, period_size, period_count, bitdepth);
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;
        default:
            aud_error("test_type error");
    }
    return 0;
}


#ifdef FILE_SYS_SUPPORT
static uint8_t audio_capture_file_cmd(uint8_t len, char *params[])
{
    int capture_type;
    int channel_num;
    int bitdepth;
    int sample_rate;
    int period_size;
    int period_count;
    int time_len;
    char file_name[256] = { 0 };

    if (len > 7) {
        aud_msg("params[0] = %s", params[0]);
        capture_type = atoi(params[0]);
        channel_num = atoi(params[1]);
        bitdepth = atoi(params[2]);
        sample_rate = atoi(params[3]);
        time_len = atoi(params[4]);
        period_size = atoi(params[5]);
        period_count = atoi(params[6]);
        snprintf(file_name, sizeof(file_name), "SD:/%s", params[7]);
    } else {
        aud_msg("capture_file [capture_type] [channel] [bitdepth] [rate] [time len] [period size] [period count] [file_path]");

        aud_msg("	0: ul3_intadc");
        return 0;
    }

    aud_msg("capture_type = %d", capture_type);
    aud_msg("channel_num = %d", channel_num);
    aud_msg("bitdepth = %d", bitdepth);
    aud_msg("sample_rate = %d", sample_rate);
    aud_msg("time_len = %d", time_len);
    aud_msg("period_size = %d", period_size);
    aud_msg("period_count = %d", period_count);
    aud_msg("file_name = %s", file_name);

    switch (capture_type) {
        case 0:
#ifdef AUD_CODEC_SUPPORT
            ul3_intadc_file(channel_num, bitdepth, sample_rate,
                            period_size, period_count, time_len, file_name);
#else /* #ifdef AUD_CODEC_SUPPORT */
            aud_msg("Codec is not supported");
#endif /* #ifdef AUD_CODEC_SUPPORT */
            break;

        default:
            aud_error("capture_type error");
    }
    return 0;
}

UINT audio_ff_cmd1(void *fid)
{
    FIL *fd = (FIL *)fid;
    UINT f_bw;

    void *src_buf = malloc(8960);
    if (src_buf == NULL) {
        aud_error("malloc src_buf error");
        return 0;
    }
    memset(src_buf, 0xFF, 2560);

    int index = 0;
    while (index < 10000) {
        switch (index % 3) {
            case 0:
            case 1:
                f_write(fd, src_buf, 8960, &f_bw);
                break;
            case 2:
                f_write(fd, src_buf, 2560, &f_bw);
                break;
            default:
                break;
        }
        index ++;
        printf("index = %d\r\n", index);
    }

    aud_msg("f_bw = %u", f_bw);
    return f_bw;
}

static uint8_t audio_ff_cmd(uint8_t len, char *params[])
{
    FIL fid;
    FRESULT f_ret;
    UINT f_bw;
    char *file_name = "SD:/data/xuan.pcm";

    void *src_buf = malloc(44);
    if (src_buf == NULL) {
        aud_error("malloc src_buf error");
        return 0;
    }
    memset(src_buf, 0, 44);

    f_ret = f_open(&fid, file_name, FA_CREATE_ALWAYS | FA_WRITE);
    if (f_ret) {
        free(src_buf);
        aud_error("f_open %s error %d", file_name, f_ret);
        return 0;
    }

    audio_ff_cmd1(&fid);

    f_write(&fid, src_buf, 44, &f_bw);

    f_close(&fid);

    free(src_buf);
    return 0;
}
#endif /* #ifdef FILE_SYS_SUPPORT */

#ifdef VA_TEST_SUPPORT
static uint8_t va_main_cmd(uint8_t len, char *params[])
{
    va_main(len, params);
    return 0;
}

static uint8_t va_stop_cmd(uint8_t len, char *params[])
{
    va_stop(len, params);
    return 0;
}

static uint8_t va_dump_cmd(uint8_t len, char *params[])
{
    va_dump(len, params);
    return 0;
}

#endif /* #ifdef VA_TEST_SUPPORT */

static uint8_t audio_get_version_cmd(uint8_t len, char *params[])
{
    printf("Audio driver core version : %s\r\n", SND_PCM_CORE_VERSION);
    printf("Audio driver version : %s\r\n", SND_PCM_DRIVER_VERSION);
    return 0;
}

cmd_t audio_gpio_cmds[] = {
    { "set", "set gpio", audio_gpio_set_cmd, NULL },
    { "get", "get gpio", audio_gpio_get_cmd, NULL },
    { NULL, NULL, NULL, NULL } // end of table
};

cmd_t audio_drv_debug_cmds[] = {
    { "loglevel", "set log level", set_loglevel_cmd, NULL },
    { "play", "play music", audio_play_cmd, NULL },
    { "aplay", "aplay music", audio_aplay_cmd, NULL },
    { "arecord", "arecord music", audio_arecord_cmd, NULL },
    { "connect", "connect route", audio_connect_cmd, NULL },
    { "capture", "capture music", audio_capture_cmd, NULL },
    { "loopback", "play the recorded data directly", audio_loopback_cmd, NULL },
    { "cget",    "amixer cget", amixer_cget_cmd, NULL },
    { "cset",    "amixer cset", amixer_cset_cmd, NULL },
    { "dsp_r",    "dsp read", dsp_read_cmd, NULL },
    { "dsp_rw",    "dsp read and write", dsp_read_write_cmd, NULL },
    { "reg_read",    "read register", audio_register_read_cmd, NULL },
    { "reg_write",    "write register", audio_register_write_cmd, NULL },
    { "ver",    "get version info", audio_get_version_cmd, NULL },
    { "gpio",    "gpio control", NULL, audio_gpio_cmds },
    { "dsp_play", "dsp playback", audio_dsp_play_cmd, NULL },
    { "freq_check", "capture sine with frequency check", audio_dsp_freq_check_cmd, NULL },
#ifdef VA_TEST_SUPPORT
    { "va",    "va", va_main_cmd, NULL },
    { "va_stop",    "va_stop", va_stop_cmd, NULL },
    { "va_dump",    "va_dump", va_dump_cmd, NULL },
#endif /* #ifdef VA_TEST_SUPPORT */
#ifdef FILE_SYS_SUPPORT
    { "play_file", "play pcm file", audio_play_file_cmd, NULL },
    { "capture_file", "capture pcm file", audio_capture_file_cmd, NULL },
    { "ff", "ff file", audio_ff_cmd, NULL },
#endif /* #ifdef FILE_SYS_SUPPORT */
    { NULL, NULL, NULL, NULL } // end of table
};

void audio_init(void)
{
    /* init audio memory in psram */
    aud_memory_init((void *)0xA0268000, 0xF0000);

    extern int mt7933_dev_probe(void);
    mt7933_dev_probe();
    aud_msg("%s done. Fireware compile time:%s %s", __FUNCTION__, __DATE__, __TIME__);
}
