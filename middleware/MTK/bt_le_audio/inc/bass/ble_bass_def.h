/* Copyright Statement:
 *
 * (C) 2020  MediaTek Inc. All rights reserved.
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

#ifndef __BLE_BASS_DEF_H__
#define __BLE_BASS_DEF_H__

#include "bt_type.h"

/**
 * @brief The BASS service UUID.
 */
#define BT_SIG_UUID16_BASS                                (0x184F)         /**< Broadcast Scan Audio Service. */

/**
 * @brief The BASS UUID type definitions.
 */
#define BLE_BASS_UUID_TYPE_BROADCAST_AUDIO_SCAN_SERVICE              0x00     /**< Broadcast audio scan service. */
#define BLE_BASS_UUID_TYPE_BROADCAST_AUDIO_SCAN_CONTROL_POINT        0x01     /**< Broadcast audio scan control point. */
#define BLE_BASS_UUID_TYPE_BROADCAST_RECEIVE_STATE                   0x02     /**< Broadcast receive state. */
#define BLE_BASS_UUID_TYPE_MAX_NUM                                   0x03     /**< The max number of BASS UUID type.*/
#define BLE_BASS_UUID_TYPE_INVALID                                   0xFF     /**< The invalid BASS UUID type.*/
typedef uint8_t ble_bass_uuid_t;                                              /**< The BASS UUID type.*/

#define BLE_BASS_UUID_TYPE_CHARC_START       BLE_BASS_UUID_TYPE_BROADCAST_AUDIO_SCAN_CONTROL_POINT

/**
 * @brief The BASS max number of characteristics.
 */
#define BLE_BASS_MAX_CHARC_NUMBER            (BLE_BASS_UUID_TYPE_MAX_NUM-1)  /**< The max number of BASS characteristics.*/

typedef struct {
    ble_bass_uuid_t uuid_type;
    uint16_t att_handle;
} ble_bass_attribute_handle_t;

#endif /* __BLE_BASS_DEF_H__ */

