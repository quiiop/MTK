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

//---------------------------------------------------------------------------
#include "stdint.h"
#include "picus.h"

//---------------------------------------------------------------------------
uint8_t bt_picus_cli(uint8_t len, char *param[])
{
    /* pointer to beginning of argv */
    picus_cmd_handler(len + 1, param - 1);
    return 0;
}

