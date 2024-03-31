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

/*-----------linux system header files----------------*/

#include <string.h>

#include "FreeRTOS.h"
#include "btif_dma.h"
#include "btif_main.h"

#ifdef HAL_GPT_MODULE_ENABLED
#include "hal_gpt.h"
#endif

#define BTIF_DMA_TX_INIT(i, s, t)							\
	{														\
		.dir = DMA_DIR_TX,									\
		.enabled = FALSE,									\
		.irq_id = (i),										\
		.p_base = ((unsigned char *)BTIF_APDMA_TX_BASE),	\
		.p_cur = NULL,										\
		.vfifo_size = (s),									\
		.p_vfifo = NULL,									\
		.thre = (t),										\
		.wpt = 0,											\
		.last_wpt_wrap = 0,									\
		.rpt = 0,											\
		.last_rpt_wrap = 0,									\
		.dma_tx_semaphore = NULL							\
	}

#define BTIF_DMA_RX_INIT(i, s, t)							\
	{														\
		.dir = DMA_DIR_RX,									\
		.enabled = FALSE,									\
		.irq_id = (i),										\
		.p_base = ((unsigned char *)BTIF_APDMA_RX_BASE),	\
		.vfifo_size = (s),									\
		.p_vfifo = NULL,									\
		.p_cur = NULL,										\
		.thre = (t),										\
		.wpt = 0,											\
		.rpt = 0,											\
		.rx_cb = NULL										\
	}

struct btif_dma_t g_btif_dma[] = {
	BTIF_DMA_TX_INIT(BTIF_APDMA_TX_IRQ_ID, BTIF_DMA_TX_VFF_SIZE,
					 BTIF_DMA_TX_THRE(BTIF_DMA_TX_VFF_SIZE)),
	BTIF_DMA_RX_INIT(BTIF_APDMA_RX_IRQ_ID, BTIF_DMA_RX_VFF_SIZE,
					 BTIF_DMA_RX_THRE(BTIF_DMA_RX_VFF_SIZE))};

extern uint8_t btmtk_get_own_type(void);

static unsigned int time_elapse(portTickType last, portTickType curr)
{
	portTickType elapse = 0;
	// this API works if the tick elapses less than 0xFFFFFFFF times
	if (curr >= last) { // no overflow
		elapse = curr - last;
	} else { // overflow
		elapse = curr + ((~last) & 0xFFFFFFFF);
	}

	return ((1000 / configTICK_RATE_HZ) * elapse);
}

struct btif_dma_t *btif_dma_info_get(unsigned char dir)
{
	if ((dir == DMA_DIR_RX) || (dir == DMA_DIR_TX))
		return &g_btif_dma[dir];

	BTIF_LOG_E("%s invalid DMA dir (%d)", __func__, dir);
	return NULL;
}

void btif_dma_rx_cb_reg(struct btif_dma_t *p_dma_info, dma_rx_data_cb rx_cb)
{
	if (p_dma_info->rx_cb != NULL) {
		BTIF_LOG_D("DMA rx_cb already registered, replace (0x%p) with (0x%p)",
				   p_dma_info->rx_cb, rx_cb);
	}
	p_dma_info->rx_cb = rx_cb;
}

void btif_dma_clk_ctrl(unsigned char en)
{
	btif_platform_apdma_clk_ctrl(en);
}

int btif_dma_tx_semaphore_init(struct btif_dma_t *p_dma_info)
{
	if (p_dma_info->dma_tx_semaphore == NULL)
		p_dma_info->dma_tx_semaphore = xSemaphoreCreateBinary();

	if (p_dma_info->dma_tx_semaphore != NULL) {
		xSemaphoreGive(p_dma_info->dma_tx_semaphore);
	} else {
		BTIF_LOG_E("%s: cannot create dma_tx_semaphore", __func__);
		return -1;
	}
	return 0;
}

void btif_dma_tx_semaphore_deinit(struct btif_dma_t *p_dma_info)
{
	if (p_dma_info->dma_tx_semaphore) {
		vSemaphoreDelete(p_dma_info->dma_tx_semaphore);
		p_dma_info->dma_tx_semaphore = NULL;
	}
}

int btif_dma_vfifo_init(struct btif_dma_t *p_dma_info)
{
	BTIF_LOG_FUNC();
	if (p_dma_info->p_vfifo == NULL) {
		// TODO: the memory shall be 8bite align
		// p_dma_info->p_vfifo = btif_util_malloc(p_dma_info->vfifo_size);
		if (p_dma_info->dir == DMA_DIR_TX) {
			p_dma_info->p_vfifo = btif_platform_apdma_get_vfifo(p_dma_info->dir);
			if (p_dma_info->p_vfifo == NULL) {
				BTIF_LOG_E("%s, L: %d get vfifo error", __func__, __LINE__);
				return -1;
			}
			memset(p_dma_info->p_vfifo, 0, BTIF_DMA_TX_VFF_SIZE);
			BTIF_LOG_D("BTIF TX dma vfifo: %p", p_dma_info->p_vfifo);
		} else {
			p_dma_info->p_vfifo = btif_platform_apdma_get_vfifo(p_dma_info->dir);
			if (p_dma_info->p_vfifo == NULL) {
				BTIF_LOG_E("%s, L: %d get vfifo error", __func__, __LINE__);
				return -1;
			}
			memset(p_dma_info->p_vfifo, 0, BTIF_DMA_RX_VFF_SIZE);
			BTIF_LOG_D("BTIF RX dma vfifo: %p", p_dma_info->p_vfifo);
		}
		if (!p_dma_info->p_vfifo) {
			BTIF_LOG_E("alloc [%s] dma vfifo fail",
					   p_dma_info->dir == DMA_DIR_TX ? "TX" : "RX");
			return -1;
		}
	} else {
		BTIF_LOG_W("BTIF %s dma vfifo is not NULL, maybe already allocated",
				   p_dma_info->dir == DMA_DIR_TX ? "TX" : "RX");
	}
	/*vFIFO reset*/
	p_dma_info->rpt = 0;
	p_dma_info->last_rpt_wrap = 0;
	p_dma_info->wpt = 0;
	p_dma_info->last_wpt_wrap = 0;

	return 0;
}

void btif_dma_vfifo_deinit(struct btif_dma_t *p_dma_info)
{
	BTIF_LOG_FUNC();
	if (p_dma_info->p_vfifo) {
		// btif_util_free(p_dma_info->p_vfifo);
		p_dma_info->p_vfifo = NULL;
	}
}

int btif_dma_irq_request(struct btif_dma_t *p_dma_info)
{
	unsigned int i_ret = 0;
	enum btif_dma_dir dir = p_dma_info->dir;

	if (dir == DMA_DIR_RX) {
		btif_platform_request_irq(p_dma_info->irq_id, btif_dma_rx_irq_handler,
								  "btif_dma_rx", p_dma_info->p_btif);
	} else if (dir == DMA_DIR_TX) {
		btif_platform_request_irq(p_dma_info->irq_id, btif_dma_tx_irq_handler,
								  "btif_dma_rx", p_dma_info->p_btif);
	} else {
		BTIF_LOG_E("invalid DMA dma dir (%d)", dir);
		i_ret = -1;
	}
	return i_ret;
}

int btif_dma_tx_setup(struct mtk_btif *p_btif)
{
	int i_ret = -1;
	struct btif_dma_t *p_dma_info = NULL;

	BTIF_LOG_FUNC();
	if (p_btif == NULL)
		return i_ret;
	p_dma_info = (struct btif_dma_t *)p_btif->p_tx_dma;
	p_dma_info->p_btif = p_btif;

	/*DMA clock is enabled by default*/
	btif_dma_clk_ctrl(TRUE);

	/*DMA controller setup*/
	btif_dma_hw_init(p_dma_info);

	/*DMA HW Enable*/
	i_ret = btif_dma_ctrl(p_dma_info, DMA_CTRL_ENABLE);
	if (!i_ret) {
		btif_tx_mode_ctrl(p_btif);
		/*DMA Tx IRQ register*/
		btif_dma_irq_request(p_dma_info);
		BTIF_LOG_D("%s success", __func__);
	} else {
		BTIF_LOG_E("%s failed, i_ret(%d)", __func__, i_ret);
		btif_dma_clk_ctrl(FALSE);
		return i_ret;
	}
	return i_ret;
}

int btif_dma_rx_setup(struct mtk_btif *p_btif)
{
	int i_ret = -1;
	struct btif_dma_t *p_dma_info = NULL;

	BTIF_LOG_FUNC();
	if (p_btif == NULL)
		return i_ret;
	p_dma_info = (struct btif_dma_t *)p_btif->p_rx_dma;
	p_dma_info->p_btif = p_btif;

	/*DMA clock is enabled by default*/
	btif_dma_clk_ctrl(TRUE);

	/*hardware init*/
	btif_dma_hw_init(p_dma_info);

	/*DMA controller enable*/
	i_ret = btif_dma_ctrl(p_dma_info, DMA_CTRL_ENABLE);
	if (!i_ret) {
		/*enable Rx DMA mode*/
		btif_rx_mode_ctrl(p_btif);
		/*DMA RX irq reg*/
		btif_dma_irq_request(p_dma_info);
		// btif_irq_ctrl(p_dma_info->irq_id, FALSE); //disable first
		BTIF_LOG_D("%s success", __func__);
	} else {
		BTIF_LOG_E("%s failed, i_ret(%d)", __func__, i_ret);
		btif_dma_clk_ctrl(FALSE);
		return i_ret;
	}
	return i_ret;
}

void btif_dma_free(struct btif_dma_t *p_dma_info)
{
	BTIF_LOG_FUNC();
	if (p_dma_info->dir == DMA_DIR_TX) {
		btif_platform_free_irq(p_dma_info->irq_id);
		btif_dma_tx_ctrl(p_dma_info, DMA_CTRL_DISABLE);
		// clock maybe disabled here TODO
		btif_dma_clk_ctrl(FALSE);
	} else if (p_dma_info->dir == DMA_DIR_RX) {
		btif_platform_free_irq(p_dma_info->irq_id);
		btif_dma_rx_ctrl(p_dma_info, DMA_CTRL_DISABLE);
		btif_dma_rx_cb_reg(p_dma_info, NULL);
		// clock maybe disabled here TODO
		btif_dma_clk_ctrl(FALSE);
	} else
		BTIF_LOG_E("%s, invalid DMA dir", __func__);
}

void btif_dma_tx_ier_ctrl(struct btif_dma_t *p_dma_info, unsigned char en)
{
	unsigned int base = (unsigned int)p_dma_info->p_base;

	if (!en)
		BTIF_CLR_BIT(TX_DMA_INT_EN(base), TX_DMA_INTEN_BIT);
	else
		BTIF_SET_BIT(TX_DMA_INT_EN(base), TX_DMA_INTEN_BIT);
	BTIF_LOG_V("%s, INT_EN = 0x%x, en = %d", __func__, BTIF_READ32(TX_DMA_INT_EN(base)), en);
}

void btif_dma_rx_ier_ctrl(struct btif_dma_t *p_dma_info, unsigned char en)
{
	unsigned int base = (unsigned int)p_dma_info->p_base;

	if (!en)
		BTIF_CLR_BIT(RX_DMA_INT_EN(base), (RX_DMA_INT_THRE_EN | RX_DMA_INT_DONE_EN));
	else
		BTIF_SET_BIT(RX_DMA_INT_EN(base), (RX_DMA_INT_THRE_EN | RX_DMA_INT_DONE_EN));
	BTIF_LOG_V("%s, INT_EN = 0x%x, en = %d", __func__, BTIF_READ32(RX_DMA_INT_EN(base)), en);
}

void btif_dma_ier_ctrl(struct btif_dma_t *p_dma_info, unsigned char en)
{
	enum btif_dma_dir dir = p_dma_info->dir;

	if (dir == DMA_DIR_RX)
		btif_dma_rx_ier_ctrl(p_dma_info, en);
	else if (dir == DMA_DIR_TX)
		btif_dma_tx_ier_ctrl(p_dma_info, en);
	else
		BTIF_LOG_E("%s invalid DMA dma dir (%d)", __func__, dir);
}

int btif_dma_tx_ctrl(struct btif_dma_t *p_dma_info, enum btif_dma_ctrl ctrl_id)
{
	unsigned int i_ret = -1;
	unsigned int base = (unsigned int)p_dma_info->p_base;
	unsigned int dat;

	BTIF_LOG_V("%s, id=%s", __func__, (ctrl_id != DMA_CTRL_DISABLE) ? "enable" : "disable");
	if (ctrl_id == DMA_CTRL_DISABLE) {
		/*if write 0 to EN bit, DMA will be stopped imediately*/
		/*if write 1 to STOP bit, DMA will be stopped after current*/
		/*transaction finished*/
		/*BTIF_CLR_BIT(TX_DMA_EN(base), DMA_EN_BIT);*/
		TickType_t old_tick = xTaskGetTickCount();
		TickType_t curr_tick = old_tick;

		do {
			if (time_elapse(old_tick, curr_tick) < BTIF_STOP_DMA_TIME_MS) {
				BTIF_SET_BIT(TX_DMA_STOP(base), DMA_STOP_BIT);
				dat = BTIF_READ32(TX_DMA_STOP(base));
			} else {
				BTIF_LOG_E("%s stop dma timeout!!!", __func__);
				break;
			}
			curr_tick = xTaskGetTickCount();
		} while (0x1 & dat);
		BTIF_LOG_V("TX DMA disabled, EN(0x%x),STOP(0x%x)",
					BTIF_READ32(TX_DMA_EN(base)),
					BTIF_READ32(TX_DMA_STOP(base)));
		i_ret = 0;
	} else if (ctrl_id == DMA_CTRL_ENABLE) {
		BTIF_SET_BIT(TX_DMA_EN(base), DMA_EN_BIT);
		// BTIF_LOG_W("BTIF Tx DMA enabled");
		i_ret = 0;
	} else {
		BTIF_LOG_E("invalid DMA ctrl_id (%d)", ctrl_id);
		i_ret = -1;
	}
	// BTIF_LOG_I("%s, end", __func__);
	return i_ret;
}

int btif_dma_rx_ctrl(struct btif_dma_t *p_dma_info, enum btif_dma_ctrl ctrl_id)
{
	unsigned int i_ret = -1;
	unsigned int base = (unsigned int)p_dma_info->p_base;
	unsigned int dat;

	BTIF_LOG_D("%s, id=%s", __func__, (ctrl_id != DMA_CTRL_DISABLE) ? "enable" : "disable");

	if (ctrl_id == DMA_CTRL_DISABLE) {
		/*if write 0 to EN bit, DMA will be stopped imediately*/
		/*if write 1 to STOP bit, DMA will be stopped after current*/
		/*transaction finished*/
		/*BTIF_CLR_BIT(RX_DMA_EN(base), DMA_EN_BIT);*/
		TickType_t old_tick = xTaskGetTickCount();
		TickType_t curr_tick = old_tick;

		do {
			if (time_elapse(old_tick, curr_tick) < BTIF_STOP_DMA_TIME_MS) {
				BTIF_SET_BIT(RX_DMA_STOP(base), DMA_STOP_BIT);
				dat = BTIF_READ32(RX_DMA_STOP(base));
			} else {
				BTIF_LOG_E("%s stop dma timeout!!!", __func__);
				break;
			}
			curr_tick = xTaskGetTickCount();
		} while (0x1 & dat);
		BTIF_LOG_V("RX DMA disabled,EN(0x%x),STOP(0x%x)",
				   BTIF_READ32(RX_DMA_EN(base)),
				   BTIF_READ32(RX_DMA_STOP(base)));
		i_ret = 0;
	} else if (ctrl_id == DMA_CTRL_ENABLE) {
		BTIF_SET_BIT(RX_DMA_EN(base), DMA_EN_BIT);
		// BTIF_LOG_W("BTIF RX DMA enabled");
		i_ret = 0;
	} else {
		BTIF_LOG_E("invalid DMA ctrl_id (%d)", ctrl_id);
		i_ret = -1;
	}

	return i_ret;
}

int btif_dma_ctrl(struct btif_dma_t *p_dma_info, enum btif_dma_ctrl ctrl_id)
{
	unsigned int i_ret = 0;
	enum btif_dma_dir dir = p_dma_info->dir;

	if (dir == DMA_DIR_RX)
		i_ret = btif_dma_rx_ctrl(p_dma_info, ctrl_id);
	else if (dir == DMA_DIR_TX)
		i_ret = btif_dma_tx_ctrl(p_dma_info, ctrl_id);
	else {
		BTIF_LOG_E("%s invalid dma ctrl id (%d)", __func__, ctrl_id);
		i_ret = -1;
	}
	return i_ret;
}

void btif_dma_ctrl_enable(struct btif_dma_t *p_dma_info)
{
	enum btif_dma_dir dir = p_dma_info->dir;
	unsigned int base = 0;

	if (p_dma_info->p_base == NULL) {
		BTIF_LOG_E("%s, p_base is NULL", __func__);
		return;
	}

	base = (unsigned int)p_dma_info->p_base;
	if (dir == DMA_DIR_RX)
		BTIF_SET_BIT(RX_DMA_EN(base), DMA_EN_BIT);
	else
		BTIF_SET_BIT(TX_DMA_EN(base), DMA_EN_BIT);
}

void btif_dma_ctrl_disable(struct btif_dma_t *p_dma_info)
{
	enum btif_dma_dir dir = p_dma_info->dir;
	unsigned int base = 0;

	if (p_dma_info->p_base == NULL) {
		BTIF_LOG_E("%s, p_base is NULL", __func__);
		return;
	}

	/* Write STOP bit as 1 to stop APDMA*/
	base = (unsigned int)p_dma_info->p_base;
	if (dir == DMA_DIR_RX)
		BTIF_SET_BIT(RX_DMA_STOP(base), DMA_STOP_BIT);
	else
		BTIF_SET_BIT(TX_DMA_STOP(base), DMA_STOP_BIT);

	/* Polling EN bit after write STOP bit as 1 */
#ifdef HAL_GPT_MODULE_ENABLED
/*
 * To prevent different time consumption for different platform,
 * we use execution time for execution boundary instead of count of read.
 * For suspend procodure, platform still have 2ms can be consume,
 * we use 500us for exception case.
 */
#define APDMA_EN_POLLING_TIMEOUT  0 /*unit:us*/
	uint32_t start = 0, dur = 0, now = 0, count = 0;
	unsigned int en = 0;

	hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &start);
	do {
		if (dir == DMA_DIR_RX)
			en = BTIF_READ32(RX_DMA_EN(base));
		else
			en = BTIF_READ32(TX_DMA_EN(base));
		hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &now);
		hal_gpt_get_duration_count(start, now, &dur);

		count++;
	} while (en & DMA_EN_BIT && dur <= APDMA_EN_POLLING_TIMEOUT);

	/* If EN bit not 0, regard it as error */
	if (en & DMA_EN_BIT) {
		BTIF_LOG_E("%s, APDMA STOP FAIL! Time %d Count %d", __func__, dur, count);
		/* Not assert in MP branch */
		assert((en & DMA_EN_BIT) == 0);
	}
#else
	BTIF_LOG_E("%s, Must enable HAL_GPT_MODULE to make the protion mechanism work!", __func__);
	__asm volatile("udf #255");
#endif
}

static int _btif_dma_tx_flush(struct btif_dma_t *p_dma_info)
{
	unsigned int i_ret = -1;
	unsigned int base = (unsigned int)p_dma_info->p_base;
	unsigned int stop = BTIF_READ32(TX_DMA_STOP(base));

	/* in MTK DMA BTIF channel we cannot set STOP and FLUSH bit at the same time */
	if ((stop && DMA_STOP_BIT) != 0)
		BTIF_LOG_E("DMA in stop state, omit flush operation");
	else {
		BTIF_LOG_V("flush tx dma");
		BTIF_SET_BIT(TX_DMA_FLUSH(base), DMA_FLUSH_BIT);
		i_ret = 0;
	}
	return i_ret;
}

void btif_dma_tx_irq_handler(int irq, void *dev)
{
#define MAX_CONTINUOUS_TIMES 512
	struct btif_dma_t *p_dma_info = (struct btif_dma_t *)((struct mtk_btif *)dev)->p_tx_dma;

	unsigned int valid_size = 0;
	unsigned int vff_len = 0;
	unsigned int left_len = 0;
	unsigned int base = (unsigned int)p_dma_info->p_base;
	unsigned int wpt = 0;
	unsigned int rpt = 0;
	static int flush_irq_counter;
	TickType_t start_tick = 0;
	TickType_t end_tick = 0;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BaseType_t xResult = 0;

	btif_irq_ctrl(p_dma_info->irq_id, FALSE);

	UBaseType_t savedIrqStatus = taskENTER_CRITICAL_FROM_ISR();

	/*check if Tx VFF Left Size equal to VFIFO size or not*/
	vff_len = BTIF_READ32(TX_DMA_VFF_LEN(base));
	valid_size = BTIF_READ32(TX_DMA_VFF_VALID_SIZE(base));
	left_len = BTIF_READ32(TX_DMA_VFF_LEFT_SIZE(base));
	rpt = BTIF_READ32(TX_DMA_VFF_RPT(base));
	wpt = BTIF_READ32(TX_DMA_VFF_WPT(base));
	BTIF_LOG_V("%s, int_fg:0x%08x, vff_len:%d, valid_len:%d, left_len:%d, wpt:0x%08x, rpt:0x%08x",
				__func__, BTIF_READ32(TX_DMA_INT_FLAG(base)), vff_len, valid_size, left_len, wpt, rpt);
#ifdef BUFFER_DEBUG_TRACE
	uint8_t own_now = btmtk_get_own_type();

	btif_tx_irq_trace_write(vff_len, valid_size, left_len, wpt, rpt, own_now);
	if (!own_now)
		BTIF_LOG_I("%s, own_now = %d, own = %d", __func__, own_now, btmtk_get_local_own_type());
#endif
	if (flush_irq_counter == 0)
		start_tick = xTaskGetTickCount();
	if ((valid_size > 0) && (valid_size < 8)) {
		_btif_dma_tx_flush(p_dma_info);
		flush_irq_counter++;
		if (flush_irq_counter >= MAX_CONTINUOUS_TIMES) {
			end_tick = xTaskGetTickCount();
			/*
			 * When btif tx fifo cannot accept any data and counts of bytes left
			 * in tx vfifo < 8 for a while,
			 * we assume that btif cannot send data for a long time
			 * In order not to generate interrupt continuously
			 * which may effect system's performance,
			 * we clear tx flag and disable btif tx interrupt
			 */
			/* clear interrupt flag */
			BTIF_CLR_BIT(TX_DMA_INT_FLAG(base), TX_DMA_INT_FLAG_MASK);
			/* vFIFO data has been read by DMA controller, just disable tx dma's irq */
			btif_dma_ier_ctrl(p_dma_info, FALSE);
			BTIF_LOG_E("**** ERROR: tx timeout ****");
			BTIF_LOG_E("Tx max time: %dms, %d to %d tick, time_elapse: %dms",
					   MAX_CONTINUOUS_TIMES, (unsigned int)start_tick,
					   (unsigned int)end_tick, time_elapse(start_tick, end_tick));
			// use UNUSED macro for debug warnig
			UNUSED(start_tick);
			UNUSED(end_tick);
		}
	} else if (vff_len == left_len) {
		flush_irq_counter = 0;
		// tx done, clear flag
		btif_set_tx_status(TX_IDLE);

		if (((struct mtk_btif *)dev)->main_task_event_group &&
			((struct mtk_btif *)dev)->main_task_hdl) {
			xResult = xEventGroupSetBitsFromISR(((struct mtk_btif *)dev)->main_task_event_group,
												BTIF_TASK_EVENT_TX_DONE, &xHigherPriorityTaskWoken);
			if (xResult != pdFAIL) {
				// Do not need error handle, because even it return fail,
				// we still need to finish the remain procedure
				// BTIF_LOG_V("%s: set_bit return fail!", __func__);
			}
		} else {
			// Print error message
			BTIF_LOG_E("%s: main_task_event_group or main_task did not created", __func__);
		}

		/*clear interrupt flag*/
		BTIF_CLR_BIT(TX_DMA_INT_FLAG(base), TX_DMA_INT_FLAG_MASK);
		// vFIFO data has been read by DMA controller, just disable tx dma's irq
		btif_dma_ier_ctrl(p_dma_info, FALSE);

		// Give Semaphore
		if (p_dma_info->dma_tx_semaphore) {
			BTIF_LOG_V("%S: Give dma_tx_semaphore!", __func__);
			xSemaphoreGiveFromISR(p_dma_info->dma_tx_semaphore, &xHigherPriorityTaskWoken);
		}
	} else {
		BTIF_LOG_D("superious IRQ: vff_len(%d),valid_size(%d),left_len(%d)",
					vff_len, valid_size, left_len);
	}
	taskEXIT_CRITICAL_FROM_ISR(savedIrqStatus);

	btif_irq_ctrl(p_dma_info->irq_id, TRUE);
	if (xHigherPriorityTaskWoken == pdTRUE)
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void btif_dma_rx_irq_handler(int irq, void *dev)
{
	int i_ret = 0;
	int notify = TRUE;
	unsigned int valid_len = 0;
	unsigned int wpt_wrap = 0;
	unsigned int rpt_wrap = 0;
	unsigned int wpt = 0;
	unsigned int rpt = 0;
	unsigned int tail_len = 0;
	unsigned int real_len = 0;
	struct btif_dma_t *p_dma_info = (struct btif_dma_t *)((struct mtk_btif *)dev)->p_rx_dma;
	unsigned int base = (unsigned int)p_dma_info->p_base;
	dma_rx_data_cb rx_cb = p_dma_info->rx_cb;
	unsigned char *p_vff_buf = NULL;
	unsigned char *vff_base = p_dma_info->p_vfifo;
	unsigned int vff_size = p_dma_info->vfifo_size;

	btif_irq_ctrl(p_dma_info->irq_id, FALSE);

	/*disable DMA Rx IER*/
	btif_dma_ier_ctrl(p_dma_info, FALSE);

	UBaseType_t savedIrqStatus = taskENTER_CRITICAL_FROM_ISR();
#ifdef BTIF_MAIN_TASK_TRACE
	btif_main_task_record_write(pdTRUE, RECORD_SYMBOL_INVALID, RECORD_SYMBOL_RX_IRQ, NULL, 0);
#endif
	btif_platform_dcache_clean();

	/*clear Rx DMA's interrupt status*/
	BTIF_SET_BIT(RX_DMA_INT_FLAG(base), RX_DMA_INT_DONE | RX_DMA_INT_THRE);

	valid_len = BTIF_READ32(RX_DMA_VFF_VALID_SIZE(base));
	rpt = BTIF_READ32(RX_DMA_VFF_RPT(base));
	wpt = BTIF_READ32(RX_DMA_VFF_WPT(base));
	if ((valid_len == 0) && (rpt == wpt)) {
		BTIF_LOG_V("no data in DMA, wpt(0x%08x), rpt(0x%08x), flg(0x%x)",
				   rpt, wpt, BTIF_READ32(RX_DMA_INT_FLAG(base)));
		notify = FALSE;
	} else {
		BTIF_LOG_V("%s, int_flag:0x%08x, valid_len:%d, rpt:0x%08x, wpt:0x%08x",
				   __func__, BTIF_READ32(RX_DMA_INT_FLAG(base)), valid_len, rpt, wpt);
	}

	while ((valid_len > 0) || (rpt != wpt)) {
		rpt_wrap = rpt & DMA_RPT_WRAP;
		wpt_wrap = wpt & DMA_WPT_WRAP;
		rpt &= DMA_RPT_MASK;
		wpt &= DMA_WPT_MASK;

		/*calcaute length of available data  in vFIFO*/
		if (wpt_wrap != p_dma_info->last_wpt_wrap)
			real_len = wpt + vff_size - rpt;
		else
			real_len = wpt - rpt;

		if (rx_cb != NULL) {
			tail_len = vff_size - rpt;
			p_vff_buf = vff_base + rpt;
			if (tail_len >= real_len) {
				(*rx_cb)(p_vff_buf, real_len);
			} else {
				(*rx_cb)(p_vff_buf, tail_len);
				p_vff_buf = vff_base;
				(*rx_cb)(p_vff_buf, real_len - tail_len);
			}
			i_ret += real_len;
		} else
			BTIF_LOG_E("no rx_cb found, check init process");
		// mb(); /* for dma irq */
		rpt += real_len;
		if (rpt >= vff_size) {
			/* read wrap bit should be revert */
			rpt_wrap ^= DMA_RPT_WRAP;
			rpt %= vff_size;
		}
		rpt |= rpt_wrap;
		/* record wpt, last_wpt_wrap, rpt, last_rpt_wrap */
		p_dma_info->wpt = wpt;
		p_dma_info->last_wpt_wrap = wpt_wrap;

		p_dma_info->rpt = rpt;
		p_dma_info->last_rpt_wrap = rpt_wrap;

		/* update rpt information to DMA controller */
		BTIF_WRITE32(RX_DMA_VFF_RPT(base), rpt);

		/* get vff valid size again and check if rx data is processed completely */
		valid_len = BTIF_READ32(RX_DMA_VFF_VALID_SIZE(base));

		rpt = BTIF_READ32(RX_DMA_VFF_RPT(base));
		wpt = BTIF_READ32(RX_DMA_VFF_WPT(base));
		BTIF_LOG_V("%s, after, valid_len:%d, rpt:%d, wpt:%d", __func__, valid_len, rpt, wpt);
	}

	taskEXIT_CRITICAL_FROM_ISR(savedIrqStatus);

	/* enable DMA Rx IER */
	btif_dma_ier_ctrl(p_dma_info, TRUE);
	btif_irq_ctrl(p_dma_info->irq_id, TRUE);

	if (notify)
		btif_consummer_nty();
}

int btif_dma_hw_init(struct btif_dma_t *p_dma_info)
{
	int i_ret = 0;
	unsigned int dat = 0;
	unsigned int base = (unsigned int)p_dma_info->p_base;

	BTIF_LOG_D("%s, L %d", __func__, __LINE__);

	if (p_dma_info->dir == DMA_DIR_RX) {
		/* do hardware reset */
		BTIF_SET_BIT(RX_DMA_RST(base), DMA_WARM_RST);
		BTIF_LOG_D("%s, RX DMA_WARM_RST start", __func__);
		do {
			dat = BTIF_READ32(RX_DMA_EN(base));
		} while (0x01 & dat);
		BTIF_LOG_D("%s, RX DMA_WARM_RST done", __func__);
		/* write vfifo base address to VFF_ADDR */
		BTIF_WRITE32(RX_DMA_VFF_ADDR(base), p_dma_info->p_vfifo);
		BTIF_WRITE32(RX_DMA_VFF_ADDR_H(base), 0);
		/* write vfifo length to VFF_LEN */
		BTIF_WRITE32(RX_DMA_VFF_LEN(base), p_dma_info->vfifo_size);
		/* write wpt to VFF_WPT */
		BTIF_WRITE32(RX_DMA_VFF_WPT(base), p_dma_info->wpt);
		/* write rpt to VFF_RPT */
		BTIF_WRITE32(RX_DMA_VFF_RPT(base), p_dma_info->rpt);
		/* write vff_thre to VFF_THRESHOLD */
		BTIF_WRITE32(RX_DMA_VFF_THRE(base), p_dma_info->thre);
		/* clear Rx DMA's interrupt status */
		BTIF_SET_BIT(RX_DMA_INT_FLAG(base), RX_DMA_INT_DONE | RX_DMA_INT_THRE);
		/* enable Rx IER by default */
		btif_dma_ier_ctrl(p_dma_info, TRUE);
	} else { // TX
		/* do hardware reset */
		BTIF_SET_BIT(TX_DMA_RST(base), DMA_WARM_RST);
		BTIF_LOG_D("%s, TX DMA_WARM_RST start", __func__);
		do {
			dat = BTIF_READ32(TX_DMA_EN(base));
		} while (0x01 & dat);
		BTIF_LOG_D("%s, TX DMA_WARM_RST done", __func__);
		/* write vfifo base address to VFF_ADDR */
		BTIF_WRITE32(TX_DMA_VFF_ADDR(base), p_dma_info->p_vfifo);
		BTIF_WRITE32(TX_DMA_VFF_ADDR_H(base), 0);
		/* write vfifo length to VFF_LEN */
		BTIF_WRITE32(TX_DMA_VFF_LEN(base), p_dma_info->vfifo_size);
		/* write wpt to VFF_WPT */
		BTIF_WRITE32(TX_DMA_VFF_WPT(base), p_dma_info->wpt);
		BTIF_WRITE32(TX_DMA_VFF_RPT(base), p_dma_info->rpt);
		/* write vff_thre to VFF_THRESHOLD */
		BTIF_WRITE32(TX_DMA_VFF_THRE(base), p_dma_info->thre);

		BTIF_CLR_BIT(TX_DMA_INT_FLAG(base), TX_DMA_INT_FLAG_MASK);

		btif_dma_ier_ctrl(p_dma_info, FALSE);
	}

	return i_ret;
}

static int _is_tx_dma_in_flush(struct btif_dma_t *p_dma_info)
{
	unsigned char b_ret = TRUE;
	unsigned int base = (unsigned int)p_dma_info->p_base;

	/*see if flush operation is in process*/
	b_ret = ((DMA_FLUSH_BIT & BTIF_READ32(TX_DMA_FLUSH(base))) != 0) ? TRUE : FALSE;

	return b_ret;
}

static unsigned char btif_dma_is_tx_allow(struct btif_dma_t *p_dma_info)
{
	unsigned char b_ret = FALSE;

	/* see if flush operation is in process */
	b_ret = _is_tx_dma_in_flush(p_dma_info) ? FALSE : TRUE;
	if (!b_ret) { // recheck it ater 1 tick
		btif_util_task_delay_ticks(1);
		b_ret = _is_tx_dma_in_flush(p_dma_info) ? FALSE : TRUE;
	}
	if (!b_ret)
		BTIF_LOG_W("btif tx dma is not allowed");
	/* after Tx flush operation finished, HW will set DMA_EN back to 0 and stop DMA */
	BTIF_LOG_V("%s: %d", __func__, b_ret);
	return b_ret;
}

static unsigned int btif_dma_get_ava_room(struct btif_dma_t *p_dma_info)
{
	unsigned int ret;
	unsigned int base = (unsigned int)p_dma_info->p_base;

	/* read vFIFO's left size */
	ret = BTIF_READ32(TX_DMA_VFF_LEFT_SIZE(base));
	BTIF_LOG_V("DMA tx ava room (%u).", ret);
	if (ret == 0)
		BTIF_LOG_W("DMA tx vfifo is full.");

	return ret;
}

int _btif_dma_write(struct btif_dma_t *p_dma_info, const unsigned char *p_buf,
					const unsigned int buf_len)
{
	unsigned int i_ret = 0;
	unsigned int base = (unsigned int)p_dma_info->p_base;
	unsigned int len_to_send = 0;
	unsigned int ava_len = 0;
	unsigned int wpt = 0;
	unsigned int last_wpt_wrap = 0;
	unsigned int vff_size = 0;
	unsigned char *p_data = (unsigned char *)p_buf;

	if ((p_buf == NULL) || (buf_len == 0)) {
		BTIF_LOG_E("invalid parameters, p_buf:0x%p, buf_len:%d", p_buf, buf_len);
		return -1;
	}
	BTIF_LOG_V("%s buf_len = %d", __func__, buf_len);

	/* check if tx dma in flush operation?
	 * if yes, should wait until DMA finish flush operation
	 * currently uplayer logic will make sure this pre-condition
	 * disable Tx IER, in case Tx irq happens, flush bit may be set in irq handler
	 */
	btif_dma_tx_ier_ctrl(p_dma_info, FALSE);

	vff_size = p_dma_info->vfifo_size;
	ava_len = BTIF_READ32(TX_DMA_VFF_LEFT_SIZE(base));
	wpt = BTIF_READ32(TX_DMA_VFF_WPT(base)) & DMA_WPT_MASK;
	last_wpt_wrap = BTIF_READ32(TX_DMA_VFF_WPT(base)) & DMA_WPT_WRAP;
#ifdef BUFFER_DEBUG_TRACE
	btif_tx_vfifo_trace_write(p_buf, buf_len, ava_len, wpt, last_wpt_wrap);
#endif
	/*
	 * copy data to vFIFO, Note: ava_len should always large than buf_len,
	 * otherwise common logic layer will not call _btif_dma_write
	 */
	if (buf_len > ava_len) {
		BTIF_LOG_E("length to send:(%d) < length available(%d), abnormal!",
					buf_len, ava_len);
		// WARN_ON(buf_len > ava_len); // this will cause kernel panic
	}

	BTIF_LOG_V("%s vfifo_size:%d, buf_len:%d, ava_len:%d, wpt:%d", __func__,
				vff_size, buf_len, ava_len, wpt);

	len_to_send = buf_len < ava_len ? buf_len : ava_len;
	if (len_to_send + wpt >= vff_size) {
		unsigned int tail_len = vff_size - wpt;

		memcpy((p_dma_info->p_vfifo + wpt), p_data, tail_len);
		p_data += tail_len;
		memcpy(p_dma_info->p_vfifo, p_data, len_to_send - tail_len);
		/*make sure all data write to memory area tx vfifo locates*/
		// mb(); //TODO
		btif_platform_dcache_clean();

		/*calculate WPT*/
		wpt = wpt + len_to_send - vff_size;
		last_wpt_wrap ^= DMA_WPT_WRAP;
	} else {
		memcpy((p_dma_info->p_vfifo + wpt), p_data, len_to_send);
		/*make sure all data write to memory area tx vfifo locates*/
		// mb();
		btif_platform_dcache_clean();

		/*calculate WPT*/
		wpt += len_to_send;
	}
	p_dma_info->wpt = wpt;
	p_dma_info->last_wpt_wrap = last_wpt_wrap;

	/* make sure tx dma is allowed(tx flush bit not set) to use before update WPT */
	if (btif_dma_is_tx_allow(p_dma_info)) {
		/*make sure tx dma enabled*/
		btif_dma_ctrl(p_dma_info, DMA_CTRL_ENABLE);

		/*update WTP to Tx DMA controller's control register*/
		BTIF_WRITE32(TX_DMA_VFF_WPT(base), wpt | last_wpt_wrap);
		BTIF_LOG_V("TX_DMA_VFF_VALID_SIZE = %d", BTIF_READ32(TX_DMA_VFF_VALID_SIZE(base)));
		if ((BTIF_READ32(TX_DMA_VFF_VALID_SIZE(base)) < 8) &&
			(BTIF_READ32(TX_DMA_VFF_VALID_SIZE(base)) > 0)) {
			/*
			 * 0 < valid size in Tx vFIFO < 8 && TX Flush is not in
			 * process<should always be done>?
			 * if yes, set flush bit to DMA
			 */
			_btif_dma_tx_flush(p_dma_info);
		}
		i_ret = len_to_send;
	} else {
		/*TODO: print error log*/
		BTIF_LOG_E("Tx DMA flush operation is in process,%s%s%s",
				   " this case should never happen,",
				   " please check if tx operation",
				   " is allowed before call this API");
		/*if flush operation is in process , we will return 0*/
		i_ret = 0;
	}

	/*Enable Tx IER*/
	btif_dma_tx_ier_ctrl(p_dma_info, TRUE);
	return i_ret;
}

int btif_dma_tx_write(struct btif_dma_t *p_dma_info, const unsigned char *p_buf,
					  unsigned int buf_len)
{
	unsigned int i_ret = 0;
	unsigned int retry = 0;
	// For FreeRTOS, taskdelay is controlled by tick, wait 5 ticks at most (5ms)
	unsigned int max_tx_retry = 5;
	int ava_room = 0;

	btif_util_dump_buffer("DMA TX", p_buf, buf_len, 0);

	btif_irq_ctrl(p_dma_info->irq_id, FALSE);
	do {
		/*wait until tx is allowed*/
		while ((!btif_dma_is_tx_allow(p_dma_info)) && (retry < max_tx_retry)) {
			retry++;
			if (retry >= max_tx_retry) {
				BTIF_LOG_E("wait for tx allowed timeout");
				break;
			}
		}
		if (retry >= max_tx_retry)
			break;

		ava_room = btif_dma_get_ava_room(p_dma_info);
		if (ava_room > 0 && buf_len <= (unsigned int)ava_room)
			i_ret = _btif_dma_write(p_dma_info, p_buf, buf_len);
		else
			i_ret = 0;
	} while (0);

	btif_irq_ctrl(p_dma_info->irq_id, TRUE);

	if (i_ret == 0 && p_dma_info->dma_tx_semaphore) {
		taskENTER_CRITICAL();
		btif_set_tx_status(TX_IDLE);
		taskEXIT_CRITICAL();

		// Write error, Give Semaphore
		BTIF_LOG_E("%s: Write FAIL! Give dma_tx_semaphore!", __func__);
		xSemaphoreGive(p_dma_info->dma_tx_semaphore);
	}
	return i_ret;
}

static void _btif_dma_tx_dump_reg(struct btif_dma_t *p_dma_info)
{
	unsigned int base = (unsigned int)p_dma_info->p_base;
	unsigned int enable = 0;
	unsigned int stop = 0;
	unsigned int wpt = 0;
	unsigned int rpt = 0;
	unsigned int int_buf = 0;

	BTIF_LOG_I("dump TX DMA status regs:");

	enable = BTIF_READ32(TX_DMA_EN(base));
	stop = BTIF_READ32(TX_DMA_STOP(base));
	wpt = BTIF_READ32(TX_DMA_VFF_WPT(base));
	rpt = BTIF_READ32(TX_DMA_VFF_RPT(base));
	int_buf = BTIF_READ32(TX_DMA_INT_BUF_SIZE(base));

	BTIF_LOG_I("EN: 0x%08x, INT_FLAG: 0x%08x, STOP: 0x%08x, FLUSH: 0x%08x",
				enable, BTIF_READ32(TX_DMA_INT_FLAG(base)), stop, BTIF_READ32(TX_DMA_FLUSH(base)));
	BTIF_LOG_I("WPT: 0x%08x, RPT: 0x%08x, INT_EN: 0x%08x, RST: 0x%08x",
				wpt, rpt, BTIF_READ32(TX_DMA_INT_EN(base)), BTIF_READ32(TX_DMA_RST(base)));
	BTIF_LOG_I("INT_BUF: 0x%08x, VF_SZ: 0x%08x, VF_ADDR: 0x%08x, VF_LEN: 0x%08x",
				int_buf, BTIF_READ32(TX_DMA_VFF_VALID_SIZE(base)),
				BTIF_READ32(TX_DMA_VFF_ADDR(base)), BTIF_READ32(TX_DMA_VFF_LEN(base)));
	BTIF_LOG_I("VF_THRE: 0x%08x, DBG_ST: 0x%08x, VF_LEFT_SZ: 0x%08x, W_INT_BUFSZ: 0x%08x",
				BTIF_READ32(TX_DMA_VFF_THRE(base)), BTIF_READ32(TX_DMA_DEBUG_STATUS(base)),
				BTIF_READ32(TX_DMA_VFF_LEFT_SIZE(base)), BTIF_READ32(TX_DMA_W_INT_BUF_SIZE(base)));
	BTIF_LOG_I("VF_ADDR_H: 0x%08x, VF_WPT: 0x%08x, FLUSH_ACT: 0x%08x, VF_WPT2: 0x%08x",
				BTIF_READ32(TX_DMA_VFF_ADDR_H(base)), BTIF_READ32(TX_DMA_VFF_WPT_VALID(base)),
				BTIF_READ32(TX_DMA_FLUSH_ACT(base)), BTIF_READ32(TX_DMA_VFF_WPT_VALID2(base)));
	BTIF_LOG_I("HW_FLUSH: 0x%08x, VF_WPT_RL: 0x%08x, SEC_EN: 0x%08x",
				BTIF_READ32(TX_DMA_HW_FLUSH(base)), BTIF_READ32(TX_DMA_VFF_WPT_REAL(base)), BTIF_READ32(TX_DMA_SEC_EN(base)));
	BTIF_LOG_I("TX dma %s,data is %s sent by HW",
				(enable & DMA_EN_BIT) && (!(stop & DMA_STOP_BIT)) ? "enabled" : "stopped",
				((wpt == rpt) && (int_buf == 0)) ? "completely" : "not completely");
	// use UNUSED macro for debug warnig
	UNUSED(base);
	UNUSED(enable);
	UNUSED(stop);
	UNUSED(wpt);
	UNUSED(rpt);
	UNUSED(int_buf);
}

static void _btif_dma_rx_dump_reg(struct btif_dma_t *p_dma_info)
{
	unsigned int base = (unsigned int)p_dma_info->p_base;
	unsigned int enable = 0;
	unsigned int stop = 0;
	unsigned int wpt = 0;
	unsigned int rpt = 0;
	unsigned int int_buf = 0;

	BTIF_LOG_I("dump RX DMA status regs:");

	enable = BTIF_READ32(RX_DMA_EN(base));
	stop = BTIF_READ32(RX_DMA_STOP(base));
	wpt = BTIF_READ32(RX_DMA_VFF_WPT(base));
	rpt = BTIF_READ32(RX_DMA_VFF_RPT(base));
	int_buf = BTIF_READ32(RX_DMA_INT_BUF_SIZE(base));

	BTIF_LOG_I("EN: 0x%08x, STOP: 0x%08x, WPT: 0x%08x, RPT: 0x%08x", enable, stop, wpt, rpt);
	BTIF_LOG_I("INT_EN: 0x%08x, RST: 0x%08x, FLUSH: 0x%08x, INT_FLAG: 0x%08x",
				BTIF_READ32(RX_DMA_INT_EN(base)), BTIF_READ32(RX_DMA_RST(base)),
				BTIF_READ32(RX_DMA_FLUSH(base)), BTIF_READ32(RX_DMA_INT_FLAG(base)));
	BTIF_LOG_I("INT_BUF: 0x%08x, VF_SZ: 0x%08x, VF_ADDR: 0x%08x, VF_LEN: 0x%08x",
			 	int_buf, BTIF_READ32(RX_DMA_VFF_VALID_SIZE(base)),
			 	BTIF_READ32(RX_DMA_VFF_ADDR(base)), BTIF_READ32(RX_DMA_VFF_LEN(base)));
	BTIF_LOG_I("VF_LEFT_SZ: 0x%08x, DBG_ST: 0x%08x, VF_THRE: 0x%08x, FLOW_CTRL: 0x%08x",
				BTIF_READ32(RX_DMA_VFF_LEFT_SIZE(base)), BTIF_READ32(RX_DMA_DEBUG_STATUS(base)),
				BTIF_READ32(RX_DMA_VFF_THRE(base)), BTIF_READ32(RX_DMA_FLOW_CTRL_THRE(base)));
	BTIF_LOG_I("SEC_EN: 0x%08x", BTIF_READ32(RX_DMA_SEC_EN(base)));
	BTIF_LOG_I("RX dma %s, data is %s by drv",
				(enable & DMA_EN_BIT) && (!(stop & DMA_STOP_BIT)) ? "enabled" : "stopped",
				((wpt == rpt) && (int_buf == 0)) ? "received" : "not received");
	// use UNUSED macro for debug warnig
	UNUSED(base);
	UNUSED(enable);
	UNUSED(stop);
	UNUSED(wpt);
	UNUSED(rpt);
	UNUSED(int_buf);
}

void btif_dma_dump_reg(struct btif_dma_t *p_dma_info)
{
	if (p_dma_info->dir == DMA_DIR_TX)
		_btif_dma_tx_dump_reg(p_dma_info);
	else if (p_dma_info->dir == DMA_DIR_RX)
		_btif_dma_rx_dump_reg(p_dma_info);
	else
		BTIF_LOG_W("unknown dir:%d\n", p_dma_info->dir);
}

bool btif_dma_check_RX_empty(void)
{
	if (BTIF_READ32(RX_DMA_INT_BUF_SIZE(BTIF_APDMA_RX_BASE))) {
		BTIF_LOG_W("%s: APDMA RX data not empty", __func__);
		return pdFALSE;
	}

	return pdTRUE;
}

/*---------------------------------------------------------------------------*/
