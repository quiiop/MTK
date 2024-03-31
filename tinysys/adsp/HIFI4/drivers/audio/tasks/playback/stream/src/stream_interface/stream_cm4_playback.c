/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */


#include "audio_types.h"
#include "source_inter.h"
#include "dsp_buffer.h"
#include "dsp_memory.h"
#include "stream_cm4_playback.h"
#include "dsp_callback.h"
#include "dsp_temp.h"
#include "mtk_heap.h"

#define CM4_PLAYBACK_PCM        0 // Yo: should use AUDIO_DSP_CODEC_TYPE_PCM to sync with MCU
#define GET_HW_SEM_RETRY_TIMES  10000
#ifndef UNUSED
#define UNUSED(p) ((void)(p))
#endif

static volatile cm4_playback_pcm_ctrl_blk_t CM4_PlaybackCtrl = {
    .data_request_threshold = 2048,
    .data_request_signal = 0,
    .frame_size = 480,
};

static volatile uint32_t int_mask;

VOID CB_CM4_PLAYBACK_DATA_REQ_ACK(VOID)
{
    CM4_PlaybackCtrl.data_request_signal = 0;
}


static VOID cm4_playback_send_data_request(VOID)
{
#ifdef USE_CCNI
    hal_ccni_message_t msg;
    memset((void *)&msg, 0, sizeof(hal_ccni_message_t));
    msg.ccni_message[0] = MSG_DSP2MCU_PLAYBACK_DATA_REQUEST << 16;
    aud_msg_tx_handler(msg, 0, FALSE);
#endif
}


static VOID cm4_playback_parameter_initialization(VOID)
{
    CM4_PlaybackCtrl.data_request_signal = 0;
}


static VOID cm4_playback_hardware_semaphore_take(VOID) // Similar to StreamDSP_HWSemaphoreTake, may consider to combine
{
#ifdef HAL_AUDIO_READY
    uint32_t take_times = 0;

    while(++take_times)
    {
        hal_nvic_save_and_set_interrupt_mask((uint32_t *)&int_mask);

        if (HAL_HW_SEMAPHORE_STATUS_OK == hal_hw_semaphore_take(HW_SEMAPHORE_AUDIO_CM4_DSP0_PLAYBACK))
            break;

        if (take_times > GET_HW_SEM_RETRY_TIMES) {
            hal_nvic_restore_interrupt_mask(int_mask);

            //error handling
            DSP_MW_LOG_I("[CM4_PB] Can not take HW Semaphore", 0);
            configASSERT(0);
        }

        //vTaskDelay(2/portTICK_PERIOD_MS);

        hal_nvic_restore_interrupt_mask(int_mask);
    }
#endif
}


static VOID cm4_playback_hardware_semaphore_give(VOID) // Similar to StreamDSP_HWSemaphoreGive, may consider to combine
{
#ifdef HAL_AUDIO_READY
    if (HAL_HW_SEMAPHORE_STATUS_OK == hal_hw_semaphore_give(HW_SEMAPHORE_AUDIO_CM4_DSP0_PLAYBACK)) {
        hal_nvic_restore_interrupt_mask(int_mask);
    } else {
        hal_nvic_restore_interrupt_mask(int_mask);

        //error handling
        DSP_MW_LOG_I("[CM4_PB] Can not give HW Semaphore", 0);
        configASSERT(0);
    }
#endif
}


static VOID cm4_playback_update_from_share_information(SOURCE source)
{

    SHARE_BUFFER_INFO *ptr = (SHARE_BUFFER_INFO *)source->param.cm4_playback.info.share_info_base_addr;

    cm4_playback_hardware_semaphore_take();

    /* Put your code here, if the semaphore is taken, do something for the critical resource...... */
    source->streamBuffer.ShareBufferInfo.startaddr = ptr->startaddr;
    source->streamBuffer.ShareBufferInfo.WriteOffset = ptr->WriteOffset;
    source->streamBuffer.ShareBufferInfo.length = ptr->length;
    source->streamBuffer.ShareBufferInfo.bBufferIsFull = ptr->bBufferIsFull;
    source->streamBuffer.ShareBufferInfo.presentation_time_stamp = ptr->presentation_time_stamp;
    source->streamBuffer.ShareBufferInfo.total_write = ptr->total_write;

    cm4_playback_hardware_semaphore_give();
}


static VOID cm4_playback_update_to_share_information(SOURCE source)
{
    SHARE_BUFFER_INFO *ptr = (SHARE_BUFFER_INFO *)source->param.cm4_playback.info.share_info_base_addr;

    cm4_playback_hardware_semaphore_take();

    /* Put your code here, if the semaphore is taken, do something for the critical resource...... */
    ptr->ReadOffset = source->streamBuffer.ShareBufferInfo.ReadOffset;

    cm4_playback_hardware_semaphore_give();
}

BOOL SourceReadBuf_CM4_playback(SOURCE source, U8 *dst_addr, U32 length)
{
    TRANSFORM transform =  source->transform;
    SHARE_BUFFER_INFO *share_buff_info  = &(source->streamBuffer.ShareBufferInfo);
    DSP_CALLBACK_PTR callback_ptr = NULL;
    S16* dst_addr1 = NULL;
    S16* dst_addr0 = NULL;
    S16* src_temp_addr = NULL;
    audio_channel_number_t channel = source->param.cm4_playback.info.channel_number;
    audio_bits_per_sample_t bit_type = source->param.cm4_playback.info.bit_type;
    U8 source_channels = source->param.cm4_playback.info.source_channels;
    U32 i;

    if (transform != NULL) {
        callback_ptr = DSP_Callback_Get(source, transform->sink);
        dst_addr = callback_ptr->EntryPara.in_ptr[0];
    }

    if (AUDIO_BITS_PER_SAMPLING_16 == bit_type) {

        DSP_C2D_BufferCopy( (VOID*)  dst_addr,
                            (VOID*)  (share_buff_info->startaddr + share_buff_info->ReadOffset),
                            (U16)    length*source_channels,
                            (VOID*)  share_buff_info->startaddr,
                            (U16)    share_buff_info->length);

        if (2 == source_channels) {

            /* Use channel 0 as temp buffer */
            src_temp_addr = (S16*)dst_addr;

            dst_addr0 = callback_ptr->EntryPara.in_ptr[0];
            dst_addr1 = callback_ptr->EntryPara.in_ptr[1];

            switch (channel)
            {
                case AUDIO_STEREO:
                    for (i = 0 ; i < length ; i++) {
                        *dst_addr0++ = *src_temp_addr++;
                        *dst_addr1++ = *src_temp_addr++;
                    }
                    break;

                case AUDIO_STEREO_BOTH_L_CHANNEL:
                    for (i = 0 ; i < length ; i++) {
                        *dst_addr0++ = *dst_addr1++ = *src_temp_addr++;
                        src_temp_addr++;
                    }
                    break;

                case AUDIO_STEREO_BOTH_R_CHANNEL:
                    for (i = 0 ; i < length ; i++) {
                        src_temp_addr++;
                        *dst_addr0++ = *dst_addr1++ = *src_temp_addr++;
                    }
                    break;

                case AUDIO_STEREO_BOTH_L_R_SWAP:
                    for (i = 0 ; i < length ; i++) {
                        *dst_addr1++ = *src_temp_addr++;
                        *dst_addr0++ = *src_temp_addr++;
                    }
                    break;

                case AUDIO_MONO:
                default:
                    configASSERT(FALSE);
                    break;
            }
        }
    }
    else {
        // 24-bits PCM: To Do
    }

    return TRUE;
}


U32 SourceSize_CM4_playback(SOURCE source)
{
    SHARE_BUFFER_INFO *share_buff_info  = &(source->streamBuffer.ShareBufferInfo);
    CM4_PLAYBACK_PARAMETER *cm4_playback_param = &(source->param.cm4_playback);
    U8 word_bytes = cm4_playback_param->info.bit_type?4:2;
    U8 source_channels = cm4_playback_param->info.source_channels;
    TRANSFORM transform = source->transform;
    U32 frame_size = 0;

    if (transform) {
        SINK sink = transform->sink;
        frame_size = sink->param.audio.count;
    } else {
        frame_size = CM4_PlaybackCtrl.frame_size;
    }

    /* update share information data */
    cm4_playback_update_from_share_information(source);

    /* Check there is data in share buffer or not */
    if (share_buff_info->bBufferIsFull ||
        share_buff_info->ReadOffset != share_buff_info->WriteOffset) {

        if (share_buff_info->bBufferIsFull && share_buff_info->ReadOffset == share_buff_info->WriteOffset) {
            cm4_playback_param->remain_bs_size = share_buff_info->length;
        } else if (share_buff_info->WriteOffset > share_buff_info->ReadOffset) {
            cm4_playback_param->remain_bs_size = (share_buff_info->WriteOffset - share_buff_info->ReadOffset);
        } else if (share_buff_info->ReadOffset > share_buff_info->WriteOffset) {
            cm4_playback_param->remain_bs_size = share_buff_info->WriteOffset + (share_buff_info->length - share_buff_info->ReadOffset);
        }

        /* PCM part */
        if (cm4_playback_param->info.codec_type == CM4_PLAYBACK_PCM) {
            if(cm4_playback_param->remain_bs_size >= word_bytes*source_channels*frame_size) {
                return word_bytes*frame_size;
            } else {
                DSP_MW_LOG_I("[CM4_PB] Not enough bitstream\r\n", 0);
                return 0;
            }
        } else {
            /* Not support codec type */
            DSP_MW_LOG_E("[CM4_PB] Not support codec type\r\n", 0);
            return 0;
        }
    } else {
        /* No data in the buffer */
        //printf("[CM4_PB] No data in buffer");
        return 0;
    }
}


VOID SourceDrop_CM4_playback(SOURCE source, U32 amount)
{
    SHARE_BUFFER_INFO *share_buff_info = &(source->streamBuffer.ShareBufferInfo);
    CM4_PLAYBACK_PARAMETER *cm4_playback_param = &(source->param.cm4_playback);
    U8 source_channels = cm4_playback_param->info.source_channels;

    /* PCM part */
    if (cm4_playback_param->info.codec_type == CM4_PLAYBACK_PCM) {
        amount = amount * source_channels;

#if 0
        if (share_buff_info->bBufferIsFull == 1) {
            share_buff_info->bBufferIsFull = 0;
        }
#endif

        share_buff_info->ReadOffset += amount;

        if ( share_buff_info->ReadOffset >= share_buff_info->length ) {
            share_buff_info->ReadOffset -= share_buff_info->length;
        }

        cm4_playback_param->remain_bs_size -= amount;

        cm4_playback_update_to_share_information(source);

        if (cm4_playback_param->remain_bs_size <= CM4_PlaybackCtrl.data_request_threshold) {
            if (CM4_PlaybackCtrl.data_request_signal == 0) {
                CM4_PlaybackCtrl.data_request_signal = 1;
                cm4_playback_send_data_request();
            }
        }
    } else {
        //Not support codec type
        DSP_MW_LOG_E("[CM4_PB] Not support codec type\r\n", 0);
    }
}


U8* SourceMap_CM4_playback(SOURCE source)
{
    UNUSED(source);
    return MapAddr;
}


BOOL SourceConfigure_CM4_playback(SOURCE source, stream_config_type type, U32 value)
{
    UNUSED(source);
    UNUSED(type);
    UNUSED(value);
    return TRUE;
}


BOOL SourceClose_CM4_playback(SOURCE source)
{
    if(source->param.cm4_playback.info.share_info_base_addr) {
    	void *buf = (void *)source->param.cm4_playback.info.share_info_base_addr;
    	MTK_vPortFree(buf);
    }
    return TRUE;
}


VOID SourceInit_CM4_playback(SOURCE source)
{
    /* buffer init */
    source->type = SOURCE_TYPE_CM4_PLAYBACK;
    source->buftype = BUFFER_TYPE_CIRCULAR_BUFFER;

    cm4_playback_parameter_initialization();

    /* interface init */
    source->sif.SourceSize        = SourceSize_CM4_playback;
    source->sif.SourceMap         = SourceMap_CM4_playback;
    source->sif.SourceConfigure   = SourceConfigure_CM4_playback;
    source->sif.SourceDrop        = SourceDrop_CM4_playback;
    source->sif.SourceClose       = SourceClose_CM4_playback;
    source->sif.SourceReadBuf     = SourceReadBuf_CM4_playback;
}

U64 SourceGetTimestamp_CM4_playback(SOURCE source)
{
    U64 presentation_timestamp = 0;
    CM4_PLAYBACK_PARAMETER *cm4_playback_param = &(source->param.cm4_playback);
    U8 source_channels = cm4_playback_param->info.source_channels;
    U8 word_bytes = cm4_playback_param->info.bit_type?4:2;
    U32 rate = cm4_playback_param->info.sampling_rate;
    U32 consumed_bytes = 0;

    if (source->type != SOURCE_TYPE_CM4_PLAYBACK) {
        DSP_MW_LOG_E("[CM4_PB] source type incorrect!\r\n", 0);
        return 0;
    }

    if (!SourceSize_CM4_playback(source)) {
        DSP_MW_LOG_E("[CM4_PB] no data in share buf!\r\n", 0);
        return 0;
    }

    presentation_timestamp = source->streamBuffer.ShareBufferInfo.presentation_time_stamp;

    if (!presentation_timestamp) {
        return 0;
    }

    if (cm4_playback_param->remain_bs_size > source->streamBuffer.ShareBufferInfo.total_write) {
        DSP_MW_LOG_I("[CM4_PB] remain size exceeds %d frames (%d bytes)\r\n", 2, source->streamBuffer.ShareBufferInfo.total_write, cm4_playback_param->remain_bs_size);
	SourceDrop_CM4_playback(source, (cm4_playback_param->remain_bs_size - source->streamBuffer.ShareBufferInfo.total_write)/source_channels);
    }

    consumed_bytes = source->streamBuffer.ShareBufferInfo.total_write - cm4_playback_param->remain_bs_size;

    DSP_MW_LOG_I("[CM4_PB] presentation_timestamp %lld, consumed_bytes %d\r\n", 2, presentation_timestamp, consumed_bytes);

    presentation_timestamp += 1000000000LL*consumed_bytes/(source_channels*word_bytes*rate);

    return presentation_timestamp;
}

