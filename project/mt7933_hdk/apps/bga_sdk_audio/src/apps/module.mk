# Copyright Statement:
#
# (C) 2005-2021  MediaTek Inc. All rights reserved.
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
# Without the prior written permission of MediaTek and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.
# You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
# if you have agreed to and been bound by the applicable license agreement with
# MediaTek ("License Agreement") and been granted explicit permission to do so within
# the License Agreement ("Permitted User").  If you are not a Permitted User,
# please cease any access or use of MediaTek Software immediately.
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
# ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
# WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#

# add MTK_BT_ENABLE here, for build fail if close all BT macros
ifeq ($(MTK_BT_ENABLE), y)

APPS_SRC = $(APP_PATH)/src/apps
APPS_INC = $(APP_PATH)/inc/apps

C_FILES += $(APPS_SRC)/apps_init.c

#-----------------------
#APPs C files
#-----------------------
# Utils
C_FILES += $(APPS_SRC)/utils/apps_debug.c
C_FILES += $(APPS_SRC)/utils/app_bt_utils.c

# Pre-proc activity
C_FILES += $(APPS_SRC)/app_preproc/app_preproc_activity.c

# Home screen activiry
C_FILES += $(APPS_SRC)/app_idle/app_home_screen_idle_activity.c
C_FILES += $(APPS_SRC)/app_idle/app_bt_conn_componet_in_homescreen.c

# BT state service
C_FILES += $(APPS_SRC)/app_bt_state/app_bt_state_service.c


#music part
C_FILES += $(APPS_SRC)/app_music/app_music_idle_activity.c
C_FILES += $(APPS_SRC)/app_music/app_music_activity.c
C_FILES += $(APPS_SRC)/app_music/app_music_utils.c

#hfp part
#C_FILES += $(APPS_SRC)/app_hfp/app_hfp_idle_activity.c
#C_FILES += $(APPS_SRC)/app_hfp/app_hfp_utils.c
#C_FILES += $(APPS_SRC)/app_hfp/app_hfp_activity.c

# app leaudio
ifeq ($(AIR_LE_AUDIO_ENABLE), y)
C_FILES += $(APPS_SRC)/app_le_audio/app_le_audio.c
C_FILES += $(APPS_SRC)/app_le_audio/app_mcp.c
endif

# App state report
#C_FILES += $(APPS_SRC)/app_state_report/apps_state_report.c

# Events folder
C_FILES += $(APPS_SRC)/events/apps_events_bt_event.c


#-----------------------
#APPs Include
#-----------------------
CFLAGS += -I$(SOURCE_DIR)/$(APPS_INC)

CFLAGS += -I$(SOURCE_DIR)/$(APPS_INC)/app_idle
CFLAGS += -I$(SOURCE_DIR)/$(APPS_INC)/app_preproc
CFLAGS += -I$(SOURCE_DIR)/$(APPS_INC)/config
CFLAGS += -I$(SOURCE_DIR)/$(APPS_INC)/events
CFLAGS += -I$(SOURCE_DIR)/$(APPS_INC)/utils


CFLAGS += -I$(SOURCE_DIR)/$(APPS_INC)/app_bt_state
CFLAGS += -I$(SOURCE_DIR)/$(APPS_INC)/app_music
#CFLAGS += -I$(SOURCE_DIR)/$(APPS_INC)/app_hfp

ifeq ($(AIR_LE_AUDIO_ENABLE), y)
CFLAGS += -I$(SOURCE_DIR)/$(APPS_INC)/app_le_audio
endif

#----------------------
#APPs features
#-----------------------

endif