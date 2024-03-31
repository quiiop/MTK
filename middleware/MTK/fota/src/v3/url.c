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


#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#include <v3/url.h>
#include "fota_osal.h"


/**
 * Check the octets in the input path string all lies in allowed charset.
 *
 * We do not support the process of URL encoding.
 *
 * Requires caller to provide appropriately encoded URL and do basic check
 * to ensure the characters are in the charset described by RFC 1738,
 * section 2.2: URL Character Encoding Issues.
 *
 * Note: '~' is not added because it is actually used in web servers.
 */
static bool _path_is_valid(const char *str)
{
    char c;
    int i = 0;

    while ((c = str[i++]) != 0)
        if (c <= 0x1F || c >= 0x7F ||  // non-printable
            c == ' '  || c == '<'  || c == '>'  || c == '"'  || // unsafe
            c == '{'  || c == '}'  || c == '|'  || c == '\\' ||
            c == '^'  || c == '['  || c == ']'  || c == '`')
            return false;

    return true;
}


/*
 * for the composition of URL, see: https://en.wikipedia.org/wiki/URL
 *
 * tftp, http, https are supported, this case is not supported:
 *    scheme:path...
 *
 * userinfo not supported.
 *
 * IPv6 not supported.
 */


static size_t _url_extract_scheme(url_t *url, const char *p)
{
    if (strncmp(p, "tftp://", 7) == 0) {
        url->scheme = TFTP;
        url->port   = 69;
        return 7;
    }
    if (strncmp(p, "http://", 7) == 0) {
        url->scheme = HTTP;
        url->port   = 80;
        return 7;
    }
    if (strncmp(p, "https://", 8) == 0) {
        url->scheme = HTTPS;
        url->port   = 443;
        return 8;
    }
    return 0;
}


int url_parse(url_t *url, const char *text)
{
    const char  *p = text;
    char        *colon;
    size_t      size;

    /*
     * check parameters
     */

    if (!url)
        return -1;
    url->host = url->path = NULL;

    if (!text)
        goto url_parse_fail;

    /*
     * scheme
     */

    if ((size = _url_extract_scheme(url, p)) == 0)
        return -1;
    p += size;

    /*
     * host and port
     */

    size = strchr(p, '/') - p;
    if (!size)
        goto url_parse_fail;

    url->host = fota_malloc(size + 1);
    if (!url->host)
        goto url_parse_fail;
    memcpy(url->host, p, size);
    url->host[size] = '\0';
    p += size + 1;

    colon = strchr(url->host, ':');
    if (colon != NULL) {
        *colon = '\0';
        url->port = atoi(colon + 1);
        if (url->port <= 1 || url->port > 65535)
            goto url_parse_fail;
    }

    /* path */

    if (! _path_is_valid(p))
        goto url_parse_fail;

    url->path = fota_malloc(strlen(p) + 1);
    if (!url->path)
        goto url_parse_fail;
    strcpy(url->path, p);

    return 0;

url_parse_fail:

    if (url->host) {
        fota_free(url->host);
        url->host = NULL;
    }

    if (url->path) {
        fota_free(url->path);
        url->path = NULL;
    }

    return -1;
}


void url_free(url_t *url)
{
    if (url) {
        if (url->host) {
            fota_free(url->host);
            url->host = NULL;
        }

        if (url->path) {
            fota_free(url->path);
            url->path = NULL;
        }
    }
}

