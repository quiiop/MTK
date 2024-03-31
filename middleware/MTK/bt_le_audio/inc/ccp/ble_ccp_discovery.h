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

#ifndef __BLE_CCP_DISCOVERY_H__
#define __BLE_CCP_DISCOVERY_H__

#include "ble_tbs_def.h"

/**
 * @brief The CCP set attribute callback.
 */
typedef void (*ble_ccp_set_attribute_callback_t)(bt_handle_t handle);

/**
 *  @brief This structure defines the attribute handles of TBS.
 */
typedef struct {
    uint16_t uuid;                  /**< UUID of characteristic.*/
    uint16_t value_handle;          /**< The handle of characteristic value.*/
    uint16_t desc_handle;           /**< The handle of descriptor.*/
} ble_ccp_characteristic_t;

/**
* @brief The parameter of #ble_ccp_set_service_attribute.
*/
typedef struct {
    uint16_t start_handle;                          /**< The start attribute handle of the TBS instance. */
    uint16_t end_handle;                            /**< The end attribute handle of the TBS instance. */
    bool is_gtbs;                                   /**< Indicate the service is generic TBS or not. */
    bool is_complete;                               /**< Indicate TBS discovery is complete or not. */
    uint8_t charc_num;                              /**< The characteristic count of the TBS instance. */
    ble_ccp_characteristic_t *charc;                /**< The characteristic information of the TBS instance. */
    ble_ccp_set_attribute_callback_t callback;      /**< The callback is invoked when finish setting TBS attribute. */
} ble_ccp_set_service_attribute_parameter_t;

/**
 * @brief                       This function set the service attribute of TBS.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] params            is the attribute information of TBS services.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_OUT_OF_MEMORY, out of memory.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_ccp_set_service_attribute(bt_handle_t handle, ble_ccp_set_service_attribute_parameter_t *params);

#endif /* __BLE_CCP_DISCOVERY_H__ */

