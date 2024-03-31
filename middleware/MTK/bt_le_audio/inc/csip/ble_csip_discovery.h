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
#ifndef __BLE_CSIP_DISCOVERY_H__
#define __BLE_CSIP_DISCOVERY_H__

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup BluetoothLeaudio LE_AUDIO
 * @{
 * @addtogroup BluetoothLeAudioCSIP CSIP
 * @{
 *
 * @section ble_csip_api_disc_usage How to do CSIP service discovery setting
 *   - User needs to call #ble_csip_set_service_attribute() when CSIS discovery completed.
 *    - Sample code:
 *     @code
 *          static void app_csis_discovery_csis_callback(bt_gattc_discovery_event_t *event)
 *          {
 *              switch (event->event_type) {
 *                  case BT_GATTC_DISCOVERY_EVENT_COMPLETE: {
 *                      ble_csip_set_service_attribute_parameter_t param;
 *                      ble_csip_characteristic_t charc[BLE_CSIS_MAX_CHARC_NUMBER];
 *                      bt_gattc_discovery_service_t *p_service = NULL;
 *                      bt_status_t status;
 *                      uint8_t i;
 *
 *                      p_service = &g_csis_srv.service;
 *
 *                      if(p_service == NULL) {
 *                          printf("[APP][CSIS] p_service NULL\n");
 *                          return;
 *                      }
 *
 *                      printf("[APP][CSIS] discovery_callback, charc_num:%d", p_service->char_count_found);
 *
 *                      for (i = 0; i < p_service->char_count_found; i++) {
 *                          (charc + i)->uuid = p_service->charateristics[i].char_uuid.uuid.uuid16;
 *                          (charc + i)->value_handle = p_service->charateristics[i].value_handle;
 *                          (charc + i)->desc_handle = p_service->charateristics[i].descriptor[0].handle;
 *                      }
 *                      param.start_handle = p_service->start_handle;
 *                      param.end_handle = p_service->end_handle;
 *                      param.callback = app_csis_set_attribute_callback;
 *                      param.charc_num = p_service->char_count_found;
 *                      param.is_complete = event->last_instance;
 *                      param.p_charc = charc;
 *
 *                      status = ble_csip_set_service_attribute(event->conn_handle, &param);
 *                      printf("[APP][CSIS] discovery_callback, status:%d charc_num:%d", status, p_service->char_count_found);
 *                      break;
 *                  }
 *              }
 *          }
 *
 *          void app_csis_init(void)
 *          {
 *              uint8_t i;
 *              ble_uuid_t csip_srv_uuid;
 *
 *              for (i = 0; i < BLE_CSIS_MAX_CHARC_NUMBER; i++) {
 *                  g_csis_srv.charc[i].descriptor_count = 1;
 *                  g_csis_srv.charc[i].descriptor = &g_csis_srv.descrp[i];
 *              }
 *
 *              csip_srv_uuid.type = BLE_UUID_TYPE_16BIT;
 *              csip_srv_uuid.uuid.uuid16 = BT_GATT_UUID16_COORDINATED_SET_IDENTIFICATION_SERVICE;
 *              g_csis_srv.service.characteristic_count = BLE_CSIS_MAX_CHARC_NUMBER;
 *              g_csis_srv.service.charateristics = g_csis_srv.charc;
 *              bt_gattc_discovery_service_register(&csip_srv_uuid, &g_csis_srv.service, app_csis_discovery_csis_callback);
 *          }
 *
 *     @endcode
 */

#include "ble_csis_def.h"
#include "bt_gattc_discovery.h"


/**
 * @brief                       This function sets the service attribute of CSIP.
 * @param[in] handle            is the connection handle of CSIS.
 * @param[in] params            is the attribute information of CSIS.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_OUT_OF_MEMORY, out of memory.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csip_set_service_attribute(bt_gattc_discovery_event_t *event);

/**
 * @brief                       This function sets the service attribute of CSIP which is not included in CAS.
 * @param[in] handle            is the connection handle of CSIS.
 * @param[in] params            is the attribute information of CSIS.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_OUT_OF_MEMORY, out of memory.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_csip_set_service_attribute_independent(bt_gattc_discovery_event_t *event);

/**
 * @}
 * @}
 * @}
 */

#endif  /* __BLE_CSIP_DISCOVERY_H__ */

