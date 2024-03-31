###########################################################
## Toolchain
###########################################################
ifeq (,$(ADSP_CROSS_COMPILE))
  XT_TOOLS_VERSION ?= RI-2019.1-linux
  XPLORER_VER := Xplorer-8.0.10
  XTENSA_CORE := hifi4_MockingBird_PROD
  # Configurable XT related paths
  XT_TOOLS := /mtkeda/xtensa/$(XPLORER_VER)/XtDevTools/install/tools/$(XT_TOOLS_VERSION)
  XT_TOOLS_DIR  := $(XT_TOOLS)/XtensaTools
  XT_HWREG_DIR  ?= $(PLATFORM_BASE_DIR)/hwcfg/builds/$(XT_TOOLS_VERSION)/$(XTENSA_CORE)
  XT_LIB_DIR := $(XT_HWREG_DIR)/xtensa-elf/lib
  XTENSA_SYSTEM := $(XT_HWREG_DIR)/config
  XT_TOOLS_BIN_DIR   := $(XT_TOOLS_DIR)/bin
  ADSP_CROSS_COMPILE := $(XT_TOOLS_BIN_DIR)/
endif

CC      := $(ADSP_CROSS_COMPILE)xt-xcc
AS      := $(ADSP_CROSS_COMPILE)xt-xcc
AR      := $(ADSP_CROSS_COMPILE)xt-ar
OBJCOPY := $(ADSP_CROSS_COMPILE)xt-objcopy
OBJDUMP := $(ADSP_CROSS_COMPILE)xt-objdump
DUMPELF := $(ADSP_CROSS_COMPILE)xt-dumpelf
SIZE    := $(ADSP_CROSS_COMPILE)xt-size
STRIP   := $(ADSP_CROSS_COMPILE)xt-strip

HEX2BIN_UTIL := $(TOOLS_DIR)/hifi4tools/hex2bin_B.py
PACK_BIN_NOSIG := $(PLATFORM_DIR)/tools/packtools/package_bin_nosig.py

XTENSA_LICENSE_RETRY_COUNT := 20
XTENSA_LICENSE_RETRY_DELAY := 15
export XTENSA_LICENSE_RETRIES=$(XTENSA_LICENSE_RETRY_COUNT):$(XTENSA_LICENSE_RETRY_DELAY)

#XT_CORE := $(patsubst %-params,%,$(notdir $(shell $(CC) --show-config=core)))

ifeq (1,$(V))
  $(info $(TINYSYS_ADSP): CC=$(CC))
  $(info $(TINYSYS_ADSP): AS=$(AS))
  $(info $(TINYSYS_ADSP): AR=$(AR))
  $(info $(TINYSYS_ADSP): OBJCOPY=$(OBJCOPY))
  $(info $(TINYSYS_ADSP): OBJDUMP=$(OBJDUMP))
  $(info $(TINYSYS_ADSP): DUMPELF=$(DUMPELF))
  $(info $(TINYSYS_ADSP): SIZE=$(SIZE))
  $(info $(TINYSYS_ADSP): STRIP=$(STRIP))
endif

# System settings
XT_SYS = --xtensa-system=$(XTENSA_SYSTEM) --xtensa-core=$(XTENSA_CORE)

COMMON_CFLAGS := -O2 -Os -g -gdwarf-2 -Wall -mno-coproc -mlongcalls -Werror
CFLAGS        += $(COMMON_CFLAGS) -ffunction-sections -fdata-sections $(XT_SYS)

DRAM_RENAME_FLAGS := \
  -mrename-section-.text=.psram.text \
  -mrename-section-.data=.psram.data \
  -mrename-section-.literal=.psram.literal \
  -mrename-section-.rodata=.psram.rodata \
  -mrename-section-.bss=.psram.bss

DRAM_RENAME_LIBFLAGS := \
  $(XT_SYS) -p \
  --rename-section .text=.psram.text \
  --rename-section .data=.psram.data \
  --rename-section .literal=.psram.literal \
  --rename-section .rodata=.psram.rodata \
  --rename-section .bss=.psram.bss

# Define build type. Default to release
BUILD_TYPE ?= release

ifeq (debug,$(strip $(BUILD_TYPE)))
  CFLAGS += -DTINYSYS_DEBUG_BUILD
endif

###########################################################
## DRAM specific resources
###########################################################
NORMAL_SECTION_C_FILES +=

###########################################################
## TCM specific resources
###########################################################
TCM_C_FILES += \
  $(RTOS_SRC_DIR)/portable/ThirdParty/XCC/Xtensa/xtensa_intr.c \
  $(RTOS_SRC_DIR)/portable/ThirdParty/XCC/Xtensa/port.c \
  $(RTOS_SRC_DIR)/list.c \
  $(RTOS_SRC_DIR)/tasks.c \
  $(RTOS_SRC_DIR)/queue.c \
  $(RTOS_SRC_DIR)/timers.c \
  $(RTOS_SRC_DIR)/event_groups.c

TCM_S_FILES += \
  $(RTOS_SRC_DIR)/portable/ThirdParty/XCC/Xtensa/portasm.S \
  $(RTOS_SRC_DIR)/portable/ThirdParty/XCC/Xtensa/xtensa_context.S \
  $(RTOS_SRC_DIR)/portable/ThirdParty/XCC/Xtensa/xtensa_intr_asm.S \
  $(RTOS_SRC_DIR)/portable/ThirdParty/XCC/Xtensa/xtensa_vectors.S

###########################################################
## Common and image specific resources
###########################################################
INCLUDES += \
  $(XT_TOOLS_DIR)/lib/xcc/include \
  $(RTOS_SRC_DIR)/include \
  $(RTOS_SRC_DIR)/portable/ThirdParty/XCC/Xtensa \
  $(RTOS_SRC_DIR)/portable/MemMang \
  $(DRIVERS_COMMON_DIR)/include \
  $(DRIVERS_PLATFORM_DIR)/include

#LDFLAGS += -ltrax -Wl,--gc-sections -Wl,-wrap,printf -Wl,-Map -Wl,$($(PROCESSOR).MAP_FILE)
C_FILES += \
  $(wildcard $(RTOS_SRC_DIR)/*.c) \
  $(RTOS_SRC_DIR)/portable/MemMang/heap_4.c \
  $(wildcard $(RTOS_SRC_DIR)/portable/ThirdParty/XCC/Xtensa/*.c)

C_FILES := $(filter-out $(TCM_C_FILES),$(C_FILES))
C_FILES := $(filter-out $(NORMAL_SECTION_C_FILES),$(C_FILES))

S_FILES += $(wildcard $(RTOS_SRC_DIR)/portable/ThirdParty/XCC/Xtensa/*.S)
S_FILES := $(filter-out $(TCM_S_FILES),$(S_FILES))
