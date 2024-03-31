/* Copyright Statement:
 *
 * (C) 2019  MediaTek Inc. All rights reserved.
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

#include "bt_le_audio_sink.h"

#include "bt_callback_manager.h"
#include "bt_gap_le_service.h"

#include "bt_le_audio_msglog.h"

#ifdef MTK_SUPPORT_CCCD_RECORD //MTK_Titan: Temp remove this feature due to no verification method
#include "nvkey.h"
#include "nvkey_id_list.h"

#include "bt_device_manager_le.h"
#include "bt_device_manager_le_config.h"
#endif

/**************************************************************************************************
* Define
**************************************************************************************************/
#ifndef PACKED
#define PACKED  __attribute__((packed))
#endif

#ifdef MTK_SUPPORT_CCCD_RECORD
/* N: LEA_SERVICE_CCCD_DEVICE_NUM_DEFAULT,
   M: LEA_SERVICE_CCCD_VALUE_NUM_DEFAULT
   total size = 1 + N* (1+6+1 + M*(2+1))
   LEA_SERVICE_CCCD_DATA_SIZE must be larger than total size
   */
#define LEA_SERVICE_CCCD_DATA_SIZE             800
#define LEA_SERVICE_CCCD_DEVICE_NUM_DEFAULT    BT_DEVICE_MANAGER_LE_BONDED_MAX
#define LEA_SERVICE_CCCD_VALUE_NUM_DEFAULT     30

#define LEA_SERVICE_CCCD_DEBOUNCE_TIME         3000

typedef struct {
    uint16_t attr_handle;
    uint8_t cccd_value;
} PACKED bt_le_audio_nvkey_cccd_value_record_t;

typedef struct {
    bt_addr_t device_addr;
    uint8_t num_of_cccd;
    bt_le_audio_nvkey_cccd_value_record_t cccd_record[];
} PACKED bt_le_audio_nvkey_cccd_device_record_t;

typedef struct {
    uint8_t num_of_device;
    bt_le_audio_nvkey_cccd_device_record_t device_record[];
} PACKED bt_le_audio_nvkey_cccd_t;
#endif

/**************************************************************************************************
* Prototype
**************************************************************************************************/
extern bt_status_t le_audio_init(ble_tmap_role_t role, bt_le_audio_sink_callback_t callback, uint8_t max_link_num);
extern bt_status_t le_audio_deinit(bt_le_audio_sink_callback_t callback, uint8_t max_link_num);
extern bt_status_t le_audio_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff);
extern void bt_le_audio_atci_init(void);
#ifdef MTK_SUPPORT_CCCD_RECORD
extern bool ble_ascs_set_cccd_handler(bt_handle_t handle, uint16_t attr_handle, uint16_t value);
extern bool ble_csis_set_cccd_handler(bt_handle_t handle, uint16_t attr_handle, uint16_t value);
extern bool ble_pacs_set_cccd_handler(bt_handle_t handle, uint16_t attr_handle, uint16_t value);
extern bool ble_vcs_set_cccd_handler(bt_handle_t handle, uint16_t attr_handle, uint16_t value);
extern bool ble_vocs_set_cccd_handler(bt_handle_t handle, uint16_t attr_handle, uint16_t value);
extern bool ble_aics_set_cccd_handler(bt_handle_t handle, uint16_t attr_handle, uint16_t value);
extern bool ble_mics_set_cccd_handler(bt_handle_t handle, uint16_t attr_handle, uint16_t value);
extern bool ble_bass_set_cccd_handler(bt_handle_t handle, uint16_t attr_handle, uint16_t value);
extern bool ble_haps_set_cccd_handler(bt_handle_t conn_handle, uint16_t attr_handle, uint16_t value);
extern bt_le_audio_cccd_record_t* ble_ascs_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num);
extern bt_le_audio_cccd_record_t* ble_csis_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num);
extern bt_le_audio_cccd_record_t* ble_pacs_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num);
extern bt_le_audio_cccd_record_t* ble_vcs_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num);
extern bt_le_audio_cccd_record_t* ble_vocs_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num);
extern bt_le_audio_cccd_record_t* ble_aics_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num);
extern bt_le_audio_cccd_record_t* ble_mics_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num);
extern bt_le_audio_cccd_record_t* ble_haps_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num);
bool bt_le_audio_sink_cccd_nvkey_save(void);
bool bt_le_audio_sink_cccd_nvkey_dump(void);
bool bt_le_audio_sink_cccd_nvkey_reset(void);
bool bt_le_audio_sink_dump_cccd(void);
#endif

/**************************************************************************************************
* Variable
**************************************************************************************************/
#ifdef MTK_SUPPORT_CCCD_RECORD
static le_audio_set_cccd_callback_t g_set_cccd_handler[] = {
    ble_ascs_set_cccd_handler,
    ble_csis_set_cccd_handler,
    ble_pacs_set_cccd_handler,
    ble_haps_set_cccd_handler,
    ble_vcs_set_cccd_handler,
    ble_vocs_set_cccd_handler,
    ble_aics_set_cccd_handler,
    ble_mics_set_cccd_handler,
    //ble_bass_set_cccd_handler
};

static le_audio_get_cccd_callback_t g_get_cccd_handler[] = {
    ble_ascs_get_cccd_handler,
    ble_csis_get_cccd_handler,
    ble_pacs_get_cccd_handler,
    ble_haps_get_cccd_handler,
    ble_vcs_get_cccd_handler,
    ble_vocs_get_cccd_handler,
    ble_aics_get_cccd_handler,
    ble_mics_get_cccd_handler,
    //ble_bass_set_cccd_handler
};

static uint8_t g_le_audio_cccd_record[LEA_SERVICE_CCCD_DATA_SIZE] = {0};
static le_audio_timer_id_t g_sink_cccd_timer_id = LE_AUDIO_TIMER_ID_MAX;
static bool g_le_audio_cccd_loading = false;
#endif
/**************************************************************************************************
* Static function
**************************************************************************************************/
#ifdef MTK_SUPPORT_CCCD_RECORD
static bt_le_audio_nvkey_cccd_device_record_t* bt_le_audio_sink_get_record(uint8_t *buf, uint32_t size, bt_addr_t *target_addr)
{
    LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_get_record 0x%x, size=%d, targe_addr=0x%x", 3, buf, size, target_addr);

    bt_le_audio_nvkey_cccd_t *p_table = (bt_le_audio_nvkey_cccd_t *)buf;
    bt_le_audio_nvkey_cccd_device_record_t *p_record = NULL, *p_record_empty = NULL;
    bt_addr_t addr_empty = {0};
    uint8_t i = 0, *p_tmp;

    if (buf == NULL) {
        return NULL;
    }

    if (p_table->num_of_device == 0) {
        p_table->num_of_device = LEA_SERVICE_CCCD_DEVICE_NUM_DEFAULT;
        LE_AUDIO_MSGLOG_W("[CCCD] bt_le_audio_sink_get_record, new table", 0);
    }

    p_tmp = buf+1;

    for (i=0; i< p_table->num_of_device; i++) {
        p_record = (bt_le_audio_nvkey_cccd_device_record_t *)p_tmp;

        LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_get_record, p_tmp %d, %d", 2, p_tmp, &p_record->device_addr);

        if (target_addr != NULL && (0 == memcmp(&p_record->device_addr, target_addr, sizeof(bt_addr_t)))){
            if (p_record->num_of_cccd != LEA_SERVICE_CCCD_VALUE_NUM_DEFAULT) {
                LE_AUDIO_MSGLOG_W("[CCCD] num_of_cccd not matched, nvdm:%d, default:%d", 2, p_record->num_of_cccd, LEA_SERVICE_CCCD_VALUE_NUM_DEFAULT);
            }
            LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_get_record, record [%d] found, cccd num:%d. [%d]%02x:%02x:%02x:%02x:%02x:%02x", 9,
                i, p_record->num_of_cccd, p_record->device_addr.type,
                p_record->device_addr.addr[5], p_record->device_addr.addr[4], p_record->device_addr.addr[3],
                p_record->device_addr.addr[2], p_record->device_addr.addr[1], p_record->device_addr.addr[0]);
            return p_record;
        }

        LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_get_record, find empty", 0);

        if (0 == memcmp(&p_record->device_addr, &addr_empty, sizeof(bt_addr_t))) {
            if (p_record_empty == NULL) {
                p_record_empty = p_record;
                LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_get_record, empty record 0x%x", 1, p_record_empty);
            }
        }
        if (target_addr == NULL) {
            // if no specifed target address, return 1st empty record pointer
            return p_record_empty;
        }
        p_tmp += sizeof(bt_addr_t) + 1 + (p_record->num_of_cccd * sizeof(bt_le_audio_nvkey_cccd_value_record_t));
    }

    // no record found, set 1st empty record pointer for this target addr
    if (p_record_empty != NULL) {
        memcpy(p_record_empty, target_addr, sizeof(bt_addr_t));
    }

    return p_record_empty;
}

static bool bt_le_audio_sink_cccd_nvkey_init()
{
#ifdef MTK_NVDM_ENABLE
    nvkey_status_t status;
    uint8_t *p_cccd_data;
    uint32_t size = LEA_SERVICE_CCCD_DATA_SIZE;

    if (NULL == (p_cccd_data = le_audio_malloc(LEA_SERVICE_CCCD_DATA_SIZE))) {
        return false;
    }

    memset(p_cccd_data, 0, LEA_SERVICE_CCCD_DATA_SIZE);

    status = nvkey_read_data(NVID_BT_LEA_SERVICE_CCCD, p_cccd_data, &size);
    if (NVKEY_STATUS_OK != status) {
        LE_AUDIO_MSGLOG_I("[CCCD] lea_service_cccd_data, error NVKEY status=%d", 1, status);
        le_audio_free(p_cccd_data);
        return false;
    }

    memcpy(g_le_audio_cccd_record, p_cccd_data, size);

    if (g_le_audio_cccd_record[0] != LEA_SERVICE_CCCD_DEVICE_NUM_DEFAULT) {
        LE_AUDIO_MSGLOG_W("[CCCD] num_of_device not matched, nvkey:%d, default:%d", 2, g_le_audio_cccd_record[0], LEA_SERVICE_CCCD_DEVICE_NUM_DEFAULT);
    }

    LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_cccd_nvkey_init done, size: %d", 1, size);
    le_audio_free(p_cccd_data);
#endif

    bt_le_audio_sink_dump_cccd();

    return true;
}

static bool bt_le_audio_sink_set_service_cccd(bt_handle_t handle, uint16_t attr_handle, uint16_t value)
{
    uint8_t k;
    for (k=0;k<sizeof(g_set_cccd_handler)/sizeof(le_audio_set_cccd_callback_t);k++) {
        le_audio_set_cccd_callback_t handler = g_set_cccd_handler[k];
        if (handler(handle, attr_handle, value)) {
            return true;
        }
    }
    return false;
}

static void bt_le_audio_sink_timer_callback(void *p_data)
{
    bt_le_audio_sink_cccd_nvkey_save();
}
#endif /* #ifdef MTK_SUPPORT_CCCD_RECORD */

/**************************************************************************************************
* Public function
**************************************************************************************************/
#ifdef MTK_SUPPORT_CCCD_RECORD
bool bt_le_audio_sink_cccd_nvkey_save(void)
{
#ifdef MTK_NVDM_ENABLE
    nvkey_status_t status = NVKEY_STATUS_OK;
    uint8_t *p_cccd_data;
    uint32_t size = LEA_SERVICE_CCCD_DATA_SIZE;

    if (NULL == (p_cccd_data = le_audio_malloc(LEA_SERVICE_CCCD_DATA_SIZE))) {
        return false;
    }

    memset(p_cccd_data, 0, LEA_SERVICE_CCCD_DATA_SIZE);

    status = nvkey_read_data(NVID_BT_LEA_SERVICE_CCCD, p_cccd_data, &size);
    if (NVKEY_STATUS_OK != status) {
        LE_AUDIO_MSGLOG_I("[CCCD] lea_service_cccd_data, error NVKEY status=%d", 1, status);
    } else if (0 == memcmp(g_le_audio_cccd_record, p_cccd_data, size)) {
        LE_AUDIO_MSGLOG_I("[CCCD] lea_service_cccd_data, no modification", 0);
        le_audio_free(p_cccd_data);
        return true;
    }

    bt_le_audio_sink_dump_cccd();

    status = nvkey_write_data(NVID_BT_LEA_SERVICE_CCCD, g_le_audio_cccd_record, LEA_SERVICE_CCCD_DATA_SIZE);
    if (NVKEY_STATUS_OK != status) {
        LE_AUDIO_MSGLOG_E("[CCCD] lea_service_cccd_data, write failed, %d", 1, status);
        le_audio_free(p_cccd_data);
        return false;
    }

    LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_cccd_save done", 0);
    le_audio_free(p_cccd_data);
#endif

    return true;
}

bool bt_le_audio_sink_cccd_nvkey_reset(void)
{
#ifdef MTK_NVDM_ENABLE
    nvkey_status_t status = NVKEY_STATUS_OK;
    uint8_t *p_cccd_data;

    if (NULL == (p_cccd_data = le_audio_malloc(LEA_SERVICE_CCCD_DATA_SIZE))) {
        return false;
    }

    memset(p_cccd_data, 0, LEA_SERVICE_CCCD_DATA_SIZE);

    status = nvkey_write_data(NVID_BT_LEA_SERVICE_CCCD, p_cccd_data, LEA_SERVICE_CCCD_DATA_SIZE);
    if (NVKEY_STATUS_OK != status) {
        LE_AUDIO_MSGLOG_I("[CCCD] lea_service_cccd_data, reset NVKEY failed=%d", 1, status);
        le_audio_free(p_cccd_data);
        return false;
    }

    LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_cccd_nvkey_reset done", 0);
    le_audio_free(p_cccd_data);
#endif

    return true;
}

bool bt_le_audio_sink_dump_cccd(void)
{
    uint8_t i = 0, j = 0, *p_tmp;

    bt_le_audio_nvkey_cccd_device_record_t *p_record = NULL;

    p_tmp = &g_le_audio_cccd_record[1];

    LE_AUDIO_MSGLOG_I("[CCCD] address %x", 1, g_le_audio_cccd_record);

    for (i=0; i< g_le_audio_cccd_record[0]; i++) {
        p_record = (bt_le_audio_nvkey_cccd_device_record_t *)p_tmp;

        LE_AUDIO_MSGLOG_I("[CCCD] %x, device record#%d [%d]%02x:%02x:%02x:%02x:%02x:%02x", 9,
            p_record, i, p_record->device_addr.type,
            p_record->device_addr.addr[5], p_record->device_addr.addr[4], p_record->device_addr.addr[3],
            p_record->device_addr.addr[2], p_record->device_addr.addr[1], p_record->device_addr.addr[0]);

        for (j=0; j<p_record->num_of_cccd; j++){
            LE_AUDIO_MSGLOG_I("[CCCD]     %d/%d attr_handle 0x%04x, cccd_value 0x%04x", 4,
                j, p_record->num_of_cccd, p_record->cccd_record[j].attr_handle, p_record->cccd_record[j].cccd_value);
        }
        p_tmp += sizeof(bt_addr_t) + 1 + (p_record->num_of_cccd * sizeof(bt_le_audio_nvkey_cccd_value_record_t));
    }

    return true;
}

/* used when LEA connected */
bool bt_le_audio_sink_load_cccd(bt_handle_t handle)
{

    uint8_t i = 0;

    bt_device_manager_le_bonded_info_t *bond_info = bt_device_manager_le_get_bonded_info_by_handle(handle);

    if (bond_info == NULL) {
        return false;
    }

    LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_load_cccd[0x%04x]: [%d]%02x:%02x:%02x:%02x:%02x:%02x", 8,
        handle, bond_info->bt_addr.type,
        bond_info->bt_addr.addr[5], bond_info->bt_addr.addr[4], bond_info->bt_addr.addr[3],
        bond_info->bt_addr.addr[2], bond_info->bt_addr.addr[1], bond_info->bt_addr.addr[0]);

    bt_le_audio_nvkey_cccd_device_record_t *p_record = bt_le_audio_sink_get_record(g_le_audio_cccd_record, LEA_SERVICE_CCCD_DATA_SIZE, &bond_info->bt_addr);

    g_le_audio_cccd_loading = true;

    if (p_record && p_record->num_of_cccd != 0) {
        for (i=0; i< p_record->num_of_cccd ; i++) {
            LE_AUDIO_MSGLOG_I("[CCCD][%d] attr_handle: 0x%04x, cccd: 0x%04x", 3, i, p_record->cccd_record[i].attr_handle, p_record->cccd_record[i].cccd_value);
            bt_le_audio_sink_set_service_cccd(handle, p_record->cccd_record[i].attr_handle, (uint16_t)p_record->cccd_record[i].cccd_value);
        }
    }

    g_le_audio_cccd_loading = false;

    return true;
}

/* used when bonded info removed */
bool bt_le_audio_sink_delete_cccd(bt_addr_t *target_addr)
{
    if (target_addr == NULL) {
        LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_delete_cccd: clear all", 0);
        memset(g_le_audio_cccd_record, 0, LEA_SERVICE_CCCD_DATA_SIZE);
        bt_le_audio_sink_cccd_nvkey_save();
        return false;
   }

    LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_delete_cccd: [%d]%02x:%02x:%02x:%02x:%02x:%02x", 7, target_addr->type,
        target_addr->addr[5], target_addr->addr[4], target_addr->addr[3],
        target_addr->addr[2], target_addr->addr[1], target_addr->addr[0]);

    bt_le_audio_nvkey_cccd_device_record_t *p_record = bt_le_audio_sink_get_record(g_le_audio_cccd_record, LEA_SERVICE_CCCD_DATA_SIZE, target_addr);

    if (p_record == NULL) {
        LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_delete_cccd, no record exist", 0);
        return false;
    }

    if (0 == memcmp(target_addr, &p_record->device_addr, sizeof(bt_addr_t))) {
        // set addr to all-zero
        memset(&p_record->device_addr, 0, sizeof(bt_addr_t));
        p_record->num_of_cccd = 0;
    }

    return true;
}

/* used when LEA service updated */
bool bt_le_audio_sink_update_cccd(bt_handle_t handle)
{
    uint8_t i = 0, j = 0, k = 0;

    bt_device_manager_le_bonded_info_t *bond_info = bt_device_manager_le_get_bonded_info_by_handle(handle);

    if (bond_info == NULL) {
        return false;
    }

    LE_AUDIO_MSGLOG_I("[CCCD] bt_le_audio_sink_update_cccd[0x%04x]: [%d]%02x:%02x:%02x:%02x:%02x:%02x", 8,
        handle, bond_info->bt_addr.type,
        bond_info->bt_addr.addr[5], bond_info->bt_addr.addr[4], bond_info->bt_addr.addr[3],
        bond_info->bt_addr.addr[2], bond_info->bt_addr.addr[1], bond_info->bt_addr.addr[0]);

    if (handle != BT_HANDLE_INVALID) {
        bt_le_audio_nvkey_cccd_device_record_t *p_record = bt_le_audio_sink_get_record(g_le_audio_cccd_record, LEA_SERVICE_CCCD_DATA_SIZE, &bond_info->bt_addr);

        if (p_record == NULL) {
            LE_AUDIO_MSGLOG_E("[CCCD] bt_le_audio_sink_update_cccd, no empty record", 0);
            return false;
        }

        if (p_record->num_of_cccd == 0) {
            p_record->num_of_cccd = LEA_SERVICE_CCCD_VALUE_NUM_DEFAULT;
            LE_AUDIO_MSGLOG_W("[CCCD] bt_le_audio_sink_update_cccd, new record", 0);
        }

        bt_le_audio_cccd_record_t *cccd_list = NULL;
        uint32_t cccd_num;
        for (k=0;k<sizeof(g_get_cccd_handler)/sizeof(le_audio_get_cccd_callback_t);k++) {
            if (i<p_record->num_of_cccd){
                cccd_list = g_get_cccd_handler[k](handle, &cccd_num);

                if (cccd_list != NULL) {
                    for (j=0 ; j<cccd_num; j++) {
                        p_record->cccd_record[i+j].attr_handle = cccd_list[j].attr_handle;
                        p_record->cccd_record[i+j].cccd_value = (uint8_t)cccd_list[j].cccd_value;
                    }
                    i+= cccd_num;
                    le_audio_free(cccd_list);
                }
            } else {
                break;
            }
        }
    }

    return true;
}

void bt_le_audio_sink_service_cccd_changed(bt_handle_t handle)
{
    if (g_le_audio_cccd_loading) {
        return;
    }

    bt_le_audio_sink_update_cccd(handle);
    le_audio_start_timer(g_sink_cccd_timer_id);
}
#endif /* #ifdef MTK_SUPPORT_CCCD_RECORD */

bt_status_t bt_le_audio_sink_init(ble_tmap_role_t role, bt_le_audio_sink_callback_t callback, uint8_t max_link_num)
{
    LE_AUDIO_MSGLOG_I("[init] bt_le_audio_sink_init", 0);

    le_audio_init_timer();

    le_audio_init(role, callback, max_link_num);

#if 0 //__MTK_COMMON__
	/* Init le audio at command */
    bt_le_audio_atci_init();
#endif

#ifdef MTK_SUPPORT_CCCD_RECORD
    bt_le_audio_sink_cccd_nvkey_init();
#endif

    /* register callback */
    bt_callback_manager_register_callback(bt_callback_type_app_event,
                                          MODULE_MASK_GAP | MODULE_MASK_GATT | MODULE_MASK_SYSTEM,
                                          (void *)le_audio_event_callback);

#ifdef MTK_SUPPORT_CCCD_RECORD
    if (LE_AUDIO_TIMER_ID_MAX == (g_sink_cccd_timer_id = le_audio_create_timer(LEA_SERVICE_CCCD_DEBOUNCE_TIME, bt_le_audio_sink_timer_callback, NULL))) {
        LE_AUDIO_MSGLOG_E("[init] bt_le_audio_sink_init no available timer", 0);
        return BT_STATUS_FAIL;
    }
#endif

    return BT_STATUS_SUCCESS;
}

