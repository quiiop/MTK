/* Copyright Statement:
 *
 * (C) 2020-2021  MediaTek Inc. All rights reserved.
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
#include "nvdm.h"
#include "bt_debug.h"
#include "bt_setting.h"
#include "syslog.h"

#define BT_SETNVDM_KEY "BT_DEBUG"
#define BT_SETTING_ON_OFF_DEFAULT                                      true

/* the name max length is 10 */
bt_setting_t g_setting_table[] = {
    {"SET_ONOFF",  BT_SETTING_VALUE_TYPE_BOOL,  .value.value_bool = BT_SETTING_ON_OFF_DEFAULT}, //BT_SETTING_ON_OFF
    {"SNOOP_LOG",  BT_SETTING_VALUE_TYPE_BOOL,  .value.value_bool = false}                 //BT_SETTING_KEY_LOG_SNOOP
    /* default level refer to bt_mesh_debug_log_layer */
#ifdef __MTK_BT_MESH_ENABLE__
    , {"MESH_MUST",  BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_INFO},    //BT_SETTING_KEY_LOG_MESH_MUST
    {"MESH_BEAR",  BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_ERROR},    //BT_SETTING_KEY_LOG_MESH_BEARER
    {"MESH_GATT",  BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_WARNING},  //BT_SETTING_KEY_LOG_MESH_BEARER_GATT
    {"MESH_NET",   BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_WARNING},  //BT_SETTING_KEY_LOG_MESH_NETWORK
    {"MESH_TRSP",  BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_WARNING},  //BT_SETTING_KEY_LOG_MESH_TRANSPORT
    {"MESH_ACS",   BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_WARNING},  //BT_SETTING_KEY_LOG_MESH_ACCESSS
    {"MESH_MOD",   BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_INFO},     //BT_SETTING_KEY_LOG_MESH_MODEL
    {"MESH_PROV",  BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_INFO},     //BT_SETTING_KEY_LOG_MESH_PROVISION
    {"MESH_BCON",  BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_ERROR},    //BT_SETTING_KEY_LOG_MESH_BEACON
    {"MESH_PROX",  BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_WARNING},  //BT_SETTING_KEY_LOG_MESH_PROXY
    {"MESH_CFG",   BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_WARNING},  //BT_SETTING_KEY_LOG_MESH_CONFIG
    {"MESH_MW",    BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_INFO},     //BT_SETTING_KEY_LOG_MESH_MIDDLEWARE
    {"MESH_FRID",  BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_INFO},     //BT_SETTING_KEY_LOG_MESH_FRIEND
    {"MESH_UTIL",  BT_SETTING_VALUE_TYPE_U8,    .value.value_u8   = PRINT_LEVEL_ERROR},    //BT_SETTING_KEY_LOG_MESH_UTILS
    {"MESH_FILT",  BT_SETTING_VALUE_TYPE_BOOL,  .value.value_bool = true}                  //BT_SETTING_KEY_LOG_MESH_FILTER
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */
};


extern bt_status_t bt_app_set_mesh_all_log_callback(void);
extern bt_status_t bt_ut_app_set_btsnoop_callback(void);


static bt_setting_log_callback_p bt_setting_log_callback_list[ ] = {
    bt_ut_app_set_btsnoop_callback
#ifdef __MTK_BT_MESH_ENABLE__
    , bt_app_set_mesh_all_log_callback
#endif /* #ifdef __MTK_BT_MESH_ENABLE__ */
};


bool bt_setting_onoff_get(void)
{
    return g_setting_table[BT_SETTING_ON_OFF].value.value_bool;
}

static bool bt_setting_write_value_to_nvm(const char *itemname, bt_setting_value_type_t value_type,
                                          bt_setting_value_t set_value)
{
#ifdef MTK_NVDM_ENABLE
    uint8_t value = 0;
    nvdm_status_t result;
    uint32_t size;

    if (value_type == BT_SETTING_VALUE_TYPE_BOOL) {
        value = set_value.value_bool;
        size = sizeof(set_value.value_bool);

        result = nvdm_write_data_item(BT_SETNVDM_KEY, itemname, NVDM_DATA_ITEM_TYPE_RAW_DATA, (const uint8_t *)(&value), size);
        if (result != NVDM_STATUS_OK) {
            BT_LOGE("BT", "write the value fail, result =%d", result);
            return false;
        } else
            BT_LOGI("APP", "save value = %d, itemname = %s size = %d result = %d", value, itemname, size, result);
    } else if (value_type == BT_SETTING_VALUE_TYPE_U8) {
        value = set_value.value_u8;
        size = sizeof(set_value.value_u8);

        result = nvdm_write_data_item(BT_SETNVDM_KEY, itemname, NVDM_DATA_ITEM_TYPE_RAW_DATA, (const uint8_t *)(&value), size);
        if (result != NVDM_STATUS_OK) {
            BT_LOGE("BT", "write the value fail, result =%d", result);
            return false;
        } else
            BT_LOGI("APP", "save value = %d, itemname = %s size = %d result = %d", value, itemname, size, result);
    } else if (value_type == BT_SETTING_VALUE_TYPE_U32) {
        uint32_t value = set_value.value_u32;
        size = sizeof(set_value.value_u32);

        result = nvdm_write_data_item(BT_SETNVDM_KEY, itemname, NVDM_DATA_ITEM_TYPE_RAW_DATA,
                                      (const uint8_t *)(&value), size);
        if (result != NVDM_STATUS_OK) {
            BT_LOGE("BT", "write the value fail, result =%d", result);
            return false;
        } else
            BT_LOGI("APP", "save value = %d, itemname = %s size = %d result = %d", value, itemname, size, result);
    } else if (value_type == BT_SETTING_VALUE_TYPE_STRING) {
        size = sizeof(set_value.value_str);

        result = nvdm_write_data_item(BT_SETNVDM_KEY, itemname, NVDM_DATA_ITEM_TYPE_RAW_DATA,
                                      (const uint8_t *)set_value.value_str, size);
        if (result != NVDM_STATUS_OK) {
            BT_LOGE("BT", "write the value fail, result =%d", result);
            return false;
        } else
            BT_LOGI("APP", "save value = %s, itemname = %s size = %d result = %d", set_value.value_str, itemname,
                    size, result);
    } else {
        BT_LOGE("BT", "Not supported value type!");
        return false;
    }

    return true;
#else /* #ifdef MTK_NVDM_ENABLE */
    BT_LOGE("BT", "NVDM did not enabled!");
    return false;
#endif /* #ifdef MTK_NVDM_ENABLE */
}

bool bt_setting_onoff_set(bool enable)
{
    bt_setting_value_t set_value;

    set_value.value_bool = enable;

    if (g_setting_table[BT_SETTING_ON_OFF].value.value_bool == enable) {
        BT_LOGI("BT", "value is the same. No set");
        return false;
    }

    if (!bt_setting_write_value_to_nvm(g_setting_table[BT_SETTING_ON_OFF].itemname,
                                       g_setting_table[BT_SETTING_ON_OFF].value_type, set_value)) {
        BT_LOGE("BT", "setting save fail");
        return false;

    }

    g_setting_table[BT_SETTING_ON_OFF].value.value_bool = set_value.value_bool;

    return true;
}

bt_setting_value_t bt_setting_value_get(bt_setting_key_t setting_idx)
{
    if (setting_idx < 0 || setting_idx >= BT_SETTING_KEY_MAX) {
        bt_setting_value_t value;
        value.value_u32 = 0xFFFFFFFF;
        BT_LOGI("BT", " Invalid index, return 0xFFFFFFFF");
        return value;
    }

    return g_setting_table[setting_idx].value;
}

bool bt_setting_value_set(uint8_t setting_idx, bt_setting_value_t set_value)
{
    if (!bt_setting_onoff_get()) {
        BT_LOGW("BT", "setting is off");
        return true;
    }

    if (setting_idx >= BT_SETTING_KEY_MAX) {
        BT_LOGE("BT", " setting_idx >= BT_SETTING_KEY_MAX, wrong parameter!");
        return false;
    }

    switch (g_setting_table[setting_idx].value_type) {
        case BT_SETTING_VALUE_TYPE_BOOL:
            if (g_setting_table[setting_idx].value.value_bool == set_value.value_bool) {
                BT_LOGI("BT", "value is the same. No save");
                return false;
            }

            if (!bt_setting_write_value_to_nvm(g_setting_table[setting_idx].itemname,
                                               g_setting_table[setting_idx].value_type, set_value))
                return false;

            g_setting_table[setting_idx].value.value_bool = set_value.value_bool;
            break;

        case BT_SETTING_VALUE_TYPE_U8:
            if (g_setting_table[setting_idx].value.value_u8 == set_value.value_u8) {
                BT_LOGI("BT", "value is the same. No save");
                return false;
            }

            if (!bt_setting_write_value_to_nvm(g_setting_table[setting_idx].itemname,
                                               g_setting_table[setting_idx].value_type, set_value))
                return false;

            g_setting_table[setting_idx].value.value_u8 = set_value.value_u8;
            break;

        case BT_SETTING_VALUE_TYPE_U32:
            if (g_setting_table[setting_idx].value.value_u32 == set_value.value_u32) {
                BT_LOGI("BT", "value is the same. No save");
                return false;
            }

            if (!bt_setting_write_value_to_nvm(g_setting_table[setting_idx].itemname,
                                               g_setting_table[setting_idx].value_type, set_value))
                return false;

            g_setting_table[setting_idx].value.value_u32 = set_value.value_u32;
            break;

        case BT_SETTING_VALUE_TYPE_STRING:
            if (strcmp(g_setting_table[setting_idx].value.value_str, set_value.value_str) == 0) {
                BT_LOGI("BT", "value is the same. No save");
                return false;
            }

            if (!bt_setting_write_value_to_nvm(g_setting_table[setting_idx].itemname,
                                               g_setting_table[setting_idx].value_type, set_value))
                return false;

            memcpy(g_setting_table[setting_idx].value.value_str, set_value.value_str, sizeof(set_value.value_str));
            break;

        default:
            BT_LOGW("BT", "Not supported value type!");
            return false;
    }

    return true;
}

static void bt_setting_log_to_all_module(void)
{
    uint8_t callback_index;

    for (callback_index = 0; callback_index < sizeof(bt_setting_log_callback_list) / sizeof(bt_setting_log_callback_p);
         callback_index++)
        bt_setting_log_callback_list[callback_index]();

    return;
}

void bt_setting_dump(void)
{
    uint8_t i;

    BT_LOGI("BT", " +++++setting dump+++++");
    for (i = 0; i < BT_SETTING_KEY_MAX; i++) {
        if (g_setting_table[i].value_type == BT_SETTING_VALUE_TYPE_BOOL)
            BT_LOGI("BT", " module = %s, val = %s",
                    g_setting_table[i].itemname, g_setting_table[i].value.value_bool ? "on" : "off");
        else if (g_setting_table[i].value_type == BT_SETTING_VALUE_TYPE_U8)
            BT_LOGI("BT", " module = %s, val = %d",
                    g_setting_table[i].itemname, g_setting_table[i].value.value_u8);
        else if (g_setting_table[i].value_type == BT_SETTING_VALUE_TYPE_U32)
            BT_LOGI("BT", " module = %s, val = %d",
                    g_setting_table[i].itemname, g_setting_table[i].value.value_u32);
        else if (g_setting_table[i].value_type == BT_SETTING_VALUE_TYPE_STRING)
            BT_LOGI("BT", " module = %s, val = %s",
                    g_setting_table[i].itemname, g_setting_table[i].value.value_str);
    }
    BT_LOGI("BT", " ++++++++++++++++++++++");
}

void bt_setting_init(void)
{
    bool setting_on_off = BT_SETTING_ON_OFF_DEFAULT;
#ifdef MTK_NVDM_ENABLE
    nvdm_status_t result;
    uint32_t size = 0;
    uint8_t i, value = 0;

    size = sizeof(g_setting_table[i].value.value_bool);
    result = nvdm_read_data_item(BT_SETNVDM_KEY, (const char *)g_setting_table[BT_SETTING_ON_OFF].itemname, &value, &size);
    if (result != NVDM_STATUS_OK) {
        //No one set setting on/off to NVDM, so use the default value
        //BT_LOGI("BT", "read the value fail, result = %d", result);
    } else {
        BT_LOGI("BT", "read setting on_off = %d, size = %d", value, size);
        setting_on_off = value;
    }

    if (setting_on_off) {
        for (i = BT_SETTING_ON_OFF + 1; i < BT_SETTING_KEY_MAX; i++) {
            if (g_setting_table[i].value_type == BT_SETTING_VALUE_TYPE_BOOL) {
                size = sizeof(g_setting_table[i].value.value_bool);

                result = nvdm_read_data_item(BT_SETNVDM_KEY, (const char *)g_setting_table[i].itemname, &value, &size);
                if (result != NVDM_STATUS_OK) {
                    //BT_LOGE("BT", "No %s in nvdm, ret = %d", g_setting_table[i].itemname, result);
                } else {
                    //BT_LOGI("BT", "read the value = %d, size = %d", value, size);
                    g_setting_table[i].value.value_bool = value;
                }
            } else if (g_setting_table[i].value_type == BT_SETTING_VALUE_TYPE_U8) {
                size = sizeof(g_setting_table[i].value.value_u8);

                result = nvdm_read_data_item(BT_SETNVDM_KEY, g_setting_table[i].itemname, &value, &size);
                if (result != NVDM_STATUS_OK) {
                    //BT_LOGE("BT", "No %s in nvdm, ret = %d", g_setting_table[i].itemname, result);
                } else {
                    //BT_LOGI("BT", "read the value = %d, size = %d", value, size);
                    g_setting_table[i].value.value_u8 = value;
                }
            } else if (g_setting_table[i].value_type == BT_SETTING_VALUE_TYPE_U32) {
                uint32_t value_u32;
                size = sizeof(g_setting_table[i].value.value_u32);

                result = nvdm_read_data_item(BT_SETNVDM_KEY, g_setting_table[i].itemname, (uint8_t *)(&value_u32), &size);
                if (result != NVDM_STATUS_OK) {
                    //BT_LOGE("BT", "No %s in nvdm, ret = %d", g_setting_table[i].itemname, result);
                } else {
                    //BT_LOGI("BT", "read the value = %d, size = %d", value, size);
                    g_setting_table[i].value.value_u32 = value_u32;
                }
            } else if (g_setting_table[i].value_type == BT_SETTING_VALUE_TYPE_STRING) {
                size = sizeof(g_setting_table[i].value.value_str);

                result = nvdm_read_data_item(BT_SETNVDM_KEY, g_setting_table[i].itemname,
                                             (uint8_t *)(&g_setting_table[i].value.value_str), &size);
                if (result != NVDM_STATUS_OK) {
                    //BT_LOGE("BT", "No %s in nvdm, ret = %d", g_setting_table[i].itemname, result);
                }
                //else {
                //    BT_LOGI("BT", "read the value = %s, size = %d", g_setting_table[i].value.value_str, size);
                //}
            }
        }
    } else
        BT_LOGI("BT", "setting is off. Use default value for all!");
#endif /* #ifdef MTK_NVDM_ENABLE */

    g_setting_table[BT_SETTING_ON_OFF].value.value_bool = setting_on_off;

    bt_setting_log_to_all_module();

    bt_setting_dump();
}
