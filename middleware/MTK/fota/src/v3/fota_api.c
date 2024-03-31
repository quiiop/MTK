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


/* standard header */
#include <stddef.h>
#include <string.h>

/* HAL */
#include <hal_flash.h>

/* v3 header */
#include <v3/fota.h>

/* private header */
#include "fota_log.h"
#include "fota_osal.h"


/****************************************************************************
 *
 * CONSTANTS AND MACROS
 *
 ****************************************************************************/


#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


/****************************************************************************
 *
 * PUBLIC FUNCTIONS
 *
 ****************************************************************************/


fota_status_t fota_io_init(
    const fota_flash_t  *flash,
    uint32_t            partition,
    fota_io_state_t     *io)
{
    int                 i;

    I;

    if ( flash == NULL || io == NULL ) {
        O;
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    }

    if ((flash->block_size - 1) & flash->block_size)
    {
        O;
        return FOTA_STATUS_ERROR_BLOCK_ALIGN;
    }

    for (i = 0; i < flash->table_entries; i++)
    {
        const fota_partition_t *entry = &flash->table[i];

        if (partition != entry->id)
            continue;

        if (entry->address & (flash->block_size - 1))
        {
            O;
            return FOTA_STATUS_ERROR_BLOCK_ALIGN;
        }

        io->block_size = flash->block_size;
        io->block_type = HAL_FLASH_BLOCK_4K;
        io->block_mask = flash->block_size - 1;

        io->bus_addr   = entry->address + flash->bus_address;;
        io->phy_addr   = entry->address;
        io->size       = entry->length;
        io->offset     = 0;

        O;
        return FOTA_STATUS_OK;
    }

    O;
    return FOTA_STATUS_ERROR_UNKNOWN_ID;
}


fota_status_t fota_io_seek(
    fota_io_state_t *io,
    int32_t         offset ) // todo int32_t
{
    I;

    if ( io == NULL ) {
        O;
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    }

    if ((uint32_t)abs(offset) > io->size)
    {
        O;
        return FOTA_STATUS_ERROR_OUT_OF_RANGE;
    }

    if (offset < 0)
        io->offset = io->size + offset;
    else
        io->offset = offset;

    O;
    return FOTA_STATUS_OK;
}


fota_status_t fota_io_read(
    fota_io_state_t *io,
    void            *buffer,
    uint32_t        length )
{
    I;

    if ( io == NULL || buffer == NULL ) {
        O;
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    }

    // boundary check
    if (io->offset + length > io->phy_addr + io->size)
    {
        O;
        return FOTA_STATUS_ERROR_OUT_OF_RANGE;
    }

    if (hal_flash_read(io->phy_addr + io->offset, buffer, length) < 0) {
        O;
        return FOTA_STATUS_ERROR_FLASH_OP;
    }
    io->offset += length;

    O;
    return FOTA_STATUS_OK;
}


fota_status_t fota_io_write(
    fota_io_state_t *io,
    const void      *buffer,
    uint32_t        length )
{
    uint32_t        addr;
    uint32_t        size;
    uint32_t        block_addr;
    uint32_t        block_next;

    I;

    if ( io == NULL ) {
        O;
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    }

    // boundary check
    if (io->offset + length > io->phy_addr + io->size)
    {
        O;
        return FOTA_STATUS_ERROR_OUT_OF_RANGE;
    }

    // presumptions:
    // 1. erase in unit of block
    // 2. write to non-block boundary as if already erased
    // 3. block size is power of 2

    while (length > 0)
    {
        addr       = io->phy_addr + io->offset;
        block_addr = addr & ~(io->block_mask);
        block_next = block_addr + io->block_size;
        size       = min(length, block_next - addr);

        // move offset forward
        io->offset += size;

        // erase at block boundary
        if (addr == block_addr)
        {
            E("#");
            V("erase addr %x %x %u", addr, (unsigned int)buffer, size);
            if (hal_flash_erase( addr, io->block_type ) != HAL_FLASH_STATUS_OK)
            {
                O;
                return FOTA_STATUS_ERROR_FLASH_OP;
            }
        }

        V("write addr %x %x %u", addr, (unsigned int)buffer, size);
        if (hal_flash_write(addr, buffer, size) != HAL_FLASH_STATUS_OK)
        {
            O;
            return FOTA_STATUS_ERROR_FLASH_OP;
        }
        buffer += size;
        length -= size;
    }

    O;
    return FOTA_STATUS_OK;
}


fota_status_t fota_read_info(
    const fota_flash_t      *flash,
    fota_upgrade_info_t     *info,
    const uint32_t          partition )
{
    fota_io_state_t         io;
    fota_status_t           status;

    I;

    if ( ! flash || ! info ) {
        O;
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    }

    status = fota_io_init( flash, partition, &io );
    if (status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    status = fota_io_seek( &io, -sizeof( *info ) );
    if (status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    status = fota_io_read( &io, info, sizeof( *info ) );
    if (status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    if (info->magic != FOTA_UPGRADE_STATUS_MAGIC) {
        O;
        return FOTA_STATUS_ERROR_CONTROL_BLOCK_CORRUPTION;
    }

    O;
    return FOTA_STATUS_OK;
}


fota_status_t fota_write_info(
    const fota_flash_t      *flash,
    fota_upgrade_info_t     *info,
    uint32_t                partition )
{ 
    fota_io_state_t         io;
    fota_status_t           status;

    I;

    if ( ! flash || ! info ) {
        O;
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    }

    status = fota_io_init( flash, partition, &io );
    if (status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    status = fota_io_seek( &io, -sizeof( *info ) );
    if (status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    status = fota_io_write( &io, info, sizeof( *info ) );
    if (status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    O;
    return FOTA_STATUS_OK;
}


fota_status_t fota_invalidate_info(
    const fota_flash_t      *flash,
    const uint32_t          partition )
{
    fota_upgrade_info_t     *info;
    fota_status_t           status;

    I;

    info = fota_info_malloc();
    if ( info == NULL ) {
        O;
        return FOTA_STATUS_ERROR_OUT_OF_MEMORY;
    }

    status = fota_read_info( flash, info, partition );
    if ( status == FOTA_STATUS_OK &&
         info->magic == FOTA_UPGRADE_STATUS_MAGIC &&
         info->state == FOTA_UPGRADE_STATUS_INVALID ) {
        fota_info_free( info );
        O;
        return FOTA_STATUS_OK;
    }

    memset( info, 0, sizeof( *info ) );
    info->magic = FOTA_UPGRADE_STATUS_MAGIC;
    info->state = FOTA_UPGRADE_STATUS_INVALID;
    
    status = fota_write_info( flash, info, partition );
    fota_info_free( info );
    if ( status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    O;
    return FOTA_STATUS_OK;
}


fota_status_t fota_trigger_upgrade(
    const fota_flash_t      *flash,
    const uint32_t          partition )
{
    fota_upgrade_info_t     *info;
    fota_status_t           status;

    I;

    info = fota_info_malloc();
    if ( info == NULL ) {
        O;
        return FOTA_STATUS_ERROR_OUT_OF_MEMORY;
    }

    info->magic = FOTA_UPGRADE_STATUS_MAGIC;
    info->state = FOTA_UPGRADE_STATUS_READY;

    status = fota_write_info( flash, info, partition );
    fota_info_free( info );
    if ( status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    O;
    return status;
}


fota_status_t fota_defuse_upgrade(
    const fota_flash_t      *flash,
    const uint32_t          partition )
{
    fota_upgrade_info_t     *info;
    fota_status_t           status;

    I;

    info = fota_info_malloc();
    if ( info == NULL ) {
        O;
        return FOTA_STATUS_ERROR_OUT_OF_MEMORY;
    }

    info->magic = FOTA_UPGRADE_STATUS_MAGIC;
    info->state = FOTA_UPGRADE_STATUS_NONE;

    status = fota_write_info( flash, info, partition );
    fota_info_free( info );
    if ( status != FOTA_STATUS_OK ) {
        O;
        return status;
    }

    O;
    return FOTA_STATUS_OK;
}

