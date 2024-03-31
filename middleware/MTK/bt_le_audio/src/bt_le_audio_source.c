/* Copyright Statement:
 *
 * (C) 2019  MediaTek Inc. All rights reserved.
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

#include "bt_le_audio_source.h"

#include "bt_le_audio_util.h"

#include "bt_callback_manager.h"

/**************************************************************************************************
* Define
**************************************************************************************************/

/**************************************************************************************************
* Variable
**************************************************************************************************/

/**************************************************************************************************
* Prototype
**************************************************************************************************/
extern bt_status_t le_audio_source_init(ble_tmap_role_t role, bt_le_audio_source_cb_t callback, uint8_t max_link_num);

extern bt_status_t le_audio_source_deinit(bt_le_audio_source_cb_t callback, uint8_t max_link_num);

extern bt_status_t le_audio_source_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff);

extern void bt_le_audio_source_service_discovery_init(void);
/**************************************************************************************************
* Static function
**************************************************************************************************/

/**************************************************************************************************
* Public function
**************************************************************************************************/
bt_status_t bt_le_audio_source_init(ble_tmap_role_t role, bt_le_audio_source_cb_t callback, uint8_t max_link_num)
{
    le_audio_init_timer();


    le_audio_source_init(role, callback, max_link_num);

	bt_le_audio_source_service_discovery_init();

    /* register callback */
    bt_callback_manager_register_callback(bt_callback_type_app_event,
                                          MODULE_MASK_GAP | MODULE_MASK_GATT | MODULE_MASK_SYSTEM,
                                          (void *)le_audio_source_event_callback);

    return BT_STATUS_SUCCESS;
}

