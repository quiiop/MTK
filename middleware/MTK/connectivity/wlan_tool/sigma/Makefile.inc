#
# Copyright (c) 2016 Wi-Fi Alliance
# 
# Permission to use, copy, modify, and/or distribute this software for any 
# purpose with or without fee is hereby granted, provided that the above 
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY 
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
# USE OR PERFORMANCE OF THIS SOFTWARE.
#

# must sync with device/config.txt

export IC_CONFIG
ifeq ($(IC_CONFIG), mt7933)
    SIGMA_BUILD_TYPE = $(IC_CONFIG)_DUT
endif
$(info == build $(IC_CONFIG) sigma $(SIGMA_BUILD_TYPE) ==)

ifeq (, $(filter $(SIGMA_BUILD_TYPE), mt7933_DUT))
BIN_DIR = /sigma
CTRL_IFACE_DIR = /var/run/wpa_supplicant
EAP_CERT_DIR = /$(BIN_DIR)/eap
endif

WTS_DEFAULT_VERSION = PCEDUT-Wi-FiTestSuite-9.1.0

ifeq (, $(filter $(SIGMA_BUILD_TYPE), mt7933_DUT))
CFLAGS := -g -O2 -D_REENTRANT -Wall -I../inc -I../mediatek/wpa
#move programs to beblow configure
#CFLAGS := -g -O2 -D_REENTRANT -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc -I../mediatek/wpa

# socket path for wpa_ctrl
CFLAGS += -DCONFIG_CTRL_IFACE_CLIENT_DIR=\"$(CTRL_IFACE_DIR)\"
endif

ifeq "$(MTK_INTERNAL_BUILD_VERNO)" ""
    CFLAGS += -DMTK_SYSTEM_VER=\"$(WTS_DEFAULT_VERSION)\"
else
    CFLAGS += -DMTK_SYSTEM_VER=\"$(strip $(MTK_INTERNAL_BUILD_VERNO))\"
endif

# path to sigma toolkits
ifeq (, $(filter $(SIGMA_BUILD_TYPE), mt7933_DUT))
#Linux
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
else
#RTOS
CFLAGS += -DBIN_DIR=\"\"
CFLAGS += -DIWPRIV=\"iwpriv\"
CFLAGS += -DMTKINBANDCMD=\"MTK_INBAND_CMD\"
CFLAGS += -DDHCPCLIENT=\"\"
CFLAGS += -DDHCPSERVER=\"\"
CFLAGS += -DDHCPRESET=\"\"
CFLAGS += -DDHCPGETSERVERIP=\"\"
CFLAGS += -DDHCPGETCLIENTIP=\"\"
#Need to implement
CFLAGS += -DSETIPCONFIG=\"\"
CFLAGS += -DGETIPCONFIG=\"\"
CFLAGS += -DEAP_CERT_PATH=\"\"
CFLAGS += -DGETPID=\"\"
CFLAGS += -DGETPSTATS=\"\"
CFLAGS += -DSTOPPING=\"\"
CFLAGS += -DUPDATEPID=\"\"
CFLAGS += -DWFAPING=\"\"
CFLAGS += -DWFAPING6=\"\"
# socket path for wpa_ctrl
CFLAGS += -DCONFIG_CTRL_IFACE_CLIENT_DIR=\"\"
endif

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

##############################################################################
# Support Program
##############################################################################

CFLAGS += -DWFA_STA_TB
CFLAGS += -DCONFIG_MTK_COMMON

CFLAGS += -DCONFIG_MTK_WPA3
CFLAGS += -DCONFIG_MTK_MBO
CFLAGS += -DCONFIG_MTK_HE

#tmp not support
ifeq (, $(filter $(SIGMA_BUILD_TYPE), mt7933_DUT mt7933_CA))
CFLAGS += -DCONFIG_MTK_AP
CFLAGS += -DCONFIG_MTK_P2P
CFLAGS += -DCONFIG_MTK_WMM_VOE
CFLAGS += -DCONFIG_MTK_TDLS
CFLAGS += -DCONFIG_MTK_WMM_PS
CFLAGS += -DCONFIG_MTK_WFD
CFLAGS += -DWFA_WMM_PS_EXT
CFLAGS += -DWFA_WMM_AC
CFLAGS += -DWFA_VOICE_EXT
endif


ifneq ($(IC_CONFIG), mt7933)
CC = $(CROSS_COMPILER)gcc
AR = $(CROSS_COMPILER)ar
endif

LIB=lib
DUT=dut
CA=ca
UCC=ucc
CON=console_src
WTG=WTGService
MAKE=make

# This is for WMM-PS
#for Ext TG
#CFLAGS = -g -O2 -D_REENTRANT -DWFA_WMM_PS_EXT -Wall -I../inc

## for WMM-AC, WMM-PS, Voice, PMF, TDLS Test-Bed 
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc

## for WMM-AC, WMM-PS Test-Bed
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_STA_TB -Wall -I../inc

## for non-Test-Bed WMM-AC, WMM-PS and Voice
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -Wall -I../inc

## for PC-ENDPOINT
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_PC_CONSOLE -Wall -I../inc

## for PC-ENDPOINT No Voice
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_PC_CONSOLE -DWFA_STA_TB -Wall -I../inc

# This is for WPA2 as default
#CFLAGS = -g -O2 -D_REENTRANT -Wall -I../inc

ifneq ($(IC_CONFIG), mt7933)
DUTLIBS = ../lib/libwfa_dut.a -lpthread
CALIBS = ../lib/libwfa_ca.a -lpthread
else
DUTLIBS = ../lib/libwfa_dut.a
endif

ifneq ($(IC_CONFIG), mt7933)
RANLIB = ranlib
endif

LIBWFA_NAME_DUT = libwfa_dut.a
ifneq ($(IC_CONFIG), mt7933)
LIBWFA_NAME_CA = libwfa_ca.a
endif
LIBWFA_NAME = libwfa.a

ifneq ($(IC_CONFIG), mt7933)
LIB_OBJS = wpa_helpers.o wpa_ctrl.o os_unix.o wfa_sock.o wfa_tg.o mtk_cs.o mtk_wfd.o wfa_ca_resp.o wfa_tlv.o wfa_typestr.o wfa_cmdtbl.o wfa_cmdproc.o wfa_miscs.o wfa_thr.o wfa_wmmps.o

LIB_OBJS_DUT = wpa_helpers.o wpa_ctrl.o os_unix.o wfa_sock.o wfa_tlv.o mtk_cs.o mtk_wfd.o wfa_cmdtbl.o wfa_tg.o wfa_miscs.o wfa_thr.o wfa_wmmps.o

LIB_OBJS_CA = wfa_sock.o wfa_tlv.o wfa_ca_resp.o wfa_cmdproc.o wfa_miscs.o wfa_typestr.o
endif

CLEANFILES = core core.* *.core.* *.o *.a
