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
 * MediaTek Inc. (C) 2022. All rights reserved.
 *
 * Author:
 *  Tianping Fang<tianping.fang@mediatek.com>
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


#ifndef __USBUAC_H
#define __USBUAC_H

#include "cli.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>

#ifdef MTK_FATFS_ENABLE
#include "ff.h"
#endif /* #ifdef MTK_FATFS_ENABLE */

#define uac_err(x...) printf("[UAC] " x)
#define uac_crit(x...) printf("[UAC] " x)

#define DBG_UAC 0
#if DBG_UAC
#define uac_debug(x...) printf("[UAC] " x)
#else /* #if DBG_UAC */
#define uac_debug(x...) do {} while (0)
#endif /* #if DBG_UAC */

#define FUNC_NUM    2
#define PERIOD_BYTES 3072
#define FRAMES_PER_PERIOD 768
#define PERIODS_PER_BUFFER 10
#define UAC_RING_BUFFER_LENGTH 20480

#define PLAYBACK 0
#define CAPTURE 1

/* A.9 Audio Class-Specific Request Codes */
#define UAC_SET_            0x00
#define UAC_GET_            0x80

#define UAC__CUR            0x1

#define UAC_SET_CUR         (UAC_SET_ | UAC__CUR)
#define UAC_GET_CUR         (UAC_GET_ | UAC__CUR)

#define UAC_EP_CS_ATTR_SAMPLE_RATE  0x01

#define USB_DIR_IN  0x80
#define UAC_CS_INTERFACE 0x24

#define MAX_URBS    12
#define MAX_QUEUE   18
#define MAX_PACKS   6       /* per URB */
#define MAX_PACKS_HS    (MAX_PACKS * 8) /* in high speed mode */

/* bInterfaceProtocol values to denote the version of the standard used */
#define UAC_VERSION_1           0x00
#define UAC_VERSION_2           0x20
#define UAC_VERSION_3           0x30
#define ADC_INTERFACE_NBR   2
#define ADC_MAX_TERMINAL    2
#define ADC_MAX_SEL_UNIT    2
#define ADC_MAX_FEATURE_UNIT    4
#define ADC_MAX_MIX_UNIT    2

/* A.2 Audio Interface Subclass Codes */
#define USB_SUBCLASS_AUDIOCONTROL   0x01
#define USB_SUBCLASS_AUDIOSTREAMING 0x02
#define USB_SUBCLASS_MIDISTREAMING  0x03

/* A.5 Audio Class-Specific AC Interface Descriptor Subtypes */
#define UAC_HEADER          0x01
#define UAC_INPUT_TERMINAL      0x02
#define UAC_OUTPUT_TERMINAL     0x03
#define UAC_MIXER_UNIT          0x04
#define UAC_SELECTOR_UNIT       0x05
#define UAC_FEATURE_UNIT        0x06
#define UAC1_PROCESSING_UNIT        0x07
#define UAC1_EXTENSION_UNIT     0x08

/* A.6 Audio Class-Specific AS Interface Descriptor Subtypes */
#define UAC_AS_GENERAL          0x01
#define UAC_FORMAT_TYPE         0x02
#define UAC_FORMAT_SPECIFIC     0x03

/* Terminal Control Selectors */
/* 4.3.2  Class-Specific AC Interface Descriptor */
struct uac1_header_descriptor {
    u8  bLength;            /* 8 + n */
    u8  bDescriptorType;        /* USB_DT_CS_INTERFACE */
    u8  bDescriptorSubtype; /* UAC_MS_HEADER */
    u16 bcdADC;         /* 0x0100 */
    u16 wTotalLength;       /* includes Unit and Terminal desc. */
    u8  bInCollection;      /* n */
    u8  baInterfaceNr[ADC_INTERFACE_NBR];       /* [n] */
} __attribute__((packed));

/* 4.3.2.1 Input Terminal Descriptor */
struct uac_input_terminal_descriptor {
    u8  bLength;            /* in bytes: 12 */
    u8  bDescriptorType;        /* CS_INTERFACE descriptor type */
    u8  bDescriptorSubtype; /* INPUT_TERMINAL descriptor subtype */
    u8  bTerminalID;        /* Constant uniquely terminal ID */
    u16 wTerminalType;      /* USB Audio Terminal Types */
    u8  bAssocTerminal;     /* ID of the Output Terminal associated */
    u8  bNrChannels;        /* Number of logical output channels */
    u16 wChannelConfig;
    u8  iChannelNames;
    u8  iTerminal;
} __attribute__((packed));

struct uac_output_terminal_descriptor {
    u8  bLength;            /* in bytes: 9 */
    u8  bDescriptorType;        /* CS_INTERFACE descriptor type */
    u8  bDescriptorSubtype; /* OUTPUT_TERMINAL descriptor subtype */
    u8  bTerminalID;        /* Constant uniquely terminal ID */
    u16 wTerminalType;      /* USB Audio Terminal Types */
    u8  bAssocTerminal;     /* ID of the Input Terminal associated */
    u8  bSourceID;      /* ID of the connected Unit or Terminal*/
    u8  iTerminal;
} __attribute__((packed));

/* 4.3.2.4 Selector Unit Descriptor */
struct uac_selector_unit_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bDescriptorSubtype;
    u8 bUintID;
    u8 bNrInPins;
    u8 baSourceID;
} __attribute__((packed));

/* 4.3.2.5 Feature Unit Descriptor */
struct uac_feature_unit_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bDescriptorSubtype;
    u8 bUnitID;
    u8 bSourceID;
    u8 bControlSize;
    u8 bmaControls[]; /* variable length */
} __attribute__((packed));

/* 4.3.2.3 Mixer Unit Descriptor */
struct uac_mixer_unit_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u8 bDescriptorSubtype;
    u8 bUnitID;
    u8 bNrInPins;
    u8 baSourceID[];
} __attribute__((packed));

/* A.6 Audio Class-Specific AS Interface Descriptor Subtypes */
#define UAC_AS_GENERAL          0x01
#define UAC_FORMAT_TYPE         0x02
#define UAC_FORMAT_SPECIFIC     0x03

/* 4.5.2 Class-Specific AS Interface Descriptor */
struct uac_as_header_descriptor {
    u8  bLength;            /* in bytes: 7 */
    u8  bDescriptorType;        /* USB_DT_CS_INTERFACE */
    u8  bDescriptorSubtype; /* AS_GENERAL */
    u8  bTerminalLink;      /* Terminal ID of connected Terminal */
    u8  bDelay;         /* Delay introduced by the data path */
    u16 wFormatTag;     /* The Audio Data Format */
} __attribute__((packed));

/* Formats - A.1.1 Audio Data Format Type I Codes */
#define UAC_FORMAT_TYPE_I_UNDEFINED 0x0
#define UAC_FORMAT_TYPE_I_PCM       0x1
#define UAC_FORMAT_TYPE_I_PCM8      0x2
#define UAC_FORMAT_TYPE_I_IEEE_FLOAT    0x3
#define UAC_FORMAT_TYPE_I_ALAW      0x4
#define UAC_FORMAT_TYPE_I_MULAW     0x5

struct uac_as_format_type_descriptor {
    u8  bLength;            /* in bytes: 8 + (ns * 3) */
    u8  bDescriptorType;        /* USB_DT_CS_INTERFACE */
    u8  bDescriptorSubtype; /* FORMAT_TYPE */
    u8  bFormatType;        /* FORMAT_TYPE_1 */
    u8  bNrChannels;        /* physical channels in the stream */
    u8  bSubframeSize;      /* */
    u8  bBitResolution;
    u8  bSamFreqType;
    u8  tSamFreq[][3];
} __attribute__((packed));

struct uac_control_desc {
    struct uac1_header_descriptor *uac1_control_header;
    struct uac_input_terminal_descriptor *uac_input_terminal[ADC_MAX_TERMINAL];
    struct uac_output_terminal_descriptor *uac_output_terminal[ADC_MAX_TERMINAL];
    struct uac_selector_unit_descriptor *uac_sel_unit[ADC_MAX_SEL_UNIT];
    struct uac_feature_unit_descriptor *uac_fea_unit[ADC_MAX_FEATURE_UNIT];
    struct uac_mixer_unit_descriptor *uac_mixer_unit[ADC_MAX_MIX_UNIT];
};

struct uac_stream_desc {
    int alternatesetting;
    int intface_index;
    int intface_num;
    bool valid;
    endpoint_t ep;
    struct uac_as_header_descriptor *as_header;
    struct uac_as_format_type_descriptor *fmt;
};

/** @brief Shared buffer structure. */
struct uac_share_buffer {
    uint8_t         *buffer_base;/**< Pointer to the ring buffer. */
    uint32_t         buffer_size;/**< Size of the ring buffer. */
    uint32_t         write;/**< Index of the ring buffer to write the data. */
    uint32_t         read; /**< Index of the ring buffer to read the data. */
    bool             waiting;
    bool             underflow;
};

struct uac_streaming {
    struct uac_device *udev;
    struct uac_stream_desc *stream_desc;
    struct ep_attr *ep_att;
    struct uac_share_buffer share_buf;
    int intfnum;
    u16 maxpsize;
    u8 *frame_buf;
    u32 frame_len;
    void (*pl_callback)(struct uac_streaming *stream);
    void (*cap_callback)(u8 *data, uint32_t buf_len);
    void (*stop_callback)(struct uac_streaming *stream);

    unsigned int error; /*error of frame buf*/
    endpoint_t *ep;

    struct urb *urb[MAX_URBS];
    char *urb_buffer[MAX_URBS];
    QueueHandle_t urb_queue;
    unsigned int urb_size;
    int capture_count;
    u8 urb_num;
    u8 isoc_packet;
    bool file_flag;
    bool play_status;
    char file_name[100];
#ifdef MTK_FATFS_ENABLE
    FIL fid;
#endif /* #ifdef MTK_FATFS_ENABLE */
    xSemaphoreHandle xSemaphore;
};

struct ep_attr {
    int pps;
    int sample_rem;
    int datasize[2];
    int sample_nomial;
    int freqn;
    int freqm;
    int freqmax;
    int maxpacksize;
    int stride;
    int nurbs;
    int npackets;
    int max_urb_frames;
    int fill_max;
    int curpacksize;
    int sample_accum;   /* sample accumulator */
    int rate;
    int channels;
    u8 datainterval;
    u8 slience_value;
    u8 dir;
};

struct uac_device {
    usbdev_t *dev;
    u8 input_terminal_num;
    u8 output_terminal_num;
    u8 select_unit_num;
    u8 feature_unit_num;
    u8 mixer_unit_num;
    struct uac_control_desc control_desc;
    struct uac_stream_desc stream_desc[FUNC_NUM][FUNC_NUM];
    struct uac_streaming *streaming[FUNC_NUM];
    TaskHandle_t xPlayTaskHandle;
    TaskHandle_t xCapTaskHandle;
};

/**
* @addtogroup SSUSB
* @{
* @addtogroup uac
* @{
* This section introduces the USB host usb audio driver export APIs,
* and details on how to use this driver.
*
* - Initialize and start the UAC playback or capture function.
*  - Step1: Call #get_uac_device() to get current uac device.
                  - playback streaming is streaming[0] of uac device.
                  - capture streaming is streaming[1] of uac device.
*  - Step2: Call #uac_set_channel() to config the channel info.
*  - Step3: Call #uac_sel_sample_rate() to config the sample rate.
*  - Step4: Call #uac_reinit_ring_buffer() to reinit ring buffer for playback.
*  - Step5: Call #uac_get_pl_buffer_write_pos() to get ring buffer write pos
*                 and write length for playback.
*  - Step6: Call #uac_set_pl_buffer_write_done() to update write pos for
*                 playback ring buffer.
*  - Step7: Call #uac_register_cap_callback() to register entry to receive data.
*  - Step8: Call #uac_playback_start() to start the playback or capture.
*  - Step9: Call #uac_playback_stop() to stop the playback or capture.
*/

/*****************************************************************************
* Functions
*****************************************************************************/

/**
 * @brief       Get UAC device structure.
 *
 * @return      Return the UAC device structure.
 */
extern struct uac_device *get_uac_device(void);

/**
 * @brief       Re-init the uac playback ring buffer.
 *
 * @param[in]   The uac playback streaming variable.
 * @return      Void.
 */
extern void uac_reinit_ring_buffer(struct uac_streaming *stream);

/**
 * @brief       Get the uac playback ring buffer write position.
 *
 * @param[in]   The uac playback streaming variable.
 * @param[in]   The uac playback ring buffer write position pointer.
 * @param[in]   The length of the uac playback ring buffer can be written.
 * @return      Void.
 */
extern void uac_get_pl_buffer_write_pos(struct uac_streaming *stream,
                                        uint8_t **buffer, uint32_t *length);

/**
 * @brief       update the uac playback ring buffer write position.
 *
 * @param[in]   The uac playback streaming variable.
 * @param[in]   The length of the written for playback ring buffer.
 * @return      Void.
 */
extern void uac_set_pl_buffer_write_done(struct uac_streaming *stream,
                                         uint32_t length);

/**
 * @brief       Register the plyaback callback to uac driver. Will use it to
 *              notify if not enough data in ring buffer.
 *
 * @param[in]   The uac playback streaming variable.
 * @param[in]   Function callback pointer.
 * @return      Void.
 */
extern void uac_register_playback_callback(struct uac_streaming *stream,
                          void (*pl_callback)(struct uac_streaming *stream));

/**
 * @brief       Register the capture callback to uac driver. Will use it to
 *              notify user to receive the capture data.
 *
 * @param[in]   The uac caputre streaming variable.
 * @param[in]   Function callback pointer.
 * @return      Void.
 */
extern void uac_register_cap_callback(struct uac_streaming *stream,
                          void (*cap_callback)(u8 *data, uint32_t len));

/**
 * @brief       Register the stop callback to uac driver. Will use it to
 *              notify user uac already stop.
 *
 * @param[in]   The uac streaming variable.
 * @param[in]   Function callback pointer.
 * @return      Void.
 */
extern void uac_register_stop_callback(struct uac_streaming *stream,
                          void (*stop_callback)(struct uac_streaming *stream));

/**
 * @brief       Set the playback or capture channel info.
 *
 * @param[in]   The uac playback or caputre streaming variable.
 * @param[in]   The channel num will to set.
 * @return      Void.
 */
extern void uac_set_channel(struct uac_streaming *stream, int channels);

/**
 * @brief       Set the playback or capture sample rate info.
 *
 * @param[in]   The uac playback or caputre streaming variable.
 * @param[in]   The sample rate will to set.
 * @return      Void.
 */
extern int uac_sel_sample_rate(struct uac_streaming *stream, int sample);

/**
 * @brief       Begin to start the playback or capture.
 *
 * @param[in]   The uac playback or caputre streaming variable.
 * @param[in]   playback: 0, capture: 1.
 * @return      Void.
 */
extern void uac_playback_start(struct uac_streaming *stream, u8 mode);

/**
 * @brief       Begin to stop the playback or capture.
 *
 * @param[in]   The uac playback or caputre streaming variable.
 * @return      Void.
 */
extern void uac_playback_stop(struct uac_streaming *stream);

#endif /* #ifndef __USBUAC_H */
