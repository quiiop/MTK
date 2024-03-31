/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#define SBC_DEC_FRAME_LEN (128)

#include <stdlib.h>
#include "sbc_decoder.h"
#include "audio_test_utils.h"
#include "bt_a2dp_codec_internal.h"
//#define BT_A2DP_BITSTREAM_DUMP_DEBUG
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
/* #define AWS_DEBUG_CODE */
//#define AWS_DEBUG_CODE
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */

#ifdef MTK_BT_A2DP_SOURCE_SUPPORT
#include "sbc_codec.h"
#endif /* #ifdef MTK_BT_A2DP_SOURCE_SUPPORT */

#include "semphr.h"
extern xSemaphoreHandle g_xSemaphore_Audio;
void bt_audio_mutex_lock(xSemaphoreHandle handle)
{
    if (handle != NULL) {
        xSemaphoreTake(handle, portMAX_DELAY);
    }
}
void bt_audio_mutex_unlock(xSemaphoreHandle handle)
{
    if (handle != NULL) {
        xSemaphoreGive(handle);
    }
}

#include "audio_nvdm_common.h"
#include "audio_nvdm_coef.h"


#include "sound/include/tinypcm.h"
#ifdef MTK_FATFS_ENABLE
#include "ff.h"
#endif

#ifdef MTK_BT_AUDIO_PR
#include "apps_debug.h"
#endif /* #ifdef MTK_BT_AUDIO_PR */

typedef enum {
    SBC_CMD_QUEUE_EVENT_PLAY,
    SBC_CMD_QUEUE_EVENT_PAUSE,
    SBC_CMD_QUEUE_EVENT_STOP,
    SBC_CMD_QUEUE_EVENT_RESUME,
    SBC_CMD_QUEUE_EVENT_CLOSE,
    SBC_CMD_QUEUE_EVENT_SKIP
} sbc_cmd_queue_event_id_t;

static uint8_t sbc_decode_queue_reg_num = 0;
static QueueHandle_t sbc_decode_queue_handle = NULL;
static QueueHandle_t sbc_cmd_queue_handle = NULL;
static TaskHandle_t sbc_decode_task_handle = NULL;
static TaskHandle_t pcm_play_task_handle = NULL;
static int16_t *g_silence = NULL;
static sbc_decode_queue_event_id_t sbc_decode_queue_id_array[MAX_SBC_DECODE_FUNCTIONS];
static sbc_decode_internal_callback_t sbc_decode_queue_handler[MAX_SBC_DECODE_FUNCTIONS];
bt_a2dp_audio_internal_handle_t *bt_a2dp_internal_handle = NULL;
uint8_t a2dp_ucPCMOpened;
struct msd_hw_params a2dp_params;
sound_t *a2dp_w_snd;
#if defined(MTK_MINICLI_ENABLE) && (MTK_FATFS_ENABLE)
extern int g_audump_inited;
extern FIL g_fid;
#endif


#define PERIOD_COUNT    4
#define PERIOD_SIZE  1280
#define MAX_LOG_CNT   600

#ifdef MTK_BT_A2DP_SOURCE_SUPPORT
const bt_codec_sbc_t source_capability_sbc[1] = {
    {
        25,  /* min_bit_pool       */
        75,  /* max_bit_pool       */
        0x1, /* block_len: 16 only     */
        0x1, /* subband_num: 8 only   */
        SBC_ALLOCATION_SUPPORT,
        0xf, /* sample_rate: all   */
        0xf  /* channel_mode: all  */
    }
};
#endif /* #ifdef MTK_BT_A2DP_SOURCE_SUPPORT */

const bt_codec_sbc_t sink_capability_sbc[1] = {
    {
        18,  /* min_bit_pool       */
        75,  /* max_bit_pool       */
        0xf, /* block_len: all     */
        0xf, /* subband_num: all   */
        0x3, /* both snr/loudness  */
        0xf, /* sample_rate: all   */
        0xf  /* channel_mode: all  */
    }
};

const bt_codec_aac_t sink_capability_aac[1] = {
    {
        true,    /*VBR         */
        0xc0,    /*Object type */
        0x03,    /*Channels    */
        0x0ff8,  /*Sample_rate */
        0x60000  /*bit_rate, 384 Kbps */
    }
};

static void sbc_decode_event_send_from_isr(sbc_decode_queue_event_id_t id, void *parameter)
{
    sbc_decode_queue_event_t event;

    event.id = id;
    event.parameter = parameter;

    if (!sbc_decode_queue_handle) {
        TASK_LOG_MSGID_I("[SBC]queue not exist, id=%d\r\n", id);
        return;
    }

    if (xQueueSendFromISR(sbc_decode_queue_handle, &event, 0) != pdPASS) {
        TASK_LOG_MSGID_I("[SBC]queue not pass, id=%d\r\n", id);
        return;
    }
    return;
}

static void sbc_decode_event_register_callback(sbc_decode_queue_event_id_t reg_id, sbc_decode_internal_callback_t callback)
{
    uint32_t id_idx;

    for (id_idx = 0; id_idx < MAX_SBC_DECODE_FUNCTIONS; id_idx++) {
        if (sbc_decode_queue_id_array[id_idx] == SBC_DECODE_QUEUE_EVENT_NONE) {
            sbc_decode_queue_id_array[id_idx] = reg_id;
            sbc_decode_queue_handler[id_idx] = callback;
            sbc_decode_queue_reg_num++;
            break;
        }
    }
    return;
}

static void sbc_decode_event_deregister_callback(sbc_decode_queue_event_id_t dereg_id)
{
    uint32_t id_idx;
    TASK_LOG_MSGID_CTRL("deregister begin\r\n", 0);
    for (id_idx = 0; id_idx < MAX_SBC_DECODE_FUNCTIONS; id_idx++) {
        if (sbc_decode_queue_id_array[id_idx] == dereg_id) {
            sbc_decode_queue_id_array[id_idx] = SBC_DECODE_QUEUE_EVENT_NONE;
            sbc_decode_queue_reg_num--;
            break;
        }
    }

    bt_audio_mutex_lock(g_xSemaphore_Audio);
    if (sbc_decode_queue_handle != NULL) {
        vQueueDelete(sbc_decode_queue_handle);
        sbc_decode_queue_handle = NULL;
    }

    if (sbc_decode_task_handle != NULL) {
        vTaskDelete(sbc_decode_task_handle);
        sbc_decode_task_handle = NULL;
    }

    if (sbc_cmd_queue_handle != NULL) {
        vQueueDelete(sbc_cmd_queue_handle);
        sbc_cmd_queue_handle = NULL;
    }

    if (pcm_play_task_handle != NULL) {
        vTaskDelete(pcm_play_task_handle);
        pcm_play_task_handle = NULL;
    }

    if (g_silence != NULL) {
        free(g_silence);
        g_silence = NULL;
    }

    if (a2dp_ucPCMOpened == 1) {
        snd_pcm_drain(a2dp_w_snd);
        snd_pcm_hw_free(a2dp_w_snd);
        snd_pcm_close(a2dp_w_snd);
        connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
        connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
        connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
        a2dp_ucPCMOpened = 0;
    }
    bt_audio_mutex_unlock(g_xSemaphore_Audio);
    TASK_LOG_MSGID_CTRL("deregister end\r\n", 0);
    return;
}

static void sbc_play_task_main(void *arg)
{
    TASK_LOG_MSGID_I("sbc_play_task_main\r\n");
    static uint16_t log_cnt = 0;
    bt_a2dp_audio_internal_handle_t *internal_handle = bt_a2dp_internal_handle;
    uint16_t silence_size = PERIOD_COUNT * PERIOD_SIZE * internal_handle->channel_number;
    g_silence = (int16_t *)malloc(silence_size * sizeof(int16_t));
    if (g_silence == NULL) {
        TASK_LOG_MSGID_E("malloc failed = %x(%d)\r\n", g_silence, silence_size);
        return;
    }
    memset(g_silence, 0, silence_size * sizeof(int16_t));

    int ret = 0;
    sbc_cmd_queue_event_id_t pCmd;
    while (1) {
        if (xQueueReceive(sbc_cmd_queue_handle, &pCmd, (TickType_t)portMAX_DELAY) == pdPASS) {

            switch (pCmd) {
                case SBC_CMD_QUEUE_EVENT_PLAY: {
                        bt_audio_mutex_lock(g_xSemaphore_Audio);
                        //play pcm here
                        linear_buffer_16bit_information_t *p_pcm_buffer = &internal_handle->pcm_buffer_info;
                        uint16_t *stream_out_pcm_buff_ptr = p_pcm_buffer->buffer_base_pointer;
                        uint32_t wrote_bytes = 0;
                        uint32_t stream_out_pcm_buff_data_count = p_pcm_buffer->buffer_data_byte_count;
                        uint32_t stream_out_pcm_buffer_data_amount = (internal_handle->channel_number == 1) ? (stream_out_pcm_buff_data_count >> 1) : (stream_out_pcm_buff_data_count >> 2);
                        //TASK_LOG_MSGID_I("%d %d\r\n", stream_out_pcm_buff_data_count, stream_out_pcm_buffer_data_amount);

                        if (stream_out_pcm_buffer_data_amount > 0) {
                            int total_frames = stream_out_pcm_buffer_data_amount;

                            if (log_cnt == 0) {
                                TASK_LOG_MSGID_I("%d %d\r\n", stream_out_pcm_buff_data_count, stream_out_pcm_buffer_data_amount);
                            }

                            if (a2dp_ucPCMOpened == 1) {
#if defined(MTK_MINICLI_ENABLE) && (MTK_FATFS_ENABLE)
                                if (g_audump_inited == 1) {
                                    UINT f_bw;
                                    f_write(&g_fid, stream_out_pcm_buff_ptr, stream_out_pcm_buff_data_count, &f_bw);
                                    if ((uint32_t)f_bw != stream_out_pcm_buff_data_count)
                                        TASK_LOG_MSGID_E("f_write error, %d %d", f_bw, stream_out_pcm_buff_data_count);
                                }
#endif
                                ret = snd_pcm_write(a2dp_w_snd, stream_out_pcm_buff_ptr, total_frames);
                            }

                            if (log_cnt == 0) {
                                TASK_LOG_MSGID_I("wrote %d frames\r\n", ret);
                            }
                            if (log_cnt++ == MAX_LOG_CNT) {
                                log_cnt = 0;
                            }

                            /* update pcm buffer data size */
                            wrote_bytes = (internal_handle->channel_number == 1) ? (ret << 1) : (ret << 2);
                            //TASK_LOG_MSGID_I("%d, %d, %d\r\n", p_pcm_buffer->buffer_data_byte_count, ret, wrote_bytes);
                            p_pcm_buffer->buffer_data_byte_count = p_pcm_buffer->buffer_data_byte_count - wrote_bytes;
                            if (p_pcm_buffer->buffer_data_byte_count > PCM_BUFFER_LENGTH) {
                                TASK_LOG_MSGID_I("unexpexted pcm write: %d, %d %d\r\n", p_pcm_buffer->buffer_data_byte_count, wrote_bytes, ret);
                                p_pcm_buffer->buffer_data_byte_count = PCM_BUFFER_LENGTH;
                            }
                            if (p_pcm_buffer->buffer_data_byte_count > 0) {
                                TASK_LOG_MSGID_I("move pcm: %d %d\r\n", p_pcm_buffer->buffer_data_byte_count, wrote_bytes);
                                memmove(p_pcm_buffer->buffer_base_pointer, \
                                        p_pcm_buffer->buffer_base_pointer + (wrote_bytes >> 1), \
                                        p_pcm_buffer->buffer_data_byte_count);
                            }
                        } else {
                            // stream_out_pcm_buff no data
                            if (a2dp_ucPCMOpened == 1) {
                                snd_pcm_write(a2dp_w_snd, g_silence, silence_size / 2);
                            }
                            //TASK_LOG_MSGID_I("silence_size = %d\r\n", silence_size);
                        }

                        /* trigger decoder hisr */
                        if (internal_handle->handle.state == BT_CODEC_STATE_PLAY) {
                            sbc_decode_event_send_from_isr(SBC_DECODE_QUEUE_EVENT_DECODE, NULL);
                        }

#ifdef MTK_BT_AUDIO_PR
                        app_bt_dbg_audio_pr_write((uint8_t *)(stream_out_pcm_buff_ptr), 0, APP_BT_DBG_AUDIO_DECODE_PATH_PLAY);
#endif /* #ifdef MTK_BT_AUDIO_PR */

                        bt_audio_mutex_unlock(g_xSemaphore_Audio);
                        break;
                    }
                case SBC_CMD_QUEUE_EVENT_PAUSE:
                case SBC_CMD_QUEUE_EVENT_STOP:
                case SBC_CMD_QUEUE_EVENT_CLOSE: {
                        if (a2dp_ucPCMOpened == 1) {
                            ret = snd_pcm_drain(a2dp_w_snd);
                            ret = snd_pcm_hw_free(a2dp_w_snd);
                            snd_pcm_close(a2dp_w_snd);
                            connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
                            connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
                            connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
                            a2dp_ucPCMOpened = 0;
                        }
                        break;
                    }
                case SBC_CMD_QUEUE_EVENT_RESUME: {
                        if (a2dp_ucPCMOpened == 0) {
                            connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
                            connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
                            connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);

                            ret = snd_pcm_open(&a2dp_w_snd, "track0", 0, 0);
                            ret = snd_pcm_hw_params(a2dp_w_snd, &a2dp_params);
                            ret = snd_pcm_prepare(a2dp_w_snd);

                            sbc_cmd_queue_event_id_t cmd = SBC_CMD_QUEUE_EVENT_PLAY;
                            xQueueSend(sbc_cmd_queue_handle, &cmd, (TickType_t) 0);
                            a2dp_ucPCMOpened = 1;
                        }
                        break;
                    }
                case SBC_CMD_QUEUE_EVENT_SKIP: {
                        sbc_decode_event_send_from_isr(SBC_DECODE_QUEUE_EVENT_DECODE, NULL);
                        break;
                    }
                default:
                    break;
            }
        }
    }
    return;
}

static void sbc_play_task_create(void)
{
    if (pcm_play_task_handle == NULL) {
        //TASK_LOG_MSGID_I("Create sbc play task\r\n");
        if (pdPASS != xTaskCreate(sbc_play_task_main, "SBC_PLAY", 1024, NULL, TASK_PRIORITY_TIMER - 1, &pcm_play_task_handle)) {
            TASK_LOG_MSGID_I("Task create fail\r\n");
        }
    }
}

static void sbc_decode_hisr_handler(void *data)
{
    static uint16_t log_cnt = 0;
    if (log_cnt == 0) {
        TASK_LOG_MSGID_I("[SBC]sbc_decode_hisr_handler\r\n");
    }

    bt_a2dp_audio_internal_handle_t *internal_handle = bt_a2dp_internal_handle;
    bt_media_handle_t *handle = (bt_media_handle_t *)internal_handle;
    linear_buffer_information_t *p_share_buffer = &internal_handle->share_buffer_info;
    linear_buffer_information_t *p_temp_buffer = &internal_handle->temp_buffer_info;
    linear_buffer_16bit_information_t *p_pcm_buffer = &internal_handle->pcm_buffer_info;

    sbc_decoder_status_t ret;
#ifndef A2DP_SBC_UT
    /* get data from BT stack */
    uint32_t wrote_byte_cnt = 0;//how many byte be written by access_share_buffer_function
    wrote_byte_cnt = handle->access_share_buffer_function((p_share_buffer->buffer_base_pointer + p_share_buffer->buffer_data_byte_count), SHARE_BUFFER_LENGTH);
    if ((wrote_byte_cnt == 0) && (p_share_buffer->buffer_data_byte_count != p_share_buffer->buffer_total_length)) {
        TASK_LOG_MSGID_W("[SBC]get NO data from BT stack\r\n", 0);
        //handle->handler(handle, BT_CODEC_MEDIA_REQUEST);
    }
    p_share_buffer->buffer_data_byte_count += wrote_byte_cnt;
#endif /* #ifndef A2DP_SBC_UT */

    /* share buffer initial */
    uint8_t *share_buffer_ptr = p_share_buffer->buffer_base_pointer;
    uint32_t share_buffer_data_size = p_share_buffer->buffer_data_byte_count;
    uint32_t share_buffer_remained_data_size = p_share_buffer->buffer_data_byte_count;//since share_buffer_data_size wiil be replaced after parser,
    //this parameter is used to backup the original value.
    /* temp buffer initial */
    uint8_t *temp_buffer_ptr = p_temp_buffer->buffer_base_pointer;
    uint32_t temp_buffer_data_size = p_temp_buffer->buffer_data_byte_count;
    uint32_t temp_buffer_total_size = p_temp_buffer->buffer_total_length;
    uint32_t temp_buffer_remained_size = temp_buffer_total_size - temp_buffer_data_size;
    uint32_t temp_buffer_remained_size_orig = temp_buffer_remained_size;
    uint32_t temp_buffer_remained_data_size = p_temp_buffer->buffer_data_byte_count;//since temp_buffer_data_size wiil be replaced after SBC decoder,
    //this parameter is used to backup the original value.
    /* pcm buffer initial */
    uint16_t *pcm_buffer_ptr = p_pcm_buffer->buffer_base_pointer;
    uint32_t pcm_buffer_data_size = p_pcm_buffer->buffer_data_byte_count;
    uint32_t pcm_buffer_total_size = p_pcm_buffer->buffer_total_length;
    uint32_t pcm_buffer_remained_size = pcm_buffer_total_size - pcm_buffer_data_size;
    sbc_cmd_queue_event_id_t cmd = SBC_CMD_QUEUE_EVENT_PLAY;
    uint8_t err = 0;

    /* BT stack -> (share buffer) -> parser -> (temp buffer) -> SBC decoder -> (pcm buffer) */
    {
#ifdef A2DP_SBC_UT
        if (temp_buffer_remained_size > 0) {
            if ((temp_ptr_w + temp_buffer_remained_size) > SHARE_BUFFER_LENGTH) {
                memcpy((temp_buffer_ptr + temp_buffer_data_size), (share_buffer_ptr + temp_ptr_w), (SHARE_BUFFER_LENGTH - temp_ptr_w));
                temp_buffer_data_size += (SHARE_BUFFER_LENGTH - temp_ptr_w);
                temp_buffer_remained_size = temp_buffer_total_size - temp_buffer_data_size;
                temp_ptr_w = 0;
                memcpy((temp_buffer_ptr + temp_buffer_data_size), (share_buffer_ptr + temp_ptr_w), temp_buffer_remained_size);
                temp_ptr_w += temp_buffer_remained_size;
                temp_buffer_data_size = TEMP_BUFFER_LENGTH;
                temp_buffer_remained_size = 0;
                temp_buffer_remained_data_size = TEMP_BUFFER_LENGTH;
            } else {
                memcpy((temp_buffer_ptr + temp_buffer_data_size), (share_buffer_ptr + temp_ptr_w), temp_buffer_remained_size);
                temp_ptr_w += temp_buffer_remained_size;
                temp_buffer_data_size = TEMP_BUFFER_LENGTH;
                temp_buffer_remained_size = 0;
                temp_buffer_remained_data_size = TEMP_BUFFER_LENGTH;
            }
        }
#endif /* #ifdef A2DP_SBC_UT */
#ifndef A2DP_SBC_UT
        /* parser */
        ret = sbc_media_payload_parser_process(internal_handle->working_buffer1_handle, \
                                               share_buffer_ptr, \
                                               &share_buffer_data_size, \
                                               (temp_buffer_ptr + temp_buffer_data_size), \
                                               &temp_buffer_remained_size);

        if (ret != SBC_DECODER_STATUS_OK) {
            TASK_LOG_MSGID_E("[SBC]invalid SBC media payload with result for parsing %d (%d %d %d)\r\n",
                             ret, share_buffer_data_size, temp_buffer_data_size, temp_buffer_remained_size);
        }
        if (share_buffer_data_size == 0 && temp_buffer_remained_size == 0) {
            vTaskDelay(30 / portTICK_RATE_MS);
            //TASK_LOG_MSGID_I("[SBC]there is no enough data for parsing\r\n");
            err = 5;
            goto end;
        }
        /* update buffer information */
        share_buffer_remained_data_size -= share_buffer_data_size;
        if (share_buffer_remained_data_size > SHARE_BUFFER_LENGTH) {
            TASK_LOG_MSGID_E("share: size overflow: %d, %d\r\n", share_buffer_remained_data_size, share_buffer_data_size);
            err = 1;
            goto end;
        }
        p_share_buffer->buffer_data_byte_count = share_buffer_remained_data_size;//update share buffer data size
        memmove(share_buffer_ptr, \
                (share_buffer_ptr + share_buffer_data_size), \
                share_buffer_remained_data_size);//move remained data to share buffer base addr.
        temp_buffer_data_size += temp_buffer_remained_size;
        temp_buffer_remained_data_size += temp_buffer_remained_size;
        if (temp_buffer_remained_size > TEMP_BUFFER_LENGTH ||
            temp_buffer_data_size > TEMP_BUFFER_LENGTH ||
            temp_buffer_remained_data_size > TEMP_BUFFER_LENGTH) {
            TASK_LOG_MSGID_E("tmp: size overflow1: %d, %d, %d, %d\r\n",
                             temp_buffer_remained_size_orig, temp_buffer_remained_size, temp_buffer_data_size, temp_buffer_remained_data_size);
            err = 2;
            goto end;
        }
#endif /* #ifndef A2DP_SBC_UT */
        /*SBC decoder */
        ret = sbc_decoder_process(internal_handle->working_buffer2_handle, \
                                  temp_buffer_ptr, \
                                  &temp_buffer_data_size, \
                                  (int16_t *)(pcm_buffer_ptr + pcm_buffer_data_size), \
                                  &pcm_buffer_remained_size);

        if (ret != SBC_DECODER_STATUS_OK) {
            uint32_t pos = temp_buffer_data_size;
            TASK_LOG_MSGID_E("[SBC]invalid SBC media payload with result for decoding, drop!! %d %d %d %d %d(%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x)\r\n",
                             ret, pos, temp_buffer_data_size, pcm_buffer_data_size, pcm_buffer_remained_size,
                             temp_buffer_ptr[pos + 0], temp_buffer_ptr[pos + 1], temp_buffer_ptr[pos + 2], temp_buffer_ptr[pos + 3],
                             temp_buffer_ptr[pos + 4], temp_buffer_ptr[pos + 5], temp_buffer_ptr[pos + 6], temp_buffer_ptr[pos + 7],
                             temp_buffer_ptr[pos + 8], temp_buffer_ptr[pos + 9], temp_buffer_ptr[pos + 10], temp_buffer_ptr[pos + 11],
                             temp_buffer_ptr[pos + 12], temp_buffer_ptr[pos + 13], temp_buffer_ptr[pos + 14], temp_buffer_ptr[pos + 15]);
            err = 3;
            goto end;
        }
        if (temp_buffer_data_size == 0 && pcm_buffer_remained_size == 0) {
            //TASK_LOG_MSGID_I("[SBC]there is no enough data for decoding\r\n");
        }

#ifdef MTK_BT_AUDIO_PR
        app_bt_dbg_audio_pr_write((uint8_t *)(pcm_buffer_ptr + pcm_buffer_data_size), 0, APP_BT_DBG_AUDIO_DECODE_PATH_DECODE_AFTER);
#endif /* #ifdef MTK_BT_AUDIO_PR */

#ifdef LATENCY_CAL
        bt_codec_silence_data_check_callback((int16_t *)(pcm_buffer_ptr + pcm_buffer_data_size), pcm_buffer_remained_size);
#endif /* #ifdef LATENCY_CAL */
        temp_buffer_remained_data_size -= temp_buffer_data_size;
        if (temp_buffer_remained_data_size > TEMP_BUFFER_LENGTH) {
            TASK_LOG_MSGID_E("tmp: size overflow2: %d, %d, %d, %d\r\n", temp_buffer_remained_size, temp_buffer_remained_data_size, temp_buffer_data_size, ret);
            err = 4;
            goto end;
        }
        p_temp_buffer->buffer_data_byte_count = temp_buffer_remained_data_size;//update temp buffer data size
        memmove(temp_buffer_ptr, \
                (temp_buffer_ptr + temp_buffer_data_size), \
                temp_buffer_remained_data_size);//move remained data to temp buffer base addr.

        pcm_buffer_data_size += pcm_buffer_remained_size;
        p_pcm_buffer->buffer_data_byte_count = pcm_buffer_data_size;//update pcm buffer data size
        if (log_cnt == 0) {
            TASK_LOG_MSGID_I("%d, %d\r\n", pcm_buffer_remained_size, pcm_buffer_data_size);
        }
        if (log_cnt++ == MAX_LOG_CNT) {
            log_cnt = 0;
        }
    }

end:
    if (err != 0) {
        p_share_buffer->buffer_data_byte_count = 0;
        p_temp_buffer->buffer_data_byte_count = 0;
        p_pcm_buffer->buffer_data_byte_count = 0;
        cmd = SBC_CMD_QUEUE_EVENT_SKIP;
    }
    bt_audio_mutex_lock(g_xSemaphore_Audio);
    if (sbc_cmd_queue_handle) {
        xQueueSend(sbc_cmd_queue_handle, &cmd, (TickType_t) 0);
    }
    bt_audio_mutex_unlock(g_xSemaphore_Audio);
}

static int8_t sbc_buffer_alloc(void)
{
    //TASK_LOG_MSGID_I("[SBC]allocate memeory for sbc decoder\r\n");

    bt_a2dp_audio_internal_handle_t *internal_handle = bt_a2dp_internal_handle;
    linear_buffer_information_t *p_share_buffer = &internal_handle->share_buffer_info;
    linear_buffer_information_t *p_temp_buffer = &internal_handle->temp_buffer_info;
    linear_buffer_16bit_information_t *p_pcm_buffer = &internal_handle->pcm_buffer_info;

    /* alloc working buffer1 */
    sbc_media_payload_parser_get_buffer_size(&internal_handle->working_buffer1_size);
    internal_handle->working_buffer1 = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * internal_handle->working_buffer1_size);
    if (internal_handle->working_buffer1 == NULL) {
        return -1;
    }
    internal_handle->working_buffer1_handle = NULL;
    /* alloc working buffer2 */
    sbc_decoder_get_buffer_size(&internal_handle->working_buffer2_size);
    internal_handle->working_buffer2 = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * internal_handle->working_buffer2_size);
    if (internal_handle->working_buffer2 == NULL) {
        vPortFree(internal_handle->working_buffer1);
        return -1;
    }
    internal_handle->working_buffer2_handle = NULL;
    /* alloc share buffer */
    p_share_buffer->buffer_base_pointer = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * SHARE_BUFFER_LENGTH);
    if (p_share_buffer->buffer_base_pointer == NULL) {
        vPortFree(internal_handle->working_buffer1);
        vPortFree(internal_handle->working_buffer2);
        return -1;
    }
    p_share_buffer->buffer_data_byte_count = 0;
    p_share_buffer->buffer_total_length = SHARE_BUFFER_LENGTH;
    //TASK_LOG_MSGID_I("ptr = 0x%x, total = %x, byte_count = %d\r\n",
    //                 p_share_buffer->buffer_base_pointer, p_share_buffer->buffer_total_length, p_share_buffer->buffer_data_byte_count);
    /* alloc temp buffer */
    p_temp_buffer->buffer_base_pointer = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * TEMP_BUFFER_LENGTH);
    if (p_temp_buffer->buffer_base_pointer == NULL) {
        vPortFree(internal_handle->working_buffer1);
        vPortFree(internal_handle->working_buffer2);
        vPortFree(p_share_buffer->buffer_base_pointer);
        return -1;
    }
    p_temp_buffer->buffer_data_byte_count = 0;
    p_temp_buffer->buffer_total_length = TEMP_BUFFER_LENGTH;
    /* alloc pcm buffer */
    p_pcm_buffer->buffer_base_pointer = (uint16_t *)pvPortMalloc(sizeof(uint16_t) * PCM_BUFFER_LENGTH);
    if (p_pcm_buffer->buffer_base_pointer == NULL) {
        vPortFree(internal_handle->working_buffer1);
        vPortFree(internal_handle->working_buffer2);
        vPortFree(p_share_buffer->buffer_base_pointer);
        vPortFree(p_temp_buffer->buffer_base_pointer);
        return -1;
    }
    p_pcm_buffer->buffer_data_byte_count = 0;
    p_pcm_buffer->buffer_total_length = PCM_BUFFER_LENGTH;
    return 1;
}

// static void sbc_buffer_clear(void)
// {
// LOGI("[SBC]buffer clear\r\n");

// bt_a2dp_audio_internal_handle_t *internal_handle = bt_a2dp_internal_handle;
// linear_buffer_information_t *p_share_buffer = &internal_handle->share_buffer_info;
// linear_buffer_information_t *p_temp_buffer = &internal_handle->temp_buffer_info;
// linear_buffer_16bit_information_t *p_pcm_buffer = &internal_handle->pcm_buffer_info;

// /* clear share buffer */
// memset(p_share_buffer, 0, SHARE_BUFFER_LENGTH);
// p_share_buffer->buffer_data_byte_count = 0;
// p_share_buffer->buffer_total_length = SHARE_BUFFER_LENGTH;
// /* clear temp buffer*/
// memset(p_temp_buffer, 0, TEMP_BUFFER_LENGTH);
// p_temp_buffer->buffer_data_byte_count = 0;
// p_temp_buffer->buffer_total_length = TEMP_BUFFER_LENGTH;
// /* clear pcm buffer */
// memset(p_pcm_buffer, 0, PCM_BUFFER_LENGTH);
// p_pcm_buffer->buffer_data_byte_count = 0;
// p_pcm_buffer->buffer_total_length = PCM_BUFFER_LENGTH;
// }

static void sbc_decode_task_main(void *arg)
{
    TASK_LOG_MSGID_I("[SBC]sbc_decode_task_main\r\n");

    sbc_decode_queue_event_t event;

    while (1) {
        if (xQueueReceive(sbc_decode_queue_handle, &event, portMAX_DELAY)) {
            sbc_decode_queue_event_id_t rece_id = event.id;
            uint32_t id_idx;
            for (id_idx = 0; id_idx < MAX_SBC_DECODE_FUNCTIONS; id_idx++) {
                if (sbc_decode_queue_id_array[id_idx] == rece_id) {
                    // LOGI("[SBC]xQueueReceive find event_id sbc_decode_queue_id_array[%d] = %d\r\n", id_idx, event.id);
                    sbc_decode_queue_handler[id_idx](event.parameter);
                    break;
                }
            }
        }
    }
}

static void sbc_decode_task_create(void)
{
    if (sbc_decode_task_handle == NULL) {
        //TASK_LOG_MSGID_I("Create sbc decode task\r\n");
        if (pdPASS != xTaskCreate(sbc_decode_task_main, SBC_DECODE_TASK_NAME, SBC_DECODE_TASK_STACKSIZE / sizeof(StackType_t), NULL, SBC_DECODE_TASK_PRIO, &sbc_decode_task_handle)) {
            TASK_LOG_MSGID_I("Task create fail\r\n");
        }
    }
}

static void bt_set_buffer(bt_media_handle_t *handle, uint8_t *buffer, uint32_t length)
{
    handle->buffer_info.buffer_base = buffer;
    length &= ~0x1; // make buffer size even
    handle->buffer_info.buffer_size = length;
    handle->buffer_info.write = 0;
    handle->buffer_info.read = 0;
    handle->waiting = false;
    handle->underflow = false;
}

static void bt_set_get_data_function(bt_media_handle_t *handle, bt_codec_get_data func)
{
    handle->directly_access_dsp_function = func;
}

static void bt_set_get_data_count_function(bt_media_handle_t *handle, bt_codec_get_data_count func)
{
    handle->get_data_count_function = func;
}

static void bt_set_get_data_in_byte_function(bt_media_handle_t *handle, bt_codec_get_data_in_byte func)
{
    handle->access_share_buffer_function = func;
}

static void bt_get_write_buffer(bt_media_handle_t *handle, uint8_t **buffer, uint32_t *length)
{
    int32_t count = 0;

    if (handle->buffer_info.read > handle->buffer_info.write) {
        count = handle->buffer_info.read - handle->buffer_info.write - 1;
    } else if (handle->buffer_info.read == 0) {
        count = handle->buffer_info.buffer_size - handle->buffer_info.write - 1;
    } else {
        count = handle->buffer_info.buffer_size - handle->buffer_info.write;
    }
    *buffer = handle->buffer_info.buffer_base + handle->buffer_info.write;
    *length = count;
}

static void bt_get_read_buffer(bt_media_handle_t *handle, uint8_t **buffer, uint32_t *length)
{
    int32_t count = 0;

    if (handle->buffer_info.write >= handle->buffer_info.read) {
        count = handle->buffer_info.write - handle->buffer_info.read;
    } else {
        count = handle->buffer_info.buffer_size - handle->buffer_info.read;
    }
    *buffer = handle->buffer_info.buffer_base + handle->buffer_info.read;
    *length = count;
}

static void bt_write_data_done(bt_media_handle_t *handle, uint32_t length)
{
    handle->buffer_info.write += length;
#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
    TASK_LOG_MSGID_I("[A2DP]write--wr: %d, len: %d\n", handle->buffer_info.write, length);
#endif /* #ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG */
    if (handle->buffer_info.write == handle->buffer_info.buffer_size) {
        handle->buffer_info.write = 0;
    }
}

static void bt_finish_write_data(bt_media_handle_t *handle)
{
    handle->waiting = false;
    handle->underflow = false;
}

static void bt_reset_share_buffer(bt_media_handle_t *handle)
{
    handle->buffer_info.write = 0;
    handle->buffer_info.read = 0;
    handle->waiting = false;
    handle->underflow = false;
}

static void bt_read_data_done(bt_media_handle_t *handle, uint32_t length)
{
    handle->buffer_info.read += length;
#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
    TASK_LOG_MSGID_I("[A2DP]read--rd: %d, len: %d\n", handle->buffer_info.read, length);
#endif /* #ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG */
    if (handle->buffer_info.read == handle->buffer_info.buffer_size) {
        handle->buffer_info.read = 0;
    }
}

static int32_t bt_get_free_space(bt_media_handle_t *handle)
{
    int32_t count = 0;

    count = handle->buffer_info.read - handle->buffer_info.write - 1;
    if (count < 0) {
        count += handle->buffer_info.buffer_size;
    }
    return count;
}

static int32_t bt_get_data_count(bt_media_handle_t *handle)
{
    int32_t count = 0;

    count = handle->buffer_info.write - handle->buffer_info.read;
    if (count < 0) {
        count += handle->buffer_info.buffer_size;
    }
    return count;
}

static void bt_set_aws_flag(bt_media_handle_t *handle, bool is_aws)
{
#if defined(MTK_AVM_DIRECT)
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)handle;

    internal_handle->is_aws = is_aws;
#endif /* #if defined(MTK_AVM_DIRECT) */
}

#if defined(MTK_AVM_DIRECT)
void Bt_c2d_buffercopy(void *DestBuf,
                       void *SrcBuf,
                       U16   CopySize,
                       void *SrcCBufStart,
                       U16   SrcCBufSize)
{
    U8 *SrcCBufEnd      = (U8 *)((U8 *)SrcCBufStart + SrcCBufSize);
    U16 UnwrapSize      = (U8 *)SrcCBufEnd - (U8 *)SrcBuf; //Remove + 1 to sync more common usage

    if (CopySize > UnwrapSize) {
        memcpy(DestBuf, SrcBuf, UnwrapSize);
        memcpy((U8 *)DestBuf + UnwrapSize, SrcCBufStart, CopySize - UnwrapSize);
    } else {
        memcpy(DestBuf, SrcBuf, CopySize);
    }
}

static uint8_t bt_sbc_get_frame_num(uint32_t ts0, bt_media_handle_t *handle, uint8_t *sample_num)
{
    uint16_t TempReadOffset = 0;
    uint32_t TempTimeStamp  = 0;
    uint16_t TempPacketSize = 0;
    uint8_t  TempFrameNum   = 0;
    uint8_t  TempBlockLen   = 0;
    uint8_t  TempSubBand    = 0;
    uint8_t  sbc_frame_info_btye2;

    uint16_t buffer_size;
    uint8_t *p_buffer_base;
    n9_dsp_share_info_t *p_share_info = (n9_dsp_share_info_t *)hal_audio_query_share_info(AUDIO_MESSAGE_TYPE_BT_AUDIO_DL);

    if (!p_share_info) {
        TASK_LOG_MSGID_I("[A2DP Codec]p_share_info null");
        return 0;
    }

    buffer_size = p_share_info->length;
    p_buffer_base = (uint8_t *)p_share_info->start_addr;

    do {
        TempReadOffset = (TempReadOffset + TempPacketSize) % (buffer_size);
        Bt_c2d_buffercopy((void *)      &TempPacketSize,
                          (void *)(p_buffer_base + (TempReadOffset + 2) % (buffer_size)),    //PCB header: 4 bytes: U8 state, U8 data size, U16 size
                          (uint16_t)   2,
                          (void *)      p_buffer_base,
                          (uint16_t)   buffer_size);
        TempPacketSize += 4; //include PCB header length
        Bt_c2d_buffercopy((void *)      &TempTimeStamp,
                          (void *)(p_buffer_base + (TempReadOffset + 8) % (buffer_size)),    //RTP header (after PCB): 12 bytes: U8 version, U8 type, U16 sequence no., U32 time stamp, U32 ssrc
                          (uint16_t)   4,
                          (void *)      p_buffer_base,
                          (uint16_t)   buffer_size);
    } while (TempTimeStamp != ts0);

    if (((bt_a2dp_audio_internal_handle_t *)handle)->is_cp == TRUE)
        TempReadOffset++;
    TempFrameNum = *(p_buffer_base + (TempReadOffset + 16) % (buffer_size));
    TempFrameNum &= 0x0F; //0~3 bit

    sbc_frame_info_btye2 = *(p_buffer_base + (TempReadOffset + 17) % (buffer_size));
    if (sbc_frame_info_btye2 == 0x9C) {
        sbc_frame_info_btye2 = *(p_buffer_base + (TempReadOffset + 18) % (buffer_size)); //subbands:1bit  allocation_method:1bit  channel_mode:2bits  blocks:2bits samplerate:2bits
        TempSubBand = ((sbc_frame_info_btye2 & 0x1) == 0) ? (uint8_t)4 : (uint8_t)8;
        TempBlockLen = (((sbc_frame_info_btye2 >> 4) & 0x3) + 1) * (uint8_t)4;
        *sample_num = (TempSubBand * TempBlockLen);
        TASK_LOG_MSGID_I("[SBC] sbc_frame_info_btye2: 0x%02x SubBand: %d BlockLen: %d FrameNum: %d", sbc_frame_info_btye2, TempSubBand, TempBlockLen, TempFrameNum);
    } else {
        TASK_LOG_MSGID_E("[SBC] sync word: 0x%02x ", 1, sbc_frame_info_btye2);
    }

    return TempFrameNum;
}

#ifndef MTK_PORTING_AB
static uint8_t bt_get_next_pkt_info(uint32_t timestamp, uint32_t *next_timestamp)
{
    uint16_t buffer_size;
    uint8_t *p_buffer_base;
    n9_dsp_share_info_t *p_share_info = (n9_dsp_share_info_t *)hal_audio_query_share_info(AUDIO_MESSAGE_TYPE_BT_AUDIO_DL);
    uint16_t TempReadOffset = 0;
    uint32_t TempTimeStamp  = 0;
    uint16_t TempPacketSize = 0;

    if (!p_share_info) {
        TASK_LOG_MSGID_I("[A2DP Codec]p_share_info null");
        return 0;
    }

    buffer_size = p_share_info->length;
    p_buffer_base = (uint8_t *)p_share_info->start_addr;

    do {
        TempReadOffset = (TempReadOffset + TempPacketSize) % (buffer_size);
        Bt_c2d_buffercopy((void *)      &TempPacketSize,
                          (void *)(p_buffer_base + (TempReadOffset + 2) % (buffer_size)),    //PCB header: 4 bytes: U8 state, U8 data size, U16 size
                          (uint16_t)   2,
                          (void *)      p_buffer_base,
                          (uint16_t)   buffer_size);
        TempPacketSize += 4; //include PCB header length
        Bt_c2d_buffercopy((void *)      &TempTimeStamp,
                          (void *)(p_buffer_base + (TempReadOffset + 8) % (buffer_size)),    //RTP header (after PCB): 12 bytes: U8 version, U8 type, U16 sequence no., U32 time stamp, U32 ssrc
                          (uint16_t)   4,
                          (void *)      p_buffer_base,
                          (uint16_t)   buffer_size);
    } while (TempTimeStamp != timestamp);

    TempReadOffset = (TempReadOffset + TempPacketSize) % (buffer_size);
    if (TempReadOffset != p_share_info->write_offset) {
        Bt_c2d_buffercopy((void *)      next_timestamp,
                          (void *)(p_buffer_base + (TempReadOffset + 8) % (buffer_size)),
                          (uint16_t)   4,
                          (void *)      p_buffer_base,
                          (uint16_t)   buffer_size);
        return TRUE;
    }
    return FALSE;
}
#endif

#endif /* #if defined(MTK_AVM_DIRECT) */

static uint32_t bt_get_ts_ratio(bt_media_handle_t *handle, uint32_t ts0, uint32_t ts1)
{
#if defined(MTK_AVM_DIRECT)
    U8  frame_num;
    U8  sample_num = SBC_DEC_FRAME_LEN;
    U32 ts_ratio;

    frame_num = bt_sbc_get_frame_num(ts0, handle, &sample_num);
    ts_ratio = (ts1 - ts0) / (frame_num * sample_num);

    if (((ts_ratio * frame_num * sample_num) != (ts1 - ts0))
        || (ts0 == ts1)) {
        ts_ratio = frame_num * sample_num;
    }

    TASK_LOG_MSGID_I("[A2DP Codec]ts1:%d, ts0:%d, frame_num:%d, sample_num:%d, ret_ts_ratio:%d",
                     ts1, ts0, frame_num, sample_num, ts_ratio);

    return ts_ratio;
#else /* #if defined(MTK_AVM_DIRECT) */
    return 1;
#endif /* #if defined(MTK_AVM_DIRECT) */
}

static void bt_set_ts_ratio(bt_media_handle_t *handle, uint32_t ts_ratio)
{
#if defined(MTK_AVM_DIRECT)
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)handle;

    internal_handle->ts_ratio = ts_ratio;
#endif /* #if defined(MTK_AVM_DIRECT) */
}

static uint32_t bt_get_sampling_rate(bt_media_handle_t *handle)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)handle;

    uint32_t ret;
    uint32_t sample_rate_idx = internal_handle->sample_rate;

    switch (sample_rate_idx) {
        case HAL_AUDIO_SAMPLING_RATE_8KHZ:
            ret = 8000;
            break;
        case HAL_AUDIO_SAMPLING_RATE_11_025KHZ:
            ret = 11025;
            break;
        case HAL_AUDIO_SAMPLING_RATE_12KHZ:
            ret = 12000;
            break;
        case HAL_AUDIO_SAMPLING_RATE_16KHZ:
            ret = 16000;
            break;
        case HAL_AUDIO_SAMPLING_RATE_22_05KHZ:
            ret = 22050;
            break;
        case HAL_AUDIO_SAMPLING_RATE_24KHZ:
            ret = 24000;
            break;
        case HAL_AUDIO_SAMPLING_RATE_32KHZ:
            ret = 32000;
            break;
        case HAL_AUDIO_SAMPLING_RATE_44_1KHZ:
            ret = 44100;
            break;
        case HAL_AUDIO_SAMPLING_RATE_48KHZ:
            ret = 48000;
            break;
        case HAL_AUDIO_SAMPLING_RATE_88_2KHZ:
            ret = 88200;
            break;
        case HAL_AUDIO_SAMPLING_RATE_96KHZ:
            ret = 96000;
            break;
        case HAL_AUDIO_SAMPLING_RATE_176_4KHZ:
            ret = 176400;
            break;
        case HAL_AUDIO_SAMPLING_RATE_192KHZ:
            ret = 192000;
            break;
        default:
            ret = 48000;
    }

    return ret;
}

static void bt_set_start_time_stamp(bt_media_handle_t *handle, uint32_t time_stamp)
{
#if defined(MTK_AVM_DIRECT)
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)handle;

    internal_handle->time_stamp = time_stamp;
#endif /* #if defined(MTK_AVM_DIRECT) */
}

static void bt_set_special_device(bt_media_handle_t *handle, bool is_special)
{
#if defined(MTK_AVM_DIRECT)
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)handle;

    internal_handle->is_lm_en = is_special;
#endif /* #if defined(MTK_AVM_DIRECT) */
}


static void bt_set_content_protection(bt_media_handle_t *handle, bool is_cp)
{
#if defined(MTK_AVM_DIRECT)
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)handle;

    internal_handle->is_cp = is_cp;
#endif /* #if defined(MTK_AVM_DIRECT) */
}

static uint32_t bt_get_max_frame_decoding_time(bt_media_handle_t *handle)                                    /**< Get the max frame decoding time (unit is 0.1 ms)  */
{
    return 232; //For 1024 sample in 44.1kHz
}

static void bt_codec_buffer_function_init(bt_media_handle_t *handle)
{
    handle->set_buffer         = bt_set_buffer;
    handle->set_get_data_function = bt_set_get_data_function;
    handle->set_get_data_count_function = bt_set_get_data_count_function;
    handle->set_get_data_in_byte_function = bt_set_get_data_in_byte_function;
    handle->get_write_buffer   = bt_get_write_buffer;
    handle->get_read_buffer    = bt_get_read_buffer;
    handle->write_data_done    = bt_write_data_done;
    handle->finish_write_data  = bt_finish_write_data;
    handle->reset_share_buffer = bt_reset_share_buffer;
    handle->read_data_done     = bt_read_data_done;
    handle->get_free_space     = bt_get_free_space;
    handle->get_data_count     = bt_get_data_count;
    handle->set_aws_flag                = bt_set_aws_flag;
    handle->get_ts_ratio                = bt_get_ts_ratio;
    handle->set_ts_ratio                = bt_set_ts_ratio;
    handle->get_sampling_rate           = bt_get_sampling_rate;
    handle->set_start_time_stamp        = bt_set_start_time_stamp;
    handle->set_special_devicce         = bt_set_special_device;
    handle->set_content_protection      = bt_set_content_protection;
    handle->get_max_frame_decoding_time = bt_get_max_frame_decoding_time;
}

static void bt_side_tone_enable(bt_media_handle_t *handle)
{
#ifndef MTK_PORTING_AB
    mcu2dsp_sidetone_param_t sidetone;
    void *p_param_share;

    if (!handle) {
        TASK_LOG_MSGID_I("[A2DP Codec]SideTone Enable No Handle\n");
        return;
    }

    TASK_LOG_MSGID_I("[A2DP Codec]SideTone Enable\n");

    sidetone.in_device                       = HAL_AUDIO_DEVICE_MAIN_MIC_DUAL;
    sidetone.in_interface                    = HAL_AUDIO_INTERFACE_1;
    sidetone.in_misc_parms                   = MICBIAS_SOURCE_ALL | MICBIAS3V_OUTVOLTAGE_1p85v;
    sidetone.out_device                      = HAL_AUDIO_DEVICE_DAC_DUAL;
    sidetone.out_interface                   = HAL_AUDIO_INTERFACE_NONE;
    sidetone.out_misc_parms                  = DOWNLINK_PERFORMANCE_NORMAL;
    sidetone.in_channel                      = HAL_AUDIO_DIRECT;
    sidetone.gain                            = 600;
    sidetone.sample_rate                     = 16000;

    p_param_share = hal_audio_dsp_controller_put_paramter(&sidetone, sizeof(mcu2dsp_sidetone_param_t), AUDIO_MESSAGE_TYPE_SIDETONE);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SIDETONE_START, 0, (uint32_t)p_param_share, true);
#endif /* #ifndef MTK_PORTING_AB */
}

static void bt_side_tone_disable(bt_media_handle_t *handle)
{
#ifndef MTK_PORTING_AB
    mcu2dsp_sidetone_param_t sidetone;
    void *p_param_share;

    if (!handle) {
        TASK_LOG_MSGID_I("[A2DP Codec]SideTone Disable No Handle\n");
        return;
    }

    TASK_LOG_MSGID_I("[A2DP Codec]SideTone Disable\n");

    sidetone.in_device                       = HAL_AUDIO_DEVICE_MAIN_MIC_DUAL;
    sidetone.in_interface                    = HAL_AUDIO_INTERFACE_1;
    sidetone.in_misc_parms                   = MICBIAS_SOURCE_ALL | MICBIAS3V_OUTVOLTAGE_1p85v;
    sidetone.out_device                      = HAL_AUDIO_DEVICE_DAC_DUAL;
    sidetone.out_interface                   = HAL_AUDIO_INTERFACE_NONE;
    sidetone.out_misc_parms                  = DOWNLINK_PERFORMANCE_NORMAL;
    sidetone.in_channel                      = HAL_AUDIO_DIRECT;
    sidetone.gain                            = 0;
    sidetone.sample_rate                     = 16000;

    p_param_share = hal_audio_dsp_controller_put_paramter(&sidetone, sizeof(mcu2dsp_sidetone_param_t), AUDIO_MESSAGE_TYPE_SIDETONE);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SIDETONE_STOP, 0, (uint32_t)p_param_share, true);
#endif /* #ifndef MTK_PORTING_AB */
}

#if defined(BT_A2DP_BITSTREAM_DUMP_DEBUG)
#define BT_A2DP_BS_LEN 160000
uint32_t bt_a2dp_ptr = 0;
uint8_t bt_a2dp_bitstream[BT_A2DP_BS_LEN];
#endif /* #if defined(BT_A2DP_BITSTREAM_DUMP_DEBUG) */

#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT)
static uint32_t bt_codec_a2dp_aws_convert_sampling_rate_from_index_to_value(uint32_t sampling_rate_index)
{
    uint32_t sampling_rate_value;
    if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_8KHZ) {
        sampling_rate_value = 8000;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_11_025KHZ) {
        sampling_rate_value = 11025;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_12KHZ) {
        sampling_rate_value = 12000;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_16KHZ) {
        sampling_rate_value = 16000;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_22_05KHZ) {
        sampling_rate_value = 22050;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_24KHZ) {
        sampling_rate_value = 24000;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_32KHZ) {
        sampling_rate_value = 32000;
    } else if (sampling_rate_index == HAL_AUDIO_SAMPLING_RATE_44_1KHZ) {
        sampling_rate_value = 44100;
    } else {
        sampling_rate_value = 48000;
    }
    return sampling_rate_value;
}



static void bt_codec_a2dp_aws_callback(aws_event_t event, void *user_data)
{
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)user_data;
    bt_media_handle_t *handle = (bt_media_handle_t *)p_info;
    switch (event) {
        case CODEC_AWS_CHECK_CLOCK_SKEW:
            handle->handler(handle, BT_CODEC_MEDIA_AWS_CHECK_CLOCK_SKEW);
            break;
        case CODEC_AWS_CHECK_UNDERFLOW:
            handle->handler(handle, BT_CODEC_MEDIA_AWS_CHECK_UNDERFLOW);
            break;
        default:
            break;
    }
    return;
}

static int32_t bt_codec_a2dp_aws_open_setting(bt_a2dp_audio_internal_handle_t *p_info)
{
    int32_t result = 0;
    bt_codec_a2dp_audio_t *p_codec = &p_info->codec_info;
    uint8_t *p_aws_buf = NULL;
    int32_t aws_buf_size = 0;
    p_info->aws_flag = false;
    p_info->aws_internal_flag = false;
    p_info->aws_init_sync_flag = false;
    if (p_codec->role == BT_A2DP_SINK) {
        bt_codec_capability_t *p_cap = &p_codec->codec_cap;
        bt_a2dp_codec_type_t type = p_cap->type;
        if (type == BT_A2DP_CODEC_SBC) {
            aws_buf_size = audio_service_aws_get_buffer_size(AWS_CODEC_TYPE_SBC_FORMAT);
            if (aws_buf_size < 0) {
                TASK_LOG_MSGID_E("[AWS] Fail: Buffer size is negative\n", 0);
                return -1;
            }
            p_info->aws_working_buf_size = aws_buf_size;
            p_aws_buf = (uint8_t *)pvPortMalloc(aws_buf_size);
            if (p_aws_buf == NULL) {
                TASK_LOG_MSGID_E("[AWS] Fail: Aws buf allocation failed\n", 0);
                return -2;
            }
            p_info->aws_working_buffer = p_aws_buf;
            audio_service_aws_init(p_aws_buf, AWS_CODEC_TYPE_SBC_FORMAT, (aws_callback_t)bt_codec_a2dp_aws_callback, (void *)p_info);
        }
#if defined(MTK_BT_A2DP_AAC_ENABLE)
        else if (type == BT_A2DP_CODEC_AAC) {
            aws_buf_size = audio_service_aws_get_buffer_size(AWS_CODEC_TYPE_AAC_FORMAT);
            if (aws_buf_size < 0) {
                TASK_LOG_MSGID_E("[AWS] Fail: Buffer size is negative\n", 0);
                return -1;
            }
            p_info->aws_working_buf_size = aws_buf_size;
            p_aws_buf = (uint8_t *)pvPortMalloc(aws_buf_size);
            if (p_aws_buf == NULL) {
                TASK_LOG_MSGID_E("[AWS] Fail: Aws buf allocation failed\n", 0);
                return -2;
            }
            p_info->aws_working_buffer = p_aws_buf;
            audio_service_aws_init(p_aws_buf, AWS_CODEC_TYPE_AAC_FORMAT, (aws_callback_t)bt_codec_a2dp_aws_callback, (void *)p_info);
        }
#endif /* #if defined(MTK_BT_A2DP_AAC_ENABLE) */
        else {  /* Invalid codec type */
            result = -1;
        }
    }
    return result;
}

static void bt_codec_a2dp_aws_close_setting(bt_a2dp_audio_internal_handle_t *p_info)
{
    uint8_t *p_aws_buf = p_info->aws_working_buffer;
    audio_service_aws_deinit();
    if (p_aws_buf != NULL) {
        vPortFree(p_aws_buf);
        p_info->aws_working_buffer = (uint8_t *)NULL;
    }
    return;
}

static void bt_codec_a2dp_aws_play_setting(bt_a2dp_audio_internal_handle_t *p_info)
{
    p_info->aws_internal_flag = true;
    audio_service_aws_set_flag(true);
    return;
}

static void bt_codec_a2dp_aws_stop_setting(bt_a2dp_audio_internal_handle_t *p_info)
{
    audio_service_aws_set_flag(false);
    p_info->aws_internal_flag = false;
    p_info->aws_init_sync_flag = false;
    return;
}
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT) */

static bt_codec_media_status_t bt_open_sink_sbc_codec(bt_media_handle_t *handle)
{
    TASK_LOG_MSGID_I("[SBC]open_codec\r\n");

    bt_a2dp_audio_internal_handle_t *internal_handle = bt_a2dp_internal_handle;
    linear_buffer_information_t *p_share_buffer = &internal_handle->share_buffer_info;
    linear_buffer_information_t *p_temp_buffer = &internal_handle->temp_buffer_info;
    linear_buffer_16bit_information_t *p_pcm_buffer = &internal_handle->pcm_buffer_info;

    sbc_decoder_status_t ret;

    sbc_decode_event_register_callback(SBC_DECODE_QUEUE_EVENT_DECODE, sbc_decode_hisr_handler);

    /* get data from BT stack */
    uint32_t wrote_byte_cnt = 0;//how many byte be written by access_share_buffer_function
    wrote_byte_cnt = handle->access_share_buffer_function(p_share_buffer->buffer_base_pointer, p_share_buffer->buffer_total_length);
    if ((wrote_byte_cnt == 0) && (p_share_buffer->buffer_data_byte_count != p_share_buffer->buffer_total_length)) {
        TASK_LOG_MSGID_I("[SBC]get NO data from BT stack\r\n");
    }

    //TASK_LOG_MSGID_I("%d, %d, %d\r\n",
    //                wrote_byte_cnt, p_share_buffer->buffer_data_byte_count, p_share_buffer->buffer_total_length);

    p_share_buffer->buffer_data_byte_count = wrote_byte_cnt;
    //for (uint32_t i = 0; i < 16; i++) {
    //    TASK_LOG_MSGID_I("sbc[%d] = 0x%x\r\n", i, *(p_share_buffer->buffer_base_pointer + i));
    //}
    /* share buffer initial */
    uint8_t *share_buffer_ptr = p_share_buffer->buffer_base_pointer;
    uint32_t share_buffer_data_size = p_share_buffer->buffer_data_byte_count;
    uint32_t share_buffer_remained_data_size = p_share_buffer->buffer_data_byte_count;//since share_buffer_data_size wiil be replaced after parser,
    //this parameter is used to backup the original value.
    /* temp buffer initial */
    uint8_t *temp_buffer_ptr = p_temp_buffer->buffer_base_pointer;
    uint32_t temp_buffer_data_size = p_temp_buffer->buffer_data_byte_count;
    uint32_t temp_buffer_total_size = p_temp_buffer->buffer_total_length;
    uint32_t temp_buffer_remained_size = temp_buffer_total_size - temp_buffer_data_size;
    uint32_t temp_buffer_remained_data_size = p_temp_buffer->buffer_data_byte_count;//since temp_buffer_data_size wiil be replaced after SBC decoder,
    //this parameter is used to backup the original value.
    /* pcm buffer initial */
    uint16_t *pcm_buffer_ptr = p_pcm_buffer->buffer_base_pointer;
    uint32_t pcm_buffer_data_size = p_pcm_buffer->buffer_data_byte_count;
    uint32_t pcm_buffer_total_size = p_pcm_buffer->buffer_total_length;
    uint32_t pcm_buffer_remained_size = pcm_buffer_total_size - pcm_buffer_data_size;

    /* BT stack -> (share buffer) -> parser -> (temp buffer) -> SBC decoder -> (pcm buffer) -> I2S -> nau8810 */
    {
#ifdef A2DP_SBC_UT
        memcpy(temp_buffer_ptr, share_buffer_ptr, TEMP_BUFFER_LENGTH);
        temp_buffer_data_size = TEMP_BUFFER_LENGTH;
        temp_buffer_remained_size = 0;
        temp_buffer_remained_data_size = TEMP_BUFFER_LENGTH;
#endif /* #ifdef A2DP_SBC_UT */
#ifndef A2DP_SBC_UT
        /* parser */
        ret = sbc_media_payload_parser_process(internal_handle->working_buffer1_handle, \
                                               share_buffer_ptr, \
                                               &share_buffer_data_size, \
                                               (temp_buffer_ptr + temp_buffer_data_size), \
                                               &temp_buffer_remained_size);
        //TASK_LOG_MSGID_I("%d, %d, %d\r\n",
        //                 share_buffer_data_size, temp_buffer_data_size, temp_buffer_remained_size);
        if (ret != SBC_DECODER_STATUS_OK) {
            TASK_LOG_MSGID_I("[SBC]invalid SBC media payload with result for parsing %d\r\n", ret);
        }
        if (share_buffer_data_size == 0 && temp_buffer_remained_size == 0) {
            TASK_LOG_MSGID_I("[SBC]there is no enough data for parsing\r\n");
        }
        /* update buffer information */
        share_buffer_remained_data_size -= share_buffer_data_size;
        if (share_buffer_remained_data_size > SHARE_BUFFER_LENGTH) {
            TASK_LOG_MSGID_I("size overflow: %d, %d\r\n", share_buffer_remained_data_size, share_buffer_data_size);
            share_buffer_remained_data_size = SHARE_BUFFER_LENGTH;
        }
        p_share_buffer->buffer_data_byte_count = share_buffer_remained_data_size;//update share buffer data size
        memmove(share_buffer_ptr, \
                (share_buffer_ptr + share_buffer_data_size), \
                share_buffer_remained_data_size);//move remained data to share buffer base addr.
        temp_buffer_data_size += temp_buffer_remained_size;
        temp_buffer_remained_data_size += temp_buffer_remained_size;
#endif /* #ifndef A2DP_SBC_UT */

        /*SBC decoder */
        ret = sbc_decoder_process(internal_handle->working_buffer2_handle, \
                                  temp_buffer_ptr, \
                                  &temp_buffer_data_size, \
                                  (int16_t *)(pcm_buffer_ptr + pcm_buffer_data_size), \
                                  &pcm_buffer_remained_size);
        // TASK_LOG_MSGID_I("%d, %d, %d\r\n",
        //                 temp_buffer_data_size, pcm_buffer_data_size, pcm_buffer_remained_size);
        if (ret != SBC_DECODER_STATUS_OK) {
            TASK_LOG_MSGID_I("[SBC]invalid SBC media payload with result for decoding %d\r\n", ret);
        }
        if (temp_buffer_data_size == 0 && pcm_buffer_remained_size == 0) {
            TASK_LOG_MSGID_I("[SBC]there is no enough data for decoding\r\n");
        }
#ifdef LATENCY_CAL
        bt_codec_silence_data_check_callback((int16_t *)(pcm_buffer_ptr + pcm_buffer_data_size), pcm_buffer_remained_size);
#endif /* #ifdef LATENCY_CAL */
        temp_buffer_remained_data_size -= temp_buffer_data_size;
        if (temp_buffer_remained_data_size > TEMP_BUFFER_LENGTH) {
            TASK_LOG_MSGID_I("size overflow: %d, %d\r\n", temp_buffer_remained_data_size, temp_buffer_data_size);
            temp_buffer_remained_data_size = TEMP_BUFFER_LENGTH;
        }
        // printf("temp_buffer_remained_data_size = %d\r\n", temp_buffer_remained_data_size);
        p_temp_buffer->buffer_data_byte_count = temp_buffer_remained_data_size;//update temp buffer data size
        memmove(temp_buffer_ptr, \
                (temp_buffer_ptr + temp_buffer_data_size), \
                temp_buffer_remained_data_size);//move remained data to temp buffer base addr.
        pcm_buffer_data_size += pcm_buffer_remained_size;
        p_pcm_buffer->buffer_data_byte_count = pcm_buffer_data_size;//update pcm buffer data size
    }

    handle->state = BT_CODEC_STATE_PLAY;

    bt_audio_mutex_lock(g_xSemaphore_Audio);
    /* Tinypcm Playback*/
    if (a2dp_ucPCMOpened == 0) {
        a2dp_params.format = MSD_PCM_FMT_S16_LE;
        a2dp_params.channels = internal_handle->channel_number;
        a2dp_params.period_count = PERIOD_COUNT;
        a2dp_params.period_size = PERIOD_SIZE;
        a2dp_params.rate = internal_handle->handle.get_sampling_rate(&internal_handle->handle);

        TASK_LOG_MSGID_I("channels = %d, cnt = %d, size = %d, rate = %d\r\n",
                         a2dp_params.channels, a2dp_params.period_count, a2dp_params.period_size, a2dp_params.rate);

        connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
        connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
        connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);

        ret = snd_pcm_open(&a2dp_w_snd, "track0", 0, 0);
        ret = snd_pcm_hw_params(a2dp_w_snd, &a2dp_params);
        ret = snd_pcm_prepare(a2dp_w_snd);
        a2dp_ucPCMOpened = 1;
    }

    if (sbc_cmd_queue_handle) {
        sbc_cmd_queue_event_id_t cmd = SBC_CMD_QUEUE_EVENT_PLAY;
        xQueueSend(sbc_cmd_queue_handle, &cmd, (TickType_t) 0);
    }
    bt_audio_mutex_unlock(g_xSemaphore_Audio);
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_close_sink_sbc_codec(bt_media_handle_t *handle)
{
    sbc_decode_event_deregister_callback(SBC_DECODE_QUEUE_EVENT_DECODE);
    handle->state = BT_CODEC_STATE_STOP;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_a2dp_sink_sbc_play(bt_media_handle_t *handle)
{
    sbc_decoder_status_t ret;
    bt_codec_media_status_t rett;
    bt_a2dp_audio_internal_handle_t *internal_handle = bt_a2dp_internal_handle;
    TASK_LOG_MSGID_CTRL("[SBC]play\r\n", 0);
    if (handle->state != BT_CODEC_STATE_READY && handle->state != BT_CODEC_STATE_STOP) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
#if defined(BT_A2DP_BITSTREAM_DUMP_DEBUG)
    bt_a2dp_ptr = 0;
    memset(bt_a2dp_bitstream, 0, BT_A2DP_BS_LEN * sizeof(uint8_t));
#endif /* #if defined(BT_A2DP_BITSTREAM_DUMP_DEBUG) */
    /* initialize parser handle */
    //    ret = sbc_media_payload_parser_init(&internal_handle->working_buffer1_handle, internal_handle->working_buffer1, 0x3453);
    ret = sbc_media_payload_parser_init(&internal_handle->working_buffer1_handle, internal_handle->working_buffer1, 0xbd9c);

    if (ret != SBC_DECODER_STATUS_OK) {
        TASK_LOG_MSGID_I("[SBC]parser initial fail, return = %d", ret);
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    /* initialize sbc decoder handle */
    ret = sbc_decoder_init(&internal_handle->working_buffer2_handle, internal_handle->working_buffer2);
    if (ret != SBC_DECODER_STATUS_OK) {
        TASK_LOG_MSGID_I("[SBC]sbc decoder initial fail, return = %d", ret);
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }

    rett = bt_open_sink_sbc_codec(handle);
    return rett;
}

static bt_codec_media_status_t bt_a2dp_sink_sbc_stop(bt_media_handle_t *handle)
{
    bt_codec_media_status_t result;
    TASK_LOG_MSGID_CTRL("[SBC]stop--state: %d\r\n", 1, handle->state);
    if (handle->state == BT_CODEC_STATE_READY) {
        sbc_decode_event_deregister_callback(SBC_DECODE_QUEUE_EVENT_DECODE);
        handle->state = BT_CODEC_STATE_STOP;
        result = BT_CODEC_MEDIA_STATUS_OK;
    } else if (handle->state != BT_CODEC_STATE_PLAY && handle->state != BT_CODEC_STATE_ERROR) {
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    } else {
        result = bt_close_sink_sbc_codec(handle);
    }
    return result;
}

static bt_codec_media_status_t bt_a2dp_sink_sbc_process(bt_media_handle_t *handle, bt_codec_media_event_t event)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    if (internal_handle == NULL) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_a2dp_sink_parse_sbc_info(bt_a2dp_audio_internal_handle_t *internal_handle)
{
    uint8_t channel_mode, sample_rate;
    bt_codec_a2dp_audio_t *pParam = (bt_codec_a2dp_audio_t *)&internal_handle->codec_info;

    channel_mode = pParam->codec_cap.codec.sbc.channel_mode;
    sample_rate  = pParam->codec_cap.codec.sbc.sample_rate;
    TASK_LOG_MSGID_I("[A2DP][SBC]sample rate=%d, channel=%d \n", sample_rate, channel_mode);
    switch (channel_mode) {
        case 8:
            internal_handle->channel_number = 1;
            break;
        case 4:
        case 2:
        case 1:
            internal_handle->channel_number = 2;
            break;
        default:
            return BT_CODEC_MEDIA_STATUS_INVALID_PARAM;
    }
    hal_audio_set_stream_out_channel_number((internal_handle->channel_number == 1) ? HAL_AUDIO_MONO : HAL_AUDIO_STEREO);

    switch (sample_rate) {
        case 8:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_16KHZ;
            break;
        case 4:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_32KHZ;
            break;
        case 2:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_44_1KHZ;
            break;
        case 1:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_48KHZ;
            break;
        default:
            return BT_CODEC_MEDIA_STATUS_INVALID_PARAM;
    }
    hal_audio_set_stream_out_sampling_rate((hal_audio_sampling_rate_t)internal_handle->sample_rate);

    return BT_CODEC_MEDIA_STATUS_OK;
}

#ifdef MTK_BT_A2DP_AAC_ENABLE
/* aac */
#if !defined(MTK_AVM_DIRECT)
static void aac_write_silence(bt_a2dp_audio_internal_handle_t *internal_handle)
{
    ring_buffer_information_t *p_ring = &internal_handle->ring_info;
    uint16_t bs_addr;
    uint16_t bs_wptr;
    uint16_t bs_rptr;

    bs_addr = *DSP_AAC_DEC_DM_BS_ADDR;
    bs_wptr = *DSP_AAC_DEC_DM_BS_MCU_W_PTR;    // in word
    bs_rptr = *DSP_AAC_DEC_DM_BS_DSP_R_PTR;    // in word
    p_ring->write_pointer       = (uint32_t)((bs_wptr - bs_addr) << 1); // in byte
    p_ring->read_pointer        = (uint32_t)((bs_rptr - bs_addr) << 1);

    {
        uint32_t loop_idx;
        uint32_t loop_cnt = 2;
        int32_t read_byte_cnt = SILENCE_TOTAL_LENGTH;
        for (loop_idx = 0; loop_idx < loop_cnt; loop_idx++) {
            uint32_t           write_byte_cnt = 0;
            uint32_t           move_byte_cnt;
            uint8_t            *p_mcu_buf      = internal_handle->aac_silence_pattern;
            volatile uint16_t  *p_dsp_buf      = NULL;

            p_mcu_buf += (SILENCE_TOTAL_LENGTH - read_byte_cnt);
            ring_buffer_get_write_information_non_mirroring(p_ring, (uint8_t **)&p_dsp_buf, &write_byte_cnt);

            write_byte_cnt &= ~0x1;     // Make it even
            move_byte_cnt = MINIMUM(write_byte_cnt, read_byte_cnt);
            {
                // Move data
                uint32_t move_word_cnt = move_byte_cnt >> 1;
                if (move_word_cnt > 0) {
                    audio_idma_write_to_dsp(p_dsp_buf, (uint16_t *)p_mcu_buf, move_word_cnt);
                    read_byte_cnt -= (move_word_cnt << 1);
                } else {    // Read buffer empty or write buffer full
                    break;
                }
            }
            ring_buffer_write_done_non_mirroring(p_ring, move_byte_cnt);
            *DSP_AAC_DEC_DM_BS_MCU_W_PTR = (uint16_t)(p_ring->write_pointer >> 1) + bs_addr;
        }
    }
    return;
}
#endif /* #if !defined(MTK_AVM_DIRECT) */

#if defined(MTK_AVM_DIRECT)
static void aac_decoder_isr_handler(hal_audio_event_t event, void *data)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)data;
    bt_media_handle_t *handle = (bt_media_handle_t *)internal_handle;

    if (event == HAL_AUDIO_EVENT_TIME_REPORT) {
        handle->handler(handle, BT_CODEC_MEDIA_TIME_REPORT);
    } else if (event == HAL_AUDIO_EVENT_LTCS_REPORT) {
        handle->handler(handle, BT_CODEC_MEDIA_LTCS_DATA_REPORT);
    } else if (event == HAL_AUDIO_EVENT_DL_REINIT_REQUEST) {

        TASK_LOG_MSGID_I("[AWS]aac_decoder_isr_handler reinitial sync\r\n", 0);
        handle->handler(handle, BT_CODEC_MEDIA_AUDIO_DL_REINIT_REQUEST);
    } else if (event == HAL_AUDIO_EVENT_ALC_REQUEST) {
        handle->handler(handle, BT_CODEC_MEDIA_AUDIO_DL_ALC_REQUEST);
    } else {
        // KH: Assume that all events are error
        handle->state = BT_CODEC_STATE_ERROR;
        handle->handler(handle, BT_CODEC_MEDIA_ERROR);
    }
}
#else /* #if defined(MTK_AVM_DIRECT) */
static void aac_decoder_isr_handler(void *data)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)data;
    bt_media_handle_t *handle = (bt_media_handle_t *)internal_handle;

#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
    TASK_LOG_MSGID_I("[A2DP[AAC]ISR");
#endif /* #ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG */

    if ((*DSP_AAC_DEC_FSM == DSP_AAC_STATE_IDLE) || (handle->state == BT_CODEC_STATE_ERROR)) {
        return;
    }

    /* error handling, but bypass the buffer underflow warning from DSP */
    if ((*DSP_AAC_DEC_ERROR_REPORT != DSP_AAC_REPORT_NONE)
        && (*DSP_AAC_DEC_ERROR_REPORT != DSP_AAC_REPORT_UNDERFLOW)) {
        internal_handle->error_count ++;

        /* fill silence when underflow continuously */
        if (*DSP_AAC_DEC_ERROR_REPORT == DSP_AAC_REPORT_BUFFER_NOT_ENOUGH) {

            int32_t mcu_data_count;
            if (handle->get_data_count_function) {
                mcu_data_count = handle->get_data_count_function();
            } else {
                mcu_data_count = handle->get_data_count(handle);
            }

            if (mcu_data_count < AAC_FILL_SILENCE_TRHESHOLD) {
                aac_write_silence(internal_handle);
            }
        } else {
            LISR_LOG_MSGID_E("[A2DP]DECODER ERR(%x), FSM:%x REPORT=%x\n", 3, (unsigned int)internal_handle->error_count, *DSP_AAC_DEC_FSM, *DSP_AAC_DEC_ERROR_REPORT);
        }

        LISR_LOG_MSGID_E("[A2DP][AAC]DECODER ERR, FSM:%x  REPORT=%x\n", 2, *DSP_AAC_DEC_FSM, *DSP_AAC_DEC_ERROR_REPORT);
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
        if (internal_handle->aws_internal_flag) {
            LISR_LOG_MSGID_E("[A2DP][AAC]DECODER ERR AWS\n", 0);
            *DSP_AAC_DEC_FSM = DSP_AAC_STATE_STOP;
            handle->state = BT_CODEC_STATE_ERROR;
            handle->handler(handle, BT_CODEC_MEDIA_ERROR);
            return;
        }
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
        if (internal_handle->error_count >= AAC_ERROR_FRAME_THRESHOLD) {
            LISR_LOG_MSGID_E("[A2DP][AAC]DECODER ERR OVER THRESHOLD\n", 0);
            *DSP_AAC_DEC_FSM = DSP_AAC_STATE_STOP;
            handle->state = BT_CODEC_STATE_ERROR;
            handle->handler(handle, BT_CODEC_MEDIA_ERROR);
            return;
        }
    } else { //if error is not consecutive, reset to 0
        internal_handle->error_count = 0;
    }

    /* bitstream buffer initialization */
    if (!internal_handle->ring_info.buffer_base_pointer) {
        uint16_t bs_page = DSP_AAC_PAGE_NUM;
        uint16_t bs_addr = *DSP_AAC_DEC_DM_BS_ADDR;
        uint16_t bs_size = *DSP_AAC_DEC_DM_BS_LEN << 1;
        internal_handle->ring_info.buffer_byte_count = (uint32_t)bs_size;
        internal_handle->ring_info.buffer_base_pointer = (uint8_t *)DSP_DM_ADDR(bs_page, bs_addr);
    }
    internal_handle->frame_count++;
    if (internal_handle->frame_count == 1) {
        uint16_t bs_addr = *DSP_AAC_DEC_DM_BS_ADDR;
        *DSP_AAC_DEC_DM_BS_MCU_W_PTR = bs_addr;
    }
    /* fill bitstream */
    bt_write_bs_to_dsp(internal_handle);
    if (handle->directly_access_dsp_function == NULL) {//Share buffer flow
        if (!handle->waiting) {
            handle->waiting = true;
            if ((!handle->underflow) && (*DSP_AAC_DEC_ERROR_REPORT == DSP_AAC_REPORT_UNDERFLOW)) {
                handle->underflow = true;
                LISR_LOG_MSGID_I("[A2DP][AAC]DSP underflow \n", 0);
                handle->handler(handle, BT_CODEC_MEDIA_UNDERFLOW);
            } else {
                handle->handler(handle, BT_CODEC_MEDIA_REQUEST);
            }
        }
    }
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    else {
        if (*DSP_AAC_DEC_ERROR_REPORT == DSP_AAC_REPORT_UNDERFLOW) {
            if (internal_handle->aws_internal_flag) {
                LISR_LOG_MSGID_I("[A2DP][AAC]DSP underflow \n", 0);
                handle->handler(handle, BT_CODEC_MEDIA_UNDERFLOW);
            }
        }
    }
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}
#endif /* #if defined(MTK_AVM_DIRECT) */

static bt_codec_media_status_t bt_open_sink_aac_codec(bt_media_handle_t *handle)
{
#if defined(MTK_AVM_DIRECT)
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    mcu2dsp_start_param_t start_param;
    void *p_param_share;

    hal_audio_service_hook_callback(AUDIO_MESSAGE_TYPE_BT_AUDIO_DL, aac_decoder_isr_handler, internal_handle);

    // Collect parameters
    start_param.param.stream_in     = STREAM_IN_A2DP;
    start_param.param.stream_out    = STREAM_OUT_AFE;

    start_param.stream_in_param.a2dp.content_protection_exist = internal_handle->is_cp;
    start_param.stream_in_param.a2dp.start_time_stamp = internal_handle->time_stamp;
    start_param.stream_in_param.a2dp.time_stamp_ratio = internal_handle->ts_ratio;
    start_param.stream_in_param.a2dp.alc_enable = internal_handle->is_alc_en;
    start_param.stream_in_param.a2dp.latency_monitor_enable = internal_handle->is_lm_en;
    start_param.stream_out_param.afe.aws_flag   =  internal_handle->is_aws;

    p_param_share = hal_audio_dsp_controller_put_paramter(&start_param, sizeof(mcu2dsp_start_param_t), AUDIO_MESSAGE_TYPE_BT_AUDIO_DL);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_BT_AUDIO_DL_START, 0, (uint32_t)p_param_share, true);

#else /* #if defined(MTK_AVM_DIRECT) */
    uint16_t I = 0;
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;

    TASK_LOG_MSGID_CTRL("[AAC]open_codec\r\n", 0);
    internal_handle->error_count = 0;

    audio_service_hook_isr(DSP_D2C_AAC_DEC_INT, aac_decoder_isr_handler, internal_handle);
    audio_service_setflag(handle->audio_id);
    *DSP_AUDIO_ASP_COMMON_FLAG_1 = 0;    //clear dsp audio common flag

    *DSP_AAC_DEC_ALLERROR_REPORT = 0;
    *DSP_AAC_DEC_DUAL_SCE = 0;

    if (*DSP_AAC_DEC_FSM != DSP_AAC_STATE_IDLE) {
        TASK_LOG_MSGID_E("[A2DP]AAC OPEN STATE ERROR(%x)\n", 1, *DSP_AAC_DEC_FSM);
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
#if defined(AWS_DEBUG_CODE)
    TASK_LOG_MSGID_I("[AWS] set AWS flag\r\n");
    bt_codec_a2dp_aws_set_flag(handle, true);
#endif /* #if defined(AWS_DEBUG_CODE) */
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    if (internal_handle->aws_flag) {
        bt_codec_a2dp_aws_play_setting(internal_handle);
    }
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    internal_handle->frame_count = 0;
    *DSP_AAC_DEC_FSM = DSP_AAC_STATE_START;

    *DSP_AUDIO_CTRL2 |= DSP_AAC_CTRL_ASP;
    *DSP_AUDIO_FLEXI_CTRL |= (FLEXI_VBI_ENABLE | FLEXI_SD_ENABLE);
    afe_set_path_type(HAL_AUDIO_PLAYBACK_MUSIC);
    dsp_audio_fw_dynamic_download(DDID_AAC);

    aac_get_silence_pattern(internal_handle->sample_rate, internal_handle->channel_number, internal_handle->aac_silence_pattern);
    audio_playback_on(ASP_TYPE_AAC_DEC, internal_handle->sample_rate);

#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    if (internal_handle->aws_internal_flag == false)
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    {
        for (I = 0; ; I++) {
            if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_PLAYING) {
                break;
            }
            /* This is the case when AAC codec has started and encoutered an error,
               aacPlaybackHisr found this and set the state to STOP, then AAC codec
               set the state to IDLE. */
            if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_IDLE) {
                TASK_LOG_MSGID_E("[A2DP]ERROR when AAC CODEC STARTS \n", 0);
                break;
            }
            if (I > 80) {
                TASK_LOG_MSGID_E("[A2DP][AAC] CODEC OPEN ERROR\n", 0);
                return BT_CODEC_MEDIA_STATUS_ERROR;
            }
            hal_gpt_delay_ms(9);
        }
    }
#if defined(AWS_DEBUG_CODE)
    else {  /* Manually reset DSP for unit test */
        bt_codec_a2dp_aws_set_initial_sync(handle);
        for (I = 0; ; I++) {
            if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_PLAYING) {
                break;
            }
            /* This is the case when AAC codec has started and encoutered an error,
               aacPlaybackHisr found this and set the state to STOP, then AAC codec
               set the state to IDLE. */
            if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_IDLE) {
                TASK_LOG_MSGID_E("[A2DP]ERROR when AAC CODEC STARTS \n", 0);
                break;
            }
            if (I > 80) {
                TASK_LOG_MSGID_E("[A2DP][AAC] CODEC OPEN ERROR\n", 0);
                return BT_CODEC_MEDIA_STATUS_ERROR;
            }
            hal_gpt_delay_ms(9);
        }
    }
#endif /* #if defined(AWS_DEBUG_CODE) */
#endif /* #if defined(MTK_AVM_DIRECT) */

    handle->state = BT_CODEC_STATE_PLAY;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_close_sink_aac_codec(bt_media_handle_t *handle)
{
#if defined(MTK_AVM_DIRECT)
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_BT_AUDIO_DL_STOP, 0, 0, true);
    hal_audio_service_unhook_callback(AUDIO_MESSAGE_TYPE_BT_AUDIO_DL);
#else /* #if defined(MTK_AVM_DIRECT) */
    uint16_t I = 0;
    TASK_LOG_MSGID_CTRL("[AAC]close_codec\r\n", 0);
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    {
        bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)handle;
        if (internal_handle->aws_internal_flag && !internal_handle->aws_flag && !internal_handle->aws_init_sync_flag) {
            *DSP_AAC_DEC_FSM = DSP_AAC_STATE_IDLE;
        }
    }
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    for (I = 0; ; I++) {
        if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_IDLE) {
            break;
        }
        if (*DSP_AAC_DEC_FSM == DSP_AAC_STATE_PLAYING) {
            *DSP_AAC_DEC_FSM = DSP_AAC_STATE_STOP;
        }
        if (I > 80) {
            return BT_CODEC_MEDIA_STATUS_ERROR;
        }
        hal_gpt_delay_ms(9);
    }

    *DSP_AUDIO_CTRL2 &= ~(DSP_AAC_CTRL_ASP | DSP_PCM_R_DIS);
    *DSP_AUDIO_FLEXI_CTRL &= ~(FLEXI_VBI_ENABLE | FLEXI_SD_ENABLE);

    audio_playback_off();
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    {
        bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *)handle;
        if (internal_handle->aws_internal_flag) {
            bt_codec_a2dp_aws_stop_setting(internal_handle);
        }
    }
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
#if defined(AWS_DEBUG_CODE)
    TASK_LOG_MSGID_I("[AWS] clear AWS flag\r\n");
    bt_codec_a2dp_aws_set_flag(handle, false);
#endif /* #if defined(AWS_DEBUG_CODE) */
    audio_service_unhook_isr(DSP_D2C_AAC_DEC_INT);
    audio_service_clearflag(handle->audio_id);
#endif /* #if defined(MTK_AVM_DIRECT) */

    handle->state = BT_CODEC_STATE_STOP;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_a2dp_sink_aac_play(bt_media_handle_t *handle)
{
#if defined(MTK_AVM_DIRECT)
    bt_audio_mutex_lock(g_xSemaphore_Audio);
#endif /* #if defined(MTK_AVM_DIRECT) */
    bt_codec_media_status_t ret;
    TASK_LOG_MSGID_CTRL("[AAC]play\r\n", 0);
    if (handle->state != BT_CODEC_STATE_READY && handle->state != BT_CODEC_STATE_STOP) {
        TASK_LOG_MSGID_E("[A2DP][AAC] CODEC PLAY ERROR \n", 0);
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    ret = bt_open_sink_aac_codec(handle);
#if defined(MTK_AVM_DIRECT)
    bt_audio_mutex_unlock(g_xSemaphore_Audio);
#endif /* #if defined(MTK_AVM_DIRECT) */
    return ret;
}

static bt_codec_media_status_t bt_a2dp_sink_aac_stop(bt_media_handle_t *handle)
{
    bt_codec_media_status_t result;
    TASK_LOG_MSGID_CTRL("[AAC]stop--state: %d\r\n", 1, handle->state);
    if (handle->state == BT_CODEC_STATE_READY) {
        handle->state = BT_CODEC_STATE_STOP;
        result = BT_CODEC_MEDIA_STATUS_OK;
    } else if (handle->state != BT_CODEC_STATE_PLAY && handle->state != BT_CODEC_STATE_ERROR) {
        TASK_LOG_MSGID_E("[A2DP][AAC] CODEC STOP ERROR \n", 0);
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    } else {
        result = bt_close_sink_aac_codec(handle);
    }
    return result;
}

static bt_codec_media_status_t bt_a2dp_sink_aac_process(bt_media_handle_t *handle, bt_codec_media_event_t event)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    if (internal_handle == NULL) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
}

static uint32_t bt_a2dp_sink_aac_get_ts_ratio(bt_media_handle_t *handle, uint32_t ts0, uint32_t ts1)
{
    uint32_t ts_ratio = ((ts1 - ts0) >> 10);
    uint32_t ts2;
    if (((ts1 - ts0) & 0x3FF) || (ts1 == ts0) || (ts_ratio > 0xF)) {
        ts_ratio = 1024;
    }
    TASK_LOG_MSGID_I("[A2DP Codec][AAC]ts1:%d, ts0:%d, ts_ratio:%d", ts1, ts0, ts_ratio);
#if defined(MTK_AVM_DIRECT)
    if ((ts_ratio != 1024) && (bt_get_next_pkt_info(ts1, &ts2) == TRUE)) {
        uint32_t next_ratio = ((ts2 - ts1) >> 10);
        if (((ts2 - ts1) & 0x3FF) || (ts2 == ts1) || (next_ratio > 0xF) || (next_ratio != ts_ratio)) {
            ts_ratio = 1024;
            TASK_LOG_MSGID_I("[A2DP Codec][AAC]ts2:%d next_ts_ratio:%d ret_ts_ratio:%d", ts2, next_ratio, ts_ratio);
        }
    }
#endif /* #if defined(MTK_AVM_DIRECT) */
    return ts_ratio;
}

static bt_codec_media_status_t bt_a2dp_sink_parse_aac_info(bt_a2dp_audio_internal_handle_t *internal_handle)
{
    uint8_t channel_mode;
    uint16_t sample_rate;
    bt_codec_a2dp_audio_t *pParam = (bt_codec_a2dp_audio_t *)&internal_handle->codec_info;

    channel_mode = pParam->codec_cap.codec.aac.channels;
    sample_rate  = pParam->codec_cap.codec.aac.sample_rate;
    TASK_LOG_MSGID_I("[A2DP][AAC] sample rate=%x, channel=%x \n", sample_rate, channel_mode);

    switch (channel_mode) {
        case 0x2:
            internal_handle->channel_number = 1;
            break;
        case 0x1:
            internal_handle->channel_number = 2;
            break;
        default:
            return BT_CODEC_MEDIA_STATUS_INVALID_PARAM;
    }
    hal_audio_set_stream_out_channel_number((internal_handle->channel_number == 1) ? HAL_AUDIO_MONO : HAL_AUDIO_STEREO);

    switch (sample_rate) {
        case 0x800:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_8KHZ;
            break;
        case 0x400:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_11_025KHZ;
            break;
        case 0x200:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_12KHZ;
            break;
        case 0x100:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_16KHZ;
            break;
        case 0x80:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_22_05KHZ;
            break;
        case 0x40:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_24KHZ;
            break;
        case 0x20:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_32KHZ;
            break;
        case 0x10:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_44_1KHZ;
            break;
        case 0x8:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_48KHZ;
            break;
        default:
            return BT_CODEC_MEDIA_STATUS_INVALID_PARAM;
    }
    hal_audio_set_stream_out_sampling_rate((hal_audio_sampling_rate_t)internal_handle->sample_rate);

    return BT_CODEC_MEDIA_STATUS_OK;
}
#endif /* #ifdef MTK_BT_A2DP_AAC_ENABLE */

#if defined(MTK_AVM_DIRECT)
#if defined(MTK_BT_A2DP_VENDOR_ENABLE)
static void vendor_decoder_isr_handler(hal_audio_event_t event, void *data)
{
    bt_media_handle_t *handle = (bt_media_handle_t *)data;

    if (event == HAL_AUDIO_EVENT_TIME_REPORT) {
        handle->handler(handle, BT_CODEC_MEDIA_TIME_REPORT);
    } else if (event == HAL_AUDIO_EVENT_LTCS_REPORT) {
        handle->handler(handle, BT_CODEC_MEDIA_LTCS_DATA_REPORT);
    } else if (event == HAL_AUDIO_EVENT_DL_REINIT_REQUEST) {
        handle->handler(handle, BT_CODEC_MEDIA_AUDIO_DL_REINIT_REQUEST);
    } else if (event == HAL_AUDIO_EVENT_ALC_REQUEST) {
        handle->handler(handle, BT_CODEC_MEDIA_AUDIO_DL_ALC_REQUEST);
    } else {
        // KH: Assume that all events are error
        handle->state = BT_CODEC_STATE_ERROR;
        handle->handler(handle, BT_CODEC_MEDIA_ERROR);
    }
}


static bt_codec_media_status_t bt_open_sink_vendor_codec(bt_media_handle_t *handle)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    mcu2dsp_start_param_t start_param;
    void *p_param_share;

    hal_audio_service_hook_callback(AUDIO_MESSAGE_TYPE_BT_AUDIO_DL, vendor_decoder_isr_handler, internal_handle);

    // Collect parameters
    start_param.param.stream_in     = STREAM_IN_A2DP;
    start_param.param.stream_out    = STREAM_OUT_AFE;

    start_param.stream_in_param.a2dp.content_protection_exist = internal_handle->is_cp;
    start_param.stream_in_param.a2dp.start_time_stamp = internal_handle->time_stamp;
    start_param.stream_in_param.a2dp.time_stamp_ratio = internal_handle->ts_ratio;
    start_param.stream_in_param.a2dp.alc_enable = internal_handle->is_alc_en;
    start_param.stream_in_param.a2dp.latency_monitor_enable = internal_handle->is_lm_en;
    start_param.stream_out_param.afe.aws_flag   =  internal_handle->is_aws;

    p_param_share = hal_audio_dsp_controller_put_paramter(&start_param, sizeof(mcu2dsp_start_param_t), AUDIO_MESSAGE_TYPE_BT_AUDIO_DL);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_BT_AUDIO_DL_START, 0, (uint32_t)p_param_share, true);

    handle->state = BT_CODEC_STATE_PLAY;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_close_sink_vendor_codec(bt_media_handle_t *handle)
{
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_BT_AUDIO_DL_STOP, 0, 0, true);
    hal_audio_service_unhook_callback(AUDIO_MESSAGE_TYPE_BT_AUDIO_DL);

    handle->state = BT_CODEC_STATE_STOP;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_a2dp_sink_vendor_play(bt_media_handle_t *handle)
{
#if defined(MTK_AVM_DIRECT)
    bt_audio_mutex_lock(g_xSemaphore_Audio);
#endif /* #if defined(MTK_AVM_DIRECT) */
    bt_codec_media_status_t ret;
    TASK_LOG_MSGID_CTRL("[VENDOR]play\r\n", 0);
    if (handle->state != BT_CODEC_STATE_READY && handle->state != BT_CODEC_STATE_STOP) {
        TASK_LOG_MSGID_E("[A2DP][VENDOR] CODEC PLAY ERROR \n", 0);
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    ret = bt_open_sink_vendor_codec(handle);
#if defined(MTK_AVM_DIRECT)
    bt_audio_mutex_unlock(g_xSemaphore_Audio);
#endif /* #if defined(MTK_AVM_DIRECT) */
    return ret;
}

static bt_codec_media_status_t bt_a2dp_sink_vendor_stop(bt_media_handle_t *handle)
{
    bt_codec_media_status_t result;
    TASK_LOG_MSGID_CTRL("[VENDOR]stop--state: %d\r\n", 1, handle->state);
    if (handle->state == BT_CODEC_STATE_READY) {
        handle->state = BT_CODEC_STATE_STOP;
        result = BT_CODEC_MEDIA_STATUS_OK;
    } else if (handle->state != BT_CODEC_STATE_PLAY && handle->state != BT_CODEC_STATE_ERROR) {
        TASK_LOG_MSGID_E("[A2DP][VENDOR] CODEC STOP ERROR \n", 0);
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    } else {
        result = bt_close_sink_vendor_codec(handle);
    }
    return result;
}

static bt_codec_media_status_t bt_a2dp_sink_vendor_process(bt_media_handle_t *handle, bt_codec_media_event_t event)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    if (internal_handle == NULL) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
}

static uint32_t bt_a2dp_sink_vendor_get_ts_ratio(bt_media_handle_t *handle, uint32_t ts0, uint32_t ts1)
{
#if 0 /*aac ts_ratio*/
    if (((ts1 - ts0) & 0x3FF) || (ts1 == ts0)) {
        return 1024;
    }
    return ((ts1 - ts0) >> 10);
#else /* #if 0 (aac ts_ratio) */
    return 1;
#endif /* #if 0 (aac ts_ratio) */
}

static bt_codec_media_status_t bt_a2dp_sink_parse_vendor_info(bt_a2dp_audio_internal_handle_t *internal_handle)
{
    uint8_t channel_mode, sample_rate;
    bt_codec_a2dp_audio_t *pParam = (bt_codec_a2dp_audio_t *)&internal_handle->codec_info;

    channel_mode = pParam->codec_cap.codec.vendor.channels;
    sample_rate  = pParam->codec_cap.codec.vendor.sample_rate;
    TASK_LOG_MSGID_I("[A2DP][VENDOR]sample rate=%d, channel=%d \n", sample_rate, channel_mode);

    switch (channel_mode) {
        case 0x04:    /*MONO*/
            internal_handle->channel_number = 1;
            break;
        case 0x02:    /*DUAL CHANNEL*/
            internal_handle->channel_number = 2;
            break;
        case 0x01:    /*STEREO*/
            internal_handle->channel_number = 2;
            break;
        default:
            return BT_CODEC_MEDIA_STATUS_INVALID_PARAM;
    }
    hal_audio_set_stream_out_channel_number((internal_handle->channel_number == 1) ? HAL_AUDIO_MONO : HAL_AUDIO_STEREO);

    switch (sample_rate) {
        case 0x20:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_44_1KHZ;
            break;
        case 0x10:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_48KHZ;
            break;
        case 0x08:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_88_2KHZ;
            break;
        case 0x04:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_96KHZ;
            break;
        case 0x02:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_176_4KHZ;
            break;
        case 0x01:
            internal_handle->sample_rate = HAL_AUDIO_SAMPLING_RATE_192KHZ;
            break;
        default:
            return BT_CODEC_MEDIA_STATUS_INVALID_PARAM;
    }
    hal_audio_set_stream_out_sampling_rate((hal_audio_sampling_rate_t)internal_handle->sample_rate);

    return BT_CODEC_MEDIA_STATUS_OK;
}
#endif /* #if defined(MTK_BT_A2DP_VENDOR_ENABLE) */
#endif /* #if defined(MTK_AVM_DIRECT) */


#ifdef MTK_BT_A2DP_SOURCE_SUPPORT

// a2dp source related API
static uint32_t bt_codec_a2dp_source_query_memory_size(bt_media_handle_t *handle)
{
    return sbc_codec_get_buffer_size();
}

static uint32_t bt_codec_a2dp_source_get_payload(bt_media_handle_t *handle, uint8_t *buffer, uint32_t buffer_size, uint32_t *sample_count)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    sbc_codec_media_handle_t        *encoder_handle  = internal_handle->sbc_encode_handle;
    if (encoder_handle == NULL) {
        TASK_LOG_MSGID_E("[A2DP source] NULL handle;", 0);
        return 0;
    }
    return encoder_handle->get_payload(encoder_handle, buffer, buffer_size, sample_count);
}

static void bt_codec_a2dp_source_get_payload_done(bt_media_handle_t *handle)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    sbc_codec_media_handle_t        *encoder_handle  = internal_handle->sbc_encode_handle;
    if (encoder_handle == NULL) {
        TASK_LOG_MSGID_E("[A2DP source] NULL handle;", 0);
    } else {
        encoder_handle->get_payload_done(internal_handle->sbc_encode_handle);
    }
}

static void bt_codec_a2dp_source_query_payload_size(bt_media_handle_t *handle, uint32_t *minimum, uint32_t *total)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    sbc_codec_media_handle_t        *encoder_handle  = internal_handle->sbc_encode_handle;
    if (encoder_handle == NULL) {
        TASK_LOG_MSGID_E("[A2DP source] NULL handle;", 0);
    } else {
        encoder_handle->query_payload_size(encoder_handle, minimum, total);
    }
}

void bt_codec_a2dp_source_event_callback(void *hdl, sbc_codec_event_t event)
{
    bt_media_handle_t *handle = (bt_media_handle_t *)hdl;

    switch (event) {
        case SBC_CODEC_MEDIA_GET_PAYLOAD: {
                handle->handler(handle, BT_CODEC_MEDIA_GET_PAYLOAD);
            }
            break;
    }
}

static uint32_t bt_codec_a2dp_source_set_bit_rate(bt_media_handle_t *handle, uint32_t bit_rate)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    sbc_codec_media_handle_t *encoder_handle = NULL;
    if (internal_handle->sbc_encode_handle == NULL) {
        encoder_handle = sbc_codec_open(
                             bt_codec_a2dp_source_event_callback, &(internal_handle->initial_parameter), handle);
        internal_handle->sbc_encode_handle = encoder_handle;
    } else {
        encoder_handle = internal_handle->sbc_encode_handle;
    }

    return encoder_handle->set_bit_rate(encoder_handle, bit_rate);
}

static bt_codec_media_status_t bt_open_source_sbc_codec(bt_media_handle_t *handle)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    sbc_codec_media_handle_t *encoder_handle = NULL;
    if (internal_handle->sbc_encode_handle == NULL) {
        encoder_handle = sbc_codec_open(
                             bt_codec_a2dp_source_event_callback, &(internal_handle->initial_parameter), handle);
        internal_handle->sbc_encode_handle = encoder_handle;
    } else {
        encoder_handle = internal_handle->sbc_encode_handle;
    }

    if (encoder_handle == NULL) {
        TASK_LOG_MSGID_E("[A2DP source][SBC] sbc encoder init fail.", 0);
        handle->state = BT_CODEC_STATE_ERROR;
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }

    if (handle->buffer_info.buffer_base == NULL) {
        TASK_LOG_MSGID_E("[A2DP source][SBC] set buffer before play.", 0);
        handle->state = BT_CODEC_STATE_ERROR;
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }

    encoder_handle->set_buffer(encoder_handle, handle->buffer_info.buffer_base, handle->buffer_info.buffer_size);
    encoder_handle->play(encoder_handle);

    handle->state = BT_CODEC_STATE_PLAY;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_close_source_sbc_codec(bt_media_handle_t *handle)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    sbc_codec_media_handle_t *encoder_handle = internal_handle->sbc_encode_handle;

    if (encoder_handle == NULL) {
        TASK_LOG_MSGID_E("[SBC] NULL handle\r\n", 0);
        handle->state = BT_CODEC_STATE_ERROR;
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }

    if (encoder_handle->stop(encoder_handle) < 0) {
        TASK_LOG_MSGID_E("[A2DP source][SBC] sbc encoder stop fail.", 0);
        handle->state = BT_CODEC_STATE_ERROR;
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }

    if (sbc_codec_close(encoder_handle) < 0) {
        TASK_LOG_MSGID_E("[A2DP source][SBC] sbc encoder close fail.", 0);
        handle->state = BT_CODEC_STATE_ERROR;
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    internal_handle->sbc_encode_handle = NULL;

    handle->state = BT_CODEC_STATE_STOP;
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_a2dp_source_sbc_play(bt_media_handle_t *handle)
{
    if (handle->state != BT_CODEC_STATE_READY && handle->state != BT_CODEC_STATE_STOP) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
#if defined(BT_A2DP_BITSTREAM_DUMP_DEBUG)
    bt_a2dp_ptr = 0;
    memset(bt_a2dp_bitstream, 0, BT_A2DP_BS_LEN * sizeof(uint8_t));
#endif /* #if defined(BT_A2DP_BITSTREAM_DUMP_DEBUG) */
    return bt_open_source_sbc_codec(handle);
}

static bt_codec_media_status_t bt_a2dp_source_sbc_stop(bt_media_handle_t *handle)
{
    bt_codec_media_status_t result;
    if (handle->state == BT_CODEC_STATE_READY) {
        handle->state = BT_CODEC_STATE_STOP;
        result = BT_CODEC_MEDIA_STATUS_OK;
    } else if (handle->state != BT_CODEC_STATE_PLAY && handle->state != BT_CODEC_STATE_ERROR) {
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    } else {
        result = bt_close_source_sbc_codec(handle);
    }
    return result;
}

static bt_codec_media_status_t bt_a2dp_source_sbc_process(bt_media_handle_t *handle, bt_codec_media_event_t event)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    if (internal_handle == NULL) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
}

static bt_codec_media_status_t bt_a2dp_source_parse_sbc_info(bt_a2dp_audio_internal_handle_t *internal_handle)
{
    sbc_codec_initial_parameter_t *pDst = (sbc_codec_initial_parameter_t *)&internal_handle->initial_parameter;

    pDst->role = SBC_CODEC_MEDIA_ENCODER;
    pDst->cap.alloc_method = internal_handle->codec_info.codec_cap.codec.sbc.alloc_method;
    pDst->cap.block_length = internal_handle->codec_info.codec_cap.codec.sbc.block_length;
    pDst->cap.channel_mode = internal_handle->codec_info.codec_cap.codec.sbc.channel_mode;
    pDst->cap.max_bit_pool = internal_handle->codec_info.codec_cap.codec.sbc.max_bit_pool;
    pDst->cap.min_bit_pool = internal_handle->codec_info.codec_cap.codec.sbc.min_bit_pool;
    pDst->cap.sample_rate  = internal_handle->codec_info.codec_cap.codec.sbc.sample_rate;
    pDst->cap.subband_num  = internal_handle->codec_info.codec_cap.codec.sbc.subband_num;

    return BT_CODEC_MEDIA_STATUS_OK;
}

#endif /* #ifdef MTK_BT_A2DP_SOURCE_SUPPORT */
// temp before app rate change function ready
uint32_t sampling_rate_enum_to_value(hal_audio_sampling_rate_t hal_audio_sampling_rate_enum)
{
    switch (hal_audio_sampling_rate_enum) {
        case HAL_AUDIO_SAMPLING_RATE_8KHZ:
            return   8000;
        case HAL_AUDIO_SAMPLING_RATE_11_025KHZ:
            return  11025;
        case HAL_AUDIO_SAMPLING_RATE_12KHZ:
            return  12000;
        case HAL_AUDIO_SAMPLING_RATE_16KHZ:
            return  16000;
        case HAL_AUDIO_SAMPLING_RATE_22_05KHZ:
            return  22050;
        case HAL_AUDIO_SAMPLING_RATE_24KHZ:
            return  24000;
        case HAL_AUDIO_SAMPLING_RATE_32KHZ:
            return  32000;
        case HAL_AUDIO_SAMPLING_RATE_44_1KHZ:
            return  44100;
        case HAL_AUDIO_SAMPLING_RATE_48KHZ:
            return  48000;
        case HAL_AUDIO_SAMPLING_RATE_88_2KHZ:
            return  88200;
        case HAL_AUDIO_SAMPLING_RATE_96KHZ:
            return  96000;
        case HAL_AUDIO_SAMPLING_RATE_176_4KHZ:
            return 176400;
        case HAL_AUDIO_SAMPLING_RATE_192KHZ:
            return 192000;

        default:
            return 8000;
    }
}

static void bt_a2dp_release_internal_handler(void)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = bt_a2dp_internal_handle;
    if (internal_handle != NULL) {
        vPortFree(internal_handle);
        internal_handle = NULL;
        bt_a2dp_internal_handle = NULL;
    }
}

bt_media_handle_t *bt_codec_a2dp_open(bt_codec_a2dp_callback_t bt_a2dp_callback, const bt_codec_a2dp_audio_t *param)
{
    int8_t ret = -1;
    bt_media_handle_t *handle;
    bt_a2dp_audio_internal_handle_t *internal_handle = NULL; /*internal handler*/

    TASK_LOG_MSGID_I("[A2DP]Open codec\n");

#if !defined(MTK_AVM_DIRECT)
    uint16_t audio_id = audio_get_id();

    if (audio_id > MAX_AUDIO_FUNCTIONS) {
        return 0;
    }
#endif /* #if !defined(MTK_AVM_DIRECT) */

#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
    TASK_LOG_MSGID_I("[A2DP]Open codec--role: %d, type: %d, 1: %d, 2: %d, 3: %d, 4: %d, 5: %d, 6: %d, 7: %d\n",
                     param->role, param->codec_cap.type,
                     param->codec_cap.codec.sbc.alloc_method,
                     param->codec_cap.codec.sbc.block_length,
                     param->codec_cap.codec.sbc.channel_mode,
                     param->codec_cap.codec.sbc.max_bit_pool,
                     param->codec_cap.codec.sbc.min_bit_pool,
                     param->codec_cap.codec.sbc.sample_rate,
                     param->codec_cap.codec.sbc.subband_num);
#endif /* #ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG */
    /* alloc internal handle space */
    internal_handle = (bt_a2dp_audio_internal_handle_t *)pvPortMalloc(sizeof(bt_a2dp_audio_internal_handle_t));
    if (internal_handle == NULL) {
        return 0;
    }
    memset(internal_handle, 0, sizeof(bt_a2dp_audio_internal_handle_t));

    /* assign internal handler to be global and static handler */
    bt_a2dp_internal_handle = internal_handle;

    handle = &internal_handle->handle;
    internal_handle->codec_info = *(bt_codec_a2dp_audio_t *)param;
#if !defined(MTK_AVM_DIRECT)
    handle->audio_id = audio_id;
#endif /* #if !defined(MTK_AVM_DIRECT) */
    handle->handler = bt_a2dp_callback;
    handle->directly_access_dsp_function = NULL;
    handle->get_data_count_function = NULL;
    handle->buffer_info.buffer_base = NULL;
    handle->access_share_buffer_function = NULL;
    bt_codec_buffer_function_init(handle);
    if (internal_handle->codec_info.role == BT_A2DP_SINK) {
        if (internal_handle->codec_info.codec_cap.type == BT_A2DP_CODEC_SBC) {
            bt_codec_media_status_t result = BT_CODEC_MEDIA_STATUS_OK;
            handle->play    = bt_a2dp_sink_sbc_play;
            handle->stop    = bt_a2dp_sink_sbc_stop;
            handle->process = bt_a2dp_sink_sbc_process;
            handle->side_tone_enable = bt_side_tone_enable;
            handle->side_tone_disable = bt_side_tone_disable;
            result = bt_a2dp_sink_parse_sbc_info(internal_handle);
            if (BT_CODEC_MEDIA_STATUS_OK != result) {
                if (internal_handle != NULL) {
                    vPortFree(internal_handle);
                    internal_handle = NULL;
                }
                return 0;
            }

#ifndef MTK_PORTING_AB
            {
                mcu2dsp_open_param_t open_param;
                void *p_param_share;

                open_param.param.stream_in  = STREAM_IN_A2DP;
                open_param.param.stream_out = STREAM_OUT_AFE;

                memcpy(&open_param.stream_in_param.a2dp.codec_info, param, sizeof(bt_codec_a2dp_audio_t));
                open_param.stream_in_param.a2dp.p_share_info    = hal_audio_query_bt_audio_dl_share_info();
                open_param.stream_in_param.a2dp.p_asi_buf       = hal_audio_query_ltcs_asi_buf();
                open_param.stream_in_param.a2dp.p_min_gap_buf   = hal_audio_query_ltcs_min_gap_buf();
                open_param.stream_in_param.a2dp.sink_latency    = bt_sink_srv_ami_get_a2dp_sink_latency();
                open_param.stream_in_param.a2dp.bt_inf_address  = bt_sink_srv_ami_get_bt_inf_address();
                open_param.stream_in_param.a2dp.clk_info_address   = hal_audio_query_rcdc_share_info();
                hal_audio_a2dp_reset_share_info(open_param.stream_in_param.a2dp.p_share_info);
                open_param.stream_in_param.a2dp.p_current_bit_rate = hal_audio_report_bitrate_buf();
                open_param.stream_in_param.a2dp.p_lostnum_report = hal_audio_report_lostnum_buf();
#if 0
                open_param.stream_out_param.afe.audio_device    = HAL_AUDIO_DEVICE_DAC_DUAL;
                open_param.stream_out_param.afe.audio_interface = HAL_AUDIO_INTERFACE_1;
                open_param.stream_out_param.afe.stream_channel  = HAL_AUDIO_DIRECT;
#else /* #if 0 */
                hal_audio_get_stream_out_setting_config(AU_DSP_AUDIO, &open_param.stream_out_param.afe.audio_device, &open_param.stream_out_param.afe.stream_channel, &open_param.stream_out_param.afe.audio_interface);
                TASK_LOG_MSGID_I("[Rdebug] A2DP CODEC_SBC out_device(0x%x), channel(%d), interface(%d)", open_param.stream_out_param.afe.audio_device, open_param.stream_out_param.afe.stream_channel, open_param.stream_out_param.afe.audio_interface);
#endif /* #if 0 */
                open_param.stream_out_param.afe.memory          = HAL_AUDIO_MEM1;
                open_param.stream_out_param.afe.format          = AFE_PCM_FORMAT_S32_LE;
                open_param.stream_out_param.afe.stream_out_sampling_rate   = sampling_rate_enum_to_value(internal_handle->sample_rate);
                //open_param.stream_out_param.afe.stream_out_sampling_rate  = p_hdl_effect->get_output_samplingrate(internal_handle->sample_rate);
                //open_param.stream_out_param.afe.sampling_rate   = sampling_rate_enum_to_value(internal_handle->sample_rate);
                open_param.stream_out_param.afe.sampling_rate   = sampling_rate_enum_to_value(hal_audio_get_device_out_supported_frequency(open_param.stream_out_param.afe.audio_device, internal_handle->sample_rate));
                //open_param.stream_out_param.afe.sampling_rate   = hal_audio_get_device_out_supported_frequency(HAL_AUDIO_DEVICE_HEADSET,open_param.stream_out_param.afe.stream_out_sampling_rate)
                open_param.stream_out_param.afe.irq_period      = 8;
                open_param.stream_out_param.afe.frame_size      = 1024;
                open_param.stream_out_param.afe.frame_number    = 4;
                open_param.stream_out_param.afe.hw_gain         = true;
                if (open_param.stream_out_param.afe.audio_device == HAL_AUDIO_DEVICE_I2S_MASTER) {
                    if (open_param.stream_out_param.afe.sampling_rate > 48000) {
                        open_param.stream_out_param.afe.misc_parms      = I2S_CLK_SOURCE_APLL;
                    } else {
                        open_param.stream_out_param.afe.misc_parms      = I2S_CLK_SOURCE_DCXO;
                    }
                } else {
                    open_param.stream_out_param.afe.misc_parms      = DOWNLINK_PERFORMANCE_NORMAL;
                }
#if defined(MTK_EXTERNAL_DSP_NEED_SUPPORT)
                ami_set_afe_param(STREAM_OUT, internal_handle->sample_rate, true);
#endif /* #if defined(MTK_EXTERNAL_DSP_NEED_SUPPORT) */
                p_param_share = hal_audio_dsp_controller_put_paramter(&open_param, sizeof(mcu2dsp_open_param_t), AUDIO_MESSAGE_TYPE_BT_AUDIO_DL);
                hal_audio_dsp_dl_clkmux_control(AUDIO_MESSAGE_TYPE_BT_AUDIO_DL, open_param.stream_out_param.afe.audio_device, open_param.stream_out_param.afe.sampling_rate, TRUE);
                // Notify to do dynamic download. Use async wait.
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_BT_AUDIO_DL_OPEN, AUDIO_DSP_CODEC_TYPE_SBC, (uint32_t)p_param_share, false);
            }
#endif /* #ifndef MTK_PORTING_AB */

            /* alloc memory for sbc decoder */
            ret = sbc_buffer_alloc();
            if (ret != 1) {
                bt_a2dp_release_internal_handler();
                return 0;
            }

            /* create queue */
            if (sbc_decode_queue_handle == NULL) {
                //TASK_LOG_MSGID_I("Create queue\r\n");
                sbc_decode_queue_handle = xQueueCreate(SBC_DECODE_QUEUE_LENGTH, sizeof(sbc_decode_queue_event_t));
                if (sbc_decode_queue_handle == NULL) {
                    TASK_LOG_MSGID_I("Queue was not created and must not be used\r\n");
                }
                for (uint8_t id_idx = 0; id_idx < MAX_SBC_DECODE_FUNCTIONS; id_idx++) {
                    sbc_decode_queue_id_array[id_idx] = SBC_DECODE_QUEUE_EVENT_NONE;
                }
            }
#ifdef LATENCY_CAL
            g_first_open_codec = true;
#endif /* #ifdef LATENCY_CAL */
            /* create decode task */
            sbc_decode_task_create();

            // create play task and command queue
            sbc_cmd_queue_handle = xQueueCreate(SBC_DECODE_QUEUE_LENGTH, sizeof(sbc_cmd_queue_event_id_t));
            sbc_play_task_create();


#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
            TASK_LOG_MSGID_I("[A2DP][SBC]Codec open");
#endif /* #ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG */
        }
#ifdef MTK_BT_A2DP_AAC_ENABLE
        else if (internal_handle->codec_info.codec_cap.type == BT_A2DP_CODEC_AAC) {
            bt_codec_media_status_t result = BT_CODEC_MEDIA_STATUS_OK;
            handle->play    = bt_a2dp_sink_aac_play;
            handle->stop    = bt_a2dp_sink_aac_stop;
            handle->process = bt_a2dp_sink_aac_process;
            handle->get_ts_ratio = bt_a2dp_sink_aac_get_ts_ratio;
            handle->side_tone_enable = bt_side_tone_enable;
            handle->side_tone_disable = bt_side_tone_disable;
            result = bt_a2dp_sink_parse_aac_info(internal_handle);
            if (BT_CODEC_MEDIA_STATUS_OK != result) {
                if (internal_handle != NULL) {
                    vPortFree(internal_handle);
                    internal_handle = NULL;
                }
                return 0;
            }

#ifndef MTK_PORTING_AB
            {
                mcu2dsp_open_param_t open_param;
                void *p_param_share;

                open_param.param.stream_in  = STREAM_IN_A2DP;
                open_param.param.stream_out = STREAM_OUT_AFE;

                memcpy(&open_param.stream_in_param.a2dp.codec_info, param, sizeof(bt_codec_a2dp_audio_t));
                open_param.stream_in_param.a2dp.p_share_info = hal_audio_query_bt_audio_dl_share_info();
                hal_audio_a2dp_reset_share_info(open_param.stream_in_param.a2dp.p_share_info);
                open_param.stream_in_param.a2dp.p_asi_buf          = hal_audio_query_ltcs_asi_buf();
                open_param.stream_in_param.a2dp.p_min_gap_buf      = hal_audio_query_ltcs_min_gap_buf();
                open_param.stream_in_param.a2dp.p_current_bit_rate = hal_audio_report_bitrate_buf();
                open_param.stream_in_param.a2dp.p_lostnum_report = hal_audio_report_lostnum_buf();
                open_param.stream_in_param.a2dp.sink_latency    = bt_sink_srv_ami_get_a2dp_sink_latency();
                open_param.stream_in_param.a2dp.bt_inf_address  = bt_sink_srv_ami_get_bt_inf_address();
                open_param.stream_in_param.a2dp.clk_info_address   = hal_audio_query_rcdc_share_info();
#if 0
                open_param.stream_out_param.afe.audio_device    = HAL_AUDIO_DEVICE_DAC_DUAL;
                open_param.stream_out_param.afe.audio_interface = HAL_AUDIO_INTERFACE_1;
                open_param.stream_out_param.afe.stream_channel  = HAL_AUDIO_DIRECT;
#else /* #if 0 */
                hal_audio_get_stream_out_setting_config(AU_DSP_AUDIO, &open_param.stream_out_param.afe.audio_device, &open_param.stream_out_param.afe.stream_channel, &open_param.stream_out_param.afe.audio_interface);
                TASK_LOG_MSGID_I("[Rdebug] A2DP CODEC_AAC out_device(0x%x), channel(%d), interface(%d)", open_param.stream_out_param.afe.audio_device, open_param.stream_out_param.afe.stream_channel, open_param.stream_out_param.afe.audio_interface);
#endif /* #if 0 */
                open_param.stream_out_param.afe.memory          = HAL_AUDIO_MEM1;
                open_param.stream_out_param.afe.format          = AFE_PCM_FORMAT_S32_LE;
                open_param.stream_out_param.afe.stream_out_sampling_rate   = sampling_rate_enum_to_value(internal_handle->sample_rate);

                //open_param.stream_out_param.afe.stream_out_sampling_rate  = p_hdl_effect->get_output_samplingrate(internal_handle->sample_rate);
                //open_param.stream_out_param.afe.sampling_rate   = sampling_rate_enum_to_value(internal_handle->sample_rate);
                open_param.stream_out_param.afe.sampling_rate   = sampling_rate_enum_to_value(hal_audio_get_device_out_supported_frequency(open_param.stream_out_param.afe.audio_device, internal_handle->sample_rate));
                //open_param.stream_out_param.afe.sampling_rate   = hal_audio_get_device_out_supported_frequency(HAL_AUDIO_DEVICE_HEADSET,open_param.stream_out_param.afe.stream_out_sampling_rate)
                open_param.stream_out_param.afe.irq_period      = 8;
                open_param.stream_out_param.afe.frame_size      = 1024;
                open_param.stream_out_param.afe.frame_number    = 4;
                open_param.stream_out_param.afe.hw_gain         = true;
                if (open_param.stream_out_param.afe.audio_device == HAL_AUDIO_DEVICE_I2S_MASTER) {
                    if (open_param.stream_out_param.afe.sampling_rate > 48000) {
                        open_param.stream_out_param.afe.misc_parms      = I2S_CLK_SOURCE_APLL;
                    } else {
                        open_param.stream_out_param.afe.misc_parms      = I2S_CLK_SOURCE_DCXO;
                    }
                } else {
                    open_param.stream_out_param.afe.misc_parms      = DOWNLINK_PERFORMANCE_NORMAL;
                }
#if defined(MTK_EXTERNAL_DSP_NEED_SUPPORT)
                ami_set_afe_param(STREAM_OUT, internal_handle->sample_rate, true);
#endif /* #if defined(MTK_EXTERNAL_DSP_NEED_SUPPORT) */
                p_param_share = hal_audio_dsp_controller_put_paramter(&open_param, sizeof(mcu2dsp_open_param_t), AUDIO_MESSAGE_TYPE_BT_AUDIO_DL);
                hal_audio_dsp_dl_clkmux_control(AUDIO_MESSAGE_TYPE_BT_AUDIO_DL, open_param.stream_out_param.afe.audio_device, open_param.stream_out_param.afe.sampling_rate, TRUE);
                // Notify to do dynamic download. Use async wait.
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_BT_AUDIO_DL_OPEN, AUDIO_DSP_CODEC_TYPE_AAC, (uint32_t)p_param_share, false);
            }

#endif /* #ifndef MTK_PORTING_AB */

#ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG
            TASK_LOG_MSGID_I("[A2DP[AAC]Codec open");
#endif /* #ifdef BT_A2DP_BITSTREAM_DUMP_DEBUG */
        }
#endif /* #ifdef MTK_BT_A2DP_AAC_ENABLE */
#if defined(MTK_BT_A2DP_VENDOR_ENABLE)
        else if (internal_handle->codec_info.codec_cap.type == BT_A2DP_CODEC_VENDOR) {
            bt_codec_media_status_t result = BT_CODEC_MEDIA_STATUS_OK;
            handle->play    = bt_a2dp_sink_vendor_play;
            handle->stop    = bt_a2dp_sink_vendor_stop;
            handle->process = bt_a2dp_sink_vendor_process;
            handle->get_ts_ratio = bt_a2dp_sink_vendor_get_ts_ratio;
            handle->side_tone_enable = bt_side_tone_enable;
            handle->side_tone_disable = bt_side_tone_disable;
            result = bt_a2dp_sink_parse_vendor_info(internal_handle);
            if (BT_CODEC_MEDIA_STATUS_OK != result) {
                if (internal_handle != NULL) {
                    vPortFree(internal_handle);
                    internal_handle = NULL;
                }
                return 0;
            }
#ifndef MTK_PORTING_AB
            {
                mcu2dsp_open_param_t open_param;
                void *p_param_share;

                open_param.param.stream_in  = STREAM_IN_A2DP;
                open_param.param.stream_out = STREAM_OUT_AFE;

                memcpy(&open_param.stream_in_param.a2dp.codec_info, param, sizeof(bt_codec_a2dp_audio_t));
                open_param.stream_in_param.a2dp.p_share_info = hal_audio_query_bt_audio_dl_share_info();
#ifdef MTK_BT_A2DP_VENDOR_CODEC_BC_ENABLE
                hal_audio_reset_share_info(open_param.stream_in_param.a2dp.p_share_info);
                open_param.stream_in_param.a2dp.p_share_info->length = SHARE_BUFFER_BT_AUDIO_DL_SIZE;
#else /* #ifdef MTK_BT_A2DP_VENDOR_CODEC_BC_ENABLE */
                hal_audio_a2dp_reset_share_info(open_param.stream_in_param.a2dp.p_share_info);
#endif /* #ifdef MTK_BT_A2DP_VENDOR_CODEC_BC_ENABLE */
                open_param.stream_in_param.a2dp.p_asi_buf          = hal_audio_query_ltcs_asi_buf();
                open_param.stream_in_param.a2dp.p_min_gap_buf      = hal_audio_query_ltcs_min_gap_buf();
                open_param.stream_in_param.a2dp.p_current_bit_rate = hal_audio_report_bitrate_buf();
                open_param.stream_in_param.a2dp.p_lostnum_report   = hal_audio_report_lostnum_buf();
#ifdef MTK_BT_A2DP_VENDOR_CODEC_BC_ENABLE
                open_param.stream_in_param.a2dp.sink_latency    = 0;
#else /* #ifdef MTK_BT_A2DP_VENDOR_CODEC_BC_ENABLE */
                open_param.stream_in_param.a2dp.sink_latency       = bt_sink_srv_ami_get_a2dp_sink_latency();
#endif /* #ifdef MTK_BT_A2DP_VENDOR_CODEC_BC_ENABLE */
                open_param.stream_in_param.a2dp.bt_inf_address     = bt_sink_srv_ami_get_bt_inf_address();
                open_param.stream_in_param.a2dp.clk_info_address   = hal_audio_query_rcdc_share_info();
#if 0
                //[TEMP]: Add AT Cmd to switch I2S mode
                if (((*((volatile uint32_t *)(0xA2120B04)) >> 2) & 0x01) == 1) {
                    // I2S Mode
                    open_param.stream_out_param.afe.audio_device    = HAL_AUDIO_DEVICE_I2S_MASTER;
                    open_param.stream_out_param.afe.misc_parms      = I2S_CLK_SOURCE_APLL;
                } else {
                    open_param.stream_out_param.afe.audio_device    = HAL_AUDIO_DEVICE_DAC_DUAL;
                }

                open_param.stream_out_param.afe.audio_interface = HAL_AUDIO_INTERFACE_1;
                open_param.stream_out_param.afe.stream_channel  = HAL_AUDIO_DIRECT;
#else /* #if 0 */
                hal_audio_get_stream_out_setting_config(AU_DSP_AUDIO, &open_param.stream_out_param.afe.audio_device, &open_param.stream_out_param.afe.stream_channel, &open_param.stream_out_param.afe.audio_interface);

                TASK_LOG_MSGID_I("[Rdebug] A2DP CODEC_VENDOR out_device(0x%x), channel(%d), interface(%d)", open_param.stream_out_param.afe.audio_device, open_param.stream_out_param.afe.stream_channel, open_param.stream_out_param.afe.audio_interface);
#endif /* #if 0 */
                open_param.stream_out_param.afe.memory          = HAL_AUDIO_MEM1;
                open_param.stream_out_param.afe.format          = AFE_PCM_FORMAT_S32_LE;
                open_param.stream_out_param.afe.stream_out_sampling_rate   = sampling_rate_enum_to_value(internal_handle->sample_rate);
                //open_param.stream_out_param.afe.stream_out_sampling_rate  = p_hdl_effect->get_output_samplingrate(internal_handle->sample_rate);
                //open_param.stream_out_param.afe.sampling_rate   = sampling_rate_enum_to_value(internal_handle->sample_rate);
                open_param.stream_out_param.afe.sampling_rate   = sampling_rate_enum_to_value(hal_audio_get_device_out_supported_frequency(open_param.stream_out_param.afe.audio_device, internal_handle->sample_rate));
                //open_param.stream_out_param.afe.sampling_rate   = hal_audio_get_device_out_supported_frequency(HAL_AUDIO_DEVICE_HEADSET,open_param.stream_out_param.afe.stream_out_sampling_rate)
                open_param.stream_out_param.afe.irq_period      = 8;
                open_param.stream_out_param.afe.frame_size      = 1024;
                open_param.stream_out_param.afe.frame_number    = 4;
                open_param.stream_out_param.afe.hw_gain         = true;
                if (open_param.stream_out_param.afe.audio_device == HAL_AUDIO_DEVICE_I2S_MASTER) {
                    if (open_param.stream_out_param.afe.sampling_rate > 48000) {
                        open_param.stream_out_param.afe.misc_parms      = I2S_CLK_SOURCE_APLL;
                    } else {
                        open_param.stream_out_param.afe.misc_parms      = I2S_CLK_SOURCE_DCXO;
                    }
                } else {
                    open_param.stream_out_param.afe.misc_parms      = DOWNLINK_PERFORMANCE_NORMAL;
                }
#if defined(MTK_EXTERNAL_DSP_NEED_SUPPORT)
                ami_set_afe_param(STREAM_OUT, internal_handle->sample_rate, true);
#endif /* #if defined(MTK_EXTERNAL_DSP_NEED_SUPPORT) */
                p_param_share = hal_audio_dsp_controller_put_paramter(&open_param, sizeof(mcu2dsp_open_param_t), AUDIO_MESSAGE_TYPE_BT_AUDIO_DL);

                // Notify to do dynamic download. Use async wait.
                hal_audio_dsp_dl_clkmux_control(AUDIO_MESSAGE_TYPE_BT_AUDIO_DL, open_param.stream_out_param.afe.audio_device, open_param.stream_out_param.afe.sampling_rate, TRUE);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_BT_AUDIO_DL_OPEN, AUDIO_DSP_CODEC_TYPE_VENDOR, (uint32_t)p_param_share, false);
            }
#endif /* #ifndef MTK_PORTING_AB */
        }
#endif /* #if defined(MTK_BT_A2DP_VENDOR_ENABLE) */
        else {
            if (internal_handle != NULL) {
                vPortFree(internal_handle);
                internal_handle = NULL;
            }
            return 0;
        }
    } else {
#ifdef MTK_BT_A2DP_SOURCE_SUPPORT
        /* A2DP source role */
        if (internal_handle->codec_info.codec_cap.type == BT_A2DP_CODEC_SBC) {
            bt_codec_media_status_t result = BT_CODEC_MEDIA_STATUS_OK;
            handle->play    = bt_a2dp_source_sbc_play;
            handle->stop    = bt_a2dp_source_sbc_stop;
            handle->process = bt_a2dp_source_sbc_process;

            handle->query_memory_size  = bt_codec_a2dp_source_query_memory_size;
            handle->set_bit_rate       = bt_codec_a2dp_source_set_bit_rate;
            handle->get_payload        = bt_codec_a2dp_source_get_payload;
            handle->get_payload_done   = bt_codec_a2dp_source_get_payload_done;
            handle->query_payload_size = bt_codec_a2dp_source_query_payload_size;

            handle->side_tone_enable = bt_side_tone_enable;
            handle->side_tone_disable = bt_side_tone_disable;
            result = bt_a2dp_source_parse_sbc_info(internal_handle);
            if (BT_CODEC_MEDIA_STATUS_OK != result) {
                if (internal_handle != NULL) {
                    vPortFree(internal_handle);
                    internal_handle = NULL;
                }
                return 0;
            }
        } else {
            if (internal_handle != NULL) {
                vPortFree(internal_handle);
                internal_handle = NULL;
            }
            return 0;
        }
#endif /* #ifdef MTK_BT_A2DP_SOURCE_SUPPORT */
    }
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT)
    {
        int32_t result = bt_codec_a2dp_aws_open_setting(internal_handle);
        if (result < 0) {
            TASK_LOG_MSGID_I("[A2DP][AWS]alloc fail, result = %d\r\n", (int)result);
            if (internal_handle != NULL) {
                vPortFree(internal_handle);
                internal_handle = NULL;
            }
            return 0;
        }
    }
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT) */
    handle->state = BT_CODEC_STATE_READY;
    return handle;
}

bt_codec_media_status_t bt_codec_a2dp_close(bt_media_handle_t *handle)
{
    bt_a2dp_audio_internal_handle_t *internal_handle = (bt_a2dp_audio_internal_handle_t *) handle;
    linear_buffer_information_t *p_share_buffer = &internal_handle->share_buffer_info;
    linear_buffer_information_t *p_temp_buffer = &internal_handle->temp_buffer_info;
    linear_buffer_16bit_information_t *p_pcm_buffer = &internal_handle->pcm_buffer_info;

    TASK_LOG_MSGID_I("[A2DP]Close codec\n");
    if (handle->state != BT_CODEC_STATE_STOP && handle->state != BT_CODEC_STATE_READY) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    handle->state = BT_CODEC_STATE_IDLE;

#if defined(MTK_AVM_DIRECT)
#ifndef MTK_PORTING_AB
    // Notify to stop
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_BT_AUDIO_DL_CLOSE, 0, 0, true);
#endif
#else /* #if defined(MTK_AVM_DIRECT) */
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_codec_a2dp_aws_close_setting(internal_handle);
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    audio_free_id(handle->audio_id);
#endif /* #if defined(MTK_AVM_DIRECT) */
    if (internal_handle != NULL) {
        /* free working buffer1 */
        if (internal_handle->working_buffer1) {
            vPortFree(internal_handle->working_buffer1);
            internal_handle->working_buffer1 = NULL;
        }
        /* free working buffer2 */
        if (internal_handle->working_buffer2) {
            vPortFree(internal_handle->working_buffer2);
            internal_handle->working_buffer2 = NULL;
        }
        /* free share buffer */
        if (p_share_buffer->buffer_base_pointer) {
            vPortFree(p_share_buffer->buffer_base_pointer);
            p_share_buffer->buffer_base_pointer = NULL;
        }
        /* free temp buffer */
        if (p_temp_buffer->buffer_base_pointer) {
            vPortFree(p_temp_buffer->buffer_base_pointer);
            p_temp_buffer->buffer_base_pointer = NULL;
        }
        /* alloc pcm buffer */
        if (p_pcm_buffer->buffer_base_pointer) {
            vPortFree(p_pcm_buffer->buffer_base_pointer);
            p_pcm_buffer->buffer_base_pointer = NULL;
        }
        vPortFree(internal_handle);
        internal_handle = NULL;
    }
#ifndef MTK_PORTING_AB
    hal_audio_dsp_dl_clkmux_control(AUDIO_MESSAGE_TYPE_BT_AUDIO_DL, 0, 0, FALSE);
#endif /* #ifndef MTK_PORTING_AB */
    return BT_CODEC_MEDIA_STATUS_OK;
}

bt_codec_media_status_t bt_codec_a2dp_aws_set_clock_skew(bool flag)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
#if !defined(MTK_AVM_DIRECT)
    if (flag) {
        audio_service_aws_set_clock_skew(true);
    } else {
        audio_service_aws_set_clock_skew(false);
    }
#endif /* #if !defined(MTK_AVM_DIRECT) */
    return BT_CODEC_MEDIA_STATUS_OK;
#else /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}
bt_codec_media_status_t bt_codec_a2dp_aws_set_flag(bt_media_handle_t *handle, bool flag)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    p_info->aws_flag = flag;
    return BT_CODEC_MEDIA_STATUS_OK;
#else /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_set_initial_sync(bt_media_handle_t *handle)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
#if defined(MTK_AVM_DIRECT)
    return BT_CODEC_MEDIA_STATUS_OK;
#else /* #if defined(MTK_AVM_DIRECT) */
    bt_codec_media_status_t result = BT_CODEC_MEDIA_STATUS_OK;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    if (handle != NULL && p_info->aws_internal_flag) {
        p_info->aws_init_sync_flag = true;
        audio_service_aws_set_initial_sync();
    } else {
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return result;
#endif /* #if defined(MTK_AVM_DIRECT) */
#else /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_get_consumed_data_count(bt_media_handle_t *handle, bt_codec_a2dp_data_count_t *information)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
#if !defined(MTK_AVM_DIRECT)
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    information->sample_count  = audio_service_aws_get_accumulated_sample_count();
    information->sampling_rate = bt_codec_a2dp_aws_convert_sampling_rate_from_index_to_value(p_info->sample_rate);
#endif /* #if !defined(MTK_AVM_DIRECT) */
    return BT_CODEC_MEDIA_STATUS_OK;
#else /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_get_silence_frame_information(bt_media_handle_t *handle, bt_codec_a2dp_bitstream_t *information)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT)
    bt_codec_media_status_t result = BT_CODEC_MEDIA_STATUS_OK;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    bt_a2dp_codec_type_t codec_type = p_info->codec_info.codec_cap.type;
    uint32_t *sil_smpl_count = &information->sample_count;
    uint32_t *sil_byte_count = &information->byte_count;
    if (codec_type == BT_A2DP_CODEC_SBC) {
        bt_codec_sbc_t *p_codec_info = &p_info->codec_info.codec_cap.codec.sbc;
        sbc_set_silence_pattern_frame_sample(p_codec_info->block_length, p_codec_info->subband_num);
        audio_service_aws_get_silence_frame_information(AWS_CODEC_TYPE_SBC_FORMAT, sil_smpl_count, sil_byte_count);
    } else if (codec_type == BT_A2DP_CODEC_AAC) {
#ifdef MTK_BT_A2DP_AAC_ENABLE
        audio_service_aws_get_silence_frame_information(AWS_CODEC_TYPE_AAC_FORMAT, sil_smpl_count, sil_byte_count);
#else /* #ifdef MTK_BT_A2DP_AAC_ENABLE */
        information->sample_count = 0;
        information->byte_count = 0;
        result = BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #ifdef MTK_BT_A2DP_AAC_ENABLE */
    } else {
        information->sample_count = 0;
        information->byte_count = 0;
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return result;
#else /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_fill_silence_frame(bt_media_handle_t *handle, bt_codec_a2dp_buffer_t *data, uint32_t target_frm_cnt)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT)
    int32_t result;
    uint32_t *sil_frm_cnt = &target_frm_cnt;
    uint32_t *buf_byte_cnt = &data->byte_count;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    bt_a2dp_codec_type_t codec_type = p_info->codec_info.codec_cap.type;
    if (codec_type == BT_A2DP_CODEC_SBC) {
        bt_codec_sbc_t *p_codec_info = &p_info->codec_info.codec_cap.codec.sbc;
        sbc_set_silence_pattern_frame_sample(p_codec_info->block_length, p_codec_info->subband_num);
        result = audio_service_aws_fill_silence_frame(data->buffer, buf_byte_cnt, AWS_CODEC_TYPE_SBC_FORMAT, sil_frm_cnt);
        if (result == HAL_AUDIO_AWS_ERROR || result == HAL_AUDIO_AWS_NOT_SUPPORT) {
            return BT_CODEC_MEDIA_STATUS_ERROR;
        }
    } else if (codec_type == BT_A2DP_CODEC_AAC) {
#ifdef MTK_BT_A2DP_AAC_ENABLE
        result = audio_service_aws_fill_silence_frame(data->buffer, buf_byte_cnt, AWS_CODEC_TYPE_AAC_FORMAT, sil_frm_cnt);
        if (result == HAL_AUDIO_AWS_ERROR || result == HAL_AUDIO_AWS_NOT_SUPPORT) {
            return BT_CODEC_MEDIA_STATUS_ERROR;
        }
#else /* #ifdef MTK_BT_A2DP_AAC_ENABLE */
        return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #ifdef MTK_BT_A2DP_AAC_ENABLE */
    } else {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
#else /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_parse_data_information(bt_media_handle_t *handle, bt_codec_a2dp_buffer_t *data, bt_codec_a2dp_bitstream_t *information)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT)
    int32_t result;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    bt_a2dp_codec_type_t codec_type = p_info->codec_info.codec_cap.type;
    uint32_t *out_smpl_cnt = &information->sample_count;
    uint32_t *out_byte_cnt = &information->byte_count;
    if (codec_type == BT_A2DP_CODEC_SBC) {
        result = audio_service_aws_parse_bitstream_information(data->buffer, data->byte_count, AWS_CODEC_TYPE_SBC_FORMAT, out_smpl_cnt, out_byte_cnt);
        if (result == HAL_AUDIO_AWS_ERROR || result == HAL_AUDIO_AWS_NOT_SUPPORT) {
            return BT_CODEC_MEDIA_STATUS_ERROR;
        }
    } else if (codec_type == BT_A2DP_CODEC_AAC) {
#ifdef MTK_BT_A2DP_AAC_ENABLE
        result = audio_service_aws_parse_bitstream_information(data->buffer, data->byte_count, AWS_CODEC_TYPE_AAC_FORMAT, out_smpl_cnt, out_byte_cnt);
        if (result == HAL_AUDIO_AWS_ERROR || result == HAL_AUDIO_AWS_NOT_SUPPORT) {
            return BT_CODEC_MEDIA_STATUS_ERROR;
        }
#else /* #ifdef MTK_BT_A2DP_AAC_ENABLE */
        information->sample_count = 0;
        information->byte_count = 0;
        return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #ifdef MTK_BT_A2DP_AAC_ENABLE */
    } else {
        information->sample_count = 0;
        information->byte_count = 0;
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
#else /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_get_clock_skew_status(bt_media_handle_t *handle, bt_codec_aws_clock_skew_status_t *status)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT)
    aws_clock_skew_status_t hal_aws_status;
    hal_aws_status = audio_service_aws_get_clock_skew_status();
    if (hal_aws_status == AWS_CLOCK_SKEW_STATUS_IDLE) {
        *status = BT_CODEC_AWS_CLOCK_SKEW_STATUS_IDLE;
    } else if (hal_aws_status == AWS_CLOCK_SKEW_STATUS_BUSY) {
        *status = BT_CODEC_AWS_CLOCK_SKEW_STATUS_BUSY;
    }
    return BT_CODEC_MEDIA_STATUS_OK;
#else /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) && !defined(MTK_AVM_DIRECT) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_set_clock_skew_compensation_value(bt_media_handle_t *handle, int32_t sample_count)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
#if !defined(MTK_AVM_DIRECT)
    int result;
    result = audio_service_aws_set_clock_skew_compensation_value(sample_count);
    if (result < 0) {
        return BT_CODEC_MEDIA_STATUS_ERROR;
    }
#endif /* #if !defined(MTK_AVM_DIRECT) */
    return BT_CODEC_MEDIA_STATUS_OK;
#else /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}

bt_codec_media_status_t bt_codec_a2dp_aws_get_audio_latency(bt_media_handle_t *handle, uint32_t sampling_rate, uint32_t *p_latency_us)
{
#if defined(__BT_A2DP_CODEC_AWS_SUPPORT__)
    bt_codec_media_status_t result = BT_CODEC_MEDIA_STATUS_OK;
    bt_a2dp_audio_internal_handle_t *p_info = (bt_a2dp_audio_internal_handle_t *)handle;
    bt_a2dp_codec_type_t codec_type = p_info->codec_info.codec_cap.type;
    uint32_t latency_us;
    if (codec_type == BT_A2DP_CODEC_SBC) {
        switch (sampling_rate) {
            case 48000:
                latency_us = 4805;
                break;
            case 44100:
                latency_us = 4805;
                break;
            case 32000:
                latency_us = 4805;
                break;
            case 16000:
                latency_us = 4805;
                break;
            default:
                latency_us = 0;
                result = BT_CODEC_MEDIA_STATUS_ERROR;
                break;
        }
    } else if (codec_type == BT_A2DP_CODEC_AAC) {
#ifdef MTK_BT_A2DP_AAC_ENABLE
        switch (sampling_rate) {
            case 48000:
                latency_us = 8604;
                break;
            case 44100:
                latency_us = 8604;
                break;
            case 32000:
                latency_us = 8604;
                break;
            case 24000:
                latency_us = 8604;
                break;
            case 22050:
                latency_us = 8604;
                break;
            case 16000:
                latency_us = 8604;
                break;
            case 12000:
                latency_us = 8604;
                break;
            case 11025:
                latency_us = 8604;
                break;
            case  8000:
                latency_us = 8604;
                break;
            default:
                latency_us = 0;
                result = BT_CODEC_MEDIA_STATUS_ERROR;
                break;
        }
#else /* #ifdef MTK_BT_A2DP_AAC_ENABLE */
        latency_us = 0;
        result = BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #ifdef MTK_BT_A2DP_AAC_ENABLE */
    } else {
        latency_us = 0;
        result = BT_CODEC_MEDIA_STATUS_ERROR;
    }
    *p_latency_us = latency_us;
    return result;
#else /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
    return BT_CODEC_MEDIA_STATUS_ERROR;
#endif /* #if defined(__BT_A2DP_CODEC_AWS_SUPPORT__) */
}


void bt_codec_a2dp_set_sw_aac(bool flag)
{
    return;
}

bt_codec_media_status_t bt_codec_a2dp_set_sw_aac_flag(bool flag)
{
    return BT_CODEC_MEDIA_STATUS_OK;
}

