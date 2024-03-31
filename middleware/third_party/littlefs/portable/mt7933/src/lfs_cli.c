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

#if defined(MTK_MINICLI_ENABLE)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cli.h"
#include "lfs.h"
#include "lfs_port.h"


uint8_t _lfs_cli_test_file(uint8_t len, char *param[])
{
    lfs_t lfs;
    lfs_file_t file;

    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
        printf("this should only happen on the first boot\n");
    }

    // read current count
    uint32_t boot_count = 0;
    lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    printf("read boot_count: %d\n", boot_count); 
    
    // update boot count
    boot_count += 1;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);

    // release any resources we were using
    lfs_unmount(&lfs);

    printf("final boot_count: %d\n", boot_count); 

    return 0;
}

uint8_t _lfs_cli_test_dir(uint8_t len, char *param[])
{
    lfs_t lfs;
    lfs_file_t file;
    struct lfs_info info;
    lfs_dir_t dir;
    char path[80];
    char buffer[32];

    int err = lfs_mount(&lfs, &cfg);
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
        printf("this should only happen on the first boot\n");
    }

    for (int i = 0; i < 6; i++) {
        sprintf(path, "dir%03d", i);
        lfs_mkdir(&lfs, path);
    }
    lfs_unmount(&lfs);

    lfs_mount(&lfs, &cfg);
    lfs_dir_open(&lfs, &dir, "/");
    while(lfs_dir_read(&lfs, &dir, &info)){
        printf("info.name:%s\n", info.name);
        printf("info.type:%u\n", info.type);
        printf("info.size:%u\n", info.size);
    }
    lfs_dir_close(&lfs, &dir);
    
    lfs_file_open(&lfs, &file, "dir003/hello", LFS_O_CREAT | LFS_O_WRONLY);
    lfs_file_write(&lfs, &file, "hola", 4);
    lfs_file_write(&lfs, &file, "bonjour", 7);
    lfs_file_write(&lfs, &file, "ohayo", 5);
    lfs_file_close(&lfs, &file);

    printf("---------\n");
    lfs_dir_open(&lfs, &dir, "/");
    while(lfs_dir_read(&lfs, &dir, &info)){
        printf("info.name:%s\n", info.name);
        printf("info.type:%u\n", info.type);
    }
    lfs_dir_close(&lfs, &dir);

   
    printf("---------\n");

    memset(buffer, 0, sizeof(buffer));
    lfs_file_open(&lfs, &file, "dir003/hello", LFS_O_RDONLY);
    lfs_file_read(&lfs, &file, buffer, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';
    printf("hello buffer:%s\n", buffer);
    lfs_file_close(&lfs, &file);
 
    lfs_unmount(&lfs);

    return 0;
}


char wbuffer[10240] = {0};
//null-terminate rbuffer manually before printing
char rbuffer[10241] = {0};

uint8_t _lfs_cli_test_bigfile(uint8_t len, char *param[])
{
    lfs_t lfs;
    lfs_file_t file;
    int i = 0, j = 0;
    int value = 0;

    int err = lfs_mount(&lfs, &cfg);
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
        printf("this should only happen on the first boot\n");
    }
    
    lfs_file_open(&lfs, &file, "BIG_FILE", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    printf("file size:%d\n",lfs_file_size(&lfs, &file));
    
    value = 0x41;
    for(i=0;i<10;i++) {
        value = 0x41 + i;
        memset(wbuffer+(i*1024), value, 1024);
    }
 
    //10240*300=3Mbytes 
    for(j=0;j<300;j++) {
        lfs_file_write(&lfs, &file, wbuffer, sizeof(wbuffer));
        printf("file size:%d\n",lfs_file_size(&lfs, &file));
        printf("current pos:%d\n", lfs_file_tell(&lfs, &file));
    }
    lfs_file_close(&lfs, &file);

    printf("---------\n");

    lfs_file_open(&lfs, &file, "BIG_FILE", LFS_O_RDONLY);
    for(j=0;j<300;j++) {
        lfs_file_read(&lfs, &file, rbuffer, sizeof(rbuffer)-1);
        rbuffer[sizeof(rbuffer)-1] = '\0';
        printf("rbuffer:%s\n", rbuffer);
        printf("current pos:%d\n", lfs_file_tell(&lfs, &file));
    }
    lfs_file_close(&lfs, &file);

    lfs_unmount(&lfs);

    return 0;
}

uint8_t _lfs_cli_format_test(uint8_t len, char *param[])
{
    lfs_t lfs;

    lfs_format(&lfs, &cfg);
    return 0;
}


/****************************************************************************
 *
 * API functions.
 *
 ****************************************************************************/

cmd_t lfs_cli[] = {
    { "test_file"     ,  "lfs test file"      , _lfs_cli_test_file,    NULL},
    { "test_dir"      ,  "lfs test dir"       , _lfs_cli_test_dir,     NULL},
    { "test_bigfile"  ,  "lfs test bigfile"   , _lfs_cli_test_bigfile, NULL},
    { "format"        ,  "lfs test format"    , _lfs_cli_format_test,  NULL},
    { NULL }
};

#endif /* #if defined(MTK_MINICLI_ENABLE) */
