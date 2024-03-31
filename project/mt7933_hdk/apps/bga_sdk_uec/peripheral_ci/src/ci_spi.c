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
#include "hal_spi_master.h"
#include "hal_spi_slave.h"


int done;

// Callback function sample code. Pass this function to the driver while calling #hal_spi_master_register_callback().
void user_spim_callback(hal_spi_master_callback_event_t event, void *user_data)
{
    if (HAL_SPI_MASTER_EVENT_SEND_FINISHED == event) {
        printf("enter spim_send_dma finish user callback!\r\n");
    } else if (HAL_SPI_MASTER_EVENT_RECEIVE_FINISHED == event) {
        printf("enter spim_send_and_receive_dma finish user callback!\r\n");
    }
    done = 1;
}

static uint8_t ut_hal_spim2spis_data_check(uint8_t *send_data_master, uint8_t *receive_data_master,
                                           uint8_t *send_data_slave, uint8_t *receive_data_slave,
                                           uint8_t size)
{
    uint8_t i = 0;
    uint8_t ret = 0;

    //printf("-------------------enter data check-------------------\r\n");
    for (i = 0; i < size; i++) {
        if (send_data_master != NULL && receive_data_slave != NULL) {
            //printf("send_data_master:%d, receive_data_slave:%d\r\n", *(send_data_master + i), *(receive_data_slave + i));
            if (*(send_data_master + i) != *(receive_data_slave + i)) {
                printf("send_data_master:%d, receive_data_slave:%d\r\n", *(send_data_master + i), *(receive_data_slave + i));
                ret++;
            }
        }
        if (send_data_slave != NULL && receive_data_master != NULL) {
            //printf("send_data_slave:%d, receive_data_master:%d\r\n", *(send_data_slave + i), *(receive_data_master + i));
            if (*(send_data_slave + i) != *(receive_data_master + i)) {
                printf("send_data_slave:%d, receive_data_master:%d\r\n", *(send_data_slave + i), *(receive_data_master + i));
                ret++;
            }
        }
    }

    return ret;
}

// Callback function sample code. Pass this function to the driver while calling #hal_spi_slave_register_callback().
static void user_spis_callback(hal_spi_slave_transaction_status_t event, void *user_data)
{
    printf("enter spis_send_and_receive_dma user callback!\r\n");
}

static ci_status_t ci_spim2spis_api_test(void)
{

    hal_spi_master_config_t spim_config;
    hal_spi_slave_config_t spis_config;
    hal_spi_master_send_and_receive_config_t spim_send_and_receive_config;
    uint8_t *spim_send_data = NULL;
    uint8_t *spim_receive_data = NULL;
    uint8_t *spis_send_data = NULL;
    uint8_t *spis_receive_data = NULL;
    uint32_t spim_size;
    uint32_t spis_size;
    hal_spi_master_running_status_t running_status = HAL_SPI_MASTER_IDLE;
    hal_spi_master_advanced_config_t advanced_config;
    hal_spi_master_chip_select_timing_t chip_select_timing;
    uint32_t ret, i;

    done = 0;
    printf("/************************enter SPIM2SPIS test**********************/\r\n");
    hal_gpio_init(HAL_GPIO_13);
    hal_gpio_init(HAL_GPIO_14);
    hal_gpio_init(HAL_GPIO_15);
    hal_gpio_init(HAL_GPIO_16);
    hal_gpio_init(HAL_GPIO_25);
    hal_gpio_init(HAL_GPIO_26);
    hal_gpio_init(HAL_GPIO_27);
    hal_gpio_init(HAL_GPIO_28);
    hal_pinmux_set_function(HAL_GPIO_13, 2);
    hal_pinmux_set_function(HAL_GPIO_14, 2);
    hal_pinmux_set_function(HAL_GPIO_15, 2);
    hal_pinmux_set_function(HAL_GPIO_16, 2);
    hal_pinmux_set_function(HAL_GPIO_25, 7);
    hal_pinmux_set_function(HAL_GPIO_26, 7);
    hal_pinmux_set_function(HAL_GPIO_27, 7);
    hal_pinmux_set_function(HAL_GPIO_28, 5);

    spim_config.clock_frequency = 200000;
    spim_config.phase = HAL_SPI_MASTER_CLOCK_PHASE0;
    spim_config.polarity = HAL_SPI_MASTER_CLOCK_POLARITY0;
    spim_config.bit_order = HAL_SPI_MASTER_MSB_FIRST;

    spis_config.phase = HAL_SPI_SLAVE_CLOCK_PHASE0;
    spis_config.polarity = HAL_SPI_SLAVE_CLOCK_POLARITY0;
    spis_config.bit_order = HAL_SPI_SLAVE_MSB_FIRST;
    spis_config.endian = HAL_SPI_SLAVE_LITTLE_ENDIAN;

    advanced_config.byte_order = HAL_SPI_MASTER_LITTLE_ENDIAN;
    advanced_config.chip_polarity = HAL_SPI_MASTER_CHIP_SELECT_LOW;
    advanced_config.get_tick = HAL_SPI_MASTER_NO_GET_TICK_MODE;
    advanced_config.sample_select = HAL_SPI_MASTER_SAMPLE_POSITIVE;

    chip_select_timing.chip_select_setup_count = 100;
    chip_select_timing.chip_select_hold_count = 100;
    chip_select_timing.chip_select_idle_count = 100;

    spim_size = 32;
    spis_size = 32;
    spim_send_data = (uint8_t *)pvPortMallocNC(spim_size);
    spim_receive_data = (uint8_t *)pvPortMallocNC(spim_size);
    spis_send_data = (uint8_t *)pvPortMallocNC(spis_size);
    spis_receive_data = (uint8_t *)pvPortMallocNC(spis_size);

    for (i = 0; i < spim_size; i++) {
        *(spim_send_data + i)   = 0x01 + i;
        *(spis_send_data + i)   = 0xa1 + i;
    }

    spim_send_and_receive_config.send_length = spim_size;
    spim_send_and_receive_config.send_data = spim_send_data;
    spim_send_and_receive_config.receive_length = spim_size;
    spim_send_and_receive_config.receive_buffer = spim_receive_data;

    /*spis_api1:slave_init*/
    ret = hal_spi_slave_init(HAL_SPI_SLAVE_0, &spis_config);
    if (ret != 0) {
        printf("spis_init fail! error:%ld\r\n", ret);
        goto test_error;
    }
    /*spim_api1:master_init*/
    ret = hal_spi_master_init(HAL_SPI_MASTER_1, &spim_config);
    if (ret != 0) {
        printf("spim_init fail! error:%ld\r\n", ret);
        goto test_error;
    }

    ret = hal_spi_master_set_advanced_config(HAL_SPI_MASTER_1, &advanced_config);
    if (ret != 0) {
        printf("spi_master_set_advanced_config fail! error:%ld\r\n", ret);
        goto test_error;
    }

    ret = hal_spi_master_set_chip_select_timing(HAL_SPI_MASTER_1, chip_select_timing);
    if (ret != 0) {
        printf("hal_spi_master_set_chip_select_timing fail! error:%ld\r\n", ret);
        goto test_error;
    }

    /*spim_api2:master_get_running_status*/
    ret = hal_spi_master_get_running_status(HAL_SPI_MASTER_1, &running_status);
    if (ret != 0) {
        printf("spim_get_running_status fail! error:%ld\r\n", ret);
        goto test_error;
    }
    if (running_status == HAL_SPI_MASTER_BUSY) {
        printf("spim_get_running_status: HAL_SPI_MASTER_BUSY.\r\n");
        goto test_error;
    }
    /*spis_api2:slave_register_callback*/
    ret = hal_spi_slave_register_callback(HAL_SPI_SLAVE_0, user_spis_callback, NULL);
    if (ret != 0) {
        printf("spis_register_callback fail! error:%ld\r\n", ret);
        goto test_error;
    }
    printf("---------------------enter spim2spis test1.---------------------\r\n");
    /*spis_api3:slave_receive*/
    for (i = 0; i < spim_size; i++)
        *(spis_receive_data + i)    = 0x00;

    ret = hal_spi_slave_receive(HAL_SPI_SLAVE_0, spis_receive_data, spis_size);
    if (ret != 0) {
        printf("spis_receive fail! error:%ld\r\n", ret);
        goto test_error;
    }
    /*spim_api3:master_send_polling*/
    ret = hal_spi_master_send_polling(HAL_SPI_MASTER_1, spim_send_data, spim_size);
    if (ret != 0) {
        printf("spim_send_polling fail! error:%ld\r\n", ret);
        goto test_error;
    }

    ret = ut_hal_spim2spis_data_check(spim_send_data, NULL, NULL, spis_receive_data, spis_size);
    if (ret == 0)
        printf("test1 pass!\r\n");
    else {
        printf("test1 fail! error:%ld\r\n", ret);
        goto test_error;
    }

    printf("---------------------enter spim2spis test2.---------------------\r\n");
    for (i = 0; i < spim_size; i++)
        *(spis_receive_data + i)    = 0x00;

    ret = hal_spi_slave_receive(HAL_SPI_SLAVE_0, spis_receive_data, spis_size);
    if (ret != 0) {
        printf("spis_receive fail! error:%ld\r\n", ret);
        goto test_error;
    }
    /*spim_api4:master_send_dma_blocking*/
    ret = hal_spi_master_send_dma_blocking(HAL_SPI_MASTER_1, spim_send_data, spim_size);
    if (ret != 0) {
        printf("spim_send_dma_blocking fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = ut_hal_spim2spis_data_check(spim_send_data, NULL, NULL, spis_receive_data, spis_size);
    if (ret == 0)
        printf("test2 pass!\r\n");
    else {
        printf("test2 fail! error:%ld\r\n", ret);
        goto test_error;
    }

    printf("---------------------enter spim2spis test3.---------------------\r\n");
    for (i = 0; i < spim_size; i++)
        *(spis_receive_data + i)    = 0x00;

    /*spim_api5:master_set_advanced_config*/
    ret = hal_spi_master_set_advanced_config(HAL_SPI_MASTER_1, &advanced_config);
    if (ret != 0) {
        printf("spi_master_set_advanced_config fail! error:%ld\r\n", ret);
        goto test_error;
    }
    /*spim_api6:master_set_chip_select_timing*/
    ret = hal_spi_master_set_chip_select_timing(HAL_SPI_MASTER_1, chip_select_timing);
    if (ret != 0) {
        printf("hal_spi_master_set_chip_select_timing fail! error:%ld\r\n", ret);
        goto test_error;
    }
    /*spim_api7:master_set_deassert*/
    ret =  hal_spi_master_set_deassert(HAL_SPI_MASTER_1, HAL_SPI_MASTER_DEASSERT_ENABLE);
    if (ret != 0) {
        printf("hal_spi_master_set_deassert fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = hal_spi_slave_receive(HAL_SPI_SLAVE_0, spis_receive_data, spis_size);
    if (ret != 0) {
        printf("spis_receive fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = hal_spi_master_send_dma_blocking(HAL_SPI_MASTER_1, spim_send_data, spim_size);
    if (ret == 0)
        printf("test3 pass!\r\n");
    else {
        printf("test3 fail! error:%ld\r\n", ret);
        goto test_error;
    }

    ret =  hal_spi_master_set_deassert(HAL_SPI_MASTER_1, HAL_SPI_MASTER_DEASSERT_DISABLE);
    if (ret != 0) {
        printf("hal_spi_master_set_deassert fail! error:%ld\r\n", ret);
        goto test_error;
    }

    printf("---------------------enter spim2spis test4.---------------------\r\n");
    for (i = 0; i < spim_size; i++) {
        *(spim_receive_data + i)    = 0x00;
        *(spis_receive_data + i)    = 0x00;
    }

    /*spis_api4:slave_send_and_receive*/
    ret = hal_spi_slave_send_and_receive(HAL_SPI_SLAVE_0, spis_send_data, spis_receive_data, spis_size);
    if (ret != 0) {
        printf("spis_send_and_receive fail! error:%ld\r\n", ret);
        goto test_error;
    }
    /*spim_api8:master_send_and_receive_dma_blocking*/
    ret = hal_spi_master_send_and_receive_dma_blocking(HAL_SPI_MASTER_1, &spim_send_and_receive_config);
    if (ret != 0) {
        printf("spim_send_and_receive_dma_blocking fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = ut_hal_spim2spis_data_check(spim_send_data, spim_receive_data,
                                      spis_send_data, spis_receive_data, spis_size);
    if (ret == 0)
        printf("test4 pass!\r\n");
    else {
        printf("test4 fail! error:%ld\r\n", ret);
        goto test_error;
    }

    printf("---------------------enter spim2spis test5.---------------------\r\n");
    for (i = 0; i < spim_size; i++) {
        *(spim_receive_data + i)    = 0x00;
        *(spis_receive_data + i)    = 0x00;
    }

    ret = hal_spi_slave_send_and_receive(HAL_SPI_SLAVE_0, spis_send_data, spis_receive_data, spis_size);
    if (ret != 0) {
        printf("spis_send_and_receive fail! error:%ld\r\n", ret);
        goto test_error;
    }
    /*spim_api9:master_send_and_receive_polling*/
    ret = hal_spi_master_send_and_receive_polling(HAL_SPI_MASTER_1, &spim_send_and_receive_config);
    if (ret != 0) {
        printf("spim_send_and_receive_polling fail! error:%ld\r\n", ret);
        goto test_error;
    }
    ret = ut_hal_spim2spis_data_check(spim_send_data, spim_receive_data,
                                      spis_send_data, spis_receive_data, spis_size);
    if (ret == 0)
        printf("test5 pass!\r\n");
    else {
        printf("test5 fail! error:%ld\r\n", ret);
        goto test_error;
    }

    printf("---------------------enter spim2spis test6.---------------------\r\n");
    for (i = 0; i < spim_size; i++) {
        *(spis_receive_data + i)    = 0x00;
    }

    ret = hal_spi_master_register_callback(HAL_SPI_MASTER_1, user_spim_callback, NULL);
    if (ret != 0) {
        printf("spim_register_callback fail! error:%ld\r\n", ret);
        goto test_error;
    }

    ret = hal_spi_slave_receive(HAL_SPI_SLAVE_0, spis_receive_data, spis_size);
    if (ret != 0) {
        printf("spis_receive fail! error:%ld\r\n", ret);
        goto test_error;
    }

    /*spim_api12:master_send_dma*/
    ret = hal_spi_master_send_dma(HAL_SPI_MASTER_1, spim_send_data, spim_size);
    if (ret != 0) {
        printf("spim_send_and_dma fail! error:%ld\r\n", ret);
        goto test_error;
    }
    while (done == 0) {
        printf("wait transfer done.\r\n");
        vTaskDelay(1000);
    };
    ret = ut_hal_spim2spis_data_check(spim_send_data, NULL, NULL, spis_receive_data, spis_size);
    if (ret == 0)
        printf("test6 pass!\r\n");
    else {
        printf("test6 fail! error:%ld\r\n", ret);
        goto test_error;
    }

    printf("---------------------enter spim2spis test7.---------------------\r\n");
    for (i = 0; i < spim_size; i++) {
        *(spim_receive_data + i)    = 0x00;
        *(spis_receive_data + i)    = 0x00;
    }
    done = 0;

    ret = hal_spi_slave_send_and_receive(HAL_SPI_SLAVE_0, spis_send_data, spis_receive_data, spis_size);
    if (ret != 0) {
        printf("spis_send_and_receive fail! error:%ld\r\n", ret);
        goto test_error;
    }
    /*spim_api11:master_send_and__receive_dma*/
    ret = hal_spi_master_send_and_receive_dma(HAL_SPI_MASTER_1, &spim_send_and_receive_config);
    if (ret != 0) {
        printf("spim_send_and_receive_dma fail! error:%ld\r\n", ret);
        goto test_error;
    }
    while (done == 0) {
        printf("wait transfer done.\r\n");
        vTaskDelay(1000);
    };
    ret = ut_hal_spim2spis_data_check(spim_send_data, spim_receive_data,
                                      spis_send_data, spis_receive_data, spis_size);
    if (ret == 0)
        printf("test7 pass!\r\n");
    else {
        printf("test7 fail! error:%ld\r\n", ret);
        goto test_error;
    }

    vPortFreeNC(spim_send_data);
    vPortFreeNC(spim_receive_data);
    vPortFreeNC(spis_send_data);
    vPortFreeNC(spis_receive_data);

    /*spim_api13:master_deinit*/
    hal_spi_master_deinit(HAL_SPI_MASTER_1);
    /*spis_api5:slave_deinit*/
    hal_spi_slave_deinit(HAL_SPI_SLAVE_0);
    return CI_PASS;

test_error:
    vPortFreeNC(spim_send_data);
    vPortFreeNC(spim_receive_data);
    vPortFreeNC(spis_send_data);
    vPortFreeNC(spis_receive_data);
    hal_spi_master_deinit(HAL_SPI_MASTER_1);
    hal_spi_slave_deinit(HAL_SPI_SLAVE_0);
    return CI_FAIL;
}

ci_status_t ci_spi_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"SPIM2SPIS API test", ci_spim2spis_api_test},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
