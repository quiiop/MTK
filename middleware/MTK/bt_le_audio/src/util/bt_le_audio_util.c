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
#include "nvkey.h"
#include "nvkey_id_list.h"
#include "bt_le_audio_util_nvkey_struct.h"

#include "bt_le_audio_util.h"
#include "bt_le_audio_msglog.h"
#include "ble_bap.h"

#ifdef __MTK_BT_SINK_SUPPORT_
#include "bt_sink_srv_ami.h"
#endif
#ifdef __MTK_BT_SINK_LA_SUPPORT_
#include "bt_sink_srv_le_cap_audio_manager.h"
#include "bt_sink_srv_le_cap_stream.h"
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/**************************************************************************************************
* Define
******************************************* *******************************************************/

/**************************************************************************************************
* Structure
**************************************************************************************************/
typedef struct {
    le_audio_timeout_callback_t cb;
    TimerHandle_t timer_handle;
} le_audio_timer_struct;

/**************************************************************************************************
* Variable
**************************************************************************************************/
static le_audio_timer_struct g_le_audio_timer[LE_AUDIO_TIMER_ID_MAX];
static le_audio_timer_id_t  g_le_audio_timer_id = 0;
#ifndef MTK_LE_AUDIO_TWO_CHANNEL
static le_audio_device_type_t g_le_audio_device_type = LE_AUDIO_DEVICE_TYPE_EARBUDS;
#else
static le_audio_device_type_t g_le_audio_device_type = LE_AUDIO_DEVICE_TYPE_HEADSET;
#endif

/**************************************************************************************************
* Prototype
**************************************************************************************************/

/**************************************************************************************************
* Functions
**************************************************************************************************/
/* MEM */
void *le_audio_malloc(uint32_t size)
{
    //uint32_t xLinkRegAddr = (uint32_t)__builtin_return_address(0);
    //LE_AUDIO_MSGLOG_I("[UTIL] le_audio_malloc lr: 0x%X", 1, xLinkRegAddr);
    void *ptr = (void *)pvPortMalloc(size);
    //LE_AUDIO_MSGLOG_I("[UTIL] le_audio_malloc ptr: 0x%X", 1, ptr);

    if (NULL != ptr) {
        memset(ptr, 0, size);
    }

    return ptr;
}

void le_audio_free(void *ptr)
{
    //uint32_t xLinkRegAddr = (uint32_t)__builtin_return_address(0);
    //LE_AUDIO_MSGLOG_I("[UTIL] le_audio_free lr: 0x%X", 1, xLinkRegAddr);
    if (NULL != ptr) {
        vPortFree(ptr);
        //LE_AUDIO_MSGLOG_I("[UTIL] le_audio_free ptr: 0x%X", 1, ptr);
    }
}

void *le_audio_memset(void *ptr, int32_t value, uint32_t size)
{
    //uint32_t xLinkRegAddr = (uint32_t)__builtin_return_address(0);
    if (NULL == ptr) {
        return NULL;
    }

    //LE_AUDIO_MSGLOG_I("[UTIL] le_audio_memset lr: 0x%X", 1, xLinkRegAddr);
    //LE_AUDIO_MSGLOG_I("[UTIL] le_audio_memset param: 0x%X, %u", 2, ptr, size);

    return memset(ptr, value, size);
}

void *le_audio_memcpy(void *dest, const void *src, uint32_t size)
{
    //uint32_t xLinkRegAddr = (uint32_t)__builtin_return_address(0);
    if ((NULL == dest) || (NULL == src)) {
        return NULL;
    }

    //LE_AUDIO_MSGLOG_I("[UTIL] le_audio_memcpy lr: 0x%X", 1, xLinkRegAddr);
    //LE_AUDIO_MSGLOG_I("[UTIL] le_audio_memcpy param: 0x%X, 0x%X, %u", 3, dest, src, size);

    return memcpy(dest, src, size);
}

int32_t le_audio_memcmp(const void *dest, const void *src, uint32_t size)
{
    if ((NULL == dest) || (NULL == src)) {
        return -1;
    }

    return memcmp(dest, src, size);
}

/* Timer */
static void le_audio_timeout_callback(TimerHandle_t xTimer)
{
    uint8_t id = 0;
    LE_AUDIO_MSGLOG_I("[UTIL] timeout_callback, xTimer:%x ", 1, xTimer);

    if (NULL == xTimer) {
        return;
    }

    for (id = 0; id < LE_AUDIO_TIMER_ID_MAX; id++) {

        if (xTimer == g_le_audio_timer[id].timer_handle) {

            void *pdata = pvTimerGetTimerID(xTimer);

            LE_AUDIO_MSGLOG_I("[UTIL] timeout_callback, id:%x ", 1, id);

            if (g_le_audio_timer[id].cb) {
                g_le_audio_timer[id].cb(pdata);
            }
            break;
        }
    }
}

void le_audio_init_timer(void)
{
    memset(g_le_audio_timer, 0, sizeof(le_audio_timer_struct)*LE_AUDIO_TIMER_ID_MAX);
    g_le_audio_timer_id = 0;
}

le_audio_timer_id_t le_audio_create_timer(uint32_t timeMs, le_audio_timeout_callback_t callback, void *p_data)
{
    le_audio_timer_id_t id = g_le_audio_timer_id;

    if (g_le_audio_timer_id >= LE_AUDIO_TIMER_ID_MAX) {
        LE_AUDIO_MSGLOG_I("[UTIL] create_timer, timer id full(%x)", 1, g_le_audio_timer_id);
        return LE_AUDIO_TIMER_ID_MAX;
    }

    if (timeMs <= 0) {
        LE_AUDIO_MSGLOG_I("[UTIL] create_timer, invalid timeout(%x)", 1, timeMs);
        return LE_AUDIO_TIMER_ID_MAX;
    }

    if (NULL != g_le_audio_timer[id].timer_handle) {
        LE_AUDIO_MSGLOG_I("[UTIL] create_timer, already create", 0);
        return LE_AUDIO_TIMER_ID_MAX;
    }

    char hex[5];
    sprintf(hex, "%X", (unsigned int)id);
    TimerHandle_t timer = xTimerCreate(hex, timeMs * portTICK_PERIOD_MS, pdFALSE, p_data, le_audio_timeout_callback);

    if (NULL == timer) {
        return LE_AUDIO_TIMER_ID_MAX;
    }

    LE_AUDIO_MSGLOG_I("[UTIL] create_timer, id:%x timer:%x", 2, id, timer);

    g_le_audio_timer[id].timer_handle = timer;
    g_le_audio_timer[id].cb = callback;
    g_le_audio_timer_id++;

    return id;
}

bt_status_t le_audio_start_timer(le_audio_timer_id_t id)
{
    TimerHandle_t timer = NULL;

    if (id >= LE_AUDIO_TIMER_ID_MAX) {
        LE_AUDIO_MSGLOG_I("[UTIL] start_timer, invalid timer_id:%x", 1, id);
        return BT_STATUS_FAIL;
    }

    if (NULL == (timer = g_le_audio_timer[id].timer_handle)) {
        LE_AUDIO_MSGLOG_I("[UTIL] start_timer, timer handle not exist", 0);
        return BT_STATUS_FAIL;
    }

    if (pdTRUE == xTimerIsTimerActive(timer)) {
        xTimerStop(timer, 0);
    }

    xTimerStart(timer, 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t le_audio_stop_timer(le_audio_timer_id_t id)
{
    TimerHandle_t timer = NULL;

    LE_AUDIO_MSGLOG_I("[UTIL] stop_timer, id:%x, handle:0x%x", 2, id, g_le_audio_timer[id].timer_handle);

    if ((id >= LE_AUDIO_TIMER_ID_MAX) || (NULL == (timer = g_le_audio_timer[id].timer_handle))) {
        return BT_STATUS_FAIL;
    }

    if (pdTRUE != xTimerIsTimerActive(timer)) {
        return BT_STATUS_FAIL;
    }

    xTimerStop(timer, 0);
    LE_AUDIO_MSGLOG_I("[UTIL] stop_timer, success", 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t le_audio_reset_timer(le_audio_timer_id_t id)
{
    TimerHandle_t timer = NULL;

    if (id >= LE_AUDIO_TIMER_ID_MAX) {
        LE_AUDIO_MSGLOG_I("[UTIL] reset_timer, invalid timer_id:%d", 1, id);
        return BT_STATUS_FAIL;
    }

    if (NULL == (timer = g_le_audio_timer[id].timer_handle)) {
        return BT_STATUS_FAIL;
    }

    xTimerReset(timer, 0);

    LE_AUDIO_MSGLOG_I("[UTIL] reset_timer, success", 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t le_audio_delete_timer(le_audio_timer_id_t id)
{
    TimerHandle_t timer = NULL;

    LE_AUDIO_MSGLOG_I("[UTIL] delete_timer, id:%x, handle:0x%x", 2, id, g_le_audio_timer[id].timer_handle);

    if ((id >= LE_AUDIO_TIMER_ID_MAX) || (NULL == (timer = g_le_audio_timer[id].timer_handle))) {
        return BT_STATUS_FAIL;
    }

    xTimerDelete(g_le_audio_timer[id].timer_handle, 0);

    memset(&g_le_audio_timer[id], 0, sizeof(le_audio_timer_struct));

    LE_AUDIO_MSGLOG_I("[UTIL] delete_timer, success", 0);

    return BT_STATUS_SUCCESS;
}

/*
bt_status_t le_audio_set_volume(le_audio_stream_type_t type, uint8_t volume)
{
#ifndef AIR_BLE_AUDIO_DONGLE_ENABLE
    bt_sink_srv_am_stream_type_t in_out = ((type == LE_AUDIO_STREAM_TYPE_IN) ? STREAM_IN : STREAM_OUT);

    if (AUD_EXECUTION_SUCCESS != bt_sink_srv_ami_audio_set_volume(bt_sink_srv_cap_am_get_aid(), volume, in_out)) {
        return BT_STATUS_FAIL;
    }
#endif
    return BT_STATUS_SUCCESS;
}
*/
/*
bt_status_t le_audio_set_mute(le_audio_stream_type_t type, bool mute)
{
#ifndef AIR_BLE_AUDIO_DONGLE_ENABLE
    bt_sink_srv_am_stream_type_t in_out = ((type == LE_AUDIO_STREAM_TYPE_IN) ? STREAM_IN : STREAM_OUT);

    if (AUD_EXECUTION_SUCCESS != bt_sink_srv_ami_audio_set_mute(bt_sink_srv_cap_am_get_aid(), mute, in_out)) {
        return BT_STATUS_FAIL;
    }
#endif
    return BT_STATUS_SUCCESS;
}
*/

bt_handle_t le_audio_get_stream_srv_link_handle(void)
{
    bt_handle_t handle = BT_HANDLE_INVALID;

#if defined(AIR_LE_AUDIO_CIS_ENABLE) && !defined(AIR_BLE_AUDIO_DONGLE_ENABLE)
    handle = bt_sink_srv_cap_stream_get_service_ble_link();
#endif

    return handle;
}

le_audio_device_type_t le_audio_get_device_type(void)
{
    return g_le_audio_device_type;
}

void le_audio_set_device_type(le_audio_device_type_t type)
{
    g_le_audio_device_type = type;

    if (g_le_audio_device_type == LE_AUDIO_DEVICE_TYPE_HEADSET) {
        uint8_t bis_indices[MAX_BIS_NUM] = {1, 2};

        ble_bap_config_sync_big(1, MAX_BIS_NUM, bis_indices);
    }
}

bt_status_t le_audio_read_nvkey_metadata_by_id(le_audio_nvkey_metadata_id_t id, uint8_t *length, uint8_t *value)
{
    uint8_t data[BLE_METADATA_NVKEY_DATA_LEN] = {0};
    nvkey_status_t nvkey_status = NVKEY_STATUS_OK;
    uint32_t size = BLE_METADATA_NVKEY_DATA_LEN;
    uint16_t tag_length, i = 0;

    if (NULL == value || LE_AUDIO_NVKEY_METADATA_NUM <= id) {
        return BT_STATUS_FAIL;
    }

    //MTK_COMMON: Keep NVKEYID_BT_LE_AUDIO_METADATA naming
    nvkey_status = nvkey_read_data(NVKEYID_BT_LE_AUDIO_METADATA, data, &size);
    LE_AUDIO_MSGLOG_I("Read tag id[%d] from NVKEY Metadata, status:0x%x size:0x%x", 3, id, nvkey_status, size);

    if (NVKEY_STATUS_OK == nvkey_status && data[i] != 0) {
        while ((i + 2 < BLE_METADATA_NVKEY_DATA_LEN) && (data[i] != 0)) {
            if ((tag_length = data[i + 1]) == 0) {
                break;
            }

            if (data[i] == id && (i + 2 + tag_length < BLE_METADATA_NVKEY_DATA_LEN)) {
                memcpy(value, &data[i + 2], tag_length);
                *length = tag_length;
                return BT_STATUS_SUCCESS;
            } else {
                i = i + tag_length + 2;
            }
        }
    }

    *length = 0;

    return BT_STATUS_FAIL;
}

bt_status_t le_audio_write_nvkey_metadata_by_id(le_audio_nvkey_metadata_id_t id, uint8_t length, uint8_t *value)
{
    uint8_t data[BLE_METADATA_NVKEY_DATA_LEN] = {0};
    nvkey_status_t nvkey_status = NVKEY_STATUS_ERROR;
    uint32_t size = BLE_METADATA_NVKEY_DATA_LEN;
    uint16_t tag_length, i = 0;

    //MTK_COMMON: Keep NVKEYID_BT_LE_AUDIO_METADATA naming
    if (NVKEY_STATUS_OK == nvkey_read_data(NVKEYID_BT_LE_AUDIO_METADATA, data, &size)) {

        while (((i + 1) < BLE_METADATA_NVKEY_DATA_LEN) && (data[i] != 0)) {
            if ((tag_length = data[i + 1]) == 0) {
                return BT_STATUS_FAIL;
            }

            if (data[i] == id) {
                if (tag_length == length) {
                    break;
                } else {
                    return BT_STATUS_FAIL;
                }

            } else {
                i = i + tag_length + 2;
            }
        }

        if ((i + 2 + length) < BLE_METADATA_NVKEY_DATA_LEN) {
            data[i] = id;
            data[i + 1] = length;
            memcpy(&data[i + 2], value, length);
            //MTK_COMMON: Keep NVKEYID_BT_LE_AUDIO_METADATA naming.
            nvkey_status = nvkey_write_data(NVKEYID_BT_LE_AUDIO_METADATA, data, size);
            LE_AUDIO_MSGLOG_I("Write tag id[%d] to NVKEY Metadata, status:0x%x size:0x%x", 3, id, nvkey_status, size);
            return BT_STATUS_SUCCESS;
        }
    }
    return BT_STATUS_FAIL;
}

bt_status_t le_audio_app_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{
    return bt_app_event_callback(msg, status, buff);
}

