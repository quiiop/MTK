/*******************************************************************************
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
 ******************************************************************************/
/*
 * Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux
 *     /gl_kal.c#10
 */

/*! \file   gl_kal.c
 *    \brief  GLUE Layer will export the required procedures here for internal
 *            driver stack.
 *
 *    This file contains all routines which are exported from GLUE Layer to
 *    internal driver stack.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_os.h"
#include "gl_kal.h"
#include "gl_init.h"
#include "precomp.h"

#include "wifi_event_gen4m.h"

#ifdef MTK_NVDM_ENABLE
#include "nvdm.h"
#include "misc.h"
#include "get_profile_string.h"
#endif /* #ifdef MTK_NVDM_ENABLE */
#if CFG_TC1_FEATURE
#include <tc1_partition.h>
#endif /* #if CFG_TC1_FEATURE */
#include "wifi_netif.h"
#include "lwip/dhcp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* the maximum length of a file name */
#define FILE_NAME_MAX CFG_FW_NAME_MAX_LEN

/* the maximum number of all possible file name */
#define FILE_NAME_TOTAL 8

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if DBG || (CONFIG_WIFI_MEM_DBG == 1)
uint32_t allocatedMemSize = 0;
uint8_t g_AddrAllocatedUsed[256];
uint32_t g_AddrAllocated[256];
uint32_t g_AddrAllocatedSize[256];
uint8_t g_AddrAllocatedi = 0;
#endif /* #if DBG || (CONFIG_WIFI_MEM_DBG == 1) */

#if CFG_PSRAM_ENABLE
ATTR_ZIDATA_IN_RAM
#endif /* #if CFG_PSRAM_ENABLE */
wifi_scan_list_item_t g_ap_list[CFG_MAX_NUM_BSS_LIST] = {{0} };
/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static void *pvIoBuffer;
static uint32_t pvIoBufferSize;
static uint32_t pvIoBufferUsage;
/* framebuffer callback related variable and status flag */
uint8_t wlan_fb_power_down = FALSE;

#if CFG_FORCE_ENABLE_PERF_MONITOR
uint8_t wlan_perf_monitor_force_enable = TRUE;
#else /* #if CFG_FORCE_ENABLE_PERF_MONITOR */
uint8_t wlan_perf_monitor_force_enable = FALSE;
#endif /* #if CFG_FORCE_ENABLE_PERF_MONITOR */

uint8_t g_wifi_on;

#if (CONFIG_WIFI_TEST_TOOL == 1)
/* OID processing table */
/* Order is important here because the OIDs should be in order of
 *  increasing value for binary searching.
 */
static struct WLAN_REQ_ENTRY arWlanOidReqTable[] = {
    /* General Operational Characteristics */
    {
        OID_CUSTOM_MCR_RW,
        (uint8_t *)"OID_CUSTOM_MCR_RW",
        true, true, ENUM_OID_DRIVER_CORE,
        sizeof(uint32_t) * 2,
        /* sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT), */
        (PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryMcrRead,
        (PFN_OID_HANDLER_FUNC_REQ) wlanoidSetMcrWrite
    }
    ,
#if (CFG_SUPPORT_QA_TOOL) || (CONFIG_WIFI_TEST_TOOL == 1)
    {
        OID_CUSTOM_TEST_MODE,
        (uint8_t *)"OID_CUSTOM_TEST_MODE",
        false, false, ENUM_OID_DRIVER_CORE, 0,
        NULL,
        (PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestSetTestMode
    }
    ,
    {
        OID_CUSTOM_ABORT_TEST_MODE,
        (uint8_t *)"OID_CUSTOM_ABORT_TEST_MODE",
        false, false, ENUM_OID_DRIVER_CORE, 0,
        NULL,
        (PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestSetAbortTestMode
    }
    ,
    {
        OID_CUSTOM_MTK_WIFI_TEST,
        (uint8_t *)"OID_CUSTOM_MTK_WIFI_TEST",
        false, false, ENUM_OID_DRIVER_CORE, 0,
        (PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestQueryAutoTest,
        (PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestSetAutoTest
    }
    ,
#endif /* #if (CFG_SUPPORT_QA_TOOL) || (CONFIG_WIFI_TEST_TOOL == 1) */
};
#endif /* #if (CONFIG_WIFI_TEST_TOOL == 1) */
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

#if (CFG_SUPPORT_PRIV_RUN_HQA == 1)
#if (CFG_SUPPORT_QA_TOOL != 1) && (CONFIG_WLAN_SERVICE == 1)
static uint8_t aucOidBuf[CMD_OID_BUF_LENGTH] = { 0 };
#endif /* #if (CFG_SUPPORT_QA_TOOL != 1) && (CONFIG_WLAN_SERVICE == 1) */
#endif /* #if (CFG_SUPPORT_PRIV_RUN_HQA == 1) */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#if CFG_ENABLE_FW_DOWNLOAD
const unsigned char *fw_entry;

#if CFG_ASSERT_DUMP
/* Core dump debug usage */
#if MTK_WCN_HIF_SDIO
uint8_t *apucCorDumpN9FileName =
    "/data/misc/wifi/FW_DUMP_N9";
uint8_t *apucCorDumpCr4FileName =
    "/data/misc/wifi/FW_DUMP_Cr4";
#else /* #if MTK_WCN_HIF_SDIO */
uint8_t *apucCorDumpN9FileName = "/tmp/FW_DUMP_N9";
uint8_t *apucCorDumpCr4FileName = "/tmp/FW_DUMP_Cr4";
#endif /* #if MTK_WCN_HIF_SDIO */
#endif /* #if CFG_ASSERT_DUMP */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        open firmware image in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t kalFirmwareOpen(IN struct GLUE_INFO *prGlueInfo,
                         IN uint8_t **apucNameTable)
{
    return WLAN_STATUS_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        release firmware image in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t kalFirmwareClose(IN struct GLUE_INFO *prGlueInfo)
{
    RELEASE_FIRMWARE(fw_entry);

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        load firmware image in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t kalFirmwareLoad(IN struct GLUE_INFO *prGlueInfo,
                         OUT void *prBuf, IN uint32_t u4Offset,
                         OUT uint32_t *pu4Size)
{
    return WLAN_STATUS_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        query firmware image size in kernel space
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/

uint32_t kalFirmwareSize(IN struct GLUE_INFO *prGlueInfo,
                         OUT uint32_t *pu4Size)
{
    return WLAN_STATUS_SUCCESS;
}

void kalConstructDefaultFirmwarePrio(struct GLUE_INFO
                                     *prGlueInfo, uint8_t **apucNameTable,
                                     uint8_t **apucName, uint8_t *pucNameIdx,
                                     uint8_t ucMaxNameIdx)
{
    struct mt66xx_chip_info *prChipInfo =
            prGlueInfo->prAdapter->chip_info;
    uint32_t chip_id = prChipInfo->chip_id;
    uint8_t sub_idx = 0;

    for (sub_idx = 0; apucNameTable[sub_idx]; sub_idx++) {
        if ((*pucNameIdx + 3) < ucMaxNameIdx) {
            /* Type 1. WIFI_RAM_CODE_MTxxxx */
            if (snprintf((char *) * (apucName + (*pucNameIdx)),
                         FILE_NAME_MAX,
                         "%s%x", apucNameTable[sub_idx], chip_id) < 0)
                DBGLOG(INIT, ERROR, "snprintf fail.\n");
            (*pucNameIdx) += 1;

            /* Type 2. WIFI_RAM_CODE_MTxxxx.bin */
            if (snprintf((char *) * (apucName + (*pucNameIdx)),
                         FILE_NAME_MAX,
                         "%s%x.bin",
                         apucNameTable[sub_idx], chip_id) < 0)
                DBGLOG(INIT, ERROR, "snprintf fail.\n");
            (*pucNameIdx) += 1;

            /* Type 3. WIFI_RAM_CODE_MTxxxx_Ex */
            if (snprintf((char *) * (apucName + (*pucNameIdx)),
                         FILE_NAME_MAX,
                         "%s%x_E%u",
                         apucNameTable[sub_idx], chip_id,
                         wlanGetEcoVersion(prGlueInfo->prAdapter)) < 0)
                DBGLOG(INIT, ERROR, "snprintf fail.\n");
            (*pucNameIdx) += 1;

            /* Type 4. WIFI_RAM_CODE_MTxxxx_Ex.bin */
            if (snprintf((char *) * (apucName + (*pucNameIdx)),
                         FILE_NAME_MAX,
                         "%s%x_E%u.bin",
                         apucNameTable[sub_idx], chip_id,
                         wlanGetEcoVersion(prGlueInfo->prAdapter)) < 0)
                DBGLOG(INIT, ERROR, "snprintf fail.\n");
            (*pucNameIdx) += 1;
        } else {
            /* the table is not large enough */
            DBGLOG(INIT, ERROR,
                   "kalFirmwareImageMapping >> file name array is not enough.\n");
            ASSERT(0);
        }
    }
}


#if CFG_STATIC_MEM_ALLOC
#define FW_FORMAT_V2_SIZE 160
uint8_t g_patch_flash_ptr[FW_FORMAT_V2_SIZE];
uint8_t g_fw_flash_ptr[FW_FORMAT_V2_SIZE];
#endif /* #if CFG_STATIC_MEM_ALLOC */
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to load firmware image
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 * \param ppvMapFileBuf  Pointer of pointer to memory-mapped firmware image
 * \param pu4FileLength  File length and memory mapped length as well
 *
 * \retval Map File Handle, used for unammping
 */
/*----------------------------------------------------------------------------*/
void *kalFirmwareImageMapping(IN struct GLUE_INFO *prGlueInfo,
                              OUT void **ppvMapFileBuf, OUT uint32_t *pu4FileLength,
                              IN enum ENUM_IMG_DL_IDX_T eDlIdx)
{
    uint32_t image_addr = getWifiBaseAddr();
    uint32_t header_size = 0;
    uint8_t *flash_ptr = NULL;
    uint32_t u4DataMode = 0;
    struct patch_dl_buf *region;
    uint32_t num_of_region;
    uint8_t num_fw_section = 3;

    /* Now,we put the FW bin in the array */
    switch (eDlIdx) {
        case IMG_DL_IDX_N9_FW:
            /* fw offset, 0x55934h */
            /* wifi_ext_flash_addr = wifi_ext_flash_addr + 169988; */
            header_size = sizeof(struct PATCH_FORMAT_V2_T) +
                          sizeof(struct PATCH_GLO_DESC) + sizeof(struct PATCH_SEC_MAP);
            image_addr = getWifiBaseAddr();
            DBGLOG(INIT, STATE, "[%d] image from %x\n", eDlIdx, image_addr);

#if CFG_STATIC_MEM_ALLOC
            flash_ptr = g_fw_flash_ptr;
            prGlueInfo->rfw_info.patch_region = (struct patch_dl_buf *)
                                                prGlueInfo->fw_region_buf;
#else /* #if CFG_STATIC_MEM_ALLOC */
            flash_ptr = (uint8_t *)vmalloc((header_size));
            if (!flash_ptr)
                goto mapping_image_err;
#endif /* #if CFG_STATIC_MEM_ALLOC */
#if (CFG_SUPPORT_FW_BUILDIN)
            kalMemCopy(flash_ptr, (uint32_t *)image_addr, header_size);
#else /* #if (CFG_SUPPORT_FW_BUILDIN) */
            hal_flash_read(image_addr, flash_ptr, header_size);
#endif /* #if (CFG_SUPPORT_FW_BUILDIN) */

            wlanImageSectionGetPatchInfoV2(prGlueInfo->prAdapter,
                                           flash_ptr,
                                           header_size,
                                           &u4DataMode,
                                           &prGlueInfo->rfw_info);

#if CFG_STATIC_MEM_ALLOC
#else /* #if CFG_STATIC_MEM_ALLOC */
            vfree(flash_ptr);
#endif /* #if CFG_STATIC_MEM_ALLOC */
            num_of_region = prGlueInfo->rfw_info.num_of_region;
            if (num_of_region <= 0) {
                DBGLOG(INIT, ERROR, "Firmware download num_of_region < 0 !\n");
                goto mapping_image_err;
            }
            /* right now only 1 region */
            region = &prGlueInfo->rfw_info.patch_region[0];
            /* flash ori + offset from head + size - tail info */
            header_size = sizeof(struct TAILER_COMMON_FORMAT_T) +
                          num_fw_section * sizeof(struct TAILER_REGION_FORMAT_T);
            image_addr = (uint32_t)getWifiBaseAddr();
            DBGLOG(INIT, STATE, "[%d] image from %x\r\n", eDlIdx, image_addr);
            image_addr =  image_addr + (region->img_ptr - flash_ptr)
                          + region->img_size - header_size;

            DBGLOG(INIT, STATE, "[%d]header_size %d, image_addr %p flash_ptr %p",
                   eDlIdx, header_size, image_addr, flash_ptr);
            DBGLOG(INIT, STATE, "img_prt %p image_size %d\r\n",
                   region->img_ptr, region->img_size);
            break;

        case IMG_DL_IDX_PATCH:
            header_size = sizeof(struct PATCH_FORMAT_V2_T) +
                          sizeof(struct PATCH_GLO_DESC) + sizeof(struct PATCH_SEC_MAP);
            image_addr = (uint32_t)getWifiPatchBaseAddr();
            DBGLOG(INIT, STATE, "[%d] image from %x\n", eDlIdx, image_addr);
            break;
        default:
            ASSERT(0);
            break;
    }

#if CFG_STATIC_MEM_ALLOC
    switch (eDlIdx) {
        case IMG_DL_IDX_N9_FW:
            flash_ptr =  g_fw_flash_ptr;
            break;
        case IMG_DL_IDX_PATCH:
            flash_ptr =  g_patch_flash_ptr;
            break;
        default:
            flash_ptr = NULL;
    }
#else /* #if CFG_STATIC_MEM_ALLOC */
    flash_ptr = (uint8_t *)vmalloc((header_size));
#endif /* #if CFG_STATIC_MEM_ALLOC */

    if (!flash_ptr)
        goto mapping_image_err;

#if (CFG_SUPPORT_FW_BUILDIN)
    kalMemCopy(flash_ptr, (uint32_t *)image_addr, header_size);
#else /* #if (CFG_SUPPORT_FW_BUILDIN) */
    hal_flash_read(image_addr, flash_ptr, header_size);
#endif /* #if (CFG_SUPPORT_FW_BUILDIN) */

    DBGLOG(INIT, INFO, "[%d]flash_ptr(%d@%08X): %x %x %x %x\r\n",
           eDlIdx, header_size, image_addr,
           flash_ptr[0], flash_ptr[1], flash_ptr[2], flash_ptr[3]);
    *ppvMapFileBuf = flash_ptr;
    *pu4FileLength = header_size;

    return *ppvMapFileBuf;
mapping_image_err:
    DBGLOG(INIT, ERROR, "alloc memory failed for flash access (%d)",
           eDlIdx);
    *pu4FileLength = 0;
    return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to unload firmware image mapped memory
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 * \param pvFwHandle     Pointer to mapping handle
 * \param pvMapFileBuf   Pointer to memory-mapped firmware image
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/

void kalFirmwareImageUnmapping(IN struct GLUE_INFO
                               *prGlueInfo, IN void *prFwHandle, IN void *pvMapFileBuf)
{
}
#endif /* #if CFG_ENABLE_FW_DOWNLOAD */

WLAN_ATTR_TEXT_IN_MEM_TX
struct pkt_buf *alloc_internal_packet(uint16_t que_idx,
                                      int len, int txd_len, void *pbuf)
{
    struct pkt_buf *p = NULL;
    struct pbuf *buf = pbuf;
    int size_cb = sizeof(struct PACKET_PRIVATE_DATA);
    int size = sizeof(*p) + txd_len + sizeof(struct PACKET_PRIVATE_DATA);

    if (size_cb & 0x3)
        size_cb = (size_cb & ~0x3) + 4;

    DBGLOG(TX, INFO, "> alloc %d memory, pkt_len %d pbuf %p\n",
           size, len, pbuf);

    size = sizeof(*p) + txd_len
           + sizeof(struct PACKET_PRIVATE_DATA);

#if CFG_WLAN_LWIP_RXZERO_COPY
    buf = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    if (!buf)
        LOG_FUNC("alloc pbuf failed %d\r\n", len);
#else /* #if CFG_WLAN_LWIP_RXZERO_COPY */
    if (!buf)
        size += (len + GET_INTERNAL_BUF_RESV_LEN() * 2);
#endif /* #if CFG_WLAN_LWIP_RXZERO_COPY */

    p = vmalloc(size);
    if (!p)
        return p;

#if CFG_WLAN_LWIP_ZERO_COPY
    if (!buf && !txd_len)
        len -= NIC_TX_HEAD_ROOM;

    if (buf)
        DBGLOG(TX, INFO, "l_txd %d len %d buf %p txd_buf %p resv %d\n",
               txd_len, len, buf,
               buf->payload, sizeof(struct PACKET_PRIVATE_DATA));
#endif /* #if CFG_WLAN_LWIP_ZERO_COPY */

    if (buf)
        kalMemZero(p, size);
    else
        kalMemZero(p, size - len);
    p->payload_len = len;
    p->txd_len = txd_len;
    p->cb_len = size_cb;
    p->que_idx = que_idx;

#if CFG_WLAN_LWIP_ZERO_COPY
    if (!buf)
        p->txd = (uint8_t *)p + sizeof(*p) + p->cb_len;
    else
        p->txd = buf->payload;
#else /* #if CFG_WLAN_LWIP_ZERO_COPY */
    p->txd = (uint8_t *)p + sizeof(*p) + p->cb_len;
#endif /* #if CFG_WLAN_LWIP_ZERO_COPY */

    p->pbuf = buf;
    if (pbuf) {
        p->payload = (uint8_t *) buf->payload;
#if CFG_WLAN_LWIP_ZERO_COPY
        if (!txd_len)
            p->payload += NIC_TX_HEAD_ROOM;
#endif /* #if CFG_WLAN_LWIP_ZERO_COPY */
    } else
        p->payload = (uint8_t *) ALIGN_X(GET_INTERNAL_BUF_RESV_LEN(),
                                         (uint32_t)((uint8_t *)p + sizeof(*p) + p->cb_len + p->txd_len));

    DBGLOG(TX, INFO, "> pointer check p %p cb %p txd %p payload %p s %u\n",
           p, p->cb, p->txd, p->payload, size);
    return p;
}
#if CFG_STATIC_MEM_ALLOC
void dup_completion(struct completion *from, struct completion *to)
{
    int ret = pdFALSE;

    ret = xSemaphoreTake(from->wait_lock, 0);
    if (ret == pdTRUE) {
        to->wait_lock = from->wait_lock;
        to->wait_complete = from->wait_complete;
        to->done = from->done;
        xSemaphoreGive(to->wait_lock);
    }
}
#endif /* #if CFG_STATIC_MEM_ALLOC */

void init_completion(struct completion *comp)
{
    comp->wait_lock = xSemaphoreCreateBinary();
    comp->wait_complete = xSemaphoreCreateCounting(1, 0);
    comp->done = 0;
    xSemaphoreGive(comp->wait_lock);
}

/*
 * to: time to timeout (ms)
 * assume no reentry
 */
void _wait_for_completion_timeout(struct completion *comp,
                                  uint32_t to, const char *name)
{
    int ret = pdFALSE;
    uint32_t set_to = to / portTICK_PERIOD_MS;

    /* debug */
    /* set_to =  portMAX_DELAY; */

    DBGLOG(INIT, TRACE, "%s call wait_for_completion_timeout \r\n", name);

    ret = xSemaphoreTake(comp->wait_lock, set_to);
    if (ret == pdTRUE) {
        DBGLOG(INIT, TRACE, "%s get wait_done lock\r\n", name);
        comp->done -= 1;
        xSemaphoreGive(comp->wait_lock);
        ret = xSemaphoreTake(comp->wait_complete, set_to);
        if (ret != pdTRUE)
            DBGLOG(INIT, ERROR, "do wait_complete failed %d\n",
                   kalGetTimeTick());
    } else
        DBGLOG(INIT, ERROR, "do wait_done lock failed\n");
}

void _wait_for_completion(struct completion *comp, const char *name)
{
    _wait_for_completion_timeout(comp, portMAX_DELAY, name);
}

/*
 * completion_done - Test to see if a completion has any waiters
 * @x:  completion structure
 *  Return: 0 if there are waiters (wait_for_completion() in progress)
 *          1 if there are no waiters.
 *  Note, this will always return true if complete_all() was called on @X.
 */
int completion_done(struct completion *comp)
{
    int ret = pdFALSE;
    int done = 0;
    uint32_t set_to = PRIV_CLI_CMD_TIMEOUT / portTICK_PERIOD_MS;

    set_to = portMAX_DELAY;

    DBGLOG(INIT, TRACE, "check number of comp->done@%d %d\n",
           kalGetTimeTick(), comp->done);
    ret = xSemaphoreTake(comp->wait_lock, set_to);
    if (ret == pdTRUE) {
        done = comp->done;
        xSemaphoreGive(comp->wait_lock);
        DBGLOG(INIT, TRACE, "check number of wait_completion %d @%d\n",
               done, kalGetTimeTick());
    } else
        DBGLOG(INIT, ERROR, "get done lock failed\n");

    if (done < 0)
        return false;
    else
        return true;
}

void _complete(struct completion *comp, const char *name)
{
    int ret = pdFALSE;
    int done = 0;
    uint32_t set_to = PRIV_CLI_CMD_TIMEOUT / portTICK_PERIOD_MS;

    set_to = portMAX_DELAY;

    DBGLOG(INIT, TRACE, "%s call complete \r\n", name);
    ret = xSemaphoreTake(comp->wait_lock, set_to);
    if (ret == pdTRUE) {
        comp->done += 1;
        done = comp->done;
        xSemaphoreGive(comp->wait_lock);
        DBGLOG(INIT, TRACE, "%s do give complete %d@%d\n",
               name, done, kalGetTimeTick());
        xSemaphoreGive(comp->wait_complete);
    } else
        DBGLOG(INIT, ERROR, "get wait_done lock failed\n");
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        acquire OS SPIN_LOCK.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rLockCategory  Specify which SPIN_LOCK
 * \param[out] pu4Flags      Pointer of a variable for saving IRQ flags
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
ATTR_TEXT_IN_SYSRAM
void kalAcquireSpinLock(IN struct GLUE_INFO *prGlueInfo,
                        IN enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
                        OUT unsigned long *plFlags)
{
    /* DBGLOG(INIT, LOUD, "SPIN_LOCK[%u] Try to acquire as mutex\n", rLockCategory); */
    /* prevent coverity issue */
    if (rLockCategory > 0 && rLockCategory < SPIN_LOCK_NUM) {
        while (!xSemaphoreTake(prGlueInfo->rSpinLock[rLockCategory],
                               (TickType_t) WLAN_WAIT_LOCK_TIME)) {
            DBGLOG(INIT, ERROR,
                   "SPIN_LOCK[%u][0x%p][%d] acquire fail!!!\n",
                   rLockCategory,
                   prGlueInfo->rSpinLock[rLockCategory],
                   uxSemaphoreGetCount(
                       prGlueInfo->rSpinLock[rLockCategory])
                  );
        }
    }
    /* DBGLOG(INIT, LOUD, "SPIN_LOCK[%u] Acquired as mutex\n", rLockCategory); */
}               /* end of kalAcquireSpinLock() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        release OS SPIN_LOCK.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] rLockCategory  Specify which SPIN_LOCK
 * \param[in] u4Flags        Saved IRQ flags
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
ATTR_TEXT_IN_SYSRAM
void kalReleaseSpinLock(IN struct GLUE_INFO *prGlueInfo,
                        IN enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
                        IN unsigned long ulFlags)
{
    ASSERT(prGlueInfo);

    if (rLockCategory > 0 && rLockCategory < SPIN_LOCK_NUM) {
        xSemaphoreGive(prGlueInfo->rSpinLock[rLockCategory]);
        /* DBGLOG(INIT, LOUD, "SPIN_LOCK[%u] release mutex\n", rLockCategory); */
    }
}               /* end of kalReleaseSpinLock() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is provided by GLUE Layer for internal driver stack to
 *        update current MAC address.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pucMacAddr     Pointer of current MAC address
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalUpdateMACAddress(IN struct GLUE_INFO *prGlueInfo,
                         IN uint8_t *pucMacAddr)
{
}

#if CFG_TCP_IP_CHKSUM_OFFLOAD
/*----------------------------------------------------------------------------*/
/*!
 * \brief To query the packet information for offload related parameters.
 *
 * \param[in] pvPacket Pointer to the packet descriptor.
 * \param[in] pucFlag  Points to the offload related parameter.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_TX
void kalQueryTxChksumOffloadParam(IN void *pvPacket,
                                  OUT uint8_t *pucFlag)
{
    uint8_t ucFlag = 0;

    ucFlag |= (TX_CS_IP_GEN | TX_CS_TCP_UDP_GEN);
    *pucFlag = ucFlag;
}               /* kalQueryChksumOffloadParam */

/* 4 2007/10/8, mikewu, this is rewritten by Mike */
/*----------------------------------------------------------------------------*/
/*!
 * \brief To update the checksum offload status to the packet to be indicated to
 *        OS.
 *
 * \param[in] pvPacket Pointer to the packet descriptor.
 * \param[in] pucFlag  Points to the offload related parameter.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalUpdateRxCSUMOffloadParam(IN void *pvPacket,
                                 IN enum ENUM_CSUM_RESULT aeCSUM[])
{
    /* struct pkt_buf *skb = (struct pkt_buf *)pvPacket; */

    ASSERT(pvPacket);

    if ((aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_SUCCESS
         || aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_SUCCESS)
        && ((aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_SUCCESS)
            || (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_SUCCESS))) {
        //skb->ip_summed = CHECKSUM_UNNECESSARY;
    } else {
        //skb->ip_summed = CHECKSUM_NONE;
#if DBG
        if (aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_NONE
            && aeCSUM[CSUM_TYPE_IPV6] == CSUM_RES_NONE)
            DBGLOG(RX, TRACE, "RX: \"non-IPv4/IPv6\" Packet\n");
        else if (aeCSUM[CSUM_TYPE_IPV4] == CSUM_RES_FAILED)
            DBGLOG(RX, TRACE, "RX: \"bad IP Checksum\" Packet\n");
        else if (aeCSUM[CSUM_TYPE_TCP] == CSUM_RES_FAILED)
            DBGLOG(RX, TRACE, "RX: \"bad TCP Checksum\" Packet\n");
        else if (aeCSUM[CSUM_TYPE_UDP] == CSUM_RES_FAILED)
            DBGLOG(RX, TRACE, "RX: \"bad UDP Checksum\" Packet\n");

#endif /* #if DBG */
    }

}               /* kalUpdateRxCSUMOffloadParam */
#endif /* #if CFG_TCP_IP_CHKSUM_OFFLOAD */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is called to free packet allocated from kalPacketAlloc.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of the packet descriptor
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void kalPacketFree(IN struct GLUE_INFO *prGlueInfo,
                   IN void *pvPacket)
{
    struct pkt_buf *buf = pvPacket;

    DBGLOG(RX, INFO, "> free internal packet\n");

    if (buf->pbuf)
        pbuf_free(buf->pbuf);
    buf->pbuf = NULL;
#if (CONFIG_WIFI_MEM_DBG == 1)
    kalMemFree(pvPacket, 0, 2468);
#else /* #if (CONFIG_WIFI_MEM_DBG == 1) */
    vfree(pvPacket);
#endif /* #if (CONFIG_WIFI_MEM_DBG == 1) */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Only handles driver own creating packet (coalescing buffer).
 *
 * \param prGlueInfo   Pointer of GLUE Data Structure
 * \param u4Size       Pointer of Packet Handle
 * \param ppucData     Status Code for OS upper layer
 *
 * \return NULL: Failed to allocate skb, Not NULL get skb
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_RX
void *kalPacketAlloc(IN struct GLUE_INFO *prGlueInfo,
                     IN uint32_t u4Size, OUT uint8_t **ppucData)
{

    struct pkt_buf *prSkb;

    prSkb = alloc_internal_packet(0, u4Size, NIC_TX_HEAD_ROOM, NULL);

    if (prSkb) {
        *ppucData = (uint8_t *)(prSkb->payload);
        kalResetPacket(prGlueInfo, (void *) prSkb);
    } else
        DBGLOG(RX, INFO, "> allocate fail size %d\n", u4Size);
    return (void *) prSkb;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Process the received packet for indicating to OS.
 *
 * \param[in] prGlueInfo     Pointer to the Adapter structure.
 * \param[in] pvPacket       Pointer of the packet descriptor
 * \param[in] pucPacketStart The starting address of the buffer of Rx packet.
 * \param[in] u4PacketLen    The packet length.
 * \param[in] pfgIsRetain    Is the packet to be retained.
 * \param[in] aerCSUM        The result of TCP/ IP checksum offload.
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_FAILURE.
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_RX
uint32_t kalProcessRxPacket(IN struct GLUE_INFO *prGlueInfo,
                            IN void *pvPacket, IN uint8_t *pucPacketStart,
                            IN uint32_t u4PacketLen,
                            /* IN PBOOLEAN           pfgIsRetain, */
                            IN uint8_t fgIsRetain, IN enum ENUM_CSUM_RESULT aerCSUM[])
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;

    struct pkt_buf *skb = (struct pkt_buf *)pvPacket;

    skb->payload = (uint8_t *) pucPacketStart;
    skb->payload_len = u4PacketLen;
    DBGLOG(RX, INFO, "len %d@ 0x%p\n", u4PacketLen, pvPacket);

    return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Process the received forward packet.
 *
 * \param[in] prGlueInfo     Pointer to the Adapter structure.
 * \param[in] pvPacket       Pointer of the packet descriptor
 *
 * \retval WLAN_STATUS_SUCCESS.
 * \retval WLAN_STATUS_RESOURCES.
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_RX
uint32_t kalProcessRxForwardPacket(IN struct GLUE_INFO *prGlueInfo,
                                   IN void *pvPacket)
{
    uint32_t rStatus = WLAN_STATUS_SUCCESS;

    struct pkt_buf *skb = (struct pkt_buf *)pvPacket;

    if (skb->pbuf == NULL) {
        struct pbuf *pbuf = pbuf_alloc(PBUF_RAW, skb->payload_len + skb->txd_len, PBUF_RAM);
        if (pbuf == NULL)
            return WLAN_STATUS_RESOURCES;
        kalMemCopy(pbuf->payload, skb->txd, skb->txd_len);
        kalMemCopy(pbuf->payload + skb->txd_len, skb->payload, skb->payload_len);

        pbuf->len = skb->payload_len + skb->txd_len;
        pbuf->tot_len = skb->payload_len + skb->txd_len;
        skb->pbuf = pbuf;
        skb->payload = pbuf->payload + skb->txd_len;
        skb->txd = pbuf->payload;
    }

    return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief To indicate an array of received packets is available for higher
 *        level protocol uses.
 *
 * \param[in] prGlueInfo Pointer to the Adapter structure.
 * \param[in] apvPkts The packet array to be indicated
 * \param[in] ucPktNum The number of packets to be indicated
 *
 * \retval TRUE Success.
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_RX
uint32_t kalRxIndicatePkts(IN struct GLUE_INFO *prGlueInfo,
                           IN void *apvPkts[], IN uint8_t ucPktNum)
{
    uint8_t ucIdx = 0;

    ASSERT(prGlueInfo);
    ASSERT(apvPkts);

    for (ucIdx = 0; ucIdx < ucPktNum; ucIdx++) {
        if (apvPkts[ucIdx] == NULL)
            continue;
        kalRxIndicateOnePkt(prGlueInfo, apvPkts[ucIdx]);
        apvPkts[ucIdx] = NULL;
    }

    KAL_WAKE_LOCK_TIMEOUT(prGlueInfo->prAdapter,
                          &prGlueInfo->rTimeoutWakeLock, MSEC_TO_JIFFIES(
                              prGlueInfo->prAdapter->rWifiVar.u4WakeLockRxTimeout));

    return WLAN_STATUS_SUCCESS;
}

#if CFG_WLAN_LWIP_RXZERO_COPY
void kalRxHidePbufHdr(struct SW_RFB *prSwRfb)
{
    struct pkt_buf *prSkb = NULL;
    struct pbuf *pbuf = NULL;
    struct HW_MAC_RX_DESC *prRxStatus;
    uint32_t u4HeaderOffset;
    int hdr_pull_len = 0;

    if (!prSwRfb)
        return;

    if (prSwRfb->ucPacketType != RX_PKT_TYPE_RX_DATA)
        return;

    prRxStatus = prSwRfb->prRxStatus;

    u4HeaderOffset = (uint32_t)
                     (HAL_RX_STATUS_GET_HEADER_OFFSET(prRxStatus));

    prSkb = prSwRfb->pvPacket;

    if (!prSkb)
        return;

    pbuf = prSkb->pbuf;

    if (!pbuf)
        return;

    hdr_pull_len = prSwRfb->u2RxStatusOffst + u4HeaderOffset;
    if (pbuf->len == CFG_RX_MAX_PKT_SIZE) {
        pbuf_header(pbuf, hdr_pull_len * -1);
        pbuf_realloc(pbuf, prSwRfb->u2PacketLen);
    } else
        DBGLOG(RX, ERROR, "Rfb %p pkt_buf %p pbuf %p len %d hdr %d\n",
               prSwRfb, prSkb, pbuf,
               pbuf->len, hdr_pull_len);
}
#endif /* #if CFG_WLAN_LWIP_RXZERO_COPY */


/*----------------------------------------------------------------------------*/
/*!
 * \brief To indicate one received packets is available for higher
 *        level protocol uses.
 *
 * \param[in] prGlueInfo Pointer to the Adapter structure.
 * \param[in] pvPkt The packet to be indicated
 *
 * \retval TRUE Success.
 *
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_RX
uint32_t kalRxIndicateOnePkt(IN struct GLUE_INFO
                             *prGlueInfo, IN void *pvPkt)
{
    struct net_device *prNetDev = NULL;
    struct pkt_buf *prSkb = NULL;
    struct pbuf *pbuf = NULL;
    uint8_t ucBssIdx;
#if CFG_PROFILE_PBUF
    struct RX_CTRL *prRxCtrl;
#endif /* #if CFG_PROFILE_PBUF */
    kalSLA_CustomLogging_Start_Label(SLA_LABEL_kalRxIndicateOnePkt);

    ASSERT(prGlueInfo);
    ASSERT(pvPkt);

    prSkb = pvPkt;
    ucBssIdx = GLUE_GET_PKT_BSS_IDX(prSkb);

    prNetDev = wlanGetNetInterfaceByBssIdx(prGlueInfo, ucBssIdx);
    if (!prNetDev)
        prNetDev = prGlueInfo->prDevHandler;
#if CFG_SUPPORT_SNIFFER
    if (prGlueInfo->fgIsEnableMon)
        prNetDev = prGlueInfo->prMonDevHandler;
#endif /* #if CFG_SUPPORT_SNIFFER */

#if CFG_SUPPORT_PERF_IND
    if (GLUE_GET_PKT_BSS_IDX(prSkb) < BSS_DEFAULT_NUM) {
        /* update Performance Indicator statistics*/
        prGlueInfo->PerfIndCache.u4CurRxBytes
        [GLUE_GET_PKT_BSS_IDX(prSkb)] += prSkb->len;
    }
#endif /* #if CFG_SUPPORT_PERF_IND */

#if CFG_WLAN_LWIP_RXZERO_COPY
    pbuf = prSkb->pbuf;
    if (pbuf) {
#if CFG_PROFILE_PBUF
        prRxCtrl = &prGlueInfo->prAdapter->rRxCtrl;
        prSkb->data_processed = kalGetTimeTick();
        prRxCtrl->t_data_processing +=
            prSkb->data_processed - prSkb->data_in;
        prRxCtrl->cnt_data_rx++;
#endif /* #if CFG_PROFILE_PBUF */
        prNetDev->netif_rxcb(prNetDev->netif, pbuf);
    } else
        DBGLOG(RX, ERROR, "pbuf for indicate is NULL from %p!\n",
               prSkb);
    vfree(prSkb);
#else /* #if CFG_WLAN_LWIP_RXZERO_COPY */
    pbuf = pbuf_alloc(PBUF_RAW, prSkb->payload_len, PBUF_RAM);
    if (pbuf == NULL) {
        vfree(prSkb);
        DBGLOG(RX, TRACE, "[WARN]cannot alloc pbuf %d\r\n",
               prSkb->payload_len);
        kalSLA_CustomLogging_Label(SLA_LABEL_kalRxIndicateOnePkt);
        kalSLA_CustomLogging_Stop(SLA_LABEL_kalRxIndicateOnePkt);
        return WLAN_STATUS_RESOURCES;
    }

    DBGLOG(RX, INFO, "pbuf for indicate %p cp from %p with len %d\n",
           pbuf, prSkb, prSkb->payload_len);
    kalMemCopy(pbuf->payload, prSkb->payload, prSkb->payload_len);
    pbuf->len = prSkb->payload_len;
    pbuf->tot_len = prSkb->payload_len;
    if (prNetDev->netif_rxcb(prNetDev->netif, pbuf) != ERR_OK) {
        DBGLOG(RX, INFO, "indicate failed %p\r\n", pbuf);
        pbuf_free(pbuf);
    }

    vfree(prSkb);
#endif /* #if CFG_WLAN_LWIP_RXZERO_COPY */
    kalSLA_CustomLogging_Stop(SLA_LABEL_kalRxIndicateOnePkt);

    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Called by driver to indicate event to upper layer, for example, the
 *        wpa supplicant or wireless tools.
 *
 * \param[in] pvAdapter Pointer to the adapter descriptor.
 * \param[in] eStatus Indicated status.
 * \param[in] pvBuf Indicated message buffer.
 * \param[in] u4BufLen Indicated message buffer size.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalIndicateStatusAndComplete(IN struct GLUE_INFO
                                  *prGlueInfo, IN uint32_t eStatus, IN void *pvBuf,
                                  IN uint32_t u4BufLen, IN uint8_t ucBssIndex)
{
#if !CFG_SUPPORT_NO_SUPPLICANT_OPS
    uint32_t bufLen;
#endif /* #if !CFG_SUPPORT_NO_SUPPLICANT_OPS */
    uint8_t arBssid[PARAM_MAC_ADDR_LEN];
    struct PARAM_SCAN_REQUEST_ADV   *prScanRequest = NULL;
    enum PARAM_POWER_MODE ePwrMode;

    GLUE_SPIN_LOCK_DECLARATION();

    kalMemZero(arBssid, MAC_ADDR_LEN);

    ASSERT(prGlueInfo);

    switch (eStatus) {
        case WLAN_STATUS_ROAM_OUT_FIND_BEST:
        case WLAN_STATUS_MEDIA_CONNECT: {
                /* format:  header:  iwreq (iwreq_data_t) */
                /*          payload: cmd   (uint32_t)     */
                /*          macaddr: maddr (uint8_t[6])   */
                /*          assoc req ie: req_ie (uint8_t[])  */
                /*          assoc resp ie: resp_ie (uint8_t[])*/
#if CFG_SUPPORT_NO_SUPPLICANT_OPS
                netif_set_link_up(prGlueInfo->prDevHandler->netif);
                LOG_FUNC("[wifi] No indicate CONNECT to supp!\n");
#else /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */
                unsigned char pkt[EVENT_PKT_MAX_LEN];
                iwreq_data_t    *iwreq;
                uint32_t        *cmd;
                uint8_t         *bssid;
                uint8_t         *resp_ie;
                uint8_t         *req_ie;
                int             resp_len = 0;
                int             req_len = 0;
                /* event return value */
                int             evt_ret;
                struct CONNECTION_SETTINGS *prConnSettings;

                prConnSettings =
                    aisGetConnSettings(prGlueInfo->prAdapter, ucBssIndex);

                kalMemZero(pkt, sizeof(pkt));
                iwreq = (iwreq_data_t *)&pkt[0];
                cmd   = (uint32_t *)&pkt[sizeof(*iwreq)];
                bssid = (uint8_t *)&pkt[sizeof(*iwreq) + sizeof(uint32_t)];
                req_ie = (uint8_t *)&pkt[EVENT_FIX_LEN];
                resp_ie = req_ie + EVENT_ASSOC_REQ_LEN;
                /* 1. retrieve the associated bssid */
                prGlueInfo->eParamMediaStateIndicated[0] = MEDIA_STATE_CONNECTED;
                wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid,
                                     bssid, MAC_ADDR_LEN, &bufLen);
                if (bufLen < MAC_ADDR_LEN)
                    memset(bssid, 0, MAC_ADDR_LEN);

                /* Association request IE */
                if (prConnSettings == NULL ||
                    prConnSettings->u4ReqIeLength == 0) {
                    DBGLOG(INIT, WARN, "request IE is null");
                } else {
                    const u8 *pos = prConnSettings->aucReqIe;
                    int len, ies_len = prConnSettings->u4ReqIeLength;
                    /* Go through the IEs and parse the MDIE and FTIE, if present. */
                    while (pos && ies_len >= ELEM_HDR_LEN) {
                        len = pos[1] + ELEM_HDR_LEN; /* +2 for tag and tag number */
                        if (len > ies_len) {
                            DBGLOG(INIT, WARN, "truncated assoc req ie");
                            break;
                        }
                        if (pos[0] == WLAN_EID_RSN ||
                            pos[0] == WLAN_EID_RSNX) {
                            if (req_len + len > EVENT_ASSOC_REQ_LEN) {
                                DBGLOG(INIT, WARN, "truncate req IE");
                                break;
                            }
                            kalMemCopy(req_ie + req_len, pos, len);
                            req_len += len;
                        }
                        ies_len -= len;
                        pos += len;
                    }
                }

#if CFG_SUPPORT_802_11R
                /* Association response IE */
                if (prConnSettings == NULL ||
                    prConnSettings->u4RspIeLength == 0) {
                    DBGLOG(INIT, WARN, "response IE is null");
                } else {
                    const u8 *pos = prConnSettings->aucRspIe;
                    int len, ies_len = prConnSettings->u4RspIeLength;
                    /* Go through the IEs and parse the MDIE and FTIE, if present. */
                    while (pos && ies_len >= ELEM_HDR_LEN) {
                        len = pos[1] + ELEM_HDR_LEN; /* +2 for tag and tag number */
                        if (len > ies_len) {
                            DBGLOG(INIT, WARN, "truncated assoc resp ie");
                            break;
                        }
                        if (pos[0] == WLAN_EID_MOBILITY_DOMAIN ||
                            pos[0] == WLAN_EID_FAST_BSS_TRANSITION ||
                            pos[0] == WLAN_EID_RSN ||
                            pos[0] == WLAN_EID_RSNX) {
                            if (resp_len + len > EVENT_ASSOC_RESP_LEN) {
                                DBGLOG(INIT, WARN, "truncate resp IE");
                                break;
                            }
                            kalMemCopy(resp_ie + resp_len, pos, len);
                            resp_len += len;
                        }
                        ies_len -= len;
                        pos += len;
                    }
                }
#endif /* #if CFG_SUPPORT_802_11R */

                DBGLOG(INIT, INFO, "req IE length=%d, resp IE length=%d",
                       req_len, resp_len);

                /* 2. IP stack link UP */
                /* Mark for supplicant trigger DHCP */

                /* 3. Wi-Fi event indication */
                *cmd               = IW_CUSTOM_EVENT_FLAG;
                iwreq->data.flags  = IW_ASSOC_EVENT_FLAG;
                if (req_len == 0 && resp_len == 0)
                    iwreq->data.length = MAC_ADDR_LEN;
                else /* length = bssid + req IE + resp IE */
                    iwreq->data.length = EVENT_DATA_MAX_LEN;
                evt_ret = wifi_evt_handler_gen4m(0, pkt, sizeof(pkt));

                /* TODO: if not handled, what should we do? */
                if (evt_ret)
                    LOG_FUNC("[wifi] assoc done not handled!\n");
#if (CFG_SUPPORT_802_11AX == 1)
                if (IS_FEATURE_ENABLED(
                        prGlueInfo->prAdapter->rWifiVar.fgEnableSR))
                    rlmSetSrControl(prGlueInfo->prAdapter, TRUE);
#endif /* #if (CFG_SUPPORT_802_11AX == 1) */
#endif /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */
                if (IS_FEATURE_ENABLED(
                        prGlueInfo->prAdapter->rWifiVar.fgEnableDefaultFastPSP)) {
                    ePwrMode = Param_PowerModeFast_PSP;
                    DBGLOG(INIT, INFO, "Connected! set to FastPSP mode!\n");
                    if (prGlueInfo->prAdapter)
                        nicConfigPowerSaveProfile(
                            prGlueInfo->prAdapter,
                            ucBssIndex, ePwrMode,
                            FALSE,
                            PS_CALLER_COMMON);
                }
                break;
            }

        case WLAN_STATUS_MEDIA_DISCONNECT:
        case WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY: {
#if CFG_SUPPORT_NO_SUPPLICANT_OPS
#else /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */
                /* format:  header:  iwreq (iwreq_data_t) */
                /*          payload: cmd   (uint32_t)     */
                /*          macaddr: maddr (uint8_t[6])   */
                unsigned char pkt[EVENT_FIX_LEN];
                iwreq_data_t    *iwreq;
                uint32_t        *cmd;
                uint8_t         *bssid;

                /* event return value */
                int             evt_ret;
#if (CFG_SUPPORT_WPA3 == 1)
                struct CONNECTION_SETTINGS *prConnSettings;

                prConnSettings =
                    aisGetConnSettings(prGlueInfo->prAdapter, ucBssIndex);
                if (prConnSettings && prConnSettings->assocIeLen > 0) {
                    kalMemZero(prConnSettings->pucAssocIEs, 200);
                    prConnSettings->assocIeLen = 0;
                }
#endif /* #if (CFG_SUPPORT_WPA3 == 1) */

                iwreq = (iwreq_data_t *)&pkt[0];
                cmd   = (uint32_t *)&pkt[sizeof(*iwreq)];
                bssid = (uint8_t *)&pkt[sizeof(*iwreq) + sizeof(uint32_t)];

                /* 1. retrieve the associated bssid */
                wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid,
                                     bssid, MAC_ADDR_LEN, &bufLen);
                if (bufLen < MAC_ADDR_LEN)
                    memset(bssid, 0, MAC_ADDR_LEN);
#endif /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */

                LOG_FUNC("[wifi] WLAN_STATUS_MEDIA_DISCONNECT\n");
                if (prGlueInfo->fgIsRegistered == TRUE) {
                    struct BSS_INFO *prBssInfo = (struct BSS_INFO *)
                                                 prGlueInfo->prAdapter->prAisBssInfo;
                    uint16_t u2DeauthReason = 0;

                    if (prBssInfo)
                        u2DeauthReason = prBssInfo->u2DeauthReason;
                    /* CFG80211 Indication */
                    DBGLOG(INIT, INFO,
                           "[wifi]Indicate disconnection: Locally[%d] %x\n",
                           (eStatus ==
                            WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY),
                           u2DeauthReason);
                    netif_set_link_down(prGlueInfo->prDevHandler->netif);
                }
                prGlueInfo->eParamMediaStateIndicated[0] = MEDIA_STATE_DISCONNECTED;
#if CFG_SUPPORT_NO_SUPPLICANT_OPS
                LOG_FUNC("[wifi] No indicate DISCONNECT to supp!\n");
#else /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */
                *cmd               = IW_CUSTOM_EVENT_FLAG;
                if (eStatus == WLAN_STATUS_MEDIA_DISCONNECT) {
                    iwreq->data.flags  = IW_DISASSOC_EVENT_FLAG;
#if CFG_SUPPORT_FAST_CONNECT
                    if (IS_FEATURE_ENABLED(
                            prGlueInfo->prAdapter->rWifiVar.fgEnableFastConnect)) {
                        prGlueInfo->prAdapter->rWifiVar.ucFastConnect = 1;
                        DBGLOG(INIT, ERROR, "[wifi] Support fast connect!\n");
                    } else {
                        prGlueInfo->prAdapter->rWifiVar.ucFastConnect = 0;
                    }
#endif /* #if CFG_SUPPORT_FAST_CONNECT */
                } else {
                    iwreq->data.flags  = IW_DISASSOC_LOCALLY_EVENT_FLAG;
#if CFG_SUPPORT_FAST_CONNECT
                    prGlueInfo->prAdapter->rWifiVar.ucFastConnect = 0;
#endif /* #if CFG_SUPPORT_FAST_CONNECT */
                }
                iwreq->data.length = MAC_ADDR_LEN; /* bssid */

                evt_ret = wifi_evt_handler_gen4m(0, pkt, sizeof(pkt));

                /* TODO: if not handled, what should we do? */
                if (evt_ret)
                    LOG_FUNC("[wifi] disconnect  done not handled!\n");
#endif /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */
                if (IS_FEATURE_ENABLED(
                        prGlueInfo->prAdapter->rWifiVar.fgEnableDefaultFastPSP)) {
                    ePwrMode = Param_PowerModeCAM;
                    DBGLOG(INIT, INFO, "Disconnected! set to CAM mode!\n");
                    if (prGlueInfo->prAdapter)
                        nicConfigPowerSaveProfile(
                            prGlueInfo->prAdapter,
                            ucBssIndex, ePwrMode,
                            FALSE,
                            PS_CALLER_COMMON);
                }
                break;
            }
        case WLAN_STATUS_SCAN_COMPLETE: {
#if CFG_SUPPORT_NO_SUPPLICANT_OPS
#else /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */
                /* format:  header:  iwreq (iwreq_data_t) */
                /*          payload: cmd   (uint32_t)     */
                unsigned char   pkt[sizeof(iwreq_data_t) + sizeof(uint32_t)];
                iwreq_data_t    *iwreq;
                uint32_t        *cmd;
                /* event return value */
                int             evt_ret;
#endif /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */

                /* 1. reset the completed scan request */

                GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

                if (prGlueInfo->prScanRequest != NULL) {
                    prScanRequest = prGlueInfo->prScanRequest;
                    prGlueInfo->prScanRequest = NULL;
                }

                GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

                if (prScanRequest)
                    vfree(prScanRequest);
#if CFG_SUPPORT_NO_SUPPLICANT_OPS
                LOG_FUNC("[wifi] No indicate SCAN done to supp!\n");
#else /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */
                /* 2. Wi-Fi event indication */

                iwreq                 = (iwreq_data_t *)&pkt[0];
                iwreq->data.flags     = IW_SCAN_COMPLETED_EVENT_FLAG;
                cmd                   = (uint32_t *)&pkt[sizeof(*iwreq)];
                *cmd                  = IW_CUSTOM_EVENT_FLAG;

                evt_ret = wifi_evt_handler_gen4m(0, pkt, sizeof(pkt));

                if (evt_ret)
                    LOG_FUNC("[wifi] scan done not handled!\n");
#endif /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */
                break;
            }

        case WLAN_STATUS_MEDIA_SPECIFIC_INDICATION: {
#if CFG_SUPPORT_NO_SUPPLICANT_OPS
#else /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */
#if CFG_SUPPORT_802_11R
                /* format:  header:  iwreq (iwreq_data_t) */
                /*          payload: cmd   (uint32_t)     */
                /*          target AP macaddr: maddr (uint8_t[6])   */
                /*          FT IE: ftie (uint8_t[257])  */
                unsigned char pkt[EVENT_FIX_LEN + EVENT_FTIE_LEN]; /* for AP macaddr and FTIE */
                iwreq_data_t    *iwreq;
                uint32_t        *cmd;
                uint8_t         *maddr;
                uint8_t         *ftie;
                int             ftie_len = 0;
                /* event return value */
                int             evt_ret;
                struct cfg80211_ft_event_params *prFtEvent =
                    aisGetFtEventParam(prGlueInfo->prAdapter, ucBssIndex);

                kalMemZero(pkt, sizeof(pkt));
                iwreq = (iwreq_data_t *)&pkt[0];
                cmd   = (uint32_t *)&pkt[sizeof(*iwreq)];
                maddr = (uint8_t *)&pkt[sizeof(*iwreq) + sizeof(uint32_t)];
                ftie  = (uint8_t *)&pkt[EVENT_FIX_LEN];

                /* 1. copy target AP mac addr */
                if (prFtEvent->target_ap)
                    kalMemCopy(maddr, prFtEvent->target_ap, MAC_ADDR_LEN);
                else
                    memset(maddr, 0, MAC_ADDR_LEN);

                /* 2. copy ftie */
                if (prFtEvent->ies && prFtEvent->ies_len) {
                    if (prFtEvent->ies_len <= EVENT_FTIE_LEN) {
                        kalMemCopy(ftie, prFtEvent->ies, prFtEvent->ies_len);
                        ftie_len = prFtEvent->ies_len;
                    } else {
                        memset(ftie, 0, EVENT_FTIE_LEN);
                        DBGLOG(INIT, WARN, "FTIE is not found");
                    }
                }
                DBGLOG(INIT, INFO, "FTIE length=%d", ftie_len);

                /* 3. Wi-Fi event indication */
                *cmd               = IW_CUSTOM_EVENT_FLAG;
                iwreq->data.flags  = IW_FT_INDICATION_EVENT_FLAG;
                iwreq->data.length = MAC_ADDR_LEN + ftie_len; /* bssid + FT IE(11R) */

                evt_ret = wifi_evt_handler_gen4m(0, pkt, sizeof(pkt));

                /* TODO: if not handled, what should we do? */
                if (evt_ret)
                    LOG_FUNC("[wifi] media specific indication not handled!\n");
#endif /* #if CFG_SUPPORT_802_11R */
#endif /* #if CFG_SUPPORT_NO_SUPPLICANT_OPS */
                break;
            }

#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS
        case WLAN_STATUS_BWCS_UPDATE:
            LOG_FUNC("[wifi] WLAN_STATUS_BWCS_UPDATE not support yet\n");
            break;
#endif /* #if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS */
        case WLAN_STATUS_JOIN_FAILURE:
        default:
            LOG_FUNC("[wifi] unknown indication:%lx\n", eStatus);
            break;
    }
}               /* kalIndicateStatusAndComplete */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to update the (re)association request
 *        information to the structure used to query and set
 *        OID_802_11_ASSOCIATION_INFORMATION.
 *
 * \param[in] prGlueInfo Pointer to the Glue structure.
 * \param[in] pucFrameBody Pointer to the frame body of the last
 *                         (Re)Association Request frame from the AP.
 * \param[in] u4FrameBodyLen The length of the frame body of the last
 *                           (Re)Association Request frame.
 * \param[in] fgReassocRequest TRUE, if it is a Reassociation Request frame.
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
void kalUpdateReAssocReqInfo(IN struct GLUE_INFO *prGlueInfo,
                             IN uint8_t *pucFrameBody, IN uint32_t u4FrameBodyLen,
                             IN uint8_t fgReassocRequest,
                             IN uint8_t ucBssIndex)
{
    uint8_t *cp;
    struct CONNECTION_SETTINGS *prConnSettings = NULL;

    ASSERT(prGlueInfo);

    prConnSettings = aisGetConnSettings(
                         prGlueInfo->prAdapter,
                         ucBssIndex);

    /* reset */
    prConnSettings->u4ReqIeLength = 0;

    if (fgReassocRequest) {
        if (u4FrameBodyLen < 15) {
            /*
             * printk(KERN_WARNING "frameBodyLen too short:%d\n",
             * frameBodyLen);
             */
            return;
        }
    } else {
        if (u4FrameBodyLen < 9) {
            /*
             * printk(KERN_WARNING "frameBodyLen too short:%d\n",
             * frameBodyLen);
             */
            return;
        }
    }

    cp = pucFrameBody;

    if (fgReassocRequest) {
        /* Capability information field 2 */
        /* Listen interval field 2 */
        /* Current AP address 6 */
        cp += 10;
        u4FrameBodyLen -= 10;
    } else {
        /* Capability information field 2 */
        /* Listen interval field 2 */
        cp += 4;
        u4FrameBodyLen -= 4;
    }

    if (u4FrameBodyLen <= CFG_CFG80211_IE_BUF_LEN) {
        prConnSettings->u4ReqIeLength = u4FrameBodyLen;
        kalMemCopy(prConnSettings->aucReqIe, cp, u4FrameBodyLen);
    }

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called to update the (re)association
 *        response information to the structure used to reply with
 *        cfg80211_connect_result
 *
 * @param prGlueInfo      Pointer to adapter descriptor
 * @param pucFrameBody    Pointer to the frame body of the last (Re)Association
 *                         Response frame from the AP
 * @param u4FrameBodyLen  The length of the frame body of the last
 *                          (Re)Association Response frame
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void kalUpdateReAssocRspInfo(IN struct GLUE_INFO
                             *prGlueInfo, IN uint8_t *pucFrameBody,
                             IN uint32_t u4FrameBodyLen,
                             IN uint8_t ucBssIndex)
{
    uint32_t u4IEOffset =
        6;  /* cap_info, status_code & assoc_id */
    uint32_t u4IELength = u4FrameBodyLen - u4IEOffset;
    struct CONNECTION_SETTINGS *prConnSettings = NULL;

    ASSERT(prGlueInfo);

    prConnSettings = aisGetConnSettings(
                         prGlueInfo->prAdapter,
                         ucBssIndex);

    /* reset */
    prConnSettings->u4RspIeLength = 0;

    if (u4IELength <= CFG_CFG80211_IE_BUF_LEN) {
        prConnSettings->u4RspIeLength = u4IELength;
        kalMemCopy(prConnSettings->aucRspIe, pucFrameBody + u4IEOffset,
                   u4IELength);
    }

}               /* kalUpdateReAssocRspInfo */

WLAN_ATTR_TEXT_IN_MEM_TX
void kalResetPacket(IN struct GLUE_INFO *prGlueInfo,
                    IN void *prPacket)
{
    struct pkt_buf *prSkb = (struct pkt_buf *)prPacket;

    /* Reset cb */
    kalMemZero(prSkb->cb, sizeof(prSkb->cb));
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is to check the pairwise eapol and wapi 1x.
 *
 * \param[in] prPacket  Pointer to struct net_device
 *
 * \retval WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint8_t kalIsPairwiseEapolPacket(IN void *prPacket)
{
    DBGLOG(RX, ERROR, "kalIsPairwiseEapolPacket no reset sk_buff in rtos reference lwip fixme\n");
    return FALSE;
}

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is TX entry point of NET DEVICE.
 *
 * \param[in] prSkb  Pointer of the sk_buff to be sent
 * \param[in] prDev  Pointer to struct net_device
 * \param[in] prGlueInfo  Pointer of prGlueInfo
 * \param[in] ucBssIndex  BSS index of this net device
 *
 * \retval WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_TX
uint32_t kalHardStartXmit(struct pkt_buf *prOrgSkb,
                          IN struct netif *prDev, struct GLUE_INFO *prGlueInfo,
                          uint8_t ucBssIndex)
{
    uint16_t u2QueueIdx = 0;
    struct pkt_buf *prSkb = NULL;
    struct ADAPTER *prAdapter = NULL;

    ASSERT(prOrgSkb);
    ASSERT(prGlueInfo);

    prAdapter = prGlueInfo->prAdapter;
    if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
        DBGLOG(TX, INFO, "GLUE_FLAG_HALT skip tx\n");
        return WLAN_STATUS_ADAPTER_NOT_READY;
    }

    if (prAdapter->fgIsEnableLpdvt) {
        DBGLOG(TX, INFO, "LPDVT enable, skip this frame\n");
        return WLAN_STATUS_NOT_ACCEPTED;
    }

    prSkb = prOrgSkb;
    GLUE_SET_PKT_BSS_IDX(prSkb, ucBssIndex);

    /* Parsing frame info */
    kalSLA_CustomLogging_Start(SLA_LABEL_wlanProcessTxFrame);
    if (!wlanProcessTxFrame(prAdapter,
                            (void *) prSkb)) {
        /* Cannot extract packet */
        DBGLOG(TX, INFO,
               "Cannot extract content, skip this frame\n");
        return WLAN_STATUS_INVALID_PACKET;
    }
    kalSLA_CustomLogging_Stop(SLA_LABEL_wlanProcessTxFrame);

#if CFG_MET_PACKET_TRACE_SUPPORT
    /* Tx profiling */
    wlanTxProfilingTagPacket(prAdapter,
                             (void *) prSkb, TX_PROF_TAG_OS_TO_DRV);
#endif /* #if CFG_MET_PACKET_TRACE_SUPPORT */

    /* Handle normal data frame */
    u2QueueIdx =  prSkb->que_idx;

    if (u2QueueIdx >= CFG_MAX_TXQ_NUM) {
        DBGLOG(TX, INFO,
               "Incorrect queue index, skip this frame\n");
        return WLAN_STATUS_INVALID_PACKET;
    }

    GLUE_INC_REF_CNT(prGlueInfo->i4TxPendingFrameNum);
    GLUE_INC_REF_CNT(
        prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
        [u2QueueIdx]);

    if (HAL_IS_TX_DIRECT(prAdapter))
        return nicTxDirectStartXmit(prSkb, prGlueInfo);

    return WLAN_STATUS_SUCCESS;
}               /* end of kalHardStartXmit() */

uint32_t kalResetStats(IN struct net_device *prDev)
{
    return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, to get the network interface
 *        statistical information.
 *
 * Whenever an application needs to get statistics for the interface, this
 * method is called. This happens, for example, when ifconfig or netstat -i is
 * run.
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \return net_device_stats buffer pointer.
 */
/*----------------------------------------------------------------------------*/
void *kalGetStats(IN struct net_device *prDev)
{
    return NULL;
}               /* end of wlanGetStats() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Notify OS with SendComplete event of the specific packet. Linux should
 *        free packets here.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 * \param[in] status         Status Code for OS upper layer
 *
 * \return -
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_TX
void kalSendCompleteAndAwakeQueue(IN struct GLUE_INFO
                                  *prGlueInfo, IN void *pvPacket)
{
    struct pkt_buf *prSkb = NULL;
    uint16_t u2QueueIdx = 0;
    uint8_t ucBssIndex = 0;

    ASSERT(pvPacket);
    /* ASSERT(prGlueInfo->i4TxPendingFrameNum); */

    prSkb = (struct pkt_buf *)pvPacket;
    u2QueueIdx = prSkb->que_idx;
    ASSERT(u2QueueIdx < CFG_MAX_TXQ_NUM);

    ucBssIndex = GLUE_GET_PKT_BSS_IDX(pvPacket);

    GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingFrameNum);
    GLUE_DEC_REF_CNT(
        prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
        [u2QueueIdx]);

    DBGLOG(TX, LOUD,
           "Release frame for BSS[%u] QIDX[%u] PKT_LEN[%u] TOT_CNT[%d] PER-Q_CNT[%d]\n",
           ucBssIndex, u2QueueIdx, prSkb->payload_len,
           GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum),
           GLUE_GET_REF_CNT(
               prGlueInfo->ai4TxPendingFrameNumPerQueue[ucBssIndex]
               [u2QueueIdx]));

    if (prSkb->pbuf)
        pbuf_free(prSkb->pbuf);
    vfree(pvPacket);

    DBGLOG(TX, LOUD, "----- pending frame %d -----\n",
           prGlueInfo->i4TxPendingFrameNum);
}

WLAN_ATTR_TEXT_IN_MEM_TX
uint8_t kalIPv4FrameClassifier(IN struct GLUE_INFO *prGlueInfo,
                               IN void *prPacket, IN uint8_t *pucIpHdr,
                               OUT struct TX_PACKET_INFO *prTxPktInfo)
{
    uint8_t ucIpVersion, ucIcmpType;
    uint8_t ucIpProto;
    uint8_t ucSeqNo;
    uint8_t *pucUdpHdr, *pucIcmp;
    uint16_t u2DstPort, u2IcmpId, u2IcmpSeq;
    struct BOOTP_PROTOCOL *prBootp;
    uint32_t u4DhcpMagicCode;
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
    struct ADAPTER *prAdapter = NULL;
#endif /* #if (CFG_TCP_IP_CHKSUM_OFFLOAD) */
#endif /* #if CFG_SUPPORT_WIFI_SYSDVT */

    /* IPv4 version check */
    ucIpVersion = (pucIpHdr[0] & IP_VERSION_MASK) >>
                  IP_VERSION_OFFSET;
    if (ucIpVersion != IP_VERSION_4) {
        DBGLOG(TX, WARN, "Invalid IPv4 packet version: %u\n",
               ucIpVersion);
        return FALSE;
    }

    ucIpProto = pucIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET];

#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
    prAdapter = prGlueInfo->prAdapter;

    /* set IP CHECKSUM to 0xff for verify CSO function */
    if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_IP
        && CSO_TX_IPV4_ENABLED(prAdapter)) {
        pucIpHdr[IPV4_HDR_IP_CSUM_OFFSET] = 0xff;
        pucIpHdr[IPV4_HDR_IP_CSUM_OFFSET + 1] = 0xff;
    }
#endif /* #if (CFG_TCP_IP_CHKSUM_OFFLOAD) */
#endif /* #if CFG_SUPPORT_WIFI_SYSDVT */

    if (ucIpProto == IP_PRO_UDP) {
        pucUdpHdr = &pucIpHdr[IPV4_HDR_LEN];

#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
        /* set UDP CHECKSUM to 0xff for verify CSO function */
        if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_UDP
            && CSO_TX_UDP_ENABLED(prAdapter)) {
            pucUdpHdr[UDP_HDR_UDP_CSUM_OFFSET] = 0xff;
            pucUdpHdr[UDP_HDR_UDP_CSUM_OFFSET + 1] = 0xff;
        }
#endif /* #if (CFG_TCP_IP_CHKSUM_OFFLOAD) */
#endif /* #if CFG_SUPPORT_WIFI_SYSDVT */

        /* Get UDP DST port */
        WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_DST_PORT_OFFSET],
                            &u2DstPort);

        /* BOOTP/DHCP protocol */
        if ((u2DstPort == IP_PORT_BOOTP_SERVER) ||
            (u2DstPort == IP_PORT_BOOTP_CLIENT)) {

            prBootp = (struct BOOTP_PROTOCOL *)
                      &pucUdpHdr[UDP_HDR_LEN];

            WLAN_GET_FIELD_BE32(&prBootp->aucOptions[0],
                                &u4DhcpMagicCode);

            if (u4DhcpMagicCode == DHCP_MAGIC_NUMBER) {
                uint32_t u4Xid;

                WLAN_GET_FIELD_BE32(&prBootp->u4TransId,
                                    &u4Xid);

                ucSeqNo = nicIncreaseTxSeqNum(
                              prGlueInfo->prAdapter);
                GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);

                DBGLOG_LIMITED(TX, INFO,
                               "DHCP PKT[0x%p] XID[0x%08x] OPT[%u] TYPE[%u], SeqNo: %d\n",
                               prPacket, u4Xid, prBootp->aucOptions[4],
                               prBootp->aucOptions[6], ucSeqNo);
                prTxPktInfo->u2Flag |= BIT(ENUM_PKT_DHCP);
            }
        } else if (u2DstPort == UDP_PORT_DNS) {
            uint16_t u2IpId = *(uint16_t *)&pucIpHdr[IPV4_ADDR_LEN];
            uint8_t *pucUdpPayload = &pucUdpHdr[UDP_HDR_LEN];
            uint16_t u2TransId = (pucUdpPayload[0] << 8) |
                                 pucUdpPayload[1];

            ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
            GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
            DBGLOG_LIMITED(TX, INFO,
                           "<TX> DNS: [0x%p] IPID[0x%02x] TransID[0x%04x] SeqNo[%d]\n",
                           prPacket, u2IpId, u2TransId, ucSeqNo);
            prTxPktInfo->u2Flag |= BIT(ENUM_PKT_DNS);
        }
    } else if (ucIpProto == IP_PRO_ICMP) {
        /* the number of ICMP packets is seldom so we print log here */
        uint16_t u2IpId =
            (pucIpHdr[IPV4_ADDR_LEN] << 8) |
            pucIpHdr[IPV4_ADDR_LEN + 1];
        pucIcmp = &pucIpHdr[20];

        ucIcmpType = pucIcmp[0];
        if (ucIcmpType ==
            3) /* don't log network unreachable packet */
            return FALSE;
        u2IcmpId = *(uint16_t *) &pucIcmp[4];
        u2IcmpSeq = *(uint16_t *) &pucIcmp[6];

        ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
        GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
        DBGLOG_LIMITED(TX, INFO,
                       "<TX> ICMP: IPID[0x%04x] Type %d, Id 0x%04x, Seq BE 0x%04x, SeqNo: %d\n",
                       u2IpId, ucIcmpType, u2IcmpId, u2IcmpSeq, ucSeqNo);
        prTxPktInfo->u2Flag |= BIT(ENUM_PKT_ICMP);
    }
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
    else if (ucIpProto == IP_PRO_TCP) {
        uint8_t *pucTcpHdr;

        pucTcpHdr = &pucIpHdr[IPV4_HDR_LEN];
        /* set TCP CHECKSUM to 0xff for verify CSO function */
        if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_TCP
            && CSO_TX_TCP_ENABLED(prAdapter)) {
            pucTcpHdr[TCP_HDR_TCP_CSUM_OFFSET] = 0xff;
            pucTcpHdr[TCP_HDR_TCP_CSUM_OFFSET + 1] = 0xff;
        }
    }
#endif /* #if (CFG_TCP_IP_CHKSUM_OFFLOAD) */
#endif /* #if CFG_SUPPORT_WIFI_SYSDVT */

    return TRUE;
}

uint8_t kalIPv6FrameClassifier(IN struct GLUE_INFO *prGlueInfo,
                               IN void *prPacket, IN uint8_t *pucIpv6Hdr,
                               OUT struct TX_PACKET_INFO *prTxPktInfo)
{
    uint8_t ucIpv6Proto;
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
    struct ADAPTER *prAdapter = NULL;
    uint8_t *pucL3Hdr;

    prAdapter = prGlueInfo->prAdapter;
#endif /* #if (CFG_TCP_IP_CHKSUM_OFFLOAD) */
#endif /* #if CFG_SUPPORT_WIFI_SYSDVT */

    ucIpv6Proto = pucIpv6Hdr[IPV6_HDR_IP_PROTOCOL_OFFSET];

    if (ucIpv6Proto == IP_PRO_UDP) {
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
        pucL3Hdr = &pucIpv6Hdr[IPV6_HDR_LEN];
        /* set UDP CHECKSUM to 0xff for verify CSO function */
        if ((prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_UDP)
            && CSO_TX_UDP_ENABLED(prAdapter)) {
            pucL3Hdr[UDP_HDR_UDP_CSUM_OFFSET] = 0xff;
            pucL3Hdr[UDP_HDR_UDP_CSUM_OFFSET + 1] = 0xff;
        }
#endif /* #if (CFG_TCP_IP_CHKSUM_OFFLOAD) */
#endif /* #if CFG_SUPPORT_WIFI_SYSDVT */
    } else if (ucIpv6Proto == IP_PRO_TCP) {
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
        pucL3Hdr = &pucIpv6Hdr[IPV6_HDR_LEN];
        /* set TCP CHECKSUM to 0xff for verify CSO function */
        if ((prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_TCP)
            && CSO_TX_TCP_ENABLED(prAdapter)) {
            pucL3Hdr[TCP_HDR_TCP_CSUM_OFFSET] = 0xff;
            pucL3Hdr[TCP_HDR_TCP_CSUM_OFFSET + 1] = 0xff;
        }
#endif /* #if (CFG_TCP_IP_CHKSUM_OFFLOAD) */
#endif /* #if CFG_SUPPORT_WIFI_SYSDVT */
    }

    return TRUE;
}

uint8_t kalArpFrameClassifier(IN struct GLUE_INFO *prGlueInfo,
                              IN void *prPacket, IN uint8_t *pucIpHdr,
                              OUT struct TX_PACKET_INFO *prTxPktInfo)
{
    uint16_t u2ArpOp;
    uint8_t ucSeqNo;

    ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
    WLAN_GET_FIELD_BE16(&pucIpHdr[ARP_OPERATION_OFFSET],
                        &u2ArpOp);

    DBGLOG_LIMITED(TX, INFO,
                   "ARP %s PKT[0x%p] TAR MAC/IP["
                   MACSTR "]/[" IPV4STR "], SeqNo: %d\n",
                   u2ArpOp == ARP_OPERATION_REQUEST ? "REQ" : "RSP",
                   prPacket, MAC2STR(&pucIpHdr[ARP_TARGET_MAC_OFFSET]),
                   IPV4TOSTR(&pucIpHdr[ARP_TARGET_IP_OFFSET]), ucSeqNo);

    GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);

    prTxPktInfo->u2Flag |= BIT(ENUM_PKT_ARP);
    return TRUE;
}

uint8_t kalTdlsFrameClassifier(IN struct GLUE_INFO *prGlueInfo,
                               IN void *prPacket, IN uint8_t *pucIpHdr,
                               OUT struct TX_PACKET_INFO *prTxPktInfo)
{
    uint8_t ucSeqNo;
    uint8_t ucActionCode;

    ucActionCode = pucIpHdr[TDLS_ACTION_CODE_OFFSET];

    DBGLOG(TX, INFO, "TDLS action code: %d\n", ucActionCode);

    ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);

    GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);

    prTxPktInfo->u2Flag |= BIT(ENUM_PKT_TDLS);

    return TRUE;
}

uint8_t kalSecurityFrameClassifier(IN struct GLUE_INFO *prGlueInfo,
                                   IN void *prPacket, IN uint8_t *pucIpHdr,
                                   IN uint16_t u2EthType, IN uint8_t *aucLookAheadBuf,
                                   OUT struct TX_PACKET_INFO *prTxPktInfo)
{
    uint8_t *pucEapol;
    uint8_t ucEapolType;
    uint8_t ucSeqNo;
#if CFG_SUPPORT_WAPI
    uint8_t ucSubType; /* sub type filed*/
    uint16_t u2Length;
    uint16_t u2Seq;
#endif /* #if CFG_SUPPORT_WAPI */
    uint8_t ucEAPoLKey = 0;
    uint8_t ucEapOffset = ETHER_HEADER_LEN;
    uint16_t u2KeyInfo = 0;

    pucEapol = pucIpHdr;

    if (u2EthType == ETH_P_1X) {

        ucEapolType = pucEapol[1];

        /* Leave EAP to check */
        ucEAPoLKey = aucLookAheadBuf[1 + ucEapOffset];
        if (ucEAPoLKey != ETH_EAPOL_KEY)
            prTxPktInfo->u2Flag |= BIT(ENUM_PKT_NON_PROTECTED_1X);
        else {
            WLAN_GET_FIELD_BE16(&aucLookAheadBuf[5 + ucEapOffset],
                                &u2KeyInfo);
            /* BIT3 is pairwise key bit */
            DBGLOG(TX, INFO, "u2KeyInfo=%d\n", u2KeyInfo);
            if (u2KeyInfo & BIT(3))
                prTxPktInfo->u2Flag |=
                    BIT(ENUM_PKT_NON_PROTECTED_1X);
        }


        switch (ucEapolType) {
            case 0: /* eap packet */

                ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
                GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
                LOG_FUNC("<TX> EAP Packet: code %d, id %d, type %d, PKT[0x%p], SeqNo: %d\r\n",
                         pucEapol[4], pucEapol[5], pucEapol[7], prPacket,
                         ucSeqNo);

                DBGLOG(TX, INFO,
                       "<TX> EAP Packet: code %d, id %d, type %d, PKT[0x%p], SeqNo: %d\n",
                       pucEapol[4], pucEapol[5], pucEapol[7], prPacket,
                       ucSeqNo);
                break;
            case 1: /* eapol start */
                ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
                GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);

                LOG_FUNC("<TX> EAPOL: start, PKT[0x%p], SeqNo: %d\r\n",
                         prPacket, ucSeqNo);
                DBGLOG(TX, INFO,
                       "<TX> EAPOL: start, PKT[0x%p], SeqNo: %d\n",
                       prPacket, ucSeqNo);
                break;
            case 3: /* key */

                ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
                GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);

                LOG_FUNC("<TX> EAPOL: key, KeyInfo 0x%04x, PKT[0x%p], SeqNo: %d\n",
                         *((uint16_t *)(&pucEapol[5])), prPacket,
                         ucSeqNo);
                DBGLOG(TX, INFO,
                       "<TX> EAPOL: key, KeyInfo 0x%04x, PKT[0x%p], SeqNo: %d\n",
                       *((uint16_t *)(&pucEapol[5])), prPacket,
                       ucSeqNo);
                break;
        }
#if CFG_SUPPORT_WAPI
    } else if (u2EthType == ETH_WPI_1X) {

        ucSubType = pucEapol[3]; /* sub type filed*/
        u2Length = *(uint16_t *)&pucEapol[6];
        u2Seq = *(uint16_t *)&pucEapol[8];
        ucSeqNo = nicIncreaseTxSeqNum(prGlueInfo->prAdapter);
        GLUE_SET_PKT_SEQ_NO(prPacket, ucSeqNo);
        prTxPktInfo->u2Flag |= BIT(ENUM_PKT_NON_PROTECTED_1X);

        DBGLOG(TX, INFO,
               "<TX> WAPI: subType %d, Len %d, Seq %d, PKT[0x%p], SeqNo: %d\n",
               ucSubType, u2Length, u2Seq, prPacket, ucSeqNo);
#endif /* #if CFG_SUPPORT_WAPI */
    }
    prTxPktInfo->u2Flag |= BIT(ENUM_PKT_1X);
    return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This inline function is to extract some packet information, including
 *        user priority, packet length, destination address, 802.1x and BT over
 *        Wi-Fi or not.
 *
 * @param prGlueInfo         Pointer to the glue structure
 * @param prPacket           Packet descriptor
 * @param prTxPktInfo        Extracted packet info
 *
 * @retval TRUE      Success to extract information
 * @retval FALSE     Fail to extract correct information
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_TX
uint8_t kalQoSFrameClassifierAndPacketInfo(IN struct GLUE_INFO *prGlueInfo,
                                           IN void *prPacket, OUT struct TX_PACKET_INFO *prTxPktInfo)
{
    uint32_t u4PacketLen;
    uint16_t u2EtherTypeLen;
    struct pkt_buf *prSkb = (struct pkt_buf *)prPacket;
    uint8_t *aucLookAheadBuf = NULL;
    uint32_t u4EthTypeLenOffset = ETHER_HEADER_LEN -
                                  ETHER_TYPE_LEN;
    uint8_t *pucNextProtocol = NULL;
#if DSCP_SUPPORT
    uint8_t ucUserPriority = 0;
#endif /* #if DSCP_SUPPORT */

    u4PacketLen = prSkb->payload_len;

    if (u4PacketLen < ETHER_HEADER_LEN) {
        DBGLOG(INIT, WARN, "Invalid Ether packet length: %u\n",
               u4PacketLen);
        return FALSE;
    }

    aucLookAheadBuf = prSkb->payload;

    /* Reset Packet Info */
    kalMemZero(prTxPktInfo, sizeof(struct TX_PACKET_INFO));

    /* 4 <0> Obtain Ether Type/Len */
    WLAN_GET_FIELD_BE16(&aucLookAheadBuf[u4EthTypeLenOffset],
                        &u2EtherTypeLen);

#if (CFG_WIFI_TX_ETH_CHK_EMPTY_PAYLOAD == 1)
    if (u2EtherTypeLen >= ETHER_TYPE_MIN &&
        u4PacketLen == ETHER_HEADER_LEN) {
        DBGLOG(TX, WARN,
               "Drop 802.3 header only but no payload packet\n");
        return FALSE;
    }
#endif /* #if (CFG_WIFI_TX_ETH_CHK_EMPTY_PAYLOAD == 1) */

    /* 4 <1> Skip 802.1Q header (VLAN Tagging) */
    if (u2EtherTypeLen == ETH_P_VLAN) {
        prTxPktInfo->u2Flag |= BIT(ENUM_PKT_VLAN_EXIST);
        u4EthTypeLenOffset += ETH_802_1Q_HEADER_LEN;
        WLAN_GET_FIELD_BE16(&aucLookAheadBuf[u4EthTypeLenOffset],
                            &u2EtherTypeLen);
    }
    /* 4 <2> Obtain next protocol pointer */
    pucNextProtocol = &aucLookAheadBuf[u4EthTypeLenOffset +
                                                          ETHER_TYPE_LEN];

    /* 4 <3> Handle ethernet format */
    switch (u2EtherTypeLen) {
        case ETH_P_IPV4:
            /* IPv4 header length check */
            if (u4PacketLen < (u4EthTypeLenOffset + ETHER_TYPE_LEN +
                               IPV4_HDR_LEN)) {
                DBGLOG(INIT, WARN, "Invalid IPv4 packet length: %u\n",
                       u4PacketLen);
                break;
            }
#if DSCP_SUPPORT
            if (GLUE_GET_PKT_BSS_IDX(prSkb) != P2P_DEV_BSS_INDEX) {
                ucUserPriority = getUpFromDscp(prGlueInfo,
                                               GLUE_GET_PKT_BSS_IDX(prSkb),
                                               (pucNextProtocol[1] & 0xFC) >> 2);
                DBGLOG(QM, INFO, "Priority=%d, tos=0x%x",
                       ucUserPriority, (pucNextProtocol[1] & 0xFC) >> 2);
            }
#endif /* #if DSCP_SUPPORT */
            kalIPv4FrameClassifier(prGlueInfo, prPacket,
                                   pucNextProtocol, prTxPktInfo);
            break;

        case ETH_P_ARP:
            kalArpFrameClassifier(prGlueInfo, prPacket, pucNextProtocol,
                                  prTxPktInfo);
            break;

        case ETH_P_1X:
        case ETH_P_PRE_1X:
#if CFG_SUPPORT_WAPI
        case ETH_WPI_1X:
#endif /* #if CFG_SUPPORT_WAPI */
            kalSecurityFrameClassifier(prGlueInfo, prPacket,
                                       pucNextProtocol, u2EtherTypeLen, aucLookAheadBuf,
                                       prTxPktInfo);
            break;

        case ETH_PRO_TDLS:
            kalTdlsFrameClassifier(prGlueInfo, prPacket,
                                   pucNextProtocol, prTxPktInfo);
            break;
#if CFG_SUPPORT_WIFI_SYSDVT
#if (CFG_TCP_IP_CHKSUM_OFFLOAD)
        case ETH_P_IPV6:
            kalIPv6FrameClassifier(prGlueInfo, prPacket,
                                   pucNextProtocol, prTxPktInfo);
            break;
#endif /* #if (CFG_TCP_IP_CHKSUM_OFFLOAD) */
#endif /* #if CFG_SUPPORT_WIFI_SYSDVT */
        default:
            /* 4 <4> Handle 802.3 format if LEN <= 1500 */
            if (u2EtherTypeLen <= ETH_802_3_MAX_LEN)
                prTxPktInfo->u2Flag |= BIT(ENUM_PKT_802_3);
            break;
    }

    /* 4 <4.1> Check for PAL (BT over Wi-Fi) */
    /* Move to kalBowFrameClassifier */

    /* 4 <5> Return the value of Priority Parameter. */
    /* prSkb->priority is assigned by Linux wireless utility
     * function(cfg80211_classify8021d)
     */
    /* at net_dev selection callback (ndo_select_queue) */

#if DSCP_SUPPORT
    if (ucUserPriority != 0xFF)
        prTxPktInfo->ucPriorityParam = ucUserPriority;
    else
#endif /* #if DSCP_SUPPORT */
        prTxPktInfo->ucPriorityParam = (pucNextProtocol[1] & 0xE0) >> 5;
    DBGLOG(QM, INFO, "UP=%d", prTxPktInfo->ucPriorityParam);

    /* 4 <6> Retrieve Packet Information - DA */
    /* Packet Length/ Destination Address */
    prTxPktInfo->u4PacketLen = u4PacketLen;

    kalMemCopy(prTxPktInfo->aucEthDestAddr, aucLookAheadBuf,
               PARAM_MAC_ADDR_LEN);

    return TRUE;
}               /* end of kalQoSFrameClassifier() */

WLAN_ATTR_TEXT_IN_MEM_TX
uint8_t kalGetEthDestAddr(IN struct GLUE_INFO *prGlueInfo,
                          IN void *prPacket, OUT uint8_t *pucEthDestAddr)
{
    struct pkt_buf *prSkb = (struct pkt_buf *)prPacket;
    uint8_t *aucLookAheadBuf = NULL;

    /* Sanity Check */
    if (!prPacket || !prGlueInfo)
        return FALSE;

    aucLookAheadBuf = prSkb->payload;

    kalMemCopy(pucEthDestAddr, aucLookAheadBuf,
               PARAM_MAC_ADDR_LEN);

    return TRUE;
}

void kalOidComplete(IN struct GLUE_INFO *prGlueInfo,
                    IN uint8_t fgSetQuery, IN uint32_t u4SetQueryInfoLen,
                    IN uint32_t rOidStatus)
{

    ASSERT(prGlueInfo);
    /* remove timeout check timer */
    wlanoidClearTimeoutCheck(prGlueInfo->prAdapter);

    prGlueInfo->rPendStatus = rOidStatus;

    prGlueInfo->u4OidCompleteFlag = 1;
    /* complete ONLY if there are waiters */
    if (!completion_done(&prGlueInfo->rPendComp)) {
        complete(&prGlueInfo->rPendComp);
    } else {
        DBGLOG(INIT, WARN, "SKIP multiple OID complete!\n");
        /* WARN_ON(TRUE); */
    }

    if (rOidStatus == WLAN_STATUS_SUCCESS)
        DBGLOG(INIT, TRACE, "Complete OID, status:success\n");
    else
        DBGLOG(INIT, WARN, "Complete OID, status:0x%08x\n",
               rOidStatus);

    /* else let it timeout on kalIoctl entry */
}

void kalOidClearance(IN struct GLUE_INFO *prGlueInfo)
{

}

void kalGetLocalTime(unsigned long long *sec, unsigned long *nsec)
{
    DBGLOG(INIT, ERROR,
           "The input parameters error when get local time\n");
}

/*
 * kalThreadSchedRetrieve
 * Retrieve thread's current scheduling statistics and
 * stored in output "sched".
 * Return value:
 *   0 : Schedstats successfully retrieved
 *  -1 : Kernel's schedstats feature not enabled
 *  -2 : pThread not yet initialized or sched is a NULL pointer
 */
static int32_t kalThreadSchedRetrieve(struct task_struct *pThread,
                                      struct KAL_THREAD_SCHEDSTATS *pSched)
{
#ifdef CONFIG_SCHEDSTATS
    struct sched_entity se;
    unsigned long long sec;
    unsigned long usec;

    if (!pSched)
        return -2;

    /* always clear sched to simplify error handling at caller side */
    memset(pSched, 0, sizeof(struct KAL_THREAD_SCHEDSTATS));

    if (!pThread)
        return -2;

    memcpy(&se, &pThread->se, sizeof(struct sched_entity));
    kalGetLocalTime(&sec, &usec);

    pSched->time = sec * 1000 + usec / 1000;
    pSched->exec = se.sum_exec_runtime;
    pSched->runnable = se.statistics.wait_sum;
    pSched->iowait = se.statistics.iowait_sum;

    return 0;
#else /* #ifdef CONFIG_SCHEDSTATS */
    /* always clear sched to simplify error handling at caller side */
    if (pSched)
        memset(pSched, 0, sizeof(struct KAL_THREAD_SCHEDSTATS));
    return -1;
#endif /* #ifdef CONFIG_SCHEDSTATS */
}

/*
 * kalThreadSchedMark
 * Record the thread's current schedstats and stored in
 * output "schedstats" parameter for profiling at later time.
 * Return value:
 *   0 : Schedstats successfully recorded
 *  -1 : Kernel's schedstats feature not enabled
 *  -2 : pThread not yet initialized or invalid parameters
 */
int32_t kalThreadSchedMark(struct task_struct *pThread,
                           struct KAL_THREAD_SCHEDSTATS *pSchedstats)
{
    return kalThreadSchedRetrieve(pThread, pSchedstats);
}

/*
 * kalThreadSchedUnmark
 * Calculate scheduling statistics against the previously marked point.
 * The result will be filled back into the schedstats output parameter.
 * Return value:
 *   0 : Schedstats successfully calculated
 *  -1 : Kernel's schedstats feature not enabled
 *  -2 : pThread not yet initialized or invalid parameters
 */
int32_t kalThreadSchedUnmark(struct task_struct *pThread,
                             struct KAL_THREAD_SCHEDSTATS *pSchedstats)
{
    int32_t ret;
    struct KAL_THREAD_SCHEDSTATS sched_now;

    ret = kalThreadSchedRetrieve(pThread, &sched_now);
    if (ret == 0) {
        pSchedstats->time =
            sched_now.time - pSchedstats->time;
        pSchedstats->exec =
            sched_now.exec - pSchedstats->exec;
        pSchedstats->runnable =
            sched_now.runnable - pSchedstats->runnable;
        pSchedstats->iowait =
            sched_now.iowait - pSchedstats->iowait;
    }
    return ret;
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to transfer linux ioctl to OID, and  we
 * need to specify the behavior of the OID by ourself
 *
 * @param prGlueInfo         Pointer to the glue structure
 * @param pvInfoBuf          Data buffer
 * @param u4InfoBufLen       Data buffer length
 * @param fgRead             Is this a read OID
 * @param fgWaitResp         does this OID need to wait for values
 * @param fgCmd              does this OID compose command packet
 * @param pu4QryInfoLen      The data length of the return values
 *
 * @retval TRUE      Success to extract information
 * @retval FALSE     Fail to extract correct information
 */
/*----------------------------------------------------------------------------*/

/* todo: enqueue the i/o requests for multiple processes access */
/*  */
/* currently, return -1 */
/*  */

/* static GL_IO_REQ_T OidEntry; */

uint8_t GET_IOCTL_BSSIDX(
    IN struct ADAPTER *prAdapter)
{
    uint8_t ucBssIndex = AIS_DEFAULT_INDEX;

    if (prAdapter) {
        struct GL_IO_REQ *prIoReq = NULL;

        prIoReq =
            &(prAdapter->prGlueInfo
              ->OidEntry);

        ucBssIndex = prIoReq->ucBssIndex;
    }

    DBGLOG(OID, LOUD, "ucBssIndex = %d\n", ucBssIndex);

    return ucBssIndex;
}

void SET_IOCTL_BSSIDX(
    IN struct ADAPTER *prAdapter,
    IN uint8_t ucBssIndex)
{
    if (prAdapter) {
        struct GL_IO_REQ *prIoReq = NULL;

        prIoReq =
            &(prAdapter->prGlueInfo
              ->OidEntry);

        DBGLOG(OID, LOUD,
               "ucBssIndex = %d\n", ucBssIndex);

        prIoReq->ucBssIndex = ucBssIndex;
    }
}

uint32_t kalIoctl(IN struct GLUE_INFO *prGlueInfo,
                  IN PFN_OID_HANDLER_FUNC pfnOidHandler,
                  IN void *pvInfoBuf,
                  IN uint32_t u4InfoBufLen, IN uint8_t fgRead,
                  IN uint8_t fgWaitResp, IN uint8_t fgCmd,
                  OUT uint32_t *pu4QryInfoLen)
{
    return kalIoctlByBssIdx(
               prGlueInfo,
               pfnOidHandler,
               pvInfoBuf,
               u4InfoBufLen,
               fgRead,
               fgWaitResp,
               fgCmd,
               pu4QryInfoLen,
               AIS_DEFAULT_INDEX);
}

uint32_t kalIoctlByBssIdx(IN struct GLUE_INFO *prGlueInfo,
                          IN PFN_OID_HANDLER_FUNC pfnOidHandler,
                          IN void *pvInfoBuf,
                          IN uint32_t u4InfoBufLen, IN uint8_t fgRead,
                          IN uint8_t fgWaitResp, IN uint8_t fgCmd,
                          OUT uint32_t *pu4QryInfoLen,
                          IN uint8_t ucBssIndex)
{
    struct GL_IO_REQ *prIoReq = NULL;
    uint32_t ret = WLAN_STATUS_SUCCESS;

    KAL_SPIN_LOCK_DECLARATION();

    /*
        if (kalIsResetting())
            return WLAN_STATUS_SUCCESS;
    */

    if (!prGlueInfo || !prGlueInfo->prAdapter) {
        DBGLOG(INIT, INFO,
               "QA_AGENT prGlueInfo or prGlueInfo->prAdapter NULL\n");
        return WLAN_STATUS_FAILURE;
    }

    if (wlanIsChipAssert(prGlueInfo->prAdapter))
        return WLAN_STATUS_SUCCESS;

    /* GLUE_SPIN_LOCK_DECLARATION(); */
    ASSERT(prGlueInfo);

    /* <1> Check if driver is halt */
    /* if (prGlueInfo->u4Flag & GLUE_FLAG_HALT) { */
    /* return WLAN_STATUS_ADAPTER_NOT_READY; */
    /* } */

    if (xSemaphoreTake(g_halt_sem, 2000 / portTICK_PERIOD_MS) != true) {
        DBGLOG(INIT, ERROR,
               "Ioctl timeout: g_halt_sem\n");
        return WLAN_STATUS_FAILURE;
    }

    if (g_u4HaltFlag) {
        xSemaphoreGive(g_halt_sem);
        return WLAN_STATUS_ADAPTER_NOT_READY;
    }

    if (xSemaphoreTake(prGlueInfo->ioctl_sem, 2000 / portTICK_PERIOD_MS) != true) {
        DBGLOG(INIT, ERROR,
               "Ioctl timeout: ioctl_sem\n");
        xSemaphoreGive(g_halt_sem);
        return WLAN_STATUS_FAILURE;
    }


    /* <2> TODO: thread-safe */

    /* <3> point to the OidEntry of Glue layer */

    KAL_ACQUIRE_SPIN_LOCK(prGlueInfo->prAdapter, SPIN_LOCK_IO_REQ);

    prIoReq = &(prGlueInfo->OidEntry);

    ASSERT(prIoReq);

    /* <4> Compose the I/O request */
    prIoReq->prAdapter = prGlueInfo->prAdapter;
    prIoReq->pfnOidHandler = pfnOidHandler;
    prIoReq->pvInfoBuf = pvInfoBuf;
    prIoReq->u4InfoBufLen = u4InfoBufLen;
    prIoReq->pu4QryInfoLen = pu4QryInfoLen;
    prIoReq->fgRead = fgRead;
    prIoReq->fgWaitResp = fgWaitResp;
    prIoReq->rStatus = WLAN_STATUS_FAILURE;
    SET_IOCTL_BSSIDX(
        prGlueInfo->prAdapter,
        ucBssIndex);

    /* <5> Reset the status of pending OID */
    prGlueInfo->rPendStatus = WLAN_STATUS_FAILURE;
    /* prGlueInfo->u4TimeoutFlag = 0; */
    prGlueInfo->u4OidCompleteFlag = 0;

    /* <6> Check if we use the command queue */
    prIoReq->u4Flag = fgCmd;

    /* <7> schedule the OID bit
     * Use memory barrier to ensure OidEntry is written done and then set
     * bit.
     */
    xEventGroupSetBits(prGlueInfo->event_main_thread, GLUE_FLAG_OID);
    /* vTaskResume(prGlueInfo->main_thread); */

    KAL_RELEASE_SPIN_LOCK(prGlueInfo->prAdapter, SPIN_LOCK_IO_REQ);

    /* <9> Block and wait for event or timeout, current the timeout is 3 secs */
    wait_for_completion_timeout(&prGlueInfo->rPendComp, PRIV_CLI_CMD_TIMEOUT);

    KAL_ACQUIRE_SPIN_LOCK(prGlueInfo->prAdapter, SPIN_LOCK_IO_REQ);
    {
        /* Case 1: No timeout. */
        /* if return WLAN_STATUS_PENDING, the status of cmd is stored in prGlueInfo  */
        if (prIoReq->rStatus == WLAN_STATUS_PENDING)
            ret = prGlueInfo->rPendStatus;
        else {
            /*
             * TX CMD may finished earlier than prPendStatus is
             * set to PENDING, while it will update
             * pPendStatus first then do complete.
             * So if rPendStatus is changed from init
             * value, this value should be referenced
             */
            if (prGlueInfo->rPendStatus != WLAN_STATUS_FAILURE)
                ret = prGlueInfo->rPendStatus;
            else
                ret = prIoReq->rStatus;
        }
    }
    KAL_RELEASE_SPIN_LOCK(prGlueInfo->prAdapter, SPIN_LOCK_IO_REQ);

    /* <10> Clear bit for error handling */
    xEventGroupClearBits(prGlueInfo->event_main_thread, GLUE_FLAG_OID);

    xSemaphoreGive(prGlueInfo->ioctl_sem);
    xSemaphoreGive(g_halt_sem);


    return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending security frames
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearSecurityFrames(IN struct GLUE_INFO *prGlueInfo)
{
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear pending security frames
 *        belongs to dedicated network type
 *
 * \param prGlueInfo         Pointer of GLUE Data Structure
 * \param eNetworkTypeIdx    Network Type Index
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearSecurityFramesByBssIdx(IN struct GLUE_INFO
                                    *prGlueInfo, IN uint8_t ucBssIndex)
{
    struct QUE *prCmdQue;
    struct QUE rTempCmdQue;
    struct QUE *prTempCmdQue = &rTempCmdQue;
    struct QUE rReturnCmdQue;
    struct QUE *prReturnCmdQue = &rReturnCmdQue;
    struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
    struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;
    struct MSDU_INFO *prMsduInfo;
    uint8_t fgFree;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    QUEUE_INITIALIZE(prReturnCmdQue);
    /* Clear pending security frames in prGlueInfo->rCmdQueue */
    prCmdQue = &prGlueInfo->rCmdQueue;

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
                      struct QUE_ENTRY *);
    while (prQueueEntry) {
        prCmdInfo = (struct CMD_INFO *) prQueueEntry;
        prMsduInfo = prCmdInfo->prMsduInfo;
        fgFree = FALSE;

        if (prCmdInfo->eCmdType == COMMAND_TYPE_SECURITY_FRAME
            && prMsduInfo) {
            if (prMsduInfo->ucBssIndex == ucBssIndex)
                fgFree = TRUE;
        }

        if (fgFree) {
            if (prCmdInfo->pfCmdTimeoutHandler)
                prCmdInfo->pfCmdTimeoutHandler(
                    prGlueInfo->prAdapter, prCmdInfo);
            else
                wlanReleaseCommand(prGlueInfo->prAdapter,
                                   prCmdInfo, TX_RESULT_QUEUE_CLEARANCE);
            cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
            GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
        } else
            QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);

        QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
                          struct QUE_ENTRY *);
    }

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending management frames
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearMgmtFrames(IN struct GLUE_INFO *prGlueInfo)
{
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all pending management frames
 *           belongs to dedicated network type
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearMgmtFramesByBssIdx(IN struct GLUE_INFO
                                *prGlueInfo, IN uint8_t ucBssIndex)
{
    struct QUE *prCmdQue;
    struct QUE rTempCmdQue;
    struct QUE *prTempCmdQue = &rTempCmdQue;
    struct QUE rReturnCmdQue;
    struct QUE *prReturnCmdQue = &rReturnCmdQue;
    struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
    struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;
    struct MSDU_INFO *prMsduInfo;
    uint8_t fgFree;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    QUEUE_INITIALIZE(prReturnCmdQue);
    /* Clear pending management frames in prGlueInfo->rCmdQueue */
    prCmdQue = &prGlueInfo->rCmdQueue;

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
                      struct QUE_ENTRY *);
    while (prQueueEntry) {
        prCmdInfo = (struct CMD_INFO *) prQueueEntry;
        prMsduInfo = prCmdInfo->prMsduInfo;
        fgFree = FALSE;

        if (prCmdInfo->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME
            && prMsduInfo) {
            if (prMsduInfo->ucBssIndex == ucBssIndex)
                fgFree = TRUE;
        }

        if (fgFree) {
            wlanReleaseCommand(prGlueInfo->prAdapter, prCmdInfo,
                               TX_RESULT_QUEUE_CLEARANCE);
            cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
            GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
        } else {
            QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);
        }

        QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
                          struct QUE_ENTRY *);
    }

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}               /* kalClearMgmtFramesByBssIdx */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all commands in command queue
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalClearCommandQueue(IN struct GLUE_INFO *prGlueInfo)
{
}

uint32_t kalProcessTxPacket(struct GLUE_INFO *prGlueInfo,
                            struct pkt_buf *prSkb)
{
    uint32_t u4Status = WLAN_STATUS_SUCCESS;

    if (prSkb == NULL) {
        DBGLOG(INIT, WARN, "prSkb == NULL in tx\n");
        return u4Status;
    }

    /* Handle security frame */
    if (0 /* GLUE_TEST_PKT_FLAG(prSkb, ENUM_PKT_1X) */
        /* No more sending via cmd */) {
        if (wlanProcessSecurityFrame(prGlueInfo->prAdapter,
                                     (void *) prSkb)) {
            u4Status = WLAN_STATUS_SUCCESS;
            GLUE_INC_REF_CNT(
                prGlueInfo->i4TxPendingSecurityFrameNum);
        } else {
            u4Status = WLAN_STATUS_RESOURCES;
        }
    }
    /* Handle normal frame */
    else
        u4Status = wlanEnqueueTxPacket(prGlueInfo->prAdapter,
                                       (void *) prSkb);

    return u4Status;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to process Tx request to main_thread
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_TX
void kalProcessTxReq(struct GLUE_INFO *prGlueInfo,
                     uint8_t *pfgNeedHwAccess)
{
    struct QUE *prCmdQue = NULL;
    struct QUE *prTxQueue = NULL;
    struct QUE_ENTRY *prQueueEntry = NULL;
    uint32_t u4Status;
    uint32_t u4TxLoopCount;
    struct QUE rTempQue;
    struct QUE *prTempQue = &rTempQue;
    struct QUE rTempReturnQue;
    struct QUE *prTempReturnQue = &rTempReturnQue;
    /* struct sk_buff      *prSkb = NULL; */
#if CFG_SUPPORT_MULTITHREAD
    uint32_t u4CmdCount = 0;
#endif /* #if CFG_SUPPORT_MULTITHREAD */

    /* for spin lock acquire and release */
    GLUE_SPIN_LOCK_DECLARATION();

    prTxQueue = &prGlueInfo->rTxQueue;
    prCmdQue = &prGlueInfo->rCmdQueue;

    QUEUE_INITIALIZE(prTempQue);
    QUEUE_INITIALIZE(prTempReturnQue);

    u4TxLoopCount =
        prGlueInfo->prAdapter->rWifiVar.u4TxFromOsLoopCount;

    /* Process Mailbox Messages */
    wlanProcessMboxMessage(prGlueInfo->prAdapter);

    /* Process CMD request */
#if CFG_SUPPORT_MULTITHREAD
    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    u4CmdCount = prCmdQue->u4NumElem;
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    if (u4CmdCount > 0)
        wlanProcessCommandQueue(prGlueInfo->prAdapter, prCmdQue);
#else /* #if CFG_SUPPORT_MULTITHREAD */
    if (prCmdQue->u4NumElem > 0) {
        if (*pfgNeedHwAccess == FALSE) {
            *pfgNeedHwAccess = TRUE;

            wlanAcquirePowerControl(prGlueInfo->prAdapter);
        }
        wlanProcessCommandQueue(prGlueInfo->prAdapter, prCmdQue);
    }
#endif /* #if CFG_SUPPORT_MULTITHREAD */
    while (u4TxLoopCount--) {
        while (QUEUE_IS_NOT_EMPTY(prTxQueue)) {
            GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
            QUEUE_MOVE_ALL(prTempQue, prTxQueue);
            GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);

            /* Handle Packet Tx */
            while (QUEUE_IS_NOT_EMPTY(prTempQue)) {
                QUEUE_REMOVE_HEAD(prTempQue, prQueueEntry,
                                  struct QUE_ENTRY *);

                if (prQueueEntry == NULL)
                    break;

                u4Status = kalProcessTxPacket(prGlueInfo,
                                              (struct pkt_buf *)
                                              GLUE_GET_PKT_DESCRIPTOR(
                                                  prQueueEntry));
                /* Enqueue packet back into TxQueue if resource
                 * is not enough
                 */
                if (u4Status == WLAN_STATUS_RESOURCES) {
                    QUEUE_INSERT_TAIL(prTempReturnQue,
                                      prQueueEntry);
                    break;
                }
            }

            if (wlanGetTxPendingFrameCount(
                    prGlueInfo->prAdapter) > 0)
                wlanTxPendingPackets(prGlueInfo->prAdapter,
                                     pfgNeedHwAccess);

            /* Enqueue packet back into TxQueue if resource is not
             * enough
             */
            if (QUEUE_IS_NOT_EMPTY(prTempReturnQue)) {
                QUEUE_CONCATENATE_QUEUES(prTempReturnQue,
                                         prTempQue);

                GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo,
                                       SPIN_LOCK_TX_QUE);
                QUEUE_CONCATENATE_QUEUES_HEAD(prTxQueue,
                                              prTempReturnQue);
                GLUE_RELEASE_SPIN_LOCK(prGlueInfo,
                                       SPIN_LOCK_TX_QUE);

                break;
            }
        }
        if (wlanGetTxPendingFrameCount(prGlueInfo->prAdapter) > 0)
            wlanTxPendingPackets(prGlueInfo->prAdapter,
                                 pfgNeedHwAccess);
    }
}

#if CFG_SUPPORT_MULTITHREAD
/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param data       data pointer to private data of hif_thread
 *
 * @retval           If the function succeeds, the return value is 0.
 * Otherwise, an error code is returned.
 *
 */
/*----------------------------------------------------------------------------*/

int hif_thread(void *data)
{
    struct net_device *dev = data;
    struct GLUE_INFO *prGlueInfo = *((struct GLUE_INFO **)
                                     netdev_priv(dev));
    struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
    int ret = 0;
#if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK)
    KAL_WAKE_LOCK_T *prHifThreadWakeLock;

    prHifThreadWakeLock = kmalloc(sizeof(KAL_WAKE_LOCK_T));
    if (!prHifThreadWakeLock) {
        DBGLOG(INIT, ERROR, "%s MemAlloc Fail\n",
               KAL_GET_CURRENT_THREAD_NAME());
        return 0;
    }

    KAL_WAKE_LOCK_INIT(prGlueInfo->prAdapter,
                       prHifThreadWakeLock, "WLAN hif_thread");
    KAL_WAKE_LOCK(prGlueInfo->prAdapter, prHifThreadWakeLock);
#endif /* #if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK) */

    DBGLOG(INIT, INFO, "%s:%u starts running...\n",
           KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

    prGlueInfo->u4HifThreadPid = KAL_GET_CURRENT_THREAD_ID();

    set_user_nice(current, prAdapter->rWifiVar.cThreadNice);

    while (TRUE) {

        if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
            DBGLOG(INIT, INFO, "hif_thread should stop now...\n");
            break;
        }

        /* Unlock wakelock if hif_thread going to idle */
        if (!(prGlueInfo->ulFlag & GLUE_FLAG_HIF_PROCESS))
            KAL_WAKE_UNLOCK(prGlueInfo->prAdapter,
                            prHifThreadWakeLock);

        /*
         * sleep on waitqueue if no events occurred. Event contain
         * (1) GLUE_FLAG_INT (2) GLUE_FLAG_OID (3) GLUE_FLAG_TXREQ
         * (4) GLUE_FLAG_HALT
         *
         */
        do {
            ret = wait_event_interruptible(prGlueInfo->waitq_hif,
                                           ((prGlueInfo->ulFlag & GLUE_FLAG_HIF_PROCESS)
                                            != 0));
        } while (ret != 0);
#if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK)
        if (!KAL_WAKE_LOCK_ACTIVE(prGlueInfo->prAdapter,
                                  prHifThreadWakeLock))
            KAL_WAKE_LOCK(prGlueInfo->prAdapter,
                          prHifThreadWakeLock);
#endif /* #if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK) */
        if (prAdapter->fgIsFwOwn
            && (prGlueInfo->ulFlag == GLUE_FLAG_HIF_FW_OWN)) {
            DBGLOG(INIT, INFO,
                   "Only FW OWN request, but now already done FW OWN\n");
            clear_bit(GLUE_FLAG_HIF_FW_OWN_BIT,
                      &prGlueInfo->ulFlag);
            continue;
        }
        wlanAcquirePowerControl(prAdapter);

        /* Handle Interrupt */
        if (test_and_clear_bit(GLUE_FLAG_INT_BIT,
                               &prGlueInfo->ulFlag)) {
            /* the Wi-Fi interrupt is already disabled in mmc
             * thread, so we set the flag only to enable the
             * interrupt later
             */
            prAdapter->fgIsIntEnable = FALSE;
            if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
                /* Should stop now... skip pending interrupt */
                DBGLOG(INIT, INFO,
                       "ignore pending interrupt\n");
            } else {
                /* DBGLOG(INIT, INFO, ("HIF Interrupt!\n")); */
                prGlueInfo->TaskIsrCnt++;
                wlanIST(prAdapter);
            }
        }

        /* Skip Tx request if SER is operating */
        if ((prAdapter->fgIsFwOwn == FALSE) &&
            !nicSerIsTxStop(prAdapter)) {
            /* TX Commands */
            if (test_and_clear_bit(GLUE_FLAG_HIF_TX_CMD_BIT,
                                   &prGlueInfo->ulFlag))
                wlanTxCmdMthread(prAdapter);

            /* Process TX data packet to HIF */
            if (test_and_clear_bit(GLUE_FLAG_HIF_TX_BIT,
                                   &prGlueInfo->ulFlag))
                nicTxMsduQueueMthread(prAdapter);
        }

        /* Read chip status when chip no response */
        if (test_and_clear_bit(GLUE_FLAG_HIF_PRT_HIF_DBG_INFO_BIT,
                               &prGlueInfo->ulFlag))
            halPrintHifDbgInfo(prAdapter);

        /* Update Tx Quota */
        if (test_and_clear_bit(GLUE_FLAG_UPDATE_WMM_QUOTA,
                               &prGlueInfo->ulFlag))
            halUpdateTxMaxQuota(prAdapter);

        /* Set FW own */
        if (test_and_clear_bit(GLUE_FLAG_HIF_FW_OWN_BIT,
                               &prGlueInfo->ulFlag))
            prAdapter->fgWiFiInSleepyState = TRUE;

        /* Release to FW own */
        wlanReleasePowerControl(prAdapter);
    }

    complete(&prGlueInfo->rHifHaltComp);
#if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK)
    if (KAL_WAKE_LOCK_ACTIVE(prGlueInfo->prAdapter,
                             prHifThreadWakeLock))
        KAL_WAKE_UNLOCK(prGlueInfo->prAdapter, prHifThreadWakeLock);
    KAL_WAKE_LOCK_DESTROY(prGlueInfo->prAdapter,
                          prHifThreadWakeLock);
    kalMemFree(prHifThreadWakeLock, VIR_MEM_TYPE,
               sizeof(KAL_WAKE_LOCK_T));
#endif /* #if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK) */

    DBGLOG(INIT, TRACE, "%s:%u stopped!\n",
           KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

#if CFG_CHIP_RESET_HANG
    while (fgIsResetHangState == SER_L0_HANG_RST_HANG) {
        kalMsleep(SER_L0_HANG_LOG_TIME_INTERVAL);
        DBGLOG(INIT, STATE, "[SER][L0] SQC hang!\n");
    }
#endif /* #if CFG_CHIP_RESET_HANG */

    return 0;
}

int rx_thread(void *data)
{
    struct net_device *dev = data;
    struct GLUE_INFO *prGlueInfo = *((struct GLUE_INFO **)
                                     netdev_priv(dev));

    struct QUE rTempRxQue;
    struct QUE *prTempRxQue = NULL;
    struct QUE_ENTRY *prQueueEntry = NULL;

    int ret = 0;
#if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK)
    KAL_WAKE_LOCK_T *prRxThreadWakeLock;
#endif /* #if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK) */
    uint32_t u4LoopCount;

    /* for spin lock acquire and release */
    KAL_SPIN_LOCK_DECLARATION();

#if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK)
    prRxThreadWakeLock = kmalloc(sizeof(KAL_WAKE_LOCK_T));
    if (!prRxThreadWakeLock) {
        DBGLOG(INIT, ERROR, "%s MemAlloc Fail\n",
               KAL_GET_CURRENT_THREAD_NAME());
        return 0;
    }

    KAL_WAKE_LOCK_INIT(prGlueInfo->prAdapter,
                       prRxThreadWakeLock, "WLAN rx_thread");
    KAL_WAKE_LOCK(prGlueInfo->prAdapter, prRxThreadWakeLock);
#endif /* #if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK) */

    DBGLOG(INIT, INFO, "%s:%u starts running...\n",
           KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

    prGlueInfo->u4RxThreadPid = KAL_GET_CURRENT_THREAD_ID();

    set_user_nice(current,
                  prGlueInfo->prAdapter->rWifiVar.cThreadNice);

    prTempRxQue = &rTempRxQue;

    while (TRUE) {

        if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
            DBGLOG(INIT, INFO, "rx_thread should stop now...\n");
            break;
        }

        /* Unlock wakelock if rx_thread going to idle */
        if (!(prGlueInfo->ulFlag & GLUE_FLAG_RX_PROCESS))
            KAL_WAKE_UNLOCK(prGlueInfo->prAdapter,
                            prRxThreadWakeLock);

        /*
         * sleep on waitqueue if no events occurred.
         */
        do {
            ret = wait_event_interruptible(prGlueInfo->waitq_rx,
                                           ((prGlueInfo->ulFlag & GLUE_FLAG_RX_PROCESS) != 0));
        } while (ret != 0);
#if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK)
        if (!KAL_WAKE_LOCK_ACTIVE(prGlueInfo->prAdapter,
                                  prRxThreadWakeLock))
            KAL_WAKE_LOCK(prGlueInfo->prAdapter,
                          prRxThreadWakeLock);
#endif /* #if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK) */
        if (test_and_clear_bit(GLUE_FLAG_RX_TO_OS_BIT,
                               &prGlueInfo->ulFlag)) {
            u4LoopCount =
                prGlueInfo->prAdapter->rWifiVar.u4Rx2OsLoopCount;

            while (u4LoopCount--) {
                while (QUEUE_IS_NOT_EMPTY(
                           &prGlueInfo->prAdapter->rRxQueue)) {
                    QUEUE_INITIALIZE(prTempRxQue);

                    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo,
                                           SPIN_LOCK_RX_TO_OS_QUE);
                    QUEUE_MOVE_ALL(prTempRxQue,
                                   &prGlueInfo->prAdapter->rRxQueue);
                    GLUE_RELEASE_SPIN_LOCK(prGlueInfo,
                                           SPIN_LOCK_RX_TO_OS_QUE);

                    while (QUEUE_IS_NOT_EMPTY(
                               prTempRxQue)) {
                        QUEUE_REMOVE_HEAD(prTempRxQue,
                                          prQueueEntry,
                                          struct QUE_ENTRY *);
                        kalRxIndicateOnePkt(prGlueInfo,
                                            (void *)
                                            GLUE_GET_PKT_DESCRIPTOR(
                                                prQueueEntry));
                    }

                    KAL_WAKE_LOCK_TIMEOUT(prGlueInfo->prAdapter,
                                          &prGlueInfo->rTimeoutWakeLock,
                                          MSEC_TO_JIFFIES(prGlueInfo->prAdapter
                                                          ->rWifiVar.u4WakeLockRxTimeout));
                }
            }
        }
    }

    complete(&prGlueInfo->rRxHaltComp);
#if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK)
    if (KAL_WAKE_LOCK_ACTIVE(prGlueInfo->prAdapter,
                             prRxThreadWakeLock))
        KAL_WAKE_UNLOCK(prGlueInfo->prAdapter, prRxThreadWakeLock);
    KAL_WAKE_LOCK_DESTROY(prGlueInfo->prAdapter,
                          prRxThreadWakeLock);
    kalMemFree(prRxThreadWakeLock, VIR_MEM_TYPE,
               sizeof(KAL_WAKE_LOCK_T));
#endif /* #if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK) */

    DBGLOG(INIT, TRACE, "%s:%u stopped!\n",
           KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

#if CFG_CHIP_RESET_HANG
    while (fgIsResetHangState == SER_L0_HANG_RST_HANG) {
        kalMsleep(SER_L0_HANG_LOG_TIME_INTERVAL);
        DBGLOG(INIT, STATE, "[SER][L0] SQC hang!\n");
    }
#endif /* #if CFG_CHIP_RESET_HANG */

    return 0;
}
#endif /* #if CFG_SUPPORT_MULTITHREAD */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is a kernel thread function for handling command packets
 * Tx requests and interrupt events
 *
 * @param data       data pointer to private data of main_thread
 *
 * @retval           If the function succeeds, the return value is 0.
 * Otherwise, an error code is returned.
 *
 */
/*----------------------------------------------------------------------------*/

void main_thread(void *data)
{
    struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)netdev_priv();
    struct GL_IO_REQ *prIoReq = NULL;
    uint8_t fgNeedHwAccess = FALSE;

    prGlueInfo->event_main_thread = xEventGroupCreate();
    xEventGroupSetBits(g_init_wait, WLAN_THREAD_INIT_BIT);

    ASSERT(prGlueInfo);
    ASSERT(prGlueInfo->prAdapter);

    DBGLOG(INIT, INFO, "%s:%u starts running...\n",
           KAL_GET_CURRENT_THREAD_NAME(),
           (uint32_t)KAL_GET_CURRENT_THREAD_ID());

    while (TRUE) {
        if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
            LOG_FUNC("%s should stop now...\n",
                     KAL_GET_CURRENT_THREAD_NAME());
            break;
        }
        prGlueInfo->ulFlag = xEventGroupWaitBits(
                                 prGlueInfo->event_main_thread, GLUE_FLAG_WAIT_BIT,
                                 true, false, portMAX_DELAY);

        if (GLUE_FLAG_FRAME_FILTER_AIS & prGlueInfo->ulFlag) {
            struct AIS_FSM_INFO *prAisFsmInfo = NULL;
            /* printk("prGlueInfo->u4OsMgmtFrameFilter = %x", prGlueInfo->u4OsMgmtFrameFilter); */
            prAisFsmInfo = (struct AIS_FSM_INFO *)
                           & (prGlueInfo->prAdapter->rWifiVar.rAisFsmInfo);
            prAisFsmInfo->u4AisPacketFilter = prGlueInfo->u4OsMgmtFrameFilter;
        }

        if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
            LOG_FUNC("%s should stop now...\n",
                     KAL_GET_CURRENT_THREAD_NAME());
            break;
        }

        fgNeedHwAccess = FALSE;

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
        if (prGlueInfo->fgEnSdioTestPattern == TRUE) {
            if (fgNeedHwAccess == FALSE) {
                fgNeedHwAccess = TRUE;

                wlanAcquirePowerControl(prGlueInfo->prAdapter);
            }

            if (prGlueInfo->fgIsSdioTestInitialized == FALSE) {
                /* enable PRBS mode */
                kalDevRegWrite(prGlueInfo, MCR_WTMCR,
                               0x00080002);
                prGlueInfo->fgIsSdioTestInitialized = TRUE;
            }

            if (prGlueInfo->fgSdioReadWriteMode == TRUE) {
                /* read test */
                kalDevPortRead(prGlueInfo, MCR_WTMDR, 256,
                               prGlueInfo->aucSdioTestBuffer,
                               sizeof(prGlueInfo->aucSdioTestBuffer));
            } else {
                /* write test */
                kalDevPortWrite(prGlueInfo, MCR_WTMDR, 172,
                                prGlueInfo->aucSdioTestBuffer,
                                sizeof(prGlueInfo->aucSdioTestBuffer));
            }
        }
#endif /* #if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN */
#if CFG_SUPPORT_MULTITHREAD
#else /* #if CFG_SUPPORT_MULTITHREAD */
        /* Handle Interrupt */
        if (GLUE_FLAG_INT & prGlueInfo->ulFlag) {
            struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
            kalSLA_CustomLogging_Start(SLA_LABEL_main_thread_DRV_OWN);

            if (fgNeedHwAccess == FALSE) {
                fgNeedHwAccess = TRUE;

                wlanAcquirePowerControl(prGlueInfo->prAdapter);
            }
            kalSLA_CustomLogging_Stop(SLA_LABEL_main_thread_DRV_OWN);
            if (prAdapter->u4NoMoreRfb) {
                DBGLOG(RX, WARN, "halRxReceiveRFBs again\n");
                if (prAdapter->u4NoMoreRfb &
                    BIT(RX_RING_DATA_IDX_0))
                    halRxReceiveRFBs(prAdapter,
                                     RX_RING_DATA_IDX_0, true);
                if (prAdapter->u4NoMoreRfb &
                    BIT(RX_RING_EVT_IDX_1))
                    halRxReceiveRFBs(prAdapter,
                                     RX_RING_EVT_IDX_1, false);
            }
            /* the Wi-Fi interrupt is already disabled in mmc
             * thread, so we set the flag only to enable the
             * interrupt later
             */
            prGlueInfo->prAdapter->fgIsIntEnable = FALSE;
            /* wlanISR(prGlueInfo->prAdapter, TRUE); */

            if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
                /* Should stop now... skip pending interrupt */
                DBGLOG(INIT, INFO,
                       "ignore pending interrupt\n");
            } else {
                kalSLA_CustomLogging_Start(SLA_LABEL_main_thread_wlanIST);
                prGlueInfo->TaskIsrCnt++;
                wlanIST(prGlueInfo->prAdapter);
                kalSLA_CustomLogging_Stop(SLA_LABEL_main_thread_wlanIST);
                kalSLA_CustomLogging_Stop(SLA_LABEL_mtk_axi_interrupt);
            }
        }

        if (prGlueInfo->prAdapter->fgWowIntr == TRUE) {
            prGlueInfo->prAdapter->fgWowIntr = FALSE;
            prGlueInfo->prAdapter->fgIsWowSuspend = FALSE;
            BaseType_t ret = pdPASS;
            TaskHandle_t wow_stop_handler;

            DBGLOG(INIT, INFO, "wifi_wow_stop task create\n");
            ret = xTaskCreate(task_wifi_wow_stop, "wifi_wow_stop"
                              , WLAN_THREAD_STACK_SIZE, prGlueInfo
                              , WLAN_THREAD_TASK_PRI, &wow_stop_handler);

            if (ret != pdPASS)
                DBGLOG(INIT, INFO,
                       "[wifi]create wifi_wow_stop task failed\n");
        }

        if (GLUE_FLAG_UPDATE_WMM_QUOTA & prGlueInfo->ulFlag)
            halUpdateTxMaxQuota(prGlueInfo->prAdapter);
#endif /* #if CFG_SUPPORT_MULTITHREAD */

        do {
            if (GLUE_FLAG_OID & prGlueInfo->ulFlag) {
                /* get current prIoReq */
                KAL_SPIN_LOCK_DECLARATION();
                KAL_ACQUIRE_SPIN_LOCK(prGlueInfo->prAdapter, SPIN_LOCK_IO_REQ);
                prIoReq = &(prGlueInfo->OidEntry);
                if (prIoReq->fgRead == FALSE) {
                    prIoReq->rStatus = wlanSetInformation(
                                           prIoReq->prAdapter,
                                           prIoReq->pfnOidHandler,
                                           prIoReq->pvInfoBuf,
                                           prIoReq->u4InfoBufLen,
                                           prIoReq->pu4QryInfoLen);
                } else {
                    prIoReq->rStatus = wlanQueryInformation(
                                           prIoReq->prAdapter,
                                           prIoReq->pfnOidHandler,
                                           prIoReq->pvInfoBuf,
                                           prIoReq->u4InfoBufLen,
                                           prIoReq->pu4QryInfoLen);
                }

                if (prIoReq->rStatus != WLAN_STATUS_PENDING) {
                    /* complete ONLY if there are waiters */
                    if (!completion_done(
                            &prGlueInfo->rPendComp))
                        complete(
                            &prGlueInfo->rPendComp);
                    else
                        DBGLOG(INIT, TRACE,
                               "SKIP multiple OID complete!\n"
                              );
                } else {
                    wlanoidTimeoutCheck(
                        prGlueInfo->prAdapter,
                        prIoReq->pfnOidHandler);
                }
                KAL_RELEASE_SPIN_LOCK(prGlueInfo->prAdapter, SPIN_LOCK_IO_REQ);
            }

        } while (FALSE);

        /*
         *
         * if TX request, clear the TXREQ flag. TXREQ set by
         * kalSetEvent/GlueSetEvent
         * indicates the following requests occur
         *
         */
        if (GLUE_FLAG_TXREQ & prGlueInfo->ulFlag)
            kalProcessTxReq(prGlueInfo, &fgNeedHwAccess);
#if CFG_SUPPORT_MULTITHREAD
        if (GLUE_FLAG_RX & prGlueInfo->ulFlag)
            nicRxProcessRFBs(prGlueInfo->prAdapter);
        if (GLUE_FLAG_TX_CMD_DONE & prGlueInfo->ulFlag)
            wlanTxCmdDoneMthread(prGlueInfo->prAdapter);
#endif /* #if CFG_SUPPORT_MULTITHREAD */

        /* Process RX, In linux, we don't need to free sk_buff by
         * ourself
         */

        /* In linux, we don't need to free sk_buff by ourself */

        /* In linux, we don't do reset */
#if CFG_SUPPORT_MULTITHREAD
#else /* #if CFG_SUPPORT_MULTITHREAD */
        if (fgNeedHwAccess == TRUE)
            wlanReleasePowerControl(prGlueInfo->prAdapter);
#endif /* #if CFG_SUPPORT_MULTITHREAD */

        if (GLUE_FLAG_TIMEOUT & prGlueInfo->ulFlag)
            wlanTimerTimeoutCheck(prGlueInfo->prAdapter);
#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
        if (prGlueInfo->fgEnSdioTestPattern == TRUE)
            kalSetEvent(prGlueInfo);
#endif /* #if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN */
    }

    /* flush the pending TX packets */
    if (GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum) > 0)
        kalFlushPendingTxPackets(prGlueInfo);

    /* flush pending security frames */
    if (GLUE_GET_REF_CNT(
            prGlueInfo->i4TxPendingSecurityFrameNum) > 0)
        kalClearSecurityFrames(prGlueInfo);

    /* remove pending oid */
    wlanReleasePendingOid(prGlueInfo->prAdapter, 1);
    complete(&prGlueInfo->rHaltComp);

    LOG_FUNC("main_thread exit>\n");
    if (prGlueInfo->event_main_thread != NULL)
        vEventGroupDelete(prGlueInfo->event_main_thread);
    prGlueInfo->main_thread = NULL;
    wlanRemoveAfterMainThread(prGlueInfo);
    vTaskDelete(NULL);

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to check if card is removed
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval TRUE:     card is removed
 *         FALSE:    card is still attached
 */
/*----------------------------------------------------------------------------*/
uint8_t kalIsCardRemoved(IN struct GLUE_INFO *prGlueInfo)
{
    ASSERT(prGlueInfo);

    return FALSE;
    /* Linux MMC doesn't have removal notification yet */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to send command to firmware for overriding
 *        netweork address
 *
 * \param pvGlueInfo Pointer of GLUE Data Structure
 *
 * \retval TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
uint8_t kalRetrieveNetworkAddress(IN struct GLUE_INFO *prGlueInfo,
                                  IN OUT uint8_t *prMacAddr)
{
    ASSERT(prGlueInfo);
    struct ADAPTER *prAdapter;

    prAdapter = prGlueInfo->prAdapter;

    /* Get MAC address override from wlan feature option */
    prGlueInfo->fgIsMacAddrOverride =
        prGlueInfo->prAdapter->rWifiVar.ucMacAddrOverride;

    wlanHwAddrToBin(
        (int8_t *)prGlueInfo->prAdapter->rWifiVar.aucMacAddrStr,
        prGlueInfo->rMacAddrOverride);

    if (prGlueInfo->fgIsMacAddrOverride == FALSE) {

#ifdef CFG_ENABLE_EFUSE_MAC_ADDR
        if (prAdapter->fgIsEmbbededMacAddrValid) {
            COPY_MAC_ADDR(prMacAddr,
                          prGlueInfo->prAdapter->rWifiVar.aucMacAddress);
            return TRUE;
        }
#endif /* #ifdef CFG_ENABLE_EFUSE_MAC_ADDR */

#ifdef MTK_NVDM_ENABLE
        uint8_t buff[PROFILE_BUF_LEN] = {0};
        uint32_t len = sizeof(buff);

        if (nvdm_read_data_item("STA", "MacAddr", buff, &len) !=
            NVDM_STATUS_OK)
            return FALSE;

        wifi_conf_get_mac_from_str((char *)prMacAddr, (char *)buff);

        if (!prMacAddr[0] && !prMacAddr[1] && !prMacAddr[2] &&
            !prMacAddr[3] && !prMacAddr[4] && !prMacAddr[5])
            return FALSE;
        return TRUE;
#endif /* #ifdef MTK_NVDM_ENABLE */

    } else {
        COPY_MAC_ADDR(prMacAddr, prGlueInfo->rMacAddrOverride);

        return TRUE;
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to update the mac address of struct netif
 *
 * \param ucOpMode   the value of opmode
 *        prMacAddr  Pointer of mac address
 *
 * \retval None
 */
/*----------------------------------------------------------------------------*/
void kalUpdateNetifMac(IN uint8_t ucOpMode,
                       IN uint8_t *prMacAddr)
{
    if (ucOpMode == LWIP_STA_MODE) {
        COPY_MAC_ADDR(wlan_sta_netif->hwaddr,
                      prMacAddr);
#if LWIP_IPV6
        netif_create_ip6_linklocal_address(wlan_sta_netif, 1);
#endif /* #if LWIP_IPV6 */
    } else if (ucOpMode == LWIP_AP_MODE) {
        COPY_MAC_ADDR(wlan_ap_netif->hwaddr,
                      prMacAddr);
#if LWIP_IPV6
        netif_create_ip6_linklocal_address(wlan_ap_netif, 1);
#endif /* #if LWIP_IPV6 */
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to flush pending TX packets in glue layer
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalFlushPendingTxPackets(IN struct GLUE_INFO
                              *prGlueInfo)
{
    struct QUE *prTxQue;
    struct QUE_ENTRY *prQueueEntry;
    void *prPacket;

    ASSERT(prGlueInfo);

    prTxQue = &(prGlueInfo->rTxQueue);

    if (GLUE_GET_REF_CNT(prGlueInfo->i4TxPendingFrameNum) == 0)
        return;

    if (HAL_IS_TX_DIRECT()) {
        nicTxDirectClearSkbQ(prGlueInfo->prAdapter);
    } else {
        GLUE_SPIN_LOCK_DECLARATION();

        while (TRUE) {
            GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
            QUEUE_REMOVE_HEAD(prTxQue, prQueueEntry,
                              struct QUE_ENTRY *);
            GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);

            if (prQueueEntry == NULL)
                break;

            prPacket = GLUE_GET_PKT_DESCRIPTOR(prQueueEntry);

            kalSendComplete(prGlueInfo, prPacket,
                            WLAN_STATUS_NOT_ACCEPTED);
        }
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is get indicated media state
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval
 */
/*----------------------------------------------------------------------------*/
enum ENUM_PARAM_MEDIA_STATE kalGetMediaStateIndicated(
    IN struct GLUE_INFO *prGlueInfo,
    IN uint8_t ucBssIndex)
{
    ASSERT(prGlueInfo);

    return prGlueInfo->eParamMediaStateIndicated[ucBssIndex];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set indicated media state
 *
 * \param pvGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalSetMediaStateIndicated(IN struct GLUE_INFO
                               *prGlueInfo, IN enum ENUM_PARAM_MEDIA_STATE
                               eParamMediaStateIndicate,
                               IN uint8_t ucBssIndex)
{
    ASSERT(prGlueInfo);
    if (IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex))
        prGlueInfo->eParamMediaStateIndicated[ucBssIndex] =
            eParamMediaStateIndicate;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear pending OID staying in command queue
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalOidCmdClearance(IN struct GLUE_INFO *prGlueInfo)
{
    struct QUE *prCmdQue;
    struct QUE rTempCmdQue;
    struct QUE *prTempCmdQue = &rTempCmdQue;
    struct QUE rReturnCmdQue;
    struct QUE *prReturnCmdQue = &rReturnCmdQue;
    struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
    struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);

    QUEUE_INITIALIZE(prReturnCmdQue);

    prCmdQue = &prGlueInfo->rCmdQueue;

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
                      struct QUE_ENTRY *);
    while (prQueueEntry) {

        if (((struct CMD_INFO *) prQueueEntry)->fgIsOid) {
            prCmdInfo = (struct CMD_INFO *) prQueueEntry;
            break;
        }
        QUEUE_INSERT_TAIL(prReturnCmdQue, prQueueEntry);
        QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
                          struct QUE_ENTRY *);
    }

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prReturnCmdQue);
    QUEUE_CONCATENATE_QUEUES_HEAD(prCmdQue, prTempCmdQue);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);

    if (prCmdInfo) {
        if (prCmdInfo->pfCmdTimeoutHandler)
            prCmdInfo->pfCmdTimeoutHandler(prGlueInfo->prAdapter,
                                           prCmdInfo);
        else
            kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, 0,
                           WLAN_STATUS_NOT_ACCEPTED);

        prGlueInfo->u4OidCompleteFlag = 1;
        cmdBufFreeCmdInfo(prGlueInfo->prAdapter, prCmdInfo);
        GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to insert command into prCmdQueue
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *        prQueueEntry   Pointer of queue entry to be inserted
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void kalEnqueueCommand(IN struct GLUE_INFO *prGlueInfo,
                       IN struct QUE_ENTRY *prQueueEntry)
{
    struct QUE *prCmdQue;
    struct CMD_INFO *prCmdInfo;
#if CFG_DBG_MGT_BUF
    struct MEM_TRACK *prMemTrack = NULL;
#endif /* #if CFG_DBG_MGT_BUF */

    GLUE_SPIN_LOCK_DECLARATION();

    ASSERT(prGlueInfo);
    ASSERT(prQueueEntry);

    prCmdQue = &prGlueInfo->rCmdQueue;

    prCmdInfo = (struct CMD_INFO *) prQueueEntry;

#if CFG_DBG_MGT_BUF
    if (prCmdInfo->pucInfoBuffer &&
        !IS_FROM_BUF(prGlueInfo->prAdapter,
                     prCmdInfo->pucInfoBuffer)) {
        prMemTrack = (struct MEM_TRACK *)
                     ((uint8_t *)prCmdInfo->pucInfoBuffer -
                      sizeof(struct MEM_TRACK));
        prMemTrack->u2CmdIdAndWhere = 0;
        prMemTrack->u2CmdIdAndWhere |= prCmdInfo->ucCID;
    }
#endif /* #if CFG_DBG_MGT_BUF */

    DBGLOG(INIT, INFO,
           "EN-Q CMD TYPE[%u] ID[0x%02X] SEQ[%u] to CMD Q\n",
           prCmdInfo->eCmdType, prCmdInfo->ucCID,
           prCmdInfo->ucCmdSeqNum);

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
    QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
    GLUE_INC_REF_CNT(prGlueInfo->i4TxPendingCmdNum);
    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_CMD_QUE);
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Handle EVENT_ID_ASSOC_INFO event packet by indicating to OS with
 *        proper information
 *
 * @param pvGlueInfo     Pointer of GLUE Data Structure
 * @param prAssocInfo    Pointer of EVENT_ID_ASSOC_INFO Packet
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void kalHandleAssocInfo(IN struct GLUE_INFO *prGlueInfo,
                        IN struct EVENT_ASSOC_INFO *prAssocInfo)
{
    /* to do */
}

/*----------------------------------------------------------------------------*/
/*!
 * * @brief Notify OS with SendComplete event of the specific packet.
 * *        Linux should free packets here.
 * *
 * * @param pvGlueInfo     Pointer of GLUE Data Structure
 * * @param pvPacket       Pointer of Packet Handle
 * * @param status         Status Code for OS upper layer
 * *
 * * @return none
 */
/*----------------------------------------------------------------------------*/

/* / Todo */
void kalSecurityFrameSendComplete(IN struct GLUE_INFO
                                  *prGlueInfo, IN void *pvPacket, IN uint32_t rStatus)
{
    ASSERT(pvPacket);

    /* dev_kfree_skb((struct sk_buff *) pvPacket); */
    kalSendCompleteAndAwakeQueue(prGlueInfo, pvPacket);
    GLUE_DEC_REF_CNT(prGlueInfo->i4TxPendingSecurityFrameNum);
}

WLAN_ATTR_TEXT_IN_MEM_TX
uint32_t kalGetTxPendingFrameCount(IN struct GLUE_INFO
                                   *prGlueInfo)
{
    ASSERT(prGlueInfo);

    return (uint32_t)(GLUE_GET_REF_CNT(
                          prGlueInfo->i4TxPendingFrameNum));
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to retrieve the number of pending commands
 *        (including MMPDU, 802.1X and command packets)
 *
 * \param prGlueInfo     Pointer of GLUE Data Structure
 *
 * \retval
 */
/*----------------------------------------------------------------------------*/
WLAN_ATTR_TEXT_IN_MEM_TX
uint32_t kalGetTxPendingCmdCount(IN struct GLUE_INFO
                                 *prGlueInfo)
{
    ASSERT(prGlueInfo);

    return (uint32_t)GLUE_GET_REF_CNT(
               prGlueInfo->i4TxPendingCmdNum);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Timer Initialization Procedure
 *
 * \param[in] prGlueInfo     Pointer to GLUE Data Structure
 * \param[in] prTimerHandler Pointer to timer handling function, whose only
 *                           argument is "prAdapter"
 *
 * \retval none
 *
 */
/*----------------------------------------------------------------------------*/

/* static struct timer_list tickfn; */

void kalOsTimerInitialize(IN struct GLUE_INFO *prGlueInfo,
                          IN void *prTimerHandler)
{

    ASSERT(prGlueInfo);

    prGlueInfo->tickfn = xTimerCreate("WF_TIMER",
                                      pdMS_TO_TICKS(WLAN_TIMER_EXPIRE_REQ),
                                      pdFALSE,    /* not reloaded */
                                      /* The ID is used to store a count of the
                                       * number of times the timer has expired, which is initialised to 0. */
                                      (void *) g_prGlueInfo,
                                      prTimerHandler);
}

/* Todo */
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the time to do the time out check.
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 * \param[in] rInterval  Time out interval from current time.
 *
 * \retval TRUE Success.
 */
/*----------------------------------------------------------------------------*/
uint8_t kalSetTimer(IN struct GLUE_INFO *prGlueInfo,
                    IN uint32_t u4Interval)
{
    BaseType_t ret = pdFAIL;

    ASSERT(prGlueInfo);

    if (xTimerIsTimerActive(prGlueInfo->tickfn) != pdFALSE) {
        /* xTimer is active, do something. */
        ret = xTimerStop(prGlueInfo->tickfn, WLAN_TIMER_SET_BLOCKTIME);
        if (ret) {
            DBGLOG(INIT, INFO, "stop timer success\n");
        } else {
            return false;
        }
    }

    /* xTimer is not active, do something else. */
    ret = xTimerChangePeriod(prGlueInfo->tickfn,
                             pdMS_TO_TICKS(u4Interval), WLAN_TIMER_SET_BLOCKTIME);
    if (ret) {
        ret = xTimerStart(prGlueInfo->tickfn, WLAN_TIMER_SET_BLOCKTIME);
        DBGLOG(INIT, INFO, "intvl %d (%d)@ %d\n"
               , u4Interval, pdMS_TO_TICKS(u4Interval), kalGetTimeTick());
    } else {
        DBGLOG(INIT, WARN, "change period timer failed\n");
        return false;
    }
    return TRUE;        /* success */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to cancel
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 *
 * \retval TRUE  :   Timer has been canceled
 *         FALAE :   Timer doens't exist
 */
/*----------------------------------------------------------------------------*/
uint8_t kalCancelTimer(IN struct GLUE_INFO *prGlueInfo)
{
    ASSERT(prGlueInfo);

    if (pdPASS == xTimerStop(prGlueInfo->tickfn, WLAN_TIMER_SET_BLOCKTIME)) {
        xTimerDelete(prGlueInfo->tickfn, 10);
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is a callback function for scanning done
 *
 * \param[in] prGlueInfo Pointer to GLUE Data Structure
 *
 * \retval none
 *
 */
/*----------------------------------------------------------------------------*/
void kalScanDone(IN struct GLUE_INFO *prGlueInfo,
                 IN uint8_t ucBssIndex,
                 IN uint32_t status)
{
    uint8_t fgAborted = (status != WLAN_STATUS_SUCCESS) ? TRUE : FALSE;
    ASSERT(prGlueInfo);

    if (IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex))
        scanLogEssResult(prGlueInfo->prAdapter);

    scanReportBss2Cfg80211(prGlueInfo->prAdapter,
                           BSS_TYPE_INFRASTRUCTURE, NULL);

    /* check for system configuration for generating error message on scan
     * list
     */
    wlanCheckSystemConfiguration(prGlueInfo->prAdapter);

    kalIndicateStatusAndComplete(prGlueInfo, WLAN_STATUS_SCAN_COMPLETE,
                                 &fgAborted, sizeof(fgAborted), ucBssIndex);
}

#if CFG_SUPPORT_SCAN_CACHE_RESULT
/*----------------------------------------------------------------------------*/
/*!
 * @brief update timestamp information of bss cache in kernel
 *
 * @param[in] prAdapter          Pointer to the Adapter structure.
 * @return   status 0 if success, error code otherwise
 */
/*----------------------------------------------------------------------------*/
uint8_t kalUpdateBssTimestamp(IN struct GLUE_INFO *prGlueInfo)
{
    struct wiphy *wiphy;
    struct cfg80211_registered_device *rdev;
    struct cfg80211_internal_bss *bss = NULL;
    struct cfg80211_bss_ies *ies;
    uint64_t new_timestamp = kalGetBootTime();

    ASSERT(prGlueInfo);
    wiphy = priv_to_wiphy(prGlueInfo);
    if (!wiphy) {
        log_dbg(REQ, ERROR, "wiphy is null\n");
        return 1;
    }
    rdev = container_of(wiphy, struct cfg80211_registered_device, wiphy);

    log_dbg(REQ, INFO, "Update scan timestamp: %llu (%llu)\n",
            new_timestamp, le64_to_cpu(new_timestamp));

    /* add 1 ms to prevent scan time too short */
    new_timestamp += 1000;

    spin_lock_bh(&rdev->bss_lock);
    list_for_each_entry(bss, &rdev->bss_list, list) {
        ies = *(struct cfg80211_bss_ies **)
              (((uint32_t) & (bss->pub)) + offsetof(struct cfg80211_bss, ies));
        if (rcu_access_pointer(bss->pub.ies) == ies) {
            ies->tsf = le64_to_cpu(new_timestamp);
        } else {
            log_limited_dbg(REQ, WARN, "Update tsf fail. bss->pub.ies=%p ies=%p\n",
                            bss->pub.ies, ies);
        }
    }
    spin_unlock_bh(&rdev->bss_lock);

    return 0;
}
#endif /* #if CFG_SUPPORT_SCAN_CACHE_RESULT */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to generate a random number
 *
 * \param none
 *
 * \retval UINT_32
 */
/*----------------------------------------------------------------------------*/
uint32_t kalRandomNumber(void)
{
    return rand();
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief command timeout call-back function
 *
 * \param[in] prGlueInfo Pointer to the GLUE data structure.
 *
 * \retval (none)
 */
/*----------------------------------------------------------------------------*/
void kalTimeoutHandler(TimerHandle_t xTimer)
{

    struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) pvTimerGetTimerID(xTimer);
    EventBits_t ret;

    ret = xEventGroupGetBits(g_init_wait);
    if (!(ret & WF_THREAD_INIT_DONE_BIT)) {
        DBGLOG(INIT, ERROR, "main thread not ready yet\n");
        return;
    }
    xEventGroupSetBits(prGlueInfo->event_main_thread, GLUE_FLAG_TIMEOUT);
}

ATTR_TEXT_IN_SYSRAM
void kalSetEvent(struct GLUE_INFO *pr)
{
    EventBits_t ret;

    ret = xEventGroupGetBits(g_init_wait);
    if (!(ret & WF_THREAD_INIT_DONE_BIT)) {
        DBGLOG(INIT, ERROR, "main thread not ready yet\n");
        return;
    }

    xEventGroupSetBits(pr->event_main_thread, GLUE_FLAG_TXREQ);
}

ATTR_TEXT_IN_SYSRAM
void kalSetIntEvent(struct GLUE_INFO *pr)
{
    EventBits_t ret;
    BaseType_t xHigherPriTaskWoken = pdFALSE;
    BaseType_t xResult;

    if (glGetBusIntInIRQ(pr))
        ret = xEventGroupGetBitsFromISR(g_init_wait);
    else
        ret = xEventGroupGetBits(g_init_wait);

    if (!(ret & WF_THREAD_INIT_DONE_BIT)) {
        DBGLOG(INIT, WARN, "main thread not ready yet\n");
        return;
    }

    if (glGetBusIntInIRQ(pr)) {
        /* Set bit 0 and bit 4 in xEventGroup. */
        xResult = xEventGroupSetBitsFromISR(
                      pr->event_main_thread, /* The event group updated. */
                      GLUE_FLAG_INT, /* The bits being set. */
                      &xHigherPriTaskWoken);
        /* Was the message posted successfully? */
        /* If xHigherPriTaskWoken is now set to pdTRUE then a context
         * switch should be requested.  The macro used is port
         * specific and will be either portYIELD_FROM_ISR()
         * or portEND_SWITCHING_ISR() - refer to
         * the documentation page for the port being used.
         */
        if (xResult != pdFAIL)
            portYIELD_FROM_ISR(xHigherPriTaskWoken);
    } else
        xEventGroupSetBits(pr->event_main_thread, GLUE_FLAG_INT);
}


void kalReleasePrivilegeCH(IN struct GLUE_INFO *pr)
{
    aisFsmReleaseCh(pr->prAdapter, AIS_DEFAULT_INDEX);
}

void kalSetWmmUpdateEvent(struct GLUE_INFO *pr)
{
    EventBits_t ret;

    ret = xEventGroupGetBits(g_init_wait);
    if (!(ret & WF_THREAD_INIT_DONE_BIT)) {
        DBGLOG(INIT, WARN, "main thread not ready yet\n");
        return;
    }
    xEventGroupSetBits(pr->event_main_thread, GLUE_FLAG_UPDATE_WMM_QUOTA);

}

void kalSetHifDbgEvent(struct GLUE_INFO *pr)
{
#if CFG_SUPPORT_MULTITHREAD
    EventBits_t ret;

    ret = xEventGroupGetBits(g_init_wait);
    if (!(ret & WF_THREAD_INIT_DONE_BIT)) {
        DBGLOG(INIT, WARN, "main thread not ready yet\n");
        return;
    }
    xEventGroupSetBits(pr->event_main_thread, GLUE_FLAG_HIF_PRT_HIF_DBG_INFO);
#endif /* #if CFG_SUPPORT_MULTITHREAD */
}

#if CFG_SUPPORT_MULTITHREAD
void kalSetTxEvent2Hif(struct GLUE_INFO *pr)
{
    if (!pr->hif_thread)
        return;

    KAL_WAKE_LOCK_TIMEOUT(pr->prAdapter, &pr->rTimeoutWakeLock,
                          MSEC_TO_JIFFIES(
                              pr->prAdapter->rWifiVar.u4WakeLockThreadWakeup));

    set_bit(GLUE_FLAG_HIF_TX_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq_hif);
}

void kalSetFwOwnEvent2Hif(struct GLUE_INFO *pr)
{
    if (!pr->hif_thread)
        return;

    KAL_WAKE_LOCK_TIMEOUT(pr->prAdapter, &pr->rTimeoutWakeLock,
                          MSEC_TO_JIFFIES(
                              pr->prAdapter->rWifiVar.u4WakeLockThreadWakeup));

    set_bit(GLUE_FLAG_HIF_FW_OWN_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq_hif);
}

void kalSetTxEvent2Rx(struct GLUE_INFO *pr)
{
    if (!pr->rx_thread)
        return;

    KAL_WAKE_LOCK_TIMEOUT(pr->prAdapter, &pr->rTimeoutWakeLock,
                          MSEC_TO_JIFFIES(
                              pr->prAdapter->rWifiVar.u4WakeLockThreadWakeup));

    set_bit(GLUE_FLAG_RX_TO_OS_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq_rx);
}

void kalSetTxCmdEvent2Hif(struct GLUE_INFO *pr)
{
    if (!pr->hif_thread)
        return;

    KAL_WAKE_LOCK_TIMEOUT(pr->prAdapter, &pr->rTimeoutWakeLock,
                          MSEC_TO_JIFFIES(
                              pr->prAdapter->rWifiVar.u4WakeLockThreadWakeup));

    set_bit(GLUE_FLAG_HIF_TX_CMD_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq_hif);
}

void kalSetTxCmdDoneEvent(struct GLUE_INFO *pr)
{
    /* do we need wake lock here */
    set_bit(GLUE_FLAG_TX_CMD_DONE_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq);
}

void kalSetRxProcessEvent(struct GLUE_INFO *pr)
{
    /* do we need wake lock here ? */
    set_bit(GLUE_FLAG_RX_BIT, &pr->ulFlag);
    wake_up_interruptible(&pr->waitq);
}
#endif /* #if CFG_SUPPORT_MULTITHREAD */
/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if configuration file (NVRAM/Registry) exists
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
uint8_t kalIsConfigurationExist(IN struct GLUE_INFO
                                *prGlueInfo)
{
    ASSERT(prGlueInfo);

    return prGlueInfo->fgNvramAvailable;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to retrieve Registry information
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           Pointer of REG_INFO_T
 */
/*----------------------------------------------------------------------------*/
struct REG_INFO *kalGetConfiguration(IN struct GLUE_INFO
                                     *prGlueInfo)
{
    ASSERT(prGlueInfo);

    return &(prGlueInfo->rRegInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief update RSSI and LinkQuality to GLUE layer
 *
 * \param[in]
 *           prGlueInfo
 *           eNetTypeIdx
 *           cRssi
 *           cLinkQuality
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
void kalUpdateRSSI(IN struct GLUE_INFO *prGlueInfo,
                   IN enum ENUM_KAL_NETWORK_TYPE_INDEX eNetTypeIdx,
                   IN int8_t cRssi, IN int8_t cLinkQuality)
{
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Pre-allocate I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
uint8_t kalInitIOBuffer(uint8_t is_pre_alloc)
{
    uint32_t u4Size;

    /* not pre-allocation for all memory usage */
    if (!is_pre_alloc) {
        pvIoBuffer = NULL;
        return FALSE;
    }

    /* pre-allocation for all memory usage */
    if (HIF_TX_COALESCING_BUFFER_SIZE >
        HIF_RX_COALESCING_BUFFER_SIZE)
        u4Size = HIF_TX_COALESCING_BUFFER_SIZE;
    else
        u4Size = HIF_RX_COALESCING_BUFFER_SIZE;

    u4Size += HIF_EXTRA_IO_BUFFER_SIZE;

    pvIoBuffer = kmalloc(u4Size);
    if (pvIoBuffer) {
        pvIoBufferSize = u4Size;
        pvIoBufferUsage = 0;

        return TRUE;
    }

    return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Free pre-allocated I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalUninitIOBuffer(void)
{
    kfree(pvIoBuffer);

    pvIoBuffer = (void *) NULL;
    pvIoBufferSize = 0;
    pvIoBufferUsage = 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Dispatch pre-allocated I/O buffer
 *
 * \param[in]
 *           u4AllocSize
 *
 * \return
 *           PVOID for pointer of pre-allocated I/O buffer
 */
/*----------------------------------------------------------------------------*/
void *kalAllocateIOBuffer(IN uint32_t u4AllocSize)
{
    void *ret = (void *) NULL;

    if (pvIoBuffer) {
        if (u4AllocSize <= (pvIoBufferSize - pvIoBufferUsage)) {
            ret = (void *)
                  & (((uint8_t *)(pvIoBuffer))[pvIoBufferUsage]);
            pvIoBufferUsage += u4AllocSize;
        }
    } else {
        /* fault tolerance */
        ret = (void *) kmalloc(u4AllocSize);
    }

    return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Release all dispatched I/O buffer
 *
 * \param[in]
 *           none
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalReleaseIOBuffer(IN void *pvAddr, IN uint32_t u4Size)
{
    if (pvIoBuffer) {
        pvIoBufferUsage -= u4Size;
    } else {
        /* fault tolerance */
        kalMemFree(pvAddr, PHY_MEM_TYPE, u4Size);
    }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void kalGetChannelList(IN struct GLUE_INFO *prGlueInfo,
                       IN enum ENUM_BAND eSpecificBand,
                       IN uint8_t ucMaxChannelNum, IN uint8_t *pucNumOfChannel,
                       IN struct RF_CHANNEL_INFO *paucChannelList)
{
    rlmDomainGetChnlList(prGlueInfo->prAdapter, eSpecificBand,
                         FALSE,
                         ucMaxChannelNum, pucNumOfChannel, paucChannelList);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
uint8_t kalIsAPmode(IN struct GLUE_INFO *prGlueInfo)
{
    return FALSE;
}


#if CFG_SUPPORT_802_11W
/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if the MFP is DISABLD/OPTIONAL/REQUIRED
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *   RSN_AUTH_MFP_DISABLED
 *   RSN_AUTH_MFP_OPTIONAL
 *   RSN_AUTH_MFP_DISABLED
 */
/*----------------------------------------------------------------------------*/
uint32_t kalGetMfpSetting(IN struct GLUE_INFO *prGlueInfo,
                          IN uint8_t ucBssIndex)
{
    uint32_t u4RsnMfp = RSN_AUTH_MFP_DISABLED;
    struct GL_WPA_INFO *prWpaInfo;

    prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter,
                              ucBssIndex);

    ASSERT(prGlueInfo);

    switch (prWpaInfo->u4Mfp) {
        case IW_AUTH_MFP_DISABLED:
            u4RsnMfp = RSN_AUTH_MFP_DISABLED;
            break;
        case IW_AUTH_MFP_OPTIONAL:
            u4RsnMfp = RSN_AUTH_MFP_OPTIONAL;
            break;
        case IW_AUTH_MFP_REQUIRED:
            u4RsnMfp = RSN_AUTH_MFP_REQUIRED;
            break;
        default:
            u4RsnMfp = RSN_AUTH_MFP_DISABLED;
            break;
    }

    return u4RsnMfp;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief to check if the RSN IE CAP setting from supplicant
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
uint8_t kalGetRsnIeMfpCap(IN struct GLUE_INFO *prGlueInfo,
                          IN uint8_t ucBssIndex)
{
    struct GL_WPA_INFO *prWpaInfo;

    ASSERT(prGlueInfo);

    prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter,
                              ucBssIndex);

    return prWpaInfo->ucRSNMfpCap;
}
/*----------------------------------------------------------------------------*/
/*!
* \brief to get group mgmt cipher from supplicant
*
* \param[in]
*           prGlueInfo
*
* \return
*           u4CipherGroupMgmt
*
*/
/*----------------------------------------------------------------------------*/
uint32_t kalGetRsnIeGroupMgmt(IN struct GLUE_INFO *prGlueInfo,
                              IN uint8_t ucBssIndex)
{
    ASSERT(prGlueInfo);

    return prGlueInfo->rWpaInfo[ucBssIndex].u4CipherGroupMgmt;
}

#endif /* #if CFG_SUPPORT_802_11W */
/*----------------------------------------------------------------------------*/
/*!
 * \brief read request firmware file binary to pucData
 *
 * \param[in] pucPath  file name
 * \param[out] pucData  Request file output buffer
 * \param[in] u4Size  read size
 * \param[out] pu4ReadSize  real read size
 * \param[in] dev
 *
 * \return
 *           0 success
 *           >0 fail
 */
/*----------------------------------------------------------------------------*/
int32_t kalRequestFirmware(const uint8_t *pucPath,
                           uint8_t *pucData, uint32_t u4Size,
                           uint32_t *pu4ReadSize, void *dev)
{
    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate BSS-INFO to NL80211 as scanning result
 *
 * \param[in]
 *           prGlueInfo
 *           pucBeaconProbeResp
 *           u4FrameLen
 *
 *
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalIndicateBssInfo(IN struct GLUE_INFO *prGlueInfo,
                        IN uint8_t *pucBeaconProbeResp,
                        IN uint32_t u4FrameLen, IN uint8_t ucChannelNum,
                        IN int32_t i4SignalStrength)
{
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate channel ready
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalReadyOnChannel(IN struct GLUE_INFO *prGlueInfo,
                       IN uint64_t uint8_tCookie,
                       IN enum ENUM_BAND eBand, IN enum ENUM_CHNL_EXT eSco,
                       IN uint8_t ucChannelNum, IN uint32_t u4DurationMs,
                       IN uint8_t ucBssIndex)
{
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate channel expiration
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalRemainOnChannelExpired(IN struct GLUE_INFO *prGlueInfo,
                               IN uint64_t uint8_tCookie, IN enum ENUM_BAND eBand,
                               IN enum ENUM_CHNL_EXT eSco, IN uint8_t ucChannelNum,
                               IN uint8_t ucBssIndex)
{
}

void kalIndicateAssocReject(IN uint8_t *aucMacAddr, IN uint16_t usStatusCode)
{
    unsigned char pkt[512] = {0};
    iwreq_data_t    *iwreq;
    uint32_t        *cmd;
    /* event return value */
    int             evt_ret;
    uint8_t *bssid;
    uint16_t *status;

    iwreq = (iwreq_data_t *)&pkt[0];
    cmd   = (uint32_t *)&pkt[sizeof(*iwreq)];
    bssid = (uint8_t *)&pkt[sizeof(*iwreq) + sizeof(uint32_t)];
    status = (uint16_t *)&pkt[sizeof(*iwreq) + sizeof(uint32_t) + 6];

    *cmd               = IW_CUSTOM_EVENT_FLAG;
    iwreq->data.flags  = IW_ASSOC_REJECT_EVENT_FLAG;
    iwreq->data.length = 8;

    kalMemCopy(bssid, aucMacAddr, ETH_ALEN);
    *status = usStatusCode;

    evt_ret = wifi_evt_handler_gen4m(0, pkt,
                                     sizeof(*iwreq) + sizeof(uint32_t) + ETH_ALEN + 2);

    /* TODO: if not handled, what should we do? */
    if (evt_ret)
        DBGLOG(INIT, ERROR, "[wifi] assoc reject not handled!\n");
}               /* kalIndicateAssocReject */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate Mgmt tx status
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalIndicateMgmtTxStatus(IN struct GLUE_INFO *prGlueInfo,
                             IN uint64_t uint8_tCookie, IN uint8_t fgIsAck,
                             IN uint8_t *pucFrameBuf, IN uint32_t u4FrameLen,
                             IN uint8_t ucBssIndex)
{
    unsigned char pkt[512] = {0};
    iwreq_data_t    *iwreq;
    uint32_t        *cmd;
    /* event return value */
    int             evt_ret;
    uint8_t *ack;
    uint32_t *len;
    uint8_t *buf;

    DBGLOG(AIS, INFO, "ack %d, len %d\n", fgIsAck, u4FrameLen);

    iwreq = (iwreq_data_t *)&pkt[0];
    cmd   = (uint32_t *)&pkt[sizeof(*iwreq)];
    ack = &pkt[sizeof(*iwreq) + sizeof(uint32_t)];
    len = (uint32_t *)&pkt[sizeof(*iwreq) + sizeof(uint32_t) + 1];
    buf = &pkt[sizeof(*iwreq) + sizeof(uint32_t) + 1 + 4];

    *cmd               = IW_CUSTOM_EVENT_FLAG;
    iwreq->data.flags  = IW_TX_STATUS_EVENT_FLAG;
    iwreq->data.length = 5 + u4FrameLen;

    *ack = fgIsAck;
    *len = u4FrameLen;
    kalMemCopy(buf, pucFrameBuf, u4FrameLen);

    evt_ret = wifi_evt_handler_gen4m(0, pkt,
                                     sizeof(*iwreq) + sizeof(uint32_t) + 5 + u4FrameLen);

    /* TODO: if not handled, what should we do? */
    if (evt_ret)
        DBGLOG(INIT, ERROR, "[wifi] Tx status not handled!\n");
}               /* kalIndicateMgmtTxStatus */

void kalIndicateRxMgmtFrame(IN struct GLUE_INFO *prGlueInfo,
                            IN struct SW_RFB *prSwRfb,
                            IN uint8_t ucBssIndex)
{
    int32_t i4Freq = 0;
    uint8_t ucChnlNum = 0;
    int32_t sig_dbm;

    unsigned char pkt[512] = {0};
    iwreq_data_t    *iwreq;
    uint32_t        *cmd;
    /* event return value */
    int             evt_ret;
    uint32_t *len;
    uint8_t *buf;
    uint32_t *freq;
    uint32_t *rssi;

    if ((prGlueInfo == NULL) || (prSwRfb == NULL))
        ASSERT(FALSE);

    ucChnlNum = prSwRfb->ucChnlNum;
    i4Freq = nicChannelNum2Freq(ucChnlNum) / 1000;

    sig_dbm = RCPI_TO_dBm((uint8_t) nicRxGetRcpiValueFromRxv(
                              prGlueInfo->prAdapter,
                              RCPI_MODE_MAX, prSwRfb));

    iwreq = (iwreq_data_t *)&pkt[0];
    cmd   = (uint32_t *)&pkt[sizeof(*iwreq)];
    freq  = (uint32_t *)&pkt[sizeof(*iwreq) + 4];
    rssi = (uint32_t *)&pkt[sizeof(*iwreq) + 8];
    len = (uint32_t *)&pkt[sizeof(*iwreq) + 12];
    buf = &pkt[sizeof(*iwreq) + 16];

    *cmd               = IW_CUSTOM_EVENT_FLAG;
    iwreq->data.flags  = IW_RX_MGMT_EVENT_FLAG;
    iwreq->data.length = 12 + prSwRfb->u2PacketLen;

    *freq = i4Freq;
    *rssi = sig_dbm;
    *len = prSwRfb->u2PacketLen;
    kalMemCopy(buf, prSwRfb->pvHeader,  prSwRfb->u2PacketLen);

    DBGLOG(RX, INFO, "signal %d, freq %d prSwRfb->u2PacketLen %d\n",
           sig_dbm, i4Freq, prSwRfb->u2PacketLen);

    evt_ret = wifi_evt_handler_gen4m(0, pkt,
                                     sizeof(*iwreq) + 16 + prSwRfb->u2PacketLen);

    /* TODO: if not handled, what should we do? */
    if (evt_ret)
        DBGLOG(INIT, INFO, "[wifi] Rx mgmt not handled!\n");
}               /* kalIndicateRxMgmtFrame */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate supplicant that a BSS
 *           has been connected
 *
 * \param[in]
 *           none
 *
 * \return
 *           none
 */
/*----------------------------------------------------------------------------*/
void kalIndicateBssConnected(void)
{
    /* + WIFI_MAC_ADDRESS_LENGTH to prevent coverity issue */
    unsigned char pkt[sizeof(iwreq_data_t) + sizeof(uint32_t) + WIFI_MAC_ADDRESS_LENGTH] = {0};
    iwreq_data_t *iwreq;
    uint32_t *cmd;
    int evt_ret;

    /* notify supplicant for
     * beacon received
     */
    iwreq = (iwreq_data_t *)&pkt[0];
    cmd = (uint32_t *)&pkt[sizeof(*iwreq)];

    *cmd = IW_CUSTOM_EVENT_FLAG;
    iwreq->data.flags = IW_SET_LISTEN_EVENT_FLAG;

    evt_ret = wifi_evt_handler_gen4m(0, pkt,
                                     sizeof(*iwreq) +
                                     sizeof(uint32_t));
    if (evt_ret)
        DBGLOG(SCN, ERROR,
               "[wifi] set listen not handled!\n");
}               /* kalIndicateMgmtTxStatus */


#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
/*----------------------------------------------------------------------------*/
/*!
 * \brief    To configure SDIO test pattern mode
 *
 * \param[in]
 *           prGlueInfo
 *           fgEn
 *           fgRead
 *
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
uint8_t kalSetSdioTestPattern(IN struct GLUE_INFO
                              *prGlueInfo, IN uint8_t fgEn, IN uint8_t fgRead)
{
    const uint8_t aucPattern[] = {
        0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
        0xaa, 0x55, 0x80, 0x80, 0x80, 0x7f, 0x80, 0x80,
        0x80, 0x7f, 0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x7f,
        0x40, 0x40, 0x40, 0xbf, 0x40, 0x40, 0x40, 0xbf,
        0xbf, 0xbf, 0x40, 0xbf, 0xbf, 0xbf, 0x20, 0x20,
        0x20, 0xdf, 0x20, 0x20, 0x20, 0xdf, 0xdf, 0xdf,
        0x20, 0xdf, 0xdf, 0xdf, 0x10, 0x10, 0x10, 0xef,
        0x10, 0x10, 0x10, 0xef, 0xef, 0xef, 0x10, 0xef,
        0xef, 0xef, 0x08, 0x08, 0x08, 0xf7, 0x08, 0x08,
        0x08, 0xf7, 0xf7, 0xf7, 0x08, 0xf7, 0xf7, 0xf7,
        0x04, 0x04, 0x04, 0xfb, 0x04, 0x04, 0x04, 0xfb,
        0xfb, 0xfb, 0x04, 0xfb, 0xfb, 0xfb, 0x02, 0x02,
        0x02, 0xfd, 0x02, 0x02, 0x02, 0xfd, 0xfd, 0xfd,
        0x02, 0xfd, 0xfd, 0xfd, 0x01, 0x01, 0x01, 0xfe,
        0x01, 0x01, 0x01, 0xfe, 0xfe, 0xfe, 0x01, 0xfe,
        0xfe, 0xfe, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
        0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
        0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff,
        0x00, 0x00, 0x00, 0xff
    };
    uint32_t i;

    ASSERT(prGlueInfo);

    /* access to MCR_WTMCR to engage PRBS mode */
    prGlueInfo->fgEnSdioTestPattern = fgEn;
    prGlueInfo->fgSdioReadWriteMode = fgRead;

    if (fgRead == FALSE) {
        /* fill buffer for data to be written */
        for (i = 0; i < sizeof(aucPattern); i++)
            prGlueInfo->aucSdioTestBuffer[i] = aucPattern[i];
    }

    return TRUE;
}
#endif /* #if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN */

#if (CFG_MET_PACKET_TRACE_SUPPORT == 1)
#define PROC_MET_PROF_CTRL                 "met_ctrl"
#define PROC_MET_PROF_PORT                 "met_port"

void *pMetGlobalData;

#endif /* #if (CFG_MET_PACKET_TRACE_SUPPORT == 1) */
/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate scheduled scan results are avilable
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
void kalSchedScanResults(IN struct GLUE_INFO *prGlueInfo)
{
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate scheduled scan has been stopped
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
void kalSchedScanStopped(IN struct GLUE_INFO *prGlueInfo,
                         uint8_t fgDriverTriggerd)
{
}

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
/*----------------------------------------------------------------------------*/
/*!
 * \brief    To check if device if wake up by wlan
 *
 * \param[in]
 *           prAdapter
 *
 * \return
 *           TRUE: wake up by wlan; otherwise, FALSE
 */
/*----------------------------------------------------------------------------*/
uint8_t kalIsWakeupByWlan(struct ADAPTER *prAdapter)
{
    return prAdapter->fgIsWowSuspend;
}
#endif /* #if CFG_SUPPORT_WAKEUP_REASON_DEBUG */



uint8_t kalGetIPv4Address(IN struct net_device *prDev,
                          IN uint32_t u4MaxNumOfAddr, OUT uint8_t *pucIpv4Addrs,
                          OUT uint32_t *pu4NumOfIpv4Addr)
{
    uint32_t u4NumIPv4 = 0;
    uint32_t u4AddrLen = IPV4_ADDR_LEN;
    struct netif *prNetif;

    /* 4 <1> Sanity check of netDevice */
    if (!prDev || !(prDev->netif)) {
        DBGLOG(INIT, INFO,
               "IPv4 address is not available for dev(0x%p)\n", prDev);

        *pu4NumOfIpv4Addr = 0;
        return FALSE;
    }

    prNetif = (struct netif *)prDev->netif;

#if LWIP_DHCP
    if (lwip_get_ipmode() == REQ_IP_MODE_DHCP) {
        DBGLOG(INIT, INFO, "mode: dhcp\n");
        if (dhcp_supplied_address(prNetif)) {
            struct dhcp *d = netif_dhcp_data(prNetif);

            DBGLOG(INIT, INFO, "ip: %s\n",
                   ip4addr_ntoa(&d->offered_ip_addr));
            DBGLOG(INIT, INFO, "netmask: %s\n",
                   ip4addr_ntoa(&d->offered_sn_mask));
            DBGLOG(INIT, INFO, "gateway: %s\n",
                   ip4addr_ntoa(&d->offered_gw_addr));

            /* 4 <2> copy the IPv4 address  struct ip4_addr*/

            kalMemCopy(&pucIpv4Addrs[u4NumIPv4 * u4AddrLen],
                       &d->offered_ip_addr.addr, u4AddrLen);
            kalMemCopy(&pucIpv4Addrs[(u4NumIPv4 + 1) * u4AddrLen],
                       &d->offered_sn_mask.addr, u4AddrLen);

            u4NumIPv4++;

        } else {
            DBGLOG(INIT, INFO, "dhcp on going\n");
            return FALSE;
        }
    } else
#endif /* #if LWIP_DHCP */
    {
        DBGLOG(INIT, INFO, "mode: static\n");
        DBGLOG(INIT, INFO, "ip: %s\n",
               ip4addr_ntoa(ip_2_ip4(&prNetif->ip_addr)));
        DBGLOG(INIT, INFO, "netmask: %s\n",
               ip4addr_ntoa(ip_2_ip4(&prNetif->netmask)));
        DBGLOG(INIT, INFO, "gateway: %s\n",
               ip4addr_ntoa(ip_2_ip4(&prNetif->gw)));

        /* 4 <2> copy the IPv4 address */

        kalMemCopy(&pucIpv4Addrs[u4NumIPv4 * u4AddrLen],
                   &(ip_2_ip4(&prNetif->ip_addr)->addr), u4AddrLen);
        kalMemCopy(&pucIpv4Addrs[(u4NumIPv4 + 1) * u4AddrLen],
                   &(ip_2_ip4(&prNetif->netmask)->addr), u4AddrLen);

        u4NumIPv4++;
    }

    *pu4NumOfIpv4Addr = u4NumIPv4;

    return TRUE;
}

void kalSetNetAddress(IN struct GLUE_INFO *prGlueInfo,
                      IN uint8_t ucBssIdx,
                      IN uint8_t *pucIPv4Addr, IN uint32_t u4NumIPv4Addr,
                      IN uint8_t *pucIPv6Addr, IN uint32_t u4NumIPv6Addr)
{
    uint32_t rStatus = WLAN_STATUS_FAILURE;
    uint32_t u4SetInfoLen = 0;
    uint32_t u4Len = OFFSET_OF(struct
                               PARAM_NETWORK_ADDRESS_LIST, arAddress);
    struct PARAM_NETWORK_ADDRESS_LIST *prParamNetAddrList;
    struct PARAM_NETWORK_ADDRESS *prParamNetAddr;
    uint32_t i, u4AddrLen;

    /* 4 <1> Calculate buffer size */
    /* IPv4 */
    u4Len += (((sizeof(struct PARAM_NETWORK_ADDRESS) - 1) +
               IPV4_ADDR_LEN) * u4NumIPv4Addr * 2);
    /* IPv6 */
    u4Len += (((sizeof(struct PARAM_NETWORK_ADDRESS) - 1) +
               IPV6_ADDR_LEN) * u4NumIPv6Addr);

    /* 4 <2> Allocate buffer */
    prParamNetAddrList = (struct PARAM_NETWORK_ADDRESS_LIST *)
                         kmalloc(u4Len);

    if (!prParamNetAddrList) {
        DBGLOG(INIT, WARN,
               "Fail to alloc buffer for setting BSS[%u] network address!\n",
               ucBssIdx);
        return;
    }
    /* 4 <3> Fill up network address */
    prParamNetAddrList->u2AddressType =
        PARAM_PROTOCOL_ID_TCP_IP;
    prParamNetAddrList->u4AddressCount = 0;
    prParamNetAddrList->ucBssIdx = ucBssIdx;

    /* 4 <3.1> Fill up IPv4 address */
    u4AddrLen = IPV4_ADDR_LEN;
    prParamNetAddr = prParamNetAddrList->arAddress;
    for (i = 0; i < u4NumIPv4Addr; i++) {
        prParamNetAddr->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
        prParamNetAddr->u2AddressLength = u4AddrLen;
        kalMemCopy(prParamNetAddr->aucAddress,
                   &pucIPv4Addr[i * u4AddrLen * 2], u4AddrLen * 2);

        prParamNetAddr = (struct PARAM_NETWORK_ADDRESS *)
                         ((unsigned long) prParamNetAddr +
                          (unsigned long)(u4AddrLen * 2 +
                                          OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress)));
    }
    prParamNetAddrList->u4AddressCount += u4NumIPv4Addr;

    /* 4 <3.2> Fill up IPv6 address */
    u4AddrLen = IPV6_ADDR_LEN;
    for (i = 0; i < u4NumIPv6Addr; i++) {
        prParamNetAddr->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
        prParamNetAddr->u2AddressLength = u4AddrLen;
        kalMemCopy(prParamNetAddr->aucAddress,
                   &pucIPv6Addr[i * u4AddrLen], u4AddrLen);

        prParamNetAddr = (struct PARAM_NETWORK_ADDRESS *)((
                                                              unsigned long) prParamNetAddr + (unsigned long)(
                                                              u4AddrLen + OFFSET_OF(
                                                                  struct PARAM_NETWORK_ADDRESS, aucAddress)));
    }
    prParamNetAddrList->u4AddressCount += u4NumIPv6Addr;

    /* 4 <4> IOCTL to main_thread */
    rStatus = kalIoctl(prGlueInfo,
                       wlanoidSetNetworkAddress,
                       (void *) prParamNetAddrList, u4Len,
                       FALSE, FALSE, TRUE, &u4SetInfoLen);

    if (rStatus != WLAN_STATUS_SUCCESS)
        DBGLOG(REQ, WARN, "%s: Fail to set network address\n",
               __func__);

    kalMemFree(prParamNetAddrList, VIR_MEM_TYPE, u4Len);

}

void kalSetNetAddressFromInterface(IN struct GLUE_INFO
                                   *prGlueInfo, IN struct net_device *prDev, IN uint8_t fgSet)
{
    uint32_t u4NumIPv4, u4NumIPv6;
    uint8_t pucIPv4Addr[IPV4_ADDR_LEN * CFG_PF_ARP_NS_MAX_NUM * 2];
    uint8_t pucIPv6Addr[IPV6_ADDR_LEN * CFG_PF_ARP_NS_MAX_NUM];

    u4NumIPv4 = 0;
    u4NumIPv6 = 0;

    if (fgSet) {
        kalGetIPv4Address(prDev, CFG_PF_ARP_NS_MAX_NUM, pucIPv4Addr,
                          &u4NumIPv4);
        kalGetIPv6Address(prDev, CFG_PF_ARP_NS_MAX_NUM, pucIPv6Addr,
                          &u4NumIPv6);
    }

    if (u4NumIPv4 + u4NumIPv6 > CFG_PF_ARP_NS_MAX_NUM) {
        if (u4NumIPv4 >= CFG_PF_ARP_NS_MAX_NUM) {
            u4NumIPv4 = CFG_PF_ARP_NS_MAX_NUM;
            u4NumIPv6 = 0;
        } else {
            u4NumIPv6 = CFG_PF_ARP_NS_MAX_NUM - u4NumIPv4;
        }
    }

    DBGLOG(INIT, INFO, "SetNetAddres bss_idx %d\n", prDev->bss_idx);
    kalSetNetAddress(prGlueInfo, prDev->bss_idx,
                     pucIPv4Addr, u4NumIPv4, pucIPv6Addr, u4NumIPv6);
}

#if CFG_MET_PACKET_TRACE_SUPPORT

uint8_t kalMetCheckProfilingPacket(IN struct GLUE_INFO
                                   *prGlueInfo, IN void *prPacket)
{
    uint32_t u4PacketLen;
    uint16_t u2EtherTypeLen;
    struct sk_buff *prSkb = (struct sk_buff *)prPacket;
    uint8_t *aucLookAheadBuf = NULL;
    uint8_t ucEthTypeLenOffset = ETHER_HEADER_LEN -
                                 ETHER_TYPE_LEN;
    uint8_t *pucNextProtocol = NULL;

    u4PacketLen = prSkb->len;

    if (u4PacketLen < ETHER_HEADER_LEN) {
        DBGLOG(INIT, WARN, "Invalid Ether packet length: %u\n",
               u4PacketLen);
        return FALSE;
    }

    aucLookAheadBuf = prSkb->data;

    /* 4 <0> Obtain Ether Type/Len */
    WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ucEthTypeLenOffset],
                        &u2EtherTypeLen);

    /* 4 <1> Skip 802.1Q header (VLAN Tagging) */
    if (u2EtherTypeLen == ETH_P_VLAN) {
        ucEthTypeLenOffset += ETH_802_1Q_HEADER_LEN;
        WLAN_GET_FIELD_BE16(&aucLookAheadBuf[ucEthTypeLenOffset],
                            &u2EtherTypeLen);
    }
    /* 4 <2> Obtain next protocol pointer */
    pucNextProtocol = &aucLookAheadBuf[ucEthTypeLenOffset +
                                                          ETHER_TYPE_LEN];

    /* 4 <3> Handle ethernet format */
    switch (u2EtherTypeLen) {

        /* IPv4 */
        case ETH_P_IPV4: {
                uint8_t *pucIpHdr = pucNextProtocol;
                uint8_t ucIpVersion;

                /* IPv4 header length check */
                if (u4PacketLen < (ucEthTypeLenOffset + ETHER_TYPE_LEN +
                                   IPV4_HDR_LEN)) {
                    DBGLOG(INIT, WARN, "Invalid IPv4 packet length: %u\n",
                           u4PacketLen);
                    return FALSE;
                }

                /* IPv4 version check */
                ucIpVersion = (pucIpHdr[0] & IP_VERSION_MASK) >>
                              IP_VERSION_OFFSET;
                if (ucIpVersion != IP_VERSION_4) {
                    DBGLOG(INIT, WARN, "Invalid IPv4 packet version: %u\n",
                           ucIpVersion);
                    return FALSE;
                }

                if (pucIpHdr[IPV4_HDR_IP_PROTOCOL_OFFSET] == IP_PRO_UDP) {
                    uint8_t *pucUdpHdr = &pucIpHdr[IPV4_HDR_LEN];
                    uint16_t u2UdpDstPort;
                    uint16_t u2UdpSrcPort;

                    /* Get UDP DST port */
                    WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_DST_PORT_OFFSET],
                                        &u2UdpDstPort);

                    /* Get UDP SRC port */
                    WLAN_GET_FIELD_BE16(&pucUdpHdr[UDP_HDR_SRC_PORT_OFFSET],
                                        &u2UdpSrcPort);

                    if (u2UdpSrcPort == prGlueInfo->u2MetUdpPort) {
                        uint16_t u2IpId;

                        /* Store IP ID for Tag */
                        WLAN_GET_FIELD_BE16(
                            &pucIpHdr[IPV4_HDR_IP_IDENTIFICATION_OFFSET],
                            &u2IpId);

                        GLUE_SET_PKT_IP_ID(prPacket, u2IpId);

                        return TRUE;
                    }
                }
            }
            break;

        default:
            break;
    }

    return FALSE;
}

static unsigned long __read_mostly tracing_mark_write_addr;

static int __mt_find_tracing_mark_write_symbol_fn(
    void *prData, const char *pcNameBuf,
    struct module *prModule, unsigned long ulAddress)
{
    if (strcmp(pcNameBuf, "tracing_mark_write") == 0) {
        tracing_mark_write_addr = ulAddress;
        return 1;
    }
    return 0;
}

static inline void __mt_update_tracing_mark_write_addr(void)
{
    if (unlikely(tracing_mark_write_addr == 0))
        kallsyms_on_each_symbol(
            __mt_find_tracing_mark_write_symbol_fn, NULL);
}

void kalMetTagPacket(IN struct GLUE_INFO *prGlueInfo,
                     IN void *prPacket, IN enum ENUM_TX_PROFILING_TAG eTag)
{
    if (!prGlueInfo->fgMetProfilingEn)
        return;

    switch (eTag) {
        case TX_PROF_TAG_OS_TO_DRV:
            if (kalMetCheckProfilingPacket(prGlueInfo, prPacket)) {
                /* trace_printk("S|%d|%s|%d\n", current->pid,
                 * "WIFI-CHIP", GLUE_GET_PKT_IP_ID(prPacket));
                 */
                __mt_update_tracing_mark_write_addr();
                GLUE_SET_PKT_FLAG_PROF_MET(prPacket);
            }
            break;

        case TX_PROF_TAG_DRV_TX_DONE:
            if (GLUE_GET_PKT_IS_PROF_MET(prPacket)) {
                /* trace_printk("F|%d|%s|%d\n", current->pid,
                 * "WIFI-CHIP", GLUE_GET_PKT_IP_ID(prPacket));
                 */
                __mt_update_tracing_mark_write_addr();
            }
            break;

        case TX_PROF_TAG_MAC_TX_DONE:
            break;

        default:
            break;
    }
}

void kalMetInit(IN struct GLUE_INFO *prGlueInfo)
{
    prGlueInfo->fgMetProfilingEn = FALSE;
    prGlueInfo->u2MetUdpPort = 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief The PROC function for adjusting Debug Level to turn on/off debugging
 *    message.
 *
 * \param[in] file   pointer to file.
 * \param[in] buffer Buffer from user space.
 * \param[in] count  Number of characters to write
 * \param[in] data   Pointer to the private data structure.
 *
 * \return number of characters write from User Space.
 */
/*----------------------------------------------------------------------------*/
static ssize_t kalMetCtrlWriteProcfs(struct file *file,
                                     const char __user *buffer, uint32_t count, loff_t *off)
{
    char acBuf[128 + 1];    /* + 1 for "\0" */
    uint32_t u4CopySize;
    int uint8_tMetProfEnable;
    ssize_t result;

    IN struct GLUE_INFO *prGlueInfo;

    u4CopySize = (count < (sizeof(acBuf) - 1)) ? count :
                 (sizeof(acBuf) - 1);
    result = copy_from_user(acBuf, buffer, u4CopySize);
    acBuf[u4CopySize] = '\0';

    if (sscanf(acBuf, " %d", &uint8_tMetProfEnable) == 1)
        DBGLOG(INIT, INFO, "MET_PROF: Write MET PROC Enable=%d\n",
               uint8_tMetProfEnable);
    if (pMetGlobalData != NULL) {
        prGlueInfo = (struct GLUE_INFO *) pMetGlobalData;
        prGlueInfo->fgMetProfilingEn = (uint8_t) uint8_tMetProfEnable;
    }
    return count;
}

static ssize_t kalMetPortWriteProcfs(struct file *file,
                                     const char __user *buffer, uint32_t count, loff_t *off)
{
    char acBuf[128 + 1];    /* + 1 for "\0" */
    uint32_t u4CopySize;
    int u16MetUdpPort;
    ssize_t result;

    IN struct GLUE_INFO *prGlueInfo;

    u4CopySize = (count < (sizeof(acBuf) - 1)) ? count :
                 (sizeof(acBuf) - 1);
    result = copy_from_user(acBuf, buffer, u4CopySize);
    acBuf[u4CopySize] = '\0';

    if (sscanf(acBuf, " %d", &u16MetUdpPort) == 1)
        DBGLOG(INIT, INFO, "MET_PROF: Write MET PROC UDP_PORT=%d\n",
               u16MetUdpPort);
    if (pMetGlobalData != NULL) {
        prGlueInfo = (struct GLUE_INFO *) pMetGlobalData;
        prGlueInfo->u2MetUdpPort = (uint16_t) u16MetUdpPort;
    }
    return count;
}

const struct file_operations rMetProcCtrlFops = {
    .write = kalMetCtrlWriteProcfs
};

const struct file_operations rMetProcPortFops = {
    .write = kalMetPortWriteProcfs
};

int kalMetInitProcfs(IN struct GLUE_INFO *prGlueInfo)
{
    /* struct proc_dir_entry *pMetProcDir; */
    if (init_net.proc_net == (struct proc_dir_entry *)NULL) {
        DBGLOG(INIT, INFO, "init proc fs fail: proc_net == NULL\n");
        return -ENOENT;
    }
    /*
     * Directory: Root (/proc/net/wlan0)
     */
    pMetProcDir = proc_mkdir("wlan0", init_net.proc_net);
    if (pMetProcDir == NULL)
        return -ENOENT;
    /*
     *  /proc/net/wlan0
     *  |-- met_ctrl         (PROC_MET_PROF_CTRL)
     */
    /* proc_create(PROC_MET_PROF_CTRL, 0x0644, pMetProcDir, &rMetProcFops);
     */
    proc_create(PROC_MET_PROF_CTRL, 0000, pMetProcDir,
                &rMetProcCtrlFops);
    proc_create(PROC_MET_PROF_PORT, 0000, pMetProcDir,
                &rMetProcPortFops);

    pMetGlobalData = (void *)prGlueInfo;

    return 0;
}

int kalMetRemoveProcfs(void)
{

    if (init_net.proc_net == (struct proc_dir_entry *)NULL) {
        DBGLOG(INIT, WARN,
               "remove proc fs fail: proc_net == NULL\n");
        return -ENOENT;
    }
    remove_proc_entry(PROC_MET_PROF_CTRL, pMetProcDir);
    remove_proc_entry(PROC_MET_PROF_PORT, pMetProcDir);
    /* remove root directory (proc/net/wlan0) */
    remove_proc_entry("wlan0", init_net.proc_net);
    /* clear MetGlobalData */
    pMetGlobalData = NULL;

    return 0;
}

#endif /* #if CFG_MET_PACKET_TRACE_SUPPORT */

#if CFG_SUPPORT_DATA_STALL
uint8_t kalIndicateDriverEvent(struct ADAPTER *prAdapter,
                               enum ENUM_VENDOR_DRIVER_EVENT event,
                               uint8_t ucBssIdx)
{
    struct sk_buff *skb = NULL;
    struct wiphy *wiphy;
    struct wireless_dev *wdev;
    struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

    wiphy = priv_to_wiphy(prAdapter->prGlueInfo);
    wdev = ((prAdapter->prGlueInfo)->prDevHandler)->ieee80211_ptr;

    if (!wiphy || !wdev || !prWifiVar)
        return -EINVAL;

    if (prAdapter->tmReportinterval > 0 &&
        !CHECK_FOR_TIMEOUT(kalGetTimeTick(),
                           prAdapter->tmReportinterval,
                           prWifiVar->u4ReportEventInterval * 1000)) {
        return -ETIME;
    }
    GET_CURRENT_SYSTIME(&prAdapter->tmReportinterval);

    skb = cfg80211_vendor_event_alloc(wiphy, wdev,
                                      (uint16_t)(sizeof(uint8_t) * 2),
                                      WIFI_EVENT_DRIVER_ERROR, GFP_KERNEL);

    if (!skb) {
        DBGLOG(REQ, ERROR, "%s allocate skb failed\n", __func__);
        return -ENOMEM;
    }

    if (unlikely(nla_put(skb, WIFI_ATTRIBUTE_ERROR_REASON
                         , sizeof(uint8_t), &event) < 0) ||
        unlikely(nla_put(skb, WIFI_ATTRIBUTE_BSS_INDEX
                         , sizeof(uint8_t), &ucBssIdx) < 0))
        goto nla_put_failure;

    cfg80211_vendor_event(skb, GFP_KERNEL);
    return TRUE;
nla_put_failure:
    kfree_skb(skb);
    return FALSE;
}
#endif /* #if CFG_SUPPORT_DATA_STALL */

uint64_t kalGetBootTime(void)
{
    return 0;
}

#if CFG_ASSERT_DUMP
uint32_t kalOpenCorDumpFile(uint8_t fgIsN9)
{
    /* Move open-op to kalWriteCorDumpFile(). Empty files only */
    uint32_t ret;
    uint8_t *apucFileName;

    if (fgIsN9)
        apucFileName = apucCorDumpN9FileName;
    else
        apucFileName = apucCorDumpCr4FileName;

    ret = kalTrunkPath(apucFileName);

    return (ret >= 0) ? WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE;
}

uint32_t kalWriteCorDumpFile(uint8_t *pucBuffer,
                             uint16_t u2Size, uint8_t fgIsN9)
{
    uint32_t ret;
    uint8_t *apucFileName;

    if (fgIsN9)
        apucFileName = apucCorDumpN9FileName;
    else
        apucFileName = apucCorDumpCr4FileName;

    ret = kalWriteToFile(apucFileName, TRUE, pucBuffer, u2Size);

    return (ret >= 0) ? WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE;
}

uint32_t kalCloseCorDumpFile(uint8_t fgIsN9)
{
    /* Move close-op to kalWriteCorDumpFile(). Do nothing here */

    return WLAN_STATUS_SUCCESS;
}
#endif /* #if CFG_ASSERT_DUMP */

#if CFG_SUPPORT_MULTITHREAD /* freertos */
void kalFreeTxMsduWorker(struct work_struct *work)
{
    struct GLUE_INFO *prGlueInfo;
    struct ADAPTER *prAdapter;
    struct QUE rTmpQue;
    struct QUE *prTmpQue = &rTmpQue;
    struct MSDU_INFO *prMsduInfo;

    if (g_u4HaltFlag)
        return;

    prGlueInfo = ENTRY_OF(work, struct GLUE_INFO,
                          rTxMsduFreeWork);
    prAdapter = prGlueInfo->prAdapter;

    if (prGlueInfo->ulFlag & GLUE_FLAG_HALT)
        return;

    KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);
    QUEUE_MOVE_ALL(prTmpQue, &prAdapter->rTxDataDoneQueue);
    KAL_RELEASE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);

    while (QUEUE_IS_NOT_EMPTY(prTmpQue)) {
        QUEUE_REMOVE_HEAD(prTmpQue, prMsduInfo, struct MSDU_INFO *);

        wlanTxProfilingTagMsdu(prAdapter, prMsduInfo,
                               TX_PROF_TAG_DRV_FREE_MSDU);

        nicTxFreePacket(prAdapter, prMsduInfo, FALSE);
        nicTxReturnMsduInfo(prAdapter, prMsduInfo);
    }
}

void kalFreeTxMsdu(struct ADAPTER *prAdapter,
                   struct MSDU_INFO *prMsduInfo)
{

    KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);
    QUEUE_INSERT_TAIL(&prAdapter->rTxDataDoneQueue,
                      (struct QUE_ENTRY *) prMsduInfo);
    KAL_RELEASE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);

    schedule_work(&prAdapter->prGlueInfo->rTxMsduFreeWork);
}

#endif /* #if CFG_SUPPORT_MULTITHREAD ( freertos ) */

#define PERF_UPDATE_PERIOD      1000 /* ms */
#if (CFG_SUPPORT_PERF_IND == 1)
void kalPerfIndReset(IN struct ADAPTER *prAdapter)
{
    uint8_t i;

    for (i = 0; i < BSSID_NUM; i++) {
        prAdapter->prGlueInfo->PerfIndCache.u4CurTxBytes[i] = 0;
        prAdapter->prGlueInfo->PerfIndCache.u4CurRxBytes[i] = 0;
        prAdapter->prGlueInfo->PerfIndCache.u2CurRxRate[i] = 0;
        prAdapter->prGlueInfo->PerfIndCache.ucCurRxRCPI0[i] = 0;
        prAdapter->prGlueInfo->PerfIndCache.ucCurRxRCPI1[i] = 0;
    }
} /* kalPerfIndReset */

void kalSetPerfReport(IN struct ADAPTER *prAdapter)
{
    struct CMD_PERF_IND *prCmdPerfReport;
    uint8_t i;
    uint32_t u4CurrentTp = 0;

    DEBUGFUNC("kalSetPerfReport()");

    prCmdPerfReport = (struct CMD_PERF_IND *)
                      cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
                                  sizeof(struct CMD_PERF_IND));

    if (!prCmdPerfReport) {
        DBGLOG(SW4, ERROR,
               "cnmMemAlloc for kalSetPerfReport failed!\n");
        return;
    }
    kalMemZero(prCmdPerfReport, sizeof(struct CMD_PERF_IND));

    prCmdPerfReport->ucCmdVer = 0;
    prCmdPerfReport->u2CmdLen = sizeof(struct CMD_PERF_IND);

    prCmdPerfReport->u4VaildPeriod = PERF_UPDATE_PERIOD;

    for (i = 0; i < BSS_DEFAULT_NUM; i++) {
        prCmdPerfReport->ulCurTxBytes[i] =
            prAdapter->prGlueInfo->PerfIndCache.u4CurTxBytes[i];
        prCmdPerfReport->ulCurRxBytes[i] =
            prAdapter->prGlueInfo->PerfIndCache.u4CurRxBytes[i];
        prCmdPerfReport->u2CurRxRate[i] =
            prAdapter->prGlueInfo->PerfIndCache.u2CurRxRate[i];
        prCmdPerfReport->ucCurRxRCPI0[i] =
            prAdapter->prGlueInfo->PerfIndCache.ucCurRxRCPI0[i];
        prCmdPerfReport->ucCurRxRCPI1[i] =
            prAdapter->prGlueInfo->PerfIndCache.ucCurRxRCPI1[i];
        prCmdPerfReport->ucCurRxNss[i] =
            prAdapter->prGlueInfo->PerfIndCache.ucCurRxNss[i];
        u4CurrentTp += (prCmdPerfReport->ulCurTxBytes[i] +
                        prCmdPerfReport->ulCurRxBytes[i]);
    }
    if (u4CurrentTp != 0) {
        DBGLOG(SW4, INFO,
               "Total TP[%d] TX-Byte[%d][%d][%d][%d],RX-Byte[%d][%d][%d][%d]\n",
               u4CurrentTp,
               prCmdPerfReport->ulCurTxBytes[0],
               prCmdPerfReport->ulCurTxBytes[1],
               prCmdPerfReport->ulCurTxBytes[2],
               prCmdPerfReport->ulCurTxBytes[3],
               prCmdPerfReport->ulCurRxBytes[0],
               prCmdPerfReport->ulCurRxBytes[1],
               prCmdPerfReport->ulCurRxBytes[2],
               prCmdPerfReport->ulCurRxBytes[3]);
        DBGLOG(SW4, INFO,
               "Rate[%d][%d][%d][%d] RCPI[%d][%d][%d][%d]\n",
               prCmdPerfReport->u2CurRxRate[0],
               prCmdPerfReport->u2CurRxRate[1],
               prCmdPerfReport->u2CurRxRate[2],
               prCmdPerfReport->u2CurRxRate[3],
               prCmdPerfReport->ucCurRxRCPI0[0],
               prCmdPerfReport->ucCurRxRCPI0[1],
               prCmdPerfReport->ucCurRxRCPI0[2],
               prCmdPerfReport->ucCurRxRCPI0[3]);

        wlanSendSetQueryCmd(prAdapter,
                            CMD_ID_PERF_IND,
                            TRUE,
                            FALSE,
                            FALSE,
                            NULL,
                            NULL,
                            sizeof(*prCmdPerfReport),
                            (uint8_t *) prCmdPerfReport, NULL, 0);
    }
    cnmMemFree(prAdapter, prCmdPerfReport);
}               /* kalSetPerfReport */

#endif /* #if (CFG_SUPPORT_PERF_IND == 1) */

#if CFG_SUPPORT_DFS
void kalIndicateChannelSwitch(IN struct GLUE_INFO *prGlueInfo,
                              IN enum ENUM_CHNL_EXT eSco,
                              IN uint8_t ucChannelNum)
{
}
#endif /* #if CFG_SUPPORT_DFS */

uint8_t kalIsValidMacAddr(const uint8_t *addr)
{
    if (!addr)
        return false;

    /* Determine if give Ethernet address is all zeros */
    if ((*(const u16 *)(addr + 0) | *(const u16 *)(addr + 2) |
         *(const u16 *)(addr + 4)) == 0)
        return false;

    /* determine if multicast address */
    if (addr[0] & 0x01)
        return false;

    return true;
}

int kalMaskMemCmp(const void *cs, const void *ct,
                  const void *mask, uint32_t count)
{
    const uint8_t *su1, *su2, *su3;
    int res = 0;

    for (su1 = cs, su2 = ct, su3 = mask;
         count > 0; ++su1, ++su2, ++su3, count--) {
        if (mask != NULL)
            res = ((*su1) & (*su3)) - ((*su2) & (*su3));
        else
            res = (*su1) - (*su2);
        if (res != 0)
            break;
    }
    return res;
}

/*
 * This func is mainly from bionic's strtok.c
 */
int8_t *kalStrtokR(int8_t *s, const int8_t *delim, int8_t **last)
{
    char *spanp;
    int c, sc;
    char *tok;


    if (s == NULL) {
        s = *last;
        if (s == 0)
            return NULL;
    }
cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc)
            goto cont;
    }

    if (c == 0) {       /* no non-delimiter characters */
        *last = NULL;
        return NULL;
    }
    tok = (char *)s - 1;

    for (;;) {
        c = *s++;
        spanp = (char *)delim;
        do {
            sc = *spanp++;
            if (sc == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *last = s;
                return (int8_t *)tok;
            }
        } while (sc != 0);
    }
}

int8_t kalAtoi(uint8_t ch)
{
    if (ch >= 'a' && ch <= 'f')
        return ch - 87;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 55;
    else if (ch >= '0' && ch <= '9')
        return ch - 48;

    return 0;
}

long KAL_NEED_IMPLEMENT(const char *f, const char *func, int line, ...)
{
    /* please refer to the implementation on other os. eg. Linux */
    DBGLOG(INIT, ERROR, "%s not supported here, %s@%d\n",
           func, f, line);
    return 0;
}

int kal_scnprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    int i = 0;

    va_start(args, fmt);
    /* i = vscnprintf(buf, size, fmt, args); */
    va_end(args);

    return i;
}

#if CFG_SUPPORT_WPA3
int kalExternalAuthRequest(IN struct ADAPTER *prAdapter,
                           IN uint8_t uBssIndex)
{
    unsigned char pkt[sizeof(iwreq_data_t) + sizeof(uint32_t)
                                           + sizeof(struct cfg80211_external_auth_params)];
    iwreq_data_t *iwreq;
    uint32_t *cmd;
    /* event return value */
    int evt_ret;
    uint8_t *data;

    struct cfg80211_external_auth_params params;
    struct AIS_FSM_INFO *prAisFsmInfo = NULL;
    struct BSS_DESC *prBssDesc = NULL;

    prAisFsmInfo = aisGetAisFsmInfo(prAdapter, uBssIndex);
    if (!prAisFsmInfo) {
        DBGLOG(SAA, WARN,
               "SAE auth failed with NULL prAisFsmInfo\n");
        return WLAN_STATUS_INVALID_DATA;
    }

    prBssDesc = prAisFsmInfo->prTargetBssDesc;
    if (!prBssDesc) {
        DBGLOG(SAA, WARN,
               "SAE auth failed without prTargetBssDesc\n");
        return WLAN_STATUS_INVALID_DATA;
    }

    kalMemZero(&params, sizeof(struct cfg80211_external_auth_params));
    params.action = NL80211_EXTERNAL_AUTH_START;
    COPY_MAC_ADDR(params.bssid, prBssDesc->aucBSSID);
    COPY_SSID(params.ssid.ssid, params.ssid.ssid_len,
              prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
    params.key_mgmt_suite = RSN_CIPHER_SUITE_SAE;

    iwreq = (iwreq_data_t *)&pkt[0];
    cmd   = (uint32_t *)&pkt[sizeof(*iwreq)];
    data = &pkt[sizeof(*iwreq) + sizeof(uint32_t)];

    *cmd               = IW_CUSTOM_EVENT_FLAG;
    iwreq->data.flags  = IW_EXTERNAL_AUTH_EVENT_FLAG;
    iwreq->data.length = sizeof(struct cfg80211_external_auth_params);

    kalMemCopy(data, &params, sizeof(struct cfg80211_external_auth_params));

    evt_ret = wifi_evt_handler_gen4m(0, pkt, sizeof(iwreq_data_t) + sizeof(uint32_t)
                                     + sizeof(struct cfg80211_external_auth_params));

    /* TODO: if not handled, what should we do? */
    if (evt_ret)
        DBGLOG(INIT, ERROR, "[wifi] auth request not handled!\n");

    return evt_ret;
}
#endif /* #if CFG_SUPPORT_WPA3 */

const uint8_t *kalFindIeMatchMask(uint8_t eid,
                                  const uint8_t *ies, int len,
                                  const uint8_t *match,
                                  int match_len, int match_offset,
                                  const uint8_t *match_mask)
{
    /* match_offset can't be smaller than 2, unless match_len is
     * zero, in which case match_offset must be zero as well.
     */
    if ((match_len && match_offset < 2) || (!match_len && match_offset))
        return NULL;
    while (len >= 2 && len >= ies[1] + 2) {
        if ((ies[0] == eid) &&
            (ies[1] + 2 >= match_offset + match_len) &&
            !kalMaskMemCmp(ies + match_offset,
                           match, match_mask, match_len))
            return ies;
        len -= ies[1] + 2;
        ies += ies[1] + 2;
    }
    return NULL;
}

const uint8_t *kalFindIeExtIE(uint8_t eid, uint8_t exteid,
                              const uint8_t *ies, int len)
{
    if (eid != ELEM_ID_RESERVED)
        return kalFindIeMatchMask(eid, ies, len, NULL, 0, 0, NULL);
    else
        return kalFindIeMatchMask(eid, ies, len, &exteid, 1, 2, NULL);
}

#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
static bool cfg80211_does_bw_fit_range(
    const struct ieee80211_freq_range *freq_range,
    u32 center_freq_khz, u32 bw_khz)
{
    u32 start_freq_khz, end_freq_khz;

    start_freq_khz = center_freq_khz - (bw_khz / 2);
    end_freq_khz = center_freq_khz + (bw_khz / 2);

    if (start_freq_khz >= freq_range->start_freq_khz &&
        end_freq_khz <= freq_range->end_freq_khz)
        return TRUE;

    return FALSE;
}

static u32 reg_get_max_bandwidth_from_range(
    const struct ieee80211_regdomain *rd,
    const struct ieee80211_reg_rule *rule)
{
    const struct ieee80211_freq_range *freq_range = &rule->freq_range;
    const struct ieee80211_freq_range *freq_range_tmp;
    const struct ieee80211_reg_rule *tmp;
    u32 start_freq, end_freq, idx, no;

    for (idx = 0; idx < rd->n_reg_rules; idx++)
        if (rule == &rd->reg_rules[idx])
            break;

    if (idx == rd->n_reg_rules)
        return 0;

    /* get start_freq */
    no = idx;

    while (no) {
        tmp = &rd->reg_rules[--no];
        freq_range_tmp = &tmp->freq_range;

        if (freq_range_tmp->end_freq_khz < freq_range->start_freq_khz)
            break;

        freq_range = freq_range_tmp;
    }

    start_freq = freq_range->start_freq_khz;

    /* get end_freq */
    freq_range = &rule->freq_range;
    no = idx;

    while (no < rd->n_reg_rules - 1) {
        tmp = &rd->reg_rules[++no];
        freq_range_tmp = &tmp->freq_range;

        if (freq_range_tmp->start_freq_khz > freq_range->end_freq_khz)
            break;

        freq_range = freq_range_tmp;
    }

    end_freq = freq_range->end_freq_khz;

    return end_freq - start_freq;
}


static u32 reg_get_max_bandwidth(const struct ieee80211_regdomain *rd,
                                 const struct ieee80211_reg_rule *rule)
{
    u32 bw = reg_get_max_bandwidth_from_range(rd, rule);
    u32 bw80 = (u32)MHZ_TO_KHZ(80);
    u32 bw40 = (u32)MHZ_TO_KHZ(40);
    u32 bw20 = (u32)MHZ_TO_KHZ(20);

    if (rule->flags & NL80211_RRF_NO_160MHZ)
        bw = min(bw, bw80);
    if (rule->flags & NL80211_RRF_NO_80MHZ)
        bw = min(bw, bw40);

    /*
     * HT40+/HT40- limits are handled per-channel. Only limit BW if both
     * are not allowed.
     */
    if (rule->flags & NL80211_RRF_NO_HT40MINUS &&
        rule->flags & NL80211_RRF_NO_HT40PLUS)
        bw = min(bw, bw20);

    return bw;
}

static u32 reg_rule_to_chan_bw_flags(
    const struct ieee80211_regdomain *regd,
    const struct ieee80211_reg_rule *reg_rule,
    const struct ieee80211_channel *chan)
{
    const struct ieee80211_freq_range *freq_range = NULL;
    u32 max_bandwidth_khz, bw_flags = 0;

    freq_range = &reg_rule->freq_range;

    max_bandwidth_khz = freq_range->max_bandwidth_khz;
    /* Check if auto calculation requested */
    if (reg_rule->flags & NL80211_RRF_AUTO_BW)
        max_bandwidth_khz = reg_get_max_bandwidth(regd, reg_rule);

    /* If we get a reg_rule we can assume that at least 5Mhz fit */
    if (!cfg80211_does_bw_fit_range(freq_range,
                                    MHZ_TO_KHZ(chan->center_freq),
                                    MHZ_TO_KHZ(10)))
        bw_flags |= IEEE80211_CHAN_NO_10MHZ;
    if (!cfg80211_does_bw_fit_range(freq_range,
                                    MHZ_TO_KHZ(chan->center_freq),
                                    MHZ_TO_KHZ(20)))
        bw_flags |= IEEE80211_CHAN_NO_20MHZ;

    if (max_bandwidth_khz < MHZ_TO_KHZ(10))
        bw_flags |= IEEE80211_CHAN_NO_10MHZ;
    if (max_bandwidth_khz < MHZ_TO_KHZ(20))
        bw_flags |= IEEE80211_CHAN_NO_20MHZ;
    if (max_bandwidth_khz < MHZ_TO_KHZ(40))
        bw_flags |= IEEE80211_CHAN_NO_HT40;
    if (max_bandwidth_khz < MHZ_TO_KHZ(80))
        bw_flags |= IEEE80211_CHAN_NO_80MHZ;
    if (max_bandwidth_khz < MHZ_TO_KHZ(160))
        bw_flags |= IEEE80211_CHAN_NO_160MHZ;
    return bw_flags;
}


static u32 map_regdom_flags(u32 rd_flags)
{
    u32 channel_flags = 0;

    if (rd_flags & NL80211_RRF_NO_IR_ALL)
        channel_flags |= IEEE80211_CHAN_NO_IR;
    if (rd_flags & NL80211_RRF_DFS)
        channel_flags |= IEEE80211_CHAN_RADAR;
    if (rd_flags & NL80211_RRF_NO_OFDM)
        channel_flags |= IEEE80211_CHAN_NO_OFDM;
    if (rd_flags & NL80211_RRF_NO_OUTDOOR)
        channel_flags |= IEEE80211_CHAN_INDOOR_ONLY;
    if (rd_flags & NL80211_RRF_IR_CONCURRENT)
        channel_flags |= IEEE80211_CHAN_IR_CONCURRENT;
    if (rd_flags & NL80211_RRF_NO_HT40MINUS)
        channel_flags |= IEEE80211_CHAN_NO_HT40MINUS;
    if (rd_flags & NL80211_RRF_NO_HT40PLUS)
        channel_flags |= IEEE80211_CHAN_NO_HT40PLUS;
    if (rd_flags & NL80211_RRF_NO_80MHZ)
        channel_flags |= IEEE80211_CHAN_NO_80MHZ;
    if (rd_flags & NL80211_RRF_NO_160MHZ)
        channel_flags |= IEEE80211_CHAN_NO_160MHZ;
    return channel_flags;
}

static bool freq_in_rule_band(const struct ieee80211_freq_range *freq_range,
                              u32 freq_khz)
{
#define ONE_GHZ_IN_KHZ  1000000
    /*
     * From 802.11ad: directional multi-gigabit (DMG):
     * Pertaining to operation in a frequency band containing a channel
     * with the Channel starting frequency above 45 GHz.
     */
    u32 limit = freq_khz > 45 * ONE_GHZ_IN_KHZ ?
                20 * ONE_GHZ_IN_KHZ : 2 * ONE_GHZ_IN_KHZ;
    if ((u32)abs(freq_khz - freq_range->start_freq_khz) <= limit)
        return TRUE;
    if ((u32)abs(freq_khz - freq_range->end_freq_khz) <= limit)
        return TRUE;
    return FALSE;
#undef ONE_GHZ_IN_KHZ
}


static const struct ieee80211_reg_rule *freq_reg_info_regd(u32 center_freq,
                                                           const struct ieee80211_regdomain *regd, u32 bw)
{
    u32 i;
    bool band_rule_found = false;
    bool bw_fits = false;

    if (!regd)
        return NULL;

    for (i = 0; i < regd->n_reg_rules; i++) {
        const struct ieee80211_reg_rule *rr;
        const struct ieee80211_freq_range *fr = NULL;

        rr = &regd->reg_rules[i];
        fr = &rr->freq_range;

        /*
         * We only need to know if one frequency rule was
         * was in center_freq's band, that's enough, so lets
         * not overwrite it once found
         */
        if (!band_rule_found)
            band_rule_found = freq_in_rule_band(fr, center_freq);

        bw_fits = cfg80211_does_bw_fit_range(fr, center_freq, bw);

        if (band_rule_found && bw_fits)
            return rr;
    }

    if (!band_rule_found)
        return NULL;

    return NULL;
}

static void handle_channel_custom(struct wiphy *wiphy,
                                  struct ieee80211_channel *chan,
                                  const struct ieee80211_regdomain *regd)
{
    u32 bw_flags = 0;
    const struct ieee80211_reg_rule *reg_rule = NULL;
    const struct ieee80211_power_rule *power_rule = NULL;
    u32 bw;

    for (bw = MHZ_TO_KHZ(20); bw >= MHZ_TO_KHZ(5); bw = bw / 2) {
        reg_rule = freq_reg_info_regd(MHZ_TO_KHZ(chan->center_freq),
                                      regd, bw);
        if (!reg_rule)
            break;
    }

    if (reg_rule == NULL) {
        LOG_FUNC(
            "Disabling freq %d MHz as custom regd has no rule that fits it\n",
            chan->center_freq);
        {
            chan->orig_flags |= IEEE80211_CHAN_DISABLED;
            chan->flags = chan->orig_flags;
        }
        return;
    }

    power_rule = &reg_rule->power_rule;

    bw_flags = reg_rule_to_chan_bw_flags(regd, reg_rule, chan);
    chan->flags |= map_regdom_flags(reg_rule->flags) | bw_flags;
    chan->max_antenna_gain = (int) MBI_TO_DBI(power_rule->max_antenna_gain);
}


void kalApplyCustomRegulatory(IN struct wiphy *pWiphy,
                              IN const struct ieee80211_regdomain *pRegdom)
{
    u32 band_idx, ch_idx;
    struct ieee80211_supported_band *sband;
    struct ieee80211_channel *chan;

    /* to reset cha->flags*/
    for (band_idx = 0; band_idx < KAL_NUM_BANDS; band_idx++) {
        sband = pWiphy->bands[band_idx];
        if (!sband)
            continue;

        for (ch_idx = 0; ch_idx < sband->n_channels; ch_idx++) {
            chan = &sband->channels[ch_idx];

            /*reset chan->flags*/
            chan->flags = 0;
            /* update wiphy */
            handle_channel_custom(pWiphy, chan, pRegdom);
        }

    }
    /* update wiphy */
}
#endif /* #if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1) */

ATTR_TEXT_IN_SYSRAM
int _kalSnprintf(char *buf, size_t size, const char *fmt, ...)
{
    int retval;
    va_list ap;

    va_start(ap, fmt);
    retval = vsnprintf(buf, size, fmt, ap);
    va_end(ap);

    return (retval < 0) ? (0) : (retval);
}

int _kalSprintf(char *buf, const char *fmt, ...)
{
    int retval;
    va_list ap;

    va_start(ap, fmt);
    retval = vsprintf(buf, fmt, ap);
    va_end(ap);

    return (retval < 0) ? (0) : (retval);
}

void wifi_conf_get_ip_from_str(uint8_t *ip_dst, const char *ip_src)
{
    unsigned int len = kalStrLen(ip_src);
    unsigned int tmp[4] = {0};
    int i = 0;
    const int8_t delim[] = ".";
    const char *s = ip_src, *cur = NULL;
#if DBG
    LOG_FUNC("%s: input[%d] %s\n", __func__, len, ip_src);
#endif /* #if DBG */
    if (len <= 15) {
        for (i = 0; (i < 4) && (s != NULL); i++) {
            cur = (char *)kalStrtokR((int8_t *)s, delim, (int8_t **)&s);
            if (cur)
                tmp[i] = atoi(&cur[0]);
        }

        for (i = 0; i < 4; i++)
            ip_dst[i] = (uint8_t)tmp[i];
#if DBG
        LOG_FUNC("%u.%u.%u.%u\n",
                 ip_dst[0], ip_dst[1], ip_dst[2], ip_dst[3]);
#endif /* #if DBG */
    } else
        LOG_FUNC("string format is wrong.. %s\n", ip_src);
}

uint8_t get_ip_from_nvdm_ap(char *type, uint8_t *ip_dst)
{
#ifdef MTK_NVDM_ENABLE
    uint8_t ip_str[16] = {0};
    uint32_t len = sizeof(ip_str);

    if (strncmp("IpAddr", type, strlen("IpAddr")) == 0) {
        if (nvdm_read_data_item("hapd", "IpAddr", ip_str, &len) != NVDM_STATUS_OK)
            return -1;
    } else if (strncmp("IpNetmask", type, strlen("IpNetmask")) == 0) {
        if (nvdm_read_data_item("hapd", "IpNetmask", ip_str, &len) != NVDM_STATUS_OK)
            return -1;
    } else if (strncmp("IpGateway", type, strlen("IpGateway")) == 0) {
        if (nvdm_read_data_item("hapd", "IpGateway", ip_str, &len) != NVDM_STATUS_OK)
            return -1;
    } else if (strncmp("IpDns1", type, strlen("IpDns1")) == 0) {
        if (nvdm_read_data_item("hapd", "IpDns1", ip_str, &len) != NVDM_STATUS_OK)
            return -1;
    } else if (strncmp("IpDns2", type, strlen("IpDns2")) == 0) {
        if (nvdm_read_data_item("hapd", "IpDns2", ip_str, &len) != NVDM_STATUS_OK)
            return -1;
    }

    wifi_conf_get_ip_from_str(ip_dst, (char *)ip_str);

    LOG_FUNC("%s: type: %s, ip: %u.%u.%u.%u\n",
             __func__, type,
             ip_dst[0], ip_dst[1], ip_dst[2], ip_dst[3]);

    return 0;

#endif /* #ifdef MTK_NVDM_ENABLE */

    return 1;
}

uint8_t _wifi_on(uint8_t len, char *param[])
{
    if (g_wifi_on == 1) {
        LOG_FUNC("wifi is already on.");
        return -1;
    }
    g_wifi_on = 1;

    _wsys_on(0, NULL);

    lwip_tcpip_init();

    return wifi_init_task();
}

uint8_t _wifi_off(uint8_t len, char *param[])
{
    if (g_wifi_on == 0) {
        LOG_FUNC("wifi is already off.");
        return -1;
    }
    g_wifi_on = 0;

    lwip_tcpip_deinit();

    return wifi_exit_task();
}

uint8_t get_mode_from_nvdm(char *mode)
{
#ifdef MTK_NVDM_ENABLE
    uint32_t len = 10;
    if (nvdm_read_data_item("network", "IpMode", (uint8_t *)mode, &len) != NVDM_STATUS_OK)
        return -1;

    return 0;

#endif /* #ifdef MTK_NVDM_ENABLE */

    return -1;
}

uint8_t get_ip_from_nvdm(char *type, uint8_t *ip_dst)
{
#ifdef MTK_NVDM_ENABLE
    uint8_t ip_str[16] = {0};
    uint32_t len = sizeof(ip_str);

    if (strncmp("IpAddr", type, strlen("IpAddr")) == 0) {
        if (nvdm_read_data_item("network", "IpAddr", ip_str, &len) != NVDM_STATUS_OK)
            return -1;
    } else if (strncmp("IpNetmask", type, strlen("IpNetmask")) == 0) {
        if (nvdm_read_data_item("network", "IpNetmask", ip_str, &len) != NVDM_STATUS_OK)
            return -1;
    } else if (strncmp("IpGateway", type, strlen("IpGateway")) == 0) {
        if (nvdm_read_data_item("network", "IpGateway", ip_str, &len) != NVDM_STATUS_OK)
            return -1;
    }

    wifi_conf_get_ip_from_str(ip_dst, (char *)ip_str);

    LOG_FUNC("%s: type: %s, ip: %u.%u.%u.%u\n",
             __func__, type,
             ip_dst[0], ip_dst[1], ip_dst[2], ip_dst[3]);

    return 0;

#endif /* #ifdef MTK_NVDM_ENABLE */

    return 1;
}

uint8_t _wsys_on(uint8_t len, char *param[])
{
    uint32_t reg = 0;
    uint32_t val = 0;
    uint8_t idx = 0;
    uint8_t t_polling = 0;

    /* LOG_FUNC("CONNSYS WAKEUP\r\n"); */
    /* hal_spm_conninfra_wakeup(); */
    LOG_FUNC("WSYS POS\r\n");

    /* wakeup conn_infra off */
    reg = 0x600601a4;
    val = CONSYS_REG_READ(reg);
    val |= BIT(0);
    /* LOG_FUNC("WRITE %x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* polling chip id */
    reg = 0x60001000;
    t_polling = 10;
    /* LOG_FUNC("polling 0x%x %u times\r\n", reg, t_polling); */
    for (idx = 0; idx <= t_polling; idx++) {
        val = CONSYS_REG_READ(reg);
        if (val == 0x02040100 || val == 0x02040200)
            break;
        kalMdelay(1);
    }

    if (idx >= t_polling) {
        LOG_FUNC("polling chip id failed\r\n");
        return -1;
    }
    /* LOG_FUNC("0x%x: %x\r\n", reg, val); */
    /* reset */
    reg = 0x60000120;
    val = CONSYS_REG_READ(reg);
    val &= ~BIT(0);
    CONSYS_REG_WRITE(reg, val);
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    /* bus slp protect disable */
    reg = 0x60001540;
    val = CONSYS_REG_READ(reg);
    val &= ~BIT(0);
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    reg = 0x30038400;
    val = CONSYS_REG_READ(reg);
    val &= ~BIT(2);
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    reg = 0x30038000;
    val = CONSYS_REG_READ(reg);
    val &= ~BIT(26);
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* wfsys top on */
    reg = 0x60000010;
    val = CONSYS_REG_READ(reg);
    val |= 0x57460080;
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);
    /* row16: polling rgu reset bit[30] 1'b1 */
    reg = 0x600602cc;
    for (idx = 0; idx <= 100; idx++) {
        val = CONSYS_REG_READ(reg);
        /* LOG_FUNC("0x%x: %x\r\n", reg, val); */
        if (val & BIT(30))
            break;
        kalMdelay(1);
    }
    /* LOG_FUNC("polling reset 0x%x %u times %x\r\n", reg, idx, val); */
    /* poll 100 times for check slp protect */
    reg = 0x60001544;
    /* LOG_FUNC("polling 0x%x 100 times\r\n", reg); */
    for (idx = 0; idx <= 100; idx++) {
        val = CONSYS_REG_READ(reg);
        if (!(val & (BIT(29) | BIT(31))))
            break;
        kalMdelay(1);
    }
    /* LOG_FUNC("polling %u times %x\r\n", idx, val); */

    reg = 0x30038410;
    /* LOG_FUNC("polling 0x%x 100 times\r\n", reg); */
    for (idx = 0; idx <= 100; idx++) {
        val = CONSYS_REG_READ(reg);
        if (!(val & BIT(2)))
            break;
        kalMdelay(1);
    }
    /* LOG_FUNC("polling %u times %x\r\n", idx, val); */

    reg = 0x30038010;
    /* LOG_FUNC("polling 0x%x 100 times\r\n", reg); */
    for (idx = 0; idx <= 100; idx++) {
        val = CONSYS_REG_READ(reg);
        if (!(val & BIT(26)))
            break;
        kalMdelay(1);
    }
    /* LOG_FUNC("polling %u times %x\r\n", idx, val); */
    /* polling chip id */
    reg = 0x60001000;
    t_polling = 10;
    /* LOG_FUNC("polling 0x%x %u times\r\n", reg, t_polling); */
    for (idx = 0; idx <= t_polling; idx++) {
        val = CONSYS_REG_READ(reg);
        if (val == 0x02040100 || val == 0x02040200)
            break;
        kalMdelay(1);
    }

    if (idx >= t_polling) {
        LOG_FUNC("polling chip id failed\r\n");
        return -1;
    }
    /* LOG_FUNC("CHIP_ID 0x%x: %x\r\n", reg, val); */
    /* release reset */
    reg = 0x60000120;
    val = CONSYS_REG_READ(reg);
    val |= BIT(0);
    CONSYS_REG_WRITE(reg, val);
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    /* power on complete */
    reg = 0x604c1604;
    t_polling = 100;
    /* LOG_FUNC("polling 0x%x %u times\r\n", reg, t_polling); */
    for (idx = 0; idx <= t_polling; idx++) {
        val = CONSYS_REG_READ(reg);
        if (val == 0x1d1e)
            break;
        kalMdelay(1);
    }

    if (val != 0x1d1e) {
        LOG_FUNC("fail to enter idle loop\r\n");
        return -1;
    }

    /* disable conn_infra off domain force on */
    reg = 0x600601a4;
    val = CONSYS_REG_READ(reg);
    val &= ~BIT(0);
    /* LOG_FUNC("WRITE %x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* WIFI SYS POS end */

    return 0;
}

uint8_t _wsys_off(uint8_t len, char *param[])
{
    uint32_t reg = 0;
    uint32_t val = 0;
    uint8_t idx = 0;
    uint8_t t_polling = 0;

    LOG_FUNC("WSYS PD\r\n");
    /* wakeup */
    reg = 0x600601a4;
    val = CONSYS_REG_READ(reg);
    val |= BIT(0);
    /* LOG_FUNC("WRITE %x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);
    /* polling chip id */
    reg = 0x60001000;
    t_polling = 10;
    /* LOG_FUNC("polling 0x%x %u times\r\n", reg, t_polling); */
    for (idx = 0; idx <= t_polling; idx++) {
        val = CONSYS_REG_READ(reg);
        if (val == 0x02040100 || val == 0x02040200)
            break;
        kalMdelay(1);
    }

    if (idx >= t_polling) {
        LOG_FUNC("polling chip id failed\r\n");
        return -1;
    }
    /* LOG_FUNC("0x%x: %x\r\n", reg, val); */
    /* bus SW slp protect enable */
    reg = 0x60001540;
    val = CONSYS_REG_READ(reg);
    val |= BIT(0);
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* polling slp protect */
    reg = 0x60001544;
    t_polling = 100;
    /* LOG_FUNC("polling 0x%x %u times\r\n", reg, t_polling); */
    for (idx = 0; idx <= t_polling; idx++) {
        val = CONSYS_REG_READ(reg);
        if (val & (BIT(29) | BIT(31)))
            break;
        kalMdelay(1);
    }

    if (idx == t_polling) {
        LOG_FUNC("polling slp prot fail\r\n");
        return -1;
    }
    /* LOG_FUNC("0x%x: %x\r\n", reg, val); */

    /* bus slp protect enable */
    reg = 0x30038400;
    val = CONSYS_REG_READ(reg);
    val |= BIT(2);
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* polling slp protect */
    reg = 0x30038410;
    t_polling = 100;
    /* LOG_FUNC("polling 0x%x %u times\r\n", reg, t_polling); */
    for (idx = 0; idx <= t_polling; idx++) {
        val = CONSYS_REG_READ(reg);
        if (val & BIT(2))
            break;
        kalMdelay(1);
    }

    if (idx == t_polling) {
        LOG_FUNC("polling slp prot fail\r\n");
        return -1;
    }
    /* LOG_FUNC("0x%x: %x\r\n", reg, val); */

    /* bus slp protect enable */
    reg = 0x30038000;
    val = CONSYS_REG_READ(reg);
    val |= BIT(26);
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* polling slp protect */
    reg = 0x30038010;
    t_polling = 100;
    /* LOG_FUNC("polling 0x%x %u times\r\n", reg, t_polling); */
    for (idx = 0; idx <= t_polling; idx++) {
        val = CONSYS_REG_READ(reg);
        if (val & BIT(26))
            break;
        kalMdelay(1);
    }

    if (idx == t_polling) {
        LOG_FUNC("polling slp prot fail\r\n");
        return -1;
    }
    /* LOG_FUNC("0x%x: %x\r\n", reg, val); */

    /* turn off wpll */
    reg = 0x60003004;
    val = CONSYS_REG_READ(reg);
    val &= ~(BIT(16) | BIT(17));
    val &= ~(BIT(1) | BIT(0));
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* read wpll */
    reg = 0x60009a00;
    val = CONSYS_REG_READ(reg);
    if (val & BITS(12, 15))
        LOG_FUNC("0x%x: %x FAILED (12:15)\r\n", reg, val);
    /* wfsys top off */
    reg = 0x60000010;
    val = CONSYS_REG_READ(reg);
    val |= 0x57460000;
    val &= ~BIT(7);
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* polling slp protect */
    reg = 0x60001544;
    t_polling = 100;
    /* LOG_FUNC("polling 0x%x %u times\r\n", reg, t_polling); */
    for (idx = 0; idx <= t_polling; idx++) {
        val = CONSYS_REG_READ(reg);
        if (val & (BIT(29) | BIT(31)))
            break;
        kalMdelay(1);
    }

    if (idx == t_polling) {
        LOG_FUNC("polling slp prot fail\r\n");
        return -1;
    }
    /* LOG_FUNC("0x%x: %x\r\n", reg, val); */

    /* polling rgu off */
    reg = 0x600602cc;
    t_polling = 100;
    /* LOG_FUNC("polling 0x%x %u times\r\n", reg, t_polling); */
    for (idx = 0; idx <= t_polling; idx++) {
        val = CONSYS_REG_READ(reg);
        if (!(val & BIT(30)))
            break;
        kalMdelay(1);
    }

    if (idx == t_polling) {
        LOG_FUNC("polling rgu off fail\r\n");
        return -1;
    }

    /* disable adie clk */
    reg = 0x60003004;
    val = CONSYS_REG_READ(reg);
    val &= ~(BITS(16, 17) | BITS(0, 1));
    val |= (2 << 16);
    val |= 2;
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* disable adie clk */
    reg = 0x60005124;
    val = CONSYS_REG_READ(reg);
    val &= ~0x3;
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* emi req */
    reg = 0x60001414;
    val = CONSYS_REG_READ(reg);
    val |= 0x1;
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);
    val = CONSYS_REG_READ(reg);
    val &= ~0x1;
    /* LOG_FUNC("WRITE 0x%x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);

    /* wakeup */
    reg = 0x600601a4;
    val = CONSYS_REG_READ(reg);
    val &= ~BIT(0);
    /* LOG_FUNC("WRITE %x %x\r\n", reg, val); */
    CONSYS_REG_WRITE(reg, val);
    return 0;
}

#if (CONFIG_WIFI_TEST_TOOL == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to search desired OID.
 *
 * \param rOid[in]               Desired NDIS_OID
 * \param ppWlanReqEntry[out]    Found registered OID entry
 *
 * \retval TRUE: Matched OID is found
 * \retval FALSE: No matched OID is found
 */
/*----------------------------------------------------------------------------*/
uint8_t reqSearchSupportedOidEntry(IN uint32_t rOid,
                                   OUT struct WLAN_REQ_ENTRY **ppWlanReqEntry)
{
    uint32_t i, j, k;

    i = 0;
    j = NUM_SUPPORTED_OIDS - 1;

    while (i <= j) {
        k = (i + j) / 2;

        if (rOid == arWlanOidReqTable[k].rOid) {
            *ppWlanReqEntry = &arWlanOidReqTable[k];
            return TRUE;
        } else if (rOid < arWlanOidReqTable[k].rOid) {
            j = k - 1;
        } else {
            i = k + 1;
        }
    }

    return FALSE;
}               /* reqSearchSupportedOidEntry */

/*----------------------------------------------------------------------------*/
/*!
 * \brief The routine handles a set operation for a single OID.
 *
 * \param[in] pDev Net device requested.
 * \param[in] ndisReq Ndis request OID information copy from user.
 * \param[out] outputLen_p If the call is successful, returns the number of
 *                         bytes written into the query buffer. If the
 *                         call failed due to invalid length of the query
 *                         buffer, returns the amount of storage needed..
 *
 * \retval 0 On success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_set_ndis(struct GLUE_INFO *prGlueInfo,
                  struct NDIS_TRANSPORT_STRUCT *prNdisReq,
                  uint32_t *pu4OutputLen)
{
    struct WLAN_REQ_ENTRY *prWlanReqEntry = NULL;
    uint32_t status = WLAN_STATUS_SUCCESS;
    uint32_t u4SetInfoLen = 0;

    if (!prNdisReq || !pu4OutputLen) {
        DBGLOG(REQ, INFO,
               "priv_set_ndis(): invalid param(0x%p, 0x%p)\n",
               prNdisReq, pu4OutputLen);
        return -EINVAL;
    }

    if (!prGlueInfo) {
        DBGLOG(REQ, INFO,
               "priv_set_ndis(): invalid prGlueInfo\n");
        return -EINVAL;
    }

    if (reqSearchSupportedOidEntry(prNdisReq->ndisOidCmd,
                                   &prWlanReqEntry) == FALSE) {
        /* WARNLOG(
         *         ("Set OID: 0x%08lx (unknown)\n",
         *         prNdisReq->ndisOidCmd));
         */
        return -EOPNOTSUPP;
    }

    if (prWlanReqEntry->pfOidSetHandler == NULL) {
        /* WARNLOG(
         *         ("Set %s: Null set handler\n",
         *         prWlanReqEntry->pucOidName));
         */
        return -EOPNOTSUPP;
    }

    if (prWlanReqEntry->fgSetBufLenChecking) {
        if (prNdisReq->inNdisOidlength !=
            prWlanReqEntry->u4InfoBufLen) {
            DBGLOG(REQ, WARN,
                   "Set %s: Invalid length (current=%d, needed=%d)\n",
                   prWlanReqEntry->pucOidName,
                   prNdisReq->inNdisOidlength,
                   prWlanReqEntry->u4InfoBufLen);

            *pu4OutputLen = prWlanReqEntry->u4InfoBufLen;
            return -EINVAL;
        }
    }

    if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_ONLY) {
        /* GLUE sw info only */
        status = prWlanReqEntry->pfOidSetHandler(prGlueInfo,
                                                 prNdisReq->ndisOidContent,
                                                 prNdisReq->inNdisOidlength, &u4SetInfoLen);
    } else if (prWlanReqEntry->eOidMethod ==
               ENUM_OID_GLUE_EXTENSION) {
        /* multiple sw operations */
        status = prWlanReqEntry->pfOidSetHandler(prGlueInfo,
                                                 prNdisReq->ndisOidContent,
                                                 prNdisReq->inNdisOidlength, &u4SetInfoLen);
    } else if (prWlanReqEntry->eOidMethod ==
               ENUM_OID_DRIVER_CORE) {
        /* driver core */

        status = kalIoctl(prGlueInfo,
                          (PFN_OID_HANDLER_FUNC) prWlanReqEntry->pfOidSetHandler,
                          prNdisReq->ndisOidContent,
                          prNdisReq->inNdisOidlength,
                          FALSE, FALSE, TRUE, &u4SetInfoLen);
    } else {
        DBGLOG(REQ, INFO,
               "priv_set_ndis(): unsupported OID method:0x%x\n",
               prWlanReqEntry->eOidMethod);
        return -EOPNOTSUPP;
    }

    *pu4OutputLen = u4SetInfoLen;

    if (status != WLAN_STATUS_SUCCESS)
        DBGLOG(REQ, INFO,
               "%s: fail, return 0x%08x\n", __func__, status);

    return status;
}               /* priv_set_ndis */

/*----------------------------------------------------------------------------*/
/*!
 * \brief The routine handles a query operation for a single OID. Basically we
 *   return information about the current state of the OID in question.
 *
 * \param[in] pDev Net device requested.
 * \param[in] ndisReq Ndis request OID information copy from user.
 * \param[out] outputLen_p If the call is successful, returns the number of
 *                        bytes written into the query buffer. If the
 *                        call failed due to invalid length of the query
 *                        buffer, returns the amount of storage needed..
 *
 * \retval 0 On success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EINVAL invalid input parameters
 *
 */
/*----------------------------------------------------------------------------*/
int priv_get_ndis(struct GLUE_INFO *prGlueInfo,
                  struct NDIS_TRANSPORT_STRUCT *prNdisReq,
                  uint32_t *pu4OutputLen)
{
    struct WLAN_REQ_ENTRY *prWlanReqEntry = NULL;
    uint32_t u4BufLen = 0;
    uint32_t status = WLAN_STATUS_SUCCESS;

    if (!prNdisReq || !pu4OutputLen) {
        DBGLOG(REQ, INFO,
               "priv_get_ndis(): invalid param(0x%p, 0x%p)\n",
               prNdisReq, pu4OutputLen);
        return -EINVAL;
    }

    if (!prGlueInfo) {
        DBGLOG(REQ, INFO,
               "priv_get_ndis(): invalid prGlueInfo\n");
        return -EINVAL;
    }

    if (reqSearchSupportedOidEntry(prNdisReq->ndisOidCmd,
                                   &prWlanReqEntry) == FALSE) {
        /* WARNLOG(
         *         ("Query OID: 0x%08lx (unknown)\n",
         *         prNdisReq->ndisOidCmd));
         */
        return -EOPNOTSUPP;
    }

    if (prWlanReqEntry->pfOidQueryHandler == NULL) {
        /* WARNLOG(
         *         ("Query %s: Null query handler\n",
         *         prWlanReqEntry->pucOidName));
         */
        return -EOPNOTSUPP;
    }

    if (prWlanReqEntry->fgQryBufLenChecking) {
        if (prNdisReq->inNdisOidlength <
            prWlanReqEntry->u4InfoBufLen) {
            /* Not enough room in InformationBuffer. Punt */
            /* WARNLOG(
             * ("Query %s: Buffer too short (current=%ld,
             * needed=%ld)\n",
             * prWlanReqEntry->pucOidName,
             * prNdisReq->inNdisOidlength,
             * prWlanReqEntry->u4InfoBufLen));
             */

            *pu4OutputLen = prWlanReqEntry->u4InfoBufLen;

            return -EINVAL;
        }
    }

    if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_ONLY) {
        /* GLUE sw info only */
        status = prWlanReqEntry->pfOidQueryHandler(prGlueInfo,
                                                   prNdisReq->ndisOidContent,
                                                   prNdisReq->inNdisOidlength, &u4BufLen);
    } else if (prWlanReqEntry->eOidMethod ==
               ENUM_OID_GLUE_EXTENSION) {
        /* multiple sw operations */
        status = prWlanReqEntry->pfOidQueryHandler(prGlueInfo,
                                                   prNdisReq->ndisOidContent,
                                                   prNdisReq->inNdisOidlength, &u4BufLen);
    } else if (prWlanReqEntry->eOidMethod ==
               ENUM_OID_DRIVER_CORE) {
        /* driver core */

        status = kalIoctl(prGlueInfo,
                          (PFN_OID_HANDLER_FUNC)prWlanReqEntry->pfOidQueryHandler,
                          prNdisReq->ndisOidContent, prNdisReq->inNdisOidlength,
                          TRUE, TRUE, TRUE, &u4BufLen);
    } else {
        DBGLOG(REQ, INFO,
               "priv_set_ndis(): unsupported OID method:0x%x\n",
               prWlanReqEntry->eOidMethod);
        return -EOPNOTSUPP;
    }

    *pu4OutputLen = u4BufLen;

    switch (status) {
        case WLAN_STATUS_SUCCESS:
            break;

        case WLAN_STATUS_INVALID_LENGTH:
            /* WARNLOG(
             * ("Set %s: Invalid length (current=%ld, needed=%ld)\n",
             *  prWlanReqEntry->pucOidName,
             *  prNdisReq->inNdisOidlength,
             *  u4BufLen));
             */
            break;
    }

    if (status != WLAN_STATUS_SUCCESS)
        return -EOPNOTSUPP;

    return 0;
}               /* priv_get_ndis */
/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl set structure handler.
 *
 * \param[in] pDev Net device requested.
 * \param[in] prIwReqData Pointer to iwreq_data structure.
 *
 * \retval 0 For success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EINVAL If a value is out of range.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_set_struct(struct iwreq *pwrq, IN char *pcExtra)
{
    uint32_t u4SubCmd = 0;
    int status = 0;
    /* WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS; */
    uint32_t u4CmdLen = 0;
    struct NDIS_TRANSPORT_STRUCT *prNdisReq;

    struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    uint32_t u4BufLen = 0;
    /* uint8_t ucBssIndex = AIS_DEFAULT_INDEX; */
    iwreq_data_t *prIwReqData = NULL;

    ASSERT(pwrq);
    /* ASSERT(pcExtra); */
    prIwReqData = &(pwrq->u);

    u4SubCmd = (uint32_t) prIwReqData->data.flags;

    prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) prIwReqData->data.pointer;

    switch (u4SubCmd) {
        case PRIV_CMD_OID:
            u4CmdLen = prIwReqData->data.length;
            if (u4CmdLen > CMD_OID_BUF_LENGTH) {
                DBGLOG(REQ, ERROR, "Input data length is invalid %u\n",
                       u4CmdLen);
                return -EINVAL;
            }
            /* Execute this OID */
            status = priv_set_ndis(prGlueInfo, prNdisReq, &u4BufLen);

            break;

        default:
            return -EOPNOTSUPP;
    }

    return status;
}               /* priv_set_struct */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Private ioctl get struct handler.
 *
 * \param[in] pDev Net device requested.
 * \param[out] pIwReq Pointer to iwreq structure.
 * \param[in] cmd Private sub-command.
 *
 * \retval 0 For success.
 * \retval -EFAULT If copy from user space buffer fail.
 * \retval -EOPNOTSUPP Parameter "cmd" not recognized.
 *
 */
/*----------------------------------------------------------------------------*/
int priv_get_struct(struct iwreq *pwrq, IN OUT char *pcExtra)
{
    uint32_t u4SubCmd = 0;
    struct NDIS_TRANSPORT_STRUCT *prNdisReq = NULL;

    struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) g_prGlueInfo;
    uint32_t u4BufLen = 0;
    iwreq_data_t *prIwReqData = &(pwrq->u);
    /* uint32_t *pu4IntBuf = NULL; */
    /* int status = 0; */

    if (!prIwReqData) {
        DBGLOG(REQ, INFO,
               "priv_get_struct(): invalid param(0x%p)\n", prIwReqData);
        return -EINVAL;
    }

    u4SubCmd = (uint32_t) prIwReqData->data.flags;
    if (!prGlueInfo) {
        DBGLOG(REQ, INFO,
               "priv_get_struct(): invalid prGlueInfo\n");
        return -EINVAL;
    }

    prNdisReq = (struct NDIS_TRANSPORT_STRUCT *) prIwReqData->data.pointer;

    switch (u4SubCmd) {
        case PRIV_CMD_OID:
            if (priv_get_ndis(prGlueInfo, prNdisReq, &u4BufLen) == 0) {
                prNdisReq->outNdisOidLength = u4BufLen;
                return 0;
            }
            prNdisReq->outNdisOidLength = u4BufLen;
            return -EFAULT;

        default:
            DBGLOG(REQ, WARN, "get struct cmd:0x%x\n", u4SubCmd);
            return -EOPNOTSUPP;
    }
}
/* __priv_get_struct */
#endif /* #if (CONFIG_WIFI_TEST_TOOL == 1) */

#if (CFG_SUPPORT_QA_TOOL != 1) && (CONFIG_WLAN_SERVICE == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief The routine handles ATE set operation.
 *
 * \param[in] pDev Net device requested.
 * \param[in] ndisReq Ndis request OID information copy from user.
 * \param[out] outputLen_p If the call is successful, returns the number of
 *                         bytes written into the query buffer. If the
 *                         call failed due to invalid length of the query
 *                         buffer, returns the amount of storage needed..
 *
 * \retval 0 On success.
 * \retval -EOPNOTSUPP If cmd is not supported.
 * \retval -EFAULT If copy from user space buffer fail.
 *
 */
/*----------------------------------------------------------------------------*/
int __priv_ate_set(IN struct GLUE_INFO *prGlueInfo,
                   char **param, IN int i4TotalLen)
{
    char *pcCommand = param[0];
    int Ret = 0;
    /* uint8_t *InBuf;
     * uint8_t *addr_str, *value_str;
     * uint32_t InBufLen;
     */
    /* u_int8_t isWrite = 0;
     * uint32_t u4BufLen = 0;
     * struct NDIS_TRANSPORT_STRUCT *prNdisReq;
     * uint32_t pu4IntBuf[2];
     */
    uint32_t u4CopySize = sizeof(aucOidBuf);

    kalMemZero(&aucOidBuf[0], u4CopySize);

    /* sanity check */

    if (GLUE_CHK_PR2(prGlueInfo, pcCommand) == FALSE)
        return -1;

    DBGLOG(REQ, INFO, "MT6632: %s, len=%d\n",
           pcCommand, kalStrLen(pcCommand));

    u4CopySize = (kalStrLen(pcCommand) < u4CopySize)
                 ? kalStrLen(pcCommand) : (u4CopySize - 1);

    kalMemCopy(&aucOidBuf[0], (uint8_t *)pcCommand, u4CopySize);
    aucOidBuf[u4CopySize] = '\0';
    DBGLOG(REQ, INFO,
           "PRIV_QACMD_SET: priv_set_string=(%s)(%d)\n",
           aucOidBuf, u4CopySize);

    Ret = AteCmdSetHandle(prGlueInfo, &aucOidBuf[0], u4CopySize);
    return Ret;
}
#endif /* #if (CFG_SUPPORT_QA_TOOL != 1) && (CONFIG_WLAN_SERVICE == 1) */
