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

#include "internal_list.h"
#include "xhci.h"
#include "xhci_mtk.h"
#include "xhci_private.h"
#include "usb.h"

#define SSP_BW_BOUNDARY 130000
#define SS_BW_BOUNDARY  51000
/* table 5-5. High-speed Isoc Transaction Limits in usb_20 spec */
#define HS_BW_BOUNDARY  6144
/* usb2 spec section11.18.1: at most 188 FS bytes per microframe */
#define FS_PAYLOAD_MAX 188
/*
 * max number of microframes for split transfer,
 * for fs isoc in : 1 ss + 1 idle + 7 cs
 */
#define TT_MICROFRAMES_MAX 9

/* mtk scheduler bitmasks */
#define EP_BPKTS(p) ((p) & 0x7f)
#define EP_BCSCOUNT(p)  (((p) & 0x7) << 8)
#define EP_BBM(p)   ((p) << 11)
#define EP_BOFFSET(p)   ((p) & 0x3fff)
#define EP_BREPEAT(p)   (((p) & 0x7fff) << 16)

static inline int _fls(int x)
{
    int position;
    int i;

    if (x != 0) {
        for (i = (x >> 1), position = 0; i != 0; ++position) {
            i = i >> 1;
        }
    } else {
        position = -1;
    }

    return position + 1;
}

static inline unsigned int roundup_pow_of_two(unsigned int x)
{
    return 1UL << _fls(x - 1);
}

static int is_fs_or_ls(usb_speed speed)
{
    return speed == FULL_SPEED || speed == LOW_SPEED;
}

/*
* get the index of bandwidth domains array which @ep belongs to.
*
* the bandwidth domain array is saved to @sch_array of struct xhci_hcd_mtk,
* each HS root port is treated as a single bandwidth domain,
* but each SS root port is treated as two bandwidth domains, one for IN eps,
* one for OUT eps.
* @real_port value is defined as follow according to xHCI spec:
* 1 for SSport0, ..., N+1 for SSportN, N+2 for HSport0, N+3 for HSport1, etc
* so the bandwidth domain array is organized as follow for simplification:
* SSport0-OUT, SSport0-IN, ..., SSportX-OUT, SSportX-IN, HSport0, ..., HSportY
*/
static int get_bw_index(xhci_t *xhci, usbdev_t *udev, endpoint_t *ep)
{
    struct xhci_hcd_mtk *mtk = xhci->priv;
    devinfo_t *di;
    int bw_index;
    u8 real_port;   /* aka port_id */

    di = &xhci->dev[udev->address];
    real_port = SC_GET(RHPORT,  di->ctx.slot);

    if (udev->speed >= SUPER_SPEED) {
        if (usb_endpoint_dir_out(ep->direction))
            bw_index = (real_port - 1) * 2;
        else
            bw_index = (real_port - 1) * 2 + 1;
    } else {
        /* add one more for each SS port */
        bw_index = real_port + mtk->num_u3_ports - 1;
    }

    return bw_index;
}

static u32 get_esit(epctx_t *ep_ctx)
{
    u32 esit;

    esit = 1 << EC_GET(INTVAL, ep_ctx);
    if (esit > XHCI_MTK_MAX_ESIT)
        esit = XHCI_MTK_MAX_ESIT;

    return esit;
}
#if 0
static struct mu3h_sch_tt *find_tt(struct usb_device *udev)
{
    struct usb_tt *utt = udev->tt;
    struct mu3h_sch_tt *tt, **tt_index, **ptt;
    unsigned int port;
    bool allocated_index = false;

    if (!utt)
        return NULL;    /* Not below a TT */

    /*
     * Find/create our data structure.
     * For hubs with a single TT, we get it directly.
     * For hubs with multiple TTs, there's an extra level of pointers.
     */
    tt_index = NULL;
    if (utt->multi) {
        tt_index = utt->hcpriv;
        if (!tt_index) {    /* Create the index array */
            tt_index = pvPortMalloc(utt->hub->maxchild * sizeof(*tt_index));
            if (!tt_index)
                return NULL;
            memset(tt_index, 0,  utt->hub->maxchild * sizeof(*tt_index));
            utt->hcpriv = tt_index;
            allocated_index = true;
        }
        port = udev->ttport - 1;
        ptt = &tt_index[port];
    } else {
        port = 0;
        ptt = (struct mu3h_sch_tt **) &utt->hcpriv;
    }

    tt = *ptt;
    if (!tt) {  /* Create the mu3h_sch_tt */
        tt = pvPortMalloc(sizeof(*tt));
        if (!tt) {
            if (allocated_index) {
                utt->hcpriv = NULL;
                vPortFree(tt_index);
            }
            return NULL;
        }
        memset(tt, 0,  sizeof(*tt));
        INIT_LIST_HEAD(&tt->ep_list);
        tt->usb_tt = utt;
        tt->tt_port = port;
        *ptt = tt;
    }

    return tt;
}

/* Release the TT above udev, if it's not in use */
static void drop_tt(struct usb_device *udev)
{
    struct usb_tt *utt = udev->tt;
    struct mu3h_sch_tt *tt, **tt_index, **ptt;
    int i, cnt;

    if (!utt || !utt->hcpriv)
        return;     /* Not below a TT, or never allocated */

    cnt = 0;
    if (utt->multi) {
        tt_index = utt->hcpriv;
        ptt = &tt_index[udev->ttport - 1];
        /*  How many entries are left in tt_index? */
        for (i = 0; i < utt->hub->maxchild; ++i)
            cnt += !!tt_index[i];
    } else {
        tt_index = NULL;
        ptt = (struct mu3h_sch_tt **)&utt->hcpriv;
    }

    tt = *ptt;
    if (!tt || !list_empty(&tt->ep_list))
        return;     /* never allocated , or still in use*/

    *ptt = NULL;
    vPortFree(tt);

    if (cnt == 1) {
        utt->hcpriv = NULL;
        vPortFree(tt_index);
    }
}
#endif /* #if 0 */
static struct mu3h_sch_ep_info *create_sch_ep(usbdev_t *udev, endpoint_t *ep, epctx_t *ep_ctx)
{
    struct mu3h_sch_ep_info *sch_ep;
    struct mu3h_sch_tt *tt = NULL;
    u32 len_bw_budget_table;
    unsigned int mem_size;

    /* if (is_fs_or_ls(udev->speed))
        len_bw_budget_table = TT_MICROFRAMES_MAX;
    else */ if ((udev->speed >= SUPER_SPEED)
                && usb_endpoint_xfer_isoc(ep->type))
        len_bw_budget_table = get_esit(ep_ctx);
    else
        len_bw_budget_table = 1;

    mem_size = sizeof(struct mu3h_sch_ep_info) +
               len_bw_budget_table * sizeof(u32);
    sch_ep = pvPortMalloc(mem_size);
    if (!sch_ep)
        return NULL;
    /*
    if (is_fs_or_ls(udev->speed)) {
        tt = find_tt(udev);
        if (!tt) {
            vPortFree(sch_ep);
            return NULL;
        }
    }
    */
    sch_ep->sch_tt = tt;
    sch_ep->ep = ep;

    return sch_ep;
}

static void setup_sch_info(usbdev_t *udev, epctx_t *ep_ctx, struct mu3h_sch_ep_info *sch_ep)
{
    u32 ep_type, maxpkt, max_burst, mult, esit_pkts, i;
    u32 *bwb_table = sch_ep->bw_budget_table;

    ep_type = EC_GET(TYPE, ep_ctx);
    maxpkt = EC_GET(MPS, ep_ctx);
    max_burst = EC_GET(MBS, ep_ctx);
    mult = EC_GET(MULT, ep_ctx);

    sch_ep->esit = get_esit(ep_ctx);
    sch_ep->ep_type = ep_type;
    sch_ep->maxpkt = maxpkt;
    sch_ep->offset = 0;
    sch_ep->burst_mode = 0;
    sch_ep->repeat = 0;

    if (udev->speed == HIGH_SPEED) {
        sch_ep->cs_count = 0;

        /*
         * usb_20 spec section5.9
         * a single microframe is enough for HS synchromous endpoints
         * in a interval
         */
        sch_ep->num_budget_microframes = 1;

        /*
         * xHCI spec section6.2.3.4
         * @max_burst is the number of additional transactions
         * opportunities per microframe
         */
        sch_ep->pkts = max_burst + 1;
        sch_ep->bw_cost_per_microframe = maxpkt * sch_ep->pkts;
        bwb_table[0] = sch_ep->bw_cost_per_microframe;
    } else if (udev->speed >= SUPER_SPEED) {
        /* usb3_r1 spec section4.4.7 & 4.4.8 */
        sch_ep->cs_count = 0;
        esit_pkts = (mult + 1) * (max_burst + 1);
        if (ep_type == INT_IN_EP || ep_type == INT_OUT_EP) {
            sch_ep->pkts = esit_pkts;
            sch_ep->num_budget_microframes = 1;
            bwb_table[0] = maxpkt * sch_ep->pkts;
        }

        if (ep_type == ISOC_IN_EP || ep_type == ISOC_OUT_EP) {
            u32 remainder;

            if (sch_ep->esit == 1)
                sch_ep->pkts = esit_pkts;
            else if (esit_pkts <= sch_ep->esit)
                sch_ep->pkts = 1;
            else
                sch_ep->pkts = roundup_pow_of_two(esit_pkts)
                               / sch_ep->esit;

            sch_ep->num_budget_microframes =
                DIV_ROUND_UP(esit_pkts, sch_ep->pkts);

            sch_ep->repeat = !!(sch_ep->num_budget_microframes > 1);
            sch_ep->bw_cost_per_microframe = maxpkt * sch_ep->pkts;

            remainder = sch_ep->bw_cost_per_microframe;
            remainder *= sch_ep->num_budget_microframes;
            remainder -= (maxpkt * esit_pkts);
            for (i = 0; i < sch_ep->num_budget_microframes - 1; i++)
                bwb_table[i] = sch_ep->bw_cost_per_microframe;

            /* last one <= bw_cost_per_microframe */
            bwb_table[i] = remainder;
        }
    }
#if 0
    else if (is_fs_or_ls(udev->speed)) {
        sch_ep->pkts = 1; /* at most one packet for each microframe */

        /*
         * num_budget_microframes and cs_count will be updated when
         * check TT for INT_OUT_EP, ISOC/INT_IN_EP type
         */
        sch_ep->cs_count = DIV_ROUND_UP(maxpkt, FS_PAYLOAD_MAX);
        sch_ep->num_budget_microframes = sch_ep->cs_count;
        sch_ep->bw_cost_per_microframe =
            (maxpkt < FS_PAYLOAD_MAX) ? maxpkt : FS_PAYLOAD_MAX;

        /* init budget table */
        if (ep_type == ISOC_OUT_EP) {
            for (i = 0; i < sch_ep->num_budget_microframes; i++)
                bwb_table[i] =  sch_ep->bw_cost_per_microframe;
        } else if (ep_type == INT_OUT_EP) {
            /* only first one consumes bandwidth, others as zero */
            bwb_table[0] = sch_ep->bw_cost_per_microframe;
        } else { /* INT_IN_EP or ISOC_IN_EP */
            bwb_table[0] = 0; /* start split */
            bwb_table[1] = 0; /* idle */
            /*
             * due to cs_count will be updated according to cs
             * position, assign all remainder budget array
             * elements as @bw_cost_per_microframe, but only first
             * @num_budget_microframes elements will be used later
             */
            for (i = 2; i < TT_MICROFRAMES_MAX; i++)
                bwb_table[i] =  sch_ep->bw_cost_per_microframe;
        }
    }
#endif /* #if 0 */
}

/* Get maximum bandwidth when we schedule at offset slot. */
static u32 get_max_bw(struct mu3h_sch_bw_info *sch_bw,
                      struct mu3h_sch_ep_info *sch_ep, u32 offset)
{
    u32 i, j, num_esit;
    u32 max_bw = 0;
    u32 bw;

    num_esit = XHCI_MTK_MAX_ESIT / sch_ep->esit;
    for (i = 0; i < num_esit; i++) {
        u32 base = offset + i * sch_ep->esit;

        for (j = 0; j < sch_ep->num_budget_microframes; j++) {
            bw = sch_bw->bus_bw[base + j] +
                 sch_ep->bw_budget_table[j];
            if (bw > max_bw)
                max_bw = bw;
        }
    }
    return max_bw;
}

static void update_bus_bw(struct mu3h_sch_bw_info *sch_bw,
                          struct mu3h_sch_ep_info *sch_ep, bool used)
{
    u32 i, j, num_esit;
    u32 base;

    num_esit = XHCI_MTK_MAX_ESIT / sch_ep->esit;
    for (i = 0; i < num_esit; i++) {
        base = sch_ep->offset + i * sch_ep->esit;
        for (j = 0; j < sch_ep->num_budget_microframes; j++) {
            if (used) {
                sch_bw->bus_bw[base + j] +=
                    sch_ep->bw_budget_table[j];
            } else {
                sch_bw->bus_bw[base + j] -=
                    sch_ep->bw_budget_table[j];
            }
        }
    }
}
#if 0
static int check_sch_tt(struct usb_device *udev,
                        struct mu3h_sch_ep_info *sch_ep, u32 offset)
{
    struct mu3h_sch_tt *tt = sch_ep->sch_tt;
    u32 extra_cs_count;
    u32 fs_budget_start;
    u32 start_ss, last_ss;
    u32 start_cs, last_cs;
    int i;

    start_ss = offset % 8;
    fs_budget_start = (start_ss + 1) % 8;

    if (sch_ep->ep_type == ISOC_OUT_EP) {
        last_ss = start_ss + sch_ep->cs_count - 1;

        /*
         * usb_20 spec section11.18:
         * must never schedule Start-Split in Y6
         */
        if (!(start_ss == 7 || last_ss < 6))
            return -ERANGE;

        for (i = 0; i < sch_ep->cs_count; i++) {
            if (test_bit(offset + i, tt->ss_bit_map)) {
                return -ERANGE;
            }
            if (test_bit(offset + i, tt->cs_bit_map)) {
                return -ERANGE;
            }
        }
    } else {
        u32 cs_count = DIV_ROUND_UP(sch_ep->maxpkt, FS_PAYLOAD_MAX);

        /*
         * usb_20 spec section11.18:
         * must never schedule Start-Split in Y6
         */
        if (start_ss == 6)
            return -ERANGE;

        /* one uframe for ss + one uframe for idle */
        start_cs = (start_ss + 2) % 8;
        last_cs = start_cs + cs_count - 1;

        if (last_cs > 7)
            return -ERANGE;

        if (sch_ep->ep_type == ISOC_IN_EP)
            extra_cs_count = (last_cs == 7) ? 1 : 2;
        else /*  ep_type : INTR IN / INTR OUT */
            extra_cs_count = (fs_budget_start == 6) ? 1 : 2;

        cs_count += extra_cs_count;

        if (cs_count > 7)
            cs_count = 7; /* HW limit */

        if (test_bit(offset, tt->ss_bit_map)) {
            return -ERANGE;
        }

        for (i = 0; i < cs_count; i++) {
            if (test_bit(offset + 2 + i, tt->cs_bit_map)) {
                return -ERANGE;
            }
        }

        sch_ep->cs_count = cs_count;
        /* one for ss, the other for idle */
        sch_ep->num_budget_microframes = cs_count + 2;

        /*
         * if interval=1, maxp >752, num_budge_micoframe is larger
         * than sch_ep->esit, will overstep boundary
         */
        if (sch_ep->num_budget_microframes > sch_ep->esit)
            sch_ep->num_budget_microframes = sch_ep->esit;
    }

    return 0;
}

static void update_sch_tt(struct usb_device *udev,
                          struct mu3h_sch_ep_info *sch_ep, bool used)
{
    struct mu3h_sch_tt *tt = sch_ep->sch_tt;
    u32 base, num_esit;
    int i, j;

    num_esit = XHCI_MTK_MAX_ESIT / sch_ep->esit;
    for (i = 0; i < num_esit; i++) {
        base = sch_ep->offset + i * sch_ep->esit;
        for (j = 0; j < sch_ep->num_budget_microframes; j++) {
            if (used)
                set_bit(base + j, tt->split_bit_map);
            else
                clear_bit(base + j, tt->split_bit_map);
        }

        if (sch_ep->ep_type == ISOC_OUT_EP) {
            for (j = 0; j < sch_ep->num_budget_microframes; j++) {
                if (used) {
                    set_bit(base + j, tt->ss_bit_map);
                    set_bit(base + j, tt->cs_bit_map);
                } else {
                    clear_bit(base + j, tt->ss_bit_map);
                    clear_bit(base + j, tt->cs_bit_map);
                }
            }
        } else {
            if (used)
                set_bit(base, tt->ss_bit_map);
            else
                clear_bit(base, tt->ss_bit_map);
            for (j = 0; j < sch_ep->cs_count; j++) {
                if (used)
                    set_bit(base + 2 + j, tt->cs_bit_map);
                else
                    clear_bit(base + 2 + j, tt->cs_bit_map);
            }
        }
    }

    if (used)
        list_add_tail(&sch_ep->tt_endpoint, &tt->ep_list);
    else
        list_del(&sch_ep->tt_endpoint);
}
#endif /* #if 0 */
static int check_sch_bw(usbdev_t *udev,
                        struct mu3h_sch_bw_info *sch_bw, struct mu3h_sch_ep_info *sch_ep)
{
    u32 offset;
    u32 esit;
    u32 boundary;
    u32 min_bw;
    u32 min_index;
    u32 worst_bw;
    u32 bw_boundary;
    u32 min_num_budget;
    u32 min_cs_count;
    //bool tt_offset_ok = false;
    //int ret;

    esit = sch_ep->esit;

    /*
     * Search through all possible schedule microframes.
     * and find a microframe where its worst bandwidth is minimum.
     */
    min_bw = ~0;
    min_index = 0;
    boundary = esit;
    min_cs_count = sch_ep->cs_count;
    min_num_budget = sch_ep->num_budget_microframes;
    for (offset = 0; offset < esit; offset++) {
        /* if (is_fs_or_ls(udev->speed)) {
            if (sch_ep->ep_type != ISOC_OUT_EP)
                boundary = esit + 1;
            ret = check_sch_tt(udev, sch_ep, offset);
            if (ret)
                continue;
            else
                tt_offset_ok = true;
        }*/

        if ((offset + sch_ep->num_budget_microframes) > boundary)
            break;

        worst_bw = get_max_bw(sch_bw, sch_ep, offset);
        if (min_bw > worst_bw) {
            min_bw = worst_bw;
            min_index = offset;
            min_cs_count = sch_ep->cs_count;
            min_num_budget = sch_ep->num_budget_microframes;
        }
        if (min_bw == 0)
            break;
    }

    if (udev->speed == SUPER_SPEED_PLUS)
        bw_boundary = SSP_BW_BOUNDARY;
    else if (udev->speed == SUPER_SPEED)
        bw_boundary = SS_BW_BOUNDARY;
    else
        bw_boundary = HS_BW_BOUNDARY;

    /* check bandwidth */
    if (min_bw > bw_boundary)
        return -1;

    sch_ep->offset = min_index;
    sch_ep->cs_count = min_cs_count;
    sch_ep->num_budget_microframes = min_num_budget;
#if 0
    if (is_fs_or_ls(udev->speed)) {
        /* all offset for tt is not ok*/
        if (!tt_offset_ok)
            return -ERANGE;

        update_sch_tt(udev, sch_ep, 1);
    }
#endif /* #if 0 */
    /* update bus bandwidth info */
    update_bus_bw(sch_bw, sch_ep, 1);

    return 0;
}

static bool need_bw_sch(endpoint_t *ep, usb_speed speed, int has_tt)
{
    /* only for periodic endpoints */
    if (usb_endpoint_xfer_control(ep->type)
        || usb_endpoint_xfer_bulk(ep->type))
        return false;

    /*
     * for LS & FS periodic endpoints which its device is not behind
     * a TT are also ignored, root-hub will schedule them directly,
     * but need set @bpkts field of endpoint context to 1.
     */
    if (is_fs_or_ls(speed) && !has_tt)
        return false;

    return true;
}

int xhci_mtk_sch_init(struct xhci_hcd_mtk *mtk)
{
    struct mu3h_sch_bw_info *sch_array;
    int num_usb_bus;
    int i;

    /* ss IN and OUT are separated */
    num_usb_bus = mtk->num_u3_ports * 2 + mtk->num_u2_ports;

    sch_array = pvPortMalloc(num_usb_bus * (sizeof(*sch_array)));
    if (sch_array == NULL)
        return -1;

    for (i = 0; i < num_usb_bus; i++)
        INIT_LIST_HEAD(&sch_array[i].bw_ep_list);

    mtk->sch_array = sch_array;

    return 0;
}

void xhci_mtk_sch_exit(struct xhci_hcd_mtk *mtk)
{
    vPortFree(mtk->sch_array);
}

int xhci_mtk_add_ep_quirk(xhci_t *xhci, usbdev_t *udev, endpoint_t *ep)
{
    struct xhci_hcd_mtk *mtk = xhci->priv;
    epctx_t *ep_ctx;
    devinfo_t *di;
    struct mu3h_sch_bw_info *sch_bw;
    struct mu3h_sch_ep_info *sch_ep;
    struct mu3h_sch_bw_info *sch_array;
    int bw_index;
    int ep_index;
    int ret = 0;

    di = &xhci->dev[udev->address];
    ep_index = xhci_ep_id(ep);

    ep_ctx = xhci->dev[udev->address].ctx.ep[ep_index];
    sch_array = mtk->sch_array;

    usb_debug("%s() type:%d, speed:%d, mpkt:%d, dir:%d, ep:%p\n",
              __func__, ep->type, udev->speed, ep->maxpacketsize,
              usb_endpoint_dir_in(ep->direction), ep);

    if (!need_bw_sch(ep, udev->speed, di->ctx.slot->f3 & TT_SLOT)) {
        /*
         * set @bpkts to 1 if it is LS or FS periodic endpoint, and its
         * device does not connected through an external HS hub
         */
        if (usb_endpoint_xfer_int(ep->type)
            || usb_endpoint_xfer_isoc(ep->type))
            ep_ctx->rsvd[0] |= (EP_BPKTS(1));

        return 0;
    }

    if (is_fs_or_ls(udev->speed)) {
        usb_err("error: schedule not support fs/ls behind external HS hub\n");
        return -1;
    }

    bw_index = get_bw_index(xhci, udev, ep);
    sch_bw = &sch_array[bw_index];
    usb_crit("sch_bw@%p, index=%d\n", sch_bw, bw_index);

    sch_ep = create_sch_ep(udev, ep, ep_ctx);
    if (!sch_ep)
        return -1;

    setup_sch_info(udev, ep_ctx, sch_ep);

    ret = check_sch_bw(udev, sch_bw, sch_ep);
    if (ret) {
        usb_err("Not enough bandwidth!\n");
        //if (is_fs_or_ls(udev->speed))
        //  drop_tt(udev);

        vPortFree(sch_ep);
        return -1;
    }

    list_add_tail(&sch_ep->endpoint, &sch_bw->bw_ep_list);

    ep_ctx->rsvd[0] |= (EP_BPKTS(sch_ep->pkts)
                        | EP_BCSCOUNT(sch_ep->cs_count) | EP_BBM(sch_ep->burst_mode));
    ep_ctx->rsvd[1] |= (EP_BOFFSET(sch_ep->offset)
                        | EP_BREPEAT(sch_ep->repeat));

    usb_crit(" PKTS:%x, CSCOUNT:%x, BM:%x, OFFSET:%x, REPEAT:%x\n",
             sch_ep->pkts, sch_ep->cs_count, sch_ep->burst_mode,
             sch_ep->offset, sch_ep->repeat);

    return 0;
}

void xhci_mtk_drop_ep_quirk(xhci_t *xhci, usbdev_t *udev, endpoint_t *ep)
{
    struct xhci_hcd_mtk *mtk = xhci->priv;
    devinfo_t *di;
    struct mu3h_sch_bw_info *sch_array;
    struct mu3h_sch_bw_info *sch_bw;
    struct mu3h_sch_ep_info *sch_ep;
    int bw_index;

    di = &xhci->dev[udev->address];
    sch_array = mtk->sch_array;

    usb_crit("%s() type:%d, speed:%d, mpks:%d, dir:%d, ep:%p\n",
             __func__, ep->type, udev->speed, ep->maxpacketsize,
             usb_endpoint_dir_in(ep->direction), ep);

    if (!need_bw_sch(ep, udev->speed, di->ctx.slot->f3 & TT_SLOT))
        return;

    bw_index = get_bw_index(xhci, udev, ep);
    sch_bw = &sch_array[bw_index];

    list_for_each_entry(sch_ep, &sch_bw->bw_ep_list, endpoint) {
        if (sch_ep->ep == ep) {
            /*if (is_fs_or_ls(udev->speed)) {
                update_sch_tt(udev, sch_ep, 0);
                drop_tt(udev);
            }*/
            update_bus_bw(sch_bw, sch_ep, 0);
            list_del(&sch_ep->endpoint);
            vPortFree(sch_ep);
            break;
        }
    }
}
