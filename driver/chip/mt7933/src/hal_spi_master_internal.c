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

#include "hal_spi_master.h"

#ifdef HAL_SPI_MASTER_MODULE_ENABLED

#include "hal_spi_master_internal.h"
#include "hal_log.h"
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"

#define DIV_ROUND_UP(n,d) (((n)+(d)-1)/(d))

#ifdef HAL_SLEEP_MANAGER_ENABLED
static uint32_t spi_master_cfg0_reg[HAL_SPI_MASTER_MAX] = {0};
static uint32_t spi_master_cfg1_reg[HAL_SPI_MASTER_MAX] = {0};
static uint32_t spi_master_cfg2_reg[HAL_SPI_MASTER_MAX] = {0};
static uint32_t spi_master_cmd_reg[HAL_SPI_MASTER_MAX] = {0};
static uint32_t spi_master_tx_src_reg[HAL_SPI_MASTER_MAX] = {0};
static uint32_t spi_master_rx_dst_reg[HAL_SPI_MASTER_MAX] = {0};
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

extern spi_master_direction_t g_spi_master_direction[HAL_SPI_MASTER_MAX];

unsigned long spim_base_addr[HAL_SPI_MASTER_MAX] = {
    SPIM0_BASE,
    SPIM1_BASE,
};

int spim_irq_num[HAL_SPI_MASTER_MAX] = {
    SPIM0_IRQn,
    SPIM1_IRQn,
};

struct mtk_spim_bus mtk_spim[HAL_SPI_MASTER_MAX];


static struct mtk_spim_bus *mtk_get_spim_bus(hal_spi_master_port_t master_port)
{
    return &mtk_spim[master_port];
}

#ifdef HAL_SLEEP_MANAGER_ENABLED
void spi_master_backup_register_callback(void *data)
{
    uint32_t port_index, reg_base;
    /* mshrink, merge duplicated code block */
    port_index = *(int *)data;
    if (port_index >= HAL_SPI_MASTER_MAX)
        return;

    reg_base = mtk_get_spim_bus(port_index)->reg_base;
    spi_master_cfg0_reg[port_index] = readl(reg_base + SPI_CFG0_REG);
    spi_master_cfg1_reg[port_index] = readl(reg_base + SPI_CFG1_REG);
    spi_master_cfg2_reg[port_index] = readl(reg_base + SPI_CFG2_REG);
    spi_master_cmd_reg[port_index]  = readl(reg_base + SPI_CMD_REG);
    spi_master_tx_src_reg[port_index] = readl(reg_base + SPI_TX_SRC_REG);
    spi_master_rx_dst_reg[port_index] = readl(reg_base + SPI_RX_DST_REG);
}

void spi_master_restore_register_callback(void *data)
{
    uint32_t port_index, reg_base;
    /* mshrink, merge duplicated code block */
    port_index = *(int *)data;
    if (port_index >= HAL_SPI_MASTER_MAX)
        return;

    reg_base = mtk_get_spim_bus(port_index)->reg_base;
    writel(spi_master_cfg0_reg[port_index], reg_base + SPI_CFG0_REG);
    writel(spi_master_cfg1_reg[port_index], reg_base + SPI_CFG1_REG);
    writel(spi_master_cfg2_reg[port_index], reg_base + SPI_CFG2_REG);
    writel(spi_master_cmd_reg[port_index],  reg_base + SPI_CMD_REG);
    writel(spi_master_tx_src_reg[port_index], reg_base + SPI_TX_SRC_REG);
    writel(spi_master_rx_dst_reg[port_index], reg_base + SPI_RX_DST_REG);
}
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

void spi_master_isr_handler(hal_spi_master_port_t master_port, hal_spi_master_callback_t user_callback, void *user_data)
{
    hal_spi_master_callback_event_t event;
    uint32_t interrupt_status = 0;
    struct mtk_spim_bus *bus;

    if (master_port >= 0 && master_port < HAL_SPI_MASTER_MAX) {
        bus = mtk_get_spim_bus(master_port);
        interrupt_status = readl(bus->reg_base + SPI_STATUS0_REG);
        if (interrupt_status & MTK_SPI_FINISH_STATUS) {
            switch (g_spi_master_direction[master_port]) {
                case SPI_MASTER_TX:
                    event = HAL_SPI_MASTER_EVENT_SEND_FINISHED;
                    break;
                case SPI_MASTER_RX:
                    event = HAL_SPI_MASTER_EVENT_RECEIVE_FINISHED;
                    break;
                default:
                    event = HAL_SPI_MASTER_EVENT_RECEIVE_FINISHED;
                    break;
            }

            /* This is just for data corruption check */
            if (NULL != user_callback) {
                user_callback(event, user_data);
            }
#ifdef HAL_SLEEP_MANAGER_ENABLED
            hal_sleep_manager_unlock_sleep(SLEEP_LOCK_SPI_MASTER);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
        } else if (interrupt_status & MTK_SPI_PAUSE_STATUS) {
            spi_master_start_transfer_dma(master_port, true, false);
        }
    }
}

void spi_master_init(hal_spi_master_port_t master_port, const hal_spi_master_config_t *spi_config)
{
    uint32_t sck_count;
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);
    uint32_t div;

    bus->reg_base = spim_base_addr[master_port];
    bus->irq_num = spim_irq_num[master_port];

    if (spi_config->clock_frequency < SPI_MASTER_INPUT_CLOCK_FREQUENCY / 2)
        div = DIV_ROUND_UP(SPI_MASTER_INPUT_CLOCK_FREQUENCY, spi_config->clock_frequency);
    else
        div = 1;
    sck_count = (div + 1) / 2;

    writel(((sck_count - 1) << SPI_CFG2_SCK_LOW_SHIFT) |
           ((sck_count - 1) << SPI_CFG2_SCK_HIGH_SHIFT),
           bus->reg_base + SPI_CFG2_REG);

    reg_val = readl(bus->reg_base + SPI_CMD_REG);
    switch (spi_config->bit_order) {
        case HAL_SPI_MASTER_LSB_FIRST:
            reg_val &= ~SPI_CMD_TXMSBF_EN;
            reg_val &= ~SPI_CMD_RXMSBF_EN;
            break;
        case HAL_SPI_MASTER_MSB_FIRST:
            reg_val |= SPI_CMD_TXMSBF_EN;
            reg_val |= SPI_CMD_RXMSBF_EN;
            break;
    }

    switch (spi_config->polarity) {
        case HAL_SPI_MASTER_CLOCK_POLARITY0:
            reg_val &= ~SPI_CMD_CPOL_EN;
            break;
        case HAL_SPI_MASTER_CLOCK_POLARITY1:
            reg_val |= SPI_CMD_CPOL_EN;
            break;
    }

    switch (spi_config->phase) {
        case HAL_SPI_MASTER_CLOCK_PHASE0:
            reg_val &= ~SPI_CMD_CPHA_EN;
            break;
        case HAL_SPI_MASTER_CLOCK_PHASE1:
            reg_val |= SPI_CMD_CPHA_EN;
            break;
    }

    /* default use non-paused mode*/
    reg_val &= ~SPI_CMD_PAUSE_EN;
    reg_val &= ~SPI_CMD_PAUSE_IE_EN;
    writel(reg_val, bus->reg_base + SPI_CMD_REG);
}

#ifdef HAL_SPI_MASTER_FEATURE_ADVANCED_CONFIG
void spi_master_set_advanced_config(hal_spi_master_port_t master_port, const hal_spi_master_advanced_config_t *advanced_config)
{
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    reg_val = readl(bus->reg_base + SPI_CMD_REG);
    switch (advanced_config->byte_order) {
        case HAL_SPI_MASTER_LITTLE_ENDIAN:
            reg_val &= ~SPI_CMD_TX_ENDIAN_EN;
            reg_val &= ~SPI_CMD_RX_ENDIAN_EN;
            break;
        case HAL_SPI_MASTER_BIG_ENDIAN:
            reg_val |= SPI_CMD_TX_ENDIAN_EN;
            reg_val |= SPI_CMD_RX_ENDIAN_EN;
            break;
    }

    switch (advanced_config->chip_polarity) {
        case HAL_SPI_MASTER_CHIP_SELECT_LOW:
            reg_val &= ~SPI_CMD_CS_POL_EN;
            break;
        case HAL_SPI_MASTER_CHIP_SELECT_HIGH:
            reg_val |= SPI_CMD_CS_POL_EN;
            break;
    }

    switch (advanced_config->sample_select) {
        case HAL_SPI_MASTER_SAMPLE_POSITIVE:
            reg_val &= ~SPI_CMD_SAMPLE_SEL_EN;
            break;
        case HAL_SPI_MASTER_SAMPLE_NEGATIVE:
            reg_val |= SPI_CMD_SAMPLE_SEL_EN;
            break;
    }
    writel(reg_val, bus->reg_base + SPI_CMD_REG);

    reg_val = readl(bus->reg_base + SPI_CFG1_REG);
    reg_val &= ~SPI_CFG1_GET_TICK_DLY_MASK;
    reg_val |= advanced_config->get_tick << SPI_CFG1_GET_TICK_DLY_SHIFT;
    writel(reg_val, bus->reg_base + SPI_CFG1_REG);
}
#endif /* #ifdef HAL_SPI_MASTER_FEATURE_ADVANCED_CONFIG */

uint32_t spi_master_get_status(hal_spi_master_port_t master_port)
{
    volatile uint32_t status;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    status = readl(bus->reg_base + SPI_STATUS1_REG);

    return status;
}

void spi_master_set_rwaddr(hal_spi_master_port_t master_port, spi_master_direction_t type, const uint8_t *addr)

{
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    switch (type) {
        case SPI_MASTER_TX:
            writel((uint32_t)addr, bus->reg_base + SPI_TX_SRC_REG);
            break;
        case SPI_MASTER_RX:
            writel((uint32_t)addr, bus->reg_base + SPI_RX_DST_REG);
            break;
    }
}

hal_spi_master_status_t spi_master_push_data(hal_spi_master_port_t master_port, const uint8_t *data, uint32_t size, uint32_t total_size)
{
    uint32_t spi_data = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    const uint8_t *temp_addr = data;
    uint8_t temp_data = 0;
    uint32_t quotient = 0;
    uint32_t remainder = 0;
    hal_spi_master_status_t status = HAL_SPI_MASTER_STATUS_OK;
    uint32_t tx_endian;
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    /* if byte_order setting is big_endian, return error */
    tx_endian = readl(bus->reg_base + SPI_CMD_REG);
    tx_endian &= SPI_CMD_TX_ENDIAN_EN;
    if (tx_endian) {
        log_hal_error("[SPIM%d][push_data]:big_endian error.\r\n", master_port);
        return HAL_SPI_MASTER_STATUS_ERROR;
    }

    /* clear and configure packet length and count register */
    /* HW limitation: When using FIFO mode, need to configure transfer size before push data to Tx FIFO */
    reg_val = readl(bus->reg_base + SPI_CFG1_REG);
    reg_val &= ~(SPI_CFG1_PACKET_LENGTH_MASK | SPI_CFG1_PACKET_LOOP_MASK);
    reg_val |= (total_size - 1) << SPI_CFG1_PACKET_LENGTH_SHIFT;
    reg_val |= (1 - 1) << SPI_CFG1_PACKET_LOOP_SHIFT;
    writel(reg_val, bus->reg_base + SPI_CFG1_REG);

    quotient = size / sizeof(uint32_t);
    remainder = size % sizeof(uint32_t);

    for (i = 0; i < quotient; i++) {
        spi_data = 0;
        for (j = 0; j < 4; j++) {
            temp_data = (*temp_addr);
            spi_data |= (temp_data << (8 * j));
            temp_addr++;
        }
        writel(spi_data, bus->reg_base + SPI_TX_DATA_REG);
    }
    if (remainder > 0) {
        spi_data = 0;
        for (j = 0; j < 4; j++) {
            temp_data = (*temp_addr);
            spi_data |= (temp_data << (8 * j));
            temp_addr++;
        }
        switch (remainder) {
            case 3:
                writel(spi_data & 0x00FFFFFF, bus->reg_base + SPI_TX_DATA_REG);
                break;
            case 2:
                writel(spi_data & 0x0000FFFF, bus->reg_base + SPI_TX_DATA_REG);
                break;
            case 1:
                writel(spi_data & 0x000000FF, bus->reg_base + SPI_TX_DATA_REG);
                break;
        }
    }

    return status;
}

hal_spi_master_status_t spi_master_pop_data(hal_spi_master_port_t master_port, uint8_t *buffer, uint32_t size)
{
    uint32_t spi_data = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    uint8_t *temp_addr = buffer;
    uint8_t temp_data = 0;
    uint32_t quotient = 0;
    uint32_t remainder = 0;
    hal_spi_master_status_t status = HAL_SPI_MASTER_STATUS_OK;
    uint32_t rx_endian;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    /* if byte_order setting is big_endian, return error */
    rx_endian = readl(bus->reg_base + SPI_CMD_REG);
    rx_endian &= SPI_CMD_RX_ENDIAN_EN;
    if (rx_endian) {
        log_hal_error("[SPIM%d][pop_data]:big_endian error.\r\n", master_port);
        return HAL_SPI_MASTER_STATUS_ERROR;
    }

    quotient = size / sizeof(uint32_t);
    remainder = size % sizeof(uint32_t);

    for (i = 0; i < quotient; i++) {
        spi_data = readl(bus->reg_base + SPI_RX_DATA_REG);
        for (j = 0; j < 4; j++) {
            temp_data = ((spi_data & (0xff << (8 * j))) >> (8 * j));
            *temp_addr = temp_data;
            temp_addr++;
        }
    }
    if (remainder > 0) {
        spi_data = readl(bus->reg_base + SPI_RX_DATA_REG);
        switch (remainder) {
            case 3:
                spi_data &= 0x00FFFFFF;
                break;
            case 2:
                spi_data &= 0x0000FFFF;
                break;
            case 1:
                spi_data &= 0x000000FF;
                break;
        }

        for (j = 0; j < remainder; j++) {
            *temp_addr = (spi_data >> (8 * j));
            temp_addr++;
        }
    }

    return status;
}

void spi_master_set_interrupt(hal_spi_master_port_t master_port, bool status)
{
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    reg_val = readl(bus->reg_base + SPI_CMD_REG);
    switch (status) {
        case false:
            reg_val &= ~SPI_CMD_PAUSE_IE_EN;
            reg_val &= ~SPI_CMD_FINISH_IE_EN;
            break;
        case true:
            reg_val |= SPI_CMD_PAUSE_IE_EN;
            reg_val |= SPI_CMD_FINISH_IE_EN;
            break;
    }
    writel(reg_val, bus->reg_base + SPI_CMD_REG);
}

void spi_master_clear_fifo(hal_spi_master_port_t master_port)
{
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    reg_val = readl(bus->reg_base + SPI_CMD_REG);
    reg_val |= 1 << SPI_CMD_RST_SHIFT;
    writel(reg_val, bus->reg_base + SPI_CMD_REG);

    reg_val = readl(bus->reg_base + SPI_CMD_REG);
    reg_val &= ~(1 << SPI_CMD_RST_SHIFT);
    writel(reg_val, bus->reg_base + SPI_CMD_REG);
}

void spi_master_set_mode(hal_spi_master_port_t master_port, spi_master_direction_t type, spi_master_mode_t mode)
{
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    reg_val = readl(bus->reg_base + SPI_CMD_REG);

    if (SPI_MASTER_TX == type) {
        switch (mode) {
            case SPI_MASTER_MODE_DMA:
                reg_val |= 1 << SPI_CMD_TX_DMA_SHIFT;
                break;
            case SPI_MASTER_MODE_FIFO:
                reg_val &= ~(1 << SPI_CMD_TX_DMA_SHIFT);
                break;
        }
    } else {
        switch (mode) {
            case SPI_MASTER_MODE_DMA:
                reg_val |= 1 << SPI_CMD_RX_DMA_SHIFT;
                break;
            case SPI_MASTER_MODE_FIFO:
                reg_val &= ~(1 << SPI_CMD_RX_DMA_SHIFT);
                break;
        }
    }
    writel(reg_val, bus->reg_base + SPI_CMD_REG);
}

/*
 * g_full_packet_count = x, g_partial_packet_count = y, g_remainder_count = z
 *
 *   index     transfer_start               transfer_end       pause_mode            condition
 *     0             x                           x                  no               x==1,y==0,z==0
 *     1             x                           x                 yes               x>1, y==0,z==0
 *     2             x                           y                 yes               x>=1,y!=0,z==0
 *     3             x                           z                 yes               x>=1,y>=0,z!=0
 *     4             y                           y                  no               x==0,y!=0,z==0
 *     5             y                           z                 yes               x==0,y!=0,z!=0
 *     6             z                           z                  no               x==0,y==0,z!=0
 */

typedef enum {
    PAUSE_END_NONE,
    PAUSE_END_FULL,
    PAUSE_END_PARTIAL,
    PAUSE_END_REMAINDER,
} pause_end_t;
static const bool g_pause_mode_on[7] = {false, true, true, true, false, true, false};
static const pause_end_t g_pause_mode_off[7] = {PAUSE_END_NONE, PAUSE_END_FULL, PAUSE_END_PARTIAL, PAUSE_END_REMAINDER, PAUSE_END_NONE, PAUSE_END_REMAINDER, PAUSE_END_NONE};
static uint32_t g_pause_mode_index[HAL_SPI_MASTER_MAX] = {0};
static uint32_t g_full_packet_count[HAL_SPI_MASTER_MAX] = {0};
static uint32_t g_partial_packet_count[HAL_SPI_MASTER_MAX] = {0};
static uint32_t g_remainder_count[HAL_SPI_MASTER_MAX] = {0};

void spi_master_start_transfer_fifo(hal_spi_master_port_t master_port, bool is_write)
{
    uint32_t status;
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    /* HW limitation: When using FIFO mode, need to configure transfer size before push data to Tx FIFO */
    reg_val = readl(bus->reg_base + SPI_CMD_REG);
    reg_val |= 1 << SPI_CMD_ACT_SHIFT;
    writel(reg_val, bus->reg_base + SPI_CMD_REG);
    do {
        status = readl(bus->reg_base + SPI_STATUS1_REG);
    } while ((status == MTK_SPI_BUSY_STATUS));
    /* read clear the finish flag after transfer complete */
    status = readl(bus->reg_base + SPI_STATUS0_REG);
}

void spi_master_start_transfer_dma(hal_spi_master_port_t master_port, bool is_continue, bool is_write)
{
    bool continue_pause_mode = true;
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    if (is_continue == true) {
        if (!(readl(bus->reg_base + SPI_CMD_REG) & SPI_CMD_PAUSE_EN)) {
            log_hal_error("[spi-%d]: pause status error.\r\n", master_port);
            return;
        }
    }

    reg_val = readl(bus->reg_base + SPI_CFG1_REG);
    if (g_full_packet_count[master_port] > 0) {
        reg_val &= ~(SPI_CFG1_PACKET_LENGTH_MASK | SPI_CFG1_PACKET_LOOP_MASK);
        reg_val |= (MTK_PACKET_LENGTH - 1) << SPI_CFG1_PACKET_LENGTH_SHIFT;
        reg_val |= (MTK_PACKET_LOOP_CNT - 1) << SPI_CFG1_PACKET_LOOP_SHIFT;
        writel(reg_val, bus->reg_base + SPI_CFG1_REG);
        g_full_packet_count[master_port]--;
        if ((g_pause_mode_off[g_pause_mode_index[master_port]] == PAUSE_END_FULL) &&
            (g_full_packet_count[master_port] == 0)) {
            continue_pause_mode = false;
        }
    } else if (g_partial_packet_count[master_port] > 0) {
        /* only need one transfer for g_partial_packet_count */
        reg_val &= ~(SPI_CFG1_PACKET_LENGTH_MASK | SPI_CFG1_PACKET_LOOP_MASK);
        reg_val |= (MTK_PACKET_LENGTH - 1) << SPI_CFG1_PACKET_LENGTH_SHIFT;
        reg_val |= ((uint8_t)g_partial_packet_count[master_port] - 1) << SPI_CFG1_PACKET_LOOP_SHIFT;
        writel(reg_val, bus->reg_base + SPI_CFG1_REG);
        g_partial_packet_count[master_port] = 0;
        if (g_pause_mode_off[g_pause_mode_index[master_port]] == PAUSE_END_PARTIAL) {
            continue_pause_mode = false;
        }
    } else if (g_remainder_count[master_port] > 0) {
        /* packet_loop_cnt = 0 */
        reg_val &= ~(SPI_CFG1_PACKET_LENGTH_MASK | SPI_CFG1_PACKET_LOOP_MASK);
        reg_val |= (g_remainder_count[master_port] - 1) << SPI_CFG1_PACKET_LENGTH_SHIFT;
        reg_val |= (1 - 1) << SPI_CFG1_PACKET_LOOP_SHIFT;
        writel(reg_val, bus->reg_base + SPI_CFG1_REG);
        if (g_pause_mode_off[g_pause_mode_index[master_port]] == PAUSE_END_REMAINDER) {
            continue_pause_mode = false;
        }
    } else {
        log_hal_error("[spi-%d]: machine status error.\r\n", master_port);
        return;
    }

    reg_val = readl(bus->reg_base + SPI_CMD_REG);
    if (is_continue == false) {
        if (g_pause_mode_on[g_pause_mode_index[master_port]] == true) {
            reg_val |= SPI_CMD_PAUSE_EN;
        }
        reg_val |= 1 << SPI_CMD_ACT_SHIFT;
    } else {
        if (continue_pause_mode == false) {
            reg_val &= ~SPI_CMD_PAUSE_EN;
            reg_val |= 1 << SPI_CMD_RESUME_SHIFT;
        } else {
            reg_val |= 1 << SPI_CMD_RESUME_SHIFT;
        }
    }
    writel(reg_val, bus->reg_base + SPI_CMD_REG);
}

void spi_master_start_transfer_dma_blocking(hal_spi_master_port_t master_port, bool is_write)
{
    bool continue_pause_mode, is_continue, loop_end;
    uint32_t irq_status;
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    is_continue = false;
    continue_pause_mode = true;
    do {
        reg_val = readl(bus->reg_base + SPI_CFG1_REG);
        if (g_full_packet_count[master_port] > 0) {
            reg_val &= ~(SPI_CFG1_PACKET_LENGTH_MASK | SPI_CFG1_PACKET_LOOP_MASK);
            reg_val |= (MTK_PACKET_LENGTH - 1) << SPI_CFG1_PACKET_LENGTH_SHIFT;
            reg_val |= (MTK_PACKET_LOOP_CNT - 1) << SPI_CFG1_PACKET_LOOP_SHIFT;
            writel(reg_val, bus->reg_base + SPI_CFG1_REG);

            g_full_packet_count[master_port]--;
            if ((g_pause_mode_off[g_pause_mode_index[master_port]] == PAUSE_END_FULL) &&
                (g_full_packet_count[master_port] == 0)) {
                continue_pause_mode = false;
            }
        } else if (g_partial_packet_count[master_port] > 0) {
            /* only need one transfer for g_partial_packet_count */
            reg_val &= ~(SPI_CFG1_PACKET_LENGTH_MASK | SPI_CFG1_PACKET_LOOP_MASK);
            reg_val |= (MTK_PACKET_LENGTH - 1) << SPI_CFG1_PACKET_LENGTH_SHIFT;
            reg_val |= (uint8_t)(g_partial_packet_count[master_port] - 1) << SPI_CFG1_PACKET_LOOP_SHIFT;
            writel(reg_val, bus->reg_base + SPI_CFG1_REG);
            g_partial_packet_count[master_port] = 0;
            if (g_pause_mode_off[g_pause_mode_index[master_port]] == PAUSE_END_PARTIAL) {
                continue_pause_mode = false;
            }
        } else if (g_remainder_count[master_port] > 0) {
            /* packet_loop_cnt = 0 */
            reg_val &= ~(SPI_CFG1_PACKET_LENGTH_MASK | SPI_CFG1_PACKET_LOOP_MASK);
            reg_val |= (g_remainder_count[master_port] - 1) << SPI_CFG1_PACKET_LENGTH_SHIFT;
            reg_val |= (1 - 1) << SPI_CFG1_PACKET_LOOP_SHIFT;
            writel(reg_val, bus->reg_base + SPI_CFG1_REG);
            if (g_pause_mode_off[g_pause_mode_index[master_port]] == PAUSE_END_REMAINDER) {
                continue_pause_mode = false;
            }
        } else {
            log_hal_error("[spi-%d]: machine status error.\r\n", master_port);
            return;
        }

        reg_val = readl(bus->reg_base + SPI_CMD_REG);
        if (is_continue == false) {
            loop_end = true;
            if (g_pause_mode_on[g_pause_mode_index[master_port]] == true) {
                reg_val |= SPI_CMD_PAUSE_EN;
                loop_end = false;
            }
            reg_val |= 1 << SPI_CMD_ACT_SHIFT;
            is_continue = true;
        } else {
            if (continue_pause_mode == false) {
                reg_val &= ~SPI_CMD_PAUSE_EN;
                reg_val |= 1 << SPI_CMD_RESUME_SHIFT;
                loop_end = true;
            } else {
                reg_val |= 1 << SPI_CMD_RESUME_SHIFT;
                loop_end = false;
            }
        }
        writel(reg_val, bus->reg_base + SPI_CMD_REG);

        /* Wait current part of transfer finish. */
        while (readl(bus->reg_base + SPI_STATUS1_REG) == MTK_SPI_BUSY_STATUS);
        do {
            irq_status = readl(bus->reg_base + SPI_STATUS0_REG);
        } while ((irq_status != MTK_SPI_FINISH_STATUS) && (irq_status != MTK_SPI_PAUSE_STATUS));
    } while (loop_end == false);
}

hal_spi_master_status_t spi_master_analyse_transfer_size(hal_spi_master_port_t master_port, uint32_t size)
{
    uint32_t remainder;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    g_full_packet_count[master_port] = (size / (MTK_PACKET_LOOP_CNT * MTK_PACKET_LENGTH));
    remainder = size % (MTK_PACKET_LOOP_CNT * MTK_PACKET_LENGTH);
    g_partial_packet_count[master_port] = remainder / MTK_PACKET_LENGTH;
    g_remainder_count[master_port] = remainder % MTK_PACKET_LENGTH;

    /*
     * 1. decide whether we need use pause mode.
     * 2. decide where should we stop pause mode.
     * 3. Refer comment above about how to decide index here.
     */
    if (g_full_packet_count[master_port] > 0) {
        if (g_remainder_count[master_port] > 0) {
            g_pause_mode_index[master_port] = 3;
        } else if (g_partial_packet_count[master_port] > 0) {
            g_pause_mode_index[master_port] = 2;
        } else if (g_full_packet_count[master_port] > 1) {
            g_pause_mode_index[master_port] = 1;
        } else {
            g_pause_mode_index[master_port] = 0;
        }
    } else {
        if (g_remainder_count[master_port] == 0) {
            g_pause_mode_index[master_port] = 4;
        } else if (g_partial_packet_count[master_port] > 0) {
            g_pause_mode_index[master_port] = 5;
        } else {
            g_pause_mode_index[master_port] = 6;
        }
    }

    /* When we need pause mode, de-assert must NOT be enabled. */
    if (g_pause_mode_on[g_pause_mode_index[master_port]] == true) {
        if (readl(bus->reg_base + SPI_CMD_REG) & SPI_CMD_DEASSERT_EN) {
            log_hal_error("[SPIM%d]:pause deassert mode error.\r\n", master_port);
            return HAL_SPI_MASTER_STATUS_ERROR;
        }
    }

    return HAL_SPI_MASTER_STATUS_OK;
}

#ifdef HAL_SPI_MASTER_FEATURE_CHIP_SELECT_TIMING
void spi_master_set_chip_select_timing(hal_spi_master_port_t master_port, hal_spi_master_chip_select_timing_t chip_select_timing)
{
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    writel(((chip_select_timing.chip_select_hold_count - 1) << SPI_CFG0_CS_HOLD_SHIFT) |
           ((chip_select_timing.chip_select_setup_count - 1) << SPI_CFG0_CS_SETUP_SHIFT),
           bus->reg_base + SPI_CFG0_REG);

    reg_val = readl(bus->reg_base + SPI_CFG1_REG);
    reg_val &= ~SPI_CFG1_CS_IDLE_MASK;
    reg_val |= (chip_select_timing.chip_select_idle_count - 1) << SPI_CFG1_CS_IDLE_SHIFT;
    writel(reg_val, bus->reg_base + SPI_CFG1_REG);
}
#endif /* #ifdef HAL_SPI_MASTER_FEATURE_CHIP_SELECT_TIMING */

#ifdef HAL_SPI_MASTER_FEATURE_DEASSERT_CONFIG
void spi_master_set_deassert(hal_spi_master_port_t master_port, hal_spi_master_deassert_t deassert)
{
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    reg_val = readl(bus->reg_base + SPI_CMD_REG);
    switch (deassert) {
        case HAL_SPI_MASTER_DEASSERT_DISABLE:
            reg_val &= ~SPI_CMD_DEASSERT_EN;
            break;
        case HAL_SPI_MASTER_DEASSERT_ENABLE:
            reg_val |= SPI_CMD_DEASSERT_EN;
            break;
    }
    writel(reg_val, bus->reg_base + SPI_CMD_REG);
}
#endif /* #ifdef HAL_SPI_MASTER_FEATURE_DEASSERT_CONFIG */

void spi_master_reset_default_value(hal_spi_master_port_t master_port)
{
    uint32_t reg_val;
    struct mtk_spim_bus *bus = mtk_get_spim_bus(master_port);

    bus->reg_base = spim_base_addr[master_port];

    reg_val = 0x00;
    writel(reg_val, bus->reg_base + SPI_CFG0_REG);
    writel(reg_val, bus->reg_base + SPI_CFG1_REG);
    writel(reg_val, bus->reg_base + SPI_TX_SRC_REG);
    writel(reg_val, bus->reg_base + SPI_RX_DST_REG);
    writel(reg_val, bus->reg_base + SPI_TX_DATA_REG);
    readl(bus->reg_base + SPI_RX_DATA_REG);
    writel(reg_val, bus->reg_base + SPI_CMD_REG);
    readl(bus->reg_base + SPI_STATUS0_REG);
    readl(bus->reg_base + SPI_STATUS1_REG);
    writel(reg_val, bus->reg_base + SPI_CFG2_REG);
}

#endif /* #ifdef HAL_SPI_MASTER_MODULE_ENABLED */

