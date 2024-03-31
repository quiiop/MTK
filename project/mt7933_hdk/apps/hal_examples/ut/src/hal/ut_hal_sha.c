/* Copyright Statement:
 *
 * (C) 2005-2030  MediaTek Inc. All rights reserved.
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ut.h"

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_SHA_MODULE_ENABLE)
#include "hal_sha.h"

uint8_t *data = (uint8_t *)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopqabcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

uint8_t digest[64] = {0};

uint8_t sha160_digest[] = {
    0xaf, 0xc5, 0x3a, 0x4e, 0xa2, 0x08, 0x56, 0xf9, 0x8e, 0x08, 0xdc, 0x6f, 0x3a, 0x5c, 0x98, 0x33,
    0x13, 0x77, 0x68, 0xed

};
uint8_t sha224_digest[] = {
    0x7d, 0xe2, 0xf9, 0x3b, 0x0d, 0x0a, 0x1f, 0x5c, 0xaf, 0x83, 0x77, 0x39, 0xda, 0x74, 0x16, 0x7a,
    0x03, 0xbd, 0x64, 0xb7, 0x93, 0x06, 0x7e, 0xbd, 0x40, 0x73, 0xd0, 0xdc
};
uint8_t sha256_digest[] = {
    0x59, 0xf1, 0x09, 0xd9, 0x53, 0x3b, 0x2b, 0x70, 0xe7, 0xc3, 0xb8, 0x14, 0xa2, 0xbd, 0x21, 0x8f,
    0x78, 0xea, 0x5d, 0x37, 0x14, 0x45, 0x5b, 0xc6, 0x79, 0x87, 0xcf, 0x0d, 0x66, 0x43, 0x99, 0xcf
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void sha_result_dump(uint8_t *result, uint8_t length)
{
    uint8_t i;

    for (i = 0; i < length; i++) {
        if (i % 16 == 0) {
            printf("\r\n");
        }

        printf(" %02x ", result[i]);
    }
    printf("\r\n");

}

ut_status_t ut_hal_sha(void)
{
    uint32_t size = (uint32_t)strlen((char *)data);
    printf("SHA1/SHA256 data: %s\r\n", (char *)data);

    hal_sha1_context_t sha1_context;
    hal_sha1_init(&sha1_context);
    hal_sha1_append(&sha1_context, data, size);
    hal_sha1_end(&sha1_context, digest);
    printf("SHA1 result:");
    if (memcmp(digest, sha160_digest, HAL_SHA1_DIGEST_SIZE)) {
        sha_result_dump(digest, HAL_SHA1_DIGEST_SIZE);
        printf("SHA1 test fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("SHA1 test success\r\n");
    }
    hal_sha224_context_t sha224_context;
    hal_sha224_init(&sha224_context);
    hal_sha224_append(&sha224_context, data, size);
    hal_sha224_end(&sha224_context, digest);
    printf("SHA224 result:");
    //sha_result_dump(digest, HAL_SHA224_DIGEST_SIZE);
    if (memcmp(digest, sha224_digest, HAL_SHA224_DIGEST_SIZE)) {
        printf("SHA224 test fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("SHA224 test success\r\n");
    }

    hal_sha256_context_t sha256_context;
    hal_sha256_init(&sha256_context);
    hal_sha256_append(&sha256_context, data, size);
    hal_sha256_end(&sha256_context, digest);
    printf("SHA256 result:");
    //sha_result_dump(digest, HAL_SHA256_DIGEST_SIZE);
    if (memcmp(digest, sha256_digest, HAL_SHA256_DIGEST_SIZE)) {
        printf("SHA256 test fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("SHA256 test success\r\n");
    }
    return UT_STATUS_OK;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_SHA_MODULE_ENABLE) */
