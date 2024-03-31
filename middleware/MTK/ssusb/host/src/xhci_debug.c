/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2013 secunet Security Networks AG
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <cli.h>
#include <FreeRTOS.h>
#include <timers.h>
#include <hal_usb_xhci_rscs.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "generic_hub.h"
#include "usb.h"
#include "usbhid.h"
#include "usbmsc.h"
#include "usbuvc.h"
#include "usb_phy.h"
#include "ssusb_hw_regs.h"
#include "xhci_private.h"
#include "xhci_mtk.h"
#include "ssusb_gadget_export.h"
#include "ssusb_host_export.h"
#include "uac.h"

#ifdef XHCI_DUMPS

void xhci_dump_slotctx(const slotctx_t *const sc)
{
    usb_debug("Slot Context (@%p):\n", sc);
    usb_debug(" FIELD1\t0x%08x\n", sc->f1);
    usb_debug(" FIELD2\t0x%08x\n", sc->f2);
    usb_debug(" FIELD3\t0x%08x\n", sc->f3);
    usb_debug(" FIELD4\t0x%08x\n", sc->f4);
    SC_DUMP(ROUTE,  sc);
    SC_DUMP(SPEED1, sc);
    SC_DUMP(MTT,    sc);
    SC_DUMP(HUB,    sc);
    SC_DUMP(CTXENT, sc);
    SC_DUMP(RHPORT, sc);
    SC_DUMP(NPORTS, sc);
    SC_DUMP(TTID,   sc);
    SC_DUMP(TTPORT, sc);
    SC_DUMP(TTT,    sc);
    SC_DUMP(UADDR,  sc);
    SC_DUMP(STATE,  sc);
}

void xhci_dump_epctx(const epctx_t *const ec)
{
    usb_debug("Endpoint Context (@%p):\n", ec);
    usb_debug(" FIELD1\t0x%08x\n", ec->f1);
    usb_debug(" FIELD2\t0x%08x\n", ec->f2);
    usb_debug(" TRDQ_L\t0x%08x\n", ec->tr_dq_low);
    usb_debug(" TRDQ_H\t0x%08x\n", ec->tr_dq_high);
    usb_debug(" FIELD5\t0x%08x\n", ec->f5);
    EC_DUMP(STATE,  ec);
    EC_DUMP(INTVAL, ec);
    EC_DUMP(CERR,   ec);
    EC_DUMP(TYPE,   ec);
    EC_DUMP(MBS,    ec);
    EC_DUMP(MPS,    ec);
    EC_DUMP(DCS,    ec);
    EC_DUMP(AVRTRB, ec);
    EC_DUMP(MAXESITLO, ec);
}

void xhci_dump_devctx(const devctx_t *const dc, const u32 ctx_mask)
{
    int i;

    if (ctx_mask & 1)
        xhci_dump_slotctx(dc->slot);
    for (i = 1; i <= SC_GET(CTXENT, dc->slot); ++i) {
        if (ctx_mask & (1 << i))
            xhci_dump_epctx(dc->ep[i]);
    }
}

void xhci_dump_inputctx(const inputctx_t *const ic)
{
    usb_debug("Input Control  add: 0x%08x\n", *ic->add);
    usb_debug("Input Control drop: 0x%08x\n", *ic->drop);
    xhci_dump_devctx(&ic->dev, *ic->add);
}

void xhci_dump_transfer_trb(const trb_t *const cur)
{
    usb_debug("Transfer TRB (@%p):\n", cur);
    usb_debug(" PTR_L\t0x%08x\n", cur->ptr_low);
    usb_debug(" PTR_H\t0x%08x\n", cur->ptr_high);
    usb_debug(" STATUS\t0x%08x\n", cur->status);
    usb_debug(" CNTRL\t0x%08x\n", cur->control);
    TRB_DUMP(TL,    cur);
    TRB_DUMP(TDS,   cur);
    TRB_DUMP(C, cur);
    TRB_DUMP(ISP,   cur);
    TRB_DUMP(CH,    cur);
    TRB_DUMP(IOC,   cur);
    TRB_DUMP(IDT,   cur);
    TRB_DUMP(TT,    cur);
    TRB_DUMP(DIR,   cur);
}

static const trb_t *xhci_next_trb(const trb_t *const cur)
{
    if (TRB_GET(TT, cur) == TRB_LINK)
        return (!cur->ptr_low) ? NULL : phys_to_virt(cur->ptr_low);
    else
        return cur + 1;
}

void xhci_dump_transfer_trbs(const trb_t *const first, const trb_t *const last)
{
    const trb_t *cur;

    for (cur = first; cur; cur = xhci_next_trb(cur)) {
        xhci_dump_transfer_trb(cur);
        if (cur == last)
            break;
    }
}

#endif /* #ifdef XHCI_DUMPS */

#define INSTANCE_INDEX 0
extern hci_t *global_hcd;

#define MAX_BUFF (65536 * 2)

static uint8_t t_ssusb_enable(uint8_t len, char *param[])
{
    mtk_usb_host_init(0);
    serial_usb_init();
    return 0;
}

extern int udc_deinit(u8 index);
static uint8_t t_ssusb_disable(uint8_t len, char *param[])
{
    serial_usb_exit();
    udc_deinit(0);
    mtk_usb_host_deinit(0);
    return 0;
}

static uint8_t t_lsusb(uint8_t len, char *param[])
{
    bool msc_dev;
    int i, j, ret;
    usbdev_t *dev;
    static const char *class_name;
    interface_descriptor_t *interface_t;
    static const char *const speeds[] = { "Full", "Low", "High", "Super", "Ultra" };
    static const char *const class_device[] = { "Audio", "Comm", "HID", "Physical",
                                                "Image", "Printer", "MSC", "HUB", "CDC",
                                                "CCID", "Security", "Vedio", "Healthcare",
                                                "Diagnostic", "Wireless", "MISC"
                                              };

    static int class_table[16][2] = {
        {0x01, 0}, /*audio_device*/
        {0x02, 1}, /*comm_device*/
        {0x03, 2}, /*hid_device*/
        {0x05, 3}, /*physical_device*/
        {0x06, 4}, /*imaging_device*/
        {0x07, 5}, /*printer_device*/
        {0x08, 6}, /*msc_device*/
        {0x09, 7}, /*hub_device*/
        {0x0a, 8}, /*cdc_device*/
        {0x0b, 9}, /*ccid_device*/
        {0x0d, 10}, /*security_device*/
        {0x0e, 11}, /*video_device*/
        {0x0f, 12}, /*healthcare_device*/
        {0xdc, 13}, /*diagnostic_device*/
        {0xe0, 14}, /*wireless_device*/
        {0xef, 15} /*misc_device*/
    };

    if (len > 1) {
        usb_err("Example: lsusb\r\nlsusb msc\r\n");
        return -1;
    }

    if (len == 1) {
        ret = memcmp("msc", param[0], 3);
        if (ret) {
            usb_err("Example: lsusb msc\r\n");
            return -1;
        }
    }

    for (i = 1; i < 128; i++) {
        msc_dev = false;
        dev = global_hcd->devices[i];
        if (dev) {
            class_name = "unknown";
            for (j = 0; j < 16; j++) {
                /* search class ID by device descriptor */
                if (dev->descriptor->bDeviceClass == class_table[j][0])
                    class_name = class_device[class_table[j][1]];

                if (dev->descriptor->bDeviceClass == class_table[6][0])
                    msc_dev = true;
            }

            if (dev->descriptor->bDeviceClass == 0) {
                /* if unknown class , search class ID by interface again */
                interface_t = (interface_descriptor_t *)
                              (((char *)dev->configuration) +
                               dev->configuration->bLength);
                for (j = 0 ; j < 16; j++) {
                    if (interface_t->bInterfaceClass == class_table[j][0])
                        class_name = class_device[class_table[j][1]];

                    if (interface_t->bInterfaceClass == class_table[j][0])
                        msc_dev = true;
                }
            }

            if (len && msc_dev) {
                usb_crit("dev[%d]@ %p , %s Device (idVed=%04x , idPud=%04x)\n",
                         i, dev, class_name, dev->descriptor->idVendor,
                         dev->descriptor->idProduct);
                usb_crit("       |__ Epnum(%d), Address(%d), Link to Hub(%d)"
                         "port(%d)-> %s Speed\n",
                         dev->num_endp,
                         dev->address,
                         dev->hub,
                         dev->port,
                         (dev->speed < (usb_speed)(sizeof(speeds) / sizeof(char *)))
                         ? speeds[dev->speed] : "unknown");
            }

            if (len == 0) {
                usb_crit("dev[%d]@ %p , %s Device (idVed=%04x , idPud=%04x)\n",
                         i, dev, class_name, dev->descriptor->idVendor,
                         dev->descriptor->idProduct);
                usb_crit("       |__ Epnum(%d), Address(%d), Link to Hub(%d)"
                         "port(%d)-> %s Speed\n",
                         dev->num_endp,
                         dev->address,
                         dev->hub,
                         dev->port,
                         (dev->speed < (usb_speed)(sizeof(speeds) / sizeof(char *)))
                         ? speeds[dev->speed] : "unknown");
            }
        }
    }

    return 0;
}

static uint8_t t_test_udisk(uint8_t len, char *param[])
{
    int sector, sector_num;
    int ret, i;
    int port_id = 0;
    usbdev_t *roothub;
    usbdev_t *dev;
    generic_hub_t *hub;
    unsigned char *bufout;
    unsigned char *bufin;
    hci_t *controller = global_hcd;
    struct xhci_hcd_mtk *xhci_mtk = &mtk_hcd[INSTANCE_INDEX];

    if (len != 3) {
        usb_err("Example: ssusb test.udisk 1(port_id)"
                "60000(first_sector) 256(number_of_sector)\r\n");
        return -1;
    }

    port_id = atoi(param[0]);
    port_id += xhci_mtk->num_u3_ports;

    port_id = port_id + 1;
    roothub = controller->devices[0];
    hub = GEN_HUB(roothub);

    dev = controller->devices[hub->ports[port_id]];
    if (!dev) {
        usb_err("%s : Error, No Udisk Found! \r\n", __func__);
        return -1;
    }

    usb_crit("%s dev:%p hub->ports[port_id]:%x hub:%p\r\n", __func__,
             dev, hub->ports[port_id], hub);
    sector = atoi(param[1]);
    sector_num = atoi(param[2]);
    usb_crit("%s begin sector:%d; number of sector:%d\n\r",
             __func__, sector, sector_num);
    bufout = (unsigned char *)pvPortMalloc(MAX_BUFF);
    if (!bufout) {
        usb_err("%s alloc bufout fail\r\n", __func__);
        return -1;
    }

    bufin = (unsigned char *)pvPortMalloc(MAX_BUFF);
    if (!bufin) {
        usb_err("%s alloc bufin fail\r\n", __func__);
        vPortFreeNC((void *)bufout);
        return -1;
    }

    if (sector_num * 512 > MAX_BUFF) {
        usb_err("%s: too many sector number, fail\r\n", __func__);
        goto error;
    }

    memset(bufout, 0, MAX_BUFF);
    memset(bufin, 0, MAX_BUFF);
    for (i = 0; i < MAX_BUFF; i++) {
        bufout[i] = (i % 255);
        if ((i % 8192) == 0)
            usb_crit("buf[%d]:%d	 \r\n", i, bufout[i]);
    }
    ret = readwrite_blocks_512(dev, sector, sector_num, cbw_direction_data_out, bufout);
    if (ret) {
        usb_err("write data to udisk fail\n");
        goto error;
    }

    ret = readwrite_blocks_512(dev, sector, sector_num, cbw_direction_data_in, bufin);
    if (ret) {
        usb_err("read data to udisk fail\n");
        goto error;
    }

    usb_crit("\n ");
    usb_crit("\n ");
    usb_crit("\n ");
    for (i = 0; i < MAX_BUFF; i++) {
        if ((i % 8192) == 0)
            usb_crit("buf[%d]:%d	 \r\n", i, bufin[i]);
    }

    ret = memcmp(bufout, bufin, sector_num * 512);
    if (ret) {
        usb_err("read/write test fail \r\n");
        goto error;
    }

    usb_crit("read/write test PASS \r\n");
    vPortFree((void *)bufin);
    vPortFree((void *)bufout);
    return 0;

error:
    vPortFree((void *)bufin);
    vPortFree((void *)bufout);
    return -1;
}

static uint8_t t_test_keyboard(uint8_t len, char *param[])
{
    unsigned short c;

    if (usbhid_havechar()) {
        c = usbhid_getchar();
        usb_crit("%s value:%c\n", __func__, c);
        if (c != 0)
            return c;
    }

    return 0;
}

static uint8_t t_test_j(uint8_t len, char *param[])
{
    int port_id = 0;
    struct xhci_hcd_mtk *xhci_mtk = &mtk_hcd[INSTANCE_INDEX];
    void *addr = xhci_mtk->xhci_base + PORTPMSC;

    if (len != 1) {
        usb_err("Example: xhci test.j port_id(0/1/2) \r\n");
        return -1;
    }

    port_id = atoi(param[0]);
    port_id += xhci_mtk->num_u3_ports;
    ssusb_clrbits(addr, port_id * 0x10, 0xf << 28);
    mdelay(10);
    ssusb_setbits(addr, port_id * 0x10, 0x1 << 28);
    usb_crit("port%x[%p]:%x \r\n", port_id, addr + (port_id * 0x10),
             ssusb_readl(addr, port_id * 0x10));
    return 0;
}

static uint8_t t_test_k(uint8_t len, char *param[])
{
    int port_id = 0;
    struct xhci_hcd_mtk *xhci_mtk = &mtk_hcd[INSTANCE_INDEX];
    void *addr = xhci_mtk->xhci_base + PORTPMSC;

    if (len != 1) {
        usb_err("Example: xhci test.k port_id(0/1/2) \r\n");
        return -1;
    }

    port_id = atoi(param[0]);
    port_id += xhci_mtk->num_u3_ports;
    ssusb_clrbits(addr, port_id * 0x10, 0xf << 28);
    mdelay(10);
    ssusb_setbits(addr, port_id * 0x10, 0x2 << 28);
    usb_crit("port%x[%p]:%x \r\n", port_id, addr + (port_id * 0x10),
             ssusb_readl(addr, port_id * 0x10));
    return 0;
}

static uint8_t t_test_se0(uint8_t len, char *param[])
{
    int port_id = 0;
    struct xhci_hcd_mtk *xhci_mtk = &mtk_hcd[INSTANCE_INDEX];
    void *addr = xhci_mtk->xhci_base + PORTPMSC;

    if (len != 1) {
        usb_err("Example: xhci test.se0 port_id(0/1/2) \r\n");
        return -1;
    }

    port_id = atoi(param[0]);
    port_id += xhci_mtk->num_u3_ports;
    ssusb_clrbits(addr, port_id * 0x10, 0xf << 28);
    mdelay(10);
    ssusb_setbits(addr, port_id * 0x10, 0x3 << 28);
    usb_crit("port%x[%p]:%x \r\n", port_id, addr + (port_id * 0x10),
             ssusb_readl(addr, port_id * 0x10));
    return 0;
}

static uint8_t t_test_packet(uint8_t len, char *param[])
{
    int port_id = 0;
    struct xhci_hcd_mtk *xhci_mtk = &mtk_hcd[INSTANCE_INDEX];
    void *addr = xhci_mtk->xhci_base + PORTPMSC;

    if (len != 1) {
        usb_err("Example: xhci test.packet port_id(0/1/2) \r\n");
        return -1;
    }

    port_id = atoi(param[0]);
    port_id += xhci_mtk->num_u3_ports;
    ssusb_clrbits(addr, port_id * 0x10, 0xf << 28);
    mdelay(10);
    ssusb_setbits(addr, port_id * 0x10, 0x4 << 28);
    usb_crit("port%x[%p]:%x \r\n", port_id, addr + (port_id * 0x10),
             ssusb_readl(addr, port_id * 0x10));
    return 0;
}

static uint8_t t_test_suspend(uint8_t len, char *param[])
{
    u32 status;
    int ret;
    int port_id = 0;
    struct xhci_hcd_mtk *xhci_mtk = &mtk_hcd[INSTANCE_INDEX];
    void *addr = xhci_mtk->xhci_base + PORTSC;

    if (len != 1) {
        usb_err("Example: xhci test.suspend port_id(0/1/2) \r\n");
        return -1;
    }

    port_id = atoi(param[0]);
    port_id += xhci_mtk->num_u3_ports;
    status = ssusb_readl(addr, port_id * 0x10);
    status = xhci_port_state_to_neutral(status);
    status = (status & ~(0xf << 5));
    status = (status | (0x3 << 5) | (1 << 16));
    ssusb_writel(addr, port_id * 0x10, status);

    ret = xhci_wait_for_value(addr + (port_id * 0x10), (0xf << 5), (3 << 5), 10, 3000);
    if (ret) {
        usb_err("Suspend Timeout \r\n");
        return -1;
    }

    status = ssusb_readl(addr, port_id * 0x10);
    status = (status >> 5) & 0xf;
    if (status != 3)
        usb_err("port[%d] not enter suspend state \r\n", port_id);
    else
        usb_crit("port[%d] enter suspend state \r\n", port_id);

    return 0;
}

static uint8_t t_test_resume(uint8_t len, char *param[])
{
    int ret;
    u32 status;
    int port_id = 0;
    struct xhci_hcd_mtk *xhci_mtk = &mtk_hcd[INSTANCE_INDEX];
    void *addr = xhci_mtk->xhci_base + PORTSC;

    if (len != 1) {
        usb_err("Example: xhci test.suspend port_id(0/1/2) \r\n");
        return -1;
    }

    port_id = atoi(param[0]);
    port_id += xhci_mtk->num_u3_ports;
    status = ssusb_readl(addr, port_id * 0x10);
    if (((status >> 5) & 0xf) != 3) {
        usb_err("port[%d] not in suspend state, please suspend port first \r\n", port_id);
        return -1;
    }

    status = xhci_port_state_to_neutral(status);
    status = (status & ~(0xf << 5));
    status = (status | (15 << 5) | (1 << 16));
    ssusb_writel(addr, port_id * 0x10, status);
    mdelay(20);

    status = ssusb_readl(addr, port_id * 0x10);
    status = xhci_port_state_to_neutral(status);
    status = (status & ~(0xf << 5));
    status = (status | (1 << 16));
    ssusb_writel(addr, port_id * 0x10, status);
    ret = xhci_wait_for_value(addr + (port_id * 0x10), (0xf << 5), (0 << 5), 10, 10000);
    if (ret) {
        usb_err("Resume Timeout \r\n");
        return -1;
    }

    status = ssusb_readl(addr, port_id * 0x10);
    status = (status >> 5) & 0xf;
    if (status != 0) {
        usb_err("port[%d] resume fail \r\n", port_id);
        return -1;
    }

    usb_crit("port[%d] resume pass \r\n", port_id);
    return 0;
}

static uint8_t t_test_enumbus(uint8_t len, char *param[])
{
    usb_crit("%s\n\r", __func__);
    return 0;
}

static uint8_t t_test_getdesc(uint8_t len, char *param[])
{
    int ret;
    int port_id = 0;
    usbdev_t *roothub;
    usbdev_t *dev;
    generic_hub_t *hub;
    hci_t *controller = global_hcd;
    struct xhci_hcd_mtk *xhci_mtk = &mtk_hcd[INSTANCE_INDEX];

    if (len != 1) {
        usb_err("Example: xhci test.getdesc port_id(0/1/2) \r\n");
        return -1;
    }

    port_id = atoi(param[0]);
    port_id = port_id + 1;
    port_id += xhci_mtk->num_u3_ports;
    roothub = controller->devices[0];
    hub = GEN_HUB(roothub);

    dev = controller->devices[hub->ports[port_id]];
    ret = get_descriptor(dev, DR_DESC, DT_DEV, 0, dev->descriptor, sizeof(*dev->descriptor));
    if (ret != sizeof(*dev->descriptor)) {
        usb_err("get device descriptor fail\r\n");
        return -1;
    }

    usb_crit("get device descriptor pass\r\n");
    return 0;
}

static uint8_t t_read_xhci(uint8_t len, char *param[])
{
    void *addr = NULL;

    if (len != 1) {
        usb_err("Example: xhci.r address\r\n");
        return -1;
    }

    addr = (void *)strtoul(param[0], NULL, 16);
    usb_crit(" %p:0x%x \r\n", addr, ssusb_readl(addr, 0));
    return ssusb_readl(addr, 0);
}

static uint8_t t_write_xhci(uint8_t len, char *param[])
{
    void *addr = NULL;
    int value = 0;

    if (len != 2) {
        usb_err("Example: xhci.w address value\r\n");
        return -1;
    }

    addr = (void *)strtoul(param[0], NULL, 16);
    value = strtoul(param[1], NULL, 16);
    ssusb_writel(addr, 0, value);
    usb_crit(" %p:0x%x \r\n", addr, ssusb_readl(addr, 0));
    return 0;
}

static uint8_t t_ssusb_suspend(uint8_t len, char *param[])
{
    struct xhci_hcd_mtk *mtk = &mtk_hcd[INSTANCE_INDEX];

    if (len != 0) {
        usb_err("Example: suspend\r\n");
        return -1;
    }

    mtk_usb_host_suspend((void *)(mtk));
    return 0;
}

static uint8_t t_ssusb_resume(uint8_t len, char *param[])
{
    struct xhci_hcd_mtk *mtk = &mtk_hcd[INSTANCE_INDEX];

    if (len != 0) {
        usb_err("Example: resume\r\n");
        return -1;
    }

    mtk_usb_host_resume((void *)(mtk));
    return 0;
}

#define BUF_LEN 1024
static void gadget_vcom_test(void *pvParameters)
{
    static char buffer[BUF_LEN];
    int get_len = 0;
    int put_len = 0;

    memset(buffer, 0, BUF_LEN);
    while (1) {
        get_len = serial_usbtty_getcn(buffer, 1024);
        if (get_len > 0) {
            usb_debug("%s: get len:%d \n", __func__, get_len);

            put_len = serial_usbtty_putcn(buffer, get_len);
            if (put_len < 0)
                usb_err("%s: put char error\n", __func__);
        }
    }
}

static uint8_t t_ssusb_gadget_vcom(uint8_t len, char *param[])
{
    xTaskCreate(gadget_vcom_test, "gadget vcom", 1000, (void *)100, 4, NULL);
    return 0;
}

cmd_t ssusb_driver_cli[] = {
    {"enable", "ssusb enable", t_ssusb_enable, NULL},
    {"disable", "ssusb disable", t_ssusb_disable, NULL},
    {"lsusb", "ssusb lsusb", t_lsusb, NULL},
    {
        "test.udisk",
        "ssusb test.udisk 0(port_id) 60000(first_sector) 256(number_of_sector)",
        t_test_udisk, NULL
    },
    {"test.keyboard", "ssusb test.keyboard", t_test_keyboard, NULL},
    {"test.j", "test.j port_id(0/1/2)", t_test_j, NULL},
    {"test.k", "test.k port_id(0/1/2)", t_test_k, NULL},
    {"test.se0", "test.se0 port_id(0/1/2)", t_test_se0, NULL},
    {"test.packet", "test.packet port_id(0/1/2)", t_test_packet, NULL},
    {"test.suspend", "test.suspend port_id(0/1/2)", t_test_suspend, NULL},
    {"test.resume", "test.resume port_id(0/1/2)", t_test_resume, NULL},
    {"test.enumbus", " ", t_test_enumbus, NULL},
    {"test.getdesc", "test.getdesc port_id(0/1/2)", t_test_getdesc, NULL},
    {"test.r", "test.r address", t_read_xhci, NULL},
    {"test.w", "test.w address value", t_write_xhci, NULL},
    {"suspend", "suspend", t_ssusb_suspend, NULL},
    {"resume", "resume", t_ssusb_resume, NULL},
    {"vcom", "gadget vcom enable", t_ssusb_gadget_vcom, NULL},
    {"capture", "uvc capture 1(enable) 640 480 2(count)", t_uvc_capture, NULL},
    {"uac_playback", "uac playback", t_uac_playback, NULL},
    { NULL, NULL, NULL, NULL }
};

