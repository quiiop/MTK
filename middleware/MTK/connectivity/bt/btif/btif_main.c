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
#include "btif_main.h"
#include "btif_dma.h"
#include "btif_util.h"
#include "btif_mt7933.h"
#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"
#endif

#define BTIF_TX_MODE BTIF_MODE_DMA
#define BTIF_RX_MODE BTIF_MODE_DMA

static bool g_btif_rx_data_lose = pdFALSE;
static uint16_t g_buf_high_cnt;
static uint8_t main_task_timeout = BTIF_TASK_EVENT_TIMEOUT;
#ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET
static xSemaphoreHandle main_task_timeout_mtx;
#endif /* #ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET */

static unsigned char _btif_rx_buf[BTIF_RX_BUFFER_SIZE] = {0};
// static btif_rx_data_ready_cb btif_rx_data_ready_nty = NULL;

#ifdef BTIF_MAIN_TASK_TRACE
struct btif_main_task_record *main_task_record;
#endif

#ifdef BUFFER_DEBUG_TRACE
#define MAX_BUF_DEBUG_COUNT 0xFFFFFFFF
static struct btif_buf_debug_trace *btif_buf_record;
static unsigned int buf_debug_count = 1;
static unsigned int g_buf_min_free_size = BTIF_RX_BUFFER_SIZE;
#endif

extern bool bt_driver_is_on(void);
extern int btmtk_trigger_fw_assert(void);

static int _btif_data_receiver(unsigned char *p_buf, unsigned int buf_len);

struct mtk_btif g_btif = {
	.tx_mode = BTIF_TX_MODE,
	.rx_mode = BTIF_RX_MODE,
	.p_base = (unsigned char *)BTIF_BASE,
	.p_tx_dma = NULL,
	.p_rx_dma = NULL,
	.tx_status = TX_IDLE,
	.rx_cb = _btif_data_receiver,
	.ownship_semaphore = NULL,
	.main_task_event_group = NULL,
	.main_task_hdl = NULL,
	.enabled = FALSE,
	.lpbk_flag = FALSE,
	.irq_id = BTIF_IRQ_ID,
	.tx_fifo_size = BTIF_TX_FIFO_SIZE, /* BTIF tx FIFO size in FIFO mode */
	.rx_fifo_size = BTIF_RX_FIFO_SIZE, /* BTIF rx FIFO size in FIFO mode */
	.tx_tri_lvl = BTIF_TX_FIFO_THRE,   /* BTIF tx trigger level in FIFO mode */
	.rx_tri_lvl = BTIF_RX_FIFO_THRE,   /* BTIF rx trigger level in FIFO mode */
};

static unsigned char _is_btif_enabled(void)
{
	return g_btif.enabled;
}

void btif_set_tx_status(enum btif_tx_status_t status)
{
	g_btif.tx_status = status;
}

static bool btif_is_tx_idle(void)
{
	enum btif_tx_status_t status;

	taskENTER_CRITICAL();
	status = g_btif.tx_status;
	taskEXIT_CRITICAL();

	if (status == TX_IDLE)
		return pdTRUE;

	return pdFALSE;
}

#ifdef BTIF_MAIN_TASK_TRACE
void btif_main_task_record_init(void)
{
	if (main_task_record) {
		BTIF_LOG_E("%s: buffer for main task record already init!", __func__);
		return;
	}

	main_task_record = (struct btif_main_task_record *)pvPortMalloc(
						sizeof(struct btif_main_task_record));
	if (main_task_record)
		memset(main_task_record, 0, sizeof(struct btif_main_task_record));
	else {
		BTIF_LOG_E("%s: malloc buffer for main task record FAIL!", __func__);
		return;
	}

	main_task_record->in = xTaskGetTickCount();
}

void btif_main_task_record_deinit(void)
{
	if (main_task_record) {
		vPortFree(main_task_record);
		main_task_record = NULL;
	}
}

void btif_main_task_record_write(bool isr, uint8_t pre_bits, uint8_t event_bits,
								 uint8_t *raw, uint32_t raw_len)
{
	uint32_t index;
	portTickType out;

	if (!main_task_record)
		return;

	if (main_task_record->trigger)
		return;

	if (isr == pdFALSE)
		taskENTER_CRITICAL();

	out = xTaskGetTickCount();
	if (out >= main_task_record->in) { //no overflow
		out = out - main_task_record->in;
	} else { //overflow
		out = out + ((~main_task_record->in) & 0xFFFFFFFF);
	}

	index = main_task_record->index;
	if (main_task_record->index == MAX_MAIN_TASK_RECORD_SIZE - 1)
		main_task_record->index = 0;
	else
		main_task_record->index++;

	main_task_record->in = xTaskGetTickCount();

	if (isr == pdFALSE)
		taskEXIT_CRITICAL();

	main_task_record->info[index].pre_bits = pre_bits;
	main_task_record->info[index].period = (out * portTICK_PERIOD_MS);
	main_task_record->info[index].event_bits = event_bits;
	if (raw && raw_len) {
		memcpy(main_task_record->info[index].raw, raw,
				raw_len > MAX_MAIN_TASK_RECORD_DATA_LEN ? MAX_MAIN_TASK_RECORD_DATA_LEN : raw_len);
		main_task_record->info[index].raw_len = raw_len;
	} else {
		main_task_record->info[index].raw_len = 0;
	}
}

void btif_main_task_record_dump(void)
{
	uint32_t i, j;

	taskENTER_CRITICAL();
	main_task_record->trigger = 1;
	j = main_task_record->index;
	taskEXIT_CRITICAL();

	BTIF_LOG_I("=========== %s ===========", __func__);
	for (i = 0; i < MAX_MAIN_TASK_RECORD_SIZE; i++) {
		if (main_task_record->info[j].pre_bits != RECORD_SYMBOL_INVALID)
			BTIF_LOG_I("[%03d] Evt 0x%02X->0x%02X PD-%03d-", j, main_task_record->info[j].pre_bits,
					   main_task_record->info[j].event_bits, main_task_record->info[j].period);
		else if (main_task_record->info[j].raw_len)
			BTIF_LOG_I("[%03d] Evt    0x%02X    PD-%03d- (0x%x 0x%x 0x%x 0x%x 0x%x len %d)",
					   j, main_task_record->info[j].event_bits, main_task_record->info[j].period,
					   main_task_record->info[j].raw[0], main_task_record->info[j].raw[1],
					   main_task_record->info[j].raw[2], main_task_record->info[j].raw[3],
					   main_task_record->info[j].raw[4], main_task_record->info[j].raw_len);
		else
			BTIF_LOG_I("[%03d] Evt    0x%02X    PD-%03d-", j, main_task_record->info[j].event_bits,
					   main_task_record->info[j].period);
		if (j == MAX_MAIN_TASK_RECORD_SIZE - 1)
			j = 0;
		else
			j++;

		if (i % 49 == 0)
			vTaskDelay(50);
	}
	BTIF_LOG_I("==================================================");

	taskENTER_CRITICAL();
	main_task_record->trigger = 0;
	taskEXIT_CRITICAL();
}
#endif

static int btif_rx_data_consummer(struct mtk_btif *p_btif)
{
	unsigned int length = 0;
	unsigned char *p_buf = NULL;
	/*get BTIF rx buffer's information*/
	struct btif_buf_str_t *p_bbs = &(p_btif->btif_buf);
	/*
	 * wr_idx of btif_buf may be modified in IRQ handler,
	 * in order not to be effected by case in which irq interrupt this operation,
	 * we record wr_idx here
	 */
	taskENTER_CRITICAL();
	unsigned int wr_idx = p_bbs->wr_idx;

	length = BBS_COUNT_CUR(p_bbs, wr_idx);
	taskEXIT_CRITICAL();

	/*make sure length of rx buffer data > 0*/
	do {
		if (length > 0) {
			/*
			 * check if rx_cb empty or not, if registered ,
			 * call user's rx callback to handle these data
			 */
			BTIF_LOG_D("%s, rd_idx = %u, wr_idx = %u, length = %u",
						__func__, p_bbs->rd_idx, wr_idx, length);
			if (p_btif->rx_cb) {
				if (p_bbs->rd_idx <= wr_idx) {
					p_buf = BBS_PTR(p_bbs, p_bbs->rd_idx);
					//length = (wr_idx >= (p_bbs)->rd_idx)?
					//         (wr_idx - (p_bbs)->rd_idx):
					//         BBS_SIZE(p_bbs) - ((p_bbs)->rd_idx - wr_idx);
					(*(p_btif->rx_cb))(p_buf, length);
				} else {
					unsigned int len_tail = BBS_SIZE(p_bbs) - (p_bbs)->rd_idx;

					p_buf = BBS_PTR(p_bbs, p_bbs->rd_idx);
					(*(p_btif->rx_cb))(p_buf, len_tail);
					// length = BBS_COUNT_CUR(p_bbs, wr_idx);
					length -= len_tail;
					/*p_buf = &(p_bbs->buf[0]);*/
					p_buf = BBS_PTR(p_bbs, 0);
					if (length)
						(*(p_btif->rx_cb))(p_buf, length);
				}
			}
#ifdef RESERVED_FOR_FUTURE // houxian todo
			else if (p_btif->rx_notify != NULL)
				(*p_btif->rx_notify) ();
#endif
			else {
				BTIF_LOG_W("p_btif:0x%p, rx_notify, rx_cb = NULL", p_btif);
				break;
			}
		} else {
			// BTIF_LOG_D("length:%d", length);
			break;
		}
		if (g_btif_rx_data_lose) {
			//btmtk_trigger_fw_assert();
			BTIF_LOG_E("rx data consummer: will trigger crash!!!");
			__asm volatile("udf #255");
			g_btif_rx_data_lose = pdFALSE;
		}
		taskENTER_CRITICAL();
		/*update rx data read index*/
		p_bbs->rd_idx = wr_idx;
		wr_idx = p_bbs->wr_idx;
		length = BBS_COUNT_CUR(p_bbs, wr_idx);
		taskEXIT_CRITICAL();
	} while (1);

	return length;
}

#ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET
void btif_task_change_timeout_setting(uint8_t timeout)
{
	if (main_task_timeout_mtx) {
		if (xSemaphoreTake(main_task_timeout_mtx, portMAX_DELAY) == pdTRUE) {
			main_task_timeout = timeout;
			xSemaphoreGive(main_task_timeout_mtx);
		}
	} else {
		main_task_timeout = timeout;
	}
}
#endif /* #ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET */

uint8_t btif_task_get_timeout_setting(void)
{
#ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET
	uint8_t ret = 0;

	if (main_task_timeout_mtx) {
		if (xSemaphoreTake(main_task_timeout_mtx, portMAX_DELAY) == pdTRUE) {
			ret = main_task_timeout;
			xSemaphoreGive(main_task_timeout_mtx);
		}
	} else {
		ret = main_task_timeout;
	}
	return ret;
#else
	return main_task_timeout;
#endif /* #ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET */
}

extern bool bt_driver_is_ready(void);
static TickType_t btif_task_wait_period(bool flag)
{
	/*
	 * There are some cases need to avoid to wait event forever
	 * 1. Wakeup by wakeup IRQ and not receive Rx event
	 * 2. Tx already done
	 * 3. Driver already ON
	 * 4. Coredump not triggered
	 * 5. BT resumed
	 * 6. Ownship ctrl did not enabled
	 */
	if (flag == pdTRUE && btmtk_get_own_ctrl() && btmtk_get_own_type() == DRIVER_OWN &&
		bt_driver_is_on() && btmtk_is_coredump_now() == pdFALSE && bt_driver_is_ready()) {
		return pdMS_TO_TICKS(btif_task_get_timeout_setting());
	}

	/*
	 * If wait event forever, that means we will not set FW OWN until we
	 * receive next event. So if you foundout another case should not set FW OWN,
	 * the judement above should be modified.
	 */
	return portMAX_DELAY;
}

static void btif_main_task(void *p_data)
{
	struct mtk_btif *p_btif = (struct mtk_btif *)p_data;
	EventBits_t bits = 0; // return value of event group
#ifdef BTIF_MAIN_TASK_TRACE
	EventBits_t pre_bits = 0;
#endif
	bool drv_own_set_fail = pdFALSE;

	BTIF_LOG_D("%s: running!", __func__);
	if (p_btif->main_task_event_group == NULL) {
		BTIF_LOG_E("%s: Event group did not initialized!", __func__);
		goto main_task_exit;
	}

	do {
#ifdef BTIF_MAIN_TASK_TRACE
		btif_main_task_record_write(pdFALSE, RECORD_SYMBOL_INVALID, bits, NULL, 0);
#endif
		bits = xEventGroupWaitBits(p_btif->main_task_event_group,
								   BTIF_TASK_EVENT_ALL, pdTRUE, pdFALSE,
								   btif_task_wait_period(btif_is_tx_idle()));
		bits &= BTIF_TASK_EVENT_ALL;
#ifdef BTIF_MAIN_TASK_TRACE
		btif_main_task_record_write(pdFALSE, pre_bits, bits, NULL, 0);
		pre_bits = bits;
#endif
		BTIF_LOG_V("%s: bits %x", __func__, bits);
		/*
		 * Since we may receive a rx event after set FW OWN,
		 * we need to apply sleep lock for this case.
		 * So need to check EVENT_TX_WAIT, EVENT_DRIVER_OWN and EVENT_RX.
		 */
		if (bits & (BTIF_TASK_EVENT_TX_WAIT | BTIF_TASK_EVENT_DRIVER_OWN | BTIF_TASK_EVENT_RX)) {
			// If get Tx or Rx, check OWNSHIP status. If Firmware OWN, set driver OWN.
			if (xSemaphoreTake(p_btif->main_task_mtx, portMAX_DELAY) == pdFALSE)
				BTIF_LOG_V("%s: main_task_mtx take failed", __func__);
#ifdef HAL_SLEEP_MANAGER_ENABLED
			// Sleep lock should be locked here
			if (!(hal_sleep_manager_get_lock_status() & (1 << SLEEP_LOCK_BT)))
				hal_sleep_manager_lock_sleep(SLEEP_LOCK_BT);
#endif
			if (btmtk_get_own_type() == FW_OWN) {
				if (btmtk_set_own_type(DRIVER_OWN)) {
					// Set Driver OWN FAIL! Maybe trigger assert?
					drv_own_set_fail = pdTRUE;
#ifdef BTIF_MAIN_TASK_TRACE
				} else {
					btif_main_task_record_write(pdFALSE, RECORD_SYMBOL_INVALID, RECORD_SYMBOL_DRIVER_OWN, NULL, 0);
#endif
				}
			}
			xSemaphoreGive(p_btif->main_task_mtx);

			if ((bits & BTIF_TASK_EVENT_TX_WAIT) && p_btif->ownship_semaphore) {
				// Give Semaphore
				xSemaphoreGive(p_btif->ownship_semaphore);
			}

			if (drv_own_set_fail) {
				btmtk_dump_driver_own_fail_regs();
				btmtk_trigger_fw_assert();
				drv_own_set_fail = pdFALSE;
			}
			// If we received wakeup IRQ but no Rx event, we should wait Rx event.
			// But if we did not get it, it is fine to set FW OWN.
			// TODO : How Long to Wait
			if ((bits & BTIF_TASK_EVENT_DRIVER_OWN) && ((bits & BTIF_TASK_EVENT_RX) == 0)) {
#ifdef BTIF_MAIN_TASK_TRACE
				btif_main_task_record_write(pdFALSE, RECORD_SYMBOL_INVALID, bits, NULL, 0);
#endif
				bits = xEventGroupWaitBits(p_btif->main_task_event_group, BTIF_TASK_EVENT_RX,
										   pdTRUE, pdFALSE, pdMS_TO_TICKS(1000));
				if (bits == 0) {
					BTIF_LOG_W("%s: Wakeup but not get RX Event in 1000ms", __func__);
					continue;
				}
#ifdef BTIF_MAIN_TASK_TRACE
				btif_main_task_record_write(pdFALSE, pre_bits, bits, NULL, 0);
				pre_bits = bits;
#endif
			}
		}

		// Receive RX event
		if (bits & BTIF_TASK_EVENT_RX)
			btif_rx_data_consummer(p_btif);

		if (bits == 0 || bits & BTIF_TASK_EVENT_FW_OWN) {
			if (xSemaphoreTake(p_btif->main_task_mtx, portMAX_DELAY) == pdFALSE)
				BTIF_LOG_V("%s: main_task_mtx get failed", __func__);

			if ((btmtk_get_own_type() == DRIVER_OWN) && btif_is_tx_idle() == pdTRUE) {
				/*
				 * Wait event timeout and did not wait for tx done!
				 * Check OWNSHIP status. If Driver OWN, set Firmware OWN.
				 */
				if (btmtk_set_own_type(FW_OWN)) {
					/*
					 * Set Firmware OWN FAIL! Maybe trigger assert?
					 */
#ifdef BTIF_MAIN_TASK_TRACE
				} else {
					btif_main_task_record_write(pdFALSE, RECORD_SYMBOL_INVALID, RECORD_SYMBOL_FW_OWN, NULL, 0);
#endif
				}
			}
#ifdef HAL_SLEEP_MANAGER_ENABLED
			if ((btmtk_get_own_type() == FW_OWN) &&
				(hal_sleep_manager_get_lock_status() & (1 << SLEEP_LOCK_BT)) &&
				btmtk_check_HW_TRX_empty()) {
				/*
				 * If the process for set FW own failed, the local own status may not the same with own status in CR.
				 * So we check local own status here.
				 */
				if (btmtk_get_local_own_type() == DRIVER_OWN) {
					BTIF_LOG_W("%s: OWN mismatch. Change local own to FW own", __func__);
					btmtk_set_local_own_type(FW_OWN);
				}
				hal_sleep_manager_unlock_sleep(SLEEP_LOCK_BT);
			}
#endif
			xSemaphoreGive(p_btif->main_task_mtx);
		}
	} while (1);

main_task_exit:
	return;
}

void btif_dump_reg(void)
{
	unsigned int base = (unsigned int)g_btif.p_base;
	// int idx = 0;
	// unsigned char reg_map[0xE0 / 4] = { 0 };

	/*WARNING! Should not read BTIF_BASE + 0*/
	// BTIF_LOG_I("REG-RBR = 0x%08x", BTIF_READ32(BTIF_RBR(base)));
	// BTIF_LOG_I("REG-THR = 0x%08x", BTIF_READ32(BTIF_THR(base)));
	BTIF_LOG_I("IER: 0x%08x, IIR: 0x%08x, LSR: 0x%08x, SLP_EN: 0x%08x",
				BTIF_READ32(BTIF_IER(base)), BTIF_READ32(BTIF_IIR(base)),
				BTIF_READ32(BTIF_LSR(base)), BTIF_READ32(BTIF_SLEEP_EN(base)));
	BTIF_LOG_I("FIFO_CTRL: 0x%08x, FAKE_LCR: 0x%08x, DMA_EN: 0x%08x, RTOCNT: 0x%08x",
				BTIF_READ32(BTIF_FIFOCTRL(base)), BTIF_READ32(BTIF_FAKELCR(base)),
				BTIF_READ32(BTIF_DMA_EN(base)), BTIF_READ32(BTIF_RTOCNT(base)));
	BTIF_LOG_I("TRI_LVL: 0x%08x, WAK: 0x%08x, WAT_TM: 0x%08x, HANDSHAKE: 0x%08x",
				BTIF_READ32(BTIF_TRI_LVL(base)), BTIF_READ32(BTIF_WAK(base)),
				BTIF_READ32(BTIF_WAT_TIME(base)), BTIF_READ32(BTIF_HANDSHAKE(base)));

	BTIF_LOG_I("TX_FIFO 0~7: %08x %08x %08x %08x %08x %08x %08x %08x",
				BTIF_READ32(BTIF_DBG_TX_FIFO_0(base)), BTIF_READ32(BTIF_DBG_TX_FIFO_1(base)),
				BTIF_READ32(BTIF_DBG_TX_FIFO_2(base)), BTIF_READ32(BTIF_DBG_TX_FIFO_3(base)),
				BTIF_READ32(BTIF_DBG_TX_FIFO_4(base)), BTIF_READ32(BTIF_DBG_TX_FIFO_5(base)),
				BTIF_READ32(BTIF_DBG_TX_FIFO_6(base)), BTIF_READ32(BTIF_DBG_TX_FIFO_7(base)));
	BTIF_LOG_I("TX_FIFO 8~f: %08x %08x %08x %08x %08x %08x %08x %08x",
				BTIF_READ32(BTIF_DBG_TX_FIFO_8(base)), BTIF_READ32(BTIF_DBG_TX_FIFO_9(base)),
				BTIF_READ32(BTIF_DBG_TX_FIFO_a(base)), BTIF_READ32(BTIF_DBG_TX_FIFO_b(base)),
				BTIF_READ32(BTIF_DBG_TX_FIFO_c(base)), BTIF_READ32(BTIF_DBG_TX_FIFO_d(base)),
				BTIF_READ32(BTIF_DBG_TX_FIFO_e(base)), BTIF_READ32(BTIF_DBG_TX_FIFO_f(base)));
	BTIF_LOG_I("RX_FIFO 0~7: %08x %08x %08x %08x %08x %08x %08x %08x",
				BTIF_READ32(BTIF_DBG_RX_FIFO_0(base)), BTIF_READ32(BTIF_DBG_RX_FIFO_1(base)),
				BTIF_READ32(BTIF_DBG_RX_FIFO_2(base)), BTIF_READ32(BTIF_DBG_RX_FIFO_3(base)),
				BTIF_READ32(BTIF_DBG_RX_FIFO_4(base)), BTIF_READ32(BTIF_DBG_RX_FIFO_5(base)),
				BTIF_READ32(BTIF_DBG_RX_FIFO_6(base)), BTIF_READ32(BTIF_DBG_RX_FIFO_7(base)));

	BTIF_LOG_I("TXPTR: 0x%08x, RXPTR: 0x%08x",
				BTIF_READ32(BTIF_DBG_TX_PTR(base)), BTIF_READ32(BTIF_DBG_RX_PTR(base)));

	/* Print IRQ Status:
	 * 		BTIF_CLK = 0x34030400
	 * 		BTIF_IRQ = 0xE000E108, 0xE000E208
	 */
	BTIF_LOG_I("CLK 0x%x = 0x%08x, IRQ 0x%x = 0x%08x, IRQ 0x%x = 0x%08x",
				0x34030400, BTIF_READ32(0x34030400), 0xE000E108, BTIF_READ32(0xE000E108),
				0xE000E208, BTIF_READ32(0xE000E208));

	BTIF_LOG_I("Tx DMA %s, Rx DMA %s, Rx data is %s, Tx data is %s",
				(BTIF_READ32(BTIF_DMA_EN(base)) & BTIF_DMA_EN_TX) ? "enabled" : "disabled",
				(BTIF_READ32(BTIF_DMA_EN(base)) & BTIF_DMA_EN_RX) ? "enabled" : "disabled",
				(BTIF_READ32(BTIF_LSR(base)) & BTIF_LSR_DR_BIT) ? "not empty" : "empty",
				(BTIF_READ32(BTIF_LSR(base)) & BTIF_LSR_TEMT_BIT) ? "empty" : "not empty");
	// use UNUSED macro for debug warnig
	UNUSED(base);

	if (g_btif.tx_mode == BTIF_MODE_DMA)
		btif_dma_dump_reg(g_btif.p_tx_dma);

	if (g_btif.rx_mode == BTIF_MODE_DMA)
		btif_dma_dump_reg(g_btif.p_rx_dma);
}

int btif_irq_ctrl(unsigned int irq_id, unsigned char en)
{
	BTIF_LOG_V("%s irq_id: %d, op=%d", __func__, irq_id, en);
	return btif_platform_irq_ctrl(irq_id, en);
}

void btif_clk_ctrl(unsigned char en)
{
	btif_platform_clk_ctrl(en);
}

void btif_rx_cb_reg(btif_rx_cb rx_cb)
{
	if (g_btif.rx_cb) {
		BTIF_LOG_D("stp_rx_cb already exist, rewrite from (0x%p) to (0x%p)",
					g_btif.rx_cb, rx_cb);
	}
	g_btif.rx_cb = rx_cb;
}

void btif_tx_mode_ctrl(struct mtk_btif *p_btif)
{
	unsigned int base = 0;

	if (p_btif->p_base == NULL) {
		BTIF_LOG_E("%s, p_base is NULL", __func__);
		return;
	}

	base = (unsigned int)p_btif->p_base;
	if (p_btif->tx_mode == BTIF_MODE_DMA)
		BTIF_SET_BIT(BTIF_DMA_EN(base), BTIF_DMA_EN_TX); /*set to DMA mode*/
	else
		BTIF_CLR_BIT(BTIF_DMA_EN(base), BTIF_DMA_EN_TX); /*set to PIO mode*/
}

void btif_rx_mode_ctrl(struct mtk_btif *p_btif)
{
	unsigned int base = 0;

	if (p_btif->p_base == NULL) {
		BTIF_LOG_E("%s, p_base is NULL", __func__);
		return;
	}

	base = (unsigned int)p_btif->p_base;
	if (p_btif->tx_mode == BTIF_MODE_DMA)
		BTIF_SET_BIT(BTIF_DMA_EN(base), BTIF_DMA_EN_RX); /*set to DMA mode*/
	else
		BTIF_CLR_BIT(BTIF_DMA_EN(base), BTIF_DMA_EN_RX); /*set to PIO mode*/
}

static void btif_rx_ier_ctrl(struct mtk_btif *p_btif, unsigned char en)
{
	unsigned int base = (unsigned int)p_btif->p_base;

	if (en == FALSE)
		BTIF_CLR_BIT(BTIF_IER(base), BTIF_IER_RXFEN);
	else
		BTIF_SET_BIT(BTIF_IER(base), BTIF_IER_RXFEN);
}

static void btif_tx_ier_ctrl(struct mtk_btif *p_btif, unsigned char en)
{
	unsigned int base = (unsigned int)p_btif->p_base;

	if (en == FALSE)
		BTIF_CLR_BIT(BTIF_IER(base), BTIF_IER_TXEEN);
	else
		BTIF_SET_BIT(BTIF_IER(base), BTIF_IER_TXEEN);
}

void btif_loopback_ctrl(unsigned char en)
{
	unsigned int base = (unsigned int)g_btif.p_base;

	if (en == FALSE)
		BTIF_CLR_BIT(BTIF_TRI_LVL(base), BTIF_TRI_LOOP_EN);
	else
		BTIF_SET_BIT(BTIF_TRI_LVL(base), BTIF_TRI_LOOP_EN);
}

#ifdef BUFFER_DEBUG_TRACE
void btif_buffer_debug_init(void)
{
	BTIF_LOG_I("%s", __func__);
	if (btif_buf_record) {
		BTIF_LOG_E("%s: buffer for debug trace already init!", __func__);
		return;
	}
	btif_buf_record = (struct btif_buf_debug_trace *)pvPortMalloc(
					   sizeof(struct btif_buf_debug_trace));
	if (btif_buf_record)
		memset(btif_buf_record, 0, sizeof(struct btif_buf_debug_trace));
	else
		BTIF_LOG_E("%s: malloc buffer for debug trace FAIL!", __func__);
}

void btif_buffer_debug_deinit(void)
{
	BTIF_LOG_I("%s", __func__);
	if (btif_buf_record) {
		vPortFree(btif_buf_record);
		btif_buf_record = NULL;
	}
}

static unsigned int *btif_buffer_debug_find_last(unsigned int *idx0, unsigned int *idx1,
												 unsigned int *idx2, unsigned int *idx3)
{
	unsigned int *ret = NULL;
	unsigned int smallest = MAX_BUF_DEBUG_COUNT;

	if (!btif_buf_record) {
		BTIF_LOG_E("%s: buf for debug trace not initialized!", __func__);
		return NULL;
	}

	if (idx0 && *idx0 < MAX_BUF_DEBUG_RECORD && (btif_buf_record->tx_vfifo_trace[*idx0].seq_num < smallest)) {
		ret = idx0;
		smallest = btif_buf_record->tx_vfifo_trace[*idx0].seq_num;
	}
	if (idx1 && *idx1 < MAX_BUF_DEBUG_RECORD && (btif_buf_record->tx_irq_trace[*idx1].seq_num < smallest)) {
		ret = idx1;
		smallest = btif_buf_record->tx_irq_trace[*idx1].seq_num;
	}
	if (idx2 && *idx2 < MAX_BUF_DEBUG_RECORD && (btif_buf_record->rx_task_buf_trace[*idx2].seq_num < smallest)) {
		ret = idx2;
		smallest = btif_buf_record->rx_task_buf_trace[*idx2].seq_num;
	}
	if (idx3 && *idx3 < MAX_BUF_DEBUG_RECORD && (btif_buf_record->rx_irq_trace[*idx3].seq_num < smallest))
		ret = idx3;

	return ret;
}

void btif_buffer_trace_restart(void)
{
	BTIF_LOG_I("%s", __func__);

	if (!btif_buf_record) {
		BTIF_LOG_E("%s: buffer for debug trace did not initialized!", __func__);
		return;
	}

	taskENTER_CRITICAL();
	btif_buf_record->trigger = 0;
	taskEXIT_CRITICAL();
}

void btif_buffer_debug(void)
{
	struct btif_buf_debug_tx_vfifo *tx_vfifo_buf_p = NULL;
	struct btif_buf_debug_tx_irq *tx_irq_p = NULL;
	struct btif_buf_debug_rx_task_buf *rx_task_buf_p = NULL;
	struct btif_buf_debug_rx_irq *rx_irq_p = NULL;
	unsigned char *base = (unsigned char *)BTIF_APDMA_TX_BASE;
	unsigned int i = 0;
	unsigned int idx0 = 0;
	unsigned int idx1 = 0;
	unsigned int idx2 = 0;
	unsigned int idx3 = 0;
	unsigned int *idx_print = NULL;

	if (!btif_buf_record) {
		BTIF_LOG_E("%s: buffer for debug trace did not initialized!", __func__);
		return;
	}

	if (btif_buf_record->trigger)
		return;

	BTIF_LOG_E("%s", __func__);
	taskENTER_CRITICAL();
	btif_buf_record->trigger = 1;
	taskEXIT_CRITICAL();

	idx0 = btif_buf_record->tx_vfifo_buf_record_idx;
	idx1 = btif_buf_record->tx_irq_record_idx;
	idx2 = btif_buf_record->rx_task_buf_record_idx;
	idx3 = btif_buf_record->rx_irq_record_idx;
	BTIF_LOG_I("--------------------- BUFFER INFO START ---------------------");
	BTIF_LOG_E("[TX] vff_len:%d valid_size:%d left_len:%d rpt:%d wpt:%d",
				BTIF_READ32(TX_DMA_VFF_LEN(base)), BTIF_READ32(TX_DMA_VFF_VALID_SIZE(base)),
				BTIF_READ32(TX_DMA_VFF_LEFT_SIZE(base)), BTIF_READ32(TX_DMA_VFF_RPT(base)),
				BTIF_READ32(TX_DMA_VFF_WPT(base)));
	BTIF_LOG_E("[RX] Buf Max Used:%d/%d", BTIF_RX_BUFFER_SIZE - g_buf_min_free_size, BTIF_RX_BUFFER_SIZE);

	tx_vfifo_buf_p = &btif_buf_record->tx_vfifo_trace[idx0];
	tx_irq_p = &btif_buf_record->tx_irq_trace[idx1];
	rx_task_buf_p = &btif_buf_record->rx_task_buf_trace[idx2];
	rx_irq_p = &btif_buf_record->rx_irq_trace[idx3];

	/* reset the record to monitor minimum free buffer size */
	g_buf_min_free_size = BTIF_RX_BUFFER_SIZE;

	for (i = 0; i < MAX_BUF_DEBUG_RECORD * 4; i++) {
		idx_print = btif_buffer_debug_find_last(&idx0, &idx1, &idx2, &idx3);

		if (idx0 < MAX_BUF_DEBUG_RECORD && &idx0 == idx_print) {
			BTIF_LOG_E("[%u]tx_cmd: %02x %02x %02x %02x %02x (len=%d) ava: %d wpt: %d wpt_wrap: %d",
						tx_vfifo_buf_p->seq_num, tx_vfifo_buf_p->buf_data[0],
						tx_vfifo_buf_p->buf_data[1], tx_vfifo_buf_p->buf_data[2],
						tx_vfifo_buf_p->buf_data[3], tx_vfifo_buf_p->buf_data[4],
						tx_vfifo_buf_p->length, tx_vfifo_buf_p->ava_len,
						tx_vfifo_buf_p->wpt, tx_vfifo_buf_p->wpt_wrap);
			idx0++;
			if (idx0 == MAX_BUF_DEBUG_RECORD)
				idx0 = 0;
			if (idx0 == btif_buf_record->tx_vfifo_buf_record_idx)
				idx0 = MAX_BUF_DEBUG_RECORD;
			else
				tx_vfifo_buf_p = &btif_buf_record->tx_vfifo_trace[idx0];
		} else if (idx1 < MAX_BUF_DEBUG_RECORD && &idx1 == idx_print) {
			BTIF_LOG_E("[%u]tx_cmd_done: vff: %d vl: %d left: %d wpt: %d rpt: %d own(f/d): %d/%d",
						tx_irq_p->seq_num, tx_irq_p->vff_len, tx_irq_p->valid_len,
						tx_irq_p->left_len, tx_irq_p->wpt, tx_irq_p->rpt,
						tx_irq_p->own_type, btmtk_get_local_own_type());
			idx1++;
			if (idx1 == MAX_BUF_DEBUG_RECORD)
				idx1 = 0;
			if (idx1 == btif_buf_record->tx_irq_record_idx)
				idx1 = MAX_BUF_DEBUG_RECORD;
			else
				tx_irq_p = &btif_buf_record->tx_irq_trace[idx1];
		} else if (idx2 < MAX_BUF_DEBUG_RECORD && &idx2 == idx_print) {
			BTIF_LOG_E("[%u]btmtk_data_recv: %02x %02x %02x %02x %02x (len = %d)",
						rx_task_buf_p->seq_num, rx_task_buf_p->buf_data[0],
						rx_task_buf_p->buf_data[1], rx_task_buf_p->buf_data[2],
						rx_task_buf_p->buf_data[3], rx_task_buf_p->buf_data[4],
						rx_task_buf_p->length);
			idx2++;
			if (idx2 == MAX_BUF_DEBUG_RECORD)
				idx2 = 0;
			if (idx2 == btif_buf_record->rx_task_buf_record_idx)
				idx2 = MAX_BUF_DEBUG_RECORD;
			else
				rx_task_buf_p = &btif_buf_record->rx_task_buf_trace[idx2];
		} else if (idx3 < MAX_BUF_DEBUG_RECORD && &idx3 == idx_print) {
			BTIF_LOG_E("[%u]bbs_write: w_len: %d emp_len: %d use_len: %d rd_idx: %d wr_idx: %d",
						rx_irq_p->seq_num, rx_irq_p->write_len, rx_irq_p->emp_len,
						rx_irq_p->use_len, rx_irq_p->rd_idx, rx_irq_p->wr_idx);
			idx3++;
			if (idx3 == MAX_BUF_DEBUG_RECORD)
				idx3 = 0;
			if (idx3 == btif_buf_record->rx_irq_record_idx)
				idx3 = MAX_BUF_DEBUG_RECORD;
			else
				rx_irq_p = &btif_buf_record->rx_irq_trace[idx3];
		} else {
			BTIF_LOG_E("%s: ERROR! index not expected!! %p %p %p %p %p",
						__func__, idx_print, &idx0, &idx1, &idx2, &idx3);
		}
	}
	BTIF_LOG_I("--------------------- BUFFER INFO END ---------------------");
	// use UNUSED macro for debug warnig
	UNUSED(tx_vfifo_buf_p);
	UNUSED(tx_irq_p);
	UNUSED(rx_task_buf_p);
	UNUSED(rx_irq_p);
	UNUSED(base);
}

static unsigned int btif_get_trace_count(void)
{
	unsigned int ret = buf_debug_count;

	if ((buf_debug_count & MAX_BUF_DEBUG_COUNT) == MAX_BUF_DEBUG_COUNT)
		buf_debug_count = 0;
	else
		buf_debug_count++;
	return ret;
}

void btif_tx_irq_trace_write(unsigned int vff_len, unsigned int valid_len,
							 unsigned int left_len, unsigned int wpt,
							 unsigned int rpt, uint8_t own)
{
	struct btif_buf_debug_tx_irq *tx_irq_p = NULL;

	if (!btif_buf_record)
		return;

	if (btif_buf_record->trigger)
		return;

	tx_irq_p = &btif_buf_record->tx_irq_trace[btif_buf_record->tx_irq_record_idx];

	tx_irq_p->seq_num = btif_get_trace_count();
	tx_irq_p->vff_len = vff_len;
	tx_irq_p->valid_len = valid_len;
	tx_irq_p->left_len = left_len;
	tx_irq_p->wpt = wpt;
	tx_irq_p->rpt = rpt;
	tx_irq_p->own_type = own;
	btif_buf_record->tx_irq_record_idx++;
	if (btif_buf_record->tx_irq_record_idx == MAX_BUF_DEBUG_RECORD)
		btif_buf_record->tx_irq_record_idx = 0;
}

void btif_tx_vfifo_trace_write(const unsigned char *buffer, unsigned int length,
							   unsigned int ava_len, unsigned int wpt, unsigned int wpt_wrap)
{
	struct btif_buf_debug_tx_vfifo *tx_vfifo_p = NULL;

	if (!btif_buf_record)
		return;

	if (btif_buf_record->trigger)
		return;

	tx_vfifo_p = &btif_buf_record->tx_vfifo_trace[btif_buf_record->tx_vfifo_buf_record_idx];

	tx_vfifo_p->seq_num = btif_get_trace_count();
	tx_vfifo_p->length = length;
	tx_vfifo_p->buf_p = (unsigned char *)buffer;
	memcpy(tx_vfifo_p->buf_data, buffer,
		   length < MAX_BUF_DATA_RECORD_LEN ? length : MAX_BUF_DATA_RECORD_LEN);
	tx_vfifo_p->ava_len = ava_len;
	tx_vfifo_p->wpt = wpt;
	tx_vfifo_p->wpt_wrap = wpt_wrap;
	btif_buf_record->tx_vfifo_buf_record_idx++;
	if (btif_buf_record->tx_vfifo_buf_record_idx == MAX_BUF_DEBUG_RECORD)
		btif_buf_record->tx_vfifo_buf_record_idx = 0;
}

static void btif_rx_irq_trace_write(unsigned int write_len, unsigned int emp_len,
									unsigned int use_len, unsigned int rd_idx,
									unsigned int write_idx)
{
	struct btif_buf_debug_rx_irq *rx_irq_p = NULL;

	rx_irq_p = &btif_buf_record->rx_irq_trace[btif_buf_record->rx_irq_record_idx];

	if (!btif_buf_record)
		return;

	if (btif_buf_record->trigger)
		return;

	if (emp_len < g_buf_min_free_size)
		g_buf_min_free_size = emp_len;

	rx_irq_p->seq_num = btif_get_trace_count();
	rx_irq_p->write_len = write_len;
	rx_irq_p->emp_len = emp_len;
	rx_irq_p->use_len = use_len;
	rx_irq_p->rd_idx = rd_idx;
	rx_irq_p->wr_idx = write_idx;
	btif_buf_record->rx_irq_record_idx++;
	if (btif_buf_record->rx_irq_record_idx == MAX_BUF_DEBUG_RECORD)
		btif_buf_record->rx_irq_record_idx = 0;
}

void btif_rx_task_buffer_trace_write(const unsigned char *buffer, unsigned int length)
{
	struct btif_buf_debug_rx_task_buf *rx_task_buf_p = NULL;

	rx_task_buf_p = &btif_buf_record->rx_task_buf_trace[btif_buf_record->rx_task_buf_record_idx];

	if (!btif_buf_record)
		return;

	if (btif_buf_record->trigger)
		return;

	taskENTER_CRITICAL();
	rx_task_buf_p->seq_num = btif_get_trace_count();
	taskEXIT_CRITICAL();
	rx_task_buf_p->buf_p = (unsigned char *)buffer;
	memcpy(rx_task_buf_p->buf_data, buffer,
		   length < MAX_BUF_DATA_RECORD_LEN ? length : MAX_BUF_DATA_RECORD_LEN);
	rx_task_buf_p->length = length;
	btif_buf_record->rx_task_buf_record_idx++;
	if (btif_buf_record->rx_task_buf_record_idx == MAX_BUF_DEBUG_RECORD)
		btif_buf_record->rx_task_buf_record_idx = 0;
}
#endif /* #ifdef BUFFER_DEBUG_TRACE */

static unsigned int _btif_bbs_wr_direct(struct btif_buf_str_t *p_bbs,
										unsigned char *p_buf, unsigned int buf_len)
{
	unsigned int tail_len = 0;
	unsigned int l = 0;
	unsigned int tmp_wr_idx = p_bbs->wr_idx;

	tail_len = BBS_SIZE(p_bbs) - tmp_wr_idx;
	BTIF_LOG_V("%s, buf_len: %d, tmp_wr_idx:%d, tail_len:%d", __func__,
				buf_len, tmp_wr_idx, tail_len);

	l = tail_len < buf_len ? tail_len : buf_len;

	memcpy((p_bbs->p_buf) + tmp_wr_idx, p_buf, l);
	memcpy(p_bbs->p_buf, p_buf + l, buf_len - l);

	tmp_wr_idx += buf_len;
	// tmp_wr_idx &= BBS_MASK(p_bbs);
	if (tmp_wr_idx >= BBS_SIZE(p_bbs))
		tmp_wr_idx -= BBS_SIZE(p_bbs);
	p_bbs->wr_idx = tmp_wr_idx;

	BTIF_LOG_V("%s, wr_idx:%d", __func__, p_bbs->wr_idx);

	return buf_len;
}

static unsigned int _btif_bbs_write(struct btif_buf_str_t *p_bbs,
									unsigned char *p_buf, unsigned int buf_len)
{
	/*in IRQ context, so read operation won't interrupt this operation*/

	unsigned int wr_len = 0;
	unsigned int emp_len = 0;
	unsigned int ava_len = 0;

	if (p_bbs == NULL) {
		BTIF_LOG_E("Error btif bbs buf is NULL");
		return 0;
	}
	emp_len = BBS_LEFT(p_bbs);

	BTIF_LOG_D("%s, want_wr: %d, emp_len: %d, buf_use: %u, rd_idx: %u, wr_idx: %u",
				__func__, buf_len, emp_len, BBS_COUNT(p_bbs), p_bbs->rd_idx, p_bbs->wr_idx);
#ifdef BTIF_MAIN_TASK_TRACE
	btif_main_task_record_write(pdTRUE, RECORD_SYMBOL_INVALID, RECORD_SYMBOL_WRITE_RX, NULL, 0);
#endif
#ifdef BUFFER_DEBUG_TRACE
	btif_rx_irq_trace_write(buf_len, emp_len, BBS_COUNT(p_bbs), p_bbs->rd_idx, p_bbs->wr_idx);
#endif

	if (emp_len == 0) {
		if (!g_btif_rx_data_lose)
			BTIF_LOG_E("no empty space for write, (%d)emp_len, (%d)to write", ava_len, buf_len);
		// After g_btif_rx_data_lose set as true, btif_main_task will trigger assert
		g_btif_rx_data_lose = pdTRUE;
		return 0;
	}

	ava_len = emp_len - 1;
	if (ava_len == 0) {
		if (!g_btif_rx_data_lose)
			BTIF_LOG_E("no empty space for write, (%d)ava_len, (%d)to write", ava_len, buf_len);
		// btif_dump_reg();

		// After g_btif_rx_data_lose set as true, btif_main_task will trigger assert
		g_btif_rx_data_lose = pdTRUE;
		return 0;
	}

	if (ava_len < buf_len) {
		if (!g_btif_rx_data_lose)
			BTIF_LOG_E("BTIF overrun, (%d)empty, (%d)needed", emp_len, buf_len);
		//btif_dump_reg();
		btif_util_dump_buffer("<DMA Rx vFIFO>", p_buf, buf_len, 0);
		g_btif_rx_data_lose = pdTRUE;
		return 0;
	}

	wr_len = buf_len < ava_len ? buf_len : ava_len;
	_btif_bbs_wr_direct(p_bbs, p_buf, wr_len);

	if (BBS_COUNT(p_bbs) >= BTIF_RX_BUF_HIGH_LEVEL_SIZE) {
		if ((g_buf_high_cnt & 0x000F) == 0) {
			BTIF_LOG_W("bbs_w, rx buf high: %d/%u, r: %u, w: %u",
						BBS_COUNT(p_bbs), p_bbs->size, p_bbs->rd_idx, p_bbs->wr_idx);
		}
		g_buf_high_cnt++;
		return wr_len;
	}
	if (g_buf_high_cnt) {
		BTIF_LOG_D("bbs_w, rx buf high: %d/%u, r: %u, w: %u",
					BBS_COUNT(p_bbs), p_bbs->size, p_bbs->rd_idx, p_bbs->wr_idx);
		g_buf_high_cnt = 0;
	}

	return wr_len;
}

static int _btif_data_receiver(unsigned char *p_buf, unsigned int buf_len)
{
	btif_util_dump_buffer("BBS RX", p_buf, buf_len, 0);
	_btif_bbs_write(&(g_btif.btif_buf), p_buf, buf_len);
	return 0;
}

void btif_consummer_nty(void)
{
	// notify btif_rx_task to read data from bbs buffer
	// if(btif_rx_data_ready_nty)
	//    btif_rx_data_ready_nty();
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BaseType_t xResult = 0;

	if (g_btif.main_task_event_group && g_btif.main_task_hdl) {
		xResult = xEventGroupSetBitsFromISR(g_btif.main_task_event_group,
											BTIF_TASK_EVENT_RX, &xHigherPriorityTaskWoken);
		if (xResult != pdFAIL)
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	} else {
		// Print error message
		BTIF_LOG_E("%s: main_task_event_group or main_task did not created", __func__);
	}
	BTIF_LOG_V("%s", __func__);
}

static int _btif_tx_setup(struct mtk_btif *p_btif)
{
	int i_ret = 0;

	if (p_btif->tx_mode == BTIF_MODE_DMA) {
		i_ret = btif_dma_tx_setup(p_btif);
		if (i_ret) {
			BTIF_LOG_E("%s failed,i_ret(%d)", __func__, i_ret);
			return i_ret;
		}
	} else {
		// do nothing, DMA EN is disabled
	}

	return i_ret;
}

static void _btif_tx_free(struct mtk_btif *p_btif)
{
	if (p_btif->tx_mode == BTIF_MODE_DMA)
		btif_dma_free((struct btif_dma_t *)p_btif->p_tx_dma);
}

static int _btif_rx_setup(struct mtk_btif *p_btif)
{
	int i_ret = 0;

	if (p_btif->rx_mode == BTIF_MODE_DMA) {
		i_ret = btif_dma_rx_setup(p_btif);
		if (i_ret) {
			BTIF_LOG_E("%s failed,i_ret(%d)", __func__, i_ret);
			return i_ret;
		}
		btif_dma_rx_cb_reg((struct btif_dma_t *)p_btif->p_rx_dma,
							(dma_rx_data_cb)_btif_data_receiver);
	} else {
		// do nothing, DMA EN is disabled
	}
	return i_ret;
}

static void _btif_rx_free(struct mtk_btif *p_btif)
{
	if (p_btif->rx_mode == BTIF_MODE_DMA)
		btif_dma_free((struct btif_dma_t *)p_btif->p_rx_dma);
}

static int _btif_buffer_init(void)
{
	int i_ret = 0;

	// BTIF_LOG_I("%s, free heap size = 0x%x Byte", __func__, xPortGetFreeHeapSize());

	// bbs buffer for RX data(both for DMA and PIO mode)
	g_btif.btif_buf.p_buf = _btif_rx_buf;
	BBS_INIT(&(g_btif.btif_buf));

	// tx dma vfifo
	if (g_btif.tx_mode == BTIF_MODE_DMA) {
		// tx dma semaphore
		i_ret = btif_dma_tx_semaphore_init((struct btif_dma_t *)g_btif.p_tx_dma);
		if (i_ret) {
			BTIF_LOG_E("create tx dma semaphore fail");
			return i_ret;
		}

		i_ret = btif_dma_vfifo_init((struct btif_dma_t *)g_btif.p_tx_dma);
		if (i_ret) {
			BTIF_LOG_E("alloc tx dma vfifo fail");
			return i_ret;
		}
	}
	// rx dma vfifo
	if (g_btif.rx_mode == BTIF_MODE_DMA) {
		i_ret = btif_dma_vfifo_init((struct btif_dma_t *)g_btif.p_rx_dma);
		if (i_ret) {
			BTIF_LOG_E("alloc rx dma vfifo fail");
			return i_ret;
		}
	}

	return 0;
}

static void _btif_buffer_deinit(void)
{
	BTIF_LOG_FUNC();
	if (g_btif.btif_buf.p_buf) {
		BBS_INIT(&(g_btif.btif_buf));
		g_btif.btif_buf.p_buf = NULL;
	}
	if (g_btif.tx_mode == BTIF_MODE_DMA) {
		btif_dma_vfifo_deinit((struct btif_dma_t *)g_btif.p_tx_dma);
		btif_dma_tx_semaphore_deinit((struct btif_dma_t *)g_btif.p_tx_dma);
	}
	if (g_btif.rx_mode == BTIF_MODE_DMA)
		btif_dma_vfifo_deinit((struct btif_dma_t *)g_btif.p_rx_dma);
}

// irq for PIO mode
static void _btif_irq_handler(int irq, void *dev)
{
	unsigned int iir = 0;
	unsigned int rx_len = 0;
	unsigned char rx_buf[256];
	unsigned int local_buf_len = 256;
	unsigned char b_notify = FALSE;

	btif_irq_ctrl(g_btif.irq_id, FALSE);

	UBaseType_t savedIrqStatus = taskENTER_CRITICAL_FROM_ISR();
	/*read interrupt identifier register*/
	iir = BTIF_READ32(BTIF_IIR(g_btif.p_base));
	BTIF_LOG_V("%s, iir = %d", __func__, iir);
	while ((iir & (BTIF_IIR_RX | BTIF_IIR_RX_TIMEOUT)) && (rx_len < local_buf_len)) {
		// rx data available
		rx_buf[rx_len] = BTIF_READ8(g_btif.p_base);
		rx_len++;
		if (rx_len == local_buf_len) { // flush every 256 bytes
			// if (g_btif.rx_cb)
			//	 (g_btif.rx_cb)(rx_buf, rx_len);
			b_notify = TRUE;
			_btif_data_receiver(rx_buf, rx_len);
			rx_len = 0;
		}
		iir = BTIF_READ32(BTIF_IIR(g_btif.p_base)); /*update IIR*/
		BTIF_LOG_I("%s, new iir = %d", __func__, iir);
	}
	if (rx_len > 0) { // less than 256
		// if (g_btif.rx_cb)
		//	 (g_btif.rx_cb)(rx_buf, rx_len);
		b_notify = TRUE;
		_btif_data_receiver(rx_buf, rx_len);
	}
	/*is tx interrupt exist?*/
	if (iir & BTIF_IIR_TX_EMPTY) { // can do tx now
		// if tx data is buffered, it can bt sent here. Otherwise, just clear the irq identifier
		btif_tx_ier_ctrl(&g_btif, FALSE);
	}
	taskEXIT_CRITICAL_FROM_ISR(savedIrqStatus);

	btif_irq_ctrl(g_btif.irq_id, TRUE);

	if (b_notify)
		btif_consummer_nty();
}

static void _btif_wakeup_irq_handler(int irq, void *dev)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BaseType_t xResult = 0;
	UBaseType_t savedIrqStatus = taskENTER_CRITICAL_FROM_ISR();

	BTIF_LOG_V("conn2ap btif wakeup interrupt(irq = %d)", irq);

	// Will enable this IRQ again after set FW OWN
	btif_irq_ctrl(BTIF_WAKEUP_IRQ_ID, FALSE);

#ifdef BTIF_MAIN_TASK_TRACE
	btif_main_task_record_write(pdTRUE, RECORD_SYMBOL_INVALID, RECORD_SYMBOL_WAKEUP_IRQ, NULL, 0);
#endif

	if (g_btif.main_task_event_group && g_btif.main_task_hdl) {
		BTIF_LOG_V("%s: notify main task set driver own", __func__);
		xResult = xEventGroupSetBitsFromISR(g_btif.main_task_event_group,
											BTIF_TASK_EVENT_DRIVER_OWN, &xHigherPriorityTaskWoken);
		if (xResult != pdFAIL) {
			taskEXIT_CRITICAL_FROM_ISR(savedIrqStatus);
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			return;
		}
	} else {
		// Print error message
		BTIF_LOG_E("%s: event_group or main_task did not created", __func__);
		btif_irq_ctrl(BTIF_WAKEUP_IRQ_ID, TRUE);
#ifdef MTK_BT_DRV_CHIP_RESET
		btmtk_reset_notify(pdTRUE);
#endif /* #ifdef MTK_BT_DRV_CHIP_RESET */
	}
	taskEXIT_CRITICAL_FROM_ISR(savedIrqStatus);
}

void _btif_hw_init(struct mtk_btif *p_btif)
{
	unsigned int base = 0;

	if (p_btif->p_base == NULL) {
		BTIF_LOG_E("%s, p_base is NULL", __func__);
		return;
	}
	base = (unsigned int)p_btif->p_base;
	/*set to normal mode*/
	BTIF_WRITE32(BTIF_FAKELCR(base), BTIF_FAKELCR_NORMAL_MODE);
	/*set to newhandshake mode*/
	BTIF_SET_BIT(BTIF_HANDSHAKE(base), BTIF_HANDSHAKE_EN_HANDSHAKE);
	/*set Rx FIFO clear bit to 1*/
	BTIF_SET_BIT(BTIF_FIFOCTRL(base), BTIF_FIFOCTRL_CLR_RX);
	/*clear Rx FIFO clear bit to 0*/
	BTIF_CLR_BIT(BTIF_FIFOCTRL(base), BTIF_FIFOCTRL_CLR_RX);
	/*set Tx FIFO clear bit to 1*/
	BTIF_SET_BIT(BTIF_FIFOCTRL(base), BTIF_FIFOCTRL_CLR_TX);
	/*clear Tx FIFO clear bit to 0*/
	BTIF_CLR_BIT(BTIF_FIFOCTRL(base), BTIF_FIFOCTRL_CLR_TX);

	/*clear LOOPBACK clear bit to 0*/
	BTIF_CLR_BIT(BTIF_TRI_LVL(base), BTIF_TRI_LOOP_EN);

	/*set BTIF TX mode(DMA or PIO)*/
	btif_tx_mode_ctrl(p_btif);
	/*set BTIF RX mode(DMA or PIO)*/
	btif_rx_mode_ctrl(p_btif);
	/*auto reset*/
	BTIF_SET_BIT(BTIF_DMA_EN(base), BTIF_DMA_EN_AUTORST_EN);
	/*disable Tx IER*/
	btif_tx_ier_ctrl(p_btif, FALSE);
	/*disable Rx IER*/
	btif_rx_ier_ctrl(p_btif, FALSE);
}

static int _btif_is_tx_allow(void)
{
	int b_ret = FALSE;
	unsigned int lsr = 0;

	/*read LSR and check THER or TEMT, either one is 1 means can accept tx data*/
	lsr = BTIF_READ32(BTIF_LSR(g_btif.p_base));
	if (!(lsr & (BTIF_LSR_TEMT_BIT | BTIF_LSR_THRE_BIT))) {
		BTIF_LOG_D("%s TX not ready, wait for 1 tick", __func__);
		btif_util_task_delay_ticks(1);
	}
	lsr = BTIF_READ32(BTIF_LSR(g_btif.p_base));
	b_ret = (lsr & (BTIF_LSR_TEMT_BIT | BTIF_LSR_THRE_BIT)) ? TRUE : pdFAIL;
	if (!b_ret)
		BTIF_LOG_W("tx is not allowed");

	return b_ret;
}

static int _btif_pio_write(const unsigned char *p_buf, unsigned int buf_len)
{
	unsigned int i_ret = 0;
	unsigned int sent_len = 0;
	unsigned int left_len = 0;
	unsigned int retry = 0;
	unsigned int max_tx_retry = 20;

	while (sent_len < buf_len) {
		if (_btif_is_tx_allow()) {
			retry = 0;
			unsigned char ava_len = 0;
			unsigned int lsr = BTIF_READ32(BTIF_LSR(g_btif.p_base));

			if (lsr & BTIF_LSR_TEMT_BIT) {
				/* Tx FIFO is empty, we can write tx FIFO count to BTIF */
				ava_len = BTIF_TX_FIFO_SIZE;
			} else if (lsr & BTIF_LSR_THRE_BIT) {
				/*
				 * Tx FIFO is under threshold, we can write
				 * (Tx FIFO count - Tx threshold) to BTIF
				 */
				ava_len = BTIF_TX_FIFO_SIZE - BTIF_TX_FIFO_THRE;
			}
			left_len = buf_len - ava_len;
			/* ava_len will be real length will write to BTIF THR */
			ava_len = ava_len > left_len ? left_len : ava_len;
			sent_len += ava_len;

			// start to write byte by byte
			while (ava_len--) {
				unsigned int value = 0x000000FF & (*(p_buf++));

				BTIF_WRITE32(BTIF_THR(g_btif.p_base), value);
			}
			// enable tx IRQ
			btif_tx_ier_ctrl(&g_btif, TRUE);
		} else if (++retry > max_tx_retry) {
			BTIF_LOG_E("%s exceed retry times limit :%d", __func__, retry);
			break;
		}
	}
	i_ret = sent_len;
	return i_ret;
}

int BTIF_write(const unsigned char *p_buf, unsigned int buf_len)
{
	int i_ret = 0;

	BTIF_LOG_V("%s, len = %d", __func__, buf_len);

	if (!_is_btif_enabled()) {
		BTIF_LOG_E("BTIF is not enabled\n");
		return -1;
	}

	if (!g_btif.ownship_semaphore)
		return -1;

	// Use dma_tx_semaphore to make sure previous TX is done
	if (g_btif.tx_mode == BTIF_MODE_DMA &&
		((struct btif_dma_t *)(g_btif.p_tx_dma))->dma_tx_semaphore) {
		BTIF_LOG_V("%s, Get Tx Semaphore", __func__);
		// dma_tx_sempaphore will be given as Tx done isr is returned to
		// make sure tx can transfer one by one.
		if (xSemaphoreTake(((struct btif_dma_t *)(g_btif.p_tx_dma))->dma_tx_semaphore,
							pdMS_TO_TICKS(TX_WRITE_TIMEOUT)) == pdTRUE) {
			if (xSemaphoreTake(g_btif.main_task_mtx, portMAX_DELAY) == pdFALSE)
				BTIF_LOG_V("%s, main_task_mtx take failed", __func__);
			taskENTER_CRITICAL();
			btif_set_tx_status(TX_PROCESSING);
			taskEXIT_CRITICAL();
			xSemaphoreGive(g_btif.main_task_mtx);
		} else {
			BTIF_LOG_E("%s: WAIT dma_tx_semaphore timeout!", __func__);
			return -1;
		}
	}

	/*
	 * This event is used for reset timer of main_task.
	 * It should wait a period of main_task_timeout with no TxRx behavior then set FW OWN
	 * So we need to send this event for every Tx to make this mechanism work.
	 * But if main_task_timeout is 0, then do not need to send this event.
	 */
	if (btif_task_get_timeout_setting() != 0)
		xEventGroupSetBits(g_btif.main_task_event_group, BTIF_TASK_EVENT_TX);

	if (xSemaphoreTake(g_btif.main_task_mtx, portMAX_DELAY) == pdFALSE)
		BTIF_LOG_V("%s, main_task_mtx get failed", __func__);

	if (btmtk_get_own_type() == FW_OWN) {
		// TODO : How long to wait
		xSemaphoreGive(g_btif.main_task_mtx);

		xEventGroupSetBits(g_btif.main_task_event_group, BTIF_TASK_EVENT_TX_WAIT);
		// Use ownship_semaphore to make sure driver own is successfully set
		if (xSemaphoreTake(g_btif.ownship_semaphore, portMAX_DELAY) == pdTRUE) {
			// TODO: How long to wait
			BTIF_LOG_V("Get ownship_semaphore!");
		} else {
			BTIF_LOG_E("WAIT ownship_semaphore timeout!");
		}
	} else {
		xSemaphoreGive(g_btif.main_task_mtx);
	}

	if (g_btif.tx_mode == BTIF_MODE_DMA) {
#ifdef BTIF_MAIN_TASK_TRACE
		btif_main_task_record_write(pdFALSE, RECORD_SYMBOL_INVALID, RECORD_SYMBOL_SEND_TX, (uint8_t *)p_buf, buf_len);
#endif
		i_ret = btif_dma_tx_write(g_btif.p_tx_dma, p_buf, buf_len);
	} else
		i_ret = _btif_pio_write(p_buf, buf_len);

	return i_ret;
}

unsigned int BTIF_read(unsigned char *buffer, unsigned int buf_len)
{
	unsigned int length = 0;
	unsigned int copyLen = 0;
	/*get BTIF rx buffer's information*/
	struct btif_buf_str_t *p_bbs = &(g_btif.btif_buf);
	/*
	 * wr_idx of btif_buf may be modified in IRQ handler,
	 * in order not to be effected by case in which irq interrupt this operation,
	 * we record wr_idx here
	 */
	unsigned int wr_idx = p_bbs->wr_idx;

	length = BBS_COUNT_CUR(p_bbs, wr_idx);
	BTIF_LOG_V("%s, length = %d", __func__, length);
	/*make sure length of rx buffer data > 0*/
	if (length > 0) {
		if (p_bbs->rd_idx < wr_idx) {
			copyLen = buf_len > length ? length : buf_len;
			memcpy(buffer, BBS_PTR(p_bbs, p_bbs->rd_idx), copyLen);
			p_bbs->rd_idx += copyLen; // update rx data read index
		} else {
			unsigned int tail_len = BBS_SIZE(p_bbs) - (p_bbs)->rd_idx;

			if (tail_len > buf_len) {
				copyLen = buf_len;
				memcpy(buffer, BBS_PTR(p_bbs, p_bbs->rd_idx), copyLen);
				p_bbs->rd_idx += copyLen; // update rx data read index
			} else {
				/* part 1: copy whole tailLen */
				memcpy(buffer, BBS_PTR(p_bbs, p_bbs->rd_idx), tail_len);
				/* part 2: check if head length is enough */
				copyLen = buf_len - tail_len;
				copyLen = (wr_idx < copyLen) ? wr_idx : copyLen;
				if (copyLen)
					memcpy(buffer += tail_len, BBS_PTR(p_bbs, 0), copyLen);
				/* Update read_p final position */
				p_bbs->rd_idx = copyLen;
				/* update return length: head + tail */
				copyLen += tail_len;
			}
		}
	} else {
		// BTIF_LOG_I("length:%d, no RX data", length);
	}
	return copyLen;
}

void btif_buffer_reset(void)
{
	struct btif_dma_t *p_tx_dma;
	struct btif_dma_t *p_rx_dma;

	p_tx_dma = (struct btif_dma_t *)g_btif.p_tx_dma;
	if (p_tx_dma) {
		p_tx_dma->rpt = 0;
		p_tx_dma->last_rpt_wrap = 0;
		p_tx_dma->wpt = 0;
		p_tx_dma->last_wpt_wrap = 0;
	}

	p_rx_dma = (struct btif_dma_t *)g_btif.p_rx_dma;
	if (p_rx_dma) {
		p_rx_dma->rpt = 0;
		p_rx_dma->last_rpt_wrap = 0;
		p_rx_dma->wpt = 0;
		p_rx_dma->last_wpt_wrap = 0;
	}

	g_btif.btif_buf.p_buf = _btif_rx_buf;
	BBS_INIT(&(g_btif.btif_buf));
}

void BTIF_suspend(void)
{
	btif_buffer_reset();
	btif_dma_ctrl_disable((struct btif_dma_t *)g_btif.p_tx_dma);
	btif_dma_ctrl_disable((struct btif_dma_t *)g_btif.p_rx_dma);
}

void BTIF_resume(void)
{
	/*
	 * BTIF Soft reset
	 * 1. Set 0x30030120[21] as 0
	 * 2. Wait a period
	 * 3. Set 0x30030120[21] as 1
	 */
	uint32_t val = BTIF_READ32(0x30030120);

	BTIF_WRITE32(0x30030120, val & ~0x200000);

	btif_rx_ier_ctrl(&g_btif, TRUE);

	btif_dma_hw_init((struct btif_dma_t *)g_btif.p_tx_dma);
	btif_dma_ctrl_enable((struct btif_dma_t *)g_btif.p_tx_dma);
	btif_tx_mode_ctrl(&g_btif);

	btif_dma_hw_init((struct btif_dma_t *)g_btif.p_rx_dma);
	btif_dma_ctrl_enable((struct btif_dma_t *)g_btif.p_rx_dma);
	btif_rx_mode_ctrl(&g_btif);

	/*Part of BTIF Soft reset*/
	val = BTIF_READ32(0x30030120);
	BTIF_WRITE32(0x30030120, val | 0x200000);

	/*BTIF init*/
	_btif_hw_init(&g_btif);
}

int BTIF_open(void)
{
	int i_ret = 0;

	BTIF_LOG_D("%s start", __func__);
	if (_is_btif_enabled() == TRUE) {
		BTIF_LOG_I("btif is already initialized");
		return 0;
	}

	g_btif.p_tx_dma = btif_dma_info_get(DMA_DIR_TX);
	g_btif.p_rx_dma = btif_dma_info_get(DMA_DIR_RX);

#ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET
	// create main task timeout setting mutex
	main_task_timeout_mtx = xSemaphoreCreateMutex();
	if (main_task_timeout_mtx == NULL) {
		BTIF_LOG_E("%s create mutex fail.", __func__);
		goto err_exit;
	}
#endif /* #ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET */
	// create btif main task
	g_btif.ownship_semaphore = xSemaphoreCreateBinary();
	if (g_btif.ownship_semaphore == NULL) {
		BTIF_LOG_E("cannot create ownship_semaphore");
		goto err_exit;
	}
	g_btif.main_task_mtx = xSemaphoreCreateMutex();
	if (g_btif.main_task_mtx == NULL) {
		BTIF_LOG_E("cannot create main_task_mtx");
		goto err_exit;
	}
	g_btif.main_task_event_group = xEventGroupCreate();
	if (g_btif.main_task_event_group == NULL) {
		BTIF_LOG_E("cannot create main_task_event_group");
		goto err_exit;
	}
	g_btif.main_task_hdl = NULL;
	if (pdPASS != xTaskCreate(btif_main_task, "btif_main",
							  (1024 * 4) / sizeof(StackType_t), &g_btif,
							  configMAX_PRIORITIES - 2, &g_btif.main_task_hdl)) {
		BTIF_LOG_E("cannot create btif_main_task");
		goto err_exit;
	}
	BTIF_LOG_D("Create btif_main_task success");

	/*memory allocate*/
	i_ret = _btif_buffer_init();
	if (i_ret < 0) {
		BTIF_LOG_E("BTIF init error return @ 1");
		goto err_exit;
	}

	// power on BTIF module and enable clock for BTIF, clock is enabled by default TODO
	btif_clk_ctrl(TRUE);

	/*basic HW init*/
	_btif_hw_init(&g_btif);

	btif_platform_request_irq(BTIF_IRQ_ID, _btif_irq_handler, "btif_irq", &g_btif);
	btif_platform_request_irq(BTIF_WAKEUP_IRQ_ID, _btif_wakeup_irq_handler,
							  "btif_wakeup_irq", &g_btif);
	/*enable Rx IER by default*/
	btif_rx_ier_ctrl(&g_btif, TRUE);

	/*TX mode setup: DMA or PIO*/
	i_ret = _btif_tx_setup(&g_btif);
	if (i_ret < 0) {
		BTIF_LOG_E("BTIF init error return @ 2");
		goto err_exit;
	}

	/*RX mode setup: DMA or PIO*/
	i_ret = _btif_rx_setup(&g_btif);
	if (i_ret < 0) {
		BTIF_LOG_E("BTIF init error return @ 3");
		goto err_exit;
	}

	g_btif.enabled = TRUE;
	g_btif_rx_data_lose = pdFALSE;
	g_buf_high_cnt = 0;
	BTIF_LOG_I("%s end success", __func__);
	return 0;

err_exit:
	BTIF_close();
	return i_ret;
}

void BTIF_close(void)
{
	BTIF_LOG_I("%s start", __func__);
	if (!_is_btif_enabled()) {
		BTIF_LOG_I("%s btif is not opened yet", __func__);
		return;
	}
	if (g_btif.main_task_hdl) {
		vTaskDelete(g_btif.main_task_hdl);
		g_btif.main_task_hdl = NULL;
	}
	if (g_btif.main_task_event_group) {
		vEventGroupDelete(g_btif.main_task_event_group);
		g_btif.main_task_event_group = NULL;
	}
	if (g_btif.ownship_semaphore) {
		vSemaphoreDelete(g_btif.ownship_semaphore);
		g_btif.ownship_semaphore = NULL;
	}
	if (g_btif.main_task_mtx) {
		vSemaphoreDelete(g_btif.main_task_mtx);
		g_btif.main_task_mtx = NULL;
	}
#ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET
	if (main_task_timeout_mtx) {
		vSemaphoreDelete(main_task_timeout_mtx);
		main_task_timeout_mtx = NULL;
	}
#endif /* #ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET */
	btif_clk_ctrl(FALSE);

	btif_platform_free_irq(g_btif.irq_id);

	_btif_tx_free(&g_btif);
	_btif_rx_free(&g_btif);
	_btif_buffer_deinit();

	g_btif.enabled = FALSE;
	g_btif_rx_data_lose = pdFALSE;
	g_buf_high_cnt = 0;

	BTIF_LOG_I("%s end", __func__);
}

/*---------------------------------------------------------------------------*/
