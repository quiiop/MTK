#include "mtk_heap.h"
#include "interrupt.h"
#include "leaudio_debug.h"
#include "audio_leaudio_type.h"
#include "audio_task_leaudio_decode.h"
#include "audio_leaudio_hw_reg.h"
#include "audio_leaudio_codec.h"
#include "encode.h"
#include "le_main.h"
#include "audio_common.h"
#include "dsp_scenario.h"
#include "systimer.h"

#define MAX_MSG_QUEUE_SIZE 16
#define LOCAL_TASK_STACK_SIZE (10 * 1024)

#define DECODED_RINGBUF_SIZE (1*1024) //LC3 packets received in IRQ: 100(packet size) * 3(packet num) = 300byte

#define INVALID_CONNECTION_HANDLE (0x0)
#define LOCAL_TASK_PRIORITY (2)
#define local_task_input_buffer_size (1*1024) // decoder inputs of LC3 packets each time(same as packets got in IRQ).
#define TASK_LEAUDIO_PROCESSING_BUFFER_SIZE (3*1024) // decoder ouputs of PCM frames each time(300byte lc3 -> 2880byte pcm).
#define local_task_output_buffer_size (6*1024) // output buffer for 2 channel PCM data(2880bytes * 2)
#define LEA_DUMP_SHARE_BUFFER_SIZE (4*1024) //size of buffer to dump lc3 data

/* leaudio format , should the same as fast for latency (?,confused)*/
#define LEAUDIO_DEFAULT_SIZE (3072)
#define LEAUDIO_DEFAULT_BNNUMBER (3) //burst number

#define LEAUDIO_DECODE_BUF_PTR (pAudio_leaudio_struct->leaudioDecodeBuf)
#define LEAUDIO_SRAM_PTR (pAudio_leaudio_struct->bt_sram_addr)

#define LEAUDIO_DECODER_INPUT_BUF (pAudio_leaudio_struct->leaudioDecodeTask.mtask_decoder_input_buf)
#define LEAUDIO_DECODER_INPUT_BUF_SIZE (pAudio_leaudio_struct->leaudioDecodeTask.mtask_decoder_input_buf_size)
#define LEAUDIO_DECODER_PROC_BUF (pAudio_leaudio_struct->leaudioDecodeTask.mtask_decoder_processing_buf)
#define LEAUDIO_DECODER_PROC_BUF_SIZE (pAudio_leaudio_struct->leaudioDecodeTask.mtask_decoder_processing_buf_size)
#define LEAUDIO_DECODER_OUTPUT_BUF (pAudio_leaudio_struct->leaudioDecodeTask.mtask_decoder_output_buf)
#define LEAUDIO_DECODER_OUTPUT_BUF_SIZE (pAudio_leaudio_struct->leaudioDecodeTask.mtask_decoder_output_buf_size)


#define memw() __asm("memw;\n")

LE_SECTION_DATA static struct audio_leaudio_struct *pAudio_leaudio_struct = NULL;

//time consuming issue debug.
#define LEAUDIO_PERFORMANCE_DEBUG
#ifdef LEAUDIO_PERFORMANCE_DEBUG
LE_SECTION_DATA unsigned long long isr_go_time_ns = 0;
LE_SECTION_DATA unsigned long long isr_exit_time_ns = 0;
LE_SECTION_DATA unsigned long long isr_dur_time_ns = 0;
LE_SECTION_DATA unsigned long long isr_previous_go_time_ns = 0;
LE_SECTION_DATA unsigned long long dec_loop_go_time_ns = 0;
LE_SECTION_DATA unsigned long long playback_time_ns = 0;
#endif

#ifdef LEAUDIO_DUMP_DATA_SUPPORT
int lc3_packet_size = 0;
int lc3_block_size = 0;
#endif

const static int pktTypeLookupTable[] =
{
    20,     //ISO_20_PKT_LEN
    40,     //ISO_40_PKT_LEN
    60,     //ISO_60_PKT_LEN
    80,     //ISO_80_PKT_LEN
    100,    //ISO_100_PKT_LEN
    120,    //ISO_120_PKT_LEN
    130,    //ISO_130_PKT_LEN
    155,    //ISO_155_PKT_LEN
    15,     //ISO_15_PKT_LEN
    30,     //ISO_30_PKT_LEN
    45,     //ISO_45_PKT_LEN
    75,     //ISO_75_PKT_LEN
    90,     //ISO_90_PKT_LEN
    117,     //ISO_117_PKT_LEN
};

LE_SECTION_FUNC void set_leaudio_state(uint32_t state)
{
    configASSERT(state < LEAUDIO_TASK_STATE_MAX);
    pAudio_leaudio_struct->state = state;
}

LE_SECTION_FUNC uint32_t get_leaudio_state(void)
{
    return pAudio_leaudio_struct->state;
}

LE_SECTION_FUNC int get_leaudio_codec_type()
{
    return pAudio_leaudio_struct->leaudio_codec_type;
}

LE_SECTION_FUNC unsigned long long calculate_presentation_time_info()
{
    return pAudio_leaudio_struct->rx_data_time_stamp + pAudio_leaudio_struct->presentation_delay * 1000;
}

LE_SECTION_FUNC static void task_leaudio_task_start(struct leaudio_task_node* pTaskNode)
{
    LE_LOG_D("start leaudio task, task name: %s", pTaskNode->pcTaskName);
    BaseType_t xReturn = pdFAIL;

    if (pTaskNode->mTask == 0) {
        xReturn = kal_xTaskCreate(
                      pTaskNode->mTaskLoopFunction,
                      pTaskNode->pcTaskName,
                      pTaskNode->task_stack_size,
                      NULL,
                      pTaskNode->task_priority,
                      &pTaskNode->mTask);

        configASSERT(xReturn == pdPASS);
    } else {
        LE_LOG_E("start leaudio task already created!!");
    }
}


LE_SECTION_FUNC static void task_leaudio_task_stop(struct leaudio_task_node* pTaskNode)
{
    LE_LOG_D(" stop leaudio task, task name: %s", pTaskNode->pcTaskName);

    if (pTaskNode->mTask == 0) {
        LE_LOG_E("task is NULL!!");
        return;
    }
    vTaskDelete(pTaskNode->mTask);
    pTaskNode->mTask = 0;
}


LE_SECTION_FUNC static void leaudio_read_aci_base_info()
{
    struct leaudio_aci_ctrl_base_info* baseInfo = &pAudio_leaudio_struct->leaudioAciCtrlBaseInfo;

    if (!baseInfo->isAciBaseInfoRead) {
        baseInfo->aci_ctrl_data_ptr = (uint32_t *)(*(LEAUDIO_SRAM_PTR + 1) - BTSRAM_REGION_1 +
                                      BTSRAM_INFRA_MAPPING);
        baseInfo->aci_ctrl_data_size = (*(LEAUDIO_SRAM_PTR) & 0xff00) >> 8;
        baseInfo->aci_ctrl_data_num = *(LEAUDIO_SRAM_PTR) & 0xff;
        baseInfo->isAciBaseInfoRead = true;

//        LE_LOG_D("aci_ctrl_data_size = 0x%x, aci_ctrl_data_num = 0x%x, aci_ctrl_data_ptr = %p",
//                 baseInfo->aci_ctrl_data_size, baseInfo->aci_ctrl_data_num, baseInfo->aci_ctrl_data_ptr);
    }
}

LE_SECTION_FUNC static int wirte_data_to_playback(SHARE_BUFFER_INFO_PTR playback_buf_info,
        int data_length,
        char* buf)
{
    configASSERT(playback_buf_info != NULL && buf != NULL);

    uint32_t spaces = 0;
    int total_write = 0;
    uint16_t unwrap_size = 0;
    void* buf_base_addr = (void*)playback_buf_info->startaddr;
    uint32_t buf_read_offset = playback_buf_info->ReadOffset;
    uint32_t buf_write_offset = playback_buf_info->WriteOffset;
    uint32_t buf_length = playback_buf_info->length;
    uint8_t buf_is_full = playback_buf_info->bBufferIsFull;


    if (pAudio_leaudio_struct->bLeaudioIsPaused == false) {
        spaces = (buf_length - buf_write_offset + buf_read_offset) % buf_length;

//        LE_LOG_D("buf_base_addr:%p, r_offset:%d, w_offset:%d, buf_isf:%d, buf_len:%d, data_len:%d, spaces:%d",
//                 buf_base_addr, buf_read_offset, buf_write_offset, buf_is_full, buf_length, data_length, spaces);
        if (spaces == 0 && buf_is_full == 0)
            spaces = buf_length;

        if (buf_is_full == 1 && buf_write_offset != buf_read_offset) {
            buf_is_full = 0;
        }
        if (!spaces || buf_is_full || spaces < data_length) {
            LE_LOG_W("PB buf not ready! r_offset:%d, w_offset:%d, buf_isf:%d, buf_len:%d, data_len:%d, spaces:%d",
                     buf_read_offset, buf_write_offset, buf_is_full, buf_length, data_length, spaces);
            return 0;
        }

        if (buf_read_offset > buf_write_offset) {
            memcpy(buf_base_addr + buf_write_offset, buf, data_length);
            buf_write_offset += data_length;
        } else {
            unwrap_size = buf_length - buf_write_offset;
            if (unwrap_size >= data_length) {
                memcpy(buf_base_addr + buf_write_offset, buf, data_length);
                buf_write_offset += data_length;
            } else {
                memcpy(buf_base_addr + buf_write_offset, buf, unwrap_size);
                memcpy(buf_base_addr, buf + unwrap_size, data_length - unwrap_size);
                buf_write_offset = data_length - unwrap_size;
            }
        }

        total_write += data_length;

        if (buf_write_offset == buf_length) {
            buf_write_offset = 0;
        }

        if (buf_write_offset == buf_read_offset) {
            buf_is_full = 1;
        }

        playback_buf_info->WriteOffset = buf_write_offset;
        playback_buf_info->bBufferIsFull = buf_is_full;
        playback_buf_info->total_write = total_write;
    }

    playback_buf_info->presentation_time_stamp = calculate_presentation_time_info();
    playback_buf_info->presentation_delay = pAudio_leaudio_struct->presentation_delay / 1000;

    return total_write;
}

LE_SECTION_FUNC static int packetType_to_packetSize(uint8_t packet_type)
{
    int ret = 80;
    int table_len = sizeof(pktTypeLookupTable)/sizeof(pktTypeLookupTable[0]);

    if (packet_type < table_len) {
        ret = pktTypeLookupTable[packet_type];
    }
    else {
        LE_LOG_W("unsupported packet type: %x, set as default value", packet_type);
    }

    return ret;
}

LE_SECTION_FUNC static int read_data_from_buf(struct RingBuf* ringBuf, int length, char* buf)
{
    unsigned int spaces;
    int total_read = 0;
    int read_len = 0;
    int retry_cnt = 5;
    while(total_read < length && (pAudio_leaudio_struct->bLeaudioIsPaused == false) && retry_cnt > 0) {
        spaces = RingBuf_getFreeSpace(ringBuf);
        if (spaces >= length) {
            read_len = length - total_read;
            if (spaces < read_len) {
                read_len = spaces;
            }
            RingBuf_copyFromLinear(ringBuf, buf+total_read, read_len);

            total_read += read_len;
        } else {
            LE_LOG_E("not enought space left, need = %u length = %d, reset RingBuf\n",
                     length - total_read, length);
            RingBuf_Reset(ringBuf);
            retry_cnt--;
            continue;
        }
    }

//    LE_LOG_D("read from bt read=%d, length=%d, data=%u \n", read_len, length, spaces);
    return total_read;
}

LE_SECTION_FUNC static int write_data_to_buf(struct RingBuf* ringBuf, uint32_t length, char* buf)
{
    unsigned int spaces;
    unsigned int total_write = 0;
    unsigned int write_len = 0;

    while (total_write < length && (pAudio_leaudio_struct->bLeaudioIsPaused == false)) {
        spaces = RingBuf_getDataCount(ringBuf);
        if (spaces > 0) {
            write_len = length - total_write;
            if (spaces < write_len) {
                write_len = spaces;
            }
            RingBuf_copyToLinear(buf+total_write, ringBuf, write_len);
            total_write += write_len;
        } else {
//            LE_LOG_W("can not write data room = %u, need write=%d \n", spaces, length-total_write);
            break;
        }
    }
//    LE_LOG_D("copy to bt writed=%u, length=%u, spaces=%u \n",write_len, length, spaces);
    return total_write;
}

LE_SECTION_FUNC static void get_data_from_bt()
{
    int i, total_read = 0;
    uint32_t* pAciData;
    struct leaudio_aci_ctrl_base_info* baseInfo = &pAudio_leaudio_struct->leaudioAciCtrlBaseInfo;

    configASSERT(baseInfo->isAciBaseInfoRead == true);

    pAciData = baseInfo->aci_ctrl_data_ptr;
    //Handle data from bt fw for each ACI block
    for (i = 0; i < baseInfo->aci_ctrl_data_num; i++) {
        //Only deal with data block whose rx ready bit is set
        bool rx_ready = (*(pAciData + 3) & 0x2) >> 1;
        if (rx_ready) {
            int j;
            uint32_t connection_handle = (*pAciData & 0xffff0000) >> 16;
            uint8_t group_id = *pAciData & 0xff;
            int rx_pkt_num = (*(pAciData + 1) & 0xf0) >> 4;
            uint8_t packet_type = (*(pAciData + 1) & 0xff00) >> 8;
            uint32_t pkt_status = *(pAciData + 4);
            char* pReadAddr = (char*)((*(pAciData + 6)) - BTSRAM_REGION_1 + BTSRAM_INFRA_MAPPING);
            int read_length = rx_pkt_num * packetType_to_packetSize(packet_type);
#if 0
        LE_LOG_D("con_handle:%04x, group_id:%x, rx_ready:%d, rx_pkt_num:%04d, packet_type:%x, pkt_status:%04x, read_length:%04d read_addr:%p",
                 connection_handle,
                 group_id,
                 rx_ready,
                 rx_pkt_num,
                 packet_type,
                 pkt_status,
                 read_length,
                 pReadAddr);
#endif
            for (j = 0; j < LEAUDIOBUF_MAX_HANDLE_CNT; j++) {
                if (LEAUDIO_DECODE_BUF_PTR[j] != NULL &&
                        connection_handle != INVALID_CONNECTION_HANDLE &&
                        connection_handle == LEAUDIO_DECODE_BUF_PTR[j]->connection_handle) {
                    //update info and copy data
                    LEAUDIO_DECODE_BUF_PTR[j]->connection_handle = connection_handle;
                    LEAUDIO_DECODE_BUF_PTR[j]->group_id = group_id;
                    LEAUDIO_DECODE_BUF_PTR[j]->packet_type = packet_type;
                    LEAUDIO_DECODE_BUF_PTR[j]->read_length = read_length;
                    LEAUDIO_DECODE_BUF_PTR[j]->pkt_status = pkt_status;
                    //clear old data before getting new data
                    RingBuf_Reset(LEAUDIO_DECODE_BUF_PTR[j]->decoded_data_ring_buf);
                    if (pkt_status == 0) {
                        total_read += read_data_from_buf(LEAUDIO_DECODE_BUF_PTR[j]->decoded_data_ring_buf,
                                                         read_length, pReadAddr);
                    }
                    break;
                }
            }

            //set rx_ready to 0
            *(pAciData + 3) &= 0xfffffffd;

            memw();
        }
//In mono case, only one connection is established and the other is invalid. So we need to mark below snippets to reduce log printings.
#if 0
         else {
            LE_LOG_W("i = %d, rx_ready not set, continue...", i);
        }
#endif

        //set status bit to 0
        *(pAciData + 3) &= 0xfffffff7;
        //check if tx_ready has been clear
        if ((*(pAciData + 3) & 0x1) == 0) {
            //set sleep_resource bit to 1
            *(pAciData + 3) |= 0x4;
            memw();
        }

        pAciData = (uint32_t*) ((char *)pAciData + baseInfo->aci_ctrl_data_size);
    }
}

/*
*get decode input length
*/
LE_SECTION_FUNC int task_leaudio_get_decode_input_length()
{
    return leaudio_calc_input_length(pAudio_leaudio_struct->leaudio_codec_type, DECODE_SCENARIO);
}

/*
*get decode output length
*/
LE_SECTION_FUNC int task_leaudio_get_decode_output_length()
{
    return leaudio_calc_output_length(pAudio_leaudio_struct->leaudio_codec_type, DECODE_SCENARIO);
}

LE_SECTION_FUNC static unsigned int getAudioFormatInBytes(unsigned int input) {
    unsigned int output;

    switch (input) {
    case AUDIO_FORMAT_PCM_16_BIT:
        output = 2;
        break;
    case AUDIO_FORMAT_PCM_8_24_BIT:
        output = 3;
        break;
    case AUDIO_FORMAT_PCM_32_BIT:
        output = 4;
        break;
    default:
        output = 2;
        break;
    }
    return output;
}

LE_SECTION_FUNC static int interleaveData(char* left_buf, char* right_buf, char* out, int frame_num, int frame_size, int channels)
{
    int ch, i, total_write = 0;
    int offset = 0;
    char* p_in = NULL;
    char* p_out = out;

    for (i = 0; i < frame_num; i++)
    {
        offset = i * frame_size;
        for (ch = 0; ch < channels; ch++)
        {
            p_in = (ch == 0) ? (left_buf + offset) : (right_buf + offset) ;
            memcpy(p_out, p_in, frame_size);
            p_out += frame_size;
            total_write += frame_size;
        }
    }

//    LE_LOG_D("total_write:%d, frame_num = %d, frame_size = %d, channels = %d\n", total_write, frame_num, frame_size, channels);
    return total_write;
}

#ifdef LEAUDIO_DUMP_DATA_SUPPORT
LE_SECTION_FUNC static int _dump_reset_buffer(void)
{
    if (pAudio_leaudio_struct->dump_share_buf == NULL) {
        void * buf_ptr;
        buf_ptr = (void*)MTK_pvPortMalloc(sizeof(lea_dsp_share_buf_info_t) + LEA_DUMP_SHARE_BUFFER_SIZE, MTK_eMemDramNormalNC);
        if(buf_ptr != NULL) {
            memset(buf_ptr, 0, sizeof(lea_dsp_share_buf_info_t) + LEA_DUMP_SHARE_BUFFER_SIZE);
            pAudio_leaudio_struct->dump_share_buf = (lea_dsp_share_buf_info_t*)buf_ptr;
            pAudio_leaudio_struct->dump_share_buf->start_addr = (unsigned long)(buf_ptr + sizeof(lea_dsp_share_buf_info_t));
            pAudio_leaudio_struct->dump_share_buf->length = LEA_DUMP_SHARE_BUFFER_SIZE;
            LE_LOG_I("LEA share buffer base:%p, start_addr:%p", buf_ptr, (void *)pAudio_leaudio_struct->dump_share_buf->start_addr);
        } else {
            LE_LOG_E("lc3 dump buffer malloc fail!");
            return -1;
        }
    }
    pAudio_leaudio_struct->dump_share_buf->is_buf_full = false;
    pAudio_leaudio_struct->dump_share_buf->read_offset = 0;
    pAudio_leaudio_struct->dump_share_buf->write_offset = 0;
    return 0;
}

LE_SECTION_FUNC static void _dump_write(void *data, uint32_t data_size)
{
    uint16_t free_size;
    uint16_t unwrap_size;
    lea_dsp_share_buf_info_t *share_buf = pAudio_leaudio_struct->dump_share_buf;
    uint16_t read_offset = share_buf->read_offset;
    uint8_t *start_ptr = (uint8_t*)share_buf->start_addr;
    lc3_block_size = data_size;

    free_size = (share_buf->length - share_buf->write_offset + read_offset) % share_buf->length;
    if (free_size == 0) {
        if (share_buf->is_buf_full) {
            free_size = 0;
        }else {
            free_size = share_buf->length;
        }
    }

    if (share_buf->is_buf_full && (share_buf->write_offset != read_offset))
        share_buf->is_buf_full = false;
#if 0
    LE_LOG_I("ds:%04d, fs:%04d, wf:%04d, rf:%04d, if:%d, len:%04d,",
             data_size,
             free_size,
             share_buf->write_offset,
             read_offset,
             share_buf->is_buf_full,
             share_buf->length);
#endif
    if (free_size < data_size || data_size == 0) {
        LE_LOG_W("ds:%04d, fs:%04d, if:%d, wf:%04d, rf:%04d",
                 data_size,
                 free_size,
                 share_buf->is_buf_full,
                 share_buf->write_offset,
                 share_buf->read_offset);
        if (free_size < data_size) {
            LE_LOG_E("Not enough space!");
        }
        return;
    }

    if (read_offset > share_buf->write_offset) {
        memcpy(start_ptr + share_buf->write_offset, data, data_size);
    } else {
        unwrap_size = share_buf->length - share_buf->write_offset;
        if (unwrap_size >= data_size) {
            memcpy(start_ptr + share_buf->write_offset, data, data_size);
        } else {
            memcpy(start_ptr + share_buf->write_offset, data, unwrap_size);
            memcpy(start_ptr, data + unwrap_size, data_size - unwrap_size);
        }
    }

    share_buf->write_offset = (share_buf->write_offset + data_size) % share_buf->length;
    if (share_buf->write_offset == read_offset) {
        share_buf->is_buf_full = true;
    }
}

LE_SECTION_FUNC static void _dump_handler(void)
{
    ipi_msg_t ipi_msg;
    int ret;
    ret = audio_send_ipi_msg(&ipi_msg,
                             TASK_SCENE_LEAUDIO_DECODE,
                             AUDIO_IPI_LAYER_TO_KERNEL,
                             AUDIO_IPI_MSG_ONLY,
                             AUDIO_IPI_MSG_BYPASS_ACK,
                             MSG_TO_HOST_DSP_LEA_DEBUG_DUMP_REQ,
                             0,
                             0,
                             NULL);
    if (ret) {
        LE_LOG_E("Le DUMP ipi cmd error!:%d", ret);
    }
}
#endif

LE_SECTION_FUNC static unsigned int task_leaudio_decode_process(unsigned int request_length, unsigned int read_length, int bfi_ext, bool is_left)
{
    uint32_t inbuf_len = 0, outbuf_len = 0;
    uint32_t total_output_size = 0;
    uint32_t lc3_size_per_decode = task_leaudio_get_decode_input_length();
    uint32_t pcm_size_per_decode = task_leaudio_get_decode_output_length();

    if (bfi_ext != 0) {
        LE_LOG_I("bfi_ext = %d, do plc\n", bfi_ext);
    }

    if (pAudio_leaudio_struct->bLeaudioIsPaused == true) {
        LE_LOG_I("bLeaudioIsPaused, return...\n");
        return 0;
    }

    if (pcm_size_per_decode == 0) {
        LE_LOG_E("pcm_size_per_decode is 0, LC3 lib might not be enabled\n");
        return 0;
    }

    if (read_length < request_length) {
        int need_feed = request_length - read_length;
        memset(LEAUDIO_DECODER_INPUT_BUF + read_length, 0, need_feed);
//        LE_LOG_D("read data %d, decoder need %d, feeded zero %d\n",
//                 read_length, request_length, need_feed);
    }

#ifdef LEAUDIO_DUMP_DATA_SUPPORT
    if (pAudio_leaudio_struct->isDumpStart && pAudio_leaudio_struct->isDumpReady) {
        lc3_packet_size = lc3_size_per_decode;
        _dump_write(LEAUDIO_DECODER_INPUT_BUF, request_length);
    }
#endif

    while (inbuf_len + lc3_size_per_decode <= request_length
            && inbuf_len + lc3_size_per_decode <= LEAUDIO_DECODER_INPUT_BUF_SIZE
            && total_output_size + pcm_size_per_decode <= LEAUDIO_DECODER_PROC_BUF_SIZE) {
        //decode 1 frame each time
        total_output_size += leaudio_do_decode(get_leaudio_codec_type(),
                                               LEAUDIO_DECODER_INPUT_BUF + inbuf_len,
                                               lc3_size_per_decode,
                                               LEAUDIO_DECODER_PROC_BUF + outbuf_len,
                                               LEAUDIO_DECODER_PROC_BUF_SIZE,
                                               bfi_ext,
                                               is_left);

        inbuf_len += lc3_size_per_decode;
        outbuf_len += pcm_size_per_decode;
    }

    return total_output_size;
}

LE_SECTION_FUNC void dsp_task_leaudio_irq_handler(void)
{
#ifdef LEAUDIO_PERFORMANCE_DEBUG
    isr_go_time_ns = read_systimer_stamp_ns();
#endif
    pAudio_leaudio_struct->rx_data_time_stamp = read_systimer_stamp_ns();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    leaudio_task_state_t state = get_leaudio_state();

    if (state != LEAUDIO_TASK_IDLE) {
        uint32_t base_addr = LEAUDIO_ACI_CTRL_DATA_ADDR;
        LEAUDIO_SRAM_PTR = (uint32_t *)(base_addr - BTSRAM_REGION_1 +
                                        BTSRAM_INFRA_MAPPING);

        //Read ACI base info
        leaudio_read_aci_base_info();

        //Get data from BT FW
        get_data_from_bt();

        //process done, clear irq
        clear_connsys_leaudio_interrupt();

        if (pAudio_leaudio_struct->leaudioDecodeTask.mTask == 0) {
            LE_LOG_E("Decode task not started!");
            configASSERT(false);
        }

#ifdef LEAUDIO_PERFORMANCE_DEBUG
        isr_exit_time_ns = read_systimer_stamp_ns();
        isr_dur_time_ns = isr_exit_time_ns - isr_go_time_ns;
#endif
        vTaskNotifyGiveFromISR(pAudio_leaudio_struct->leaudioDecodeTask.mTask, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        //Should not run into this case
        LE_LOG_E("receive IRQ in AUDIO_TASK_IDLE state, just mask irq!");
        mask_irq(LX_BTAP_IRQ_B);
    }
}

LE_SECTION_FUNC static void task_leaudio_decode_init(void)
{
    LE_LOG_D("+%s()", __func__);
    struct LEAUDIO_FORMAT_INFO_T* format_info;
    format_info = leaudio_get_format_info();

    pAudio_leaudio_struct->leaudio_codec_type = set_leaudio_codec_type(format_info->codec_type);
    // codec init
    leaudio_decoder_init(get_leaudio_codec_type());

    LEAUDIO_DECODER_INPUT_BUF_SIZE = local_task_input_buffer_size;
    LEAUDIO_DECODER_PROC_BUF_SIZE = TASK_LEAUDIO_PROCESSING_BUFFER_SIZE;
    LEAUDIO_DECODER_OUTPUT_BUF_SIZE = local_task_output_buffer_size;

    LEAUDIO_DECODER_INPUT_BUF = (char*)MTK_pvPortMalloc(LEAUDIO_DECODER_INPUT_BUF_SIZE, LE_MALLOC_TYPE);
    configASSERT(LEAUDIO_DECODER_INPUT_BUF != NULL);
    memset(LEAUDIO_DECODER_INPUT_BUF, 0, LEAUDIO_DECODER_INPUT_BUF_SIZE);

    LEAUDIO_DECODER_PROC_BUF = (char*)MTK_pvPortMalloc(LEAUDIO_DECODER_PROC_BUF_SIZE, LE_MALLOC_TYPE);
    configASSERT(LEAUDIO_DECODER_PROC_BUF != NULL);
    memset(LEAUDIO_DECODER_PROC_BUF, 0 ,LEAUDIO_DECODER_PROC_BUF_SIZE);

    LEAUDIO_DECODER_OUTPUT_BUF = (char*)MTK_pvPortMalloc(LEAUDIO_DECODER_OUTPUT_BUF_SIZE, LE_MALLOC_TYPE);
    configASSERT(LEAUDIO_DECODER_OUTPUT_BUF != NULL);
    memset(LEAUDIO_DECODER_OUTPUT_BUF, 0, LEAUDIO_DECODER_OUTPUT_BUF_SIZE);

    pAudio_leaudio_struct->pLeftBuf = (char *)MTK_pvPortMalloc(LEAUDIO_DEFAULT_SIZE * sizeof(char), LE_MALLOC_TYPE);
    configASSERT(pAudio_leaudio_struct->pLeftBuf != NULL);
    memset(pAudio_leaudio_struct->pLeftBuf, 0, LEAUDIO_DEFAULT_SIZE * sizeof(char));

    pAudio_leaudio_struct->pRightBuf = (char *)MTK_pvPortMalloc(LEAUDIO_DEFAULT_SIZE * sizeof(char), LE_MALLOC_TYPE);
    configASSERT(pAudio_leaudio_struct->pRightBuf != NULL);
    memset(pAudio_leaudio_struct->pRightBuf, 0, LEAUDIO_DEFAULT_SIZE * sizeof(char));
}

LE_SECTION_FUNC static void task_leaudio_decode_deinit(void)
{
    LE_LOG_D("+%s()", __func__);

    int i;
    for (i = 0; i < LEAUDIOBUF_MAX_HANDLE_CNT; i++) {
        RingBuf_Reset(LEAUDIO_DECODE_BUF_PTR[i]->decoded_data_ring_buf);
    }
#if 0
    //reset callback func
    pAudio_leaudio_struct->ul_notify_cbk = NULL;
    pAudio_leaudio_struct->ul_audio_dump_func = NULL;
#endif

    // codec deinit
    leaudio_decoder_deinit(get_leaudio_codec_type());

    if (pAudio_leaudio_struct->pLeftBuf != NULL) {
        MTK_vPortFree(pAudio_leaudio_struct->pLeftBuf);
        pAudio_leaudio_struct->pLeftBuf = NULL;
    }

    if (pAudio_leaudio_struct->pRightBuf != NULL) {
        MTK_vPortFree(pAudio_leaudio_struct->pRightBuf);
        pAudio_leaudio_struct->pRightBuf = NULL;
    }

    if (LEAUDIO_DECODER_INPUT_BUF) {
        MTK_vPortFree(LEAUDIO_DECODER_INPUT_BUF);
        LEAUDIO_DECODER_INPUT_BUF = NULL;
    }

    if (LEAUDIO_DECODER_PROC_BUF) {
        MTK_vPortFree(LEAUDIO_DECODER_PROC_BUF);
        LEAUDIO_DECODER_PROC_BUF = NULL;
    }

    if (LEAUDIO_DECODER_OUTPUT_BUF) {
        MTK_vPortFree(LEAUDIO_DECODER_OUTPUT_BUF);
        LEAUDIO_DECODER_OUTPUT_BUF = NULL;
    }
}

LE_SECTION_FUNC static void task_leaudio_decode_task_loop(void* arg)
{
    LE_LOG_D("+%s\n", __func__);

    task_leaudio_decode_init();

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        leaudio_task_state_t state = get_leaudio_state();
        if(state == LEAUDIO_TASK_DEINIT || state == LEAUDIO_TASK_IDLE)
            break;
#ifdef LEAUDIO_PERFORMANCE_DEBUG
        dec_loop_go_time_ns = read_systimer_stamp_ns();
#endif
        if (pAudio_leaudio_struct->bLeaudioIsPaused == false) {

            xSemaphoreTake(pAudio_leaudio_struct->xLeAudioTaskSemphr, portMAX_DELAY);

            int i;
            unsigned int index_l = 0, index_r = 0;
            bool valid_conn_handle_l = false, valid_conn_handle_r = false;
            int total_write = 0, total_output_size = 0, bfi_ext = 0;
            int left_output_size = 0, right_output_size = 0;
            uint32_t data_cnt = 0, copy_size = 0;
            uint32_t nChannels = pAudio_leaudio_struct->fmt_attr.num_channels;
            //get the index of each connection handle
            for (i = 0; i < LEAUDIOBUF_MAX_HANDLE_CNT; i++) {
                if (pAudio_leaudio_struct->conn_handle_l != INVALID_CONNECTION_HANDLE &&
                        pAudio_leaudio_struct->conn_handle_l == LEAUDIO_DECODE_BUF_PTR[i]->connection_handle) {
                    index_l = i;
                    valid_conn_handle_l = true;
                    continue;
                }

                if (pAudio_leaudio_struct->conn_handle_r != INVALID_CONNECTION_HANDLE &&
                        pAudio_leaudio_struct->conn_handle_r == LEAUDIO_DECODE_BUF_PTR[i]->connection_handle) {
                    index_r = i;
                    valid_conn_handle_r = true;
                    continue;
                }
            }

            if (!(valid_conn_handle_l) && !(valid_conn_handle_r)) {
                LE_LOG_E("There is no valid connection handle, return\n");
                xSemaphoreGive(pAudio_leaudio_struct->xLeAudioTaskSemphr);
                continue;
            }

            if (valid_conn_handle_l) {
                data_cnt = LEAUDIO_DECODE_BUF_PTR[index_l]->read_length;
                bfi_ext = LEAUDIO_DECODE_BUF_PTR[index_l]->pkt_status;
                LEAUDIO_DECODE_BUF_PTR[index_l]->pkt_status = 0;

                total_write = write_data_to_buf(LEAUDIO_DECODE_BUF_PTR[index_l]->decoded_data_ring_buf,
                                                data_cnt, LEAUDIO_DECODER_INPUT_BUF);

                //start decode process
                left_output_size = task_leaudio_decode_process(data_cnt, total_write, bfi_ext, true);
                memset(pAudio_leaudio_struct->pLeftBuf, 0, LEAUDIO_DEFAULT_SIZE * sizeof(char));
                configASSERT(left_output_size <= LEAUDIO_DEFAULT_SIZE * sizeof(char));
                memcpy(pAudio_leaudio_struct->pLeftBuf, LEAUDIO_DECODER_PROC_BUF, left_output_size);
            } else if (nChannels == STEREO_CHANNEL) {
                //Fill 0 data into dump
                total_write = LEAUDIO_DECODE_BUF_PTR[index_r]->read_length;
                memset(LEAUDIO_DECODER_INPUT_BUF, 0, LEAUDIO_DECODER_INPUT_BUF_SIZE);
                memset(pAudio_leaudio_struct->pLeftBuf, 0, LEAUDIO_DEFAULT_SIZE * sizeof(char));
                left_output_size = 0;

#ifdef LEAUDIO_DUMP_DATA_SUPPORT
                if (pAudio_leaudio_struct->isDumpStart && pAudio_leaudio_struct->isDumpReady) {
                    _dump_write(LEAUDIO_DECODER_INPUT_BUF, total_write);
                }
#endif
            }

            if (valid_conn_handle_r) {
                data_cnt = LEAUDIO_DECODE_BUF_PTR[index_r]->read_length;
                bfi_ext = LEAUDIO_DECODE_BUF_PTR[index_r]->pkt_status;
                LEAUDIO_DECODE_BUF_PTR[index_r]->pkt_status = 0;

                total_write = write_data_to_buf(LEAUDIO_DECODE_BUF_PTR[index_r]->decoded_data_ring_buf,
                                                data_cnt, LEAUDIO_DECODER_INPUT_BUF);

                //start decode process
                right_output_size = task_leaudio_decode_process(data_cnt, total_write, bfi_ext, false);
                memset(pAudio_leaudio_struct->pRightBuf, 0, LEAUDIO_DEFAULT_SIZE * sizeof(char));
                configASSERT(right_output_size <= LEAUDIO_DEFAULT_SIZE * sizeof(char));
                memcpy(pAudio_leaudio_struct->pRightBuf, LEAUDIO_DECODER_PROC_BUF, right_output_size);
            } else if (nChannels == STEREO_CHANNEL) {
                //Fill 0 data into dump
                total_write = LEAUDIO_DECODE_BUF_PTR[index_l]->read_length;
                memset(LEAUDIO_DECODER_INPUT_BUF, 0, LEAUDIO_DECODER_INPUT_BUF_SIZE);
                memcpy(pAudio_leaudio_struct->pRightBuf, LEAUDIO_DECODER_PROC_BUF, right_output_size);
                right_output_size = 0;

#ifdef LEAUDIO_DUMP_DATA_SUPPORT
                if (pAudio_leaudio_struct->isDumpStart && pAudio_leaudio_struct->isDumpReady) {
                    _dump_write(LEAUDIO_DECODER_INPUT_BUF, total_write);
                }
#endif
            }

            if (nChannels == STEREO_CHANNEL) {
                //STEREO case, should interleave data
                int frame_size, frame_num;
                char* p_left_buf = pAudio_leaudio_struct->pLeftBuf;
                char* p_right_buf = pAudio_leaudio_struct->pRightBuf;
                int pcm_total_size = left_output_size + right_output_size;

                if (!(valid_conn_handle_l) || !(valid_conn_handle_r)) {
                    //STEREO case with only 1 earpieces connected
//                    index_l = valid_conn_handle_l ? index_l : index_r;
//                    index_r = valid_conn_handle_r ? index_r : index_l;

                    p_left_buf = valid_conn_handle_l ? pAudio_leaudio_struct->pLeftBuf : pAudio_leaudio_struct->pRightBuf;
                    p_right_buf = valid_conn_handle_r ? pAudio_leaudio_struct->pRightBuf : pAudio_leaudio_struct->pLeftBuf;
                    pcm_total_size *= 2; // one of the buffer must have no data and its data size must be 0.
                }

                frame_size = getAudioFormatInBytes(leaudio_get_audio_format(get_leaudio_codec_type()));
                frame_num = pcm_total_size / frame_size / nChannels;
                configASSERT(pcm_total_size <= local_task_output_buffer_size);

                total_output_size = interleaveData(p_left_buf, p_right_buf, LEAUDIO_DECODER_OUTPUT_BUF,
                                                   frame_num, frame_size, nChannels);
            } else {
                //MONO case, just read data
                char* p_data_buf = valid_conn_handle_l ? pAudio_leaudio_struct->pLeftBuf : pAudio_leaudio_struct->pRightBuf;
                total_output_size = valid_conn_handle_l ? left_output_size : right_output_size;

                memcpy(LEAUDIO_DECODER_OUTPUT_BUF, p_data_buf, total_output_size);
            }

            //copy data to ring buf of specific connection handle
            //TODO: copy data to playback.
//            audio_dump_lc3_helper_write(1, LEAUDIO_DECODER_OUTPUT_BUF, total_output_size);
            taskENTER_CRITICAL();
            copy_size = wirte_data_to_playback((SHARE_BUFFER_INFO_PTR)pAudio_leaudio_struct->lea_playback_info_ptr,
                                               total_output_size,
                                               LEAUDIO_DECODER_OUTPUT_BUF);
            taskEXIT_CRITICAL();
            if(pAudio_leaudio_struct->first_frame_in == false) {
                pAudio_leaudio_struct->first_frame_in = true;
                trigger_playback_sink_out();
            }

#ifdef LEAUDIO_DUMP_DATA_SUPPORT
//            _dump_write(LEAUDIO_DECODER_OUTPUT_BUF, total_output_size);
            if (pAudio_leaudio_struct->isDumpStart && pAudio_leaudio_struct->isDumpReady)
                _dump_handler();
#endif

#ifdef LEAUDIO_PERFORMANCE_DEBUG
            playback_time_ns = read_systimer_stamp_ns();
            signed long long dur = playback_time_ns - isr_go_time_ns;
            unsigned long long isr_interval = isr_go_time_ns - isr_previous_go_time_ns;
            LE_LOG_D("i:%lld, itv:%lld, d:%lld, p:%lld, dur:%lld, sz:%d", isr_go_time_ns, isr_interval, dec_loop_go_time_ns, playback_time_ns, dur, copy_size);
            if (isr_go_time_ns > playback_time_ns)
                LE_LOG_W("ISR > PB");
            isr_previous_go_time_ns = isr_go_time_ns;
#endif
            xSemaphoreGive(pAudio_leaudio_struct->xLeAudioTaskSemphr);
        }
    }

    task_leaudio_decode_deinit();
    pAudio_leaudio_struct->leaudioDecodeTask.mTask = NULL;
    vTaskDelete(NULL);
}

LE_SECTION_FUNC static void leaudio_structure_init(void)
{
    LE_LOG_D("%s", __func__);
    int i;
    pAudio_leaudio_struct = MTK_pvPortMalloc(sizeof(struct audio_leaudio_struct), LE_MALLOC_TYPE);
    if (pAudio_leaudio_struct == NULL) {
        configASSERT(false);
        return;
    }
    memset(pAudio_leaudio_struct, 0, sizeof(struct audio_leaudio_struct));

    // Create Semaphore
    pAudio_leaudio_struct->xLeAudioTaskSemphr = xSemaphoreCreateCounting(1, 0);
    if (pAudio_leaudio_struct->xLeAudioTaskSemphr == NULL)
        configASSERT(false);

#if 0
    pAudio_leaudio_struct->xResourceSemphr = NULL;
    pAudio_leaudio_struct->xResourceSemphr = xSemaphoreCreateCounting(1, 1);
    if (pAudio_leaudio_struct->xResourceSemphr == NULL)
        LE_LOG_W("%s() xResourceSemphr create fail!", __func__);
#endif

    for (i = 0; i < LEAUDIOBUF_MAX_HANDLE_CNT; i++) {
        // init decode data buf
        LEAUDIO_DECODE_BUF_PTR[i] = (struct LEAUDIO_DECODE_BUF_T*)MTK_pvPortMalloc(
                                        sizeof(struct LEAUDIO_DECODE_BUF_T), LE_MALLOC_TYPE);
        memset(LEAUDIO_DECODE_BUF_PTR[i], 0, sizeof(struct LEAUDIO_DECODE_BUF_T));

        LEAUDIO_DECODE_BUF_PTR[i]->connection_handle = INVALID_CONNECTION_HANDLE;
        LEAUDIO_DECODE_BUF_PTR[i]->decoded_data_ring_buf = MTK_pvPortMalloc(sizeof(struct RingBuf), LE_MALLOC_TYPE);
        LEAUDIO_DECODE_BUF_PTR[i]->pDecodedRingbufAddr = MTK_pvPortMalloc(DECODED_RINGBUF_SIZE, LE_MALLOC_TYPE);
        memset(LEAUDIO_DECODE_BUF_PTR[i]->pDecodedRingbufAddr, 0, DECODED_RINGBUF_SIZE);

        init_ring_buf(LEAUDIO_DECODE_BUF_PTR[i]->decoded_data_ring_buf,
                      LEAUDIO_DECODE_BUF_PTR[i]->pDecodedRingbufAddr, DECODED_RINGBUF_SIZE);
    }

    pAudio_leaudio_struct->leaudioDecodeTask.mTask = 0;
    pAudio_leaudio_struct->leaudioDecodeTask.pcTaskName = "Leaudio Decode task";
    pAudio_leaudio_struct->leaudioDecodeTask.task_stack_size = LOCAL_TASK_STACK_SIZE / sizeof(StackType_t);
    pAudio_leaudio_struct->leaudioDecodeTask.task_priority = LOCAL_TASK_PRIORITY;
    pAudio_leaudio_struct->leaudioDecodeTask.mTaskLoopFunction = task_leaudio_decode_task_loop;

    pAudio_leaudio_struct->leaudioAciCtrlBaseInfo.isAciBaseInfoRead = false;
}

LE_SECTION_FUNC static void leaudio_structure_deinit(void)
{
    int i;
    LE_LOG_D("+%s()", __func__);
    configASSERT(pAudio_leaudio_struct != NULL);

    task_leaudio_task_stop(&pAudio_leaudio_struct->leaudioDecodeTask);

    if (pAudio_leaudio_struct->xLeAudioTaskSemphr)
        vSemaphoreDelete(pAudio_leaudio_struct->xLeAudioTaskSemphr);

#if 0
    if (pAudio_leaudio_struct->xResourceSemphr)
        MTK_vPortFree(pAudio_leaudio_struct->xResourceSemphr);
#endif

    for (i = 0; i < LEAUDIOBUF_MAX_HANDLE_CNT; i++) {
        if (LEAUDIO_DECODE_BUF_PTR[i] == NULL) {
            continue;
        }
        if (LEAUDIO_DECODE_BUF_PTR[i]->pDecodedRingbufAddr != NULL) {
            MTK_vPortFree(LEAUDIO_DECODE_BUF_PTR[i]->pDecodedRingbufAddr);
        }
        if (LEAUDIO_DECODE_BUF_PTR[i]->decoded_data_ring_buf != NULL) {
            MTK_vPortFree(LEAUDIO_DECODE_BUF_PTR[i]->decoded_data_ring_buf);
        }
    }

    MTK_vPortFree(pAudio_leaudio_struct);
    pAudio_leaudio_struct = NULL;
}

LE_SECTION_FUNC static void task_leaudio_constructor(AUDIO_TASK *this)
{
    configASSERT(this != NULL);

    this->task_priv = NULL;
    this->msg_queue = aud_create_msg_queue(MAX_MSG_QUEUE_SIZE);
    configASSERT(this->msg_queue != NULL);
}

LE_SECTION_FUNC static void task_leaudio_destructor(AUDIO_TASK *this)
{
    configASSERT(this != NULL);

    set_leaudio_state(LEAUDIO_TASK_DEINIT);

    leaudio_structure_deinit();
    vQueueDelete(this->msg_queue);
    this->msg_queue = NULL;
}
#if 0
LE_SECTION_FUNC static void _leaudio_dsp_task_msg_setpram(ipi_msg_t *ipi_msg)
{
    configASSERT(pAudio_leaudio_struct != NULL);
//    uint32_t state = get_leaudio_state();
    switch(ipi_msg->param2) {
        /* TODO define leaudio param if needed */
    case DSP_BT_BLE_START:
        break;
    case DSP_BT_BLE_STOP:

        break;
    case DSP_BT_BLE_LC3_CODEC: {

        break;
    }
    case DSP_BT_BLE_CH_UPDATE: {
        int i;
        struct bt_ConnParam ConnParam;
        if (ipi_msg->data_type != AUDIO_IPI_PAYLOAD) {
            LE_LOG_W("data_type 0x%x is not payload!\n", ipi_msg->data_type);
            break;
        }

        if (ipi_msg->param1 != sizeof (struct bt_ConnParam)) {
            LE_LOG_W("param1 0x%x, size mismatch!\n", ipi_msg->param1);
            break;
        }

        memcpy(&ConnParam, ipi_msg->payload, ipi_msg->param1);

        //set BT conn param
        pAudio_leaudio_struct->conn_handle_l = ConnParam.conn_handle_L;
        pAudio_leaudio_struct->conn_handle_r = ConnParam.conn_handle_R;
        pAudio_leaudio_struct->bnNum = ConnParam.bn;

        LE_LOG_D("DSP_BT_BLE_CH_UPDATE conn_handle_l = %d, conn_handle_r = %d, bnNum = %d",
                 pAudio_leaudio_struct->conn_handle_l,
                 pAudio_leaudio_struct->conn_handle_r,
                 pAudio_leaudio_struct->bnNum);

        for (i = 0; i < LEAUDIOBUF_MAX_HANDLE_CNT; i++) {
            if (pAudio_leaudio_struct->conn_handle_l == INVALID_CONNECTION_HANDLE ||
                    LEAUDIO_DECODE_BUF_PTR[i]->connection_handle == pAudio_leaudio_struct->conn_handle_l) {
                //handle already exists or invalid handle
                break;
            }

            if (LEAUDIO_DECODE_BUF_PTR[i]->connection_handle == INVALID_CONNECTION_HANDLE) {
                //add an entry for new handle
                LEAUDIO_DECODE_BUF_PTR[i]->connection_handle = pAudio_leaudio_struct->conn_handle_l;
                break;
            }
        }

        for (i = 0; i < LEAUDIOBUF_MAX_HANDLE_CNT; i++) {
            if (pAudio_leaudio_struct->conn_handle_r == INVALID_CONNECTION_HANDLE ||
                    LEAUDIO_DECODE_BUF_PTR[i]->connection_handle == pAudio_leaudio_struct->conn_handle_r) {
                //handle already exists or invalid handle
                break;
            }

            if (LEAUDIO_DECODE_BUF_PTR[i]->connection_handle == INVALID_CONNECTION_HANDLE) {
                //add an entry for new handle
                LEAUDIO_DECODE_BUF_PTR[i]->connection_handle = pAudio_leaudio_struct->conn_handle_r;
                break;
            }
        }

        dump_bt_connparam(ConnParam);
        break;
    }
    default:
        LE_LOG_W("AUDIO_DSP_TASK_SETPRAM(%d) not supported.\n", ipi_msg->param2);
    }
}
#endif

LE_SECTION_FUNC static void task_leaudio_task_loop(void *pvParameters)
{
    LE_LOG_I("+%s",__func__);

    AUDIO_TASK* this = (AUDIO_TASK*)pvParameters;
    configASSERT(this != NULL);
    BaseType_t ret;
    ipi_msg_t ipi_msg = {0};
    leaudio_structure_init();
    set_leaudio_state(LEAUDIO_TASK_IDLE);

    while (1) {
        //wait for queue
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

        ret = xQueueReceive(this->msg_queue, &ipi_msg, 0);
        configASSERT(ret == pdTRUE);

        /* process message */
        switch (ipi_msg.msg_id) {
        case MSG_TO_DSP_LEAUDIO_OPEN: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_OPEN");
            struct bt_LeAudioCodecConfiguration codec_config;
            if (ipi_msg.data_type != AUDIO_IPI_PAYLOAD) {
                LE_LOG_E("data_type 0x%x is not payload!", ipi_msg.data_type);
                break;
            }
            if (ipi_msg.payload == NULL) {
                LE_LOG_E("payload ptr is NULL!");
                break;
            }

            if (ipi_msg.param1 != sizeof (struct bt_LeAudioCodecConfiguration)) {
                LE_LOG_E("param1 0x%x, size mismatch!", ipi_msg.param1);
                break;
            }

            memcpy(&codec_config, ipi_msg.payload, ipi_msg.param1);

            task_leaudio_set_codecinfo(codec_config.codecType, &codec_config);

            dump_lc3_codec(codec_config);

            set_leaudio_state(LEAUDIO_TASK_OPEN);
            pAudio_leaudio_struct->bLeaudioIsPaused = false;
            pAudio_leaudio_struct->first_frame_in = false;
            pAudio_leaudio_struct->presentation_delay = codec_config.presentation_delay;
            query_playback_shared_mem(&pAudio_leaudio_struct->lea_playback_info_ptr);
            configASSERT(pAudio_leaudio_struct->lea_playback_info_ptr != 0);
            LE_LOG_D("playback_buf_info_ptr:%p", (uint32_t*)pAudio_leaudio_struct->lea_playback_info_ptr);

            if (pAudio_leaudio_struct->leaudioDecodeTask.mTask == 0) {
                task_leaudio_task_start(&pAudio_leaudio_struct->leaudioDecodeTask);
            }

            pAudio_leaudio_struct->bnNum = LEAUDIO_DEFAULT_BNNUMBER;
            pAudio_leaudio_struct->fmt_attr.audio_format = get_leaudio_codec_pcmformat(pAudio_leaudio_struct->leaudio_codec_type);
            pAudio_leaudio_struct->fmt_attr.num_channels = get_leaudio_codec_channel(pAudio_leaudio_struct->leaudio_codec_type);
            pAudio_leaudio_struct->fmt_attr.sample_rate = get_leaudio_codec_samplerate(pAudio_leaudio_struct->leaudio_codec_type);

            request_irq(LX_BTAP_IRQ_B, dsp_task_leaudio_irq_handler, "AUDIO_LEAUDIO_DECODE");

            xSemaphoreGive(pAudio_leaudio_struct->xLeAudioTaskSemphr);
            break;
        }
        case MSG_TO_DSP_LEAUDIO_SETPRAM: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_SETPRAM");
//            _leaudio_dsp_task_msg_setpram(&ipi_msg);
            break;
        }
        case MSG_TO_DSP_LEAUDIO_START: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_START");
            set_leaudio_state(LEAUDIO_TASK_WORKING);
            pAudio_leaudio_struct->bLeaudioStreamStatus = true;
            break;
        }
        case MSG_TO_DSP_LEAUDIO_STOP: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_STOP");
            pAudio_leaudio_struct->bLeaudioStreamStatus = false;
            pAudio_leaudio_struct->first_frame_in = false;
            break;
        }
        case MSG_TO_DSP_LEAUDIO_CLOSE: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_CLOSE");
            int i;
            set_leaudio_state(LEAUDIO_TASK_DEINIT);

            mask_irq(LX_BTAP_IRQ_B);
            set_leaudio_state(LEAUDIO_TASK_IDLE);

            pAudio_leaudio_struct->bLeaudioIsPaused = true;
            pAudio_leaudio_struct->first_frame_in = false;

            //clear handle
            pAudio_leaudio_struct->conn_handle_l = INVALID_CONNECTION_HANDLE;
            pAudio_leaudio_struct->conn_handle_r = INVALID_CONNECTION_HANDLE;

            for (i = 0; i < LEAUDIOBUF_MAX_HANDLE_CNT; i++) {
                LEAUDIO_DECODE_BUF_PTR[i]->connection_handle = INVALID_CONNECTION_HANDLE;
            }
#ifdef LEAUDIO_DUMP_DATA_SUPPORT
            LE_LOG_I("LC3 [packet size:%d, block size:%d]", lc3_packet_size, lc3_block_size);
#endif
            xSemaphoreTake(pAudio_leaudio_struct->xLeAudioTaskSemphr,pdMS_TO_TICKS(30));
            xTaskNotifyGive(pAudio_leaudio_struct->leaudioDecodeTask.mTask);
            break;
        }
        case MSG_TO_DSP_LEAUDIO_CH_UPDATE: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_CH_UPDATE");
            int i;
            struct bt_ConnParam ConnParam;
            if (ipi_msg.data_type != AUDIO_IPI_PAYLOAD) {
                LE_LOG_W("data_type 0x%x is not payload!", ipi_msg.data_type);
                break;
            }
            if (ipi_msg.payload == NULL) {
                LE_LOG_W("payload ptr is NULL!");
                break;
            }
            if (ipi_msg.param1 != sizeof (struct bt_ConnParam)) {
                LE_LOG_W("param1 0x%x, size mismatch!", ipi_msg.param1);
                break;
            }

            memcpy(&ConnParam, ipi_msg.payload, ipi_msg.param1);

            //set BT conn param
            pAudio_leaudio_struct->conn_handle_l = ConnParam.conn_handle_L;
            pAudio_leaudio_struct->conn_handle_r = ConnParam.conn_handle_R;
            pAudio_leaudio_struct->bnNum = ConnParam.bn;

            LE_LOG_I("DSP_BT_BLE_CH_UPDATE conn_handle_l = %d, conn_handle_r = %d, bnNum = %d",
                     pAudio_leaudio_struct->conn_handle_l,
                     pAudio_leaudio_struct->conn_handle_r,
                     pAudio_leaudio_struct->bnNum);

            for (i = 0; i < LEAUDIOBUF_MAX_HANDLE_CNT; i++) {
                if (pAudio_leaudio_struct->conn_handle_l == INVALID_CONNECTION_HANDLE ||
                        LEAUDIO_DECODE_BUF_PTR[i]->connection_handle == pAudio_leaudio_struct->conn_handle_l) {
                    //handle already exists or invalid handle
                    break;
                }

                if (LEAUDIO_DECODE_BUF_PTR[i]->connection_handle == INVALID_CONNECTION_HANDLE) {
                    //add an entry for new handle
                    LEAUDIO_DECODE_BUF_PTR[i]->connection_handle = pAudio_leaudio_struct->conn_handle_l;
                    break;
                }
            }

            for (i = 0; i < LEAUDIOBUF_MAX_HANDLE_CNT; i++) {
                if (pAudio_leaudio_struct->conn_handle_r == INVALID_CONNECTION_HANDLE ||
                        LEAUDIO_DECODE_BUF_PTR[i]->connection_handle == pAudio_leaudio_struct->conn_handle_r) {
                    //handle already exists or invalid handle
                    break;
                }

                if (LEAUDIO_DECODE_BUF_PTR[i]->connection_handle == INVALID_CONNECTION_HANDLE) {
                    //add an entry for new handle
                    LEAUDIO_DECODE_BUF_PTR[i]->connection_handle = pAudio_leaudio_struct->conn_handle_r;
                    break;
                }
            }
            break;
        }
        case MSG_TO_DSP_LEAUDIO_SUSPEND: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_SUSPEND");
            pAudio_leaudio_struct->bLeaudioIsPaused = true;
            pAudio_leaudio_struct->first_frame_in = false;
            break;
        }
        case MSG_TO_DSP_LEAUDIO_RESUME: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_RESUME");
            pAudio_leaudio_struct->bLeaudioIsPaused = false;
            break;
        }
#ifdef LEAUDIO_DUMP_DATA_SUPPORT
        case MSG_TO_DSP_LEAUDIO_DEBUG_READY: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_DEBUG_READY");
            if (ipi_msg.param1 != sizeof(lea_host_dump_param_t)) {
                LE_LOG_E("parameter size miss matching!");
                break;
            }
            lea_host_dump_param_t dsp_param_ack;

            if(_dump_reset_buffer()) {
                dsp_param_ack.dsp_buf_info_base = 0;
                dsp_param_ack.is_inited = false;
                break;
            } else {
                dsp_param_ack.dsp_buf_info_base = (unsigned long)pAudio_leaudio_struct->dump_share_buf;
                dsp_param_ack.is_inited = true;
                pAudio_leaudio_struct->isDumpReady = true;
            }
            memcpy(ipi_msg.payload, &dsp_param_ack, ipi_msg.param1);
            lc3_packet_size = 0;
            lc3_block_size = 0;
            break;
        }
        case MSG_TO_DSP_LEAUDIO_DEBUG_START: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_DEBUG_START");
            pAudio_leaudio_struct->isDumpStart = true;
            break;
        }
        case MSG_TO_DSP_LEAUDIO_DEBUG_STOP: {
            LE_LOG_I("process MSG_TO_DSP_LEAUDIO_DEBUG_STOP");
            pAudio_leaudio_struct->isDumpStart = false;
            pAudio_leaudio_struct->isDumpReady = false;
            if (pAudio_leaudio_struct->dump_share_buf) {
                MTK_vPortFree(pAudio_leaudio_struct->dump_share_buf);
                pAudio_leaudio_struct->dump_share_buf = NULL;
            }
            break;
        }
#endif
        default:
            LE_LOG_E("msg_id %d not implement yet,thread_id:%d",
                     ipi_msg.msg_id, this->thread_id);
            break;
        }

        /* send ack back if need */
        audio_send_ipi_msg_ack_back(&ipi_msg);
        memset(&ipi_msg, 0, sizeof(ipi_msg_t));
    }
}

LE_SECTION_FUNC static void task_leaudio_recv_message(AUDIO_TASK* this, struct ipi_msg_t* ipi_msg)
{
    BaseType_t ret;
    AUDIO_TASK* task;

    LE_LOG_D("recv ipi_msg->msg_id 0x%x", ipi_msg->msg_id);

    task = aud_get_audio_task(TASK_SCENE_LEAUDIO_DECODE);

    ret = xQueueSendToBack(task->msg_queue, ipi_msg, 0);
    if (ret != pdPASS) {
        LE_LOG_E("send msg failed");
        return;
    }

    xTaskNotifyGive(task->thread_handler);
}

LE_SECTION_RODATA const AUDIO_TASK_OPS g_aud_task_leaudio_decode_msg_ops = {
    .constructor = task_leaudio_constructor,
    .destructor = task_leaudio_destructor,
    .create_task_loop = aud_create_task_loop_common,
    .destroy_task_loop = aud_destroy_task_loop_common,
    .task_loop_func = task_leaudio_task_loop,
    .recv_message = task_leaudio_recv_message,
};

