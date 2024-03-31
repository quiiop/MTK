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
#include "hal.h"
#include "hal_gpt.h"
#include "hal_nvic.h"
#include "hal_gpio_internal.h"
#include "hal_sdio.h"
#include "hal_msdc.h"
#include "ci_sdiom.h"
#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

hal_sdio_command53_config_t cmd53_conf;
static hal_sdio_status_t sdio_cmd53_read(int addr, int length, uint32_t buffer_addr)
{

    hal_sdio_status_t status;
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
    status = hal_sdio_execute_command53(HAL_SDIO_PORT_0, &cmd53_conf);
    return status;
}
static hal_sdio_status_t sdio_cmd53_write(int addr, int length, uint32_t buffer_addr)
{
    hal_sdio_status_t status;
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
    status = hal_sdio_execute_command53(HAL_SDIO_PORT_0, &cmd53_conf);
    return status;
}


static hal_gpio_pin_t _Input_GPIO_PIN = HAL_GPIO_16;


Host_Data host_data_RXQ0;
ci_status_t ci_sdiom_sample_main(unsigned int portnum)
{

    printf("%s get portnum:%u\n", __FUNCTION__, portnum);

    hal_gpio_data_t _gpio_sts = HAL_GPIO_DATA_LOW ;
    hal_sdio_status_t status = -1;
    hal_sdio_status_t rx0_status = 0;
    hal_sdio_status_t tx1_status = 0;
    uint32_t blk_size = 0;
    uint32_t int_status = 0x0;
    uint32_t tx_cnt = 0;
    uint32_t rx_len = 0;
    uint32_t init_cnt = 0;
    uint32_t cnt = 0;
    uint32_t val = 0;
    uint32_t D2H_msg = 0x12345678;
    uint32_t H2D_msg = 0x12345678;
    uint32_t H2D_swint = 0x10000000;
    cmd53_conf.function = HAL_SDIO_FUNCTION_1;                                   /**< Function type for the SDIO COMMAND53. */
    cmd53_conf.block = FALSE;                                                       /**< Indicates whether read/write is in a block mode or not. */
    cmd53_conf.operation = HAL_SDIO_FIXED_ADDRESS;                          /**< Operation mode for the SDIO COMMAND53. */




    hal_gpio_set_direction(_Input_GPIO_PIN, HAL_GPIO_DIRECTION_INPUT);
    // API : 0 : pull  down , 1: pull up
    hal_gpio_set_pupd_register(_Input_GPIO_PIN, 0, 1, 1);// pull down
    hal_pinmux_set_function(_Input_GPIO_PIN,  MT7933_PIN_16_FUNC_GPIO16);

    while (_gpio_sts == HAL_GPIO_DATA_LOW) {

        hal_gpio_get_input(_Input_GPIO_PIN, &_gpio_sts);
        hal_gpt_delay_ms(100);
        init_cnt++;
    };
    printf("sdio master start \n");
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_SDIO);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    hal_sdio_config_t sdio_config = {HAL_SDIO_BUS_WIDTH_4, 50000};
    status = hal_sdio_init(HAL_SDIO_PORT_0, &sdio_config);
    if (HAL_SDIO_STATUS_OK != status) {
        printf("sdio probe failed \n");
        goto Error_Handle;
    }
    status = hal_sdio_set_block_size(HAL_SDIO_PORT_0, HAL_SDIO_FUNCTION_1, SDIO_BLK_SIZE);
    log_hal_error("KH hal_sdio_set_block_size status = %d \r\n", status);
    if (HAL_SDIO_STATUS_OK != status) {
        log_hal_error("sdio set block size error. \r\n");
        goto Error_Handle;
    }

    status = hal_sdio_get_block_size(HAL_SDIO_PORT_0, HAL_SDIO_FUNCTION_1, &blk_size);
    log_hal_error("KH hal_sdio_get_block_size status = %d", status);
    if (HAL_SDIO_STATUS_OK != status) {
        log_hal_error("sdio get block size error. ");
        goto Error_Handle;
    } else {
        log_hal_error("sdio get block size, block size is %ld.", blk_size);
    }

    //DRV_OWN
    val = FW_OWN_REQ_CLR;
    status = sdio_cmd53_write(SDIO_IP_WHLPCR, 0x4, (uint32_t)(&val));
    if (HAL_SDIO_STATUS_OK != status) {
        printf("Host DRV OWN failed! \n");
        goto Error_Handle;
    }
    while (1) {
        if (cnt >= 50) {
            printf("Slave prepare TRX Buffer Timeout \n");
            goto Error_Handle;
        }
        status = sdio_cmd53_read(SDIO_IP_WHISR, 0x4, (uint32_t)(&int_status));
        if (HAL_SDIO_STATUS_OK != status) {
            printf("Master Read INT status failed  \n");
            goto Error_Handle;
        }

        if ((int_status & (TX_DONE_INT | RX0_DONE_INT)) == (TX_DONE_INT | RX0_DONE_INT)) {
            sdio_cmd53_read(SDIO_IP_WTSR0, 0x4, (uint32_t)(&tx_cnt));
            sdio_cmd53_read(SDIO_IP_WRPLR, 0x4, (uint32_t)(&rx_len));

            rx0_status = sdio_cmd53_read(SDIO_IP_WRDR0, 4092, (uint32_t)(host_data_RXQ0.data));
            printf("rxQ0_data[0] : 0x%08lx, rxQ0_data[1022]: 0x%08lx status : 0x%08lx \n", host_data_RXQ0.data[0], host_data_RXQ0.data[1022], int_status);

            if (HAL_SDIO_STATUS_OK != rx0_status) {
                printf("Host RX Failed! \n");
                goto Error_Handle;
            }
            break;
        }
        hal_gpt_delay_ms(100);
        cnt++;
    }

    host_data_RXQ0.tx_len = 0x1000; //data length is 4092
    tx1_status = sdio_cmd53_write(SDIO_IP_WTDR1, 4096, (uint32_t)(&(host_data_RXQ0.tx_len)));
    if (HAL_SDIO_STATUS_OK != tx1_status) {
        printf("Host TX Failed! \n");
        goto Error_Handle;
    }

    //waiting slave to notify host
    while (!(int_status & 0x10000000)) {
        status = sdio_cmd53_read(SDIO_IP_WHISR, 0x4, (uint32_t)(&int_status));
        if (cnt == 100 || HAL_SDIO_STATUS_OK != status) {
            printf("Waiting Slave sw interrupt error! status :  0x%08lx \n", int_status);
            goto  Error_Handle;
        }
        hal_gpt_delay_ms(100);
        cnt++;
    }
    sdio_cmd53_read(SDIO_IP_D2HRM0R, 0x4, (uint32_t)(&D2H_msg));
    if (D2H_msg != 0x87654321) {
        goto Error_Handle;
    }

    //Host trigger interrupt to slave
    status = sdio_cmd53_write(SDIO_IP_H2DSM0R, 0x4, (uint32_t)(&H2D_msg));
    if (HAL_SDIO_STATUS_OK != status) {
        printf("Host Send H2D Failed! \n");
        goto Error_Handle;
    }

    status =  sdio_cmd53_write(SDIO_IP_WSICR, 0x4, (uint32_t)(&H2D_swint));
    if (HAL_SDIO_STATUS_OK != status) {
        printf("Host Trigger INT  Failed! \n");
        goto Error_Handle;
    }


    goto SUCCESS;


Error_Handle:
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_SDIO);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    return CI_FAIL;
SUCCESS:
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_SDIO);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    return CI_PASS;
}
