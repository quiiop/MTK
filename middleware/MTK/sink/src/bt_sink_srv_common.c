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

#include "bt_avm.h"
#include "avm_direct.h"
#include "bt_callback_manager.h"
#include "bt_sink_srv_ami.h"
#include "bt_sink_srv_utils.h"
#include "bt_sink_srv_common.h"
#include "bt_connection_manager_internal.h"
#include "bt_sink_srv_a2dp.h"
#include "bt_sink_srv_avrcp.h"
#include "bt_sink_srv_music.h"

extern bt_status_t bt_sink_srv_call_set_mute(bt_sink_srv_mute_t type, bool mute);

bt_sink_feature_config_t bt_sink_srv_features_config;

void bt_sink_srv_config_features(bt_sink_feature_config_t *features)
{
    bt_sink_srv_memcpy(&bt_sink_srv_features_config, features, sizeof(bt_sink_feature_config_t));
}

const bt_sink_feature_config_t *bt_sink_srv_get_features_config(void)
{
    return &bt_sink_srv_features_config;
}

static bt_status_t bt_sink_srv_avm_common_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
{
    int32_t ret = 0;

    switch (msg) {
            default:
                break;
    }

    return ret;
}

static bt_status_t bt_sink_srv_mm_common_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
{
    int32_t ret = 0;

    switch (msg) {
            case BT_MEMORY_FREE_GARBAGE_IND: {
                break;
            }
            default:
                break;
    }

    return ret;
}

bt_status_t bt_sink_srv_common_callback(bt_msg_type_t msg, bt_status_t status, void *buffer)
{
    bt_status_t result = BT_STATUS_SUCCESS;
    uint32_t moduel = msg & 0xFF000000;
    switch (moduel) {
        case BT_MODULE_GAP:
        case BT_MODULE_SDP:
#ifdef MTK_BT_HFP_ENABLE
            result = bt_sink_srv_hf_gap_callback(msg, status, buffer);
#endif /* #ifdef MTK_BT_HFP_ENABLE */
#ifdef MTK_BT_A2DP_ENABLE
            result = bt_sink_srv_a2dp_common_callback(msg, status, buffer);
#endif
            break;

#ifdef MTK_BT_HFP_ENABLE
        case BT_MODULE_HFP:
        case BT_MODULE_HSP:
            result = bt_sink_srv_call_common_callback(msg, status, buffer);
            break;
#endif /* #ifdef MTK_BT_HFP_ENABLE */

#ifdef MTK_BT_A2DP_ENABLE
        case BT_MODULE_A2DP:
            result = bt_sink_srv_a2dp_common_callback(msg, status, buffer);
            break;
#endif

        case BT_MODULE_AVM:
            result = bt_sink_srv_avm_common_callback(msg, status, buffer);
            break;

#ifdef MTK_BT_AVRCP_ENABLE
        case BT_MODULE_AVRCP:
            result = bt_sink_srv_avrcp_common_callback(msg, status, buffer);
            break;
#endif

#ifdef MTK_BT_PBAP_ENABLE
        case BT_MODULE_PBAPC:
            result = bt_sink_srv_pbapc_common_callback(msg, status, buffer);
            break;
#endif /* #ifdef MTK_BT_PBAP_ENABLE */

        case BT_MODULE_MM:
            result = bt_sink_srv_mm_common_callback(msg, status, buffer);
            break;

#ifdef MTK_BT_HID_ENABLE
        case BT_MODULE_HID:
            bt_sink_srv_report_id("[SINK][COMMON]HID event", 0);
            result = bt_app_hid_event_callback(msg, status, buffer);
            break;
#endif /* #ifdef MTK_BT_HID_ENABLE */
        default:
            bt_sink_srv_report_id("[SINK][COMMON]Unknown Bluetooth MSG:0x%x, status:0x%x", 2, msg, status);
            break;
    }
    return result;
}


bt_status_t bt_sink_srv_set_clock_offset_ptr_to_dsp(const bt_bd_addr_t *address)
{
    bt_sink_srv_assert(address && "Err: address NULL");
    uint32_t handle = bt_sink_srv_cm_get_gap_handle((bt_bd_addr_t *)address);
    if (handle == 0) {
        bt_sink_srv_report_id("[SINK][COMMON]GAP handle is NULL.", 0);
        //bt_sink_srv_assert(0 && "Err: handle NULL");
        return BT_SINK_SRV_STATUS_INVALID_PARAM;
    }

    const void *clk_offset_buf = bt_avm_get_clock_offset_address(handle);
    if (clk_offset_buf == NULL) {
        bt_sink_srv_report_id("[SINK][COMMON]Get clock offset ptr buff is NULL.", 0);
        //bt_sink_srv_assert(0 && "Err: clock_buff ptr NULL");
        return BT_SINK_SRV_STATUS_FAIL;
    }

    bt_sink_srv_ami_set_bt_inf_address((bt_sink_srv_am_bt_audio_param_t)clk_offset_buf);
    bt_sink_srv_report_id("[SINK][COMMON]Set clock offset ptr, handle:0x%08x, buf ptr:0x%08x", 2,
                          handle, clk_offset_buf);
    return BT_STATUS_SUCCESS;
}

void bt_sink_srv_register_callback_init(void)
{
    bt_callback_manager_register_callback(bt_callback_type_app_event,
                                          (uint32_t)(MODULE_MASK_GAP | MODULE_MASK_SYSTEM
#ifdef MTK_BT_HFP_ENABLE
                                                     | MODULE_MASK_HFP | MODULE_MASK_HSP
#endif /* #ifdef MTK_BT_HFP_ENABLE */
#ifdef MTK_BT_AVRCP_ENABLE
                                                     | MODULE_MASK_AVRCP
#endif
#ifdef MTK_BT_A2DP_ENABLE
                                                     | MODULE_MASK_A2DP
#endif
#ifdef MTK_BT_DUO_ENABLE
                                                     | MODULE_MASK_PBAPC | MODULE_MASK_SPP
                                                     | MODULE_MASK_AVM | MODULE_MASK_SDP
#endif
                                                     | MODULE_MASK_MM
                                                    ),
                                          (void *)bt_sink_srv_common_callback);
#ifdef MTK_BT_HFP_ENABLE
    bt_callback_manager_register_callback(bt_callback_type_hfp_get_init_params,
                                          0,
                                          (void *)bt_sink_srv_hf_get_init_params);
#endif /* #ifdef MTK_BT_HFP_ENABLE */
#ifdef MTK_BT_A2DP_ENABLE
    bt_callback_manager_register_callback(bt_callback_type_a2dp_get_init_params,
                                          0,
                                          (void *)bt_sink_srv_a2dp_get_init_params);
#endif
}

uint32_t bt_sink_srv_get_volume(bt_bd_addr_t *bd_addr, bt_sink_srv_volume_type_t type)
{
    uint32_t volume = 0xffffffff;
#ifdef MTK_BT_HFP_ENABLE
    if (type == BT_SINK_SRV_VOLUME_HFP) {
        bt_sink_srv_hf_get_speaker_volume(bd_addr, &volume);
    } else
#endif /* #ifdef MTK_BT_HFP_ENABLE */
        if (type == BT_SINK_SRV_VOLUME_A2DP) {
            bt_sink_srv_a2dp_get_volume(bd_addr, &volume);
        }

    return volume;
}

uint32_t bt_sink_srv_get_device_state(const bt_bd_addr_t *device_address, bt_sink_srv_device_state_t *state_list, uint32_t list_number)
{
    bt_bd_addr_t null_address = {0};
    uint32_t address_number = 0;
    bt_bd_addr_t address_list[BT_SINK_SRV_MAX_DEVICE_NUM] = {{0}};

    if (device_address != NULL) {
        address_number = 1;
        bt_sink_srv_memcpy(&address_list[0], device_address, sizeof(bt_bd_addr_t));
    } else {
        address_number = bt_cm_get_connected_devices(BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_NONE),
                                                     address_list, BT_SINK_SRV_MAX_DEVICE_NUM);
    }

    for (uint32_t i = 0; i < address_number; i++) {
        if (bt_sink_srv_memcmp(&address_list[i], null_address, sizeof(bt_bd_addr_t)) != 0) {
            bt_sink_srv_device_state_t device_state = {0};

            bt_sink_srv_memcpy(&device_state.address, &address_list[i], sizeof(bt_bd_addr_t));
            device_state.music_state = bt_sink_srv_music_get_music_state(&device_state.address);
            //bt_sink_srv_call_get_device_state(&device_state);

            bt_sink_srv_report_id("[Sink][Common]get device state, address:0x%x-%x-%x-%x-%x-%x", 6,
                                  device_state.address[0], device_state.address[1], device_state.address[2],
                                  device_state.address[3], device_state.address[4], device_state.address[5]);

            if (i < list_number) {
                bt_sink_srv_memcpy(&state_list[i], &device_state, sizeof(bt_sink_srv_device_state_t));
                bt_sink_srv_report_id("[Sink][Common]get device state, music:0x%x call:0x%x sco:0x%x", 3,
                                      device_state.music_state, device_state.call_state, device_state.sco_state);
            }
        }
    }

    return (address_number < list_number) ? address_number : list_number;
}

bt_status_t bt_sink_srv_set_mute(bt_sink_srv_mute_t type, bool mute)
{
    bt_status_t status = BT_STATUS_SUCCESS;

    switch (type) {
        case BT_SINK_SRV_MUTE_MICROPHONE: {
            status = bt_sink_srv_call_set_mute(type, mute);
            break;
        }

        case BT_SINK_SRV_MUTE_SPEAKER: {
            const audio_src_srv_handle_t *device = audio_src_srv_get_runing_pseudo_device();
            if (device != NULL) {
                bt_sink_srv_report_id("[Sink]set mute, mute 0x%x speaker", 1, device->type);
                if (device->type == AUDIO_SRC_SRV_PSEUDO_DEVICE_A2DP) {
                    status = bt_sink_srv_music_set_mute(mute);
                } else if (device->type == AUDIO_SRC_SRV_PSEUDO_DEVICE_HFP) {
                    status = bt_sink_srv_call_set_mute(type, mute);
                } else {
                    /* Add more cases here. */
                }
            }
            break;
        }

        default: {
            break;
        }
    }

    bt_sink_srv_report_id("[Sink]set mute, type:%x mute:%d status:0x%x", 3, type, mute, status);
    return status;
}

#if 0
bt_sink_srv_device_info_t g_bt_sink_srv_device_info[3 + 2];
void bt_sink_srv_update_device_info(uint32_t handle, bt_sink_srv_type_t type, uint8_t state)
{
    uint8_t i = 0;
    bt_sink_srv_report_id("[LE_BT][Sink]update_info: handle:0x%x, type:0x%x, state:0x%x", 3, handle, type, state);
    for (; i < 6; i++) {

        if (state == BT_SINK_SRV_STATE_CONNECTED) {
            bt_sink_srv_report_id("[LE_BT][Sink]conn:mask:0x%x, handle:0x%x", 2, g_bt_sink_srv_device_info[i].mask, g_bt_sink_srv_device_info[i].handle);

            if (!g_bt_sink_srv_device_info[i].mask) {
                g_bt_sink_srv_device_info[i].mask = (0x08 | (i + 1));
                g_bt_sink_srv_device_info[i].handle = handle;
                g_bt_sink_srv_device_info[i].type = type;
                break;
            } else if (g_bt_sink_srv_device_info[i].handle == handle) {
                if (g_bt_sink_srv_device_info[i].type == type) {
                    bt_sink_srv_report_id("[LE_BT][Sink]update_info: connect is exist", 0);
                    break;
                }
            }
        } else if (state == BT_SINK_SRV_STATE_POWER_ON) {
            bt_sink_srv_report_id("[LE_BT][Sink]disconn: check mask:0x%x, handle:0x%x", 2, g_bt_sink_srv_device_info[i].mask, g_bt_sink_srv_device_info[i].handle);
            if (g_bt_sink_srv_device_info[i].mask && handle == g_bt_sink_srv_device_info[i].handle) {
                g_bt_sink_srv_device_info[i].mask = 0;
                break;
            }
        } else if (state == BT_SINK_SRV_STATE_STREAMING) {
            bt_sink_srv_report_id("[LE_BT][Sink]operation: check mask:0x%x, handle:0x%x", 2, g_bt_sink_srv_device_info[i].mask, g_bt_sink_srv_device_info[i].handle);
            if (g_bt_sink_srv_device_info[i].mask && handle == g_bt_sink_srv_device_info[i].handle) {
                g_bt_sink_srv_device_info[i].mask |= 0x81;
                break;
            }
        }
    }
    bt_sink_srv_report_id("[LE_BT][Sink]update_info, ret: i:%x, handle: 0x%x, mask: %x", 2, i, handle);
}

bt_sink_srv_device_info_t *bt_sink_srv_get_device_info()
{
    return &g_bt_sink_srv_device_info;
}
#endif /* #if 0 */
