MTK_BT_BOOTS_PATH = middleware/MTK/bt_tools/boots

C_FILES  += $(MTK_BT_BOOTS_PATH)/boots.c
C_FILES  += $(MTK_BT_BOOTS_PATH)/boots_srv.c
C_FILES  += $(MTK_BT_BOOTS_PATH)/boots_pkt.c
C_FILES  += $(MTK_BT_BOOTS_PATH)/boots_uart.c
C_FILES  += $(MTK_BT_BOOTS_PATH)/boots_script.c
C_FILES  += $(MTK_BT_BOOTS_PATH)/bt_boots_cli.c
#################################################################################
#include path
CFLAGS += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/include
CFLAGS += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/include/bits
CFLAGS += -I$(SOURCE_DIR)/kernel/rtos/FreeRTOS/Source/portable/GCC/ARM_CM4F
CFLAGS += -I$(SOURCE_DIR)/kernel/service/inc

CFLAGS += -I$(SOURCE_DIR)/$(MTK_BT_BOOTS_PATH)
CFLAGS += -I$(SOURCE_DIR)/driver/CMSIS/Device/MTK/mt7686/Include
CFLAGS += -I$(SOURCE_DIR)/driver/CMSIS/Include

#Enable the feature by configuring
ifeq ($(MTK_BT_BOOTS_CLI_ENABLE),y)
CFLAGS += -DMTK_BT_BOOTS_CLI_ENABLE
endif
CFLAGS += -DMTK_BT_BOOTS_ENABLE
CFLAGS += -DBTMTK_PLATFORM_MT7933

ifeq ($(MTK_SLT_ENABLE),y)
CFLAGS += -DMTK_BOOTS_SCRIPT_SLT_FULL
CFLAGS += -DMTK_BOOTS_SLT_ENABLE
endif

#Some project may only enable BT slt define, but no MEK_SLT_ENABLE define
ifeq ($(MTK_SLT_BT_EN),y)
CFLAGS += -DMTK_BOOTS_SCRIPT_SLT_FULL
CFLAGS += -DMTK_BOOTS_SLT_ENABLE
endif

#CFLAGS += -DMTK_BOOTS_SCRIPT_LOOPBACK_FULL
