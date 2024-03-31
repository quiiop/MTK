
###############################################################################
# feature option dependency
###############################################################################

###############################################################################
## MTK_FOTA_V3_ENABLE
##
## Brief:       Set this option to "y" to enable FOTAv3.
##
## Value:       y|n
##
## Usage:       If the value is "y", compile flag MTK_FOTA_V3_ENABLE is
##              defined and can be used in C or C++ code.
##              You must include the middleware/MTK/fota/module.mk in your
##              Makefile before setting the option to "y".
##
## Path:        middleware/MTK/fota
##
## Dependency:  HAL_FLASH_MODULE_ENABLED and HAL_WDT_MODULE_ENABLED must also
##              be defined in the hal_feature_config.h under project "inc"
##              folder.
##
## Makefile
##      MTK_FOTA_V3_ENABLE
## CFLAGS:
##      MTK_FOTA_V3_ENABLE
###############################################################################


# default value
MTK_FOTA_V3_ENABLE ?= n

ifeq ($(MTK_FOTA_V3_ENABLE),y)

    CFLAGS += -D MTK_FOTA_V3_ENABLE

else ifneq ($(MTK_FOTA_V3_ENABLE),n)

    $(error MTK_FOTA_V3_ENABLE must be y or n)

endif


###############################################################################
## MTK_FOTA_V3_CLI_ENABLE
##
## Brief:   Set this option to "y" to enable FOTAv3 CLI.
##
## Value:   y|n
##
## Makefile
##      MTK_FOTA_V3_CLI_ENABLE
## CFLAGS:
##      MTK_FOTA_V3_CLI_ENABLE
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    # default value
    MTK_FOTA_V3_CLI_ENABLE ?= n

    ifeq ($(MTK_FOTA_V3_CLI_ENABLE),y)
    CFLAGS += -D MTK_FOTA_V3_CLI_ENABLE
    endif

endif


###############################################################################
## MTK_FOTA_V3_TFTP_ENABLE
##
## Brief:   Set this option to "y" to enable TFTP support.
##
## Value:   y|n
##
## MAKEFILE
##      MTK_FOTA_V3_TFTP_ENABLE
## CFLAGS:
##      MTK_FOTA_V3_TFTP_ENABLE
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    # default value
    MTK_FOTA_V3_TFTP_ENABLE         ?= n

    ifeq ($(MTK_FOTA_V3_TFTP_ENABLE),y)
    CFLAGS += -D MTK_FOTA_V3_TFTP_ENABLE
    include $(SOURCE_DIR)/middleware/MTK/tftp/module.mk
    endif

endif


###############################################################################
## MTK_FOTA_V3_HTTP_ENABLE
##
## Brief: Set this option to "y" to enable HTTP support.
##
## Value:      y|n
##
## MAKEFILE
##      MTK_FOTA_V3_HTTP_ENABLE
## CFLAGS:
##      MTK_FOTA_V3_HTTP_ENABLE
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    # default value
    MTK_FOTA_V3_HTTP_ENABLE         ?= n

    ifeq ($(MTK_FOTA_V3_HTTP_ENABLE),y)
    CFLAGS += -D MTK_FOTA_V3_HTTP_ENABLE
    include $(SOURCE_DIR)/middleware/third_party/httpclient/module.mk
    endif

endif


###############################################################################
## MTK_FOTA_V3_HTTPS_ENABLE
##
## Brief: Set this option to "y" to enable HTTPS support.
##
## Value:      y|n
##
## MAKEFILE
##      MTK_FOTA_V3_HTTPS_ENABLE
## CFLAGS:
##      MTK_FOTA_V3_HTTPS_ENABLE
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    # default value
    MTK_FOTA_V3_HTTPS_ENABLE        ?= n

    ifeq ($(MTK_FOTA_V3_HTTPS_ENABLE),y)
    CFLAGS += -D MTK_FOTA_V3_HTTPS_ENABLE
    include $(SOURCE_DIR)/middleware/third_party/httpclient/module.mk
    endif

endif


###############################################################################
## MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM
##
## Brief:   Set this option to "aes128" to enable packet AES128 encryption.
##          If enabled, must also set the key of encryption algorithm:
##          MTK_FOTA_V3_PACKET_ENCRYPTION_KEY.
##
## Value:   none|aes128
##
## Makefile
##      MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM
## CFLAGS:
##      MTK_FOTA_V3_PACKET_ENCRYPTION_AES128
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    # default value
    MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM  ?= none

    ifeq ($(MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM),aes128)

        CFLAGS += -D MTK_FOTA_V3_PACKET_ENCRYPTION_AES128

    else ifneq ($(MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM),none)

        $(error NOT SUPPORTED ENCRYPTION: $(MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM))

    endif

endif


###############################################################################
## MTK_FOTA_V3_PACKET_ENCRYPTION_KEY
##
## Brief:   This option is used when MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM
##          is set to aes128.
##
## Value:   example: 0x00112233445566778899aabbccddeeff (32 hex digits)
##
## Makefile
##      MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM
## CFLAGS:
##      MTK_FOTA_V3_PACKET_AES128_KEY
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    ifeq ($(MTK_FOTA_V3_PACKET_ENCRYPTION_ALGORITHM),aes128)

        MTK_FOTA_V3_PACKET_AES128_KEY ?= 0x00112233445566778899aabbccddeeff

        KEY="$(shell echo $(MTK_FOTA_V3_PACKET_AES128_KEY) | xxd -r | xxd -i -g 1)"
        CFLAGS += -D MTK_FOTA_V3_PACKET_AES128_KEY=\"$(KEY)\"

        $(info CONVERTED AES128 PACKET ENCRYPTION KEY: $(KEY) (MTK_FOTA_V3_PACKET_AES128_KEY).)

    endif

endif


###############################################################################
## MTK_FOTA_V3_PAYLOAD_ENCRYPTION_ALGORITHM
##
## Brief: Set this option to "aes128" to enable payload AES128 encryption.
##        If enabled, must also set the key of encryption algorithm:
##        MTK_FOTA_V3_PAYLOAD_ENCRYPTION_KEY.
##
## Value:   none|aes128
##
## MAKEFILE
##      MTK_FOTA_V3_PAYLOAD_ENCRYPTION_ALGORITHM
## CFLAGS:
##      MTK_FOTA_V3_PAYLOAD_ENCRYPTION_AES128
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    # default value
    MTK_FOTA_V3_PAYLOAD_ENCRYPTION_ALGORITHM ?= none

    ifeq ($(MTK_FOTA_V3_PAYLOAD_ENCRYPTION_ALGORITHM),aes128)

        CFLAGS += -D MTK_FOTA_V3_PAYLOAD_ENCRYPTION_AES128

    else ifneq ($(MTK_FOTA_V3_PAYLOAD_ENCRYPTION_ALGORITHM),none)

        $(error NOT SUPPORTED PAYLOAD ENCRYPTION ALGORITHM)

    endif

endif


###############################################################################
## MTK_FOTA_V3_PAYLOAD_ENCRYPTION_KEY
##
## Brief:   This option is used when MTK_FOTA_V3_PAYLOAD_ENCRYPTION_ALGORITHM
##          is set to aes128.
##
## Value:   example: 0x00112233445566778899aabbccddeeff (32 hex digits)
##
## MAKEFILE
##      MTK_FOTA_V3_PAYLOAD_ENCRYPTION_ALGORITHM
## CFLAGS:
##      MTK_FOTA_V3_PAYLOAD_ENCRYPTION_KEY
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    ifeq ($(MTK_FOTA_V3_PAYLOAD_ENCRYPTION_ALGORITHM),aes128)

        # default value
        MTK_FOTA_V3_PAYLOAD_ENCRYPTION_KEY ?= 0x00112233445566778899aabbccddeeff

        KEY="$(shell echo $(MTK_FOTA_V3_PAYLOAD_ENCRYPTION_KEY)|xxd -r|xxd -i -g 1)"
        CFLAGS += -D MTK_FOTA_V3_PAYLOAD_ENCRYPTION_KEY=\"$(KEY)\"

        $(info CONVERTED PAYLOAD ENCRYPTION KEY (MTK_FOTA_V3_PAYLOAD_ENCRYPTION_KEY): $(KEY))

    endif

endif


###############################################################################
## MTK_FOTA_V3_PAYLOAD_CHECKSUM_ENABLE
##
## Brief: Set this option to "sha256" to enable payload SHA256 checksum.
##
## Value:   none|sha256
##
## MAKEFILE
##      MTK_FOTA_V3_PAYLOAD_CHECKSUM_ENABLE
## CFLAGS:
##      MTK_FOTA_V3_PAYLOAD_CHECKSUM_SHA256
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    # default value
    MTK_FOTA_V3_PAYLOAD_CHECKSUM_ENABLE ?= n

    ifeq ($(MTK_FOTA_V3_PAYLOAD_CHECKSUM_ENABLE),sha256)

        CFLAGS += -D MTK_FOTA_V3_PAYLOAD_CHECKSUM_ENABLE

    else ifneq ($(MTK_FOTA_V3_PAYLOAD_CHECKSUM_ENABLE),n)

        $(error NOT SUPPORTED PAYLOAD CHECKSUM ALGORITHM)

    endif

endif


###############################################################################
## MTK_FOTA_V3_PAYLOAD_SIGN_ALGORITHM
##
## Brief: Set this option to "ecdsa" to enable payload ECDSA elliptic curved
##        based digital signature algorithm.
##        If enabled, must also set the key of encryption algorithm:
##        MTK_FOTA_V3_PAYLOAD_SIGN_ECDSA_PEM
##
## Value:   none|ecdsa
##
## MAKEFILE:
##      MTK_FOTA_V3_PAYLOAD_SIGN_ALGORITHM
## CFLAGS:
##      MTK_FOTA_V3_PAYLOAD_SIGN_ECDSA_ENABLE
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    # default value
    MTK_FOTA_V3_PAYLOAD_SIGN_ALGORITHM ?= none

    ifeq ($(MTK_FOTA_V3_PAYLOAD_SIGN_ALGORITHM),ecdsa)

        CFLAGS += -D MTK_FOTA_V3_PAYLOAD_SIGN_ECDSA_ENABLE

    else ifneq ($(MTK_FOTA_V3_PAYLOAD_SIGN_ALGORITHM),none)

        $(error NOT SUPPORTED: $(MTK_FOTA_V3_PAYLOAD_SIGN_ALGORITHM) (MTK_FOTA_V3_PAYLOAD_SIGN_ALGORITHM).)

    endif

endif


###############################################################################
## MTK_FOTA_V3_PAYLOAD_SIGN_ECDSA_PEM
##
## Brief:   Set this option to a "PEM file" containing private and public
##          keys to sign and verify image.
##
## Value:   <file name> or <file path>
##
## MAKEFILE:
##      MTK_FOTA_V3_PAYLOAD_SIGN_ALGORITHM
## CFLAGS:
##      MTK_FOTA_V3_PAYLOAD_SIGN_ECDSA_PEM
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    ifeq ($(MTK_FOTA_V3_PAYLOAD_SIGN_ALGORITHM),ecdsa)

        # default value
        MTK_FOTA_V3_PAYLOAD_SIGN_ECDSA_PEM ?= notspecified

        ifeq ($(MTK_FOTA_V3_PAYLOAD_SIGN_ECDSA_PEM),notspecified)

            $(error PEM file is not specified, please specify MTK_FOTA_V3_PAYLOAD_SIGN_ECDSA_PEM)

        endif

        $(info USE PRIVATE KEY FROM $(MTK_FOTA_V3_PAYLOAD_SIGN_ECDSA_PEM))

    endif

endif


###############################################################################
## MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
##
## Brief:   Set this option to support LZMA compression.
##
## Value:   n|y
##
## MAKEFILE:
##      MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
## CFLAGS:
##      MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    # default value
    MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE ?= n

    ifeq ($(MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE),y)

        CFLAGS += -D MTK_FOTA_V3_PAYLOAD_LZMA_ENABLE

    endif

endif


###############################################################################
## MTK_FOTA_V3_PAYLOAD_PLAIN_ENABLE
##
## Brief:   Set this option to support PLAIN text (no compression).
##
## Value:   y|n
##
## MAKEFILE:
##      MTK_FOTA_V3_PAYLOAD_PLAIN_ENABLE
## CFLAGS:
##      MTK_FOTA_V3_PAYLOAD_PLAIN_ENABLE
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    # default value
    MTK_FOTA_V3_PAYLOAD_PLAIN_ENABLE ?= y

    ifeq ($(MTK_FOTA_V3_PAYLOAD_PLAIN_ENABLE),y)

        CFLAGS += -D MTK_FOTA_V3_PAYLOAD_PLAIN_ENABLE

    endif

endif


###############################################################################
## MTK_FOTA_V3_DUAL_PACKET_ENABLE
##
## Brief: Set this option to support A/B images scheme.
##        Two images exists on flash concurrently but only one of them is
##        active at once.
##
## Value:   y|n
##
## MAKEFILE:
##      MTK_FOTA_V3_DUAL_PACKET_ENABLE
## CFLAGS:
##      MTK_FOTA_V3_DUAL_PACKET_ENABLE
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    MTK_FOTA_V3_DUAL_PACKET_ENABLE ?= n

    ifeq ($(MTK_FOTA_V3_DUAL_PACKET_ENABLE),y)

        ifneq ($(MTK_FOTA_V3_PACKET_STORAGE),none)

            $(error DUAL PACKET AND SINGLE PACKET MODES BOTH ENABLED.)

        endif

        CFLAGS += -D MTK_FOTA_V3_DUAL_PACKET_ENABLED

    endif

endif


###############################################################################
## MTK_FOTA_V3_PACKET_STORAGE
##
## BRIEF: Set this option to "fs" to store packet in file system.
##        Set this option to "flash" to store packet in flash partition.
##
## VALUE:   fs|flash
##
## MAKEFILE:
##      MTK_FOTA_V3_PACKET_STORAGE
##
## CFLAGS:
##      MTK_FOTA_V3_PACKET_IN_FS
##      MTK_FOTA_V3_PACKET_IN_FLASH
###############################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    MTK_FOTA_V3_PACKET_STORAGE ?= none

    ifneq ($(MTK_FOTA_V3_PACKET_STORAGE),none)

        ifneq ($(MTK_FOTA_V3_DUAL_PACKET_ENABLE),n)

            $(error DUAL PACKET AND SINGLE PACKET MODES BOTH ENABLED.)

        endif

        ifeq ($(MTK_FOTA_V3_PACKET_STORAGE),fs)

            CFLAGS += -D MTK_FOTA_V3_PACKET_IN_FS

        else ifeq ($(MTK_FOTA_V3_PACKET_STORAGE),flash)

            CFLAGS += -D MTK_FOTA_V3_PACKET_IN_FLASH

        else

            $(error PACKET STORAGE TYPE NOT SUPPORTED: $(MTK_FOTA_V3_PACKET_STORAGE) (MTK_FOTA_V3_PACKET_STORAGE).)

        endif

    endif

endif


#################################################################################
# SOURCE FILES
################################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    _FOTA_PATH_  = middleware/MTK/fota

    C_FILES     += $(_FOTA_PATH_)/src/v3/fota_api.c
    C_FILES     += $(_FOTA_PATH_)/src/v3/fota_log.c
    C_FILES     += $(_FOTA_PATH_)/src/v3/fota_migrate.c
    C_FILES     += $(_FOTA_PATH_)/src/v3/fota_payload.c
    C_FILES     += $(_FOTA_PATH_)/src/v3/fota_osal.c

    ifeq ($(MTK_FOTA_V3_CLI_ENABLE),y)
        C_FILES     += $(_FOTA_PATH_)/src/v3/url.c
        C_FILES     += $(_FOTA_PATH_)/src/v3/fota_cli.c
    endif

    ifeq ($(MTK_FOTA_V3_DUAL_PACKET_ENABLE),y)
        C_FILES     += $(_FOTA_PATH_)/src/v3/fota_two_packet.c
    endif


    ifeq ($(MTK_FOTA_V3_FREERTOS_ENABLE),y)
        CFLAGS      += -D MTK_FOTA_V3_FREERTOS_ENABLE
        C_FILES     += $(_FOTA_PATH_)/src/v3/fota_download.c
    endif

    ifeq ($(MTK_FOTA_V3_BOOTLOADER_ENABLE),y)
        CFLAGS      += -D MTK_FOTA_V3_BOOTLOADER_ENABLE
    endif

endif


#################################################################################
# INCLUDE PATH
################################################################################


ifeq ($(MTK_FOTA_V3_ENABLE),y)

    CFLAGS      += -I$(SOURCE_DIR)/$(_FOTA_PATH_)/inc

endif

