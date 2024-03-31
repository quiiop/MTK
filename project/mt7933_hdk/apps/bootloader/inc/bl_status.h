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


#ifndef __BL_STATUS_H__
#define __BL_STATUS_H__


#include <stdint.h>


typedef enum {
    BL_STATUS_OK                    = 0,
    BL_STATUS_NOT_FOUND             = 1,
    BL_STATUS_INVALID_PARAM         = 2,
    BL_STATUS_NO_SPACE              = 3,
    BL_STATUS_SEC_CLK_FAIL          = 4,
    BL_STATUS_ECC_CLK_FAIL          = 5,
    BL_STATUS_SEC_HW_FAIL           = 6,
    BL_STATUS_MEM_HW_FAIL           = 7,
    BL_STATUS_FLASH_HW_FAIL         = 8,
    BL_STATUS_WDOG_HW_FAIL          = 9,
    BL_STATUS_WDOG_EN_FAIL          = 10,
    BL_STATUS_WDOG_DIS_FAIL         = 11,
    BL_STATUS_BOOT_VECT_FAIL        = 12,

    BL_STATUS_IMG_VERIFY_FAIL       = 31,
    BL_STATUS_IMG_ERASE_FAIL        = 32,

    BL_STATUS_SEC_FATAL             = 61, // physical attack or code defect
    BL_STATUS_SEC_NO_HDR            = 62, // image has no imgtool header
    BL_STATUS_SEC_INV_SRC           = 63, // invalid source address
    BL_STATUS_SEC_RAM               = 64, // image must be in RAM
    BL_STATUS_SEC_INV_TGT           = 65, // can not fit into target memory
    BL_STATUS_SEC_TGT_SIZE          = 66, // source size smaller than target size
    BL_STATUS_SEC_VERIFY            = 67, // signature verification failed

    BL_STATUS_PHYSICAL_ATTACK       = 127,

    BL_STATUS_MAX
} bl_status_t;


/**
 * Store the status, fail info, then jump to _exit.
 */
#define exit_with_status(_cond, _st, _u1, _u2) \
    if (_cond) {                               \
        (void)bl_status_set(_st, _u1, _u2);    \
        status = _st;                          \
        goto _exit;                            \
    }


void bl_status_init(void);


/**
 * Keep the fail info and BOOTLOADER status for later being dumped by
 * bl_status_print() at the end of execution of BOOTLOADER.
 */
bl_status_t bl_status_set(bl_status_t status, uint32_t info_1, uint32_t info_2);


/**
 * Dump the previously saved BOOTLOADER status to console.
 */
void bl_status_print(void);


#endif /* __BL_STATUS_H__ */
