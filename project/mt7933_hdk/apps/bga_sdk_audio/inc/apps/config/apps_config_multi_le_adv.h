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

/**
 * File: apps_config_multi_le_adv.h
 *
 * Description: This file for user to manager it multi_le_adv instances
 *
 */

#ifndef __APPS_CONFIG_MULTI_LE_ADV_H__
#define __APPS_CONFIG_MULTI_LE_ADV_H__

#include "bt_gap_le.h"


/**
 *  @brief This enum defines the multi_adv instance ID, the module support multi instance BLE adv.
 */
 //Titan: Or we can write an API to autogen this instant ID
typedef enum {
    MULTI_ADV_INST_ID_DEFAULT = 0x10,     /**< The default instance ID for general purpose. Not start from 0 due to ut_app will use 0 as default sample code*/
#ifdef AIR_LE_AUDIO_ENABLE
    MULTI_ADV_INST_ID_LEA,                 /**< Instance ID for LE audio */
#endif
    MULTI_ADV_INST_ID_MAX_COUNT
} multi_adv_instance_t;

#define APPS_MAX_ADV_INST_ID_COUNT  (MULTI_ADV_INST_ID_MAX_COUNT - MULTI_ADV_INST_ID_DEFAULT)


/**
 *  @brief This structure defines the BLE adv information, it include parameter, data and scan response.
 */
typedef struct {
    bt_hci_le_set_ext_advertising_parameters_t *adv_param;  /**< The BLE adv parameter. */
    bt_gap_le_set_ext_advertising_data_t *adv_data;         /**< The BLE adv data. */
    bt_gap_le_set_ext_scan_response_data_t *scan_rsp;       /**< The BLE adv scan response. */
} multi_ble_adv_info_t;


#endif /* __APPS_CONFIG_MULTI_LE_ADV_H__ */
