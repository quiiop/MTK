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

#include <hal_gpt.h>
#include <stdio.h>

#include "usb_phy.h"

int u2_phy_instance_init(struct mtk_phy_instance *phy)
{
    void *com;
    int index;
    u32 tmp;

    if (!phy) {
        phy_err("%s: phy is null\n", __func__);
        return -1;
    }

    for (index = 0; index < phy->u2_phy_num; index++) {
        com = phy->u2_banks[index]->com;
        phy_debug("%s: phy[%d]:%p\n", __func__, index, com);

        /* init */
        /* switch to USB function, and enable usb pll */
        tmp = readl(com + U3D_U2PHYDTM0);
        tmp &= ~(FORCE_UART_EN | FORCE_SUSPENDM);
        tmp |= RG_XCVRSEL_VAL(1) | RG_DATAIN_VAL(0);
        writel(tmp, com + U3D_U2PHYDTM0);

        tmp = readl(com + U3D_U2PHYDTM1);
        tmp &= ~RG_UART_EN;
        writel(tmp, com + U3D_U2PHYDTM1);

        tmp = readl(com + U3D_USBPHYACR0);
        tmp |= RG_USB20_INTR_EN;
        writel(tmp, com + U3D_USBPHYACR0);

        /* disable switch 100uA current to SSUSB */
        tmp = readl(com + U3D_USBPHYACR5);
        tmp &= ~RG_USB20_HS_100U_U3_EN;
        writel(tmp, com + U3D_USBPHYACR5);

        if (!index) {
            tmp = readl(com + U3D_U2PHYACR4);
            tmp &= ~(RG_USB20_GPIO_CTL | USB20_GPIO_MODE);
            writel(tmp, com + U3D_U2PHYACR4);
        }

        tmp = readl(com + U3D_USBPHYACR6);
        tmp &= ~RG_USB20_BC11_SW_EN;    /* DP/DM BC1.1 path Disable */
        tmp &= ~RG_USB20_SQTH;
        tmp |= RG_USB20_SQTH_VAL(2);
        writel(tmp, com + U3D_USBPHYACR6);

        /* power on */
        tmp = readl(com + U3D_U2PHYDTM0);
        tmp &= ~(RG_XCVRSEL | RG_DATAIN | DTM0_PART_MASK);
        writel(tmp, com + U3D_U2PHYDTM0);

        /* OTG Enable */
        tmp = readl(com + U3D_USBPHYACR6);
        tmp |= RG_USB20_OTG_VBUSCMP_EN;
        writel(tmp, com + U3D_USBPHYACR6);

        tmp = readl(com + U3D_U2PHYDTM1);
        tmp |= RG_VBUSVALID | RG_AVALID;
        tmp &= ~RG_SESSEND;
        writel(tmp, com + U3D_U2PHYDTM1);
    }

    return 0;
}

int u2_phy_set_mode(struct mtk_phy_instance *phy, enum phy_mode mode)
{
    void *com;
    u32 tmp;

    if (!phy) {
        phy_err("%s: phy is null\n", __func__);
        return -1;
    }

    com = phy->u2_banks[0]->com;
    tmp = readl(com + U3D_U2PHYDTM1);
    switch (mode) {
        case USB_MODE_PERIPHERAL:
            tmp |= FORCE_IDDIG | RG_IDDIG;
            break;
        case USB_MODE_HOST:
            tmp |= FORCE_IDDIG;
            tmp &= ~RG_IDDIG;
            break;
        case USB_MODE_OTG:
            tmp &= ~(FORCE_IDDIG | RG_IDDIG);
            break;
        default:
            phy_err("%s: error mode\n", __func__);
            break;
    }

    writel(tmp, com + U3D_U2PHYDTM1);
    return 0;
}

int u3_phy_instance_init(struct mtk_phy_instance *phy)
{
    return 0;
}

int u2_phy_instance_deinit(struct mtk_phy_instance *phy)
{
    void *com;
    int index;

    if (!phy) {
        phy_err("%s: phy is null\n", __func__);
        return -1;
    }

    for (index = 0; index < phy->u2_phy_num; index++) {
        com = phy->u2_banks[index]->com;
        phy_debug("%s: phy[%d]:%p\n", __func__, index, com);
        /* power down device mode */
        phy_clrbits(com, U3D_U2PHYDTM1, RG_VBUSVALID | RG_BVALID | RG_AVALID);
        phy_setbits(com, U3D_U2PHYDTM1, RG_IDDIG | RG_SESSEND);

        /* cleaer device force mode */
        phy_clrbits(com, U3D_U2PHYDTM1, U3D_U2PHYFRCDEV_MASK);
        phy_clrbits(com, U3D_U2PHYDTM0, RG_SUSPENDM);
        phy_setbits(com, U3D_U2PHYDTM0, FORCE_SUSPENDM);
        mdelay(2);
    }

    return 0;
}

int u3_phy_instance_deinit(struct mtk_phy_instance *phy)
{
    return 0;
}

