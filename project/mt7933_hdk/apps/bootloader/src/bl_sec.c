/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2022-2022. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */


#ifdef MTK_SECURE_BOOT_ENABLE


#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "scott.h"
#include "hal_efuse_get.h"
#include "hw_uart.h"
#include "bl_image.h"
#include "bl_mem.h"
#include "bl_sec.h"
#include "bl_test.h"
#include "bl_util.h"


/***************************************************************************
 * DATA STRUCTURES
 ***************************************************************************/


typedef struct image_header image_header_t;


typedef struct image_tlv_info image_tlv_info_t;


#define L  hw_uart_printf("sboot %d\n", __LINE__);


/***************************************************************************
 * PRIVATE FUNCTIONS
 ***************************************************************************/


#define IS_EMPTY( _v )  ( ( _v ) == (uint32_t)0 || ( _v ) == (uint32_t)~0 )


/*
 * calculate target address and length
 */
static void _bl_sec_get_target(const uint32_t  image,
                               uint32_t        *addr,
                               uint32_t        *len)
{
    const image_header_t    *h = (const image_header_t *)image;
    image_tlv_info_t        *tlvh;

    *addr = h->ih_load_addr - h->ih_hdr_size;

    *len  = h->ih_hdr_size +
            h->ih_img_size +
            h->ih_protect_tlv_size;  /* protected */

    tlvh  = (void *)h + *len;        /* tip of unprotected TLVs */

    *len  = *len + tlvh->it_tlv_tot; /* protected + unprotected */
}


bl_status_t bl_sec_verify_sig(const bl_region_t *region)
{
    scott_status_t              scott_status;
    uint16_t                    pub_key_len;
    uint16_t                    signature_len;
    struct scott_image_info     info;
    static uint8_t              pub_key[128];
    static uint8_t              signature[512];
    uint32_t                    non_cache_addr;

    EMU_TRISTATE_RETURN( g_emu_tri_verified, BL_STATUS_OK,
                                             BL_STATUS_IMG_VERIFY_FAIL );

    if (region == NULL)
        return BL_STATUS_INVALID_PARAM;

    non_cache_addr = HAL_CACHE_VIRTUAL_TO_PHYSICAL(region->addr);

    //hw_uart_printf("addr virtual : %x\n", (int)region->addr);
    //hw_uart_printf("     physical: %x\n", (int)non_cache_addr);

    scott_status = scott_image_init(&info, non_cache_addr, region->size);
    if (scott_status != SCOTT_STATUS_OK) {
        hw_uart_printf("scott_image_init: %x!\n", scott_status);
        return BL_STATUS_IMG_VERIFY_FAIL;
    }

    //hw_uart_printf("     imgaddr: %x\n", (int)info.image_addr);
    //hw_uart_printf("     physical: %x\n", (int)non_cache_addr);

    pub_key_len = sizeof(pub_key);
    scott_status = scott_image_pub_get(&info, &pub_key[0], &pub_key_len);
    if (scott_status !=  SCOTT_STATUS_OK) {
        hw_uart_printf("scott_image_pub_get: 0x%x\n", scott_status);
        return BL_STATUS_IMG_VERIFY_FAIL;
    }

    scott_status = scott_image_pub_is_valid(&pub_key[0], pub_key_len);
    if (scott_status !=  SCOTT_STATUS_OK) {
        hw_uart_printf("invalid key 0x%x\n", scott_status);
        return BL_STATUS_IMG_VERIFY_FAIL;
    }

    signature_len = sizeof(signature);
    scott_status = scott_image_signature_get(&info, &signature[0], &signature_len);
    if (scott_status !=  SCOTT_STATUS_OK) {
        hw_uart_printf("scott_image_signature_get: 0x%x\n", scott_status);
        return BL_STATUS_IMG_VERIFY_FAIL;
    }

    scott_status = scott_image_verify(&info, &pub_key[0], pub_key_len, &signature[0], signature_len);
    if (scott_status !=  SCOTT_STATUS_OK) {
        hw_uart_printf("scott_image_verify 0x%x\n", scott_status);
        return BL_STATUS_IMG_VERIFY_FAIL;
    }

    return BL_STATUS_OK;
}


/*
 * Look for imgtool header and fallback to check the exsitence of a
 * non-secure firmware if permissive mode is enabled.
 *
 * @retval BL_STATUS_INVALID_PARAM
 * @retval BL_STATUS_NOT_FOUND
 * @retval BL_STATUS_OK
 */
bl_status_t bl_sec_get_image_offset(uint32_t addr, uint32_t *offset)
{
    *offset = 0;

    image_header_t *header = (image_header_t *)addr;

    if (header == NULL)
        return BL_STATUS_INVALID_PARAM;

    if (header->ih_magic == IMAGE_MAGIC)
        *offset = header->ih_hdr_size;
    else {
#if MTK_BL_SECURE_BOOT_PERMISSIVE_ENABLE == 1
        EMU_TRISTATE_RUN_IF_TRUE(
            g_emu_tri_strict,
            return BL_STATUS_NOT_FOUND);
#else
        return BL_STATUS_NOT_FOUND;
#endif
    }

    return BL_STATUS_OK;
}


static bool _bl_sec_has_hdr(const bl_region_t *region)
{
    const image_header_t *header;

    header = (image_header_t *)region->addr;
    return header->ih_magic == IMAGE_MAGIC;
}


static void _bl_sec_region_copy(bl_region_t *dst, const bl_region_t *src)
{
    size_t offset;

    memcpy( dst, src, sizeof( *dst ) );

    offset = dst->addr & 0xFF;
    dst->addr -= offset;
    dst->size += offset;

    EMU_TRISTATE_UPDATE( dst->verify, g_emu_tri_verify );
    EMU_TRISTATE_UPDATE( dst->ram, g_emu_tri_ram );
}


/***************************************************************************
 * PUBLIC FUNCTIONS
 ***************************************************************************/


/*
 * @retval BL_STATUS_OK
 *         Return this if:
 *         1. Secure Boot Check (SBC) isn't enabled in eFUSE.
 *         2. All images were verified successfully.
 *
 * @retval BL_STATUS_IMG_VERIFY_FAIL
 *         If any of secure images failed
 *
 * @retval BL_STATUS_SEC_FATAL
 *         If a physical attack happened
 */
bl_status_t bl_sec_verify_all(const bl_region_t        *regions,
                              bl_sec_verify_callback_t  update)
{
    volatile bl_status_t    status = BL_STATUS_SEC_FATAL;
    const bl_region_t       *ptr;
    bl_region_t             region;

    ptr = &regions[1];

    while ( ptr->id != ROM_REGION_MAX )
    {
        volatile bl_status_t s = BL_STATUS_OK;

        _bl_sec_region_copy( &region, ptr );

#if defined(MTK_RELEASE_MODE) && defined(MTK_MFG_VERIFY)
        if ( ptr->id == ROM_REGION_MFG )
        {
            // skip manufacturing region if erased
            if ( BL_STATUS_NOT_FOUND == bl_image_mfg_check( region.addr ) ) {
                ptr++;
                continue;
            }
        }
#endif

        if ( ! _bl_sec_has_hdr( &region ) ) {
            if ( is_sbc_enable() && region.verify ) {
#if MTK_BL_SECURE_BOOT_PERMISSIVE_ENABLE == 1
                EMU_TRISTATE_RUN_IF_TRUE(
                    g_emu_tri_strict,
                    exit_with_status(true, BL_STATUS_SEC_NO_HDR,
                                     region.addr, s)
                );

                hw_uart_printf("sboot warn: no hdr\n", region.addr, s);
#else
                exit_with_status(true, BL_STATUS_SEC_NO_HDR, region.addr, s);
#endif
            }

            if (update) {
                exit_with_status( BL_STATUS_OK != update(&region),
                                  BL_STATUS_SEC_FATAL,
                                  region.addr, region.size );
            }
            ptr++;
            continue;
        }

        /* add source to memeory map and check it */
        uint32_t saddr = region.addr, slen = region.size;
        exit_with_status( ! bl_mem_ins( saddr, slen ),
                            BL_STATUS_SEC_INV_SRC, saddr, slen );

        /* convey the target address in source region */
        uint32_t taddr, tlen;
        _bl_sec_get_target( saddr, &taddr, &tlen );

        /* check whether the region complies to RAM limit */
        if ( ! is_in_ram( (void *)taddr, tlen ) ) {
            exit_with_status( region.ram, BL_STATUS_SEC_RAM, taddr, tlen );
        } else {
            exit_with_status( ! bl_mem_ins( taddr, tlen ),
                              BL_STATUS_SEC_INV_TGT, taddr, tlen );
            exit_with_status( tlen > slen,
                              BL_STATUS_SEC_TGT_SIZE, tlen, slen );
            /* load region into RAM */
            memcpy( (void *)taddr, (void *)saddr, tlen );
            region.addr = taddr;
            region.size = tlen;
        }

        if ( is_sbc_enable() && region.verify ) {
            s = bl_sec_verify_sig( &region );
            if ( s != BL_STATUS_OK ) {
#if MTK_BL_SECURE_BOOT_PERMISSIVE_ENABLE == 1
                EMU_TRISTATE_RUN_IF_TRUE(
                    g_emu_tri_strict,
                    exit_with_status(true, BL_STATUS_SEC_VERIFY,
                                     region.addr, s)
                );
                
                hw_uart_printf( "sboot warn: verify fail %x %x\n",
                                region.addr, s );
#else
                exit_with_status(true, BL_STATUS_SEC_VERIFY, 
                                 region.addr, s);
#endif
            }
        }

        if (update) {
            exit_with_status( BL_STATUS_OK != update( &region ),
                              BL_STATUS_SEC_FATAL,
                              region.addr, region.size );
        }
        ptr++;
    }

    // this happens only when table is incorrectons is implemented incorrectly
    // or being physically attacked
    exit_with_status(ptr->id != ROM_REGION_MAX, BL_STATUS_SEC_FATAL, 0, 0);

    status = BL_STATUS_OK;

_exit:
    return status;
}


#else


/* dummy */
bl_status_t bl_sec_get_image_offset(uint32_t addr, uint32_t *offset)
{
    *offset = 0;
    return BL_STATUS_OK;
}


#endif /* MTK_SECURE_BOOT_ENABLE */

