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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "memory_map.h"
#include "hal_log.h"
#include "memory_attribute.h"
#include "wifi_netif.h"
#include "wifi_api.h"
#include "task_def.h"
#include "wifi_api.h"
#include "cyberon_test_cli.h"
#include "license_default_config.h"
#include "CybServer_MTK_Trial_little.h"
#include "hal_flash.h"
#include "memory_map.h"
//log_create_module(httpd_example, PRINT_LEVEL_INFO);
#include "CybDSpotter.h"
extern const uint8_t g_lpbyCybServer_MTK_Trial[312];
static unsigned char cyberon_check(uint8_t len, char *param[])
{
    int nRet = 1;

    nRet = CybDSpotterCheckLicense();
    if (nRet != CYB_SUCCESS) {
        printf("CybDSpotterCheckLicense() fail! ret = %d.\n", nRet);
    }
    //printf("size : %d \n", sizeof(g_lpbyCybServer_MTK_Trial) );
    return 0;
}


static unsigned char cyberon_get_license(uint8_t len, char *param[])
{
    int nRet = 0;
    nRet = CybDSpotterGetLicense((const uint8_t *)g_lpbyCybServer_MTK_Trial, sizeof(g_lpbyCybServer_MTK_Trial));
    if (nRet == CYB_SUCCESS)
        printf("CybDSpotterGetLicense() OK.\n");
    else
        printf("CybDSpotterGetLicense() fail! ret = %d.\n", nRet);
    return 0;

}

static unsigned char cyberon_license_backup(uint8_t len, char *param[])
{
    int nRet = -1;
    nvdm_status_t sts = NVDM_STATUS_OK;
    uint8_t license[512];
    uint32_t start_address = CYBERON_DATA_START & 0x00ffffff;
    uint32_t size = sizeof(license);
    memset(license, 0, sizeof(license));

    /*nRet = CybDSpotterCheckLicense();
    if (nRet != CYB_SUCCESS) {
        printf("CybDSpotterCheckLicense() fail! ret = %d.\n", nRet);
        return nRet;
    }*/
    sts = read_license_key(license, &size);
    if (sts != NVDM_STATUS_OK) {
        printf("Read license failed. sts = %d.\n", sts);
        return nRet;
    }
    printf("license size : %ld, start_address : %ld  \r\n", size, start_address);
    if (HAL_FLASH_STATUS_OK != hal_flash_init()) {
        printf("Flash init failed \n");
        return -1;
    }
    if (HAL_FLASH_STATUS_OK != hal_flash_erase(start_address, HAL_FLASH_BLOCK_4K)) {
        //error handling
        printf("Flash erase failed \n");
        return -1;
    }
    if (HAL_FLASH_STATUS_OK != hal_flash_write(start_address, license, size)) {
        //error handling
        printf("Flash write failed \n");
        return -1;
    }
    uint8_t data_read[LICENSE_BUF_LEN] = {0};
    if (HAL_FLASH_STATUS_OK != hal_flash_read(start_address, data_read, sizeof(data_read))) {
        //error handling
        printf("Flash read failed \n");
        return -1;
    }
    printf("backup to flash \r\n");
    return 0;
}


static unsigned char write_license(uint8_t len, char *param[])
{
    uint32_t start_address = CYBERON_DATA_START & 0x00ffffff;
    uint8_t data_read[LICENSE_BUF_LEN] = {0};
    int nRet = -1;
    nvdm_status_t sts = NVDM_STATUS_OK;

    if (HAL_FLASH_STATUS_OK != hal_flash_init()) {
    }
    if (HAL_FLASH_STATUS_OK != hal_flash_read(start_address, data_read, sizeof(data_read))) {
        //error handling
    }

    sts = write_license_key(data_read, LICENSE_BUF_LEN);
    if (sts != NVDM_STATUS_OK) {
        printf("Write license failed. sts = %d.\n", sts);
        return nRet;
    }
    //sts = write_license_key(license, size);
    return 0;
}


cmd_t cyberon_cli_cmds[] = {
    {"test_check", "Cyberon test check api", cyberon_check, NULL },
    {"test_get_license", "Cyberon test get license api", cyberon_get_license, NULL },
    {"license_backup", "Cyberon license handler", cyberon_license_backup, NULL },
    {"write_license", "Flash write license", write_license, NULL },
    { NULL, NULL, NULL, NULL }
};

