PICUS_SRC = middleware/MTK/bt_tools/picus

PICUS_FILES  = $(PICUS_SRC)/picus.c

#CFLAGS += -DPICUS_LOG2FILE
ifeq ($(IC_CONFIG),mt7933)
CFLAGS += -DCHIP_MT7933
CFLAGS += -I$(SOURCE_DIR)/$(PICUS_SRC)
CFLAGS += -I$(SOURCE_DIR)/$(MT7933_BT_DRIVER_DIR)/inc
C_FILES += $(PICUS_SRC)/bt_picus_cli.c
else
ifeq ($(IC_CONFIG),mt8512)
CFLAGS += -DCHIP_MT8512
CFLAGS += -I$(SOURCE_DIR)/driver/chip/$(IC_CONFIG)/src_protected/connectivity/bt/btif/inc
endif
endif

C_FILES += $(PICUS_FILES)

