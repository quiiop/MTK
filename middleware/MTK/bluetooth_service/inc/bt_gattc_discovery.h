/* Copyright Statement:
 *
 * (C) 2022  Airoha Technology Corp. All rights reserved.
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

#ifndef __BT_GATTC_DISCOVERY_H__
#define __BT_GATTC_DISCOVERY_H__

/**
 * @addtogroup Bluetooth_Services_Group Bluetooth Services
 * @{
 * @addtogroup BluetoothService GATT Client Discovery Service
 * @{
 * This section provides APIs for different APP module users to discover GATT Services on remote server.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b GATTC                      | GATT Client is the device that initiates commands and requests to the server. |
 *
 * @section bt_gattc_discovery_api_usage How to use this module
 * - Step 1. Mandatory, initialize the GATTC discovery service.
 *  - Sample Code:
 *     @code
 *              bt_gattc_discovery_init();
 *     @endcode
 * - Step 2. Implement discovery event callback function to handle the GATTC discovery events.
 *  - Sample code:
 *     @code
 *             static void demo_discovery_callback(bt_gattc_discovery_event_t *event)
 *             {
 *                 if (event == NULL) {
 *                     APPS_LOG_MSGID_E("discovery_callback, NULL event", 0);
 *                     return;
 *                 }
 *
 *                 if (event->event_type == BT_GATTC_DISCOVERY_EVENT_FAIL) {
 *                     // discovery failed
 *                     APPS_LOG_MSGID_I("discovery_callback, error_code:0x%04x, conn_handle=0x%04x", 2, event->params.error_code, event->conn_handle);
 *                     return;
 *                 } else if (event->event_type == BT_GATTC_DISCOVERY_EVENT_COMPLETE) {
 *                     // discovery completed
 *                     APPS_LOG_MSGID_I("discovery_callback, completed! uuid=0x%04x", 1, event->params.discovered_db->service_uuid.uuid.uuid16);
 *                     APPS_LOG_MSGID_I("discovery_callback, found charateristics num: %d", 1, event->params.discovered_db->char_count_found);
 *                 }
 *
 *                 bt_gattc_discovery_continue(event->conn_handle);
 *             }
 *     @endcode
 * - Step 3. The upper users call this function @ref bt_gattc_discovery_register_service to register the services that they want to discover.
 *  - Sample code:
 *     @code
 *              #define MAX_CHARC_NUM 2
 *              #define MAX_CHARC0_DESCR_NUM 2
 *              #define MAX_CHARC1_DESCR_NUM 3
 *
 *              bt_gattc_discovery_characteristic_t charc[MAX_CHARC_NUM];
 *              bt_gattc_discovery_descriptor_t charc0_descr[MAX_CHARC0_DESCR_NUM];
 *              bt_gattc_discovery_descriptor_t charc1_descr[MAX_CHARC1_DESCR_NUM];
 *              bt_gattc_discovery_service_t service_buffer;
 *              bt_gattc_discovery_user_data_t user_data;
 *
 *              charc[0].descriptor_count = MAX_CHARC0_DESCR_NUM;
 *              charc[0].descriptor = charc0_descr;
 *
 *              charc[1].descriptor_count = MAX_CHARC1_DESCR_NUM;
 *              charc[1].descriptor = charc1_descr;
 *
 *              service_buffer.characteristic_count = MAX_CHARC_NUM;
 *              service_buffer.charateristics = charc;
 *
 *              user_data.uuid.type = BLE_UUID_TYPE_16BIT;
 *              user_data.uuid.uuid.uuid16 = 0xAABB;
 *              user_data.need_cache = FALSE;
 *              user_data.srv_info = &service_buffer;
 *              user_data.handler = demo_discovery_callback;
 *
 *              bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &user_data);
 *     @endcode
 * - Step 3. Use function bt_gattc_discovery_start to trigger start discovery gatt services by uuid.
 *  - Sample code:
 *     @code
 *               bt_gattc_discovery_start(BT_GATTC_DISCOVERY_USER_LE_AUDIO, conn_handle, TRUE);
 *     @endcode
 */

#include "bt_uuid.h"
#include "bt_type.h"
#include "bt_system.h"
#include "bt_gattc.h"
#include "bt_gatt.h"
#include "bt_gap_le.h"
#include "bt_hci.h"
#include "bt_debug.h"

BT_EXTERN_C_BEGIN

/**
 * @defgroup bt_gattc_discovery_define Define
 * @{
 */

/**
 * @brief Define the types of BLE UUID.
 */
typedef uint8_t  ble_uuid_type_t;
#define BLE_UUID_TYPE_UNKNOWN       0x00 /**< Invalid UUID type. */
#define BLE_UUID_TYPE_16BIT         0x01 /**< Bluetooth SIG UUID (16-bit). */
#define BLE_UUID_TYPE_32BIT         0x02 /**< Bluetooth UUID (32-bit). */
#define BLE_UUID_TYPE_128BIT        0x03 /**< Vendor UUID types start at this index (128-bit). */

/**
 * @brief BLE GATTC Discovery status.
 */
typedef int32_t  bt_gattc_discovery_status_t;
#define BT_GATTC_DISCOVERY_STATUS_SUCCESS               0x00   /**< Operation success. */
#define BT_GATTC_DISCOVERY_STATUS_FAIL                 -0x01   /**< Operation fail. */
#define BT_GATTC_DISCOVERY_STATUS_INVALID_PARAMS       -0x02   /**< Input parameters error, like Null Pointer. */
#define BT_GATTC_DISCOVERY_STATUS_BUSY                 -0x03   /**< Busy. */
#define BT_GATTC_DISCOVERY_STATUS_INVALID_STATE        -0x04   /**< Invalid state, operation disallowed in this state. */
#define BT_GATTC_DISCOVERY_STATUS_OUT_OF_MEMORY        -0x05   /**< No Memory for operation. */
#define BT_GATTC_DISCOVERY_STATUS_INVALID_UUID         -0x06   /**< Invalid UUID. */

/**
 *  @brief Define the error code of GATTC discovery service.
 */
typedef int32_t  bt_gattc_discovery_error_t;
#define BT_GATTC_DISCOVERY_ERROR_SERVICE_NOT_FOUND     0x01   /**< Error indicating that the service was not found at the peer.*/
#define BT_GATTC_DISCOVERY_ERROR_INC_SERV_FOUND_FAIL   0x02   /**< Error indicating that finding the included service failed.*/
#define BT_GATTC_DISCOVERY_ERROR_CHAR_FOUND_FAIL       0x03   /**< Error indicating that finding the characteristic failed.*/
#define BT_GATTC_DISCOVERY_ERROR_DESCRIPTOR_FOUND_FAIL 0x04   /**< Error indicating that finding the descriptor failed.*/
#define BT_GATTC_DISCOVERY_ERROR_UNEXPECTED_DISCONNECT 0x05   /**< Error indicating that the service was not found because of disconnection come.*/

/**
 * @brief Define the event type of GATTC discovery.
 */
typedef int32_t  bt_gattc_discovery_result_t;
#define BT_GATTC_DISCOVERY_EVENT_COMPLETE              0x00   /**< Event indicating that the GATT Database discovery is complete. */
#define BT_GATTC_DISCOVERY_EVENT_FAIL                 -0x01   /**< Event indicating that an internal error has occurred in the BT GATTC Discovery module. This could typically be because of the SoftDevice API returning an error code during the DB discover.*/

/**
 * @brief Define the user type of GATTC discovery.
 */
typedef uint8_t bt_gattc_discovery_user_t;
#define BT_GATTC_DISCOVERY_USER_NONE        0x00    /**< The user is none. */
#define BT_GATTC_DISCOVERY_USER_LE_AUDIO    0x01    /**< The le audio service user. */
#define BT_GATTC_DISCOVERY_USER_PRIVACY     0x02    /**< The privacy service user. */
#define BT_GATTC_DISCOVERY_USER_ANCS        0x03    /**< The ANCS service user. */
#define BT_GATTC_DISCOVERY_USER_AIR_SERVICE 0x04    /**< The AIR_SERVICE service user. */


/**
 * @}
 */

/**
 * @defgroup bt_gatt_discovery_struct Struct
 * @{
 */

/**
 *  @brief The type of BLE UUID, encapsulates both 16-bit and 128-bit UUIDs.
 */
typedef struct {
    ble_uuid_type_t     type;  /**< UUID type, see @ref ble_uuid_type_t. If type is @ref BLE_UUID_TYPE_UNKNOWN, the value of uuid is undefined. */
    union {
        uint16_t uuid16;       /**< 16-bit UUID. */
        uint32_t uuid32;       /**< 32-bit UUID. */
        uint8_t uuid[16];      /**< An array to store 128-bit UUID. */
    } uuid;
} ble_uuid_t;

/**
 *  @brief Structure for holding the descriptor during the discovery process.
 */
typedef struct {
    uint16_t         handle;                /**< Descriptor Handle value for this characteristic. */
    ble_uuid_t       descriptor_uuid;       /**< UUID of the descriptor. */
} bt_gattc_discovery_descriptor_t;

/**
 *  @brief Structure for holding information about the characteristic and the descriptors found during
 *         the discovery process.
 */
typedef struct {
    ble_uuid_t                        char_uuid;         /**< UUID of the characteristic. */
    uint16_t                          handle;            /**< Handle of the characteristic. */
    uint16_t                          value_handle;      /**< Handle of the characteristic value. */
    uint8_t                           property;          /**< Property of the characteristic. */
    uint8_t                           descr_count_found; /**< Real found Number of Descriptors of a Characteristic you want to store. */
    uint8_t                           descriptor_count;  /**< User-defined, The Max Number of Descriptors of a Characteristic you want to store. */
    bt_gattc_discovery_descriptor_t   *descriptor;       /**< User-defined global buffer, to save the list of 1 or more descriptor Information. */
} bt_gattc_discovery_characteristic_t;

/**
 *  @brief Structure for holding information about the included service and the characteristics found during
 *         the discovery process.
 */
typedef struct {
    ble_uuid_t                            service_uuid;                 /**< UUID of the service. User needs to configure the UUID of the service to be discovered.*/
    uint16_t                              start_handle;                 /**< Service Handle Range start. */
    uint16_t                              end_handle;                   /**< Service Handle Range end. */
    uint8_t                               multi_instance_count;         /**< The number of instances of that service with the same UUID. */
    uint8_t                               char_count;                   /**< User-defined, The Max Number of characteristics of a service you want to store. */
    uint8_t                               char_count_found;             /**< Real Found Number of characteristics of a service. */
    bt_gattc_discovery_characteristic_t   *charateristics;              /**< User-defined global buffer, to save the list of information related to the characteristics present in the service. */
} bt_gattc_discovery_included_service_t;

/**
 *  @brief Structure for holding information about the service, included service and the characteristics found during
 *         the discovery process.
 */
typedef struct {
    ble_uuid_t                            service_uuid;                 /**< UUID of the service. User needs to configure the UUID of the service to be discovered. */
    uint16_t                              start_handle;                 /**< Service Handle Range start. */
    uint16_t                              end_handle;                   /**< Service Handle Range end. */
    uint8_t                               included_srv_count;           /**< User-defined, The Max Number of included services of a service you want to store. */
    uint8_t                               included_srv_count_found;     /**< Real Found Number of included services of a service. */
    uint8_t                               characteristic_count;         /**< User-defined, The Max Number of characteristics of a service you want to store. */
    uint8_t                               char_count_found;             /**< Real Found Number of characteristics of a service. */
    bt_gattc_discovery_included_service_t *included_service;            /**< User-defined global buffer, to save the list of information related to the included services present in the service. */
    bt_gattc_discovery_characteristic_t   *charateristics;              /**< User-defined global buffer, to save the list of information related to the characteristics present in the service. */
} bt_gattc_discovery_service_t;


/**
 *  @brief Structure containing the event from the DB discovery module to the application.
 */
typedef struct {
    uint16_t                         conn_handle;      /**< Handle of the connection for which this event has occurred. */
    bool                             last_instance;    /**< Flag of whether this discovery result is the last instance or not. */
    bt_gattc_discovery_result_t      event_type;       /**< Type of event. */
    union {
        bt_gattc_discovery_service_t *discovered_db;   /**< Structure containing the information about the GATT Database at the server. This will be filled when the event type is @ref BT_GATTC_DISCOVERY_EVENT_COMPLETE.*/
        bt_gattc_discovery_error_t   error_code;       /**< Error code indicating the type of error which occurred in the DB Discovery module. This will be filled when the event type is @ref BT_GATTC_DISCOVERY_EVENT_FAIL. */
    } params;
} bt_gattc_discovery_event_t;

/**
 *  @brief Define a function to listen and handle the events from the GATTC discovery service.
 */
typedef void (* bt_gattc_discovery_event_handler_t)(bt_gattc_discovery_event_t *event);

/**
 *  @brief Structure containing the services information that need to be registered by user.
 */
typedef struct {
    ble_uuid_t                                  uuid;           /**< The UUID of the service to be discovered at the server. */
    bool                                        need_cache;     /**< Flag of whether the discovered service information needs to be cached. */
    bt_gattc_discovery_service_t                *srv_info;      /**< Pointer to a user-defined structure of the service to store the discovered service information. */
    bt_gattc_discovery_event_handler_t          handler;        /**< Event handler to be called by the GATTC discovery module when any event
                                                                     related to discovery failure of the registered services occurs or all registered services have been discovered.*/
} bt_gattc_discovery_user_data_t;

/**
 * @}
 */

/**
 * @brief   This function is used for upper user to initialize the GATTC discovery service. It is recommended to call this API once during the bootup.
 * @param[in] void.
 * @return    #BT_STATUS_SUCCESS, the operation completed successfully.
 *            #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t bt_gattc_discovery_init(void);

/**
 * @brief   This function is used for upper user to register the services that they want to discover to the GATTC discovery service.
 * @details   The application can use this function to inform which service it is interested in
 *            discovering at the server.
 * @param[in] user               Specifies the application module that the service is registered for.
 * @param[in] user_data          Pointer of the data that contains information about the service to be registered by upper user.
 * @return    #BT_GATTC_DISCOVERY_STATUS_SUCCESS             Operation success.
 *            #BT_GATTC_DISCOVERY_STATUS_INVALID_PARAMS      When a NULL pointer is passed as input.
 *            #BT_GATTC_DISCOVERY_STATUS_INVALID_STATE       If this function is called without calling the @ref bt_gattc_discovery_init.
 *            #BT_GATTC_DISCOVERY_STATUS_OUT_OF_MEMORY       The maximum number of registrations allowed by this module has been reached.
 *            #BT_GATTC_DISCOVERY_STATUS_BUSY                If a discovery is already in progress.
 */
bt_gattc_discovery_status_t bt_gattc_discovery_register_service(bt_gattc_discovery_user_t user, bt_gattc_discovery_user_data_t *user_data);

/**
 * @brief  This function is used to start the GATTC discovery process.
 * @param[in]  user         Application modules that uses the GATTC discovery service.
 * @param[in]  conn_handle  The handle of the connection for which the discovery should be started.
 * @param[in]  refresh      true: do not use the discovery cache and trigger discovery; false: use the discovery cache if already have cache;
 * @return     #BT_GATTC_DISCOVERY_STATUS_SUCCESS         Operation success.
 *             #BT_GATTC_DISCOVERY_STATUS_FAIL            Operation fail.
 *             #BT_GATTC_DISCOVERY_STATUS_INVALID_STATE   If this function is called without calling the @ref bt_gattc_discovery_init, or without calling @ref bt_gattc_discovery_evt_register.
 *             #BT_GATTC_DISCOVERY_STATUS_BUSY            If a discovery is already in progress for the current connection.
 */
bt_gattc_discovery_status_t bt_gattc_discovery_start(bt_gattc_discovery_user_t user, uint16_t conn_handle, bool refresh);

/**
 * @brief   This function is used to continue the discovery procedure after the discovery event has been processed by the upper user.
 * @param[in]  conn_handle  The handle of the connection for which the discovery procedure should be continued.
 */
void bt_gattc_discovery_continue(uint16_t conn_handle);

#endif /* #ifndef __BT_GATTC_DISCOVERY_H__ */
