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
#include "ci.h"
#include "ci_cli.h"
#include "hal.h"
#include "hal_gpt.h"
#include "hal_sd.h"
#include "hal_platform.h"


volatile int8_t read_write_flag = 0;
volatile uint8_t card_inserted = 0;

void sd_callback(hal_sd_callback_event_t event, void *user_data)
{
    if (HAL_SD_EVENT_SUCCESS == event) {
        read_write_flag = 1;
    } else {
        read_write_flag = -1;
    }
}

ci_status_t ci_sd_read_test(void)
{
    hal_sd_config_t sd_config = {
        .bus_width = HAL_SD_BUS_WIDTH_4,
        .clock = 45000,
    };

    hal_sd_port_t sd_port = HAL_SD_PORT_0;
    uint32_t *read_buffer = malloc(1024);
    uint32_t start_address = 0x1;
    uint32_t block_number = 2;

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_init(sd_port, &sd_config));

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_read_blocks(sd_port, read_buffer, start_address, block_number));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_read_blocks(sd_port, read_buffer, start_address * 2, block_number));

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_register_callback(sd_port, sd_callback, NULL));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_read_blocks_dma(sd_port, read_buffer, start_address, block_number));
    /* wait dma transfer finished, and set flag to 0 */
    while (read_write_flag != 1);
    read_write_flag = 0;

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_read_blocks_dma_blocking(sd_port, read_buffer, start_address, block_number));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_read_blocks_dma_blocking(sd_port, read_buffer, start_address * 2, block_number));

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_deinit(sd_port));

    return CI_PASS;
}

ci_status_t ci_sd_write_test(void)
{
    hal_sd_config_t sd_config = {
        .bus_width = HAL_SD_BUS_WIDTH_4,
        .clock = 45000,
    };

    hal_sd_port_t sd_port = HAL_SD_PORT_0;
    uint32_t *write_buffer = malloc(1024);
    uint32_t start_address = 0x1;
    uint32_t block_number = 2;

    memset(write_buffer, 0x5A, 1024);
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_init(sd_port, &sd_config));

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_write_blocks_dma(sd_port, write_buffer, start_address, block_number));
    /* wait dma transfer finished, and set flag to 0 */
    while (read_write_flag != 1);
    read_write_flag = 0;

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_write_blocks(sd_port, write_buffer, start_address, block_number));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_write_blocks(sd_port, write_buffer, start_address * 2, block_number));

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_write_blocks_dma_blocking(sd_port, write_buffer, start_address, block_number));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_write_blocks_dma_blocking(sd_port, write_buffer, start_address * 2, block_number));

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_deinit(sd_port));

    return CI_PASS;
}

ci_status_t ci_sd_erase_test(void)
{
    hal_sd_config_t sd_config = {
        .bus_width = HAL_SD_BUS_WIDTH_4,
        .clock = 45000,
    };

    hal_sd_port_t sd_port = HAL_SD_PORT_0;
    uint32_t erase_size = 0;

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_init(sd_port, &sd_config));

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_get_erase_sector_size(sd_port, &erase_size));
    log_hal_error("erase_size = 0x%x\n", erase_size);

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_erase_sectors(sd_port, 0, 0x100));

    return CI_PASS;
}


ci_status_t ci_sd_other_test(void)
{
    hal_sd_config_t sd_config = {
        .bus_width = HAL_SD_BUS_WIDTH_4,
        .clock = 45000,
    };
    hal_sd_card_type_t card_type;
    hal_sd_port_t sd_port = HAL_SD_PORT_0;
    uint32_t *read_buffer = malloc(512);
    uint32_t clock = 30000;
    uint64_t capacity = 0;

    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_init(sd_port, &sd_config));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_set_clock(sd_port, clock));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_get_clock(sd_port, &clock));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_set_bus_width(sd_port, HAL_SD_BUS_WIDTH_1));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_get_capacity(sd_port, &capacity));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_get_csd(sd_port, read_buffer));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_get_cid(sd_port, read_buffer));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_get_ocr(sd_port, read_buffer));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_get_card_status(sd_port, read_buffer));
    EXPECT_VAL(HAL_SD_STATUS_OK, hal_sd_get_card_type(sd_port, &card_type));

    return CI_PASS;
}

ci_status_t ci_sd_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"SDCard_Read_Test", ci_sd_read_test},
        {"SDCard_Write_Test", ci_sd_write_test},
        {"SDCard_Erase_Test", ci_sd_erase_test},
        {"SDCard_Other_Test", ci_sd_other_test},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
