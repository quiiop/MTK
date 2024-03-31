###################################################################
# Audio Feature Option Configuration
###################################################################
CFG_MTK_AUDIODSP_SUPPORT       = y
CFG_WRAP_CAPTURE_SUPPORT       = n
CFG_FILE_SYS_SUPPORT           = y
CFG_DSP_BOOT_RUN               = n
CFG_VA_TEST_SUPPORT            = y
CFG_VA_TASK_SUPPORT            = y
CFG_RECORD_TASK_SUPPORT        = n
CFG_IPC_SUPPORT                = y
CFG_GPIO_SUPPORT               = y
CFG_DRV_MUSIC_DATA_EN          = n
CFG_DSP_DEBUG_DUMP             = y
CFG_AUDIO_SUSPEND_EN           = y
ifeq ($(MTK_HIFI4DSP_BT_AUDIO_ENABLE),)
CFG_DSPOTTER_SUPPORT           = y
CFG_DSPOTTER_BASE_VERSION      = 0 #0: hey siri  1:zhinengguanjia  2:Hello Aircon
endif
ifeq ($(MTK_MT7933_AUDIO_CODEC_ENABLE),n)
CFG_AUD_CODEC_SUPPORT          = n
else
CFG_AUD_CODEC_SUPPORT          = y
endif
###################################################################
# Audio SYSTEM Option Configuration
###################################################################
CFG_AUDIO_FREERTOS_SUPPORT     = y
###################################################################
# Portable Feature Option Configuration
###################################################################
PORTABLE_FREERTOS_POSIX_SUPPORT     = y
###################################################################
# CFLAGS
###################################################################
# AUDIO AFE SRAM 64KB (48KB for DSP, 16KB for AFE DMA)
CFLAGS += -DAFE_SRAM_BASE=0x4008C000
CFLAGS += -DAFE_SRAM_SIZE=0x00004000
CFLAGS += -DCFG_ALIGNMENT_BYTES=0x20
ifeq ($(CFG_AUDIO_FREERTOS_SUPPORT),y)
    CFLAGS += -DAUDIO_FREERTOS_SUPPORT
ifeq ($(PORTABLE_FREERTOS_POSIX_SUPPORT),y)
    CFLAGS += -DFREERTOS_POSIX_SUPPORT
    CFLAGS += -DSND_THREAD_SAFE_API
endif #PORTABLE_FREERTOS_POSIX_SUPPORT
endif #CFG_AUDIO_FREERTOS_SUPPORT
ifeq ($(MTK_HIFI4DSP_ENABLE), y)
    ifeq ($(CFG_MTK_AUDIODSP_SUPPORT),y)
        CFLAGS += -DCONFIG_MTK_ADSP_SUPPORT
        ifeq ($(CFG_VA_TEST_SUPPORT),y)
            CFLAGS += -DVA_TEST_SUPPORT
            CFLAGS += -DCFG_DSPOTTER_BASE_VERSION=2 #0: hey siri  1:zhinengguanjia  2:Hello Aircon
            CFLAGS += -DCFG_ALARM_TONE_VERSION=2 #0: Dingdone 1: nihao,shoudao  2: imhere,gotit
        endif #CFG_VA_TEST_SUPPORT
        ifeq ($(CFG_DSP_BOOT_RUN),y)
            CFLAGS += -DDSP_BOOT_RUN
        endif #CFG_DSP_BOOT_RUN
        ifeq ($(CFG_VA_TASK_SUPPORT),y)
            CFLAGS += -DCFG_VA_TASK_SUPPORT
        endif #CFG_VA_TASK_SUPPORT
        ifeq ($(CFG_RECORD_TASK_SUPPORT),y)
            CFLAGS += -DCFG_RECORD_TASK_SUPPORT
        endif #CFG_RECORD_TASK_SUPPORT
        ifeq ($(CFG_WRAP_PCM_SUPPORT),y)
            CFLAGS += -DCFG_WRAP_PCM_SUPPORT
        endif #CFG_WRAP_PCM_SUPPORT
    endif #CFG_MTK_AUDIODSP_SUPPORT
    ifeq ($(CFG_IPC_SUPPORT),y)
        CFLAGS += -DCFG_IPC_SUPPORT
    endif #CFG_IPC_SUPPORT
endif #MTK_HIFI4DSP_ENABLE
ifeq ($(CFG_FILE_SYS_SUPPORT),y)
    CFLAGS += -DFILE_SYS_SUPPORT
endif #CFG_FILE_SYS_SUPPORT
ifeq ($(CFG_DRV_MUSIC_DATA_EN),y)
    CFLAGS += -DCFG_DRV_MUSIC_DATA_EN
endif #CFG_DRV_MUSIC_DATA_EN
ifeq ($(CFG_GPIO_SUPPORT),y)
    CFLAGS += -DCFG_GPIO_SUPPORT
endif #CFG_GPIO_SUPPORT
ifeq ($(CFG_DSP_DEBUG_DUMP),y)
    CFLAGS += -DDSP_DEBUG_DUMP
endif #CFG_DSP_DEBUG_DUMP
ifeq ($(PROJ_NAME),iot_sdk_demo)
    CFLAGS += -DIOT_SDK_DEMO_PROJECT
else ifeq ($(PROJ_NAME),bga_sdk_demo)
CFLAGS += -DIOT_SDK_DEMO_PROJECT
else ifeq ($(PROJ_NAME),audio_ref_design)
CFLAGS += -DIOT_SDK_DEMO_PROJECT
else ifeq ($(PROJ_NAME),bga_sdk_audio)
CFLAGS += -DIOT_SDK_DEMO_PROJECT
else ifeq ($(PROJ_NAME),bga_sdk_vad)
CFLAGS += -DIOT_SDK_DEMO_PROJECT
else ifeq ($(PROJ_NAME),bga_sdk_uec)
CFLAGS += -DIOT_SDK_DEMO_PROJECT
endif
ifeq ($(CFG_AUDIO_SUSPEND_EN),y)
    CFLAGS += -DCFG_AUDIO_SUSPEND_EN
endif #CFG_AUDIO_SUSPEND_EN
ifeq ($(MTK_AUDIO_DSP_PLAYBACK_ENABLE),y)
    CFLAGS += -DCFG_AUDIO_DSP_PLAYBACK_EN
endif #MTK_AUDIO_DSP_PLAYBACK_ENABLE
ifeq ($(MTK_AUDIO_DSP_LEAUDIO_ENABLE),y)
    CFLAGS += -DCFG_AUDIO_DSP_LEAUDIO_EN
endif #MTK_AUDIO_DSP_LEAUDIO_ENABLE
ifeq ($(CFG_DSPOTTER_SUPPORT),y)
	CFLAGS += -DCFG_DSPOTTER_SUPPORT
	CFLAGS += -DCYB_DSPOTTER_BASE_VERSION=$(CFG_DSPOTTER_BASE_VERSION)
endif #CFG_DSPOTTER_SUPPORT
