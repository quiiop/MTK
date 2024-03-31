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

#include <string.h>
#include <stdlib.h>
#include "hal_audio.h"

/*
  In this file, we implement the APIs listed in audio.h.
  If we need to communicate with DSP, it will call APIs provided by hal_audio_dsp_controller.c.
*/

#if defined(HAL_AUDIO_MODULE_ENABLED)

//==== Include header files ====
#include "hal_log.h"
#include "hal_ccni.h"
#include "hal_gpio.h"
#include "memory_attribute.h"
#include "hal_audio_cm4_dsp_message.h"
#include "hal_audio_message_struct.h"
#include "hal_audio_internal.h"
#include "hal_clock_platform_ab155x.h"
#include "hal_hw_semaphore.h"
#include "hal_resource_assignment.h"
#include "audio_test_utils.h"
#include "audio_messenger_ipi.h"
#include "audio_shared_info.h"
#include "FreeRTOS.h"
#include "task.h"

#define PLAYING_PROMPT_VOICE_ENABLE
#include "hal_audio_golden_table.h"

#ifdef HAL_DVFS_MODULE_ENABLED
#include "hal_dvfs.h"
#include "hal_dvfs_internal.h"
#endif /* #ifdef HAL_DVFS_MODULE_ENABLED */

#if defined(HAL_AUDIO_SUPPORT_APLL)
extern  uint8_t clock_set_pll_on(clock_pll_id pll_id);
extern  uint8_t clock_set_pll_off(clock_pll_id pll_id);
extern void platform_assert(const char *expr, const char *file, int line);
static int16_t aud_apll_1_cntr;
static int16_t aud_apll_2_cntr;
static hal_audio_mclk_status_t mclk_status[4]; // 4 is number of I2S interfaces.
#endif /* #if defined(HAL_AUDIO_SUPPORT_APLL) */

//==== Static variables ====
uint16_t g_stream_in_sample_rate = 16000;
uint16_t g_stream_in_code_type   = AUDIO_DSP_CODEC_TYPE_PCM;//modify for opus
encoder_bitrate_t g_bit_rate = ENCODER_BITRATE_32KBPS;

#ifdef PLAYING_PROMPT_VOICE_ENABLE
static bool is_vp_playing = false;
#endif /* #ifdef PLAYING_PROMPT_VOICE_ENABLE */
audio_common_t audio_common;
#ifdef HAL_AUDIO_SUPPORT_MULTIPLE_STREAM_OUT
HAL_AUDIO_CHANNEL_SELECT_t audio_Channel_Select;
HAL_DSP_PARA_AU_AFE_CTRL_t audio_nvdm_HW_config;
#endif /* #ifdef HAL_AUDIO_SUPPORT_MULTIPLE_STREAM_OUT */
#define HAL_AUDIO_MAX_OUTPUT_SAMPLING_FREQUENCY 192000
#define HAL_AUDIO_SAMPLING_RATE_MAX HAL_AUDIO_SAMPLING_RATE_192KHZ
//static int default_audio_device_out     = HAL_AUDIO_DEVICE_DAC_DUAL;
//static int default_audio_device_in      = HAL_AUDIO_DEVICE_MAIN_MIC_L;
const char supported_SR_audio_adc_in[HAL_AUDIO_SAMPLING_RATE_MAX + 1] = {
    false,  //HAL_AUDIO_SAMPLING_RATE_8KHZ      = 0, /**< 8000Hz  */
    false,  //HAL_AUDIO_SAMPLING_RATE_11_025KHZ = 1, /**< 11025Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_12KHZ     = 2, /**< 12000Hz */
    true,   //HAL_AUDIO_SAMPLING_RATE_16KHZ     = 3, /**< 16000Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_22_05KHZ  = 4, /**< 22050Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_24KHZ     = 5, /**< 24000Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_32KHZ     = 6, /**< 32000Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_44_1KHZ   = 7, /**< 44100Hz */
    true,   //HAL_AUDIO_SAMPLING_RATE_48KHZ     = 8, /**< 48000Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_88_2KHZ   = 9, /**< 88200Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_96KHZ     = 10,/**< 96000Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_176_4KHZ  = 11,/**< 176400Hz */
    false   //HAL_AUDIO_SAMPLING_RATE_192KHZ    = 12,/**< 192000Hz */
};

const char supported_SR_audio_dac_out[HAL_AUDIO_SAMPLING_RATE_MAX + 1] = {
    false,  //HAL_AUDIO_SAMPLING_RATE_8KHZ      = 0, /**< 8000Hz  */
    false,  //HAL_AUDIO_SAMPLING_RATE_11_025KHZ = 1, /**< 11025Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_12KHZ     = 2, /**< 12000Hz */
    true,   //HAL_AUDIO_SAMPLING_RATE_16KHZ     = 3, /**< 16000Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_22_05KHZ  = 4, /**< 22050Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_24KHZ     = 5, /**< 24000Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_32KHZ     = 6, /**< 32000Hz */
    true,   //HAL_AUDIO_SAMPLING_RATE_44_1KHZ   = 7, /**< 44100Hz */
    true,   //HAL_AUDIO_SAMPLING_RATE_48KHZ     = 8, /**< 48000Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_88_2KHZ   = 9, /**< 88200Hz */
    true,   //HAL_AUDIO_SAMPLING_RATE_96KHZ     = 10,/**< 96000Hz */
    false,  //HAL_AUDIO_SAMPLING_RATE_176_4KHZ  = 11,/**< 176400Hz */
    true    //HAL_AUDIO_SAMPLING_RATE_192KHZ    = 12,/**< 192000Hz */
};

const char supported_SR_audio_i2s_inout[HAL_AUDIO_SAMPLING_RATE_MAX + 1] = {
    true,  //HAL_AUDIO_SAMPLING_RATE_8KHZ      = 0, /**< 8000Hz  */
    true,  //HAL_AUDIO_SAMPLING_RATE_11_025KHZ = 1, /**< 11025Hz */
    true,  //HAL_AUDIO_SAMPLING_RATE_12KHZ     = 2, /**< 12000Hz */
    true,  //HAL_AUDIO_SAMPLING_RATE_16KHZ     = 3, /**< 16000Hz */
    true,  //HAL_AUDIO_SAMPLING_RATE_22_05KHZ  = 4, /**< 22050Hz */
    true,  //HAL_AUDIO_SAMPLING_RATE_24KHZ     = 5, /**< 24000Hz */
    true,  //HAL_AUDIO_SAMPLING_RATE_32KHZ     = 6, /**< 32000Hz */
    true,  //HAL_AUDIO_SAMPLING_RATE_44_1KHZ   = 7, /**< 44100Hz */
    true,  //HAL_AUDIO_SAMPLING_RATE_48KHZ     = 8, /**< 48000Hz */
    true,  //HAL_AUDIO_SAMPLING_RATE_88_2KHZ   = 9, /**< 88200Hz */
    true,  //HAL_AUDIO_SAMPLING_RATE_96KHZ     = 10,/**< 96000Hz */
    true,  //HAL_AUDIO_SAMPLING_RATE_176_4KHZ  = 11,/**< 176400Hz */
    true   //HAL_AUDIO_SAMPLING_RATE_192KHZ    = 12,/**< 192000Hz */
};

//==== Public API ====
hal_audio_status_t hal_audio_init(void)
{
    if (audio_common.init) {
        return HAL_AUDIO_STATUS_OK;
    }

    hal_audio_dsp_controller_init();

#if defined(HAL_AUDIO_SUPPORT_APLL)
    aud_apll_1_cntr = 0;
    aud_apll_2_cntr = 0;
    memset((void *)&mclk_status, 0, 4 * sizeof(hal_audio_mclk_status_t));
#endif /* #if defined(HAL_AUDIO_SUPPORT_APLL) */

    audio_common.init = true;

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_deinit(void)
{
    if (audio_common.init) {
        hal_audio_dsp_controller_deinit();
    }

    audio_common.init = false;

    return HAL_AUDIO_STATUS_OK;
}

/**
  * @ Register callback to copy the content of stream out
  * @ callback : callback function
  * @ user_data : user data (for exampple, handle)
  * @ Retval: HAL_AUDIO_STATUS_OK if operation is successful, others if operation is invalid
  */
hal_audio_status_t hal_audio_register_copied_stream_out_callback(hal_audio_stream_copy_callback_t callback, void *user_data)
{
    //KH: ToDo
    return HAL_AUDIO_STATUS_OK;
}

/**
  * @ Updates the audio output frequency
  * @ sample_rate : audio frequency used to play the audio stream
  * @ This API should be called before hal_audio_start_stream_out() to adjust the audio frequency
  * @ Retval: HAL_AUDIO_STATUS_OK if operation is successful, others if sample rate is invalid
  */
hal_audio_status_t hal_audio_set_stream_out_sampling_rate(hal_audio_sampling_rate_t sampling_rate)
{
    switch (sampling_rate) {
        case HAL_AUDIO_SAMPLING_RATE_8KHZ:
        case HAL_AUDIO_SAMPLING_RATE_11_025KHZ:
        case HAL_AUDIO_SAMPLING_RATE_12KHZ:
        case HAL_AUDIO_SAMPLING_RATE_16KHZ:
        case HAL_AUDIO_SAMPLING_RATE_22_05KHZ:
        case HAL_AUDIO_SAMPLING_RATE_24KHZ:
        case HAL_AUDIO_SAMPLING_RATE_32KHZ:
        case HAL_AUDIO_SAMPLING_RATE_44_1KHZ:
        case HAL_AUDIO_SAMPLING_RATE_48KHZ:
        case HAL_AUDIO_SAMPLING_RATE_88_2KHZ:
        case HAL_AUDIO_SAMPLING_RATE_96KHZ:
        case HAL_AUDIO_SAMPLING_RATE_176_4KHZ:
        case HAL_AUDIO_SAMPLING_RATE_192KHZ:
            audio_common.stream_out.stream_sampling_rate = sampling_rate;
            return HAL_AUDIO_STATUS_OK;
        default:
            return HAL_AUDIO_STATUS_INVALID_PARAMETER;
    }
}

/**
  * @ Updates the audio output channel number
  * @ channel_number : audio channel mode to play the audio stream
  * @ This API should be called before hal_audio_start_stream_out() to adjust the output channel number
  * @ Retval: HAL_AUDIO_STATUS_OK if operation success, others if channel number is invalid
  */
hal_audio_status_t hal_audio_set_stream_out_channel_number(hal_audio_channel_number_t channel_number)
{
    hal_audio_status_t result = HAL_AUDIO_STATUS_OK;
    switch (channel_number) {
        case HAL_AUDIO_MONO:
            audio_common.stream_out.stream_channel = HAL_AUDIO_MONO;
            break;
        case HAL_AUDIO_STEREO:
        case HAL_AUDIO_STEREO_BOTH_L_CHANNEL:
        case HAL_AUDIO_STEREO_BOTH_R_CHANNEL:
        case HAL_AUDIO_STEREO_BOTH_L_R_SWAP:
            audio_common.stream_out.stream_channel = HAL_AUDIO_STEREO;
            break;
        default:
            return HAL_AUDIO_STATUS_INVALID_PARAMETER;
    }
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SET_OUTPUT_DEVICE_CHANNEL, 0, channel_number, false);
    return result;
}

/**
  * @ Updates the audio output channel mode
  * @ channel_mode : audio channel mode to play the audio stream
  * @ This API should be called before hal_audio_start_stream_out() to adjust the output channel mode
  * @ Retval: HAL_AUDIO_STATUS_OK if operation is successful, others if channel mode is invalid
  */
hal_audio_status_t hal_audio_set_stream_out_channel_mode(hal_audio_channel_number_t channel_mode)
{
    hal_audio_status_t result = HAL_AUDIO_STATUS_OK;
    switch (channel_mode) {
        case HAL_AUDIO_MONO:
        case HAL_AUDIO_STEREO:
        case HAL_AUDIO_STEREO_BOTH_L_CHANNEL:
        case HAL_AUDIO_STEREO_BOTH_R_CHANNEL:
        case HAL_AUDIO_STEREO_BOTH_L_R_SWAP:
            audio_common.stream_out.stream_channel_mode = channel_mode;
            break;
        default:
            result = HAL_AUDIO_STATUS_INVALID_PARAMETER;
            break;
    }
    return result;
}

/**
  * @ Start the playback of audio stream
  */
hal_audio_status_t hal_audio_start_stream_out(hal_audio_active_type_t active_type)
{
    //ToDo: limit the scope -- treat it as local playback
    //audio_dsp_playback_info_t temp_param;
    void *p_param_share;
    bool is_running;

    audio_message_type_t msg_type = AUDIO_MESSAGE_TYPE_PLAYBACK;
    //n9_dsp_share_info_t *p_share_buf_info;

    is_running = hal_audio_status_query_running_flag(AUDIO_MESSAGE_TYPE_PLAYBACK);
    if (is_running) {
        // Reentry: don't allow multiple playback
        log_hal_msgid_info("Re-entry\r\n", 0);
    } else {
        hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_PLAYBACK, true);
    }

    // Open playback
    mcu2dsp_open_param_t open_param;

    // Collect parameters
    open_param.param.stream_in  = STREAM_IN_PLAYBACK;
    open_param.param.stream_out = STREAM_OUT_AFE;

    open_param.stream_in_param.playback.bit_type = HAL_AUDIO_BITS_PER_SAMPLING_16;
    open_param.stream_in_param.playback.sampling_rate = audio_common.stream_out.stream_sampling_rate;
    open_param.stream_in_param.playback.channel_number = audio_common.stream_out.stream_channel;
    open_param.stream_in_param.playback.codec_type = 0;  //KH: should use AUDIO_DSP_CODEC_TYPE_PCM
    open_param.stream_in_param.playback.p_share_info = (n9_dsp_share_info_t *)hal_audio_query_share_info(msg_type);

    hal_audio_reset_share_info(open_param.stream_in_param.playback.p_share_info);
#if 0
    open_param.stream_out_param.afe.audio_device    = HAL_AUDIO_DEVICE_DAC_DUAL;
    open_param.stream_out_param.afe.stream_channel  = HAL_AUDIO_DIRECT;
#else /* #if 0 */
    hal_audio_get_stream_out_setting_config(AU_DSP_AUDIO, &open_param.stream_out_param.afe.audio_device, &open_param.stream_out_param.afe.stream_channel, &open_param.stream_out_param.afe.audio_interface);
    if (open_param.stream_out_param.afe.audio_device == HAL_AUDIO_DEVICE_I2S_MASTER) {
        open_param.stream_out_param.afe.misc_parms      = I2S_CLK_SOURCE_DCXO;
    } else {
        open_param.stream_out_param.afe.misc_parms      = DOWNLINK_PERFORMANCE_NORMAL;
    }
#endif /* #if 0 */
    open_param.stream_out_param.afe.memory          = HAL_AUDIO_MEM1;
    open_param.stream_out_param.afe.format          = AFE_PCM_FORMAT_S16_LE;
    open_param.stream_out_param.afe.stream_out_sampling_rate   = 16000;
    open_param.stream_out_param.afe.sampling_rate   = 16000;
    open_param.stream_out_param.afe.irq_period      = 10;
    open_param.stream_out_param.afe.frame_size      = 256;
    open_param.stream_out_param.afe.frame_number    = 2;
    open_param.stream_out_param.afe.hw_gain         = false;
    p_param_share = hal_audio_dsp_controller_put_paramter(&open_param, sizeof(mcu2dsp_open_param_t), msg_type);

    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_PLAYBACK_OPEN, AUDIO_DSP_CODEC_TYPE_PCM, (uint32_t)p_param_share, true);

    // Start playback
    mcu2dsp_start_param_t start_param;

    // Collect parameters
    start_param.param.stream_in     = STREAM_IN_PLAYBACK;
    start_param.param.stream_out    = STREAM_OUT_AFE;

    start_param.stream_out_param.afe.aws_flag   =  false;

    p_param_share = hal_audio_dsp_controller_put_paramter(&start_param, sizeof(mcu2dsp_start_param_t), msg_type);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_PLAYBACK_START, 0, (uint32_t)p_param_share, true);

    return HAL_AUDIO_STATUS_OK;
}

/**
  * @ Stop the playback of audio stream
  */
void hal_audio_stop_stream_out(void)
{
    //ToDo: limit the scope -- treat it as local playback
    audio_message_type_t msg_type = AUDIO_MESSAGE_TYPE_PLAYBACK;
    n9_dsp_share_info_t *p_share_buf_info;

    // Stop playback
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_PLAYBACK_STOP, 0, 0, true);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_PLAYBACK_CLOSE, 0, 0, true);

    hal_audio_status_set_running_flag(msg_type, false);

    // Clear buffer
    p_share_buf_info = (n9_dsp_share_info_t *)hal_audio_query_share_info(msg_type);
    hal_audio_reset_share_info(p_share_buf_info);
}

/**
  * @ Updates the audio output volume
  * @ digital_volume_index: digital gain index
  * @ analog_volume_index : analog gain index
  */
static uint8_t g_mute_enabled = false;
hal_audio_status_t hal_audio_set_stream_out_volume(uint32_t digital_volume_index, uint32_t analog_volume_index)
{
#if 1 // TODO: Below is for A2DP, but will be controller by DSP in LE Audio case.
    log_hal_msgid_info("[Sink][AM] Digital: %04x, Analog: %04x", 2, digital_volume_index, analog_volume_index);

    uint32_t mode[1] = {1};
    uint32_t analog_gain[2] = {1, analog_volume_index};

    if (analog_volume_index == 0) {
        log_hal_msgid_info("set to mute", 0);
        g_mute_enabled = true;
        analog_gain[0] = 0;
    } else {
        // unlock mute
        if (g_mute_enabled == true) {
            uint32_t unlock_mute[2] = {0, 1};
            control_cset("Audio_Downlink_Vol_Mode", 1, mode);
            control_cset("Audio_Downlink_Vol", 2, unlock_mute);
            g_mute_enabled = false;
        }
    }

    control_cset("Audio_Downlink_Vol_Mode", 1, mode);
    control_cset("Audio_Downlink_Vol", 2, analog_gain);
    return HAL_AUDIO_STATUS_OK;
#else /* #if 1 // TODO: Below is for A2DP, but will be controller by DSP in LE Audio case. */
    uint32_t data32;

    audio_common.stream_out.digital_gain_index = digital_volume_index;
    audio_common.stream_out.analog_gain_index = analog_volume_index;

    data32 = (analog_volume_index << 16) | (digital_volume_index & 0xFFFF);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SET_OUTPUT_DEVICE_VOLUME, 0, data32, false);

    return HAL_AUDIO_STATUS_OK;
#endif /* #if 1 // TODO: Below is for A2DP, but will be controller by DSP in LE Audio case. */
}

hal_audio_status_t hal_audio_get_stream_out_volume(uint32_t *analog_volume_index)
{
    uint32_t analog_gain[2] = {0, 0};
    _auddrv_cget("Audio_Downlink_Vol", 2, analog_gain);
    if (analog_gain[0] == 1) {
        *analog_volume_index = analog_gain[1];
        return HAL_AUDIO_STATUS_OK;
    } else if (analog_gain[0] == 0 && analog_gain[1] == 0) {//mute case
        *analog_volume_index = analog_gain[1];
        return HAL_AUDIO_STATUS_OK;
    }

    return HAL_AUDIO_STATUS_ERROR;
}

/**
  * @ Updates the audio output DL2 volume
  * @ digital_volume_index: digital gain index
  * @ analog_volume_index : analog gain index
  */
hal_audio_status_t hal_audio_set_stream_out_dl2_volume(uint32_t digital_volume_index, uint32_t analog_volume_index)
{
    uint32_t data32;

    audio_common.stream_out_DL2.digital_gain_index = digital_volume_index;
    audio_common.stream_out_DL2.analog_gain_index = analog_volume_index;

    data32 = (analog_volume_index << 16) | (digital_volume_index & 0xFFFF);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SET_OUTPUT_DEVICE_VOLUME, 1, data32, false);

    return HAL_AUDIO_STATUS_OK;
}

#if defined(HAL_AUDIO_SUPPORT_MULTIPLE_STREAM_OUT)
/**
  * @ Mute stream ouput path
  * @ mute: true -> set mute / false -> set unmute
  * @ hw_gain_index: HAL_AUDIO_STREAM_OUT1-> indicate hw gain1 / HAL_AUDIO_STREAM_OUT2-> indicate hw gain2 / HAL_AUDIO_STREAM_OUT_ALL-> indicate hw gain1 and hw gain2
  */
void hal_audio_mute_stream_out(bool mute, hal_audio_hw_stream_out_index_t hw_gain_index)
{
    uint32_t data32;
    if (hw_gain_index == HAL_AUDIO_STREAM_OUT1) {
        audio_common.stream_out.mute = mute;
    } else if (hw_gain_index == HAL_AUDIO_STREAM_OUT2) {
        audio_common.stream_out_DL2.mute = mute;
    } else {
        audio_common.stream_out.mute = mute;
        audio_common.stream_out_DL2.mute = mute;
    }
    data32 = (hw_gain_index << 16) | (mute & 0xFFFF);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_MUTE_OUTPUT_DEVICE, 0, data32, false);
}
#else /* #if defined(HAL_AUDIO_SUPPORT_MULTIPLE_STREAM_OUT) */
/**
  * @ Mute stream ouput path
  * @ mute: true -> set mute / false -> set unmute
  */
void hal_audio_mute_stream_out(bool mute)
{
    audio_common.stream_out.mute = mute;

    //hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_MUTE_OUTPUT_DEVICE, 0, mute, false);
    hal_audio_set_stream_out_volume(0, 0);
}
#endif /* #if defined(HAL_AUDIO_SUPPORT_MULTIPLE_STREAM_OUT) */

/**
  * @ Control the audio output device
  * @ device: output device
  */
hal_audio_status_t hal_audio_set_stream_out_device(hal_audio_device_t device)
{
    audio_common.stream_out.audio_device = device;

    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SET_OUTPUT_DEVICE, 0, device, false);

    return HAL_AUDIO_STATUS_OK;
}

/**
  * @ Write data into audio output stream for playback.
  * @ buffer: Pointer to the buffer
  * @ size : number of audio data [in bytes]
  * @ Retval HAL_AUDIO_STATUS_OK if operation is successful, others if failed.
  */
hal_audio_status_t hal_audio_write_stream_out(const void *buffer, uint32_t size)
{
    //ToDo: limit the scope -- treat it as local playback
    hal_audio_status_t result;

    result = hal_audio_write_stream_out_by_type(AUDIO_MESSAGE_TYPE_PLAYBACK, buffer, size);

    return result;
}

/**
  * @ Query the free space of output stream.
  * @ sample_count : number of free space [in bytes]
  * @ Retval HAL_AUDIO_STATUS_OK if operation is successful, others if failed
  */
hal_audio_status_t hal_audio_get_stream_out_sample_count(uint32_t *sample_count)
{
    //ToDo: limit the scope -- treat it as local playback
    n9_dsp_share_info_t *p_info = hal_audio_query_playback_share_info();

    *sample_count = hal_audio_buf_mgm_get_free_byte_count(p_info);

    return HAL_AUDIO_STATUS_OK;
}

/**
  * @ Register the callback of stream out.
  * @ callback : callback function
  * @ user_data : pointer of user data
  * @ Retval HAL_AUDIO_STATUS_OK if operation is successful, others if failed
  */
hal_audio_status_t hal_audio_register_stream_out_callback(hal_audio_stream_out_callback_t callback, void *user_data)
{
    //ToDo: limit the scope -- treat it as local playback

    hal_audio_service_hook_callback(AUDIO_MESSAGE_TYPE_PLAYBACK, callback, user_data);

    return HAL_AUDIO_STATUS_OK;
}

/**
  * @ Updates the audio input frequency
  * @ sample_rate : audio frequency used to record the audio stream
  * @ This API should be called before hal_audio_start_stream_in() to adjust the audio frequency
  * @ Retval: HAL_AUDIO_STATUS_OK if operation is successful, others if sample rate is invalid
  */
hal_audio_status_t hal_audio_set_stream_in_sampling_rate(hal_audio_sampling_rate_t sampling_rate)
{
    //ToDo: extend the sampling rate from 8k/16kHz to 8k~48kHz

    switch (sampling_rate) {
        case HAL_AUDIO_SAMPLING_RATE_8KHZ:
        case HAL_AUDIO_SAMPLING_RATE_11_025KHZ:
        case HAL_AUDIO_SAMPLING_RATE_12KHZ:
        case HAL_AUDIO_SAMPLING_RATE_16KHZ:
        case HAL_AUDIO_SAMPLING_RATE_22_05KHZ:
        case HAL_AUDIO_SAMPLING_RATE_24KHZ:
        case HAL_AUDIO_SAMPLING_RATE_32KHZ:
        case HAL_AUDIO_SAMPLING_RATE_44_1KHZ:
        case HAL_AUDIO_SAMPLING_RATE_48KHZ:
            audio_common.stream_in.stream_sampling_rate = sampling_rate;
            return HAL_AUDIO_STATUS_OK;
        default:
            return HAL_AUDIO_STATUS_INVALID_PARAMETER;
    }
}

/**
  * @ Updates the audio input channel number
  * @ channel_number : audio channel mode to record the audio stream
  * @ This API should be called before hal_audio_start_stream_in() to adjust the input channel number
  * @ Retval: HAL_AUDIO_STATUS_OK if operation is successful, others if channel number is invalid
  */
hal_audio_status_t hal_audio_set_stream_in_channel_number(hal_audio_channel_number_t channel_number)
{
    switch (channel_number) {
        case HAL_AUDIO_MONO:
        case HAL_AUDIO_STEREO:
            audio_common.stream_in.stream_channel = channel_number;
            hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SET_INPUT_DEVICE_CHANNEL, 0, channel_number, false);
            return HAL_AUDIO_STATUS_OK;
        default:
            return HAL_AUDIO_STATUS_INVALID_PARAMETER;
    }
}

/**
  * @ Start the recording of audio stream
  */
hal_audio_status_t hal_audio_start_stream_in(hal_audio_active_type_t active_type)
{
    //ToDo: limit the scope -- treat it as recording
    //audio_dsp_playback_info_t temp_param;
    void *p_param_share;
    bool is_running;
    audio_message_type_t msg_type = AUDIO_MESSAGE_TYPE_RECORD;

    is_running = hal_audio_status_query_running_flag(AUDIO_MESSAGE_TYPE_RECORD);
    if (is_running) {
        // Re-entry: don't allow multiple recording
        //log_hal_msgid_info("Re-entry\r\n", 0);
    } else {
        // Temp protect in APP level    //TODO record middleware.
        //hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_RECORD, true);
    }

#if 0
    // Collect parameters
    temp_param.bit_type = HAL_AUDIO_BITS_PER_SAMPLING_16;
    temp_param.sampling_rate = audio_common.stream_in.stream_sampling_rate;
    temp_param.channel_number = audio_common.stream_in.stream_channel;
    temp_param.codec_type = 0;  //KH: should use AUDIO_DSP_CODEC_TYPE_PCM
    temp_param.p_share_info = hal_audio_query_record_share_info();

    // Open codec
    p_param_share = hal_audio_dsp_controller_put_paramter(&temp_param, sizeof(audio_dsp_playback_info_t));
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_RECORD_OPEN, AUDIO_DSP_CODEC_TYPE_PCM, (uint32_t)p_param_share, false);

    // Start recording
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_RECORD_START, 0, 0, true);
#else /* #if 0 */
    // Open playback
    mcu2dsp_open_param_t open_param;

    // Collect parameters
    open_param.param.stream_in  = STREAM_IN_AFE;
    open_param.param.stream_out = STREAM_OUT_RECORD;
#if 0
    open_param.stream_in_param.afe.audio_device    = HAL_AUDIO_DEVICE_MAIN_MIC_DUAL;
    open_param.stream_in_param.afe.stream_channel  = HAL_AUDIO_DIRECT;
    open_param.stream_in_param.afe.audio_interface = HAL_AUDIO_INTERFACE_1;
#else /* #if 0 */
    hal_audio_get_stream_in_setting_config(AU_DSP_VOICE, &open_param.stream_in_param.afe.audio_device, &open_param.stream_in_param.afe.stream_channel, &open_param.stream_in_param.afe.audio_interface);
    if (open_param.stream_in_param.afe.audio_device == HAL_AUDIO_DEVICE_I2S_MASTER) {
        open_param.stream_in_param.afe.misc_parms      = I2S_CLK_SOURCE_DCXO;
    } else {
        open_param.stream_in_param.afe.misc_parms      = MICBIAS_SOURCE_ALL | MICBIAS3V_OUTVOLTAGE_1p85v;
    }
#endif /* #if 0 */
    open_param.stream_in_param.afe.memory          = HAL_AUDIO_MEM1;
    open_param.stream_in_param.afe.format          = AFE_PCM_FORMAT_S16_LE;
    open_param.stream_in_param.afe.sampling_rate   = g_stream_in_sample_rate;
    open_param.stream_in_param.afe.irq_period      = 8;
    open_param.stream_in_param.afe.frame_size      = 256; // Warning: currently fixed @ 480 in DSP
    open_param.stream_in_param.afe.frame_number    = 2;
    open_param.stream_in_param.afe.hw_gain         = false;

    open_param.stream_out_param.record.p_share_info = (n9_dsp_share_info_t *)hal_audio_query_share_info(msg_type);
    open_param.stream_out_param.record.frames_per_message = 4; // DSP triggers CCNI message after collecting this value of frames
    open_param.stream_out_param.record.bitrate = g_bit_rate;
    hal_audio_reset_share_info(open_param.stream_out_param.record.p_share_info);

    p_param_share = hal_audio_dsp_controller_put_paramter(&open_param, sizeof(mcu2dsp_open_param_t), msg_type);

    //hal_audio_dsp_controller_send_message(MSG_MCU2DSP_RECORD_OPEN, AUDIO_DSP_CODEC_TYPE_PCM, (uint32_t)p_param_share, true);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_RECORD_OPEN, g_stream_in_code_type, (uint32_t)p_param_share, true);//modify for opus

    // Start playback
    mcu2dsp_start_param_t start_param;

    // Collect parameters
    start_param.param.stream_in     = STREAM_IN_AFE;
    start_param.param.stream_out    = STREAM_OUT_RECORD;

    start_param.stream_in_param.afe.aws_flag   =  false;

    p_param_share = hal_audio_dsp_controller_put_paramter(&start_param, sizeof(mcu2dsp_open_param_t), msg_type);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_RECORD_START, 0, (uint32_t)p_param_share, true);
#endif /* #if 0 */

    return HAL_AUDIO_STATUS_OK;
}

/**
  * @ Stop the recording of audio stream
  */
void hal_audio_stop_stream_in(void)
{
    //ToDo: limit the scope -- treat it as recording
    if (hal_audio_status_query_running_flag(AUDIO_MESSAGE_TYPE_RECORD)) {
        // Stop recording
        hal_audio_dsp_controller_send_message(MSG_MCU2DSP_RECORD_STOP, 0, 0, true);
        hal_audio_dsp_controller_send_message(MSG_MCU2DSP_RECORD_CLOSE, 0, 0, true);

        // Temp protect in APP level    //TODO record middleware.
        //hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_RECORD, false);
    } else {
        log_hal_msgid_info("Recording was not existed.", 0);
    }
}

/**
  * @ Updates the audio input volume
  * @ digital_volume_index: digital gain index
  * @ analog_volume_index : analog gain index
  */
hal_audio_status_t hal_audio_set_stream_in_volume(uint32_t digital_volume_index, uint32_t analog_volume_index)

{
    uint32_t data32;

    audio_common.stream_in.digital_gain_index = digital_volume_index;
    audio_common.stream_in.analog_gain_index = analog_volume_index;

    data32 = (analog_volume_index << 16) | (digital_volume_index & 0xFFFF);
    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SET_INPUT_DEVICE_VOLUME, 0, data32, false);

    return HAL_AUDIO_STATUS_OK;
}

/**
  * @ Mute stream in path
  * @ mute: true -> set mute / false -> set unmute
  */
void hal_audio_mute_stream_in(bool mute)
{
    audio_common.stream_in.mute = mute;

    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_MUTE_INPUT_DEVICE, 0, mute, false);
}

/**
  * @ Control the audio input device
  * @ device: input device
  */
hal_audio_status_t hal_audio_set_stream_in_device(hal_audio_device_t device)
{
    audio_common.stream_in.audio_device = device;

    hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SET_INPUT_DEVICE, 0, device, false);

    return HAL_AUDIO_STATUS_OK;
}


/**
  * @ Start audio recording
  * @ buffer: buffer pointer for the recorded data storing
  * @ size : number of audio data
  * @ Retval HAL_AUDIO_STATUS_OK if operation is successful, others if failed.
  */
hal_audio_status_t hal_audio_read_stream_in(void *buffer, uint32_t size)
{
    //ToDo: limit the scope -- treat it as recording
    n9_dsp_share_info_t *p_info = hal_audio_query_record_share_info();
    uint32_t data_byte_count;
    hal_audio_status_t result = HAL_AUDIO_STATUS_OK;
    uint32_t i;
    uint8_t *p_dest_buf = (uint8_t *)buffer;
    bool is_notify;

    // Check buffer
    if (buffer == NULL)
        return HAL_AUDIO_STATUS_ERROR;

    // Check data amount
    data_byte_count = hal_audio_buf_mgm_get_data_byte_count(p_info);
    if (size > data_byte_count) {
        return HAL_AUDIO_STATUS_ERROR;
    }

    // When buffer is enough
    for (i = 0; (i < 2) && size; i++) {
        uint8_t *p_source_buf;
        uint32_t buf_size, segment;

        hal_audio_buf_mgm_get_data_buffer(p_info, &p_source_buf, &buf_size);
        if (size >= buf_size) {
            segment = buf_size;
        } else {
            segment = size;
        }
        memcpy(p_dest_buf, p_source_buf, segment);
        hal_audio_buf_mgm_get_read_data_done(p_info, segment);
        p_dest_buf += segment;
        size -= segment;
    }

    if (p_info->bBufferIsFull) {
        p_info->bBufferIsFull = 0;
    }

    // Check status and notify DSP
    is_notify = hal_audio_status_query_notify_flag(AUDIO_MESSAGE_TYPE_RECORD);
    if (is_notify) {
        hal_audio_status_set_notify_flag(AUDIO_MESSAGE_TYPE_RECORD, false);
        hal_audio_dsp_controller_send_message(MSG_MCU2DSP_RECORD_DATA_NOTIFY_ACK, 0, 0, false);
    }

    return result;
}

/**
  * @ Query the data amount of input stream.
  * @ sample_count : number of data [in bytes]
  * @ Retval HAL_AUDIO_STATUS_OK if operation is successful, others if failed
  */
hal_audio_status_t hal_audio_get_stream_in_sample_count(uint32_t *sample_count)
{
    //ToDo: limit the scope -- treat it as recording
    n9_dsp_share_info_t *p_info = hal_audio_query_record_share_info();

    *sample_count = hal_audio_buf_mgm_get_data_byte_count(p_info);

    return HAL_AUDIO_STATUS_OK;
}

/**
  * @ Register the callback of stream in.
  * @ callback : callback function
  * @ user_data : pointer of user data
  * @ Retval HAL_AUDIO_STATUS_OK if operation is successful, others if failed
  */
hal_audio_status_t hal_audio_register_stream_in_callback(hal_audio_stream_in_callback_t callback, void *user_data)
{
    //ToDo: limit the scope -- treat it as recording

    hal_audio_service_hook_callback(AUDIO_MESSAGE_TYPE_RECORD, callback, user_data);

    return HAL_AUDIO_STATUS_OK;
}

/**
 * @ Query the size of needed memory to be allocated for internal use in audio driver
 * @ memory_size : the amount of memory required by the audio driver for an internal use (in bytes).
 * @ Retval HAL_AUDIO_STATUS_OK if operation is successful, others if failed.
 */
hal_audio_status_t hal_audio_get_memory_size(uint32_t *memory_size)
{
    //ToDo: assume that we don't ennd extra memory
    *memory_size = 0;

    return HAL_AUDIO_STATUS_OK;
}

/**
 * @ Hand over allocated memory to audio driver
 * @ memory : the pointer to an allocated memory. It should be 4 bytes aligned.
 * @ Retval HAL_AUDIO_STATUS_OK if operation is successful, others if failed.
 */
hal_audio_status_t hal_audio_set_memory(uint16_t *memory)
{
    audio_common.allocated_memory = memory;

    return HAL_AUDIO_STATUS_OK;
}

/**
 * @ Get audio clock.
 * @ sample_count : a pointer to the accumulated audio sample count.
 * @ Retval HAL_AUDIO_STATUS_OK if operation is successful, others if failed.
 */
hal_audio_status_t hal_audio_get_audio_clock(uint32_t *sample_count)
{
    //ToDo: currently, use fake function.
    *sample_count = 0;

    return HAL_AUDIO_STATUS_OK;
}
int32_t hal_audio_get_device_out_supported_frequency(hal_audio_device_t audio_out_device, hal_audio_sampling_rate_t freq)
{
    int32_t device_supported_frequency = -1;
    int32_t i = freq;

    switch (audio_out_device) {
        case HAL_AUDIO_DEVICE_DAC_L:
        case HAL_AUDIO_DEVICE_DAC_R:
        case HAL_AUDIO_DEVICE_DAC_DUAL:
            while (1) {
                if (i > HAL_AUDIO_SAMPLING_RATE_MAX) {
                    device_supported_frequency = -1;
                    break;
                }
                if (supported_SR_audio_dac_out[i] == true) {
                    device_supported_frequency = i;
                    break;
                }
                i++;
            }
            break;
        case HAL_AUDIO_DEVICE_I2S_MASTER:
        case HAL_AUDIO_DEVICE_I2S_SLAVE:
            while (1) {
                if (i > HAL_AUDIO_SAMPLING_RATE_MAX) {
                    device_supported_frequency = -1;
                    break;
                }
                if (supported_SR_audio_i2s_inout[i] == true) {
                    device_supported_frequency = i;
                    break;
                }
                i++;
            }
            break;
        default:
            break;
    }

    if (device_supported_frequency == -1) {
        switch (i - 1) {
            case HAL_AUDIO_SAMPLING_RATE_88_2KHZ:
                device_supported_frequency = HAL_AUDIO_SAMPLING_RATE_96KHZ;
                break;
            case HAL_AUDIO_SAMPLING_RATE_176_4KHZ:
                device_supported_frequency = HAL_AUDIO_SAMPLING_RATE_192KHZ;
                break;
            default:
                log_hal_msgid_warning("Not found AFE supported rate", 0);
                break;
        }

    }
    return device_supported_frequency;
}


#ifdef HAL_AUDIO_SUPPORT_MULTIPLE_STREAM_OUT
hal_audio_status_t hal_audio_get_stream_in_setting_config(audio_scenario_sel_t Audio_or_Voice, hal_audio_device_t *Device, hal_audio_channel_selection_t *MemInterface, hal_audio_interface_t *i2s_interface)
{
    hal_gpio_status_t status;
    hal_gpio_data_t channel_gpio_data = HAL_GPIO_DATA_LOW;
    uint8_t channel_temp;

    //Audio HW I/O Configure setting
    if (Audio_or_Voice) { //0:Audio, 1:Voice
        *i2s_interface = 1 << ((audio_nvdm_HW_config.Voice_InputDev & 0x30) >> 4);
        switch ((audio_nvdm_HW_config.Voice_InputDev & 0xC0) >> 6) {
            case 0x02: { //I2S_Master_In
                    *Device = HAL_AUDIO_DEVICE_I2S_MASTER;
                    break;
                }
            case 0x01: { //Digital Mic
                    if ((audio_nvdm_HW_config.Voice_InputDev & 0x03) == 0x02) {
                        *Device = HAL_AUDIO_DEVICE_DIGITAL_MIC_DUAL;
                    } else if ((audio_nvdm_HW_config.Voice_InputDev & 0x03) == 0x01) {
                        *Device = HAL_AUDIO_DEVICE_DIGITAL_MIC_R;
                    } else if ((audio_nvdm_HW_config.Voice_InputDev & 0x03) == 0x00) {
                        *Device = HAL_AUDIO_DEVICE_DIGITAL_MIC_L;
                    }
                    break;
                }
            case 0x00: { //Analog Mic
                    if (((audio_nvdm_HW_config.Voice_InputDev & 0x0C) >> 2) == 0x02) {
                        *Device = HAL_AUDIO_DEVICE_MAIN_MIC_DUAL;
                    } else if (((audio_nvdm_HW_config.Voice_InputDev & 0x0C) >> 2) == 0x01) {
                        *Device = HAL_AUDIO_DEVICE_MAIN_MIC_R;
                    } else if (((audio_nvdm_HW_config.Voice_InputDev & 0x0C) >> 2) == 0x00) {
                        *Device = HAL_AUDIO_DEVICE_MAIN_MIC_L;
                    }
                    break;
                }
            default:
                log_hal_msgid_info("Get Voice Stream in Device error. Defualt", 0);
                *Device = HAL_AUDIO_DEVICE_MAIN_MIC_L;
                break;
        }
    } else {
        log_hal_msgid_info("Get Audio Stream in Device error.", 0);
    }

    //Audio Channel selection setting
    if (audio_Channel_Select.modeForAudioChannel) {
        //HW_mode
        status = hal_gpio_get_input((hal_gpio_pin_t)audio_Channel_Select.hwAudioChannel.gpioIndex, &channel_gpio_data);
        if (status == HAL_GPIO_STATUS_OK) {
            if (channel_gpio_data == HAL_GPIO_DATA_HIGH) {
                channel_temp = (audio_Channel_Select.hwAudioChannel.audioChannelGPIOH & 0xF0) >> 4;
            } else {
                channel_temp = (audio_Channel_Select.hwAudioChannel.audioChannelGPIOL & 0xF0) >> 4;
            }
        } else {
            channel_temp = AU_DSP_CH_LR; //default.
            log_hal_msgid_info("Get Stream in channel setting false with HW_mode.", 0);
        }
    } else {
        channel_temp = (audio_Channel_Select.audioChannel & 0xF0) >> 4;
    }
    switch (channel_temp) {
        case AU_DSP_CH_LR: {
                *MemInterface = HAL_AUDIO_DIRECT;
                break;
            }
        case AU_DSP_CH_L: {
                *MemInterface = HAL_AUDIO_BOTH_L;
                break;
            }
        case AU_DSP_CH_R: {
                *MemInterface = HAL_AUDIO_BOTH_R;
                break;
            }
        case AU_DSP_CH_SWAP: {
                *MemInterface = HAL_AUDIO_SWAP_L_R;
                break;
            }
        case AU_DSP_CH_MIX: {
                *MemInterface = HAL_AUDIO_MIX_L_R;
                break;
            }
        case AU_DSP_CH_MIX_SHIFT: {
                *MemInterface = HAL_AUDIO_MIX_SHIFT_L_R;
                break;
            }
        default: {
                *MemInterface = HAL_AUDIO_DIRECT;
                break;
            }
    }
    //For debug
    //log_hal_msgid_info("Get Stream in channel(%d,) In_device(0x%x), In_I2S_Interface(0x%x)", 3, *MemInterface, *Device, *i2s_interface);
    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_get_stream_out_setting_config(audio_scenario_sel_t Audio_or_Voice, hal_audio_device_t *Device, hal_audio_channel_selection_t *MemInterface, hal_audio_interface_t *i2s_interface)
{
    hal_gpio_status_t status;
    hal_gpio_data_t channel_gpio_data = HAL_GPIO_DATA_LOW;
    uint8_t channel_temp;

    //Audio HW I/O Configure setting
    if (Audio_or_Voice) { //0:Audio, 1:Voice
        *i2s_interface = 1 << ((audio_nvdm_HW_config.Voice_OutputDev & 0x0F));
        switch ((audio_nvdm_HW_config.Voice_OutputDev & 0xF0) >> 4) {
            case 0x03: { //I2S_Master_Out
                    *Device = HAL_AUDIO_DEVICE_I2S_MASTER;
                    break;
                }
            case 0x02: { //Analog_SPK_Out_DUAL
                    *Device = HAL_AUDIO_DEVICE_DAC_DUAL;
                    break;
                }
            case 0x01: { //Analog_SPK_Out_R
                    *Device = HAL_AUDIO_DEVICE_DAC_R;
                    break;
                }
            case 0x00: { //Analog_SPK_Out_L
                    *Device = HAL_AUDIO_DEVICE_DAC_L;
                    break;
                }
            default:
                log_hal_msgid_info("Get Voice Stream out Device error. Defualt", 0);
                *Device = HAL_AUDIO_DEVICE_DAC_DUAL;
                break;
        }
    } else {
        *i2s_interface = 1 << ((audio_nvdm_HW_config.Audio_OutputDev & 0x0F));
        switch ((audio_nvdm_HW_config.Audio_OutputDev & 0xF0) >> 4) {
            case 0x03: { //I2S_Master_Out
                    *Device = HAL_AUDIO_DEVICE_I2S_MASTER;
                    break;
                }
            case 0x02: { //Analog_SPK_Out_DUAL
                    *Device = HAL_AUDIO_DEVICE_DAC_DUAL;
                    break;
                }
            case 0x01: { //Analog_SPK_Out_R
                    *Device = HAL_AUDIO_DEVICE_DAC_R;
                    break;
                }
            case 0x00: { //Analog_SPK_Out_L
                    *Device = HAL_AUDIO_DEVICE_DAC_L;
                    break;
                }
            default:
                log_hal_msgid_info("Get Voice Stream out Device error. Defualt", 0);
                *Device = HAL_AUDIO_DEVICE_DAC_DUAL;
                break;
        }

    }

    //Audio Channel selection setting
    if (audio_Channel_Select.modeForAudioChannel) {
        //HW_mode
        status = hal_gpio_get_input((hal_gpio_pin_t)audio_Channel_Select.hwAudioChannel.gpioIndex, &channel_gpio_data);
        if (status == HAL_GPIO_STATUS_OK) {
            if (channel_gpio_data == HAL_GPIO_DATA_HIGH) {
                channel_temp = audio_Channel_Select.hwAudioChannel.audioChannelGPIOH & 0x0F;
            } else {
                channel_temp = audio_Channel_Select.hwAudioChannel.audioChannelGPIOL & 0x0F;
            }
        } else {
            channel_temp = AU_DSP_CH_LR; //default.
            log_hal_msgid_info("Get Stream out channel setting false with HW_mode.", 0);
        }
    } else {
        channel_temp = audio_Channel_Select.audioChannel & 0x0F;
    }

    //Change to DSP SW Channel select
    switch (channel_temp) {
        case AU_DSP_CH_LR: {
                *MemInterface = HAL_AUDIO_DIRECT;
                break;
            }
#if 0
        case AU_DSP_CH_L: {
                *MemInterface = HAL_AUDIO_BOTH_L;
                break;
            }
        case AU_DSP_CH_R: {
                *MemInterface = HAL_AUDIO_BOTH_R;
                break;
            }
        case AU_DSP_CH_SWAP: {
                *MemInterface = HAL_AUDIO_SWAP_L_R;
                break;
            }
        case AU_DSP_CH_MIX: {
                *MemInterface = HAL_AUDIO_MIX_L_R;
                break;
            }
        case AU_DSP_CH_MIX_SHIFT: {
                *MemInterface = HAL_AUDIO_MIX_SHIFT_L_R;
                break;
            }
#endif /* #if 0 */
        default: {
                *MemInterface = HAL_AUDIO_DIRECT;
                break;
            }
    }
    //For debug
    //log_hal_msgid_info("Get Stream out channel(%d,) Out_device(0x%x), Out_I2S_Interface(0x%x)", 3, *MemInterface, *Device, *i2s_interface);
    return HAL_AUDIO_STATUS_OK;
}
#endif /* #ifdef HAL_AUDIO_SUPPORT_MULTIPLE_STREAM_OUT */

#if defined(HAL_AUDIO_SUPPORT_DEBUG_DUMP)
/**
  * @ Dump audio debug register
  */
void hal_audio_debug_dump(void)
{
    hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_AFE, true);
    log_hal_msgid_info("[BASIC]", 0);
    log_hal_msgid_info("[Audio Top Control Register 0]0x70000000 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000000)));
    log_hal_msgid_info("[Audio Top Control Register 1]0x70000004 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000004)));
    log_hal_msgid_info("[AFE Control Register 0]0x70000010 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000010)));
    log_hal_msgid_info("[AFE Control Register 1]0x70000014 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000014)));
    log_hal_msgid_info("[AFE Control Register 2]0x700002E0 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x700002E0)));

    log_hal_msgid_info("[Connection]", 0);
    log_hal_msgid_info("[AFE_CONN0]0x70000020 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000020)));
    log_hal_msgid_info("[AFE_CONN1]0x70000024 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000024)));
    log_hal_msgid_info("[AFE_CONN2]0x70000028 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000028)));
    log_hal_msgid_info("[AFE_CONN3]0x7000002c = 0x%x\r\n", 1, *((volatile uint32_t *)(0x7000002c)));
    log_hal_msgid_info("[AFE_CONN4]0x70000030 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000030)));
    log_hal_msgid_info("[AFE_CONN5]0x7000005c = 0x%x\r\n", 1, *((volatile uint32_t *)(0x7000005c)));
    log_hal_msgid_info("[AFE_CONN6]0x700000bc = 0x%x\r\n", 1, *((volatile uint32_t *)(0x700000bc)));
    log_hal_msgid_info("[AFE_CONN7]0x70000420 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000420)));
    log_hal_msgid_info("[AFE_CONN8]0x70000438 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000438)));
    log_hal_msgid_info("[AFE_CONN9]0x70000440 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000440)));
    log_hal_msgid_info("[AFE_CONN10]0x70000444 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000444)));
    log_hal_msgid_info("[AFE_CONN11]0x70000448 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000448)));
    log_hal_msgid_info("[AFE_CONN12]0x7000044c = 0x%x\r\n", 1, *((volatile uint32_t *)(0x7000044c)));
    log_hal_msgid_info("[AFE_CONN13]0x70000450 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000450)));
    log_hal_msgid_info("[AFE_CONN14]0x70000454 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000454)));
    log_hal_msgid_info("[AFE_CONN15]0x70000458 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000458)));
    log_hal_msgid_info("[AFE_CONN16]0x7000045c = 0x%x\r\n", 1, *((volatile uint32_t *)(0x7000045c)));
    log_hal_msgid_info("[AFE_CONN17]0x70000460 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000460)));
    log_hal_msgid_info("[AFE_CONN18]0x70000464 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000464)));
    log_hal_msgid_info("[AFE_CONN19]0x70000468 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000468)));
    log_hal_msgid_info("[AFE_CONN20]0x7000046c = 0x%x\r\n", 1, *((volatile uint32_t *)(0x7000046c)));
    log_hal_msgid_info("[AFE_CONN21]0x70000470 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000470)));
    log_hal_msgid_info("[AFE_CONN22]0x70000474 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000474)));
    log_hal_msgid_info("[AFE_CONN23]0x70000478 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000478)));

    log_hal_msgid_info("[Output data format]0x7000006c = 0x%x\r\n", 1, *((volatile uint32_t *)(0x7000006c)));

    log_hal_msgid_info("[HWSRC]", 0);
    log_hal_msgid_info("0x70000004 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000004)));
    log_hal_msgid_info("0x70001150 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70001150)));
    log_hal_msgid_info("0x70001170 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70001170)));
    log_hal_msgid_info("[HWSRC DL1]0x70001100 = 0x%x, ASRC_BUSY=%x\r\n", 2, *((volatile uint32_t *)(0x70001100)), (*((volatile uint32_t *)(0x70001100))) & (1 << 9));
    log_hal_msgid_info("[HWSRC DL1]0x70001100 = 0x%x, ASRC_EN=%x\r\n", 2, *((volatile uint32_t *)(0x70001100)), (*((volatile uint32_t *)(0x70001100))) & (1 << 8));
    log_hal_msgid_info("[HWSRC DL2]0x70001200 = 0x%x, ASRC_BUSY=%x\r\n", 2, *((volatile uint32_t *)(0x70001200)), (*((volatile uint32_t *)(0x70001200))) & (1 << 9));
    log_hal_msgid_info("[HWSRC DL2]0x70001200 = 0x%x, ASRC_EN=%x\r\n", 2, *((volatile uint32_t *)(0x70001200)), (*((volatile uint32_t *)(0x70001200))) & (1 << 8));

    log_hal_msgid_info("[I2S Master]", 0);
    log_hal_msgid_info("[I2S Master 0]0x70000860 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000860)));
    log_hal_msgid_info("[I2S Master 1]0x70000864 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000864)));
    log_hal_msgid_info("[I2S Master 2]0x70000868 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000868)));
    log_hal_msgid_info("[I2S Master 3]0x7000086c = 0x%x\r\n", 1, *((volatile uint32_t *)(0x7000086c)));
    log_hal_msgid_info("[I2S Master clock gating]0x70000004 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000004)));
    log_hal_msgid_info("[DSP APLL clock gating]", 0);
    log_hal_msgid_info("0x70000000 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000000)));
    log_hal_msgid_info("0x70000dd0 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000dd0)));

    log_hal_msgid_info("[Mic]", 0);
    log_hal_msgid_info("0xA2070108 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070108)));
    log_hal_msgid_info("0xA207010C = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA207010C)));
    log_hal_msgid_info("0xA2070224 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070224)));
    log_hal_msgid_info("0xA2070124 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070124)));
    log_hal_msgid_info("0xA2070130 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070130)));
    log_hal_msgid_info("0xA2070134 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070134)));
    log_hal_msgid_info("[L PGA GAIN]0xA2070100 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070100)));
    log_hal_msgid_info("[R PGA GAIN]0xA2070104 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070104)));
    log_hal_msgid_info("0x70000f98 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000f98)));
    log_hal_msgid_info("0xA207011C = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA207011C)));
    log_hal_msgid_info("0xA2070128 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070128)));
    log_hal_msgid_info("0xA207012C = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA207012C)));
    log_hal_msgid_info("[AFE_ADDA_TOP_CON0]0x70000120 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000120)));
    log_hal_msgid_info("[AFE_ADDA_UL_SRC_CON0]0x70000114 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000114)));
    log_hal_msgid_info("[AFE_ADDA2_UL_SRC_CON0]0x70000604 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000604)));
    log_hal_msgid_info("[AFE_ADDA6_UL_SRC_CON0]0x70000a84 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000a84)));
    log_hal_msgid_info("[AFE_ADDA_UL_DL_CON0]0x70000124 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000124)));

    log_hal_msgid_info("[DAC]", 0);
    log_hal_msgid_info("0x70000108 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000108)));
    log_hal_msgid_info("0x70000c50 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000c50)));
    log_hal_msgid_info("[AFE_ADDA_UL_DL_CON0]0x70000124 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000124)));
    log_hal_msgid_info("0xA2070200 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070200)));
    log_hal_msgid_info("0xA2070204 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070204)));
    log_hal_msgid_info("0xA2070208 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070208)));
    log_hal_msgid_info("0xA207020C = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA207020C)));
    log_hal_msgid_info("0xA2070210 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070210)));
    log_hal_msgid_info("0xA2070214 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070214)));
    log_hal_msgid_info("0xA207021C = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA207021C)));
    log_hal_msgid_info("0xA2070220 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070220)));
    log_hal_msgid_info("0xA2070224 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070224)));
    log_hal_msgid_info("0xA2070228 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070228)));
    log_hal_msgid_info("0xA207022C = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA207022C)));
    log_hal_msgid_info("0xA2070230 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070230)));
    log_hal_msgid_info("[AMP GAIN]0x70000f58 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000f58)));
    log_hal_msgid_info("0x70000f50 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000f50)));
    log_hal_msgid_info("0x70000ed0 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000ed0)));

    log_hal_msgid_info("[UL/DL and classg]", 0);
    log_hal_msgid_info("0x70000908 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000908)));
    log_hal_msgid_info("0x70000900 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000900)));
    log_hal_msgid_info("0x70000908 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000908)));
    log_hal_msgid_info("0x70000e6c = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000e6c)));
    log_hal_msgid_info("0x70000EE4 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000EE4)));
    log_hal_msgid_info("0x70000EE8 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000EE8)));
    log_hal_msgid_info("0x70000EEC = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000EEC)));
    log_hal_msgid_info("0x70000EE0 = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000EE0)));
    log_hal_msgid_info("0x70000EDC = 0x%x\r\n", 1, *((volatile uint32_t *)(0x70000EDC)));
    log_hal_msgid_info("0xA2070224 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2070224)));

    log_hal_msgid_info("[I2S Slave]", 0);
    log_hal_msgid_info("[I2S Slave 0]0xA0070038 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA0070038)));
    log_hal_msgid_info("[I2S Slave 0]0xA0070008 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA0070008)));
    log_hal_msgid_info("[I2S Slave 0]0xA0070030 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA0070030)));
    log_hal_msgid_info("[I2S Slave 1]0xA0080038 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA0080038)));
    log_hal_msgid_info("[I2S Slave 1]0xA0080008 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA0080008)));
    log_hal_msgid_info("[I2S Slave 1]0xA0080030 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA0080030)));
    log_hal_msgid_info("[I2S Slave 2]0xA0090038 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA0090038)));
    log_hal_msgid_info("[I2S Slave 2]0xA0090008 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA0090008)));
    log_hal_msgid_info("[I2S Slave 2]0xA0090030 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA0090030)));
    log_hal_msgid_info("[I2S Slave 3]0xA00A0038 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA00A0038)));
    log_hal_msgid_info("[I2S Slave 3]0xA00A0008 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA00A0008)));
    log_hal_msgid_info("[I2S Slave 3]0xA00A0030 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA00A0030)));

    log_hal_msgid_info("[APLL]", 0);
    log_hal_msgid_info("[APLL 1]0xA2050003 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050003)));
    log_hal_msgid_info("[APLL 1]0xA2050000 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050000)));
    log_hal_msgid_info("[APLL 1]0xA2050001 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050001)));
    log_hal_msgid_info("[APLL 1]0xA2050004 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050004)));
    log_hal_msgid_info("[APLL 1]0xA205002C = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA205002C)));

    log_hal_msgid_info("[APLL 2]0xA2050103 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050103)));
    log_hal_msgid_info("[APLL 2]0xA2050100 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050100)));
    log_hal_msgid_info("[APLL 2]0xA2050101 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050101)));
    log_hal_msgid_info("[APLL 2]0xA2050104 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050104)));
    log_hal_msgid_info("[APLL 2]0xA205012C = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA205012C)));

    log_hal_msgid_info("[APLL TUNER]", 0);
    log_hal_msgid_info("[APLL TUNER 1]0xA2050038 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050038)));
    log_hal_msgid_info("[APLL TUNER 1]0xA2050034 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050034)));
    log_hal_msgid_info("[APLL TUNER 2]0xA2050138 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050138)));
    log_hal_msgid_info("[APLL TUNER 2]0xA2050134 = 0x%x\r\n", 1, *((volatile uint32_t *)(0xA2050134)));
    hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_AFE, false);
}
#endif /* #if defined(HAL_AUDIO_SUPPORT_DEBUG_DUMP) */

#if defined(HAL_AUDIO_SUPPORT_APLL)

#define AFE_WRITE8(addr, val)   *((volatile uint8_t *)(addr)) = val
#define AFE_READ(addr)          *((volatile uint32_t *)(addr))
#define AFE_WRITE(addr, val)    *((volatile uint32_t *)(addr)) = val
#define AFE_SET_REG(addr, val, msk)  AFE_WRITE((addr), ((AFE_READ(addr) & (~(msk))) | ((val) & (msk))))
#define ReadREG(_addr)          (*(volatile uint32_t *)(_addr))

afe_apll_source_t afe_get_apll_by_samplerate(uint32_t samplerate)
{
    if (samplerate == 176400 || samplerate == 88200 || samplerate == 44100 || samplerate == 22050 || samplerate == 11025) {
        return AFE_APLL1;
    } else {
        return AFE_APLL2;
    }
}

void afe_set_aplltuner(bool enable, afe_apll_source_t apll)
{
    log_hal_msgid_info("DSP afe_set_apll_for_i2s_reg APLL:%d, enable:%d\r\n", 2, apll, enable);
    if (true == enable) {
        // Clear upper layer audio CG
        //AFE_SET_REG(0xA2020238, 0x01020000, 0xFFFFFFFF);//CKSYS_CLK_CFG_2 //Selects clk_aud_interface0_sel APLL2_CK, 49.152(MHz) and clk_aud_interface1_sel APLL1_CK, 45.1584(MHz)

        switch (apll) {
            case AFE_APLL1:
                //Open APLL1
                clock_mux_sel(CLK_AUD_INTERFACE1_SEL, 1);//Selects clk_aud_interface1_sel APLL1_CK, 45.1584(MHz)
                //Setting APLL1 Tuner
                AFE_SET_REG(APLL1_CTL14__F_RG_APLL1_LCDDS_TUNER_PCW_NCPO, 0x0DE517A9, 0xFFFFFFFF);//[31:24] is integer  number, [23:0] is fractional number
                //AFE_SET_REG(APLL1_CTL12__F_RG_APLL1_LCDDS_TUNER_PCW_NCPO, 0x00000100, 0xFFFFFFFF);//DDS SSC enable
                AFE_SET_REG(APLL1_CTL13__F_RG_APLL1_LCDDS_TUNER_EN, 0x00000001, 0xFFFFFFFF);//DDS PCW tuner enable
                break;
            case AFE_APLL2:
                //Open APLL2
                clock_mux_sel(CLK_AUD_INTERFACE0_SEL, 2);//Selects clk_aud_interface0_sel APLL2_CK, 49.152(MHz)
                //Setting APLL2 Tuner
                AFE_SET_REG(APLL2_CTL14__F_RG_APLL2_LCDDS_TUNER_PCW_NCPO, 0x0F1FAA4C, 0xFFFFFFFF);//[31:24] is integer  number, [23:0] is fractional number
                //AFE_SET_REG(APLL2_CTL12__F_RG_APLL2_LCDDS_TUNER_PCW_NCPO, 0x00000100, 0xFFFFFFFF);//DDS SSC enable
                AFE_SET_REG(APLL2_CTL13__F_RG_APLL2_LCDDS_TUNER_EN, 0x00000001, 0xFFFFFFFF);//DDS PCW tuner enable
                break;
            default:
                log_hal_msgid_warning("I2S Master not turn on APLL1 and APLL2", 0);
                break;
        }
    } else {
        uint32_t set_value = 0;
        switch (apll) {
            case AFE_APLL1:
                // Disable APLL1 Tuner
                set_value = ReadREG(APLL1_CTL15__F_RG_APLL1_LCDDS_PCW_NCPO);                //[31:24] is integer  number, [23:0] is fractional number
                AFE_SET_REG(APLL1_CTL10__F_RG_APLL1_LCDDS_PCW_NCPO, set_value, 0xFFFFFFFF); //[31:24] is integer  number, [23:0] is fractional number
                //AFE_WRITE8(APLL1_CTL12__F_RG_APLL1_LCDDS_TUNER_PCW_NCPO, 0);
                AFE_SET_REG(APLL1_CTL13__F_RG_APLL1_LCDDS_TUNER_EN, 0x00000000, 0xFFFFFFFF);//DDS PCW tuner disable
                break;
            case AFE_APLL2:
                // Disable APLL2 Tuner
                set_value = ReadREG(APLL2_CTL15__F_RG_APLL2_LCDDS_PCW_NCPO);                //[31:24] is integer  number, [23:0] is fractional number
                AFE_SET_REG(APLL2_CTL10__F_RG_APLL2_LCDDS_PCW_NCPO, set_value, 0xFFFFFFFF); //[31:24] is integer  number, [23:0] is fractional number
                //AFE_WRITE8(APLL2_CTL12__F_RG_APLL2_LCDDS_TUNER_PCW_NCPO, 0);
                AFE_SET_REG(APLL2_CTL13__F_RG_APLL2_LCDDS_TUNER_EN, 0x00000000, 0xFFFFFFFF);//DDS PCW tuner disable
                break;
            default:
                log_hal_msgid_warning("I2S Master not turn off APLL1 and APLL2", 0);
                break;
        }
    }
}

#define AUDIO_TOP_CON0                          0x70000000
#define AFE_APLL1_TUNER_CFG                     0x700003f0
#define AFE_APLL2_TUNER_CFG                     0x700003f4

#define AUDIO_TOP_CON0_PDN_APLL2_TUNER_POS      (18)
#define AUDIO_TOP_CON0_PDN_APLL2_TUNER_MASK     (1<<AUDIO_TOP_CON0_PDN_APLL2_TUNER_POS)
#define AUDIO_TOP_CON0_PDN_APLL_TUNER_POS       (19)
#define AUDIO_TOP_CON0_PDN_APLL_TUNER_MASK      (1<<AUDIO_TOP_CON0_PDN_APLL_TUNER_POS)

#define MAX_TIMES  10000

void afe_aplltuner_clock_on(bool enable, afe_apll_source_t apll)
{
    uint32_t take_times = 0;
    uint32_t mask;
    hal_nvic_save_and_set_interrupt_mask(&mask);

    while (++take_times) {
        if (HAL_HW_SEMAPHORE_STATUS_OK == hal_hw_semaphore_take(HW_SEMAPHORE_APLL)) {
            break;
        }
        if (take_times > MAX_TIMES) {
            hal_nvic_restore_interrupt_mask(mask);
            //error handling
            log_hal_msgid_info("[SEMAPHORE] CM4 take semaphore %d fail.", 1, HW_SEMAPHORE_APLL);
            platform_assert("[SEMAPHORE] CM4 take semaphore %d fail.", __func__, __LINE__);
            return;
        }
    }

    if (true == enable) {
        switch (apll) {
            case AFE_APLL1:
                AFE_SET_REG(AUDIO_TOP_CON0, 0 << AUDIO_TOP_CON0_PDN_APLL_TUNER_POS, AUDIO_TOP_CON0_PDN_APLL_TUNER_MASK); //PDN control for apll tuner
                break;
            case AFE_APLL2:
                AFE_SET_REG(AUDIO_TOP_CON0, 0 << AUDIO_TOP_CON0_PDN_APLL2_TUNER_POS, AUDIO_TOP_CON0_PDN_APLL2_TUNER_MASK);//PDN control for apll2 tuner
                break;
            default:
                break;
        }
    } else {
        switch (apll) {
            case AFE_APLL1:
                AFE_SET_REG(AUDIO_TOP_CON0, 1 << AUDIO_TOP_CON0_PDN_APLL_TUNER_POS, AUDIO_TOP_CON0_PDN_APLL_TUNER_MASK);
                break;
            case AFE_APLL2:
                AFE_SET_REG(AUDIO_TOP_CON0, 1 << AUDIO_TOP_CON0_PDN_APLL2_TUNER_POS, AUDIO_TOP_CON0_PDN_APLL2_TUNER_MASK);
                break;
            default:
                break;
        }
    }

    if (HAL_HW_SEMAPHORE_STATUS_OK == hal_hw_semaphore_give(HW_SEMAPHORE_APLL)) {
        hal_nvic_restore_interrupt_mask(mask);
    } else {
        hal_nvic_restore_interrupt_mask(mask);
        //error handling
        log_hal_msgid_info("[SEMAPHORE] CM4 give semaphore %d fail.", 1, HW_SEMAPHORE_APLL);
        platform_assert("[SEMAPHORE] CM4 give semaphore %d fail.", __func__, __LINE__);
    }
}

void afe_enable_apll_tuner(bool enable, afe_apll_source_t apll)
{
    if (true == enable) {
        switch (apll) {
            //Enable tuner
            case AFE_APLL1:
                AFE_SET_REG(AFE_APLL1_TUNER_CFG, 0x00000432, 0x0000FFF7);//AFE TUNER Control Register
                AFE_SET_REG(AFE_APLL1_TUNER_CFG, 0x1, 0x1);
                break;
            case AFE_APLL2:
                AFE_SET_REG(AFE_APLL2_TUNER_CFG, 0x00000434, 0x0000FFF7);//AFE TUNER2 Control Register
                AFE_SET_REG(AFE_APLL2_TUNER_CFG, 0x1, 0x1);
                break;
            default:
                break;
        }
    } else {
        switch (apll) {
            //Disable tuner
            case AFE_APLL1:
                AFE_SET_REG(AFE_APLL1_TUNER_CFG, 0x0, 0x1);
                break;
            case AFE_APLL2:
                AFE_SET_REG(AFE_APLL2_TUNER_CFG, 0x0, 0x1);
                break;
            default:
                break;
        }
    }
}

hal_audio_status_t hal_audio_apll_enable(bool enable, uint32_t samplerate)
{
    uint32_t mask;
    hal_nvic_save_and_set_interrupt_mask(&mask);
    hal_audio_status_t result = HAL_AUDIO_STATUS_OK;

    if (true == enable) {
        switch (afe_get_apll_by_samplerate(samplerate)) {
            case AFE_APLL1:
                aud_apll_1_cntr++;
                if (aud_apll_1_cntr == 1) {
#ifdef HAL_DVFS_MODULE_ENABLED
                    pmu_vcore_lock_control(PMU_NORMAL, PMIC_VCORE_1P1_V, PMU_LOCK);
                    log_hal_msgid_info("frequency is risen to 1.1V for open apll1", 0);
#endif /* #ifdef HAL_DVFS_MODULE_ENABLED */
                    log_hal_msgid_info("[APLL] TurnOnAPLL1, FS:%d, APLL1_CNT:%d", 2, samplerate, aud_apll_1_cntr);
                    clock_set_pll_on(CLK_APLL1);
                    afe_set_aplltuner(true, AFE_APLL1);
                    afe_aplltuner_clock_on(true, AFE_APLL1);
                    afe_enable_apll_tuner(true, AFE_APLL1);
                } else {
                    log_hal_msgid_info("[APLL] TurnOnAPLL1 again, FS:%d, APLL1_CNT:%d", 2, samplerate, aud_apll_1_cntr);
                }
                break;
            case AFE_APLL2:
                aud_apll_2_cntr++;
                if (aud_apll_2_cntr == 1) {
#ifdef HAL_DVFS_MODULE_ENABLED
                    pmu_vcore_lock_control(PMU_NORMAL, PMIC_VCORE_1P1_V, PMU_LOCK);
                    log_hal_msgid_info("frequency is risen to 1.1V for open apll2", 0);
#endif /* #ifdef HAL_DVFS_MODULE_ENABLED */
                    log_hal_msgid_info("[APLL] TurnOnAPLL2, FS:%d, APLL2_CNT:%d", 2, samplerate, aud_apll_2_cntr);
                    clock_set_pll_on(CLK_APLL2);
                    afe_set_aplltuner(true, AFE_APLL2);
                    afe_aplltuner_clock_on(true, AFE_APLL2);
                    afe_enable_apll_tuner(true, AFE_APLL2);
                } else {
                    log_hal_msgid_info("[APLL] TurnOnAPLL2 again, FS:%d, APLL2_CNT:%d", 2, samplerate, aud_apll_2_cntr);
                }
                break;
            default:
                result = HAL_AUDIO_STATUS_INVALID_PARAMETER;
                break;
        }
    } else {
        switch (afe_get_apll_by_samplerate(samplerate)) {
            case AFE_APLL1:
                aud_apll_1_cntr--;
                if (aud_apll_1_cntr == 0) {
                    afe_enable_apll_tuner(false, AFE_APLL1);
                    afe_aplltuner_clock_on(false, AFE_APLL1);
                    afe_set_aplltuner(false, AFE_APLL1);
                    clock_set_pll_off(CLK_APLL1);
                    log_hal_msgid_info("[APLL] TurnOffAPLL1, FS:%d, APLL1_CNT:%d", 2, samplerate, aud_apll_1_cntr);
#ifdef HAL_DVFS_MODULE_ENABLED
                    pmu_vcore_lock_control(PMU_NORMAL, PMIC_VCORE_1P1_V, PMU_UNLOCK);
                    log_hal_msgid_info("frequency is risen to 1.1V for close apll1", 0);
#endif /* #ifdef HAL_DVFS_MODULE_ENABLED */
                } else if (aud_apll_1_cntr < 0) {
                    log_hal_msgid_info("[APLL] Error, Already TurnOffAPLL1, FS:%d, APLL1_CNT:0", 1, samplerate);
                    aud_apll_1_cntr = 0;
                    result = HAL_AUDIO_STATUS_ERROR;
                } else {
                    log_hal_msgid_info("[APLL] TurnOffAPLL1 again, FS:%d, APLL1_CNT:%d", 2, samplerate, aud_apll_1_cntr);
                }
                break;
            case AFE_APLL2:
                aud_apll_2_cntr--;
                if (aud_apll_2_cntr == 0) {
                    afe_enable_apll_tuner(false, AFE_APLL2);
                    afe_aplltuner_clock_on(false, AFE_APLL2);
                    afe_set_aplltuner(false, AFE_APLL2);
                    clock_set_pll_off(CLK_APLL2);
                    log_hal_msgid_info("[APLL] TurnOffAPLL2, FS:%d, APLL2_CNT:%d", 2, samplerate, aud_apll_2_cntr);
#ifdef HAL_DVFS_MODULE_ENABLED
                    pmu_vcore_lock_control(PMU_NORMAL, PMIC_VCORE_1P1_V, PMU_UNLOCK);
                    log_hal_msgid_info("frequency is risen to 1.1V for close apll1", 0);
#endif /* #ifdef HAL_DVFS_MODULE_ENABLED */
                } else if (aud_apll_2_cntr < 0) {
                    log_hal_msgid_info("[APLL] Error, Already TurnOffAPLL2, FS:%d, APLL2_CNT:0", 1, samplerate);
                    aud_apll_2_cntr = 0;
                    result = HAL_AUDIO_STATUS_ERROR;
                } else {
                    log_hal_msgid_info("[APLL] TurnOffAPLL2 again, FS:%d, APLL2_CNT:%d", 2, samplerate, aud_apll_2_cntr);
                }
                break;
            default:
                result = HAL_AUDIO_STATUS_INVALID_PARAMETER;
                break;
        }
    }
    hal_nvic_restore_interrupt_mask(mask);
    return result;
}

hal_audio_status_t hal_audio_query_apll_status(void)
{
    log_hal_msgid_info("[APLL] aud_apll_1_cntr=%d", 1, aud_apll_1_cntr);
    log_hal_msgid_info("[APLL] aud_apll_2_cntr=%d", 1, aud_apll_2_cntr);
    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_mclk_enable(bool enable, afe_mclk_out_pin_t mclkoutpin, afe_apll_source_t apll, uint8_t divider)
{
    if (mclkoutpin != AFE_MCLK_PIN_FROM_I2S0 && mclkoutpin != AFE_MCLK_PIN_FROM_I2S1 && mclkoutpin != AFE_MCLK_PIN_FROM_I2S2 && mclkoutpin != AFE_MCLK_PIN_FROM_I2S3) {
        log_hal_msgid_info("[MCLK] not support mclkoutpin=%d", 1, mclkoutpin);
        return HAL_AUDIO_STATUS_INVALID_PARAMETER;
    }
    if (apll != AFE_APLL1 && apll != AFE_APLL2) {
        log_hal_msgid_info("[MCLK] not support apll=%d", 1, apll);
        return HAL_AUDIO_STATUS_INVALID_PARAMETER;
    }
    if (divider > 127) {
        log_hal_msgid_info("[MCLK] not support divider=%d", 1, divider);
        return HAL_AUDIO_STATUS_INVALID_PARAMETER;
    }

    if (enable) {
        if (mclk_status[mclkoutpin].status == MCLK_DISABLE) {
            ami_hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_AFE, true);
            uint32_t clock_divider_reg;
            uint32_t clock_divider_shift;
            bool toggled_bit = 0;

            if ((mclkoutpin == AFE_MCLK_PIN_FROM_I2S0) || (mclkoutpin == AFE_MCLK_PIN_FROM_I2S1)) {
                clock_divider_reg = 0xA2020308;
            } else {
                clock_divider_reg = 0xA202030C;
            }
            if ((mclkoutpin == AFE_MCLK_PIN_FROM_I2S0) || (mclkoutpin == AFE_MCLK_PIN_FROM_I2S2)) {
                clock_divider_shift = 0;
            } else {
                clock_divider_shift = 16;
            }

            switch (apll) {
                case AFE_APLL1:
                    hal_audio_apll_enable(true, 44100);
                    AFE_SET_REG(0xA2020304, 0, 0x3 << (8 * mclkoutpin));                // I2S0/1/2/3 clock_source from APLL1
                    mclk_status[mclkoutpin].apll = AFE_APLL1;
                    break;
                case AFE_APLL2:
                    hal_audio_apll_enable(true, 48000);
                    AFE_SET_REG(0xA2020304, 0x1 << (8 * mclkoutpin), 0x3 << (8 * mclkoutpin)); // I2S0/1/2/3 clock_source from APLL2
                    mclk_status[mclkoutpin].apll = AFE_APLL2;
                    break;
                default:
                    break;
            }

            // Setting audio clock divider   //Toggled to apply apll_ck_div bit-8 or bit-24
            //MCLK = clock_source/(1+n), n = [6:0], clock_source : AFE_I2S_SETTING_MCLK_SOURCE, n : AFE_I2S_SETTING_MCLK_DIVIDER)
            toggled_bit = (ReadREG(clock_divider_reg) & (0x00000100 << clock_divider_shift)) >> 8;
            if (toggled_bit == true) {
                AFE_SET_REG(clock_divider_reg, divider << clock_divider_shift, 0x17f << clock_divider_shift);
            } else {
                AFE_SET_REG(clock_divider_reg, (divider | 0x00000100) << clock_divider_shift, 0x17f << clock_divider_shift);
            }

            //Power on apll12_div0/1/2/3 divider
            AFE_SET_REG(0xA2020300, 0 << (8 * mclkoutpin), 1 << (8 * mclkoutpin));
            mclk_status[mclkoutpin].mclk_cntr++;
            mclk_status[mclkoutpin].divider = divider;
            mclk_status[mclkoutpin].status = MCLK_ENABLE;
            log_hal_msgid_info("[MCLK] TurnOnMCLK[%d], apll%d with divider%d, MCLK_CNT=%d", 4, mclkoutpin, apll, divider, mclk_status[mclkoutpin].mclk_cntr);
            return HAL_AUDIO_STATUS_OK;
        } else {
            if ((mclk_status[mclkoutpin].apll == apll) && (mclk_status[mclkoutpin].divider == divider)) {
                mclk_status[mclkoutpin].mclk_cntr++;
                log_hal_msgid_info("[MCLK] TurnOnMCLK[%d], apll%d with divider%d again, MCLK_CNT=%d", 4, mclkoutpin, apll, divider, mclk_status[mclkoutpin].mclk_cntr);
                return HAL_AUDIO_STATUS_OK;
            } else {
                log_hal_msgid_info("[MCLK] Error, Already TurnOnMCLK[%d] apll%d with divider%d, Request apll%d with divider%d is invalid", 5, mclkoutpin, mclk_status[mclkoutpin].apll, mclk_status[mclkoutpin].divider, apll, divider);
                return HAL_AUDIO_STATUS_ERROR;
            }
        }
    } else {
        if (mclk_status[mclkoutpin].status == MCLK_ENABLE) {
            if ((mclk_status[mclkoutpin].apll == apll) && (mclk_status[mclkoutpin].divider == divider)) {
                mclk_status[mclkoutpin].mclk_cntr--;

                if (mclk_status[mclkoutpin].mclk_cntr == 0) {
                    AFE_SET_REG(0xA2020300, 1 << (8 * mclkoutpin), 1 << (8 * mclkoutpin));
                    if (mclk_status[mclkoutpin].apll == AFE_APLL1) {
                        hal_audio_apll_enable(false, 44100);
                    } else {
                        hal_audio_apll_enable(false, 48000);
                    }
                    mclk_status[mclkoutpin].status = MCLK_DISABLE;
                    mclk_status[mclkoutpin].mclk_cntr = 0;
                    mclk_status[mclkoutpin].apll = AFE_APLL_NOUSE;
                    mclk_status[mclkoutpin].divider = 0;
                    log_hal_msgid_info("[MCLK] TurnOffMCLK[%d], apll%d with divider%d, MCLK_CNT=%d", 4, mclkoutpin, apll, divider, mclk_status[mclkoutpin].mclk_cntr);
                    if (mclk_status[AFE_MCLK_PIN_FROM_I2S0].mclk_cntr == 0 && mclk_status[AFE_MCLK_PIN_FROM_I2S1].mclk_cntr == 0 && mclk_status[AFE_MCLK_PIN_FROM_I2S2].mclk_cntr == 0 && mclk_status[AFE_MCLK_PIN_FROM_I2S3].mclk_cntr == 0) {
                        ami_hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_AFE, false);
                    }
                    return HAL_AUDIO_STATUS_OK;
                } else if (mclk_status[mclkoutpin].mclk_cntr < 0) {
                    log_hal_msgid_info("[MCLK] Error, Already TurnOffMCLK[%d], apll%d with divider%d, MCLK_CNT=%d", 4, mclkoutpin, apll, divider, mclk_status[mclkoutpin].mclk_cntr);
                    mclk_status[mclkoutpin].mclk_cntr = 0;
                    return HAL_AUDIO_STATUS_ERROR;
                } else {
                    log_hal_msgid_info("[MCLK] TurnOffMCLK[%d], apll%d with divider%d again, MCLK_CNT=%d", 4, mclkoutpin, apll, divider, mclk_status[mclkoutpin].mclk_cntr);
                    return HAL_AUDIO_STATUS_OK;
                }
            } else {
                log_hal_msgid_info("[MCLK] Error, Already TurnOnMCLK[%d] apll%d with divider%d, Request TurnOffMCLK apll%d with divider%d is invalid", 5, mclkoutpin, mclk_status[mclkoutpin].apll, mclk_status[mclkoutpin].divider, apll, divider);
                return HAL_AUDIO_STATUS_ERROR;
            }
        } else {
            log_hal_msgid_info("[MCLK] Already TurnOffMCLK[%d]", 1, mclkoutpin);
            return HAL_AUDIO_STATUS_ERROR;
        }
    }
}

hal_audio_status_t hal_audio_query_mclk_status(void)
{
    uint8_t i = 0;
    for (i = 0; i < 4; i++) {
        log_hal_msgid_info("[MCLK] mclk_status[%d].status=%d", 2, i, mclk_status[i].status);
        log_hal_msgid_info("[MCLK] mclk_status[%d].mclk_cntr=%d", 2, i, mclk_status[i].mclk_cntr);
        log_hal_msgid_info("[MCLK] mclk_status[%d].apll=%d", 2, i, mclk_status[i].apll);
        log_hal_msgid_info("[MCLK] mclk_status[%d].divider=%d", 2, i, mclk_status[i].divider);
    }
    return HAL_AUDIO_STATUS_OK;
}
#endif /* #if defined(HAL_AUDIO_SUPPORT_APLL) */

#ifdef MTK_PORTING_AB
uint32_t hal_audio_sampling_rate_enum_to_value(hal_audio_sampling_rate_t hal_audio_sampling_rate_enum)
{
    switch (hal_audio_sampling_rate_enum) {
        case HAL_AUDIO_SAMPLING_RATE_8KHZ:
            return 8000;
        case HAL_AUDIO_SAMPLING_RATE_11_025KHZ:
            return 11025;
        case HAL_AUDIO_SAMPLING_RATE_12KHZ:
            return 12000;
        case HAL_AUDIO_SAMPLING_RATE_16KHZ:
            return 16000;
        case HAL_AUDIO_SAMPLING_RATE_22_05KHZ:
            return 22050;
        case HAL_AUDIO_SAMPLING_RATE_24KHZ:
            return 24000;
        case HAL_AUDIO_SAMPLING_RATE_32KHZ:
            return 32000;
        case HAL_AUDIO_SAMPLING_RATE_44_1KHZ:
            return 44100;
        case HAL_AUDIO_SAMPLING_RATE_48KHZ:
            return 48000;
        case HAL_AUDIO_SAMPLING_RATE_88_2KHZ:
            return 88200;
        case HAL_AUDIO_SAMPLING_RATE_96KHZ:
            return 96000;
        case HAL_AUDIO_SAMPLING_RATE_176_4KHZ:
            return 176400;
        case HAL_AUDIO_SAMPLING_RATE_192KHZ:
            return 192000;
        default:
            return 8000;
    }
}

n9_dsp_share_info_t *hal_audio_query_ble_audio_sub_dl_share_info(void)
{
    return NULL;
}
#endif /* #ifdef MTK_PORTING_AB */

uint32_t volume_level_map[MAXIMUM_VOLUME_LEVEL] = {
    52,        //-80dB
    3308,      //-44dB
    6600,      //-38dB
    13170,     //-32dB
    26277,     //-26dB
    52429,     //-20dB
    66004,     //-18dB
    83094,     //-16dB
    104609,    //-14dB
    131695,    //-12dB
    165794,    //-10dB
    208723,    // -8dB
    262767,    // -6dB
    330803,    // -4dB
    416457,    // -2dB
    524288,    //  0dB
};

hal_audio_status_t hal_audio_stream_out_open(uint32_t sample_rate,
                                             uint32_t channels,
                                             uint32_t bit_type)
{

    int32_t ret;
    stream_params_t stream_parm;

    stream_parm.sample_rate = sample_rate;
    stream_parm.channels = channels;
    stream_parm.bit_type = bit_type;
    stream_parm.period_size = 10 * stream_parm.sample_rate / 1000;
    stream_parm.period_count = 4;
    stream_parm.format = 1 << 2; //MSD_PCM_FMT_S16_LE, fixed argument.

    log_hal_debug("sample_rate:%d", stream_parm.sample_rate);
    log_hal_debug("channels:%d", stream_parm.channels);
    log_hal_debug("bit_type:%d", stream_parm.bit_type);

    /* send ipi message to run playback open*/
    ret = audio_ipi_dsp_open(&stream_parm);
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    /* audio driver connect route, open*/
    ret = audio_drv_init(&stream_parm, false);
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_stream_out_close(void)
{
    int32_t ret;
    /* audio drvier drop, free, close and disconnect route*/
    ret = audio_drv_deinit(false);
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    /* ipi message to run dsp close */
    ret = audio_ipi_dsp_close();
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_stream_out_start(void)
{

    int32_t ret;

    ret = audio_ipi_dsp_start();
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_stream_out_stop(void)
{

    int32_t ret;

    /* ipi message to run dsp stop */
    ret = audio_ipi_dsp_stop();
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    return HAL_AUDIO_STATUS_OK;
}

#if 0
hal_audio_status_t hal_audio_stream_out_suspend(void)
{
    log_hal_info("%s", __FUNCTION__);

    int32_t ret;

    /* ipi message to run dsp suspend */
    ret = audio_ipi_dsp_suspend();
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_stream_out_resume(void)
{
    log_hal_info("%s", __FUNCTION__);

    int32_t ret;

    /* ipi message to run dsp resume */
    ret = audio_ipi_dsp_resume();
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    return HAL_AUDIO_STATUS_OK;
}
#endif /* #if 0 */

hal_audio_status_t hal_audio_stream_out_set_volume(uint16_t volume_level)
{
    int32_t ret;

    if (volume_level >= MAXIMUM_VOLUME_LEVEL) {
        log_hal_error("invalid volume level!");
        return HAL_AUDIO_STATUS_ERROR;
    }

    ret = audio_drv_set_volume(volume_level_map[volume_level]);
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    return HAL_AUDIO_STATUS_OK;
}

uint16_t hal_audio_stream_out_write(void *buffer, uint32_t data_size)
{
    return _copy_shared(buffer, data_size);
}

#ifdef PLAYING_PROMPT_VOICE_ENABLE
static void _hal_play_prompt_voice(void *param)
{
    uint8_t *data_ptr = (uint8_t *)hal_prompt_voice;
    uint32_t data_len = sizeof(hal_prompt_voice);
    uint32_t size_writting = 960;
    int32_t size_written = 0;
    uint32_t total_written = 0;

    if (HAL_AUDIO_STATUS_OK != hal_audio_stream_out_open(48000, 2, 16))
        goto vp_exit;
    hal_audio_stream_out_start();

    do  {
        size_written = hal_audio_stream_out_write(data_ptr + total_written, size_writting);
        if (size_written < 0) {
            log_hal_info("write error!");
            break;
        }

        total_written += size_written;

        if (total_written + size_writting > data_len)
            size_writting = data_len - total_written;

        vTaskDelay(pdMS_TO_TICKS(5));
    } while (total_written < data_len);

    hal_audio_stream_out_stop();
    hal_audio_stream_out_close();

vp_exit:
    is_vp_playing = false;
    vTaskDelete(NULL);
}

uint16_t hal_audio_play_prompt_voice(void)
{
    if (is_vp_playing) {
        log_hal_warning("prompt voice is playing!");
        return HAL_AUDIO_STATUS_ERROR;
    }
    if (pdPASS != xTaskCreate(_hal_play_prompt_voice, "vp_task", 4 * 1024 / sizeof(portSTACK_TYPE), NULL, 4, NULL)) {
        log_hal_error("play voice prompt err!");
        return HAL_AUDIO_STATUS_ERROR;
    }

    is_vp_playing = true;
    return HAL_AUDIO_STATUS_OK;
}
#endif /* #ifdef PLAYING_PROMPT_VOICE_ENABLE */

#ifdef CFG_AUDIO_DSP_LEAUDIO_EN
uint32_t _leaudio_get_sample_rate(SampleRate rate)
{
    uint32_t sample_rate = 0;
    switch (rate) {
        case LC3_RATE_8000: {
                sample_rate = 8000;
                break;
            }
        case LC3_RATE_16000: {
                sample_rate = 16000;
                break;
            }
        case LC3_RATE_24000: {
                sample_rate = 24000;
                break;
            }
        case LC3_RATE_32000: {
                sample_rate = 32000;
                break;
            }
        case LC3_RATE_44100: {
                sample_rate = 44100;
                break;
            }
        case LC3_RATE_48000: {
                sample_rate = 48000;
                break;
            }
        case LC3_RATE_88200: {
                sample_rate = 88200;
                break;
            }
        case LC3_RATE_96000: {
                sample_rate = 96000;
                break;
            }
        case LC3_RATE_176400: {
                sample_rate = 176400;
                break;
            }
        case LC3_RATE_192000: {
                sample_rate = 192000;
                break;
            }
        default:
            break;
    }
    return sample_rate;
}

uint32_t _leaudio_get_bitType(BitsPerSample bits)
{
    uint32_t bit_type = 0;
    switch (bits) {
        case LC3_BITS_16: {
                bit_type = 16;
                break;
            }
        case LC3_BITS_24: {
                bit_type = 24;
                break;

            }
        case LC3_BITS_32: {
                bit_type = 32;
                break;
            }
        default:
            break;
    }
    return bit_type;
}

#if defined CFG_LEA_DUMP_ENABLE && defined FILE_SYS_SUPPORT
#define HAL_LEA_DUMP_ENABLE
#endif /* #if defined CFG_LEA_DUMP_ENABLE && defined FILE_SYS_SUPPORT */

#ifdef HAL_LEA_DUMP_ENABLE
#include "mtk_hifixdsp_common.h"
#include "audio_task_manager.h"
#include "ff.h"
#include "task_def.h"
#include "semphr.h"

typedef struct {
    unsigned long start_addr;           // start address of N10-DSP share buffer
    volatile uint16_t length;           // total buffer length
    volatile bool is_buf_full;          // indicate if buffer is full
    volatile uint16_t read_offset;      // read pointer of N10-DSP share buffer
    volatile uint16_t write_offset;     // write pointer of N10-DSP share buffer
} lea_dsp_share_buf_info_t;

typedef struct {
    uint8_t *pBufBase;
    uint8_t *pBufEnd;
    uint8_t *pRead;
    uint8_t *pWrite;
    uint32_t bufLen;
    uint32_t datacount;
} ring_buf;

typedef struct {
    bool is_inited;
    unsigned long dsp_buf_info_base;
} lea_host_dump_param_t;

typedef struct {
    TaskHandle_t dump_tsk_hdl;
    FIL fid;
    lea_dsp_share_buf_info_t *dsp_share_buf;
    ring_buf lea_dump_buf;
    SemaphoreHandle_t dump_semphr;
    bool dump_waiting_data;
    bool dump_start;
    bool dump_ready;
    bool dsp_ready;
    uint32_t file_size;
} lea_debug_dump_t;

#define LEA_DUMP_SHARE_BUFFER_SIZE (4 * 1024)
#define LC3_DUMP_DATA_BUFFER_SIZE (30 * 1024)
#define LC3_DUMP_MAX_FILE_SIZE (10 * 1024 * 1024)
static lea_debug_dump_t g_lea_dbg = {0};

void *_convert_addr(unsigned long addr)
{
    unsigned long p_addr;
    void *v_addr;
    p_addr = adsp_hal_phys_addr_dsp2cpu(addr);
    v_addr = adsp_get_shared_sysram_phys2virt(p_addr);
    return v_addr;
}

void _lea_dump_write_file(void *buff, uint32_t bytes_count)
{
    FRESULT f_ret;
    UINT f_bw;
    FIL *fid = &g_lea_dbg.fid;

    if (bytes_count == 0)
        return;

    if (g_lea_dbg.file_size >= LC3_DUMP_MAX_FILE_SIZE) {
        f_ret = f_lseek(fid, 0);
        if (f_ret) {
            log_hal_error("f_lseek error:%d", f_ret);
        } else {
            log_hal_info("LEA dump file reset!");
            g_lea_dbg.file_size = 0;
        }
    }

    f_ret = f_write(fid, buff, bytes_count, &f_bw);
    if (f_ret || bytes_count != f_bw) {
        log_hal_error("f_write error! f_ret:%d, %u, %d", f_ret, f_bw, bytes_count);
        return;
    }
    g_lea_dbg.file_size += bytes_count;
    return;
}

void _lea_dump_sync_file(void)
{
    FRESULT f_ret;
    FIL *fid = &g_lea_dbg.fid;

    f_ret = f_sync(fid);
    if (f_ret) {
        log_hal_error("f_sync error! f_ret:%d", f_ret);
    }
}

int lea_init_ring_buf(ring_buf *buf, int size)
{
    if (buf == NULL) {
        log_hal_error("buf == NULL\n");
        return -1;
    } else if (size == 0) {
        log_hal_error("size == 0\n");
        return -1;
    }

    uint8_t *pbuf = (uint8_t *)pvPortMalloc(size);
    if (pbuf == NULL) {
        log_hal_error("malloc failed!");
        return -1;
    }
    buf->pBufBase = pbuf;
    buf->pBufEnd = buf->pBufBase + size;
    buf->pRead = pbuf;
    buf->pWrite = pbuf;
    buf->bufLen = size;
    buf->datacount = 0;
    return 0;
}

int lea_deinit_ring_buf(ring_buf *RingBuf1)
{
    if (RingBuf1 == NULL)
        return -1;

    if (RingBuf1->pBufBase)
        vPortFree(RingBuf1->pBufBase);

    RingBuf1->pBufBase = NULL;
    RingBuf1->pBufEnd = NULL;
    RingBuf1->pRead = NULL;
    RingBuf1->pWrite = NULL;
    RingBuf1->bufLen = 0;
    return 0;
}

void lea_RingBuf_receive_data(ring_buf *RingBuf1, uint8_t *buf, uint32_t count)
{
    uint32_t data_count = RingBuf1->datacount;
    uint8_t *pRead = RingBuf1->pRead;
    uint8_t *pWrite = RingBuf1->pWrite;

    uint32_t spaceIHave;
    spaceIHave = RingBuf1->bufLen - data_count;

    if (spaceIHave < count) {
        log_hal_error("not enough space, spaceIHave %d < count %d\n", spaceIHave, count);
        return;
    }

    if (pRead <= pWrite) {
        uint32_t w2e = RingBuf1->pBufEnd - pWrite;

        if (count <= w2e) {
            memcpy(pWrite, buf, count);
            pWrite += count;
            if (pWrite >= RingBuf1->pBufEnd)
                pWrite -= RingBuf1->bufLen;
        } else {
            memcpy(pWrite, buf, w2e);
            memcpy(RingBuf1->pBufBase, buf + w2e, count - w2e);
            pWrite = RingBuf1->pBufBase + count - w2e;
        }
    } else {
        memcpy(pWrite, buf, count);
        pWrite += count;
        if (pWrite >= RingBuf1->pBufEnd)
            pWrite -= RingBuf1->bufLen;
    }

    RingBuf1->pWrite = pWrite;
    RingBuf1->datacount += count;
}

void _lea_receive_data_from_dsp(void)
{
    uint16_t data_count;
    uint16_t unwrap_size;
    ring_buf *dump_rbuf = &g_lea_dbg.lea_dump_buf;
    lea_dsp_share_buf_info_t *share_buf = g_lea_dbg.dsp_share_buf;
    uint16_t write_offset = share_buf->write_offset;
    uint8_t *start_addr = (uint8_t *)_convert_addr(share_buf->start_addr);
    bool need_write = true;

    data_count = (share_buf->length - share_buf->read_offset + write_offset) % share_buf->length;
    if (write_offset == share_buf->read_offset) {
        if (share_buf->is_buf_full) {
            data_count = share_buf->length;
        } else {
            data_count = 0;
            need_write = false;
        }
    }
    if (data_count > LEA_DUMP_SHARE_BUFFER_SIZE) {
        need_write = false;
    }
#if 0
    log_hal_info("dump lea, dc:%04d, wf:%04d, rf:%04d, if:%d, len:%04d,",
                 data_count,
                 write_offset,
                 share_buf->read_offset,
                 share_buf->is_buf_full,
                 share_buf->length);
#endif /* #if 0 */
    if (!need_write) {
        log_hal_error("Lea dump - something wrong! dc:%04d, wf:%04d, rf:%04d, if:%d, len:%04d,",
                      data_count,
                      write_offset,
                      share_buf->read_offset,
                      share_buf->is_buf_full,
                      share_buf->length);
        return;
    }

    if (share_buf->read_offset < write_offset) {
        lea_RingBuf_receive_data(dump_rbuf, start_addr + share_buf->read_offset, data_count);
    } else {
        unwrap_size = share_buf->length - share_buf->read_offset;
        lea_RingBuf_receive_data(dump_rbuf, start_addr + share_buf->read_offset, unwrap_size);
        if (share_buf->read_offset != 0) {
            lea_RingBuf_receive_data(dump_rbuf, start_addr, data_count - unwrap_size);
        }
    }

    share_buf->read_offset = (share_buf->read_offset + data_count) % share_buf->length;
    if (g_lea_dbg.dump_waiting_data) {
        if (pdPASS != xSemaphoreGive(g_lea_dbg.dump_semphr)) {
            log_hal_error("semaphore give error!");
        }
    }
    return;
}

void _lea_dump_process(void *arg)
{
    bool dataNotReady;
    uint32_t data_count;
    uint8_t *pRead;
    uint8_t *pWrite;
    ring_buf *pbuf = &g_lea_dbg.lea_dump_buf;

    do {

        taskENTER_CRITICAL();
        if (pbuf->datacount == 0) {
            dataNotReady = true;
            g_lea_dbg.dump_waiting_data = true;
        } else {
            dataNotReady = false;
            data_count = pbuf->datacount;
            pRead = pbuf->pRead;
            pWrite = pbuf->pWrite;
            g_lea_dbg.dump_waiting_data = false;
        }
        taskEXIT_CRITICAL();

        if (dataNotReady) {
            //            log_hal_info("waiting data!");
            xSemaphoreTake(g_lea_dbg.dump_semphr, portMAX_DELAY);
            continue;
        }

        //    log_hal_info("lea_dump in pRead:%p, pWrite:%p, data_count:%d", pRead, pWrite, data_count);
        if (pRead < pWrite) {
            _lea_dump_write_file(pRead, data_count);
            pRead += data_count;
            if (pRead >= pbuf->pBufEnd)
                pRead -= pbuf->bufLen;
        } else {
            uint32_t r2e = pbuf->pBufEnd - pRead;
            _lea_dump_write_file(pRead, r2e);
            _lea_dump_write_file(pbuf->pBufBase, data_count - r2e);
            pRead = pbuf->pBufBase + data_count - r2e;
        }
        _lea_dump_sync_file();
        //    log_hal_info("lea_dump out pRead:%p, pWrite:%p, data_count:%d", pRead, pWrite, data_count);

        taskENTER_CRITICAL();
        pbuf->pRead = pRead;
        pbuf->datacount -= data_count;
        taskEXIT_CRITICAL();
    } while (1);

    vTaskDelete(NULL);
}

static void _lea_mt7933_adsp_recv_msg(struct ipi_msg_t *p_ipi_msg)
{
    //    log_hal_error("%s %p, ready:%d, start:%d", __func__, p_ipi_msg, g_lea_dbg.dump_ready, g_lea_dbg.dump_start);
    if (!p_ipi_msg || !g_lea_dbg.dump_ready || !g_lea_dbg.dump_start) {
        log_hal_warning("lea dump receive msg error! %p, ready:%d, start:%d", p_ipi_msg, g_lea_dbg.dump_ready, g_lea_dbg.dump_start);
        return;
    }

    if (p_ipi_msg->task_scene == TASK_SCENE_LEAUDIO_DECODE) {
        if (p_ipi_msg->msg_id == MSG_TO_HOST_DSP_LEA_DEBUG_DUMP_REQ) {
            taskENTER_CRITICAL();
            _lea_receive_data_from_dsp();
            taskEXIT_CRITICAL();
        }
    } else {
        log_hal_error("%s. Not a support task scene: ID = %d\n", __func__, p_ipi_msg->task_scene);
    }
    return;
}

static int _lea_mt7933_adsp_init_dump(void)
{
    log_hal_info("%s", __func__);
    int ret;
    struct ipi_msg_t msg;
    lea_host_dump_param_t host_param_ack;
    void *paddr = NULL;

    memset(&host_param_ack, 0, sizeof(lea_host_dump_param_t));
    ret = audio_send_ipi_msg(&msg,
                             TASK_SCENE_LEAUDIO_DECODE,
                             AUDIO_IPI_LAYER_TO_DSP,
                             AUDIO_IPI_PAYLOAD,
                             AUDIO_IPI_MSG_NEED_ACK,
                             MSG_TO_DSP_LEAUDIO_DEBUG_READY,
                             sizeof(lea_host_dump_param_t),
                             0,
                             &host_param_ack);

    if (ret) {
        log_hal_error("dump start ipi msg error! ret:%d", ret);
        return -1;
    }
    memcpy((void *)(&host_param_ack), (void *)(&msg.payload), sizeof(lea_host_dump_param_t));
    if (!host_param_ack.is_inited) {
        log_hal_error("lea debug is not ready!");
        return -1;
    }
    paddr = _convert_addr(host_param_ack.dsp_buf_info_base);
    if (paddr == NULL) {
        log_hal_error("get dsp buffer error!");
        return -1;
    }

    g_lea_dbg.dsp_share_buf = (lea_dsp_share_buf_info_t *)paddr;
    log_hal_info("dump buf base:%08x, start address:%08x, length:%d, is_full:%d read_offset:%08x, write_offset;%08x",
                 (uint32_t)g_lea_dbg.dsp_share_buf,
                 g_lea_dbg.dsp_share_buf->start_addr,
                 g_lea_dbg.dsp_share_buf->length,
                 g_lea_dbg.dsp_share_buf->is_buf_full,
                 g_lea_dbg.dsp_share_buf->read_offset,
                 g_lea_dbg.dsp_share_buf->write_offset);
    g_lea_dbg.dump_ready = true;
    return 0;
}

static void _lea_mt7933_adsp_start_dump(void)
{
    log_hal_info("%s", __func__);
    if (!g_lea_dbg.dump_ready) {
        log_hal_warning("dump shared memroy not ready!");
        return;
    }
    int ret;
    struct ipi_msg_t msg;
    ret = audio_send_ipi_msg(&msg,
                             TASK_SCENE_LEAUDIO_DECODE,
                             AUDIO_IPI_LAYER_TO_DSP,
                             AUDIO_IPI_MSG_ONLY,
                             AUDIO_IPI_MSG_NEED_ACK,
                             MSG_TO_DSP_LEAUDIO_DEBUG_START,
                             0,
                             0,
                             NULL);

    if (ret) {
        log_hal_error("dump start ipi msg error! ret:%d", ret);
    }
    return;
}

static void _lea_mt7933_adsp_stop_dump(void)
{
    log_hal_info("%s", __func__);
    int ret;
    struct ipi_msg_t msg;
    g_lea_dbg.dump_ready = false;
    if (g_lea_dbg.dsp_ready) {
        ret = audio_send_ipi_msg(&msg,
                                 TASK_SCENE_LEAUDIO_DECODE,
                                 AUDIO_IPI_LAYER_TO_DSP,
                                 AUDIO_IPI_MSG_ONLY,
                                 AUDIO_IPI_MSG_NEED_ACK,
                                 MSG_TO_DSP_LEAUDIO_DEBUG_STOP,
                                 0,
                                 0,
                                 NULL);
        if (ret) {
            log_hal_error("dump stop ipi msg error! ret:%d", ret);
        }
    }
}
#endif /* #ifdef HAL_LEA_DUMP_ENABLE */

hal_audio_status_t hal_audio_lea_dl_open(struct bt_LeAudioCodecConfiguration *cfg)
{

    int ret;
    struct ipi_msg_t msg;
    stream_params_t stream_par;

    if (cfg == NULL) {
        log_hal_error("NULL pointer encountered!");
        return HAL_AUDIO_STATUS_ERROR;
    }

    stream_par.sample_rate = _leaudio_get_sample_rate(cfg->lc3Config.samplingFrequency);
    stream_par.channels = cfg->audioChannelAllocation;
    stream_par.bit_type = _leaudio_get_bitType(cfg->lc3Config.pcmBitDepth);
    stream_par.period_size = 5 * stream_par.sample_rate / 1000; //time(ms) * sample_rate / 1000, time = 5ms
    stream_par.period_count = 8;
    stream_par.format = 1 << 2; //MSD_PCM_FMT_S16_LE, fixed argument.

    log_hal_info("sample_rate:%d, channels:%d, bit_type:%d, bitrate:%d",
                 stream_par.sample_rate,
                 stream_par.channels,
                 stream_par.bit_type,
                 cfg->encodedAudioBitrate);

    //open playback
    ret = le_audio_ipi_dsp_open(&stream_par);
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    //open and start afe.
    ret = audio_drv_init(&stream_par, true);
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    //open le audio fecther and decoder
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_LEAUDIO_DECODE,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_PAYLOAD,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_LEAUDIO_OPEN,
              sizeof(struct bt_LeAudioCodecConfiguration),
              0,
              cfg);

    if (ret || msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        log_hal_error("open failed! ack type %u", msg.ack_type);
        return HAL_AUDIO_STATUS_ERROR;
    }

#ifdef HAL_LEA_DUMP_ENABLE
    g_lea_dbg.dsp_ready = true;
    if (g_lea_dbg.dump_start) {
        _lea_mt7933_adsp_init_dump();
        _lea_mt7933_adsp_start_dump();
    }
#endif /* #ifdef HAL_LEA_DUMP_ENABLE */

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_lea_dl_start(void)
{
    int ret;
    struct ipi_msg_t msg;

    //playback start
    ret = le_audio_ipi_dsp_start();
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    //decoder start
    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_LEAUDIO_DECODE,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_LEAUDIO_START,
              0,
              0,
              NULL);

    if (ret || msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        log_hal_error("start failed! ack type %u", msg.ack_type);
        return HAL_AUDIO_STATUS_ERROR;
    }

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_lea_dl_stop(void)
{
    int ret;
    struct ipi_msg_t msg;

    //playback stop
    ret = le_audio_ipi_dsp_stop();
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_LEAUDIO_DECODE,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_LEAUDIO_STOP,
              0,
              0,
              NULL);

    if (ret || msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        log_hal_error("stop failed, ack type %u", msg.ack_type);
        return HAL_AUDIO_STATUS_ERROR;
    }

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_lea_dl_close(void)
{
    int ret;
    struct ipi_msg_t msg;

    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_LEAUDIO_DECODE,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_LEAUDIO_CLOSE,
              0,
              0,
              NULL);

    if (ret || msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        log_hal_error("close failed, ack type %u", msg.ack_type);
    }

#ifdef HAL_LEA_DUMP_ENABLE
    if (g_lea_dbg.dump_start) {
        _lea_mt7933_adsp_stop_dump();
        g_lea_dbg.dsp_share_buf = NULL;
    }
    g_lea_dbg.dsp_ready = false;
#endif /* #ifdef HAL_LEA_DUMP_ENABLE */

    ret = audio_drv_deinit(true);
    if (ret)
        log_hal_error("deinit audio driver err!");

    ret = le_audio_ipi_dsp_close();
    if (ret)
        log_hal_error("close playback err!");

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_lea_dl_ch_update(struct bt_ConnParam *parm)
{
    int32_t ret;
    struct ipi_msg_t msg;

    if (parm == NULL) {
        log_hal_error("parm pointer is NULL!");
        return HAL_AUDIO_STATUS_ERROR;
    }

    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_LEAUDIO_DECODE,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_PAYLOAD,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_LEAUDIO_CH_UPDATE,
              sizeof(struct bt_ConnParam),
              0,
              parm);

    if (ret || msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        log_hal_error("ch update failed! ack type %u", msg.ack_type);
        return HAL_AUDIO_STATUS_ERROR;
    }

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_lea_dl_suspend(void)
{
    int ret;
    struct ipi_msg_t msg;

    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_LEAUDIO_DECODE,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_LEAUDIO_SUSPEND,
              0,
              0,
              NULL);

    if (ret || msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        log_hal_error("close failed, ack type %u", msg.ack_type);
        return HAL_AUDIO_STATUS_ERROR;
    }

    ret = le_audio_ipi_dsp_suspend();
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_lea_dl_resume(void)
{
    int ret;
    struct ipi_msg_t msg;

    ret = le_audio_ipi_dsp_resume();
    if (ret)
        return HAL_AUDIO_STATUS_ERROR;

    ret = audio_send_ipi_msg(
              &msg,
              TASK_SCENE_LEAUDIO_DECODE,
              AUDIO_IPI_LAYER_TO_DSP,
              AUDIO_IPI_MSG_ONLY,
              AUDIO_IPI_MSG_NEED_ACK,
              MSG_TO_DSP_LEAUDIO_RESUME,
              0,
              0,
              NULL);

    if (ret || msg.ack_type != AUDIO_IPI_MSG_ACK_BACK) {
        log_hal_error("close failed, ack type %u", msg.ack_type);
        return HAL_AUDIO_STATUS_ERROR;
    }

    return HAL_AUDIO_STATUS_OK;
}
#endif /* #ifdef CFG_AUDIO_DSP_LEAUDIO_EN */

#define HAL_AUDIO_CLI_TEST
#ifdef HAL_AUDIO_CLI_TEST
#include "cli.h"
static uint8_t audio_test_status = 0;
static xTaskHandle audio_test_task = 0;

static void hal_audio_test_task(void *param)
{
    log_hal_info("%s", __FUNCTION__);

#ifdef HAL_USE_SGEN
    uint8_t *data_ptr = (uint8_t *)hal_afe_sgen_golden_table_16bits;
#else /* #ifdef HAL_USE_SGEN */
    uint8_t *data_ptr = (uint8_t *)hal_holiday_model; //sample rate:8000, channels:2, bit_depth:16
#endif /* #ifdef HAL_USE_SGEN */

    int32_t size_written;
    uint32_t size_writting;
    uint32_t total_written;
    do {
        if (audio_test_status) {
#ifdef HAL_USE_SGEN
            size_writting = sizeof(hal_afe_sgen_golden_table_16bits);
            total_written = 0;

            //log_hal_info("write begin....");

            size_written = hal_audio_stream_out_write(data_ptr, size_writting);
            if (size_written < 0) {
                log_hal_info("write error");
                break;
            }
            total_written += size_written;

            size_written = hal_audio_stream_out_write(data_ptr, size_writting);
            if (size_written < 0) {
                log_hal_info("write error");
                break;
            }
            total_written += size_written;

            size_written = hal_audio_stream_out_write(data_ptr, size_writting);
            if (size_written < 0) {
                log_hal_info("write error");
                break;
            }
            total_written += size_written;

            size_written = hal_audio_stream_out_write(data_ptr, size_writting);
            if (size_written < 0) {
                log_hal_info("write error");
                break;
            }
            total_written += size_written;

            //log_hal_info("write end, total_written = %d", total_written);
#else /* #ifdef HAL_USE_SGEN */
            size_writting = 1024;
            total_written = 0;
            size_written = 0;

            log_hal_info("write start...0x%08x", *(uint32_t *)data_ptr);

            while (total_written < 160000) {
                if (!audio_test_status) {
                    break;
                }
                log_hal_debug("total_written 0x%08x", total_written);

                size_written = hal_audio_stream_out_write(data_ptr + total_written, size_writting);
                if (size_written < 0) {
                    log_hal_info("write error");
                    break;
                }

                total_written += size_written;

                if (total_written + size_writting > 160000)
                    size_writting = 160000 - total_written;

                vTaskDelay(pdMS_TO_TICKS(30));
            }
#endif /* #ifdef HAL_USE_SGEN */
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    } while (1);
}

#ifdef CFG_AUDIO_DSP_LEAUDIO_EN
#define AM_DSP_ON_RETRY_MAX           (60)
#define AM_DSP_OFF_RETRY_MAX          (5)
#define ADSP_DISABLED 0
#define ADSP_ENABLED 1
#define ADSP_LOADING 2

hal_audio_status_t hal_audio_dsp_on(void)
{
    uint32_t enable = 1;
    uint32_t status = 0;
    short retry = 0;
    control_cget_v2("ADSP_Enable", 1, &status);
    if (!status) {
        control_cset("ADSP_Enable", 1, &enable);
        while (retry < AM_DSP_ON_RETRY_MAX && !status) {
            control_cget_v2("ADSP_Enable", 1, &status);
            vTaskDelay(pdMS_TO_TICKS(5));
            retry++;
        }
        if (retry >= AM_DSP_ON_RETRY_MAX) {
            log_hal_error("[sink][AM]DSP ON timeout:, retry times: %d", 1, retry);
            return HAL_AUDIO_STATUS_ERROR;
        }
    }
    return HAL_AUDIO_STATUS_OK;
}

hal_audio_status_t hal_audio_dsp_off(void)
{
    uint32_t enable = 0;
    uint32_t status = 1;
    short retry = 0;

    control_cget_v2("ADSP_Enable", 1, &status);
    if (status == ADSP_DISABLED) {
        return HAL_AUDIO_STATUS_OK;
    }

    if (status == ADSP_LOADING) {
        // adsp enable might be still in progress, wait for finish
        log_hal_info("adsp enabling");
        while (retry < AM_DSP_ON_RETRY_MAX) {
            control_cget_v2("ADSP_Enable", 1, &status);
            if (status == ADSP_ENABLED) {
                log_hal_info("enable done");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(5));
            retry++;
        }
        if (retry >= AM_DSP_ON_RETRY_MAX) {
            log_hal_error("DSP ON timeout:, retry times: %d", retry);
            return HAL_AUDIO_STATUS_ERROR;
        }
        log_hal_info("retry times: %d, status = %d", retry, status);
    }

    // try to disable adsp
    retry = 0;
    control_cset("ADSP_Enable", 1, &enable);
    while (retry < AM_DSP_OFF_RETRY_MAX) {
        control_cget_v2("ADSP_Enable", 1, &status);
        if (status == ADSP_DISABLED) {
            log_hal_info("disable done");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
        retry++;
    }
    if (retry >= AM_DSP_OFF_RETRY_MAX) {
        log_hal_error("DSP OFF timeout, retry times: %d", retry);
        return HAL_AUDIO_STATUS_ERROR;
    }
    log_hal_info("status:%d", status);
    return HAL_AUDIO_STATUS_OK;
}
#endif /* #ifdef CFG_AUDIO_DSP_LEAUDIO_EN */

static uint8_t cmd_hal_audio_init(uint8_t len, char *params[])
{
    if (audio_test_task) {
        log_hal_error("already initialized!");
        return -1;
    }

    if (pdPASS != xTaskCreate(hal_audio_test_task, "hal_aud", 1024 / sizeof(portSTACK_TYPE), NULL, 4, &audio_test_task)) {
        log_hal_error("open hal_audio err!");
        return -1;
    }
    log_hal_info("%s", __FUNCTION__);
    return 0;
}

static uint8_t cmd_hal_audio_deinit(uint8_t len, char *params[])
{
    log_hal_info("%s", __FUNCTION__);

    if (audio_test_task)
        vTaskDelete(audio_test_task);
    audio_test_task = 0x0;

    return 0;

}

static uint8_t cmd_hal_audio_open(uint8_t len, char *params[])
{
    if (len != 3) {
        log_hal_error("hal_aud open [sample_rate] [channels] [bit_type]");
        log_hal_error("[hal_aud open 8000 2 16], golden table <<holiday model>>");
        return -1;
    }

    uint32_t sample_rate = atoi(params[0]);
    uint32_t channels = atoi(params[1]);
    uint32_t bit_type = atoi(params[2]);

    hal_audio_stream_out_open(sample_rate, channels, bit_type);

    return 0;
}
static uint8_t cmd_hal_audio_close(uint8_t len, char *params[])
{
    hal_audio_stream_out_close();
    return 0;
}

static uint8_t cmd_hal_audio_start(uint8_t len, char *params[])
{
    if (hal_audio_stream_out_start()) {
        return -1;
    }

    audio_test_status = 1;
    return 0;
}

static uint8_t cmd_hal_audio_stop(uint8_t len, char *params[])
{
    audio_test_status = 0;

    if (hal_audio_stream_out_stop()) {
        return -1;
    }

    return 0;
}

static uint8_t cmd_hal_audio_set_volume(uint8_t len, char *params[])
{
    if (len != 1) {
        log_hal_error("hal_aud setvol [volume], volume:0~15");
        return -1;
    }
    uint16_t vol = atoi(params[0]);
    if (hal_audio_stream_out_set_volume(vol)) {
        return -1;
    }

    return 0;
}

static uint8_t cmd_hal_audio_volume_up_down(uint8_t len, char *params[])
{
    const uint8_t vol_map[16] = {
        0x00, 0x01, 0x03, 0x05,
        0x07, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0F, 0x10,
        0x11, 0x12, 0x13, 0x14,
    };

    if (len != 1) {
        log_hal_error("vol [+/-]");
        return -1;
    }

    uint32_t analog_volume;
    int8_t index;
    bool is_vol_found = false;

    if (hal_audio_get_stream_out_volume(&analog_volume)) {
        log_hal_error("get volume err!");
    } else {
        for (index = 0; index < 16; index++) {
            if (vol_map[index] == analog_volume) {
                is_vol_found = true;
                break;
            }
        }
    }

    if (is_vol_found) {
        if (!strcmp(params[0], "+")) {
            if (++index >= 16)
                index = 15;
        } else if (!strcmp(params[0], "-")) {
            if (--index < 0)
                index = 0;
        } else {
            log_hal_error("vol [+/-]");
            return -1;
        }
    } else {
        index = 5;
    }
    hal_audio_set_stream_out_volume(0, vol_map[index]);

#ifdef PLAYING_PROMPT_VOICE_ENABLE
    hal_audio_play_prompt_voice();
#endif /* #ifdef PLAYING_PROMPT_VOICE_ENABLE */
    return 0;
}

#ifdef CFG_AUDIO_DSP_LEAUDIO_EN
static uint8_t cmd_leaudio_dl_open(uint8_t len, char *params[])
{
    struct bt_LeAudioCodecConfiguration cfg;
    if (len != 3) {
        log_hal_error("hal_aud lea_dl_open [sample_rate] [channels] [bit_type]");
        return -1;
    }

    uint32_t sample_rate = atoi(params[0]);
    uint32_t channels = atoi(params[1]);
    uint32_t bit_type = atoi(params[2]);

    if (sample_rate == 8000) {
        cfg.lc3Config.samplingFrequency = LC3_RATE_8000;
    } else if (sample_rate == 16000) {
        cfg.lc3Config.samplingFrequency = LC3_RATE_16000;
    } else if (sample_rate == 24000) {
        cfg.lc3Config.samplingFrequency = LC3_RATE_24000;
    } else if (sample_rate == 32000) {
        cfg.lc3Config.samplingFrequency = LC3_RATE_32000;
    } else if (sample_rate == 48000) {
        cfg.lc3Config.samplingFrequency = LC3_RATE_48000;
    } else if (sample_rate == 44100) {
        cfg.lc3Config.samplingFrequency = LC3_RATE_44100;
    } else if (sample_rate == 88200) {
        cfg.lc3Config.samplingFrequency = LC3_RATE_88200;
    } else if (sample_rate == 96000) {
        cfg.lc3Config.samplingFrequency = LC3_RATE_96000;
    } else if (sample_rate == 176400) {
        cfg.lc3Config.samplingFrequency = LC3_RATE_176400;
    } else if (sample_rate == 192000) {
        cfg.lc3Config.samplingFrequency = LC3_RATE_192000;
    } else {
        log_hal_error("%d:unsupported sample rete!", sample_rate);
        return -1;
    }

    if (bit_type == 16) {
        cfg.lc3Config.pcmBitDepth = LC3_BITS_16;
    } else if (bit_type == 24) {
        cfg.lc3Config.pcmBitDepth = LC3_BITS_24;
    } else if (bit_type == 32) {
        cfg.lc3Config.pcmBitDepth = LC3_BITS_32;
    } else {
        log_hal_error("%d:unsupported bit depth!", bit_type);
        return -1;
    }

    cfg.codecType = LEAUDIO_CODEC_TYPE_LC3;
    cfg.audioChannelAllocation = channels; //1:mono 2:stero
    cfg.encodedAudioBitrate = 124; //bit per sec:
    cfg.bfi_ext = 0;
    cfg.plc_method = 0;
    cfg.le_audio_type = 0; //not used.
    cfg.lc3Config.frameDuration = DURATION_10000US; //10msec
    cfg.lc3Config.octetsPerFrame = 0; //not used.
    cfg.lc3Config.blocksPerSdu = 0; //not used.
    cfg.presentation_delay = 40;//ms
    if (hal_audio_lea_dl_open(&cfg)) {
        return -1;
    }
    return 0;
}

static uint8_t cmd_leaudio_dl_start(uint8_t len, char *params[])
{
    if (hal_audio_lea_dl_start())
        return -1;
    return 0;
}

static uint8_t cmd_leaudio_dl_stop(uint8_t len, char *params[])
{
    if (hal_audio_lea_dl_stop())
        return -1;
    return 0;
}

static uint8_t cmd_leaudio_dl_close(uint8_t len, char *params[])
{
    if (hal_audio_lea_dl_close())
        return -1;
    return 0;
}

static uint8_t cmd_leaudio_dl_ch_update(uint8_t len, char *params[])
{
    struct bt_ConnParam connParam;
    connParam.conn_handle_L = 0x01;
    connParam.conn_handle_R = 0x00;
    connParam.bn = 0;

    if (hal_audio_lea_dl_ch_update(&connParam))
        return -1;
    return 0;
}

static uint8_t cmd_leaudio_dl_suspend(uint8_t len, char *params[])
{
    if (hal_audio_lea_dl_suspend())
        return -1;
    return 0;
}

static uint8_t cmd_leaudio_dl_resume(uint8_t len, char *params[])
{
    if (hal_audio_lea_dl_resume())
        return -1;
    return 0;
}

#endif /* #ifdef CFG_AUDIO_DSP_LEAUDIO_EN */

#ifdef HAL_LEA_DUMP_ENABLE
int hal_lea_dump_open(bool isUSB_dump)
{
    char *file_name;
    if (isUSB_dump) {
        file_name = "USB:/LeAudio_dump.lc3";
    } else {
        file_name = "SD:/LeAudio_dump.lc3";
    }

    log_hal_info("[Leaudio] dump data to %s", file_name);

    FRESULT f_ret;
    f_ret = f_open(&g_lea_dbg.fid, file_name, FA_CREATE_ALWAYS | FA_WRITE);
    if (f_ret) {
        log_hal_error("f_open error ret:%d", f_ret);
        return -1;
    }

    BaseType_t base_ret;
    base_ret = xTaskCreate(_lea_dump_process,
                           "lea_dbg",
                           configMINIMAL_STACK_SIZE * 4,
                           NULL,
                           TASK_PRIORITY_SOFT_REALTIME,
                           &g_lea_dbg.dump_tsk_hdl);

    if (base_ret != pdTRUE) {
        log_hal_error("%s, wake thread failed\n", __func__);
        goto lea_dump_open_err_exit;
    }

    if (lea_init_ring_buf(&g_lea_dbg.lea_dump_buf, LC3_DUMP_DATA_BUFFER_SIZE))
        goto lea_dump_open_err_exit;

    g_lea_dbg.dump_semphr = xSemaphoreCreateCounting(1, 0);
    if (g_lea_dbg.dump_semphr == NULL)
        goto lea_dump_open_err_exit;

    audio_task_register_callback(TASK_SCENE_LEAUDIO_DECODE, _lea_mt7933_adsp_recv_msg, NULL);
    if (g_lea_dbg.dsp_ready) {
        _lea_mt7933_adsp_init_dump();
        _lea_mt7933_adsp_start_dump();
    }
    g_lea_dbg.dump_start = true;
    return 0;

lea_dump_open_err_exit:
    f_close(&g_lea_dbg.fid);

    if (g_lea_dbg.dump_tsk_hdl) {
        vTaskDelete(g_lea_dbg.dump_tsk_hdl);
        g_lea_dbg.dump_tsk_hdl = NULL;
    }
    if (g_lea_dbg.dump_semphr) {
        vSemaphoreDelete(g_lea_dbg.dump_semphr);
        g_lea_dbg.dump_semphr = NULL;
    }

    return -1;
}

int hal_lea_dump_close(void)
{
    log_hal_info("Le Audio dump stop!");

    g_lea_dbg.dump_start = false;
    _lea_mt7933_adsp_stop_dump();
    f_close(&g_lea_dbg.fid);
    if (g_lea_dbg.dump_tsk_hdl) {
        vTaskDelete(g_lea_dbg.dump_tsk_hdl);
        g_lea_dbg.dump_tsk_hdl = NULL;
    }

    if (g_lea_dbg.dump_semphr) {
        vSemaphoreDelete(g_lea_dbg.dump_semphr);
        g_lea_dbg.dump_semphr = NULL;
    }

    lea_deinit_ring_buf(&g_lea_dbg.lea_dump_buf);
    return 0;
}

static uint8_t cmd_leaudio_dump_open(uint8_t len, char *params[])
{
    if (len != 1) {
        log_hal_error("hal_aud lea_dump_open [1/0]");
        log_hal_error("1:USB, 0:SDcard");
        return -1;
    }

    bool isUsb = atoi(params[0]);
    hal_lea_dump_open(isUsb);
    return 0;
}

static uint8_t cmd_leaudio_dump_close(uint8_t len, char *params[])
{
    hal_lea_dump_close();
    return 0;
}

#endif /* #ifdef HAL_LEA_DUMP_ENABLE */

cmd_t cmd_hal_audio[] = {
    {"init", "init", cmd_hal_audio_init, NULL},
    {"deinit", "de-init", cmd_hal_audio_deinit, NULL},
    {"open", "open hal audio", cmd_hal_audio_open, NULL},
    {"close", "de-close hal audio", cmd_hal_audio_close, NULL},
    {"start", "start stream out", cmd_hal_audio_start, NULL},
    {"stop", "stop stream out", cmd_hal_audio_stop, NULL},
    {"setvol", "set stream out volume(hw gain)", cmd_hal_audio_set_volume, NULL},
    {"vol", "set dl volume(DAC)", cmd_hal_audio_volume_up_down, NULL},
#ifdef CFG_AUDIO_DSP_LEAUDIO_EN
    {"lea_dl_open", "le audio dl open", cmd_leaudio_dl_open, NULL},
    {"lea_dl_start", "le audio dl start", cmd_leaudio_dl_start, NULL},
    {"lea_dl_stop", "le audio dl stop", cmd_leaudio_dl_stop, NULL},
    {"lea_dl_close", "le audio dl close", cmd_leaudio_dl_close, NULL},
    {"lea_dl_update", "le audio dl update", cmd_leaudio_dl_ch_update, NULL},
    {"lea_dl_suspend", "le audio dl suspend", cmd_leaudio_dl_suspend, NULL},
    {"lea_dl_resume", "le audio dl resume", cmd_leaudio_dl_resume, NULL},
#ifdef HAL_LEA_DUMP_ENABLE
    {"lea_dump_open", "le audio dump open", cmd_leaudio_dump_open, NULL},
    {"lea_dump_close", "le audio dump close", cmd_leaudio_dump_close, NULL},
#endif /* #ifdef HAL_LEA_DUMP_ENABLE */
#endif /* #ifdef CFG_AUDIO_DSP_LEAUDIO_EN */
};
#endif /* #ifdef HAL_AUDIO_CLI_TEST */

#endif /* #if defined(HAL_AUDIO_MODULE_ENABLED) */
