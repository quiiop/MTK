#include "le_main.h"
#include "audio_leaudio_codec.h"
#include "encode.h"
#include "leaudio_debug.h"

LE_SECTION_DATA struct LEAUDIO_FORMAT_INFO_T leaudio_format_info = {LEAUDIO_CODEC_TYPE_LC3, 0x200, 128000, 2, 0, 0, 0, 0, 0, 0, 1};

LE_SECTION_FUNC struct LEAUDIO_FORMAT_INFO_T* leaudio_get_format_info()
{
    return &leaudio_format_info;
}

LE_SECTION_FUNC void task_leaudio_set_codecinfo(uint32_t codec_type, void* config)
{
    configASSERT(config);
    if (config == NULL)
        return;

    switch(codec_type) {
    case LEAUDIO_CODEC_TYPE_LC3: {
        struct bt_LeAudioCodecConfiguration* codec_config = (struct bt_LeAudioCodecConfiguration*) config;
        struct bt_Lc3Parameters lc3_config = codec_config->lc3Config;

        leaudio_format_info.codec_type = codec_config->codecType;
        leaudio_format_info.bitrate = codec_config->encodedAudioBitrate * codec_config->audioChannelAllocation;
        leaudio_format_info.nChannels = codec_config->audioChannelAllocation;
        leaudio_format_info.sample_rate = lc3_config.samplingFrequency;
        leaudio_format_info.frame_dms = lc3_config.frameDuration;
        leaudio_format_info.bits_per_sample = lc3_config.pcmBitDepth;
        break;
    }
    default:
        LE_LOG_E("codec_type = %u is not support.", codec_type);
        break;
    }

    LE_LOG_I("codec_type = %u, sample_rate = %u, bit_per_sample=%d, channel_mode=%d, frame_dms=%u, bitrate=%u\n",
             leaudio_format_info.codec_type,
             leaudio_format_info.sample_rate,
             leaudio_format_info.bits_per_sample,
             leaudio_format_info.nChannels,
             leaudio_format_info.frame_dms,
             leaudio_format_info.bitrate);
}

/*
 * query with target samplerate channle and format
 * task leaudio will do src and format convert base on quecy format.
 */

LE_SECTION_FUNC unsigned int get_leaudio_codec_channel(int codec_type)
{
    return leaudio_get_num_channels(codec_type);
}

/*
AUDIO_FORMAT_PCM_16_BIT            = 0x1u,        // (PCM | PCM_SUB_16_BIT)
AUDIO_FORMAT_PCM_8_BIT             = 0x2u,        // (PCM | PCM_SUB_8_BIT)
AUDIO_FORMAT_PCM_32_BIT            = 0x3u,        // (PCM | PCM_SUB_32_BIT)
AUDIO_FORMAT_PCM_8_24_BIT          = 0x4u,        // (PCM | PCM_SUB_8_24_BIT)
AUDIO_FORMAT_PCM_FLOAT             = 0x5u,        // (PCM | PCM_SUB_FLOAT)
AUDIO_FORMAT_PCM_24_BIT_PACKED     = 0x6u,        // (PCM | PCM_SUB_24_BIT_PACKED)
*/

LE_SECTION_FUNC unsigned int get_leaudio_codec_pcmformat(int codec_type)
{
    return leaudio_get_audio_format(codec_type);
}
LE_SECTION_FUNC unsigned int get_leaudio_codec_samplerate(int codec_type)
{
    return leaudio_get_sample_rate(codec_type);
}

/*
*  codec to dsp used type
*/
LE_SECTION_FUNC int set_leaudio_codec_type(uint32_t format_type)
{
    //1:LC3
    const char* str_codec = "NOT SUPPORT";
    int codec_type = MAX_CODEC_SUPPORT;
    switch(format_type) {
    case LEAUDIO_CODEC_TYPE_LC3:
        codec_type = LC3_CODEC;
        str_codec = "LC3";
        break;
    default:
        LE_LOG_E(" leaudio_format_info.codec_type = %u is not support.", format_type);
        codec_type = MAX_CODEC_SUPPORT;
        break;
    }
    LE_LOG_D(" set codec to %s", str_codec);
    return codec_type;
}

LE_SECTION_FUNC void dump_bt_connparam(struct bt_ConnParam param)
{
    LE_LOG_D(" conn_handle_R %u conn_handle_L %u bn %u\n",
             param.conn_handle_R,
             param.conn_handle_L,
             param.bn);
}

LE_SECTION_FUNC void dump_lc3_param(struct bt_Lc3Parameters param)
{
    LE_LOG_I("pcmBitDepth %u samplingFrequency %u frameDuration %u octetsPerFrame %u blocksPerSdu %d\n",
             param.pcmBitDepth,
             param.samplingFrequency,
             param.frameDuration,
             param.octetsPerFrame,
             param.blocksPerSdu);
}

LE_SECTION_FUNC void dump_lc3_codec(struct bt_LeAudioCodecConfiguration codec_config)
{
    LE_LOG_I("codecType %u audioChannelAllocation %u encodedAudioBitrate %u bfi_ext %u plc_method %u le_audio_type %u pts:%u\n",
             codec_config.codecType,
             codec_config.audioChannelAllocation,
             codec_config.encodedAudioBitrate,
             codec_config.bfi_ext,
             codec_config.plc_method,
             codec_config.le_audio_type,
             codec_config.presentation_delay);
    dump_lc3_param(codec_config.lc3Config);
}

