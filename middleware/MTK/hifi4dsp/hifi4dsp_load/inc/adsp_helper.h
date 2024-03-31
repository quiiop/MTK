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
#ifndef __ADSP_HELPER_H__
#define __ADSP_HELPER_H__

#include "adsp_reg.h"
#include "FreeRTOS.h"
#include "hal_log.h"
#include <stdint.h>
#include "memory_map.h"

#define ADSP_DEBUG                 0
#define DSP_LOG_BUF_MAGIC          (0x67676F6C)

/*
 * Design const definition with specific platform.
 * SHARED_DTCM
 * HEAD|---logger(1KB)---+---IPC(1KB)---|TAIL
 */
#define SIZE_SHARED_DTCM_FOR_IPC      (0x0400)  /* 1KB */
#if 1 //DSP log in sram
#define SIZE_SHARED_DTCM_FOR_LOGGER   (0x0400)  /* 1KB */
#define TOTAL_SIZE_SHARED_DTCM_FROM_TAIL  \
        (SIZE_SHARED_DTCM_FOR_IPC + SIZE_SHARED_DTCM_FOR_LOGGER)
#else /* #if 1 //DSP log in sram */
#define SIZE_SHARED_DRAM_FOR_LOGGER   0x20000 /* 0x2_0000(128KB) */
#define TOTAL_SIZE_SHARED_DTCM_FROM_TAIL (SIZE_SHARED_DTCM_FOR_IPC)
#endif /* #if 1 //DSP log in sram */

#define TOTAL_SIZE_SHARED_DRAM_FROM_TAIL  (0x0004A000)  /* enlarged from ProjectConfig.mk */

#define DTCM_PHYS_BASE_FROM_DSP_VIEW  (0xA0000000) /* MT7933 SRAM base address from DSP view */
#define DRAM_PHYS_BASE_FROM_DSP_VIEW  (0xA0050000) /* MT7933 available PSRAM base address DSP view */

#if !DSP_LOAD_FROM_RTOS
#define DSP_ROM_BASE                  ((uint32_t)XIP_DSP_START)
#define DSP_ROM_SIZE                  ((uint32_t)DSP_LENGTH)         /*   64KB */
#endif /* #if !DSP_LOAD_FROM_RTOS */

/*
 * Defined in adsp_sys_ao_reg
 * CPU to DSP DRAM remap control Register, bit 31:12 valid,
 * need write offset = CPU Dram Address - DSP Dram Base Address
 */
#define DSP_EMI_BASE_ADDR    (SYSCFG_AO_BASE + 0x081C)

struct hifi4dsp_log_ctrl {
    uint32_t magic;
    uint32_t start;
    uint32_t size;
    uint32_t offset;
    uint32_t full;
    uint32_t last_print_to;
};

typedef unsigned long phys_addr_t;

#define ADSP_DRAM_PHYSICAL_BASE 0xA0058000 /* PSRAM for DSP */
#define ADSP_DRAM_SIZE          0x21A000 /* 2MB + 64KB + 40KB */


struct adsp_chip_info {
    phys_addr_t pa_itcm;
    phys_addr_t pa_dtcm;
    phys_addr_t pa_dram; /* adsp dram physical base */
    phys_addr_t pa_cfgreg;
    uint32_t itcmsize;
    uint32_t dtcmsize;
    uint32_t dramsize;
    uint32_t cfgregsize;
    void *va_itcm;
    void *va_dtcm; /* corresponding to pa_dtcm */
    void *va_dram; /* corresponding to pa_dram */
    void *va_cfgreg;
    void *shared_dtcm; /* part of  va_dtcm */
    void *shared_dram; /* part of  va_dram */
    phys_addr_t adsp_bootup_addr;
    int adsp_bootup_done;
};

/* adsp helper api */
extern int platform_parse_resource(void *data);
extern int adsp_shared_base_ioremap(void *data);
extern int adsp_wdt_device_init(void);
extern int adsp_wdt_device_remove(void);
extern int adsp_must_setting_early(void);
extern int adsp_clock_power_on(void);
extern int adsp_clock_power_off(void);
extern void hifixdsp_boot_sequence(uint32_t boot_addr);
extern void hifixdsp_release_sequence(void);
extern void hifixdsp_shutdown(void);
extern int adsp_misc_setting_after_poweron(void);
extern int adsp_remove_setting_after_shutdown(void);
extern int adsp_shutdown_notify_check(void);
extern void init_adsp_sysram_reserve_mblock(phys_addr_t pbase,
                                            void *vbase);
extern void *get_adsp_chip_data(void);
extern void get_adsp_firmware_size_addr(void **addr, size_t *size);
extern void *get_adsp_reg_base(void);
extern unsigned int is_from_supend;
extern void adsp_cli_register(void);
#endif /* #ifndef __ADSP_HELPER_H__ */
