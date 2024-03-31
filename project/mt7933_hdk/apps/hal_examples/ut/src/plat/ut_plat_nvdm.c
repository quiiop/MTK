/* Copyright Statement:
 *
 * (C) 2005-2030  MediaTek Inc. All rights reserved.
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
#include "ut.h"

#if defined(UT_PLATFORM_ENABLE) && defined (UT_PLATFORM_NVDM_ENABLE)

#include "nvdm.h"

#define STA_IPADDR_DEFAULT_VALUE "192.168.0.1"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
ut_status_t ut_plat_nvdm(void)
{
    nvdm_status_t status;
    uint32_t size;
    uint8_t buffer[16];

    printf("\r\n<<NVRAM>>\r\n");

    // Write NVDM Data
    status = nvdm_write_data_item("STA", "IpAddr", NVDM_DATA_ITEM_TYPE_STRING, (uint8_t *)STA_IPADDR_DEFAULT_VALUE, sizeof(STA_IPADDR_DEFAULT_VALUE));
    if (status != NVDM_STATUS_OK) {
        //Error handler;
        printf("Fail\r\n\r\n");
        return UT_STATUS_ERROR;
    }

    // Read NVDM Data
    size = sizeof(buffer);
    status = nvdm_read_data_item("STA", "IpAddr", buffer, &size);
    if (status != NVDM_STATUS_OK) {
        printf("Fail\r\n\r\n");
        return UT_STATUS_ERROR;
    }

    printf("Write:%s\r\nRead :%s\r\n", STA_IPADDR_DEFAULT_VALUE, buffer);

    if (memcmp(buffer, STA_IPADDR_DEFAULT_VALUE, sizeof(STA_IPADDR_DEFAULT_VALUE)) == 0) {
        printf("Pass\r\n\r\n");
        return UT_STATUS_ERROR;
    } else {
        printf("Fail\r\n\r\n");
        return UT_STATUS_ERROR;
    }

    return UT_STATUS_OK;
}

#endif /* #if defined(UT_PLATFORM_ENABLE) && defined (UT_PLATFORM_NVDM_ENABLE) */
