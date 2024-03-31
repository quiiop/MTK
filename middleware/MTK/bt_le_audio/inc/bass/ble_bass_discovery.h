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
#ifndef __BLE_BASS_DISCOVERY_H__
#define __BLE_BASS_DISCOVERY_H__

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothLeaudio LE_AUDIO
 * @{
 * @addtogroup BluetoothLeAudioBASS BASS
 * @{
 *
 * @section ble_bass_api_disc_usage How to do BASS service discovery setting
 *   - User needs to call #ble_bass_set_service_attribute() when BASS discovery completed.
 *    - Sample code:
 *     @code
 *          static void app_bass_discovery_bass_callback(bt_gattc_discovery_event_t *event)
 *          {
 *              switch (event->event_type) {
 *                  case BT_GATTC_DISCOVERY_EVENT_COMPLETE: {
 *                      ble_bass_set_service_attribute_parameter_t param;
 *                      ble_bass_characteristic_t charc[BLE_BASS_MAX_CHARC_NUMBER];
 *                      bt_gattc_discovery_service_t *p_service = NULL;
 *                      bt_status_t status;
 *                      uint8_t i;
 *
 *                      p_service = &g_bass_srv.service;
 *
 *                      if(p_service == NULL) {
 *                          printf("[APP][BASS] p_service NULL\n");
 *                          return;
 *                      }
 *
 *                      printf("[APP][BASS] discovery_callback, charc_num:%d", p_service->char_count_found);
 *
 *                      for (i = 0; i < p_service->char_count_found; i++) {
 *                          (charc + i)->uuid = p_service->charateristics[i].char_uuid.uuid.uuid16;
 *                          (charc + i)->value_handle = p_service->charateristics[i].value_handle;
 *                          (charc + i)->desc_handle = p_service->charateristics[i].descriptor[0].handle;
 *                      }
 *                      param.start_handle = p_service->start_handle;
 *                      param.end_handle = p_service->end_handle;
 *                      param.callback = app_bass_set_attribute_callback;
 *                      param.charc_num = p_service->char_count_found;
 *                      param.is_complete = event->last_instance;
 *                      param.p_charc = charc;
 *
 *                      status = ble_bass_set_service_attribute(event->conn_handle, &param);
 *                      printf("[APP][BASS] discovery_callback, status:%d charc_num:%d", status, p_service->char_count_found);
 *                      break;
 *                  }
 *              }
 *          }
 *
 *          void app_bass_init(void)
 *          {
 *              uint8_t i;
 *              ble_uuid_t bass_srv_uuid;
 *
 *              for (i = 0; i < BLE_BASS_MAX_CHARC_NUMBER; i++) {
 *                  g_bass_srv.charc[i].descriptor_count = 1;
 *                  g_bass_srv.charc[i].descriptor = &g_bass_srv.descrp[i];
 *              }
 *
 *              bass_srv_uuid.type = BLE_UUID_TYPE_16BIT;
 *              bass_srv_uuid.uuid.uuid16 = BT_GATT_UUID16_COORDINATED_SET_IDENTIFICATION_SERVICE;
 *              g_bass_srv.service.characteristic_count = BLE_BASS_MAX_CHARC_NUMBER;
 *              g_bass_srv.service.charateristics = g_bass_srv.charc;
 *              bt_gattc_discovery_service_register(&bass_srv_uuid, &g_bass_srv.service, app_bass_discovery_bass_callback);
 *          }
 *
 *     @endcode
 */

#include "ble_bass_def.h"
#include "bt_gattc_discovery.h"

/**
 * @defgroup Bluetoothble_BASS_struct Struct
 * @{
 * This section defines basic data structures for the BASS.
 */

/**
 * @brief The BASS set attribute callback.
 */
typedef void (*ble_bass_set_attribute_callback_t)(void);

/**
 *  @brief This structure defines the initial conditions for the service.
 */
typedef struct {
    uint16_t uuid;                  /**< UUID of characteristic.*/
    uint16_t value_handle;          /**< The handle of characteristic value.*/
    uint16_t desc_handle;           /**< The handle of descriptor.*/
} ble_bass_characteristic_t;

/**
* @brief The parameter of #ble_bass_set_service_attribute.
*/
typedef struct {
    uint16_t start_handle;          /**< The start handle of service.*/
    uint16_t end_handle;            /**< The end handle of service.*/
    uint8_t charc_num;              /**< The total count of characteristics in service.*/
    bool is_complete;               /**< Indicates the discovery flow is completed or not. */
    ble_bass_set_attribute_callback_t callback;  /**< The callback is invoked when finish setting BASS attribute. */
    ble_bass_characteristic_t *charc;  /**< The characteristic information of BASS. */
} ble_bass_set_service_attribute_parameter_t;

/**
 * @}
 */

/**
 * @brief                       This function sets the service attribute of BASS.
 * @param[in] event             is the attribute information of BASS.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_OUT_OF_MEMORY, out of memory.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bass_set_service_attribute(bt_gattc_discovery_event_t *event);

/**
 * @}
 * @}
 * @}
 */

#endif  /* __BLE_BASS_DISCOVERY_H__ */

