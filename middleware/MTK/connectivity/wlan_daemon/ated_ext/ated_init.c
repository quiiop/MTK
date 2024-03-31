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

/* Includes ------------------------------------------------------------------*/
#include "hal_uart.h"
#include "hal_pinmux_define.h"
#include "gl_qa_agent.h"
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "limits.h"  /* for ULONG_MAX */
#include "semphr.h"
#include "task_def.h"
#include "lwip/def.h"
#include "hal_gpio.h"
#include "ated_init.h"
#if (MTK_SLT_WIFI == 1)
#include "slt.h"
#include "slt_wifi.h"
#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ATED_DEBUG 0

#if ATED_DEBUG
#define CONFIG_SET_TRANS_LEN 1
#define CONFIG_SET_PADDING 1
#else
#define CONFIG_SET_TRANS_LEN 0
#define CONFIG_SET_PADDING 0
#endif

#define ATED_INIT_TASK_PRI  (configMAX_PRIORITIES - 2)
//#define ATED_INIT_TASK_PRI  TASK_PRIORITY_NORMAL
#define ATED_INIT_STACK_SIZE (configMINIMAL_STACK_SIZE * 2) /* unit in word */

#define ATED_CMD_UART_PORT HAL_UART_1

#define CMD_HEADER_SIZE 12
#define CMD_DATA_SIZE 2048
#define HQA_CMD_MAGIC_NO 0x18142880

#define ATED_TX_BIT 0x01
#define ATED_RX_BIT 0x02


#if CONFIG_SET_PADDING
enum ATED_PADDING_MODE {
	ATED_PADDING_MODE_NONE,
	ATED_PADDING_MODE_DATA,
	ATED_PADDING_MODE_TOTAL,
};
#endif

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static TaskHandle_t ated_task_handler = NULL;
struct ated_data *g_ated_buffer;
enum ATED_STATUS g_ated_status;

#if (MTK_SLT_WIFI == 1)
uint8_t g_ated_role = 0;
#define ATED_ROLE_TX_CMD 0 /* TX -> RX RESP */
#define ATED_ROLE_RX_CMD 1 /* RX -> DO -> send RESP */
#endif

uint8_t g_ated_baudrate = HAL_UART_BAUDRATE_921600;
#if CONFIG_SET_TRANS_LEN
uint32_t g_ated_trans_len;
#endif
#if CONFIG_SET_PADDING
enum ATED_PADDING_MODE g_ated_padding_mode = ATED_PADDING_MODE_NONE;
uint32_t g_ated_padding_len = 0;
#endif


/* Private functions ---------------------------------------------------------*/


void print_buf(uint8_t *pbuf, uint32_t len)
{
	uint32_t i;

	for(i = 0; i < len; i++) {
		printf("0x%02x ", pbuf[i]);
		if ((i+1) % 16 == 0)
			printf("\r\n");
	}
	printf("\r\n");

	return;
}

uint8_t ated_get_cmd_header(struct ated_data* data)
{
	uint32_t read_cnt;
	uint32_t rcv_cnt;
	uint8_t *pbuf;

	read_cnt = data->has_read;
	pbuf = data->cmd_buf;

	rcv_cnt = hal_uart_receive_dma(ATED_CMD_UART_PORT, &pbuf[read_cnt], CMD_HEADER_SIZE - read_cnt);
	read_cnt += rcv_cnt;

#if ATED_DEBUG
	printf("[ATED] get header %ld\r\n", read_cnt);
#endif
	if (read_cnt == CMD_HEADER_SIZE) {
		if (ntohl(((struct HQA_CMD_FRAME *)pbuf)->MagicNo) == HQA_CMD_MAGIC_NO) {
			g_ated_status = ATED_STATUS_READ_CMD_DATA;
#if ATED_DEBUG
			printf("[ATED] get header done, %ld\r\n", read_cnt);
#endif
		}
		else {
			int mov;
			int new;
			printf("[ATED] not magic no, read_cnt = %ld\r\n", read_cnt);
			for (mov = 1; mov < read_cnt; mov ++) {
				if (pbuf[mov] == 0x18) {
					break;
				}
			}
			read_cnt -= mov;
			for (new = 0; new < read_cnt; new ++) {
				pbuf[new] = pbuf[mov+new];
			}
			printf("[ATED] not magic no, read_cnt = %ld\r\n", read_cnt);
		}
	}

	data->has_read = read_cnt;
	return 0;
}
uint8_t ated_get_cmd_data(struct ated_data* data)
{
	uint32_t read_cnt;
	uint32_t rcv_cnt;
	uint32_t cmd_len;
	uint32_t total_len;
	uint8_t *pbuf;
#if (MTK_SLT_WIFI  == 1)
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
#endif

	read_cnt = data->has_read;
	pbuf = data->cmd_buf;

	cmd_len = ntohs(((struct HQA_CMD_FRAME *)pbuf)->Length);
	if (cmd_len > CMD_DATA_SIZE) {
		printf("[ATED] error cmd data len: %ld > %d, skip.\r\n", cmd_len, CMD_DATA_SIZE);
		g_ated_status = ATED_STATUS_IDLE;
	}
	else{
		total_len = (CMD_HEADER_SIZE + cmd_len);
		rcv_cnt = hal_uart_receive_dma(ATED_CMD_UART_PORT, &pbuf[read_cnt], total_len-read_cnt);
		read_cnt += rcv_cnt;

		if( read_cnt == total_len ) {
			printf("[ATED] read cmd done, read total: %ld, receive: %ld\r\n", total_len, read_cnt);
			g_ated_status = ATED_STATUS_DO_CMD;
			print_buf(pbuf, total_len);
#if (MTK_SLT_WIFI == 1)
			if (g_ated_role == ATED_ROLE_TX_CMD) {
#ifndef SLT_GOLDEN
				memcpy(&slt_hqa_frame_data, pbuf, total_len);
#endif
				g_ated_status = ATED_STATUS_IDLE;
				xTaskNotifyFromISR(sltwifi_task_handler, 1, eSetBits, &xHigherPriorityTaskWoken);
			}
#endif
		}
	}
	data->has_read = read_cnt;

	return 0;
}

uint8_t ated_get_cmd(struct ated_data* data)
{
#if (MTK_SLT_WIFI == 1)
	if (g_ated_status == ATED_STATUS_READ_RESP)
		g_ated_role = ATED_ROLE_TX_CMD;
	else if (g_ated_status == ATED_STATUS_IDLE)
		g_ated_role = ATED_ROLE_RX_CMD;
#endif

	if (g_ated_status == ATED_STATUS_IDLE
#if (MTK_SLT_WIFI == 1)
		|| g_ated_status == ATED_STATUS_READ_RESP
#endif
		)
		g_ated_status = ATED_STATUS_READ_CMD_HEADER;

	if (g_ated_status == ATED_STATUS_READ_CMD_HEADER) {
#if ATED_DEBUG
		printf("[ATED] status: %d, get header\r\n", g_ated_status);
#endif
		ated_get_cmd_header(data);
	}

	if (g_ated_status == ATED_STATUS_READ_CMD_DATA) {
#if ATED_DEBUG
		printf("[ATED] status: %d, get data\r\n", g_ated_status);
#endif
		ated_get_cmd_data(data);
	}

	return 0;
}

uint8_t ated_do_cmd(struct ated_data* data)
{
	uint8_t *pbuf;
	int32_t ret = 0;
#if ATED_DEBUG
	uint32_t cmd_len;
#endif
	if (g_ated_status == ATED_STATUS_DO_CMD) {
		pbuf = data->cmd_buf;
#if (MTK_SLT_WIFI == 1)
#ifdef SLT_GOLDEN
		if (((struct HQA_CMD_FRAME *)pbuf)->Id == htons(0xffff) &&
			((struct HQA_CMD_FRAME *)pbuf)->Length == htons(0x0))
		{
			portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
			xTaskNotifyFromISR(sltwifi_task_handler, 1, eSetBits, &xHigherPriorityTaskWoken);
			return ret;
		}
#endif
#endif
		if (((struct HQA_CMD_FRAME *)pbuf)->Id == htons(0x1314)) {
			((struct HQA_CMD_FRAME *)pbuf)->Length = htons(0x0006);
			(((struct HQA_CMD_FRAME *)pbuf)->Data)[1] = htons(0x01);
		}
		else
			ret = priv_qa_agent(pbuf, (char *)&data->has_read);
		((struct HQA_CMD_FRAME *)pbuf)->Type = htons(0x8008);
		if (ret == 0x0105) {
			printf("[ATED] cmd not support, return: 0x%lx\r\n", ret);
			((struct HQA_CMD_FRAME *)pbuf)->Length =
				htons(((struct HQA_CMD_FRAME *)pbuf)->Length);
		}
#if ATED_DEBUG
		cmd_len = ntoh(((struct HQA_CMD_FRAME *)pbuf)->Length);
		printf("[ATED] do cmd done, resp_len: %ld\r\n", cmd_len);
#endif
		g_ated_status = ATED_STATUS_SEND_RESP;
	}
	return ret;
}

#if (MTK_SLT_WIFI == 1)
#define ATED_MAX_ONE_WRITE_LEN 32
#endif
uint8_t ated_send_resp(struct ated_data* data)
{
	uint8_t *pbuf;
	uint32_t total_len;
	uint32_t send_remain;
	uint32_t has_send;
	uint32_t write_cnt;

	if (g_ated_status == ATED_STATUS_SEND_RESP) {
		pbuf = data->cmd_buf;
		total_len = ntohs(((struct HQA_CMD_FRAME *)pbuf)->Length) + CMD_HEADER_SIZE;
		has_send = data->has_send;
		send_remain = (total_len - has_send);

#if (MTK_SLT_WIFI == 1)
		if (send_remain > ATED_MAX_ONE_WRITE_LEN) {
			write_cnt = hal_uart_send_dma(ATED_CMD_UART_PORT, &pbuf[has_send], ATED_MAX_ONE_WRITE_LEN);
			data->has_send += write_cnt ;
		} else
#endif
		{
			write_cnt = hal_uart_send_dma(ATED_CMD_UART_PORT, &pbuf[has_send], send_remain);
			data->has_send += write_cnt ;
		}

#if ATED_DEBUG
		printf("[ATED]send data len %ld(%ld)\r\n", write_cnt, send_remain);
#endif

		if (has_send == total_len) {
			g_ated_status = ATED_STATUS_IDLE;
			printf("[ATED] send resp done, resp_len: %ld\r\n", total_len);
			print_buf(pbuf, total_len);
		}
	}
	return 0;
}

#if (MTK_SLT_WIFI == 1)
uint8_t ated_send_cmd(struct ated_data* data)
{
	uint8_t *pbuf;
	uint32_t total_len;
	uint32_t send_remain;
	uint32_t has_send;
	uint32_t write_cnt = 0;

	printf("[ATED] -->ated_send_cmd\r\n");

	if (g_ated_status == ATED_STATUS_SEND_CMD) {
		pbuf = data->cmd_buf;
		total_len = ntohs(((struct HQA_CMD_FRAME *)pbuf)->Length) + CMD_HEADER_SIZE;
		has_send = data->has_send;
		send_remain = (total_len - has_send);

		if (send_remain > ATED_MAX_ONE_WRITE_LEN) {
			write_cnt = hal_uart_send_dma(ATED_CMD_UART_PORT, &pbuf[has_send], ATED_MAX_ONE_WRITE_LEN);
			data->has_send += write_cnt;
		} else {
			write_cnt = hal_uart_send_dma(ATED_CMD_UART_PORT, &pbuf[has_send], send_remain);
			data->has_send += write_cnt;
		}

#if 1
		printf("[ATED] ated_send_cmd: send data len %ld(%ld)\r\n", write_cnt, send_remain);
#endif

		if (has_send == total_len) {
			g_ated_status = ATED_STATUS_READ_RESP;
			printf("[ATED] send cmd done, cmd_len: %ld\r\n", total_len);
			print_buf(pbuf, total_len);
		}
	} else {
		printf("[ATED] ated_send_cmd: ated status is not SEND_CMD!! (%d)\r\n", g_ated_status);
	}
	return 0;
}
#endif

uint8_t ated_reset(struct ated_data* data)
{
	uint32_t reset_len;
	uint8_t *pbuf;

	if (g_ated_status == ATED_STATUS_IDLE
#if (MTK_SLT_WIFI == 1)
		|| g_ated_status == ATED_STATUS_READ_RESP
#endif
		) {
		pbuf = data->cmd_buf;
		reset_len = ntohs(((struct HQA_CMD_FRAME *)pbuf)->Length)
			+ CMD_HEADER_SIZE;
		memset(pbuf, 0, reset_len * sizeof(uint8_t));
		pbuf[0] = 0xff; /* for debug */
		data->has_read = 0;
		data->has_send = 0;
	}
	return 0;
}

void start_ATED(void){

	struct ated_data *pbuf;
	uint32_t ulNotifyValue;

	BaseType_t xResult;

	printf("[ATED] in %s\r\n", __func__);
	pbuf = g_ated_buffer;
	for(;;){
		xResult = xTaskNotifyWait(pdFALSE, ULONG_MAX, &ulNotifyValue, portMAX_DELAY);
		if (xResult == pdPASS)
		{
			if ((ulNotifyValue & ATED_RX_BIT) != 0)
			{
				ated_get_cmd(pbuf);

				ated_do_cmd(pbuf);

			}

			if ((ulNotifyValue & ATED_TX_BIT) != 0 || g_ated_status == ATED_STATUS_SEND_RESP
#if (MTK_SLT_WIFI == 1)
				|| g_ated_status == ATED_STATUS_SEND_CMD
#endif
				) {

#if (MTK_SLT_WIFI == 1)
				if (g_ated_status == ATED_STATUS_SEND_RESP)
					ated_send_resp(pbuf);
				if (g_ated_status == ATED_STATUS_SEND_CMD)
					ated_send_cmd(pbuf);
#else
				ated_send_resp(pbuf);
#endif

				ated_reset(pbuf);
			}

		}
	}

	return;
}

static void user_uart_callback(hal_uart_callback_event_t status, void *user_data)
{

	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	if (ated_task_handler == NULL){
		printf("[ATED] %s: ERROR!! handler is NULL.\r\n", __func__);
		return;
	}

	if(status == HAL_UART_EVENT_READY_TO_WRITE)
	{
		xTaskNotifyFromISR(ated_task_handler, ATED_TX_BIT, eSetBits, &xHigherPriorityTaskWoken);
	}
	else if(status == HAL_UART_EVENT_READY_TO_READ)
	{
		xTaskNotifyFromISR(ated_task_handler, ATED_RX_BIT, eSetBits, &xHigherPriorityTaskWoken);
	}
	return;
}
static unsigned int ated_uart_get_baudrate_from_enum(uint8_t baud)
{
    switch (baud){
    case HAL_UART_BAUDRATE_110:
        return 110;
    case HAL_UART_BAUDRATE_300:
        return 300;
    case HAL_UART_BAUDRATE_1200:
        return 1200;
    case HAL_UART_BAUDRATE_2400:
        return 2400;
    case HAL_UART_BAUDRATE_4800:
        return 4800;
    case HAL_UART_BAUDRATE_9600:
        return 9600;
    case HAL_UART_BAUDRATE_19200:
        return 19200;
    case HAL_UART_BAUDRATE_38400:
        return 38400;
    case HAL_UART_BAUDRATE_57600:
        return 57600;
    case HAL_UART_BAUDRATE_115200:
        return 115200;
    case HAL_UART_BAUDRATE_230400:
        return 230400;
    case HAL_UART_BAUDRATE_460800:
        return 460800;
    case HAL_UART_BAUDRATE_921600:
        return 921600;
    default:
        return HAL_UART_STATUS_ERROR_PARAMETER;
    }
}

static unsigned int ated_uart_get_enum_from_baudrate(uint32_t baud)
{
    switch (baud){
    case 110:
	return HAL_UART_BAUDRATE_110;
    case 330:
	return HAL_UART_BAUDRATE_300;
    case 1200:
	return HAL_UART_BAUDRATE_1200;
    case 2400:
	return HAL_UART_BAUDRATE_2400;
    case 4800:
	return HAL_UART_BAUDRATE_4800;
    case 9600:
        return HAL_UART_BAUDRATE_9600;
    case 19200:
        return HAL_UART_BAUDRATE_19200;
    case 38400:
	return HAL_UART_BAUDRATE_38400;
    case 57600:
	return HAL_UART_BAUDRATE_57600;
    case 115200:
	return HAL_UART_BAUDRATE_115200;
    case 230400:
	return HAL_UART_BAUDRATE_230400;
    case 460800:
	return HAL_UART_BAUDRATE_460800;
    case 921600:
	return HAL_UART_BAUDRATE_921600;
    default:
        return HAL_UART_STATUS_ERROR_PARAMETER;
    }
}

int ated_mt7933_uart_init(void)
{
	/* hal_uart_status_t status_t; */
	hal_uart_config_t uart_config;
	/* hal_uart_dma_config_t dma_config; */
	int ret = 0;

#if 0
	hal_gpio_init(HAL_GPIO_17);
	hal_gpio_init(HAL_GPIO_44);
	hal_pinmux_set_function(HAL_GPIO_17, MT7933_PIN_17_FUNC_UART0_RX);
	hal_pinmux_set_function(HAL_GPIO_44, MT7933_PIN_44_FUNC_UART0_TX);
#endif

	uart_config.baudrate = g_ated_baudrate;
	uart_config.parity = HAL_UART_PARITY_NONE;
	uart_config.stop_bit = HAL_UART_STOP_BIT_1;
	uart_config.word_length = HAL_UART_WORD_LENGTH_8;

	ret = hal_uart_init(ATED_CMD_UART_PORT, &uart_config);

	if (ret != 0)
	{
		printf("[ATED] uart init fail!!\r\n");
		return ret;
	}
	
	hal_uart_register_callback(ATED_CMD_UART_PORT, user_uart_callback, NULL);
	printf("[ATED] uart init rate: %d!!\r\n", ated_uart_get_baudrate_from_enum(g_ated_baudrate));

	return 0;
}

int ated_init_param(void)
{
	printf("[ATED] req buffer init, size: %d\r\n", REQ_BUFFER_SIZE);
	g_ated_buffer = (struct ated_data *)malloc(sizeof(struct ated_data));
	if (!g_ated_buffer) {
		printf("[ATED] req buffer init fail.\r\n");
		return -1;
	}	
	memset(g_ated_buffer, 0, sizeof(struct ated_data));
	g_ated_status = ATED_STATUS_IDLE;
	return 0;
}
static void ated_init(void *data)
{
	int ret = 0;

	printf("[ATED] ated init start\r\n");
	ret = ated_mt7933_uart_init();

	if(ret == 0)
		printf("[ATED] uart init done\r\n");
	else {
		printf("[ATED] uart init fail\r\n");
		return;
	}

	ret = ated_init_param();

	if(ret == 0)
		printf("[ATED] buffer init done\r\n");
	else {
		printf("[ATED] buffer fail\r\n");
		return;
	}
	start_ATED();

	printf("[ATED] ated init success\r\n");
	vTaskDelete(NULL);
	
}

BaseType_t ated_init_task(void)
{
	BaseType_t ret = pdFALSE;

	printf("[ATED] ated task create\r\n");

	ret = xTaskCreate(ated_init, "ated_task"
		, ATED_INIT_STACK_SIZE, NULL
		, ATED_INIT_TASK_PRI, &ated_task_handler);
	if (ret != pdPASS)
	{
		printf("[ATED] create init task failed\r\n");
		return ret;
	}

	printf("[ATED] ated task create success\r\n");
	return ret;
}

BaseType_t ated_finish_task(void)
{
	int ret = -1;
	TaskHandle_t task_handler;

	ret = hal_uart_deinit(ATED_CMD_UART_PORT);
	if (ret)
	{
		printf("[ATED]uart deinit fail\r\n");
		return ret;
	}	
	printf("[ATED]uart deinit success\r\n");

	free(g_ated_buffer);

	task_handler = ated_task_handler;
	ated_task_handler = NULL;

	printf("[ATED]ated task delete\r\n");
	if (task_handler != NULL)
	{
		vTaskDelete(task_handler);
	}
	return ret;
}

uint8_t ated_on(uint8_t argc, char *argv[])
{
	return ated_init_task();
}

uint8_t ated_off(uint8_t argc, char *argv[])
{
	return ated_finish_task();
}

uint8_t ated_set_baudrate(uint8_t argc, char *argv[])
{
	uint8_t cur_rate = g_ated_baudrate;
	uint32_t new_baudrate = 0;
	uint32_t ret;

	if (argc > 1 || argc <= 0) {
		printf("ated_set_baudrate <rate>\r\n");
		return 0;
	}
	new_baudrate = atoi(argv[0]);
	printf("current rate: %d, set rate to: %d\r\n",
		ated_uart_get_baudrate_from_enum(cur_rate), atoi(argv[0]));

	ret = hal_uart_deinit(ATED_CMD_UART_PORT);
	if (ret)
	{
		printf("[ATED]uart deinit fail\r\n");
		return ret;
	}
	g_ated_baudrate = ated_uart_get_enum_from_baudrate(new_baudrate);
	ated_mt7933_uart_init();

	return 0;
}
#if CONFIG_SET_TRANS_LEN
uint8_t ated_set_trans_len(uint8_t argc, char *argv[])
{
	if (argc > 1 || argc <= 0) {
		printf("ated_set_trans_len <len>\r\n");
		return 0;
	}	
	printf("[ATED] set uart transmit %d\r\n", atoi(argv[0]));
	g_ated_trans_len = atoi(argv[0]);
	printf("[ATED] set uart transmit %ld\r\n", g_ated_trans_len);
	return 0;
}
#else
uint8_t ated_set_trans_len(uint8_t argc, char *argv[])
{
	printf("[ATED] set uart transmit len not support\r\n");
	return 0;
}
#endif

#if CONFIG_SET_PADDING
uint8_t ated_set_padding(uint8_t argc, char *argv[])
{
	if (argc > 2 || argc <= 0) {
		printf("ated_set_padding <mode> <len>\r\n");
		return 0;
	}	
	printf("set padding of %s, padding to align %d \r\n", argv[0], atoi(argv[1]));

	if(strncmp("none",argv[0],4)) {
		g_ated_padding_mode = ATED_PADDING_MODE_NONE;
	}
	if(strncmp("data",argv[0],4)) {
		g_ated_padding_mode = ATED_PADDING_MODE_DATA;
	}
	else if(strncmp("total",argv[0],4)) {
		g_ated_padding_mode = ATED_PADDING_MODE_TOTAL;
	}
	else
		printf("unknown padding type %s\r\n", argv[0]);

	if (argc == 2)
		g_ated_padding_len = atoi(argv[1]);
	else
		g_ated_padding_len = 0;

	printf("set padding of mode %d, padding to align %ld \r\n", g_ated_padding_mode, g_ated_padding_len);
	return 0;
}
#else
uint8_t ated_set_padding(uint8_t argc, char *argv[])
{

	printf("[ATED] set padding len not support\r\n");
	return 0;
}
#endif
uint8_t ated_test_write(uint8_t argc, char *argv[])
{

	if (argc > 1 || argc <= 0) {
		printf("ated_test_write \"<content>\"\r\n");
		return 0;
	}

	hal_uart_send_dma(ATED_CMD_UART_PORT,
		(uint8_t *)argv[0], strlen(argv[0])+1);
	printf("[ATED] write %s done\r\n", argv[0]);
	return 0;
}

uint8_t ated_exit_wifi_test_mode(uint8_t argc, char *argv[])
{
	int ret;	
	printf("in %s, send close adapter to wifi\r\n", __func__);
	uint8_t cmd_buff[]={0x18, 0x14, 0x28, 0x80, 0x00, 0x05, 0x10, 0x01, 0x00, 0x00, 0x00, 0x01};

	print_buf(cmd_buff, 12);
	ret = priv_qa_agent(cmd_buff, NULL);
	
	printf("in %s, done with ret %d\r\n", __func__, ret);

	return ret;
}

cmd_t _ated_cli[] = {
	{ "on", "ated on", ated_on},
	{ "off", "ated off", ated_off},
	{ "exit_testmode", "leave wifi testmode", ated_exit_wifi_test_mode},
	{ "set_baudrate", "set uart baudrate", ated_set_baudrate},
	{ "set_trans_len", "set transmit len", ated_set_trans_len},
	{ "set_padding", "set padding", ated_set_padding},
	{ "test_write", "test uart write to PC", ated_test_write},
	{ NULL }
};

