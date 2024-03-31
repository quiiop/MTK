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
#include "mt7933.h"
#include "core_cm33.h"
#include "hal_log.h"

#include "hal_dwt_internal.h"

unsigned int DWT_NUMCOMP;

/* reset all comparators' setting */
void hal_dwt_reset(void)
{
    DWT->COMP0 = 0;
    DWT->COMP1 = 0;
    DWT->COMP2 = 0;
    DWT->COMP3 = 0;
    DWT->FUNCTION0 &= ~DWT_FUNCTION_MATCH_Msk;
    DWT->FUNCTION1 &= ~DWT_FUNCTION_MATCH_Msk;
    DWT->FUNCTION2 &= ~DWT_FUNCTION_MATCH_Msk;
    DWT->FUNCTION3 &= ~DWT_FUNCTION_MATCH_Msk;

    g_dwt_ctrl.comp[0] = 0;
    g_dwt_ctrl.comp[1] = 0;
    g_dwt_ctrl.comp[2] = 0;
    g_dwt_ctrl.comp[3] = 0;
    g_dwt_ctrl.func[0] = DWT->FUNCTION0;
    g_dwt_ctrl.func[1] = DWT->FUNCTION1;
    g_dwt_ctrl.func[2] = DWT->FUNCTION2;
    g_dwt_ctrl.func[3] = DWT->FUNCTION3;
}
void hal_dwt_init(void)
{
    /* only enable hardware stack overflow check by the DWT when halting debug is disabled,
           because under halting-mode, the ICE will take over the DWT function.
           So the software stack overflow need to be checked by SW under halting-mode.
           The halting debug status can be checked by the bit(C_DEBUGEND), which is set when debugger is connected.
      */

    if (!(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)) {
        /* DWT reset*/
        hal_dwt_reset();

        /* enable debug monitor mode    */
        if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_MON_EN_Msk)) {
            CoreDebug->DEMCR |= CoreDebug_DEMCR_MON_EN_Msk;
        }

        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

        DWT_NUMCOMP = DWT->CTRL >> DWT_CTRL_NUMCOMP_Pos;

        g_dwt_ctrl.DEMCR = CoreDebug->DEMCR;
        g_dwt_ctrl.DWT_NUMCOMP = DWT_NUMCOMP;

        log_hal_info(" DWT has %d comparators,\r\n ctrl register status 0x%lx,\r\n", DWT_NUMCOMP, DWT->CTRL);
    }
}

void hal_dwt_dump_status(void)
{
    log_hal_info("DHCSR:0x%lx, DEMCR:0x%lx \r\n", CoreDebug->DHCSR, CoreDebug->DEMCR);
    log_hal_info("DWT_CTRL: 0x%lx \r\n", DWT->CTRL);
    log_hal_info("COMP0: %8lx \t  FUNC0: %8lx \r\n", DWT->COMP0, DWT->FUNCTION0);
    log_hal_info("COMP1: %8lx \t  FUNC1: %8lx \r\n", DWT->COMP1, DWT->FUNCTION1);
    log_hal_info("COMP2: %8lx \t  FUNC2: %8lx \r\n", DWT->COMP2, DWT->FUNCTION2);
    log_hal_info("COMP3: %8lx \t  FUNC3: %8lx \r\n", DWT->COMP3, DWT->FUNCTION3);
}

/*
@param index:         comparator N, valid scope [0,DWT_NUMCOMP-1]
@param addr_base: address for data accesses or instruction fetches
@param addr_mask: the size of the ignore mask applied to address range matching
@param func:        which kind of compared accesses will generate watchpoint debug event
@return val:        status, -1: fail; 0:success
!! Note: the addr_base should be 2^(addr_mask) byte alignment, otherwise the behavior is UNPREDICTABLE !!
!! Note: only enable hardware stack overflow check by the DWT when halting debug is disabled, because under halting-mode, the ICE will take over the DWT function.
         The halting debug status can be checked by the bit(C_DEBUGEND), which is set when debugger is connected.
!! Note:
             Comparator 3 is used to check pxCurrentTCB stack
             Comparator 2/1/0 is reserved for future usage
*/
int32_t hal_dwt_request_watchpoint(uint32_t index, uint32_t addr_base, uint32_t addr_mask, DWT_FUNC_TYPE func)
{
    uint32_t offset;

    /* only enable hardware stack overflow check by the DWT when halting debug is disabled,
           because under halting-mode, the ICE will take over the DWT function.
           The SW will do stack overflow under halting-mode.
           The halting debug status can be checked by the bit(C_DEBUGEND), which is set when debugger is connected.
       */
    if (!(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)) {
        /* comparator N */
        if ((index >= DWT_NUMCOMP) || (addr_base & addr_mask)) {
            return -1;
        }

        offset = (0x10 * index) / 4; // pointer size = 4 bytes

        /* set and enable comparator N */
        *((uint32_t *)(&DWT->COMP0) + offset) = addr_base;
        /* DWT_FUNCTION_ACTION_Msk: Generate Debug Event */
        *((uint32_t *)(&DWT->FUNCTION0) + offset) = (*((uint32_t *)(&DWT->FUNCTION0) + offset) & DWT_FUNCTION_MATCH_Msk) | (func | DWT_FUNCTION_ACTION_Msk);

        g_dwt_ctrl.comp[index] = *((uint32_t *)(&DWT->COMP0) + offset);
        g_dwt_ctrl.func[index] = *((uint32_t *)(&DWT->FUNCTION0) + offset);

        return 0;
    } else {
        return -1;
    }
}
#endif /* #ifdef HAL_DWT_MODULE_ENABLED */
