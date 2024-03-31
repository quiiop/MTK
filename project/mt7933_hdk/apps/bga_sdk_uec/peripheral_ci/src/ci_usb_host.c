/* Copyright Statement:
 *
 * (C) 2005-2030  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ci.h"
#include "ci_cli.h"
#include "hal.h"
#include "hal_gpt.h"
#include "usb.h"
#include "ssusb_host_export.h"
#include "ssusb_host_msc.h"

extern hci_t *global_hcd;
#define MAX_BUFF (65536 * 2)
#define SECTOR_NUM 256

ci_status_t ci_usb_host_init_exit_sample(void)
{
    int ret, i;
    usbdev_t *dev = NULL;
    unsigned char *bufout;
    unsigned char *bufin;

#if 0
    hal_gpio_data_t data;

    hal_pinmux_set_function(HAL_GPIO_35, 0);
    hal_gpio_set_direction(HAL_GPIO_35, HAL_GPIO_DIRECTION_OUTPUT);
    //hal_gpio_set_output(HAL_GPIO_35, HAL_GPIO_DATA_HIGH);
    hal_gpio_set_output(HAL_GPIO_35, HAL_GPIO_DATA_LOW);
    hal_gpio_get_input(HAL_GPIO_35, &data);
    usb_crit("USB HOST SWITCHT GPIO35(HIGH):%d !\n", data);
#endif /* #if 0 */

    EXPECT_VAL(mtk_usb_host_init(0), 0);
    hal_gpt_delay_ms(1000);
    dev = global_hcd->devices[1];

    if (dev == NULL) {
        usb_err("No Disk Found!\n");
        return CI_FAIL;
    }

    EXPECT_VAL(get_msc_device(), dev);
    if (get_capacity(dev) < 1) {
        usb_err("get_capacity test fail\n");
        return CI_FAIL;
    }

    if (get_block_size(dev) < 1) {
        usb_err("get_block_size test fail\n");
        return CI_FAIL;
    }

    EXPECT_VAL(erase_sector(dev, 60000, 100), 0);

    bufout = (unsigned char *)pvPortMalloc(MAX_BUFF);
    if (!bufout) {
        usb_err("%s alloc bufout fail\r\n", __func__);
        return CI_FAIL;
    }

    bufin = (unsigned char *)pvPortMalloc(MAX_BUFF);
    if (!bufin) {
        usb_err("%s alloc bufin fail\r\n", __func__);
        vPortFreeNC((void *)bufout);
        return CI_FAIL;
    }

    memset(bufout, 0, MAX_BUFF);
    memset(bufin, 0, MAX_BUFF);
    for (i = 0; i < MAX_BUFF; i++) {
        bufout[i] = (i % 255);
        if ((i % 8192) == 0)
            usb_debug("buf[%d]:%d     \r\n", i, bufout[i]);
    }

    ret = readwrite_blocks_512(dev, 60000, SECTOR_NUM, cbw_direction_data_out, bufout);
    if (ret) {
        usb_err("write data to udisk fail\n");
        goto error;
    }

    ret = readwrite_blocks_512(dev, 60000, SECTOR_NUM, cbw_direction_data_in, bufin);
    if (ret) {
        usb_err("read data to udisk fail\n");
        goto error;
    }

    usb_debug("\n ");
    usb_debug("\n ");
    usb_debug("\n ");
    for (i = 0; i < MAX_BUFF; i++) {
        if ((i % 8192) == 0)
            usb_debug("buf[%d]:%d     \r\n", i, bufin[i]);
    }

    ret = memcmp(bufout, bufin, SECTOR_NUM * 512);
    if (ret) {
        usb_err("read/write test fail \r\n");
        goto error;
    }

    usb_crit("read/write test PASS \r\n");
    vPortFree((void *)bufin);
    vPortFree((void *)bufout);

    EXPECT_VAL(mtk_usb_host_deinit(0), 0);

    return CI_PASS;

error:
    vPortFree((void *)bufin);
    vPortFree((void *)bufout);
    return CI_FAIL;
}


ci_status_t ci_usb_host_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Host Init/Emu Disk/Exit Test", ci_usb_host_init_exit_sample},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
