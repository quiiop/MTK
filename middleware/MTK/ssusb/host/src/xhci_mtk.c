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

#include <FreeRTOS.h>
#include <timers.h>
#include <hal_clk.h>
#include <hal_gpio.h>
#include <hal_spm.h>
#include <hal_usb_xhci_rscs.h>

#include <usb.h>
#include <usb_phy.h>
#include <ssusb_hw_regs.h>
#include <xhci.h>
#include <xhci_private.h>
#include "xhci_mtk.h"

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_nvic.h"
#include "hal_sleep_manager_internal.h"
#include "hal_sleep_manager_platform.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

int xhci_wait_for_value(void *addr, u32 msk, u32 value, u32 us_intvl, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++) {
        if ((readl(addr) & msk) == value)
            return 0;
        udelay(us_intvl);
    }
    return -1;
}

static int check_ip_clk_status(struct xhci_hcd_mtk *mtk)
{
    int ret;
    u32 temp;
    u32 u3_port_num;
    u32 u2_port_num;
    void *ippc = mtk->ippc_base;

    temp = readl(ippc + U3D_SSUSB_IP_XHCI_CAP);
    u3_port_num = SSUSB_IP_XHCI_U3_PORT_NUM(temp);
    u2_port_num = SSUSB_IP_XHCI_U2_PORT_NUM(temp);

    ret = xhci_wait_for_value(ippc + U3D_SSUSB_IP_PW_STS1,
                              SSUSB_SYSPLL_STABLE, SSUSB_SYSPLL_STABLE, 100, 100);
    if (ret) {
        usb_err("sypll is not stable!!!\n");
        goto err;
    }

    ret = xhci_wait_for_value(ippc + U3D_SSUSB_IP_PW_STS1,
                              SSUSB_REF_RST_B_STS, SSUSB_REF_RST_B_STS, 100, 100);
    if (ret) {
        usb_err("ref_clk is still inactive!!!\n");
        goto err;
    }

    ret = xhci_wait_for_value(ippc + U3D_SSUSB_IP_PW_STS1,
                              SSUSB_SYS125_RST_B_STS, SSUSB_SYS125_RST_B_STS, 100, 100);
    if (ret) {
        usb_err("sys125_ck is still inactive!!!\n");
        goto err;
    }

    ret = xhci_wait_for_value(ippc + U3D_SSUSB_IP_PW_STS1,
                              SSUSB_XHCI_RST_B_STS, SSUSB_XHCI_RST_B_STS, 100, 100);
    if (ret) {
        usb_err("xhci_ck is still inactive!!!\n");
        goto err;
    }

    if (u3_port_num) {
        ret = xhci_wait_for_value(ippc + U3D_SSUSB_IP_PW_STS1,
                                  SSUSB_U3_MAC_RST_B_STS, SSUSB_U3_MAC_RST_B_STS, 100, 100);
        if (ret) {
            usb_err("mac3_mac_ck is still inactive!!!\n");
            goto err;
        }
    }

    if (u2_port_num) {
        ret = xhci_wait_for_value(ippc + U3D_SSUSB_IP_PW_STS2,
                                  SSUSB_U2_MAC_SYS_RST_B_STS,
                                  SSUSB_U2_MAC_SYS_RST_B_STS, 100, 100);
        if (ret) {
            usb_err("mac2_sys_ck is still inactive!!!\n");
            goto err;
        }
    }

    return 0;

err:
    usb_err("usb clk is not stable!!!\n");
    return -1;
}

static int xhci_mtk_host_enable(struct xhci_hcd_mtk *mtk)
{
    int i;
    u32 temp;

    temp = readl(mtk->ippc_base + U3D_SSUSB_IP_XHCI_CAP);
    mtk->num_u3_ports = SSUSB_IP_XHCI_U3_PORT_NUM(temp);
    mtk->num_u2_ports = SSUSB_IP_XHCI_U2_PORT_NUM(temp);

    /* power on host ip */
    temp = readl(mtk->ippc_base + U3D_SSUSB_IP_PW_CTRL1);
    temp &= ~(SSUSB_IP_HOST_PDN);
    writel(temp, mtk->ippc_base + U3D_SSUSB_IP_PW_CTRL1);

    /* power on and enable all u3 ports */
    for (i = 0; i < mtk->num_u3_ports; i++) {
        temp = readl(mtk->ippc_base + SSUSB_U3_CTRL(i));
        temp &= ~(SSUSB_U3_PORT_PDN | SSUSB_U3_PORT_DIS);
        temp |= SSUSB_U3_PORT_HOST_SEL;
        writel(temp, mtk->ippc_base + SSUSB_U3_CTRL(i));
        usb_debug("%s: enable u3port[%d]\n", __func__, i);
    }

    /* power on and enable all u2 ports */
    for (i = 0; i < mtk->num_u2_ports; i++) {
        temp = readl(mtk->ippc_base + SSUSB_U2_CTRL(i));
        temp &= ~(SSUSB_U2_PORT_PDN | SSUSB_U2_PORT_DIS);
        temp |= SSUSB_U2_PORT_HOST_SEL;
        writel(temp, mtk->ippc_base + SSUSB_U2_CTRL(i));
        usb_debug("%s: enable u2port[%d]\n", __func__, i);
    }

    return check_ip_clk_status(mtk);
}

static int xhci_mtk_host_disable(struct xhci_hcd_mtk *mtk)
{
    int i;
    int ret;
    u32 temp;

    /* power on and enable all u3 ports */
    for (i = 0; i < mtk->num_u3_ports; i++) {
        temp = readl(mtk->ippc_base + SSUSB_U3_CTRL(i));
        temp |= SSUSB_U3_PORT_PDN;
        writel(temp, mtk->ippc_base + SSUSB_U3_CTRL(i));
        usb_debug("%s: disable u3port[%d]\n", __func__, i);
    }

    /* power on and enable all u2 ports */
    for (i = 0; i < mtk->num_u2_ports; i++) {
        temp = readl(mtk->ippc_base + SSUSB_U2_CTRL(i));
        temp |= SSUSB_U2_PORT_PDN;
        writel(temp, mtk->ippc_base + SSUSB_U2_CTRL(i));
        usb_debug("%s: disable u2port[%d]\n", __func__, i);
    }

    /* power down host ip */
    temp = readl(mtk->ippc_base + U3D_SSUSB_IP_PW_CTRL1);
    temp |= SSUSB_IP_HOST_PDN;
    writel(temp, mtk->ippc_base + U3D_SSUSB_IP_PW_CTRL1);

    /* wait for host ip to sleep */
    ret = xhci_wait_for_value(mtk->ippc_base + U3D_SSUSB_IP_PW_STS1,
                              SSUSB_IP_SLEEP_STS, SSUSB_IP_SLEEP_STS, 5, 1000);
    if (ret) {
        usb_err("ssusb ip sleep failed!!!\n");
        return ret;
    }

    usb_debug("ssusb ip sleep!!!\n");
    return 0;
}

static void mtk_usb_sw_reset(struct xhci_hcd_mtk *mtk)
{
    u32 temp;

    /* reset whole ip */
    usb_debug("%s: reset whole ip\n", __func__);
    temp = readl(mtk->ippc_base + U3D_SSUSB_IP_PW_CTRL0);
    temp |= SSUSB_IP_SW_RST;
    writel(temp, mtk->ippc_base + U3D_SSUSB_IP_PW_CTRL0);
    udelay(1);
    temp = readl(mtk->ippc_base + U3D_SSUSB_IP_PW_CTRL0);
    temp &= ~(SSUSB_IP_SW_RST);
    writel(temp, mtk->ippc_base + U3D_SSUSB_IP_PW_CTRL0);
}

static int mtk_usb_get_pd(struct xhci_hcd_mtk *mtk)
{
    return 0;
}

static int mtk_usb_enable_pd(struct xhci_hcd_mtk *mtk)
{
    MTCMOS_PWR_ON_USB2;
    SRAM_PWR_ON_USB2;

    return 0;
}

static int mtk_usb_disable_pd(struct xhci_hcd_mtk *mtk)
{
    SRAM_PWR_DOWN_USB2;
    MTCMOS_PWR_DOWN_USB2;

    return 0;
}

static int mtk_usb_get_clk(struct xhci_hcd_mtk *mtk)
{
    return 0;
}

static int mtk_usb_enable_clk(struct xhci_hcd_mtk *mtk)
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
        usb_err("%s: fail!\n", __func__);
        return -1;
    }

    return 0;
}

static int mtk_usb_disable_clk(struct xhci_hcd_mtk *mtk)
{
    int ret;

    ret = hal_clock_disable(HAL_CLOCK_CG_USB_SYS);
    ret += hal_clock_disable(HAL_CLOCK_CG_USB_XHCI);
    ret += hal_clock_disable(HAL_CLOCK_CG_USB_PLL);
    ret += hal_clock_disable(HAL_CLOCK_CG_USB_PHY_XTAL);
    ret += hal_clock_disable(HAL_CLOCK_CG_USB_CTRL_XTAL);

    if (ret) {
        usb_err("%s: fail!\n", __func__);
        return -1;
    }

    return 0;
}

int mtk_usb_port_suspend(struct xhci_hcd_mtk *mtk, int port)
{
    int ret;
    int status;
    void *addr = mtk->xhci_base + PORTSC;

    status = ssusb_readl(addr, port * 0x10);
    status = xhci_port_state_to_neutral(status);
    status = (status & ~(0xf << 5));
    status = (status | (0x3 << 5) | (1 << 16));
    ssusb_writel(addr, port * 0x10, status);

    ret = xhci_wait_for_value(addr + (port * 0x10), (0xf << 5), (3 << 5), 10, 1000);
    if (ret) {
        usb_err("Port[%d] Suspend Timeout \r\n", port);
        return -1;
    }

    usb_debug("Port[%d] Suspend \r\n", port);
    return 0;
}

int mtk_usb_port_resume(struct xhci_hcd_mtk *mtk, int port, bool is_u2)
{
    int ret;
    int status;
    void *addr = mtk->xhci_base + PORTSC;

    status = ssusb_readl(addr, port * 0x10);
    if (((status >> 5) & 0xf) != 3) {
        usb_err("port[%d] not in suspend state, please suspend port first \r\n", port);
        return -1;
    }

    if (is_u2) {
        status = xhci_port_state_to_neutral(status);
        status = (status & ~(0xf << 5));
        status = (status | (15 << 5) | (1 << 16));
        ssusb_writel(addr, port * 0x10, status);
        mdelay(20);
    }

    status = ssusb_readl(addr, port * 0x10);
    status = xhci_port_state_to_neutral(status);
    status = (status & ~(0xf << 5));
    status = (status | (1 << 16));
    ssusb_writel(addr, port * 0x10, status);
    ret = xhci_wait_for_value(addr + (port * 0x10), (0xf << 5), (0 << 5), 10, 10000);
    if (ret) {
        usb_err("Port[%d] Resume Timeout \r\n", port);
        return -1;
    }

    usb_debug("Port[%d] Resume \r\n", port);
    return 0;
}

static bool port0_is_host(struct xhci_hcd_mtk *mtk)
{
    bool is_host;
    u32 u2_host, u3_host;

    if (mtk->num_u3_ports) {
        u3_host = readl(mtk->ippc_base + SSUSB_U3_CTRL(0));
        u3_host &= SSUSB_U3_PORT_HOST_SEL;
    }

    u2_host = readl(mtk->ippc_base + SSUSB_U2_CTRL(0));
    u2_host &= SSUSB_U2_PORT_HOST_SEL;
    is_host = mtk->num_u3_ports ? (u2_host && u3_host) : u2_host;
    usb_debug("%s: port0_is_host:%d\n", __func__, is_host);
    return !!is_host;

}

void mtk_usb_host_suspend(void *data)
{
    bool is_host;
    u32 conn_status;
    void *addr;
    int i, index, ret;
    struct xhci_hcd_mtk *mtk;

    mtk = (struct xhci_hcd_mtk *)data;
    index = mtk->index;
    addr = mtk->xhci_base + PORTSC;
    if (index > (XHCI_INSTANCE - 1)) {
        usb_err("%s: index error!\n", __func__);
        return;
    }

    is_host = port0_is_host(mtk);
    if (!is_host)
        return;

    /*
     * if the device is connected, suspend the device first,
     * do not support external hub suspend
     */
    for (i = 0; i < mtk->num_u3_ports; i++) {
        conn_status = readl(addr + i * 0x10);
        conn_status &= (0x1 << 0);
        if (conn_status)
            mtk_usb_port_suspend(mtk, i);
    }

    for (i = 0; i < mtk->num_u2_ports; i++) {
        conn_status = readl(addr + ((i + mtk->num_u3_ports) * 0x10));
        conn_status &= (0x1 << 0);
        if (conn_status)
            mtk_usb_port_suspend(mtk, i + mtk->num_u3_ports);
    }

    ret = xhci_mtk_host_disable(mtk);
    if (ret)
        return;

    ret = mtk_usb_disable_clk(mtk);
    if (ret)
        return;

    ssusb_setbits((void *)PERI_SSUSB_SPM_GLUE_CG, 0,
                  (u32)(RG_SSUSB_SPM_INT_EN | RG_SSUSB_IP_SLEEP_EN));
    usb_debug("ssusb host suspend!\n");
    return;
}

void mtk_usb_host_resume(void *data)
{
    bool is_host;
    u32 conn_status;
    void *addr;
    int i, index, ret;
    struct xhci_hcd_mtk *mtk;

    mtk = (struct xhci_hcd_mtk *)data;
    index = mtk->index;
    addr = mtk->xhci_base + PORTSC;
    if (index > (XHCI_INSTANCE - 1)) {
        usb_err("%s: index error!\n", __func__);
        return;
    }

    is_host = port0_is_host(mtk);
    if (!is_host)
        return;

    ssusb_clrbits((void *)PERI_SSUSB_SPM_GLUE_CG, 0,
                  (u32)(RG_SSUSB_SPM_INT_EN | RG_SSUSB_IP_SLEEP_EN));
    ret = mtk_usb_enable_clk(mtk);
    if (ret)
        return;

    ret = xhci_mtk_host_enable(mtk);
    if (ret)
        return;

    /*
     * if the device is connected, resume the device first,
     * do not support external hub resume
     */
    for (i = 0; i < mtk->num_u3_ports; i++) {
        conn_status = readl(addr + i * 0x10);
        conn_status &= (0x1 << 0);
        if (conn_status)
            mtk_usb_port_resume(mtk, i, false);
    }

    for (i = 0; i < mtk->num_u2_ports; i++) {
        conn_status = readl(addr + ((i + mtk->num_u3_ports) * 0x10));
        conn_status &= (0x1 << 0);
        if (conn_status)
            mtk_usb_port_resume(mtk, i + mtk->num_u3_ports, true);
    }

    usb_debug("ssusb host resume!\n");
    return;
}

void mtk_usb_set_vbus(struct xhci_hcd_mtk *mtk, bool enable)
{
    int i;

    for (i = 0; i < VBUS_NUM; i++) {
        usb_debug("%s: gpio:%d set vbus[%d]:%d\n", __func__,
                  *mtk->vbus_gpio[i], i, enable);
        hal_pinmux_set_function(*mtk->vbus_gpio[i], 0);
        hal_gpio_set_direction(*mtk->vbus_gpio[i], HAL_GPIO_DIRECTION_OUTPUT);

        if (enable)
            hal_gpio_set_output(*mtk->vbus_gpio[i], HAL_GPIO_DATA_HIGH);
        else
            hal_gpio_set_output(*mtk->vbus_gpio[i], HAL_GPIO_DATA_LOW);
    }
}

void mtk_usb_host_task(struct xhci_hcd_mtk *mtk)
{
    mtk->timer = xTimerCreate("usb_poll", pdMS_TO_TICKS(300),
                              true, NULL, usb_poll);

    if (!mtk->timer) {
        usb_err("%s: failed\n", __func__);
        return;
    }

    if (xTimerStart(mtk->timer, 0) != pdPASS)
        usb_err("%s: xTimerStart failed\n", __func__);
}

int mtk_usb_host_deinit(int index)
{
    struct xhci_hcd_mtk *mtk;
    hci_t *controller;
    u8 max_slots, i;
    int ret;

    if (index > (XHCI_INSTANCE - 1)) {
        usb_err("%s: index error!\n", __func__);
        return -1;
    }

    mtk = &mtk_hcd[index];
    if (xTimerStop(mtk->timer, 0) != pdPASS)
        usb_err("%s: xTimerStop failed\n", __func__);

    if (xTimerDelete(mtk->timer, pdMS_TO_TICKS(300)) != pdPASS)
        usb_err("%s: xTimerDelete failed\n", __func__);

    mtk_usb_set_vbus(mtk, false);
    u2_phy_instance_deinit(&mtk->phy);
    mtk_usb_sw_reset(mtk);
    xhci_mtk_sch_exit(mtk);

    ret = mtk_usb_disable_clk(mtk);
    ret += mtk_usb_disable_pd(mtk);
    if (ret)
        return -1;

#ifdef HAL_SLEEP_MANAGER_ENABLED
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_USB);
    sleep_management_register_suspend_callback(SLEEP_BACKUP_RESTORE_USB, NULL, NULL);
    sleep_management_register_resume_callback(SLEEP_BACKUP_RESTORE_USB, NULL, NULL);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    controller = mtk->xhci_hcd;
    max_slots = ((xhci_t *)controller->instance)->max_slots_en;
    if (controller != NULL) {
        for (i = 1; i < max_slots + 1 ; i++) {
            if (controller->devices[i] != 0) {
                usb_detach_device(controller, i);
                usb_crit("%s i:%d \n", __func__, i);
            }
        }
    }

    controller->shutdown(controller);
    return 0;
}

hci_t *global_hcd;
int mtk_usb_host_init(int index)
{
    int ret;
    struct xhci_hcd_mtk *mtk;

    if (index > (XHCI_INSTANCE - 1)) {
        usb_err("%s: index error!\n", __func__);
        return -1;
    }

    mtk = &mtk_hcd[index];
    mtk->index = index;
    ret = mtk_usb_get_pd(mtk);
    ret += mtk_usb_enable_pd(mtk);
    ret += mtk_usb_get_clk(mtk);
    ret += mtk_usb_enable_clk(mtk);
    if (ret)
        return ret;

    mtk_usb_sw_reset(mtk);
    u2_phy_instance_init(&mtk->phy);
    ret = xhci_mtk_host_enable(mtk);
    if (ret) {
        usb_err("%s fail\n", __func__);
        return -1;
    }

    ret = xhci_mtk_sch_init(mtk);
    if (ret) {
        usb_err("%s fail\n", __func__);
        return -1;
    }

    mtk->xhci_hcd = xhci_init(mtk->xhci_base, mtk->xhci_irq, mtk);

    if (!mtk->xhci_hcd) {
        usb_err("xhci init fail\n\r");
        mtk_usb_host_deinit(index);
        return -1;
    }
    mtk_usb_set_vbus(mtk, true);
    global_hcd = (hci_t *)mtk->xhci_hcd;
    usb_debug("mtk->xhci_hcd:%p global_hcd:%p\n",
              mtk->xhci_hcd, global_hcd);
    mtk_usb_host_task(mtk);

#ifdef HAL_SLEEP_MANAGER_ENABLED
    usb_debug("xhci register suspend\n\r");
    hal_nvic_register_isr_handler(SSUSB_SPM_IRQ, NULL);
    hal_nvic_irq_set_type(SSUSB_SPM_IRQ, HAL_NVIC_IRQ_TYPE_LEVEL_HIGH);
    hal_nvic_enable_irq(SSUSB_SPM_IRQ);
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_USB);
    sleep_management_register_suspend_callback(SLEEP_BACKUP_RESTORE_USB, mtk_usb_host_suspend, mtk);
    sleep_management_register_resume_callback(SLEEP_BACKUP_RESTORE_USB, mtk_usb_host_resume, mtk);
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

    return 0;
}

