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

#ifndef __BT_LE_AUDIO_TYPE_H__
#define __BT_LE_AUDIO_TYPE_H__

#include "bt_type.h"

/**
 * @brief Defines the LE Audio module.
 */

#define BT_LE_AUDIO_MODULE_OFFSET   8       /**< Module offset.*/

/**
 * @brief Defines the LE Audio module type
 */
#define BT_LE_AUDIO_MODULE_COMMON          (0x00<<BT_LE_AUDIO_MODULE_OFFSET)  /**< Prefix of the common module. 0x0000*/
#define BT_LE_AUDIO_MODULE_ASE             (0x01<<BT_LE_AUDIO_MODULE_OFFSET)  /**< Prefix of the ASE module. 0x0100*/
#define BT_LE_AUDIO_MODULE_BASE            (0x02<<BT_LE_AUDIO_MODULE_OFFSET)  /**< Prefix of the BASE module. 0x0200*/
#define BT_LE_AUDIO_MODULE_CALL            (0x03<<BT_LE_AUDIO_MODULE_OFFSET)  /**< Prefix of the CALL module. 0x0300*/
#define BT_LE_AUDIO_MODULE_MEDIA           (0x04<<BT_LE_AUDIO_MODULE_OFFSET)  /**< Prefix of the MEDIA module. 0x0400*/
#define BT_LE_AUDIO_MODULE_VOLUME          (0x05<<BT_LE_AUDIO_MODULE_OFFSET)  /**< Prefix of the VOLUME module. 0x0500*/
#define BT_LE_AUDIO_MODULE_MICROPHONE      (0x06<<BT_LE_AUDIO_MODULE_OFFSET)  /**< Prefix of the MICROPHONE module. 0x0600*/
#define BT_LE_AUDIO_MODULE_ROUTING         (0x07<<BT_LE_AUDIO_MODULE_OFFSET)  /**< Prefix of the ROUTING module. 0x0700*/

#define BT_LE_AUDIO_MODULE_MASK             0xFFFF0000U /**< Mask for LE Audio module. */
#define BT_LE_AUDIO_MODULE_VCP              (BT_MODULE_LE_AUDIO | 0x010000)  /**< Prefix of the VCP module. 0x70010000 */
#define BT_LE_AUDIO_MODULE_VCP_VCS          (BT_MODULE_LE_AUDIO | 0x011000)  /**< Prefix of the VCP VCS module. 0x70011000 */
#define BT_LE_AUDIO_MODULE_CCP              (BT_MODULE_LE_AUDIO | 0x020000)  /**< Prefix of the CCP module. 0x70020000 */
#define BT_LE_AUDIO_MODULE_MCP              (BT_MODULE_LE_AUDIO | 0x030000)  /**< Prefix of the MCP module. 0x70030000 */
#define BT_LE_AUDIO_MODULE_MICP             (BT_MODULE_LE_AUDIO | 0x040000)  /**< Prefix of the MICP module. 0x70040000 */
#define BT_LE_AUDIO_MODULE_MICP_MICS        (BT_MODULE_LE_AUDIO | 0x041000)  /**< Prefix of the MICP MICS module. 0x70041000 */
#define BT_LE_AUDIO_MODULE_HAPS             (BT_MODULE_LE_AUDIO | 0x050000)  /**< Prefix of the HAPS module. 0x70050000 */

#endif  /* __BT_LE_AUDIO_TYPE_H__ */

