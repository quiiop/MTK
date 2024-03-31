/* Copyright Statement:
 *
 * (C) 2005-2017  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#ifndef __SBC_DECODER_H__
#define __SBC_DECODER_H__

#include <stdint.h>

/** @brief SBC decoder status enumeration. */
typedef enum {
    SBC_DECODER_STATUS_OK            =  0,  /**<  No error occurred during the function call. */
    SBC_DECODER_STATUS_ERROR         = -1,  /**<  An error occurred during the function call. */
    SBC_DECODER_STATUS_INVALID_PARAM = -2   /**<  A wrong parameter is given. */
} sbc_decoder_status_t;

/**
 * @brief     Get the internal buffer size of the SBC media payload parser.
 * @param[in] internal_buffer_size is the internal buffer size in bytes.
 * @return    #SBC_DECODER_STATUS_OK, if OK.
 */
sbc_decoder_status_t sbc_media_payload_parser_get_buffer_size(uint32_t *internal_buffer_size);

/**
 * @brief     Initialize the SBC media payload parser handle.
 * @param[out] handle_pointer is the pointer to the SBC media payload parser handle.
 * @param[in] internal_buffer is the pointer to the internal buffer.
 * @param[in] magic_word is the magic word of the media payload header.
 * @return    #SBC_DECODER_STATUS_OK, if OK.
 */
sbc_decoder_status_t sbc_media_payload_parser_init(
    void **handle_pointer,
    uint8_t *internal_buffer,
    uint16_t magic_word
);

/**
 * @brief     Parse the SBC media payload into SBC bitstream.
 * @param[in] handle is the SBC media payload parser handle.
 * @param[in] input_media_payload is the pointer to the input media payload buffer.
 * @param[in] input_byte_count is the pointer to the available input media payload buffer byte count.
 * @param[out] input_byte_count is the pointer to the consumed input media payload buffer byte count.
 * @param[out] output_bitstream is the pointer to the output bitstream buffer.
 * @param[in] output_byte_count is the pointer to the available output bitstream buffer byte count.
 * @param[out] output_byte_count is the pointer to the produced output bitstream buffer byte count.
 * @return    #SBC_DECODER_STATUS_OK, if OK.
 */
sbc_decoder_status_t sbc_media_payload_parser_process(
    void *handle,
    uint8_t *input_media_payload,
    uint32_t *input_byte_count,
    uint8_t *output_bitstream,
    uint32_t *output_byte_count
);

/**
 * @brief     Get the internal buffer size of the SBC decoder.
 * @param[out] internal_buffer_size is the internal buffer size in bytes.
 * @return    #SBC_DECODER_STATUS_OK, if OK.
 */
sbc_decoder_status_t sbc_decoder_get_buffer_size(uint32_t *internal_buffer_size);

/**
 * @brief     Initialize the SBC decoder handle.
 * @param[out] handle_pointer is the pointer to the SBC decoder handle.
 * @param[in] internal_buffer is the pointer to the internal buffer.
 * @return    #SBC_DECODER_STATUS_OK, if OK.
 */
sbc_decoder_status_t sbc_decoder_init(
    void **handle_pointer,
    uint8_t *internal_buffer
);

/**
 * @brief     Decode the SBC bitstream to PCM data.
 * @param[in] handle is the SBC decoder handle.
 * @param[in] input_bitstream is the pointer to the input bitstream buffer.
 * @param[in] input_byte_count is the pointer to the available input bitstream buffer byte count.
 * @param[out] input_byte_count is the pointer to the consumed input bitstream buffer byte count.
 * @param[out] output_pcm is the pointer to the output PCM buffer.
 * @param[in] output_byte_count is the pointer to the available output PCM buffer byte count.
 * @param[out] output_byte_count is the pointer to the produced output PCM buffer byte count.
 * @return    #SBC_DECODER_STATUS_OK, if OK.
 */
sbc_decoder_status_t sbc_decoder_process(
    void *handle,
    uint8_t *input_bitstream,
    uint32_t *input_byte_count,
    int16_t *output_pcm,
    uint32_t *output_byte_count
);

#endif /* __SBC_DECODER_H__ */
