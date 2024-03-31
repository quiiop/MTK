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
 * Author: Min Guo <min.guo@mediatek.com>
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

#ifndef USB_PHY_H
#define USB_PHY_H

#include "system_rscs.h"
#include <reg_base.h>

/* APB Module ssusb_top_sif - USB3_SIF2_BASE */
#define SSUSB_SIFSLV_U2_PORT0_BASE        (USB_SIF_BASE)
#define SSUSB_SIFSLV_U2PORT0_MISC_BASE    (SSUSB_SIFSLV_U2_PORT0_BASE + 0x000)
#define SSUSB_SIFSLV_U2PORT0_U2FREQ_BASE  (SSUSB_SIFSLV_U2_PORT0_BASE + 0x100)
#define SSUSB_SIFSLV_U2PORT0_COM_BASE     (SSUSB_SIFSLV_U2_PORT0_BASE + 0x300)

/* tphy phy u3banks */
#define SSUSB_SIFSLV_U31_PORT0_BASE     (USB_SIF_BASE + 0x8000)
#define SSUSB_SIFSLV_SPLLC_BASE         (SSUSB_SIFSLV_U31_PORT0_BASE + 0x700)
#define SSUSB_SIFSLV_U3PHYD_BASE        (SSUSB_SIFSLV_U31_PORT0_BASE + 0x900)
#define SSUSB_SIFSLV_U3PHYA_BASE        (SSUSB_SIFSLV_U31_PORT0_BASE + 0xB00)
#define SSUSB_SIFSLV_U3PHYA_DA_BASE     (SSUSB_SIFSLV_U31_PORT0_BASE + 0xC00)

/* xsphy phy u3banks */
#define SSPXTP_SIFSLV_U31_PORT0_BASE    (USB_SIF_BASE + 0X3000)
#define SSPXTP_SIFSLV_PHYA_GLB          (SSPXTP_SIFSLV_U31_PORT0_BASE + 0x100)
#define SSPXTP_SIFSLV_DIG_LN_TOP        (SSPXTP_SIFSLV_U31_PORT0_BASE + 0x400)
#define SSPXTP_SIFSLV_DIG_LN_TX0        (SSPXTP_SIFSLV_U31_PORT0_BASE + 0x500)
#define SSPXTP_SIFSLV_DIG_LN_RX0        (SSPXTP_SIFSLV_U31_PORT0_BASE + 0x600)
#define SSPXTP_SIFSLV_DIG_LN_DAIF       (SSPXTP_SIFSLV_U31_PORT0_BASE + 0x700)
#define SSPXTP_SIFSLV_PHYA_LN           (SSPXTP_SIFSLV_U31_PORT0_BASE + 0x800)

/* referenecd from ssusb_USB20_PHY_regmap_com_T28.xls */
#define U3D_USBPHYACR0              (0x0000) /* bit 2~bit 30 */
#define U3D_USBPHYACR1              (0x0004)
#define U3D_USBPHYACR2              (0x0008) /* bit 0~ bit15 */
#define U3D_USBPHYACR4              (0x0010)
#define U3D_USBPHYACR5              (0x0014)
#define U3D_USBPHYACR6              (0x0018)
#define U3D_U2PHYACR3               (0x001c)
#define U3D_U2PHYACR4               (0x0020) /* bit8~ bit18 */
#define U3D_U2PHYAMON0              (0x0024)
#define U3D_U2PHYDCR0               (0x0060)
#define U3D_U2PHYDCR1               (0x0064)
#define U3D_U2PHYDTM0               (0x0068)
#define U3D_U2PHYDTM1               (0x006C)
#define U3D_U2PHYDMON0              (0x0070)
#define U3D_U2PHYDMON1              (0x0074)
#define U3D_U2PHYDMON2              (0x0078)
#define U3D_U2PHYDMON3              (0x007C)
#define U3D_U2PHYBC12C              (0x0080)
#define U3D_U2PHYBC12C1             (0x0084)
#define U3D_U2PHYREGFPPC            (0x00e0)
#define U3D_U2PHYVERSIONC           (0x00f0)
#define U3D_U2PHYREGFCOM            (0x00fc)

/* U3D_USBPHYACR0 */
#define RG_USB20_MPX_OUT_SEL               (0x7 << 28) /* 30:28 */
#define RG_USB20_TX_PH_ROT_SEL             (0x7 << 24) /* 26:24 */
#define RG_USB20_PLL_DIVEN                 (0x7 << 20) /* 22:20 */
#define RG_USB20_PLL_BR                    (0x1 << 18) /* 18:18 */
#define RG_USB20_PLL_BP                    (0x1 << 17) /* 17:17 */
#define RG_USB20_PLL_BLP                   (0x1 << 16) /* 16:16 */
#define RG_USB20_USBPLL_FORCE_ON           (0x1 << 15) /* 15:15 */
#define RG_USB20_PLL_FBDIV                 (0x7f << 8) /* 14:8 */
#define RG_USB20_PLL_PREDIV                (0x3 << 6) /* 7:6 */
#define RG_USB20_INTR_EN                   (0x1 << 5) /* 5:5 */
#define RG_USB20_REF_EN                    (0x1 << 4) /* 4:4 */
#define RG_USB20_BGR_DIV                   (0x3 << 2) /* 3:2 */
#define RG_SIFSLV_CHP_EN                   (0x1 << 1) /* 1:1 */
#define RG_SIFSLV_BGR_EN                   (0x1 << 0) /* 0:0 */

/* U3D_USBPHYACR1 */
#define RG_USB20_INTR_CAL                  (0x1f << 19) /* 23:19 */
#define RG_USB20_OTG_VBUSTH                (0x7 << 16) /* 18:16 */
#define RG_USB20_VRT_VREF_SEL              (0x7 << 12) /* 14:12 */
#define RG_USB20_TERM_VREF_SEL             (0x7 << 8) /* 10:8 */
#define RG_USB20_MPX_SEL                   (0xff << 0) /* 7:0 */

/* U3D_USBPHYACR2 */
#define RG_SIFSLV_MAC_BANDGAP_EN           (0x1 << 17) /* 17:17 */
#define RG_SIFSLV_MAC_CHOPPER_EN           (0x1 << 16) /* 16:16 */
#define RG_USB20_CLKREF_REV                (0xffff << 0) /* 15:0 */

/* U3D_USBPHYACR4 */
#define RG_USB20_DP_ABIST_SOURCE_EN        (0x1 << 31) /* 31:31 */
#define RG_USB20_DP_ABIST_SELE             (0xf << 24) /* 27:24 */
#define RG_USB20_ICUSB_EN                  (0x1 << 16) /* 16:16 */
#define RG_USB20_LS_CR                     (0x7 << 12) /* 14:12 */
#define RG_USB20_FS_CR                     (0x7 << 8) /* 10:8 */
#define RG_USB20_LS_SR                     (0x7 << 4) /* 6:4 */
#define RG_USB20_FS_SR                     (0x7 << 0) /* 2:0 */

/* U3D_USBPHYACR5 */
#define RG_USB20_DISC_FIT_EN               (0x1 << 28) /* 28:28 */
#define RG_USB20_INIT_SQ_EN_DG             (0x3 << 26) /* 27:26 */
#define RG_USB20_HSTX_TMODE_SEL            (0x3 << 24) /* 25:24 */
#define RG_USB20_SQD                       (0x3 << 22) /* 23:22 */
#define RG_USB20_DISCD                     (0x3 << 20) /* 21:20 */
#define RG_USB20_HSTX_TMODE_EN             (0x1 << 19) /* 19:19 */
#define RG_USB20_PHYD_MONEN                (0x1 << 18) /* 18:18 */
#define RG_USB20_INLPBK_EN                 (0x1 << 17) /* 17:17 */
#define RG_USB20_CHIRP_EN                  (0x1 << 16) /* 16:16 */
#define RG_USB20_HSTX_SRCAL_EN             (0x1 << 15) /* 15:15 */
#define RG_USB20_HSTX_SRCTRL               (0x7 << 12) /* 14:12 */
#define RG_USB20_HS_100U_U3_EN             (0x1 << 11) /* 11:11 */
#define RG_USB20_GBIAS_ENB                 (0x1 << 10) /* 10:10 */
#define RG_USB20_DM_ABIST_SOURCE_EN        (0x1 << 7) /* 7:7 */
#define RG_USB20_DM_ABIST_SELE             (0xf << 0) /* 3:0 */

/* U3D_USBPHYACR6 */
#define RG_USB20_ISO_EN                    (0x1 << 31) /* 31:31 */
#define RG_USB20_PHY_REV                   (0xef << 24) /* 31:24 */
#define RG_USB20_BC11_SW_EN                (0x1 << 23) /* 23:23 */
#define RG_USB20_SR_CLK_SEL                (0x1 << 22) /* 22:22 */
#define RG_USB20_OTG_VBUSCMP_EN            (0x1 << 20) /* 20:20 */
#define RG_USB20_OTG_ABIST_EN              (0x1 << 19) /* 19:19 */
#define RG_USB20_OTG_ABIST_SELE            (0x7 << 16) /* 18:16 */
#define RG_USB20_HSRX_MMODE_SELE           (0x3 << 12) /* 13:12 */
#define RG_USB20_HSRX_BIAS_EN_SEL          (0x3 << 9) /* 10:9 */
#define RG_USB20_HSRX_TMODE_EN             (0x1 << 8) /* 8:8 */
#define RG_USB20_DISCTH                    (0xf << 4) /* 7:4 */
#define RG_USB20_SQTH                      (0xf << 0) /* 3:0 */
#define RG_USB20_SQTH_VAL(x)               (0xf & (x))

/* U3D_U2PHYACR3 */
#define RG_USB20_HSTX_DBIST                (0xf << 28) /* 31:28 */
#define RG_USB20_HSTX_BIST_EN              (0x1 << 26) /* 26:26 */
#define RG_USB20_HSTX_I_EN_MODE            (0x3 << 24) /* 25:24 */
#define RG_USB20_USB11_TMODE_EN            (0x1 << 19) /* 19:19 */
#define RG_USB20_TMODE_FS_LS_TX_EN         (0x1 << 18) /* 18:18 */
#define RG_USB20_TMODE_FS_LS_RCV_EN        (0x1 << 17) /* 17:17 */
#define RG_USB20_TMODE_FS_LS_MODE          (0x1 << 16) /* 16:16 */
#define RG_USB20_HS_TERM_EN_MODE           (0x3 << 13) /* 14:13 */
#define RG_USB20_PUPD_BIST_EN              (0x1 << 12) /* 12:12 */
#define RG_USB20_EN_PU_DM                  (0x1 << 11) /* 11:11 */
#define RG_USB20_EN_PD_DM                  (0x1 << 10) /* 10:10 */
#define RG_USB20_EN_PU_DP                  (0x1 << 9) /* 9:9 */
#define RG_USB20_EN_PD_DP                  (0x1 << 8) /* 8:8 */

/* U3D_U2PHYACR4 */
#define RG_USB20_DP_100K_MODE              (0x1 << 18) /* 18:18 */
#define RG_USB20_DM_100K_EN                (0x1 << 17) /* 17:17 */
#define USB20_DP_100K_EN                   (0x1 << 16) /* 16:16 */
#define USB20_GPIO_DM_I                    (0x1 << 15) /* 15:15 */
#define USB20_GPIO_DP_I                    (0x1 << 14) /* 14:14 */
#define USB20_GPIO_DM_OE                   (0x1 << 13) /* 13:13 */
#define USB20_GPIO_DP_OE                   (0x1 << 12) /* 12:12 */
#define RG_USB20_GPIO_CTL                  (0x1 << 9) /* 9:9 */
#define USB20_GPIO_MODE                    (0x1 << 8) /* 8:8 */
#define RG_USB20_TX_BIAS_EN                (0x1 << 5) /* 5:5 */
#define RG_USB20_TX_VCMPDN_EN              (0x1 << 4) /* 4:4 */
#define RG_USB20_HS_SQ_EN_MODE             (0x3 << 2) /* 3:2 */
#define RG_USB20_HS_RCV_EN_MODE            (0x3 << 0) /* 1:0 */

/* U3D_U2PHYAMON0 */
#define RGO_USB20_GPIO_DM_O                (0x1 << 1) /* 1:1 */
#define RGO_USB20_GPIO_DP_O                (0x1 << 0) /* 0:0 */

/* U3D_U2PHYDCR0 */
#define RG_USB20_CDR_TST                   (0x3 << 30) /* 31:30 */
#define RG_USB20_GATED_ENB                 (0x1 << 29) /* 29:29 */
#define RG_USB20_TESTMODE                  (0x3 << 26) /* 27:26 */
#define RG_SIFSLV_USB20_PLL_STABLE         (0x1 << 25) /* 25:25 */
#define RG_SIFSLV_USB20_PLL_FORCE_ON       (0x1 << 24) /* 24:24 */
#define RG_USB20_PHYD_RESERVE              (0xffff << 8) /* 23:8 */
#define RG_USB20_EBTHRLD                   (0x1 << 7) /* 7:7 */
#define RG_USB20_EARLY_HSTX_I              (0x1 << 6) /* 6:6 */
#define RG_USB20_TX_TST                    (0x1 << 5) /* 5:5 */
#define RG_USB20_NEGEDGE_ENB               (0x1 << 4) /* 4:4 */
#define RG_USB20_CDR_FILT                  (0xf << 0) /* 3:0 */

/* U3D_U2PHYDCR1 */
#define RG_USB20_PROBE_SEL                 (0xff << 24) /* 31:24 */
#define RG_USB20_DRVVBUS                   (0x1 << 23) /* 23:23 */
#define RG_DEBUG_EN                        (0x1 << 22) /* 22:22 */
#define RG_USB20_OTG_PROBE                 (0x3 << 20) /* 21:20 */
#define RG_USB20_SW_PLLMODE                (0x3 << 18) /* 19:18 */
#define RG_USB20_BERTH                     (0x3 << 16) /* 17:16 */
#define RG_USB20_LBMODE                    (0x3 << 13) /* 14:13 */
#define RG_USB20_FORCE_TAP                 (0x1 << 12) /* 12:12 */
#define RG_USB20_TAPSEL                    (0xfff << 0) /* 11:0 */

/* U3D_U2PHYDTM0 */
#define RG_UART_MODE                       (0x3 << 30) /* 31:30 */
#define FORCE_UART_I                       (0x1 << 29) /* 29:29 */
#define FORCE_UART_BIAS_EN                 (0x1 << 28) /* 28:28 */
#define FORCE_UART_TX_OE                   (0x1 << 27) /* 27:27 */
#define FORCE_UART_EN                      (0x1 << 26) /* 26:26 */
#define FORCE_USB_CLKEN                    (0x1 << 25) /* 25:25 */
#define FORCE_DRVVBUS                      (0x1 << 24) /* 24:24 */
#define FORCE_DATAIN                       (0x1 << 23) /* 23:23 */
#define FORCE_TXVALID                      (0x1 << 22) /* 22:22 */
#define FORCE_DM_PULLDOWN                  (0x1 << 21) /* 21:21 */
#define FORCE_DP_PULLDOWN                  (0x1 << 20) /* 20:20 */
#define FORCE_XCVRSEL                      (0x1 << 19) /* 19:19 */
#define FORCE_SUSPENDM                     (0x1 << 18) /* 18:18 */
#define FORCE_TERMSEL                      (0x1 << 17) /* 17:17 */
#define FORCE_OPMODE                       (0x1 << 16) /* 16:16 */
#define UTMI_MUXSEL                        (0x1 << 15) /* 15:15 */
#define RG_RESET                           (0x1 << 14) /* 14:14 */
#define RG_DATAIN                          (0xf << 10) /* 13:10 */
#define RG_TXVALIDH                        (0x1 << 9) /* 9:9 */
#define RG_TXVALID                         (0x1 << 8) /* 8:8 */
#define RG_DMPULLDOWN                      (0x1 << 7) /* 7:7 */
#define RG_DPPULLDOWN                      (0x1 << 6) /* 6:6 */
#define RG_XCVRSEL                         (0x3 << 4) /* 5:4 */
#define RG_SUSPENDM                        (0x1 << 3) /* 3:3 */
#define RG_TERMSEL                         (0x1 << 2) /* 2:2 */
#define RG_OPMODE                          (0x3 << 0) /* 1:0 */
#define DTM0_PART_MASK \
        (FORCE_DATAIN | FORCE_DM_PULLDOWN | \
        FORCE_DP_PULLDOWN | FORCE_XCVRSEL | \
        FORCE_TERMSEL | RG_DMPULLDOWN | \
        RG_DPPULLDOWN | RG_TERMSEL)

/* U3D_U2PHYDTM1 */
#define RG_USB20_PRBS7_EN                  (0x1 << 31) /* 31:31 */
#define RG_USB20_PRBS7_BITCNT              (0x3f << 24) /* 29:24 */
#define RG_USB20_CLK48M_EN                 (0x1 << 23) /* 23:23 */
#define RG_USB20_CLK60M_EN                 (0x1 << 22) /* 22:22 */
#define RG_UART_I                          (0x1 << 19) /* 19:19 */
#define RG_UART_BIAS_EN                    (0x1 << 18) /* 18:18 */
#define RG_UART_TX_OE                      (0x1 << 17) /* 17:17 */
#define RG_UART_EN                         (0x1 << 16) /* 16:16 */
#define RG_IP_U2_PORT_POWER                (0x1 << 15) /* 15:15 */
#define FORCE_IP_U2_PORT_POWER             (0x1 << 14) /* 14:14 */
#define FORCE_VBUSVALID                    (0x1 << 13) /* 13:13 */
#define FORCE_SESSEND                      (0x1 << 12) /* 12:12 */
#define FORCE_BVALID                       (0x1 << 11) /* 11:11 */
#define FORCE_AVALID                       (0x1 << 10) /* 10:10 */
#define FORCE_IDDIG                        (0x1 << 9) /* 9:9 */
#define FORCE_IDPULLUP                     (0x1 << 8) /* 8:8 */
#define RG_VBUSVALID                       (0x1 << 5) /* 5:5 */
#define RG_SESSEND                         (0x1 << 4) /* 4:4 */
#define RG_BVALID                          (0x1 << 3) /* 3:3 */
#define RG_AVALID                          (0x1 << 2) /* 2:2 */
#define RG_IDDIG                           (0x1 << 1) /* 1:1 */
#define RG_IDPULLUP                        (0x1 << 0) /* 0:0 */

/* U3D_U2PHYDMON0 */
#define RG_USB20_PRBS7_BERTH               (0xff << 0) /* 7:0 */

/* U3D_U2PHYDMON1 */
#define USB20_UART_O                       (0x1 << 31) /* 31:31 */
#define RGO_USB20_LB_PASS                  (0x1 << 30) /* 30:30 */
#define RGO_USB20_LB_DONE                  (0x1 << 29) /* 29:29 */
#define AD_USB20_BVALID                    (0x1 << 28) /* 28:28 */
#define USB20_IDDIG                        (0x1 << 27) /* 27:27 */
#define AD_USB20_VBUSVALID                 (0x1 << 26) /* 26:26 */
#define AD_USB20_SESSEND                   (0x1 << 25) /* 25:25 */
#define AD_USB20_AVALID                    (0x1 << 24) /* 24:24 */
#define USB20_LINE_STATE                   (0x3 << 22) /* 23:22 */
#define USB20_HST_DISCON                   (0x1 << 21) /* 21:21 */
#define USB20_TX_READY                     (0x1 << 20) /* 20:20 */
#define USB20_RX_ERROR                     (0x1 << 19) /* 19:19 */
#define USB20_RX_ACTIVE                    (0x1 << 18) /* 18:18 */
#define USB20_RX_VALIDH                    (0x1 << 17) /* 17:17 */
#define USB20_RX_VALID                     (0x1 << 16) /* 16:16 */
#define USB20_DATA_OUT                     (0xffff << 0) /* 15:0 */

/* U3D_U2PHYDMON2 */
#define RGO_TXVALID_CNT                    (0xff << 24) /* 31:24 */
#define RGO_RXACTIVE_CNT                   (0xff << 16) /* 23:16 */
#define RGO_USB20_LB_BERCNT                (0xff << 8) /* 15:8 */
#define USB20_PROBE_OUT                    (0xff << 0) /* 7:0 */

/* U3D_U2PHYDMON3 */
#define RGO_USB20_PRBS7_ERRCNT             (0xffff << 16) /* 31:16 */
#define RGO_USB20_PRBS7_DONE               (0x1 << 3) /* 3:3 */
#define RGO_USB20_PRBS7_LOCK               (0x1 << 2) /* 2:2 */
#define RGO_USB20_PRBS7_PASS               (0x1 << 1) /* 1:1 */
#define RGO_USB20_PRBS7_PASSTH             (0x1 << 0) /* 0:0 */

/* U3D_U2PHYBC12C */
#define RG_SIFSLV_CHGDT_DEGLCH_CNT         (0xf << 28) /* 31:28 */
#define RG_SIFSLV_CHGDT_CTRL_CNT           (0xf << 24) /* 27:24 */
#define RG_SIFSLV_CHGDT_FORCE_MODE         (0x1 << 16) /* 16:16 */
#define RG_CHGDT_ISRC_LEV                  (0x3 << 14) /* 15:14 */
#define RG_CHGDT_VDATSRC                   (0x1 << 13) /* 13:13 */
#define RG_CHGDT_BGVREF_SEL                (0x7 << 10) /* 12:10 */
#define RG_CHGDT_RDVREF_SEL                (0x3 << 8) /* 9:8 */
#define RG_CHGDT_ISRC_DP                   (0x1 << 7) /* 7:7 */
#define RG_SIFSLV_CHGDT_OPOUT_DM           (0x1 << 6) /* 6:6 */
#define RG_CHGDT_VDAT_DM                   (0x1 << 5) /* 5:5 */
#define RG_CHGDT_OPOUT_DP                  (0x1 << 4) /* 4:4 */
#define RG_SIFSLV_CHGDT_VDAT_DP            (0x1 << 3) /* 3:3 */
#define RG_SIFSLV_CHGDT_COMP_EN            (0x1 << 2) /* 2:2 */
#define RG_SIFSLV_CHGDT_OPDRV_EN           (0x1 << 1) /* 1:1 */
#define RG_CHGDT_EN                        (0x1 << 0) /* 0:0 */

/* U3D_U2PHYBC12C1 */
#define RG_CHGDT_REV                       (0xff << 0) /* 7:0 */

/* U3D_REGFPPC */
#define USB11_OTG_REG                      (0x1 << 4) /* 4:4 */
#define USB20_OTG_REG                      (0x1 << 3) /* 3:3 */
#define CHGDT_REG                          (0x1 << 2) /* 2:2 */
#define USB11_REG                          (0x1 << 1) /* 1:1 */
#define USB20_REG                          (0x1 << 0) /* 0:0 */

/* U3D_VERSIONC */
#define VERSION_CODE_REGFILE               (0xff << 24) /* 31:24 */
#define USB11_VERSION_CODE                 (0xff << 16) /* 23:16 */
#define VERSION_CODE_ANA                   (0xff << 8) /* 15:8 */
#define VERSION_CODE_DIG                   (0xff << 0) /* 7:0 */

/* U3D_REGFCOM */
#define RG_PAGE                            (0xff << 24) /* 31:24 */
#define I2C_MODE                           (0x1 << 16) /* 16:16 */

#define U3D_U2PHYDEV_MASK   (RG_IDDIG | RG_AVALID | \
    RG_BVALID | RG_VBUSVALID)

#define U3D_U2PHYFRCDEV_MASK (FORCE_IDDIG | FORCE_AVALID | \
    FORCE_BVALID | FORCE_SESSEND | FORCE_VBUSVALID)

#define RG_XCVRSEL_VAL(x)       ((0x3 & (x))  <<  4)
#define RG_DATAIN_VAL(x)        ((0xf & (x))  <<  10)

#define phy_err(x...) printf("[USB][PHY] " x)
#define phy_crit(x...) printf("[USB][PHY] " x)

#define DBG_USB_PHY 0
#if DBG_USB_PHY
#define phy_debug(x...) printf("[USB][PHY] " x)
#else /* #if DBG_USB_PHY */
#define phy_debug(x...) do {} while (0)
#endif /* #if DBG_USB_PHY */

static inline void phy_setbits(void *base, u32 offset, u32 bits)
{
    void *addr = base + offset;
    u32 tmp = readl(addr);

    writel((tmp | (bits)), addr);
}

static inline void phy_clrbits(void *base, u32 offset, u32 bits)
{
    void *addr = base + offset;
    u32 tmp = readl(addr);

    writel((tmp & ~(bits)), addr);
}

enum mtk_phy_type {
    MTK_TPHY = 0,
    MTK_XSHY,
};

enum phy_mode {
    USB_MODE_UNKNOWN,
    USB_MODE_HOST,
    USB_MODE_PERIPHERAL,
    USB_MODE_OTG,
};

struct u2phy_banks {
    void *misc;
    void *fmreg;
    void *com;
};

struct tphy_banks {
    void *spllc;
    void *phyd; /* include u3phyd_bank2 */
    void *phya; /* include u3phya_da */
};

struct xsphy_banks {
    void *phya_glb;
    void *dig_ln_top;
    void *dig_ln_tx0;
    void *dig_ln_rx0;
    void *dig_ln_daif;
    void *phya_ln;
};

struct u3phy_banks {
    union {
        struct tphy_banks tphy;
        struct xsphy_banks xsphy;
    };
};

struct mtk_phy_instance {
    void *phy_base;
    enum mtk_phy_type type;
    u8 u2_phy_num;
    u8 u3_phy_num;

    struct u2phy_banks **u2_banks;
    struct u3phy_banks **u3_banks;
};

int u2_phy_instance_init(struct mtk_phy_instance *phy);
int u3_phy_instance_init(struct mtk_phy_instance *phy);
int u2_phy_set_mode(struct mtk_phy_instance *phy, enum phy_mode mode);
int u2_phy_instance_deinit(struct mtk_phy_instance *phy);
int u3_phy_instance_deinit(struct mtk_phy_instance *phy);
#endif /* #ifndef USB_PHY_H */
