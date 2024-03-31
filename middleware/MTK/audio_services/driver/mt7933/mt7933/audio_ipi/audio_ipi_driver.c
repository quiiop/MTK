/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifdef CFG_IPC_SUPPORT
#include "adsp_ipi_queue.h"
#include "adsp_ipi.h"
#endif /* #ifdef CFG_IPC_SUPPORT */

#include "audio_task_manager.h"
#include "audio_messenger_ipi.h"
#include "audio_ipi_platform.h"
#include "audio_ipi_driver.h"
#include "aud_log.h"
#include <stddef.h>

/*
 * =============================================================================
 *                     implementation
 * =============================================================================
 */

static int send_func(void *buf, uint32_t len, uint32_t wait)
{
    int ret = 0;
#ifdef CFG_IPC_SUPPORT
    ret = adsp_ipi_send(IPI_AUDIO, buf, len, wait, IPI_AP2ADSP);
#endif /* #ifdef CFG_IPC_SUPPORT */

    return ret;
}

void audio_ipi_platform_init(void)
{
    struct ipi_send_info_t info;
#ifdef CFG_IPC_SUPPORT
    info.ipi_id = IPI_AUDIO;
    info.ipi_dir = IPI_AP2ADSP;
#endif /* #ifdef CFG_IPC_SUPPORT */
    info.ipi_send_func = send_func;
    aud_set_ipi_info(&info);
}

void audio_ipi_platform_uninit(void)
{
    struct ipi_send_info_t *info = aud_get_ipi_info();

    if (info != NULL) {
        info->ipi_id = 0;
        info->ipi_dir = 0;
        info->ipi_send_func = NULL;
    }
}

int audio_ipi_receive_msg_init(void)
{
    int i = 0;
    int ret = 0;

    for (i = 0; i < TASK_SCENE_SIZE; i++)
        audio_des_recv_message(i);

#ifdef CFG_IPC_SUPPORT
    ret = adsp_ipi_registration(
              IPI_AUDIO,
              audio_ipi_msg_dispatcher,
              "audio");
    if (ret != 0)
        aud_msg("adsp_ipi_registration fail!!");
#endif /* #ifdef CFG_IPC_SUPPORT */

    return ret;
}

#ifdef CFG_IPC_SUPPORT
extern ipi_status adsp_ipi_unregistration(uint32_t id);
#endif /* #ifdef CFG_IPC_SUPPORT */

int audio_ipi_receive_msg_uninit(void)
{
    int i = 0;
    int ret = 0;

    for (i = 0; i < TASK_SCENE_SIZE; i++)
        audio_des_recv_message(i);

#ifdef CFG_IPC_SUPPORT
    ret = adsp_ipi_unregistration(IPI_AUDIO);
    if (ret != 0)
        aud_msg("adsp_ipi_unregistration fail!!");
#endif /* #ifdef CFG_IPC_SUPPORT */

    return ret;
}

int audio_ipi_driver_init(void)
{
    int ret = 0;

#ifdef CFG_IPC_SUPPORT
    scp_ipi_queue_init(AUDIO_OPENDSP_USE_HIFI4);
#endif /* #ifdef CFG_IPC_SUPPORT */
    audio_task_manager_init();
    audio_ipi_receive_msg_init();
    audio_ipi_platform_init();

    return ret;
}

void audio_ipi_driver_exit(void)
{
    audio_task_manager_deinit();
    audio_ipi_receive_msg_uninit();
    audio_ipi_platform_uninit();
}

