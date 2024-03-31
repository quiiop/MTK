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

#include "hal_trng.h"
#include "hal_log.h"

#ifdef HAL_TRNG_MODULE_ENABLED

#include "hal_trng_internal.h"

static volatile uint8_t trng_init_status = 0;

hal_trng_status_t hal_trng_init(void)
{
    hal_trng_status_t busy_status;
    TRNG_CHECK_AND_SET_BUSY(busy_status);
    if (HAL_TRNG_STATUS_ERROR == busy_status) {
        return HAL_TRNG_STATUS_ERROR;
    }

    trng_init();
    return HAL_TRNG_STATUS_OK;
}

hal_trng_status_t hal_trng_deinit(void)
{
    trng_deinit();
    TRNG_SET_IDLE();
    return HAL_TRNG_STATUS_OK;
}

hal_trng_status_t hal_trng_get_generated_random_number(uint32_t *random_number)
{
    uint32_t generate_data = 0;

    if (NULL == random_number) {
        log_hal_error("[TRNG] Error: input buffer is NULL\n\n");
        return HAL_TRNG_STATUS_INVALID_PARAMETER;
    }

    generate_data = trng_get_random_data();
    if (generate_data == 0) {
        return  HAL_TRNG_STATUS_ERROR;
    } else {
        *random_number = generate_data;
    }

    return HAL_TRNG_STATUS_OK;
}

bool hal_trng_is_initialized(void)
{
    return (trng_init_status == TRNG_INIT);
}

#endif /* #ifdef HAL_TRNG_MODULE_ENABLED */

