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
#include "FreeRTOS.h"
#include "hal_gpt.h"
#include "task.h"

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_SPI_SLAVE_MODULE_ENABLE)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void wakeup_gpt_callback(void *user_data)
{
    hal_gpt_stop_timer(HAL_GPT_1);
}

void spis_sleep_function(void)
{
    hal_gpt_init(HAL_GPT_1);
    hal_gpt_register_callback(HAL_GPT_1, wakeup_gpt_callback, NULL);
    hal_gpt_stop_timer(HAL_GPT_1);
    hal_gpt_start_timer_ms(HAL_GPT_1, 500, HAL_GPT_TIMER_TYPE_ONE_SHOT);
    vTaskDelay(50);
    hal_sleep_manager_enter_sleep_mode(HAL_SLEEP_MODE_SLEEP);
    hal_gpt_deinit(HAL_GPT_1);
}

uint8_t ut_hal_spim2spis_data_check(uint8_t *send_data_master, uint8_t *receive_data_master,
                                    uint8_t *send_data_slave, uint8_t *receive_data_slave,
                                    uint8_t size)
{
    uint8_t i = 0;
    uint8_t ret = 0;

    printf("enter data check-----------------------------------------------\r\n");
    for (i = 0; i < size; i++) {
        if (send_data_master != NULL && receive_data_slave != NULL) {
            printf("send_data_master:%d, receive_data_slave:%d\r\n", *(send_data_master + i), *(receive_data_slave + i));
            if (*(send_data_master + i) != *(receive_data_slave + i)) {
                //printf("send_data_master:%d, receive_data_slave:%d\r\n",*(send_data_master + i),*(receive_data_slave + i));
                ret++;
            }
        }
        if (send_data_slave != NULL && receive_data_master != NULL) {
            printf("send_data_slave:%d, receive_data_master:%d\r\n", *(send_data_slave + i), *(receive_data_master + i));
            if (*(send_data_slave + i) != *(receive_data_master + i)) {
                //printf("send_data_slave:%d, receive_data_master:%d\r\n",*(send_data_slave + i),*(receive_data_master + i));
                ret++;
            }
        }
    }

    return ret;
}

// Callback function sample code. Pass this function to the driver while calling #hal_spi_master_register_callback().
void user_spis_callback(hal_spi_slave_transaction_status_t event, void *user_data)
{
    printf("enter spis_send_and_receive_dma user callback!\r\n");
}

ut_status_t ut_hal_spi_slave(void)
{
    hal_spi_master_config_t spim_config;
    hal_spi_slave_config_t spis_config;
    hal_spi_master_chip_select_timing_t spim_chip_select;
    hal_spi_master_send_and_receive_config_t spim_send_and_receive_config;
    uint8_t *spim_send_data = NULL;
    uint8_t *spim_receive_data = NULL;
    uint8_t *spis_send_data = NULL;
    uint8_t *spis_receive_data = NULL;
    uint32_t spim_size;
    uint32_t spis_size;
    uint32_t ret, i;

    printf("/************************enter SPIS test**********************/\r\n");
    writel(0xc0000021, 0x30020288); //spis:200M(default:26M)
    // Initialize the GPIO, set GPIO pinmux.
    hal_gpio_init(HAL_GPIO_13);
    hal_gpio_init(HAL_GPIO_14);
    hal_gpio_init(HAL_GPIO_15);
    hal_gpio_init(HAL_GPIO_16);
    hal_gpio_init(HAL_GPIO_25);
    hal_gpio_init(HAL_GPIO_26);
    hal_gpio_init(HAL_GPIO_27);
    hal_gpio_init(HAL_GPIO_28);
    // Call hal_pinmux_set_function() to set GPIO pinmux, for more information, please refer to hal_pinmux_define.h
    hal_pinmux_set_function(HAL_GPIO_13, 2);
    hal_pinmux_set_function(HAL_GPIO_14, 2);
    hal_pinmux_set_function(HAL_GPIO_15, 2);
    hal_pinmux_set_function(HAL_GPIO_16, 2);
    hal_pinmux_set_function(HAL_GPIO_25, 7);
    hal_pinmux_set_function(HAL_GPIO_26, 7);
    hal_pinmux_set_function(HAL_GPIO_27, 7);
    hal_pinmux_set_function(HAL_GPIO_28, 5);

    spim_config.bit_order = HAL_SPI_MASTER_MSB_FIRST;
    spim_config.clock_frequency = 15000000;
    spim_config.phase = HAL_SPI_MASTER_CLOCK_PHASE0;
    spim_config.polarity = HAL_SPI_MASTER_CLOCK_POLARITY0;
    spis_config.phase = HAL_SPI_SLAVE_CLOCK_PHASE0;
    spis_config.polarity = HAL_SPI_SLAVE_CLOCK_POLARITY0;
    spis_config.bit_order = HAL_SPI_SLAVE_MSB_FIRST;
    spis_config.endian = HAL_SPI_SLAVE_LITTLE_ENDIAN;

    ret = hal_spi_slave_init(HAL_SPI_SLAVE_0, &spis_config);
    if (ret != 0) {
        printf("spis_init fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = hal_spi_master_init(HAL_SPI_MASTER_1, &spim_config);
    if (ret != 0) {
        printf("spim_init fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = hal_spi_slave_register_callback(HAL_SPI_SLAVE_0, user_spis_callback, NULL);
    if (ret != 0) {
        printf("spis_register_callback fail! error:%ld\r\n", ret);
        goto test_error;
    }

    //test1 spim_send_polling_to_spis_receive(async)
    spis_sleep_function();
    spim_size = 32;
    spis_size = 32;
    spim_send_data = (uint8_t *)pvPortMallocNC(spim_size);
    spim_receive_data = (uint8_t *)pvPortMallocNC(spim_size);
    spis_send_data = (uint8_t *)pvPortMallocNC(spis_size);
    spis_receive_data = (uint8_t *)pvPortMallocNC(spis_size);

    for (i = 0; i < spim_size; i++) {
        *(spim_send_data + i)   = 0x01 + i;
        *(spim_receive_data + i)    = 0x00;
    }
    for (i = 0; i < spis_size; i++) {
        *(spis_send_data + i)   = 0xa1 + i;
        *(spis_receive_data + i)    = 0x00;
    }
    spim_send_and_receive_config.receive_length = spim_size;
    spim_send_and_receive_config.send_length = spim_size;
    spim_send_and_receive_config.send_data = spim_send_data;
    spim_send_and_receive_config.receive_buffer = spim_receive_data;

    ret = hal_spi_slave_receive(HAL_SPI_SLAVE_0, spis_receive_data, spis_size);
    if (ret != 0) {
        printf("spis_receive fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = hal_spi_master_send_polling(HAL_SPI_MASTER_1, spim_send_data, spim_size);
    if (ret != 0) {
        printf("spim_send_polling fail! error:%ld\r\n", ret);
        goto test_error;
    }

    ret = ut_hal_spim2spis_data_check(spim_send_data, NULL, NULL, spis_receive_data, spis_size);
    if (ret == 0)
        printf("test1 spim_send_polling_to_spis_receive(async) pass!\r\n");
    else {
        printf("test1 spim_send_polling_to_spis_receive(async) fail! error:%ld\r\n", ret);
        goto test_error;
    }
    vPortFreeNC(spim_send_data);
    vPortFreeNC(spim_receive_data);
    vPortFreeNC(spis_send_data);
    vPortFreeNC(spis_receive_data);

    //test2 spim_send_dma_bocking_to_spis_receive(async)
    spis_sleep_function();
    spim_size = 32;
    spis_size = 32;
    spim_send_data = (uint8_t *)pvPortMallocNC(spim_size);
    spim_receive_data = (uint8_t *)pvPortMallocNC(spim_size);
    spis_send_data = (uint8_t *)pvPortMallocNC(spis_size);
    spis_receive_data = (uint8_t *)pvPortMallocNC(spis_size);
    for (i = 0; i < spim_size; i++) {
        *(spim_send_data + i)   = 0x01 + i;
        *(spim_receive_data + i)    = 0x00;
    }
    for (i = 0; i < spis_size; i++) {
        *(spis_send_data + i)   = 0xa1 + i;
        *(spis_receive_data + i)    = 0x00;
    }
    spim_send_and_receive_config.receive_length = spim_size;
    spim_send_and_receive_config.send_length = spim_size;
    spim_send_and_receive_config.send_data = spim_send_data;
    spim_send_and_receive_config.receive_buffer = spim_receive_data;

    ret = hal_spi_slave_receive(HAL_SPI_SLAVE_0, spis_receive_data, spis_size);
    if (ret != 0) {
        printf("spis_receive fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = hal_spi_master_send_dma_blocking(HAL_SPI_MASTER_1, spim_send_data, spim_size);
    if (ret != 0) {
        printf("spim_send_dma_blocking fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = ut_hal_spim2spis_data_check(spim_send_data, NULL, NULL, spis_receive_data, spis_size);
    if (ret == 0)
        printf("test2 spim_send_dma_blocking_to_spis_receive(async) pass!\r\n");
    else {
        printf("test2 spim_send_dma_blocking_to_spis_receive(async) fail! error:%ld\r\n", ret);
        goto test_error;
    }
    vPortFreeNC(spim_send_data);
    vPortFreeNC(spim_receive_data);
    vPortFreeNC(spis_send_data);
    vPortFreeNC(spis_receive_data);

    //test3 spim2spi_send_and_receive(async)
    spis_sleep_function();
    spim_chip_select.chip_select_setup_count = 100;
    spim_chip_select.chip_select_hold_count = 100;
    spim_chip_select.chip_select_idle_count = 100;

    hal_spi_master_set_chip_select_timing(HAL_SPI_MASTER_1, spim_chip_select);

    spim_size = 32;
    spis_size = 32;
    spim_send_data = (uint8_t *)pvPortMallocNC(spim_size);
    spim_receive_data = (uint8_t *)pvPortMallocNC(spim_size);
    spis_send_data = (uint8_t *)pvPortMallocNC(spis_size);
    spis_receive_data = (uint8_t *)pvPortMallocNC(spis_size);
    for (i = 0; i < spim_size; i++) {
        *(spim_send_data + i)   = 0x01 + i;
        *(spim_receive_data + i)    = 0x00;
    }
    for (i = 0; i < spis_size; i++) {
        *(spis_send_data + i)   = 0xa1 + i;
        *(spis_receive_data + i)    = 0x00;
    }
    spim_send_and_receive_config.receive_length = spim_size;
    spim_send_and_receive_config.send_length = spim_size;
    spim_send_and_receive_config.send_data = spim_send_data;
    spim_send_and_receive_config.receive_buffer = spim_receive_data;

    ret = hal_spi_slave_send_and_receive(HAL_SPI_SLAVE_0, spis_send_data, spis_receive_data, spis_size);
    if (ret != 0) {
        printf("spis_send_and_receive fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = hal_spi_master_send_and_receive_dma_blocking(HAL_SPI_MASTER_1, &spim_send_and_receive_config);
    if (ret != 0) {
        printf("spim_send_and_receive_dma_blocking fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = ut_hal_spim2spis_data_check(spim_send_data, spim_receive_data,
                                      spis_send_data, spis_receive_data, spis_size);
    if (ret == 0)
        printf("test3 spim2spi_send_and_receive(async) pass!\r\n");
    else {
        printf("test3 spim2spi_send_and_receive(async) fail! error:%ld\r\n", ret);
        goto test_error;
    }
    vPortFreeNC(spim_send_data);
    vPortFreeNC(spim_receive_data);
    vPortFreeNC(spis_send_data);
    vPortFreeNC(spis_receive_data);

    hal_spi_master_deinit(HAL_SPI_MASTER_1);
    hal_spi_slave_deinit(HAL_SPI_SLAVE_0);
    return UT_STATUS_OK;

test_error:
    vPortFreeNC(spim_send_data);
    vPortFreeNC(spim_receive_data);
    vPortFreeNC(spis_send_data);
    vPortFreeNC(spis_receive_data);
    hal_spi_master_deinit(HAL_SPI_MASTER_1);
    hal_spi_slave_deinit(HAL_SPI_SLAVE_0);
    return UT_STATUS_ERROR;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_SPI_SLAVE_MODULE_ENABLE) */
