# ---------------------------------------------------
# Platform Options
# ---------------------------------------------------
CONFIG_MTK_COMBO_WIFI_HIF = axi
#WIFI_TARGET := mt76x8
WLAN_CHIP_ID := MT7933

#/***** Manage configs into compile options ******/
# Define maximum different channels supported for ieee80211_iface_combination setting.
CFG_NUM_DIFFERENT_CHANNELS_STA=2
CFG_NUM_DIFFERENT_CHANNELS_P2P=2

# Define initial driver running mode.
# 0=RUNNING_P2P_MODE, 1=RUNNING_AP_MODE, 2=RUNNING_DUAL_AP_MODE, 3=RUNNING_P2P_AP_MODE
CFG_DRIVER_INITIAL_RUNNING_MODE=3

# Define to enable Android wake_lock
CFG_ENABLE_WAKE_LOCK=0
CFG_DEFAULT_DBG_LEVEL=DBG_CLASS_ERROR
CFG_TX_DIRECT_USB=0
CFG_RX_DIRECT_USB=0
CFG_ENABLE_EFUSE_MAC_ADDR=0
CFG_SUPPORT_SINGLE_SKU_LOCAL_DB=1
#CFG_SUPPORT_DFS_MASTER=1
CFG_SCAN_CHANNEL_SPECIFIED=1
CFG_SUPPORT_ROAMING=1
# Report all bss networks to cfg80211 when do p2p scan
CFG_P2P_SCAN_REPORT_ALL_BSS=0

# Support to change sta, p2p, ap interface names
# y: enable, n: disable
# eg. insmod wlan_mt76x8_usb.ko sta=wlan p2p=p2p ap=ap
CFG_DRIVER_INF_NAME_CHANGE=n

ifneq ($(MTK_RELEASE_MODE),)
CFG_CHIP_RESET_SUPPORT=0
else
CFG_CHIP_RESET_SUPPORT=1
endif

# ---------------------------------------------------
# Compile Options
# ---------------------------------------------------
WLAN_CHIP_LIST:=-UMT6620 -UMT6628 -UMT5931 -UMT6630 -UMT6632
CFG_SUPPORT_DEBUG_FS=0
CFG_SUPPORT_AGPS_ASSIST=0
CFG_SUPPORT_TSF_USING_BOOTTIME=1

CONFIG_MTK_WIFI_MCC_SUPPORT=y

MTK_MET_PROFILING_SUPPORT=no

MTK_MET_TAG_SUPPORT=no

CFG_SUPPORT_STA_P2P_MCC=n

#Set P2P GO to open security,
#For OOBE support, must enable 1
CFG_SUPPORT_P2P_OPEN_SECURITY=n

#Set P2P GO to send data at 11b rate
#For OOBE support, must enable 2
CFG_SUPPORT_P2P_GO_11b_RATE=n

#Let beacon of P2P GO not update by supplicant
#For OOBE support, must enable 3
CFG_SUPPORT_P2P_GO_KEEP_RATE_SETTING=n

CFG_SUPPORT_ADMINCTRL=y

CFG_SUPPORT_ADJUST_MCC_STAY_TIME=n
CFG_DUMP_TXPOWR_TABLE=n
#For adjust channel request interval when ais join net work
CFG_SUPPORT_ADJUST_JOIN_CH_REQ_INTERVAL=n


CFG_SUPPORT_DISABLE_BCN_HINTS=n

#For Ext-PTA debug command
CFG_SUPPORT_EXT_PTA_DEBUG_COMMAND=n


# For FreeRTOS porting
#For freeRTOS Load FW File From Memory(y) OR From Header(n)
ifeq ($(MTK_MT7658_FW_CLI_ENABLE), y)
CFG_SUPPORT_FREERTOS_NVRAM=n
else
CFG_SUPPORT_FREERTOS_NVRAM=n
endif

#For direct RX using PBUF allocated from lwip
CFG_SUPPORT_DIRECT_PBUF_RX=n

ifeq ($(MTK_MINISUPP_ENABLE), y)
CFG_SUPPORT_NO_SUPPLICANT_OPS=n
CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P=n
else
CFG_SUPPORT_NO_SUPPLICANT_OPS=y
CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P=y
endif

# should align minisupp wpa_supplicant/.config CONFIG_AP
ifeq ($(MTK_WIFI_AP_ENABLE), y)
CFG_SUPPORT_P2P=y
else
CFG_SUPPORT_P2P=n
endif

CFG_DIRECT_PBUF_WO_DMA=n

CFG_PROFILE_PBUF=n

#for size shrink. Do not maintain scan info in drv. should not supported
#in Linux system
CFG_NO_SCANINFO_IN_DRV=y

#CONFIG_IPV6=y

CONFIG_MTK_WIFI_TWT_SUPPORT=y
CONFIG_MTK_WIFI_11AX_SUPPORT=y

# for p2p_dev_fsm
ifneq ($(MTK_RELEASE_MODE),)
CFG_P2P_DEV_FSM=n
else
CFG_P2P_DEV_FSM=y
endif

# for wlan_service
ifeq ($(MTK_WLAN_SERVICE_ENABLE), y)
CFG_SUPPORT_WLAN_SERVICE=y
CONFIG_WLAN_SERVICE=1
CONFIG_TEST_ENGINE_OFFLOAD=1
else
CFG_SUPPORT_WLAN_SERVICE=n
CONFIG_WLAN_SERVICE=0
CONFIG_TEST_ENGINE_OFFLOAD=0
endif

#for wifi test tool
ifeq ($(MTK_WIFI_TEST_TOOL_ENABLE), y)
CONFIG_WIFI_TEST_TOOL=1
else
CONFIG_WIFI_TEST_TOOL=0
endif

CONFIG_WIFI_SUPPORT_MODULATION=1
# 0: 64-QAM
# 1: 256-QAM
# 2: 1024-QAM

CONFIG_WIFI_SINGLE_FW=y
CFG_BUFFER_BIN_FROM_FLASH=1

#for ram mode
CFG_ZIDATA_IN_MEM=2
CFG_CODE_IN_MEM_TX=0
CFG_CODE_IN_MEM_RX=0
# 0: SYSRAM
# 1: PSRAM
# 2: no attribute

