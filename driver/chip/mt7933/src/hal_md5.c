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
#ifdef HAL_MD5_MODULE_ENABLED

#include <string.h>
#include <common.h>

#include "driver_api.h"

#include "hal_gcpu_internal.h"
#include "hal_log.h"
#include "hal_md5.h"
#include "hal_sleep_manager_internal.h"

typedef struct {
    uint32_t HashValue[4];
    uint64_t MessageLen;
    uint8_t *Block;
    uint8_t Buff[MD5_BLOCK_SIZE];
    uint32_t BlockLen;
} MD5_CTX_STRUC, *PMD5_CTX_STRUC;

/* MD5 constants */
const UINT32 MD5_DefaultHashValue[4] = {
    0x67452301UL, 0xefcdab89UL, 0x98badcfeUL, 0x10325476UL
};

static void crypt_md5_hash(MD5_CTX_STRUC *pMD5_CTX, bool end_flag)
{
    MD5_Param rMD5Param;

    rMD5Param.fgFirstPacket = FALSE;

    if (!end_flag)
        rMD5Param.fgLastPacket = FALSE;
    else
        rMD5Param.fgLastPacket = TRUE;

    rMD5Param.pbIniHash = (BYTE *) & (pMD5_CTX->HashValue);
    rMD5Param.u4DatLen = pMD5_CTX->BlockLen;
    rMD5Param.u4SrcSa = (unsigned long)pMD5_CTX->Block;
    rMD5Param.u8BitCnt = pMD5_CTX->MessageLen * 8 - pMD5_CTX->BlockLen * 8;
    rMD5Param.pbResHash = (BYTE *) & (pMD5_CTX->HashValue);
    gcpu_exe_cmd(MD5, &rMD5Param);

    pMD5_CTX->BlockLen = 0;
} /* End of crypt_md5_hash */

static void crypt_md5_init(MD5_CTX_STRUC *pMD5_CTX)
{
    /* Init struct pMD5_CTX */
    memcpy(pMD5_CTX->HashValue, MD5_DefaultHashValue, sizeof(MD5_DefaultHashValue));
    pMD5_CTX->MessageLen = 0;
    pMD5_CTX->Block = NULL;
    memset(pMD5_CTX->Buff, 0x00, MD5_BLOCK_SIZE);
    pMD5_CTX->BlockLen = 0;

#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(LOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    /* HW initialize */
    gcpu_clock_enable();
    gcpu_hw_init();
}

/*
========================================================================
Routine Description:
    The message is appended to block. If block size > 64 bytes, the MD5_Hash
will be called.

Arguments:
    pMD5_CTX        Pointer to MD5_CTX_STRUC
    message         Message context
    messageLen      The length of message in bytes

Return Value:
    None

Note:
    None
========================================================================
*/
static void crypt_md5_append(MD5_CTX_STRUC *pMD5_CTX, uint8_t Message[],
                             uint32_t MessageLen)
{
    UINT32 diffLen = 0, block_n = 0;

    if ((pMD5_CTX->BlockLen + MessageLen) >= MD5_BLOCK_SIZE) {
        UINT32 prefix_len = (MD5_BLOCK_SIZE - pMD5_CTX->BlockLen);

        memcpy(&pMD5_CTX->Buff[pMD5_CTX->BlockLen], Message, prefix_len);
        pMD5_CTX->BlockLen = MD5_BLOCK_SIZE;
        pMD5_CTX->Block = pMD5_CTX->Buff;
        crypt_md5_hash(pMD5_CTX, FALSE);

        block_n = (MessageLen - prefix_len) / MD5_BLOCK_SIZE;
        diffLen = (MessageLen - prefix_len) % MD5_BLOCK_SIZE;

        pMD5_CTX->Block = Message + prefix_len;
        if (block_n > 0) {
            /* Leading blocks */
            pMD5_CTX->BlockLen = block_n * MD5_BLOCK_SIZE;
            crypt_md5_hash(pMD5_CTX, FALSE);
            pMD5_CTX->Block = Message + prefix_len + (block_n * MD5_BLOCK_SIZE);
        }

        memset(pMD5_CTX->Buff, 0x00, MD5_BLOCK_SIZE);
        memcpy(pMD5_CTX->Buff, pMD5_CTX->Block, diffLen);
    } else {
        memcpy(&pMD5_CTX->Buff[pMD5_CTX->BlockLen], Message, MessageLen);
        diffLen = pMD5_CTX->BlockLen + MessageLen;
    }

    /* Padding blocks */
    pMD5_CTX->MessageLen += MessageLen;
    pMD5_CTX->Block = NULL;
    pMD5_CTX->BlockLen = diffLen;
} /* End of crypt_md5_append */

/*
========================================================================
Routine Description:
    1. Append bit 1 to end of the message
    2. Append the length of message in rightmost 64 bits
    3. Transform the Hash Value to digest message

Arguments:
    pMD5_CTX        Pointer to MD5_CTX_STRUC

Return Value:
    digestMessage   Digest message

Note:
    None
========================================================================
*/
static void crypt_md5_end(MD5_CTX_STRUC *pMD5_CTX, uint8_t DigestMessage[])
{
    pMD5_CTX->Block = pMD5_CTX->Buff;

    crypt_md5_hash(pMD5_CTX, TRUE);
    memcpy(DigestMessage, (UINT8 *)pMD5_CTX->HashValue, MD5_DIGEST_SIZE);

    gcpu_clock_disable();
#ifdef HAL_SLEEP_MANAGER_ENABLED
    sleep_management_lock_sleep(UNLOCK_SLEEP, SLEEP_LOCK_GCPU);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
} /* End of crypt_md5_end */

hal_md5_status_t hal_md5_init(hal_md5_context_t *context)
{
    if (context == NULL) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_MD5_STATUS_ERROR;
    }

    crypt_md5_init((MD5_CTX_STRUC *)context);

    return HAL_MD5_STATUS_OK;
}

hal_md5_status_t hal_md5_append(hal_md5_context_t *context, uint8_t *message,
		                uint32_t length)
{
    if ((context == NULL) || (message == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_MD5_STATUS_ERROR;
    }

    crypt_md5_append((MD5_CTX_STRUC *)context, message, length);

    return HAL_MD5_STATUS_OK;
}

hal_md5_status_t hal_md5_end(hal_md5_context_t *context,
		             uint8_t digest_message[HAL_MD5_DIGEST_SIZE])
{
    if ((context == NULL) || (digest_message == NULL)) {
        log_hal_error("Invalid params (input null).\n");
        return HAL_MD5_STATUS_ERROR;
    }

    crypt_md5_end((MD5_CTX_STRUC *)context, digest_message);

    return HAL_MD5_STATUS_OK;
}

#endif /* #ifdef HAL_MD5_MODULE_ENABLED */
