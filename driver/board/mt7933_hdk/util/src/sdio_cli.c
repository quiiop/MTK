/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "memory_map.h"
#include "sdio_cli.h"
#include "hal_log.h"
#include "memory_attribute.h"
#include "hal_gpt.h"
#include "hal_nvic.h"
#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

#ifdef IOT_CAM_ENABLE
#include "customer_func.h"
#endif /* #ifdef IOT_CAM_ENABLE */

volatile hal_sdio_slave_callback_event_t test_event = HAL_SDIO_SLAVE_EVENT_NONE;
volatile int host_tx_count = 0;
volatile int host_rx0_count = 0;
volatile int host_rx1_count = 0;
//ATTR_ZIDATA_IN_NONCACHED_SYSRAM_4BYTE_ALIGN uint32_t sdio_slave_send_test_cli_buf[16][1024];
//ATTR_ZIDATA_IN_NONCACHED_SYSRAM_4BYTE_ALIGN uint32_t sdio_receive_receive_test_buf[16][1024];
sdio_slave_interrupt_status_t *debug_status;
uint32_t *sdio_slave_send_test_cli_buf;
uint32_t *sdio_slave_receive_test_cli_buf;


uint32_t isr_1st_TS = 0;
uint32_t isr_2nd_TS = 0;
uint32_t isr_duration[16] = {0};
volatile uint32_t isr_cnt = 0;

static void dump_CR(void)
{
    printf("dumping register====\r\n");
    printf("Global register~~~~~\r\n");
    printf("HGFCR : 0x%08lx\r\n", SDIO_SLAVE_REG->HGFCR);
    printf("HGFISR : 0x%08lx\r\n", SDIO_SLAVE_REG->HGFISR);
    printf("HGFIER : 0x%08lx\r\n\r\n", SDIO_SLAVE_REG->HGFIER);
    printf("WLAN register~~~~~\r\n");
    printf("HWFISR : 0x%08lx\r\n", SDIO_SLAVE_REG->HWFISR);
    printf("HWFIER : 0x%08lx\r\n\r\n", SDIO_SLAVE_REG->HWFIER);
    printf("TXQ INT register~~~~~\r\n");
    printf("HWFTE0SR : 0x%08lx\r\n", SDIO_SLAVE_REG->HWFTE0SR);
    printf("HWFTE0ER : 0x%08lx\r\n\r\n", SDIO_SLAVE_REG->HWFTE0ER);
    printf("RXQ INT register~~~~~\r\n");
    printf("HWFRE0SR : 0x%08lx\r\n", SDIO_SLAVE_REG->HWFRE0SR);
    printf("HWFRE0ER : 0x%08lx\r\n", SDIO_SLAVE_REG->HWFRE0ER);
    printf("HWFRE1SR : 0x%08lx\r\n", SDIO_SLAVE_REG->HWFRE1SR);
    printf("HWFRE1ER : 0x%08lx\r\n\r\n", SDIO_SLAVE_REG->HWFRE1ER);
    printf("WLAN Firmware Interrupt Control~~~~~\r\n");
    printf("HWFICR  : 0x%08lx\r\n", SDIO_SLAVE_REG->HWFICR);
    printf("HWFCR   : 0x%08lx\r\n\r\n", SDIO_SLAVE_REG->HWFCR);
    printf("dumping done====\r\n\r\n");

}


volatile uint32_t error_notify = 0;
void sdio_slave_test_callback(hal_sdio_slave_callback_event_t event,  void *data, void *user_data)
{
    if (event == HAL_SDIO_SLAVE_EVENT_TX1_DONE) {
        host_tx_count++;
    }
    if (event == HAL_SDIO_SLAVE_EVENT_RX0_DONE) {
        host_rx0_count++;
    }
    if (event == HAL_SDIO_SLAVE_EVENT_RX1_DONE) {
        host_rx1_count++;
    }
    if (event == HAL_SDIO_SLAVE_EVENT_SW_INTERRUPT) {
        uint32_t mailbox1;
        uint32_t mailbox2;
        log_hal_info("SW INT No: 0x%08lx \r\n", ((hal_sdio_slave_callback_sw_interrupt_parameter_t *)data)-> hal_sdio_slave_sw_interrupt_number);
        hal_sdio_slave_read_mailbox(HAL_SDIO_SLAVE_PORT_0, 0, &mailbox1);
        hal_sdio_slave_read_mailbox(HAL_SDIO_SLAVE_PORT_0, 1, &mailbox2);
        log_hal_info("Mailbox1 : 0x%08lx, Mailbox2 : 0x%08lx \r\n", mailbox1, mailbox2);
    }
    if (event == HAL_SDIO_SLAVE_EVENT_ERROR) {
        printf("Error occur! \r\n");
        error_notify = 1;

    }
    test_event =  event;
    //test_event = (test_event|  event);
}
#if (SDIO_DEBUG_ENABLE ==1)
/*******************************************************************
* NAME :            void dump_GPD(int isrx)
*
* DESCRIPTION :     dump GPD info
*
* INPUTS :
*       PARAMETERS:
*           int     is_slv_rx       0 if host rx/slave tx, 1 if host tx/slave rx
*           sdio_slave_gpd_header_t *gpd_ptr    pointer to GPD
*/
void dump_GPD(int is_slv_rx, sdio_slave_gpd_header_t *gpd_ptr)
{
    int i = 0;
    for (; gpd_ptr != 0x0; gpd_ptr = SDIO_SLAVE_GPD_GET_NEXT_HEADER(gpd_ptr)) {
        printf("This is %d GPD !\r\n", i);
        printf("GPD BUFFER ADDR : 0x%08lx \r\n", SDIO_SLAVE_GPD_GET_BUFFER_ADDRESS(gpd_ptr));
        if (is_slv_rx) {
            printf("GPD BUFFER LEN : %u \r\n", SDIO_SLAVE_GPD_GET_BUFFER_LENGTH(gpd_ptr));
        }
        printf("GPD ALLOW LEN : %u \r\n", gpd_ptr->word_0.allow_buffer_length);
        printf("GPD HW0 : %u \r\n", SDIO_SLAVE_GPD_GET_HW0(gpd_ptr));
        printf("GPD IOC : %u \r\n", SDIO_SLAVE_GPD_GET_IOC(gpd_ptr));
        printf("GPD BDP : %u \r\n", SDIO_SLAVE_GPD_GET_BDP(gpd_ptr));
        printf("GPD BPS : %u \r\n", SDIO_SLAVE_GPD_GET_BPS(gpd_ptr));
        printf("GPD CHECK SUM: 0x%08x \r\n", gpd_ptr->word_0.checksum);
        printf("NEXT GPD Header: 0x%08lx \r\n", (uint32_t)SDIO_SLAVE_GPD_GET_NEXT_HEADER(gpd_ptr));
        i += 1;
        printf("=======================================\r\n");
    }
}

void dump_SLV_rxinfo(void)
{
    sdio_slave_gpd_header_t *gpd_ptr;

    uint32_t txq1_gpd_start = SDIO_SLAVE_REG->HWFTQ1SAR;
    gpd_ptr = (sdio_slave_gpd_header_t *)txq1_gpd_start;
    printf("txq1 GPD dump====\r\n");
    dump_GPD(1, gpd_ptr);
    printf("txq1 GPD dump done====\r\n");

}
void dump_SLV_txinfo(int rx_que_no[], int qnum)
{
    sdio_slave_gpd_header_t *gpd_ptr;

    for (int i = 0; i < qnum; i++) {
        uint32_t rx_gpd_start = 0x0;
        if (rx_que_no [i] == SDIO_SLAVE_RX_QUEUE_0) {
            rx_gpd_start = SDIO_SLAVE_REG->HWFRQ0SAR;
        } else if (rx_que_no [i] == SDIO_SLAVE_RX_QUEUE_1) {
            rx_gpd_start = SDIO_SLAVE_REG->HWFRQ1SAR;
        }
        gpd_ptr = (sdio_slave_gpd_header_t *)rx_gpd_start;
        printf("rxq%d GPD dump====\r\n", qnum);
        dump_GPD(0, gpd_ptr);
        printf("rxq%d GPD dump done====\r\n", qnum);
    }
}

#endif /* #if (SDIO_DEBUG_ENABLE ==1) */


uint32_t mpu_setting[4];
uint32_t gpio_setting[2];
static void MPU_Disable(void)
{

    log_hal_info("Disable SMPU Security \r\n");
    //mpu_setting[0] = READ_REG(0x30000130);
    mpu_setting[1] = READ_REG(0x30000230);
    gpio_setting[0] = READ_REG(0x30404300);
    gpio_setting[1] = READ_REG(0x30404310);
    //mpu_setting[2] = READ_REG(0x30000330);
    //mpu_setting[3] = READ_REG(0x30000430);
    //WRITE_REG(0x0,0x30000130);
    WRITE_REG(0x0, 0x30000230);
    WRITE_REG((gpio_setting[0] & 0x00ffffff) | 0x11000000, 0x30404300);
    WRITE_REG((gpio_setting[1] & 0xffff0000) | 0x00001111, 0x30404310);

}
static void MPU_Enable(void)
{
    log_hal_info("MPU Enable\r\n");
    //WRITE_REG(mpu_setting[0],0x30000130);
    WRITE_REG(mpu_setting[1], 0x30000230);
    WRITE_REG(gpio_setting[0], 0x30404300);
    WRITE_REG(gpio_setting[1], 0x30404310);
}

#define TX_DEBUG_LOG   0
static int slave_tx_basic(int length, int q_id, int rx_enhance, int packet_num)
{


    sdio_slave_send_test_cli_buf = (uint32_t *)pvPortMallocNCExt(eHeapRegion_SYSTEM, GPD_BUF_LEN_ * packet_num);
    if (sdio_slave_send_test_cli_buf == NULL) {
        printf("No free space for request, space : %d \r\n", xPortGetFreeHeapSize());
        return -1;
    }
    memset(sdio_slave_send_test_cli_buf, 0, (GPD_BUF_LEN_ * packet_num));
    for (int i = 0; i < 1024 * 4 * packet_num; i++)
        sdio_slave_send_test_cli_buf[i] = 0x5a5a5a5a + i;



#if TX_DEBUG_LOG
    printf("sdio slave start send data to RXQ.\r\n");
    printf("buf address : 0x%08lx \r\n", (uint32_t)sdio_slave_send_test_cli_buf);
#endif /* #if TX_DEBUG_LOG */

    if (packet_num == 1) {
        if (q_id == 0 || q_id == 2) {
            if (HAL_SDIO_SLAVE_STATUS_OK != hal_sdio_slave_send_dma(HAL_SDIO_SLAVE_PORT_0, HAL_SDIO_SLAVE_RX_QUEUE_0,  sdio_slave_send_test_cli_buf, length)) {
                printf("sdio slave send  Q0 error.\r\n");
                return -2;
            }
        }
        if (q_id == 1 || q_id == 2) {
            if (HAL_SDIO_SLAVE_STATUS_OK != hal_sdio_slave_send_dma(HAL_SDIO_SLAVE_PORT_0, HAL_SDIO_SLAVE_RX_QUEUE_1, sdio_slave_send_test_cli_buf, length)) {
                printf("sdio slave send Q1 error.\r\n");
                return -2;
            }
        }

    }
    //wait rx_rdy
    if ((sdio_slave_rx_queue_id_t)q_id == SDIO_SLAVE_RX_QUEUE_0) {
        while (test_event != HAL_SDIO_SLAVE_EVENT_RX0_DONE || host_rx0_count != packet_num);
    } else if ((sdio_slave_rx_queue_id_t)q_id == SDIO_SLAVE_RX_QUEUE_1) {
        while (test_event != HAL_SDIO_SLAVE_EVENT_RX1_DONE  || host_rx1_count != packet_num);
    } else if ((sdio_slave_rx_queue_id_t)q_id == SDIO_SLAVE_RX_QUEUE_MAX) {
        while (host_rx0_count != packet_num || host_rx1_count != packet_num);
    }

#if TX_DEBUG_LOG
    printf("test status :%d  \r\n", test_event);
    printf("host rx0 count : %d, host rx1 count : %d  \r\n", host_rx0_count, host_rx1_count);
#endif /* #if TX_DEBUG_LOG */

    test_event = HAL_SDIO_SLAVE_EVENT_NONE;


    host_rx0_count = 0;
    host_rx1_count = 0;
    vPortFreeNCExt(eHeapRegion_SYSTEM, sdio_slave_send_test_cli_buf);
    return 0;
}


//Host rx,Slave tx
static unsigned char sdio_slv_tx(uint8_t len, char *param[])
{

    if (len !=  4) {
        cli_puts("Parameter as [QNUM] [packet len] [packet num] [waiting time(unit : 10ms)]\n");
        return 0;
    }
    int q_id = atoi(param[0]);
    int pkt_len = atoi(param[1]);
    int pkt_num = atoi(param[2]);
    int waiting_time = atoi(param[3]);
    int cnt = 0;
    if (q_id >= 3 || q_id < 0) {
        printf("invalid q_id : %d!\r\n", q_id);
        printf("q_id should >=0 and <=1 !\r\n");
        return 0;
    }
    if (pkt_num < 1) {
        printf("invalid pkt_no : %d!\r\n", pkt_num);
        return 0;
    }

    printf("QNUM : %d, Pkt_len : %d, Pkt_num : %d \r\n", q_id, pkt_len, pkt_num);

    for (int i = 0; i < 16; i++) {
        isr_duration[i] = 0;
    }

    // Disable MPU setting
    MPU_Disable();
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_SDIO);
    printf("SDIO Slave Lock Request! \r\n");
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    if (0 > hal_sdio_slave_init(HAL_SDIO_SLAVE_PORT_0)) {
        printf("sdio slave init error. \r\n");
        return -1;
    } else {
        printf("sdio slave init ok. \r\n");

    }
    //set sdio callback
    hal_sdio_slave_register_callback(HAL_SDIO_SLAVE_PORT_0, sdio_slave_test_callback, NULL);

    //Waiting Drv own for 10s
    printf("Waiting Driver own! \r\n");
    while (hal_sdio_slave_check_fw_own()) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        cnt += 1;
        if (cnt == waiting_time) {
            printf("Drv own timeout ! \n");
            break;
        }
    }

    printf("This is for HQA! continuous tx, each time transmit one Packet. Each packets are 4092 BYTES \r\n");
    for (int i = 0; i < pkt_num; i++) {
        //printf("This is %d Times \r\n", i);
        slave_tx_basic(pkt_len, q_id, 1, 1);
    }

#if TX_DEBUG_LOG
    if (q_id  != 2) {
        int qid_arr[1] = {q_id};
    } else {
        int qid_arr[2] = {0, 1};
    }
#endif /* #if TX_DEBUG_LOG */

    // Restore  MPU setting
    MPU_Enable();
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_SDIO);
    printf("SDIO Slave Release lock! \r\n");
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    hal_sdio_slave_deinit(HAL_SDIO_SLAVE_PORT_0);
    return 0;

}
int compare(int pattern, uint32_t *buf, int length)
{
    int i = 0;
    for (i = 0 ; i < length ; i++) {
        if (buf[i] != (unsigned int)(pattern + i)) {
            printf("fail idx : %d , pattern : 0x%08x, data : 0x%08lx \r\n", i, pattern + i, buf[i]);
            return -1;
        }
    }
    return 0;
}

#define COMPARE_RX_DATA 1
#define RX_DEBUG_LOG   0
#define RX_BUF_DUMP 1
int slave_rx_basic(int length, int packet_num, int pattern)
{
    int i = 0;
#if COMPARE_RX_DATA
    int ret = 0;
#endif /* #if COMPARE_RX_DATA */

    sdio_slave_receive_test_cli_buf = pvPortMallocNCExt(eHeapRegion_SYSTEM, GPD_BUF_LEN_ * packet_num);
    if (sdio_slave_receive_test_cli_buf == NULL) {
        printf("No free space for request, space : %d \r\n", xPortGetFreeHeapSize());
        return -1;
    }
    memset(sdio_slave_receive_test_cli_buf, 0, (1024 * 4 * packet_num));
    test_event = HAL_SDIO_SLAVE_EVENT_NONE;

#if RX_DEBUG_LOG
    printf("buf address : 0x%08lx \r\n", (uint32_t)sdio_slave_receive_test_cli_buf);
#endif /* #if RX_DEBUG_LOG */

    //mailbox for Host, host to see this value to send length
    SDIO_SLAVE_REG->D2HSM0R = length;
    if (packet_num == 1) {
        if (HAL_SDIO_SLAVE_STATUS_OK != hal_sdio_slave_receive_dma(HAL_SDIO_SLAVE_PORT_0, HAL_SDIO_SLAVE_TX_QUEUE_1, sdio_slave_receive_test_cli_buf, 4092)) {
            printf("sdio slave receive error.\r\n");
            return -3;
        }
    }


#if RX_DEBUG_LOG
    printf("waiting done! packet num = %d \r\n", packet_num);
#endif /* #if RX_DEBUG_LOG */
    while (test_event != HAL_SDIO_SLAVE_EVENT_TX1_DONE || host_tx_count != packet_num) {
#if RX_BUF_DUMP
        if (error_notify) {
            for (int i = 0; i < length / 4 ; i++) {
                printf("[Buf] %d : 0x%08lx \r\n", i,  *(sdio_slave_receive_test_cli_buf + i));
            }
            error_notify = 0;
            return 0;
        }
#endif /* #if RX_BUF_DUMP */
    };

#if RX_DEBUG_LOG
    printf("test status :%d \r\n", test_event);
#endif /* #if RX_DEBUG_LOG */

    test_event = HAL_SDIO_SLAVE_EVENT_NONE;

#if RX_DEBUG_LOG
    printf("host tx count : %d \r\n", host_tx_count);
#endif /* #if RX_DEBUG_LOG */

    if (host_tx_count != packet_num) {
        printf("GPD Done INT number is not match with expected! \r\n");
    }

    host_tx_count = 0;
#if COMPARE_RX_DATA
    for (i = 0; i < packet_num; i++) {
        printf("GPD%d (pattern): 0x%08x \r\n", i, 0x5a5a5a5a + ((length + 3) / 4)*i);
        ret = compare(0x5a5a5a5a + ((length + 3) / 4) * i, sdio_slave_receive_test_cli_buf + ((GPD_BUF_LEN_ + 3) / 4) * i, (length + 3) / 4);
    }
    if (ret < 0)
        printf("Error Occur when compare \r\n");

#endif /* #if COMPARE_RX_DATA */
    //vPortFreeNC(slt_sdio_slave_receive_buf);
#if RX_DEBUG_LOG
    printf("sdio slave receive ok.\r\n");
#endif /* #if RX_DEBUG_LOG */

    vPortFreeNCExt(eHeapRegion_SYSTEM, sdio_slave_receive_test_cli_buf);
    return 0;
}


static unsigned char sdio_slv_rx(uint8_t len, char *param[])
{
    if (len !=  3) {
        cli_puts("Parameter as [packet len] [times] [waiting time(unit : 10ms)]\r\n");
        return 0;
    }

    int pkt_len = atoi(param[0]);
    int pkt_num = atoi(param[1]);
    int waiting_time = atoi(param[2]);
    int cnt = 0;
    //uint32_t start_count = 0;
    //uint32_t end_count= 0;//tx_time,rx_time,tx_end,rx_end = 0;
    //uint32_t count = 0;

    if (pkt_num < 1) {
        log_hal_info("invalid pkt_no : %d!\r\n", pkt_num);
        return 0;
    }

    if (pkt_len > 4092) {
        log_hal_info("invalid pkt_len : %d!\r\n", pkt_num);
    }




    //log_hal_info("Pkt_len : %d, pkt_num : %d \r\n", pkt_len, pkt_num);

    for (int i = 0; i < 16; i++) {
        isr_duration[i] = 0;
    }

    // Disable MPU setting
    MPU_Disable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_SDIO);
    printf("SDIO Slave Lock Request! \r\n");
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    if (0 > hal_sdio_slave_init(HAL_SDIO_SLAVE_PORT_0)) {
        // log_hal_info("sdio slave init error. \r\n");
        return -1;
    } else {
        // log_hal_info("sdio slave init ok. \r\n");
    }


    //set sdio callback
    hal_sdio_slave_register_callback(HAL_SDIO_SLAVE_PORT_0, sdio_slave_test_callback, NULL);

    while (hal_sdio_slave_check_fw_own()) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        cnt += 1;
        if (cnt == waiting_time) {
            printf("DRV OWN Timeout \n");
            return -1;
        }
    }

    printf("This is for HQA! continuous tx, each time receives one Packet. Each packets are 4092 BYTES \r\n");
    for (int i = 0; i < pkt_num; i++) {
        //printf("This is %d Times \r\n", i);
        //hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &start_count);
        slave_rx_basic(4092, 1, 0xaa55aa55);
        //hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &end_count);
        //hal_gpt_get_duration_count(start_count, end_count, &count);
        //printf("Start cnt : %ld, End cnt: %ld, Exec Time : %ld \r\n", start_count, end_count, count);
    }

#if (SDIO_DEBUG_ENABLE ==1)
    dump_SLV_rxinfo();
#endif /* #if (SDIO_DEBUG_ENABLE ==1) */
    // Restore  MPU setting
    MPU_Enable();

#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_SDIO);
    printf("SDIO Slave Release lock! \r\n");
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    hal_sdio_slave_deinit(HAL_SDIO_SLAVE_PORT_0);
    isr_cnt = 0;

    return 0;

}


#if SUPPORT_SDIO_AGG
#define dump_rx_buf 1
static unsigned char  sdio_slv_loopback(uint8_t len, char *param[])
{
    uint32_t start_count = 0;
    uint32_t end_count = 0; //tx_time,rx_time,tx_end,rx_end = 0;
    uint32_t count = 0;
    int waiting_time = 20;
    int cnt = 0;

    test_event = HAL_SDIO_SLAVE_EVENT_NONE;
    host_tx_count = 0;
    host_rx0_count = 0;
    host_rx1_count = 0;

    if (len !=  3 && len !=  4) {
        cli_puts("Parameter as [QNUM] [packet len]  [iteration] <waiting time(units:ms)>\r\n");
        cli_puts("Only suupport 4092 byte each packet \r\n");
        return 0;
    }
    int q_id = atoi(param[0]);
    int pkt_len = atoi(param[1]);
    int pkt_num = 1;
    int times = atoi(param[2]);

    if (len == 4)
        waiting_time = atoi(param[3]);



    if (q_id >= 3 || q_id < 0) {
        printf("invalid q_id : %d!\r\n", q_id);
        printf("q_id should >=0 and <=1 !\r\n");
        return 0;
    }
    if (pkt_num < 1) {
        printf("invalid pkt_no : %d!\r\n", pkt_num);
        return 0;
    } else {
        printf("invalid pkt_no : %d!\r\n", pkt_num);
    }
    if (pkt_len > 4092 || pkt_len < 1) {
        printf("invalid pkt_len : %d!\r\n", pkt_len);
        return 0;

    }

    printf("QNUM : %d, Pkt_len : %d, Pkt_num : %d times : %d \r\n", q_id, pkt_len, pkt_num, times);

    sdio_slave_send_test_cli_buf = (uint32_t *)pvPortMallocNCExt(eHeapRegion_SYSTEM, 1024 * 4 * pkt_num);
    if (sdio_slave_send_test_cli_buf == NULL) {
        printf("No free space for TX request, space : %d \r\n", xPortGetFreeHeapSize());
        return -1;
    }

    sdio_slave_receive_test_cli_buf = pvPortMallocNCExt(eHeapRegion_SYSTEM, 1024 * 4 * pkt_num);
    if (sdio_slave_receive_test_cli_buf == NULL) {
        printf("No free space for RX request, space : %d \r\n", xPortGetFreeHeapSize());
        return -1;
    }

    memset(sdio_slave_send_test_cli_buf, 0, 1024 * 4 * pkt_num);
    memset(sdio_slave_receive_test_cli_buf, 0, (1024 * 4 * pkt_num));

    // Prepare data for max length
    for (int j = 0; j < pkt_num ; j++) {
        for (int k = 0; k < (pkt_len + 3) / 4; k++) {
            *(sdio_slave_send_test_cli_buf + k + (j * 1024))  = 0x5a5a5a5a + k + j * ((pkt_len + 3) / 4);
        }
    }

    //printf( "RX enhance mode : %s \r\n", rx_enhance ? "Enable"  : "Disable");

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &start_count);

    // Disable MPU setting
    MPU_Disable();

    if (0 > hal_sdio_slave_init(HAL_SDIO_SLAVE_PORT_0)) {
        printf("sdio slave init error. \r\n");
        return -1;
    } else {
        //printf("sdio slave init ok. \r\n");
    }


    //set sdio callback
    hal_sdio_slave_register_callback(HAL_SDIO_SLAVE_PORT_0, sdio_slave_test_callback, NULL);

    //waiting Drv own
    while (hal_sdio_slave_check_fw_own()) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        cnt += 1;
        if (cnt == waiting_time)
            break;
    }


    for (int i = 0; i < times; i++) {
        printf("This is %d times \r\n", i);
        if (q_id == 2 || q_id == 0) {
            if (HAL_SDIO_SLAVE_STATUS_OK != hal_sdio_slave_send_dma(HAL_SDIO_SLAVE_PORT_0, SDIO_SLAVE_RX_QUEUE_0, sdio_slave_send_test_cli_buf, pkt_len)) {
                printf("sdio slave send error.\r\n");
                return -2;
            }
        }

        if (q_id == 2 || q_id == 1) {
            if (HAL_SDIO_SLAVE_STATUS_OK != hal_sdio_slave_send_dma(HAL_SDIO_SLAVE_PORT_0, SDIO_SLAVE_RX_QUEUE_1, sdio_slave_send_test_cli_buf, pkt_len)) {
                printf("sdio slave send error.\r\n");
                return -2;
            }
        }



        if (HAL_SDIO_SLAVE_STATUS_OK != hal_sdio_slave_receive_dma(HAL_SDIO_SLAVE_PORT_0, HAL_SDIO_SLAVE_TX_QUEUE_1, sdio_slave_receive_test_cli_buf, 4092)) {
            printf("sdio slave receive error.\r\n");
            return -3;
        }

        if (q_id == 2 || q_id == 0)
            while (host_rx0_count != pkt_num);

        if (q_id == 2 || q_id == 1)
            while (host_rx1_count != pkt_num);

        while (host_tx_count != pkt_num) {
#if dump_rx_buf
            if (error_notify == 1) {
                for (int i = 0 ; i < pkt_len / 4; i ++) {
                    printf("[Buf] %d : 0x%08lx \r\n", i, *(sdio_slave_receive_test_cli_buf + i));
                }
                error_notify = 0;
                return 0;

            }
#endif /* #if dump_rx_buf */
        };


        for (int j = 0; j < pkt_num; j++) {
            //printf("GPD%d : 0x%08lx \r\n" , j, sdio_slave_send_test_cli_buf[j][0] );
            for (int k = 0; k < (pkt_len + 3) / 4; k++) {
                int offset = j * 1024;
                if (*(sdio_slave_receive_test_cli_buf + k + offset) != *(sdio_slave_send_test_cli_buf + k + offset)) {
                    printf("[Error]Send : 0x%08lx, Receive : 0x%08lx \r\n", *(sdio_slave_send_test_cli_buf + k + offset), *(sdio_slave_receive_test_cli_buf + k + offset));
                    return 0;
                }
            }
        }


        test_event = HAL_SDIO_SLAVE_EVENT_NONE;
        host_tx_count = 0;
        host_rx0_count = 0;
        host_rx1_count = 0;
        memset(sdio_slave_receive_test_cli_buf, 0, (1024 * 4 * pkt_num));

        //Slave TX first
        //slave_tx_basic(4092 , q_id, 1, pkt_num);

        //slave_rx_basic(4092, pkt_num, 0x5a5a5a5a);
    }
    //hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &rx_time);
    //hal_gpt_get_duration_count(start_count, rx_time, &rx_end);


    // Restore  MPU setting
    MPU_Enable();

    hal_sdio_slave_deinit(HAL_SDIO_SLAVE_PORT_0);
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &end_count);
    hal_gpt_get_duration_count(start_count, end_count, &count);

    //printf("Start cnt : %ld, TX End cnt: %ld, TX Exec Time(us) : %ld \r\n", start_count, tx_time, tx_end);
    //printf("Start cnt : %ld, RX End cnt: %ld, RX Exec Time(us) : %ld \r\n", start_count, rx_time, rx_end);
    printf("Start cnt : %ld, End cnt: %ld, Exec Time : %ld \r\n", start_count, end_count, count);
    vPortFreeNCExt(eHeapRegion_SYSTEM, sdio_slave_receive_test_cli_buf);
    vPortFreeNCExt(eHeapRegion_SYSTEM, sdio_slave_send_test_cli_buf);
    return 0;

}
#endif /* #if SUPPORT_SDIO_AGG */

static unsigned char sdio_slv_init(uint8_t len, char *param[])
{
    MPU_Disable();
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_SDIO);
    printf("SDIO Slave Lock Request! \r\n");
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    if (0 > hal_sdio_slave_init(HAL_SDIO_SLAVE_PORT_0)) {
        printf("sdio slave init error. \r\n");
        return -1;
    } else {
        printf("sdio slave init ok. \r\n");

    }
    hal_sdio_slave_register_callback(HAL_SDIO_SLAVE_PORT_0, sdio_slave_test_callback, NULL);
    return 0;
}

static unsigned char sdio_slv_deinit(uint8_t len, char *param[])
{
    hal_sdio_slave_deinit(HAL_SDIO_SLAVE_PORT_0);
    MPU_Enable();
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_SDIO);
    printf("SDIO Slave Release lock! \r\n");
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    return 0;
}

static unsigned char sdio_cr_get(uint8_t len, char *param[])
{
    dump_CR();
    return 0;
}

#if (SDIO_DVT ==1)

static unsigned char sdio_tx_int(uint8_t len, char *param[])
{

    if (len !=  3) {
        cli_puts("Parameter as [D2H0 Value]  [D2H1 Value] [TX1_CNT] \r\n");
        cli_puts("Only suupport 4092 byte each packet \r\n");
        return 0;
    }

    // Disable MPU setting
    MPU_Disable();

    if (0 > hal_sdio_slave_init(HAL_SDIO_SLAVE_PORT_0)) {
        printf("sdio slave init error. \r\n");
        return -1;
    } else {
        printf("sdio slave init ok. \r\n");

    }
    hal_sdio_slave_register_callback(HAL_SDIO_SLAVE_PORT_0, sdio_slave_test_callback, NULL);


    int D2Hmailbox0 = atoi(param[0]);
    int D2Hmailbox1 = atoi(param[1]);
    int tx_cnt = atoi(param[2]);
    if (tx_cnt  > 0xff) {
        printf("TX CNT should not big than 255 \r\n");
        return 0;
    }
    printf("Set D2H mailbox\r\n");
    SDIO_SLAVE_REG->D2HSM0R = D2Hmailbox0;
    SDIO_SLAVE_REG->D2HSM1R = D2Hmailbox1;
    printf("Set TX_CNT(will be clean when read WTSR.TQ1_CNT ) \r\n");
    sdio_slave_set_tx_queue_node_add(SDIO_SLAVE_TX_QUEUE_1, tx_cnt);

    slave_tx_basic(4092, SDIO_SLAVE_RX_QUEUE_0, 1, 1);

    MPU_Enable();
    hal_sdio_slave_deinit(HAL_SDIO_SLAVE_PORT_0);

    return 0;

}

#endif /* #if (SDIO_DVT ==1) */

uint8_t CR[] = {0x1a, 0x1b};
static unsigned char sdio_slv_debug(uint8_t len, char *param[])
{
    uint32_t temp = 0x0;
    if (len !=  0) {
        cli_puts("Parameter as NULL \r\n");
    }
    for (unsigned int i = 0 ; i < sizeof(CR); i ++) {
        temp = READ_REG(HDBGCR);
        WRITE_REG((temp & 0xff) | CR[i], HDBGCR);
        temp = READ_REG(HDBGCR);
        log_hal_info("CR : 0x%02lx, value : 0x%08lx \r\n", CR[i], temp);
    }
    return 0;
}

#ifdef IOT_CAM_ENABLE
static unsigned char sdio_slv_recv_iot_pkt(uint8_t len, char *param[])
{
    extern int sdio_tx_queue_reset(void);
    int status = sdio_tx_queue_reset();
    if (status)
        printf("receive iot packet error. %d\r\n", status);
    else
        printf("OK\r\n");
    return status;
}

static unsigned char sdio_slv_lock_sts(uint8_t len, char *param[])
{
    if (check_sdio_sleep_lock()) {
        printf("Lock cnt is 0! The chip can go to sleep! \r\n");
    } else {
        printf("The chip can not go to sleep \r\n");
    }
    return 0;
}

#endif /* #ifdef IOT_CAM_ENABLE */


#include <timers.h>
static void sdio_status_monitor_timer_cb(TimerHandle_t xTimer)
{
    printf("sdio_status_monitor_timer_cb\r\n");
}

static unsigned char sdio_status_monitor(uint8_t len, char *param[])
{

    TimerHandle_t handle = xTimerCreate("SDIO Status Monitor",
                                        3000 / portTICK_PERIOD_MS,
                                        pdTRUE, (void *)0, sdio_status_monitor_timer_cb);
    printf("param = %s\r\n", param[0]);
    xTimerStart(handle, 1);
    return 0;
}


cmd_t sdio_cli_cmds[] = {
    //{ "get", "get configurations of all pins", gpio_get, NULL },
    { "dump", "dump sdio CR", sdio_cr_get, NULL },
    { "tx", "sdio slave transmission ", sdio_slv_tx, NULL },
    { "rx", "sdio slave receive ", sdio_slv_rx, NULL },
    { "debug", "debug CR", sdio_slv_debug, NULL },
#if SUPPORT_SDIO_AGG
    { "lpb", "sdio loopback ", sdio_slv_loopback, NULL },
#endif /* #if SUPPORT_SDIO_AGG */
    { "init", "sdio init", sdio_slv_init, NULL },
    { "deinit", "sdio deinit", sdio_slv_deinit, NULL },
#if (SDIO_DVT ==1)
    { "tx_int", "tx_int", sdio_tx_int, NULL },
#endif /* #if (SDIO_DVT ==1) */
#ifdef IOT_CAM_ENABLE
    {"rx_iot_pkt", "Receive pkt from sav", sdio_slv_recv_iot_pkt, NULL },
    {"sts_lock", "Check SDIO lock", sdio_slv_lock_sts, NULL },
#endif /* #ifdef IOT_CAM_ENABLE */
    {"monitor", "sdio status monitor", sdio_status_monitor, NULL },
    { NULL, NULL, NULL, NULL }
};

#ifdef IOT_CAM_ENABLE
#include "iot_cam_gpio.h"
unsigned char sdio_slv_api_init(hal_sdio_slave_callback_t sdio_slave_callback)
{
    printf("####sdio_slv_api_init####\r\n");

    //sdio_slv_init(0, 0);

    MPU_Disable();
    if (0 > hal_sdio_slave_init(HAL_SDIO_SLAVE_PORT_0)) {
        printf("sdio slave init error. \r\n");
        return -1;
    } else {
        printf("sdio slave init ok. \r\n");

    }
    hal_sdio_slave_register_callback(HAL_SDIO_SLAVE_PORT_0, sdio_slave_callback, NULL);
    return 0;
}

unsigned char sdio_slv_api_deinit(void)
{
    printf("####sdio_slv_api_deinit####\r\n");

    if (0 > hal_sdio_slave_deinit(HAL_SDIO_SLAVE_PORT_0)) {
        printf("sdio slave deinit error. \r\n");
        return -1;
    } else {
        printf("sdio slave deinit ok. \r\n");
    }
    iot_cam_gpio_sdio_removal();
    return 0;
}
#endif /* #ifdef IOT_CAM_ENABLE */
