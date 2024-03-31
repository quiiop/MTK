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

#ifndef __AUDIO_IPI_PLATFORM_H__
#define __AUDIO_IPI_PLATFORM_H__

#include <stdint.h>
#include <stdbool.h>

enum opendsp_id {
    AUDIO_OPENDSP_USE_CM4_A, /* => SCP_A_ID */
    AUDIO_OPENDSP_USE_CM4_B, /* => SCP_B_ID */
    AUDIO_OPENDSP_USE_HIFI3, /* => ADSP_A_ID */
    AUDIO_OPENDSP_USE_HIFI4,
    NUM_OPENDSP_TYPE,
    AUDIO_OPENDSP_ID_INVALID
};

struct ipi_send_info_t {
    uint32_t ipi_id;
    uint32_t ipi_dir;
    int (*ipi_send_func)(void *buf, uint32_t len, uint32_t wait);
};

uint32_t audio_get_opendsp_id(const uint8_t task);
//uint32_t audio_get_ipi_id(const uint8_t task);

void aud_set_ipi_info(struct ipi_send_info_t *info);
struct ipi_send_info_t *aud_get_ipi_info(void);

#endif /* #ifndef __AUDIO_IPI_PLATFORM_H__ */

