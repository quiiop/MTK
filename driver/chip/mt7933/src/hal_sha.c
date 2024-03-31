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
#ifdef HAL_SHA_MODULE_ENABLED

#include <string.h>
#include <common.h>

#include "driver_api.h"

#include "hal_gcpu_internal.h"
#include "hal_log.h"
#include "hal_sha.h"
#include "hal_sleep_manager_internal.h"

typedef enum {
    HAL_SHA_INTERNAL_SHA1 = 0,
    HAL_SHA_INTERNAL_SHA224 = 1,
    HAL_SHA_INTERNAL_SHA256 = 2,
} hal_sha_internal_type_t;

/**
 * SHA support structure - common structure.
 */
typedef struct _SHA_CTX_STRUC {
    uint32_t HashValue[16];
    uint32_t MessageLen;
    uint8_t Buff[SHA256_BLOCK_SIZE];
    const uint8_t *Block;
    uint32_t BlockLen;
} SHA_CTX_STRUC, *PSHA_CTX_STRUC;

#define SHA1_CTX_STRUC SHA_CTX_STRUC
#define PSHA1_CTX_STRUC PSHA_CTX_STRUC
#define SHA224_CTX_STRUC SHA_CTX_STRUC
#define PSHA224_CTX_STRUC PSHA_CTX_STRUC
#define SHA256_CTX_STRUC SHA_CTX_STRUC
#define PSHA256_CTX_STRUC PSHA_CTX_STRUC

/* SHA1 constants */
static const UINT32 SHA1_DefaultHashValue[5] = {
    0x01234567UL, 0x89abcdefUL, 0xfedcba98UL, 0x76543210UL,
    0xf0e1d2c3UL,
};

/* SHA224 constants */
static const UINT32 SHA224_DefaultHashValue[8] = {
    0xd89e05c1UL, 0x07d57c36UL, 0x17dd7030UL, 0x39590ef7UL,
    0x310bc0ffUL, 0x11155868UL, 0xa78ff964UL, 0xa44ffabeUL,
};

/* SHA256 constants */
static const UINT32 SHA256_DefaultHashValue[8] = {
    0x67e6096aUL, 0x85ae67bbUL, 0x72f36e3cUL, 0x3af54fa5UL,
    0x7f520e51UL, 0x8c68059bUL, 0xabd9831fUL, 0x19cde05bUL,
};

static void crypt_sha_init(PSHA_CTX_STRUC pSHA_CTX,
		           hal_sha_internal_type_t sha_type)
{
    if (sha_type == HAL_SHA_INTERNAL_SHA1) {
        memcpy(pSHA_CTX->HashValue, SHA1_DefaultHashValue,
	       sizeof(SHA1_DefaultHashValue));
	memset(pSHA_CTX->Buff, 0x00, SHA1_BLOCK_SIZE);
    } else if (sha_type == HAL_SHA_INTERNAL_SHA224) {
        memcpy(pSHA_CTX->HashValue, SHA224_DefaultHashValue,
	       sizeof(SHA224_DefaultHashValue));
	memset(pSHA_CTX->Buff, 0x00, SHA224_BLOCK_SIZE);
    } else if (sha_type == HAL_SHA_INTERNAL_SHA256) {
        memcpy(pSHA_CTX->HashValue, SHA256_DefaultHashValue,
	       sizeof(SHA256_DefaultHashValue));
	memset(pSHA_CTX->Buff, 0x00, SHA256_BLOCK_SIZE);
    }

    pSHA_CTX->Block = NULL;
    pSHA_CTX->MessageLen = 0;
    pSHA_CTX->BlockLen = 0;
} /* End of crypt_sha_init */

/*
========================================================================
Routine Description:
    SHA1/224/256/384/512 computation for n block (n * 512 bits)

Arguments:
    pSHA_CTX        Pointer to SHA_CTX_STRUC

Return Value:
    None

Note:
    None
========================================================================
*/
static void crypt_sha_hash(SHA_CTX_STRUC *pSHA_CTX,
		           hal_sha_internal_type_t sha_type, bool end_flag)
{
    SHA_Param rShaParam;

    rShaParam.fgFirstPacket = FALSE;

    if (!end_flag)
        rShaParam.fgLastPacket = FALSE;
    else
        rShaParam.fgLastPacket = TRUE;
    rShaParam.pbIniHash = (BYTE *) & (pSHA_CTX->HashValue);
    rShaParam.u4DatLen = pSHA_CTX->BlockLen;
    rShaParam.u4SrcSa = (unsigned long)pSHA_CTX->Block;
    rShaParam.u8BitCnt = pSHA_CTX->MessageLen * 8 - pSHA_CTX->BlockLen * 8;
    rShaParam.pbResHash = (BYTE *) & (pSHA_CTX->HashValue);

    if (sha_type == HAL_SHA_INTERNAL_SHA1) {
        rShaParam.iv_count = 5;
	gcpu_exe_cmd(SHA_1, &rShaParam);
    } else if (sha_type == HAL_SHA_INTERNAL_SHA224) {
        rShaParam.iv_count = 8;
	gcpu_exe_cmd(SHA_224, &rShaParam);
    } else if (sha_type == HAL_SHA_INTERNAL_SHA256) {
        rShaParam.iv_count = 8;
        gcpu_exe_cmd(SHA_256, &rShaParam);
    }
    pSHA_CTX->BlockLen = 0;
} /* End of crypt_sha_hash */

/*
========================================================================
Routine Description:
    The message is appended to block. If block size > 64 bytes, the SHA_Hash
will be called.

Arguments:
    pSHA_CTX        Pointer to SHA_CTX_STRUC
    message         Message context
    messageLen      The length of message in bytes

Return Value:
    None
========================================================================
*/
static void crypt_sha_append(PSHA_CTX_STRUC pSHA_CTX, const uint8_t Message[],
			     uint32_t MessageLen,
			     hal_sha_internal_type_t sha_type)
{
    UINT32 diffLen = 0, block_n = 0;
    UINT32 block_size;

    if (sha_type == HAL_SHA_INTERNAL_SHA1) {
	block_size = SHA1_BLOCK_SIZE;
    } else if (sha_type == HAL_SHA_INTERNAL_SHA224) {
        block_size = SHA224_BLOCK_SIZE;
    } else if (sha_type == HAL_SHA_INTERNAL_SHA256) {
        block_size = SHA256_BLOCK_SIZE;
    }

    if ((pSHA_CTX->BlockLen + MessageLen) >= block_size) {
        UINT32 prefix_len = (block_size - pSHA_CTX->BlockLen);

        memcpy(&pSHA_CTX->Buff[pSHA_CTX->BlockLen], Message, prefix_len);
        pSHA_CTX->BlockLen = block_size;
        pSHA_CTX->Block = pSHA_CTX->Buff;
        crypt_sha_hash(pSHA_CTX, sha_type, FALSE);

        block_n = (MessageLen - prefix_len) / block_size;
        diffLen = (MessageLen - prefix_len) % block_size;

        pSHA_CTX->Block = Message + prefix_len;
        if (block_n > 0) {
            /* Leading blocks */
            pSHA_CTX->BlockLen = block_n * block_size;
            crypt_sha_hash(pSHA_CTX, sha_type, FALSE);
            pSHA_CTX->Block = Message + prefix_len + (block_n * block_size);
        }

        memset(pSHA_CTX->Buff, 0x00, block_size);
        memcpy(pSHA_CTX->Buff, pSHA_CTX->Block, diffLen);
    } else {
        memcpy(&pSHA_CTX->Buff[pSHA_CTX->BlockLen], Message, MessageLen);
        diffLen = pSHA_CTX->BlockLen + MessageLen;
    }

    /* Padding blocks */

    pSHA_CTX->MessageLen += MessageLen;
    pSHA_CTX->Block = NULL;
    pSHA_CTX->BlockLen = diffLen;
} /* End of crypt_sha_append */

/*
========================================================================
Routine Description:
    1. Append bit 1 to end of the message
    2. Append the length of message in rightmost 64 bits
    3. Transform the Hash Value to digest message

Arguments:
    pSHA_CTX        Pointer to SHA_CTX_STRUC

Return Value:
    digestMessage   Digest message

Note:
    None
========================================================================
*/
static void crypt_sha_end(PSHA_CTX_STRUC pSHA_CTX, uint8_t DigestMessage[],
                          hal_sha_internal_type_t sha_type)
{
    UINT8 digest_size;

    if (sha_type == HAL_SHA_INTERNAL_SHA1) {
        digest_size = HAL_SHA1_DIGEST_SIZE;
    } else if (sha_type == HAL_SHA_INTERNAL_SHA224) {
        digest_size = HAL_SHA224_DIGEST_SIZE;
    } else if (sha_type == HAL_SHA_INTERNAL_SHA256) {
        digest_size = HAL_SHA256_DIGEST_SIZE;
    }

    pSHA_CTX->Block = pSHA_CTX->Buff;
    crypt_sha_hash(pSHA_CTX, sha_type, TRUE);
    memcpy(DigestMessage, (UINT8 *)pSHA_CTX->HashValue, digest_size);
} /* End of crypt_sha_end */

hal_sha_status_t hal_sha1_init(hal_sha1_context_t *context)
{
    if (context == NULL) {
	log_hal_error("Invalid params (input null).\n");
	return HAL_SHA_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    gcpu_clock_enable();
    gcpu_hw_init();

    crypt_sha_init((SHA_CTX_STRUC *)context, HAL_SHA_INTERNAL_SHA1);

    return HAL_SHA_STATUS_OK;
}

hal_sha_status_t hal_sha1_append(hal_sha1_context_t *context,
		                 const uint8_t *message, uint32_t length)
{
    if ((context == NULL) || (message == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_SHA_STATUS_ERROR;
    }

    crypt_sha_append((SHA1_CTX_STRUC *)context, message, length,
		      HAL_SHA_INTERNAL_SHA1);

    return HAL_SHA_STATUS_OK;
}

hal_sha_status_t hal_sha1_end(hal_sha1_context_t *context,
		              uint8_t digest_message[HAL_SHA1_DIGEST_SIZE])
{
    if ((context == NULL) || (digest_message == NULL)) {
	log_hal_error("Invalid params (input null).\n");
	return HAL_SHA_STATUS_ERROR;
    }

    crypt_sha_end((SHA1_CTX_STRUC *)context, digest_message,
		   HAL_SHA_INTERNAL_SHA1);

    gcpu_clock_disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return HAL_SHA_STATUS_OK;
}

hal_sha_status_t hal_sha224_init(hal_sha224_context_t *context)
{
    if (context == NULL) {
	log_hal_error("Invalid params (input null).\n");
	return HAL_SHA_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    gcpu_clock_enable();
    gcpu_hw_init();

    crypt_sha_init((SHA_CTX_STRUC *)context, HAL_SHA_INTERNAL_SHA224);

    return HAL_SHA_STATUS_OK;
}

hal_sha_status_t hal_sha224_append(hal_sha224_context_t *context,
		                   const uint8_t *message, uint32_t length)
{
    if ((context == NULL) || (message == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_SHA_STATUS_ERROR;
    }

    crypt_sha_append((SHA224_CTX_STRUC *)context, message, length,
		      HAL_SHA_INTERNAL_SHA224);

    return HAL_SHA_STATUS_OK;
}

hal_sha_status_t hal_sha224_end(hal_sha224_context_t *context,
		                uint8_t digest_message[HAL_SHA224_DIGEST_SIZE])
{
    if ((context == NULL) || (digest_message == NULL)) {
	log_hal_error("Invalid params (input null).\n");
	return HAL_SHA_STATUS_ERROR;
    }

    crypt_sha_end((SHA224_CTX_STRUC *)context, digest_message,
		   HAL_SHA_INTERNAL_SHA224);

    gcpu_clock_disable();
#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return HAL_SHA_STATUS_OK;
}

hal_sha_status_t hal_sha256_init(hal_sha256_context_t *context)
{
    if (context == NULL) {
	log_hal_error("Invalid params (input null).\n");
	return HAL_SHA_STATUS_ERROR;
    }

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    gcpu_clock_enable();
    gcpu_hw_init();

    crypt_sha_init((SHA_CTX_STRUC *)context, HAL_SHA_INTERNAL_SHA256);

    return HAL_SHA_STATUS_OK;
}

hal_sha_status_t hal_sha256_append(hal_sha256_context_t *context,
		                   const uint8_t *message, uint32_t length)
{
    if ((context == NULL) || (message == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_SHA_STATUS_ERROR;
    }

    crypt_sha_append((SHA256_CTX_STRUC *)context, message, length,
		      HAL_SHA_INTERNAL_SHA256);

    return HAL_SHA_STATUS_OK;
}

hal_sha_status_t hal_sha256_end(hal_sha256_context_t *context,
		                uint8_t digest_message[HAL_SHA256_DIGEST_SIZE])
{
    if ((context == NULL) || (digest_message == NULL)) {
	log_hal_error("Invalid params (input null).\n");
	return HAL_SHA_STATUS_ERROR;
    }

    crypt_sha_end((SHA256_CTX_STRUC *)context, digest_message,
		   HAL_SHA_INTERNAL_SHA256);

    gcpu_clock_disable();
#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return HAL_SHA_STATUS_OK;
}

#endif /* #ifdef HAL_SHA_MODULE_ENABLED */
