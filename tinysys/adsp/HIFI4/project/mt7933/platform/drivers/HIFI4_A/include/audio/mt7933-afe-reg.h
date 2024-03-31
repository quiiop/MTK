/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2018. All rights reserved.
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
* have been modified by MediaTek Inc. All revisions are subject to any receiver\'s
* applicable license agreements with MediaTek Inc.
*/

#ifndef _MT7933_AFE_REG_H_
#define _MT7933_AFE_REG_H_

#include "types.h"

#define AFE_REG_BASE			0x40090000
#define AFE_REG_SIZE			0x3000
#define AFE_INTERNAL_SRAM_PHY_BASE	(0x40080000)
#define AFE_INTERNAL_SRAM_VIR_BASE	(0x40080000)

#define AFE_INTERNAL_SRAM_SIZE		(0x10000)

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/
#define AUDIO_TOP_CON0			0x0000
#define AUDIO_TOP_CON1			0x0004
#define AUDIO_TOP_CON3			0x000c
#define AUDIO_TOP_CON4			0x0010
#define AUDIO_TOP_CON5			0x0014

#define ASMO_TIMING_CON0		0x0100
#define PWR1_ASM_CON1			0x0108

#define ASYS_IRQ_CONFIG                 0x0110
#define ASYS_IRQ1_CON			0x0114
#define ASYS_IRQ2_CON			0x0118
#define ASYS_IRQ3_CON			0x011c
#define ASYS_IRQ4_CON			0x0120
#define ASYS_IRQ5_CON			0x0124
#define ASYS_IRQ6_CON			0x0128
#define ASYS_IRQ7_CON			0x012c
#define ASYS_IRQ8_CON			0x0130
#define ASYS_IRQ9_CON			0x0134
#define ASYS_IRQ10_CON			0x0138
#define ASYS_IRQ11_CON			0x013c
#define ASYS_IRQ12_CON			0x0140
#define ASYS_IRQ13_CON                  0x0144
#define ASYS_IRQ14_CON                  0x0148
#define ASYS_IRQ15_CON                  0x014c
#define ASYS_IRQ16_CON                  0x0150

#define ASYS_IRQ_CLR			0x0154
#define ASYS_IRQ_STATUS			0x0158

#define ASYS_IRQ_MON1                   0x015c
#define ASYS_IRQ_MON2                   0x0160

#define AFE_IRQ_MCU_CLR			0x0170
#define AFE_IRQ_STATUS			0x0174
#define AFE_IRQ_MASK			0x0178
#define ASYS_IRQ_MASK			0x017c

#define AFE_SINEGEN_CON0		0x01f0
#define AFE_SINEGEN_CON1		0x01f4
#define AFE_SINEGEN_CON2		0x01f8
#define AFE_SINEGEN_CON3		0x01fc

#define AFE_TDMOUT_CONN0		0x0390
#define AFE_TDMOUT_CONN1		0x0398
#define AFE_TDMOUT_CONN2		0x039c

#define AFE_APLL_TUNER_CFG              0x03f8
#define AUDIO_TOP_STA0                  0x0400
#define AUDIO_TOP_STA1                  0x0404
#define AFE_GAIN1_CON0                  0x0410
#define AFE_GAIN1_CON1                  0x0414
#define AFE_GAIN1_CON2                  0x0418
#define AFE_GAIN1_CON3                  0x041c
#define AFE_GAIN1_CUR                   0x0424

#define ASYS_TOP_CON			0x0600
#define PWR2_TOP_CON0			0x0634
#define PWR2_TOP_CON1                   0x0638

#define AFE_I2S_UL9_REORDER		0x0660
#define AFE_I2S_UL2_REORDER		0x0664
#define AFE_CONN_24BIT			0x06b8
#define AFE_CONN_16BIT			0x06bc
#define AFE_CONN4                       0x06d0
#define AFE_CONN5                       0x06d4
#define AFE_CONN12                      0x06f0
#define AFE_CONN13                      0x06f4
#define AFE_CONN14                      0x06f8
#define AFE_CONN15                      0x06fc
#define AFE_CONN16                      0x0700
#define AFE_CONN17                      0x0704
#define AFE_CONN18                      0x0708
#define AFE_CONN19                      0x070c
#define AFE_CONN20                      0x0710
#define AFE_CONN21                      0x0714
#define AFE_CONN22                      0x0718
#define AFE_CONN23                      0x071c
#define AFE_CONN26                      0x0728
#define AFE_CONN27                      0x072c
#define AFE_CONN28                      0x0730
#define AFE_CONN29                      0x0734
#define AFE_CONN30                      0x0738
#define AFE_CONN31                      0x073c
#define AFE_CONN32                      0x0740
#define AFE_CONN33                      0x0744
#define AFE_CONN34                      0x0748
#define AFE_CONN35                      0x074c
#define AFE_CONN36                      0x0750
#define AFE_CONN37                      0x0754
#define AFE_CONN38                      0x0758
#define AFE_CONN39                      0x075c
#define AFE_CONN40                      0x0760
#define AFE_CONN41                      0x0764
#define AFE_CONN42                      0x0768
#define AFE_CONN43                      0x076c
#define AFE_CONN4_1                     0x07d4
#define AFE_CONN5_1                     0x07d8
#define AFE_CONN12_1                    0x07f4
#define AFE_CONN13_1                    0x07f8
#define AFE_CONN14_1                    0x07fc
#define AFE_CONN15_1                    0x0800
#define AFE_CONN16_1                    0x0804
#define AFE_CONN17_1                    0x0808
#define AFE_CONN18_1                    0x080c
#define AFE_CONN19_1                    0x0810
#define AFE_CONN20_1                    0x0814
#define AFE_CONN21_1                    0x0818
#define AFE_CONN22_1                    0x081c
#define AFE_CONN23_1                    0x0820
#define AFE_CONN26_1                    0x082c
#define AFE_CONN27_1                    0x0830
#define AFE_CONN28_1                    0x0834
#define AFE_CONN29_1                    0x0838
#define AFE_CONN30_1                    0x083c
#define AFE_CONN31_1                    0x0840
#define AFE_CONN32_1                    0x0844
#define AFE_CONN33_1                    0x0848
#define AFE_CONN34_1                    0x084c
#define AFE_CONN35_1                    0x0850
#define AFE_CONN36_1                    0x0854
#define AFE_CONN37_1                    0x0858
#define AFE_CONN38_1                    0x085c
#define AFE_CONN39_1                    0x0860
#define AFE_CONN40_1                    0x0864
#define AFE_CONN41_1                    0x0868
#define AFE_CONN42_1                    0x086c
#define AFE_CONN43_1                    0x0870
#define AFE_CONN_RS                     0x0900
#define AFE_CONN_RS_1                   0x0904
#define AFE_CONN_16BIT_1                0x0908
#define AFE_CONN_24BIT_1                0x090c

#define AFE_GASRC0_NEW_CON0		0x0b00
#define AFE_GASRC0_NEW_CON1		0x0b04
#define AFE_GASRC0_NEW_CON2		0x0b08
#define AFE_GASRC0_NEW_CON3		0x0b0c
#define AFE_GASRC0_NEW_CON4		0x0b10
#define AFE_GASRC0_NEW_CON6		0x0b18
#define AFE_GASRC0_NEW_CON7		0x0b1c
#define AFE_GASRC0_NEW_CON8		0x0b20
#define AFE_GASRC0_NEW_CON9		0x0b24
#define AFE_GASRC0_NEW_CON10		0x0b28
#define AFE_GASRC0_NEW_CON11		0x0b2c
#define AFE_GASRC0_NEW_CON13		0x0b34
#define AFE_GASRC0_NEW_CON14		0x0b38

#define AFE_LRCK_CNT                    0x1018

#define AFE_DAC_CON0			0x1200
#define AFE_DAC_CON1			0x1204
#define AFE_DAC_MON0                    0x1218

#define AFE_DL2_BASE			0x1250
#define AFE_DL2_CUR			0x1254
#define AFE_DL2_END			0x1258
#define AFE_DL2_CON0			0x125c

#define AFE_DL3_BASE			0x1260
#define AFE_DL3_CUR			0x1264
#define AFE_DL3_END			0x1268
#define AFE_DL3_CON0			0x126c

#define AFE_DL10_BASE			0x12d0
#define AFE_DL10_CUR			0x12d4
#define AFE_DL10_END			0x12d8
#define AFE_DL10_CON0			0x12dc
#define AFE_UL2_BASE			0x1310
#define AFE_UL2_CUR			0x1314
#define AFE_UL2_END			0x1318
#define AFE_UL2_CON0			0x131c

#define AFE_UL3_BASE			0x1320
#define AFE_UL3_CUR			0x1324
#define AFE_UL3_END			0x1328
#define AFE_UL3_CON0			0x132c

#define AFE_UL4_BASE			0x1330
#define AFE_UL4_CUR			0x1334
#define AFE_UL4_END			0x1338
#define AFE_UL4_CON0			0x133c
#define AFE_UL8_BASE			0x1370
#define AFE_UL8_CUR			0x1374
#define AFE_UL8_END			0x1378
#define AFE_UL8_CON0			0x137c

#define AFE_UL9_BASE			0x1380
#define AFE_UL9_CUR			0x1384
#define AFE_UL9_END			0x1388
#define AFE_UL9_CON0			0x138c

#define AFE_UL10_BASE			0x13d0
#define AFE_UL10_CUR			0x13d4
#define AFE_UL10_END			0x13d8
#define AFE_UL10_CON0			0x13dc

#define AFE_DL10_CHK_SUM1               0x1418
#define AFE_DL10_CHK_SUM2               0x141c
#define AFE_DL10_CHK_SUM3               0x1420
#define AFE_DL10_CHK_SUM4               0x1424
#define AFE_DL10_CHK_SUM5               0x1428
#define AFE_DL10_CHK_SUM6               0x142c
#define AFE_UL2_CHK_SUM1                0x1458
#define AFE_UL2_CHK_SUM2                0x145c
#define AFE_UL3_CHK_SUM1                0x1460
#define AFE_UL3_CHK_SUM2                0x1464
#define AFE_UL4_CHK_SUM1                0x1468
#define AFE_UL4_CHK_SUM2                0x146c
#define AFE_UL8_CHK_SUM1                0x1488
#define AFE_UL8_CHK_SUM2                0x148c
#define AFE_DL2_CHK_SUM1                0x14a0
#define AFE_DL2_CHK_SUM2                0x14a4
#define AFE_DL3_CHK_SUM1                0x14b0
#define AFE_DL3_CHK_SUM2                0x14b4
#define AFE_UL9_CHK_SUM1                0x1528
#define AFE_UL9_CHK_SUM2                0x152c
#define AFE_BUS_MON1			0x1540
#define AFE_UL10_CHK_SUM1               0x1550
#define AFE_UL10_CHK_SUM2               0x1554
#define UL2_MOD2AGT_CNT_LAT             0x156c
#define UL3_MOD2AGT_CNT_LAT             0x1570
#define UL4_MOD2AGT_CNT_LAT             0x1574
#define UL8_MOD2AGT_CNT_LAT             0x1588
#define UL9_MOD2AGT_CNT_LAT             0x158c
#define UL10_MOD2AGT_CNT_LAT            0x1590
#define AFE_MEMIF_AGENT_FS_CON0		0x15a0
#define AFE_MEMIF_AGENT_FS_CON1		0x15a4
#define AFE_MEMIF_AGENT_FS_CON2		0x15a8
#define AFE_MEMIF_AGENT_FS_CON3		0x15ac

#define AFE_MEMIF_BURST_CFG             0x1600
#define AFE_MEMIF_BUF_FULL_MON          0x1610
#define AFE_MEMIF_BUF_MON1              0x161c
#define AFE_MEMIF_BUF_MON5              0x162c
#define AFE_MEMIF_BUF_MON6              0x1630
#define AFE_MEMIF_BUF_MON7              0x1634
#define AFE_MEMIF_BUF_MON9              0x163c
#define AFE_MEMIF_BUF_MON10             0x1640
#define DL2_AGENT2MODULE_CNT            0x1678
#define DL3_AGENT2MODULE_CNT            0x167c
#define DL10_AGENT2MODULE_CNT           0x1698
#define UL2_MODULE2AGENT_CNT            0x16a4
#define UL3_MODULE2AGENT_CNT            0x16a8
#define UL4_MODULE2AGENT_CNT            0x16ac
#define UL8_MODULE2AGENT_CNT            0x16bc
#define UL9_MODULE2AGENT_CNT            0x16c0
#define UL10_MODULE2AGENT_CNT           0x16c4
#define AFE_SECURE_CON1                 0x179c
#define AFE_SECURE_CON                  0x17a0
#define AFE_SRAM_BOUND                  0x17a4
#define AFE_SE_SECURE_CON               0x17a8
#define AFE_SECURE_MASK_CONN0           0x17c0
#define AFE_SECURE_MASK_CONN1           0x17c4
#define AFE_SECURE_MASK_CONN2           0x17c8
#define AFE_SECURE_MASK_CONN3           0x17cc
#define AFE_SECURE_MASK_CONN4           0x17d0
#define AFE_SECURE_MASK_CONN5           0x17d4
#define AFE_SECURE_MASK_CONN6           0x17d8
#define AFE_SECURE_MASK_CONN7           0x17dc
#define AFE_SECURE_MASK_CONN8           0x17e0
#define AFE_SECURE_MASK_CONN9           0x17e4
#define AFE_SECURE_MASK_CONN10          0x17e8
#define AFE_SECURE_MASK_CONN11          0x17ec
#define AFE_SECURE_MASK_CONN12          0x17f0
#define AFE_SECURE_MASK_CONN13          0x17f4
#define AFE_SECURE_MASK_CONN14          0x17f8
#define AFE_SECURE_MASK_CONN15          0x17fc
#define AFE_SECURE_MASK_CONN16          0x1800
#define AFE_SECURE_MASK_CONN17          0x1804
#define AFE_SECURE_MASK_CONN18          0x1808
#define AFE_SECURE_MASK_CONN19          0x180c
#define AFE_SECURE_MASK_CONN20          0x1810
#define AFE_SECURE_MASK_CONN21          0x1814
#define AFE_SECURE_MASK_CONN22          0x1818
#define AFE_SECURE_MASK_CONN23          0x181c
#define AFE_SECURE_MASK_CONN24          0x1820
#define AFE_SECURE_MASK_CONN25          0x1824
#define AFE_SECURE_MASK_CONN26          0x1828
#define AFE_SECURE_MASK_CONN27          0x182c
#define AFE_SECURE_MASK_CONN28          0x1830
#define AFE_SECURE_MASK_CONN29          0x1834
#define AFE_SECURE_MASK_CONN30          0x1838
#define AFE_SECURE_MASK_CONN31          0x183c
#define AFE_SECURE_MASK_CONN32          0x1840
#define AFE_SECURE_MASK_CONN33          0x1844
#define AFE_SECURE_MASK_CONN34          0x1848
#define AFE_SECURE_MASK_CONN35          0x184c
#define AFE_SECURE_MASK_CONN36          0x1850
#define AFE_SECURE_MASK_CONN37          0x1854
#define AFE_SECURE_MASK_CONN38          0x1858
#define AFE_SECURE_MASK_CONN39          0x185c
#define AFE_SECURE_MASK_CONN40          0x1860
#define AFE_SECURE_MASK_CONN41          0x1864
#define AFE_SECURE_MASK_CONN42          0x1868
#define AFE_SECURE_MASK_CONN43          0x186c
#define AFE_SECURE_MASK_CONN0_1         0x18c4
#define AFE_SECURE_MASK_CONN1_1         0x18c8
#define AFE_SECURE_MASK_CONN2_1         0x18cc
#define AFE_SECURE_MASK_CONN3_1         0x18d0
#define AFE_SECURE_MASK_CONN4_1         0x18d4
#define AFE_SECURE_MASK_CONN5_1         0x18d8
#define AFE_SECURE_MASK_CONN6_1         0x18dc
#define AFE_SECURE_MASK_CONN7_1         0x18e0
#define AFE_SECURE_MASK_CONN8_1         0x18e4
#define AFE_SECURE_MASK_CONN9_1         0x18e8
#define AFE_SECURE_MASK_CONN10_1        0x18ec
#define AFE_SECURE_MASK_CONN11_1        0x18f0
#define AFE_SECURE_MASK_CONN12_1        0x18f4
#define AFE_SECURE_MASK_CONN13_1        0x18f8
#define AFE_SECURE_MASK_CONN14_1        0x18fc
#define AFE_SECURE_MASK_CONN_24BIT      0x1900
#define AFE_SECURE_MASK_CONN_16BIT      0x1904
#define AFE_SECURE_SIDEBAND0            0x1908
#define AFE_SECURE_SIDEBAND1            0x190c
#define AFE_SECURE_SIDEBAND2            0x1910
#define AFE_SECURE_SIDEBAND3            0x1914
#define AFE_SECURE_MASK_BASE_ADR_MSB    0x1920
#define AFE_SECURE_MASK_END_ADR_MSB     0x1924
#define AFE_NORMAL_BASE_ADR_MSB		0x192c
#define AFE_NORMAL_END_ADR_MSB		0x1930
#define AFE_DMIC0_UL_SRC_CON0           0x1a00
#define AFE_DMIC0_UL_SRC_CON1           0x1a04
#define AFE_DMIC0_SRC_DEBUG             0x1a08
#define AFE_DMIC0_SRC_DEBUG_MON0        0x1a0c
#define AFE_DMIC0_UL_SRC_MON0           0x1a10
#define AFE_DMIC0_UL_SRC_MON1           0x1a14
#define AFE_DMIC0_IIR_COEF_02_01        0x1a18
#define AFE_DMIC0_IIR_COEF_04_03        0x1a1c
#define AFE_DMIC0_IIR_COEF_06_05        0x1a20
#define AFE_DMIC0_IIR_COEF_08_07        0x1a24
#define AFE_DMIC0_IIR_COEF_10_09        0x1a28
#define AFE_DMIC1_UL_SRC_CON0           0x1a68
#define AFE_DMIC1_UL_SRC_CON1           0x1a6c
#define AFE_DMIC1_SRC_DEBUG             0x1a70
#define AFE_DMIC1_SRC_DEBUG_MON0        0x1a74
#define AFE_DMIC1_UL_SRC_MON0           0x1a78
#define AFE_DMIC1_UL_SRC_MON1           0x1a7c
#define AFE_DMIC1_IIR_COEF_02_01        0x1a80
#define AFE_DMIC1_IIR_COEF_04_03        0x1a84
#define AFE_DMIC1_IIR_COEF_06_05        0x1a88
#define AFE_DMIC1_IIR_COEF_08_07        0x1a8c
#define AFE_DMIC1_IIR_COEF_10_09        0x1a90
#define AFE_SECURE_MASK_CONN15_1        0x1e00
#define AFE_SECURE_MASK_CONN16_1        0x1e04
#define AFE_SECURE_MASK_CONN17_1        0x1e08
#define AFE_SECURE_MASK_CONN18_1        0x1e0c
#define AFE_SECURE_MASK_CONN19_1        0x1e10
#define AFE_SECURE_MASK_CONN20_1        0x1e14
#define AFE_SECURE_MASK_CONN21_1        0x1e18
#define AFE_SECURE_MASK_CONN22_1        0x1e1c
#define AFE_SECURE_MASK_CONN23_1        0x1e20
#define AFE_SECURE_MASK_CONN24_1        0x1e24
#define AFE_SECURE_MASK_CONN25_1        0x1e28
#define AFE_SECURE_MASK_CONN26_1        0x1e2c
#define AFE_SECURE_MASK_CONN27_1        0x1e30
#define AFE_SECURE_MASK_CONN28_1        0x1e34
#define AFE_SECURE_MASK_CONN29_1        0x1e38
#define AFE_SECURE_MASK_CONN30_1        0x1e3c
#define AFE_SECURE_MASK_CONN31_1        0x1e40
#define AFE_SECURE_MASK_CONN32_1        0x1e44
#define AFE_SECURE_MASK_CONN33_1        0x1e48
#define AFE_SECURE_MASK_CONN34_1        0x1e4c
#define AFE_SECURE_MASK_CONN35_1        0x1e50
#define AFE_SECURE_MASK_CONN36_1        0x1e54
#define AFE_SECURE_MASK_CONN37_1        0x1e58
#define AFE_SECURE_MASK_CONN38_1        0x1e5c
#define AFE_SECURE_MASK_CONN39_1        0x1e60
#define AFE_SECURE_MASK_CONN40_1        0x1e64
#define AFE_SECURE_MASK_CONN41_1        0x1e68
#define AFE_SECURE_MASK_CONN42_1        0x1e6c
#define AFE_SECURE_MASK_CONN43_1        0x1e70
#define AFE_SECURE_MASK_CONN_RS         0x1e8c
#define AFE_SECURE_MASK_CONN_RS_1       0x1e90
#define AFE_SECURE_MASK_CONN_16BIT_1    0x1e94
#define AFE_SECURE_MASK_CONN_24BIT_1    0x1e98
#define ETDM_IN1_MONITOR		0x22c0
#define ETDM_IN2_MONITOR		0x22c4
#define ETDM_OUT2_MONITOR		0x22d4
#define ETDM_COWORK_SEC_CON0            0x22e0
#define ETDM_COWORK_SEC_CON1            0x22e4

#define ETDM_COWORK_CON0		0x22f0
#define ETDM_COWORK_CON1		0x22f4
#define ETDM_IN1_CON0			0x2300
#define ETDM_IN1_CON1			0x2304
#define ETDM_IN1_CON2			0x2308
#define ETDM_IN1_CON3			0x230c
#define ETDM_IN1_CON4			0x2310
#define ETDM_IN1_CON5                   0x2314
#define ETDM_IN1_CON6                   0x2318
#define ETDM_IN2_CON0			0x2320
#define ETDM_IN2_CON1			0x2324
#define ETDM_IN2_CON2			0x2328
#define ETDM_IN2_CON3			0x232c
#define ETDM_IN2_CON4			0x2330
#define ETDM_IN2_CON5                   0x2334
#define ETDM_IN2_CON6                   0x2338
#define ETDM_IN2_CON7                   0x233c
#define ETDM_OUT2_CON0			0x23a0
#define ETDM_OUT2_CON1			0x23a4
#define ETDM_OUT2_CON2			0x23a8
#define ETDM_OUT2_CON3			0x23ac
#define ETDM_OUT2_CON4			0x23b0
#define ETDM_OUT2_CON6                  0x23b8
#define ETDM_OUT2_CON7                  0x23bc
#define GASRC_CFG0			0x2400

#define GASRC_TIMING_CON0		0x2408
#define GASRC_TIMING_CON1		0x240c
#define ASYS_TOP_DEBUG                  0x2500
#define UL2_AMIC_GAIN_CON0              0x2850
#define UL2_AMIC_GAIN_CON1              0x2854
#define UL2_AMIC_GAIN_CON2              0x2858
#define UL2_AMIC_GAIN_CON3              0x285c
#define UL2_AMIC_GAIN_CUR               0x2860
#define UL2_AMIC_IIR_COEF_02_01         0x2864
#define UL2_AMIC_IIR_COEF_04_03         0x2868
#define UL2_AMIC_IIR_COEF_06_05         0x286c
#define UL2_AMIC_IIR_COEF_08_07         0x2870
#define UL2_AMIC_IIR_COEF_10_09         0x2874
#define AFE_AD_UL2_SRC_CON1             0x2908
#define AFE_AD_UL2_SRC_DEBUG            0x290c
#define AFE_AD_UL2CF_CFG_02_01          0x2910
#define AFE_AD_UL2CF_CFG_04_03          0x2914
#define AFE_AD_UL2CF_CFG_06_05          0x2918
#define AFE_AD_UL2CF_CFG_08_07          0x291c
#define AFE_AD_UL2CF_CFG_10_09          0x2920
#define AFE_AD_UL2CF_CFG_12_11          0x2924
#define AFE_AD_UL2CF_CFG_14_13          0x2928
#define AFE_AD_UL2CF_CFG_16_15          0x292c
#define AFE_AD_UL2CF_CFG_18_17          0x2930
#define AFE_AD_UL2CF_CFG_20_19          0x2934
#define AFE_AD_UL2CF_CFG_22_21          0x2938
#define AFE_AD_UL2CF_CFG_24_23          0x293c
#define AFE_AD_UL2CF_CFG_26_25          0x2940
#define AFE_AD_UL2CF_CFG_28_27          0x2944
#define AFE_AD_UL2CF_CFG_30_29          0x2948
#define ABB_UL2AFE_CON0                 0x294c
#define ABB_UL2AFE_CON1                 0x2950
#define AFE_ADDA_DL_SRC2_CON0           0x2d08
#define AFE_ADDA_DL_SRC2_CON1           0x2d0c
#define AFE_ADDA_TOP_CON0               0x2d20
#define AFE_ADDA_UL_DL_CON0             0x2d24
#define AFE_ADDA_SRC_DEBUG              0x2d2c
#define AFE_ADDA_SRC_DEBUG_MON0         0x2d30
#define AFE_ADDA_SRC_DEBUG_MON1         0x2d34
#define AFE_ADDA_PREDIS_CON0            0x2d40
#define AFE_ADDA_PREDIS_CON1            0x2d44

#define AFE_NLE_CFG                     0x2d50
#define AFE_NLE_PRE_BUF_CFG             0x2d54
#define AFE_NLE_PWR_DET_LCH_CFG         0x2d58
#define AFE_NLE_ZCD_LCH_CFG             0x2d5c
#define AFE_NLE_GAIN_ADJ_LCH_CFG0       0x2d60
#define AFE_NLE_GAIN_ADJ_LCH_CFG1       0x2d64
#define AFE_NLE_GAIN_ADJ_LCH_CFG2       0x2d68
#define AFE_NLE_GAIN_IMP_LCH_CFG0       0x2d6c
#define AFE_NLE_GAIN_IMP_LCH_CFG1       0x2d70
#define AFE_NLE_GAIN_IMP_LCH_CFG2       0x2d74
#define AFE_NLE_PWE_DET_LCH_MON         0x2d78
#define AFE_NLE_GAIN_ADJ_LCH_MON0       0x2d7c
#define AFE_NLE_GAIN_ADJ_LCH_MON1       0x2d80
#define AFE_NLE_LCH_MON0                0x2d84
#define AFE_NLE_LCH_MON1                0x2d88
#define AFE_NLE_PWR_DET_RCH_CFG         0x2d90
#define AFE_NLE_ZCD_RCH_CFG             0x2d94
#define AFE_NLE_GAIN_ADJ_RCH_CFG0       0x2d98
#define AFE_NLE_GAIN_ADJ_RCH_CFG1       0x2d9c
#define AFE_NLE_GAIN_ADJ_RCH_CFG2       0x2da0
#define AFE_NLE_GAIN_IMP_RCH_CFG0       0x2da4
#define AFE_NLE_GAIN_IMP_RCH_CFG1       0x2da8
#define AFE_NLE_GAIN_IMP_RCH_CFG2       0x2dac
#define AFE_NLE_PWE_DET_RCH_MON         0x2db0
#define AFE_NLE_GAIN_ADJ_RCH_MON0       0x2db4
#define AFE_NLE_GAIN_ADJ_RCH_MON1       0x2db8
#define AFE_NLE_RCH_MON0                0x2dbc
#define AFE_NLE_RCH_MON1                0x2dc0
#define ABB_AFE_CON0                    0x2e00
#define ABB_AFE_CON1                    0x2e04
#define ABB_AFE_CON2                    0x2e08
#define ABB_AFE_CON3                    0x2e0c
#define ABB_AFE_CON4                    0x2e10
#define ABB_AFE_CON5                    0x2e14
#define ABB_AFE_CON6                    0x2e18
#define ABB_AFE_CON7                    0x2e1c
#define ABB_AFE_CON10                   0x2e28
#define ABB_AFE_CON11                   0x2e2c
#define ABB_AFE_STA0                    0x2e30
#define ABB_AFE_STA1                    0x2e34
#define ABB_AFE_STA2                    0x2e38
#define AFE_MON_DEBUG0                  0x2e44
#define AFE_MON_DEBUG1                  0x2e48
#define ABB_AFE_SDM_TEST                0x2e4c

#define AMIC_GAIN_CON0                  0x2e50
#define AMIC_GAIN_CON1                  0x2e54
#define AMIC_GAIN_CON2                  0x2e58
#define AMIC_GAIN_CON3                  0x2e5c
#define AMIC_GAIN_CUR                   0x2e60
#define AMIC_IIR_COEF_02_01             0x2e64
#define AMIC_IIR_COEF_04_03             0x2e68
#define AMIC_IIR_COEF_06_05             0x2e6c
#define AMIC_IIR_COEF_08_07             0x2e70
#define AMIC_IIR_COEF_10_09             0x2e74
#define AFE_AD_UL_SRC_CON1              0x2f08
#define AFE_AD_SRC_DEBUG                0x2f0c
#define AFE_AD_ULCF_CFG_02_01           0x2f10
#define AFE_AD_ULCF_CFG_04_03           0x2f14
#define AFE_AD_ULCF_CFG_06_05           0x2f18
#define AFE_AD_ULCF_CFG_08_07           0x2f1c
#define AFE_AD_ULCF_CFG_10_09           0x2f20
#define AFE_AD_ULCF_CFG_12_11           0x2f24
#define AFE_AD_ULCF_CFG_14_13           0x2f28
#define AFE_AD_ULCF_CFG_16_15           0x2f2c
#define AFE_AD_ULCF_CFG_18_17           0x2f30
#define AFE_AD_ULCF_CFG_20_19           0x2f34
#define AFE_AD_ULCF_CFG_22_21           0x2f38
#define AFE_AD_ULCF_CFG_24_23           0x2f3c
#define AFE_AD_ULCF_CFG_26_25           0x2f40
#define AFE_AD_ULCF_CFG_28_27           0x2f44
#define AFE_AD_ULCF_CFG_30_29           0x2f48
#define ABB_ULAFE_CON0                  0x2f4c
#define ABB_ULAFE_CON1                  0x2f50

#define MAX_REGISTER			ABB_ULAFE_CON1

#define AFE_IRQ_STATUS_BITS		0x1feffff
#define AFE_IRQ_MCU_CLR_BITS		0x1fe
#define ASYS_IRQ_CLR_BITS		0xffff

/* AUDIO_TOP_CON0 (0x0000) */
#define AUD_TCON0_PDN_ADC			BIT(28)
#define AUD_TCON0_PDN_TML			BIT(27)
#define AUD_TCON0_PDN_DAC_PREDIS		BIT(26)
#define AUD_TCON0_PDN_DAC			BIT(25)
#define AUD_TCON0_PDN_APLL2			BIT(24)
#define AUD_TCON0_PDN_APLL			BIT(23)
#define AUD_TCON0_PDN_SPDIF_OUT			BIT(21)
#define AUD_TCON0_PDN_APLL2_TUNER		BIT(20)
#define AUD_TCON0_PDN_APLL_TUNER		BIT(19)
#define AUD_TCON0_PDN_UPLINK_TML		BIT(18)
#define AUD_TCON0_SPDF_OUT_PLL_SEL_MASK		BIT(15)
#define AUD_TCON0_PDN_SPDIFIN_TUNER_APLL_CK	BIT(10)
#define AUD_TCON0_PDN_AFE			BIT(2)
#define AUD_TCON0_SPDF_OUT_SRC_APLL2		(1 << 15)
#define AUD_TCON0_SPDF_OUT_SRC_APLL1		(0 << 15)

/* AUDIO_TOP_CON1(0x0004) */
#define AUD_TCON1_PDN_A1SYS_HOPPING_CK		BIT(2)
#define AUD_TCON1_A1SYS_HP_SEL_MASK		GENMASK(1, 0)
#define AUD_TCON1_A1SYS_HP_26M_HP		(0)
#define AUD_TCON1_A1SYS_HP_A1SYS		(1)
#define AUD_TCON1_A1SYS_HP_26M_FIX		(2)

/* AUDIO_TOP_CON2 (0x0008) */
#define AUD_TCON0_CON2_SPDF_DIV_MASK		GENMASK(7, 0)
#define AUD_TCON0_CON2_SPDF_DIV(x)		(((x-1) & 0xff))

/* AUDIO_TOP_CON4 (0x0010) */
#define AUD_TCON4_PDN_GASRC3			BIT(29)
#define AUD_TCON4_PDN_GASRC2			BIT(28)
#define AUD_TCON4_PDN_GASRC1			BIT(27)
#define AUD_TCON4_PDN_GASRC0			BIT(26)
#define AUD_TCON4_PDN_PCMIF			BIT(24)
#define AUD_TCON4_PDN_AFE_CONN			BIT(23)
#define AUD_TCON4_PDN_A2SYS			BIT(22)
#define AUD_TCON4_PDN_A1SYS			BIT(21)
#define AUD_TCON4_PDN_INTDIR			BIT(20)
#define AUD_TCON4_PDN_MULTI_IN			BIT(19)
#define AUD_TCON4_PDN_DL_ASRC			BIT(18)
#define AUD_TCON4_PDN_ASRC12			BIT(17)
#define AUD_TCON4_PDN_ASRC11			BIT(16)
#define AUD_TCON4_PDN_TDM_OUT			BIT(7)
#define AUD_TCON4_PDN_I2S_OUT			BIT(6)
#define AUD_TCON4_PDN_TDM_IN			BIT(1)
#define AUD_TCON4_PDN_I2S_IN			BIT(0)

/* AUDIO_TOP_CON5 (0x0014) */
#define AUD_TCON5_PDN_DL10			BIT(26)
#define AUD_TCON5_PDN_DL6			BIT(22)
#define AUD_TCON5_PDN_DL3			BIT(19)
#define AUD_TCON5_PDN_DL2			BIT(18)
#define AUD_TCON5_PDN_UL10			BIT(9)
#define AUD_TCON5_PDN_UL9			BIT(8)
#define AUD_TCON5_PDN_UL8			BIT(7)
#define AUD_TCON5_PDN_UL5			BIT(4)
#define AUD_TCON5_PDN_UL4			BIT(3)
#define AUD_TCON5_PDN_UL3			BIT(2)
#define AUD_TCON5_PDN_UL2			BIT(1)
#define AUD_TCON5_PDN_UL1			BIT(0)

/* ASMO_TIMING_CON0 (0x0100) */
#define ASMO_TIMING_CON0_ASMO0_MODE_MASK	GENMASK(4, 0)
#define ASMO_TIMING_CON0_ASMO0_MODE_VAL(x)	((x & 0x1f) << 0)

/* PWR1_ASM_CON1 (0x0108) */
#define PWR1_ASM_CON1_GASRC0_CALI_CK_SEL_MASK	BIT(2)
#define PWR1_ASM_CON1_GASRC0_CALI_CK_SEL(x)	(x << 2)
#define PWR1_ASM_CON1_GASRC1_CALI_CK_SEL_MASK	BIT(5)
#define PWR1_ASM_CON1_GASRC1_CALI_CK_SEL(x)	(x << 5)
#define PWR1_ASM_CON1_GASRC2_CALI_CK_SEL_MASK	BIT(20)
#define PWR1_ASM_CON1_GASRC2_CALI_CK_SEL(x)	(x << 20)
#define PWR1_ASM_CON1_GASRC3_CALI_CK_SEL_MASK	BIT(23)
#define PWR1_ASM_CON1_GASRC3_CALI_CK_SEL(x)	(x << 23)
#define PWR1_ASM_CON1_DL_ASRC_CALI_CK_SEL_MASK	BIT(26)
#define PWR1_ASM_CON1_DL_ASRC_CALI_CK_SEL(x)	(x << 26)

/* AFE_IRQ_MASK (0x0178) */
#define AFE_IRQ_MASK_EN_BITS			(0x1feffff)
#define AFE_IRQ_MASK_EN_MASK			GENMASK(24, 0)

/* AFE_SINEGEN_CON0 (0x01f0) */
#define AFE_SINEGEN_CON0_INIT_MASK		(0x1f01f)
#define AFE_SINEGEN_CON0_INIT_VAL		(0x1001)
#define AFE_SINEGEN_CON0_EN			BIT(26)
#define AFE_SINEGEN_CON0_MODE_MASK		GENMASK(31, 27)
#define AFE_SINEGEN_CON0_FREQ_DIV_CH2_MASK	GENMASK(16, 12)
#define AFE_SINEGEN_CON0_FREQ_DIV_CH1_MASK	GENMASK(4, 0)
#define AFE_SINEGEN_CON0_FREQ_DIV_CH2(x)	(((x) & 0x1f) << 12)
#define AFE_SINEGEN_CON0_FREQ_DIV_CH1(x)	(((x) & 0x1f) << 0)

/* AFE_SINEGEN_CON1 (0x01f4) */
#define AFE_SINEGEN_CON1_TIMING_CH2_MASK	GENMASK(25, 21)
#define AFE_SINEGEN_CON1_TIMING_CH1_MASK	GENMASK(20, 16)
#define AFE_SINEGEN_CON1_TIMING_CH2(x)		(((x) & 0x1f) << 21)
#define AFE_SINEGEN_CON1_TIMING_CH1(x)		(((x) & 0x1f) << 16)
#define AFE_SINEGEN_CON1_TIMING_8K		(0)
#define AFE_SINEGEN_CON1_TIMING_12K		(1)
#define AFE_SINEGEN_CON1_TIMING_16K		(2)
#define AFE_SINEGEN_CON1_TIMING_24K		(3)
#define AFE_SINEGEN_CON1_TIMING_32K		(4)
#define AFE_SINEGEN_CON1_TIMING_48K		(5)
#define AFE_SINEGEN_CON1_TIMING_96K		(6)
#define AFE_SINEGEN_CON1_TIMING_192K		(7)
#define AFE_SINEGEN_CON1_TIMING_384K		(8)
#define AFE_SINEGEN_CON1_TIMING_7P35K		(16)
#define AFE_SINEGEN_CON1_TIMING_11P025K		(17)
#define AFE_SINEGEN_CON1_TIMING_14P7K		(18)
#define AFE_SINEGEN_CON1_TIMING_22P05K		(19)
#define AFE_SINEGEN_CON1_TIMING_29P4K		(20)
#define AFE_SINEGEN_CON1_TIMING_44P1K		(21)
#define AFE_SINEGEN_CON1_TIMING_88P2K		(22)
#define AFE_SINEGEN_CON1_TIMING_176P4K		(23)
#define AFE_SINEGEN_CON1_TIMING_352P8K		(24)
#define AFE_SINEGEN_CON1_TIMING_DL_1X_EN	(30)
#define AFE_SINEGEN_CON1_TIMING_SGEN_EN		(31)
#define AFE_SINEGEN_CON1_GASRC_IN_SGEN		BIT(13)
#define AFE_SINEGEN_CON1_GASRC_OUT_SGEN		BIT(12)

/* AFE_SPDIF_OUT_CON0 (0x0380) */
#define AFE_SPDIF_OUT_CON0_TIMING_MASK		BIT(1)
#define AFE_SPDIF_OUT_CON0_TIMING_ON		(1 << 1)
#define AFE_SPDIF_OUT_CON0_TIMING_OFF		(0 << 1)

/* AFE_APLL_TUNER_CFG (0x03f8) */
#define AFE_APLL_TUNER_CFG_MASK			GENMASK(15, 1)
#define AFE_APLL_TUNER_CFG_EN_MASK		GENMASK(0, 0)

/* AFE_APLL_TUNER_CFG1 (0x03fc) */
#define AFE_APLL_TUNER_CFG1_MASK		GENMASK(15, 1)
#define AFE_APLL_TUNER_CFG1_EN_MASK		GENMASK(0, 0)

/* AFE_PCM_SRC_FS_CON0 (0x0428) */
#define PCM_TX_RX_SYNC_SAMPLE_RATE_MASK		(0x1f << 3)
#define PCM_TX_RX_SYNC_FS(x)			((x) << 3)
#define EXT_PCM_1X_EN				(0xf)
#define PCM_TX_RX_SYNC_ENABLE_MASK		(0x1 << 0)
#define PCM_TX_RX_SYNC_ENABLE			(0x1 << 0)
#define PCM_TX_RX_SYNC_DISABLE			(0x0 << 0)

/* AFE_IEC_CFG (0x0480) */
#define AFE_IEC_CFG_SET_MASK			(0xff9e0073)
#define AFE_IEC_CFG_SW_RST_MASK			BIT(23)
#define AFE_IEC_CFG_FORCE_UPDATE_SIZE(x)	(((x) & 0xff) << 24)
#define AFE_IEC_CFG_FORCE_UPDATE		BIT(20)
#define AFE_IEC_CFG_SWAP_IEC_BYTE		BIT(17)
#define AFE_IEC_CFG_EN_MASK			BIT(16)
#define AFE_IEC_CFG_RAW_24BIT_SWITCH		BIT(6)
#define AFE_IEC_CFG_RAW_24BIT			BIT(5)
#define AFE_IEC_CFG_VALID_DATA			BIT(4)
#define AFE_IEC_CFG_NO_SW_RST			(1 << 23)
#define AFE_IEC_CFG_SW_RST			(0 << 23)
#define AFE_IEC_CFG_ENABLE_CTRL			(1 << 16)
#define AFE_IEC_CFG_DISABLE_CTRL		(0 << 16)
#define AFE_IEC_CFG_MUTE_DATA			(1 << 3)
#define AFE_IEC_CFG_UNMUTE_DATA			(0 << 3)
#define AFE_IEC_CFG_ENCODED_DATA		(1 << 1)
#define AFE_IEC_CFG_PCM_DATA			(0 << 1)
#define AFE_IEC_CFG_DATA_SRC_DRAM		(1 << 0)

/* AFE_IEC_NSNUM (0x0484) */
#define AFE_IEC_NSNUM_SET_MASK			(0x3fff3fff)
#define AFE_IEC_NSNUM_INTR_NUM(x)		(((x) & 0x3fff) << 16)
#define AFE_IEC_NSNUM_SAM_NUM(x)		(((x) & 0x3fff) << 0)

/* AFE_IEC_BURST_INFO (0x0488) */
#define AFE_IEC_BURST_INFO_READY_MASK		BIT(16)
#define AFE_IEC_BURST_INFO_NOT_READY		(1 << 16)
#define AFE_IEC_BURST_INFO_READY		(0 << 16)
#define AFE_IEC_BURST_INFO_SET_MASK		GENMASK(15, 0)

/* AFE_IEC_BURST_LEN (0x048c) */
#define AFE_IEC_BURST_LEN_SET_MASK		GENMASK(18, 0)

/* AFE_IEC_CHL_STAT0 (0x04a0)
 * AFE_IEC_CHL_STAT1 (0x04a4)
 * AFE_IEC_CHR_STAT0 (0x04a8)
 * AFE_IEC_CHR_STAT1 (0x04ac)
 */
#define AFE_IEC_CH_STAT0_SET_MASK		GENMASK(31, 0)
#define AFE_IEC_CH_STAT1_SET_MASK		GENMASK(15, 0)

/* AFE_SPDIFIN_CFG0 (0x0500) */
#define AFE_SPDIFIN_CFG0_SET_MASK		(0x3ffff7a)
#define AFE_SPDIFIN_CFG0_MAX_LEN_NUM(x)		(((x) & 0xff) << 16)
#define AFE_SPDIFIN_CFG0_GMAT_BC_256_CYCLES	(3 << 24)
#define AFE_SPDIFIN_CFG0_DE_SEL_3_SAMPLES	(0 << 13)
#define AFE_SPDIFIN_CFG0_DE_SEL_14_SAMPLES	(1 << 13)
#define AFE_SPDIFIN_CFG0_DE_SEL_30_SAMPLES	(2 << 13)
#define AFE_SPDIFIN_CFG0_DE_SEL_CNT		(3 << 13)
#define AFE_SPDIFIN_CFG0_DE_CNT(x)		(((x) & 0x1f) << 8)
#define AFE_SPDIFIN_CFG0_INT_EN			BIT(6)
#define AFE_SPDIFIN_CFG0_DERR2IDLE_EN		(BIT(5)|BIT(4))
#define AFE_SPDIFIN_CFG0_DPERR2IDLE_EN		BIT(3)
#define AFE_SPDIFIN_CFG0_FLIP			BIT(1)
#define AFE_SPDIFIN_CFG0_EN			BIT(0)

/* AFE_SPDIFIN_CFG1 (0x0504) */
#define AFE_SPDIFIN_CFG1_SET_MASK		(0xfff10073)
#define AFE_SPDIFIN_CFG1_FIFOSTART_MASK		GENMASK(6, 4)
#define AFE_SPDIFIN_CFG1_CHSTS_INT		BIT(30)
#define AFE_SPDIFIN_CFG1_CHSTS_CHANGE_INT	BIT(29)
#define AFE_SPDIFIN_CFG1_TIMEOUT_INT		BIT(28)
#define AFE_SPDIFIN_CFG1_PREAMBLE_ERR		BIT(20)
#define AFE_SPDIFIN_CFG1_SEL_DEC0_CLK_EN	BIT(1)
#define AFE_SPDIFIN_CFG1_SEL_DEC0_DATA_EN	BIT(0)
#define AFE_SPDIFIN_CFG1_SEL_BCK_SPDIFIN	(0x1 << 16)
#define AFE_SPDIFIN_CFG1_FIFOSTART_5POINTS	(0x1 << 4)
#define AFE_SPDIFIN_CFG1_INT_BITS		(0x7f1 << 20)

/* AFE_SPDIFIN_DEBUG1 (0x0520) */
#define AFE_SPDIFIN_DEBUG1_DATALAT_ERR		BIT(10)
#define AFE_SPDIFIN_DEBUG1_CS_MASK		GENMASK(28, 24)

/* AFE_SPDIFIN_DEBUG2 (0x0524) */
#define AFE_SPDIFIN_DEBUG2_FIFO_ERR		(BIT(31)|BIT(30))
#define AFE_SPDIFIN_DEBUG2_CHSTS_INT_FLAG	BIT(26)
#define AFE_SPDIFIN_DEBUG2_PERR_9TIMES_FLAG	BIT(25)

/* AFE_SPDIFIN_DEBUG3 (0x0528) */
#define AFE_SPDIFIN_DEBUG3_ALL_ERR			GENMASK(6, 0)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_NON_STS		BIT(0)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_B_STS		BIT(1)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_M_STS		BIT(2)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_W_STS		BIT(3)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_BITCNT_STS		BIT(4)
#define AFE_SPDIFIN_DEBUG3_PRE_ERR_PARITY_STS		BIT(5)
#define AFE_SPDIFIN_DEBUG3_TIMEOUT_ERR_STS		BIT(6)
#define AFE_SPDIFIN_DEBUG3_CHSTS_PREAMPHASIS_STS	BIT(7)

/* AFE_SPDIFIN_EC (0x0530) */
#define AFE_SPDIFIN_EC_CLEAR_ALL		(0x30fff)
#define AFE_SPDIFIN_EC_PRE_ERR_CLEAR		BIT(0)
#define AFE_SPDIFIN_EC_PRE_ERR_B_CLEAR		BIT(1)
#define AFE_SPDIFIN_EC_PRE_ERR_M_CLEAR		BIT(2)
#define AFE_SPDIFIN_EC_PRE_ERR_W_CLEAR		BIT(3)
#define AFE_SPDIFIN_EC_PRE_ERR_BITCNT_CLEAR	BIT(4)
#define AFE_SPDIFIN_EC_PRE_ERR_PARITY_CLEAR	BIT(5)
#define AFE_SPDIFIN_EC_FIFO_ERR_CLEAR		(BIT(7)|BIT(6))
#define AFE_SPDIFIN_EC_TIMEOUT_INT_CLEAR	BIT(8)
#define AFE_SPDIFIN_EC_CHSTS_PREAMPHASIS_CLEAR	BIT(9)
#define AFE_SPDIFIN_EC_USECODE_COLLECTION_CLEAR	BIT(10)
#define AFE_SPDIFIN_EC_CHSTS_COLLECTION_CLEAR	BIT(11)
#define AFE_SPDIFIN_EC_DATA_LRCK_CHANGE_CLEAR	BIT(16)
#define AFE_SPDIFIN_EC_DATA_LATCH_CLEAR		BIT(17)

/* AFE_SPDIFIN_INT_EXT (0x0548) */
#define AFE_SPDIFIN_INT_EXT_INPUT_SEL_MASK	GENMASK(15, 14)
#define AFE_SPDIFIN_INT_EXT_SET_MASK		(0xeff00)
#define AFE_SPDIFIN_INT_EXT_DATALAT_ERR_EN	BIT(17)
#define AFE_SPDIFIN_INT_EXT_SEL_OPTICAL		(0 << 14)
#define AFE_SPDIFIN_INT_EXT_SEL_COAXIAL		(1 << 14)
#define AFE_SPDIFIN_INT_EXT_SEL_ARC		(2 << 14)
#define AFE_SPDIFIN_INT_EXT_SEL_TIED_LOW	(3 << 14)

/* AFE_SPDIFIN_INT_EXT2 (0x054c) */
#define AFE_SPDIFIN_INT_EXT2_ROUGH_FS_MASK	GENMASK(31, 28)
#define AFE_SPDIFIN_INT_EXT2_FS_NOT_DEFINED	(0 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_32K		(1 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_44D1K		(2 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_48K		(3 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_64K		(4 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_88D2K		(5 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_96K		(6 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_128K		(7 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_144K		(8 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_176D4K		(9 << 28)
#define AFE_SPDIFIN_INT_EXT2_FS_192K		(10 << 28)
#define AFE_SPDIFIN_INT_EXT2_LRCK_CHANGE	BIT(27)
#define SPDIFIN_594MODE_MASK			(1 << 17)
#define SPDIFIN_594MODE_EN			(1 << 17)

/* ASYS_TOP_CON (0x0600) */
#define ASYS_TCON_A1SYS_TIMING_ON		BIT(0)
#define ASYS_TCON_A2SYS_TIMING_ON		BIT(1)
#define ASYS_TCON_LP_26M_ENGEN_ON		BIT(2)
#define ASYS_TCON_LP_MODE_ON			BIT(3)
#define ASYS_TCON_O34_O41_1X_EN_MASK		BIT(15)
#define ASYS_TCON_O34_O41_1X_EN_UL9		(0 << 15)
#define ASYS_TCON_O34_O41_1X_EN_UL2		(1 << 15)
#define ASYS_TCON_O26_O33_1X_EN_MASK		BIT(14)
#define ASYS_TCON_O26_O33_1X_EN_UL9		(0 << 14)
#define ASYS_TCON_O26_O33_1X_EN_UL2		(1 << 14)
#define ASYS_TCON_UL8_USE_SINEGEN		BIT(8)

/* PWR2_TOP_CON (0x0634) */
#define PWR2_TOP_CON_PDN_DMIC0			BIT(0)
#define PWR2_TOP_CON_PDN_DMIC1			BIT(1)
#define PWR2_TOP_CON_PDN_DMIC2			BIT(2)
#define PWR2_TOP_CON_PDN_DMIC3			BIT(3)
#define PWR2_TOP_CON_DMIC8_SRC_SEL_MASK		GENMASK(31, 29)
#define PWR2_TOP_CON_DMIC7_SRC_SEL_MASK		GENMASK(28, 26)
#define PWR2_TOP_CON_DMIC6_SRC_SEL_MASK		GENMASK(25, 23)
#define PWR2_TOP_CON_DMIC5_SRC_SEL_MASK		GENMASK(22, 20)
#define PWR2_TOP_CON_DMIC4_SRC_SEL_MASK		GENMASK(19, 17)
#define PWR2_TOP_CON_DMIC3_SRC_SEL_MASK		GENMASK(16, 14)
#define PWR2_TOP_CON_DMIC2_SRC_SEL_MASK		GENMASK(13, 11)
#define PWR2_TOP_CON_DMIC1_SRC_SEL_MASK		GENMASK(10, 8)
#define PWR2_TOP_CON_DMIC8_SRC_SEL_VAL(x)	(x << 29)
#define PWR2_TOP_CON_DMIC7_SRC_SEL_VAL(x)	(x << 26)
#define PWR2_TOP_CON_DMIC6_SRC_SEL_VAL(x)	(x << 23)
#define PWR2_TOP_CON_DMIC5_SRC_SEL_VAL(x)	(x << 20)
#define PWR2_TOP_CON_DMIC4_SRC_SEL_VAL(x)	(x << 17)
#define PWR2_TOP_CON_DMIC3_SRC_SEL_VAL(x)	(x << 14)
#define PWR2_TOP_CON_DMIC2_SRC_SEL_VAL(x)	(x << 11)
#define PWR2_TOP_CON_DMIC1_SRC_SEL_VAL(x)	(x << 8)

#define PWR2_TOP_CON1_DMIC_PDM_INTF_ON		BIT(1)

/* PCM_INTF_CON1 (0x063c) */
#define PCM_INTF_CON1_16BIT			(0 << 16)
#define PCM_INTF_CON1_24BIT			(1 << 16)
#define PCM_INTF_CON1_32BCK			(0 << 14)
#define PCM_INTF_CON1_64BCK			(1 << 14)
#define PCM_INTF_CON1_MASTER_MODE		(0 << 5)
#define PCM_INTF_CON1_SLAVE_MODE		(1 << 5)
#define PCM_INTF_CON1_FS_8K			(0 << 3)
#define PCM_INTF_CON1_FS_16K			(1 << 3)
#define PCM_INTF_CON1_FS_32K			(2 << 3)
#define PCM_INTF_CON1_FS_48K			(3 << 3)
#define PCM_INTF_CON1_SYNC_LEN(x)		(((x) - 1) << 9)
#define PCM_INTF_CON1_FORMAT(x)			((x) << 1)
#define PCM_INTF_CON1_SYNC_OUT_INV		BIT(23)
#define PCM_INTF_CON1_BCLK_OUT_INV		BIT(22)
#define PCM_INTF_CON1_SYNC_IN_INV		BIT(21)
#define PCM_INTF_CON1_BCLK_IN_INV		BIT(20)
#define PCM_INTF_CON1_BYPASS_ASRC		BIT(6)
#define PCM_INTF_CON1_EN			BIT(0)
#define PCM_INTF_CON1_CONFIG_MASK		(0xf1fffe)

/* PCM_INTF_CON2 (0x0640) */
#define PCM_INTF_CON2_LPBK_EN			BIT(8)
#define PCM_CLK_DOMAIN_POS			(23)
#define PCM_LK_DOMAIN_MASK			(0x3<<PCM_CLK_DOMAIN_POS)

/* AFE_MPHONE_MULTI_CON0 (0x06a4) */
#define AFE_MPHONE_MULTI_CON0_SET_MASK		(0x3ffc07e)
#define AFE_MPHONE_MULTI_CON0_SDATA3_SEL(x)	(((x) & 0x7) << 23)
#define AFE_MPHONE_MULTI_CON0_SDATA2_SEL(x)	(((x) & 0x7) << 20)
#define AFE_MPHONE_MULTI_CON0_SDATA1_SEL(x)	(((x) & 0x7) << 17)
#define AFE_MPHONE_MULTI_CON0_SDATA0_SEL(x)	(((x) & 0x7) << 14)
#define AFE_MPHONE_MULTI_CON0_256DWORD_PERIOD	(0x3 << 4)
#define AFE_MPHONE_MULTI_CON0_128DWORD_PERIOD	(0x2 << 4)
#define AFE_MPHONE_MULTI_CON0_64DWORD_PERIOD	(0x1 << 4)
#define AFE_MPHONE_MULTI_CON0_32DWORD_PERIOD	(0x0 << 4)
#define AFE_MPHONE_MULTI_CON0_16BIT_SWAP	BIT(3)
#define AFE_MPHONE_MULTI_CON0_24BIT_DATA	(0x1 << 1)
#define AFE_MPHONE_MULTI_CON0_16BIT_DATA	(0x0 << 1)
#define AFE_MPHONE_MULTI_CON0_EN		BIT(0)

/* AFE_MPHONE_MULTI_CON1 (0x06a8) */
#define AFE_MPHONE_MULTI_CON1_SET_MASK		(0xfffff6f)
#define AFE_MPHONE_MULTI_CON1_SYNC_ON		BIT(24)
#define AFE_MPHONE_MULTI_CON1_24BIT_SWAP_BYPASS	BIT(22)
#define AFE_MPHONE_MULTI_CON1_NON_COMPACT_MODE	(0x1 << 19)
#define AFE_MPHONE_MULTI_CON1_COMPACT_MODE	(0x0 << 19)
#define AFE_MPHONE_MULTI_CON1_HBR_MODE		BIT(18)
#define AFE_MPHONE_MULTI_CON1_LRCK_32_CYCLE	(0x2 << 16)
#define AFE_MPHONE_MULTI_CON1_LRCK_24_CYCLE	(0x1 << 16)
#define AFE_MPHONE_MULTI_CON1_LRCK_16_CYCLE	(0x0 << 16)
#define AFE_MPHONE_MULTI_CON1_LRCK_INV		BIT(15)
#define AFE_MPHONE_MULTI_CON1_DELAY_DATA	BIT(14)
#define AFE_MPHONE_MULTI_CON1_LEFT_ALIGN	BIT(13)
#define AFE_MPHONE_MULTI_CON1_BCK_INV		BIT(6)
#define AFE_MPHONE_MULTI_CON1_BIT_NUM(x)	((((x) - 1) & 0x1f) << 8)
#define AFE_MPHONE_MULTI_CON1_CH_NUM(x)		((((x) >> 1) - 1) & 0x3)

/* AFE_CONN76 (0x07f0) */
#define AFE_CONN76_I10_I11_SEL_MASK		GENMASK(31, 29)
#define AFE_CONN76_I18_I19_SEL_MASK		GENMASK(28, 27)
#define AFE_CONN76_I10_I11_SEL_DMIC		(0 << 29)
#define AFE_CONN76_I10_I11_SEL_AMIC		(1 << 29)
#define AFE_CONN76_I10_I11_SEL_ETDM_IN1		(2 << 29)
#define AFE_CONN76_I10_I11_SEL_AMIC_FIFO	(4 << 29)
#define AFE_CONN76_I18_I19_SEL_ETDM_IN2		(0 << 27)
#define AFE_CONN76_I18_I19_SEL_AMIC		(1 << 27)
#define AFE_CONN76_I18_I19_SEL_AMIC_FIFO	(2 << 27)

/* AFE_GASRC0_NEW_CON0 (0x0800)
 * AFE_GASRC1_NEW_CON0 (0x0840)
 * AFE_GASRC2_NEW_CON0 (0x0880)
 * AFE_GASRC3_NEW_CON0 (0x08c0)
 */
#define GASRC_NEW_CON0_ONE_HEART			BIT(31)
#define GASRC_NEW_CON0_CHSET0_CLR_IIR_HISTORY	BIT(17)
#define GASRC_NEW_CON0_CHSET0_OFS_SEL_MASK	GENMASK(15, 14)
#define GASRC_NEW_CON0_CHSET0_OFS_SEL_TX		(0 << 14)
#define GASRC_NEW_CON0_CHSET0_OFS_SEL_RX		(1 << 14)
#define GASRC_NEW_CON0_CHSET0_IFS_SEL_MASK	GENMASK(13, 12)
#define GASRC_NEW_CON0_CHSET0_IFS_SEL_TX		(3 << 12)
#define GASRC_NEW_CON0_CHSET0_IFS_SEL_RX		(2 << 12)
#define GASRC_NEW_CON0_CHSET0_IIR_EN		BIT(11)
#define GASRC_NEW_CON0_CHSET0_IIR_STAGE(x)	(((x) - 1) << 8)
#define GASRC_NEW_CON0_CHSET0_IIR_STAGE_MASK	GENMASK(10, 8)
#define GASRC_NEW_CON0_CHSET_STR_CLR		BIT(4)
#define GASRC_NEW_CON0_COEFF_SRAM_CTRL		BIT(1)
#define GASRC_NEW_CON0_ASM_ON				BIT(0)


/* AFE_GASRC0_NEW_CON6 (0x0818)
 * AFE_GASRC1_NEW_CON6 (0x0858)
 * AFE_GASRC2_NEW_CON6 (0x0898)
 * AFE_GASRC3_NEW_CON6 (0x08d8)
 */
#define GASRC_NEW_CON6_FREQ_CALI_CYCLE_MASK	GENMASK(31, 16)
#define GASRC_NEW_CON6_FREQ_CALI_CYCLE(x)	(((x - 1) & 0xffff) << 16)
#define GASRC_NEW_CON6_AUTO_TUNE_FREQ3	BIT(12)
#define GASRC_NEW_CON6_COMP_FREQ_RES_EN	BIT(11)
#define GASRC_NEW_CON6_FREQ_CALI_BP_DGL	BIT(7)
#define GASRC_NEW_CON6_AUTO_TUNE_FREQ2	BIT(3)
#define GASRC_NEW_CON6_FREQ_CALI_AUTO_RESTART	BIT(2)
#define GASRC_NEW_CON6_CALI_USE_FREQ_OUT	BIT(1)
#define GASRC_NEW_CON6_CALI_EN				BIT(0)

/* AFE_GASRC0_NEW_CON7 (0x081c)
 * AFE_GASRC1_NEW_CON7 (0x085c)
 * AFE_GASRC2_NEW_CON7 (0x089c)
 * AFE_GASRC3_NEW_CON7 (0x08dc)
 */
#define GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_MASK	GENMASK(23, 0)
#define GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_49M	(0x3C00)
#define GASRC_NEW_CON7_FREQ_CALC_DENOMINATOR_45M	(0x3720)

/* AFE_GASRC0_NEW_CON13 (0x0834)
 * AFE_GASRC0_NEW_CON14 (0x0838)
 * AFE_GASRC1_NEW_CON13 (0x0874)
 * AFE_GASRC1_NEW_CON14 (0x0878)
 * AFE_GASRC2_NEW_CON13 (0x08b4)
 * AFE_GASRC2_NEW_CON14 (0x08b8)
 * AFE_GASRC3_NEW_CON13 (0x08f4)
 * AFE_GASRC3_NEW_CON14 (0x08f8)
 */
#define GASRC_NEW_CON_FREQ_CALI_AUTORST_TH_MASK	GENMASK(23, 0)

/* AFE_DMIC0_UL_SRC_CON0  (0x1A00)
 * AFE_DMIC1_UL_SRC_CON0 (0x1A68)
 * AFE_DMIC2_UL_SRC_CON0 (0x1AD0)
 * AFE_DMIC3_UL_SRC_CON0 (0x1B38)
 */

#define DMIC_UL_CON0_SRC_ON_TMP_CTL		BIT(0)
#define DMIC_UL_CON0_SDM_3_LEVEL_CTL		BIT(1)
#define DMIC_UL_CON0_LOOPBACK_MODE_CTL		BIT(2)
#define DMIC_UL_CON0_3P25M_1P625M_SEL(x)	((x) << 5)
#define DMIC_UL_CON0_IIR_MODE_SEL(x)		((x) << 7)
#define DMIC_UL_CON0_IIR_ON_TMP_CTL		BIT(10)
#define DMIC_UL_CON0_DISABLE_HW_CG_CTL		BIT(12)
#define DMIC_UL_CON0_LOW_POWER_MODE_SEL(x)	((x) << 14)
#define DMIC_UL_CON0_VOCIE_MODE_8K		(0 << 17)
#define DMIC_UL_CON0_VOCIE_MODE_16K		(1 << 17)
#define DMIC_UL_CON0_VOCIE_MODE_32K		(2 << 17)
#define DMIC_UL_CON0_VOCIE_MODE_48K		(3 << 17)
#define DMIC_UL_CON0_MODE_3P25M_CH1_CTL		BIT(21)
#define DMIC_UL_CON0_MODE_3P25M_CH2_CTL		BIT(22)
#define DMIC_UL_CON0_TWO_WIRE_MODE_CTL		BIT(23)
#define DMIC_UL_CON0_PHASE_SEL_CH2(x)		((x) << 24)
#define DMIC_UL_CON0_PHASE_SEL_CH1(x)		((x) << 27)
#define DMIC_UL_CON0_ULCF_CFG_EN_CTL		BIT(31)
#define DMIC_UL_CON0_CONFIG_MASK		(0xBF8ED7A6)

/* AFE_DMIC0_UL_SRC_CON1  (0x1A04)
 * AFE_DMIC1_UL_SRC_CON1 (0x1A6C)
 * AFE_DMIC2_UL_SRC_CON1 (0x1AD4)
 * AFE_DMIC3_UL_SRC_CON1 (0x1B3C)
 */

#define DMIC_UL_CON1_SGEN_EN			BIT(27)
#define DMIC_UL_CON1_SGEN_MUTE			BIT(26)
#define DMIC_UL_CON1_TRIANGULAR_TONE		BIT(25)
#define DMIC_UL_CON1_SGEN_CH2_AMP_DIV(x)	((x) << 21)
#define DMIC_UL_CON1_SGEN_CH2_FREQ_DIV(x)	((x) << 16)
#define DMIC_UL_CON1_SGEN_CH2_SINE_MODE(x)	((x) << 12)
#define DMIC_UL_CON1_SGEN_CH1_AMP_DIV(x)	((x) << 9)
#define DMIC_UL_CON1_SGEN_CH1_FREQ_DIV(x)	((x) << 4)
#define DMIC_UL_CON1_SGEN_CH1_SINE_MODE(x)	((x) << 0)

/* AFE_ADDA_UL_DL_CON0 */
#define ADDA_UL_DL_CON0_DMIC_CLKDIV_ON		BIT(1)
#define ADDA_UL_DL_CON0_ADDA_INTF_ON		BIT(0)

/* ETDM_COWORK_CON0 (0x22f0) */
#define ETDM_COWORK_CON0_TDM_IN1_SLV_SEL_MASK		GENMASK(27, 24)
#define ETDM_COWORK_CON0_TDM_IN1_SLV_SEL_IN1_SLV	(0x1 << 24)
#define ETDM_COWORK_CON0_TDM_IN1_SLV_SEL_OUT1_MAS	(0x8 << 24)
#define ETDM_COWORK_CON0_TDM_IN1_SLV_SEL_OUT2_MAS	(0xa << 24)

/* ETDM_COWORK_CON1 (0x22f4) */
#define ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_MASK		GENMASK(3, 0)
#define ETDM_COWORK_CON1_TDM_IN1_DAT1_3_SEL_MASK	GENMASK(7, 4)
#define ETDM_COWORK_CON1_TDM_IN2_DAT0_SEL_MASK		GENMASK(19, 16)
#define ETDM_COWORK_CON1_TDM_IN2_DAT1_3_SEL_MASK	GENMASK(23, 20)
#define ETDM_COWORK_CON1_TDM_IN2_SLV_SEL_MASK		GENMASK(11, 8)
#define ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_PAD		(0x0)
#define ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT1		(0x8)
#define ETDM_COWORK_CON1_TDM_IN1_DAT0_SEL_OUT2		(0xa)
#define ETDM_COWORK_CON1_TDM_IN1_DAT1_3_SEL_PAD		(0x0 << 4)
#define ETDM_COWORK_CON1_TDM_IN1_DAT1_3_SEL_OUT1	(0x8 << 4)
#define ETDM_COWORK_CON1_TDM_IN1_DAT1_3_SEL_OUT2	(0xa << 4)
#define ETDM_COWORK_CON1_TDM_IN2_DAT0_SEL_PAD		(0x2 << 16)
#define ETDM_COWORK_CON1_TDM_IN2_DAT0_SEL_OUT1		(0x8 << 16)
#define ETDM_COWORK_CON1_TDM_IN2_DAT0_SEL_OUT2		(0xa << 16)
#define ETDM_COWORK_CON1_TDM_IN2_DAT1_3_SEL_PAD		(0x2 << 20)
#define ETDM_COWORK_CON1_TDM_IN2_DAT1_3_SEL_OUT1	(0x8 << 20)
#define ETDM_COWORK_CON1_TDM_IN2_DAT1_3_SEL_OUT2	(0xa << 20)
#define ETDM_COWORK_CON1_TDM_IN2_SLV_SEL_IN2_SLV	(0x3 << 8)
#define ETDM_COWORK_CON1_TDM_IN2_SLV_SEL_OUT1_MAS	(0x8 << 8)
#define ETDM_COWORK_CON1_TDM_IN2_SLV_SEL_OUT2_MAS	(0xa << 8)
#define ETDM_COWORK_CON1_TDM_IN2_BYPASS_INTERCONN	BIT(24)

/* ETDM_COWORK_CON3 (0x22fc) */
#define ETDM_COWORK_CON3_OUT1_USE_SINEGEN	BIT(28)

#define UL_REORDER_START_DATA(x)		(((x) & 0xf) << 8)
#define UL_REORDER_NO_BYPASS			BIT(6)
#define UL_REORDER_EN				BIT(4)
#define UL_REORDER_CHANNEL(x)			(((x) - 1))
#define UL_REORDER_CTRL_MASK			(0xfef)

#define ETDM_CON0_CH_NUM(x)			(((x) - 1) << 23)
#define ETDM_CON0_WORD_LEN(x)			(((x) - 1) << 16)
#define ETDM_CON0_BIT_LEN(x)			(((x) - 1) << 11)
#define ETDM_CON0_FORMAT(x)			((x) << 6)
#define ETDM_CON0_MASTER_LRCK_INV		BIT(30)
#define ETDM_CON0_MASTER_BCK_INV		BIT(29)
#define ETDM_CON0_SLAVE_LRCK_INV		BIT(28)
#define ETDM_CON0_SLAVE_BCK_INV			BIT(27)
#define ETDM_CON0_SLAVE_MODE			BIT(5)
#define ETDM_CON0_LOW_POWER_MODE		BIT(2)
#define ETDM_CON0_SYNC_MODE			BIT(1)

#define ETDM_CON1_MCLK_OUTPUT			BIT(16)
#define ETDM_CON1_LRCK_MANUAL_MODE		(0 << 29)
#define ETDM_CON1_LRCK_AUTO_MODE		(1 << 29)
#define ETDM_CON1_BCK_FROM_DIVIDER		BIT(30)

#define ETDM_CON4_ASYNC_RESET			BIT(11)

#define ETDM_IN_CON0_CTRL_MASK			(0x7f9ff9e2)
#define ETDM_IN_CON1_CTRL_MASK			(0x7ff10000)
#define ETDM_IN_CON2_CTRL_MASK			(0x840f801f)
#define ETDM_IN_CON3_CTRL_MASK			(0x7c000000)
#define ETDM_IN_CON4_CTRL_MASK			(0x1f000000)

#define ETDM_IN_CON1_LRCK_WIDTH(x)		(((x) - 1) << 20)
#define ETDM_IN_CON2_MULTI_IP_CH(x)		(((x) - 1) << 16)
#define ETDM_IN_CON2_MULTI_IP_2CH_MODE		BIT(31)
#define ETDM_IN_CON2_MULTI_IP_ONE_DATA		BIT(15)
#define ETDM_IN_CON2_UPDATE_POINT_AUTO_DIS	(0 << 26)
#define ETDM_IN_CON2_UPDATE_POINT_AUTO_EN	(1 << 26)
#define ETDM_IN_CON2_UPDATE_POINT(x)		((x) & 0x1f)
#define ETDM_IN_CON3_FS(x)			(((x) & 0x1f) << 26)
#define ETDM_IN_CON4_CONN_FS(x)			(((x) & 0x1f) << 24)

#define ETDM_OUT_CON0_CTRL_MASK			(0x7f9ff9e2)
#define ETDM_OUT_CON1_CTRL_MASK			(0x7ff10000)
#define ETDM_OUT_CON4_CTRL_MASK			(0x1f)

#define ETDM_OUT_CON1_LRCK_WIDTH(x)		(((x) - 1) << 20)
#define ETDM_OUT_CON4_FS(x)			(((x) & 0x1f) << 0)
#define ETDM_OUT_CON4_CONN_FS(x)		(((x) & 0x1f) << 24)

/* ETDM_OUT2_CON4 (0x23b0) */
#define ETDM_OUT_CON4_INTERCONN_EN_SEL_MASK	(0x1f000000)

/* GASRC_CFG0 (0x2400) */
#define GASRC_CFG0_GASRC0_SOFT_RST		BIT(0)
#define GASRC_CFG0_GASRC1_SOFT_RST		BIT(8)
#define GASRC_CFG0_GASRC2_SOFT_RST		BIT(16)
#define GASRC_CFG0_GASRC3_SOFT_RST		BIT(24)
#define GASRC_CFG0_GASRC0_LRCK_SEL_MASK	GENMASK(6, 4)
#define GASRC_CFG0_GASRC0_LRCK_SEL(x)	(((x) & 0x7) << 4)
#define GASRC_CFG0_GASRC1_LRCK_SEL_MASK	GENMASK(14, 12)
#define GASRC_CFG0_GASRC1_LRCK_SEL(x)	(((x) & 0x7) << 12)
#define GASRC_CFG0_GASRC2_LRCK_SEL_MASK	GENMASK(22, 20)
#define GASRC_CFG0_GASRC2_LRCK_SEL(x)	(((x) & 0x7) << 20)
#define GASRC_CFG0_GASRC3_LRCK_SEL_MASK	GENMASK(30, 28)
#define GASRC_CFG0_GASRC3_LRCK_SEL(x)	(((x) & 0x7) << 28)
#define GASRC_CFG0_GASRC0_USE_SEL_MASK	BIT(1)
#define GASRC_CFG0_GASRC0_USE_SEL(x)	((x) << 1)
#define GASRC_CFG0_GASRC1_USE_SEL_MASK	BIT(9)
#define GASRC_CFG0_GASRC1_USE_SEL(x)	((x) << 9)
#define GASRC_CFG0_GASRC2_USE_SEL_MASK	BIT(17)
#define GASRC_CFG0_GASRC2_USE_SEL(x)	((x) << 17)
#define GASRC_CFG0_GASRC3_USE_SEL_MASK	BIT(25)
#define GASRC_CFG0_GASRC3_USE_SEL(x)	((x) << 25)

/* GASRC_TIMING_CON0 (0x2408) */
#define GASRC_TIMING_CON0_GASRC0_IN_MODE(x)	(((x) & 0x1f) << 0)
#define GASRC_TIMING_CON0_GASRC1_IN_MODE(x)	(((x) & 0x1f) << 5)
#define GASRC_TIMING_CON0_GASRC2_IN_MODE(x)	(((x) & 0x1f) << 10)
#define GASRC_TIMING_CON0_GASRC3_IN_MODE(x)	(((x) & 0x1f) << 15)
#define GASRC_TIMING_CON0_GASRC0_IN_MODE_MASK	GENMASK(4, 0)
#define GASRC_TIMING_CON0_GASRC1_IN_MODE_MASK	GENMASK(9, 5)
#define GASRC_TIMING_CON0_GASRC2_IN_MODE_MASK	GENMASK(14, 10)
#define GASRC_TIMING_CON0_GASRC3_IN_MODE_MASK	GENMASK(19, 15)

/* GASRC_TIMING_CON1 (0x240c) */
#define GASRC_TIMING_CON1_GASRC0_OUT_MODE(x)	(((x) & 0x1f) << 0)
#define GASRC_TIMING_CON1_GASRC1_OUT_MODE(x)	(((x) & 0x1f) << 5)
#define GASRC_TIMING_CON1_GASRC2_OUT_MODE(x)	(((x) & 0x1f) << 10)
#define GASRC_TIMING_CON1_GASRC3_OUT_MODE(x)	(((x) & 0x1f) << 15)
#define GASRC_TIMING_CON1_GASRC0_OUT_MODE_MASK	GENMASK(4, 0)
#define GASRC_TIMING_CON1_GASRC1_OUT_MODE_MASK	GENMASK(9, 5)
#define GASRC_TIMING_CON1_GASRC2_OUT_MODE_MASK	GENMASK(14, 10)
#define GASRC_TIMING_CON1_GASRC3_OUT_MODE_MASK	GENMASK(19, 15)

#define BLOCK_DPIDLE_REG 0xb6c
#define BLOCK_DPIDLE_REG_BIT 31
#define BLOCK_DPIDLE_REG_MASK (1 << BLOCK_DPIDLE_REG_BIT)
#define BLOCK_DPIDLE_REG_BIT_ON  (1 << BLOCK_DPIDLE_REG_BIT)
#define BLOCK_DPIDLE_REG_BIT_OFF (0 << BLOCK_DPIDLE_REG_BIT)

#endif
