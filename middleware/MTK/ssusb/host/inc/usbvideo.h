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


#ifndef __USBVIDEO_H
#define __USBVIDEO_H

/* --------------------------------------------------------------------------
 * UVC constants
 */

/* A.1. Video Interface Class Codes */
#define UVC_CC_VIDEO 0x0E

/* A.2. Video Interface Subclass Codes */
#define UVC_SC_UNDEFINED 0x00
#define UVC_SC_VIDEOCONTROL 0x01
#define UVC_SC_VIDEOSTREAMING 0x02
#define UVC_SC_VIDEO_INTERFACE_COLLECTION 0x03

/* A.3. Video Interface Protocol Codes */
#define UVC_PC_PROTOCOL_UNDEFINED 0x00
#define UVC_PC_PROTOCOL_15 0x01

/* A.4. Video Class-Specific Descriptor Types Codes */
#define UVC_CS_INTERFACE 0x24
#define UVC_CS_ENDPOINT 0x25

/* A.5. Video Class-Specific VC Interface Descriptor Subtypes */
#define UVC_VC_DESCRIPTOR_UNDEFINED 0x00
#define UVC_VC_HEADER 0x01
#define UVC_VC_INPUT_TERMINAL 0x02
#define UVC_VC_OUTPUT_TERMINAL 0x03
#define UVC_VC_SELECTOR_UNIT 0x04
#define UVC_VC_PROCESSING_UNIT 0x05
#define UVC_VC_EXTENSION_UNIT 0x06

/* A.6. Video Class-Specific VS Interface Descriptor Subtypes */
#define UVC_VS_UNDEFINED 0x00
#define UVC_VS_INPUT_HEADER 0x01
#define UVC_VS_OUTPUT_HEADER 0x02
#define UVC_VS_STILL_IMAGE_FRAME 0x03
#define UVC_VS_FORMAT_UNCOMPRESSED 0x04
#define UVC_VS_FRAME_UNCOMPRESSED 0x05
#define UVC_VS_FORMAT_MJPEG 0x06
#define UVC_VS_FRAME_MJPEG 0x07
#define UVC_VS_FORMAT_MPEG2TS 0x0a
#define UVC_VS_FORMAT_DV 0x0c
#define UVC_VS_COLORFORMAT 0x0d
#define UVC_VS_FORMAT_FRAME_BASED 0x10
#define UVC_VS_FRAME_FRAME_BASED 0x11
#define UVC_VS_FORMAT_STREAM_BASED 0x12

/* A.7. Video Class-Specific Endpoint Descriptor Subtypes */
#define UVC_EP_UNDEFINED 0x00
#define UVC_EP_GENERAL 0x01
#define UVC_EP_ENDPOINT 0x02
#define UVC_EP_INTERRUPT 0x03

/* A.8. Video Class-Specific Request Codes */
#define UVC_RC_UNDEFINED 0x00
#define UVC_SET_CUR 0x01
#define UVC_GET_CUR 0x81
#define UVC_GET_MIN 0x82
#define UVC_GET_MAX 0x83
#define UVC_GET_RES 0x84
#define UVC_GET_LEN 0x85
#define UVC_GET_INFO 0x86
#define UVC_GET_DEF 0x87

/* A.9.1. VideoControl Interface Control Selectors */
#define UVC_VC_CONTROL_UNDEFINED 0x00
#define UVC_VC_VIDEO_POWER_MODE_CONTROL 0x01
#define UVC_VC_REQUEST_ERROR_CODE_CONTROL 0x02

/* A.9.2. Terminal Control Selectors */
#define UVC_TE_CONTROL_UNDEFINED 0x00

/* A.9.3. Selector Unit Control Selectors */
#define UVC_SU_CONTROL_UNDEFINED 0x00
#define UVC_SU_INPUT_SELECT_CONTROL 0x01

/* A.9.4. Camera Terminal Control Selectors */
#define UVC_CT_CONTROL_UNDEFINED 0x00
#define UVC_CT_SCANNING_MODE_CONTROL 0x01
#define UVC_CT_AE_MODE_CONTROL 0x02
#define UVC_CT_AE_PRIORITY_CONTROL 0x03
#define UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04
#define UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL 0x05
#define UVC_CT_FOCUS_ABSOLUTE_CONTROL 0x06
#define UVC_CT_FOCUS_RELATIVE_CONTROL 0x07
#define UVC_CT_FOCUS_AUTO_CONTROL 0x08
#define UVC_CT_IRIS_ABSOLUTE_CONTROL 0x09
#define UVC_CT_IRIS_RELATIVE_CONTROL 0x0a
#define UVC_CT_ZOOM_ABSOLUTE_CONTROL 0x0b
#define UVC_CT_ZOOM_RELATIVE_CONTROL 0x0c
#define UVC_CT_PANTILT_ABSOLUTE_CONTROL 0x0d
#define UVC_CT_PANTILT_RELATIVE_CONTROL 0x0e
#define UVC_CT_ROLL_ABSOLUTE_CONTROL 0x0f
#define UVC_CT_ROLL_RELATIVE_CONTROL 0x10
#define UVC_CT_PRIVACY_CONTROL 0x11

/* A.9.5. Processing Unit Control Selectors */
#define UVC_PU_CONTROL_UNDEFINED 0x00
#define UVC_PU_BACKLIGHT_COMPENSATION_CONTROL 0x01
#define UVC_PU_BRIGHTNESS_CONTROL 0x02
#define UVC_PU_CONTRAST_CONTROL 0x03
#define UVC_PU_GAIN_CONTROL 0x04
#define UVC_PU_POWER_LINE_FREQUENCY_CONTROL 0x05
#define UVC_PU_HUE_CONTROL 0x06
#define UVC_PU_SATURATION_CONTROL 0x07
#define UVC_PU_SHARPNESS_CONTROL 0x08
#define UVC_PU_GAMMA_CONTROL 0x09
#define UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL 0x0a
#define UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0b
#define UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL 0x0c
#define UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0d
#define UVC_PU_DIGITAL_MULTIPLIER_CONTROL 0x0e
#define UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL 0x0f
#define UVC_PU_HUE_AUTO_CONTROL 0x10
#define UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL 0x11
#define UVC_PU_ANALOG_LOCK_STATUS_CONTROL 0x12

/* A.9.7. VideoStreaming Interface Control Selectors */
#define UVC_VS_CONTROL_UNDEFINED 0x00
#define UVC_VS_PROBE_CONTROL 0x01
#define UVC_VS_COMMIT_CONTROL 0x02
#define UVC_VS_STILL_PROBE_CONTROL 0x03
#define UVC_VS_STILL_COMMIT_CONTROL 0x04
#define UVC_VS_STILL_IMAGE_TRIGGER_CONTROL 0x05
#define UVC_VS_STREAM_ERROR_CODE_CONTROL 0x06
#define UVC_VS_GENERATE_KEY_FRAME_CONTROL 0x07
#define UVC_VS_UPDATE_FRAME_SEGMENT_CONTROL 0x08
#define UVC_VS_SYNC_DELAY_CONTROL 0x09

/* B.1. USB Terminal Types */
#define UVC_TT_VENDOR_SPECIFIC 0x0100
#define UVC_TT_STREAMING 0x0101

/* B.2. Input Terminal Types */
#define UVC_ITT_VENDOR_SPECIFIC 0x0200
#define UVC_ITT_CAMERA 0x0201
#define UVC_ITT_MEDIA_TRANSPORT_INPUT 0x0202

/* B.3. Output Terminal Types */
#define UVC_OTT_VENDOR_SPECIFIC 0x0300
#define UVC_OTT_DISPLAY 0x0301
#define UVC_OTT_MEDIA_TRANSPORT_OUTPUT 0x0302

/* B.4. External Terminal Types */
#define UVC_EXTERNAL_VENDOR_SPECIFIC 0x0400
#define UVC_COMPOSITE_CONNECTOR 0x0401
#define UVC_SVIDEO_CONNECTOR 0x0402
#define UVC_COMPONENT_CONNECTOR 0x0403

/* 2.4.2.2. Status Packet Type */
#define UVC_STATUS_TYPE_CONTROL 1
#define UVC_STATUS_TYPE_STREAMING 2

/* 2.4.3.3. Payload Header Information */
#define UVC_STREAM_EOH (1 << 7)
#define UVC_STREAM_ERR (1 << 6)
#define UVC_STREAM_STI (1 << 5)
#define UVC_STREAM_RES (1 << 4)
#define UVC_STREAM_SCR (1 << 3)
#define UVC_STREAM_PTS (1 << 2)
#define UVC_STREAM_EOF (1 << 1)
#define UVC_STREAM_FID (1 << 0)

/* 4.1.2. Control Capabilities */
#define UVC_CONTROL_CAP_GET (1 << 0)
#define UVC_CONTROL_CAP_SET (1 << 1)
#define UVC_CONTROL_CAP_DISABLED (1 << 2)
#define UVC_CONTROL_CAP_AUTOUPDATE (1 << 3)
#define UVC_CONTROL_CAP_ASYNCHRONOUS (1 << 4)

/* ------------------------------------------------------------------------ */

#define INTERFACE_NBR 5

/* 3.7.2. Video Control Interface Header Descriptor */
struct uvc_header_descriptor {
    u8   bLength;
    u8   bDescriptorType;
    u8   bDescriptorSubType;
    u16 bcdUVC;
    u16 wTotalLength;
    u32 dwClockFrequency;
    u8   bInCollection;
    u8   baInterfaceNr[INTERFACE_NBR];
} __attribute__((__packed__));

/* 3.7.2.1. Input Terminal Descriptor */
struct uvc_input_terminal_descriptor {
    u8   bLength;
    u8   bDescriptorType;
    u8   bDescriptorSubType;
    u8   bTerminalID;
    u16 wTerminalType;
    u8   bAssocTerminal;
    u8   iTerminal;
} __attribute__((__packed__));

/* 3.7.2.2. Output Terminal Descriptor */
struct uvc_output_terminal_descriptor {
    u8   bLength;
    u8   bDescriptorType;
    u8   bDescriptorSubType;
    u8   bTerminalID;
    u16 wTerminalType;
    u8   bAssocTerminal;
    u8   bSourceID;
    u8   iTerminal;
} __attribute__((__packed__));

/* 3.7.2.3. Camera Terminal Descriptor */
struct uvc_camera_terminal_descriptor {
    u8   bLength;
    u8   bDescriptorType;
    u8   bDescriptorSubType;
    u8   bTerminalID;
    u16 wTerminalType;
    u8   bAssocTerminal;
    u8   iTerminal;
    u16 wObjectiveFocalLengthMin;
    u16 wObjectiveFocalLengthMax;
    u16 wOcularFocalLength;
    u8   bControlSize;
    u8   bmControls[3];
} __attribute__((__packed__));

/* 3.7.2.4. Selector Unit Descriptor */
struct uvc_selector_unit_descriptor {
    u8  bLength;
    u8  bDescriptorType;
    u8  bDescriptorSubType;
    u8  bUnitID;
    u8  bNrInPins;
    u8  baSourceID[0];
    u8  iSelector;
} __attribute__((__packed__));

/* 3.7.2.5. Processing Unit Descriptor */
struct uvc_processing_unit_descriptor {
    u8   bLength;
    u8   bDescriptorType;
    u8   bDescriptorSubType;
    u8   bUnitID;
    u8   bSourceID;
    u16 wMaxMultiplier;
    u8   bControlSize;
    u8   bmControls[2];
    u8   iProcessing;
} __attribute__((__packed__));

/* 3.7.2.6. Extension Unit Descriptor */
struct uvc_extension_unit_descriptor {
    u8  bLength;
    u8  bDescriptorType;
    u8  bDescriptorSubType;
    u8  bUnitID;
    u8  guidExtensionCode[16];
    u8  bNumControls;
    u8  bNrInPins;
    u8  baSourceID[0];
    u8  bControlSize;
    u8  bmControls[0];
    u8  iExtension;
} __attribute__((__packed__));

/* 3.8.2.2. Video Control Interrupt Endpoint Descriptor */
struct uvc_control_endpoint_descriptor {
    u8   bLength;
    u8   bDescriptorType;
    u8   bDescriptorSubType;
    u16 wMaxTransferSize;
} __attribute__((__packed__));

#define NUM_CONTROLS 3

/* 3.9.2.1. Input Header Descriptor */
struct uvc_input_header_descriptor {
    u8   bLength;
    u8   bDescriptorType;
    u8   bDescriptorSubType;
    u8   bNumFormats;
    u16 wTotalLength;
    u8   bEndpointAddress;
    u8   bmInfo;
    u8   bTerminalLink;
    u8   bStillCaptureMethod;
    u8   bTriggerSupport;
    u8   bTriggerUsage;
    u8   bControlSize;
    u8   bmaControls[NUM_CONTROLS];
} __attribute__((__packed__));

/* 3.9.2.2. Output Header Descriptor */
struct uvc_output_header_descriptor {
    u8   bLength;
    u8   bDescriptorType;
    u8   bDescriptorSubType;
    u8   bNumFormats;
    u16 wTotalLength;
    u8   bEndpointAddress;
    u8   bTerminalLink;
    u8   bControlSize;
    u8   bmaControls[];
} __attribute__((__packed__));

/* 3.9.2.6. Color matching descriptor */
struct uvc_color_matching_descriptor {
    u8  bLength;
    u8  bDescriptorType;
    u8  bDescriptorSubType;
    u8  bColorPrimaries;
    u8  bTransferCharacteristics;
    u8  bMatrixCoefficients;
} __attribute__((__packed__));

/* 4.3.1.1. Video Probe and Commit Controls */
struct uvc_streaming_control {
    u16 bmHint;
    u8  bFormatIndex;
    u8  bFrameIndex;
    u32 dwFrameInterval;
    u16 wKeyFrameRate;
    u16 wPFrameRate;
    u16 wCompQuality;
    u16 wCompWindowSize;
    u16 wDelay;
    u32 dwMaxVideoFrameSize;
    u32 dwMaxPayloadTransferSize;
    u32 dwClockFrequency;
    u8  bmFramingInfo;
    u8  bPreferedVersion;
    u8  bMinVersion;
    u8  bMaxVersion;
} __attribute__((__packed__));

/* MJPEG Payload - 3.1.1. MJPEG Video Format Descriptor */
struct uvc_format_mjpeg {
    u8  bLength;
    u8  bDescriptorType;
    u8  bDescriptorSubType;
    u8  bFormatIndex;
    u8  bNumFrameDescriptors;
    u8  bmFlags;
    u8  bDefaultFrameIndex;
    u8  bAspectRatioX;
    u8  bAspectRatioY;
    u8  bmInterfaceFlags;
    u8  bCopyProtect;
} __attribute__((__packed__));

/* MJPEG Payload - 3.1.2. MJPEG Video Frame Descriptor */
struct uvc_frame_mjpeg {
    u8   bLength;
    u8   bDescriptorType;
    u8   bDescriptorSubType;
    u8   bFrameIndex;
    u8   bmCapabilities;
    u16 wWidth;
    u16 wHeight;
    u32 dwMinBitRate;
    u32 dwMaxBitRate;
    u32 dwMaxVideoFrameBufferSize;
    u32 dwDefaultFrameInterval;
    u8   bFrameIntervalType;
    u32 dwFrameInterval;
} __attribute__((__packed__));

#define PROBE_PARAMS 26
/* Video Probe and Commit Controls */
struct video_probe {
    u16    bmHint;
    u8     bFormatIndex;
    u8     bFrameIndex;
    u8     dwFrameInterval;
    u16    wKeyFrameRate;
    u16    wPFrameRate;
    u16    wCompQuality;
    u16    wCompWindowSize;
    u16    wDelay;
    u32    dwMaxVideoFrameSize;
    u32    dwMaxPayloadTransferSize;
    u32    dwClockFrequency;
    u8     bmFramingInfo;
    u8     bPreferedVersion;
    u8     bMinVersion;
    u8     bMaxVersion;
} __attribute__((__packed__));

#endif /* #ifndef __USBVIDEO_H */
