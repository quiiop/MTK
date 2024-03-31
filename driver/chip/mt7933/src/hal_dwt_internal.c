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

#include "hal_dwt.h"

#ifdef HAL_DWT_MODULE_ENABLED
#include "hal_dwt_internal.h"
#include "memory_attribute.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */


ATTR_RWDATA_IN_TCM DWT_CTRL_Type g_dwt_ctrl;
ATTR_TEXT_IN_TCM void dwt_status_save(void)
{

}

ATTR_TEXT_IN_TCM void dwt_status_restore(void)
{
    /* only enable hardware stack overflow check by the DWT when halting debug is disabled,
             because under halting-mode, the ICE will take over the DWT function.
             So the software stack overflow need to be checked by SW under halting-mode.
             The halting debug status can be checked by the bit(C_DEBUGEND), which is set when debugger is connected.
        */

    if (!(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)) {
        DWT_NUMCOMP = g_dwt_ctrl.DWT_NUMCOMP;
        CoreDebug->DEMCR = g_dwt_ctrl.DEMCR;

        DWT->COMP0 = g_dwt_ctrl.comp[0];
        DWT->COMP1 = g_dwt_ctrl.comp[1];
        DWT->COMP2 = g_dwt_ctrl.comp[2];
        DWT->COMP3 = g_dwt_ctrl.comp[3];
        DWT->FUNCTION0 = g_dwt_ctrl.func[0];
        DWT->FUNCTION1 = g_dwt_ctrl.func[1];
        DWT->FUNCTION2 = g_dwt_ctrl.func[2];
        DWT->FUNCTION3 = g_dwt_ctrl.func[3];

        CoreDebug->DEMCR = g_dwt_ctrl.DEMCR;
    }
}

#else /* #ifdef HAL_DWT_MODULE_ENABLED */

#include "memory_attribute.h"
/* Dummy function for sleep manager call in libhal_core.a */
ATTR_TEXT_IN_TCM void dwt_status_save(void)
{

}

ATTR_TEXT_IN_TCM void dwt_status_restore(void)
{

}

#endif /* #ifdef HAL_DWT_MODULE_ENABLED */


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


