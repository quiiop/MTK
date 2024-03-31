###################################################
STREAM_PATH = $(DRIVERS_COMMON_DIR)/audio/tasks/playback/stream

NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/source.c
NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/sink.c
NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream.c
NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/transform.c

ifeq ($(CFG_AUDIO_HARDWARE_ENABLE),yes)
NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_audio.c
NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_audio_afe.c
endif

ifeq ($(MTK_BT_A2DP_ENABLE), yes)
    ifeq ($(MTK_BT_AVM_SHARE_BUF), yes)
        NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_avm_a2dp.c
    else
        NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_n9_a2dp.c
    endif
endif

ifeq ($(MTK_BT_HFP_ENABLE), yes)
    ifeq ($(MTK_BT_HFP_FORWARDER_ENABLE), yes)
        NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_forwarder_sco.c
    else
        NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_n9sco.c
    endif
endif

ifeq ($(AIR_BT_CODEC_BLE_ENABLED), yes)
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_n9ble.c
endif

ifeq ($(CFG_CM4_PLAYBACK_ENABLE), yes)
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_cm4_playback.c
endif

ifeq ($(MTK_CM4_RECORD_ENABLE), yes)
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_cm4_record.c
endif

ifeq ($(CFG_PROMPT_SOUND_ENABLE), yes)
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_cm4_vp_playback.c
endif

ifeq ($(AIR_PROMPT_SOUND_DUMMY_SOURCE_ENABLE), yes)
    CCFLAG += -DAIR_PROMPT_SOUND_DUMMY_SOURCE_ENABLE
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_cm4_vp_dummy_source_playback.c
endif

ifeq ($(MTK_SENSOR_SOURCE_ENABLE), yes)
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_gsensor_detect.c
endif

ifeq ($(MTK_AUDIO_TRANSMITTER_ENABLE), yes)
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_audio_transmitter.c
endif

ifeq ($(MTK_AUDIO_BT_COMMON_ENABLE), y)
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/stream_bt_common.c
endif

ifeq ($(AIR_GAMING_MODE_DONGLE_ENABLE), yes)
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/audio_transmitter_scenario_port/scenario_ull_audio.c
endif

ifeq ($(AIR_WIRED_AUDIO_ENABLE), yes)
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/audio_transmitter_scenario_port/scenario_wired_audio.c
endif

ifeq ($(AIR_BLE_AUDIO_DONGLE_ENABLE), yes)
    NORMAL_SECTION_C_FILES += $(STREAM_PATH)/src/stream_interface/audio_transmitter_scenario_port/scenario_ble_audio.c
endif

###################################################
# include path
INCLUDES += $(STREAM_PATH)/inc
INCLUDES += $(DRIVERS_COMMON_DIR)/audio/tasks/playback/dspfw/inc
