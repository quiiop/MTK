IC_CONFIG                           = mt7933
BOARD_CONFIG                        = mt7933_hdk
# build warring
MTK_BUILD_WARNING_AS_ERROR_ENABLE   = n
# debug level: none, error, warning, and info
MTK_DEBUG_LEVEL                       = info
# 3 options with psram/flash or not, only 1 option is y, the others should be n
MTK_MEMORY_WITH_PSRAM_FLASH         = y
MTK_MEMORY_WITHOUT_PSRAM            = n
MTK_MEMORY_WITHOUT_PSRAM_FLASH      = n
# System service debug feature for internal use
MTK_NON_INIT_HEAP                   = n
MTK_SUPPORT_HEAP_DEBUG              = n

MTK_OS_CPU_UTILIZATION_ENABLE       = n
MTK_XIP_ENABLE                      = y

#NVDM
MTK_NVDM_ENABLE                     = y
MTK_NVDM_NO_FLASH_ENABLE            = n

#CONSYS
MTK_MT7933_CONSYS_ENABLE            = y

#CONNSYS WF
MTK_MT7933_CONSYS_WIFI_ENABLE       = y

MTK_MT7933_CONSYS_FW_MODE           = qfn

#WIFI features
MTK_WIFI_AP_ENABLE                  = y
MTK_WIFI_ROUTER_ENABLE              = y
MTK_WIFI_PROFILE_ENABLE             = y
MTK_WLAN_SERVICE_ENABLE             = n
MTK_ATED_ENABLE                     = n
MTK_WIFI_TEST_TOOL_ENABLE           = n
MTK_WIFI_PSRAM_ENABLE               = y
MTK_WIFI_EMI_IN_PSRAM               = y
MTK_WIFI_SWLA_ENABLE                = n
MTK_WF_CLI_ENABLE                   = y
MTK_WF_DBG_CLI_ENABLE               = n

#enable mini-supplicant
MTK_MINISUPP_ENABLE                 = y

#MTK system hang issue debug feauture option
MTK_SYSTEM_HANG_CHECK_ENABLE        = n

#LWIP features
MTK_LWIP_ENABLE                     = y
MTK_IPERF_ENABLE                    = y
MTK_PING_OUT_ENABLE                 = y
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
MTK_OS_HEAP_EXTEND                  = multi

# BT Dual mode
MTK_BT_DUO_ENABLE                   = n

#BT driver
MTK_MT7933_BT_ENABLE                = y
MTK_BT_BUFFER_BIN_MODE              = y
MTK_BT_FW_BIN_TYPE                  = le

# not support in qfn load or need modify ld
#MTK_BT_FW_DL_TO_EMI_ENABLE         = n

# BT stack: Hummingbird
MTK_BT_ENABLE                       = y
MTK_BLE_ONLY_ENABLE                 = y
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

MTK_BT_TIMER_EXTERNAL_ENABLE        = n

# bluetooth connection manager feature support.
MTK_BT_CM_SUPPORT                   = n

# BT memory shrink
MTK_BT_MEM_SHRINK                   = y

# BT APP LIGHT
MTK_BLE_APP_LITE_ENABLE            = y

# BT Cli
MTK_BLE_CLI_ENABLE                  = y
MTK_BT_MESH_CLI_ENABLE              = n

# mesh example project: none, pts_device, test, pts_provisioner, switch, vendor_device, pts_lighting_client, pts_lighting_server
MTK_BT_MESH_EXAMPLE_PROJECT         = vendor_device

#BT tool
MTK_BT_BOOTS_ENABLE                 = n
MTK_BT_PICUS_ENABLE                 = y

#TFM features
MTK_TFM_ENABLE                       = y
MTK_TFM_CLI_ENABLE                   = n

#Bootloader Options
BL_FEATURE                           = feature_tfm.mk
BL_OPT_MTK_MFG_VERIFY                = y

MTK_AUXADCCLI_ENABLE                 = y
MTK_HAL_SER_ENABLE                   = y

MTK_HAL_SLA_LIB_ALL_IN_ONE           = n

#SSUSB
MTK_SSUSB_GADGET_ENABLE              = n
MTK_SSUSB_HOST_ENABLE                = n

#HIFIXDSP
MTK_HIFI4DSP_ENABLE                  = n

#THERMAL_CLI
MTK_THERMAL_CLI_ENABLE               = n

#POSIX
MTK_POSIX_SUPPORT_ENABLE             = n

#Audio driver
MTK_MT7933_AUDIO_DRIVER_ENABLE       = n
MTK_MT7933_AUDIO_CODEC_ENABLE       = n

#FPGA Feature Disable
MTK_FPGA_ENABLE                      = n

MTK_1ST_LINK_SRAM_BOOT               = n

# select the build images
MT7931AN_XIP_QFN_BW_ENABLE           = y

#Core Mini Dump
MTK_MINI_DUMP_ENABLE                 = y

# compiler flag for DVT code
MTK_DVT_CODE_ENABLE                  = n

# HTTPCLIENT SSL
MTK_HTTPCLIENT_SSL_ENABLE            = n

MTK_FOTA_V3_ENABLE                   = n
MTK_FOTA_V3_FREERTOS_ENABLE          = n
MTK_FOTA_V3_CLI_ENABLE               = n
MTK_FOTA_V3_TFTP_ENABLE              = n
MTK_FOTA_V3_HTTP_ENABLE              = n
MTK_FOTA_V3_HTTPS_ENABLE             = n
MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM = aes128

MTK_SECURE_BOOT_ENABLE               = n
MTK_RTOS_SIGN_ENABLE                 = n
MTK_WIFI_SIGN_ENABLE                 = n
MTK_BT_SIGN_ENABLE                   = n

# SWLA
MTK_SWLA_ENABLE                      = y
MTK_SWLA_USE_SYSRAM_BUFFER           = n
MTK_SWLA_WDT_RESET_TRACE             = n

#Syslog
MTK_SAVE_LOG_AND_CONTEXT_DUMP_ENABLE = n
MTK_MEMORY_EXTRACTOR_ENABLE          = n
MTK_LIGHT_WEIGHT_PRINTF_ENABLED      = y

MTK_AUDIO_MP3_ENABLED                = n
MTK_BT_CODEC_ENABLED                 = n

#Enable GCC LTO
MTK_GCC_LTO_ENABLE                   = y

# mbedtls config file
ifeq ($(MTK_FOTA_V3_HTTPS_ENABLE),y)
MTK_MBEDTLS_CONFIG_FILE              = config-mtk-fotav3.h
else
MTK_MBEDTLS_CONFIG_FILE              = config-mtk-homekit.h
endif

# CORE PKCS11
MTK_CORE_PKCS11_ENABLE               = n
MTK_CORE_PKCS11_SYSTEM_TESTS_ENABLE  = n

#F32K clock source software detection
MTK_F32K_SW_DETECT_ENABLED           = n

#scan list size
MTK_SCAN_LIST_SIZE                   = 32
