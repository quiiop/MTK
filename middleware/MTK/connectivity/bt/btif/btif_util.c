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
#include "semphr.h"
#include "btif_util.h"
#include <string.h>

static unsigned char g_btif_log_lvl = BTIF_LOG_LVL_I;
static int buffer_dump_enable;
static uint16_t buf_print_times = 1; // bytes = times * 16
// static xSemaphoreHandle dump_buff_sema = NULL;
#ifdef MTK_MT7933_BT_ENABLE
log_create_module(BTIF, PRINT_LEVEL_INFO);
log_create_module(BOOTS, PRINT_LEVEL_INFO);
log_create_module(PICUS, PRINT_LEVEL_INFO);
#endif

void btif_set_log_lvl(unsigned char lvl, int dump_buffer, int times)
{
	if (lvl > BTIF_LOG_LVL_V)
		g_btif_log_lvl = BTIF_LOG_LVL_V;

	g_btif_log_lvl = lvl;
	buffer_dump_enable = dump_buffer;
	if (times > 16)
		buf_print_times = 16;
	else
		buf_print_times = times;
}

unsigned char btif_get_log_lvl(void)
{
	return g_btif_log_lvl;
}

unsigned int btif_strtoui(const char *str)
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

xSemaphoreHandle btif_util_binary_sema_create(void)
{
	xSemaphoreHandle handle = NULL;

	handle = xSemaphoreCreateBinary();
	if (handle != NULL)
		xSemaphoreGive(handle);
	return handle;
}

xSemaphoreHandle btif_util_mutex_create(void)
{
	xSemaphoreHandle handle = NULL;

	handle = xSemaphoreCreateMutex();
	// if (handle != NULL)
	//  xSemaphoreGive(handle);
	return handle;
}

void btif_util_sema_delete(xSemaphoreHandle sema)
{
	vSemaphoreDelete(sema);
}

int btif_util_sema_lock(xSemaphoreHandle sema, unsigned char from_isr)
{
	if (sema != NULL) {
		if (from_isr)
			return xSemaphoreTakeFromISR(sema, NULL);
		else
			return xSemaphoreTake(sema, portMAX_DELAY);
	}
	return 0;
}

void btif_util_sema_unlock(xSemaphoreHandle sema, unsigned char from_isr)
{
	if (sema != NULL) {
		if (from_isr)
			xSemaphoreGiveFromISR(sema, NULL);
		else
			xSemaphoreGive(sema);
	}
}

xTimerHandle btif_util_timer_create(const char *const pcTimerName,
									unsigned int timer_ms, int repeat,
									TimerCallbackFunction_t pxCallbackFunction)
{
	return xTimerCreate(pcTimerName, pdMS_TO_TICKS(timer_ms), repeat,
					    (void *)0, pxCallbackFunction);
}

void btif_util_timer_start(xTimerHandle timer)
{
	if (xTimerStart(timer, 1) != pdPASS) {
		if (xTimerStart(timer, 1) != pdPASS)
			BTIF_LOG_E("%s fail\n", __func__);
	}
}

void btif_util_timer_stop(xTimerHandle timer)
{
	if (xTimerStop(timer, 1) != pdPASS) {
		if (xTimerStop(timer, 1) != pdPASS)
			BTIF_LOG_E("%s fail\n", __func__);
	}
}

void btif_util_timer_reset(xTimerHandle timer)
{
	if (xTimerReset(timer, 1) != pdPASS) {
		if (xTimerReset(timer, 1) != pdPASS)
			BTIF_LOG_E("%s fail\n", __func__);
	}
}

void btif_util_dump_buffer(const char *label, const unsigned char *buf,
						   unsigned int length, unsigned char force_print)
{
	char string[256] = {0};
	unsigned char temp_buf[16] = {0};
	unsigned int i = 0;
	unsigned int cycle = length / 16 + 1;
	unsigned int cnt = 0;

	if ((!force_print) && (!buffer_dump_enable))
		return;

	if ((label == NULL) || (buf == NULL) || (length == 0))
		return;

	if (cycle > buf_print_times)
		cycle = buf_print_times;

	for (i = 0; i < cycle; i++) {
		unsigned char cp_len = (i + 1) * 16 < length ? 16 : (length - i * 16);

		memset(temp_buf, 0, 16);
		memset(string, 0, 128);
		memcpy(temp_buf, &buf[i * 16], cp_len);
		cnt = snprintf(string, 128,
			  "%s: [%d][%d~%d] %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
			  label, length, i * 16, i * 16 + cp_len - 1, temp_buf[0],
			  temp_buf[1], temp_buf[2], temp_buf[3], temp_buf[4], temp_buf[5],
			  temp_buf[6], temp_buf[7], temp_buf[8], temp_buf[9], temp_buf[10],
			  temp_buf[11], temp_buf[12], temp_buf[13], temp_buf[14], temp_buf[15]);
		if (cnt <= 0 || cnt > 128) {
			BTIF_LOG_E("%s, L: %d, snprintf error", __func__, __LINE__);
			break;
		}
		if (cp_len < 16) {
			unsigned int len = strlen(string);

			string[len - 3 * (16 - cp_len)] = '\0';
		}
		BTIF_LOG_I("%s", string);
		if (i * 16 + cp_len >= length)
			break;
	}
}

void btif_util_task_delay_ms(unsigned int ms)
{
	//TickType_t old = xTaskGetTickCount();
	//vTaskDelayUntil(&old, pdMS_TO_TICKS(ms));
	vTaskDelay(pdMS_TO_TICKS(ms));
}

void btif_util_task_delay_ticks(unsigned int ticks)
{
	//TickType_t old = xTaskGetTickCount();
	//vTaskDelayUntil(&old, (TickType_t)ticks);
	vTaskDelay(ticks);
}

void *btif_util_malloc(unsigned int size)
{
	return pvPortMalloc(size);
}

void btif_util_free(void *ptr)
{
	if (ptr)
		vPortFree(ptr);
}
/*---------------------------------------------------------------------------*/
