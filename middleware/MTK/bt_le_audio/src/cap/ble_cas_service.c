/*
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

#include "ble_cas_def.h"
#include "ble_csis_def.h"

#include "bt_gatts.h"

/**
 * @brief The CAS service start handle.
 */
#define CAS_START_HANDLE                               (0x7001)     /**< CAS Start Handle. */
#ifndef AIR_LE_AUDIO_HEADSET_ENABLE
#define CAS_END_HANDLE                                 (0x7002)     /**< CAS End Handle. */
#else
#define CAS_END_HANDLE                                 (0x7001)     /**< CAS End Handle. */
#endif


/************************************************
*   SERVICE TABLE
*************************************************/
BT_GATTS_NEW_PRIMARY_SERVICE_16(ble_cas_primary_service, BT_SIG_UUID16_CAS);

#ifndef AIR_LE_AUDIO_HEADSET_ENABLE
BT_GATTS_NEW_INCLUDED_SERVICE_16(ble_cas_included_service, CSIS_START_HANDLE, CSIS_END_HANDLE, BT_SIG_UUID16_CSIS);
#endif

static const bt_gatts_service_rec_t *ble_cas_service_rec[] = {
    (const bt_gatts_service_rec_t *) &ble_cas_primary_service,
#ifndef AIR_LE_AUDIO_HEADSET_ENABLE
    (const bt_gatts_service_rec_t *) &ble_cas_included_service,
#endif
};


const bt_gatts_service_t ble_cas_service = {
    .starting_handle = CAS_START_HANDLE,   /* 0x7001 */
    .ending_handle = CAS_END_HANDLE,       /* 0x7002 */
    .required_encryption_key_size = 0,
    .records = ble_cas_service_rec
};

