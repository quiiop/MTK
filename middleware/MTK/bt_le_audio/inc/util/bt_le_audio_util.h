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

#ifndef __BT_LE_AUDIO_UTIL_H__
#define __BT_LE_AUDIO_UTIL_H__

#include "bt_type.h"

/**
 * @brief Defines the Metadata Tag ID.
 */
typedef enum {
    LE_AUDIO_NVKEY_METADATA_BROADCAST_CODE = 1,   /**< The tag ID is broadcast code. Fixed size: 16 bytes */
    LE_AUDIO_NVKEY_METADATA_NUM,                  /**< The total number of the tag ID. */
} le_audio_nvkey_metadata_id_t;                   /**< The NVKEY metadata tag ID. */

/**
 * @brief Defines the device type.
 */
#define LE_AUDIO_DEVICE_TYPE_EARBUDS     0x01    /**< Earbuds. */
#define LE_AUDIO_DEVICE_TYPE_HEADSET     0x02    /**< Headset. */
#define LE_AUDIO_DEVICE_TYPE_UNKNOWN     0x03    /**< Unknown device. */
typedef uint8_t le_audio_device_type_t;          /**< The type of the device. */

/**
 * @brief Defines the timer ID.
 */
#define LE_AUDIO_TIMER_ID_MAX       10  /**< Maxium timer ID. */
typedef uint32_t le_audio_timer_id_t;   /**< The type of timer ID. */

/**
 *  @brief This structure defines the cccd record.
 */
typedef struct {
    uint16_t attr_handle;
    uint16_t cccd_value;
} bt_le_audio_cccd_record_t;

/**
 * @brief Defines the structure of set CCCD callback.
 */
typedef bool (*le_audio_set_cccd_callback_t)(bt_handle_t handle, uint16_t attr_handle, uint16_t value);

/**
 * @brief Defines the structure of get CCCD callback.
 */
typedef bt_le_audio_cccd_record_t* (*le_audio_get_cccd_callback_t)(bt_handle_t handle, uint32_t *num);

/**
 * @brief Defines the structure of timer callback.
 */
typedef void (*le_audio_timeout_callback_t)(void *p_data);

/**
 * @brief                       This function alloc a block of size bytes of memeory.
 * @param[in] size              is the size of memeory block in bytes.
 * @return                      with NULL pointer, if command fail. Otherwise, success.
 */
void *le_audio_malloc(uint32_t size);

/**
 * @brief                       This function free the memory block that perviously allocated.
 * @param[in] ptr               is the pointer of memory block.
 */
void le_audio_free(void *ptr);

/**
 * @brief                       This function set the first number of memory block to specified value.
 * @param[in] ptr               is the pointer of memory block.
 * @param[in] value             is the value to be set.
 * @param[in] size              is the number of bytes to be set to the value.
 */
void *le_audio_memset(void *ptr, int32_t value, uint32_t size);

/**
 * @brief                       This function copies the value from the source location to the destination.
 * @param[in] dest              is the pointer of destination.
 * @param[in] src               is the pointer of source of data to be copied.
 * @param[in] size              is the number of bytes to be copied.
 */
void *le_audio_memcpy(void *dest, const void *src, uint32_t size);

/**
 * @brief                       This function compare the value between the source location to the destination.
 * @param[in] dest              is the pointer of destination.
 * @param[in] src               is the pointer of source of data.
 * @param[in] size              is the number of bytes to be compare.
 * @return                      the value 0 indicates the contents of both memory block are equal, otherwise different.
 */
int32_t le_audio_memcmp(const void *dest, const void *src, uint32_t size);

/**
 * @brief                       This function initializes timer.
 */
void le_audio_init_timer(void);

/**
 * @brief                       This function create a timer.
 * @param[in] timeMs            is the timeout in milliseconds.
 * @param[in] callback          is the function pointer of the callback. This callback will be called when the timer expires.
 * @param[in] p_data            is the pointer to the user defined data to pass to the callback function.
 * @return                      is the timer ID.
 */
le_audio_timer_id_t le_audio_create_timer(uint32_t timeMs, le_audio_timeout_callback_t callback, void *p_data);

/**
 * @brief                       This function start the timer.
 * @param[in] id                is the timer ID.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t le_audio_start_timer(le_audio_timer_id_t id);

/**
 * @brief                       This function stop the timer.
 * @param[in] id                is the timer ID.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t le_audio_stop_timer(le_audio_timer_id_t id);

/**
 * @brief                       This function reset the timer.
 * @param[in] id                is the timer ID.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t le_audio_reset_timer(le_audio_timer_id_t id);

/**
 * @brief                       This function delete the timer.
 * @param[in] id                is the timer ID.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t le_audio_delete_timer(le_audio_timer_id_t id);


bt_status_t le_audio_app_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff);

/**
 * @brief                       This function gets the device type.
 * @return                      is the device type.
 */
le_audio_device_type_t le_audio_get_device_type(void);

/**
 * @brief                       This function sets the device type.
 * @param[in] type              is the device type.
 */
void le_audio_set_device_type(le_audio_device_type_t type);

/**
 * @brief                       This function reads the value of the specified tag id of the NVKEY metadata section.
 * @param[in] id                is the tag id to be read.
 * @param[out] length           is the length of the specified tag id.
 * @param[out] value            is the value of the specified tag id.
 */
bt_status_t le_audio_read_nvkey_metadata_by_id(le_audio_nvkey_metadata_id_t id, uint8_t *length, uint8_t *value);

/**
 * @brief                       This function writes the value of the specified tag id of the NVKEY metadata section.
 * @param[in] id                is the tag id to be written.
 * @param[in] length            is the length of the specified tag id.
 * @param[in] value             is the value of the specified tag id.
 */
bt_status_t le_audio_write_nvkey_metadata_by_id(le_audio_nvkey_metadata_id_t id, uint8_t length, uint8_t *value);

#endif  /* __BT_LE_AUDIO_UTIL_H__ */

