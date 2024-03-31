#ifndef __AUDIO_LEAUDIO_CODEC_H__
#define __AUDIO_LEAUDIO_CODEC_H__
//#include "stdint.h"
#include "audio_leaudio_type.h"
#include "audio_bttype.h"

void task_leaudio_set_codecinfo(uint32_t codec_type, void* config);

unsigned int get_leaudio_codec_channel(int codec_type);
unsigned int get_leaudio_codec_pcmformat(int codec_type);
unsigned int get_leaudio_codec_samplerate(int codec_type);
int set_leaudio_codec_type(uint32_t format_type);
struct LEAUDIO_FORMAT_INFO_T* leaudio_get_format_info();

/* dump lc3 codec info*/
void dump_lc3_codec(struct bt_LeAudioCodecConfiguration codec_config);
void dump_lc3_param(struct bt_Lc3Parameters param);
void dump_bt_connparam(struct bt_ConnParam param);


#endif
