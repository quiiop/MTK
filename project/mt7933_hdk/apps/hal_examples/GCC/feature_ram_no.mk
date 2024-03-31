IC_CONFIG                           = mt7933
BOARD_CONFIG                        = mt7933_hdk
# debug level: none, error, warning, and info
MTK_DEBUG_LEVEL                       = info
# 3 options with psram/flash or not, only 1 option is y, the others should be n 
MTK_MEMORY_WITH_PSRAM_FLASH         = y
MTK_MEMORY_WITHOUT_PSRAM            = n
MTK_MEMORY_WITHOUT_PSRAM_FLASH      = n
# System service debug feature for internal use
MTK_SUPPORT_HEAP_DEBUG              = n

MTK_OS_CPU_UTILIZATION_ENABLE       = y
MTK_XIP_ENABLE                      = n

#NVDM
MTK_NVDM_ENABLE                     = n
MTK_NVDM_NO_FLASH_ENABLE            = n

#CONSYS
MTK_MT7933_CONSYS_ENABLE            = n

#CONNSYS WF
MTK_MT7933_CONSYS_WIFI_ENABLE       = n

#WIFI features
MTK_WIFI_AP_ENABLE                  = n
MTK_WIFI_TGN_VERIFY_ENABLE          = n
MTK_WIFI_PROFILE_ENABLE             = n
MTK_WLAN_SERVICE_ENABLE             = n
MTK_ATED_ENABLE                     = n
MTK_WIFI_TEST_TOOL_ENABLE           = n

#enable mini-supplicant
MTK_MINISUPP_ENABLE                 = n

#MTK system hang issue debug feauture option
MTK_SYSTEM_HANG_CHECK_ENABLE        = n

#LWIP features
MTK_LWIP_ENABLE                     = n
MTK_IPERF_ENABLE                    = n
MTK_PING_OUT_ENABLE                 = n
MTK_USER_FAST_TX_ENABLE             = n

#CLI features
MTK_MINICLI_ENABLE                  = y
MTK_CLI_TEST_MODE_ENABLE            = y
MTK_LP_DVT_CLI_ENABLE               = n
MTK_HAL_LOWPOWER_ENABLE             = n
MTK_HIF_GDMA_ENABLE                 = n

MTK_FREERTOS_VERSION                = V10
MTK_UT_ENABLE                       = n
MTK_OS_CLI_ENABLE                   = y

# OS Heap Extend: n, heap5, multi
#MTK_OS_HEAP_EXTEND                  = multi


#BT driver
MTK_MT7933_BT_ENABLE                = n
# BT stack: Hummingbird
MTK_BT_ENABLE                       = n
MTK_BLE_ONLY_ENABLE                 = n
MTK_BT_HFP_ENABLE                   = n
MTK_BT_AVRCP_ENABLE                 = n
MTK_BT_AVRCP_ENH_ENABLE             = n
MTK_BT_A2DP_ENABLE                  = n
MTK_BT_SPP_ENABLE                   = n
MTK_BT_HID_ENABLE                   = n
MTK_BT_PBAP_ENABLE                  = n
MTK_BT_MESH_ENABLE                  = n
MTK_BT_ACEMW_ENABLE                 = n
MTK_BT_BAS_SERVICE_ENABLE           = n
MTK_BLE_SMTCN_ENABLE                = n
MTK_BT_SUPPORT_FW_ASSERT_RECOVERY   = n

# BT rx buffer record
MTK_BT_RX_BUF_RECORD                = n

# BT Cli
MTK_BLE_CLI_ENABLE                  = n
MTK_BT_MESH_CLI_ENABLE              = n

# mesh example project: none, pts_device, test, pts_provisioner, switch, vendor_device, pts_lighting_client, pts_lighting_server
MTK_BT_MESH_EXAMPLE_PROJECT         = vendor_device
MTK_MBEDTLS_CONFIG_FILE             = config-mtk-homekit.h

#BT tool
MTK_BT_BOOTS_ENABLE                 = n
MTK_BT_PICUS_ENABLE                 = n

#TFM features
MTK_TFM_ENABLE                       = n
MTK_TFM_CLI_ENABLE                   = n

MTK_AUXADCCLI_ENABLE                 = n
MTK_HAL_SER_ENABLE                   = n

MTK_HAL_SLA_LIB_ALL_IN_ONE           = n

#SSUSB
MTK_SSUSB_GADGET_ENABLE              = n
MTK_SSUSB_HOST_ENABLE                = n

#HIFIXDSP
MTK_HIFI4DSP_ENABLE                  = n

#THERMAL_CLI
MTK_THERMAL_CLI_ENABLE               = n


#POSIX
MTK_POSIX_SUPPORT_ENABLE            = n

#Audio driver
MTK_MT7933_AUDIO_DRIVER_ENABLE      = n
MTK_MT7933_AUDIO_CODEC_ENABLE      = n

#FPGA Feature Disable
MTK_FPGA_ENABLE                     = n

MTK_1ST_LINK_SRAM_BOOT              = n

# select the build images
MT7931AN_RAM_QFN_NO_ENABLE          = y
MT7931AN_RAM_QFN_BT_ENABLE          = n
MT7931AN_RAM_QFN_WF_ENABLE          = n
MT7931AN_XIP_QFN_NO_ENABLE          = n
MT7931AN_XIP_QFN_BT_ENABLE          = n
MT7931AN_XIP_QFN_WF_ENABLE          = n
MT7931AN_XIP_QFN_BW_ENABLE          = n
MT7933CV_XIP_BGA_NO_ENABLE          = n
MT7933CV_XIP_BGA_BT_ENABLE          = n
MT7933CV_XIP_BGA_WF_ENABLE          = n
MT7933CV_XIP_BGA_AU_ENABLE          = n
MT7933CV_XIP_BGA_AL_ENABLE          = n

#Core Mini Dump
MTK_MINI_DUMP_ENABLE                 = y

# compiler flag for DVT code
MTK_DVT_CODE_ENABLE                  = n

#FATFS
MTK_FATFS_ENABLE                 = n

# SWLA
MTK_SWLA_ENABLE                     = n
