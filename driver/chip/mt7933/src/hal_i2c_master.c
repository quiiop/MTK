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
#if defined(HAL_I2C_MASTER_MODULE_ENABLED)
#include <stddef.h>
#include <string.h>
#include "mt7933.h"
#include "hal_i2c_master_internal.h"
#include "hal_log.h"
#include "common.h"
#include <stdlib.h>

struct mtk_i2c i2c_mtk[HAL_I2C_MASTER_MAX];

hal_i2c_status_t hal_i2c_master_set_frequency(hal_i2c_port_t i2c_port,
                                              hal_i2c_frequency_t frequency)
{
    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    switch (frequency) {
        case (HAL_I2C_FREQUENCY_50K) :
            i2c_mtk[i2c_port].speed = I2C_FREQUENCY_50K;
            break;
        case (HAL_I2C_FREQUENCY_100K) :
            i2c_mtk[i2c_port].speed = I2C_FREQUENCY_100K;
            break;
        case (HAL_I2C_FREQUENCY_200K) :
            i2c_mtk[i2c_port].speed = I2C_FREQUENCY_200K;
            break;
        case (HAL_I2C_FREQUENCY_300K) :
            i2c_mtk[i2c_port].speed = I2C_FREQUENCY_300K;
            break;
        case (HAL_I2C_FREQUENCY_400K) :
            i2c_mtk[i2c_port].speed = I2C_FREQUENCY_400K;
            break;
        case (HAL_I2C_FREQUENCY_1M) :
            i2c_mtk[i2c_port].speed = I2C_FREQUENCY_1M;
            break;
        default :
            return HAL_I2C_STATUS_INVALID_PARAMETER;
            break;
    }
    return HAL_I2C_STATUS_OK;
}

hal_i2c_status_t hal_i2c_master_init(hal_i2c_port_t i2c_port, hal_i2c_config_t *i2c_config)
{
    hal_i2c_status_t busy_status;
    struct mtk_i2c *i2c;

    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    if (i2c_config == NULL) {
        log_hal_info("i2c_config is NULL!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    i2c = &i2c_mtk[i2c_port];
    i2c->id = i2c_port;
    busy_status = i2c_init(i2c);
    if (busy_status) {
        return busy_status;
    }
    i2c->write_buf = (uint8_t *)pvPortMallocNC(HAL_I2C_MAXIMUM_DMA_TRANSACTION_SIZE);
    if (i2c->write_buf == NULL) {
        return -44;
    }
    i2c->read_buf = (uint8_t *)pvPortMallocNC(HAL_I2C_MAXIMUM_DMA_TRANSACTION_SIZE);
    if (i2c->write_buf == NULL) {
        return -44;
    }
    busy_status = hal_i2c_master_set_frequency(i2c_port, i2c_config->frequency);
    return busy_status;
}

hal_i2c_status_t hal_i2c_master_deinit(hal_i2c_port_t i2c_port)
{
    struct mtk_i2c *i2c;

    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    i2c = &i2c_mtk[i2c_port];

    if (i2c->read_buf != NULL) {
        vPortFreeNC(i2c->read_buf);
    }

    if (i2c->write_buf != NULL) {
        vPortFreeNC(i2c->write_buf);
    }

    return i2c_deinit(i2c);
}

hal_i2c_status_t hal_i2c_master_send_dma(hal_i2c_port_t i2c_port, uint8_t slave_address,
                                         const uint8_t *data, uint32_t size)
{
    hal_i2c_status_t busy_status;
    struct i2c_msg msgs;
    struct mtk_i2c *i2c;

    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    if (data == NULL) {
        log_hal_info("The send data is NULL!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (size > HAL_I2C_MAXIMUM_DMA_TRANSACTION_SIZE) {
        log_hal_info("The size(%ld) of data exceed the MAX DMA size!\n", size);
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    i2c = &i2c_mtk[i2c_port];
    i2c->poll_en = false;
    i2c->dma_en = true;
    i2c->auto_restart = false;
    i2c->pushpull = false;
    i2c->id = i2c_port;
    i2c->addr = slave_address;
    i2c->tmp_len = 0;

    memcpy(i2c->write_buf, data, size);
    msgs.addr = slave_address;
    msgs.flags = 0;
    msgs.buf = i2c->write_buf;
    msgs.len = size;

    busy_status = mtk_i2c_transfer(i2c, &msgs, 1);
    if (busy_status != HAL_I2C_STATUS_OK) {
        log_hal_info("mtk_i2c_write fails(%d).\n", busy_status);
    }

    return busy_status;
}

hal_i2c_status_t hal_i2c_master_receive_dma(hal_i2c_port_t i2c_port, uint8_t slave_address,
                                            uint8_t *buffer, uint32_t size)
{
    hal_i2c_status_t busy_status;
    struct i2c_msg msgs;
    struct mtk_i2c *i2c;

    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    if (buffer == NULL) {
        log_hal_info("The buffer is NULL!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (size > HAL_I2C_MAXIMUM_DMA_TRANSACTION_SIZE) {
        log_hal_info("The size(%ld) of buffer exceed the MAX DMA size!\n", size);
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    i2c = &i2c_mtk[i2c_port];
    i2c->poll_en = false;
    i2c->dma_en = true;
    i2c->auto_restart = false;
    i2c->pushpull = false;
    i2c->id = i2c_port;
    i2c->addr = slave_address;
    i2c->tmp_buf = buffer;
    i2c->tmp_len = size;

    msgs.addr = slave_address;
    msgs.flags = 1;
    msgs.buf = i2c->read_buf;
    msgs.len = size;
    busy_status = mtk_i2c_transfer(i2c, &msgs, 1);

    if (busy_status != HAL_I2C_STATUS_OK) {
        log_hal_info("mtk_i2c_read fails(%d).\n", busy_status);
    }
    if (i2c->i2c_callback == NULL)
        memcpy(buffer, i2c->read_buf, size);

    return busy_status;
}

hal_i2c_status_t hal_i2c_master_send_to_receive_dma(hal_i2c_port_t i2c_port,
                                                    hal_i2c_send_to_receive_config_t *i2c_send_to_receive_config)
{
    hal_i2c_status_t busy_status;
    struct i2c_msg msgs[2];
    struct mtk_i2c *i2c;

    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    if (i2c_send_to_receive_config == NULL) {
        log_hal_info("The config struct is NULL!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (i2c_send_to_receive_config->send_length >
        HAL_I2C_MAXIMUM_DMA_TRANSACTION_SIZE) {
        log_hal_info("The send_length exceed the MAX DMA size!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (i2c_send_to_receive_config->send_data == NULL) {
        log_hal_info("The send_data is NULL!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (i2c_send_to_receive_config->receive_buffer == NULL) {
        log_hal_info("The receive_buffer is NULL!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (i2c_send_to_receive_config->receive_length >
        HAL_I2C_MAXIMUM_DMA_TRANSACTION_SIZE) {
        log_hal_info("The receive_length exceed the MAX DMA size!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    i2c = &i2c_mtk[i2c_port];
    i2c->poll_en = false;
    i2c->dma_en = true;
    i2c->auto_restart = false;
    i2c->pushpull = false;
    i2c->id = i2c_port;
    i2c->addr = i2c_send_to_receive_config->slave_address;
    i2c->tmp_buf = i2c_send_to_receive_config->receive_buffer;
    i2c->tmp_len = i2c_send_to_receive_config->receive_length;;

    msgs[0].addr = i2c_send_to_receive_config->slave_address;
    msgs[0].flags = 0;
    msgs[0].len = i2c_send_to_receive_config->send_length;

    msgs[1].addr = i2c_send_to_receive_config->slave_address;
    msgs[1].flags = 1;
    msgs[1].len = i2c_send_to_receive_config->receive_length;

    memcpy(i2c->write_buf, i2c_send_to_receive_config->send_data, msgs[0].len);
    msgs[0].buf = i2c->write_buf;
    msgs[1].buf = i2c->read_buf;

    busy_status = mtk_i2c_transfer(i2c, msgs, 2);
    if (busy_status != HAL_I2C_STATUS_OK) {
        log_hal_info("mtk_i2c_write_read fails(%d).\n", busy_status);
    }

    if (i2c->i2c_callback == NULL)
        memcpy(i2c_send_to_receive_config->receive_buffer, i2c->read_buf, msgs[1].len);

    return busy_status;
}

hal_i2c_status_t hal_i2c_master_send_polling(hal_i2c_port_t i2c_port, uint8_t slave_address,
                                             const uint8_t *data, uint32_t size)
{
    hal_i2c_status_t busy_status;
    struct i2c_msg msgs;
    struct mtk_i2c *i2c;

    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    if (data == NULL) {
        log_hal_info("The data is NULL!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (size > HAL_I2C_MAXIMUM_POLLING_TRANSACTION_SIZE) {
        log_hal_info("The size(%ld) of data exceed the depth of FIFO!\n", size);
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    i2c = &i2c_mtk[i2c_port];
    i2c->poll_en = true;
    i2c->dma_en = false;
    i2c->auto_restart = false;
    i2c->pushpull = false;
    i2c->id = i2c_port;
    i2c->addr = slave_address;

    msgs.addr = slave_address;
    msgs.flags = 0;
    msgs.buf = (uint8_t *)data;
    msgs.len = size;
    busy_status = mtk_i2c_transfer(i2c, &msgs, 1);
    if (busy_status != HAL_I2C_STATUS_OK) {
        log_hal_info("mtk_i2c_write fails(%d).\n", busy_status);
    }

    return busy_status;
}

hal_i2c_status_t hal_i2c_master_receive_polling(hal_i2c_port_t i2c_port, uint8_t slave_address,
                                                uint8_t *buffer, uint32_t size)
{
    hal_i2c_status_t busy_status;
    struct i2c_msg msgs;
    struct mtk_i2c *i2c;

    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    if (buffer == NULL) {
        log_hal_info("The buffer is NULL!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (size > HAL_I2C_MAXIMUM_POLLING_TRANSACTION_SIZE) {
        log_hal_info("The size(%ld) of buffer exceed the depth of FIFO!\n", size);
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    i2c = &i2c_mtk[i2c_port];
    i2c->poll_en = true;
    i2c->dma_en = false;
    i2c->auto_restart = false;
    i2c->pushpull = false;
    i2c->id = i2c_port;
    i2c->addr = slave_address;

    msgs.addr = slave_address;
    msgs.flags = 1;
    msgs.buf = buffer;
    msgs.len = size;
    busy_status = mtk_i2c_transfer(i2c, &msgs, 1);

    if (busy_status != HAL_I2C_STATUS_OK) {
        log_hal_info("mtk_i2c_read fails(%d).\n", busy_status);
    }

    return busy_status;
}

hal_i2c_status_t hal_i2c_master_send_to_receive_polling(hal_i2c_port_t i2c_port,
                                                        hal_i2c_send_to_receive_config_t *i2c_send_to_receive_config)
{
    hal_i2c_status_t busy_status;
    struct i2c_msg msgs[2];
    struct mtk_i2c *i2c;

    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    if (i2c_send_to_receive_config == NULL) {
        log_hal_info("The config struct is NULL!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (i2c_send_to_receive_config->send_length >
        HAL_I2C_MAXIMUM_POLLING_TRANSACTION_SIZE) {
        log_hal_info("The send_length exceed the depth of FIFO!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (i2c_send_to_receive_config->receive_length >
        HAL_I2C_MAXIMUM_POLLING_TRANSACTION_SIZE) {
        log_hal_info("The receive_length exceed the depth of FIFO!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    i2c = &i2c_mtk[i2c_port];
    i2c->poll_en = true;
    i2c->dma_en = false;
    i2c->auto_restart = false;
    i2c->pushpull = false;
    i2c->id = i2c_port;
    i2c->addr = i2c_send_to_receive_config->slave_address;

    msgs[0].addr = i2c_send_to_receive_config->slave_address;
    msgs[0].flags = 0;
    msgs[0].buf = (uint8_t *)i2c_send_to_receive_config->send_data;
    msgs[0].len = i2c_send_to_receive_config->send_length;

    msgs[1].addr = i2c_send_to_receive_config->slave_address;
    msgs[1].flags = 1;
    msgs[1].buf = i2c_send_to_receive_config->receive_buffer;
    msgs[1].len = i2c_send_to_receive_config->receive_length;
    busy_status = mtk_i2c_transfer(i2c, msgs, 2);

    if (busy_status != HAL_I2C_STATUS_OK) {
        log_hal_info("mtk_i2c_write_read fails(%d).\n", busy_status);
    }
    return busy_status;
}

hal_i2c_status_t hal_i2c_master_get_running_status(hal_i2c_port_t i2c_port,
                                                   hal_i2c_running_status_t *running_status)
{
    hal_i2c_status_t busy_status = HAL_I2C_STATUS_OK;

    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    if (running_status == NULL) {
        log_hal_info("running_status is NULL!\n");
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    if (mtk_i2c_get_running_status(i2c_port))
        running_status->running_status = HAL_I2C_STATUS_BUS_BUSY;
    else
        running_status->running_status = HAL_I2C_STATUS_IDLE;

    return busy_status;
}

hal_i2c_status_t hal_i2c_master_register_callback(hal_i2c_port_t i2c_port,
                                                  hal_i2c_callback_t i2c_callback, void *user_data)
{
    struct mtk_i2c *i2c;

    if (i2c_port >= HAL_I2C_MASTER_MAX) {
        log_hal_info("i2c port(%d) error!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }

    i2c = &i2c_mtk[i2c_port];

    if (i2c_callback != NULL) {
        mtk_irq_handler_register(i2c, i2c_callback, user_data);
        return HAL_I2C_STATUS_OK;
    } else {
        log_hal_info("i2c_callback is NULL!\n", i2c_port);
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }
}

uint16_t hal_I2C_Get_RxNoOfbytes(hal_i2c_port_t i2c_port)
{
    return mtk_i2c_get_rx_bytes(i2c_port);
}

uint16_t hal_I2C_Get_TxNoOfbytes(hal_i2c_port_t i2c_port)
{
    return mtk_i2c_get_tx_bytes(i2c_port);
}

hal_i2c_status_t hal_I2C_BusReset(hal_i2c_port_t i2c_port)
{
    return mtk_i2c_soft_reset(i2c_port);
}

hal_i2c_status_t hal_i2c_master_send_dma_ex(hal_i2c_port_t i2c_port,
                                            hal_i2c_send_config_t *i2c_send_config)
{
    log_hal_info("#error hal_i2c_master_send_dma_ex not support!\n");
    return -99;
}

hal_i2c_status_t hal_i2c_master_receive_dma_ex(hal_i2c_port_t i2c_port,
                                               hal_i2c_receive_config_t *i2c_receive_config)
{
    log_hal_info("#error hal_i2c_master_receive_dma_ex not support!\n");
    return -99;
}

hal_i2c_status_t hal_i2c_master_send_to_receive_dma_ex(hal_i2c_port_t i2c_port,
                                                       hal_i2c_send_to_receive_config_ex_t *i2c_send_to_receive_config_ex)
{
    log_hal_info("#error hal_i2c_master_send_to_receive_dma_ex not support!\n");
    return -99;
}
#endif /* #if defined(HAL_I2C_MASTER_MODULE_ENABLED) */
