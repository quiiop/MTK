
# SDK_PATH and APP_PATH are inherited from project Makefile.

PROJECT_CFLAGS     += $(subst -I..,-I$(SDK_PATH)/$(APP_PATH)/GCC/..,$(CFLAGS))
PROJECT_CFLAGS     += -I$(SDK_PATH)/kernel/rtos/FreeRTOS-ext/FreeRTOS-Labs/Source/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/include
PROJECT_CFLAGS     += -I$(SDK_PATH)/kernel/rtos/FreeRTOS-ext/FreeRTOS-Labs/Source/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/source
PROJECT_CFLAGS     += -I$(SDK_PATH)/kernel/rtos/FreeRTOS-ext/FreeRTOS-Labs/Source/FreeRTOS-Plus-POSIX/include

PROJECT_CFLAGS     += -I$(SDK_PATH)/middleware/MTK/audio_services/sound/include
PROJECT_CFLAGS     += -L$(SDK_PATH)/prebuilt/middleware/MTK/audio_services/lib/7933/
PROJECT_CFLAGS     += -Wno-error

# source

THIRD_PARTY_DIR    := $(SDK_PATH)/$(APP_PATH)/3rdparty

# destination

OUT_DIR            := $(OUT)/third_party
INSTALL_PREFIX     := $(OUT_DIR)/install

# 3rdparty libraries

LIBS_3RDPARTY += $(INSTALL_PREFIX)/lib/libopus.a
LIBS_3RDPARTY += $(INSTALL_PREFIX)/lib/libportaudio.a

.PHONY: third_party
$(LIBS_3RDPARTY): third_party

third_party:
	mkdir -p $(OUT_DIR) &&                                                   \
	mkdir -p $(OUT)/third_party/install &&                                   \
	cd $(OUT)/third_party &&                                                 \
	cmake -DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX)                           \
	      -DCMAKE_TOOLCHAIN_FILE=$(THIRD_PARTY_DIR)/freertos-armv8m-cross.mk \
	      -DPROJECT_CFLAGS="$(PROJECT_CFLAGS)"                               \
	      -DSDK_PATH="$(SDK_PATH)"                                           \
	      $(THIRD_PARTY_DIR) &&                                              \
	make &&                                                                  \
	make install

# add 3rdparty libraries to project's link flag.

LIBS += $(LIBS_3RDPARTY)
LDFLAGS += -L$(INSTALL_PREFIX)/lib/libopus.a
LDFLAGS += -L$(INSTALL_PREFIX)/lib/libportaudio.a
