/* Copyright Statement:
 *
 * (C) 2020-2020  MediaTek Inc. All rights reserved.
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

#ifndef __SCOTT_H__
#define __SCOTT_H__


#include <stdint.h>


/**
 * @file scott.h
 *
 * @brief The file scoot.h contains the APIs of MediaTek's secure
 *        chain-of-trust.
 *
 * SCOTT is the abbrevation of MediaTek's Secure Chain-Of-Trust Technology.
 *
 * Image security:
 *
 * PHASE 1: Retrive full public key (Pub)
 *
 *  1. Bootloader find the pub key (called Pub1)) in itself.
 *  2. Bootloader calculates the hash of Pub1: hash(Pub1)
 *  3. Bootloader find the hash in eFUSE, hash(Pub2)
 *  4. If the bootloader is generated legally, then:
 *     hash(Pub1) = hash(Pub2)
 *  5. Pub = Pub1
 *
 * PHASE 2: Verify images (images will not contain public key)
 *
 *  1. Calculate hash (called HashImg) of the image under verification.
 *  2. Verify HashImg with ECDSA, take Pub and hashImg as parameters.
 */


/***************************************************************************
 *
 * SCOTT IMAGE API
 *
 ***************************************************************************/


#define EC256_PUB_KEY_LEN       (91)
#define PUB_KEY_LEN             (EC256_PUB_KEY_LEN)


/**
 * The status values to be returned by SCOTT API functions.
 */
typedef enum {
    SCOTT_STATUS_OK            = 0x00ff,
    SCOTT_STATUS_VERIFY_ERR    = 0x0ff0,
    SCOTT_STATUS_PARAM_ERR     = 0x3333,
    SCOTT_STATUS_IMAGE_ERR     = 0x33cc,
    SCOTT_STATUS_KEY_ERR       = 0x3c3c,
    SCOTT_STATUS_SIGNATURE_ERR = 0x3cc3,
    SCOTT_STATUS_NOT_FOUND     = 0x5555
} scott_status_t;


/**
 * The data structure for holding parsed meta data.
 */
struct scott_image_info {
    void                    *image_addr; /**<
                                          * The memory address of parsed
                                          * image address with some info
                                          * skipped.
                                          */
    uint32_t                image_size;  /**<
                                          * The actual size of the image
                                          * to be processed.
                                          */
};


/**
 * Process the format of an image stored in the memory address (addr) to
 * obtain meta data in it.
 *
 * @param info  Data structure holding the meta info of an image file/memory.
 *
 * @param addr  Memory address of the start of an image.
 *
 * @param size  The length of the image stored in 'addr'.
 *
 * @retval SCOTT_STATUS_PARAM_ERR Parameter(s) not accepted.
 * @retval SCOTT_STATUS_IMAGE_ERR Image format is incorrect.
 * @retval SCOTT_STATUS_OK        Image format is correct. Loaded 'info' with
 *                                parsed information.
 */
scott_status_t scott_image_init(
    struct scott_image_info *info,
    const uint32_t          addr,
    const uint32_t          size
);


/**
 * Retrive the first signature from image if image is packed with a signature.
 *
 * @param info  Data structure holding the meta info of an image file/memory.
 *
 * @param signature     The buffer for retrieved signature.
 *
 * @param signature_len When calling this function, this contains the
 *                      length of the buffer.
 *                      When the API succeeds and returns SCOTT_STATUS_OK,
 *                      it stores the length of the returned buffer.
 *
 * @retval SCOTT_STATUS_PARAM_ERR       Parameter(s) not accepted.
 * @retval SCOTT_STATUS_IMAGE_ERR       Image format is incorrect.
 * @retval SCOTT_STATUS_NOT_FOUND       Signature not found in image.
 * @retval SCOTT_STATUS_SIGNATURE_ERR   Signature is found but format is
 *                                      not recognized.
 * @retval SCOTT_STATUS_OK              Signature found and returend.
 *                                      signature_len is set.
 */
scott_status_t scott_image_signature_get(
    const struct scott_image_info   *info,
    uint8_t                         *signature,
    uint16_t                        *signature_len
);


/**
 * Retrive the first public key from image if image is packed with a public
 * key.
 *
 * @param info  Data structure holding the meta info of an image file/memory.
 *
 * @param key       The buffer for retrieved public key.
 *
 * @param key_len   When calling this function, this contains the
 *                  length of the buffer.
 *                  When the API succeeds and returns SCOTT_STATUS_OK,
 *                  it stores the length of the returned buffer.
 *
 * @retval SCOTT_STATUS_PARAM_ERR   Parameter(s) not accepted.
 * @retval SCOTT_STATUS_IMAGE_ERR   Image format is incorrect.
 * @retval SCOTT_STATUS_NOT_FOUND   Public key not found in image.
 * @retval SCOTT_STATUS_KEY_ERR     Public key is found but format is
 *                                  not recognized.
 * @retval SCOTT_STATUS_OK          Public key found and returend.
 *                                  key_len is set.
 */
scott_status_t scott_image_pub_get(
    const struct scott_image_info   *info,
    void                            *key,
    uint16_t                        *key_len
);


/**
 * Determine whether the public key (pub_key) is valid and trusted in the
 * chain-of-trust of this chip.
 *
 * @param key       The buffer storing a public key in DER format.
 *
 * @param key_len   The length of key in buffer.
 *
 * @retval SCOTT_STATUS_PARAM_ERR   Parameter(s) not accepted.
 * @retval SCOTT_STATUS_KEY_ERR     Public key not recognized.
 * @retval SCOTT_STATUS_OK          Public key found and returned,
 *                                  key_len is set.
 */
scott_status_t scott_image_pub_is_valid(
    const void                      *key,
    const uint16_t                  key_len
);


/**
 * Verify the image with public key.
 *
 * @param info  Data structure holding the meta info of an image file/memory.
 *
 * @param key       The buffer storing a public key in DER format.
 *
 * @param key_len   The length of key in buffer.
 *
 * @param signature The signature to be verified.
 *
 * @param signature_len The length of the signature to be verified.
 *
 * @retval SCOTT_STATUS_PARAM_ERR   Parameter(s) not accepted.
 * @retval SCOTT_STATUS_IMAGE_ERR   Image format is incorrect.
 * @retval SCOTT_STATUS_VERIFY_ERR  Unable to verify the image.
 * @retval SCOTT_STATUS_SIGNATURE_ERR   Signature format is not recognized.
 * @retval SCOTT_STATUS_OK          The image is verified with the given key
 *                                  and signature.
 */
scott_status_t scott_image_verify(
    const struct scott_image_info   *info,
    const void                      *key,
    const uint16_t                  key_len,
    const void                      *signature,
    const uint16_t                  signature_len
);


/**
 * Strip the secure boot wrapper and return the start address and the length
 * of an payload image.
 *
 * @param addr  Supply the start address of a wrapped image. If the address
 *              contains a correctly formatted secure boot image, a stripped
 *              starting address is updated into this parameter. If the image
 *              is not correctly formatted, 'addr' will remain the same.
 *
 * @param size  The length of the image stored in 'addr'. Size is updated if
 *              addr is to be updated.
 *
 * @retval SCOTT_STATUS_PARAM_ERR Parameter(s) not accepted.
 * @retval SCOTT_STATUS_IMAGE_ERR Image format is incorrect.
 * @retval SCOTT_STATUS_OK        Image format is correct. Loaded 'addr' and
 *                                'size' with updated information.
 */
scott_status_t scott_image_strip(
    uint32_t                *addr,
    uint32_t                *size
);


/**
 * Return the corresponding string of a return value.
 *
 * To keep the code bit-alternating safe, the status numbers are defined as
 * non-zero values with multiple bits as 0 and 1.
 *
 * @return The corresponding string.
 */
const char *scott_status_to_string(
    const scott_status_t            status
);


#endif /* #ifndef __SCOTT_H__ */
