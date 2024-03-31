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
 * Author:
 *  Joson Chen<joson.chen@mediatek.com>
 *  Min Guo <min.guo@mediatek.com>
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
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

#include <usb.h>
#include <usbuvc.h>
#include <usbvideo.h>

struct uvc_device *global_udev;

#if 0
static void print_probe_params(struct video_probe *probe_params)
{
    if (!probe_params)
        return;

    uvc_debug("****** Probe Params ****** \n");
    uvc_debug("bFormatIndex:%x \n", probe_params->bFormatIndex);
    uvc_debug("bFrameIndex:%x \n", probe_params->bFrameIndex);
    uvc_debug("dwFrameInterval:%x \n", probe_params->dwFrameInterval);
    uvc_debug("dwMaxVideoFrameSize:%x \n", probe_params->dwMaxVideoFrameSize);
    uvc_debug("dwMaxPayloadTransferSize:%x \n", probe_params->dwMaxPayloadTransferSize);
}
#endif /* #if 0 */

static void print_probe_data(u8 *data)
{

    int i = 0;
    if (!data)
        return;
    uvc_debug("\n******  print_probe_data begin****** \n");
    do {
        uvc_debug("0x%x ", data[i]);
        i++;
    } while (i < 26);
    uvc_debug("\n******  print_probe_data end****** \n");
}

static void print_vc_header_descs(struct uvc_header_descriptor *uvc_control_header)
{
    if (!uvc_control_header)
        return;

    uvc_debug("****** Control Interface Header Descriptor ****** \n");
    uvc_debug("bLength:%x \n", uvc_control_header->bLength);
    uvc_debug("bDescriptorType:%x \n", uvc_control_header->bDescriptorType);
    uvc_debug("bDescriptorSubType:%x \n", uvc_control_header->bDescriptorSubType);
    uvc_debug("bcdUVC:%x \n", uvc_control_header->bcdUVC);
    uvc_debug("wTotalLength:%x \n", uvc_control_header->wTotalLength);
    uvc_debug("dwClockFrequency:%x \n", uvc_control_header->dwClockFrequency);
    uvc_debug("bInCollection:%x \n", uvc_control_header->bInCollection);
    uvc_debug("baInterfaceNr:%x \n", uvc_control_header->baInterfaceNr[0]);
}

static void print_vc_input_terminal_descs(struct uvc_camera_terminal_descriptor *uvc_camera_terminal)
{
    if (!uvc_camera_terminal)
        return;

    uvc_debug("****** Camera Terminal Descriptor ****** \n");
    uvc_debug("bLength:%x \n", uvc_camera_terminal->bLength);
    uvc_debug("bDescriptorType:%x \n", uvc_camera_terminal->bDescriptorType);
    uvc_debug("bDescriptorSubType:%x \n", uvc_camera_terminal->bDescriptorSubType);
    uvc_debug("bTerminalID:%x \n", uvc_camera_terminal->bTerminalID);
    uvc_debug("wTerminalType:%x \n", uvc_camera_terminal->wTerminalType);
    uvc_debug("bAssocTerminal:%x \n", uvc_camera_terminal->bAssocTerminal);
    uvc_debug("iTerminal:%x \n", uvc_camera_terminal->iTerminal);
    uvc_debug("wObjectiveFocalLengthMin:%x \n", uvc_camera_terminal->wObjectiveFocalLengthMin);
    uvc_debug("wObjectiveFocalLengthMax:%x \n", uvc_camera_terminal->wObjectiveFocalLengthMax);
    uvc_debug("wOcularFocalLength:%x \n", uvc_camera_terminal->wOcularFocalLength);
    uvc_debug("bControlSize:%x \n", uvc_camera_terminal->bControlSize);
    uvc_debug("bmControls0:%x \n", uvc_camera_terminal->bmControls[0]);
    uvc_debug("bmControls1:%x \n", uvc_camera_terminal->bmControls[1]);
    uvc_debug("bmControls2:%x \n", uvc_camera_terminal->bmControls[2]);
}

static void print_vc_processing_unit_descs(struct uvc_processing_unit_descriptor *uvc_processing_unit)
{
    if (!uvc_processing_unit)
        return;

    uvc_debug("****** Processing Unit Descriptor ****** \n");
    uvc_debug("bLength:%x \n", uvc_processing_unit->bLength);
    uvc_debug("bDescriptorType:%x \n", uvc_processing_unit->bDescriptorType);
    uvc_debug("bDescriptorSubType:%x \n", uvc_processing_unit->bDescriptorSubType);
    uvc_debug("bUnitID:%x \n", uvc_processing_unit->bUnitID);
    uvc_debug("bSourceID:%x \n", uvc_processing_unit->bSourceID);
    uvc_debug("wMaxMultiplier:%x \n", uvc_processing_unit->wMaxMultiplier);
    uvc_debug("bControlSize:%x \n", uvc_processing_unit->bControlSize);
    uvc_debug("bmControls0:%x \n", uvc_processing_unit->bmControls[0]);
    uvc_debug("bmControls1:%x \n", uvc_processing_unit->bmControls[1]);
    uvc_debug("iProcessing:%x \n", uvc_processing_unit->iProcessing);
}

static void print_vc_output_terminal_descs(struct uvc_output_terminal_descriptor *uvc_output_terminal)
{
    if (!uvc_output_terminal)
        return;

    uvc_debug("****** Output Terminal Descriptor ****** \n");
    uvc_debug("bLength:%x \n", uvc_output_terminal->bLength);
    uvc_debug("bDescriptorType:%x \n", uvc_output_terminal->bDescriptorType);
    uvc_debug("bDescriptorSubType:%x \n", uvc_output_terminal->bDescriptorSubType);
    uvc_debug("bTerminalID:%x \n", uvc_output_terminal->bTerminalID);
    uvc_debug("wTerminalType:%x \n", uvc_output_terminal->wTerminalType);
    uvc_debug("bAssocTerminal:%x \n", uvc_output_terminal->bAssocTerminal);
    uvc_debug("bSourceID:%x \n", uvc_output_terminal->bSourceID);
    uvc_debug("iTerminal:%x \n", uvc_output_terminal->iTerminal);
}

static void print_vs_input_header_descs(struct uvc_input_header_descriptor *uvc_input_header)
{
    if (!uvc_input_header)
        return;

    uvc_debug("****** Streaming Input Header Descriptor ****** \n");
    uvc_debug("bLength:%x \n", uvc_input_header->bLength);
    uvc_debug("bDescriptorType:%x \n", uvc_input_header->bDescriptorType);
    uvc_debug("bDescriptorSubType:%x \n", uvc_input_header->bDescriptorSubType);
    uvc_debug("bNumFormats:%x \n", uvc_input_header->bNumFormats);
    uvc_debug("wTotalLength:%x \n", uvc_input_header->wTotalLength);
    uvc_debug("bEndpointAddress:%x \n", uvc_input_header->bEndpointAddress);
    uvc_debug("bmInfo:%x \n", uvc_input_header->bmInfo);
    uvc_debug("bTerminalLink:%x \n", uvc_input_header->bTerminalLink);
    uvc_debug("bStillCaptureMethod:%x \n", uvc_input_header->bStillCaptureMethod);
    uvc_debug("bTriggerSupport:%x \n", uvc_input_header->bTriggerSupport);
    uvc_debug("bTriggerUsage:%x \n", uvc_input_header->bTriggerUsage);
    uvc_debug("bControlSize:%x \n", uvc_input_header->bControlSize);
    uvc_debug("bmaControls0:%x \n", uvc_input_header->bmaControls[0]);
}

static void print_vs_format_mjpeg_descs(struct uvc_format_mjpeg *mjpeg_fromat)
{
    if (!mjpeg_fromat)
        return;

    uvc_debug("****** MJPEG Video Format Descriptor ****** \n");
    uvc_debug("bLength:%x \n", mjpeg_fromat->bLength);
    uvc_debug("bDescriptorType:%x \n", mjpeg_fromat->bDescriptorType);
    uvc_debug("bDescriptorSubType:%x \n", mjpeg_fromat->bDescriptorSubType);
    uvc_debug("bFormatIndex:%x \n", mjpeg_fromat->bFormatIndex);
    uvc_debug("bNumFrameDescriptors:%x \n", mjpeg_fromat->bNumFrameDescriptors);
    uvc_debug("bmFlags:%x \n", mjpeg_fromat->bmFlags);
    uvc_debug("bDefaultFrameIndex:%x \n", mjpeg_fromat->bDefaultFrameIndex);
    uvc_debug("bAspectRatioX:%x \n", mjpeg_fromat->bAspectRatioX);
    uvc_debug("bAspectRatioY:%x \n", mjpeg_fromat->bAspectRatioY);
    uvc_debug("bmInterfaceFlags:%x \n", mjpeg_fromat->bmInterfaceFlags);
    uvc_debug("bCopyProtect:%x \n", mjpeg_fromat->bCopyProtect);
}

static void print_vs_frame_mjpeg_descs(struct uvc_frame_mjpeg *mjpeg_frame)
{
    if (!mjpeg_frame)
        return;

    uvc_debug("****** MJPEG Video Frame Descriptor ****** \n");
    uvc_debug("bLength:%x \n", mjpeg_frame->bLength);
    uvc_debug("bDescriptorType:%x \n", mjpeg_frame->bDescriptorType);
    uvc_debug("bDescriptorSubType:%x \n", mjpeg_frame->bDescriptorSubType);
    uvc_debug("bFrameIndex:%x \n", mjpeg_frame->bFrameIndex);
    uvc_debug("bmCapabilities:%x \n", mjpeg_frame->bmCapabilities);
    uvc_debug("wWidth:%x \n", mjpeg_frame->wWidth);
    uvc_debug("wHeight:%x \n", mjpeg_frame->wHeight);
    uvc_debug("dwMinBitRate:%x \n", mjpeg_frame->dwMinBitRate);
    uvc_debug("dwMaxBitRate:%x \n", mjpeg_frame->dwMaxBitRate);
    uvc_debug("dwMaxVideoFrameBufferSize:%x \n", mjpeg_frame->dwMaxVideoFrameBufferSize);
    uvc_debug("dwDefaultFrameInterval:%x \n", mjpeg_frame->dwDefaultFrameInterval);
    uvc_debug("bFrameIntervalType:%x \n", mjpeg_frame->bFrameIntervalType);
    uvc_debug("dwFrameInterval:%x \n", mjpeg_frame->dwFrameInterval);
}

static void assign_control_descriptors(struct uvc_device *udev, char *buf)
{
    struct uvc_control_desc *control_desc = &udev->control_desc;

    switch (buf[2]) {
        /* video control subclass */
        case UVC_VC_HEADER:
            control_desc->uvc_control_header = (struct uvc_header_descriptor *)buf;
            print_vc_header_descs(control_desc->uvc_control_header);
            break;
        case UVC_VC_INPUT_TERMINAL:
            if (udev->input_terminal_num < INPUT_TERMINAL) {
                control_desc->uvc_camera_terminal[udev->input_terminal_num] = (struct uvc_camera_terminal_descriptor *)buf;
                print_vc_input_terminal_descs(control_desc->uvc_camera_terminal[udev->input_terminal_num]);
                udev->input_terminal_num++;
            }
            break;
        case UVC_VC_PROCESSING_UNIT:
            if (udev->process_unit_num < PROCESS_UNIT) {
                control_desc->uvc_processing_unit[udev->process_unit_num] = (struct uvc_processing_unit_descriptor *)buf;
                print_vc_processing_unit_descs(control_desc->uvc_processing_unit[udev->process_unit_num]);
                udev->process_unit_num++;
            }
            break;
        case UVC_VC_OUTPUT_TERMINAL:
            if (udev->output_terminal_num < OUTPUT_TERMINAL) {
                control_desc->uvc_output_terminal[udev->output_terminal_num] = (struct uvc_output_terminal_descriptor *)buf;
                print_vc_output_terminal_descs(control_desc->uvc_output_terminal[udev->output_terminal_num]);
                udev->output_terminal_num++;
            }
            break;
        default:
            break;
    };
}

static void assign_stream_descriptors(struct uvc_device *udev, char *buf)
{
    struct uvc_streaming_desc *stream_desc = &udev->stream_desc;

    switch (buf[2]) {
        /* video streamimg subclass */
        case UVC_VS_INPUT_HEADER:
            if (udev->input_header_num < INPUT_HEADER) {
                stream_desc->uvc_input_header[udev->input_header_num] = (struct uvc_input_header_descriptor *)buf;
                print_vs_input_header_descs(stream_desc->uvc_input_header[udev->input_header_num]);
                udev->input_header_num++;
            }
            break;
        case UVC_VS_FORMAT_MJPEG:
            if (udev->mjpeg_format_num < MJPEG_FORMAT) {
                stream_desc->mjpeg_fromat[udev->mjpeg_format_num] = (struct uvc_format_mjpeg *)buf;
                print_vs_format_mjpeg_descs(stream_desc->mjpeg_fromat[udev->mjpeg_format_num]);

                udev->mjpeg_format_num++;
            }
            break;
        case UVC_VS_FRAME_MJPEG:
            if (udev->mjpeg_frame_num < MJPEG_FRAME) {
                stream_desc->mjpeg_frame[udev->mjpeg_frame_num] = (struct uvc_frame_mjpeg *)buf;
                print_vs_frame_mjpeg_descs(stream_desc->mjpeg_frame[udev->mjpeg_frame_num]);

                udev->mjpeg_frame_num++;
            }
            break;
        default:
            break;
    };
}

static int find_interface_index(struct uvc_device *udev, int itf_num, int alt_setting)
{
    int i;

    for (i = 0; i < MAX_INTERFACES; i++) {
        if ((udev->dev->config.intf[i].intf_desc.bInterfaceNumber == itf_num) &&
            (udev->dev->config.intf[i].intf_desc.bAlternateSetting == alt_setting))
            return i;
    }

    return -1;
}

static void uvc_parse_csdescriptors(usbdev_t *dev)
{
    struct uvc_device *udev;
    usb_descheader_t *desc;
    char *buf;
    u32 config_len;
    u32 itf_num, alt_setting;
    int itf_index;
    u32 sub_class = 0;
    u32 length = 0;

    udev = (struct uvc_device *)(dev->data);
    desc = (usb_descheader_t *)dev->configuration;
    config_len = dev->configuration->wTotalLength;

    while (length < config_len) {
        desc = usb_get_nextdesc((u8 *)desc, &length);
        buf = (char *)desc;

        switch (desc->bDescriptorType) {
            case DT_INTF:
                itf_num = *(buf + 2);
                alt_setting = *(buf + 3);
                itf_index = find_interface_index(udev, itf_num, alt_setting);
                if (itf_index >= 0)
                    sub_class = udev->dev->config.intf[itf_index].intf_desc.bInterfaceSubClass;
                usb_debug("itf_num:%d alt_setting:%d itf_index:%d sub_class:%d\n", itf_num, alt_setting, itf_index, sub_class);
                break;
            case UVC_CS_INTERFACE:
                if (sub_class == UVC_SC_VIDEOCONTROL) {
                    assign_control_descriptors(udev, buf);
                } else if (sub_class == UVC_SC_VIDEOSTREAMING) {
                    assign_stream_descriptors(udev, buf);
                }
                break;
            default:
                break;
        }
    }
}

static int uvc_find_streamin(struct uvc_device *udev, u32 ep_maxp)
{
    int i;

    for (i = 0; i < MAX_INTERFACES; i++) {
        if ((udev->dev->config.intf[i].intf_desc.bInterfaceClass == UVC_CC_VIDEO) &&
            (udev->dev->config.intf[i].intf_desc.bInterfaceSubClass == UVC_SC_VIDEOSTREAMING)) {
            if ((udev->dev->config.intf[i].ep_desc[0].bEndpointAddress & 0x80) &&
                (udev->dev->config.intf[i].ep_desc[0].wMaxPacketSize > 0)) {
                if (udev->dev->config.intf[i].ep_desc[0].wMaxPacketSize == ep_maxp) {
                    udev->interface = udev->dev->config.intf[i].intf_desc.bInterfaceNumber;
                    udev->altsetting = udev->dev->config.intf[i].intf_desc.bAlternateSetting;

                    udev->isoc_in.dev = udev->dev;
                    udev->isoc_in.endpoint = udev->dev->config.intf[i].ep_desc[0].bEndpointAddress;
                    udev->isoc_in.maxpacketsize = udev->dev->config.intf[i].ep_desc[0].wMaxPacketSize;
                    udev->isoc_in.direction = (udev->dev->config.intf[i].ep_desc[0].bEndpointAddress & 0x80) ? IN : OUT;
                    udev->isoc_in.type = udev->dev->config.intf[i].ep_desc[0].bmAttributes & 0x3;
                    udev->isoc_in.interval = usb_decode_interval(udev->dev->speed, udev->isoc_in.type, udev->dev->config.intf[i].ep_desc[0].bInterval);
                    usb_crit("find stream interface[%d] altsetting[%d] EP:%d size:%d bytes. \n",
                             udev->interface, udev->altsetting, udev->isoc_in.endpoint, udev->isoc_in.maxpacketsize);
                    return 0;
                }
            }
        }
    }

    usb_crit("#Cannot Find Stream Interface\n");
    return -1;
}

#if 0
static void uvc_config_streamin_endpoint(struct uvc_device *udev)
{
    int ret;
    usbdev_t *dev;

    dev = udev->dev;
    ret = dev->controller->config_endpoint(&udev->isoc_in);
    if (ret)
        usb_crit("%s: fail \n", __func__);

    return;
}
#endif /* #if 0 */
#define UVC_SETCUR gen_bmRequestType(host_to_device, class_type, iface_recp)
#define UVC_GETCUR gen_bmRequestType(device_to_host, class_type, iface_recp)

static int uvc_setcur(struct uvc_device *udev)
{
    int ret;
    dev_req_t *dr;
    u8 *data;

    uvc_debug("%s \n", __func__);
    data = udev->probe_params;
    data[3] = udev->bFrameIndex;
    usb_err("%s: frame index:%d\n", __func__, data[3]);
    print_probe_data(data);

    dr = pvPortMallocNC(sizeof(dev_req_t));
    if (dr == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;;
    };

    dr->bmRequestType = UVC_SETCUR;
    dr->data_dir = host_to_device;
    dr->bRequest = UVC_SET_CUR;
    dr->wValue = UVC_VS_PROBE_CONTROL << 8;
    dr->wIndex = udev->interface;
    dr->wLength = PROBE_PARAMS;

    ret = usb_control_msg(udev->dev, OUT, dr, PROBE_PARAMS, data);
    vPortFreeNC((void *)dr);
    return ret;
}

static int uvc_commit_video(struct uvc_device *udev)
{
    int ret;
    dev_req_t *dr;

    uvc_debug("%s \n", __func__);
    /* probe_params = udev->probe_params;
     * assign paraments
     * probe_params. = ...
     * probe_params->bmHint = 1;
     */

    dr = pvPortMallocNC(sizeof(dev_req_t));
    if (dr == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;;
    };

    dr->bmRequestType = UVC_SETCUR;
    dr->data_dir = host_to_device;
    dr->bRequest = UVC_SET_CUR;
    dr->wValue = UVC_VS_COMMIT_CONTROL << 8;
    dr->wIndex = udev->interface;
    dr->wLength = PROBE_PARAMS;

    ret = usb_control_msg(udev->dev, OUT, dr, PROBE_PARAMS, udev->probe_params);
    vPortFreeNC((void *)dr);
    return ret;
}

static int uvc_getcur(struct uvc_device *udev)
{
    int ret;
    dev_req_t *dr;

    dr = pvPortMallocNC(sizeof(dev_req_t));
    if (dr == NULL) {
        usb_err("%s: Out of memory\n", __func__);
        return -1;;
    };

    dr->bmRequestType = UVC_GETCUR;
    dr->data_dir = device_to_host;
    dr->bRequest = UVC_GET_CUR;
    dr->wValue = UVC_VS_PROBE_CONTROL << 8;
    dr->wIndex = udev->interface;
    dr->wLength = PROBE_PARAMS;

    print_probe_data(udev->probe_params);
    ret = usb_control_msg(udev->dev, IN, dr, PROBE_PARAMS, udev->probe_params);

    vPortFreeNC((void *)dr);
    return ret;
}

static void usb_uvc_poll(usbdev_t *dev)
{
    return;
}

static void usb_uvc_destroy(usbdev_t *dev)
{
    int i;
    struct uvc_device *udev;
    struct uvc_control_desc *control_desc;
    struct uvc_streaming_desc *stream_desc;

    if (!dev->data)
        return;

    udev = (struct uvc_device *)(dev->data);
    control_desc = &udev->control_desc;
    stream_desc = &udev->stream_desc;

    /* video control subclass */
    control_desc->uvc_control_header = NULL;
    for (i = 0; i < INPUT_TERMINAL; i++)
        control_desc->uvc_camera_terminal[i] = NULL;

    for (i = 0; i < PROCESS_UNIT; i++)
        control_desc->uvc_processing_unit[i] = NULL;

    for (i = 0; i < OUTPUT_TERMINAL; i++)
        control_desc->uvc_output_terminal[i] = NULL;

    /* video streamimg subclass */
    for (i = 0; i < INPUT_HEADER; i++)
        stream_desc->uvc_input_header[i] = NULL;

    for (i = 0; i < MJPEG_FORMAT; i++)
        stream_desc->mjpeg_fromat[i] = NULL;

    for (i = 0; i < MJPEG_FRAME; i++)
        stream_desc->mjpeg_frame[i] = NULL;

    udev->dev = NULL;
    vPortFreeNC(udev->probe_params);
    vPortFree(dev->data);
    dev->data = NULL;
    global_udev = NULL;

    return;
}

void usb_uvc_init(usbdev_t *dev)
{
    u32 ep_maxp;
    struct uvc_device *udev;
    //struct uvc_streaming *stream;

    ep_maxp = 0x13fc; /* select ep size by parameter */
    dev->data = pvPortMalloc(sizeof(struct uvc_device));
    if (!dev->data) {
        usb_err("Not enough memory for USB uvc device.\n");
        return;
    }

    memset(dev->data, 0, sizeof(struct uvc_device));
    udev = dev->data;
    udev->dev = dev;
    dev->destroy = usb_uvc_destroy;
    dev->poll = usb_uvc_poll;

    udev->probe_params = pvPortMallocNC(PROBE_PARAMS);
    if (! udev->probe_params) {
        usb_err("Not enough memory for USB uvc probe_params.\n");
        return;
    }

    /*parse uvc control and streaming descriptor from dev->configuration*/
    uvc_parse_csdescriptors(dev);
    uvc_find_streamin(udev, ep_maxp);
    /*uvc_config_streamin_endpoint(udev);*/
    set_interface(dev, udev->interface, 0);
    udev->stream = uvc_stream_new(udev);
    if (!udev->stream) {
        usb_err("Not enough memory for USB uvc stream.\n");
        return;
    }

    uvc_debug("%s %d %d\n", __func__, __LINE__, udev->isoc_in.maxpacketsize);
    global_udev = dev->data;
    return;
}

/* use get_uvc_device replace global_udev */
/* usbdev_t *get_uvc_device(void)
 *{
 *}
 */

void print_frame_data(u8 *data, u32 buf_len)
{
    uvc_debug("uvc_video_complete UVC_STREAM_EOF %d 0x%x 0x%x 0x%x 0x%x\n", buf_len, data[0], data[1],
              data[buf_len - 2], data[buf_len - 1]);
}

void uvc_capture(struct uvc_device *udev, int enable, int width, int height, int count)
{
    if (!udev) {
        uvc_debug("%s %d udev is null\n", __func__, __LINE__);
        return;
    }

    if (!udev->stream) {
        uvc_debug("%s %d stream is null\n", __func__, __LINE__);
        return;
    }

    if (!enable) {
        uvc_debug("%s %d\n", __func__, __LINE__);
        set_interface(udev->dev, udev->interface, 0);
        uvc_video_enable(udev->stream, 0);
        return;
    }

    udev->stream->capture_count = count;

    if (!udev->stream->enable) {
        uvc_getcur(udev);
        /*todo set width and heigh*/
        uvc_setcur(udev);
        uvc_getcur(udev);
        uvc_commit_video(udev);
        uvc_debug("%s %d\n", __func__, __LINE__);
        set_interface(udev->dev, udev->interface, udev->altsetting);
        uvc_video_enable(udev->stream, 1);
    } else {
        uvc_debug("%s %d\n", __func__, __LINE__);
        uvc_submit_urb(udev->stream);
    }
}
/*
* enable:0/1
* //format:MJPEG/YUV
* resolution width:
* resolution height:
* //fps:
* count: frame count
*/

uint8_t t_uvc_capture(uint8_t len, char *param[])
{
    int enable;
    //int format;
    int width, height;
    int count;
    u8 *frame_buf;
    struct uvc_streaming *stream;

    enable = atoi(param[0]);
    /*1:MJPEG,0:YUV*/
    //format = atoi(param[1]);
    width = atoi(param[1]);
    height = atoi(param[2]);
    count = atoi(param[3]);
    stream = global_udev->stream;

    stream->urb_num = atoi(param[4]);
    stream->isoc_packet = atoi(param[5]);

    if (stream->urb_num > UVC_URBS)
        stream->urb_num = UVC_URBS;

    if (stream->isoc_packet > UVC_MAX_PACKETS)
        stream->isoc_packet = UVC_MAX_PACKETS;

    uvc_debug("%s %d %d %d %d %d urb_num:%d td_packet:%d \n", __func__, __LINE__, enable, width, height, count, stream->urb_num, stream->isoc_packet);

    if (480 == height)
        global_udev->bFrameIndex = 1;

    if (720 == height)
        global_udev->bFrameIndex = 3;

    if (1080 == height)
        global_udev->bFrameIndex = 2;

    frame_buf = pvPortMalloc(MAX_FRAME_SIZE);
    memset(frame_buf, 0, MAX_FRAME_SIZE);
    if (!frame_buf) {
        uvc_debug("%s frame_buf NULL\n", __func__);
    }
    uvc_debug("%s switch_index :%d \n", __func__, global_udev->bFrameIndex);
    uvc_register_user_callback(global_udev, print_frame_data, frame_buf);

    uvc_capture(global_udev, enable, width, height, count);

    return 0;
}

