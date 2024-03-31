/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include "adsp_reg.h"
#include "adsp_debug.h"
#include "adsp_helper.h"
#include "FreeRTOS.h"
#include "mtk_hifixdsp_common.h"
#include <string.h>
#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager_internal.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
#ifdef MTK_POSIX_SUPPORT_ENABLE
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#include "FreeRTOS_POSIX/unistd.h"
#endif /* #ifdef MTK_POSIX_SUPPORT_ENABLE */

#ifdef MTK_POSIX_SUPPORT_ENABLE
static pthread_t tid_print_dsp_log;
#endif /* #ifdef MTK_POSIX_SUPPORT_ENABLE */
static uint32_t inShutDown;

static unsigned char prvAdspPoweron(uint8_t len, char *param[])
{
    /* power on dsp */
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_AUDIO);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    async_load_hifixdsp_bin_and_run((void *)0, (void *)0);

    return 0;
}

static unsigned char prvAdspShutdown(uint8_t len, char *param[])
{
    /* shutdown dsp */
    inShutDown = 1;
    hifixdsp_stop_run();
    inShutDown = 0;

#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_AUDIO);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return 0;
}

int cancel_dsp_log_thread(void)
{
    /* stop thread: print dsp log periodly */
#ifdef MTK_POSIX_SUPPORT_ENABLE
    if (tid_print_dsp_log)
        return pthread_join(tid_print_dsp_log, NULL);
    else
#endif /* #ifdef MTK_POSIX_SUPPORT_ENABLE */
        return 0;
}

int dump_adsp_log_buf(void)
{
    uint32_t i;
    char tmp_buf[SIZE_SHARED_DTCM_FOR_LOGGER];
    char *log_str;
    struct adsp_chip_info *adsp = get_adsp_chip_data();
    uint32_t log_len;
    uint32_t tmp_log_start, tmp_log_size;
    struct hifi4dsp_log_ctrl *log_ctrl;

    if (adsp && adsp->adsp_bootup_done) {
        log_ctrl = (struct hifi4dsp_log_ctrl *)adsp->shared_dtcm; //dsp log in sram
        //log_ctrl = (struct hifi4dsp_log_ctrl *)adsp_get_reserve_sysram_virt(ADSP_LOGGER_BUF_MEM_ID);

        if (log_ctrl->magic != DSP_LOG_BUF_MAGIC) {
            log_hal_error("ADSP log buf is invalid!\n");
            return 0;
        }

        if (log_ctrl->full)
            log_len = log_ctrl->size;
        else
            log_len = log_ctrl->offset;

        log_str = (char *)adsp->shared_dtcm + sizeof(struct hifi4dsp_log_ctrl);

        //PRINTF_I("log_str addr: %p\n", log_str);
        //PRINTF_I("log_ctrl->magic: 0x%x\n", log_ctrl->magic);
        //PRINTF_I("log_ctrl->size: 0x%x\n", log_ctrl->size);

        log_hal_info("print log from buf: %p\n", log_ctrl);
        log_hal_info("==========================\n");
        log_hal_info("=== dump dsp log start ===\n");
        log_hal_info("==========================\n");

        if (log_ctrl->full && log_ctrl->offset) {
            for (i = tmp_log_start = log_ctrl->offset; i < log_ctrl->size; i++)
                if (log_str[i] == '\n' || i == log_len - 1) {
                    tmp_log_size = i - tmp_log_start + 1;
                    strncpy(tmp_buf, log_str + tmp_log_start, tmp_log_size);
                    tmp_buf[tmp_log_size] = '\0';
                    log_hal_info("%s", tmp_buf);
                    tmp_log_start = i + 1;
                }
            for (i = tmp_log_start = 0; i < log_ctrl->offset; i++)
                if (log_str[i] == '\n' || i == log_len - 1) {
                    tmp_log_size = i - tmp_log_start + 1;
                    strncpy(tmp_buf, log_str + tmp_log_start, tmp_log_size);
                    tmp_buf[tmp_log_size] = '\0';
                    log_hal_info("%s", tmp_buf);
                    tmp_log_start = i + 1;
                }

        } else {
            for (i = tmp_log_start = 0; i < log_len; i++) {
                if (log_str[i] == '\n' || i == log_len - 1) {
                    tmp_log_size = i - tmp_log_start + 1;
                    strncpy(tmp_buf, log_str + tmp_log_start, tmp_log_size);
                    tmp_buf[tmp_log_size] = '\0';
                    log_hal_info("%s", tmp_buf);
                    tmp_log_start = i + 1;
                }
            }
        }

        log_hal_info("==========================\n");
        log_hal_info("=== dump dsp log end  ====\n");
        log_hal_info("==========================\n");
    } else {
        log_hal_info("ADSP is not ready\n");
    }

    return 0;
}

#ifdef MTK_POSIX_SUPPORT_ENABLE
static void *print_dsp_log_periodly(void *arg)
{
    uint32_t i;
    char *log_str, *tmp_buf;
    uint32_t tmp_log_start, tmp_log_size;
    struct hifi4dsp_log_ctrl *log_ctrl;
    struct adsp_chip_info *adsp = get_adsp_chip_data();

    tmp_buf = (char *)pvPortMalloc(SIZE_SHARED_DTCM_FOR_LOGGER);
    if (tmp_buf == NULL) {
        log_hal_error("fail to malloc %d\n", SIZE_SHARED_DTCM_FOR_LOGGER);
        return NULL;
    }

    log_ctrl = (struct hifi4dsp_log_ctrl *)adsp->shared_dtcm;
    log_str = (char *)log_ctrl + sizeof(struct hifi4dsp_log_ctrl);

    while (1) {
        if (inShutDown)
            break;
        log_hal_info("print dsp log from %p\n", log_ctrl);

        if (log_ctrl->magic != DSP_LOG_BUF_MAGIC) {
            log_hal_error("ADSP log buf is invalid!\n");
            break;
        }

        if (log_ctrl->last_print_to < (log_ctrl->start + log_ctrl->offset)) {
            for (i = tmp_log_start = log_ctrl->last_print_to - log_ctrl->start; i < log_ctrl->offset; i++) {
                if (log_str[i] == '\n' || i == log_ctrl->offset - 1) {
                    tmp_log_size = i - tmp_log_start + 1;
                    strncpy(tmp_buf, log_str + tmp_log_start, tmp_log_size);
                    tmp_buf[tmp_log_size] = '\0';
                    log_hal_info("%s", tmp_buf);
                    tmp_log_start = i + 1;
                }
            }
        } else {
            for (i = tmp_log_start = log_ctrl->last_print_to - log_ctrl->start; i < log_ctrl->size; i++)
                if (log_str[i] == '\n') {
                    tmp_log_size = i - tmp_log_start + 1;
                    strncpy(tmp_buf, log_str + tmp_log_start, tmp_log_size);
                    tmp_buf[tmp_log_size] = '\0';
                    log_hal_info("%s", tmp_buf);
                    tmp_log_start = i + 1;
                }
            for (i = tmp_log_start = 0; i < log_ctrl->offset; i++)
                if (log_str[i] == '\n' || i == log_ctrl->offset - 1) {
                    tmp_log_size = i - tmp_log_start + 1;
                    strncpy(tmp_buf, log_str + tmp_log_start, tmp_log_size);
                    tmp_buf[tmp_log_size] = '\0';
                    log_hal_info("%s", tmp_buf);
                    tmp_log_start = i + 1;
                }
        }
        log_ctrl->last_print_to = log_ctrl->start + log_ctrl->offset;
        sleep(20);
    }

    vPortFree(tmp_buf);

    return 0;
}
#endif /* #ifdef MTK_POSIX_SUPPORT_ENABLE */

static unsigned char prvDumpAdspLog(uint8_t len, char *param[])
{
    dump_adsp_log_buf();

    return 0;
}

static unsigned char prvPrintAdspLog(uint8_t len, char *param[])
{
#ifdef MTK_POSIX_SUPPORT_ENABLE
    struct adsp_chip_info *adsp = get_adsp_chip_data();

    /* create thread: print dsp log periodly */
    if (!tid_print_dsp_log) { //in case "adsp_log_to_ap" is triggered more than once
        if (adsp && adsp->adsp_bootup_done) {
            pthread_create(&tid_print_dsp_log, NULL, print_dsp_log_periodly, NULL);
        } else {
            log_hal_info("ADSP is not ready\n");
        }
    } else {
        log_hal_info("cli adsp_log_to_ap is triggered more than once\n");
    }
#else /* #ifdef MTK_POSIX_SUPPORT_ENABLE */
    log_hal_info("Not support cmd\n");
#endif /* #ifdef MTK_POSIX_SUPPORT_ENABLE */
    return 0;
}

static unsigned char prvGetAdspVer(uint8_t len, char *params[])
{
    printf("HIFI4DSP Firmware version : %s\r\n", HIFI4DSP_FW_VERSION);
    return 0;
}

cmd_t adsp_cli_cmd[] = {
    {"adsp_poweron", "Adsp Poweron Test", prvAdspPoweron, NULL},
    {"adsp_shutdown", "Adsp Shutdown Test", prvAdspShutdown, NULL},
    {"adsp_log", "dump dsp log", prvDumpAdspLog, NULL},
    {"adsp_log_to_ap", "print dsp log to AP", prvPrintAdspLog, NULL},
    {"adsp_ver", "Get HIFI4DSP firmware version", prvGetAdspVer, NULL},
    { NULL, NULL, NULL, NULL }
};
