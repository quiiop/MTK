/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include "encode.h"
#include "lc3_codec_exp.h"
#include "audio_leaudio_type.h"
#include "audio_leaudio_codec.h"
#include "leaudio_debug.h"
#include "mtk_heap.h"
#include "le_main.h"

#define MAX_DECODER_NUM 2

struct encoder_helper {
    Word8 *pEncHandle;
    int sample_rate;
    int frame_dms;
    int bitrate;
    short nChannels;
    int audio_format;
    int nSamples;
    int nBytes;
    short *pInBuf;      //block pcm data after deinterleaved
    int pcm_size_per_encode;
};

LE_SECTION_BSS static struct encoder_helper lc3_enc;

struct decoder_helper {
    Word8 *pDecHandle;
    int sample_rate;
    int frame_dms;
    int bitrate;
    short nChannels;
    int audio_format;
    int nSamples;
    int nBytes;
    int plcMeth;
    short *pInBuf;       //block pcm data after deinterleaved
    int pcm_size_per_decode;
};

LE_SECTION_BSS static struct decoder_helper lc3_dec[MAX_DECODER_NUM];

#define LC3_ENCODER_INTERVAL_MS LEAUDIO_TIMER_PERIOD
#define LC3_FRAME_SAMPLES (1024)

enum SampleRate {
    LC3_RATE_UNKNOWN = 0x00,
    LC3_RATE_44100 = 0x01,
    LC3_RATE_48000 = 0x02,
    LC3_RATE_88200 = 0x04,
    LC3_RATE_96000 = 0x08,
    LC3_RATE_176400 = 0x10,
    LC3_RATE_192000 = 0x20,
    LC3_RATE_16000 = 0x40,
    LC3_RATE_24000 = 0x80,
    LC3_RATE_8000 = 0x100,
    LC3_RATE_32000 = 0x200,
};

enum BitsPerSample {
    LC3_BITS_UNKNOWN = 0x00,
    LC3_BITS_16 = 0x01,
    LC3_BITS_24 = 0x02,
    LC3_BITS_32 = 0x04,
};

enum Lc3FrameDuration {
    DURATION_10000US = 0x00,
    DURATION_7500US = 0x01,
};

LE_SECTION_DATA static const char *ERROR_MESSAGE[] = {
    /* [LC3_OK]                               =*/ "No error ocurred",
    /* [LC3_ALIGN_ERROR]                      =*/ "Unaligned pointer",
    /* [LC3_BITRATE_ERROR]                    =*/ "Invalid bitrate",
    /* [LC3_BITRATE_SET_ERROR]                =*/ "Function called after bitrate has been set",
    /* [LC3_BITRATE_UNSET_ERROR]              =*/ "Function called before bitrate has been set",
    /* [LC3_CHANNELS_ERROR]                   =*/ "Invalid number of channels",
    /* [LC3_DECODE_ERROR]                     =*/ "Frame failed to decode and was concealed!",
    /* [LC3_EPMODE_ERROR]                     =*/ "Invalid EP mode",
    /* [LC3_EPMR_ERROR]                       =*/ "Invalid EPMR",
    /* [LC3_BITDEPTH_ERROR]                   =*/ "Function called with illegal bit depth",
    /* [LC3_FRAMEMS_ERROR]                    =*/ "Invalid frame_ms",
    /* [LC3_FRAMESIZE_ERROR]                  =*/ "Frame size below minimum or above maximum",
    /* [LC3_HRMODE_ERROR]                     =*/ "Invalid usage of hrmode, sampling rate and frame size",
    /* [LC3_NULL_ERROR]                       =*/ "Pointer argument is null",
    /* [LC3_NUMBYTES_ERROR]                   =*/ "Invalid number of bytes",
    /* [LC3_PADDING_ERROR]                    =*/ "Padding error",
    /* [LC3_PLCMODE_ERROR]                    =*/ "Invalid PLC method",
    /* [LC3_PLCMODE_CONF_ERROR]               =*/ "PLC method not supported due to hrmode or frame size",
    /* [LC3_RESTRICT_BT_BINARY_ERROR]         =*/ "Unsupported combination of frame length, sampling rate and bitrate",
    /* [LC3_SAMPLERATE_ERROR]                 =*/ "Invalid sampling rate",
    /* [LC3_SCRATCH_INVALID_ERROR]            =*/ "Scratch space not allocated or size invalidated",
    /* [LC3_SET_BANDWIDTH_NOT_SUPPORTED]      =*/ "Bandwidth controler not available",
    /* [LC3_LFE_MODE_NOT_SUPPORTED]           =*/ "LFE support not available",
    /* [LC3_ERROR_PROTECTION_NOT_SUPPORTED]   =*/ "Error protection not available",
    /* [LC3_WAV_FORMAT_NOT_SUPPORTED]         =*/ "Unsupported waveform format",
    /* [LC3_ALLOC_ERROR]                      =*/ "Table allocation failed!",
    /* [LC3_WARNING]                          =*/ "Generic Warning",
    /* [LC3_BW_WARNING]                       =*/ "Invalid bandwidth cutoff frequency",
    /* [LC3_Error_LAST]                       =*/ "invalid error code",
};
LE_SECTION_DATA unsigned int LC3_ERROR_NUM = sizeof(ERROR_MESSAGE) / sizeof(ERROR_MESSAGE[0]);

LE_SECTION_FUNC static void interleave(short **in, short *out, int n, int channels)
{
    int ch, i;
    for (ch = 0; ch < channels; ch++)
    {
        for (i = 0; i < n; i++)
        {
            out[i * channels + ch] = in[ch][i];
        }
    }
}

LE_SECTION_FUNC static void deinterleave(short *in, short **out, int n, int channels)
{
    int ch, i;
    for (ch = 0; ch < channels; ch++) {
        for (i = 0; i < n; i++) {
            out[ch][i] = in[i * channels + ch];
        }
    }
}

LE_SECTION_FUNC static unsigned int get_lc3_samplerate(unsigned int input) {
    unsigned int output;

    switch (input) {
    case LC3_RATE_8000:
        output = 8000;
        break;
    case LC3_RATE_16000:
        output = 16000;
        break;
    case LC3_RATE_24000:
        output = 24000;
        break;
    case LC3_RATE_32000:
        output = 32000;
        break;
    case LC3_RATE_44100:
        output = 44100;
        break;
    case LC3_RATE_48000:
        output = 48000;
        break;
    case LC3_RATE_88200:
        output = 88200;
        break;
    case LC3_RATE_96000:
        output = 96000;
        break;
    case LC3_RATE_176400:
        output = 176400;
        break;
    case LC3_RATE_192000:
        output = 192000;
        break;
    default:
        output = 0;
        LE_LOG_E("unspupported sampling rate(%d)", input);
        break;
    }
    return output;
}

LE_SECTION_FUNC static unsigned int get_lc3_audioFormat(unsigned int input) {
    unsigned int output;

    switch (input) {
    case LC3_BITS_16:
        output = 16;
        break;
    case LC3_BITS_24:
        output = 24;
        break;
    case LC3_BITS_32:
        output = 32;
        break;
    default:
        output = 16;
        LE_LOG_E("unspupported format(%d)", input);
        break;
    }
    return output;
}

LE_SECTION_FUNC static unsigned int get_lc3_bitsPerSample(unsigned int input) {
    unsigned int output;

    switch (input) {
    case LC3_BITS_16:
        output = AUDIO_FORMAT_PCM_16_BIT;
        break;
    case LC3_BITS_24:
        output = AUDIO_FORMAT_PCM_8_24_BIT;
        break;
    case LC3_BITS_32:
        output = AUDIO_FORMAT_PCM_32_BIT;
        break;
    default:
        output = AUDIO_FORMAT_PCM_16_BIT;
        LE_LOG_E("unspupported bit type(%d)", input);
        break;
    }
    return output;
}

LE_SECTION_FUNC static unsigned int get_lc3_frameDms(unsigned int input) {
    unsigned int output;

    switch (input) {
    case DURATION_10000US:
        output = 100;
        break;
    case DURATION_7500US:
        output = 75;
        break;
    default:
        output = 100;
        LE_LOG_E("unspupported frameDms(%d)", input);
        break;
    }
    return output;
}

LE_SECTION_FUNC static unsigned int mtk_lc3_get_num_channels()
{
    struct LEAUDIO_FORMAT_INFO_T* format_info = leaudio_get_format_info();
    if (format_info == NULL) {
        LE_LOG_E("get format info failed!!!");
        return 0;
    }
    return format_info->nChannels;
}

LE_SECTION_FUNC static unsigned int mtk_lc3_get_sample_rate()
{
    struct LEAUDIO_FORMAT_INFO_T* format_info = leaudio_get_format_info();
    if (format_info == NULL) {
        LE_LOG_E("get format info failed!!!");
        return 0;
    }
    return get_lc3_samplerate(format_info->sample_rate);
}

LE_SECTION_FUNC static unsigned int mtk_lc3_get_audio_format()
{
    struct LEAUDIO_FORMAT_INFO_T* format_info = leaudio_get_format_info();
    if (format_info == NULL) {
        LE_LOG_E("get format info failed!!!");
        return 0;
    }
    return get_lc3_bitsPerSample(format_info->bits_per_sample);
}

LE_SECTION_FUNC static int mtk_lc3_calc_input_length(int scenario)
{
    int ret = 0;
    if (scenario == ENCODE_SCENARIO) {
        ret = lc3_enc.pcm_size_per_encode;
    } else if (scenario == DECODE_SCENARIO) {
        ret = lc3_dec[0].nBytes;
    }
    return ret;
}

LE_SECTION_FUNC static void mtk_lc3_encoder_init()
{
    int encHdlSize = LC3_ENC_GetBufferSize();
    struct LEAUDIO_FORMAT_INFO_T* format_info = leaudio_get_format_info();

    lc3_enc.sample_rate = get_lc3_samplerate(format_info->sample_rate);
    lc3_enc.pEncHandle = (Word8 *)MTK_pvPortMalloc(encHdlSize, LE_MALLOC_TYPE);
    lc3_enc.frame_dms = get_lc3_frameDms(format_info->frame_dms);
    lc3_enc.bitrate = format_info->bitrate;
    lc3_enc.nChannels = format_info->nChannels;
    lc3_enc.audio_format = get_lc3_audioFormat(format_info->bits_per_sample);

    LC3_ENC_SetParam(lc3_enc.pEncHandle,
                     lc3_enc.sample_rate,
                     lc3_enc.frame_dms,
                     lc3_enc.bitrate,
                     lc3_enc.nChannels,
                     lc3_enc.audio_format);

    LC3_ENC_Init(lc3_enc.pEncHandle);

    lc3_enc.nSamples = ((lc3_enc_t *)lc3_enc.pEncHandle)->nSamples;
    lc3_enc.nBytes = ((lc3_enc_t *)lc3_enc.pEncHandle)->nBytes;
    lc3_enc.pcm_size_per_encode = lc3_enc.nSamples * sizeof(short) * lc3_enc.nChannels;
    lc3_enc.pInBuf = (short *)MTK_pvPortMalloc(lc3_enc.pcm_size_per_encode, LE_MALLOC_TYPE);

    format_info->nSamples = ((lc3_enc_t *)lc3_enc.pEncHandle)->nSamples;
    format_info->nBytes   = ((lc3_enc_t *)lc3_enc.pEncHandle)->nBytes;

    LE_LOG_D("encHdlSize=%d, sample_rate=%d, nSamples = %d, nBytes=%d",
             encHdlSize, lc3_enc.sample_rate,
             lc3_enc.nSamples, lc3_enc.nBytes);
}

LE_SECTION_FUNC static void mtk_lc3_encoder_deinit()
{
    if (lc3_enc.pEncHandle) {
        LC3_ENC_UnInit(lc3_enc.pEncHandle);
        MTK_vPortFree(lc3_enc.pEncHandle);
        lc3_enc.pEncHandle = NULL;
    }
    MTK_vPortFree(lc3_enc.pInBuf);
    lc3_enc.pInBuf = NULL;
}

LE_SECTION_FUNC static uint32_t mtk_lc3_encode(char* in_buff,
        uint32_t in_buffer_size,
        char* out_buff,
        uint32_t out_buffer_size)
{
    uint32_t output_size = 0;
    short* input16[] = {lc3_enc.pInBuf, lc3_enc.pInBuf + lc3_enc.nSamples};
    unsigned int err = 0;

    if (in_buff == NULL) {
        LE_LOG_E("in_buff is NULL!\n");
        configASSERT(false);
        return -1;
    }
    if (out_buff == NULL) {
        LE_LOG_E("out_buff is NULL!\n");
        configASSERT(false);
        return -1;
    }

    /* deinterleave channels */
    deinterleave((short *)in_buff, input16, lc3_enc.nSamples, lc3_enc.nChannels);

    /* Run encoder */
    err = (int)LC3_ENC_Process(lc3_enc.pEncHandle, (void **)input16, out_buff, &lc3_enc.nBytes);
    if (err != LC3_OK) {
        if (err >= LC3_ERROR_NUM) {
            LE_LOG_D("ERROR_MESSAGE array out of bounds.\n");
        } else {
            LE_LOG_D("%s", ERROR_MESSAGE[err]);
        }
        return -1;
    }

    LE_LOG_D("lc3_encode: input_size = %u, output_size = %u", in_buffer_size, lc3_enc.nBytes);

    output_size = lc3_enc.nBytes;

    return output_size;
}

LE_SECTION_FUNC int mtk_lc3_encodec_pcm_buffer_len()
{
    return lc3_enc.nSamples * sizeof(short) * lc3_enc.nChannels;
}

LE_SECTION_FUNC static void mtk_lc3_decoder_init()
{
    int i = 0;
    int decHdlSize = LC3_DEC_GetBufferSize();
    struct LEAUDIO_FORMAT_INFO_T* format_info = leaudio_get_format_info();

    for (i = 0; i < format_info->nChannels && i < MAX_DECODER_NUM; i++) {
        lc3_dec[i].sample_rate = get_lc3_samplerate(format_info->sample_rate);
        lc3_dec[i].pDecHandle = (Word8 *)MTK_pvPortMalloc(decHdlSize, LE_MALLOC_TYPE);
        lc3_dec[i].frame_dms = get_lc3_frameDms(format_info->frame_dms);
        lc3_dec[i].bitrate = format_info->bitrate / format_info->nChannels;
        lc3_dec[i].nChannels = 1;
        lc3_dec[i].audio_format = get_lc3_audioFormat(format_info->bits_per_sample);
        lc3_dec[i].plcMeth = 0;

        LC3_DEC_SetParam(lc3_dec[i].pDecHandle,
                         lc3_dec[i].sample_rate,
                         lc3_dec[i].frame_dms,
                         lc3_dec[i].bitrate,
                         lc3_dec[i].nChannels,
                         lc3_dec[i].audio_format,
                         lc3_dec[i].plcMeth);

        LC3_DEC_Init(lc3_dec[i].pDecHandle);

        lc3_dec[i].nSamples = ((lc3_dec_t *)lc3_dec[i].pDecHandle)->nSamples;
        lc3_dec[i].nBytes = ((lc3_dec_t *)lc3_dec[i].pDecHandle)->nBytes;
        lc3_dec[i].pcm_size_per_decode = lc3_dec[i].nSamples * sizeof(short);
        lc3_dec[i].pInBuf = (short *)MTK_pvPortMalloc(lc3_dec[i].pcm_size_per_decode, LE_MALLOC_TYPE);

        format_info->nSamples = ((lc3_dec_t *)lc3_dec[i].pDecHandle)->nSamples;
        format_info->nBytes   = ((lc3_dec_t *)lc3_dec[i].pDecHandle)->nBytes;

        LE_LOG_I("for decoder %d: decHdlSize=%d, channels = %d, sample_rate=%d, format=%d, nSamples = %d, nBytes=%d, plcMeth=%d",
                 i, decHdlSize, lc3_dec[i].nChannels, lc3_dec[i].sample_rate, lc3_dec[i].audio_format,
                 lc3_dec[i].nSamples, lc3_dec[i].nBytes, lc3_dec[i].plcMeth);
    }
}

LE_SECTION_FUNC static void mtk_lc3_decoder_deinit()
{
    int i = 0;
    struct LEAUDIO_FORMAT_INFO_T* format_info = leaudio_get_format_info();

    for (i = 0; i < format_info->nChannels && i < MAX_DECODER_NUM; i++) {
        if (lc3_dec[i].pDecHandle) {
            LC3_DEC_UnInit(lc3_dec[i].pDecHandle);
            MTK_vPortFree(lc3_dec[i].pDecHandle);
            lc3_dec[i].pDecHandle = NULL;
        }
        MTK_vPortFree(lc3_dec[i].pInBuf);
        lc3_dec[i].pInBuf = NULL;
    }
}

LE_SECTION_FUNC static uint32_t mtk_lc3_decode(char* in_buff,
        uint32_t in_buffer_size,
        char* out_buff,
        uint32_t out_buffer_size,
        int bfi_ext,
        bool is_left)
{
    int index = is_left == true ? 0 : 1;
    char* p_input = in_buff;
    short* p_out = (short *)out_buff;
    short* output16[] = {lc3_dec[index].pInBuf, lc3_dec[index].pInBuf + lc3_dec[index].nSamples};
    uint32_t output_size = 0;
    unsigned int err = 0;

    configASSERT(in_buff != NULL);
    configASSERT(out_buff != NULL);

//    LE_LOG_D("lc3_decode: input_size = %u, lc3 size per frame = %u", in_buffer_size, lc3_dec[index].nBytes);

    /* Run decoder */
    err = (int)LC3_DEC_Process(lc3_dec[index].pDecHandle, p_input, (void **)output16, bfi_ext);
    if (err != LC3_OK && err != LC3_DECODE_ERROR) {
        if (err >= LC3_ERROR_NUM) {
            LE_LOG_D("ERROR_MESSAGE array out of bounds.\n");
        } else {
            LE_LOG_D("%s", ERROR_MESSAGE[err]);
        }
        return -1;
    }

    /* interleave channels */
    interleave(output16, p_out, lc3_dec[index].nSamples, lc3_dec[index].nChannels);

    lc3_dec[index].pcm_size_per_decode = lc3_dec[index].nSamples * sizeof(short);
    output_size = lc3_dec[index].pcm_size_per_decode;

    return output_size;
}

LE_SECTION_FUNC static int mtk_lc3_calc_output_length(int scenario)
{
    int ret = 0;
    if (scenario == ENCODE_SCENARIO) {
        ret = lc3_enc.nBytes;
    } else if (scenario == DECODE_SCENARIO) {
        ret = lc3_dec[0].pcm_size_per_decode;
    }
    return ret;
}

LE_SECTION_RODATA const codec_st lc3_encode_op = {
    "MTK_LC3",
    &mtk_lc3_encoder_init,
    &mtk_lc3_encode,
    &mtk_lc3_encoder_deinit,
    &mtk_lc3_calc_input_length,
    &mtk_lc3_get_sample_rate,
    &mtk_lc3_get_audio_format,
    &mtk_lc3_get_num_channels,
    &mtk_lc3_encodec_pcm_buffer_len,
    &mtk_lc3_decoder_init,
    &mtk_lc3_decode,
    &mtk_lc3_decoder_deinit,
    &mtk_lc3_calc_output_length,
};
