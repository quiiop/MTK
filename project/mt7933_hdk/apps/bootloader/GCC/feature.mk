IC_CONFIG                           = mt7933
BOARD_CONFIG                        = mt7933_hdk

# build warring
MTK_BUILD_WARNING_AS_ERROR_ENABLE   = n

# security options
include security.mk

# can configure
MTK_FOTA_ENABLE                     = n
MTK_BL_DEBUG_ENABLE                 = y
MTK_BL_BOOT_MENU_ENABLE             = n
MTK_BL_RESTORE_DEFAULT_ENABLE       = n
MTK_FOTA_DUAL_IMAGE_ENABLE          = n
MTK_MPERF_ENABLE                    = n
MTK_MINICLI_ENABLE                  = y
MTK_BL_CACHE_ENABLE                 = y
MTK_BL_WDT_ENABLE                   = n

#FPGA Feature Disable
MTK_FPGA_ENABLE                     = n

MTK_FOTA_V3_ENABLE                  = y
MTK_FOTA_V3_BOOTLOADER_ENABLE       = y
MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE     = y

##############################################################################
# Options for main project
##############################################################################

# Each chip package corresponds to its default pin config.
# Supported package types: QFN, BGA
MTK_BL_PACKAGE_TYPE                ?= qfn

#
# RAM boot - to be specified in bl_feature.mk of main project
#

MTK_BL_PSRAM_ENABLE                ?= n

