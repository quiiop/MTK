/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/*
 * MediaTek Inc. (C) 2018. All rights reserved.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "system_rscs.h"

#include "u_serial.h"

#define tty_err(x...) printf("[USBTTY] " x)
#define tty_crit(x...) printf("[USBTTY] " x)

#define  USB_TTY_DBG_LOG 0
#if USB_TTY_DBG_LOG
#define tty_log(x...) printf("[USBTTY] " x)
#else /* #if USB_TTY_DBG_LOG */
#define tty_log(x...) do {} while (0)
#endif /* #if USB_TTY_DBG_LOG */

#define CDC_DTR_MASK    0x01

#define INIT_STR_DESC(array, desctype, str) \
    do { \
        struct string_descriptor *string = NULL; \
        string = (struct string_descriptor *)array; \
        string->bDescriptorType = desctype; \
        string->bLength = sizeof(array); \
        str2wide(str, string->wData); \
    } while (0)

static struct gadget_dev mt_serial_dev[1];
static struct mt_config mt_serial_config[NUM_CONFIGS];
static struct mt_intf *mt_serial_interface[NUM_INTERFACES];
static struct mt_intf mt_serial_data_interface[NUM_DATA_INTERFACES];
static struct mt_altsetting mt_serial_data_alt_if[NUM_DATA_INTERFACES];
static struct mt_intf mt_serial_comm_interface[NUM_COMM_INTERFACES];
static struct mt_altsetting mt_serial_comm_alt_if[NUM_COMM_INTERFACES];
static unsigned short acm_port_state;
#define WTOTALLENGTH 256
static char raw_data[WTOTALLENGTH];

/* one extra for control endpoint, but not used */
struct udc_endpoint *mt_serial_ep[NUM_ENDPOINTS + 1];
static int serial_online;

/* USB descriptors */
/* string descriptors */
#define ALIGN4 __align(4)

static u8 language[4]__attribute__((aligned(4))) = { 4, USB_DESCRIPTOR_TYPE_STRING, 0x9, 0x4 };
static u8 manufacturer[2 + 2 * (sizeof(USBD_MANUFACTURER) - 1)]__attribute__((aligned(4)));
static u8 product[2 + 2 * (sizeof(USBD_PRODUCT_NAME) - 1)]__attribute__((aligned(4)));
static u8 configuration[2 + 2 * (sizeof(USBD_CONFIGURATION_STR) - 1)]__attribute__((aligned(4)));
static u8 dataInterface[2 + 2 * (sizeof(USBD_DATA_INTERFACE_STR) - 1)]__attribute__((aligned(4)));
static u8 commInterface[2 + 2 * (sizeof(USBD_COMM_INTERFACE_STR) - 1)]__attribute__((aligned(4)));

static struct string_descriptor *serial_string_table[] = {
    (struct string_descriptor *)language,
    (struct string_descriptor *)manufacturer,
    (struct string_descriptor *)product,
    (struct string_descriptor *)configuration,
    (struct string_descriptor *)dataInterface,
    (struct string_descriptor *)commInterface,
};

/* device descriptor */
static struct device_descriptor serial_device_desc = {
    sizeof(struct device_descriptor),
    USB_DESCRIPTOR_TYPE_DEVICE,
    USB_BCD_VERSION,
    USBDL_DEVICE_CLASS,
    USBDL_DEVICE_SUBCLASS,
    USBDL_DEVICE_PROTOCOL,
    EP0_MAX_PACKET_SIZE,
    USBD_VENDORID,
    USBD_PRODUCTID,
    USBD_BCD_DEVICE,
    STR_MANUFACTURER,
    STR_PRODUCT,
    0,
    NUM_CONFIGS
};

static struct bos_descriptor serial_bos_descriptor = {
    sizeof(struct bos_descriptor),
    USB_DESCRIPTOR_TYPE_BOS,
    sizeof(struct bos_descriptor) + sizeof(struct ext_cap_descriptor) + sizeof(struct ss_cap_descriptor),
    2
};

static struct ext_cap_descriptor serial_ext_cap_descriptor = {
    sizeof(struct ext_cap_descriptor),
    USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY,
    DEV_CAP_USB20_EXT,
    SUPPORT_LPM_PROTOCOL
};

static struct ss_cap_descriptor serial_ss_cap_descriptor = {
    sizeof(struct ss_cap_descriptor),
    USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY,
    DEV_CAP_SS_USB,
    NOT_LTM_CAPABLE,
    (SUPPORT_FS_OP | SUPPORT_HS_OP | SUPPORT_5GBPS_OP),
    SUPPORT_FS_OP,
    DEFAULT_U1_DEV_EXIT_LAT,
    DEFAULT_U2_DEV_EXIT_LAT
};

/* device qualifier descriptor */
static struct device_qualifier_descriptor serial_device_qualifier_desc = {
    sizeof(struct device_qualifier_descriptor),
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    USB_BCD_VERSION,
    USBDL_DEVICE_CLASS,
    USBDL_DEVICE_SUBCLASS,
    USBDL_DEVICE_PROTOCOL,
    EP0_MAX_PACKET_SIZE,
    NUM_CONFIGS,
};

/* configuration descriptor */
static struct configuration_descriptor serial_config_descriptors[NUM_CONFIGS] = {
    {
        sizeof(struct configuration_descriptor),
        USB_DESCRIPTOR_TYPE_CONFIGURATION,
        (sizeof(struct configuration_descriptor) * NUM_CONFIGS) +
        (sizeof(struct interface_descriptor) * NUM_INTERFACES) +
        (sizeof(struct cdcacm_class_header_function_descriptor)) +
        (sizeof(struct cdcacm_class_abstract_control_descriptor)) +
        (sizeof(struct cdcacm_class_union_function_descriptor)) +
        (sizeof(struct cdcacm_class_call_management_descriptor)) +
        (sizeof(struct endpoint_descriptor) * NUM_ENDPOINTS),
        NUM_INTERFACES,
        1,
        STR_CONFIG,
        0xc0,
        USBD_MAXPOWER
    },
};

static struct interface_descriptor serial_interface_descriptors[NUM_INTERFACES] = {
    /*
     * interface_descriptors[0]: data interface
     * interface_descriptors[1]: communication interface
     */
    {
        sizeof(struct interface_descriptor),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        0,
        0,
        NUM_DATA_ENDPOINTS,
        USBDL_DATA_INTERFACE_CLASS,
        USBDL_DATA_INTERFACE_SUBCLASS,
        USBDL_DATA_INTERFACE_PROTOCOL,
        STR_DATA_INTERFACE
    }, {
        sizeof(struct interface_descriptor),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        1,
        0,
        NUM_COMM_ENDPOINTS,
        USBDL_COMM_INTERFACE_CLASS,
        USBDL_COMM_INTERFACE_SUBCLASS,
        USBDL_COMM_INTERFACE_PROTOCOL,
        STR_COMM_INTERFACE
    },
};

static struct cdcacm_class_header_function_descriptor
    header_function_descriptor = {
    0x05,
    0x24,
    0x00,  /* 0x00 for header functional descriptor */
    0x0110,
};

static struct cdcacm_class_abstract_control_descriptor
    abstract_control_descriptor = {
    0x04,
    0x24,
    0x02,  /* 0x02 for abstract control descriptor */
    0x0f,
};

struct cdcacm_class_union_function_descriptor union_function_descriptor = {
    0x05,
    0x24,
    0x06,  /* 0x06 for union functional descriptor */
    0x01,
    0x00,
};

struct cdcacm_class_call_management_descriptor call_management_descriptor = {
    0x05,
    0x24,
    0x01,  /* 0x01 for call management descriptor */
    0x03,
    0x00,
};

static struct endpoint_descriptor serial_ss_ep_descriptors[NUM_ENDPOINTS] = {
    {
        sizeof(struct endpoint_descriptor),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USBD_SERIAL_OUT_ENDPOINT | USB_DIR_OUT,
        USB_EP_XFER_BULK,
        USBD_SERIAL_OUT_SS_PKTSIZE,
        0,
    }, {
        sizeof(struct endpoint_descriptor),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USBD_SERIAL_IN_ENDPOINT | USB_DIR_IN,
        USB_EP_XFER_BULK,
        USBD_SERIAL_IN_SS_PKTSIZE,
        0,
    }, {
        sizeof(struct endpoint_descriptor),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USBD_INT_IN_ENDPOINT | USB_DIR_IN,
        USB_EP_XFER_INT,
        USBD_INT_IN_SS_PKTSIZE,
        0x10,   /* polling interval is every 16 frames */
    }
};

/* Currently all ep can share the same companion descriptors */
static struct ss_ep_comp_descriptor serial_ss_ep_comp_descriptor[NUM_ENDPOINTS] = {
    {
        sizeof(struct ss_ep_comp_descriptor),
        USB_DESCRIPTOR_TYPE_SS_ENDPOINT_COMPANION,
        15,
        0,
        0,
    }, {
        sizeof(struct ss_ep_comp_descriptor),
        USB_DESCRIPTOR_TYPE_SS_ENDPOINT_COMPANION,
        15,
        0,
        0,
    }, {
        sizeof(struct ss_ep_comp_descriptor),
        USB_DESCRIPTOR_TYPE_SS_ENDPOINT_COMPANION,
        0,
        0,
        16,
    },
};

static struct endpoint_descriptor serial_hs_ep_descriptors[NUM_ENDPOINTS] = {
    {
        sizeof(struct endpoint_descriptor),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USBD_SERIAL_OUT_ENDPOINT | USB_DIR_OUT,
        USB_EP_XFER_BULK,
        USBD_SERIAL_OUT_HS_PKTSIZE,
        0,
    }, {
        sizeof(struct endpoint_descriptor),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USBD_SERIAL_IN_ENDPOINT | USB_DIR_IN,
        USB_EP_XFER_BULK,
        USBD_SERIAL_IN_HS_PKTSIZE,
        0,
    }, {
        sizeof(struct endpoint_descriptor),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USBD_INT_IN_ENDPOINT | USB_DIR_IN,
        USB_EP_XFER_INT,
        USBD_INT_IN_HS_PKTSIZE,
        0x10,  /* polling interval is every 16 frames */
    },
};

static struct endpoint_descriptor serial_fs_ep_descriptors[NUM_ENDPOINTS] = {
    {
        sizeof(struct endpoint_descriptor),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USBD_SERIAL_OUT_ENDPOINT | USB_DIR_OUT,
        USB_EP_XFER_BULK,
        USBD_SERIAL_OUT_FS_PKTSIZE,
        0,
    }, {
        sizeof(struct endpoint_descriptor),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USBD_SERIAL_IN_ENDPOINT | USB_DIR_IN,
        USB_EP_XFER_BULK,
        USBD_SERIAL_IN_FS_PKTSIZE,
        0,
    }, {
        sizeof(struct endpoint_descriptor),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USBD_INT_IN_ENDPOINT | USB_DIR_IN,
        USB_EP_XFER_INT,
        USBD_INT_IN_FS_PKTSIZE,
        0x10,  /* polling interval is every 16 frames */
    },
};

static struct endpoint_descriptor
    *serial_ss_data_ep_descriptor_ptrs[NUM_DATA_ENDPOINTS] = {
    &serial_ss_ep_descriptors[0],
    &serial_ss_ep_descriptors[1],
};

static struct endpoint_descriptor
    *serial_ss_comm_ep_descriptor_ptrs[NUM_COMM_ENDPOINTS] = {
    &serial_ss_ep_descriptors[2],
};

static struct ss_ep_comp_descriptor
    *serial_ss_data_ep_comp_descriptor_ptrs[NUM_DATA_ENDPOINTS] = {
    &serial_ss_ep_comp_descriptor[0],
    &serial_ss_ep_comp_descriptor[1],
};

static struct ss_ep_comp_descriptor
    *serial_ss_comm_ep_comp_descriptor_ptrs[NUM_COMM_ENDPOINTS] = {
    &serial_ss_ep_comp_descriptor[2],
};

static struct endpoint_descriptor
    *serial_hs_data_ep_descriptor_ptrs[NUM_DATA_ENDPOINTS] = {
    &serial_hs_ep_descriptors[0],
    &serial_hs_ep_descriptors[1],
};

static struct endpoint_descriptor
    *serial_hs_comm_ep_descriptor_ptrs[NUM_COMM_ENDPOINTS] = {
    &serial_hs_ep_descriptors[2],
};

static struct endpoint_descriptor
    *serial_fs_data_ep_descriptor_ptrs[NUM_DATA_ENDPOINTS] = {
    &serial_fs_ep_descriptors[0],
    &serial_fs_ep_descriptors[1],
};

static struct endpoint_descriptor
    *serial_fs_comm_ep_descriptor_ptrs[NUM_COMM_ENDPOINTS] = {
    &serial_fs_ep_descriptors[2],
};

static void str2wide(char *str, u16 *wide)
{
    unsigned int i;

    for (i = 0; i < strlen(str) && str[i]; ++i)
        wide[i] = (u16)str[i];
}

static int serial_ep0_setup(struct udc_request *req, const struct usb_setup *usb_ctrl);
static int serial_set_alt(struct udc_request *req, const struct usb_setup *usb_ctrl);
static int serial_bind(void);
static void serial_unbind(void);

static void serial_assign_descs(USB_SPEED speed);

static struct device_descriptor *get_device_desc(int enableU3)
{
    if (enableU3 == 0) {
        serial_device_desc.bcdUSB = USB_BCD_VERSION;
        serial_device_desc.bMaxPacketSize0 = EP0_MAX_PACKET_SIZE;
    } else {
        serial_device_desc.bcdUSB = USB3_BCD_VERSION;
        serial_device_desc.bMaxPacketSize0 = EP0_MAX_PACKET_SIZE_U3_EXP;
    }

    return &serial_device_desc;
}

static struct device_qualifier_descriptor *get_qualified_desc(int enableU3)
{
    if (enableU3 == 0) {
        serial_device_qualifier_desc.bcdUSB = USB_BCD_VERSION;
        serial_device_qualifier_desc.bMaxPacketSize0 = EP0_MAX_PACKET_SIZE;
    } else {
        serial_device_qualifier_desc.bcdUSB = USB3_BCD_VERSION;
        serial_device_qualifier_desc.bMaxPacketSize0 = EP0_MAX_PACKET_SIZE_U3_EXP;
    }

    return &serial_device_qualifier_desc;
}

static struct configuration_descriptor *get_config_desc(int enableU3)
{
    int i = 0;

    if (enableU3 == 0) {
        for (i = 0; i < NUM_CONFIGS; ++i) {
            serial_config_descriptors[i].bMaxPower = USBD_MAXPOWER;
            serial_config_descriptors[i].wTotalLength =
                (sizeof(struct configuration_descriptor) * NUM_CONFIGS) +
                (sizeof(struct interface_descriptor) * NUM_INTERFACES) +
                (sizeof(struct cdcacm_class_header_function_descriptor)) +
                (sizeof(struct cdcacm_class_abstract_control_descriptor)) +
                (sizeof(struct cdcacm_class_union_function_descriptor)) +
                (sizeof(struct cdcacm_class_call_management_descriptor)) +
                (sizeof(struct endpoint_descriptor) * NUM_ENDPOINTS);
        }
    } else {
        for (i = 0; i < NUM_CONFIGS; ++i) {
            serial_config_descriptors[i].bMaxPower = USBD_SS_MAXPOWER;
            serial_config_descriptors[i].wTotalLength =
                (sizeof(struct configuration_descriptor) * NUM_CONFIGS) +
                (sizeof(struct interface_descriptor) * NUM_INTERFACES) +
                (sizeof(struct cdcacm_class_header_function_descriptor)) +
                (sizeof(struct cdcacm_class_abstract_control_descriptor)) +
                (sizeof(struct cdcacm_class_union_function_descriptor)) +
                (sizeof(struct cdcacm_class_call_management_descriptor)) +
                (sizeof(struct endpoint_descriptor) * NUM_ENDPOINTS) +
                (sizeof(struct ss_ep_comp_descriptor) * NUM_ENDPOINTS);
        }
    }

    return (struct configuration_descriptor *)(&serial_config_descriptors);
}

static void serial_udev_notify(struct gadget_dev *gdev, unsigned int event)
{
    switch (event) {
        case UDC_EVENT_OFFLINE:
            serial_online = 0;
            break;
        case UDC_EVENT_ONLINE:
            serial_online = 1;
            break;
        default:
            break;
    }

    tty_log("notify event : %d\n", event);
}

static void serial_device_init(void)
{
    /* device instance initialization */
    mt_serial_dev->name = CLASS_NAME;
    mt_serial_dev->dev_desc = get_device_desc(0);
    mt_serial_dev->dev_qualifier_desc = get_qualified_desc(0);
    mt_serial_dev->bos_desc = &serial_bos_descriptor;
    mt_serial_dev->ext_cap_desc = &serial_ext_cap_descriptor;
    mt_serial_dev->ss_cap_desc = &serial_ss_cap_descriptor;
    mt_serial_dev->configs = NUM_CONFIGS;
    mt_serial_dev->conf_array = mt_serial_config;
    mt_serial_dev->speed = SSUSB_SPEED_FULL;
    mt_serial_dev->notify = serial_udev_notify;
    mt_serial_dev->driver.setup = serial_ep0_setup;
    mt_serial_dev->driver.set_alt = serial_set_alt;
    mt_serial_dev->driver.bind = serial_bind;
    mt_serial_dev->driver.unbind = serial_unbind;
    mt_serial_dev->assign_descs = serial_assign_descs;
    tty_log("%s: %s\n", __func__, mt_serial_dev->name);
}

void serial_config_init(void)
{
    /* configuration instance initialization */
    memset(mt_serial_config, 0, sizeof(mt_serial_config));
    mt_serial_config->interfaces = NUM_INTERFACES;
    mt_serial_config->config_desc = get_config_desc(0);
    mt_serial_config->interface_array = mt_serial_interface;
    mt_serial_config->raw = raw_data;
}

void serial_interfaces_init(void)
{
    mt_serial_interface[0] = mt_serial_data_interface;
    mt_serial_interface[1] = mt_serial_comm_interface;

    /* data interface instance */
    memset(mt_serial_data_interface, 0, sizeof(mt_serial_data_interface));
    mt_serial_data_interface->alternates = 1;
    mt_serial_data_interface->altsetting_array = mt_serial_data_alt_if;

    /* data alternates instance */
    memset(mt_serial_data_alt_if, 0, sizeof(mt_serial_data_alt_if));

    mt_serial_data_alt_if->p_interface_desc = &serial_interface_descriptors[0];
    mt_serial_data_alt_if->endpoints = NUM_DATA_ENDPOINTS;
    mt_serial_data_alt_if->pp_eps_desc_array = serial_fs_data_ep_descriptor_ptrs;

    /* communication interface instance */
    memset(mt_serial_comm_interface, 0, sizeof(mt_serial_comm_interface));
    mt_serial_comm_interface->alternates = 1;
    mt_serial_comm_interface->altsetting_array = mt_serial_comm_alt_if;

    /* communication alternates instance */
    /* contains communication class specific interface descriptors */
    memset(mt_serial_comm_alt_if, 0, sizeof(mt_serial_comm_alt_if));

    mt_serial_comm_alt_if->p_interface_desc = &serial_interface_descriptors[1];
    mt_serial_comm_alt_if->p_header_function_desc = &header_function_descriptor;
    mt_serial_comm_alt_if->p_abstract_control_desc = &abstract_control_descriptor;
    mt_serial_comm_alt_if->p_union_function_desc = &union_function_descriptor;
    mt_serial_comm_alt_if->p_call_management_desc = &call_management_descriptor;
    mt_serial_comm_alt_if->endpoints = NUM_COMM_ENDPOINTS;
    mt_serial_comm_alt_if->pp_eps_desc_array = serial_fs_comm_ep_descriptor_ptrs;
}

/* should called after @mt_serial_dev init */
static void update_eps_from_desc(struct endpoint_descriptor *ed_array)
{
    struct udc_endpoint *ep;
    struct endpoint_descriptor *ep_desc;
    int i = 1;

    for (i = 1; i < mt_serial_dev->num_eps; i++) {
        ep = mt_serial_dev->eps[i];
        ep_desc = &ed_array[i - 1];

        ep->in = !!GET_EP_DIR(ep_desc->bEndpointAddress);
        ep->binterval = ep_desc->bInterval;
        ep->type = ep_desc->bmAttributes;
        ep->maxpkt = ep_desc->wMaxPacketSize;
        ep->desc = ep_desc;
    }
}

static void update_eps_comp_from_desc(struct ss_ep_comp_descriptor *ed_array)
{
    struct udc_endpoint *ep;
    struct ss_ep_comp_descriptor *ep_desc;
    int i = 1;

    for (i = 1; i < mt_serial_dev->num_eps; i++) {
        ep = mt_serial_dev->eps[i];
        ep_desc = &ed_array[i - 1];

        ep->comp_desc = ep_desc;
    }
}

static udc_endpoint_t *serial_ep_in;
static udc_endpoint_t *serial_ep_out;

static int serial_bind(void)
{
    int i;
    /* alloc ep */
    mt_serial_ep[ACM_BULK_OUT_EP] = udc_endpoint_alloc(mt_serial_dev->private,
                                                       USB_EP_XFER_BULK, USB_DIR_OUT);
    mt_serial_ep[ACM_BULK_IN_EP] = udc_endpoint_alloc(mt_serial_dev->private,
                                                      USB_EP_XFER_BULK, USB_DIR_IN);
    mt_serial_ep[ACM_INT_IN_EP] = udc_endpoint_alloc(mt_serial_dev->private,
                                                     USB_EP_XFER_INT, USB_DIR_IN);
    for (i = 1; i <= NUM_ENDPOINTS; i++) {
        if (!mt_serial_ep[i]) {
            tty_err("alloc ep[%d] fail\n", i);
            return -1;
        }
    }

    mt_serial_dev->eps = mt_serial_ep;
    mt_serial_dev->num_eps = NUM_ENDPOINTS + 1;
    serial_ep_in = mt_serial_ep[ACM_BULK_IN_EP];
    serial_ep_out = mt_serial_ep[ACM_BULK_OUT_EP];
    /* alloc request */
    serial_ep_in->req = udc_request_alloc(serial_ep_in);
    if (!serial_ep_in->req) {
        tty_err("%s: alloc req fail\n", serial_ep_in->name);
        return -1;
    }
    serial_ep_out->req = udc_request_alloc(serial_ep_out);
    if (!serial_ep_out->req) {
        tty_err("%s: alloc req fail\n", serial_ep_out->name);
        return -1;
    }

    for (i = 1; i <= NUM_ENDPOINTS; i++)
        tty_log("alloc ep[%d]:%p req:%p\n", i, mt_serial_ep[i], mt_serial_ep[i]->req);

    update_eps_from_desc(serial_fs_ep_descriptors);
    return 0;
}

static void serial_unbind(void)
{
    int i;

    for (i = 1; i <= NUM_ENDPOINTS; i++) {
        udc_request_free(mt_serial_ep[i]->req);
        mt_serial_ep[i]->req = NULL;
        udc_endpoint_free(mt_serial_ep[i]);
        mt_serial_ep[i] = NULL;
    }
}

void serial_strings_init(void)
{
    /* initialize string descriptor array */
    INIT_STR_DESC(manufacturer, USB_DESCRIPTOR_TYPE_STRING, USBD_MANUFACTURER);
    INIT_STR_DESC(product, USB_DESCRIPTOR_TYPE_STRING, USBD_PRODUCT_NAME);
    INIT_STR_DESC(configuration, USB_DESCRIPTOR_TYPE_STRING, USBD_CONFIGURATION_STR);
    INIT_STR_DESC(dataInterface, USB_DESCRIPTOR_TYPE_STRING, USBD_DATA_INTERFACE_STR);
    INIT_STR_DESC(commInterface, USB_DESCRIPTOR_TYPE_STRING, USBD_COMM_INTERFACE_STR);

    /* Now, initialize the string table for ep0 handling */
    mt_serial_dev->string_table = serial_string_table;
}

int copy_all_descs(void)
{
    int size;
    int intf_num = 0;
    char *src_data = NULL;
    struct mt_config *cfg = NULL;
    struct configuration_descriptor *cfg_dsc = NULL;

    cfg = mt_serial_config;
    cfg_dsc = mt_serial_config->config_desc;
    size = mt_serial_config->config_desc->wTotalLength;
    if (size > WTOTALLENGTH)
        tty_err("error: configure desc buffer overflow\n");

    /* copy configure descriptor */
    src_data = raw_data;
    memcpy(src_data, cfg_dsc, sizeof(*cfg_dsc));
    src_data += sizeof(*cfg_dsc);
    /* loop over all interfaces */
    for (intf_num = 0; intf_num < cfg_dsc->bNumInterfaces; ++intf_num) {
        struct interface_descriptor *intf_dsc = NULL;
        struct mt_intf *intf = NULL;
        int alt_num = 0;

        intf = cfg->interface_array[intf_num];
        if (!intf) {
            tty_err("intface is NULL\n");
            return -EINVAL;
        }

        /* loop over all interface alternates */
        for (alt_num = 0; alt_num < intf->alternates; ++alt_num) {
            struct mt_altsetting *alt = NULL;
            int ep_num = 0;

            alt = intf->altsetting_array + alt_num;
            if (!alt) {
                tty_err("altsetting is NULL\n");
                return -EINVAL;
            }

            intf_dsc = alt->p_interface_desc;
            if (!intf_dsc) {
                tty_err("intf_dsc is NULL\n");
                return -EINVAL;
            }

            /* copy descriptor for this interface */
            memcpy(src_data, intf_dsc, sizeof(*intf_dsc));
            src_data += sizeof(*intf_dsc);
            if (alt->p_header_function_desc) {
                memcpy(src_data, alt->p_header_function_desc,
                       sizeof(*alt->p_header_function_desc));
                src_data += sizeof(*alt->p_header_function_desc);
            }

            if (alt->p_abstract_control_desc) {
                memcpy(src_data, alt->p_abstract_control_desc,
                       sizeof(*alt->p_abstract_control_desc));
                src_data += sizeof(*alt->p_abstract_control_desc);
            }

            if (alt->p_union_function_desc) {
                memcpy(src_data, alt->p_union_function_desc,
                       sizeof(*alt->p_union_function_desc));
                src_data += sizeof(*alt->p_union_function_desc);
            }
            if (alt->p_call_management_desc) {
                memcpy(src_data, alt->p_call_management_desc,
                       sizeof(*alt->p_call_management_desc));
                src_data += sizeof(*alt->p_call_management_desc);
            }
            /* iterate across endpoints for this alternate interface */
            for (ep_num = 0; ep_num < alt->endpoints; ++ep_num) {
                struct endpoint_descriptor *ep_desc = alt->pp_eps_desc_array[ep_num];
                if (!ep_desc) {
                    tty_err("ep_desc is NULL\n");
                    return -EINVAL;
                }

                /* copy descriptor for this endpoint */
                memcpy(src_data, ep_desc, sizeof(*ep_desc));
                src_data += sizeof(*ep_desc);
                if (mt_serial_dev->speed == SSUSB_SPEED_SUPER) {
                    struct ss_ep_comp_descriptor *ep_comp = alt->p_ss_ep_comp_desc[ep_num];
                    if (!ep_comp) {
                        tty_err("ep_comp is NULL\n");
                        return -EINVAL;
                    }
                    memcpy(src_data, ep_comp, sizeof(*ep_comp));
                    src_data += sizeof(*ep_comp);
                }
            }
        }
    }
    return 0;
}

/* Initialize the usb client port. */
int serial_usb_init(void)
{
    int ret;

    serial_online = 0;
    /* initialize usb variables */
    serial_device_init();
    serial_config_init();
    serial_interfaces_init();
    serial_strings_init();
    copy_all_descs();
    ret = udc_register_gadget(mt_serial_dev, 0);
    if (ret != UDC_CLASS_ONLINE) {
        ret = udc_init(0);
        if (ret)
            return ret;
    }

    serial_bind();
    udc_pullup(mt_serial_dev->private, true);
    return 0;
}

void serial_usb_exit(void)
{
    udc_pullup(mt_serial_dev->private, false);
    serial_unbind();
}

int serial_port_is_present(void)
{
    return (acm_port_state & CDC_DTR_MASK);
}

int serial_configured(void)
{
    return serial_online;
}

static struct usb_acm_line_coding g_line_coding = {
    921600, 0, 0, 8,
};

static int serial_ep0_setup(struct udc_request *req, const struct usb_setup *usb_ctrl)
{
    switch (usb_ctrl->bRequest) {
        case CDCACM_REQ_SET_LINE_CODING:
            tty_log("CDCACM_REQ_SET_LINE_CODING\n");
            break;
        case CDCACM_REQ_GET_LINE_CODING:
            tty_log("CDCACM_REQ_GET_LINE_CODING\n");
            memcpy(req->buffer, &g_line_coding, sizeof(g_line_coding));
            req->length = sizeof(g_line_coding);
            break;
        case CDCACM_REQ_SET_CONTROL_LINE_STATE:
            tty_log("CDCACM_REQ_SET_CONTROL_LINE_STATE, setup->wValue=%x\n", usb_ctrl->wValue);
            acm_port_state = usb_ctrl->wValue;
            break;
        case CDCACM_REQ_SEND_BREAK:
            tty_log("CDCACM_REQ_SEND_BREAK\n");
            break;
        default:
            return -1;
    }

    return 0;
}

static int serial_set_alt(struct udc_request *req, const struct usb_setup *usb_ctrl)
{
    int i;
    int ep_num = 0;
    struct mt_intf *intf = NULL;
    struct mt_altsetting *alt = NULL;
    struct endpoint_descriptor *ep_desc = NULL;

    if (usb_ctrl->wLength != 0) {
        tty_err("Request Length Error\n");
        return -1;
    }

    if (usb_ctrl->wIndex > NUM_INTERFACES) {
        tty_err("Interface Number Error\n");
        return -1;
    }

    if (usb_ctrl->wValue >= mt_serial_config->interface_array[usb_ctrl->wIndex]->alternates) {
        tty_err("Alternate Setting Number Error\n");
        return -1;
    }

    intf = mt_serial_config->interface_array[usb_ctrl->wIndex];
    alt = intf->altsetting_array + usb_ctrl->wValue;

    for (ep_num = 0; ep_num < alt->endpoints; ++ep_num) {
        ep_desc = alt->pp_eps_desc_array[ep_num];
        for (i = 1; i <= mt_serial_dev->num_eps; i++) {
            struct udc_endpoint *ept = mt_serial_ep[i];
            if ((ept->num == (ep_desc->bEndpointAddress & 0xf)) &&
                (ept->in == !!(ep_desc->bEndpointAddress & 0x80))) {
                udc_endpoint_disable(ept);
                tty_crit("ept%d%s disable\n", ept->num, ept->in ? "in" : "out");
            }
        }
    }

    for (ep_num = 0; ep_num < alt->endpoints; ++ep_num) {
        ep_desc = alt->pp_eps_desc_array[ep_num];
        for (i = 1; i <= mt_serial_dev->num_eps; i++) {
            struct udc_endpoint *ept = mt_serial_ep[i];
            if ((ept->num == (ep_desc->bEndpointAddress & 0xf)) &&
                (ept->in == !!(ep_desc->bEndpointAddress & 0x80))) {
                ept->in = !!GET_EP_DIR(ep_desc->bEndpointAddress);
                ept->binterval = ep_desc->bInterval;
                ept->type = ep_desc->bmAttributes;
                ept->maxpkt = ep_desc->wMaxPacketSize;
                ept->desc = ep_desc;
                udc_endpoint_enable(mt_serial_dev->private, ept);
                tty_crit("ept%d%s enable\n", ept->num, ept->in ? "in" : "out");
            }
        }
    }

    return 0;
}

static void serial_assign_fs_descs(void)
{
    int is_ss = 0;

    tty_log("%s %d\n", __func__, __LINE__);
    mt_serial_dev->speed = SSUSB_SPEED_FULL;
    mt_serial_dev->dev_desc = get_device_desc(is_ss);
    mt_serial_dev->dev_qualifier_desc = get_qualified_desc(is_ss);
    mt_serial_data_alt_if->pp_eps_desc_array = serial_fs_data_ep_descriptor_ptrs;
    mt_serial_comm_alt_if->pp_eps_desc_array = serial_fs_comm_ep_descriptor_ptrs;
    mt_serial_config->config_desc = get_config_desc(is_ss);

    update_eps_from_desc(serial_fs_ep_descriptors);
    copy_all_descs();
}

static void serial_assign_hs_descs(void)
{
    int is_ss = 0;

    tty_log("%s %d\n", __func__, __LINE__);
    mt_serial_dev->speed = SSUSB_SPEED_HIGH;
    mt_serial_dev->dev_desc = get_device_desc(is_ss);
    mt_serial_dev->dev_qualifier_desc = get_qualified_desc(is_ss);
    mt_serial_data_alt_if->pp_eps_desc_array = serial_hs_data_ep_descriptor_ptrs;
    mt_serial_comm_alt_if->pp_eps_desc_array = serial_hs_comm_ep_descriptor_ptrs;
    mt_serial_config->config_desc = get_config_desc(is_ss);

    update_eps_from_desc(serial_hs_ep_descriptors);
    copy_all_descs();
}

static void serial_assign_ss_descs(void)
{
    int is_ss = 1;

    tty_log("%s %d\n", __func__, __LINE__);
    mt_serial_dev->speed = SSUSB_SPEED_SUPER;
    mt_serial_dev->dev_desc = get_device_desc(is_ss);
    mt_serial_dev->dev_qualifier_desc = get_qualified_desc(is_ss);
    mt_serial_data_alt_if->pp_eps_desc_array = serial_ss_data_ep_descriptor_ptrs;
    mt_serial_data_alt_if->p_ss_ep_comp_desc = serial_ss_data_ep_comp_descriptor_ptrs;
    mt_serial_comm_alt_if->pp_eps_desc_array = serial_ss_comm_ep_descriptor_ptrs;
    mt_serial_comm_alt_if->p_ss_ep_comp_desc = serial_ss_comm_ep_comp_descriptor_ptrs;
    mt_serial_config->config_desc = get_config_desc(is_ss);

    update_eps_from_desc(serial_ss_ep_descriptors);
    update_eps_comp_from_desc(serial_ss_ep_comp_descriptor);
    copy_all_descs();
}

static void serial_assign_descs(USB_SPEED speed)
{
    tty_log("%s %d\n", __func__, __LINE__);
    switch (speed) {
        case SSUSB_SPEED_FULL:
            serial_assign_fs_descs();
            break;
        case SSUSB_SPEED_HIGH:
            serial_assign_hs_descs();
            break;
        case SSUSB_SPEED_SUPER:
        case SSUSB_SPEED_SUPER_PLUS:
            serial_assign_ss_descs();
            break;
        default:
            serial_assign_fs_descs();
            tty_log("%s, default FS's descs\n", __func__);
            break;
    }
}

static void serial_req_complete(udc_request_t *req, unsigned int actual, int status)
{
    portBASE_TYPE pxHigherPriorityTaskWoken = pdFALSE;

    req->length = actual;
    if (pdPASS != xSemaphoreGiveFromISR(req->xSemaphore, &pxHigherPriorityTaskWoken))
        tty_err("%s: give semphore fail\n", __func__);

    if (req->status < 0)
        tty_err("%s req:%p transaction failed\n", __func__, req);
}

static int serial_wait_request_complete(udc_request_t *req)
{
    if (pdPASS != xSemaphoreTake(req->xSemaphore, portMAX_DELAY)) {
        tty_err("%s: wait timeout\n", __func__);
        return -1;
    }

    return 0;
}
int serial_usb_read(void *_buf, unsigned int len)
{
    udc_request_t *req = serial_ep_out->req;
    void *buf;
    int ret;

    buf = pvPortMallocNC(len);
    if (buf == NULL) {
        tty_err("%s: Out of memory\n", __func__);
        return -1;
    }

    tty_log("%s buf:%p, len:%d\n", __func__, buf, len);
    if (len > MAX_USBFS_BULK_SIZE) {
        tty_err("%s req length > supported MAX:%d requested:%d\n",
                __func__, MAX_USBFS_BULK_SIZE, len);
        goto oops;
    }

    req->buffer = buf;
    req->length = len;
    req->complete = serial_req_complete;
    ret = udc_request_queue(serial_ep_out, req);
    if (ret < 0) {
        tty_err("%s queue failed, ret:%d \n", __func__, ret);
        goto oops;
    }

    serial_wait_request_complete(req);
    if (req->status < 0) {
        tty_err("%s transaction failed\n", __func__);
        goto oops;
    }

    memcpy(_buf, buf, req->length);
    vPortFreeNC(buf);
    buf = NULL;
    return req->length;

oops:
    vPortFreeNC(buf);
    buf = NULL;
    return -1;
}

int serial_usb_write(void *_buf, unsigned int len)
{
    udc_request_t *req = serial_ep_in->req;
    void *buf;
    int ret;

    buf = pvPortMallocNC(len);
    if (buf == NULL) {
        tty_err("%s: Out of memory\n", __func__);
        return -1;
    }

    memcpy(buf, _buf, len);
    tty_log("%s, buf:%p, len:%d\n", __func__, _buf, len);
    if (len > MAX_USBFS_BULK_SIZE) {
        tty_err("%s req length > supported MAX:%d requested:%d\n",
                __func__, MAX_USBFS_BULK_SIZE, len);
        goto oops;
    }

    req->buffer = buf;
    req->length = len;
    req->complete = serial_req_complete;
    ret = udc_request_queue(serial_ep_in, req);
    if (ret < 0) {
        tty_err("%s() queue failed\n", __func__);
        goto oops;
    }

    serial_wait_request_complete(req);
    if (req->status < 0) {
        tty_err("%s transaction failed\n", __func__);
        goto oops;
    }

    vPortFreeNC(buf);
    buf = NULL;
    return req->length;

oops:
    vPortFreeNC(buf);
    buf = NULL;
    return -1;
}

int serial_usbtty_putcn(char *buf, int count)
{
    return serial_usb_write(buf, count);
}

int serial_usbtty_getcn(char *buf, int count)
{
    return serial_usb_read(buf, count);
}
