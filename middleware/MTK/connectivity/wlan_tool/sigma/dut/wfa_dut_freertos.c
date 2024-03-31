/****************************************************************************
*
* Copyright (c) 2016 Wi-Fi Alliance
*
* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
* SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
* RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
* NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
* USE OR PERFORMANCE OF THIS SOFTWARE.
*
*****************************************************************************/

/*
 * File: wfa_dut_freertos.c - The main program for DUT agent.
 *       This is the top level of traffic control. It initializes a local TCP
 *       socket for command and control link and waits for a connect request
 *       from a Control Agent. Once the the connection is established, it
 *       will process the commands from the Control Agent. For details, please
 *       reference the architecture documents.
 *
 */
#include <time.h>
#include <fcntl.h>
#include <sched.h>

#include "wfa_portall.h"
#include "wfa_debug.h"
#include "wfa_main.h"
#include "wfa_types.h"
#include "wfa_dut.h"
#include "wfa_sock.h"
#include "wfa_tlv.h"
#include "wfa_tg.h"
#include "wfa_agt.h"
#include "wfa_rsp.h"
#include "wfa_wmmps.h"
#include "syslog.h"
//#include "wpa_helpers.h"

#include "hal_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "ping.h"
#include "task_def.h"
#include "hal_gpio.h"
#include "limits.h"  /* for ULONG_MAX */


#define DUT_INIT_TASK_PRI   (configMAX_PRIORITIES - 2)
#define DUT_INIT_STACK_SIZE (configMINIMAL_STACK_SIZE * 20) /* unit in word */
#define DUT_TG_STACK_SIZE   (configMINIMAL_STACK_SIZE * 40) /* unit in word */
#define DUT_UART_BAUD_RATE  HAL_UART_BAUDRATE_921600
#define DUT_UART_DMA_MODE
#define VFIFO_SIZE 2048
#define SEND_THRESHOLD_SIZE 64
#define RECEIVE_THRESHOLD_SIZE 512
#define DUT_TX_BIT 0x01
#define DUT_RX_BIT 0x02
#define DEBUG 1


static TaskHandle_t dut_task_handler = NULL;
#ifdef DUT_UART_DMA_MODE
static unsigned int g_vff_mem_addr = 0;
#endif
static BYTE *g_cmd_buf = NULL;
static BYTE *g_param_buf = NULL; 
extern int g_sigma_mode_max;


/* Global flags for synchronizing the TG functions */
int        gtimeOut = 0;        /* timeout value for select call in usec */

extern int isString(char *);
extern int isNumber(char *);
extern int isIpV4Addr(char *);


#ifdef WFA_WMM_PS_EXT
extern BOOL gtgWmmPS;
extern unsigned long psTxMsg[512];
extern unsigned long psRxMsg[512];
extern wfaWmmPS_t wmmps_info;
extern tgWMM_t    wmmps_mutex_info;
extern int  psSockfd;
extern struct apts_msg *apts_msgs;

extern void BUILD_APTS_MSG(int msg, unsigned long *txbuf);
extern int wfaWmmPowerSaveProcess(int sockfd);
extern void wfaSetDUTPwrMgmt(int);
extern void wfaTGSetPrio(int, int);
#endif /* WFA_WMM_PS_EXT */

extern     int adj_latency;           /* adjust sleep time due to latency */
char       gnetIf[WFA_BUFF_32];        /* specify the interface to use */

extern BYTE   *trafficBuf, *respBuf;

/* stream table */
extern tgStream_t gStreams[];         /* streams' buffers             */

/* the agent local Socket, Agent Control socket and baseline test socket*/
extern int btSockfd;


/* the WMM traffic streams socket fds - Socket Handler table */
extern int tgSockfds[];

extern     xcCommandFuncPtr gWfaCmdFuncTbl[]; /* command process functions */
extern     char gCmdStr[];
extern     tgStream_t *findStreamProfile(int);
extern     int clock_drift_ps;

extern dutCmdResponse_t gGenericResp;

extern char *sigma_mode_tbl[];
extern int sigma_mode;


/* Debug message flags */
unsigned short wfa_defined_debug = WFA_DEBUG_ERR | WFA_DEBUG_WARNING | WFA_DEBUG_INFO;
unsigned short dfd_lvl = WFA_DEBUG_DEFAULT | WFA_DEBUG_ERR | WFA_DEBUG_INFO;

/*
 * Thread Synchronize flags
 */
extern tgWMM_t wmm_thr[WFA_THREADS_NUM];

extern void *wfa_wmm_thread(void *thr_param);
extern void *wfa_wmmps_thread();

extern int shell(const char *fmt, ...);


extern double gtgPktRTDelay;

int gxcSockfd = -1;

extern int wfa_estimate_timer_latency();
extern void wfa_dut_init(BYTE **tBuf, BYTE **rBuf, BYTE **paBuf, BYTE **cBuf, struct timeval **timerp);

extern int set_sigma_mode(int mode);

#ifdef _FREERTOS
bool SendDone = 0;
void dut_get_cmd(void)
{
	uint32_t nbytes = 0;
	int cmdLen = 0, respLen = 0, i = 0;
	uint32_t write_cnt = 0;
	int read_cnt = 0;
	WORD     xcCmdTag;
	wfaTLV *tlv_data = NULL;

	nbytes = hal_uart_receive_dma(DUT_CMD_UART_PORT, g_cmd_buf, 4);
	if(nbytes > 0) {
		printf("Receive header: %ld\n", nbytes);
#if DEBUG
		for (i = 0; i< nbytes; i++)
			printf("%x ", g_cmd_buf[i]);
		printf("\n");
#endif		
		tlv_data = (wfaTLV *)g_cmd_buf;
		cmdLen = tlv_data->len;
		if (cmdLen >= WFA_BUFF_4K) {
			printf("cmdLen: %d, buffer len: %d, overflow!!!", cmdLen, WFA_BUFF_4K);
			goto cmd_end;
		}
		nbytes = 0;
		read_cnt = 0;
		while (cmdLen > 0 && nbytes < cmdLen) {
			read_cnt = hal_uart_receive_dma(DUT_CMD_UART_PORT, g_cmd_buf + 4 + nbytes, cmdLen - nbytes);
			if(read_cnt < 0) {
				printf("Receive data failed\n");
				return;
			}
			nbytes += read_cnt;
#if DEBUG
			if (read_cnt > 0) {
				printf("Receive data: %ld\n", nbytes);
				for (i = read_cnt; i< nbytes; i++)
					printf("%x ", g_cmd_buf[4 + i]);
				printf("\n");
			}
#endif		
		}
		/* command received */
		wfaDecodeTLV(g_cmd_buf, nbytes, &xcCmdTag, &cmdLen, g_param_buf);
		memset(respBuf, 0, WFA_RESP_BUF_SZ);
		respLen = 0;

		/* reset two commond storages used by control functions */
		memset(gCmdStr, 0, WFA_CMD_STR_SZ);
		memset(&gGenericResp, 0, sizeof(dutCmdResponse_t));

		/* command process function defined in wfa_ca.c and wfa_tg.c */
		if(xcCmdTag != 0 && gWfaCmdFuncTbl[xcCmdTag] != NULL)
		{
			/* since the new commands are expanded to new block */
			gWfaCmdFuncTbl[xcCmdTag](cmdLen, g_param_buf, &respLen, (BYTE *)respBuf);
		}
		else
		{
			// no command defined
			gWfaCmdFuncTbl[0](cmdLen, g_param_buf, &respLen, (BYTE *)respBuf);
		}

		write_cnt = 0;
		nbytes = 0;
		while((write_cnt < respLen) && (SendDone == 0)) {
			nbytes = hal_uart_send_dma(DUT_CMD_UART_PORT, respBuf + write_cnt, respLen - write_cnt);
			write_cnt += nbytes;
			if(write_cnt != respLen) {
				DPRINT_WARNING(WFA_WNG, "wfa-dut main:Send response returned value %ld != respLen %d\n", write_cnt, respLen);
			} else {
#if DEBUG
				printf("Send response: %ld\n", write_cnt);
				for(i = 0; i< write_cnt; i++)
					printf("%x ", respBuf[i]);
				printf("\n");
#endif
				SendDone = 1;
			}
		}	
	}
cmd_end:
	return;
}

void start_dut(void)
{
	int i;
	uint32_t ulNotifyValue = 0;
	BaseType_t xResult;

	printf("[DUT] in %s\r\n", __func__);
	for (;;) {
		xResult = xTaskNotifyWait(pdFALSE, ULONG_MAX, &ulNotifyValue, portMAX_DELAY);
		if (xResult == pdPASS) {
			if ((ulNotifyValue & DUT_RX_BIT) != 0) {
				SendDone = 0;
				dut_get_cmd();
			}

			if (SendDone && ((ulNotifyValue & DUT_TX_BIT) != 0)) {
				printf("Send response success\n");
				memset(g_cmd_buf, 0, WFA_CMD_STR_SZ);
				memset(respBuf, 0, WFA_RESP_BUF_SZ);
			}
#if 0
			if ((ulNotifyValue & ATED_TX_BIT) != 0 || g_ated_status == ATED_STATUS_SEND_RESP
#ifdef MTK_SLT_ENABLE
					|| g_ated_status == ATED_STATUS_SEND_CMD
#endif
			   ) {

#ifdef MTK_SLT_ENABLE
				if (g_ated_status == ATED_STATUS_SEND_RESP)
					ated_send_resp(pbuf);
				if (g_ated_status == ATED_STATUS_SEND_CMD)
					ated_send_cmd(pbuf);
#else
				ated_send_resp(pbuf);
#endif

				ated_reset(pbuf);
			}
#endif

		}
	}

	/* Close sockets */
	wCLOSE(gxcSockfd);
	wCLOSE(btSockfd);

	for(i = 0; i< WFA_THREADS_NUM; i++)
		vTaskDelete(wmm_thr[i].task_id);

	for(i = 0; i< WFA_MAX_TRAFFIC_STREAMS; i++)
	{
		if(tgSockfds[i] != -1)
		{
			wCLOSE(tgSockfds[i]);
			tgSockfds[i] = -1;
		}
	}

	return;
}

static void user_uart_callback(hal_uart_callback_event_t status, void *user_data)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	if (dut_task_handler == NULL){
		printf("[DUT] %s: ERROR!! handler is NULL.\r\n", __func__);
		return;
	}

	if(status == HAL_UART_EVENT_READY_TO_WRITE)
	{
		xTaskNotifyFromISR(dut_task_handler, DUT_TX_BIT, eSetBits, &xHigherPriorityTaskWoken);
	}
	else if(status == HAL_UART_EVENT_READY_TO_READ)
	{
		xTaskNotifyFromISR(dut_task_handler, DUT_RX_BIT, eSetBits, &xHigherPriorityTaskWoken);
	}
	return;
}


int dut_mt7933_uart_init(void)
{
	hal_uart_config_t uart_config;
#ifdef DUT_UART_DMA_MODE
	hal_uart_status_t status_t;
	hal_uart_dma_config_t dma_config;
#endif
	int ret = 0;

	uart_config.baudrate = DUT_UART_BAUD_RATE;
	uart_config.parity = HAL_UART_PARITY_NONE;
	uart_config.stop_bit = HAL_UART_STOP_BIT_1;
	uart_config.word_length = HAL_UART_WORD_LENGTH_8;

	ret = hal_uart_init(DUT_CMD_UART_PORT, &uart_config);

	if (ret != 0)
	{
		printf("[DUT] uart init fail!!\r\n");
		return ret;
	}

#ifdef DUT_UART_DMA_MODE
	g_vff_mem_addr = (unsigned int)pvPortMallocNC(VFIFO_SIZE * 2);

	if (g_vff_mem_addr == 0) {
		DPRINT_ERR(WFA_ERR,"%s pvPortMallocNC error %d (size = %d)", __func__, g_vff_mem_addr, (VFIFO_SIZE * 2));
		vPortFreeNC((unsigned int *)g_vff_mem_addr);
		return HAL_UART_STATUS_ERROR;
	}

	/*Step2: Configure UART port to dma mode. */
	dma_config.receive_vfifo_buffer = (uint32_t)g_vff_mem_addr;
	dma_config.receive_vfifo_buffer_size = VFIFO_SIZE;
	dma_config.receive_vfifo_threshold_size = RECEIVE_THRESHOLD_SIZE;
	dma_config.send_vfifo_buffer = (uint32_t)g_vff_mem_addr + VFIFO_SIZE;
	dma_config.send_vfifo_buffer_size = VFIFO_SIZE;
	dma_config.send_vfifo_threshold_size = SEND_THRESHOLD_SIZE;

	status_t = hal_uart_set_dma(DUT_CMD_UART_PORT, &dma_config);
	if (status_t) {
		DPRINT_ERR(WFA_ERR,"%s hal_uart_set_dma error %d", __func__, status_t);
		vPortFreeNC((unsigned int *)g_vff_mem_addr);
		g_vff_mem_addr = 0;
		return status_t;
	}
	DPRINT_ERR(WFA_ERR,"%s: use dma mode (port = %d) VFIFO_SIZE = %d", __func__, DUT_CMD_UART_PORT, VFIFO_SIZE);
	DPRINT_ERR(WFA_ERR,"%s: dma rx_th = %d, tx_th =%d", __func__, RECEIVE_THRESHOLD_SIZE, SEND_THRESHOLD_SIZE);
#endif /* end of DUT_UART_DMA_MODE */

	hal_uart_register_callback(DUT_CMD_UART_PORT, user_uart_callback, NULL);
	DPRINT_ERR(WFA_ERR,"[DUT] uart init success\n");

	return 0;
}


static void dut_init(void *data)
{
        int ret = 0;

	printf("[DUT] dut init start\r\n");
	ret = dut_mt7933_uart_init();

	if(ret == 0)
		printf("[DUT] uart init done\r\n");
	else {
		printf("[DUT] uart init fail\r\n");
		return;
	}

#if 0
	ret = dut_init_param();

	if(ret == 0)
		printf("[DUT] buffer init done\r\n");
	else {
		printf("[DUT] buffer fail\r\n");
		return;
	}
#endif
	start_dut();

	printf("[DUT] dut init success\r\n");
	/*
	 * necessarily free all mallocs for flat memory real-time systems
	 */
	wFREE(trafficBuf);
	wFREE(respBuf);
	wFREE(g_cmd_buf);
	wFREE(g_param_buf);

	vTaskDelete(NULL);

}


BaseType_t dut_init_task(void)
{
	BaseType_t ret = pdFALSE;

	printf("[DUT] dut task create\r\n");

	ret = xTaskCreate(dut_init, "wfa_dut_task"
			, DUT_INIT_STACK_SIZE, NULL
			, DUT_INIT_TASK_PRI, &dut_task_handler);
	if (ret != pdPASS)
	{
		printf("[DUT] create init task failed\r\n");
		return ret;
	}

	printf("[DUT] dut task create success\r\n");
	return ret;
}

int
dut_main(int argc, char **argv)
{
    BYTE      *xcCmdBuf=NULL, *parmsVal=NULL;
    struct timeval *toutvalp=NULL, *tovalp; /* Timeout for select()           */
    int i = 0, count = g_sigma_mode_max;
    BaseType_t ret = pdFALSE;
    char *p;

    DPRINT_ERR(WFA_ERR, "count = %d \n", count);

    if (argc < 1)              /* Test for correct number of arguments */
    {
        DPRINT_ERR(WFA_ERR, "Usage:  wfa_dut <sigma mode> \n");
        DPRINT_ERR(WFA_ERR, "sigma mode: \n");

        for (i = 0; i < count; i++) {
            DPRINT_ERR(WFA_ERR, "%d: %s \n", i+1, sigma_mode_tbl[i]);
        }

        return 0;
    }

    if(isNumber(argv[0]) == WFA_FAILURE)
    {
        DPRINT_ERR(WFA_ERR, "incorrect sigma_mode\n");
        return 0;
    }

    sigma_mode = (int)strtol(argv[0], &p, 10);
    if (errno != 0 || *p != '\0' || sigma_mode > INT_MAX || sigma_mode < INT_MIN) {
	DPRINT_ERR(WFA_OUT, "error sigma_mode\n");
	return 0;
    }
    else if (sigma_mode > count) {
	    DPRINT_ERR(WFA_OUT, "sigma_mode: %d is not supported!\n", sigma_mode);
            return 0;
    }

#if 0
    /* Test shell cmd */
    DPRINT_ERR(WFA_ERR, "####### Test iwpriv wlan0 driver ver #######\n");
    shell(IWPRIV" wlan0 driver ver");
#endif

    set_sigma_mode(sigma_mode - 1);


    /* allocate the traffic stream table */
    wfa_dut_init(&trafficBuf, &respBuf, &parmsVal, &xcCmdBuf, &toutvalp);
    g_cmd_buf = xcCmdBuf;
    g_param_buf = parmsVal;

    /*
     * Create multiple tasks for WMM Stream processing.
     */
    for(i = 0; i< WFA_THREADS_NUM; i++)
    {
        wmm_thr[i].thr_flag_wait = xEventGroupCreate();
        xTaskCreate(wfa_wmm_thread, "wmm_task", DUT_TG_STACK_SIZE,
            (void *)i, DUT_TG_TASK_PRI, &wmm_thr[i].task_id);
    }

    for(i = 0; i < WFA_MAX_TRAFFIC_STREAMS; i++)
        tgSockfds[i] = -1;

    ret = dut_init_task();
    if (ret == pdFALSE) {
        DPRINT_ERR(WFA_ERR, "Initialize dut task failed!\n");
        return 0;
    }

    adj_latency = wfa_estimate_timer_latency() + 4000; /* four more mini */

    if(adj_latency > 500000)
    {
        printf("****************** WARNING  **********************\n");
        printf("!!!THE SLEEP TIMER LATENCY IS TOO HIGH!!!!!!!!!!!!\n");
        printf("**************************************************\n");

        /* Just set it to  500 mini seconds */
        adj_latency = 500000;
    }

#if 0
    /* Test wpa_cli cmd */
    DPRINT_ERR(WFA_ERR, "####### Test wpa_cli list_network #######\n");
    wpa_command(gnetIf, "list_network");
#endif

    /*
     * necessarily free all mallocs for flat memory real-time systems
     */
    wFREE(toutvalp);

    return 0;
}
#endif
