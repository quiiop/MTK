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

#include <stdio.h>
#include <stdlib.h>
#include "hal_flash.h"
#include <string.h>
#include "memextract.h"
#include "exception_handler.h"
#include "memory_map.h"

memextract_entry memextract_entry_array[MEMORY_EXTRACT_MAX_FILES];
uint8_t memextract_entry_num = 0;

static uint32_t user_offset = 0;
static uint32_t erase_offest = 0;

#define MEMORY_EXTRACT_HEADER_BUFFER_SIZE (64)

static void write_header(extract_file_t file, uint32_t addr, int type)
{
    int header_size = 4 + file.filename_size + 4;
    char header[MEMORY_EXTRACT_HEADER_BUFFER_SIZE];

    /* write file name size */
    header[0] = ((file.filename_size-8)) ^ 0xFF;
    header[1] = ((file.filename_size-8) >> 8) ^ 0xFF;
    header[2] = ((file.filename_size-8) >> 16) ^ 0xFF;
    header[3] = ((file.filename_size-8) >> 24) ^ 0xFF;
    /* write file name*/
    unsigned int i;
    int j;
    for(i = 4, j = file.filename_size-1 ; j >= 0 ; i++, j--) {
        if(i < MEMORY_EXTRACT_HEADER_BUFFER_SIZE) {
            header[i] = file.filename[j] ^ 0xFF;
        }
    }
    /* write file size*/
    if(i < MEMORY_EXTRACT_HEADER_BUFFER_SIZE) {
        header[i++] = (file.file_size);
    }
    if(i < MEMORY_EXTRACT_HEADER_BUFFER_SIZE) {
        header[i++] = (file.file_size >> 8);
    }
    if(i < MEMORY_EXTRACT_HEADER_BUFFER_SIZE) {
        header[i++] = (file.file_size >> 16);
    }
    if(i < MEMORY_EXTRACT_HEADER_BUFFER_SIZE) {
        header[i++] = (file.file_size >> 24);
    }
    
    switch(type){
        case PREDEFINED_SECTION:
            hal_flash_write(addr, (uint8_t*)header, header_size);
            break;
        case USER_SECTION:
            hal_flash_write(USER_BASE+user_offset, (uint8_t*)header, header_size);
            user_offset += header_size;
            break;
    }

}

static void memextract_exception_cb(char *buf, unsigned int bufsize)
{
    region_list_t* region_list;
    int region_num;
    uint32_t total_size;

    int swla_offest = 0;
    extract_file_t file;

    for(uint8_t i = 0 ; i < memextract_entry_num ; i++) {
        memextract_entry_array[i].cb(&region_list, &region_num, &total_size);

        switch(memextract_entry_array[i].type){
            case MEMORY_EXTRACT_SWLA:
            {
                /* check if SWLA is too large*/
                if( (SWLA_BASE+swla_offest+total_size) > USER_BASE ){
                        printf("[Memextract]: SWLA size is too large\n");
                        return;
                }
                for(int j = 0 ; j < region_num; j++){
                    hal_flash_write(SWLA_BASE+swla_offest, (uint8_t*)(region_list[j].start_addr), region_list[j].len);
                    swla_offest += region_list[j].len;
                }
                break;
            }
            case MEMORY_EXTRACT_OTHER:
            {
                /* check if the remain space is sufficient or not*/
                if( (USER_BASE+user_offset)+(HEADER_SIZE+total_size) > (LOG_BASE+LOG_FLASH_LENGTH)){
                        printf("[Memextract]: User data %d is too large\n", i+1);
                        return;
                }

                /* check the case in which dummy header cannot fit in*/
                if( ( (USER_BASE+user_offset) + (HEADER_SIZE+total_size) < (LOG_BASE+LOG_FLASH_LENGTH) )
                    && ( (LOG_BASE+LOG_FLASH_LENGTH) - ((USER_BASE+user_offset)+(HEADER_SIZE+total_size)) ) < HEADER_SIZE ){
                        printf("[Memextract]: User data size is invalid.\n");
                        return;
                }

                /* erase blocks if needed */
                if( (USER_BASE+user_offset) + (HEADER_SIZE+total_size) > (USER_BASE + erase_offest) ) {
                    uint32_t align_addr = ( (USER_BASE+user_offset) + (HEADER_SIZE+total_size) ) & ( ~(0x1000-1) );
                    if( ( (USER_BASE+user_offset) + (HEADER_SIZE+total_size) - align_addr ) != 0) {
                        align_addr += 0x1000;
                    }
                    for(; (USER_BASE + erase_offest) < align_addr ; erase_offest += 0x1000) {
                        hal_flash_erase(USER_BASE + erase_offest, HAL_FLASH_BLOCK_4K);
                    }
                }

                file.filename_size = FILENAME_SIZE;
                char str_addr[FILENAME_SIZE+1];
                int ret = 0;
                ret = snprintf(str_addr, ( FILENAME_SIZE + 1 ), "sys_mem_0x%lx", (0x18c3a000+user_offset));
                if(ret < 0) {
                    return;
                }
                file.filename = str_addr;
                file.file_size = total_size;
                file.content = NULL;
                write_header(file, 0, USER_SECTION);

                for(int j = 0; j < region_num; j++){
                    hal_flash_write(USER_BASE+user_offset, (uint8_t*)(region_list[j].start_addr), region_list[j].len);
                    user_offset += region_list[j].len;
                }
                break;
        }
    }
    }

    /* erase blocks for dummy header if needed */
    if((USER_BASE+user_offset) + (HEADER_SIZE) > (USER_BASE + erase_offest) ) {
        uint32_t align_addr_dummy = ((USER_BASE+user_offset) + (HEADER_SIZE)) & ( ~(0x1000-1) );
        printf("align_addr_dummy-(%lu)\n", align_addr_dummy);
        if( ( (USER_BASE+user_offset) + (HEADER_SIZE) - align_addr_dummy ) != 0) {
            align_addr_dummy += 0x1000;
        }
        for(; (USER_BASE + erase_offest) < align_addr_dummy ; erase_offest += 0x1000) {
            hal_flash_erase(USER_BASE + erase_offest, HAL_FLASH_BLOCK_4K);
        }
    }


    /* dummy header to fit memextracter's format*/
    if( (USER_BASE+user_offset) < (LOG_BASE+LOG_FLASH_LENGTH) ) {
        extract_file_t dummy_file;
        dummy_file.filename_size = FILENAME_SIZE;
        dummy_file.filename = "sys_mem_0x18c41000";
        dummy_file.file_size = (LOG_BASE+LOG_FLASH_LENGTH)-(USER_BASE+user_offset)-HEADER_SIZE;
        dummy_file.content = NULL;
        write_header(dummy_file, 0, USER_SECTION);
    }

}

void memextract_register_callbacks(memextract_entry* cb)
{
    memextract_entry_array[memextract_entry_num++] = *cb;
}


/* write EDBGINFO and version */
void memextract_init(void)
{
    /*erase the first 4K of flash log section*/
    hal_flash_erase(LOG_BASE, HAL_FLASH_BLOCK_4K);

    int offset_sf = 0;
    /*write EDBGINFO*/
    char* str = "EDBGINFO";
    hal_flash_write(LOG_BASE + offset_sf, (uint8_t*)str, strlen(str));
    offset_sf += strlen(str);

    /*write Version (2)*/
    str[0] = 2;
    str[1] = 0;
    str[2] = 0;
    str[3] = 0;
    hal_flash_write(LOG_BASE + offset_sf, (uint8_t*)str, 4);
    offset_sf += 4;

    /*write swla header*/
    extract_file_t swla_file;
    swla_file.filename_size = SWLA_FILENAME_SIZE;
    swla_file.filename = "swla_sla_mem";
    swla_file.file_size = SWLA_LENGTH;//4096-64: 4K minus swla header and log header
    swla_file.content = NULL;
    write_header(swla_file, LOG_BASE + offset_sf, PREDEFINED_SECTION);
    offset_sf += (SWLA_HEADER_SIZE+swla_file.file_size);


    /* write log header*/
    extract_file_t log_file;
    log_file.filename_size = FILENAME_SIZE;
    log_file.filename = "sys_mem_0x18c32000";
    log_file.file_size = NORMAL_SYSLOG_LENGTH;
    log_file.content = NULL;
    write_header(log_file, LOG_BASE + offset_sf, PREDEFINED_SECTION);
    offset_sf += (HEADER_SIZE+log_file.file_size);


    /*register exception callback*/
    exception_config_type exception_config;
    exception_config.init_cb = NULL;
    exception_config.dump_cb = memextract_exception_cb;
    exception_register_callbacks(&exception_config);
}