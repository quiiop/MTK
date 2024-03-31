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
 * Author: Min Guo <min.guo@mediatek.com>
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

#ifndef __XHCI_MTK_H
#define __XHCI_MTK_H

#include <timers.h>
#include "usb.h"
#include "internal_list.h"
#include "xhci_private.h"
#include "usb_phy.h"

#define XHCI_MTK_MAX_ESIT   64
#define TT_SLOT     0xff

/**
 * @split_bit_map: used to avoid split microframes overlay
 * @ep_list: Endpoints using this TT
 * @usb_tt: usb TT related
 * @tt_port: TT port number
 */
struct mu3h_sch_tt {
    struct list_head ep_list;
    struct usb_tt *usb_tt;
    int tt_port;
};

/**
 * struct mu3h_sch_bw_info: schedule information for bandwidth domain
 *
 * @bus_bw: array to keep track of bandwidth already used at each uframes
 * @bw_ep_list: eps in the bandwidth domain
 *
 * treat a HS root port as a bandwidth domain, but treat a SS root port as
 * two bandwidth domains, one for IN eps and another for OUT eps.
 */
struct mu3h_sch_bw_info {
    u32 bus_bw[XHCI_MTK_MAX_ESIT];
    struct list_head bw_ep_list;
};

/**
 * struct mu3h_sch_ep_info: schedule information for endpoint
 *
 * @esit: unit is 125us, equal to 2 << Interval field in ep-context
 * @num_budget_microframes: number of continuous uframes
 *      (@repeat==1) scheduled within the interval
 * @bw_cost_per_microframe: bandwidth cost per microframe
 * @endpoint: linked into bandwidth domain which it belongs to
 * @tt_endpoint: linked into mu3h_sch_tt's list which it belongs to
 * @sch_tt: mu3h_sch_tt linked into
 * @ep_type: endpoint type
 * @maxpkt: max packet size of endpoint
 * @ep: address of usb_host_endpoint struct
 * @offset: which uframe of the interval that transfer should be
 *      scheduled first time within the interval
 * @repeat: the time gap between two uframes that transfers are
 *      scheduled within a interval. in the simple algorithm, only
 *      assign 0 or 1 to it; 0 means using only one uframe in a
 *      interval, and 1 means using @num_budget_microframes
 *      continuous uframes
 * @pkts: number of packets to be transferred in the scheduled uframes
 * @cs_count: number of CS that host will trigger
 * @burst_mode: burst mode for scheduling. 0: normal burst mode,
 *      distribute the bMaxBurst+1 packets for a single burst
 *      according to @pkts and @repeat, repeate the burst multiple
 *      times; 1: distribute the (bMaxBurst+1)*(Mult+1) packets
 *      according to @pkts and @repeat. normal mode is used by
 *      default
 * @bw_budget_table: table to record bandwidth budget per microframe
 */
struct mu3h_sch_ep_info {
    u32 esit;
    u32 num_budget_microframes;
    u32 bw_cost_per_microframe;
    struct list_head endpoint;
    struct list_head tt_endpoint;
    struct mu3h_sch_tt *sch_tt;
    u32 ep_type;
    u32 maxpkt;
    void *ep;
    /*
     * mtk xHCI scheduling information put into reserved DWs
     * in ep context
     */
    u32 offset;
    u32 repeat;
    u32 pkts;
    u32 cs_count;
    u32 burst_mode;
    u32 bw_budget_table[0];
};

#define MU3C_U3_PORT_MAX 4
#define MU3C_U2_PORT_MAX 5

struct xhci_hcd_mtk {
    struct mu3h_sch_bw_info *sch_array;
    void *xhci_base;
    void *ippc_base;
    int num_u2_ports;
    int num_u3_ports;
    u32 xhci_irq;
    u32 **vbus_gpio;
    struct mtk_phy_instance phy;
    TimerHandle_t timer;
    void *xhci_hcd;
    int index;
};

int xhci_mtk_sch_init(struct xhci_hcd_mtk *mtk);
void xhci_mtk_sch_exit(struct xhci_hcd_mtk *mtk);
int xhci_mtk_add_ep_quirk(xhci_t *xhci, usbdev_t *udev, endpoint_t *ep);
void xhci_mtk_drop_ep_quirk(xhci_t *xhci, usbdev_t *udev, endpoint_t *ep);
void xhci_hqa_register(struct xhci_hcd_mtk *mtk);
void mtk_usb_host_resume(void *data);
void mtk_usb_host_suspend(void *data);

#endif /* #ifndef __XHCI_MTK_H */

