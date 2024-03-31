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

#ifndef __BLE_CSIS_DEF_H__
#define __BLE_CSIS_DEF_H__

#include "bt_type.h"

/**
 * @brief The CSIS service UUID.
 */
#define BT_SIG_UUID16_CSIS                                          (0x1846)    /**< Coordinated set identification service. */

/**
 * @brief The CSIS service start handle.
 */
#define CSIS_START_HANDLE                               (0x6001)     /**< CSIS Start Handle. */
#define CSIS_END_HANDLE                                 (0x600A)     /**< CSIS End Handle. */

/**
 * @brief The CSIS UUID type definitions.
 */
#define BLE_CSIS_UUID_TYPE_COORDINATED_SET_IDENTIFICATION_SERVICE    0x00    /**< Coordinated set identification service. */
#define BLE_CSIS_UUID_TYPE_SET_IDENTITY_RESOLVING_KEY                0x01    /**< Set Identity Resolving Key. */
#define BLE_CSIS_UUID_TYPE_COORDINATED_SET_SIZE                      0x02    /**< Coordinated Set Size. */
#define BLE_CSIS_UUID_TYPE_SET_MEMBER_LOCK                           0x03    /**< Set Member Lock */
#define BLE_CSIS_UUID_TYPE_SET_MEMBER_RANK                           0x04    /**< Set Member Rank. */
#define BLE_CSIS_UUID_TYPE_MAX_NUM                                   0x05    /**< The max number of CSIS UUID type.*/
#define BLE_CSIS_UUID_TYPE_INVALID                                   0xFF    /**< The invalid CSIS UUID type.*/
typedef uint8_t ble_csis_uuid_t;                                             /**< The CSIS UUID type.*/

#define BLE_CSIS_UUID_TYPE_CHARC_START       BLE_CSIS_UUID_TYPE_SET_IDENTITY_RESOLVING_KEY

/**
 * @brief The CSIS max number of characteristics.
 */
#define BLE_CSIS_MAX_CHARC_NUMBER            (BLE_CSIS_UUID_TYPE_MAX_NUM-1)  /**< The max number of CSIS characteristics.*/

#endif /* __BLE_CSIS_DEF_H__ */

