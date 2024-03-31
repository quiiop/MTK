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
 * driver for SSUSB controller
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

#include <hal_clk.h>
#include <hal_spm.h>
#include <hal_nvic.h>
#include <hal_usb_mtu3_rscs.h>
#include <hal_eint.h>
#include <hal_gpio.h>
#include "hal_boot.h"

#include "mtu3.h"
#include "usbd.h"
#include "usbdcore.h"
#include "ssusb_hw_regs.h"

static int mtu3_get_clk(struct ssusb_mtk *ssusb)
{
    return 0;
}

static int mtu3_enable_clk(struct ssusb_mtk *ssusb)
{
    int ret;

    ret = hal_clock_enable(HAL_CLOCK_CG_USB_CTRL_XTAL);
    ret += hal_clock_enable(HAL_CLOCK_CG_USB_PHY_XTAL);

    ret += hal_clock_mux_select(HAL_CLOCK_SEL_USB_SYS, CLK_USB_SYS_CLKSEL_120M);
    ret += hal_clock_mux_select(HAL_CLOCK_SEL_USB_XHCI, CLK_USB_XHCI_CLKSEL_133M);

    ret += hal_clock_enable(HAL_CLOCK_CG_USB_PLL);
    ret += hal_clock_enable(HAL_CLOCK_CG_USB_XHCI);
    ret += hal_clock_enable(HAL_CLOCK_CG_USB_SYS);

    if (ret) {
        mtu3_err("%s: fail!\n", __func__);
        return -1;
    }

    return 0;
}

static int mtu3_disable_clk(struct ssusb_mtk *ssusb)
{
    int ret;

    ret = hal_clock_disable(HAL_CLOCK_CG_USB_SYS);
    ret += hal_clock_disable(HAL_CLOCK_CG_USB_XHCI);
    ret += hal_clock_disable(HAL_CLOCK_CG_USB_PLL);
    ret += hal_clock_disable(HAL_CLOCK_CG_USB_PHY_XTAL);
    ret += hal_clock_disable(HAL_CLOCK_CG_USB_CTRL_XTAL);

    if (ret) {
        mtu3_err("%s: fail!\n", __func__);
        return -1;
    }

    return 0;
}

int mtu3_get_pd(struct ssusb_mtk *ssusb)
{
    return 0;
}

int mtu3_enable_pd(struct ssusb_mtk *ssusb)
{
    MTCMOS_PWR_ON_USB2;
    SRAM_PWR_ON_USB2;

    return 0;
}

int mtu3_disable_pd(struct ssusb_mtk *ssusb)
{
    SRAM_PWR_DOWN_USB2;
    MTCMOS_PWR_DOWN_USB2;

    return 0;
}

static void mtu3_ip_sw_reset(struct ssusb_mtk *ssusb)
{
    /* reset whole ip */
    mtu3_debug("%s: reset whole ip\n", __func__);
    ssusb_setbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);
    udelay(1);
    ssusb_clrbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);
}

void mtu3_rscs_log(struct ssusb_mtk *ssusb)
{
    mtu3_debug("%s: mac:%p\n", __func__, ssusb->u3d.mac_base);
    mtu3_debug("%s: dev_irq:%x\n", __func__, ssusb->u3d.dev_irq);
    mtu3_debug("%s: ippc:%p\n", __func__, ssusb->ippc_base);
    mtu3_debug("%s: phy:%p\n", __func__, ssusb->phy.u2_banks[0]->com);
    mtu3_debug("%s: u2_phy_num:%x\n", __func__, ssusb->phy.u2_phy_num);
}

static void mtu3_get_ip_vers(struct ssusb_mtk *ssusb)
{
    u32 val;

    val = ssusb_readl(ssusb->ippc_base, U3D_SSUSB_IP_DEV_CAP);
    ssusb->u3d.is_u3_ip = !!SSUSB_IP_DEV_U3_PORT_NUM(val);
    val = ssusb_readl(ssusb->ippc_base, U3D_SSUSB_HW_ID);
    mtu3_debug("IP version 0x%x(%s IP)\n", val,
               ssusb->u3d.is_u3_ip ? "U3" : "U2");
}

static void mtu3_dev_reset(struct ssusb_mtk *ssusb)
{
    ssusb_setbits(ssusb->ippc_base, U3D_SSUSB_DEV_RST_CTRL, SSUSB_DEV_SW_RST);
    ssusb_clrbits(ssusb->ippc_base, U3D_SSUSB_DEV_RST_CTRL, SSUSB_DEV_SW_RST);
}

/* hardware access APIs */
int wait_for_value(void *addr, u32 msk, u32 value, int us_intvl, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if ((readl(addr) & msk) == value)
            return 0;

        udelay(us_intvl);
    }

    return -ETIMEDOUT;
}

static int mtu3_check_clk_sts(struct ssusb_mtk *ssusb)
{
    u32 check_val;
    int ret = 0;

    check_val = SSUSB_SYS125_RST_B_STS | SSUSB_SYSPLL_STABLE |
                SSUSB_REF_RST_B_STS;
    if (ssusb->u3d.is_u3_ip)
        check_val |= SSUSB_U3_MAC_RST_B_STS;

    ret = wait_for_value(ssusb->ippc_base + U3D_SSUSB_IP_PW_STS1,
                         check_val, check_val, 100, 100);
    if (ret) {
        mtu3_err("clk sys125 NG\n");
        goto err;
    } else {
        mtu3_debug("clk sys125 OK\n");
    }

    ret = wait_for_value(ssusb->ippc_base + U3D_SSUSB_IP_PW_STS2,
                         SSUSB_U2_MAC_SYS_RST_B_STS,
                         SSUSB_U2_MAC_SYS_RST_B_STS, 100, 100);
    if (ret) {
        mtu3_err("clk mac2 NG\n");
        goto err;
    } else {
        mtu3_debug("clk mac2 OK\n");
    }

    return 0;

err:
    mtu3_err("Refer clocks stability check failed!\n");
    return ret;
}

static int mtu3_ssusb_enable(struct ssusb_mtk *ssusb)
{
    ssusb_clrbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);
    ssusb_clrbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);
    ssusb_clrbits(ssusb->ippc_base, U3D_SSUSB_U2_CTRL_0P,
                  (SSUSB_U2_PORT_DIS | SSUSB_U2_PORT_PDN |
                   SSUSB_U2_PORT_HOST_SEL));

    if (ssusb->u3d.is_u3_ip)
        ssusb_clrbits(ssusb->ippc_base, U3D_SSUSB_U3_CTRL_0P,
                      (SSUSB_U3_PORT_DIS | SSUSB_U3_PORT_PDN |
                       SSUSB_U3_PORT_HOST_SEL));

    if (ssusb->dr_mode == USB_DR_MODE_OTG) {
        ssusb_setbits(ssusb->ippc_base, SSUSB_U2_CTRL(0), SSUSB_U2_PORT_OTG_SEL);
        if (ssusb->u3d.is_u3_ip)
            ssusb_setbits(ssusb->ippc_base, SSUSB_U3_CTRL(0),
                          SSUSB_U3_PORT_DUAL_MODE);
    }

    return mtu3_check_clk_sts(ssusb);
}

static int mtu3_hw_init(struct ssusb_mtk *ssusb)
{
    int ret;

    mtu3_get_ip_vers(ssusb);
    mtu3_dev_reset(ssusb);
    ret = mtu3_ssusb_enable(ssusb);
    if (ret) {
        mtu3_err("%s: fail\n", __func__);
        return ret;
    }

    return 0;
}

static void mtu3_intr_disable(struct mu3d *u3d)
{
    void *mbase = u3d->mac_base;

    ssusb_writel(mbase, U3D_LV1IECR, ~0x0);
    ssusb_writel(mbase, U3D_EPIECR, ~0x0);
}

static void mtu3_intr_status_clear(struct mu3d *u3d)
{
    void *mbase = u3d->mac_base;

    ssusb_writel(mbase, U3D_EPISR, ~0x0);
    ssusb_writel(mbase, U3D_COMMON_USB_INTR, ~0x0);
    ssusb_writel(mbase, U3D_LTSSM_INTR, ~0x0);
    ssusb_writel(mbase, U3D_DEV_LINK_INTR, ~0x0);
}

void mtu3_start(struct ssusb_mtk *ssusb)
{
    mtu3_crit("%s\n", __func__);

    ssusb_clrbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);
    if (ssusb->u3d.speed == SSUSB_SPEED_FULL)
        ssusb_clrbits(ssusb->u3d.mac_base, U3D_POWER_MANAGEMENT, HS_ENABLE);

    mtu3_intr_enable(&ssusb->u3d);
    mtu3_soft_connect(&ssusb->u3d);
}

void mtu3_stop(struct ssusb_mtk *ssusb)
{
    mtu3_crit("%s\n", __func__);

    mtu3_intr_disable(&ssusb->u3d);
    mtu3_intr_status_clear(&ssusb->u3d);
    mtu3_soft_disconnect(&ssusb->u3d);
    ssusb_setbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);
}

static u32 mtu3_get_id_status(struct ssusb_mtk *ssusb)
{
    hal_gpio_data_t data;

    hal_gpio_get_input(ssusb->iddig_gpio, &data);
    return data;
}

static void mtu3_toggle_opstate(struct ssusb_mtk *ssusb)
{
    if (!ssusb->u3d.is_u3_ip) {
        ssusb_setbits(ssusb->u3d.mac_base, U3D_DEVICE_CONTROL, DC_SESSION);
        ssusb_setbits(ssusb->u3d.mac_base, U3D_POWER_MANAGEMENT, SOFT_CONN);
    }
}

void ssusb_set_force_mode(struct ssusb_mtk *ssusb, enum usb_dr_mode mode)
{
    u32 value;

    value = ssusb_readl(ssusb->ippc_base, SSUSB_U2_CTRL(0));
    switch (mode) {
        case USB_DR_MODE_PERIPHERAL:
            value |= SSUSB_U2_PORT_FORCE_IDDIG | SSUSB_U2_PORT_RG_IDDIG;
            break;
        case USB_DR_MODE_HOST:
            value |= SSUSB_U2_PORT_FORCE_IDDIG;
            value &= ~SSUSB_U2_PORT_RG_IDDIG;
            break;
        case USB_DR_MODE_OTG:
            value &= ~(SSUSB_U2_PORT_FORCE_IDDIG | SSUSB_U2_PORT_RG_IDDIG);
            break;
        default:
            return;
    }
    ssusb_writel(ssusb->ippc_base, SSUSB_U2_CTRL(0), value);
}

static void mtu3_switch_port(struct ssusb_mtk *ssusb, bool is_host)
{
    u32 value;
    void *ibase = ssusb->ippc_base;

    mtu3_debug("%s is_host:%d\n", (ssusb->u3d.is_u3_ip) ? "u3port" : "u2port", is_host);

    value = ssusb_readl(ibase, SSUSB_U2_CTRL(0));
    value |= SSUSB_U2_PORT_PDN | SSUSB_U2_PORT_DIS;
    ssusb_writel(ibase, SSUSB_U2_CTRL(0), value);

    value = ssusb_readl(ibase, SSUSB_U2_CTRL(0));
    value &= ~(SSUSB_U2_PORT_PDN | SSUSB_U2_PORT_DIS);
    value = is_host ? (value | SSUSB_U2_PORT_HOST_SEL) :
            (value & (~SSUSB_U2_PORT_HOST_SEL));
    ssusb_writel(ibase, SSUSB_U2_CTRL(0), value);

    if (ssusb->u3d.is_u3_ip) {
        value = ssusb_readl(ibase, SSUSB_U3_CTRL(0));
        value |= SSUSB_U3_PORT_PDN | SSUSB_U3_PORT_DIS;
        ssusb_writel(ibase, SSUSB_U3_CTRL(0), value);

        value = ssusb_readl(ibase, SSUSB_U3_CTRL(0));
        value &= ~(SSUSB_U3_PORT_PDN | SSUSB_U3_PORT_DIS);
        value = is_host ? (value | SSUSB_U3_PORT_HOST_SEL) :
                (value & (~SSUSB_U3_PORT_HOST_SEL));
        ssusb_writel(ibase, SSUSB_U3_CTRL(0), value);
    }
}

static void mtu3_switch_port_to_host(struct ssusb_mtk *ssusb)
{
    mtu3_switch_port(ssusb, true);
    mtu3_check_clk_sts(ssusb);

    /* after all clocks are stable */
    mtu3_toggle_opstate(ssusb);
}

static void mtu3_switch_port_to_device(struct ssusb_mtk *ssusb)
{
    mtu3_switch_port(ssusb, false);
    mtu3_check_clk_sts(ssusb);
}

static void mtu3_set_port0_vbus(struct ssusb_mtk *ssusb, bool enable)
{
    hal_pinmux_set_function(ssusb->vbus_gpio, 0);
    hal_gpio_set_direction(ssusb->vbus_gpio, HAL_GPIO_DIRECTION_OUTPUT);

    if (enable)
        hal_gpio_set_output(ssusb->vbus_gpio, HAL_GPIO_DATA_HIGH);
    else
        hal_gpio_set_output(ssusb->vbus_gpio, HAL_GPIO_DATA_LOW);
}

void mtu3_role_switch(void *user_data)
{
    u32 id_status;
    struct ssusb_mtk *ssusb;
    u32 debounce_time_ms = 100;

    ssusb = &ssusb_rscs[0];
    id_status = mtu3_get_id_status(ssusb);
    switch (id_status) {
        case 0:
            mtu3_crit("id:%d switch to host\n", id_status);
            ssusb_set_force_mode(ssusb, USB_DR_MODE_HOST);
            mtu3_stop(ssusb);
            mtu3_switch_port_to_host(ssusb);
            mtu3_set_port0_vbus(ssusb, true);
            hal_eint_set_debounce_count(IDDIG_EINT, 0);
            hal_eint_set_trigger_mode(IDDIG_EINT, HAL_EINT_LEVEL_HIGH);
            hal_eint_set_debounce_time(IDDIG_EINT, debounce_time_ms);
            break;
        case 1:
            mtu3_crit("id:%d switch to device\n", id_status);
            mtu3_set_port0_vbus(ssusb, false);
            ssusb_set_force_mode(ssusb, USB_DR_MODE_PERIPHERAL);
            mtu3_switch_port_to_device(ssusb);
            mtu3_start(ssusb);
            hal_eint_set_debounce_count(IDDIG_EINT, 0);
            hal_eint_set_trigger_mode(IDDIG_EINT, HAL_EINT_LEVEL_LOW);
            hal_eint_set_debounce_time(IDDIG_EINT, debounce_time_ms);
            break;
        default:
            mtu3_err("invalid status:%d\n", id_status);
    }
}

static int mtu3_dual_role_init(struct ssusb_mtk *ssusb)
{
    /* u32 debounce_time_ms = 100; */
    mtu3_set_port0_vbus(ssusb, false);

    hal_eint_mask(IDDIG_EINT);
    hal_eint_clear_software_trigger(IDDIG_EINT);
    if (hal_boot_get_hw_ver() == 0x8A00) {
        hal_pinmux_set_function(ssusb->iddig_gpio, MT7933_PIN_34_FUNC_CM33_GPIO_EINT26);
        hal_gpio_set_direction(ssusb->iddig_gpio, HAL_GPIO_DIRECTION_INPUT);
        hal_gpio_pull_up(ssusb->iddig_gpio);
    } else
        hal_pinmux_set_function(ssusb->iddig_gpio, MT7933_PIN_34_FUNC_USB_IDDIG);

    if (hal_eint_register_callback(IDDIG_EINT, mtu3_role_switch, NULL) != HAL_EINT_STATUS_OK) {
        mtu3_err("%s: register callback error.\n", __func__);
        return -1;
    }

    hal_eint_set_trigger_mode(IDDIG_EINT, HAL_EINT_LEVEL_LOW);
    /* if (hal_eint_set_debounce_time(IDDIG_EINT, debounce_time_ms) != HAL_EINT_STATUS_OK) {
     *   mtu3_err("%s: set debounce error.\n", __func__);
     *   return -1;
     *}
     */

    hal_eint_unmask(IDDIG_EINT);

    return 0;
}

static int mtu3_dual_role_deinit(struct ssusb_mtk *ssusb)
{
    hal_eint_mask(IDDIG_EINT);
    hal_eint_register_callback(IDDIG_EINT, NULL, NULL);
    return 0;
}

int udc_register_gadget(struct gadget_dev *gdev, u8 index)
{
    struct ssusb_mtk *ssusb;

    if (!gdev || index > (SSUSB_IP - 1)) {
        mtu3_err("%s: fail!\n", __func__);
        return -1;
    }

    ssusb = &ssusb_rscs[index];
    if (ssusb->u3d.gdev) {
        mtu3_crit("%s: Class[%s] already register!\n", __func__, ssusb->u3d.gdev->name);
        gdev->next = ssusb->u3d.gdev->next;
        ssusb->u3d.gdev->next = gdev;
        return UDC_CLASS_ONLINE;
    }

    mtu3_crit("%s: Class[%s] register!\n", __func__, gdev->name);
    ssusb->u3d.gdev = gdev;
    ssusb->u3d.gdev->next = gdev;

    gdev->private = &ssusb->u3d;
    return 0;
}

int udc_init(u8 index)
{
    int ret;
    struct ssusb_mtk *ssusb;

    if (index > (SSUSB_IP - 1)) {
        mtu3_err("%s: index error!\n", __func__);
        return -1;
    }

    ssusb = &ssusb_rscs[index];
    mtu3_rscs_log(ssusb);

    ret = mtu3_get_pd(ssusb);
    ret += mtu3_enable_pd(ssusb);
    ret += mtu3_get_clk(ssusb);
    ret += mtu3_enable_clk(ssusb);
    if (ret)
        return ret;

    u2_phy_instance_init(&ssusb->phy);

    ret = mtu3_hw_init(ssusb);
    if (ret) {
        mtu3_err("%s: mtu3_hw_init fail\n", __func__);
        return ret;
    }

    ret = udc_enable(&ssusb->u3d);
    if (ret) {
        mtu3_err("%s: udc_enable fail\n", __func__);
        return ret;
    }

    if (ssusb->dr_mode == USB_DR_MODE_OTG) {
        ret = mtu3_dual_role_init(ssusb);
        if (ret) {
            mtu3_err("%s: mtu3_dual_role_init fail\n", __func__);
            return ret;
        }

        mtu3_role_switch(0);
    }
    return 0;
}

int udc_deinit(u8 index)
{
    int ret;
    struct ssusb_mtk *ssusb;

    if (index > (SSUSB_IP - 1)) {
        mtu3_err("%s: index error!\n", __func__);
        return -1;
    }

    ssusb = &ssusb_rscs[index];
    if (!ssusb->u3d.gdev) {
        mtu3_err("%s: fail!\n", __func__);
        return -1;
    }

    udc_disable(&ssusb->u3d);
    u2_phy_instance_deinit(&ssusb->phy);
    mtu3_ip_sw_reset(ssusb);

    ret = mtu3_disable_clk(ssusb);
    ret += mtu3_disable_pd(ssusb);
    if (ret)
        return -1;

    if (ssusb->dr_mode == USB_DR_MODE_OTG)
        mtu3_dual_role_deinit(ssusb);

    ssusb->u3d.gdev = NULL;
    return 0;
}

