##############################################################################
#
# External Dependencies
#
#############################################################################


##############################################################################
#
# Configurations
#
#############################################################################

SBOOT_REPO                ?= middleware/MTK/sboot

MTK_SBOOT_SCOTT_SKIP_SIGN ?= n


ifeq ($(MTK_SECURE_BOOT_ENABLE),y)
CFLAGS  += -I$(SOURCE_DIR)/$(SBOOT_REPO)/inc
endif

ifeq ($(MTK_SECURE_BOOT_ENABLE),y)
C_FILES += $(SBOOT_REPO)/src/scott_image.c
endif


# Info necessary for image sign


#
# MT7933 imgtool.py ECDSA signature generation
#

imgtool_env_in_sdk     := $(SDK_PATH)/tools/venv_imgtool_linux

imgtool_env_paths      += /proj/srv_mt7933/venv/imgtool/bin/activate
imgtool_env_paths      += /site/p_mt7933/venv/imgtool/bin/activate
imgtool_env_paths      += $(imgtool_env_in_sdk)/imgtool/bin/activate

$(foreach f,$(imgtool_env_paths),$(if $(wildcard $(f)),$(eval IMGTOOL_ENV=$(f)),))

ifneq ($(IMGTOOL_ENV),)
_MTK_SBOOT_IMGTOOL_ENV_FROM = "preinstalled venv"
else
ifeq ($(wildcard $(imgtool_env_in_sdk)),)
_MTK_SBOOT_IMGTOOL_ENV_FROM = "SDK python environment"
else
_MTK_SBOOT_IMGTOOL_ENV_FROM = "no python environment for imgtool, skip sign"
MTK_SBOOT_SCOTT_SKIP_SIGN  := y
endif
endif

IMGTOOL          ?= $(SDK_PATH)/tools/mcuboot/scripts/imgtool.py

##############################################################################
#
# Sign Images
#
#############################################################################

# MTK_SECURE_BOOT_FIRMWARE is used by modules providing F/W that needs
# to be signed. The target sboot_firmware_sign is used to sign the
# files listed in MTK_SECURE_BOOT_FIRMWARE.

.PHONY: sboot_firmware_sign

sboot_firmware_sign:
	$(Q)echo "sboot: $(_MTK_SBOOT_IMGTOOL_ENV_FROM)"
ifneq ($(MTK_SBOOT_SCOTT_SKIP_SIGN),y)

	$(Q)echo "sboot: sign firmware input:  $(SBOOT_FW_IN)"      \
    &&                                                          \
    if [ ! -f $(SBOOT_FW_IN) ]; then                            \
        echo "sboot: not found $(SBOOT_FW_IN)" >&2; exit -1;    \
    fi                                                          \
    &&                                                          \
    if [ ! -f "$(SGN_PEM)" ]; then                              \
        echo "sboot: PEM keyfile not found" >&2; exit 1;        \
    fi                                                          \
    &&                                                          \
    echo "sboot: secure header size: $(SBOOT_FW_HDR_SZ)"        \
    &&                                                          \
    echo "sboot: firmware load addr: $(SBOOT_FW_ADDR)"          \
    &&                                                          \
    echo "sboot: firmware size:      $(SBOOT_FW_SIZE)"          \
    &&                                                          \
    echo "sboot: firmware version:   $(SBOOT_FW_VER)"           \
    &&                                                          \
    if [ -f $(SBOOT_FW_OUT) ]; then                             \
        echo "sboot: remove $(SBOOT_FW_OUT)";                   \
        rm -f $(SBOOT_FW_OUT);                                  \
    fi                                                          \
    &&                                                          \
    echo "sboot: $(_MTK_SBOOT_IMGTOOL_ENV_FROM)"                \
    &&                                                          \
    source $(IMGTOOL_ENV)                                       \
    &&                                                          \
    $(IMGTOOL) sign --pad-header                                \
                    --header-size $(SBOOT_FW_HDR_SZ)            \
                    --load-addr $(SBOOT_FW_ADDR)                \
                    -k $(SGN_PEM)                               \
                    -S $(SBOOT_FW_SIZE)                         \
                    --align 4                                   \
                    -v $(SBOOT_FW_VER)                          \
                    --pubkey                                    \
                    $(SBOOT_FW_IN)                              \
                    $(SBOOT_FW_OUT)                             \
    &&                                                          \
    echo "sboot: sign firmware output: $(SBOOT_FW_OUT)"
endif
