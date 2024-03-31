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


/* fota header */
#include <v3/fota.h>


/* per project header */
#include "memory_map.h"
#include "fota_flash_config.h"


/****************************************************************************
 *
 * CONSTANTS AND MACROS
 *
 ****************************************************************************/


#define FLASH_BLOCK_SIZE        (4096)

/* convenient macro */
#define AREA( a, b, c ) { .id = a, .address = b, .length = c }


/****************************************************************************
 *
 * GLOBAL VARIABLES
 *
 ****************************************************************************/


static const fota_partition_t _g_flash_table[] = {
    AREA(ROM_REGION_BL,           LOADER_BASE,        LOADER_LENGTH),
    AREA(ROM_REGION_TFM_A,        TFM_BASE,           TFM_LENGTH),
    AREA(ROM_REGION_RTOS,         RTOS_BASE,          RTOS_LENGTH),
    AREA(ROM_REGION_FOTA,         FOTA_BASE,          FOTA_LENGTH),
    AREA(ROM_REGION_WIFI_FW,      WIFI_BASE,          WIFI_LENGTH),
    AREA(ROM_REGION_WIFI_PATCH,   WIFI_PATCH_BASE,    WIFI_PATCH_LENGTH),
    AREA(ROM_REGION_BT_FW,        BT_BASE,            BT_LENGTH),
};
#define FLASH_ENTRIES (sizeof(_g_flash_table) / sizeof(fota_partition_t))


/**
 * required if FOTAv3 is used
 */
const fota_flash_t g_fota_flash_config = {
    .bus_address    = 0x18000000,
    .table          = &_g_flash_table[0],
    .table_entries  = FLASH_ENTRIES,
    .block_size     = FLASH_BLOCK_SIZE
};

