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

#ifdef BT_DRV_BTSNOOP_TO_UART
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include "bt_driver_btsnoop.h"
#include "bt_driver.h"

static unsigned char g_btsnoop_log_enable;
TaskHandle_t btsnoop_task_hdl;

#define PACKET_NUM (20)
// stored up to 20 packet, that should be enough
struct btsnoop_packet_t packet_list[PACKET_NUM];

unsigned char g_next_packet_idx;
xSemaphoreHandle packet_list_mtx;

extern int btsnoop_log_write_to_uart(unsigned char *buf, int len);

// btsnoop_log_write_to_uart shall be platform specific
#pragma weak btsnoop_log_write_to_uart = default_btsnoop_log_write_to_uart
static int default_btsnoop_log_write_to_uart(unsigned char *buf, int len)
{
	return 0;
}

static int _is_packet_list_empty(void)
{
	unsigned char i = 0;

	for (i = 0; i < PACKET_NUM; i++) {
		if (packet_list[i].b_valid)
			break;
	}
	return (i == PACKET_NUM);
}

static unsigned char _get_next_avai_packet_entry(void)
{
	unsigned char i = 0;

	for (i = g_next_packet_idx; i < PACKET_NUM; i++) {
		if (!packet_list[i].b_valid)
			return i;
	}
	if (i == PACKET_NUM) { // search from head
		for (i = 0; i < g_next_packet_idx; i++) {
			if (!packet_list[i].b_valid)
				return i;
		}
		if (i == g_next_packet_idx) {
			BT_DRV_LOGE("%s packet_list is full", __func__);
			return PACKET_NUM;
		}
	}
	return PACKET_NUM;
}

void bt_driver_btsnoop_ctrl(unsigned char enable)
{
	BT_DRV_LOGI("%s enable: %d", __func__, enable);
	g_btsnoop_log_enable = enable;
}

int bt_driver_btsnoop_push_packet(unsigned char type, unsigned char *buffer, unsigned int length)
{
	unsigned char idx;
	unsigned int len = length - 1; // remove packet indicator
	unsigned char head[5] = {0xAB, 0xCD, type,
							(unsigned char)((len & 0xff00) >> 8),
							(unsigned char)(len & 0xff)};

	if (!g_btsnoop_log_enable)
		return -1;

	if ((buffer == NULL) || (len <= 0)) {
		BT_DRV_LOGE("%s invalid args", __func__);
		return -1;
	}

	// btif_util_dump_buffer("push packet", buffer, length, 0);
	if (xSemaphoreTake(packet_list_mtx, portMAX_DELAY) == pdFALSE) {
		BT_DRV_LOGE("%s, packet_list_mtx get failed!", __func__);
		return -1;
	}
	idx = _get_next_avai_packet_entry();
	if (idx == PACKET_NUM)
		goto fail;

	packet_list[idx].packet = (unsigned char *)pvPortMalloc(len + sizeof(head));
	if (!packet_list[idx].packet) {
		BT_DRV_LOGE("%s malloc packet buf fail", __func__);
		goto fail;
	}
	memcpy(packet_list[idx].packet, head, sizeof(head));
	memcpy(packet_list[idx].packet + sizeof(head), &buffer[1], len);
	packet_list[idx].len = len + sizeof(head);
	packet_list[idx].b_valid = pdTRUE;

	BT_DRV_LOGV("btsnoop packet %d is pushed", idx);

	xSemaphoreGive(packet_list_mtx);

	if (btsnoop_task_hdl)
		xTaskNotifyGive(btsnoop_task_hdl);

	return 0;

fail:
	xSemaphoreGive(packet_list_mtx);
	return -1;
}

int bt_driver_btsnoop_pop_packet(void)
{
	if (!g_btsnoop_log_enable)
		return pdFALSE;

	if (xSemaphoreTake(packet_list_mtx, portMAX_DELAY) == pdFALSE) {
		BT_DRV_LOGE("%s, packet_list_mtx get failed!", __func__);
		return pdFALSE;
	}

	if (_is_packet_list_empty()) {
		xSemaphoreGive(packet_list_mtx);
		return pdFALSE;
	}
	if (packet_list[g_next_packet_idx].b_valid &&
		packet_list[g_next_packet_idx].packet && (packet_list[g_next_packet_idx].len > 0)) {

		BT_DRV_LOGV("btsnoop packet %d write to uart", g_next_packet_idx);

		//btif_util_dump_buffer("pop packet", packet_list[g_next_packet_idx].packet,
		//						 packet_list[g_next_packet_idx].len, 0);
		btsnoop_log_write_to_uart(packet_list[g_next_packet_idx].packet,
								  packet_list[g_next_packet_idx].len);

		packet_list[g_next_packet_idx].len = 0;
		packet_list[g_next_packet_idx].b_valid = pdFALSE;
		vPortFree(packet_list[g_next_packet_idx].packet);
		packet_list[g_next_packet_idx].packet = NULL;

		BT_DRV_LOGV("btsnoop packet %d is popped", g_next_packet_idx);
		if (++g_next_packet_idx == PACKET_NUM)
			g_next_packet_idx = 0;
	}

	xSemaphoreGive(packet_list_mtx);
	return pdTRUE;
}

static void bt_driver_btsnoop_task(void *arg)
{
	BT_DRV_LOGD("%s is running", __func__);
	while (1) {
		if (pdFALSE == bt_driver_btsnoop_pop_packet()) {
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for data
			continue;
		} else {
			continue;
		}
	}
}

int bt_driver_btsnoop_init(void)
{
	if (btsnoop_task_hdl != NULL) {
		BT_DRV_LOGI("%s already init", __func__);
		return 0;
	}
	BT_DRV_LOGD("%s", __func__);
	packet_list_mtx = xSemaphoreCreateMutex();
	if (packet_list_mtx == NULL) {
		BT_DRV_LOGE("%s create mutex fail.", __func__);
		return -1;
	}
	memset(packet_list, 0, sizeof(packet_list));
	if (pdPASS != xTaskCreate(bt_driver_btsnoop_task, "btsnoop",
							  (1024 * 4) / sizeof(StackType_t), NULL,
							  configMAX_PRIORITIES - 2,
							  &btsnoop_task_hdl)) {
		BT_DRV_LOGE("%s create bt_driver_btsnoop_task fail.", __func__);
		vSemaphoreDelete(packet_list_mtx);
	}
	return 0;
}
#endif
