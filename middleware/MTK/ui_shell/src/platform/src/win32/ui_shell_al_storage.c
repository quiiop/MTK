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

#include "ui_shell_al_storage.h"
#include "stdio.h"
#include "string.h"

#define STORAGE_MAX_LEN    (512*2)

ui_shell_al_storage_status_t ui_shell_al_storage_write_data_item(const char *group_name,
                                               const char *data_item_name,
                                               ui_shell_al_storage_data_item_type_t type,
                                               const uint8_t *buffer,
                                               uint32_t size)
{
    FILE * fp;
    uint8_t data[STORAGE_MAX_LEN] = { 0 };
    int32_t current_position = 0;
    int32_t name_len = strlen(data_item_name) + 1;
    int32_t group_name_len = strlen(group_name) + 1;
    uint32_t active_data_len = 0;

    if (buffer == NULL || size == 0){
        return AL_STORAGE_STATUS_INVALID_PARAMETER;
    }
    fp = fopen("test.txt", "a");
    fclose(fp);
    fp = fopen("test.txt", "rb+");
    fseek(fp, 0L, SEEK_SET);
    while(1) {
        int32_t readed_len = fread(data, 1, STORAGE_MAX_LEN, fp);
        if (readed_len > 0) {
            if (strcmp(group_name, (char *)data) == 0) {
                if (strcmp(data_item_name, (char *)data + group_name_len) == 0) {
                    break;
                }
            }
        } else {
            memcpy(data, group_name, group_name_len);
            memcpy(data + group_name_len, data_item_name, name_len);
            break;
        }
        current_position += STORAGE_MAX_LEN;
    }
    fseek(fp, current_position, SEEK_SET);
    active_data_len = group_name_len + name_len;
    memcpy(data + active_data_len, &size, sizeof(size));
    active_data_len += sizeof(size);
    memcpy(data + active_data_len, buffer, size);
    active_data_len += size;
    memset(data + active_data_len, 0, STORAGE_MAX_LEN - active_data_len);
    data[STORAGE_MAX_LEN - 2] = '\n';
    data[STORAGE_MAX_LEN - 1] = '\r';

    fwrite(data, 1, STORAGE_MAX_LEN, fp);
    fclose(fp);
    return AL_STORAGE_STATUS_OK;
}

ui_shell_al_storage_status_t ui_shell_al_storage_read_data_item(const char *group_name,
                                              const char *data_item_name,
                                              uint8_t *buffer,
                                              uint32_t *size)
{
    FILE * fp;
    int32_t name_len = strlen(data_item_name) + 1;
    int32_t group_name_len = strlen(group_name) + 1;
    uint8_t data[STORAGE_MAX_LEN] = { 0 };
    ui_shell_al_storage_status_t result = AL_STORAGE_STATUS_ITEM_NOT_FOUND;
    fp = fopen("test.txt", "rb");
    if (fp == NULL) {
        memset(buffer, 0, *size);
        *size = 0;
        return AL_STORAGE_STATUS_ITEM_NOT_FOUND;
    } else {
        fseek(fp, 0L, SEEK_SET);
        while(1) {
            int32_t readed_len = fread(data, 1, STORAGE_MAX_LEN, fp);
            if (readed_len > 0) {
                if (strcmp(group_name, (char *)data) == 0
                        && strcmp(data_item_name, (char *)data + group_name_len) == 0) {
                    uint8_t *current_data_position = data + group_name_len + name_len;
                    uint32_t data_size = *(uint32_t *)(current_data_position);
                    if (data_size >= *size) {
                        data_size = *size;
                    }
                    current_data_position += sizeof(data_size);
                    memcpy(buffer, current_data_position, data_size);
                    if (*size > data_size) {
                        memset(buffer + data_size, 0, *size - data_size);
                    }
                    *size = data_size;
                    result = AL_STORAGE_STATUS_OK;
                    break;
                }
            } else {
                memset(buffer, 0, *size);
                *size = 0;
                break;
            }
        }
        fclose(fp);
        return result;
    }
}
