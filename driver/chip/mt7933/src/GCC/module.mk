
MT7933_CHIP = driver/chip/mt7933
COMPONENT = driver/board/component

#CFLAGS   += $(FPUFLAGS) -DUSE_HAL_DRIVER
LIBS += $(SOURCE_DIR)/prebuilt/driver/chip/mt7933/libpdct.a

CFLAGS   += -I$(SOURCE_DIR)/$(MT7933_CHIP)/src/common/inc
C_FILES  += $(MT7933_CHIP)/src/common/mhal_osai.c
C_FILES  += $(MT7933_CHIP)/src/hal_sys.c

ifeq ($(MTK_HAL_ADC_MODULE_ENABLE), y)
CFLAGS   += -DHAL_ADC_MODULE_ENABLED
CFLAGS   += -I$(SOURCE_DIR)/$(MT7933_CHIP)/src/adc/inc

C_FILES  += $(MT7933_CHIP)/src/adc/hdl_adc.c
C_FILES  += $(MT7933_CHIP)/src/adc/mhal_adc.c
C_FILES  += $(MT7933_CHIP)/src/adc/hal_adc.c
endif

ifeq ($(MTK_HAL_GCPU_MODULE_ENABLE), y)
ifeq ($(MTK_HAL_AES_MODULE_ENABLE), y)
CFLAGS   += -DHAL_AES_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_aes.c
endif
endif

ifeq ($(MTK_HAL_CACHE_MODULE_ENABLE), y)
CFLAGS   += -DHAL_CACHE_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_cache.c
C_FILES  += $(MT7933_CHIP)/src/hal_cache_internal.c
endif

ifeq ($(MTK_HAL_CLOCK_MODULE_ENABLE), y)
CFLAGS   += -DHAL_CLOCK_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_clk.c
endif

ifeq ($(MTK_HAL_GCPU_MODULE_ENABLE), y)
ifeq ($(MTK_HAL_DES_MODULE_ENABLE), y)
CFLAGS   += -DHAL_DES_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_des.c
endif
endif

ifeq ($(MTK_HAL_EINT_MODULE_ENABLE), y)
CFLAGS   += -DHAL_EINT_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/eint/hal_eint.c
C_FILES  += $(MT7933_CHIP)/src/eint/mhal_eint.c
C_FILES  += $(MT7933_CHIP)/src/eint/hdl_eint.c
endif

ifeq ($(MTK_HAL_FLASH_MODULE_ENABLE), y)
CFLAGS   += -DHAL_FLASH_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_flash.c
endif

ifeq ($(MTK_HAL_GDMA_MODULE_ENABLE), y)
CFLAGS   += -DHAL_GDMA_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_gdma.c
C_FILES  += $(MT7933_CHIP)/src/hal_gdma_internal.c
endif

ifeq ($(MTK_HAL_GPIO_MODULE_ENABLE), y)
CFLAGS   += -DHAL_GPIO_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_gpio.c
endif

ifeq ($(MTK_HAL_GPT_MODULE_ENABLE), y)
CFLAGS   += -DHAL_GPT_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_gpt.c
C_FILES  += $(MT7933_CHIP)/src/hal_gpt_internal.c
endif

ifeq ($(MTK_HAL_SEJ_GPT_MODULE_ENABLE), y)
CFLAGS   += -DHAL_SEJ_GPT_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_sej_gpt.c
endif

ifeq ($(MTK_HAL_GCPU_MODULE_ENABLE), y)
CFLAGS   += -DHAL_GCPU_MODULE_ENABLED
#C_FILES  += $(MT7933_CHIP)/src/gcpu_hw.c
endif

ifeq ($(MTK_HAL_I2C_MASTER_MODULE_ENABLE), y)
CFLAGS   += -DHAL_I2C_MASTER_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_i2c_master.c
C_FILES  += $(MT7933_CHIP)/src/hal_i2c_master_internal.c
endif

ifeq ($(MTK_HAL_NVIC_MODULE_ENABLE), y)
CFLAGS   += -DHAL_NVIC_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_nvic.c
C_FILES  += $(MT7933_CHIP)/src/hal_nvic_internal.c
endif

ifeq ($(MTK_HAL_IRRX_MODULE_ENABLE), y)
CFLAGS   += -DHAL_IRRX_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_irrx.c
endif

ifeq ($(MTK_HAL_KEYPAD_MODULE_ENABLE), y)
CFLAGS   += -DHAL_KEYPAD_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_keypad.c
C_FILES  += $(MT7933_CHIP)/src/hal_keypad_internal.c
endif

ifeq ($(MTK_HAL_GCPU_MODULE_ENABLE), y)
ifeq ($(MTK_HAL_MD5_MODULE_ENABLE), y)
CFLAGS   += -DHAL_MD5_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_md5.c
endif
endif

ifeq ($(MTK_HAL_MPU_MODULE_ENABLE), y)
CFLAGS   += -DHAL_MPU_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_mpu.c
C_FILES  += $(MT7933_CHIP)/src/hal_mpu_internal.c
endif

ifeq ($(MTK_HAL_SECURITY_MODULE_ENABLE), y)
ifeq ($(MTK_HAL_ASIC_MPU_MODULE_ENABLE), y)
CFLAGS   += -DHAL_ASIC_MPU_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_asic_mpu.c
endif
endif

ifeq ($(MTK_HAL_DEVAPC_MODULE_ENABLE), y)
CFLAGS   += -DHAL_DEVAPC_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_devapc.c
endif

ifeq ($(MTK_HAL_PMU_MODULE_ENABLE), y)
CFLAGS   += -DHAL_PMU_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_pmu.c
C_FILES  += $(MT7933_CHIP)/src/hal_pmu_wrap_interface.c
endif

ifeq ($(MTK_HAL_PMU_FORCE_VCORE_0P8V), y)
CFLAGS   += -DHAL_PMU_FORCE_VCORE_0P8V
endif

ifeq ($(MTK_HAL_CLK_FORCE_RTC_XOSC), y)
CFLAGS   += -DHAL_CLK_FORCE_RTC_XOSC
endif

ifeq ($(MTK_HAL_PSRAM_MODULE_ENABLE), y)
CFLAGS   += -DHAL_PSRAM_MODULE_ENABLED
CFLAGS   += -I$(SOURCE_DIR)/$(MT7933_CHIP)/src/psramc/inc
CFLAGS   += -I$(SOURCE_DIR)/$(MT7933_CHIP)/src

ifeq ($(MTK_HAL_PSRAM_UHS_ENABLE), y)
CFLAGS   += -DHAL_PSRAM_UHS_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_psram.c
C_FILES  += $(MT7933_CHIP)/src/psramc/Hal_io.c
C_FILES  += $(MT7933_CHIP)/src/psramc/pemi.c
C_FILES  += $(MT7933_CHIP)/src/psramc/psramc_pi_basic_api.c
C_FILES  += $(MT7933_CHIP)/src/psramc/psramc_pi_calibration_api.c
C_FILES  += $(MT7933_CHIP)/src/psramc/psramc_pi_main.c
else
CFLAGS   += -DHAL_PSRAM_NON_UHS_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_psram.c
C_FILES  += $(MT7933_CHIP)/src/psramc/Hal_io.c
C_FILES	 += $(MT7933_CHIP)/src/psramc/non_uhs_psram.c
endif

endif


ifeq ($(MTK_HAL_PWM_MODULE_ENABLE), y)
CFLAGS   += -DHAL_PWM_MODULE_ENABLED
CFLAGS   += -I$(SOURCE_DIR)/$(MT7933_CHIP)/src/pwm/inc
C_FILES  += $(MT7933_CHIP)/src/pwm/hdl_pwm.c
C_FILES  += $(MT7933_CHIP)/src/pwm/mhal_pwm.c
C_FILES  += $(MT7933_CHIP)/src/pwm/hal_pwm.c
endif

ifeq ($(MTK_HAL_RTC_MODULE_ENABLE), y)
CFLAGS   += -DHAL_RTC_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_rtc.c
endif

ifeq ($(MTK_HAL_SPI_MASTER_MODULE_ENABLE), y)
CFLAGS   += -DHAL_SPI_MASTER_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_spi_master.c
C_FILES  += $(MT7933_CHIP)/src/hal_spi_master_internal.c
endif

ifeq ($(MTK_HAL_SPI_SLAVE_MODULE_ENABLE), y)
CFLAGS   += -DHAL_SPI_SLAVE_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_spi_slave.c
C_FILES  += $(MT7933_CHIP)/src/hal_spi_slave_internal.c
endif

ifeq ($(MTK_HAL_SD_MODULE_ENABLE), y)
CFLAGS   += -DHAL_SD_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_msdc.c
C_FILES  += $(MT7933_CHIP)/src/hal_sd.c
C_FILES  += $(MT7933_CHIP)/src/hal_mtk_sd.c
endif

ifeq ($(MTK_HAL_SDIO_MODULE_ENABLE), y)
CFLAGS   += -DHAL_SDIO_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_msdc.c
C_FILES  += $(MT7933_CHIP)/src/hal_sdio.c
C_FILES  += $(MT7933_CHIP)/src/hal_mtk_sdio.c
endif

ifeq ($(MTK_HAL_SDIO_SLAVE_MODULE_ENABLE), y)
CFLAGS   += -DHAL_SDIO_SLAVE_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_sdio_slave.c
C_FILES  += $(MT7933_CHIP)/src/hal_sdio_slave_internal.c
endif

ifeq ($(MTK_HAL_GCPU_MODULE_ENABLE), y)
ifeq ($(MTK_HAL_SHA_MODULE_ENABLE), y)
CFLAGS   += -DHAL_SHA_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_sha.c
endif
endif

ifeq ($(MTK_HAL_SLEEP_MANAGER_MODULE_ENABLE), y)
CFLAGS   += -DHAL_SLEEP_MANAGER_ENABLED
endif

ifeq ($(MTK_HAL_TRNG_MODULE_ENABLE), y)
CFLAGS   += -DHAL_TRNG_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_trng.c
C_FILES  += $(MT7933_CHIP)/src/hal_trng_internal.c
endif

ifeq ($(MTK_HAL_UART_MODULE_ENABLE), y)
CFLAGS   += -DHAL_UART_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_uart.c
endif

ifeq ($(MTK_HAL_USB_MODULE_ENABLE), y)
CFLAGS   += -DHAL_USB_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_usb.c
endif

ifeq ($(MTK_HAL_WDT_MODULE_ENABLE), y)
CFLAGS   += -DHAL_WDT_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_wdt.c
C_FILES  += $(MT7933_CHIP)/src/hal_wdt_internal.c
endif

ifeq ($(MTK_HAL_DWT_MODULE_ENABLE), y)
CFLAGS   += -DHAL_DWT_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_dwt.c
C_FILES  += $(MT7933_CHIP)/src/hal_dwt_internal.c
else
C_FILES  += $(MT7933_CHIP)/src/hal_dwt_internal.c
endif

ifeq ($(MTK_HAL_CLK_CTP_SUPPORT), y)
CFLAGS   += -DHAL_CLK_CTP_SUPPORT
endif

ifeq ($(MTK_HAL_SPM_LOW_POWER_SUPPORT), y)
CFLAGS   += -DHAL_SPM_LOW_POWER_SUPPORT
endif

ifeq ($(MTK_HAL_TOP_THERMAL_MODULE_ENABLE), y)
CFLAGS   += -DHAL_TOP_THERMAL_MODULE_ENABLED
C_FILES  += $(MT7933_CHIP)/src/hal_top_thermal.c
endif

ifeq ($(MTK_HAL_EFUSE_CLI_ENABLE), y)
CFLAGS   += -DMTK_HAL_EFUSE_CLI_ENABLE
CFLAGS   += -I$(SOURCE_DIR)/$(MT7933_CHIP)/src/efuse/inc
C_FILES  += $(MT7933_CHIP)/src/efuse/efuse_cli.c
ifeq ($(MTK_HAL_EFUSE_MODULE_ENABLE), n)
LIBS += $(SOURCE_DIR)/prebuilt/driver/chip/mt7933/lib/libefuse.a
endif
endif

ifeq ($(MTK_HAL_EFUSE_MODULE_ENABLE), y)
CFLAGS   += -DHAL_EFUSE_MODULE_ENABLED
endif

C_FILES  += $(MT7933_CHIP)/src/hal_log.c
C_FILES  += $(MT7933_CHIP)/src/hal_ecc_api.c

ifeq ($(MTK_HAL_KPD_MODULE_ENABLE), y)
CFLAGS   += -I$(SOURCE_DIR)/$(MT7933_CHIP)/src/keypad/inc
CFLAGS   += -I$(SOURCE_DIR)/$(MT7933_CHIP)/src/inc
C_FILES  += $(MT7933_CHIP)/src/keypad/src/kpd.c
endif

ifeq ($(MTK_HAL_SECURITY_MODULE_ENABLE), y)
CFLAGS   += -DHAL_SECURITY_MODULE_ENABLE
endif

ifeq ($(MTK_HAL_AUDIO_MODULE_ENABLED),y)
ifeq ($(MTK_AUDIO_DSP_PLAYBACK_ENABLE), y)
CFLAGS   += -DCFG_AUDIO_DSP_PLAYBACK_EN
endif
ifeq ($(MTK_AUDIO_DSP_LEAUDIO_ENABLE), y)
CFLAGS  += -DCFG_AUDIO_DSP_LEAUDIO_EN
CFLAGS  += -DCFG_LEA_DUMP_ENABLE
endif #MTK_AUDIO_DSP_LEAUDIO_ENABLE
C_FILES  += $(MT7933_CHIP)/src/hal_audio.c
C_FILES  += $(MT7933_CHIP)/src/hal_audio_internal.c
C_FILES  += $(MT7933_CHIP)/src/hal_audio_dsp_controller.c
C_FILES  += $(MT7933_CHIP)/src/hal_core_status.c
endif

#################################################################################
#include path
CFLAGS  += -I../inc
CFLAGS  += -Iinc

CFLAGS  += -Isrc/common/include

CFLAGS 	+= -I$(SOURCE_DIR)/kernel/service/inc
CFLAGS += -I$(SOURCE_DIR)/driver/CMSIS/Device/MTK/mt7933/Include
CFLAGS += -I$(SOURCE_DIR)/driver/CMSIS/Include

