
###################################################
# Sources
LITTLEFS_SRC = middleware/third_party/littlefs

C_FILES  += $(LITTLEFS_SRC)/lfs.c
C_FILES  += $(LITTLEFS_SRC)/lfs_util.c

ifeq ($(PRODUCT_VERSION),7933)
C_FILES += $(LITTLEFS_SRC)/portable/mt7933/src/lfs_port.c
C_FILES += $(LITTLEFS_SRC)/portable/mt7933/src/lfs_cli.c
endif


#################################################################################
# include path

CFLAGS 	+= -I$(SOURCE_DIR)/middleware/util/include
CFLAGS 	+= -I$(SOURCE_DIR)/middleware/MTK/minicli/inc
CFLAGS  += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/include
CFLAGS  += -I$(SOURCE_DIR)/kernel/service/inc
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/inc
CFLAGS  += -I$(SOURCE_DIR)/$(LITTLEFS_SRC)

ifeq ($(PRODUCT_VERSION),7933)
CFLAGS  += -I$(SOURCE_DIR)/$(LITTLEFS_SRC)/portable/mt7933/inc
endif

