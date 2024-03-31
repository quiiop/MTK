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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ut.h"

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_AES_MODULE_ENABLE)
#include "hal_aes.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
uint8_t key[16] = {
    0x4d, 0x54, 0x4b, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x32, 0x30, 0x31, 0x34, 0x30, 0x38, 0x31, 0x35
};

uint8_t aes_cbc_iv[HAL_AES_CBC_IV_LENGTH] = {
    0x61, 0x33, 0x46, 0x68, 0x55, 0x38, 0x31, 0x43,
    0x77, 0x68, 0x36, 0x33, 0x50, 0x76, 0x33, 0x46
};

uint8_t plain[] = {
    0x00, 0x0b, 0x16, 0x21, 0x2c, 0x37, 0x42, 0x4d, 0x58, 0x63, 0x00, 0x0b, 0x16, 0x21, 0x2c, 0x37,
    0x42, 0x4d, 0x58, 0x63, 0x00, 0x0b, 0x16, 0x21, 0x2c, 0x37, 0x42, 0x4d, 0x58, 0x63, 0x00, 0x00
};

uint8_t plain_iteration_1[] = {
    0x00, 0x0b, 0x16, 0x21, 0x2c, 0x37, 0x42, 0x4d, 0x58, 0x63, 0x00, 0x0b, 0x16, 0x21, 0x2c, 0x37
};

uint8_t plain_iteration_2[] = {
    0x42, 0x4d, 0x58, 0x63, 0x00, 0x0b, 0x16, 0x21, 0x2c, 0x37, 0x42, 0x4d, 0x58, 0x63, 0x00, 0x00
};

uint8_t encrypted_cbc[] = {
    0x85, 0x50, 0xcd, 0x40, 0xe9, 0xe5, 0xc9, 0xf0, 0xe6, 0xa3, 0x3f, 0x7a, 0x76, 0x68, 0xaa, 0x11,
    0x6e, 0xcd, 0x8c, 0x12, 0xf6, 0x15, 0x26, 0xec, 0x30, 0x43, 0x12, 0xab, 0x16, 0xa7, 0x40, 0x66
};
uint8_t encrypted_ecb[] = {
    0x3f, 0x70, 0xf3, 0xca, 0x70, 0x92, 0x91, 0xb1, 0xfc, 0xf7, 0x5d, 0x2a, 0xa8, 0x0f, 0xfe, 0x47,
    0x77, 0x33, 0xfa, 0x01, 0xa6, 0x8d, 0x0a, 0x55, 0xfc, 0xaa, 0x29, 0x3a, 0x1b, 0x9a, 0x5e, 0x78
};

uint8_t encrypted_buffer[32] = {0};
uint8_t decrypted_buffer[32] = {0};

uint8_t encrypted_buffer_iteration_1[16] = {0};
uint8_t encrypted_buffer_iteration_2[16] = {0};
uint8_t decrypted_buffer_iteration_1[16] = {0};
uint8_t decrypted_buffer_iteration_2[16] = {0};

hal_aes_buffer_t plain_text = {
    .buffer = plain,
    .length = sizeof(plain)
};

hal_aes_buffer_t plain_iteration_text_1 = {
    .buffer = plain_iteration_1,
    .length = sizeof(plain_iteration_1)
};

hal_aes_buffer_t plain_iteration_text_2 = {
    .buffer = plain_iteration_2,
    .length = sizeof(plain_iteration_2)
};

hal_aes_buffer_t key_text = {
    .buffer = key,
    .length = sizeof(key)
};

hal_aes_buffer_t encrypted_text = {
    .buffer = encrypted_buffer,
    .length = sizeof(encrypted_buffer)
};

hal_aes_buffer_t encrypted_iteration_text_1 = {
    .buffer = encrypted_buffer_iteration_1,
    .length = sizeof(encrypted_buffer_iteration_1)
};

hal_aes_buffer_t encrypted_iteration_text_2 = {
    .buffer = encrypted_buffer_iteration_2,
    .length = sizeof(encrypted_buffer_iteration_2)
};

hal_aes_buffer_t decrypted_text = {
    .buffer = decrypted_buffer,
    .length = sizeof(decrypted_buffer)
};

hal_aes_buffer_t decrypted_iteration_text_1 = {
    .buffer = decrypted_buffer_iteration_1,
    .length = sizeof(decrypted_buffer_iteration_1)
};

hal_aes_buffer_t decrypted_iteration_text_2 = {
    .buffer = decrypted_buffer_iteration_2,
    .length = sizeof(decrypted_buffer_iteration_2)
};

/* GCM */
uint8_t plain_gcm[80] = {0};
uint8_t key_gcm[16] = {0};
uint8_t iv[16] = {0};
uint8_t aad[16] = {0};
uint8_t tag_out[16] = {0};
uint8_t encrypted_gcm_buffer[60] = {0};
uint8_t decrypted_gcm_buffer[60] = {0};

hal_aes_buffer_t plain_gcm_text = {
    .buffer = plain_gcm,
    .length = sizeof(plain_gcm)
};
hal_aes_buffer_t key_gcm_text = {
    .buffer = key_gcm,
    .length = sizeof(key_gcm)
};

hal_aes_buffer_t iv_text = {
    .buffer = iv,
    .length = sizeof(iv)
};

hal_aes_buffer_t aad_text = {
    .buffer = aad,
    .length = sizeof(aad)
};
hal_aes_buffer_t tag_text = {
    .buffer = tag_out,
    .length = sizeof(tag_out)
};

hal_aes_buffer_t encrypted_gcm_text = {
    .buffer = encrypted_gcm_buffer,
    .length = sizeof(encrypted_gcm_buffer)
};

hal_aes_buffer_t decrypted_gcm_text = {
    .buffer = decrypted_gcm_buffer,
    .length = sizeof(decrypted_gcm_buffer)
};
uint8_t encrypted_gcm[] = {
    0xa3, 0xb2, 0x2b, 0x84, 0x49, 0xaf, 0xaf, 0xbc,
    0xd6, 0xc0, 0x9f, 0x2c, 0xfa, 0x9d, 0xe2, 0xbe,
    0x93, 0x8f, 0x8b, 0xbf, 0x23, 0x58, 0x63, 0xd0,
    0xce, 0x02, 0x84, 0x27, 0x22, 0xfd, 0x50, 0x34,
    0x89, 0x0a, 0xd0, 0xf9, 0xf1, 0x4d, 0xdd, 0x88,
    0xd0, 0x6e, 0x87, 0x1e, 0x48, 0x77, 0x49, 0xfa,
    0x7f, 0x7f, 0xf9, 0x35, 0x5c, 0x0a, 0x18, 0xad,
    0xba, 0x85, 0x87, 0x44
};

uint8_t tag_golden[] = {
    0x1d, 0xc9, 0xe0, 0x4d, 0xe8, 0x87, 0x5d, 0x2e,
    0x81, 0x14, 0x1f, 0xc3, 0x69, 0x0b, 0x47, 0x32,
};


/**
*@brief Log the data in the format of 16 bytes per line.
*@param[in] result: pointer to the data that will be logged out.
*@param[in] length: indicates the length of the data which will be logged out.
*@return None.
*/
static void aes_result_dump(uint8_t *result, uint8_t length)
{
    uint8_t i;

    for (i = 0; i < length; i++) {
        if (i % 16 == 0) {
            printf("\r\n");
        }

        printf(" 0x%02x, ", result[i]);
    }
    printf("\r\n");

}

ut_status_t ut_hal_aes(void)
{
    uint8_t aes_cbc_iv_tmp[HAL_AES_CBC_IV_LENGTH] = {0};

    /* AES GCM Test*/
    hal_aes_gcm_encrypt(&encrypted_gcm_text, &plain_gcm_text, &iv_text, &aad_text, &key_gcm_text, &tag_text);
    printf("Encrypted data(AES GCM):");
    if (memcmp(encrypted_gcm_text.buffer, encrypted_gcm, encrypted_gcm_text.length)) {
        aes_result_dump(encrypted_gcm_text.buffer, encrypted_gcm_text.length);
        printf("AES GCM encryption fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES GCM encryption pass\r\n");
    }
    if (memcmp(tag_text.buffer, tag_golden, tag_text.length)) {
        aes_result_dump(tag_text.buffer, tag_text.length);
        printf("AES EGCM authentication fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES EGCM authentication pass\r\n");
    }
    hal_aes_gcm_decrypt(&decrypted_gcm_text, &encrypted_gcm_text, &iv_text, &aad_text, &key_gcm_text, &tag_text);
    printf("Decrypted data(AES GCM):");
    if (memcmp(decrypted_gcm_text.buffer, plain_gcm_text.buffer, decrypted_gcm_text.length)) {
        aes_result_dump(decrypted_gcm_text.buffer, decrypted_gcm_text.length);
        printf("AES GCM decryption fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES GCM decryption pass\r\n");
    }

    /* AES CBC Test*/
    hal_aes_cbc_encrypt(&encrypted_text, &plain_text, &key_text, aes_cbc_iv);
    printf("Encrypted data(AES CBC):");

    if (memcmp(encrypted_text.buffer, encrypted_cbc, encrypted_text.length)) {
        aes_result_dump(encrypted_text.buffer, encrypted_text.length);
        printf("AES CBC encryption fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES CBC encryption success\r\n");
    }

    hal_aes_cbc_decrypt(&decrypted_text, &encrypted_text, &key_text, aes_cbc_iv);
    printf("Decrypted data(AES CBC):");
    if (memcmp(decrypted_text.buffer, plain, decrypted_text.length)) {
        printf("AES CBC decryption fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES CBC decryption success\r\n");
    }

    /* AES CBC iteration Test*/
    memcpy(aes_cbc_iv_tmp, aes_cbc_iv, HAL_AES_CBC_IV_LENGTH);
    hal_aes_cbc_encrypt_iteration(&encrypted_iteration_text_1,
                                  &plain_iteration_text_1, &key_text, &aes_cbc_iv_tmp[0]);
    hal_aes_cbc_encrypt_iteration(&encrypted_iteration_text_2,
                                  &plain_iteration_text_2, &key_text,
                                  &aes_cbc_iv_tmp[0]);    //aes_cbc_iv_tmp is changed: is the valut of previous XOR

    printf("Encrypted data(AES CBC iteration):");
    if (memcmp(encrypted_iteration_text_1.buffer, encrypted_cbc, encrypted_iteration_text_1.length) ||
        memcmp(encrypted_iteration_text_2.buffer, encrypted_cbc + 16, encrypted_iteration_text_2.length)) {
        aes_result_dump(encrypted_iteration_text_1.buffer, encrypted_iteration_text_1.length);
        aes_result_dump(encrypted_iteration_text_2.buffer, encrypted_iteration_text_2.length);
        printf("AES CBC encryption iteration fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES CBC encryption iteration success\r\n");
    }

    memcpy(aes_cbc_iv_tmp, aes_cbc_iv, HAL_AES_CBC_IV_LENGTH);  //Recover the value of  aes_cbc_iv_tmp
    hal_aes_cbc_decrypt_iteration(&decrypted_iteration_text_1,
                                  &encrypted_iteration_text_1, &key_text, &aes_cbc_iv_tmp[0]);
    hal_aes_cbc_decrypt_iteration(&decrypted_iteration_text_2,
                                  &encrypted_iteration_text_2,
                                  &key_text, &aes_cbc_iv_tmp[0]); //aes_cbc_iv_tmp is changed: is the valut of previous XOR

    printf("Decrypted data(AES CBC iteration):");
    if (memcmp(decrypted_iteration_text_1.buffer, plain_iteration_1, decrypted_iteration_text_1.length) ||
        memcmp(decrypted_iteration_text_2.buffer, plain_iteration_2, decrypted_iteration_text_2.length)) {
        aes_result_dump(decrypted_iteration_text_1.buffer, decrypted_iteration_text_1.length);
        aes_result_dump(decrypted_iteration_text_2.buffer, decrypted_iteration_text_2.length);
        printf("AES CBC Decrypted iteration fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES CBC Decrypted iteration success\r\n");
    }

    hal_aes_cbc_encrypt_ex(&encrypted_text, &plain_text, &key_text, aes_cbc_iv, 0);
    //printf("Encrypted data(aes_cbc_ex):");
    //aes_result_dump(encrypted_text.buffer, encrypted_text.length);
    hal_aes_cbc_decrypt_ex(&decrypted_text, &encrypted_text, &key_text, aes_cbc_iv, 0);
    if (memcmp(decrypted_text.buffer, plain, decrypted_text.length)) {
        printf("AES CBC EX fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES CBC EX success\r\n");
    }

    /*AES ECB Test*/
    hal_aes_ecb_encrypt(&encrypted_text, &plain_text, &key_text);
    printf("Encrypted data(AES ECB):");
    if (memcmp(encrypted_text.buffer, encrypted_ecb, encrypted_text.length)) {
        aes_result_dump(encrypted_text.buffer, encrypted_text.length);
        printf("AES ECB encryption fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES ECB encryption success\r\n");
    }

    hal_aes_ecb_decrypt(&decrypted_text, &encrypted_text, &key_text);
    printf("Decrypted data(AES ECB):");
    if (memcmp(decrypted_text.buffer, plain, decrypted_text.length)) {
        printf("AES ECB decryption fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES ECB decryption success\r\n");
    }

    hal_aes_ecb_encrypt_ex(&encrypted_text, &plain_text, &key_text, 0);
    hal_aes_ecb_decrypt_ex(&decrypted_text, &encrypted_text, &key_text, 0);
    if (memcmp(decrypted_text.buffer, plain, decrypted_text.length)) {
        printf("AES ECB EX fail\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("AES ECB EX success\r\n");
    }

    return UT_STATUS_OK;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_AES_MODULE_ENABLE) */
