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
#include "ci_cli.h"
#include "memory_attribute.h"
#include "hal.h"
#include "hal_gpt.h"
#include "hal_nvic.h"
#include "hal_gpio_internal.h"
#include "ci_sdios.h"
#include "hal_sdio_slave.h"
#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

static hal_gpio_pin_t _Trigger_GPIO_PIN = HAL_GPIO_16;


static void GPIO_notification_config(void)
{

    hal_gpio_init(_Trigger_GPIO_PIN);
    hal_gpio_set_output(_Trigger_GPIO_PIN,  HAL_GPIO_DATA_HIGH);
    hal_gpio_set_direction(_Trigger_GPIO_PIN, HAL_GPIO_DIRECTION_OUTPUT);
    hal_pinmux_set_function(_Trigger_GPIO_PIN, MT7933_PIN_16_FUNC_GPIO16);

}


volatile int tx_count = 0;
volatile int rx0_count = 0;
volatile int rx1_count = 0;
volatile int sw_count = 0;
ATTR_ZIDATA_IN_NONCACHED_SYSRAM_4BYTE_ALIGN uint32_t sdio_slave_send_test_buf[1024];
ATTR_ZIDATA_IN_NONCACHED_SYSRAM_4BYTE_ALIGN uint32_t sdio_slave_receive_test_buf[1024];

void sdio_user_callback(hal_sdio_slave_callback_event_t event,  void *data, void *user_data)
{
    if (event == HAL_SDIO_SLAVE_EVENT_TX1_DONE) {
        tx_count++;
        //log_hal_error("tx done!");

    }
    if (event == HAL_SDIO_SLAVE_EVENT_RX0_DONE) {
        rx0_count++;
        //log_hal_error("rx done!");
    }
    if (event == HAL_SDIO_SLAVE_EVENT_RX1_DONE) {
        rx1_count++;
        //log_hal_error("rx done!");
    }

    if (event == HAL_SDIO_SLAVE_EVENT_SW_INTERRUPT) {
        sw_count++;
    }

}



ci_status_t ci_sdios_sample_main(unsigned int portnum)
{

    printf("%s get portnum:%u\n", __FUNCTION__, portnum);

    int cnt = 0;
    uint32_t D2H_msg = 0x87654321;
    uint32_t H2D_msg = 0x12345678;
    uint32_t pattern = 0x5a5a5a5a;
    hal_sdio_slave_status_t status = HAL_SDIO_SLAVE_STATUS_ERROR;


#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_SDIO);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    if (0 > hal_sdio_slave_init(HAL_SDIO_SLAVE_PORT_0)) {
        printf("sdio slave init error. \r\n");
        goto Error_Handler;
    }
    hal_sdio_slave_register_callback(HAL_SDIO_SLAVE_PORT_0, sdio_user_callback, NULL);

    //notify host slave is already ready
    GPIO_notification_config();
    for (int i = 0; i < (4092 + 3) / 4; i++) {
        sdio_slave_send_test_buf[i] = pattern + i;
    }
    memset(sdio_slave_receive_test_buf, 0, 4096);
    while (1) {
        //about 5s Timeout
        if (cnt >= 500) {
            printf("T/RX Timeout or DRV OWN Timeout tx_count: %d, rx0_count: %d, rx1_count: %d, slave HWFICR : 0x%08lx \r\n", tx_count, rx0_count, rx1_count, SDIO_SLAVE_REG->HWFICR);
            goto Error_Handler;
        }

        if (false == hal_sdio_slave_check_fw_own()) {
            break;
        }
        hal_gpt_delay_ms(10);
        cnt += 1;
    }
    //host rx
    status = hal_sdio_slave_send_dma(HAL_SDIO_SLAVE_PORT_0, HAL_SDIO_SLAVE_RX_QUEUE_0, (uint32_t *)sdio_slave_send_test_buf, 4092);
    if (status != HAL_SDIO_SLAVE_STATUS_OK) {
        printf("sdio slave send  error. RXQ0 fail! Error code : %d \r\n", status);
        goto Error_Handler;
    }
    //host tx
    status = hal_sdio_slave_receive_dma(HAL_SDIO_SLAVE_PORT_0, HAL_SDIO_SLAVE_TX_QUEUE_1, (uint32_t *)sdio_slave_receive_test_buf, 4092);
    if (status != HAL_SDIO_SLAVE_STATUS_OK) {
        printf("sdio slave receive error. Error code : %d \r\n", status);
        goto Error_Handler;
    }
    // trigger interrupt(BIT 28) to  Host and send mailbox
    hal_sdio_slave_write_mailbox(HAL_SDIO_SLAVE_PORT_0, 0, D2H_msg);
    hal_sdio_slave_trigger_d2h_interrupt(HAL_SDIO_SLAVE_PORT_0, 20);

    //waiting interrupt and checking H2D mailbox
    while (!sw_count) {
        if (cnt >= 700) {
            printf("Host checking Timeout! \n");
            goto Error_Handler;
        }
        hal_gpt_delay_ms(10);
        cnt += 1;
    }
    hal_sdio_slave_read_mailbox(HAL_SDIO_SLAVE_PORT_0, 0, &H2D_msg);
    printf("Slave receive [0] : 0x%08lx [1022] : 0x%08lx \n", sdio_slave_receive_test_buf[0], sdio_slave_receive_test_buf[1022]);
    if (H2D_msg == 0x12345678)
        goto SUCCESS;

Error_Handler:
    hal_sdio_slave_deinit(HAL_SDIO_SLAVE_PORT_0);
    hal_gpio_set_output(_Trigger_GPIO_PIN,  HAL_GPIO_DATA_LOW);
    sw_count = 0;
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_SDIO);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    return CI_FAIL;

SUCCESS:
    hal_sdio_slave_deinit(HAL_SDIO_SLAVE_PORT_0);
    hal_gpio_set_output(_Trigger_GPIO_PIN,  HAL_GPIO_DATA_LOW);
    sw_count = 0;
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_SDIO);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    return CI_PASS;
}
