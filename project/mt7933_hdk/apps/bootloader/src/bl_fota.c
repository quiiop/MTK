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

/**
 * @file This file contains APIs for supporting:
 *
 *	1. download with xmodem via UART (MTK_FOTA_XMODEM_ENABLE)
 *  2. dual bootable images swapping (MTK_FOTA_DUAL_IMAGE_ENABLE)
 */


/***************************************************************************
 * feature control
 ***************************************************************************/


//#define MTK_FOTA_XMODEM_ENABLE	/* enable this to enable UART download */


/****************************************************************************
 *
 * HEADER FILES
 *
 ****************************************************************************/


/* HAL */
#include <hal_wdt.h>

/* v3 header */
#ifdef MTK_FOTA_V3_ENABLE
#include <v3/fota.h>
#include <v3/fota_migrate.h>
#endif

#ifdef MTK_FOTA_DUAL_IMAGE_ENABLE
#include "flash_map_dual.h"
#include "fota_dual_image.h"
#endif

// per project header
#ifdef MTK_FOTA_V3_ENABLE
#include "fota_flash_config.h"
#endif
#include "hw_uart.h"
#include "bl_mperf.h"


/***************************************************************************
 * header file inclusion: self API declaration
 ***************************************************************************/


#include "bl_fota.h"


/***************************************************************************
 * macros
 ***************************************************************************/


#ifdef MTK_FOTA_XMODEM_ENABLE
#define BL_FOTA_BLOCK_SIZE (256)
#endif


/***************************************************************************
 * data types
 ***************************************************************************/


#ifdef MTK_FOTA_XMODEM_ENABLE
typedef struct bl_fota_job_s
{
    uint32_t partition;
    uint32_t size;
    uint32_t pos;
} bl_fota_job_t;
#endif


/***************************************************************************
 * API - download with xmodem via UART
 ***************************************************************************/


#ifdef MTK_FOTA_XMODEM_ENABLE

#if MTK_FOTA_XMODEM_DEBUG
static int callbacks;
static int rx_bytes;
#endif

static void bl_fota_rx_callback(void *ptr, uint8_t *buffer, int len)
{
    bl_fota_job_t  *job = ptr;

    if (!ptr)
        return;

    if (len <= 0) {
        return;
    }

    job->pos += len;

    if (job->pos > job->size) {
        return;
    }

#if MTK_FOTA_XMODEM_DEBUG
    callbacks += 1;
    rx_bytes  += len;
#endif

    fota_write(job->partition, buffer, len);
}


void bl_fota_xmodem_download(uint32_t partition, uint32_t len)
{
    uint8_t     buffer[ FOTU_BLOCK_SIZE ];
    bl_fota_job_t  fotu_job = { .partition = partition, .size = len, .pos = 0 };

    /* prepare target */
    fota_init(&fota_flash_default_config);
    fota_seek(partition, 0);

    xmodem_block_rx(&fotu_job, buffer, len, bl_fota_rx_callback);

#if MTK_FOTA_XMODEM_DEBUG
    hw_uart_printf("callback %d\n", callbacks);
    hw_uart_printf("rx_bytes %d\n", rx_bytes);
    rx_bytes = callbacks = 0;
#endif

    return;
}
#endif /* MTK_FOTA_XMODEM_ENABLE */


/***************************************************************************
 * API - dual bootable images swapping
 ***************************************************************************/


bl_status_t bl_fota_query_active_image(rom_region_id_t *image_id)
{
    bl_status_t status = BL_STATUS_OK;

#ifdef MTK_FOTA_DUAL_IMAGE_ENABLE
    fota_dual_image_init();
    if (fota_query_active_image(image_id) != FOTA_STATUS_OK)
        status = BL_STATUS_NOT_FOUND;
#else
    *image_id = ROM_REGION_RTOS; // TODO: update when mcuboot/tfm enabled.
#endif

	return status;
}


#ifdef MTK_FOTA_V3_ENABLE

#define UPGRADE_BLOCK_SIZE  (4096)

void bl_fota_boot_status_sync(void)
{
    fota_upgrade_info_t info;
    fota_status_t       status;

    status = fota_read_info( &g_fota_flash_config, &info,
                             ROM_REGION_FOTA );
    if ( status != FOTA_STATUS_OK ) {
        return;
    }

    if ( info.state == FOTA_UPGRADE_STATUS_RUNNING ) {
        hw_uart_printf( "upgrade aborted\n" );
        fota_invalidate_info( &g_fota_flash_config, ROM_REGION_FOTA );
        return;
    }

    if ( info.state == FOTA_UPGRADE_STATUS_READY ) {
        hw_uart_printf( "upgrade\n" );

        status = fota_migrate( &g_fota_flash_config, ROM_REGION_FOTA );
        if ( status != FOTA_STATUS_OK ) {
            hw_uart_printf( "upgrade failed (%x)\n", status );
        } else {
            hw_uart_printf( "upgrade ok\n" );
            fota_invalidate_info( &g_fota_flash_config, ROM_REGION_FOTA );
        }

    }

    /* feed watchdog regularly. */
    hal_wdt_feed(HAL_WDT_FEED_MAGIC);

    bl_mperf_profile_tag(&g_mperf_profile, "fd");

    return;
}
#endif /* MTK_FOTA_V3_ENABLE */
