/*******************************************************************************
 *                      P U B L I C   D A T A
 *******************************************************************************
 */

#ifndef __ATE_INIT_H__
#define	__ATE_INIT_H__

#include "FreeRTOS.h"
#include "cli.h"

#if defined(MTK_MINICLI_ENABLE)
extern cmd_t _ated_cli[];
#define ATED_CLI_ENTRY \
	{ "ated", "QA Tool relay", NULL, (cmd_t *)_ated_cli},
#endif

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define REQ_BUFFER_SIZE 2064

#if defined(MTK_SLT_ENABLE) && !defined(MTK_SLT_WIFI_TESTCODE_ENABLE)
#define MTK_SLT_WIFI 1
#else
#define MTK_SLT_WIFI 0
#endif

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
struct ated_data {
	uint8_t cmd_buf[REQ_BUFFER_SIZE];
	uint32_t has_read;
	uint32_t has_send;
};

enum ATED_STATUS {
	ATED_STATUS_IDLE,
	/* Rx */
	ATED_STATUS_RECEIVE_NOTICE,
	ATED_STATUS_SEND_NOTICE,
	ATED_STATUS_READ_CMD_HEADER,
	ATED_STATUS_READ_CMD_DATA,
	ATED_STATUS_DO_CMD,
	ATED_STATUS_SEND_RESP,

#if (MTK_SLT_WIFI == 1)
	/* Tx */
	ATED_STATUS_SEND_CMD,
	ATED_STATUS_READ_RESP,
#endif
};

/*******************************************************************************
 *                      F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
extern enum ATED_STATUS g_ated_status;
extern struct ated_data *g_ated_buffer;

uint8_t ated_on(uint8_t argc, char *argv[]);
uint8_t ated_off(uint8_t argc, char *argv[]);
uint8_t ated_set_trans_len(uint8_t argc, char *argv[]);
uint8_t ated_set_padding(uint8_t argc, char *argv[]);
uint8_t ated_exit_wifi_test_mode(uint8_t argc, char *argv[]);
uint8_t ated_test_write(uint8_t argc, char *argv[]);
uint8_t ated_set_baudrate(uint8_t argc, char *argv[]);

BaseType_t ated_init_task(void);
#if ( MTK_SLT_WIFI == 1)
uint8_t ated_send_cmd(struct ated_data* data);
uint8_t ated_get_cmd(struct ated_data* data);
uint8_t ated_reset(struct ated_data* data);
#endif

#endif
