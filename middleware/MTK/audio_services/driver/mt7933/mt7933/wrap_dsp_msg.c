#include "sound/utils/include/aud_log.h"

#include "asound.h"
#include "tinycontrol.h"
#include "tinypcm.h"
#include "audio_shared_info.h"
#include "wrap_dsp_msg.h"
#include "mt7933-adsp-utils.h"
#include <errno.h>
#include <string.h>
#ifdef AUDIO_FREERTOS_SUPPORT
#include "freertos/snd_portable.h"
#endif /* #ifdef AUDIO_FREERTOS_SUPPORT */
#define WARP_MAX_MSG_QUEUE_SIZE 20

static sound_t *r_snd;

enum {
    CAPTURE_STATE_OPENED,
    CAPTURE_STATE_STARTED,
    CAPTURE_STATE_STOPPEP,
    CAPTURE_STATE_CLOSED,
    CAPTURE_STATE_INVALID,
};

#define STATE_STR_ENTRY(s) [s]=#s

static char *state_str[CAPTURE_STATE_INVALID] = {
    STATE_STR_ENTRY(CAPTURE_STATE_OPENED),
    STATE_STR_ENTRY(CAPTURE_STATE_STARTED),
    STATE_STR_ENTRY(CAPTURE_STATE_STOPPEP),
    STATE_STR_ENTRY(CAPTURE_STATE_CLOSED),
};

static int current_state = CAPTURE_STATE_CLOSED;

static struct msd_hw_params g_hw_params = {
    .format = MSD_PCM_FMT_S16_LE,
    .channels = 4,
    .rate = 16000,
    .period_size = 160,
    .period_count = 4,
};

static void wrap_dsp_msg_task_loop(void *void_this);

static WRAP_MSG_TASK g_wrap_msg_task = {
    .thread_name = "wrap_dsp_msg",
    .thread_stack_dep = 4 * configMINIMAL_STACK_SIZE,
    .thread_priority = tskIDLE_PRIORITY,
    .task_loop_func = wrap_dsp_msg_task_loop,
};

extern int connect_route(const char *src, const char *sink, int value, int type);

static void wrap_handle_open_capture(struct ipi_msg_t *p_ipi_msg)
{
    int ret = 0;
    struct msd_hw_params dsp_params;

    xSemaphoreTake(g_wrap_msg_task.sema, portMAX_DELAY);
    aud_msg("START_CAPTURE begin\n");
    if (current_state != CAPTURE_STATE_CLOSED) {
        aud_error("wrong state:%s", state_str[current_state]);
        ret = -EINVAL;
        return;
    }
    connect_route("ADSP_LOCAL_RECORD", "BE_UL9", 1, CONNECT_FE_BE);
    connect_route("BE_UL9", "INTADC_in", 1, CONNECT_FE_BE);
    connect_route("BE_UL9", "GASRC0_C", 1, CONNECT_FE_BE);
    connect_route("GASRC0_C", "dummy_end_c", 1, CONNECT_FE_BE);
    connect_route("I_60", "O_26", 1, CONNECT_IO_PORT);
    connect_route("I_61", "O_27", 1, CONNECT_IO_PORT);
    connect_route("I_32", "O_28", 1, CONNECT_IO_PORT);
    connect_route("I_33", "O_29", 1, CONNECT_IO_PORT);
    ret = snd_pcm_open(&r_snd, "ADSP_LOCAL_RECORD", MSD_STREAM_CAPTURE, 0);
    if (ret < 0) {
        aud_error("ret:%d", ret);
        return;
    }
    memcpy((void *)&dsp_params,
           (void *)(p_ipi_msg->payload),
           sizeof(struct msd_hw_params));
    if (!dsp_params.channels) {
        dsp_params.channels = g_hw_params.channels;
    }
    if (!dsp_params.rate) {
        dsp_params.rate = g_hw_params.rate;
    }
    if (!dsp_params.format) {
        dsp_params.format = g_hw_params.format;
    }
    if (!dsp_params.period_count) {
        dsp_params.period_count = g_hw_params.period_count;
    }
    if (!dsp_params.period_size) {
        dsp_params.period_size = g_hw_params.period_size;
    }

    ret = snd_pcm_hw_params(r_snd, &dsp_params);
    if (ret < 0) {
        snd_pcm_close(r_snd);
        aud_error("ret:%d", ret);
        return;
    }
    ret = snd_pcm_prepare(r_snd);
    if (ret < 0) {
        snd_pcm_hw_free(r_snd);
        snd_pcm_close(r_snd);
        aud_error("ret:%d", ret);
        return;
    }
    current_state = CAPTURE_STATE_OPENED;
    aud_msg("OPEN_CAPTURE over\n");
    xSemaphoreGive(g_wrap_msg_task.sema);
}

static void wrap_handle_start_capture(struct ipi_msg_t *p_ipi_msg)
{
    int ret = 0;

    xSemaphoreTake(g_wrap_msg_task.sema, portMAX_DELAY);
    aud_msg("START_CAPTURE begin\n");
    if (current_state != CAPTURE_STATE_OPENED) {
        aud_error("wrong state:%s", state_str[current_state]);
        ret = -EINVAL;
        return;
    }
    ret = snd_pcm_start(r_snd);
    if (ret < 0) {
        aud_error("ret:%d", ret);
        return;
    }
    current_state = CAPTURE_STATE_STARTED;
    aud_msg("START_CAPTURE over\n");
    xSemaphoreGive(g_wrap_msg_task.sema);
}

static void wrap_handle_stop_capture(struct ipi_msg_t *p_ipi_msg)
{
    int ret = 0;
    xSemaphoreTake(g_wrap_msg_task.sema, portMAX_DELAY);
    aud_msg("STOP_CAPTURE begin\n");
    if (current_state != CAPTURE_STATE_STARTED) {
        aud_error("wrong state:%s", state_str[current_state]);
        ret = -EINVAL;
        return;
    }
    ret = snd_pcm_drop(r_snd);
    if (ret < 0) {
        aud_error("ret:%d", ret);
        return;
    }
    current_state = CAPTURE_STATE_STOPPEP;
    aud_msg("STOP_CAPTURE over\n");
    xSemaphoreGive(g_wrap_msg_task.sema);
}

static void wrap_handle_close_capture(struct ipi_msg_t *p_ipi_msg)
{
    int ret = 0;

    xSemaphoreTake(g_wrap_msg_task.sema, portMAX_DELAY);
    aud_msg("CLOSE_CAPTURE begin\n");
    if (current_state != CAPTURE_STATE_STOPPEP) {
        aud_error("wrong state:%s", state_str[current_state]);
        ret = -EINVAL;
        return;
    }
    ret = snd_pcm_hw_free(r_snd);
    if (ret < 0) {
        aud_error("ret:%d", ret);
        return;
    }
    ret = snd_pcm_close(r_snd);
    if (ret < 0) {
        aud_error("ret:%d", ret);
        return;
    }
    current_state = CAPTURE_STATE_CLOSED;
    aud_msg("CLOSE_CAPTURE over\n");
    xSemaphoreGive(g_wrap_msg_task.sema);
}

void wrap_notify_dsp(void)
{
    int magic = WRAP_NOTIFY_MAGIC;

    mt7933_adsp_send_ipi_cmd(NULL,
                             TASK_SCENE_RECORD,
                             AUDIO_IPI_LAYER_TO_DSP,
                             AUDIO_IPI_PAYLOAD,
                             AUDIO_IPI_MSG_DIRECT_SEND,
                             MSG_TO_DSP_NOTIFY_RECORD_T,
                             sizeof(int), 0, (char *)&magic);
}

static int wrap_handle_ipc_msg(struct ipi_msg_t *p_ipi_msg)
{
    //  int ret = 0;
    int cmd = p_ipi_msg->msg_id;

    if (p_ipi_msg->task_scene != TASK_SCENE_RECORD) {
        return -EINVAL;
    }
    snd_msg_lock();
    switch (cmd) {
        case MSG_TO_HOST_RECORD_OPEN_CAPTURE:
            wrap_handle_open_capture(p_ipi_msg);
            break;
        case MSG_TO_HOST_RECORD_START_CAPTURE:
            wrap_handle_start_capture(p_ipi_msg);
            break;
        case MSG_TO_HOST_RECORD_STOP_CAPTURE:
            wrap_handle_stop_capture(p_ipi_msg);
            break;
        case MSG_TO_HOST_RECORD_CLOSE_CAPTURE:
            wrap_handle_close_capture(p_ipi_msg);
            break;
        default:
            aud_msg("Not a support cmd: %d\n", cmd);
            break;
    }
    wrap_notify_dsp();

    snd_msg_unlock();
    return 0;
}

static void wrap_dsp_msg_task_loop(void *void_this)
{
    WRAP_MSG_TASK *this = void_this;
    int ret;
    struct ipi_msg_t ipi_msg;
    aud_msg("+%s\n", __func__);

    while (1) {
        ret = xQueueReceive(this->msg_queue, &ipi_msg, portMAX_DELAY);
        if (ret != pdTRUE) {
            aud_error("%s, receive msg failed\n", __func__);
            continue;
        }
        wrap_handle_ipc_msg(&ipi_msg);
    }
}

int capture_wrap_msg_hdl(struct ipi_msg_t *p_ipi_msg)
{
    int ret;
    ret = xQueueSendToBack(g_wrap_msg_task.msg_queue, p_ipi_msg, 0);
    if (ret != pdTRUE) {
        aud_error("%s, send msg failed\n", __func__);
        return -1;
    }
    return 0;
}

int wrap_dsp_msg_probe(void)
{
    int ret = 0;
    g_wrap_msg_task.msg_queue = xQueueCreate(WARP_MAX_MSG_QUEUE_SIZE, sizeof(struct ipi_msg_t));
    g_wrap_msg_task.sema = xSemaphoreCreateBinary();

    xSemaphoreGive(g_wrap_msg_task.sema);

    ret = xTaskCreate(g_wrap_msg_task.task_loop_func, g_wrap_msg_task.thread_name,
                      g_wrap_msg_task.thread_stack_dep, (void *)&g_wrap_msg_task,
                      g_wrap_msg_task.thread_priority, &g_wrap_msg_task.thread_handler);
    if (ret != pdTRUE) {
        aud_error("%s, create wrap msg task failed\n", __func__);
        return -1;
    }
    return 0;
}

void wrap_dsp_msg_remove(void)
{
    if (g_wrap_msg_task.thread_handler != 0) {
        vTaskDelete(g_wrap_msg_task.thread_handler);
        g_wrap_msg_task.thread_handler = 0;
    }

    vQueueDelete(g_wrap_msg_task.msg_queue);
}

void aud_wdt_function(void)
{
    set_execption_happen_flag();
    aud_msg("!!!ADSP Exception Trigger Audio Recovery Start!!!\n");
    switch (current_state) {
        case CAPTURE_STATE_STARTED:
            aud_msg("Doing Stop. Then Close.\n");
            wrap_handle_stop_capture(NULL);
        case CAPTURE_STATE_OPENED:
        case CAPTURE_STATE_STOPPEP:
            aud_msg("Doing Close.\n");
            wrap_handle_close_capture(NULL);
            break;
        default:
            aud_msg("Not Open. No Job to do.\n");
            break;
    }
    aud_msg("!!!ADSP Exception Trigger Audio Recovery Done!!!\n");
    clear_execption_happen_flag();
}


