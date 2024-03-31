MINISUPP_SRC = middleware/third_party/minisupp
ifeq ($(MTK_WIFI_AP_ENABLE),n)
MINISUPP_POST_NAME=_sta
else
MINISUPP_POST_NAME=
endif

ifneq ($(MTK_RELEASE_MODE),)
ifeq ($(MTK_RELEASE_MODE),release)
MODE_POST_NAME=_mshrink
else
MODE_POST_NAME=_mshrink_dbg
endif
else
MODE_POST_NAME=
endif

#################################################################################
#include path
#CFLAGS  += -I$(SOURCE_DIR)/middleware/util/include
#CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/minicli/include
#CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/include
#CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/include 
#CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/portable/GCC/ARM_CM4F
#CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source
#CFLAGS  += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/include
#CFLAGS  += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/os/freertos/include
#CFLAGS  += -I$(SOURCE_DIR)/driver/chip/inc
#CFLAGS 	+= -I$(SOURCE_DIR)/middleware/MTK/minisupp/inc

#################################################################################

#lib/libminisupp_sta_lto.a
MINISUPP_LIB = libminisupp${MINISUPP_POST_NAME}${LTO_LIB_POST_NAME}${MODE_POST_NAME}.a

#################################################################################
ifneq ($(wildcard $(strip $(SOURCE_DIR))/$(MINISUPP_SRC)/),)
$(info build minisupp src: $(MINISUPP_SRC))
MODULE_PATH += $(SOURCE_DIR)/$(MINISUPP_SRC)/GCC
LIBS += $(OUTPATH)/$(MINISUPP_LIB)
else
$(info link minisupp lib)
include $(SOURCE_DIR)/prebuilt/middleware/MTK/minisupp/module.mk
endif

#################################################################################
