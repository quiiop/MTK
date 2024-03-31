###################################################################
# Global
###################################################################
ifeq ($(PRODUCT_VERSION),7933)
mt_chip_name = mt7933
else ifeq ($(PRODUCT_VERSION),8512)
mt_chip_name = mt8512
endif
AUDIO_SOURCES_PATH := middleware/MTK/audio_services/driver/$(mt_chip_name)
###################################################################
# Source & Include & Define & Lib
###################################################################
include $(SOURCE_DIR)/middleware/MTK/audio_services/driver/$(mt_chip_name)/feature.mk
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/audio_services_protected/sound/),)
    include $(SOURCE_DIR)/middleware/MTK/audio_services_protected/sound/GCC/module.mk
else
    LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio_services/lib/$(PRODUCT_VERSION)/libaudio${LTO_LIB_POST_NAME}.a
endif
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/audio_services_protected/audio_ipi/),)
    include $(SOURCE_DIR)/middleware/MTK/audio_services_protected/audio_ipi/GCC/module.mk
else
    CFLAGS += \
        -I$(SOURCE_DIR)/middleware/MTK/audio_services/audio_ipi/common/framework
    LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio_services/lib/$(PRODUCT_VERSION)/libaudio_ipi${LTO_LIB_POST_NAME}.a
endif
#CFLAGS += -DMTK_CYBERON_VERIFY
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/audio_services_protected/cyberon/),)
    include $(SOURCE_DIR)/middleware/MTK/audio_services_protected/cyberon/GCC/module.mk
else
    CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/audio_services/lib/$(PRODUCT_VERSION)/cyberon/inc
    CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/license/cyberon_license/inc
    LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio_services/lib/$(PRODUCT_VERSION)/libCybDSpotter.a
endif
C_FILES += \
    $(AUDIO_SOURCES_PATH)/test/audio_test.c \
    $(AUDIO_SOURCES_PATH)/test/audio_test_utils.c \
    $(AUDIO_SOURCES_PATH)/test/audio_test_model.c \
    $(AUDIO_SOURCES_PATH)/$(mt_chip_name)/$(mt_chip_name)-afe-pcm.c \
    $(AUDIO_SOURCES_PATH)/$(mt_chip_name)/$(mt_chip_name)-machine.c \
    $(AUDIO_SOURCES_PATH)/$(mt_chip_name)/$(mt_chip_name)-afe-utils.c \
    $(AUDIO_SOURCES_PATH)/$(mt_chip_name)/audio_ipi/audio_ipi_driver.c
ifeq ($(CFG_AUD_CODEC_SUPPORT),y)
    C_FILES += $(AUDIO_SOURCES_PATH)/$(mt_chip_name)/$(mt_chip_name)-codec.c
    CFLAGS += -DAUD_CODEC_SUPPORT
endif
CFLAGS  += \
    -I$(SOURCE_DIR)/middleware/MTK/audio_services/driver/$(mt_chip_name)/common \
    -I$(SOURCE_DIR)/middleware/MTK/audio_services/driver/$(mt_chip_name)/test \
    -I$(SOURCE_DIR)/middleware/MTK/audio_services \
    -I$(SOURCE_DIR)/middleware/MTK/audio_services/sound/utils/include \
    -I$(SOURCE_DIR)/middleware/MTK/audio_services/sound/include \
    -I$(SOURCE_DIR)/middleware/MTK/audio_services/model \
    -I$(SOURCE_DIR)/driver/chip/$(mt_chip_name)/inc
ifeq ($(MTK_HIFI4DSP_ENABLE), y)
    ifeq ($(CFG_MTK_AUDIODSP_SUPPORT),y)
        C_FILES += \
            $(AUDIO_SOURCES_PATH)/$(mt_chip_name)/$(mt_chip_name)-adsp-pcm.c \
            $(AUDIO_SOURCES_PATH)/$(mt_chip_name)/$(mt_chip_name)-adsp-utils.c
    ifeq ($(CFG_VA_TEST_SUPPORT),y)
        C_FILES += \
            $(AUDIO_SOURCES_PATH)/test/va.c \
            $(AUDIO_SOURCES_PATH)/test/va_golden_tone.c
    endif #CFG_VA_TEST_SUPPORT
    ifeq ($(CFG_WRAP_PCM_SUPPORT),y)
        C_FILES += $(AUDIO_SOURCES_PATH)/$(mt_chip_name)/wrap_dsp_msg.c
    endif #CFG_WRAP_PCM_SUPPORT
    endif #CFG_MTK_AUDIODSP_SUPPORT
endif #MTK_HIFI4DSP_ENABLE
