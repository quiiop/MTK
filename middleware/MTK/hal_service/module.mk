SCR_PATH = middleware/MTK/hal_service

ifeq ($(MTK_HAL_ADC_MODULE_ENABLE), y)
C_FILES += $(SCR_PATH)/adc/os_hal_adc.c
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/mt7933/inc/
endif