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

#include "hal_spi_slave.h"

#ifdef HAL_SPI_SLAVE_MODULE_ENABLED

#include "hal_spi_slave_internal.h"
#include "hal_clock.h"
#include "hal_log.h"
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"

#ifdef HAL_SLEEP_MANAGER_ENABLED
static uint32_t spi_slave_irq_en_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t spi_slave_irq_mask_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t spi_slave_cfg_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t spi_slave_dma_cfg_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t spi_slave_tx_src_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t spi_slave_rx_dst_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t spi_slave_tx_fifo_default_reg[HAL_SPI_SLAVE_MAX] = {0};
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

static uint32_t spis_base_addr[HAL_SPI_SLAVE_MAX] = {
    SPIS_BASE,
};

struct mtk_spis_bus {
    uint32_t reg_base;
};

static struct mtk_spis_bus mtk_spis[HAL_SPI_SLAVE_MAX];

static struct mtk_spis_bus *mtk_get_spis_bus(hal_spi_slave_port_t spi_port)
{
    return &mtk_spis[spi_port];
}

static int mtk_spis_addr_4bytes_align_check(uint32_t addr)
{
    return (addr & 0x3) ? HAL_SPI_SLAVE_STATUS_INVALID_PARAMETER : HAL_SPI_SLAVE_STATUS_OK;
}

static void mtk_spis_dma_map(hal_spi_slave_port_t spi_port, struct spis_transfer *xfer)
{
    if (xfer->tx_buf) {
        xfer->tx_dma = (uint32_t)xfer->tx_buf;
        log_hal_error("mtk_spis_dma_map:tx_buf(%p), tx_dma(0x%lx)\n",
                      xfer->tx_buf, xfer->tx_dma);
    }

    if (xfer->rx_buf) {
        xfer->rx_dma = (uint32_t)xfer->rx_buf;
        log_hal_error("mtk_spis_dma_map:rx_buf(%p), rx_dma(0x%lx)\n",
                      xfer->rx_buf, xfer->rx_dma);
    }
}

static void mtk_spis_disable_dma(hal_spi_slave_port_t spi_port)
{
    int reg_val;
    struct mtk_spis_bus *bus = mtk_get_spis_bus(spi_port);

    /* disable dma tx/rx */
    reg_val = readl(bus->reg_base + SPIS_DMA_CFG_REG);
    reg_val &= ~RX_DMA_EN;
    reg_val &= ~TX_DMA_EN;
    writel(reg_val, bus->reg_base + SPIS_DMA_CFG_REG);
}

static void mtk_spis_disable_xfer(hal_spi_slave_port_t spi_port)
{
    int reg_val;
    struct mtk_spis_bus *bus = mtk_get_spis_bus(spi_port);

    /* disable config reg tx rx_enable */
    reg_val = readl(bus->reg_base + SPIS_CFG_REG);
    reg_val &= ~SPIS_TX_EN;
    reg_val &= ~SPIS_RX_EN;
    writel(reg_val, bus->reg_base + SPIS_CFG_REG);
}

void spi_slave_lisr(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{
    hal_spi_slave_transaction_status_t status;
    hal_spi_slave_fsm_status_t fsm_state = HAL_SPI_SLAVE_FSM_SUCCESS_OPERATION;
    struct mtk_spis_bus *bus = mtk_get_spis_bus(spi_port);
    int interrupt_status;

    interrupt_status = readl(bus->reg_base + SPIS_IRQ_ST_REG);
    writel(interrupt_status, bus->reg_base + SPIS_IRQ_CLR_REG);

    status.interrupt_status = HAL_SPI_SLAVE_EVENT_DMA_DONE;
    status.fsm_status = fsm_state;
    if (NULL != user_callback) {
        user_callback(status, user_data);
    }
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_SPI_SLAVE);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
}

static int mtk_spis_dma_transfer(hal_spi_slave_port_t spi_port, struct spis_transfer *xfer)
{
    int reg_val, ret;
    struct mtk_spis_bus *bus = mtk_get_spis_bus(spi_port);

    if (xfer->tx_buf) {
        ret = mtk_spis_addr_4bytes_align_check((uint32_t) xfer->tx_buf);
        if (ret)
            return ret;
    }
    if (xfer->rx_buf) {
        ret = mtk_spis_addr_4bytes_align_check((uint32_t) xfer->rx_buf);
        if (ret)
            return ret;
    }

    /* soft rest for dma */
    writel(SPIS_SOFT_RST1, bus->reg_base + SPIS_SOFT_RST_REG);

    /* just enable dma_done enable and mask */
    reg_val = readl(bus->reg_base + SPIS_IRQ_EN_REG);
    reg_val |= DMA_DONE_EN;
    writel(reg_val, bus->reg_base + SPIS_IRQ_EN_REG);
    reg_val = readl(bus->reg_base + SPIS_IRQ_MASK_REG);
    reg_val |= DMA_DONE_MASK;
    writel(reg_val, bus->reg_base + SPIS_IRQ_MASK_REG);

    mtk_spis_disable_dma(spi_port);
    mtk_spis_disable_xfer(spi_port);

    mtk_spis_dma_map(spi_port, xfer);

    if (xfer->tx_buf)
        writel(xfer->tx_dma, bus->reg_base + SPIS_TX_SRC_REG);

    if (xfer->rx_buf)
        writel(xfer->rx_dma, bus->reg_base + SPIS_RX_DST_REG);

    writel(SPIS_DMA_ADDR_LOAD, bus->reg_base + SPIS_SOFT_RST_REG);

    /* enable config reg tx/rx_enable */
    reg_val = readl(bus->reg_base + SPIS_CFG_REG);
    if (xfer->tx_buf)
        reg_val |= SPIS_TX_EN;
    if (xfer->rx_buf)
        reg_val |= SPIS_RX_EN;
    writel(reg_val, bus->reg_base + SPIS_CFG_REG);

    /* config dma */
    reg_val = readl(bus->reg_base + SPIS_DMA_CFG_REG);
    reg_val &= ~TX_DMA_LEN_MASK;
    reg_val |= (xfer->len - 1) & TX_DMA_LEN_MASK;
    writel(reg_val, bus->reg_base + SPIS_DMA_CFG_REG);

    reg_val = readl(bus->reg_base + SPIS_DMA_CFG_REG);
    if (xfer->tx_buf) {
        reg_val |= TX_DMA_EN;
        reg_val |= TX_DMA_TRIG_EN;
    }

    if (xfer->rx_buf)
        reg_val |= RX_DMA_EN;

    writel(reg_val, bus->reg_base + SPIS_DMA_CFG_REG);

    return HAL_SPI_SLAVE_STATUS_OK;
}

static int mtk_spis_transfer_one(hal_spi_slave_port_t spi_port, struct spis_transfer *xfer)
{
    int ret;

    if ((!xfer->len) || ((!xfer->tx_buf) && (!xfer->rx_buf))) {
        log_hal_error("mtk_spis_transfer_one:Invalid parameter\n");
        return HAL_SPI_SLAVE_STATUS_INVALID_PARAMETER;
    }

    ret = mtk_spis_dma_transfer(spi_port, xfer);

    if (ret)
        return ret;

    return HAL_SPI_SLAVE_STATUS_OK;
}

hal_spi_slave_status_t spi_slave_init(hal_spi_slave_port_t spi_port, const hal_spi_slave_config_t *spi_config)
{
    int reg_val;
    struct mtk_spis_bus *bus = mtk_get_spis_bus(spi_port);

    bus->reg_base = spis_base_addr[spi_port];

    reg_val = readl(bus->reg_base + SPIS_CFG_REG);
    if (spi_config->bit_order)
        reg_val |= SPIS_TXMSBF;
    else
        reg_val &= ~SPIS_TXMSBF;

    if (spi_config->bit_order)
        reg_val |= SPIS_RXMSBF;
    else
        reg_val &= ~SPIS_RXMSBF;

    if (spi_config->phase)
        reg_val |= SPIS_CPHA;
    else
        reg_val &= ~SPIS_CPHA;

    if (spi_config->polarity)
        reg_val |= SPIS_CPOL;
    else
        reg_val &= ~SPIS_CPOL;

    if (spi_config->endian)
        reg_val |= SPIS_TX_ENDIAN;
    else
        reg_val &= ~SPIS_TX_ENDIAN;

    if (spi_config->endian)
        reg_val |= SPIS_RX_ENDIAN;
    else
        reg_val &= ~SPIS_RX_ENDIAN;
    writel(reg_val, bus->reg_base + SPIS_CFG_REG);

    mtk_spis_disable_dma(spi_port);
    mtk_spis_disable_xfer(spi_port);

    return HAL_SPI_SLAVE_STATUS_OK;
}

hal_spi_slave_status_t spi_slave_receive(hal_spi_slave_port_t spi_port, uint8_t *buffer, uint32_t size)
{
    struct spis_transfer xfer = {0,};
    int ret;

    xfer.rx_buf = buffer;
    xfer.len = size;
    ret = mtk_spis_transfer_one(spi_port, &xfer);
    if (ret) {
        log_hal_error("mtk_spis_transfer_one fail\n");
        return ret;
    }

    return HAL_SPI_SLAVE_STATUS_OK;
}

hal_spi_slave_status_t spi_slave_send_and_receive(hal_spi_slave_port_t spi_port, const uint8_t *tx_buf, uint8_t *rx_buf, uint32_t size)
{
    struct spis_transfer xfer = {0,};
    int ret;

    xfer.tx_buf = tx_buf;
    xfer.rx_buf = rx_buf;
    xfer.len = size;

    ret = mtk_spis_transfer_one(spi_port, &xfer);
    if (ret) {
        log_hal_error("mtk_spis_transfer_one fail\n");
        return ret;
    }

    return HAL_SPI_SLAVE_STATUS_OK;
}

#ifdef HAL_SLEEP_MANAGER_ENABLED
void spi_slave_backup_register_callback(void *data)
{
    struct mtk_spis_bus *bus0 = mtk_get_spis_bus(HAL_SPI_SLAVE_0);

    spi_slave_irq_en_reg[HAL_SPI_SLAVE_0] = readl(bus0->reg_base + SPIS_IRQ_EN_REG);
    spi_slave_irq_mask_reg[HAL_SPI_SLAVE_0] = readl(bus0->reg_base + SPIS_IRQ_MASK_REG);
    spi_slave_cfg_reg[HAL_SPI_SLAVE_0] = readl(bus0->reg_base + SPIS_CFG_REG);
    spi_slave_dma_cfg_reg[HAL_SPI_SLAVE_0] = readl(bus0->reg_base + SPIS_DMA_CFG_REG);
    spi_slave_tx_src_reg[HAL_SPI_SLAVE_0] = readl(bus0->reg_base + SPIS_TX_SRC_REG);
    spi_slave_rx_dst_reg[HAL_SPI_SLAVE_0] = readl(bus0->reg_base + SPIS_RX_DST_REG);
    spi_slave_tx_fifo_default_reg[HAL_SPI_SLAVE_0] = readl(bus0->reg_base + SPIS_TX_FIFO_DEFAULT_REG);
}

void spi_slave_restore_register_callback(void *data)
{
    struct mtk_spis_bus *bus0 = mtk_get_spis_bus(HAL_SPI_SLAVE_0);

    writel(spi_slave_irq_en_reg[HAL_SPI_SLAVE_0], bus0->reg_base + SPIS_IRQ_EN_REG);
    writel(spi_slave_irq_mask_reg[HAL_SPI_SLAVE_0], bus0->reg_base + SPIS_IRQ_MASK_REG);
    writel(spi_slave_cfg_reg[HAL_SPI_SLAVE_0], bus0->reg_base + SPIS_CFG_REG);
    writel(spi_slave_dma_cfg_reg[HAL_SPI_SLAVE_0], bus0->reg_base + SPIS_DMA_CFG_REG);
    writel(spi_slave_tx_src_reg[HAL_SPI_SLAVE_0], bus0->reg_base + SPIS_TX_SRC_REG);
    writel(spi_slave_rx_dst_reg[HAL_SPI_SLAVE_0], bus0->reg_base + SPIS_RX_DST_REG);
    writel(spi_slave_tx_fifo_default_reg[HAL_SPI_SLAVE_0], bus0->reg_base + SPIS_TX_FIFO_DEFAULT_REG);
}
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

#endif /* #ifdef HAL_SPI_SLAVE_MODULE_ENABLED */

