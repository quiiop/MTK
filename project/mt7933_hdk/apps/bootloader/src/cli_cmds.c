/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */


#include <stdint.h>
#include <stddef.h>
#include <string.h>


#if defined(MTK_MINICLI_ENABLE)


#include "cli.h"


static cli_t *_cli_ptr;


#include "hal_flash.h"
#include "xmodem.h"


#include "bl_image.h"
#include "bl_test.h"


/****************************************************************************
 *
 * Integer conversion
 *
 ****************************************************************************/


enum {
    TOI_BIN,
    TOI_OCT,
    TOI_DEC,
    TOI_HEX,
    TOI_ERR
};


#define is_0_to_1(_c)   (_c == '0' || _c == '1')
#define is_0(_c)        (_c == '0')
#define is_x(_c)        (_c == 'x' || _c == 'X')
#define is_b(_c)        (_c == 'b' || _c == 'B')
#define is_0_to_7(_c)   (_c >= '0' && _c <= '7')
#define is_0_to_9(_c)   (_c >= '0' && _c <= '9')
#define is_1_to_9(_c)   (_c >= '1' && _c <= '9')
#define is_a_to_f(_c)   (_c >= 'a' && _c <= 'f')
#define is_A_to_F(_c)   (_c >= 'A' && _c <= 'F')

#define dec_to_dec(_c)  (_c - '0')
#define hex_to_dec(_c)  (_c - 'a' + 10)
#define heX_to_dec(_c)  (_c - 'A' + 10)

/**
 * Detect integer type.
 *
 * @param b     input string buffer.
 * @param type  the detected type.
 *
 * @return      Pointer address. The address is not useful if type is error.
 */
static char *_toi_detect(char *b, uint8_t *type)
{
    *type = TOI_ERR;

    if (is_1_to_9( *b )) {
        *type = TOI_DEC;
    } else if (is_0( *b )) {
        b++;
        if (is_x( *b )) {
            b++;
            *type = TOI_HEX;
        } else if (is_b( *b )) {
            b++;
            *type = TOI_BIN;
        } else if (is_0_to_7( *b )) {
            *type = TOI_OCT;
        } else if (*b == 0) {
            *type = TOI_DEC; // a single '0'.
        }
    }

    return b;
}

uint32_t toi(char *b, uint8_t *type)
{
    uint32_t    v       = 0;

    b = _toi_detect(b, type);

    if (*type == TOI_ERR)
        return v;

    if (*type == TOI_BIN) {
        while (*b != 0) {
            if (!is_0_to_1( *b )) {
                *type = TOI_ERR; break;
            }
            v  = v << 1;
            v += *b - '0';
            b++;
        }
    }

    if (*type == TOI_OCT) {
        while (*b != 0) {
            if (!is_0_to_7( *b )) {
                *type = TOI_ERR; break;
            }
            v  = v << 3;
            v += *b - '0';
            b++;
        }
    }

    if (*type == TOI_DEC) {
        while (*b != 0) {
            if (!is_0_to_9( *b )) {
                *type = TOI_ERR; break;
            }
            v  = v * 10;
            v += *b - '0';
            b++;
        }
    }

    if (*type == TOI_HEX) {
        while (*b != 0) {
            v  = v << 4;
            if (is_0_to_9( *b )) {
                v += *b - '0';
            } else if (is_a_to_f( *b )) {
                v += *b - 'a' + 10;
            } else if (is_A_to_F( *b )) {
                v += *b - 'A' + 10;
            } else {
                *type = TOI_ERR; break;
            }
            b++;
        }
    }

    return (*type == TOI_ERR) ? 0 : (uint32_t)v;
}


uint8_t tov(char *b, uint32_t *value)
{
    uint8_t     type;

    *value = toi(b, &type);

    return type;
}


/****************************************************************************
 *
 * TEST commands
 *
 ****************************************************************************/


#define MEM_C(a)  (*(volatile unsigned char *)(a))
#define MEM_I(a)  (*(volatile unsigned int  *)(a))


struct mem_info
{
    char        *desc;
    uint32_t    base;
    uint32_t    limit;
};


static const struct mem_info _g_mem_info[] =
{
    { "CM33 LOCAL - ROM",                  0x00000000, 0x00010000 },
    { "CM33 LOCAL - TCM First (rsvd)",     0x0010b41c, 0x0010b42c },
    { "CM33 LOCAL - TCM First",            0x00100000, 0x00110000 },
    { "CM33 LOCAL - TCM_Next",             0x00110000, 0x00118000 },
    { "CM33 LOCAL - SYSRAM_ALIAS",         0x08000000, 0x10000000 },
    { "CM33 LOCAL - PSRAM_ALIAS",          0x10000000, 0x18000000 },
    { "CM33 LOCAL - FLASH_ALIAS",          0x18000000, 0x20000000 },
    { "CM33 LOCAL - CM33_CFG",             0x20000000, 0x20100000 },
    { "CM33 LOCAL - CM33_IRQ_CFG",         0x21000000, 0x21010000 },
    { "CM33 LOCAL - CM33_APXGPT",          0x21010000, 0x21011000 },
    { "CM33 LOCAL - WDT",                  0x21020000, 0x21030000 },
    { "CM33 LOCAL - UART",                 0x21040000, 0x21040100 },
    { "CM33 LOCAL - CM33_BUS_REG",         0x21050000, 0x21060000 },
    { "CM33 LOCAL - CHIP_MISC_CFG",        0x21060000, 0x21070000 },
    { "CM33 LOCAL - CM33_LOCAL_BUS_BRCM",  0x210A0000, 0x210A1000 },
    { "CM33 LOCAL - PPB",                  0xE0000000, 0xE0100000 },
    { "CM33 LOCAL - VENDOR_SYS",           0xE0100000, 0xFFFFFFFF },

    { "CM33&DSP - Firewall",               0x30000000, 0x30010000 },
    { "CM33&DSP - Audio ADC/DAC",          0x30010000, 0x30020000 },
    { "CM33&DSP - TOP_CLK_OFF",            0x30020000, 0x30030000 },
    { "CM33&DSP - TOP_CFG_AON",            0x30030000, 0x30040000 },
    { "CM33&DSP - PWM 0~2",                0x30040000, 0x30070000 },
    { "CM33&DSP - RTC",                    0x30070000, 0x30080000 },
    { "CM33&DSP - SEJ",                    0x30090000, 0x300A0000 },
    { "CM33&DSP - Security wrapper",       0x300A0000, 0x300B0000 },
    { "CM33&DSP - PMU",                    0x300B0000, 0x300C0000 },
    { "CM33&DSP - PLL",                    0x300C0000, 0x300D0000 },
    { "CM33&DSP - SPM",                    0x300D0000, 0x300E0000 },
    { "CM33&DSP - XTAL CTRL",              0x300E0000, 0x300F0000 },
    { "CM33&DSP - DEVAPC AON",             0x30300000, 0x30304000 },
    { "CM33&DSP - SYSRAM",                 0x80000000, 0x90000000 },
    { "CM33&DSP - PSRAM",                  0xA0000000, 0xB0000000 },
    { "CM33&DSP - FLASH",                  0x90000000, 0xA0000000 },

    { NULL, 0, 0 }
};


const struct mem_info *mem_type_find(const struct mem_info *info, uint32_t addr)
{
    info = !info ? &_g_mem_info[0] : (info + 1);

    while (info->desc) {
        if (info->base <= addr && info->limit > addr)
            return info;
        info++;
    }

    return NULL;
}

static uint8_t _cmd_mem_type(uint8_t len, char *param[])
{
    uint8_t     type;
    uint32_t    addr;
    int         i;
    const struct mem_info *info;

    if (len == 0)
    {
        info = &_g_mem_info[0];
        while (info->desc) {
            cli_puts (info->desc);  cli_putc(' ');
            cli_put0x(info->base);  cli_putc(' ');
            cli_put0x(info->limit); cli_putln();
            info++;
        }
        return 0;
    }

    for (i = 0; i < len; i++)
    {
        addr = toi(param[i], &type);
        if (type == TOI_ERR) return 1;

        info = mem_type_find(NULL, addr);

        do {
            cli_put0x(addr); cli_putc(' ');
            if (info == NULL) {
                cli_puts("unknown"); cli_putln();
                break;
            }

            cli_puts((char *)info->desc);  cli_putc(' '); // fixme
            cli_put0x(info->base);  cli_putc(' ');
            cli_put0x(info->limit); cli_putln();
            info = mem_type_find(info, addr);
        } while (info);
    }

    return 0;
}


static uint8_t _cmd_find_mem(uint8_t len, char *param[])
{
#define PAT_MAX     (32)
    uint8_t type;
    int addr, size;
    int i, j;
    uint8_t pattern[PAT_MAX];

    if (len < 3 || len > PAT_MAX + 2)
        return 1;

    addr = toi(param[0], &type);
    if (type == TOI_ERR) return 1;

    size = toi(param[1], &type);
    if (type == TOI_ERR) return 1;

    for (j = 2; j < len; j++) {
        pattern[j -  2] = toi(param[j], &type);
        if (type == TOI_ERR) return 1;
    }

    for (i = addr; i < addr + size + 2 - len; i++) {
        for (j = 0; j < len - 2; j++)
            if (pattern[j] != MEM_C(i + j))
                break;
        if (j == len - 2) {
            cli_puts("found at ");
            cli_put0x(i);
            cli_putln();
        }
    }

    return 0;
}


static uint8_t _cmd_dump_mem(uint8_t len, char *param[])
{
    uint8_t type, c;
    int addr, size;
    int i = 0, j;

    while (i < len) {
        addr = toi(param[i], &type);
        if (type == TOI_ERR) break;

        if (i + 1 < len) {
            size = toi(param[i + 1], &type);
        } else
            size = 64; // default 64 bytes
        i += 2;

        // dump memory
        for (j = (addr & ~15); j < addr + size; j++) {
            // print address
            if ((j & 15) == 0) {
                cli_putx(j);
                cli_putc(' ');
            }

            // print 1 byte or 'blank'
            if (j >= addr) {
                c = MEM_C(j);
                cli_putx(c >> 4);
                cli_putx(c & 15);
            } else {
                cli_putc(' ');
                cli_putc(' ');
            }

            if ((j & 15) == 15)
                cli_putln();
            else if ((j & 15) == 7) {
                cli_putc(' ');
                cli_putc(' ');
            } else
                cli_putc(' ');
        }
    }
    return 1;
}


static uint8_t _cmd_fill_mem(uint8_t len, char *param[])
{
    uint8_t type, c;
    int addr, size;

    if (len == 3) {
        addr = toi(param[0], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        c = (uint8_t)toi(param[1], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        size = toi(param[2], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        memset((void *)addr, c, (size_t)size);
    }

    return 0;
}



struct _cmd_xmodem_rx_data_s
{
    bool    flash;
    void    *target;
    int     received;
};


static uint32_t flash_map_to_phy(uint32_t addr)
{
    /*
     * FLASH PHYSICAL ADDRESS    - accessible via hal_flash API
     * FLASH MEMORY-MAPPED ALIAS - memory mapped address alias
     * FLASH MEMORY-MAPPED       - memory mapped address
     */
    if (addr >= 0x90000000)
        return addr - 0x90000000;
    if (addr >= 0x18000000)
        return addr - 0x18000000;
    return addr;
}


static int _cmd_xmodem_rx_write_fail;
static void _cmd_xmodem_rx_callback(void *ptr, uint8_t *buffer, int len)
{
    struct              _cmd_xmodem_rx_data_s *rx_data = ptr;
    hal_flash_status_t  status;
    uint32_t            addr;

    if (rx_data->flash)
    {
        addr = (uint32_t)rx_data->target + rx_data->received;

        status = hal_flash_write(flash_map_to_phy(addr), buffer, len);
        if (status != HAL_FLASH_STATUS_OK)
            _cmd_xmodem_rx_write_fail++;
    }
    else
    {
        memcpy(rx_data->target + rx_data->received, buffer, len);
    }

    rx_data->received += len;
}


static uint8_t _cmd_xd_to_mem(uint8_t len, char *param[])
{
    uint8_t type;
    uint32_t addr, size;
    struct _cmd_xmodem_rx_data_s rx_data;
    int i = 0;

    if (len == 0 || len > 2) {
        cli_puts("param error\n");
        return 1;
    }

    addr = toi(param[0], &type);
    if (type == TOI_ERR) {
        cli_puts("param error!\n");
        return 1;
    }

    if (len == 2) {
        size = toi(param[1], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }
    } else {
        size = 1024 * 1024; // 1MB as the default maximum.
    }

    rx_data.flash       = false;
    rx_data.target      = (void *)addr;
    rx_data.received    = 0;

    i = xmodem_block_rx(&rx_data, NULL, size, _cmd_xmodem_rx_callback);

    if (i > 0) {
        cli_puts("rx ");
        cli_putd(i);
        cli_puts(" bytes");
    } else {
        cli_puts("error ");
        cli_putd(i);
    }
    cli_putln();

    return 0;
}


static uint8_t _cmd_read_reg(uint8_t len, char *param[])
{
    uint8_t type;
    uint32_t addr;
    int i = 0;

    while (i < len) {
        addr = toi(param[i], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
        } else {
            cli_puts("rr ");
            cli_put0x(addr);
            cli_putln();
            cli_put0x(addr);
            cli_puts(": ");
            cli_put0x(MEM_I(addr));
            cli_putln();
        }
        i++;
    }

    return 0;
}


static uint8_t _cmd_write_reg(uint8_t len, char *param[])
{
    uint8_t type;
    uint32_t addr, data;
    int i = 0;

    if (len & 1) {
        cli_puts("<reg> <value> pairs expected\n");
        return 1;
    }

    while (i < len) {
        addr = toi(param[i], &type);
        if (type == TOI_ERR) {
            cli_puts("param error\n");
            return 2;
        }
        data = toi(param[i + 1], &type);
        if (type == TOI_ERR) {
            cli_puts("param error\n");
            return 2;
        }
        MEM_I(addr) = data;

        cli_puts("wr ");
        cli_put0x(addr);
        cli_putc(' ');
        cli_put0x(data);
        cli_putln();

        i += 2;
    }

    return 0;
}


static uint8_t _cmd_go(uint8_t len, char *param[])
{
    uint8_t type;
    uint32_t pc, sp;

    if (len < 1 || len > 2) {
        cli_puts("<pc> [<sp>] pair expected\n");
        return 1;
    }

    pc = toi(param[0], &type);
    if (type == TOI_ERR) {
        cli_puts("param error\n");
        return 1;
    }

    if (len > 1) {
        sp = toi(param[1], &type);
        if (type == TOI_ERR) {
            cli_puts("param error\n");
            return 2;
        }
    } else {
        sp = 0;
    }

    bl_image_jump(sp, pc);

    return 0;
}


static uint8_t _cmd_flash_init(uint8_t len, char *param[])
{
    hal_flash_status_t  status;
    status = hal_flash_init();
    if (status != HAL_FLASH_STATUS_OK) {
        cli_puts("init error: ");
        cli_putd(status);
        cli_putln();
        return 1;
    }

    cli_puts("done");
    cli_putln();

    return 0;
}


static uint8_t _cmd_flash_erase(uint8_t len, char *param[])
{
    uint8_t             type;
    int                 addr, size;
    hal_flash_status_t  status;

    if (len != 2) {
        cli_puts("param error!\n");
    } else {
        addr = toi(param[0], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        size = toi(param[1], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        if (size & (0x1000 - 1) || addr & (0x1000 - 1)) {
            cli_puts("not 4KB aligned!\n");
            return 1;
        }

        while (size) {
            status = hal_flash_erase(addr, HAL_FLASH_BLOCK_4K);
            if (status != HAL_FLASH_STATUS_OK) {
                cli_puts("erase error: ");
                cli_putd(status);
                cli_putln();
                return 2;
            }

            size -= 0x1000;
            addr += 0x1000;
        }

        cli_puts("done");
        cli_putln();
    }

    return 0;
}


static uint8_t _cmd_flash_write(uint8_t len, char *param[])
{
    uint8_t             type;
    uint32_t            mem, addr, size;
    hal_flash_status_t  status;

    if (len != 3) {
        cli_puts("param error!\n");
    } else {
        mem = toi(param[0], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        addr = toi(param[1], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        size = toi(param[2], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        if (addr & (0x1000 - 1)) {
            cli_puts("not 4KB aligned!\n");
            return 1;
        }

        cli_puts("flash addr: "); cli_put0x(addr); cli_putln();
        cli_puts("mem   addr: "); cli_put0x(mem);  cli_putln();
        cli_puts("    length: "); cli_put0x(size); cli_putln();

        status = hal_flash_write(addr,
                                 (uint8_t *)mem,
                                 size);
        if (status != HAL_FLASH_STATUS_OK) {
            cli_puts("write error: ");
            cli_putd(status);
            cli_putln();
            return 2;
        }

        cli_puts("done");
        cli_putln();
    }

    return 0;
}


static uint8_t _cmd_flash_read(uint8_t len, char *param[])
{
    uint8_t             type;
    int                 mem, addr, size;
    hal_flash_status_t  status;

    if (len != 3) {
        cli_puts("param error!\n");
    } else {
        mem = toi(param[0], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        addr = toi(param[1], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        size = toi(param[2], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        if (addr & (0x1000 - 1)) {
            cli_puts("not 4KB aligned!\n");
            return 1;
        }

        status = hal_flash_read(addr, (uint8_t *)mem, size);
        if (status != HAL_FLASH_STATUS_OK) {
            cli_puts("read error: ");
            cli_putd(status);
            cli_putln();
            return 2;
        }

        cli_puts("done");
        cli_putln();
    }

    return 0;
}


static uint8_t _cmd_flash_xdl(uint8_t len, char *param[])
{
    uint8_t                         type;
    int                             addr, size;
    struct _cmd_xmodem_rx_data_s    rx_data;
    int                             i;

    if (len != 2) {
        cli_puts("param error!\n");
    } else {
        addr = toi(param[0], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        size = toi(param[1], &type);
        if (type == TOI_ERR) {
            cli_puts("param error!\n");
            return 1;
        }

        if (addr & (0x1000 - 1)) {
            cli_puts("not 4KB aligned!\n");
            return 1;
        }

        rx_data.flash       = true;
        rx_data.target      = (void *)addr;
        rx_data.received    = 0;

        _cmd_xmodem_rx_write_fail = 0;
        i = xmodem_block_rx(&rx_data, NULL, size, _cmd_xmodem_rx_callback);

        if (i > 0) {
            cli_puts("rx ");
            cli_putd(i);
            cli_puts(" bytes");
        } else {
            cli_puts("error ");
            cli_putd(i);
        }
        cli_putln();

        if (_cmd_xmodem_rx_write_fail) {
            cli_puts("rx write issues ");
            cli_putd(_cmd_xmodem_rx_write_fail);
            cli_putln();
        }
    }

    return 0;
}


#ifdef MTK_BL_TEST


static uint8_t _cmd_test_toggle(uint8_t len, char *param[])
{
    uint8_t i;
    char    *p;
    for (i = 0; i < len; i++) {
        p = param[i];
        while (*p) {
            bl_test_cmd( *p );
            p++;
        }
    }
    return 0;
}


static uint8_t _cmd_test_dump(uint8_t len, char *param[])
{
    bl_test_dump();
    return 0;
}


#endif /* MTK_BL_TEST */


static uint8_t _cmd_exit(uint8_t len, char *param[])
{
    _cli_ptr->state = 0;
    return 0;
}


/****************************************************************************
 *
 * NORMAL MODE
 *
 ****************************************************************************/


static const cmd_t   _flash_cmds[] = {
    { "init",  "init flash",                        _cmd_flash_init,  NULL },
    { "erase", "<addr> <len> - erase flash",        _cmd_flash_erase, NULL },
    { "write", "<mem> <addr> <len> - mem to flash", _cmd_flash_write, NULL },
    { "read",  "<addr> <mem> <len> - flash to mem", _cmd_flash_read,  NULL },
    { "dl",    "<addr> <len> - d/l and write flash",_cmd_flash_xdl,   NULL },
    { NULL, NULL, NULL, NULL } // end of table
};


#ifdef MTK_BL_TEST
static const cmd_t   _test_cmds[] = {
    { "t",  "toggle options",                       _cmd_test_toggle, NULL },
    { "d",  "dump options",                         _cmd_test_dump,   NULL },
    { NULL, NULL, NULL, NULL } // end of table
};
#endif


static const cmd_t   _cmds[] = {
    { "mem",   "show memory type of <addr>",   _cmd_mem_type,      NULL },
    { "s",     "search <addr> <len> <pat>",    _cmd_find_mem,      NULL },
    { "d",     "dump memory <addr> <len>",     _cmd_dump_mem,      NULL },
    { "f",     "fill memory",                  _cmd_fill_mem,      NULL },
    { "x",     "xmodem d/l to <addr> [max]",   _cmd_xd_to_mem,     NULL },
    { "rr",    "read reg",                     _cmd_read_reg,      NULL },
    { "wr",    "write reg",                    _cmd_write_reg,     NULL },
    { "go",    "go with <pc> [<sp>]",          _cmd_go,            NULL },
    { "flash", "flash",                        NULL,     &_flash_cmds[0]},
#ifdef MTK_BL_TEST
    { "test",  "unit test",                    NULL,     &_test_cmds[0]},
#endif
    { "exit",  "exit CLI",                     _cmd_exit,          NULL },
    { NULL, NULL, NULL, NULL } // end of table
};


/****************************************************************************
 *
 * PUBLIC functions
 *
 ****************************************************************************/


void cli_cmds_init(cli_t *cli)
{
    _cli_ptr = cli;
    _cli_ptr->cmd = &_cmds[0];
}


#endif /* #if defined(MTK_MINICLI_ENABLE) */

