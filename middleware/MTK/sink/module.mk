# BT sink source files
BT_SINK_SRV_SRC = middleware/MTK/sink/src
BT_SINK_SRV_FILES = $(BT_SINK_SRV_SRC)/bt_sink_srv.c \
                    $(BT_SINK_SRV_SRC)/bt_sink_srv_common.c \
                    $(BT_SINK_SRV_SRC)/bt_sink_srv_state_notify.c \
                    $(BT_SINK_SRV_SRC)/bt_sink_srv_utils.c \

# Sink avm_direct related
# BT_SINK_SRV_AVM_DIRECT = middleware/MTK/sink/src/avm_direct
# BT_SINK_SRV_FILES += $(BT_SINK_SRV_AVM_DIRECT)/avm_direct_util.c \

# BT callback manager module
include $(SOURCE_DIR)/middleware/MTK/bt_callback_manager/module.mk

# BT device manager module
include $(SOURCE_DIR)/middleware/MTK/bluetooth_service/bt_device_manager_module.mk

# Audio manager
ifeq ($(AIR_LE_AUDIO_ENABLE), y)
CFLAGS += -DMTK_BT_CODEC_BLE_ENABLED
endif

include $(SOURCE_DIR)/middleware/MTK/audio_manager/module.mk

CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/sink/inc/bt_music
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/atci/inc
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/sink/inc

ifeq ($(MTK_AVM_DIRECT), y)
    CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/bt_codec/inc
endif

ifneq ($(filter y, $(MTK_BT_A2DP_ENABLE) $(MTK_BT_AVRCP_ENABLE)), )
# Sink bt_music related
BT_SINK_SRV_BT_MUSIC_SRC = middleware/MTK/sink/src/bt_music
BT_SINK_SRV_FILES += $(BT_SINK_SRV_BT_MUSIC_SRC)/bt_sink_srv_a2dp.c \
                     $(BT_SINK_SRV_BT_MUSIC_SRC)/bt_sink_srv_a2dp_callback.c \
                     $(BT_SINK_SRV_BT_MUSIC_SRC)/bt_sink_srv_a2dp_state_machine.c \
                     $(BT_SINK_SRV_BT_MUSIC_SRC)/bt_sink_srv_avrcp.c \
                     $(BT_SINK_SRV_BT_MUSIC_SRC)/bt_sink_srv_music.c

# Include bt sink path
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/bluetooth_service/inc
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/sink/inc/call
#CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/bt_sink/inc/audio_command_receiver
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/bluetooth/inc
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/bluetooth/inc
endif #ifeq ($(MTK_BT_DUO_ENABLE), y)

ifeq ($(AIR_LE_AUDIO_ENABLE), y)
BT_SINK_SRV_LE_AUDIO_SRC = middleware/MTK/sink/src/le_audio
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/sink/inc/le_audio
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/bt_bap
BT_SINK_SRV_FILES += $(BT_SINK_SRV_LE_AUDIO_SRC)/bt_sink_srv_le_cap.c\
                     $(BT_SINK_SRV_LE_AUDIO_SRC)/bt_sink_srv_le_cap_audio_manager.c\
                     $(BT_SINK_SRV_LE_AUDIO_SRC)/bt_sink_srv_le_cap_stream.c\
                     $(BT_SINK_SRV_LE_AUDIO_SRC)/bt_sink_srv_le.c\
                     $(BT_SINK_SRV_LE_AUDIO_SRC)/bt_sink_srv_le_music.c\
                     $(BT_SINK_SRV_LE_AUDIO_SRC)/bt_sink_srv_le_call.c\
                     $(BT_SINK_SRV_LE_AUDIO_SRC)/bt_sink_srv_le_volume.c
endif

C_FILES += $(BT_SINK_SRV_FILES)
CFLAGS += -mno-unaligned-access

#ifeq ($(MTK_WAV_DECODER_ENABLE), y)
#CFLAGS += -DMTK_WAV_DECODER_ENABLE
#endif

#ifeq ($(MTK_MP3_DECODER_ENABLED), y)
#CFLAGS += -DMTK_MP3_DECODER_ENABLED
#endif

#ifeq ($(MTK_AUDIO_GAIN_TABLE_ENABLE), y)
#CFLAGS += -DMTK_AUDIO_GAIN_TABLE_ENABLE
#endif

#ifeq ($(MTK_AM_NOT_SUPPORT_STREAM_IN), y)
#CFLAGS += -DMTK_AM_NOT_SUPPORT_STREAM_IN
#endif

# ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/avm_direct_protected/),)
# include $(SOURCE_DIR)/middleware/MTK/avm_direct_protected/GCC/module.mk
# else
# LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/bluetooth/lib/libavm_direct.a
# endif

