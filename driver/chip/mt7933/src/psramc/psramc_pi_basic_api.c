/*----------------------------------------------------------------------------*
 * Copyright Statement:                                                       *
 *                                                                            *
 *   This software/firmware and related documentation ("MediaTek Software")   *
 * are protected under international and related jurisdictions'copyright laws *
 * as unpublished works. The information contained herein is confidential and *
 * proprietary to MediaTek Inc. Without the prior written permission of       *
 * MediaTek Inc., any reproduction, modification, use or disclosure of        *
 * MediaTek Software, and information contained herein, in whole or in part,  *
 * shall be strictly prohibited.                                              *
 * MediaTek Inc. Copyright (C) 2010. All rights reserved.                     *
 *                                                                            *
 *   BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND     *
 * AGREES TO THE FOLLOWING:                                                   *
 *                                                                            *
 *   1)Any and all intellectual property rights (including without            *
 * limitation, patent, copyright, and trade secrets) in and to this           *
 * Software/firmware and related documentation ("MediaTek Software") shall    *
 * remain the exclusive property of MediaTek Inc. Any and all intellectual    *
 * property rights (including without limitation, patent, copyright, and      *
 * trade secrets) in and to any modifications and derivatives to MediaTek     *
 * Software, whoever made, shall also remain the exclusive property of        *
 * MediaTek Inc.  Nothing herein shall be construed as any transfer of any    *
 * title to any intellectual property right in MediaTek Software to Receiver. *
 *                                                                            *
 *   2)This MediaTek Software Receiver received from MediaTek Inc. and/or its *
 * representatives is provided to Receiver on an "AS IS" basis only.          *
 * MediaTek Inc. expressly disclaims all warranties, expressed or implied,    *
 * including but not limited to any implied warranties of merchantability,    *
 * non-infringement and fitness for a particular purpose and any warranties   *
 * arising out of course of performance, course of dealing or usage of trade. *
 * MediaTek Inc. does not provide any warranty whatsoever with respect to the *
 * software of any third party which may be used by, incorporated in, or      *
 * supplied with the MediaTek Software, and Receiver agrees to look only to   *
 * such third parties for any warranty claim relating thereto.  Receiver      *
 * expressly acknowledges that it is Receiver's sole responsibility to obtain *
 * from any third party all proper licenses contained in or delivered with    *
 * MediaTek Software.  MediaTek is not responsible for any MediaTek Software  *
 * releases made to Receiver's specifications or to conform to a particular   *
 * standard or open forum.                                                    *
 *                                                                            *
 *   3)Receiver further acknowledge that Receiver may, either presently       *
 * and/or in the future, instruct MediaTek Inc. to assist it in the           *
 * development and the implementation, in accordance with Receiver's designs, *
 * of certain softwares relating to Receiver's product(s) (the "Services").   *
 * Except as may be otherwise agreed to in writing, no warranties of any      *
 * kind, whether express or implied, are given by MediaTek Inc. with respect  *
 * to the Services provided, and the Services are provided on an "AS IS"      *
 * basis. Receiver further acknowledges that the Services may contain errors  *
 * that testing is important and it is solely responsible for fully testing   *
 * the Services and/or derivatives thereof before they are used, sublicensed  *
 * or distributed. Should there be any third party action brought against     *
 * MediaTek Inc. arising out of or relating to the Services, Receiver agree   *
 * to fully indemnify and hold MediaTek Inc. harmless.  If the parties        *
 * mutually agree to enter into or continue a business relationship or other  *
 * arrangement, the terms and conditions set forth herein shall remain        *
 * effective and, unless explicitly stated otherwise, shall prevail in the    *
 * event of a conflict in the terms in any agreements entered into between    *
 * the parties.                                                               *
 *                                                                            *
 *   4)Receiver's sole and exclusive remedy and MediaTek Inc.'s entire and    *
 * cumulative liability with respect to MediaTek Software released hereunder  *
 * will be, at MediaTek Inc.'s sole discretion, to replace or revise the      *
 * MediaTek Software at issue.                                                *
 *                                                                            *
 *   5)The transaction contemplated hereunder shall be construed in           *
 * accordance with the laws of Singapore, excluding its conflict of laws      *
 * principles.  Any disputes, controversies or claims arising thereof and     *
 * related thereto shall be settled via arbitration in Singapore, under the   *
 * then current rules of the International Chamber of Commerce (ICC).  The    *
 * arbitration shall be conducted in English. The awards of the arbitration   *
 * shall be final and binding upon both parties and shall be entered and      *
 * enforceable in any court of competent jurisdiction.                        *
 *---------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------
 *
 * $Author: zhishang.liu $
 * $Date: 2019/24/5 $
 * $RCSfile: pi_basic_api.c,v $
 * $Revision: #1 $
 *
 *---------------------------------------------------------------------------*/

/** @file pi_basic_api.c
 *  Basic PSRAMC API implementation
 */

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
//#include "..\Common\pd_common.h"
//#include "Register.h"
#if __ETT__
#include "processor.h"
#endif /* #if __ETT__ */
#include "dramc_common.h"
#include "x_hal_io.h"
#include "hal_spm.h"
#include "hal_sys.h"
#if (FOR_DV_SIMULATION_USED == 0)
#include "emi.h"
#endif /* #if (FOR_DV_SIMULATION_USED == 0) */

#if SUPPORT_TYPE_PSRAM
typedef struct _ACTimePsram_T {
    U8 dramType;
    U16 freq;
    U8 readLat, writeLat;
    U8 dqsinctl, datlat; //DQSINCTL, DATLAT aren't in ACTiming excel file

    //CE_CNT0===================================
U16 tce_mrw : Fld_wid(CE_CNT0_TCE_MRW);
U16 tce_hsleepx : Fld_wid(CE_CNT0_TCE_HSLEEPX);
U16 tce_wr : Fld_wid(CE_CNT0_TCE_WR);
U16 tce_rd : Fld_wid(CE_CNT0_TCE_RD);

    //SHU_ACTIM0 ===================================
U16 tcem : Fld_wid(SHU_ACTIM0_TCEM);
U16 txhs : Fld_wid(SHU_ACTIM0_TXHS);
U16 trc : Fld_wid(SHU_ACTIM0_TRC);
U16 trfc : Fld_wid(SHU_ACTIM0_TRFC);

    //SHU_ACTIM1 ===================================
U16 tcphw : Fld_wid(SHU_ACTIM1_TCPHW);
U16 tcphr_l : Fld_wid(SHU_ACTIM1_TCPHR_L);
U16 tcphr_s : Fld_wid(SHU_ACTIM1_TCPHR_S);

    //SHU_ACTIM2 ===================================
U16 tmrw : Fld_wid(SHU_ACTIM2_TMRW);

    //REFCNT=======================================
U16 trefcnt_fr_clk1: Fld_wid(REFCNT_FR_CLK1_REFCNT_FR_CLK_1X);
U16 trefcnt_fr_clk2: Fld_wid(REFCNT_FR_CLK1_REFCNT_FR_CLK_2X);

    //SHU_ACTIM3 ===================================
U16 tzqcs : Fld_wid(SHU_ACTIM3_TZQCS);
U16 tzqcl : Fld_wid(SHU_ACTIM3_TZQCL);

    //SHU_ACTIM4 ===================================
U16 txrefcnt : Fld_wid(SHU_ACTIM4_TXREFCNT);

    //CE_ CNT_05T ===================================
U16 tce_mrw_05T : Fld_wid(CE_CNT_05T_TCE_MRW_05T);
U16 tce_hsleepx_05T : Fld_wid(CE_CNT_05T_TCE_HSLEEPX_05T);
U16 tce_wr_05T : Fld_wid(CE_CNT_05T_TCE_WR_05T);
U16 tce_rd_05T : Fld_wid(CE_CNT_05T_TCE_RD_05T);

    //AC_TIMEING_05T ===================================
U16 tmrw_05T : Fld_wid(PSRAMC_SHU_AC_TIME_05T_TMRW_05T);
U16 tcphw_05T : Fld_wid(PSRAMC_SHU_AC_TIME_05T_TCPHW_05T);
U16 tcphr_l_05T : Fld_wid(PSRAMC_SHU_AC_TIME_05T_TCPHR_L_05T);
U16 tcphr_s_05T : Fld_wid(PSRAMC_SHU_AC_TIME_05T_TCPHR_S_05T);
U16 tcem_05T : Fld_wid(PSRAMC_SHU_AC_TIME_05T_TCEM_05T);
U16 txhs_05T : Fld_wid(PSRAMC_SHU_AC_TIME_05T_TXHS_05T);
U16 trc_05T : Fld_wid(PSRAMC_SHU_AC_TIME_05T_TRC_05T);
U16 trfc_05T : Fld_wid(PSRAMC_SHU_AC_TIME_05T_TRFC_05T);

    //AC_TIMEING_05T_B ===================================
U16 tzqcs_05T : Fld_wid(SHU_AC_TIME_05T_B_TZQCS_05T);
U16 tzqcl_05T : Fld_wid(SHU_AC_TIME_05T_B_TZQCL_05T);

} ACTimePsram_T;

#if SUPPORT_PSRAM_256M
const ACTimePsram_T ACTimingPsramTbl[2] = {
    //PSRAM 1600/////
    {
        .dramType = TYPE_PSRAM, .freq = 1600,
        //.readLat = 40,    .writeLat = 16,
        .tce_mrw = 1,
        .tce_wr = 5, .tce_rd = 10,
        .tce_hsleepx = 20,
        .tcem = 0,   .trfc = 18,
        .trc = 12,   .txhs = 20,
        .tcphr_l = 14,   .tcphr_s = 11,
        .tcphw = 10,    .tmrw = 20,
        .tzqcl = 200,   .tzqcs = 100,
        .txrefcnt = 26,

        .trefcnt_fr_clk1 = 101,
        .trefcnt_fr_clk2 = 202,

        .tce_wr_05T = 0,    .tce_rd_05T = 0,
        .tce_hsleepx_05T = 1, .tce_mrw_05T = 1,

        .tcem_05T = 1,  .trfc_05T = 0,
        .trc_05T = 0,   .txhs_05T = 0,

        .tcphr_l_05T = 0,   .tcphr_s_05T = 0,
        .tcphw_05T = 1, .tmrw_05T = 1,

        .tzqcl_05T = 0, .tzqcs_05T = 0

        //DQSINCTL, DATLAT aren't in ACTiming excel file
        //.dqsinctl = 7,     .datlat = 18
    },

    // PSRAM 2133/////
    {
        .dramType = TYPE_PSRAM, .freq = 2133,
        //.readLat = 40,    .writeLat = 16,

        .tce_mrw = 1,
        .tce_wr = 6, .tce_rd = 12,
        .tce_hsleepx = 26,
        .tcem = 0,   .trfc = 23,
        .trc = 16,   .txhs = 26,
        .tcphr_l = 17,   .tcphr_s = 13,
        .tcphw = 13,    .tmrw = 26,
        .tzqcl = 261,   .tzqcs = 130,
        .txrefcnt = 33,

        .trefcnt_fr_clk1 = 101,
        .trefcnt_fr_clk2 = 202,

        .tce_wr_05T = 0,    .tce_rd_05T = 1,
        .tce_hsleepx_05T = 1, .tce_mrw_05T = 1,

        .tcem_05T = 1,  .trfc_05T = 1,
        .trc_05T = 0,   .txhs_05T = 1,

        .tcphr_l_05T = 1,   .tcphr_s_05T = 1,
        .tcphw_05T = 1, .tmrw_05T = 1,

        .tzqcl_05T = 0, .tzqcs_05T = 1

        //DQSINCTL, DATLAT aren't in ACTiming excel file
        //.dqsinctl = 7,     .datlat = 18
    },
};
#else /* #if SUPPORT_PSRAM_256M */
const ACTimePsram_T ACTimingPsramTbl[2] = {
    //PSRAM 1600/////
    {
        .dramType = TYPE_PSRAM, .freq = 1600,
        //.readLat = 40,    .writeLat = 16,
        .tce_mrw = 1,
        .tce_wr = 5, .tce_rd = 10,
        .tce_hsleepx = 20,
        .tcem = 0,   .trfc = 12,
        .trc = 12,   .txhs = 20,
        .tcphr_l = 14,   .tcphr_s = 11,
        .tcphw = 10,    .tmrw = 20,
        .tzqcl = 200,   .tzqcs = 100,
        .txrefcnt = 20,

        .trefcnt_fr_clk1 = 101,
        .trefcnt_fr_clk2 = 202,

        .tce_wr_05T = 0,    .tce_rd_05T = 0,
        .tce_hsleepx_05T = 1, .tce_mrw_05T = 1,

        .tcem_05T = 1,  .trfc_05T = 0,
        .trc_05T = 0,   .txhs_05T = 0,

        .tcphr_l_05T = 0,   .tcphr_s_05T = 0,
        .tcphw_05T = 1, .tmrw_05T = 1,

        .tzqcl_05T = 0, .tzqcs_05T = 0

        //DQSINCTL, DATLAT aren't in ACTiming excel file
        //.dqsinctl = 7,     .datlat = 18
    },

    // PSRAM 2133/////
    {
        .dramType = TYPE_PSRAM, .freq = 2133,
        //.readLat = 40,    .writeLat = 16,

        .tce_mrw = 1,
        .tce_wr = 6, .tce_rd = 12,
        .tce_hsleepx = 26,
        .tcem = 0,   .trfc = 16,
        .trc = 16,   .txhs = 26,
        .tcphr_l = 17,   .tcphr_s = 13,
        .tcphw = 13,    .tmrw = 26,
        .tzqcl = 261,   .tzqcs = 130,
        .txrefcnt = 25,

        .trefcnt_fr_clk1 = 101,
        .trefcnt_fr_clk2 = 202,

        .tce_wr_05T = 0,    .tce_rd_05T = 0,
        .tce_hsleepx_05T = 1, .tce_mrw_05T = 1,

        .tcem_05T = 1,  .trfc_05T = 0,
        .trc_05T = 0,   .txhs_05T = 1,

        .tcphr_l_05T = 1,   .tcphr_s_05T = 1,
        .tcphw_05T = 1, .tmrw_05T = 1,

        .tzqcl_05T = 0, .tzqcs_05T = 1

        //DQSINCTL, DATLAT aren't in ACTiming excel file
        //.dqsinctl = 7,     .datlat = 18
    },
};
#endif /* #if SUPPORT_PSRAM_256M */

const U8 uiPSRAM_DQ_Mapping_POP[CHANNEL_NUM][8] = {
    //CH-A
    {
        5, 4, 7, 6, 2, 3, 0, 1
    },
};

//MRR PSRAM->PSRAMC
static const U8 uiPSRAM_MRR_Mapping_POP[CHANNEL_NUM][8] = {
    //CH-A
    {
        0, 1, 2, 3, 4, 5, 6, 7
    },
};

extern void PsramcNewDutyCalibration(DRAMC_CTX_T *p);

static void Set_PSRAM_MRR_Pinmux_Mapping(DRAMC_CTX_T *p)
{
    //U8 *uiPSRAM_MRR_Mapping = NULL;
    U8 backup_channel;
    DRAM_CHANNEL_T chIdx = CHANNEL_A;

    //Backup channel
    backup_channel = vGetPHY2ChannelMapping(p);

    for (chIdx = CHANNEL_A; chIdx < (DRAM_CHANNEL_T)(p->support_channel_num); chIdx++) {
        vSetPHY2ChannelMapping(p, chIdx);
        //Set MRR pin mux
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_MRR_BIT_MUX1),
                           P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][0], MRR_BIT_MUX1_MRR_BIT0_SEL) |
                           P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][1], MRR_BIT_MUX1_MRR_BIT1_SEL) |
                           P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][2], MRR_BIT_MUX1_MRR_BIT2_SEL) |
                           P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][3], MRR_BIT_MUX1_MRR_BIT3_SEL));
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_MRR_BIT_MUX2),
                           P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][4], MRR_BIT_MUX2_MRR_BIT4_SEL) |
                           P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][5], MRR_BIT_MUX2_MRR_BIT5_SEL) |
                           P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][6], MRR_BIT_MUX2_MRR_BIT6_SEL) |
                           P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][7], MRR_BIT_MUX2_MRR_BIT7_SEL));
    }
    //Recover channel
    vSetPHY2ChannelMapping(p, backup_channel);
}

#if DUMP_REG
void dump_ddrphy_reg()
{
    unsigned int base, addr, len;

    base = Channel_A_PHY_AO_BASE_ADDRESS;
    len = 0x1FCC;

    mcSHOW_CHECK_RG("=============== Dump PHY AO REG ============\n");
    for (addr = base; addr <= (base + len); addr += 4) {
        mcSHOW_CHECK_RG("Reg(0x%xh) Address 0x%X = 0x%X\n",
                        (addr - base) / 4, addr, *((volatile unsigned int *)addr));
    }

    base = Channel_A_PHY_NAO_BASE_ADDRESS;
    len = 0x00220;

    mcSHOW_CHECK_RG("=============== Dump PHY NAO REG ============\n");
    for (addr = base; addr <= (base + len); addr += 4) {
        mcSHOW_CHECK_RG("Reg(0x%xh) Address 0x%X = 0x%X\n",
                        (addr - base) / 4, addr, *((volatile unsigned int *)addr));
    }
}

void dump_psramc_reg()
{
    unsigned int base, addr, len;

    base = Channel_A_PSRAM_AO_BASE_ADDRESS;
    len = 0x007D4;

    mcSHOW_CHECK_RG("=============== Dump PSRAMC AO REG ============\n");
    for (addr = base; addr <= (base + len); addr += 4) {
        mcSHOW_CHECK_RG("Reg(0x%xh) Address 0x%X = 0x%X\n",
                        (addr - base) / 4, addr, *((volatile unsigned int *)addr));
    }
    base = Channel_A_PSRAM_NAO_BASE_ADDRESS;
    len = 0x0073C;

    mcSHOW_CHECK_RG("=============== Dump PSRAMC NAO REG ============\n");
    for (addr = base; addr <= (base + len); addr += 4) {
        mcSHOW_CHECK_RG("Reg(0x%xh) Address 0x%X = 0x%X\n",
                        (addr - base) / 4, addr, *((volatile unsigned int *)addr));
    }
}
#endif /* #if DUMP_REG */

U32 u4gpRegBackupVlaue[64];
void DramcBackupRegisters(DRAMC_CTX_T *p, U32 *backup_addr, U32 backup_num)
{
    U32 u4RegIdx;

#if __ETT__
    if (backup_num > 64) {
        mcSHOW_ERR_MSG("[DramcBackupRegisters] backup number over 64!!!\n");
        while (1);
    }
#endif /* #if __ETT__ */

    for (u4RegIdx = 0; u4RegIdx < backup_num; u4RegIdx++) {
        u4gpRegBackupVlaue[u4RegIdx] = u4IO32Read4B(backup_addr[u4RegIdx]);
    }
}

void DramcRestoreRegisters(DRAMC_CTX_T *p, U32 *restore_addr, U32 restore_num)
{
    U32 u4RegIdx;

    for (u4RegIdx = 0; u4RegIdx < restore_num; u4RegIdx++) {
        vIO32Write4B(restore_addr[u4RegIdx], u4gpRegBackupVlaue[u4RegIdx]);
    }
}

#define CKGEN_FMETER 0x0
#define ABIST_FMETER 0x1

/*
1. Select meter clock input: CLK_DBG_CFG[1:0] = 0x0
2. Select clock source from below table: CLK_DBG_CFG[21:16] = 0x39
3. Setup meter div: CLK_MISC_CFG_0[31:24] = 0x0
4. Enable frequency meter: CLK26CALI_0[12] = 0x1
5. Trigger frequency meter: CLK26CALI_0[4] = 0x1
6. Wait until CLK26CALI_0[4] = 0x0
7. Read meter count: CLK26CALI_1[15:0]
8. Calculate measured frequency: freq. = (26 * cal_cnt) / 1024

DE: Mas Lin
*/
U16 gddrphyfmeter_value = 0;
void DDRPhyFMeter_Init(void)
{
    mcSHOW_DBG_MSG("DDRPhyFMeter_Init: MT7933 UHS Psram do not support FMeter,return. \n");
    return;
}

U16 DDRPhyFMeter(void)
{
    return gddrphyfmeter_value;
}


#if ENABLE_DDRPHY_FREQ_METER
void DDRPhyFreqMeter(void)
{
    mcSHOW_DBG_MSG("DDRPhyFreqMeter: MT7933 UHS Psram do not support FMeter,return. \n");
    return;
}
#endif /* #if ENABLE_DDRPHY_FREQ_METER */

#ifdef DDR_INIT_TIME_PROFILING
U32 l_low_tick0, l_high_tick0, l_low_tick1, l_high_tick1;
void TimeProfileBegin(void)
{
#if __ETT__
    l_low_tick0 = GPT_GetTickCount(&l_high_tick0);
#else /* #if __ETT__ */
    l_low_tick0 = get_timer(0);
#endif /* #if __ETT__ */
}

U32 TimeProfileEnd(void)
{
#if __ETT__
    l_low_tick1 = GPT_GetTickCount(&l_high_tick1);
    return ((l_low_tick1 - l_low_tick0) * 76) / 1000;
#else /* #if __ETT__ */
    l_low_tick1 = get_timer(l_low_tick0);
    return l_low_tick1 * 1000;
#endif /* #if __ETT__ */
}
#endif /* #ifdef DDR_INIT_TIME_PROFILING */

#ifdef DUMMY_READ_FOR_TRACKING
void PsramcDummyReadAddressSetting(DRAMC_CTX_T *p)
{
    U8 backup_channel = p->channel, backup_rank = p->rank;
    U8 channelIdx, rankIdx;
    dram_addr_t dram_addr;

    for (channelIdx = CHANNEL_A; channelIdx < CHANNEL_NUM; channelIdx++) {
        vSetPHY2ChannelMapping(p, channelIdx);
        for (rankIdx = RANK_0; rankIdx < p->support_rank_num; rankIdx++) {
            vSetRank(p, rankIdx);

            dram_addr.ch = channelIdx;
            dram_addr.rk = rankIdx;
            dram_addr.col = 0;
            dram_addr.row = 0;
            dram_addr.bk = 0;
            //get_dummy_read_addr(&dram_addr);
            mcSHOW_DBG_MSG3("=== dummy read address: CH_%d, RK%d, row: 0x%x, bk: %d, col: 0x%x\n\n",
                            channelIdx, rankIdx, dram_addr.row, dram_addr.bk, dram_addr.col);

            vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_RK_DUMMY_RD_ADR),
                               P_Fld(dram_addr.col, RK_DUMMY_RD_ADR_DMY_RD_COL_ADR) |
                               P_Fld(0, RK_DUMMY_RD_ADR_DMY_RD_LEN));
            vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_RK_DUMMY_RD_ADR2),
                               P_Fld(dram_addr.bk, RK_DUMMY_RD_ADR2_DMY_RD_BK) |
                               P_Fld(dram_addr.row, RK_DUMMY_RD_ADR2_DMY_RD_ROW_ADR));
        }
    }

    vSetPHY2ChannelMapping(p, backup_channel);
    vSetRank(p, backup_rank);
}

void PsramcDummyReadForTrackingEnable(DRAMC_CTX_T *p)
{
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(1, "//PsramcDummyReadForTrackingEnable start\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

    vIO32WriteFldAlign_All(PSRAMC_REG_DUMMY_RD, 1, DUMMY_RD_RANK_NUM);
    /* Dummy read pattern (Better efficiency during rx dly tracking) DE: YH Tsai, Wei-jen */
    vIO32Write4B_All(PSRAMC_REG_RK_DUMMY_RD_WDATA0, 0xAAAA5555); // Field RK0_DUMMY_RD_WDATA0_DMY_RD_RK0_WDATA0
    vIO32Write4B_All(PSRAMC_REG_RK_DUMMY_RD_WDATA1, 0xAAAA5555); // Field RK0_DUMMY_RD_WDATA1_DMY_RD_RK0_WDATA1
    vIO32Write4B_All(PSRAMC_REG_RK_DUMMY_RD_WDATA2, 0xAAAA5555); // Field RK0_DUMMY_RD_WDATA2_DMY_RD_RK0_WDATA2
    vIO32Write4B_All(PSRAMC_REG_RK_DUMMY_RD_WDATA3, 0xAAAA5555); // Field RK0_DUMMY_RD_WDATA3_DMY_RD_RK0_WDATA3

    PsramcDummyReadAddressSetting(p);

    /* DUMMY_RD_RX_TRACK = 1:
     * During "RX input delay tracking enable" and "DUMMY_RD_EN=1" Dummy read will force a read command to a certain rank,
     * ignoring whether or not EMI has executed a read command to that certain rank in the past 4us.
     */
    if (p->frequency > 800) {
        vIO32WriteFldAlign_All(PSRAMC_REG_DUMMY_RD, 1, DUMMY_RD_DUMMY_RD_EN);
        mcSHOW_DBG_MSG("High Freq DUMMY_READ_FOR_TRACKING: ON\n");
    } else {
        mcSHOW_DBG_MSG("Low Freq DUMMY_READ_FOR_TRACKING: OFF\n");
    }
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(0, "//PsramcDummyReadForTrackingEnable end\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */
    return;
}

#endif /* #ifdef DUMMY_READ_FOR_TRACKING */

void Psram_Global_Option_Init2(DRAMC_CTX_T *p)
{
    //U8 u1rank_num = 0;

#if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0)
    //EMI_SETTINGS *emi_set;

    //emi_set = get_emi_setting();

    //mcSHOW_DBG_MSG("Rank info CONA[0x%x]\n", emi_set->EMI_CONA_VAL);

#if 0//(fcFOR_CHIP_ID == fcSchubert)
    if (emi_set->PIN_MUX_TYPE) {
        p->bDLP3 = 1;
        mcSHOW_DBG_MSG("\n\nusing DLP3\n\n");
    }
#endif /* #if 0//(fcFOR_CHIP_ID == fcSchubert) */

    //p->vendor_id = emi_set->iLPDDRX_MODE_REG_5;

    //u1rank_num = ((emi_set->EMI_CONA_VAL >> 17) & 0x1)==1 ? 0 : 1;
#endif /* #if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0) */
    Set_PSRAM_MRR_Pinmux_Mapping(p);
}

void vPsramcInit_PreSettings(DRAMC_CTX_T *p)
{
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(1, "//vPsramcInit_PreSettings Start\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */
    /* PAD_RRESETB control sequence */
    //remove twice dram reset pin pulse before dram power on sequence flow
    vIO32WriteFldMulti(DDRPHY_CA_CMD8, P_Fld(0x0, CA_CMD8_RG_TX_RRESETB_PULL_UP)
                       | P_Fld(0x0, CA_CMD8_RG_TX_RRESETB_PULL_DN)
                       | P_Fld(0x1, CA_CMD8_RG_TX_RRESETB_DDR3_SEL)
                       | P_Fld(0x0, CA_CMD8_RG_TX_RRESETB_DDR4_SEL)
                       | P_Fld(0xa, CA_CMD8_RG_RRESETB_DRVP)
                       | P_Fld(0xa, CA_CMD8_RG_RRESETB_DRVN));
    vIO32WriteFldAlign(DDRPHY_MISC_CTRL1, 0x1, MISC_CTRL1_R_DMRRESETB_I_OPT); //Change to glitch-free path
    //replace DDRCONF0_GDDR3RST with MISC_CTRL1_R_DMDA_RRESETB_I
    vIO32WriteFldAlign(DDRPHY_MISC_CTRL1, 0x1, MISC_CTRL1_R_DMDA_RRESETB_I);
    vIO32WriteFldAlign(DDRPHY_MISC_CTRL1, 0x1, MISC_CTRL1_R_DMDA_RRESETB_E);
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(0, "//vPsramcInit_PreSettings End\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

    return;
}

void PsramPhyFreqSel(DRAMC_CTX_T *p, DRAM_PLL_FREQ_SEL_T sel)
{
    p->freq_sel = sel;

    switch (p->freq_sel) {
        case PSRAM_2133:
            p->frequency = 1066;
            break;
        case PSRAM_1600:
            p->frequency = 800;
            break;
        default:
            p->frequency = 800;
            break;
    }

    if (p->frequency <= 800) { // DDR1600, DDR800
        p->freqGroup = 800;
    } else if (p->frequency <= 1200) { //DDR2133 DDR2280
        p->freqGroup = 1200;
    } else {
        p->freqGroup = 800;
    }
    //gu4TermFreq = 1200; //This is for TX per BIT k used
    mcSHOW_DBG_MSG3("[setFreqGroup] p-> frequency %u, freqGroup: %u\n", p->frequency, p->freqGroup);
}

void PSramcEngine2SetPat(DRAMC_CTX_T *p, U8 testaudpat, U8 log2loopcount, U8 Use_Len1_Flag)
{

    if ((Use_Len1_Flag != 0) && (testaudpat != TEST_AUDIO_PATTERN)) {
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4),
                           P_Fld(1, TEST2_A4_TEST_REQ_LEN1));   //test agent 2 with cmd length = 0
    } else {
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4),
                           P_Fld(0, TEST2_A4_TEST_REQ_LEN1));   //test agent 2 with cmd length = 0
    }

    if (testaudpat == TEST_XTALK_PATTERN) {  // xtalk
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
                           P_Fld(0, TEST2_A3_TESTAUDPAT) |
                           P_Fld(0, TEST2_A3_AUTO_GEN_PAT) |
                           P_Fld(0, TEST2_A3_HFIDPAT) |
                           P_Fld(0, TEST2_A3_TEST_AID_EN));
        // select XTALK pattern
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4),
                           P_Fld(0, TEST2_A4_TESTDMITGLPAT) |
                           P_Fld(1, TEST2_A4_TESTXTALKPAT)); //dont use audio pattern

    } else if (testaudpat == TEST_AUDIO_PATTERN) { // audio
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4),
                           P_Fld(0, TEST2_A4_TESTAUDBITINV) |
                           P_Fld(0, TEST2_A4_TESTXTALKPAT));
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
                           P_Fld(1, TEST2_A3_TESTAUDPAT) |
                           P_Fld(0, TEST2_A3_AUTO_GEN_PAT) |
                           P_Fld(0, TEST2_A3_HFIDPAT) |
                           P_Fld(0, TEST2_A3_TEST_AID_EN));
    } else { // ISI
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4),
                           P_Fld(0, TEST2_A4_TESTDMITGLPAT) |
                           P_Fld(0, TEST2_A4_TESTXTALKPAT));
        // select ISI pattern
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
                           P_Fld(0, TEST2_A3_TESTAUDPAT) |
                           P_Fld(1, TEST2_A3_AUTO_GEN_PAT) |
                           P_Fld(1, TEST2_A3_HFIDPAT) |
                           P_Fld(1, TEST2_A3_TEST_AID_EN));
    }
}

DRAM_STATUS_T PSramcEngine2Init(DRAMC_CTX_T *p, U32 u4Base, U32 u4Offset, U8 testaudpat, U8 log2loopcount)
{
    U8 Use_Len1_Flag;

    // error handling
    if (!p) {
        mcSHOW_ERR_MSG("context is NULL\n");
        return DRAM_FAIL;
    }

    // check loop number validness
    //    if ((log2loopcount > 15) || (log2loopcount < 0))      // U8 >=0 always.
    if (log2loopcount > 15) {
        mcSHOW_ERR_MSG("wrong param: log2loopcount > 15\n");
        return DRAM_FAIL;
    }

    Use_Len1_Flag = (testaudpat & 0x80) >> 7;   //len1 mapping to (testpattern + 8) or testpattern
    testaudpat = testaudpat & 0x7f;

    /////attention1 lzs mark here below are test agents setting, maybe should move into K flow code
    vIO32WriteFldAlign(PSRAMC_REG_TEST2_A0, 0x1, TEST2_A0_WDT_BY_DRAMC_DIS);
    vIO32WriteFldMulti(PSRAMC_REG_TEST2_A3, P_Fld(0x1, TEST2_A3_TEST2WREN2_HW_EN)
                       | P_Fld(0x1, TEST2_A3_TESTCLKRUN));
    //vIO32WriteFldMulti(PSRAMC_REG_TEST2_A4, P_Fld(0x0, TEST2_A4_TESTXTALKPAT)
    //          | P_Fld(0x11, TEST2_A4_TESTAUDINIT));
    //vIO32WriteFldAlign(PSRAMC_REG_RK_TEST2_A1, 0x10000, RK_TEST2_A1_TEST2_BASE);
    ////////end attention1 /////////////////////////


    // 1.set pattern ,base address ,offset address
    // 2.select  ISI pattern or audio pattern or xtalk pattern
    // 3.set loop number
    // 4.enable read or write
    // 5.loop to check DM_CMP_CPT
    // 6.return CMP_ERR
    // currently only implement ucengine_status = 1, others are left for future extension

    // 1
    // vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4), P_Fld(test2_1>>24,TEST2_A4_TESTXTALKPAT)|P_Fld(test2_2>>24,TEST2_0_TEST2_PAT1));

#if (FOR_DV_SIMULATION_USED == 1 || SW_CHANGE_FOR_SIMULATION == 1)
    //DV sim memory 0~0x100 has values, can't used
    vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_RK_TEST2_A1), (u4Base) & 0x00ffffff, RK_TEST2_A1_TEST2_BASE);
#else /* #if (FOR_DV_SIMULATION_USED == 1 || SW_CHANGE_FOR_SIMULATION == 1) */
    vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_RK_TEST2_A1), (u4Base + 0x200000) & 0x00ffffff, RK_TEST2_A1_TEST2_BASE);
#endif /* #if (FOR_DV_SIMULATION_USED == 1 || SW_CHANGE_FOR_SIMULATION == 1) */
    vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A2), u4Offset & 0x00ffffff, TEST2_A2_TEST2_OFF);  //lzs,  this should set 0x20 after K

    // 2 & 3
    // (TESTXTALKPAT, TESTAUDPAT) = 00 (ISI), 01 (AUD), 10 (XTALK), 11 (UNKNOWN)
    PSramcEngine2SetPat(p, testaudpat, log2loopcount, Use_Len1_Flag);

    return DRAM_OK;
}

void PSramcEngine2CheckComplete(DRAMC_CTX_T *p)
{
    U32 u4loop_count = 0;

    while (u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_TESTRPT), TESTRPT_DM_CMP_CPT_RK0) != 1) {
        mcDELAY_US(1);
        u4loop_count++;
        if ((u4loop_count > 3) && (u4loop_count <= MAX_CMP_CPT_WAIT_LOOP)) {
            // mcSHOW_ERR_MSG("PSRAMC_REG_TESTRPT: %d\n", u4loop_count);
        } else if (u4loop_count > MAX_CMP_CPT_WAIT_LOOP) {
            /*TINFO="fcWAVEFORM_MEASURE_A %d: time out\n", u4loop_count*/
            mcSHOW_DBG_MSG("PSramcEngine2CheckComplete %d :time out\n", u4loop_count);
            mcFPRINTF(fp_A60501, "PSramcEngine2CheckComplete %d: time out\n", u4loop_count);
            break;
        }
    }
}

U32 PSramcEngine2Compare(DRAMC_CTX_T *p, DRAM_TE_OP_T wr)
{
    U32 u4result = 0xffffffff;
    U8  u1status = 1; //RK0

    if (wr == TE_OP_WRITE_READ_CHECK) {
        // read data compare ready check
        PSramcEngine2CheckComplete(p);

        // disable write
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
                           P_Fld(0, TEST2_A3_TEST2W) |
                           P_Fld(0, TEST2_A3_TEST2R) |
                           P_Fld(0, TEST2_A3_TEST1));

        mcDELAY_US(1);

        // enable read
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
                           P_Fld(0, TEST2_A3_TEST2W) |
                           P_Fld(1, TEST2_A3_TEST2R) |
                           P_Fld(0, TEST2_A3_TEST1));
    }

    // 5
    // read data compare ready check
    PSramcEngine2CheckComplete(p);

    // delay 10ns after ready check from DE suggestion (1ms here)
    //mcDELAY_US(1);

    u4result = (u4IO32Read4B(DRAMC_REG_ADDR(PSRAMC_REG_TESTRPT)) >> 4) & u1status; //CMP_ERR_RK0

    return u4result;
}

U32 PSramcEngine2Run(DRAMC_CTX_T *p, DRAM_TE_OP_T wr, U8 testaudpat)
{
    U32 u4result = 0xffffffff;

    // 4
    if (wr == TE_OP_READ_CHECK) {
        // enable read,
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3), P_Fld(0, TEST2_A3_TEST2W)
                           | P_Fld(1, TEST2_A3_TEST2R));
    } else if (wr == TE_OP_WRITE_READ_CHECK) {
        // enable write
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3), P_Fld(1, TEST2_A3_TEST2W)
                           | P_Fld(0, TEST2_A3_TEST2R));
    }
    //5
    PSramcEngine2CheckComplete(p);

    // delay 10ns after ready check from DE suggestion (1ms here)
    mcDELAY_US(1);

    // 6
    // return CMP_ERR, 0 is ok ,others are fail,diable test2w or test2r
    // get result
    // or all result
    u4result = (u4IO32Read4B(DRAMC_REG_ADDR(PSRAMC_REG_CMP_ERR)));
    // disable read
    vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3), P_Fld(0, TEST2_A3_TEST2W)
                       | P_Fld(0, TEST2_A3_TEST2R));

    return ((u4result >> 8) | (u4result & 0xff));
}

void PSramcEngine2End(DRAMC_CTX_T *p)
{
    vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3), P_Fld(0, TEST2_A3_TEST2W));
}

#if 0
static U32 TestPSramEngineCompare(DRAMC_CTX_T *p)
{
    //U8 jj;
    U32 u4err_value;

    //  if(p->test_pattern <= TEST_XTALK_PATTERN)
    PSramcEngine2Init(p, 0, 0x1, TEST_AUDIO_PATTERN, 0);
    u4err_value = PSramcEngine2Run(p, TE_OP_READ_CHECK, TEST_AUDIO_PATTERN);
    PSramcEngine2End(p);
    mcSHOW_ERR_MSG("TEST AUDIO PATTERN result: %d\n", u4err_value);

    PSramcEngine2Init(p, 0, 0x23, TEST_XTALK_PATTERN, 0);
    u4err_value = PSramcEngine2Run(p, TE_OP_WRITE_READ_CHECK, TEST_XTALK_PATTERN);
    PSramcEngine2End(p);
    mcSHOW_ERR_MSG("TEST_XTALK_PATTERN result: %d\n", u4err_value);


    PSramcEngine2Init(p, 0, 0x23, TEST_ISI_PATTERN, 0);
    u4err_value = PSramcEngine2Run(p, TE_OP_READ_CHECK, TEST_ISI_PATTERN);
    PSramcEngine2End(p);
    mcSHOW_ERR_MSG("TEST_ISI_PATTERN result: %d\n", u4err_value);

    return u4err_value;
}
#endif /* #if 0 */
#if 0
void PSramc_TA2_Test_Run_Time_HW_Set_Column_Num(DRAMC_CTX_T *p)
{
    U8 u1ChannelIdx = 0, shu_index;
    U32 u4matypeR0 = 0, u4matypeR1 = 0;
    U32 u4matype = 0;
    DRAM_CHANNEL_T eOriChannel = p->channel;

    for (u1ChannelIdx = 0; u1ChannelIdx < p->support_channel_num; u1ChannelIdx++) {
        vSetPHY2ChannelMapping(p, u1ChannelIdx);
        u4matype = u4IO32Read4B(EMI_APB_BASE);
        u4matypeR0 = ((u4matype >> (4 + u1ChannelIdx * 16)) & 0x3) + 1; //refer to init_ta2_single_channel()
        u4matypeR1 = ((u4matype >> (6 + u1ChannelIdx * 16)) & 0x3) + 1; //refer to init_ta2_single_channel()

        if (p->support_rank_num == RANK_SINGLE) {
            u4matype = u4matypeR0;
        } else { //dual rank
            u4matype = (u4matypeR0 > u4matypeR1) ? u4matypeR1 : u4matypeR0; //get min value
        }

        for (shu_index = DRAM_DFS_SHUFFLE_1; shu_index < DRAM_DFS_SHUFFLE_MAX; shu_index++)
            vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_SHU_CONF0) + SHU_GRP_DRAMC_OFFSET * shu_index, u4matype, SHU_CONF0_MATYPE);
    }
    vSetPHY2ChannelMapping(p, eOriChannel);

    return;
}
#endif /* #if 0 */
#define TA2_RANK0_ADDRESS   (0xa0000000)
void PSramc_TA2_Test_Run_Time_HW_Presetting(DRAMC_CTX_T *p, U32 len)
{
    U32 u4BaseR0, u4Offset;
    //U32 u4Addr;

    u4BaseR0 = TA2_RANK0_ADDRESS & 0x1fffffff;

    //u4BaseR0 = u4Addr >> 4;
    u4Offset = (len >> 4);//16B per pattern //len should be >>2 or test engine will time out
    u4Offset = (u4Offset == 0) ? 1 : u4Offset;  //halt if u4Offset = 0
    mcSHOW_DBG_MSG("=== TA2 HW\n");

    vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_RK_TEST2_A1), u4BaseR0, RK_TEST2_A1_TEST2_BASE);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A2), u4Offset, TEST2_A2_TEST2_OFF);

    //TA2_Test_Run_Time_HW_Set_Column_Num(p);

    return;
}

#define TA2_PAT TEST_XTALK_PATTERN
void PSramc_TA2_Test_Run_Time_Pat_Setting(DRAMC_CTX_T *p, U8 PatSwitch)
{
    static U8 u1Pat = TA2_PAT, u1loop = 1;
    U8 u1ChannelIdx = 0;
    DRAM_CHANNEL_T eOriChannel = p->channel;


    if (u1loop || (PatSwitch == TA2_PAT_SWITCH_ON)) {
        mcSHOW_DBG_MSG("TA2 PAT: %s\n",
                       (u1Pat == TEST_XTALK_PATTERN) ? "XTALK" : (u1Pat == TEST_AUDIO_PATTERN) ? "AUDIO" : "WSI");
        for (u1ChannelIdx = CHANNEL_A; u1ChannelIdx < p->support_channel_num; u1ChannelIdx++) {
            p->channel = u1ChannelIdx;
            PSramcEngine2SetPat(p, u1Pat, p->support_rank_num - 1, 0);
        }
        p->channel = eOriChannel;
        if (PatSwitch)
            u1Pat = (u1Pat + 1) % 3;
        else
            u1loop = 0;
    }
    return;
}

void PSramc_TA2_Test_Run_Time_HW_Write(DRAMC_CTX_T *p, U8 u1Enable)
{
    DRAM_CHANNEL_T eOriChannel = p->channel;
    U8 u1ChannelIdx;

    for (u1ChannelIdx = 0; u1ChannelIdx < p->support_channel_num; u1ChannelIdx++) {
        p->channel = u1ChannelIdx;
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
                           P_Fld(u1Enable, TEST2_A3_TEST2W) |
                           P_Fld(0, TEST2_A3_TEST2R));
    }
    p->channel = eOriChannel;
    return;
}

void PSramc_TA2_Test_Run_Time_HW_Status(DRAMC_CTX_T *p)
{
    U8 u1ChannelIdx = 0;
    U8 u1RankIdx = 0;
    //U32 u4loop_count = 0;
    //U32 u4ErrRegField = 0;
    U32 u4ErrorValue = 0;
    static U32 err_count = 0;
    static U32 pass_count = 0;
    DRAM_CHANNEL_T eOriChannel = p->channel;

    for (u1ChannelIdx = 0; u1ChannelIdx < p->support_channel_num; u1ChannelIdx++) {
        p->channel = u1ChannelIdx;
        u4ErrorValue = PSramcEngine2Compare(p, TE_OP_WRITE_READ_CHECK);
        if (u4ErrorValue) { //RK0
            mcSHOW_DBG_MSG("=== HW channel(%d) u4ErrorValue: 0x%x, bit error: 0x%x\n",
                           u1ChannelIdx, u4ErrorValue, u4IO32Read4B(DRAMC_REG_ADDR(PSRAMC_REG_CMP_ERR)));
#if defined(SLT)
            while (1);
#endif /* #if defined(SLT) */
        }
        for (u1RankIdx = 0 ; u1RankIdx < p->support_rank_num; u1RankIdx++) {
            if (u4ErrorValue & (1 << u1RankIdx)) {
                err_count++;
                mcSHOW_DBG_MSG("HW channel(%d) Rank(%d), TA2 failed, pass_cnt:%d, err_cnt:%d\n",
                               u1ChannelIdx, u1RankIdx, pass_count, err_count);
                mcSHOW_TIME_MSG("HW channel(%d) Rank(%d), TA2 failed, pass_cnt:%d, err_cnt:%d\n",
                                u1ChannelIdx, u1RankIdx, pass_count, err_count);
            } else {
                pass_count++;
                mcSHOW_DBG_MSG("HW channel(%d) Rank(%d), TA2 pass, pass_cnt:%d, err_cnt:%d\n",
                               u1ChannelIdx, u1RankIdx, pass_count, err_count);
                mcSHOW_TIME_MSG("HW channel(%d) Rank(%d), TA2 pass, pass_cnt:%d, err_cnt:%d\n",
                                u1ChannelIdx, u1RankIdx, pass_count, err_count);
            }
        }
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
                           P_Fld(0, TEST2_A3_TEST2W) |
                           P_Fld(0, TEST2_A3_TEST2R) |
                           P_Fld(0, TEST2_A3_TEST1));
    }
    p->channel = eOriChannel;

    return;
}

void PSramc_TA2_Test_Run_Time_HW(DRAMC_CTX_T *p)
{
    DRAM_CHANNEL_T channel_bak = p->channel;
    DRAM_RANK_T rank_bak = p->rank;

    PSramc_TA2_Test_Run_Time_HW_Presetting(p, 0x200);  //TEST2_2_TEST2_OFF = 0x400
    PSramc_TA2_Test_Run_Time_Pat_Setting(p, TA2_PAT_SWITCH_OFF);
    PSramc_TA2_Test_Run_Time_HW_Write(p, ENABLE);
    //mcDELAY_MS(1);
    PSramc_TA2_Test_Run_Time_HW_Status(p);

    p->channel = channel_bak;
    p->rank = rank_bak;
    return;
}

void PsramcModeRegWrite(DRAMC_CTX_T *p, U8 u1MRIdx, U8 u1Value)
{
    U32 counter = 0;

    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_CTRL0, u1MRIdx, SWCMD_CTRL0_MRSMA);
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_CTRL0, u1Value, SWCMD_CTRL0_MRWOP);

    // MRW command will be fired when MRWEN 0->1
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 1, SWCMD_EN_MRWEN);

    // wait MRW command fired.
    while (u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SPCMDRESP), PSRAM_SPCMDRESP_MRW_RESPONSE) == 0) {
        counter++;
        mcSHOW_DBG_MSG2("wait MRW command MR%d =0x%x fired (%d)\n",  u1MRIdx, u1Value, counter);
        mcDELAY_US(1);
    }

    // Set MRWEN =0 for next time MRW.
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0, SWCMD_EN_MRWEN);

    if (1) { // this should control by switch
        mcSHOW_DBG_MSG2("Write MR%d =0x%x\n",  u1MRIdx, u1Value);
        mcFPRINTF(fp_A60501, "Write MR%d =0x%x\n", u1MRIdx, u1Value);
    }
}

void PsramcModeRegRead(DRAMC_CTX_T *p, U8 u1MRIdx, U8 *u1Value)
{
    U32 counter = 0;
    U8 u1MRvalue;

    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_CTRL0, u1MRIdx, SWCMD_CTRL0_MRSMA);

    // MRR command will be fired when MRREN 0->1
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 1, SWCMD_EN_MRREN);

    // wait MRW command fired.
    while (u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SPCMDRESP), PSRAM_SPCMDRESP_MRR_RESPONSE) == 0) {
        counter++;
        mcSHOW_DBG_MSG2("wait MRR command  MR%d fired (%d)\n",  u1MRIdx, counter);
        mcDELAY_US(1);
    }
    u1MRvalue = u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_MRR_STATUS), PSRAM_MRR_STATUS_MRR_REG);
    *u1Value = u1MRvalue;

    // Set MRREN =0 for next time MRR.
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0, SWCMD_EN_MRREN);

    mcSHOW_DBG_MSG2("Read MR%d = 0x%x\n", u1MRIdx, u1MRvalue);
}

static void PsramReleaseDMSUS(DRAMC_CTX_T *p)
{
    vIO32WriteFldMulti(DDRPHY_MISC_CTRL1_PSRAM, P_Fld(0x1, MISC_CTRL1_PSRAM_R_DMDQSIENCG_EN_PSRAM)
                       | P_Fld(0x1, MISC_CTRL1_PSRAM_R_DM_TX_ARCMD_OE_PSRAM)
                       | P_Fld(0x1, MISC_CTRL1_PSRAM_R_DM_TX_ARCLK_OE_PSRAM));

    vIO32WriteFldMulti(DDRPHY_MISC_SPM_CTRL1, P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10)
                       | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_B0)
                       | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_B1)
                       | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_CA));
}

#if 0
static void PsramDataWindowTest(DRAMC_CTX_T *p)
{
    U32 u4Mrr = 0;
    U16 u2Tmp;
    U8 i, j, u1FirstUIPass = 0, u1LastUIPass = 0, u1CAStart = 16;
    U8 u1FirstPIPass = 0, u1LastPIpass = 0;
    U8 passcnt = 0;

    PsramcModeRegRead(p, 0x1);    //read predefine data for DQ calibration
    u4Mrr = u4IO32ReadFldAlign(PSRAMC_REG_MRR_STATUS, PSRAM_MRR_STATUS_MRR_REG);
    u4Mrr = (u4Mrr & 0xff);

    mcSHOW_DBG_MSG2("MRR2 read data: 0x%x\n", u4Mrr);

    for (i = u1CAStart; i < u1CAStart + 5; i = i + 1) {
        PSramTXSetDelayReg_CA(p, TRUE, i, &u2Tmp); // CA setting (2, 1), CA OEN (1, 6)

        for (j = 0; j < 32; j = j + 4) {

            mcSHOW_DBG_MSG2("scan range(%d, %d)\n", i, j);
            vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), j, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0);

            PsramcModeRegRead(p, 0x20);   //read predefine data for DQ calibration
            u4Mrr = u4IO32ReadFldAlign(PSRAMC_REG_MRR_STATUS, PSRAM_MRR_STATUS_MRR_REG);
            u4Mrr = (u4Mrr & 0xff);
            mcSHOW_DBG_MSG2("MRR32 read data: 0x%x\n", u4Mrr);

            u2Tmp = (u4Mrr << 8);

            PsramcModeRegRead(p, 0x28);   //read predefine data for DQ calibration
            u4Mrr = u4IO32ReadFldAlign(PSRAMC_REG_MRR_STATUS, PSRAM_MRR_STATUS_MRR_REG);
            u4Mrr = (u4Mrr & 0xff);
            mcSHOW_DBG_MSG2("MRR40 read data: 0x%x\n", u4Mrr);

            u2Tmp = (u2Tmp + u4Mrr);

            if ((u2Tmp == 0xff00) && (u1FirstUIPass == 0)) {
                passcnt ++;
                if (passcnt == 1) {
                    u1FirstUIPass = i;
                    u1FirstPIPass = j;
                }
                mcSHOW_DBG_MSG2("CA pass window (%d, %d)\n", i, j);
            } else if ((passcnt > 10) && (u2Tmp == 0xff00)) {
                u1LastUIPass = i;
                u1LastPIpass = j;
                mcSHOW_DBG_MSG2("CA pass window found! (%d ~ %d)\n", u1FirstUIPass, u1LastUIPass);
                i = u1CAStart + 5; //break;
                break;
            } else if ((passcnt > 10) && (u2Tmp != 0xff00)) {
                passcnt = 0;
                mcSHOW_DBG_MSG2("CA fail window %d\n", i);
            }

        }
    }

    u1FirstUIPass = (u1FirstUIPass + u1LastUIPass) >> 1;
    u1FirstPIPass = (((u1LastUIPass - u1FirstUIPass) > 2 ? ((u1LastUIPass - u1FirstUIPass - 1) << 5) : ((u1LastUIPass - u1FirstUIPass) << 5)) + u1LastPIpass - u1FirstPIPass) >> 1;
    u1FirstPIPass = u1FirstPIPass + u1FirstPIPass;
    u1FirstPIPass = (u1FirstPIPass > 32 ? (u1FirstPIPass - 32) : u1FirstPIPass);

    mcSHOW_DBG_MSG2("final CA center(%d %d)\n", u1FirstUIPass, u1FirstPIPass);
    PSramTXSetDelayReg_CA(p, TRUE, u1FirstUIPass, &u2Tmp); // CA setting (2, 1), CA OEN (1, 6)
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), u1FirstPIPass, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0);
}
#endif /* #if 0 */


static void PSRAMCModeRegInit(DRAMC_CTX_T *p)
{
#if SUPPORT_PSRAM_256M
    if (p->frequency == 800) {
        PsramcModeRegWrite(p, 0, 0x11);
    } else { //2133
        PsramcModeRegWrite(p, 0, 0x13);
    }
#else /* #if SUPPORT_PSRAM_256M */
    if (p->frequency == 800) {
        PsramcModeRegWrite(p, 0, 0x11);

        PsramcModeRegWrite(p, 2, 0x20); //enable Temperature mode
    } else { //2133
        PsramcModeRegWrite(p, 0, 0x13);

        PsramcModeRegWrite(p, 2, 0x20); //enable Temperature mode
    }
#endif /* #if SUPPORT_PSRAM_256M */
    //PsramDataWindowTest(p);
}



void vPSRAM_AutoRefreshSwitch(DRAMC_CTX_T *p, U8 option)
{
    if (option == ENABLE) {
        //enable autorefresh
        vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_REFCTRL0), 0, REFCTRL0_REFDIS);     //REFDIS=0, enable auto refresh
    } else {
        //disable autorefresh
        vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_REFCTRL0), 1, REFCTRL0_REFDIS);     //REFDIS=1, disable auto refresh

        //because HW will actually disable autorefresh after refresh_queue empty, so we need to wait quene empty.
        mcDELAY_US(u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_MISC_STATUSA), MISC_STATUSA_REFRESH_QUEUE_CNT) * 4); //wait refresh_queue_cnt * 3.9us
    }

}

static DRAM_STATUS_T PSRAMCPowerOn(DRAMC_CTX_T *p)
{
    U32 u4Response;
    U32 u4TimeCnt = TIME_OUT_CNT;
    U32 u4Pwron_retry = 10;

    mcSHOW_DBG_MSG("[PSRAM PowerOn Start!]\n");

    vIO32WriteFldAlign(PSRAMC_REG_CKECTRL, 0x1, CKECTRL_CKEFIXON);
    //vIO32WriteFldAlign(PSRAMC_REG_DRAMC_PD_CTRL, 0x0, PSAMC_PD_CTRL_APHYCKCG_FIXOFF);

    mcDELAY_US(3); //tPU and Toggle CK>100 cycles

    do {
        u4TimeCnt = TIME_OUT_CNT;
        //global reset
        vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0x1, SWCMD_EN_RESETEN);
        do {
            u4Response = u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SPCMDRESP), PSRAM_SPCMDRESP_RESET_RESPONSE);
            u4TimeCnt --;
            mcDELAY_US(1);  // tRST

            mcSHOW_DBG_MSG3("%d- ", u4TimeCnt);
            mcFPRINTF(fp_A60501, "%d- ", u4TimeCnt);
        } while ((u4Response == 0) && (u4TimeCnt > 0));

        if (u4TimeCnt == 0) {
            mcSHOW_DBG_MSG("SWRST CMD FAIL \n");
            vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0x0, SWCMD_EN_RESETEN);
            mcDELAY_US(150);
            u4Pwron_retry --;
        } else {
            u4Pwron_retry = 0;
        }
    } while ((u4Response == 0) && (u4Pwron_retry > 0));

    if (u4Response == 0) {
        mcSHOW_DBG_MSG("SWRST CMD FAIL ,call SDT reset\n");
        __disable_irq();
        MTCMOS_PWR_DOWN_AUDIO_AFE;
        MTCMOS_PWR_DOWN_DSP;
        MTCMOS_PWR_DOWN_AUDIO_AO;

        hal_cache_disable();
        hal_cache_deinit();
        hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
    }
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0x0, SWCMD_EN_RESETEN);

    mcDELAY_US(10); // tRST

    //release TCKFIXON
    vIO32WriteFldAlign(PSRAMC_REG_CKECTRL, 0x0, CKECTRL_CKEFIXON);

    //u1PowerOn=1;
    mcSHOW_DBG_MSG("APPLY_PSRAM_POWER_INIT_SEQUENCE Done!\n");

    u4TimeCnt = TIME_OUT_CNT;


    mcSHOW_DBG_MSG("[PSRAM ZQCalibration Start!]\n");
    mcDELAY_US(1);

    vIO32WriteFldAlign(PSRAMC_REG_ZQC_CTRL1, 0x05, ZQC_CTRL1_ZQCSAD);
    vIO32WriteFldAlign(PSRAMC_REG_ZQC_CTRL1, 0x0, ZQC_CTRL1_ZQCSOP);

    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0x1, SWCMD_EN_ZQCEN);
    do {
        u4Response = u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SPCMDRESP), PSRAM_SPCMDRESP_ZQC_RESPONSE);
        u4TimeCnt --;
        mcDELAY_US(1);  // Wait tZQCAL(min) 1us or wait next polling

        mcSHOW_DBG_MSG3("%d- ", u4TimeCnt);
        mcFPRINTF(fp_A60501, "%d- ", u4TimeCnt);
    } while ((u4Response == 0) && (u4TimeCnt > 0));

    if (u4TimeCnt == 0) { //time out
        vSetCalibrationResult(p, DRAM_CALIBRATION_ZQ, DRAM_FAIL);
        mcSHOW_DBG_MSG("PSRAM ZQCAL Start fail (time out)\n");
        mcFPRINTF(fp_A60501, "PSRAM ZQCAL Start fail (time out)\n");
        return DRAM_FAIL;
    } else {
        vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0x0, SWCMD_EN_ZQCEN);
    }

    mcSHOW_DBG_MSG("[PSRAM ZQCalibration Done!]\n");

    return DRAM_OK;
}



DRAM_STATUS_T PsramUpdateACTimingReg(DRAMC_CTX_T *p, const ACTimePsram_T *ACTbl)
{
    ACTimePsram_T ACTblFinal;

    if (ACTbl == NULL)
        return DRAM_FAIL;

    ACTblFinal = *ACTbl;

    vIO32WriteFldMulti(PSRAMC_REG_CE_CNT0, P_Fld(ACTblFinal.tce_wr, CE_CNT0_TCE_WR)
                       | P_Fld(ACTblFinal.tce_mrw, CE_CNT0_TCE_MRW)
                       | P_Fld(ACTblFinal.tce_rd, CE_CNT0_TCE_RD)
                       | P_Fld(ACTblFinal.tce_hsleepx, CE_CNT0_TCE_HSLEEPX));

    vIO32WriteFldMulti(PSRAMC_REG_CE_CNT_05T, P_Fld(ACTblFinal.tce_wr_05T, CE_CNT_05T_TCE_WR_05T)
                       | P_Fld(ACTblFinal.tce_rd_05T, CE_CNT_05T_TCE_RD_05T)
                       | P_Fld(ACTblFinal.tce_hsleepx_05T, CE_CNT_05T_TCE_HSLEEPX_05T)
                       | P_Fld(ACTblFinal.tce_mrw_05T, CE_CNT_05T_TCE_MRW_05T));

    vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM0, P_Fld(ACTblFinal.tcem, SHU_ACTIM0_TCEM)
                       | P_Fld(ACTblFinal.txhs, SHU_ACTIM0_TXHS)
                       | P_Fld(ACTblFinal.trc, SHU_ACTIM0_TRC)
                       | P_Fld(ACTblFinal.trfc, SHU_ACTIM0_TRFC));
    vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM1, P_Fld(ACTblFinal.tcphw, SHU_ACTIM1_TCPHW)
                       | P_Fld(ACTblFinal.tcphr_l, SHU_ACTIM1_TCPHR_L)
                       | P_Fld(ACTblFinal.tcphr_s, SHU_ACTIM1_TCPHR_S));

    vIO32WriteFldMulti(PSRAMC_REG_SHU_AC_TIME_05T, P_Fld(ACTblFinal.trfc_05T, PSRAMC_SHU_AC_TIME_05T_TRFC_05T)
                       | P_Fld(ACTblFinal.trc_05T, PSRAMC_SHU_AC_TIME_05T_TRC_05T)
                       | P_Fld(ACTblFinal.txhs_05T, PSRAMC_SHU_AC_TIME_05T_TXHS_05T)
                       | P_Fld(ACTblFinal.tcem_05T, PSRAMC_SHU_AC_TIME_05T_TCEM_05T)
                       | P_Fld(ACTblFinal.tcphr_s_05T, PSRAMC_SHU_AC_TIME_05T_TCPHR_S_05T)
                       | P_Fld(ACTblFinal.tcphr_l_05T, PSRAMC_SHU_AC_TIME_05T_TCPHR_L_05T)
                       | P_Fld(ACTblFinal.tcphw_05T, PSRAMC_SHU_AC_TIME_05T_TCPHW_05T)
                       | P_Fld(ACTblFinal.tmrw_05T, PSRAMC_SHU_AC_TIME_05T_TMRW_05T));
    vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM2, P_Fld(ACTblFinal.tmrw, SHU_ACTIM2_TMRW));
    vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM3, P_Fld(ACTblFinal.tzqcl, SHU_ACTIM3_TZQCL)
                       | P_Fld(ACTblFinal.tzqcs, SHU_ACTIM3_TZQCS));
    vIO32WriteFldMulti(PSRAMC_REG_SHU_AC_TIME_05T_B, P_Fld(ACTblFinal.tzqcl_05T, SHU_AC_TIME_05T_B_TZQCL_05T)
                       | P_Fld(ACTblFinal.tzqcs_05T, SHU_AC_TIME_05T_B_TZQCS_05T));

    vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM4, P_Fld(ACTblFinal.txrefcnt, SHU_ACTIM4_TXREFCNT));

    vIO32WriteFldMulti(PSRAMC_REG_REFCNT_FR_CLK1, P_Fld(ACTblFinal.trefcnt_fr_clk1, REFCNT_FR_CLK1_REFCNT_FR_CLK_1X)
                       | P_Fld(ACTblFinal.trefcnt_fr_clk2, REFCNT_FR_CLK1_REFCNT_FR_CLK_2X));

    vIO32WriteFldMulti(PSRAMC_REG_ZQC_CTRL1, P_Fld(0x5, ZQC_CTRL1_ZQCSAD)
                       | P_Fld(0x0, ZQC_CTRL1_ZQCSOP));

    return DRAM_OK;
}

void EnablePsramcPhyDCM(DRAMC_CTX_T *p, BOOL bEn)//Should refer to "vSetChannelNumber"
{
    //INT8 i1ShuIdx = 0;
    //INT8 i1ShuffleMax = DRAM_DFS_SHUFFLE_1;

    // DramcBroadcastOnOff(DRAMC_BROADCAST_OFF);//Just for bring up
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(1, "//EnablePsramcPhyDCM start\n");
    mcSHOW_DBG_MSG("bEn = %d\n", bEn);
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

    //PIC:Lynx, different from sylvia, related to PHY 0x298[22][20]
#if 1
    vIO32WriteFldAlign_All(DDRPHY_MISC_CTRL0_PSRAM, 0x1, MISC_CTRL0_PSRAM_R_DMSHU_PHYDCM_FORCEOFF_PSRAM);
#endif /* #if 1 */

    //APHY PICG DQ & DQM PIC:JOE
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI1, 0x0, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0_REG_OPT);
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI1, 0x1, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0);
    vIO32WriteFldAlign_All(DDRPHY_SHU1_B0_DLL0, 0x1, SHU1_B0_DLL0_RG_ARPISM_MCK_SEL_B0_SHU);//PI_CLOCK_CG_IMPROVEMENT PIC:JOE
    if (bEn) { //DCM on
#if 1//PSRAM_SPEC
        vIO32WriteFldMulti(DDRPHY_MISC_CG_CTRL0, P_Fld(0x0, MISC_CG_CTRL0_RG_CG_NAO_FORCE_OFF_PSRAMC)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_PSRAMC_CHA_CK_OFF)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_NAO_FORCE_OFF)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_DRAMC_CHB_CK_OFF)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_INFRA_OFF_DISABLE)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_RX_COMB1_OFF_DISABLE)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_RX_COMB0_OFF_DISABLE)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_RX_CMD_OFF_DISABLE)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_COMB1_OFF_DISABLE)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_COMB0_OFF_DISABLE)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_CMD_OFF_DISABLE)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_COMB_OFF_DISABLE)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_PHY_OFF_DIABLE)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_DRAMC_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_EMI_OFF_DISABLE));

        vIO32WriteFldMulti(PSRAMC_REG_DRAMC_PD_CTRL, P_Fld(0x1, PSAMC_PD_CTRL_COMBCLKCTRL)
                           | P_Fld(0x1, PSAMC_PD_CTRL_PHYCLKDYNGEN)
                           | P_Fld(0x0, PSAMC_PD_CTRL_MIOCKCTRLOFF)
                           | P_Fld(0x0, PSAMC_PD_CTRL_COMBPHY_CLKENSAME)
                           | P_Fld(0x0, PSAMC_PD_CTRL_PHYGLUECLKRUN)
                           | P_Fld(0x1, PSAMC_PD_CTRL_DCMENNOTRFC)
                           | P_Fld(0x1, PSRAMC_PD_CTRL_DCMEN));

        vIO32WriteFldMulti(PSRAMC_REG_TX_CG_SET0, P_Fld(0x0, TX_CG_SET0_SELPH_4LCG_DIS)
                           | P_Fld(0x0, TX_CG_SET0_SELPH_CMD_CG_DIS));

        vIO32WriteFldAlign(PSRAMC_REG_RX_CG_SET0, 0x0, RX_CG_SET0_RDYCKAR);
        vIO32WriteFldAlign(PSRAMC_REG_CLKAR, 0x0, CLKAR_REQQUECLKRUN);
        vIO32WriteFldAlign(PSRAMC_REG_SREF_DPD_CTRL, 0x0, SREF_DPD_CTRL_SREF_CG_OPT);
        vIO32WriteFldAlign(PSRAMC_REG_ACTIMING_CTRL, 0x0, ACTIMING_CTRL_SEQCLKRUN);

        vIO32WriteFldAlign(PSRAMC_REG_CKECTRL, 0x0, CKECTRL_CKEFIXON);
        vIO32WriteFldAlign(PSRAMC_REG_TEST2_A3, 0x0, TEST2_A3_TESTCLKRUN);

#endif /* #if 1//PSRAM_SPEC */

        //mem_dcm
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x806003BE);//divided freq change to 1/4
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x806003BF);//divided freq change to 1/4
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x806003BE);//divided freq change to 1/4
        vIO32WriteFldMulti(DDRPHY_MISC_CG_CTRL2, P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG)
                           | P_Fld(0x17, MISC_CG_CTRL2_RG_MEM_DCM_APB_SEL)
                           | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_FORCE_ON)
                           | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_DCM_EN)
                           | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_DBC_EN)
                           | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_DBC_CNT)
                           | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_FSEL)
                           | P_Fld(0x3, MISC_CG_CTRL2_RG_MEM_DCM_IDLE_FSEL)
                           | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_FORCE_OFF)
                           | P_Fld(0x0, MISC_CG_CTRL2_RG_PHY_CG_OFF_DISABLE)
                           | P_Fld(0x0, MISC_CG_CTRL2_RG_PIPE0_CG_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_CG_OFF_DISABLE));

        vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL2, 0x1, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG);
        vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL2, 0x0, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG);

        //TX pipe/sync cell CG
#if 1//PSRAM_SPEC
        vIO32WriteFldMulti_All(DDRPHY_SHU1_CA_CMD7_PSRAM, P_Fld(0x0, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CS_NEW_PSRAM)
                               | P_Fld(0x0, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CLK_NEW_PSRAM)
                               | P_Fld(0x0, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CMD_NEW_PSRAM));//PI_CLOCK_CG_IMPROVEMENT PIC:JOE

        vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQM_NEW_B0_PSRAM)
                               | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQS_NEW_B0_PSRAM)
                               | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQ_NEW_B0_PSRAM));//PI_CLOCK_CG_IMPROVEMENT PIC:JOE
        vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ8_PSRAM, P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRANK_CHG_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRANK_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_SYNC_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMSTBEN_SYNC_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRXDLY_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_RMRODTEN_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRANK_RXDLY_PIPE_CG_IG_B0_PSRAM));
#endif /* #if 1//PSRAM_SPEC */


        vIO32WriteFldMulti(DDRPHY_PSRAM_APHY_PICG_CTRL0, P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_DQSIENCG_EN)
                           | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQSIEN)
                           | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQ)
                           | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQS)
                           | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQM)
                           | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_CG_MCK)
                           | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_MPDIV_CG)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_ARPI_MPDIV_CG_DQ_OPT));
        vIO32WriteFldMulti(DDRPHY_PSRAM_APHY_PICG_CTRL1, P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_CLKIENCG_EN)
                           | P_Fld(0x1, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CLKIEN)
                           | P_Fld(0x1, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CMD)
                           | P_Fld(0x1, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CLK)
                           | P_Fld(0x1, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CS)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_MPDIV_CG_CA_OPT)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_MCTL_CA_OPT)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_MCK_CA_OPT)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_CMD_OPT)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_CLK_OPT)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_TX_ARPI_CG_CMD_NEW)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_TX_ARPI_CG_CS_NEW));
        vIO32WriteFldMulti_All(DDRPHY_MISC_CG_CTRL5, P_Fld(0x1, MISC_CG_CTRL5_R_CA_DLY_DCM_EN)
                               | P_Fld(0x1, MISC_CG_CTRL5_R_DQ0_DLY_DCM_EN)
                               | P_Fld(0x1, MISC_CG_CTRL5_R_DQ1_DLY_DCM_EN)
                               | P_Fld(0x1, MISC_CG_CTRL5_R_CA_PI_DCM_EN)
                               | P_Fld(0x1, MISC_CG_CTRL5_R_DQ0_PI_DCM_EN)
                               | P_Fld(0x1, MISC_CG_CTRL5_R_DQ1_PI_DCM_EN));
    } else { //DCM off
        vIO32WriteFldMulti(DDRPHY_MISC_CG_CTRL0, P_Fld(0x0, MISC_CG_CTRL0_RG_CG_NAO_FORCE_OFF_PSRAMC)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_PSRAMC_CHA_CK_OFF)
                           | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_NAO_FORCE_OFF)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_DRAMC_CHB_CK_OFF)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_INFRA_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_RX_COMB1_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_RX_COMB0_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_RX_CMD_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_COMB1_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_COMB0_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_CMD_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_COMB_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_PHY_OFF_DIABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_DRAMC_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_EMI_OFF_DISABLE));
        vIO32WriteFldMulti(PSRAMC_REG_DRAMC_PD_CTRL, P_Fld(0x0, PSAMC_PD_CTRL_COMBCLKCTRL)
                           | P_Fld(0x0, PSAMC_PD_CTRL_PHYCLKDYNGEN)
                           | P_Fld(0x1, PSAMC_PD_CTRL_MIOCKCTRLOFF)
                           | P_Fld(0x1, PSAMC_PD_CTRL_COMBPHY_CLKENSAME)
                           | P_Fld(0x1, PSAMC_PD_CTRL_PHYGLUECLKRUN)
                           | P_Fld(0x0, PSAMC_PD_CTRL_DCMENNOTRFC)
                           | P_Fld(0x0, PSRAMC_PD_CTRL_DCMEN));

        vIO32WriteFldMulti(PSRAMC_REG_TX_CG_SET0, P_Fld(0x1, TX_CG_SET0_SELPH_4LCG_DIS)
                           | P_Fld(0x1, TX_CG_SET0_SELPH_CMD_CG_DIS));

        vIO32WriteFldAlign(PSRAMC_REG_RX_CG_SET0, 0x1, RX_CG_SET0_RDYCKAR);
        vIO32WriteFldAlign(PSRAMC_REG_CLKAR, 0x1, CLKAR_REQQUECLKRUN);
        vIO32WriteFldAlign(PSRAMC_REG_SREF_DPD_CTRL, 0x1, SREF_DPD_CTRL_SREF_CG_OPT);
        vIO32WriteFldAlign(PSRAMC_REG_ACTIMING_CTRL, 0x1, ACTIMING_CTRL_SEQCLKRUN);

        vIO32WriteFldAlign(PSRAMC_REG_CKECTRL, 0x1, CKECTRL_CKEFIXON);
        vIO32WriteFldAlign(PSRAMC_REG_TEST2_A3, 0x1, TEST2_A3_TESTCLKRUN);

        //mem_dcm
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x8060037E);//divided freq change to 1/4
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x8060037F);//divided freq change to 1/4
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x8060037E);//divided freq change to 1/4
        vIO32WriteFldMulti(DDRPHY_MISC_CG_CTRL2, P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG)
                           | P_Fld(0x17, MISC_CG_CTRL2_RG_MEM_DCM_APB_SEL)
                           | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_FORCE_ON)
                           | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_DCM_EN)
                           | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_DBC_EN)
                           | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_DBC_CNT)
                           | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_FSEL)
                           | P_Fld(0x3, MISC_CG_CTRL2_RG_MEM_DCM_IDLE_FSEL)
                           | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_FORCE_OFF)
                           | P_Fld(0x0, MISC_CG_CTRL2_RG_PHY_CG_OFF_DISABLE)
                           | P_Fld(0x0, MISC_CG_CTRL2_RG_PIPE0_CG_OFF_DISABLE)
                           | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_CG_OFF_DISABLE));
        vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL2, 0x1, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG);
        vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL2, 0x0, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG);
        //TX pipe/sync cell CG
#if 1//PSRAM_SPEC
        vIO32WriteFldMulti_All(DDRPHY_SHU1_CA_CMD7_PSRAM, P_Fld(0x0, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CS_NEW_PSRAM)
                               | P_Fld(0x0, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CLK_NEW_PSRAM)
                               | P_Fld(0x0, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CMD_NEW_PSRAM));//PI_CLOCK_CG_IMPROVEMENT PIC:JOE

        vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQM_NEW_B0_PSRAM)
                               | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQS_NEW_B0_PSRAM)
                               | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQ_NEW_B0_PSRAM));//PI_CLOCK_CG_IMPROVEMENT PIC:JOE

        vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ8_PSRAM, P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRANK_CHG_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRANK_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_SYNC_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMSTBEN_SYNC_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDLY_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_PIPE_CG_IG_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_RMRODTEN_CG_IG_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRANK_RXDLY_PIPE_CG_IG_B0_PSRAM));
#endif /* #if 1//PSRAM_SPEC */
        vIO32WriteFldMulti(DDRPHY_PSRAM_APHY_PICG_CTRL0, P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_DQSIENCG_EN)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQSIEN)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQ)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQS)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQM)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_CG_MCK)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_MPDIV_CG)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_ARPI_MPDIV_CG_DQ_OPT));
        vIO32WriteFldMulti(DDRPHY_PSRAM_APHY_PICG_CTRL1, P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_CLKIENCG_EN)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CLKIEN)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CMD)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CLK)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CS)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_MPDIV_CG_CA_OPT)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_MCTL_CA_OPT)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_MCK_CA_OPT)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_CMD_OPT)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_CLK_OPT)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_TX_ARPI_CG_CMD_NEW)
                           | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_TX_ARPI_CG_CS_NEW));
        vIO32WriteFldMulti_All(DDRPHY_MISC_CG_CTRL5, P_Fld(0x0, MISC_CG_CTRL5_R_CA_DLY_DCM_EN)
                               | P_Fld(0x0, MISC_CG_CTRL5_R_DQ0_DLY_DCM_EN)
                               | P_Fld(0x0, MISC_CG_CTRL5_R_DQ1_DLY_DCM_EN)
                               | P_Fld(0x0, MISC_CG_CTRL5_R_CA_PI_DCM_EN)
                               | P_Fld(0x0, MISC_CG_CTRL5_R_DQ0_PI_DCM_EN)
                               | P_Fld(0x0, MISC_CG_CTRL5_R_DQ1_PI_DCM_EN));
    }

#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(0, "//EnablePsramcPhyDCM end\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */
}

void PsramcHWGatingInit(DRAMC_CTX_T *p)
{
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(1, "//PsramcHWGatingInit start\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

#ifdef HW_GATING
    vIO32WriteFldMulti(DDRPHY_PSRAM_STBCAL_CTRL0, P_Fld(1, PSRAM_STBCAL_CTRL0_PICHGBLOCK_NORD)
                       | P_Fld(0, PSRAM_STBCAL_CTRL0_REFUICHG)
                       | P_Fld(0, PSRAM_STBCAL_CTRL0_PHYVALID_IG)
                       | P_Fld(0, PSRAM_STBCAL_CTRL0_STBSTATE_OPT)
                       | P_Fld(0, PSRAM_STBCAL_CTRL0_STBDLELAST_FILTER)
                       | P_Fld(1, PSRAM_STBCAL_CTRL0_STB_FLAGCLR_OPT)
                       | P_Fld(1, PSRAM_STBCAL_CTRL0_STB_SHIFT_DTCOUT_IG));
    vIO32WriteFldMulti(DDRPHY_PSRAM_STBCAL_CTRL1, P_Fld(1, PSRAM_STBCAL_CTRL1_STBCAL_FILTER)
                       | P_Fld(0, PSRAM_STBCAL_CTRL1_STBDLELAST_PULSE)
                       | P_Fld(0x9, PSRAM_STBCAL_CTRL1_STB_UPDMASKCYC)
                       | P_Fld(1, PSRAM_STBCAL_CTRL1_STB_UPDMASK_EN)
                       | P_Fld(1, PSRAM_STBCAL_CTRL1_STBCNT_MODESEL)
                       | P_Fld(0, PSRAM_STBCAL_CTRL1_STBDLELAST_OPT)
                       | P_Fld(1, PSRAM_STBCAL_CTRL1_PIMASK_RKCHG_OPT));
    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL0_PSRAM), P_Fld(1, MISC_CTRL0_PSRAM_R_DMDQSIEN_FIFO_EN_PSRAM)
                           | P_Fld(0, MISC_CTRL0_PSRAM_R_DMVALID_DLY_PSRAM)
                           | P_Fld(0, MISC_CTRL0_PSRAM_R_DMVALID_DLY_OPT_PSRAM)
                           | P_Fld(0, MISC_CTRL0_PSRAM_R_DMDQSIEN_SYNCOPT_PSRAM));
    vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B0_DQ6), 0, B0_DQ6_RG_RX_ARDQ_DMRANK_OUTSEL_B0);
    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), P_Fld(1, B0_DQ9_R_DMDQSIEN_RDSEL_LAT_B0)
                           | P_Fld(0, B0_DQ9_R_DMRXDVS_VALID_LAT_B0));
#endif /* #ifdef HW_GATING */
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(0, "//PsramcHWGatingInit end\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

}

void PsramcHWGatingOnOff(DRAMC_CTX_T *p, U8 u1OnOff)
{
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(1, "//PsramcHWGatingOnOff start\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

#ifdef HW_GATING
    //PSRAM UI/PI tracking enable
    vIO32WriteFldMulti(DDRPHY_PSRAM_STBCAL_CTRL0, P_Fld(u1OnOff, PSRAM_STBCAL_CTRL0_STB_UI_TRACK_EN)
                       | P_Fld(u1OnOff, PSRAM_STBCAL_CTRL0_STB_PI_TRACK_EN));
#else /* #ifdef HW_GATING */
    vIO32WriteFldMulti(DDRPHY_PSRAM_STBCAL_CTRL0, P_Fld(0, PSRAM_STBCAL_CTRL0_STB_UI_TRACK_EN)
                       | P_Fld(0, PSRAM_STBCAL_CTRL0_STB_PI_TRACK_EN));
#endif /* #ifdef HW_GATING */
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(0, "//PsramcHWGatingOnOff end\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

}

void TransferToSPMControl(DRAMC_CTX_T *p)
{
#if PSRAM_SPEC
    vIO32WriteFldAlign(DDRPHY_MISC_SPM_CTRL2, 0xffffffef, MISC_SPM_CTRL2_PHY_SPM_CTL2);
    vIO32WriteFldAlign(DDRPHY_MISC_SPM_CTRL0, 0xfbffefff, MISC_SPM_CTRL0_PHY_SPM_CTL0);
    vIO32WriteFldMulti(PSRAMC_REG_SREF_DPD_CTRL, P_Fld(0x1, SREF_DPD_CTRL_SREF_HW_EN)
                       | P_Fld(0x1, SREF_DPD_CTRL_HSLEEP_HW_EN));
#endif /* #if PSRAM_SPEC */
    return;
}

void TransferPLLToSPMControl(DRAMC_CTX_T *p)
{
#if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0)
    U8 shu_level = 0;
    //shu_level = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DRAMC_REG_SHUSTATUS), SHUSTATUS_SHUFFLE_LEVEL);

    /*TINFO="DRAM : enter SW DVFS"*/
    /*TINFO="DRAM : SPM presetting for pinmux"*/
    //! set SPM project code and enable clock enable

    vIO32WriteFldMulti(SPM_POWERON_CONFIG_EN, P_Fld(0xB16, POWERON_CONFIG_EN_PROJECT_CODE)
                       | P_Fld(1, POWERON_CONFIG_EN_BCLK_CG_EN));

    //! set SPM pinmux
    vIO32WriteFldMulti(SPM_PCM_PWR_IO_EN, P_Fld(0, PCM_PWR_IO_EN_RG_PCM_PWR_IO_EN)
                       | P_Fld(0, PCM_PWR_IO_EN_RG_RF_SYNC_EN));

    vIO32WriteFldAlign(SPM_DRAMC_DPY_CLK_SW_CON_SEL, 0xaaffffff, DRAMC_DPY_CLK_SW_CON_SEL_FULL);
    vIO32WriteFldAlign(SPM_DRAMC_DPY_CLK_SW_CON_SEL2, 0xffffffff, DRAMC_DPY_CLK_SW_CON_SEL2_FULL);

    //! set  sc_dpy_2nd_dll_en, sc_dpy_dll_en, sc_dpy_dll_ck_en ,sc_dpy_vref_en , sc_phypll_en = 1
    vIO32WriteFldMulti(SPM_POWER_ON_VAL0, P_Fld(1, SPM_POWER_ON_VAL0_SC_DPY_2ND_DLL_EN_PCM)
                       | P_Fld(1, SPM_POWER_ON_VAL0_SC_DPY_DLL_EN_PCM)
                       | P_Fld(1, SPM_POWER_ON_VAL0_SC_DPY_DLL_CK_EN_PCM)
                       | P_Fld(1, SPM_POWER_ON_VAL0_SC_DPY_VREF_EN_PCM)
                       | P_Fld(1, SPM_POWER_ON_VAL0_SC_PHYPLL_EN_PCM));

    vIO32WriteFldAlign(SPM_S1_MODE_CH, 3, S1_MODE_CH_SPM_S1_MODE_CH);

    mcSHOW_DBG_MSG("TransferPLLToSPMControl - MODE SW ");
    if (shu_level == 1) {
        mcSHOW_DBG_MSG("CLRPLL\n");
        vIO32WriteFldAlign(SPM_POWER_ON_VAL0, 0, SPM_POWER_ON_VAL0_SC_PHYPLL_MODE_SW_PCM);
        vIO32WriteFldAlign(SPM_POWER_ON_VAL0, 1, SPM_POWER_ON_VAL0_SC_PHYPLL2_MODE_SW_PCM);
        vIO32WriteFldAlign(SPM_DRAMC_DPY_CLK_SW_CON2, 0, DRAMC_DPY_CLK_SW_CON2_SW_PHYPLL_MODE_SW);
        vIO32WriteFldAlign(SPM_DRAMC_DPY_CLK_SW_CON2, 1, DRAMC_DPY_CLK_SW_CON2_SW_PHYPLL2_MODE_SW);
    } else {
        mcSHOW_DBG_MSG("PHYPLL\n");
        vIO32WriteFldAlign(SPM_POWER_ON_VAL0, 0, SPM_POWER_ON_VAL0_SC_PHYPLL2_MODE_SW_PCM);
        vIO32WriteFldAlign(SPM_POWER_ON_VAL0, 1, SPM_POWER_ON_VAL0_SC_PHYPLL_MODE_SW_PCM);
        vIO32WriteFldAlign(SPM_DRAMC_DPY_CLK_SW_CON2, 0, DRAMC_DPY_CLK_SW_CON2_SW_PHYPLL2_MODE_SW);
        vIO32WriteFldAlign(SPM_DRAMC_DPY_CLK_SW_CON2, 1, DRAMC_DPY_CLK_SW_CON2_SW_PHYPLL_MODE_SW);
    }
    mcDELAY_US(1);
    vIO32WriteFldAlign_All(DDRPHY_PLL1, 0x0, PLL1_RG_RPHYPLL_EN);
    vIO32WriteFldAlign_All(DDRPHY_PLL2, 0x0, PLL2_RG_RCLRPLL_EN);
#endif /* #if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0) */
    return;
}


void DramcRunTimeConfig(DRAMC_CTX_T *p)
{
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(1, "//DramcRunTimeConfig start\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

#if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0)
#if 0//def SPM_CONTROL_AFTERK
    //due to 7933 haven't spm core, no need change pll control to spm
    TransferPLLToSPMControl(p);
#endif /* #if 0//def SPM_CONTROL_AFTERK */
#endif /* #if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0) */

#ifdef HW_GATING
    PsramcHWGatingInit(p);     // HW gating initial before RunTime config.
    PsramcHWGatingOnOff(p, 1); // Enable HW gating tracking
    mcSHOW_DBG_MSG("HW_GATING: ON\n");
#else /* #ifdef HW_GATING */
    mcSHOW_DBG_MSG("HW_GATING: OFF\n");
#endif /* #ifdef HW_GATING */

#ifdef DUMMY_READ_FOR_TRACKING
    PsramcDummyReadForTrackingEnable(p);
    mcSHOW_DBG_MSG("DUMMY_READ_FOR_TRACKING: ON\n");
#else /* #ifdef DUMMY_READ_FOR_TRACKING */
    mcSHOW_DBG_MSG("DUMMY_READ_FOR_TRACKING: OFF\n");
#endif /* #ifdef DUMMY_READ_FOR_TRACKING */

#if PSRAM_SPEC
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(1, "//HW ZQ Calibration start\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

    vIO32WriteFldAlign(PSRAMC_REG_ZQC_CTRL0, 1, ZQC_CTRL0_ZQCSDISB);
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(0, "//HW ZQ Calibration end\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

    mcSHOW_DBG_MSG("PSRAM HW ZQ Calibration: ON\n");
#endif /* #if PSRAM_SPEC */
#if APPLY_LOWPOWER_GOLDEN_SETTINGS
#if (SW_CHANGE_FOR_SIMULATION == 0)
    EnablePsramcPhyDCM(p, 1);
    mcSHOW_DBG_MSG("LOWPOWER_GOLDEN_SETTINGS(DCM): ON\n");
#endif /* #if (SW_CHANGE_FOR_SIMULATION == 0) */
#else /* #if APPLY_LOWPOWER_GOLDEN_SETTINGS */
#if (SW_CHANGE_FOR_SIMULATION == 0)
    EnablePsramcPhyDCM(p, 0);
    mcSHOW_DBG_MSG("LOWPOWER_GOLDEN_SETTINGS(DCM): OFF\n");
#endif /* #if (SW_CHANGE_FOR_SIMULATION == 0) */
#endif /* #if APPLY_LOWPOWER_GOLDEN_SETTINGS */

#ifdef SPM_CONTROL_AFTERK
    TransferToSPMControl(p);  //don't enable in ETT
    mcSHOW_DBG_MSG("SPM_CONTROL_AFTERK: ON\n");
#else /* #ifdef SPM_CONTROL_AFTERK */
    mcSHOW_DBG_MSG("SPM_CONTROL_AFTERK: OFF\n");
#endif /* #ifdef SPM_CONTROL_AFTERK */

#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(1, "//TEMP_SENSOR_ENABLE start\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

#ifdef TEMP_SENSOR_ENABLE
    //PsramcModeRegRead(p,0x4);   //read refresh rate sw mode
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_CTRL0, 0x4, SWCMD_CTRL0_MRSMA);
    vIO32WriteFldMulti(PSRAMC_REG_HMR4, P_Fld(0x0, HMR4_REFRDIS)
                       | P_Fld(0x1, HMR4_REFR_PERIOD_OPT));
    mcSHOW_DBG_MSG("TEMP_SENSOR: ON\n");
#else /* #ifdef TEMP_SENSOR_ENABLE */
    vIO32WriteFldMulti(PSRAMC_REG_HMR4, P_Fld(0x1, HMR4_REFRDIS)
                       | P_Fld(0x1, HMR4_REFR_PERIOD_OPT));
    mcSHOW_DBG_MSG("TEMP_SENSOR: OFF\n");
#endif /* #ifdef TEMP_SENSOR_ENABLE */
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(0, "//TEMP_SENSOR_ENABLE end\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

    mcSHOW_DBG_MSG("=========================\n");
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(0, "//DramcRunTimeConfig end\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

}

void PsramDDRPhyPLLSetting(DRAMC_CTX_T *p)
{
    U8 i;
    U8 u1CAP_SEL;
    U8 u1MIDPICAP_SEL;
    U8 u1VTH_SEL;
    U16 u2SDM_PCW = 0;
    U8 u1CA_DLL_Mode[2];
    U8 iChannel = CHANNEL_A;
    //U8 u1BRPI_MCTL_EN_CA = 0;
#if ENABLE_TMRRI_NEW_MODE
    //U8 u1RankIdx;
#endif /* #if ENABLE_TMRRI_NEW_MODE */

    u1VTH_SEL = 0x2; /* RG_*RPI_MIDPI_VTH_SEL[1:0] is 2 for all freqs */

#if (fcFOR_CHIP_ID == fcMockingbird)
    //Freq > 1333, CAP_SEL=0
    //Freq 801~1333, CAP_SEL=2
    //Freq <= 800, CAP_SEL=3
    u1CAP_SEL = 0x3;
#endif /* #if (fcFOR_CHIP_ID == fcMockingbird) */

    if (p->frequency <= 1866) {
        u1MIDPICAP_SEL = 0x2;
    } else {
        u1MIDPICAP_SEL = 0x0;
    }


    vIO32WriteFldAlign_All(DDRPHY_SHU1_PLL4, 0x0, SHU1_PLL4_RG_RPHYPLL_RESERVED);
    vIO32WriteFldAlign_All(DDRPHY_SHU1_PLL6, 0x0, SHU1_PLL6_RG_RCLRPLL_RESERVED);

#if (fcFOR_CHIP_ID == fcMockingbird)
#if 0
    u1BRPI_MCTL_EN_CA = 1;
    u1CA_DLL_Mode[CHANNEL_A] = u1CA_DLL_Mode[CHANNEL_B] = DLL_MASTER;
    vIO32WriteFldAlign(DDRPHY_MISC_SHU_OPT + ((U32)CHANNEL_A << POS_BANK_NUM), 1, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN);
    vIO32WriteFldAlign(DDRPHY_MISC_SHU_OPT + SHIFT_TO_CHB_ADDR, 1, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN);
    vIO32WriteFldMulti(DDRPHY_CKMUX_SEL + ((U32)CHANNEL_A << POS_BANK_NUM), P_Fld(0, CKMUX_SEL_FMEM_CK_MUX) | P_Fld(0, CKMUX_SEL_FB_CK_MUX));
    vIO32WriteFldMulti(DDRPHY_CKMUX_SEL + SHIFT_TO_CHB_ADDR, P_Fld(2, CKMUX_SEL_FMEM_CK_MUX) | P_Fld(2, CKMUX_SEL_FB_CK_MUX));
    vIO32WriteFldAlign_All(DDRPHY_SHU1_CA_CMD0, 0x0, SHU1_CA_CMD0_RG_FB_CK_MUX);
#else /* #if 0 */
    u1CA_DLL_Mode[CHANNEL_A] = DLL_MASTER;
    u1CA_DLL_Mode[CHANNEL_B] = DLL_SLAVE;
    vIO32WriteFldAlign(DDRPHY_MISC_SHU_OPT + ((U32)CHANNEL_A << POS_BANK_NUM), 1, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN);
    //vIO32WriteFldAlign(DDRPHY_MISC_SHU_OPT+SHIFT_TO_CHB_ADDR, 2, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN);
    vIO32WriteFldMulti(DDRPHY_CKMUX_SEL + ((U32)CHANNEL_A << POS_BANK_NUM), P_Fld(1, CKMUX_SEL_FMEM_CK_MUX) | P_Fld(1, CKMUX_SEL_FB_CK_MUX));
    //vIO32WriteFldMulti(DDRPHY_CKMUX_SEL+SHIFT_TO_CHB_ADDR, P_Fld(1, CKMUX_SEL_FMEM_CK_MUX) | P_Fld(1, CKMUX_SEL_FB_CK_MUX));
    vIO32WriteFldAlign_All(DDRPHY_SHU1_CA_CMD0, 0x1, SHU1_CA_CMD0_RG_FB_CK_MUX);
#endif /* #if 0 */

#if ENABLE_DLL_ALL_SLAVE_MODE
    if (p->frequency <= 933) {
        u1CA_DLL_Mode[CHANNEL_A] = u1CA_DLL_Mode[CHANNEL_B] = DLL_SLAVE;
    }
#endif /* #if ENABLE_DLL_ALL_SLAVE_MODE */

#if 0
    if (u1CA_DLL_Mode[CHANNEL_A] == DLL_SLAVE) { //All slave mode
        vIO32WriteFldAlign_All(DRAMC_REG_DVFSDLL, 1, DVFSDLL_R_BYPASS_1ST_DLL_SHU1);
    } else {
        vIO32WriteFldAlign_All(DRAMC_REG_DVFSDLL, 0, DVFSDLL_R_BYPASS_1ST_DLL_SHU1);
    }
#endif /* #if 0 */

    //for(iChannel=CHANNEL_A; iChannel<=CHANNEL_B; iChannel++)
    //  {
    if (u1CA_DLL_Mode[iChannel] == DLL_MASTER) {
        vIO32WriteFldMulti(DDRPHY_SHU1_CA_DLL0 + ((U32)iChannel << POS_BANK_NUM), P_Fld(0x0, SHU1_CA_DLL0_RG_ARDLL_PHDET_OUT_SEL_CA)
                           | P_Fld(0x0, SHU1_CA_DLL0_RG_ARDLL_PHDET_IN_SWAP_CA)
                           | P_Fld(0x6, SHU1_CA_DLL0_RG_ARDLL_GAIN_CA)
                           | P_Fld(0x9, SHU1_CA_DLL0_RG_ARDLL_IDLECNT_CA)
                           | P_Fld(0x8, SHU1_CA_DLL0_RG_ARDLL_P_GAIN_CA)
                           | P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHJUMP_EN_CA)
                           | P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHDIV_CA)
                           | P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_FAST_PSJP_CA));
        vIO32WriteFldMulti(DDRPHY_SHU1_CA_DLL1 + ((U32)iChannel << POS_BANK_NUM), P_Fld(0x1, SHU1_CA_DLL1_RG_ARDLL_PD_CK_SEL_CA) | P_Fld(0x0, SHU1_CA_DLL1_RG_ARDLL_FASTPJ_CK_SEL_CA));
        vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD6 + ((U32)iChannel << POS_BANK_NUM), 1, RG_ARPI_RESERVE_BIT_01_DLL_FAST_PSJP); // RG_*RPI_RESERVE_CA[1] 1'b1 tracking leaf(slave)
    } else {
        vIO32WriteFldMulti(DDRPHY_SHU1_CA_DLL0 + ((U32)iChannel << POS_BANK_NUM), P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHDET_OUT_SEL_CA)
                           | P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHDET_IN_SWAP_CA)
                           | P_Fld(0x7, SHU1_CA_DLL0_RG_ARDLL_GAIN_CA)
                           | P_Fld(0x7, SHU1_CA_DLL0_RG_ARDLL_IDLECNT_CA)
                           | P_Fld(0x8, SHU1_CA_DLL0_RG_ARDLL_P_GAIN_CA)
                           | P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHJUMP_EN_CA)
                           | P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHDIV_CA)
                           | P_Fld(0x0, SHU1_CA_DLL0_RG_ARDLL_FAST_PSJP_CA));
        vIO32WriteFldMulti(DDRPHY_SHU1_CA_DLL1 + ((U32)iChannel << POS_BANK_NUM), P_Fld(0x0, SHU1_CA_DLL1_RG_ARDLL_PD_CK_SEL_CA) | P_Fld(0x1, SHU1_CA_DLL1_RG_ARDLL_FASTPJ_CK_SEL_CA));
        vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD6 + ((U32)iChannel << POS_BANK_NUM), 0, RG_ARPI_RESERVE_BIT_01_DLL_FAST_PSJP); // RG_*RPI_RESERVE_CA[1] 1'b1 tracking leaf(slave)
    }
    //  }
#endif /* #if (fcFOR_CHIP_ID == fcMockingbird) */


    U32 u4RegBackupAddress[] = {
        (DDRPHY_B0_DQ7),
        (DDRPHY_CA_CMD7),
    };

    //if(p->vendor_id==VENDOR_SAMSUNG && p->dram_type==TYPE_LPDDR3)
    {
#if 0
        mcSHOW_DBG_MSG("DDRPhyPLLSetting-DMSUS\n");
        vIO32WriteFldAlign_All(DDRPHY_MISC_SPM_CTRL0, 0x0, MISC_SPM_CTRL0_PHY_SPM_CTL0);
        vIO32WriteFldAlign_All(DDRPHY_MISC_SPM_CTRL2, 0x0, MISC_SPM_CTRL2_PHY_SPM_CTL2);
        vIO32WriteFldMulti_All(DDRPHY_MISC_SPM_CTRL1, P_Fld(0x1, MISC_SPM_CTRL1_RG_ARDMSUS_10) | P_Fld(0x1, MISC_SPM_CTRL1_RG_ARDMSUS_10_B0)
                               | P_Fld(0x1, MISC_SPM_CTRL1_RG_ARDMSUS_10_B1) | P_Fld(0x1, MISC_SPM_CTRL1_RG_ARDMSUS_10_CA));
#else /* #if 0 */
        DramcBackupRegisters(p, u4RegBackupAddress, sizeof(u4RegBackupAddress) / sizeof(U32));
        vIO32WriteFldMulti_All(DDRPHY_B0_DQ7, P_Fld(0x1, B0_DQ7_RG_TX_ARDQ_PULL_DN_B0)
                               | P_Fld(0x1, B0_DQ7_RG_TX_ARDQM0_PULL_DN_B0)
                               | P_Fld(0x1, B0_DQ7_RG_TX_ARDQS0_PULL_DN_B0)
                               | P_Fld(0x1, B0_DQ7_RG_TX_ARDQS0B_PULL_DN_B0));
        vIO32WriteFldMulti_All(DDRPHY_CA_CMD7, P_Fld(0x1, CA_CMD7_RG_TX_ARCMD_PULL_DN)
                               | P_Fld(0x1, CA_CMD7_RG_TX_ARCS_PULL_DN)
                               | P_Fld(0x1, CA_CMD7_RG_TX_ARCLK_PULL_DN)
                               | P_Fld(0x1, CA_CMD7_RG_TX_ARCLKB_PULL_DN));

        // DMSUS replaced by CA_CMD2_RG_TX_ARCMD_OE_DIS, CMD_OE_DIS(1) will prevent illegal command ouput
        // And DRAM 1st reset_n pulse will disappear if use CA_CMD2_RG_TX_ARCMD_OE_DIS
        vIO32WriteFldAlign_All(DDRPHY_CA_CMD2, 1, CA_CMD2_RG_TX_ARCMD_OE_DIS);
#endif /* #if 0 */
    }

    //26M
    vIO32WriteFldAlign_All(DDRPHY_MISC_CG_CTRL0, 0x0, MISC_CG_CTRL0_CLK_MEM_SEL);

#ifdef USE_CLK26M
    vIO32WriteFldAlign_All(DDRPHY_MISC_CG_CTRL0, 0x0, MISC_CG_CTRL0_RG_DA_RREF_CK_SEL);
#endif /* #ifdef USE_CLK26M */


    //MIDPI
    vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ6, P_Fld(0x0, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0)
                           | P_Fld(0x0, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0)
                           | P_Fld(0x1, SHU1_B0_DQ6_RG_ARPI_MIDPI_BYPASS_EN_B0));

    vIO32WriteFldMulti_All(DDRPHY_PLL4, P_Fld(0x0, PLL4_RG_RPHYPLL_ADA_MCK8X_EN)
                           | P_Fld(0x0, PLL4_RG_RPHYPLL_RESETB));

    //PLL
    vIO32WriteFldAlign_All(DDRPHY_PLL1, 0x0, PLL1_RG_RPHYPLL_EN);
    vIO32WriteFldAlign_All(DDRPHY_PLL2, 0x0, PLL2_RG_RCLRPLL_EN);

    //DLL
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI2, 0x0, B0_DLL_ARPI2_RG_ARDLL_PHDET_EN_B0);

    vIO32WriteFldMulti_All(DDRPHY_B0_DLL_ARPI2_PSRAM, P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCK_B0_PSRAM)
                           | P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCK_FB2DLL_B0_PSRAM)
                           | P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCTL_B0_PSRAM)
                           | P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_FB_B0_PSRAM)
                           | P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQS_B0_PSRAM)
                           | P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQM_B0_PSRAM)
                           | P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQ_B0_PSRAM)
                           | P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQSIEN_B0_PSRAM)
                           | P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_MPDIV_CG_B0_PSRAM));

    vIO32WriteFldMulti_All(DDRPHY_B0_DLL_ARPI2, P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_MCK_B0)
                           | P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_B0)
                           | P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_MCTL_B0)
                           | P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_FB_B0)
                           | P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_DQS_B0)
                           | P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_DQM_B0)
                           | P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_DQ_B0)
                           | P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_DQSIEN_B0)
                           | P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_MPDIV_CG_B0));
    vIO32WriteFldMulti_All(DDRPHY_CA_DLL_ARPI2, P_Fld(0x0, CA_DLL_ARPI2_RG_ARPI_CG_CLK) | P_Fld(0x1, CA_DLL_ARPI2_RG_ARPI_CG_CLKIEN));

    //RESETB
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI0, 0x0, B0_DLL_ARPI0_RG_ARPI_RESETB_B0);

    mcDELAY_US(1);

    ///TODO: PLL/MIDPI Settings
    //Ref clock should be 20M~30M, if MPLL=52M, Pre-divider should be set to 1
#ifdef USE_CLK26M
    vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL8, P_Fld(0x0, SHU1_PLL8_RG_RPHYPLL_POSDIV)
                           | P_Fld(0x0, SHU1_PLL8_RG_RPHYPLL_PREDIV));
    vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL10, P_Fld(0x0, SHU1_PLL10_RG_RCLRPLL_POSDIV)
                           | P_Fld(0x0, SHU1_PLL10_RG_RCLRPLL_PREDIV));
#else /* #ifdef USE_CLK26M */
    vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL8, P_Fld(0x0, SHU1_PLL8_RG_RPHYPLL_POSDIV)
                           | P_Fld(0x1, SHU1_PLL8_RG_RPHYPLL_PREDIV));
    vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL10, P_Fld(0x0, SHU1_PLL10_RG_RCLRPLL_POSDIV)
                           | P_Fld(0x1, SHU1_PLL10_RG_RCLRPLL_PREDIV));
#endif /* #ifdef USE_CLK26M */

    if (p->frequency == 800) {
        u2SDM_PCW = 0x7600;
    } else if (p->frequency == 1066) {
        u2SDM_PCW = 0x5000;
    } else {
        u2SDM_PCW = 0X7600;
    }

    /* SDM_PCW: Feedback divide ratio (8-bit integer + 8-bit fraction)
     * PLL_SDM_FRA_EN: SDMPLL fractional mode enable (0:Integer mode, 1:Fractional mode)
     */
    vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL5, P_Fld(u2SDM_PCW, SHU1_PLL5_RG_RPHYPLL_SDM_PCW)
                           | P_Fld(0x0, SHU1_PLL5_RG_RPHYPLL_SDM_FRA_EN)); // Disable fractional mode
    vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL7, P_Fld(u2SDM_PCW, SHU1_PLL7_RG_RCLRPLL_SDM_PCW)
                           | P_Fld(0x0, SHU1_PLL7_RG_RCLRPLL_SDM_FRA_EN)); // Disable fractional mode


    vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI0, 1, CA_DLL_ARPI0_RG_ARMCTLPLL_CK_SEL_CA);
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI0, 1, B0_DLL_ARPI0_RG_ARMCTLPLL_CK_SEL_B0);
    vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI1, 0, CA_DLL_ARPI1_RG_ARPI_CLKIEN_JUMP_EN);
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI1, 0, B0_DLL_ARPI1_RG_ARPI_MCTL_JUMP_EN_B0);

    vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ6, P_Fld(u1VTH_SEL, SHU1_B0_DQ6_RG_ARPI_MIDPI_VTH_SEL_B0)
                           | P_Fld(u1CAP_SEL, SHU1_B0_DQ6_RG_ARPI_CAP_SEL_B0)
                           | P_Fld(u1MIDPICAP_SEL, SHU1_B0_DQ6_RG_ARPI_MIDPI_CAP_SEL_B0));

    ///TODO: PLL EN
    vIO32WriteFldAlign_All(DDRPHY_PLL1, 0x1, PLL1_RG_RPHYPLL_EN);
    vIO32WriteFldAlign_All(DDRPHY_PLL2, 0x1, PLL2_RG_RCLRPLL_EN);
    if (FOR_DV_SIMULATION_USED == 1) {
        for (i = 0; i < 100; i++)
            mcDELAY_US(1);
    } else
        mcDELAY_US(100);

    ///TODO: MIDPI Init 2
    /* MIDPI Settings (Olympus): DA_*RPI_MIDPI_EN, DA_*RPI_MIDPI_CKDIV4_EN
     * Justin suggests use frequency > 933 as boundary
     */
    if (p->frequency > 933) {
        vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ6, P_Fld(0x1, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0)
                               | P_Fld(0x0, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0));
    } else {
        vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ6, P_Fld(0x0, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0)
                               | P_Fld(0x1, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0));
    }
    mcDELAY_US(1);

    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI0, 0x1, B0_DLL_ARPI0_RG_ARPI_RESETB_B0);
    mcDELAY_US(1);

    ///TODO: MIDPI Init 1
    vIO32WriteFldMulti_All(DDRPHY_PLL4, P_Fld(0x1, PLL4_RG_RPHYPLL_ADA_MCK8X_EN)
                           | P_Fld(0x1, PLL4_RG_RPHYPLL_RESETB));

    mcDELAY_US(1);

    vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI3, 0x1, CA_DLL_ARPI3_RG_ARPI_CLK_EN);
    vIO32WriteFldMulti_All(DDRPHY_B0_DLL_ARPI3, P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_MCTL_EN_B0)
                           | P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_FB_EN_B0)
                           | P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_DQS_EN_B0)
                           | P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_DQM_EN_B0)
                           | P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_DQ_EN_B0)
                           | P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_DQSIEN_EN_B0));

    vIO32WriteFldMulti_All(DDRPHY_B0_DLL_ARPI2_PSRAM, P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCK_B0_PSRAM)
                           | P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCK_FB2DLL_B0_PSRAM)
                           | P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCTL_B0_PSRAM)
                           | P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_FB_B0_PSRAM)
                           | P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQS_B0_PSRAM)
                           | P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQM_B0_PSRAM)
                           | P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQ_B0_PSRAM)
                           | P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_MPDIV_CG_B0_PSRAM));

    vIO32WriteFldMulti_All(DDRPHY_B0_DLL_ARPI2, P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_MCK_B0)
                           | P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_B0)
                           | P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_MCTL_B0)
                           | P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_FB_B0)
                           | P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_DQS_B0)
                           | P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_DQM_B0)
                           | P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_DQ_B0)
                           | P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_MPDIV_CG_B0));

#if (fcFOR_CHIP_ID == fcMockingbird)
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI2_PSRAM, 0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQSIEN_B0_PSRAM);
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI2, 0, B0_DLL_ARPI2_RG_ARPI_CG_DQSIEN_B0);
#endif /* #if (fcFOR_CHIP_ID == fcMockingbird) */

    mcDELAY_US(2);

    vIO32WriteFldAlign_All(DDRPHY_MISC_CG_CTRL0, 0x1, MISC_CG_CTRL0_CLK_MEM_SEL);
    mcDELAY_US(1);

    //DLL
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI2, 0x1, B0_DLL_ARPI2_RG_ARDLL_PHDET_EN_B0);
    mcDELAY_US(1);

    {
#if 0
        mcSHOW_DBG_MSG("DDRPhyPLLSetting-DMSUS\n\n");
        vIO32WriteFldMulti_All(DDRPHY_MISC_SPM_CTRL1, P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10) | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_B0)
                               | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_B1) | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_CA));
        vIO32WriteFldAlign_All(DDRPHY_MISC_SPM_CTRL0, 0xffffffff, MISC_SPM_CTRL0_PHY_SPM_CTL0);
        vIO32WriteFldAlign_All(DDRPHY_MISC_SPM_CTRL2, 0xffffffff, MISC_SPM_CTRL2_PHY_SPM_CTL2);
#else /* #if 0 */
        // DMSUS replaced by CA_CMD2_RG_TX_ARCMD_OE_DIS, CMD_OE_DIS(1) will prevent illegal command ouput
        vIO32WriteFldAlign_All(DDRPHY_CA_CMD2, 0, CA_CMD2_RG_TX_ARCMD_OE_DIS);
        DramcRestoreRegisters(p, u4RegBackupAddress, sizeof(u4RegBackupAddress) / sizeof(U32));
#endif /* #if 0 */
        mcSHOW_DBG_MSG("PsramDDRPhyPLLSetting-CKEON\n\n");
    }

    DDRPhyFreqMeter();
}

void PsramDDRPhyReservedRGSetting(DRAMC_CTX_T *p)
{
    U8 u1HYST_SEL = 0;
    U8 u1MIDPI_CAP_SEL2 = 0;
    U8 u1LP3_SEL = 0;
    U8 u1SER_RST_MODE = 1;
    U8 u1TX_READ_BASE_EN = 1;
    U8 u1ARPI_BIT4to10 = 0;
    U8 u1PSMUX_DRV_SEL = 0;
    U8 u1Bypass_SR = 1;

    if (p->frequency <= 1333)
        u1HYST_SEL = 1;

    if (p->frequency < 1333)
        u1MIDPI_CAP_SEL2 = 1;
    else
        u1MIDPI_CAP_SEL2 = 0;

    if (p->frequency >= 933)
        u1PSMUX_DRV_SEL = 1;

    if (p->frequency <= 933)
        u1ARPI_BIT4to10 = 1;

    u1Bypass_SR = 1;

    vIO32WriteFldAlign_All(DDRPHY_SHU1_PLL4, 1, RG_PLL_RESERVE_BIT_13_PLL_FS_EN);
    vIO32WriteFldAlign_All(DDRPHY_SHU1_PLL6, 1, RG_PLL_RESERVE_BIT_13_PLL_FS_EN);

    //PI
    vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD6, P_Fld(0x1, RG_ARPI_RESERVE_BIT_00_TX_CG_EN) // RG_*RPI_RESERVE_B0[0] 1'b1 prevent leakage
                       | P_Fld(1, RG_ARPI_RESERVE_BIT_01_DLL_FAST_PSJP)
                       | P_Fld(u1HYST_SEL, RG_ARPI_RESERVE_BIT_02_HYST_SEL)
                       | P_Fld(u1MIDPI_CAP_SEL2, RG_ARPI_RESERVE_BIT_03_MIDPI_CAP_SEL)
                       | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_04_8PHASE_XLATCH_FORCE)
                       | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_05_PSMUX_XLATCH_FORCE)
                       | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_06_PSMUX_XLATCH_FORCEDQS)
                       | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_07_SMT_XLATCH_FORCE)
                       | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_08_SMT_XLATCH_FORCE_DQS)
                       | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_09_BUFGP_XLATCH_FORCE)
                       | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_10_BUFGP_XLATCH_FORCE_DQS)
                       | P_Fld(u1Bypass_SR, RG_ARPI_RESERVE_BIT_11_BYPASS_SR)
                       | P_Fld(u1Bypass_SR, RG_ARPI_RESERVE_BIT_12_BYPASS_SR_DQS)
                       | P_Fld(0, RG_ARPI_RESERVE_BIT_13_CG_SYNC_ENB)
                       | P_Fld(u1LP3_SEL, RG_ARPI_RESERVE_BIT_14_LP3_SEL)
                       | P_Fld(u1PSMUX_DRV_SEL, RG_ARPI_RESERVE_BIT_15_PSMUX_DRV_SEL));

    vIO32WriteFldMulti_All(DDRPHY_SHU1_CA_DLL1, P_Fld(0x1, RG_ARCMD_REV_BIT_00_TX_LSH_DQ_CG_EN)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_01_TX_LSH_DQS_CG_EN)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_02_TX_LSH_DQM_CG_EN)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_03_RX_DQS_GATE_EN_MODE)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_04_RX_DQSIEN_RB_DLY)
                           | P_Fld(u1SER_RST_MODE, RG_ARCMD_REV_BIT_05_RX_SER_RST_MODE)
                           | P_Fld(0x1, RG_ARCMD_REV_BIT_06_MCK4X_SEL_CKE0)
                           | P_Fld(0x1, RG_ARCMD_REV_BIT_07_MCK4X_SEL_CKE1)
                           | P_Fld(0x4, RG_ARCMD_REV_BIT_1208_TX_CKE_DRVN)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_13_TX_DDR3_CKE_SEL)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_14_TX_DDR4_CKE_SEL)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_15_TX_DDR4P_CKE_SEL)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_1716_TX_LP4Y_SEL)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_18_RX_LP4Y_EN)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_19_RX_DQSIEN_FORCE_ON_EN)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_20_DATA_SWAP_EN)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_2221_DATA_SWAP)
                           | P_Fld(0x0, RG_ARCMD_REV_BIT_23_NA));

    vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ6, P_Fld(0x1, RG_ARPI_RESERVE_BIT_00_TX_CG_EN) // RG_*RPI_RESERVE_B0[0] 1'b1 prevent leakage
                           | P_Fld(0, RG_ARPI_RESERVE_BIT_01_DLL_FAST_PSJP)
                           | P_Fld(u1HYST_SEL, RG_ARPI_RESERVE_BIT_02_HYST_SEL)
                           | P_Fld(u1MIDPI_CAP_SEL2, RG_ARPI_RESERVE_BIT_03_MIDPI_CAP_SEL)
                           | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_04_8PHASE_XLATCH_FORCE)
                           | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_05_PSMUX_XLATCH_FORCE)
                           | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_06_PSMUX_XLATCH_FORCEDQS)
                           | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_07_SMT_XLATCH_FORCE)
                           | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_08_SMT_XLATCH_FORCE_DQS)
                           | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_09_BUFGP_XLATCH_FORCE)
                           | P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_10_BUFGP_XLATCH_FORCE_DQS)
                           | P_Fld(u1Bypass_SR, RG_ARPI_RESERVE_BIT_11_BYPASS_SR)
                           | P_Fld(u1Bypass_SR, RG_ARPI_RESERVE_BIT_12_BYPASS_SR_DQS)
                           | P_Fld(0, RG_ARPI_RESERVE_BIT_13_CG_SYNC_ENB)
                           | P_Fld(u1LP3_SEL, RG_ARPI_RESERVE_BIT_14_LP3_SEL)
                           | P_Fld(u1PSMUX_DRV_SEL, RG_ARPI_RESERVE_BIT_15_PSMUX_DRV_SEL));

    //TX
    vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DLL1, P_Fld(0x0, RG_ARDQ_REV_BIT_00_DQS_MCK4X_DLY_EN)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_01_DQS_MCK4XB_DLY_EN)
                           | P_Fld(u1TX_READ_BASE_EN, RG_ARDQ_REV_BIT_02_TX_READ_BASE_EN_DQSB)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_03_RX_DQS_GATE_EN_MODE)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_04_RX_DQSIEN_RB_DLY)
                           | P_Fld(u1SER_RST_MODE, RG_ARDQ_REV_BIT_05_RX_SER_RST_MODE)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_06_MCK4X_SEL_DQ1)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_07_MCK4X_SEL_DQ5)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_08_TX_ODT_DISABLE)
                           | P_Fld(u1TX_READ_BASE_EN, RG_ARDQ_REV_BIT_09_TX_READ_BASE_EN)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_1110_DRVN_PRE)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_1312_DRVP_PRE)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_14_TX_PRE_DATA_SEL)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_15_TX_PRE_EN)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_1716_TX_LP4Y_SEL)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_18_RX_LP4Y_EN)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_19_RX_DQSIEN_FORCE_ON_EN)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_20_DATA_SWAP_EN)
                           | P_Fld(0x0, RG_ARDQ_REV_BIT_2221_DATA_SWAP)
                           | P_Fld(0x1, RG_ARDQ_REV_BIT_23_NA));

}

//#define PSRAM_MODEL_32MB

DRAM_STATUS_T PsramUpdateACTimingReg(DRAMC_CTX_T *p, const ACTimePsram_T *ACTbl);

void PSramcInitSetting(DRAMC_CTX_T *p)
{
    //U8 u1CAP_SEL;
    //U8 u1MIDPICAP_SEL;
    //U8 u1TXDLY_CMD;
    U16 u2Tmp;
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(1, "//PSramcInitSetting Start\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

    //before switch clock from 26M to PHY, need to init PHY clock first
    vIO32WriteFldMulti_All(DDRPHY_CKMUX_SEL, P_Fld(0x1, CKMUX_SEL_R_PHYCTRLMUX)  //move CKMUX_SEL_R_PHYCTRLMUX to here (it was originally between MISC_CG_CTRL0_CLK_MEM_SEL and MISC_CTRL0_R_DMRDSEL_DIV2_OPT)
                           | P_Fld(0x1, CKMUX_SEL_R_PHYCTRLDCM)); // PHYCTRLDCM 1: follow DDRPHY_conf DCM settings, 0: follow infra DCM settings

    //chg_mem_en = 1
    vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL0, 0x1, MISC_CG_CTRL0_W_CHG_MEM);//attention
    //26M
    vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL0, 0x0, MISC_CG_CTRL0_CLK_MEM_SEL);

    vIO32WriteFldAlign(DDRPHY_MISC_CTRL0_PSRAM, 0x0, MISC_CTRL0_PSRAM_R_DMRDSEL_DIV2_OPT_PSRAM);

    //Francis : pin mux issue, need to set CHD
    vIO32WriteFldAlign(DDRPHY_MISC_SPM_CTRL2, 0x0, MISC_SPM_CTRL2_PHY_SPM_CTL2);
    vIO32WriteFldAlign(DDRPHY_MISC_SPM_CTRL0, 0x0, MISC_SPM_CTRL0_PHY_SPM_CTL0);
    vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL4_PSRAM, 0x333f3f00, MISC_CG_CTRL4_PSRAM_R_PHY_MCK_CG_CTRL_PSRAM);
    vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL4, 0x11400000, MISC_CG_CTRL4_R_PHY_MCK_CG_CTRL);

    vIO32WriteFldMulti(DDRPHY_SHU1_PLL1, P_Fld(0x1, SHU1_PLL1_R_SHU_AUTO_PLL_MUX)
                       | P_Fld(0x7, SHU1_PLL1_SHU1_PLL1_RFU));

    vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7, P_Fld(0x1, SHU1_B0_DQ7_MIDPI_ENABLE)
                       | P_Fld(0x1, SHU1_B0_DQ7_R_DMRXRANK_DQS_EN_B0)
                       | P_Fld(0x1, SHU1_B0_DQ7_R_DMRXRANK_DQ_LAT_B0)
                       | P_Fld(0x1, SHU1_B0_DQ7_R_DMRXRANK_DQ_EN_B0)
                       | P_Fld(0x0, SHU1_B0_DQ7_MIDPI_DIV4_ENABLE)
                       | P_Fld(0, SHU1_B0_DQ7_R_DMRANKRXDVS_B0));
    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ8, 0x1, SHU1_B0_DQ8_R_RMRX_TOPHY_CG_IG_B0);
    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ9, 0x165ee3, SHU1_B0_DQ9_RG_ARPI_RESERVE_B0);

    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7_PSRAM, 0x0, SHU1_B0_DQ7_PSRAM_R_DMRANKRXDVS_B0_PSRAM);

    vIO32WriteFldMulti(DDRPHY_SHU1_PLL4, P_Fld(0x1, SHU1_PLL4_RG_RPHYPLL_IBIAS)
                       | P_Fld(0x1, SHU1_PLL4_RG_RPHYPLL_ICHP)
                       | P_Fld(0x2, SHU1_PLL4_RG_RPHYPLL_FS));
    vIO32WriteFldMulti(DDRPHY_SHU1_PLL6, P_Fld(0x1, SHU1_PLL6_RG_RCLRPLL_IBIAS)
                       | P_Fld(0x1, SHU1_PLL6_RG_RCLRPLL_ICHP)
                       | P_Fld(0x2, SHU1_PLL6_RG_RCLRPLL_FS));
    vIO32WriteFldAlign(DDRPHY_SHU1_PLL14, 0x0, SHU1_PLL14_RG_RPHYPLL_SDM_SSC_PH_INIT);
    vIO32WriteFldAlign(DDRPHY_SHU1_PLL20, 0x0, SHU1_PLL20_RG_RCLRPLL_SDM_SSC_PH_INIT);
    vIO32WriteFldMulti(DDRPHY_CA_CMD2, P_Fld(0x0, CA_CMD2_RG_TX_ARCMD_OE_DIS)
                       | P_Fld(0x0, CA_CMD2_RG_TX_ARCMD_ODTEN_DIS)
                       | P_Fld(0x0, CA_CMD2_RG_TX_ARCLK_OE_DIS)
                       | P_Fld(0x0, CA_CMD2_RG_TX_ARCLK_ODTEN_DIS));
    vIO32WriteFldMulti(DDRPHY_B0_DQ2, P_Fld(0x0, B0_DQ2_RG_TX_ARDQ_OE_DIS_B0)
                       | P_Fld(0x0, B0_DQ2_RG_TX_ARDQ_ODTEN_DIS_B0)
                       | P_Fld(0x0, B0_DQ2_RG_TX_ARDQS0_OE_DIS_B0)
                       | P_Fld(0x0, B0_DQ2_RG_TX_ARDQS0_ODTEN_DIS_B0));
#if 0 //Correct settings are set in UpdateInitialSettings_LP4()
    vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x0, B0_DQ9_R_IN_GATE_EN_LOW_OPT_B0);
    vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x7, B1_DQ9_R_IN_GATE_EN_LOW_OPT_B1);
    vIO32WriteFldAlign(DDRPHY_CA_CMD10, 0x0, CA_CMD10_R_IN_GATE_EN_LOW_OPT_CA);
#endif /* #if 0 //Correct settings are set in UpdateInitialSettings_LP4() */

    vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x0, B0_DQ9_R_IN_GATE_EN_LOW_OPT_B0);
    vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_R_DMRXDVS_RDSEL_LAT_B0_PSRAM);
    vIO32WriteFldAlign(DDRPHY_CA_CMD10, 0x0, CA_CMD10_R_DMRXDVS_RDSEL_LAT_CA);

    vIO32WriteFldAlign(DDRPHY_B0_RXDVS0, 0x1, B0_RXDVS0_R_RX_DLY_TRACK_CG_EN_B0);
    vIO32WriteFldAlign(DDRPHY_B0_RXDVS0, 0x1, B0_RXDVS0_R_DMRXDVS_DQIENPRE_OPT_B0);
    vIO32WriteFldAlign(DDRPHY_R0_B0_RXDVS2, 0x1, R0_B0_RXDVS2_R_RK0_DVS_FDLY_MODE_B0);
    vIO32WriteFldAlign(DDRPHY_R0_B1_RXDVS2, 0x1, R0_B1_RXDVS2_R_RK0_DVS_FDLY_MODE_B1);
    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ5, 0x3, SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0);
    vIO32WriteFldMulti(DDRPHY_R0_B0_RXDVS1, P_Fld(0x2, R0_B0_RXDVS1_R_RK0_B0_DVS_TH_LEAD)
                       | P_Fld(0x2, R0_B0_RXDVS1_R_RK0_B0_DVS_TH_LAG));
    vIO32WriteFldMulti(DDRPHY_R0_B1_RXDVS1, P_Fld(0x2, R0_B1_RXDVS1_R_RK0_B1_DVS_TH_LEAD)
                       | P_Fld(0x2, R0_B1_RXDVS1_R_RK0_B1_DVS_TH_LAG));

    vIO32WriteFldMulti(DDRPHY_R0_B0_RXDVS2, P_Fld(0x2, R0_B0_RXDVS2_R_RK0_DVS_MODE_B0)
                       | P_Fld(0x1, R0_B0_RXDVS2_R_RK0_RX_DLY_RIS_TRACK_GATE_ENA_B0)
                       | P_Fld(0x1, R0_B0_RXDVS2_R_RK0_RX_DLY_FAL_TRACK_GATE_ENA_B0));
    vIO32WriteFldMulti(DDRPHY_R0_B1_RXDVS2, P_Fld(0x2, R0_B1_RXDVS2_R_RK0_DVS_MODE_B1)
                       | P_Fld(0x1, R0_B1_RXDVS2_R_RK0_RX_DLY_RIS_TRACK_GATE_ENA_B1)
                       | P_Fld(0x1, R0_B1_RXDVS2_R_RK0_RX_DLY_FAL_TRACK_GATE_ENA_B1));

    vIO32WriteFldAlign(DDRPHY_B0_RXDVS0, 0x0, B0_RXDVS0_R_RX_DLY_TRACK_CG_EN_B0);
    vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_RG_RX_ARDQ_STBEN_RESETB_B0_PSRAM);
    vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x1, B0_DQ9_RG_RX_ARDQ_STBEN_RESETB_B0);

#if 0//LEGACY_DELAY_CELL
    LegacyDlyCellInitLP4_DDR3200(p);
#endif /* #if 0//LEGACY_DELAY_CELL */
#if 0 //lzs temp mark
    vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ7, P_Fld(0x1f, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0)
                       | P_Fld(0x1f, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
    vIO32WriteFldMulti(DDRPHY_SHU1_R0_B1_DQ7, P_Fld(0x1f, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1)
                       | P_Fld(0x1f, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
    vIO32WriteFldMulti(DDRPHY_SHU1_R1_B0_DQ7, P_Fld(0x1f, SHU1_R1_B0_DQ7_RK1_ARPI_DQM_B0)
                       | P_Fld(0x1f, SHU1_R1_B0_DQ7_RK1_ARPI_DQ_B0));
    vIO32WriteFldMulti(DDRPHY_SHU1_R1_B1_DQ7, P_Fld(0x1f, SHU1_R1_B1_DQ7_RK1_ARPI_DQM_B1)
                       | P_Fld(0x1f, SHU1_R1_B1_DQ7_RK1_ARPI_DQ_B1));
#endif /* #if 0 //lzs temp mark */
    vIO32WriteFldMulti(DDRPHY_B0_DQ4, P_Fld(0x12, B0_DQ4_RG_RX_ARDQS_EYE_R_DLY_B0)
                       | P_Fld(0x1a, B0_DQ4_RG_RX_ARDQS_EYE_F_DLY_B0)
                       | P_Fld(0xa, B0_DQ4_RG_RX_ARDQ_EYE_R_DLY_B0)
                       | P_Fld(0x12, B0_DQ4_RG_RX_ARDQ_EYE_F_DLY_B0));
    vIO32WriteFldMulti(DDRPHY_B0_DQ5, P_Fld(0x0, B0_DQ5_RG_RX_ARDQ_EYE_EN_B0)
                       | P_Fld(0x0, B0_DQ5_RG_RX_ARDQ_EYE_SEL_B0)
                       | P_Fld(0x1, B0_DQ5_RG_RX_ARDQ_VREF_EN_B0)
                       | P_Fld(0x10, B0_DQ5_B0_DQ5_RFU));
    vIO32WriteFldMulti(DDRPHY_B0_DQ6, P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_EYE_DLY_DQS_BYPASS_B0)
                       | P_Fld(0x1, B0_DQ6_RG_TX_ARDQ_DDR3_SEL_B0)
                       | P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_DDR3_SEL_B0)
                       | P_Fld(0x0, B0_DQ6_RG_TX_ARDQ_DDR4_SEL_B0)
                       | P_Fld(0x0, B0_DQ6_RG_RX_ARDQ_DDR4_SEL_B0)
                       | P_Fld(0x0, B0_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B0)
                       | P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_BIAS_EN_B0)
                       | P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_OP_BIAS_SW_EN_B0)
                       | P_Fld(0x0, B0_DQ6_RG_TX_ARDQ_SER_MODE_B0));
    vIO32WriteFldMulti(DDRPHY_B0_DQ5, P_Fld(0x1, B0_DQ5_RG_RX_ARDQ_EYE_STBEN_RESETB_B0)
                       | P_Fld(0x0, B0_DQ5_B0_DQ5_RFU));

    vIO32WriteFldAlign(DDRPHY_CA_CMD3, 0x1, CA_CMD3_RG_ARCMD_RESETB);
    vIO32WriteFldMulti(DDRPHY_CA_CMD6, P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_DDR4_SEL)
                       | P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_BIAS_VREF_SEL)
                       | P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN));

#if PSRAM_VREF_FROM_RTN
    vIO32WriteFldMulti(DDRPHY_PLL3, P_Fld(0x0, PLL3_RG_RPHYPLL_TSTOP_EN) | P_Fld(0x1, PLL3_RG_RPHYPLL_TST_EN));
    vIO32WriteFldAlign(DDRPHY_MISC_VREF_CTRL, 0x1, MISC_VREF_CTRL_RG_RVREF_VREF_EN); //LP3 VREF
    vIO32WriteFldMulti_All(DDRPHY_SHU1_MISC0, P_Fld(14, SHU1_MISC0_RG_RVREF_SEL_CMD)
                           | P_Fld(0x1, SHU1_MISC0_RG_RVREF_DDR3_SEL)
                           | P_Fld(0x0, SHU1_MISC0_RG_RVREF_DDR4_SEL)
                           | P_Fld(14, SHU1_MISC0_RG_RVREF_SEL_DQ));
#else /* #if PSRAM_VREF_FROM_RTN */
    vIO32WriteFldMulti(DDRPHY_PLL3, P_Fld(0x1, PLL3_RG_RPHYPLL_TSTOP_EN)
                       | P_Fld(0x1, PLL3_RG_RPHYPLL_TST_EN));
    vIO32WriteFldAlign(DDRPHY_MISC_VREF_CTRL, 0x1, MISC_VREF_CTRL_RG_RVREF_VREF_EN); //LP3 VREF
    vIO32WriteFldAlign(DDRPHY_MISC_IMP_CTRL1, 0x1, MISC_IMP_CTRL1_RG_RIMP_SUS_ECO_OPT);
#endif /* #if PSRAM_VREF_FROM_RTN */

    vIO32WriteFldAlign(DDRPHY_B0_DQ3, 0x1, B0_DQ3_RG_ARDQ_RESETB_B0);

    mcDELAY_US(1);

    //Ref clock should be 20M~30M, if MPLL=52M, Pre-divider should be set to 1
    vIO32WriteFldMulti(DDRPHY_SHU1_PLL8, P_Fld(0x0, SHU1_PLL8_RG_RPHYPLL_POSDIV)
                       | P_Fld(0x0, SHU1_PLL8_RG_RPHYPLL_PREDIV));

    mcDELAY_US(1);

    vIO32WriteFldMulti(DDRPHY_SHU1_PLL9, P_Fld(0x0, SHU1_PLL9_RG_RPHYPLL_MONCK_EN)
                       | P_Fld(0x0, SHU1_PLL9_RG_RPHYPLL_MONVC_EN)
                       | P_Fld(0x0, SHU1_PLL9_RG_RPHYPLL_LVROD_EN)
                       | P_Fld(0x2, SHU1_PLL9_RG_RPHYPLL_RST_DLY));
    vIO32WriteFldMulti(DDRPHY_SHU1_PLL11, P_Fld(0x0, SHU1_PLL11_RG_RCLRPLL_MONCK_EN)
                       | P_Fld(0x0, SHU1_PLL11_RG_RCLRPLL_MONVC_EN)
                       | P_Fld(0x0, SHU1_PLL11_RG_RCLRPLL_LVROD_EN)
                       | P_Fld(0x1, SHU1_PLL11_RG_RCLRPLL_RST_DLY));

    mcDELAY_US(1);

    //Ref clock should be 20M~30M, if MPLL=52M, Pre-divider should be set to 1
    vIO32WriteFldMulti(DDRPHY_SHU1_PLL10, P_Fld(0x0, SHU1_PLL10_RG_RCLRPLL_POSDIV)
                       | P_Fld(0x1, SHU1_PLL10_RG_RCLRPLL_PREDIV));

    mcDELAY_US(1);

    ///TODO: MIDPI Init 1
    vIO32WriteFldMulti(DDRPHY_PLL4, P_Fld(0x0, PLL4_RG_RPHYPLL_AD_MCK8X_EN)
                       | P_Fld(0x1, PLL4_PLL4_RFU)
                       | P_Fld(0x1, PLL4_RG_RPHYPLL_MCK8X_SEL));


    mcDELAY_US(1);

    vIO32WriteFldAlign(DDRPHY_SHU1_PLL0, 0x3, SHU1_PLL0_RG_RPHYPLL_TOP_REV); // debug1111, org:3 -> mdf:0

    mcDELAY_US(1);

    vIO32WriteFldMulti(DDRPHY_B0_DQ3, P_Fld(0x1, B0_DQ3_RG_RX_ARDQ_STBENCMP_EN_B0)
                       | P_Fld(0x1, B0_DQ3_RG_TX_ARDQ_EN_B0)
                       | P_Fld(0x1, B0_DQ3_RG_RX_ARDQ_SMT_EN_B0));

#if (fcFOR_CHIP_ID == fcMockingbird)
    vIO32WriteFldAlign(DDRPHY_SHU1_CA_DLL0, 0x1, SHU1_CA_DLL0_RG_ARPISM_MCK_SEL_CA_SHU);
#endif /* #if (fcFOR_CHIP_ID == fcMockingbird) */

    vIO32WriteFldMulti(DDRPHY_SHU1_B0_DLL0, P_Fld(0x0, SHU1_B0_DLL0_RG_ARDLL_PHDET_OUT_SEL_B0)
                       | P_Fld(0x0, SHU1_B0_DLL0_RG_ARDLL_PHDET_IN_SWAP_B0)
                       | P_Fld(0x6, SHU1_B0_DLL0_RG_ARDLL_GAIN_B0)
                       | P_Fld(0x9, SHU1_B0_DLL0_RG_ARDLL_IDLECNT_B0)
                       | P_Fld(0x8, SHU1_B0_DLL0_RG_ARDLL_P_GAIN_B0)
                       | P_Fld(0x1, SHU1_B0_DLL0_RG_ARDLL_PHJUMP_EN_B0)
                       | P_Fld(0x1, SHU1_B0_DLL0_RG_ARDLL_PHDIV_B0)
                       | P_Fld(0x1, SHU1_B0_DLL0_RG_ARDLL_FAST_PSJP_B0));

    vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD5, 0x0, SHU1_CA_CMD5_RG_RX_ARCMD_VREF_SEL);

    vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD0, P_Fld(0x1, SHU1_CA_CMD0_RG_TX_ARCMD_PRE_EN)
                       | P_Fld(0x4, SHU1_CA_CMD0_RG_TX_ARCLK_DRVN_PRE)
                       | P_Fld(0x1, SHU1_CA_CMD0_RG_TX_ARCLK_PRE_EN));

#if (fcFOR_CHIP_ID == fcMockingbird)
    vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD6, 0x3, SHU1_CA_CMD6_RG_ARPI_RESERVE_CA);
#else /* #if (fcFOR_CHIP_ID == fcMockingbird) */
    vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD6, 0x3, SHU1_CA_CMD6_RG_ARPI_RESERVE_CA);
#endif /* #if (fcFOR_CHIP_ID == fcMockingbird) */

    vIO32WriteFldMulti(DDRPHY_MISC_SHU_OPT, P_Fld(0x1, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN)
                       | P_Fld(0x1, MISC_SHU_OPT_R_CA_SHU_PHY_GATING_RESETB_SPM_EN)
                       | P_Fld(0x2, MISC_SHU_OPT_R_DQB1_SHU_PHDET_SPM_EN)
                       | P_Fld(0x1, MISC_SHU_OPT_R_DQB1_SHU_PHY_GATING_RESETB_SPM_EN)
                       | P_Fld(0x2, MISC_SHU_OPT_R_DQB0_SHU_PHDET_SPM_EN)
                       | P_Fld(0x1, MISC_SHU_OPT_R_DQB0_SHU_PHY_GATING_RESETB_SPM_EN));

    mcDELAY_US(9);

#if (fcFOR_CHIP_ID == fcMockingbird)
    vIO32WriteFldMulti(DDRPHY_SHU1_B0_DLL1, P_Fld(0x1, SHU1_B0_DLL1_RG_ARDLL_PD_CK_SEL_B0)
                       | P_Fld(0x0, SHU1_B0_DLL1_RG_ARDLL_FASTPJ_CK_SEL_B0));
#endif /* #if (fcFOR_CHIP_ID == fcMockingbird) */

    mcDELAY_US(1);

    vIO32WriteFldAlign(DDRPHY_PLL2, 0x0, PLL2_RG_RCLRPLL_EN);

    mcDELAY_US(1);

    PsramDDRPhyReservedRGSetting(p);
    PsramDDRPhyPLLSetting(p);

    vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x1, B0_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B0);

    vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_RG_RX_ARDQS0_STBEN_RESETB_B0_PSRAM);
    vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x1, B0_DQ9_R_DMDQSIEN_RDSEL_LAT_B0);
    vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_R_DMDQSIEN_RDSEL_LAT_B0_PSRAM);
    vIO32WriteFldAlign(DDRPHY_CA_CMD10, 0x0, CA_CMD10_R_DMDQSIEN_RDSEL_LAT_CA);
    vIO32WriteFldMulti(DDRPHY_MISC_CTRL0_PSRAM, P_Fld(0x0, MISC_CTRL0_PSRAM_R_STBENCMP_DIV4CK_EN_PSRAM)
                       | P_Fld(0x0, MISC_CTRL0_PSRAM_R_DQS1IEN_DIV4_CK_CG_CTRL_PSRAM)
                       | P_Fld(0x1, MISC_CTRL0_PSRAM_R_DMDQSIEN_FIFO_EN_PSRAM)
                       | P_Fld(0x1, MISC_CTRL0_PSRAM_IMPCAL_CDC_ECO_OPT_PSRAM)
                       | P_Fld(0x1, MISC_CTRL0_PSRAM_IMPCAL_LP_ECO_OPT_PSRAM)
                       | P_Fld(0x1, MISC_CTRL0_PSRAM_R_DMSTBEN_OUTSEL_PSRAM)
                       | P_Fld(0xf, MISC_CTRL0_PSRAM_R_DMDQSIEN_SYNCOPT_PSRAM));

    vIO32WriteFldMulti(DDRPHY_MISC_CTRL0, P_Fld(0x0, MISC_CTRL0_R_STBENCMP_DIV4CK_EN)
                       | P_Fld(0x0, MISC_CTRL0_R_DQS1IEN_DIV4_CK_CG_CTRL)
                       | P_Fld(0x1, MISC_CTRL0_R_DMDQSIEN_FIFO_EN)
                       | P_Fld(0x1, MISC_CTRL0_IMPCAL_CDC_ECO_OPT)
                       | P_Fld(0x1, MISC_CTRL0_IMPCAL_LP_ECO_OPT)
                       | P_Fld(0x1, MISC_CTRL0_R_DMSTBEN_OUTSEL)
                       | P_Fld(0x0, MISC_CTRL0_R_DMDQSIEN_SYNCOPT));

    vIO32WriteFldAlign(DDRPHY_B0_RXDVS0, 1, B0_RXDVS0_R_HWSAVE_MODE_ENA_B0);
    vIO32WriteFldAlign(DDRPHY_CA_RXDVS0, 0, CA_RXDVS0_R_HWSAVE_MODE_ENA_CA);

    vIO32WriteFldAlign(DDRPHY_CA_CMD7, 0x0, CA_CMD7_RG_TX_ARCMD_PULL_DN);
    vIO32WriteFldAlign(DDRPHY_CA_CMD7, 0x0, CA_CMD7_RG_TX_ARCS_PULL_DN); // Added by Lingyun.Wu, 11-15
    vIO32WriteFldAlign(DDRPHY_B0_DQ7, 0x0, B0_DQ7_RG_TX_ARDQ_PULL_DN_B0);

    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7, 0x1, SHU1_B0_DQ7_R_DMRODTEN_B0);

    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7_PSRAM, 0x1, SHU1_B0_DQ7_PSRAM_R_DMRODTEN_B0_PSRAM);

#if 0 //lzs temp mark
    vIO32WriteFldMulti(DDRPHY_SHU1_R0_B1_DQ7, P_Fld(0x1a, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1)
                       | P_Fld(0x1a, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
    vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ7, P_Fld(0x1a, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0)
                       | P_Fld(0x1a, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
    vIO32WriteFldMulti(DDRPHY_SHU1_R1_B1_DQ7, P_Fld(0x14, SHU1_R1_B1_DQ7_RK1_ARPI_DQM_B1)
                       | P_Fld(0x14, SHU1_R1_B1_DQ7_RK1_ARPI_DQ_B1));
    vIO32WriteFldMulti(DDRPHY_SHU1_R1_B0_DQ7, P_Fld(0x14, SHU1_R1_B0_DQ7_RK1_ARPI_DQM_B0)
                       | P_Fld(0x14, SHU1_R1_B0_DQ7_RK1_ARPI_DQ_B0));
#endif /* #if 0 //lzs temp mark */
    mcDELAY_US(1);

    vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x1, B0_DQ9_RG_RX_ARDQS0_DQSIENMODE_B0);
    vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_RG_RX_ARDQS0_DQSIENMODE_B0_PSRAM);
    vIO32WriteFldAlign(DDRPHY_B0_DQ6, 0x1, B0_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B0);

    vIO32WriteFldAlign(DDRPHY_MISC_CTRL0_PSRAM, 0x0, MISC_CTRL0_PSRAM_R_DMDQSIEN_SYNCOPT_PSRAM);

    vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_DQM_EN_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_FLAG_OPT_B0_PSRAM));


    ///TODO: DDR3733
    if (p->freqGroup == 800) {
        vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7_PSRAM, 0x0, SHU1_B0_DQ7_PSRAM_R_DMRODTEN_B0_PSRAM);
        vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ5, 0x4, SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0);

        vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_DQM_EN_B0_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMDQMDBI_SHU_B0_PSRAM)
                           | P_Fld(0x1, SHU1_B0_DQ7_PSRAM_MIDPI_DIV4_ENABLE_PSRAM)
                           | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_MIDPI_ENABLE_PSRAM));
    }

    vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRODTEN_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ7_PSRAM_MIDPI_ENABLE_PSRAM));

    vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD0, P_Fld(0x1, SHU1_CA_CMD0_RG_TX_ARCMD_PRE_EN) // OE Suspend EN
                       | P_Fld(0x1, SHU1_CA_CMD0_RG_TX_ARCLK_PRE_EN)); //ODT Suspend EN

    //close RX DQ/DQS tracking to save power
    vIO32WriteFldMulti(DDRPHY_R0_B0_RXDVS2, P_Fld(0x0, R0_B0_RXDVS2_R_RK0_DVS_MODE_B0)
                       | P_Fld(0x0, R0_B0_RXDVS2_R_RK0_RX_DLY_RIS_TRACK_GATE_ENA_B0)
                       | P_Fld(0x0, R0_B0_RXDVS2_R_RK0_RX_DLY_FAL_TRACK_GATE_ENA_B0));

    //wei-jen: RX rank_sel for CA is not used(Bianco), set it's dly to 0 to save power
    vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD7_PSRAM, 0, SHU1_CA_CMD7_PSRAM_R_DMRANKRXDVS_CA_PSRAM);

    vIO32WriteFldAlign(DDRPHY_CA_CMD10, 0x0, CA_CMD10_RG_RX_ARCLK_DQSIENMODE);

    vIO32WriteFldMulti(DDRPHY_B0_DQ3, P_Fld(0x1, B0_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B0)
                       | P_Fld(0x0, B0_DQ3_RG_RX_ARDQM0_IN_BUFF_EN_B0)
                       | P_Fld(0x1, B0_DQ3_RG_RX_ARDQS0_IN_BUFF_EN_B0));

    vIO32WriteFldAlign(DDRPHY_B0_DQ3, 0x0, B0_DQ3_RG_RX_ARDQ_SMT_EN_B0);
    vIO32WriteFldAlign(DDRPHY_B0_DQ5, 0x0, B0_DQ5_RG_RX_ARDQS0_DVS_EN_B0);
    vIO32WriteFldAlign(DDRPHY_CA_CMD5, 0x0, CA_CMD5_RG_RX_ARCLK_DVS_EN);

    vIO32WriteFldMulti(DDRPHY_CA_CMD6, P_Fld(0x1, CA_CMD6_RG_TX_ARCMD_DDR3_SEL)
                       | P_Fld(0x0, CA_CMD6_RG_TX_ARCMD_DDR4_SEL)
                       | P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_DDR3_SEL)
                       | P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_DDR4_SEL));
    vIO32WriteFldMulti(DDRPHY_MISC_IMP_CTRL0, P_Fld(0x1, MISC_IMP_CTRL0_RG_RIMP_DDR3_SEL)
                       | P_Fld(0x0, MISC_IMP_CTRL0_RG_RIMP_DDR4_SEL));

    vIO32WriteFldAlign(DDRPHY_B0_DQ6, 0x0, B0_DQ6_RG_RX_ARDQ_O1_SEL_B0);
    vIO32WriteFldAlign(DDRPHY_CA_CMD6, 0x0, CA_CMD6_RG_RX_ARCMD_O1_SEL);
    vIO32WriteFldAlign(DDRPHY_B0_DQ6, 0x1, B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0);
    vIO32WriteFldAlign(DDRPHY_CA_CMD6, 0x1, CA_CMD6_RG_RX_ARCMD_BIAS_PS);
    vIO32WriteFldAlign(DDRPHY_CA_CMD6, 0x1, CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN);
    vIO32WriteFldAlign(DDRPHY_B0_DQ6, 0x0, B0_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B0);
    vIO32WriteFldAlign(DDRPHY_CA_CMD6, 0x0, CA_CMD6_RG_TX_ARCMD_ODTEN_EXT_DIS);
    vIO32WriteFldAlign(DDRPHY_B0_DQ6, 0x1, B0_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B0);
    vIO32WriteFldAlign(DDRPHY_CA_CMD6, 0x0, CA_CMD6_RG_RX_ARCMD_RPRE_TOG_EN);
    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ5, 0xb, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);
    vIO32WriteFldAlign(DDRPHY_B0_DQ5, 0xb, B0_DQ5_RG_RX_ARDQ_EYE_VREF_SEL_B0);

    vIO32WriteFldMulti(DDRPHY_B0_DQ8, P_Fld(0x0, B0_DQ8_RG_TX_ARDQ_EN_LP4P_B0)
                       | P_Fld(0x0, B0_DQ8_RG_TX_ARDQ_EN_CAP_LP4P_B0)
                       | P_Fld(0x0, B0_DQ8_RG_TX_ARDQ_CAP_DET_B0));

    vIO32WriteFldMulti(DDRPHY_CA_CMD9, P_Fld(0x0, CA_CMD9_RG_TX_ARCMD_EN_LP4P)
                       | P_Fld(0x0, CA_CMD9_RG_TX_ARCMD_EN_CAP_LP4P)
                       | P_Fld(0x0, CA_CMD9_RG_TX_ARCMD_CAP_DET)
                       | P_Fld(0x1, CA_CMD9_RG_ARDLL_RESETB_CA));

    /* BIAS_VREF_SEL is used as switch for old, new burst modes */
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ6), 2, B0_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B0);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), 1, B0_DQ9_PSRAM_RG_RX_ARDQS0_DQSIENMODE_B0_PSRAM);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), 1, B0_DQ9_RG_RX_ARDQS0_DQSIENMODE_B0);

    /* Perform reset (makes sure PHY's behavior works as the above setting) */
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), P_Fld(0, B0_DQ9_PSRAM_RG_RX_ARDQS0_STBEN_RESETB_B0_PSRAM)
                       | P_Fld(0, B0_DQ9_PSRAM_RG_RX_ARDQ_STBEN_RESETB_B0_PSRAM));
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), P_Fld(0, B0_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B0)
                       | P_Fld(0, B0_DQ9_RG_RX_ARDQ_STBEN_RESETB_B0));

    mcDELAY_US(1);//delay 10ns
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), P_Fld(1, B0_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B0)
                       | P_Fld(1, B0_DQ9_RG_RX_ARDQ_STBEN_RESETB_B0));
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), P_Fld(1, B0_DQ9_PSRAM_RG_RX_ARDQS0_STBEN_RESETB_B0_PSRAM)
                       | P_Fld(1, B0_DQ9_PSRAM_RG_RX_ARDQ_STBEN_RESETB_B0_PSRAM));
    vIO32WriteFldAlign(DDRPHY_CA_CMD8, 0x1, CA_CMD8_RG_TX_RRESETB_DDR3_SEL);
    vIO32WriteFldAlign(DDRPHY_CA_CMD8, 0x0, CA_CMD8_RG_TX_RRESETB_DDR4_SEL); //TODO: Remove if register default value is 0
    //End of DDRPhyTxRxInitialSettings_LP4

    //Update setting for Bianco
    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ5, 0x0, SHU1_B0_DQ5_RG_ARPI_FB_B0);
    vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD5, 0x0, SHU1_CA_CMD5_RG_ARPI_FB_CA);


    //Reserved bits usage, check with PHY owners
    vIO32WriteFldAlign_All(DDRPHY_SHU1_B0_DQ6, 0x0, SHU1_B0_DQ6_RG_ARPI_OFFSET_DQSIEN_B0);
    vIO32WriteFldAlign_All(DDRPHY_SHU1_CA_CMD6, 0x0, SHU1_CA_CMD6_RG_ARPI_OFFSET_CLKIEN);

    //IMP Tracking Init Settings
    //Write (DRAMC _BASE+ 0x219) [31:0] = 32'h80080020//DDR3200 default
    //SHU_IMPCAL1_IMPCAL_CHKCYCLE should > 12.5/MCK, 1:4 mode will disable imp tracking -> don't care

    vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ8_PSRAM, P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRANK_CHG_PIPE_CG_IG_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRANK_PIPE_CG_IG_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_PIPE_CG_IG_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_PIPE_CG_IG_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_SYNC_CG_IG_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMSTBEN_SYNC_CG_IG_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDLY_CG_IG_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_PIPE_CG_IG_B0_PSRAM));

    vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD8_PSRAM, P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMRANK_CHG_PIPE_CG_IG_CA_PSRAM)
                       | P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMRANK_PIPE_CG_IG_CA_PSRAM)
                       | P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMDQSIEN_RDSEL_TOG_PIPE_CG_IG_CA_PSRAM)
                       | P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMDQSIEN_RDSEL_PIPE_CG_IG_CA_PSRAM)
                       | P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMDQSIEN_FLAG_PIPE_CG_IG_CA_PSRAM)
                       | P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMDQSIEN_FLAG_SYNC_CG_IG_CA_PSRAM)
                       | P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMSTBEN_SYNC_CG_IG_CA_PSRAM)
                       | P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMRXDLY_CG_IG_CA_PSRAM)
                       | P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMRXDVS_RDSEL_TOG_PIPE_CG_IG_CA_PSRAM)
                       | P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMRXDVS_RDSEL_PIPE_CG_IG_CA_PSRAM)
                       | P_Fld(0x0, SHU1_CA_CMD8_PSRAM_R_DMRXDVS_UPD_FORCE_EN_CA_PSRAM)
                       | P_Fld(0x7fff, SHU1_CA_CMD8_PSRAM_R_DMRXDVS_UPD_FORCE_CYC_CA_PSRAM));
    vIO32WriteFldMulti(DDRPHY_MISC_CTRL3_PSRAM, P_Fld(0x0, MISC_CTRL3_PSRAM_R_DDRPHY_COMB_CG_IG_PSRAM)
                       | P_Fld(0x0, MISC_CTRL3_PSRAM_R_DDRPHY_RX_PIPE_CG_IG_PSRAM));
    vIO32WriteFldMulti(DDRPHY_MISC_CTRL3, P_Fld(0x0, MISC_CTRL3_R_DDRPHY_COMB_CG_IG)
                       | P_Fld(0x0, MISC_CTRL3_R_DDRPHY_RX_PIPE_CG_IG));
    vIO32WriteFldAlign(DDRPHY_CA_DLL_ARPI2_PSRAM, 0x1, CA_DLL_ARPI2_PSRAM_RG_ARPI_CG_CLKIEN_PSRAM);
    /* Bianco HW design issue: run-time PBYTE (B0, B1) flags will lose it's function and become per-bit -> set to 0 */
    vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_DQM_EN_B0_PSRAM)
                       | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_FLAG_OPT_B0_PSRAM)
                       | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_DQM_FLAGSEL_B0_PSRAM));
    vIO32WriteFldAlign(DDRPHY_PSRAM_EYESCAN, 0x0, PSRAM_EYESCAN_DQSERRCNT_DIS);

#if 1//#ifndef BIANCO_TO_BE_PORTING
    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DLL0, 0x1, SHU1_B0_DLL0_RG_ARPISM_MCK_SEL_B0_SHU);
    vIO32WriteFldAlign(DDRPHY_CA_DLL_ARPI1, 0x1, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA);
#endif /* #if 1//#ifndef BIANCO_TO_BE_PORTING */
    //end _K_

    vIO32WriteFldMulti(DDRPHY_B0_DQ6, P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_OP_BIAS_SW_EN_B0)
                       | P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_BIAS_EN_B0));
    vIO32WriteFldMulti(DDRPHY_CA_CMD6, P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_OP_BIAS_SW_EN)
                       | P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_BIAS_EN));
    vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXRANK_DQS_LAT_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMRXRANK_DQS_EN_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMRXRANK_DQ_LAT_B0_PSRAM)
                       | P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMRXRANK_DQ_EN_B0_PSRAM));

    vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x4, B0_DQ9_PSRAM_R_IN_GATE_EN_LOW_OPT_B0_PSRAM);
    vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x4, B0_DQ9_R_IN_GATE_EN_LOW_OPT_B0);

#if 0
    //Modify for corner IC failed at HQA test XTLV
    vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x7, B0_DQ9_PSRAM_R_IN_GATE_EN_LOW_OPT_B0_PSRAM);
    vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x7, B0_DQ9_R_IN_GATE_EN_LOW_OPT_B0);

    //vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x7, B1_DQ9_R_IN_GATE_EN_LOW_OPT_B1);
#endif /* #if 0 */
    vIO32WriteFldAlign(DDRPHY_CA_CMD10, 0x0, CA_CMD10_R_IN_GATE_EN_LOW_OPT_CA);
    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ8_PSRAM, 0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDLY_CG_IG_B0_PSRAM);

#if 0//ENABLE_TX_WDQS
    mcSHOW_DBG_MSG("Enable WDQS\n");
    //Check reserved bits with PHY integrator
    vIO32WriteFldMulti(DDRPHY_SHU1_B0_DLL1, P_Fld(1, RG_ARDQ_REV_BIT_09_TX_READ_BASE_EN)  | P_Fld(1, RG_ARDQ_REV_BIT_02_TX_READ_BASE_EN_DQSB)
                       | P_Fld(!p->odt_onoff, RG_ARDQ_REV_BIT_08_TX_ODT_DISABLE));
    vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7_PSRAM, 0x1, SHU1_B0_DQ7_PSRAM_R_DMRODTEN_B0_PSRAM);

    //vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ7, 0x1, SHU1_B1_DQ7_R_DMRODTEN_B1);
#if ENABLE_RODT_TRACKING_SAVE_MCK
    SetTxWDQSStatusOnOff(1);
#endif /* #if ENABLE_RODT_TRACKING_SAVE_MCK */

#else /* #if 0//ENABLE_TX_WDQS */
    //Check reserved bits with PHY integrator
    vIO32WriteFldMulti(DDRPHY_SHU1_B0_DLL1, P_Fld(0, RG_ARDQ_REV_BIT_09_TX_READ_BASE_EN)
                       | P_Fld(0, RG_ARDQ_REV_BIT_02_TX_READ_BASE_EN_DQSB)
                       | P_Fld(0, RG_ARDQ_REV_BIT_08_TX_ODT_DISABLE));
#endif /* #if 0//ENABLE_TX_WDQS */
    vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL0, 0x1, MISC_CG_CTRL0_RG_IDLE_SRC_SEL);
    //DE review Bianco
    /* ARPISM_MCK_SEL_B0, B1 set to 1 (Joe): "Due to TX_PICG modify register is set to 1,
     * ARPISM_MCK_SEL_Bx should be 1 to fulfill APHY TX OE spec for low freq (Ex: DDR1600)"
     */
    vIO32WriteFldMulti(DDRPHY_B0_DLL_ARPI1, P_Fld(0x0, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0_REG_OPT)
                       | P_Fld(0x1, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0));
    vIO32WriteFldMulti(DDRPHY_CA_DLL_ARPI1, P_Fld(0x0, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA_REG_OPT)
                       | P_Fld(0x1, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA));

    vIO32WriteFldAlign(DDRPHY_MISC_CTRL0_PSRAM, 0, MISC_CTRL0_PSRAM_R_DMSHU_PHYDCM_FORCEOFF_PSRAM);

    vIO32WriteFldAlign(DDRPHY_MISC_RXDVS2, 1, MISC_RXDVS2_R_DMRXDVS_SHUFFLE_CTRL_CG_IG);

    vIO32WriteFldMulti(DDRPHY_CA_TX_MCK, P_Fld(0x1, CA_TX_MCK_R_DMRESET_FRPHY_OPT)
                       | P_Fld(0xa, CA_TX_MCK_R_DMRESETB_DRVP_FRPHY)
                       | P_Fld(0xa, CA_TX_MCK_R_DMRESETB_DRVN_FRPHY));

    //[Cervino DVT]RX FIFO debug feature, MP setting should enable debug function
    vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_R_DMRXFIFO_STBENCMP_EN_B0_PSRAM);
    vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x0, B0_DQ9_R_DMRXFIFO_STBENCMP_EN_B0);
    vIO32WriteFldMulti(DDRPHY_MISC_CTRL1, P_Fld(0x1, MISC_CTRL1_R_DMDQSIENCG_EN)
                       | P_Fld(0x1, MISC_CTRL1_R_DM_TX_ARCMD_OE)
                       | P_Fld(0x1, MISC_CTRL1_R_DM_TX_ARCLK_OE));

#ifndef LOOPBACK_TEST
    DDRPhyFreqMeter();
#endif /* #ifndef LOOPBACK_TEST */


    ///TODO: DVFS_Enable


#ifndef LOOPBACK_TEST
    DDRPhyFMeter_Init();
#endif /* #ifndef LOOPBACK_TEST */

#if 1 // 20190625 move release dmsus here
    PsramReleaseDMSUS(p);
#endif /* #if 1 // 20190625 move release dmsus here */

    // DVFSSettings(p);

    vIO32WriteFldAlign_All(DDRPHY_DVFS_EMI_CLK, 1, DVFS_EMI_CLK_RG_52M_104M_SEL); //Set DVFS_SM's clk

    vIO32WriteFldAlign_All(DDRPHY_DVFS_EMI_CLK, 0x0, DVFS_EMI_CLK_RG_DLL_SHUFFLE);

    //lzs add here for PSRAMC init setting
#if 1// lzs move CK = CK +0.5UI, CA = CA +0.5UI
    vIO32WriteFldAlign(DDRPHY_SHU1_R0_CA_CMD9, 0x10, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9), 0x10, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD);
#endif /* #if 1// lzs move CK = CK +0.5UI, CA = CA +0.5UI */

    //TX init setting follow simulation result
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), 0x10, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0); //CA/DQ move 0.5UI to edge align CK
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), 0x10, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0); //

    vIO32WriteFldAlign(PSRAMC_REG_SHU_SELPH_CA1, 0x2, SHU_SELPH_CA1_TXDLY_CS);

    PSramTXSetDelayReg_CA(p, TRUE, 17, &u2Tmp); // CA setting (2, 1), CA OEN (1, 6)

    if (p->frequency == 800) {
        vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_DQS0, P_Fld(0x1, SHU_SELPH_DQS0_TXDLY_OEN_DQS0)
                           | P_Fld(0x3, SHU_SELPH_DQS0_TXDLY_DQS0));

        PSramTXSetDelayReg_DQ(p, TRUE, 3, 2, 3, 3, 16); //DQ (3, 3, 16), DQ OEN (2, 3)
        PSramTXSetDelayReg_DQM(p, TRUE, 3, 2, 3, 3, 16); //DQM (3, 3, 16), DQM OEN (2, 3)

    } else { //2133
        vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_DQS0, P_Fld(0x2, SHU_SELPH_DQS0_TXDLY_OEN_DQS0)
                           | P_Fld(0x4, SHU_SELPH_DQS0_TXDLY_DQS0));

        PSramTXSetDelayReg_DQ(p, TRUE, 4, 3, 3, 3, 16); //DQ (4, 3, 16), DQ OEN (3, 3)
        PSramTXSetDelayReg_DQM(p, TRUE, 4, 3, 3, 3, 16); //DQM (4, 3, 16), DQM OEN (3, 3)

    }

#ifdef PSRAM_MODEL_32MB
    vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_DQS1, P_Fld(0x3, SHU_SELPH_DQS1_DLY_OEN_DQS0)
                       | P_Fld(0x5, SHU_SELPH_DQS1_DLY_DQS0));
#else /* #ifdef PSRAM_MODEL_32MB */
    vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_DQS1, P_Fld(0x3, SHU_SELPH_DQS1_DLY_OEN_DQS0)
                       | P_Fld(0x3, SHU_SELPH_DQS1_DLY_DQS0));
#endif /* #ifdef PSRAM_MODEL_32MB */

#if SUPPORT_PSRAM_256M
    vIO32WriteFldAlign(PSRAMC_REG_DDRCOMMON0, 0x1, DDRCOMMON0_PSRAM_256M_EN);
#endif /* #if SUPPORT_PSRAM_256M */

    //gating and datlat setting follow simulation result
    vIO32WriteFldAlign(DDRPHY_PSRAM_DQSIEN_CTRL0, 0x07, PSRAM_DQSIEN_CTRL0_DQSINCTL_RK0);
    if (p->frequency == 800) { //1600 setting
        vIO32WriteFldMulti(DDRPHY_PSRAM_DQSIEN_DLY_CTRL0, P_Fld(0x8, PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_UI_RK0_B0)
                           | P_Fld(0xc, PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_PI_RK0_B0));

        vIO32WriteFldMulti(DDRPHY_PSRAM_RDAT, P_Fld(0xe, PSRAM_RDAT_DATLAT)
                           | P_Fld(0xd, PSRAM_RDAT_DATLAT_DSEL)
                           | P_Fld(0xd, PSRAM_RDAT_DATLAT_DSEL_PHY));
    } else { //follow 2133 simulation result
        vIO32WriteFldMulti(DDRPHY_PSRAM_DQSIEN_DLY_CTRL0, P_Fld(0x1a, PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_UI_RK0_B0)
                           | P_Fld(0x10, PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_PI_RK0_B0));

        vIO32WriteFldMulti(DDRPHY_PSRAM_RDAT, P_Fld(0x10, PSRAM_RDAT_DATLAT)
                           | P_Fld(0xf, PSRAM_RDAT_DATLAT_DSEL)
                           | P_Fld(0xf, PSRAM_RDAT_DATLAT_DSEL_PHY));
    }

    //RX setting follow simulation
    //DQ delay
    vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ2, P_Fld(0x0, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0));
    vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ3, P_Fld(0x0, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0));
    vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ4, P_Fld(0x0, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0));
    vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ5, P_Fld(0x0, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0)
                       | P_Fld(0x0, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0));
    //DQS and DQM delay
    if (p->frequency == 800) { //1600 setting
        vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ6, P_Fld(0x14, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0)
                           | P_Fld(0x14, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0)
                           | P_Fld(0x0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0)
                           | P_Fld(0x0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
    } else { //2133 setting
        vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ6, P_Fld(0xb, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0)
                           | P_Fld(0xb, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0)
                           | P_Fld(0x0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0)
                           | P_Fld(0x0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
    }


#if 0
    vIO32WriteFldMulti(PSRAMC_REG_DRAMC_PD_CTRL, P_Fld(0x1, PSAMC_PD_CTRL_PHYCLKDYNGEN)   //when 2133 should change this value
                       | P_Fld(0x04, PSAMC_PD_CTRL_APHYPI_CKCGH_CNT)  //when 2133 should change this value
                       | P_Fld(0x1, PSRAMC_PD_CTRL_DCMEN)
                       | P_Fld(0x0, PSAMC_PD_CTRL_DCMEN2));
#else /* #if 0 */
    vIO32WriteFldMulti(PSRAMC_REG_DRAMC_PD_CTRL, P_Fld(0x1, PSAMC_PD_CTRL_PHYCLKDYNGEN)
                       | P_Fld(0x0, PSAMC_PD_CTRL_APHYCKCG_FIXOFF)
                       | P_Fld(0x0, PSAMC_PD_CTRL_MIOCKCTRLOFF));
#endif /* #if 0 */


    vIO32WriteFldAlign(PSRAMC_REG_SHU_DCM_CTRL0, 0x01, SHU_DCM_CTRL0_DDRPHY_CLK_EN_OPT);

    vIO32WriteFldAlign(PSRAMC_REG_DLLFRZ_CTRL, 0x1, DLLFRZ_CTRL_DLLFRZ);
    vIO32WriteFldAlign(PSRAMC_REG_CE_CTRL, 0x3f, CE_CTRL_WRCS_NOT_MASK);

    vIO32WriteFldAlign(PSRAMC_REG_TX_CG_SET0, 0x1, TX_CG_SET0_SELPH_CMD_CG_DIS);
    vIO32WriteFldAlign(PSRAMC_REG_RX_CG_SET0, 0x01, RX_CG_SET0_RDYCKAR);
    vIO32WriteFldAlign(PSRAMC_REG_CLKAR, 0x1, CLKAR_REQQUECLKRUN);
    vIO32WriteFldAlign(PSRAMC_REG_CLKAR, 0x0, CLKAR_DCMREF_OPT);

    vIO32WriteFldAlign(PSRAMC_REG_REFCTRL0, 0x1, REFCTRL0_REFDIS);
    vIO32WriteFldAlign(PSRAMC_REG_REFCTRL1, 0x1, REFCTRL1_REFPEND_OPT1);
#if SUPPORT_PSRAM_256M
    vIO32WriteFldAlign(PSRAMC_REG_SHU_MATYPE, 0x2, SHU_MATYPE_MATYPE);
#else /* #if SUPPORT_PSRAM_256M */
    vIO32WriteFldAlign(PSRAMC_REG_SHU_MATYPE, 0x3, SHU_MATYPE_MATYPE);
#endif /* #if SUPPORT_PSRAM_256M */
    vIO32WriteFldMulti(PSRAMC_REG_SHU_DCM_CTRL0, P_Fld(0x1, SHU_DCM_CTRL0_FASTWAKE2)
                       | P_Fld(0x1, SHU_DCM_CTRL0_DDRPHY_CLK_EN_OPT));

    vIO32WriteFldMulti(PSRAMC_REG_SHU_RX_CG_SET0, P_Fld(0x1, SHU_RX_CG_SET0_DLE_LAST_EXTEND1)
                       | P_Fld(0x1, SHU_RX_CG_SET0_READ_START_EXTEND2)
                       | P_Fld(0x1, SHU_RX_CG_SET0_DLE_LAST_EXTEND2)
                       | P_Fld(0x1, SHU_RX_CG_SET0_READ_START_EXTEND3)
                       | P_Fld(0x1, SHU_RX_CG_SET0_READ_START_EXTEND1)
                       | P_Fld(0x1, SHU_RX_CG_SET0_DLE_LAST_EXTEND3));

    vIO32WriteFldMulti(PSRAMC_REG_DUMMY_RD, P_Fld(0, DUMMY_RD_DMYRD_HPRI_DIS)
                       | P_Fld(1, DUMMY_RD_DUMMY_RD_SW)
                       | P_Fld(1, DUMMY_RD_DUMMY_RD_PA_OPT)
                       | P_Fld(0, DUMMY_RD_DMY_RD_RX_TRACK)
                       | P_Fld(1, DUMMY_RD_RANK_NUM));
    vIO32WriteFldMulti(PSRAMC_REG_DUMMY_RD_INTV, P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_CNT6)
                       | P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_CNT5)
                       | P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_CNT3)
                       | P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_1_CNT4)
                       | P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_1_CNT3)
                       | P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_1_CNT1));

    //AC Timing setting
    if (p->frequency == 800)
        PsramUpdateACTimingReg(p, &ACTimingPsramTbl[0]);
    else //2133
        PsramUpdateACTimingReg(p, &ACTimingPsramTbl[1]);

#if 0
    PsramReleaseDMSUS(p);
#endif /* #if 0 */
    mcDELAY_US(1);

#if 1//DV_SIMULATION_SW_IMPED
    PsramcSwImpedanceCal(p, 1, 0); //without term
#endif /* #if 1//DV_SIMULATION_SW_IMPED */

    //Clk free run
#if (SW_CHANGE_FOR_SIMULATION == 0)
    EnablePsramcPhyDCM(p, 0);
#endif /* #if (SW_CHANGE_FOR_SIMULATION == 0) */

#if ENABLE_8PHASE_CALIBRATION
    Psramc8PhaseCal(p);   //haohao sun,for bring up
#endif /* #if ENABLE_8PHASE_CALIBRATION */
    //PsramcNewDutyCalibration(p); //haohao sun,for bring up

    PSRAMCPowerOn(p);
    PSRAMCModeRegInit(p);
    // TestPSramEngineCompare(p);
    // PsramDataWindowTest(p);
    vPSRAM_AutoRefreshSwitch(p, ENABLE);
#if (DUMP_INIT_RG_LOG_TO_DE==1)
    DUMP_INIT_RG_ONOFF(0, "//PSramcInitSetting End\n");
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

}

#endif /* #if SUPPORT_TYPE_PSRAM */
