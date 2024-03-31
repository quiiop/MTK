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
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
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


#ifndef __MTU3_HW_REG_H__
#define __MTU3_HW_REG_H__

#include "system_rscs.h"
#include <reg_base.h>

#define SSUSB_DEV_BASE              (USB_BASE + 0x1000)
#define SSUSB_SIFSLV_IPPC_BASE          (USB_BASE + 0x3e00)
#define SSUSB_EPCTL_CSR_BASE            (0x0800)
#define SSUSB_USB3_MAC_CSR_BASE         (0x1400)
#define SSUSB_USB3_SYS_CSR_BASE         (0x1600)
#define SSUSB_USB2_CSR_BASE         (0x2400)


#define BITS_PER_LONG 32
#ifndef BIT
#define BIT(bit) (1UL << (bit))
#endif /* #ifndef BIT */
#ifndef GENMASK
#define GENMASK(h, l) \
    ((unsigned int)(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h)))))
#endif /* #ifndef GENMASK */

/* SSUSB_DEV REGISTER DEFINITION */
#define U3D_LV1ISR      (0x0000)
#define U3D_LV1IER      (0x0004)
#define U3D_LV1IESR     (0x0008)
#define U3D_LV1IECR     (0x000C)

#define U3D_EPISR       (0x0080)
#define U3D_EPIER       (0x0084)
#define U3D_EPIESR      (0x0088)
#define U3D_EPIECR      (0x008C)

#define U3D_EP0CSR      (0x0100)
#define U3D_RXCOUNT0    (0x0108)
#define U3D_RESERVED    (0x010C)
#define U3D_TX1CSR0     (0x0110)
#define U3D_TX1CSR1     (0x0114)
#define U3D_TX1CSR2     (0x0118)

#define U3D_RX1CSR0     (0x0210)
#define U3D_RX1CSR1     (0x0214)
#define U3D_RX1CSR2     (0x0218)
#define U3D_RX1CSR3     (0x021C)

#define U3D_FIFO0       (0x0300)

#define U3D_QCR0        (0x0400)
#define U3D_QCR1        (0x0404)
#define U3D_QCR2        (0x0408)
#define U3D_QCR3        (0x040C)

#define U3D_TXQCSR1     (0x0510)
#define U3D_TXQSAR1     (0x0514)
#define U3D_TXQCPR1     (0x0518)

#define U3D_RXQCSR1     (0x0610)
#define U3D_RXQSAR1     (0x0614)
#define U3D_RXQCPR1     (0x0618)
#define U3D_RXQLDPR1    (0x061C)

#define U3D_QISAR0      (0x0700)
#define U3D_QIER0       (0x0704)
#define U3D_QIESR0      (0x0708)
#define U3D_QIECR0      (0x070C)
#define U3D_QISAR1      (0x0710)
#define U3D_QIER1       (0x0714)
#define U3D_QIESR1      (0x0718)
#define U3D_QIECR1      (0x071C)

#define U3D_TQERRIR0        (0x0780)
#define U3D_TQERRIER0       (0x0784)
#define U3D_TQERRIESR0      (0x0788)
#define U3D_TQERRIECR0      (0x078C)
#define U3D_RQERRIR0        (0x07C0)
#define U3D_RQERRIER0       (0x07C4)
#define U3D_RQERRIESR0      (0x07C8)
#define U3D_RQERRIECR0      (0x07CC)
#define U3D_RQERRIR1        (0x07D0)
#define U3D_RQERRIER1       (0x07D4)
#define U3D_RQERRIESR1      (0x07D8)
#define U3D_RQERRIECR1      (0x07DC)

#define U3D_CAP_EP0FFSZ     (0x0C04)
#define U3D_CAP_EPNTXFFSZ   (0x0C08)
#define U3D_CAP_EPNRXFFSZ   (0x0C0C)
#define U3D_CAP_EPINFO      (0x0C10)
#define U3D_MISC_CTRL       (0x0C84)

/* SSUSB_DEV FIELD DEFINITION */
/* U3D_LV1ISR */
#define EP_CTRL_INTR    BIT(5)
#define MAC2_INTR       BIT(4)
#define DMA_INTR        BIT(3)
#define MAC3_INTR       BIT(2)
#define QMU_INTR        BIT(1)
#define BMU_INTR        BIT(0)

/* U3D_LV1IECR */
#define LV1IECR_MSK     GENMASK(31, 0)

/* U3D_EPISR */
#define EP_RXISR_MSK    GENMASK(31, 17)
#define EP_RXISR(x)     (BIT(16) << (x))
#define EP_TXISR_MSK    GENMASK(15, 1)
#define EP_TXISR(x)     (BIT(0) << (x))
#define EP_EP0ISR       BIT(0)

/* U3D_EP0CSR */
#define EP0_AUTOCLEAR   BIT(30)
#define EP0_AUTOSET     BIT(29)
#define EP0_DMAREQEN    BIT(28)
#define EP0_SENDSTALL   BIT(25)
#define EP0_FIFOFULL    BIT(23)
#define EP0_SENTSTALL   BIT(22)
#define EP0_DPHTX       BIT(20)
#define EP0_DATAEND     BIT(19)
#define EP0_TXPKTRDY        BIT(18)
#define EP0_SETUPPKTRDY     BIT(17)
#define EP0_RXPKTRDY        BIT(16)
#define EP0_MAXPKTSZ_MSK    GENMASK(9, 0)
#define EP0_MAXPKTSZ(x)     ((x) & EP0_MAXPKTSZ_MSK)
#define EP0_W1C_BITS    (~(EP0_RXPKTRDY | EP0_SETUPPKTRDY | EP0_SENTSTALL))

/* U3D_TX1CSR0 */
#define TX_AUTOSET      BIT(30)
#define TX_DMAREQEN     BIT(29)
#define TX_FIFOFULL     BIT(25)
#define TX_FIFOEMPTY    BIT(24)
#define TX_SENTSTALL    BIT(22)
#define TX_SENDSTALL    BIT(21)
#define TX_TXPKTRDY     BIT(16)
#define TX_TXMAXPKTSZ_MSK   GENMASK(10, 0)
#define TX_TXMAXPKTSZ(x)    ((x) & TX_TXMAXPKTSZ_MSK)
#define TX_W1C_BITS     (~(TX_SENTSTALL))

/* U3D_TX1CSR1 */
#define TX_MULT(x)      (((x) & 0x3) << 22)
#define TX_MAX_PKT(x)   (((x) & 0x3f) << 16)
#define TX_SLOT(x)      (((x) & 0x3f) << 8)
#define TX_TYPE(x)      (((x) & 0x3) << 4)
#define TX_SS_BURST(x)      (((x) & 0xf) << 0)

/* for TX_TYPE & RX_TYPE */
#define TYPE_BULK       (0x0)
#define TYPE_INT        (0x1)
#define TYPE_ISO        (0x2)
#define TYPE_MSK        (0x3)

/* U3D_TX1CSR2 */
#define TX_BINTERVAL(x)     (((x) & 0xff) << 24)
#define TX_FIFOSEGSIZE(x)   (((x) & 0xf) << 16)
#define TX_FIFOADDR(x)      (((x) & 0x1fff) << 0)

/* U3D_RX1CSR0 */
#define RX_AUTOCLEAR    BIT(30)
#define RX_DMAREQEN     BIT(29)
#define RX_SENTSTALL    BIT(22)
#define RX_SENDSTALL    BIT(21)
#define RX_RXPKTRDY     BIT(16)
#define RX_RXMAXPKTSZ_MSK   GENMASK(10, 0)
#define RX_RXMAXPKTSZ(x)    ((x) & RX_RXMAXPKTSZ_MSK)
#define RX_W1C_BITS     (~(RX_SENTSTALL | RX_RXPKTRDY))

/* U3D_RX1CSR1 */
#define RX_MULT(x)      (((x) & 0x3) << 22)
#define RX_MAX_PKT(x)   (((x) & 0x3f) << 16)
#define RX_SLOT(x)      (((x) & 0x3f) << 8)
#define RX_TYPE(x)      (((x) & 0x3) << 4)
#define RX_SS_BURST(x)  (((x) & 0xf) << 0)

/* U3D_RX1CSR2 */
#define RX_BINTERVAL(x)     (((x) & 0xff) << 24)
#define RX_FIFOSEGSIZE(x)   (((x) & 0xf) << 16)
#define RX_FIFOADDR(x)      (((x) & 0x1fff) << 0)

/* U3D_RX1CSR3 */
#define EP_RX_COUNT(x)      (((x) >> 16) & 0x7ff)

/* U3D_FIFO: ep(0-15)*/
#define U3D_FIFO(x)         (U3D_FIFO0 + ((x) * 0x10))
#define USB_FIFO(x)         (U3D_FIFO(x))


/* U3D_CAP_EPINFO */
#define CAP_RX_EP_NUM(x)    (((x) >> 8) & 0x1f)
#define CAP_TX_EP_NUM(x)    ((x) & 0x1f)

/* U3D_MISC_CTRL */
#define VBUS_ON         BIT(1)
#define VBUS_FRC_EN     BIT(0)


/* SSUSB_EPCTL_CSR REGISTER DEFINITION */
#define U3D_DEVICE_CONF     (SSUSB_EPCTL_CSR_BASE + 0x0000)
#define U3D_EP_RST          (SSUSB_EPCTL_CSR_BASE + 0x0004)

#define U3D_DEV_LINK_INTR_ENABLE    (SSUSB_EPCTL_CSR_BASE + 0x0050)
#define U3D_DEV_LINK_INTR       (SSUSB_EPCTL_CSR_BASE + 0x0054)

/* SSUSB_EPCTL_CSR FIELD DEFINITION */
/* U3D_DEVICE_CONF */
#define DEV_ADDR_MSK    GENMASK(30, 24)
#define DEV_ADDR(x)     ((0x7f & (x)) << 24)
#define HW_USB2_3_SEL       BIT(18)
#define SW_USB2_3_SEL_EN    BIT(17)
#define SW_USB2_3_SEL       BIT(16)
#define SSUSB_DEV_SPEED(x)  ((x) & 0x7)

/* U3D_EP_RST */
#define EP1_IN_RST      BIT(17)
#define EP1_OUT_RST     BIT(1)
#define EP_RST(is_in, epnum)    (((is_in) ? BIT(16) : BIT(0)) << (epnum))
#define EP0_RST         BIT(0)

/* U3D_DEV_LINK_INTR_ENABLE */
/* U3D_DEV_LINK_INTR */
#define SSUSB_DEV_SPEED_CHG_INTR    BIT(0)


/* SSUSB_USB3_MAC_CSR REGISTER DEFINITION */
#define U3D_LTSSM_CTRL      (SSUSB_USB3_MAC_CSR_BASE + 0x0010)
#define U3D_USB3_CONFIG     (SSUSB_USB3_MAC_CSR_BASE + 0x001C)

#define U3D_LTSSM_INTR_ENABLE   (SSUSB_USB3_MAC_CSR_BASE + 0x013C)
#define U3D_LTSSM_INTR      (SSUSB_USB3_MAC_CSR_BASE + 0x0140)

/* SSUSB_USB3_MAC_CSR FIELD DEFINITION */
/* U3D_LTSSM_CTRL */
#define FORCE_POLLING_FAIL  BIT(4)
#define FORCE_RXDETECT_FAIL BIT(3)
#define SOFT_U3_EXIT_EN     BIT(2)
#define COMPLIANCE_EN       BIT(1)
#define U1_GO_U2_EN     BIT(0)

/* U3D_USB3_CONFIG */
#define USB3_EN         BIT(0)

/* U3D_LTSSM_INTR_ENABLE */
/* U3D_LTSSM_INTR */
#define U3_RESUME_INTR      BIT(18)
#define U3_LFPS_TMOUT_INTR  BIT(17)
#define VBUS_FALL_INTR      BIT(16)
#define VBUS_RISE_INTR      BIT(15)
#define RXDET_SUCCESS_INTR  BIT(14)
#define EXIT_U3_INTR        BIT(13)
#define EXIT_U2_INTR        BIT(12)
#define EXIT_U1_INTR        BIT(11)
#define ENTER_U3_INTR       BIT(10)
#define ENTER_U2_INTR       BIT(9)
#define ENTER_U1_INTR       BIT(8)
#define ENTER_U0_INTR       BIT(7)
#define RECOVERY_INTR       BIT(6)
#define WARM_RST_INTR       BIT(5)
#define HOT_RST_INTR        BIT(4)
#define LOOPBACK_INTR       BIT(3)
#define COMPLIANCE_INTR     BIT(2)
#define SS_DISABLE_INTR     BIT(1)
#define SS_INACTIVE_INTR    BIT(0)

/* SSUSB_USB3_SYS_CSR REGISTER DEFINITION */
#define U3D_LINK_UX_INACT_TIMER (SSUSB_USB3_SYS_CSR_BASE + 0x020C)
#define U3D_LINK_POWER_CONTROL  (SSUSB_USB3_SYS_CSR_BASE + 0x0210)
#define U3D_LINK_ERR_COUNT      (SSUSB_USB3_SYS_CSR_BASE + 0x0214)

/* SSUSB_USB3_SYS_CSR FIELD DEFINITION */
/* U3D_LINK_UX_INACT_TIMER */
#define DEV_U2_INACT_TIMEOUT_MSK    GENMASK(23, 16)
#define DEV_U2_INACT_TIMEOUT_VALUE(x)   (((x) & 0xff) << 16)
#define U2_INACT_TIMEOUT_MSK        GENMASK(15, 8)
#define U1_INACT_TIMEOUT_MSK        GENMASK(7, 0)
#define U1_INACT_TIMEOUT_VALUE(x)   ((x) & 0xff)

/* U3D_LINK_POWER_CONTROL */
#define SW_U2_ACCEPT_ENABLE     BIT(9)
#define SW_U1_ACCEPT_ENABLE     BIT(8)
#define UX_EXIT         BIT(5)
#define LGO_U3          BIT(4)
#define LGO_U2          BIT(3)
#define LGO_U1          BIT(2)
#define SW_U2_REQUEST_ENABLE    BIT(1)
#define SW_U1_REQUEST_ENABLE    BIT(0)

/* U3D_LINK_ERR_COUNT */
#define CLR_LINK_ERR_CNT    BIT(16)
#define LINK_ERROR_COUNT    GENMASK(15, 0)

/* SSUSB_USB2_CSR REGISTER DEFINITION */
#define U3D_POWER_MANAGEMENT        (SSUSB_USB2_CSR_BASE + 0x0004)
#define U3D_DEVICE_CONTROL          (SSUSB_USB2_CSR_BASE + 0x000C)
#define U3D_USB2_TEST_MODE          (SSUSB_USB2_CSR_BASE + 0x0014)
#define U3D_COMMON_USB_INTR_ENABLE  (SSUSB_USB2_CSR_BASE + 0x0018)
#define U3D_COMMON_USB_INTR         (SSUSB_USB2_CSR_BASE + 0x001C)
#define U3D_LINK_RESET_INFO         (SSUSB_USB2_CSR_BASE + 0x0024)
#define U3D_USB20_FRAME_NUM         (SSUSB_USB2_CSR_BASE + 0x003C)
#define U3D_USB20_LPM_PARAMETER     (SSUSB_USB2_CSR_BASE + 0x0044)
#define U3D_USB20_MISC_CONTROL      (SSUSB_USB2_CSR_BASE + 0x004C)

/* SSUSB_USB2_CSR FIELD DEFINITION */
/* U3D_POWER_MANAGEMENT */
#define LPM_BESL_STALL      BIT(14)
#define LPM_BESLD_STALL     BIT(13)
#define LPM_RWP         BIT(11)
#define LPM_HRWE        BIT(10)
#define LPM_MODE(x)     (((x) & 0x3) << 8)
#define ISO_UPDATE      BIT(7)
#define SOFT_CONN       BIT(6)
#define HS_ENABLE       BIT(5)
#define RESUME          BIT(2)
#define SUSPENDM_ENABLE     BIT(0)

/* U3D_DEVICE_CONTROL */
#define DC_HOSTREQ      BIT(1)
#define DC_SESSION      BIT(0)

/* U3D_USB2_TEST_MODE */
#define U2U3_AUTO_SWITCH    BIT(10)
#define LPM_FORCE_STALL     BIT(8)
#define FIFO_ACCESS         BIT(6)
#define FORCE_FS            BIT(5)
#define FORCE_HS            BIT(4)
#define TEST_PACKET_MODE    BIT(3)
#define TEST_K_MODE         BIT(2)
#define TEST_J_MODE         BIT(1)
#define TEST_SE0_NAK_MODE   BIT(0)

/* U3D_COMMON_USB_INTR_ENABLE */
/* U3D_COMMON_USB_INTR */
#define LPM_RESUME_INTR BIT(9)
#define LPM_INTR        BIT(8)
#define DISCONN_INTR    BIT(5)
#define CONN_INTR       BIT(4)
#define SOF_INTR        BIT(3)
#define RESET_INTR      BIT(2)
#define RESUME_INTR     BIT(1)
#define SUSPEND_INTR    BIT(0)

/* U3D_LINK_RESET_INFO */
#define WTCHRP_MSK      GENMASK(19, 16)

/* U3D_USB20_LPM_PARAMETER */
#define LPM_BESLCK_U3(x)    (((x) & 0xf) << 12)
#define LPM_BESLCK(x)       (((x) & 0xf) << 8)
#define LPM_BESLDCK(x)      (((x) & 0xf) << 4)
#define LPM_BESL            GENMASK(3, 0)

/* U3D_USB20_MISC_CONTROL */
#define LPM_U3_ACK_EN       BIT(0)

/* SSUSB_SIFSLV_IPPC REGISTER DEFINITION */
#define U3D_SSUSB_IP_PW_CTRL0                     (0x0000)
#define U3D_SSUSB_IP_PW_CTRL1                     (0x0004)
#define U3D_SSUSB_IP_PW_CTRL2                     (0x0008)
#define U3D_SSUSB_IP_PW_CTRL3                     (0x000C)
#define U3D_SSUSB_IP_PW_STS1                      (0x0010)
#define U3D_SSUSB_IP_PW_STS2                      (0x0014)
#define U3D_SSUSB_IP_MAC_CAP                      (0x0020)
#define U3D_SSUSB_IP_XHCI_CAP                     (0x0024)
#define U3D_SSUSB_IP_DEV_CAP                      (0x0028)
#define U3D_SSUSB_U3_CTRL_0P                      (0x0030)
#define U3D_SSUSB_U3_CTRL_1P                      (0x0038)
#define U3D_SSUSB_U3_CTRL_2P                      (0x0040)
#define U3D_SSUSB_U3_CTRL_3P                      (0x0048)
#define U3D_SSUSB_U2_CTRL_0P                      (0x0050)
#define U3D_SSUSB_U2_CTRL_1P                      (0x0058)
#define U3D_SSUSB_U2_CTRL_2P                      (0x0060)
#define U3D_SSUSB_U2_CTRL_3P                      (0x0068)
#define U3D_SSUSB_U2_CTRL_4P                      (0x0070)
#define U3D_SSUSB_U2_CTRL_5P                      (0x0078)
#define U3D_SSUSB_U2_PHY_PLL                      (0x007C)
#define U3D_SSUSB_DMA_CTRL                        (0x0080)
#define U3D_SSUSB_MAC_CK_CTRL                     (0x0084)
#define U3D_SSUSB_CSR_CK_CTRL                     (0x0088)
#define U3D_SSUSB_REF_CK_CTRL                     (0x008C)
#define U3D_SSUSB_XHCI_CK_CTRL                    (0x0090)
#define U3D_SSUSB_XHCI_RST_CTRL                   (0x0094)
#define U3D_SSUSB_DEV_RST_CTRL                    (0x0098)
#define U3D_SSUSB_SYS_CK_CTRL                     (0x009C)
#define U3D_SSUSB_HW_ID                           (0x00A0)
#define U3D_SSUSB_HW_SUB_ID                       (0x00A4)
#define U3D_SSUSB_PRB_CTRL0                       (0x00B0)
#define U3D_SSUSB_PRB_CTRL1                       (0x00B4)
#define U3D_SSUSB_PRB_CTRL2                       (0x00B8)
#define U3D_SSUSB_PRB_CTRL3                       (0x00BC)
#define U3D_SSUSB_PRB_CTRL4                       (0x00C0)
#define U3D_SSUSB_PRB_CTRL5                       (0x00C4)
#define U3D_SSUSB_IP_SPARE0                       (0x00C8)
#define U3D_SSUSB_IP_SPARE1                       (0x00CC)
#define U3D_SSUSB_FPGA_I2C_OUT_0P                 (0x00D0)
#define U3D_SSUSB_FPGA_I2C_IN_0P                  (0x00D4)
#define U3D_SSUSB_FPGA_I2C_OUT_1P                 (0x00D8)
#define U3D_SSUSB_FPGA_I2C_IN_1P                  (0x00DC)
#define U3D_SSUSB_FPGA_I2C_OUT_2P                 (0x00E0)
#define U3D_SSUSB_FPGA_I2C_IN_2P                  (0x00E4)
#define U3D_SSUSB_FPGA_I2C_OUT_3P                 (0x00E8)
#define U3D_SSUSB_FPGA_I2C_IN_3P                  (0x00EC)
#define U3D_SSUSB_FPGA_I2C_OUT_4P                 (0x00F0)
#define U3D_SSUSB_FPGA_I2C_IN_4P                  (0x00F4)
#define U3D_SSUSB_IP_SLV_TMOUT                    (0x00F8)


/* SSUSB_SIFSLV_IPPC FIELD DEFINITION */
/* U3D_SSUSB_IP_PW_CTRL0 */
#define SSUSB_IP_SW_RST         BIT(0)

/* U3D_SSUSB_IP_PW_CTRL1 */
#define SSUSB_IP_HOST_PDN       BIT(0)

/* U3D_SSUSB_IP_PW_CTRL2 */
#define SSUSB_IP_DEV_PDN        BIT(0)

/* U3D_SSUSB_IP_PW_CTRL3 */
#define SSUSB_IP_PCIE_PDN       BIT(0)

/* U3D_SSUSB_IP_PW_STS1 */
#define SSUSB_IP_SLEEP_STS      BIT(30)
#define SSUSB_U3_MAC_RST_B_STS  BIT(16)
#define SSUSB_XHCI_RST_B_STS    BIT(11)
#define SSUSB_SYS125_RST_B_STS  BIT(10)
#define SSUSB_REF_RST_B_STS     BIT(8)
#define SSUSB_SYSPLL_STABLE     BIT(0)

/* U3D_SSUSB_IP_PW_STS2 */
#define SSUSB_U2_MAC_SYS_RST_B_STS  BIT(0)

/* U3D_SSUSB_OTG_STS */
#define SSUSB_VBUS_VALID        BIT(9)

/* U3D_SSUSB_OTG_STS_CLR */
#define SSUSB_VBUS_INTR_CLR     BIT(6)

/* U3D_SSUSB_IP_XHCI_CAP */
#define SSUSB_IP_XHCI_U2_PORT_NUM(x)    (((x) >> 8) & 0xff)
#define SSUSB_IP_XHCI_U3_PORT_NUM(x)    ((x) & 0xff)

/* U3D_SSUSB_IP_DEV_CAP */
#define SSUSB_IP_DEV_U3_PORT_NUM(x) ((x) & 0xff)

/* U3D_SSUSB_OTG_INT_EN */
#define SSUSB_VBUS_CHG_INT_A_EN     BIT(7)
#define SSUSB_VBUS_CHG_INT_B_EN     BIT(6)

/* U3D_SSUSB_U3_CTRL_0P */
#define SSUSB_U3_PORT_SSP_SPEED BIT(9)
#define SSUSB_U3_PORT_DUAL_MODE BIT(7)
#define SSUSB_U3_PORT_HOST_SEL  BIT(2)
#define SSUSB_U3_PORT_PDN       BIT(1)
#define SSUSB_U3_PORT_DIS       BIT(0)

/* U3D_SSUSB_U2_CTRL_0P */
#define SSUSB_U2_PORT_RG_IDDIG      BIT(12)
#define SSUSB_U2_PORT_FORCE_IDDIG   BIT(11)
#define SSUSB_U2_PORT_VBUSVALID BIT(9)
#define SSUSB_U2_PORT_OTG_SEL   BIT(7)
#define SSUSB_U2_PORT_HOST  BIT(2)
#define SSUSB_U2_PORT_PDN       BIT(1)
#define SSUSB_U2_PORT_DIS       BIT(0)
#define SSUSB_U2_PORT_HOST_SEL  (SSUSB_U2_PORT_VBUSVALID | SSUSB_U2_PORT_HOST)

#define SSUSB_U3_CTRL(p)    (U3D_SSUSB_U3_CTRL_0P + ((p) * 0x08))
#define SSUSB_U2_CTRL(p)    (U3D_SSUSB_U2_CTRL_0P + ((p) * 0x08))

/* U3D_SSUSB_DEV_RST_CTRL */
#define SSUSB_DEV_SW_RST        BIT(0)

static inline u32 ssusb_readl(void *base, u32 offset)
{
    return readl(base + offset);
}

static inline void ssusb_writel(void *base, u32 offset, u32 data)
{
    writel(data, base + offset);
}

static inline void ssusb_setbits(void *base, u32 offset, u32 bits)
{
    void *addr = base + offset;
    u32 tmp = readl(addr);

    writel((tmp | (bits)), addr);
}

static inline void ssusb_clrbits(void *base, u32 offset, u32 bits)
{
    void *addr = base + offset;
    u32 tmp = readl(addr);

    writel((tmp & ~(bits)), addr);
}

#endif /* #ifndef __MTU3_HW_REG_H__ */
