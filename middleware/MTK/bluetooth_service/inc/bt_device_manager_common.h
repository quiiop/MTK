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

#ifndef __BT_DEVICE_MANAGER_COMMON_H__
#define __BT_DEVICE_MANAGER_COMMON_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bt_type.h"
#include "bt_gap.h"

#ifdef __cplusplus
extern "C"
{
#endif /* #ifdef __cplusplus */

/**
 * @brief     This function initializes the common part of Bluetooth device manager. It's recommended to call this function at the boot up.
 * @return    None.
 */
void bt_device_manager_common_init(void);

/**
 * @brief         This function sets the local bluetooth address.
 * @param[in]     address     is the address of the device.
 * @return         None.
 */
void bt_device_manager_store_local_address(bt_bd_addr_t *address);

/**
 * @brief         This function gets the local bluetooth address.
 * @return     The bluetooth local address.
 */
bt_bd_addr_t *bt_device_manager_get_local_address(void);

/**
 * @brief         This function sets the local bluetooth random address.
 * @param[in]     address     is the random address of the device.
 * @return         None.
 */
void bt_device_manager_store_local_random_address(bt_bd_addr_t *address);

/**
 * @brief         This function gets the local bluetooth random address.
 * @return     The bluetooth local random address.
 */
bt_bd_addr_t *bt_device_manager_get_local_random_address(void);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifndef __BT_DEVICE_MANAGER_COMMON_H__ */

