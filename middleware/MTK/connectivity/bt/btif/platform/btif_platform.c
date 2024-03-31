/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include "btif_main.h"
#include "btif_platform.h"
#include "btif_util.h"
#include "btif_dma.h"
#include "hal_cache.h"

#ifdef CHIP_MT7933
#include "memory_map.h"
#endif

btif_irq_handler_t btif_isr;
btif_irq_handler_t btif_wakeup_isr;
btif_irq_handler_t btif_apdma_tx_isr;
btif_irq_handler_t btif_apdma_rx_isr;

/*
 * Need to align with HAL_CACHE_LINE_SIZE, so change 8 to 32.
 * For more detail, check btif_platform_check_vfifo_buf_align().
 */
static unsigned char p_btif_dma_tx_vfifo[BTIF_DMA_TX_VFF_SIZE] __attribute__((aligned(32))) = {0};
static unsigned char p_btif_dma_rx_vfifo[BTIF_DMA_RX_VFF_SIZE] __attribute__((aligned(32))) = {0};

#ifdef CHIP_MT7933
void *p_btif_irq_dev;
void *p_btif_wakeup_irq_dev;
void *p_btif_apdma_tx_irq_dev;
void *p_btif_apdma_rx_irq_dev;

static void _btif_platform_isr(hal_nvic_irq_t irq)
{
	if (btif_isr)
		btif_isr(irq, p_btif_irq_dev);
}
static void _btif_platform_wakeup_isr(hal_nvic_irq_t irq)
{
	if (btif_wakeup_isr)
		btif_wakeup_isr(irq, p_btif_wakeup_irq_dev);
}
static void _btif_platform_apdma_tx_isr(hal_nvic_irq_t irq)
{
	if (btif_apdma_tx_isr)
		btif_apdma_tx_isr(irq, p_btif_apdma_tx_irq_dev);
}
static void _btif_platform_apdma_rx_isr(hal_nvic_irq_t irq)
{
	if (btif_apdma_rx_isr)
		btif_apdma_rx_isr(irq, p_btif_apdma_rx_irq_dev);
}
#else
static void _btif_platform_isr(int irq, void *dev)
{
	if (btif_isr)
		btif_isr(irq, dev);
}
static void _btif_platform_apdma_tx_isr(int irq, void *dev)
{
	if (btif_apdma_tx_isr)
		btif_apdma_tx_isr(irq, dev);
}
static void _btif_platform_apdma_rx_isr(int irq, void *dev)
{
	if (btif_apdma_rx_isr)
		btif_apdma_rx_isr(irq, dev);
}
#endif

void btif_platform_request_irq(unsigned int irq, btif_irq_handler_t handler,
							   const char *name, void *dev)
{
#ifdef CHIP_MT7933
	if (irq == BTIF_IRQ_ID) {
		btif_isr = handler;
		hal_nvic_register_isr_handler(irq, _btif_platform_isr);
		hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
		p_btif_irq_dev = dev;
		// hal_nvic_enable_irq(irq);

	} else if (irq == BTIF_WAKEUP_IRQ_ID) {
		btif_wakeup_isr = handler;
		hal_nvic_register_isr_handler(irq, _btif_platform_wakeup_isr);
		hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
		p_btif_wakeup_irq_dev = dev;
		hal_nvic_enable_irq(irq);

	} else if (irq == BTIF_APDMA_TX_IRQ_ID) {
		btif_apdma_tx_isr = handler;
		hal_nvic_register_isr_handler(irq, _btif_platform_apdma_tx_isr);
		hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
		p_btif_apdma_tx_irq_dev = dev;
		hal_nvic_enable_irq(irq);

	} else if (irq == BTIF_APDMA_RX_IRQ_ID) {
		btif_apdma_rx_isr = handler;
		hal_nvic_register_isr_handler(irq, _btif_platform_apdma_rx_isr);
		hal_nvic_irq_set_type(irq, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
		p_btif_apdma_rx_irq_dev = dev;
		hal_nvic_enable_irq(irq);

	} else {
		BTIF_LOG_E("%s unknown irq number: %d", __func__, irq);
		return;
	}
// hal_nvic_set_priority(irq, 10); //no need currently
#else
	if (irq == BTIF_IRQ_ID) {
		btif_isr = handler;
		request_irq(irq, _btif_platform_isr, IRQ_TYPE_LEVEL_LOW, name, dev);
	} else if (irq == BTIF_APDMA_TX_IRQ_ID) {
		btif_apdma_tx_isr = handler;
		request_irq(irq, _btif_platform_apdma_tx_isr, IRQ_TYPE_LEVEL_LOW, name, dev);
	} else if (irq == BTIF_APDMA_RX_IRQ_ID) {
		btif_apdma_rx_isr = handler;
		request_irq(irq, _btif_platform_apdma_rx_isr, IRQ_TYPE_LEVEL_LOW, name, dev);
	} else {
		BTIF_LOG_E("%s unknown irq number: %d", __func__, irq);
		return;
	}
#endif
}

void btif_platform_free_irq(unsigned int irq)
{
#ifdef CHIP_MT7933

#else
	free_irq(irq);
#endif
}

int btif_platform_irq_ctrl(unsigned int irq_id, unsigned char en)
{
#ifdef CHIP_MT7933
	if (en)
		return hal_nvic_enable_irq(irq_id);
	else
		return hal_nvic_disable_irq(irq_id);
#else
	if (en)
		enable_irq(irq_id);
	else
		disable_irq(irq_id);

	return 0;
#endif
}

void btif_platform_clk_ctrl(unsigned char en)
{
#ifdef CHIP_MT7933
	BTIF_LOG_V("%s, op = %s", __func__, en == TRUE ? "Enable" : "Disable");
#else
	unsigned int val = 0;

	BTIF_LOG_V("%s, op = %s", __func__, en == TRUE ? "Enable" : "Disable");
	if (en == FALSE) {
		BTIF_WRITE32(BTIF_CG_SET_REG, BTIF_CG_SET_BIT);
		do {
			val = BTIF_READ32(BTIF_CG_STA_REG);
		} while ((val & BTIF_CG_STA_BIT) == 0);
		BTIF_LOG_V("%s, CLK is Disabled", __func__);
	} else {
		BTIF_WRITE32(BTIF_CG_CLR_REG, BTIF_CG_CLR_BIT);
		do {
			val = BTIF_READ32(BTIF_CG_STA_REG);
		} while ((val & BTIF_CG_STA_BIT) != 0);
		BTIF_LOG_V("%s, CLK is Enabled", __func__);
	}
#endif
}

void btif_platform_apdma_clk_ctrl(unsigned char en)
{
#ifdef CHIP_MT7933
	BTIF_LOG_D("%s, op = %s", __func__, en == TRUE ? "Enable" : "Disable");
#else
	unsigned int val = 0;

	BTIF_LOG_D("%s, op = %s", __func__, en == TRUE ? "Enable" : "Disable");
	if (en == FALSE) {
		BTIF_WRITE32(BTIF_APDMA_CG_SET_REG, BTIF_APDMA_CG_SET_BIT);
		do {
			val = BTIF_READ32(BTIF_APDMA_CG_STA_REG);
		} while ((val & BTIF_APDMA_CG_STA_BIT) == 0);
		BTIF_LOG_D("%s, DMA CLK is Disabled", __func__);
	} else {
		BTIF_WRITE32(BTIF_APDMA_CG_CLR_REG, BTIF_APDMA_CG_CLR_BIT);
		do {
			val = BTIF_READ32(BTIF_APDMA_CG_STA_REG);
		} while ((val & BTIF_APDMA_CG_STA_BIT) != 0);
		BTIF_LOG_D("%s, DMA CLK is Enabled", __func__);
	}
#endif
}

void btif_platform_dcache_clean(void)
{
#ifndef CHIP_MT7933
	k_dcache_clean_all();
#endif
}

unsigned char *btif_platform_apdma_get_vfifo(unsigned char dir)
{
	if (dir == DMA_DIR_TX)
#ifdef CHIP_MT7933
		return (unsigned char *)HAL_CACHE_VIRTUAL_TO_PHYSICAL((unsigned int)p_btif_dma_tx_vfifo);
#else
		return p_btif_dma_tx_vfifo;
#endif
	else if (dir == DMA_DIR_RX)
#ifdef CHIP_MT7933
		return (unsigned char *)HAL_CACHE_VIRTUAL_TO_PHYSICAL((unsigned int)p_btif_dma_rx_vfifo);
#else
		return p_btif_dma_rx_vfifo;
#endif
	else
		BTIF_LOG_E("%s, invalid dir(%u)", __func__, dir);
	return NULL;
}

bool btif_platform_check_vfifo_buf_align(void)
{
	/*
	 * Since we use this buffer with physical address,
	 * we need to make sure the address is align with the size of cache line
	 * or the data in physical memory may be overwriten when cache flush.
	 */
	if (((uint32_t)p_btif_dma_tx_vfifo & (HAL_CACHE_LINE_SIZE - 1)) ||
		((uint32_t)p_btif_dma_rx_vfifo & (HAL_CACHE_LINE_SIZE - 1)))
		return false;

	return true;
}

/*---------------------------------------------------------------------------*/
