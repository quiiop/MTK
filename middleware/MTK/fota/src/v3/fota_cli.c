/* Copyright Statement:
 *
 * (C) 2021-2021  MediaTek Inc. All rights reserved.
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


#ifdef MTK_FOTA_V3_CLI_ENABLE


/****************************************************************************
 *
 * HEADER FILES
 *
 ****************************************************************************/


// toolchain header
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* v3 header */
#include <v3/fota.h>
#include <v3/fota_cli.h>
#include <v3/fota_download.h>
#include <v3/url.h>

// per project header
#include "fota_flash_config.h"
#include "memory_map.h"


/****************************************************************************
 *
 * FORWARD DECLARATIONS
 *
 ****************************************************************************/


static uint8_t _fota_cli_init    ( uint8_t argc, char *argv[] );
static uint8_t _fota_cli_status  ( uint8_t argc, char *argv[] );
static uint8_t _fota_cli_dl      ( uint8_t argc, char *argv[] );
static uint8_t _fota_cli_mem     ( uint8_t argc, char *argv[] );
static uint8_t _fota_cli_sha1    ( uint8_t argc, char *argv[] );

static uint8_t _fota_cli_trig_yes( uint8_t argc, char *argv[] );
static uint8_t _fota_cli_trig_no ( uint8_t argc, char *argv[] );


/****************************************************************************
 *
 * GLOBAL VARIABLES
 *
 ****************************************************************************/


static const cmd_t _fota_cli_trig[] = {
    { "set",    "set upgrade flag",             _fota_cli_trig_yes, NULL },
    { "clear",  "clear upgrade flag",           _fota_cli_trig_no,  NULL },
    { NULL,     NULL,                           NULL,               NULL }
};


const cmd_t fota_cli[] = {
    { "init",   "init fota",                    _fota_cli_init,     NULL },
    { "status", "show fota status",             _fota_cli_status,   NULL },
    { "dl",     "download from <url> to flash", _fota_cli_dl,       NULL },
    { "mem",    "memory leak debug",            _fota_cli_mem,      NULL },
    { "sha1",   "calculate sha1",               _fota_cli_sha1,     NULL },

    { "trig",   "write trigger flag",           NULL,     _fota_cli_trig },
    { NULL,     NULL,                           NULL,               NULL }
};


/****************************************************************************
 *
 * PRIVATE FUNCTIONS
 *
 ****************************************************************************/


static uint8_t _fota_cli_trig_yes(uint8_t argc, char *argv[])
{
    fota_status_t         ret;

    ret = fota_trigger_upgrade(&g_fota_flash_config, ROM_REGION_FOTA);
    if (ret != FOTA_STATUS_OK) {
        cli_puts("trigger upgrade failed: ");
        cli_putd(ret);
        cli_putln();
        return 1;
    }

    return 0;
}


static uint8_t _fota_cli_trig_no(uint8_t argc, char *argv[])
{
    fota_status_t         ret;

    ret = fota_defuse_upgrade(&g_fota_flash_config, ROM_REGION_FOTA);
    if (ret != FOTA_STATUS_OK) {
        cli_puts("defuse upgrade failed: ");
        cli_putd(ret);
        cli_putln();
        return 1;
    }

    return 0;
}


static uint8_t _fota_cli_init(uint8_t argc, char *argv[])
{
    return 0;
}


static uint8_t _fota_cli_status(uint8_t argc, char *argv[])
{
    fota_status_t           ret;
    fota_upgrade_info_t     info;

    ret = fota_read_info(&g_fota_flash_config, &info, ROM_REGION_FOTA);
    if (ret != FOTA_STATUS_OK) {
        cli_puts("unable to read info\n");
        return 1;
    }

    cli_puts("info: ");
    cli_putd(info.state);
    cli_putln();

    return 0;
}


static uint8_t _fota_cli_dl(uint8_t argc, char *argv[])
{
    url_t           url;
    bool            dl_only = false;
    fota_status_t   status;

    do {
        if (argc == 0) {
            cli_puts("no url\n");
            break;
        }

        if (argc == 2) {
            if (strcmp(argv[1], "dl_only") != 0) {
                cli_puts("invalid mode");
                cli_putln();
                break;
            }
            dl_only = true;
        }

        if (url_parse(&url, argv[0]) < 0) {
            cli_puts("invalid url\n");
            break;
        } else {
            cli_puts("url: ");
            cli_putd(url.scheme);
            cli_putc(' ');
            cli_puts(url.host);
            cli_putc(' ');
            cli_putd(url.port);
            cli_putc(' ');
            cli_puts(url.path);
            cli_putln();
        }

        status = fota_download(&url, &g_fota_flash_config,
                               ROM_REGION_FOTA, dl_only);
        if (status != FOTA_STATUS_OK) {
            cli_puts("download fail: ");
            cli_putd(status);
            cli_putln();
        } else {
            cli_puts("fota download success");
            cli_putln();
        }

        url_free(&url);
    } while (0);

    return 0;
}


#include "hal_sha.h"

static uint8_t _fota_cli_sha1(uint8_t argc, char *argv[])
{
    hal_sha1_context_t  ctx;
    uint8_t             answer[ HAL_SHA1_DIGEST_SIZE ];
    int                 i = 0;

    for (i = 0; i < argc; i++) {
        if (hal_sha1_init(&ctx) != HAL_SHA_STATUS_OK) {
            cli_puts("sha1 init fail\n");
            continue;
        }

        if (hal_sha1_append(&ctx, (uint8_t *)argv[ i ],
                            strlen(argv[ i ])) != HAL_SHA_STATUS_OK) {
            cli_puts("sha1 append fail\n");
            continue;
        }

        if (hal_sha1_end(&ctx, answer) != HAL_SHA_STATUS_OK) {
            cli_puts("sha1 append fail\n");
            continue;
        }

        for (int j = 0; j < HAL_SHA1_DIGEST_SIZE; j++) {
            cli_putx(answer[j] & 0xFF);
            cli_putc(' ');
        }
        cli_putln();
    }

    return 0;
}


static uint8_t _fota_cli_mem(uint8_t argc, char *argv[])
{
#ifdef LEAK_HUNT
#define MALLOC_RECORDS (1000)
    struct malloc_record {
        const char *f;
        int         l;
        void        *p;
        size_t      s;
    };
    int i;
    extern struct malloc_record malloc_records[ MALLOC_RECORDS ];
    for (i = 0; i < MALLOC_RECORDS; i++)
        if (malloc_records[i].f) {
            printf("%s %u %p %d\n",
                   malloc_records[i].f, malloc_records[i].l, malloc_records[i].p, malloc_records[i].s);
            malloc_records[i].f = NULL;
        }
#endif
    return 0;
}


#endif /* MTK_FOTA_V3_CLI_ENABLE */

