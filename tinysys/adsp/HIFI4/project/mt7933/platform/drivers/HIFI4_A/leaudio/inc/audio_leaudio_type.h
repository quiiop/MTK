#ifndef __AUDIO_LEAUDIO_TYPE_H__
#define __AUDIO_LEAUDIO_TYPE_H__
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "semphr.h"
#include "audio_ringbuf.h"
#include "mtk_heap_type.h"

/*
typedef enum
{
    MTK_eMemDefault = 0,
    MTK_eMemDramNormal,
    MTK_eMemDramNormalNC,
    MTK_eMemInvalid
} MTK_eMemoryType;
*/

#define LE_MALLOC_TYPE MTK_eMemDramNormal

#define LEAUDIOBUF_MAX_HANDLE_CNT (2)

#define LEAUDIO_TIMER_PERIOD (20)

#define LEAUDIO_DUMP_DATA_SUPPORT

typedef enum {
    AUDIO_FORMAT_INVALID             = 0xFFFFFFFFu,
    AUDIO_FORMAT_DEFAULT             = 0,
    AUDIO_FORMAT_PCM                 = 0x00000000u,

    AUDIO_FORMAT_PCM_16_BIT            = 0x1u,        // (PCM | PCM_SUB_16_BIT)
    AUDIO_FORMAT_PCM_8_BIT             = 0x2u,        // (PCM | PCM_SUB_8_BIT)
    AUDIO_FORMAT_PCM_32_BIT            = 0x3u,        // (PCM | PCM_SUB_32_BIT)
    AUDIO_FORMAT_PCM_8_24_BIT          = 0x4u,        // (PCM | PCM_SUB_8_24_BIT)
    AUDIO_FORMAT_PCM_FLOAT             = 0x5u,        // (PCM | PCM_SUB_FLOAT)
    AUDIO_FORMAT_PCM_24_BIT_PACKED     = 0x6u,        // (PCM | PCM_SUB_24_BIT_PACKED)
    AUDIO_FORMAT_AAC_LC                = 0x4000002u,  // (AAC | AAC_SUB_LC)
    AUDIO_FORMAT_AAC_HE_V1             = 0x4000010u,
    AUDIO_FORMAT_AAC_HE_V2             = 0x4000100u,  // (AAC | AAC_SUB_HE_V2)
} audio_format_t;

struct LEAUDIO_FORMAT_INFO_T {
    uint32_t codec_type; // codec types ex: LC3
    uint32_t sample_rate; // sample rate, ex: 44.1/48.88.2/96 KHz
    uint32_t bitrate; // encoder audio bitrates
    uint8_t nChannels;
    uint32_t frame_dms;
    uint32_t nSamples;
    uint32_t nBytes;
    uint16_t acl_hdl; // connection handle
    uint16_t l2c_rcid; // l2cap channel id
    uint16_t mtu; // mtu size
    uint8_t bits_per_sample; // bits per sample, ex: 16/24/32
};

typedef enum {
    LEAUDIO_TASK_DEINIT,
    LEAUDIO_TASK_IDLE,
    LEAUDIO_TASK_OPEN,
    LEAUDIO_TASK_INIT,
    LEAUDIO_TASK_PREPARED,
    LEAUDIO_TASK_WORKING,
    LEAUDIO_TASK_STATE_MAX
} leaudio_task_state_t;

struct aud_fmt_cfg_t {
    uint32_t audio_format;      /* audio_format_t */
    uint32_t num_channels : 4;  /* 1, 2, 3, 4, ..., 15 */
    uint32_t sample_rate  : 19; /* 8000, 16000, 32000, ..., 352800, 384000 */
#if 0
    uint32_t b_interleave : 1;  /* 0: non-interleave, 1: interleave */
    uint32_t frame_size_ms: 8;  /* ex, 20ms; 0: not frame base. [0 ~ 255] */
#endif
};

enum {
    DSP_BT_BLE_START = 0,
    DSP_BT_BLE_STOP,
    DSP_BT_BLE_LC3_CODEC,
    DSP_BT_BLE_CH_UPDATE,
};

struct leaudio_aci_ctrl_base_info {
    bool isAciBaseInfoRead;
    uint32_t* aci_ctrl_data_ptr;
    char aci_ctrl_data_num;
    char aci_ctrl_data_size;
};


struct leaudio_task_node {
    TaskHandle_t mTask;
    char* pcTaskName;
    unsigned int task_stack_size;
    int task_priority;
    TaskFunction_t mTaskLoopFunction;
    char* mtask_decoder_input_buf;
    uint32_t mtask_decoder_input_buf_size;
    char* mtask_decoder_processing_buf;
    uint32_t mtask_decoder_processing_buf_size;
    char* mtask_decoder_output_buf;
    uint32_t mtask_decoder_output_buf_size;
};

struct LEAUDIO_DECODE_BUF_T {
    uint32_t connection_handle;
    uint8_t group_id;
    uint8_t packet_type;
    int read_length;
    uint32_t pkt_status;
    void* pDecodedRingbufAddr;
    struct RingBuf* decoded_data_ring_buf;
};

#ifdef LEAUDIO_DUMP_DATA_SUPPORT
typedef struct {
    unsigned long start_addr;           // start address of N10-DSP share buffer
    volatile uint16_t length;           // total buffer length
    volatile bool is_buf_full;          // indicate if buffer is full
    volatile uint16_t read_offset;      // read pointer of N10-DSP share buffer
    volatile uint16_t write_offset;     // write pointer of N10-DSP share buffer
} lea_dsp_share_buf_info_t;

typedef struct {
    bool is_inited;
    unsigned long dsp_buf_info_base;
}lea_host_dump_param_t;
#endif

struct audio_leaudio_struct {
    SemaphoreHandle_t xLeAudioTaskSemphr;
    bool first_frame_in;
    unsigned long long rx_data_time_stamp; //unit ns
    uint32_t presentation_delay; //unit us
    bool bLeaudioStreamStatus;
    bool bLeaudioIsPaused;
    leaudio_task_state_t state;
    //leaudio encode/decode task
    struct leaudio_task_node leaudioDecodeTask;
    //ACI control base info
    struct leaudio_aci_ctrl_base_info leaudioAciCtrlBaseInfo;
    //current codec type
    int leaudio_codec_type;
    // connection handle of the earpieces
    uint32_t conn_handle_l;
    uint32_t conn_handle_r;
    //temp pcm buf for decoder
    char* pLeftBuf;
    char* pRightBuf;
    //data buffer for lc3 data to be decoded
    struct LEAUDIO_DECODE_BUF_T* leaudioDecodeBuf[LEAUDIOBUF_MAX_HANDLE_CNT];
    //pointer of playback info
    uint32_t lea_playback_info_ptr;
#ifdef LEAUDIO_DUMP_DATA_SUPPORT
    //lc3 dump ring buffer info
    lea_dsp_share_buf_info_t *dump_share_buf;
    bool isDumpReady;
    bool isDumpStart;
#endif
    //IRQ interval in
    uint32_t bnNum;
    //Packet Sequence Counter
    uint32_t pktSeqNum;
    //Leaudio Scenario
    uint8_t leAudioScenario;
    struct aud_fmt_cfg_t fmt_attr;
    uint32_t* bt_sram_addr;
};

#define LEAUDIO_CODEC_TYPE_LC3 0x40

#endif
