# Copyright Statement:
#
# (C) 2005-2016  MediaTek Inc. All rights reserved.
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
# Without the prior written permission of MediaTek and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.
# You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
# if you have agreed to and been bound by the applicable license agreement with
# MediaTek ("License Agreement") and been granted explicit permission to do so within
# the License Agreement ("Permitted User").  If you are not a Permitted User,
# please cease any access or use of MediaTek Software immediately.
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
# ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
# WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#

#######################################################
# Project vairables assignment

SOURCE_DIR = ../../../../..
BINPATH     = ~/gcc-arm-none-eabi/bin
DATIME      = $(shell date --iso=seconds)
SDK_PATH = $(abspath $(PWD)/$(SOURCE_DIR))
PWD= $(shell pwd)
FEATURE ?= feature.mk
HAL_FEATURE ?= hal_feature.mk
DEBUG = 0
FLOAT_TYPE = hard
BUILD_DIR = $(PWD)/build
SHELL     = /bin/bash

# Project name
PROJ_NAME = $(shell basename $(dir $(PWD)))
PROJ_PATH = $(abspath $(dir $(PWD)))/GCC
OUTPATH = $(PWD)/build
SCATTER_TOOL_PY=$(SDK_PATH)/tools/scripts/get_scatter/get_scatter_arg_v2.py

LDFLAGS = $(ALLFLAGS) $(FPUFLAGS) --specs=nano.specs -lnosys -nostartfiles
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-wrap=malloc -Wl,-wrap=calloc -Wl,-wrap=realloc -Wl,-wrap=free
LDFLAGS += -u _printf_float -u _scanf_float

#######################################################
# default target before any other targets

all:

#######################################################
# Include Module

include $(FEATURE)
include $(HAL_FEATURE)

LDFLAGS     += -L$(PROJ_PATH)/ld
LDFILE_XIP   = mt7933_flash.ld
LDFLAGS_XIP += -Wl,-T$(LDFILE_XIP)
LDFLAGS_RAM += -Wl,-Tmt7933_ram.ld

XIP_EXCLUDE_OBJECTS = $(shell grep -m 1 EXCLUDE_FILE $(LDFILE_XIP) | sed 's/\(.*\)EXCLUDE_FILE(\(.*\))\ \(.*\)/\2/g')


# Gloabl Config
-include $(SOURCE_DIR)/.config
# IC Config
-include $(SOURCE_DIR)/config/chip/$(IC_CONFIG)/chip.mk
# Board Config
-include $(SOURCE_DIR)/config/board/$(BOARD_CONFIG)/board.mk

include $(SOURCE_DIR)/driver/board/mt7933_hdk/ept/module.mk

ifeq ($(MTK_BUILD_WARNING_AS_ERROR_ENABLE),y)
CFLAGS += -Werror
endif
CFLAGS += -Wdouble-promotion -Wtype-limits
CFLAGS += -Wsign-compare -Wstrict-prototypes
CFLAGS += -Wmissing-field-initializers -Wvla
#sub-items of -Wextra
CFLAGS +=-Wclobbered -Wignored-qualifiers -Wold-style-declaration
CFLAGS +=-Wmissing-parameter-type -Woverride-init -Wstring-compare
CFLAGS +=-Wuninitialized -Wshift-negative-value -Wunused-but-set-parameter
CFLAGS +=-Wempty-body -Wimplicit-fallthrough=3 -Wcast-function-type -Wenum-conversion
CFLAGS +=-mno-unaligned-access

ifeq ($(MTK_HAL_SPM_LOW_POWER_SUPPORT), y)
CFLAGS += -mcmse
endif

ifeq ($(MTK_IOT_SDK_DEMO_AS_LIB),y)
CFLAGS += -DMTK_IOT_SDK_DEMO_AS_LIB
endif

export IC_CONFIG
export BORAD_CONFIG
export PROJ_NAME
export PROJ_PATH

# WiFi CONNAC driver files
ifeq ($(MTK_MT7933_CONSYS_WIFI_ENABLE), y)
CFLAGS += -DMTK_MT7933_WIFI_ENABLE
ifeq ($(MTK_WLAN_SERVICE_ENABLE), y)
WLAN_SERVICE_DIR = middleware/MTK/connectivity/wlan_service
endif
WF_DRIVER_DIR = middleware/MTK/connectivity/wlan
export WF_DRIVER_DIR
include $(SOURCE_DIR)/$(WF_DRIVER_DIR)/GCC/module.mk
GEN4M_H_FILES = $(shell find $(SOURCE_DIR)/$(WF_DRIVER_DIR)/*/  -name *.h | grep linux -v)
GEN4M_H_FILES_PATH = $(subst $(SOURCE_DIR)/$(WF_DRIVER_DIR)/,,$(dir $(GEN4M_H_FILES)))
endif

ifeq ($(MTK_ATED_ENABLE), y)
ATED_DIR = middleware/MTK/connectivity/wlan_daemon/ated_ext
include $(SOURCE_DIR)/$(ATED_DIR)/GCC/module.mk
CFLAGS += -DMTK_ATED_ENABLE
endif

ifeq ($(MTK_WIFI_TEST_TOOL_ENABLE), y)
TEST_TOOL_DIR = middleware/MTK/connectivity/wlan_tool/wifi_test_tool/wifitesttool
$(info TEST_TOOL_DIR $(TEST_TOOL_DIR))
include $(SOURCE_DIR)/$(TEST_TOOL_DIR)/module.mk
CFLAGS += -DMTK_WIFI_TEST_TOOL_ENABLE
ifeq ($(MTK_WF_CLI_ENABLE),y)
CFLAGS += -DMTK_WF_DRV_CLI_ENABLE
CFLAGS += -DMTK_MINISUPP_CLI_ENABLE
CFLAGS += -DMTK_LWIP_CLI_ENABLE
ifeq ($(MTK_WF_DBG_CLI_ENABLE),y)
CFLAGS += -DMTK_WIFI_DRV_DBG_CLI_ENABLE
endif
endif
endif

# HAL driver files
include $(SOURCE_DIR)/driver/chip/mt7933/module.mk
# EPT Config
#include $(SOURCE_DIR)/driver/board/mt7933_hdk/ept/module.mk

# RTOS source files
include $(SOURCE_DIR)/kernel/rtos/FreeRTOS/module.mk
# kernel service files
include $(SOURCE_DIR)/kernel/service/module.mk

# RTOS POSIX files
ifeq ($(MTK_POSIX_SUPPORT_ENABLE),y)
CFLAGS += -DMTK_POSIX_SUPPORT_ENABLE
include $(SOURCE_DIR)/kernel/rtos/FreeRTOS-ext/FreeRTOS-Labs/Source/FreeRTOS-Plus-POSIX/module.mk
endif

# NVDM files
ifeq ($(MTK_NVDM_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/nvdm/module.mk
endif

include $(SOURCE_DIR)/driver/board/mt7933_hdk/util/module.mk

# Connsys
ifeq ($(MTK_MT7933_CONSYS_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/connsys/module.mk
CFLAGS += -DMTK_MT7933_CONSYS_ENABLE
endif

# WiFi RAM EMI
ifeq ($(MTK_WIFI_EMI_IN_PSRAM),y)
CFLAGS += -DMTK_WIFI_EMI_IN_PSRAM
endif

# WiFi previous GEN driver files
ifeq ($(MTK_CM4_WIFI_TASK_ENABLE),y)
ifeq ($(MTK_WIFI_ROM_ENABLE), y)
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/wifi_stack),)
include $(SOURCE_DIR)/middleware/MTK/wifi_stack/module.mk
else
include $(SOURCE_DIR)/prebuilt/middleware/MTK/wifi_stack/module.mk
endif
endif
endif

# BT driver files
ifeq ($(MTK_MT7933_BT_ENABLE),y)
MT7933_BT_DRIVER_DIR = middleware/MTK/connectivity/bt
include $(SOURCE_DIR)/$(MT7933_BT_DRIVER_DIR)/btif/GCC/module.mk
CFLAGS += -DMTK_MT7933_BT_ENABLE
endif

ifeq ($(MTK_BT_BUFFER_BIN_MODE),y)
CFLAGS  += -DMTK_BT_BUFFER_BIN_MODE
endif

# BT stack
ifeq ($(MTK_BT_ENABLE), y)
CFLAGS += -DBT_DEBUG
CFLAGS += -DMTK_BT_ENABLE
#CFLAGS += -D__BT_MULTI_ADV__
ifeq ($(MTK_BT_DUO_ENABLE), y)
CFLAGS += -D__BT_HB_DUO__
endif
#CFLAGS += -D__BT_GATTS_EH__
include $(SOURCE_DIR)/middleware/MTK/bluetooth/module.mk
include $(SOURCE_DIR)/middleware/MTK/bt_callback_manager/module.mk
endif

# BT mesh middleware
ifeq ($(MTK_BT_MESH_ENABLE), y)
include $(SOURCE_DIR)/middleware/MTK/mesh/module.mk
endif

# BT tool - boots
ifeq ($(MTK_BT_BOOTS_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/bt_tools/boots/module.mk
endif
# BT tool - picus
ifeq ($(MTK_BT_PICUS_ENABLE),y)
CFLAGS += -DMTK_BT_PICUS_ENABLE
include $(SOURCE_DIR)/middleware/MTK/bt_tools/picus/module.mk
endif

# BT stack cli
ifeq ($(MTK_BLE_CLI_ENABLE),y)
CFLAGS += -DMTK_BLE_CLI_ENABLE
endif

# BT fw assert recovery
ifeq ($(MTK_BT_SUPPORT_FW_ASSERT_RECOVERY),y)
CFLAGS += -DMTK_BT_SUPPORT_FW_ASSERT_RECOVERY
endif

# BT gatt bas service
ifeq ($(MTK_BT_BAS_SERVICE_ENABLE),y)
CFLAGS += -DMTK_BT_BAS_SERVICE_ENABLE
include $(SOURCE_DIR)/middleware/MTK/ble_bas/module.mk
endif

ifeq ($(findstring y,$(MTK_SMTCN_V4_ENABLE)$(MTK_SMTCN_V5_ENABLE)),y)
-include $(SOURCE_DIR)/middleware/MTK/smtcn/module.mk
endif
ifeq ($(MTK_MINICLI_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/minicli/module.mk
endif

# AUDIO driver files
ifeq ($(MTK_MT7933_AUDIO_DRIVER_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/audio_services/driver/mt7933/module.mk
CFLAGS += -DMTK_MT7933_AUDIO_DRIVER_ENABLE
endif

# minisupp
ifeq ($(MTK_MINISUPP_ENABLE),y)
MID_MINISUPP_PATH = $(SOURCE_DIR)/middleware/MTK/minisupp
include $(SOURCE_DIR)/middleware/MTK/minisupp/module.mk
CFLAGS += -DMTK_MINISUPP_ENABLE
$(info build minisupp)
endif

# Router
ifeq ($(MTK_WIFI_ROUTER_ENABLE),y)
CFLAGS += -DMTK_WIFI_ROUTER_ENABLE
endif

#include $(SOURCE_DIR)/middleware/third_party/micro_ecc/module.mk
#include $(SOURCE_DIR)/middleware/third_party/mbedtls/module.mk

#include lwip
ifeq ($(MTK_LWIP_ENABLE),y)
include $(SOURCE_DIR)/middleware/third_party/lwip/module.mk
endif

#include fatfs
ifeq ($(MTK_FATFS_ENABLE),y)
CFLAGS += -DMTK_FATFS_ENABLE
include $(SOURCE_DIR)/middleware/third_party/fatfs/module.mk
endif

#include ping
ifeq ($(MTK_PING_OUT_ENABLE),y)
include $(SOURCE_DIR)/middleware/third_party/ping/module.mk
endif

#include iperf
ifeq ($(MTK_IPERF_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/iperf/module.mk
endif

#include dhcpd
ifeq ($(MTK_WIFI_AP_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/dhcpd/module.mk
endif

# wifi src
#include $(SOURCE_DIR)/middleware/MTK/wifi_service/combo/module.mk

# add ATCI files
ifeq ($(MTK_ATCI_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/atci/module.mk
endif

# add TFM files
ifeq ($(MTK_TFM_ENABLE),y)
-include $(SOURCE_DIR)/middleware/third_party/tfm/build/module.mk
endif

#add hal server file
ifeq ($(MTK_HAL_SER_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/hal_service/module.mk
endif

# ssusb src
ifeq ($(findstring y,$(MTK_SSUSB_GADGET_ENABLE)$(MTK_SSUSB_HOST_ENABLE)),y)
	ifeq ($(MTK_SSUSB_GADGET_ENABLE),y)
		CFLAGS += -DMTK_SSUSB_GADGET_ENABLE
	endif
	ifeq ($(MTK_SSUSB_HOST_ENABLE),y)
		CFLAGS += -DMTK_SSUSB_HOST_ENABLE
	endif
include $(SOURCE_DIR)/middleware/MTK/ssusb/module.mk
endif

# hifi4dsp
ifeq ($(MTK_HIFI4DSP_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/hifi4dsp/module.mk
endif
include $(SOURCE_DIR)/middleware/third_party/httpclient/module.mk
ifeq ($(MTK_FOTA_V3_ENABLE),y)
    include $(SOURCE_DIR)/middleware/MTK/fota/module.mk
endif

# f32K clock source software detection
ifeq ($(MTK_F32K_SW_DETECT_ENABLED),y)
CFLAGS += -DMTK_F32K_SW_DETECT_ENABLED
endif

#add g_ap_list size
ifdef ($MTK_SCAN_LIST_SIZE)
    CFLAGS += -DMTK_SCAN_LIST_SIZE=$(MTK_SCAN_LIST_SIZE)
else
    CFLAGS += -DMTK_SCAN_LIST_SIZE=32
endif

#######################################################
# Main APP files
#######################################################
APP_PATH        = $(patsubst $(SDK_PATH)/%,%,$(abspath $(dir $(PWD))))
APP_PATH_SRC    = $(APP_PATH)/src

##############################################################################
#
# SDK source files
#
##############################################################################
APP_FILES      += $(APP_PATH_SRC)/main.c
APP_FILES      += $(APP_PATH_SRC)/regions_init.c
APP_FILES      += $(APP_PATH_SRC)/sys_init.c
#APP_FILES      += $(APP_PATH_SRC)/ept_gpio_var.c
#APP_FILES      += $(APP_PATH_SRC)/ept_eint_var.c
APP_FILES      += $(APP_PATH)/GCC/syscalls.c
#APP_FILES      += $(APP_PATH_SRC)/mem_layout_info.c
#APP_FILES      += $(APP_PATH_SRC)/wifi_lwip_helper.c
APP_FILES      += $(APP_PATH_SRC)/filogic_led.c
APP_FILES      += $(APP_PATH_SRC)/cyberon_test_cli.c

ifeq ($(MTK_AUXADCCLI_ENABLE),y)
APP_FILES      += $(APP_PATH_SRC)/adc_test.c
endif

ifeq ($(MTK_NVDM_ENABLE),y)
APP_FILES      += $(APP_PATH_SRC)/nvdm_config.c
APP_FILES      += $(APP_PATH_SRC)/network_default_config.c
APP_FILES      += $(APP_PATH_SRC)/platform_default_config.c
APP_FILES      += $(APP_PATH_SRC)/wifi_cfg_default_config.c
endif

ifeq ($(MTK_MINICLI_ENABLE),y)
APP_FILES      += $(APP_PATH_SRC)/cli_def.c
APP_FILES      += $(APP_PATH_SRC)/cli_cmds.c
endif

ifeq ($(MTK_BT_ENABLE),y)
CFLAGS += -I$(SOURCE_DIR)/$(APP_PATH)/inc/bt_app_utility
#APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/bt_lwip.c
APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/connection_info.c
APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/gatt_command.c
APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/gatt_service.c
APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/ut_app.c
APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/ut_app_gap.c
APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/ut_app_gatts.c
APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/ut_app_gattc.c
APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/ble_smtcn.c
APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/hci_log.c
APP_FILES      += $(APP_PATH_SRC)/bt_app_utility/bt_setting.c
APP_FILES      += $(APP_PATH_SRC)/bt_init.c
endif

# BT mesh app
ifeq ($(MTK_BT_MESH_ENABLE),y)
CFLAGS += -I$(SOURCE_DIR)/$(APP_PATH)/inc/mesh
ifeq ($(MTK_BT_MESH_EXAMPLE_PROJECT),switch)
APP_FILES      += $(APP_PATH_SRC)/mesh/mesh_app_switch.c \
                  $(APP_PATH_SRC)/mesh/mesh_app_util.c \
                  $(APP_PATH_SRC)/mesh/mesh_app_switch_mesh_msg.c \
                  $(APP_PATH_SRC)/mesh/mesh_app_switch_dump_node.c \
                  $(APP_PATH_SRC)/mesh/mesh_app_switch_firmware_callbacks.c
                  #$(APP_PATH_SRC)/bt/bt_gatt_service_switch.c           (Already integrated to gatt_service.c)
else ifeq ($(MTK_BT_MESH_EXAMPLE_PROJECT),vendor_device)
APP_FILES      += $(APP_PATH_SRC)/mesh/mesh_app_vendor_device.c \
                  $(APP_PATH_SRC)/mesh/mesh_app_util.c \
                  $(APP_PATH_SRC)/mesh/mesh_app_vendor_device_msg_handler.c \
                  $(APP_PATH_SRC)/mesh/mesh_app_vendor_device_pwm.c \
                  #$(APP_PATH_SRC)/bt/bt_gatt_service_vendor_device.c     (Already integrated to gatt_service.c)
endif
endif

ifeq ($(MTK_UT_ENABLE), y)
APP_PATH_UT     = $(APP_PATH)/ut
CFLAGS += -DMTK_UT_ENABLE
include ../ut/module.mk
endif

ifeq ($(MTK_FOTA_V3_ENABLE),y)
APP_FILES      += $(APP_PATH_SRC)/fota_flash_config.c
endif

ifeq ($(MTK_PERIPHERAL_CI_UT_ENABLED), y)
APP_PATH_CI     = $(APP_PATH)/peripheral_ci
CFLAGS += -DMTK_PERIPHERAL_CI_UT_ENABLED
include ../peripheral_ci/module.mk
endif

SYS_FILES      += $(APP_PATH_SRC)/system_mt7933.c

C_FILES        += $(APP_FILES) $(SYS_FILES)
S_FILES        += $(APP_PATH)/GCC/startup_mt7933.s

#######################################################
# Include path
#######################################################
CFLAGS += -I$(SOURCE_DIR)/$(APP_PATH)/inc
CFLAGS += -I$(SOURCE_DIR)/driver/chip/mt7933/inc
CFLAGS += -I$(SOURCE_DIR)/driver/chip/inc
CFLAGS += -I$(SOURCE_DIR)/driver/board/mt7933_hdk/ept/inc
#CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/wifi_service/combo/inc

###################################################
# MCUBOOT sign settings
###################################################

SGN_PEM ?= $(SDK_PATH)/$(APP_PATH)/mtk-dev.pem

ifeq ($(MTK_RTOS_SIGN_ENABLE),y)
  SGN_VER        = 0.0.1
  HEADER_SGN_SZ := 0x80
  LOAD_ADDR_XIP := 0x18044080
else
  HEADER_SGN_SZ := 0x00
endif

DaliyBuildTime := $(shell cd $(SOURCE_DIR)/prebuilt/middleware/MTK/minicli && git log -1 --date=format:'%Y_%m_%d_%H_%M_%S' | grep Date| tr -d '[:space:]'| cut -d ":" -f2)


ifeq ($(MTK_AIA_ENABLE),y)
CFLAGS += -DMTK_AIA_ENABLE
include $(SDK_PATH)/$(APP_PATH)/3rdparty/module.mk
endif

ifeq ($(MTK_LIGHT_WEIGHT_PRINTF_ENABLED), y)
# Replace Newlib's nano-printf with lwprintf for no-malloc
include $(SOURCE_DIR)/middleware/third_party/lwprintf/module.mk
# wrap printf/sprintf
LDFLAGS += -Wl,-wrap=printf -Wl,-wrap=vprintf -Wl,-wrap=fprintf
LDFLAGS += -Wl,-wrap=fiprintf
LDFLAGS += -Wl,-wrap=sprintf -Wl,-wrap=vsprintf -Wl,-wrap=snprintf -Wl,-wrap=vsnprintf
else
# Use Newlib's nano-printf with floating point support
LDFLAGS += -u _printf_float -u _scanf_float
endif

#######################################################
#   PARAMETERS of common_rule_populate
#
#   $1 stem of binary file name
#   $2 RAM or XIP
#   $4 BT or WF or AU or BW, AL
#######################################################

define common_rule_populate

.PHONY: $1 clean $(MODULE_PATH)

#######################################################
# SIGN BT Firmware
#######################################################

ifeq ($(MTK_BT_SIGN_ENABLE),y)

.PHONY: mtk_bt_firmware_prepare_$(1)

$(1): mtk_bt_firmware_prepare_$(1)

mtk_bt_firmware_prepare_$(1):
	$(Q)make mtk_bt_fw              \
	     MTK_BT_FW_HDR_SZ=0x80      \
	     MTK_BT_FW_ADDR=0x18EC0000  \
	     MTK_BT_FW_SIZE=593920      \
         Q=$(Q)
endif

#######################################################
# SIGN DSP Firmware
#######################################################

ifeq ($(MTK_DSP_SIGN_ENABLE),y)

.PHONY: mtk_dsp_firmware_prepare_$(1)

$(1): mtk_dsp_firmware_prepare_$(1)

mtk_dsp_firmware_prepare_$(1):
	$(Q)make mtk_dsp_fw              \
	     MTK_DSP_FW_HDR_SZ=0x80      \
	     MTK_DSP_FW_ADDR=0x18C41000  \
	     MTK_DSP_FW_SIZE=2617344     \
         Q=$(Q)
endif

#######################################################
# SIGN Wi-Fi Firmware
#######################################################

ifeq ($(MTK_WIFI_SIGN_ENABLE),y)

.PHONY: mtk_wifi_firmware_prepare_$(1)

$(1): mtk_wifi_firmware_prepare_$(1)

mtk_wifi_firmware_prepare_$(1):
	$(Q)make mtk_wifi_firmware              \
	     MTK_WIFI_FIRMWARE_HDR_SZ=0x80      \
	     MTK_WIFI_FIRMWARE_ADDR=0x18F60000  \
	     MTK_WIFI_FIRMWARE_SIZE=651264      \
         Q=$(Q)
endif

#######################################################
# SIGN RTOS Firmware
#######################################################

ifeq ($(MTK_RTOS_SIGN_ENABLE),y)

$(1): $(OUTPATH)/$(1).sgn

$(OUTPATH)/$(1).sgn: $(OUTPATH)/$(1).bin
	$(Q)make sboot_firmware_sign            \
	     SBOOT_FW_HDR_SZ=$(HEADER_SGN_SZ)   \
	     SBOOT_FW_ADDR=$(LOAD_ADDR_XIP)     \
	     SBOOT_FW_SIZE=3145600              \
	     SBOOT_FW_VER=$(SGN_VER)            \
	     SBOOT_FW_IN=$$^                    \
	     SBOOT_FW_OUT=$$@                   \
         Q=$(Q)
endif

ifeq ($(MTK_IOT_SDK_DEMO_AS_LIB), y)
$(1): $(OUTPATH)/libiot_sdk_demo.a
endif

#######################################################
# Scatter file generation
#######################################################

$(1): $(OUTPATH)/$(1)_scatter.ini

ifeq ($(MTK_RTOS_SIGN_ENABLE),y)
$(1)_EXT=sgn
else
$(1)_EXT=bin
endif

$(OUTPATH)/$(1)_scatter.ini: $(OUTPATH)/$(1).$$($(1)_EXT)
	$(Q)$(SCATTER_TOOL_PY) --prj_name    "$(PROJ_NAME)"                 \
	                       --ld_file     "$(PROJ_PATH)/ld/memory.ld"    \
	                       --scatter     "$(OUTPATH)/$(1)_scatter.ini"  \
	                       --bl_fw       "mt7931an_bootloader-xip.sgn"  \
	                       --rtos_fw     "$(1).$$($(1)_EXT)"            \
	                       --wifi_ext    "$(MTK_WIFI_FIRMWARE)"         \
	                       --buff_bin    "$(MTK_WIFI_EFUSE_BUFFER_BIN)" \
	                       --bt_fw       "$(MTK_BT_FW)"                 \
	                       --dsp_fw      "$(MTK_DSP_FW)"                \
			2>>$(ERR_LOG) | tee -a $(BUILD_LOG)

#######################################################

ifneq ($(OVERRIDE_CFLAGS),)
    CFLAG_$1 += $$(OVERRIDE_CFLAGS)
else
    CFLAG_$1 += $$(CFLAGS)
endif


ifeq ($2,XIP)
    LD_SCRIPT_$1=$$(LDFLAGS_XIP)
    CMM_SRC_$1=mt7933_hdk_xip.cmm
    CMM_TGT_$1=$1.cmm
else ifeq ($2,RAM)
    LD_SCRIPT_$1=$$(LDFLAGS_RAM)
    CMM_SRC_$1=mt7933_hdk.cmm
    CMM_TGT_$1=$1.cmm
endif

# this overrides MTK_NVDM_NO_FLASH_ENABLE in feature.mk
ifeq ($2,XIP)
    EXEC_MODEL_FLAG_$1  = -UMTK_NVDM_NO_FLASH_ENABLE -DMTK_XIP_ENABLE
else ifeq ($2,RAM)
    EXEC_MODEL_FLAG_$1  = -DMTK_NVDM_NO_FLASH_ENABLE -UMTK_XIP_ENABLE
endif
CFLAG_$1 += $$(EXEC_MODEL_FLAG_$1)


PACKAGE_FLAG_$1 = -DPINMUX_BGA_DEFAULT  -I$(SOURCE_DIR)/$(APP_PATH)/inc
CFLAG_$1 += $$(PACKAGE_FLAG_$1)


ifeq ($4,BT)
    SUBSYS_FLAG_$1=-DBRINGUP_BT_ENABLE
else ifeq ($4,WF)
    SUBSYS_FLAG_$1=-DBRINGUP_WIFI_ENABLE
else ifeq ($4,AU)
    SUBSYS_FLAG_$1=-DBRINGUP_DSP_ENABLE
else ifeq ($4,BW)
    SUBSYS_FLAG_$1=-DBRINGUP_BT_ENABLE -DBRINGUP_WIFI_ENABLE
else ifeq ($4,AL)
    SUBSYS_FLAG_$1=-DBRINGUP_BT_ENABLE -DBRINGUP_WIFI_ENABLE -DBRINGUP_DSP_ENABLE
endif
CFLAG_$1    += $$(SUBSYS_FLAG_$1)


C_OBJS_$1   := $$(C_FILES:%.c=$(BUILD_DIR)/$1/%.o)
CXX_OBJS_$1 := $$(CXX_FILES:%.cpp=$(BUILD_DIR)/$1/%.o)
S_OBJS_$1   := $$(S_FILES:%.s=$(BUILD_DIR)/$1/%.o)


# verno generation
VERNO_C_FILE_$1 = $(BUILD_DIR)/middleware/MTK/verno/verno.c
$$(VERNO_C_FILE_$1): VERNO_GEN_$1
.PHONY: VERNO_GEN_$1
VERNO_GEN_$1:
	@mkdir -p $(dir $@)
	@$(SOURCE_DIR)/middleware/MTK/verno/gen_verno.sh \
	    $(SOURCE_DIR)/middleware/MTK/verno/verno.template \
	    $@ $(MTK_FW_VERSION) $(BOARD_CONFIG) $(SOURCE_DIR)
C_OBJS_$1   += $$(VERNO_C_FILE_$1:$(BUILD_DIR)/%.c=$(BUILD_DIR)/$1/%.o)

ifneq ($(DaliyBuildTime),)
    CFLAG_$1+=-DDaliyBuildTime=\"$(DaliyBuildTime)\"
endif

C_OBJS      += $$(C_OBJS_$1)
CXX_OBJS    += $$(CXX_OBJS_$1)


LD_OPTS_$1   = $$(LDFLAGS)
LD_OPTS_$1  += $$(LD_SCRIPT_$1)
LD_OPTS_$1  += -Wl,--whole-archive -Wl,--start-group
LD_OPTS_$1  += $$(patsubst $$(SDK_PATH)/%,$$(SOURCE_DIR)/%,$$^)
LD_OPTS_$1  += -Wl,--end-group -Wl,--no-whole-archive
LD_OPTS_$1  += $$(patsubst $$(SDK_PATH)/%,$$(SOURCE_DIR)/%,-Wl,-Map=$$@.map)
LD_OPTS_$1  += -lm
LD_OPTS_$1  += $$(patsubst $$(SDK_PATH)/%,$$(SOURCE_DIR)/%,-o $$@)


$(OUTPATH)/$1.cmm: $$(CMM_SRC_$1)
	@echo "CREATE  $$(notdir $$@)"
	@echo "sed \"s/qfn_evb/$1/\" $$^ > $$@" >> $(BUILD_LOG)
	@sed "s/qfn_evb/$1/" $$^ > $$@

ifeq ($(MTK_IOT_SDK_DEMO_AS_LIB), y)
$(OUTPATH)/libiot_sdk_demo.a: $$(C_OBJS_$1) $$(CXX_OBJS_$1) $$(S_OBJS_$1) $(LIBS)
	$(Q)$(AR) -r $(OUTPATH)/libiot_sdk_demo.a $$(C_OBJS_$1) $$(CXX_OBJS_$1) $$(S_OBJS_$1) >>$(BUILD_LOG) 2>>$(ERR_LOG)
	@if [ -e "$(OUTPATH)/iot_sdk_demo.cflags" ]; then rm -f "$(OUTPATH)/iot_sdk_demo.cflags"; fi
	$(Q)echo '$$(CFLAG_$1)'|sed 's%-I\.%-I$(PWD)/\.%g' > $(OUTPATH)/iot_sdk_demo.cflags
endif

$(OUTPATH)/$1.bin: $(OUTPATH)/$1.elf  $(OUTPATH)/$1.cmm
	@(if [ -e "$$@" ]; then rm -f "$$@"; fi  ; \
	  echo OBJCOPY $$(notdir $$@)            ; \
	  echo $(OBJCOPY) -O binary $$< $$@ 1>&2 ; \
	  $(OBJCOPY) -O binary $$< $$@)            \
			2>>$(ERR_LOG) | tee -a $(BUILD_LOG)


$(OUTPATH)/$1.elf: $$(C_OBJS_$1) $$(CXX_OBJS_$1) $$(S_OBJS_$1) $(LIBS)
	@echo "LINK    $$(notdir $$@)"     | tee -a $(BUILD_LOG)
	@if [ -e "$$@"      ]; then rm -f "$$@";      fi
	@if [ -e "$$@.map"  ]; then rm -f "$$@.map";  fi
	@if [ -e "$$@.hex"  ]; then rm -f "$$@.hex";  fi
	@if [ -e "$$@.opts" ]; then rm -f "$$@.opts"; fi
	@echo $$(patsubst $$(SDK_PATH)/%,$$(SOURCE_DIR)/%,$$(LD_OPTS_$1)) > $$@.opts
	@echo $$(CXX) @$$@.opts >> $(BUILD_LOG); \
	 exec 1> >(tee -a $(BUILD_LOG)) ; \
	 exec 2> >(tee -a $(ERR_LOG))   ; \
	 $(CXX) @$$@.opts


# LTO + XIP, need to exclude some objects for WIFI performance
# Disable LTO if the text of a object is must placed in SYSRAM
ifeq ($(MTK_GCC_LTO_ENABLE)_$2,y_XIP)

$(BUILD_DIR)/$1/%.o: $(SOURCE_DIR)/%.c
	@mkdir -p $$(dir $$@)
	@if [ -e "$$@" ]; then rm -f "$$@"; fi
	@echo $(CC) $$(CFLAG_$1) -c $$< -o $$@ >> $(BUILD_LOG)
	@for i in $(XIP_EXCLUDE_OBJECTS); do \
		if [[ $$@ == $$$$i ]]; then \
			FOUND=1; \
			break; \
		fi \
	done; \
	if [[ "$$$$FOUND" == "1" ]]; then \
		echo "CC      $1 - $$(notdir $$@)" | tee -a $(BUILD_LOG) ; \
		exec 1> >(tee -a $(BUILD_LOG)) ;                           \
		exec 2> >(tee -a $(ERR_LOG))   ;                           \
		$(CC) $$(CFLAG_$1) -fno-lto -fno-fat-lto-objects -c $$< -o $$@ ; \
	else \
		echo "CC      $1 - $$(notdir $$@)" | tee -a $(BUILD_LOG) ; \
		exec 1> >(tee -a $(BUILD_LOG)) ;                           \
		exec 2> >(tee -a $(ERR_LOG))   ;                           \
		$(CC) $$(CFLAG_$1) -c $$< -o $$@ ; \
	fi

else

$(BUILD_DIR)/$1/%.o: $(SOURCE_DIR)/%.c
	@mkdir -p $$(dir $$@)
	@if [ -e "$$@" ]; then rm -f "$$@"; fi
	@echo $(CC) $$(CFLAG_$1) -c $$< -o $$@ >> $(BUILD_LOG)
	@echo "CC      $1 - $$(notdir $$@)" | tee -a $(BUILD_LOG) ; \
	 exec 1> >(tee -a $(BUILD_LOG)) ;                           \
	 exec 2> >(tee -a $(ERR_LOG))   ;                           \
	 $(CC) $$(CFLAG_$1) -c $$< -o $$@

endif

$(BUILD_DIR)/$1/%.d: $(SOURCE_DIR)/%.c
	@mkdir -p $(dir $$@)
	@if [ -e "$$@" ]; then rm -f "$$@"; fi
	@set -e; export B_NAME=`echo $$@ | sed 's/\.d//g'` ; \
	  $(CC) -MM $$(CFLAG_$1) $$< | \
	  sed 's@\(.*\)\.o@'"$$$${B_NAME}\.o $$$${B_NAME}\.d"'@g' > $$@


$(BUILD_DIR)/$1/%.o: $(SOURCE_DIR)/%.s
	@mkdir -p $$(dir $$@)
	@if [ -e "$$@" ]; then rm -f "$$@"; fi
	@echo $(CC) $$(CFLAG_$1) -c $$< -o $$@ >> $(BUILD_LOG)
	@echo "ASM     $1 - $$(notdir $$@)" | tee -a $(BUILD_LOG) ; \
	 exec 1> >(tee -a $(BUILD_LOG)) ;                           \
	 exec 2> >(tee -a $(ERR_LOG))   ;                           \
	 $(CC) $$(CFLAG_$1) -c $$< -o $$@


$(BUILD_DIR)/$1/%.d: $(SOURCE_DIR)/%.s
	@mkdir -p $(dir $$@)
	@if [ -e "$$@" ]; then rm -f "$$@"; fi
	@set -e; export B_NAME=`echo $$@ | sed 's/\.d//g'` ; \
	  $(CC) -MM $$(CFLAG_$1) $$< | \
	  sed 's@\(.*\)\.o@'"$$$${B_NAME}\.o $$$${B_NAME}\.d"'@g' > $$@


endef


#######################################################
# Rules
#######################################################

.PHONY: clean $(MODULE_PATH) $$(O)

.SECONDEXPANSION:
all: cleanlog $$(O)

#######################################################
# image selection
#######################################################

ifeq ($(O),)
    O += $(if $(MT7933CV_XIP_BGA_AL_ENABLE:n=),mt7933cv_xip_bga_al)
endif

#######################################################
# mt7933cv_xip_bga_al
#######################################################

ifneq ($(findstring mt7933cv_xip_bga_al, $(O)),)
$(eval $(call common_rule_populate,mt7933cv_xip_bga_al,XIP,BGA,AL))
endif

#######################################################

MOD_EXTRA = BUILD_DIR=$(BUILD_DIR) OUTPATH=$(OUTPATH) PROJ_PATH=$(PROJ_PATH)

$(LIBS): $(MODULE_PATH)

$(MODULE_PATH):
	@+make -C $@ $(MOD_EXTRA) $($@_EXTRA)

include $(SOURCE_DIR)/.rule.mk

clean:
	@rm -rf $(OUTPATH)

.PHONY: options

options:
	@for opt in `sed 's/[()]/\n/g' $(SOURCE_DIR)/config/chip/$(IC_CONFIG)/chip.mk|sed 's/-D/ /'|sort|uniq|grep ^MTK_|grep _ENABLE$$`; do echo $$opt; done
print-%  : ; @echo $* = $($*)


