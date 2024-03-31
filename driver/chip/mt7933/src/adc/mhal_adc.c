/*
 * (C) 2005-2020 MediaTek Inc. All rights reserved.
 *
 * Copyright Statement:
 *
 * This MT3620 driver software/firmware and related documentation
 * ("MediaTek Software") are protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. ("MediaTek"). You may only use, reproduce, modify, or
 * distribute (as applicable) MediaTek Software if you have agreed to and been
 * bound by this Statement and the applicable license agreement with MediaTek
 * ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User"). If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
 * PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS
 * ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO
 * LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED
 * HEREUNDER WILL BE ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
 * RECEIVER TO MEDIATEK DURING THE PRECEDING TWELVE (12) MONTHS FOR SUCH
 * MEDIATEK SOFTWARE AT ISSUE.
 */

#include "hal_platform.h"

#ifdef HAL_ADC_MODULE_ENABLED
#include "hdl_adc.h"
#include "mhal_adc.h"
#include "mhal_osai.h"
#include "FreeRTOS.h"


#define ADC_DMA_RX_PORT                 0x38000200

/* auxadc apdam register */
#define APDMA_ADC_BASE  0x34407600
#define APDMA_ADC_RX_INT_FLAG   (APDMA_ADC_BASE + 0x0000)
#define APDMA_ADC_RX_INT_FLAG0_BIT BIT(0)
#define APDMA_ADC_RX_INT_FLAG1_BIT BIT(1)
#define APDMA_ADC_RX_INT_EN     (APDMA_ADC_BASE + 0x0004)
#define APDMA_ADC_RX_INT_EN0_BIT BIT(0)
#define APDMA_ADC_RX_INT_EN1_BIT BIT(1)
#define APDMA_ADC_RX_EN         (APDMA_ADC_BASE + 0x0008)
#define APDMA_ADC_RX_EN_BIT     BIT(0)
#define APDMA_ADC_RX_RST            (APDMA_ADC_BASE + 0x000C)
#define APDMA_ADC_WARM_RST  BIT(0)
#define APDMA_ADC_HARD_RST  BIT(1)
#define APDMA_ADC_RX_STOP           (APDMA_ADC_BASE + 0x0010)
#define APDMA_ADC_RX_STOP_BIT   BIT(0)
#define APDMA_ADC_RX_FLUSH      (APDMA_ADC_BASE + 0x0014)
#define APDMA_ADC_RX_VFF_ADDR   (APDMA_ADC_BASE + 0x001c)
#define APDMA_ADC_RX_VFF_LEN        (APDMA_ADC_BASE + 0x0024)
#define APDMA_ADC_RX_VFF_THRE   (APDMA_ADC_BASE + 0x0028)
#define APDMA_ADC_RX_VFF_WPT        (APDMA_ADC_BASE + 0x002C)
#define APDMA_ADC_RX_VFF_RPT        (APDMA_ADC_BASE + 0x0030)
#define APDMA_ADC_RX_FLOW_CTRL_THRE     (APDMA_ADC_BASE + 0x0034)
#define APDMA_ADC_RX_IN_BUF_SIZE        (APDMA_ADC_BASE + 0x0038)
#define APDMA_ADC_RX_VFF_VALID_SIZE (APDMA_ADC_BASE + 0x003c)
#define APDMA_ADC_RX_VFF_LEFT_SIZE  (APDMA_ADC_BASE + 0x0040)
#define APDMA_ADC_RX_DEBUG_STATUS   (APDMA_ADC_BASE + 0x0050)
#define APDMA_ADC_RX_VFF_ADDR2  (APDMA_ADC_BASE + 0x0054)
#define APDMA_ADC_RX_SEC_EN     (0x34407FAC)

#define DMA_GetVFIFO_Avail              DRV_Reg32(APDMA_ADC_RX_VFF_VALID_SIZE)

#define MAX_CH_NUM   16
#define ADC_FIFO_SIZE   32
#define ADC_DMA_BUF_WORD_SIZE  64
#define COUNT_8MS   1000
#define ADC_APDMA_RING_BUFF_LEN  2048

static void _mtk_mhal_adc_fifo_get_length(struct mtk_adc_controller *ctlr,
                                          u32 *length)
{
    switch (ctlr->adc_fsm_parameter->fifo_mode) {
        case ADC_FIFO_DMA:
            break;
        case ADC_FIFO_DIRECT:
            mtk_hdl_adc_fifo_get_length(ctlr->base, length);
            break;
    }
}

static int _mtk_mhal_adc_fifo_read_data(struct mtk_adc_controller *ctlr,
                                        u32 length, u32 *samples_data)
{
    u32 counter = 0;
    int ret = 0;

    switch (ctlr->adc_fsm_parameter->fifo_mode) {
        case ADC_FIFO_DMA:
            break;
        case ADC_FIFO_DIRECT:
            /* Pull out data from RX buffer register */
            for (counter = 0; counter < length; counter++) {
                samples_data[counter] =
                    mtk_hdl_adc_fifo_read_data(ctlr->base);
                adc_debug("\t-length(%d),samples_data[%d]%x.\n",
                          length, counter, samples_data[counter]);
            }
            break;
    }

    return ret;
}

static int _mtk_mhal_adc_period_sample_data(struct mtk_adc_controller *ctlr)
{
    u8 channel_num = 0;
    u32 count = 0, write_point = 0;
    u32 valid_fifo_len = 0;
    u32 fifo_data[ADC_FIFO_SIZE] = {0};
    int ret = 0;

    _mtk_mhal_adc_fifo_get_length(ctlr, &valid_fifo_len);

    /*Query fifo available length */
    ret = _mtk_mhal_adc_fifo_read_data(ctlr, valid_fifo_len, &fifo_data[0]);
    if (ret)
        return ret;

    /*Get fifo data*/
    if (ctlr->adc_fsm_parameter->fifo_mode == ADC_FIFO_DMA)
        valid_fifo_len = valid_fifo_len / 4;

    adc_debug("\tmtk mhal_adc_period_get_data valid fifo len(%d)-.\n",
              valid_fifo_len);

    for (count = 0; count < valid_fifo_len; count++) {
        channel_num = fifo_data[count] & ADC_CH_ID_MASK;
        /*Channel Id: 0~11*/

        if (channel_num >= ADC_CHANNEL_MAX)
            return -ADC_EAGAIN;

        write_point = ctlr->current_xfer[channel_num].write_point;
        /*Prev write pointer*/

        if (ctlr->adc_fsm_parameter->pmode == ADC_PMODE_PERIODIC) {
            if (ctlr->current_xfer[channel_num].count ==
                ADC_RING_BUF_SIZE) {
                adc_debug("period mode buf full, no deal!\n");
            }
        }

        ctlr->current_xfer[channel_num].ring_buf[write_point] =
            ((fifo_data[count] & ADC_DATA_MASK) >> ADC_DATA_BIT_OFFSET);

        adc_debug("ctlr->current_xfer[%d].count = %d.\n",
                  channel_num, ctlr->current_xfer[channel_num].count);

        adc_debug("ctlr->current_xfer[%d].ring_buf[%d] = %d.\n",
                  channel_num, write_point, ctlr->current_xfer[channel_num].ring_buf[write_point]);

        ctlr->current_xfer[channel_num].count++;
        if (ctlr->current_xfer[channel_num].count >= ADC_RING_BUF_SIZE)
            ctlr->current_xfer[channel_num].count = ADC_RING_BUF_SIZE;

        ctlr->current_xfer[channel_num].write_point++;
        /*Next write pointer , ADC sw buf full*/
        if (ctlr->current_xfer[channel_num].write_point >=
            ADC_RING_BUF_SIZE)
            ctlr->current_xfer[channel_num].write_point = 0;
    }

    ctlr->rx_done_callback(ctlr->callback_param);

    return 0;
}

static int _mtk_mhal_adc_dma_release_chan(struct mtk_adc_controller *ctlr)
{
    return osai_dma_release_chan(ctlr->dma_channel);
}

static inline uint32_t vfifo_get_avail(void)
{
    return DRV_Reg32(APDMA_ADC_RX_VFF_VALID_SIZE);
}

static uint16_t cbuffer_get_roomleft(const struct cbuffer *buf)
{
    uint16_t left;

    if (buf->read_idx <= buf->write_idx) {
        left = buf->len - buf->write_idx + buf->read_idx - 1U;
    } else {
        left = buf->read_idx - buf->write_idx - 1U;
    }

    return left;
}

static void vfifo_pop_byte(struct uart_vfifo *vff, uint8_t *chr)
{
    uint32_t offset;
    uint32_t rpt;
    uint32_t addr;

    rpt = DRV_Reg32(APDMA_ADC_RX_VFF_RPT);
    offset = rpt & 0xffffUL;
    addr = vff->membase + offset;
    adc_debug("vfifo_rx_handler: vff->membase: 0x%lx \n", vff->membase);

    *chr = DRV_Reg8(addr);
    adc_debug("vfifo_rx_handler: *chr: 0x%x \n", *chr);
    if (offset == (DRV_Reg32(APDMA_ADC_RX_VFF_LEN) - 1UL)) {
        offset = (~rpt) & 0x10000UL;
        DRV_WriteReg32(APDMA_ADC_RX_VFF_RPT, offset);
    } else {
        DRV_WriteReg32(APDMA_ADC_RX_VFF_RPT, rpt + 1UL);
    }
}

static void cbuffer_push(struct mtk_adc_controller *ctlr, struct cbuffer *buf, const uint8_t *data)
{
#if !defined(DISABLE_ADC_DEBUG)
    uint16_t cnt = 0UL;
    uint32_t  aru4FifoData[ADC_FIFO_SIZE] = {0};
    uint8_t   channel_num = 0;
    uint16_t i = 0UL;
    uint32_t raw_data, voltage;
#endif /* #if !defined(DISABLE_ADC_DEBUG) */

    buf->buf[buf->write_idx] = *data;
    buf->write_idx++;
    if (buf->write_idx >= buf->len) {
        buf->write_idx -= buf->len;
        /*disabel DMA interrupt*/
        mtk_apdma_vfifo_disable_interrupt();

#if !defined(DISABLE_ADC_DEBUG)
        /*adc_debug buf data*/
        while ((cnt < buf->len) && (i < ADC_FIFO_SIZE)) {
            aru4FifoData[i] = (buf->buf[cnt + 3] << 24) | (buf->buf[cnt + 2] << 16) | (buf->buf[cnt + 1] << 8) | (buf->buf[cnt] << 0);
            channel_num = aru4FifoData[i] & ADC_CH_ID_MASK;
            raw_data = ((aru4FifoData[i] & ADC_DATA_MASK) >> ADC_DATA_BIT_OFFSET);
            voltage = raw_data * 1800 / 4096;
            adc_debug("\tChId(%d)/raw_data(%08lX)/voltage(%ld mv) \n", channel_num, raw_data, voltage);
            cnt += 4;
        }
        adc_debug("\t func(%s) dump APDMA register \n", __FUNCTION__);
        for (int i = 0; i < 10; i++) {
            uint32_t val = DRV_Reg32(0x34407600 + i * 16);
            uint32_t val1 = DRV_Reg32(0x34407600 + i * 16 + 4);
            uint32_t val2 = DRV_Reg32(0x34407600 + i * 16 + 8);
            uint32_t val3 = DRV_Reg32(0x34407600 + i * 16 + 12);
            adc_debug("%x:\t%08lx\t%08lx\t%08lx\t%08lx\t\n",
                      0x34407600 + i * 16,
                      val, val1, val2, val3);
        }
#endif /* #if !defined(DISABLE_ADC_DEBUG) */
        /*enable DMA interrupt*/
        mtk_apdma_vfifo_enable_interrupt();
        ctlr->rx_done_callback(ctlr->callback_param);
    }
}

void vfifo_rx_handler(struct mtk_adc_controller *ctlr)
{
    uint32_t room_left;
    uint8_t chr;
    uint32_t dropped = 0UL;
    uint32_t cnt = 0UL;
    struct cbuffer *buf = &ctlr->dma_rx_buf;
    struct uart_vfifo *vff = &ctlr->rx_vfifo;
#if !defined(DISABLE_ADC_DEBUG)
    uint32_t avail_size;
#endif /* #if !defined(DISABLE_ADC_DEBUG) */

    room_left = cbuffer_get_roomleft(buf);

#if !defined(DISABLE_ADC_DEBUG)
    avail_size = vfifo_get_avail();
    adc_debug("vfifo_rx_handler: avail:0x%lx, room_left:0x%lx\n", avail_size, room_left);
#endif /* #if !defined(DISABLE_ADC_DEBUG) */

    if (room_left == 0UL) {
        while (TRUE) {
            if (vfifo_get_avail() > 0UL) {
                vfifo_pop_byte(vff, &chr);
                dropped++;
            } else {
                vff->dropped += dropped;
                break;
            }
        }
    } else {
        while (TRUE) {
            if (vfifo_get_avail() > 0UL) {
                vfifo_pop_byte(vff, &chr);
                cbuffer_push(ctlr, buf, &chr);
                cnt++;
            } else {
                break;
            }
        }
    }

    adc_debug("vfifo_rx_handler: get data len:%ld, drop(%ld)\n", cnt, dropped);
}

void mtk_apdma_vfifo_enable_interrupt(void)
{
    DRV_WriteReg32(APDMA_ADC_RX_INT_EN,
                   APDMA_ADC_RX_INT_EN0_BIT | APDMA_ADC_RX_INT_EN1_BIT);

    adc_debug("\t func(%s) INT_EN = 0x%x\n", __FUNCTION__,
              DRV_Reg32(APDMA_ADC_RX_INT_EN));
}

void mtk_apdma_vfifo_disable_interrupt(void)
{
    DRV_WriteReg32(APDMA_ADC_RX_INT_EN, 0);

    adc_debug("\t func(%s) INT_EN = 0x%x\n", __FUNCTION__,
              DRV_Reg32(APDMA_ADC_RX_INT_EN));
}

static void mtk_apdma_start(void)
{
    DRV_WriteReg32(APDMA_ADC_RX_EN, APDMA_ADC_RX_EN_BIT);
    adc_debug("\t func(%s) RX_EN = 0x%x\n", __FUNCTION__, DRV_Reg32(APDMA_ADC_RX_EN));
}

static void mtk_apdma_reset(void)
{
    DRV_WriteReg32(APDMA_ADC_RX_RST, APDMA_ADC_WARM_RST);
    while (DRV_Reg32(APDMA_ADC_RX_RST) & APDMA_ADC_WARM_RST) {
        adc_debug("\t func(%s) wait warm reset done!!\n", __FUNCTION__);
        osai_delay_ms(10);
    }
}

void cbuffer_init(struct cbuffer *buf, uint8_t *addr, uint16_t size)
{
    buf->read_idx = 0;
    buf->write_idx = 0;
    buf->len = size;
    buf->buf = addr;
}

void DMA_Vfifo_SetAdrs(uint32_t rx_vff_addr, uint32_t rx_vff_len, uint32_t rx_vff_thre)
{
    adc_debug("\t func(%s) rx_vff_addr = 0x%lx, rx_vff_len=0x%lx, rx_vff_thre=0x%lx\n", __FUNCTION__,
              rx_vff_addr, rx_vff_len, rx_vff_thre);
    DRV_WriteReg32(APDMA_ADC_RX_VFF_ADDR, rx_vff_addr);
    DRV_WriteReg32(APDMA_ADC_RX_VFF_LEN, rx_vff_len);
    //DRV_WriteReg32(APDMA_ADC_RX_VFF_THRE, rx_vff_thre);
    DRV_WriteReg32(APDMA_ADC_RX_VFF_THRE, rx_vff_len / 16UL);
    //DRV_WriteReg32(APDMA_ADC_RX_FLOW_CTRL_THRE, rx_vff_len/2UL);
    DRV_WriteReg32(APDMA_ADC_RX_FLOW_CTRL_THRE, rx_vff_len / 16UL);

    adc_debug("\t func(%s) ADDR = 0x%x, LEN=0x%x, THRE=0x%x\n", __FUNCTION__,
              DRV_Reg32(APDMA_ADC_RX_VFF_ADDR),
              DRV_Reg32(APDMA_ADC_RX_VFF_LEN), DRV_Reg32(APDMA_ADC_RX_VFF_THRE));
}

static int _mtk_mhal_adc_dma_config(struct mtk_adc_controller *ctlr)
{
    uint8_t *rx_buffer = NULL;

    ctlr->rx_vfifo.membase = osai_get_phyaddr(ctlr->adc_fsm_parameter->dma_vfifo_addr);
    ctlr->rx_vfifo.memlen = ctlr->adc_fsm_parameter->dma_vfifo_len;
    adc_debug("\t func(%s) ctlr.rx_vfifo.membase(0x%lx)!!\n", __FUNCTION__, ctlr->rx_vfifo.membase);
    adc_debug("\t func(%s) ctlr.rx_vfifo.memlen(0x%lx)!!\n", __FUNCTION__, ctlr->rx_vfifo.memlen);

    rx_buffer = (uint8_t *)pvPortMalloc(ADC_APDMA_RING_BUFF_LEN);
    cbuffer_init(&ctlr->dma_rx_buf, rx_buffer, ADC_APDMA_RING_BUFF_LEN);
    DMA_Vfifo_SetAdrs(ctlr->rx_vfifo.membase, ctlr->rx_vfifo.memlen, ctlr->rx_vfifo.memlen / 2UL);
    mtk_apdma_vfifo_enable_interrupt();
#if 0
    adc_debug("\t func(%s) dump register before APDMA enable\n", __FUNCTION__);
    for (int i = 0; i < 10; i++) {
        uint32_t val = DRV_Reg32(APDMA_ADC_BASE + i * 16);
        uint32_t val1 = DRV_Reg32(APDMA_ADC_BASE + i * 16 + 4);
        uint32_t val2 = DRV_Reg32(APDMA_ADC_BASE + i * 16 + 8);
        uint32_t val3 = DRV_Reg32(APDMA_ADC_BASE + i * 16 + 12);
        adc_debug("%x:\t%08lx\t%08lx\t%08lx\t%08lx\t\n",
                  APDMA_ADC_BASE + i * 16,
                  val, val1, val2, val3);
    }
#endif /* #if 0 */
    mtk_apdma_start();
#if 0
    adc_debug("\t func(%s) dump register after APDMA enable\n", __FUNCTION__);
    for (int i = 0; i < 10; i++) {
        uint32_t val = DRV_Reg32(APDMA_ADC_BASE + i * 16);
        uint32_t val1 = DRV_Reg32(APDMA_ADC_BASE + i * 16 + 4);
        uint32_t val2 = DRV_Reg32(APDMA_ADC_BASE + i * 16 + 8);
        uint32_t val3 = DRV_Reg32(APDMA_ADC_BASE + i * 16 + 12);
        adc_debug("%x:\t%08lx\t%08lx\t%08lx\t%08lx\t\n",
                  APDMA_ADC_BASE + i * 16,
                  val, val1, val2, val3);
    }
#endif /* #if 0 */

    return 0;
}

int mtk_mhal_adc_stop_dma(struct mtk_adc_controller *ctlr)
{
    if (!ctlr)
        return -ADC_EPTR;

    return osai_dma_stop(ctlr->dma_channel);
}

int mtk_mhal_adc_enable_clk(struct mtk_adc_controller *ctlr)
{
    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->cg_base)
        return -ADC_EPTR;

    mtk_hdl_adc_enable_clk(ctlr->cg_base);

    return 0;
}

int mtk_mhal_adc_disable_clk(struct mtk_adc_controller *ctlr)
{
    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->cg_base)
        return -ADC_EPTR;

    mtk_hdl_adc_disable_clk(ctlr->cg_base);

    return 0;
}

int mtk_mhal_adc_init(struct mtk_adc_controller *ctlr)
{
    int ret = 0;

    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    if ((ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DMA) &&
        (ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DIRECT))
        return -ADC_EPARAMETER;

    mtk_hdl_adc_reset(ctlr->rst_base);
    if (ctlr->adc_fsm_parameter->fifo_mode == ADC_FIFO_DMA) {
        /*reset apdma register*/
        mtk_apdma_reset();
    }

    return ret;
}

int mtk_mhal_adc_deinit(struct mtk_adc_controller *ctlr)
{
    int ret = 0;

    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->cg_base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    if ((ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DMA) &&
        (ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DIRECT))
        return -ADC_EPARAMETER;

    if (ctlr->adc_fsm_parameter->fifo_mode == ADC_FIFO_DMA) {
        ret = _mtk_mhal_adc_dma_release_chan(ctlr);
        if (ret)
            return -ADC_EPTR;
    }
    mtk_hal_adc_uninit(ctlr->base);

    return 0;
}

int mtk_mhal_adc_fsm_param_set(struct mtk_adc_controller *ctlr,
                               struct adc_fsm_param *adc_fsm_parameter)
{
    u8 channel_num = 0, i = 0;
    int ret = 0;

    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    if ((adc_fsm_parameter->avg_mode !=  ADC_AVG_1_SAMPLE) &&
        (adc_fsm_parameter->avg_mode !=  ADC_AVG_2_SAMPLE) &&
        (adc_fsm_parameter->avg_mode !=  ADC_AVG_4_SAMPLE) &&
        (adc_fsm_parameter->avg_mode !=  ADC_AVG_8_SAMPLE) &&
        (adc_fsm_parameter->avg_mode !=  ADC_AVG_16_SAMPLE) &&
        (adc_fsm_parameter->avg_mode !=  ADC_AVG_32_SAMPLE) &&
        (adc_fsm_parameter->avg_mode !=  ADC_AVG_64_SAMPLE)) {
        return -ADC_EPARAMETER;
    }

    if ((ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DMA) &&
        (ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DIRECT))
        return -ADC_EPARAMETER;

    if ((adc_fsm_parameter->pmode != ADC_PMODE_ONE_TIME) &&
        (adc_fsm_parameter->pmode != ADC_PMODE_PERIODIC))
        return -ADC_EPARAMETER;

    /* Parameter check */
    if (adc_fsm_parameter->channel_map == 0) {
        adc_err("\tillegal channel map.\n");
        return -ADC_EPARAMETER;
    }

    for (i = 0; i < ADC_CHANNEL_MAX; i++) {
        if (adc_fsm_parameter->channel_map & BIT(i))
            channel_num++;
    }

    ctlr->channel_count = channel_num;
    ctlr->adc_processing = false;

    ctlr->adc_fsm_parameter->pmode = adc_fsm_parameter->pmode;
    ctlr->adc_fsm_parameter->avg_mode = adc_fsm_parameter->avg_mode;
    ctlr->adc_fsm_parameter->channel_map = adc_fsm_parameter->channel_map;
    ctlr->adc_fsm_parameter->period = adc_fsm_parameter->period;
    ctlr->adc_fsm_parameter->fifo_mode = adc_fsm_parameter->fifo_mode;

    mtk_hdl_adc_fsm_param_set(ctlr->base, adc_fsm_parameter->pmode,
                              adc_fsm_parameter->avg_mode, adc_fsm_parameter->channel_map,
                              adc_fsm_parameter->period);

    switch (adc_fsm_parameter->fifo_mode) {
        case ADC_FIFO_DMA:
            /* Initialize ADC Virtual FIFO DMA*/
            /*set alert*/
            adc_debug("\t-DMA Alert Enable-.\n");
            adc_debug("\t-channel_num(%d)-.\n", channel_num);
            adc_debug("\t-dma_vfifo_addr(0x%p)/dma_vfifo_len(%d)-.\n",
                      adc_fsm_parameter->dma_vfifo_addr,
                      adc_fsm_parameter->dma_vfifo_len);

            /* DMA mode */
            ret = _mtk_mhal_adc_dma_config(ctlr);
            if (ret)
                return ret;
            adc_debug("\tmtk mhal_adc_fsm_param_set.\n");
            mtk_hdl_adc_dma_enable(ctlr->base);

            break;
        case ADC_FIFO_DIRECT:

            adc_debug("\tFIFO mode.\n");

            if (adc_fsm_parameter->pmode == ADC_PMODE_ONE_TIME)
                mtk_hdl_adc_periodic_mode_set(ctlr->base,
                                              ADC_PMODE_ONE_TIME);
            if (adc_fsm_parameter->pmode == ADC_PMODE_PERIODIC)
                mtk_hdl_adc_periodic_mode_set(ctlr->base,
                                              ADC_PMODE_PERIODIC);

            adc_debug("channel_num %d.\n", channel_num);
            mtk_hdl_adc_trigger_level_set(ctlr->base, channel_num);
            /*set trigger level equal to used channel number!!*/

            /* Direct mode */
            if ((ctlr->adc_fsm_parameter->ier_mode ==
                 ADC_FIFO_IER_RXFULL) ||
                (ctlr->adc_fsm_parameter->ier_mode ==
                 ADC_FIFO_IER_BOTH)) {
                mtk_hdl_adc_fifo_rx_full_enable(ctlr->base);
            }

            if ((ctlr->adc_fsm_parameter->ier_mode ==
                 ADC_FIFO_IER_TIMEOUT) ||
                (ctlr->adc_fsm_parameter->ier_mode ==
                 ADC_FIFO_IER_BOTH)) {
                mtk_hdl_adc_fifo_rx_timeout_enable(ctlr->base);
            }

            break;
    }

    return 0;
}

int mtk_mhal_adc_fsm_param_get(struct mtk_adc_controller *ctlr,
                               struct adc_fsm_param *adc_fsm_parameter)
{
    u16 pmode = 0;
    u16 avg_mode = 0;
    u16 channel_map = 0;
    u32 period = 0;

    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    mtk_hdl_adc_fsm_param_get(ctlr->base, &pmode,
                              &avg_mode, &channel_map, &period);

    adc_fsm_parameter->pmode = (adc_pmode)pmode;
    adc_fsm_parameter->avg_mode = (adc_avg_mode)avg_mode;
    adc_fsm_parameter->channel_map = channel_map;
    adc_fsm_parameter->period = period;

    return 0;
}

int mtk_mhal_adc_start(struct mtk_adc_controller *ctlr)
{
    int ret = 0;

    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    adc_debug("ctlr->adc_fsm_parameter->fifo_mode == %d.\n",
              ctlr->adc_fsm_parameter->fifo_mode);
    adc_debug("ctlr->adc_fsm_parameter->pmode == %d.\n",
              ctlr->adc_fsm_parameter->pmode);
    if ((ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DMA) &&
        (ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DIRECT))
        return -ADC_EPARAMETER;
    if ((ctlr->adc_fsm_parameter->pmode != ADC_PMODE_ONE_TIME) &&
        (ctlr->adc_fsm_parameter->pmode != ADC_PMODE_PERIODIC))
        return -ADC_EPARAMETER;

    if (ctlr->adc_processing == true) {
        /*ADC is processing*/
        return -ADC_EPARAMETER;
    }

    ctlr->adc_processing = true;

    switch (ctlr->adc_fsm_parameter->fifo_mode) {
        case ADC_FIFO_DMA:
#if 0
            osai_dma_clr_dreq(ctlr->dma_channel);
            ret = osai_dma_start(ctlr->dma_channel);
            if (ret < 0)
                return ret;
#endif /* #if 0 */
            break;
        case ADC_FIFO_DIRECT:
            if (ctlr->adc_fsm_parameter->pmode
                == ADC_PMODE_ONE_TIME) {
                ctlr->adc_fsm_parameter->ier_mode = ADC_FIFO_IER_RXFULL;
                if ((ctlr->adc_fsm_parameter->ier_mode ==
                     ADC_FIFO_IER_RXFULL) ||
                    (ctlr->adc_fsm_parameter->ier_mode ==
                     ADC_FIFO_IER_BOTH)) {
                    mtk_hdl_adc_fifo_rx_full_enable(
                        ctlr->base);
                }
                /*enable interrupt*/
            }

            mtk_hdl_adc_trigger_level_set(ctlr, ctlr->channel_count);
            /*set trigger level equal to used channel number!!*/
            break;
    }

    mtk_hdl_adc_start(ctlr->base);

    return ret;
}

int  mtk_mhal_adc_start_ch(struct mtk_adc_controller *ctlr,
                           u16 ch_bit_map)
{
    int ret = 0;
    u32 channel_num = 0;
    u32 channel_count = 0;

    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    if ((ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DMA) &&
        (ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DIRECT))
        return -ADC_EPARAMETER;

    if ((ctlr->adc_fsm_parameter->pmode != ADC_PMODE_ONE_TIME) &&
        (ctlr->adc_fsm_parameter->pmode != ADC_PMODE_PERIODIC))
        return -ADC_EPARAMETER;

    if (ch_bit_map >= (1 << ADC_CHANNEL_MAX))
        return -ADC_EPARAMETER;

    if (ctlr->adc_processing == true) {
        /*ADC is processing*/
        return -ADC_EPARAMETER;
    }

    ctlr->adc_processing = true;

    ctlr->adc_fsm_parameter->channel_map = ch_bit_map;
    for (channel_count = 0; channel_count < ADC_CHANNEL_MAX;
         channel_count++) {
        if (ctlr->adc_fsm_parameter->channel_map &
            BIT(channel_count))
            channel_num++;
    }

    ctlr->channel_count = channel_num;

    switch (ctlr->adc_fsm_parameter->fifo_mode) {
        case ADC_FIFO_DMA:
#if 0
            osai_dma_clr_dreq(ctlr->dma_channel);
            osai_dma_start(ctlr->dma_channel);
#endif /* #if 0 */
            break;
        case ADC_FIFO_DIRECT:
            if (ctlr->adc_fsm_parameter->pmode
                == ADC_PMODE_ONE_TIME) {
                ctlr->adc_fsm_parameter->ier_mode = ADC_FIFO_IER_RXFULL;
                if ((ctlr->adc_fsm_parameter->ier_mode ==
                     ADC_FIFO_IER_RXFULL) ||
                    (ctlr->adc_fsm_parameter->ier_mode ==
                     ADC_FIFO_IER_BOTH)) {
                    mtk_hdl_adc_fifo_rx_full_enable(
                        ctlr->base);
                }
                /*enable interrupt*/
            }

            mtk_hdl_adc_trigger_level_set(ctlr->base, channel_num);
            /*set trigger level equal to used channel number!!*/
            break;
    }

    mtk_hdl_adc_start_ch(ctlr->base, ch_bit_map);

    return ret;
}

int mtk_mhal_adc_stop(struct mtk_adc_controller *ctlr)
{
    int ret = 0;

    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    if ((ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DMA) &&
        (ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DIRECT))
        return -ADC_EPARAMETER;

    mtk_hdl_adc_stop(ctlr->base);

    mtk_hdl_adc_reset(ctlr->rst_base);

    return ret;
}
int mtk_mhal_adc_one_shot_get_data(struct mtk_adc_controller *ctlr,
                                   adc_channel channel, u32 *data)
{
    u32 write_point = 0;
    u32 read_point = 0;

    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!data)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    if (ctlr->adc_fsm_parameter->pmode != ADC_PMODE_ONE_TIME)
        return -ADC_EPARAMETER;

    if (channel >= ADC_CHANNEL_MAX)
        return -ADC_EPARAMETER;

    write_point = ctlr->current_xfer[(u32)channel].write_point;
    /*get prev data to guarantee latest*/

    if (!write_point)
        read_point = ADC_RING_BUF_SIZE - 1;
    else
        read_point = --write_point;
    adc_debug("data point:%p, write_point %d.\n", data, read_point);
    adc_debug("data point:%p, write_point %d.\n", data, write_point);

    *data = ctlr->current_xfer[(u32)channel].ring_buf[read_point];

    adc_debug("mtk mhal_adc_one_shot_get_data : channel->%d,data %d.\n",
              channel, ctlr->current_xfer[(u32)channel].ring_buf[read_point]);

    adc_debug("data point:%d.\n", *data);

    return 0;
}

int mtk_mhal_adc_dma_period_get_data(struct mtk_adc_controller *ctlr,
                                     adc_channel channel, u32 *data, u32 *length)
{
    u32 count = 0;
    uint32_t  aru4FifoData[ADC_FIFO_SIZE] = {0};
    uint8_t   channel_num = 0;
    uint16_t i = 0UL;
    uint32_t raw_data;
#if !defined(DISABLE_ADC_DEBUG)
    uint32_t voltage;
#endif /* #if !defined(DISABLE_ADC_DEBUG) */

    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    if ((ctlr->adc_fsm_parameter->pmode != ADC_PMODE_ONE_TIME) &&
        (ctlr->adc_fsm_parameter->pmode != ADC_PMODE_PERIODIC))
        return -ADC_EPARAMETER;

    if (ctlr->adc_fsm_parameter->fifo_mode != ADC_FIFO_DMA)
        return -ADC_EPARAMETER;

    if (channel >= ADC_CHANNEL_MAX)
        return -ADC_EPARAMETER;

    struct cbuffer *buf = &ctlr->dma_rx_buf;

    /*adc_debug buf data*/
    while ((count < buf->len) && (i < ADC_FIFO_SIZE)) {
        aru4FifoData[i] = (buf->buf[count + 3] << 24) | (buf->buf[count + 2] << 16) | (buf->buf[count + 1] << 8) | (buf->buf[count] << 0);
        channel_num = aru4FifoData[i] & ADC_CH_ID_MASK;
        raw_data = ((aru4FifoData[i] & ADC_DATA_MASK) >> ADC_DATA_BIT_OFFSET);
#if !defined(DISABLE_ADC_DEBUG)
        voltage = raw_data * 1800 / 4096;
        adc_debug("\t ChId(%d)/raw_data(%08lX)/voltage(%ld mv) \n", channel_num, raw_data, voltage);
#endif /* #if !defined(DISABLE_ADC_DEBUG) */
        if (channel_num == (uint8_t)channel) {
            *data = raw_data;
            *length = i;
            return 0;
        }
        count += 4;
    }

    return 0;
}


int mtk_mhal_adc_period_get_data(struct mtk_adc_controller *ctlr,
                                 adc_channel channel, u32 *data, u32 *length)
{
    u32 write_point = 0;
    u32 read_point = 0;
    u32 count = 0;

    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    if ((ctlr->adc_fsm_parameter->pmode != ADC_PMODE_ONE_TIME) &&
        (ctlr->adc_fsm_parameter->pmode != ADC_PMODE_PERIODIC))
        return -ADC_EPARAMETER;

    if (channel >= ADC_CHANNEL_MAX)
        return -ADC_EPARAMETER;

    ctlr->rx_size = 0;

    write_point = ctlr->current_xfer[(u32)channel].write_point;

    if (ctlr->adc_fsm_parameter->pmode == ADC_PMODE_ONE_TIME) {
        if (!write_point)
            read_point = ADC_RING_BUF_SIZE - 1;
        else
            read_point = --write_point;
        *(ctlr->rx_buf) =
            ctlr->current_xfer[(u32)channel].ring_buf[read_point];
        ctlr->rx_size = 1;
        adc_debug("one shot mode ctlr->rx_buf:%d.\n", *(ctlr->rx_buf));

        return 0;
    }

    if (ctlr->adc_fsm_parameter->pmode == ADC_PMODE_PERIODIC) {
        for (count = ctlr->current_xfer[(u32)channel].count; count > 0;
             count--) {
            /*Next write pointer ,ADC sw buf full*/
            read_point = ctlr->current_xfer[(u32)channel].read_point;
            if (read_point >= ADC_RING_BUF_SIZE) {
                ctlr->current_xfer[(u32)channel].read_point = 0;
                read_point = 0;
            }

            data[ctlr->rx_size] =
                ctlr->current_xfer[(u32)channel].ring_buf[read_point];

            adc_debug("channel- >%d,data:%d.\n",
                      channel, data[ctlr->rx_size]);
            adc_debug("channel->%d,size:%d.\n",
                      channel,
                      ctlr->rx_size);
            ctlr->current_xfer[(u32)channel].count--;
            ctlr->current_xfer[(u32)channel].read_point++;
            ctlr->rx_size++;
        }
        *length = ctlr->rx_size;
    }

    return 0;
}


int mtk_mhal_adc_rx_notify_callback_register(struct mtk_adc_controller *ctlr,
                                             adc_rx_done_callback callback,
                                             void *callback_param)
{
    if (!ctlr || !callback || !callback_param)
        return -ADC_EPTR;

    ctlr->callback_param = callback_param;
    ctlr->rx_done_callback = callback;

    return 0;
}

int mtk_mhal_adc_fifo_handle_rx(struct mtk_adc_controller *ctlr)
{
    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    _mtk_mhal_adc_period_sample_data(ctlr);

    if (ctlr->adc_fsm_parameter->pmode == ADC_PMODE_ONE_TIME) {
        adc_debug("mtk_hdl_adc_stop one shot.\n");
        mtk_hdl_adc_stop(ctlr->base);
    }
    ctlr->adc_processing = false;

    return 0;
}

int mtk_mhal_adc_apdma_fifo_handle_rx(struct mtk_adc_controller *ctlr)
{
    if (!ctlr)
        return -ADC_EPTR;

    if (!ctlr->base)
        return -ADC_EPTR;

    if (!ctlr->adc_fsm_parameter)
        return -ADC_EPTR;

    adc_debug("mtk_mhal_adc_apdma_fifo_handle_rx\n");

    /*clear interrupt flag*/
    DRV_WriteReg32(APDMA_ADC_RX_INT_FLAG, APDMA_ADC_RX_INT_FLAG0_BIT | APDMA_ADC_RX_INT_FLAG1_BIT);

    vfifo_rx_handler(ctlr);

    ctlr->adc_processing = false;

    return 0;
}


#endif /* #ifdef HAL_ADC_MODULE_ENABLED */
