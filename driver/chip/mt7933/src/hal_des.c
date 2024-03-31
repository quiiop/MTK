/* Copyright Statement:
 *
 * (C) 2005-2022  MediaTek Inc. All rights reserved.
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

#ifdef HAL_DES_MODULE_ENABLED

#include <common.h>

#include "driver_api.h"

#include "hal_des.h"
#include "hal_gcpu_internal.h"
#include "hal_log.h"

#define HAL_DES_MIN_SIZE (16)

#define DES_TYPE_ECB        (0)
#define DES_TYPE_CBC        (1)
#define DES_TYPE_NUM        (2)

#define DES_MODE_DECRYPT    (0)
#define DES_MODE_ENCRYPT    (1)
#define DES_MODE_NUM        (2)

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

static hal_des_status_t hal_des_crypt(hal_des_buffer_t *encrypted_text,
                                      hal_des_buffer_t *plain_text,
                                      hal_des_buffer_t *key,
                                      uint8_t init_vector[HAL_DES_CBC_IV_LENGTH],
                                      uint8_t mode, bool encrypt)
{
    TDES_Param rDesParam;
    unsigned int u4IVResult[20] = {0};
    int i4Ret = -1;

    if ((encrypted_text->length % HAL_DES_BLOCK_SIZES) != 0) {
        log_hal_error("Invalid data length: %lu.", encrypted_text->length);
        return HAL_DES_STATUS_ERROR;
    }

    if (plain_text->length < (encrypted_text->length - HAL_DES_BLOCK_SIZES)) {
        log_hal_error("Plain text buffer length %lu is too small, encrypted length is: %lu",
                      plain_text->length, encrypted_text->length);
        return HAL_DES_STATUS_ERROR;
    }

    if (key->length == HAL_DES_KEY_LENGTH_64) {
        rDesParam.uKeyLen = 0;
    } else if (key->length == HAL_DES_KEY_LENGTH_128) {
        rDesParam.uKeyLen = 1;
    } else if (key->length == HAL_DES_KEY_LENGTH_192) {
        rDesParam.uKeyLen = 2;
    } else {
        log_hal_error("Invalid key length: %lu", key->length);
        return HAL_DES_STATUS_ERROR;
    }

    rDesParam.u4SrcSa = (unsigned long)plain_text->buffer;
    rDesParam.u4DstSa = (unsigned long)encrypted_text->buffer;
    rDesParam.u4Len = encrypted_text->length;
    rDesParam.pbKey = (unsigned char *)key->buffer;
    rDesParam.pbIV = (unsigned char *)init_vector;
    rDesParam.pbFB = (unsigned char *)u4IVResult;

    if (encrypt) {
        switch (mode) {
        case DES_TYPE_ECB: /*_MODE_ECB_*/
	    i4Ret = gcpu_exe_cmd(TDES_DMA_E, &rDesParam);
	    break;
	case DES_TYPE_CBC: /*_MODE_CBC_*/
	    i4Ret = gcpu_exe_cmd(TDES_CBC_E, &rDesParam);
	    break;
	default:
	    log_hal_error("Unknown DES enc mode\n");
	    break;
        }
    } else {
        switch (mode) {
        case DES_TYPE_ECB: /*_MODE_ECB_*/
            i4Ret = gcpu_exe_cmd(TDES_DMA_D, &rDesParam);
	    break;
	case DES_TYPE_CBC: /*_MODE_CBC_*/
	    i4Ret = gcpu_exe_cmd(TDES_CBC_D, &rDesParam);
	    break;
	default:
	    log_hal_error("Unknown DES dec mode\n");
	    break;
        }
    }

    if (i4Ret)
        return HAL_DES_STATUS_ERROR;

    return HAL_DES_STATUS_OK;
}

hal_des_status_t hal_des_cbc_encrypt(hal_des_buffer_t *encrypted_text,
                                     hal_des_buffer_t *plain_text,
                                     hal_des_buffer_t *key,
                                     uint8_t init_vector[HAL_DES_CBC_IV_LENGTH])
{
    hal_des_status_t status;

    if (((encrypted_text == NULL) || plain_text == NULL) || (key == NULL) ||
        (init_vector == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_DES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_des_crypt(encrypted_text, plain_text, key, init_vector,
                           DES_TYPE_CBC, 1);
    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_des_status_t hal_des_cbc_decrypt(hal_des_buffer_t *plain_text,
                                     hal_des_buffer_t *encrypted_text,
                                     hal_des_buffer_t *key,
                                     uint8_t init_vector[HAL_DES_CBC_IV_LENGTH])
{
    hal_des_status_t status;

    if ((plain_text == NULL) || (encrypted_text == NULL) || (key == NULL) ||
        (init_vector == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_DES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_des_crypt(plain_text, encrypted_text, key, init_vector,
                           DES_TYPE_CBC, 0);
    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_des_status_t hal_des_ecb_encrypt(hal_des_buffer_t *encrypted_text,
                                     hal_des_buffer_t *plain_text,
                                     hal_des_buffer_t *key)
{
    uint8_t init_vector[8] = {0};
    hal_des_status_t status;

    if ((plain_text == NULL) || (encrypted_text == NULL) || (key == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_DES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_des_crypt(encrypted_text, plain_text, key, init_vector,
                           DES_TYPE_ECB, 1);
    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_des_status_t hal_des_ecb_decrypt(hal_des_buffer_t *plain_text,
                                     hal_des_buffer_t *encrypted_text,
                                     hal_des_buffer_t *key)
{
    uint8_t init_vector[8] = {0};
    hal_des_status_t status;

    if ((plain_text == NULL) || (encrypted_text == NULL) || (key == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_DES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_des_crypt(plain_text, encrypted_text, key, init_vector,
                           DES_TYPE_ECB, 0);

    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

#endif /* #ifdef HAL_DES_MODULE_ENABLED */
