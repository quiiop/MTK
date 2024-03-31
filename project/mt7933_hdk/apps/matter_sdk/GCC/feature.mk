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
MTK_XIP_ENABLE                      = y

#NVDM
MTK_NVDM_ENABLE                     = y
MTK_NVDM_NO_FLASH_ENABLE            = n

#CONSYS
MTK_MT7933_CONSYS_ENABLE            = y

#CONNSYS WF
MTK_MT7933_CONSYS_WIFI_ENABLE       = y

#WIFI features
MTK_WIFI_AP_ENABLE                  = y
MTK_WIFI_DIRECT_ENABLE              = n
MTK_WIFI_PROFILE_ENABLE             = y
MTK_WIFI_ROM_ENABLE                 = n
MTK_WLAN_SERVICE_ENABLE             = y
MTK_WIFI_PSRAM_ENABLE               = n
MTK_WIFI_EMI_IN_PSRAM               = y

#enable mini-supplicant
MTK_MINISUPP_ENABLE                 = y

#MTK system hang issue debug feauture option
MTK_SYSTEM_HANG_CHECK_ENABLE        = n

#LWIP features
MTK_LWIP_ENABLE                     = y
MTK_PING_OUT_ENABLE                 = y
MTK_USER_FAST_TX_ENABLE             = n

MTK_FREERTOS_VERSION                = V10

# OS Heap Extend: n, heap5, multi
MTK_OS_HEAP_EXTEND                  = multi

#BT driver
MTK_MT7933_BT_ENABLE                = y
MTK_BT_BUFFER_BIN_MODE              = y
MTK_BT_FW_DL_TO_EMI_ENABLE          = y
# BT stack: Hummingbird
MTK_BT_ENABLE                       = y
MTK_BLE_ONLY_ENABLE                 = n
MTK_BT_SUPPORT_FW_ASSERT_RECOVERY   = n

# mesh example project: none, pts_device, test, pts_provisioner, switch, vendor_device, pts_lighting_client, pts_lighting_server
MTK_MBEDTLS_CONFIG_FILE             = config-mtk-homekit.h

#BT tool
MTK_BT_BOOTS_ENABLE                 = y
MTK_BT_PICUS_ENABLE                 = y

MTK_HAL_SER_ENABLE                   = y

MTK_HAL_SLA_LIB_ALL_IN_ONE           = n

#SSUSB
MTK_SSUSB_GADGET_ENABLE              = y
MTK_SSUSB_HOST_ENABLE                = y

#THERMAL_CLI
MTK_THERMAL_CLI_ENABLE               = y

#POSIX
MTK_POSIX_SUPPORT_ENABLE             = y

#FPGA Feature Disable
MTK_FPGA_ENABLE                      = n

MTK_1ST_LINK_SRAM_BOOT               = n

#Core Mini Dump
MTK_MINI_DUMP_ENABLE                 = y

#FATFS
MTK_FATFS_ENABLE                     = y

MTK_FOTA_V3_ENABLE                   = y
MTK_FOTA_V3_FREERTOS_ENABLE          = y
MTK_FOTA_V3_CLI_ENABLE               = y
MTK_FOTA_V3_TFTP_ENABLE              = y
MTK_FOTA_V3_HTTP_ENABLE              = y
MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM = aes128

MTK_RTOS_SIGN_ENABLE                    = y
MTK_SECURE_BOOT_ENABLE                  = y

# SWLA
MTK_SWLA_ENABLE                      = y
MTK_SWLA_USE_SYSRAM_BUFFER           = y
MTK_SWLA_WDT_RESET_TRACE             = n

#Syslog
MTK_SAVE_LOG_AND_CONTEXT_DUMP_ENABLE = y
MTK_MEMORY_EXTRACTOR_ENABLE          = y

