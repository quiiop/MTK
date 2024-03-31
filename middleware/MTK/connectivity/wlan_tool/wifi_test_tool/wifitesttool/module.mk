#
# Makefile for MT7933
#

TEST_TOOL_CODE = $(TEST_TOOL_DIR)
$(info build WIFI TEST TOOL in $(TEST_TOOL_CODE))
SOURCE_DIR ?= ../../../../..

# definitions
# ------------------------------------------------------------------------------

TESTTOOL_C_FILES += $(TEST_TOOL_CODE)/main.c
TESTTOOL_C_FILES += $(TEST_TOOL_CODE)/lib.c
TESTTOOL_C_FILES += $(TEST_TOOL_CODE)/libwifitest.c
TESTTOOL_C_FILES += $(TEST_TOOL_CODE)/libtbtest.c

C_FILES += $(TESTTOOL_C_FILES)

###################################################
# include path
###################################################
CFLAGS += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/SourceV10/include
#CFLAGS += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS-ext/FreeRTOS-Labs/Source/FreeRTOS-Plus-POSIX/include/
CFLAGS += -I$(SOURCE_DIR)/middleware/third_party/lwip/src/include
CFLAGS += -I$(SOURCE_DIR)/$(TEST_TOOL_CODE)

ifeq ($(MTK_WLAN_SERVICE_ENABLE), y)
CFLAGS += -DMTK_WLAN_SERVICE_ENABLE
CFLAGS += -I$(SOURCE_DIR)/$(WLAN_SERVICE)/include
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WIFI_CODE)/wlan_service/glue/osal/include
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/$(WIFI_CODE)/os/freertos/include
endif

CFLAGS  += -DFREERTOS

# wifitest tool info
#PROGRAM				 = wifitest

#LDFLAGS := -lutil -lm

#CFLAGS  := -DCONFIG_YOCTO_EEPROM_PATH

# compiling and linking
# ------------------------------------------------------------------------------
#all: $(PROGRAM)
#clean:
#	-rm -f $(PROGRAM)
#$(PROGRAM): main.c lib.c libwifitest.c  libtbtest.c
#	$(CC) ${CFLAGS} -Wall -o $@ $< lib.c libwifitest.c libtbtest.c  $(LDFLAGS)
