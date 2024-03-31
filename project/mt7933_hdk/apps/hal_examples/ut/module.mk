
UT_FEATURE ?= ../ut/ut_feature.mk

include $(UT_FEATURE)

UT_SRC  = $(APP_PATH_UT)/src

APP_FILES      += $(UT_SRC)/ut_main.c

#
# UT CLI Command
ifeq ($(MTK_MINICLI_ENABLE),y)
APP_FILES      += $(UT_SRC)/ut_cli.c
endif

#
# RTOS UT Feature
#
ifeq ($(UT_RTOS_ENABLE), y)
CFLAGS    += -DUT_RTOS_ENABLE
ifeq ($(UT_POSIX_ENABLE), y)
CFLAGS    += -DUT_POSIX_ENABLE
endif
APP_FILES += $(UT_SRC)/ut_rtos.c
endif
# End-Of-RTOS UT Feature


#
# Platform UT Feature
#
ifeq ($(UT_PLATFORM_ENABLE), y)
CFLAGS    += -DUT_PLATFORM_ENABLE
APP_FILES += $(UT_SRC)/ut_platform.c

ifeq ($(UT_PLATFORM_LAYOUT_ENABLE),y)
CFLAGS    += -DUT_PLATFORM_LAYOUT_ENABLE
APP_FILES += $(UT_SRC)/plat/ut_plat_layout.c
endif

ifeq ($(MTK_NVDM_ENABLE),y)
ifeq ($(UT_PLATFORM_NVDM_ENABLE),y)
CFLAGS    += -DUT_PLATFORM_NVDM_ENABLE
APP_FILES += $(UT_SRC)/plat/ut_plat_nvdm.c
endif
endif


ifeq ($(UT_PLATFORM_COREDUMP_ENABLE),y)
CFLAGS    += -DUT_PLATFORM_COREDUMP_ENABLE
APP_FILES += $(UT_SRC)/plat/ut_plat_coredump.c
endif


ifeq ($(UT_PLATFORM_HEAP_ENABLE),y)
CFLAGS    += -DUT_PLATFORM_HEAP_ENABLE
APP_FILES += $(UT_SRC)/plat/ut_plat_heap.c
endif


endif
# End-Of-Platform UT Feature


#
# HAL UT Features
#
ifeq ($(UT_HAL_ENABLE), y)
CFLAGS    += -DUT_HAL_ENABLE
APP_FILES += $(UT_SRC)/ut_hal.c

ifeq ($(MTK_HAL_ADC_MODULE_ENABLE), y)
ifeq ($(UT_HAL_ADC_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_ADC_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_adc.c
endif
endif

ifeq ($(MTK_HAL_AES_MODULE_ENABLE), y)
ifeq ($(UT_HAL_AES_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_AES_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_aes.c
endif
endif

ifeq ($(MTK_HAL_CACHE_MODULE_ENABLE), y)
ifeq ($(UT_HAL_CACHE_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_CACHE_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_cache.c
endif
endif

ifeq ($(MTK_HAL_CLOCK_MODULE_ENABLE), y)
ifeq ($(UT_HAL_CLOCK_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_CLOCK_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_clock.c
endif
endif

ifeq ($(MTK_HAL_DES_MODULE_ENABLE), y)
ifeq ($(UT_HAL_DES_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_DES_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_des.c
endif
endif

ifeq ($(MTK_HAL_EFUSE_MODULE_ENABLE), y)
ifeq ($(UT_HAL_EFUSE_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_EFUSE_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_efuse.c
endif
endif

ifeq ($(MTK_HAL_EINT_MODULE_ENABLE), y)
ifeq ($(UT_HAL_EINT_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_EINT_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_eint.c
endif
endif

ifeq ($(MTK_HAL_FLASH_MODULE_ENABLE), y)
ifeq ($(UT_HAL_FLASH_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_FLASH_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_flash.c
endif
endif

ifeq ($(MTK_HAL_GDMA_MODULE_ENABLE), y)
ifeq ($(UT_HAL_GDMA_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_GDMA_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_gdma.c
endif
endif

ifeq ($(MTK_HAL_GPIO_MODULE_ENABLE), y)
ifeq ($(UT_HAL_GPIO_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_GPIO_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_gpio.c
endif
endif

ifeq ($(MTK_HAL_GPT_MODULE_ENABLE), y)
ifeq ($(UT_HAL_GPT_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_GPT_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_gpt.c
endif
endif

ifeq ($(MTK_HAL_I2C_MASTER_MODULE_ENABLE), y)
ifeq ($(UT_HAL_I2C_MASTER_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_I2C_MASTER_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_i2c_master.c
endif
endif

ifeq ($(MTK_HAL_NVIC_MODULE_ENABLE), y)
ifeq ($(UT_HAL_NVIC_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_NVIC_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_nvic.c
endif
endif

ifeq ($(MTK_HAL_IRRX_MODULE_ENABLE), y)
ifeq ($(UT_HAL_IRRX_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_IRRX_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_irrx.c
endif
endif

ifeq ($(MTK_HAL_KEYPAD_MODULE_ENABLE), y)
ifeq ($(UT_HAL_KEYPAD_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_KEYPAD_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_keypad.c
endif
endif

ifeq ($(MTK_HAL_MD5_MODULE_ENABLE), y)
ifeq ($(UT_HAL_MD5_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_MD5_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_md5.c
endif
endif

ifeq ($(MTK_HAL_MPU_MODULE_ENABLE), y)
ifeq ($(UT_HAL_MPU_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_MPU_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_mpu.c
endif
endif

ifeq ($(MTK_HAL_ASIC_MPU_MODULE_ENABLE), y)
ifeq ($(UT_HAL_ASIC_MPU_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_ASIC_MPU_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_asic_mpu.c
endif
endif

ifeq ($(MTK_HAL_PMU_MODULE_ENABLE), y)
ifeq ($(UT_HAL_PMU_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_PMU_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_pmu.c
endif
endif

ifeq ($(MTK_HAL_PSRAM_MODULE_ENABLE), y)
ifeq ($(UT_HAL_PSRAM_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_PSRAM_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_psram.c
endif
endif

ifeq ($(MTK_HAL_PWM_MODULE_ENABLE), y)
ifeq ($(UT_HAL_PWM_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_PWM_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_pwm.c
endif
endif

ifeq ($(MTK_HAL_RTC_MODULE_ENABLE), y)
ifeq ($(UT_HAL_RTC_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_RTC_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_rtc.c
endif
endif

ifeq ($(MTK_HAL_SPI_MASTER_MODULE_ENABLE), y)
ifeq ($(UT_HAL_SPI_MASTER_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_SPI_MASTER_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_spi_master.c
endif
endif

ifeq ($(MTK_HAL_SPI_SLAVE_MODULE_ENABLE), y)
ifeq ($(UT_HAL_SPI_SLAVE_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_SPI_SLAVE_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_spi_slave.c
endif
endif

ifeq ($(MTK_HAL_SD_MODULE_ENABLE), y)
ifeq ($(UT_HAL_SD_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_SD_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_sd.c
endif
endif

ifeq ($(MTK_HAL_SDIO_MODULE_ENABLE), y)
ifeq ($(UT_HAL_SDIO_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_SDIO_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_sdio.c
endif
endif

ifeq ($(MTK_HAL_SDIO_SLAVE_MODULE_ENABLE), y)
ifeq ($(UT_HAL_SDIO_SLAVE_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_SDIO_SLAVE_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_sdio_slave.c
endif
endif

ifeq ($(MTK_HAL_SHA_MODULE_ENABLE), y)
ifeq ($(UT_HAL_SHA_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_SHA_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_sha.c
endif
endif

ifeq ($(MTK_HAL_SLEEP_MANAGER_MODULE_ENABLE), y)
ifeq ($(UT_HAL_SLEEP_MANAGER_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_SLEEP_MANAGER_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_sleep_manager.c
endif
endif

ifeq ($(MTK_HAL_TRNG_MODULE_ENABLE), y)
ifeq ($(UT_HAL_TRNG_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_TRNG_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_trng.c
endif
endif

ifeq ($(MTK_HAL_UART_MODULE_ENABLE), y)
ifeq ($(UT_HAL_UART_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_UART_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_uart.c
endif
endif

ifeq ($(MTK_SSUSB_GADGET_ENABLE), y)
ifeq ($(UT_HAL_SSUSB_GADGET_ENABLE), y)
CFLAGS    += -DUT_HAL_SSUSB_GADGET_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_ssusb_gadget.c
endif
endif

ifeq ($(MTK_SSUSB_HOST_ENABLE), y)
ifeq ($(UT_HAL_SSUSB_HOST_ENABLE), y)
CFLAGS    += -DUT_HAL_SSUSB_HOST_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_ssusb_host.c
endif
endif

ifeq ($(MTK_HAL_WDT_MODULE_ENABLE), y)
ifeq ($(UT_HAL_WDT_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_WDT_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_wdt.c
endif
endif

ifeq ($(MTK_HAL_DWT_MODULE_ENABLE), y)
ifeq ($(UT_HAL_DWT_MODULE_ENABLE), y)
CFLAGS    += -DUT_HAL_DWT_MODULE_ENABLE
APP_FILES += $(UT_SRC)/hal/ut_hal_dwt.c
endif
endif

endif
# End-Of-HAL UT Features


CFLAGS += -I$(SOURCE_DIR)/$(APP_PATH_UT)/inc
