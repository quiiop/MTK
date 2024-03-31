/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef ADSP_CLK_H
#define ADSP_CLK_H

extern int platform_parse_clock(void *data);
extern int adsp_default_clk_init(int enable);
extern int adsp_pm_register_early(void);
extern int adsp_power_enable(void);
extern void adsp_power_disable(void);

#endif /* #ifndef ADSP_CLK_H */
