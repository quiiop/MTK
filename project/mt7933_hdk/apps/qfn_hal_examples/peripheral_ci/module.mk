CI_SRC = $(APP_PATH_CI)/src


##source code
APP_FILES += $(CI_SRC)/ci_main.c

CFLAGS += -I$(SOURCE_DIR)/$(APP_PATH_CI)/inc


#peripheral
ifeq ($(MTK_HAL_SDIO_MODULE_ENABLE), y)
CFLAGS += -DCI_SDIOM_ENABLE
APP_FILES      += $(CI_SRC)/ci_sdiom.c
endif

ifeq ($(MTK_HAL_SDIO_SLAVE_MODULE_ENABLE), y)
CFLAGS += -DCI_SDIOS_ENABLE
APP_FILES      += $(CI_SRC)/ci_sdios.c
endif

ifeq ($(MTK_HAL_SPI_MASTER_MODULE_ENABLE), y)
ifeq ($(MTK_HAL_SPI_SLAVE_MODULE_ENABLE), y)
CFLAGS += -DCI_SPI_ENABLE
APP_FILES      += $(CI_SRC)/ci_spi.c
endif
endif

ifeq ($(MTK_HAL_RTC_MODULE_ENABLE), y)
CFLAGS += -DCI_RTC_ENABLE
APP_FILES      += $(CI_SRC)/ci_rtc.c
endif

ifeq ($(MTK_HAL_EINT_MODULE_ENABLE), y)
CFLAGS += -DCI_EINT_ENABLE
APP_FILES      += $(CI_SRC)/ci_eint.c
endif

ifeq ($(MTK_HAL_GCPU_MODULE_ENABLE), y)
CFLAGS += -DCI_GCPU_ENABLE
APP_FILES      += $(CI_SRC)/ci_gcpu.c
endif

ifeq ($(MTK_HAL_ECC_MODULE_ENABLE), y)
CFLAGS += -DCI_ECC_ENABLE
APP_FILES      += $(CI_SRC)/ci_ecc.c
endif

ifeq ($(MTK_HAL_GDMA_MODULE_ENABLE), y)
CFLAGS += -DCI_GDMA_ENABLE
APP_FILES      += $(CI_SRC)/ci_gdma.c
endif

ifeq ($(MTK_HAL_ADC_MODULE_ENABLE), y)
CFLAGS += -DCI_ADC_ENABLE
APP_FILES      += $(CI_SRC)/ci_adc.c
endif

ifeq ($(MTK_HAL_NVIC_MODULE_ENABLE), y)
CFLAGS += -DCI_NVIC_ENABLE
APP_FILES      += $(CI_SRC)/ci_nvic.c
endif

ifeq ($(MTK_HAL_PWM_MODULE_ENABLE), y)
CFLAGS += -DCI_PWM_ENABLE
APP_FILES      += $(CI_SRC)/ci_pwm.c
endif

ifeq ($(MTK_HAL_GPT_MODULE_ENABLE), y)
CFLAGS += -DCI_GPT_ENABLE
APP_FILES      += $(CI_SRC)/ci_gpt.c
endif

ifeq ($(MTK_HAL_WDT_MODULE_ENABLE), y)
CFLAGS += -DCI_WDT_ENABLE
APP_FILES      += $(CI_SRC)/ci_wdt.c
endif

ifeq ($(MTK_HAL_SLEEP_MANAGER_MODULE_ENABLE), y)
CFLAGS += -DCI_SLEEPMANAGER_ENABLE
APP_FILES      += $(CI_SRC)/ci_sleepmanager.c
endif

ifeq ($(MTK_HAL_PMU_MODULE_ENABLE), y)
CFLAGS += -DCI_PMU_ENABLE
APP_FILES      += $(CI_SRC)/ci_pmu.c
endif

ifeq ($(MTK_NVDM_ENABLE), y)
CFLAGS += -DCI_NVDM_ENABLE
APP_FILES      += $(CI_SRC)/ci_nvdm.c
endif

ifeq ($(MTK_HAL_CACHE_MODULE_ENABLE), y)
CFLAGS += -DCI_CACHE_ENABLE
APP_FILES      += $(CI_SRC)/ci_cache.c
endif

ifeq ($(MTK_HAL_MPU_MODULE_ENABLE), y)
CFLAGS += -DCI_MPU_ENABLE
APP_FILES      += $(CI_SRC)/ci_mpu.c
endif

ifeq ($(MTK_HAL_GPIO_MODULE_ENABLE), y)
CFLAGS += -DCI_GPIO_ENABLE
APP_FILES      += $(CI_SRC)/ci_gpio.c
endif

ifeq ($(MTK_HAL_FLASH_MODULE_ENABLE), y)
CFLAGS += -DCI_FLASH_ENABLE
APP_FILES      += $(CI_SRC)/ci_flash.c
endif

ifeq ($(MTK_HAL_TRNG_MODULE_ENABLE), y)
CFLAGS += -DCI_TRNG_ENABLE
APP_FILES      += $(CI_SRC)/ci_trng.c
endif

