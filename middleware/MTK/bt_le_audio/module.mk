# Copyright Statement:
# 
# (C) 2019  MediaTek Inc. All rights reserved.
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


ifeq ($(AIR_LE_AUDIO_ENABLE), y)
###################################################
# Sources
###################################################
LE_AUDIO_SRC = middleware/MTK/bt_le_audio/src

LE_AUDIO_FILE = $(LE_AUDIO_SRC)/ascs/ble_ascs_service.c \
                $(LE_AUDIO_SRC)/csip/ble_csis_service.c \
                $(LE_AUDIO_SRC)/micp/ble_mics_service.c \
                $(LE_AUDIO_SRC)/pacs/ble_pacs_service.c \
                $(LE_AUDIO_SRC)/vcp/ble_aics_service.c \
                $(LE_AUDIO_SRC)/vcp/ble_vcs_service.c \
                $(LE_AUDIO_SRC)/vcp/ble_vocs_service.c \
                $(LE_AUDIO_SRC)/util/bt_le_audio_util.c \
                $(LE_AUDIO_SRC)/util/bt_le_audio_log.c \
                $(LE_AUDIO_SRC)/cap/ble_cas_service.c \
                $(LE_AUDIO_SRC)/haps/ble_has_service.c \

C_FILES    +=    $(LE_AUDIO_FILE)
C_FILES    +=    $(LE_AUDIO_SRC)/bt_le_audio_sink.c
C_FILES    +=    $(LE_AUDIO_SRC)/bt_le_audio_source.c
C_FILES    +=    $(LE_AUDIO_SRC)/bt_le_audio_source_service_discovery.c
###################################################
# include path (bt_le_audio)
###################################################
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/ascs
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/bap
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/bass
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/ccp
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/csip
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/mcp
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/micp
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/pacs
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/tmap
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/vcp
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/util
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/cap
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/pbp
CFLAGS    +=    -I$(SOURCE_DIR)/middleware/MTK/bt_le_audio/inc/haps

###################################################
# include path
###################################################
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/bluetooth/inc
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/le_audio/inc

###################################################
# Libs
###################################################
# The library name
LE_AUDIO_LIB = libbt_leaudio.a

LE_AUDIO_LIB_PATH = le_audio

# check the bt_cap_protected fodler exist or not.
# If the folder eixst, make the lib with source code
# otherwise, use the library directly.
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/bt_le_audio_protected/),)
LIBS += $(OUTPATH)/$(LE_AUDIO_LIB)
MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_le_audio_protected/GCC
else
LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/$(LE_AUDIO_LIB_PATH)/lib/$(LE_AUDIO_LIB)
endif


endif