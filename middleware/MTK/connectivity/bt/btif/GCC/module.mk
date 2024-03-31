ifeq ($(MTK_BT_DRV_CLI_ENABLE),y)
CFLAGS   += -DMTK_BT_DRV_CLI_ENABLE
endif
ifeq ($(MTK_SLT_ENABLE),y)
CFLAGS   += -DMTKBT_DRV_SLT_ENABLE
else
CFLAGS   += -DBT_DRV_BTSNOOP_TO_UART
endif
ifeq ($(MTK_BT_RX_BUF_RECORD), y)
CFLAGS  += -DMTK_BT_RX_BUF_RECORD
endif
ifeq ($(MTK_BT_FW_DL_TO_EMI_ENABLE), y)
CFLAGS += -DMTK_BT_FW_DL_TO_EMI_ENABLE
endif

#If HB support drv vendor layer, we should not use direct call APIs
ifeq ($(MTK_DRV_VND_LAYER), y)
CFLAGS += -DMTK_DRV_VND_LAYER
endif

BT_DRIVER_FILES  = $(MT7933_BT_DRIVER_DIR)/btif/btif_main.c \
			$(MT7933_BT_DRIVER_DIR)/btif/btif_dma.c \
			$(MT7933_BT_DRIVER_DIR)/btif/platform/btif_platform.c \
			$(MT7933_BT_DRIVER_DIR)/btif/bt_driver.c \
			$(MT7933_BT_DRIVER_DIR)/btif/bt_driver_cli.c \
			$(MT7933_BT_DRIVER_DIR)/btif/bt_driver_btsnoop.c \
			$(MT7933_BT_DRIVER_DIR)/btif/btif_util.c

ifeq ($(MTK_BT_BUFFER_BIN_MODE),y)
BT_DRIVER_FILES  += $(MT7933_BT_DRIVER_DIR)/btif/bt_buffer_mode.c
endif

ifeq ($(IC_CONFIG),mt7933)
CFLAGS += -DCHIP_MT7933
BT_DRIVER_FILES +=	$(MT7933_BT_DRIVER_DIR)/btif/platform/btif_mt7933.c
endif

C_FILES += $(BT_DRIVER_FILES)


#################################################################################
#include path
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/inc
CFLAGS  += -I$(SOURCE_DIR)/driver/board/$(BOARD_CONFIG)/inc
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/$(IC_CONFIG)/src/common/include
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/$(IC_CONFIG)/inc
CFLAGS  += -I$(SOURCE_DIR)/kernel/service/inc

CFLAGS += -I$(SOURCE_DIR)/$(MT7933_BT_DRIVER_DIR)/btif
CFLAGS += -I$(SOURCE_DIR)/$(MT7933_BT_DRIVER_DIR)/btif/inc
CFLAGS += -I$(SOURCE_DIR)/$(MT7933_BT_DRIVER_DIR)/btif/platform

#################################################################################
#
# BT F/W filenames are stored in:
# 1. BGA package: MTK_BT_FW_BGA_BIN.
# 2. QFN package: MTK_BT_FW_QFN_BIN.
#
# If digital signature is supported, MTK_BT_SIGN_ENABLE is set to 'y' and
# its filename is stored in MTK_BT_FW_BGA_SGN and MTK_BT_FW_QFN_BIN.
#
# Projects should:
# 1. make recursively using target 'MTK_BT_FW' and provide parameters
#    for firmware preparation:
#
#       make mtk_bt_bga_firmware MTK_BT_FW_HDR_SIZE=<size> \
#                                MTK_BT_FW_ADDR=<addr>     \
#                                MTK_BT_FW_SIZE=<size>     \
#                                MTK_BT_FW_VER=<version>
#
#       make mtk_bt_qfn_firmware MTK_BT_FW_HDR_SIZE=<size> \
#                                MTK_BT_FW_ADDR=<addr>     \
#                                MTK_BT_FW_SIZE=<size>     \
#                                MTK_BT_FW_VER=<version>
#
# 2. use MTK_BT_BGA_FIRMWARE or MTK_BT_QFN_FIRMWARE to generate scatter files.
#
# Depends on MTK_BT_SIGN_ENABLE, the firmware for project could be:
# 1. BT_RAM_CODE_MT7933_1_1_hdr.sgn and BT_RAM_CODE_MT7933_1_1_hdr.sgn, or
# 2. BT_RAM_CODE_MT7933_2_1_hdr.bin and BT_RAM_CODE_MT7933_2_1_hdr.bin
#
# The filename is kept in MTK_BT_BGA_FIRMWARE and MTK_BT_QFN_FIRMWARE for
# projects to use them further.
#
#################################################################################

MTK_BT_FW_VER ?= 1

# Use for generating scatter.ini
# Type: 'dual'    = BT_RAM_CODE_MT7933_1_1_hdr.bin
#                  (Should set MTK_BT_FW_DL_TO_EMI_ENABLE = y)
#       'le_only' = BT_RAM_CODE_MT7933_2_1_hdr.bin
MTK_BT_FW_BIN_TYPE ?= dual

MTK_BT_FW_DUAL_BIN_NAME ?= BT_RAM_CODE_MT7933_1_1_hdr
MTK_BT_FW_LE_BIN_NAME ?= BT_RAM_CODE_MT7933_2_1_hdr

ifeq ($(MTK_BT_FW_BIN_TYPE),dual)
ifneq ($(MTK_BT_FW_DL_TO_EMI_ENABLE),y)
#    $(error "BT driver has to support EMI (MTK_BT_FW_DL_TO_EMI_ENABLE=y) when FW is dual")
endif
endif

MTK_BT_FW_DUAL_BIN = $(OUTPATH)/$(MTK_BT_FW_DUAL_BIN_NAME).bin
MTK_BT_FW_LE_BIN   = $(OUTPATH)/$(MTK_BT_FW_LE_BIN_NAME).bin

ifeq ($(MTK_BT_SIGN_ENABLE),y)
    MTK_BT_FW_DUAL_SGN = $(OUTPATH)/$(MTK_BT_FW_DUAL_BIN_NAME).sgn
    MTK_BT_FW_LE_SGN   = $(OUTPATH)/$(MTK_BT_FW_LE_BIN_NAME).sgn
    MTK_BT_FW_DUAL     = $(MTK_BT_FW_DUAL_BIN_NAME).sgn
    MTK_BT_FW_LE       = $(MTK_BT_FW_LE_BIN_NAME).sgn
else
    MTK_BT_FW_DUAL     = $(MTK_BT_FW_DUAL_BIN_NAME).bin
    MTK_BT_FW_LE       = $(MTK_BT_FW_LE_BIN_NAME).bin
endif


ifeq ($(MTK_BT_FW_BIN_TYPE),dual)
    MTK_BT_FW     = $(MTK_BT_FW_DUAL)
else
    MTK_BT_FW     = $(MTK_BT_FW_LE)
endif


ifeq ($(MTK_BT_SIGN_ENABLE),y)
$(MTK_BT_FW_DUAL_SGN): $(MTK_BT_FW_DUAL_BIN)
	$(Q)make sboot_firmware_sign                    \
	         SBOOT_FW_HDR_SZ=$(MTK_BT_FW_HDR_SZ)    \
	         SBOOT_FW_ADDR=$(MTK_BT_FW_ADDR)        \
	         SBOOT_FW_SIZE=$(MTK_BT_FW_SIZE)        \
	         SBOOT_FW_VER=$(MTK_BT_FW_VER)          \
	         SBOOT_FW_IN=$(MTK_BT_FW_DUAL_BIN)      \
	         SBOOT_FW_OUT=$(MTK_BT_FW_DUAL_SGN)     \
             Q=$(Q)
endif


ifeq ($(MTK_BT_SIGN_ENABLE),y)
$(MTK_BT_FW_LE_SGN): $(MTK_BT_FW_LE_BIN)
	$(Q)make sboot_firmware_sign                    \
	         SBOOT_FW_HDR_SZ=$(MTK_BT_FW_HDR_SZ)    \
	         SBOOT_FW_ADDR=$(MTK_BT_FW_ADDR)        \
	         SBOOT_FW_SIZE=$(MTK_BT_FW_SIZE)        \
	         SBOOT_FW_VER=$(MTK_BT_FW_VER)          \
	         SBOOT_FW_IN=$(MTK_BT_FW_LE_BIN)        \
	         SBOOT_FW_OUT=$(MTK_BT_FW_LE_SGN)       \
             Q=$(Q)
endif


.PHONY: mtk_bt_fw


ifeq ($(MTK_BT_SIGN_ENABLE),y)
mtk_bt_fw: $(MTK_BT_FW_DUAL_SGN) $(MTK_BT_FW_LE_SGN)
else
mtk_bt_fw:
	$(Q)echo BT FW: $(MTK_BT_FW)
endif

