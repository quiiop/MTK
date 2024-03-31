/* Copyright Statement:
 *
 * (C) 2021-2021  MediaTek Inc. All rights reserved.
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


// C library header
#include <unistd.h>
#include <string.h>

// hal header
#ifndef FOTAV3_UNIT_TEST
#include <hal_sha.h>
#endif

// fotav3 headers
#include <v3/fota_migrate.h>
#include <v3/fota_format.h>

// fotav3 internal headers
#include "fota_osal.h"
#include "fota_log.h"

// library
#include "cli.h"
#ifdef MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
#include <lzma_decode_interface.h>
#endif

/* project header */
#include "memory_map.h"


/****************************************************************************
 *
 * CONSTANTS AND MACROS
 *
 ****************************************************************************/


#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define BLOCK_MASK          ( ~( block_size - 1 ) )

#define IS_ALIGNED( addr )  ( ( (uint32_t)addr & BLOCK_MASK ) == (uint32_t)addr )

#define OFFSET( addr )      ( (uint32_t)addr & ( block_size - 1 ) )

#define ALIGN( addr )       ( (uint32_t)addr & BLOCK_MASK )


/****************************************************************************
 *
 * PRIVATE FUNCTIONS
 *
 ****************************************************************************/


static bool _iot_fota_header_verify(const IOT_FOTA_HEADER *hdr)
{
    if ( hdr->m_bin_num > BIN_NUMBER )
        return false;

    return FOTA_HEADER_GET_MAGIC( hdr->m_magic_ver ) == FOTA_HEADER_MAGIC;
}


static bool _sha_verify(
    const void          *ptr,
    ssize_t             size,
    const uint8_t       digest[ SHA1_HASH_CODE_LEN ] )
{
    hal_sha1_context_t  ctx;
    uint8_t             answer[ HAL_SHA1_DIGEST_SIZE ];
    void                *non_cache_ptr;

    non_cache_ptr = (void *)HAL_CACHE_VIRTUAL_TO_PHYSICAL( (uint32_t)ptr );

    if ( hal_sha1_init  ( &ctx )                      != HAL_SHA_STATUS_OK ||
         hal_sha1_append( &ctx, non_cache_ptr, size ) != HAL_SHA_STATUS_OK ||
         hal_sha1_end   ( &ctx, answer )              != HAL_SHA_STATUS_OK )
        return false;

    return ! memcmp( digest, answer, SHA1_HASH_CODE_LEN );
}


#ifdef MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
static fota_status_t _fota_migrate_lzma(
    void        *dst,
    size_t      dst_len,
    const void  *src,
    size_t      src_len,
    size_t      block_size )
{
    lzma_alloc_t    mem_alloc = { fota_lzma_malloc, fota_lzma_free };
    int             ret;

    V("decompress %x bytes from %x to %x", src_len, src, dst);

    if ( block_size != 4096 )
        return FOTA_STATUS_ERROR_FLASH_OP;

    ret = lzma_decode2flash( dst, dst_len, src, &mem_alloc );
    if ( ret == LZMA_OK )
        V( "copy completed" );
    else
        V( "decompress failed (%d)", ret );

    return FOTA_STATUS_OK;
}
#endif /* MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE */


#ifdef MTK_FOTA_V3_PAYLOAD_PLAIN_ENABLE
static fota_status_t _fota_migrate_plain(
    void        *dst,
    const void  *src,
    size_t      n,
    size_t      block_size )
{
    uint32_t    d = (uint32_t)dst;
    uint32_t    s = (uint32_t)src;
    uint32_t    len;
    uint8_t     buf[ 4096 ] = {0};
    hal_flash_status_t  r;

    V("copy %x bytes from %x to %x", n, s, d);

    if ( block_size != 4096 )
        return FOTA_STATUS_ERROR_FLASH_OP;

    while ( n > 0 )
    {
        len = min( block_size - OFFSET( d ), n );

        r = hal_flash_read( s, &buf[ OFFSET( d ) ], len );
        if (r != HAL_FLASH_STATUS_OK)
            return FOTA_STATUS_ERROR_FLASH_OP;

        V("erase %x", ALIGN( d ));

        r = hal_flash_erase( ALIGN( d ), HAL_FLASH_BLOCK_4K );
        if (r != HAL_FLASH_STATUS_OK)
            return FOTA_STATUS_ERROR_FLASH_OP;

        V("write %x bytes to %x", len, d);

        r = hal_flash_write( ALIGN( d ), &buf[0], len + OFFSET( d ) );
        if (r != HAL_FLASH_STATUS_OK)
            return FOTA_STATUS_ERROR_FLASH_OP;

        n -= len;
        d += len;
        s += len;
    }

    V("copy completed");

    return FOTA_STATUS_OK;
}
#endif /* MTK_FOTA_V3_PAYLOAD_PLAIN_ENABLE */


/****************************************************************************
 *
 * PUBLIC FUNCTIONS
 *
 ****************************************************************************/


fota_status_t fota_migrate_mem(
    const fota_flash_t      *flash,
    const void              *packet_top,
    size_t                  packet_size )
{
    // TODO: lzma uncompress buffer

    fota_status_t           status;
    const uint8_t           *p  = packet_top;
    size_t                  len = packet_size;
    if ( ! p )
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;

#define MOVE_PTR( p, len, size ) do { p += size; len -= size; } while (0);

    // header

    const IOT_FOTA_HEADER *hdr  = ( IOT_FOTA_HEADER * )p;
    const IOT_BIN_INFO    *binfo;

    if ( len < sizeof( *hdr ) || ! _iot_fota_header_verify( hdr ) )
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    else
    {
        MOVE_PTR( p, len, sizeof( *hdr ) );
        binfo = ( IOT_BIN_INFO * )p;
    }

    // sha1 ( header + bin info ) vs. digest

    uint32_t        binfos         = hdr->m_bin_num;
    uint32_t        binfo_len      = binfos * sizeof( IOT_BIN_INFO );
    uint32_t        message_len    = sizeof( *hdr ) + binfo_len;
    const uint8_t   *packet_digest = ( ( uint8_t * )hdr ) + message_len;

    if ( len < message_len + SHA1_HASH_CODE_LEN ||
         ! _sha_verify( hdr, message_len, packet_digest ) )
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    else
        MOVE_PTR( p, len, binfo_len + SHA1_HASH_CODE_LEN );

    // bin info

    const uint8_t *base = packet_top;
    uint32_t i;
    for ( i = 0; i < binfos; i++, binfo++ )
    {
        void        *d, *s;
        size_t      l;
        uint32_t    bin_start_addr;

        if ( binfo->m_bin_offset == 0 )
            continue;

        if ( binfo->m_bin_offset + binfo->m_bin_length + FOTA_SIGNATURE_SIZE
                > packet_size )
            return FOTA_STATUS_ERROR_INVALD_PARAMETER;

        const uint8_t *bin = base + binfo->m_bin_offset;

        bin_start_addr = binfo->m_bin_start_addr;

        // map memory bus address to flash physical address
        if ( ( bin_start_addr & flash->bus_address ) == flash->bus_address )
            bin_start_addr -= flash->bus_address;

        W("* upgrade %x len %x to %x/%x%s",
                            bin, binfo->m_bin_length, bin_start_addr,
                            bin_start_addr + flash->bus_address,
                            binfo->m_is_compressed ? " (decompress)" : "");

        V("verify addr %x len %x: answer at %x",
          bin, binfo->m_bin_length, bin + binfo->m_bin_length);

        if ( ! _sha_verify( bin, binfo->m_bin_length,
                            bin + binfo->m_bin_length ) )
            return FOTA_STATUS_ERROR_INVALD_PARAMETER;

        // future work: ECDSA verify

        // migrate bin
        d = (void *)bin_start_addr;
        s = (void *)packet_top + binfo->m_bin_offset - flash->bus_address;
        l  = binfo->m_bin_length;

        switch ( binfo->m_is_compressed )
        {
#ifdef MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
            case 1:
                status = _fota_migrate_lzma( d, binfo->m_partition_length,
                                             s, l, flash->block_size );
                break;
#endif
#ifdef MTK_FOTA_V3_PAYLOAD_PLAIN_ENABLE
            case 0:
                status = _fota_migrate_plain( d, s, l, flash->block_size );
                break;
#endif
            default:
                status = FOTA_STATUS_ERROR_INVALD_PARAMETER;
                break;
        }

        if ( status != FOTA_STATUS_OK )
            return status;
    }

    return FOTA_STATUS_OK;
}


fota_status_t fota_migrate(
    const fota_flash_t      *flash,
    const uint32_t          partition )
{
    fota_io_state_t         io;
    fota_upgrade_info_t     info;
    fota_status_t           status;

    I;

    status = fota_io_init( flash, partition, &io );
    if (status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    status = fota_io_seek( &io, -sizeof( info ) );
    if (status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    status = fota_io_read( &io, &info, sizeof( info ) );
    if (status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    if (info.state == FOTA_UPGRADE_STATUS_READY)
    {
        status = fota_migrate_mem( flash, (void *)io.bus_addr, io.size );
        O;
        return status;
    }

    O;
    return FOTA_STATUS_OK;
}

