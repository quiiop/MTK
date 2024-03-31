INCLUDES += $(DRIVERS_PLATFORM_DIR)/leaudio/inc
INCLUDES += $(SOURCE_DIR)/../license/prebuilt/HIFI4/mt7933/thirdparty-algo/frauhofer/LC3/include

NORMAL_SECTION_LIBS += $(SOURCE_DIR)/../license/prebuilt/HIFI4/mt7933/thirdparty-algo/frauhofer/LC3/lib/libHIFI4_LC3.a

C_FILES += \
		$(DRIVERS_PLATFORM_DIR)/leaudio/src/audio_leaudio_codec.c \
		$(DRIVERS_PLATFORM_DIR)/leaudio/src/audio_leaudio_hw.c \
		$(DRIVERS_PLATFORM_DIR)/leaudio/src/audio_ringbuf.c \
		$(DRIVERS_PLATFORM_DIR)/leaudio/src/audio_task_leaudio_decode.c \
		$(DRIVERS_PLATFORM_DIR)/leaudio/src/encode.c \
		$(DRIVERS_PLATFORM_DIR)/leaudio/src/encode_mtk_lc3.c \
		$(DRIVERS_PLATFORM_DIR)/leaudio/src/lc3_wrapper.c

#NORMAL_SECTION_C_FILES
#C_FILES
