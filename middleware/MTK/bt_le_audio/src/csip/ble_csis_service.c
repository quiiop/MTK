/*
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
#include "bt_sink_srv_ami.h"

#include "ble_csis.h"
#include "ble_csis_def.h"

#include "nvkey.h"
#include "nvkey_id_list.h"
#include "bt_le_audio_msglog.h"
#include "bt_le_audio_util.h"
#include "bt_le_audio_util_nvkey_struct.h"


extern bool ble_csis_set_cccd(bt_handle_t handle, uint16_t attr_handle, uint16_t value);
extern bt_le_audio_cccd_record_t* ble_csis_get_cccd(bt_handle_t handle, uint32_t *num);

#define MTK_COMMON_LE_CSIS

#ifdef AIR_LE_AUDIO_HEADSET_ENABLE
#define BLE_CSIS_DEFAULT_COORDINATED_SET_SIZE   1
#else
#define BLE_CSIS_DEFAULT_COORDINATED_SET_SIZE   2
#endif

#define BLE_CSIS_DEFAULT_SET_MEMBER_RANK        1

static const bt_key_t default_sirk = {0xf3, 0x0b, 0x65, 0x1f, 0x26, 0x84, 0xd7, 0xe1, 0x5f, 0x25, 0xd9, 0x48, 0x60, 0x53, 0xa4, 0x97};

void ble_csis_init_parameter(void)
{
    bt_lea_csis_data_nvkey_t data_nvkey;
    bt_key_t irk = {0};
    nvkey_status_t nvkey_status = NVKEY_STATUS_OK;
    uint32_t size = BLE_CSIS_NVKEY_DATA_LEN;

    /* NVKEYID_BT_LE_AUDIO_CSIS_DATA: | SIRK (16 bytes) | size (1 byte) | rank (1 byte) | */
    //MTK_COMMON: Keep original NVKEYID_BT_LE_AUDIO_CSIS_DATA naming
    nvkey_status = nvkey_read_data(NVKEYID_BT_LE_AUDIO_CSIS_DATA, (uint8_t*)&data_nvkey, &size);
    LE_AUDIO_MSGLOG_I("[CSIS] read NVKEY CSIS data, status:0x%x size:0x%x", 2, nvkey_status, size);

    /* SET SINK */
    if ((NVKEY_STATUS_OK == nvkey_status) && (0 != memcmp(irk, (uint8_t*)&data_nvkey, sizeof(bt_key_t)))) {
        memcpy((uint8_t *)&irk, (uint8_t*)&data_nvkey, sizeof(bt_key_t));
    } else {
        memcpy((uint8_t *)&irk, (uint8_t *)&default_sirk, sizeof(bt_key_t));
    }
    ble_csis_set_sirk(irk);

    /* SET SIZE and RANK */
    data_nvkey.set_size = (NVKEY_STATUS_OK == nvkey_status) ? data_nvkey.set_size : BLE_CSIS_DEFAULT_COORDINATED_SET_SIZE;
    ble_csis_set_coordinated_set_size(data_nvkey.set_size);

    if (data_nvkey.set_size > 1) {
        audio_channel_t channel = ami_get_audio_channel();
        ble_csis_set_coordinated_set_rank((channel == AUDIO_CHANNEL_R) ? 2 : 1);
    } else {
        ble_csis_set_coordinated_set_rank(1);
    }

}

void ble_csis_write_nvkey_sirk(bt_key_t *sirk)
{
    bt_lea_csis_data_nvkey_t data_nvkey;
    nvkey_status_t nvkey_status = NVKEY_STATUS_ERROR;
    uint32_t size = BLE_CSIS_NVKEY_DATA_LEN;

    //MTK_COMMON: Keep original NVKEYID_BT_LE_AUDIO_CSIS_DATA naming
    if (NVKEY_STATUS_OK == nvkey_read_data(NVKEYID_BT_LE_AUDIO_CSIS_DATA, (uint8_t*)&data_nvkey, &size)) {
        memcpy(&data_nvkey.sirk, (uint8_t *)sirk, sizeof(bt_key_t));
        nvkey_status = nvkey_write_data(NVKEYID_BT_LE_AUDIO_CSIS_DATA, (uint8_t*)&data_nvkey, size);
    }

    LE_AUDIO_MSGLOG_I("[CSIS] write NVKEY SIRK, status:%x", 1, nvkey_status);
}

bool ble_csis_set_cccd_handler(bt_handle_t conn_handle, uint16_t attr_handle, uint16_t value)
{
    if (attr_handle < CSIS_END_HANDLE && attr_handle > CSIS_START_HANDLE) {
        ble_csis_set_cccd(conn_handle, attr_handle, value);
        return true;
    }
    return false;
}

bt_le_audio_cccd_record_t* ble_csis_get_cccd_handler(bt_handle_t conn_handle, uint32_t *num)
{
    if (num == NULL) {
        return NULL;
    }

    return ble_csis_get_cccd(conn_handle, num);
}
