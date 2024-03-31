# Copyright Statement:
#
# (C) 2022-2022  MediaTek Inc. All rights reserved.
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

SOURCE_DIR      = ../../../../..
BINPATH         = ~/gcc-arm-none-eabi/bin
DATIME          = $(shell date --iso=seconds)
SDK_PATH        = $(abspath $(PWD)/$(SOURCE_DIR))
PWD             = $(shell pwd)
FEATURE        ?= feature.mk
HAL_FEATURE    ?= hal_feature.mk
DEBUG           = 0
FLOAT_TYPE      = hard
BUILD_DIR       = $(PWD)/build
SHELL           = /bin/bash

# Project name
PROJ_NAME       = $(shell basename $(dir $(PWD)))
PROJ_PATH       = $(abspath $(dir $(PWD)))/GCC
OUTPATH         = $(PWD)/build
SCATTER_TOOL_PY = $(SDK_PATH)/tools/scripts/get_scatter/get_scatter_arg_v2.py

LDFLAGS         = $(ALLFLAGS) $(FPUFLAGS) --specs=nano.specs -lnosys -nostartfiles
LDFLAGS        += -Wl,--gc-sections
LDFLAGS        += -Wl,-wrap=malloc -Wl,-wrap=calloc -Wl,-wrap=realloc -Wl,-wrap=free
LDFLAGS        += -u _printf_float -u _scanf_float

#######################################################
# Include Module

include $(FEATURE)
include $(HAL_FEATURE)

LDFLAGS     += -L$(PROJ_PATH)/ld
LDFLAGS_XIP += -Wl,-Tmt7933_flash.ld

# Gloabl Config
-include $(SOURCE_DIR)/.config
# IC Config
-include $(SOURCE_DIR)/config/chip/$(IC_CONFIG)/chip.mk
# Board Config
-include $(SOURCE_DIR)/config/board/$(BOARD_CONFIG)/board.mk

include $(SOURCE_DIR)/driver/board/mt7933_hdk/ept/module.mk

CFLAGS += -Werror

ifeq ($(MTK_HAL_SPM_LOW_POWER_SUPPORT), y)
CFLAGS += -mcmse
endif

export IC_CONFIG
export BORAD_CONFIG
export PROJ_NAME
export PROJ_PATH

# WiFi CONNAC driver files
ifeq ($(MTK_MT7933_CONSYS_WIFI_ENABLE), y)
ifeq ($(MTK_WLAN_SERVICE_ENABLE), y)
WLAN_SERVICE_DIR = middleware/MTK/connectivity/wlan_service
endif
WF_DRIVER_DIR = middleware/MTK/connectivity/wlan
export WF_DRIVER_DIR
include $(SOURCE_DIR)/$(WF_DRIVER_DIR)/GCC/module.mk
GEN4M_H_FILES      = $(shell find $(SOURCE_DIR)/$(WF_DRIVER_DIR)/*/  -name *.h | grep linux -v)
GEN4M_H_FILES_PATH = $(subst $(SOURCE_DIR)/$(WF_DRIVER_DIR)/,,$(dir $(GEN4M_H_FILES)))
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
CFLAGS += -D__BT_MULTI_ADV__
CFLAGS += -D__BT_HB_DUO__
include $(SOURCE_DIR)/middleware/MTK/bluetooth/module.mk
include $(SOURCE_DIR)/middleware/MTK/bt_callback_manager/module.mk
endif

# BT mesh middleware
ifeq ($(MTK_BT_MESH_ENABLE), y)
include $(SOURCE_DIR)/middleware/MTK/mesh/module.mk
include $(SOURCE_DIR)/middleware/third_party/micro_ecc/module.mk
include $(SOURCE_DIR)/middleware/third_party/mbedtls/module.mk
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

# BT fw assert recovery
ifeq ($(MTK_BT_SUPPORT_FW_ASSERT_RECOVERY),y)
CFLAGS += -DMTK_BT_SUPPORT_FW_ASSERT_RECOVERY
endif

# BT gatt bas service
ifeq ($(MTK_BT_BAS_SERVICE_ENABLE),y)
CFLAGS += -DMTK_BT_BAS_SERVICE_ENABLE
include $(SOURCE_DIR)/middleware/MTK/ble_bas/module.mk
endif

# minisupp
ifeq ($(MTK_MINISUPP_ENABLE),y)
MID_MINISUPP_PATH = $(SOURCE_DIR)/middleware/MTK/minisupp
include $(SOURCE_DIR)/middleware/MTK/minisupp/module.mk
CFLAGS += -DMTK_MINISUPP_ENABLE
$(info build minisupp)
endif

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

#include dhcpd
ifeq ($(MTK_WIFI_AP_ENABLE),y)
include $(SOURCE_DIR)/middleware/MTK/dhcpd/module.mk
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

ifeq ($(MTK_FOTA_V3_ENABLE),y)
    include $(SOURCE_DIR)/middleware/MTK/fota/module.mk
endif

#######################################################
# Main APP files

# Main APP files
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
APP_FILES      += $(APP_PATH_SRC)/low_pwr.c
APP_FILES      += $(APP_PATH)/GCC/syscalls.c
APP_FILES      += $(APP_PATH_SRC)/network_default_config.c

ifeq ($(MTK_NVDM_ENABLE),y)
APP_FILES      += $(APP_PATH_SRC)/nvdm_config.c
APP_FILES      += $(APP_PATH_SRC)/platform_default_config.c
APP_FILES      += $(APP_PATH_SRC)/wifi_cfg_default_config.c
endif

ifeq ($(MTK_BT_ENABLE),y)
APP_FILES      += $(APP_PATH_SRC)/bt_app/connection_info.c
APP_FILES      += $(APP_PATH_SRC)/bt_app/gatt_command.c
APP_FILES      += $(APP_PATH_SRC)/bt_app/gatt_service.c
APP_FILES      += $(APP_PATH_SRC)/bt_app/ut_app.c
APP_FILES      += $(APP_PATH_SRC)/bt_app/ut_app_gap.c
APP_FILES      += $(APP_PATH_SRC)/bt_app/ut_app_gatts.c
APP_FILES      += $(APP_PATH_SRC)/bt_app/ut_app_gattc.c
APP_FILES      += $(APP_PATH_SRC)/bt_init.c
APP_FILES      += $(APP_PATH_SRC)/ble_smtcn.c
APP_FILES      += $(APP_PATH_SRC)/hci_log.c
APP_FILES      += $(APP_PATH_SRC)/bt_setting.c
endif

ifeq ($(MTK_FOTA_V3_ENABLE),y)
APP_FILES      += $(APP_PATH_SRC)/fota_flash_config.c
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

###################################################
# MCUBOOT sign settings
###################################################

SGN_PEM ?= $(SDK_PATH)/$(APP_PATH)/mtk-dev.pem

ifeq ($(MTK_RTOS_SIGN_ENABLE),y)
  SGN_VER        = 0.0.1
  HEADER_SGN_SZ := 0x80
  LOAD_ADDR     := 0x18044080
endif

DaliyBuildTime := $(shell cd $(SOURCE_DIR)/prebuilt/middleware/MTK/minicli && git log -1 --date=format:'%Y_%m_%d_%H_%M_%S' | grep Date| tr -d '[:space:]'| cut -d ":" -f2)

define common_rule_populate

.PHONY: mt7933cv_xip_bga_al $(MODULE_PATH)

ifeq ($(MTK_RTOS_SIGN_ENABLE),y)

mt7933cv_xip_bga_al: $(OUTPATH)/mt7933cv_xip_bga_al.sgn $(OUTPATH)/scatter.ini

$(OUTPATH)/scatter.ini: $(OUTPATH)/mt7933cv_xip_bga_al.sgn
	$(Q)$(SCATTER_TOOL_PY) --prj_name    "$(PROJ_NAME)"                 \
	                       --ld_file     "$(PROJ_PATH)/ld/memory.ld"    \
	                       --scatter     "$(OUTPATH)/$(1)_scatter.ini"  \
	                       --bl_fw       "mt7931an_bootloader-xip.sgn"  \
	                       --rtos_fw     "mt7933cv_xip_bga_al.sgn"      \
	                       --wifi_ext    "$(MTK_WIFI_FIRMWARE)"         \
	                       --buff_bin    "$(MTK_WIFI_EFUSE_BUFFER_BIN)" \
	                       --bt_fw       "$(MTK_BT_FW)"                 \
	                       --dsp_fw      "$(MTK_DSP_FW)"
else
mt7933cv_xip_bga_al: $(OUTPATH)/mt7933cv_xip_bga_al.bin $(OUTPATH)/scatter.ini
$(OUTPATH)/scatter.ini: $(OUTPATH)/mt7933cv_xip_bga_al.bin
	$(Q)$(SCATTER_TOOL_PY) --prj_name    "$(PROJ_NAME)"                 \
	                       --ld_file     "$(PROJ_PATH)/ld/memory.ld"    \
	                       --scatter     "$(OUTPATH)/$(1)_scatter.ini"  \
	                       --bl_fw       "mt7931an_bootloader-xip.sgn"  \
	                       --rtos_fw     "mt7933cv_xip_bga_al.bin"      \
	                       --wifi_ext    "$(MTK_WIFI_FIRMWARE)"         \
	                       --buff_bin    "$(MTK_WIFI_EFUSE_BUFFER_BIN)" \
	                       --bt_fw       "$(MTK_BT_FW)"                 \
	                       --dsp_fw      "$(MTK_DSP_FW)"
endif

ifneq ($(OVERRIDE_CFLAGS),)
    C_FLAGS += $(OVERRIDE_CFLAGS)
else
    C_FLAGS += $(CFLAGS)
endif

SCT_OPT=$(LDFLAGS_XIP)

CMM_SRC =mt7933_hdk_xip.cmm
CMM_TGT =mt7933cv_xip_bga_al.cmm

# this overrides MTK_NVDM_NO_FLASH_ENABLE in feature.mk
EXEC_MODEL_FLAG_mt7933cv_xip_bga_al  = -UMTK_NVDM_NO_FLASH_ENABLE -DMTK_XIP_ENABLE
C_FLAGS += $(EXEC_MODEL_FLAG_mt7933cv_xip_bga_al)


PACKAGE_FLAG_mt7933cv_xip_bga_al = -DPINMUX_BGA_DEFAULT  -I$(SOURCE_DIR)/$(APP_PATH)/inc
C_FLAGS += $(PACKAGE_FLAG_mt7933cv_xip_bga_al)

C_DEFINES += -DBRINGUP_BT_ENABLE
C_DEFINES += -DBRINGUP_WIFI_ENABLE

C_FLAGS    += $(C_DEFINES)

C_OBJS   := $(C_FILES:%.c=$(BUILD_DIR)/mt7933cv_xip_bga_al/%.o)
CXX_OBJS := $(CXX_FILES:%.cpp=$(BUILD_DIR)/mt7933cv_xip_bga_al/%.o)
S_OBJS   := $(S_FILES:%.s=$(BUILD_DIR)/mt7933cv_xip_bga_al/%.o)

# verno generation
VERNO_C_FILE_mt7933cv_xip_bga_al = $(BUILD_DIR)/middleware/MTK/verno/verno.c
$(VERNO_C_FILE_mt7933cv_xip_bga_al): VERNO_GEN_mt7933cv_xip_bga_al
.PHONY: VERNO_GEN_mt7933cv_xip_bga_al
VERNO_GEN_mt7933cv_xip_bga_al:
	@mkdir -p $(dir $@)
	@$(SOURCE_DIR)/middleware/MTK/verno/gen_verno.sh \
	    $(SOURCE_DIR)/middleware/MTK/verno/verno.template \
	    $@ $(MTK_FW_VERSION) $(BOARD_CONFIG) $(SOURCE_DIR)
C_OBJS   += $(VERNO_C_FILE_mt7933cv_xip_bga_al:$(BUILD_DIR)/%.c=$(BUILD_DIR)/mt7933cv_xip_bga_al/%.o)

ifneq ($(DaliyBuildTime),)
    C_FLAGS+=-DDaliyBuildTime=\"$(DaliyBuildTime)\"
endif

C_OBJS      += $(C_OBJS)
CXX_OBJS    += $(CXX_OBJS)

LD_OPTS   = $(LDFLAGS)
LD_OPTS  += $(SCT_OPT)
LD_OPTS  += -Wl,--whole-archive -Wl,--start-group
LD_OPTS  += $(patsubst $(SDK_PATH)/%,$(SOURCE_DIR)/%,$^)
LD_OPTS  += -Wl,--end-group -Wl,--no-whole-archive
LD_OPTS  += $(patsubst $(SDK_PATH)/%,$(SOURCE_DIR)/%,-Wl,-Map=$@.map)
LD_OPTS  += -lm
LD_OPTS  += $(patsubst $(SDK_PATH)/%,$(SOURCE_DIR)/%,-o $@)


$(OUTPATH)/$(CMM_TGT): $(CMM_SRC)
	@sed "s/qfn_evb/mt7933cv_xip_bga_al/" $^ > $@


$(OUTPATH)/mt7933cv_xip_bga_al.bin: $(OUTPATH)/mt7933cv_xip_bga_al.elf  $(OUTPATH)/$(CMM_TGT)
	@$(OBJCOPY) -O binary $< $@)


$(OUTPATH)/mt7933cv_xip_bga_al.elf: $(C_OBJS) $(CXX_OBJS) $(S_OBJS) $(LIBS)
	@echo $(patsubst $(SDK_PATH)/%,$(SOURCE_DIR)/%,$(LD_OPTS)) > $@.opts
	@$(CXX) @$@.opts


$(BUILD_DIR)/mt7933cv_xip_bga_al/%.o: $(SOURCE_DIR)/%.c
	@$(CC) $(C_FLAGS) -c $< -o $@


$(BUILD_DIR)/mt7933cv_xip_bga_al/%.o: $(SOURCE_DIR)/%.s
	@$(CC) $(C_FLAGS) -c $< -o $@


endef


#######################################################
# Targets
#######################################################


.PHONY: clean $(MODULE_PATH) sdk_export

all: sdk_export

clean:
	rm -rf $(OUTPATH)


#######################################################
# mt7933cv_xip_bga_al
#######################################################


.PHONY: gn_gen_new
gn_gen_new:
	echo > $(OUTPATH)/.c_opts
	echo > $(OUTPATH)/.cxx_opts
	echo > $(OUTPATH)/.s_opts
	echo > $(OUTPATH)/.ld_opts

.PHONY: gn_check_new
gn_check_new: gn_gen_new
	if 

sdk_export: gn_check_new
	if 

$(eval $(call common_rule_populate,mt7933cv_xip_bga_al))


#######################################################

MOD_EXTRA = BUILD_DIR=$(BUILD_DIR) OUTPATH=$(OUTPATH) PROJ_PATH=$(PROJ_PATH)

$(LIBS): $(MODULE_PATH)

$(MODULE_PATH):
	@+make -C $@ $(MOD_EXTRA) $($@_EXTRA)

include $(SOURCE_DIR)/.rule.mk

clean:
	@rm -rf $(OUTPATH)

