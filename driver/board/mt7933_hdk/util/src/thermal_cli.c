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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "toi.h"
#include "hal_spm.h"
#include "hal_nvic.h"
#include "hal_top_thermal.h"
#include "thermal_cli.h"
#include "mt7933_pos.h"
#include "hal_log.h"

#include "errno.h"
#define ULONG_MAX 0xFFFFFFFFUL

/*#define SUBSYS_ATOP_ID  (0x5)
#define SUBSYS_WF0_ID   (0x1)
#define SUBSYS_WF1_ID   (0x0)
#define SUBSYS_BT_ID    (0x2)*/

#if defined(HAL_TOP_THERMAL_MODULE_ENABLED)
char target_table[6][20] = {
    "SUBSYS_WF1_ID \0",
    "SUBSYS_WF0_ID \0",
    "SUBSYS_BT_ID \0",
    "\0",
    "\0",
    "SUBSYS_ATOP_ID \0",
};


int strtoul_16hex(uint32_t *val, char *param)
{

    *val = strtoul(param, NULL, 16);

    if (*val == ULONG_MAX && errno == ERANGE) {
        errno = 0;
        return ERANGE;
    } else if (*val == 0 && errno == EINVAL) {
        errno = 0;
        return EINVAL;
    }
    return 0;

}

static int thermal_connInfra_on(void)
{

    if (!MTCMOS_IS_ON(MTCMOS_STATE_INFRA_ON)) {
        cli_puts("Power on Con_infra AON\r\n");
        hal_spm_conninfra_on();
        mt7933_conninfra_init();
        return 1;
    } else {
        return 0;
    }
}

static unsigned char cli_rfspi_write(uint8_t len, char *param[])
{

    bool infra_wakeup = false;
    int ret =  0;
    uint32_t addr = 0;
    uint32_t val = 0;
    int conn_infra_on = 0;

    if (len !=  3) {
        cli_puts("Parameter as [Target] [addr] [val]\r\n");
        return 0;
    }
    uint32_t target     = atoi(param[0]);

    ret = strtoul_16hex(&addr, param[1]);
    if (ret != 0) {
        cli_puts(" Invaild addr \r\n");
        return 0;
    }

    ret = strtoul_16hex(&val, param[2]);
    if (ret != 0) {
        cli_puts(" Invaild Input Val  \r\n");
        return 0;
    }

    if (target == 3 || target == 4 || target > 5) {
        cli_puts("Invalid Target! \r\n");
        return 0;
    }

    printf("RFSPI Target %s \r\n", target_table[target]);
    printf("RFSPI Write addr : 0x%08lx \r\n", addr);
    printf("RFSPI Write val : 0x%08lx \r\n", val);

    if (!MTCMOS_IS_ON(MTCMOS_STATE_INFRA_ON)) {
        cli_puts("Power on Con_infra AON\r\n");
        hal_spm_conninfra_pos();
        conn_infra_on = 1;
    }

    infra_wakeup = hal_spm_conninfra_is_wakeup();
    if (!infra_wakeup)
        hal_spm_conninfra_wakeup();

    topspi_write(addr, val, target);


    if (!infra_wakeup)
        hal_spm_conninfra_sleep();

    if (conn_infra_on) {
        hal_spm_conninfra_off();
    }

    return 0;
}

static unsigned char cli_rfspi_read(uint8_t len, char *param[])
{

    bool infra_wakeup = false;
    uint32_t value  = 0;
    uint32_t addr = 0;
    int ret = 0;
    int conn_infra_on = 0;
    if (len !=  2) {
        cli_puts("Parameter as [Target] [addr] \r\n");
        return 0;
    }

    uint32_t target     = atoi(param[0]);
    ret = strtoul_16hex(&addr, param[1]);
    if (ret != 0) {
        cli_puts(" Invaild addr \r\n");
        return 0;
    }

    if (target == 3 || target == 4 || target > 5) {
        cli_puts("Invalid Target! \r\n");
        return 0;
    }

    printf("RFSPI Target %s \r\n", target_table[target]);
    printf("RFSPI Read addr : 0x%08lx \r\n", addr);

    if (!MTCMOS_IS_ON(MTCMOS_STATE_INFRA_ON)) {
        cli_puts("Power on Con_infra AON\r\n");
        hal_spm_conninfra_pos();
        conn_infra_on = 1;
    }

    infra_wakeup = hal_spm_conninfra_is_wakeup();
    if (!infra_wakeup)
        hal_spm_conninfra_wakeup();

    value = topspi_read(addr, target);
    printf("Read addr : 0x%04lx \r\n", value);

    if (!infra_wakeup)
        hal_spm_conninfra_sleep();

    if (conn_infra_on) {
        hal_spm_conninfra_off();
    }

    return 0;
}

static unsigned char cli_get_temperature(uint8_t len, char *param[])
{
    int temp;
    int conn_infra_on = 0;
    int ret = 0;
    if (len !=  0) {
        cli_puts("Parameter No effect \r\n");
    }

    conn_infra_on = thermal_connInfra_on();

    ret = connsys_thermal_query(&temp);
    if (ret == 0) {
        log_hal_info("Temperature : %d", temp);
    }

    if (conn_infra_on)
        hal_spm_conninfra_off();
    return 0;
}

static unsigned char cli_thermal_adie_init(uint8_t len, char *param[])
{
    bool infra_wakeup = false;
    int conn_infra_on = 0;
    if (len != 0) {
        return 0;
    }

    conn_infra_on = thermal_connInfra_on();
    infra_wakeup = hal_spm_conninfra_is_wakeup();
    if (!infra_wakeup)
        hal_spm_conninfra_wakeup();

    if (adie_thermal_init()) {
        printf("adie init fail \r\n");
    }

    if (!infra_wakeup)
        hal_spm_conninfra_sleep();

    if (conn_infra_on)
        hal_spm_conninfra_off();
    return 0;
}

static unsigned char cli_update_formula_params(uint8_t len, char *param[])
{
    bool infra_wakeup = false;
    int conn_infra_on = 0;
    if (len != 0) {
        return 0;
    }

    conn_infra_on = thermal_connInfra_on();
    infra_wakeup = hal_spm_conninfra_is_wakeup();
    if (!infra_wakeup)
        hal_spm_conninfra_wakeup();

    if (update_formula_value_by_efuse()) {
        printf("efuse update value fail \r\n");
    }

    if (!infra_wakeup)
        hal_spm_conninfra_sleep();

    if (conn_infra_on)
        hal_spm_conninfra_off();

    return 0;
}
#endif /* #if defined(HAL_TOP_THERMAL_MODULE_ENABLED) */
cmd_t thermal_cli_cmds[] = {
#if defined(HAL_TOP_THERMAL_MODULE_ENABLED)
    { "rfspi_r", "connsys top spi read",  cli_rfspi_read, NULL },
    { "rfspi_w", "connsys top spi write", cli_rfspi_write, NULL },
    { "read_cel", "Get Celsius ", cli_get_temperature, NULL },
    { "init", "Adie thermal init ", cli_thermal_adie_init, NULL },
    { "formula_update", "update default formula params ", cli_update_formula_params, NULL },
    { NULL, NULL, NULL, NULL }
#endif /* #if defined(HAL_TOP_THERMAL_MODULE_ENABLED) */
};
