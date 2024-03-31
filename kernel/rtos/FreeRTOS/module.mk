
###################################################
# Sources
ifdef MTK_FREERTOS_VERSION
RTOS_SRC = kernel/rtos/FreeRTOS/Source$(MTK_FREERTOS_VERSION)
else
RTOS_SRC = kernel/rtos/FreeRTOS/Source
endif

RTOS_FILES =	$(RTOS_SRC)/tasks.c \
		$(RTOS_SRC)/list.c \
		$(RTOS_SRC)/queue.c \
		$(RTOS_SRC)/timers.c \
		$(RTOS_SRC)/event_groups.c

ifeq ($(MTK_OS_HEAP_EXTEND),heap5)
CFLAGS     += -DMTK_OS_HEAP_EXTEND=HEAP_EXTEND_HEAP5
RTOS_FILES += $(RTOS_SRC)/portable/MemMang/heap_5.c
else ifeq ($(MTK_OS_HEAP_EXTEND),multi)
CFLAGS     += -DMTK_OS_HEAP_EXTEND=HEAP_EXTEND_MULTI
RTOS_FILES += $(RTOS_SRC)/portable/MemMang/heap_ext.c
else
RTOS_FILES += $(RTOS_SRC)/portable/MemMang/heap_4.c
endif


ifeq ($(MTK_FREERTOS_VERSION),V10)
RTOS_FILES +=	$(RTOS_SRC)/stream_buffer.c
endif


ifeq ($(PRODUCT_VERSION), 7933)
RTOS_FILES +=   $(RTOS_SRC)/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
RTOS_FILES +=   $(RTOS_SRC)/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
RTOS_FILES += 	$(RTOS_SRC)/portable/GCC/mt7933/port_tick.c
CFLAGS     += -I$(SOURCE_DIR)/$(RTOS_SRC)/portable/GCC/mt7933

else ifeq ($(PRODUCT_VERSION),8512)
RTOS_FILES +=   $(RTOS_SRC)/portable/GCC/ARM_CA7/port.c
MTK_PORTABLE_FILES = $(RTOS_SRC)/portable/GCC/ARM_CA7/portASM.s \
                     $(RTOS_SRC)/portable/GCC/ARM_CA7/k_cache_gcc.s

else
RTOS_FILES +=   $(RTOS_SRC)/portable/GCC/ARM_CM4F/port.c
endif


ifeq ($(PRODUCT_VERSION),$(filter $(PRODUCT_VERSION), 7687 7697))
RTOS_FILES +=   $(RTOS_SRC)/portable/GCC/mt7687/port_tick.c
endif

ifeq ($(PRODUCT_VERSION),$(filter $(PRODUCT_VERSION), 7686 7682 5932))
RTOS_FILES += 	$(RTOS_SRC)/portable/GCC/mt7686/port_tick.c
CFLAGS     += -I$(SOURCE_DIR)/$(RTOS_SRC)/portable/GCC/mt7686
endif


C_FILES += $(RTOS_FILES)
S_FILES += $(MTK_PORTABLE_FILES)

###################################################
# include path
ifeq ($(PRODUCT_VERSION),8512)
CFLAGS += -I$(SOURCE_DIR)/$(RTOS_SRC)/include_CA7
else
CFLAGS += -I$(SOURCE_DIR)/$(RTOS_SRC)/include
endif

ifeq ($(PRODUCT_VERSION), 7933)
CFLAGS += -I$(SOURCE_DIR)/$(RTOS_SRC)/portable/GCC/ARM_CM33_NTZ/non_secure
else ifeq ($(PRODUCT_VERSION),8512)
CFLAGS += -I$(SOURCE_DIR)/$(RTOS_SRC)/portable/GCC/ARM_CA7
else
CFLAGS += -I$(SOURCE_DIR)/$(RTOS_SRC)/portable/GCC/ARM_CM4F
endif

CFLAGS += -I$(SOURCE_DIR)/kernel/service/inc

#################################################################################
#Enable the feature by configuring
CFLAGS += -DFREERTOS_ENABLE

