ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/hifi4dsp_protected/),)
	include $(SOURCE_DIR)/middleware/MTK/hifi4dsp_protected/module.mk
else
	include $(SOURCE_DIR)/prebuilt/middleware/MTK/hifi4dsp/module.mk
endif

HIFI4_DSP_SRC = middleware/MTK/hifi4dsp
CFLAGS   += -DMTK_HIFI4DSP_ENABLE
C_FILES  += $(HIFI4_DSP_SRC)/hifi4dsp_load/src/adsp_clk.c
C_FILES  += $(HIFI4_DSP_SRC)/hifi4dsp_load/src/adsp_debug.c
C_FILES  += $(HIFI4_DSP_SRC)/hifi4dsp_load/src/adsp_helper.c
C_FILES  += $(HIFI4_DSP_SRC)/hifi4dsp_load/src/adsp_mem.c
C_FILES  += $(HIFI4_DSP_SRC)/hifi4dsp_load/src/adsp_plat.c
C_FILES  += $(HIFI4_DSP_SRC)/hifi4dsp_load/src/adsp_sema.c
C_FILES  += $(HIFI4_DSP_SRC)/hifi4dsp_load/src/adsp_wdt.c
C_FILES  += $(HIFI4_DSP_SRC)/hifi4dsp_ipi/src/adsp_ipi.c
C_FILES  += $(HIFI4_DSP_SRC)/hifi4dsp_ipi/src/adsp_ipi_queue.c

#################################################################################
#include path
CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/hifi4dsp/hifi4dsp_load/inc
CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/hifi4dsp/hifi4dsp_ipi/inc

#################################################################################
#
# If digital signature is supported, MTK_DSP_SIGN_ENABLE is set to 'y' and
# its filename is stored in MTK_DSP_FW_SGN.
# (Digital signature is supported only on single F/W bin file)
#
# Projects should:
# 1. make recursively using target 'mtk_wifi_firmware' and provide parameters
#    for firmware preparation:
#       make mtk_dsp_firmware MTK_DSP_FW_HDR_SZ=<size>    \
#                              MTK_DSP_FW_ADDR=<addr>     \
#                              MTK_DSP_FW_SIZE=<size>     \
#                              MTK_DSP_FW_VER=<version>
# 2. use MTK_DSP_FW to generate scatter files.
#
#
# Depends on MTK_DSP_SIGN_ENABLE, the firmware for project could be:
# 1. hifi4dsp_load.sgn, or
# 2. hifi4dsp_load.bin
#
# The filename is kept in MTK_DSP_FW for projects to use it
# further.
#
#################################################################################


MTK_DSP_FW_VER ?= 1

ifeq ($(MTK_DSP_SIGN_ENABLE),y)
MTK_DSP_FW_BIN = $(OUTPATH)/hifi4dsp_load.bin
MTK_DSP_FW_SGN = $(OUTPATH)/hifi4dsp_load.sgn
MTK_DSP_FW     = hifi4dsp_load.sgn
else
MTK_DSP_FW_BIN = $(OUTPATH)/hifi4dsp_load.bin
MTK_DSP_FW     = hifi4dsp_load.bin
endif

ifeq ($(MTK_DSP_SIGN_ENABLE),y)
mtk_dsp_fw:
	$(Q)make sboot_firmware_sign                     \
	         SBOOT_FW_HDR_SZ=$(MTK_DSP_FW_HDR_SZ)    \
	         SBOOT_FW_ADDR=$(MTK_DSP_FW_ADDR)        \
	         SBOOT_FW_SIZE=$(MTK_DSP_FW_SIZE)        \
	         SBOOT_FW_VER=$(MTK_DSP_FW_VER)          \
	         SBOOT_FW_IN=$(MTK_DSP_FW_BIN)           \
	         SBOOT_FW_OUT=$(MTK_DSP_FW_SGN)          \
             Q=$(Q)
endif

