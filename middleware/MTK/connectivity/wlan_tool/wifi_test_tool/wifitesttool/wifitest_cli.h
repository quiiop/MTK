
/*******************************************************************************
 *                      P U B L I C   D A T A
 *******************************************************************************
 */

#ifndef __WIFITEST_CLI_H__
#define	__WIFITEST_CLI_H__

#include "FreeRTOS.h"
#include "cli.h"

#if defined(MTK_MINICLI_ENABLE)
#define WIFITEST_CLI_ENTRY \
	{ "wifitest", "Wifi Test Tool", wifitest_cli, NULL},
#endif

/*******************************************************************************
 *                      F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
uint8_t wifitest_cli(uint8_t len, char* param[]);

#endif
