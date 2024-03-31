export $(IC_CONFIG)

ifeq ($(IC_CONFIG), mt7933)
SIGMA_SRC = middleware/MTK/connectivity/wlan_tool/sigma

# Gloabl Config
-include $(SOURCE_DIR)/.config
# IC Config
-include $(SOURCE_DIR)/config/chip/$(IC_CONFIG)/chip.mk
# Board Config
-include $(SOURCE_DIR)/config/board/$(BOARD_CONFIG)/board.mk
$(info wifi driver $(WF_DRIVER_DIR))

-include $(SOURCE_DIR)/$(SIGMA_SRC)/Makefile.inc

#################################################################################
#include path
CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/SourceV10/include 
CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/SourceV10/portable/GCC/ARM_CM33_NTZ
CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/SourceV10/portable/GCC/ARM_CM33_NTZ/non_secure
CFLAGS  += -I$(SOURCE_DIR)/$(WF_DRIVER_DIR)
CFLAGS  += -I$(SOURCE_DIR)/$(WF_DRIVER_DIR)/os/freertos/include
CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/minisupp/src_protected/wpa_supplicant_2.6/src
CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/minisupp/src_protected/wpa_supplicant_2.6/src/common
CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/minisupp/src_protected/wpa_supplicant_2.6/src/utils
CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/minisupp/src_protected/wpa_supplicant_2.6/src/driver
CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/lwip/src/include
CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/lwip/ports/include
#CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/ping/inc
CFLAGS  += -I$(SOURCE_DIR)/$(PROJ_PATH)/inc
CFLAGS 	+= -I$(SOURCE_DIR)/driver/chip/mt7933/inc
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/inc
CFLAGS  += -I$(SOURCE_DIR)/$(SIGMA_SRC)/inc
CFLAGS  += -I$(SOURCE_DIR)/$(SIGMA_SRC)/mediatek/wpa

include $(SOURCE_DIR)/$(SIGMA_SRC)/lib/module.mk
include $(SOURCE_DIR)/$(SIGMA_SRC)/dut/module.mk
include $(SOURCE_DIR)/$(SIGMA_SRC)/mediatek/module.mk

#################################################################################

CFLAGS += -D_FREERTOS
CFLAGS += -DCONFIG_CTRL_IFACE=udp -DCONFIG_CTRL_IFACE_UDP
CFLAGS += -DWPA_SUPPLICANT_VER=WPA_SUPPLICANT_2_6
#$(info $(CFLAGS))
#LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/minisupp/lib/libminisupp.a
endif
