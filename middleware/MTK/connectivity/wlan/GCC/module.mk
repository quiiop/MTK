WIFI_CODE=$(WF_DRIVER_DIR)_src_protected
WLAN_SERVICE_DIR = middleware/MTK/connectivity/wlan_service
MTK_WF_DRIVER_DIR = $(SOURCE_DIR)/$(WF_DRIVER_DIR)

include $(SOURCE_DIR)/$(WF_DRIVER_DIR)/cflag.mk

MTK_MT7933_CONSYS_WIFI_LIB_ENABLE=y
export MTK_MT7933_CONSYS_WIFI_LIB_ENABLE
MTK_MT7933_CONSYS_WIFI_LIB_REF_LWIP_STRUCT=y
export MTK_MT7933_CONSYS_WIFI_LIB_REF_LWIP_STRUCT

ifeq ($(MTK_MT7933_CONSYS_WIFI_ENABLE), y)
CFLAGS += -DMTK_MT7933_CONSYS_WIFI_ENABLE
endif

ifeq ($(MTK_WLAN_SERVICE_ENABLE), y)
CFLAGS += -DMTK_WLAN_SERVICE_ENABLE
endif

ifeq ($(MTK_MT7933_CONSYS_WIFI_ENABLE), y)
CFLAGS += -DMTK_MT7933_CONSYS_WIFI_ENABLE
endif

ifeq ($(MTK_WIFI_FW_BUILDIN), y)
    CFLAGS += -DCFG_SUPPORT_FW_BUILDIN=1
else
    CFLAGS += -DCFG_SUPPORT_FW_BUILDIN=0
endif

ifeq ($(CONFIG_WIFI_SINGLE_FW), y)
CFLAGS += -DCONFIG_WIFI_SINGLE_FW
endif

ifeq ($(MTK_WIFI_PSRAM_ENABLE), y)
    CFLAGS += -DCFG_PSRAM_ENABLE=1
else
    CFLAGS += -DCFG_PSRAM_ENABLE=0
endif

ifeq ($(MTK_SWLA_ENABLE), y)
ifeq ($(MTK_WIFI_SWLA_ENABLE), y)
    CFLAGS += -DCFG_WIFI_SWLA_ENABLE=1
else
    CFLAGS += -DCFG_WIFI_SWLA_ENABLE=0
endif
else
    CFLAGS += -DCFG_WIFI_SWLA_ENABLE=0
endif

ifeq ($(CFG_SUPPORT_WLAN_SERVICE), y)
WLAN_SERVICE=$(WLAN_SERVICE_DIR)
endif

ifeq ($(MTK_HAL_SLEEP_MANAGER_MODULE_ENABLE), y)
CFLAGS   += -DHAL_SLEEP_MANAGER_ENABLED
endif

ifeq ($(MTK_SIGMA_ENABLE), y)
CFLAGS += -DMTK_SIGMA_ENABLE
endif
CFLAGS += -DCFG_SUPPORT_HE_ER=1
CFLAGS += -DCFG_SUPPORT_FAST_CONNECT=1
#include path
ifdef MTK_FREERTOS_VERSION
CFLAGS += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source$(MTK_FREERTOS_VERSION)/include
else
CFLAGS += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/include
endif
CFLAGS += -I$(SOURCE_DIR)/driver/chip/inc

CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/os
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/os/freertos/include
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/os/freertos/hif/axi/include
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/os/freertos/hif/common/include
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/include
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/include/chips
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/include/mgmt
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/include/nic
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/prealloc/include
#USE FW HERDER TO LOAD FW FILE
ifeq ($(CFG_SUPPORT_FREERTOS_NVRAM),n)
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/os/freertos/include/fw_header
endif

ifeq ($(CFG_SUPPORT_NO_SUPPLICANT_OPS),n)
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/minisupp/inc/inc
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/minisupp/inc/src
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/minisupp/inc/wpa_supplicant
endif

########## Wlan_service include files ##########
ifeq ($(CFG_SUPPORT_WLAN_SERVICE), y)
CFLAGS  += -I$(SOURCE_DIR)/$(WLAN_SERVICE)/include
CFLAGS  += -I$(SOURCE_DIR)/$(WLAN_SERVICE)/service/include
CFLAGS  += -I$(SOURCE_DIR)/$(WLAN_SERVICE)/glue/osal/include
CFLAGS  += -I$(SOURCE_DIR)/$(WLAN_SERVICE)/glue/hal/include
endif

CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/lwip/src/include
CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/lwip/ports/include

########## Wifitesttool include files ##########
ifeq ($(MTK_WIFI_TEST_TOOL_ENABLE), y)
CFLAGS  += -I$(SOURCE_DIR)/$(TEST_TOOL_DIR)
endif

################## AXI #####################
CFLAGS  += -DFREERTOS
ifeq ($(MTK_WF_CLI_ENABLE), y)
CFLAGS  += -DMTK_WF_DRV_CLI_ENABLE
ifeq ($(MTK_WF_DBG_CLI_ENABLE), y)
CFLAGS  += -DMTK_WF_IWPRIV_CLI_ENABLE
endif
endif

ifeq ($(MTK_WIFI_AP_ENABLE), y)
    CFLAGS += -DMTK_WIFI_AP_ENABLE=1
    CFLAGS += -DCONFIG_AP
endif

$(info MTK_WLAN_SERVICE_ENABLE=$(MTK_WLAN_SERVICE_ENABLE), MTK_WIFI_AP_ENABLE=$(MTK_WIFI_AP_ENABLE), MTK_MINISUPP_ENABLE=$(MTK_MINISUPP_ENABLE))
ifeq ($(MTK_WLAN_SERVICE_ENABLE), n)
ifeq ($(MTK_WIFI_AP_ENABLE), y)
ifeq ($(MTK_RELEASE_MODE), release)
WLAN_LIB_POST=_ap_sta
else
WLAN_LIB_POST=_ap_sta_dbg
endif
else
ifeq ($(MTK_RELEASE_MODE), release)
WLAN_LIB_POST=_sta
else
WLAN_LIB_POST=_sta_dbg
endif
endif
else
ifeq ($(MTK_WIFI_AP_ENABLE), y)
WLAN_LIB_POST=
else
ifeq ($(MTK_MINISUPP_ENABLE), y)
WLAN_LIB_POST=_sta_wifitest
else
WLAN_LIB_POST=_sta_mfg
endif
endif
endif
# Project name
WF_LIB=libwifi${WLAN_LIB_POST}${LTO_LIB_POST_NAME}.a


#################################################################################
ifneq ($(wildcard $(strip $(SOURCE_DIR))/$(WIFI_CODE)/),)
MODULE_PATH += $(MTK_WF_DRIVER_DIR)_src_protected/GCC
CFLAGS  += -I$(SOURCE_DIR)/$(WIFI_CODE)
LIBS    += $(OUTPATH)/$(WF_LIB)
else
include $(SOURCE_DIR)/prebuilt/$(WF_DRIVER_DIR)/module.mk
endif

#################################################################################
ifdef MTK_MT7933_CONSYS_WIFI_LIB_ENABLE
C_FILES += $(WF_DRIVER_DIR)/mgmt/reg_rule.c
C_FILES += $(WF_DRIVER_DIR)/flash_addr.c
ifeq ($(MTK_MT7933_CONSYS_WIFI_ENABLE), y)
C_FILES += $(WF_DRIVER_DIR)/chips/mt7933/mt7933_fw.c
endif

ifdef MTK_MT7933_CONSYS_WIFI_LIB_REF_LWIP_STRUCT
C_FILES += $(WF_DRIVER_DIR)/os/freertos/netif/wifi_netif.c
C_FILES += $(WF_DRIVER_DIR)/os/freertos/gl_init.c
C_FILES += $(WF_DRIVER_DIR)/os/freertos/gl_kal.c
C_FILES += $(WF_DRIVER_DIR)/os/freertos/gl_upperlayer.c
ifeq ($(CFG_SUPPORT_P2P),y)
C_FILES += $(WF_DRIVER_DIR)/os/freertos/gl_p2p.c
endif
ifeq ($(MTK_WF_CLI_ENABLE), y)
C_FILES += $(WF_DRIVER_DIR)/os/freertos/gl_wifi_cli.c
endif
C_FILES += $(WF_DRIVER_DIR)/os/freertos/wifi_api.c
endif
endif

#################################################################################
# eFUSE buffer bin
#################################################################################

ifeq ($(MTK_MT7933_CONSYS_FW_MODE),bga)
MTK_WIFI_EFUSE_BUFFER_BIN = MT7933_BGA_TDD_EEPROM.bin
else
MTK_WIFI_EFUSE_BUFFER_BIN = MT7931_QFN_TDD_EEPROM.bin
endif

#################################################################################
#
# If Wi-Fi single F/W is supported, CONFIG_WIFI_SINGLE_FW is set to 'y' and
# its filename is stored in MTK_WIFI_FIRMWARE_BIN.
#
# If digital signature is supported, MTK_WIFI_SIGN_ENABLE is set to 'y' and
# its filename is stored in MTK_WIFI_FIRMWARE_SGN.
# (Digital signature is supported only on single F/W bin file)
#
# Projects should:
# 1. make recursively using target 'mtk_wifi_firmware' and provide parameters
#    for firmware preparation:
#       make mtk_wifi_firmware MTK_WIFI_FIRMWARE_HDR_SZ=<size> \
#                              MTK_WIFI_FIRMWARE_ADDR=<addr>     \
#                              MTK_WIFI_FIRMWARE_SIZE=<size>     \
#                              MTK_WIFI_FIRMWARE_VER=<version>
# 2. use MTK_WIFI_FIRMWARE to generate scatter files.
#
#
# Depends on MTK_WIFI_SIGN_ENABLE, the firmware for project could be:
# 1. WIFI_RAM_CODE_MT7933_ALL.sgn, or
# 2. WIFI_RAM_CODE_MT7933_ALL.bin
#
# The filename is kept in MTK_WIFI_SINGLE_FIRMWARE for projects to use it
# further.
#
#################################################################################


MTK_WIFI_FIRMWARE_VER ?= 1

ifeq ($(CONFIG_WIFI_SINGLE_FW), y)

ifeq ($(MTK_RELEASE_MODE),)
ifeq ($(MTK_WIFI_SIGN_ENABLE),y)
MTK_WIFI_FIRMWARE_BIN = $(OUTPATH)/WIFI_RAM_CODE_MT7933_ALL.bin
MTK_WIFI_FIRMWARE_SGN = $(OUTPATH)/WIFI_RAM_CODE_MT7933_ALL.sgn
MTK_WIFI_FIRMWARE     = WIFI_RAM_CODE_MT7933_ALL.sgn
else
MTK_WIFI_FIRMWARE_BIN = $(OUTPATH)/WIFI_RAM_CODE_MT7933_ALL.bin
MTK_WIFI_FIRMWARE_SGN =
MTK_WIFI_FIRMWARE     = WIFI_RAM_CODE_MT7933_ALL.bin
endif
else
ifeq ($(MTK_WIFI_SIGN_ENABLE),y)
MTK_WIFI_FIRMWARE_BIN = $(OUTPATH)/WIFI_RAM_CODE_MT7933_MSHRINK_ALL.bin
MTK_WIFI_FIRMWARE_SGN = $(OUTPATH)/WIFI_RAM_CODE_MT7933_MSHRINK_ALL.sgn
MTK_WIFI_FIRMWARE     = WIFI_RAM_CODE_MT7933_MSHRINK_ALL.sgn
else
MTK_WIFI_FIRMWARE_BIN = $(OUTPATH)/WIFI_RAM_CODE_MT7933_MSHRINK_ALL.bin
MTK_WIFI_FIRMWARE_SGN =
MTK_WIFI_FIRMWARE     = WIFI_RAM_CODE_MT7933_MSHRINK_ALL.bin
endif
endif

mtk_wifi_firmware:
ifeq ($(MTK_WIFI_SIGN_ENABLE),y)
	$(Q)make sboot_firmware_sign                            \
	         SBOOT_FW_HDR_SZ=$(MTK_WIFI_FIRMWARE_HDR_SZ)    \
	         SBOOT_FW_ADDR=$(MTK_WIFI_FIRMWARE_ADDR)        \
	         SBOOT_FW_SIZE=$(MTK_WIFI_FIRMWARE_SIZE)        \
	         SBOOT_FW_VER=$(MTK_WIFI_FIRMWARE_VER)          \
	         SBOOT_FW_IN=$(MTK_WIFI_FIRMWARE_BIN)           \
	         SBOOT_FW_OUT=$(MTK_WIFI_FIRMWARE_SGN)          \
             Q=$(Q)
endif

endif
