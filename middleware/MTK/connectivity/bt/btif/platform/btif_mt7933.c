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
#include <string.h>
#include <stdlib.h>
#include "btif_mt7933.h"
#include "driver_api.h"
#include "btif_main.h"
#include "memory_attribute.h"
#include "mt7933_pos.h"
#include "hal_flash.h"
#include "hal_spm.h"
#include "hal.h"
#include "hal_gpt.h"

#ifdef MTK_NVDM_ENABLE
#include "nvdm.h"
#endif
#ifdef MTK_BT_PICUS_ENABLE
#include "picus.h"
#endif
#include "mt7933_connsys_dbg.h"

#ifdef MTK_BT_AUDIO_PR
#include "apps_debug.h"
#endif

#define OPEN_UART_DMA_MODE

// 1024 (From 1024 to 2048 due to FW has a test case over the 1024)
#define VFIFO_SIZE 2048
#define SEND_THRESHOLD_SIZE 64
#define RECEIVE_THRESHOLD_SIZE 512
#define RECEIVE_ALERT_SIZE 30

static int g_inited_uart_port = -1;
#ifdef OPEN_UART_DMA_MODE
static unsigned int g_vff_mem_addr;
#endif

#define UART_WRITE_RETRY_CNT   10
#define UART_WRITE_RETRY_DELAY 10 // ms
#define RX_BUF_DEBUG_DATA_LEN  16

// send hci cmd retry times and wait time
#define HCI_CMD_EVT_READ_RETRY_CNT 20
#define HCI_CMD_EVT_READ_TOUT 300

#ifdef MTK_SECURE_BOOT_ENABLE
	#include <scott.h>
#else
	#ifdef MTK_BT_SIGN_ENABLE
	#error "MTK_BT_SIGN_ENABLE needs MTK_SECURE_BOOT_ENABLE"
	#endif
#endif

//---------------------------------------------------------------------------
#define POS_POLLING_RTY_LMT 100
#ifdef MTK_BT_FW_DL_TO_EMI_ENABLE
#define BGF_EMI_BUFFER_SIZE (1024 * 352)
ATTR_TEXT_BT_FIRMWARE_IN_EMI uint8_t bgf_emi_buffer[BGF_EMI_BUFFER_SIZE];
#endif
#define DYNAMIC_MAP_MAX_SIZE 0x300000
static struct _Section_Map *g_ilm_data_section_map;
static struct btmtk_ring_buffer_t bt_rx_buffer;
static struct btmtk_pkt_paser_t parser;
static btmtk_event_cb receiver_notify_cb;
static btmtk_fwlog_recv_cb fwlog_recv_cb;
static btmtk_event_cb fw_assert_nty;
static TaskHandle_t hci_cmd_task;
static uint8_t psram_copy = true;
static uint8_t hif_dl = true;
static uint8_t g_bgfsys_on;
static bool bperf_enable = pdFALSE;
static uint32_t bt_fw_start_addr = XIP_BT_START;
static TaskHandle_t btmtk_coredump_task;
static struct bt_driver_info bt_drv_info;

static struct coredump_region *mem_dump_region;
static uint32_t mem_dump_size;
static struct coredump_region *cr_dump_region;
static uint32_t cr_dump_size;
static unsigned int total_coredump_sz;
#ifdef MTK_BT_PICUS_ENABLE
#define CORE_DUMP_PRINT_LINE 20
#endif
#define CORE_DUMP_LINE_SIZE 80
#define CORE_DUMP_LINE_MAX_SIZE 256

/* Flag for ownship */
static uint8_t g_own_control = 1;
static uint8_t g_own_type = DRIVER_OWN;

static unsigned char BT_PROC_ON_CMD[] = {0x01, 0x6F, 0xFC, 0x06, 0x01,
										 0x06, 0x02, 0x00, 0x00, 0x01};
static unsigned char BT_PROC_OFF_CMD[] = {0x01, 0x6F, 0xFC, 0x06, 0x01,
										  0x06, 0x02, 0x00, 0x00, 0x00};
static unsigned char BT_PROC_ON_OFF_EVENT[] = {0x04, 0xE4, 0x05, 0x02,
											   0x06, 0x01, 0x00, 0x00};
static unsigned char BT_RF_CALI_CMD[] = {0x01, 0x6F, 0xFC, 0x06, 0x01,
										 0x14, 0x02, 0x00, 0x01, 0x00};
static unsigned char BT_RF_CALI_EVENT[] = {0x04, 0xE4, 0x06, 0x02, 0x14,
										   0x02, 0x00, 0x00, 0x00};
//static unsigned char BT_PROC_HCI_RESET_CMD[] = {0x01, 0x03, 0x0C, 0x00};
//static unsigned char BT_PROC_HCI_RESET_EVENT[] = {0x04, 0x0E, 0x04, 0x01, 0x03, 0x0C, 0x00};
static unsigned char BT_PROC_COREDUMP_CMD[] = {0x01, 0x5B, 0xFD, 0x00};

#ifdef MTK_BT_DRV_CHIP_RESET
static struct chip_rst_info g_rst_info = {
	.rst_mtx = NULL,
	.rst_hdl = NULL,
	.rst_type = SUBSYS_CHIP_RESET,
	.rst_state = CHIP_RESET_UNKNOWN,
};
#endif /* #ifdef MTK_BT_DRV_CHIP_RESET */

static void btmtk_coredump_init_dump_regions(void);
static void btmtk_coredump_task_main(void *arg);
extern bool bt_driver_is_on(void);

//---------------------------------------------------------------------------
static int rtos_mdelay(unsigned int ms)
{
	const TickType_t xDelay = ms / portTICK_PERIOD_MS;

	vTaskDelay(xDelay);
	return 0;
}

static void _btmtk_reset_parser(void)
{
	memset(parser.hdr, 0, sizeof(parser.hdr));
	parser.buf = NULL;
	parser.valid_len = 0;
	parser.exp_len = 1;
}

static int _btmtk_push_hci_packet(const unsigned char *buffer, unsigned int length)
{
	unsigned int room_left = 0;
	unsigned int last_len = 0;

	btif_util_dump_buffer("hci pkt", buffer, length, 0);

	btif_util_sema_lock(bt_rx_buffer.mtx, 0);
	BTIF_LOG_V("%s start read_p=0x%x, write_p=0x%x, length = %u", __func__,
			   bt_rx_buffer.read_p, bt_rx_buffer.write_p, length);
	if (bt_rx_buffer.read_p <= bt_rx_buffer.write_p)
		room_left = BTMTK_RING_BUFFER_SIZE - bt_rx_buffer.write_p + bt_rx_buffer.read_p - 1;
	else
		room_left = bt_rx_buffer.read_p - bt_rx_buffer.write_p - 1;

	if (room_left < length) {
		btif_util_sema_unlock(bt_rx_buffer.mtx, 0);
		if (HCI_PKT_EVT == buffer[0]) {
			BTIF_LOG_E("Q full! Drop EVT:0x%x len:%u, room:%u",
						buffer[1], length, room_left);
		} else {
			BTIF_LOG_E("Q full! Drop 0x%x: len:%u, room:%u",
						buffer[0], length, room_left);
		}
		return -1;
	}

	if (length + bt_rx_buffer.write_p < BTMTK_RING_BUFFER_SIZE) {
		memcpy(bt_rx_buffer.buffer + bt_rx_buffer.write_p, buffer, length);
		bt_rx_buffer.write_p += length;
	} else {
		last_len = BTMTK_RING_BUFFER_SIZE - bt_rx_buffer.write_p;
		memcpy(bt_rx_buffer.buffer + bt_rx_buffer.write_p, buffer, last_len);
		memcpy(bt_rx_buffer.buffer, buffer + last_len, length - last_len);
		bt_rx_buffer.write_p = length - last_len;
	}
	BTIF_LOG_V("%s end read_p=0x%x, write_p=0x%x", __func__,
			   bt_rx_buffer.read_p, bt_rx_buffer.write_p);

	btif_util_sema_unlock(bt_rx_buffer.mtx, 0);

	if (hci_cmd_task) { // driver is waiting for internal hci event
		xTaskNotifyGive(hci_cmd_task);
	} else if (receiver_notify_cb) { // notify uppper task to receive data
		receiver_notify_cb();
	}
	return 0;
}

static void btmtk_debug_unknown_pkt(const unsigned char *buf, unsigned int length, int mid)
{
	int len = length - mid;

	if (mid >= RX_BUF_DEBUG_DATA_LEN) {
		btif_util_dump_buffer("unknown pkt before (Head)", buf - mid,
							  (RX_BUF_DEBUG_DATA_LEN / 2), 1);
		btif_util_dump_buffer("unknown pkt before (Tail)",
							  buf - (RX_BUF_DEBUG_DATA_LEN / 2),
							  (RX_BUF_DEBUG_DATA_LEN / 2), 1);
	} else
		btif_util_dump_buffer("unknown pkt before", buf - mid, mid, 1);
	if (len >= RX_BUF_DEBUG_DATA_LEN)
		btif_util_dump_buffer("unknown pkt after", buf, RX_BUF_DEBUG_DATA_LEN, 1);
	else
		btif_util_dump_buffer("unknown pkt after", buf, len, 1);
}

// this function copy data from btif bbs buffer, parse it and push it into ring
// buffer or fwlog task
static int btmtk_data_recv(const unsigned char *buffer, unsigned int length)
{
	unsigned int i = 0;
	unsigned int copy_len = 0;

	if ((buffer == NULL) || (length == 0)) {
		BTIF_LOG_E("%s invalid args", __func__);
		return -1;
	}
#ifdef BUFFER_DEBUG_TRACE
	btif_rx_task_buffer_trace_write(buffer, length);
#endif

	while (i < length) {
		if (parser.valid_len == 0)
			_btmtk_reset_parser();

		copy_len = parser.exp_len < (length - i) ? parser.exp_len : (length - i);
		// header is not received yet
		if ((parser.valid_len == 0) ||
			((parser.hdr[0] == HCI_PKT_EVT && parser.valid_len < HCI_EVT_HDR_LEN)) ||
			(parser.hdr[0] == HCI_PKT_ACL && parser.valid_len < HCI_ACL_HDR_LEN) ||
			(parser.hdr[0] == HCI_PKT_ISO && parser.valid_len < HCI_ISO_HDR_LEN)) {
			memcpy(&parser.hdr[parser.valid_len], &buffer[i], copy_len);
		} else if (parser.buf != NULL) {
			memcpy(&parser.buf[parser.valid_len], &buffer[i], copy_len);
		}
		parser.valid_len += copy_len;
		parser.exp_len -= copy_len;
		i += copy_len;

		BTIF_LOG_V("%s, len: %u, i: %d, exp_len: %u, valid_len: %u, hdr: 0x%02x %02x %02x %02x",
					__func__, length, i, parser.exp_len, parser.valid_len,
					parser.hdr[0], parser.hdr[2], parser.hdr[4], parser.hdr[3]);
		if (parser.hdr[0] == HCI_PKT_EVT) {
			if (parser.valid_len == 1) {
				parser.exp_len = HCI_EVT_HDR_LEN - 1; // expect length info
			} else if (parser.valid_len == HCI_EVT_HDR_LEN) {
				parser.exp_len = parser.hdr[2]; // expect payload length
				parser.buf = (unsigned char *)pvPortMalloc(parser.exp_len + HCI_EVT_HDR_LEN);
				if (parser.buf == NULL) {
					BTIF_LOG_E("%s, Fatal Error, no memory for parser event", __func__);
					configASSERT(0);
				}
			}
		} else if (parser.hdr[0] == HCI_PKT_ACL) {
			if (parser.valid_len == 1) {
				parser.exp_len = HCI_ACL_HDR_LEN - 1; // expect length info
			} else if (parser.valid_len == HCI_ACL_HDR_LEN) {
				// expect payload length
				parser.exp_len = (((unsigned int)parser.hdr[4]) << 8) + parser.hdr[3];
				parser.buf = (unsigned char *)pvPortMalloc(parser.exp_len + HCI_ACL_HDR_LEN);
				if (parser.buf == NULL) {
					BTIF_LOG_E("%s, Fatal Error, no memory for parser ACL", __func__);
					configASSERT(0);
				}
			}
		} else if (parser.hdr[0] == HCI_PKT_ISO) {
			if (parser.valid_len == 1) {
				parser.exp_len = HCI_ISO_HDR_LEN - 1;
			} else if (parser.valid_len == HCI_ISO_HDR_LEN) {
				parser.exp_len = (((unsigned int)(parser.hdr[4] & 0x3F)) << 8) + parser.hdr[3];
				parser.buf = (unsigned char *)pvPortMalloc(parser.exp_len + HCI_ISO_HDR_LEN);
				if (parser.buf == NULL) {
					BTIF_LOG_E("%s, Fatal Error, no memory for parser ISO", __func__);
					configASSERT(0);
				}
			}
		}
		else {
			int j = i - copy_len;

			BTIF_LOG_E("%s, Unknown type: 0x%02x, buf_len = %u, j = %d, cp_len = %u, exp = %u",
					   __func__, parser.hdr[0], length, j, copy_len, parser.exp_len);
			btmtk_debug_unknown_pkt(buffer + j, length, j);
			/*
			 * btmtk_flush_rx_queue();
			 * _btmtk_reset_parser();
			 */
			btmtk_trigger_fw_assert();
			return -1;
		}

		if (parser.exp_len == 0) { // a complete packet is received
			unsigned char head_len = 0;

			switch (parser.hdr[0]) {
			case HCI_PKT_ACL:
				head_len = HCI_ACL_HDR_LEN;
				break;
			case HCI_PKT_ISO:
				head_len = HCI_ISO_HDR_LEN;
				break;
			default:
				head_len = HCI_EVT_HDR_LEN;
				break;
			}

			if (!parser.buf) {
				_btmtk_reset_parser();
				return 0;
			}

			memcpy(parser.buf, parser.hdr, head_len);
			if (((parser.buf[0] == HCI_PKT_ACL) &&
				(parser.buf[1] == 0x6F) && (parser.buf[2] == 0xFC)) ||
				((parser.buf[0] == HCI_PKT_ACL) &&
				((parser.buf[1] == 0xFF) || (parser.buf[1] == 0xFE)) &&
				(parser.buf[2] == 0x05))) { // coredump or fwlog
				BTIF_LOG_V("%s Received coredump/fwlog, len = %u", __func__, parser.valid_len);
				if (fwlog_recv_cb)
					fwlog_recv_cb(parser.buf, parser.valid_len);
				else
					_btmtk_push_hci_packet(parser.buf, parser.valid_len);
			} else {
				/*
				 * Since we only need RSSI and PER feature, we only pass this two event to picus.
				 * If need HID/A2DP performance analysis, need to change the judge condition.
				 * TODO : Discuss push it to hci packet or not
				 */
				if (bperf_enable == pdTRUE &&
					(((parser.buf[0] == HCI_PKT_EVT) &&
					(parser.buf[4] == 0x61) && (parser.buf[5] == 0xFC)) ||
					((parser.buf[0] == HCI_PKT_EVT) &&
					(parser.buf[4] == 0x11) && (parser.buf[5] == 0xFD)))) {
					if (fwlog_recv_cb)
						fwlog_recv_cb(parser.buf, parser.valid_len);
#ifdef MTK_BT_AUDIO_PR
				} else if ((parser.buf[0] == HCI_PKT_ACL)) {
					app_bt_dbg_audio_pr_write(parser.buf, 9, APP_BT_DBG_AUDIO_DECODE_PATH_RX);
#endif
				}
				_btmtk_push_hci_packet(parser.buf, parser.valid_len);
			}
			vPortFree(parser.buf);
			_btmtk_reset_parser();
		}
	}

	return 0;
}

int btmtk_send_hci_cmd(const unsigned char *cmd, const int cmd_len,
					   const unsigned char *event, const int event_len,
					   unsigned char *ret_event, int ret_event_len)
{
	int ret = -1;
	unsigned char recv[HCI_MAX_EVENT_SIZE] = {0};
	unsigned char *recv_buf = recv;
	int bytesToRead = event_len;
	int bytesRead = 0;
	unsigned char retry = HCI_CMD_EVT_READ_RETRY_CNT;

	if (cmd == NULL) {
		BTIF_LOG_E("%s cmd is null", __func__);
		return ret;
	}

	if (ret_event_len > event_len)
		bytesToRead = ret_event_len;

	hci_cmd_task = xTaskGetCurrentTaskHandle();
	btmtk_flush_rx_queue();
	// send cmd
	ret = BTIF_write(cmd, cmd_len);
	if (ret <= 0) {
		BTIF_LOG_E("%s: send hci cmd failed, terminate! err: %d", __func__, ret);
		// May happened bus fault, so dump bus register
		dump_bus_hang_reg();
		goto err_out;
	}

	if (event == NULL)
		return 0;

	/* Try to receive event_len bytes */
	while (bytesToRead > 0) {
		// try read first in case task switch
		bytesRead = btmtk_read(recv_buf, bytesToRead);
		if (bytesRead == 0) {
			BTIF_LOG_D("%s no data for read, retry=%d", __func__, retry);
			if (--retry == 0) {
				BTIF_LOG_E("%s read timeout, read:%d, target:%d",
						   __func__, (event_len - bytesToRead), event_len);
				goto err_out;
			}
			// if coredump state is BT_FW_COREDUMP_START, should not wait and break
			if (btmtk_get_coredump_state() > BT_FW_COREDUMP_CMD_SEND_START) {
				BTIF_LOG_E("%s, now is coredumping, break", __func__);
				goto err_out;
			}
			// wait for data ready notify, the caller task shall register the event cb first
			ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(HCI_CMD_EVT_READ_TOUT));
		} else {
			bytesToRead -= bytesRead;
			recv_buf += bytesRead;
		}
	}

	// check event
	if (memcmp(event, recv, event_len) == 0) {
		ret = event_len;
		btif_util_dump_buffer("hci cmd: ", cmd, cmd_len, 0);
		btif_util_dump_buffer("Received event: ", recv, event_len, 0);
	} else {
		BTIF_LOG_E("%s: HCI event mismatched, terminate!", __func__);
		btif_util_dump_buffer("hci cmd: ", cmd, cmd_len, 1);
		btif_util_dump_buffer("Expected event: ", event, event_len, 1);
		btif_util_dump_buffer("Received event: ", recv, event_len, 1);
		goto err_out;
	}

	if (ret_event)
		memcpy(ret_event, recv, ret_event_len);

	hci_cmd_task = NULL;
	return ret; // hci event match

err_out:
	hci_cmd_task = NULL;
	return -1;
}

#ifdef MTK_NVDM_ENABLE
static uint32_t str2ui(char const *str)
{
	uint32_t sum = 0;
	uint32_t i = 0;

	if (!str)
		return sum;

	if (str[0] == '0' && str[1] == 'x') {
		i = 2;
		while ((str[i] >= '0' && str[i] <= '9') || (str[i] >= 'a' && str[i] <= 'f') ||
				(str[i] >= 'A' && str[i] <= 'F')) {
			if (str[i] >= '0' && str[i] <= '9')
				sum = sum * 16 + str[i] - '0';
			if (str[i] >= 'a' && str[i] <= 'f')
				sum = sum * 16 + str[i] - 'a' + 10;
			if (str[i] >= 'A' && str[i] <= 'F')
				sum = sum * 16 + str[i] - 'A' + 10;
			i++;
		}
	} else {
		while (str[i] >= '0' && str[i] <= '9') {
			sum = sum * 10 + str[i] - '0';
			i++;
		}
	}
	return sum;
}

/*
 * bt config read, write and delete operations
 */
static int btmtk_check_bt_cfg_format(char const *key, char *val[], uint8_t len)
{
	char tmp;
	uint8_t i = 0;

	// first char should be A ~ Z
	if (*key < 'A' || *key > 'Z') {
		BTIF_LOG_E("the first char of \'%s\' should A ~ Z", key);
		return -1;
	}
	tmp = *(key + 1);
	while ((i < strlen(key) - 1) && tmp) {
		if (!(tmp >= 'A' && tmp <= 'Z') && (tmp != '_') &&
			!(tmp >= '0' && tmp <= '9')) {
			BTIF_LOG_E("\'%s\' should in A ~ Z and \'_\' and 0 ~ 9", key);
			return -1;
		}
		tmp = *(key + 2 + i++);
	}

	if (len == 1) { // decimal input check
		if (strlen(val[0]) > BT_CFG_PARAM_CHAR_SIZE - 1) {
			BTIF_LOG_E("input \'%s\' should be an decimal, in 0 ~ 255", val[0]);
			return -1;
		}

		for (i = 0; *(val[0] + i); i++) {
			if (*(val[0] + i) < '0' || *(val[0] + i) > '9') {
				BTIF_LOG_E("input \'%s\' should be dec, in 0 ~ 255", val[0]);
				return -1;
			}
		}
		if (str2ui(val[0]) > UINT8_MAX) {
			BTIF_LOG_E("input \'%s\' should be in 0 ~ 255", val[0]);
			return -1;
		}
	} else { // hexadecimal input check
		for (i = 0; i < len; i++) {
			if (strlen(val[i]) == BT_CFG_PARAM_CHAR_SIZE) {
				if (val[i][0] != '0' ||
					(val[i][1] != 'x' && val[i][1] != 'X') ||
					(!(val[i][2] >= '0' && val[i][2] <= '9') &&
					!(val[i][2] >= 'a' && val[i][2] <= 'f') &&
					!(val[i][2] >= 'A' && val[i][2] <= 'F')) ||
					(!(val[i][3] >= '0' && val[i][3] <= '9') &&
					!(val[i][3] >= 'a' && val[i][3] <= 'f') &&
					!(val[i][3] >= 'A' && val[i][3] <= 'F'))) {
					BTIF_LOG_E("param[%d] = \'%s\' is not hex, format: 0xXX", i, val[i]);
					return -1;
				}
			} else {
				BTIF_LOG_E("input param[%d] = \'%s\' should be hex as 0xXX", i, val[i]);
				return -1;
			}
		}
	}

	return 0;
}

static int btmtk_parse_bt_config_item(char *context, uint8_t *buf)
{
	char *pstr = NULL;
	uint8_t cnt = 0;

	if (!context) {
		BTIF_LOG_E("%s, context is null", __func__);
		return -1;
	}

	if (!strstr(context, ",")) {
		buf[0] = str2ui(context);
		return (cnt + 1);
	}

	pstr = strtok(context, ",");
	while (pstr) {
		while (*pstr == ' ')
			pstr++;
		buf[cnt] = str2ui(pstr);
		pstr = strtok(NULL, ",");
		cnt++;
	}

	return cnt;
}
#endif

void btmtk_write_bt_cfg(char *key, char *val[], uint8_t len)
{
#ifdef MTK_NVDM_ENABLE
	nvdm_status_t status;
	uint8_t buf[BT_CFG_ITEM_VAL_SIZE] = {0};
	uint8_t i = 0;

	if (!key)
		return;

	if (btmtk_check_bt_cfg_format(key, val, len))
		return;
	for (i = 0; i < len; i++) {
		// save format: 0xAA, 0xBB, 0xCC, so interval = 6
		memcpy(buf + i * (BT_CFG_PARAM_CHAR_SIZE + 2), val[i], BT_CFG_PARAM_CHAR_SIZE);
		if (i < len - 1)
			memcpy(buf + i * (BT_CFG_PARAM_CHAR_SIZE + 2) + BT_CFG_PARAM_CHAR_SIZE, ", ", 2);
	}
	len = (BT_CFG_PARAM_CHAR_SIZE + 2) * (len - 1) + BT_CFG_PARAM_CHAR_SIZE;
	status = nvdm_write_data_item(BT_CFG_GRP_NAME, key, NVDM_DATA_ITEM_TYPE_STRING, buf, len);
	if (status != NVDM_STATUS_OK)
		BTIF_LOG_E("%s, set \"%s\" fail", __func__, key);
	else
		BTIF_LOG_I("%s, set \"%s\" ok", __func__, key);
#endif
}

int btmtk_read_bt_cfg(void)
{
#ifdef MTK_NVDM_ENABLE
	nvdm_status_t status;
	uint32_t item_cnt = 0;
	char item[BT_CFG_ITEM_TOTAL][BT_CFG_ITEM_NAME_SIZE] = {0};
	char value[BT_CFG_ITEM_TOTAL][BT_CFG_ITEM_VAL_SIZE] = {0};
	uint8_t i = 0;

	item_cnt = nvdm_get_item_count_by_group(BT_CFG_GRP_NAME);
	BTIF_LOG_I("total items in BT_CONFIG: %lu", item_cnt);
	if (!item_cnt)
		return -1;

	status = nvdm_get_all_items_by_group(BT_CFG_GRP_NAME, (char *)item,
										 BT_CFG_ITEM_NAME_SIZE, (char *)value,
										 BT_CFG_ITEM_VAL_SIZE);
	if (status != NVDM_STATUS_OK) {
		BTIF_LOG_E("%s, get all items by \'%s\', err: %d", __func__, BT_CFG_GRP_NAME, status);
		return -1;
	}
	for (i = 0; i < item_cnt; i++)
		BTIF_LOG_I("%28s %s", item[i], value[i]);

#endif
	return 0;
}

void btmtk_delete_bt_cfg(char *key)
{
#ifdef MTK_NVDM_ENABLE
	nvdm_status_t status;

	if (!key)
		return;

	status = nvdm_delete_data_item(BT_CFG_GRP_NAME, key);
	if (status != NVDM_STATUS_OK)
		BTIF_LOG_E("%s, delete \"%s\" error, err: %d", __func__, key, status);
	else
		BTIF_LOG_I("%s, key: %s deleted", __func__, key);
#endif
}

static void btmtk_parse_bt_config(void)
{
#ifdef MTK_NVDM_ENABLE
	nvdm_status_t status;
	uint8_t buf[BT_CFG_ITEM_VAL_SIZE] = {0};
	uint8_t param[BT_CFG_ITEM_VAL_SIZE / 6] = {0};
	uint32_t size = BT_CFG_ITEM_VAL_SIZE;
	uint8_t cnt = 0;

#ifdef MTK_BT_DRV_CHIP_RESET
	status = nvdm_read_data_item(BT_CFG_GRP_NAME, BT_CFG_SUPPORT_DONGLE_RESET, buf, &size);
	if (status == NVDM_STATUS_OK) {
		cnt = btmtk_parse_bt_config_item((char *)buf, param);
		if (cnt == BT_CFG_BASE_PARAM_LEN) {
			bt_drv_info.cfg.support_dongle_reset = param[0];
		} else {
			BTIF_LOG_E("%s, set \'%s\' is len err, len: %d, should be %d",
						__func__, BT_CFG_SUPPORT_DONGLE_RESET, cnt, BT_CFG_BASE_PARAM_LEN);
			return;
		}
	}
#endif /* #ifdef MTK_BT_DRV_CHIP_RESET */
	size = BT_CFG_ITEM_VAL_SIZE;
	status = nvdm_read_data_item(BT_CFG_GRP_NAME, BT_CFG_SUPPORT_SINGLE_SKU, buf, &size);
	if (status == NVDM_STATUS_OK) {
		cnt = btmtk_parse_bt_config_item((char *)buf, param);
		if (cnt == BT_CFG_BASE_PARAM_LEN) {
			bt_drv_info.cfg.support_single_sku = param[0];
		} else {
			BTIF_LOG_E("%s, your set \'%s\' is len err, len: %d, should be %d",
						__func__, BT_CFG_SUPPORT_SINGLE_SKU, cnt, BT_CFG_BASE_PARAM_LEN);
			return;
		}
	}

	size = BT_CFG_ITEM_VAL_SIZE;
	status = nvdm_read_data_item(BT_CFG_GRP_NAME, BT_CFG_WAIT_FW_DUMP_OVER, buf, &size);
	if (status == NVDM_STATUS_OK) {
		cnt = btmtk_parse_bt_config_item((char *)buf, param);
		if (cnt == BT_CFG_BASE_PARAM_LEN) {
			bt_drv_info.cfg.wait_fw_dump_over = param[0];
		} else {
			BTIF_LOG_E("%s, your set \'%s\' is len err, len: %d, should be %d",
						__func__, BT_CFG_SUPPORT_SINGLE_SKU, cnt, BT_CFG_BASE_PARAM_LEN);
			return;
		}
	}

#ifdef MTK_BT_DRV_AUTO_PICUS
	size = BT_CFG_ITEM_VAL_SIZE;
	status = nvdm_read_data_item(BT_CFG_GRP_NAME, BT_CFG_SUPPORT_AUTO_PICUS, buf, &size);
	if (status == NVDM_STATUS_OK) {
		cnt = btmtk_parse_bt_config_item((char *)buf, param);
		if (cnt == BT_CFG_BASE_PARAM_LEN) {
			bt_drv_info.cfg.support_auto_picus = param[0];
		} else {
			BTIF_LOG_E("%s, your set \'%s\' is len err, len: %d should be %d",
						__func__, BT_CFG_SUPPORT_AUTO_PICUS, cnt, BT_CFG_BASE_PARAM_LEN);
			return;
		}
	}

	if (bt_drv_info.cfg.support_auto_picus) {
		size = BT_CFG_ITEM_VAL_SIZE;
		status = nvdm_read_data_item(BT_CFG_GRP_NAME, BT_CFG_PICUS_LOG_LEVEL, buf, &size);
		if (status == NVDM_STATUS_OK) {
			cnt = btmtk_parse_bt_config_item((char *)buf, param);
			if (cnt == BT_CFG_BASE_PARAM_LEN) {
				bt_drv_info.cfg.picus_log_level = param[0];
			} else {
				BTIF_LOG_E("%s, your set \'%s\' len err, len: %d should be %d",
							__func__, BT_CFG_PICUS_LOG_LEVEL, cnt, BT_CFG_BASE_PARAM_LEN);
				bt_drv_info.cfg.picus_log_level = 0xff;
				return;
			}
		}

		size = BT_CFG_ITEM_VAL_SIZE;
		status = nvdm_read_data_item(BT_CFG_GRP_NAME, BT_CFG_PICUS_LOG_VIA, buf, &size);
		if (status == NVDM_STATUS_OK) {
			cnt = btmtk_parse_bt_config_item((char *)buf, param);
			if (cnt == BT_CFG_BASE_PARAM_LEN) {
				bt_drv_info.cfg.picus_log_via = param[0];
			} else {
				BTIF_LOG_E("%s, set \'%s\' is len err, len: %d should be %d",
							__func__, BT_CFG_PICUS_LOG_VIA, cnt, BT_CFG_BASE_PARAM_LEN);
				bt_drv_info.cfg.picus_log_via = 0xff;
				return;
			}
		}

		size = BT_CFG_ITEM_VAL_SIZE;
		status = nvdm_read_data_item(BT_CFG_GRP_NAME, BT_CFG_PICUS_LOG_UART_BAUDRATE, buf, &size);
		if (status == NVDM_STATUS_OK) {
			cnt = btmtk_parse_bt_config_item((char *)buf, param);
			if (cnt == BT_CFG_BASE_PARAM_LEN) {
				bt_drv_info.cfg.picus_uart_baudrate = param[0];
			} else {
				BTIF_LOG_E("%s, set \'%s\' len err, len: %d should be %d",
							__func__, BT_CFG_PICUS_LOG_UART_BAUDRATE, cnt, BT_CFG_BASE_PARAM_LEN);
				return;
			}
		}
	}
#endif /* #ifdef MTK_BT_DRV_AUTO_PICUS */
	BTIF_LOG_I("bt config:");
#ifdef MTK_BT_DRV_CHIP_RESET
	BTIF_LOG_I("%20s = %d", BT_CFG_SUPPORT_DONGLE_RESET,
				bt_drv_info.cfg.support_dongle_reset);
#endif /* #ifdef MTK_BT_DRV_CHIP_RESET */
	BTIF_LOG_I("%20s = %d", BT_CFG_SUPPORT_SINGLE_SKU,
				bt_drv_info.cfg.support_single_sku);
	BTIF_LOG_I("%20s = %d", BT_CFG_WAIT_FW_DUMP_OVER,
				bt_drv_info.cfg.wait_fw_dump_over);
#ifdef MTK_BT_DRV_AUTO_PICUS
	BTIF_LOG_I("%20s = %d", BT_CFG_SUPPORT_AUTO_PICUS,
				bt_drv_info.cfg.support_auto_picus);
	BTIF_LOG_I("%20s = %d", BT_CFG_PICUS_LOG_LEVEL,
				bt_drv_info.cfg.picus_log_level);
	BTIF_LOG_I("%20s = %d", BT_CFG_PICUS_LOG_VIA,
				bt_drv_info.cfg.picus_log_via);
	BTIF_LOG_I("%20s = %d", BT_CFG_PICUS_LOG_UART_BAUDRATE,
				bt_drv_info.cfg.picus_uart_baudrate);
#endif /* #ifdef MTK_BT_DRV_AUTO_PICUS */
#endif
}

#ifdef MTK_NVDM_ENABLE
/*
 * country's power set, read, write and delete operations
 *
 * cty: country code, use two chars insted, e.g. AU
 * pw: power set, <br_edr_pwr_mode, edr_max_tx_pwr, ble_def_tx_pwr,
 * ble_def_tx_pwr_2m, ble_lr_s2, ble_lr_s8>
 *     e.g. 1, 1.75, -3, -3, -2, -2; 0, 17, 17, 17, 20, 20
 */
static int btmtk_check_power_set_format(char *const item, uint8_t type)
{
	int ret = 0;
	int param_val = 0;
	char *str = NULL;

	if (!item)
		return -1;

	str = (char *)btif_util_malloc(strlen(item) + 1);

	if (!str)
		return -1;

	memset(str, 0, strlen(item) + 1);
	memcpy(str, item, strlen(item));
	str[strlen(item)] = '\0';

	switch (type) {
	// mode is 0 or 1
	case CHECK_SINGLE_SKU_PWR_MODE:
		if (strlen(str) != 1 || (str[0] != '0' && str[0] != '1')) {
			ret = -1;
			BTIF_LOG_E("param[0] \'%s\' not 0 or 1", str);
			break;
		}
		ret = 0;
		break;
	// EDR_MAX is -32 ~ 17, dot in 0.25, 0.5 or 0.75
	case CHECK_SINGLE_SKU_EDR_MAX:
		if (strstr(str, ".")) {
			if (!(strstr(str, ".25") || strstr(str, ".5") || strstr(str, ".75"))) {
				BTIF_LOG_E("param[1] \'%s\' should .25, .5 or .75", str);
				ret = -1;
				break;
			}
			param_val = atoi(strtok(str, "."));
			if (param_val < -31 || param_val > 16) {
				BTIF_LOG_E("param[1] \'%s\' should be -32 ~ 17", item);
				ret = -1;
				break;
			}
		} else {
			param_val = atoi(str);
			if (param_val < -32 || param_val > 17) {
				BTIF_LOG_E("param[1] \'%s\' should be -32 ~ 17", str);
				ret = -1;
				break;
			}
		}
		ret = 0;
		break;
	// LE power is -29 ~ 20, dot in 0.25, 0.5 or 0.75
	case CHECK_SINGLE_SKU_BLE:
	case CHECK_SINGLE_SKU_BLE_2M:
	case CHECK_SINGLE_SKU_BLE_LR_S2:
	case CHECK_SINGLE_SKU_BLE_LR_S8:
		if (strstr(str, ".")) {
			if (!(strstr(str, ".25") || strstr(str, ".5") || strstr(str, ".75"))) {
				BTIF_LOG_E("param[%d] \'%s\' should .25, .5 or .75", type, str);
				ret = -1;
				break;
			}
			param_val = atoi(strtok(str, "."));
			if (param_val < -28 || param_val > 19) {
				BTIF_LOG_E("param[%d] \'%s\' should be -29 ~ 20", type, item);
				ret = -1;
				break;
			}
		} else {
			param_val = atoi(str);
			if (param_val < -29 || param_val > 20) {
				BTIF_LOG_E("param[%d] \'%s\' should be -29 ~ 20", type, str);
				ret = -1;
				break;
			}
		}
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}

	btif_util_free(str);
	return ret;
}

static void btmtk_init_power_setting_struct(void)
{
	bt_drv_info.pw_sets.edr_max = 0;
	bt_drv_info.pw_sets.lv9 = 0;
	bt_drv_info.pw_sets.dm = 0;
	bt_drv_info.pw_sets.ir = 0;
	bt_drv_info.pw_sets.ble_1m = 0;
	bt_drv_info.pw_sets.ble_2m = 0;
	bt_drv_info.pw_sets.ble_lr_s2 = 0;
	bt_drv_info.pw_sets.ble_lr_s8 = 0;
}

static int btmtk_check_power_resolution(char *str)
{
	if (str == NULL)
		return -1;
	if (strstr(str, ".25") || strstr(str, ".75"))
		return RES_DOT_25;
	if (strstr(str, ".5"))
		return RES_DOT_5;
	if (!strstr(str, ".") || strstr(str, ".0"))
		return RES_1;
	return -1;
}

static int btmtk_set_power_value(char *str, int resolution, int is_edr)
{
	int power = ERR_PWR;
	int integer = 0;
	int decimal = 0;
	int ret = 0;

	if (resolution == RES_DOT_25) {
		/* XX.YY => XX.YY/0.25 = XX*4 + YY/25 */
		if (strstr(str, ".")) {
			ret = sscanf(str, "%d.%d", &integer, &decimal);
			if (ret < 0)
				return ret;
			if (decimal != 25 && decimal != 75 && decimal != 5 && decimal != 50)
				return ERR_PWR;
			if (decimal == 5)
				decimal = 50;
			if (integer >= 0)
				power = integer * 4 + decimal / 25;
			else
				power = integer * 4 - decimal / 25;
		} else {
			ret = sscanf(str, "%d", &integer);
			if (ret < 0)
				return ret;
			power = integer * 4;
		}

		BTIF_LOG_D("%s: power = %d", __func__, power);

		if (is_edr) {
			if (power > EDR_MAX_R2 || power < EDR_MIN_R2)
				return ERR_PWR;
			if (power > EDR_MIN_LV9_R2)
				bt_drv_info.pw_sets.lv9 = 1;
		} else {
			if (power > BLE_MAX_R2 || power < BLE_MIN_R2)
				return ERR_PWR;
		}
	} else if (resolution == RES_DOT_5) {
		/* XX.YY => XX.YY/0.5 = XX*2 + YY/5 */
		if (strstr(str, ".")) {
			ret = sscanf(str, "%d.%d", &integer, &decimal);
			if (ret < 0)
				return ret;
			if (decimal != 5)
				return ERR_PWR;
			if (integer >= 0)
				power = integer * 2 + decimal / 5;
			if (integer < 0)
				power = integer * 2 - decimal / 5;
		} else {
			ret = sscanf(str, "%d", &integer);
			if (ret < 0)
				return ret;
			power = integer * 2;
		}

		BTIF_LOG_D("%s: power = %d", __func__, power);

		if (is_edr) {
			if (power > EDR_MAX_R1 || power < EDR_MIN_R1)
				return ERR_PWR;
			if (power > EDR_MIN_LV9_R1)
				bt_drv_info.pw_sets.lv9 = 1;
		} else {
			if (power > BLE_MAX_R1 || power < BLE_MIN_R1)
				return ERR_PWR;
		}
	} else if (resolution == RES_1) {
		ret = sscanf(str, "%d", &power);
		if (ret < 0)
			return ret;

		BTIF_LOG_D("%s: power = %d", __func__, power);

		if (is_edr) {
			if (power > EDR_MAX || power < EDR_MIN)
				return ERR_PWR;
			if (power > EDR_MIN_LV9)
				bt_drv_info.pw_sets.lv9 = 1;
		} else {
			if (power > BLE_MAX || power < BLE_MIN)
				return ERR_PWR;
		}
	}

	return power;
}

static int btmtk_parse_power_item(char *context)
{
	char *ptr = NULL;
	int step = 0, temp;
	int resolution;
	int power;

	if (context == NULL) {
		BTIF_LOG_E("%s context is NULL", __func__);
		return -1;
	}

	BTIF_LOG_D("%s, context = %s", __func__, context);
	btmtk_init_power_setting_struct();

	resolution = btmtk_check_power_resolution(context);
	if (resolution == -1) {
		BTIF_LOG_E("Check resolution fail");
		return -1;
	}

	bt_drv_info.pw_sets.ir |= resolution;
	BTIF_LOG_D("%s: resolution = %d", __func__, resolution);

	ptr = strtok(context, ",");
	while (ptr != NULL) {
		while (*ptr == ' ')
			ptr++;

		switch (step) {
		/* BR_EDR_PWR_MODE */
		case CHECK_SINGLE_SKU_PWR_MODE:
			temp = atoi(ptr);
			if (temp >= 0) {
				if (temp == 0 || temp == 1) {
					bt_drv_info.pw_sets.dm = temp;
					step++;
					break;
				}
				BTIF_LOG_E("PWR MODE value wrong");
				return -1;
			}
			BTIF_LOG_E("Read PWR MODE Fail");
			return -1;
		/* Parse EDR MAX */
		case CHECK_SINGLE_SKU_EDR_MAX:
			power = btmtk_set_power_value(ptr, resolution, 1);
			if (power == ERR_PWR) {
				BTIF_LOG_E("EDR MAX value wrong");
				return -1;
			}
			bt_drv_info.pw_sets.edr_max = power;
			step++;
			break;
		/* Parse BLE Default */
		case CHECK_SINGLE_SKU_BLE:
			power = btmtk_set_power_value(ptr, resolution, 0);
			if (power == ERR_PWR) {
				BTIF_LOG_E("BLE value wrong");
				return -1;
			}
			bt_drv_info.pw_sets.ble_1m = power;
			step++;
			break;
		/* Parse BLE 2M */
		case CHECK_SINGLE_SKU_BLE_2M:
			power = btmtk_set_power_value(ptr, resolution, 0);
			if (power == ERR_PWR) {
				BTIF_LOG_E("BLE 2M value wrong");
				return -1;
			}
			bt_drv_info.pw_sets.ble_2m = power;
			step++;
			break;
		/* Parse BLE long range S2 */
		case CHECK_SINGLE_SKU_BLE_LR_S2:
			power = btmtk_set_power_value(ptr, resolution, 0);
			if (power == ERR_PWR) {
				BTIF_LOG_E("BLE LR S2 value wrong");
				return -1;
			}
			bt_drv_info.pw_sets.ble_lr_s2 = power;
			step++;
			break;
		/* Parse BLE long range S8 */
		case CHECK_SINGLE_SKU_BLE_LR_S8:
			power = btmtk_set_power_value(ptr, resolution, 0);
			if (power == ERR_PWR) {
				BTIF_LOG_E("BLE LR S8 value wrong");
				return -1;
			}
			bt_drv_info.pw_sets.ble_lr_s8 = power;
			step++;
			break;
		default:
			BTIF_LOG_E("%s step is wrong: %d", __func__, step);
			break;
		}
		ptr = strtok(NULL, ",");
	}

	return step;
}

static int btmtk_send_tx_power_cmd(void)
{
	/**
	 *  TCI Set TX Power Command
	 *  01 2C FC 0C QQ 00 00 00 XX YY ZZ GG AA BB CC DD
	 *  QQ: EDR init TX power dbm // the value is equal to EDR MAX
	 *  XX: BLE TX power dbm
	 *  YY: EDR MAX TX power dbm
	 *  ZZ: Enable LV9
	 *  GG: 3db diff mode
	 *  AA: [5:4] Indicator  [5] 1: cmd send to BT1, [4] 1: cmd send to BT0
	 *      [3:0] Resolution  0: 1dBm, 1: 0.5dBm, 2: 0.25dBm
	 *  BB: BLE 2M
	 *  CC: BLE S2
	 *  DD: BLE S8
	 */

	uint8_t cmd[TX_POWER_CMD_LEN] = {0x01, 0x2C, 0xFC, 0x0C, 0x00, 0x00, 0x00, 0x00,
									 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t event[TX_POWER_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x2C, 0xFC, 0x00};
	int ret = 0;

	cmd[4] = (uint8_t)bt_drv_info.pw_sets.edr_max;
	cmd[8] = (uint8_t)bt_drv_info.pw_sets.ble_1m;
	cmd[9] = (uint8_t)bt_drv_info.pw_sets.edr_max;
	cmd[10] = (uint8_t)bt_drv_info.pw_sets.lv9;
	cmd[11] = (uint8_t)bt_drv_info.pw_sets.dm;
	cmd[12] = (uint8_t)bt_drv_info.pw_sets.ir;
	cmd[13] = (uint8_t)bt_drv_info.pw_sets.ble_2m;
	cmd[14] = (uint8_t)bt_drv_info.pw_sets.ble_lr_s2;
	cmd[15] = (uint8_t)bt_drv_info.pw_sets.ble_lr_s8;

	BTIF_LOG_I("power_set:  edr_init = %d", (int8_t)cmd[4]);
	BTIF_LOG_I("power_set:    ble_1m = %d", (int8_t)cmd[8]);
	BTIF_LOG_I("power_set:   edr_max = %d", (int8_t)cmd[9]);
	BTIF_LOG_I("power_set:       lv9 = %d", (int8_t)cmd[10]);
	BTIF_LOG_I("power_set:        dm = %d", (int8_t)cmd[11]);
	BTIF_LOG_I("power_set:        ir = %d", (int8_t)cmd[12]);
	BTIF_LOG_I("power_set:    ble_2m = %d", (int8_t)cmd[13]);
	BTIF_LOG_I("power_set: ble_lr_s2 = %d", (int8_t)cmd[14]);
	BTIF_LOG_I("power_set: ble_lr_s8 = %d", (int8_t)cmd[15]);

	ret = btmtk_send_hci_cmd(cmd, TX_POWER_CMD_LEN, event, TX_POWER_EVT_LEN, NULL, 0);
	if (ret < 0)
		BTIF_LOG_E("%s failed!!, err: %d", __func__, ret);
	else
		BTIF_LOG_I("%s: OK", __func__);

	return ret;
}
#endif

void btmtk_write_country_bt_power(char *cty, char *pwr[])
{
#ifdef MTK_NVDM_ENABLE
	nvdm_status_t status;
	uint8_t buf[BT_PWR_ITEM_VAL_SIZE] = {0};
	uint8_t i = 0, len = 0;

	if (!cty || !pwr)
		return;

	for (i = 0; i < BT_PWR_PARAM_CNT; i++)
		if (btmtk_check_power_set_format(pwr[i], i))
			return;

	for (i = 0; i < BT_PWR_PARAM_CNT; i++) {
		if ((len + strlen(pwr[i])) > BT_PWR_ITEM_VAL_SIZE) {
			BTIF_LOG_E("%s, run out of buf", __func__);
			break;
		}

		memcpy(buf + len, pwr[i], strlen(pwr[i]));
		len += strlen(pwr[i]);
		if (i < BT_PWR_PARAM_CNT - 1) {
			memcpy(buf + len, ", ", 2);
			len += 2;
		}
	}
	status = nvdm_write_data_item(BT_PWR_TBL_GRP_NAME, cty, NVDM_DATA_ITEM_TYPE_STRING, buf, len);
	if (status != NVDM_STATUS_OK)
		BTIF_LOG_E("%s, set \"%s\" fail", __func__, cty);
	else
		BTIF_LOG_I("%s, set \"%s\" ok", __func__, cty);
#endif
}

int btmtk_read_country_bt_power(void)
{
#ifdef MTK_NVDM_ENABLE
	nvdm_status_t status;
	uint32_t item_cnt = 0;
	uint32_t i = 0;
	char item[BT_PWR_ITEM_TOTAL][BT_PWR_ITEM_NAME_SIZE] = {0};
	char value[BT_PWR_ITEM_TOTAL][BT_PWR_ITEM_VAL_SIZE] = {0};

	item_cnt = nvdm_get_item_count_by_group(BT_PWR_TBL_GRP_NAME);
	BTIF_LOG_I("total items in BT_POWER_TABLE: %lu", item_cnt);
	if (!item_cnt)
		return -1;

	status = nvdm_get_all_items_by_group(BT_PWR_TBL_GRP_NAME, (char *)item,
										 BT_PWR_ITEM_NAME_SIZE, (char *)value,
										 BT_PWR_ITEM_VAL_SIZE);
	if (status != NVDM_STATUS_OK) {
		BTIF_LOG_E("%s, get all items by \'%s\', err: %d", __func__, BT_PWR_TBL_GRP_NAME, status);
		return -1;
	}
	for (i = 0; i < item_cnt; i++)
		BTIF_LOG_I("%4s %s", item[i], value[i]);
#endif
	return 0;
}

void btmtk_delete_country_bt_power(char *cty)
{
#ifdef MTK_NVDM_ENABLE
	nvdm_status_t status;

	if (!cty)
		return;

	status = nvdm_delete_data_item(BT_PWR_TBL_GRP_NAME, cty);
	if (status != NVDM_STATUS_OK)
		BTIF_LOG_E("%s, delete \"%s\" error, err: %d", __func__, cty, status);
	else
		BTIF_LOG_I("%s, key: %s deleted", __func__, cty);
#endif
}

int btmtk_is_country_code_set(void)
{
	BTIF_LOG_D("%s, country_code: %s", __func__, bt_drv_info.pw_sets.country_code);
	if (strcmp(bt_drv_info.pw_sets.country_code, "") == 0)
		return 0;
	else
		return -1;
}

void btmtk_set_bt_power_by_country_code(char *cty_code)
{
#ifdef MTK_NVDM_ENABLE
	int ret = 0;
	nvdm_status_t status;
	char buf[BT_PWR_ITEM_VAL_SIZE] = {0};
	uint32_t size = BT_PWR_ITEM_VAL_SIZE;

	if (!bt_drv_info.cfg.support_single_sku) {
		BTIF_LOG_W("do not support single sku");
		return;
	}

	if (!cty_code) {
		BTIF_LOG_E("country code is NULL");
		return;
	}

	BTIF_LOG_I("%s, country_code: %s", __func__, cty_code);
	status = nvdm_read_data_item(BT_PWR_TBL_GRP_NAME, cty_code, (uint8_t *)buf, &size);
	if (status != NVDM_STATUS_OK) {
		BTIF_LOG_E("not find country: \'%s\'", cty_code);
		return;
	}

	memcpy(bt_drv_info.pw_sets.country_code, cty_code, sizeof(bt_drv_info.pw_sets.country_code));
	ret = btmtk_parse_power_item(buf);
	if (ret != CHECK_SINGLE_SKU_ALL) {
		BTIF_LOG_E("Parse power fail, ret = %d", ret);
		return;
	}

	btmtk_send_tx_power_cmd();
#endif
}

int btmtk_send_low_power_cmd(void)
{
	int ret = 0;
	uint8_t cmd[LOW_POWER_COMMAND_LEN] = {0x01, 0x7A, 0xFC, 0x07, 0x03, 0x04,
										  0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t event[LOW_POWER_EVENT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x7A, 0xFC, 0x00};

	ret = btmtk_send_hci_cmd(cmd, LOW_POWER_COMMAND_LEN, event, LOW_POWER_EVENT_LEN, NULL, 0);
	if (ret < 0)
		BTIF_LOG_E("%s failed!!, err: %d", __func__, ret);
	else
		BTIF_LOG_I("%s: OK", __func__);

	return ret;
}

//---------------------------------------------------------------------------
/* btmtk_bgfsys_power_on
 *
 *    BGF MCU power on sequence
 *
 * Arguments:
 *    N/A
 *
 * Return Value:
 *     0 if success, otherwise error code
 *
 */
int btmtk_bgfsys_power_on(void)
{
	unsigned int value;
	int32_t retry = POS_POLLING_RTY_LMT;
	unsigned int mcu_idle;

	BTIF_LOG_I("BGFSYS power on start");
	/* wake up conn_infra off */
	SET_BIT(CONN_INFRA_WAKEUP_BT, BIT(0)); // 0x180601A8[0]
	consys_polling_chipid();

	/* reset n10 cpu core */
	CLR_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST, BGF_CPU_SW_RST_B); // 0x18000124[0]

	/* power on bgfsys on */
	value = DRV_Reg32(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL) | CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_ON;
	DRV_WriteReg32(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL, value);
	value = DRV_Reg32(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL);
	BTIF_LOG_D("%s: addr(0x%08x) = 0x%08x", __func__, CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL, value);

	/* enable bt function en */
	SET_BIT(CONN_INFRA_CFG_BT_PWRCTLCR0, BT_FUNC_EN_B); // 0x18001208[0]

	/* polling bgfsys top on power ack bits until they are asserted */
	do {
		value = BGF_ON_PWR_ACK_B &
				DRV_Reg32(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_ACK_ST); // 0x18000414[25:24]
		BTIF_LOG_D("bgfsys on power ack = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value != BGF_ON_PWR_ACK_B && retry > 0);

	if (retry == 0)
		goto error;

	/* polling bgfsys top off power ack bits until they are asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BGF_OFF_PWR_ACK_B &
				DRV_Reg32(CONN_INFRA_RGU_BGFSYS_OFF_TOP_PWR_ACK_ST); // 0x18000424[24]
		BTIF_LOG_D("bgfsys off top power ack_b = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value != BGF_OFF_PWR_ACK_B && retry > 0);

	if (retry == 0)
		goto error;

	retry = POS_POLLING_RTY_LMT;
	do {
		value = BGF_OFF_PWR_ACK_S &
				DRV_Reg32(CONN_INFRA_RGU_BGFSYS_OFF_TOP_PWR_ACK_ST); // 0x18000424[1:0]
		BTIF_LOG_D("bgfsys off top power ack_s = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value != BGF_OFF_PWR_ACK_S && retry > 0);

	if (retry == 0)
		goto error;

	/* disable conn2bt slp_prot rx en */
	CLR_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL, CONN2BT_SLP_PROT_RX_EN_B); // 0x18001550[4]

	/* polling conn2bt slp_prot rx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_RX_ACK_B &
				DRV_Reg32(CONN_INFRA_CONN2BT_GALS_SLP_STATUS); // 0x18001554[22]
		BTIF_LOG_D("conn2bt slp_prot rx ack = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value != 0 && retry > 0);

	if (retry == 0)
		goto error;

	/* disable conn2bt slp_prot tx en */
	CLR_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL, CONN2BT_SLP_PROT_TX_EN_B); // 0x18001550[0]

	/* polling conn2bt slp_prot tx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_TX_ACK_B &
				DRV_Reg32(CONN_INFRA_CONN2BT_GALS_SLP_STATUS); // 0x18001554[23]
		BTIF_LOG_D("conn2bt slp_prot tx ack = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value != 0 && retry > 0);

	if (retry == 0)
		goto error;

	/* disable bt2conn slp_prot rx en */
	CLR_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL, BT2CONN_SLP_PROT_RX_EN_B); // 0x18001560[4]
	/* polling bt2conn slp_prot rx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_RX_ACK_B &
				DRV_Reg32(CONN_INFRA_BT2CONN_GALS_SLP_STATUS); // 0x18001564[22]
		BTIF_LOG_D("bt2conn slp_prot rx ack = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value != 0 && retry > 0);

	if (retry == 0)
		goto error;

	/* disable bt2conn slp_prot tx en */
	CLR_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL, BT2CONN_SLP_PROT_TX_EN_B); // 0x18001560[0]
	/* polling bt2conn slp_prot tx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_TX_ACK_B &
				DRV_Reg32(CONN_INFRA_BT2CONN_GALS_SLP_STATUS); // 0x18001564[23]
		BTIF_LOG_D("bt2conn slp_prot tx ack = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value != 0 && retry > 0);

	if (retry == 0)
		goto error;

	rtos_mdelay(1);

	/* read and check bgfsys version id */
	retry = 10;
	do {
		value = DRV_Reg32(BGF_IP_VERSION); // 0x18812010[31:0]
		BTIF_LOG_I("bgfsys version id = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value != BGF_IP_VER_ID && retry > 0);

	if (retry == 0)
		goto error;

	/* read bgfsys hw_version */
	value = DRV_Reg32(BGF_HW_VERSION); // 0x18812000[31:0]
	BTIF_LOG_I("bgfsys hw version id = 0x%08x", value);
	if (value != BGF_HW_VER_ID)
		goto error;

	/* read bgfsys fw_version */
	value = DRV_Reg32(BGF_FW_VERSION); // 0x18812004[31:0]
	BTIF_LOG_I("bgfsys fw version id = 0x%08x", value);
	if (value != BGF_FW_VER_ID)
		goto error;

	/* read bgfsys hw_code */
	value = DRV_Reg32(BGF_HW_CODE); // 0x18812008[31:0]
	BTIF_LOG_I("bgfsys hw code = 0x%08x", value);
	if (value != BGF_HW_CODE_ID)
		goto error;

	/* clear con_cr_ahb_auto_dis */
	CLR_BIT(BGF_MCCR, BGF_CON_CR_AHB_AUTO_DIS); // 0x18800100[31]

	/* set con_cr_ahb_stop */
	DRV_SetReg32(BGF_MCCR_SET, BGF_CON_CR_AHB_STOP); // 0x18800104[31:0]

	/* mask mtcmos fsm pwr_ack&ack_s status check while power on */
	SET_BIT(CONN_INFRA_CFG_BT_MANUAL_CTRL, CONN_INFRA_BT_MANUAL_CTRL_B); // 0x18001108[27]

	/* release n10 cpu core */
	SET_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST, BGF_CPU_SW_RST_B); // 0x18000124[0]

	/* release conn_infra force on */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, CONN_INFRA_WAKE_BT_B); // 0x180601A8[0]

	/*
	 * polling BGFSYS MCU sw_dbg_ctl cr to wait it becomes 0x1D1E,
	 * which indicates that the power-on part of ROM is completed.
	 */
	BTIF_LOG_D("%s: wait cos_idle_loop...", __func__);
	retry = POS_POLLING_RTY_LMT;
	do {
		mcu_idle = DRV_Reg32(BGF_MCU_CFG_SW_DBG_CTL);
		BTIF_LOG_D("MCU sw_dbg_ctl = 0x%08x, retry = %d", mcu_idle, retry);
		BTIF_LOG_D("MCU pc = 0x%08x", DRV_Reg32(CONN_MCU_PC));
		if (mcu_idle == 0x1D1E)
			break;
#ifdef MTK_FPGA_ENABLE
		rtos_mdelay(1000);
#else
		rtos_mdelay(5);
#endif
		retry--;
	} while (retry > 0);

	g_bgfsys_on = 1;
	BTIF_LOG_I("BGFSYS power on finish");
	return 0;

error:
	/* release conn_infra force on */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, CONN_INFRA_WAKE_BT_B);
	return -1;
}

//---------------------------------------------------------------------------
/* btmtk_bgfsys_power_off
 *
 *    BGF MCU power off sequence
 *
 * Arguments:
 *    N/A
 *
 * Return Value:
 *     0 if success, otherwise error code
 *
 */
int btmtk_bgfsys_power_off(void)
{
	unsigned int value = 0;
	int32_t retry = POS_POLLING_RTY_LMT;
	int32_t ret = 0;

	BTIF_LOG_D("BGFSYS power off start");
	/* wake up conn_infra off */
	SET_BIT(CONN_INFRA_WAKEUP_BT, BIT(0)); // 0x180601A8[0]
	// consys_polling_chipid();

	/* enable bt2conn slp_prot tx en */
	SET_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL, BT2CONN_SLP_PROT_TX_EN_B); // 0x18001560[0]
	/* polling bt2conn slp_prot tx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_TX_ACK_B &
				DRV_Reg32(CONN_INFRA_BT2CONN_GALS_SLP_STATUS); // 0x18001564[23]
		BTIF_LOG_D("bt2conn slp_prot tx ack = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		ret = -1;

	/* enable bt2conn slp_prot rx en */
	SET_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL, BT2CONN_SLP_PROT_RX_EN_B); // 0x18001560[4]
	/* polling bt2conn slp_prot rx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_RX_ACK_B &
				DRV_Reg32(CONN_INFRA_BT2CONN_GALS_SLP_STATUS); // 0x18001564[22]
		BTIF_LOG_D("bt2conn slp_prot rx ack = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		ret = -2;

	/* enable conn2bt slp_prot tx en */
	SET_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL, CONN2BT_SLP_PROT_TX_EN_B); // 0x18001550[0]
	/* polling conn2bt slp_prot tx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_TX_ACK_B &
				DRV_Reg32(CONN_INFRA_CONN2BT_GALS_SLP_STATUS); // 0x18001554[23]
		BTIF_LOG_D("conn2bt slp_prot tx ack = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		ret = -2;

	/* enable conn2bt slp_prot rx en */
	SET_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL, CONN2BT_SLP_PROT_RX_EN_B); // 0x18001550[4]
	/* polling conn2bt slp_prot rx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_RX_ACK_B &
				DRV_Reg32(CONN_INFRA_CONN2BT_GALS_SLP_STATUS); // 0x18001554[22]
		BTIF_LOG_D("conn2bt slp_prot rx ack = 0x%08x", value);
		rtos_mdelay(1);
		retry--;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		ret = -1;

	/* disable bt function en */
	CLR_BIT(CONN_INFRA_CFG_BT_PWRCTLCR0, BT_FUNC_EN_B); // 0x18001208[0]

	/* power off bgfsys on */
	value = DRV_Reg32(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL);
	value = (value | CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_OFF_HIGH) &
			CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_OFF_LOW;
	DRV_WriteReg32(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL, value);
	value = DRV_Reg32(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL);
	BTIF_LOG_D("addr = 0x%x, value = 0x%x",
				CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL, value);

	/* reset n10 cpu core */
	CLR_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST, BGF_CPU_SW_RST_B); // 0x180000124[0]

	/* clear bt_emi_req */
	SET_BIT(CONN_INFRA_CFG_EMI_CTL_BT_EMI_REQ_BT, BT_EMI_CTRL_BIT);
	CLR_BIT(CONN_INFRA_CFG_EMI_CTL_BT_EMI_REQ_BT, BT_EMI_CTRL_BIT);

	/* release conn_infra force on */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0)); // 0x180601A8[0]

	if (ret == -2) {
		// TODO: TBD
		// conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_BT, "Power off fail");
	}
	g_bgfsys_on = 0;
	BTIF_LOG_I("BGFSYS power off finish");
	return ret;
}

//---------------------------------------------------------------------------
static int btmtk_send_wmt_download_cmd(uint8_t *cmd, int cmd_len,
									   uint8_t *event, int event_len,
									   struct _Section_Map *sectionMap,
									   uint8_t fw_state, uint8_t dma_flag)
{
	int payload_len = 0;
	int ret = -1;
	uint8_t expected_evt[] = {0x04, 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00}; // evt[7] is status

	if (cmd == NULL || event == NULL || sectionMap == NULL) {
		BTIF_LOG_E("%s: invalid parameter!", __func__);
		return ret;
	}

	/* need refine this cmd to mtk_wmt_hdr struct*/
	/* prepare HCI header */
	cmd[0] = 0x01;
	cmd[1] = 0x6F;
	cmd[2] = 0xFC;

	/* prepare WMT header */
	cmd[4] = 0x01;
	cmd[5] = 0x01; /* opcode */

	if (fw_state == 0) {
		/* prepare WMT DL cmd */
		payload_len = SEC_MAP_NEED_SEND_SIZE + 2;

		cmd[3] = (payload_len + 4) & 0xFF; /* length*/
		cmd[6] = payload_len & 0xFF;
		cmd[7] = (payload_len >> 8) & 0xFF;
		cmd[8] = 0x00;     /* which is the FW download state 0 */
		cmd[9] = dma_flag; /* 1:using DMA to download, 0:using legacy wmt cmd */
		cmd_len = SEC_MAP_NEED_SEND_SIZE + PATCH_HEADER_SIZE;

		memcpy(&cmd[10], (u8 *)sectionMap + FW_ROM_PATCH_SEC_MAP_SIZE -
				SEC_MAP_NEED_SEND_SIZE, SEC_MAP_NEED_SEND_SIZE);
		btif_util_dump_buffer("fw_state:0, Send wmt cmd: ", cmd, cmd_len, 0);
		ret = btmtk_send_hci_cmd(cmd, cmd_len, expected_evt, sizeof(expected_evt), NULL, 0);
		if (ret < 0) {
			BTIF_LOG_E("%s: send wmd dl cmd failed, terminate!", __func__);
			return PATCH_ERR;
		}
		if (ret == event_len)
			return expected_evt[7];
		return PATCH_ERR;

	} else
		BTIF_LOG_E("%s: fw state is error!", __func__);

	return PATCH_ERR;
}

//---------------------------------------------------------------------------
static int btmtk_load_fw_patch_using_wmt_cmd(uint8_t *wmt, uint8_t *event,
											 int event_len, uint32_t start_addr,
											 int offset, unsigned int patch_len)
{
	int ret = 0;
	unsigned int cur_len = 0;
	unsigned int sent_len;
	int first_block = 1;
	uint8_t phase;

	if (wmt == NULL) {
		BTIF_LOG_W("%s, invalid parameters!", __func__);
		ret = -1;
		goto exit;
	}

	/* loading rom patch */
	while (1) {
		unsigned int sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;

		sent_len = (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

		if (sent_len > 0) {
			if (first_block == 1) {
				if (sent_len < sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE1;
				first_block = 0;
			} else if (sent_len == sent_len_max) {
				if (patch_len - cur_len == sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE2;
			} else {
				phase = PATCH_PHASE3;
			}

			/* prepare HCI header */
			wmt[0] = 0x02;
			wmt[1] = 0x6F;
			wmt[2] = 0xFC;
			wmt[3] = (sent_len + 5) & 0xFF;
			wmt[4] = ((sent_len + 5) >> 8) & 0xFF;

			/* prepare WMT header */
			wmt[5] = 0x01;
			wmt[6] = 0x01;
			wmt[7] = (sent_len + 1) & 0xFF;
			wmt[8] = ((sent_len + 1) >> 8) & 0xFF;

			wmt[9] = phase;
			memcpy(&wmt[10], (void *)(start_addr + offset + cur_len), sent_len);
			// if (phase == PATCH_PHASE3) {}

			ret = btmtk_send_hci_cmd(wmt, sent_len + PATCH_HEADER_SIZE,
									 event, event_len, NULL, 0);
			if (ret < 0) {
				BTIF_LOG_E("%s: send patch failed, terminate", __func__);
				goto exit;
			}

			cur_len += sent_len;
			BTIF_LOG_D("Load FW: len = %d, phase = %d", cur_len, phase);
		} else
			break;
	}
exit:
	return ret;
}

//---------------------------------------------------------------------------
static int btmtk_load_fw_using_emi(struct _Section_Map *sectionMap, uint32_t bin_start_addr)
{
	int ret = 0;
#ifdef MTK_BT_FW_DL_TO_EMI_ENABLE
	unsigned int sent_len = 0;
	unsigned int cur_len = 0;
	unsigned int dl_size = 0;
	unsigned int section_offset = 0;
	unsigned int sent_len_max = UPLOAD_PATCH_UNIT;
	unsigned int emi_buf_max_size = BGF_EMI_BUFFER_SIZE - 0x30;
	uint8_t *p_emi_buf = bgf_emi_buffer + 0x30;
	uint32_t bin_addr = 0;

	BTIF_LOG_I("%s: copy firmware code from flash to EMI", __func__);
	section_offset = sectionMap->u4SecOffset;
	bin_addr = bin_start_addr + section_offset;
	dl_size = sectionMap->bin_info_spec.u4DLSize;
	if (dl_size > BGF_EMI_BUFFER_SIZE - 0x30) {
		BTIF_LOG_E("%s, emi buf size too small", __func__);
		return -1;
	}

	while (1) {
		sent_len = (dl_size - cur_len) >= sent_len_max ? sent_len_max : (dl_size - cur_len);
		if (sent_len > 0 && cur_len + sent_len <= emi_buf_max_size) {
			memcpy(p_emi_buf + cur_len, (void *)(bin_addr + cur_len), sent_len);
			cur_len += sent_len;
		} else {
			BTIF_LOG_I("%s: copy to EMI done, cur_len = 0x%x, sent_len = 0x%x",
					   __func__, cur_len, sent_len);
			break;
		}
	}
#else
	BTIF_LOG_E("%s, emi copy not support", __func__);
#endif
	return ret;
}

//---------------------------------------------------------------------------
static int btmtk_load_fw_using_hif(struct _Section_Map *sectionMap, uint32_t bin_start_addr)
{
	uint8_t *cmd;
	int ret = -1;
	int retry = 20;
	int patch_status = 0;
	unsigned int dl_size = 0;
	unsigned int section_offset = 0;
	uint8_t evt[] = {0x04, 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00}; // evt[7] is status

	dl_size = sectionMap->bin_info_spec.u4DLSize;
	section_offset = sectionMap->u4SecOffset;

	cmd = (uint8_t *)btif_util_malloc(UPLOAD_PATCH_UNIT);
	if (!cmd) {
		BTIF_LOG_E("%s: alloc memory failed", __func__);
		goto exit;
	}

	do {
		patch_status = btmtk_send_wmt_download_cmd(cmd, 0, evt, sizeof(evt),
												   sectionMap, 0, PATCH_DOWNLOAD_USING_WMT);
		//BTIF_LOG_D("dlfw_hif: st=%d", patch_status);

		if (patch_status > PATCH_READY || patch_status == PATCH_ERR) {
			BTIF_LOG_E("%s: patch_status error", __func__);
			goto exit;
		} else if (patch_status == PATCH_READY) {
			BTIF_LOG_I("%s: no need to load this section", __func__);
			goto exit;
		} else if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
			rtos_mdelay(100);
			retry--;
		} else if (patch_status == PATCH_NEED_DOWNLOAD) {
			break; /* Download ROM patch directly */
		}
	} while (retry > 0);

	if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
		BTIF_LOG_W("%s: Hold by another fun more than 2 seconds", __func__);
		goto exit;
	}

	/* using legacy wmt cmd to download fw patch */
	ret = btmtk_load_fw_patch_using_wmt_cmd(cmd, evt, sizeof(evt),
											bin_start_addr, section_offset, dl_size);
	if (ret < 0) {
		BTIF_LOG_E("%s: btmtk_load_fw_patch_using_wmt_cmd failed!", __func__);
		goto exit;
	}

exit:
	btif_util_free(cmd);
	return ret;
}

//---------------------------------------------------------------------------
static void stripSecureBootWrapper(uint32_t *addr, uint32_t *size)
{
#ifdef MTK_BT_SIGN_ENABLE
	(void)scott_image_strip(addr, size);
#endif
}

int btmtk_load_ilm_data(uint32_t start_addr)
{
	int ret;

	ret = btmtk_load_fw_using_hif(g_ilm_data_section_map, start_addr);
	return ret;
}

uint8_t *btmtk_get_fw_version(void)
{
	uint32_t start_addr = XIP_BT_START;
	uint32_t fw_length  = XIP_BT_LEGNTH;

	stripSecureBootWrapper(&start_addr, &fw_length);
	bt_fw_start_addr = start_addr;

	struct fw_patch_header_t *patchHdr = (struct fw_patch_header_t *)start_addr;

	return patchHdr->ucDateTime;
}

uint32_t btmtk_get_fw_size(void)
{
	uint32_t start_addr = XIP_BT_START;
	uint32_t fw_length  = XIP_BT_LEGNTH;
	uint8_t i = 0;
	uint8_t sec_num = 0;
	static uint32_t fw_size;
	struct _Global_Descr *globalDescr = NULL;
	struct _Section_Map *sectionMap = NULL;

	if (fw_size > 0)
		return fw_size;

	stripSecureBootWrapper(&start_addr, &fw_length);
	bt_fw_start_addr = start_addr;

	globalDescr = (struct _Global_Descr *)(bt_fw_start_addr + FW_ROM_PATCH_HEADER_SIZE);
	sec_num = globalDescr->u4SectionNum;
	while (i < sec_num) {
		sectionMap = (struct _Section_Map *)(bt_fw_start_addr + FW_ROM_PATCH_HEADER_SIZE +
					  FW_ROM_PATCH_GD_SIZE + FW_ROM_PATCH_SEC_MAP_SIZE * i);
		fw_size += sectionMap->bin_info_spec.u4DLSize;
		i++;
	}
	BTIF_LOG_I("fw size = %lu, bt_rom_len: %lu", fw_size, (uint32_t)BT_LENGTH);
	return fw_size;
}

/*
 * flash virtural address start from 0x18000000
 *       physical address start from 0x90000000
 * here we get physical address content, in order to see whether flash has been covered
 */
int btmtk_send_fw_rom_patch(enum dl_fw_phase_t phase)
{
	uint32_t start_addr = XIP_BT_START;
	uint32_t fw_length  = XIP_BT_LEGNTH;
	uint32_t start_addr_db;
	unsigned int loop_count = 0;
	int ret = -1;
	unsigned int section_num = 0;
	unsigned int section_type = 0;
	unsigned int binary_type = 0;
	unsigned int total_size = 0;
	struct _Section_Map *sectionMap = NULL;
	struct _Global_Descr *globalDescr = NULL;
	struct _Global_Descr *globalDescr_db = NULL;

	if (phase != DL_FW_PHASE1 && phase != DL_FW_PHASE2)
		return ret;

	stripSecureBootWrapper(&start_addr, &fw_length);
	bt_fw_start_addr = start_addr;

	// 0x90000000 is physical addr
	start_addr_db = (start_addr & 0x00FFFFFF) + 0x90000000;

	// Global descriptor: 64 bytes
	globalDescr = (struct _Global_Descr *)(start_addr + FW_ROM_PATCH_HEADER_SIZE);
	globalDescr_db = (struct _Global_Descr *)(start_addr_db + FW_ROM_PATCH_HEADER_SIZE);
	BTIF_LOG_I("Send FW: loading rom patch vir_addr: 0x%lx, phy_addr: 0x%lx",
				start_addr, start_addr_db);

	section_num = globalDescr->u4SectionNum;
	BTIF_LOG_I("Send FW: sec_num = 0x%08x, phase %d, sec_num at phy = 0x%08x",
				section_num, phase, globalDescr_db->u4SectionNum);
	// use UNUSED macro for debug warnig
	UNUSED(globalDescr_db);

	// Display patch version information
	if (phase == DL_FW_PHASE1) {
		struct fw_patch_header_t *patchHdr = (struct fw_patch_header_t *)start_addr;

		BTIF_LOG_I("FW Ver = %s", patchHdr->ucDateTime);
		/*
		 * BTIF_LOG_I("Hw Ver = 0x%04x", patchHdr->u2HwVer);
		 * BTIF_LOG_I("Sw Ver = 0x%04x", patchHdr->u2SwVer);
		 * BTIF_LOG_I("Magic Number = 0x%08lx", patchHdr->u4MagicNum);
		 * BTIF_LOG_I("Platform = %c%c%c%c",
		 *            patchHdr->ucPlatform[0], patchHdr->ucPlatform[1],
		 *            patchHdr->ucPlatform[2], patchHdr->ucPlatform[3]);
		 */
		// use UNUSED macro for debug warnig
		UNUSED(patchHdr);
	}

	do {
		// section map: 64 bytes
		sectionMap = (struct _Section_Map *)(start_addr + FW_ROM_PATCH_HEADER_SIZE +
					  FW_ROM_PATCH_GD_SIZE + FW_ROM_PATCH_SEC_MAP_SIZE * loop_count);
		total_size += sectionMap->bin_info_spec.u4DLSize;
		BTIF_LOG_I("Send FW: loop_count = %d, total size = %d bytes", loop_count, total_size);

		if (total_size > (unsigned int)BT_LENGTH) {
			BTIF_LOG_E("Send FW: FW size is too large(%d > %d)",
						total_size, (unsigned int)BT_LENGTH);
			return ret;
		}

		section_type = sectionMap->u4SecType;
		binary_type = sectionMap->bin_info_spec.u4BinaryType;
		BTIF_LOG_I("Send FW: section_type = 0x%x, binary_type = 0x%x", section_type, binary_type);

		if (phase == DL_FW_PHASE1) {
			// BT emi.text section, action: copy to EMI memory
			if (GET_SECTION_TYPE(section_type) == SD_SECTION_TYPE_SUBSYS_BT &&
			    GET_BINARY_TYPE(binary_type) == SD_SECTION_BINARY_TYPE_BT_EMI_TEXT) {
				if (psram_copy) {
					BTIF_LOG_I("Send FW: copy BT_EMI TEXT and DATA section, size = %u bytes",
								sectionMap->bin_info_spec.u4DLSize);
					return btmtk_load_fw_using_emi(sectionMap, start_addr);
				}
				BTIF_LOG_I("Send FW: skip PSRAM FW copy");
				return 0;
			}
			continue;
		}

		if (GET_SECTION_TYPE(section_type) == SD_SECTION_TYPE_SUBSYS_BT) {
			// ilm.data section, action: download via HIF
			if (GET_BINARY_TYPE(binary_type) == SD_SECTION_BINARY_TYPE_BT_ILM_DATA) {
				if (hif_dl) {
					BTIF_LOG_I("Send FW: copy BT DLM section");
					ret = btmtk_load_fw_using_hif(sectionMap, start_addr);
					g_ilm_data_section_map = sectionMap;
				} else
					continue;

				// BT ilm.text section, action: download via HIF
			} else if (GET_BINARY_TYPE(binary_type) ==
					   SD_SECTION_BINARY_TYPE_BT_ILM_TEXT_EX9_DATA) {
				if (hif_dl) {
					BTIF_LOG_I("Send FW: copy BT RAM section");
					ret = btmtk_load_fw_using_hif(sectionMap, start_addr);
				} else
					continue;

			} else if (GET_BINARY_TYPE(binary_type) == SD_SECTION_BINARY_TYPE_BT_EMI_TEXT) {
				BTIF_LOG_I("Send FW: skip EMI section");
				continue;

			} else {
				BTIF_LOG_E("Send FW: unexpected binary_type(%d)", binary_type);
				break;
			}

		} else if (GET_SECTION_TYPE(section_type) == SD_SECTION_TYPE_SUBSYS_MCU) {
			// MCU ilm.text section, action: download via HIF
			if (GET_BINARY_TYPE(binary_type) == SD_SECTION_BINARY_TYPE_BT_PATCH_TEXT_DATA) {
				if (hif_dl) {
					BTIF_LOG_I("Send FW: copy patch section");
					ret = btmtk_load_fw_using_hif(sectionMap, start_addr);
				} else
					continue;
			} else {
				BTIF_LOG_E("Send FW: unexpected binary_type(%d)", binary_type);
				break;
			}

		} else {
			BTIF_LOG_E("Send FW: unexpected section(%d)", section_type);
			break;
		}

		if (ret < 0) {
			BTIF_LOG_E("Send FW: load fw failed. num = %d, ret = %d", loop_count, ret);
			break;
		}
	} while (++loop_count < section_num);

	/*FW Download finished */
	if (loop_count == section_num) {
		BTIF_LOG_D("Send FW: load bt fw... Done");
		ret = 0;
	}
	return ret;
}

int btmtk_start_calibration(void)
{
	int ret = 0;

	ret = btmtk_send_hci_cmd(BT_RF_CALI_CMD, sizeof(BT_RF_CALI_CMD),
							 BT_RF_CALI_EVENT, sizeof(BT_RF_CALI_EVENT), NULL, 0);
	if (ret > 0)
		BTIF_LOG_I("BT RF calibration success");
	else
		BTIF_LOG_E("BT RF calibration fail");

	return ret;
}

int btmtk_func_ctrl(int b_on)
{
	int ret = 0;

	BTIF_LOG_D("%s, send BT power %s cmd", __func__, b_on ? "on" : "off");
	if (b_on) {
		// need to reload ram data everytime BT on in MT7933
		if (hif_dl)
			btmtk_load_ilm_data(bt_fw_start_addr);

		ret = btmtk_send_hci_cmd(BT_PROC_ON_CMD, sizeof(BT_PROC_ON_CMD),
								 BT_PROC_ON_OFF_EVENT, sizeof(BT_PROC_ON_OFF_EVENT), NULL, 0);
	} else {
		ret = btmtk_send_hci_cmd(BT_PROC_OFF_CMD, sizeof(BT_PROC_OFF_CMD),
								 BT_PROC_ON_OFF_EVENT, sizeof(BT_PROC_ON_OFF_EVENT), NULL, 0);
	}

	if (ret > 0)
		BTIF_LOG_I("%s, BT power %s success", __func__, b_on ? "on" : "off");
	else
		BTIF_LOG_E("%s, BT power %s fail", __func__, b_on ? "on" : "off");

	return (ret > 0 ? 0 : ret);
}

bool btmtk_is_coredump_now(void)
{
	bool ret = pdFALSE;

	taskENTER_CRITICAL();
	if (bt_drv_info.fw_coredump_st > BT_FW_COREDUMP_INIT&&
		bt_drv_info.fw_coredump_st <= BT_FW_COREDUMP_END) {
		ret = pdTRUE;
	}
	taskEXIT_CRITICAL();

	if (ret)
		BTIF_LOG_I("%s: state is %d", __func__, bt_drv_info.fw_coredump_st);

	return ret;
}

void btmtk_set_coredump_state(enum bt_fw_coredump_state_t state)
{
	taskENTER_CRITICAL();
	bt_drv_info.fw_coredump_st = state;
	taskEXIT_CRITICAL();
}

enum bt_fw_coredump_state_t btmtk_get_coredump_state(void)
{
	enum bt_fw_coredump_state_t st = BT_FW_COREDUMP_UNKNOWN;

	taskENTER_CRITICAL();
	st = bt_drv_info.fw_coredump_st;
	taskEXIT_CRITICAL();

	return st;
}

int btmtk_trigger_fw_assert(void)
{
	int ret = 0;

	BTIF_LOG_I("%s: trigger fw assert", __func__);

	if (btmtk_is_coredump_now() == pdTRUE) {
		BTIF_LOG_I("%s, now is fw coredumping, return", __func__);
		return ret;
	}

	btmtk_set_coredump_state(BT_FW_COREDUMP_CMD_SEND_START);

	ret = btmtk_send_hci_cmd(BT_PROC_COREDUMP_CMD, sizeof(BT_PROC_COREDUMP_CMD),
							 NULL, 0, NULL, 0);
	if (ret < 0) {
#ifdef BUFFER_DEBUG_TRACE
		btif_buffer_debug();
#endif
		btmtk_dump_sleep_fail_reg();
		dump_bus_hang_reg();
		btmtk_dump_reg();
		btmtk_set_coredump_state(BT_FW_COREDUMP_INIT);
		return ret;
	}

	btmtk_set_coredump_state(BT_FW_COREDUMP_CMD_SEND_END);
	return ret;
}

static bool is_host_view_cr(unsigned int addr, unsigned int *host_view)
{
	if (addr >= 0x7C000000 && addr <= 0x7Cffffff) {
		if (host_view)
			*host_view = ((addr - 0x7C000000) + 0x60000000);
		return true;
	} else if (addr >= 0x60000000 && addr <= 0x60ffffff) {
		if (host_view)
			*host_view = addr;
		return true;
	}

	return false;
}

static unsigned int btmtk_coredump_setup_dynamic_remap(unsigned int base, unsigned int length)
{
	unsigned int map_len = length > DYNAMIC_MAP_MAX_SIZE ? DYNAMIC_MAP_MAX_SIZE : length;
	uint32_t vir_addr = 0;

	if (is_host_view_cr(base, NULL)) {
		BTIF_LOG_I("Host view CR: 0x%x, skip dynamic remap\n", base);
		return length;
	}

	/* Expand to request size */
	vir_addr = BGF_CONN2BGF_REMAP_SEG_1;
	if (vir_addr)
		DRV_WriteReg32(vir_addr, BGF_CONN2BGF_REMAP_SEG_0_ADDR + map_len);

	/* Setup map base */
	vir_addr = BGF_CONN2BGF_REMAP_1;
	if (vir_addr)
		DRV_WriteReg32(vir_addr, base);

	return map_len;
}

static void btmtk_coredump_init_dump_regions(void)
{
	uint32_t addr = 0;
	uint32_t base = 0;
	unsigned int size = 0;
	unsigned int i = 0;

	BTIF_LOG_I("%s", __func__);
	addr = DRV_Reg32(BGF_COREDUMP_CTRL_INFO_ADDR_ADDR);
	btmtk_coredump_setup_dynamic_remap(addr, DRV_Reg32(BGF_COREDUMP_CTRL_INFO_ADDR_LEN));
	base = BGF_CONN2BGF_REMAP_SEG_0_ADDR;
	size = DRV_Reg32(base + 0x18);

	BTIF_LOG_I("mem: addr:%lx, size:%u", addr, size);
	if (size == 0) {
		BTIF_LOG_I("%s, REMAP_SEG_0 size = 0, return", __func__);
		return;
	}

	mem_dump_size = size;
	if (!mem_dump_region)
		mem_dump_region = (struct coredump_region *)pvPortMalloc(
						   size * sizeof(struct coredump_region));
	memset(mem_dump_region, 0, size * sizeof(struct coredump_region));

	for (i = 0; i < size; i++) {
		memcpy(mem_dump_region[i].name,
			   (uint32_t *)(base + BGF_COREDUMP_CTRL_MEM_REGION_START + i * 12), 4);
		mem_dump_region[i].base = DRV_Reg32(base + BGF_COREDUMP_CTRL_MEM_REGION_START +
											i * 12 + 4);
		mem_dump_region[i].length = DRV_Reg32(base + BGF_COREDUMP_CTRL_MEM_REGION_START +
											  i * 12 + 8);
		BTIF_LOG_D("mem: name:%s, base:%x, len:%x",
					mem_dump_region[i].name, mem_dump_region[i].base, mem_dump_region[i].length);
	}

	addr = DRV_Reg32(BGF_COREDUMP_CR_REGION_ADDR_ADDR);
	size = DRV_Reg32(BGF_COREDUMP_CR_REGION_ADDR_LEN);
	if (size == 0) {
		BTIF_LOG_I("%s, CR_REGION size = 0, return", __func__);
		return;
	}
	btmtk_coredump_setup_dynamic_remap(addr, size * 8);

	cr_dump_size = size;
	if (!cr_dump_region)
		cr_dump_region = (struct coredump_region *)pvPortMalloc(
						  size * sizeof(struct coredump_region));
	memset(cr_dump_region, 0, size * sizeof(struct coredump_region));

	BTIF_LOG_I("cr: addr:%x, size:%d", addr, size);
	for (i = 0; i < size; i++) {
		cr_dump_region[i].base = DRV_Reg32(base + i * 8);
		cr_dump_region[i].length = DRV_Reg32(base + i * 8 + 4);
		BTIF_LOG_D("cr: base:%x, len:%x", cr_dump_region[i].base, cr_dump_region[i].length);
	}
}

static void btmtk_coredump_deinit_dump_regions(void)
{
	if (mem_dump_region) {
		vPortFree(mem_dump_region);
		mem_dump_region = NULL;
	}
	if (cr_dump_region) {
		vPortFree(cr_dump_region);
		cr_dump_region = NULL;
	}
}

static void btmtk_coredump_task_main(void *arg)
{
	uint32_t ctrl_info_base = 0;
	uint32_t ctrl_info_addr = 0;
	uint32_t dump_buff_base = 0;
	uint32_t dump_buff_sz = 0;
	uint32_t addr = 0;
	uint32_t value = 0;
	char per_cr[11];
	uint32_t idx = 0, i = 0;
	char string[CORE_DUMP_LINE_SIZE + 1] = {0};

	while (1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		/* get coredump information */
		btmtk_coredump_init_dump_regions();
		/* find where coredump plain text is */
		ctrl_info_addr = DRV_Reg32(BGF_COREDUMP_CTRL_INFO_ADDR_ADDR);
		btmtk_coredump_setup_dynamic_remap(ctrl_info_addr, DRV_Reg32(BGF_COREDUMP_CTRL_INFO_ADDR_LEN));
		ctrl_info_base = BGF_CONN2BGF_REMAP_SEG_0_ADDR;
		total_coredump_sz = DRV_Reg32(ctrl_info_base + 0x10);

#ifdef BUFFER_DEBUG_TRACE
		btif_buffer_debug();
#endif
		// Current plf will drop log if too many logs, so add delay for avalabe buffer
		vTaskDelay(pdMS_TO_TICKS(40));
		btmtk_dump_sleep_fail_reg();
		vTaskDelay(pdMS_TO_TICKS(70));
		dump_bus_hang_reg();
		vTaskDelay(pdMS_TO_TICKS(30));
		btif_dump_reg();
		vTaskDelay(pdMS_TO_TICKS(30));

		BTIF_LOG_I("coredump: plain text size: %u bytes", total_coredump_sz);

		ctrl_info_base = BGF_CONN2BGF_REMAP_SEG_0_ADDR;
		dump_buff_base = DRV_Reg32(BGF_COREDUMP_DUMP_BUFF_ADDR_ADDR);
		dump_buff_sz = DRV_Reg32(BGF_COREDUMP_DUMP_BUFF_ADDR_LEN);
		btmtk_coredump_setup_dynamic_remap(dump_buff_base, dump_buff_sz);

		if (btmtk_get_fw_size() > CONSYS_BGF_COREDUMP_FLASH_ADDR - BT_BASE) {
			BTIF_LOG_I("fw_size(%lu) larger than bt_rom(%lu) minus 4k!!!",
						btmtk_get_fw_size(), (uint32_t)BT_LENGTH);
		} else {
			/* copy first 4KB coredump to flash */
			hal_flash_erase(CONSYS_BGF_COREDUMP_FLASH_ADDR, HAL_FLASH_BLOCK_4K);
			hal_flash_write(CONSYS_BGF_COREDUMP_FLASH_ADDR,
							(const uint8_t *)ctrl_info_base, CORE_DUMP_INFO_LEN);
		}

#ifdef MTK_BT_PICUS_ENABLE
		picus_coredump_out(DUMP_PLAIN, NULL, (unsigned char *)ctrl_info_base, total_coredump_sz);
		for (i = 0; i < CORE_DUMP_PRINT_LINE; i++) {
			memset(string, 0, CORE_DUMP_LINE_SIZE + 1);
			memcpy(string, (void *)(ctrl_info_base + i * CORE_DUMP_LINE_SIZE), CORE_DUMP_LINE_SIZE);
			BTIF_LOG_I("%s", string);
		}
#else
		for (i = 0; i < (total_coredump_sz / CORE_DUMP_LINE_SIZE); i++) {
			memcpy(string, (void *)(ctrl_info_base + i * CORE_DUMP_LINE_SIZE), CORE_DUMP_LINE_SIZE);
			BTIF_LOG_I("%s", string);
		}
		memset(string, 0, CORE_DUMP_LINE_SIZE + 1);
		memcpy(string, (void *)(ctrl_info_base + i * CORE_DUMP_LINE_SIZE),
				total_coredump_sz - (i * CORE_DUMP_LINE_SIZE));
		BTIF_LOG_I("%s", string);
#endif
		if (!bt_driver_is_on() && !bt_drv_info.cfg.wait_fw_dump_over)
			goto dumpend;
		BTIF_LOG_I("coredump: dump cr, size: %lu", cr_dump_size);
		for (idx = 0; idx < cr_dump_size; idx++) {
			if (cr_dump_region[idx].length == 0)
				continue;
			btmtk_coredump_setup_dynamic_remap(cr_dump_region[idx].base,
											   cr_dump_region[idx].length);
			for (i = 0; i < cr_dump_region[idx].length; i += 4) {
				addr = cr_dump_region[idx].base + i;
				value = DRV_Reg32(ctrl_info_base + i);
				memset(per_cr, 0, sizeof(per_cr));
				per_cr[0] = '[';
				memcpy(&per_cr[1], &addr, 4);
				per_cr[5] = ',';
				memcpy(&per_cr[6], &value, 4);
				per_cr[10] = ']';
#ifdef MTK_BT_PICUS_ENABLE
				picus_coredump_out(DUMP_CR, NULL, (unsigned char *)per_cr, sizeof(per_cr));
#endif
			if (!bt_driver_is_on() && !bt_drv_info.cfg.wait_fw_dump_over)
				goto dumpend;
			}
			BTIF_LOG_I("coredump: dump cr, idx: %d", idx);
		}
		BTIF_LOG_I("coredump: dump cr end");

		BTIF_LOG_I("coredump: dump mem, size: %lu", mem_dump_size);
		for (idx = 0; idx < mem_dump_size; idx++) {
			btmtk_coredump_setup_dynamic_remap(mem_dump_region[idx].base,
											   mem_dump_region[idx].length);
#ifdef MTK_BT_PICUS_ENABLE
			BTIF_LOG_I("dump: === %s ===", mem_dump_region[idx].name);
			picus_coredump_out(DUMP_MEM, (unsigned char *)(&mem_dump_region[idx]),
							   (unsigned char *)ctrl_info_base, mem_dump_region[idx].length);
#endif
			if (!bt_driver_is_on() && !bt_drv_info.cfg.wait_fw_dump_over)
				goto dumpend;
		}
#ifdef MTK_BT_DRV_CHIP_RESET
		/*
		 * main task done and other task may continue(a.s. boots),
		 * so should do chip reset in another task
		 */
		uint32_t emi_ctrl_st = 0;
		emi_ctrl_st = BTIF_READ32(BGF_EMI_CTL_ST);
		BTIF_LOG_I("coredump: emi_ctrl_st(0x%08x) = 0x%08lx", BGF_EMI_CTL_ST, emi_ctrl_st);
		if (emi_ctrl_st & BGF_SW_IRQ_ASSERT_SUBSYS)
			g_rst_info.rst_type = SUBSYS_CHIP_RESET;
		else if (emi_ctrl_st & BGF_SW_IRQ_ASSERT_WHOLE)
			g_rst_info.rst_type = WHOLE_CHIP_RESET;

		btmtk_set_coredump_state(BT_FW_COREDUMP_RESET_START);
		btmtk_reset_notify(pdFALSE);
#endif /* #ifdef MTK_BT_DRV_CHIP_RESET */

dumpend:
		btmtk_coredump_deinit_dump_regions();
		btmtk_set_coredump_state(BT_FW_COREDUMP_END);
		if (bt_driver_is_on() && fw_assert_nty)
			fw_assert_nty();
	}
}

static void btmtk_coredump_handler(hal_nvic_irq_t irq)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	btif_irq_ctrl(irq, FALSE);

	if (bt_drv_info.fw_coredump_st == BT_FW_COREDUMP_START) {
		BTIF_LOG_I("%s, now is saving fwdump log, return",
					__func__, bt_drv_info.fw_coredump_st);
		btif_irq_ctrl(irq, TRUE);
		return;
	}
	BTIF_LOG_I("%s: %s trigger, BT core dump!!!!!!!!", __func__,
				(bt_drv_info.fw_coredump_st == BT_FW_COREDUMP_CMD_SEND_END) ? "Manual" : "FW");

	bt_drv_info.fw_coredump_st = BT_FW_COREDUMP_START;
	if (!btmtk_coredump_task) {
		bt_drv_info.fw_coredump_st = BT_FW_COREDUMP_END;
		if (fw_assert_nty)
			fw_assert_nty();
	} else {
		vTaskNotifyGiveFromISR(btmtk_coredump_task, &xHigherPriorityTaskWoken);
	}

	/* clear BGF SW interrupt */
	SET_BIT(BGF_EMI_CTL_CLR, BGF_SW_IRQ_CLR_BT);
	btif_irq_ctrl(irq, TRUE);
}

void btmtk_read_coredump_info(void)
{
	uint8_t *string = NULL;
	uint8_t *pstr = NULL;
	uint32_t coredump_info_addr;
	uint16_t i = 0, j = 0;

	coredump_info_addr = COREDUMP_FLASH_READ_ADDR;
	string = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * CORE_DUMP_LINE_MAX_SIZE);
	if (!string) {
		BTIF_LOG_E("%s, malloc error", __func__);
		return;
	}

	for (i = 0; i < CORE_DUMP_INFO_LEN; i++) {
		pstr = (uint8_t *)(coredump_info_addr + i);
		if (*pstr == '\n') {
			memset(string, 0, CORE_DUMP_LINE_MAX_SIZE);
			memcpy(string, (uint8_t *)(coredump_info_addr + j),
				   i - j < CORE_DUMP_LINE_MAX_SIZE ? i - j : CORE_DUMP_LINE_MAX_SIZE - 1);
			j = i + 1;
			BTIF_LOG_I("%s", string);
		}
	}

	vPortFree(string);
}

int btmtk_init(void)
{
	int ret = 0;

	BTIF_LOG_I("%s", __func__);

	if (btif_platform_check_vfifo_buf_align() == false) {
		BTIF_LOG_E("vfifo buffer did not align!");
		return -1;
	}

	bt_rx_buffer.mtx = btif_util_mutex_create();
	if (bt_rx_buffer.mtx == NULL) {
		BTIF_LOG_E("create rx ring buf  mutex fail");
		return -1;
	}
	memset((void *)&bt_drv_info, 0, sizeof(bt_drv_info));
	btmtk_parse_bt_config();

#ifdef BTIF_MAIN_TASK_TRACE
	btif_main_task_record_init();
#endif
#ifdef BUFFER_DEBUG_TRACE
	btif_buffer_debug_init();
#endif
	ret = BTIF_open();
	if (ret != 0) {
		btif_util_sema_delete(bt_rx_buffer.mtx);
		bt_rx_buffer.mtx = NULL;
		BTIF_LOG_E("%s: btif open failed", __func__);
		return ret;
	}
#ifdef MTK_BT_DRV_CHIP_RESET
	g_rst_info.rst_mtx = xSemaphoreCreateMutex();
	g_rst_info.rst_hdl = NULL;
	if (pdPASS != xTaskCreate(btmtk_chip_reset_task, "bt_chip_rst", 1024,
							  &g_rst_info, configMAX_PRIORITIES - 4,
							  &g_rst_info.rst_hdl)) {
		BTIF_LOG_E("cannot create btmtk_chip_reset_task");
		btif_util_sema_delete(g_rst_info.rst_mtx);
		g_rst_info.rst_mtx = NULL;
	}
#endif /* #ifdef MTK_BT_DRV_CHIP_RESET */

	/* setup IRQ for BT FW assert */
	hal_nvic_register_isr_handler(CONN2AP_SW_IRQn, btmtk_coredump_handler);
	hal_nvic_irq_set_type(CONN2AP_SW_IRQn, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
	// move enable irq to download fw patch
	//hal_nvic_enable_irq(CONN2AP_SW_IRQn);

	/* callback from btif driver */
	btif_rx_cb_reg((btif_rx_cb)btmtk_data_recv);
	return ret;
}

int btmtk_open(void)
{
	int ret;

	/* The system add the feature that it will trigger assert if malloc in ISR.
	 * So, we can not create task in ISR and move it in initialize procedure.
	 */
	if (!btmtk_coredump_task &&
		pdPASS != xTaskCreate(btmtk_coredump_task_main, "btmtk_coredump_rx",
							  (1024 * 4) / sizeof(StackType_t), NULL,
							  configMAX_PRIORITIES - 2,
							  &btmtk_coredump_task)) {
		BTIF_LOG_E("%s cannot create coredump task.", __func__);
	}

	/* TODO: we need to check efuse configuration to see if EMI exist */
	ret = btmtk_send_fw_rom_patch(DL_FW_PHASE1);
	if (ret != 0) {
		BTIF_LOG_E("%s, download firmware phase1 fail(%d)", __func__, ret);
		return ret;
	}

	/* make sure bgfsys is powered on */
	if (g_bgfsys_on == 0)
		btmtk_bgfsys_power_on();

	btmtk_set_own_type(DRIVER_OWN);

	BTIF_LOG_I("Download firmware start");
	ret = btmtk_send_fw_rom_patch(DL_FW_PHASE2);
	if (ret != 0) {
		BTIF_LOG_E("%s, download firmware phase2 fail(%d)", __func__, ret);
		return ret;
	}
	BTIF_LOG_I("Download firmware finish");
	return 0;
}

int btmtk_close(void)
{
	int ret = 0;

	if (btmtk_coredump_task) {
		vTaskDelete(btmtk_coredump_task);
		btmtk_coredump_task = NULL;
	}
	if (btmtk_bgfsys_power_off()) {
		BTIF_LOG_E("%s, bgfsys power off fail", __func__);
		ret = -1;
	}

	return ret;
}

void btmtk_deinit(void)
{
	BTIF_LOG_D("%s", __func__);
#ifdef MTK_BT_DRV_CHIP_RESET
	if (g_rst_info.rst_hdl) {
		vTaskDelete(g_rst_info.rst_hdl);
		g_rst_info.rst_hdl = NULL;
	}
	if (g_rst_info.rst_mtx) {
		btif_util_sema_delete(g_rst_info.rst_mtx);
		g_rst_info.rst_mtx = NULL;
	}
#endif /* #ifdef MTK_BT_DRV_CHIP_RESET */
	btmtk_coredump_deinit_dump_regions();

	if (btmtk_coredump_task) {
		vTaskDelete(btmtk_coredump_task);
		btmtk_coredump_task = NULL;
	}
	btmtk_set_coredump_state(BT_FW_COREDUMP_UNKNOWN);

#ifdef BUFFER_DEBUG_TRACE
	btif_buffer_debug_deinit();
#endif
#ifdef BTIF_MAIN_TASK_TRACE
	btif_main_task_record_deinit();
#endif
	BTIF_close();
}

void btmtk_suspend(void)
{
	BTIF_LOG_D("%s", __func__);
	BTIF_suspend();
}

void btmtk_resume(void)
{
	BTIF_LOG_D("%s", __func__);
	BTIF_resume();
}

void btmtk_register_event_cb(void (*func)(void))
{
	BTIF_LOG_D("%s curr: %p, target: %p", __func__, receiver_notify_cb, func);
	receiver_notify_cb = func;
}

void btmtk_register_fwlog_recv_cb(btmtk_fwlog_recv_cb cb)
{
	BTIF_LOG_I("%s cb: %p", __func__, cb);
	fwlog_recv_cb = cb;
}

void btmtk_enable_bperf(bool enable)
{
	BTIF_LOG_I("%s: %s", __func__, enable == pdTRUE ? "True" : "False");
	bperf_enable = enable;
}

int btmtk_read(unsigned char *buffer, unsigned int length)
{
	unsigned int copyLen = 0;
	unsigned int tailLen = 0;

	if ((buffer == NULL) || (length == 0)) {
		BTIF_LOG_E("%s invalid arg", __func__);
		return -1;
	}

	btif_util_sema_lock(bt_rx_buffer.mtx, 0);
	BTIF_LOG_V("%s start read_p=0x%x, write_p=0x%x", __func__,
			   bt_rx_buffer.read_p, bt_rx_buffer.write_p);
	while (bt_rx_buffer.read_p != bt_rx_buffer.write_p) {
		if (bt_rx_buffer.write_p > bt_rx_buffer.read_p) {
			copyLen = bt_rx_buffer.write_p - bt_rx_buffer.read_p;
			if (copyLen > length)
				copyLen = length;
			memcpy(buffer, bt_rx_buffer.buffer + bt_rx_buffer.read_p, copyLen);
			bt_rx_buffer.read_p += copyLen;
			break;
		}
		tailLen = BTMTK_RING_BUFFER_SIZE - bt_rx_buffer.read_p;
		if (tailLen > length) {
			/* exclude equal case to skip wrap check */
			copyLen = length;
			memcpy(buffer, bt_rx_buffer.buffer + bt_rx_buffer.read_p, copyLen);
			bt_rx_buffer.read_p += copyLen;
		} else {
			/* part 1: copy tailLen */
			memcpy(buffer, bt_rx_buffer.buffer + bt_rx_buffer.read_p, tailLen);
			buffer += tailLen; /* update buffer offset */
			/* part 2: check if head length is enough */
			copyLen = length - tailLen;
			copyLen = (bt_rx_buffer.write_p < copyLen) ? bt_rx_buffer.write_p : copyLen;
			if (copyLen)
				memcpy(buffer, bt_rx_buffer.buffer + 0, copyLen);
			/* Update read_p final position */
			bt_rx_buffer.read_p = copyLen;
			/* update return length: head + tail */
			copyLen += tailLen;
		}
		break;
	}
	BTIF_LOG_V("%s end  read_p=0x%x, write_p=0x%x", __func__,
			   bt_rx_buffer.read_p, bt_rx_buffer.write_p);
	btif_util_sema_unlock(bt_rx_buffer.mtx, 0);

	return copyLen;
}

int btmtk_write(const unsigned char *buffer, const unsigned int length)
{
	int ret = BTIF_write(buffer, length);

	if (ret <= 0) {
		BTIF_LOG_E("%s: BTIF_write fail! %d", __func__, ret);
		// May happened bus fault, so dump bus register
		dump_bus_hang_reg();
	}

	return ret;
}

void btmtk_loopback_ctrl(unsigned char enable)
{
	btif_loopback_ctrl(enable);
}

void btmtk_dump_reg(void)
{
	btif_dump_reg();
}

void btmtk_buffer_debug(void)
{
#ifdef BUFFER_DEBUG_TRACE
	btif_buffer_debug();
	btif_buffer_trace_restart();
#else
	BTIF_LOG_E("%s: Debug trace did not enabled!", __func__);
#endif
}

void btmtk_dump_bgfsys_hang_reg(void)
{
	unsigned int addr = 0;
	int i = 0;
	uint32_t val[14];
	uint32_t sleep_protect_val;
	uint32_t bt_cfg_clk;

	struct bus_hang_info vff_node = {0, 0, 0, 0, 0, 0, 0};
	struct bus_hang_info ext_node = {0, 0, 0, 0, 0, 0, 0};
	struct bus_hang_info ram_node = {0, 0, 0, 0, 0, 0, 0};
	struct bus_hang_info cfg_node = {0, 0, 0, 0, 0, 0, 0};
	struct bus_hang_info cfg_apb_node = {0, 0, 0, 0, 0, 0, 0};

	/*
	 * 1. if tx/rx bit 0, bgf bus hang; if 1, wakeup issue
	 *    [31]: tx irq enable, [22]: tx irq ready, [23]: rx irq ready
	 */
	sleep_protect_val = BTIF_READ32(CONN_INFRA_CONN2BT_GALS_SLP_STATUS);
	BTIF_LOG_D("check AP2BT readable:\n"
			   " sleep_protect_CR(0x%08x) = 0x%08lx[31, 23, 22]",
			   CONN_INFRA_CONN2BT_GALS_SLP_STATUS, sleep_protect_val);
	if (BBIT(sleep_protect_val, 31) || BBIT(sleep_protect_val, 23) ||
		BBIT(sleep_protect_val, 22)) {
		BTIF_LOG_I("check BGFSYS wakeup");
		/* write 3b'010 to cr */
		BTIF_CLR_BIT(BGF_CONN_INFRA_CFG_CLK_WR, BIT(0));
		BTIF_SET_BIT(BGF_CONN_INFRA_CFG_CLK_WR, BIT(1));
		BTIF_CLR_BIT(BGF_CONN_INFRA_CFG_CLK_WR, BIT(2));
		bt_cfg_clk = BTIF_READ32(BGF_CONN_INFRA_CFG_CLK_RD);
		BTIF_LOG_D("check cfg_clk(0x%08x) = 0x%08lx[30]", BGF_CONN_INFRA_CFG_CLK_RD, bt_cfg_clk);
		if (RBBIT(bt_cfg_clk, 30))
			BTIF_LOG_E("CONN_INFRA osc control ERR, find conn_infra clk/cfg owner");
		else
			btmtk_dump_sleep_fail_reg();
		return;
	}
	BTIF_LOG_I("AP2BT readable check ok!");

	/* 2. dump bgf bus hang cr */
	BTIF_LOG_I("might be hang on bgfsys bus, dump CR:");
	for (i = 0; i < 14; i++) {
		addr = BGF_SYS_BUS_HANG_ADDR_ADD(BGF_SYS_BUS_HANG_BASE_ADDR, i * 4);
		val[i] = BTIF_READ32(addr);
		BTIF_LOG_I("BGFSYS_BUS_HANG_ADDR(0x%x) = 0x%08lx", addr + 0x1F800000, val[i]);
	}

	/*
	 * the following value is used for analyzing whether bus hang,
	 * the calculate is based on firmware's method
	 */
	vff_node.hang = (val[0] % (1 << 17)) / (1 << 16);
	ext_node.hang = (val[0] % (1 << 18)) / (1 << 17);
	ram_node.hang = (val[0] % (1 << 19)) / (1 << 18);
	cfg_node.hang = (val[0] % (1 << 20)) / (1 << 19);
	cfg_apb_node.hang = (val[0] % (1 << 21)) / (1 << 20);

	if (vff_node.hang) {
		vff_node.addr = val[2];
		vff_node.rd_wr = (val[3] % (1 << 4)) / (1 << 3); /* 1: write, 0: read */
		vff_node.master_id = (val[3] % (1 << 14)) / (1 << 6);
		vff_node.htrans = (val[3] % (1 << 6)) / (1 << 4);
		vff_node.hburst = val[3] % (1 << 3);
	}
	if (ext_node.hang) {
		ext_node.addr = val[4];
		ext_node.rd_wr = (val[5] % (1 << 4)) / (1 << 3);
		ext_node.master_id = (val[5] % (1 << 14)) / (1 << 6);
		ext_node.htrans = (val[5] % (1 << 6)) / (1 << 4);
		ext_node.hburst = val[5] % (1 << 3);
	}
	if (ram_node.hang) {
		ram_node.addr = val[6];
		ram_node.rd_wr = (val[7] % (1 << 4)) / (1 << 3);
		ram_node.master_id = (val[7] % (1 << 14)) / (1 << 6);
		ram_node.htrans = (val[7] % (1 << 6)) / (1 << 4);
		ram_node.hburst = val[7] % (1 << 3);
	}
	if (cfg_node.hang) {
		cfg_node.addr = val[8];
		cfg_node.rd_wr = (val[9] % (1 << 4)) / (1 << 3);
		cfg_node.master_id = (val[9] % (1 << 14)) / (1 << 6);
		cfg_node.htrans = (val[9] % (1 << 6)) / (1 << 4);
		cfg_node.hburst = val[9] % (1 << 3);
	}
	if (cfg_apb_node.hang) {
		cfg_apb_node.addr = val[10];
		cfg_apb_node.rd_wr = (val[11] % (1 << 4)) / (1 << 3);
		cfg_apb_node.master_id = (val[11] % (1 << 14)) / (1 << 6);
		cfg_apb_node.htrans = (val[11] % (1 << 6)) / (1 << 4);
		cfg_apb_node.hburst = val[11] % (1 << 3);
	}

	vff_node.fake_rdate = val[13];
	ext_node.fake_rdate = val[13];
	ram_node.fake_rdate = val[13];
	cfg_node.fake_rdate = val[13];
	cfg_apb_node.fake_rdate = val[13];

	BTIF_LOG_I("bgfsys bus hang parse result:");
	BTIF_LOG_I("%12s %11s %10s %5s %9s %6s %6s %10s", " ", "hang_or_not",
			   "addr", "w/r", "master_ID", "htrans", "hburst", "fake_rdata");
	if (vff_node.hang) {
		BTIF_LOG_I("%12s %11s 0x%08lx %5s %9d %6d %6d 0x%08lx",
				   "vff_node", "hang", vff_node.addr,
				   vff_node.rd_wr ? "write" : "read",
				   vff_node.master_id, vff_node.htrans, vff_node.hburst,
				   vff_node.fake_rdate);
	} else {
		BTIF_LOG_I("%12s %11s %10s %5s %9s %6s %6s 0x%08lx", "vff_node",
				   "ok", "ok", "ok", "ok", "ok", "ok",
				   vff_node.fake_rdate);
	}
	if (ext_node.hang) {
		BTIF_LOG_I("%12s %11s 0x%08lx %5s %9d %6d %6d 0x%08lx",
				   "ext_node", "hang", ext_node.addr,
				   ext_node.rd_wr ? "write" : "read",
				   ext_node.master_id, ext_node.htrans, ext_node.hburst,
				   ext_node.fake_rdate);
	} else {
		BTIF_LOG_I("%12s %11s %10s %5s %9s %6s %6s 0x%08lx", "ext_node",
				   "ok", "ok", "ok", "ok", "ok", "ok",
				   ext_node.fake_rdate);
	}
	if (ram_node.hang) {
		BTIF_LOG_I("%12s %11s 0x%08lx %5s %9d %6d %6d 0x%08lx",
				   "ram_node", "hang", ram_node.addr,
				   ram_node.rd_wr ? "write" : "read",
				   ram_node.master_id, ram_node.htrans, ram_node.hburst,
				   ram_node.fake_rdate);
	} else {
		BTIF_LOG_I("%12s %11s %10s %5s %9s %6s %6s 0x%08lx", "ram_node",
				   "ok", "ok", "ok", "ok", "ok", "ok",
				   ram_node.fake_rdate);
	}
	if (cfg_node.hang) {
		BTIF_LOG_I("%12s %11s 0x%08lx %5s %9d %6d %6d 0x%08lx",
				   "cfg_node", "hang", cfg_node.addr,
				   cfg_node.rd_wr ? "write" : "read",
				   cfg_node.master_id, cfg_node.htrans, cfg_node.hburst,
				   cfg_node.fake_rdate);
	} else {
		BTIF_LOG_I("%12s %11s %10s %5s %9s %6s %6s 0x%08lx", "cfg_node",
				   "ok", "ok", "ok", "ok", "ok", "ok",
				   cfg_node.fake_rdate);
	}
	if (cfg_apb_node.hang) {
		BTIF_LOG_I("%12s %11s 0x%08lx %5s %9d %6d %6d 0x%08lx",
				   "cfg_apb_node", "hang", cfg_apb_node.addr,
				   cfg_apb_node.rd_wr ? "write" : "read",
				   cfg_apb_node.master_id, cfg_apb_node.htrans,
				   cfg_apb_node.hburst, cfg_apb_node.fake_rdate);
	} else {
		BTIF_LOG_I("%12s %11s %10s %5s %9s %6s %6s 0x%08lx",
				   "cfg_apb_node", "ok", "ok", "ok", "ok", "ok", "ok",
				   cfg_apb_node.fake_rdate);
	}
}

void btmtk_dump_sleep_fail_reg(void)
{
	uint32_t wr_val[48];
	uint32_t rd_val[48];
	int i = 0;

	for (i = 0; i < 20; i++)
		wr_val[i] = 0x80 + i;

	for (i = 0; i < 9; i++)
		wr_val[20 + i] = 0xc0 + i;

	for (i = 0; i < 15; i++)
		wr_val[29 + i] = 0xd0 + i;

	for (i = 0; i < 4; i++)
		wr_val[44 + i] = 0xe0 + i;

	for (i = 0; i < 48; i++) {
		BTIF_WRITE32(BGF_BT_TOP_SIGNAL_WR, wr_val[i]);
		rd_val[i] = BTIF_READ32(BGF_BT_TOP_SIGNAL_RD);
	}

	BTIF_LOG_I("sleep reg: w_addr: %08x, r_addr: %08x",
				BGF_BT_TOP_SIGNAL_WR - 0x48000000, BGF_BT_TOP_SIGNAL_RD - 0x48000000);
	BTIF_LOG_I("dump w_ral and r_val:");

	// 0 <= i < 20 for 0x80 - 0x93, 20 <= i < 24 for 0xc0 - 0xc3
	for (i = 0; i < 24; i += 4)
		BTIF_LOG_I("%08lx %08lx   %08lx %08lx   %08lx %08lx   %08lx %08lx",
					wr_val[i], rd_val[i], wr_val[i + 1], rd_val[i + 1],
					wr_val[i + 2], rd_val[i + 2], wr_val[i + 3], rd_val[i + 3]);

	// 24, 25 for 0xc4 - 0xc5, 44 <= i < 47 for 0xe0 - 0xe3
	BTIF_LOG_I("%08lx %08lx   %08lx %08lx   %08lx %08lx   %08lx %08lx",
				wr_val[24], rd_val[24], wr_val[25], rd_val[25],
				wr_val[44], rd_val[44], wr_val[45], rd_val[45]);
	BTIF_LOG_I("%08lx %08lx", wr_val[46], rd_val[46]);

	// use UNUSED macro for debug warnig
	UNUSED(wr_val);
	UNUSED(rd_val);
}

void btmtk_set_dbg_level(unsigned char level, int dump_buffer, int times)
{
	btif_set_log_lvl(level, dump_buffer, times);
}

void btmtk_flush_rx_queue(void)
{
	BTIF_LOG_D("%s", __func__);
	btif_util_sema_lock(bt_rx_buffer.mtx, 0);
	bt_rx_buffer.read_p = 0;
	bt_rx_buffer.write_p = 0;
	// memset(bt_rx_buffer.buffer, 0, BTMTK_RING_BUFFER_SIZE); //Remove to reduce CPU time
	btif_util_sema_unlock(bt_rx_buffer.mtx, 0);
}

int bt_uart_mapping_baudrate(int speed)
{
	int set_baudrate = 0;

	switch (speed) {
	case 110:
		set_baudrate = HAL_UART_BAUDRATE_110;
		break;
	case 300:
		set_baudrate = HAL_UART_BAUDRATE_300;
		break;
	case 1200:
		set_baudrate = HAL_UART_BAUDRATE_1200;
		break;
	case 2400:
		set_baudrate = HAL_UART_BAUDRATE_2400;
		break;
	case 4800:
		set_baudrate = HAL_UART_BAUDRATE_4800;
		break;
	case 9600:
		set_baudrate = HAL_UART_BAUDRATE_9600;
		break;
	case 38400:
		set_baudrate = HAL_UART_BAUDRATE_38400;
		break;
	case 57600:
		set_baudrate = HAL_UART_BAUDRATE_57600;
		break;
	case 115200:
		set_baudrate = HAL_UART_BAUDRATE_115200;
		break;
	case 230400:
		set_baudrate = HAL_UART_BAUDRATE_230400;
		break;
	case 460800:
		set_baudrate = HAL_UART_BAUDRATE_460800;
		break;
	case 921600:
		set_baudrate = HAL_UART_BAUDRATE_921600;
		break;
#ifdef HAL_UART_FEATURE_3M_BAUDRATE
	case 3000000:
		set_baudrate = HAL_UART_BAUDRATE_3000000;
		break;
#endif
	default:
		BTIF_LOG_E("%s speed %d is not mapping", __func__, speed);
		break;
	}

	return set_baudrate;
}

hal_uart_status_t bt_uart_init(int port, int speed)
{
	int set_baudrate = 0;
	hal_uart_status_t status_t;
	hal_uart_config_t basic_config;
	// hal_uart_dma_config_t dma_config;

	if (port == HAL_UART_0) {
		BTIF_LOG_D("port 0 is used for systemlog, already inited");
		return HAL_UART_STATUS_OK;
	}
	if (g_inited_uart_port == port) {
		BTIF_LOG_D("%s, port %d is inited", __func__, port);
		return HAL_UART_STATUS_OK;
	} else if (g_inited_uart_port > 0) {
		BTIF_LOG_E("%s, only support one port at dma mode, cur: %d",
				   __func__, g_inited_uart_port);
		return HAL_UART_STATUS_ERROR;
	}

	/* Configure UART port with basic function */
	set_baudrate = bt_uart_mapping_baudrate(speed);
	basic_config.baudrate = set_baudrate;
	basic_config.parity = HAL_UART_PARITY_NONE;
	basic_config.stop_bit = HAL_UART_STOP_BIT_1;
	basic_config.word_length = HAL_UART_WORD_LENGTH_8;
	status_t = hal_uart_init(port, &basic_config);

	if (status_t) {
		BTIF_LOG_E("%s hal_uart_init error %d", __func__, status_t);
		return status_t;
	}

#ifdef OPEN_UART_DMA_MODE
	hal_uart_dma_config_t dma_config;
	// Workaround for PSRAM 37.5M cause uart data drop
	// g_vff_mem_addr = (unsigned int)pvPortMallocNC(VFIFO_SIZE * 2); // 8 bytes align
	g_vff_mem_addr = (unsigned int)SYS_MALLOC_NC(VFIFO_SIZE * 2);

	if (g_vff_mem_addr == 0) {
		BTIF_LOG_E("%s malloc(%dbytes) error, return addr = 0",
				   __func__, VFIFO_SIZE * 2);
		return HAL_UART_STATUS_ERROR;
	}

	/*Step2: Configure UART port to dma mode. */
	// dma_config.receive_vfifo_alert_size = RECEIVE_ALERT_SIZE;
	dma_config.receive_vfifo_buffer = (uint32_t)g_vff_mem_addr;
	dma_config.receive_vfifo_buffer_size = VFIFO_SIZE;
	dma_config.receive_vfifo_threshold_size = RECEIVE_THRESHOLD_SIZE;
	dma_config.send_vfifo_buffer = (uint32_t)g_vff_mem_addr + VFIFO_SIZE;
	dma_config.send_vfifo_buffer_size = VFIFO_SIZE;
	dma_config.send_vfifo_threshold_size = SEND_THRESHOLD_SIZE;

	status_t = hal_uart_set_dma(port, &dma_config);
	if (status_t) {
		BTIF_LOG_E("%s hal_uart_set_dma error %d", __func__, status_t);
		// vPortFreeNC((unsigned int *)g_vff_mem_addr);
		SYS_FREE_NC((unsigned int *)g_vff_mem_addr);
		g_vff_mem_addr = 0;
		return status_t;
	}
	BTIF_LOG_I("%s: use dma mode (port = %d) VFIFO_SIZE = %d", __func__,
			   port, VFIFO_SIZE);
	BTIF_LOG_I("%s: dma rx_th = %d, tx_th =%d", __func__,
			   RECEIVE_THRESHOLD_SIZE, SEND_THRESHOLD_SIZE);
#endif //(end -- OPEN_UART_DMA_MODE)

	g_inited_uart_port = port;
	BTIF_LOG_D("%s success", __func__);
	return HAL_UART_STATUS_OK;
}

hal_uart_status_t bt_uart_deinit(int fd)
{
#ifdef CHIP_MT7933
	hal_uart_status_t status_t = HAL_UART_STATUS_OK;

	if (g_inited_uart_port < 0)
		return status_t;

	if (fd == HAL_UART_0)
		return status_t;

	status_t = hal_uart_deinit(fd); //(HAL_UART_2);

#ifdef OPEN_UART_DMA_MODE
	if (g_vff_mem_addr) {
		// vPortFreeNC((unsigned int *)g_vff_mem_addr);
		SYS_FREE_NC((unsigned int *)g_vff_mem_addr);
		g_vff_mem_addr = 0;
	}
#endif

	g_inited_uart_port = -1;
	return status_t;
#else
	return 0;
#endif
}

uint32_t bt_uart_read(int fd, unsigned char *buf, int len)
{
	int length;

	BTIF_LOG_D("%s: fd = 0x%x", __func__, fd);

	length = hal_uart_receive_dma(fd, (uint8_t *)buf, len);
	if (len < length)
		BTIF_LOG_E("%s input buffer len(%d) is not enough, need %ld",
				   __func__, len, length);
	return length;
}

uint32_t bt_uart_write(int fd, unsigned char *buf, int len)
{
	uint32_t ret = 0;
	int write_len = 0;
	uint32_t retry = UART_WRITE_RETRY_CNT;

	// btif_util_dump_buffer("PICUS", buf, len, 0);

	do {
		if (write_len)
			vTaskDelay(pdMS_TO_TICKS(UART_WRITE_RETRY_DELAY));
		ret = hal_uart_send_dma(fd, (const uint8_t *)buf + write_len, len - write_len);
		write_len += ret;
	} while (write_len < len && retry-- > 0);

	if (write_len == len) {
		BTIF_LOG_V("%s: send success, write_len = %d retry = %d",
				   __func__, write_len, retry);
	} else {
		BTIF_LOG_E("%s: send failure, len:%d write_len = %d retry = %d",
				   __func__, len, write_len, retry);
	}
	return write_len;
}

int btsnoop_log_write_to_uart(unsigned char *buf, int len)
{
	btif_util_dump_buffer("BTSNOOP", buf, len, 0);
	log_write_binary((char *)buf, len);
	return len;
}

void btmtk_set_ice_fwdl(char *param)
{
	if (memcmp(param, "cm33", sizeof("cm33")) == 0) {
		/**
		 * Firmware download by CM33 ICE after psram cal done
		 * 1. D.LOAD.BINARY .\bt_emi_7933.bin 0x10000030 /long /noclear
		 * 2. D.LOAD.BINARY .\BT_RAM_CODE_MT7933_1_1_hdr.bin 0x10057800 /long /noclear
		 */
		psram_copy = false;
		hif_dl = true;
		bt_fw_start_addr = BT_FIRMWARE_IN_PSRAM_START;
	} else if (memcmp(param, "n10", sizeof("n10")) == 0) {
		/*
		 * Firmware download by N10 ICE after psram cal done
		 * 1. D.LOAD.BINARY .\bt_emi_7933.bin 0xF8000030 /long /noclear
		 * 2. D.LOAD.BINARY .\test_bt.bin 0x00800000 /long /noclear
		 * 3. D.LOAD.BINARY .\test_bt_patch.bin 0x00900000 /long /noclear
		 * 4. D.LOAD.BINARY .\bt_ilm_7933.bin 0x00904000 /long /noclear
		 * 5. D.LOAD.BINARY .\bt_ilm_7933.bin.rwdata 0x0200AC00 /long /noclear
		 */
		psram_copy = false;
		hif_dl = false;
	}
	BTIF_LOG_I("%s: psram_copy = %d, hif_dl = %d", __func__, psram_copy, hif_dl);
}

extern bool btif_dma_check_RX_empty(void);
bool btmtk_check_HW_TRX_empty(void)
{
	/*
	 * Check BTIF Tx Empty :
	 * BTIF_BASE + 0x14 [6] == 1
	 * Check BTIF Rx Empty :
	 * BTIF_BASE + 0x14 [0] == 0
	 */
	unsigned int base = (unsigned int)BTIF_BASE;
	unsigned int lsr = 0x0;

	lsr = BTIF_READ32(BTIF_LSR(base));

	if (lsr & BTIF_LSR_DR_BIT) {
		BTIF_LOG_W("BTIF Rx data not empty");
		return pdFALSE;
	}

	if (!(lsr & BTIF_LSR_TEMT_BIT)) {
		BTIF_LOG_W("BTIF Tx data not empty");
		return pdFALSE;
	}

	if (!btif_dma_check_RX_empty()) {
		BTIF_LOG_W("APDMA RX data not empty");
		return pdFALSE;
	}

	return pdTRUE;
}

void btmtk_set_own_ctrl(uint8_t ctrl)
{
	g_own_control = ctrl;
}

uint8_t btmtk_get_own_ctrl(void)
{
	return g_own_control;
}

uint8_t btmtk_get_local_own_type(void)
{
	return g_own_type;
}

void btmtk_set_local_own_type(unsigned char own_type)
{
	g_own_type = own_type;
}

uint8_t btmtk_get_own_type(void)
{
	uint32_t val = 0;

	val = BTIF_READ32(BGF_REG_LPCTL);
	if (val & BGF_OWNER_STATE_SYNC_B)
		return FW_OWN;
	else
		return DRIVER_OWN;
}

int btmtk_set_own_type(unsigned char own_type)
{
	uint32_t val = 0;
	uint32_t retry = LPCR_POLLING_RETRY;
	// int count = 0;

	if (!btmtk_get_own_ctrl()) {
		BTIF_LOG_E("ownship control diabled!");
		return -1;
	}

	if (own_type != FW_OWN && own_type != DRIVER_OWN) {
		BTIF_LOG_E("not valid own type!");
		return -1;
	}

	val = BTIF_READ32(BGF_REG_LPCTL);
	if ((own_type == FW_OWN) && (val & BGF_OWNER_STATE_SYNC_B)) {
		// BTIF_LOG_V("already set fw own!");
		return 0;
	}
	if ((own_type == DRIVER_OWN) && (!(val & BGF_OWNER_STATE_SYNC_B))) {
		// BTIF_LOG_V("already set driver own!");
		return 0;
	}

	do {
		if (own_type == FW_OWN) {
			if ((retry & 0xF) == 0) {
				BTIF_WRITE32(BGF_REG_LPCTL, BGF_HOST_SET_FW_OWN_B);
				// BTIF_LOG_V("write FW_OWN, cr_val = 0x%lx, count = %d", val, count++);
			}
			vTaskDelay(pdMS_TO_TICKS(1));
			val = BTIF_READ32(BGF_REG_LPCTL);
			if (val & BGF_OWNER_STATE_SYNC_B) {
				// BTIF_LOG_V("set FW_OWN success, cr_val = 0x%lx, end!", val);
				break;
			} else if (!btmtk_check_HW_TRX_empty()) {
				BTIF_LOG_E("set FW_OWN fail due to remain rx data");
				retry = 0; // set FW OWN fail, so we need to enable wakeup IRQ.
				break;
			}
		} else if (own_type == DRIVER_OWN) {
			if ((retry & 0xF) == 0) {
				BTIF_WRITE32(BGF_REG_LPCTL, BGF_HOST_SET_DRV_OWN_B);
				// BTIF_LOG_V("write DRIVER_OWN, cr_val = 0x%lx, count = %d", val, count++);
			}
			vTaskDelay(pdMS_TO_TICKS(1));
			val = BTIF_READ32(BGF_REG_LPCTL);
			if (!(val & BGF_OWNER_STATE_SYNC_B)) {
				// BTIF_LOG_V("set DRIVER_OWN success, cr_val = 0x%lx, end!", val);
				break;
			}
		}
		retry--;
	} while (retry > 0);

	if (retry > 0) {
		BTIF_LOG_V("set %s Success!", own_type == FW_OWN ? "FW_OWN" : "DRIVER_OWN");
		g_own_type = own_type;
	} else {
		BTIF_LOG_E("set %s fail!", own_type == FW_OWN ? "FW_OWN" : "DRIVER_OWN");
		// Since we may set FW OWN fail when FW is going to send data to driver
		// We should not trigger coredump for this case.
		if (own_type == FW_OWN)
			btif_irq_ctrl(BTIF_WAKEUP_IRQ_ID, TRUE);
		return -1;
	}

	// We need to enable Wakeup IRQ after set FW OWN success.
	if (own_type == FW_OWN)
		btif_irq_ctrl(BTIF_WAKEUP_IRQ_ID, TRUE);
	/*
	 * We need to clear Wakeup IRQ after set Driver OWN success
	 * or we will get one more Wakeup IRQ after we set FW OWN again.
	 */
	else // Driver OWN
		hal_nvic_clear_pending_irq(BTIF_WAKEUP_IRQ_ID);

	return 0;
}

void btmtk_dump_driver_own_fail_regs(void)
{
	uint8_t i = 0;
	/*
	 * 1. read mcu current pc
	 * 2. read Driver own interrupt status
	 * 3. read Driver own status
	 * 4. read BGF_SYSSTRAP_RD_DBG
	 * 5. read BGF_MET_CFG_ON_EVENT_DATA
	 * 6. set driver own
	 * 7. do 1 ~ 6 steps 15 times
	 */
	for (i = 0; i < DRV_OWN_FAIL_LOOP_CNT; i++) {
		BTIF_LOG_E("drv_own fail, pc: %08x, irq: %08x, ownst: %08x, systrap: %08x, metevt: %08x",
					BTIF_READ32(BGF_MCU_CUR_PC), BTIF_READ32(BGF_DRV_OWN_INTERRUPT_ST),
					BTIF_READ32(BGF_REG_LPCTL), BTIF_READ32(BGF_SYSSTRAP_RD_DBG),
					BTIF_READ32(BGF_MET_CFG_ON_EVENT_DATA));
		BTIF_WRITE32(BGF_REG_LPCTL, BGF_HOST_SET_DRV_OWN_B);
	}

	// 8. read dma write pointer
	// 9. read dma read pointer
	// 10. read dma fifo count
	// 11. read dma fifo state
	BTIF_LOG_E("drv_own fail, dma info, wr: %08x, rd: %08x, ffcnt: %08x, ffsta: %08x",
				BTIF_READ32(BTIF_DMA_WRPTR), BTIF_READ32(BTIF_DMA_RDPTR),
				BTIF_READ32(BTIF_DMA_FFCNT), BTIF_READ32(BTIF_DMA_FFSTA));
}

#ifdef MTK_BT_DRV_CHIP_RESET
int btmtk_whole_chip_reset(void)
{
	BTIF_LOG_I("%s, We cannot support whole chip reset!!", __func__);
	return -1;
#ifdef RESERVED_FOR_FUTURE
	int ret = 0;

	BTIF_LOG_I("%s, start", __func__);
	g_rst_info.rst_state = WHOLE_CHIP_RESET_START;

	/*
	 * do not call driver_power_off, if fail, it will return directly,
	 * so we can not power off bt. we should call bgfsys_power_off
	 */
	ret = bt_driver_power_off();
	if (ret)
		goto error;

	connsys_power_off();
	connsys_power_on();

	ret = bt_driver_power_on();
	if (ret)
		goto error;

	g_rst_info.rst_state = WHOLE_CHIP_RESET_END;
	BTIF_LOG_I("%s, finish", __func__);
	return 0;
error:
	BTIF_LOG_E("%s, whole chip reset fail: ret = %d", __func__, ret);
	return ret;
#endif
}

int btmtk_subsys_chip_reset(void)
{
	BTIF_LOG_I("%s, We cannot support sub chip reset!!", __func__);
	return -1;
#ifdef RESERVED_FOR_FUTURE
	int ret = 0;

	BTIF_LOG_I("%s, start", __func__);
	g_rst_info.rst_state = SUBSYS_CHIP_RESET_START;

	/*
	 * there not be check func_ctrl error, because the state may be chaos,
	 * so send hci cmd may error, but will not affect power off
	 */
	ret = bt_driver_power_off();
	if (ret)
		goto error;

	ret = bt_driver_power_on();
	if (ret)
		goto error;

	g_rst_info.rst_state = SUBSYS_CHIP_RESET_END;
	BTIF_LOG_I("%s, finish", __func__);
	return 0;
error:
	BTIF_LOG_E("%s, subsys reset error: ret = %d", __func__, ret);
	return ret;
#endif
}

void btmtk_trigger_chip_reset(void)
{
	int ret = 0;

	BTIF_LOG_I("%s: rest_type = %d", __func__, g_rst_info.rst_type);
	btif_irq_ctrl(CONN2AP_SW_IRQn, FALSE);
	if (!g_rst_info.rst_type) {
		ret = btmtk_subsys_chip_reset();
		if (ret)
			ret = btmtk_whole_chip_reset();
	} else {
		ret = btmtk_whole_chip_reset();
	}

	if (!ret)
		BTIF_LOG_I("%s, chip reset success!", __func__);
	else
		BTIF_LOG_E("%s, chip reset fail, ret = %d", __func__, ret);

	g_rst_info.rst_type = SUBSYS_CHIP_RESET;
	g_rst_info.rst_state = CHIP_RESET_UNKNOWN;
	btmtk_set_coredump_state(BT_FW_COREDUMP_UNKNOWN);
	btif_irq_ctrl(CONN2AP_SW_IRQn, TRUE);
}

void btmtk_chip_reset_task(void *p_data)
{
	struct chip_rst_info *p_info = (struct chip_rst_info *)p_data;

	BTIF_LOG_D("%s running!", __func__);

	while (1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (xSemaphoreTake(p_info->rst_mtx, portMAX_DELAY) == pdTRUE) {
			btmtk_trigger_chip_reset();
			xSemaphoreGive(p_info->rst_mtx);
		}
	}
}

void btmtk_reset_notify(unsigned char from_isr)
{
	BTIF_LOG_D("%s, reset on: %d", __func__, bt_drv_info.cfg.support_dongle_reset);
	if (bt_drv_info.cfg.support_dongle_reset) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		if (from_isr)
			vTaskNotifyGiveFromISR(g_rst_info.rst_hdl, &xHigherPriorityTaskWoken);
		else
			xTaskNotifyGive(g_rst_info.rst_hdl);
	}
}

int btmtk_get_chip_reset_state(void)
{
	return g_rst_info.rst_state;
}
#endif /* #ifdef MTK_BT_DRV_CHIP_RESET */

void btmtk_fw_assert_cb_register(btmtk_event_cb assert_nty)
{
	BTIF_LOG_D("%s called, assert_nty is %s", __func__,
			   assert_nty == NULL ? "NULL" : "not NULL");
	fw_assert_nty = assert_nty;
}

int btmtk_get_bgfsys_power_state(void)
{
	return g_bgfsys_on;
}

#ifdef MTK_BT_DRV_AUTO_PICUS
void btmtk_enable_picus_log(void)
{
#ifdef MTK_NVDM_ENABLE
#ifdef MTK_BT_PICUS_ENABLE
	char *cmd[3];
	char temp[] = "0";
	int cmd_len = sizeof(cmd) / sizeof(char *);

	cmd[0] = "picus";

	if (!bt_drv_info.cfg.support_auto_picus) {
		BTIF_LOG_W("do not auto enable picus");
		return;
	}

	if (bt_drv_info.cfg.picus_log_level == FWLOG_LVL_LOW_POWER ||
		bt_drv_info.cfg.picus_log_level == FWLOG_LVL_FULL) {
		if (bt_drv_info.cfg.picus_log_level == FWLOG_LVL_LOW_POWER)
			cmd[1] = "-o";
		else
			cmd[1] = "-f";
		picus_cmd_handler(2, cmd);
	}

	if (bt_drv_info.cfg.picus_log_via != 0xff) {
		cmd[1] = "-v";
		temp[0] = bt_drv_info.cfg.picus_log_via + '0';
		cmd[2] = temp;
		picus_cmd_handler(cmd_len, cmd);
	}

	if (bt_drv_info.cfg.picus_uart_baudrate) {
		cmd[1] = "-b";
		if (bt_drv_info.cfg.picus_uart_baudrate == FWLOG_UART_BAUDRATE_115200) {
			cmd[2] = "115200";
		} else if (bt_drv_info.cfg.picus_uart_baudrate == FWLOG_UART_BAUDRATE_921600) {
			cmd[2] = "921600";
		} else if (bt_drv_info.cfg.picus_uart_baudrate == FWLOG_UART_BAUDRATE_3000000) {
			cmd[2] = "3000000";
		} else {
			BTIF_LOG_W("%s: Invalid Baurate setting 0x%02x", bt_drv_info.cfg.picus_uart_baudrate);
			cmd_len = 2;
		}
		if (cmd_len == 3)
			picus_cmd_handler(cmd_len, cmd);
	}

	cmd[1] = "-x";
	cmd[2] = "1";
	cmd_len = sizeof(cmd) / sizeof(char *);
	picus_cmd_handler(cmd_len, cmd);
#endif
#endif
}
#endif  /* #ifdef MTK_BT_DRV_AUTO_PICUS */
