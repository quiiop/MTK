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
#include <xhci_private.h>

/*
 * Allocate transfer buffers. This function can be called with buffers
 * already allocated when resuming from suspend, in which case it will
 * return without touching the buffers.
 *
 * Limit the buffer size to UVC_MAX_PACKETS bulk/isochronous packets. If the
 * system is too low on memory try successively smaller numbers of packets
 * until allocation succeeds.
 *
 * Return the number of allocated packets on success or 0 when out of memory.
 */

static int uvc_alloc_urb_buffers(struct uvc_streaming *stream,
                                 unsigned int size, unsigned int psize)
{
    unsigned int npackets;
    unsigned int i;

    /* Buffers are already allocated, bail out. */
    if (stream->urb_size)
        return stream->urb_size / psize;

    /* Compute the number of packets. Bulk endpoints might transfer UVC
     * payloads across multiple URBs.
     */
    npackets = DIV_ROUND_UP(size, psize);
    if (npackets > stream->isoc_packet)
        npackets = stream->isoc_packet;

    for (i = 0; i < stream->urb_num; ++i) {
        stream->urb_size = psize * npackets;
        //stream->urb_buffer[i] = pvPortMallocNC(stream->urb_size);
        stream->urb_buffer[i] =    xhci_align(64 * 1024, stream->urb_size);
        uvc_debug("%s %d urb[%d]: %p\n", __func__, __LINE__, i, (void *)stream->urb_buffer[i]);
        memset(stream->urb_buffer[i], 0, stream->urb_size);
        if (!stream->urb_buffer[i]) {
            //uvc_free_urb_buffers(stream);
            uvc_debug("%s %d\n", __func__, __LINE__);
            return 0;
        }
    }

    uvc_debug("%s %d %d\n", __func__, __LINE__, npackets);
    return npackets;
}

static int uvc_video_complete(struct urb *urb)
{
    struct uvc_streaming *stream = urb->context;
    u8 *mem;
    int ret, i;

    if (stream->capture_count <= 0) {
        uvc_debug("capture_count end %s %d \n", __func__, __LINE__);
        return 0;
    }

    /*decode*/
    for (i = 0; i < urb->number_of_packets; ++i) {
        u32 temp_len;
        u8 fid;

        if (urb->iso_frame_desc[i].status == 100) {
            uvc_debug("%s %d,%d\n", __func__, __LINE__, urb->iso_frame_desc[i].status);
            continue;
        }

        if (urb->iso_frame_desc[i].status < 0) {
            uvc_debug("%s %d,%d\n", __func__, __LINE__, urb->iso_frame_desc[i].status);
            stream->error = 1;
            continue;
        }

        /* Decode the payload header. */
        temp_len = urb->iso_frame_desc[i].actual_length;
        mem = urb->buffer + urb->iso_frame_desc[i].offset;

        /* Sanity checks:
         * - packet must be at least 2 bytes long
         * - bHeaderLength value must be at least 2 bytes (see above)
         * - bHeaderLength value can't be larger than the packet size.
         */
        //uvc_debug("uvc_video_complete,head 0x%x 0x%x\n", mem[0], mem[1]);
        if (temp_len < 2 || mem[0] < 2 || mem[0] > temp_len) {
            uvc_debug("%s %d,%d \n", __func__, __LINE__, stream->frame_len);
            stream->frame_len = 0;
            stream->error = 1;
            continue;
        }

        /* Mark the buffer as bad if the error bit is set. */
        if (mem[1] & UVC_STREAM_ERR) {
            uvc_debug("%s %d,%d\n", __func__, __LINE__, stream->frame_len);
            stream->error = 1;
            stream->frame_len = 0;
            continue;
        }

        if (!(mem[1] & UVC_STREAM_EOH)) {
            uvc_debug("%s %d,%d\n", __func__, __LINE__, stream->frame_len);
            stream->error = 1;
            stream->frame_len = 0;
            continue;
        }

        fid = mem[1] & UVC_STREAM_FID;
        if (stream->last_fid != fid) {
            /*frame end*/
#if 0
            uvc_debug("uvc_video_complete UVC_STREAM_EOF %d eof %d, frame 0x%x,0x%x,0x%x %d,0x%x 0x%x 0x%x 0x%x\n", temp_len, stream->eof,
                      mem[11], mem[10], mem[1], stream->frame_len, (stream->frame_buf)[0], (stream->frame_buf)[1],
                      (stream->frame_buf)[stream->frame_len - 2], (stream->frame_buf)[stream->frame_len - 1]);
#endif /* #if 0 */
            if (stream->error)
                stream->error = 0;
            else {
                if (0xff == (stream->frame_buf)[0]
                    && 0xd8 == (stream->frame_buf)[1]
                    && 0xff == (stream->frame_buf)[stream->frame_len - 2]
                    && 0xd9 == (stream->frame_buf)[stream->frame_len - 1]) {
                    /*callback if end of frame*/
                    if (stream->callback)
                        stream->callback(stream->frame_buf, stream->frame_len);

                    //stream->capture_count--;
                    if (!stream->capture_count) {
                        uvc_debug("capture_count end %s %d\n", __func__, __LINE__);
                        vPortFree(stream->frame_buf);
                        return 0;
                    }
                } else
                    usb_err("not jpeg %s %d\n", __func__, __LINE__);
            }
            /*start new frame*/
            stream->last_fid = fid;
            stream->frame_len = 0;
            stream->eof = 0;
        }

        if (temp_len > mem[0]) {
            /* Copy the video data to the buffer. */
            memcpy(stream->frame_buf + stream->frame_len, mem + mem[0], temp_len - mem[0]);
            ret = (temp_len - mem[0]);
            stream->frame_len += ret;
        }

        /* Mark the buffer as done if the EOF marker is set. */
        if (mem[1] & UVC_STREAM_EOF /*&& stream->frame_len != 0*/) {
            stream->eof = 1;
            //stream->last_fid ^= UVC_STREAM_FID;
        }
    }

    /*submit urb*/
    memset(urb->buffer, 0, urb->transfer_buffer_length);
    ret = urb->ep->dev->controller->isochronous(urb->ep, urb);
    if (ret < 0) {
        /*uvc_printk(KERN_ERR, "Failed to submit URB %u "
                    "(%d).\n", i, ret);
            uvc_uninit_video(stream, 1);*/
        return ret;
    }

    return 0;
}

static int uvc_init_video_isoc(struct uvc_streaming *stream, endpoint_t *ep)
{
    struct urb *urb;
    unsigned int npackets, i, j;
    u16 psize;
    u32 size;

    psize = usb_endpoint_maxp(ep) * usb_endpoint_maxp_mult(ep);/**/
    uvc_debug("%s %d,0x%x, %d\n", __func__, __LINE__, ep->maxpacketsize, psize);
    size = 614400;//dwMaxVideoFrameSize stream->ctrl.dwMaxPayloadTransferSize;
    npackets = uvc_alloc_urb_buffers(stream, size, psize);
    if (npackets == 0)
        return -1;

    uvc_debug("uvc_init_video_isoc,psize %d npackets %d\n", psize, npackets);

    for (i = 0; i < stream->urb_num; ++i) {
        urb = usb_alloc_urb(npackets);
        if (urb == NULL) {
            /*uvc_uninit_video(stream, 1);*/
            return -1;
        }

        urb->in = ep->direction;
        urb->ep = ep;
        urb->transfer_dma = virt_to_phys(stream->urb_buffer[i]);
        urb->transfer_buffer_length = stream->urb_size;
        urb->buffer = stream->urb_buffer[i];
        urb->interval = ep->interval;
        urb->number_of_packets = npackets;

        urb->context = stream;
        urb->complete = uvc_video_complete;

        for (j = 0; j < npackets; ++j) {
            urb->iso_frame_desc[j].offset = j * psize;
            urb->iso_frame_desc[j].length = psize;
        }

        stream->urb[i] = urb;
    }

    return 0;
}

int uvc_submit_urb(struct uvc_streaming *stream)
{
    int ret;
    endpoint_t *best_ep = &stream->udev->isoc_in;
    unsigned int i;

    taskENTER_CRITICAL();
    /* Submit the ISOC URBs. */
    for (i = 0; i < stream->urb_num; ++i) {
        memset(stream->urb[i]->buffer, 0, stream->urb[i]->transfer_buffer_length);
        ret = best_ep->dev->controller->isochronous(best_ep, stream->urb[i]);
        if (ret < 0) {
            /*uvc_printk(KERN_ERR, "Failed to submit URB %u "
                    "(%d).\n", i, ret);
            uvc_uninit_video(stream, 1);*/
            taskEXIT_CRITICAL();
            return ret;
        }
    }
    taskEXIT_CRITICAL();
    return 0;
}

/*
 * Initialize isochronous/bulk URBs and allocate transfer buffers.
*/
static int uvc_init_video(struct uvc_streaming *stream)
{
    int ret;
    endpoint_t *best_ep = &stream->udev->isoc_in;

    /*find ep and Check if the bandwidth is high enough to choose best_ep*/
    /*
    ret = usb_set_interface(stream->dev->udev, intfnum, altsetting);
    if (ret < 0)
        return ret;
    */
    uvc_debug("%s %d\n", __func__, best_ep->maxpacketsize);
    ret = uvc_init_video_isoc(stream, best_ep);

    if (ret < 0)
        return ret;

    /* Submit the ISOC URBs. */
    uvc_submit_urb(stream);

    return 0;
}

/*set callback*/
void uvc_register_user_callback(struct uvc_device *udev,
                                void (*callback)(u8 *data, u32 buf_len),
                                u8 *data)
{
    if (!udev->stream) {
        uvc_debug("%s %d stream is null\n", __func__, __LINE__);
        return;
    }
    uvc_debug("%s %d\n", __func__, __LINE__);
    udev->stream->callback = callback;
    udev->stream->frame_buf = data;
}
/*
 * Enable or disable the video stream.
 */
struct uvc_streaming *uvc_stream_new(struct uvc_device *udev)
{
    struct uvc_streaming *stream;
    uvc_debug("%s \n", __func__);

    stream = pvPortMalloc(sizeof(struct uvc_streaming));
    if (!stream) {
        uvc_debug("%s NULL\n", __func__);
        return NULL;
    }

    memset(stream, 0, sizeof(struct uvc_streaming));
    stream->urb_size = 0;
    stream->udev = udev;
    return stream;
}
int uvc_video_enable(struct uvc_streaming *stream, int enable)
{
    int ret = 0;

    uvc_debug("%s \n", __func__);
    if (!stream) {
        uvc_debug("%s !stream\n", __func__);
        return 0;
    }

    if (!enable) {
        /*uvc_uninit_video(stream, 1);*/
        stream->enable = 0;
        return 0;
    }

    ret = uvc_init_video(stream);
    stream->enable = 1;
    if (ret < 0)
        goto error_video;

    return 0;

error_video:
    //usb_set_interface(stream->dev->udev, stream->intfnum, 0);

    return ret;
}


