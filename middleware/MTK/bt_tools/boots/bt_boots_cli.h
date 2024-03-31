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

#ifndef __BT_BOOTS_CLI_H_
#define __BT_BOOTS_CLI_H_

#include <cli.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t btpriv_cli(uint8_t len, char *param[]);
#define BTCMD_CLI_ENTRY	  { "btpriv", "bt command", btpriv_cli, NULL }, //Command

#ifdef __cplusplus
}
#endif



#endif /*__BT_BOOTS_CLI_H_*/

