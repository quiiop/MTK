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
#include "hal_irrx.h"
#include "hal_gpio.h"
#if defined(UT_HAL_ENABLE) && defined (UT_HAL_IRRX_MODULE_ENABLE)
#define IRRX_CH_ORDINV             ((uint32_t)(1 << 4))
#define MTK_RC5_CONFIG         (IRRX_CH_ORDINV)
#define TERMINATE_THROD     ((uint32_t)10200)
typedef struct pwd_data {
    uint8_t PwNum;
    uint8_t pucPWBuf[4];
    uint8_t ucBufLen;
    uint8_t received_length;
} PWD_DATA;

hal_irrx_pwd_config_t pwd_config = {
    .inverse = 1,
    .terminate_threshold = TERMINATE_THROD
};

hal_irrx_rc5_code_t ir_rc5_rx;

void halIrRxRC5_callback(hal_irrx_event_t event, void  *user_data)
{
    hal_irrx_status_t rc5_status, irrx_status;
    hal_irrx_rc5_code_t *rc5_rx_data = (hal_irrx_rc5_code_t *)user_data;

    irrx_status = hal_irrx_deinit();
    if (HAL_IRRX_STATUS_OK  == irrx_status)
        printf("irrx_deinit success.\n");
    else {
        printf("irrx_deinit not support status.\n");
        return;
    }

    rc5_status = hal_irrx_receive_rc5(rc5_rx_data);
    if (HAL_IRRX_STATUS_OK  == rc5_status)
        printf("receive_rc5 success.\n");
    else {
        printf("receive_rc5 not support status.\n");
        return;
    }
}
void halIrRxPWD_callback(hal_irrx_event_t event, void  *user_data)
{
    PWD_DATA *pwd_rx_data = (PWD_DATA *)user_data;

    hal_irrx_receive_pwd(&(pwd_rx_data->received_length)
                         , pwd_rx_data->pucPWBuf, pwd_rx_data->ucBufLen);
}

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
ut_status_t ut_hal_irrx(void)
{
    hal_irrx_status_t irrx_status;
    hal_gpio_status_t gpio_status;
    hal_pinmux_status_t pinmux_status;

    gpio_status = hal_gpio_init(HAL_GPIO_16);
    if (HAL_GPIO_STATUS_OK == gpio_status)
        printf("gpio init sucess.\n");
    else {
        printf("gpio not support status .\n");
        return UT_STATUS_NOT_SUPPORT;
    }

    pinmux_status = hal_pinmux_set_function(HAL_GPIO_16, 3); //Sets the GPIO16 to IRRX mode.
    if (HAL_PINMUX_STATUS_OK == pinmux_status)
        printf("pinmux init sucess.\n");
    else {
        printf("pinmux not support status .\n");
        return UT_STATUS_NOT_SUPPORT;
    }

    irrx_status = hal_irrx_init();
    if (HAL_IRRX_STATUS_OK  == irrx_status)
        printf("irrx init sucess.\n");
    else {
        printf("irrx not support status .\n");
        return UT_STATUS_NOT_SUPPORT;
    }

    irrx_status = hal_irrx_receive_rc5_start(MTK_RC5_CONFIG,
                                             halIrRxRC5_callback, (void *)&ir_rc5_rx);
    if (HAL_IRRX_STATUS_OK  == irrx_status)
        printf("rc5 start init sucess.\n");
    else {
        printf("IRRX not support status .\n");
        return UT_STATUS_NOT_SUPPORT;
    }

    return UT_STATUS_OK;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_IRRX_MODULE_ENABLE) */
