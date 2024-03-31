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

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_SD_MODULE_ENABLE)
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ut.h"
#include "hal_sd.h"

#define SD_UT_TEST_BLOCK_NUM  (1)
uint32_t __attribute__((__section__(".noncached_zidata"))) msdc_buffer1[128 * SD_UT_TEST_BLOCK_NUM] = {0};
uint32_t __attribute__((__section__(".noncached_zidata"))) msdc_buffer2[128 * SD_UT_TEST_BLOCK_NUM] = {0};

volatile int8_t read_write_flg = 0;

void sd_callback(hal_sd_callback_event_t event, void *user_data)
{
    if (HAL_SD_EVENT_SUCCESS == event) {
        read_write_flg = 1;
    } else {
        read_write_flg = -1;
    }
}

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
ut_status_t ut_hal_sd(void)
{
    uint32_t i;
    hal_sd_status_t status;
    hal_sd_config_t sd_cfg = { HAL_SD_BUS_WIDTH_4, 26000};

    status = hal_sd_init(HAL_SD_PORT_0, &sd_cfg);
    if (status != HAL_SD_STATUS_OK) {
        return -1;
    }

    hal_sd_register_callback(HAL_SD_PORT_0, sd_callback, NULL);

    memset(msdc_buffer1, 0, sizeof(msdc_buffer1));
    memset(msdc_buffer2, 0, sizeof(msdc_buffer2));

    for (i = 0; i < 128 * SD_UT_TEST_BLOCK_NUM; i++) {
        msdc_buffer1[i] = ((i + 10) << 0) | ((i + 11) << 8) | ((i + 12) << 16) | ((i + 13) << 24);
    }

    status = hal_sd_write_blocks(HAL_SD_PORT_0, msdc_buffer1, 0, SD_UT_TEST_BLOCK_NUM);
    if (status != HAL_SD_STATUS_OK) {
        return -2;
    }

    printf("----Test Write Blocks Done-----\n");
    status = hal_sd_read_blocks(HAL_SD_PORT_0, msdc_buffer2, 0, SD_UT_TEST_BLOCK_NUM);
    if (status != HAL_SD_STATUS_OK) {
        return -3;
    }

    printf("----Test Read Blocks Done-----\n");
    for (i = 0; i < 128 * SD_UT_TEST_BLOCK_NUM; i++) {
        if (msdc_buffer1[i] != msdc_buffer2[i]) {
            return -4;
        }
    }

    status = hal_sd_erase_sectors(HAL_SD_PORT_0, 0, SD_UT_TEST_BLOCK_NUM);
    if (status != HAL_SD_STATUS_OK) {
        return -5;
    }
    printf("----Test erase Blocks Done-----\n");

    memset(msdc_buffer2, 0, sizeof(msdc_buffer2));

    status = hal_sd_write_blocks_dma_blocking(HAL_SD_PORT_0, msdc_buffer1, 0, SD_UT_TEST_BLOCK_NUM);
    if (status != HAL_SD_STATUS_OK) {
        return -6;
    }
    printf("----Test write Blocks Done by dma-----\n");

    status = hal_sd_read_blocks_dma_blocking(HAL_SD_PORT_0, msdc_buffer2, 0, SD_UT_TEST_BLOCK_NUM);
    if (status != HAL_SD_STATUS_OK) {
        return -7;
    }

    printf("----Test read Blocks Done by dma-----\n");
    for (i = 0; i < 128 * SD_UT_TEST_BLOCK_NUM; i++) {
        if (msdc_buffer1[i] != msdc_buffer2[i]) {
            return -8;
        }
    }

#if 0
    memset(msdc_buffer2, 0, sizeof(msdc_buffer2));

    status = hal_sd_write_blocks_dma(HAL_SD_PORT_0, msdc_buffer1, 0, SD_UT_TEST_BLOCK_NUM);
    if (status != HAL_SD_STATUS_OK) {
        return -6;
    }
    printf("----Test write Blocks Done dma-----\n");

    while (!read_write_flg);
    if (-1 == read_write_flg) {
        return -7;
    }
    read_write_flg = 0;

    printf("----Test read Blocks Done dma-----\n");
    status = hal_sd_read_blocks_dma(HAL_SD_PORT_0, msdc_buffer2, 0, SD_UT_TEST_BLOCK_NUM);
    if (status != HAL_SD_STATUS_OK) {
        return -8;
    }

    while (!read_write_flg);
    if (-1 == read_write_flg) {
        return -9;
    }
    read_write_flg = 0;

    for (i = 0; i < 128 * SD_UT_TEST_BLOCK_NUM; i++) {
        if (msdc_buffer1[i] != msdc_buffer2[i]) {
            return -10;
        }
    }
#endif /* #if 0 */
    hal_sd_deinit(HAL_SD_PORT_0);

    printf("----Test SD-----\n");
    return UT_STATUS_OK;
}
#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_SD_MODULE_ENABLE) */
