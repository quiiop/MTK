/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
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


#include <string.h>


#include "memory_attribute.h"

#include "task_def.h"


#include "mp3_codec_internal_7933.h"
#include "mp3_codec.h"
#include "audio_internal_service.h"

#include "hal_platform.h"
#include "hal_gpt.h"

#define MAX_MP3_CODEC_FUNCTIONS      3

#include "sound/include/tinypcm.h"
#include "audio_test_utils.h"
#include <stdlib.h>


static mp3_codec_internal_handle_t *mp3_codec_internal_handle = NULL;
PRIVILEGED_DATA static QueueHandle_t mp3_codec_queue_handle = NULL;
static uint32_t mp3_codec_queue_reg_num = 0;
static mp3_codec_queue_event_id_t mp3_codec_queue_id_array[MAX_MP3_CODEC_FUNCTIONS];
static mp3_codec_internal_callback_t mp3_codec_queue_handler[MAX_MP3_CODEC_FUNCTIONS];

/* tinypcm */
sound_t *w_snd;
int ret;
void *golden_src;
int golden_size;
struct msd_hw_params params;
int bytes_per_frame;

/*
#ifdef __ICCARM__
_Pragma("data_alignment=4") static uint8_t mp3_decode_buffer[41000] = {0};
#else
static __attribute__((__aligned__(4))) uint8_t mp3_decode_buffer[41000] = {0};
#endif
*/

#define MP3_DECODE_BUFFER_LENGTH 41000
static uint8_t *mp3_decode_buffer = NULL;


//static uint32_t mp3_task_stack_buf[1024];


/* for calculate MCPS
volatile int *DWT_CONTROL = (int *)0xE0001000;
volatile int *DWT_CYCCNT = (int *)0xE0001004;
volatile int *DEMCR = (int *)0xE000EDFC;
volatile uint32_t count_test = 0;
volatile uint32_t offset = 0;
#define CPU_RESET_CYCLECOUNTER do { *DEMCR = *DEMCR | 0x01000000; \
*DWT_CYCCNT = 0; \
*DWT_CONTROL = *DWT_CONTROL | 1 ; } while(0)
*/

#define ID3V2_HEADER_LENGTH 10  // in bytes
#define MPEG_AUDIO_FRAME_HEADER_LENGTH 4 // in bytes

/*
#define IS_MP3_HEAD(head) (!( ((head & 0xfff80000) != 0xfff80000) || \
  ( ((head>>17)&3)!= 1) || \
  ( ((head>>12)&0xf) == 0xf) || ( ((head>>12)&0xf) == 0x0) || \
( ((head>>10)&0x3) == 0x3 )))
*/

#define IS_MP3_HEAD(head) (!( (((head & 0xfff00000) != 0xfff00000) && ((head & 0xfff80000) != 0xffe00000) ) || \
  ( ((head>>17)&3)== 3) || (((head>>17)&3)== 0) || \
  ( ((head>>12)&0xf) == 0xf) || ( ((head>>12)&0xf) == 0x0) || \
( ((head>>10)&0x3) == 0x3 )))



#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_NA    0
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_32    1
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_40    2
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_48    3
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_56    4
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_64    5
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_80    6
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_96    7
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_112    8
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_128    9
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_160    0xa
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_192    0xb
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_224    0xc
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_256    0xd
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_320    0xe
#define MP3_MPEG_AUDIO_FRAME_BIT_RATE_NA2    0xf


/*  share buffer operation function */
static void mp3_codec_set_share_buffer(mp3_codec_media_handle_t *handle, uint8_t *buffer, uint32_t length)
{
    handle->share_buff.buffer_base = buffer;
    //length &= ~0x1; // make buffer size even
    handle->share_buff.buffer_size = length;
    handle->share_buff.write = 0;
    handle->share_buff.read = 0;
    handle->waiting = false;
    handle->underflow = false;
}

static void mp3_codec_get_share_buffer_write_information(mp3_codec_media_handle_t *handle, uint8_t **buffer, uint32_t *length)
{
    int32_t count = 0;

    if (handle->share_buff.read > handle->share_buff.write) {
        count = handle->share_buff.read - handle->share_buff.write - 1;
    } else if (handle->share_buff.read == 0) {
        count = handle->share_buff.buffer_size - handle->share_buff.write - 1;
    } else {
        count = handle->share_buff.buffer_size - handle->share_buff.write;
    }
    *buffer = handle->share_buff.buffer_base + handle->share_buff.write;
    *length = count;
}


static void mp3_codec_get_share_buffer_read_information(mp3_codec_media_handle_t *handle, uint8_t **buffer, uint32_t *length)
{
    int32_t count = 0;

    if (handle->share_buff.write >= handle->share_buff.read) {
        count = handle->share_buff.write - handle->share_buff.read;
    } else {
        count = handle->share_buff.buffer_size - handle->share_buff.read;
    }
    *buffer = handle->share_buff.buffer_base + handle->share_buff.read;
    *length = count;
}


static void mp3_codec_share_buffer_write_data_done(mp3_codec_media_handle_t *handle, uint32_t length)
{
    handle->share_buff.write += length;
    if (handle->share_buff.write == handle->share_buff.buffer_size) {
        handle->share_buff.write = 0;
    }
#if 0
    printf("[MP3 Codec ]mp3_codec_share_buffer_write_data_done:: handle->share_buff.write=%ld\r\n", handle->share_buff.write);
#endif /* #if 0 */
}

static void mp3_codec_finish_write_data(mp3_codec_media_handle_t *handle)
{
    handle->waiting = false;
    handle->underflow = false;
}

static void mp3_codec_flush(mp3_codec_media_handle_t *handle, int32_t flush_data_flag)
{
    handle->flush_data_flag = flush_data_flag;
    printf("[MP3 Codec] flush_data_flag = %ld\n", handle->flush_data_flag);
}

static void mp3_codec_reset_share_buffer(mp3_codec_media_handle_t *handle)
{
    handle->share_buff.write = 0;
    handle->share_buff.read = 0;
    handle->waiting = false;
    handle->underflow = false;
}


static void mp3_codec_share_buffer_read_data_done(mp3_codec_media_handle_t *handle, uint32_t length)
{
    handle->share_buff.read += length;
    if (handle->share_buff.read == handle->share_buff.buffer_size) {
        handle->share_buff.read = 0;
    }
}


static int32_t mp3_codec_get_share_buffer_free_space(mp3_codec_media_handle_t *handle)
{
    int32_t count = 0;

    count = handle->share_buff.read - handle->share_buff.write - 2;
    if (count < 0) {
        count += handle->share_buff.buffer_size;
    }
    return count;
}

static int32_t mp3_codec_get_share_buffer_data_count(mp3_codec_media_handle_t *handle)
{
    int32_t count = 0;

    count = handle->share_buff.write - handle->share_buff.read;
    if (count < 0) {
        count += handle->share_buff.buffer_size;
    }
    return count;
}

static void mp3_codec_reset_stream_out_pcm_buffer(void)
{
    mp3_codec_internal_handle_t *internal_handle = mp3_codec_internal_handle;
    internal_handle->stream_out_pcm_buff.read_pointer = 0;
    internal_handle->stream_out_pcm_buff.write_pointer = 0;
}

static void mp3_codec_buffer_function_init(mp3_codec_media_handle_t *handle)
{
    handle->set_share_buffer   = mp3_codec_set_share_buffer;
    handle->get_write_buffer   = mp3_codec_get_share_buffer_write_information;
    handle->get_read_buffer    = mp3_codec_get_share_buffer_read_information;
    handle->write_data_done    = mp3_codec_share_buffer_write_data_done;
    handle->finish_write_data  = mp3_codec_finish_write_data;
    handle->reset_share_buffer = mp3_codec_reset_share_buffer;
    handle->read_data_done     = mp3_codec_share_buffer_read_data_done;
    handle->get_free_space     = mp3_codec_get_share_buffer_free_space;
    handle->get_data_count     = mp3_codec_get_share_buffer_data_count;
    handle->flush              = mp3_codec_flush;
}


static uint32_t mp3_codec_get_bytes_from_share_buffer(mp3_codec_media_handle_t *handle, uint8_t *destination_buffer, uint32_t get_bytes_amount, bool want_move_read_ptr)
{
    uint8_t *share_buffer_read_address;
    uint32_t share_buffer_data_length;
    uint32_t share_buffer_read_index_original = 0;
    uint32_t bytes_amount_temp = get_bytes_amount;
    uint32_t got_bytes_amount = 0;  // real got bytes amount from share buffer

    share_buffer_read_index_original = handle->share_buff.read; // store original share_buffer read pointer


    uint16_t loop_idx = 0;
    for (loop_idx = 0; loop_idx < 2; loop_idx++) {
        mp3_codec_get_share_buffer_read_information(handle, &share_buffer_read_address, &share_buffer_data_length);
        if (share_buffer_data_length > 0) {
            uint32_t get_bytes_amount = MINIMUM(bytes_amount_temp, share_buffer_data_length);
            memcpy(destination_buffer, share_buffer_read_address, get_bytes_amount);
            bytes_amount_temp -= get_bytes_amount;
            destination_buffer += get_bytes_amount;
            mp3_codec_share_buffer_read_data_done(handle, get_bytes_amount);

            if (bytes_amount_temp == 0) {
                break;
            }
        } else {
            // share buffer empty
            break;
        }
    }


    got_bytes_amount = get_bytes_amount - bytes_amount_temp;  // real read amount

    if (got_bytes_amount != get_bytes_amount) {
        printf("[MP3 Codec]mp3_codec_get_bytes_from_share_buffer: got_bytes_amount(%ld) != get_bytes_amount(%ld)\r\n", got_bytes_amount, get_bytes_amount);
    }


    if (want_move_read_ptr == false) {
        handle->share_buff.read = share_buffer_read_index_original;
    }

    return got_bytes_amount;
}



static uint32_t mp3_codec_discard_bytes_of_share_buffer(mp3_codec_media_handle_t *handle, uint32_t discard_bytes_amount)
{
    uint8_t *share_buffer_read_address;
    uint32_t share_buffer_data_length;
    uint32_t bytes_amount_temp = discard_bytes_amount;
    uint32_t discarded_bytes_amount = 0;


    uint16_t loop_idx = 0;
    for (loop_idx = 0; loop_idx < 2; loop_idx++) {
        mp3_codec_get_share_buffer_read_information(handle, &share_buffer_read_address, &share_buffer_data_length);
        if (share_buffer_data_length > 0) {
            uint32_t get_bytes_amount = MINIMUM(bytes_amount_temp, share_buffer_data_length);
            bytes_amount_temp -= get_bytes_amount;
            mp3_codec_share_buffer_read_data_done(handle, get_bytes_amount);

            if (bytes_amount_temp == 0) {
                break;
            }
        } else {
            // share buffer empty
            break;
        }
    }

    discarded_bytes_amount = discard_bytes_amount - bytes_amount_temp;  // real read amount

    if (discarded_bytes_amount != discard_bytes_amount) {
        printf("[MP3 Codec]mp3_codec_discard_bytes_of_share_buffer : discarded_bytes_amount(%ld) != discard_bytes_amount(%ld)\r\n", discarded_bytes_amount, discard_bytes_amount);
    }


    return discarded_bytes_amount;
}

static mp3_codec_function_return_state_t mp3_codec_request_data_to_share_buffer(mp3_codec_media_handle_t *handle)
{
    // return MP3_CODEC_RETURN_OK:          request success
    // return  MP3_CODEC_RETURN_ERROR:    already request and waiting feed back

    if (!handle->waiting) {
        handle->waiting = true;
        handle->handler(handle, MP3_CODEC_MEDIA_REQUEST);
        return MP3_CODEC_RETURN_OK;
    } else {
        printf("[MP3 Codec] mp3_codec_request_data_to_share_buffer: already request and waiting feed back\r\n");
        return MP3_CODEC_RETURN_ERROR;
    }
}

static uint32_t mp3_codec_combine_four_bytes_buffer_to_uint32_value(uint8_t *buffer)
{
    uint32_t uint32_value = 0;

    uint32_value = *buffer;
    uint32_value = uint32_value << 8 | *(buffer + 1);
    uint32_value = uint32_value << 8 | *(buffer + 2);
    uint32_value = uint32_value << 8 | *(buffer + 3);

    return uint32_value;
}

static mp3_codec_function_return_state_t mp3_codec_reach_next_frame_and_get_audio_frame_header(mp3_codec_media_handle_t *handle, uint32_t *audio_frame_header, uint32_t maximum_check_bytes, uint32_t want_skip_frame_after_got_header)
{
    uint8_t check_mpeg_header_buffer[MPEG_AUDIO_FRAME_HEADER_LENGTH] = {0};
    uint32_t temp_mpeg_header = 0;
    uint32_t discard_bytes_amount = 0;
    uint32_t temp_uint32_t = 0;
    uint32_t temp_maximum_check_bytes = maximum_check_bytes;
    uint32_t maximum_request_data_time = maximum_check_bytes / handle->share_buff.buffer_size + 2;  // 2: arbitrarily selected


    do {
        if (mp3_codec_get_share_buffer_data_count(handle) < MPEG_AUDIO_FRAME_HEADER_LENGTH) {

            if (mp3_codec_request_data_to_share_buffer(handle) == MP3_CODEC_RETURN_OK) {
                maximum_request_data_time--;
            }

            if (mp3_codec_get_share_buffer_data_count(handle) < MPEG_AUDIO_FRAME_HEADER_LENGTH) {
                return MP3_CODEC_RETURN_ERROR;
            }
        }

        mp3_codec_get_bytes_from_share_buffer(handle, check_mpeg_header_buffer, MPEG_AUDIO_FRAME_HEADER_LENGTH, 0);
        temp_mpeg_header = mp3_codec_combine_four_bytes_buffer_to_uint32_value(check_mpeg_header_buffer);

        if (IS_MP3_HEAD(temp_mpeg_header)) {
            // find MP3 HEAD
            *audio_frame_header = temp_mpeg_header;

            if (want_skip_frame_after_got_header) {
                discard_bytes_amount = 4;
                temp_uint32_t = mp3_codec_discard_bytes_of_share_buffer(handle, discard_bytes_amount);
                if (temp_uint32_t < discard_bytes_amount) {  // share buffer didn't have enoungh data to discared
                    return MP3_CODEC_RETURN_ERROR;
                }
            }
            // printf("[MP3 Codec]mp3_codec_reach_next_frame: find mp3 header=%x\r\n", *audio_frame_header);
            return MP3_CODEC_RETURN_OK;
        }

        discard_bytes_amount = 1;
        temp_uint32_t = MINIMUM(discard_bytes_amount, temp_maximum_check_bytes);
        temp_uint32_t = mp3_codec_discard_bytes_of_share_buffer(handle, temp_uint32_t);
        if (temp_uint32_t < discard_bytes_amount) {  // share buffer didn't have enoungh data to discared
            return MP3_CODEC_RETURN_ERROR;
        }

        temp_maximum_check_bytes -= temp_uint32_t;

    } while (temp_maximum_check_bytes != 0 && maximum_request_data_time != 0);


    printf("[MP3 Codec]mp3_codec_reach_next_frame: not find mp3 header\r\n");
    *audio_frame_header = 0;

    return MP3_CODEC_RETURN_ERROR;
}


static void mp3_codec_get_id3v2_info_and_skip(mp3_codec_media_handle_t *handle, uint32_t file_size)
{
    uint32_t want_get_bytes = 0;
    uint8_t temp_buffer[10] = {0};
    uint32_t id3v2_remain_tagesize = 0; // not include ID3v2 header size, refert to ID3v2 spec
    uint32_t id3v2_tage_size = 0;   // total ID3v2 tage size which include ID3v2 header
    uint32_t remain_file_data_size = file_size; // in bytes

    handle->jump_file_to_specified_position = 0;    // asume from file begin
    handle->id3v2_information.has_id3v2 = false;
    handle->id3v2_information.id3v2_tage_length = 0;

    while (1) {
        //printf("MOM I AM HERE!!!\n");
        want_get_bytes = ID3V2_HEADER_LENGTH;
        if (mp3_codec_get_bytes_from_share_buffer(handle, temp_buffer, want_get_bytes, 0) != want_get_bytes) {
            printf("[MP3 Codec]mp3_codec_get_id3v2_info_and_skip: share buffer data amount less than ID3v2 header length\r\n");
            return;    // just return
        }

        if (strncmp((const char *)temp_buffer, "ID3", 3) == 0) {
            id3v2_remain_tagesize = ((temp_buffer[6] & 0x7f) << 21) | ((temp_buffer[7] & 0x7f) << 14) | ((temp_buffer[8] & 0x7f) <<  7) | ((temp_buffer[9] & 0x7f) <<  0);
            id3v2_tage_size = id3v2_remain_tagesize + ID3V2_HEADER_LENGTH;
            printf("[MP3 Codec]find id3v2: id3v2_tagesize=%ld, id3v2_remain_tagesize =%ld\r\n", id3v2_tage_size, id3v2_remain_tagesize);


            if (remain_file_data_size < id3v2_tage_size) {
                // the tag size calculate form ID3v2 may wrong
                return;
            }


            handle->id3v2_information.has_id3v2 = true;
            handle->id3v2_information.id3v2_tage_length += id3v2_tage_size;


            // Although the remaing data in share buffer can be used,
            // but the fast and clear way to skip ID3v2 is just ask user to jump file to specific position and refill the share buffer
            mp3_codec_reset_share_buffer(handle);   // since we want to ask user to jump file to specific position and get data, thus remaining data is no use.
            handle->jump_file_to_specified_position += id3v2_tage_size;
            handle->handler(handle, MP3_CODEC_MEDIA_JUMP_FILE_TO);

            mp3_codec_request_data_to_share_buffer(handle);


            remain_file_data_size -= id3v2_tage_size;


        } else {
            //            printf("[MP3 Codec]done skip ID3v2, has_id3v2=%d, id3v2_tage_length=%d\r\n", (uint32_t)handle->id3v2_information.has_id3v2, handle->id3v2_information.id3v2_tage_length);
            return;
        }

    }
}


#if 1
mp3_codec_function_return_state_t mp3_codec_set_memory2(void)
{
    if (mp3_codec_internal_handle == NULL) {
        log_hal_error("[MP3 Codec]mp3_codec_internal_handle = NULL\r\n");
        return MP3_CODEC_RETURN_ERROR;
    }

    mp3_codec_internal_handle_t *internal_handle = mp3_codec_internal_handle;
    mp3_codec_media_handle_t *handle = &internal_handle->handle;

    if (handle->state != MP3_CODEC_STATE_READY) {
        return MP3_CODEC_RETURN_ERROR;
    }

    uint8_t *memory_base = NULL;
    int working_buff1_size,         /* the required Working buffer1 size in byte    */
        working_buff2_size,         /* the required Working buffer2 size in byte    */
        decode_pcm_buffer_size,              /* the required pcm buffer size in byte          */
        share_buff_size;            /* the share buffer size      */

    /*STEP 1 : Allocate data memory for MP3 decoder*/
    MP3Dec_GetMemSize(&share_buff_size, &decode_pcm_buffer_size, &working_buff1_size, &working_buff2_size);

    //share_buff_size += 1024;     // piter delete

    //4bytes aligned
    share_buff_size = (share_buff_size + 3) & ~0x3;
    decode_pcm_buffer_size = (decode_pcm_buffer_size + 3) & ~0x3;
    working_buff1_size = (working_buff1_size + 3) & ~0x3;
    working_buff2_size = (working_buff2_size + 3) & ~0x3;

    internal_handle->share_buff_size = share_buff_size;
    internal_handle->decode_pcm_buffer_size = decode_pcm_buffer_size;
    internal_handle->working_buff1_size = working_buff1_size;
    internal_handle->working_buff2_size = working_buff2_size;
    internal_handle->stream_out_pcm_buff_size = decode_pcm_buffer_size * 3;

    //specify memory pool
    internal_handle->memory_pool = memory_base = mp3_decode_buffer;

    //set share buffer
    mp3_codec_set_share_buffer(handle, memory_base, share_buff_size);
    memory_base += share_buff_size;

    // set decode_pcm_buffer
    internal_handle->decode_pcm_buff.buffer_base_pointer = memory_base;
    internal_handle->decode_pcm_buff.buffer_byte_count = internal_handle->decode_pcm_buffer_size;
    internal_handle->decode_pcm_buff.read_pointer = 0;
    internal_handle->decode_pcm_buff.write_pointer = 0;
    memory_base += internal_handle->decode_pcm_buffer_size;


    //set working buffer
    internal_handle->working_buff1 = memory_base;
    memory_base += internal_handle->working_buff1_size;
    internal_handle->working_buff2 = memory_base;
    memory_base += internal_handle->working_buff2_size;

    // set PCM buffer
    internal_handle->stream_out_pcm_buff.buffer_base_pointer = memory_base;
    internal_handle->stream_out_pcm_buff.buffer_byte_count = internal_handle->stream_out_pcm_buff_size;
    internal_handle->stream_out_pcm_buff.read_pointer = 0;
    internal_handle->stream_out_pcm_buff.write_pointer = 0;




    /*STEP 2 : Get MP3 Handler */
    internal_handle->mp3_handle = MP3Dec_Init(internal_handle->working_buff1, internal_handle->working_buff2);
    if (internal_handle->mp3_handle == NULL) {
        log_hal_error("[MP3 Codec]MP3Dec_Init fail");
    }

#if 0
    printf("[MP3 Codec]set_memory memory range : start=%08x end=%08x\n",
           (unsigned int)&mp3_decode_buffer[0], (unsigned int)&mp3_decode_buffer[40999]);
    printf("[MP3 Codec]set_memory share buffer : base=%08x size=%u\n",
           (unsigned int)handle->share_buff.buffer_base, (unsigned int)handle->share_buff.buffer_size);
    printf("[MP3 Codec]set_memory decode_pcm_buff.buffer : base=%08x size=%u\n",
           (unsigned int)internal_handle->decode_pcm_buff.buffer_base_pointer, (unsigned int)internal_handle->decode_pcm_buff.buffer_byte_count);
    printf("[MP3 Codec]set_memory work1 buffer : base=%08x size=%u\n",
           (unsigned int)internal_handle->working_buff1, (unsigned int)internal_handle->working_buff1_size);
    printf("[MP3 Codec]set_memory work2 buffer : base=%08x size=%u\n",
           (unsigned int)internal_handle->working_buff2, (unsigned int)internal_handle->working_buff2_size);
    printf("[MP3 Codec]set_memory stream_out_pcm_buff : base=%08x size=%u\n",
           (unsigned int)internal_handle->stream_out_pcm_buff.buffer_base_pointer, (unsigned int)internal_handle->stream_out_pcm_buff.buffer_byte_count);
#endif /* #if 0 */

    return MP3_CODEC_RETURN_OK;

}
#endif /* #if 1 */


static void mp3_codec_event_send_from_isr(mp3_codec_queue_event_id_t id, void *parameter)
{
    mp3_codec_queue_event_t event;
    event.id        = id;
    event.parameter = parameter;
    if (xQueueSendFromISR(mp3_codec_queue_handle, &event, 0) != pdPASS) {
        printf("[MP3 Codec]queue not pass %d\r\n", id);
        return;
    }

    return;
}

static void mp3_codec_event_register_callback(mp3_codec_queue_event_id_t reg_id, mp3_codec_internal_callback_t callback)
{
    uint32_t id_idx;
    for (id_idx = 0; id_idx < MAX_MP3_CODEC_FUNCTIONS; id_idx++) {
        if (mp3_codec_queue_id_array[id_idx] == MP3_CODEC_QUEUE_EVENT_NONE) {
            mp3_codec_queue_id_array[id_idx] = reg_id;
            mp3_codec_queue_handler[id_idx] = callback;
            mp3_codec_queue_reg_num++;
            break;
        }
    }
    return;
}

static void mp3_codec_event_deregister_callback(mp3_codec_queue_event_id_t dereg_id)
{
    uint32_t id_idx;
    for (id_idx = 0; id_idx < MAX_MP3_CODEC_FUNCTIONS; id_idx++) {
        if (mp3_codec_queue_id_array[id_idx] == dereg_id) {
            mp3_codec_queue_id_array[id_idx] = MP3_CODEC_QUEUE_EVENT_NONE;
            mp3_codec_queue_reg_num--;
            break;
        }
    }
    return;
}


static void mp3_codec_task_main(void *arg)
{
    printf("[MP3 Codec]mp3_codec_task_main create\r\n");

    mp3_codec_queue_event_t event;

    while (1) {
        if (xQueueReceive(mp3_codec_queue_handle, &event, portMAX_DELAY)) {
            //printf("[MP3 Codec]xQueueReceive event\r\n");
            mp3_codec_queue_event_id_t rece_id = event.id;
            uint32_t id_idx;
            for (id_idx = 0; id_idx < MAX_MP3_CODEC_FUNCTIONS; id_idx++) {
                if (mp3_codec_queue_id_array[id_idx] == rece_id) {
                    //printf("[MP3 Codec]xQueueReceive find event_id mp3_codec_queue_id_array[%d]=%d\r\n",id_idx,event.id);
                    mp3_codec_queue_handler[id_idx](event.parameter);
                    break;
                }
            }
        }
    }
}


static uint16_t mp3_codec_translate_sample_rate(uint16_t sample_rate_index)
{
    uint16_t sampleRate = 44100;
    switch (sample_rate_index) {
        case 0: // mp3 decoder SWIP = 44.1k
            sampleRate = 44100;
            break;
        case 1: // mp3 decoder SWIP = 48k
            sampleRate = 48000;
            break;
        case 2: // mp3 decoder SWIP = 32k
            sampleRate = 32000;
            break;
        case 3: // mp3 decoder SWIP = 22.05k
            sampleRate = 22050;
            break;
        case 4: // mp3 decoder SWIP = 24k
            sampleRate = 24000;
            break;
        case 5: // mp3 decoder SWIP = 16k
            sampleRate = 16000;
            break;
        case 6: // mp3 decoder SWIP = 11.025k
            sampleRate = 11025;
            break;
        case 7: // mp3 decoder SWIP = 12k
            sampleRate = 12000;
            break;
        case 8: // mp3 decoder SWIP = 8k
            sampleRate = 8000;
            break;
    }
    return sampleRate;
}

typedef enum {
    MP3_CMD_QUEUE_EVENT_PLAY,
    MP3_CMD_QUEUE_EVENT_PAUSE,
    MP3_CMD_QUEUE_EVENT_STOP,
    MP3_CMD_QUEUE_EVENT_RESUME,
    MP3_CMD_QUEUE_EVENT_CLOSE
} mp3_cmd_queue_event_id_t;

TaskHandle_t xPlayTaskHandle = NULL;
TaskHandle_t xDecodeTaskHandle = NULL;
PRIVILEGED_DATA static QueueHandle_t mp3_cmd_queue_handle = NULL;
uint8_t ucPCMOpened = 0; // to prevent open or close pcm twice
int16_t *g_mp3_silence = NULL;

static void mp3_play_task_main(void *arg)
{
    mp3_codec_internal_handle_t *internal_handle = mp3_codec_internal_handle;
    uint16_t silence_size = params.period_count * params.period_size * internal_handle->mp3_handle->CHNumber * 2;
    g_mp3_silence = (int16_t *)malloc(silence_size * sizeof(int16_t));
    memset(g_mp3_silence, 0, silence_size * sizeof(int16_t));
    mp3_cmd_queue_event_id_t pCmd;
    while (1) {
        if (xQueueReceive(mp3_cmd_queue_handle, &pCmd, (TickType_t)portMAX_DELAY) == pdPASS) {
            switch (pCmd) {
                case MP3_CMD_QUEUE_EVENT_PLAY: {
                        //play pcm here
                        uint32_t stream_out_pcm_buffer_data_count = 0;
                        uint8_t *out_buf_ptr    = NULL;
                        ring_buffer_get_read_information(&internal_handle->stream_out_pcm_buff, &out_buf_ptr, &stream_out_pcm_buffer_data_count);
                        uint32_t stream_out_pcm_buffer_data_tone_amount = (internal_handle->mp3_handle->CHNumber == 1) ? (stream_out_pcm_buffer_data_count / 2) : (stream_out_pcm_buffer_data_count / 4);

                        if (stream_out_pcm_buffer_data_tone_amount > 0) {
                            int total_frames = stream_out_pcm_buffer_data_tone_amount;
                            if (ucPCMOpened == 1) {
                                ret = snd_pcm_write(w_snd, out_buf_ptr, total_frames);
                            }
                            if (internal_handle->mp3_handle->CHNumber == 1) {
                                ring_buffer_read_done(&internal_handle->stream_out_pcm_buff, (stream_out_pcm_buffer_data_tone_amount * 2));
                            } else {
                                ring_buffer_read_done(&internal_handle->stream_out_pcm_buff, (stream_out_pcm_buffer_data_tone_amount * 4));
                            }

                        } else {
                            // stream_out_pcm_buff no data
                            //printf("stream_out_pcm_buff no data\n");
                            if (ucPCMOpened == 1) {
                                snd_pcm_write(w_snd, g_mp3_silence, silence_size / 2);
                            }
                        }

                        if (ring_buffer_get_space_byte_count(&internal_handle->stream_out_pcm_buff) >= internal_handle->decode_pcm_buffer_size) {
                            mp3_codec_event_send_from_isr(MP3_CODEC_QUEUE_EVENT_DECODE, NULL);
                        }
                        break;
                    }
                case MP3_CMD_QUEUE_EVENT_PAUSE:
                case MP3_CMD_QUEUE_EVENT_STOP:
                case MP3_CMD_QUEUE_EVENT_CLOSE: {
                        if (ucPCMOpened == 1) {
                            ret = snd_pcm_drain(w_snd);
                            ret = snd_pcm_hw_free(w_snd);
                            snd_pcm_close(w_snd);
                            connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
                            connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
                            connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
                            ucPCMOpened = 0;
                        }
                        break;
                    }
                case MP3_CMD_QUEUE_EVENT_RESUME: {
                        if (ucPCMOpened == 0) {
                            connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
                            connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
                            connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);

                            ret = snd_pcm_open(&w_snd, "track0", 0, 0);
                            ret = snd_pcm_hw_params(w_snd, &params);
                            ret = snd_pcm_prepare(w_snd);

                            mp3_cmd_queue_event_id_t cmd = MP3_CMD_QUEUE_EVENT_PLAY;
                            xQueueSend(mp3_cmd_queue_handle, &cmd, (TickType_t) 0);
                            ucPCMOpened = 1;
                        }
                        break;
                    }
                default:
                    break;
            }
        }
    }

    return;
}

static void mp3_play_task_create(void)
{
    xTaskCreate(mp3_play_task_main, "MP3_PLAY", 1024, NULL, TASK_PRIORITY_TIMER - 1, &xPlayTaskHandle);
}

static void mp3_codec_task_create(void)
{
    xTaskCreate(mp3_codec_task_main, MP3_CODEC_TASK_NAME, MP3_CODEC_TASK_STACKSIZE / sizeof(StackType_t), NULL, MP3_CODEC_TASK_PRIO, &xDecodeTaskHandle);
}

static void mp3_codec_deocde_hisr_handler(void *data)
{
    int32_t consumeBS = -1;
    uint8_t *share_buff_read_ptr = NULL;

    mp3_codec_internal_handle_t *internal_handle = mp3_codec_internal_handle;
    mp3_codec_media_handle_t *handle = &internal_handle->handle;

    if (handle->state != MP3_CODEC_STATE_PLAY) {
        printf("[MP3 Codec] mp3_codec_deocde_hisr_handler() just return because handle->state = %d\r\n", handle->state);
        return;
    }

    if (1 == internal_handle->media_bitstream_end) {
        printf("[MP3 Codec] handle->handler(handle, MP3_CODEC_MEDIA_BITSTREAM_END);\r\n");
        handle->handler(handle, MP3_CODEC_MEDIA_BITSTREAM_END);
        return;
    }

    //update read ptr to share buffer
    share_buff_read_ptr = handle->share_buff.buffer_base + handle->share_buff.read;

    int32_t share_data_amount = mp3_codec_get_share_buffer_data_count(handle);

    if ((1 == handle->flush_data_flag) && (share_data_amount <= MPEG_AUDIO_FRAME_HEADER_LENGTH)) {
        uint8_t *out_buf_ptr    = NULL;
        uint32_t stream_out_pcm_buffer_data_count = 0;
        ring_buffer_get_read_information(&internal_handle->stream_out_pcm_buff, &out_buf_ptr, &stream_out_pcm_buffer_data_count);

        if (0 == stream_out_pcm_buffer_data_count) {
            internal_handle->is_set_eof_called = 1;
        }
    }

    if (share_data_amount <= MPEG_AUDIO_FRAME_HEADER_LENGTH) {
        printf("[MP3 Codec] share_data_amount <= MPEG_AUDIO_FRAME_HEADER_LENGTH\n");
        mp3_codec_request_data_to_share_buffer(handle);
        return;
    }

    /*   Deocde mp3 frame
        *   return: The consumed data size of Bitsream buffer for this  frame
      */
    consumeBS = MP3Dec_Decode(internal_handle->mp3_handle,
                              internal_handle->decode_pcm_buff.buffer_base_pointer,
                              handle->share_buff.buffer_base,
                              handle->share_buff.buffer_size,
                              share_buff_read_ptr);

    if (consumeBS < 0) {
        printf("[MP3 Codec] Invalid return , consumeBS= %d\r\n", (int)consumeBS);
        //todo, error frame handle
    } else if (consumeBS >= 0 && consumeBS <= share_data_amount) {
        //update read index to share buffer
        handle->share_buff.read += consumeBS;
        if (handle->share_buff.read >= handle->share_buff.buffer_size) {    // although share buffer is a ring buffer, the mp3 decoder ip only code to the end of buffer at most.
            handle->share_buff.read -= handle->share_buff.buffer_size;
        }
    } else if (consumeBS > share_data_amount) {
        consumeBS = share_data_amount;  // It strange here, in fact, mp3 SWIP consumBS must not > share_data_amount. we met consumBS > share_data_amount in playing combine two songs in one mp3 file, so just workaround here.
        handle->share_buff.read += consumeBS;
        if (handle->share_buff.read >= handle->share_buff.buffer_size) {    // although share buffer is a ring buffer, the mp3 decoder ip only code to the end of buffer at most.
            handle->share_buff.read -= handle->share_buff.buffer_size;
        }

        uint32_t decoded_data_count = internal_handle->mp3_handle->PCMSamplesPerCH * internal_handle->mp3_handle->CHNumber * 2;

        decoded_data_count = ((int32_t)decoded_data_count) < 0 ? 0 : decoded_data_count;

        memset(internal_handle->decode_pcm_buff.buffer_base_pointer, 0, decoded_data_count);
    } else {
        printf("[MP3 Codec] never hear\r\n");
    }


    if ((internal_handle->mp3_handle->CHNumber != -1) && (internal_handle->mp3_handle->sampleRateIndex != -1)) {
        uint32_t loop_idx = 0;
        uint32_t decoded_data_count = internal_handle->mp3_handle->PCMSamplesPerCH * internal_handle->mp3_handle->CHNumber * 2;
        uint32_t decoded_data_index = 0;
        for (loop_idx = 0; loop_idx < 2; loop_idx++) {
            uint8_t *out_buf_ptr    = NULL;
            uint32_t out_byte_count = 0;
            ring_buffer_get_write_information(&internal_handle->stream_out_pcm_buff, &out_buf_ptr, &out_byte_count);
            if (out_byte_count > 0) {
                uint32_t consumed_byte_count = MINIMUM(decoded_data_count, out_byte_count);
                uint8_t *p_in_base         = internal_handle->decode_pcm_buff.buffer_base_pointer;
                uint8_t *p_in_buf          = p_in_base + decoded_data_index;
                memcpy(out_buf_ptr, p_in_buf, consumed_byte_count);
                decoded_data_index += consumed_byte_count;
                decoded_data_count  -= consumed_byte_count;

                ring_buffer_write_done(&internal_handle->stream_out_pcm_buff, consumed_byte_count);

            } else {
                // stream_out_pcm_buffer full, do nothing.
                break;
            }
        }
    }

    if (mp3_codec_get_share_buffer_data_count(handle) <= 1566) {    // share buufer less than decode bs * 3, we request user to fill data
        mp3_codec_request_data_to_share_buffer(handle);
    }

    if (ring_buffer_get_space_byte_count(&internal_handle->stream_out_pcm_buff) >= internal_handle->decode_pcm_buffer_size) {
        mp3_codec_event_send_from_isr(MP3_CODEC_QUEUE_EVENT_DECODE, NULL);
    } else {
        mp3_cmd_queue_event_id_t cmd = MP3_CMD_QUEUE_EVENT_PLAY;
        xQueueSend(mp3_cmd_queue_handle, &cmd, (TickType_t) 0);
    }
}

static mp3_codec_function_return_state_t mp3_codec_skip_id3v2_and_reach_next_frame(mp3_codec_media_handle_t *handle, uint32_t file_size)
{
    uint32_t auido_frame_header = 0;
    if (mp3_codec_get_share_buffer_data_count(handle) < ID3V2_HEADER_LENGTH) {
        return MP3_CODEC_RETURN_ERROR;
    }

    mp3_codec_get_id3v2_info_and_skip(handle, file_size);

    if (mp3_codec_reach_next_frame_and_get_audio_frame_header(handle, &auido_frame_header, 2048, 0) != MP3_CODEC_RETURN_OK) {
        return MP3_CODEC_RETURN_ERROR;
    }

    return MP3_CODEC_RETURN_OK;
}

static mp3_codec_function_return_state_t mp3_codec_play_internal(mp3_codec_media_handle_t *handle)
{
    int32_t consumeBS = -1;
    uint8_t *share_buff_read_ptr = NULL;
    int16_t max_loop_times = 20;    // arbitrarily selected

    printf("[MP3 Codec] mp3_codec_play_internal ++\r\n");


    mp3_codec_internal_handle_t *internal_handle = mp3_codec_internal_handle;


    mp3_codec_event_register_callback(MP3_CODEC_QUEUE_EVENT_DECODE, mp3_codec_deocde_hisr_handler);

    do {
        //update read ptr to share buffer
        share_buff_read_ptr = handle->share_buff.buffer_base + handle->share_buff.read;


        consumeBS = MP3Dec_Decode(internal_handle->mp3_handle,
                                  internal_handle->decode_pcm_buff.buffer_base_pointer,
                                  handle->share_buff.buffer_base,
                                  handle->share_buff.buffer_size,
                                  share_buff_read_ptr);


        if (consumeBS < 0) {
            printf("[MP3 Codec] Invalid return , consumeBS= %d\r\n", (int)consumeBS);
        }

        if ((internal_handle->mp3_handle->CHNumber != -1) && (internal_handle->mp3_handle->sampleRateIndex != -1)) {
            {
                //update read index to share buffer
                handle->share_buff.read += consumeBS;
                if (handle->share_buff.read >= handle->share_buff.buffer_size) {
                    handle->share_buff.read -= handle->share_buff.buffer_size;
                }
            }

            uint32_t loop_idx = 0;
            uint32_t decoded_data_count = internal_handle->mp3_handle->PCMSamplesPerCH * internal_handle->mp3_handle->CHNumber * 2;
            uint32_t decoded_data_index = 0;

            for (loop_idx = 0; loop_idx < 2; loop_idx++) {
                uint8_t *out_buf_ptr    = NULL;
                uint32_t out_byte_count = 0;
                ring_buffer_get_write_information(&internal_handle->stream_out_pcm_buff, &out_buf_ptr, &out_byte_count);
                if (out_byte_count > 0) {
                    uint32_t consumed_byte_count = MINIMUM(decoded_data_count, out_byte_count);
                    uint8_t *p_in_base         = internal_handle->decode_pcm_buff.buffer_base_pointer;
                    uint8_t *p_in_buf          = p_in_base + decoded_data_index;
                    memcpy(out_buf_ptr, p_in_buf, consumed_byte_count);
                    decoded_data_index += consumed_byte_count;
                    decoded_data_count  -= consumed_byte_count;
                    ring_buffer_write_done(&internal_handle->stream_out_pcm_buff, consumed_byte_count);
                } else {
                    // stream_out_pcm_buffer full, do nothing.
                    break;
                }

            }

        }

        max_loop_times--;

        if (max_loop_times < 0) {
            return MP3_CODEC_RETURN_ERROR;
        }
    } while (ring_buffer_get_space_byte_count(&internal_handle->stream_out_pcm_buff) >= 4608);

    // mp3_codec_request_data_to_share_buffer(handle); // since we decoded some data, thus we request again to fill share buffer.


    /*TODO:   Tinypcm Playback*/

    handle->state = MP3_CODEC_STATE_PLAY;

    if (ucPCMOpened == 0) {
        params.format = MSD_PCM_FMT_S16_LE;
        params.channels = internal_handle->mp3_handle->CHNumber;
        params.period_count = 4;
        params.period_size = 1280;
        params.rate = mp3_codec_translate_sample_rate((uint16_t)internal_handle->mp3_handle->sampleRateIndex);
        bytes_per_frame = 16 * params.channels / 8;

        connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
        connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
        connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);

        ret = snd_pcm_open(&w_snd, "track0", 0, 0);
        ret = snd_pcm_hw_params(w_snd, &params);
        ret = snd_pcm_prepare(w_snd);
        ucPCMOpened = 1;
    }

    mp3_cmd_queue_event_id_t cmd = MP3_CMD_QUEUE_EVENT_PLAY;
    xQueueSend(mp3_cmd_queue_handle, &cmd, (TickType_t) 0);

    printf("[MP3 Codec] mp3_codec_play_internal --\r\n");
    return MP3_CODEC_RETURN_OK;
}


static mp3_codec_function_return_state_t mp3_codec_resume(mp3_codec_media_handle_t *handle)
{
    if (handle == NULL || handle->state != MP3_CODEC_STATE_PAUSE || mp3_cmd_queue_handle == NULL) {
        printf("%s: error, 0x%x 0x%x\r\n", __func__, (unsigned int)handle, (unsigned int)mp3_cmd_queue_handle);
        return MP3_CODEC_RETURN_ERROR;
    }

    //printf("[MP3 Codec] resume++: stream out pcm data=%d, share buffer data=%d\r\n", ring_buffer_get_data_byte_count(&mp3_codec_internal_handle->stream_out_pcm_buff), mp3_codec_get_share_buffer_data_count(handle));
    printf("[MP3 Codec] resume++\r\n");

    //printf("[MP3 Codec] resume--: stream out pcm data=%d, share buffer data=%d\r\n", ring_buffer_get_data_byte_count(&mp3_codec_internal_handle->stream_out_pcm_buff), mp3_codec_get_share_buffer_data_count(handle));
    printf("[MP3 Codec] resume--\r\n");
    handle->state = MP3_CODEC_STATE_PLAY;

    /*TODO:   Tinypcm Resume*/
    // send command here to avoid play task preempt and the handle state is not changed successfully
    mp3_cmd_queue_event_id_t cmd = MP3_CMD_QUEUE_EVENT_RESUME;
    xQueueSend(mp3_cmd_queue_handle, &cmd, (TickType_t) 0);

    return MP3_CODEC_RETURN_OK;
}

static mp3_codec_function_return_state_t mp3_codec_pause(mp3_codec_media_handle_t *handle)
{
    if (handle == NULL || handle->state != MP3_CODEC_STATE_PLAY || mp3_cmd_queue_handle == NULL) {
        printf("%s: error, 0x%x 0x%x\r\n", __func__, (unsigned int)handle, (unsigned int)mp3_cmd_queue_handle);
        return MP3_CODEC_RETURN_ERROR;
    }

    printf("[MP3 Codec] pause++: stream out pcm data=%d, share buffer data=%d\r\n", (int)ring_buffer_get_data_byte_count(&mp3_codec_internal_handle->stream_out_pcm_buff), (int)mp3_codec_get_share_buffer_data_count(handle));

    /*TODO:   Tinypcm Pause*/
    mp3_cmd_queue_event_id_t cmd = MP3_CMD_QUEUE_EVENT_PAUSE;
    xQueueSend(mp3_cmd_queue_handle, &cmd, (TickType_t) 0);

    printf("[MP3 Codec] pause--: stream out pcm data=%d, share buffer data=%d\r\n", (int)ring_buffer_get_data_byte_count(&mp3_codec_internal_handle->stream_out_pcm_buff), (int)mp3_codec_get_share_buffer_data_count(handle));

    handle->state = MP3_CODEC_STATE_PAUSE;
    return MP3_CODEC_RETURN_OK;
}

static mp3_codec_function_return_state_t mp3_codec_play(mp3_codec_media_handle_t *handle)
{
    if (handle->state != MP3_CODEC_STATE_READY && handle->state != MP3_CODEC_STATE_STOP) {
        return MP3_CODEC_RETURN_ERROR;
    }

    mp3_codec_internal_handle_t *internal_handle = mp3_codec_internal_handle;

    internal_handle->mp3_handle = MP3Dec_Init(internal_handle->working_buff1, internal_handle->working_buff2);  // must do this, or SW mp3 IP will work wrong
    if (internal_handle->mp3_handle == NULL) {
        return MP3_CODEC_RETURN_ERROR;
    }

    internal_handle->media_bitstream_end = 0;
    handle->flush_data_flag = 0;
    internal_handle->is_set_eof_called = 0;
    return mp3_codec_play_internal(handle);
}


static mp3_codec_function_return_state_t mp3_codec_stop(mp3_codec_media_handle_t *handle)
{
    //if (handle->state != MP3_CODEC_STATE_PLAY && handle->state != MP3_CODEC_STATE_PAUSE) {
    //    return MP3_CODEC_RETURN_ERROR;
    //}
#if defined(MTK_AUDIO_MIXER_SUPPORT)
    printf("[MP3 Codec] stop++: handle(%X) role%d\r\n", handle, handle->mixer_track_role);
#else /* #if defined(MTK_AUDIO_MIXER_SUPPORT) */
    printf("[MP3 Codec] stop++\r\n");
#endif /* #if defined(MTK_AUDIO_MIXER_SUPPORT) */

    mp3_codec_event_deregister_callback(MP3_CODEC_QUEUE_EVENT_DECODE);

    mp3_codec_reset_share_buffer(handle);
    mp3_codec_reset_stream_out_pcm_buffer();    // if don't do this it will have residue data, and it will be played if you play again

    if (handle->state != MP3_CODEC_STATE_PLAY && handle->state != MP3_CODEC_STATE_PAUSE) {
        return MP3_CODEC_RETURN_ERROR;
    }

    handle->state = MP3_CODEC_STATE_STOP;

#if defined(MTK_AUDIO_MIXER_SUPPORT)
    printf("[MP3 Codec] stop--: handle(%X) role%d\r\n", handle, handle->mixer_track_role);
#else /* #if defined(MTK_AUDIO_MIXER_SUPPORT) */
    printf("[MP3 Codec] stop--\r\n");
#endif /* #if defined(MTK_AUDIO_MIXER_SUPPORT) */

    /*TODO:   Tinypcm Stop*/
    mp3_cmd_queue_event_id_t cmd = MP3_CMD_QUEUE_EVENT_STOP;
    xQueueSend(mp3_cmd_queue_handle, &cmd, (TickType_t) 0);


    return MP3_CODEC_RETURN_OK;
}


mp3_codec_function_return_state_t mp3_codec_close(mp3_codec_media_handle_t *handle)
{
    mp3_codec_internal_handle_t *internal_handle = (mp3_codec_internal_handle_t *) handle;

    printf("[MP3 Codec]Close codec\r\n");
    if (handle->state != MP3_CODEC_STATE_STOP) {
        return MP3_CODEC_RETURN_ERROR;
    }
    handle->state = MP3_CODEC_STATE_IDLE;

    /*TODO:   Tinypcm Close*/
    mp3_cmd_queue_event_id_t cmd = MP3_CMD_QUEUE_EVENT_CLOSE;
    xQueueSend(mp3_cmd_queue_handle, &cmd, (TickType_t) 0);

    //vPortFree(internal_handle->memory_pool);  // since now we are using static memory, so can't free, but even using dynamic, i think free function also act at app site
    internal_handle->memory_pool = NULL;

    vPortFree(mp3_decode_buffer);
    mp3_decode_buffer = NULL;

    vPortFree(internal_handle);
    internal_handle = NULL;

    mp3_codec_internal_handle = NULL;
    return MP3_CODEC_RETURN_OK;
}


mp3_codec_media_handle_t *mp3_codec_open(mp3_codec_callback_t mp3_codec_callback)
{
    printf("[MP3 Codec]Open codec\r\n");

    mp3_codec_media_handle_t *handle;
    mp3_codec_internal_handle_t *internal_handle; /*internal handler*/

    if (mp3_decode_buffer) {
        vPortFree(mp3_decode_buffer);
        mp3_decode_buffer = NULL;
        printf("free mp3_decode_buffer\r\n");
    }

    if (mp3_codec_internal_handle) {
        vPortFree(mp3_codec_internal_handle);
        mp3_codec_internal_handle = NULL;
        printf("free mp3_codec_internal_handle\r\n");
    }

    /* alloc internal handler space */
    internal_handle = (mp3_codec_internal_handle_t *)pvPortMalloc(sizeof(mp3_codec_internal_handle_t));
    if (NULL == internal_handle) {
        log_hal_error("[MP3 Codec] Cannot alloc internal_handle\r\n");

        return NULL;
    }

    memset(internal_handle, 0, sizeof(mp3_codec_internal_handle_t));

    /* assign internal handler to be global and static handler*/
    mp3_codec_internal_handle = internal_handle;

    /* initialize internal handle*/
    internal_handle->share_buff_size = 0;
    internal_handle->decode_pcm_buffer_size = 0;
    internal_handle->stream_out_pcm_buff_size = 0;
    internal_handle->working_buff1_size = 0;
    internal_handle->working_buff2_size = 0;

    mp3_decode_buffer = pvPortMalloc(sizeof(uint8_t) * MP3_DECODE_BUFFER_LENGTH);
    if (NULL == mp3_decode_buffer) {
        log_hal_error("[MP3 Codec] Cannot alloc mp3_decode_buffer\r\n");

        vPortFree(internal_handle);
        internal_handle = NULL;

        return NULL;
    }


    handle = &internal_handle->handle;
    handle->audio_id = 0;
    handle->handler  = mp3_codec_callback;
    handle->play     = mp3_codec_play;
    handle->pause    = mp3_codec_pause;
    handle->resume   = mp3_codec_resume;
    handle->stop     = mp3_codec_stop;
    handle->close_codec = mp3_codec_close;
    handle->skip_id3v2_and_reach_next_frame = mp3_codec_skip_id3v2_and_reach_next_frame;
    handle->state    = MP3_CODEC_STATE_READY;
    mp3_codec_buffer_function_init(handle);

    if (g_mp3_silence) {
        free(g_mp3_silence);
        g_mp3_silence = NULL;
        printf("free g_mp3_silence\r\n");
    }

    if (mp3_codec_queue_handle) {
        vQueueDelete(mp3_codec_queue_handle);
        mp3_codec_queue_handle = NULL;
    }
    if (mp3_cmd_queue_handle) {
        vQueueDelete(mp3_cmd_queue_handle);
        mp3_cmd_queue_handle = NULL;
    }
    if (xDecodeTaskHandle) {
        vTaskDelete(xDecodeTaskHandle);
        xDecodeTaskHandle = NULL;
    }
    if (xPlayTaskHandle) {
        vTaskDelete(xPlayTaskHandle);
        xPlayTaskHandle = NULL;
    }

    mp3_codec_queue_handle = xQueueCreate(MP3_CODEC_QUEUE_LENGTH, sizeof(mp3_codec_queue_event_t));
    {   /* Initialize queue registration */
        uint32_t id_idx;
        for (id_idx = 0; id_idx < MAX_MP3_CODEC_FUNCTIONS; id_idx++) {
            mp3_codec_queue_id_array[id_idx] = MP3_CODEC_QUEUE_EVENT_NONE;
        }
    }

    //create decode task
    mp3_codec_task_create();

    // create play task and command queue
    mp3_cmd_queue_handle = xQueueCreate(MP3_CODEC_QUEUE_LENGTH, sizeof(mp3_cmd_queue_event_id_t));
    mp3_play_task_create();

    return handle;
}
