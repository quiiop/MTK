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


#include "ble_csip_discovery.h"
#include "ble_vcp_discovery.h"
#include "ble_bap_discovery.h"
#include "ble_bass_discovery.h"
#include "ble_micp_discovery.h"
#include "ble_tmap_discovery.h"
#include "ble_cas_def.h"
#include "bt_gattc_discovery.h"
#include "bt_le_audio_msglog.h"


/**************************************************************************************************
* Define
**************************************************************************************************/
/* BLE_CSIS_MAX_CHARC_NUMBER    : 4 */
/* BLE_VCS_MAX_CHARC_NUMBER     : 3 */
/* BLE_PACS_MAX_CHARC_NUMBER    : 6 */
/* BLE_ASCS_MAX_CHARC_NUMBER    : 4 */
/* BLE_MICS_MAX_CHARC_NUMBER    : 1 */
#define APP_LE_ASCS_MAX_CHARC_NUMBER        6
#define APP_LE_AIR_SRV_MAX_CHARC_NUMBER     3
#define BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER   6   /* Must be the MAX charc num of the services to be discovered */
#define BT_LE_AUDIO_SERVICE_DISCOVERY_DESCRIPTOR_MAX_NUMBER BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER


/**************************************************************************************************
* Variable
**************************************************************************************************/
static bt_gattc_discovery_characteristic_t g_le_audio_service_discovery_character[BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER];
static bt_gattc_discovery_descriptor_t g_le_audio_service_discovery_descriptor[BT_LE_AUDIO_SERVICE_DISCOVERY_DESCRIPTOR_MAX_NUMBER];

static bt_gattc_discovery_characteristic_t g_le_audio_include_service_discovery_character[BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER];
static bt_gattc_discovery_descriptor_t g_le_audio_include_service_discovery_descriptor[BT_LE_AUDIO_SERVICE_DISCOVERY_DESCRIPTOR_MAX_NUMBER];

static bt_gattc_discovery_included_service_t g_le_audio_include_service_discovery_csis;
static bt_gattc_discovery_service_t g_le_audio_service_discovery_cas;
static bt_gattc_discovery_service_t g_le_audio_service_discovery_pacs;
static bt_gattc_discovery_service_t g_le_audio_service_discovery_ascs;
static bt_gattc_discovery_service_t g_le_audio_service_discovery_vcs;
static bt_gattc_discovery_service_t g_le_audio_service_discovery_mics;
static bt_gattc_discovery_service_t g_le_audio_service_discovery_tmas;
/* for PTS CSIP profile */
static bt_gattc_discovery_service_t g_le_audio_service_discovery_csis;
bool g_le_audio_service_discovery_csis_include_in_cas = true;
static bt_gattc_discovery_service_t g_le_audio_service_discovery_bass;

/**************************************************************************************************
* Prototype
**************************************************************************************************/

/**************************************************************************************************
* Static function
**************************************************************************************************/
static bool bt_le_audio_service_discovery_validate_event(bt_gattc_discovery_event_t *event,
                                                                    bt_gattc_discovery_service_t *service,
                                                                    bt_gattc_discovery_included_service_t *include_service)
{
    LE_AUDIO_MSGLOG_I("[SERVICE DISCOVERY] validate_event event:0x%x conn_handle:0x%x event_type:0x%x discovered_db:0x%x", 4,
        event,
        event ? event->conn_handle : 0,
        event ? event->event_type : 0xFF,
        event ? event->params.discovered_db : NULL);

    if (NULL == event ||
         NULL == service ||
        (BT_GATTC_DISCOVERY_EVENT_COMPLETE == event->event_type &&
         ((event->params.discovered_db != service ||
           event->params.discovered_db->charateristics != g_le_audio_service_discovery_character) ||
          (event->params.discovered_db->included_service != include_service ||
           (include_service &&
            event->params.discovered_db->included_service->charateristics != g_le_audio_include_service_discovery_character))))) {
        return FALSE;
    }

    return TRUE;
}

static void bt_le_audio_service_discovery_csis_callback(bt_gattc_discovery_event_t *event)
{
    bool ret = bt_le_audio_service_discovery_validate_event(event, &g_le_audio_service_discovery_csis, NULL);

    if (ret) {
        ble_csip_set_service_attribute_independent(event);
    }
}

/* Process the included_service part of the event */
static void bt_le_audio_include_service_discovery_csis_callback(bt_gattc_discovery_event_t *event)
{
    ble_csip_set_service_attribute(event);
}

static void bt_le_audio_service_discovery_cas_callback(bt_gattc_discovery_event_t *event)
{
    bool ret = bt_le_audio_service_discovery_validate_event(event, &g_le_audio_service_discovery_cas, &g_le_audio_include_service_discovery_csis);

    if (ret) {
        /* There shall be no more than one CAS instance. */
        if (!event->last_instance) {
            // TODO: disconnect LE link when there is more than one CAS instance?
            LE_AUDIO_MSGLOG_I("[SERVICE DISCOVERY] CAS last_instance:0x%x", 1, event->last_instance);
            return;
        }

        if (true == g_le_audio_service_discovery_csis_include_in_cas) {
            bt_le_audio_include_service_discovery_csis_callback(event);
        } else {
            bt_gattc_discovery_continue(BT_HANDLE_INVALID);
        }
    }
}


static void bt_le_audio_service_discovery_vcs_callback(bt_gattc_discovery_event_t *event)
{
    bool ret = bt_le_audio_service_discovery_validate_event(event, &g_le_audio_service_discovery_vcs, NULL);

    if (ret) {
        ble_vcp_set_service_attribute(event, BLE_VCP_VCS);
    }
}

static void bt_le_audio_service_discovery_bass_callback(bt_gattc_discovery_event_t *event)
{
    bool ret = bt_le_audio_service_discovery_validate_event(event, &g_le_audio_service_discovery_bass, NULL);

    if (ret) {
        ble_bass_set_service_attribute(event);
    }
}

static void bt_le_audio_service_discovery_pacs_callback(bt_gattc_discovery_event_t *event)
{
    bool ret = bt_le_audio_service_discovery_validate_event(event, &g_le_audio_service_discovery_pacs, NULL);

    if (ret) {
        ble_bap_set_service_attribute(event, BT_GATT_UUID16_PACS_SERVICE);
    }
}


static void bt_le_audio_service_discovery_ascs_callback(bt_gattc_discovery_event_t *event)
{
    bool ret = bt_le_audio_service_discovery_validate_event(event, &g_le_audio_service_discovery_ascs, NULL);

    if (ret) {
        ble_bap_set_service_attribute(event, BT_GATT_UUID16_ASCS_SERVICE);
    }
}


static void bt_le_audio_mics_discovery_callback(bt_gattc_discovery_event_t *event)
{
    bool ret = bt_le_audio_service_discovery_validate_event(event, &g_le_audio_service_discovery_mics, NULL);

    if (ret) {
        ble_micp_set_service_attribute(event);
    }
}

static void bt_le_audio_tmas_discovery_callback(bt_gattc_discovery_event_t *event)
{
    bool ret = bt_le_audio_service_discovery_validate_event(event, &g_le_audio_service_discovery_tmas, NULL);

    if (ret) {
        ble_tmap_set_service_attribute(event);
    }
}


/**************************************************************************************************
* Public function
**************************************************************************************************/
void bt_le_audio_source_service_discovery_register_csis(void) /* for PTS CSIP profile */
{
    bt_gattc_discovery_user_data_t user_data;
    bt_gattc_discovery_status_t ret = BT_GATTC_DISCOVERY_STATUS_FAIL;

    g_le_audio_service_discovery_csis_include_in_cas = false;
    g_le_audio_service_discovery_cas.included_srv_count = 0;
    g_le_audio_service_discovery_cas.included_service = NULL;

    /* Register CSIS discovery */
    g_le_audio_service_discovery_csis.characteristic_count = BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER;
    g_le_audio_service_discovery_csis.charateristics = g_le_audio_service_discovery_character;

    user_data.uuid.type = BLE_UUID_TYPE_16BIT;
    user_data.uuid.uuid.uuid16 = BT_SIG_UUID16_CSIS;
    user_data.need_cache = TRUE;
    user_data.srv_info = &g_le_audio_service_discovery_csis;
    user_data.handler = bt_le_audio_service_discovery_csis_callback;
    ret = bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &user_data);
    if (BT_GATTC_DISCOVERY_STATUS_SUCCESS != ret) {
        LE_AUDIO_MSGLOG_I("[SERVICE DISCOVERY] register CSIS discovery fail, ret:%x", 1, ret);
    }
}

void bt_le_audio_source_service_discovery_init(void)
{
    uint32_t i;
    bt_gattc_discovery_user_data_t user_data;
    bt_gattc_discovery_status_t ret = BT_GATTC_DISCOVERY_STATUS_FAIL;

    for (i = 0; i < BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER; i++) {
        g_le_audio_service_discovery_character[i].descriptor_count = 1;
        g_le_audio_service_discovery_character[i].descriptor = &g_le_audio_service_discovery_descriptor[i];

        g_le_audio_include_service_discovery_character[i].descriptor_count = 1;
        g_le_audio_include_service_discovery_character[i].descriptor = &g_le_audio_include_service_discovery_descriptor[i];
    }

    /* Register CAS discovery */
    g_le_audio_service_discovery_cas.characteristic_count = BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER;
    g_le_audio_service_discovery_cas.charateristics = g_le_audio_service_discovery_character;

    g_le_audio_include_service_discovery_csis.service_uuid.type = BLE_UUID_TYPE_16BIT;
    g_le_audio_include_service_discovery_csis.service_uuid.uuid.uuid16 = BT_SIG_UUID16_CSIS;
    g_le_audio_include_service_discovery_csis.char_count = BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER;
    g_le_audio_include_service_discovery_csis.charateristics = g_le_audio_include_service_discovery_character;
    g_le_audio_service_discovery_cas.included_srv_count = 1;
    g_le_audio_service_discovery_cas.included_service = &g_le_audio_include_service_discovery_csis;

    user_data.uuid.type = BLE_UUID_TYPE_16BIT;
    user_data.uuid.uuid.uuid16 = BT_SIG_UUID16_CAS;
    user_data.need_cache = TRUE;
    user_data.srv_info = &g_le_audio_service_discovery_cas;
    user_data.handler = bt_le_audio_service_discovery_cas_callback;
    ret = bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &user_data);
    if (BT_GATTC_DISCOVERY_STATUS_SUCCESS != ret) {
        LE_AUDIO_MSGLOG_I("[SERVICE DISCOVERY] register CAS discovery fail, ret:%x", 1, ret);
    }

    /* Register MICS discovery */
    g_le_audio_service_discovery_mics.characteristic_count = BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER;
    g_le_audio_service_discovery_mics.charateristics = g_le_audio_service_discovery_character;

    user_data.uuid.type = BLE_UUID_TYPE_16BIT;
    user_data.uuid.uuid.uuid16 = BT_SIG_UUID16_MICS;
    user_data.need_cache = TRUE;
    user_data.srv_info = &g_le_audio_service_discovery_mics;
    user_data.handler = bt_le_audio_mics_discovery_callback;
    ret = bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &user_data);
    if (BT_GATTC_DISCOVERY_STATUS_SUCCESS != ret) {
        LE_AUDIO_MSGLOG_I("[SERVICE DISCOVERY] register MICS discovery fail, ret:%x", 1, ret);
    }

    /* Register VCS discovery */
    g_le_audio_service_discovery_vcs.characteristic_count = BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER;
    g_le_audio_service_discovery_vcs.charateristics = g_le_audio_service_discovery_character;

    user_data.uuid.type = BLE_UUID_TYPE_16BIT;
    user_data.uuid.uuid.uuid16 = BT_SIG_UUID16_VOLUME_CONTROL_SERVICE;
    user_data.need_cache = TRUE;
    user_data.srv_info = &g_le_audio_service_discovery_vcs;
    user_data.handler = bt_le_audio_service_discovery_vcs_callback;
    ret = bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &user_data);
    if (BT_GATTC_DISCOVERY_STATUS_SUCCESS != ret) {
        LE_AUDIO_MSGLOG_I("[SERVICE DISCOVERY] register VCS discovery fail, ret:%x", 1, ret);
    }

    /* Register TMAS discovery */
    g_le_audio_service_discovery_tmas.characteristic_count = BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER;
    g_le_audio_service_discovery_tmas.charateristics = g_le_audio_service_discovery_character;

    user_data.uuid.type = BLE_UUID_TYPE_16BIT;
    user_data.uuid.uuid.uuid16 = BT_SIG_UUID16_TMAS;
    user_data.need_cache = TRUE;
    user_data.srv_info = &g_le_audio_service_discovery_tmas;
    user_data.handler = bt_le_audio_tmas_discovery_callback;
    ret = bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &user_data);
    if (BT_GATTC_DISCOVERY_STATUS_SUCCESS != ret) {
        LE_AUDIO_MSGLOG_I("[SERVICE DISCOVERY] register TMAS discovery fail, ret:%x", 1, ret);
    }

    /* Register BASS discovery */
    g_le_audio_service_discovery_bass.characteristic_count = BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER;
    g_le_audio_service_discovery_bass.charateristics = g_le_audio_service_discovery_character;

    user_data.uuid.type = BLE_UUID_TYPE_16BIT;
    user_data.uuid.uuid.uuid16 = BT_SIG_UUID16_BASS;
    user_data.need_cache = TRUE;
    user_data.srv_info = &g_le_audio_service_discovery_bass;
    user_data.handler = bt_le_audio_service_discovery_bass_callback;
    ret = bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &user_data);
    if (BT_GATTC_DISCOVERY_STATUS_SUCCESS != ret) {
        LE_AUDIO_MSGLOG_I("[SERVICE DISCOVERY] register BASS discovery fail, ret:%x", 1, ret);
    }

    /* Register PACS discovery */
    g_le_audio_service_discovery_pacs.characteristic_count = BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER;
    g_le_audio_service_discovery_pacs.charateristics = g_le_audio_service_discovery_character;

    user_data.uuid.type = BLE_UUID_TYPE_16BIT;
    user_data.uuid.uuid.uuid16 = BT_GATT_UUID16_PACS_SERVICE;
    user_data.need_cache = TRUE;
    user_data.srv_info = &g_le_audio_service_discovery_pacs;
    user_data.handler = bt_le_audio_service_discovery_pacs_callback;
    ret = bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &user_data);
    if (BT_GATTC_DISCOVERY_STATUS_SUCCESS != ret) {
        LE_AUDIO_MSGLOG_I("[SERVICE DISCOVERY] register PACS discovery fail, ret:%x", 1, ret);
    }

    /* Register ASCS discovery */
    g_le_audio_service_discovery_ascs.characteristic_count = BT_LE_AUDIO_SERVICE_DISCOVERY_CHARACTER_MAX_NUMBER;
    g_le_audio_service_discovery_ascs.charateristics = g_le_audio_service_discovery_character;

    user_data.uuid.type = BLE_UUID_TYPE_16BIT;
    user_data.uuid.uuid.uuid16 = BT_GATT_UUID16_ASCS_SERVICE;
    user_data.need_cache = TRUE;
    user_data.srv_info = &g_le_audio_service_discovery_ascs;
    user_data.handler = bt_le_audio_service_discovery_ascs_callback;
    ret = bt_gattc_discovery_register_service(BT_GATTC_DISCOVERY_USER_LE_AUDIO, &user_data);
    if (BT_GATTC_DISCOVERY_STATUS_SUCCESS != ret) {
        LE_AUDIO_MSGLOG_I("[SERVICE DISCOVERY] register ASCS discovery fail, ret:%x", 1, ret);
    }
}

