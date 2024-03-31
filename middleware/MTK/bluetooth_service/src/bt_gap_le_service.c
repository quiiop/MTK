/* Copyright Statement:
 *
 * (C) 2021  MediaTek Inc. All rights reserved.
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


#include "bt_device_manager_le.h"

#include "bt_gap_le_service.h"



/*-----------------
 * Static variables
 *------------------*/



/*-----------------
 * Global variables
 *------------------*/



/*-----------------
 * Macros
 *------------------*/



/*-----------------
 * Functions
 *------------------*/

bt_gap_le_srv_conn_info_t *bt_gap_le_srv_get_conn_info(bt_handle_t handle)
{
    bt_status_t sts = BT_STATUS_FAIL;
    bt_gap_le_connection_information_t conn_info;
    static bt_gap_le_srv_conn_info_t ret_info;

    sts = bt_gap_le_get_connection_information(handle, &conn_info);

    if (sts == BT_STATUS_SUCCESS) {
        ret_info.role = conn_info.role;
        memcpy(&ret_info.local_addr, &conn_info.local_addr, sizeof(bt_addr_t));
        memcpy(&ret_info.peer_addr, &conn_info.peer_addr, sizeof(bt_addr_t));
        return &ret_info;
    } else
        return NULL;
}

bt_gap_le_srv_conn_params_t *bt_gap_le_srv_get_current_conn_params(bt_handle_t handle)
{
    static bt_gap_le_srv_conn_params_t ret_param;
    bt_device_manager_le_connection_param_t *pParam;

    pParam = bt_device_manager_le_get_current_connection_param(handle);

    if (pParam) {
        memset(&ret_param, 0x0, sizeof(bt_gap_le_srv_conn_params_t));
        ret_param.conn_interval = pParam->conn_interval;
        ret_param.conn_latency = pParam->slave_latency;
        ret_param.supervision_timeout = pParam->supervision_timeout;
        return &ret_param;
    } else
        return NULL;
}


bt_status_t bt_gap_le_srv_set_extended_scan(bt_hci_le_set_ext_scan_parameters_t *p_param,
                                            bt_hci_cmd_le_set_extended_scan_enable_t *p_enable,
                                            void *temp)
{
    //Titan: For BMR, so not yet implement.
    return BT_STATUS_SUCCESS;
}

