/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
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

#include "hal_mpu.h"

#ifdef HAL_MPU_MODULE_ENABLED

#include "hal_mpu_internal.h"
#include "memory_attribute.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

ATTR_RWDATA_IN_TCM volatile uint8_t g_mpu_status = MPU_IDLE;
ATTR_RWDATA_IN_TCM MPU_CTRL_Type g_mpu_ctrl;
ATTR_RWDATA_IN_TCM MPU_REGION_EN_Type g_mpu_region_en;
ATTR_RWDATA_IN_TCM ARM_MPU_Region_t g_mpu_entry[HAL_CM33_MPU_REGION_MAX];

ATTR_TEXT_IN_TCM void mpu_status_save(void)
{

}

/* restores only regions that are enabled before entering into deepsleep */
ATTR_TEXT_IN_TCM void mpu_status_restore(void)
{
    hal_mpu_region_t region;

    if (g_mpu_status == MPU_IDLE) {
        return;
    }

    for (region = HAL_MPU_REGION_0; region < HAL_CM33_MPU_REGION_MAX; region ++) {
        ARM_MPU_SetRegion(region, g_mpu_entry[region].RBAR, g_mpu_entry[region].RLAR);

        //MPU->RBAR = g_mpu_entry[region].RBAR;
        //MPU->RLAR = g_mpu_entry[region].RLAR;
    }
    MPU->CTRL = g_mpu_ctrl.w;
}


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifdef HAL_MPU_MODULE_ENABLED */

