/*
 * MediaTek Inc. (C) 2019. All rights reserved.
 *
 * Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
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
#if defined(MTK_AUXADCCLI_ENABLE)

#include <stdint.h>
#include <stdlib.h>

#include "cli.h"
#include "os_hal_adc.h"

#define ADC_RX_BUF_SIZE   32

uint32_t RX_DATA[ADC_CHANNEL_MAX][ADC_RX_BUF_SIZE];
uint32_t LENGTH;

void *ts_adc_init(u8 len, char *param[])
{
    u8 pmode = 0;
    u8 fifo_mode = 0;
    u16 bit_map = 0;
    int ret = 0;

    pmode = strtoul(param[0], NULL, 0);
    fifo_mode = strtoul(param[1], NULL, 0);
    bit_map = strtoul(param[2], NULL, 0);

    ret = mtk_os_hal_adc_ctlr_init((adc_pmode)pmode,
                                   (adc_fifo_mode)fifo_mode, bit_map);
    if (ret)
        printf("adc init fail\n");
    else
        printf("adc init success\n");

    return (void *)(intptr_t) ret;
}

void *ts_adc_start(u8 len, char *param[])
{
    int ret = 0;

    ret = mtk_os_hal_adc_start();

    return (void *)(intptr_t) ret;
}

void *ts_adc_start_ch(u8 len, char *param[])
{
    u8 ch_bit_map = 0;
    int ret = 0;

    ch_bit_map = strtoul(param[0], NULL, 0);

    ret = mtk_os_hal_adc_start_ch(ch_bit_map);

    return (void *)(intptr_t) ret;
}

void *ts_adc_deinit(u8 len, char *param[])
{
    int ret = 0;

    ret = mtk_os_hal_adc_ctlr_deinit();

    return (void *)(intptr_t) ret;
}

void *ts_adc_fsm_param_set(u8 len, char *param[])
{
    struct adc_fsm_param adc_fsm_parameter;
    int ret = 0;

    adc_fsm_parameter.pmode = (adc_pmode)strtoul(param[0], NULL, 0);
    adc_fsm_parameter.avg_mode = (adc_avg_mode)strtoul(param[1], NULL, 0);
    adc_fsm_parameter.channel_map = strtoul(param[2], NULL, 0);
    adc_fsm_parameter.period = strtoul(param[3], NULL, 0);
    adc_fsm_parameter.fifo_mode =
        (adc_fifo_mode) strtoul(param[4], NULL, 0);
    adc_fsm_parameter.ier_mode =
        (adc_fifo_ier_mode)strtoul(param[5], NULL, 0);

    adc_fsm_parameter.dma_vfifo_len = ADC_DMA_BUF_WORD_SIZE;

    printf("adc_fsm_parameter.pmode:%d\n", adc_fsm_parameter.pmode);
    printf("adc_fsm_parameter.avg_mode:%d\n",
           adc_fsm_parameter.avg_mode);
    printf("adc_fsm_parameter.channel_map:%d\n",
           adc_fsm_parameter.channel_map);
    printf("adc_fsm_parameter.period:0x%x\n", adc_fsm_parameter.period);
    printf("adc_fsm_parameter.fifo_mode:%d\n",
           adc_fsm_parameter.fifo_mode);
    printf("adc_fsm_parameter.ier_mode:%d\n",
           adc_fsm_parameter.ier_mode);


    ret = mtk_os_hal_adc_fsm_param_set(&adc_fsm_parameter);

    return (void *)(intptr_t) ret;
}

void *ts_adc_one_shot_get_data(u8 len, char *param[])
{
    adc_channel sample_channel = ADC_CHANNEL_0;
    u32 data;
    int ret = 0;

    sample_channel = (adc_channel)strtoul(param[0], NULL, 0);

    if (sample_channel >= ADC_CHANNEL_MAX)
        return (void *) - ADC_EPARAMETER;

    ret = mtk_os_hal_adc_one_shot_get_data(sample_channel, &data);

    return (void *)(intptr_t) ret;
}

void *ts_adc_period_get_data(u8 len, char *param[])
{
    adc_channel sample_channel = ADC_CHANNEL_0;
    int ret = 0;

    sample_channel = (adc_channel)strtoul(param[0], NULL, 0);

    if (sample_channel >= ADC_CHANNEL_MAX)
        return (void *) - ADC_EPARAMETER;

    ret = mtk_os_hal_adc_period_get_data((u32(*)[32])RX_DATA[0],
                                         (u32 *)&LENGTH);
    printf("CH%d: %d\n", ADC_CHANNEL_1, (int)RX_DATA[ADC_CHANNEL_1][0]);

    return (void *)(intptr_t) ret;
}

static cmd_t   adc_cli[] = {
    {
        "init", "ts_adc_init [#1]adc_pmode [#2]adc_fifo_mode",
        ts_adc_init, NULL
    },
    { "start", "ts_adc_start ", ts_adc_start, NULL },
    { "startch", "ts_adc_start_ch [#1]adc_pmode", ts_adc_start_ch, NULL },
    { "deinit", "ts_adc_deinit ", ts_adc_deinit, NULL },
    {
        "params",
        "ts_adc_fsm_param_set [#1]pmode [#2]avg_mode [#3]channel_map [#4]period [#5]fifo_mode [#6]ier_mode",
        ts_adc_fsm_param_set, NULL
    },
    {
        "oneshot",  "ts_adc_one_shot_get_data [#1]sample_channel",
        ts_adc_one_shot_get_data, NULL
    },
    { "period", "ts_adc_period_get_data", ts_adc_period_get_data, NULL },
    { NULL, NULL, NULL, NULL  }
};

CLI_CMD(adc, "ADC items", NULL, adc_cli);

#endif
