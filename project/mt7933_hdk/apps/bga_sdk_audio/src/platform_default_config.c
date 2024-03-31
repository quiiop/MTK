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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nvdm.h"
#include "nvdm_ctrl.h"

#include "exception_handler.h"


#define MAX_CORE_DUMP_SIZE 4096
#define MAX_DATA_ITEM_SIZE 2048  //middleware\MTK\nvdm_core\inc_core\nvdm_internal.h

extern memory_region_type memory_regions[];

static const group_data_item_t g_platform_coredump_data_array[] = {
    NVDM_DATA_ITEM("dumpfmt",  "0"),
    NVDM_DATA_ITEM("dumpsize",  "0"),
    NVDM_RAW_DATA_ITEM("dumpdata1",  "null", sizeof("null")),
    NVDM_RAW_DATA_ITEM("dumpdata2",  "null", sizeof("null"))
};

static const group_data_item_t g_platform_memdump_data_array[] = {
    NVDM_DATA_ITEM("dumpfmt",  "1"),
};

#ifdef MTK_MINI_DUMP_ENABLE
static uint8_t crdump_lock = 0;
#endif

uint8_t  dumpbuffer[MAX_CORE_DUMP_SIZE] = {0};

static void coredump_check_default_value(void)
{
    check_default_value("coredump",
                        g_platform_coredump_data_array,
                        sizeof(g_platform_coredump_data_array) / sizeof(g_platform_coredump_data_array[0]));
}

static void coredump_reset_to_default(void)
{
    reset_to_default("coredump",
                     g_platform_coredump_data_array,
                     sizeof(g_platform_coredump_data_array) / sizeof(g_platform_coredump_data_array[0]));
}

void coredump_show_value(void)
{
    uint32_t dfmt    = 0;
    uint32_t dumpsize = 0;
    uint32_t buffer_size = MAX_CORE_DUMP_SIZE;

    nvdm_status_t status;

    memset(&dumpbuffer[0], 0, sizeof(dumpbuffer));

    // Get Core Dump Display Forma
    status = nvdm_read_data_item("coredump", "dumpfmt", dumpbuffer, &buffer_size);
    if (status != NVDM_STATUS_OK) {
        printf("[CoreDump]: Unknow Display Format!\r\n");
        return;
    }
    dfmt = atoi((const char *)dumpbuffer);

    // Get Core Dump Size
    buffer_size = MAX_CORE_DUMP_SIZE;
    status = nvdm_read_data_item("coredump", "dumpsize", dumpbuffer, &buffer_size);
    if (status != NVDM_STATUS_OK) {
        printf("[CoreDump]: Unknow Core Dump Size!\r\n");
        return;
    }
    dumpsize = atoi((const char *)dumpbuffer);

    memset(&dumpbuffer[0], 0, sizeof(dumpbuffer));
    buffer_size = MAX_CORE_DUMP_SIZE;
    status = nvdm_read_data_item("coredump", "dumpdata1", dumpbuffer, &buffer_size);
    if (NVDM_STATUS_OK != status) {
        printf("[CoreDump]: data read fail - dumpdata1\r\n");
        return;
    }

    if (dumpsize > MAX_DATA_ITEM_SIZE) {
        buffer_size = (MAX_CORE_DUMP_SIZE - MAX_DATA_ITEM_SIZE);

        status = nvdm_read_data_item("coredump", "dumpdata2", dumpbuffer + MAX_DATA_ITEM_SIZE, &buffer_size);
        if (NVDM_STATUS_OK != status) {
            printf("[CoreDump]: data read fail - dumpdata2\r\n");
            return;
        }
    }

    printf("[coredump]: dumpfmt(%lu), dumpsize(%lu)\r\n", dfmt, dumpsize);
    if (dfmt && dumpsize) {
        exception_dump_show(0, (unsigned int *)&dumpbuffer[0], dumpsize);
    }
}


static void memdump_check_default_value(void)
{
    check_default_value("memdump",
                        g_platform_memdump_data_array,
                        sizeof(g_platform_memdump_data_array) / sizeof(g_platform_memdump_data_array[0]));
}

static void memdump_reset_to_default(void)
{
    reset_to_default("memdump",
                     g_platform_memdump_data_array,
                     sizeof(g_platform_memdump_data_array) / sizeof(g_platform_memdump_data_array[0]));
}

static void memdump_show_value(void)
{
    show_group_value("memdump",
                     g_platform_memdump_data_array,
                     sizeof(g_platform_memdump_data_array) / sizeof(g_platform_memdump_data_array[0]));
}

const user_data_item_operate_t platform_data_item_array[] = {
    {
        "coredump",
        coredump_check_default_value,
        coredump_reset_to_default,
        coredump_show_value,
    },
    {
        "memdump",
        memdump_check_default_value,
        memdump_reset_to_default,
        memdump_show_value,
    },
};


#ifdef MTK_MINI_DUMP_ENABLE
void plat_except_init_cb(char *buf, unsigned int bufsize)
{
    uint32_t dfmt    = 0;
    uint32_t buffer_size = MAX_CORE_DUMP_SIZE;
    nvdm_status_t status;

    status = nvdm_read_data_item("memdump", "dumpfmt", dumpbuffer, &buffer_size);
    if (status != NVDM_STATUS_OK) {
        printf("[MemDump]: Unknow Memory Dump Format!\r\n");
        return;
    }
    dfmt = atoi((const char *)dumpbuffer);
    printf("Memory Dump Format(%lu)\r\n", dfmt);

    exception_dump_config(EXCEPT_CFG_MEM_DUMP, dfmt);
}

void plat_except_coredump_cb(char *buf, unsigned int bufsize)
{
    char          sbuf[10] = {0};
    nvdm_status_t status;

    if (buf == NULL || bufsize == 0 || bufsize > MAX_CORE_DUMP_SIZE) {
        printf("Invalid Core Dump Info...\r\n");
        return;
    }

    if (crdump_lock) {
        printf("Skip Core Dump Save for Nested Exception!");
        return;
    }

    crdump_lock++;

    status = nvdm_write_data_item("coredump", "dumpsize", NVDM_DATA_ITEM_TYPE_STRING, (uint8_t *)"0", sizeof("0"));
    if (NVDM_STATUS_OK != status) {
        printf("Platform Core Dump Write Fail - dumpsize \r\n");
        return;
    }

    status = nvdm_write_data_item("coredump", "dumpdata1", NVDM_DATA_ITEM_TYPE_RAW_DATA, (uint8_t *)buf, (bufsize > MAX_DATA_ITEM_SIZE) ? MAX_DATA_ITEM_SIZE : bufsize);
    if (NVDM_STATUS_OK != status) {
        printf("Platform Core Dump Write Fail - dumpdata1 \r\n");
        return;
    }

    if (bufsize > MAX_DATA_ITEM_SIZE) {
        status = nvdm_write_data_item("coredump", "dumpdata2", NVDM_DATA_ITEM_TYPE_RAW_DATA, (uint8_t *)buf + MAX_DATA_ITEM_SIZE, (bufsize - MAX_DATA_ITEM_SIZE));
        if (NVDM_STATUS_OK != status) {
            printf("Platform Core Dump Write Fail - dumpdata2 \r\n");
            return;
        }
    }

    memset(sbuf, 0, sizeof(sbuf));
    itoa(bufsize, sbuf, 10);
    status = nvdm_write_data_item("coredump", "dumpsize", NVDM_DATA_ITEM_TYPE_STRING, (uint8_t *)sbuf, strlen(sbuf));
    if (NVDM_STATUS_OK != status) {
        printf("Platform Core Dump Write Fail - dumpsize \r\n");
        return;
    }
    printf("Platform Core Dump Write OK! - size(%s)\r\n", sbuf);
}

void plat_exception_handler_init(void)
{
    exception_config_type plat_except_config;

    plat_except_config.init_cb   = plat_except_init_cb;
    plat_except_config.dump_cb   = plat_except_coredump_cb;

    exception_register_callbacks(&plat_except_config);
}
#endif

