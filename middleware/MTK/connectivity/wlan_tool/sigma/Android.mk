LOCAL_PATH:= $(call my-dir)

# must sync with device/config.txt
BIN_DIR = /data/bin
CTRL_IFACE_DIR = /data/vendor/wifi/wpa/sockets
EAP_CERT_DIR = /data/misc/wifi
SIGMA_OUT_DIR = $(PRODUCT_OUT)/testcases/sigma

# wfa options, do not modify
CFLAGS := -g -O2 -D_REENTRANT -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc
CFLAGS += -Wno-unused-variable -Wno-unused-parameter -Wno-format-security -Wno-format-invalid-specifier -Wno-format-extra-args -Wno-format -Wno-undefined-inline -Wno-pointer-sign -Wno-unused-parameter -Wno-incompatible-pointer-types -Wno-dangling-else -Wno-pointer-arith -Wno-parentheses-equality -Wno-array-bounds -Wno-missing-field-initializers -Wno-constant-logical-operand -Wno-incompatible-pointer-types-discards-qualifiers -Wno-sign-compare -Wno-user-defined-warnings -Wno-implicit-function-declaration -Wno-unused-label -Wno-missing-braces -Wno-int-conversion -Wno-varargs -Wno-unused-function -Wno-deprecated-declarations -Wno-switch

# socket path for wpa_ctrl
CFLAGS += -DCONFIG_CTRL_IFACE_CLIENT_DIR=\"$(CTRL_IFACE_DIR)\"
CFLAGS += -DMTK_SYSTEM_VER=\"$(strip $(MTK_INTERNAL_BUILD_VERNO))\"

# path to sigma toolkits
CFLAGS += -DBIN_DIR=\"$(BIN_DIR)\"
CFLAGS += -DIWPRIV=\"$(BIN_DIR)/iwpriv\"
CFLAGS += -DDHCPCLIENT=\"$(BIN_DIR)/mtk_dhcp_client.sh\"
CFLAGS += -DDHCPSERVER=\"$(BIN_DIR)/mtk_dhcp_server.sh\"
CFLAGS += -DDHCPRESET=\"$(BIN_DIR)/mtk_dhcp_reset.sh\"
CFLAGS += -DDHCPGETSERVERIP=\"$(BIN_DIR)/mtk_dhcp_getserverip.sh\"
CFLAGS += -DDHCPGETCLIENTIP=\"$(BIN_DIR)/mtk_dhcp_getclientipbymac.sh\"
CFLAGS += -DSETIPCONFIG=\"$(BIN_DIR)/mtk_setipconfig.sh\"
CFLAGS += -DGETIPCONFIG=\"$(BIN_DIR)/mtk_getipconfig.sh\"
CFLAGS += -DEAP_CERT_PATH=\"$(EAP_CERT_DIR)\"
CFLAGS += -DGETPID=\"$(BIN_DIR)/getpid.sh\"
CFLAGS += -DGETPSTATS=\"$(BIN_DIR)/getpstats.sh\"
CFLAGS += -DSTOPPING=\"$(BIN_DIR)/stoping.sh\"
CFLAGS += -DUPDATEPID=\"$(BIN_DIR)/updatepid.sh\"
CFLAGS += -DWFAPING=\"$(BIN_DIR)/wfaping.sh\"
CFLAGS += -DWFAPING6=\"$(BIN_DIR)/wfaping6.sh\"
CFLAGS += -DMTKINBANDCMD=\"$(BIN_DIR)/mtk_inband_cmd.sh\"

CFLAGS += -DMODE_WMM_PS=\"WMMPS\"
CFLAGS += -DMODE_WMM_AC=\"WMMAC\"
CFLAGS += -DMODE_VOE=\"VoE\"
CFLAGS += -DMODE_P2P=\"P2P\"
CFLAGS += -DMODE_AP=\"AP\"
CFLAGS += -DMODE_TDLS=\"TDLS\"
CFLAGS += -DMODE_TG_N=\"TGn\"
CFLAGS += -DMODE_TG_AC=\"TGac\"
CFLAGS += -DMODE_WPA3=\"WPA3\"
CFLAGS += -DMODE_MBO=\"MBO\"
CFLAGS += -DMODE_PMF=\"PMF\"
CFLAGS += -DMODE_TG_AX=\"WiFi6\"
CFLAGS += -DMODE_WFD=\"WFD\"

CFLAGS += -DCONFIG_MTK_COMMON
CFLAGS += -DCONFIG_MTK_AP
CFLAGS += -DCONFIG_MTK_P2P
CFLAGS += -DCONFIG_MTK_WMM_VOE
CFLAGS += -DCONFIG_MTK_TDLS
CFLAGS += -DCONFIG_MTK_WPA3
CFLAGS += -DCONFIG_MTK_WMM_PS
CFLAGS += -DCONFIG_MTK_MBO
CFLAGS += -DCONFIG_MTK_HE
CFLAGS += -DCONFIG_MTK_WFD

########################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    lib/wfa_sock.c \
    lib/wfa_tlv.c \
    lib/wfa_cmdtbl.c \
    lib/wfa_tg.c \
    lib/wfa_miscs.c \
    lib/wfa_thr.c \
    lib/wfa_wmmps.c \

# mtk proprierary configure and setup
LOCAL_SRC_FILES += \
    mediatek/mtk_cs.c \
    mediatek/mtk_wfd.c \

# files copied from wpa_supplicant
LOCAL_SRC_FILES += \
    mediatek/wpa/wpa_helpers.c \
    mediatek/wpa/wpa_ctrl.c \
    mediatek/wpa/os_unix.c \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libnetutils

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/mediatek/wpa \
    system/core/libnetutils/include

LOCAL_MODULE := libwfadut_static
LOCAL_CFLAGS := $(CFLAGS) -DANDROID_LOG_NAME=\"wfa_dut\" -DCONFIG_CTRL_IFACE -DCONFIG_CTRL_IFACE_UNIX -DANDROID
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := tests
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
include $(BUILD_STATIC_LIBRARY)

########################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    dut/wfa_dut.c \
    dut/wfa_dut_init.c \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libnetutils

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/inc \

LOCAL_STATIC_LIBRARIES += libwfadut_static
LOCAL_CFLAGS := $(CFLAGS)
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := tests
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_PATH := $(SIGMA_OUT_DIR)/scripts
LOCAL_MODULE:= wfa_dut

include $(BUILD_EXECUTABLE)

########################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    lib/wfa_sock.c \
    lib/wfa_tlv.c \
    lib/wfa_ca_resp.c \
    lib/wfa_cmdproc.c \
    lib/wfa_miscs.c \
    lib/wfa_typestr.c \
    ca/wfa_ca.c \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/inc \

LOCAL_CFLAGS := $(CFLAGS) -DANDROID_LOG_NAME=\"wfa_ca\"
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := tests
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_PATH := $(SIGMA_OUT_DIR)/scripts
LOCAL_MODULE:= wfa_ca

include $(BUILD_EXECUTABLE)

########################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    console_src/wfa_con.c \
    console_src/wfa_sndrcv.c \
    console_src/wfa_util.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libc \
    liblog

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/inc

LOCAL_CFLAGS := $(CFLAGS) -DANDROID_LOG_NAME=\"wfa_con\"
LOCAL_STATIC_LIBRARIES += libwfadut_static
LOCAL_CFLAGS := $(CFLAGS)
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := tests
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_PATH := $(SIGMA_OUT_DIR)/scripts
LOCAL_MODULE:= wfa_con

include $(BUILD_EXECUTABLE)

########################

PATH_TO_FILE = mediatek/mtk_inband_cmd.sh.x.c

ifeq ("$(wildcard $(LOCAL_PATH)/$(PATH_TO_FILE))", "")

$(warning "$(LOCAL_PATH)/$(PATH_TO_FILE) doesn't exist. Please fetch files from alps/wifi_tool")

else

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(PATH_TO_FILE)
LOCAL_CFLAGS := $(CFLAGS)
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := tests
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_PATH := $(SIGMA_OUT_DIR)/scripts
LOCAL_MODULE:= mtk_inband_cmd.sh

include $(BUILD_EXECUTABLE)

endif
