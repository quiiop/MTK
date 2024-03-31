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


#ifndef __USBUVC_H
#define __USBUVC_H

#include "usbvideo.h"
#include "cli.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#define DBG_UVC 1
#if DBG_UVC
#define uvc_debug(x...) printf("[USB] " x)
#else /* #if DBG_UVC */
#define uvc_debug(x...) do {} while (0)
#endif /* #if DBG_UVC */

#define MAX_FRAME_SIZE 614400
/* Number of isochronous URBs. */
#define UVC_URBS 16
/* Maximum number of packets per URB. */
#define UVC_MAX_PACKETS 16

#define MAX_MJPEG_FRAME 10
#define LE16(addr) (((u16)(*((u8 *)(addr))))\
    + (((u16)(*(((u8 *)(addr)) + 1))) << 8))

struct uvc_interface {
    interface_descriptor_t intf_desc;
    endpoint_descriptor_t ep_desc[MAX_ENDPOINT];
};

struct uvc_config {
    configuration_descriptor_t config_desc;
    interface_assoc_descriptor_t uvc_iad;
    struct uvc_interface intf[MAX_INTERFACES];
};

/*
 * VideoControl I/F
 * Interface Descriptor--VIDEOCONTROL subclass
 * VideoControl header
 * input terminal(may no exit on this project)
 * camera terminal
 * processing unit
 * encoding unit
 * output terminal
 * 0~n extension unit
 */

/* Video Control Descriptor */
#define INPUT_TERMINAL 10
#define OUTPUT_TERMINAL 4
#define PROCESS_UNIT 2

struct uvc_control_desc {
    //interface_descriptor_t uvc_control_intf;
    struct uvc_header_descriptor *uvc_control_header;
    u8 header_interface_n;//number of baInterfaceNr[]
    //struct uvc_input_terminal_descriptor  uvc_input_terminal;
    struct uvc_camera_terminal_descriptor *uvc_camera_terminal[INPUT_TERMINAL];
    struct uvc_processing_unit_descriptor *uvc_processing_unit[PROCESS_UNIT];
    //struct uvc_extension_unit_descriptor uvc_extension_unit;
    //#define UVC_DT_EXTENSION_UNIT_SIZE(p, n) (24+(p)+(n))
    struct uvc_output_terminal_descriptor *uvc_output_terminal[OUTPUT_TERMINAL];
};

/*
* uvc_streaming_interface
* standard VS interface descriptor alt0
* Class-Specific VS interface descriptors {
*    input header descriptor
*    playload format descriptor
*    video frame descriptor
*    video frame descriptor
*    uvc_color_matching_descriptor(no need)
*    playload format descriptor
*    video frame descriptor
*    video frame descriptor
*    uvc_color_matching_descriptor(no need)
*    }
* standard VS interface descriptor alt1
*
*/

/* Video Streaming Descriptor */
#define INPUT_HEADER 3
#define MJPEG_FORMAT 3
#define MJPEG_FRAME 10

struct uvc_streaming_desc {
    //#define UVC_DT_INPUT_HEADER_SIZE(n, p) (13+(n*p))
    //u8  bmaControls[p][n]
    struct uvc_input_header_descriptor *uvc_input_header[INPUT_HEADER];
    struct uvc_format_mjpeg *mjpeg_fromat[MJPEG_FORMAT];
    struct uvc_frame_mjpeg *mjpeg_frame[MJPEG_FRAME];
};

/*
 * IAD I/F
 * VideoControl I/F
 * Interrupt Endpoint(no need)
 * VideoStreaming I/F
 * Alt.Seting 1~n
 */
struct uvc_device {
    usbdev_t *dev;
    //struct video_probe *probe_params;
    u8 *probe_params;
    struct uvc_config config;
    struct uvc_control_desc control_desc;
    u8 input_terminal_num;
    u8 process_unit_num;
    u8 output_terminal_num;

    struct uvc_streaming_desc stream_desc;
    u8 input_header_num;
    u8 mjpeg_format_num;
    u8 mjpeg_frame_num;

    endpoint_t isoc_in;

    u8 interface;
    u8 altsetting;

    struct uvc_streaming *stream;
    u8 bFrameIndex;
};

#define UVC_INST(dev) ((struct uvc_device *)(dev)->data)

struct uvc_streaming {
    //struct usb_interface *intf;
    struct uvc_device *udev;
    int intfnum;
    u16 maxpsize;
    u8 *frame_buf;
    u32 frame_len;
    u32 eof;
    void (*callback)(u8 *data, u32 buf_len);

    unsigned int error; /*error of frame buf*/
    u8 last_fid;
    endpoint_t *ep;

    struct urb *urb[UVC_URBS];
    char *urb_buffer[UVC_URBS];
    unsigned int urb_size;
    int enable;
    int capture_count;
    u8 urb_num;
    u8 isoc_packet;
    //struct uvc_streaming_control ctrl;
};

extern struct uvc_streaming *uvc_stream_new(struct uvc_device *udev);
extern int uvc_video_enable(struct uvc_streaming *stream, int enable);
extern void uvc_register_user_callback(struct uvc_device *udev,
                                       void (*callback)(u8 *data, u32 buf_len),
                                       u8 *data);
extern int uvc_submit_urb(struct uvc_streaming *stream);
extern void uvc_capture(struct uvc_device *udev, int enable, int width, int height, int count);
uint8_t t_uvc_capture(uint8_t len, char *param[]);
#endif /* #ifndef __USBUVC_H */
