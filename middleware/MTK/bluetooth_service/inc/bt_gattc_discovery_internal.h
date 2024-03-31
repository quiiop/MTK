/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#ifndef __BT_GATTC_DISCOVERY_INTERNAL_H__
#define __BT_GATTC_DISCOVERY_INTERNAL_H__

#include "bt_uuid.h"
#include "bt_type.h"
#include "bt_system.h"
#include "bt_gattc.h"
#include "bt_gatt.h"
#include "bt_gap_le.h"
#include "bt_hci.h"
#include "bt_debug.h"
#include "bt_gattc_discovery.h"
#include "bt_utils.h"

BT_EXTERN_C_BEGIN

#define BT_GATTC_DISCOVERY_MAX_SRVS (10)  /**< Maximum number of services supported by this module. This also indicates the maximum number of users allowed to be registered to this module. */

#define BT_GATTC_RETRY_DISCOVERY_MAX_TIMES (10)  /**< Maximum number of re-discovery supported by this module.  */

#define BT_GATTC_DISCOVERY_MAX_SAME_UUID_SRVS (5)  /**< Maximum number of service with same uuid supported by this module. */

// #define BT_GATTC_DISCOVERY_MAX_CONN_HANDLES BT_LE_CONNECTION_NUM  /**< Maximum number of connect handles supported by this module. */

/**
 * @brief Connect handle discovery states.
 */

typedef int32_t bt_gattc_conn_discovery_state_t;
#define BT_GATTC_CONN_DISCOVERY_STATE_NONE                      0x00   /**< No connection was established. >*/
#define BT_GATTC_CONN_DISCOVERY_STATE_IDLE                      0x01   /**< The connection is established and ready for discovery, or has been discovered. >*/
#define BT_GATTC_CONN_DISCOVERY_STATE_READY                     0x02   /**< The connection is alredy started discovery. >*/
#define BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_PRIMERY_SERV    0x03   /**< The connection is running discovery. >*/
#define BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_SERV   0x04   /**< The connection is running discovery. >*/
#define BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_CHARC  0x05   /**< The connection is running discovery. >*/
#define BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_INCLUDED_DESCR  0x06   /**< The connection is running discovery. >*/
#define BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_PRIMERY_CHARC   0x07   /**< The connection is running discovery. >*/
#define BT_GATTC_CONN_DISCOVERY_STATE_DISCOVERY_PRIMERY_DESCR   0x08   /**< The connection is running discovery. >*/
#define BT_GATTC_CONN_DISCOVERY_STATE_PENDING                   0x09
#define BT_GATTC_CONN_DISCOVERY_STATE_INVALID                   0x0A

/**
 *  @brief Structure for holding multiple instances service hanldes found during the discovery process.
 */

typedef struct {
    uint16_t                start_handle;
    uint16_t                end_handle;
} bt_gattc_discovery_service_handles_t;

typedef struct {
    ble_uuid_t                              uuid;                                                   /**< The UUID of multiple instances service found during the discovery process. */
    uint8_t                                 discovering_multi_serv_index;                                          /**< The Number of multiple instance services during the discovery process. */
    uint8_t                                 service_count;                                          /**< The Number of multiple instance services during the discovery process. */
    bt_gattc_discovery_service_handles_t    service_handles[BT_GATTC_DISCOVERY_MAX_SAME_UUID_SRVS]; /**< The buffer of holding multiple instances service hanldes found during the discovery process. */
} bt_gattc_discovery_multiple_instance_t;


typedef struct user_triggered_Node {
    bt_gattc_discovery_user_t  user;
    struct user_triggered_Node *next;
} user_triggered_t;

typedef struct {
    uint32_t all_size;
    uint32_t serv_szie;
    uint32_t chara_size;
    uint32_t desc_size;
    uint32_t inc_serv_size;
    uint32_t inc_chara_size;
    uint32_t inc_desc_size;
} bt_gattc_discovery_buffer_size_t;

/**
 *  @brief Structure for holding multiple instances service information, connection handle and its discovery state found during
 *         the discovery process.
 */

typedef struct {
    uint16_t                                conn_handle;                                            /**< Handle of the connection for which this event has occurred. */
    bt_gattc_conn_discovery_state_t         conn_discovery_state;                                   /**< The state of the connection on which this event occurred. */
    user_triggered_t                        *user_triggered_list;
    uint8_t                                 user_triggered_count;
    uint8_t                                 discovering_serv_index;
    uint8_t                                 discovering_inc_serv_index;
    uint8_t                                 discovering_char_index;
    bt_gattc_discovery_multiple_instance_t  multi_instance;
    bt_gattc_discovery_service_t            *discovery_buffer;
} bt_gattc_discovery_context_t;

/**
*  @brief Array of structures containing information about the registered application modules.
*/
typedef struct {
    ble_uuid_t                          service_uuid;       /**< The UUID of the service for which the application module had registered itself.*/
    bt_gattc_discovery_user_t           user_registered;    /**< Specifies the application module that the uuid is registered for, so that only discovery the service that application registered. */
    bt_gattc_discovery_event_handler_t  event_handler;      /**< The event handler of the application module to be called in case there are any events.*/
    bt_gattc_discovery_service_t        *service_info;      /**< Ther pointer of the global buffer holding information about the descovered service. */
} bt_gattc_registered_handlers;


#endif /* #ifndef __BT_GATTC_DISCOVERY_INTERNAL_H__ */

