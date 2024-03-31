/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <reg_base.h>
#include "hal.h"
#include "typedefs.h"
#include "driver_api.h"
#include "hal_clk.h"
#include "non_uhs_psram.h"
#include "hal_boot.h"
#include "memory_attribute.h"

#ifdef HAL_GDMA_MODULE_ENABLED
//#define DMA_SUPPORT
#endif /* #ifdef HAL_GDMA_MODULE_ENABLED */

#ifdef DMA_SUPPORT
#include "hal_gdma.h"
#include "hal_gdma_internal.h"
#endif /* #ifdef DMA_SUPPORT */

#ifdef DMA_SUPPORT
#define GDMA_POLLING_ENABLE
#endif /* #ifdef DMA_SUPPORT */

#define MT7933_HW_VERSION_ADDR          0x30034000

#define MT7933_RTC_BACKUP00_ADDR        0x30070140
#define NONUHS_PSRAM_APM_4MB_GLOBAL_RESET_DONE_FLAG    (0x99)

#define NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR    0x38070000

#define NON_UHS_PSRAM_TOP_CLK_OFF_BASE      0x30020000
#define NON_UHS_PSRAM_TOP_CFG_AON_BASE_ADDR 0x30030000

#define NON_UHS_PSRAM_MBIST_CTRL_0_ADDR                 (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x0110)
#define NON_UHS_PSRAM_MBIST_CTRL_1_ADDR                 (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x0114)
#define NON_UHS_PSRAM_MBIST_CTRL_2_ADDR                 (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x0118)
#define NON_UHS_PSRAM_MBIST_CTRL_3_ADDR                 (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x011c)
#define NON_UHS_PSRAM_MBIST_CTRL_4_ADDR                 (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x0120)

#define NON_UHS_PSRAM_AXI2PSRAM_CFG_2_ADDR              (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x08)
#define NON_UHS_PSRAM_NORMAL_RW_CFG_ADDR                (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x44)
#define NON_UHS_PSRAM_CFG_0_ADDR                        (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x48)

#define NON_UHS_PSRAM_NORMAL_SW_RST_B                   (NON_UHS_PSRAM_TOP_CFG_AON_BASE_ADDR+0x120)
#define NON_UHS_PSRAM_NORMAL_WDT_RST_CONTROL            (NON_UHS_PSRAM_TOP_CFG_AON_BASE_ADDR+0x100)

#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR       (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x10)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_1_ADDR       (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x14)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_2_ADDR       (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x18)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_3_ADDR       (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x1C)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_4_ADDR       (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x20)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_5_ADDR       (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x24)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_6_ADDR       (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x28)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_7_ADDR       (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x2C)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_8_ADDR       (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x30)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_9_ADDR       (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x34)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_10_ADDR      (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x38)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_11_ADDR      (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x3C)
#define NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_12_ADDR      (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x40)

#define NON_UHS_PSRAM_TIMING_0_ADDR         (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x50)
#define NON_UHS_PSRAM_TIMING_2_ADDR         (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x58)
#define NON_UHS_PSRAM_TIMING_3_ADDR         (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x5C)
#define NON_UHS_PSRAM_TIMING_4_ADDR         (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x60)

#define NON_UHS_PSRAM_ARB_CFG_ADDR          (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x70)

#define NON_UHS_PSRAM_DLL_CTRL_CFG_STUS_ADDR    (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0x80)
#define NON_UHS_PSRAM_CTRL_CFG_ADDR             (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0xB4)
#define NON_UHS_PSRAM_CTRL_STUS_ADDR            (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR+0xB8)

#define NON_UHS_PSRAM_DLL_INIT_TIMEOUT_SECOND   2
#define NON_UHS_PSRAM_RESET_TIMEOUT_SECOND      2
#define NON_UHS_PSRAM_REG_CMD_RD_BLOCK_TIMEOUT_SECOND     2
#define NON_UHS_PSRAM_REG_CMD_RD_BLOCK_NOCHK_TIMEOUT_SECOND     2
#define NON_UHS_PSRAM_REG_CMD_WR_BLOCK_TIMEOUT_SECOND     2

#define NON_UHS_PSRAM_MAX_MR_ID 0x08

//#define SECURE_SETTING_FOR_NON_UHS_ENABLE

#ifdef SECURE_SETTING_FOR_NON_UHS_ENABLE
#define SECURE_SETTING_DQ_IN_DELAY_VALUE    0x00
#define SECURE_SETTING_DQS_IN_DELAY_VALUE   0x0C
#endif /* #ifdef SECURE_SETTING_FOR_NON_UHS_ENABLE */

#define DQ_IN_DELAY_MAX             0x1F
#define DQS_IN_DELAY_MAX            0x1F
#define DQ_IN_DELAY_TUNING_STEP     1
#define DQS_IN_DELAY_TUNING_STEP    1

#define DLL_PHY_CFG_0   (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR + 0x84)
#define DLL_PHY_CFG_1   (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR + 0x88)
#define DLL_PHY_CFG_2   (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR + 0x8C)
#define DLL_PHY_CFG_3   (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR + 0x90)
#define DLL_PHY_CFG_4   (NON_UHS_PSRAM_CTRL_TOP_BASE_ADDR + 0x94)

#define DQ0_IN_DEL_OFFSET   0
#define DQ1_IN_DEL_OFFSET   8
#define DQ2_IN_DEL_OFFSET   16
#define DQ3_IN_DEL_OFFSET   24
#define DQ4_IN_DEL_OFFSET   0
#define DQ5_IN_DEL_OFFSET   8
#define DQ6_IN_DEL_OFFSET   16
#define DQ7_IN_DEL_OFFSET   24

#define DQ0_OUT_DEL_OFFSET   0
#define DQ1_OUT_DEL_OFFSET   8
#define DQ2_OUT_DEL_OFFSET   16
#define DQ3_OUT_DEL_OFFSET   24
#define DQ4_OUT_DEL_OFFSET   0
#define DQ5_OUT_DEL_OFFSET   8
#define DQ6_OUT_DEL_OFFSET   16
#define DQ7_OUT_DEL_OFFSET   24

#define DQS_IN_DEL_OFFSET       0
#define DQS_OUT_DEL_OFFSET      8
#define DQM_OUT_DEL_OFFSET      16

//#define MBIST_LOG_PRINT

#ifdef MBIST_LOG_PRINT
#define non_uhs_dbg_print   log_hal_info
#else /* #ifdef MBIST_LOG_PRINT */
#define non_uhs_dbg_print(...)
#endif /* #ifdef MBIST_LOG_PRINT */

//#define SET_DQ_DQS_DQM_DEALY_OUT_DELAY

#define DQM_OUT_DELAY_VALUE     7
#define DQS_OUT_DELAY_VALUE     0
U8 dq_out_delay_buf[8] = {0, 7, 4, 5, 5, 0, 4, 2};

#define PSRAM_BASE_ADDR             0xA0000000
#define MEM_RD_WR_TEST_ADDR         (PSRAM_BASE_ADDR + 0x3000)
#define MEM_RD_WR_DEST_ADDR         (PSRAM_BASE_ADDR + 0x13000)
#define MEM_RD_WR_55AA_TEST_ADDR    (PSRAM_BASE_ADDR + 0x3000)
#define MEM_RD_WR_FFFF_TEST_ADDR    (PSRAM_BASE_ADDR + 0xF0F0)
#define MEM_RD_WR_TEST_LEN          0x2000      //8k Bytes

#ifdef DMA_SUPPORT
#define PSRAM_WR_RD_TEST_DMA_CHANNEL    2
#endif /* #ifdef DMA_SUPPORT */

#define NON_UHS_PSRAM_BIST_TEST_TIMEOUT_US          1000*1000UL

#define BIST_TEST_WRITE_ONLY            0
#define BIST_TEST_READ_ONLY             1
#define BIST_TEST_WRITE_READ            2

#define BIST_TEST_TRANS_SIZE_1BYTE      0
#define BIST_TEST_TRANS_SIZE_2BYTE      1
#define BIST_TEST_TRANS_SIZE_4BYTE      2

#define BIST_TEST_TRANS_LEN_1           0
#define BIST_TEST_TRANS_LEN_4           1
#define BIST_TEST_TRANS_LEN_8           2
#define BIST_TEST_TRANS_LEN_16          3

#define BIST_TEST_RANDOM_DISABLE        0
#define BIST_TEST_RANDOM_ENABLE         1

#define BIST_TEST_INVERT_DISABLE        0
#define BIST_TEST_INVERT_ENABLE         1

#define BIST_SMALLEST_SIZE              (0x100)

typedef enum {
    e_DEALY_DQ = 0,
    e_DEALY_DQM = 1,
    e_DEALY_DQS = 2
} eDQ_DQM_DQS_DELAY;

typedef enum {
    e_55AA_ONLY = 0,
    e_55AA_BIT_INVERT = 1,
    e_55AA_RANDOM = 2,
    e_55AA_RANDOM_BIT_INVERT = 3,
    e_FFFF_ONLY = 4,
    e_FFFF_BIT_INVERT = 5,
    e_FFFF_RANDOM = 6,
    e_FFFF_RANDOM_BIT_INVERT = 7,
    e_PATTERN_TYPE_MAX = 8
} e_pattern_type;

typedef enum {
    e_CPU_TEST = 0,
    e_DMA_TEST = 1
} eTestType;

ePsram_CLK_type non_uhs_psram_dest_clk =  e_CLK_37P5_MHZ;

unsigned char get_edge_point = 0;

extern unsigned char non_uhs_psram_1st_k_fail;

ePsram_CLK_type non_uhs_psram_Get_dest_CLK(void)
{
    return non_uhs_psram_dest_clk;
}

static int non_uhs_psram_bist_test(U8 bist_rw, U8 bist_len, U8 bist_size, U16 bist_bg_data, U8 bist_ran, U8 bist_inv, U16 bist_start_addr, U16 bist_addr_range, U16 bist_addr_offset)
{
    uint32_t __start = 0, __dur = 0;

    HAL_REG_32(0x34030200) = (HAL_REG_32(0x34030200)) | 0x00000001;       // switch MUX to BIST

    //HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR)) | (((U32)bist_bg_data<<16) | ((U32)bist_ran<<13) | ((U32)bist_inv<<12) | ((U32)bist_len<<8) | ((U32)bist_size<<4) | ((U32)bist_rw<<1));
    HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR) & 0x0000CCC9) | (((U32)bist_bg_data << 16) | ((U32)bist_ran << 13) | ((U32)bist_inv << 12) | ((U32)bist_len << 8) | ((U32)bist_size << 4) | ((U32)bist_rw << 1));
    //HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_1_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_1_ADDR)) | ((U32)bist_start_addr<<16) | ((U32)bist_addr_range);
    HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_1_ADDR) = ((U32)bist_start_addr << 16) | ((U32)bist_addr_range);
    //HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_2_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_2_ADDR)) | ((U32)bist_addr_offset<<16) | ((U32)PSRAM_BASE_ADDR >> 16);
    HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_2_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_2_ADDR) & 0xFFC00000) | ((U32)bist_addr_offset << 16) | ((U32)PSRAM_BASE_ADDR >> 16);

    HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR)) | 0x00000001;

    non_uhs_dbg_print("NON_UHS_PSRAM_MBIST_CTRL_CR:\n110[0x%x] 114[0x%x] 118[0x%x]\n", HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR), HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_1_ADDR), HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_2_ADDR));
    non_uhs_dbg_print("11c[0x%x] 120[0x%x]\n", HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_3_ADDR), HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_4_ADDR));
    /* TINFO ="[PSRAM BIST] ---------- PSRAM BIST START! ----------"*/
    non_uhs_dbg_print("[PSRAM BIST] ---------- PSRAM BIST START! ---------- \n");
    non_uhs_dbg_print("[PSRAM BIST] RW mode        = %x \n", bist_rw);
    non_uhs_dbg_print("[PSRAM BIST] Length         = %x \n", bist_len);
    non_uhs_dbg_print("[PSRAM BIST] Size           = %x \n", bist_size);
    non_uhs_dbg_print("[PSRAM BIST] Begin data     = %x \n", bist_bg_data);
    non_uhs_dbg_print("[PSRAM BIST] Random         = %x \n", bist_ran);
    non_uhs_dbg_print("[PSRAM BIST] Inverter       = %x \n", bist_inv);
    non_uhs_dbg_print("[PSRAM BIST] Start address  = %x \n", bist_start_addr);
    non_uhs_dbg_print("[PSRAM BIST] Address range  = %x \n", bist_addr_range);
    non_uhs_dbg_print("[PSRAM BIST] Address Offset = %x \n", bist_addr_offset);

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__start);
    while ((((HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_3_ADDR)) & 0x00000002) != 0x00000002) && __dur <= (NON_UHS_PSRAM_BIST_TEST_TIMEOUT_US)) {
        uint32_t __now = 0;
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &__now);
        hal_gpt_get_duration_count(__start, __now, &__dur);
    }

    if (__dur > NON_UHS_PSRAM_BIST_TEST_TIMEOUT_US) {
        non_uhs_dbg_print("non_uhs_psram_bist_test fail,timeOut,return!\n");
        HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR)) & 0xFFFFFFFE;
        HAL_REG_32(0x34030200) = (HAL_REG_32(0x34030200)) & 0xFFFFFFFE;        // switch MUX to normal path (not BIST)
        return -1;
    }

    if (((HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_3_ADDR)) & 0x00000001) == 0x00000001) {
        non_uhs_dbg_print("[PSRAM BIST] ERROR!! PSRAM BIST Result = FAIL. \n");
        HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR)) & 0xFFFFFFFE;
        HAL_REG_32(0x34030200) = (HAL_REG_32(0x34030200)) & 0xFFFFFFFE;        // switch MUX to normal path (not BIST)
        return -1;
    } else {
        non_uhs_dbg_print("[PSRAM BIST] PSRAM BIST Result = PASS. \n");
        hal_gpt_delay_us(10);
    }

    non_uhs_dbg_print("[PSRAM BIST] ---------- PSRAM BIST DONE!!! ----------. \n");

    HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_MBIST_CTRL_0_ADDR)) & 0xFFFFFFFE;
    HAL_REG_32(0x34030200) = (HAL_REG_32(0x34030200)) & 0xFFFFFFFE;        // switch MUX to normal path (not BIST)

    hal_gpt_delay_us(10);

    return 0;
}

static int bist_all_memory_range_test(void)
{
    return non_uhs_psram_bist_test(BIST_TEST_WRITE_READ, BIST_TEST_TRANS_LEN_16, BIST_TEST_TRANS_SIZE_4BYTE, 0x5a5a, BIST_TEST_RANDOM_DISABLE, BIST_TEST_INVERT_DISABLE, (U16)((0xa0000000 & 0x00FFFFFF) >> 8), (U16)((psram_size_get() - 1) >> 8), (U16)0x0);
}

static int bist_partial_memory_range_test(unsigned int test_size)
{
    if (test_size < BIST_SMALLEST_SIZE) {
        non_uhs_dbg_print("bist_partial_memory_range_test error, test_size error [0x%x] <  [%x]!\n", test_size, BIST_SMALLEST_SIZE);
        return -1;
    }
    return non_uhs_psram_bist_test(BIST_TEST_WRITE_READ, BIST_TEST_TRANS_LEN_16, BIST_TEST_TRANS_SIZE_4BYTE, 0x5a5a, BIST_TEST_RANDOM_ENABLE, BIST_TEST_INVERT_DISABLE, (U16)((0xa0000000 & 0x00FFFFFF) >> 8), (U16)((test_size - 1) >> 8), (U16)0x0);
}

static int bist_minimum_length_and_size_test(void)
{
    return non_uhs_psram_bist_test(BIST_TEST_WRITE_READ, BIST_TEST_TRANS_LEN_1, BIST_TEST_TRANS_SIZE_1BYTE, 0x5a5a, BIST_TEST_RANDOM_DISABLE, BIST_TEST_INVERT_DISABLE, (U16)((0xa0000000 & 0x00FFFFFF) >> 8), (U16)(0x0FFFFF >> 8), (U16)0x1);
}

static int bist_only_write_and_only_read_test(void)
{
    int rst = 0;
    rst = non_uhs_psram_bist_test(BIST_TEST_WRITE_ONLY, BIST_TEST_TRANS_LEN_16, BIST_TEST_TRANS_SIZE_4BYTE, 0x5a5a, BIST_TEST_RANDOM_DISABLE, BIST_TEST_INVERT_DISABLE, (U16)((0xa0000000 & 0x00FFFFFF) >> 8), (U16)(0x0FFFFF >> 8), (U16)0x4);
    if (rst < 0) {
        non_uhs_dbg_print("[bist_only_write_and_only_read_test]: only write Fail. \n");
        return -1;
    }

    rst = non_uhs_psram_bist_test(BIST_TEST_READ_ONLY, BIST_TEST_TRANS_LEN_16, BIST_TEST_TRANS_SIZE_4BYTE, 0x5a5a, BIST_TEST_RANDOM_DISABLE, BIST_TEST_INVERT_DISABLE, (U16)((0xa0000000 & 0x00FFFFFF) >> 8), (U16)(0x0FFFFF >> 8), (U16)0x4);
    if (rst < 0) {
        non_uhs_dbg_print("[bist_only_write_and_only_read_test]: only Read Fail. \n");
        return -1;
    }

    return 0;
}
static int bist_fixed_data_test(void)
{
    int rst = 0;
    rst = non_uhs_psram_bist_test(BIST_TEST_WRITE_READ, BIST_TEST_TRANS_LEN_1, BIST_TEST_TRANS_SIZE_2BYTE, 0x3c3c, BIST_TEST_RANDOM_DISABLE, BIST_TEST_INVERT_ENABLE, (U16)((0xa0000000 & 0x00FFFFFF) >> 8), (U16)(0x0FFFFF >> 8), (U16)0x2);
    if (rst < 0) {
        non_uhs_dbg_print("[bist_fixed_data_test]: Fail. \n");
        return -1;
    }

    //then check the data,1:if the data is fixed.2:if data[31:16]&data[15:0]is inverted.
    non_uhs_dbg_print("[0xa0000000~0xa000000c]: [0x%x] [0x%x] [0x%x] [0x%x]\n", HAL_REG_32(0xa0000000), HAL_REG_32(0xa0000004), HAL_REG_32(0xa0000008), HAL_REG_32(0xa000000c));
    int i = 1;
    for (i = 1; i < (0x100000 / 4); i++) {
        if ((unsigned short)(HAL_REG_32(0xa0000000 + i * 4) & 0xFFFF) != (unsigned short)(~((HAL_REG_32(0xa0000000 + i * 4) >> 16) & 0xFFFF))) {
            non_uhs_dbg_print("[bist_fixed_data_test]: data invert check fail. \n");
            return -1;
        }
    }

    non_uhs_dbg_print("[bist_fixed_data_test]: data invert check Pass. \n");

    return 0;
}

int non_uhs_psram_mbist_test(void)
{
    if (bist_all_memory_range_test() < 0) {
        non_uhs_dbg_print("bist_all_memory_range_test fail! \n\n");
        return  NON_UHS_PSRAM_FAIL;
    }
    non_uhs_dbg_print("bist_all_memory_range_test pass! \n\n");

    if (bist_minimum_length_and_size_test() < 0) {
        non_uhs_dbg_print("bist_minimum_length_and_size_test fail! \n\n");
        return  NON_UHS_PSRAM_FAIL;
    }
    non_uhs_dbg_print("bist_minimum_length_and_size_test pass! \n\n");

    if (bist_only_write_and_only_read_test() < 0) {
        non_uhs_dbg_print("bist_only_write_and_only_read_test fail! \n\n");
        return  NON_UHS_PSRAM_FAIL;
    }
    non_uhs_dbg_print("bist_only_write_and_only_read_test pass! \n\n");

    hal_gpt_delay_us(100);
    if (bist_fixed_data_test() < 0) {
        non_uhs_dbg_print("bist_fixed_data_test fail! \n\n");
        return  NON_UHS_PSRAM_FAIL;
    }
    non_uhs_dbg_print("bist_fixed_data_test pass! \n\n");
    non_uhs_dbg_print("bist test pass! \n");

    return NON_UHS_PSRAM_SUCCESS;
}

static void non_uhs_psram_REG_CMD_WR_BLOCK(UINT32 cs_n_sel, UINT32 fsm_cmd_en, UINT32 fsm_lat_en, UINT32 fsm_dat_en, UINT32 fsm_dum_en, UINT32 fsm_csl_en, UINT32 fsm_rst_en, UINT32 cmd_cyc_num, UINT32 dat_cyc_num, UINT32 dum_cyc_num, UINT32 csl_ckoff, UINT32 csh_ckon, UINT32 cmd1, UINT32 cmd0, UINT32 wdata3, UINT32 wdata2, UINT32 wdata1, UINT32 wdata0)
{
    UINT32 tmp = 0;
    UINT64 timeout_cnt = 0;

    HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) = HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) | 0x00000010;             // Block AXI vld
    /* TINFO ="Block AXI vld" */

    tmp = (cs_n_sel << 16) + (csh_ckon << 9) + (csl_ckoff << 8) + (fsm_dum_en << 5) + (fsm_csl_en << 4) + (fsm_dat_en << 3) + (fsm_lat_en << 2) + (fsm_cmd_en << 1) + fsm_rst_en;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_1_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_1_ADDR, tmp */
    tmp = (dum_cyc_num << 8) + (dat_cyc_num << 4) + cmd_cyc_num;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_2_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_2_ADDR, tmp */
    tmp = cmd1;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_3_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_3_ADDR, tmp */
    tmp = cmd0;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_4_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_4_ADDR, tmp */

    tmp = wdata3;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_5_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_5_ADDR, tmp */
    tmp = wdata2;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_6_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_6_ADDR, tmp */
    tmp = wdata1;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_7_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_7_ADDR, tmp */
    tmp = wdata0;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_8_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_8_ADDR, tmp */

    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR) = 0x00000003;    /* trig command */
    tmp = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR);           /* polling command transfer end */
    while ((tmp & 0x00000001) == 0x00000001) {        /* polling command transfer end */
        /* polling command transfer end */
        if (timeout_cnt * 1 > NON_UHS_PSRAM_REG_CMD_WR_BLOCK_TIMEOUT_SECOND * 1000 * 1000UL) {
            non_uhs_dbg_print("!!!!!!!!!!!!NONUHS_REG_CMD_WR_BLOCK fail,timeOut,return!\n");
            return;
        }
        hal_gpt_delay_us(1);
        timeout_cnt++;
        tmp = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR);       /* polling command transfer end */
        /* TINFO ="APB2PSRAM_CUSTOM_CFG_0_ADDR(%x), data=%x, waiting for reg_cmd transfer done.", APB2PSRAM_CUSTOM_CFG_0_ADDR, tmp */
    }

    hal_gpt_delay_us(1);
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR) = 0x00000000;

    HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) = HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) & 0xFFFFFFEF;
    /* TINFO ="disable block AXI vld" */
}

/*============================================================
// NONUHS_REG_CMD_RD_BLOCK
//============================================================*/
#if 0
static void non_uhs_psram_REG_CMD_RD_BLOCK(UINT32 cs_n_sel, UINT32 fsm_cmd_en, UINT32 fsm_lat_en, UINT32 fsm_dat_en, UINT32 fsm_dum_en, UINT32 fsm_csl_en, UINT32 fsm_rst_en, UINT32 cmd_cyc_num, UINT32 dat_cyc_num, UINT32 dum_cyc_num, UINT32 csl_ckoff, UINT32 csh_ckon, UINT32 cmd1, UINT32 cmd0, UINT32 rdata_chk3, UINT32 rdata_chk2, UINT32 rdata_chk1, UINT32 rdata_chk0)
{
    UINT32 tmp = 0;
    UINT64 timeout_cnt = 0;

    HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) | 0x00000010);            /* Block AXI vld */
    /* TINFO ="Block AXI vld" */

    tmp = (cs_n_sel << 16) + (csh_ckon << 9) + (csl_ckoff << 8) + (fsm_dum_en << 5) + (fsm_csl_en << 4) + (fsm_dat_en << 3) + (fsm_lat_en << 2) + (fsm_cmd_en << 1) + fsm_rst_en;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_1_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_1_ADDR, tmp */
    tmp = (dum_cyc_num << 8) + (dat_cyc_num << 4) + cmd_cyc_num;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_2_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_2_ADDR, tmp */
    tmp = cmd1;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_3_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_3_ADDR, tmp */
    tmp = cmd0;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_4_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_4_ADDR, tmp */

    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR) = 0x00000001;    /* trig command */
    tmp = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR);           /* polling command transfer end */
    while ((tmp & 0x00000001) == 0x00000001) {        /* polling command transfer end */
        /* polling command transfer end */
        //delay_task(50);                             /* polling command transfer end */
        hal_gpt_delay_us(50);
        timeout_cnt++;
        if (timeout_cnt * 50 > NON_UHS_PSRAM_REG_CMD_RD_BLOCK_TIMEOUT_SECOND * 1000 * 1000UL) {
            non_uhs_dbg_print("!!!!!!!!!!!!NONUHS_REG_CMD_RD_BLOCK fail,timeOut,return!\n");
            return;
        }
        tmp = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR);       /* polling command transfer end */
        /* TINFO ="APB2PSRAM_CUSTOM_CFG_0_ADDR(%x), data=%x, waiting for reg_cmd transfer done.", APB2PSRAM_CUSTOM_CFG_0_ADDR, tmp */
    }

    //delay_task(10);
    hal_gpt_delay_us(10);
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR) = 0x00000000;

    HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) = HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) & 0xFFFFFFEF;
    /* TINFO ="disable block AXI vld" */

    tmp = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_9_ADDR);
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_9_ADDR, tmp */
    if (tmp != rdata_chk3) {
        /* TERR ="REG_CMD_RD (CR:%x) data mismatch!!!. exp = %x, read = %x.", APB2PSRAM_CUSTOM_CFG_9_ADDR, rdata_chk3, data_mrg*/
        non_uhs_dbg_print("TERR = REG_CMD_RD (CR:%x) data mismatch!!!. exp = 0x%x, read = 0x%x.\n", NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_9_ADDR, rdata_chk3, tmp);
    } else {
        /* TINFO ="REG_CMD_RD (CR:%x) data read = %x.", APB2PSRAM_CUSTOM_CFG_9_ADDR, data_mrg*/
    }

    tmp = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_10_ADDR);
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_10_ADDR, tmp */
    if (tmp != rdata_chk2) {
        /* TERR ="REG_CMD_RD (CR:%x) data mismatch!!!. exp = %x, read = %x.", APB2PSRAM_CUSTOM_CFG_10_ADDR, rdata_chk2, data_mrg*/
        non_uhs_dbg_print("TERR = REG_CMD_RD (CR:%x) data mismatch!!!. exp = 0x%x, read = 0x%x.\n", NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_10_ADDR, rdata_chk2, tmp);
    } else {
        /* TINFO ="REG_CMD_RD (CR:%x) data read = %x.", APB2PSRAM_CUSTOM_CFG_10_ADDR, data_mrg*/
    }

    tmp = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_11_ADDR);
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_11_ADDR, tmp */
    if (tmp != rdata_chk1) {
        /* TERR ="REG_CMD_RD (CR:%x) data mismatch!!!. exp = %x, read = %x.", APB2PSRAM_CUSTOM_CFG_11_ADDR, rdata_chk1, data_mrg*/
        non_uhs_dbg_print("TERR = REG_CMD_RD (CR:%x) data mismatch!!!. exp = 0x%x, read = 0x%x.\n", NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_11_ADDR, rdata_chk1, tmp);
    } else {
        /* TINFO ="REG_CMD_RD (CR:%x) data read = %x.", APB2PSRAM_CUSTOM_CFG_11_ADDR, data_mrg*/
    }

    tmp = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_12_ADDR);
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_12_ADDR, tmp */
    if (tmp != rdata_chk0) {
        /* TERR ="REG_CMD_RD (CR:%x) data mismatch!!!. exp = %x, read = %x.", APB2PSRAM_CUSTOM_CFG_12_ADDR, rdata_chk0, data_mrg*/
        non_uhs_dbg_print("TERR = REG_CMD_RD (CR:%x) data mismatch!!!. exp = 0x%x, read = 0x%x.\n", NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_12_ADDR, rdata_chk0, tmp);
    } else {
        /* TINFO ="REG_CMD_RD (CR:%x) data read = %x.", APB2PSRAM_CUSTOM_CFG_12_ADDR, data_mrg*/
    }
}

//============================================================
// nonuhs_psram_MRW
//============================================================
static int non_uhs_psram_MRW(PSRAM_MODE_ENUM psram_type, U8 mr_MA, U8 mr_OP)
{
    if (mr_MA > NON_UHS_PSRAM_MAX_MR_ID) {
        non_uhs_dbg_print("Invalid mr_MA[%d], return \n", mr_MA);
        return -1;
    }

    if (PSRAM_MODE_PSRAM_4MB_APM == psram_type) {
        //                          cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,                       cmd1,       cmd0,                     data3,      data2,      data1,      data0
        NONUHS_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x1,   0x1,   0x0,   0x0,   0x2,   0x1,   0x1,    0x0,    0x1, (0x0000C000 | (mr_MA << 16)), 0x00000000, (0x00000000 | (mr_OP << 8)), 0x00000000, 0x00000000, 0x00000000);
    } else if (PSRAM_MODE_PSRAM_8MB_APM == psram_type) {
        //                          cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,                 cmd0,                     data3,      data2,      data1,      data0
        NONUHS_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0000C0C0, (0x00000000 | mr_MA), (0x00000000 | (mr_OP << 8)), 0x00000000, 0x00000000, 0x00000000);
    } else {
        non_uhs_dbg_print("MRR Fail, Invalid psram type = %d \n", psram_type);
        return -1;
    }


    return 0;
}
#endif /* #if 0 */
//============================================================
// NONUHS_REG_CMD_RD_BLOCK_NOCHK
//============================================================
static void non_uhs_psram_REG_CMD_RD_BLOCK_NOCHK(UINT32 cs_n_sel, UINT32 fsm_cmd_en, UINT32 fsm_lat_en, UINT32 fsm_dat_en, UINT32 fsm_dum_en, UINT32 fsm_csl_en, UINT32 fsm_rst_en, UINT32 cmd_cyc_num, UINT32 dat_cyc_num, UINT32 dum_cyc_num, UINT32 csl_ckoff, UINT32 csh_ckon, UINT32 cmd1, UINT32 cmd0)
{
    UINT32 tmp = 0;
    U64 timeout_cnt = 0;

    HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) | 0x00000010);

    tmp = (cs_n_sel << 16) + (csh_ckon << 9) + (csl_ckoff << 8) + (fsm_dum_en << 5) + (fsm_csl_en << 4) + (fsm_dat_en << 3) + (fsm_lat_en << 2) + (fsm_cmd_en << 1) + fsm_rst_en;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_1_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_1_ADDR, tmp */
    tmp = (dum_cyc_num << 8) + (dat_cyc_num << 4) + cmd_cyc_num;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_2_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_2_ADDR, tmp */
    tmp = cmd1;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_3_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_3_ADDR, tmp */
    tmp = cmd0;
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_4_ADDR) = tmp;
    /* TINFO ="CUSTOM_CFG_ADDR(%x), data=%x ", APB2PSRAM_CUSTOM_CFG_4_ADDR, tmp */

    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR) = 0x00000001;    /* trig command */
    tmp = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR);           /* polling command transfer end */
    while ((tmp & 0x00000001) == 0x00000001) {        /* polling command transfer end */
        /* polling command transfer end */
        if (timeout_cnt * 1 > NON_UHS_PSRAM_REG_CMD_RD_BLOCK_NOCHK_TIMEOUT_SECOND * 1000 * 1000UL) {
            non_uhs_dbg_print("!!!!!!!!!!!!NONUHS_REG_CMD_RD_BLOCK fail,timeOut,return!\n");
            return;
        }
        hal_gpt_delay_us(1);
        timeout_cnt++;
        tmp = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR);       /* polling command transfer end */
        /* TINFO ="APB2PSRAM_CUSTOM_CFG_0_ADDR(%x), data=%x, waiting for reg_cmd transfer done.", APB2PSRAM_CUSTOM_CFG_0_ADDR, tmp */
    }

    hal_gpt_delay_us(1);
    HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_0_ADDR) = 0x00000000;

    HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_ARB_CFG_ADDR) & 0xFFFFFFEF);
    /* TINFO ="disable block AXI vld" */
}

//============================================================
// nonuhs_psram_MRR
//============================================================
static int non_uhs_psram_MRR(PSRAM_MODE_ENUM psram_type, U8 mr_MA, U32 *p_MR_value)
{
    if ((mr_MA > NON_UHS_PSRAM_MAX_MR_ID) || (p_MR_value == NULL)) {
        non_uhs_dbg_print("Invalid mr_MA[%d] or p_MR_value == NULL, return \n", mr_MA);
        return -1;
    }


    if (PSRAM_MODE_PSRAM_4MB_APM == psram_type) {
        //                                cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,                       cmd1,       cmd0
        non_uhs_psram_REG_CMD_RD_BLOCK_NOCHK(0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x2,   0x1,   0x0,    0x0,    0x1, (0x00004000 | (mr_MA << 16)), 0x00000000);
    } else if (PSRAM_MODE_PSRAM_8MB_APM == psram_type) {
        //                                cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,                 cmd0
        non_uhs_psram_REG_CMD_RD_BLOCK_NOCHK(0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x00004040, (0x00000000 | mr_MA));
    } else if ((PSRAM_MODE_PSRAM_4MB_WB == psram_type) || (PSRAM_MODE_PSRAM_8MB_WB == psram_type)) {
        //                                cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on, cmd1,      cmd0
        switch (mr_MA) {
            case 0: //CR0
                non_uhs_psram_REG_CMD_RD_BLOCK_NOCHK(0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0100E000, 0x00000000);
                break;
            case 1: //CR1
                non_uhs_psram_REG_CMD_RD_BLOCK_NOCHK(0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0100E000, 0x00000001);
                break;
            case 2: //IR0
                non_uhs_psram_REG_CMD_RD_BLOCK_NOCHK(0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0000E000, 0x00000000);
                break;
            case 3: //IR1
                non_uhs_psram_REG_CMD_RD_BLOCK_NOCHK(0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0000E000, 0x00000001);
                break;
            default:
                break;
        }
    } else {
        non_uhs_dbg_print("MRR Fail, Invalid psram type = %d \n", psram_type);
        return -1;
    }

    *p_MR_value = HAL_REG_32(NON_UHS_PSRAM_APB2PSRAM_CUSTOM_CFG_9_ADDR);

    return 0;
}

#ifdef DMA_SUPPORT
#ifdef GDMA_POLLING_ENABLE
static int non_uhs_psram_GDMA_POLLING_TEST(U32 *p_dst_addr, U32 *p_src_addr, U32 len)
{
    int ret = 0;

    memset(p_dst_addr, 0, len);

    /* p_src_addr => p_dst_addr */
    /* GDMA channel X polling mode */
    ret = hal_gdma_init(PSRAM_WR_RD_TEST_DMA_CHANNEL);
    if (ret) {
        non_uhs_dbg_print("channel%d hal_gdma_init fail,ret=%d\r\n", PSRAM_WR_RD_TEST_DMA_CHANNEL, ret);
        return -1;
    }

    ret = hal_gdma_start_polling(PSRAM_WR_RD_TEST_DMA_CHANNEL, (uint32_t)p_dst_addr, (uint32_t)p_src_addr, len);
    if (ret) {
        non_uhs_dbg_print("channel%d hal_gdma_start_polling fail,ret=%d\r\n", PSRAM_WR_RD_TEST_DMA_CHANNEL, ret);
        return -1;
    }

    ret = hal_gdma_deinit(PSRAM_WR_RD_TEST_DMA_CHANNEL);
    if (ret) {
        non_uhs_dbg_print("channel%d hal_gdma_deinit fail,ret=%d\r\n", PSRAM_WR_RD_TEST_DMA_CHANNEL, ret);
        return -1;
    }

    return 0;
}
#else /* #ifdef GDMA_POLLING_ENABLE */
static void non_uhs_psram_GDMA0_ISR_HANDLER(hal_gdma_event_t event, void  *user_data)
{
    //non_uhs_dbg_print("[UT_GDMA]GDMA0_ISR_HANDLER\r\n");
    gdma_clear_irq(PSRAM_WR_RD_TEST_DMA_CHANNEL);
}

static int non_uhs_psram_GDMA_INTERRUPT_TEST(U32 *p_dst_addr, U32 *p_src_addr, U32 len)
{
    int ret = 0;
    hal_gdma_running_status_t run_sta = 0;

    memset(p_src_addr, 0, len);

    /* GDMA channel 0 interrupt mode */
    ret = hal_gdma_init(PSRAM_WR_RD_TEST_DMA_CHANNEL);
    if (ret) {
        non_uhs_dbg_print("channel%d init fail,ret=%d\r\n", PSRAM_WR_RD_TEST_DMA_CHANNEL, ret);
        return -1;
    }
    ret = hal_gdma_register_callback(PSRAM_WR_RD_TEST_DMA_CHANNEL, non_uhs_psram_GDMA0_ISR_HANDLER, NULL);
    if (ret) {
        non_uhs_dbg_print("channel%d hal_gdma_register_callback fail,ret=%d\r\n", PSRAM_WR_RD_TEST_DMA_CHANNEL, ret);
        return -1;
    }
    /* p_src_addr => p_dst_addr */
    ret = hal_gdma_start_interrupt(PSRAM_WR_RD_TEST_DMA_CHANNEL, (uint32_t)p_dst_addr, (uint32_t)p_src_addr, len);
    if (ret) {
        non_uhs_dbg_print("channel%d hal_gdma_start_interrupt fail,ret=%d\r\n", PSRAM_WR_RD_TEST_DMA_CHANNEL, ret);
        return -1;
    }
    hal_gdma_get_running_status(PSRAM_WR_RD_TEST_DMA_CHANNEL, &run_sta);
    while (run_sta) {
        hal_gdma_get_running_status(PSRAM_WR_RD_TEST_DMA_CHANNEL, &run_sta);
    }

    ret = hal_gdma_deinit(PSRAM_WR_RD_TEST_DMA_CHANNEL);
    if (ret) {
        non_uhs_dbg_print("channel0 hal_gdma_deinit fail,ret=%d\r\n", ret);
        return -1;
    }

    return 0;
}
#endif /* #ifdef GDMA_POLLING_ENABLE */
#endif /* #ifdef DMA_SUPPORT */

static void non_uhs_psram_pattern_generate(e_pattern_type type, U64 init_data, U64 *p_pattern, U32 addr)
{
    switch (type) {
        case e_55AA_ONLY:
            *p_pattern = init_data;
            break;

        case e_55AA_BIT_INVERT:
            *p_pattern = ~(init_data);
            break;

        case e_55AA_RANDOM:
            *p_pattern = (~(((init_data >> 63) & 0x1) | (((init_data >> 32) & 0x3FFFFFFF) << 1) | (((init_data >> 31) & 0x1) << (32)) | ((init_data & 0x7FFFFFFF) << (33)))) +
                         (~(((U64)((addr >> 3) & 0x1FFF)) | ((U64)((addr & 0x7) << 13)) | (((U64)((addr >> 13) & 0x7)) << 16) | (((U64)(addr & 0x1FFF)) << 19)
                            | (((U64)((addr >> 3) & 0x1FFF)) << 32) | (((U64)(addr & 0x7)) << 45) | (((U64)((addr >> 13) & 0x7)) << 48) | (((U64)(addr & 0x1FFF)) << 51)));
            *p_pattern = init_data + (*p_pattern);
            break;

        case e_55AA_RANDOM_BIT_INVERT:
            *p_pattern = (~(((init_data >> 63) & 0x1) | (((init_data >> 32) & 0x3FFFFFFF) << 1) | (((init_data >> 31) & 0x1) << (32)) | ((init_data & 0x7FFFFFFF) << (33)))) +
                         (~(((U64)((addr >> 3) & 0x1FFF)) | ((U64)((addr & 0x7) << 13)) | (((U64)((addr >> 13) & 0x7)) << 16) | (((U64)(addr & 0x1FFF)) << 19)
                            | (((U64)((addr >> 3) & 0x1FFF)) << 32) | (((U64)(addr & 0x7)) << 45) | (((U64)((addr >> 13) & 0x7)) << 48) | (((U64)(addr & 0x1FFF)) << 51)));
            *p_pattern = ~(init_data + (*p_pattern));
            break;

        case e_FFFF_ONLY:
            *p_pattern = init_data;
            break;
        case e_FFFF_BIT_INVERT:
            *p_pattern = ~(init_data);
            break;
        case e_FFFF_RANDOM:
            *p_pattern = (~(((init_data >> 63) & 0x1) | (((init_data >> 32) & 0x3FFFFFFF) << 1) | (((init_data >> 31) & 0x1) << (32)) | ((init_data & 0x7FFFFFFF) << (33)))) +
                         (~(((U64)((addr >> 3) & 0x1FFF)) | ((U64)((addr & 0x7) << 13)) | (((U64)((addr >> 13) & 0x7)) << 16) | (((U64)(addr & 0x1FFF)) << 19)
                            | (((U64)((addr >> 3) & 0x1FFF)) << 32) | (((U64)(addr & 0x7)) << 45) | (((U64)((addr >> 13) & 0x7)) << 48) | (((U64)(addr & 0x1FFF)) << 51)));
            *p_pattern = init_data + (*p_pattern);
            break;

        case e_FFFF_RANDOM_BIT_INVERT:
            *p_pattern = (~(((init_data >> 63) & 0x1) | (((init_data >> 32) & 0x3FFFFFFF) << 1) | (((init_data >> 31) & 0x1) << (32)) | ((init_data & 0x7FFFFFFF) << (33)))) +
                         (~(((U64)((addr >> 3) & 0x1FFF)) | ((U64)((addr & 0x7) << 13)) | (((U64)((addr >> 13) & 0x7)) << 16) | (((U64)(addr & 0x1FFF)) << 19)
                            | (((U64)((addr >> 3) & 0x1FFF)) << 32) | (((U64)(addr & 0x7)) << 45) | (((U64)((addr >> 13) & 0x7)) << 48) | (((U64)(addr & 0x1FFF)) << 51)));
            *p_pattern = ~(init_data + (*p_pattern));
            break;

        default:
            break;
    }
}

static U64 non_uhs_psram_init_data_map(e_pattern_type type)
{
    switch (type) {
        case e_55AA_ONLY:
        case e_55AA_BIT_INVERT:
        case e_55AA_RANDOM:
        case e_55AA_RANDOM_BIT_INVERT:
            return 0x5aa55a5aa5a5a55a;
        case e_FFFF_ONLY:
        case e_FFFF_BIT_INVERT:
        case e_FFFF_RANDOM:
        case e_FFFF_RANDOM_BIT_INVERT:
            return 0xFFFFFFFFFFFFFFFF;

        default:
            non_uhs_dbg_print("non_uhs_psram_init_data_map: Wrong type[%d]. \n", type);
            return 0;
    }
}

static U32 non_uhs_psram_init_addr_map(e_pattern_type type)
{
    switch (type) {
        case e_55AA_ONLY:
        case e_55AA_BIT_INVERT:
        case e_55AA_RANDOM:
        case e_55AA_RANDOM_BIT_INVERT:
            return MEM_RD_WR_55AA_TEST_ADDR;
        case e_FFFF_ONLY:
        case e_FFFF_BIT_INVERT:
        case e_FFFF_RANDOM:
        case e_FFFF_RANDOM_BIT_INVERT:
            return MEM_RD_WR_FFFF_TEST_ADDR;

        default:
            non_uhs_dbg_print("non_uhs_psram_init_addr_map: Wrong type[%d]. \n", type);
            return 0;
    }
}

static int non_uhs_psram_CPU_DMA_RD_WR_test(eTestType test_type)
{
    int i = 0;
    int j = 0;
    U64 tmp_pattern = 0;
    U64 build_pattern = 0;

    for (i = 0; i < (int)e_PATTERN_TYPE_MAX; i++) {
        memset((void *)MEM_RD_WR_DEST_ADDR, 0, MEM_RD_WR_TEST_LEN);
        memset((void *)MEM_RD_WR_55AA_TEST_ADDR, 0, MEM_RD_WR_TEST_LEN);
        memset((void *)MEM_RD_WR_FFFF_TEST_ADDR, 0, MEM_RD_WR_TEST_LEN);

        tmp_pattern = non_uhs_psram_init_data_map((e_pattern_type)i);
        for (j = 0; j < MEM_RD_WR_TEST_LEN; j = j + sizeof(U64)) {
            non_uhs_psram_pattern_generate((e_pattern_type)i, tmp_pattern, &build_pattern, (non_uhs_psram_init_addr_map((e_pattern_type)i) + j));
            (*((volatile U64 *)(non_uhs_psram_init_addr_map((e_pattern_type)i) + j))) = build_pattern;
            tmp_pattern = build_pattern;
        }

        if (e_CPU_TEST == test_type) {
            /*CPU Test, Psram1->Psram2*/
            memcpy((void *)(MEM_RD_WR_DEST_ADDR), (void *)(non_uhs_psram_init_addr_map((e_pattern_type)i)), MEM_RD_WR_TEST_LEN);
        } else if (e_DMA_TEST == test_type) {
#ifdef DMA_SUPPORT
            int rst = 0;
#ifdef GDMA_POLLING_ENABLE
            rst = non_uhs_psram_GDMA_POLLING_TEST((U32 *)(MEM_RD_WR_DEST_ADDR), (U32 *)((non_uhs_psram_init_addr_map((e_pattern_type)i))), MEM_RD_WR_TEST_LEN);
#else /* #ifdef GDMA_POLLING_ENABLE */
            rst = non_uhs_psram_GDMA_INTERRUPT_TEST((U32 *)(MEM_RD_WR_DEST_ADDR), (U32 *)((non_uhs_psram_init_addr_map((e_pattern_type)i))), MEM_RD_WR_TEST_LEN);
#endif /* #ifdef GDMA_POLLING_ENABLE */
            if (rst < 0) {
                non_uhs_dbg_print("DMA transfer data fail, return! \n");
                return -1;
            }
#endif /* #ifdef DMA_SUPPORT */
        }

        /*check Data*/
        tmp_pattern = non_uhs_psram_init_data_map((e_pattern_type)i);
        for (j = 0; j < MEM_RD_WR_TEST_LEN; j = j + sizeof(U64)) {
            non_uhs_psram_pattern_generate((e_pattern_type)i, tmp_pattern, &build_pattern, (non_uhs_psram_init_addr_map((e_pattern_type)i) + j));
            if (memcmp((void *)(&build_pattern), (void *)(MEM_RD_WR_DEST_ADDR + j), sizeof(U64)) != 0) {
                non_uhs_dbg_print("\tWr/Rd Fail, \n\tsrc_data = [0x%x %x]\n", (unsigned int)((build_pattern >> 32) & 0xFFFFFFFF), (unsigned int)(build_pattern & 0xFFFFFFFF));
                non_uhs_dbg_print("\tdst_data = [0x%x %x]\n", (unsigned int)(((*((volatile U64 *)(MEM_RD_WR_DEST_ADDR + j))) >> 32) & 0xFFFFFFFF), (unsigned int)((*((volatile U64 *)(MEM_RD_WR_DEST_ADDR + j))) & 0xFFFFFFFF));
                return -1;
            }
            tmp_pattern = build_pattern;
        }
    }

    return 0;
}

static int non_uhs_psram_CPU_wr_rd_test(unsigned int start_addr, unsigned int len)
{
    unsigned int i = 0;
    for (i = 0; i < len; i = i + sizeof(unsigned int)) {
        HAL_REG_32((start_addr + i)) = i;
    }

    for (i = 0; i < len; i = i + sizeof(unsigned int)) {
        if (HAL_REG_32(start_addr + i) != i) {
            non_uhs_dbg_print("non_uhs_psram_CPU_wr_rd_test: Fail, i = 0x%x, dst_data = [0x%x]\n", i, HAL_REG_32(start_addr + i));
            return -1;
        }
    }

    return 0;
}

static eNon_UHS_Psram_Status non_uhs_psram_write_read_test(void)
{
    if (get_chip_version() == CHIP_HW_VER_E1) {
        if (non_uhs_psram_CPU_DMA_RD_WR_test(e_CPU_TEST) < 0) {
            non_uhs_dbg_print("\tCPU write read test Fail!\n");
            return NON_UHS_PSRAM_FAIL;
        }
        non_uhs_dbg_print("[CPU write read test Pass!]\n");

#ifdef DMA_SUPPORT
        if (non_uhs_psram_CPU_DMA_RD_WR_test(e_DMA_TEST) < 0) {
            non_uhs_dbg_print("\tDMA write read test Fail!\n");
            return NON_UHS_PSRAM_FAIL;
        }
        non_uhs_dbg_print("[DMA write read test Pass!]\n");
#endif /* #ifdef DMA_SUPPORT */
    } else {
        if ((get_edge_point) || (non_uhs_psram_1st_k_fail)) {
            if (bist_partial_memory_range_test(0x10000) < 0) {
                non_uhs_dbg_print("\tBIST write read test Fail!\n");
                return NON_UHS_PSRAM_FAIL;
            }
        } else {
            if (bist_partial_memory_range_test(0x1000) < 0) {//0x10000(64K) -> 0x1000(4K) for reducing init time
                non_uhs_dbg_print("\tBIST write read test Fail!\n");
                return NON_UHS_PSRAM_FAIL;
            }
        }
        non_uhs_dbg_print("[BIST write read test Pass!]\n");

        if (non_uhs_psram_CPU_wr_rd_test(PSRAM_BASE_ADDR, 0x100) < 0) {
            non_uhs_dbg_print("\tCPU write read test Fail!\n");
            return NON_UHS_PSRAM_FAIL;
        }
        non_uhs_dbg_print("[CPU write read test Pass!]\n");
    }

    return NON_UHS_PSRAM_SUCCESS;
}

static eNon_UHS_Psram_Status non_uhs_psram_set_DQ_DQS_in_delay_value(eDQ_DQM_DQS_DELAY type, unsigned int delay_value)
{
    if (delay_value > ((0x1 << 6) - 1)) {
        return  NON_UHS_PSRAM_FAIL;
    }

    if (type == e_DEALY_DQ) {
        HAL_REG_32(DLL_PHY_CFG_0) = (HAL_REG_32(DLL_PHY_CFG_0) & 0xE0E0E0E0) | delay_value << DQ3_IN_DEL_OFFSET | delay_value << DQ2_IN_DEL_OFFSET | delay_value << DQ1_IN_DEL_OFFSET | delay_value;
        HAL_REG_32(DLL_PHY_CFG_1) = (HAL_REG_32(DLL_PHY_CFG_1) & 0xE0E0E0E0) | delay_value << DQ7_IN_DEL_OFFSET | delay_value << DQ6_IN_DEL_OFFSET | delay_value << DQ5_IN_DEL_OFFSET | delay_value;
    } else if (type == e_DEALY_DQS) {
        HAL_REG_32(DLL_PHY_CFG_4) = (HAL_REG_32(DLL_PHY_CFG_4) & 0xFFFFFFE0) | delay_value << DQS_IN_DEL_OFFSET;
    } else {
        return  NON_UHS_PSRAM_FAIL;
    }

    return  NON_UHS_PSRAM_SUCCESS;
}

#ifdef SET_DQ_DQS_DQM_DEALY_OUT_DELAY
static eNon_UHS_Psram_Status non_uhs_psram_set_DQ_DQS_out_delay_value(eDQ_DQM_DQS_DELAY type, U8 *p_delay_buf)
{
    if (p_delay_buf == NULL) {
        non_uhs_dbg_print("non_uhs_psram_set_DQ_DQS_out_delay_value: invalid para, p_delay_buf == NULL \n");
        return  NON_UHS_PSRAM_FAIL;
    }

    int i = 0;
    if (type == e_DEALY_DQ) {
        for (i = 0; i < 8; i++) {
            if (p_delay_buf[i] > ((0x1 << 6) - 1)) {
                non_uhs_dbg_print("non_uhs_psram_set_DQ_DQS_out_delay_value: invalid para, p_delay_buf[%d] == 0x%x\n", i, p_delay_buf[i]);
                return  NON_UHS_PSRAM_FAIL;
            }
        }
        HAL_REG_32(DLL_PHY_CFG_2) = (HAL_REG_32(DLL_PHY_CFG_2) & 0xE0E0E0E0) | (p_delay_buf[3] << DQ3_OUT_DEL_OFFSET) | (p_delay_buf[2] << DQ2_OUT_DEL_OFFSET) | (p_delay_buf[1] << DQ1_OUT_DEL_OFFSET) | (p_delay_buf[0]);
        HAL_REG_32(DLL_PHY_CFG_3) = (HAL_REG_32(DLL_PHY_CFG_3) & 0xE0E0E0E0) | (p_delay_buf[7] << DQ7_OUT_DEL_OFFSET) | (p_delay_buf[6] << DQ6_OUT_DEL_OFFSET) | (p_delay_buf[5] << DQ5_OUT_DEL_OFFSET) | (p_delay_buf[4]);
    } else if (type == e_DEALY_DQM) {
        if (p_delay_buf[0] > ((0x1 << 6) - 1)) {
            non_uhs_dbg_print("non_uhs_psram_set_DQ_DQS_out_delay_value: invalid DQM out Dealy == 0x%x\n", p_delay_buf[0]);
            return  NON_UHS_PSRAM_FAIL;
        }
        HAL_REG_32(DLL_PHY_CFG_4) = (HAL_REG_32(DLL_PHY_CFG_4) & 0xFFE0FFFF) | (p_delay_buf[0] << DQM_OUT_DEL_OFFSET);
    } else if (type == e_DEALY_DQS) {
        if (p_delay_buf[0] > ((0x1 << 6) - 1)) {
            non_uhs_dbg_print("non_uhs_psram_set_DQ_DQS_out_delay_value: invalid DQS out Dealy == 0x%x\n", p_delay_buf[0]);
            return  NON_UHS_PSRAM_FAIL;
        }
        HAL_REG_32(DLL_PHY_CFG_4) = (HAL_REG_32(DLL_PHY_CFG_4) & 0xFFFFE0FF) | (p_delay_buf[0] << DQS_OUT_DEL_OFFSET);
    } else {
        return  NON_UHS_PSRAM_FAIL;
    }

    return  NON_UHS_PSRAM_SUCCESS;
}
#endif /* #ifdef SET_DQ_DQS_DQM_DEALY_OUT_DELAY */

#ifdef SECURE_SETTING_FOR_NON_UHS_ENABLE
static eNon_UHS_Psram_Status non_uhs_psram_secure_setting_for_DQDQS_delay(void)
{
    eNon_UHS_Psram_Status rst = NON_UHS_PSRAM_SUCCESS;
    rst = non_uhs_psram_set_DQ_DQS_in_delay_value(e_DEALY_DQ, SECURE_SETTING_DQ_IN_DELAY_VALUE);
    if (rst != NON_UHS_PSRAM_SUCCESS) {
        non_uhs_dbg_print("secure_setting DQ in delay fail");
        return NON_UHS_PSRAM_FAIL;
    }

    rst = non_uhs_psram_set_DQ_DQS_in_delay_value(e_DEALY_DQS, SECURE_SETTING_DQS_IN_DELAY_VALUE);
    if (rst != NON_UHS_PSRAM_SUCCESS) {
        non_uhs_dbg_print("secure_setting DQS in delay fail");
        return NON_UHS_PSRAM_FAIL;
    }

    return NON_UHS_PSRAM_SUCCESS;
}
#endif /* #ifdef SECURE_SETTING_FOR_NON_UHS_ENABLE */

#ifdef SET_DQ_DQS_DQM_DEALY_OUT_DELAY
static eNon_UHS_Psram_Status non_uhs_psram_Set_Reg_Out_Delay_STA(void)
{
    U8 *p_dq_out_delay = dq_out_delay_buf;
    if (NON_UHS_PSRAM_FAIL == non_uhs_psram_set_DQ_DQS_out_delay_value(e_DEALY_DQ, p_dq_out_delay)) {
        return NON_UHS_PSRAM_FAIL;
    }

    U8 dqm_out_delay = DQM_OUT_DELAY_VALUE;
    if (NON_UHS_PSRAM_FAIL == non_uhs_psram_set_DQ_DQS_out_delay_value(e_DEALY_DQM, &dqm_out_delay)) {
        return NON_UHS_PSRAM_FAIL;
    }

    U8 dqs_out_delay = DQS_OUT_DELAY_VALUE;
    if (NON_UHS_PSRAM_FAIL == non_uhs_psram_set_DQ_DQS_out_delay_value(e_DEALY_DQS, &dqs_out_delay)) {
        return NON_UHS_PSRAM_FAIL;
    }

    return NON_UHS_PSRAM_SUCCESS;
}
#endif /* #ifdef SET_DQ_DQS_DQM_DEALY_OUT_DELAY */

static eNon_UHS_Psram_Status non_uhs_psram_read_calibration_flow(void)
{
    int dqy_in_del = DQ_IN_DELAY_MAX;
    int dqs_in_del = DQS_IN_DELAY_MAX;
    eNon_UHS_Psram_Status rst = NON_UHS_PSRAM_SUCCESS;

    rst = non_uhs_psram_set_DQ_DQS_in_delay_value(e_DEALY_DQ, 0);
    if (NON_UHS_PSRAM_FAIL == rst) {
        non_uhs_dbg_print("DQ in delay set to 0 Fail \n");
        return NON_UHS_PSRAM_FAIL;
    }
    rst = non_uhs_psram_set_DQ_DQS_in_delay_value(e_DEALY_DQS, 0);
    if (NON_UHS_PSRAM_FAIL == rst) {
        non_uhs_dbg_print("DQS in delay set to 0 Fail \n");
        return NON_UHS_PSRAM_FAIL;
    }

#ifdef SET_DQ_DQS_DQM_DEALY_OUT_DELAY
    if (NON_UHS_PSRAM_FAIL == non_uhs_psram_Set_Reg_Out_Delay_STA()) {
        non_uhs_dbg_print("non_uhs_psram_Set_Reg_Out_Delay_STA Fail \n");
        return NON_UHS_PSRAM_FAIL;
    }
#endif /* #ifdef SET_DQ_DQS_DQM_DEALY_OUT_DELAY */

    non_uhs_dbg_print("\n*************** RX K Start, DQ delay start *****************\n");
    for (dqy_in_del = DQ_IN_DELAY_MAX; dqy_in_del >= 0; dqy_in_del -= DQ_IN_DELAY_TUNING_STEP) {
        non_uhs_dbg_print("RX K , dqy_in_del = 0x%x \n", dqy_in_del);
        rst = non_uhs_psram_set_DQ_DQS_in_delay_value(e_DEALY_DQ, dqy_in_del);
        if (NON_UHS_PSRAM_FAIL == rst) {
            non_uhs_dbg_print("DQ or DQS in delay set value Fail \n");
            return NON_UHS_PSRAM_FAIL;
        }

        if (NON_UHS_PSRAM_SUCCESS ==  non_uhs_psram_write_read_test()) {
            non_uhs_dbg_print("RX K: PASS, DQ_IN_DELAY find window, DQ delay = 0x%x \n", dqy_in_del);
            non_uhs_dbg_print("*************** RX K : DQ delay end , K Pass *************** \n");
            return NON_UHS_PSRAM_SUCCESS;
        }
    }
    non_uhs_dbg_print("*************** RX K : DQ delay end , DQ_IN_DELAY can't find window *************** \n");

    non_uhs_dbg_print("\n\n*************** RX K : DQS delay start *************** \n");
    if (dqy_in_del < 0) { //DQ_IN_DELAY can't find window
        for (dqs_in_del = 0x1; dqs_in_del <= DQS_IN_DELAY_MAX; dqs_in_del += DQS_IN_DELAY_TUNING_STEP) {
            non_uhs_dbg_print("RX K , dqs_in_del = 0x%x \n", dqs_in_del);
            rst = non_uhs_psram_set_DQ_DQS_in_delay_value(e_DEALY_DQS, dqs_in_del);
            if (NON_UHS_PSRAM_FAIL == rst) {
                return NON_UHS_PSRAM_FAIL;
            }

            if (NON_UHS_PSRAM_SUCCESS ==  non_uhs_psram_write_read_test()) {
                non_uhs_dbg_print("RX K: PASS, DQS_IN_DELAY find window, DQS delay = 0x%x \n", dqs_in_del);
                non_uhs_dbg_print("*************** RX K : DQS delay end , K Pass *************** \n");
                return  NON_UHS_PSRAM_SUCCESS;
            }
        }
    }
    non_uhs_dbg_print("*************** RX K : DQS delay end , K Fail *************** \n");

    return  NON_UHS_PSRAM_FAIL;
}

static eNon_UHS_Psram_Status non_uhs_psram_Reset_De_Assert(PSRAM_MODE_ENUM psram_type)
{
    UINT32 tmp = 0;
    U64 timeout_cnt = 0;

    if (PSRAM_MODE_PSRAM_4MB_APM == psram_type) {
        HAL_REG_32(NON_UHS_PSRAM_TIMING_2_ADDR) = 0x00000190;    /*TINFO ="Set Timing. t_fsm_lat_csh = 0x190" */
        hal_gpt_delay_us(1);
        // RESET PSRAM ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        //                       cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,       cmd0,      data3,      data2,      data1,      data0
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x0,   0x0,   0x0,   0x0,   0x4,   0x0,   0x0,    0x0,    0x1, 0x0000ff00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000);

        /*TINFO ="[PSRAM] Enter RESET state" */

        tmp = HAL_REG_32(NON_UHS_PSRAM_CTRL_STUS_ADDR);
        while ((tmp & 0x00000001) == 0x00000000) {
            /*TINFO ="wait PSRAM RESET done." */
            if (timeout_cnt * 1 > NON_UHS_PSRAM_RESET_TIMEOUT_SECOND * 1000 * 1000UL) {
                non_uhs_dbg_print("non_uhs_psram_Reset_De_assertfail,timeOut,return!\n");
                return NON_UHS_PSRAM_FAIL;
            }
            hal_gpt_delay_us(1);
            timeout_cnt++;
            tmp = HAL_REG_32(NON_UHS_PSRAM_CTRL_STUS_ADDR);
        }
        /*TINFO ="[PSRAM] RESET done & exit RESET state." */
        HAL_REG_32(NON_UHS_PSRAM_TIMING_2_ADDR) = (((HAL_REG_32(NON_UHS_PSRAM_TIMING_2_ADDR)) & 0xFF000000) | 0x00000008);  // [23:0] = 24'h8, t_fsm_lat_csh return
        /*TINFO ="Set Timing. t_fsm_lat_csh = 8" */
    } else if (PSRAM_MODE_PSRAM_8MB_APM == psram_type) {
        HAL_REG_32(NON_UHS_PSRAM_TIMING_3_ADDR) = 0x000000C9;
        /*TINFO ="Set Timing. t_rp = 0xc9" */
        HAL_REG_32(NON_UHS_PSRAM_TIMING_4_ADDR) = 0x000000192;
        /*TINFO ="Set Timing. t_rst = 0x192" */

        hal_gpt_delay_us(1);
        // RESET PSRAM ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        //                       cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,       cmd0,      data3,      data2,      data1,      data0
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x0,   0x0,   0x0,   0x0,   0x0,   0x1,   0x0,   0x0,   0x0,    0x0,    0x0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000);

        tmp = HAL_REG_32(NON_UHS_PSRAM_CTRL_STUS_ADDR);
        while ((tmp & 0x00000001) == 0x00000000) {
            /*TINFO ="wait PSRAM RESET done." */
            if (timeout_cnt * 1 > NON_UHS_PSRAM_RESET_TIMEOUT_SECOND * 1000 * 1000UL) {
                non_uhs_dbg_print("non_uhs_psram_Reset_De_assertfail,timeOut,return!\n");
                return NON_UHS_PSRAM_FAIL;
            }
            hal_gpt_delay_us(1);
            timeout_cnt++;
            tmp = HAL_REG_32(NON_UHS_PSRAM_CTRL_STUS_ADDR);
        }
        /*TINFO ="[PSRAM] RESET done & exit RESET state." */
    } else if (PSRAM_MODE_PSRAM_4MB_WB == psram_type) {
        HAL_REG_32(NON_UHS_PSRAM_TIMING_3_ADDR) = 0x00000258;
        /*TINFO ="Set Timing. t_rp = 0x258" */
        HAL_REG_32(NON_UHS_PSRAM_TIMING_4_ADDR) = 0x00007540;
        /*TINFO ="Set Timing. t_rst = 0x7540" */

        hal_gpt_delay_us(1);
        // RESET PSRAM ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        //                          cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,       cmd0,      data3,      data2,      data1,      data0
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x0,   0x0,   0x0,   0x0,   0x0,   0x1,   0x0,   0x0,   0x0,    0x0,    0x0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000);
        tmp = HAL_REG_32(NON_UHS_PSRAM_CTRL_STUS_ADDR);
        while ((tmp & 0x00000001) == 0x00000000) {
            /*TINFO ="wait PSRAM RESET done." */
            if (timeout_cnt * 1 > NON_UHS_PSRAM_RESET_TIMEOUT_SECOND * 1000 * 1000UL) {
                non_uhs_dbg_print("non_uhs_psram_Reset_De_assertfail,timeOut,return!\n");
                return NON_UHS_PSRAM_FAIL;
            }
            hal_gpt_delay_us(1);
            tmp = HAL_REG_32(NON_UHS_PSRAM_CTRL_STUS_ADDR);
            timeout_cnt++;
        }
        /*TINFO ="[PSRAM] RESET done & exit RESET state." */
    } else if (PSRAM_MODE_PSRAM_8MB_WB == psram_type) {
        HAL_REG_32(NON_UHS_PSRAM_TIMING_3_ADDR) = 0x00000258;
        /*TINFO ="Set Timing. t_rp = 0x258" */
        HAL_REG_32(NON_UHS_PSRAM_TIMING_4_ADDR) = 0x00007540;
        /*TINFO ="Set Timing. t_rst = 0x7540" */

        hal_gpt_delay_us(1);

        // RESET PSRAM ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        //                          cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,       cmd0,      data3,      data2,      data1,      data0
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x0,   0x0,   0x0,   0x0,   0x0,   0x1,   0x0,   0x0,   0x0,    0x0,    0x0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000);

        tmp = HAL_REG_32(NON_UHS_PSRAM_CTRL_STUS_ADDR);
        while ((tmp & 0x00000001) == 0x00000000) {
            /*TINFO ="wait PSRAM RESET done." */
            if (timeout_cnt * 1 > NON_UHS_PSRAM_RESET_TIMEOUT_SECOND * 1000 * 1000UL) {
                non_uhs_dbg_print("non_uhs_psram_Reset_De_assertfail,timeOut,return!\n");
                return NON_UHS_PSRAM_FAIL;
            }
            hal_gpt_delay_us(1);
            timeout_cnt++;
            tmp = HAL_REG_32(NON_UHS_PSRAM_CTRL_STUS_ADDR);
        }
        /*TINFO ="[PSRAM] RESET done & exit RESET state." */
    } else {
        non_uhs_dbg_print("Wrong non-uhs Psram type = %d,return!\n", psram_type);
        return NON_UHS_PSRAM_FAIL;
    }

    return NON_UHS_PSRAM_SUCCESS;
}

static void non_uhs_psram_global_reset_done_set(void)
{
    HAL_REG_32(MT7933_RTC_BACKUP00_ADDR) = HAL_REG_32(MT7933_RTC_BACKUP00_ADDR) | (NONUHS_PSRAM_APM_4MB_GLOBAL_RESET_DONE_FLAG << 0);
    hal_gpt_delay_us(5);
}

static U8 non_uhs_psram_global_reset_done_check(void)
{
    return ((HAL_REG_32(MT7933_RTC_BACKUP00_ADDR) & 0xFF) == NONUHS_PSRAM_APM_4MB_GLOBAL_RESET_DONE_FLAG);
}

static eNon_UHS_Psram_Status non_uhs_psram_Ctrl_CR_Set(PSRAM_MODE_ENUM psram_type)
{
    HAL_REG_32(NON_UHS_PSRAM_TOP_CFG_AON_BASE_ADDR + 0x0020) = (HAL_REG_32(NON_UHS_PSRAM_TOP_CFG_AON_BASE_ADDR + 0x0020)) | 0x80000000;
    /*TINFO ="PSRAM_MUX switch" */
    if (PSRAM_MODE_PSRAM_4MB_APM == psram_type) {
        HAL_REG_32(NON_UHS_PSRAM_AXI2PSRAM_CFG_2_ADDR) = 0xA000A03F;
        /*TINFO ="PSRAM addr range: 0xA0000000~0xA03FFFFF(4MB)." */

        if (get_chip_version() == CHIP_HW_VER_EX) {
            HAL_REG_32(NON_UHS_PSRAM_NORMAL_RW_CFG_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_NORMAL_RW_CFG_ADDR) & 0xFFFFFFF0) | 0x00000009;
            /*TINFO ="set PSRAM_CTRL cfg for normal rw, csn_low_clk_en = 1, idle_clk_en = 0, csh_clk_en = 0, csh_ckon_en = 1" */
        }

        HAL_REG_32(NON_UHS_PSRAM_CFG_0_ADDR) = ((HAL_REG_32(NON_UHS_PSRAM_CFG_0_ADDR)) & 0xFFFF0000) | 0x00005031;
        /*TINFO ="set PSRAM cfg, psram_sel = APM_E2, psram_size = 4MB, bl_max = 128, bound = 1024B" */

        if (non_uhs_psram_global_reset_done_check()) { //workaround for APM 4MB wdt reset fail issue
            HAL_REG_32(NON_UHS_PSRAM_TIMING_0_ADDR) = 0x02060000;
            /*TINFO ="Set Timing. t_fsm_lat_rd = 6, t_fsm_lat_wr = 2" */
        } else {
            HAL_REG_32(NON_UHS_PSRAM_TIMING_0_ADDR) = 0x00060000;
            /*TINFO ="Set Timing. t_fsm_lat_rd = 6" */
        }

        /*TINFO ="Set Timing. t_fsm_lat_rd = 6" */
        HAL_REG_32(NON_UHS_PSRAM_TIMING_2_ADDR) = 0x00000008;
        /*TINFO ="Set Timing. t_fsm_lat_csh = 8" */
    } else if (PSRAM_MODE_PSRAM_8MB_APM == psram_type) {
        HAL_REG_32(NON_UHS_PSRAM_AXI2PSRAM_CFG_2_ADDR) = 0xA000A07F;
        /*TINFO ="PSRAM addr range: 0xA0000000~0xA07FFFFF(8MB)." */

        HAL_REG_32(NON_UHS_PSRAM_CFG_0_ADDR) = ((HAL_REG_32(NON_UHS_PSRAM_CFG_0_ADDR)) & 0xFFFF0000) | 0x00005040;
        /*TINFO ="set PSRAM cfg, psram_sel = APM_E3, psram_size = 8MB, bl_max = 128, bound = 1024B" */

        HAL_REG_32(NON_UHS_PSRAM_TIMING_0_ADDR) = 0x06060010;
        /*TINFO ="Set Timing. t_fsm_lat_rd = 6, t_fsm_lat_wr = 6, rg_t_csh = 1" */
        HAL_REG_32(NON_UHS_PSRAM_TIMING_2_ADDR) = 0x00000007;
        /*TINFO ="Set Timing. t_fsm_lat_csh = 7" */

    } else if (PSRAM_MODE_PSRAM_4MB_WB == psram_type) {
        HAL_REG_32(NON_UHS_PSRAM_AXI2PSRAM_CFG_2_ADDR) = 0xA000A03F;
        /*TINFO ="PSRAM addr range: 0xA0000000~0xA03FFFFF(3MB)." */

        HAL_REG_32(NON_UHS_PSRAM_CFG_0_ADDR) = ((HAL_REG_32(NON_UHS_PSRAM_CFG_0_ADDR)) & 0xFFFF0000) | 0x00002032;
        /*TINFO ="set PSRAM cfg, psram_sel = WB_4MB, psram_size = 4MB, bl_max = 128, bound = 128B" */

        HAL_REG_32(NON_UHS_PSRAM_TIMING_0_ADDR) = 0x05050011;
        /*TINFO ="Set Timing. t_fsm_lat_rd = 5, t_fsm_lat_wr = 5, rg_t_csh = 1" */
        HAL_REG_32(NON_UHS_PSRAM_TIMING_2_ADDR) = 0x00000008;
        /*TINFO ="Set Timing. t_fsm_lat_csh = 8" */
    } else if (PSRAM_MODE_PSRAM_8MB_WB == psram_type) {
        HAL_REG_32(NON_UHS_PSRAM_AXI2PSRAM_CFG_2_ADDR) = 0xA000A07F;
        /*TINFO ="PSRAM addr range: 0xA0000000~0xA07FFFFF(8MB)." */

        HAL_REG_32(NON_UHS_PSRAM_CFG_0_ADDR) = ((HAL_REG_32(NON_UHS_PSRAM_CFG_0_ADDR)) & 0xFFFF0000) | 0x00005042;
        /*TINFO ="set PSRAM cfg, psram_sel = WB_8MB, psram_size = 8MB, bl_max = 128, bound = 1024B" */
        HAL_REG_32(NON_UHS_PSRAM_TIMING_0_ADDR) = 0x06060011;
        /*TINFO ="Set Timing. t_fsm_lat_rd = 6, t_fsm_lat_wr = 6, rg_t_csh = 1" */
        HAL_REG_32(NON_UHS_PSRAM_TIMING_2_ADDR) = 0x00000008;
        /*TINFO ="Set Timing. t_fsm_lat_csh = 8" */
    } else {
        non_uhs_dbg_print("Wrong non-uhs Psram type = %d,return!\n", psram_type);
        return NON_UHS_PSRAM_FAIL;
    }

    return NON_UHS_PSRAM_SUCCESS;
}

static eNon_UHS_Psram_Status non_uhs_psram_enable_DLL_init(void)
{
    U32 tmp = 0;
    U64 timeout_cnt = 0;

    /*TINFO ="PSRAM ctrl DLL init start." */
    HAL_REG_32(NON_UHS_PSRAM_DLL_CTRL_CFG_STUS_ADDR) = 0x00000f05;      // enable dll_cal_init to calibrate DLL

    tmp = HAL_REG_32(NON_UHS_PSRAM_DLL_CTRL_CFG_STUS_ADDR);     // wait calibration done
    while ((tmp & 0x00010000) == 0x00000000) {
        /*TINFO ="wait calibration done." */
        if (timeout_cnt * 1 > NON_UHS_PSRAM_DLL_INIT_TIMEOUT_SECOND * 1000 * 1000UL) {
            non_uhs_dbg_print("non_uhs_psram_DLL_Init fail,timeOut,return!\n");
            return NON_UHS_PSRAM_FAIL;
        }
        hal_gpt_delay_us(1);
        timeout_cnt++;
        tmp = HAL_REG_32(NON_UHS_PSRAM_DLL_CTRL_CFG_STUS_ADDR);
    }
    /*TINFO ="Calibration done." */

    HAL_REG_32(NON_UHS_PSRAM_DLL_CTRL_CFG_STUS_ADDR) = 0x00000f07;     // enable dll_soft_upd to update DLL value into PSRAM PHY
    hal_gpt_delay_us(1);
    HAL_REG_32(NON_UHS_PSRAM_DLL_CTRL_CFG_STUS_ADDR) = 0x00000f01;     // deassert dll_cal_init & dll_soft_upd to return FSM back to idle. The DLL cal will be executed in each specified period.

    return NON_UHS_PSRAM_SUCCESS;
}

//============================================================
// nonuhs_dll_disable
//============================================================
ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME eNon_UHS_Psram_Status non_uhs_psram_dll_disable(void)
{
    /*TINFO ="PSRAM ctrl DLL disable." */
    HAL_REG_32(NON_UHS_PSRAM_DLL_CTRL_CFG_STUS_ADDR) = (HAL_REG_32(NON_UHS_PSRAM_DLL_CTRL_CFG_STUS_ADDR)) & 0xffff0000;                               // disable dll_cal_init to calibrate DLL
    return NON_UHS_PSRAM_SUCCESS;
}

static eNon_UHS_Psram_Status non_uhs_psram_MR_Set(PSRAM_MODE_ENUM psram_type)
{
    if (PSRAM_MODE_PSRAM_4MB_APM == psram_type) {
        //                      cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,       cmd0,      data3,      data2,      data1,      data0
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x1,   0x1,   0x0,   0x0,   0x2,   0x1,   0x1,    0x0,    0x1, 0x0004C000, 0x00000000, 0x00008000, 0x00000000, 0x00000000, 0x00000000);

        (HAL_REG_32(NON_UHS_PSRAM_TIMING_0_ADDR)) = ((HAL_REG_32(NON_UHS_PSRAM_TIMING_0_ADDR)) & 0xE0FFFFFF) | 0x02000000;    // [28:24] = 5'h2, t_lat_wr = 2
        /*TINFO ="Set Timing. t_lat_wr = 2" */
        //                       cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,       cmd0,      data3,      data2,      data1,      data0

        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x2,   0x1,   0x1,    0x0,    0x1, 0x0000C000, 0x00000000, 0x00001800, 0x00000000, 0x00000000, 0x00000000);

        //non_uhs_psram_REG_CMD_RD_BLOCK(  0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x2,   0x1,   0x0,    0x0,    0x1, 0x00004000, 0x00000000, 0x0000188D, 0x00000000, 0x00000000, 0x00000000);
        //non_uhs_psram_REG_CMD_RD_BLOCK(  0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x2,   0x1,   0x0,    0x0,    0x1, 0x00044000, 0x00000000, 0x00008018, 0x00000000, 0x00000000, 0x00000000);
    } else if (PSRAM_MODE_PSRAM_8MB_APM == psram_type) {
        //                       cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,       cmd0,      data3,      data2,      data1,      data0
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0000C0C0, 0x00000000, 0x00001000, 0x00000000, 0x00000000, 0x00000000);
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0000C0C0, 0x00000004, 0x00002000, 0x00000000, 0x00000000, 0x00000000);
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0000C0C0, 0x00000008, 0x00000700, 0x00000000, 0x00000000, 0x00000000);

        //non_uhs_psram_REG_CMD_RD_BLOCK(  0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x00004040, 0x00000000, 0x0000108D, 0x00000000, 0x00000000, 0x00000000);
        //non_uhs_psram_REG_CMD_RD_BLOCK(  0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x00004040, 0x00000004, 0x00002007, 0x00000000, 0x00000000, 0x00000000);
    }  else if (PSRAM_MODE_PSRAM_4MB_WB == psram_type) {
        //                          cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,       cmd0,      data3,      data2,      data1,      data0
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x01006000, 0x00000000, 0x00008010, 0x00000000, 0x00000000, 0x00000000);
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x01006000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000);
        //non_uhs_psram_REG_CMD_RD_BLOCK(  0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0100E000, 0x00000000, 0x00008010, 0x00000000, 0x00000000, 0x00000000);
        //non_uhs_psram_REG_CMD_RD_BLOCK(  0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0100E000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000);
    } else if (PSRAM_MODE_PSRAM_8MB_WB == psram_type) {
        //                          cs_se, cmd_e, lat_e, dat_e, dum_e, csl_e, rst_e, cmd_#, dat_#, dum_#, csl_of, csh_on,       cmd1,       cmd0,      data3,      data2,      data1,      data0
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x01006000, 0x00000000, 0x00008020, 0x00000000, 0x00000000, 0x00000000);
        non_uhs_psram_REG_CMD_WR_BLOCK(0x1,   0x1,   0x0,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x01006000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000);
        //non_uhs_psram_REG_CMD_RD_BLOCK(  0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0100E000, 0x00000000, 0x00008020, 0x00000000, 0x00000000, 0x00000000);
        //non_uhs_psram_REG_CMD_RD_BLOCK(  0x1,   0x1,   0x1,   0x1,   0x0,   0x0,   0x0,   0x3,   0x1,   0x0,    0x0,    0x0, 0x0100E000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000);
    } else {
        non_uhs_dbg_print("Wrong non-uhs Psram type = %d,return!\n", psram_type);
        return NON_UHS_PSRAM_FAIL;
    }

    return NON_UHS_PSRAM_SUCCESS;
}

static void non_uhs_psram_CLK_37P5MHZ_Set(void)
{
    HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x260) = ((HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x260) | 0x01) & (0xFFFFFF0F)) | (7 << 4);
    HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x278) = (HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x278) | 0x00000001);
    hal_gpt_delay_us(1);
}

static void non_uhs_psram_CLK_100MHZ_Set(void)
{
    HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x260) = ((HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x260) | 0x01) & (0xFFFFFF0F)) | (2 << 4);
    HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x278) = (HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x278) | 0x00000001);
    hal_gpt_delay_us(1);
}

static void non_uhs_psram_CLK_200MHZ_Set(void)
{
    HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x260) = (HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x260) & (0xFFFFFFFE));
    HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x278) = (HAL_REG_32(NON_UHS_PSRAM_TOP_CLK_OFF_BASE + 0x278) | 0x00000001);
    hal_gpt_delay_us(1);
}

eNon_UHS_Psram_Status non_uhs_psram_Set_CLK(ePsram_CLK_type clk)
{
    if (e_CLK_37P5_MHZ == clk) {
        non_uhs_psram_CLK_37P5MHZ_Set();
    } else if (e_CLK_100_MHZ == clk) {
        non_uhs_psram_CLK_100MHZ_Set();
    } else if (e_CLK_200_MHZ == clk) {
        non_uhs_psram_CLK_200MHZ_Set();
    } else {
        return NON_UHS_PSRAM_FAIL;
    }
    hal_gpt_delay_us(1);

    return NON_UHS_PSRAM_SUCCESS;
}

static void non_uhs_psram_wdt_reset_enable(void)
{
    HAL_REG_32(NON_UHS_PSRAM_NORMAL_WDT_RST_CONTROL) = (HAL_REG_32(NON_UHS_PSRAM_NORMAL_WDT_RST_CONTROL) & (~(0x1 << 18)));
    hal_gpt_delay_us(5);
    HAL_REG_32(NON_UHS_PSRAM_NORMAL_WDT_RST_CONTROL) = (HAL_REG_32(NON_UHS_PSRAM_NORMAL_WDT_RST_CONTROL) & (~(0x1 << 20)));;
    hal_gpt_delay_us(5);
}

ATTR_TEXT_IN_RAM_MODE_SUSPEND_RESUME void non_uhs_psram_controller_reset(void)
{
    HAL_REG_32(NON_UHS_PSRAM_NORMAL_SW_RST_B) = (HAL_REG_32(NON_UHS_PSRAM_NORMAL_SW_RST_B) & (~(0x1 << 20)));
    hal_gpt_delay_us(5);
    HAL_REG_32(NON_UHS_PSRAM_NORMAL_SW_RST_B) = (HAL_REG_32(NON_UHS_PSRAM_NORMAL_SW_RST_B) | (0x1 << 20));
    hal_gpt_delay_us(5);
}

static int non_uhs_psram_check_vendor_ID(PSRAM_MODE_ENUM psram_type)
{
    U8 vendor_ID = 0;
    U8 vendor_ID_mr_MA = 0;
    U32 MR_value = 0;
    U8 cmp_vendor_ID = 0;

    if ((PSRAM_MODE_PSRAM_4MB_APM == psram_type) || (PSRAM_MODE_PSRAM_8MB_APM == psram_type)) {
        vendor_ID_mr_MA = 1;
        cmp_vendor_ID = 0xD;
    } else if ((PSRAM_MODE_PSRAM_4MB_WB == psram_type) || (PSRAM_MODE_PSRAM_8MB_WB == psram_type)) {
        vendor_ID_mr_MA = 2;
        if (PSRAM_MODE_PSRAM_4MB_WB == psram_type) {
            cmp_vendor_ID = 0xF;
        } else {
            cmp_vendor_ID = 0x6;
        }
    } else {
        non_uhs_dbg_print("non_uhs_psram_check_vendor_ID: wrong Psram type [%d] ! \n", psram_type);
        return -1;
    }

    if (non_uhs_psram_MRR(psram_type, vendor_ID_mr_MA, &MR_value) < 0) {
        non_uhs_dbg_print("non-uhs psram Read vendor ID Fail ! \n");
        return -1;
    }

    if ((PSRAM_MODE_PSRAM_4MB_APM == psram_type) || (PSRAM_MODE_PSRAM_8MB_APM == psram_type)) {
        vendor_ID = (U8)(((MR_value & 0x0000FF00) >> 8) & (0x1F));
    } else if ((PSRAM_MODE_PSRAM_4MB_WB == psram_type) || (PSRAM_MODE_PSRAM_8MB_WB == psram_type)) {
        vendor_ID = (U8)(MR_value & 0x0F);//[3:0] WB vendor ID,  for WB 8MB , [7:4] Size 8MB, [12:8] Size 8MB ; for WB 4MB, [6:4] Size 4MB
    } else {
        non_uhs_dbg_print("non_uhs_psram_check_vendor_ID: wrong Psram type [%d] ! \n", psram_type);
        return -1;
    }
    non_uhs_dbg_print("[Vender ID = 0x%x.]\n", vendor_ID);
    if (vendor_ID != cmp_vendor_ID) {
        non_uhs_dbg_print("Vendor ID check Fail [%d]!\n", cmp_vendor_ID);
        return -1;
    }
    non_uhs_dbg_print("Vendor ID check Pass! \n");

    return 0;
}


eNon_UHS_Psram_Status non_uhs_psram_K_Flow(PSRAM_MODE_ENUM psram_type, ePsram_CLK_type clk)
{
    if (e_CLK_37P5_MHZ == clk) {
        non_uhs_dbg_print("\nnon-uhs psram: \n");
        non_uhs_dbg_print("0. enable wdt reset psramc. \n");
        non_uhs_psram_wdt_reset_enable();

        non_uhs_dbg_print("1. set clk to 37.5MHZ. \n");
        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_Set_CLK(e_CLK_37P5_MHZ)) {
            return NON_UHS_PSRAM_FAIL;
        }

        non_uhs_dbg_print("2. Ctrl_CR_Set. \n");
        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_Ctrl_CR_Set(psram_type)) {
            return NON_UHS_PSRAM_FAIL;
        }

        if ((non_uhs_psram_global_reset_done_check()) && (PSRAM_MODE_PSRAM_4MB_APM == psram_type)) { //workaround for APM 4MB wdt reset fail issue
            non_uhs_dbg_print("3. Have global reset done, no need Reset_De_Assert. \n");
            non_uhs_dbg_print("4. Have global reset done, no need do MR_Set. \n");
        } else {
            non_uhs_dbg_print("3. no psram global reset, now Reset_De_Assert. \n");
            if (NON_UHS_PSRAM_FAIL == non_uhs_psram_Reset_De_Assert(psram_type)) {
                return NON_UHS_PSRAM_FAIL;
            }

            non_uhs_dbg_print("4. no psram global reset, now MR_Set. \n");
            if (NON_UHS_PSRAM_FAIL == non_uhs_psram_MR_Set(psram_type)) {
                return NON_UHS_PSRAM_FAIL;
            }

            non_uhs_psram_global_reset_done_set();
        }

#ifdef SECURE_SETTING_FOR_NON_UHS_ENABLE
        non_uhs_dbg_print("5. Secure Setting for psram. \n");
        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_secure_setting_for_DQDQS_delay()) {
            return NON_UHS_PSRAM_FAIL;
        }

        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_enable_DLL_init()) {
            return NON_UHS_PSRAM_FAIL;
        }

        if (NON_UHS_PSRAM_SUCCESS ==  non_uhs_psram_write_read_test()) {
            non_uhs_dbg_print("[37.5 MHz, Secure Setting Pass] \n");
            return  NON_UHS_PSRAM_SUCCESS;
        }

        non_uhs_psram_check_vendor_ID(psram_type);

        non_uhs_dbg_print("\t!!! 37.5MHz Rx calibration Fail, reset psram controller! \n");
        non_uhs_psram_controller_reset();
        return NON_UHS_PSRAM_FAIL;

        non_uhs_dbg_print("\t37.5 MHz, Secure Setting Fail, Need K RX!\n");
#endif /* #ifdef SECURE_SETTING_FOR_NON_UHS_ENABLE */
        non_uhs_dbg_print("6. dll_disable. \n");
        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_dll_disable()) {
            return NON_UHS_PSRAM_FAIL;
        }
        non_uhs_dbg_print("7. read_calibration. \n");
        get_edge_point = 0;
        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_read_calibration_flow()) {
            return NON_UHS_PSRAM_FAIL;
        }
        get_edge_point = 1;
        non_uhs_dbg_print("8. enable_DLL_init. \n");
        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_enable_DLL_init()) {
            return NON_UHS_PSRAM_FAIL;
        }
        non_uhs_dbg_print("9. write_read_test. \n");
        if (NON_UHS_PSRAM_SUCCESS ==  non_uhs_psram_write_read_test()) {
            non_uhs_dbg_print("[37.5 MHz, Secure Setting Pass]\n");
            return  NON_UHS_PSRAM_SUCCESS;
        }
        non_uhs_dbg_print("\t!!! 37.5MHz Rx calibration Fail, reset psramc controller! \n");

        return NON_UHS_PSRAM_FAIL;
    } else if (e_CLK_200_MHZ == clk) {
        non_uhs_dbg_print("\nnon-uhs psram: \n");
        if (CHIP_HW_VER_EX == get_chip_version()) {
            non_uhs_dbg_print("0. dll_disable. \n");
            if (NON_UHS_PSRAM_FAIL == non_uhs_psram_dll_disable()) {
                return NON_UHS_PSRAM_FAIL;
            }
        }

        non_uhs_dbg_print("1. set clk to 200MHZ. \n");
        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_Set_CLK(e_CLK_200_MHZ)) {
            return NON_UHS_PSRAM_FAIL;
        }

        if (CHIP_HW_VER_EX == get_chip_version()) {
            non_uhs_dbg_print("0+. enable_DLL_init. \n");
            if (NON_UHS_PSRAM_FAIL == non_uhs_psram_enable_DLL_init()) {
                return NON_UHS_PSRAM_FAIL;
            }
        }

        non_uhs_dbg_print("2. write_read_test again. \n");
        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_write_read_test()) {
            non_uhs_psram_check_vendor_ID(psram_type);
            return NON_UHS_PSRAM_FAIL;
        }
        return NON_UHS_PSRAM_SUCCESS;
    } else if (e_CLK_100_MHZ == clk) {
        non_uhs_dbg_print("\nnon-uhs psram: \n");
        non_uhs_dbg_print("1. set clk to 100MHZ. \n");
        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_Set_CLK(e_CLK_100_MHZ)) {
            return NON_UHS_PSRAM_FAIL;
        }

        non_uhs_dbg_print("2. write_read_test again. \n");
        if (NON_UHS_PSRAM_FAIL == non_uhs_psram_write_read_test()) {
            non_uhs_psram_check_vendor_ID(psram_type);
            return NON_UHS_PSRAM_FAIL;
        }
        return NON_UHS_PSRAM_SUCCESS;
    }  else {
        non_uhs_dbg_print("unsupport CLK, return \n");
        return NON_UHS_PSRAM_FAIL;
    }
}

