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
#include <common.h>
#include <reg_base.h>
#include "memory_attribute.h"
#include "hal.h"
#include "hal_ls_api.h"
#include "hal_security_api.h"
#include "hal_sleep_manager_internal.h"
#include "hal_psram.h"
#include "hal_psram_internal.h"
#include "hal_spm.h"
#include "hal_clk.h"
#include "dramc_pi_api.h"
#include "non_uhs_psram.h"
#include "hal_efuse_get.h"
#include "hal_boot.h"

#define hal_psram_log_print log_hal_info

//#define PSRAM_DEBUG_SUSPEND_RESUME
#if defined (PSRAM_DEBUG_SUSPEND_RESUME)
#define hal_psram_log_suspend_resume log_hal_info
#else /* #if defined (PSRAM_DEBUG_SUSPEND_RESUME) */
#define hal_psram_log_suspend_resume(...)
#endif /* #if defined (PSRAM_DEBUG_SUSPEND_RESUME) */

#define PSRAM_CTRL_TOP_BASE_ADDR        0x38070000
#define PMU_CTRL_BASE_ADDR              0x300B0000
#define PLL_CTRL_BASE_ADDR              0x380C0034
#define EEF_TOP_BASE_ADDR               0x30400000
#define GRP1_EEF_TOP_BASE_ADDR          0x30405000
#define GRP2_EEF_TOP_BASE_ADDR          0x30406000
#define TOP_CFG_AON_BASE_ADDR           0x30030000
#define TOP_CLK_OFF_BASE_ADDR           0x30020000

#define NONUHS_PSRAM_RESERVE_RG         0x380701E0
#define NONUHS_PSRAM_CTRL_DEBUG_CTRL_2  0x38070198

#define UHS_PSRAM_RESERVE_RG            0x3809000C

#define DLL_CTRL_CFG_STUS_ADDR          (PSRAM_CTRL_TOP_BASE_ADDR+0x80)
#define PSRAM_CTRL_CFG_ADDR             (PSRAM_CTRL_TOP_BASE_ADDR+0xB4)
#define AXI2PSRAM_CFG_2_ADDR            (PSRAM_CTRL_TOP_BASE_ADDR+0x08)
#define PSRAM_TIMING_0_ADDR             (PSRAM_CTRL_TOP_BASE_ADDR+0x50)
#define PSRAM_TIMING_1_ADDR             (PSRAM_CTRL_TOP_BASE_ADDR+0x54)
#define PSRAM_TIMING_2_ADDR             (PSRAM_CTRL_TOP_BASE_ADDR+0x58)
#define PSRAM_TIMING_3_ADDR             (PSRAM_CTRL_TOP_BASE_ADDR+0x5C)
#define PSRAM_TIMING_4_ADDR             (PSRAM_CTRL_TOP_BASE_ADDR+0x60)
#define PSRAM_CTRL_STUS_ADDR            (PSRAM_CTRL_TOP_BASE_ADDR+0xB8)

#define ARB_CFG_ADDR                    (PSRAM_CTRL_TOP_BASE_ADDR+0x70)

#define APB2PSRAM_CUSTOM_CFG_0_ADDR     (PSRAM_CTRL_TOP_BASE_ADDR+0x10)
#define APB2PSRAM_CUSTOM_CFG_1_ADDR     (PSRAM_CTRL_TOP_BASE_ADDR+0x14)
#define APB2PSRAM_CUSTOM_CFG_2_ADDR     (PSRAM_CTRL_TOP_BASE_ADDR+0x18)
#define APB2PSRAM_CUSTOM_CFG_3_ADDR     (PSRAM_CTRL_TOP_BASE_ADDR+0x1C)
#define APB2PSRAM_CUSTOM_CFG_4_ADDR     (PSRAM_CTRL_TOP_BASE_ADDR+0x20)
#define APB2PSRAM_CUSTOM_CFG_5_ADDR     (PSRAM_CTRL_TOP_BASE_ADDR+0x24)
#define APB2PSRAM_CUSTOM_CFG_6_ADDR     (PSRAM_CTRL_TOP_BASE_ADDR+0x28)
#define APB2PSRAM_CUSTOM_CFG_7_ADDR     (PSRAM_CTRL_TOP_BASE_ADDR+0x2C)
#define APB2PSRAM_CUSTOM_CFG_8_ADDR     (PSRAM_CTRL_TOP_BASE_ADDR+0x30)
#define APB2PSRAM_CUSTOM_CFG_9_ADDR     (PSRAM_CTRL_TOP_BASE_ADDR+0x34)
#define APB2PSRAM_CUSTOM_CFG_10_ADDR    (PSRAM_CTRL_TOP_BASE_ADDR+0x38)
#define APB2PSRAM_CUSTOM_CFG_11_ADDR    (PSRAM_CTRL_TOP_BASE_ADDR+0x3C)
#define APB2PSRAM_CUSTOM_CFG_12_ADDR    (PSRAM_CTRL_TOP_BASE_ADDR+0x40)

#define PSRAM_MEM_TEST_START            0xa0000000

#define UHS_PSRAM_K_TIMEOUT_SECOND                  120
#define NON_UHS_PSRAM_K_TIMEOUT_SECOND              2
#define NON_UHS_LOW_POWER_SLP_TIMEOUT_US            1000*1000UL
#define NON_UHS_LOW_POWER_WAKEUP_TIMEOUT_US         1000*1000UL
#define NON_UHS_REG_CMD_WR_BLOCK_TIMEOUT_US         1000*1000UL
#define NON_UHS_REG_CMD_WR_NO_BLOCK_TIMEOUT_US      1000*1000UL
#define NON_UHS_REG_CMD_RD_BLOCK_TIMEOUT_US         1000*1000UL
#define UHS_PSRAM_SIDLE_TO_S1_SREF_TIMEOUT_US       1000*1000UL
#define UHS_PSRAM_S1_TO_SIDLE_SREF_TIMEOUT_US       1000*1000UL
#define UHS_PSRAM_SIDLE_TO_S1_HSLP_TIMEOUT_US       1000*1000UL
#define UHS_PSRAM_S1_TO_SIDLE_HSLP_TIMEOUT_US       1000*1000UL
#define UHS_PSRAM_S1_TO_S0_TIMEOUT_US               1000*1000UL
#define UHS_PSRAM_S0_TO_S1_TIMEOUT_US               1000*1000UL

#define EEF_GRP2_BASE_ADDR        0x00000000
#define EFUSE_VERSION_REG_ADDR          (EEF_GRP2_BASE_ADDR + 0x02)

#ifndef MTK_TFM_ENABLE
#define AO_RG_ADDR          0x2106000C
#else /* #ifndef MTK_TFM_ENABLE */
#define AO_RG_ADDR          0xB106000C
#endif /* #ifndef MTK_TFM_ENABLE */

//#define PSRAM_WRITE_READ_TEST

typedef struct {
    uint32_t    addr;
    uint32_t    value[4];
} ePsram_CR_table;

typedef struct {
    uint8_t     value_id;
    PSRAM_MODE_ENUM mode;
    uint8_t     start_1st;
    uint8_t     end_1st;
    uint8_t     start_2nd;
    uint8_t     end_2nd;
} ePsram_CR_ctl_tbl;

struct hal_psram_CR {
    uint32_t    addr;
    uint32_t    value;
};

struct hal_ls_psram_e2_table {
    struct hal_ls_table e1_table;
    uint8_t CR_1st_count;
    struct hal_psram_CR *p_psram_CR_1st;
    uint32_t *p_para_array;
    uint8_t CR_2nd_count;
    struct hal_psram_CR *p_psram_CR_2nd;
} __attribute__((aligned(64)));

unsigned int psram_log_enable = 0;
int uhs_psram_k_result = 0;

p_asic_mpu_config_callbak g_psram_cb = NULL;
unsigned char non_uhs_psram_1st_k_fail = 0;

extern ePsram_CLK_type non_uhs_psram_dest_clk;

#define PARA_ARRAY_NUM      20
#define CR_1ST_MAX_NUM      5
#define CR_2ND_MAX_NUM      6

ATTR_RWDATA_IN_SYSRAM
struct hal_psram_CR CR_1st[CR_1ST_MAX_NUM];

ATTR_RWDATA_IN_SYSRAM
struct hal_psram_CR CR_2nd[CR_2ND_MAX_NUM];

ATTR_RWDATA_IN_SYSRAM
uint32_t para_array[PARA_ARRAY_NUM] = {0};

// load/restore table for non-uhs psram low power
ATTR_RWDATA_IN_SYSRAM
struct hal_ls_register psram_nonuhs_psram_ls_cr_entries[] = {
    { 0x38070048, 0, 0 },
    { 0x38070084, 0, 0 },
    { 0x38070088, 0, 0 },
    { 0x3807008C, 0, 0 },
    { 0x38070090, 0, 0 },
    { 0x38070094, 0, 0 },
    { 0x38070098, 0, 0 },
    { 0x380700A0, 0, 0 },
    { 0x380700A4, 0, 0 },
    { 0x380700A8, 0, 0 },

    { 0x30020260, 0, 0 }, // PSRAM_CLK_CTL
    { 0x30020278, 0, 0 }  // PSRAM_AXI_CLK_CTL
};

ATTR_RWDATA_IN_SYSRAM
struct hal_ls_module psram_nonuhs_psram_ls_modules[] = {
    {
        .init_count    = 0,
        .init_register = NULL,

        .conf_count    = sizeof(psram_nonuhs_psram_ls_cr_entries) / sizeof(struct hal_ls_register),
        .conf_register = &psram_nonuhs_psram_ls_cr_entries[0],

        .lock_count    = 0,
        .lock_register = NULL
    }
};

ATTR_RWDATA_IN_SYSRAM
struct hal_ls_psram_e2_table nonuhs_psram_ls_e2_table = {
    .e1_table.count         = sizeof(psram_nonuhs_psram_ls_modules) / sizeof(struct hal_ls_module),
    .e1_table.modules       = &psram_nonuhs_psram_ls_modules[0],

    .p_psram_CR_1st         = NULL,
    .p_para_array           = NULL,
    .p_psram_CR_2nd         = NULL
};

ATTR_RWDATA_IN_SYSRAM
struct hal_ls_register spm_security_psram_nonuhs_ls_cr_entries[] = {
    { 0x38070048, 0, 0 },
    { 0x38070198, 0, 0x01 },
    { 0x38070058, 0, 0x08 },
    { 0x38070054, 0, 0x63 }
};

ATTR_RWDATA_IN_SYSRAM
struct hal_ls_register spm_security_psram_uhs_ls_cr_entries[] = {
    { 0x380c0000, 0, 0 }
};

static unsigned char efuse_to_AO_done = 0;

static void NONUHS_REG_CMD_WR_NO_BLOCK(uint32_t csn_low_clk_en, uint32_t csh_ckon_en, uint32_t cs_n_sel, uint32_t fsm_cmd_en, uint32_t fsm_lat_en, uint32_t fsm_dat_en, uint32_t fsm_dum_en, uint32_t fsm_csl_en, uint32_t fsm_rst_en, uint32_t cmd_cyc_num, uint32_t dat_cyc_num, uint32_t dum_cyc_num, uint32_t csl_ckoff, uint32_t csh_ckon, uint32_t cmd1, uint32_t cmd0, uint32_t wdata3, uint32_t wdata2, uint32_t wdata1, uint32_t wdata0);

#ifdef PSRAM_WRITE_READ_TEST
#define MEM_BASE MEM32_BASE
#define PATTERN1 0x0
#define PATTERN2 0xffffffff
#define _errorExit(errcode) return(-errcode)

static U32 U64_high32_value(U64 value)
{
    return (U32)((value >> 32) & 0xFFFFFFFF);
}

static U32 U64_low32_value(U64 value)
{
    return (U32)(value & 0xFFFFFFFF);
}

static int memory_read_write_test(U32 start, U32 len)
{
    P_U8 MEM8_BASE = (P_U8)start;
    P_U16 MEM16_BASE = (P_U16)start;
    P_U32 MEM32_BASE = (P_U32)start;
    P_U64 MEM64_BASE = (P_U64)start;
    U8 pattern8 = 0;
    U16 pattern16 = 0;
    U64 pattern64 = 0;
    U32 i = 0;
    U32 j = 0;
    U32 size = 0;
    U32 pattern32 = 0;
    U32 value = 0;

    hal_psram_log_print("Test start address = %x, test length = %x\n", start, len);

    size = len >> 2;

    if ((start & 0xFFFFF) == 0) {
        __asm__ __volatile__("nop");
    }

    /* === Verify the tied bits (tied low) === */
    for (i = 0; i < size; i++) {
        MEM32_BASE[i] = 0;
    }

    for (i = 0; i < size; i++) {
        if (MEM32_BASE[i] != 0) {
            hal_psram_log_print("Tied low Test: Address %x not all zero, %x!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i]);
            hal_psram_log_print("....32bits all zero test: Fail!\n\r");
            _errorExit(1);
        } else {
            MEM32_BASE[i] = 0xffffffff;
        }
    }
    if (i == size)
        hal_psram_log_print("..32bits all zero test: Pass!\n\r");


    /* === Verify the tied bits (tied high) === */

    for (i = 0; i < size; i++) {
        if (MEM32_BASE[i] != 0xffffffff) {
            hal_psram_log_print("Tied High Test: Address %x not equal 0xFFFFFFFF, %x!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i]);
            hal_psram_log_print("....32bits all one test: Fail!\n\r");
            _errorExit(2);
        } else {
            MEM32_BASE[i] = 0x00;
        }
    }
    if (i == size)
        hal_psram_log_print("..32bits all one test: Pass!\n\r");


    /* === Verify pattern 1 (0x00, 0x01, 0x02, ... 0xFF) === */

    pattern8 = 0x00;
    for (i = 0; i < len; i++)
        MEM8_BASE[i] = pattern8++;

    pattern8 = 0x00;
    for (i = 0; i < len; i++) {
        if (MEM8_BASE[i] != pattern8++) {
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM8_BASE[i]), MEM8_BASE[i], --pattern8);
            hal_psram_log_print("....8bits 0x00~0xff pattern test: Fail!\n\r");
            _errorExit(3);
        }
    }
    if (i == len)
        hal_psram_log_print("..8bits 0x00~0xff pattern test: Pass!\n\r");

    /* === Verify pattern 2 (0x0000, 0x0001, 0x0002, ... 0xFFFF) === */

    pattern8 = 0x00;
    for (i = j = 0; i < len; i += 2, j++) {
        if (MEM8_BASE[i] == pattern8)
            MEM16_BASE[j] = pattern8;
        if (MEM16_BASE[j] != pattern8) {
            hal_psram_log_print("Address %x = [%x+%x] or (%x), and %x is expected!\n\r", (U32)(&MEM8_BASE[i]), MEM8_BASE[i], MEM8_BASE[i + 1], MEM16_BASE[i], pattern8);
            hal_psram_log_print("....Read Bytes and Write half word test: Fail!\n\r");
            _errorExit(4);
        }
        pattern8 += 2;
    }
    if (i == len)
        hal_psram_log_print("..Read Bytes and Write half word test (0x00 ~ 0xff): Pass!\n\r");

    /* === Verify pattern 3 (0x0000, 0x0001, 0x0002, ... 0xFFFF) === */

    pattern16 = 0x00;
    for (i = 0; i < (len >> 1); i++)
        MEM16_BASE[i] = pattern16++;

    pattern16 = 0x00;
    for (i = 0; i < (len >> 1); i++) {
        if (MEM16_BASE[i] != pattern16++) {
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM16_BASE[i]), MEM16_BASE[i], --pattern16);
            hal_psram_log_print("....16bits 0x00~0xffff pattern test: Fail!\n\r");
            _errorExit(5);
        }
    }
    if (i == (len >> 1))
        hal_psram_log_print("..16bits 0x00~0xffff pattern test: Pass!\n\r");


    /* === Verify pattern 4 (0x00000000, 0x00000001, 0x00000002, ... 0xFFFFFFFF) === */

    pattern32 = 0x00;
    for (i = 0; i < (len >> 2); i++)
        MEM32_BASE[i] = pattern32++;
    pattern32 = 0x00;
    for (i = 0; i < (len >> 2); i++) {
        if (MEM32_BASE[i] != pattern32++) {
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i], --pattern32);
            hal_psram_log_print("....32bits 0x00~0xffffffff pattern test: Fail!\n\r");
            _errorExit(6);
        }
    }
    if (i == (len >> 2))
        hal_psram_log_print("..32bits 0x00~0xffffffff pattern test: Pass!\n\r");


    /* === Pattern 5: Filling memory range with 0x44332211 === */

    for (i = 0; i < size; i++)
        MEM32_BASE[i] = 0x44332211;

    /* === Read Check then Fill Memory with a5a5a5a5 Pattern === */
    for (i = 0; i < size; i++) {
        if (MEM32_BASE[i] != 0x44332211) {
            hal_psram_log_print("Address %x = %x, 0x44332211 is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i]);
            hal_psram_log_print("....0x44332211 pattern test: Fail!\n\r");
            _errorExit(7);
        } else {
            MEM32_BASE[i] = 0xa5a5a5a5;
        }
    }

    if (i == size)
        hal_psram_log_print("..0x44332211 pattern test: Pass!\n\r");


    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 0h === */
    for (i = 0; i < size; i++) {
        if (MEM32_BASE[i] != 0xa5a5a5a5) {
            hal_psram_log_print("Address %x = %x, 0xa5a5a5a5 is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i]);
            hal_psram_log_print("....0xa5a5a5a5 pattern test: Fail!\n\r");
            _errorExit(8);
        } else {
            MEM8_BASE[i * 4] = 0x00;
        }
    }
    if (i == size)
        hal_psram_log_print("..0xa5a5a5a5 pattern test: Pass!\n\r");

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 2h === */

    for (i = 0; i < size; i++) {
        if (MEM32_BASE[i] != 0xa5a5a500) {
            hal_psram_log_print("Address %x = %x, 0xa5a5a500 is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i]);
            hal_psram_log_print("....0xa5a5a500 pattern test: Fail!\n\r");
            _errorExit(9);
        } else {
            MEM8_BASE[i * 4 + 2] = 0x00;
        }
    }
    if (i == size)
        hal_psram_log_print("..0xa5a5a500 pattern test: Pass!\n\r");


    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 1h === */

    for (i = 0; i < size; i++) {
        if (MEM32_BASE[i] != 0xa500a500) {
            hal_psram_log_print("Address %x = %x, 0xa500a500 is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i]);
            hal_psram_log_print("....0xa500a500 pattern test: Fail!\n\r");
            _errorExit(10);
        } else {
            MEM8_BASE[i * 4 + 1] = 0x00;
        }
    }
    if (i == size)
        hal_psram_log_print("..0xa500a500 pattern test: Pass!\n\r");


    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 3h === */

    for (i = 0; i < size; i++) {
        if (MEM32_BASE[i] != 0xa5000000) {
            hal_psram_log_print("Address %x = %x, 0xa5000000 is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i]);
            hal_psram_log_print("....0xa5000000 pattern test: Fail!\n\r");
            _errorExit(11);
        } else {
            MEM8_BASE[i * 4 + 3] = 0x00;
        }
    }
    if (i == size)
        hal_psram_log_print("..0xa5000000 pattern test: Pass!\n\r");


    /* === Read Check then Fill Memory with ffff Word Pattern at offset 1h === */

    for (i = 0; i < size; i++) {
        if (MEM32_BASE[i] != 0x00000000) {
            hal_psram_log_print("Address %x = %x, 0x00000000 is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i]);
            hal_psram_log_print("....0x00000000 pattern test: Fail!\n\r");
            _errorExit(12);
        } else {
            MEM16_BASE[i * 2 + 1] = 0xffff;
        }
    }
    if (i == size)
        hal_psram_log_print("..0x00000000 pattern test: Pass!\n\r");


    /* === Read Check then Fill Memory with ffff Word Pattern at offset 0h === */

    for (i = 0; i < size; i++) {
        if (MEM32_BASE[i] != 0xffff0000) {
            hal_psram_log_print("Address %x = %x, 0xffff0000 is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i]);
            hal_psram_log_print("....0xffff0000 pattern test: Fail!\n\r");
            _errorExit(13);
        } else {
            MEM16_BASE[i * 2] = 0xffff;
        }
    }
    if (i == size)
        hal_psram_log_print("..0xffff0000 pattern test: Pass!\n\r");


    /*===  Read Check === */

    for (i = 0; i < size; i++) {
        if (MEM32_BASE[i] != 0xffffffff) {
            hal_psram_log_print("Address %x = %x, 0xffffffff is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i]);
            hal_psram_log_print("....0xffffffff pattern test: Fail!\n\r");
            _errorExit(14);
        }
    }
    if (i == size)
        hal_psram_log_print("..0xffffffff pattern test: Pass!\n\r");


    /************************************************
    * Additional verification
    ************************************************/

    /* === stage 1 => write 0 === */

    for (i = 0; i < size; i++) {
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 2 => read 0, write 0xF === */

    for (i = 0; i < size; i++) {
        value = MEM_BASE[i];

        if (value != PATTERN1) {
            hal_psram_log_print("\nStage 2 error. Addr = %x, value = %x\n", (U32)(&(MEM_BASE[i])), value);
            _errorExit(15);
        }
        MEM_BASE[i] = PATTERN2;
    }


    /* === stage 3 => read 0xF, write 0 === */

    for (i = 0; i < size; i++) {
        value = MEM_BASE[i];
        if (value != PATTERN2) {
            hal_psram_log_print("\nStage 3 error. Addr = %x, value = %x\n", (U32)(&(MEM_BASE[i])), value);
            _errorExit(16);
        }
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 4 => read 0, write 0xF === */

    for (i = 0; i < size; i++) {
        value = MEM_BASE[i];
        if (value != PATTERN1) {
            hal_psram_log_print("\nStage 4 error. Addr = %x, value = %x\n", (U32)(&(MEM_BASE[i])), value);
            _errorExit(17);
        }
        MEM_BASE[i] = PATTERN2;
    }


    /* === stage 5 => read 0xF, write 0 === */

    for (i = 0; i < size; i++) {
        value = MEM_BASE[i];
        if (value != PATTERN2) {
            hal_psram_log_print("\nStage 5 error. Addr = %x, value = %x\n", (U32)(&(MEM_BASE[i])), value);
            _errorExit(18);
        }
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 6 => read 0 === */

    for (i = 0; i < size; i++) {
        value = MEM_BASE[i];
        if (value != PATTERN1) {
            hal_psram_log_print("\nStage 6 error. Addr = %x, value = %x\n", (U32)(&(MEM_BASE[i])), value);
            _errorExit(19);
        }
    }

    hal_psram_log_print("..%x and %x Interleaving test: Pass!\n\r", PATTERN1, PATTERN2);

    /* === 1/2/4-byte combination test === */

    i = (U32)MEM_BASE;
    while (i < (U32)MEM_BASE + (size << 2)) {
        *((U8 *)i) = 0x78;
        i += 1;
        *((U8 *)i) = 0x56;
        i += 1;
        *((U16 *)i) = 0x1234;
        i += 2;
        *((U32 *)i) = 0x12345678;
        i += 4;
        *((U16 *)i) = 0x5678;
        i += 2;
        *((U8 *)i) = 0x34;
        i += 1;
        *((U8 *)i) = 0x12;
        i += 1;
        *((U32 *)i) = 0x12345678;
        i += 4;
        *((U8 *)i) = 0x78;
        i += 1;
        *((U8 *)i) = 0x56;
        i += 1;
        *((U16 *)i) = 0x1234;
        i += 2;
        *((U32 *)i) = 0x12345678;
        i += 4;
        *((U16 *)i) = 0x5678;
        i += 2;
        *((U8 *)i) = 0x34;
        i += 1;
        *((U8 *)i) = 0x12;
        i += 1;
        *((U32 *)i) = 0x12345678;
        i += 4;
    }

    for (i = 0; i < size; i++) {
        value = MEM_BASE[i];
        if (value != 0x12345678) {
            hal_psram_log_print("\nCombined byte/2-byte/4-byte error. Addr = %x, value = %x\n", (U32)(&(MEM_BASE[i])), value);
            _errorExit(20);
        }
    }
    hal_psram_log_print("..Combined byte/2-byte/4-byte test: Pass!\n\r");

    /* === Verify pattern 1 (0x00~0xff) - write/pre-read test === */

    pattern8 = 0x00;
    MEM8_BASE[0] = pattern8;
    for (i = 0; i < size * 4; i++) {
        if (i < size * 4 - 1)
            MEM8_BASE[i + 1] = pattern8 + 1;
        if (MEM8_BASE[i] != pattern8) {
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM8_BASE[i]), MEM8_BASE[i], pattern8);
            hal_psram_log_print("....8bits 0x00~0xff write/pre-read test: Fail!\n\r");
            _errorExit(21);
        }
        pattern8++;
    }
    if (i == size * 4)
        hal_psram_log_print("..8bits 0x00~0xff write/pre-read test: Pass!\n\r");


    /* === Verify pattern 2 (0x00~0xffff) - write/pre-read test === */

    pattern16 = 0x00;
    MEM16_BASE[0] = pattern16;
    for (i = 0; i < size * 2; i++) {
        if (i < size * 2 - 1)
            MEM16_BASE[i + 1] = pattern16 + 1;
        if (MEM16_BASE[i] != pattern16) {
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM16_BASE[i]), MEM16_BASE[i], pattern16);
            hal_psram_log_print("....16bits 0x00~0xffff write/pre-read test: Fail!\n\r");
            _errorExit(22);
        }
        pattern16++;
    }
    if (i == size * 2)
        hal_psram_log_print("..16bits 0x00~0xffff write/pre-read test: Pass!\n\r");


    /* === Verify pattern 3 (0x00~0xffffffff) - write/pre-read test === */

    pattern32 = 0x00;
    MEM32_BASE[0] = pattern32;
    for (i = 0; i < size; i++) {
        if (i < size - 1)
            MEM32_BASE[i + 1] = pattern32 + 1;
        if (MEM32_BASE[i] != pattern32) {
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i], pattern32);
            hal_psram_log_print("....32bits 0x00~0xffffffff write/pre-read  test: Fail!\n\r");
            _errorExit(23);
        }
        pattern32++;
    }
    if (i == size)
        hal_psram_log_print("..32bits 0x00~0xffffffff write/pre-read test: Pass!\n\r");

    /* === Verify pattern 4 (0x00~0xffffffffffffffff) - write/pre-read test === */

    pattern64 = 0x00;
    MEM64_BASE[0] = pattern64;
    for (i = 0; i < size / 2; i++) {
        if (i < size / 2 - 1)
            MEM64_BASE[i + 1] = pattern64 + 1;
        if (MEM64_BASE[i] != pattern64) {
            hal_psram_log_print("Address %x = [0x%x][%x], [0x%x][%x] is expected!\n\r", (U32)(&MEM64_BASE[i]), U64_high32_value(MEM64_BASE[i]), U64_low32_value(MEM64_BASE[i]), U64_high32_value(pattern64), U64_low32_value(pattern64));
            hal_psram_log_print("....64bits 0x00~0xffffffffffffffff write/pre-read  test: Fail!\n\r");
            _errorExit(24);
        }
        pattern64++;
    }
    if (i == size / 2)
        hal_psram_log_print("..64bits 0x00~0xffffffffffffffff write/pre-read test: Pass!\n\r");

    /* === Verify pattern 1 (0x00~0xff) - write write read read test === */

    pattern8 = 0x00;
    MEM8_BASE[0] = pattern8;
    MEM8_BASE[1] = pattern8 + 1;
    for (i = 0; i < size * 4; i += 2) {
        if (i < size * 4 - 2) {
            MEM8_BASE[i + 2] = pattern8 + 2;
            MEM8_BASE[i + 3] = pattern8 + 3;
        }
        if ((MEM8_BASE[i] != pattern8) || (MEM8_BASE[i + 1] != (pattern8 + 1))) {
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM8_BASE[i]), MEM8_BASE[i], pattern8);
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM8_BASE[i + 1]), MEM8_BASE[i + 1], pattern8 + 1);
            hal_psram_log_print("....8bits 0x00~0xff write/write/pre-read/pre-read test: Fail!\n\r");
            _errorExit(25);
        }
        pattern8 += 2;
    }
    if (i == size * 4)
        hal_psram_log_print("..8bits 0x00~0xff write/write/pre-read/pre-read test: Pass!\n\r");


    /* === Verify pattern 2 (0x00~0xffff) - write write read read test === */

    pattern16 = 0x00;
    MEM16_BASE[0] = pattern16;
    MEM16_BASE[1] = pattern16 + 1;
    for (i = 0; i < size * 2; i += 2) {
        if (i < size * 2 - 2) {
            MEM16_BASE[i + 2] = pattern16 + 2;
            MEM16_BASE[i + 3] = pattern16 + 3;
        }

        if ((MEM16_BASE[i] != pattern16) || (MEM16_BASE[i + 1] != (pattern16 + 1))) {
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM16_BASE[i]), MEM16_BASE[i], pattern16);
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM16_BASE[i + 1]), MEM16_BASE[i + 1], pattern16 + 1);
            hal_psram_log_print("....16bits 0x00~0xffff write/write/pre-read/pre-read test: Fail!\n\r");
            _errorExit(26);
        }
        pattern16 += 2;
    }
    if (i == size * 2)
        hal_psram_log_print("..16bits 0x00~0xffff write/write/pre-read/pre-read test: Pass!\n\r");


    /* === Verify pattern 3 (0x00~0xffffffff) - write write read read test === */

    pattern32 = 0x00;
    MEM32_BASE[0] = pattern32;
    MEM32_BASE[1] = pattern32 + 1;
    for (i = 0; i < size; i += 2) {
        if (i < size - 2) {
            MEM32_BASE[i + 2] = pattern32 + 2;
            MEM32_BASE[i + 3] = pattern32 + 3;
        }
        if ((MEM32_BASE[i] != pattern32) || (MEM32_BASE[i + 1] != (pattern32 + 1))) {
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM32_BASE[i]), MEM32_BASE[i], pattern32);
            hal_psram_log_print("Address %x = %x, %x is expected!\n\r", (U32)(&MEM32_BASE[i + 1]), MEM32_BASE[i + 1], pattern32 + 1);
            hal_psram_log_print("....32bits 0x00~0xffffffff write/write/pre-read/pre-read  test: Fail!\n\r");
            _errorExit(27);
        }
        pattern32 += 2;
    }
    if (i == size)
        hal_psram_log_print("..32bits 0x00~0xffffffff write/write/pre-read/pre-read test: Pass!\n\r");

    /* === Verify pattern 4 (0x00~0xffffffffffffffff) - write write read read test === */

    pattern64 = 0x00;
    MEM64_BASE[0] = pattern64;
    MEM64_BASE[1] = pattern64 + 1;
    for (i = 0; i < size / 2; i += 2) {
        if (i < size / 2 - 2) {
            MEM64_BASE[i + 2] = pattern64 + 2;
            MEM64_BASE[i + 3] = pattern64 + 3;
        }
        if ((MEM64_BASE[i] != pattern64) || (MEM64_BASE[i + 1] != (pattern64 + 1))) {
            hal_psram_log_print("Address %x = [0x%x][%x], [0x%x][%x] is expected!\n\r", (U32)(&MEM64_BASE[i]), U64_high32_value(MEM64_BASE[i]), U64_low32_value(MEM64_BASE[i]), U64_high32_value(pattern64), U64_low32_value(pattern64));
            hal_psram_log_print("Address %x = [0x%x][%x], [0x%x][%x] is expected!\n\r", (U32)(&MEM64_BASE[i + 1]), U64_high32_value(MEM64_BASE[i + 1]), U64_low32_value(MEM64_BASE[i + 1]), U64_high32_value(pattern64 + 1), U64_low32_value(pattern64 + 1));
            hal_psram_log_print("....64bits 0x00~0xffffffffffffffff write/write/pre-read/pre-read  test: Fail!\n\r");
            _errorExit(28);
        }
        pattern64 += 2;
    }
    if (i == size / 2)
        hal_psram_log_print("..64bits 0x00~0xffffffffffffffff write/write/pre-read/pre-read test: Pass!\n\r");

    return 0;
}

void psram_write_read_test(void)
{
    unsigned int pass_cnt = 0;
    unsigned int fail_cnt = 0;

    hal_psram_log_print("psram write read test start !!! \n");
    while (1) {
        hal_psram_log_print("\nthis is %d test, Pass[%d], Fail[%d]!!! \n", (pass_cnt + fail_cnt + 1), pass_cnt, fail_cnt);
        if (memory_read_write_test(PSRAM_MEM_TEST_START, psram_size_get()) < 0) {
            fail_cnt++;
            hal_psram_log_print("psram CPU write read test fail!!! \n");
            break;
        }
        hal_psram_log_print("psram CPU write read test Pass!!! \n");

        pass_cnt++;
    }
    hal_psram_log_print("psram write read test end !!! \n");
}
#endif /* #ifdef PSRAM_WRITE_READ_TEST */

static U8 check_nonuhs_is_initial(void)
{
    uint32_t CR_addr_tmp = NONUHS_PSRAM_CTRL_DEBUG_CTRL_2;

    if (get_chip_version() == CHIP_HW_VER_EX) {
        CR_addr_tmp = NONUHS_PSRAM_RESERVE_RG;
    }

    return ((HAL_REG_32(CR_addr_tmp)) & 0x1);
}

ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME static void set_nonuhs_is_initial(void)
{
    uint32_t CR_addr_tmp = NONUHS_PSRAM_CTRL_DEBUG_CTRL_2;

    if (get_chip_version() == CHIP_HW_VER_EX) {
        CR_addr_tmp = NONUHS_PSRAM_RESERVE_RG;
    }

    HAL_REG_32(CR_addr_tmp) = (HAL_REG_32(CR_addr_tmp)) | (0x1 << 0);
}

ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME static void clr_nonuhs_is_initial(void)
{
    uint32_t CR_addr_tmp = NONUHS_PSRAM_CTRL_DEBUG_CTRL_2;

    if (get_chip_version() == CHIP_HW_VER_EX) {
        CR_addr_tmp = NONUHS_PSRAM_RESERVE_RG;
    }

    HAL_REG_32(CR_addr_tmp) = (HAL_REG_32(CR_addr_tmp)) & (~(0x1 << 0));
}

U8 check_is_initial(void)
{
    uint32_t CR_addr_tmp = UHS_PSRAM_RESERVE_RG;
    return ((HAL_REG_32(CR_addr_tmp) >> 31) & 0x1);
}
void set_is_initial(void)
{
    uint32_t CR_addr_tmp = UHS_PSRAM_RESERVE_RG;
    HAL_REG_32(CR_addr_tmp) = (HAL_REG_32(CR_addr_tmp)) | (0x1 << 31);
}

U8 check_psram_initial_done(PSRAM_MODE_ENUM psram_type)
{
    if ((PSRAM_MODE_PSRAM_4MB_APM == psram_type) || (PSRAM_MODE_PSRAM_8MB_APM == psram_type) ||
        (PSRAM_MODE_PSRAM_4MB_WB == psram_type) || (PSRAM_MODE_PSRAM_8MB_WB == psram_type)) {
        if (check_nonuhs_is_initial() == 0x1) {
            return 1;
        }
    } else if ((PSRAM_MODE_UHS_PSRAM_8MB == psram_type) || (PSRAM_MODE_UHS_PSRAM_16MB == psram_type)) {
        if (check_is_initial() == 0x1) {
            return 1;
        }
    }

    return 0;
}

void set_psram_initial_done(PSRAM_MODE_ENUM psram_type)
{
    if ((PSRAM_MODE_PSRAM_4MB_APM == psram_type) || (PSRAM_MODE_PSRAM_8MB_APM == psram_type) ||
        (PSRAM_MODE_PSRAM_4MB_WB == psram_type) || (PSRAM_MODE_PSRAM_8MB_WB == psram_type)) {
        set_nonuhs_is_initial();
    } else if ((PSRAM_MODE_UHS_PSRAM_8MB == psram_type) || (PSRAM_MODE_UHS_PSRAM_16MB == psram_type)) {
        set_is_initial();
    }
}

ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME eCHIP_VERSION get_chip_version(void)
{
    eCHIP_VERSION rst = CHIP_HW_VER_E1;//default E1

    if (0x8A00 == hal_boot_get_hw_ver()) {
        rst = CHIP_HW_VER_E1;
    } else {
        rst = CHIP_HW_VER_EX;
    }

    return rst;
}

ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME static void psram_efuse_mode_to_AO_RG(void)
{
    // Read the reboot status from AO register
    bool is_reboot = ((HAL_REG_32(WAKEUP_AO_REG) & WAKEUP_REBOOT_STATUS) != 0);

    // Read the psram mode value from efuse group 2 and fill into AO register on coldboot
    if (!is_reboot) {
        hal_efuse_psram_mode_t efuse_mode = EFUSE_PSRAM_MODE_VALUE(HAL_REG_32(EFUSE_GRP2_BLK_0));
        U32 psram_mode;
        switch (efuse_mode) {
            case HAL_EFUSE_PSRAM_MODE_4MB_APM_NONUHS:
                psram_mode = PSRAM_MODE_PSRAM_4MB_APM;
                break;
            case HAL_EFUSE_PSRAM_MODE_8MB_APM_NONUHS:
                psram_mode = PSRAM_MODE_PSRAM_8MB_APM;
                break;
            case HAL_EFUSE_PSRAM_MODE_8MB_UHS:
                psram_mode = PSRAM_MODE_UHS_PSRAM_8MB;
                break;
            case HAL_EFUSE_PSRAM_MODE_16MB_UHS:
                psram_mode = PSRAM_MODE_UHS_PSRAM_16MB;
                break;
            case HAL_EFUSE_PSRAM_MODE_4MB_WB_NONUHS:
                psram_mode = PSRAM_MODE_PSRAM_4MB_WB;
                break;
            case HAL_EFUSE_PSRAM_MODE_8MB_WB_NONUHS:
                psram_mode = PSRAM_MODE_PSRAM_8MB_WB;
                break;
            case HAL_EFUSE_PSRAM_MODE_RESERVED:
                psram_mode = PSRAM_MODE_RESERVED;
                break;
            case HAL_EFUSE_PSRAM_MODE_NO_PSRAM:
                psram_mode = PSRAM_MODE_NONE;
                break;
            default:
                hal_psram_log_print("ERROR: Wrong efuse mode = %d \n", efuse_mode);
                break;
        }

        HAL_REG_32(WAKEUP_AO_REG) |= (psram_mode << WAKEUP_PSRAM_MODE_SHIFT);

#ifdef HAL_SECURITY_MODULE_ENABLE
        if (CHIP_HW_VER_E1 == get_chip_version()) {
            // 7933 E1 Workaround: Add PSRAM configuration setting backup restore in security setting flow
            // Overwrite empty security setting init table
            // This security backup/restore will be called by UHS and Non-UHS PSRAM, must ensure that the CR to be load/store matches
            // PSRAM corresponding type
            extern struct hal_ls_module g_hal_security_module_mpu_psram[];
            ASSERT(g_hal_security_module_mpu_psram[0].init_count == 0);

            if (psram_mode == PSRAM_MODE_PSRAM_4MB_APM || psram_mode == PSRAM_MODE_PSRAM_8MB_APM ||
                psram_mode == PSRAM_MODE_PSRAM_4MB_WB || psram_mode == PSRAM_MODE_PSRAM_8MB_WB) {
                g_hal_security_module_mpu_psram[0].init_count = (sizeof(spm_security_psram_nonuhs_ls_cr_entries) / sizeof(struct hal_ls_register));
                g_hal_security_module_mpu_psram[0].init_register = &spm_security_psram_nonuhs_ls_cr_entries[0];
            } else if (psram_mode == PSRAM_MODE_UHS_PSRAM_8MB || psram_mode == PSRAM_MODE_UHS_PSRAM_16MB) {
                g_hal_security_module_mpu_psram[0].init_count = (sizeof(spm_security_psram_uhs_ls_cr_entries) / sizeof(struct hal_ls_register));
                g_hal_security_module_mpu_psram[0].init_register = &spm_security_psram_uhs_ls_cr_entries[0];
            }
        }
#endif /* #ifdef HAL_SECURITY_MODULE_ENABLE */
    }
}

ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME PSRAM_MODE_ENUM psram_type_get_from_AO_RG(void)
{
    uint32_t data_tmp = 0;
    PSRAM_MODE_ENUM type = PSRAM_MODE_NONE;

    data_tmp = HAL_REG_32(AO_RG_ADDR);
    data_tmp = (data_tmp >> 2) & 0x00000007;

    switch (data_tmp) {
        case 0:
            type = PSRAM_MODE_PSRAM_4MB_APM;
            break;
        case 1:
            type = PSRAM_MODE_PSRAM_8MB_APM;
            break;
        case 2:
            type = PSRAM_MODE_UHS_PSRAM_8MB;
            break;
        case 3:
            type = PSRAM_MODE_UHS_PSRAM_16MB;
            break;
        case 4:
            type = PSRAM_MODE_PSRAM_4MB_WB;
            break;
        case 5:
            type = PSRAM_MODE_PSRAM_8MB_WB;
            break;
        case 6:
            type = PSRAM_MODE_RESERVED;
            break;
        case 7:
            type = PSRAM_MODE_NONE;
            break;
        default:
            break;

    }

    return type;

}

U32 psram_size_get(void)
{
    PSRAM_MODE_ENUM psram_type = psram_type_get_from_AO_RG();

    switch (psram_type) {
        case PSRAM_MODE_PSRAM_4MB_APM:
        case PSRAM_MODE_PSRAM_4MB_WB:
            return 0x400000;
        case PSRAM_MODE_PSRAM_8MB_APM:
        case PSRAM_MODE_PSRAM_8MB_WB:
        case PSRAM_MODE_UHS_PSRAM_8MB:
            return 0x800000;
        case PSRAM_MODE_UHS_PSRAM_16MB:
            return 0x1000000;
        default:
            hal_psram_log_print("[non_uhs_psram_mbist_psram_size_get] psram type wrong = %d \n", psram_type);
            return 0x400000;
    }
}

ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME static void psram_mux_sw(int is_uhs_psram)
{
    /*TINFO ="PSMRA_MUX switch" */
    if (is_uhs_psram)
        HAL_REG_32(TOP_CFG_AON_BASE_ADDR + 0x0020) &= ~(BIT(31));
    else
        HAL_REG_32(TOP_CFG_AON_BASE_ADDR + 0x0020) |= BIT(31);
}

//========================================================================
//  NON-UHS PSRAM Low power half sleep
//========================================================================
ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME int nonuhs_psram_low_pwr_half_slp(PSRAM_MODE_ENUM mode)
{
    if (mode > PSRAM_MODE_PSRAM_8MB_WB) {
        hal_psram_log_suspend_resume("unsupport psram mode,return! \n");
        return HAL_PSRAM_STATUS_FAIL;
    }

    uint32_t data, data_mod;
    uint32_t __start = 0, __dur = 0;

    data = HAL_REG_32(ARB_CFG_ADDR);
    data_mod = data | 0x00000010;    /* Block AXI vld in case of data transferring during HS mode. */
    HAL_REG_32(ARB_CFG_ADDR) = data_mod;
    /* TINFO ="Block AXI vld" */

    if (mode == PSRAM_MODE_PSRAM_4MB_WB) {
        HAL_REG_32(PSRAM_TIMING_2_ADDR) = 0x260; // t_fsm_csh (3us)
        //                          csn_low_clk_en, csh_ckon_en, cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,     cmd1,       cmd0,      data3,      data2,      data1,      data0
        NONUHS_REG_CMD_WR_NO_BLOCK(0x0,  0x0,  0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x01006000, 0x00000001, 0x00000020, 0x00000000, 0x00000000, 0x00000000);
    } else if (mode == PSRAM_MODE_PSRAM_8MB_WB) {
        HAL_REG_32(PSRAM_TIMING_2_ADDR) = 0x260;           // t_fsm_csh (3us)
        //                          csn_low_clk_en, csh_ckon_en, cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,     cmd1,       cmd0,      data3,      data2,      data1,      data0
        NONUHS_REG_CMD_WR_NO_BLOCK(0x0, 0x0,  0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x01006000, 0x00000001, 0x00000020, 0x00000000, 0x00000000, 0x00000000);
    } else if (mode == PSRAM_MODE_PSRAM_8MB_APM) {
        HAL_REG_32(PSRAM_TIMING_2_ADDR) = 0x318; /* t_fsm_csh (3us) */
        //                          csn_low_clk_en, csh_ckon_en, cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,     cmd1,       cmd0,      data3,      data2,      data1,      data0
        NONUHS_REG_CMD_WR_NO_BLOCK(0x0, 0x0,  0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0000c0c0, 0x00000006, 0x0000f000, 0x00000000, 0x00000000, 0x00000000);
    } else if (mode == PSRAM_MODE_PSRAM_4MB_APM) {
        HAL_REG_32(PSRAM_TIMING_2_ADDR) = 0x318;           // t_fsm_csh (3us)
        //                          csn_low_clk_en, csh_ckon_en, cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,     cmd1,       cmd0,      data3,      data2,      data1,      data0
        if (get_chip_version() == CHIP_HW_VER_E1) {
            NONUHS_REG_CMD_WR_NO_BLOCK(0x0, 0x0,  0x1,   0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x2,   0x1,   0x1,    0x0,    0x1, 0x0006c000, 0x00000000, 0x0000f000, 0x00000000, 0x00000000, 0x00000000);
        } else {
            NONUHS_REG_CMD_WR_NO_BLOCK(0x1, 0x1,  0x1,   0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x2,   0x1,   0x1,    0x0,    0x1, 0x0006c000, 0x00000000, 0x0000f000, 0x00000000, 0x00000000, 0x00000000);
        }
    }

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__start);
    while ((((HAL_REG_32(PSRAM_CTRL_STUS_ADDR)) & 0x00000001) == 0x00000000) && __dur <= (NON_UHS_LOW_POWER_SLP_TIMEOUT_US)) {
        uint32_t __now = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__now);
        hal_gpt_get_duration_count(__start, __now, &__dur);
    }

    if (__dur > NON_UHS_LOW_POWER_SLP_TIMEOUT_US) {
        hal_psram_log_suspend_resume("nonuhs-psram enter HS mode timeOut, return!\n");
        return HAL_PSRAM_STATUS_FAIL;
    }
    /* TINFO ="enter HS mode" */

    clr_nonuhs_is_initial();

    return HAL_PSRAM_STATUS_SUCCESS;
}

/*========================================================================
  NON-UHS PSRAM Low power wake-up initialization
  ========================================================================*/
ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME void nonuhs_psram_low_pwr_wake_up(PSRAM_MODE_ENUM mode)
{
    uint32_t __start = 0, __dur = 0;
    struct hal_ls_psram_e2_table *p_non_uhs_psram_e2_settings = (struct hal_ls_psram_e2_table *)hal_ls_psram_param_addr_get();

    if (p_non_uhs_psram_e2_settings == NULL) {
        hal_psram_log_suspend_resume("p_non_uhs_psram_e2_settings = NULL, please check! \n");
        return;
    }

    /*DLL initialize ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
    /*TINFO ="PSRAM ctrl DLL init start." */
    HAL_REG_32(DLL_CTRL_CFG_STUS_ADDR) = 0x00000f05;    /* enable dll_cal_init to calibrate DLL */

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__start);
    while ((((HAL_REG_32(DLL_CTRL_CFG_STUS_ADDR)) & 0x00010000) == 0x00000000) && __dur <= (NON_UHS_LOW_POWER_WAKEUP_TIMEOUT_US)) {
        uint32_t __now = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__now);
        hal_gpt_get_duration_count(__start, __now, &__dur);
    }

    if (__dur > NON_UHS_LOW_POWER_WAKEUP_TIMEOUT_US) {
        hal_psram_log_suspend_resume("nonuhs-psram wait Calibration done timeOut, return!\n");
        return;
    }
    /*TINFO ="Calibration done." */

    HAL_REG_32(DLL_CTRL_CFG_STUS_ADDR) = 0x00000f07;    /* enable dll_soft_upd to update DLL value into PSRAM PHY */
    hal_gpt_delay_us(1);
    HAL_REG_32(DLL_CTRL_CFG_STUS_ADDR) = 0x00000f01;    /* deassert dll_cal_init & dll_soft_upd to return FSM back to idle. The DLL cal will be executed in each specified period. */

    /*[E] DLL initialize -----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
    /*TINFO ="PSMRA_MUX switch" */
    psram_mux_sw(0);

    if ((p_non_uhs_psram_e2_settings->p_psram_CR_1st != NULL) && (p_non_uhs_psram_e2_settings->p_psram_CR_2nd != NULL)
        && (p_non_uhs_psram_e2_settings->p_para_array != NULL)) {
        struct hal_psram_CR *_CR_1st = p_non_uhs_psram_e2_settings->p_psram_CR_1st;
        struct hal_psram_CR *_CR_2nd = p_non_uhs_psram_e2_settings->p_psram_CR_2nd;
        int idx = 0;

        while (idx < p_non_uhs_psram_e2_settings->CR_1st_count) {
            if (((_CR_1st[idx].addr) & 0xFFFF0000) != PSRAM_CTRL_TOP_BASE_ADDR) {
                hal_psram_log_suspend_resume("nonuhs_psram_low_pwr_wake_up fail, wrong base address [0x%x], return!\n", (unsigned int)(_CR_1st[idx].addr));
                return;
            }
            HAL_REG_32(_CR_1st[idx].addr) = _CR_1st[idx].value;
            idx++;
        }

        uint32_t *p_para = p_non_uhs_psram_e2_settings->p_para_array;
        NONUHS_REG_CMD_WR_NO_BLOCK(p_para[0], p_para[1], p_para[2], p_para[3], p_para[4], p_para[5], p_para[6], p_para[7], p_para[8],
                                   p_para[9], p_para[10], p_para[11], p_para[12], p_para[13], p_para[14], p_para[15], p_para[16], p_para[17], p_para[18], p_para[19]);

        __start = 0;
        __dur = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__start);
        while ((((HAL_REG_32(PSRAM_CTRL_STUS_ADDR)) & 0x00000001) == 0x00000000) && __dur <= (NON_UHS_LOW_POWER_WAKEUP_TIMEOUT_US)) {
            uint32_t __now = 0;
            hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__now);
            hal_gpt_get_duration_count(__start, __now, &__dur);
        }

        if (__dur > NON_UHS_LOW_POWER_WAKEUP_TIMEOUT_US) {
            hal_psram_log_suspend_resume("nonuhs-psram exit HS mode timeOut, return!\n");
            return;
        }
        /* TINFO ="exit HS mode" */

        idx = 0;
        while (idx < p_non_uhs_psram_e2_settings->CR_2nd_count) {
            if (((_CR_2nd[idx].addr) & 0xFFFF0000) != PSRAM_CTRL_TOP_BASE_ADDR) {
                hal_psram_log_suspend_resume("nonuhs_psram_low_pwr_wake_up fail, wrong base address [0x%x], return!\n", (unsigned int)(_CR_2nd[idx].addr));
                return;
            }
            HAL_REG_32(_CR_2nd[idx].addr) = _CR_2nd[idx].value;
            idx++;
        }
    } else {
        hal_psram_log_suspend_resume("Points = NULL, please check, return! \n");
        return;
    }

    hal_gpt_delay_us(1);

    HAL_REG_32(ARB_CFG_ADDR) = HAL_REG_32(ARB_CFG_ADDR) & 0xFFFFFFEF;
    /* TINFO ="disable block AXI vld" */

    set_nonuhs_is_initial();
}

ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME static ePsram_CR_ctl_tbl *get_psram_ctrl_table_id(PSRAM_MODE_ENUM mode, ePsram_CR_ctl_tbl *p_tbl, uint8_t cnt)
{
    ePsram_CR_ctl_tbl *p_tbl_tmp = p_tbl;

    if (p_tbl_tmp == NULL)
        return NULL;

    while (cnt--) {
        if (mode == p_tbl_tmp->mode)
            return p_tbl_tmp;
        p_tbl_tmp++;
    }

    return NULL;
}

ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME static int non_uhs_psram_ls_e1_table_fill(void)
{
    struct hal_psram_CR CR_1st_tmp[] = {
        {0x38070048, 0x00005031},
        {0x38070054, 0x00000063},
        {0x38070058, 0x00001B58}
    };

    struct hal_psram_CR CR_2nd_tmp[] = {
        {0x38070008, 0xA000A03F},
        {0x38070050, 0x02060600},
        {0x38070058, 0x00000008}
    };

    uint32_t para_array_local[PARA_ARRAY_NUM] = {0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x2, 0x0, 0x0, 0x1, 0x1, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

    memset((void *)CR_1st, 0, CR_1ST_MAX_NUM * sizeof(struct hal_psram_CR));
    memset((void *)CR_2nd, 0, CR_2ND_MAX_NUM * sizeof(struct hal_psram_CR));
    memset((void *)para_array, 0, PARA_ARRAY_NUM * sizeof(uint32_t));

    if (sizeof(CR_1st_tmp) / sizeof(struct hal_psram_CR) <= CR_1ST_MAX_NUM) {
        memcpy((void *)CR_1st, (void *)CR_1st_tmp, sizeof(CR_1st_tmp));
        nonuhs_psram_ls_e2_table.CR_1st_count = sizeof(CR_1st_tmp) / sizeof(struct hal_psram_CR);
    } else {
        hal_psram_log_suspend_resume("CR_1st cnt overflow: %d, return! \n", sizeof(CR_1st_tmp) / sizeof(struct hal_psram_CR));
        return -1;
    }

    if (sizeof(CR_2nd_tmp) / sizeof(struct hal_psram_CR) <= CR_2ND_MAX_NUM) {
        memcpy((void *)CR_2nd, (void *)CR_2nd_tmp, sizeof(CR_2nd_tmp));
        nonuhs_psram_ls_e2_table.CR_2nd_count = sizeof(CR_2nd_tmp) / sizeof(struct hal_psram_CR);
    } else {
        hal_psram_log_suspend_resume("CR_2nd cnt overflow: %d, return! \n", sizeof(CR_2nd_tmp) / sizeof(struct hal_psram_CR));
        return -1;
    }

    memcpy((void *)para_array, (void *)para_array_local, (PARA_ARRAY_NUM * sizeof(uint32_t)));
    nonuhs_psram_ls_e2_table.p_psram_CR_1st         = CR_1st;
    nonuhs_psram_ls_e2_table.p_para_array           = para_array;
    nonuhs_psram_ls_e2_table.p_psram_CR_2nd         = CR_2nd;

    return 0;
}

ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME static int non_uhs_psram_ls_e2_table_fill(PSRAM_MODE_ENUM mode)
{
    int i = 0;
    int count_1st = 0;
    int count_2nd = 0;

    ePsram_CR_table psram_CR_table[] = {
        {0x38070044, {0x00000009, 0x00000000, 0x00000000, 0x00000000}},
        {0x38070048, {0x00005031, 0x00005040, 0x00002032, 0x00005042}},
        {0x38070054, {0x000000C6, 0x000000C6, 0x0000002F, 0x0000002F}},
        {0x38070058, {0x000036B0, 0x00004D58, 0x00004F00, 0x00004F00}},

        {0x38070008, {0xA000A03F, 0xA000A07F, 0xA000A03F, 0xA000A07F}},
        {0x38070050, {0x02060600, 0x06060010, 0x05050011, 0x06060011}},
        {0x38070058, {0x00000008, 0x00000007, 0x00000008, 0x00000008}},
        {0x3807005C, {0x00000000, 0x000000C9, 0x00000000, 0x00000000}},
        {0x38070060, {0x00000000, 0x00000192, 0x00000000, 0x00000000}}
    };

    ePsram_CR_ctl_tbl ctrl_table[] = {
        {0, PSRAM_MODE_PSRAM_4MB_APM,  0,  3,  4,  6},
        {1, PSRAM_MODE_PSRAM_8MB_APM,  1,  3,  4,  8},
        {2, PSRAM_MODE_PSRAM_4MB_WB,   1,  3,  4,  6},
        {3, PSRAM_MODE_PSRAM_8MB_WB,   1,  3,  4,  6}
    };

    uint32_t psram_cmd_para[][PARA_ARRAY_NUM] = {
        {0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x2, 0x0, 0x0, 0x1, 0x1, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
        {0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x3, 0x1, 0x0, 0x1, 0x0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
        {0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
        {0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}
    };

    memset((void *)CR_1st, 0, CR_1ST_MAX_NUM * sizeof(struct hal_psram_CR));
    memset((void *)CR_2nd, 0, CR_2ND_MAX_NUM * sizeof(struct hal_psram_CR));
    memset((void *)para_array, 0, PARA_ARRAY_NUM * sizeof(uint32_t));

    ePsram_CR_ctl_tbl *p_ctl_tbl = get_psram_ctrl_table_id(mode, ctrl_table, sizeof(ctrl_table) / sizeof(ePsram_CR_ctl_tbl));
    if (p_ctl_tbl == NULL) {
        hal_psram_log_suspend_resume("not support psram type: %d, return!\n", mode);
        return -1;
    }

    for (i = 0; i < (int)(sizeof(psram_CR_table) / sizeof(ePsram_CR_table)); i++) {
        if ((i >= p_ctl_tbl->start_1st) && (i <= p_ctl_tbl->end_1st)) {
            if ((count_1st < CR_1ST_MAX_NUM) && (count_1st >= 0))  {
                CR_1st[count_1st].addr = psram_CR_table[i].addr;
                CR_1st[count_1st].value = psram_CR_table[i].value[p_ctl_tbl->value_id];
                count_1st++;
            } else {
                hal_psram_log_suspend_resume("CR_1st cnt overflow: %d, return! \n", (count_1st + 1));
                return -1;
            }
        }

        if ((i >= p_ctl_tbl->start_2nd) && (i <= p_ctl_tbl->end_2nd)) {
            if ((count_2nd < CR_2ND_MAX_NUM)  && (count_2nd >= 0)) {
                CR_2nd[count_2nd].addr = psram_CR_table[i].addr;
                CR_2nd[count_2nd].value = psram_CR_table[i].value[p_ctl_tbl->value_id];
                count_2nd++;
            } else {
                hal_psram_log_suspend_resume("CR_2nd cnt overflow: %d, return! \n", (count_2nd + 1));
                return -1;
            }
        }
    }

    memcpy((void *)para_array, (void *)(&(psram_cmd_para[p_ctl_tbl->value_id][0])), (PARA_ARRAY_NUM * sizeof(uint32_t)));
    nonuhs_psram_ls_e2_table.CR_1st_count   = count_1st;
    nonuhs_psram_ls_e2_table.p_psram_CR_1st = CR_1st;
    nonuhs_psram_ls_e2_table.p_para_array   = para_array;
    nonuhs_psram_ls_e2_table.CR_2nd_count   = count_2nd;
    nonuhs_psram_ls_e2_table.p_psram_CR_2nd = CR_2nd;

    return 0;
}

/*============================================================
// NONUHS_REG_CMD_WR_NO_BLOCK
//============================================================*/
/* #define PSRAM_NONUHS_PSRAM_LITTLE_ENDIAN */
ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME static void NONUHS_REG_CMD_WR_NO_BLOCK(uint32_t csn_low_clk_en, uint32_t csh_ckon_en, uint32_t cs_n_sel, uint32_t fsm_cmd_en, uint32_t fsm_lat_en, uint32_t fsm_dat_en, uint32_t fsm_dum_en, uint32_t fsm_csl_en, uint32_t fsm_rst_en, uint32_t cmd_cyc_num, uint32_t dat_cyc_num, uint32_t dum_cyc_num, uint32_t csl_ckoff, uint32_t csh_ckon, uint32_t cmd1, uint32_t cmd0, uint32_t wdata3, uint32_t wdata2, uint32_t wdata1, uint32_t wdata0)
{
    uint32_t tmp;
    uint32_t __start = 0, __dur = 0;

    tmp = (cs_n_sel << 16) + (csn_low_clk_en << 10) + (csh_ckon << 9) + (csl_ckoff << 8) + (csh_ckon_en << 6) + (fsm_dum_en << 5) + (fsm_csl_en << 4) + (fsm_dat_en << 3) + (fsm_lat_en << 2) + (fsm_cmd_en << 1) + fsm_rst_en;
    HAL_REG_32(APB2PSRAM_CUSTOM_CFG_1_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_1_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_1_ADDR, tmp */
    tmp = (dum_cyc_num << 8) + (dat_cyc_num << 4) + (cmd_cyc_num);
    HAL_REG_32(APB2PSRAM_CUSTOM_CFG_2_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_2_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_2_ADDR, tmp */
    tmp = cmd1;
    HAL_REG_32(APB2PSRAM_CUSTOM_CFG_3_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_3_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_3_ADDR, tmp */
    tmp = cmd0;
    HAL_REG_32(APB2PSRAM_CUSTOM_CFG_4_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_4_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_4_ADDR, tmp */

    tmp = wdata3;
    HAL_REG_32(APB2PSRAM_CUSTOM_CFG_5_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_5_ADDR, tmp */
    tmp = wdata2;
    HAL_REG_32(APB2PSRAM_CUSTOM_CFG_6_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_6_ADDR, tmp */
    tmp = wdata1;
    HAL_REG_32(APB2PSRAM_CUSTOM_CFG_7_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_7_ADDR, tmp */
    tmp = wdata0;
    HAL_REG_32(APB2PSRAM_CUSTOM_CFG_8_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_8_ADDR, tmp */

    HAL_REG_32(APB2PSRAM_CUSTOM_CFG_0_ADDR) = 0x00000003;    /* trig command */

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__start);
    while ((((HAL_REG_32(APB2PSRAM_CUSTOM_CFG_0_ADDR)) & 0x00000001) == 0x00000001) && __dur <= (NON_UHS_REG_CMD_WR_NO_BLOCK_TIMEOUT_US)) {
        uint32_t __now = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__now);
        hal_gpt_get_duration_count(__start, __now, &__dur);
    }

    if (__dur > NON_UHS_REG_CMD_WR_NO_BLOCK_TIMEOUT_US) {
        hal_psram_log_suspend_resume("nonuhs-psram waiting for reg_cmd transfer done timeOut, return!\n");
        return;
    }

    hal_gpt_delay_us(1);
    HAL_REG_32(APB2PSRAM_CUSTOM_CFG_0_ADDR) = 0x00000000;
}

void uhs_psram_Sidle_to_S1_SREF(void)
{
    uint32_t data, spm_ba;
    uint32_t __start = 0, __dur = 0;

    spm_ba = SPM_TOP_BASE;
#ifdef HAL_CLOCK_MODULE_ENABLED
    hal_clk_psramaxi_clksel(PSRAMAXI_SRC_XTAL);
    /* TINFO =" [UHS-PSRAM] Sidle to S1 (SREF) start */
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */
    data = 0x01000000;
    HAL_REG_32(spm_ba + 0x0810) = data;
    data = 0x00000002;
    HAL_REG_32(spm_ba + 0x0808) = data;

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__start);
    while ((((HAL_REG_32(spm_ba + 0x0808)) & 0x00000200) != 0x00000200) && __dur <= (UHS_PSRAM_SIDLE_TO_S1_SREF_TIMEOUT_US)) {
        uint32_t __now = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__now);
        hal_gpt_get_duration_count(__start, __now, &__dur);
    }

    if (__dur > UHS_PSRAM_SIDLE_TO_S1_SREF_TIMEOUT_US) {
        hal_psram_log_suspend_resume("nuhs_psram_Sidle_to_S1_SREF timeOut, return!\n");
        return;
    }

    data = 0x00000000;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x00010000;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x00010100;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x00010103;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x00010101;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x00000101;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x00000001;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x00000000;
    HAL_REG_32(spm_ba + 0x0814) = data;

    /* TINFO =" [UHS-PSRAM] Sidle to S1 (SREF) done */
}

void uhs_psram_S1_to_Sidle_SREF(void)
{
    uint32_t data, spm_ba;
    uint32_t __start = 0, __dur = 0;

    spm_ba = SPM_TOP_BASE;
    /* TINFO =" [UHS-PSRAM] S1 to Sidle (SREF) start */
    data = 0x00000001;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x00000101;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x01010103;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x00010101;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x01010101;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x01000103;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x01000100;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x01000000;
    HAL_REG_32(spm_ba + 0x0810) = data;

    hal_gpt_delay_us(1);

    data = 0x00000200;
    HAL_REG_32(spm_ba + 0x0808) = data;

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__start);
    while ((((HAL_REG_32(spm_ba + 0x0808)) & 0x00000200) != 0x00000000) && __dur <= (UHS_PSRAM_S1_TO_SIDLE_SREF_TIMEOUT_US)) {
        uint32_t __now = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__now);
        hal_gpt_get_duration_count(__start, __now, &__dur);
    }

    if (__dur > UHS_PSRAM_S1_TO_SIDLE_SREF_TIMEOUT_US) {
        hal_psram_log_suspend_resume("uhs_psram_S1_to_Sidle_SREF timeOut, return!\n");
        return;
    }

    data = 0x00000100;
    HAL_REG_32(spm_ba + 0x081c) = data;

    data = 0x00000000;
    HAL_REG_32(spm_ba + 0x081c) = data;

    data = 0x00000000;
    HAL_REG_32(spm_ba + 0x0810) = data;
    /* TINFO =" [UHS-PSRAM] S1 to Sidle (SREF) done */

#ifdef HAL_CLOCK_MODULE_ENABLED
    hal_clk_psramaxi_clksel(PSRAMAXI_SRC_PSRAM);
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */
}

void uhs_psram_Sidle_to_S1_HSLEEP(void)
{
    uint32_t data, spm_ba;
    uint32_t __start = 0, __dur = 0;

    spm_ba = SPM_TOP_BASE;
#ifdef HAL_CLOCK_MODULE_ENABLED
    hal_clk_psramaxi_clksel(PSRAMAXI_SRC_XTAL);
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */
    /* TINFO =" [UHS-PSRAM] Sidle to S1 (HALF_SLEEP) start */
    data = 0x01000000;
    HAL_REG_32(spm_ba + 0x0810) = data;
    data = 0x01000000;
    HAL_REG_32(spm_ba + 0x0808) = data;

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__start);
    while ((((HAL_REG_32(spm_ba + 0x0808)) & 0x00000200) != 0x00000200) && __dur <= (UHS_PSRAM_SIDLE_TO_S1_HSLP_TIMEOUT_US)) {
        uint32_t __now = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__now);
        hal_gpt_get_duration_count(__start, __now, &__dur);
    }

    if (__dur > UHS_PSRAM_SIDLE_TO_S1_HSLP_TIMEOUT_US) {
        hal_psram_log_suspend_resume("uhs_psram_Sidle_to_S1_HSLEEP timeOut, return!\n");
        return;
    }

    data = 0x00000000;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x01010000;
    HAL_REG_32(spm_ba + 0x0808) = data;

    data = 0x00010000;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x00010100;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x00010103;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x00010101;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x00000101;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x00000001;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x00000000;
    HAL_REG_32(spm_ba + 0x0814) = data;

    /* TINFO =" [UHS-PSRAM] Sidle to S1 (HALF_SLEEP) done */
}


void uhs_psram_S1_to_Sidle_HSLEEP(void)
{
    U32 data, spm_ba;
    uint32_t __start = 0, __dur = 0;

    spm_ba = SPM_TOP_BASE;
    /* TINFO =" [UHS-PSRAM] S1 to Sidle (HALF_SLEEP) start */
    data = 0x00000001;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x00000101;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x01010103;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x00010101;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x01010101;
    HAL_REG_32(spm_ba + 0x0814) = data;

    data = 0x01000103;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x01000100;
    HAL_REG_32(spm_ba + 0x0810) = data;

    hal_gpt_delay_us(1);

    data = 0x01000000;
    HAL_REG_32(spm_ba + 0x0810) = data;

    data = 0x00010000;
    HAL_REG_32(spm_ba + 0x0808) = data;

    // TODO: delay 100us, time for HS exit
    hal_gpt_delay_us(266);
    data = 0x00000000;
    HAL_REG_32(spm_ba + 0x0808) = data;

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__start);
    while ((((HAL_REG_32(spm_ba + 0x0808)) & 0x00000200) != 0x00000000) && __dur <= (UHS_PSRAM_S1_TO_SIDLE_HSLP_TIMEOUT_US)) {
        uint32_t __now = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__now);
        hal_gpt_get_duration_count(__start, __now, &__dur);
    }

    if (__dur > UHS_PSRAM_S1_TO_SIDLE_HSLP_TIMEOUT_US) {
        hal_psram_log_suspend_resume("uhs_psram_S1_to_Sidle_HSLEEP timeOut, return!\n");
        return;
    }

    data = 0x00000000;
    HAL_REG_32(spm_ba + 0x0810) = data;
#ifdef HAL_CLOCK_MODULE_ENABLED
    hal_clk_psramaxi_clksel(PSRAMAXI_SRC_PSRAM);
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */

    /* TINFO =" [UHS-PSRAM] S1 to Sidle (HALF_SLEEP) done */
}


void uhs_psram_S1_to_S0(void)
{
    uint32_t data, spm_ba;

    spm_ba = SPM_TOP_BASE;

    /* TINFO =" [UHS-PSRAM] S1 to S0 start */
    data = 0x00000000;
    HAL_REG_32(spm_ba + 0x0818) = data;

    //MTCMOS MOVE TO MTCMOS_PWR_DOWN_UHS_PSRAM_OFF
    /* TINFO =" [UHS-PSRAM] S1 to S0 (SREF) done */
}

void uhs_psram_S0_to_S1(void)
{
    U32 data, spm_ba;

    spm_ba = SPM_TOP_BASE;

    /* TINFO =" [UHS-PSRAM] S0 to S1 start */

    //MTCMOS MOVE TO MTCMOS_PWR_ON_UHS_PSRAM_OFF
    data = 0x01000000;
    HAL_REG_32(spm_ba + 0x081c) = data;

    data = 0x01010000;
    HAL_REG_32(spm_ba + 0x081c) = data;

    data = 0x01000000;
    HAL_REG_32(spm_ba + 0x081c) = data;

    data = 0x00000000;
    HAL_REG_32(spm_ba + 0x081c) = data;

    data = 0x01000000;
    HAL_REG_32(spm_ba + 0x0818) = data;

    // TODO: delay 20us, for PLL stablization, enabled by SPM
    hal_gpt_delay_us(60);
    /* TINFO =" [UHS-PSRAM] S0 to S1 done */
}

void asic_mpu_config_psram(void)
{
    if (g_psram_cb != NULL) {
        g_psram_cb();
    }
}

/**
 * @brief     This function gets psram power to wakeup status.
 * @return    #HAL_PSRAM_STATUS_SUCCESS, if the operation is successful.\n
 *            #HAL_PSRAM_STATUS_FAIL, if the operation is failed.
 */
ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME hal_psram_status_t hal_psram_power_wakeup(void)
{
    hal_psram_status_t rst = HAL_PSRAM_STATUS_SUCCESS;
    PSRAM_MODE_ENUM mode = psram_type_get_from_AO_RG();

#ifdef HAL_PSRAM_NON_UHS_ENABLED
    struct hal_ls_psram_e2_table *p_non_uhs_psram_e2_settings = (struct hal_ls_psram_e2_table *)hal_ls_psram_param_addr_get();
#endif /* #ifdef HAL_PSRAM_NON_UHS_ENABLED */
#if defined(HAL_PSRAM_NON_UHS_ENABLED) || defined(HAL_PSRAM_UHS_ENABLED)
    U16 g_psram_wakeup_done = false;
#endif /* #if defined(HAL_PSRAM_NON_UHS_ENABLED) || defined(HAL_PSRAM_UHS_ENABLED) */

    switch (mode) {

#ifdef HAL_PSRAM_NON_UHS_ENABLED
        case PSRAM_MODE_PSRAM_4MB_APM:
        case PSRAM_MODE_PSRAM_8MB_APM:
        case PSRAM_MODE_PSRAM_4MB_WB:
        case PSRAM_MODE_PSRAM_8MB_WB:
            g_psram_wakeup_done = MTCMOS_IS_ON(MTCMOS_STATE_NON_UHS_PSRAM) ? true : false; // non-UHS
            if (g_psram_wakeup_done)
                return rst;
            MTCMOS_PWR_ON_NONUHS_PSRAM;
            if (p_non_uhs_psram_e2_settings) {
                hal_ls_restore(&(p_non_uhs_psram_e2_settings->e1_table)); // non-uhs restore CRs
            } else {
                hal_psram_log_suspend_resume("p_non_uhs_psram_e2_settings = NULL, please check! \n");
                return HAL_PSRAM_STATUS_FAIL;
            }
            nonuhs_psram_low_pwr_wake_up(mode);
            MTCMOS_PWR_ON_PSRAM_AXI_BUS; //last power on
            psram_mux_sw(FALSE);
#ifdef HAL_CLOCK_MODULE_ENABLED
            hal_clock_mux_select(HAL_CLOCK_SEL_PSRAM_AXI, CLK_PSRAM_AXI_CLKSEL_PSRAM);
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */
            if (CHIP_HW_VER_E1 == get_chip_version()) {
                non_uhs_psram_dest_clk = e_CLK_37P5_MHZ;
            } else {
                non_uhs_psram_dest_clk = e_CLK_200_MHZ;
            }
            non_uhs_psram_Set_CLK(non_uhs_psram_Get_dest_CLK());
            break;
#endif /* #ifdef HAL_PSRAM_NON_UHS_ENABLED */
#ifdef HAL_PSRAM_UHS_ENABLED

        case PSRAM_MODE_UHS_PSRAM_8MB:
        case PSRAM_MODE_UHS_PSRAM_16MB:
            g_psram_wakeup_done = MTCMOS_IS_ON(MTCMOS_STATE_UHS_PSRAM) ? true : false; // UHS
            if (g_psram_wakeup_done)
                return rst;
            MTCMOS_PWR_ON_UHS_PSRAM_OFF;
            uhs_psram_S0_to_S1();
            uhs_psram_S1_to_Sidle_HSLEEP();
            MTCMOS_PWR_ON_PSRAM_AXI_BUS;
            SRAM_PWR_ON_UHS_PSRAM;

            uint32_t pemi_nao_ba;
            pemi_nao_ba = 0x380c0000;
            if (mode == PSRAM_MODE_UHS_PSRAM_16MB) {
                HAL_REG_32(pemi_nao_ba + 0x0000) = 0x00002021;
            } else {
                HAL_REG_32(pemi_nao_ba + 0x0000) = 0x00001021;
            }
            HAL_REG_32(pemi_nao_ba + 0x0038) = 0x00000003;
            HAL_REG_32(pemi_nao_ba + 0x0028) = 0x00000000;
            HAL_REG_32(pemi_nao_ba + 0x0060) = 0x00000dff;

            break;
#endif /* #ifdef HAL_PSRAM_UHS_ENABLED */

        default:
            hal_psram_log_suspend_resume("No PSRAM configured \n");
            rst = HAL_PSRAM_STATUS_FAIL;
            break;
    }

    return rst;
}

/**
 * @brief     This function restore psram setting after wakeup.
 * @return    #HAL_PSRAM_STATUS_SUCCESS, if the operation is successful.\n
 *            #HAL_PSRAM_STATUS_FAIL, if the operation is failed.
 */
hal_psram_status_t hal_psram_security_restore_after_wakeup(void)
{
    hal_psram_status_t rst = HAL_PSRAM_STATUS_SUCCESS;
    PSRAM_MODE_ENUM mode = psram_type_get_from_AO_RG();

    switch (mode) {
        case PSRAM_MODE_PSRAM_4MB_APM:
        case PSRAM_MODE_PSRAM_8MB_APM:
        case PSRAM_MODE_PSRAM_4MB_WB:
        case PSRAM_MODE_PSRAM_8MB_WB:
            if (hal_security_restore(HAL_SECURITY_MODULE_MPU_PSRAM) == false) {
                hal_psram_log_suspend_resume("NON-UHS E_REBOOT_INVALID_SECURE_SETTING_PSRAM \n");
            }
            break;

        case PSRAM_MODE_UHS_PSRAM_8MB:
        case PSRAM_MODE_UHS_PSRAM_16MB:
            if (hal_security_restore(HAL_SECURITY_MODULE_MPU_PSRAM) == false) {
                hal_psram_log_suspend_resume("UHS E_REBOOT_INVALID_SECURE_SETTING_PSRAM \n");
            }
            break;

        default:
            hal_psram_log_suspend_resume("No PSRAM configured \n");
            rst = HAL_PSRAM_STATUS_FAIL;
            break;
    }

    return rst;
}

/**
 * @brief     This function backup psram setting before hsleep.
 * @return    #HAL_PSRAM_STATUS_SUCCESS, if the operation is successful.\n
 *            #HAL_PSRAM_STATUS_FAIL, if the operation is failed.
 */
ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME hal_psram_status_t hal_psram_ls_backup_before_hsleep(void)
{
    hal_psram_status_t rst = HAL_PSRAM_STATUS_SUCCESS;
    PSRAM_MODE_ENUM mode = psram_type_get_from_AO_RG();

    switch (mode) {
        case PSRAM_MODE_PSRAM_4MB_APM:
        case PSRAM_MODE_PSRAM_8MB_APM:
        case PSRAM_MODE_PSRAM_4MB_WB:
        case PSRAM_MODE_PSRAM_8MB_WB:
            hal_ls_backup(&(nonuhs_psram_ls_e2_table.e1_table));
            if (CHIP_HW_VER_E1 == get_chip_version()) { //E1 non_uhs psram 100MHz, and only support APM 4MB
                if (mode == PSRAM_MODE_PSRAM_4MB_APM) {
                    if (non_uhs_psram_ls_e1_table_fill() < 0)
                        return HAL_PSRAM_STATUS_FAIL;
                }
            } else {
                if (non_uhs_psram_ls_e2_table_fill(mode) < 0)
                    return HAL_PSRAM_STATUS_FAIL;
            }

            hal_ls_psram_param_addr_set(&nonuhs_psram_ls_e2_table);
            break;

        case PSRAM_MODE_UHS_PSRAM_8MB:
        case PSRAM_MODE_UHS_PSRAM_16MB:
            rst = HAL_PSRAM_STATUS_SUCCESS;//uhs psram no need to backup setting
            break;

        default:
            hal_psram_log_suspend_resume("No PSRAM configured \n");
            rst = HAL_PSRAM_STATUS_FAIL;
            break;
    }

    return rst;
}


/**
 * @brief     This function gets psram power to half sleep status.
 * @return    #HAL_PSRAM_STATUS_SUCCESS, if the operation is successful.\n
 *            #HAL_PSRAM_STATUS_FAIL, if the operation is failed.
 */
ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME hal_psram_status_t hal_psram_power_hsleep(void)
{
    hal_psram_status_t rst = HAL_PSRAM_STATUS_SUCCESS;
    PSRAM_MODE_ENUM mode = psram_type_get_from_AO_RG();

    switch (mode) {

#ifdef HAL_PSRAM_NON_UHS_ENABLED
        case PSRAM_MODE_PSRAM_4MB_APM:
        case PSRAM_MODE_PSRAM_8MB_APM:
        case PSRAM_MODE_PSRAM_4MB_WB:
        case PSRAM_MODE_PSRAM_8MB_WB:
            MTCMOS_PWR_DOWN_PSRAM_AXI_BUS;
            nonuhs_psram_low_pwr_half_slp(mode);
            non_uhs_psram_dll_disable();
            non_uhs_psram_controller_reset();
            MTCMOS_PWR_DOWN_NONUHS_PSRAM;
            break;
#endif /* #ifdef HAL_PSRAM_NON_UHS_ENABLED */
#ifdef HAL_PSRAM_UHS_ENABLED

        case PSRAM_MODE_UHS_PSRAM_8MB:
        case PSRAM_MODE_UHS_PSRAM_16MB:
            SRAM_PWR_DOWN_UHS_PSRAM;
            MTCMOS_PWR_DOWN_PSRAM_AXI_BUS;
            uhs_psram_Sidle_to_S1_HSLEEP();
            uhs_psram_S1_to_S0();
            MTCMOS_PWR_DOWN_UHS_PSRAM_OFF;
            break;
#endif /* #ifdef HAL_PSRAM_UHS_ENABLED */

        default:
            hal_psram_log_suspend_resume("No PSRAM configured \n");
            rst = HAL_PSRAM_STATUS_FAIL;
            break;
    }

    return rst;
}

/**
 * @brief     This function initializes the psram base enironment. Call this function if psram is required.
 * @return    #HAL_PSRAM_STATUS_SUCCESS, if the operation is successful.\n
 *            #HAL_PSRAM_STATUS_FAIL, if the operation is failed.
 */
hal_psram_status_t hal_psram_init(void)
{
    hal_psram_status_t rst = HAL_PSRAM_STATUS_SUCCESS;
    PSRAM_MODE_ENUM mode = PSRAM_MODE_NONE;

    hal_psram_log_print("hal_psram_init \n");

    if (!efuse_to_AO_done) { //for psram reinit assert issue
        psram_efuse_mode_to_AO_RG();
        efuse_to_AO_done = 1;
    }

    mode = psram_type_get_from_AO_RG();
    hal_psram_log_print("\nPsram type : %d \n ", (int)mode);

    switch (mode) {
#ifdef HAL_PSRAM_NON_UHS_ENABLED
        case PSRAM_MODE_PSRAM_4MB_APM:
        case PSRAM_MODE_PSRAM_8MB_APM:
        case PSRAM_MODE_PSRAM_4MB_WB:
        case PSRAM_MODE_PSRAM_8MB_WB:
            if (CHIP_HW_VER_E1 == get_chip_version()) {
                non_uhs_psram_dest_clk = e_CLK_37P5_MHZ;
            } else {
                non_uhs_psram_dest_clk = e_CLK_200_MHZ;
            }
            MTCMOS_PWR_ON_NONUHS_PSRAM;
            MTCMOS_PWR_ON_PSRAM_AXI_BUS;
            psram_mux_sw(FALSE);
#ifdef HAL_CLOCK_MODULE_ENABLED
            hal_clock_enable(HAL_CLOCK_CG_PSRAM);
            hal_clock_enable(HAL_CLOCK_CG_PSRAM_AXI);
            hal_clock_mux_select(HAL_CLOCK_SEL_PSRAM_AXI, CLK_PSRAM_AXI_CLKSEL_PSRAM);
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */
            if (check_psram_initial_done(mode)) {
                hal_psram_log_print("psram ready, no need reinit \n");
                asic_mpu_config_psram();
                return rst;
            }
            asic_mpu_config_psram();

            rst = (hal_psram_status_t)non_uhs_psram_K_Flow(mode, e_CLK_37P5_MHZ);
            if (HAL_PSRAM_STATUS_FAIL == rst) {
                hal_psram_log_print("Non-UHS PSRAM 37.5MHz K Fail, K 2nd\n");
                non_uhs_psram_1st_k_fail = 1;
                rst = (hal_psram_status_t)non_uhs_psram_K_Flow(mode, e_CLK_37P5_MHZ);
                if (HAL_PSRAM_STATUS_FAIL == rst) {
                    hal_psram_log_print("Non-UHS PSRAM K 2nd Fail!\n");
                } else {
                    hal_psram_log_print("Non-UHS PSRAM K 2nd Pass!\n");
                }
            } else {
                non_uhs_psram_1st_k_fail = 0;
                hal_psram_log_print("Non-UHS PSRAM 37.5MHz K Pass !!! \n");
            }

            if ((CHIP_HW_VER_EX == get_chip_version()) && (rst == HAL_PSRAM_STATUS_SUCCESS)) {
                rst = (hal_psram_status_t)non_uhs_psram_K_Flow(mode, e_CLK_200_MHZ);
                if (HAL_PSRAM_STATUS_FAIL == rst) {
                    hal_psram_log_print("Non-UHS PSRAM 200MHz K Fail!\n");
                } else {
                    hal_psram_log_print("Non-UHS PSRAM 200MHz K Pass!\n");
                }
            }
            break;
#endif /* #ifdef HAL_PSRAM_NON_UHS_ENABLED */

#ifdef HAL_PSRAM_UHS_ENABLED
        case PSRAM_MODE_UHS_PSRAM_8MB:
        case PSRAM_MODE_UHS_PSRAM_16MB:
            MTCMOS_PWR_ON_UHS_PSRAM_AON;
            MTCMOS_PWR_ON_UHS_PSRAM_OFF;
            MTCMOS_PWR_ON_PSRAM_AXI_BUS;
            SRAM_PWR_ON_UHS_PSRAM;
            psram_mux_sw(TRUE);
#ifdef HAL_CLOCK_MODULE_ENABLED
            hal_clock_enable(HAL_CLOCK_CG_UHS_PSRAM_XTAL);
            hal_clock_enable(HAL_CLOCK_CG_PSRAM_AXI);
            hal_clock_mux_select(HAL_CLOCK_SEL_PSRAM_AXI, CLK_PSRAM_AXI_CLKSEL_PSRAM);
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */
            if (check_psram_initial_done(mode)) {
                hal_psram_log_print("psram ready, no need reinit \n");
                asic_mpu_config_psram();
                return rst;
            }
            asic_mpu_config_psram();
            Init_DRAM(TYPE_PSRAM, CBT_R0_R1_NORMAL, NULL, NORMAL_USED);
            if (uhs_psram_k_result < 0) {
                hal_psram_log_print("UHS PSRAM K Fail \n");
                rst = HAL_PSRAM_STATUS_FAIL;
            } else {
                hal_psram_log_print("UHS PSRAM K Pass \n");
                rst = HAL_PSRAM_STATUS_SUCCESS;
            }
            break;
#endif /* #ifdef HAL_PSRAM_UHS_ENABLED */
        default: {
                hal_psram_log_print("PSRAM type wrong, type =%d \n", (mode));
                rst = HAL_PSRAM_STATUS_FAIL;
            }
            break;
    }

#ifdef PSRAM_WRITE_READ_TEST
    if (HAL_PSRAM_STATUS_SUCCESS == rst) {
        psram_write_read_test();
    }
#endif /* #ifdef PSRAM_WRITE_READ_TEST */

    set_psram_initial_done(mode);

    return rst;
}

/**
 * @brief     This function power off the psram, including slp memory cell.
 * @return    #HAL_PSRAM_STATUS_SUCCESS, if the operation is successful.\n
 *            #HAL_PSRAM_STATUS_FAIL, if the operation is failed.
 */
hal_psram_status_t hal_psram_off(void)
{
    hal_psram_status_t rst = HAL_PSRAM_STATUS_SUCCESS;
    PSRAM_MODE_ENUM mode = psram_type_get_from_AO_RG();

    switch (mode) {
        case PSRAM_MODE_PSRAM_4MB_APM:
        case PSRAM_MODE_PSRAM_8MB_APM:
        case PSRAM_MODE_PSRAM_4MB_WB:
        case PSRAM_MODE_PSRAM_8MB_WB:
#ifdef HAL_CLOCK_MODULE_ENABLED
            hal_clock_disable(HAL_CLOCK_CG_PSRAM);
            hal_clock_disable(HAL_CLOCK_CG_PSRAM_AXI);
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */
            MTCMOS_PWR_DOWN_PSRAM_AXI_BUS;
            nonuhs_psram_low_pwr_half_slp(mode);
            MTCMOS_PWR_DOWN_NONUHS_PSRAM;
            log_hal_info("Non-UHS PSRAM power off done\r\n");
            break;

        case PSRAM_MODE_UHS_PSRAM_8MB:
        case PSRAM_MODE_UHS_PSRAM_16MB:
#ifdef HAL_CLOCK_MODULE_ENABLED
            hal_clock_disable(HAL_CLOCK_CG_UHS_PSRAM_XTAL);
            hal_clock_disable(HAL_CLOCK_CG_PSRAM_AXI);
#endif /* #ifdef HAL_CLOCK_MODULE_ENABLED */
            SRAM_PWR_DOWN_UHS_PSRAM;
            MTCMOS_PWR_DOWN_PSRAM_AXI_BUS;
            uhs_psram_Sidle_to_S1_HSLEEP();
            uhs_psram_S1_to_S0();
            MTCMOS_PWR_DOWN_UHS_PSRAM_OFF;
            log_hal_info("UHS-PSRAM power off done\r\n");
            break;

        default: {
                hal_psram_log_print("unsupport psram mode \n");
                rst = HAL_PSRAM_STATUS_FAIL;
            }
            break;
    }
    return rst;
}
