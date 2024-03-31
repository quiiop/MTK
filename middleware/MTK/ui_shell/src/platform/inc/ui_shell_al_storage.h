/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#ifndef __UI_SHELL_AL_STORAGE_H__
#define __UI_SHELL_AL_STORAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    AL_STORAGE_DATA_ITEM_TYPE_RAW_DATA = 0x01,   /**< Defines the display type with raw data. */
    AL_STORAGE_DATA_ITEM_TYPE_STRING = 0x02,     /**< Defines the display type with string. */
} ui_shell_al_storage_data_item_type_t;

typedef enum {
    AL_STORAGE_STATUS_INVALID_PARAMETER = -5,  /**< The user parameter is invalid. */
    AL_STORAGE_STATUS_ITEM_NOT_FOUND = -4,     /**< The data item wasn't found by the NVDM. */
    AL_STORAGE_STATUS_INSUFFICIENT_SPACE = -3, /**< No space is available in the flash. */
    AL_STORAGE_STATUS_INCORRECT_CHECKSUM = -2, /**< The NVDM found a checksum error when reading the data item. */
    AL_STORAGE_STATUS_ERROR = -1,              /**< An unknown error occurred. */
    AL_STORAGE_STATUS_OK = 0,                  /**< The operation was successful. */
} ui_shell_al_storage_status_t;


ui_shell_al_storage_status_t ui_shell_al_storage_write_data_item(const char *group_name,
                                   const char *data_item_name,
                                   ui_shell_al_storage_data_item_type_t type,
                                   const uint8_t *buffer,
                                   uint32_t size);

ui_shell_al_storage_status_t ui_shell_al_storage_read_data_item(const char *group_name,
                                  const char *data_item_name,
                                  uint8_t *buffer,
                                  uint32_t *size);

#ifdef __cplusplus
}
#endif

#endif /* __UI_SHELL_AL_STORAGE_H__ */
