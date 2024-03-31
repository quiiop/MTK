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

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_SDIO_MODULE_ENABLE)

#define DESENSE_TEST

#ifdef DESENSE_TEST
#include <hal_msdc.h>

bool stop = false;

void sdio_desense_set_frequency(uint32_t khz)
{
    msdc_set_output_clock(khz); /*setting bus clock to 240KHz.*/
    log_hal_info("SDIO desense set freq:%dMhz done!\n", khz / 1000);
}

uint32_t sdio_desense_get_frequency()
{
    uint32_t khz =  msdc_get_output_clock();
    log_hal_info("SDIO desense current freq:%dMhz!\n", khz / 1000);
    return khz;
}

void sdio_desense_init()
{
    msdc_init(HAL_SDIO_BUS_WIDTH_4);
    /*reset data timeout conter to 65536*256 cycles*/
    MSDC_REG->SDC_CFG = MSDC_REG->SDC_CFG | 0xFF000000;

    /*enable SDIO mode*/
    MSDC_REG->SDC_CFG |= SDC_CFG_SDIO;

    /*enable 74 serial clock*/
    MSDC_REG->MSDC_CFG = MSDC_REG->MSDC_CFG | MSDC_CFG_CKPDN;

    /*enable SDIO interrupt*/
    MSDC_REG->SDC_CFG |= SDC_CFG_SDIOIDE;

    sdio_desense_set_frequency(25000);
    log_hal_info("SDIO desense init done!\n");
}

void sdio_desense_start()
{
    uint32_t blk_size;
    uint32_t loop = 1000;
    stop = false;
    while (!stop && loop) {
        log_hal_info("SDIO desense start loop:%d!\n", loop);
        hal_sdio_set_block_size(HAL_SDIO_PORT_0, HAL_SDIO_FUNCTION_1, 256);
        hal_sdio_get_block_size(HAL_SDIO_PORT_0, HAL_SDIO_FUNCTION_1, &blk_size);
        hal_gpt_delay_ms(1);
        loop--;
    }
    log_hal_info("SDIO desense start done!\n");
}

void sdio_desense_stop()
{
    stop = true;
    log_hal_info("SDIO desense stop done!\n");
}

void sdio_desense_deinit()
{
    msdc_deinit();
    log_hal_info("SDIO desense deinit done!\n");
}

ut_status_t ut_hal_sdio(void)
{
    uint32_t test_loop = 10;
    while (test_loop) {
        log_hal_info("==========================================\n");
        log_hal_info("SDIO desense test loop=%d!\n", test_loop);
        sdio_desense_init();
        sdio_desense_set_frequency(50000);
        sdio_desense_get_frequency();
        sdio_desense_start();
        sdio_desense_stop();
        sdio_desense_deinit();
        test_loop--;
        log_hal_info("==========================================\n");
    }
    log_hal_info("SDIO desense Done!\n");
    return UT_STATUS_OK;
}
#else /* #ifdef DESENSE_TEST */
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
hal_sdio_command53_config_t cmd53_conf;
#define SDIO_BLK_SIZE 256

static void sdio_cmd53_read(int addr, int length, uint32_t buffer_addr)
{
    if (length < SDIO_BLK_SIZE) {
        cmd53_conf.count = length;    /**< Byte or block count. */
        cmd53_conf.block = FALSE;
    } else {
        cmd53_conf.count = (length + (SDIO_BLK_SIZE - 1)) / SDIO_BLK_SIZE;
        cmd53_conf.block = TRUE;
    }
    cmd53_conf.direction = HAL_SDIO_DIRECTION_READ;                                    /**< Read/write direction for the SDIO COMMAND53. */
    cmd53_conf.address = addr;                                                  /**< Read/write address of the SDIO COMMAND53. */
    cmd53_conf.buffer = buffer_addr;
    log_hal_info("addr: 0x%02x, size : %d, buffer_addr : 0x%08x, block mode : %d  ", addr, cmd53_conf.count, buffer_addr, cmd53_conf.block);
    hal_sdio_execute_command53(HAL_SDIO_PORT_0, &cmd53_conf);
}

static void sdio_cmd53_write(int addr, int length, uint32_t buffer_addr)
{

    if (length < SDIO_BLK_SIZE) {
        cmd53_conf.count = length;    /**< Byte or block count. */
        cmd53_conf.block = FALSE;
    } else {
        cmd53_conf.count = (length + (SDIO_BLK_SIZE - 1)) / SDIO_BLK_SIZE;
        cmd53_conf.block = TRUE;
    }
    cmd53_conf.direction = HAL_SDIO_DIRECTION_WRITE;                                    /**< Read/write direction for the SDIO COMMAND53. */
    cmd53_conf.address = addr;                                                  /**< Read/write address of the SDIO COMMAND53. */
    cmd53_conf.buffer = buffer_addr;
    log_hal_info("addr: 0x%02x, size : %d, buffer_addr : 0x%08x, block mode : %d ", addr, cmd53_conf.count, buffer_addr, cmd53_conf.block);
    hal_sdio_execute_command53(HAL_SDIO_PORT_0, &cmd53_conf);

}

ut_status_t ut_hal_sdio(void)
{
    hal_sdio_status_t status = 0;
    hal_sdio_config_t sdio_config = {HAL_SDIO_BUS_WIDTH_4, 50000};
    uint32_t blk_size;
    uint32_t val;

    log_hal_error("wifi_chip_hw_power_on -- start \r\n");
    //wifi_chip_hw_power_on();
    log_hal_error("wifi_chip_hw_power_on -- end \r\n");

    /* Excute SDIO SW hook flow */
    do {
        status = hal_sdio_init(HAL_SDIO_PORT_0, &sdio_config);
        log_hal_error("KH hal_sdio_init status = %d \r\n", status);
        if (HAL_SDIO_STATUS_OK != status) {
            log_hal_error("sdio init error. \r\n");
        }
        hal_gpt_delay_ms(10);
    } while (HAL_SDIO_STATUS_OK != status);

    status = hal_sdio_set_block_size(HAL_SDIO_PORT_0, HAL_SDIO_FUNCTION_1, 256);
    log_hal_error("KH hal_sdio_set_block_size status = %d \r\n", status);
    if (HAL_SDIO_STATUS_OK != status) {
        log_hal_error("sdio set block size error. \r\n");
    }

    status = hal_sdio_get_block_size(HAL_SDIO_PORT_0, HAL_SDIO_FUNCTION_1, &blk_size);
    log_hal_error("KH hal_sdio_get_block_size status = %d \r\n", status);
    if (HAL_SDIO_STATUS_OK != status) {
        log_hal_error("sdio get block size error. \r\n");
    } else {
        log_hal_error("sdio get block size, block size is %d. \r\n", blk_size);
    }

    sdio_cmd53_read(0x0, 0x4, (uint32_t)(&val));
    log_hal_error("chip_id : 0x%08x \n", val);

    sdio_cmd53_write(0x4, 0x4, 0x00000200);
    log_hal_error("4bit write done \n");

    sdio_cmd53_read(0x0, 0x4, (uint32_t)(&val));
    log_hal_error("chip_id : 0x%08x \n", val);
    return UT_STATUS_OK;
}
#endif /* #ifdef DESENSE_TEST */
#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_SDIO_MODULE_ENABLE) */
