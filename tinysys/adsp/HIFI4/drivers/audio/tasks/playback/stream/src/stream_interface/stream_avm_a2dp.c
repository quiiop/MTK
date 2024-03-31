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



#include <string.h>
#include "audio_types.h"
#include "source_inter.h"
#include "dsp_buffer.h"
#include "dtm.h"
#include "stream_n9_a2dp.h"
#include "stream_n9sco.h"
#include "long_term_clk_skew.h"
#include "dsp_audio_msg.h"
#include "dsp_audio_msg_define.h"
#include "dsp_audio_ctrl.h"
#include "dsp_memory.h"
#include "bt_interface.h"
#ifdef MTK_PEQ_ENABLE
#include "peq_interface.h"
#endif
#include "bt_types.h"
#include "swla.h"
//#define REDUNDANT

//log_create_module(strm_a2dp, PRINT_LEVEL_INFO);

/******************************************************************************
 * Definition
*******************************************************************************/
#define UNUSED(p) ((void)(p))
#define N9_A2DP_DEBUG_LOG   0
#define AAC_FRAME_DECODE_SAMPLE_NUM 1024
#define BUFFER_EMPTY_ASI_PADDING_NUM 1024
#define SBC_FRAME_DECODE_SAMPLE_NUM 128
#define AAC_BITRATE_REPORT_ACCUM_TIME 30
#define BUFFER_EMPTY_REINIT_THD 5
#define BUFFER_WAIT_PLR_ISR_NUM 3
#define BUFFER_WAIT_PLR_ISR_NUM_ALC 5
#define BUFFER_WAIT_PLR_ISR_NUM_GAMING_MODE 2
#define BUFFER_STATE_LOST_SKIP_MAX_NUM 1
#define ASI_MASK (0x3FFFFFF)
#define NOTIFY_LOW_BOND   80
#define NOTIFY_LOW_THD    200
#define NOTIFY_LOW_BOND_GAMING_MODE   20


#define REINIT_FLAG (0x7FFF)
#define RESEND_REINIT_CNT (30) // Resend interval is  roughtly about (20ms *  BUFFER_EMPTY_REINIT_THD * RESEND_REINIT_CNT)
#define SSRC_MAGIC_NUM 0x12345678

#define PCB_HEADER_LEN    4
#define RTP_HEADER_LEN    12
#define CP_LEN            1
#define PAYLOAD_HEADER    1
#define VEND_FRAME_HEADER 4
#define N9_A2DP_DEBUG_ASSERT (0)
#define MACRO_CHK_FIRST_PKT(param,state)                \
        {                                               \
            if(param->DspReportStartId == (0x8000 | MSG_DSP2MCU_BT_AUDIO_DL_START_ACK))\
            {                                           \
                STRM_A2DP_LOG_I("%s at first packet\n", 1, state);   \
            }                                           \
        }


#ifdef MTK_BT_A2DP_VENDOR_BC_ENABLE
#include "vendor_decoder_proting.h"
vend_bc_extern_buf_v2_t* p_vend_bc_exbuf_if;
#endif

#ifdef MTK_BT_A2DP_AIRO_CELT_ENABLE
#define MEDIATEK_PKT_FAKE_VERIFY          (FALSE)
#define MEDIATEK_GAMING_HS_DL_DBG_LOG     (TRUE)
#define MEDIATEK_GAMING_STD_ENC_FRM_SIZE  (80)
#define MEDIATEK_GAMING_FRAME_NO          (367/MEDIATEK_GAMING_STD_ENC_FRM_SIZE)

#if (MEDIATEK_PKT_FAKE_VERIFY)
#define FAKE_MEM_BLOCK_SIZE (400)
#define FAKE_MEM_BLOCK_NO   (10)

AVM_SHARE_BUF_INFO AiroFakeAvmShareBufInfo = {
    .StartAddr = 0,       // start address of share buffer
    .ReadIndex = 0,  // read pointer of share buffer  : DSP monitor
    .WriteIndex = 0, // write pointer of share buffer : Controller monitor
    .SampleRate = 48000, // sample rate for clock skew counting
    .MemBlkSize = FAKE_MEM_BLOCK_SIZE, // block length for each frame
    .MemBlkNum = FAKE_MEM_BLOCK_NO,  // number of block for frame usage
    .DbgInfoAddr = NULL, // start address of controller packet address table
    .FrameSampleNum = 0,  // DSP notify audio
    .codec_type = 0,      // Codec information
    .codec_config = 0,    // Codec config information
    .NotifyCount = 0,  // notify count of DSP notify controller not to sleep
    .ForwarderAddr = NULL, // forwarder buffer address, also used to report ASI compromise in A2DP
    .SinkLatency = 14000, // a2dp sink latency
};

U8 AiroFakeShareData[FAKE_MEM_BLOCK_SIZE*FAKE_MEM_BLOCK_NO];
U8 const AiroFakeDatabase[] = {
    #include "airo_fake_pattern.txt"
};
U32 AiroFakeDatabaseIdx;
#if 0
#define FAKE_BLOCK_SIZE                 (12+1+1+2+4+MEDIATEK_GAMING_FRAME_NO*(1+MEDIATEK_GAMING_STD_ENC_FRM_SIZE))
#else
#define FAKE_BLOCK_SIZE                 (1+1+2+4+MEDIATEK_GAMING_FRAME_NO*(1+MEDIATEK_GAMING_STD_ENC_FRM_SIZE))
#endif
#endif /* End of MEDIATEK_PKT_FAKE_VERIFY */

#endif

//#define A2DP_IDS_DEBUG_EN

#ifdef A2DP_IDS_DEBUG_EN

#define A2DP_IDS_DEBUG_GPIO_NO  HAL_GPIO_6
static bool is_gpio_init = false;
void gpio_init_and_set_low(void) {
    if(is_gpio_init == false) {
        hal_gpio_init(A2DP_IDS_DEBUG_GPIO_NO);
        hal_pinmux_set_function(A2DP_IDS_DEBUG_GPIO_NO, 0);
        hal_gpio_set_direction(A2DP_IDS_DEBUG_GPIO_NO, HAL_GPIO_DIRECTION_OUTPUT);
        hal_gpio_set_output(A2DP_IDS_DEBUG_GPIO_NO, 0);

        STRM_A2DP_LOG_I("[A2DP_IDS_DEBUG] GPIO init and set to low", 0);
        is_gpio_init = true;
    }
}

void gpio_revert(uint32_t revert_times) {
    for(uint32_t i=0; i<revert_times; i++) {
        hal_gpio_set_output(A2DP_IDS_DEBUG_GPIO_NO, 1);
        hal_gpio_set_output(A2DP_IDS_DEBUG_GPIO_NO, 0);
    }
    STRM_A2DP_LOG_I("[A2DP_IDS_DEBUG] GPIO revert %d times", 1, revert_times);
}

#endif

/******************************************************************************
 * Extern Functions
*******************************************************************************/
extern VOID StreamDSP_HWSemaphoreTake(VOID);
extern VOID StreamDSP_HWSemaphoreGive(VOID);
extern uint32_t afe_get_dl1_query_data_amount(void);
extern audio_dsp_ull_start_ctrl_param_t audio_headset_ull_ctrl;

/******************************************************************************
 * Static Functions
*******************************************************************************/
VOID SourceDrop_N9_a2dp(SOURCE source, U32 amount);
ATTR_TEXT_IN_IRAM_LEVEL_2 static VOID update_from_share_information(SOURCE source)
{
    #if MEDIATEK_PKT_FAKE_VERIFY
    if (source->param.n9_a2dp.codec_info.codec_cap.type == BT_A2DP_CODEC_AIRO_CELT)
    memcpy(&(source->streamBuffer.AVMBufferInfo.ReadIndex), (void *)(&AiroFakeAvmShareBufInfo.ReadIndex), sizeof(AVM_SHARE_BUF_INFO)-sizeof(U32));
    else
    #endif
    //StreamDSP_HWSemaphoreTake();
    memcpy(&(source->streamBuffer.AVMBufferInfo.ReadIndex), (void *)(source->param.n9_a2dp.share_info_base_addr + sizeof(U32)), sizeof(AVM_SHARE_BUF_INFO)-sizeof(U32));/* share info fix 32 byte */
}
static VOID update_to_share_information(SOURCE source)
{
    //StreamDSP_HWSemaphoreTake();
    /* Put your code here, if the semaphore is taken, do something for the critical resource...... */
    ((AVM_SHARE_BUF_INFO *)source->param.n9_a2dp.share_info_base_addr)->ReadIndex = (source->streamBuffer.AVMBufferInfo.ReadIndex);
    //StreamDSP_HWSemaphoreGive();
}

static VOID share_information_read_index_update(SOURCE source)
{
    U32 mask;
    U32 readoffset = source->streamBuffer.AVMBufferInfo.MemBlkSize*source->streamBuffer.AVMBufferInfo.ReadIndex;
    hal_nvic_save_and_set_interrupt_mask(&mask);
    ((bt_codec_a2dp_hdr_type_ptr)((U8*)source->streamBuffer.AVMBufferInfo.StartAddr + readoffset))->pcb_state = PCB_STATE_DECODED;
    source->streamBuffer.AVMBufferInfo.ReadIndex = (source->streamBuffer.AVMBufferInfo.ReadIndex + 1)%source->streamBuffer.AVMBufferInfo.MemBlkNum;
    update_to_share_information(source);
    hal_nvic_restore_interrupt_mask(mask);
}


#ifdef MTK_BT_A2DP_VENDOR_BC_ENABLE
static U8* reshape_share_buffer_for_vend_bc(SOURCE source,U16 length)
{
    return (U8*)(hal_memview_cm4_to_dsp0(((AVM_SHARE_BUF_INFO_PTR)source->param.n9_a2dp.share_info_base_addr)->StartAddr) + length);
}

#endif

ATTR_TEXT_IN_IRAM_LEVEL_1 static U32 a2dp_get_hdr_info(SOURCE source)
{
    AVM_SHARE_BUF_INFO *share_buff_info  = &(source->streamBuffer.AVMBufferInfo);
    N9_A2DP_PARAMETER *n9_a2dp_param    = &(source->param.n9_a2dp);
    U16 return_status = RETURN_PASS;
    bt_codec_a2dp_hdr_type_t a2dp_hdr_info;
    do {
        memcpy(&a2dp_hdr_info,(U8*)(share_buff_info->StartAddr + share_buff_info->MemBlkSize*share_buff_info->ReadIndex),sizeof(bt_codec_a2dp_hdr_type_t));
        
        /* Save global information */
        {
            n9_a2dp_param->current_frame_size = a2dp_hdr_info.frame_size;
        }

        if (a2dp_hdr_info.pcb_state == PCB_STATE_LOST)
        {
            //printf("meet state lost");
            //configASSERT(0);
            if (source->transform == NULL){
                return_status = RETURN_FAIL;
                break;
            }

            if (n9_a2dp_param->pkt_lost_report_state == PL_STATE_NONE)
            {
               n9_a2dp_param->pkt_lost_report_state = PL_STATE_REPORTPREVSEQN;
            }
#ifdef CFG_AUDIO_HARDWARE_ENABLE
            if ((afe_get_dl1_query_data_amount() > (((source->param.n9_a2dp.latency_monitor == true) ? BUFFER_WAIT_PLR_ISR_NUM_GAMING_MODE : \
            (source->param.n9_a2dp.alc_monitor) ? BUFFER_WAIT_PLR_ISR_NUM : BUFFER_WAIT_PLR_ISR_NUM_ALC)*(source->transform->sink->param.audio.rate * source->transform->sink->param.audio.period / 1000)))||(afe_get_bt_sync_monitor(AUDIO_DIGITAL_BLOCK_MEM_DL1) ==0)){
                S16 i;
                for (i = 1; i < (BUFFER_STATE_LOST_SKIP_MAX_NUM + 1); i++)
                {
                    memcpy(&a2dp_hdr_info,(U8*)(share_buff_info->StartAddr + share_buff_info->MemBlkSize*((source->streamBuffer.AVMBufferInfo.ReadIndex + i)%source->streamBuffer.AVMBufferInfo.MemBlkNum)),sizeof(bt_codec_a2dp_hdr_type_t));
                    if((a2dp_hdr_info.pcb_state == PCB_STATE_USED)&&(n9_a2dp_param->predict_asi == (a2dp_hdr_info.frame_asi & ASI_MASK)))
                    {
                        STRM_A2DP_LOG_I("PCB_STATE_LOST skip overpad block seqN:%d skip frame num:%d Wo %d Ro %d ", 4, n9_a2dp_param->current_seq_num,i,share_buff_info->WriteIndex,share_buff_info->ReadIndex);
                        for (;i>0;i--)
                        {
                            share_information_read_index_update(source);
                        }
                        break;
                    }
                }
                return_status = RETURN_FAIL;
                break;
            }
            else
#endif
            {
                U32 gpt_timer;
                if (share_buff_info->ReadIndex == share_buff_info->WriteIndex) {
                    return_status = RETURN_FAIL;
                    break;
                }
                hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &gpt_timer);
                share_information_read_index_update(source);
                STRM_A2DP_LOG_I("PCB_STATE_LOST SKIP seqN:%d skip size:%d Wo %d Ro %d timer %d", 5, n9_a2dp_param->current_seq_num,n9_a2dp_param->current_frame_size,share_buff_info->WriteIndex,share_buff_info->ReadIndex,gpt_timer);
                if (share_buff_info->ReadIndex == share_buff_info->WriteIndex) {
                    return_status = RETURN_FAIL;
                    break;
                }
            }
        }
        else if((a2dp_hdr_info.pcb_state == PCB_STATE_SKIP)||(a2dp_hdr_info.pcb_state == PCB_STATE_DECODED))
        {
            if ((share_buff_info->ReadIndex == share_buff_info->WriteIndex)&&(a2dp_hdr_info.pcb_state == PCB_STATE_DECODED)) {
                return_status = RETURN_FAIL;
                break;
            }
            share_information_read_index_update(source);
            STRM_A2DP_LOG_I("PCB STATE skip frame status %d ReadIndex:%d WriteIndex:%d", 3,a2dp_hdr_info.pcb_state,share_buff_info->ReadIndex,share_buff_info->WriteIndex);
        }
        else if ((a2dp_hdr_info.pcb_state == PCB_STATE_FEC)||(a2dp_hdr_info.pcb_state == PCB_STATE_FREE))
        {
            STRM_A2DP_LOG_I("Parse PCB meet rare state :%d Ro: %d  Wo: %d ",3,a2dp_hdr_info.pcb_state,share_buff_info->ReadIndex,share_buff_info->WriteIndex);
            return RETURN_FAIL;
        }
        else if (a2dp_hdr_info.pcb_state > PCB_STATE_DECODED)
        {
            if (share_buff_info->ReadIndex == share_buff_info->WriteIndex){
                return_status = RETURN_FAIL;
                break;
            }
            configASSERT(0);
        }
    } while(a2dp_hdr_info.pcb_state != PCB_STATE_USED);

    if (a2dp_hdr_info.frame_size > share_buff_info->MemBlkSize)// abnormal large packet
    {
        STRM_A2DP_LOG_E("a2dp illegal pkt size :%d Ro:%d Wo:%d", 3, a2dp_hdr_info.frame_size,share_buff_info->ReadIndex,share_buff_info->WriteIndex);
        if (share_buff_info->ReadIndex != share_buff_info->WriteIndex)
        {configASSERT(0);}
    }

    if (return_status == RETURN_PASS)
    {
        n9_a2dp_param->current_asi        =   a2dp_hdr_info.frame_asi & ASI_MASK;
        n9_a2dp_param->current_seq_num    =   a2dp_hdr_info.frame_pkt_seqn;
        n9_a2dp_param->current_frame_size =   a2dp_hdr_info.frame_size;
    }
    return return_status;
}

static VOID a2dp_skip_expired_packet(SOURCE source)
{
    AVM_SHARE_BUF_INFO *share_buff_info  = &(source->streamBuffer.AVMBufferInfo);
    N9_A2DP_PARAMETER *n9_a2dp_param    = &(source->param.n9_a2dp);

    if ((share_buff_info->WriteIndex - share_buff_info->ReadIndex) == 0)
    {
        STRM_A2DP_LOG_E("skip expired Wi %d = Ri %d", share_buff_info->WriteIndex,share_buff_info->ReadIndex);
        return;
    }
    STRM_A2DP_LOG_I("skip expired Wi %d Ri %d asi %d pasi %d", 4, share_buff_info->WriteIndex,share_buff_info->ReadIndex,n9_a2dp_param->current_asi, n9_a2dp_param->predict_asi);

    while((((S32)n9_a2dp_param->predict_asi - (S32)n9_a2dp_param->current_asi)) > 0){
        /* Check next packet is expired or not */
        share_information_read_index_update(source);
        if (a2dp_get_hdr_info(source) == RETURN_FAIL)
        {
            break;
        }
        if ((share_buff_info->WriteIndex - share_buff_info->ReadIndex) == 0)
        {
            STRM_A2DP_LOG_E("skip expired Wi %d = Ri %d, first", share_buff_info->WriteIndex,share_buff_info->ReadIndex);
            break;
        }
    }
    STRM_A2DP_LOG_I("skip expired Wi %d Ri %d asi %d pasi %d end", 4, share_buff_info->WriteIndex,share_buff_info->ReadIndex,n9_a2dp_param->current_asi, n9_a2dp_param->predict_asi);
}


ATTR_TEXT_IN_IRAM_LEVEL_2 static VOID sbc_get_frame_information(SOURCE source, bt_codec_a2dp_sbc_frame_header_t *sbc_frame_info)
{
    AVM_SHARE_BUF_INFO *share_buff_info  = &(source->streamBuffer.AVMBufferInfo);
    U32 read_offset;
    read_offset = share_buff_info->MemBlkSize*share_buff_info->ReadIndex + sizeof(bt_codec_a2dp_rtpheader_t);
    memcpy(sbc_frame_info,(U8*)(share_buff_info->StartAddr+ read_offset),sizeof(bt_codec_a2dp_sbc_frame_header_t));
    if ( sbc_frame_info->SyncWord != 0x9C ){
        STRM_A2DP_LOG_D("sync word error, %x\r\n", 1, sbc_frame_info->SyncWord);
    }
}


uint32_t a2dp_get_samplingrate(SOURCE source)
{
    uint32_t samplerate = 0;
    if (source->param.n9_a2dp.codec_info.codec_cap.type == BT_A2DP_CODEC_SBC ) {
        switch (source->param.n9_a2dp.codec_info.codec_cap.codec.sbc.sample_rate) {
            case 8:
                samplerate = 16000;
                break;
            case 4:
                samplerate = 32000;
                break;
            case 2:
                samplerate = 44100;
                break;
            case 1:
                samplerate = 48000;
                break;
            default:
                STRM_A2DP_LOG_E("sample rate info error/r/n", 0);
        }
    }
    else if (source->param.n9_a2dp.codec_info.codec_cap.type == BT_A2DP_CODEC_AAC ) {
        switch (source->param.n9_a2dp.codec_info.codec_cap.codec.aac.sample_rate) {
            case 0x800:
                samplerate = 8000;
                break;
            case 0x400:
                samplerate = 11025;
                break;
            case 0x200:
                samplerate = 12000;
                break;
            case 0x100:
                samplerate = 16000;
                break;
            case 0x80:
                samplerate = 22050;
                break;
            case 0x40:
                samplerate = 24000;
                break;
            case 0x20:
                samplerate = 32000;
                break;
            case 0x10:
                samplerate = 44100;
                break;
            case 0x8:
                samplerate = 48000;
                break;
            default:
                STRM_A2DP_LOG_E("sample rate info error/r/n", 0);
        }
    }
    else if (source->param.n9_a2dp.codec_info.codec_cap.type ==  BT_A2DP_CODEC_VENDOR) {
        switch (source->param.n9_a2dp.codec_info.codec_cap.codec.vend.sample_rate) {
            case 0x20:
                samplerate = 44100;
                break;
            case 0x10:
                samplerate = 48000;
                break;
            case 0x08:
                samplerate = 88200;
                break;
            case 0x04:
                samplerate = 96000;
                break;
            case 0x02:
                samplerate = 176400;
                break;
            case 0x01:
                samplerate = 192000;
                break;
            default:
                STRM_A2DP_LOG_E("sample rate info error %d/r/n",1, source->param.n9_a2dp.codec_info.codec_cap.codec.aac.sample_rate);
            }
    }
    else {
        //not support codec type
    }
    return samplerate;
}

uint32_t a2dp_get_channel(SOURCE source)
{
    uint32_t channel = 0;
    if (source->param.n9_a2dp.codec_info.codec_cap.type == BT_A2DP_CODEC_SBC ) {
        switch (source->param.n9_a2dp.codec_info.codec_cap.codec.sbc.channel_mode) {
            case 8:
                channel = 1;
                break;
            case 4:
            case 2:
            case 1:
                channel = 2;
                break;
            default:
                STRM_A2DP_LOG_E("channel info error/r/n" , 0);
        }
    }
    else if (source->param.n9_a2dp.codec_info.codec_cap.type == BT_A2DP_CODEC_AAC ) {
        switch (source->param.n9_a2dp.codec_info.codec_cap.codec.aac.channels) {
            case 0x2:
                channel = 1;
                break;
            case 0x1:
                channel = 2;
                break;
            default:
                STRM_A2DP_LOG_E("channel info error/r/n", 0);
        }
    }
    else if (source->param.n9_a2dp.codec_info.codec_cap.type == BT_A2DP_CODEC_VENDOR ) {
        switch (source->param.n9_a2dp.codec_info.codec_cap.codec.vend.channels) {
            case 0x0:
                channel = 1;
                break;
            case 0x1:
            case 0x2:
                channel = 2;
                break;
            default:
                STRM_A2DP_LOG_E("channel info error %d/r/n", 1, source->param.n9_a2dp.codec_info.codec_cap.codec.vend.channels);
        }
    }
    else {
        //not support codec type
    }
    return channel;
}

static VOID vend_get_frame_information(SOURCE source, bt_codec_a2dp_vend_frame_header_t *vend_frame_info)
{
    AVM_SHARE_BUF_INFO *share_buff_info  = &(source->streamBuffer.AVMBufferInfo);
    U32 read_offset;
    read_offset = share_buff_info->MemBlkSize*share_buff_info->ReadIndex + sizeof(bt_codec_a2dp_rtpheader_t);
    memcpy(vend_frame_info,(U8*)(share_buff_info->StartAddr + read_offset),sizeof(bt_codec_a2dp_vend_frame_header_t));
}


static U16 vend_get_frame_size(SOURCE source)
{
    U16 frame_size;
    switch (source->streamBuffer.AVMBufferInfo.SampleRate) {
        case 44100:
        case 48000:
            frame_size = 128;
            break;
        case 88200:
        case 96000:
            frame_size = 256;
            break;
        case 176400:
        case 192000:
            frame_size = 512;
            break;
        default :
            frame_size = 0;
            break;
    }
    return frame_size;
}

#if 0
ATTR_TEXT_IN_IRAM_LEVEL_2 static U32 vend_frame_size_calc(bt_codec_a2dp_vend_frame_header_t *vend_frame_info)
{
    U32 frame_size;
    frame_size = (vend_frame_info->vend_byte2 & 0x7);
    frame_size = (frame_size<<6) + (vend_frame_info->vend_byte3>>2) + VEND_FRAME_HEADER;
    return frame_size;
}
#endif

#ifdef MTK_BT_A2DP_VENDOR_BC_ENABLE
void vend_bc_write_loop_callback(void)
{
    SOURCE source = p_vend_bc_exbuf_if->a2dp_source;
    N9_A2DP_PARAMETER *n9_a2dp_param    = &(p_vend_bc_exbuf_if->a2dp_source->param.n9_a2dp);
    bt_codec_a2dp_vend_frame_header_t vend_frame_info;
    AVM_SHARE_BUF_INFO *share_buff_info  = &(p_vend_bc_exbuf_if->a2dp_source->streamBuffer.AVMBufferInfo);
    while (a2dp_get_hdr_info(source) == RETURN_PASS){
        if(share_buff_info->ReadIndex == share_buff_info->WriteIndex){
            break;
        }
        vend_get_frame_information(source, &vend_frame_info);
        n9_a2dp_param->current_frame_size = (vend_frame_info.vend_byte2 & 0x7);
        n9_a2dp_param->current_frame_size = (n9_a2dp_param->current_frame_size<<6) + (vend_frame_info.vend_byte3>>2) + VEND_FRAME_HEADER;
        if (vend_bc_write(p_vend_bc_exbuf_if->p_handle, (void*)(share_buff_info->StartAddr + share_buff_info->ReadIndex*share_buff_info->MemBlkSize + sizeof(bt_codec_a2dp_hdr_type_t)), n9_a2dp_param->current_frame_size) != 0x7fff)
        {
            share_information_read_index_update(source);
            update_from_share_information(source);
            if(share_buff_info->ReadIndex == share_buff_info->WriteIndex){
                break;
            }
        }
        else
        {
            break;
        }
    }
}

#if 0
extern vend_bc_param_t bc_params;void config_vend_bc_params(hal_ccni_message_t msg, hal_ccni_message_t *ack){
    UNUSED(ack);
    switch (msg.ccni_message[0]&0xFFFF){
        case 1 :
                bc_params.num_frame_start = msg.ccni_message[1];
                break;
        case 2 :
                bc_params.num_frame_target = msg.ccni_message[1];
                break;
        case 3 :
                bc_params.num_frame_threshold_upper = msg.ccni_message[1];
                break;
        case 4 :
                bc_params.num_frame_threshold_lower = msg.ccni_message[1];
                break;
        default :
                STRM_A2DP_LOG_I("BC config failed: %d",1,msg.ccni_message[1]);
                break;
        }
}


static void s_set_vend_bc_params(vend_bc_param_t *params){
    params->num_observe               = 256;    /* Number of observe for average retained frames, Range is 1 - 256. */
    params->observe_interval_msec     = 1*1000; /* Observed interval for average retained frames */
    params->num_frame_start           = 64;     /* Num frame to start */
    params->num_frame_target          = 64;     /* Target frames. [ 375 frame/sec * 0.17 sec = 64 (round up 63.75) ] */
    params->num_frame_threshold_upper = 5;      /* Frame upper threshold, difference from target frames. */
    params->num_frame_threshold_lower = -20;    /* Frame lower threshold, difference from target frames. */
    params->burst_enable              = 0;      /* Enable burst. setting value is 0(disable) or other than 0(enable). */
    params->burst_msec                = 2000;   /* Set when burst is enabled. Range is 0 - 2000 msec, If exceeds the range, rounded to boundary value in the range. */
}
#endif
static vend_bc_param_t bc_params = {256,1000,60,60,5,-8,1,2000,0};

#endif


ATTR_TEXT_IN_IRAM_LEVEL_2 static U32 sbc_frame_size_calc(bt_codec_a2dp_sbc_frame_header_t *sbc_frame_info)
{
    U32 framesize;
    U16 blocks;
    U16 subbands;
    U16 bitpool;
    U16 channels;

    channels    = (sbc_frame_info->Byte1.bit_alloc.CHANNEL_MODE == SBC_MONO_CHANNEL) ? (U16)1:(U16)2;
    subbands    = (sbc_frame_info->Byte1.bit_alloc.SUBBANDS == 0) ? (U16)4:(U16)8;
    bitpool     = sbc_frame_info->Bitpool;
    blocks      = (sbc_frame_info->Byte1.bit_alloc.BLOCKS + 1) * (U16)4;

    switch(sbc_frame_info->Byte1.bit_alloc.CHANNEL_MODE)
    {
        case SBC_MONO_CHANNEL:
        case SBC_DUAL_CHANNEL:
            framesize = ((blocks * channels * bitpool) + 7)/8 + 4 + (4 * subbands * channels)/8;
            break;

        case SBC_STEREO_CHANNEL:
            framesize = ((blocks * bitpool) + 7)/8 + 4 + (4 * subbands * channels)/8;
            break;

        default:
            framesize  = ((subbands + (blocks * bitpool)) + 7)/8 + 4 + (4 * subbands * channels)/8;
            break;
    }
    return framesize;
}

static bool sbc_report_bitrate(SOURCE source)
{
    U16 blocks;
    U16 subbands;
    bt_codec_a2dp_sbc_frame_header_t sbc_frame_info;
    /* Get sbc frame information */
    sbc_get_frame_information(source, &sbc_frame_info);
    /* Calculating frame size */
    subbands    = (sbc_frame_info.Byte1.bit_alloc.SUBBANDS == 0) ? (U16)4:(U16)8;
    blocks      = (sbc_frame_info.Byte1.bit_alloc.BLOCKS + 1) * (U16)4;
    *(source->param.n9_a2dp.a2dp_bitrate_report.p_a2dp_bitrate_report) = 8*(sbc_frame_size_calc(&sbc_frame_info)*source->streamBuffer.ShareBufferInfo.sample_rate/(U32)(subbands*blocks));
    //STRM_A2DP_LOG_I("sbc bit rate %d", 1, *(source->param.n9_a2dp.a2dp_bitrate_report.p_a2dp_bitrate_report));
    return (sbc_frame_info.SyncWord == 0x9c);
}
void Au_DL_send_reinit_request(DSP_REINIT_CAUSE reinit_msg)
{
    hal_ccni_message_t msg;
    memset((void *)&msg, 0, sizeof(hal_ccni_message_t));
    msg.ccni_message[0] = MSG_DSP2MCU_BT_AUDIO_DL_REINIT_REQUEST << 16;
    msg.ccni_message[1] = reinit_msg;
    aud_msg_tx_handler(msg, 0, FALSE);
}

void Au_DL_send_alc_request(U32 alc_latency)
{
    hal_ccni_message_t msg;
    memset((void *)&msg, 0, sizeof(hal_ccni_message_t));
    msg.ccni_message[0] = MSG_DSP2MCU_BT_AUDIO_DL_ALC_REQUEST << 16;
    msg.ccni_message[1] = alc_latency;
    aud_msg_tx_handler(msg, 0, FALSE);
}


BOOL A2dp_Buffer_Notify(SOURCE source,U16 thd)
{
    AVM_SHARE_BUF_INFO *share_buff_info  = &(source->streamBuffer.AVMBufferInfo);
    uint32_t sink_buffering_time;
    uint32_t temp_read_index = share_buff_info->ReadIndex;
    uint32_t read_asi,receive_asi;
    if ((source != NULL) && (source->transform != NULL) && (source->transform->sink != NULL))
    {
#ifdef CFG_AUDIO_HARDWARE_ENABLE
        sink_buffering_time = afe_get_dl1_query_data_amount()/(source->transform->sink->param.audio.src_rate/1000);
#endif
        *(U32*)source->param.n9_a2dp.readOffset = (source->transform->sink->param.audio.irq_exist) ? sink_buffering_time : (share_buff_info->SinkLatency/1000);
    }
    else
    {
        STRM_A2DP_LOG_E("[A2dp_Buffer_Notify] transform or sink is null", 0);
        return FALSE;
    }
    while (((bt_codec_a2dp_hdr_type_ptr)(share_buff_info->StartAddr + ((temp_read_index+1)%share_buff_info->MemBlkNum)*share_buff_info->MemBlkSize))->pcb_state == PCB_STATE_USED)
    {
        temp_read_index = (temp_read_index+1)%share_buff_info->MemBlkNum;
        if (temp_read_index ==  share_buff_info->ReadIndex)
        {
            ((AVM_SHARE_BUF_INFO *)source->param.n9_a2dp.share_info_base_addr)->NotifyCount = sink_buffering_time;
            return TRUE;
        }
    }
    read_asi    = ((bt_codec_a2dp_hdr_type_ptr)(share_buff_info->StartAddr + share_buff_info->ReadIndex*share_buff_info->MemBlkSize))->frame_asi;
    receive_asi = ((bt_codec_a2dp_hdr_type_ptr)(share_buff_info->StartAddr + temp_read_index*share_buff_info->MemBlkSize))->frame_asi;

    ((AVM_SHARE_BUF_INFO *)source->param.n9_a2dp.share_info_base_addr)->reserved[0] = read_asi - ((sink_buffering_time*(share_buff_info->SampleRate/1000))&0xFFFFFF00);
    #ifdef CFG_AUDIO_HARDWARE_ENABLE
    sink_buffering_time = afe_get_dl1_query_data_amount()/(source->transform->sink->param.audio.src_rate/1000) + ((receive_asi - read_asi)/(share_buff_info->SampleRate/1000));
    #endif /* CFG_AUDIO_HARDWARE_ENABLE */
    ((AVM_SHARE_BUF_INFO *)source->param.n9_a2dp.share_info_base_addr)->NotifyCount = sink_buffering_time;

    //STRM_A2DP_LOG_I("[A2dp_Buffer_Notify] dbg  %d %d %d ",3 ,read_asi,receive_asi,sink_buffering_time,(source->transform->sink->param.audio.rate/1000));
    return (sink_buffering_time < thd);
}
ATTR_TEXT_IN_IRAM_LEVEL_1 BOOL SourceReadBuf_N9_a2dp(SOURCE source, U8 *dst_addr, U32 length)
{
    STRM_A2DP_LOG_D("SourceReadBuf_N9_a2dp++\r\n", 0);

    AVM_SHARE_BUF_INFO *share_buff_info  = &(source->streamBuffer.AVMBufferInfo);

#ifdef MTK_BT_A2DP_VENDOR_BC_ENABLE
    N9_A2DP_PARAMETER *n9_a2dp_param    = &(source->param.n9_a2dp);
    if ( n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_VENDOR )
    {
        if (source->param.n9_a2dp.sink_latency != 0)
        {
           memcpy(dst_addr,(U8*)(share_buff_info->StartAddr + share_buff_info->ReadIndex*share_buff_info->MemBlkSize + sizeof(bt_codec_a2dp_hdr_type_t)),length);
        }
        else 
        {
            vend_bc_read(p_vend_bc_exbuf_if->p_handle, dst_addr, length);
            if (length >  VEND_FRAME_BUF_SIZE)
            {
                STRM_A2DP_LOG_I("Vend codec meet abnormal frame size", 1, length);
            }
        }
    }
    else
#endif
    {
        memcpy(dst_addr,(U8*)(share_buff_info->StartAddr + share_buff_info->ReadIndex*share_buff_info->MemBlkSize + sizeof(bt_codec_a2dp_hdr_type_t)),length);
    }
    return TRUE;
}



ATTR_TEXT_IN_IRAM_LEVEL_1 U32 SourceSize_N9_a2dp(SOURCE source)
{
    STRM_A2DP_LOG_D("SourceSize_N9_a2dp++\r\n", 0);

#ifdef A2DP_IDS_DEBUG_EN
    gpio_init_and_set_low();
#endif

    U32 frame_size = 0;
    ltcs_bt_type_t codec_type = LTCS_TYPE_OTHERS;
    U8 pcb_parse_state = RETURN_FAIL;
    AVM_SHARE_BUF_INFO *share_buff_info  = &(source->streamBuffer.AVMBufferInfo);
    N9_A2DP_PARAMETER *n9_a2dp_param    = &(source->param.n9_a2dp);
    #ifdef AIR_BT_CLK_SKEW_ENABLE
    U32 mask;
    ltcs_anchor_info_t * pcdc_info = (ltcs_anchor_info_t *)source->param.n9_a2dp.pcdc_info_buf;
    #endif

    UNUSED(codec_type);

    /* update share information data */
    update_from_share_information(source);
#ifdef MTK_BT_A2DP_VENDOR_BC_ENABLE
    if ((n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_VENDOR) && (source->param.n9_a2dp.sink_latency == 0)){
        vend_bc_write_loop_callback();
        if (source->param.n9_a2dp.DspReportStartId != 0xFFFF)
        {
            SourceDrop_N9_a2dp(source,0);
        }
        if ((source->transform != NULL)&&(SinkSlack(source->transform->sink) >= share_buff_info->FrameSampleNum*(source->transform->sink->param.audio.format_bytes))){
            A2dp_Buffer_Notify(source,0);
            return vend_bc_read(p_vend_bc_exbuf_if->p_handle, NULL, 0);
        }
        else {
            return 0;
        }
    }
#endif

#ifdef AIR_BT_CLK_SKEW_ENABLE/* update pcdc information data */
    hal_nvic_save_and_set_interrupt_mask(&mask);
    lt_clk_skew_set_drift_value(pcdc_info->drift_comp_val); // update drift value
    lt_clk_skew_update_base_asi(pcdc_info->asi_base, pcdc_info->asi_cur); // update anchor base asi
    hal_nvic_restore_interrupt_mask(mask);
#endif

#ifdef MTK_PEQ_ENABLE
    PEQ_Update_Info(pcdc_info->anchor_clk, n9_a2dp_param->predict_asi);
#endif
    /* Check there is underflow or not */
    if ((source->transform != NULL) &&(DSP_Callback_BypassModeGet(source,source->transform->sink)==BYPASS_CODEC_MODE && n9_a2dp_param->mce_flag == TRUE)){
        return n9_a2dp_param->current_frame_size;
    }
    A2dp_Buffer_Notify(source,0);

    pcb_parse_state = a2dp_get_hdr_info(source);

    BOOL enterbcm = FALSE;
    if ( pcb_parse_state == RETURN_FAIL ) {
#ifdef CFG_AUDIO_HARDWARE_ENABLE
         if (((source->transform != NULL) && (n9_a2dp_param->mce_flag == TRUE) && (source->transform->sink->type == SINK_TYPE_AUDIO) )
            && (afe_get_dl1_query_data_amount() <= (((source->param.n9_a2dp.latency_monitor == true) ? BUFFER_WAIT_PLR_ISR_NUM_GAMING_MODE : \
            (source->param.n9_a2dp.alc_monitor) ? BUFFER_WAIT_PLR_ISR_NUM : BUFFER_WAIT_PLR_ISR_NUM_ALC)*(source->transform->sink->param.audio.rate * source->transform->sink->param.audio.period / 1000))) && (afe_get_bt_sync_monitor(AUDIO_DIGITAL_BLOCK_MEM_DL1) !=0)){
            if (share_buff_info->ReadIndex == share_buff_info->WriteIndex)
            {
                if (((AVM_SHARE_BUF_INFO *)source->param.n9_a2dp.share_info_base_addr)->ForwarderAddr < 8192)
                {
                    ((AVM_SHARE_BUF_INFO *)source->param.n9_a2dp.share_info_base_addr)->ForwarderAddr += BUFFER_EMPTY_ASI_PADDING_NUM;
                }
#ifdef A2DP_IDS_DEBUG_EN
                    gpio_revert(1);
#endif
                STRM_A2DP_LOG_I("enter BCM buffer empty , pt:%d  t:%d ro: %d padding :%d", 4, n9_a2dp_param->predict_asi, n9_a2dp_param->current_asi,share_buff_info->ReadIndex,((AVM_SHARE_BUF_INFO *)source->param.n9_a2dp.share_info_base_addr)->ForwarderAddr);
                if(n9_a2dp_param->buffer_empty_cnt < REINIT_FLAG){
                    n9_a2dp_param->buffer_empty_cnt++;
                }
            }
            else
            {
                STRM_A2DP_LOG_I("enter BCM PCM data not enough", 0);
            }
            enterbcm = TRUE;
         }
         else
         {
             if (source->param.n9_a2dp.DspReportStartId != 0xFFFF)
             {
                 SourceDrop_N9_a2dp(source,0);
             }
             return frame_size;
         }
#endif
    }
    /* Check there is data in share buffer or not */
    else{
        frame_size = n9_a2dp_param->current_frame_size;
        n9_a2dp_param->predict_asi = n9_a2dp_param->predict_asi & ASI_MASK;
        if (( n9_a2dp_param->predict_asi == n9_a2dp_param->current_asi )||(!((n9_a2dp_param->mce_flag == TRUE) && (source->transform->sink->type == SINK_TYPE_AUDIO) ))) {
                //do nothing
            }
            else if ( (S32)(n9_a2dp_param->predict_asi - n9_a2dp_param->current_asi) < 0 ) {
                //packet lost
#ifdef A2DP_IDS_DEBUG_EN
                gpio_revert(4);
#endif
                STRM_A2DP_LOG_I("enter BCM packet loss, pasi:%d Jump to asi:%d (seqn:%d) Wo:%d Ro:%d", 5, n9_a2dp_param->predict_asi, n9_a2dp_param->current_asi,n9_a2dp_param->current_seq_num,share_buff_info->WriteIndex,share_buff_info->ReadIndex);
                if(n9_a2dp_param->buffer_empty_cnt < REINIT_FLAG){
#ifdef CFG_AUDIO_HARDWARE_ENABLE
                    if (afe_get_bt_sync_monitor(AUDIO_DIGITAL_BLOCK_MEM_DL1) !=0)
                    {
                        n9_a2dp_param->buffer_empty_cnt++;
                    }
#endif
                }
                enterbcm = TRUE;
            }
            else{
                a2dp_skip_expired_packet(source);
                if ( n9_a2dp_param->predict_asi == n9_a2dp_param->current_asi ) {
                        //do nothing
                }
                else {
#ifdef A2DP_IDS_DEBUG_EN
                gpio_revert(3);
#endif
                    STRM_A2DP_LOG_I("enter BCM packet loss(drop past) pt:%d  t:%d seqn:%d", 3, n9_a2dp_param->predict_asi, n9_a2dp_param->current_asi,n9_a2dp_param->current_seq_num);
                    if(n9_a2dp_param->buffer_empty_cnt < REINIT_FLAG){
#ifdef CFG_AUDIO_HARDWARE_ENABLE
                        if (afe_get_bt_sync_monitor(AUDIO_DIGITAL_BLOCK_MEM_DL1) !=0)
                        {
                            n9_a2dp_param->buffer_empty_cnt++;
                        }
#endif
                    }
                    enterbcm = TRUE;
                }
            }

    }
    if (enterbcm){
        DSP_Callback_BypassModeCtrl(source,source->transform->sink,BYPASS_CODEC_MODE);
        if ( n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_AAC) {
            frame_size = AAC_FRAME_DECODE_SAMPLE_NUM;//sample
        }
        else if (n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_SBC){
            frame_size = SBC_FRAME_DECODE_SAMPLE_NUM;
        }
        else if (n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_VENDOR){
            frame_size = vend_get_frame_size(source);//sample
        }
        n9_a2dp_param->current_frame_size = frame_size;
        //configASSERT(0);
        return frame_size;
    }
    if (n9_a2dp_param->pkt_lost_report_state == PL_STATE_REPORTPREVSEQN)
    {
        U32 gpt_timer;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &gpt_timer);
#ifdef A2DP_DIS_DEBUG_EN
        gpio_revert(2);
#endif
        STRM_A2DP_LOG_I("A2DP meet packet loss state,Last PKT SEQN:%d timer: %d", 2, n9_a2dp_param->current_seq_num,gpt_timer);
        n9_a2dp_param->pkt_lost_report_state = PL_STATE_REPORTNEXTSEQN;
    }
    /*Header check for SBC & Vendor decoder */
    if ( n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_SBC ) {
        bt_codec_a2dp_sbc_frame_header_t sbc_frame_info;
        /* Get sbc frame information */
        sbc_get_frame_information(source, &sbc_frame_info);
        if ( sbc_frame_info.SyncWord != 0x9C )
        {
             STRM_A2DP_LOG_I("A2DP meet wrong sync word,Ro : %d Syncword : %d", 2, share_buff_info->ReadIndex,sbc_frame_info.SyncWord);
             SourceDrop(source,n9_a2dp_param->current_frame_size);
             n9_a2dp_param->predict_asi -= share_buff_info->FrameSampleNum;
             return SourceSize_N9_a2dp(source);
        }
    }
    else if ( n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_VENDOR )
    {
        bt_codec_a2dp_vend_frame_header_t vend_frame_info;
        vend_get_frame_information(source, &vend_frame_info);
        if ( vend_frame_info.vend_byte1 != 0xAA )
        {
            STRM_A2DP_LOG_I("A2DP meet vend wrong sync word,Ro : %d Syncword : %d frame size: %d", 2, share_buff_info->ReadIndex,vend_frame_info.vend_byte1,n9_a2dp_param->current_frame_size);
            if (n9_a2dp_param->current_frame_size != 0)
            {
            SourceDrop(source,n9_a2dp_param->current_frame_size);
            n9_a2dp_param->predict_asi -= share_buff_info->FrameSampleNum;
            }
            else
            {
                share_information_read_index_update(source);
            }
            return SourceSize_N9_a2dp(source);
        }
    }

    return frame_size;
}

U8* SourceMap_N9_a2dp(SOURCE source)
{
    UNUSED(source);
    return NULL;
}

ATTR_TEXT_IN_IRAM_LEVEL_1 VOID SourceDrop_N9_a2dp (SOURCE source, U32 amount)
{

    AVM_SHARE_BUF_INFO *share_buff_info  = &(source->streamBuffer.AVMBufferInfo);
    N9_A2DP_PARAMETER *n9_a2dp_param    = &(source->param.n9_a2dp);
    U16 reinit_thd = (n9_a2dp_param->codec_info.codec_cap.type != BT_A2DP_CODEC_AAC) ? BUFFER_EMPTY_REINIT_THD<<3 : BUFFER_EMPTY_REINIT_THD;
    BOOL buffer_notify;
    U16 dsp_notify_thd;
    if(share_buff_info->SampleRate>48000)
    {
        reinit_thd = reinit_thd*(share_buff_info->SampleRate/48000);
    }
    if (source->param.n9_a2dp.DspReportStartId != 0xFFFF)
    {
        U32 gpt_timer;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &gpt_timer);
        //aud_msg_ack(source->param.n9_a2dp.DspReportStartId, FALSE);
        STRM_A2DP_LOG_I("[Measure DSP Callback Streaming]First decode done, Ack A2DP start ID :%d Time :%d", 2, source->param.n9_a2dp.DspReportStartId,gpt_timer);
        source->param.n9_a2dp.DspReportStartId = 0xFFFF;
        n9_a2dp_param->buffer_empty_cnt = 0;
        //n9_a2dp_param->mce_flag = FALSE;
    }
    if(amount == 0) {
        return;
    }
    if ((n9_a2dp_param->buffer_empty_cnt > (reinit_thd*(!source->param.n9_a2dp.alc_monitor)))
        &&(n9_a2dp_param->buffer_empty_cnt < REINIT_FLAG)){
#ifdef CFG_AUDIO_HARDWARE_ENABLE
        U32 read_reg = afe_get_bt_sync_monitor(AUDIO_DIGITAL_BLOCK_MEM_DL1);
        if (read_reg != 0){
            //afe_set_hardware_digital_gain(AFE_HW_DIGITAL_GAIN1, 0);
            STRM_A2DP_LOG_I("Buffer status trigger re-sync empty_cnt:%d",1, n9_a2dp_param->buffer_empty_cnt);
#if (ADATIVE_LATENCY_CTRL)
            if (source->param.n9_a2dp.alc_monitor)
            {
                STRM_A2DP_LOG_I("ALC request Latency : %d", 1, ADATIVE_LATENCY);
                Au_DL_send_alc_request(ADATIVE_LATENCY);
            }
            else
            {   //Gaming mode no need reinit
                if(source->param.n9_a2dp.latency_monitor == false){
                    if((n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_SBC) && (n9_a2dp_param->codec_info.codec_cap.codec.sbc.max_bit_pool == 37)) {
                        STRM_A2DP_LOG_I("by pass re-sync due to codec type is sbc and max_bit_pool is 37", 0);
                    } else {
                        Au_DL_send_reinit_request(MSG2_DSP2CN4_REINIT_BUF_ABNORMAL);
                    }
                }
            }
#else
            //Gaming mode no need reinit
            if(source->param.n9_a2dp.latency_monitor == false){
                    if((n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_SBC) && (n9_a2dp_param->codec_info.codec_cap.codec.sbc.max_bit_pool == 37)) {
                        STRM_A2DP_LOG_I("by pass re-sync due to codec type is sbc and max_bit_pool is 37", 0);
                    } else {
                        Au_DL_send_reinit_request(MSG2_DSP2CN4_REINIT_BUF_ABNORMAL);
                    }
            }
#endif
            n9_a2dp_param->buffer_empty_cnt = REINIT_FLAG;
        }
        else
#endif
        {
            n9_a2dp_param->buffer_empty_cnt = 0;
        }
    }else if (n9_a2dp_param->buffer_empty_cnt >= REINIT_FLAG)
    {
        n9_a2dp_param->buffer_empty_cnt = (n9_a2dp_param->buffer_empty_cnt > (REINIT_FLAG + (reinit_thd*RESEND_REINIT_CNT))) ? 0 : n9_a2dp_param->buffer_empty_cnt + 1;
    }
    n9_a2dp_param->predict_asi += share_buff_info->FrameSampleNum;
    if ((source->transform != NULL)&&(DSP_Callback_BypassModeGet(source,source->transform->sink)==BYPASS_CODEC_MODE)) {
        DSP_Callback_BypassModeCtrl(source,source->transform->sink,STREAMING_MODE);
        return;
    }
    if((n9_a2dp_param->buffer_empty_cnt < REINIT_FLAG)&&(source->transform != NULL)){
        if ((source->param.n9_a2dp.alc_monitor == FALSE)&&(source->param.n9_a2dp.sink_latency != 0))
        {
            dsp_notify_thd = ((source->param.n9_a2dp.sink_latency/1000)  > (NOTIFY_LOW_BOND + NOTIFY_LOW_THD)) ? ((source->param.n9_a2dp.sink_latency/1000) - NOTIFY_LOW_THD) : NOTIFY_LOW_BOND;
            //set a lower notify thd for gaming mode, no need resync, for debug logging only
            if(source->param.n9_a2dp.latency_monitor) {
                dsp_notify_thd = NOTIFY_LOW_BOND_GAMING_MODE;
            }
            buffer_notify =  A2dp_Buffer_Notify(source,dsp_notify_thd);
            n9_a2dp_param->buffer_empty_cnt = (buffer_notify) ? n9_a2dp_param->buffer_empty_cnt + 1 : (n9_a2dp_param->buffer_empty_cnt != 0) ? n9_a2dp_param->buffer_empty_cnt - 1 : 0;
            if(n9_a2dp_param->buffer_empty_cnt > (reinit_thd>>1))
            {
                STRM_A2DP_LOG_I("Buffer low cnt:%d ro:%d wo:%d seqn:%d thd:%d latency:%d", 6, n9_a2dp_param->buffer_empty_cnt, share_buff_info->ReadIndex, share_buff_info->WriteIndex,n9_a2dp_param->current_seq_num,dsp_notify_thd,((AVM_SHARE_BUF_INFO *)source->param.n9_a2dp.share_info_base_addr)->NotifyCount);
            }
        }
    }

    /* SBC part */
    if ( n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_SBC ) {
            if ( n9_a2dp_param->current_frame_size == amount ){
                if (n9_a2dp_param->a2dp_bitrate_report.a2dp_report_cnt == 0)
                {
                    n9_a2dp_param->a2dp_bitrate_report.a2dp_report_cnt = sbc_report_bitrate(source);
                }
                share_information_read_index_update(source);
        }
    }
    /* AAC part */
    else if ( n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_AAC ) {
        //get packet loss status
        if ( n9_a2dp_param->current_frame_size == amount ){
            if (n9_a2dp_param->a2dp_bitrate_report.a2dp_report_cnt >= AAC_BITRATE_REPORT_ACCUM_TIME){
                *(n9_a2dp_param->a2dp_bitrate_report.p_a2dp_bitrate_report) = (U32)(n9_a2dp_param->a2dp_bitrate_report.a2dp_accumulate_cnt)*a2dp_get_samplingrate(source)/(U32)(AAC_FRAME_DECODE_SAMPLE_NUM*n9_a2dp_param->a2dp_bitrate_report.a2dp_report_cnt/8);
                n9_a2dp_param->a2dp_bitrate_report.a2dp_report_cnt = 0;
                n9_a2dp_param->a2dp_bitrate_report.a2dp_accumulate_cnt = 0;
            }
            else{
                n9_a2dp_param->a2dp_bitrate_report.a2dp_report_cnt++;
                n9_a2dp_param->a2dp_bitrate_report.a2dp_accumulate_cnt += amount;
            }
            share_information_read_index_update(source);
        }
        else {
            //TBD
        }
    }
    /* VENDOR part */
    else if ( n9_a2dp_param->codec_info.codec_cap.type == BT_A2DP_CODEC_VENDOR ) {
#ifdef MTK_BT_A2DP_VENDOR_BC_ENABLE
        if ((source->param.n9_a2dp.sink_latency != 0) && ( n9_a2dp_param->current_frame_size == amount ))
#else
        if ( n9_a2dp_param->current_frame_size == amount )
#endif
        {
            share_information_read_index_update(source);
        }
        else {
            //TBD
        }
    }
    else {
        //Not support codec type
    }
}

BOOL SourceConfigure_N9_a2dp(SOURCE source, stream_config_type type, U32 value)
{
    UNUSED(source);
    UNUSED(type);
    UNUSED(value);
    return TRUE;
}

BOOL SourceClose_N9_a2dp(SOURCE source)
{
    ((SHARE_BUFFER_INFO *)source->param.n9_a2dp.share_info_base_addr)->notify_count = 0;
    #ifdef MTK_BT_A2DP_VENDOR_BC_ENABLE
    DSPMEM_Free(source->taskId, (VOID*)source);
    #endif
    return TRUE;
}

VOID SourceInit_N9_a2dp(SOURCE source)
{
    /* buffer init */
#ifdef MTK_BT_A2DP_VENDOR_BC_ENABLE
    if (( source->param.n9_a2dp.codec_info.codec_cap.type == BT_A2DP_CODEC_VENDOR ) && (source->param.n9_a2dp.sink_latency == 0))
    {
        p_vend_bc_exbuf_if = DSPMEM_tmalloc(source->taskId, sizeof(vend_bc_extern_buf_v2_t), (VOID*)source);
        p_vend_bc_exbuf_if->a2dp_source = source;
        //s_set_vend_bc_params(&(p_vend_bc_exbuf_if->param));
        p_vend_bc_exbuf_if->param = bc_params;
        U8* vend_bc_buf;
        STRM_A2DP_LOG_I("Vendor BC request buffer size : %d %d", 2,VEND_BC_HANDLE_SIZE,source->streamBuffer.AVMBufferInfo.MemBlkNum*source->streamBuffer.AVMBufferInfo.MemBlkSize);
        vend_bc_buf = reshape_share_buffer_for_vend_bc(source,(source->streamBuffer.AVMBufferInfo.MemBlkNum*source->streamBuffer.AVMBufferInfo.MemBlkSize+3)&0xFFFC);
        p_vend_bc_exbuf_if->p_handle = vend_bc_open(vend_bc_buf, VEND_BC_HANDLE_SIZE, &p_vend_bc_exbuf_if->param);
        vend_bc_write_loop_callback();
    }
#endif
    //source->type = SOURCE_TYPE_N9_A2DP;
    source->buftype = BUFFER_TYPE_CIRCULAR_BUFFER;

#if (MEDIATEK_PKT_FAKE_VERIFY)
    if (source->param.n9_a2dp.codec_info.codec_cap.type == BT_A2DP_CODEC_SBC) {
        source->param.n9_a2dp.codec_info.codec_cap.type = BT_A2DP_CODEC_AIRO_CELT;
    }
#endif

    /* interface init */
    if (source->param.n9_a2dp.codec_info.codec_cap.type == BT_A2DP_CODEC_AIRO_CELT) {
#ifdef MTK_BT_A2DP_AIRO_CELT_ENABLE
        /* Test Code */
        source->sif.SourceSize        = SourceSize_Mediatek_A2DP;
        source->sif.SourceMap         = SourceMap_Mediatek_A2DP;
        source->sif.SourceConfigure   = SourceConfigure_Mediatek_A2DP;
        source->sif.SourceDrop        = SourceDrop_Mediatek_A2DP;
        source->sif.SourceClose       = SourceClose_Mediatek_A2DP;
        source->sif.SourceReadBuf     = SourceReadBuf_Mediatek_A2DP;

#if (MEDIATEK_PKT_FAKE_VERIFY)
        source->streamBuffer.AVMBufferInfo.StartAddr = &AiroFakeShareData[0];
        AiroFakeDatabaseIdx = 0;
        gA2dpGamingCtrl.MinAfeSinkSize = MEDIATEK_GAMING_FRAME_SIZE*10;
#else
        gA2dpGamingCtrl.MinAfeSinkSize = MEDIATEK_GAMING_FRAME_SIZE;
#endif

        gA2dpGamingCtrl.AccumulatedSamples = 0;
        gA2dpGamingCtrl.CntIdx = 0;

        gA2dpGamingCtrl.Initialized = FALSE;
        gA2dpGamingCtrl.DebugSeqNo = 0;

#else
        STRM_A2DP_LOG_I(" Codec Type:%d not supported", 1, source->param.n9_a2dp.codec_info.codec_cap.type);
        configASSERT(0);
#endif
    } else {
        source->sif.SourceSize        = SourceSize_N9_a2dp;
        source->sif.SourceMap         = SourceMap_N9_a2dp;
        source->sif.SourceConfigure   = SourceConfigure_N9_a2dp;
        source->sif.SourceDrop        = SourceDrop_N9_a2dp;
        source->sif.SourceClose       = SourceClose_N9_a2dp;
        source->sif.SourceReadBuf     = SourceReadBuf_N9_a2dp;
    }

}




