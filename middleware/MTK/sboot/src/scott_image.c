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


/****************************************************************************
 *
 * HEADER FILES
 *
 ****************************************************************************/


#include <stdlib.h>
#include <string.h>


// TODO: use features to enable
#ifndef HAL_SHA_MODULE_ENABLED
#define HAL_SHA_MODULE_ENABLED
#endif /* #ifndef HAL_SHA_MODULE_ENABLED */

#include "hal_sha.h"
#include "hal_ecc.h"

#include "hal_gcpu_internal.h" // for gcpu_init

#include "scott.h"

#define __ASIC_ONLY___ 1

/****************************************************************************
 *
 * CONSTANTS AND MACROS
 *
 ****************************************************************************/


//#define __HAL_SHA_LONG_BUFFER_WORKAROUND__


/**
 * @note GPT is used only when verifying a BOOTLOADER image.
 */


/***************************************************************************
 *
 * GPT declaration
 *
 ***************************************************************************/


#define GPT_LBA_SIZE_SIG        (0x455A49532041424C)
#define GPT_HDR_EFI_SIG         (0x5452415020494645)


/***************************************************************************
 *
 * imgtool format declaration
 *
 ***************************************************************************/


#define IMAGE_MAGIC                 (0xbd9fc018)


#define IMAGE_TLV_PROT_INFO_MAGIC   (0x5677)


#define IMAGE_TLV_INFO_MAGIC        (0x5676)


#define IMAGE_TLV_PUB_KEY           (0x60)   /* Public key */


#define IMAGE_TLV_ECDSA256          (0x22)   /* ECDSA of hash output */


/****************************************************************************
 *
 * TYPE DECLARATION
 *
 ****************************************************************************/


struct image_version {
    uint8_t  iv_major;
    uint8_t  iv_minor;
    uint16_t iv_revision;
    uint32_t iv_build_num;
};


struct image_tlv_header {
    uint16_t it_magic;
    uint16_t it_tlv_tot;
    uint8_t  it_tlvs[0];
};

struct image_tlv {
    uint16_t it_type;
    uint16_t it_len;
    uint8_t  it_value[0];
};


struct image_header {
    uint32_t ih_magic;
    uint32_t ih_load_addr;
    uint16_t ih_hdr_size;           /* Size of image header (bytes). */
    uint16_t ih_protect_tlv_size;   /* Size of protected TLV area (bytes). */
    uint32_t ih_img_size;           /* Does not include header. */
    uint32_t ih_flags;              /* IMAGE_F_[...]. */
    struct image_version ih_ver;
    uint32_t _pad1;
};


/***************************************************************************
 *
 * Secure image verification routines
 *
 ***************************************************************************/


#define STATUS_SET(_error_)                     \
    do {                                        \
        g_scott_image_error_line = __LINE__;    \
        status = _error_;                       \
    } while (0)


/*
 * line number storage
 */
volatile uint32_t g_scott_image_error_line;



/***************************************************************************
 * key format
 ***************************************************************************/


#define ECC_KEYSIZE_BYTE_P_256  (32)

#define CT_SHA256_LEN           (256 / 8) // 32 bytes (SHA256)
#define CT_SHA384_LEN           (384 / 8) // 48 bytes (SHA384)

#define EC_KEYSIZE_BYTE         ECC_KEYSIZE_BYTE_P_256
//#define PUB_KEY_LEN             EC256_PUB_KEY_LEN

#define CT_MAX_HASH_LEN         CT_SHA256_LEN


/*
 * public key in DER format.
 */
struct scott_der_pub {
    uint8_t  key[PUB_KEY_LEN];
    uint16_t len;
};


/* Take this process since the data format to ECC is specific */
static void ECC_Input_Reverse_And_To_BE32(uint32_t *ecc_sig_param)
{
    uint8_t  i;

    for (i = 0; i < 4; i++) {
        uint32_t        temp = ecc_sig_param[ i ];
        ecc_sig_param[  i  ] = __builtin_bswap32(ecc_sig_param[7 - i]);
        ecc_sig_param[7 - i] = __builtin_bswap32(temp);
    }
}


/***************************************************************************
 * GPT - GUID Partition Table API
 ***************************************************************************/


static scott_status_t gpt_is_header_gpt(void *addr)
{
    return GPT_LBA_SIZE_SIG == *(uint64_t *)addr ? SCOTT_STATUS_OK : 0;
}


static void gpt_header_skip(struct scott_image_info *info)
{
    if (gpt_is_header_gpt(info->image_addr) == SCOTT_STATUS_OK) {
    } else {
    }

    (void)info;
}


/***************************************************************************
 * MCUBOOT - image header
 ***************************************************************************/


static scott_status_t is_image_hdr(const void *hdr)
{
    const struct image_header *ih = (struct image_header *)hdr;
    return ih->ih_magic == IMAGE_MAGIC ? SCOTT_STATUS_OK : 0;
}

static scott_status_t is_signed_tlvs_hdr(const void *hdr)
{
    const struct image_tlv_header *th = (const struct image_tlv_header *)hdr;
    return th->it_magic == IMAGE_TLV_PROT_INFO_MAGIC ? SCOTT_STATUS_OK : 0;
}

static scott_status_t is_normal_tlvs_hdr(const void *hdr)
{
    const struct image_tlv_header *th = (const struct image_tlv_header *)hdr;
    return th->it_magic == IMAGE_TLV_INFO_MAGIC ? SCOTT_STATUS_OK : 0;
}

struct tlv_iter {
    void        *mem_begin;
    void        *mem_end;
    void        *mem_offs;
};


typedef enum {
    CHECK_SIGNED,
    CHECK_ALL
} tlv_type_t;


typedef enum {
    ITER_OK,
    ITER_PARAM_ERR,
    ITER_FORMAT_ERR,
    ITER_END
} tlv_iter_status_t;


tlv_iter_status_t tlv_iter_begin(
    struct tlv_iter             *iter,
    const struct image_header   *ih,
    tlv_type_t                  signed_only
)
{
    struct image_tlv_header     *th;
    scott_status_t              signed_th;

    if (iter == NULL || ih == NULL)
        return ITER_PARAM_ERR;

    if (is_image_hdr(ih) != SCOTT_STATUS_OK)
        return ITER_FORMAT_ERR;

    th = (void *)ih + ih->ih_hdr_size + ih->ih_img_size;
    signed_th = is_signed_tlvs_hdr(th);

    // make sure the first TLV header is accepted
    if (signed_th != SCOTT_STATUS_OK &&
        ((is_normal_tlvs_hdr(th) != SCOTT_STATUS_OK) || signed_only == CHECK_SIGNED))
        return ITER_FORMAT_ERR;

    // check the length of signed TLV header matches image header
    if (signed_th == SCOTT_STATUS_OK && ih->ih_protect_tlv_size != th->it_tlv_tot)
        return ITER_FORMAT_ERR;

    iter->mem_begin   = th;
    iter->mem_offs    = th;

    if (signed_th == SCOTT_STATUS_OK) {
        void *h = (void *)th + th->it_tlv_tot;
        if (is_normal_tlvs_hdr(h) == SCOTT_STATUS_OK)
            th = h;
    }
    iter->mem_end = (void *)th + th->it_tlv_tot;

    return ITER_OK;
}


/**
 * Iterate over TLVs
 */
tlv_iter_status_t tlv_iter_next(
    struct tlv_iter         *iter,
    const uint16_t          tlv_type,
    struct image_tlv        **tlv_out
)
{
    struct image_tlv_header *th;
    struct image_tlv        *tlv;

    if (iter == NULL || tlv_out == NULL)
        return ITER_PARAM_ERR;

    while (iter->mem_offs < iter->mem_end) {
        // skip TLV header
        th = iter->mem_offs;
        if (is_signed_tlvs_hdr(th) == SCOTT_STATUS_OK ||
            is_normal_tlvs_hdr(th) == SCOTT_STATUS_OK) {
            iter->mem_offs += 4;
            th = iter->mem_offs;
        }

        tlv             = iter->mem_offs;
        iter->mem_offs += sizeof(*tlv) + tlv->it_len;

        if (tlv->it_type == tlv_type) {
            *tlv_out = tlv;
            return ITER_OK;
        }
    }

    return ITER_END;
}


static scott_status_t mcuboot_check_header(
    struct scott_image_info *info
)
{
    scott_status_t          status;

    do {
        STATUS_SET(SCOTT_STATUS_PARAM_ERR);

        if (!info)
            break;

        STATUS_SET(SCOTT_STATUS_IMAGE_ERR);

        if (is_image_hdr(info->image_addr) != SCOTT_STATUS_OK)
            break;

        STATUS_SET(SCOTT_STATUS_OK);
    } while (0);

    return status;
}


static scott_status_t mcuboot_search_tlv(
    const struct scott_image_info   *info,
    const uint16_t                  tlv_type,
    struct image_tlv                **tlv
)
{
    scott_status_t          status;
    struct tlv_iter         iter;
    tlv_iter_status_t       tlv_status;

    do {
        STATUS_SET(SCOTT_STATUS_PARAM_ERR);

        if (!tlv)
            break;

        STATUS_SET(SCOTT_STATUS_NOT_FOUND);

        if (tlv_iter_begin(&iter, info->image_addr, CHECK_ALL) != ITER_OK)
            break;

        tlv_status = tlv_iter_next(&iter, tlv_type, tlv);
        switch (tlv_status) {
            case ITER_PARAM_ERR:
                STATUS_SET(SCOTT_STATUS_IMAGE_ERR);
                break;
            case ITER_OK:
                STATUS_SET(SCOTT_STATUS_OK);
                break;
            case ITER_END:
                STATUS_SET(SCOTT_STATUS_NOT_FOUND);
                break;
            default:
                STATUS_SET(SCOTT_STATUS_IMAGE_ERR);
                break;
        }
    } while (0);

    return status;
}


/***************************************************************************
 *
 * e-FUSE - should use SDK API
 *
 ***************************************************************************/

#define EFUSE1_BASE           (0x30405000)

#define EFUSE_EE_BLK10_01     ((uint32_t *)(EFUSE1_BASE + 0x154))
#define EFUSE_SBC_PUBK_HASH1_FA_DIS (0x1 << 29)
#define EFUSE_SBC_PUBK_HASH0_FA_DIS (0x1 << 30)

#define EFUSE_EE_BLK19_00     ((uint32_t *)(EFUSE1_BASE + 0x1D0))

#define EE_BLK11_00           ((uint32_t *)(EFUSE1_BASE + 0x160))
#define EFUSE_SBC_PUBK_HASH0_DIS    (0x1 << 0)
#define EFUSE_SBC_PUBK_HASH1_DIS    (0x1 << 1)

#define EFUSE_SBC_PUBK_HASH0  ((uint32_t *)(EFUSE1_BASE + 0x100)) /*0x100 ~ 0x11C*/

#define EFUSE_SBC_PUBK_HASH1  ((uint32_t *)(EFUSE1_BASE + 0x120)) /*0x120 ~ 0x13C*/

#define EFUSE_FA_MODE_EN      (0x1 << 2)


#if __ASIC_ONLY___
static bool EFUSE_FA_MODE_IsEnabled(void)
{
    return ((*EFUSE_EE_BLK19_00) & EFUSE_FA_MODE_EN) ? true : false;
}


static uint32_t EFUSE_Get_SBC_MULTI_PUBK_Hash(uint32_t hash[8], uint32_t index)
{
    uint32_t i, ret;

    if (EFUSE_FA_MODE_IsEnabled()) {
        switch (index) {
            case 0:
                ret = (*EFUSE_EE_BLK10_01) & EFUSE_SBC_PUBK_HASH0_FA_DIS;
                break;
            case 1:
                ret = (*EFUSE_EE_BLK10_01) & EFUSE_SBC_PUBK_HASH1_FA_DIS;
                break;
            default:
                ret = 0;
                index = 0;
                break;
        }
    } else {
        switch (index) {
            case 0:
                ret = (*EE_BLK11_00) & EFUSE_SBC_PUBK_HASH0_DIS;
                break;
            case 1:
                ret = (*EE_BLK11_00) & EFUSE_SBC_PUBK_HASH1_DIS;
                break;
            default:
                ret = 0;
                index = 0;
                break;
        }
    }

    if (ret)
        return ret;

    if (index == 0) {
        for (i = 0; i < 8; i++) {
            hash[i] = *(EFUSE_SBC_PUBK_HASH0 + i);
        }
    } else {
        for (i = 0; i < 8; i++) {
            hash[i] = *(EFUSE_SBC_PUBK_HASH1 + (index - 1) * 8 + i);
        }
    }
    return 0;
}
#endif /* #if __ASIC_ONLY___ */


/***************************************************************************
 *
 * UTILITY
 *
 ***************************************************************************/


#if __ASIC_ONLY___
static scott_status_t scott_memcmp(void *ptr1, void *ptr2, uint32_t len)
{
    const uint8_t *p =  ptr1, *q = ptr2;
    scott_status_t status = 0;
    uint8_t not_match = 0;
    size_t i;

    for (i = 0; i < len; i++)
        not_match |= p[i] ^ q[i];

    if (not_match == 0)
        status = SCOTT_STATUS_OK;

    return status;
}
#endif /* #if __ASIC_ONLY___ */


static scott_status_t _calculate_sha_digest(
    const uint8_t   *addr,
    const uint32_t  size,
    uint8_t         *digest
)
{
    hal_sha_context_t ctx;
    uint32_t          len;
#ifdef __HAL_SHA_LONG_BUFFER_WORKAROUND__
    uint32_t          remain = size;

    if (HAL_SHA_STATUS_OK != hal_sha256_init(&ctx))
        return 0;

    while (remain > 0) {
        len = remain > 4096 ? 4096 : remain;

        if (HAL_SHA_STATUS_OK != hal_sha256_append(&ctx, addr, len))
            return 0;

        addr   += len;
        remain -= len;
    }

    if (HAL_SHA_STATUS_OK != hal_sha256_end(&ctx, digest))
        return 0;
#else /* #ifdef __HAL_SHA_LONG_BUFFER_WORKAROUND__ */
    len = size;
    if (HAL_SHA_STATUS_OK != hal_sha256_init(&ctx) ||
        HAL_SHA_STATUS_OK != hal_sha256_append(&ctx, addr, len) ||
        HAL_SHA_STATUS_OK != hal_sha256_end(&ctx, digest))
        return 0;
#endif /* #ifdef __HAL_SHA_LONG_BUFFER_WORKAROUND__ */

    return SCOTT_STATUS_OK;
}


union hash {
    uint8_t     m_u8 [ CT_MAX_HASH_LEN      ];
    uint32_t    m_u32[ CT_MAX_HASH_LEN >> 2 ];
};


#define PUBK_NUM (2)


static scott_status_t _check_hash_in_efuse(uint8_t *digest)
{
#if __ASIC_ONLY___
    union hash      h;
    scott_status_t  status;
    int             i;

    STATUS_SET(SCOTT_STATUS_KEY_ERR);

    for (i = 0; i < 2; i++) {
        if (EFUSE_Get_SBC_MULTI_PUBK_Hash(h.m_u32, i) != 0)
            continue;

        if (scott_memcmp(digest, h.m_u8, CT_MAX_HASH_LEN) == SCOTT_STATUS_OK) {
            STATUS_SET(SCOTT_STATUS_OK);
            break;
        }
    }

    memset(&h, 0, sizeof(h));

    return status;
#endif /* #if __ASIC_ONLY___ */

    return SCOTT_STATUS_OK;
}


struct ecc_sig_vec {
    uint32_t r [ EC_KEYSIZE_BYTE >> 2 ];
    uint32_t s [ EC_KEYSIZE_BYTE >> 2 ];
    uint32_t e [ EC_KEYSIZE_BYTE >> 2 ];
    uint32_t Qx[ EC_KEYSIZE_BYTE >> 2 ];
    uint32_t Qy[ EC_KEYSIZE_BYTE >> 2 ];
    uint32_t v [ EC_KEYSIZE_BYTE >> 2 ];
};


static scott_status_t _sig_check_format(
    const uint8_t   *tlv_val,
    uint8_t         *pad1,
    uint8_t         *pad2)
{
    scott_status_t  status;

    STATUS_SET(SCOTT_STATUS_SIGNATURE_ERR);

    do {
        if (tlv_val[0] != 0x30)
            break;

        STATUS_SET(SCOTT_STATUS_SIGNATURE_ERR);

        if (tlv_val[2] != 0x02)
            break;

        STATUS_SET(SCOTT_STATUS_SIGNATURE_ERR);

        if (tlv_val[3] != 0x21 && tlv_val[3] != 0x20)
            break;

        STATUS_SET(SCOTT_STATUS_SIGNATURE_ERR);

        if (tlv_val[4 + tlv_val[3] + 1] != 0x21 &&
            tlv_val[4 + tlv_val[3] + 1] != 0x20)
            break;

        *pad1 = (tlv_val[3] == 0x21);
        *pad2 = (tlv_val[5 + tlv_val[3]]) == 0x21;

        STATUS_SET(SCOTT_STATUS_OK);
    } while (0);

    return status;
}


static scott_status_t _sig_vec_init(
    struct ecc_sig_vec      *v,
    const void              *dgt,
    const void              *sig,
    const void              *key)
{
    scott_status_t          status;
    uint8_t                 pad1;
    uint8_t                 pad2;
    const uint8_t               *s;
    const struct scott_der_pub  *pub;

    status = _sig_check_format(sig, &pad1, &pad2);
    if (status != SCOTT_STATUS_OK)
        return status;

    pub = key;
    s   = sig;

    memset(v, 0, sizeof(*v));

    memcpy(v->r,  s + 4 + pad1,        EC_KEYSIZE_BYTE);
    memcpy(v->s,  s + 6 + s[3] + pad2, EC_KEYSIZE_BYTE);
    memcpy(v->e,  dgt,                 EC_KEYSIZE_BYTE);
    memcpy(v->Qx, pub->key + 27,       EC_KEYSIZE_BYTE);
    memcpy(v->Qy, pub->key + 59,       EC_KEYSIZE_BYTE);

    ECC_Input_Reverse_And_To_BE32(v->r);
    ECC_Input_Reverse_And_To_BE32(v->s);
    ECC_Input_Reverse_And_To_BE32(v->e);
    ECC_Input_Reverse_And_To_BE32(v->Qx);
    ECC_Input_Reverse_And_To_BE32(v->Qy);

    return SCOTT_STATUS_OK;
}


/***************************************************************************
 *
 * SCOTT IMAGE API
 *
 ***************************************************************************/


scott_status_t scott_image_init(
    struct scott_image_info *info,
    const uint32_t          addr,
    const uint32_t          size
)
{
    scott_status_t          status;

    STATUS_SET(SCOTT_STATUS_PARAM_ERR);

    /*
     * FORMAT: GPT header is optional, identified by GPT magic number
     * +----------+-------+--------+--------------+------------------+
     * |GPT header|img hdr|img body|protected tlvs|non-protected tlvs|
     * +----------+-------+--------+--------------+------------------+
     */

    if (info != NULL && addr != 0 && size != 0) {
        STATUS_SET(SCOTT_STATUS_IMAGE_ERR);

        memset(info, 0, sizeof(struct scott_image_info));

        info->image_addr = (void *)addr;
        info->image_size = size;

        gpt_header_skip(info);

        status = mcuboot_check_header(info);
    }

    return status;
}


scott_status_t scott_image_signature_get(
    const struct scott_image_info   *info,
    uint8_t                         *signature,
    uint16_t                        *signature_len
)
{
    scott_status_t                  status;
    struct image_tlv                *tlv;

    STATUS_SET(SCOTT_STATUS_PARAM_ERR);

    if (info != NULL && signature != NULL && signature_len != NULL) {

        switch (status = mcuboot_search_tlv(info, IMAGE_TLV_ECDSA256, &tlv)) {

            case SCOTT_STATUS_OK:
                if (tlv->it_len > *signature_len) {
                    STATUS_SET(SCOTT_STATUS_SIGNATURE_ERR);
                    break;
                }

                memcpy(signature, &tlv->it_value[0], tlv->it_len);
                *signature_len = tlv->it_len;
                break;

            default:
                break;
        }
    }

    return status;
}


scott_status_t scott_image_pub_get(
    const struct scott_image_info   *info,
    void                            *key,
    uint16_t                        *key_len
)
{
    scott_status_t                  status;
    struct image_tlv                *tlv;
    struct scott_der_pub            *pub;

    STATUS_SET(SCOTT_STATUS_PARAM_ERR);

    if (info != NULL &&
        key != NULL && key_len != NULL && *key_len >= sizeof(*pub)) {

        switch (status = mcuboot_search_tlv(info, IMAGE_TLV_PUB_KEY, &tlv)) {

            case SCOTT_STATUS_OK:

                if (tlv->it_len != PUB_KEY_LEN) {
                    STATUS_SET(SCOTT_STATUS_KEY_ERR);
                    break;
                }

                pub      = key;
                pub->len = tlv->it_len;
                *key_len = sizeof(*pub);
                memcpy(&pub->key[0], &tlv->it_value[0], pub->len);
                break;

            default:
                break;
        }
    }

    return status;
}


scott_status_t scott_image_pub_is_valid(
    const void                      *key,
    const uint16_t                  key_len
)
{
    scott_status_t                  status;
    const struct scott_der_pub      *pub;
    uint8_t                         d_verify[HAL_SHA256_DIGEST_SIZE];

    do {
        STATUS_SET(SCOTT_STATUS_PARAM_ERR);

        if (key == NULL || key_len < sizeof(*pub))
            break;

        STATUS_SET(SCOTT_STATUS_KEY_ERR);

        pub = key;

        if (pub->len != PUB_KEY_LEN)
            break;

        if (pub->key[0] != 0x30 || pub->key[1] != 0x59)
            break;

        if (pub->key[23] != 3 || pub->key[24] != 0x42 || pub->key[26] != 4)
            break;

        STATUS_SET(SCOTT_STATUS_KEY_ERR);

        if (_calculate_sha_digest(&pub->key[0], pub->len, &d_verify[0])
            != SCOTT_STATUS_OK)
            break;

        STATUS_SET(SCOTT_STATUS_KEY_ERR);

        if (_check_hash_in_efuse(&d_verify[0]) != SCOTT_STATUS_OK)
            break;

        STATUS_SET(SCOTT_STATUS_OK);

    } while (0);

    return status;
}


scott_status_t scott_image_verify(
    const struct scott_image_info   *info,
    const void                      *key,
    const uint16_t                  key_len,
    const void                      *signature,
    const uint16_t                  signature_len
)
{
    scott_status_t                  status;

    uint8_t                         *addr;
    uint32_t                        size;
    uint8_t                         digest[HAL_SHA256_DIGEST_SIZE];
    struct image_header             *ih;
    struct ecc_sig_vec              vec;
    const struct scott_der_pub      *pub;
    hal_ecc_status_t                ecc_status;

    do {
        STATUS_SET(SCOTT_STATUS_PARAM_ERR);

        if (info == NULL || key == NULL || key_len < sizeof(*pub))
            break;

        STATUS_SET(SCOTT_STATUS_PARAM_ERR);

        if (signature == NULL)
            break;

        gcpu_init(); // assumed safe to invoke more than once

        STATUS_SET(SCOTT_STATUS_VERIFY_ERR);

        /*
         * 1. calculate the address and length of image.
         * 2. calculate SHA of image (from header to protected TLVs)
         * 3. verify ECC of SHA.
         */

        ih   = (struct image_header *) info->image_addr;
        addr = (uint8_t *)info->image_addr;
        size = (uint32_t)ih->ih_hdr_size + ih->ih_img_size + ih->ih_protect_tlv_size;

        status = _calculate_sha_digest(addr, size, &digest[0]);
        if (status != SCOTT_STATUS_OK)
            break;

        pub = key;

        STATUS_SET(SCOTT_STATUS_IMAGE_ERR);

        status = _sig_vec_init(&vec, &digest[0], signature, &pub->key[0]);
        if (status != SCOTT_STATUS_OK)
            break;

        STATUS_SET(SCOTT_STATUS_VERIFY_ERR);

        if (hal_ecc_init() == false)
            break;
        ecc_status = hal_ecc_ecdsa_verify(HAL_ECC_CURVE_NIST_P_256,
                                          vec.r, vec.s, vec.e, vec.Qx, vec.Qy, vec.v);
        hal_ecc_deinit();

        if (ecc_status != HAL_ECC_STATUS_OK)
            break;

        STATUS_SET(SCOTT_STATUS_VERIFY_ERR);

        if (memcmp(vec.v, vec.r, sizeof(vec.v)) != 0)
            break;

        STATUS_SET(SCOTT_STATUS_OK);

    } while (0);

    return status;
}


scott_status_t scott_image_strip(
    uint32_t                *addr,
    uint32_t                *size
)
{
    scott_status_t          status;
    struct image_header     *ih;
    struct scott_image_info info;

    STATUS_SET(SCOTT_STATUS_PARAM_ERR);

    status = scott_image_init(&info, *addr, *size);
    if (status != SCOTT_STATUS_OK)
        return status;

    ih     = (struct image_header *) info.image_addr;
    *addr  = (uint32_t) info.image_addr + ih->ih_hdr_size;
    *size  = (uint32_t) ih->ih_img_size;

    status = SCOTT_STATUS_OK;

    return status;
}


const char *scott_status_to_string(
    const scott_status_t            status
)
{
    switch (status) {
        case SCOTT_STATUS_OK:
            return "ok";
            break;
        case SCOTT_STATUS_VERIFY_ERR:
            return "verify_err";
            break;
        case SCOTT_STATUS_PARAM_ERR:
            return "param_err";
            break;
        case SCOTT_STATUS_IMAGE_ERR:
            return "image_err";
            break;
        case SCOTT_STATUS_KEY_ERR:
            return "key_err";
            break;
        case SCOTT_STATUS_SIGNATURE_ERR:
            return "signature_err";
            break;
        case SCOTT_STATUS_NOT_FOUND:
            return "not_found";
            break;
        default:
            return "unknown";
            break;
    }
}

