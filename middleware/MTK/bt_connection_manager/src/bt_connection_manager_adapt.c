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

#include "bt_type.h"
#include "bt_device_manager.h"
#include "bt_device_manager_internal.h"
#include "bt_connection_manager_utils.h"
#include "bt_connection_manager.h"
#include "bt_connection_manager_internal.h"



bt_bd_addr_t *bt_sink_srv_cm_last_connected_device()
{
    return bt_cm_get_last_connected_device();
}


static bt_cm_profile_service_mask_t bt_cm_porting_convert_profile_mask(bt_sink_srv_profile_type_t profile)
{
    bt_cm_profile_service_mask_t profile_mask = BT_CM_PROFILE_SERVICE_MASK_NONE;
    if (BT_SINK_SRV_PROFILE_HFP & profile) {
        profile_mask |= BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_HFP);
    }
    if (BT_SINK_SRV_PROFILE_A2DP_SINK & profile) {
        profile_mask |= BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_A2DP_SINK);
    }
    if (BT_SINK_SRV_PROFILE_AVRCP & profile) {
        profile_mask |= BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_AVRCP);
    }
    if (BT_SINK_SRV_PROFILE_PBAPC & profile) {
        profile_mask |= BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_PBAPC);
    }
    if (BT_SINK_SRV_PROFILE_HSP & profile) {
        profile_mask |= BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_HSP);
    }
    return profile_mask;
}

uint32_t bt_sink_srv_cm_get_connected_device(bt_sink_srv_profile_type_t profile, bt_bd_addr_t addr_list[BT_SINK_SRV_CM_MAX_DEVICE_NUMBER])
{
    bt_cm_profile_service_mask_t profile_mask = bt_cm_porting_convert_profile_mask(profile);
    return bt_cm_get_connected_devices(profile_mask, addr_list, 0xFF);
}

uint32_t bt_sink_srv_cm_get_connected_device_list(bt_sink_srv_profile_type_t profile, bt_bd_addr_t *addr_list, uint32_t list_num)
{
    bt_cm_profile_service_mask_t profile_mask = bt_cm_porting_convert_profile_mask(profile);
    return bt_cm_get_connected_devices(profile_mask, addr_list, list_num);
}

bt_sink_srv_profile_type_t bt_sink_srv_cm_get_connected_profiles(bt_bd_addr_t *address)
{
    bt_sink_srv_profile_type_t profile_type = BT_SINK_SRV_PROFILE_NONE;
    bt_cm_profile_service_mask_t profile_mask;

    bt_cm_mutex_lock();
    profile_mask = bt_cm_get_connected_profile_services(*address);
    bt_cm_mutex_unlock();

    bt_cmgr_report_id("[BT_CM][I] profile_mask 0x%X 0x%x", 2, profile_mask, BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_A2DP_SINK));
    if (profile_mask & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_HFP)) {
        profile_type |= BT_SINK_SRV_PROFILE_HFP;
    }
    if (profile_mask & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_A2DP_SINK)) {
        profile_type |= BT_SINK_SRV_PROFILE_A2DP_SINK;
    }
    if (profile_mask & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_AVRCP)) {
        profile_type |= BT_SINK_SRV_PROFILE_AVRCP;
    }
    if (profile_mask & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_PBAPC)) {
        profile_type |= BT_SINK_SRV_PROFILE_PBAPC;
    }
    if (profile_mask & BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_HSP)) {
        profile_type |= BT_SINK_SRV_PROFILE_HSP;
    }
    return profile_type;
}

bt_gap_connection_handle_t bt_sink_srv_cm_get_gap_handle(bt_bd_addr_t *address_p)
{
    return bt_cm_get_gap_handle(*address_p);
}

void bt_sink_srv_cm_profile_status_notify(bt_bd_addr_t *addr,
                                          bt_sink_srv_profile_type_t profile, bt_sink_srv_profile_connection_state_t state, bt_status_t reason)
{
    /* MTK_David:Simplized. Original method need to complete bt_cm_profile_service_status_notify*/
    extern bt_cm_remote_info_update_ind_t g_remote_info;
    bt_cm_mutex_lock();
    bt_cm_remote_device_t *device_p = bt_cm_find_device(BT_CM_FIND_BY_ADDR, addr);
    bt_cm_profile_service_t service = 0;

    switch (profile) {
        case BT_SINK_SRV_PROFILE_A2DP_SINK:
            g_remote_info.update_service = BT_CM_PROFILE_SERVICE_A2DP_SINK;
            service |= BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_A2DP_SINK);
            break;
        case BT_SINK_SRV_PROFILE_AVRCP:
            g_remote_info.update_service = BT_CM_PROFILE_SERVICE_AVRCP;
            service |= BT_CM_PROFILE_SERVICE_MASK(BT_CM_PROFILE_SERVICE_AVRCP);
            break;
        default:
            bt_cmgr_report_id("[BT_CM][I] Not supported profile 0x%X", 1, profile);
            bt_cm_mutex_unlock();
            return;
    }

    g_remote_info.pre_connected_service = g_remote_info.connected_service;

    switch (state) {
        case BT_SINK_SRV_PROFILE_CONNECTION_STATE_DISCONNECTED:
            g_remote_info.connected_service &= ~service;
            if (device_p)
                device_p->link_info.connected_mask &= ~service;
            break;
        case BT_SINK_SRV_PROFILE_CONNECTION_STATE_CONNECTED:
            g_remote_info.connected_service |= service;
            if (device_p)
                device_p->link_info.connected_mask |= service;
            break;
        default:
            break;
    }

    g_remote_info.reason = reason;
    bt_cm_mutex_unlock();
    bt_cm_event_callback(BT_CM_EVENT_REMOTE_INFO_UPDATE, &g_remote_info, sizeof(bt_cm_remote_info_update_ind_t));
    bt_cm_mutex_lock();
    g_remote_info.update_service = 0;
    bt_cm_mutex_unlock();
}

void bt_connection_manager_device_local_info_store_local_address(bt_bd_addr_t *addr)
{
    bt_device_manager_store_local_address(addr);
}

bt_bd_addr_t *bt_connection_manager_device_local_info_get_local_address(void)
{
    return bt_device_manager_get_local_address();
}

void bt_connection_manager_write_scan_enable_mode(bt_gap_scan_mode_t mode)
{
    if (BT_GAP_SCAN_MODE_DISCOVERABLE_ONLY == mode) {
        bt_cm_write_scan_mode(BT_CM_COMMON_TYPE_ENABLE, BT_CM_COMMON_TYPE_DISABLE);
    } else if (BT_GAP_SCAN_MODE_NOT_ACCESSIBLE == mode) {
        bt_cm_write_scan_mode(BT_CM_COMMON_TYPE_DISABLE, BT_CM_COMMON_TYPE_DISABLE);
    } else if (BT_GAP_SCAN_MODE_GENERAL_ACCESSIBLE == mode) {
        bt_cm_write_scan_mode(BT_CM_COMMON_TYPE_ENABLE, BT_CM_COMMON_TYPE_ENABLE);
    } else if (BT_GAP_SCAN_MODE_CONNECTABLE_ONLY) {
        bt_cm_write_scan_mode(BT_CM_COMMON_TYPE_DISABLE, BT_CM_COMMON_TYPE_ENABLE);
    }
}

bt_cm_power_state_t bt_connection_manager_power_get_state(void)
{
    return bt_cm_power_get_state();
}
