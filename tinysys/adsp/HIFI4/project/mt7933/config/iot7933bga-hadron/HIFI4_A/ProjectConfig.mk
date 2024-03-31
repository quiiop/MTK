# This project is created only for generating ADSP default image. Project-based
# configuration here should be minimized, because one ADSP default image
# project belongs to a specific platform.

CFG_MTK_AUDIODSP_SUPPORT := yes

###################################################################
# ADSP internal feature options
###################################################################
CFG_AUDIO_SUPPORT = yes
CFG_FPGA = no
CFG_HIFI4_A = yes
CFG_HIFI4_DUAL_CORE = no
CFG_ASSERT_SUPPORT = yes
CFG_GPIO_SUPPORT = no
CFG_AUXADC_SUPPORT = no
CFG_XGPT_SUPPORT = no
CFG_SYSTIMER_SUPPORT = yes
CFG_CLK_PM_SUPPORT = no
CFG_I2C_SUPPORT = no
CFG_PMIC_SUPPORT = no
CFG_PWM_SUPPORT = no
CFG_EINT_SUPPORT = no
CFG_UART_SUPPORT = yes
CFG_TRAX_SUPPORT = no
CFG_TASK_MONITOR = no
# CFG_MTK_APUART_SUPPORT
# Do not use this with eng load or log may mix together and hard to recognzie
# Do not use this on lower power, it keeps infra always on
CFG_CACHE_SUPPORT = no
CFG_IPC_SUPPORT = yes
CFG_ONCHIP_IPI_SUPPORT = no
CFG_PRINT_TIMESTAMP = yes
CFG_LOGGER_SUPPORT = yes
CFG_WDT_SUPPORT = yes
CFG_DMA_SUPPORT = no
CFG_HEAP_GUARD_SUPPORT = no
CFG_VCORE_DVFS_SUPPORT = no
CFG_RAMDUMP_SUPPORT = no
CFG_FREERTOS_TRACE_SUPPORT = no
CFG_IRQ_MONITOR_SUPPORT = no
CFG_IPI_STAMP_SUPPORT = no
CFG_AP_AWAKE_SUPPORT = no
CFG_WAKELOCK_SUPPORT = no
CFG_CLI_SUPPORT = yes
CFG_POSIX_SUPPORT = yes
CFG_HW_SEMA_SUPPORT = yes
CFG_DSP_SEND_IRQ_SUPPORT = no
CFG_PWM_BUCK_SUPPORT = no
CFG_LIB_RENAME_SECTION_TO_DRAM = no
CFG_TICKLESS_SUPPORT = yes
CFG_SPM_SUPPORT = yes
CFG_DSP_CLK_SUPPORT = yes
CFG_MTK_PDCT_SUPPORT = yes
CFG_MCPS_CALC_SUPPORT = yes
CFG_VA_WW_MCPS_CALC = yes
CFG_VA_VAD_MCPS_CALC = yes
CFG_VA_PREPROC_MCPS_CALC = yes
CFG_MTK_MALLOC_DEBUG = no
CFG_DSP_AP_IRQ_SUPPORT = yes

###################################################################
# HIFI4_A address layout
# SRAM total: 256KB + 48KB = 0x4C000
###################################################################
CFG_HIFI4_SRAM_ADDRESS = 0xA0000000
CFG_HIFI4_SRAM_SIZE    = 0x0004B800
CFG_HIFI4_SRAM_NONCACHE_ADDRESS = 0xA004B800
CFG_HIFI4_SRAM_NONCACHE_SIZE    = 0x00000800
CFG_HIFI4_DRAM_ADDRESS = 0xA0050000
CFG_HIFI4_DRAM_SIZE    = 0x00FC000
CFG_HIFI4_DRAM_RESERVE_CACHE_START = 0xA014C000
CFG_HIFI4_DRAM_RESERVE_CACHE_SIZE  = 0xD4000
CFG_HIFI4_DRAM_SHARED_NONCACHE_START = 0xA0220000
CFG_HIFI4_DRAM_SHARED_NONCACHE_SIZE  = 0x0004A000
# dram-cpu-view is uncertain
CFG_HIFI4_SRAM_CPU_VIEW = 0x40040000
CFG_HIFI4_DRAM_DSP_VIEW = 0xA0050001
CFG_HIFI4_BOOTUP_ADDR_DSP_VIEW = 0xA0000000

