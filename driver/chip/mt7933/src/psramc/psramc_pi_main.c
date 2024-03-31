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
 * $RCSfile: pi_main.c,v $
 * $Revision: #1 $
 *
 *---------------------------------------------------------------------------*/

/** @file pi_main.c
 *  Basic PSRAMC API implementation
 */

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
//#include "..\Common\pd_common.h"
//#include "Register.h"
//=============================================================================
//  Include Files
//=============================================================================
#include <stdio.h>
#include "dramc_pi_api.h"
#if (FOR_DV_SIMULATION_USED==0 && SW_CHANGE_FOR_SIMULATION==0)
#if __ETT__
#include <common.h>
#include <ett_common.h>
#include <api.h>
#endif /* #if __ETT__ */
#endif /* #if (FOR_DV_SIMULATION_USED==0 && SW_CHANGE_FOR_SIMULATION==0) */

#if 0//SUPPORT_TYPE_PSRAM
#define DV_SIMULATION_INIT_C    1
#define DV_SIMULATION_BEFORE_K  1
#define DV_SIMULATION_MIOCKJMETER  1
#define DV_SIMULATION_SW_IMPED 0
#define DV_SIMULATION_LP4_ZQ 1
#define DV_SIMULATION_CA_TRAINING 0
#define DV_SIMULATION_WRITE_LEVELING  0
#define DV_SIMULATION_GATING 1
#define DV_SIMULATION_DATLAT 1
#define DV_SIMULATION_RX_PERBIT    1
#define DV_SIMULATION_TX_PERBIT    1// Please enable with write leveling
#define DV_SIMULATION_AFTER_K   1
#define DV_SIMULATION_DBI_ON   0
#define DV_SIMULATION_RUNTIME_CONFIG 0
#define DV_SIMULATION_RUN_TIME_MRW 0
//U8 gu1BroadcastIsLP4 = TRUE;
#endif /* #if 0//SUPPORT_TYPE_PSRAM */

#if (FOR_DV_SIMULATION_USED==0)
#include "emi.h"
#endif /* #if (FOR_DV_SIMULATION_USED==0) */
#include "dramc_common.h"
#include "dramc_pi_api.h"
#include "x_hal_io.h"
#if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0)
#ifndef MT6761_FPGA
//#include <pmic.h>
#endif /* #ifndef MT6761_FPGA */
#endif /* #if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0) */

#if ! __ETT__
#if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0)
//#include "pmic.h"
#endif /* #if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0) */
#endif /* #if ! __ETT__ */

DRAMC_CTX_T *psCurrDramCtx;
#if defined(DDR_INIT_TIME_PROFILING) || (__ETT__ && SUPPORT_SAVE_TIME_FOR_CALIBRATION)
DRAMC_CTX_T gTimeProfilingDramCtx;
U8 gtime_profiling_flag = 0;
#endif /* #if defined(DDR_INIT_TIME_PROFILING) || (__ETT__ && SUPPORT_SAVE_TIME_FOR_CALIBRATION) */


#if PSRAM_SPEC
DRAMC_CTX_T DramCtx_pSRAM = {
    CHANNEL_SINGLE, // Channel number
    CHANNEL_A,          // DRAM_CHANNEL
    RANK_SINGLE,        //DRAM_RANK_NUMBER_T
    RANK_0,               //DRAM_RANK_T
    PSRAM_2133,
    DRAM_DFS_SHUFFLE_1,
    TYPE_PSRAM,        // DRAM_DRAM_TYPE_T
    FSP_0,  ////  no use
    ODT_OFF,    //no use
    {CBT_NORMAL_MODE}, // no use
    {DBI_OFF, DBI_OFF}, // no use
    {DBI_OFF, DBI_OFF}, // no use
    DATA_WIDTH_8BIT,     // DRAM_DATA_WIDTH_T
    DEFAULT_TEST2_1_CAL,    // test2_1;
    DEFAULT_TEST2_2_CAL,    // test2_2;
    TEST_AUDIO_PATTERN,     // test_pattern;
    1600,                  // frequency
    800,                  // freqGroup
    0xaa, //vendor_id initial value
    REVISION_ID_MAGIC, //revision id
    0xff, //density
    {0},       // no use
    0,  // no use, ucnum_dlycell_perT;
    0,  // no use, u2DelayCellTimex100;
    DISABLE,   // enable_cbt_scan_vref;
#if EYESCAN_LOG
    ENABLE,  // enable_rx_scan_vref;
    ENABLE,   // enable_tx_scan_vref;
#else /* #if EYESCAN_LOG */
    DISABLE,  // disable_rx_scan_vref;
    DISABLE,   // disable_tx_scan_vref;
#endif /* #if EYESCAN_LOG */
#if PRINT_CALIBRATION_SUMMARY
#if (fcFOR_CHIP_ID == fcMockingbird) //tg removed RANK1
    {{0},},
    {{0},},
#else /* #if (fcFOR_CHIP_ID == fcMockingbird) //tg removed RANK1 */
    //aru4CalResultFlag[CHANNEL_NUM][RANK_MAX]
    {{0}, {0},},
    //aru4CalExecuteFlag[CHANNEL_NUM][RANK_MAX]
    {{0}, {0},},
#endif /* #if (fcFOR_CHIP_ID == fcMockingbird) //tg removed RANK1 */
#endif /* #if PRINT_CALIBRATION_SUMMARY */

    {{0},},  ///no use
    {{FALSE},}, // no use

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
    FALSE, //femmc_Ready
    0,
    0,
    0,
    &SavetimeData,
#endif /* #if SUPPORT_SAVE_TIME_FOR_CALIBRATION */
#if (fcFOR_CHIP_ID == fcMockingbird)
    0,  //bDLP3
#endif /* #if (fcFOR_CHIP_ID == fcMockingbird) */

};

BOOL ucIsPSRAM = FALSE;
U8 gfirst_init_flag = 0;
#endif /* #if PSRAM_SPEC */

extern int uhs_psram_k_result;

#if SUPPORT_TYPE_PSRAM
extern void pemi_init(DRAMC_CTX_T *p);
#endif /* #if SUPPORT_TYPE_PSRAM */

#if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0)
extern void EMI_Init2(void);
#endif /* #if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0) */

void vSetVcoreByFreq(DRAMC_CTX_T *p)
{
#ifndef MT6761_FPGA
#if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0)
#if __FLASH_TOOL_DA__
    dramc_set_vcore_voltage(700000);
#else /* #if __FLASH_TOOL_DA__ */
    unsigned int vcore, vdram, vddq;

    vcore = vdram = vddq = 0;

#if defined(DRAM_HQA) && defined(__ETT__)
    hqa_set_voltage_by_freq(p, &vio18, &vcore, &vdram, &vddq);
#else /* #if defined(DRAM_HQA) && defined(__ETT__) */
#ifdef DRAM_HQA
    vio18 = HQA_VIO18;
#endif /* #ifdef DRAM_HQA */

    if (0/*u1IsLP4Family(p->dram_type)*/) {
        if (p->frequency >= 1600)
#ifdef VCORE_BIN
            vcore = get_vcore_uv_table(0);
#else /* #ifdef VCORE_BIN */
            vcore = (SEL_PREFIX_VCORE(LP4, KOPP0) + SEL_PREFIX_VCORE(LP4, KOPP1)) >> 1;
#endif /* #ifdef VCORE_BIN */
        else if (p->frequency == 1200)
#ifdef VCORE_BIN
            vcore = (get_vcore_uv_table(0) + get_vcore_uv_table(1)) >> 1;
#else /* #ifdef VCORE_BIN */
            vcore = (SEL_PREFIX_VCORE(LP4, KOPP1) + SEL_PREFIX_VCORE(LP4, KOPP2)) >> 1;
#endif /* #ifdef VCORE_BIN */
        else if (p->frequency == 800)
#ifdef VCORE_BIN
            vcore = (get_vcore_uv_table(1) + get_vcore_uv_table(3)) >> 1;
#else /* #ifdef VCORE_BIN */
            vcore = (SEL_PREFIX_VCORE(LP4, KOPP2) + SEL_PREFIX_VCORE(LP4, KOPP4)) >> 1;
#endif /* #ifdef VCORE_BIN */
        else
            return ;
    } else {
#if ENABLE_LP3_SW
        // for 1866
        if (p->frequency >= 933)
#ifdef VCORE_BIN
            vcore = get_vcore_uv_table(0);
#else /* #ifdef VCORE_BIN */
            vcore = SEL_PREFIX_VCORE(LP3, KOPP0);
#endif /* #ifdef VCORE_BIN */
        else if (p->frequency == 800) // for 1600
#ifdef VCORE_BIN
            vcore = (get_vcore_uv_table(0) + get_vcore_uv_table(1)) >> 1;
#else /* #ifdef VCORE_BIN */
            vcore = (SEL_PREFIX_VCORE(LP3, KOPP0) + SEL_PREFIX_VCORE(LP3, KOPP2)) >> 1;
#endif /* #ifdef VCORE_BIN */
        else if (p->frequency == 667) // for 1333
#ifdef VCORE_BIN
            vcore = get_vcore_uv_table(1);
#else /* #ifdef VCORE_BIN */
            vcore = SEL_PREFIX_VCORE(LP3, KOPP2);
#endif /* #ifdef VCORE_BIN */
        else if (p->frequency == 600) // for 1200
#ifdef VCORE_BIN
            vcore = (get_vcore_uv_table(1) + get_vcore_uv_table(3)) >> 1;
#else /* #ifdef VCORE_BIN */
            vcore = (SEL_PREFIX_VCORE(LP3, KOPP2) + SEL_PREFIX_VCORE(LP3, KOPP4)) >> 1;
#endif /* #ifdef VCORE_BIN */
        else
            return ;
#endif /* #if ENABLE_LP3_SW */
    }
#endif /* #if defined(DRAM_HQA) && defined(__ETT__) */

#ifdef LAST_DRAMC
    update_last_dramc_k_voltage(p, vcore);
#endif /* #ifdef LAST_DRAMC */

#if defined(DRAM_HQA)
    if (vio18)
        dramc_set_vdd1_voltage(p->dram_type, vio18);
#endif /* #if defined(DRAM_HQA) */

    if (vcore)
        dramc_set_vcore_voltage(vcore);

#if defined(DRAM_HQA)
    if (vdram)
        dramc_set_vdd2_voltage(p->dram_type, vdram);

    if (0/*u1IsLP4Family(p->dram_type)*/) {
        if (vddq)
            dramc_set_vddq_voltage(p->dram_type, vddq);
    }
#endif /* #if defined(DRAM_HQA) */

#ifndef DDR_INIT_TIME_PROFILING
    mcSHOW_DBG_MSG("Read voltage for %d\n", p->frequency);
    //mcSHOW_DBG_MSG("Vio18 = %d\n", dramc_get_vdd1_voltage());
    //mcSHOW_DBG_MSG("Vcore = %d\n", dramc_get_vcore_voltage());
    //mcSHOW_DBG_MSG("Vdram = %d\n", dramc_get_vdd2_voltage(p->dram_type));

    if (0/*u1IsLP4Family(p->dram_type)*/)
        mcSHOW_DBG_MSG("Vddq = %d\n", dramc_get_vddq_voltage(p->dram_type));
#endif /* #ifndef DDR_INIT_TIME_PROFILING */
#endif /* #if __FLASH_TOOL_DA__ */
#endif /* #if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0) */
#endif /* #ifndef MT6761_FPGA */
}

#if CPU_RW_TEST_AFTER_K
void mem_test_address_calculation(DRAMC_CTX_T *p, U32 uiSrcAddr, U32 *pu4Dest)
{
    //U32 u4RankSize;

    *pu4Dest = uiSrcAddr + p->ranksize[RANK_0];
}

void vDramCPUReadWriteTestAfterCalibration(DRAMC_CTX_T *p)
{
    //U8 u1DumpInfo=0;
    U8 u1RankIdx;
    U32 uiLen, count;
    U32 uiRankdAddr[RANK_MAX], uiFixedAddr;
    U32 pass_count, err_count;
    uiLen = 0xffff;

    uiRankdAddr[0] = DDR_BASE;
    mem_test_address_calculation(p, DDR_BASE, &uiRankdAddr[1]);

    for (u1RankIdx = 0; u1RankIdx < p->support_rank_num; u1RankIdx++) {
        //u1DumpInfo = 0;
        err_count = 0;
        pass_count = 0;
#if !__ETT__
        // scy: not to test rank1 (wrong addr 0x0000_0000)
        if (u1RankIdx >= 1)
            continue;
#endif /* #if !__ETT__ */

        uiFixedAddr = uiRankdAddr[u1RankIdx];

        for (count = 0; count < uiLen; count += 4) {
            *(volatile unsigned int *)(count + uiFixedAddr) = count + (0x5a5a << 16);
        }

        for (count = 0; count < uiLen; count += 4) {
            if (*(volatile unsigned int *)(count + uiFixedAddr) != count + (0x5a5a << 16)) {
                //mcSHOW_DBG_MSG("[Fail] Addr %xh = %xh\n",count, *(volatile unsigned int   *)(count));
                err_count++;
            } else {
                pass_count++;
            }
        }

        if (err_count) {
            mcSHOW_DBG_MSG("[MEM_TEST] Rank %d Fail.", u1RankIdx);
            //u1DumpInfo =1;
#if defined(SLT)
            while (1);
#endif /* #if defined(SLT) */
        } else {
            mcSHOW_DBG_MSG("[MEM_TEST] Rank %d OK.", u1RankIdx);
        }
        mcSHOW_DBG_MSG("(uiFixedAddr 0x%X, Pass count =%d, Fail count =%d)\n", uiFixedAddr, pass_count, err_count);

        if (err_count != 0) {
            uhs_psram_k_result = -1;
            mcSHOW_DBG_MSG("PSRAM MEM_TEST FAIL\n");
        }
    }
}
#endif /* #if CPU_RW_TEST_AFTER_K */

U8 gGet_MDL_Used_Flag = GET_MDL_USED; // tg change 0 -> 1 to test clk duty
void Set_MDL_Used_Flag(U8 value)
{
    gGet_MDL_Used_Flag = value;
}

U8 Get_MDL_Used_Flag(void)
{
    return gGet_MDL_Used_Flag;
}

U8 gPRE_MIOCK_JMETER_HQA_USED_flag = 0;
void Set_PRE_MIOCK_JMETER_HQA_USED_flag(U8 value)
{
    gPRE_MIOCK_JMETER_HQA_USED_flag = value;
}
U8 Get_PRE_MIOCK_JMETER_HQA_USED_flag(void)
{
    return gPRE_MIOCK_JMETER_HQA_USED_flag;
}

#if ENABLE_MIOCK_JMETER
void PRE_MIOCK_JMETER_HQA_USED(DRAMC_CTX_T *p)
{
    U32 backup_freq_sel, backup_channel;

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
    U32 channel_idx, shuffleIdx;
    if (p->femmc_Ready == 1) {
        for (channel_idx = CHANNEL_A; channel_idx < p->support_channel_num; channel_idx++) {
            ucg_num_dlycell_perT_all[p->shu_type][channel_idx] = p->pSavetimeData->ucnum_dlycell_perT;
            u2gdelay_cell_ps_all[p->shu_type][channel_idx] = p->pSavetimeData->u2DelayCellTimex100;
        }

        p->ucnum_dlycell_perT = p->pSavetimeData->ucnum_dlycell_perT;
        p->u2DelayCellTimex100 = p->pSavetimeData->u2DelayCellTimex100;
        return;
    }
#endif /* #if SUPPORT_SAVE_TIME_FOR_CALIBRATION */

    backup_freq_sel = p->freq_sel;
    backup_channel = p->channel;

    mcSHOW_DBG_MSG3("[JMETER_HQA]\n");
    Set_PRE_MIOCK_JMETER_HQA_USED_flag(1);

    vSetPHY2ChannelMapping(p, CHANNEL_A);
#if PSRAM_SPEC
    PSramcMiockJmeter(p);
#endif /* #if PSRAM_SPEC */

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
    if (p->femmc_Ready == 0) {
        p->pSavetimeData->ucnum_dlycell_perT = p->ucnum_dlycell_perT;
        p->pSavetimeData->u2DelayCellTimex100 = p->u2DelayCellTimex100;
    }
#endif /* #if SUPPORT_SAVE_TIME_FOR_CALIBRATION */
    vSetPHY2ChannelMapping(p, backup_channel);

    Set_PRE_MIOCK_JMETER_HQA_USED_flag(0);

    p->freq_sel = backup_freq_sel;
}
#endif /* #if ENABLE_MIOCK_JMETER */

#if PSRAM_SPEC
void vCalibration_Flow_PSRAM(DRAMC_CTX_T *p)
{
#if ENABLE_RX_TRACKING_LP4
    PsramRxInputTrackingSetting(p);
#endif /* #if ENABLE_RX_TRACKING_LP4 */

    PSramcRxdqsGatingCal(p);

#if PINMUX_AUTO_TEST_PER_BIT_RX
    CheckPsramRxPinMux(p);
#endif /* #if PINMUX_AUTO_TEST_PER_BIT_RX */

    PSramcRxWindowPerbitCal(p, 1);

    PSramcTxWindowPerbitCal(p, TX_DQ_DQS_MOVE_DQ_DQM, FALSE);

#if PINMUX_AUTO_TEST_PER_BIT_TX
    CheckPsramTxPinMux(p);
#endif /* #if PINMUX_AUTO_TEST_PER_BIT_TX */

    PSramcTxWindowPerbitCal(p, TX_DQ_DQS_MOVE_DQ_ONLY, p->enable_tx_scan_vref);
    PSramcRxdatlatScan(p, fcDATLAT_USE_DEFAULT);

#if DV_SIMULATION_RUN_TIME_MRW
    enter_pasr_dpd_config(0, 0xFF);
#endif /* #if DV_SIMULATION_RUN_TIME_MRW */

}


int Init_DRAM(DRAM_DRAM_TYPE_T dram_type, DRAM_CBT_MODE_EXTERN_T dram_cbt_mode_extern, DRAM_INFO_BY_MRR_T *DramInfo, U8 get_mdl_used)
{
#if !SW_CHANGE_FOR_SIMULATION

    //int mem_start,len, s4value;
    DRAMC_CTX_T *p;
    //U8 ucstatus = 0;
    //U32 u4value;
    //U8 chIdx;
#if defined(SLT)
    U32 u1backup_vcore;
#endif /* #if defined(SLT) */

#ifdef DDR_INIT_TIME_PROFILING
    U32 CPU_Cycle;
    TimeProfileBegin();
#endif /* #ifdef DDR_INIT_TIME_PROFILING */

    psCurrDramCtx = &DramCtx_pSRAM;
    ucIsPSRAM = TRUE;

#if defined(DDR_INIT_TIME_PROFILING) || (__ETT__ && SUPPORT_SAVE_TIME_FOR_CALIBRATION)
    if (gtime_profiling_flag == 0) {
        memcpy(&gTimeProfilingDramCtx, psCurrDramCtx, sizeof(DRAMC_CTX_T));
        gtime_profiling_flag = 1;
    }

    p = &gTimeProfilingDramCtx;
    gfirst_init_flag = 0;
#else /* #if defined(DDR_INIT_TIME_PROFILING) || (__ETT__ && SUPPORT_SAVE_TIME_FOR_CALIBRATION) */
    p = psCurrDramCtx;
#endif /* #if defined(DDR_INIT_TIME_PROFILING) || (__ETT__ && SUPPORT_SAVE_TIME_FOR_CALIBRATION) */

    Set_MDL_Used_Flag(get_mdl_used);

    p->dram_type = dram_type;

    /* Convert DRAM_CBT_MODE_EXTERN_T to DRAM_CBT_MODE_T */
    switch ((int)dram_cbt_mode_extern) {
        case CBT_R0_R1_NORMAL:
            p->dram_cbt_mode[RANK_0] = CBT_NORMAL_MODE;
#if (fcFOR_CHIP_ID != fcMockingbird)
            p->dram_cbt_mode[RANK_1] = CBT_NORMAL_MODE;
#endif /* #if (fcFOR_CHIP_ID != fcMockingbird) */
            break;
        case CBT_R0_R1_BYTE:
            p->dram_cbt_mode[RANK_0] = CBT_BYTE_MODE1;
#if (fcFOR_CHIP_ID != fcMockingbird)
            p->dram_cbt_mode[RANK_1] = CBT_BYTE_MODE1;
#endif /* #if (fcFOR_CHIP_ID != fcMockingbird) */
            break;
        case CBT_R0_NORMAL_R1_BYTE:
            p->dram_cbt_mode[RANK_0] = CBT_NORMAL_MODE;
#if (fcFOR_CHIP_ID != fcMockingbird)
            p->dram_cbt_mode[RANK_1] = CBT_BYTE_MODE1;
#endif /* #if (fcFOR_CHIP_ID != fcMockingbird) */
            break;
        case CBT_R0_BYTE_R1_NORMAL:
            p->dram_cbt_mode[RANK_0] = CBT_BYTE_MODE1;
#if (fcFOR_CHIP_ID != fcMockingbird)
            p->dram_cbt_mode[RANK_1] = CBT_NORMAL_MODE;
#endif /* #if (fcFOR_CHIP_ID != fcMockingbird) */
            break;
        default:
            mcSHOW_ERR_MSG("Error!");
            break;
    }
#if (fcFOR_CHIP_ID == fcMockingbird)
    mcSHOW_DBG_MSG2("dram_cbt_mode_extern: %d\n"
                    "dram_cbt_mode [RK0]: %d\n",
                    (int)dram_cbt_mode_extern, p->dram_cbt_mode[RANK_0]);
#else /* #if (fcFOR_CHIP_ID == fcMockingbird) */
    mcSHOW_DBG_MSG2("dram_cbt_mode_extern: %d\n"
                    "dram_cbt_mode [RK0]: %d, [RK1]: %d\n",
                    (int)dram_cbt_mode_extern, p->dram_cbt_mode[RANK_0], p->dram_cbt_mode[RANK_1]);
#endif /* #if (fcFOR_CHIP_ID == fcMockingbird) */

    if (gfirst_init_flag == 0) {
        gfirst_init_flag = 1;
    }

    Psram_Global_Option_Init2(p);

    mcSHOW_DBG_MSG("before emi init\n");
#if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0)
#if SUPPORT_TYPE_PSRAM
    pemi_init(p);
#endif /* #if SUPPORT_TYPE_PSRAM */
#endif /* #if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0) */

#if (fcFOR_CHIP_ID == fcMockingbird) //tg removed RANK1
    mcSHOW_DBG_MSG("\n\n[Bianco] ETT version 0.0.0.1\n dram_type %d, R0 cbt_mode %d, VENDOR=%d\n\n", p->dram_type, p->dram_cbt_mode[RANK_0], p->vendor_id);
#else /* #if (fcFOR_CHIP_ID == fcMockingbird) //tg removed RANK1 */
    mcSHOW_DBG_MSG("\n\n[Bianco] ETT version 0.0.0.1\n dram_type %d, R0 cbt_mode %d, R1 cbt_mode %d VENDOR=%d\n\n", p->dram_type, p->dram_cbt_mode[RANK_0], p->dram_cbt_mode[RANK_1], p->vendor_id);
#endif /* #if (fcFOR_CHIP_ID == fcMockingbird) //tg removed RANK1 */

#ifdef FT_test
    mcSHOW_DBG_MSG("\n\nFT\n\n");
    Psram_FT(p);
#else /* #ifdef FT_test */

#if PSRAM_SPEC
    vPsramcInit_PreSettings(p);
#endif /* #if PSRAM_SPEC */
    //===  First frequency ======
    PsramPhyFreqSel(p, p->freq_sel);

    vSetVcoreByFreq(p);

#ifdef DDR_INIT_TIME_PROFILING
    CPU_Cycle = TimeProfileEnd();
    mcSHOW_TIME_MSG("(0)Pre_Init + SwImdepance takes %d ms\n\r", CPU_Cycle / 1000);
#endif /* #ifdef DDR_INIT_TIME_PROFILING */

#if (DUMP_INIT_RG_LOG_TO_DE==1)
    gDUMP_INIT_RG_LOG_TO_DE_RG_log_flag = 1;
#endif /* #if (DUMP_INIT_RG_LOG_TO_DE==1) */

#if PSRAM_SPEC
    mcSHOW_DBG_MSG("PSramcInitSetting\n");
    PSramcInitSetting(p);
    vApplyPsramConfigBeforeCalibration(p);

#if ENABLE_MIOCK_JMETER
    PRE_MIOCK_JMETER_HQA_USED(p); // jitter meter
#endif /* #if ENABLE_MIOCK_JMETER */
#endif /* #if PSRAM_SPEC */

#if defined(SLT)
    if (Get_MDL_Used_Flag() == NORMAL_USED) {
        u1backup_vcore = dramc_get_vcore_voltage();
#if !__ETT__
        if (seclib_get_devinfo_with_index(19) & 0x1)
            dramc_set_vcore_voltage(737500);
        else
#endif /* #if !__ETT__ */
            dramc_set_vcore_voltage(843750);

        PRE_MIOCK_JMETER_HQA_USED(p);

        //FT_Pattern_For_PI_Glitch_K_Flow(p);

        dramc_set_vcore_voltage(u1backup_vcore);
    }
#endif /* #if defined(SLT) */

#ifdef TEST_MODE_MRS
    if (global_which_test == 0)
        TestModeTestMenu();
#endif /* #ifdef TEST_MODE_MRS */

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
    p->pSavetimeData->support_rank_num = p->support_rank_num;
#endif /* #if SUPPORT_SAVE_TIME_FOR_CALIBRATION */

#if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0)
    //EMI_Init2();
#endif /* #if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0) */

#if PSRAM_SPEC
    vCalibration_Flow_PSRAM(p);
#endif /* #if PSRAM_SPEC */

#ifdef DDR_INIT_TIME_PROFILING
    TimeProfileBegin();
#endif /* #ifdef DDR_INIT_TIME_PROFILING */

#if PSRAM_SPEC
    vApplyPsramConfigAfterCalibration(p);
#endif /* #if PSRAM_SPEC */

#if !LCPLL_IC_SCAN
#if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0)
    //print_DBG_info(p);
    //Dump_EMIRegisters(p);
#endif /* #if (FOR_DV_SIMULATION_USED == 0 && SW_CHANGE_FOR_SIMULATION == 0) */
#endif /* #if !LCPLL_IC_SCAN */

#if (DVT_TEST_DUMMY_RD_SIDEBAND_FROM_SPM && defined(DUMMY_READ_FOR_TRACKING))
    PsramcDummyReadForTrackingEnable(p);
    mcSHOW_DBG_MSG("DUMMY_READ_FOR_TRACKING: ON\n");

    //Disable auto refresh: set R_DMREFDIS=1
    vPSRAM_AutoRefreshSwitch(p, DISABLE);

    while (1) {
        mcSHOW_DBG_MSG("\ndummy read is 1us ===\n");
        DVS_DMY_RD_ENTR(p);
        mcDELAY_MS(5000);
        mcSHOW_DBG_MSG("\ndummy read is 4us ===\n");
        DVS_DMY_RD_EXIT(p);
        mcDELAY_MS(5000);
    }
#endif /* #if (DVT_TEST_DUMMY_RD_SIDEBAND_FROM_SPM && defined(DUMMY_READ_FOR_TRACKING)) */
#endif /* #ifdef FT_test */
    // update emi CONA/CH CONA setting by dram density size
    //DramcUpdateEmiSetting(p);

#if CPU_RW_TEST_AFTER_K
    mcSHOW_DBG_MSG("\n[MEM_TEST] 02: After DFS, before run time config\n");
    vDramCPUReadWriteTestAfterCalibration(p);
#endif /* #if CPU_RW_TEST_AFTER_K */

#if PSRAM_SPEC
    mcSHOW_DBG_MSG("\n[TA2_TEST]\n");
    PSramc_TA2_Test_Run_Time_HW(p);
#endif /* #if PSRAM_SPEC */
#endif /* #if !SW_CHANGE_FOR_SIMULATION */

    // when time profiling multi times, SW impedance tracking will fail when trakcing enable.
    // ignor SW impedance tracking when doing time profling
#if __ETT__
#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
    if (p->femmc_Ready == 0)
#elif defined(DDR_INIT_TIME_PROFILING)
    if (u2TimeProfileCnt == (DDR_INIT_TIME_PROFILING_TEST_CNT - 1)) //last time of loop
#endif /* #if SUPPORT_SAVE_TIME_FOR_CALIBRATION */
#endif /* #if __ETT__ */
    {
        mcSHOW_DBG_MSG("\n\nSettings after calibration\n\n");
        DramcRunTimeConfig(p);
    }

#if CPU_RW_TEST_AFTER_K
    mcSHOW_DBG_MSG("\n[MEM_TEST] 03: After run time config\n");
    vDramCPUReadWriteTestAfterCalibration(p);

#if 0//PSRAM_SPEC
    mcSHOW_DBG_MSG("\n[MEM_TEST]temply while(1)\n");
    DRV_WriteReg32(IO_PHYS + 0x7000, 0x22000000);  // disable WDT
    while (1);
#endif /* #if 0//PSRAM_SPEC */

#endif /* #if CPU_RW_TEST_AFTER_K */

#if TA2_RW_TEST_AFTER_K
    mcSHOW_DBG_MSG("\n[TA2_TEST]\n");
    PSramc_TA2_Test_Run_Time_HW(p);
    vIO32WriteFldAlign(PSRAMC_REG_TEST2_A2, 0x20, TEST2_A2_TEST2_OFF);
#endif /* #if TA2_RW_TEST_AFTER_K */

#if (__ETT__ && CPU_RW_TEST_AFTER_K)
    /* 0x46000000 is LK base addr */
    //while(1)
    {
        if ((s4value = complex_mem_test(0x40024000, 0x20000)) == 0) {
            mcSHOW_DBG_MSG("1st complex R/W mem test pass\n");
        } else {
            mcSHOW_DBG_MSG("1st complex R/W mem test fail :-%d\n", -s4value);
#if defined(SLT)
            while (1);
#endif /* #if defined(SLT) */
        }
    }
#endif /* #if (__ETT__ && CPU_RW_TEST_AFTER_K) */

#ifdef DDR_INIT_TIME_PROFILING
    CPU_Cycle = TimeProfileEnd();
    mcSHOW_TIME_MSG("(5) After calibration takes %d ms\n\r", CPU_Cycle / 1000);
#endif /* #ifdef DDR_INIT_TIME_PROFILING */
#endif /* #if PSRAM_SPEC */

#if DUMP_REG
    mcSHOW_CHECK_RG("Dump register for psram\n");
    dump_ddrphy_reg();
    dump_psramc_reg();
#endif /* #if DUMP_REG */

    return 0;
}

#if (FOR_DV_SIMULATION_USED == 1)
void DPI_SW_main_PSRAM(DRAMC_CTX_T *DramConfig)
{

    DRAM_DRAM_TYPE_T type;

    type = TYPE_PSRAM;

    Init_DRAM(type, CBT_R0_R1_NORMAL, NULL, NORMAL_USED);

}
#endif /* #if (FOR_DV_SIMULATION_USED == 1) */
