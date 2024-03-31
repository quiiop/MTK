/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include <string.h>
#include <stdbool.h>
#include "hal.h"
#include "common.h"
#include "hal_ls_api.h"
#include "memory_attribute.h"
#include "hal_flash_cmd_macro.h"
#include "hal_nvic_internal.h"
#include "hal_flash_internal.h"
#include "hal_cache.h"
#include "memory_map.h"

#define SFLASH_PAGE_SIZE     (0x100)
#define HAL_FLASH_USE_MACRO 0
ATTR_RWDATA_IN_TCM_SEC_SHM bool NOR_FLASH_BUSY = false;
ATTR_RWDATA_IN_TCM_SEC_SHM bool NOR_FLASH_SUSPENDED = false;

#define poll_cond_tmo(condition, tmo, interval) \
    ({ \
        int __tmo_ret = ERR_TIMED_OUT; \
        int __tmo = tmo / interval; \
        while (__tmo--) { \
            if (condition) { \
                __tmo_ret = NO_ERROR; \
                break; \
            } \
            udelay(interval); \
        } \
        __tmo_ret; \
    })

/*
 * REG_SF_CMD[0:5] These bits will be auto-cleared when done,
 * so we could check these bits after written it
 */
#if HAL_FLASH_USE_MACRO
#define wr_poll_cmd_reg(cmd, tmo, interval) \
    ({ \
        int __ret = 0; \
        SF_WRITEL(REG_SF_CMD, cmd); \
        __ret = poll_cond_tmo(\
            (SF_READL(REG_SF_CMD) & \
            ((cmd) & ~CMD_TYPE_AUTOINC)) == 0, tmo, interval); \
        __ret; \
    })
#else /* #if HAL_FLASH_USE_MACRO */
ATTR_TEXT_IN_SYSRAM int wr_poll_cmd_reg(uint32_t cmd, uint32_t tmo, uint32_t interval)
{
    int ret = ERR_TIMED_OUT;

    tmo = tmo / interval;

    SF_WRITEL(REG_SF_CMD, cmd);

    while (tmo--) {
        if ((SF_READL(REG_SF_CMD) & ((cmd) & ~CMD_TYPE_AUTOINC)) == 0) {
            ret = NO_ERROR;
            break;
        }
        udelay(interval);
    }
    return ret;
}
#endif /* #if HAL_FLASH_USE_MACRO */

ATTR_RODATA_IN_SYSRAM static const struct nor_flash_info nor_flash_tbl[] = {
    /* For bring up */
    { 0xEF, 0x60, 0x18, 1, 0x1000000, 0x10000 },//"WINBOND(W25Q128JWPIQ)"
    { 0xEF, 0x60, 0x19, 1, 0x2000000, 0x10000 },//"WINBOND(W25Q256JWPIQ)"
    /* 25U64/128/256  has been tested */
    { 0xC2, 0x25, 0x36, 1, 0x400000, 0x1000  },//"MX25U32"
    { 0xEF, 0x80, 0x16, 1, 0x400000, 0x10000 },//"WINBOND(W25Q32JW)"
    { 0xC2, 0x25, 0x37, 1, 0x800000, 0x10000 },//"MXIC(25U64"
    { 0xC2, 0x20, 0x17, 1, 0x800000, 0x10000 },//"MXIC(25L64"
    { 0xEF, 0x80, 0x17, 1, 0x800000, 0x10000 },//"WINBOND(W25Q64JW)"
    { 0xEF, 0x60, 0x17, 1, 0x800000, 0x10000 },//"WINBOND(W25Q64FW)"
    { 0xC2, 0x25, 0x38, 1, 0x1000000, 0x10000},//"MXIC(25U128"
    { 0xC2, 0x20, 0x18, 1, 0x1000000, 0x10000},//"MXIC(25L128"
    { 0xEF, 0x40, 0x18, 1, 0x1000000, 0x10000},//"WINBOND(W25Q128FV)"
    { 0xC2, 0x20, 0x17, 1, 0x800000, 0x10000 },//"MXIC(MX25L64356)"
    { 0xEF, 0x40, 0x17, 1, 0x800000, 0x10000 },//"WINBOND(W25Q64JVIQ)"

    { 0xC2, 0x25, 0x39, 1, 0x2000000, 0x10000},//"MXIC(25U256"
    { 0xC2, 0x20, 0x19, 1, 0x2000000, 0x10000},//"MXIC(25L256"
    /*  W25Q64/128/256 has been tested */
    { 0xEF, 0x80, 0x19, 1, 0x2000000, 0x10000},//"WINBOND(W25Q256JW)"
    { 0x20, 0x40, 0x18, 1, 0x1000000, 0x10000},//"XMC(XM25QH128C)"
    { 0xC8, 0x40, 0x18, 1, 0x1000000, 0x10000},//"GD(GD25Q128E)"
    { 0xC8, 0x65, 0x18, 1, 0x1000000, 0x10000},//"GD(GD25WQ128E)"
    { 0x00, 0x00, 0x00, 1, 0x000000, 0x00000 },//"NULL Device"
};


ATTR_RWDATA_IN_SYSRAM struct sf_desc sflash;
ATTR_RWDATA_IN_SYSRAM static const struct nor_flash_info *nor;
ATTR_RWDATA_IN_SYSRAM  bool initialization_already = false;

/* Record the flash info */
ATTR_ZIDATA_IN_SYSRAM struct sf_info_record_table info __attribute__((aligned(64)));
ATTR_ZIDATA_IN_SYSRAM struct info_record flash_status  __attribute__((aligned(64)));

/* sf_buffer_write tmp buffer, sflash.wrbuf_size 256 bytes*/
ATTR_ZIDATA_IN_SYSRAM static uint32_t g_tmp_buf[SFLASH_PAGE_SIZE / 4];

ATTR_TEXT_IN_SYSRAM enum sf_read_mode sf_get_read_mode(void)
{
    return sflash.read_mode;
}

ATTR_TEXT_IN_SYSRAM const struct nor_flash_info *sf_get_flash(uint8_t manuf, uint8_t type, uint8_t density)
{
    int i = 0;

    while (nor_flash_tbl[i].manufact != 0) {
        if (nor_flash_tbl[i].manufact == manuf && nor_flash_tbl[i].mem_type == type &&
            nor_flash_tbl[i].mem_density == density)
            return &nor_flash_tbl[i];

        i++;
    }

    return NULL;
}

/*
 * since the read mode is saved in REG_SF_PRGDATA4, therefore, if the txlen is
 * greater than 0, it will overwrite the original value, In this case, we need
 * to save and restore the value of REG_SF_PRGDATA4.
 */
ATTR_TEXT_IN_SYSRAM int sf_prgm_cmd(uint8_t op,  uint8_t *tx, int txlen, uint8_t *rx, int rxlen)
{
#define SF_START_IDX    5
#define REG_SF_PRGDATA(n)   (REG_SF_PRGDATA0 + 4 * (n))
#define REG_SF_SHREG(n)     (REG_SF_SHREG0 + 4 * (n))
#define REG_SF_PRGDATAN(n)  (REG_SF_PRGDATAN1 + 4 * (n))

    int len = 1 + txlen + rxlen;
    int i, ret, idx;

    SF_WRITEL(REG_SF_CNT, len * 8);

    /* start at PRGDATA5, go down to PRGDATA0 */
    idx = SF_START_IDX;
    SF_WRITEB(REG_SF_PRGDATA(idx), op);
    idx--;

    /* program TX data */
    for (i = 0; i < txlen; i++, idx--) {
        SF_WRITEB(REG_SF_PRGDATA(idx), tx[i]);
        if (idx == 0)
            break;
    }

    for (idx = 0, ++i; i < txlen; i++, ++idx) {
        SF_WRITEB(REG_SF_PRGDATAN(idx), tx[i]);
    }

    ret = wr_poll_cmd_reg(CMD_TYPE_PRG, CMD_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    if (rxlen) {
        /* restart at first RX byte */
        idx = rxlen - 1;

        /* read out RX data */
        for (i = 0; i < rxlen; i++, idx--)
            rx[i] = SF_READB(REG_SF_SHREG(idx));
    }
    return 0;
}



ATTR_TEXT_IN_SYSRAM int sf_write_enable(void)
{
    return sf_prgm_cmd(NOR_OP_WREN, NULL, 0, NULL, 0);
}


ATTR_TEXT_IN_SYSRAM int sf_read_sr(uint8_t *status)
{
    int ret;

    ret = wr_poll_cmd_reg(CMD_TYPE_RDSR, CMD_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    *status = SF_READB(REG_SF_RDSR);

    return ret;
}


ATTR_TEXT_IN_SYSRAM int sf_wait_sr_ready(uint32_t tmo, uint32_t interval)
{
    int ret = 0;
    uint8_t status = SR_WIP;
    uint32_t ltmo = tmo / interval;
    uint32_t mask = save_and_set_interrupt_mask();

    while (ltmo--) {
        ret = sf_read_sr(&status);
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        }

        if (!(status & SR_WIP))
            break;

        udelay(interval);
    }

    restore_interrupt_mask(mask);
    return (status & SR_WIP) ? ERR_BUSY : NO_ERROR;
}


ATTR_TEXT_IN_SYSRAM int sf_read_cr(uint8_t *config)
{
    int ret;

    ret = sf_prgm_cmd(NOR_OP_RDCR, NULL, 0, config, 1);
    if (ret)
        return ret;

    *config = SF_READB(REG_SF_SHREG0);

    return ret;
}


ATTR_TEXT_IN_SYSRAM  int sf_write_sr(uint8_t status)
{
    int ret;
    uint8_t tmp_value = 0;

    ret = sf_write_enable();
    if (ret)
        return ret;

    /* Wait here to ensure write reg or program is done */
    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    if (sf_get_read_mode() == QUADIO_READ)
        tmp_value = SF_READL(REG_SF_PRGDATA4);

    ret = sf_prgm_cmd(NOR_OP_WRSR, &status, 1, NULL, 0);
    if (ret)
        return ret;

    if (sf_get_read_mode() == QUADIO_READ)
        SF_WRITEL(REG_SF_PRGDATA4, tmp_value);

    /* Wait here to ensure register is written to device */
    return sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
}


ATTR_TEXT_IN_SYSRAM int sf_write_cr(uint8_t config)
{
    int ret;
    uint8_t val[2];
    uint8_t tmp_value = 0;

    ret = sf_write_enable();
    if (ret)
        return ret;

    /* Wait here to ensure write reg or program is done */
    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    ret = sf_read_sr(&val[0]);
    if (ret)
        return ret;

    if (sf_get_read_mode() == QUADIO_READ)
        tmp_value = SF_READL(REG_SF_PRGDATA4);

    val[1] = config;
    ret = sf_prgm_cmd(NOR_OP_WRSR, val, 2, NULL, 0);
    if (ret)
        return ret;

    if (sf_get_read_mode() == QUADIO_READ)
        SF_WRITEL(REG_SF_PRGDATA4, tmp_value);

    /* Wait here to ensure register is written to device */
    return sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
}


ATTR_TEXT_IN_SYSRAM  int sf_read_id(uint8_t *manufact, uint8_t *mem_type, uint8_t *mem_density)
{
    int ret;
    uint8_t id[3];

    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    ret = sf_prgm_cmd(NOR_OP_RDID, NULL, 0, id, 3);
    if (ret) {
        return ret;
    }

    *mem_density = (uint8_t)SF_READL(REG_SF_SHREG0);
    *mem_type = (uint8_t)SF_READL(REG_SF_SHREG1);
    *manufact = (uint8_t)SF_READL(REG_SF_SHREG2);

    flash_status.manu_id = *manufact;
    if (flash_status.manu_id == NOR_MF_XMC) { //Micron && XMC MID all is 0x20,now remove Micron

        flash_status.manu_id = NOR_MF_WINBOND;
    }

    return ret;
}

ATTR_TEXT_IN_SYSRAM  int sf_set_4byte(int enable)
{
    int ret;
    uint32_t val;

    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    ret = sf_prgm_cmd(enable ? NOR_OP_EN4B : NOR_OP_EX4B, NULL, 0, NULL, 0);
    if (ret)
        return ret;

    val = SF_READL(REG_SF_DUAL);
    if (enable)
        val |= SF_LARGE_ADDR_EN;
    else
        val &= ~SF_LARGE_ADDR_EN;

    SF_WRITEL(REG_SF_DUAL, val);

    sflash.addr_width = SF_ADDR_WIDTH_4B;

    return ret;
}


ATTR_TEXT_IN_SYSRAM int set_mxic_quad(bool enable)
{
    int ret;
    uint8_t status;

    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    ret =  sf_read_sr(&status);
    if (ret)
        return ret;

    if (enable)
        status |= SR_QUAD_EN_MX;
    else
        status &= ~SR_QUAD_EN_MX;

    ret = sf_write_sr(status);

    return ret;
}


ATTR_TEXT_IN_SYSRAM int set_spansion_quad(bool enable)
{
    int ret;
    uint8_t config, newcr;

    /* Wait write reg or program done to make sure we read the up-to-date register */
    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    ret = sf_read_cr(&config);
    if (ret)
        return ret;

    if (enable)
        config |= CR_QUAD_EN_SPAN;
    else
        config &= ~CR_QUAD_EN_SPAN;

    ret = sf_write_cr(config);
    if (ret)
        return ret;

    ret = sf_read_cr(&newcr);
    if (ret || config != newcr)
        return ERR_BAD_STATE;

    return ret;
}


ATTR_TEXT_IN_SYSRAM int set_nor_quad_mode(bool enable)
{
    switch (nor->manufact) {
        case NOR_MF_MACRONIX:
        case NOR_MF_ISSI:
            return set_mxic_quad(enable);

        case NOR_MF_WINBOND:
        case NOR_MF_XMC:
        case NOR_MF_SPANSION:
        case NOR_MF_SST:
        case NOR_MF_GIGADEVICE:
            return set_spansion_quad(enable);

        //case NOR_MF_MICRON:
        case NOR_MF_ESMT:
            return NO_ERROR;

        default:
            return ERR_NOT_VALID;
    }
}

ATTR_TEXT_IN_SYSRAM int sfc_write_spansion_register(uint8_t dummy_cycle)
{
    int ret = 0;
    u32 address = (nor->mem_type == SPANSION_FL_L_TYPE_ID ?
                   SPANSION_CR3V_ADDR : SPANSION_CR2V_ADDR);
    uint8_t addr[5] = {HHB(address), HLB(address), LHB(address), LLB(address), dummy_cycle};
    uint8_t tmp_value = 0;

    ret = sf_write_enable();
    if (ret != 0)
        return ret;

    if (sf_get_read_mode() == QUADIO_READ)
        tmp_value = SF_READL(REG_SF_PRGDATA4);

    if (sflash.addr_width == SF_ADDR_WIDTH_4B)
        ret = sf_prgm_cmd(SPANSION_OP_WRAR, addr, 5, NULL, 0);
    else
        ret = sf_prgm_cmd(SPANSION_OP_WRAR, addr + 1, 4, NULL, 0);

    if (sf_get_read_mode() == QUADIO_READ)
        SF_WRITEL(REG_SF_PRGDATA4, tmp_value);

    return ret;
}


ATTR_TEXT_IN_SYSRAM int sfc_set_dummy_cycle(void)
{
    uint8_t dummy_cycle  = 10;
    int ret = 0;

    switch (nor->manufact) {
        case NOR_MF_SPANSION:
            dummy_cycle = dummy_cycle + SPANSION_LATENCY_CODE ;
            ret =  sfc_write_spansion_register(dummy_cycle - SPANSION_MODE_CYCLE);
            if (ret == 0)
                SF_WRITEL(REG_SF_DUMMY, dummy_cycle | DUMMY_CFG_EN);

            break;

        //case NOR_MF_MICRON:
        /* 7933  brom dummy_cycles set to 10 */
        //    SF_WRITEL(REG_SF_DUMMY, dummy_cycle  | DUMMY_CFG_EN);
        //    break;

        default:
            break;
    }

    return ret;
}



ATTR_TEXT_IN_SYSRAM int sf_set_read_mode(enum sf_read_mode mode, bool skip_nor_qe)
{
    int ret = 0;
    uint8_t val, dummy_cycle = 0;

    switch (mode) {
        case SINGLE_READ:
            if (!skip_nor_qe) {
                set_nor_quad_mode(false);
            }
            SF_WRITEL(REG_SF_DUAL, SF_READB(REG_SF_DUAL) & 0xF0);
            SF_WRITEL(REG_SF_DUMMY, 0);
            flash_status.io_mode  = 0;
            break;

        case QUADIO_READ:
            if (!skip_nor_qe) {
                ret = set_nor_quad_mode(true);
                if (ret)
                    return ret;
            }

            SF_WRITEL(REG_SF_PRGDATA4, NOR_OP_READ_4IO);
            val = SF_READ_QUAD_EN | SF_READ_ADDR_QUAD | (SF_READB(REG_SF_DUAL) & 0xF0);
            SF_WRITEL(REG_SF_DUAL, val);
            //if (nor->manufact == NOR_MF_SPANSION || nor->manufact == NOR_MF_MICRON)
            if (nor->manufact == NOR_MF_SPANSION)
                sfc_set_dummy_cycle();

            flash_status.io_mode = 1;
            break;
        default:
            SF_WRITEL(REG_SF_DUMMY, dummy_cycle);
            return ERR_NOT_VALID;
    }

    sflash.read_mode = mode;

    return ret;
}


ATTR_TEXT_IN_SYSRAM int sf_do_dma(uint32_t src, uint32_t dest, uint32_t len)
{
    uint32_t cnt = 0;

    writel(BIT(0), CQ_DMA_G_DMA_0_SEC_EN);
    writel(len, CQ_DMA_G_DMA_0_LEN1);
    writel(src, CQ_DMA_G_SRC_ADDR);
    writel(dest, CQ_DMA_G_DEST_ADDR);
    writel(1, CQ_DMA_G_EN);

    while (true) {
        if (readl(CQ_DMA_G_EN)  == 0)
            break;

        if (cnt ++ > ST_BUSY_TIMEOUT) {
            return ERR_TIMED_OUT;
        }

        udelay(100);
    }

    return 0;

}
#if HAL_FLASH_USE_MACRO
#define sf_set_rw_addr(addr) \
    do { \
        SF_WRITEL(REG_SF_RADR3, HHB(addr)); \
        SF_WRITEL(REG_SF_RADR2, HLB(addr)); \
        SF_WRITEL(REG_SF_RADR1, LHB(addr)); \
        SF_WRITEL(REG_SF_RADR0, LLB(addr)); \
    } while (0)
#else /* #if HAL_FLASH_USE_MACRO */
ATTR_TEXT_IN_SYSRAM void sf_set_rw_addr(uint32_t addr)
{
    SF_WRITEL(REG_SF_RADR3, HHB(addr));
    SF_WRITEL(REG_SF_RADR2, HLB(addr));
    SF_WRITEL(REG_SF_RADR1, LHB(addr));
    SF_WRITEL(REG_SF_RADR0, LLB(addr));
}
#endif /* #if HAL_FLASH_USE_MACRO */

ATTR_TEXT_IN_SYSRAM int sf_read(uint32_t addr, uint8_t *buf, uint32_t len, enum sf_read_path path)
{
    int ret = 0;
    uint32_t i;

    switch (path) {
        case PIO_PATH:
            sf_set_rw_addr(addr);

            for (i = 0; i < len; i++) {
                ret = wr_poll_cmd_reg(CMD_TYPE_AUTOINC | CMD_TYPE_RD,
                                      CMD_TIMEOUT,
                                      CHECK_INTERVAL_10_US);
                if (ret)
                    return ret;

                buf[i] = (uint8_t)SF_READL(REG_SF_RDATA);
            }

            return ret;

        case MEMCOPY_PATH:
            memcpy(buf, (uint8_t *)(sflash.map_base + addr), len);
            return ret;

        case DMA_PATH:
            /* must use 0x90000000, for 0x90000000 is the address of  flash seen from CQ_DMA */
            return sf_do_dma(addr, (uint32_t)buf, len);

        default:
            return ERR_NOT_VALID;
    }
}

ATTR_TEXT_IN_SYSRAM int sf_erase_sector_single(uint32_t addr, uint32_t cmd)
{
    int ret;
    uint8_t tmp_value = 0;
    uint8_t address[4] = {HHB(addr), HLB(addr), LHB(addr), LLB(addr)};

    if (sf_get_read_mode() == QUADIO_READ)
        tmp_value = SF_READL(REG_SF_PRGDATA4);

    ret = sf_write_enable();
    if (ret)
        return ret;

    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    if (sf_get_read_mode() == QUADIO_READ)
        tmp_value = SF_READL(REG_SF_PRGDATA4);

    if (sflash.addr_width == SF_ADDR_WIDTH_4B)
        ret = sf_prgm_cmd(cmd, address, 4, NULL, 0);
    else
        ret = sf_prgm_cmd(cmd, address + 1, 3, NULL, 0);

    if (sf_get_read_mode() == QUADIO_READ)
        SF_WRITEL(REG_SF_PRGDATA4, tmp_value);

    if (ret)
        return ret;

    if (sf_get_read_mode() == QUADIO_READ)
        SF_WRITEL(REG_SF_PRGDATA4, tmp_value);

    return ret;
}


ATTR_TEXT_IN_SYSRAM int sf_erase_chip(void)
{
    int ret;

    ret = sf_write_enable();
    if (ret)
        return ret;

    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    ret = wr_poll_cmd_reg(CMD_TYPE_ERASE, ERASE_TIMEOUT,
                          CHECK_INTERVAL_100_US);
    if (ret)
        return ret;

    /* erase should be a synchronized operation */
    return sf_wait_sr_ready(ERASE_TIMEOUT, CHECK_INTERVAL_100_US);
}


ATTR_TEXT_IN_SYSRAM int sf_erase(uint32_t addr, uint32_t len, enum sf_erase_mode mode)
{
#define ALIGNED_SECTOR(x)   (!((x) & (nor->sect_size - 1)))
    int ret = 0;
    uint32_t loop, step, cmd, mask, xAddr;

    if ((addr + len) > nor->chip_size)
        return ERR_INVALID_ARGS;

    /* translate physical address to cacheable address */
    xAddr = HAL_CACHE_PHYSICAL_TO_VIRTUAL(sflash.map_base + addr);

    switch (mode) {
        case ERASE_SEC:
            if (ALIGNED_SECTOR(addr) && ALIGNED_SECTOR(len)) {
                step = nor->sect_size;
                cmd = NOR_OP_SE;
                break;
            } else if (ALIGNED_4K(addr) && ALIGNED_4K(len)) {
                step = nor->sect_size;
                cmd = NOR_OP_SE;
                addr = (addr >> 0x10) << 0x10;
                len = (len + nor->sect_size - 1) & nor->sect_size;
                break;
            }
            return ERR_INVALID_ARGS;

        case ERASE_32K_BLK:
            if (ALIGNED_32K(addr) && ALIGNED_32K(len)) {
                step = SF_SIZE_32K;
                cmd = NOR_OP_BE_32K;
                break;
            }
            return ERR_INVALID_ARGS;

        case ERASE_4K_BLK:
            if (ALIGNED_4K(addr) && ALIGNED_4K(len)) {
                step = SF_SIZE_4K;
                cmd = NOR_OP_BE_4K;
                break;
            }
            return ERR_INVALID_ARGS;

        default:
            return ERR_NOT_VALID;
    }

    loop = len / step;
    while (loop) {
    retry:
        do {
            ret = sf_CheckDeviceReady();
        } while (ERR_BUSY == ret);

        mask = save_and_set_interrupt_mask();

        if (NOR_FLASH_SUSPENDED) {
            restore_interrupt_mask(mask);
            goto retry;
        }

        ret = sf_erase_sector_single(addr, cmd);
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        } else {
            NOR_FLASH_BUSY = true;
        }

        restore_interrupt_mask(mask);

        do {
            ret = sf_CheckDeviceReady();
        } while (ERR_BUSY == ret);

        if (ret)
            return ret;

        hal_cache_invalidate_multiple_cache_lines(xAddr, step);
        xAddr += step;
        addr += step;
        loop--;
    }

    return ret;
}


ATTR_TEXT_IN_SYSRAM int sf_do_write(void)
{
    int ret;

    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    ret = sf_write_enable();
    if (ret)
        return ret;

    ret = wr_poll_cmd_reg(CMD_TYPE_WR | CMD_TYPE_AUTOINC,
                          CMD_TIMEOUT, CHECK_INTERVAL_10_US);
    return ret;
}


ATTR_TEXT_IN_SYSRAM  int sf_byte_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    int ret = 0;
    uint32_t i;
    uint32_t xAddr = HAL_CACHE_PHYSICAL_TO_VIRTUAL(sflash.map_base + addr);
    uint32_t mask = save_and_set_interrupt_mask();

    sf_set_rw_addr(addr);

    for (i = 0; i < len; i++, addr++, buf++) {
        SF_WRITEL(REG_SF_WDATA, *buf);
        ret = sf_do_write();
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        }

        ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        }
    }
    restore_interrupt_mask(mask);
    hal_cache_invalidate_multiple_cache_lines(xAddr, len);

    return ret;
}


ATTR_TEXT_IN_SYSRAM  int sf_buffer_write(uint32_t addr, const uint32_t *buf, uint32_t len)
{
#define U8_TO_U32_SHIFT 2
#define SF_REG_FL_STATUS1 0x00000074
    uint32_t i, loop, reg;
    uint32_t cnt = sflash.wrbuf_size >> U8_TO_U32_SHIFT;
    uint32_t *tmp_buf;
    uint32_t mask, xAddr;
    int ret = 0;

    if (len == 0)
        return 0;

    xAddr = HAL_CACHE_PHYSICAL_TO_VIRTUAL(sflash.map_base + addr);
    loop = len / sflash.wrbuf_size;

    while (loop) {
        tmp_buf = g_tmp_buf;
        memcpy((uint8_t *)tmp_buf, buf, sflash.wrbuf_size);

    retry:
        do {
            ret = sf_CheckDeviceReady();
        } while (ERR_BUSY == ret);

        mask = save_and_set_interrupt_mask();

        if (NOR_FLASH_SUSPENDED) {
            restore_interrupt_mask(mask);
            goto retry;
        }

        reg = SF_READL(REG_SF_CFG1) | 0x40;
        SF_WRITEL(REG_SF_CFG1, reg);

        ret = poll_cond_tmo((SF_READL(SF_REG_FL_STATUS1) & 0xF) == 0x7, ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        }

        host_en_buf_wr();
        sf_set_rw_addr(addr);

        for (i = 0; i < cnt; i++)
            SF_WRITEL(REG_SF_PP_DW_DATA, *tmp_buf++);

        ret = sf_do_write();
        if (ret) {
            host_dis_buf_wr();
            restore_interrupt_mask(mask);
            return ret;
        } else {
            NOR_FLASH_BUSY = true;
        }

        do {
            ret = sf_CheckDeviceReady();
        } while (ERR_BUSY == ret);

        hal_cache_invalidate_multiple_cache_lines(xAddr, sflash.wrbuf_size);
        xAddr += sflash.wrbuf_size;
        buf += (sflash.wrbuf_size >> U8_TO_U32_SHIFT);
        addr += sflash.wrbuf_size;
        loop--;

        host_dis_buf_wr();

        restore_interrupt_mask(mask);
    }

    return ret;
}


ATTR_TEXT_IN_SYSRAM int sf_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
#define SFLASH_PAGE_SIZE    (0x100)
    int ret = 0;

    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    if ((addr + len) > nor->chip_size)
        return ERR_INVALID_ARGS;

    /* Indicate whether address aligned to SFLASH_PAGE_SIZE(256) */
    uint32_t byte_write_len = ((addr % SFLASH_PAGE_SIZE == 0) ?
                               (0) : (SFLASH_PAGE_SIZE - (addr % SFLASH_PAGE_SIZE)));

    uint32_t page_write_len = 0;

    /* data len may be less than byte_write_len */
    if (byte_write_len > len)
        byte_write_len = len;

    /* If address not aligned to SFLASH_PAGE_SIZE, use sf_byte_write to
      * make address aligned to SFLASH_PAGE_SIZE
      */
    if (byte_write_len > 0) {
        ret = sf_byte_write(addr, buf, byte_write_len);
        if (ret != 0) {
            return ret;
        }

        addr += byte_write_len;
        buf += byte_write_len;
        len -= byte_write_len;

        if (len < SFLASH_PAGE_SIZE) {
            return sf_byte_write(addr, buf, len);
        }
    }

    /* When code execute to here, address would be aligned to SFLASH_DEVICE_PAGE_SIZE */
    byte_write_len = len % SFLASH_PAGE_SIZE;
    page_write_len = len - byte_write_len;

    ret = sf_buffer_write(addr, (const uint32_t *)buf, page_write_len);
    if (ret < 0)
        return ret;

    addr += page_write_len;
    buf += page_write_len;

    ret = sf_byte_write(addr, buf, byte_write_len);
    if (ret < 0)
        return ret;

    return ret;
}



ATTR_TEXT_IN_SYSRAM void flash_release_deep_pd(void)
{
    sf_prgm_cmd(NOR_OP_RDP, NULL, 0, NULL, 0);
    udelay(40);     /* wait device tRES time */
}


ATTR_TEXT_IN_SYSRAM void flash_enter_deep_pd(void)
{
    sf_prgm_cmd(NOR_OP_ENTER_DP, NULL, 0, NULL, 0);
    udelay(20);     /* wait device tDP time */
}


ATTR_TEXT_IN_SYSRAM void flash_resume(void)
{
    uint8_t val;

    host_wp_off();

    if (sflash.read_mode == QUADIO_READ) {
        SF_WRITEL(REG_SF_PRGDATA4, NOR_OP_READ_4IO);
        val = SF_READ_QUAD_EN | SF_READ_ADDR_QUAD | (SF_READB(REG_SF_DUAL) & 0xF0);
        SF_WRITEL(REG_SF_DUAL, val);

        if (nor->manufact == NOR_MF_SPANSION)
            SF_WRITEL(REG_SF_DUMMY, 0x0F | DUMMY_CFG_EN);
    }

    if (sflash.addr_width == SF_ADDR_WIDTH_4B) {
        val = SF_READL(REG_SF_DUAL);
        SF_WRITEL(REG_SF_DUAL, val | SF_LARGE_ADDR_EN);
    }

}


ATTR_TEXT_IN_SYSRAM void sf_reset(void)
{
    sf_prgm_cmd(NOR_OP_RSTEN, NULL, 0, NULL, 0);
    sf_prgm_cmd(NOR_OP_RST, NULL, 0, NULL, 0);
    udelay(100);        /* wait device tRST time */
}


ATTR_TEXT_IN_SYSRAM void io_init(void)
{
#define FLASH_CLK_CTL   0x3002026C

    flash_status.io_delay = 0;

    /* record frequency */
    uint32_t clk = DRV_Reg32(FLASH_CLK_CTL);
    uint32_t mask = BIT(4) | BIT(5) | BIT(6) | BIT(7);

    if (!(clk & BIT(0))) {
        flash_status.frequency = FREQUENCY_26M;
        return;
    }

    switch ((clk & mask) >> 4) {
        case 9:
            flash_status.frequency = FREQUENCY_60M;
            break;

        case 12:
            flash_status.frequency = FREQUENCY_46M;
            break;

        case 15:
            flash_status.frequency = FREQUENCY_37M;
            break;
        default:
            flash_status.frequency = 0;
            break;
    }
}

ATTR_TEXT_IN_SYSRAM int sf_read_scur(uint8_t *config)
{
    int ret;

    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
    if (ret)
        return ret;

    if (flash_status.manu_id == NOR_MF_WINBOND)
        ret = sf_prgm_cmd(NOR_OP_RDCR, NULL, 0, config, 1);
    else
        ret = sf_prgm_cmd(NOR_OP_RDSCUR_MXIC, NULL, 0, config, 1);

    if (ret)
        return ret;

    *config = SF_READB(REG_SF_SHREG0);

    return ret;
}

ATTR_TEXT_IN_SYSRAM bool sf_is_suspend(void)
{
    bool is_suspended;
    uint8_t config;

    sf_read_scur(&config);

    if (flash_status.manu_id == NOR_MF_WINBOND) {
        is_suspended = !!(config & CR_SUSPEND_STATUS);
    } else {
        is_suspended = !!(config & (CR_PROGRAM_SUSPEND | CR_ERASE_SUSPEND));
    }

    return is_suspended;
}

/* If an interrupt comes during program/erase, send suspend command

 * to nor device, after device enter to suspend status, then return
 * to IRQ handle and execute a request.
 */
ATTR_TEXT_IN_SYSRAM int sf_send_suspend(void)
{
    int ret = 0;

    if (NOR_FLASH_SUSPENDED)
        return ret;

    if (flash_status.manu_id == NOR_MF_WINBOND) {
        ret = sf_prgm_cmd(NOR_OP_SUSPEND_WINBOND, NULL, 0, NULL, 0);
    } else {
        ret = sf_prgm_cmd(NOR_OP_SUSPEND_MXIC, NULL, 0, NULL, 0);
    }

    udelay(25); /* program/erase suspend latency */
    NOR_FLASH_SUSPENDED = true;

    return ret;
}

/* If IRQ handle complete, The system needs to be called up to
 * perform an incomplete erase or write operation.
 */
ATTR_TEXT_IN_SYSRAM int sf_send_resume(void)
{
    int ret = 0;

    /* if the device isn't in suspeded. return directly */
    if (!NOR_FLASH_SUSPENDED)
        return ret;

    if (flash_status.manu_id == NOR_MF_WINBOND) {
        ret = sf_prgm_cmd(NOR_OP_RESUME_WINBOND, NULL, 0, NULL, 0);
    } else {
        ret = sf_prgm_cmd(NOR_OP_RESUME_MXIC, NULL, 0, NULL, 0);
    }

    NOR_FLASH_SUSPENDED = false;

    return ret;
}

/* this function just used for erase / page program function */
ATTR_TEXT_IN_SYSRAM int sf_CheckDeviceReady(void)
{
    int ret;
    uint8_t status;
    bool status_busy, status_suspend;
    uint32_t mask = save_and_set_interrupt_mask();

    /* read status */
    sf_read_sr(&status);
    status_busy = !!(status & SR_WIP);

    if (!status_busy) {
        /* when flash is in the free state, check whether it is in suspended */
        status_suspend = sf_is_suspend();
        if (status_suspend && NOR_FLASH_SUSPENDED) {
            /* call flash resume cmd and set NOR_FLASH_BUSY to true */
            sf_send_resume();
            NOR_FLASH_BUSY = true;
            ret = ERR_BUSY;
        } else {
            /* flash is neither busy nor suspended */
            NOR_FLASH_BUSY = false;
            NOR_FLASH_SUSPENDED = false;
            ret = NO_ERROR;
        }
    } else {
        ret = ERR_BUSY;
    }
    restore_interrupt_mask(mask);

    return ret;
}

ATTR_TEXT_IN_SYSRAM int sf_nor_init(struct sf_desc *desc)
{
    int ret = 0;
    uint8_t manufact, mem_type, mem_density;

    if (!desc->reg_base ||  !desc->map_base)
        return ERR_INVALID_ARGS;

    memcpy(&sflash, desc, sizeof(struct sf_desc));

    io_init();
    host_wp_off();
    sf_reset();

    ret = sf_read_id(&manufact, &mem_type, &mem_density);
    if (ret)
        return ret;

    nor = sf_get_flash(manufact, mem_type, mem_density);
    if (!nor) {
        return ERR_NOT_SUPPORTED;
    }

    if (nor->chip_size > SF_SIZE_16M) {
        ret = sf_set_4byte(true);
        if (ret)
            return ret;

        flash_status.address_mode = 1;
    } else {
        sflash.addr_width = SF_ADDR_WIDTH_3B;
        flash_status.address_mode = 0;
    }

    ret = sf_set_read_mode(QUADIO_READ, false);
    if (ret) {
        return ret;
    }

    info.infos = &flash_status;
    info.count = 1;
    hal_ls_flash_init_param_addr_set(&info);

#if 0

    if ((nor->manufact == NOR_MF_SPANSION) && (nor->mem_type != SPANSION_FL_L_TYPE_ID)) {
        ret = sf_write_enable();
        if (ret)
            return ret;

        if (sflash.addr_width == SF_ADDR_WIDTH_4B)
            ret = sf_prgm_cmd(6, SPANSION_OP_WRAR,
                              HHB(SPANSION_CR3NV_ADDR),
                              HLB(SPANSION_CR3NV_ADDR),
                              LHB(SPANSION_CR3NV_ADDR),
                              LLB(SPANSION_CR3NV_ADDR),
                              SPANSION_4KB_ERASE_DIS);
        else
            ret = sf_prgm_cmd(5, SPANSION_OP_WRAR,
                              HLB(SPANSION_CR3NV_ADDR),
                              LHB(SPANSION_CR3NV_ADDR),
                              LLB(SPANSION_CR3NV_ADDR),
                              SPANSION_4KB_ERASE_DIS);
    }
#endif /* #if 0 */

    return ret;
}



ATTR_TEXT_IN_SYSRAM hal_flash_status_t hal_flash_deinit(void)
{
    return HAL_FLASH_STATUS_OK;
}


ATTR_TEXT_IN_SYSRAM hal_flash_status_t hal_flash_init(void)
{
    int ret;
    uint32_t mask;
    struct sf_desc desc;

    if (initialization_already == true)
        return HAL_FLASH_STATUS_OK;

    info.count = 1;
    desc.reg_base = (unsigned long)0x34403000;
    desc.map_base = (unsigned long)0x90000000;
    desc.wrbuf_size = 256;

    mask = save_and_set_interrupt_mask();
    ret = sf_nor_init(&desc);
    if (ret != 0)  {
        restore_interrupt_mask(mask);
        return HAL_FLASH_STATUS_ERROR_NO_INIT;
    }

    restore_interrupt_mask(mask);
    initialization_already = true;

    return HAL_FLASH_STATUS_OK;
}


ATTR_TEXT_IN_SYSRAM hal_flash_status_t hal_flash_erase(uint32_t start_address,  hal_flash_block_t block_type)
{
    int ret;
    uint32_t length = 0;
    enum sf_erase_mode  e_mode = ERASE_4K_BLK;

    if (block_type == HAL_FLASH_CHIP_ERASE)
        return sf_erase_chip();

    switch (block_type) {
        case HAL_FLASH_BLOCK_4K:
            e_mode = ERASE_4K_BLK;
            length = 0x1000;
            break;

        case HAL_FLASH_BLOCK_32K:
            e_mode = ERASE_32K_BLK;
            length = 0x8000;
            break;

        default:
        case HAL_FLASH_BLOCK_64K:
            e_mode = ERASE_SEC;
            length = 0x10000;
            break;
    }

    if (nor->manufact == NOR_MF_SPANSION)
        e_mode = ERASE_SEC;

    ret = sf_erase(start_address,  length, e_mode);

    return (ret == 0 ? HAL_FLASH_STATUS_OK : HAL_FLASH_STATUS_ERROR_ERASE_FAIL);
}

ATTR_TEXT_IN_SYSRAM hal_flash_status_t hal_flash_read(uint32_t start_address, uint8_t *buffer, uint32_t length)
{
    int ret;

    ret = sf_read(start_address, buffer, length, MEMCOPY_PATH);

    return (ret == 0 ? HAL_FLASH_STATUS_OK : HAL_FLASH_STATUS_ERROR);
}

ATTR_TEXT_IN_SYSRAM hal_flash_status_t hal_flash_write(uint32_t address, const uint8_t *data, uint32_t length)
{
    int ret;

    ret = sf_write(address, data, length);

    return (ret == 0 ? HAL_FLASH_STATUS_OK : HAL_FLASH_STATUS_ERROR_PROG_FAIL);
}


ATTR_TEXT_IN_SYSRAM void hal_flash_set_base_address(uint32_t base_address)
{
    sflash.map_base = base_address;
}

/* For Winbond flash device */
ATTR_TEXT_IN_SYSRAM hal_flash_status_t hal_flash_lock_down(hal_flash_lock_size_t size)
{
    uint8_t sr = 0x1;
    uint8_t TB_bit = 0x5;
    uint32_t mask;
    int ret = 0;

    switch (size) {
        case HAL_FLASH_LOCK_SIZE_256K:
            sr = 0x1;
            break;

        case HAL_FLASH_LOCK_SIZE_512K:
            sr = 0x2;
            break;

        case HAL_FLASH_LOCK_SIZE_1M:
            sr = 0x3;
            break;

        case HAL_FLASH_LOCK_SIZE_2M:
            sr = 0x4;
            break;

        case HAL_FLASH_LOCK_SIZE_4M:
            sr = 0x5;
            break;

        case HAL_FLASH_LOCK_SIZE_ALL:
            sr = 0xF;
    }

    switch (nor->chip_size) {
        case 0x800000:
            sr += 1;
            break;

        case 0x400000:
            sr += 2;
            break;

        case 0x1000000:
            break;

        case 0x2000000:
        case 0x4000000:
            sr += 2;
            TB_bit = 6;
            break;
    }

    sr <<= 0x2;
    sr |= BIT(TB_bit);

    mask = save_and_set_interrupt_mask();
    ret = sf_write_sr(sr);

    restore_interrupt_mask(mask);

    return ret;
}

/* For Winbond flash device */
ATTR_TEXT_IN_SYSRAM hal_flash_status_t hal_flash_unlock(void)
{
    int ret = 0;
    uint32_t mask = save_and_set_interrupt_mask();

    ret = sf_write_sr(0x0);

    restore_interrupt_mask(mask);

    return ret;
}


ATTR_TEXT_IN_RAM  int  otp_read_mxic(uint32_t addr, uint32_t len, uint8_t *buffer)
{
    uint32_t i;
    int ret = 0;

    if ((addr + len > MXIC_OTP_REGION_SIZE) || (buffer == NULL) || (len == 0))
        return HAL_FLASH_STATUS_ERROR;

    /* Enter secured OTP mode */
    ret = sf_prgm_cmd(MXIC_ENSO, NULL, 0, NULL, 0);
    if (ret)
        return ret;

    sf_set_rw_addr(addr);

    for (i = 0; i < len; i++) {
        ret = wr_poll_cmd_reg(CMD_TYPE_AUTOINC | CMD_TYPE_RD,
                              CMD_TIMEOUT,
                              CHECK_INTERVAL_10_US);
        if (ret)
            return ret;

        buffer[i] = SF_READB(REG_SF_RDATA);
    }


    /* Exit secured OTP mode */
    ret = sf_prgm_cmd(MXIC_EXSO, NULL, 0, NULL, 0);
    if (ret)
        return ret;

    return HAL_FLASH_STATUS_OK;
}

ATTR_TEXT_IN_RAM  int  otp_write_mxic(uint32_t addr, uint32_t len, uint8_t *buffer)
{
    uint32_t i;
    int ret = 0;

    if ((addr + len > MXIC_OTP_REGION_SIZE) || (buffer == NULL) || (len == 0))
        return HAL_FLASH_STATUS_ERROR;

    /* Enter secured OTP mode */
    ret = sf_prgm_cmd(MXIC_ENSO, NULL, 0, NULL, 0);
    if (ret)
        return ret;

    sf_set_rw_addr(addr);

    for (i = 0; i < len; i++, addr++, buffer++) {
        SF_WRITEL(REG_SF_WDATA, *buffer);
        ret = sf_do_write();
        if (ret)
            return ret;
    }

    /* Exit secured OTP mode */
    ret = sf_prgm_cmd(MXIC_EXSO, NULL, 0, NULL, 0);
    if (ret)
        return ret;

    return HAL_FLASH_STATUS_OK;
}


/********************************************
  * Support winbond & GigaDevice
  ********************************************/
ATTR_TEXT_IN_RAM  int sf_read_otp_byte_winbond(uint32_t addr, uint32_t len, uint8_t *buffer)
{
    uint32_t    i, tmp_addr;
    uint8_t     data_write[4] ;
    int         ret = 0, j;

    tmp_addr = addr;
    for (i = 0; i < len; i++, buffer++) {
        for (j = sflash.addr_width - 1; j >= 0; j--) {
            data_write[j] = tmp_addr & 0xff;
            tmp_addr >>= 8;
        }
        ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
        if (ret)
            return ret;
        /* one time read one byte */
        ret = sf_prgm_cmd(WINBOND_OTP_READ, data_write, 4, buffer, 1);
        if (ret)
            return ret;

        addr += 1;
        tmp_addr = addr;
    }
    return ret;
}


ATTR_TEXT_IN_RAM  int sf_write_otp_byte_winbond(uint32_t addr, uint32_t len, uint8_t *buffer)
{
    uint32_t    i, tmp_addr;
    uint8_t     data_write[4] ;
    int         ret, j;
    uint8_t tmp_value = 0;

    tmp_addr = addr;
    for (i = 0; i < len; i++, buffer++) {
        for (j = sflash.addr_width; j >= 0; j--) {
            if ((uint32_t)j < sflash.addr_width) {
                data_write[j] = tmp_addr & 0xff;
                tmp_addr >>= 8;
            }
            if ((uint32_t)j == sflash.addr_width) {
                data_write[j] = *buffer;
            }
        }

        ret = sf_write_enable();
        if (ret)
            return ret;

        if (sf_get_read_mode() == QUADIO_READ)
            tmp_value = SF_READL(REG_SF_PRGDATA4);

        ret = sf_prgm_cmd(WINBOND_OTP_WRITE, data_write, 4, NULL, 0);
        if (ret)
            return ret;

        if (sf_get_read_mode() == QUADIO_READ)
            SF_WRITEL(REG_SF_PRGDATA4, tmp_value);

        addr ++ ;
        tmp_addr = addr;
        ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
        if (ret)
            return ret;
    }
    return ret;
}


ATTR_TEXT_IN_RAM int otp_read_winbond(uint32_t addr, uint32_t len, uint8_t *buffer)
{
    uint32_t    sr_addr = 0x000000, i;
    int         ret = 0;
    uint32_t    head_len, body_len, tail_len;
    uint8_t     body_cnt;

    if (((addr + len) > WINBOND_OTP_REGION_SIZE) || (buffer == NULL) || (len == 0))
        return -1;

    sr_addr = WINBOND_OTP_SR_ADDR_1 * ((addr / WINBOND_OTP_SR_SIZE) + 1) + (addr % WINBOND_OTP_SR_SIZE);

    head_len = WINBOND_OTP_SR_SIZE - addr % WINBOND_OTP_SR_SIZE;
    if (len > head_len) {
        body_len = ((len - head_len) / WINBOND_OTP_SR_SIZE) * WINBOND_OTP_SR_SIZE;
        body_cnt = body_len / WINBOND_OTP_SR_SIZE;
        tail_len = (len - head_len - body_len) % WINBOND_OTP_SR_SIZE;
    } else {
        head_len = len;
        body_len = 0;
        tail_len = 0;
    }

    if (head_len) {
        ret = sf_read_otp_byte_winbond(sr_addr, head_len, buffer);
        if (ret)
            return ret;

        buffer += head_len;
    }

    if (body_len) {
        for (i = 0; i < body_cnt; i++) {
            sr_addr += WINBOND_OTP_SR_ADDR_1;
            //sr_addr &= 0x3000;
            sr_addr = WINBOND_OTP_SR_ADDR_1 * (sr_addr / WINBOND_OTP_SR_ADDR_1);
            ret = sf_read_otp_byte_winbond(sr_addr, WINBOND_OTP_SR_SIZE, buffer);
            if (ret)
                return ret;

            buffer += WINBOND_OTP_SR_SIZE;
        }
    }

    if (tail_len) {
        sr_addr += WINBOND_OTP_SR_ADDR_1;
        //sr_addr &= 0x3000;
        sr_addr = WINBOND_OTP_SR_ADDR_1 * (sr_addr / WINBOND_OTP_SR_ADDR_1);
        ret = sf_read_otp_byte_winbond(sr_addr, tail_len, buffer);
        if (ret)
            return ret;
    }

    return ret;
}


ATTR_TEXT_IN_RAM int otp_write_winbond(uint32_t addr, uint32_t len, uint8_t *buffer)
{
    uint32_t    sr_addr = 0x000000, i;
    int         ret = 0;
    uint32_t    head_len, body_len, tail_len;
    uint8_t     body_cnt;

    if (((addr + len) > WINBOND_OTP_REGION_SIZE) || (buffer == NULL) || (len == 0))
        return -1;

    sr_addr = WINBOND_OTP_SR_ADDR_1 * ((addr / WINBOND_OTP_SR_SIZE) + 1) + (addr % WINBOND_OTP_SR_SIZE);

    head_len = WINBOND_OTP_SR_SIZE - addr % WINBOND_OTP_SR_SIZE;
    if (len > head_len) {
        body_len = ((len - head_len) / WINBOND_OTP_SR_SIZE) * WINBOND_OTP_SR_SIZE;
        body_cnt = body_len / WINBOND_OTP_SR_SIZE;
        tail_len = (len - head_len - body_len) % WINBOND_OTP_SR_SIZE;
    } else {
        head_len = len;
        body_len = 0;
        tail_len = 0;
    }

    if (head_len) {
        ret = sf_write_otp_byte_winbond(sr_addr, head_len, buffer);
        if (ret)
            return ret;

        buffer += head_len;
    }

    if (body_len) {
        for (i = 0; i < body_cnt; i++) {
            sr_addr += WINBOND_OTP_SR_ADDR_1;
            //sr_addr &= 0x3000;
            sr_addr = WINBOND_OTP_SR_ADDR_1 * (sr_addr / WINBOND_OTP_SR_ADDR_1);
            ret = sf_write_otp_byte_winbond(sr_addr, WINBOND_OTP_SR_SIZE, buffer);
            if (ret)
                return ret;

            buffer += WINBOND_OTP_SR_SIZE;
        }
    }

    if (tail_len) {
        sr_addr += WINBOND_OTP_SR_ADDR_1;
        //sr_addr &= 0x3000;
        sr_addr = WINBOND_OTP_SR_ADDR_1 * (sr_addr / WINBOND_OTP_SR_ADDR_1);
        ret = sf_write_otp_byte_winbond(sr_addr, tail_len, buffer);
        if (ret)
            return ret;
    }

    return ret;
}


ATTR_TEXT_IN_RAM int otp_lockstatus_winbond(uint8_t *lockstatus)
{
    int ret = 0;

    ret = sf_read_cr(lockstatus);
    if (ret) {
        return ret;
    }

    if ((*lockstatus >> 3) == 7) {
        *lockstatus = LOCK;
    } else {
        *lockstatus = UNLOCK;
    }

    return 0;
}


ATTR_TEXT_IN_RAM int otp_lockstatus_mxic(uint8_t *lockstatus)
{
    int ret;

    ret = sf_prgm_cmd(MXIC_OTP_RDSCUR, NULL, 0, lockstatus, 1);
    if (ret)
        return ret;

    *lockstatus = SF_READB(REG_SF_SHREG0);

    if ((*lockstatus >> 1) == 1) {
        *lockstatus = LOCK;
    } else {
        *lockstatus = UNLOCK;
    }

    return 0;
}


ATTR_TEXT_IN_RAM int otp_lock_winbond(void)
{
    uint8_t lock;
    uint8_t lockstatus;
    int ret = 0;

    ret = sf_read_cr(&lockstatus);
    if (ret) {
        return ret;
    }

    lock =  lockstatus | 0x38;   //winbond status reg2 bit[5:3] =Security Register Lock Bits (LB3, LB2, LB1)
    ret = sf_write_cr(lock);

    return ret;
}


ATTR_TEXT_IN_RAM int otp_lock_mxic(void)
{
    int ret;
    uint8_t config;

    ret = otp_lockstatus_mxic(&config);
    if (ret)
        return ret;

    ret = sf_write_enable();

    if (ret)
        return ret;
    /* Wait here to ensure write reg or program is done */
    ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);

    if (ret)
        return ret;

    ret = sf_prgm_cmd(MXIC_OTP_WRSCUR, NULL, 0, NULL, 0);

    if (ret)
        return ret;

    /* Wait here to ensure register is written to device */
    return sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);

}


ATTR_TEXT_IN_RAM hal_flash_status_t hal_flash_otp_read(uint32_t addr, uint32_t len, uint8_t *buffer)
{
    int ret = 0;
    uint32_t mask = save_and_set_interrupt_mask();

    if (flash_status.manu_id == NOR_MF_MACRONIX)
        ret = otp_read_mxic(addr, len, buffer);
    else if (flash_status.manu_id == NOR_MF_WINBOND) {
        ret = sf_set_read_mode(SINGLE_READ, false);
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        }

        ret = otp_read_winbond(addr, len, buffer);

        if (ret) {
            ret = sf_set_read_mode(QUADIO_READ, false);
            if (ret) {
                restore_interrupt_mask(mask);
                return ret;
            }
            restore_interrupt_mask(mask);
            return ret;
        }

        ret = sf_set_read_mode(QUADIO_READ, false);
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        }
    }

    restore_interrupt_mask(mask);

    return (ret == 0 ? HAL_FLASH_STATUS_OK : HAL_FLASH_STATUS_ERROR);
}


ATTR_TEXT_IN_RAM hal_flash_status_t hal_flash_otp_write(uint32_t addr, uint32_t len, uint8_t *buffer)
{
    int ret = 0;
    uint32_t mask = save_and_set_interrupt_mask();

    if (flash_status.manu_id == NOR_MF_MACRONIX)
        ret = otp_write_mxic(addr, len, buffer);
    else if (flash_status.manu_id == NOR_MF_WINBOND) {
        ret = sf_set_read_mode(SINGLE_READ, false);
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        }

        ret = otp_write_winbond(addr, len, buffer);
        if (ret) {
            ret = sf_set_read_mode(QUADIO_READ, false);
            if (ret) {
                restore_interrupt_mask(mask);
                return ret;
            }
            restore_interrupt_mask(mask);
            return ret;
        }

        ret = sf_set_read_mode(QUADIO_READ, false);
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        }

    }
    restore_interrupt_mask(mask);

    return (ret == 0 ? HAL_FLASH_STATUS_OK : HAL_FLASH_STATUS_ERROR);
}


ATTR_TEXT_IN_RAM hal_flash_status_t hal_flash_otp_lock(void)
{
    int ret = 0;
    uint32_t mask = save_and_set_interrupt_mask();

    if (flash_status.manu_id == NOR_MF_MACRONIX)
        ret = otp_lock_mxic();
    else if (flash_status.manu_id == NOR_MF_WINBOND) {
        ret = sf_set_read_mode(SINGLE_READ, false);
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        }
        ret = otp_lock_winbond();

        if (ret) {
            ret = sf_set_read_mode(QUADIO_READ, false);
            if (ret) {
                restore_interrupt_mask(mask);
                return ret;
            }
            restore_interrupt_mask(mask);
            return ret;
        }

        ret = sf_set_read_mode(QUADIO_READ, false);
        if (ret) {
            restore_interrupt_mask(mask);
            return ret;
        }

    }
    restore_interrupt_mask(mask);

    return (ret == 0 ? HAL_FLASH_STATUS_OK : HAL_FLASH_STATUS_ERROR);
}

// lockstatus = 1: OTP LOCK; lockstatus = 0: OTP UNLOCK;
ATTR_TEXT_IN_RAM hal_flash_status_t hal_flash_otp_lockstatus(uint8_t *lockstatus)
{
    int ret = 0;
    uint32_t mask = save_and_set_interrupt_mask();

    if (flash_status.manu_id == NOR_MF_MACRONIX)
        ret = otp_lockstatus_mxic(lockstatus);
    else if (flash_status.manu_id == NOR_MF_WINBOND)
        ret = otp_lockstatus_winbond(lockstatus);

    restore_interrupt_mask(mask);

    return (ret == 0 ? HAL_FLASH_STATUS_OK : HAL_FLASH_STATUS_ERROR);

}


ATTR_TEXT_IN_SYSRAM hal_flash_status_t Flash_ReturnReady(void)
{
    int ret = 0;
    uint32_t mask = save_and_set_interrupt_mask();

    if (NOR_FLASH_BUSY && !NOR_FLASH_SUSPENDED) {
        sf_send_suspend();
        /* wait for device ready */
        do {
            ret = sf_wait_sr_ready(ST_BUSY_TIMEOUT, CHECK_INTERVAL_10_US);
        } while (ERR_BUSY == ret);
    } else {
        NOR_FLASH_BUSY = FALSE;
    }
    restore_interrupt_mask(mask);

    return ret;
}


