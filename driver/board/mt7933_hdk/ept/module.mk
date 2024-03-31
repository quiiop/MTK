
EPT_7933_SRC = driver/board/mt7933_hdk

C_FILES  += $(EPT_7933_SRC)/ept/src/bsp_gpio_ept_config.c

#################################################################################
#include path
CFLAGS 	+= -Iinclude
CFLAGS  += -I$(SOURCE_DIR)/$(EPT_7933_SRC)/ept/inc
CFLAGS  += -I$(SOURCE_DIR)/driver/CMSIS/Device/MTK/mt7933/Include


