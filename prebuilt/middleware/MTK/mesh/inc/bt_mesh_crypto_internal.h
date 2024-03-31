/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
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

#ifndef __BT_MESH_CRYPTO_INTERNAL_H__
#define __BT_MESH_CRYPTO_INTERNAL_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "porting.h"

#define MESH_SW_P256 1

/* A macro to create callback structure for encryption callback */
#define CREATE_ENCRYPT_CALLBACK( CB, DATA ) \
    mesh_crypto_cb_t *enc; \
    if((enc = (mesh_crypto_cb_t *)bt_mesh_os_layer_memory_alloc(sizeof(mesh_crypto_cb_t))) == NULL) \
    { \
        MESH_DEBUG_MSG_MUST_INFO("OOM"); \
        if (DATA != NULL) \
            bt_mesh_os_layer_memory_free(DATA); \
        return; \
    } \
    enc->cb = CB; \
    enc->data = DATA;

/* A macro to create callback structure with a return value for encryption callback */
#define CREATE_ENCRYPT_CALLBACK_RET_VALUE( CB, DATA, VALUE ) \
    mesh_crypto_cb_t *enc; \
    if((enc = (mesh_crypto_cb_t *)bt_mesh_os_layer_memory_alloc(sizeof(mesh_crypto_cb_t))) == NULL) \
    { \
        MESH_DEBUG_MSG_MUST_INFO("OOM"); \
        if (DATA != NULL) \
            bt_mesh_os_layer_memory_free(DATA); \
        return VALUE; \
    } \
    enc->cb = CB; \
    enc->data = DATA;

/* Mesh crypto status */
typedef enum {
    MESH_CRYPTO_STATUS_OK = 0,              /**< success */
    MESH_CRYPTO_STATUS_FAIL,                /**< process failed */
    MESH_CRYPTO_STATUS_AUTHEN_FAIL,         /**< authentication failed */
    MESH_CRYPTO_STATUS_ERR_CCM_NO_RESOURCE, /**< no resource */
    MESH_CRYPTO_STATUS_ERR_CCM_BUSY,        /**< system busy */
    MESH_CRYPTO_STATUS_ERR_BAD_INPUT,       /**< bad input parameters */
} mesh_crypto_status_t;

/** This defines the callback function prototype for crypto module.
 *  @param [in] enc_data is the encrypted or decrypted data.
 *  @param [in] status is the status of this process.
 *  @param [in] data is the user data which set during encryption or decryption.
 *  @return NONE
 *  @note the user data should be freed by user.
 */
typedef void (*mesh_crypto_callback)(uint8_t *enc_data, mesh_crypto_status_t status, void *data);

/* Mesh crypto callback structure */
typedef struct {
    mesh_crypto_callback cb;                /**< the callback called when process complete */
    void *data;                             /**< user data */
} mesh_crypto_cb_t;

/**
 * @brief 	This function is used for crypto module initialization
 * @return NONE
 * @note Please use #bt_mesh_init instead of using this api individually.
 */
void mesh_crypto_init(bool enable_bt5);

/**
 * @brief 	Do exclusive-or of two buffer.
 * @param[in] src is the source address
 * @param[in] dst is the destination address
 * @param[in] count is the length of target.
 * @return NONE
 */
void mesh_crypto_xor(const uint8_t *src, uint8_t *dst, uint8_t count);

/**
 * @brief 	Calculate Frame Check Sequence value as defined by 3GPP TS 27.010 with the Polynomial (x^8 + x^2 + x^1 + 1)
 * @param[in] m is the variable length data.
 * @param[in] m_len is the length of m.
 * @return The result FCS value.
 */
uint8_t mesh_crypto_fcs(const uint8_t *m, int m_len);

/**
 * @brief 	Encryption function e, as defined in Volume 3, Part H, Section 2.2.1 of the Core Specification
 * @param[in] key is the parameters for initialization.
 * @param[in] plaintext is the parameters for initialization.
 * @param[out] mac is the result of AES-CMAC.
 * @return
 * #MESH_CRYPTO_STATUS_OK,\n
 * #MESH_CRYPTO_STATUS_FAIL,\n
 * #MESH_CRYPTO_STATUS_ERR_BAD_INPUT\n
 */
mesh_crypto_status_t mesh_crypto_aes_cipher(const uint8_t *key, const uint8_t *plaintext, uint8_t *out);

/*!
 * @brief   This function implement the AES Counter with CBC-MAC (CCM), also known as AES-CCM.
 * @param[in] k is the 128-bit key
 * @param[in] n is a 104-bit nonce
 * @param[in] n_len is the length of nonce
 * @param[in] m is the variable length data to be encrypted and authenticated, also known as "plaintext"
 * @param[in] m_len is the length of m.
 * @param[in] tag is the message integrity check value of m and a, also known as the

MAC. \n
 *                It should be NULL in encrypt mode.
 * @param[in] tag_len The length of tag.
 * @param[in] a is the variable length data to be authenticated, also known as "Additional Data"
 * @param[in] a_len is the length of additional data
 * @param[out] encData is the data after ccm computation.
 * @param[in] encrypt determines encryption or decryption.
 * @return
 * #MESH_CRYPTO_STATUS_OK, \n
 * #MESH_CRYPTO_STATUS_ERR_BAD_INPUT, input parameter is not valid\n
 * #MESH_CRYPTO_STATUS_AUTHEN_FAIL, authentication failed
 */
mesh_crypto_status_t mesh_crypto_aes_ccm_ex(
    const uint8_t *key, const uint8_t *n, uint8_t n_len,
    const uint8_t *m, uint32_t m_len, const uint8_t *tag, uint32_t tag_len,
    const uint8_t *a, uint32_t a_len, uint8_t *encData, bool encrypt);
/*!
 * @brief   This function implement the AES Counter with CBC-MAC (CCM), also known as AES-CCM.
 * @param[in] k is the 128-bit key
 * @param[in] n is a 104-bit nonce
 * @param[in] n_len is the length of nonce
 * @param[in] m is the variable length data to be encrypted and authenticated, also known as "plaintext"
 * @param[in] m_len is the length of m.
 * @param[in] tag is the message integrity check value of m and a, also known as the

MAC. \n
 *                It should be NULL in encrypt mode.
 * @param[in] tag_len The length of tag.
 * @param[in] a is the variable length data to be authenticated, also known as "Additional Data"
 * @param[in] a_len is the length of additional data
 * @param[in] cb is the function callback when CCM is done.
 * @param[in] encrypt determines encryption or decryption.
 * @return
 * #MESH_CRYPTO_STATUS_OK, \n
 * #MESH_CRYPTO_STATUS_ERR_BAD_INPUT, input parameter is not valid
 */
mesh_crypto_status_t mesh_crypto_aes_ccm(const uint8_t *key, const uint8_t *n, uint8_t n_len,
        const uint8_t *m, uint32_t m_len, const uint8_t *tag, uint32_t tag_len,
        const uint8_t *a, uint32_t a_len, mesh_crypto_cb_t *cb, bool encrypt);

/**
 * @brief 	This function implement the Cipher-based Message Authentication Code (CMAC) \n
 * that uses AES-128 as the block cipher function, also known as AES-CMAC.
 * @param[in] k is the 128-bit key.
 * @param[in] m is the variable length data to be authenticated.
 * @param[in] length is the length of m.
 * @param[out] mac is the 128-bit message authentication code (MAC).
 * @return
 * #MESH_CRYPTO_STATUS_OK, \n
 */
mesh_crypto_status_t mesh_crypto_aes_cmac (const uint8_t *k, const uint8_t *m, uint8_t length, uint8_t *mac );

/**
 * @brief 	This function implement the security tool s1 according to Mesh Profile Specification v1.0 3.8.2.4
 * @param[in] m is the variable length data to be authenticated, which is an octect array of ASCII code string
 * @param[in] len is the length of m.
 * @return The result of s1(m)
 */
uint8_t *mesh_crypto_s1(const uint8_t *m,         uint8_t len);

/**
 * @brief 	This function implement the security tool k1 according to Mesh Profile Specification v1.0 3.8.2.5
 * @param[in] n is the variable length data to be authenticated, which can be 0 or more bits, in multiples of 8
 * @param[in] n_len is the length of n.
 * @param[in] salt is a 128-bit salt value.
 * @param[in] p is a pvalue, which can be 0 or more bits, in multiples of 8
 * @param[in] p_len is the length of p.
 * @return The result of k1(n, salt, p), which is a 128-bit value.
 */
uint8_t *mesh_crypto_k1(const uint8_t *n,         uint8_t n_len, const uint8_t *salt, const uint8_t *p, uint8_t p_len);

/**
 * @brief 	This function implement the security tool k2 according to Mesh Profile Specification v1.0 3.8.2.6
 * @param[in] n is a 128-bit plaintext.
 * @param[in] p is a pvalue, which can be 0 or more bits, in multiples of 8
 * @param[in] p_len is the length of p.
 * @return The result of k2(n, p), which is a 263-bit value.
 */
uint8_t *mesh_crypto_k2(const uint8_t *n,
                        const uint8_t *p,
                        uint8_t p_len);

/**
 * @brief 	This function implement the security tool k3 according to Mesh Profile Specification v1.0 3.8.2.7
 * @param[in] n is a 128-bit value
 * @return The result of k3(n), which is a 64-bit value.
 */
uint8_t *mesh_crypto_k3(const uint8_t *n);

/*
 * @brief 	This function implement the security tool k4 according to Mesh Profile Specification v1.0 3.8.2.8
 * @param[in] n is a 128-bit value
 * @return The result of k4(n), which is a 6-bit value.
 */
uint8_t mesh_crypto_k4(const uint8_t *n);

#endif // __BT_MESH_CRYPTO_INTERNAL_H__
