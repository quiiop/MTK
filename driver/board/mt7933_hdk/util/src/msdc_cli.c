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
#include "msdc_cli.h"
#include "hal_log.h"
#include "memory_attribute.h"
#include "hal_gpt.h"
#include "hal_nvic.h"
#include "hal_gpio_internal.h"

extern hal_sdio_status_t sdio_interrupt_register_callback(hal_sdio_port_t sdio_port, hal_sdio_callback_t sdio_callback, void *user_data);
uint32_t *msdc_send_test_buf;
uint32_t *msdc_receive_test_buf;
hal_sdio_command53_config_t cmd53_conf_cli;
hal_sdio_command52_config_t cmd52_conf_cli;
uint32_t probe_state = -1;
//hal_sdio_command52_config_t cmd52_conf_cli;
/*uint32_t msdc_mpu_setting[4];
static void MSDC_MPU_Disable(void)
{

    printf("Disable SMPU Security \r\n");
    msdc_mpu_setting[1] = READ_REG(0x30000230);
    WRITE_REG(0x0,0x30000230);

}

static void MSDC_MPU_Enable(void)
{
    printf("MPU Enable\r\n");
    WRITE_REG(msdc_mpu_setting[1],0x30000230);

}
*/
#define RX_CMD_LOG 1
#define TX_CMD_LOG 1
static hal_sdio_status_t sdio_cmd53_read(int addr, int length, uint32_t buffer_addr)
{

    hal_sdio_status_t status;
    if (length < SDIO_BLK_SIZE) {
        cmd53_conf_cli.count = length;    /**< Byte or block count. */
        cmd53_conf_cli.block = FALSE;
    } else {
        cmd53_conf_cli.count = (length + (SDIO_BLK_SIZE - 1)) / SDIO_BLK_SIZE;
        cmd53_conf_cli.block = TRUE;
    }
    cmd53_conf_cli.direction = HAL_SDIO_DIRECTION_READ;                                    /**< Read/write direction for the SDIO COMMAND53. */
    cmd53_conf_cli.address = addr;                                                  /**< Read/write address of the SDIO COMMAND53. */
    cmd53_conf_cli.buffer = buffer_addr;
#if RX_CMD_LOG
    log_hal_info("addr: 0x%02x, size : %d, buffer_addr : 0x%08x, block mode : %d  ", addr, cmd53_conf_cli.count, buffer_addr, cmd53_conf_cli.block);
#endif /* #if RX_CMD_LOG */
    status = hal_sdio_execute_command53(HAL_SDIO_PORT_0, &cmd53_conf_cli);
    return status;
}

static hal_sdio_status_t sdio_cmd53_write(int addr, int length, uint32_t buffer_addr)
{
    hal_sdio_status_t status;
    if (length < SDIO_BLK_SIZE) {
        cmd53_conf_cli.count = length;    /**< Byte or block count. */
        cmd53_conf_cli.block = FALSE;
    } else {
        cmd53_conf_cli.count = (length + (SDIO_BLK_SIZE - 1)) / SDIO_BLK_SIZE;
        cmd53_conf_cli.block = TRUE;
    }
    cmd53_conf_cli.direction = HAL_SDIO_DIRECTION_WRITE;                                    /**< Read/write direction for the SDIO COMMAND53. */
    cmd53_conf_cli.address = addr;                                                  /**< Read/write address of the SDIO COMMAND53. */
    cmd53_conf_cli.buffer = buffer_addr;
#if TX_CMD_LOG
    log_hal_info("addr: 0x%02x, size : %d, buffer_addr : 0x%08x, block mode : %d ", addr, cmd53_conf_cli.count, buffer_addr, cmd53_conf_cli.block);
#endif /* #if TX_CMD_LOG */
    status = hal_sdio_execute_command53(HAL_SDIO_PORT_0, &cmd53_conf_cli);
    return status;
}

static hal_sdio_status_t sdio_cmd52_read(int func, int addr, uint32_t *data)
{
    hal_sdio_status_t status;
    cmd52_conf_cli.direction = HAL_SDIO_DIRECTION_READ;
    cmd52_conf_cli.function = func;
    cmd52_conf_cli.read_after_write = false;
    cmd52_conf_cli.stop = false;
    cmd52_conf_cli.address = addr;
    log_hal_info("SDIO CMD52 func : %d ,SDIO CMD52 addr  :%d  \r\n \r\n", cmd52_conf_cli.function, cmd52_conf_cli.address);
    status = hal_sdio_execute_command52(HAL_SDIO_PORT_0, &cmd52_conf_cli);
    if (status != HAL_SDIO_STATUS_OK) {
        log_hal_error("SDIO CMD52 Read fail  %d  \r\n", status);
    }
    *data = cmd52_conf_cli.data;
    log_hal_info("Data  0x%02x  \r\n", cmd52_conf_cli.data);
    return status;

}
static hal_sdio_status_t sdio_cmd52_write(int func, int addr, uint32_t data)
{
    hal_sdio_status_t status;
    cmd52_conf_cli.direction = HAL_SDIO_DIRECTION_WRITE;
    cmd52_conf_cli.function = func;
    cmd52_conf_cli.read_after_write = false;
    cmd52_conf_cli.stop = false;
    cmd52_conf_cli.address = addr;
    cmd52_conf_cli.data = data;
    log_hal_info("SDIO CMD52 func : %d, SDIO CMD52 addr : %d, SDIO CMD52 data : %d \r\n \r\n", cmd52_conf_cli.function, cmd52_conf_cli.address, cmd52_conf_cli.data);
    status = hal_sdio_execute_command52(HAL_SDIO_PORT_0, &cmd52_conf_cli);
    if (status != HAL_SDIO_STATUS_OK) {
        log_hal_error("SDIO CMD52 Write fail  %d  \r\n", status);
    }
    return status;

}


static int msdc_int_status(int speed)
{

    hal_sdio_status_t status = -1;
    hal_sdio_config_t sdio_config = {HAL_SDIO_BUS_WIDTH_4, speed};

    status = hal_sdio_init(HAL_SDIO_PORT_0, &sdio_config);
    log_hal_error("KH hal_sdio_init status = %d  \r\n", status);
    if (HAL_SDIO_STATUS_OK != status) {
        printf("sdio init error. \r\n");
        probe_state = -1;
    } else {
        /*Set block size */
        status = hal_sdio_set_block_size(HAL_SDIO_PORT_0, HAL_SDIO_FUNCTION_1, SDIO_BLK_SIZE);
        log_hal_error("KH hal_sdio_set_block_size status = %d \r\n", status);
        if (HAL_SDIO_STATUS_OK != status) {
            printf("sdio set block size error. \r\n");
            probe_state = -1;
            return probe_state;
        }

        uint32_t blk_size = 0;
        status = hal_sdio_get_block_size(HAL_SDIO_PORT_0, HAL_SDIO_FUNCTION_1, &blk_size);
        log_hal_error("KH hal_sdio_get_block_size status = %d \r\n ", status);
        if (HAL_SDIO_STATUS_OK != status) {
            log_hal_error("sdio get block size error. \r\n");
        } else {
            log_hal_error("sdio get block size, block size is %ld. \r\n", blk_size);
        }
        probe_state = 1;
        uint32_t val = 0x0;
        sdio_cmd53_read(SDIO_CHIPID_CR, 0x4, (uint32_t)(&val));
        log_hal_error("chip_id : 0x%08x ", val);
    }

    return probe_state;
}
static int msdc_drv_own(void)
{

    uint32_t val = 0;
    //hal_sdio_status_t status = -1;

    sdio_cmd53_read(SDIO_IP_WHLPCR, 0x4, (uint32_t)(&val));
    log_hal_error("sdio OWN Status  0x%lx. \r\n", val);
    if (val & DRV_OWN_STATUS) {
        return val & DRV_OWN_STATUS;
    } else {
        val = val | FW_OWN_REQ_CLR;
        sdio_cmd53_write(SDIO_IP_WHLPCR, 0x4, (uint32_t)(&val));
    }
    return val & DRV_OWN_STATUS;
}


static unsigned char msdc_rx(uint8_t len, char *param[])
{

    int cnt = 0;
    uint32_t int_status = 0x0;
    unsigned int rx_len = 0x0;
    unsigned int rx0_len = 0x0;
    unsigned int rx1_len = 0x0;

    hal_sdio_status_t rx0_status = -1;
    hal_sdio_status_t rx1_status = -1;
    cmd53_conf_cli.function = HAL_SDIO_FUNCTION_1;                                   /**< Function type for the SDIO COMMAND53. */
    cmd53_conf_cli.block = FALSE;                                                       /**< Indicates whether read/write is in a block mode or not. */
    cmd53_conf_cli.operation = HAL_SDIO_FIXED_ADDRESS;                          /**< Operation mode for the SDIO COMMAND53. */

    if (len !=  3) {
        cli_puts("Parameter as [Data len] [check buf] [waiting time(unit in 100ms)] \r\n");
        return 0;
    }


    int pkt_len = atoi(param[0]);
    int check = atoi(param[1]);
    int waiting_time = atoi(param[2]);


    if (pkt_len > 4096) {
        printf("Invalid Data Len \r\n");
        return -1;
    }


    msdc_receive_test_buf = (uint32_t *)pvPortMallocNCExt(eHeapRegion_SYSTEM, GPD_BUF_LEN);
    if (msdc_receive_test_buf == NULL) {
        log_hal_error("No free space for request, space : %d ", xPortGetFreeHeapSize());
        return -1;
    }
    log_hal_info("buf address : 0x%08lx ", (uint32_t)msdc_receive_test_buf);
    memset(msdc_receive_test_buf, 0x0, GPD_BUF_LEN);

    /*msdc_init and set block size*/
    log_hal_error("MSDC INIT! ");
    if (probe_state != 1) {
        log_hal_error("Pls do probe first !");
        return -1;
    }
    while (!msdc_drv_own()) {
        hal_gpt_delay_ms(10);
        if (cnt == waiting_time) {
            log_hal_error("Waiting Status Timeout ");
            return -1;
        }
    }
    /* watiting INT for slave prepare GPD and data */
    log_hal_info("Waiting Interrupt!");
    while (1) {
        sdio_cmd53_read(SDIO_IP_WHISR, 0x4, (uint32_t)(&int_status));
        hal_gpt_delay_ms(100);
        cnt += 1;
        if (int_status & (RX0_DONE_INT | RX1_DONE_INT)) {
            break;
        }
        if (cnt % 3 != 0) {
            log_hal_info("RX Done not get!");
        }
        if (cnt == waiting_time) {
            log_hal_error("Waiting Status Timeout ");
            return -1;
        }
    }

    log_hal_error("INT Status : 0x%08lx ", int_status);

    /* Read data len*/
    sdio_cmd53_read(SDIO_IP_WRPLR, 0x4, (uint32_t)(&rx_len));
    log_hal_error("RX Len : 0x%08lx ", rx_len);

    /* Receive data from host*/
    if (int_status & RX0_DONE_INT) {
        rx0_len = rx_len & SDIO_RX0_LEN;
        log_hal_error("rx0_len : 0x%d ", rx0_len);
        rx0_status =  sdio_cmd53_read(SDIO_IP_WRDR0, rx0_len, (uint32_t)(msdc_receive_test_buf));
        log_hal_error("rxQ0_data[0] : 0x%08lx, rxQ0_data[1022]: 0x%08lx ", msdc_receive_test_buf[0], msdc_receive_test_buf[1022]);

        if (check) {
            for (unsigned int i = 0; i < (rx0_len + 3) / 4 ; i++) {
                if (msdc_receive_test_buf[i] != 0x5a5a5a5a + i) {
                    log_hal_error("Pattern[%d] : 0x%08lx, rxQ0_data[%d]: 0x%08lx ", i, 0x5a5a5a5a + i, i, msdc_receive_test_buf[i]);
                    break;
                }
            }
        }
        if (rx0_status != 0) {
            log_hal_error("RX0 ERROR OCCURES! : %d", rx0_status);
            return -1;
        }
    }


    if (int_status & RX1_DONE_INT) {
        rx1_len = (rx_len & SDIO_RX1_LEN) >> 0x10 ;
        log_hal_error("rx1_len : 0x%d ", rx1_len);
        rx1_status =  sdio_cmd53_read(SDIO_IP_WRDR1, rx1_len, (uint32_t)(msdc_receive_test_buf));
        log_hal_error("rxQ1_data[0] : 0x%08lx, rxQ1_data[1022]: 0x%08lx ", msdc_receive_test_buf[0], msdc_receive_test_buf[1022]);

        if (check) {
            for (unsigned int i = 0; i < (rx1_len + 3) / 4 ; i++) {
                if (msdc_receive_test_buf[i] != 0x5a5a5a5a + i) {
                    log_hal_error("Pattern[%d] : 0x%08lx, rxQ0_data[%d]: 0x%08lx ", i, 0x5a5a5a5a + i, i, msdc_receive_test_buf[i]);
                    break;

                }
            }
        }
        if (rx1_status != 0) {
            log_hal_error("RX1 ERROR OCCURES! : %d", rx1_status);
            return -1;
        }
    }

    log_hal_error("RX Success!");
    vPortFreeNCExt(eHeapRegion_SYSTEM, msdc_receive_test_buf);
    return 0;
}
static unsigned char msdc_tx(uint8_t len, char *param[])
{

    int32_t cnt = 0;
    uint32_t int_status = 0x0;
    int tx_cnt = 0;

    hal_sdio_status_t tx_status = -1;
    cmd53_conf_cli.function = HAL_SDIO_FUNCTION_1;                                   /**< Function type for the SDIO COMMAND53. */
    cmd53_conf_cli.block = FALSE;                                                       /**< Indicates whether read/write is in a block mode or not. */
    cmd53_conf_cli.operation = HAL_SDIO_FIXED_ADDRESS;                          /**< Operation mode for the SDIO COMMAND53. */

    if (len !=  2) {
        cli_puts("Parameter as [Data len] [waiting time(unit in 100ms)]");
        return 0;
    }


    int pkt_len = atoi(param[0]);
    int waiting_time = atoi(param[1]);



    if (pkt_len > 4096) {
        log_hal_error("Invalid Data Len ");
        return 0;
    }


    msdc_send_test_buf = (uint32_t *)pvPortMallocNCExt(eHeapRegion_SYSTEM, GPD_BUF_LEN);
    if (msdc_send_test_buf == NULL) {
        log_hal_error("No free space for request, space : %d", xPortGetFreeHeapSize());
        return -1;
    }
    printf("buf address : 0x%08lx ", (uint32_t)msdc_send_test_buf);
    memset(msdc_send_test_buf, 0x0, GPD_BUF_LEN);

    /*msdc_init and set block size*/
    printf("MSDC INIT!");
    if (probe_state != 1) {
        log_hal_error("Pls do probe first !");
        return -1;
    }
    while (!msdc_drv_own()) {
        hal_gpt_delay_ms(10);
        if (cnt == waiting_time) {
            log_hal_error("Waiting Status Timeout ");
            return -1;
        }
    }
    /* watiting INT for slave prepare GPD and data */
    log_hal_error("Waiting Interrupt!");
    while (1) {

        sdio_cmd53_read(SDIO_IP_WHISR, 0x4, (uint32_t)(&int_status));
        hal_gpt_delay_ms(100);
        cnt += 1;
        if (int_status & TX_DONE_INT) {
            break;
        }
        if (cnt % 3 == 0) {
            log_hal_info("TX Done not get!");
        }
        if (cnt == waiting_time) {
            log_hal_info("Waiting Status Timeout ");
            return -1;
        }
    }

    log_hal_info("INT Status : 0x%08lx ", int_status);

    /*Read PKT no */
    sdio_cmd53_read(SDIO_IP_WTSR0, 0x4, (uint32_t)(&tx_cnt));
    log_hal_info("TX PKT NO : %d \r\n", (tx_cnt >> 0x8));

    /* Prepare Data and send to slave*/
    struct SDIO_Host_Data  *host_data = (struct SDIO_Host_Data *)msdc_send_test_buf;
    host_data->tx_len = pkt_len ;
    for (int i = 0; i < (pkt_len - 4) / 4 ; i++)
        host_data->data[i] = 0x5a5a5a5a + i;
    tx_status = sdio_cmd53_write(SDIO_IP_WTDR1, 4096, (uint32_t)(&(host_data->tx_len)));
    if (tx_status != 0) {
        log_hal_error("Error Occure! ");
    } else {
        log_hal_info("TX Success!");
    }
    vPortFreeNCExt(eHeapRegion_SYSTEM, msdc_send_test_buf);
    return 0;
}





static unsigned char msdc_set(uint8_t len, char *param[])
{

    hal_sdio_status_t tx_status = -1;
    uint32_t addr = 0;
    uint32_t data = 0;
    uint32_t tmp = 0;
    int pkt_len = 0;

    if (len != 2 && len != 3) {
        cli_puts("Parameter as [addr] [data] <len>\r\n");
        return 0;
    }


    tmp = strtoul(param[0], NULL, 16);
    addr = tmp;

    tmp = strtoul(param[1], NULL, 16);
    data = tmp;

    if (len == 3) {
        pkt_len = atoi(param[2]);
    }

    cmd53_conf_cli.function = HAL_SDIO_FUNCTION_1;                                   /**< Function type for the SDIO COMMAND53. */
    cmd53_conf_cli.block = FALSE;                                                       /**< Indicates whether read/write is in a block mode or not. */
    cmd53_conf_cli.operation = HAL_SDIO_FIXED_ADDRESS;                          /**< Operation mode for the SDIO COMMAND53. */

    if (addr >= 0x15C) {
        log_hal_error("Invalid Addr \r\n");
        return 0;
    }

    log_hal_error("Try Write [0x%08lx] : 0x%08lx \r\n", addr, data);
    /*msdc_init and set block size*/

    if (probe_state != 1) {
        log_hal_error("Pls do probe first !");
        return -1;
    }

    if (!msdc_drv_own()) {
        log_hal_error("Pls check drv own !");
        return -1;
    }

    if (addr != SDIO_IP_WTDR1) {
        tx_status = sdio_cmd53_write(addr, 4, (uint32_t)(&data));
        printf("Write [0x%08lx] : 0x%08lx \r\n", addr, data);
    } else {
        msdc_send_test_buf = (uint32_t *)pvPortMallocNCExt(eHeapRegion_SYSTEM, GPD_BUF_LEN);
        if (msdc_send_test_buf == NULL) {
            log_hal_error("No free space for request, space : %d \r\n", xPortGetFreeHeapSize());
            return -1;
        }
        memset(msdc_send_test_buf, 0x0, GPD_BUF_LEN);
        struct SDIO_Host_Data  *host_data = (struct SDIO_Host_Data *)msdc_send_test_buf;
        host_data->tx_len = pkt_len;
        for (int i = 0; i < (pkt_len - 4) / 4 ; i++)
            host_data->data[i] = 0x5a5a5a5a + i;
        tx_status = sdio_cmd53_write(SDIO_IP_WTDR1, pkt_len, (uint32_t)(&(host_data->tx_len)));
        if (tx_status != 0) {
            log_hal_error("Error Occur!  \r\n");
        }

        vPortFreeNCExt(eHeapRegion_SYSTEM, msdc_send_test_buf);
    }

    if (tx_status != 0) {
        log_hal_error("Write fail! \r\n");
    }

    return 0;
}

static unsigned char msdc_read(uint8_t len, char *param[])
{

    uint32_t val = 0x0;
    hal_sdio_status_t rx_status = -1;
    int len_opt = 4;

    if (len != 2) {
        cli_puts("Parameter as [addr] [length] \r\n");
        return 0;
    }


    uint32_t addr = strtoul(param[0], NULL, 16);

    if (len == 2) {
        len_opt = atoi(param[1]);
    }


    cmd53_conf_cli.function = HAL_SDIO_FUNCTION_1;                                   /**< Function type for the SDIO COMMAND53. */
    cmd53_conf_cli.block = FALSE;                                                       /**< Indicates whether read/write is in a block mode or not. */
    cmd53_conf_cli.operation = HAL_SDIO_FIXED_ADDRESS;                          /**< Operation mode for the SDIO COMMAND53. */

    log_hal_error("Try Read [0x%08lx] \r\n", addr);

    if (addr >= 0x15C) {
        printf("Invalid Addr \r\n");
        return 0;
    }

    if (probe_state != 1) {
        log_hal_error("Pls do probe first !");
        return -1;
    }


    if (!msdc_drv_own()) {
        log_hal_error("Pls check drv own !");
        return -1;
    }

    if (len_opt != 4) {
        log_hal_error("Read Length : %d!  \r\n", len_opt);
        msdc_receive_test_buf = (uint32_t *)pvPortMallocNCExt(eHeapRegion_SYSTEM, GPD_BUF_LEN);
        memset(msdc_receive_test_buf, 0x0, GPD_BUF_LEN);
        if (msdc_receive_test_buf == NULL) {
            log_hal_error("No free space for request, space : %d \r\n", xPortGetFreeHeapSize());
            return -1;
        }
        log_hal_error("buf address : 0x%08lx \r\n", (uint32_t)msdc_receive_test_buf);
        memset(msdc_receive_test_buf, 0x0, GPD_BUF_LEN);
        if (addr == SDIO_IP_WTMDR || addr == SDIO_IP_WRDR0 || addr == SDIO_IP_WRDR1) {
            rx_status = sdio_cmd53_read(addr, len_opt, (uint32_t)(msdc_receive_test_buf));
        }
        log_hal_error("rxQ_data[0] : 0x%08lx, rxQ_data[1022]: 0x%08lx \r\n", msdc_receive_test_buf[0], msdc_receive_test_buf[1022]);
        for (int i = 0; i < (len_opt + 4) / 4  ; i++) {
            printf("[%d] : 0x%08lx \r\n", i, msdc_receive_test_buf[i]);
        }
        vPortFreeNCExt(eHeapRegion_SYSTEM, msdc_receive_test_buf);
    } else {
        rx_status = sdio_cmd53_read(addr, len_opt, (uint32_t)(&val));
        log_hal_error("Read [0x%08lx] : 0x%08lx \r\n", addr, val);
    }

    if (rx_status != 0) {
        log_hal_error("Read fail! status : 0x%08x \r\n");
    } else {
        log_hal_error("Read success! \r\n");
    }

    return 0;
}


static unsigned char msdc_set_clock(uint8_t len, char *param[])
{
    if (len !=  1) {
        cli_puts("Parameter as [clock rate] \r\n");
        return 0;
    }

    int clk = atoi(param[0]);
    if (clk < 0 || clk > 50000) {
        cli_puts("clock rate do not support!!\r\n");
        return 0;
    }

    hal_sdio_set_clock(HAL_SDIO_PORT_0, clk);
    return 0;
}


static unsigned char msdc_52_operation(uint8_t len, char *param[])
{

    if (len != 3 && len != 2) {
        printf("Parameters [addr] [read/write] [data] \r\n");
        return 0;
    }
    uint32_t addr = strtoul(param[0], NULL, 16);
    int rw  = atoi(param[1]);
    uint32_t data = 0x0;
    if (len == 3) {
        data = strtoul(param[2], NULL, 16);
    }
    int status;
    if (rw) {
        status = sdio_cmd52_write(0, addr, data);
    } else {
        status = sdio_cmd52_read(0, addr, &data);
    }
    if (status != 0) {
        printf("cmd 52 failed! \r\n");
    }
    return 0;
}

bool sdio_hif_enable_interrupt(void)
{
    uint32_t value ;

    value = W_INT_EN_SET;

    if (0 != sdio_cmd53_write(SDIO_IP_WHLPCR, 4, (uint32_t)&value)) {
        return false ;
    }

    value = 0x000000087 ;

    if (0 != sdio_cmd53_write(SDIO_IP_WHIER, 4, (uint32_t) &value)) {
        return false;
    }

    return true;
}

void sdio_interrupt_callback(hal_sdio_callback_event_t event, void *user_data)
{
    if (HAL_SDIO_EVENT_SUCCESS == event) {
        printf("sdio INT \r\n");
    }
}


static unsigned char msdc_sdio_intctrl(uint8_t len, char *param[])
{
    if (len != 1)
        return 0;

    int en  = atoi(param[0]);
    uint32_t value = 0x0;
    hal_sdio_status_t status = -1;

    if (en > 0) {
        sdio_hif_enable_interrupt();
        status = sdio_interrupt_register_callback(HAL_SDIO_PORT_0, sdio_interrupt_callback, NULL);
        if (HAL_SDIO_STATUS_OK != status) {
            printf("sdio register callback error. \r\n");
            return status;
        }
    }  //disable interrupt
    else {
        if (0 != sdio_cmd53_write(SDIO_IP_WHLPCR, 4, (uint32_t)&value)) {
            return false ;
        }
        if (0 != sdio_cmd53_write(SDIO_IP_WHIER, 4, (uint32_t)&value)) {
            return false ;
        }

    }
    return 0;
}

static unsigned char msdc_cli_init(uint8_t len, char *param[])
{

    int32_t cnt = 0;
    int speed = 50000;
    int speed_opt = 0;
    if (len !=  1 && len != 2) {
        cli_puts("Parameter as[waiting time(unit in 100ms)] [Speed 0:26M  1 : 50M]\r\n");
        return 0;
    }


    int waiting_time = atoi(param[0]);

    if (len == 2) {
        speed_opt = atoi(param[1]);
        if (speed_opt != 0 && speed_opt != 1) {
            printf("Speed only support :   0(26M)  1(50M) \r\n");
        }
        if (speed_opt == 1) {
            speed = 50000;
        } else {
            speed = 26000;
        }

    }

    cmd53_conf_cli.function = HAL_SDIO_FUNCTION_1;                                   /**< Function type for the SDIO COMMAND53. */
    cmd53_conf_cli.block = FALSE;                                                       /**< Indicates whether read/write is in a block mode or not. */
    cmd53_conf_cli.operation = HAL_SDIO_FIXED_ADDRESS;                          /**< Operation mode for the SDIO COMMAND53. */

    /*msdc_init and set block size*/
    log_hal_error("MSDC INIT! \r\n");
    while (msdc_int_status(speed)) {
        hal_gpt_delay_ms(100);
        cnt += 1;
        if (cnt == waiting_time) {
            log_hal_error("MSDC INIT Timeout \r\n");
            return -1;
        }
        if (probe_state == 1) {
            return 0;
        }
    }
    return 0;

}


static unsigned char msdc_drv_own_retry(uint8_t len, char *param[])
{

    if (len !=  1) {
        cli_puts("Parameter as[waiting time(unit in 100ms)]\r\n");
        return 0;
    }

    int32_t cnt = 0;
    int waiting_time = atoi(param[0]);


    while (!msdc_drv_own()) {
        hal_gpt_delay_ms(100);
        cnt += 1;
        if (cnt == waiting_time) {
            log_hal_error("DRV OWN Timeout \r\n");
            return -1;
        }
    }
    return 0;
}


struct SDIO_Host_Data  host_data;
static unsigned char msdc_lpb(uint8_t len, char *param[])
{

    hal_sdio_status_t rx0_status = 0;
    hal_sdio_status_t rx1_status = 0;
    hal_sdio_status_t tx1_status = 0;
    uint32_t timeout_cnt = 0;
    uint32_t int_status = 0x0;
    uint32_t tx_cnt = 0;
    uint32_t rx_len = 0;
    int cnt = 0;

    if (len != 1) {
        cli_puts("Parameter as [waiting time(unit in 100ms)] \r\n");
        return 0;
    }


    int waiting_time = atoi(param[0]);

    cmd53_conf_cli.function = HAL_SDIO_FUNCTION_1;                                   /**< Function type for the SDIO COMMAND53. */
    cmd53_conf_cli.block = FALSE;                                                       /**< Indicates whether read/write is in a block mode or not. */
    cmd53_conf_cli.operation = HAL_SDIO_FIXED_ADDRESS;                          /**< Operation mode for the SDIO COMMAND53. */


    if (probe_state != 1) {
        log_hal_error("Pls do probe first !");
        return -1;
    }

    while (!msdc_drv_own()) {
        hal_gpt_delay_ms(10);
        if (cnt == waiting_time) {
            log_hal_error("Waiting Status Timeout ");
            return -1;
        }
    }

readStatus:
    sdio_cmd53_read(SDIO_IP_WHISR, 0x4, (uint32_t)(&int_status));

    if ((int_status & (TX_DONE_INT | RX0_DONE_INT | RX1_DONE_INT)) == (TX_DONE_INT | RX0_DONE_INT | RX1_DONE_INT)) {

        log_hal_info("WHISR : 0x%08x", int_status);

        sdio_cmd53_read(SDIO_IP_WTSR0, 0x4, (uint32_t)(&tx_cnt));
        sdio_cmd53_read(SDIO_IP_WRPLR, 0x4, (uint32_t)(&rx_len));
        log_hal_info("SDIO_IP_WTSR0 : 0x%08x, SDIO_IP_WRPLR : 0x%08x", tx_cnt, rx_len);

        //rx0
        rx0_status = sdio_cmd53_read(SDIO_IP_WRDR0, 4092, (uint32_t)(host_data.data));
        log_hal_info("rxQ0_data[0] : 0x%08x, rxQ0_data[1022]: 0x%08x", host_data.data[0], host_data.data[1022]);
        memset(host_data.data, 0x0, 4092);

        //rx1
        rx1_status = sdio_cmd53_read(SDIO_IP_WRDR1, 4092, (uint32_t)(host_data.data));
        log_hal_info("rxQ1_data[1] : 0x%08x, rxQ1_data[1022]: 0x%08x", host_data.data[0], host_data.data[1022]);


        //tx1
        host_data.tx_len = 0x1000;
        tx1_status = sdio_cmd53_write(SDIO_IP_WTDR1, 4096, (uint32_t)(&(host_data.tx_len)));
        log_hal_info("rxQ1_data[1] : 0x%08x, rxQ1_data[1022]: 0x%08x", host_data.data[0], host_data.data[1022]);
    } else {
        if (timeout_cnt  == 10) {
            log_hal_info("Timeout");
            memset(host_data.data, 0x0, 4092);
            return 0;
        }
        hal_gpt_delay_ms(300);
        timeout_cnt++;

        goto readStatus;
    }
    memset(host_data.data, 0x0, 4092);
    log_hal_info("rx0_status : 0x%08x, rx1_status: 0x%08x, tx1_status: 0x%08x", rx0_status, rx1_status, tx1_status);
    if ((rx0_status | rx1_status | tx1_status) == 0x0) {
        int_status = 0x43434343;
        log_hal_info("write H2D0");
        tx1_status = sdio_cmd53_write(SDIO_IP_H2DSM0R, 0x4, (uint32_t)(&int_status));
        log_hal_info("loopback success!");
    }
    return 0;
}

static unsigned char msdc_current_set(uint8_t len, char *param[])
{


    int gpio = 0;
    int lev = 0;
    hal_gpio_driving_current_t driving = 0;
    if (len !=  2) {
        cli_puts("Parameter as[GPIO_#] [current lev]\r\n");
        return 0;
    }
    gpio = atoi(param[0]);
    lev = atoi(param[1]);
    hal_gpio_set_driving_current(gpio, (hal_gpio_driving_current_t)lev);

    hal_gpio_get_driving_current(gpio, &driving);
    log_hal_info(" driving : %d", driving);

    return 0;

}
#define SDIO_GPIO_NUM  6
static unsigned char msdc_current_get(uint8_t len, char *param[])
{

    hal_gpio_driving_current_t driving = 0;
    if (len !=  0) {
        cli_puts("Parameter as Null \r\n");
        return 0;
    }


    for (int i = SDIO_GPIO_NUM; i < SDIO_GPIO_NUM + 6 ; i++) {
        hal_gpio_get_driving_current(i, &driving);
        log_hal_info(" driving : %d", driving);
    }

    return 0;

}

static unsigned char msdc_help(uint8_t len, char *param[])
{
    cli_puts("init\r\n");
    cli_puts("  Parameter as[waiting time(unit in 100ms)] [Speed 0:26M  1 : 50M]\r\n");
    cli_puts("   msdc init 1000   (50M CLK)\r\n");
    cli_puts("   msdc init 1000  0(26M CLK)\r\n");
    cli_puts("\r\n");
    cli_puts("drv_own\r\n");
    cli_puts("  Parameter as[waiting time(unit in 100ms)]\r\n");
    cli_puts("    msdc drv_own 10\r\n");
    cli_puts("\r\n");
    cli_puts("rx\r\n");
    cli_puts("  Parameter as [Data len] [check buf] [waiting time(unit in 100ms)] \r\n");
    cli_puts("  [Data len] is unsed parameter \r\n");
    cli_puts("    msdc rx 4096 1 1000 \r\n");
    cli_puts("\r\n");
    cli_puts("tx\r\n");
    cli_puts("  Parameter as [Data len] [waiting time(unit in 100ms)]\r\n");
    cli_puts("  [Data len] include tx header which is 4-bytes.Therfore, Data length is Data_len - 4 \r\n");
    cli_puts("    msdc tx 4096 1000 \r\n");
    cli_puts("\r\n");
    cli_puts("set\r\n");
    cli_puts("  Parameter as [addr] [data] <len>\r\n");
    cli_puts("    msdc set 0x70 0x11111111 (wrtie 4-byte data to addr)\r\n");
    cli_puts("    msdc set 0x34 0x1 4096 (length only work for addr : 0x34\r\n");
    cli_puts("\r\n");
    cli_puts("read\r\n");
    cli_puts("  Parameter as [addr] [length] \r\n");
    cli_puts("    msdc read 0x10 4 \r\n");
    cli_puts("    msdc read 0x50 4096  (length which is great than 4 byte only work for addr : 0x50 & 0x54\r\n");
    cli_puts("\r\n");
    cli_puts("clk\r\n");
    cli_puts("  Parameter as [clock rate] \r\n");
    cli_puts("    msdc clk 50000 \r\n");
    cli_puts("\r\n");
    cli_puts("f0_cmd52\r\n");
    cli_puts("  Parameters [addr] [read/write] [data] \r\n");
    cli_puts("    msdc f0_cmd52 0x13 0 (cmd52 read)\r\n");
    cli_puts("    msdc f0_cmd52 0x13 1 0x1(cmd52 write)\r\n");
    cli_puts("\r\n");
    cli_puts("lpb_mode\r\n");
    cli_puts("    msdc lpb_mode\r\n");
    cli_puts("\r\n");
    cli_puts("current_set\r\n");
    cli_puts("  Parameters [addr] [read/write] [data] \r\n");
    cli_puts("    msdc current_set 6 3\r\n");
    cli_puts("\r\n");
    cli_puts("current_get\r\n");
    cli_puts("    msdc current_get\r\n");
    return 0;
}


cmd_t msdc_cli_cmds[] = {
    { "init", "sdio master init", msdc_cli_init, NULL },
    { "drv_own", "sdio driver own", msdc_drv_own_retry, NULL },
    { "rx", "sdio master rx", msdc_rx, NULL },
    { "tx", "sdio master tx", msdc_tx, NULL },
    { "set", "sdio master set config", msdc_set, NULL },
    { "read", "sdio master read  config", msdc_read, NULL },
    { "clk", "sdio master set clock", msdc_set_clock, NULL },
    { "f0_cmd52", "Read or Write func0", msdc_52_operation, NULL },
    { "int_mode", "sdio master set clock", msdc_sdio_intctrl, NULL },
    { "lpb_mode", "sdio master loop back", msdc_lpb, NULL },
    { "current_set", "sdio master set driving current", msdc_current_set, NULL },
    { "current_get", "sdio master get driving current", msdc_current_get, NULL },
    { "help", "sdio master help", msdc_help, NULL },
    { NULL, NULL, NULL, NULL }
};
