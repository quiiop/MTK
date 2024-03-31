/* Copyright Statement:
 *
 * (C) 2022  MediaTek Inc. All rights reserved.
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
#ifdef HAL_AES_MODULE_ENABLED

#include <common.h>

#include "driver_api.h"

#include "hal_aes.h"
#include "hal_gcpu_internal.h"
#include "hal_log.h"

#define AES_TYPE_ECB        (0)
#define AES_TYPE_CBC        (1)
#define AES_TYPE_CTR        (2)
#define AES_TYPE_NUM        (3)

#define AES_MODE_DECRYPT    (0)
#define AES_MODE_ENCRYPT    (1)
#define AES_MODE_NUM        (2)

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

uint32_t key_ptr = 0x2f; //real PTR = 0x30

#if GCPU_DEBUG
static void crypto_dump_buffer(unsigned char *buf, unsigned int size)
{
    int dump_len, i;

    dump_len = (size > 64) ? 64 : size;

    log_hal_info("len to dump = 0x%x\n", dump_len);
    for (i = 0; i < dump_len; i += 8) {
        log_hal_info("0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X",
                     buf[0 + i], buf[1 + i], buf[2 + i], buf[3 + i],
                     buf[4 + i], buf[5 + i], buf[6 + i], buf[7 + i]);
    }
}
#endif /* #if GCPU_DEBUG */

static int generate_secure_key(hal_aes_efuse_key_t key_index, uint32_t key_len)
{
    int32_t i4Ret = 0;

    /* generate_secure_key by load efuse from HRK */
    EF_Param_LDKEY rLdKeyParam;
    rLdKeyParam.eId = (GCPU_EF_LDKEY_ID_T)key_index;

    rLdKeyParam.u2DstSlotHandle = key_ptr;
    rLdKeyParam.key_len = key_len;

    i4Ret = gcpu_exe_cmd(LD_EF_KEY, &rLdKeyParam);
    if (i4Ret) {
        log_hal_error("GCPU load efuse key HRK Fail, i4Ret = %d\n", i4Ret);
        return HAL_AES_STATUS_ERROR;
    }
    return HAL_AES_STATUS_OK;
}

static hal_aes_status_t hal_aes_crypt(hal_aes_buffer_t *encrypted_text,
                                      hal_aes_buffer_t *plain_text,
                                      hal_aes_buffer_t *key,
                                      uint8_t init_vector[HAL_AES_CBC_IV_LENGTH],
                                      uint8_t mode, bool encrypt)
{
    AES_Param rAesParam;
    unsigned int u4IVResult[20] = {0};
    int i4Ret = -1;

    if (key->length == 16) {
        rAesParam.uKeyLen = 0;
    } else if (key->length == 24) {
        rAesParam.uKeyLen = 1;
    } else if (key->length == 32) {
        rAesParam.uKeyLen = 2;
    } else {
        log_hal_error("Error Key Length, key_len must be 16 or 32 but got %d\n",
                      key->length);
        return HAL_AES_STATUS_ERROR;
    }

    rAesParam.u4SrcSa = (unsigned long)plain_text->buffer;
    rAesParam.u4DstSa = (unsigned long)encrypted_text->buffer;
    rAesParam.u4DatLen = plain_text->length;
    rAesParam.pbKey = (unsigned char *)key->buffer;
    rAesParam.pbIV = (unsigned char *)init_vector;
    rAesParam.pbIVResult = (unsigned char *)u4IVResult;

#if GCPU_DEBUG
    log_hal_info("hw aes crypt engine.\n");
    log_hal_info("enc = %d\n", encrypt);
    log_hal_info("mode = %d\n", mode);
    log_hal_info("key_len = %d\n", key->length);
    log_hal_info("src = %p\n", rAesParam.u4SrcSa);
    log_hal_info("src_len = %d\n", rAesParam.u4DatLen);
    log_hal_info("dst = %p\n", rAesParam.u4DstSa);
    log_hal_info("dump src data...\n");
    crypto_dump_buffer(plain_text->buffer, 32);
    log_hal_info("dump key data...\n");
    crypto_dump_buffer(key->buffer, key->length);
    log_hal_info("dump iv data...\n");
    crypto_dump_buffer(init_vector, 16);
#endif /* #if GCPU_DEBUG */

    if (encrypt) {
        switch (mode) {
            case AES_TYPE_ECB: /*_MODE_ECB_*/
                i4Ret = gcpu_exe_cmd(AES_EPAK, &rAesParam);
                break;
            case AES_TYPE_CBC: /*_MODE_CBC_*/
                i4Ret = gcpu_exe_cmd(AES_ECBC, &rAesParam);
                break;
            case AES_TYPE_CTR: /*_MODE_CTR_*/
                i4Ret = gcpu_exe_cmd(AES_CTR, &rAesParam);
                break;
            default:
                log_hal_error("Unknown AES enc mode\n");
                break;
        }
    } else {
        switch (mode) {
            case AES_TYPE_ECB: /*_MODE_ECB_*/
                i4Ret = gcpu_exe_cmd(AES_DPAK, &rAesParam);
                break;
            case AES_TYPE_CBC: /*_MODE_CBC_*/
                i4Ret = gcpu_exe_cmd(AES_DCBC, &rAesParam);
                break;
            default:
                log_hal_error("Unknown AES dec mode\n");
                break;
        }
    }

    if (i4Ret)
        return HAL_AES_STATUS_ERROR;

    return HAL_AES_STATUS_OK;
}

static hal_aes_status_t hal_aes_crypt_iteration(hal_aes_buffer_t *encrypted_text,
                                                hal_aes_buffer_t *plain_text,
                                                hal_aes_buffer_t *key,
                                                uint8_t *init_vector,
                                                bool encrypt)
{
    AES_Param rAesParam;
    int i4Ret = 0;

    if (key->length == 16) {
        rAesParam.uKeyLen = 0;
    } else if (key->length == 24) {
        rAesParam.uKeyLen = 1;
    } else if (key->length == 32) {
        rAesParam.uKeyLen = 2;
    } else {
        log_hal_error("Error Key Length, key_len must be 16/24/32 but got %d\n",
                      key->length);
        return HAL_AES_STATUS_ERROR;
    }

    rAesParam.u4SrcSa = (unsigned long)plain_text->buffer;
    rAesParam.u4DstSa = (unsigned long)encrypted_text->buffer;
    rAesParam.u4DatLen = plain_text->length;
    rAesParam.pbKey = (unsigned char *)key->buffer;
    rAesParam.pbIV = (unsigned char *)init_vector;
    rAesParam.pbIVResult = (unsigned char *)init_vector;

#if GCPU_DEBUG
    log_hal_info("hw aes crypt engine.\n");
    log_hal_info("enc = %d\n", encrypt);
    log_hal_info("key_len = %d\n", key->length);
    log_hal_info("src = %p\n", rAesParam.u4SrcSa);
    log_hal_info("src_len = %d\n", rAesParam.u4DatLen);
    log_hal_info("dst = %p\n", rAesParam.u4DstSa);
    log_hal_info("dump src data...\n");
    crypto_dump_buffer(plain_text->buffer, 32);
    log_hal_info("dump key data...\n");
    crypto_dump_buffer(key->buffer, key->length);
    log_hal_info("dump iv data...\n");
    crypto_dump_buffer(init_vector, 16);
#endif /* #if GCPU_DEBUG */

    if (encrypt) {
        i4Ret = gcpu_exe_cmd(AES_ECBC, &rAesParam);
    } else {
        i4Ret = gcpu_exe_cmd(AES_DCBC, &rAesParam);
    }

    if (i4Ret)
        return HAL_AES_STATUS_ERROR;

    return HAL_AES_STATUS_OK;
}

static hal_aes_status_t hal_aes_gcm_crypt(hal_aes_buffer_t *encrypted_text,
                                          hal_aes_buffer_t *plain_text,
                                          hal_aes_buffer_t *iv,
                                          hal_aes_buffer_t *aad,
                                          hal_aes_buffer_t *key,
                                          hal_aes_buffer_t *tag, bool encrypt)
{
    AES_GCM_Param rAesParam;
    unsigned int u4IVResult[4] = {0, 0, 0, 0};
    int i4Ret = -1;

    if (key->length == 16) {
        rAesParam.uKeyLen = 0;
    } else if (key->length == 24) {
        rAesParam.uKeyLen = 1;
    } else if (key->length == 32) {
        rAesParam.uKeyLen = 2;
    } else {
        log_hal_error("Error Key Length, key_len must be 16/24/32 but got %d\n",
                      key->length);
        return HAL_AES_STATUS_ERROR;
    }

    rAesParam.u4SrcSa = (unsigned long)plain_text->buffer;
    rAesParam.u4DstSa = (unsigned long)encrypted_text->buffer;
    rAesParam.u4DatLen = encrypted_text->length * 8;
    rAesParam.u4IV = (unsigned long)iv->buffer;
    rAesParam.u4IVLen = iv->length * 8;
    rAesParam.u4AAD = (unsigned long)aad->buffer;
    rAesParam.u4AADLen = aad->length * 8;
    rAesParam.pbKey = (unsigned char *)key->buffer;
    rAesParam.pbTag = (unsigned char *)tag->buffer;
    rAesParam.pbIVResult = (unsigned char *)u4IVResult;

    if (encrypt) {
        i4Ret = gcpu_exe_cmd(AES_EGCM, &rAesParam);
    } else {
        i4Ret = gcpu_exe_cmd(AES_DGCM, &rAesParam);
    }

    if (i4Ret)
        return HAL_AES_STATUS_ERROR;

    return HAL_AES_STATUS_OK;
}


static hal_aes_status_t hal_aes_crypt_ex(hal_aes_buffer_t *encrypted_text,
                                         hal_aes_buffer_t *plain_text,
                                         hal_aes_buffer_t *key,
                                         uint8_t init_vector[HAL_AES_CBC_IV_LENGTH],
                                         uint8_t mode, bool encrypt,
                                         hal_aes_efuse_key_t key_index)
{
    AESPK_Param_PAK rAesPAKParam;
    unsigned int u4IVResult[20] = {0};
    int i4Ret = -1;

    /*no need user key*/
    if (key->length != 16 && key->length != 32) {
        log_hal_error("Error Key Length, key_len must be 16/32 but got %d\n",
                      key->length);
        return HAL_AES_STATUS_ERROR;
    }

    // HUK total 256 BITs
    // key_index 0, length 16: HUK[127:0]
    // key_index 0, length 32: HUK[256:0]
    // key_index 1, length 16: HUK[255:128]
    // key_index 1, length 32: not support
    if (key_index == 1 && key->length == 32) {
        log_hal_error("not supported key_index and key_length pair\n");
        return HAL_AES_STATUS_ERROR;
    }

    generate_secure_key(key_index, key->length);
    rAesPAKParam.tKeySlotHandle = key_ptr;

    rAesPAKParam.u4SrcSa = (unsigned long)plain_text->buffer;
    rAesPAKParam.u4DstSa = (unsigned long)encrypted_text->buffer;
    rAesPAKParam.u4DatLen = encrypted_text->length;
    rAesPAKParam.pbIV = (unsigned char *)init_vector;
    rAesPAKParam.pbXOR = (unsigned char *)u4IVResult;
    rAesPAKParam.uMode = 1;
    if (key->length == 32)
        rAesPAKParam.uMode |= (0x1 << 24);

#if GCPU_DEBUG
    log_hal_info("hw aes crypt ex.\n");
    log_hal_info("enc = %d\n", encrypt);
    log_hal_info("mode = %d\n", mode);
    log_hal_info("src = %p\n", rAesPAKParam.u4SrcSa);
    log_hal_info("src_len = %d\n", rAesPAKParam.u4DatLen);
    log_hal_info("dst = %p\n", rAesPAKParam.u4DstSa);
    log_hal_info("dump src data...\n");
    crypto_dump_buffer(plain_text->buffer, 32);
    log_hal_info("dump iv data...\n");
    crypto_dump_buffer(init_vector, 16);
#endif /* #if GCPU_DEBUG */

    if (encrypt) {
        switch (mode) {
            case AES_TYPE_ECB: /*_MODE_ECB_*/
                i4Ret = gcpu_exe_cmd(AESPK_EPAK, &rAesPAKParam);
                break;
            case AES_TYPE_CBC: /*_MODE_CBC_*/
                i4Ret = gcpu_exe_cmd(AESPK_ECBC, &rAesPAKParam);
                break;
            default:
                log_hal_error("Unknown AES enc mode\n");
                break;
        }
    } else {
        switch (mode) {
            case AES_TYPE_ECB: /*_MODE_ECB_*/
                i4Ret = gcpu_exe_cmd(AESPK_DPAK, &rAesPAKParam);
                break;
            case AES_TYPE_CBC: /*_MODE_CBC_*/
                i4Ret = gcpu_exe_cmd(AESPK_DCBC, &rAesPAKParam);
                break;
            default:
                log_hal_error("Unknown AES dec mode\n");
                break;
        }
    }

    if (i4Ret)
        return HAL_AES_STATUS_ERROR;

    return HAL_AES_STATUS_OK;
}

hal_aes_status_t hal_aes_gcm_encrypt(hal_aes_buffer_t *encrypted_text,
                                     hal_aes_buffer_t *plain_text,
                                     hal_aes_buffer_t *iv,
                                     hal_aes_buffer_t *aad,
                                     hal_aes_buffer_t *key,
                                     hal_aes_buffer_t *tag)
{
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (iv == NULL) ||
        (aad == NULL) || (key == NULL) || (tag == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_gcm_crypt(encrypted_text, plain_text, iv, aad,
                               key, tag, 1);
    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_aes_status_t hal_aes_gcm_decrypt(hal_aes_buffer_t *plain_text,
                                     hal_aes_buffer_t *encrypted_text,
                                     hal_aes_buffer_t *iv,
                                     hal_aes_buffer_t *aad,
                                     hal_aes_buffer_t *key,
                                     hal_aes_buffer_t *tag)
{
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (iv == NULL) ||
        (aad == NULL) || (key == NULL) || (tag == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_gcm_crypt(plain_text, encrypted_text, iv, aad,
                               key, tag, 0);
    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_aes_status_t hal_aes_cbc_encrypt(hal_aes_buffer_t *encrypted_text,
                                     hal_aes_buffer_t *plain_text,
                                     hal_aes_buffer_t *key,
                                     uint8_t init_vector[HAL_AES_CBC_IV_LENGTH])
{
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL) ||
        (init_vector == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt(encrypted_text, plain_text, key, init_vector,
                           AES_TYPE_CBC, 1);

    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_aes_status_t hal_aes_cbc_decrypt(hal_aes_buffer_t *plain_text,
                                     hal_aes_buffer_t *encrypted_text,
                                     hal_aes_buffer_t *key,
                                     uint8_t init_vector[HAL_AES_CBC_IV_LENGTH])
{
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL) ||
        (init_vector == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt(plain_text, encrypted_text, key, init_vector,
                           AES_TYPE_CBC, 0);

    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_aes_status_t hal_aes_ctr(hal_aes_buffer_t *encrypted_text,
                             hal_aes_buffer_t *plain_text,
                             hal_aes_buffer_t *key,
                             uint8_t init_vector[HAL_AES_CBC_IV_LENGTH])
{
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL) ||
        (init_vector == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt(encrypted_text, plain_text, key, init_vector,
                           AES_TYPE_CTR, 1);

    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_aes_status_t hal_aes_cbc_encrypt_iteration(hal_aes_buffer_t *encrypted_text,
                                               hal_aes_buffer_t *plain_text,
                                               hal_aes_buffer_t *key,
                                               uint8_t *init_vector)
{
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL) ||
        (init_vector == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt_iteration(encrypted_text, plain_text,
                                     key, init_vector, 1);
    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_aes_status_t hal_aes_cbc_decrypt_iteration(hal_aes_buffer_t *plain_text,
                                               hal_aes_buffer_t *encrypted_text,
                                               hal_aes_buffer_t *key,
                                               uint8_t *init_vector)
{
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL) ||
        (init_vector == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt_iteration(plain_text, encrypted_text,
                                     key, init_vector, 0);
    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_aes_status_t hal_aes_ecb_encrypt(hal_aes_buffer_t *encrypted_text,
                                     hal_aes_buffer_t *plain_text,
                                     hal_aes_buffer_t *key)
{
    uint8_t init_vector[16] = {0};
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt(encrypted_text, plain_text, key, init_vector,
                           AES_TYPE_ECB, 1);

    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_aes_status_t hal_aes_ecb_decrypt(hal_aes_buffer_t *plain_text,
                                     hal_aes_buffer_t *encrypted_text,
                                     hal_aes_buffer_t *key)
{
    uint8_t init_vector[HAL_AES_CBC_IV_LENGTH] = {0};
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt(plain_text, encrypted_text, key, init_vector,
                           AES_TYPE_ECB, 0);

    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}
hal_aes_status_t hal_aes_cbc_encrypt_ex(hal_aes_buffer_t *encrypted_text,
                                        hal_aes_buffer_t *plain_text,
                                        hal_aes_buffer_t *key,
                                        uint8_t init_vector[HAL_AES_CBC_IV_LENGTH],
                                        hal_aes_efuse_key_t key_index)
{
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL) ||
        (init_vector == NULL) || (key_index >= 2)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt_ex(encrypted_text, plain_text, key, init_vector,
                              AES_TYPE_CBC, 1, key_index);

    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}
hal_aes_status_t hal_aes_cbc_decrypt_ex(hal_aes_buffer_t *plain_text,
                                        hal_aes_buffer_t *encrypted_text,
                                        hal_aes_buffer_t *key,
                                        uint8_t init_vector[HAL_AES_CBC_IV_LENGTH],
                                        hal_aes_efuse_key_t key_index)
{
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL) ||
        (init_vector == NULL) || (key_index >= 2)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt_ex(plain_text, encrypted_text, key, init_vector,
                              AES_TYPE_CBC, 0, key_index);
    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_aes_status_t hal_aes_ecb_encrypt_ex(hal_aes_buffer_t *encrypted_text,
                                        hal_aes_buffer_t *plain_text,
                                        hal_aes_buffer_t *key,
                                        hal_aes_efuse_key_t key_index)
{
    uint8_t init_vector[16] = {0};
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL) ||
        (init_vector == NULL) || (key_index >= 2)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt_ex(encrypted_text, plain_text, key, init_vector,
                              AES_TYPE_ECB, 1, key_index);
    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

hal_aes_status_t hal_aes_ecb_decrypt_ex(hal_aes_buffer_t *plain_text,
                                        hal_aes_buffer_t *encrypted_text,
                                        hal_aes_buffer_t *key,
                                        hal_aes_efuse_key_t key_index)
{
    uint8_t init_vector[HAL_AES_CBC_IV_LENGTH] = {0};
    hal_aes_status_t status;

    if ((encrypted_text == NULL) || (plain_text == NULL) || (key == NULL) ||
        (init_vector == NULL) || (key_index >= 2)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_AES_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    status = hal_aes_crypt_ex(plain_text, encrypted_text, key, init_vector,
		              AES_TYPE_ECB, 0, key_index);

    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return status;
}

#endif /* #ifdef HAL_AES_MODULE_ENABLED */
