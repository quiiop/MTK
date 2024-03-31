###################################################
DSPFW_PATH = $(DRIVERS_COMMON_DIR)/audio/tasks/playback/dspfw

CFLAGS += -DPRELOADER_ENABLE

ifeq ($(CFG_PROMPT_SOUND_ENABLE),yes)
CFLAGS += -DMTK_PROMPT_SOUND_ENABLE
endif

NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_scenario.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_sdk.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_update_entry_list.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_audio_msg_task.c

ifeq ($(CFG_AUDIO_HARDWARE_ENABLE),yes)
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_drv/dsp_drv_afe.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_drv/dsp_drv_dfe.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_drv/audio_afe_common.c
ifeq ($(AIR_I2S_SLAVE_ENABLE),y)
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_drv/audio_i2s_slave.c
endif
endif

NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_interface/dsp_audio_ctrl.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_interface/dsp_memory.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_interface/dsp_buffer.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_interface/dsp_callback.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_interface/dsp_stream_connect.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_interface/dsp_audio_process.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_interface/dsp_feature_interface.c
ifeq ($(CFG_DSP_GAIN_CONTROL_ENABLE),yes)
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_interface/dsp_gain_control.c
endif
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_interface/dsp_update_para.c

NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_stream/stream_audio_driver.c
ifeq ($(CFG_AUDIO_HARDWARE_ENABLE),yes)
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_stream/stream_audio_hardware.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_lower_layer/dsp_stream/stream_audio_setting.c
endif

NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_upper_layer/davt/davt.c
#NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_upper_layer/dhpt/dhpt.c
#NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_upper_layer/dprt/dprt.c
NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/dsp_upper_layer/dsp_task/dsp_task.c

NORMAL_SECTION_C_FILES += $(DSPFW_PATH)/src/utility/dlist.c

###################################################
# include path
INCLUDES += $(DRIVERS_COMMON_DIR)/audio/tasks/playback/stream/inc

INCLUDES += $(DSPFW_PATH)/inc
INCLUDES += $(DSPFW_PATH)/inc/dsp
INCLUDES += $(DSPFW_PATH)/inc/voice_plc
INCLUDES += $(DSPFW_PATH)/inc/system

INCLUDES += $(DSPFW_PATH)/inc/dsp_lower_layer/dsp_drv
INCLUDES += $(DSPFW_PATH)/inc/dsp_lower_layer/dsp_interface
INCLUDES += $(DSPFW_PATH)/inc/dsp_lower_layer/dsp_interface/nvdm
INCLUDES += $(DSPFW_PATH)/inc/dsp_lower_layer/dsp_stream
INCLUDES += $(DSPFW_PATH)/inc/dsp_lower_layer/dtm
INCLUDES += $(DSPFW_PATH)/inc/dsp_upper_layer

INCLUDES += $(DSPFW_PATH)/inc/utility
