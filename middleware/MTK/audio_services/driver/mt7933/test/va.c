#include <stdlib.h>
#include <string.h>
#include "audio_test_utils.h"
#include "va.h"
#include "va_golden_tone.h"
#include "sound/include/asound.h"
#include "sound/include/tinypcm.h"
#include "sound/driver/include/pcm.h"
#include "audio_shared_info.h"
#include "hal_boot.h"

#ifdef FREERTOS_POSIX_SUPPORT
#include "FreeRTOS_POSIX/time.h"
#endif /* #ifdef FREERTOS_POSIX_SUPPORT */

//#define WAV_DUMP

#ifdef FILE_SYS_SUPPORT
#include "ff.h"
#endif /* #ifdef FILE_SYS_SUPPORT */

#ifdef FILE_SYS_SUPPORT
#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164
#define FORMAT_PCM 1
#endif /* #ifdef FILE_SYS_SUPPORT */

enum {
    EVT_UPLOAD,
    EVT_LC_TRIGGER,
    EVT_LC_COMMAND,
    EVT_LC_COMMAND_TO,
    EVT_STOP,
    EVT_NUM,
};

TaskHandle_t g_handler = NULL;
//static int force_read = 0;
static int voice_upload = 0;
static int vow_enable = 0;
static int cap_time = 0;
static int va_stoped = 0;
/*static*/ int va_started = 0;
sound_t *hostless_snd;
sound_t *upload_snd;
int g_event;

void *buffer_g = NULL;

#if (CFG_DSPOTTER_BASE_VERSION == 0)
const char *const local_cmd_trigger[] = {
    "Hey Siri",
    "Hey Siri ^1",
};
const char *const local_cmd_command[] = {
    "Open Camera",
    "Open Camera ^1",
    "Open Camera ^2",
    "Take a picture",
    "Take a picture ^1",
    "Play music",
    "Play music ^1",
    "Play music ^2",
    "Play music ^3",
    "Stop music",
    "Stop music ^1",
    "Stop music ^2",
    "Stop music ^3",
    "Previous Song",
    "Next Song",
    "Next Song ^1",
    "Volume Louder",
    "Volume Down",
};
#elif (CFG_DSPOTTER_BASE_VERSION == 1)
const char *const local_cmd_trigger[] = {
    "���ܹܼ�",
};
const char *const local_cmd_command[] = {
    "���ز���",
    "��������绰",
    "��������",
    "���ڼ���",
    "�ʶ���Ѷ",
    "��������",
    "�ر�����",
    "������ʶ",
};
#else /* #if (CFG_DSPOTTER_BASE_VERSION == 0) */
const char *const local_cmd_trigger[] = {
    "Hello Aircon",
    "Hello Aircon ^1",
    "Hello Aircon ^2",
    "Hello Aircon ^3",
};

const char *const local_cmd_command[] = {
    "Turn on the Aircon",
    "Turn on the Aircon ^1",
    "Turn on the Aircon ^2",
    "Turn on the Aircon ^3",
    "Aircon off",
    "Set temperature to twenty four degree",
    "Set temperature to twenty four degree ^1",
    "Set temperature to twenty four degree ^2",
    "Set temperature to twenty four degree ^3",
    "Set temperature to twenty five degree",
    "Set temperature to twenty five degree ^1",
    "Set temperature to twenty five degree ^2",
    "Set temperature to twenty five degree ^3",
    "Set timer to four hours",
    "Set timer to four hours ^1",
    "Set timer to five hours",
    "Set timer to five hours ^1",
    "Turn on the living room light",
    "Turn on the living room light ^1",
    "Turn on the living room light ^2",
    "Turn on the living room light ^3",
    "Living room light off",

};
#endif /* #if (CFG_DSPOTTER_BASE_VERSION == 0) */

static TaskHandle_t get_handle(void)
{
    return g_handler;
}

static void set_handle(TaskHandle_t thread_handler)
{
    g_handler = thread_handler;
}

struct va_task *task_constructor(void)
{
    struct va_task *p_va_task;
    struct msd_hw_params *params;

    p_va_task = (struct va_task *)pvPortMalloc(sizeof(struct va_task));
    memset(p_va_task, 0, sizeof(struct va_task));

    p_va_task->thread_stack_dep = configMINIMAL_STACK_SIZE * 4;
    p_va_task->thread_priority = configMAX_PRIORITIES - 2;

    p_va_task->priv = pvPortMalloc(sizeof(struct msd_hw_params));
    memset(p_va_task->priv, 0, sizeof(struct msd_hw_params));

    params = p_va_task->priv;

    params->format = MSD_PCM_FMT_S16_LE;
    params->channels = 4;
    params->period_count = 12;
    params->rate = 16000;
    params->period_size = params->rate / 100; //10ms;

    return p_va_task;
}

void task_destructor(struct va_task *p_va_task)
{
    vPortFree(p_va_task->priv);
    vPortFree(p_va_task);
}

#ifdef MTK_AIA_ENABLE
volatile int aia_upload_done = 1;
#endif /* #ifdef MTK_AIA_ENABLE */
void notify_va_upload(int type, int value)
{
    if (type == VA_NOTIFY_VAD_PASS || type == VA_NOTIFY_WAKEWORD_PASS) {
        printf("[notify_va_upload] WW.\n");
        g_event = EVT_UPLOAD;
    } else if (type == VA_NOTIFY_LOCAL_CMD_TRIGGER_PASS && value >= 0) {
        printf("[notify_va_upload] Trigger word : %d.\n", value);
        g_event = EVT_LC_TRIGGER;
    } else if (type == VA_NOTIFY_LOCAL_CMD_COMMAND_PASS && value >= 0) {
        printf("[notify_va_upload] command idx [%d].\n", value);
        g_event = EVT_LC_COMMAND;
    }    else if (type == VA_NOTIFY_LOCAL_CMD_COMMAND_TIMEOUT) {
        g_event = EVT_LC_COMMAND_TO;
    }
#ifdef MTK_AIA_ENABLE
    if (g_event == EVT_UPLOAD) {
        uint8_t aia_status = 0;
        uint8_t aia_link = 0;
        extern int isAiaInitDone;
        aia_status = wifi_connection_get_link_status(&aia_link);
        if (aia_link == 1) {
            printf("[notify_va_upload][AIA] aia_link=%d, the station is connecting to an AP router.\n", aia_link);
        } else if (aia_link == 0) {
            printf("[notify_va_upload][AIA] aia_link=%d, the station does not connect to an AP router.\n", aia_link);
            return;
        }

        if (aia_upload_done == 1 && aia_link == 1 && isAiaInitDone == 1) {
            printf("get handler! \n");
            xTaskNotifyGive(get_handle());
        } else {
            printf("Do nothing! : %d \n", aia_upload_done);
        }
    } else {
        xTaskNotifyGive(get_handle());
    }
#else /* #ifdef MTK_AIA_ENABLE */
    xTaskNotifyGive(get_handle());
#endif /* #ifdef MTK_AIA_ENABLE */
}

static int va_pcm_start(struct msd_hw_params *p_params)
{
    int ret = 0;

    ret = snd_pcm_open(&hostless_snd, "ADSP_HOSTLESS_VA", MSD_STREAM_CAPTURE, 0);
    if (ret < 0) {
        printf("Hostless open fail %d.\n", ret);
        return ret;
    }

    va_started = 1;
    ret = snd_pcm_hw_params(hostless_snd, p_params);
    if (ret) {
        printf("Hostless hw params error %d.\n", ret);
        goto exit0;
    }

    ret = snd_pcm_prepare(hostless_snd);
    if (ret < 0) {
        printf("Hostless prepare error %d.\n", ret);
        goto exit0;
    }

    snd_pcm_start(hostless_snd);

    return 0;

exit0:
    snd_pcm_hw_free(hostless_snd);
    snd_pcm_close(hostless_snd);

    return ret;
}

static int va_pcm_stop(void)
{
    int ret = 0;

    ret = snd_pcm_drop(hostless_snd);
    if (ret)
        printf("Hostless drop error %d.\n", ret);

    ret = snd_pcm_hw_free(hostless_snd);
    if (ret)
        printf("Hostless free error %d.\n", ret);

    ret = snd_pcm_close(hostless_snd);
    if (ret)
        printf("Hostless close error %d.\n", ret);

    return 0;
}

#ifdef MTK_AIA_ENABLE

#include "stream_buffer.h"
StreamBufferHandle_t xAiaRecordingStreamBuffer;
const size_t xAiaRecordingStreamBufferSizeBytes = 2560 * 8, xAiaRecordingTriggerLevel = 1;
volatile int aia_va_record_flag = 1;

static unsigned int do_va_voiceupload_capture(void *fid, struct msd_hw_params *p_params)
{
    aia_upload_done = 0;
    void *buffer;
    void *align_buf;
    void *half_buffer;
    unsigned int frames_read = 0;
    unsigned int period_bytes = 0;
    unsigned int rate = p_params->rate;
    int period_size = p_params->period_size;
    int chunk_bytes = 0;
    int chunk_size = 0;
    int channels = p_params->channels;
    int bitdepth = snd_pcm_format_width(p_params->format);
    int ret;
    size_t xBytesSent;

    ret = snd_pcm_open(&upload_snd, "ADSP_VA_FE", MSD_STREAM_CAPTURE, 0);
    if (ret) {
        aia_upload_done = 1;
        printf("Upload open fail %d.\n", ret);
        return 0;
    }
    snd_pcm_hw_params(upload_snd, p_params);
    ret = snd_pcm_prepare(upload_snd);
    if (ret < 0) {
        aia_upload_done = 1;
        printf("Upload prepare error %d.\n", ret);
        goto exit1;
    }

    period_bytes = period_size * channels * bitdepth >> 3;
    chunk_bytes = period_bytes * 4;
    chunk_size = period_size * 4;

    buffer = malloc(chunk_bytes + CFG_ALIGNMENT_BYTES);
    if (!buffer) {
        printf("Err: Unable to allocate %u bytes\n", chunk_bytes);
        vPortFree(buffer);
        ret = snd_pcm_hw_free(upload_snd);
        if (ret)
            printf("Upload hw free error %d.\n", ret);
        ret = snd_pcm_close(upload_snd);
        if (ret)
            printf("Upload close error %d.\n", ret);
        aia_upload_done = 1;
        return 0;
    }

    /* for AIA one channel pcm data limit */
    half_buffer = malloc((chunk_bytes / 2)) ;

    align_buf = (void *)((((unsigned int)buffer + chunk_bytes + CFG_ALIGNMENT_BYTES) & (~(CFG_ALIGNMENT_BYTES - 1))) - chunk_bytes);
    align_buf = (void *)HAL_CACHE_VIRTUAL_TO_PHYSICAL((unsigned int)align_buf);

    /* the flag will be set to 0 by AIA task when capture stage is done */
    aia_va_record_flag = 1;

    while (aia_va_record_flag) {
        ret = snd_pcm_read(upload_snd, align_buf, chunk_size);
        if (ret != chunk_size)
            printf("Warning: Expect read bytes %d, actually read bytes %d\n", chunk_size, ret);
        frames_read += ret;

        /* Seprate single channel data for AIA */
        for (int i = 0 ; i < chunk_size ; i++) {
            ((int16_t *)half_buffer)[i] = ((int16_t *)align_buf)[ i * 2 ];
        }

        /* Send recorded pcm data to stream buffer for AIA task to receive */
        xBytesSent = xStreamBufferSend(xAiaRecordingStreamBuffer, half_buffer, (chunk_bytes / 2), 0);
        //if(xBytesSent != (chunk_bytes / 2)) {
        //printf("xBytesSent != chunk_bytes\n");
        //}
    }

    aia_va_record_flag = 0;
    free(buffer);
    free(half_buffer);
    /* Discard all data in stream buffer for next capture */
    xStreamBufferReset(xAiaRecordingStreamBuffer);
    ret = snd_pcm_drop(upload_snd);
    if (ret)
        printf("drop error %d\n", ret);
exit1:
    ret = snd_pcm_hw_free(upload_snd);
    if (ret)
        printf("free error %d\n", ret);

    ret = snd_pcm_close(upload_snd);
    if (ret)
        printf("close error %d\n", ret);
    aia_upload_done = 1;
    return frames_read;
}
#else /* #ifdef MTK_AIA_ENABLE */

static unsigned int do_va_voiceupload_capture(void *fid, struct msd_hw_params *p_params)
{
    void *buffer;
    void *align_buf;
    unsigned int frames_read = 0;
    unsigned int period_bytes = 0;
    unsigned int rate = p_params->rate;
    int period_size = p_params->period_size;
    int chunk_bytes = 0;
    int chunk_size = 0;
    int channels = p_params->channels;
    int bitdepth = snd_pcm_format_width(p_params->format);
#ifdef FREERTOS_POSIX_SUPPORT
    struct timespec end;
    struct timespec now;
    struct timespec st;
    struct timespec ed;
    int cur_time;
    int total_time = 0;
    int total_count = 0;
    int min_time = 1000000;
    int max_time = 0;
#endif /* #ifdef FREERTOS_POSIX_SUPPORT */
    int first_read_time = 1;
    int ret;
    //unsigned int alarm_index = VOW_ACCEPT;

    voice_upload = 1;

    ret = snd_pcm_open(&upload_snd, "ADSP_VA_FE", MSD_STREAM_CAPTURE, 0);
    if (ret) {
        printf("Upload open fail %d.\n", ret);
        return ret;
    }
    snd_pcm_hw_params(upload_snd, p_params);
    ret = snd_pcm_prepare(upload_snd);
    if (ret < 0) {
        printf("Upload prepare error %d.\n", ret);
        return ret;
    }

    period_bytes = period_size * channels * bitdepth >> 3;
    chunk_bytes = period_bytes * 4;
    chunk_size = period_size * 4;

    buffer = malloc(chunk_bytes + CFG_ALIGNMENT_BYTES);
    if (!buffer) {
        printf("Err: Unable to allocate %u bytes\n", chunk_bytes);
        vPortFree(buffer);
        ret = snd_pcm_hw_free(upload_snd);
        if (ret)
            printf("Upload hw free error %d.\n", ret);
        ret = snd_pcm_close(upload_snd);
        if (ret)
            printf("Upload close error %d.\n", ret);
        return 0;
    }
    align_buf = (void *)((((unsigned int)buffer + chunk_bytes + CFG_ALIGNMENT_BYTES) & (~(CFG_ALIGNMENT_BYTES - 1))) - chunk_bytes);
    align_buf = (void *)HAL_CACHE_VIRTUAL_TO_PHYSICAL((unsigned int)align_buf);
    printf("[%s] Allocated 0x%x adjusted to 0x%x, size = %x\n", __func__, (unsigned int)buffer, (unsigned int)align_buf, chunk_bytes);
    buffer_g = align_buf;

#ifdef FREERTOS_POSIX_SUPPORT
    clock_gettime(CLOCK_MONOTONIC, &now);
    end.tv_sec = now.tv_sec + cap_time;
    end.tv_nsec = now.tv_nsec;
#endif /* #ifdef FREERTOS_POSIX_SUPPORT */

    //int count = 0;
    while (voice_upload) {
        //printf("[yajun test] va read (%d)\n", count++);
#ifdef FREERTOS_POSIX_SUPPORT
        clock_gettime(CLOCK_MONOTONIC, &st);
#endif /* #ifdef FREERTOS_POSIX_SUPPORT */
        ret = snd_pcm_read(upload_snd, align_buf, chunk_size);
#ifdef FREERTOS_POSIX_SUPPORT
        clock_gettime(CLOCK_MONOTONIC, &ed);
#endif /* #ifdef FREERTOS_POSIX_SUPPORT */
        if (ret != chunk_size)
            printf("Warning: Expect read bytes %d, actually read bytes %d\n", chunk_size, ret);

#ifdef FILE_SYS_SUPPORT
        FIL *fd = (FIL *)fid;
        UINT f_bw;
        printf("%d: %p %d\r\n", __LINE__, align_buf, chunk_bytes);
        printf("%d\r\n", chunk_size);
        f_write(fd, align_buf, chunk_bytes, &f_bw);
        if ((int)f_bw != chunk_bytes) {
            printf("Error capturing sample, period_bytes = 0x%x\n", chunk_bytes);
            break;
        }
#endif /* #ifdef FILE_SYS_SUPPORT */
        frames_read += chunk_size;
        if (first_read_time) {
#ifdef FREERTOS_POSIX_SUPPORT
            cur_time = (int)(ed.tv_sec - st.tv_sec) * 1000000 + (int)(ed.tv_nsec - st.tv_nsec) / 1000;
            total_time += cur_time;
            total_count++;
            if (min_time > cur_time)
                min_time = cur_time;
            if (max_time < cur_time)
                max_time = cur_time;
#endif /* #ifdef FREERTOS_POSIX_SUPPORT */
            if ((unsigned int)(frames_read * 1000 / rate) > 10) {
#ifdef FREERTOS_POSIX_SUPPORT
                printf("first read frames(%d frames) cost time(%d us), count(%d), avg(%d), min(%d), max(%d)\n",
                       frames_read, total_time, total_count, total_time / total_count, min_time, max_time);
#endif /* #ifdef FREERTOS_POSIX_SUPPORT */
                first_read_time = 0;
                printf("[%s] alert, vow_enable:%d\n", __func__, vow_enable);
                if (vow_enable) {
                    notify_alarm_play(0);
                    voice_upload = 0;
                }
            }
        }
#ifdef FREERTOS_POSIX_SUPPORT
        if (cap_time) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > end.tv_sec ||
                (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
                break;
        }
#endif /* #ifdef FREERTOS_POSIX_SUPPORT */
        //taskYIELD();
    }
    voice_upload = 0;
    free(buffer);
    ret = snd_pcm_drop(upload_snd);
    if (ret)
        printf("drop error %d\n", ret);

    ret = snd_pcm_hw_free(upload_snd);
    if (ret)
        printf("free error %d\n", ret);

    ret = snd_pcm_close(upload_snd);
    if (ret)
        printf("close error %d\n", ret);

    return frames_read;
}
#endif /* #ifdef MTK_AIA_ENABLE */


static void va_voiceupload_capture(struct msd_hw_params *p_params)
{
    unsigned int total_frames = 0;
#ifdef WAV_DUMP
    struct wav_header header;
#endif /* #ifdef WAV_DUMP */
    //va_wake_lock();
    printf("ww detect , start to read voice\n");

#ifdef FILE_SYS_SUPPORT
    static int voice_upload_times = 0;
    FIL fd;
    FRESULT f_ret;
    //  int rd;
    char file_path[128] = {'\0'};

    snprintf(file_path, sizeof(file_path), "SD:/data/va_%d.wav", voice_upload_times++);
    printf("Store to:%s\n", file_path);

    f_ret = f_open(&fd, file_path, FA_CREATE_ALWAYS | FA_WRITE);
    if (f_ret) {
        printf("Failed to open %s, ret: %d\n", file_path, f_ret);
        //va_wake_unlock();
        return;
    }

#ifdef WAV_DUMP
    UINT f_br;
    header.riff_id = ID_RIFF;
    header.riff_sz = 0;
    header.riff_fmt = ID_WAVE;
    header.fmt_id = ID_FMT;
    header.fmt_sz = snd_pcm_format_width(p_params->format);
    header.audio_format = FORMAT_PCM;
    header.num_channels = 2; //should same with upload channel number
    header.sample_rate = p_params->rate;
    header.bits_per_sample = header.fmt_sz;
    header.byte_rate = (header.bits_per_sample / 8) * header.num_channels * header.sample_rate;
    header.block_align = header.num_channels * (header.bits_per_sample / 8);
    header.data_id = ID_DATA;
    /* leave enough room for header */
    f_lseek(&fd, sizeof(struct wav_header));
#endif /* #ifdef WAV_DUMP */
#else /* #ifdef FILE_SYS_SUPPORT */
    int fd;
#endif /* #ifdef FILE_SYS_SUPPORT */


    total_frames = do_va_voiceupload_capture(&fd, p_params);
    printf("Captured %u frames\n", total_frames);

#ifdef FILE_SYS_SUPPORT
#ifdef WAV_DUMP
    /* write header now all information is known */
    header.data_sz = total_frames * header.block_align;
    header.riff_sz = header.data_sz + sizeof(header) - 8;
    f_lseek(fd, 0);
    f_ret = f_write(&fd, &header, sizeof(struct wav_header), &f_br);
    if (f_ret) {
        printf("Failed to write to file %s, ret: %d\n", file_name, f_ret);
    }
#endif /* #ifdef WAV_DUMP */

    f_close(&fd);
#endif /* #ifdef FILE_SYS_SUPPORT */
    //va_wake_unlock();
}

#ifdef MTK_AIA_ENABLE
#include "event_groups.h"
extern EventGroupHandle_t aia_eg;
#endif /* #ifdef MTK_AIA_ENABLE */

void va_capture_loop(void *void_this)
{
    struct va_task *this = (struct va_task *)void_this;
    struct msd_hw_params *params = (struct msd_hw_params *)this->priv;

    struct msd_hw_params hostless_param;
    struct msd_hw_params upload_param;

    int ret = 0;

    memcpy((void *)&hostless_param, (void *)params, sizeof(struct msd_hw_params));
    memcpy((void *)&upload_param, (void *)params, sizeof(struct msd_hw_params));

    upload_param.channels = 2;
#ifdef MTK_AIA_ENABLE
    /* Create Stream Buffer for AIA */
    xAiaRecordingStreamBuffer = xStreamBufferCreate(xAiaRecordingStreamBufferSizeBytes, xAiaRecordingTriggerLevel);
#endif /* #ifdef MTK_AIA_ENABLE */
    ret = va_pcm_start(&hostless_param);
    if (ret != 0)
        goto _exit;

    set_handle(xTaskGetCurrentTaskHandle());
    this->alarm_handler = (TaskHandle_t)va_alarm_create();

    if (!(this->alarm_handler))
        goto _stop;

    while (1) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        if (va_stoped)
            goto _stop;

        switch (g_event) {
            case EVT_UPLOAD:
#ifdef MTK_AIA_ENABLE
                xEventGroupSetBits(aia_eg, (0x1 << 4));
#endif /* #ifdef MTK_AIA_ENABLE */
                va_voiceupload_capture(&upload_param);
                break;
            case EVT_LC_TRIGGER:
                notify_alarm_play(0);
                break;
            case EVT_LC_COMMAND:
                notify_alarm_play(1);
                break;
            default:
                break;
        }
    }

_stop:
    va_pcm_stop();
    va_stoped = 0;
    va_started = 0;
_exit:
    if (this->alarm_handler) {
        vTaskDelete(this->alarm_handler);
        this->alarm_handler = NULL;
    }
    task_destructor(this);
    set_handle(NULL);
    vTaskDelete(NULL);
    return;
}

void va_cmd_show_usage(void)
{
    printf("\nUSAGE:\n");
    printf("    va [OPTIONS] [<params>] [OPTIONS] [<params>] ...\n");
    printf("\nFLAGS:\n");
    printf("    -r \t\t\t Sample Rate, 8000/16000/48000, defaut 16000\n");
    printf("    -p \t\t\t Period Size, unit frames, default 10ms\n");
    printf("    -n \t\t\t Period Count, default 6\n");
    printf("    -t \t\t\t Capture time, unit second, default not setting\n");
    printf("    -v \t\t\t Voice Alarm when wake word success, set to 0 or 1, defaut 0(off)\n");
}

int va_cmd_parser(int argc, char *argv[], struct msd_hw_params *params)
{
    int option_index;

    if (argc <= 1) {
        printf("Use defaut params setting\n");
        return 0;
    }

    // Parser params
    for (option_index = 0; option_index < argc; option_index++) {
        if (strcmp(argv[option_index], "-r") == 0) {
            params->rate = atoi(argv[option_index + 1]);
            option_index++;
            continue;
        } else if (strcmp(argv[option_index], "-p") == 0) {
            params->period_size = atoi(argv[option_index + 1]);
            option_index++;
            continue;
        } else if (strcmp(argv[option_index], "-n") == 0) {
            params->period_count = atoi(argv[option_index + 1]);
            option_index++;
            continue;
        } else if (strcmp(argv[option_index], "-t") == 0) {
            cap_time = atoi(argv[option_index + 1]);
            option_index++;
            continue;
        } else if (strcmp(argv[option_index], "-v") == 0) {
            vow_enable = !!(atoi(argv[option_index + 1]));
            option_index++;
            continue;
        } else {
            printf("This is not a support command option: %s\n", argv[option_index]);
            va_cmd_show_usage();
            return -1;
        }
    }
    return 0;
}


void va_main(int argc, char *argv[])
{
    int ret = 0;
    if (get_handle()) {
        printf("The task of VAD has been started.");
        return;
    }
    struct va_task *p_va_task = task_constructor();
    struct msd_hw_params *params = (struct msd_hw_params *)p_va_task->priv;

    ret = va_cmd_parser(argc, argv, params);
    if (ret < 0)
        return;

    connect_route("ADSP_HOSTLESS_VA", "ADSP_UL9_IN_BE", 1, CONNECT_FE_BE);
    connect_route("ADSP_VA_FE", "ADSP_UL9_IN_BE", 1, CONNECT_FE_BE);
    connect_route("ADSP_UL9_IN_BE", "ETDM1_IN_BE", 1, CONNECT_FE_BE);
    connect_route("ADSP_UL9_IN_BE", "GASRC0_C", 1, CONNECT_FE_BE);
    connect_route("GASRC0_C", "dummy_end_c", 1, CONNECT_FE_BE);
    connect_route("I_42", "O_26", 1, CONNECT_IO_PORT);
    connect_route("I_43", "O_27", 1, CONNECT_IO_PORT);
    connect_route("I_22", "O_28", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_29", 1, CONNECT_IO_PORT);

    xTaskCreate(va_capture_loop,
                "ap_aud_t_va",
                p_va_task->thread_stack_dep,
                (void *)p_va_task,
                p_va_task->thread_priority,
                &p_va_task->thread_handler);
}

void va_stop(int argc, char *argv[])
{
    if (!get_handle()) {
        printf("The task of VAD has not been started.");
        return;
    }
    if (argc < 1) {
        printf("\nUSAGE:\n");
        printf("	va_stop 0/1\n");
        printf("\nPARAMS:\n");
        printf("	0 \t\t\t Stop VA Upload, VA will continue capture\n");
        printf("	1 \t\t\t Stop VA Capture\n");
        return;
    }

    if (strcmp(argv[0], "0") == 0) {
        voice_upload = 0;
    } else if (strcmp(argv[0], "1") == 0) {
        va_stoped = 1;
        if (voice_upload == 0) {
            xTaskNotifyGive(get_handle());
        }
        voice_upload = 0;

    } else {
        printf("This is not a support command param: %s\n", argv[0]);
    }

    return;
}


#ifdef DSP_DEBUG_DUMP
extern void dump_open(int dump_position, int *pcm_info);
#endif /* #ifdef DSP_DEBUG_DUMP */

void va_dump(int argc, char *argv[])
{
    if (argc < 1) {
        printf("\nUSAGE:\n");
        printf("	va_dump <dump position> -t[opt]<time/ms> -c[opt]<channel> -f[opt]<bitwidth>\n");
        printf("\nPARAMS:\n");
        printf("	<dump position> \t\t\t 1/2/3/4\n");
        printf("	-t[opt]<time/ms> \t\t\t unit ms\n");
        printf("	-c[opt]<channel> \t\t\t 2/4\n");
        printf("	-f[opt]<bitwidth> \t\t\t 16/32\n");
        return;
    }
#ifdef DSP_DEBUG_DUMP
    int params[3] = {0};
    int option_index;
    int dump_position = atoi(argv[0]) - 1;
    if (dump_position < 0 || dump_position > 3)
        printf("[Params error] not support position %d.\n", dump_position);

    for (option_index = 1; option_index < argc; option_index++) {
        if (strcmp(argv[option_index], "-t") == 0) {
            params[0] = atoi(argv[option_index + 1]);
            option_index++;
            continue;
        }
        if (strcmp(argv[option_index], "-c") == 0) {
            params[1] = atoi(argv[option_index + 1]);
            option_index++;
            continue;
        }
        if (strcmp(argv[option_index], "-f") == 0) {
            params[2] = atoi(argv[option_index + 1]);
            option_index++;
            continue;
        }
    }
    dump_open(dump_position, params);
#endif /* #ifdef DSP_DEBUG_DUMP */
    return;
}

/*============================================================*/

TaskHandle_t g_test_handler;

void notify_my_test(void)
{
    xTaskNotifyGive(g_test_handler);
}

void task1_func(void *void_this)
{
    while (1) {
        taskENTER_CRITICAL();
        vTaskSuspendAll();
        printf("task1 running.\n");
        if (!xTaskResumeAll())
            taskYIELD();
        taskEXIT_CRITICAL();
        vTaskDelay(200);
    }
}

void task2_func(void *void_this)
{
    while (1) {
        taskENTER_CRITICAL();
        vTaskSuspendAll();
        printf("task2 running.\n");
        if (!xTaskResumeAll())
            taskYIELD();
        taskEXIT_CRITICAL();
        vTaskDelay(200);
    }
}

void task3_func(void *void_this)
{
    while (1) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        taskENTER_CRITICAL();
        vTaskSuspendAll();
        printf("task3 running.\n");
        if (!xTaskResumeAll())
            taskYIELD();
        taskEXIT_CRITICAL();
        vTaskDelay(200);
    }
}

void mtask(void)
{
    xTaskCreate(task1_func,
                "task1",
                configMINIMAL_STACK_SIZE,
                NULL,
                configMAX_PRIORITIES - 1,
                NULL);
    xTaskCreate(task2_func,
                "task2",
                configMINIMAL_STACK_SIZE,
                NULL,
                configMAX_PRIORITIES - 1,
                NULL);
    xTaskCreate(task3_func,
                "task3",
                configMINIMAL_STACK_SIZE,
                NULL,
                configMAX_PRIORITIES - 1,
                &g_test_handler);
}

