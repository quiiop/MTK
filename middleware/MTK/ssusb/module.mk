
#FEATURE ?= feature.mk
#include $(FEATURE)

SSUSB_PHY_MTK_ENABLE   = y
SSUSB_XHCI_MTK_ENABLE  = y
SSUSB_MTU3_MTK_ENABLE  = y

###################################################
# Sources
USB_SRC = middleware/MTK/ssusb

ifeq ($(SSUSB_PHY_MTK_ENABLE), y)
C_FILES += $(USB_SRC)/phy/src/usb_phy.c
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/ssusb/phy/inc
endif

ifeq ($(SSUSB_XHCI_MTK_ENABLE), y)
C_FILES += $(USB_SRC)/host/src/generic_hub.c \
                 $(USB_SRC)/host/src/quirks.c \
                 $(USB_SRC)/host/src/usb.c \
                 $(USB_SRC)/host/src/usb_dev.c \
                 $(USB_SRC)/host/src/xhci.c \
                 $(USB_SRC)/host/src/xhci_devconf.c \
                 $(USB_SRC)/host/src/xhci_commands.c \
                 $(USB_SRC)/host/src/xhci_events.c \
                 $(USB_SRC)/host/src/xhci_mtk.c \
                 $(USB_SRC)/host/src/xhci_mtk_sch.c \
                 $(USB_SRC)/host/src/xhci_rh.c \
                 $(USB_SRC)/host/src/xhci_debug.c \
                 $(USB_SRC)/host/src/usbmsc.c \
                 $(USB_SRC)/host/src/usbhid.c \
                 $(USB_SRC)/host/src/usbhub.c \
                 $(USB_SRC)/host/src/usbuvc_driver.c \
                 $(USB_SRC)/host/src/usbuvc_video.c

ifeq ($(SSUSB_UAC_HOST_ENABLE), y)
C_FILES += $(USB_SRC)/host/src/usbuac_driver.c \
                 $(USB_SRC)/host/src/usbuac.c
endif

CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/ssusb/host/inc
endif

ifeq ($(SSUSB_MTU3_MTK_ENABLE), y)
C_FILES += $(USB_SRC)/gadget/src/mtu3_plat.c \
                 $(USB_SRC)/gadget/src/mtu3.c \
                 $(USB_SRC)/gadget/src/mtu3_qmu.c \
                 $(USB_SRC)/gadget/src/u_serial.c

CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/ssusb/gadget/inc
endif

###################################################
# include path
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/ssusb/inc
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/ssusb/inc_export

