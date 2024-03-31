/* Copyright Statement:
 *
 * @2015 MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek Inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE.
 */
#ifdef HAL_GDMA_MODULE_ENABLED

#include "hal_cache.h"
#include "hal_gdma.h"
#include "hal_gdma_internal.h"

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

#ifndef __UBL__
#include "assert.h"
#else /* #ifndef __UBL__ */
#define assert(expr) log_hal_error("assert\r\n")
#endif /* #ifndef __UBL__ */

typedef struct {
    hal_gdma_callback_t callback;
    void *user_data;
} hal_gdma_callback_context;

static volatile uint8_t gdma_init_status[HAL_GDMA_CHANNEL_MAX] = {0};

static uint32_t hal_gdma_check_valid_channel(hal_gdma_channel_t channel)
{
    uint32_t index = INVALID_INDEX;

    if (channel >= HAL_GDMA_CHANNEL_MAX) {
        index = INVALID_INDEX;
    } else {
        index = channel;
    }
    return index;
}

hal_gdma_status_t hal_gdma_init(hal_gdma_channel_t channel)

{
    uint32_t index;
    hal_gdma_status_t status;

    index = hal_gdma_check_valid_channel(channel);

    if (INVALID_INDEX == index) {
        return HAL_GDMA_STATUS_ERROR_CHANNEL;
    }

    /*set gdma busy*/
    GDMA_CHECK_AND_SET_BUSY(index, status);
    if (HAL_GDMA_STATUS_ERROR == status) {
        return HAL_GDMA_STATUS_ERROR;
    }

    gdma_init(index);

    return  HAL_GDMA_STATUS_OK;
}

hal_gdma_status_t hal_gdma_deinit(hal_gdma_channel_t channel)

{
    uint32_t index;

    volatile uint32_t  global_status = 0;
    index = hal_gdma_check_valid_channel(channel);

    if (INVALID_INDEX == index) {
        return HAL_GDMA_STATUS_ERROR_CHANNEL;
    }

    /* check whether gdma is in running mode  */
    global_status = gdma_get_working_status(index);

    if (global_status & GDMA_START_BIT_MASK) {
        /*gdma is running now,assert here may be better*/
        assert(0);
        return HAL_GDMA_STATUS_ERROR;
    }

    GDMA_SET_IDLE(index);

    return  HAL_GDMA_STATUS_OK;
}

hal_gdma_status_t hal_gdma_start_polling(hal_gdma_channel_t channel, uint32_t destination_address,
                                         uint32_t source_address,  uint32_t data_length)
{
    /*define general dma count variable*/
    uint32_t index;
    volatile uint32_t  global_status = 0;
    index = hal_gdma_check_valid_channel(channel);
    if (INVALID_INDEX == index) {
        return HAL_GDMA_STATUS_ERROR_CHANNEL;
    }

    if (data_length < MIN_LENGHT_VALUE || data_length > MAX_LENGTH_VALUE) {
        return HAL_GDMA_STATUS_INVALID_PARAMETER;
    }

    /* the address for DMA buffer must be 4 bytes aligned */
    if ((destination_address % 4) > 0) {
        assert(0);
        return HAL_GDMA_STATUS_INVALID_PARAMETER;
    }

    /* the address for DMA buffer must be 4 bytes aligned */
    if ((source_address % 4) > 0) {
        assert(0);
        return HAL_GDMA_STATUS_INVALID_PARAMETER;
    }

#ifdef HAL_CACHE_MODULE_ENABLED
    /*the address for DMA buffer must be non-cacheable*/
    if (true == hal_cache_is_cacheable(destination_address)) {
        assert(0);
        return HAL_GDMA_STATUS_INVALID_PARAMETER;
    }

    /*the address for DMA buffer must be non-cacheable*/
    if (true == hal_cache_is_cacheable(source_address)) {
        assert(0);
        return HAL_GDMA_STATUS_INVALID_PARAMETER;
    }
#endif /* #ifdef HAL_CACHE_MODULE_ENABLED */

    /* check whether gdma is in running mode  */
    global_status = gdma_get_working_status(index);

    if (global_status & GDMA_START_BIT_MASK) {
        /*gdma is running now,assert here may be better*/
        assert(0);
        return HAL_GDMA_STATUS_ERROR;
    }

    /*gdma configuration*/
    gdma_set_len(index, data_length);
    gdma_set_address(index, destination_address, source_address);
    gdma_set_iten(index, false);

    /*start gdma transfer*/
    gdma_start(index);

    /* check whether gdma is in running mode  */
    global_status = gdma_get_working_status(index);

    while (global_status & GDMA_START_BIT_MASK) {
        /*gdma is running now*/
        global_status = gdma_get_working_status(index);
    }

    return HAL_GDMA_STATUS_OK;
}

hal_gdma_status_t hal_gdma_start_interrupt(hal_gdma_channel_t channel, uint32_t destination_address,
                                           uint32_t source_address,  uint32_t data_length)
{
    /*define general dma count variable*/
    uint32_t index;
    volatile uint32_t  global_status = 0;
    index = hal_gdma_check_valid_channel(channel);
    if (INVALID_INDEX == index) {
        return HAL_GDMA_STATUS_ERROR_CHANNEL;
    }

    if (data_length < MIN_LENGHT_VALUE || data_length > MAX_LENGTH_VALUE) {
        return HAL_GDMA_STATUS_INVALID_PARAMETER;
    }

    /* the address for DMA buffer must be 4 bytes aligned */
    if ((destination_address % 4) > 0) {
        assert(0);
        return HAL_GDMA_STATUS_INVALID_PARAMETER;
    }

    /* the address for DMA buffer must be 4 bytes aligned */
    if ((source_address % 4) > 0) {
        assert(0);
        return HAL_GDMA_STATUS_INVALID_PARAMETER;
    }

#ifdef HAL_CACHE_MODULE_ENABLED
    /*the address for DMA buffer must be non-cacheable*/
    if (true == hal_cache_is_cacheable(destination_address)) {
        assert(0);
        return HAL_GDMA_STATUS_INVALID_PARAMETER;
    }

    /*the address for DMA buffer must be non-cacheable*/
    if (true == hal_cache_is_cacheable(source_address)) {
        assert(0);
        return HAL_GDMA_STATUS_INVALID_PARAMETER;
    }
#endif /* #ifdef HAL_CACHE_MODULE_ENABLED */

    /* check whether gdma is in running mode  */
    global_status = gdma_get_working_status(index);

    if (global_status & GDMA_START_BIT_MASK) {
        /*gdma is running now,assert here may be better*/
        assert(0);
        return HAL_GDMA_STATUS_ERROR;
    }

    /*gdma configuration*/
    gdma_set_len(index, data_length);
    gdma_set_address(index, destination_address, source_address);
    gdma_set_iten(index, true);

#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_DMA);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    /*start gdma transfer*/
    gdma_start(index);

    return HAL_GDMA_STATUS_OK;
}

hal_gdma_status_t hal_gdma_register_callback(hal_gdma_channel_t channel, hal_gdma_callback_t callback, void *user_data)

{
    uint32_t index;

    index = hal_gdma_check_valid_channel(channel);

    if (INVALID_INDEX == index) {
        return HAL_GDMA_STATUS_ERROR_CHANNEL;
    }

    gdma_register_callback(index, callback, &user_data);

    return HAL_GDMA_STATUS_OK;

}

hal_gdma_status_t hal_gdma_get_running_status(hal_gdma_channel_t channel, hal_gdma_running_status_t *running_status)
{
    /*define peripheral dma global status tmp variable*/
    volatile uint32_t  global_status = 0;

    uint32_t index;

    index = hal_gdma_check_valid_channel(channel);

    if (INVALID_INDEX == index) {
        return HAL_GDMA_STATUS_ERROR_CHANNEL;
    }

    /*  read gdma running  status   */
    global_status = gdma_get_working_status(index);

    if (global_status & GDMA_START_BIT_MASK) {
        *running_status = HAL_GDMA_BUSY;
    } else {
        *running_status = HAL_GDMA_IDLE;
    }

    return HAL_GDMA_STATUS_OK;
}

hal_gdma_status_t hal_gdma_stop(hal_gdma_channel_t channel)
{
    uint32_t index;

    index = hal_gdma_check_valid_channel(channel);

    if (INVALID_INDEX == index) {
        return HAL_GDMA_STATUS_ERROR_CHANNEL;
    }

    return HAL_GDMA_STATUS_OK;
}
#endif /* #ifdef HAL_GDMA_MODULE_ENABLED */
