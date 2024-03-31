/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc.    (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS    ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef __USBDCORE_H__
#define __USBDCORE_H__

#include "semphr.h"

/* Request types */
#define USB_TYPE_MASK       (0x03 << 5)
#define USB_TYPE_STANDARD   (0x00 << 5)
#define USB_TYPE_CLASS      (0x01 << 5)
#define USB_TYPE_VENDOR     (0x02 << 5)
#define USB_TYPE_RESERVED   (0x03 << 5)

/* USB recipients */
#define USB_RECIP_MASK      0x1f
#define USB_RECIP_DEVICE    0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT  0x02
#define USB_RECIP_OTHER     0x03

#define USB_DEVICE_SELF_POWERED     0   /* (read only) */
#define USB_DEVICE_REMOTE_WAKEUP    1   /* dev may initiate wakeup */
#define USB_DEVICE_TEST_MODE        2   /* (wired high speed only) */
#define USB_DEVICE_BATTERY      2   /* (wireless) */
#define USB_DEVICE_B_HNP_ENABLE     3   /* (otg) dev may initiate HNP */
#define USB_DEVICE_WUSB_DEVICE      3   /* (wireless)*/
#define USB_DEVICE_A_HNP_SUPPORT    4   /* (otg) RH port supports HNP */
#define USB_DEVICE_A_ALT_HNP_SUPPORT    5   /* (otg) other RH port does */
#define USB_DEVICE_DEBUG_MODE       6   /* (special devices only) */

/*
 * Test Mode Selectors
 * See USB 2.0 spec Table 9-7
 */
#define TEST_J      1
#define TEST_K      2
#define TEST_SE0_NAK    3
#define TEST_PACKET 4

/* USB transfer directions */
#define USB_DIR_MASK        0x80
#define USB_DIR_IN      0x80
#define USB_DIR_OUT     0

/* Endpoints */
#define USB_EP_NUM_MASK     0x0f    /* in bEndpointAddress */

#define GET_EP_NUM(bEndpointAddress)    (bEndpointAddress & USB_EP_NUM_MASK)
#define GET_EP_DIR(bEndpointAddress)    (bEndpointAddress & USB_DIR_MASK)

#define USB_EP_XFER_CTRL        0
#define USB_EP_XFER_ISO         1
#define USB_EP_XFER_BULK        2
#define USB_EP_XFER_INT         3

#define UDC_BULK_IN    0x82
#define UDC_BULK_OUT   0x02

/* Standard requests */
#define STDREQ_GET_STATUS       0x00
#define STDREQ_CLEAR_FEATURE        0x01
#define STDREQ_SET_FEATURE      0x03
#define STDREQ_SET_ADDRESS      0x05
#define STDREQ_SET_SEL          0x30

#define STDREQ_GET_DESCRIPTOR       0x06
#define STDREQ_GET_CONFIGURATION    0x08
#define STDREQ_SET_CONFIGURATION    0x09
#define STDREQ_GET_INTERFACE        0x0A
#define STDREQ_SET_INTERFACE        0x0B

/* USB release number   (2.0 does not mean high speed!) */
#define USB_BCD_VERSION         0x0210
#define USB3_BCD_VERSION        0x0300

/* values used in GET_STATUS requests */
#define USB_STAT_SELFPOWERED        0x01

/* Descriptor types */
#define USB_DESCRIPTOR_TYPE_DEVICE          0x01
#define USB_DESCRIPTOR_TYPE_CONFIGURATION       0x02
#define USB_DESCRIPTOR_TYPE_STRING          0x03
#define USB_DESCRIPTOR_TYPE_INTERFACE           0x04
#define USB_DESCRIPTOR_TYPE_ENDPOINT            0x05
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER        0x06
#define USB_DESCRIPTOR_TYPE_BOS             0x0f
#define USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY       0x10
#define USB_DESCRIPTOR_TYPE_SS_ENDPOINT_COMPANION   0x30

#define UDC_EVENT_ONLINE    1
#define UDC_EVENT_OFFLINE   2
#define UDC_CLASS_ONLINE   100

#define MAX_USBFS_BULK_SIZE (65532)

/* USB Requests */
struct usb_setup {
    unsigned char bmRequestType;
    unsigned char bRequest;
    unsigned short wValue;
    unsigned short wIndex;
    unsigned short wLength;
} __attribute__((packed));

typedef struct udc_request udc_request_t;
typedef struct udc_endpoint udc_endpoint_t;

/* USB Device Controller Transfer Request */
struct udc_request {
    void *buffer;
    unsigned int length;
    unsigned int actual;
    int status;
    bool zero;
    xSemaphoreHandle xSemaphore;
    void (*complete)(udc_request_t *req, unsigned int actual, int status);
    void *context;
};

struct mt_intf {
    int alternates;
    struct mt_altsetting *altsetting_array;
};

struct mt_config {
    int interfaces;
    struct configuration_descriptor *config_desc;
    struct mt_intf **interface_array;
    char *raw;
};

typedef enum {
    RET_SUCCESS = 0,
    RET_FAIL,
} USB_RESULT;

typedef enum {
    SSUSB_SPEED_UNKNOWN = 0,
    SSUSB_SPEED_LOW = 1,
    SSUSB_SPEED_FULL = 2,
    SSUSB_SPEED_HIGH = 3,
    SSUSB_SPEED_SUPER = 5,
    SSUSB_SPEED_SUPER_PLUS = 6,
} USB_SPEED;

typedef enum {
    DEV_CAP_WIRELESS = 1,   /* Wireless_USB, */
    DEV_CAP_USB20_EXT = 2,  /* USB2.0 extension */
    DEV_CAP_SS_USB = 3, /* Superspeed USB */
    DEV_CAP_CON_ID = 4  /* Container ID */
} DEV_CAPABILITY_TYPE;

typedef enum {
    EP0_IDLE = 0,
    EP0_RX,
    EP0_TX,
    EP0_TX_END,
} EP0_STATE;

struct function_driver {
    int (*setup)(struct udc_request *req, const struct usb_setup *usb_ctrl);
    int (*set_alt)(struct udc_request *req, const struct usb_setup *usb_ctrl);
    int (*rx_setup)(struct udc_request *req);
    int (*bind)(void);
    void (*unbind)(void);
};

struct gadget_dev {
    char *name;
    struct gadget_dev *next;
    struct device_descriptor *dev_desc; /* per device descriptor */
    struct device_qualifier_descriptor *dev_qualifier_desc;
    struct bos_descriptor *bos_desc;
    struct ext_cap_descriptor *ext_cap_desc;
    struct ss_cap_descriptor *ss_cap_desc;
    struct string_descriptor **string_table;

    /* configuration descriptors */
    int configs;
    struct mt_config *conf_array;
    void (*notify)(struct gadget_dev *gdev, unsigned event);
    void (*assign_descs)(USB_SPEED speed);

    unsigned char address;       /* function address, 0 by default */
    unsigned char configuration;    /* configuration, 0 by default, means unconfigured */
    unsigned char interface;        /* interface, 0 by default */
    unsigned char alternate;        /* alternate setting */
    unsigned char speed;
    struct udc_endpoint **eps;
    int num_eps;
    struct function_driver driver;
    void *private;
};

struct device_descriptor {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short bcdUSB;
    unsigned char bDeviceClass;
    unsigned char bDeviceSubClass;
    unsigned char bDeviceProtocol;
    unsigned char bMaxPacketSize0;
    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char iManufacturer;
    unsigned char iProduct;
    unsigned char iSerialNumber;
    unsigned char bNumConfigurations;
} __attribute__((packed));

/* U2 spec 9.6.2 */
struct device_qualifier_descriptor {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short bcdUSB;
    unsigned char bDeviceClass;
    unsigned char bDeviceSubClass;
    unsigned char bDeviceProtocol;
    unsigned char bMaxPacketSize0;
    unsigned char bNumConfigurations;
} __attribute__((packed));

struct bos_descriptor {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short wTotalLength;
    unsigned char bNumDeviceCaps;
} __attribute__((packed));

#define SUPPORT_LPM_PROTOCOL    (1 << 1)    /* supports LPM */

/* Link Power Management */

struct ext_cap_descriptor {
    unsigned char   bLength;
    unsigned char   bDescriptorType;
    unsigned char   bDevCapabilityType;
    unsigned int bmAttributes;
} __attribute__((packed));

#define NOT_LTM_CAPABLE     (0 << 1)    /* Not able to generate Latenct Tolerance Messages */
#define SUPPORT_LS_OP       (1)     /* Low speed operation */
#define SUPPORT_FS_OP       (1 << 1)    /* Full speed operation */
#define SUPPORT_HS_OP       (1 << 2)    /* High speed operation */
#define SUPPORT_5GBPS_OP    (1 << 3)    /* Operation at 5Gbps */
#define DEFAULT_U1_DEV_EXIT_LAT 0x01        /* Less then 1 microsec */
#define DEFAULT_U2_DEV_EXIT_LAT 0x1F4       /* Less then 500 microsec */

struct ss_cap_descriptor {          /* Link Power Management */
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bDevCapabilityType;
    unsigned char bmAttributes;
    unsigned short wSpeedSupported;
    unsigned char bFunctionalitySupport;
    unsigned char bU1devExitLat;
    unsigned short bU2DevExitLat;
} __attribute__((packed));

struct configuration_descriptor {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short wTotalLength;
    unsigned char bNumInterfaces;
    unsigned char bConfigurationValue;
    unsigned char iConfiguration;
    unsigned char bmAttributes;
    unsigned char bMaxPower;
} __attribute__((packed));

struct interface_descriptor {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bInterfaceNumber;
    unsigned char bAlternateSetting;
    unsigned char bNumEndpoints;
    unsigned char bInterfaceClass;
    unsigned char bInterfaceSubClass;
    unsigned char bInterfaceProtocol;
    unsigned char iInterface;
} __attribute__((packed));

struct endpoint_descriptor {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bEndpointAddress;
    unsigned char bmAttributes;
    unsigned short wMaxPacketSize;
    unsigned char bInterval;
} __attribute__((packed));

struct ss_ep_comp_descriptor {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bMaxBurst;
    unsigned char bmAttributes;
    unsigned short wBytesPerInterval;
} __attribute__((packed));

struct string_descriptor {
    unsigned char bLength;
    unsigned char bDescriptorType;   /* 0x03 */
    unsigned short wData[256];
} __attribute__((packed));

#endif /* #ifndef __USBDCORE_H__ */
