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

#ifndef __BT_GAP_LE_SERVICE_H__
#define __BT_GAP_LE_SERVICE_H__

#include "bt_system.h"
#include "bt_type.h"
#include "bt_gap_le.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
* @defgroup BT_GAP_LE_SERVICE Define
* @{
* Define bt gap le service data types and values.
*/

#define BT_ROLE_MASTER                  0x00 /**< Master or Central. */
#define BT_ROLE_SLAVE                   0x01 /**< Slave or Peripheral. */
typedef uint8_t bt_role_t;  /**< Define the role type. */

/**
 *  @brief LE connection infomation structure.
 */
typedef struct {
    bt_role_t         role;            /**< The role of local device. */
    bt_addr_t         local_addr;      /**< The LE address of local device. */
    bt_addr_t         peer_addr;       /**< The LE address of peer device. */
} bt_gap_le_srv_conn_info_t;

typedef struct {
    uint16_t conn_interval;
    uint16_t conn_latency;
    uint16_t supervision_timeout;
} bt_gap_le_srv_conn_params_t;

bt_gap_le_srv_conn_info_t *bt_gap_le_srv_get_conn_info(bt_handle_t handle);

bt_gap_le_srv_conn_params_t *bt_gap_le_srv_get_current_conn_params(bt_handle_t handle);

bt_status_t bt_gap_le_srv_set_extended_scan(bt_hci_le_set_ext_scan_parameters_t *p_param,
                                            bt_hci_cmd_le_set_extended_scan_enable_t *p_enable,
                                            void *temp);


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
/**
 * @}
 * @}
 * @}
 */

#endif /* #ifndef __BT_GAP_LE_SERVICE_H__ */
