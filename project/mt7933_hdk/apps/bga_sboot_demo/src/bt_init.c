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

#include <stdint.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "nvdm.h"
#include "bt_system.h"
#include "hal_trng.h"
#include "project_config.h"
#include <string.h>
#include <stdlib.h>
#include "task_def.h"
#include "bt_callback_manager.h"
#include "bt_init.h"
#include "bt_gap.h"
#include "nvdm.h"
#include "hal_sys.h"
#include "hal_cache.h"

BT_ALIGNMENT4(
    static char timer_cb_buf[BT_TIMER_BUF_SIZE]//one timer control block is 20 bytes
);
BT_ALIGNMENT4(
    static char le_connection_cb_buf[BT_LE_CONNECTION_BUF_SIZE]
);
BT_ALIGNMENT4(
    static char adv_set_cb_buf[BT_LE_ADV_SET_BUF_SIZE]
);

BT_ALIGNMENT4(
    static char tx_buf[BT_TX_BUF_SIZE]
);

BT_ALIGNMENT4(
    static char rx_buf[BT_RX_BUF_SIZE]
);
extern void bt_task(void *arg);

bt_bd_addr_t local_public_addr;
uint8_t g_btstack_is_running = false;
static uint8_t g_bt_boot_init_mode = BT_BOOT_INIT_MODE_NONE;

static void bt_preread_local_address(bt_bd_addr_t addr)
{
    nvdm_status_t status;
    int8_t i;
    uint32_t random_seed;
    uint32_t size = 12;
    uint8_t buffer[18] = {0};
    uint8_t tmp_buf[3] = {0};
    bt_bd_addr_t tempaddr = {0};
    hal_trng_status_t ret;
    if (memcmp(addr, &tempaddr, sizeof(bt_bd_addr_t)) == 0) {
        LOG_I(common, "[BT]Empty bt address after bt_gap_le_get_local_address()\n");
        LOG_I(common, "[BT]Try to read from NVDM.\n");
        status = nvdm_read_data_item("BT", "address", buffer, &size);
        if (NVDM_STATUS_OK == status) {
            LOG_I(common, "[BT]Read from NVDM:%s\n", buffer);
            for (i = 0; i < 6; ++i) {
                tmp_buf[0] = buffer[2 * i];
                tmp_buf[1] = buffer[2 * i + 1];
                addr[i] = (uint8_t)strtoul((char *)tmp_buf, NULL, 16);
            }

            LOG_I(common, "[BT]Read address from NVDM [%02X:%02X:%02X:%02X:%02X:%02X]\n", addr[0],
                  addr[1], addr[2], addr[3], addr[4], addr[5]);
            return;
        } else {
            LOG_I(common, "[BT]Failed to Read from NVDM:%d.\n", status);
            ret = hal_trng_init();
            if (HAL_TRNG_STATUS_OK != ret) {
                LOG_I(common, "[BT]generate_random_address--error 1");
            }
            for (i = 0; i < 30; ++i) {
                ret = hal_trng_get_generated_random_number(&random_seed);
                if (HAL_TRNG_STATUS_OK != ret) {
                    LOG_I(common, "[BT]generate_random_address--error 2");
                }
                LOG_I(common, "[BT]generate_random_address--trn: 0x%x", random_seed);
            }
            /* randomly generate address */
            ret = hal_trng_get_generated_random_number(&random_seed);
            if (HAL_TRNG_STATUS_OK != ret) {
                LOG_I(common, "[BT]generate_random_address--error 3");
            }
            LOG_I(common, "[BT]generate_random_address--trn: 0x%x", random_seed);
            addr[0] = random_seed & 0xFF;
            addr[1] = (random_seed >> 8) & 0xFF;
            addr[2] = (random_seed >> 16) & 0xFF;
            addr[3] = (random_seed >> 24) & 0xFF;
            ret = hal_trng_get_generated_random_number(&random_seed);
            if (HAL_TRNG_STATUS_OK != ret) {
                LOG_I(common, "[BT]generate_random_address--error 3");
            }
            LOG_I(common, "[BT]generate_random_address--trn: 0x%x", random_seed);
            addr[4] = random_seed & 0xFF;
            addr[5] = (random_seed >> 8) & 0xCF;
            hal_trng_deinit();
        }
    }
    /* save address to NVDM */
    for (i = 0; i < 6; ++i) {
        int icn = 0;
        icn = snprintf((char *)buffer + 2 * i, 3, "%02X", addr[i]);
        if (icn < 0 || icn > 3) {
            LOG_E(common, "[BT]address write to buffer error");
            return;
        }
    }
    LOG_I(common, "[BT]address to write:%s len:%d\n", buffer, strlen((char *)buffer));
    status = nvdm_write_data_item("BT", "address", NVDM_DATA_ITEM_TYPE_STRING, buffer, strlen((char *)buffer));
    if (NVDM_STATUS_OK != status) {
        LOG_I(common, "[BT]Failed to store address.\n");
    } else {
        LOG_I(common, "[BT]Successfully store address to NVDM [%02X:%02X:%02X:%02X:%02X:%02X]\n", addr[0],
              addr[1], addr[2], addr[3], addr[4], addr[5]);
    }
}

void bt_mm_init(void)
{
    LOG_I(common, "bt init TX_Buf:%d, RX_Buf:%d\n", BT_TX_BUF_SIZE, BT_RX_BUF_SIZE);

    bt_memory_init_packet(BT_MEMORY_TX_BUFFER, tx_buf, BT_TX_BUF_SIZE);
    bt_memory_init_packet(BT_MEMORY_RX_BUFFER, rx_buf, BT_RX_BUF_SIZE);
    bt_memory_init_control_block(BT_MEMORY_CONTROL_BLOCK_TIMER, timer_cb_buf, BT_TIMER_BUF_SIZE);
    bt_memory_init_control_block(BT_MEMORY_CONTROL_BLOCK_LE_CONNECTION, le_connection_cb_buf,
                                 BT_LE_CONNECTION_BUF_SIZE);
    bt_memory_init_control_block(BT_MEMORY_CONTROL_BLOCK_LE_ADV_SET, adv_set_cb_buf,
                                 BT_LE_ADV_SET_BUF_SIZE);
}

#include "bt_gap_le.h"
#include "bt_gatts.h"
extern bt_gap_le_local_config_req_ind_t *bt_ut_gap_le_get_local_config(void);
extern bt_status_t bt_ut_gap_le_get_pairing_config(bt_gap_le_bonding_start_ind_t *ind);
extern bt_status_t bt_ut_app_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff);
extern bt_status_t bt_ut_gatts_service_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff);
extern bt_status_t bt_ut_gatts_get_authorization_check_result(bt_gatts_authorization_check_req_t *req);
extern bt_gap_le_bonding_info_t *bt_ut_gap_le_get_bonding_info(const bt_addr_t remote_addr);
extern bt_gap_le_local_key_t *bt_ut_gap_le_get_local_key(void);
#ifdef __MTK_BT_MESH_ENABLE__
extern bt_status_t mesh_app_bt_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff);
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */
extern bt_status_t bt_ut_gatts_get_execute_write_result(bt_gatts_execute_write_req_t *req);
extern void bt_gatts_switch_init_mode(uint8_t init_mode);
extern void bt_setting_init(void);
extern void bt_gap_set_local_configuration_name(char *name);


//get bt init mode
bt_boot_mode_t bt_get_init_mode(void)
{
    uint8_t init_mode = BT_BOOT_INIT_MODE_NONE;
    uint32_t size = sizeof(init_mode);
    nvdm_status_t status;

    //return saved mode directly to prevent frequently access flash.
    if (g_bt_boot_init_mode != BT_BOOT_INIT_MODE_NONE)
        return g_bt_boot_init_mode;

    status = nvdm_read_data_item("BT_MODE", "initmode", &init_mode, &size);
    BT_LOGI("APP", "boot init_mode = %d, read status = %d", init_mode, status);
    if ((NVDM_STATUS_OK != status) ||
        (init_mode == BT_BOOT_INIT_MODE_NONE) ||
        (init_mode >= BT_BOOT_INIT_MODE_MAX)) {
#if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE))
        init_mode = BT_BOOT_INIT_MODE_GATTS;
#elif defined(__MTK_BT_MESH_ENABLE__)
        init_mode = BT_BOOT_INIT_MODE_MESH;
#else /* #if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE)) */
        init_mode = BT_BOOT_INIT_MODE_DEFAULT;
#endif /* #if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE)) */
        bt_save_init_mode(init_mode);//if save not sucess, also go on
    } else
        g_bt_boot_init_mode = init_mode;

    return (bt_boot_mode_t)init_mode;
}

//save bt init mode
bool bt_save_init_mode(uint8_t init_mode)
{
    nvdm_status_t result = nvdm_write_data_item("BT_MODE",
                                                "initmode",
                                                NVDM_DATA_ITEM_TYPE_RAW_DATA,
                                                (const uint8_t *)(&init_mode),
                                                sizeof(init_mode));
    BT_LOGI("APP", "bt mode save, init_mode = %d, save_result = %d", init_mode, result);

    g_bt_boot_init_mode = init_mode;

    return (NVDM_STATUS_OK == result) ? true : false;
}

void bt_reboot_system(void)
{
    //reboot
    hal_cache_disable();
    hal_cache_deinit();
    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
    return;
}

void bt_create_task(void)
{
    bt_boot_mode_t bt_mode;

    bt_setting_init();

    g_btstack_is_running = true;
    bt_mm_init();

    bt_callback_manager_register_callback(bt_callback_type_gap_le_get_local_cofig,
                                          0,
                                          (void *)bt_ut_gap_le_get_local_config);
    bt_callback_manager_register_callback(bt_callback_type_gap_le_get_pairing_config,
                                          0,
                                          (void *)bt_ut_gap_le_get_pairing_config);
    bt_callback_manager_register_callback(bt_callback_type_gap_le_get_bonding_info,
                                          0,
                                          (void *)bt_ut_gap_le_get_bonding_info);
    bt_callback_manager_register_callback(bt_callback_type_gap_le_get_local_key,
                                          0,
                                          (void *)bt_ut_gap_le_get_local_key);
    bt_callback_manager_register_callback(bt_callback_type_gatts_get_authorization_check_result,
                                          0,
                                          (void *)bt_ut_gatts_get_authorization_check_result);
    bt_callback_manager_register_callback(bt_callback_type_gatts_get_execute_write_result,
                                          0,
                                          (void *)bt_ut_gatts_get_execute_write_result);
    bt_callback_manager_register_callback(bt_callback_type_app_event,
                                          (uint32_t)(MODULE_MASK_GAP | MODULE_MASK_GATT | MODULE_MASK_SYSTEM | MODULE_MASK_MM),
                                          (void *)bt_ut_app_event_callback);

    bt_mode = bt_get_init_mode();
    switch (bt_mode) {
#if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE))
        case BT_BOOT_INIT_MODE_GATTS:
            bt_gatts_switch_init_mode(BT_BOOT_INIT_MODE_GATTS);
            bt_callback_manager_register_callback(bt_callback_type_app_event,
                                                  (uint32_t)(MODULE_MASK_GAP | MODULE_MASK_GATT | MODULE_MASK_SYSTEM | MODULE_MASK_MM),
                                                  (void *)bt_ut_gatts_service_event_callback);
            break;
#endif /* #if (defined(MTK_BT_BAS_SERVICE_ENABLE) || defined(MTK_BLE_SMTCN_ENABLE)) */

#ifdef __MTK_BT_MESH_ENABLE__
        case BT_BOOT_INIT_MODE_MESH:
            bt_gatts_switch_init_mode(BT_BOOT_INIT_MODE_MESH);
            bt_callback_manager_register_callback(bt_callback_type_app_event,
                                                  (uint32_t)(MODULE_MASK_GAP | MODULE_MASK_GATT | MODULE_MASK_SYSTEM | MODULE_MASK_MM),
                                                  (void *)mesh_app_bt_event_callback);
            break;
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */

        default:
            bt_callback_manager_register_callback(bt_callback_type_app_event,
                                                  (uint32_t)(MODULE_MASK_GAP | MODULE_MASK_GATT | MODULE_MASK_SYSTEM | MODULE_MASK_MM),
                                                  (void *)bt_ut_gatts_service_event_callback);
            break;
    }

    bt_preread_local_address(local_public_addr);
    BT_LOGI("BT", "local_public_addr [%02X:%02X:%02X:%02X:%02X:%02X]", local_public_addr[5],
            local_public_addr[4], local_public_addr[3], local_public_addr[2], local_public_addr[1], local_public_addr[0]);
    //rename BT name by address -- HB Duo dev AA-BB -> change AA-BB to addr
    char dev_name[BT_GAP_MAX_DEVICE_NAME_LENGTH];
    int ret = snprintf(dev_name, BT_GAP_MAX_DEVICE_NAME_LENGTH, "HB Duo dev %02x-%02x",
                       local_public_addr[1], local_public_addr[0]);
    BT_LOGI("BT", "Device Name %s (len %d)", dev_name, ret);
    bt_gap_set_local_configuration_name(dev_name);

    //log_config_print_switch(BT, DEBUG_LOG_OFF);
    //log_config_print_switch(BTMM, DEBUG_LOG_OFF);
    //log_config_print_switch(BTHCI, DEBUG_LOG_OFF);
    //log_config_print_switch(BTL2CAP, DEBUG_LOG_OFF);
    if (pdPASS != xTaskCreate(bt_task, BLUETOOTH_TASK_NAME, BLUETOOTH_TASK_STACKSIZE / sizeof(StackType_t),
                              (void *)local_public_addr, BLUETOOTH_TASK_PRIO, NULL)) {
        LOG_E(common, "cannot create bt_task.");
    }
}
