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


#include <stdlib.h>
#include <string.h>


#include "bl_region.h"

#if defined(MTK_RELEASE_MODE) && defined(MTK_MFG_VERIFY)
#include "bl_mfg.h"
#endif

/***************************************************************************
 * PARTITION SETTINGS
 ***************************************************************************/


#define BOOTABLE                true
#define NONBOOTABLE             false


#define BL_SEC_TYPE             true
#define BL_RUN_TYPE             false


#ifdef MTK_TFM_ENABLE
    #define TFM_SEC_TYPE        true
    #ifdef MTK_TFM_IN_RAM
        #define TFM_RUN_TYPE    true
    #else
        #define TFM_RUN_TYPE    false
    #endif
#endif


#ifdef MTK_RTOS_SIGN_ENABLE
    #define OS_SEC_TYPE         true
#else
    #define OS_SEC_TYPE         false
#endif
#ifdef MTK_RTOS_IN_RAM
    #define OS_RUN_TYPE         true
#else
    #define OS_RUN_TYPE         false
#endif


#ifdef MTK_BL_WIFI_FW_ENABLE
    #ifdef MTK_WIFI_SIGN_ENABLE
        #define WIFI_SEC_TYPE   true
    #else
        #define WIFI_SEC_TYPE   false
    #endif
    #ifdef MTK_BL_WIFI_IN_RAM
        #define WIFI_RUN_TYPE   true
    #else
        #define WIFI_RUN_TYPE   false
    #endif
#endif


#ifdef MTK_BL_BT_FW_ENABLE
    #define BT_SEC_TYPE         true
    #ifdef MTK_BL_BT_IN_RAM
        #define BT_RUN_TYPE     true
    #else
        #define BT_RUN_TYPE     false
    #endif
#endif


#ifdef MTK_BL_ADSP_FW_ENABLE
    #ifdef MTK_ADSP_SIGN_ENABLE
        #define ADSP_SEC_TYPE   true
    #else
        #define ADSP_SEC_TYPE   false
    #endif
    #ifdef MTK_BL_ADSP_IN_RAM
        #define ADSP_RUN_TYPE   true
    #else
        #define ADSP_RUN_TYPE   false
    #endif
#endif


#ifdef MTK_BL_FOTA_ENABLE
    #ifdef MTK_FOTA_SIGN_ENABLE
        #define FOTA_SEC_TYPE   true
    #else
        #define FOTA_SEC_TYPE   false
    #endif
    #ifdef MTK_BL_FOTA_IN_RAM
        #define FOTA_RUN_TYPE   true
    #else
        #define FOTA_RUN_TYPE   false
    #endif
#endif


/***************************************************************************
 * CONSTANT DATA
 ***************************************************************************/


static const bl_region_t _g_bl_regions[] = {
    { ROM_REGION_BL,      "bl",   LOADER_BASE,      /* flash addr   */
                                  LOADER_LENGTH,    /* length       */
                                  BOOTABLE,         /* bootable     */
                                  BL_SEC_TYPE,      /* secure mode? */
                                  BL_RUN_TYPE },    /* run where?   */
#ifdef MTK_TFM_ENABLE
    { ROM_REGION_TFM_A,   "tfm",  XIP_TFM_ADDR,     /* flash addr   */
                                  TFM_LENGTH,       /* length       */
                                  BOOTABLE,         /* bootable     */
                                  TFM_SEC_TYPE,     /* secure mode? */
                                  TFM_RUN_TYPE },   /* run where?   */
#endif
    { ROM_REGION_RTOS,    "rtos", XIP_RTOS_START,   /* flash addr   */
                                  RTOS_LENGTH,      /* length       */
                                  BOOTABLE,         /* bootable     */
                                  OS_SEC_TYPE,      /* secure mode? */
                                  OS_RUN_TYPE },    /* run where?   */
#ifdef MTK_BL_WIFI_FW_ENABLE
    { ROM_REGION_WIFI_FW, "wifi", XIP_WIFI_START,   /* flash addr   */
                                  WIFI_LENGTH,      /* length       */
                                  NONBOOTABLE,      /* not bootable */
                                  WIFI_SEC_TYPE,    /* secure mode? */
                                  WIFI_RUN_TYPE },   /* run where?   */
#endif
#ifdef MTK_BL_BT_FW_ENABLE
    { ROM_REGION_BT_FW,   "bt",   XIP_BT_START,     /* flash addr   */
                                  BT_LENGTH,        /* length       */
                                  NONBOOTABLE,      /* not bootable */
                                  BT_SEC_TYPE,      /* secure mode? */
                                  BT_RUN_TYPE },    /* run where?   */
#endif
#ifdef MTK_BL_ADSP_FW_ENABLE
    { ROM_REGION_DSP_FW,  "dsp",  XIP_DSP_START,    /* flash addr   */
                                  DSP_LENGTH,       /* length       */
                                  NONBOOTABLE,      /* not bootable */
                                  ADSP_SEC_TYPE,    /* secure mode? */
                                  ADSP_RUN_TYPE },
#endif
#ifdef MTK_BL_FOTA_ENABLE
    { ROM_REGION_FOTA,    "ota",  FOTA_BASE,        /* flash addr   */
                                  FOTA_LENGTH,      /* length       */
                                  NONBOOTABLE,      /* not bootable */
                                  FOTA_SEC_TYPE,    /* secure mode? */
                                  FOTA_RUN_TYPE },  /* run where?   */
#endif
#if defined(MTK_RELEASE_MODE) && defined(MTK_MFG_VERIFY)
    { ROM_REGION_MFG,    "mfg",   XIP_MFG_START,    /* flash addr   */
                                  MFG_LENGTH,       /* length       */
                                  BOOTABLE,         /* bootable     */
                                  MFG_SEC_TYPE,     /* secure mode? */
                                  MFG_RUN_TYPE },   /* run where?   */
#endif
    { ROM_REGION_MAX, NULL, 0, 0, false,  false,  false }
};


/***************************************************************************
 * PUBLIC FUNCTIONS
 ***************************************************************************/


const bl_region_t * bl_region_get_table(void)
{
    return &_g_bl_regions[0];
}


bl_status_t bl_region_get(rom_region_id_t id, bl_region_t *region)
{
    bl_status_t         status = BL_STATUS_OK;
    uint32_t            i = 0;

    exit_with_status( region == NULL, BL_STATUS_INVALID_PARAM,
                      id, (int)region);

    while ( _g_bl_regions[i].id < ROM_REGION_MAX )
    {
        if ( _g_bl_regions[i].id == id )
        {
            memcpy( region, &_g_bl_regions[i], sizeof( *region ) );
            break;
        }
        i++;
    }

    exit_with_status(region->id != id, BL_STATUS_NOT_FOUND, id, i);

    /*
     * If offset 0x80 appears at the beging of image, adjust to boundary.
     *
     * For backward compatibility, this takes care of the case when addresses
     * and lengths in region table contain precalculated offset.
     */

    uint16_t hdr_offset  = region->addr & 0xFF;

    region->addr = region->addr - hdr_offset;
    region->size = region->size + hdr_offset;

_exit:

    return status;
}

