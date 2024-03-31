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
#include "hal_sdio_slave.h"
#include "hal_sdio_slave_internal.h"
#include "memory_map.h"
#include "common.h"


#if defined(UT_HAL_ENABLE) && defined (UT_HAL_SDIO_SLAVE_MODULE_ENABLE)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
volatile hal_sdio_slave_callback_event_t ut_test_event = HAL_SDIO_SLAVE_EVENT_NONE;
volatile int tx_count = 0;
volatile int rx_count = 0;
uint32_t *slt_sdio_slave_send_buf;
uint32_t *slt_sdio_slave_receive_buf;

uint32_t UT_GPIO[2] = {0};
static void UT_SDIO_GPIO_Setting(void)
{
    UT_GPIO[0] = READ_REG(0x30404300);
    UT_GPIO[1] = READ_REG(0x30404310);
    WRITE_REG(((UT_GPIO[0] & 0x00FFFFFF) |  0x11000000), 0x30404300);
    WRITE_REG(((UT_GPIO[1] & 0xFFFF0000) |  0x00001111), 0x30404310);
}
static void UT_GPIO_Restore(void)
{
    WRITE_REG(UT_GPIO[0], 0x30404300);
    WRITE_REG(UT_GPIO[1], 0x30404310);
}

void ut_sdio_slave_test_callback(hal_sdio_slave_callback_event_t event,  void *data, void *user_data)
{
    if (event == HAL_SDIO_SLAVE_EVENT_TX1_DONE) {
        tx_count++;
    }
    if (event == HAL_SDIO_SLAVE_EVENT_RX0_DONE || event == HAL_SDIO_SLAVE_EVENT_RX1_DONE) {
        rx_count++;
    }
    if (event == HAL_SDIO_SLAVE_EVENT_SW_INTERRUPT) {
        uint32_t mailbox1;
        uint32_t mailbox2;
        printf("SW INT No: 0x%08lx \r\n", ((hal_sdio_slave_callback_sw_interrupt_parameter_t *)data)-> hal_sdio_slave_sw_interrupt_number);
        hal_sdio_slave_read_mailbox(HAL_SDIO_SLAVE_PORT_0, 0, &mailbox1);
        hal_sdio_slave_read_mailbox(HAL_SDIO_SLAVE_PORT_0, 1, &mailbox2);
        printf("Mailbox1 : 0x%08lx, Mailbox2 : 0x%08lx \r\n", mailbox1, mailbox2);
    }
    ut_test_event =  event;
    //test_event = (test_event|  event);
}

int ut_compare(int pattern, uint32_t *buf, int length)
{
    int i = 0;
    for (i = 0 ; i < length ; i++) {
        if (buf[i] != pattern + i) {
            return 1;
        }
    }
    return 0;
}

int rx_basic(int length, int q_id, int rx_enhance)
{
    int i = 0;
    //memset(&slt_sdio_slave_send_buf, 0, 1024*4);
    slt_sdio_slave_send_buf = (uint32_t *)pvPortMallocNC(1024 * 4);
    for (i = 0; i < 1024; i++) {
        slt_sdio_slave_send_buf[i] = 0x5a5a5a5a + i;
    }
    /*if (rx_enhance)
    {
        length = length - ENHANCE_DATA_LEN;
    }*/
    printf("sdio slave start send data to RXQ.\r\n");
    printf("buf address : 0x%08lx \r\n", (uint32_t)slt_sdio_slave_send_buf);
    if (HAL_SDIO_SLAVE_STATUS_OK != hal_sdio_slave_send_dma(HAL_SDIO_SLAVE_PORT_0, q_id,  slt_sdio_slave_send_buf, length)) {
        printf("sdio slave send error.\r\n");
        return -2;
    }
    //wait rx_rdy
    if ((sdio_slave_rx_queue_id_t)q_id == SDIO_SLAVE_RX_QUEUE_0) {
        while (ut_test_event != HAL_SDIO_SLAVE_EVENT_RX0_DONE);
    } else if ((sdio_slave_rx_queue_id_t)q_id == SDIO_SLAVE_RX_QUEUE_1) {
        while (ut_test_event != HAL_SDIO_SLAVE_EVENT_RX1_DONE);
    }
    ut_test_event = HAL_SDIO_SLAVE_EVENT_NONE;
    vPortFreeNC(slt_sdio_slave_send_buf);

    return 0;
}

int tx_basic(int length)
{
    ut_test_event = HAL_SDIO_SLAVE_EVENT_NONE;
    slt_sdio_slave_receive_buf = (uint32_t *)pvPortMallocNC(1024 * 4);
    memset(slt_sdio_slave_receive_buf, 0, (1024 * 4));
    printf("buf address : 0x%08lx \r\n", (uint32_t)slt_sdio_slave_receive_buf);
    if (HAL_SDIO_SLAVE_STATUS_OK != hal_sdio_slave_receive_dma(HAL_SDIO_SLAVE_PORT_0, HAL_SDIO_SLAVE_TX_QUEUE_1, slt_sdio_slave_receive_buf, length)) {
        printf("sdio slave receive error.\r\n");
        return -3;
    }

    while (ut_test_event != HAL_SDIO_SLAVE_EVENT_TX1_DONE);
    ut_test_event = HAL_SDIO_SLAVE_EVENT_NONE;

    while (ut_compare(0xaa55aa55, slt_sdio_slave_receive_buf, length / 4));
    vPortFreeNC(slt_sdio_slave_receive_buf);
    printf("sdio slave receive ok.\r\n");
    return 0;
}

int sdio_slv_test_func(void)
{

    uint32_t restore_setting2;

    printf("Enable SMPU Security \r\n");

    restore_setting2 = READ_REG(0x30000230);

    //Disable MPU to make SDIO can accesss SYSRAM(0x8000_0000)
    WRITE_REG(0x0, 0x30000230);



    if (0 > hal_sdio_slave_init(HAL_SDIO_SLAVE_PORT_0)) {
        printf("sdio slave init error. \r\n");
        return -1;
    } else {
        printf("sdio slave init ok. \r\n");

    }
    //set sdio callback
    hal_sdio_slave_register_callback(HAL_SDIO_SLAVE_PORT_0, ut_sdio_slave_test_callback, NULL);

    printf("waiting Drv own. \r\n");
    while (hal_sdio_slave_check_fw_own());
    printf("Drv own. \r\n");

    //rx que0 with rx enhance mode
    printf("RXQ0 Testing \r\n");
    rx_basic(4092, 0, 1);
    printf("RXQ1 Testing \r\n");
    rx_basic(4092, 1, 1);
    printf("TXQ1 Testing \r\n");
    tx_basic(4092);


    printf("Write D2H Mailbox1\r\n");
    hal_sdio_slave_write_mailbox(HAL_SDIO_SLAVE_PORT_0, 0, 0x11111111);
    hal_sdio_slave_trigger_d2h_interrupt(HAL_SDIO_SLAVE_PORT_0, 8); //
    printf("Write D2H Mailbox2 \r\n");
    hal_sdio_slave_write_mailbox(HAL_SDIO_SLAVE_PORT_0, 1, 0x22222222);
    hal_sdio_slave_trigger_d2h_interrupt(HAL_SDIO_SLAVE_PORT_0, 9); //

    printf("H2D will show in interrupt \r\n");


    printf("SDIO UT End! SMPU Restore\r\n");

    WRITE_REG(restore_setting2, 0x30000230);

    //hal_sdio_slave_deinit(HAL_SDIO_SLAVE_PORT_0);
    //hal_sdio_slave_register_callback(HAL_SDIO_SLAVE_PORT_0, NULL, NULL);
    return 0;
}
ut_status_t ut_hal_sdio_slave(void)
{
    int ret = 0;
    UT_SDIO_GPIO_Setting();
    ret = sdio_slv_test_func();
    ret = sdio_slv_test_func();
    ret = sdio_slv_test_func();
    UT_GPIO_Restore();

    //set sdio callback , test H2D mailbox
    //hal_sdio_slave_register_callback(HAL_SDIO_SLAVE_PORT_0, ut_sdio_slave_test_callback, NULL);
    if (ret == 0) {
        return UT_STATUS_OK;
    } else {
        return UT_STATUS_ERROR;
    }
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_SDIO_SLAVE_MODULE_ENABLE) */
