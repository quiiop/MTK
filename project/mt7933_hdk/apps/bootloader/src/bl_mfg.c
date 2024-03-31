/* Copyright Statement:
 *
 * (C) 2022  MediaTek Inc. All rights reserved.
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


#include <hal_gpio.h>


#include "bl_status.h"


#include "bl_mfg.h"
#include "memory_map.h"
#include "bl_image.h"
#include "bl_sec.h"


#if defined(MTK_RELEASE_MODE) && defined(MTK_MFG_VERIFY)
bool bl_mfg_is_mfg_boot( void )
{
#if defined(MTK_MFG_BOOT)
    return true;
#else
    hal_gpio_status_t   r_gpio;
    hal_gpio_data_t     data;

    r_gpio = hal_gpio_get_input( BL_SWITCH_GPIO, &data );

    return data == 0;
#endif
}
#endif


#if defined(MTK_RELEASE_MODE) && defined(MTK_MFG_VERIFY)
bl_status_t bl_mfg_boot( void )
{
    if ( ! bl_mfg_is_mfg_boot() )
        return BL_STATUS_OK;

    return bl_image_boot( ROM_REGION_MFG );
}


bl_status_t bl_image_mfg_check( uint32_t addr )
{
    bl_status_t     status;
    uint32_t        offset = 0;

    status = bl_sec_get_image_offset( addr, &offset );
    if ( status != BL_STATUS_OK )
        return status;

    if ( offset == 0 ) {
        if ( IS_MFG_MAGIC( addr ) )
            return BL_STATUS_OK;
        offset = 0x80;
    }

    if ( IS_MFG_MAGIC( addr + offset ) )
        return BL_STATUS_OK;

    return BL_STATUS_NOT_FOUND;
}
#endif
