/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "memory_map.h"
#include "syslog.h"
#include "gl_init.h"

#ifdef MTK_SECURE_BOOT_ENABLE
#include <scott.h>
#else
#ifdef MTK_WIFI_SIGN_ENABLE
#error "MTK_WIFI_SIGN_ENABLE needs MTK_SECURE_BOOT_ENABLE"
#endif
#endif

#ifdef CONFIG_WIFI_SINGLE_FW
#define WF_SINGLE_BIN_REG_NUM (3)
#define WF_REGION_INFO_SIZE (64)
#define WF_GLO_INFO_SIZE (96)
uint8_t g_fw_region_buf[WF_REGION_INFO_SIZE * WF_SINGLE_BIN_REG_NUM];
uint8_t g_singlefw_flash_ptr[WF_GLO_INFO_SIZE];
/* Align with how bin file is packaged */
enum binType {
    WF_SINGLE_BIN_PATCH,
    WF_SINGLE_BIN_FW,
    WF_SINGLE_BIN_EMI,
};
#endif

static void stripSecureBootWrapper(long *addr, long *size)
{
#ifdef MTK_WIFI_SIGN_ENABLE
    if (!IS_ADDR_IN_FLASH(*addr)) {
        /* convert a flash storage address to a CM33 local bus address.
           strip using CM33 local bus address.
           convert a CM33 local bus address to a flash storage address. */
        *addr = _TO_VFLASH_(*addr);
        (void)scott_image_strip((uint32_t *)addr, (uint32_t *)size);
        *addr = _TO_STORAGE_(*addr);
    } else {
        /* strip using a bus address */
        (void)scott_image_strip((uint32_t *)addr, (uint32_t *)size);
    }
#endif
}

#ifdef CONFIG_WIFI_SINGLE_FW
long getWiFiSingleBinAddr(enum binType eType)
{
    /* uint8_t *flash_ptr = NULL; */
    /* struct PATCH_FORMAT_V2_T *prPatchFormat; */
    /* struct patch_dl_buf *regions = NULL; */
#if (CFG_SUPPORT_FW_BUILDIN)
    long bin_addr = (long)uacFWImage;
#else
    long bin_addr = WIFI_EXT_BASE;
#endif
    long bin_size = WIFI_EXT_LENGTH;
    long ret_addr = 0;
    struct PATCH_GLO_DESC *glo_desc = NULL;
    struct PATCH_SEC_MAP *sec_map = NULL;
    uint32_t num_of_region = 0;
    uint8_t *img_ptr;
    uint8_t i = 0;

    stripSecureBootWrapper(&bin_addr, &bin_size);

    /* read header from flash */
    hal_flash_read(bin_addr, g_singlefw_flash_ptr, WF_GLO_INFO_SIZE);
    img_ptr = g_singlefw_flash_ptr;

    /* single firmware header */
    /* prPatchFormat = (struct PATCH_FORMAT_V2_T *)img_ptr; */

    /* global descriptor */
    img_ptr += sizeof(struct PATCH_FORMAT_V2_T);
    glo_desc = (struct PATCH_GLO_DESC *)img_ptr;
    num_of_region = be2cpu32(glo_desc->section_num);

    if (num_of_region != WF_SINGLE_BIN_REG_NUM) {
        DBGLOG(INIT, STATE, "[%x]region %u != default size %u\n",
               eType, num_of_region, WF_SINGLE_BIN_REG_NUM);
        return ret_addr;
    }

    DBGLOG(INIT, STATE, "Region number: %u\n", num_of_region);
    ret_addr = bin_addr;
    /* section map */
    bin_addr += sizeof(struct PATCH_FORMAT_V2_T) + sizeof(struct PATCH_GLO_DESC);
    hal_flash_read(bin_addr, g_fw_region_buf, WF_REGION_INFO_SIZE * num_of_region);
    img_ptr = g_fw_region_buf;

    for (i = 0; i < num_of_region; i++) {
        sec_map = (struct PATCH_SEC_MAP *)img_ptr;
        img_ptr += sizeof(struct PATCH_SEC_MAP);

        if (eType == i) {
            DBGLOG(INIT, STATE, "Section %d: type = 0x%x, offset = 0x%x, size = 0x%x\n",
                   i,
                   be2cpu32(sec_map->section_type),
                   be2cpu32(sec_map->section_offset),
                   be2cpu32(sec_map->section_size));
            ret_addr += be2cpu32(sec_map->section_offset);
            break;
        }
    }

    DBGLOG(INIT, STATE, "Type: %x, Addr: 0x%x\n", eType, ret_addr);

    return ret_addr;
}
#endif

uint32_t getWifiBaseAddr(void)
{
    long ret = WIFI_BASE;
    long len = WIFI_LENGTH;

#ifdef CONFIG_WIFI_SINGLE_FW
    ret = getWiFiSingleBinAddr(WF_SINGLE_BIN_FW);

    if (ret == 0)
        ret = WIFI_BASE;
#endif

    stripSecureBootWrapper(&ret, &len);

    LOG_D(WIFI, "WIFI BASE = 0x%8x \n", ret);

    return (uint32_t)ret;
}

uint32_t getWifiExtBaseAddr(void)
{
    long ret = WIFI_EXT_BASE;
    long len = WIFI_EXT_LENGTH;

#ifdef CONFIG_WIFI_SINGLE_FW
    ret = getWiFiSingleBinAddr(WF_SINGLE_BIN_EMI);

    if (ret == 0)
        ret = WIFI_EXT_BASE;
#endif

    stripSecureBootWrapper(&ret, &len);

    LOG_D(WIFI, "WIFI EXT BASE = 0x%8x \n", ret);

    return (uint32_t)ret;
}

uint32_t getWifiPatchBaseAddr(void)
{
    long ret = WIFI_PATCH_BASE;
    long len = WIFI_PATCH_LENGTH;

#ifdef CONFIG_WIFI_SINGLE_FW
    ret = getWiFiSingleBinAddr(WF_SINGLE_BIN_PATCH);

    if (ret == 0)
        ret = WIFI_PATCH_BASE;
#endif

    stripSecureBootWrapper(&ret, &len);

    LOG_D(WIFI, "WIFI PATCH BASE = 0x%8x \n", ret);

    return (uint32_t)ret;
}

uint32_t getWifiPwrTblBaseAddr(void)
{
    long ret = WIFI_PWRTBL_BASE;

    LOG_D(WIFI, "WIFI_PWRTBL_BASE = 0x%8x \n", ret);

    return (uint32_t)ret;
}


#if (CFG_BUFFER_BIN_FROM_FLASH == 1)
uint32_t getBufBinAddr(void)
{
    LOG_D(WIFI, "CONN BUF BIN BASE = 0x%8x \n", CONN_BUF_BIN_BASE);
    return (uint32_t)CONN_BUF_BIN_BASE;
}
#endif
