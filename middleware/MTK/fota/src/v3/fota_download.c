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


/****************************************************************************
 *
 * HEADER FILES
 *
 ****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <syslog.h>
#ifdef MTK_FOTA_V3_TFTP_ENABLE
#include <tftp.h>
#include <tftpc.h>
#endif
#if defined(MTK_FOTA_V3_HTTP_ENABLE) || defined(MTK_FOTA_V3_HTTPS_ENABLE)
#include "httpclient.h"
#endif

// fotav3
#include <v3/fota_download.h>
#include <v3/url.h>
#include "fota_osal.h"
#include "fota_log.h"

// per project
//#include "memory_map.h"


/****************************************************************************
 *
 * CONSTANTS AND MACROS
 *
 ****************************************************************************/


#if defined(MTK_FOTA_V3_HTTP_ENABLE) || defined(MTK_FOTA_V3_HTTPS_ENABLE)
#define HTTP_DOWNLOAD_BUF_SIZE (4096)
#endif


/****************************************************************************
 *
 * TYPE DECLARATION
 *
 ****************************************************************************/


#if defined(MTK_FOTA_V3_HTTP_ENABLE) || defined(MTK_FOTA_V3_HTTPS_ENABLE)
typedef struct _http_dl_t {
    char                *url;
    httpclient_t        client;
    httpclient_data_t   data;
    uint32_t            pos;      // receive index
    char                buf[ HTTP_DOWNLOAD_BUF_SIZE ];
    fota_io_state_t     io;
} http_dl_t;
#endif


/****************************************************************************
 *
 * GLOBAL VARIABLES
 *
 ****************************************************************************/


log_create_module(fota_dl_api, PRINT_LEVEL_INFO);


/****************************************************************************
 *
 * PUBLIC FUNCTIONS
 *
 ****************************************************************************/


#ifdef MTK_FOTA_V3_TFTP_ENABLE
/**
 * Download FOTA update package using TFTP protocol.
 *
 * If success, then reboot device and trigger update process.
 *
 */
ssize_t fota_download_by_tftp(
    char                *address,
    uint16_t            rport,
    char                *filename,
    const uint32_t      partition,
    bool                writing,
    const fota_flash_t  *flash)
{
    tftpc_t             *tftpc;
    ssize_t             total_len = 0;
    tftpc_status_t      status;
    uint16_t            pkt_len;
    uint16_t            lport = 21000; // TODO: use random number
    fota_io_state_t     io;

    tftpc = tftpc_read_init(address, rport, lport, filename);
    if (! tftpc) {
        LOG_I(fota_dl_api, "tftpc init failed\n");
        return -1;
    }

    if (writing) {
        if (fota_io_init(flash, partition, &io) != FOTA_STATUS_OK) {
            return -2;
        }

        if (fota_io_seek(&io, 0) != FOTA_STATUS_OK) {
            return -3;
        }
    }

    do {
        status = tftpc_read_data(tftpc, &pkt_len);

        if (status != TFTPC_STATUS_MORE && status != TFTPC_STATUS_DONE) {
            LOG_E(fota_dl_api, "download error\n");
            total_len = -4;
        }

        if (pkt_len != 0) {
            pkt_len     -= 4;
            total_len   += pkt_len;

            if (writing) {
                if (fota_io_write(&io,
                                  &tftpc->buf[0] + 4,
                                  pkt_len) != FOTA_STATUS_OK) {
                    total_len = -5;
                    LOG_I(fota_dl_api, "flash write erorr\n");
                    break;
                }
            }
        }
    } while (status == TFTPC_STATUS_MORE);

    tftpc_read_done(tftpc);

    return total_len;
}
#endif /* MTK_FOTA_V3_TFTP_ENABLE */


#ifdef MTK_FOTA_V3_HTTP_ENABLE
/**
 * Download FOTA update package using HTTP protocol.
 *
 * If success, then reboot device and trigger update process.
 */
ssize_t fota_download_by_http(
    char                *address,
    uint16_t            rport,
    char                *filename,
    const uint32_t      partition,
    bool                writing,
    const fota_flash_t  *flash)
{
    http_dl_t           *h = NULL;
    int32_t             ret;
    ssize_t             status;
    char                *block = NULL;

    do {
        /* prepare control block */

        status = -1;
        h = fota_malloc(sizeof(*h));
        if (! h)
            break;
        else
            memset(h, 0, sizeof(*h));

        if (writing) {
            block = fota_malloc(flash->block_size);
            if (! block)
                break;
            else
                memset(block, 0, sizeof(flash->block_size));

            if (fota_io_init(flash, partition, &h->io) != FOTA_STATUS_OK) {
                status = -10;
                break;
            }

            if (fota_io_seek(&h->io, 0) != FOTA_STATUS_OK) {
                status = -11;
                break;
            }

            // clear header
            if (fota_io_write(&h->io, block,
                              flash->block_size) != FOTA_STATUS_OK) {
                status = -12;
                break;
            }

            if (fota_io_seek(&h->io,
                             - flash->block_size) != FOTA_STATUS_OK) {
                status = -13;
                break;
            }

            // clear info block
            if (fota_io_write(&h->io, block,
                              flash->block_size) != FOTA_STATUS_OK) {
                status = -14;
                break;
            }

            fota_free(block);
            block = NULL;

            if (fota_io_seek(&h->io, 0) != FOTA_STATUS_OK) {
                status = -15;
                break;
            }
        }

        /* prepare url */

        status = -2;
        h->url = fota_malloc(snprintf(NULL, 0, "http://%s:%d/%s",
                                      address, rport, filename) + 1);
        if (! h->url)
            break;
        else
            (void)sprintf(h->url, "http://%s:%d/%s",
                          address, rport, filename);

        /* build connection */

        status = -3;
        ret = httpclient_connect(&h->client, h->url);
        if (ret != 0) {
            W("http connect fail");
            break;
        }

        /* send request */

        status = -4;
        h->data.response_buf     = &h->buf[ 0 ];
        h->data.response_buf_len = sizeof(h->buf);
        ret = httpclient_send_request(&h->client, h->url,
                                      HTTPCLIENT_GET, &h->data);
        if (ret < 0) {
            W("http send request fail");
            break;
        }

        /* download loop */

        status = -5;
        do {
            uint32_t len;

            ret = httpclient_recv_response(&h->client, &h->data);
            if (ret < 0) {
                W("rx err %d", ret);
                status = -51;
                break;
            }

            if (h->data.response_content_len >= 0 &&
                (uint32_t)h->data.response_content_len >
                h->io.size - sizeof(fota_upgrade_info_t)) {
                W("object too large (%d)", h->data.response_content_len);
                status = -52;
                break;
            }

            len = h->data.response_content_len -
                  h->data.retrieve_len - h->pos;
#if 0
            // data.retrieve_len is the bytes yet to be retrieved
            // data.response_content_len is the total size of .bin

            V("%u %u %u %u", h->pos, len,
              h->data.retrieve_len,
              h->data.response_content_len);
#endif
            if (writing && fota_io_write(&h->io, &h->data.response_buf[0],
                                         len) != FOTA_STATUS_OK) {
                ret = -53;
                break;
            }

            // update current position
            h->pos += len;

        } while (ret == HTTPCLIENT_RETRIEVE_MORE_DATA);

        if (ret != HTTPCLIENT_OK)
            break;

        /* received length */
        status = h->pos;
    } while (0);

    // error handling

    if (h) {
        httpclient_close(&h->client);
        if (h->url)
            fota_free(h->url);
        fota_free(h);
    }

    if (block)
        fota_free(block);

    return status;
}
#endif /* MTK_FOTA_V3_HTTP_ENABLE */


#ifdef MTK_FOTA_V3_HTTPS_ENABLE
/**
 * Download FOTA update package using HTTP protocol.
 *
 * If success, then reboot device and trigger update process.
 *
 */
ssize_t fota_download_by_https(
    char                *address,
    uint16_t            rport,
    char                *filename,
    const uint32_t      partition,
    bool                writing,
    const fota_flash_t  *flash)
{
    http_dl_t           *h = NULL;
    int32_t             ret;
    ssize_t             status;
    char                *block = NULL;

    do {
        /* prepare control block */

        status = -1;
        h = fota_malloc(sizeof(*h));
        if (! h)
            break;
        else
            memset(h, 0, sizeof(*h));

        if (writing) {
            block = fota_malloc(flash->block_size);
            if (! block)
                break;
            else
                memset(block, 0, sizeof(flash->block_size));

            if (fota_io_init(flash, partition, &h->io) != FOTA_STATUS_OK) {
                status = -10;
                break;
            }

            if (fota_io_seek(&h->io, 0) != FOTA_STATUS_OK) {
                status = -11;
                break;
            }

            // clear header
            if (fota_io_write(&h->io, block,
                              flash->block_size) != FOTA_STATUS_OK) {
                status = -12;
                break;
            }

            if (fota_io_seek(&h->io,
                             - flash->block_size) != FOTA_STATUS_OK) {
                status = -13;
                break;
            }

            // clear info block
            if (fota_io_write(&h->io, block,
                              flash->block_size) != FOTA_STATUS_OK) {
                status = -14;
                break;
            }

            fota_free(block);
            block = NULL;

            if (fota_io_seek(&h->io, 0) != FOTA_STATUS_OK) {
                status = -15;
                break;
            }
        }

        /* prepare url */

        status = -2;
        h->url = fota_malloc(snprintf(NULL, 0, "https://%s:%d/%s",
                                      address, rport, filename) + 1);
        if (! h->url)
            break;
        else
            (void)sprintf(h->url, "https://%s:%d/%s",
                          address, rport, filename);

        /* build connection */

        status = -3;
        ret = httpclient_connect(&h->client, h->url);
        if (ret != 0) {
            W("http connect fail");
            break;
        }

        /* send request */

        status = -4;
        h->data.response_buf     = &h->buf[ 0 ];
        h->data.response_buf_len = sizeof(h->buf);
        ret = httpclient_send_request(&h->client, h->url,
                                      HTTPCLIENT_GET, &h->data);
        if (ret < 0) {
            W("http send request fail");
            break;
        }

        /* download loop */

        status = -5;
        do {
            uint32_t len;

            ret = httpclient_recv_response(&h->client, &h->data);
            if (ret < 0) {
                W("rx err %d", ret);
                status = -51;
                break;
            }

            if (h->data.response_content_len >= 0 &&
                (uint32_t)h->data.response_content_len >
                h->io.size - sizeof(fota_upgrade_info_t)) {
                W("object too large (%d)", h->data.response_content_len);
                status = -52;
                break;
            }

            len = h->data.response_content_len -
                  h->data.retrieve_len - h->pos;
#if 0
            // data.retrieve_len is the bytes yet to be retrieved
            // data.response_content_len is the total size of .bin

            V("%u %u %u %u", h->pos, len,
              h->data.retrieve_len,
              h->data.response_content_len);
#endif
            if (writing && fota_io_write(&h->io, &h->data.response_buf[0],
                                         len) != FOTA_STATUS_OK) {
                ret = -53;
                break;
            }

            // update current position
            h->pos += len;

        } while (ret == HTTPCLIENT_RETRIEVE_MORE_DATA);

        if (ret != HTTPCLIENT_OK)
            break;

        /* received length */
        status = h->pos;
    } while (0);

    // error handling

    if (h) {
        httpclient_close(&h->client);
        if (h->url)
            fota_free(h->url);
        fota_free(h);
    }

    if (block)
        fota_free(block);

    return status;
}
#endif


fota_status_t fota_download(
    const url_t         *url,
    const fota_flash_t  *flash,
    const uint32_t      partition,
    bool                download_only)
{
    fota_status_t       status = FOTA_STATUS_OK;

    if (! url)
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;

    switch (url->scheme) {
#ifdef MTK_FOTA_V3_TFTP_ENABLE
        case TFTP: {
                ssize_t size = fota_download_by_tftp(url->host, url->port, url->path,
                                                     partition, !download_only,
                                                     flash);
                if (size < 0) {
                    W("fota download %d", size);
                    status = FOTA_STATUS_DOWNLOAD_FAIL;
                }
                break;
            }
#endif
#ifdef MTK_FOTA_V3_HTTP_ENABLE
        case HTTP: {
                ssize_t size = fota_download_by_http(url->host, url->port, url->path,
                                                     partition, !download_only,
                                                     flash);
                if (size < 0) {
                    W("fota download %d", size);
                    status = FOTA_STATUS_DOWNLOAD_FAIL;
                }
                break;
            }
#endif
#ifdef MTK_FOTA_V3_HTTPS_ENABLE
        case HTTPS: {
                ssize_t size = fota_download_by_https(url->host, url->port, url->path,
                                                      partition, !download_only,
                                                      flash);
                if (size < 0) {
                    W("fota download %d", size);
                    status = FOTA_STATUS_DOWNLOAD_FAIL;
                }
                break;
            }
#endif
        default:
            status = FOTA_STATUS_ERROR_PROTOCOL;
            break;
    }

    return status;
}

