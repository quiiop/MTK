/* Copyright Statement:
 *
 * (C) 2005-2021  MediaTek Inc. All rights reserved.
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

#include "bt_sink_srv_utils.h"
#include "bt_sink_srv_music.h"
#include "bt_sink_srv_avrcp.h"
#include "bt_sink_srv_ami.h"
#include "bt_sink_srv_a2dp.h"
#include "audio_src_srv.h"
#include "math.h"
#include "bt_avm.h"
#include "avm_direct.h"
#include "nvkey.h"
#include "nvkey_id_list.h"
#include "bt_device_manager_internal.h"
#include "bt_sink_srv_state_notify.h"

uint8_t g_alc_enable = 1;    /* for A2DP ALC */
bt_sink_srv_music_context_t g_bt_sink_srv_cntx;
extern hal_audio_status_t hal_audio_write_audio_drift_val(int32_t val);
extern hal_audio_status_t hal_audio_write_audio_anchor_clk(uint32_t val);
extern hal_audio_status_t hal_audio_write_audio_asi_base(uint32_t val);
extern hal_audio_status_t hal_audio_write_audio_asi_cur(uint32_t val);
extern bt_sink_srv_music_device_t *bt_sink_srv_avrcp_get_device(void *param, bt_sink_srv_action_t action);

void bt_sink_srv_music_init(void)
{
    g_alc_enable = 1;
    bt_cm_profile_service_register(BT_CM_PROFILE_SERVICE_A2DP_SINK, (bt_cm_profile_service_handle_callback_t)bt_sink_srv_a2dp_cm_callback_handler);
    bt_cm_profile_service_register(BT_CM_PROFILE_SERVICE_AVRCP, (bt_cm_profile_service_handle_callback_t)bt_sink_srv_avrcp_cm_callback_handler);

    /* Construct A2DP pseudo handle */
    bt_sink_srv_a2dp_create_pse_handle();
}

void bt_sink_srv_music_init_context(void)
{
    int32_t i = 0;
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();

    if (ctx->a2dp_aid != 0) {
        bt_sink_srv_report_id("[sink][music] sink_music a2dp_aid was init :%d", 1, ctx->a2dp_aid);
        return;
    }

    bt_sink_srv_memset(ctx, 0x00, sizeof(bt_sink_srv_music_context_t));
    ctx->state = AUDIO_SRC_SRV_STATE_NONE;

    for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
        // Init state and target state
        ctx->sink_dev[i].state = AUDIO_SRC_SRV_STATE_NONE;
        ctx->sink_dev[i].target_state = AUDIO_SRC_SRV_STATE_NONE;

        /* Init invalid a2dp & avrcp & aws handle */
        ctx->sink_dev[i].a2dp_hd = BT_SINK_SRV_MUSIC_INVALID_HD;
        ctx->sink_dev[i].avrcp_hd = BT_SINK_SRV_MUSIC_INVALID_HD;
        ctx->sink_dev[i].role = BT_A2DP_INVALID_ROLE;
    }
    ctx->a2dp_aid = BT_SINK_SRV_INVALID_AID;
    ctx->vol_lev = AUD_VOL_OUT_LEVEL4;
    ctx->init_sink_latency = BT_SINK_SRV_A2DP_DEFAULT_SINK_LATENCY;
    /* default A2DP interrupt ULL flag are configured as true */
    bt_sink_srv_music_set_interrupt_flag(BT_SINK_SRV_CNTX_FLAG_A2DP_INTER_ULL_BY_AVRCP, true);
}

bt_sink_srv_music_context_t *bt_sink_srv_music_get_context(void)
{
    return &g_bt_sink_srv_cntx;
}

bt_sink_srv_music_device_t *bt_sink_srv_music_get_device(bt_sink_srv_music_device_type_t type, const void *param)
{
    bt_sink_srv_music_device_t *dev = NULL;
    bt_sink_srv_music_context_t *ctx = NULL;
    bt_bd_addr_t *dev_addr = NULL;
    int32_t i = 0;
    uint32_t *p_hd = NULL;
    audio_src_srv_handle_t *pse_hd = NULL;

    ctx = bt_sink_srv_music_get_context();

    switch (type) {
        case BT_SINK_SRV_MUSIC_DEVICE_A2DP_HD: {
                p_hd = (uint32_t *)param;

                for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
                    if (ctx->sink_dev[i].a2dp_hd != BT_SINK_SRV_MUSIC_INVALID_HD
                        && ctx->sink_dev[i].a2dp_hd == *p_hd) {
                        dev = &(ctx->sink_dev[i]);
                        break;
                    }
                }

                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_AVRCP_HD: {
                p_hd = (uint32_t *)param;

                for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
                    if (ctx->sink_dev[i].avrcp_hd != BT_SINK_SRV_MUSIC_INVALID_HD
                        && ctx->sink_dev[i].avrcp_hd == *p_hd) {
                        dev = &(ctx->sink_dev[i]);
                        break;
                    }
                }

                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_ADDR_A2DP: {
                dev_addr = (bt_bd_addr_t *)param;

                for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
                    if ((ctx->sink_dev[i].a2dp_hd != BT_SINK_SRV_MUSIC_INVALID_HD) &&
                        (memcmp(dev_addr, &(ctx->sink_dev[i].dev_addr), sizeof(bt_bd_addr_t)) == 0)) {
                        dev = &(ctx->sink_dev[i]);
                        break;
                    }
                }

                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_ADDR_AVRCP: {
                dev_addr = (bt_bd_addr_t *)param;

                for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
                    if ((ctx->sink_dev[i].avrcp_hd != BT_SINK_SRV_MUSIC_INVALID_HD) &&
                        (memcmp(dev_addr, &(ctx->sink_dev[i].dev_addr), sizeof(bt_bd_addr_t)) == 0)) {
                        dev = &(ctx->sink_dev[i]);
                        break;
                    }
                }

                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_UNUSED: {
                for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
                    if ((ctx->sink_dev[i].a2dp_hd == BT_SINK_SRV_MUSIC_INVALID_HD)
                        && (ctx->sink_dev[i].avrcp_hd == BT_SINK_SRV_MUSIC_INVALID_HD)
                        && (ctx->run_dev != &(ctx->sink_dev[i]))) {
                        dev = &(ctx->sink_dev[i]);
                        ctx->latest_dev = dev;
                        break;
                    }
                }

                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_USED: {
                for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
                    if (ctx->sink_dev[i].a2dp_hd != BT_SINK_SRV_MUSIC_INVALID_HD) {
                        dev = &(ctx->sink_dev[i]);
                        break;
                    }
                }

                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_LATEST: {
                dev = ctx->latest_dev;
                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_INTERRUPT: {
                for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
                    if (ctx->sink_dev[i].flag & BT_SINK_SRV_MUSIC_FLAG_A2DP_INTERRUPT) {
                        dev = &(ctx->sink_dev[i]);
                        break;
                    }
                }

                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_FOCUS: {
                /* find SP dev information */
                /* if there are no streaming dev, the latest conneced dev will be
                the focus dev */
                if (ctx->focus_dev) {
                    dev = ctx->focus_dev;
                } else {
                    dev = ctx->latest_dev;
                }
                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_PSE_HD: {
                pse_hd = (audio_src_srv_handle_t *) param;
                for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
                    if ((ctx->sink_dev[i].handle) &&
                        (ctx->sink_dev[i].handle == pse_hd)) {
                        dev = &(ctx->sink_dev[i]);
                        break;
                    }
                }
                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_SP: {
                for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
                    if ((ctx->sink_dev[i].conn_bit & BT_SINK_SRV_MUSIC_A2DP_CONN_BIT)
                        || (ctx->sink_dev[i].conn_bit & BT_SINK_SRV_MUSIC_AVRCP_CONN_BIT)) {
                        dev = &(ctx->sink_dev[i]);
                        break;
                    }
                }
                break;
            }

        case BT_SINK_SRV_MUSIC_DEVICE_AVRCP_BROWSE_HD: {
                p_hd = (uint32_t *)param;

                for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
                    if (ctx->sink_dev[i].avrcp_browse_hd != BT_SINK_SRV_MUSIC_INVALID_HD
                        && ctx->sink_dev[i].avrcp_browse_hd == *p_hd) {
                        dev = &(ctx->sink_dev[i]);
                        break;
                    }
                }

            }
        default:
            break;
    }

    //if (BT_SINK_SRV_STATE_STREAMING != bt_sink_srv_get_state()) //MTK_Titan: Add to reduce streaming log
    //    bt_sink_srv_report_id("[sink][music]get_dev-dev: 0x%x, type: %d, param: 0x%x\n", 3,
    //                           dev, type, param);

    return dev;
}


void bt_sink_srv_music_reset_device(bt_sink_srv_music_device_t *dev)
{
    bt_sink_srv_music_context_t *ctx = NULL;

    bt_sink_srv_assert(dev);
    bt_sink_srv_memset(dev, 0x00, sizeof(bt_sink_srv_music_device_t));
    dev->state = AUDIO_SRC_SRV_STATE_NONE;
    dev->target_state = AUDIO_SRC_SRV_STATE_NONE;
    dev->role = BT_A2DP_INVALID_ROLE;

    /* Update device */
    ctx = bt_sink_srv_music_get_context();

    ctx->latest_dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_USED, NULL);
    bt_sink_srv_music_set_focus_device(ctx->latest_dev);
}


void bt_sink_srv_music_set_focus_device(bt_sink_srv_music_device_t *dev)
{
    bt_sink_srv_music_context_t *ctx = NULL;

    ctx = bt_sink_srv_music_get_context();
    ctx->focus_dev = dev;
}


void bt_sink_srv_music_update_run_device(bt_sink_srv_music_device_t *dev)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();

    bt_sink_srv_report_id("[sink][music]update_run_device: 0x%08x", 1, dev);
    ctx->run_dev = dev;

    if (dev) {
        bt_sink_srv_music_set_focus_device(dev);
    }
}

void bt_sink_srv_music_drv_play(void *param)
{
    bt_sink_srv_music_device_t *dev = (bt_sink_srv_music_device_t *)param;
    if (AUDIO_SRC_SRV_PSEUDO_DEVICE_A2DP == dev->handle->type) {
        bt_sink_srv_a2dp_drv_play(param);
    } else {
        bt_sink_srv_report_id("[sink][music]drv_play(error)--type: %d", 1, dev->handle->type);
    }
}

void bt_sink_srv_music_drv_stop(bt_sink_srv_music_device_t *dev, uint8_t aid)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_sink_srv_am_media_handle_t *med_hd = NULL;
    int32_t ret = 0;

    if (ctx->run_dev == dev) {
        med_hd = &(dev->med_handle);
        BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_DRV_STOP);
        ret = med_hd->stop(aid);
        bt_sink_srv_report_id("[sink][a2dp]drv_stop-ret: 0x%x", 1, ret);

        if (BT_CODEC_MEDIA_STATUS_OK == ret) {
            /* Remove DRV play flag */
            BT_SINK_SRV_REMOVE_FLAG(dev->op, BT_SINK_SRV_MUSIC_OP_DRV_PLAY);
            /* Remove play flag */
            BT_SINK_SRV_REMOVE_FLAG(dev->op, BT_SINK_SRV_MUSIC_REINIT_ON_PARTNER_LATER_JOIN_FLAG);
        } else {
            /* Error handle */
            bt_sink_srv_report_id("[sink][a2dp]drv_stop(error)--ret: %d", 1, ret);
        }
    }
}

void bt_sink_srv_music_clear_codec(bt_sink_srv_music_device_t *dev, uint8_t aid)
{
    int32_t ret = 0;

    if (dev->op & BT_SINK_SRV_MUSIC_OP_DRV_PLAY  ||
        dev->op & BT_SINK_SRV_MUSIC_OP_PLAY_IND) {
        BT_SINK_SRV_SET_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_DRV_STOP);
        /* Sync DRV stop, or codec close must wait DRV stop done(cost time) */
        bt_sink_srv_music_drv_stop(dev, aid);
    }
    if (!(dev->flag & BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC) // for agent
        && (BT_SINK_SRV_STATE_STREAMING == bt_sink_srv_get_state())) {
        bt_sink_srv_state_reset(BT_SINK_SRV_STATE_STREAMING);
    }

    /* 2. Codec close */
    if (dev->op & BT_SINK_SRV_MUSIC_OP_CODEC_OPEN) {
        ret = bt_sink_srv_ami_audio_stop(aid);
        if (ret != AUD_EXECUTION_SUCCESS) {
            /* Failed close codec */
            bt_sink_srv_report_id("[sink][a2dp]clear_codec(error)--ret: %d", 1, ret);
        } else {
            BT_SINK_SRV_REMOVE_FLAG(dev->op, BT_SINK_SRV_MUSIC_OP_CODEC_OPEN);
        }
    }

    /* 3. Transfer suspend SPK2 */

    /* 4. Clear flag */
    /* Clear wait start gpt timer */
    BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_START_GPT_TIMER);
    /* Clear wait start gpt timeout */
    BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAIT_GPT_TIMEOUT);
}

void bt_sink_srv_music_stop(bt_sink_srv_music_device_t *dev, uint8_t aid)
{
    bt_sink_srv_assert(dev);
    uint32_t default_time_len;
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();

    bt_sink_srv_music_clear_codec(dev, aid);

    if (!(dev->flag & BT_SINK_SRV_MUSIC_FLAG_WAIT_AMI_OPEN_CODEC)) {
        bt_sink_srv_music_update_run_device(NULL);
    }

    default_time_len = bt_gap_get_default_sniff_time_length();
    bt_gap_reset_sniff_timer(default_time_len);
    bt_sink_srv_report_id("[sink][a2dp]to set sniff timer to default:%d", 1, default_time_len);

    /* Clear done */
    bt_sink_srv_music_state_machine_handle(dev, BT_SINK_SRV_MUSIC_EVT_CODEC_CLEAR, NULL);

    if (!(dev->conn_bit & BT_SINK_SRV_MUSIC_A2DP_CONN_BIT)) {
        bt_sink_srv_music_state_machine_handle(dev, BT_SINK_SRV_MUSIC_EVT_UNAVAILABLE, NULL);
    } else {
        bt_sink_srv_music_state_machine_handle(dev, BT_SINK_SRV_MUSIC_EVT_READY, NULL);
    }
    if ((dev->flag & BT_SINK_SRV_MUSIC_FLAG_ADD_WAITING_LIST) && (NULL != audio_src_srv_get_runing_pseudo_device())) {
        bt_sink_srv_a2dp_add_waitinglist(dev->handle);
    }
    BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_ADD_WAITING_LIST);

    if (dev->handle && !(dev->conn_bit) &&
        dev->avrcp_hd == BT_SINK_SRV_MUSIC_INVALID_HD) {
        bt_sink_srv_a2dp_free_pseudo_handle(dev->handle);
        dev->handle = NULL;
        bt_sink_srv_music_reset_device(dev);
        if (ctx->context_flag & BT_SINK_SRV_CNTX_FLAG_POWER_OFF) {
            bt_sink_srv_ami_audio_close(aid);
            ctx->a2dp_aid = BT_SINK_SRV_INVALID_AID;
            BT_SINK_SRV_REMOVE_FLAG(ctx->context_flag, BT_SINK_SRV_CNTX_FLAG_POWER_OFF);
        }
    }

    if (dev->flag & BT_SINK_SRV_MUSIC_NORMAL_STOP_FLAG) {
        BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_NORMAL_STOP_FLAG);
        if (((dev->flag & BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC) && dev->handle)
            || (dev->flag & BT_SINK_SRV_MUSIC_FLAG_WAITING_START)) {
            BT_SINK_SRV_REMOVE_FLAG(dev->flag, BT_SINK_SRV_MUSIC_FLAG_WAITING_START);
            bt_sink_srv_music_state_machine_handle(dev, BT_A2DP_START_STREAMING_IND, NULL);
        }
    } else {
        if (NULL == audio_src_srv_get_runing_pseudo_device()) {
            bt_sink_srv_report_id("[Sink][music] dev: 0x%x, a2dp_status: 0x%x", 2, dev, dev->a2dp_status);
            if ((BT_SINK_SRV_A2DP_STATUS_STREAMING == dev->a2dp_status) && dev->handle) {
                bt_sink_srv_music_state_machine_handle(dev, BT_A2DP_START_STREAMING_IND, NULL);
            }
        }
    }

    bt_sink_srv_report_id("[Sink][music] music_stop", 0);
}

void bt_sink_srv_music_fill_audio_src_callback(audio_src_srv_handle_t *handle)
{
    bt_sink_srv_assert(handle);
    handle->play = bt_sink_srv_music_play_handle;
    handle->stop = bt_sink_srv_music_stop_handle;
    handle->suspend = bt_sink_srv_music_suspend_handle;
    handle->reject = bt_sink_srv_music_reject_handle;
    handle->exception_handle = bt_sink_srv_music_exception_handle;
}

int32_t bt_sink_srv_music_start_gpt_timer(hal_gpt_callback_t callback, void *user_data, uint32_t duration)
{
    hal_gpt_port_t gpt_port = HAL_GPT_MAX_PORT;
    hal_gpt_status_t gpt_ret = HAL_GPT_STATUS_ERROR_PORT_USED;

    if (hal_gpt_init(HAL_GPT_1) == HAL_GPT_STATUS_OK) {
        gpt_port = HAL_GPT_1;
    } else if (hal_gpt_init(HAL_GPT_2) == HAL_GPT_STATUS_OK) {
        gpt_port = HAL_GPT_2;
    }

    if (gpt_port != HAL_GPT_MAX_PORT) {
        hal_gpt_register_callback(gpt_port,
                                  callback, user_data);
        //sink_loc_play_nclk_intra += duration;
        gpt_ret = hal_gpt_start_timer_us(gpt_port, duration - 2000, HAL_GPT_TIMER_TYPE_ONE_SHOT);
    } else {
        bt_sink_srv_report_id("[sink][music]start_gpt_timer(error)--dur: %d", 1, duration);
    }

    bt_sink_srv_report_id("[sink][music]start_gpt_timer--ret: %d", 1, gpt_ret);

    return gpt_port;
}

void bt_sink_srv_music_fill_am_aud_param(bt_sink_srv_am_audio_capability_t  *aud_cap,
                                         bt_a2dp_codec_capability_t *a2dp_cap, bt_a2dp_role_t role, uint16_t a2dp_mtu)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();

    memset(aud_cap, 0x00, sizeof(bt_sink_srv_am_audio_capability_t));
    aud_cap->type = A2DP;
    aud_cap->codec.a2dp_format.a2dp_codec.role = role;
    memcpy(&(aud_cap->codec.a2dp_format.a2dp_codec.codec_cap), a2dp_cap, sizeof(bt_a2dp_codec_capability_t));
    aud_cap->audio_stream_out.audio_device = BT_SINK_SRV_A2DP_OUTPUT_DEVICE;
    aud_cap->audio_stream_out.audio_volume = (bt_sink_srv_am_volume_level_out_t)(ctx->vol_lev);
    ctx->last_volume = ctx->vol_lev;
    bt_sink_srv_report_id("agent open codec init vol is %d", 1, aud_cap->audio_stream_out.audio_volume);
    aud_cap->audio_stream_out.audio_mute = false;
}

void BT_A2DP_MAKE_SBC_CODEC(bt_a2dp_codec_capability_t *codec,
                            bt_a2dp_role_t role,
                            uint8_t min_bit_pool, uint8_t max_bit_pool,
                            uint8_t block_length, uint8_t subband_num,
                            uint8_t alloc_method, uint8_t sample_rate,
                            uint8_t channel_mode)
{
    do {
        codec->type = BT_A2DP_CODEC_SBC;
        codec->sep_type = role;
        codec->length = sizeof(bt_a2dp_sbc_codec_t);
        codec->codec.sbc.channel_mode = (channel_mode & 0x0F);
        codec->codec.sbc.sample_freq = (sample_rate & 0x0F);
        codec->codec.sbc.alloc_method = (alloc_method & 0x03);
        codec->codec.sbc.subbands = (subband_num & 0x03);
        codec->codec.sbc.block_len = (block_length & 0x0F);
        codec->codec.sbc.min_bitpool = (min_bit_pool & 0xFF);
        codec->codec.sbc.max_bitpool = (max_bit_pool & 0xFF);
#ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__
        codec->delay_report = true;
        codec->sec_type = BT_A2DP_CP_SCMS_T;
#endif /* #ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__ */
    } while (0);
}

void BT_A2DP_MAKE_AAC_CODEC(bt_a2dp_codec_capability_t *codec,
                            bt_a2dp_role_t role, bool vbr, uint8_t object_type,
                            uint8_t channels, uint16_t sample_rate,
                            uint32_t bit_rate)
{
    do {
        codec->type = BT_A2DP_CODEC_AAC;
        codec->sep_type = role;
        codec->length = sizeof(bt_a2dp_aac_codec_t);
        codec->codec.aac.object_type = object_type;
        codec->codec.aac.freq_h = ((sample_rate >> 4) & 0xFF);
        codec->codec.aac.reserved = 0x0;
        codec->codec.aac.channels = channels;
        codec->codec.aac.freq_l = (sample_rate & 0x0F);
        codec->codec.aac.br_h = ((bit_rate >> 16) & 0x7F);
        codec->codec.aac.vbr = vbr;
        codec->codec.aac.br_m = ((bit_rate >> 8) & 0xFF);
        codec->codec.aac.br_l = (bit_rate & 0xFF);
#ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__
        codec->delay_report = true;
        codec->sec_type = BT_A2DP_CP_SCMS_T;
#endif /* #ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__ */
    } while (0);
}

void BT_A2DP_MAKE_VENDOR_CODEC(bt_a2dp_codec_capability_t *codec,
                               bt_a2dp_role_t role, uint32_t vendor_id, uint16_t
                               codec_id, uint8_t sample_frequency, uint8_t
                               channel_mode)
{
    bt_a2dp_vendor_codec_t *vendor = &codec->codec.vendor;
    do {
        codec->type = BT_A2DP_CODEC_VENDOR;
        codec->sep_type = role;
        codec->length = 8;
        vendor->vendor_id = vendor_id;
        vendor->codec_id = codec_id;
        vendor->value[0] = 0x3f & sample_frequency;
        vendor->value[1] = 0x07 & channel_mode;
#ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__
        codec->delay_report = true;
        codec->sec_type = BT_A2DP_CP_SCMS_T;
#endif /* #ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__ */
    } while (0);
}

void BT_A2DP_CONVERT_SBC_CODEC(bt_codec_capability_t *dst_codec,
                               bt_a2dp_codec_capability_t *src_codec)
{
    dst_codec->type = BT_A2DP_CODEC_SBC;
    dst_codec->codec.sbc.min_bit_pool = src_codec->codec.sbc.min_bitpool;
    dst_codec->codec.sbc.max_bit_pool = src_codec->codec.sbc.max_bitpool;
    dst_codec->codec.sbc.block_length = src_codec->codec.sbc.block_len;
    dst_codec->codec.sbc.subband_num = src_codec->codec.sbc.subbands;
    dst_codec->codec.sbc.alloc_method = src_codec->codec.sbc.alloc_method;
    dst_codec->codec.sbc.sample_rate = src_codec->codec.sbc.sample_freq;
    dst_codec->codec.sbc.channel_mode = src_codec->codec.sbc.channel_mode;
    bt_sink_srv_report_id("[sink][am][med]CONVERT_SBC--min_pool: %d, max_pool: %d, b_len: %d, sub_num: %d, all_met: %d, samp_rate: %d, ch_m: %d", 7,
                          dst_codec->codec.sbc.min_bit_pool, dst_codec->codec.sbc.max_bit_pool,
                          dst_codec->codec.sbc.block_length, dst_codec->codec.sbc.subband_num,
                          dst_codec->codec.sbc.alloc_method, dst_codec->codec.sbc.sample_rate,
                          dst_codec->codec.sbc.channel_mode);
}

void BT_A2DP_CONVERT_AAC_CODEC(bt_codec_capability_t *dst_codec,
                               bt_a2dp_codec_capability_t *src_codec)
{
    dst_codec->type = BT_A2DP_CODEC_AAC;
    dst_codec->codec.aac.vbr = src_codec->codec.aac.vbr;
    dst_codec->codec.aac.object_type = src_codec->codec.aac.object_type;
    dst_codec->codec.aac.channels = src_codec->codec.aac.channels;
    dst_codec->codec.aac.sample_rate = (src_codec->codec.aac.freq_h << 4) | (src_codec->codec.aac.freq_l);
    dst_codec->codec.aac.bit_rate = (src_codec->codec.aac.br_h << 16) | (src_codec->codec.aac.br_m << 8) | (src_codec->codec.aac.br_l);
    bt_sink_srv_report_id("[sink][am][med]CONVERT_AAC--vbr: %d, object_type: %d, channels: %d, sample_rate: %d, bit_rate: %d", 5,
                          dst_codec->codec.aac.vbr, dst_codec->codec.aac.object_type,
                          dst_codec->codec.aac.channels, dst_codec->codec.aac.sample_rate,
                          dst_codec->codec.aac.bit_rate);
}

uint64_t bt_sink_srv_music_convert_btaddr_to_devid(bt_bd_addr_t *bd_addr)
{
    uint64_t dev_id = 0;
    int32_t i = 0;
    uint8_t addr[16] = {0};

    bt_sink_srv_assert(bd_addr);
    bt_sink_srv_memcpy(addr, bd_addr, sizeof(bt_bd_addr_t));
    for (i = 0; i < BT_BD_ADDR_LEN; ++i) {
        dev_id = ((dev_id << 8) | (addr[i]));
    }

    bt_sink_srv_report_id("[music]convert_btaddr_to_devid--hdev: 0x%x, ldev: 0x%x", 2, (dev_id >> 32 & 0xffffffff), (dev_id & 0xffffffff));

    return dev_id;
}

uint8_t bt_sink_srv_get_vol_local2bt(uint8_t vol, uint8_t local_level, uint8_t bt_level)
{
    return (uint8_t)(floor(vol * bt_level / local_level + 0.5));
}


uint8_t bt_sink_srv_get_vol_bt2local(uint8_t vol, uint8_t local_level, uint8_t bt_level)
{
    return (uint8_t)(floor(vol * local_level / bt_level + 0.5));
}

void bt_sink_srv_music_update_base_parameters_to_dsp(int16_t drift_value, uint32_t nclk, uint32_t asi_base, uint32_t asi_cur)
{
    hal_audio_write_audio_drift_val(drift_value);
    hal_audio_write_audio_anchor_clk(nclk);
    hal_audio_write_audio_asi_cur(asi_cur);
    hal_audio_write_audio_asi_base(asi_base);
    bt_sink_srv_report_id("[music]updte parameters, drift:%d, pta-nclk:0x%08x, asi_base:0x%08x, asi_cur:0x%08x", 4, drift_value, nclk, asi_base, asi_cur);
}

bt_status_t bt_sink_srv_music_set_sink_latency(uint32_t latency_value, bool is_initial_value)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();

    bt_sink_srv_ami_set_a2dp_sink_latency(latency_value);
    if (is_initial_value) {
        ctx->init_sink_latency = latency_value;
    }

    return BT_STATUS_SUCCESS;
}

uint32_t bt_sink_srv_music_get_sink_latency(void)
{
    return bt_sink_srv_ami_get_a2dp_sink_latency();
}

bt_avrcp_operation_id_t bt_sink_srv_music_get_play_pause_action(bt_sink_srv_music_device_t *dev)
{
    bt_avrcp_operation_id_t action_id = 0;
    if (dev->last_play_pause_action == BT_AVRCP_OPERATION_ID_PLAY) {
        action_id = BT_AVRCP_OPERATION_ID_PAUSE;
    } else if (dev->last_play_pause_action == BT_AVRCP_OPERATION_ID_PAUSE) {
        action_id = BT_AVRCP_OPERATION_ID_PLAY;
    } else if (dev->avrcp_status == BT_AVRCP_STATUS_PLAY_PLAYING) {
        action_id = BT_AVRCP_OPERATION_ID_PAUSE;
    } else if (dev->avrcp_status == BT_AVRCP_STATUS_PLAY_PAUSED
               || dev->avrcp_status == BT_AVRCP_STATUS_PLAY_STOPPED) {
        action_id = BT_AVRCP_OPERATION_ID_PLAY;
    } else if (dev->avrcp_status == BT_SINK_SRV_MUSIC_AVRCP_INVALID_STATUS && dev->a2dp_status == BT_SINK_SRV_A2DP_STATUS_STREAMING) {
        action_id = BT_AVRCP_OPERATION_ID_PAUSE;
    } else if (dev->avrcp_status == BT_SINK_SRV_MUSIC_AVRCP_INVALID_STATUS && dev->a2dp_status == BT_SINK_SRV_A2DP_STATUS_SUSPEND) {
        action_id = BT_AVRCP_OPERATION_ID_PLAY;
    } else if ((dev->avrcp_status == BT_AVRCP_STATUS_PLAY_FWD_SEEK || dev->avrcp_status == BT_AVRCP_STATUS_PLAY_REV_SEEK)
               && dev->a2dp_status == BT_SINK_SRV_A2DP_STATUS_STREAMING) {
        action_id = BT_AVRCP_OPERATION_ID_PAUSE;
    } else if ((dev->avrcp_status == BT_AVRCP_STATUS_PLAY_FWD_SEEK || dev->avrcp_status == BT_AVRCP_STATUS_PLAY_REV_SEEK)
               && dev->a2dp_status == BT_SINK_SRV_A2DP_STATUS_SUSPEND) {
        action_id = BT_AVRCP_OPERATION_ID_PLAY;
    }
    return action_id;
}

void bt_sink_srv_music_decide_play_pause_action(bt_sink_srv_music_device_t *dev)
{
    bt_avrcp_operation_id_t action_id = 0;
    bt_status_t ret = BT_STATUS_SUCCESS;
    uint16_t conn_bit = 0;

    if (dev && (dev->conn_bit & BT_SINK_SRV_MUSIC_AVRCP_CONN_BIT)) {
        conn_bit = dev->conn_bit;
        action_id = bt_sink_srv_music_get_play_pause_action(dev);

        if (action_id) {
            ret = bt_sink_srv_avrcp_send_play_pause_command(dev, action_id);
            if (ret == BT_STATUS_SUCCESS) {
                dev->last_wear_action = BT_SINK_SRV_INVALID_LAST_PLAY_PAUSE_ACTION;
                dev->last_play_pause_action = action_id;
            }
        } else {
            bt_sink_srv_assert(0 && "action_id 0");
        }
    }
    bt_sink_srv_report_id("[music]bt_sink_srv_music_decide_play_pause_action, dev:0x%08x, conn_bit:0x%04x, action_id:0x%02x, ret:0x%08x",
                          4, dev, conn_bit, action_id, ret);
}

void bt_sink_srv_music_avrcp_status_change_notify(bt_bd_addr_t *remote_addr, bt_avrcp_status_t avrcp_status)
{
    bt_sink_srv_event_param_t *params = bt_sink_srv_memory_alloc(sizeof(bt_sink_srv_event_param_t));
    bt_sink_srv_assert(params);
    bt_sink_srv_memcpy(&params->avrcp_status_change.address, remote_addr, sizeof(bt_bd_addr_t));
    params->avrcp_status_change.avrcp_status = avrcp_status;

    bt_sink_srv_event_callback(BT_SINK_SRV_EVENT_AVRCP_STATUS_CHANGE, params, sizeof(bt_sink_srv_event_param_t));
    bt_sink_srv_memory_free(params);
}

bt_status_t bt_sink_srv_music_set_mute(bool is_mute)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_sink_srv_am_result_t ret = 0;
    uint8_t aid = 0;

    aid = ctx->a2dp_aid;

    ret = bt_sink_srv_ami_audio_set_mute(aid, is_mute, STREAM_OUT);
    bt_sink_srv_report_id("[music]bt_sink_srv_music_set_mute, run_dev:0x%08x, aid:0x%02x, mute:%d, ret:0x%08x",
                          4, ctx->run_dev, aid, is_mute, ret);

    return BT_STATUS_SUCCESS;
}

bool bt_sink_srv_music_get_a2dp_nvdm_data(bt_bd_addr_t *bt_addr, void *data_p, uint32_t size)
{
    bool result = false;
#ifdef BT_SINK_SRV_A2DP_STORAGE_SIZE
    // Warnning: Due to the task stack limite, record should not increase more than 100 bytes
    bt_device_manager_db_remote_profile_info_t record;
    bt_sink_srv_memset(&record, 0, sizeof(bt_device_manager_db_remote_profile_info_t));
    bt_sink_srv_assert(size <= BT_SINK_SRV_A2DP_STORAGE_SIZE);
    if (NULL != bt_addr && NULL != data_p &&
        BT_STATUS_SUCCESS == bt_device_manager_remote_find_profile_info((*bt_addr), &record)) {
        bt_sink_srv_memcpy(data_p, record.a2dp_info, size);
        result = true;
    }
#endif /* #ifdef BT_SINK_SRV_A2DP_STORAGE_SIZE */
    return result;
}

bool bt_sink_srv_music_set_a2dp_nvdm_data(bt_bd_addr_t *bt_addr, void *data_p, uint32_t size)
{
    bool result = false;
#ifdef BT_SINK_SRV_A2DP_STORAGE_SIZE
    // Warnning: Due to the task stack limite, record should not increase more than 100 bytes
    bt_device_manager_db_remote_profile_info_t record;
    bt_sink_srv_memset(&record, 0, sizeof(bt_device_manager_db_remote_profile_info_t));
    bt_sink_srv_assert(size <= BT_SINK_SRV_A2DP_STORAGE_SIZE);
    if (NULL != bt_addr && NULL != data_p) {
        bt_device_manager_remote_find_profile_info((*bt_addr), &record);
        bt_sink_srv_memcpy(record.a2dp_info, data_p, size >= BT_SINK_SRV_A2DP_STORAGE_SIZE ? BT_SINK_SRV_A2DP_STORAGE_SIZE : size);
        result = bt_device_manager_remote_update_profile_info((*bt_addr), &record);
    }
#endif /* #ifdef BT_SINK_SRV_A2DP_STORAGE_SIZE */
    return result;
}

bt_status_t bt_sink_srv_music_get_nvdm_data(bt_bd_addr_t *remote_addr, bt_sink_srv_music_data_type_t data_type, void *data)
{
    bt_sink_srv_music_stored_data_t dev_db = {0};
    bt_sink_srv_music_device_t *dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_AVRCP, remote_addr);
    bt_status_t ret = BT_STATUS_SUCCESS;

    bt_sink_srv_music_get_a2dp_nvdm_data(remote_addr, &dev_db, sizeof(dev_db));
    switch (data_type) {
        case BT_SINK_SRV_MUSIC_DATA_VOLUME: {
                uint8_t *volume_data = (uint8_t *)data;
                bt_sink_srv_report_id("[sink][music]music_vol:0x%04x", 1, dev_db.music_volume);
                if ((dev_db.music_volume & 0xff00) == BT_SINK_SRV_A2DP_MAGIC_CODE) {
                    /* use storge volume value */
                    *volume_data = (dev_db.music_volume & 0x00ff);
                } else {
                    /* use dedefault volume value and update it */
                    if (dev && dev->volume_change_status) {
                        *volume_data = bt_sink_srv_get_default_volume_level(true);
                    } else {
                        *volume_data = bt_sink_srv_get_default_volume_level(false);
                    }
                    dev_db.music_volume = (BT_SINK_SRV_A2DP_MAGIC_CODE | (*volume_data));
                    bt_sink_srv_music_set_a2dp_nvdm_data(remote_addr, &dev_db, sizeof(dev_db));
                }
                bt_sink_srv_report_id("[sink][music]get_nvdm_data, vol_data:%d", 1, *volume_data);
                break;
            }
        case BT_SINK_SRV_MUSIC_DATA_LOCAL_ASI_FLAG: {
                uint8_t *local_asi_flag_data = (uint8_t *)data;
                *local_asi_flag_data = dev_db.local_asi_flag;
                bt_sink_srv_report_id("[sink][music]get_nvdm_data, local_asi_flag:%d", 1, *local_asi_flag_data);
                break;
            }
        default:
            break;
    }

    //MTK_Titan: To reduce log
    //bt_sink_srv_report_id("[sink][music]get_nvdm_data, data_type:%d, bt_addr:0x%08x, ret:%d, role:%d",
    //    4, data_type, (uint32_t)*remote_addr, ret, role);
    return ret;
}

bt_status_t bt_sink_srv_music_set_nvdm_data(bt_bd_addr_t *remote_addr, bt_sink_srv_music_data_type_t data_type, void *data)
{
    bt_sink_srv_music_stored_data_t dev_db;
    bt_sink_srv_memset(&dev_db, 0, sizeof(bt_sink_srv_music_stored_data_t));
    bt_status_t ret = BT_STATUS_SUCCESS;
    bool get_data_ret = bt_sink_srv_music_get_a2dp_nvdm_data(remote_addr, &dev_db, sizeof(dev_db));

    switch (data_type) {
        case BT_SINK_SRV_MUSIC_DATA_VOLUME: {
                uint8_t *vol_data = (uint8_t *)data;
                dev_db.music_volume = (*vol_data) | BT_SINK_SRV_A2DP_MAGIC_CODE;
                bt_sink_srv_report_id("[sink][music]set_nvdm_data, vol_data:%d", 1, *vol_data);
                break;
            }
        case BT_SINK_SRV_MUSIC_DATA_LOCAL_ASI_FLAG: {
                uint8_t *local_asi_flag_data = (uint8_t *)data;
                dev_db.local_asi_flag = *local_asi_flag_data;
                bt_sink_srv_report_id("[sink][music]set_nvdm_data, local_asi_flag:%d", 1, *local_asi_flag_data);
                break;
            }
    }
    bt_sink_srv_music_set_a2dp_nvdm_data(remote_addr, &dev_db, sizeof(dev_db));
    bt_sink_srv_report_id("[sink][music]set_nvdm_data, data_type:%d, bt_addr:0x%08x, ret:%d, get_data_ret:%d", 4, data_type, (uint32_t)(*remote_addr), ret, get_data_ret);
    return ret;
}

uint32_t bt_sink_srv_get_latency(void)
{
    uint32_t latency_val = 0;
    latency_val = latency_val * 1000;
    bt_sink_srv_report_id("[sink][music]get_music_latency, val:%d", 1, latency_val);
    return latency_val;
}

uint8_t default_bt_sink_srv_get_default_volume_level(bool support_absolute_volume)
{
    uint8_t default_vol = bt_sink_srv_ami_get_a2dp_default_volume_level();
    if (!support_absolute_volume) {
        default_vol = bt_sink_srv_ami_get_a2dp_max_volume_level();
    }
    bt_sink_srv_report_id("[sink][music]default_volume_level, absolute_vol:%d, default_vol:%d",
                          2, support_absolute_volume, default_vol);
    return default_vol;
}

void bt_sink_srv_music_reject_a2dp_1M(void)
{
    extern void bt_avm_reject_a2dp_1M(void);
    bt_avm_reject_a2dp_1M();
}

void bt_sink_srv_music_set_max_bit_pool(uint32_t max_bp)
{
    extern bt_a2dp_codec_capability_t g_bt_sink_srv_a2dp_codec_list[];
    bt_a2dp_codec_capability_t *sbc_codec = &g_bt_sink_srv_a2dp_codec_list[0];
    sbc_codec->codec.sbc.max_bitpool = (max_bp & 0xFF);
}

void bt_sink_srv_get_module_data_list_by_connection_info(uint8_t *data_count, void *data_list,
                                                         bt_sink_srv_profile_type_t module, bt_sink_srv_profile_type_t profile_connection_select)
{
    bt_device_manager_paired_infomation_t *paired_list = (bt_device_manager_paired_infomation_t *)bt_sink_srv_memory_alloc(BT_SINK_SRV_CM_MAX_TRUSTED_DEV * sizeof(bt_device_manager_paired_infomation_t));
    uint32_t dev_count = BT_SINK_SRV_CM_MAX_TRUSTED_DEV;
    bt_sink_srv_assert(paired_list);
    *data_count = 0;
    uint32_t i = 0;
    bt_device_manager_get_paired_list(paired_list, &dev_count);
    for (i = 0; i < dev_count; i++) {
        bt_sink_srv_profile_type_t connection_info = bt_sink_srv_cm_get_connected_profiles(&(paired_list[i].address));
        switch (module) {
            case BT_SINK_SRV_PROFILE_A2DP_SINK: {
                    bt_sink_srv_music_stored_data_t dev_db = { 0, 0, 0, 0 };
                    bt_sink_srv_music_get_a2dp_nvdm_data(&(paired_list[i].address), &dev_db, sizeof(dev_db));
                    uint8_t *temp_addr = (uint8_t *)(&paired_list[i].address);
                    bt_sink_srv_music_data_list *temp_data_list = (bt_sink_srv_music_data_list *)data_list;
                    //bt_sink_srv_report_id("[sink][music]get_data, addr[%d]--0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x, count:%d", 8, i,
                    //    temp_addr[0], temp_addr[1], temp_addr[2], temp_addr[3], temp_addr[4], temp_addr[5], dev_db.play_order_count);
                    //bt_sink_srv_report_id("[sink][music]bt_sink_srv_get_module_data_list_by_connection_info, play_order_count:%d, connect_info:%d", 2,
                    //    dev_db.play_order_count, connection_info);
                    if ((temp_addr[0] == 0) && (temp_addr[1] == 0) && (temp_addr[2] == 0) && (temp_addr[3] == 0) && (temp_addr[4] == 0)) {
                        assert(0 && "unexpected addr");
                    }
                    if (((connection_info & profile_connection_select) == profile_connection_select) && (dev_db.play_order_count > 0)) {
                        bt_sink_srv_memcpy(&(temp_data_list[*data_count].dev_db), &dev_db, sizeof(bt_sink_srv_music_stored_data_t));
                        bt_sink_srv_memcpy(&(temp_data_list[*data_count].remote_addr), &(paired_list[i].address), sizeof(bt_bd_addr_t));
                        (*data_count)++;
                        bt_sink_srv_report_id("[sink][music]find selected dev [%d]--%02x:%02x:%02x:%02x:%02x:%02x, count:%d, connect_info:%d",
                                              9, i, temp_addr[0], temp_addr[1], temp_addr[2], temp_addr[3], temp_addr[4], temp_addr[5],
                                              dev_db.play_order_count, connection_info);

                    }
                    break;
                }
            case BT_SINK_SRV_PROFILE_HFP:
            default:
                break;
        }
    }

    bt_sink_srv_memory_free(paired_list);
    bt_sink_srv_report_id("[sink][music] get_data_list_by_conn_info, dev_cnt:%d, data_cnt:%d", 2, dev_count, *data_count);
}

void bt_sink_srv_order_for_list(void *data_list, bt_sink_srv_profile_type_t module_type, uint8_t data_count)
{
    uint8_t i = 0;
    uint8_t j = 0;
    bt_sink_srv_assert(data_list && data_count && "please check input parameters");
    for (i = data_count; i > 0; i--) {
        for (j = 0; j < i - 1; j++) {
            switch (module_type) {
                case BT_SINK_SRV_PROFILE_A2DP_SINK: {
                        bt_sink_srv_music_data_list *played_data_list = (bt_sink_srv_music_data_list *)data_list;
                        if (played_data_list[j].dev_db.play_order_count < played_data_list[j + 1].dev_db.play_order_count) {
                            bt_sink_srv_music_data_list temp_data;
                            bt_sink_srv_memcpy(&temp_data, &played_data_list[j], sizeof(bt_sink_srv_music_data_list));
                            bt_sink_srv_memcpy(&played_data_list[j], &played_data_list[j + 1], sizeof(bt_sink_srv_music_data_list));
                            bt_sink_srv_memcpy(&played_data_list[j + 1], &temp_data, sizeof(bt_sink_srv_music_data_list));
                        }
                        break;
                    }
                default:
                    break;
            }
        }
    }
}

bt_sink_srv_music_device_list_t *bt_sink_srv_music_get_played_device_list(bool is_connected)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_sink_srv_music_data_list data_list[BT_SINK_SRV_CM_MAX_TRUSTED_DEV];
    uint8_t played_dev_count = 0;
    uint8_t i = 0;
    bt_sink_srv_profile_type_t connected_situation = 0;

    if (is_connected) {
        connected_situation = BT_SINK_SRV_PROFILE_A2DP_SINK | BT_SINK_SRV_PROFILE_AVRCP;
    }

    bt_sink_srv_get_module_data_list_by_connection_info(&played_dev_count, data_list,
                                                        BT_SINK_SRV_PROFILE_A2DP_SINK, connected_situation);
    if (played_dev_count) {
        bt_sink_srv_order_for_list(data_list, BT_SINK_SRV_PROFILE_A2DP_SINK, played_dev_count);

        for (i = 0; i < played_dev_count; i++) {
            //bt_sink_srv_report_id("addr[%d]--0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x, dev_count:%d\r\n", 8, i,
            //    data_list[i].remote_addr[0], data_list[i].remote_addr[1],
            //    data_list[i].remote_addr[2], data_list[i].remote_addr[3],
            //    data_list[i].remote_addr[4], data_list[i].remote_addr[5], data_list[i].dev_db.play_order_count);
            bt_sink_srv_memcpy(&ctx->played_connected_dev.device_list[i], &data_list[i].remote_addr, sizeof(bt_bd_addr_t));
        }
    }
    ctx->played_connected_dev.number = played_dev_count;
    bt_sink_srv_report_id("[sink][music] get_played_dev_list, dev_count:%d", 1, played_dev_count);
    return &ctx->played_connected_dev;
}

void bt_sink_srv_update_last_device(bt_bd_addr_t *remote_addr, bt_sink_srv_profile_type_t module_type)
{
    bt_device_manager_paired_infomation_t *paired_list = (bt_device_manager_paired_infomation_t *)bt_sink_srv_memory_alloc(BT_SINK_SRV_CM_MAX_TRUSTED_DEV * sizeof(bt_device_manager_paired_infomation_t));
    uint32_t dev_count = BT_SINK_SRV_CM_MAX_TRUSTED_DEV;
    bt_sink_srv_assert(paired_list);
    void *newest_data = NULL;
    uint32_t i = 0;

    bt_device_manager_get_paired_list(paired_list, &dev_count);

    if (module_type == BT_SINK_SRV_PROFILE_A2DP_SINK) {
        newest_data = bt_sink_srv_memory_alloc(sizeof(bt_sink_srv_music_data_list));
        if (newest_data == NULL) {
            return;
        }
        bt_sink_srv_memset(newest_data, 0, sizeof(bt_sink_srv_music_data_list));
    }
    //Find out the latest device by play_order_count. (The biggest count is the latest device)
    for (i = 0; i < dev_count; i++) {
        if (module_type == BT_SINK_SRV_PROFILE_A2DP_SINK) {
            bt_sink_srv_music_stored_data_t dev_db;
            bt_sink_srv_music_data_list *temp_data = (bt_sink_srv_music_data_list *)newest_data;
            //get a2dp info from paired list
            bt_sink_srv_music_get_a2dp_nvdm_data(&paired_list[i].address, &dev_db, sizeof(dev_db));
            uint8_t *temp_addr = (uint8_t *)(&paired_list[i].address);
            bt_sink_srv_report_id("[sink][music]addr[%d]--%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x, cnt:%d", 8, i,
                                  temp_addr[0], temp_addr[1], temp_addr[2], temp_addr[3], temp_addr[4], temp_addr[5], dev_db.play_order_count);

            if ((temp_addr[0] == 0) && (temp_addr[1] == 0) && (temp_addr[2] == 0) && (temp_addr[3] == 0) && (temp_addr[4] == 0)) {
                assert(0 && "update unexpected addr");
            }
            if (temp_data->dev_db.play_order_count < dev_db.play_order_count) {
                bt_sink_srv_memcpy(&(temp_data->remote_addr), &paired_list[i].address, sizeof(bt_bd_addr_t));
                bt_sink_srv_memcpy(&(temp_data->dev_db), &dev_db, sizeof(bt_sink_srv_music_stored_data_t));
            }
        }
    }

    if (module_type == BT_SINK_SRV_PROFILE_A2DP_SINK) {
        bt_sink_srv_music_data_list *temp_data = (bt_sink_srv_music_data_list *)newest_data;
        uint8_t *temp_addr = (uint8_t *)(&temp_data->remote_addr);
        if (bt_sink_srv_memcmp(&temp_data->remote_addr, remote_addr, sizeof(bt_bd_addr_t)) != 0) {
            bt_sink_srv_music_stored_data_t dev_db;
            uint8_t *p_remote_addr = (uint8_t *)remote_addr;
            bt_sink_srv_music_get_a2dp_nvdm_data(remote_addr, &dev_db, sizeof(dev_db));
            dev_db.play_order_count = temp_data->dev_db.play_order_count + 1;
            bt_sink_srv_music_set_a2dp_nvdm_data(remote_addr, &dev_db, sizeof(dev_db));
            bt_sink_srv_report_id("[sink][music]replace last dev--%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x, new cnt:%d", 7,
                                  p_remote_addr[0], p_remote_addr[1], p_remote_addr[2],
                                  p_remote_addr[3], p_remote_addr[4], p_remote_addr[5], dev_db.play_order_count);
        } else {
            bt_sink_srv_report_id("[sink][music]current last dev--%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x, cnt:%d", 7,
                                  temp_addr[0], temp_addr[1], temp_addr[2],
                                  temp_addr[3], temp_addr[4], temp_addr[5], temp_data->dev_db.play_order_count);
        }
        bt_sink_srv_memory_free(newest_data);
    }
    bt_sink_srv_memory_free(paired_list);
}

uint32_t bt_sink_srv_music_find_free_timer(uint32_t start_timer_id, uint32_t end_timer_id)
{
    uint32_t i = 0;
    uint32_t timer_ret = 0;
    for (i = start_timer_id; i <= end_timer_id; i++) {
        if (!bt_timer_ext_find(i)) {
            timer_ret = i;
            break;
        }
    }
    bt_sink_srv_report_id("[sink][music] find timer id:0x%08x", 1, timer_ret);
    return timer_ret;
}

void bt_sink_srv_music_set_ALC_enable(uint8_t is_enable)
{
    bt_sink_srv_report_id("[sink][music] bt_sink_srv_music_set_ALC_enable:0x%x", 1, is_enable);
    g_alc_enable = is_enable;
}

uint8_t bt_sink_srv_music_get_ALC_enable(void)
{
    bt_sink_srv_report_id("[sink][music] bt_sink_srv_music_get_ALC_enable:0x%x", 1, g_alc_enable);
    return g_alc_enable;
}

void bt_sink_srv_music_get_waiting_list_devices(uint32_t *device_list, uint32_t *device_count)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    uint32_t count = 0;

    for (int i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
        if (ctx->sink_dev[i].handle && ctx->sink_dev[i].handle->flag & AUDIO_SRC_SRV_FLAG_WAITING) {
            device_list[count] = (uint32_t)(&(ctx->sink_dev[i]));
            count++;
            bt_sink_srv_report_id("[Sink][music] waiting list dev index = %d,addr:%02x:%02x:%02x:%02x:%02x:%02x", 7, i,
                                  ctx->sink_dev[i].dev_addr[5], ctx->sink_dev[i].dev_addr[4], ctx->sink_dev[i].dev_addr[3],
                                  ctx->sink_dev[i].dev_addr[2], ctx->sink_dev[i].dev_addr[1], ctx->sink_dev[i].dev_addr[0]);
        }
    }

    *device_count = count;
}

void bt_sink_srv_music_device_waiting_list_operation(uint32_t *device_list, uint32_t device_count, bool is_add)
{
    bt_sink_srv_music_device_t *device_ptr = NULL;
    uint32_t i = 0;
    for (i = 0; i < device_count; i++) {
        device_ptr = (bt_sink_srv_music_device_t *)(device_list[i]);
        if (is_add) {
            audio_src_srv_add_waiting_list(device_ptr->handle);
        } else {
            audio_src_srv_del_waiting_list(device_ptr->handle);
        }
    }
}

bt_sink_srv_music_device_t *bt_sink_srv_music_get_must_play_flag_dev(void)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_sink_srv_music_device_t *dev = NULL;
    uint32_t i = 0;

    for (i = 0; i < BT_SINK_SRV_MUISC_DEV_COUNT; ++i) {
        if (ctx->sink_dev[i].avrcp_flag & BT_SINK_SRV_AVRCP_MUST_PLAY_RING_TONE_FLAG) {
            dev = &(ctx->sink_dev[i]);
            break;
        }
    }

    return dev;
}

uint8_t bt_sink_srv_get_last_music_volume(void)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    return ctx->last_volume;
}

bt_status_t bt_sink_srv_set_local_volume(uint8_t volume)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_sink_srv_music_device_t *run_dev = ctx->run_dev;
    bt_status_t ret = BT_STATUS_FAIL;
    uint8_t aid = ctx->a2dp_aid;

    if (run_dev && (run_dev->op & BT_SINK_SRV_MUSIC_OP_CODEC_OPEN)) {
        bt_sink_srv_ami_audio_set_volume(aid, volume, STREAM_OUT);
        ctx->last_volume = volume;
        ret = BT_STATUS_SUCCESS;
    }

    bt_sink_srv_report_id("[Sink][music] set local volume : 0x%02x, ret:0x%08x, dev:0x%08x", 3, volume, ret, run_dev);
    return ret;
}

bt_sink_srv_music_playback_state_t bt_sink_srv_get_music_state(void)
{
    bt_sink_srv_music_device_t *dev = bt_sink_srv_avrcp_get_device(NULL, BT_SINK_SRV_ACTION_PLAY_PAUSE);
    bt_sink_srv_music_playback_state_t state = BT_SINK_SRV_MUSIC_PLAYBACK_STATE_NONE;
    bt_avrcp_operation_id_t action_id = 0;
    if (dev) {
        action_id = bt_sink_srv_music_get_play_pause_action(dev);
        if (action_id == BT_AVRCP_OPERATION_ID_PLAY) {
            state = BT_SINK_SRV_MUSIC_PLAYBACK_STATE_STOPPED;
        } else if (action_id == BT_AVRCP_OPERATION_ID_PAUSE) {
            state = BT_SINK_SRV_MUSIC_PLAYBACK_STATE_PLAYING;
        }
    }

    return state;
}

bt_sink_srv_state_t bt_sink_srv_music_get_music_state(bt_bd_addr_t *addr)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();
    bt_sink_srv_music_device_t *run_dev = ctx->run_dev;
    bt_sink_srv_state_t state = BT_SINK_SRV_STATE_NONE;

    bt_sink_srv_music_device_t *dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_AVRCP, (void *)addr);
    if (!dev) {
        dev = bt_sink_srv_music_get_device(BT_SINK_SRV_MUSIC_DEVICE_ADDR_A2DP, (void *)addr);
    }

    if (dev && run_dev && run_dev == dev) {
        state = BT_SINK_SRV_STATE_STREAMING;
    } else {
        state = BT_SINK_SRV_STATE_CONNECTED;
    }

    return state;
}

void bt_sink_srv_music_set_interrupt_flag(uint32_t flagMask, bool val)
{
    bt_sink_srv_music_context_t *ctx = bt_sink_srv_music_get_context();

    if (BT_SINK_SRV_CNTX_FLAG_A2DP_INTER_ULL_BY_AVRCP != flagMask) {
        assert(0 && "unexpect music interrupt flag");
    }
    if (val) {
        BT_SINK_SRV_SET_FLAG(ctx->context_flag, flagMask);
    } else {
        BT_SINK_SRV_REMOVE_FLAG(ctx->context_flag, flagMask);
    }
    bt_sink_srv_report_id("[Sink][music] set a2dp INT flag: 0x%x,mask: 0x%x, val:0x%08x",
                          3, ctx->context_flag, flagMask, val);
}

#if _MSC_VER >= 1500
#pragma comment(linker, "/alternatename:_bt_sink_srv_get_default_volume_level=_default_bt_sink_srv_get_default_volume_level")
#elif defined(__GNUC__) || defined(__ICCARM__) || defined(__CC_ARM)
#pragma weak bt_sink_srv_get_default_volume_level = default_bt_sink_srv_get_default_volume_level
#else /* #if _MSC_VER >= 1500 */
#error "Unsupported Platform"
#endif /* #if _MSC_VER >= 1500 */


