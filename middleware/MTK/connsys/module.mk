
CONNSYS_DIR = middleware/MTK/connsys


ifeq ($(MTK_MT7933_CONSYS_ENABLE), y)
C_FILES  += $(CONNSYS_DIR)/src/mt7933_pos.c
C_FILES  += $(CONNSYS_DIR)/src/mt7933_connsys_dbg.c
CFLAGS 	 += -I$(SOURCE_DIR)/$(CONNSYS_DIR)/inc
endif

ifeq ($(MTK_MT7933_CONSYS_ENABLE), y)
C_FILES  += $(CONNSYS_DIR)/src/mt7933_lp.c
endif
