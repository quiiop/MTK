/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/**
 * @file mt_i2c.c
 * This i2c driver is used to control MediaTek I2C controller.\n
 * It provides the interfaces which will be used in LK.
 */

/**
 * @defgroup IP_group_i2c I2C
 *
 *   @{
 *       @defgroup IP_group_i2c_external EXTERNAL
 *         The external API document for I2C. \n
 *
 *         @{
 *            @defgroup IP_group_i2c_external_function 1.function
 *              External function in i2c driver.
 *            @defgroup IP_group_i2c_external_struct 2.structure
 *              none.
 *            @defgroup IP_group_i2c_external_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_i2c_external_enum 4.enumeration
 *              none.
 *            @defgroup IP_group_i2c_external_def 5.define
 *              none.
 *         @}
 *
 *       @defgroup IP_group_i2c_internal INTERNAL
 *         The internal API document for I2C. \n
 *
 *         @{
 *            @defgroup IP_group_i2c_internal_function 1.function
 *              Internal function in i2c driver.
 *            @defgroup IP_group_i2c_internal_struct 2.structure
 *              Internal structure in i2c driver.
 *            @defgroup IP_group_i2c_internal_typedef 3.typedef
 *              none.
 *            @defgroup IP_group_i2c_internal_enum 4.enumeration
 *              Internal enumeration in i2c driver.
 *            @defgroup IP_group_i2c_internal_def 5.define
 *              Internal define in i2c driver.
 *         @}
 *   @}
 */
#include "hal_i2c_master_internal.h"
#include "hal_clock.h"
#include "hal_log.h"
#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager_internal.h"
#include "hal_sleep_manager.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
struct mtk_i2c *i2c_g[HAL_I2C_MASTER_MAX];
static unsigned char g_addr[HAL_I2C_MASTER_MAX];
unsigned int BASE_ADDR[I2C_PORT_NUM] = {I2C0_BASE, I2C1_BASE};
/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Write data to i2c controller register.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains register base\n
 *     address.
 * @param[in]
 *     offset: register relative base offset value.
 * @param[in]
 *     value: The value set to register.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
void i2c_writel(struct mtk_i2c *i2c, unsigned int offset,
                unsigned int value)
{
    DRV_WriteReg32((i2c->base + offset), value);
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Read data from i2c controller register.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains register base\n
 *     address.
 * @param[in]
 *     offset: register relative base offset value.
 * @return
 *     i2c controller register value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
unsigned int i2c_readl(struct mtk_i2c *i2c, unsigned int offset)
{
    return DRV_Reg32(i2c->base + offset);
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Write data to DMA controller register.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains DMA register\n
 *     base address.
 * @param[in]
 *     offset: register relative base offset value.
 * @param[in]
 *     value: The value set to register.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
void i2c_dma_writel(struct mtk_i2c *i2c, unsigned int offset,
                    unsigned int value)
{
    DRV_WriteReg32((i2c->dmabase + offset), value);
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Read data from DMA controller register.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains DMA register\n
 *     base address.
 * @param[in]
 *     offset: register relative base offset value.
 * @return
 *     DMA controller register value.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
unsigned int i2c_dma_readl(struct mtk_i2c *i2c, unsigned int offset)
{
    return DRV_Reg32(i2c->dmabase + offset);
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Enable i2c clock.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains i2c bus number.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int i2c_clock_enable(hal_i2c_port_t i2c_port)
{
    int ret = 0;

    switch (i2c_port) {
        case HAL_I2C_MASTER_0:
            ret = hal_clock_enable(HAL_CLOCK_CG_I2C0);
            break;
        case HAL_I2C_MASTER_1:
            ret = hal_clock_enable(HAL_CLOCK_CG_I2C1);
            break;
        default:
            log_hal_info("Clock enable failed,invalid id : %d\n", i2c_port);
            ret = HAL_I2C_STATUS_INVALID_PARAMETER;
            break;
    }
    return ret;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Disable i2c clock.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains i2c bus number.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int i2c_clock_disable(hal_i2c_port_t i2c_port)
{
    int ret = 0;

    switch (i2c_port) {
        case HAL_I2C_MASTER_0:
            ret = hal_clock_disable(HAL_CLOCK_CG_I2C0);
            break;
        case HAL_I2C_MASTER_1:
            ret = hal_clock_disable(HAL_CLOCK_CG_I2C1);
            break;
        default:
            log_hal_info("Clock disable failed,invalid id : %d\n", i2c_port);
            ret = HAL_I2C_STATUS_INVALID_PARAMETER;
            break;
    }
    return ret;
}

void mtk_irq_handler_register(struct mtk_i2c *i2c,
                              hal_i2c_callback_t i2c_callback, void *user_data)
{
    i2c->user_data = user_data;
    i2c->i2c_callback = i2c_callback;
}

static void mtk_i2c_irq(unsigned char bus_num)
{
    unsigned short restart_flag = 0;
    unsigned short intr_stat;
    struct mtk_i2c *i2c = i2c_g[bus_num];
    int ret = 0;

    if (i2c->auto_restart)
        restart_flag = I2C_RS_TRANSFER;

    intr_stat = i2c_readl(i2c, OFFSET_INTR_STAT);
    i2c_writel(i2c, OFFSET_INTR_STAT, intr_stat);

    /*
     * when occurs ack error, i2c controller generate two interrupts
     * first is the ack error interrupt, then the complete interrupt
     * i2c->irq_stat need keep the two interrupt value.
     */
    i2c->irq_stat |= intr_stat;

    if (i2c->irq_stat & (I2C_TRANSAC_COMP | restart_flag)) {
        i2c->msg_complete = true;
        if (i2c->irq_stat & I2C_ACKERR)
            ret = HAL_I2C_EVENT_ACK_ERROR;
        else if (i2c->irq_stat & I2C_TRANSAC_COMP)
            ret = HAL_I2C_EVENT_SUCCESS;

        if (i2c->i2c_callback != NULL && i2c->dma_en) {
            memcpy(i2c->tmp_buf, i2c->read_buf, i2c->tmp_len);
            i2c->i2c_callback(g_addr[i2c->id], ret, i2c->user_data);
#ifdef HAL_SLEEP_MANAGER_ENABLED
            hal_sleep_manager_unlock_sleep(SLEEP_LOCK_I2C);    //unlock sleep after i2c_callback
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
        }
    }
}

static void mtk_irq_i2c0_handler(hal_nvic_irq_t irq)
{
    mtk_i2c_irq(HAL_I2C_MASTER_0);
}

static void mtk_irq_i2c1_handler(hal_nvic_irq_t irq)
{
    mtk_i2c_irq(HAL_I2C_MASTER_1);
}

char mtk_i2c_get_running_status(unsigned char id)
{
    struct mtk_i2c *i2c = i2c_g[id];

    if (i2c->bus_busy) {
        log_hal_info("i2c-%d busy !!!\n", i2c->id);
        return HAL_I2C_STATUS_BUS_BUSY;
    }
    return HAL_I2C_STATUS_IDLE;
}

int mtk_i2c_soft_reset(unsigned char id)
{
    int ret = HAL_I2C_STATUS_OK;

    switch (id) {
        case HAL_I2C_MASTER_0:
            DRV_WriteReg32(I2C0_BASE + OFFSET_SOFTRESET, 0x0001);
            break;

        case HAL_I2C_MASTER_1:
            DRV_WriteReg32(I2C1_BASE + OFFSET_SOFTRESET, 0x0001);
            break;

        default:
            log_hal_info(
                "hal_I2C_Get_TxNoOfbytes, invalid para: i2c_port=%d\n",
                id);
            ret = HAL_I2C_STATUS_INVALID_PARAMETER;
            break;
    }
    return ret;
}

unsigned short mtk_i2c_get_rx_bytes(unsigned char id)
{
    if (DRV_Reg32(BASE_ADDR[id] + OFFSET_TRANSAC_LEN) > 1)
        return DRV_Reg32(BASE_ADDR[id] + OFFSET_TRANSFER_LEN_AUX);
    else
        return DRV_Reg32(BASE_ADDR[id] + OFFSET_TRANSFER_LEN);
}

unsigned short mtk_i2c_get_tx_bytes(unsigned char id)
{
    return DRV_Reg32(BASE_ADDR[id] + OFFSET_TRANSFER_LEN);
}

void mtk_irq_init(struct mtk_i2c *i2c)
{
    hal_nvic_disable_irq(i2c->irqnr);

    switch (i2c->id) {
        case HAL_I2C_MASTER_0:
            hal_nvic_register_isr_handler(i2c->irqnr, mtk_irq_i2c0_handler);
            break;
        case HAL_I2C_MASTER_1:
            hal_nvic_register_isr_handler(i2c->irqnr, mtk_irq_i2c1_handler);
            break;
        default:
            log_hal_info("mtk_irq_init failed,invalid id : %d\n", i2c->id);
            break;
    }
    hal_nvic_enable_irq(i2c->irqnr);
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Get i2c bus base address, DMA base address and source clock.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains register base\n
 *     address, DMA base address and bus number information.
 * @return
 *     0, set base address successfully.\n
 *     HAL_I2C_STATUS_INVALID_PARAMETER, invalid i2c bus id.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Invalid i2c bus number, return HAL_I2C_STATUS_INVALID_PARAMETER.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_init_base(struct mtk_i2c *i2c)
{
    switch (i2c->id) {
        case HAL_I2C_MASTER_0:
            i2c->base = I2C0_BASE;
            i2c->dmabase = AP_DMA_BASE + 0x80;
            i2c->irqnr = I2C0_IRQn;
            break;
        case HAL_I2C_MASTER_1:
            i2c->base = I2C1_BASE;
            i2c->dmabase = AP_DMA_BASE + 0x100;
            i2c->irqnr = I2C1_IRQn;
            break;
        default:
            log_hal_info("invalid para: i2c->id=%d\n", i2c->id);
            return HAL_I2C_STATUS_INVALID_PARAMETER;
    }
    i2c->bus_busy = false;
    i2c->i2c_callback = NULL;

    return HAL_I2C_STATUS_OK;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Initialize i2c hardware, soft reset i2c controller, then\n
 *     configure io mode and control registers.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains register base\n
 *     address, ioconfig and i2c hardware information.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static void mtk_i2c_init_hw(struct mtk_i2c *i2c)
{
    unsigned short control_reg;

    i2c_writel(i2c, OFFSET_SOFTRESET, I2C_SOFT_RST);
    /* set ioconfig */
    if (i2c->pushpull)
        i2c_writel(i2c, OFFSET_IO_CONFIG, I2C_IO_CONFIG_PUSH_PULL);
    else
        i2c_writel(i2c, OFFSET_IO_CONFIG, I2C_IO_CONFIG_OPEN_DRAIN);
    control_reg = I2C_CONTROL_DEFAULT | I2C_CONTROL_ACKERR_DET_EN |
                  I2C_CONTROL_CLK_EXT_EN;
    i2c_writel(i2c, OFFSET_CONTROL, control_reg);
    i2c_writel(i2c, OFFSET_DELAY_LEN, I2C_DELAY_LEN);

    i2c_dma_writel(i2c, OFFSET_DMA_RST, I2C_DMA_HARD_RST);
    i2c_dma_writel(i2c, OFFSET_DMA_RST, I2C_DMA_CLR_FLAG);
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Calculate i2c speed.\n
 *     Hardware design:\n
 *     i2c_bus_freq = source_clk / (2 * sample_cnt * step_cnt)\n
 *     The calculation want to pick the highest bus frequency that\n
 *     is still less than or equal to i2c->speed_hz. The\n
 *     calculation try to get sample_cnt and step_cnt.
 * @param[in]
 *     clk_src: i2c module source clock.
 * @param[in]
 *     target_speed: i2c target speed.
 * @param[out]
 *     timing_step_cnt: i2c step_cnt value.
 * @param[out]
 *     timing_sample_cnt: i2c sample_cnt value.
 * @return
 *     0, calculate speed successfully.\n
 *     HAL_I2C_STATUS_INVALID_PARAMETER, calculate speed fail.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Target speed is too low, calculate speed fail, return\n
 *     HAL_I2C_STATUS_INVALID_PARAMETER.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_calculate_speed(unsigned int clk_src,
                                   unsigned int target_speed,
                                   unsigned int *timing_step_cnt,
                                   unsigned int *timing_sample_cnt)
{
    unsigned int step_cnt;
    unsigned int sample_cnt;
    unsigned int max_step_cnt;
    unsigned int base_sample_cnt = MAX_SAMPLE_CNT_DIV;
    unsigned int base_step_cnt;
    unsigned int opt_div;
    unsigned int best_mul;
    unsigned int cnt_mul;

    if (target_speed > MAX_FS_PLUS_SPEED)
        max_step_cnt = MAX_HS_STEP_CNT_DIV;
    else
        max_step_cnt = MAX_STEP_CNT_DIV;

    base_step_cnt = max_step_cnt;

    /* find the best combination */
    opt_div = DIV_ROUND_UP(clk_src >> 1, target_speed);
    best_mul = MAX_SAMPLE_CNT_DIV * max_step_cnt;

    /* Search for the best pair (sample_cnt, step_cnt) with
     * 0 < sample_cnt < MAX_SAMPLE_CNT_DIV
     * 0 < step_cnt < max_step_cnt
     * sample_cnt * step_cnt >= opt_div
     * optimizing for sample_cnt * step_cnt being minimal
     */
    for (sample_cnt = 1; sample_cnt <= MAX_SAMPLE_CNT_DIV; sample_cnt++) {
        step_cnt = DIV_ROUND_UP(opt_div, sample_cnt);
        cnt_mul = step_cnt * sample_cnt;
        if (step_cnt > max_step_cnt)
            continue;

        if (cnt_mul < best_mul) {
            best_mul = cnt_mul;
            base_sample_cnt = sample_cnt;
            base_step_cnt = step_cnt;
            if (best_mul == opt_div)
                break;
        }
    }

    sample_cnt = base_sample_cnt;
    step_cnt = base_step_cnt;

    if ((clk_src / (2 * sample_cnt * step_cnt)) > target_speed) {
        log_hal_info("Unsupported speed (%u KHz)\n", target_speed);
        return HAL_I2C_STATUS_INVALID_PARAMETER;
    }

    *timing_step_cnt = step_cnt - 1;
    *timing_sample_cnt = sample_cnt - 1;

    return HAL_I2C_STATUS_OK;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Calculate i2c speed and write sample_cnt, step_cnt to TIMING register.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains i2c source clock,
 *     clock divide and speed.
 * @return
 *     0, set speed successfully.\n
 *     error code from mtk_i2c_calculate_speed().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If mtk_i2c_calculate_speed() fails, return its error code.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int mtk_i2c_set_speed(struct mtk_i2c *i2c)
{
    unsigned int clk_src;
    unsigned int step_cnt;
    unsigned int sample_cnt;
    unsigned int target_speed;
    int ret;

    i2c->clk = MTK_I2C_SOURCE_CLK;
    i2c->clk_src_div = MTK_I2C_CLK_DIV;

    if (i2c->speed == 0)
        i2c->speed = I2C_DEFAULT_SPEED;

    i2c->clock_div_reg = I2C_DEFAULT_CLK_DIV;

    if (i2c->clk_src_div == 0)
        i2c->clk_src_div = MTK_I2C_CLK_DIV;

    i2c->clk_src_div *= i2c->clock_div_reg;

    clk_src = (i2c->clk) / (i2c->clk_src_div);
    target_speed = i2c->speed;

    if (target_speed > MAX_FS_PLUS_SPEED) {
        /* Set master code speed register */
        i2c->timing_reg = I2C_TIMING_400K;

        /* Set the high speed mode register */
        ret = mtk_i2c_calculate_speed(clk_src, target_speed,
                                      &step_cnt, &sample_cnt);
        if (ret < 0)
            return ret;

        i2c->high_speed_reg = I2C_TIME_DEFAULT_VALUE |
                              (sample_cnt << 12) |
                              (step_cnt << 8);
    } else {
        ret = mtk_i2c_calculate_speed(clk_src, target_speed,
                                      &step_cnt, &sample_cnt);
        if (ret < 0)
            return ret;

        i2c->timing_reg = (sample_cnt << 8) | step_cnt;

        /* Disable the high speed transaction */
        i2c->high_speed_reg = 0;
    }

    i2c_writel(i2c, OFFSET_CLOCK_DIV, (i2c->clock_div_reg - 1));
    i2c_writel(i2c, OFFSET_TIMING, i2c->timing_reg);
    i2c_writel(i2c, OFFSET_HS, i2c->high_speed_reg);

    return HAL_I2C_STATUS_OK;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Dump i2c controller registers and DMA registers value.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains register base\n
 *     address and DMA base address.
 * @return
 *     none.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
void i2c_dump_info(struct mtk_i2c *i2c)
{
    log_hal_info("I2C structure:\n");
    log_hal_info("id=%d,dma_en=%x,auto_restart=%x,poll_en=%x,op=%x\n",
                 i2c->id, i2c->dma_en, i2c->auto_restart, i2c->poll_en, i2c->op);
    log_hal_info("irq_stat=%x,source_clk=%d,clk_div=%d,speed=%d\n",
                 i2c->irq_stat, i2c->clk, i2c->clk_src_div, i2c->speed);
    log_hal_info("msg_complete=%x,addr=%x\n",
                 i2c->msg_complete, i2c->addr);
    log_hal_info("irqnr=%x,timing_reg=%x,high_speed_reg=%x\n",
                 i2c->irqnr, i2c->timing_reg, i2c->high_speed_reg);
    log_hal_info("con_num=%d,delay_len=%x,ext_time=%x,scl_ratio=%x\n",
                 i2c->con_num, i2c->delay_len, i2c->ext_time, i2c->scl_ratio);
    log_hal_info("hs_scl_ratio=%x,scl_mis_comp=%x,sta_stop_time=%x\n",
                 i2c->hs_scl_ratio, i2c->scl_mis_comp, i2c->sta_stop_time);
    log_hal_info("hs_sta_stop_time=%x,sda_time=%x\n",
                 i2c->hs_sta_stop_time, i2c->sda_time);

    log_hal_info("I2C base address 0x%x\n", i2c->base);
    log_hal_info("I2C register:\n");
    log_hal_info("SLAVE_ADDR=0x%x,INTR_MASK=0x%x,INTR_STAT=0x%x\n",
                 (i2c_readl(i2c, OFFSET_SLAVE_ADDR)),
                 (i2c_readl(i2c, OFFSET_INTR_MASK)),
                 (i2c_readl(i2c, OFFSET_INTR_STAT)));
    log_hal_info("CONTROL=0x%x,TIMING=0x%x\n",
                 (i2c_readl(i2c, OFFSET_CONTROL)),
                 (i2c_readl(i2c, OFFSET_TIMING)));
    log_hal_info("TRANSFER_LEN=0x%x,TRANSAC_LEN=0x%x,DELAY_LEN=0x%x\n",
                 (i2c_readl(i2c, OFFSET_TRANSFER_LEN)),
                 (i2c_readl(i2c, OFFSET_TRANSAC_LEN)),
                 (i2c_readl(i2c, OFFSET_DELAY_LEN)));
    log_hal_info("START=0x%x,EXT_CONF=0x%x,IO_CONFIG=0x%x\n",
                 (i2c_readl(i2c, OFFSET_START)),
                 (i2c_readl(i2c, OFFSET_EXT_CONF)),
                 (i2c_readl(i2c, OFFSET_IO_CONFIG)));
    log_hal_info("FIFO_STAT1=0x%x,FIFO_STAT=0x%x,FIFO_THRESH=0x%x\n",
                 (i2c_readl(i2c, OFFSET_FIFO_STAT1)),
                 (i2c_readl(i2c, OFFSET_FIFO_STAT)),
                 (i2c_readl(i2c, OFFSET_FIFO_THRESH)));
    log_hal_info("DEBUGSTAT=0x%x,TRANSFER_LEN_AUX=0x%x,CLOCK_DIV=0x%x,debugctrl=0x%x\n",
                 (i2c_readl(i2c, OFFSET_DEBUGSTAT)),
                 (i2c_readl(i2c, OFFSET_TRANSFER_LEN_AUX)),
                 (i2c_readl(i2c, OFFSET_CLOCK_DIV)), (i2c_readl(i2c, OFFSET_DEBUGCTRL)));
    log_hal_info("HS=0x%x,SCL_HL_RATIO=0x%x,HS_SCL_HL_RATIO=0x%x\n",
                 (i2c_readl(i2c, OFFSET_HS)),
                 (i2c_readl(i2c, OFFSET_SCL_HL_RATIO)),
                 (i2c_readl(i2c, OFFSET_HS_SCL_HL_RATIO)));
    log_hal_info("STA_STOP_AC_TIME=0x%x,HS_STA_STOP_AC_TIME=0x%x\n",
                 (i2c_readl(i2c, OFFSET_STA_STOP_AC_TIME)),
                 (i2c_readl(i2c, OFFSET_HS_STA_STOP_AC_TIME)));
    log_hal_info("SCL_MIS_COMP_POINT=0x%x,SDA_TIME=0x%x,FIFO_PAUSE=0x%x\n",
                 (i2c_readl(i2c, OFFSET_SCL_MIS_COMP_POINT)),
                 (i2c_readl(i2c, OFFSET_SDA_TIME)),
                 (i2c_readl(i2c, OFFSET_FIFO_PAUSE)));

    log_hal_info("DMA base address 0x%x\n", i2c->dmabase);
    log_hal_info("I2C DMA register:\n");
    log_hal_info("OFFSET_DMA_TX_MEM_ADDR=0x%x,OFFSET_DMA_RX_MEM_ADDR=0x%x\n",
                 (i2c_dma_readl(i2c, OFFSET_DMA_TX_MEM_ADDR)),
                 (i2c_dma_readl(i2c, OFFSET_DMA_RX_MEM_ADDR)));
    log_hal_info("OFFSET_DMA_TX_LEN=0x%x,OFFSET_DMA_RX_LEN=0x%x\n",
                 (i2c_dma_readl(i2c, OFFSET_DMA_TX_LEN)),
                 (i2c_dma_readl(i2c, OFFSET_DMA_RX_LEN)));
    log_hal_info("OFFSET_DMA_CON=0x%x,OFFSET_DMA_EN=0x%x\n",
                 (i2c_dma_readl(i2c, OFFSET_DMA_CON)),
                 (i2c_dma_readl(i2c, OFFSET_DMA_EN)));
    log_hal_info("OFFSET_DMA_INT_EN=0x%x,OFFSET_DMA_INT_FLAG=0x%x\n",
                 (i2c_dma_readl(i2c, OFFSET_DMA_INT_EN)),
                 (i2c_dma_readl(i2c, OFFSET_DMA_INT_FLAG)));
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Set gpio to i2c mode.
 * @param[in]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains bus number\n
 *     information.
 * @return
 *     0, set gpio to i2c mode successfully.\n
 *     HAL_I2C_STATUS_INVALID_PARAMETER, invalid i2c bus id.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. Invalid i2c bus number, return HAL_I2C_STATUS_INVALID_PARAMETER.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int i2c_init(struct mtk_i2c *i2c)
{
    int ret = 0;

    ret = i2c_clock_enable(i2c->id);
    if (ret) {
        log_hal_info("Failed to enable i2c clock.\n");
        return ret;
    }
    ret = mtk_i2c_init_base(i2c);
    if (ret) {
        log_hal_info("Failed to init i2c base.\n");
        return ret;
    }
    return ret;
}

int i2c_deinit(struct mtk_i2c *i2c)
{
    i2c->i2c_callback = NULL;
    i2c_clock_disable(i2c->id);
    return 0;
}
/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Configure i2c register and trigger transfer.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains register base\n
 *     address, operation mode, interrupt status and i2c driver data.
 * @param[out]
 *     msgs: i2c_msg pointer, struct i2c_msg contains slave\n
 *     address, operation mode, msg length and data buffer.
 * @param[in]
 *     num: i2c_msg number.
 * @param[in]
 *     left_num: left i2c_msg number.
 * @return
 *     0, i2c transfer successfully.\n
 *     HAL_I2C_EVENT_TIMEOUT_ERROR, i2c transfer timeout.\n
 *     EREMOTEIO_I2C, i2c receive data length does not equal to request data\n
 *     length.\n
 *     HAL_I2C_EVENT_ACK_ERROR, i2c transfer ack error.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. i2c transfer timeout, return HAL_I2C_EVENT_TIMEOUT_ERROR.\n
 *     2. i2c receive data length does not equal to request data\n
 *     length, return -EREMOTEIO_I2C.\n
 *     3. i2c transfer ack error, return HAL_I2C_EVENT_ACK_ERROR.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
static int mtk_i2c_do_transfer(struct mtk_i2c *i2c, struct i2c_msg *msgs,
                               int num, int left_num)
{
    bool tmo = false;
    bool trans_error = false;
    unsigned char *data_buf = msgs->buf;
    unsigned short data_len = msgs->len;
    unsigned short read_len;
    unsigned short addr_reg;
    unsigned short start_reg;
    unsigned short control_reg;
    unsigned short restart_flag = 0;
    unsigned int tmo_poll = I2C_POLL_VALUE;
    int ret = HAL_I2C_STATUS_OK;

    i2c->irq_stat = 0;

    if (i2c->auto_restart)
        restart_flag = I2C_RS_TRANSFER;

    control_reg = i2c_readl(i2c, OFFSET_CONTROL) &
                  ~(I2C_CONTROL_DIR_CHANGE | I2C_CONTROL_RS);

    if ((i2c->speed > MAX_FS_PLUS_SPEED) || (num > 1))
        control_reg |= I2C_CONTROL_RS;
    if (i2c->op == I2C_MASTER_WRRD)
        control_reg |= I2C_CONTROL_DIR_CHANGE | I2C_CONTROL_RS;

    if (i2c->dma_en)
        control_reg |= I2C_CONTROL_AYNCS_MODE | I2C_CONTROL_DMA_EN;
    i2c_writel(i2c, OFFSET_CONTROL, control_reg);
    /* set start condition */
    if (i2c->speed <= I2C_DEFAULT_SPEED)
        i2c_writel(i2c, OFFSET_EXT_CONF, I2C_ST_START_CON);
    else
        i2c_writel(i2c, OFFSET_EXT_CONF, I2C_FS_START_CON);

    addr_reg = msgs->addr << 1;
    if (i2c->op == I2C_MASTER_RD)
        addr_reg |= 0x1;

    i2c_writel(i2c, OFFSET_SLAVE_ADDR, addr_reg);

    /* clear interrupt status */
    i2c_writel(i2c, OFFSET_INTR_STAT, I2C_RS_TRANSFER | I2C_ACKERR |
               I2C_TRANSAC_COMP);

    i2c_writel(i2c, OFFSET_FIFO_ADDR_CLR, I2C_FIFO_ADDR_CLR);

    if (i2c->poll_en)
        i2c_writel(i2c, OFFSET_INTR_MASK, 0);
    else
        i2c_writel(i2c, OFFSET_INTR_MASK, restart_flag | I2C_ACKERR |
                   I2C_TRANSAC_COMP);

    /* set transfer and transaction len */
    if (i2c->op == I2C_MASTER_WRRD) {
        i2c_writel(i2c, OFFSET_TRANSFER_LEN, msgs->len);
        i2c_writel(i2c, OFFSET_TRANSFER_LEN_AUX, (msgs + 1)->len);
        i2c_writel(i2c, OFFSET_TRANSAC_LEN, I2C_WRRD_TRANAC_VALUE);
    } else {
        i2c_writel(i2c, OFFSET_TRANSFER_LEN, msgs->len);
        i2c_writel(i2c, OFFSET_TRANSAC_LEN, num);
    }
    if (i2c->dma_en) {
        if (i2c->op == I2C_MASTER_WR) {
            i2c_dma_writel(i2c, OFFSET_DMA_INT_FLAG,
                           I2C_DMA_INT_FLAG_NONE);
            i2c_dma_writel(i2c, OFFSET_DMA_CON,
                           I2C_DMA_CON_TX);
            i2c_dma_writel(i2c, OFFSET_DMA_TX_MEM_ADDR,
                           (unsigned int)(msgs->buf));
            i2c_dma_writel(i2c, OFFSET_DMA_TX_LEN,
                           (unsigned int)(msgs->len));
        } else if (i2c->op == I2C_MASTER_RD) {
            i2c_dma_writel(i2c, OFFSET_DMA_INT_FLAG,
                           I2C_DMA_INT_FLAG_NONE);
            i2c_dma_writel(i2c, OFFSET_DMA_CON,
                           I2C_DMA_CON_RX);
            i2c_dma_writel(i2c, OFFSET_DMA_RX_MEM_ADDR,
                           (unsigned int)(msgs->buf));
            i2c_dma_writel(i2c, OFFSET_DMA_RX_LEN,
                           (unsigned int)(msgs->len));
        } else if (i2c->op == I2C_MASTER_WRRD) {
            i2c_dma_writel(i2c, OFFSET_DMA_INT_FLAG,
                           I2C_DMA_CLR_FLAG);
            i2c_dma_writel(i2c, OFFSET_DMA_CON,
                           I2C_DMA_CLR_FLAG);
            i2c_dma_writel(i2c, OFFSET_DMA_TX_MEM_ADDR,
                           (unsigned int)(msgs->buf));
            i2c_dma_writel(i2c, OFFSET_DMA_RX_MEM_ADDR,
                           (unsigned int)((msgs + 1)->buf));
            i2c_dma_writel(i2c, OFFSET_DMA_TX_LEN,
                           (unsigned int)(msgs->len));
            i2c_dma_writel(i2c, OFFSET_DMA_RX_LEN,
                           (unsigned int)((msgs + 1)->len));
        }
        i2c_dma_writel(i2c, OFFSET_DMA_EN, I2C_DMA_START_EN);
    }  else {
        if (i2c->op != I2C_MASTER_RD) {
            data_buf = msgs->buf;
            data_len = msgs->len;
            while (data_len--)
                i2c_writel(i2c, OFFSET_DATA_PORT,
                           *(data_buf++));
        }
    }
    if (!i2c->auto_restart)
        start_reg = I2C_TRANSAC_START;
    else {
        start_reg = I2C_TRANSAC_START | I2C_RS_MUL_TRIG;
        if (left_num >= 1)
            start_reg |= I2C_RS_MUL_CNFG;
    }

    i2c_writel(i2c, OFFSET_START, start_reg);//set start reg = 1

    if (i2c->i2c_callback != NULL && i2c->dma_en)
        /* Deal result in i2c_callback */
        return HAL_I2C_STATUS_OK;  //Unlock sleep in irq handler

    if (i2c->poll_en) {
        for (;;) {
            i2c->irq_stat = i2c_readl(i2c, OFFSET_INTR_STAT);
            if (i2c->irq_stat & (I2C_TRANSAC_COMP | restart_flag)) {
                tmo = false;
                if (i2c->irq_stat & I2C_ACKERR)
                    trans_error = true;
                break;
            }
            tmo_poll--;
            if (tmo_poll == 0) {
                tmo = true;
                break;
            }
        }
    } else {
        for (;;) {
            if (i2c_g[i2c->id]->msg_complete) {
                if (i2c_g[i2c->id]->irq_stat & (I2C_TRANSAC_COMP | restart_flag)) {
                    tmo = false;
                    if (i2c_g[i2c->id]->irq_stat & I2C_ACKERR)
                        trans_error = true;
                    break;
                }
            }
            tmo_poll--;
            if (tmo_poll == 0) {
                tmo = true;
                break;
            }
        }
    }
    /* clear interrupt mask */
    i2c_writel(i2c, OFFSET_INTR_MASK, ~(restart_flag | I2C_ACKERR |
                                        I2C_TRANSAC_COMP));
    if ((!tmo) && (!trans_error)) {
        if (!i2c->dma_en && i2c->op != I2C_MASTER_WR) {
            data_buf = (i2c->op == I2C_MASTER_RD) ?
                       msgs->buf : (msgs + 1)->buf;
            data_len = (i2c->op == I2C_MASTER_RD) ?
                       msgs->len : (msgs + 1)->len;
            read_len = i2c_readl(i2c, OFFSET_FIFO_STAT1)
                       & 0x1f;

            if (read_len == data_len) {
                while (data_len--)
                    *(data_buf++) = i2c_readl(i2c, OFFSET_DATA_PORT);
            } else {
                log_hal_info("ERROR: read_len=%d, data_len=%x!\n", read_len, data_len);
                ret = HAL_I2C_STATUS_INVALID_PARAMETER;
            }
        }
    } else {
        /* timeout or ACKERR */
        if (tmo)
            ret = HAL_I2C_EVENT_TIMEOUT_ERROR;
        else
            ret = HAL_I2C_EVENT_ACK_ERROR;


        if (tmo) {
            log_hal_info(
                "id=%d, addr: %x, transfer timeout\n",
                i2c->id, msgs->addr);
            i2c_dump_info(i2c);
        } else {
            log_hal_info(
                "id=%d, addr: %x, I2C_ACKERR\n",
                i2c->id, msgs->addr);
        }
    }
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_I2C);    //unlock sleep after i2c_callback
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    return ret;
}

/** @ingroup IP_group_i2c_internal_function
 * @par Description
 *     Common i2c transfer API. Set i2c transfer mode according to i2c_msg\n
 *     information, then call mtk_i2c_do_transfer() to configure i2c register\n
 *     and trigger transfer.
 * @param[out]
 *     i2c: mtk_i2c pointer, struct mtk_i2c contains register base\n
 *     address, operation mode, interrupt status and i2c driver data.
 * @param[out]
 *     msgs: i2c_msg pointer, struct i2c_msg contains slave\n
 *     address, operation mode, msg length and data buffer.
 * @param[in]
 *     num: i2c_msg number.
 * @return
 *     i2c_msg number, i2c transfer successfully.\n
 *     HAL_I2C_STATUS_INVALID_PARAMETER, msg length is 0 or more than 16, msg data buffer is NULL,\n
 *     use DMA MODE or slave address more than 0x7f.\n
 *     error code from mtk_i2c_init_base().\n
 *     error code from mtk_i2c_set_speed().\n
 *     error code from mtk_i2c_do_transfer().
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     1. If msg length is 0 or more than 16, msg data buffer is NULL,\n
 *     use DMA MODE or slave address more than 0x7f, return HAL_I2C_STATUS_INVALID_PARAMETER.
 *     2. If mtk_i2c_init_base() fails, return its error code.\n
 *     3. If mtk_i2c_set_speed() fails, return its error code.\n
 *     4. If mtk_i2c_do_transfer() fails, return its error code.
 * @par Call graph and Caller graph
 * @par Refer to the source code
 */
int mtk_i2c_transfer(struct mtk_i2c *i2c, struct i2c_msg *msgs, int num)
{
    unsigned char num_cnt;
    int left_num = num;
    int ret;

    if (i2c->bus_busy) {
        log_hal_info("i2c-%d busy !!!\n", i2c->id);
        return HAL_I2C_STATUS_BUS_BUSY;
    }
    i2c->bus_busy = true;
#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_I2C);  //Lock sleep before setting i2c and dma registers
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */
    mtk_i2c_init_hw(i2c);
    mtk_i2c_set_speed(i2c);
    if (!i2c->poll_en) {
        i2c_g[i2c->id] = i2c;
        mtk_irq_init(i2c);
    }
    g_addr[i2c->id] = msgs->addr;
    for (num_cnt = 0; num_cnt < num; num_cnt++) {
        if (((msgs + num_cnt)->addr) > 0x7f) {
            log_hal_info("i2c addr: msgs[%d]->addr(%x) > 0x7f, error!\n",
                         num_cnt, ((msgs + num_cnt)->addr));
            ret = HAL_I2C_STATUS_INVALID_PARAMETER;
            goto err_exit;
        }

        if (!(msgs + num_cnt)->buf) {
            log_hal_info("msgs[%d]->buf is NULL.\n", num_cnt);
            ret = HAL_I2C_STATUS_INVALID_PARAMETER;
            goto err_exit;
        }

        if ((msgs + num_cnt)->len == 0) {
            log_hal_info("msgs[%d]->len == 0, error!\n", num_cnt);
            ret = HAL_I2C_STATUS_INVALID_PARAMETER;
            goto err_exit;
        }

        if ((msgs + num_cnt)->len > I2C_FIFO_SIZE)
            i2c->dma_en = true;
    }
    if ((num == 1) || ((num == 2) &&
                       (!(msgs[0].flags & I2C_M_RD) && (msgs[1].flags & I2C_M_RD) &&
                        (msgs[0].addr == msgs[1].addr))))
        i2c->auto_restart = false;
    else
        i2c->auto_restart = true;

    while (left_num--) {
        if (msgs->flags & I2C_M_RD)
            i2c->op = I2C_MASTER_RD;
        else
            i2c->op = I2C_MASTER_WR;

        if (!i2c->auto_restart) {
            if (num == 2) {
                /* combined two messages into one transaction */
                i2c->op = I2C_MASTER_WRRD;
                left_num--;
            }
        }
        ret = mtk_i2c_do_transfer(i2c, msgs, num, left_num);
        if (ret < 0)
            goto err_exit;

        msgs++;
    }

err_exit:
    i2c->bus_busy = false;
    return ret;
}
