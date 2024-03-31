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
#include "ut.h"

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_UART_MODULE_ENABLE)

#include "hal_uart_internal.h"
#include <stdlib.h>

extern const uint32_t UART_BaseAddr[DRV_SUPPORT_UART_PORTS];
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
ut_status_t ut_hal_uart(void)
{
    int i;
    int port_line = UART_PORT2;//port num to be tested

    if (port_line >= UART_PORT_MAX)
        return UT_STATUS_INVALID_PARAMETER;

    const int buffer_len = UART_BUF_LEN;
    int has_send = 0, has_read = 0;
    int onetime_send = 0, onetime_read = 0;
    int tx_retry_cnt = 0, rx_retry_cnt = 0;
    unsigned int uart_base = UART_BaseAddr[port_line];
    UINT8 *send_buff = (UINT8 *)malloc(buffer_len);
    UINT8 *recv_buff = (UINT8 *)malloc(buffer_len);

    mtk_uart_init_ut(port_line);
    DRV_WriteReg32(UART_MCR(uart_base), UART_MCR_Normal | UART_MCR_LOOPB);//internal loopback test

    for (;;) {
        has_send = 0, has_read = 0;
        tx_retry_cnt = 0, rx_retry_cnt = 0;
        for (i = 0; i < buffer_len; i++)
            send_buff[i] = i;
        for (i = 0; i < buffer_len; i++)
            recv_buff[i] = 0;
        while (1) {
            onetime_send = hal_uart_send_dma(port_line, &(send_buff[has_send]), buffer_len - has_send);
            has_send += onetime_send;
            if (onetime_send)
                printf("onetime_send: %d, has send:%d\r\n", onetime_send, has_send);
            else {
                tx_retry_cnt++;
                printf("tx_retry_cnt = %d\r\n", tx_retry_cnt);
            }
            if ((has_send == buffer_len) || (tx_retry_cnt > UART_RETRY_MAX_CNT)) {
                if (has_send == buffer_len)
                    printf("Finish send\r\n");
                break;
            }
        }
        //vTaskDelay(3000);
        while (1) {
            onetime_read = hal_uart_receive_dma(port_line, &(recv_buff[has_read]), buffer_len - has_read);
            has_read += onetime_read;
            if (onetime_read)
                printf("onetime_read: %d, has read:%d\r\n", onetime_read, has_read);
            else {
                rx_retry_cnt++;
                printf("rx_retry_cnt = %d\r\n", rx_retry_cnt);
            }

            if ((has_read == buffer_len) || (rx_retry_cnt > UART_RETRY_MAX_CNT)) {
                if (has_read == buffer_len)
                    printf("Finish receive\r\n");
                break;
            }
        }
        //check data
        for (i = 0; i < has_read; i++) {
            if (recv_buff[i] != send_buff[i]) {
                printf("Data mis-match, recv[%d] not equal send[%d]\r\n", recv_buff[i], send_buff[i]);
                return UT_STATUS_ERROR;
            }
        }
        printf("DSP Unit Test OK!\r\n");
        //vTaskDelay(1000);
        break;
    }
    return UT_STATUS_OK;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_UART_MODULE_ENABLE) */


