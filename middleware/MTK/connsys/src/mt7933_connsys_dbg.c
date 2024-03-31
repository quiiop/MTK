/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

//---------------------------------------------------------------------------
#include <string.h>
#include "FreeRTOS.h"
#include <task.h>
#include "mt7933_pos.h"
#include "mt7933_connsys_dbg.h"
#include "syslog.h"
#include <stdio.h>
#include "stdint.h"
#include <stdlib.h>

log_create_module(connsys_dbg, PRINT_LEVEL_INFO);
extern void btmtk_dump_bgfsys_hang_reg(void);
extern void mt7933_dump_wsys_hang_reg(void);

void dump_bus_hang_reg(void)
{
    int infra_on;
    int infra_off;
#ifdef MTK_MT7933_WIFI_ENABLE
    int wfsys_hang;
#endif /* #ifdef MTK_MT7933_WIFI_ENABLE */

    LOG_I(connsys_dbg, "%s, start", __func__);
    // 1 check conn infra on readable. if ok, check infra off; if fail, dump
    infra_on = dump_conn_infra_on_reg();
    if (infra_on)
        goto end;

    // 2 check infra off readable. if ok, check wfsys and bgfsys; if fail, dump
    infra_off = dump_conn_infra_off_reg();
    if (infra_off)
        goto end;

#ifdef MTK_MT7933_WIFI_ENABLE
    // 3 check wfsys readable. if ok, check wfsys; if fail, check wakeup
    wfsys_hang = dump_wfsys_hang_reg();
    if (wfsys_hang)
        goto end;
#endif /* #ifdef MTK_MT7933_WIFI_ENABLE */

    // 4 check bgfsys readable. if ok, check bgfsys; if fail, check wakeup
    dump_bgfsys_hang_reg();

end:
    LOG_I(connsys_dbg, "%s, end", __func__);
}

int dump_conn_infra_on_reg(void)
{
    uint32_t host_ck_cg;
    uint32_t gals_tx_val;
    uint32_t gals_rx_val;

    host_ck_cg = CONSYS_REG_READ(BGF_CONN_INFRA_ON_HOST_CLK_CG);
    gals_tx_val = CONSYS_REG_READ(BGF_CONN_INFRA_ON_GALS_TX);
    gals_rx_val = CONSYS_REG_READ(BGF_CONN_INFRA_ON_GALS_RX);

    LOG_I(connsys_dbg, "check ap2conn_infra on readable:\n"
          "host_clock_cg(0x%08x) = 0x%08lx[2], gals_tx_val(0x%08x) = 0x%08lx[31], gals_rx_val(0x%08x) = 0x%08lx[0]",
          BGF_CONN_INFRA_ON_HOST_CLK_CG, host_ck_cg,
          BGF_CONN_INFRA_ON_GALS_TX, gals_tx_val,
          BGF_CONN_INFRA_ON_GALS_RX, gals_rx_val);
    if (RBBIT(host_ck_cg, 2) || BBIT(gals_tx_val, 31) || BBIT(gals_rx_val, 0)) {
        if (BBIT(host_ck_cg, 2))
            LOG_I(connsys_dbg, "might be SW code issue");
        else
            LOG_I(connsys_dbg, "might be sleep protect control issue");
        return -1;
    } else {
        LOG_I(connsys_dbg, "check ap2conn_infra_on ok");
        return 0;
    }
}
int dump_conn_infra_off_reg(void)
{
    uint32_t bus_clk_val;
    uint32_t ip_ver;
    uint32_t bus_irq_val;
    uint32_t vndr_cur_mode[8];
    uint32_t power_st;
    uint32_t ahb_apb_tout_addr[6];
    uint32_t slp_prot_addr[11];
    // the following variable will use unused macor when MTK_DEBUG_LEVEL is not info
    uint32_t vndr_cur_st[8];
    uint32_t ahb_apb_tout[6];
    uint32_t ao_st_val[8];
    uint32_t slp_prot[11];

    int i = 0;

    SET_BIT(BGF_CONN_INFRA_OFF_BUS_CLK, BIT(0));
    for (i = 0; i < 5; i++) {
        bus_clk_val = CONSYS_REG_READ(BGF_CONN_INFRA_OFF_BUS_CLK);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    ip_ver = CONSYS_REG_READ(BGF_CONN_INFRA_OFF_IP_VER);
    bus_irq_val = CONSYS_REG_READ(BGF_CONN_INFRA_OFF_BUS_HANG_IRQ_ST);
    LOG_I(connsys_dbg, "check ap2conn_infra_off readable:\n"
          "  bus_clk(0x%08x) = 0x%08lx[2,1], ip_ver(0x%08x) = 0x%08lx, bus_irq(0x%08x) = 0x%08lx[1]",
          BGF_CONN_INFRA_OFF_BUS_CLK, bus_clk_val,
          BGF_CONN_INFRA_OFF_IP_VER, ip_ver,
          BGF_CONN_INFRA_OFF_BUS_HANG_IRQ_ST, bus_irq_val);
    LOG_I(connsys_dbg, "check parts:\n"
          "  vdnr_main(0x%08x) = 0x%08lx[0], vdnr_axi(0x%08x) = 0x%08lx[0], ahap_tout(0x%08x) = 0x%08lx[0,1]",
          BGF_CONN_INFRA_OFF_VDNR_MAIN_BUS_TOUT, CONSYS_REG_READ(BGF_CONN_INFRA_OFF_VDNR_MAIN_BUS_TOUT),
          BGF_CONN_INFRA_OFF_VDNR_AXI_LAYER_BUS_TOUT, CONSYS_REG_READ(BGF_CONN_INFRA_OFF_VDNR_AXI_LAYER_BUS_TOUT),
          BGF_CONN_INFRA_OFF_AHB_APB_BUS_TOUT, CONSYS_REG_READ(BGF_CONN_INFRA_OFF_AHB_APB_BUS_TOUT));
    LOG_I(connsys_dbg, "info: \n"
          "  wt_slp_clk(0x%08x) = 0x%08lx, connsys_cfg_st(0x%08x) = 0x%08lx, ahap_tout_mech(0x%08x) = 0x%08lx",
          BGF_CONN_WT_SLEEP_CLK, CONSYS_REG_READ(BGF_CONN_WT_SLEEP_CLK),
          BGF_CONN_INFRA_CFG_ST, CONSYS_REG_READ(BGF_CONN_INFRA_CFG_ST),
          BGF_CONN_INFRA_AHB_APB_TOUT_MECH, CONSYS_REG_READ(BGF_CONN_INFRA_AHB_APB_TOUT_MECH));

    /* 1. if ap2conn_infra off readable fail, conn_infra off bus hang, dump cr, return */
    if (RBBIT(bus_clk_val, 1) || RBBIT(bus_clk_val, 2) || RBBIT(bus_irq_val, 0) ||
        (ip_ver != BGF_CONN_INFRA_IP_VERSION)) {
        LOG_E(connsys_dbg, "conn_infra off bus might be hang, check power:");
        power_st = CONSYS_REG_READ(BGF_CONN_MCUSYS_POWER_ST);
        LOG_I(connsys_dbg, "chk pwr st CR(conn_infra, wfsys, bgfsys)\n"
              " power_st(0x%08x) = 0x%08lx[8~11,16~21,24~29]",
              BGF_CONN_MCUSYS_POWER_ST, power_st);
        LOG_I(connsys_dbg, "bgfsys_sleep_pw(0x%08x) = 0x%08x",
              BGF_SYSTRAP_RD_DEBUG, CONSYS_REG_READ(BGF_SYSTRAP_RD_DEBUG));
        LOG_I(connsys_dbg, "vndr bus tout(0x%08x)   = 0x%08x",
              CONN_INFRA_BUS_HANG_DETECT, CONSYS_REG_READ(CONN_INFRA_BUS_HANG_DETECT));

        ahb_apb_tout_addr[0] = BGF_CONN_INFRA_AHB_APB_TOUT_A1;
        ahb_apb_tout_addr[1] = BGF_CONN_INFRA_AHB_APB_TOUT_A2;
        ahb_apb_tout_addr[2] = BGF_CONN_INFRA_AHB_APB_TOUT_A3;
        ahb_apb_tout_addr[3] = BGF_CONN_INFRA_AHB_APB_TOUT_A4;
        ahb_apb_tout_addr[4] = BGF_CONN_INFRA_AHB_APB_TOUT_A5;
        ahb_apb_tout_addr[5] = BGF_CONN_INFRA_AHB_APB_TOUT_A6;
        slp_prot_addr[0] = BGF_CONN_INFRA_SLEEP_PRO_A1;
        slp_prot_addr[1] = BGF_CONN_INFRA_SLEEP_PRO_A2;
        slp_prot_addr[2] = BGF_CONN_INFRA_SLEEP_PRO_A3;
        slp_prot_addr[3] = BGF_CONN_INFRA_SLEEP_PRO_A4;
        slp_prot_addr[4] = BGF_CONN_INFRA_SLEEP_PRO_A5;
        slp_prot_addr[5] = BGF_CONN_INFRA_SLEEP_PRO_A6;
        slp_prot_addr[6] = BGF_CONN_INFRA_SLEEP_PRO_A7;
        slp_prot_addr[7] = BGF_CONN_INFRA_SLEEP_PRO_A8;
        slp_prot_addr[8] = BGF_CONN_INFRA_SLEEP_PRO_A9;
        slp_prot_addr[9] = BGF_CONN_INFRA_SLEEP_PRO_A10;
        slp_prot_addr[10] = BGF_CONN_INFRA_SLEEP_PRO_A11;

        //check vndr current state
        vndr_cur_mode[0] = 0x00010001;
        vndr_cur_mode[1] = 0x00020001;
        vndr_cur_mode[2] = 0x00010002;
        vndr_cur_mode[3] = 0x00020002;
        vndr_cur_mode[4] = 0x00010003;
        vndr_cur_mode[5] = 0x00020003;
        vndr_cur_mode[6] = 0x00010004;
        vndr_cur_mode[7] = 0x00020004;
        for (i = 0; i < 8; i++) {
            CONSYS_REG_WRITE(BGF_CONN_VNDR_CUR_ST_WRITE, vndr_cur_mode[i]);
            vndr_cur_st[i] = CONSYS_REG_READ(BGF_CONN_VNDR_CUR_ST_READ);
        }
        LOG_I(connsys_dbg, "vndr current state, write addr: 0x%08x, read addr: 0x%08x",
              BGF_CONN_VNDR_CUR_ST_WRITE, BGF_CONN_VNDR_CUR_ST_READ);
        LOG_I(connsys_dbg, "dump w_val and r_val:");
        for (i = 0; i < 8; i += 4)
            LOG_I(connsys_dbg, "%08x %08x   %08x %08x   %08x %08x   %08x %08x",
                  vndr_cur_mode[i], vndr_cur_st[i], vndr_cur_mode[i + 1], vndr_cur_st[i + 1],
                  vndr_cur_mode[i + 2], vndr_cur_st[i + 2], vndr_cur_mode[i + 3], vndr_cur_st[i + 3]);

        // 1.1 check conn_infra wake up
        if (BBIT(power_st, 8) && BBIT(power_st, 9) && RBBIT(power_st, 10) &&
            BBIT(power_st, 11)) {
            LOG_I(connsys_dbg, "act before wakup(0x%08lx) = 0x%08lx",
                  CONN_INFRA_CHECK_ACTION_BEFORE_WAKEUP, CONSYS_REG_READ(CONN_INFRA_CHECK_ACTION_BEFORE_WAKEUP));
            LOG_I(connsys_dbg, "conn_infra wakeup ok! dump ahb_apb, ao_st, slp_pro info:");
            LOG_I(connsys_dbg, "addr and val:");
            for (i = 0; i < 6; i++) {
                ahb_apb_tout[i] = CONSYS_REG_READ(ahb_apb_tout_addr[i]);
                if (i == 2 || i == 5)
                    LOG_I(connsys_dbg, "%08lx %08lx   %08lx %08lx   %08lx %08lx",
                          ahb_apb_tout_addr[i - 2], ahb_apb_tout[i - 2], ahb_apb_tout_addr[i - 1], ahb_apb_tout[i - 1],
                          ahb_apb_tout_addr[i], ahb_apb_tout[i]);
            }
            for (i = 0; i < 8; i++) {
                ao_st_val[i] = CONSYS_REG_READ(BGF_CONN_INFRA_DEBUG_AO_ST_BASE + i * 4);
                if (i == 3 || i == 7)
                    LOG_I(connsys_dbg, "%08x %08lx   %08x %08lx   %08x %08lx   %08x %08lx",
                          BGF_CONN_INFRA_DEBUG_AO_ST_BASE + (i - 3) * 4, ao_st_val[i - 3],
                          BGF_CONN_INFRA_DEBUG_AO_ST_BASE + (i - 2) * 4, ao_st_val[i - 2],
                          BGF_CONN_INFRA_DEBUG_AO_ST_BASE + (i - 1) * 4, ao_st_val[i - 1],
                          BGF_CONN_INFRA_DEBUG_AO_ST_BASE + i * 4, ao_st_val[i]);
            }
            for (i = 0; i < 8; i++) {
                slp_prot[i] = CONSYS_REG_READ(slp_prot_addr[i]);
                if (i == 3 || i == 7)
                    LOG_I(connsys_dbg, "%08lx %08lx   %08lx %08lx   %08lx %08lx   %08lx %08lx",
                          slp_prot_addr[i - 3], slp_prot[i - 3], slp_prot_addr[i - 2], slp_prot[i - 2],
                          slp_prot_addr[i - 1], slp_prot[i - 1], slp_prot_addr[i], slp_prot[i]);
            }
            LOG_I(connsys_dbg, "%08lx %08lx   %08lx %08lx   %08lx %08lx", slp_prot_addr[8], slp_prot[8],
                  slp_prot_addr[9], slp_prot[9], slp_prot_addr[10], slp_prot[10]);

            // 1.2 check wfsys wake up
            if (BBIT(power_st, 16) && BBIT(power_st, 17) && BBIT(power_st, 18) &&
                BBIT(power_st, 19) && RBBIT(power_st, 20) && BBIT(power_st, 21)) {
                LOG_I(connsys_dbg, "conn_infra wfsys wakeup ok!");
            } else {
                LOG_E(connsys_dbg, "conn_infra wfsys wakeup fail!");
            }
            // 1.3 check bgfsys wake up
            if (BBIT(power_st, 24) && BBIT(power_st, 25) && BBIT(power_st, 26) &&
                BBIT(power_st, 27) && RBBIT(power_st, 28) && BBIT(power_st, 29)) {
                LOG_I(connsys_dbg, "conn_infra bgfsys wakeup ok!");
                if (BBIT(bus_clk_val, 1) && BBIT(bus_clk_val, 2) &&
                    (ip_ver == BGF_CONN_INFRA_IP_VERSION))
                    LOG_I(connsys_dbg, "conn_infra off bus clk and ip ver ok!");
                else
                    LOG_E(connsys_dbg, "conn_infra off bus clk or ip ver fail! dump apb, ao:");
            } else {
                LOG_E(connsys_dbg, "conn_infra bgfsys wakeup fail!");
                //wake up check
            }
        } else {
            LOG_E(connsys_dbg, "conn_infra wakeup fail!");
        }
        return -1;
    }

    //Use UNUSED() to fix build warning
    UNUSED(vndr_cur_st);
    UNUSED(ahb_apb_tout);
    UNUSED(ao_st_val);
    UNUSED(slp_prot);

    LOG_I(connsys_dbg, "check ap2conn_infra_off ok");
    return 0;
}

#ifdef MTK_MT7933_WIFI_ENABLE
int dump_wfsys_hang_reg(void)
{
    uint32_t wf_irq_val;

    /* if wifi_irq 1, maybe wifi hang; if 0, maybe bt hang */
    wf_irq_val = CONSYS_REG_READ(BGF_CONN_WF_MCUSYS_BUS_HANG_IRQ);
    LOG_I(connsys_dbg, "check AP2WF readable:\n"
          " wf_bus_hang_irq_status(0x%08x) = 0x%08lx[0]",
          BGF_CONN_WF_MCUSYS_BUS_HANG_IRQ, wf_irq_val);
    if (BBIT(wf_irq_val, 0)) {
        LOG_I(connsys_dbg, "might be hang at wifi");
        mt7933_dump_wsys_hang_reg();
        return -1;
    } else {
        LOG_I(connsys_dbg, "might be hang at bt");
        return 0;
    }
}
#endif /* #ifdef MTK_MT7933_WIFI_ENABLE */

int dump_bgfsys_hang_reg(void)
{
    btmtk_dump_bgfsys_hang_reg();
    return 0;
}

