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
#include "task.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bt_driver.h"
#include "bt_driver_cli.h"
#ifdef CHIP_MT7933
#include "btif_main.h"
#include "btif_mt7933.h"
#include "bt_driver_btsnoop.h"
#include "mt7933_connsys_dbg.h"
#else
#include "wmt.h"
#include "wmt_core.h"
#endif

#define BT_DRV_CLI_LOGV BTIF_LOG_V
#define BT_DRV_CLI_LOGD BTIF_LOG_D
#define BT_DRV_CLI_LOGI BTIF_LOG_I
#define BT_DRV_CLI_LOGW BTIF_LOG_W
#define BT_DRV_CLI_LOGE BTIF_LOG_E

int b_inq_cli_running;
#ifndef MTK_BT_DRV_CLI_LITE
static TaskHandle_t _cli_running_task_hdl;
#endif
#define C2N(x) (x <= '9' ? x - '0' : (x > 'F' ? x - 'a' + 10 : x - 'A' + 10))

#ifdef CHIP_MT7933
#ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET
extern void btif_task_change_timeout_setting(uint8_t timeout);
extern uint8_t btif_task_get_timeout_setting(void);
#endif /* #ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET */

static int _Str2u8HexNumArray(char *str, unsigned char numbs[])
{
	int i, dig;
	int str_len = strlen(str);

	for (i = 0; i < (str_len / 2); i++) {
		str[2 * i] = toupper(str[2 * i]);
		dig = C2N(str[2 * i]);
		numbs[i] = dig << 4;
		str[2 * i + 1] = toupper(str[2 * i + 1]);
		dig = C2N(str[2 * i + 1]);
		numbs[i] |= dig;
	}

	return i;
}

static unsigned int _strtoui(const char *str)
{
	unsigned int result = 0;

	if (str) {
		int i = 0;

		while ((str[i] >= '0') && (str[i] <= '9')) {
			result = result * 10 + (str[i] - '0');
			i++;
		}
	}
	return result;
}

#ifndef MTK_BT_DRV_CLI_LITE
static void bt_driver_cli_data_ready(void) // rx data ready callback
{
	BT_DRV_CLI_LOGI("%s", __func__);
	if (_cli_running_task_hdl)
		xTaskNotifyGive(_cli_running_task_hdl);
}

static void bt_driver_cli_parse_inq_resp(void)
{
#ifdef BT_DRV_CLI_SUPPORT_INQUIRY_TEST
	unsigned char ucRxBuf[512];
	unsigned char ucHeader = 0, retry = 0;
	unsigned int u4Len = 0, pkt_len = 0;
	unsigned char btaddr[6];
	char str[512];
	char *p_str;
	int cnt = 0;

	while (b_inq_cli_running) {
		if (bt_driver_rx_timeout(&ucHeader, sizeof(ucHeader)) < 0) {
			BT_DRV_CLI_LOGW("Zero byte read, retry = %d", retry);
			if (++retry == 5)
				goto CleanUp;
			continue;
		}

		memset(ucRxBuf, 0, sizeof(ucRxBuf));
		ucRxBuf[0] = ucHeader;
		u4Len = 1;

		switch (ucHeader) {
		case 0x04:
			BT_DRV_CLI_LOGI("Receive HCI event");
			if (bt_driver_rx_timeout(&ucRxBuf[1], 2) < 0) {
				BT_DRV_CLI_LOGE("Read event header fails");
				goto CleanUp;
			}

			u4Len += 2;
			pkt_len = (unsigned int)ucRxBuf[2];
			if ((u4Len + pkt_len) > sizeof(ucRxBuf)) {
				BT_DRV_CLI_LOGE("Read buffer overflow! packet len %d", u4Len + pkt_len);
				goto CleanUp;
			}

			if (bt_driver_rx_timeout(&ucRxBuf[3], pkt_len) < 0) {
				BT_DRV_CLI_LOGE("Read event param fails");
				goto CleanUp;
			}

			u4Len += pkt_len;

			/* Dump rx packet */
			/* BT_DRV_CLI_LOGI("read:\n");
			 * for (i = 0; i < u4Len; i++) {
			 *	BT_DRV_CLI_LOGI("%02x\n", ucRxBuf[i]);
			 * }
			 */
			btif_util_dump_buffer("inq result", ucRxBuf, u4Len, 0);

			if (ucRxBuf[1] == 0x0F) {
				/* Command status event */
				if (pkt_len != 0x04) {
					BT_DRV_CLI_LOGE("Unexpected command status event len %d", pkt_len);
					goto CleanUp;
				}

				if (ucRxBuf[3] != 0x00) {
					BT_DRV_CLI_LOGE("Unexpected command status %02x", ucRxBuf[3]);
					goto CleanUp;
				}
			} else if (ucRxBuf[1] == 0x01) {
				/* Inquiry complete event */
				if (pkt_len != 0x01) {
					BT_DRV_CLI_LOGE("Unexpected inquiry complete event len %d", pkt_len);
					goto CleanUp;
				}

				if (ucRxBuf[3] != 0x00) {
					BT_DRV_CLI_LOGE("Unexpected inquiry complete status %02x", ucRxBuf[3]);
					goto CleanUp;
				}

				BT_DRV_CLI_LOGW("---Inquiry completed---");
				b_inq_cli_running = 0;
			} else if (ucRxBuf[1] == 0x02 || ucRxBuf[1] == 0x22 || ucRxBuf[1] == 0x2F) {
				/* Inquiry result event */
				/*
				 * if (pkt_len != 0x0F) {
				 *	ERR("Unexpected inquiry result event len %d", pkt_len);
				 *	goto CleanUp;
				 * }
				 */
				unsigned int i = 18;
				unsigned char ucn = 0;
				/* Retrieve BD addr */
				btaddr[0] = ucRxBuf[9];
				btaddr[1] = ucRxBuf[8];
				btaddr[2] = ucRxBuf[7];
				btaddr[3] = ucRxBuf[6];
				btaddr[4] = ucRxBuf[5];
				btaddr[5] = ucRxBuf[4];

				/* Inquiry result callback */
				memset(str, 0, sizeof(str));
				p_str = str;
				p_str += sprintf(p_str, "    %02x:%02x:%02x:%02x:%02x:%02x",
								 btaddr[0], btaddr[1], btaddr[2], btaddr[3], btaddr[4], btaddr[5]);

				if (ucRxBuf[1] == 0x22 || ucRxBuf[1] == 0x2F) {
					char rssi = ~ucRxBuf[17] + 1;

					cnt = snprintf(p_str, 12, ", RSSI:%s%d", ucRxBuf[17] > 0x7F ? "-" : "",
								   ucRxBuf[17] > 0x7F ? rssi : ucRxBuf[17]);
					if (cnt > 0 && cnt <= 12)
						p_str += cnt;
					else
						BT_DRV_CLI_LOGE("%s, L: %d, snprintf error", __func__, __LINE__);
				}
				while (ucRxBuf[1] == 0x2F && i < 509 && ucRxBuf[i] > 1 && ucRxBuf[i] < 128) {
					if (ucRxBuf[i + 1] == 8 || ucRxBuf[i + 1] == 9) {
						char name[128] = {0};

						memcpy(name, &ucRxBuf[i + 2], ucRxBuf[i] - 1);
						ucn = ucRxBuf[i] - 1;
						name[ucn] = '\0';
						cnt = snprintf(p_str, strlen(name), ", Name:%s", name);
					}
					if (cnt > 6 && cnt < 135)
						p_str += cnt;
					cnt = 0;
					i += (ucRxBuf[i] + 1);
				}
				cnt = sprintf(p_str, "\n");
				if (cnt > 0)
					p_str += cnt;
				else
					BT_DRV_CLI_LOGE("%s, L: %d, sprintf error", __func__, __LINE__);
				BT_DRV_CLI_LOGW("Device: %s", str);
			} else {
				/* simply ignore it? */
				BT_DRV_CLI_LOGE("Unexpected event %02x", ucRxBuf[1]);
			}
			break;

		default:
			BT_DRV_CLI_LOGE("Unexpected BT packet header %02x", ucHeader);
			goto CleanUp;
		}
	}

CleanUp:
	b_inq_cli_running = 0;
#endif
}
#endif /* #ifndef MTK_BT_DRV_CLI_LITE */

unsigned char bt_driver_cli_set_dbg_lvl(unsigned char len, char *param[])
{
	unsigned char dbg_lvl = 0;
	int dump_buffer = 0;
	int print_times = 1;

	if (len < 1) {
		BT_DRV_CLI_LOGE("%s Usage: btdrv sdl <level 0~4> [dump buf 0/1]", __func__);
		return 0;
	}
	BT_DRV_CLI_LOGI("%s level: %s", __func__, param[0]);
	dbg_lvl = _strtoui(param[0]);
	if (len >= 2) {
		BT_DRV_CLI_LOGI("%s dump_buffer: %s", __func__, param[1]);
		dump_buffer = _strtoui(param[1]);
	}
	if (len >= 3) {
		BT_DRV_CLI_LOGI("%s dump data, print_times: %s(times * 16bytes)", __func__, param[2]);
		print_times = _strtoui(param[2]);
	}
	btmtk_set_dbg_level(dbg_lvl, dump_buffer, print_times);
	return 0;
}
unsigned char bt_driver_cli_deinit(unsigned char len, char *param[])
{
	bt_driver_power_off();
	return 0;
}

#ifndef MTK_BT_DRV_CLI_LITE
unsigned char bt_driver_cli_dump_reg(unsigned char len, char *param[])
{
	btmtk_dump_reg();
	return 0;
}
unsigned char bt_driver_cli_loopback_set(unsigned char len, char *param[])
{
	unsigned char en = pdFALSE;

	if (len < 1) {
		BT_DRV_CLI_LOGE("%s Usage: btdrv lb_set <0|1>", __func__);
		return 0;
	}
	BT_DRV_CLI_LOGI("%s param: %s", __func__, param[0]);

	if (strcmp(param[0], "0") == 0) {
		BT_DRV_CLI_LOGI("%s, disable", __func__);
		en = pdFALSE;
	} else {
		BT_DRV_CLI_LOGI("%s, enable", __func__);
		en = pdTRUE;
	}
	btmtk_loopback_ctrl(en);
	return 0;
}
#endif /* #ifndef MTK_BT_DRV_CLI_LITE */

unsigned char bt_driver_cli_send(unsigned char len, char *param[])
{
	unsigned int data_len = 0;
	unsigned char data[128] = {0};

	if (len < 1) {
		BT_DRV_CLI_LOGE("%s Usage: btdrv tx <00112233445667788...>", __func__);
		return 0;
	}
	BT_DRV_CLI_LOGI("%s: %s\n", __func__, param[0]);
	data_len = _Str2u8HexNumArray(param[0], data);
	if (data_len > 256) {
		BT_DRV_CLI_LOGE("%s send data must be less than 128 bytes", __func__);
		return 0;
	}

	BT_DRV_CLI_LOGI("start to send %d byte\n", data_len);
	bt_driver_tx(data, data_len);
	return 0;
}

#ifndef MTK_BT_DRV_CLI_LITE
unsigned char bt_driver_cli_inquiry(unsigned char len, char *param[])
{
	unsigned char INQ_MODE[] = {0x01, 0x45, 0x0C, 0x01, 0x02};
	unsigned char ucAckEvent[7];
	/* Event expected */
	unsigned char ucEvent[] = {0x04, 0x0E, 0x04, 0x01, 0x45, 0x0C, 0x00};
	unsigned char HCI_INQUIRY[] = {0x01, 0x01, 0x04, 0x05, 0x33, 0x8B, 0x9E, 0x05, 0x0A};

	BT_DRV_CLI_LOGI("%s, start", __func__); // argv[0] is the cmd name

	if (b_inq_cli_running) {
		BT_DRV_CLI_LOGE("Inquiry CLI is running, please try it later");
		return 0;
	}

	b_inq_cli_running = 1;
	_cli_running_task_hdl = xTaskGetCurrentTaskHandle();
	bt_driver_register_event_cb(bt_driver_cli_data_ready, 0);

	if (bt_driver_tx(INQ_MODE, sizeof(INQ_MODE)) < 0) {
		BT_DRV_CLI_LOGE("Send inquiry mode command fail");
		goto inq_fail;
	}

	BT_DRV_CLI_LOGI("write inquiry mode: %02x %02x %02x %02x %02x",
					(unsigned int)INQ_MODE[0], (unsigned int)INQ_MODE[1],
					(unsigned int)INQ_MODE[2], (unsigned int)INQ_MODE[3],
					(unsigned int)INQ_MODE[4]);

	/* Receive command complete event */
	if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
		BT_DRV_CLI_LOGE("Receive command complete event fail");
		goto inq_fail;
	}
	BT_DRV_CLI_LOGI("cli_inquiry read: %02x %02x %02x %02x %02x %02x %02x",
					(unsigned int)ucAckEvent[0], (unsigned int)ucAckEvent[1],
					(unsigned int)ucAckEvent[2], (unsigned int)ucAckEvent[3],
					(unsigned int)ucAckEvent[4], (unsigned int)ucAckEvent[5],
					(unsigned int)ucAckEvent[6]);
	if (memcmp(ucAckEvent, ucEvent, sizeof(ucEvent))) {
		BT_DRV_CLI_LOGE("Receive unexpected event");
		goto inq_fail;
	}

	if (bt_driver_tx(HCI_INQUIRY, sizeof(HCI_INQUIRY)) < 0) {
		BT_DRV_CLI_LOGE("Send inquiry command fail");
		goto inq_fail;
	}
	BT_DRV_CLI_LOGI("write inq: %02x %02x %02x %02x %02x %02x %02x %02x %02x",
					HCI_INQUIRY[0], HCI_INQUIRY[1], HCI_INQUIRY[2], HCI_INQUIRY[3], HCI_INQUIRY[4],
					HCI_INQUIRY[5], HCI_INQUIRY[6], HCI_INQUIRY[7], HCI_INQUIRY[8]);

	bt_driver_cli_parse_inq_resp();
	b_inq_cli_running = 0;
	bt_driver_register_event_cb(NULL, 1);
	BT_DRV_CLI_LOGI("%s, end success", __func__);
	return 0;

inq_fail:
	b_inq_cli_running = 0;
	bt_driver_register_event_cb(NULL, 1);
	BT_DRV_CLI_LOGE("%s, end fail", __func__);
	return 0;
}

unsigned char bt_driver_cli_coredump(unsigned char len, char *param[])
{
	return btmtk_trigger_fw_assert();
}

unsigned char bt_driver_cli_ice_fwdl(unsigned char len, char *param[])
{
	BT_DRV_CLI_LOGI("%s: len = %d, param[0] = %s", __func__, len, param[0]);
	btmtk_set_ice_fwdl(param[0]);
	return 0;
}

unsigned char bt_driver_cli_bgfsys_on(unsigned char len, char *param[])
{
	bt_driver_bgfsys_on();
	return 0;
}

unsigned char bt_driver_cli_snoop_on(unsigned char len, char *param[])
{
#ifdef BT_DRV_BTSNOOP_TO_UART
	bt_driver_btsnoop_ctrl(1);
#else
	BT_DRV_CLI_LOGW("BT_DRV_BTSNOOP_TO_UART is not defined");
#endif
	return 0;
}
#endif /* #ifndef MTK_BT_DRV_CLI_LITE */
unsigned char bt_driver_cli_dl_fw(unsigned char len, char *param[])
{
	bt_driver_dlfw();
	return 0;
}

unsigned char bt_driver_cli_bt_on(unsigned char len, char *param[])
{
	bt_driver_func_on();
	return 0;
}

unsigned char bt_driver_cli_own_control(unsigned char len, char *param[])
{
	unsigned char ctrl = 0;

	if (len != 1) {
		BT_DRV_CLI_LOGE("%s Usage: btdrv own_ctrl <0 or 1>", __func__);
		return 0;
	}
	ctrl = _strtoui(param[0]);
	BT_DRV_CLI_LOGI("%s own control: %s", __func__, ctrl ? "open" : "close");
	bt_driver_set_own_ctrl(ctrl);
	return 0;
}

unsigned char bt_driver_cli_set_own_type(unsigned char len, char *param[])
{
	unsigned char own_type = 0;

	if (len != 1) {
		BT_DRV_CLI_LOGE("%s Usage: btdrv own_set <0 or 1>", __func__);
		return 0;
	}
	own_type = _strtoui(param[0]);
	BT_DRV_CLI_LOGI("%s own set: %s", __func__, own_type ? "driver own" : "fw own");
	bt_driver_set_own_type(own_type);

	return 0;
}

#ifndef MTK_BT_DRV_CLI_LITE
unsigned char bt_driver_cli_dump_bus_hang_reg(unsigned char len, char *param[])
{
	dump_bus_hang_reg();
	return 0;
}

unsigned char bt_driver_cli_dump_sleep_fail_reg(unsigned char len, char *param[])
{
	btmtk_dump_sleep_fail_reg();
	return 0;
}
#endif /* #ifndef MTK_BT_DRV_CLI_LITE */
#ifdef MTK_BT_DRV_CHIP_RESET
unsigned char bt_driver_cli_whole_chip_reset(unsigned char len, char *param[])
{
	btmtk_whole_chip_reset();
	return 0;
}

unsigned char bt_driver_cli_subsys_chip_reset(unsigned char len, char *param[])
{
	btmtk_subsys_chip_reset();
	return 0;
}
#endif
unsigned char bt_driver_cli_write_btcfg(unsigned char len, char *param[])
{
	if (len < 2) {
		BT_DRV_CLI_LOGE("%s Usage: btdrv wbtcfg <key value>", __func__);
		return 0;
	}
	btmtk_write_bt_cfg(param[0], &param[1], len - 1);
	return 0;
}

unsigned char bt_driver_cli_read_btcfg(unsigned char len, char *param[])
{
	btmtk_read_bt_cfg();
	return 0;
}

unsigned char bt_driver_cli_del_btcfg(unsigned char len, char *param[])
{
	if (len < 1) {
		BT_DRV_CLI_LOGE("%s Usage: btdrv dcfg <key>", __func__);
		return 0;
	}
	btmtk_delete_bt_cfg(param[0]);
	return 0;
}

unsigned char bt_driver_cli_write_country_btpwr(unsigned char len, char *param[])
{
	if (len != 7) {
		BT_DRV_CLI_LOGE("Usage:");
		BT_DRV_CLI_LOGE("btdrv wcbp <city, mode, edr_pwr, ble_pwr, ble_pwr_2m, ble_s2, ble_s8>");
		BT_DRV_CLI_LOGE("city: AU, SA, US, JP or other");
		BT_DRV_CLI_LOGE("mode: 0 or 1");
		BT_DRV_CLI_LOGE("edr_pwr: -32 ~ 17");
		BT_DRV_CLI_LOGE("ble_pwr: -29 ~ 20");
		BT_DRV_CLI_LOGE("ble_pwr_2m: -29 ~ 20");
		BT_DRV_CLI_LOGE("ble_s2: -29 ~ 20");
		BT_DRV_CLI_LOGE("ble_s8: -29 ~ 20");
		return 0;
	}
	btmtk_write_country_bt_power(param[0], &param[1]);
	return 0;
}

unsigned char bt_driver_cli_read_country_btpwr(unsigned char len, char *param[])
{
	btmtk_read_country_bt_power();
	return 0;
}

unsigned char bt_driver_cli_del_country_btpwr(unsigned char len, char *param[])
{
	if (len < 1) {
		BT_DRV_CLI_LOGE("Usage: dcbp <cty>");
		return 0;
	}
	btmtk_delete_country_bt_power(param[0]);
	return 0;
}

unsigned char bt_driver_cli_show_buf_trace(unsigned char len, char *param[])
{
	btmtk_buffer_debug();
	return 0;
}

unsigned char bt_driver_cli_get_own_type(unsigned char len, char *param[])
{
	if (bt_driver_get_own_ctrl())
		BT_DRV_CLI_LOGI("own_type: in driver %s, in CR %s",
						bt_driver_get_own_type() ? "DRIVER_OWN" : "FW_OWN",
						btmtk_get_own_type() ? "DRIVER_OWN" : "FW_OWN");
	else
		BT_DRV_CLI_LOGI("own_ctrl is 0, own_type: in CR %s",
						btmtk_get_own_type() ? "DRIVER_OWN" : "FW_OWN");
	return 0;
}

#ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET
unsigned char bt_driver_cli_change_main_task_timeout(unsigned char len, char *param[])
{
	uint8_t val = 0;

	if (len != 1) {
		BT_DRV_CLI_LOGE("%s Usage: btdrv maintasktimeout <value>", __func__);
		return 0;
	}
	val = _strtoui(param[0]);
	btif_task_change_timeout_setting((uint8_t)val);
	return 0;
}

unsigned char bt_driver_cli_get_main_task_timeout(unsigned char len, char *param[])
{
	BT_DRV_CLI_LOGI("current timeout setting : %d", btif_task_get_timeout_setting());
	return 0;
}
#endif /* #ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET */

unsigned char bt_driver_cli_print_version_info(unsigned char len, char *param[])
{
	bt_driver_print_version_info();
	return 0;
}

unsigned char bt_driver_cli_read_cordump_info(unsigned char len, char *param[])
{
	btmtk_read_coredump_info();
	return 0;
}

#ifdef BTIF_MAIN_TASK_TRACE
unsigned char bt_driver_cli_show_main_task_trace(unsigned char len, char *param[])
{
	btif_main_task_record_dump();
	return 0;
}
#endif

cmd_t bt_driver_cli[] = {
	{ "sdl",        "set log level",                    bt_driver_cli_set_dbg_lvl,  NULL },
	{ "dlfw",       "download firmware",                bt_driver_cli_dl_fw,        NULL },
	{ "bt_on",      "Power on BT",                      bt_driver_cli_bt_on,        NULL },
	{ "close",      "deinit driver",                    bt_driver_cli_deinit,       NULL },
	{ "tx",         "send dummy data",                  bt_driver_cli_send,         NULL },
	{ "own_ctrl",   "fw own and driver own control",    bt_driver_cli_own_control,  NULL },
	{ "own_set",    "set fw own or driver own",         bt_driver_cli_set_own_type, NULL },
	{ "own_get",    "get driver own type",              bt_driver_cli_get_own_type, NULL },
#ifndef MTK_BT_DRV_CLI_LITE
	{ "inq",        "inquiry test",                     bt_driver_cli_inquiry,      NULL },
	{ "coredump",   "trigger coredump",                 bt_driver_cli_coredump,     NULL },
	{ "ice_fwdl",   "skip fw dl and send BT power on",  bt_driver_cli_ice_fwdl,     NULL },
	{ "bgfsys_on",  "power on bgfsys",                  bt_driver_cli_bgfsys_on,    NULL },
	{ "snoop_on",   "capture HCI log in driver",        bt_driver_cli_snoop_on,     NULL },
	{ "dump_reg",   "dump register",                    bt_driver_cli_dump_reg,     NULL },
	{ "lb_set",     "enable or disable btif loopback",  bt_driver_cli_loopback_set, NULL },
	{ "dump_bhr",   "dump bus hang register",           bt_driver_cli_dump_bus_hang_reg,    NULL },
	{ "dump_sfr",   "dump sleep fail register",         bt_driver_cli_dump_sleep_fail_reg,  NULL },
#endif /* #ifndef MTK_BT_DRV_CLI_LITE */
#ifdef MTK_BT_DRV_CHIP_RESET
	{ "wrst",       "whole chip reset",                 bt_driver_cli_whole_chip_reset,    NULL },
	{ "srst",       "subsys chip reset",                bt_driver_cli_subsys_chip_reset,   NULL },
#endif /* #ifdef MTK_BT_DRV_CHIP_RESET */
	{ "wbtcfg",     "write bt config",                  bt_driver_cli_write_btcfg,         NULL },
	{ "rbtcfg",     "read bt config",                   bt_driver_cli_read_btcfg,          NULL },
	{ "dbtcfg",     "delete bt config",                 bt_driver_cli_del_btcfg,           NULL },
	{ "wcbp",       "write country bt power",           bt_driver_cli_write_country_btpwr, NULL },
	{ "rcbp",       "read country bt power",            bt_driver_cli_read_country_btpwr,  NULL },
	{ "dcbp",       "delete country bt power",          bt_driver_cli_del_country_btpwr,   NULL },
	{ "bufdebug",   "show buffer trace log",            bt_driver_cli_show_buf_trace,      NULL },
#ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET
	{ "settasktimeout",   "Set btif main task timeout (test only)",
												   bt_driver_cli_change_main_task_timeout, NULL },
	{ "gettasktimeout",   "Get btif main task timeout (test only)",
													  bt_driver_cli_get_main_task_timeout, NULL },
#endif /* #ifdef MTK_BT_DRV_MAIN_TASK_TIMEOUT_SET */
	{ "ver",        "show bt version info",             bt_driver_cli_print_version_info,  NULL },
	{ "dumpinfo",   "read coredump info from flash",    bt_driver_cli_read_cordump_info,   NULL },
#ifdef BTIF_MAIN_TASK_TRACE
	{ "mtdbg",      "show main task trace log",         bt_driver_cli_show_main_task_trace, NULL },
#endif
	{ NULL,         NULL,                               NULL,                       NULL },
};
#endif // CHIP_MT7933

