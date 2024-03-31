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

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "semphr.h"

#include <stdlib.h>
#include <string.h>
#ifdef CHIP_MT7933
#include "btif_mt7933.h"
#include "mt7933_pos.h"
#else
#include "wmt.h"
#include "wmt_core.h"
#include "bt_vendor.h"
#endif
#include "bt_driver.h"
#ifdef MTK_BT_BUFFER_BIN_MODE
#include "bt_buffer_mode.h"
#endif
#ifdef BT_DRV_BTSNOOP_TO_UART
#include "bt_driver_btsnoop.h"
#endif
#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager_internal.h"
#endif
#include "hal_nvic.h"

#define BT_DRV_VERSION "BT_1.0.0_23011901"

// wait coredump times
#define BT_DRV_WAIT_DUMP_CNT 150  // 150 times
#define BT_DRV_WAIT_DUMP_INTERVAL 200  // 200ms

static unsigned short debug_hci_opcode;
static const char * const g_pkt_type_str[] = { "rx_ACL", "rx_SCO", "rx_EVT", "rx_ISO" };

/************Four driver API for HB stack***********************/
int bt_driver_stack_recv(unsigned char *packet);
// This API shall be implemented by HB driver
#pragma weak bt_driver_stack_recv = default_bt_driver_stack_recv
/************Four driver API for HB stack end*******************/

#ifdef CHIP_MT7933
SemaphoreHandle_t gBTConnsysCalLock;
SemaphoreHandle_t driver_bt_state_mtx;
//SemaphoreHandle_t gConnsysRadioOnLock;
#ifdef MTK_BT_ENABLE
extern uint8_t g_btstack_is_running;
extern void bt_rx_handler(void);
extern void bt_fw_assert_handler(void);
#endif
#endif

static int bt_drv_init_done;
static enum bt_driver_state_t bt_driver_st = BT_DRIVER_STATE_UNKNOWN;

EventGroupHandle_t wakeup_events;
// the corresponding event for debug hci cmd comes
#define BIT_DBG_EVENT_ARRIVE (1UL << 0UL)

static int _bt_is_open;
static int _bt_is_dlfw;
/*
 * To support multiple upper rx task, they may switch dynamiclly to receive
 * data, always trigger the curr_data_ready_cb.
 * However, we shall to save the last_data_ready_cb for restoring.
 */
static bt_rx_data_ready_cb curr_data_ready_cb;
static bt_rx_data_ready_cb last_data_ready_cb;
xSemaphoreHandle data_ready_cb_mutex;
static bt_state_change_cb state_change_cb;

struct bt_rx_packet_t rx_packet;

static inline void _bt_driver_reset_rx_pkt_buf(void)
{
	BT_DRV_LOGV("%s", __func__);
	memset(rx_packet.buf, 0, RX_PACKET_BUF_LEN);
	rx_packet.valid_len = 0;
	rx_packet.exp_len = 1; // expect packet indicator
}

static int _bt_driver_read(unsigned char *buf, unsigned int buf_len)
{
	int readLen = 0;

	if (NULL == buf || 0 == buf_len) {
		BT_DRV_LOGE("%s invalid args", __func__);
		return -1;
	}

#ifdef CHIP_MT7933
	readLen = btmtk_read(buf, buf_len);
#else
	readLen = wmt_exp_read(buf, buf_len, WMTDRV_TYPE_BT);
#endif
	return readLen;
}

int bt_driver_trigger_controller_codedump(void)
{
#ifdef CHIP_MT7933
	return btmtk_trigger_fw_assert();
#else
	wmt_exp_trigger_assert();
	return 0;
#endif
}

static void bt_driver_set_bt_state(int state)
{
	if (driver_bt_state_mtx == NULL) {
		BT_DRV_LOGE("%s, driver_bt_state_mtx did not created", __func__);
		return;
	}
	if (xSemaphoreTake(driver_bt_state_mtx, portMAX_DELAY) == pdTRUE) {
		_bt_is_open = state;
		xSemaphoreGive(driver_bt_state_mtx);
	}
	BT_DRV_LOGI("%s, st: %d", __func__, state);
}

bool bt_driver_is_on(void)
{
	bool ret = pdFALSE;

	if (!driver_bt_state_mtx) {
		BT_DRV_LOGD("%s, driver_bt_state_mtx did not created", __func__);
		return ret;
	}

	if (xSemaphoreTake(driver_bt_state_mtx, portMAX_DELAY) == pdTRUE) {
		if (_bt_is_open)
			ret = pdTRUE;
		xSemaphoreGive(driver_bt_state_mtx);
	}

	return ret;
}

/*
 * try to receive a complete packet
 * return  - TRUE: A complete packet is received and stored in rx_packet.buf
 *           FALSE: Other cases
 */
int _bt_driver_packet_recv(void)
{
	int readLen = 0;
	int ret = 0;

	do {
		if ((rx_packet.exp_len + rx_packet.valid_len) >= RX_PACKET_BUF_LEN) {
			/*
			 * Driver only allloc 3K buf, RX_PACKET_BUF_LEN, to handle HCI packet.
			 * If over this len, we should check w/ FW owner.
			 */
			BT_DRV_LOGE("Fatal Error, Invalid len (%d) at pkt type: 0x%x, flush rx buffer",
						 rx_packet.exp_len, rx_packet.buf[0]);
			btmtk_flush_rx_queue();
			_bt_driver_reset_rx_pkt_buf();
			ret = -1;
			break;
		}
		readLen = _bt_driver_read(&rx_packet.buf[rx_packet.valid_len], rx_packet.exp_len);
		if (readLen <= 0) { // no data
			ret = -1;
			break;
		}
		rx_packet.valid_len += readLen;
		rx_packet.exp_len -= readLen;
		if (rx_packet.buf[0] == HCI_PKT_EVT) {
			if (rx_packet.valid_len == 1)
				rx_packet.exp_len = 2; // expect length info
			else if (rx_packet.valid_len == 3)
				rx_packet.exp_len = rx_packet.buf[2]; // expect payload length
		} else if (rx_packet.buf[0] == HCI_PKT_ACL) {
			if (rx_packet.valid_len == 1)
				rx_packet.exp_len = 4; // expect length info
			else if (rx_packet.valid_len == 5) {
				// expect payload length
				rx_packet.exp_len = (((unsigned int)rx_packet.buf[4]) << 8) + rx_packet.buf[3];
			}
		} else if (rx_packet.buf[0] == HCI_PKT_ISO) {
			if (rx_packet.valid_len == 1)
				rx_packet.exp_len = 4;
			else if (rx_packet.valid_len == 5)
				rx_packet.exp_len = (((unsigned int)(rx_packet.buf[4] & 0x3F)) << 8) + rx_packet.buf[3];
		} else {
			BT_DRV_LOGE("Fatal Error, Unknown pkt type: 0x%x, flush rx buffer", rx_packet.buf[0]);
#ifdef CHIP_MT7933
			btmtk_flush_rx_queue();
#else
			wmt_exp_flush_rx_queue(WMTDRV_TYPE_BT);
#endif
			_bt_driver_reset_rx_pkt_buf();
			ret = -1;
			break;
		}

		if (rx_packet.exp_len == 0) { // a complete packet is received
			uint8_t type_index = rx_packet.buf[0] - HCI_PKT_ACL;

			btif_util_dump_buffer(g_pkt_type_str[type_index], rx_packet.buf,
								  rx_packet.valid_len > 16 ? 16 : rx_packet.valid_len, 0);
			ret = 0;
			break;
		}
	} while (readLen > 0);

	if (ret < 0)
		return pdFALSE;
	else
		return pdTRUE;
}

static void _bt_driver_data_ready(void) // rx data ready callback
{
	int complete_packet = pdFALSE;

	// bt_hci_notify();
	if (curr_data_ready_cb) {
		BT_DRV_LOGV("RX ready, notify consumer");
		curr_data_ready_cb();
		return;
	}

	// no consumer, read and parse all the data, and then drop it.
	do {
		if (xSemaphoreTake(rx_packet.mtx, portMAX_DELAY) == pdFALSE) {
			BT_DRV_LOGE("%s, rx_packet.mtx get failed!", __func__);
			break;
		}

		complete_packet = _bt_driver_packet_recv();
		if (complete_packet == pdTRUE) {
			BT_DRV_LOGW("no comsumer, drop all the rx packet");
			if ((rx_packet.buf[0] == HCI_PKT_EVT) && (rx_packet.buf[1] == 0x0E) &&
			    (debug_hci_opcode == (rx_packet.buf[4] | (rx_packet.buf[5] << 8)))) {
				// Command complete event for debug CMD
				BT_DRV_LOGW("The debug event 0x%04x is filtered", debug_hci_opcode);
				btif_util_dump_buffer("DBG EVENT", rx_packet.buf, rx_packet.valid_len, 1);
				debug_hci_opcode = 0;
				_bt_driver_reset_rx_pkt_buf();
				xEventGroupSetBits(wakeup_events, BIT_DBG_EVENT_ARRIVE);
			}
			_bt_driver_reset_rx_pkt_buf();
		}
		xSemaphoreGive(rx_packet.mtx);
	} while (pdTRUE == complete_packet);
}

void bt_driver_register_state_change_cb(bt_state_change_cb cb, int *curr_state)
{
	BT_DRV_LOGW("%s, cb: %p, current_state: %d", __func__, cb, _bt_is_open);
	state_change_cb = cb;
	*curr_state = _bt_is_open;
}

int bt_driver_register_event_cb(bt_rx_data_ready_cb cb, int restore_cb)
{
	if (!bt_drv_init_done) {
		BT_DRV_LOGE("%s bt is not init yet", __func__);
		return -1;
	}

	if (xSemaphoreTake(data_ready_cb_mutex, portMAX_DELAY) == pdFALSE) {
		BT_DRV_LOGE("%s, data_ready_cb_mutex get failed", __func__);
		return -1;
	}
	if (restore_cb) {
		BT_DRV_LOGW("restore event cb from %p -> %p", curr_data_ready_cb, last_data_ready_cb);
		curr_data_ready_cb = last_data_ready_cb;
	} else {
		BT_DRV_LOGW("update event cb from %p -> %p", curr_data_ready_cb, cb);
		last_data_ready_cb = curr_data_ready_cb;
		curr_data_ready_cb = cb;
	}
	_bt_driver_reset_rx_pkt_buf();
#ifdef CHIP_MT7933
	btmtk_flush_rx_queue();
#else
	wmt_exp_flush_rx_queue(WMTDRV_TYPE_BT);
#endif
	xSemaphoreGive(data_ready_cb_mutex);
	BT_DRV_LOGW("%s, end", __func__);
	return 0;
}

static int _bt_driver_power_on(void)
{
#ifdef CHIP_MT7933
	if (bt_drv_init_done && !bt_driver_is_on() &&
		bt_driver_st == BT_DRIVER_STATE_SUSPENDED) {
		// we should do dma hw init after power off
		bt_driver_st = BT_DRIVER_STATE_RESUMING;
		btmtk_resume();
		bt_driver_st = BT_DRIVER_STATE_RESUMED;
	}
	if (bt_driver_init() != 0) {
		BT_DRV_LOGE("%s, drv init fail", __func__);
		return -1;
	}
#ifdef MTK_BT_ENABLE
/*
 * bt_rx_handler() is implemented in HB (libbtdriver_7933.a),
 * so we need use MTK_BT_ENABLE to define
 */
	if (g_btstack_is_running) {
		BT_DRV_LOGI("%s: register driver callback for stack", __func__);
		bt_driver_register_event_cb(bt_rx_handler, 0);
	}
#endif
	if (bt_driver_is_on() == pdTRUE) {
		BT_DRV_LOGI("%s, BT is ON currently", __func__);
		return 0;
	}

	if (bt_driver_dlfw() != 0)
		return -1;

	if (bt_driver_func_on() != 0)
		return -1;

	if (btmtk_is_country_code_set() == 0)
		btmtk_set_bt_power_by_country_code("WW"); // WW: worldwide
#ifdef MTK_BT_DRV_AUTO_PICUS
	btmtk_enable_picus_log();
#endif /* #ifdef MTK_BT_DRV_AUTO_PICUS */
#else
	int ret = 0;

	if (bt_driver_init() != 0) {
		BT_DRV_LOGE("%s, init fail", __func__);
		return -1;
	}

	if (bt_driver_is_on() == pdTRUE) {
		BT_DRV_LOGE("%s, BT is ON currently", __func__);
		return 0;
	}
	if (!wmt_exp_is_init_done()) {
		BT_DRV_LOGE("%s, WMT is not init yet!!!", __func__);
		return -1;
	}
	// wmt&stp driver is init by default
	if (wmt_func_bt_on() != 0) {
		BT_DRV_LOGE("%s, BT ON fail", __func__);
		return -1;
	}
	wmt_exp_bt_register_event_cb(_bt_driver_data_ready);
	bt_driver_set_bt_state(BT_ON);
	if (state_change_cb)
		state_change_cb(pdTRUE);
	ret = bt_vendor_init_device(0x6631);
	if (ret == 0)
		BT_DRV_LOGI("%s, BT ON success", __func__);
	else {
		BT_DRV_LOGE("%s, BT ON fail", __func__);
		wmt_exp_bt_register_event_cb(NULL);
		bt_driver_set_bt_state(BT_OFF);
		if (state_change_cb)
			state_change_cb(pdFALSE);
		return -1;
	}
#endif
	return 0;
}

static int _bt_driver_power_off(void)
{
	int ret = 0;

	BT_DRV_LOGI("%s, driver_st: %d", __func__, bt_driver_is_on());

#ifdef CHIP_MT7933
	if (bt_driver_is_on() == pdFALSE) {
		if (btmtk_get_bgfsys_power_state()) {
			ret = btmtk_bgfsys_power_off();
			if (ret) {
				BT_DRV_LOGE("%s, bgfsys power off fail, err: %d", __func__, ret);
				return -1;
			}
		} else
			BT_DRV_LOGE("%s, BT is OFF currently", __func__);
		_bt_is_dlfw = DLFW_NONE;
		return 0;
	}

	// share notify before doing func off, incase that some cmds have to be sent
	if (state_change_cb)
		state_change_cb(pdFALSE);

	bt_driver_set_bt_state(BT_OFF);

#ifdef MTK_BT_DRV_CHIP_RESET
	if ((btmtk_get_chip_reset_state() != SUBSYS_CHIP_RESET_START) &&
		(btmtk_get_chip_reset_state() != WHOLE_CHIP_RESET_START))
#endif /* #ifdef MTK_BT_DRV_CHIP_RESET */
	{
		/* when powering off process, no need to judge the return value, 
		 * whether success or fail, also should do bgfsys power off
		 */
		btmtk_func_ctrl(0);
	}

	/* Disable fw soft irq, and check whether now is coredumping. If it is now,
	 * wait until dumping end or interrupt from dumping data each 200ms till timeout 30s.
	 * Use wait_fw_dump_over which is set when parsing bt config to judge whether dump fw coredump file completely.
	 */
	hal_nvic_disable_irq(CONN2AP_SW_IRQn);
	if (btmtk_get_coredump_state() == BT_FW_COREDUMP_START) {
		uint16_t cnt = 0;
		while ((cnt < BT_DRV_WAIT_DUMP_CNT) && (btmtk_get_coredump_state() != BT_FW_COREDUMP_END)) {
			vTaskDelay(pdMS_TO_TICKS(BT_DRV_WAIT_DUMP_INTERVAL));
			if ((cnt & 0xF) == 0)
				BT_DRV_LOGW("%s, wait fw dumping return(%dms)", __func__, cnt * BT_DRV_WAIT_DUMP_INTERVAL);
			cnt++;
		}
	}
	btmtk_set_coredump_state(BT_FW_COREDUMP_UNKNOWN);

	ret = btmtk_close();
	if (ret) {
		BT_DRV_LOGE("%s, btmtk close fail", __func__);
		return -1;
	}
	bt_driver_st = BT_DRIVER_STATE_SUSPENDING;
	btmtk_suspend();
	bt_driver_st = BT_DRIVER_STATE_SUSPENDED;
// after power off, own type is driver own, so sleep lock does not be released in main task
#ifdef HAL_SLEEP_MANAGER_ENABLED
	if (hal_sleep_manager_get_lock_status() & (1 << SLEEP_LOCK_BT))
		hal_sleep_manager_unlock_sleep(SLEEP_LOCK_BT);
#endif
#else // MT8512
	if (bt_driver_is_on() == pdFALSE) {
		BT_DRV_LOGE("%s, BT is OFF currently", __func__);
		return 0;
	}

	// share notify before doing func off, incase that some cmds have to be sent
	if (state_change_cb)
		state_change_cb(pdFALSE);

	ret = wmt_func_bt_off();
	if (ret != 0) {
		BT_DRV_LOGE("%s, wmt bt OFF fail", __func__);
		return -1;
	}
#endif
	BT_DRV_LOGI("%s, BT OFF success", __func__);

	_bt_is_dlfw = DLFW_NONE;
	return 0;
}

int bt_driver_func_on(void)
{
	int ret = 0;

	BT_DRV_LOGD("%s", __func__);

#ifdef CHIP_MT7933
	if (gBTConnsysCalLock == NULL) {
		// BT_DRV_LOGI("%s: RadioOnLock is NULL", __func__);
		gBTConnsysCalLock = xSemaphoreCreateMutex();
		if (!gBTConnsysCalLock) {
			BT_DRV_LOGE("%s: Create gBTConnsysCalLock Fail!", __func__);
			return -1;
		}
	}

	if (xSemaphoreTake(gBTConnsysCalLock, portMAX_DELAY) != pdTRUE) {
		BT_DRV_LOGE("Take gBTConnsysCalLock Fail on BT power on!");
		return -1;
	}

	// Do BT FW power on
	ret = btmtk_func_ctrl(1);
	if (ret < 0) {
		BT_DRV_LOGE("%s: btmtk_func_ctrl fail.", __func__);
		xSemaphoreGive(gBTConnsysCalLock);
		return -1;
	}

	xSemaphoreGive(gBTConnsysCalLock);

#ifdef MTK_BT_BUFFER_BIN_MODE
	if (!btmtk_buffer_mode_initialize())
		btmtk_buffer_mode_send();
#endif
	btmtk_send_low_power_cmd();

#endif

	btmtk_register_event_cb(_bt_driver_data_ready);
	if (state_change_cb)
		state_change_cb(pdTRUE);

	bt_driver_set_bt_state(BT_ON);
	BT_DRV_LOGI("%s: success", __func__);
	return ret;
}

int bt_driver_func_off(void)
{
	BT_DRV_LOGI("%s", __func__);
	// share notify before doing func off, in case that some cmds have to be sent
	if (state_change_cb)
		state_change_cb(pdFALSE);
#ifdef CHIP_MT7933
	btmtk_func_ctrl(0);
#endif
	BT_DRV_LOGI("%s: success", __func__);
	return 0;
}

int bt_driver_packet_parser(unsigned char *buf, unsigned int length)
{
	int ret = 0;
	unsigned int i = 0;
	int copy_len = 0;

	if ((buf == NULL) || (length == 0)) {
		BT_DRV_LOGE("%s invalid args", __func__);
		return -1;
	}
	BT_DRV_LOGV("%s start, length: %d, curr pkt valid_len:%d, exp_len:%d",
				__func__, length, rx_packet.valid_len, rx_packet.exp_len);

	if (xSemaphoreTake(rx_packet.mtx, portMAX_DELAY) == pdFALSE) {
		BT_DRV_LOGE("%s, rx_packet.mtx get failed!", __func__);
		return -1;
	}

	while (i < length) {
		if ((rx_packet.exp_len + rx_packet.valid_len) > RX_PACKET_BUF_LEN) {
			BT_DRV_LOGE("Fatal Error, Pkt overflow, valid_len:%d, exp_len:%d",
						rx_packet.valid_len, rx_packet.exp_len);
			ret = -1;
			goto out;
		}
		copy_len = rx_packet.exp_len < (length - i) ? rx_packet.exp_len : (length - i);
		memcpy(&rx_packet.buf[rx_packet.valid_len], &buf[i], copy_len);
		rx_packet.valid_len += copy_len;
		rx_packet.exp_len -= copy_len;
		i += copy_len;

		if (rx_packet.buf[0] == HCI_PKT_EVT) {
			if (rx_packet.valid_len == 1)
				rx_packet.exp_len = 2; // expect length info
			else if (rx_packet.valid_len == 3)
				rx_packet.exp_len = rx_packet.buf[2]; // expect payload length
		} else if (rx_packet.buf[0] == HCI_PKT_ACL) {
			if (rx_packet.valid_len == 1) {
				rx_packet.exp_len = 4; // expect length info
			} else if (rx_packet.valid_len == 5) {
				// expect payload length
				rx_packet.exp_len = (((unsigned int)rx_packet.buf[4]) << 8) + rx_packet.buf[3];
			}
		}  else if (rx_packet.buf[0] == HCI_PKT_ISO) {
			if (rx_packet.valid_len == 1)
				rx_packet.exp_len = 4;
			else if (rx_packet.valid_len == 5)
				rx_packet.exp_len = (((unsigned int)(rx_packet.buf[4] & 0x3F)) << 8) + rx_packet.buf[3];
		} else {
			BT_DRV_LOGE("Fatal Error, Unknown pkt type: 0x%x, flush rx buffer", rx_packet.buf[0]);
#ifdef CHIP_MT7933
			btmtk_flush_rx_queue();
#else
			wmt_exp_flush_rx_queue(WMTDRV_TYPE_BT);
#endif
			_bt_driver_reset_rx_pkt_buf();
			ret = -1;
			goto out;
		}

		if (rx_packet.exp_len == 0) { // a complete packet is received
			uint8_t type_index = rx_packet.buf[0] - HCI_PKT_ACL;

			btif_util_dump_buffer(g_pkt_type_str[type_index], rx_packet.buf,
								  rx_packet.valid_len > 16 ? 16 : rx_packet.valid_len, 0);

			if ((rx_packet.buf[0] == HCI_PKT_EVT) && (rx_packet.buf[1] == 0x0E) &&
			    (debug_hci_opcode == (rx_packet.buf[4] | (rx_packet.buf[5] << 8)))) {
				// Command complete event for debug CMD
				BT_DRV_LOGW("The debug event 0x%04x is filtered", debug_hci_opcode);
				btif_util_dump_buffer("DBG EVENT", rx_packet.buf, rx_packet.valid_len, 0);
				debug_hci_opcode = 0;
				xEventGroupSetBits(wakeup_events, BIT_DBG_EVENT_ARRIVE);
			}
			_bt_driver_reset_rx_pkt_buf();
		}
	}
out:
	xSemaphoreGive(rx_packet.mtx);
	BT_DRV_LOGV("%s end", __func__);

	return ret;
}

static int _bt_driver_tx(unsigned char *packet, unsigned int length)
{
	int bytesWritten = 0;
	unsigned int bytesToWrite = length;
#ifdef BT_DRV_BTSNOOP_TO_UART
	unsigned char *pPacket = packet;
#endif

	BT_DRV_LOGV("%s write start", __func__);
	/* Try to send len bytes data in buffer */
	while (bytesToWrite > 0) {
#ifdef CHIP_MT7933
		bytesWritten = btmtk_write(packet, bytesToWrite);
#else
		bytesWritten = wmt_exp_write(packet, bytesToWrite, WMTDRV_TYPE_BT);
#endif
		if (bytesWritten <= 0) {
			BT_DRV_LOGE("%s write fail", __func__);
			return -1;
		}
		bytesToWrite -= bytesWritten;
		packet += bytesWritten;
	}

#ifdef BT_DRV_BTSNOOP_TO_UART
	if (pPacket[0] == HCI_PKT_CMD)
		bt_driver_btsnoop_push_packet(BT_HCI_LOG_CMD, pPacket, length);
	else if (pPacket[0] == HCI_PKT_ACL)
		bt_driver_btsnoop_push_packet(BT_HCI_LOG_ACL_OUT, pPacket, length);
    else if (pPacket[0] == HCI_PKT_ISO)
        bt_driver_btsnoop_push_packet(BT_HCI_LOG_ISO_OUT, pPacket, length);
#endif

	BT_DRV_LOGV("%s write end", __func__);
	return 0;
}

/*
 * This function is for special test case, the corrponding event to
 * the test HCI cmd will be filted by driver
 */
int bt_driver_tx_debug(unsigned char *packet, unsigned int length)
{
	if ((packet == NULL) || (length == 0)) {
		BT_DRV_LOGE("%s, invalid args, length=%d", __func__, length);
		return -1;
	}
	if ((packet[0] == 0x01) && (length > 3)) { // hci cmd
		EventBits_t bits = 0;

		if (length != (unsigned int)(packet[3] + 4)) {
			BT_DRV_LOGE("%s, invalid hci cmd", __func__);
			return -1;
		}
		debug_hci_opcode = (packet[1] | (packet[2] << 8));
		BT_DRV_LOGI("send DBG HCI cmd: 0x%04x", debug_hci_opcode);
		_bt_driver_tx(packet, length);
		bits = xEventGroupWaitBits(wakeup_events, BIT_DBG_EVENT_ARRIVE,
								   pdTRUE, pdFALSE, pdMS_TO_TICKS(3000));
		if (!(bits & BIT_DBG_EVENT_ARRIVE)) {
			BT_DRV_LOGE("Waiting Event: 0x%04x timeout", debug_hci_opcode);
			return -1;
		}
		BT_DRV_LOGI("send DBG HCI cmd success");
		return 0;
	}

	// other packet, just send it.
	// This might not happen, check nothing, send packet anyway
	return _bt_driver_tx(packet, length);
}

static int _bt_driver_rx(unsigned char *buf, unsigned int buf_len)
{
	int bytesRead = 0;

	bytesRead = _bt_driver_read(buf, buf_len);
	BT_DRV_LOGD("%s readLen %d", __func__, bytesRead);
	if (bytesRead > 0 && !curr_data_ready_cb)
		bt_driver_packet_parser(buf, bytesRead);

	return bytesRead;
}

int bt_driver_rx_timeout(unsigned char *buf, unsigned int len)
{
	unsigned char retry = 3;
	int bytesRead = 0;
	unsigned int bytesToRead = len;

	BT_DRV_LOGD("%s read start", __func__);
	/* Try to receive len bytes */
	while (bytesToRead > 0) {
		// try read first
		bytesRead = _bt_driver_read(buf, bytesToRead);
		if (bytesRead == 0) {
			BT_DRV_LOGD("%s no data for read, retry=%d", __func__, retry);
			if (--retry == 0) {
				BT_DRV_LOGW("read timeout, read:%d, target:%d", (len - bytesToRead), len);
				return -1;
			}
			// wait for data ready notify, the caller task shall
			// register the event cb first
			ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
		} else {
			bytesToRead -= bytesRead;
			buf += bytesRead;
		}
	}
	BT_DRV_LOGD("%s read end", __func__);
	return 0;
}

static void _bt_driver_recv(void)
{
	int complete_packet = pdFALSE;

	do {
		if (xSemaphoreTake(rx_packet.mtx, portMAX_DELAY) == pdFALSE) {
			BT_DRV_LOGE("%s, rx_packet.mtx get failed!", __func__);
			break;
		}
		complete_packet = _bt_driver_packet_recv();
		if (complete_packet == pdTRUE) {
			if ((rx_packet.buf[0] == HCI_PKT_EVT) && (rx_packet.buf[1] == 0x0E) &&
			    (debug_hci_opcode == (rx_packet.buf[4] | (rx_packet.buf[5] << 8)))) {
				// Command complete event for debug CMD
				BT_DRV_LOGW("The debug event 0x%04x is filtered", debug_hci_opcode);
				btif_util_dump_buffer("DBG EVENT", rx_packet.buf, rx_packet.valid_len, 1);
				debug_hci_opcode = 0;
				_bt_driver_reset_rx_pkt_buf();
				xEventGroupSetBits(wakeup_events, BIT_DBG_EVENT_ARRIVE);
			} else {
				// Send to HB
				bt_driver_stack_recv(rx_packet.buf);

#ifdef BT_DRV_BTSNOOP_TO_UART
				if (rx_packet.buf[0] == HCI_PKT_EVT)
					bt_driver_btsnoop_push_packet(BT_HCI_LOG_EVENT, rx_packet.buf,
												  rx_packet.valid_len);
				else if (rx_packet.buf[0] == HCI_PKT_ACL)
					bt_driver_btsnoop_push_packet(BT_HCI_LOG_ACL_IN, rx_packet.buf,
												  rx_packet.valid_len);
				else if (rx_packet.buf[0] == HCI_PKT_ISO)
					bt_driver_btsnoop_push_packet(BT_HCI_LOG_ISO_IN, rx_packet.buf,
												  rx_packet.valid_len);
#endif
			}
			_bt_driver_reset_rx_pkt_buf();
		}
		xSemaphoreGive(rx_packet.mtx);
	} while (pdTRUE == complete_packet);
}

static int default_bt_driver_stack_recv(unsigned char *packet)
{
	BT_DRV_LOGD("%s", __func__);
	return pdTRUE;
}

int bt_driver_init(void)
{
	if (bt_drv_init_done) {
		BT_DRV_LOGD("%s already init.", __func__);
		return 0;
	}
	BT_DRV_LOGI("%s, version: %s", __func__, BT_DRV_VERSION);

	_bt_driver_reset_rx_pkt_buf();
	rx_packet.mtx = xSemaphoreCreateMutex();
	if (rx_packet.mtx == NULL) {
		BT_DRV_LOGE("%s create rx_packet.mtx fail", __func__);
		goto init_fail;
	}
	data_ready_cb_mutex = xSemaphoreCreateMutex();
	if (data_ready_cb_mutex == NULL) {
		BT_DRV_LOGE("%s create data_ready_cb_mutex fail", __func__);
		goto init_fail;
	}
	wakeup_events = xEventGroupCreate();
	if (wakeup_events == NULL) {
		BT_DRV_LOGE("%s create wakeup_events fail.", __func__);
		goto init_fail;
	}
	if (driver_bt_state_mtx == NULL) {
		driver_bt_state_mtx = xSemaphoreCreateMutex();
		if (!driver_bt_state_mtx) {
			BT_DRV_LOGE("%s: create driver_bt_state_mtx fail!", __func__);
			goto init_fail;
		}
	}

#ifdef CHIP_MT7933
	if (btmtk_init() != 0)
		goto init_fail;
#endif
#ifdef BT_DRV_BTSNOOP_TO_UART
	bt_driver_btsnoop_init();
#endif
	btmtk_register_event_cb(_bt_driver_data_ready);
#ifdef MTK_BT_ENABLE
	btmtk_fw_assert_cb_register(bt_fw_assert_handler);
#endif
	bt_drv_init_done = 1;
	bt_driver_st = BT_DRIVER_STATE_INIT;
#ifdef HAL_SLEEP_MANAGER_ENABLED
	sleep_management_register_suspend_callback(SLEEP_BACKUP_RESTORE_BT, bt_driver_suspend, NULL);
	sleep_management_register_resume_callback(SLEEP_BACKUP_RESTORE_BT, bt_driver_resume, NULL);
#endif
	BT_DRV_LOGI("%s success.", __func__);
	return 0;

init_fail:
	if (rx_packet.mtx)
		vSemaphoreDelete(rx_packet.mtx);
	rx_packet.mtx = NULL;
	if (data_ready_cb_mutex)
		vSemaphoreDelete(data_ready_cb_mutex);
	data_ready_cb_mutex = NULL;
	if (wakeup_events)
		vEventGroupDelete(wakeup_events);
	wakeup_events = NULL;
	return -1;
}

void bt_driver_deinit(void)
{
	if (rx_packet.mtx)
		vSemaphoreDelete(rx_packet.mtx);
	rx_packet.mtx = NULL;
	if (data_ready_cb_mutex)
		vSemaphoreDelete(data_ready_cb_mutex);
	data_ready_cb_mutex = NULL;
	if (wakeup_events)
		vEventGroupDelete(wakeup_events);
	wakeup_events = NULL;
	if (driver_bt_state_mtx) {
		vSemaphoreDelete(driver_bt_state_mtx);
		driver_bt_state_mtx = NULL;
	}
#ifdef CHIP_MT7933
	btmtk_deinit();
#endif
#ifdef HAL_SLEEP_MANAGER_ENABLED
	sleep_management_register_suspend_callback(SLEEP_BACKUP_RESTORE_BT, NULL, NULL);
	sleep_management_register_resume_callback(SLEEP_BACKUP_RESTORE_BT, NULL, NULL);
#endif
	bt_drv_init_done = 0;
	bt_driver_st = BT_DRIVER_STATE_UNKNOWN;
}

#ifdef CHIP_MT7933
void bt_driver_bgfsys_on(void)
{
	btmtk_bgfsys_power_on();
}

int bt_driver_dlfw(void)
{
	int ret = -1;

	if (bt_driver_is_on() == pdTRUE) {
		BT_DRV_LOGI("%s, BT is opened, do not download fw again", __func__);
		return 0;
	} else if (_bt_is_dlfw == DLFW_DONE) {
		BT_DRV_LOGW("%s, No need download FW again!", __func__);
		return 0;
	} else if (bt_drv_init_done &&
			bt_driver_st == BT_DRIVER_STATE_SUSPENDED) {
		/* we should resume DMA to catch fw response. */
		bt_driver_st = BT_DRIVER_STATE_RESUMING;
		btmtk_resume();
		bt_driver_st = BT_DRIVER_STATE_RESUMED;
	} else {
		bt_driver_init();
	}

	btmtk_set_coredump_state(BT_FW_COREDUMP_INIT);
	// we should clear sw irq to avoid fw error triggering after disabled sw irq
	hal_nvic_clear_pending_irq(CONN2AP_SW_IRQn);
	hal_nvic_enable_irq(CONN2AP_SW_IRQn);

	if (gBTConnsysCalLock == NULL) {
		gBTConnsysCalLock = xSemaphoreCreateMutex();
		if (!gBTConnsysCalLock) {
			BT_DRV_LOGE("%s: create gBTConnsysCalLock fail!", __func__);
			return -1;
		}
	}

#ifdef HAL_SLEEP_MANAGER_ENABLED
	if (hal_sleep_manager_get_lock_status() & (1 << SLEEP_LOCK_BT))
		BT_DRV_LOGW("%s, sleep lock locked in main task", __func__);
	else
		hal_sleep_manager_lock_sleep(SLEEP_LOCK_BT);
#endif
	if (xSemaphoreTake(gBTConnsysCalLock, portMAX_DELAY) == pdTRUE) {
		/* download firmware */
		ret = btmtk_open();
		xSemaphoreGive(gBTConnsysCalLock);
	}
#ifdef HAL_SLEEP_MANAGER_ENABLED
	// do unlock after download fw patch
	if (hal_sleep_manager_get_lock_status() & (1 << SLEEP_LOCK_BT))
		hal_sleep_manager_unlock_sleep(SLEEP_LOCK_BT);
#endif

	if (ret != 0) {
		BT_DRV_LOGE("%s: download firmware failed.", __func__);
		return ret;
	}

	_bt_is_dlfw = DLFW_DONE;

	return 0;
}

void bt_driver_print_version_info(void)
{
	BT_DRV_LOGI("bt driver ver: %s", BT_DRV_VERSION);
	BT_DRV_LOGI("bt fw ver: %s", btmtk_get_fw_version());
}

void bt_driver_set_own_ctrl(unsigned char ctrl)
{
	btmtk_set_own_ctrl(ctrl);
}

uint8_t bt_driver_get_own_ctrl(void)
{
	return btmtk_get_own_ctrl();
}

int bt_driver_set_own_type(unsigned char own_type)
{
	return btmtk_set_own_type(own_type);
}

uint8_t bt_driver_get_own_type(void)
{
	return btmtk_get_local_own_type();
}

void bt_driver_suspend(void *p_data)
{
	if ((bt_driver_st != BT_DRIVER_STATE_INIT) &&
		(bt_driver_st != BT_DRIVER_STATE_RESUMED)) {
		BT_DRV_LOGE("%s, state is %d", __func__, bt_driver_st);
		return;
	}

	if (!_bt_is_open) // Just return if BT is not power on
		return;
	if (bt_driver_get_own_type() != FW_OWN) {
		BT_DRV_LOGE("%s, not fw_own state, error!!!", __func__);
		return;
	}

	if (!btmtk_check_HW_TRX_empty()) {
		BT_DRV_LOGE("%s, HW rx data not empty!!!", __func__);
		return;
	}

	bt_driver_st = BT_DRIVER_STATE_SUSPENDING;
#ifdef CHIP_MT7933
	btmtk_suspend();
#endif
	bt_driver_st = BT_DRIVER_STATE_SUSPENDED;
}

void bt_driver_resume(void *p_data)
{
	if (!_bt_is_open) // Just return if BT is not power on
		return;
	if (bt_driver_st != BT_DRIVER_STATE_SUSPENDED) {
		BT_DRV_LOGE("%s, state is %d", __func__, bt_driver_st);
		return;
	}

	bt_driver_st = BT_DRIVER_STATE_RESUMING;
#ifdef CHIP_MT7933
	btmtk_resume();
#endif
	bt_driver_st = BT_DRIVER_STATE_RESUMED;
}

bool bt_driver_is_ready(void)
{
	if ((bt_driver_st != BT_DRIVER_STATE_INIT) &&
		(bt_driver_st != BT_DRIVER_STATE_RESUMED)) {
		BT_DRV_LOGE("%s, state is %d", __func__, bt_driver_st);
		return pdFALSE;
	}
	return pdTRUE;
}
#endif

/****************************************
 * BTIF APIs to be called by upper layer
 ****************************************/
#ifndef MTK_DRV_VND_LAYER
int bt_driver_power_on(void)
{
	return _bt_driver_power_on();
}

int bt_driver_power_off(void)
{
	return _bt_driver_power_off();
}

int bt_driver_tx(unsigned char *packet, unsigned int length)
{
	return _bt_driver_tx(packet, length);
}

void bt_driver_recv(void)
{
	_bt_driver_recv();
}

int bt_driver_rx(unsigned char *buf, unsigned int buf_len)
{
	return _bt_driver_rx(buf, buf_len);
}

#else
int BTIF_driver_power_on(void)
{
	return _bt_driver_power_on();
}

int BTIF_driver_power_off(void)
{
	return _bt_driver_power_off();
}

int BTIF_driver_tx(unsigned char *packet, unsigned int length)
{
	return _bt_driver_tx(packet, length);
}

void BTIF_driver_recv(void)
{
	_bt_driver_recv();
}

int BTIF_driver_rx(unsigned char *buf, unsigned int buf_len)
{
	return _bt_driver_rx(buf, buf_len);
}

int BTIF_driver_snoop_ctrl(unsigned char enable)
{
#ifdef BT_DRV_BTSNOOP_TO_UART
	bt_driver_btsnoop_ctrl(enable);
#else
	BT_DRV_LOGE("%s, BT_DRV_BTSNOOP_TO_UART is not defined", __func__);
#endif
	return 0;
}
#endif
