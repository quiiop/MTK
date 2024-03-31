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

#include "bt_sink_srv_a2dp.h"
#include "bt_sink_srv_avrcp.h"
#include "bt_sink_srv_music.h"
//#include "bt_sink_srv_action.h"
#include "bt_sink_srv_ami.h"
#include "bt_gap.h"
#include "bt_sdp.h"
#include "bt_di.h"
#include "bt_linknode.h"
#include "bt_hci_spec.h"
#include "hal_audio.h"
#include "hal_audio_message_struct.h"
#include "bt_sink_srv_state_notify.h"
#include "bt_sink_srv_utils.h"
#include "bt_codec.h"
#include "avm_direct.h"
#include "bt_avm.h"
#include "bt_sink_srv_ami.h"
#include "bt_device_manager.h"
#include "nvkey.h"
#include "nvkey_id_list.h"
#include "bt_device_manager_internal.h"
#include "bt_gap.h"
#ifdef MTK_BT_AUDIO_PR
#include "apps_debug.h"
#endif /* MTK_BT_AUDIO_PR */

//#define MTK_BT_A2DP_FPGA_IT
#define BT_AUDIO_PLAY_EN_EXPIRED_SUPPORT

/* currently care sink role */
#define SBC_CODEC_NUM (0x01)

#define BT_SINK_SRV_A2DP_LATER_JOIN_REINIT_PARAMETER (0X03)

#define BT_SINK_SRV_A2DP_CODEC_MAX_NUM (SBC_CODEC_NUM)

bt_sink_srv_a2dp_pseudo_handle_t g_a2dp_pse_hd[BT_SINK_SRV_A2DP_PSEUDO_COUNT + BT_SINK_SRV_MCE_A2DP_PSEUDO_COUNT] = {{0}};

bt_a2dp_codec_capability_t g_bt_sink_srv_a2dp_codec_list[BT_SINK_SRV_A2DP_CODEC_MAX_NUM];

static bool g_bt_sink_srv_a2dp_init_complete = false;

bt_a2dp_streaming_received_ind_t *bt_sink_srv_a2dp_rece_ind = NULL;

uint32_t g_a2dp_gpt_codec_run_count_begin = 0;
uint32_t g_a2dp_gpt_codec_run_count_end = 0;

static bt_codec_sbc_t sbc_cap[1] = {
    {
        8,  // min_bit_pool
        75,  // max_bit_pool
        0xf, // block_len: all
        0xf, // subband_num: all
        0x3, // both snr/loudness
        0x3, // MTK_David: change from 0xf to 0x3. (Only support 44.1 and 48K)
        0xf  // channel_mode: all
    }
};

extern void bt_sink_srv_music_a2dp_ami_hdr(bt_sink_srv_am_id_t aud_id, bt_sink_srv_am_cb_msg_class_t msg_id, bt_sink_srv_am_cb_sub_msg_t sub_msg, void *param);
extern void bt_sink_srv_music_a2dp_common_ami_hdr(bt_sink_srv_am_id_t aud_id, bt_sink_srv_am_cb_msg_class_t msg_id, bt_sink_srv_am_cb_sub_msg_t sub_msg, void *param);
extern n9_dsp_share_info_t *hal_audio_query_bt_audio_dl_share_info(void);

audio_src_srv_handle_t *bt_sink_srv_a2dp_alloc_pseudo_handle(void);

void bt_sink_srv_a2dp_free_pseudo_handle(audio_src_srv_handle_t *hd);

static bool g_a2dp_codec_aac_support = true;
void bt_sink_srv_a2dp_reinitial_sync(void);
static int32_t bt_sink_srv_a2dp_handle_start_streaming_ind(bt_a2dp_start_streaming_ind_t *start_ind, bool is_cnf);
static int32_t bt_sink_srv_a2dp_handle_suspend_streaming_ind(bt_a2dp_suspend_streaming_ind_t *suspend_ind, bool need_response);

//MTK_Titan: Add static prototype
static void _bt_sink_srv_a2dp_clear_media_nodes(bt_sink_srv_music_data_list_t *pList);

void bt_sink_srv_switch_aac_codec(bool enable)
{
    bt_sink_srv_report_id("[sink][a2dp] aac switch, origin: %d, current: %d", 2,
                          g_a2dp_codec_aac_support, enable);
    g_a2dp_codec_aac_support = enable;
}

bt_status_t bt_sink_srv_a2dp_get_init_params(bt_a2dp_init_params_t *param)
{
    int32_t ret = BT_STATUS_FAIL;
    uint32_t num = 0;

    if (param) {
        /* init sink sep */
        bt_sink_srv_a2dp_sbc_config_parameter_t sbc_config = {
            .min_bitpool = 8,
            .max_bitpool = 75,
        };
        uint32_t read_size = sizeof(sbc_config);
        /* MTK_David TODO:May get from NVDM or set by cli command */
        sbc_cap[0].min_bit_pool = sbc_config.min_bitpool;
        sbc_cap[0].max_bit_pool = sbc_config.max_bitpool;
        bt_sink_srv_report_id("[sink][a2dp]sbc,bitpool (max~min):0x%02x~0x%02x, read_size:%d", 4,
                              sbc_config.max_bitpool, sbc_config.min_bitpool, read_size);
        BT_A2DP_MAKE_SBC_CODEC(g_bt_sink_srv_a2dp_codec_list + num, BT_A2DP_SINK,
                               sbc_cap[0].min_bit_pool, sbc_cap[0].max_bit_pool,
                               sbc_cap[0].block_length, sbc_cap[0].subband_num,
                               sbc_cap[0].alloc_method, sbc_cap[0].sample_rate,
                               sbc_cap[0].channel_mode);

        num++;

        param->sink_feature = 0x0F;
        param->source_feature = 0x00;
        /* init source sep */

        /* fill init params */
        param->codec_number = num;
        param->codec_list = g_bt_sink_srv_a2dp_codec_list;
#ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__
        param->sink_delay = BT_SINK_SRV_A2DP_DELAY;
#endif /* #ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__ */

        ret = BT_STATUS_SUCCESS;
        //(reduce log)bt_sink_srv_report_id("[sink][a2dp]init-ret: %d, num:%d\n", 2, ret, param->codec_number);
    }
    g_bt_sink_srv_a2dp_init_complete = true;

    return ret;
}

static void bt_sink_srv_a2dp_init(void)
{
    bt_sink_srv_music_context_t *ctx = NULL;

    ctx = bt_sink_srv_music_get_context();

    if (ctx->a2dp_aid != BT_SINK_SRV_INVALID_AID) {
        bt_sink_srv_ami_audio_close(ctx->a2dp_aid);
        ctx->a2dp_aid = BT_SINK_SRV_INVALID_AID;
    }

    ctx->a2dp_aid = bt_sink_srv_ami_audio_open(AUD_MIDDLE, bt_sink_srv_music_a2dp_common_ami_hdr);
    bt_sink_srv_report_id("[sink][a2dp]init-a2dp_aid: %d", 1, ctx->a2dp_aid);

    bt_sink_srv_music_set_sink_latency(BT_SINK_SRV_A2DP_DEFAULT_SINK_LATENCY, true);

#if 0 //MTK_COMMON: David - reduce log
#if defined(__GNUC__)
    bt_sink_srv_report("[sink][a2dp]--Version: %s", "__GNUC__");
#endif /* #if defined(__GNUC__) */

#if defined(__ARMCC_VERSION)
    bt_sink_srv_report("[sink][a2dp]--Version: %s", "__ARMCC_VERSION");
#endif /* #if defined(__ARMCC_VERSION) */

#if defined(__ICCARM__)
    bt_sink_srv_report("[sink][a2dp]--Version: %s", "__ICCARM__");
#endif /* #if defined(__ICCARM__) */
#endif
}

void bt_sink_srv_a2dp_ami_hdr(bt_sink_srv_am_id_t aud_id, bt_sink_srv_am_cb_msg_class_t msg_id, bt_sink_srv_am_cb_sub_msg_t sub_msg, void *param)
{
    bt_sink_srv_music_context_t *ctx = NULL;
    int32_t ret = 0;
    int32_t err_code = 0;
    bt_sink_srv_music_device_t *run_dev = NULL;

    ctx = bt_sink_srv_music_get_context();
    run_dev = ctx->run_dev;
    //bt_sink_srv_assert(run_dev);
    if ((ctx->a2dp_aid == aud_id) &&
        (msg_id == AUD_A2DP_PROC_IND) &&
        (sub_msg == AUD_STREAM_EVENT_DATA_REQ)) {
        // drop
        ;
    } else {
        bt_sink_srv_report_id("[sink][a2dp]ami_hdr[s]-aid: %d, aud_id: %d, msg_id: %d, sub_msg: %d, 2nd: 0x%x", 5,
                              ctx->a2dp_aid, aud_id, msg_id, sub_msg, sub_msg);
    }
    if (ctx->a2dp_aid == aud_id) {
        switch (msg_id) {
            case AUD_SELF_CMD_REQ: {
                    if (AUD_CMD_FAILURE == sub_msg) {
                    }
                    break;
                }

            case AUD_SINK_OPEN_CODEC: {
                    if ((run_dev) && (run_dev->flag & BT_SINK_SRV_MUSIC_FLAG_WAIT_AMI_OPEN_CODEC)) {
                        /* Save codec handle */
                        bt_sink_srv_memcpy(&(run_dev->med_handle), param, sizeof(bt_sink_srv_am_media_handle_t));

                        BT_SINK_SRV_REMOVE_FLAG(run_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_AMI_OPEN_CODEC);

                        /* Set codec open flag */
                        BT_SINK_SRV_SET_FLAG(run_dev->op, BT_SINK_SRV_MUSIC_OP_CODEC_OPEN);
                        BT_SINK_SRV_REMOVE_FLAG(run_dev->op, BT_SINK_SRV_MUSIC_OP_PLAY_IND); //MTK_Titan:added to control media input
                        _bt_sink_srv_a2dp_clear_media_nodes(&ctx->media_list);

                        if (run_dev->flag & BT_SINK_SRV_MUSIC_WAIT_SET_VOLUME) {
                            bt_sink_srv_ami_audio_set_volume(ctx->a2dp_aid, ctx->vol_lev, STREAM_OUT);
                            BT_SINK_SRV_REMOVE_FLAG(run_dev->flag, BT_SINK_SRV_MUSIC_WAIT_SET_VOLUME);
                        }

                        if (BT_SINK_SRV_MUSIC_TRANSIENT_STATE_CLEAR_CODEC == run_dev->handle->substate) {
                            if (run_dev->flag & BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC) {
                                BT_SINK_SRV_REMOVE_FLAG(run_dev->flag, BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC);
                            }

                            if (run_dev->flag & BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START) {
                                bt_sink_srv_report_id("[sink][a2dp] need to response a2dp start", 0);
                                BT_SINK_SRV_REMOVE_FLAG(run_dev->flag, BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START);
                                bt_a2dp_start_streaming_response(run_dev->a2dp_hd, true);
                            }
                            bt_sink_srv_report_id("[sink][a2dp] to clear codec", 0);
                            if (!(run_dev->conn_bit & BT_SINK_SRV_MUSIC_A2DP_CONN_BIT)) {
                                bt_sink_srv_cm_profile_status_notify(&run_dev->dev_addr, BT_SINK_SRV_PROFILE_A2DP_SINK, BT_SINK_SRV_PROFILE_CONNECTION_STATE_DISCONNECTED, BT_STATUS_SUCCESS);
                            }
                            bt_sink_srv_music_state_machine_handle(run_dev, BT_SINK_SRV_MUSIC_EVT_CODEC_OPEN, NULL);
                            if (run_dev->op & BT_SINK_SRV_MUSIC_WAITING_REINITIAL_SYNC) {
                                BT_SINK_SRV_REMOVE_FLAG(run_dev->op, BT_SINK_SRV_MUSIC_WAITING_REINITIAL_SYNC);
                            }
                            break;
                        }

                        if (run_dev->flag & BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START) {
                            BT_SINK_SRV_REMOVE_FLAG(run_dev->flag, BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START);
                            bt_sink_srv_report_id("[sink][a2dp]start rsp", 0);
                            ret = bt_a2dp_start_streaming_response(run_dev->a2dp_hd, true);
                        } else {
                            ret = BT_STATUS_SUCCESS;
                        }
                        if (BT_STATUS_SUCCESS == ret) {

                            if (BT_SINK_SRV_MUSIC_TRANSIENT_STATE_CLEAR_CODEC != run_dev->handle->substate) {
#ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__
                                if (run_dev->codec.delay_report && !(run_dev->flag & BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC)) {
                                    int32_t de_ret = 0;
                                    uint32_t delay_report = bt_sink_srv_music_get_sink_latency();
                                    delay_report = delay_report / 10000;
                                    de_ret = bt_a2dp_set_delay_report(run_dev->a2dp_hd, delay_report);
                                    bt_sink_srv_report_id("[sink][a2dp]delay_report--ret: 0x%08x", 1, de_ret);
                                }

                                if (run_dev->codec.sec_type) {
                                    bt_sink_srv_report_id("[sink][a2dp]set content protection", 0);
                                    bt_sink_srv_am_media_handle_t *med_handle = &(run_dev->med_handle);
                                    med_handle->med_hd->set_content_protection(med_handle->med_hd, true);
                                }
#endif /* #ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__ */
#ifdef BT_SINK_SRV_SET_AUDIO_CHANNEL_BY_SINK_MUSIC
                                hal_audio_set_stream_out_channel_number(channel_num);
#endif /* #ifdef BT_SINK_SRV_SET_AUDIO_CHANNEL_BY_SINK_MUSIC */
                            }

                            bt_sink_srv_music_state_machine_handle(run_dev, BT_SINK_SRV_MUSIC_EVT_CODEC_OPEN, NULL);
                            /*MTK_Titan: Update Streaming state to APP*/
                            bt_sink_srv_state_set(BT_SINK_SRV_STATE_STREAMING);

                        } else {
                            /* Error handle */
                            bt_sink_srv_music_state_machine_handle(run_dev, BT_SINK_SRV_MUSIC_EVT_PREPARE_FAIL, NULL);
                        }
                    }
                    break;
                }

            /* interrupt */
            case AUD_SUSPEND_BY_IND: {
                    break;
                }

            case AUD_RESUME_IND: {
                }
                break;

            case AUD_A2DP_PROC_IND: {
                    if (run_dev) {
                        switch (sub_msg) {
                            case AUD_A2DP_CODEC_RESTART: {
                                    break;
                                }
                            case AUD_A2DP_DL_REINIT_REQUEST: {
                                    break;
                                }

                            case AUD_A2DP_ACTIVE_LATENCY_REQUEST: {
                                    break;
                                }

                            default:
                                break;
                        }
                    } else {
                        bt_sink_srv_report_id("[sink][a2dp]ami_hdr--empty run_dev, plase note!!!", 0);
                    }
                    break;
                }
            default:
                break;
        }
    }

    if (ctx->a2dp_aid == aud_id && msg_id == AUD_A2DP_PROC_IND &&
        (sub_msg == AUD_STREAM_EVENT_DATA_REQ)) {
        // drop
        ;
    } else {
        bt_sink_srv_report_id("[sink][a2dp]ami_hdr[e]-ret: %d", 1, ret);
    }
}

void bt_sink_srv_a2dp_initial_avrcp_timer(uint32_t timer_id, uint32_t data)
{
    bt_sink_srv_music_device_t *dev = (bt_sink_srv_music_device_t *)data;
    bt_sink_srv_report_id("[sink][a2dp]initial avrcp timer, conn_bit:0x%04x, hd;0x%08x", 2,
                          dev->conn_bit, dev->avrcp_hd);
    bt_sink_srv_assert(dev && "initial_avrcp_timer");
    if (dev->avrcp_hd == BT_SINK_SRV_MUSIC_INVALID_HD &&
        !(dev->conn_bit & BT_SINK_SRV_MUSIC_AVRCP_CONN_BIT)) {
        uint32_t hd = BT_SINK_SRV_MUSIC_INVALID_HD;
        bt_sink_srv_init_role(BT_AVRCP_ROLE_CT);//[TD-PLAYER]
        bt_status_t ret = bt_avrcp_connect(&hd, (const bt_bd_addr_t *)(&(dev->dev_addr)));
        if (BT_STATUS_SUCCESS == ret) {
            BT_SINK_SRV_SET_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_AVRCP_CONN_RESULT);
            dev->avrcp_hd = hd;
            dev->avrcp_role = BT_AVRCP_ROLE_CT;//[TD-PLAYER]
        }
        bt_sink_srv_report_id("[sink][a2dp]connect_cnf-ret: %d, avrcp_hd: 0x%x", 2, ret, hd);
    }
}

void bt_sink_srv_a2dp_disconnect_avrcp_timer(uint32_t timer_id, uint32_t data)
{
    bt_sink_srv_music_device_t *dev = (bt_sink_srv_music_device_t *)data;
    bt_sink_srv_report_id("[sink][a2dp]deinitial avrcp timer, conn_bit:0x%04x, hd;0x%08x", 2,
                          dev->conn_bit, dev->avrcp_hd);
    if ((dev->conn_bit & BT_SINK_SRV_MUSIC_AVRCP_CONN_BIT) && !(dev->conn_bit &
                                                                BT_SINK_SRV_MUSIC_A2DP_CONN_BIT)) {
        bt_status_t ret = bt_avrcp_disconnect(dev->avrcp_hd);
        bt_sink_srv_report_id("[sink][a2dp]ret:0x%08x", 1, ret);
    }
}

int32_t bt_sink_srv_a2dp_handle_connect_cnf(bt_a2dp_connect_cnf_t *conn_cnf)
{
    bt_sink_srv_music_device_t *dev = NULL;
    //bt_sink_srv_music_context_t *ctx = NULL;
    int32_t ret = 0;
    bool connected = false;
    bt_bd_addr_t dev_addr;
    uint32_t hd = 0;

    //ctx = bt_sink_srv_music_get_context();

    bt_sink_srv_report_id("[sink][a2dp]connect_cnf-a2dp_hd: 0x%x", 1, conn_cnf->handle);

    dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_A2DP_HD, (void *)(&(conn_cnf->handle)));

    if (!dev) {
        return ret;
    }

    bt_sink_srv_memcpy(&dev_addr, &(dev->dev_addr), sizeof(bt_bd_addr_t));
    dev->a2dp_status = BT_SINK_SRV_A2DP_STATUS_SUSPEND;
    if (conn_cnf->status == BT_STATUS_SUCCESS) {
        /* Save codec capability */
        memcpy(&(dev->codec), (conn_cnf->codec_cap), sizeof(bt_a2dp_codec_capability_t));

        bt_sink_srv_report_id("[sink][a2dp]connect_cnf, codec_type:%d, codec_type: %d", 2,
                              dev->codec.type, conn_cnf->codec_cap->type);

        /* As slave */

        //bt_sink_srv_cm_set_role(&(dev->dev_addr), BT_ROLE_SLAVE);
        BT_SINK_SRV_SET_FLAG(dev->conn_bit, BT_SINK_SRV_MUSIC_A2DP_CONN_BIT);

        bt_sink_srv_report_id("[sink][a2dp]connect_cnf-con_bit: 0x%04x, flag:0x%08x", 2,
                              dev->conn_bit, dev->flag);
        if (BT_SINK_SRV_MUSIC_FLAG_DEV_TO_CON_AVRCP & dev->flag) {
            if (dev->avrcp_hd == BT_SINK_SRV_MUSIC_INVALID_HD) {
                bt_sink_srv_init_role(BT_AVRCP_ROLE_CT);//[TD-PLAYER]
                ret = bt_avrcp_connect(&hd, (const bt_bd_addr_t *)(&(dev->dev_addr)));
                if (BT_STATUS_SUCCESS == ret) {
                    BT_SINK_SRV_SET_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_AVRCP_CONN_RESULT);
                    dev->avrcp_hd = hd;
                    dev->avrcp_role = BT_AVRCP_ROLE_CT;//[TD-PLAYER]
                }
            }
            BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_DEV_TO_CON_AVRCP);

            bt_sink_srv_report_id("[sink][a2dp]connect_cnf-ret: %d, avrcp_hd: 0x%x", 2, ret, hd);
        } else {
            if ((!(dev->conn_bit & BT_SINK_SRV_MUSIC_AVRCP_CONN_BIT))
                && (0 == bt_sink_srv_music_get_context()->rho_flag)) {
                uint32_t data = (uint32_t)dev;
                bt_timer_ext_start(BT_SINK_SRV_AVRCP_CONNECTION_TIMER_ID, data,
                                   BT_SINK_SRV_AVRCP_CONNECTION_TIMER_DUR, bt_sink_srv_a2dp_initial_avrcp_timer);
            }
        }
        connected = true;
        /* State machine handle */
        bt_sink_srv_music_state_machine_handle(dev, BT_SINK_SRV_MUSIC_EVT_READY, NULL);
    } else {
        /* State machine handle */
        bt_sink_srv_music_state_machine_handle(dev, BT_A2DP_CONNECT_CNF, conn_cnf);
        BT_SINK_SRV_REMOVE_FLAG(dev->conn_bit, BT_SINK_SRV_MUSIC_A2DP_CONN_BIT);
        BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_INITIAL_A2DP_BY_DEVICE);
        BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_DEV_TO_CON_AVRCP);
        if (!(dev->conn_bit) && dev->avrcp_hd == BT_SINK_SRV_MUSIC_INVALID_HD) {
            bt_sink_srv_a2dp_free_pseudo_handle(dev->handle);
            dev->handle = NULL;
            bt_sink_srv_music_reset_device(dev);
        } else {
            dev->a2dp_hd = BT_SINK_SRV_MUSIC_INVALID_HD;
        }
    }
    if (connected) {
        //void bt_sink_srv_aws_mce_init_eir(bt_bd_addr_t *remote_address);
        bt_sink_srv_cm_profile_status_notify(&dev_addr, BT_SINK_SRV_PROFILE_A2DP_SINK, BT_SINK_SRV_PROFILE_CONNECTION_STATE_CONNECTED, conn_cnf->status);
    } else {
        bt_sink_srv_cm_profile_status_notify(&dev_addr, BT_SINK_SRV_PROFILE_A2DP_SINK, BT_SINK_SRV_PROFILE_CONNECTION_STATE_DISCONNECTED, conn_cnf->status);
    }
    return ret;
}


int32_t bt_sink_srv_a2dp_handle_connect_ind(bt_a2dp_connect_ind_t *conn_ind)
{
    bt_sink_srv_music_device_t *dev = NULL;
    int32_t ret = 0;
    uint8_t *addr = NULL;

    if (!conn_ind) {
        bt_sink_srv_report_id("[sink][a2dp]connect_ind conn_ind is NULL", 0);
        return ret;
    }

    addr = (uint8_t *)conn_ind->address;
    bt_sink_srv_report_id("[sink][a2dp]connect_ind-a2dp_hd: 0x%x, addr--0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x", 7, conn_ind->handle, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    /* A2dp connected */
    dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_AVRCP, (void *)(conn_ind->address));
    if (!dev) {
        dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_A2DP, (void *)(conn_ind->address));
    }
    if (dev == NULL) {
        dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_UNUSED, AUDIO_SRC_SRV_PSEUDO_DEVICE_A2DP);
        if (!dev) {
            bt_sink_srv_report_id("[sink][a2dp]Error, dev NULL", 0);
            return BT_STATUS_FAIL;
        }

        dev->handle = bt_sink_srv_a2dp_alloc_pseudo_handle();
        if (dev->handle) {
            dev->handle->type = AUDIO_SRC_SRV_PSEUDO_DEVICE_A2DP;
            dev->handle->dev_id = bt_sink_srv_music_convert_btaddr_to_devid(conn_ind->address);
            dev->handle->priority = AUDIO_SRC_SRV_PRIORITY_NORMAL;
        }
    }

    /* Init pse handle */
    dev->a2dp_hd = conn_ind->handle;
    dev->role = conn_ind->role;
    memcpy(&(dev->dev_addr), conn_ind->address, sizeof(bt_bd_addr_t));
    BT_SINK_SRV_SET_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_A2DP_CON_IND);

    ret = bt_a2dp_connect_response(conn_ind->handle, true);
    bt_sink_srv_music_state_machine_handle(dev, BT_A2DP_CONNECT_IND, conn_ind);

    return ret;
}


static int32_t bt_sink_srv_a2dp_handle_disconnect_cnf(bt_a2dp_disconnect_cnf_t *disconn_cnf)
{
    bt_sink_srv_music_device_t *a2dp_dev = NULL;
    bt_bd_addr_t dev_addr = {0};
    int32_t ret = 0;

    bt_sink_srv_report_id("[sink][a2dp]disconnect_cnf-hd: 0x%x", 1, disconn_cnf->handle);

    a2dp_dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_A2DP_HD, (void *)(&(disconn_cnf->handle)));
    if (a2dp_dev && a2dp_dev->handle) {
        /* force set a2dp suspend when disconnect */
        a2dp_dev->a2dp_status = BT_SINK_SRV_A2DP_STATUS_SUSPEND;
        bt_sink_srv_report_id("[sink_a2dp_flow]sp_disconnect_cnf delete waiting list handle: 0x%08x", 1, a2dp_dev->handle);
        audio_src_srv_del_waiting_list(a2dp_dev->handle);

        if (a2dp_dev->flag & BT_SINK_SRV_MUSIC_FLAG_WAIT_LIST_SINK_PLAY) {
            BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_LIST_SINK_PLAY);
        }
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_STOP_MUSIC_PENDING);
        bt_sink_srv_memcpy(&dev_addr, &(a2dp_dev->dev_addr), sizeof(bt_bd_addr_t));
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->conn_bit, BT_SINK_SRV_MUSIC_A2DP_CONN_BIT);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_INITIAL_A2DP_BY_DEVICE);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_DEV_TO_CON_AVRCP);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAITING_START);
        /* Clear recover flag */

        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_GAME_FLAG);

        if ((a2dp_dev->op & BT_SINK_SRV_MUSIC_A2DP_HF_INTERRUPT)) {
            BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_A2DP_HF_INTERRUPT);
            //audio_src_srv_del_waiting_list(a2dp_dev->handle);
        }
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC);
        a2dp_dev->a2dp_hd = BT_SINK_SRV_MUSIC_INVALID_HD;
        /* Music state machine handle first */
        bt_sink_srv_music_state_machine_handle(a2dp_dev, BT_A2DP_DISCONNECT_CNF, NULL);
    } else {
        bt_sink_srv_report_id("[sink][a2dp]disconnect_cnf, no device.", 0);
        return 0;
    }

    /* Deinit pse handle */
    if (a2dp_dev->handle && !(a2dp_dev->conn_bit) &&
        (uint8_t)BT_SINK_SRV_MUSIC_TRANSIENT_STATE_CLEAR_CODEC != (uint8_t)a2dp_dev->handle->substate
        && a2dp_dev->avrcp_hd == BT_SINK_SRV_MUSIC_INVALID_HD) {
        bt_sink_srv_a2dp_free_pseudo_handle(a2dp_dev->handle);
        a2dp_dev->handle = NULL;
        bt_sink_srv_music_reset_device(a2dp_dev);
    }
    bool notify_n9_dis = true;
    if (a2dp_dev->handle && (uint8_t)BT_SINK_SRV_MUSIC_TRANSIENT_STATE_CLEAR_CODEC == (uint8_t)a2dp_dev->handle->substate) {
        notify_n9_dis = false;
    }
    if (notify_n9_dis) {
        bt_sink_srv_cm_profile_status_notify(&dev_addr, BT_SINK_SRV_PROFILE_A2DP_SINK, BT_SINK_SRV_PROFILE_CONNECTION_STATE_DISCONNECTED, disconn_cnf->status);
    }

    return ret;
}


static int32_t bt_sink_srv_a2dp_handle_disconnect_ind(bt_a2dp_disconnect_ind_t *disconn_ind)
{
    bt_sink_srv_music_device_t *a2dp_dev = NULL;
    bt_bd_addr_t dev_addr = {0};
    int32_t ret = 0;

    bt_sink_srv_report_id("[sink][a2dp]disconnect_ind-hd: 0x%x", 1, disconn_ind->handle);

    bt_timer_ext_stop(BT_SINK_SRV_AVRCP_CONNECTION_TIMER_ID);
    a2dp_dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_A2DP_HD, (void *)(&(disconn_ind->handle)));
    if (!a2dp_dev) {
        bt_sink_srv_report_id("[sink][a2dp]It has been disconnected!", 0);
        return ret;
    }
    /* force set a2dp suspend when disconnect */
    a2dp_dev->a2dp_status = BT_SINK_SRV_A2DP_STATUS_SUSPEND;
    if (a2dp_dev && a2dp_dev->handle) {
        bt_sink_srv_report_id("[sink_flow] sp_disconnect_ind, delete waiting list hanlde : 0x%08x", 1, a2dp_dev->handle);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_ADD_WAITING_LIST);
        audio_src_srv_del_waiting_list(a2dp_dev->handle);
    }

    if (a2dp_dev && (a2dp_dev->flag & BT_SINK_SRV_MUSIC_FLAG_WAIT_LIST_SINK_PLAY)) {
        //audio_src_srv_del_waiting_list(a2dp_dev->handle);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_LIST_SINK_PLAY);
    }

    if (a2dp_dev && (a2dp_dev->conn_bit & BT_SINK_SRV_MUSIC_A2DP_CONN_BIT)) {
        bt_sink_srv_memcpy(&dev_addr, &(a2dp_dev->dev_addr), sizeof(bt_bd_addr_t));
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->conn_bit, BT_SINK_SRV_MUSIC_A2DP_CONN_BIT);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_INITIAL_A2DP_BY_DEVICE);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_DEV_TO_CON_AVRCP);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_STOP_MUSIC_PENDING);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAITING_START);
        /* Clear recover flag */

        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_GAME_FLAG);

        if ((a2dp_dev->op & BT_SINK_SRV_MUSIC_A2DP_HF_INTERRUPT)) {
            BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_A2DP_HF_INTERRUPT);
            //audio_src_srv_del_waiting_list(a2dp_dev->handle);
        }

        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC);
        /* Music state machine handle first */
        bt_sink_srv_music_state_machine_handle(a2dp_dev, BT_A2DP_DISCONNECT_IND, NULL);
        a2dp_dev->a2dp_hd = BT_SINK_SRV_MUSIC_INVALID_HD;
    }
    uint32_t data = (uint32_t)a2dp_dev;
    if (a2dp_dev->conn_bit & BT_SINK_SRV_MUSIC_AVRCP_CONN_BIT) {
        if (a2dp_dev->avrcp_flag & BT_SINK_SRV_INIT_AVRCP_BY_REMOTE_DEVICE) {
            bt_timer_ext_start(BT_SINK_SRV_AVRCP_DISCONNECT_TIMER_ID, data,
                               BT_SINK_SRV_AVRCP_DISCONNECT_TIMER_DUR, bt_sink_srv_a2dp_disconnect_avrcp_timer);
        } else {
            ret = bt_avrcp_disconnect(a2dp_dev->avrcp_hd);
        }
    } else if ((a2dp_dev->flag & BT_SINK_SRV_MUSIC_FLAG_WAIT_AVRCP_CONN_RESULT)) {
        BT_SINK_SRV_SET_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_INIT_DISCONNNECT_AVRCP_TIMER_FLAG);
    }
    /* Deinit pse handle */
    if (a2dp_dev->handle && !(a2dp_dev->conn_bit) &&
        (uint8_t)BT_SINK_SRV_MUSIC_TRANSIENT_STATE_CLEAR_CODEC != (uint8_t)a2dp_dev->handle->substate
        && a2dp_dev->avrcp_hd == BT_SINK_SRV_MUSIC_INVALID_HD) {

        bt_sink_srv_a2dp_free_pseudo_handle(a2dp_dev->handle);
        a2dp_dev->handle = NULL;
        bt_sink_srv_music_reset_device(a2dp_dev);
    }
    bool notify_n9_dis = true;
    if (a2dp_dev->handle && (uint8_t)BT_SINK_SRV_MUSIC_TRANSIENT_STATE_CLEAR_CODEC == (uint8_t)a2dp_dev->handle->substate) {
        notify_n9_dis = false;
    }
    if (notify_n9_dis) {
        bt_sink_srv_cm_profile_status_notify(&dev_addr, BT_SINK_SRV_PROFILE_A2DP_SINK, BT_SINK_SRV_PROFILE_CONNECTION_STATE_DISCONNECTED, BT_STATUS_SUCCESS);
    }

    return ret;
}


static int32_t bt_sink_srv_a2dp_handle_start_streaming_cnf(bt_a2dp_start_streaming_cnf_t *start_cnf)
{
    int32_t ret = 0;
    bt_a2dp_start_streaming_ind_t start_ind;
    bt_sink_srv_music_device_t *dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_A2DP_HD, (void *)(&(start_cnf->handle)));
    if (dev && start_cnf->status == BT_STATUS_SUCCESS) {
        start_ind.handle = start_cnf->handle;
        start_ind.codec_cap = start_cnf->codec_cap;
        bt_sink_srv_a2dp_handle_start_streaming_ind(&start_ind, true);
    }
    bt_sink_srv_report_id("[sink][a2dp]start_cnf, dev:0x%08x, status:0x%08x", 2, dev, start_cnf->status);
    return ret;
}

static int32_t bt_sink_srv_a2dp_handle_start_streaming_ind(bt_a2dp_start_streaming_ind_t *start_ind, bool is_cnf)
{
    bt_sink_srv_music_device_t *dev = NULL;
    int32_t ret = 0;

    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_A2DP_HD, (void *)(&(start_ind->handle)));
    if (!dev) {
        bt_sink_srv_assert(dev);
        return ret;
    }
    bt_sink_srv_memcpy(&(dev->codec), start_ind->codec_cap, sizeof(bt_a2dp_codec_capability_t));
    dev->a2dp_status = BT_SINK_SRV_A2DP_STATUS_STREAMING;
    if (!is_cnf) {
        BT_SINK_SRV_SET_FLAG(dev->flag, BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START);
    }
    if ((!(dev->avrcp_flag & BT_SINK_SRV_AVRCP_MUST_PLAY_RING_TONE_FLAG))
        && (ctx->context_flag & BT_SINK_SRV_CNTX_FLAG_MUST_PLAY_RING_TONE_FLAG)) {
        BT_SINK_SRV_SET_FLAG(dev->avrcp_flag, BT_SINK_SRV_AVRCP_MUST_PLAY_RING_TONE_FLAG);
        BT_SINK_SRV_REMOVE_FLAG(ctx->context_flag, BT_SINK_SRV_CNTX_FLAG_MUST_PLAY_RING_TONE_FLAG);
    }

    bool need_start_streaming = true;
    if (need_start_streaming) {
        bt_sink_srv_music_state_machine_handle(dev, BT_A2DP_START_STREAMING_IND, NULL);
    }
    bt_sink_srv_report_id("[sink][a2dp]start_ind, ret:0x%08x, avrcp_status:0x%02x", 2,
                          ret, dev->avrcp_status);

    return ret;
}

static int32_t bt_sink_srv_a2dp_handle_suspend_streaming_cnf(bt_a2dp_suspend_streaming_cnf_t *suspend_cnf)
{
    bt_a2dp_suspend_streaming_ind_t suspend_streaming_ind;
    suspend_streaming_ind.handle = suspend_cnf->handle;
    int32_t ret = bt_sink_srv_a2dp_handle_suspend_streaming_ind(&suspend_streaming_ind, false);

    return ret;
}


static int32_t bt_sink_srv_a2dp_handle_suspend_streaming_ind(bt_a2dp_suspend_streaming_ind_t *suspend_ind, bool need_response)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_sink_srv_music_device_t *a2dp_dev = NULL;
    int32_t ret = 0;

    bt_sink_srv_report_id("[sink][a2dp]suspend_streaming_ind, latency:%d", 1, ctx->init_sink_latency);

    a2dp_dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_A2DP_HD, (void *)(&(suspend_ind->handle)));

    _bt_sink_srv_a2dp_clear_media_nodes(&ctx->media_list);

#ifdef MTK_BT_A2DP_FPGA_IT
    ret = bt_a2dp_suspend_streaming_response(suspend_ind->handle, true);
    return ret;
#endif /* #ifdef MTK_BT_A2DP_FPGA_IT */
    if (a2dp_dev) {
        a2dp_dev->a2dp_status = BT_SINK_SRV_A2DP_STATUS_SUSPEND;
        if (!(a2dp_dev->flag & BT_SINK_SRV_MUSIC_FLAG_A2DP_INTERRUPT)) {
            audio_src_srv_del_waiting_list(a2dp_dev->handle);
        }
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_A2DP_HF_INTERRUPT);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_STOP_MUSIC_PENDING);
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAITING_START);

        if (a2dp_dev->op & BT_SINK_SRV_MUSIC_GAME_FLAG) {
            BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_GAME_FLAG);
            BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_A2DP_INTERRUPT);
            audio_src_srv_del_waiting_list(a2dp_dev->handle);
        }

        if (a2dp_dev->flag & BT_SINK_SRV_MUSIC_FLAG_WAIT_LIST_SINK_PLAY) {
            BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_LIST_SINK_PLAY);
            bt_sink_srv_report_id("[sink][a2dp]flag wait list sink play", 0);
        }
    }
    if ((a2dp_dev) &&
        (ctx->run_dev) &&
        (a2dp_dev == ctx->run_dev)) {
        /* SP suspend */
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC);
        /* check other device is playing or not */
        audio_src_srv_del_waiting_list(a2dp_dev->handle);

        bt_sink_srv_music_state_machine_handle(a2dp_dev, BT_A2DP_SUSPEND_STREAMING_IND, NULL);
    }

    /* MTK_David: Change to always send suspend rsp here. */
    ret = bt_a2dp_suspend_streaming_response(suspend_ind->handle, true);

    return ret;
}


static int32_t bt_sink_srv_a2dp_handle_reconfigure_cnf(bt_a2dp_reconfigure_cnf_t *reconfigure_cnf)
{
    int32_t ret = 0;

    return ret;
}


static int32_t bt_sink_srv_a2dp_handle_reconfigure_ind(bt_a2dp_reconfigure_ind_t *reconfigure_ind)
{
    bt_sink_srv_music_device_t *a2dp_dev = NULL;
    int32_t ret = 0;

    a2dp_dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_A2DP_HD, (void *)(&(reconfigure_ind->handle)));
    if (a2dp_dev) {
        memcpy(&(a2dp_dev->codec), reconfigure_ind->codec_cap, sizeof(bt_a2dp_codec_capability_t));
    }

    ret = bt_a2dp_reconfigure_response(reconfigure_ind->handle, true);

    return ret;
}


#ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__
static int32_t bt_sink_srv_a2dp_handle_delay_report_cnf(bt_a2dp_delay_report_cnf_t *delay_cnf)
{
    int32_t ret = 0;

    bt_sink_srv_report_id("[sink][a2dp]delay_report_cnf-hd: 0x%08x, status: 0x%08x", 2,
                          delay_cnf->handle, delay_cnf->status);

    return ret;
}
#endif /* #ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__ */

#define BT_SINK_SRV_A2DP_NODE_DROP_THRESHOLD (15)
// Push data to link list to wait AM pop data
static void _bt_sink_srv_a2dp_push_data(bt_sink_srv_music_data_list_t *pList, uint8_t *pHciNode, uint32_t med_size)
{
    static uint32_t drop_cnt = 0;
    static uint32_t max = 0;
    media_node_t *pNode;

    if (pList == NULL || pHciNode == NULL) {
        bt_sink_srv_report_id("[sink][a2dp]push data node fail! Params == NULL (%p, %p)", 2, pList, pHciNode);
        return;
    }

    if (pList->node_cnt > BT_SINK_SRV_A2DP_NODE_DROP_THRESHOLD) {
        drop_cnt++;
        bt_sink_srv_report_id("[sink][a2dp]drop media data (num of node %d) %d", 1, pList->node_cnt, drop_cnt);
        return;
    }

    pNode = (media_node_t *)pvPortMalloc(sizeof(media_node_t));
    if (pNode == NULL) {
        bt_sink_srv_report_id("[sink][a2dp]push data node fail! (malloc fail)", 0);
        return;
    }
    pNode->pData = pHciNode;
    pNode->size = med_size;
    pNode->pNext = NULL;

    /* hold med node */
    bt_a2dp_hold_media_data_node(pHciNode);

    //Protect this critical section
    bt_sink_srv_mutex_lock();
    if (pList->pHead == NULL) {
        pList->pHead = pNode;
        pList->pTail = pList->pHead;
    } else {
        pList->pTail->pNext = pNode;
        pList->pTail = pNode;
    }
    pList->med_size += med_size; //(pkt->packet_length - pkt->offset);
    pList->node_cnt ++;

    if (pList->node_cnt > max) {
        bt_sink_srv_report_id("[sink][a2dp] push node MAX update %d", 1, max);
        max = pList->node_cnt;
    }

    bt_sink_srv_mutex_unlock();
    //exit critical section
    //bt_sink_srv_report_id("[sink][a2dp]med_list push node %p (list = %p, cnt =%d, head=%p)", 3, pNode, pList, pList->node_cnt, pList->pHead);
}

// Popup data from link list for feeding media data
static uint32_t _bt_sink_srv_a2dp_pop_data(bt_sink_srv_music_data_list_t *pList, uint8_t *buf, uint32_t buf_len)
{
    media_node_t *pNode;
    bt_hci_packet_t *pHciNode;
    uint8_t *pCurNode;
    uint8_t *pMediaRaw; //media raw data
    uint32_t media_len;
    uint32_t ret_len = 0;
    static uint8_t test_log = 0;

    //Protect this critical section
    bt_sink_srv_mutex_lock();
    pNode = pList->pHead;
    if (pNode == NULL) {
        //bt_sink_srv_report_id("[sink][a2dp]no data to pop!", 0);
        bt_sink_srv_mutex_unlock();
        goto pop_node_end;
    }
    pCurNode = pNode->pData;
    media_len = pNode->size;
    pList->med_size -= pNode->size; //(pkt->packet_length - pkt->offset);
    pList->node_cnt --;
    pList->pHead = pNode->pNext;
    bt_sink_srv_mutex_unlock();
    //exit critical section after remove the node

    //copy data to decode buf
    pHciNode = (bt_hci_packet_t *) pCurNode;
    pMediaRaw = pCurNode + pHciNode->offset + BT_SINK_SRV_MEDIA_PKT_HEADER_LEN; //+1 shift SBC media packet header

#ifdef MTK_BT_AUDIO_PR
    app_bt_dbg_audio_pr_write(pCurNode, pHciNode->offset, APP_BT_DBG_AUDIO_DECODE_PATH_DECODE_BEFORE);
#endif /* MTK_BT_AUDIO_PR */

    //Titan to do: need check SBC lib need this byte or not
    //pMediaRaw += 1; //shift 1 to skip SBC media packet header (F,S,L,Rs,NumOfFrame)
    //media_len -= 1; //minus 1 for this shift byte

#ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__
    //Titan to do:no proper device to check this
    if (pList->pRunDev && pList->pRunDev->codec.sec_type & BT_A2DP_CP_SCMS_T) {
        pMediaRaw += 1; //shift 1 to skip cp
        media_len -= 1; //minus 1 for this shift byte
        if (test_log == 0)
            bt_sink_srv_report_id("[sink][a2dp]media_raw shift 1 byte due to support CP_SCMS_T", 0);
    }
#endif /* #ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__ */

    if (media_len > buf_len) {
        bt_sink_srv_report_id("[sink][a2dp]why media len is not enough? (%d, %d))", 2, media_len, buf_len);
        goto pop_node_end;
    }
    memcpy(buf, pMediaRaw, media_len);
    ret_len = media_len;
    if (test_log == 0) {
        test_log = 1;
        bt_sink_srv_report_id("[sink][a2dp] media_raw(%d) 0x%02x %02x %02x %02x %02x, %02x %02x %02x %02x %02x", 0,
                              media_len,
                              pMediaRaw[0], pMediaRaw[1], pMediaRaw[2], pMediaRaw[3], pMediaRaw[4],
                              pMediaRaw[5], pMediaRaw[6], pMediaRaw[7], pMediaRaw[8], pMediaRaw[9]);
    }
    //free all data
    bt_a2dp_release_media_data_node((uint8_t *)pHciNode);
    vPortFree(pNode);

pop_node_end:
    //bt_sink_srv_report_id("[sink][a2dp]med_list pop node %p (cnt = %d)", 1, pNode, pList->node_cnt);
    return ret_len;
}

static void _bt_sink_srv_a2dp_clear_media_nodes(bt_sink_srv_music_data_list_t *pList)
{
    media_node_t *pNode;
    bt_sink_srv_report_id("[sink][a2dp] enter clear media node(list = %p , cnt = %d, size = %d, head=%p)", 2,
                          pList, pList->node_cnt, pList->med_size, pList->pHead);

    bt_sink_srv_mutex_lock();
    while (pList->pHead) {
        bt_sink_srv_report_id("[sink][a2dp] free node(head = %p, cnt = %d)", 1, pList->pHead, pList->node_cnt);
        pNode = pList->pHead;
        pList->med_size -= pNode->size; //(pkt->packet_length - pkt->offset);
        pList->node_cnt --;
        pList->pHead = pNode->pNext;
        bt_a2dp_release_media_data_node(pNode->pData);
        vPortFree(pNode);
    }
    bt_sink_srv_mutex_unlock();
    bt_sink_srv_report_id("[sink][a2dp] exit clear media node(%d)", 1, pList->node_cnt);

}

static int32_t bt_sink_srv_a2dp_handle_data_received_ind(bt_a2dp_streaming_received_ind_t *data_ind)
{
#define MAX_MEDIA_IN_LOG_LOOP                   (600)  //The peroid for printing log
#define BT_SINK_SRV_A2DP_DATA_THRESHHOLD        (6 * 1024)
#ifdef MTK_BT_AUDIO_PR
#define BT_SINK_SRV_A2DP_PKT_THRESHHOLD         (1)
#else /* MTK_BT_AUDIO_PR */
#define BT_SINK_SRV_A2DP_PKT_THRESHHOLD         (5)
#endif /* MTK_BT_AUDIO_PR */

    bt_sink_srv_music_context_t *ctx = NULL;
    bt_sink_srv_music_device_t *run_dev = NULL;
    int32_t ret = 0;
    bt_sink_srv_am_media_handle_t *med_hd = NULL;
    uint32_t payload_size = 0;
    //int32_t free_len;

    ctx = bt_sink_srv_music_get_context();
    run_dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_A2DP_HD, (void *)(&(data_ind->handle)));
    if (run_dev == NULL) {
        bt_sink_srv_report_id("[sink][a2dp]rx_ind(err_drop)-run_dev == NULL", 0);
        return ret;
    }

    if (run_dev->a2dp_hd != data_ind->handle) {
        bt_sink_srv_report_id("[sink][a2dp]rx_ind(err_drop)- state: %d, c_hd: 0x%08x, d_hd: 0x%08x",
                              3, run_dev->state, run_dev->a2dp_hd, data_ind->handle);
        return ret;
    }

    med_hd = &(run_dev->med_handle);
    if (!med_hd) {
        bt_sink_srv_report_id("[sink][a2dp]rx_ind(err_drop)- why med_hd == NULL!!", 0);
        return ret;
    }

    /* data_ind->data_note + data_ind->media_offset = AVDTP media header (12bytes of header)
    AVDTP media heaer = [9b:][7b:Payload type] [2B:Seq Num][4B:Time Stamp][4B:SSRC] --see AVDTP v1.3spec*/
    payload_size = data_ind->total_length - data_ind->media_offset - BT_SINK_SRV_MEDIA_PKT_HEADER_LEN;

#if 0// Raw data debug code. (Can be removed if streaming flow is ready)
    /*EX:
        rx_ind: acl_hdl: 0x00000032, acl_len: 850
        a2dp_aid: 0x00000000, total_len =874, med_offset = 28
        raw #1 0x0 0 0 0 6a, 3 1c 0 0 0
        raw offset(28) 0x80 60 0 1 0, 4a 2c 4e 0 0 <-- start of media header
    */
    bt_hci_packet_t *next_packet = (bt_hci_packet_t *)data_ind->data_node;
    if (s_media_in_log_cnt == 0) {
        bt_sink_srv_report_id("[sink][a2dp]rx_ind: acl_hdl: 0x%08x, acl_len: %d",
                              2, next_packet->acl.handle, next_packet->acl.length);
        bt_sink_srv_report_id("[sink][a2dp]a2dp_aid: 0x%08x, total_len =%d, med_offset = %d",
                              3, ctx->a2dp_aid, data_ind->total_length, data_ind->media_offset);
    }

    if (s_media_in_log_cnt == 0) {
        uint8_t *pDataNode = (uint8_t *)data_ind->data_node;
        uint8_t *pDataShift = (uint8_t *)(pDataNode + data_ind->media_offset);
        bt_sink_srv_report_id("[sink][a2dp] raw #1 0x%02x %02x %02x %02x %02x, %02x %02x %02x %02x %02x", 0,
                              pDataNode[0], pDataNode[1], pDataNode[2], pDataNode[3], pDataNode[4],
                              pDataNode[5], pDataNode[6], pDataNode[7], pDataNode[8], pDataNode[9]);
        bt_sink_srv_report_id("[sink][a2dp] raw offset(%d) 0x%02x %02x %02x %02x %02x, %02x %02x %02x %02x %02x", 0,
                              data_ind->media_offset,
                              pDataShift[0], pDataShift[1], pDataShift[2], pDataShift[3], pDataShift[4],
                              pDataShift[5], pDataShift[6], pDataShift[7], pDataShift[8], pDataShift[9]);
    }

    if (s_media_in_log_cnt++ >= MAX_MEDIA_IN_LOG_LOOP)
        s_media_in_log_cnt = 0;
#endif /* #if 0// Raw data debug code. (Can be removed if streaming flow is ready) */

    if (ctx->media_list.pRunDev == NULL) //update run dev
        ctx->media_list.pRunDev = run_dev;

#ifdef MTK_BT_AUDIO_PR
    app_bt_dbg_audio_pr_write(data_ind->data_node, (unsigned int)data_ind->media_offset, APP_BT_DBG_AUDIO_DECODE_PATH_BTTASK);
#endif /* MTK_BT_AUDIO_PR */

    //(Here is push and pop test code)
    _bt_sink_srv_a2dp_push_data(&ctx->media_list, data_ind->data_node, payload_size);

    //Trigger play if reached threshold
    if (!(run_dev->op & BT_SINK_SRV_MUSIC_OP_PLAY_IND)) {
        if ((ctx->media_list.med_size >= BT_SINK_SRV_A2DP_DATA_THRESHHOLD) ||
            (ctx->media_list.node_cnt >= BT_SINK_SRV_A2DP_PKT_THRESHHOLD)) {
            BT_SINK_SRV_SET_FLAG(run_dev->op, BT_SINK_SRV_MUSIC_OP_PLAY_IND); //MTK_Titan:added to control media input
            bt_sink_srv_report_id("[sink][a2dp]call med_hd->play(aid = %d)", 1, ctx->a2dp_aid);
            med_hd->play(ctx->a2dp_aid);
        }
    }

    return ret;
}

/*Called by AM module to read SBC raw data*/
int32_t bt_sink_srv_a2dp_am_read_data(uint8_t *buf, uint32_t max_len)
{
    bt_sink_srv_music_context_t *ctx = NULL;

    ctx = bt_sink_srv_music_get_context();
    if (ctx == NULL) {
        bt_sink_srv_report_id("[sink][a2dp]am read fail!! %p, %p", 1, ctx);
        return 0;
    }

    return _bt_sink_srv_a2dp_pop_data(&ctx->media_list, buf, max_len);
}

audio_src_srv_handle_t *bt_sink_srv_a2dp_alloc_pseudo_handle(void)
{
    int32_t i = 0;
    audio_src_srv_handle_t *hd = NULL;

    for (i = 0; i < BT_SINK_SRV_A2DP_PSEUDO_COUNT + BT_SINK_SRV_MCE_A2DP_PSEUDO_COUNT; ++i) {
        if (!(g_a2dp_pse_hd[i].flag & BT_SINK_SRV_A2DP_PSEUDO_FLAG_USEED)) {
            hd = g_a2dp_pse_hd[i].hd;
            BT_SINK_SRV_SET_FLAG(g_a2dp_pse_hd[i].flag, BT_SINK_SRV_A2DP_PSEUDO_FLAG_USEED);
            bt_sink_srv_music_fill_audio_src_callback(hd);
            break;
        }
    }

    bt_sink_srv_report_id("[sink][a2dp]alloc_pseudo_handle--hd: 0x%x", 1, hd);

    bt_sink_srv_assert(hd);
    return hd;
}


void bt_sink_srv_a2dp_free_pseudo_handle(audio_src_srv_handle_t *hd)
{
    int32_t i = 0;

    for (i = 0; i < BT_SINK_SRV_A2DP_PSEUDO_COUNT + BT_SINK_SRV_MCE_A2DP_PSEUDO_COUNT; ++i) {
        if ((hd) &&
            (g_a2dp_pse_hd[i].flag & BT_SINK_SRV_A2DP_PSEUDO_FLAG_USEED) &&
            (g_a2dp_pse_hd[i].hd == hd)) {
            BT_SINK_SRV_REMOVE_FLAG(g_a2dp_pse_hd[i].flag, BT_SINK_SRV_A2DP_PSEUDO_FLAG_USEED);
            break;
        }
    }

    bt_sink_srv_report_id("[sink][a2dp]free_pseudo_handle--hd: 0x%x", 1, hd);
}


bt_status_t bt_sink_srv_a2dp_action_handler(bt_sink_srv_action_t action, void *param)
{
    /* MTK_David:Combined bt_sink_srv_a2dp_action_handler and bt_sink_srv_a2dp_cm_callback_handler together. */
    bt_status_t ret = 0;
    //int32_t err = 0;
    bt_sink_srv_music_device_t *dev = NULL;
    bt_bd_addr_t *dev_addr = NULL;
    uint32_t hd = 0;
    bt_sink_srv_profile_connection_action_t *conn_info = NULL;

    //bt_sink_srv_report_id("[sink][a2dp]process_a2dp_action[s]-action: 0x%x, base: 0x%x\n", 2, action, BT_SINK_MODULE_A2DP_ACTION);

    switch (action) {
        case BT_SINK_SRV_ACTION_PROFILE_INIT: {
                break;
            }
        case BT_SINK_SRV_ACTION_PROFILE_DEINIT: {
                break;
            }
        case BT_SINK_SRV_ACTION_PROFILE_CONNECT: {
                conn_info = (bt_sink_srv_profile_connection_action_t *)param;
                dev_addr = &(conn_info->address);
                bt_device_manager_db_remote_pnp_info_t dev_id_p;
                bt_device_manager_remote_find_pnp_info((void *)dev_addr, &dev_id_p);

                if (!(conn_info->profile_connection_mask & BT_SINK_SRV_PROFILE_A2DP_SINK))
                    break;

                ret = bt_a2dp_connect(&hd, (const bt_bd_addr_t *)dev_addr, BT_A2DP_SINK);
                if (ret != BT_STATUS_SUCCESS) {
                    bt_sink_srv_report_id("[sink][a2dp]Connect Fail 0x%x!", 1, ret);
                    break;
                }

                dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_A2DP, (void *)dev_addr);
                if (!dev)
                    dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_AVRCP, (void *)dev_addr);

                if (!dev) {
                    dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_UNUSED, NULL);
                    bt_sink_srv_assert(dev && "Error: a2dp dev NULL");

                    dev->handle = bt_sink_srv_a2dp_alloc_pseudo_handle();
                    if (dev->handle) {
                        dev->handle->type = AUDIO_SRC_SRV_PSEUDO_DEVICE_A2DP;
                        dev->handle->dev_id = bt_sink_srv_music_convert_btaddr_to_devid(dev_addr);
                        dev->handle->priority = AUDIO_SRC_SRV_PRIORITY_NORMAL;
                    }
                }

                dev->a2dp_hd = hd;
                dev->role = BT_A2DP_SINK;
                BT_SINK_SRV_SET_FLAG(dev->flag, BT_SINK_SRV_MUSIC_INITIAL_A2DP_BY_DEVICE);
                bt_sink_srv_memcpy(&(dev->dev_addr), dev_addr, sizeof(bt_bd_addr_t));
                break;
            }
        case BT_SINK_SRV_ACTION_PROFILE_DISCONNECT: {
                conn_info = (bt_sink_srv_profile_connection_action_t *)param;
                dev_addr = &(conn_info->address);

                if (!(conn_info->profile_connection_mask & BT_SINK_SRV_PROFILE_A2DP_SINK))
                    break;

                dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_A2DP, (void *)dev_addr);
                if ((dev) && (dev->conn_bit & BT_SINK_SRV_MUSIC_A2DP_CONN_BIT)) {
                    bt_timer_ext_stop(BT_SINK_SRV_AVRCP_CONNECTION_TIMER_ID);
                    ret = bt_a2dp_disconnect(dev->a2dp_hd);
                    if (ret != BT_STATUS_SUCCESS) {
                        bt_sink_srv_report_id("[sink][a2dp]disconnect fail 0x%x!", 1, ret);
                        break;
                    }
                }
                break;
            }
        case BT_SINK_SRV_ACTION_VOLUME_UP: {
                bt_sink_srv_a2dp_change_volume(VOLUME_UP, 1, 0, dev);
                break;
            }
        case BT_SINK_SRV_ACTION_VOLUME_DOWN: {
                bt_sink_srv_a2dp_change_volume(VOLUME_DOWN, 1, 0, dev);
                break;
            }
        case BT_SINK_SRV_ACTION_SET_VOLUME: {
                uint8_t *volume_value = (uint8_t *)param;
                bt_sink_srv_a2dp_change_volume(VOLUME_VALUE, 1, *volume_value, dev);
                break;
            }
        case BT_SINK_SRV_ACTION_SET_LATENCY: {
                bt_sink_srv_report_id("[sink][a2dp]Action is set sink latency.", 0);
                if (param) {
                    uint32_t *latency_val = (uint32_t *)param;
                    ret = bt_sink_srv_music_set_sink_latency(*latency_val, true);
                    if (ret != BT_STATUS_SUCCESS) {
                        bt_sink_srv_report_id("[sink][a2dp]latency set fail 0x%x!", 1, ret);
                        break;
                    }
                } else {
                    bt_sink_srv_report_id("[sink][a2dp]sink latency is null!", 0);
                }
                break;
            }

        default:
            break;
    }

    //bt_sink_srv_report_id("[sink][a2dp]process_a2dp_action[e]-ret: %d", 1, ret);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sink_srv_a2dp_common_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
{
    int32_t ret = 0;

    switch (msg) {
        case BT_A2DP_CONNECT_CNF: {
                bt_a2dp_connect_cnf_t *conn_cnf = (bt_a2dp_connect_cnf_t *)buffer;
                ret = bt_sink_srv_a2dp_handle_connect_cnf(conn_cnf);
                break;
            }

        case BT_A2DP_CONNECT_IND: {
                bt_a2dp_connect_ind_t *conn_ind = (bt_a2dp_connect_ind_t *)buffer;
                ret = bt_sink_srv_a2dp_handle_connect_ind(conn_ind);
                break;
            }

        case BT_A2DP_DISCONNECT_CNF: {
                bt_a2dp_disconnect_cnf_t *disconn_cnf = (bt_a2dp_disconnect_cnf_t *)buffer;
                ret = bt_sink_srv_a2dp_handle_disconnect_cnf(disconn_cnf);
                break;
            }

        case BT_A2DP_DISCONNECT_IND: {
                bt_a2dp_disconnect_ind_t *disconn_ind = (bt_a2dp_disconnect_ind_t *)buffer;
                ret = bt_sink_srv_a2dp_handle_disconnect_ind(disconn_ind);
                break;
            }

        case BT_A2DP_START_STREAMING_CNF: {
                bt_a2dp_start_streaming_cnf_t *start_cnf = (bt_a2dp_start_streaming_cnf_t *)buffer;
                ret = bt_sink_srv_a2dp_handle_start_streaming_cnf(start_cnf);
                break;
            }

        case BT_A2DP_START_STREAMING_IND: {
                bt_a2dp_start_streaming_ind_t *start_ind = (bt_a2dp_start_streaming_ind_t *)buffer;
                //bt_sink_srv_report_id("[sink][a2dp]start_ind--buff: 0x%x, cap: 0x%x", 2, buffer, start_ind->codec_cap);
                //bt_a2dp_clear_gpio_12();

#ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__
                bt_sink_srv_report_id("[sink][a2dp]start_ind--sec_type: 0x%04x, delay: %d", 2, start_ind->codec_cap->sec_type, start_ind->codec_cap->delay_report);
#endif /* #ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__ */

                if (start_ind->codec_cap->type == BT_A2DP_CODEC_SBC) {
                    bt_sink_srv_report_id("[sink][a2dp]start_ind(sbc)--hd: 0x%x, type: %d, sep_type: %d, len: %d, 1: %d, 2: %d, 3: %d, 4: %d, 5: %d, 6: %d, 7: %d", 11,
                                          start_ind->handle,
                                          start_ind->codec_cap->type,
                                          start_ind->codec_cap->sep_type,
                                          start_ind->codec_cap->length,
                                          start_ind->codec_cap->codec.sbc.channel_mode,
                                          start_ind->codec_cap->codec.sbc.sample_freq,
                                          start_ind->codec_cap->codec.sbc.alloc_method,
                                          start_ind->codec_cap->codec.sbc.subbands,
                                          start_ind->codec_cap->codec.sbc.block_len,
                                          start_ind->codec_cap->codec.sbc.min_bitpool,
                                          start_ind->codec_cap->codec.sbc.max_bitpool);
                } else if (start_ind->codec_cap->type == BT_A2DP_CODEC_AAC) {
                    bt_sink_srv_report_id("[sink][a2dp]start_ind(aac)--hd: 0x%x, type: %d, sep_type: %d, len: %d, 1: %d, 2: %d, 3: %d, 4: %d, 5: %d, 6: %d, 7: %d, 8: %d, 9: %d", 13,
                                          start_ind->handle,
                                          start_ind->codec_cap->type,
                                          start_ind->codec_cap->sep_type,
                                          start_ind->codec_cap->length,
                                          start_ind->codec_cap->codec.aac.object_type,
                                          start_ind->codec_cap->codec.aac.freq_h,
                                          start_ind->codec_cap->codec.aac.reserved,
                                          start_ind->codec_cap->codec.aac.channels,
                                          start_ind->codec_cap->codec.aac.freq_l,
                                          start_ind->codec_cap->codec.aac.br_h,
                                          start_ind->codec_cap->codec.aac.vbr,
                                          start_ind->codec_cap->codec.aac.br_m,
                                          start_ind->codec_cap->codec.aac.br_l);
                }

                ret = bt_sink_srv_a2dp_handle_start_streaming_ind(start_ind, false);
                break;
            }

        case BT_A2DP_SUSPEND_STREAMING_CNF: {
                bt_a2dp_suspend_streaming_cnf_t *suspend_cnf = (bt_a2dp_suspend_streaming_cnf_t *)buffer;

                ret = bt_sink_srv_a2dp_handle_suspend_streaming_cnf(suspend_cnf);
                break;
            }

        case BT_A2DP_SUSPEND_STREAMING_IND: {
                bt_a2dp_suspend_streaming_ind_t *suspend_ind = (bt_a2dp_suspend_streaming_ind_t *)buffer;

                ret = bt_sink_srv_a2dp_handle_suspend_streaming_ind(suspend_ind, true);
                break;
            }

        case BT_A2DP_RECONFIGURE_CNF: {
                bt_a2dp_reconfigure_cnf_t *reconfigure_cnf = (bt_a2dp_reconfigure_cnf_t *)buffer;

                ret = bt_sink_srv_a2dp_handle_reconfigure_cnf(reconfigure_cnf);
                break;
            }

        case BT_A2DP_RECONFIGURE_IND: {
                bt_a2dp_reconfigure_ind_t *reconfigure_ind = (bt_a2dp_reconfigure_ind_t *)buffer;

                ret = bt_sink_srv_a2dp_handle_reconfigure_ind(reconfigure_ind);
                break;
            }

        case BT_A2DP_STREAMING_RECEIVED_IND: {
                //bt_sink_srv_report_id("[sink][a2dp]Media data recevied.", 0);
                //MTK_Titan: Added to support SW decode flow
                bt_a2dp_streaming_received_ind_t *data_ind = (bt_a2dp_streaming_received_ind_t *)buffer;
                ret = bt_sink_srv_a2dp_handle_data_received_ind(data_ind);
                break;
            }

#ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__
        case BT_A2DP_DELAY_REPORT_CNF: {
                bt_a2dp_delay_report_cnf_t *delay_cnf = (bt_a2dp_delay_report_cnf_t *)buffer;
                ret = bt_sink_srv_a2dp_handle_delay_report_cnf(delay_cnf);
                break;
            }
#endif /* #ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__ */
        default:
            break;
    }

    /*if (msg != BT_A2DP_STREAMING_RECEIVED_IND && msg != BT_MEMORY_FREE_GARBAGE_IND) {
        bt_sink_srv_report_id("[sink][a2dp]common_hdr[e]-ret: %d", 1, ret);
    }*/

    return ret;
}

void bt_sink_srv_a2dp_drv_play(void *param)
{
    bt_sink_srv_music_device_t *a2dp_dev = (bt_sink_srv_music_device_t *)param;
    bt_sink_srv_music_context_t *ctx = NULL;
    bt_sink_srv_am_media_handle_t *med_hd = NULL;
    int32_t ret = 0;
    uint32_t gpt_run_count_begin = 0;
    uint32_t gpt_run_count_end = 0;
    uint32_t cost_dur = 0;

    ctx = bt_sink_srv_music_get_context();

    if (ctx->run_dev == a2dp_dev) {
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_DRV_PLAY);
        med_hd = &(a2dp_dev->med_handle);

        ret = med_hd->play(ctx->a2dp_aid);
        bt_sink_srv_report_id("[sink][a2dp]drv_play--ret: %d", 1, ret);
        if (BT_CODEC_MEDIA_STATUS_OK == ret) {
            BT_SINK_SRV_SET_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_OP_DRV_PLAY);
            if (!(a2dp_dev->flag & BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC)) {
                bt_sink_srv_state_set(BT_SINK_SRV_STATE_STREAMING);
            }
            //bt_sink_srv_music_state_machine_handle(a2dp_dev, BT_SINK_SRV_MUSIC_EVT_PLAYING, NULL);
        } else {
            /* Error handle */
            bt_sink_srv_music_state_machine_handle(a2dp_dev, BT_SINK_SRV_MUSIC_EVT_PREPARE_FAIL, NULL);
        }
    }
}

int32_t bt_sink_srv_a2dp_change_volume(uint8_t type, uint8_t sync, uint32_t volume_value, bt_sink_srv_music_device_t *dev)
{
    bt_sink_srv_mutex_lock();

    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_sink_srv_music_device_t *run_dev = ctx->run_dev;
    int32_t ret = BT_SINK_SRV_MUSIC_ERR_SUCCESS_OK;
    bt_sink_srv_am_id_t ami_ret = 0;
    uint8_t vol = ctx->vol_lev;

    /* volume up */
    if (VOLUME_UP == type) {
        if (vol < BT_SINK_SRV_A2DP_MAX_VOL_LEV) {
            vol = vol + 1;
        } else {
            ret = BT_SINK_SRV_MUSIC_ERR_FAIL_1ST;
        }
    } else if (VOLUME_DOWN == type) {
        if (vol > BT_SINK_SRV_A2DP_MIN_VOL_LEV) {
            vol = vol - 1;
        } else {
            ret = BT_SINK_SRV_MUSIC_ERR_FAIL_2ND;
        }
    } else if (VOLUME_VALUE == type) {
        if (volume_value <= BT_SINK_SRV_A2DP_MAX_VOL_LEV) {
            vol = volume_value;
        }
    }

    if ((vol != ctx->vol_lev) && dev) {
        if (run_dev == dev && sync) {
            ami_ret = bt_sink_srv_ami_audio_set_volume(ctx->a2dp_aid, vol, STREAM_OUT);
        }
        ctx->vol_lev = vol;
        ctx->last_volume = vol;
        if (sync) {
            bt_sink_srv_music_set_nvdm_data(&(dev->dev_addr), BT_SINK_SRV_MUSIC_DATA_VOLUME, &ctx->vol_lev);
        }
        bt_sink_srv_avrcp_volume_notification(dev->avrcp_hd, ctx->vol_lev, type);
    }

    bt_sink_srv_report_id("[sink][a2dp]change_volume-ami_ret: %d, ret: %d, vol: %d, dev:0x%x, run_dev:0x%x", 5,
                          ami_ret, ret, ctx->vol_lev, dev, run_dev);
    bt_sink_srv_mutex_unlock();

    return ret;
}


void bt_sink_srv_a2dp_play(audio_src_srv_handle_t *handle)
{
    bt_sink_srv_music_device_t *a2dp_dev = NULL;
    bt_sink_srv_am_audio_capability_t aud_cap = {0};
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_sink_srv_am_result_t am_ret;
    uint32_t latency_val = 0;
    int32_t ret = 0;

    a2dp_dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_PSE_HD, (void *)handle);
    if (!a2dp_dev) {
        bt_sink_srv_assert(a2dp_dev);
        return;
    }

    BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_LIST_SINK_PLAY);

    bt_sink_srv_report_id("[sink][a2dp]play(s)--hd: 0x%08x, type: %d, flag: 0x%08x, op: 0x%08x", 4,
                          handle, handle->type, a2dp_dev->flag, a2dp_dev->op);

    if (a2dp_dev->op & BT_SINK_SRV_MUSIC_A2DP_HF_INTERRUPT) {
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_A2DP_HF_INTERRUPT);
        BT_SINK_SRV_SET_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_AVRCP_PLAY_TRIGGER);
        a2dp_dev->handle->substate = BT_SINK_SRV_MUSIC_TRANSIENT_STATE_PREPARE_CODEC;
        bt_sink_srv_report_id("[sink][a2dp]play(s)--HF int resume", 0);
    }

    if (a2dp_dev->flag & BT_SINK_SRV_MUSIC_FLAG_A2DP_INTERRUPT) {
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_A2DP_INTERRUPT);

        if (!(a2dp_dev->op & BT_SINK_SRV_MUSIC_GAME_FLAG)) {
            ret = bt_sink_srv_avrcp_play_music(a2dp_dev);
        }
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_GAME_FLAG);

        bt_sink_srv_report_id("[sink][a2dp]play(int)--ret: %d, a2dp_status:%d", 2, ret, a2dp_dev->a2dp_status);
        if (a2dp_dev->a2dp_status == BT_SINK_SRV_A2DP_STATUS_SUSPEND) {
            bt_sink_srv_music_state_machine_handle(a2dp_dev, BT_SINK_SRV_MUSIC_EVT_RESUME, NULL);
            return ;
        }
    }

    /* Audio source accept play request */
    BT_SINK_SRV_SET_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_AMI_OPEN_CODEC);
    /* Update run device */
    bt_sink_srv_music_update_run_device(a2dp_dev);
    /* 1. Open A2DP codec */
    bt_sink_srv_music_get_nvdm_data(&(a2dp_dev->dev_addr), BT_SINK_SRV_MUSIC_DATA_VOLUME, &ctx->vol_lev);

    uint16_t a2dp_mtu = bt_a2dp_get_mtu_size(a2dp_dev->a2dp_hd);
    bt_sink_srv_music_fill_am_aud_param(&aud_cap, &a2dp_dev->codec, BT_A2DP_SINK, a2dp_mtu);

    if (!(a2dp_dev->flag & BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC)) {
        latency_val = bt_sink_srv_get_latency();
        bt_sink_srv_music_set_sink_latency(latency_val, true);
    }
    int32_t gpt_ret = 0;
    gpt_ret = (int32_t)hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &g_a2dp_gpt_codec_run_count_begin);
    bt_sink_srv_report_id("[sink][a2dp]drv_play--begin: %d, gpt_ret: %d", 2, g_a2dp_gpt_codec_run_count_begin, gpt_ret);

    am_ret = bt_sink_srv_ami_audio_play(ctx->a2dp_aid, &aud_cap);
    bt_sink_srv_update_last_device(&(a2dp_dev->dev_addr), BT_SINK_SRV_PROFILE_A2DP_SINK);

    if (AUD_EXECUTION_SUCCESS != am_ret) {
        /* Exception: play fail */
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_AMI_OPEN_CODEC);
        bt_sink_srv_music_update_run_device(NULL);
        bt_sink_srv_music_state_machine_handle(a2dp_dev, BT_SINK_SRV_MUSIC_EVT_PREPARE_FAIL, NULL);
    }
    if ((uint8_t)a2dp_dev->handle->substate != (uint8_t)BT_SINK_SRV_MUSIC_TRANSIENT_STATE_PREPARE_CODEC) {
        bt_sink_srv_music_state_machine_handle(a2dp_dev, BT_SINK_SRV_MUSIC_EVT_PREPARE_CODEC, NULL);
    }
    bt_gap_reset_sniff_timer(BT_SINK_SRV_A2DP_MIN_SNIFF_DUR);
    bt_sink_srv_report_id("[sink][a2dp]to set sniff timer to 1s", 0);

    BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_AVRCP_PLAY_TRIGGER);
}

void bt_sink_srv_a2dp_stop(audio_src_srv_handle_t *handle)
{
    bt_sink_srv_music_device_t *a2dp_dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_PSE_HD, (void *)handle);
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();

    bt_sink_srv_assert(a2dp_dev);

    bt_sink_srv_report_id("[sink][a2dp]stop(s)--hd: 0x%x, type: %d, flag: 0x%x, op: 0x%x", 4,
                          handle, handle->type, a2dp_dev->flag, a2dp_dev->op);

    BT_SINK_SRV_SET_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_NORMAL_STOP_FLAG);

    //MTK_Titan:clear media list if stop music. (If disconnection directly, the suspend flow won't be run.)
    if (!ctx) {
        bt_sink_srv_report_id("[sink][a2dp]stop, music context is NULL", 0);
        return;
    }
    _bt_sink_srv_a2dp_clear_media_nodes(&ctx->media_list);

    /* Clear codec */
    bt_sink_srv_music_stop(a2dp_dev, ctx->a2dp_aid);

    if (a2dp_dev->handle && !(a2dp_dev->conn_bit) &&
        a2dp_dev->avrcp_hd == BT_SINK_SRV_MUSIC_INVALID_HD) {
        bt_sink_srv_a2dp_free_pseudo_handle(a2dp_dev->handle);
        a2dp_dev->handle = NULL;
        bt_sink_srv_music_reset_device(a2dp_dev);
        if (ctx->context_flag & BT_SINK_SRV_CNTX_FLAG_POWER_OFF) {
            bt_sink_srv_ami_audio_close(ctx->a2dp_aid);
            ctx->a2dp_aid = BT_SINK_SRV_INVALID_AID;
            BT_SINK_SRV_REMOVE_FLAG(ctx->context_flag, BT_SINK_SRV_CNTX_FLAG_POWER_OFF);
        }
    }

    if (((a2dp_dev->flag & BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC) && a2dp_dev->handle) || (a2dp_dev->flag & BT_SINK_SRV_MUSIC_FLAG_WAITING_START)) {
        BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAITING_START);
        bt_sink_srv_music_state_machine_handle(a2dp_dev, BT_A2DP_START_STREAMING_IND, NULL);
    }
}


void bt_sink_srv_a2dp_suspend(audio_src_srv_handle_t *handle, audio_src_srv_handle_t *int_hd)
{
    bt_sink_srv_mutex_lock();
    bt_sink_srv_music_device_t *a2dp_dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_PSE_HD, (void *)handle);
    bt_sink_srv_music_device_t *int_dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_PSE_HD, (void *)int_hd);
    int32_t ret = 0, err = 0;
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();

    bt_sink_srv_assert(a2dp_dev);

    bt_sink_srv_report_id("[sink][a2dp]suspend(s)--hd: 0x%x, type: %d, int: 0x%x, type: %d, flag: 0x%x, op: 0x%x", 6,
                          handle, handle->type, int_hd, int_hd->type, a2dp_dev->flag, a2dp_dev->op);

    /*GVA-13775, if a2dp suspend come before open codec done and esco need to suspend a2dp, just do nothing.*/
    if (a2dp_dev->a2dp_status == BT_SINK_SRV_A2DP_STATUS_SUSPEND
        && (uint8_t)BT_SINK_SRV_MUSIC_TRANSIENT_STATE_CLEAR_CODEC == (uint8_t)a2dp_dev->handle->substate) {
        bt_sink_srv_mutex_unlock();
        return;
    }

    bt_device_manager_db_remote_pnp_info_t dev_id_p = {0, 0};
    bt_device_manager_remote_find_pnp_info(a2dp_dev->dev_addr, &dev_id_p);

    /* Clear codec */
    if (a2dp_dev->handle->substate != BT_SINK_SRV_MUSIC_TRANSIENT_STATE_CLEAR_CODEC) {
        bt_sink_srv_music_state_machine_handle(a2dp_dev, BT_SINK_SRV_MUSIC_EVT_PREPARE_CLEAR, NULL);
    }
    BT_SINK_SRV_REMOVE_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC);

    bt_sink_srv_music_stop(a2dp_dev, ctx->a2dp_aid);

    if ((handle->dev_id == int_hd->dev_id) &&
        (int_hd->type == AUDIO_SRC_SRV_PSEUDO_DEVICE_HFP)) {
        if (a2dp_dev->avrcp_status == BT_AVRCP_STATUS_PLAY_PLAYING) {
            /* WSAP00041710 Nexus 5, HF don't interrupt A2DP */
            /* Add self in waiting list */
            bt_sink_srv_report_id("[sink][a2dp]a2dp to add waiting list", 0);
            bt_sink_srv_a2dp_add_waitinglist(handle);
            /* Set interrupt flag */
            BT_SINK_SRV_SET_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_A2DP_HF_INTERRUPT);
            err = BT_SINK_SRV_MUSIC_ERR_SUCCESS_7TH;
        } else {
            /* The same device & HF interrupt */
            err = BT_SINK_SRV_MUSIC_ERR_SUCCESS_1ST;
        }
    } else if ((int_hd->type == AUDIO_SRC_SRV_PSEUDO_DEVICE_A2DP) ||
               (int_hd->priority == AUDIO_SRC_SRV_PRIORITY_NORMAL)) {
        /* PartyMode interrupt */
        if ((int_dev && (!(int_dev->avrcp_flag & BT_SINK_SRV_AVRCP_MUST_PLAY_RING_TONE_FLAG)))) {
            if (dev_id_p.product_id == 0x1200 && dev_id_p.vender_id == 0x038f && a2dp_dev->a2dp_status == BT_SINK_SRV_A2DP_STATUS_STREAMING) {
                ret = bt_sink_srv_avrcp_force_pause_music(a2dp_dev);
            } else {
                ret = bt_sink_srv_avrcp_stop_music(a2dp_dev);
            }
        }
        if (ret != BT_STATUS_SUCCESS) {
            /* Pause SP failed */
            bt_sink_srv_report_id("[sink][a2dp]suspend(err)--ret: %d", 1, ret);
        }
        err = BT_SINK_SRV_MUSIC_ERR_SUCCESS_2ND;
    } else {
        /* Add self in waiting list */
        if (!(a2dp_dev->avrcp_flag & BT_SINK_SRV_AVRCP_MUST_PLAY_RING_TONE_FLAG)) {
            BT_SINK_SRV_SET_FLAG(a2dp_dev->flag, BT_SINK_SRV_MUSIC_FLAG_A2DP_INTERRUPT);
            bt_sink_srv_a2dp_add_waitinglist(handle);
            /* Set interrupt flag */
            /* Send pause cmd */
            if (dev_id_p.product_id == 0x1200 && dev_id_p.vender_id == 0x038f && a2dp_dev->a2dp_status == BT_SINK_SRV_A2DP_STATUS_STREAMING) {
                ret = bt_sink_srv_avrcp_force_pause_music(a2dp_dev);
            } else {
                if (a2dp_dev->avrcp_status == BT_AVRCP_STATUS_PLAY_PLAYING) {
                    ret = bt_sink_srv_avrcp_stop_music(a2dp_dev);
                } else {
                    BT_SINK_SRV_SET_FLAG(a2dp_dev->op, BT_SINK_SRV_MUSIC_GAME_FLAG);
                }
            }
        }
        if (ret != BT_STATUS_SUCCESS) {
            /* Pause SP failed */
            bt_sink_srv_report_id("[sink][a2dp]suspend(error)--ret: %d", 1, ret);
        }
        err = BT_SINK_SRV_MUSIC_ERR_SUCCESS_3RD;
    }

    bt_sink_srv_mutex_unlock();

    bt_sink_srv_report_id("[sink][a2dp]suspend(e)--ret: %d, err: %d", 2, ret, err);
}

void bt_sink_srv_a2dp_reject(audio_src_srv_handle_t *handle)
{
    bt_sink_srv_mutex_lock();
    bt_sink_srv_music_device_t *dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_PSE_HD, (void *)handle);
    const audio_src_srv_handle_t *rej_handle = audio_src_srv_get_runing_pseudo_device();
    bt_device_manager_db_remote_pnp_info_t dev_id_p = {0, 0};

    if (!dev) {
        bt_sink_srv_assert(dev);
        return;
    }
    bt_sink_srv_assert(rej_handle);
    bt_device_manager_remote_find_pnp_info(dev->dev_addr, &dev_id_p);

    bt_sink_srv_report_id("[sink][a2dp]reject(s)--hd: 0x%x, type: %d, flag: 0x%x, op: 0x%x", 4,
                          handle, handle->type, dev->flag, dev->op);

    if (rej_handle->dev_id == handle->dev_id) {
        bt_sink_srv_report_id("[sink][a2dp]Rejected by same device, add to waiting list", 0);
        BT_SINK_SRV_SET_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_LIST_SINK_PLAY);
        audio_src_srv_add_waiting_list(handle);
        if (dev && (dev->flag & BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START)) {
            bt_a2dp_start_streaming_response(dev->a2dp_hd, true);
            BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START);
        }
    } else {
        /* Reject handle case 1. accept SP start streaming; 2. pause SP music */

        if (dev->flag & BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START) {
            BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START);
            bt_a2dp_start_streaming_response(dev->a2dp_hd, true);
        }
        if (dev_id_p.product_id == 0x1200 && dev_id_p.vender_id == 0x038f && dev->a2dp_status == BT_SINK_SRV_A2DP_STATUS_STREAMING) {
            if (dev->conn_bit & BT_SINK_SRV_MUSIC_AVRCP_CONN_BIT) {
                bt_sink_srv_avrcp_force_pause_music(dev);
            } else {
                BT_SINK_SRV_SET_FLAG(dev->avrcp_flag, BT_SINK_SRV_AVRCP_WAITING_FORCE_PAUSE_FLAG);
            }
        } else {
            bt_sink_srv_avrcp_stop_music(dev);
        }
    }
    bt_gap_connection_handle_t gap_hd = bt_sink_srv_cm_get_gap_handle(&(dev->dev_addr));
    bt_sink_srv_assert(gap_hd);
    bt_sink_srv_music_state_machine_handle(dev, BT_SINK_SRV_MUSIC_EVT_REJECT, NULL);

    /* force add to waiting list to avoid pause fail */
    if (dev->a2dp_status == BT_SINK_SRV_A2DP_STATUS_STREAMING
        || dev->avrcp_status == BT_AVRCP_STATUS_PLAY_PLAYING) {
        bt_sink_srv_a2dp_add_waitinglist(dev->handle);
    }
    /* Notify state machine reject reason */
    bt_sink_srv_mutex_unlock();
}


void bt_sink_srv_a2dp_exception(audio_src_srv_handle_t *handle, int32_t event, void *param)
{
}


void bt_sink_srv_a2dp_create_pse_handle(void)
{
    int32_t i = 0;

    for (i = 0; i < BT_SINK_SRV_A2DP_PSEUDO_COUNT + BT_SINK_SRV_MCE_A2DP_PSEUDO_COUNT; ++i) {
        g_a2dp_pse_hd[i].hd = audio_src_srv_construct_handle(AUDIO_SRC_SRV_PSEUDO_DEVICE_A2DP);
    }
}


void bt_sink_srv_a2dp_destroy_pse_handle(void)
{
    int32_t i = 0;

    for (i = 0; i < BT_SINK_SRV_A2DP_PSEUDO_COUNT + BT_SINK_SRV_MCE_A2DP_PSEUDO_COUNT; ++i) {
        audio_src_srv_destruct_handle(g_a2dp_pse_hd[i].hd);
    }
}


int32_t bt_sink_srv_a2dp_set_volume(uint8_t volume, bt_sink_srv_music_device_t *sp_dev)
{
    bt_sink_srv_music_context_t *cntx = bt_sink_srv_music_get_context();;
    bt_sink_srv_music_device_t *run_dev = cntx->run_dev;
    uint8_t new_vol = bt_sink_srv_avrcp_get_volume_level(volume);
    uint8_t old_vol = cntx->vol_lev;
    int32_t ret = BT_SINK_SRV_MUSIC_ERR_SUCCESS_OK;
    bt_sink_srv_am_id_t ami_ret = 0;

    bt_sink_srv_report_id("[sink][a2dp]set_volume[s]-new_vol: %d, old_vol: %d", 2, new_vol, old_vol);
    bt_sink_srv_assert(sp_dev && "sp_dev is NULL why?");

    ret = bt_sink_srv_music_set_nvdm_data(&(sp_dev->dev_addr), BT_SINK_SRV_MUSIC_DATA_VOLUME, &new_vol);

    if ((old_vol != new_vol) && run_dev && run_dev == sp_dev) {
        cntx->vol_lev = new_vol;
        cntx->last_volume = new_vol;
        if (run_dev->op & BT_SINK_SRV_MUSIC_OP_CODEC_OPEN) {
            ami_ret = bt_sink_srv_ami_audio_set_volume(cntx->a2dp_aid, new_vol, STREAM_OUT);
        } else {
            BT_SINK_SRV_SET_FLAG(run_dev->flag, BT_SINK_SRV_MUSIC_WAIT_SET_VOLUME);
        }
    }

    bt_sink_srv_report_id("[sink][a2dp]set_volume-ami_ret: %d, ret: %d, vol: %d", 3,
                          ami_ret, ret, cntx->vol_lev);
    return ret;
}

int32_t bt_sink_srv_a2dp_add_waitinglist(audio_src_srv_handle_t *handle)
{
    int32_t ret = 0;
    uint8_t idx = 0;
    bt_sink_srv_music_device_t *dev = NULL;
    bt_sink_srv_music_device_list_t *device_list = bt_sink_srv_music_get_played_device_list(true);

    bt_sink_srv_assert(handle);
    if (device_list && device_list->number) {
        for (idx = 0; idx < device_list->number; idx++) {
            dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_AVRCP, (void *)(&(device_list->device_list[idx])));
            if (!dev) {
                dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_A2DP, (void *)(&(device_list->device_list[idx])));
            }

            if (dev) {
                if (dev->handle->flag & AUDIO_SRC_SRV_FLAG_WAITING) {
                    bt_sink_srv_report_id("[sink_a2dp_flow] waiting list exist, dev->handle: 0x%08x, handle: 0x%08x", 2, dev->handle, handle);
                    if (dev->handle != handle) {
                        ret = -2;
                    }
                    break;
                }
            } else {
                ret = -1;
                bt_sink_srv_report_id("[sink_a2dp_flow][error] no device find, handle: 0x%08x", 1, handle);
            }
        }
    } else {
        ret = -1;
        bt_sink_srv_report_id("[sink_a2dp_flow][error] no device connected, handle: 0x%08x", 1, handle);
    }
    if (0 == ret) {
        bt_sink_srv_report_id("[sink_a2dp_flow]add to waiting list, handle: 0x%08x", 1, handle);
        audio_src_srv_add_waiting_list(handle);
    }

    return ret;
}

bt_status_t bt_sink_srv_a2dp_get_codec_parameters(bt_sink_srv_a2dp_basic_config_t *config_data)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_sink_srv_music_device_t *run_dev = ctx->run_dev;
    bt_status_t ret = BT_STATUS_FAIL;
    bt_sink_srv_assert(config_data);
    config_data->status = 0x01;
    if (run_dev) {
        //bt_gap_connection_handle_t gap_hd = bt_sink_srv_cm_get_gap_handle(&(run_dev->dev_addr));

        config_data->latency = avm_direct_get_sink_latency();
        config_data->codec_type = run_dev->codec.type;
        config_data->min_bit_pool = 0xff;
        config_data->max_bit_pool = 0xff;
        config_data->bit_rate = 0xffffffff;
        if (config_data->codec_type == BT_A2DP_CODEC_SBC) {
            config_data->min_bit_pool = run_dev->codec.codec.sbc.min_bitpool;
            config_data->max_bit_pool = run_dev->codec.codec.sbc.max_bitpool;
        } else if (config_data->codec_type == BT_A2DP_CODEC_AAC) {
            bt_a2dp_aac_codec_t *aac_codec = &(run_dev->codec.codec.aac);
            config_data->bit_rate = (aac_codec->br_h << 16 | aac_codec->br_m << 8 | aac_codec->br_l);
        }
        config_data->status = 0x00;
        ret = BT_STATUS_SUCCESS;
    }
    bt_sink_srv_report_id("[sink_a2dp]get_codec_parameters status:0x%02x, 3M:0x%02x, latency:%d, codec_type:0x%02x, min:0x%02x, max:0x%02x, bit_rate:0x%08x", 7,
                          config_data->status, config_data->enable_3M, config_data->latency, config_data->codec_type, config_data->min_bit_pool, config_data->max_bit_pool, config_data->bit_rate);

    return ret;
}

bt_status_t  bt_sink_srv_a2dp_cm_callback_handler(bt_cm_profile_service_handle_t type, void *data)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_status_t status = BT_STATUS_SUCCESS;

    bt_sink_srv_report_id("[a2dp] cm_cb_handler type:0x%02x", 1, type);
    switch (type) {
        case BT_CM_PROFILE_SERVICE_HANDLE_POWER_ON: {
                //firstly init context
                bt_sink_srv_music_init_context();
                //secondary init a2dp profile
                bt_sink_srv_a2dp_init();
                break;
            }
        case BT_CM_PROFILE_SERVICE_HANDLE_POWER_OFF: {
                bt_sink_srv_music_context_t *cntx = bt_sink_srv_music_get_context();
                if (cntx->a2dp_aid != BT_SINK_SRV_INVALID_AID) {
                    if (ctx->run_dev && (ctx->run_dev->flag & BT_SINK_SRV_MUSIC_FLAG_WAIT_AMI_OPEN_CODEC)) {
                        BT_SINK_SRV_SET_FLAG(ctx->context_flag, BT_SINK_SRV_CNTX_FLAG_POWER_OFF);
                    } else {
                        bt_sink_srv_ami_audio_close(cntx->a2dp_aid);
                        cntx->a2dp_aid = BT_SINK_SRV_INVALID_AID;
                    }
                }
                break;
            }
        default:
            break;
    }
    return status;
}