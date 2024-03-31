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

#ifndef __MTU3_H__
#define __MTU3_H__

#include <hal_gpt.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "ssusb_hw_regs.h"
#include "internal_list.h"
#include "mtu3_qmu.h"
#include "usbdcore.h"
#include "usb_phy.h"

#define mtu3_err(x...) printf("[USBD] " x)
#define mtu3_crit(x...) printf("[USBD] " x)

#define  USBD_DBG_LOG 0
#if USBD_DBG_LOG
#define mtu3_debug(x...) printf("[USBD] " x)
#else /* #if USBD_DBG_LOG */
#define mtu3_debug(x...) do {} while (0)
#endif /* #if USBD_DBG_LOG */

/* if want to use PIO mode, comment out the following macro */
#define SUPPORT_QMU

#define EP0 0

#define ENXIO 6     /* No such device or address */
#define ENOMEM 12   /* Not enough core */
#define EACCES 13   /* Permission denied */
#define EBUSY 16    /* Mount device busy */
#define ENODEV 19   /* No such device */
#define EINVAL 22   /* Invalid argument */
#define EPIPE 32    /* Broken pipe */
#define ETIMEDOUT 116   /* Connection timed out */
#define ENOTSUP 134     /* Not supported */
#define ESHUTDOWN 110   /* Can't send after socket shutdown */

#define MT_EP_NUM  (8+8+1) /* 8T8R+EP0 */
#define MAX_EP_NUM 8

/* U3 IP: EP0, TX, RX has separate SRAMs */
#define U3IP_TX_FIFO_START_ADDR   0
#define U3IP_RX_FIFO_START_ADDR   0

/* U2 IP: EP0, TX, RX share one SRAM. 0-63 bytes are reserved for EP0 */
#define U2IP_TX_FIFO_START_ADDR   (64)
#define U2IP_RX_FIFO_START_ADDR   (64 + 512 * (MAX_EP_NUM))

#define U3D_U3IP_DFT_SPEED SSUSB_SPEED_SUPER
#define U3D_U2IP_DFT_SPEED SSUSB_SPEED_HIGH

/*
 * fastboot only supports BULK, alloc 1024B for each ep and offset are
 * also fixed, such as, offset-1024 for ep1, offset-2048 for ep2;
 * so MT_EP_NUM should not greater than 9(ep0 + 4 bulk in + 4 bulk out)
 */
#define U3D_FIFO_SIZE_UNIT 1024

#define EP0_MAX_PKT_SIZE 64
#define EP0_MAX_PKT_SIZE_U3 512

#define USB_FIFOSZ_SIZE_8       (0x03)
#define USB_FIFOSZ_SIZE_16      (0x04)
#define USB_FIFOSZ_SIZE_32      (0x05)
#define USB_FIFOSZ_SIZE_64      (0x06)
#define USB_FIFOSZ_SIZE_128     (0x07)
#define USB_FIFOSZ_SIZE_256     (0x08)
#define USB_FIFOSZ_SIZE_512     (0x09)
#define USB_FIFOSZ_SIZE_1024    (0x0A)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif /* #ifndef ARRAY_SIZE */

#define MU3D_EP_TXCR0(epnum)    (U3D_TX1CSR0 + (((epnum) - 1) * 0x10))
#define MU3D_EP_TXCR1(epnum)    (U3D_TX1CSR1 + (((epnum) - 1) * 0x10))
#define MU3D_EP_TXCR2(epnum)    (U3D_TX1CSR2 + (((epnum) - 1) * 0x10))

#define MU3D_EP_RXCR0(epnum)    (U3D_RX1CSR0 + (((epnum) - 1) * 0x10))
#define MU3D_EP_RXCR1(epnum)    (U3D_RX1CSR1 + (((epnum) - 1) * 0x10))
#define MU3D_EP_RXCR2(epnum)    (U3D_RX1CSR2 + (((epnum) - 1) * 0x10))
#define MU3D_EP_RXCR3(epnum)    (U3D_RX1CSR3 + (((epnum) - 1) * 0x10))

enum usb_dr_mode {
    USB_DR_MODE_UNKNOWN,
    USB_DR_MODE_HOST,
    USB_DR_MODE_PERIPHERAL,
    USB_DR_MODE_OTG,
};

struct mtu3_gpd_ring {
    u32 dma;
    struct qmu_gpd *start;
    struct qmu_gpd *end;
    struct qmu_gpd *enqueue;
    struct qmu_gpd *dequeue;
};

struct mu3d_req {
    struct udc_request req; /* should be first */
    struct list_head list;
    struct udc_endpoint *ept;
    struct qmu_gpd *gpd;
    unsigned int actual;    /* data already sent/rcv */
};

/* endpoint data */
struct udc_endpoint {
    struct mu3d *u3d;
    struct udc_request *req;
    struct list_head req_list;
    struct mtu3_gpd_ring gpd_ring;
    char name[12];
    unsigned int maxpkt;
    unsigned char num;
    unsigned char in;
    unsigned char type; /* Transfer type */
    unsigned int bit;   /* EPT_TX/EPT_RX */
    int endpoint_address;   /* endpoint address */
    unsigned char binterval;
    struct endpoint_descriptor *desc;
    struct ss_ep_comp_descriptor *comp_desc;
};

struct mu3d {
    struct gadget_dev *gdev;
    void *mac_base;
    u32 dev_irq;
    EP0_STATE ep0_state;
    USB_SPEED speed;
    u32 tx_fifo_addr;
    u32 rx_fifo_addr;

    struct udc_endpoint eps[MT_EP_NUM]; /* index 0 is fixed as EP0 */
    struct udc_endpoint *ep0;
    struct mu3d_req ep0_mreq;
    u32 ept_alloc_table;

    u8 address;
    unsigned usb_online: 1;
    unsigned is_u3_ip: 1;
    unsigned test_mode: 1;
};

struct ssusb_mtk {
    struct mu3d u3d;
    void *ippc_base;
    struct mtk_phy_instance phy;
    u32 iddig_irq;
    u32 iddig_gpio;
    u32 vbus_gpio;
    enum usb_dr_mode dr_mode;
};

int wait_for_value(void *addr, u32 msk, u32 value, int us_intvl, int count);
struct udc_endpoint *mtu3_find_ep(struct mu3d *u3d, int ep_num, u8 dir);
void mtu3_intr_enable(struct mu3d *u3d);
void mtu3_soft_disconnect(struct mu3d *u3d);
void mtu3_soft_connect(struct mu3d *u3d);
void handle_ept_complete(struct udc_endpoint *ept, int status);

static inline struct mu3d_req *to_mu3d_req(struct udc_request *req)
{
    return (struct mu3d_req *)req;
}

static inline struct mu3d_req *next_request(struct udc_endpoint *ept)
{
    return list_first_entry_or_null(&ept->req_list, struct mu3d_req, list);
}

#endif /* #ifndef __MTU3_H__ */

