/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2008-2010 coresystems GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <usb.h>
#include <xhci_private.h>

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_nvic.h"
#include "hal_sleep_manager_internal.h"
#include "hal_sleep_manager_platform.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

hci_t *usb_hcs;

hci_t *new_controller(void)
{
    hci_t *controller = pvPortMalloc(sizeof(hci_t));
    if (!controller) {
        usb_err("Allocate xhci controller fail\n");
        return NULL;
    }

    memset(controller, 0, sizeof(hci_t));
    controller->next = usb_hcs;
    usb_hcs = controller;
    usb_debug("controller:%x next:%x\n", (u32)controller, (u32)(controller->next));
    return controller;
}

void detach_controller(hci_t *controller)
{
    if (controller == NULL)
        return;

    usb_detach_device(controller, 0);   /* tear down root hub tree */

    if (usb_hcs == controller) {
        usb_hcs = controller->next;
    } else {
        hci_t *it = usb_hcs;
        while (it != NULL) {
            if (it->next == controller) {
                it->next = controller->next;
                return;
            }
            it = it->next;
        }
    }
}

/**
 * Shut down all controllers
 */
int usb_exit(void)
{
    while (usb_hcs != NULL) {
        usb_hcs->shutdown(usb_hcs);
    }
    return 0;
}

/**
 * Polls all hubs on all USB controllers, to find out about device changes
 */
void usb_poll(TimerHandle_t expiredTimer)
{
    if (usb_hcs == 0)
        return;
    hci_t *controller = usb_hcs;
    u8 max_slots = ((xhci_t *)controller->instance)->max_slots_en;

    while (controller != NULL) {
        int i;
        for (i = 0; i < max_slots + 1 ; i++) {
            if (controller->devices[i] != 0)
                controller->devices[i]->poll(controller->devices[i]);
        }
        controller = controller->next;
    }

    return;
}

usbdev_t *init_device_entry(hci_t *controller, int i)
{
    usbdev_t *dev = pvPortMalloc(sizeof(usbdev_t));
    if (!dev) {
        usb_err("no memory to allocate device structure\n");
        return NULL;
    }

    memset(dev, 0, sizeof(usbdev_t));
    if (controller->devices[i] != 0)
        usb_err("warning: device %d reassigned?\n", i);
    controller->devices[i] = dev;
    dev->controller = controller;
    dev->address = -1;
    dev->hub = -1;
    dev->port = -1;
    dev->init = usb_nop_init;
    dev->init(controller->devices[i]);
    usb_debug("controller->devices[%d]:%p\n", i, dev);

#ifdef HAL_SLEEP_MANAGER_ENABLED
    if (i > 0)
        hal_sleep_manager_lock_sleep(SLEEP_LOCK_CM33);
#endif
    return dev;
}

struct urb *usb_alloc_urb(int iso_packets)
{
    struct urb *urb;

    urb = pvPortMalloc(sizeof(struct urb) +
                       iso_packets * sizeof(struct usb_iso_packet_desc));
    if (!urb) {
        usb_err("%s: fail\n", __func__);
        return NULL;
    }

    memset(urb, 0, sizeof(*urb));
    if (iso_packets) {
        urb->hcpriv = pvPortMalloc(sizeof(struct urb_priv) +
                                   iso_packets * sizeof(struct xhci_td));
        if (!urb->hcpriv) {
            usb_err("%s: alloc urb_priv fail.\n", __func__);
            vPortFree(urb);
            return NULL;
        }

        memset(urb->hcpriv, 0, sizeof(struct urb_priv) +
               iso_packets * sizeof(struct xhci_td));
    }

    urb->xSemaphore = xSemaphoreCreateBinary();
    if (urb->xSemaphore == NULL) {
        usb_err("%s: create cmd semahpone fail\n", __func__);
        if (iso_packets)
            vPortFree(urb->hcpriv);

        vPortFree(urb);
        return NULL;
    }

    return urb;
}

void usb_free_urb(struct urb *urb)
{
    if (!urb) {
        return;
    }

    if (urb->number_of_packets) {
        usb_err("%s: free hcpriv\n", __func__);
        vPortFree(urb->hcpriv);
    }

    vSemaphoreDelete(urb->xSemaphore);
    vPortFree(urb);
    return;
}

static int usb_wait_urb(struct urb *urb)
{
    if (pdPASS != xSemaphoreTake(urb->xSemaphore, 5000 / portTICK_RATE_MS)) {
        usb_err("%s: wait timeout\n", __func__);
        return -1;
    }

    return 0;
}

static void usb_fill_control_urb(struct urb *urb, direction_t in,
                                 void *devreq, int data_length, u8 *data,
                                 usb_complete_t complete_fn, void *context)
{
    urb->in = in;
    urb->setup_packet = devreq;
    urb->buffer = data;
    urb->transfer_dma = virt_to_phys(data);
    urb->transfer_buffer_length = data_length;

    if (complete_fn) {
        urb->complete = complete_fn;
        urb->context = context;
    } else
        urb->complete = usb_wait_urb;
}

static void usb_fill_bulk_urb(struct urb *urb, endpoint_t *ep,
                              int data_length, u8 *data,
                              usb_complete_t complete_fn, void *context)
{
    urb->in = ep->direction;
    urb->ep = ep;
    urb->buffer = data;
    urb->transfer_dma = virt_to_phys(data);
    urb->transfer_buffer_length = data_length;

    if (complete_fn) {
        urb->complete = complete_fn;
        urb->context = context;
    } else
        urb->complete = usb_wait_urb;
}

int usb_control_msg(usbdev_t *dev, direction_t in,
                    void *devreq, int data_length, u8 *data)
{
    struct urb *urb;
    int ret;

    urb = usb_alloc_urb(0);
    if (!urb) {
        return -1;
    }

    usb_fill_control_urb(urb, in, devreq, data_length, data, NULL, NULL);

    ret = dev->controller->control(dev, urb);
    if (ret)
        return -1;

    ret = urb->complete(urb);
    if (ret)
        return -1;

    ret = urb->actual_length;
    usb_free_urb(urb);
    return ret;
}

int usb_bulk_msg(endpoint_t *ep, int size, u8 *src)
{
    struct urb *urb;
    int ret;

    urb = usb_alloc_urb(0);
    if (!urb) {
        return -1;
    }

    usb_fill_bulk_urb(urb, ep, size, src, NULL, NULL);

    ret = ep->dev->controller->bulk(ep, urb);
    if (ret)
        return -1;

    ret = urb->complete(urb);
    if (ret)
        return -1;

    ret = urb->actual_length;
    usb_free_urb(urb);
    return ret;
}

int set_interface(usbdev_t *dev, int interface, int altsetting)
{
    int ret;
    dev_req_t *dr;

    dr = pvPortMallocNC(sizeof(dev_req_t));
    if (dr == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;;
    };

    dr->bmRequestType = USB_SET_INTERFACE;
    dr->data_dir = host_to_device;
    dr->bRequest = SET_INTERFACE;
    dr->wValue = altsetting;
    dr->wIndex = interface;
    dr->wLength = 0;

    ret = usb_control_msg(dev, OUT, dr, 0, 0);
    vPortFreeNC((void *)dr);
    return ret;
}


int set_feature(usbdev_t *dev, int endp, int feature, int rtype)
{
    int ret;
    dev_req_t *dr;

    dr = pvPortMallocNC(sizeof(dev_req_t));
    if (dr == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;;
    };

    dr->bmRequestType = rtype;
    dr->data_dir = host_to_device;
    dr->bRequest = SET_FEATURE;
    dr->wValue = feature;
    dr->wIndex = endp;
    dr->wLength = 0;

    ret = usb_control_msg(dev, OUT, dr, 0, 0);
    vPortFreeNC((void *)dr);
    return ret;
}

int get_status(usbdev_t *dev, int intf, int rtype, int len, void *data)
{
    int ret;
    void *buf;
    dev_req_t *dr;

    buf = pvPortMallocNC(len);
    if (buf == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;;
    }

    dr = pvPortMallocNC(sizeof(dev_req_t));
    if (dr == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;;
    };

    dr->bmRequestType = rtype;
    dr->data_dir = device_to_host;
    dr->bRequest = GET_STATUS;
    dr->wValue = 0;
    dr->wIndex = intf;
    dr->wLength = len;

    ret = usb_control_msg(dev, IN, dr, len, buf);
    memcpy(data, buf, len);
    vPortFreeNC(buf);
    vPortFreeNC((void *)dr);
    return ret;
}

/*
 * Certain Lexar / Micron USB 2.0 disks will fail the get_descriptor(DT_CFG)
 * call due to timing issues. Work around this by making extra attempts on
 * failure.
 */
#define GET_DESCRIPTOR_TRIES 3

usb_descheader_t *usb_get_nextdesc(u8 *buffer, u32 *length)
{
    usb_descheader_t *pnext;

    *length += ((usb_descheader_t *)buffer)->bLength;
    pnext = (usb_descheader_t *)(buffer + \
                                 ((usb_descheader_t *)buffer)->bLength);

    return pnext;
}

int get_descriptor(usbdev_t *dev, int rtype, int desc_type, int desc_idx,
                   void *data, int len)
{
    dev_req_t *dr;
    int fail_tries = 0;
    int ret = 0;

    dr = pvPortMallocNC(sizeof(dev_req_t));
    if (dr == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;;
    };

    while (fail_tries++ < GET_DESCRIPTOR_TRIES) {
        dr->bmRequestType = rtype;
        dr->bRequest = GET_DESCRIPTOR;
        dr->wValue = desc_type << 8 | desc_idx;
        dr->wIndex = 0;
        dr->wLength = len;

        ret = usb_control_msg(dev, IN, dr, len, data);
        usb_debug("%s: try:%d ret=%d\n", __func__, fail_tries, ret);

        if (ret == len)
            break;
        udelay(10);
    }

    vPortFreeNC((void *)dr);
    return ret;
}

int set_configuration(usbdev_t *dev)
{
    int ret;
    dev_req_t *dr;

    dr = pvPortMallocNC(sizeof(dev_req_t));
    if (dr == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;;
    };

    dr->bmRequestType = 0;
    dr->bRequest = SET_CONFIGURATION;
    dr->wValue = dev->configuration->bConfigurationValue;
    dr->wIndex = 0;
    dr->wLength = 0;

    ret = usb_control_msg(dev, OUT, dr, 0, 0);
    vPortFreeNC((void *)dr);
    return ret;
}

int clear_feature(usbdev_t *dev, int endp, int feature, int rtype)
{
    int ret;
    dev_req_t *dr;

    dr = pvPortMallocNC(sizeof(dev_req_t));
    if (dr == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;;
    };

    dr->bmRequestType = rtype;
    dr->data_dir = host_to_device;
    dr->bRequest = CLEAR_FEATURE;
    dr->wValue = feature;
    dr->wIndex = endp;
    dr->wLength = 0;

    ret = usb_control_msg(dev, OUT, dr, 0, 0) < 0;
    vPortFreeNC((void *)dr);
    return ret;
}

int clear_stall(endpoint_t *ep)
{
    int ret = clear_feature(ep->dev, ep->endpoint, ENDPOINT_HALT,
                            gen_bmRequestType(host_to_device, standard_type, endp_recp));
    ep->toggle = 0;
    return ret;
}

int usb_decode_mps0(usb_speed speed, u8 bMaxPacketSize0)
{
    switch (speed) {
        case LOW_SPEED:
            if (bMaxPacketSize0 != 8) {
                usb_debug("Invalid MPS0: 0x%02x\n", bMaxPacketSize0);
                bMaxPacketSize0 = 8;
            }
            return bMaxPacketSize0;
        case FULL_SPEED:
            switch (bMaxPacketSize0) {
                case 8:
                case 16:
                case 32:
                case 64:
                    return bMaxPacketSize0;
                default:
                    usb_debug("Invalid MPS0: 0x%02x\n", bMaxPacketSize0);
                    return 8;
            }
        case HIGH_SPEED:
            if (bMaxPacketSize0 != 64) {
                usb_debug("Invalid MPS0: 0x%02x\n", bMaxPacketSize0);
                bMaxPacketSize0 = 64;
            }
            return bMaxPacketSize0;
        case SUPER_SPEED:
        /* Intentional fallthrough */
        case SUPER_SPEED_PLUS:
            if (bMaxPacketSize0 != 9) {
                usb_debug("Invalid MPS0: 0x%02x\n", bMaxPacketSize0);
                bMaxPacketSize0 = 9;
            }
            return 1 << bMaxPacketSize0;
        default:    /* GCC is stupid and cannot deal with enums correctly */
            return 8;
    }
}

int speed_to_default_mps(usb_speed speed)
{
    switch (speed) {
        case LOW_SPEED:
            return 8;
        case FULL_SPEED:
        case HIGH_SPEED:
            return 64;
        case SUPER_SPEED:
        /* Intentional fallthrough */
        case SUPER_SPEED_PLUS:
        default:
            return 512;
    }
}

/* Normalize bInterval to log2 of microframes */
int usb_decode_interval(usb_speed speed, const endpoint_type type, const unsigned char bInterval)
{
#define LOG2(a) ((sizeof(unsigned) << 3) - __builtin_clz(a) - 1)
    switch (speed) {
        case LOW_SPEED:
            switch (type) {
                case ISOCHRONOUS:
                case INTERRUPT:
                    return LOG2(bInterval) + 3;
                default:
                    return 0;
            }
        case FULL_SPEED:
            switch (type) {
                case ISOCHRONOUS:
                    return (bInterval - 1) + 3;
                case INTERRUPT:
                    return LOG2(bInterval) + 3;
                default:
                    return 0;
            }
        case HIGH_SPEED:
            switch (type) {
                case ISOCHRONOUS:
                case INTERRUPT:
                    return bInterval - 1;
                default:
                    return LOG2(bInterval);
            }
        case SUPER_SPEED:
        /* Intentional fallthrough */
        case SUPER_SPEED_PLUS:
            switch (type) {
                case ISOCHRONOUS:
                case INTERRUPT:
                    return bInterval - 1;
                default:
                    return 0;
            }
        default:
            return 0;
    }
#undef LOG2
}

static void print_config_descs(configuration_descriptor_t *config_desc)
{
    if (!config_desc)
        return;

    usb_debug("****** Configuration Descriptor ****** \n");
    usb_debug("bLength:%x \n", config_desc->bLength);
    usb_debug("bDescriptorType:%x \n", config_desc->bDescriptorType);
    usb_debug("wTotalLength:%x \n", config_desc->wTotalLength);
    usb_debug("bNumInterfaces:%x \n", config_desc->bNumInterfaces);
    usb_debug("bConfigurationValue:%x \n", config_desc->bConfigurationValue);
    usb_debug("iConfiguration:%x \n", config_desc->iConfiguration);
    usb_debug("bmAttributes:%x \n", config_desc->bmAttributes);
    usb_debug("bMaxPower:%x \n", config_desc->bMaxPower);
}

static void print_interface_descs(interface_descriptor_t *intf_desc)
{
    if (!intf_desc)
        return;

    usb_debug("****** Interface Descriptor ****** \n");
    usb_debug("bLength:%x \n", intf_desc->bLength);
    usb_debug("bDescriptorType:%x \n", intf_desc->bDescriptorType);
    usb_debug("bInterfaceNumber:%x \n", intf_desc->bInterfaceNumber);
    usb_debug("bAlternateSetting:%x \n", intf_desc->bAlternateSetting);
    usb_debug("bNumEndpoints:%x \n", intf_desc->bNumEndpoints);
    usb_debug("bInterfaceClass:%x \n", intf_desc->bInterfaceClass);
    usb_debug("bInterfaceSubClass:%x \n", intf_desc->bInterfaceSubClass);
    usb_debug("bInterfaceProtocol:%x \n", intf_desc->bInterfaceProtocol);
    usb_debug("iInterface:%x \n", intf_desc->iInterface);
}

static void print_endpoint_descs(endpoint_descriptor_t *ep_desc)
{
    if (!ep_desc)
        return;

    usb_debug("****** Endpoint Descriptor ****** \n");
    usb_debug("bLength:%x \n", ep_desc->bLength);
    usb_debug("bDescriptorType:%x \n", ep_desc->bDescriptorType);
    usb_debug("bEndpointAddress:%x \n", ep_desc->bEndpointAddress);
    usb_debug("bmAttributes:%x \n", ep_desc->bmAttributes);
    usb_debug("wMaxPacketSize:%x \n", ep_desc->wMaxPacketSize);
    usb_debug("bInterval:%x \n", ep_desc->bInterval);
}

static void assign_config_descs(configuration_descriptor_t *config_desc, char *buf)
{
    config_desc->bLength = *(u8 *)(buf + 0);
    config_desc->bDescriptorType = *(u8 *)(buf + 1);
    config_desc->wTotalLength = LE16(buf + 2);
    config_desc->bNumInterfaces = *(u8 *)(buf + 4);
    config_desc->bConfigurationValue = *(u8 *)(buf + 5);
    config_desc->iConfiguration = *(u8 *)(buf + 6);
    config_desc->bmAttributes = *(u8 *)(buf + 7);
    config_desc->bMaxPower = *(u8 *)(buf + 8);
    print_config_descs(config_desc);
}

static void assign_interface_descs(interface_descriptor_t *intf_desc, char *buf)
{
    intf_desc->bLength  = *(u8 *)(buf + 0);
    intf_desc->bDescriptorType = *(u8 *)(buf + 1);
    intf_desc->bInterfaceNumber = *(u8 *)(buf + 2);
    intf_desc->bAlternateSetting = *(u8 *)(buf + 3);
    intf_desc->bNumEndpoints = *(u8 *)(buf + 4);
    intf_desc->bInterfaceClass = *(u8 *)(buf + 5);
    intf_desc->bInterfaceSubClass = *(u8 *)(buf + 6);
    intf_desc->bInterfaceProtocol = *(u8 *)(buf + 7);
    intf_desc->iInterface = *(u8 *)(buf + 8);
    print_interface_descs(intf_desc);
}

static void assign_endpoint_descs(endpoint_descriptor_t *ep_desc, char *buf)
{
    ep_desc->bLength = *(u8 *)(buf + 0);
    ep_desc->bDescriptorType = *(u8 *)(buf + 1);
    ep_desc->bEndpointAddress = *(u8 *)(buf + 2);
    ep_desc->bmAttributes = *(u8 *)(buf + 3);
    ep_desc->wMaxPacketSize = LE16(buf + 4);
    ep_desc->bInterval = *(u8 *)(buf + 6);
    print_endpoint_descs(ep_desc);
}

static void usb_parse_standard_descriptors(usbdev_t *dev)
{
    usb_descheader_t *desc;
    struct dev_config *dev_config;
    struct dev_interface *intf;
    endpoint_descriptor_t *ep;
    u32 config_len;
    u32 length = USB_CONFIG_DESC_SIZE;
    u8 if_index = 0;
    u8 ep_index = 0;
    u8 last_ep_address = 0;
    char *transfertypes[4] = {
        "control", "isochronous", "bulk", "interrupt"
    };

    dev->num_endp = 1;
    dev_config = &dev->config;
    desc = (usb_descheader_t *)dev->configuration;
    config_len = dev->configuration->wTotalLength;

    assign_config_descs(&dev_config->config_desc, (char *)desc);

    while ((if_index < MAX_INTERFACES) && (length < config_len)) {
        desc = usb_get_nextdesc((u8 *)desc, &length);
        /* get standard interface info */
        if (desc->bDescriptorType   == DT_INTF) {
            intf = &dev_config->intf[if_index];
            assign_interface_descs(&intf->intf_desc, (char *)desc);
            ep_index = 0;
            while ((ep_index < intf->intf_desc.bNumEndpoints) && (length < config_len)) {
                desc = usb_get_nextdesc((u8 *)desc, &length);
                if (desc->bDescriptorType == DT_ENDP) {
                    ep = &intf->ep_desc[ep_index];
                    assign_endpoint_descs(ep, (char *)desc);
                    ep_index++;

                    usb_crit(" #Endpoint %d (%s), max packet size %d, type %s\n",
                             ep->bEndpointAddress & 0x7f,
                             (ep->bEndpointAddress & 0x80) ? "in" : "out",
                             ep->wMaxPacketSize,
                             transfertypes[ep->bmAttributes & 0x3]);

                    if (last_ep_address != ep->bEndpointAddress) {
                        endpoint_t *epn = &dev->endpoints[dev->num_endp++];
                        epn->dev = dev;
                        epn->endpoint = ep->bEndpointAddress;
                        epn->toggle = 0;
                        if (ep->bmAttributes == 5)
                            epn->maxpacketsize = 0x13fc;
                        else
                            epn->maxpacketsize = ep->wMaxPacketSize;
                        epn->direction = (ep->bEndpointAddress & 0x80) ? IN : OUT;
                        epn->type = ep->bmAttributes & 0x3;
                        epn->interval = usb_decode_interval(dev->speed, epn->type,
                                                            ep->bInterval);
                        usb_debug(" #dev->num_endp: %d\n", dev->num_endp);
                    }
                    last_ep_address = ep->bEndpointAddress;
                }
            }
            if_index++;
        }
    }
}

static int set_address(hci_t *controller, usb_speed speed, int hubport, int hubaddr)
{
    usbdev_t *dev = controller->set_address(controller, speed,
                                            hubport, hubaddr);
    if (!dev) {
        usb_err("set_address failed\n");
        return -1;
    }

    dev->descriptor = pvPortMallocNC(sizeof(*dev->descriptor));
    if (!dev->descriptor || get_descriptor(dev, DR_DESC, DT_DEV, 0,
                                           dev->descriptor, sizeof(*dev->descriptor))
        != sizeof(*dev->descriptor)) {
        usb_err("get_descriptor(DT_DEV) failed\n");
        usb_detach_device(controller, dev->address);
        return -1;
    }

    usb_debug("* found device (0x%04x:0x%04x, USB %x.%x, MPS0: %d)\n",
              dev->descriptor->idVendor, dev->descriptor->idProduct,
              dev->descriptor->bcdUSB >> 8, dev->descriptor->bcdUSB & 0xff,
              dev->endpoints[0].maxpacketsize);
    dev->quirks = usb_quirk_check(dev->descriptor->idVendor,
                                  dev->descriptor->idProduct);

    usb_debug("device has %d configurations\n",
              dev->descriptor->bNumConfigurations);
    if (dev->descriptor->bNumConfigurations == 0) {
        /* device isn't usable */
        usb_debug("... no usable configuration!\n");
        usb_detach_device(controller, dev->address);
        return -1;
    }

    u16 *buf = (u16 *)pvPortMallocNC(4);
    if (buf == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;
    }

    if (get_descriptor(dev, DR_DESC, DT_CFG, 0, buf, 4) != 4) {
        usb_err("first get_descriptor(DT_CFG) failed\n");
        usb_detach_device(controller, dev->address);
        vPortFreeNC(buf);
        return -1;
    }
    /* workaround for some USB devices: wait until they're ready, or
     * they send a NAK when they're not allowed to do. 1ms is enough */
    mdelay(1);

    dev->configuration = pvPortMallocNC(buf[1]);
    if (!dev->configuration) {
        usb_err("could not allocate %d bytes for DT_CFG\n", buf[1]);
        usb_detach_device(controller, dev->address);
        vPortFreeNC(buf);
        return -1;
    }
    if (get_descriptor(dev, DR_DESC, DT_CFG, 0, dev->configuration,
                       buf[1]) != buf[1]) {
        usb_err("get_descriptor(DT_CFG) failed\n");
        usb_detach_device(controller, dev->address);
        vPortFreeNC(buf);
        return -1;
    }
    configuration_descriptor_t *cd = dev->configuration;
    if (cd->wTotalLength != buf[1]) {
        usb_err("configuration descriptor size changed, aborting\n");
        usb_detach_device(controller, dev->address);
        vPortFreeNC(buf);
        return -1;
    }

    vPortFreeNC(buf);

    /*
     * If the device is not well known (ifnum == -1), we use the first
     * interface we encounter, as there was no need to implement something
     * else for the time being. If you need it, see the SetInterface and
     * GetInterface functions in the USB specification and set it yourself.
     */
    usb_debug("device has %x interfaces\n", cd->bNumInterfaces);
    int ifnum = usb_interface_check(dev->descriptor->idVendor,
                                    dev->descriptor->idProduct);
    if (ifnum == 0)
        usb_crit("NOTICE: Your device should add to\n"
                 "the list of well-known quirks.\n");

    usb_parse_standard_descriptors(dev);
    usb_crit("End parse descriptor.\n");

    if ((controller->finish_device_config &&
         controller->finish_device_config(dev)) ||
        set_configuration(dev) < 0) {
        usb_debug("Could not finalize device configuration\n");
        usb_detach_device(controller, dev->address);
        return -1;
    }

    int class = dev->descriptor->bDeviceClass;
    if (class == 0 || class == 0xef)
        class = dev->config.intf[0].intf_desc.bInterfaceClass;

    enum {
        audio_device      = 0x01,
        comm_device       = 0x02,
        hid_device        = 0x03,
        physical_device   = 0x05,
        imaging_device    = 0x06,
        printer_device    = 0x07,
        msc_device        = 0x08,
        hub_device        = 0x09,
        cdc_device        = 0x0a,
        ccid_device       = 0x0b,
        security_device   = 0x0d,
        video_device      = 0x0e,
        healthcare_device = 0x0f,
        diagnostic_device = 0xdc,
        wireless_device   = 0xe0,
        misc_device       = 0xef,
    };
    usb_debug("Class: ");
    switch (class) {
        case audio_device:
            usb_debug("audio\n");
            dev->init = usb_uac_init;
            return dev->address;
            break;
        case comm_device:
            usb_debug("communication\n");
            break;
        case hid_device:
            usb_debug("HID\n");
            dev->init = usb_hid_init;
            return dev->address;
            break;
        case physical_device:
            usb_debug("physical\n");
            break;
        case imaging_device:
            usb_debug("camera\n");
            break;
        case printer_device:
            usb_debug("printer\n");
            break;
        case msc_device:
            usb_debug("MSC\n");
            dev->init = usb_msc_init;
            return dev->address;
            break;
        case hub_device:
            usb_debug("hub\n");
            dev->init = usb_hub_init;
            return dev->address;
            break;
        case cdc_device:
            usb_debug("CDC\n");
            break;
        case ccid_device:
            usb_debug("smartcard / CCID\n");
            break;
        case security_device:
            usb_debug("content security\n");
            break;
        case video_device:
            usb_debug("video\n");
            dev->init = usb_uvc_init;
            return dev->address;
            break;
        case healthcare_device:
            usb_debug("healthcare\n");
            break;
        case diagnostic_device:
            usb_debug("diagnostic\n");
            break;
        case wireless_device:
            usb_debug("wireless\n");
            break;
        default:
            usb_debug("unsupported class %x\n", class);
            break;
    }
    dev->init = usb_generic_init;
    return dev->address;
}

/*
 * Should be called by the hub drivers whenever a physical detach occurs
 * and can be called by usb class drivers if they are unsatisfied with a
 * malfunctioning device.
 */
void usb_detach_device(hci_t *controller, int devno)
{
    /* check if device exists, as we may have
       been called yet by the usb class driver */
    if (controller->devices[devno]) {
        controller->devices[devno]->destroy(controller->devices[devno]);

        if (controller->destroy_device)
            controller->destroy_device(controller, devno);

        vPortFreeNC(controller->devices[devno]->descriptor);
        controller->devices[devno]->descriptor = NULL;
        vPortFreeNC(controller->devices[devno]->configuration);
        controller->devices[devno]->configuration = NULL;

        /* Tear down the device itself *after* destroy_device()
         * has had a chance to interrogate it. */
        vPortFree(controller->devices[devno]);
        controller->devices[devno] = NULL;
#ifdef HAL_SLEEP_MANAGER_ENABLED
        hal_sleep_manager_unlock_sleep(SLEEP_LOCK_CM33);
#endif
    }
}

int usb_attach_device(hci_t *controller, int hubaddress, int port, usb_speed speed)
{
    static const char *speeds[] = { "full", "low", "high", "super", "ultra" };
    usb_crit("%sspeed device\n", (speed < (usb_speed)ARRAY_SIZE(speeds))
             ? speeds[speed] : "invalid value - no");
    int newdev = set_address(controller, speed, port, hubaddress);
    if (newdev == -1)
        return -1;
    usbdev_t *newdev_t = controller->devices[newdev];
    newdev_t->init(newdev_t);
    /* init() may have called usb_detach_device() yet, so check */
    return controller->devices[newdev] ? newdev : -1;
}

static void usb_generic_destroy(usbdev_t *dev)
{
    if (usb_generic_remove)
        usb_generic_remove(dev);
}

void usb_generic_init(usbdev_t *dev)
{
    dev->data = NULL;
    dev->destroy = usb_generic_destroy;

    if (usb_generic_create)
        usb_generic_create(dev);

    if (dev->data == NULL) {
        usb_debug("Detaching device not used by payload\n");
        usb_detach_device(dev->controller, dev->address);
    }
}

/*
 * returns the speed is above SUPER_SPEED or not
 */
_Bool is_usb_speed_ss(usb_speed speed)
{
    return (speed == SUPER_SPEED || speed == SUPER_SPEED_PLUS);
}

/*
 * returns the address of the closest USB2.0 hub, which is responsible for
 * split transactions, along with the number of the used downstream port
 */
int closest_usb2_hub(const usbdev_t *dev, int *const addr, int *const port)
{
    const usbdev_t *usb1dev;

    do {
        usb1dev = dev;
        if ((dev->hub >= 0) && (dev->hub < 128))
            dev = dev->controller->devices[dev->hub];
        else
            dev = NULL;
    } while (dev && (dev->speed < 2));

    if (dev) {
        *addr = usb1dev->hub;
        *port = usb1dev->port;
        return 0;
    }

    usb_debug("Couldn't find closest USB2.0 hub.\n");
    return 1;
}
