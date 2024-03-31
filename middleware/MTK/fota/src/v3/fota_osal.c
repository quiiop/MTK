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


#include <stdint.h>
#include <stddef.h>
#include <string.h>


#ifdef MTK_FOTA_V3_FREERTOS_ENABLE
#include <FreeRTOS.h>
#endif


#ifdef MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
#include <assert.h>
#endif


#include <v3/fota.h>


/****************************************************************************
 *
 * CONSTANTS AND MACROS
 *
 ****************************************************************************/


#ifdef MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
#define CHECK(s,v) assert( (s) == sizeof(v))
#endif


/****************************************************************************
 *
 * GLOBAL VARIABLES
 *
 ****************************************************************************/


#ifdef MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
static int      _g_lzma_alloc_count;
static uint8_t  _g_lzma_buf_0[15980]; /* 15980 */
static uint8_t  _g_lzma_buf_1[16384]; /* 16384 */
static uint8_t  _g_lzma_buf_2[ 4096]; /*  4096 */
static uint8_t  _g_lzma_buf_3[ 4096]; /*  4096 */
#endif


/****************************************************************************
 *
 * PUBLIC FUNCTIONS
 *
 ****************************************************************************/


#ifdef MTK_FOTA_V3_FREERTOS_ENABLE
void *fota_malloc( size_t size )
{
    if ( size > 0 )
    {
        return pvPortMalloc( size );
    }
    return NULL;
}
#endif


#ifdef MTK_FOTA_V3_FREERTOS_ENABLE
void fota_free( void *ptr )
{
    vPortFree( ptr );
}
#endif


#ifdef MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
void *fota_lzma_malloc( void *p, size_t size )
{
    _g_lzma_alloc_count++;
    switch ( _g_lzma_alloc_count )
    {
    case 1: CHECK(size, _g_lzma_buf_0); return &_g_lzma_buf_0[0]; break;
    case 2: CHECK(size, _g_lzma_buf_1); return &_g_lzma_buf_1[0]; break;
    case 3: CHECK(size, _g_lzma_buf_2); return &_g_lzma_buf_2[0]; break;
    case 4: CHECK(size, _g_lzma_buf_3); return &_g_lzma_buf_3[0]; break;
    default: _g_lzma_alloc_count--; return NULL; break;
    }
}
#endif


#ifdef MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
void fota_lzma_free( void *p, void *ptr )
{
    if ( ptr && _g_lzma_alloc_count > 0 )
        _g_lzma_alloc_count--;
}
#endif


void *fota_info_malloc ( void )
{
#ifdef MTK_FOTA_V3_FREERTOS_ENABLE
    fota_upgrade_info_t *info = fota_malloc( sizeof( *info ) );
    if ( info ) {
        memset( info, 0, sizeof( *info ) );
    }
    return info;
#else
    static fota_upgrade_info_t info;
    memset( &info, 0, sizeof( info ) );
    return &info;
#endif
}


void fota_info_free( void *info )
{
#ifdef MTK_FOTA_V3_FREERTOS_ENABLE
    fota_free( info );
#else
    (void)info;
#endif
}

