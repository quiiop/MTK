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
#include "hal_gdma_internal.h"  //This .h file is only for MTK internal ut test

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

void GDMA0_ISR_HANDLER(hal_gdma_event_t event, void  *user_data)
{
    //Don't use printf API in ISR, this just a example code.
    printf("[UT_GDMA]GDMA0_ISR_HANDLER\r\n");
    //gdma_clear_irq(), This API is only for MTK internal ut test
    gdma_clear_irq(HAL_GDMA_CHANNEL_0);
}

void GDMA1_ISR_HANDLER(hal_gdma_event_t event, void  *user_data)
{
    printf("[UT_GDMA]GDMA1_ISR_HANDLER\r\n");
    //gdma_clear_irq(), This API is only for MTK internal ut test
    gdma_clear_irq(HAL_GDMA_CHANNEL_1);
}

void GDMA2_ISR_HANDLER(hal_gdma_event_t event, void  *user_data)
{
    printf("[UT_GDMA]GDMA2_ISR_HANDLER\r\n");
    //gdma_clear_irq(), This API is only for MTK internal ut test
    gdma_clear_irq(HAL_GDMA_CHANNEL_2);
}

ci_status_t GDMA_POLLING_TEST(unsigned int *src_addr, unsigned int *dst_addr1, unsigned int *dst_addr2,
                              unsigned int *dst_addr3, unsigned int len)
{
    unsigned int i;
    int ret;

    for (i = 0 ; i < len / (sizeof(unsigned int)); i++) {
        *((unsigned int *)src_addr + i) = i;
    }

    memset(dst_addr1, 0, len);
    memset(dst_addr2, 0, len);
    memset(dst_addr3, 0, len);

    /* GDMA channel 0 polling mode */
    ret = hal_gdma_init(HAL_GDMA_CHANNEL_0);
    if (ret) {
        printf("channel0 hal_gdma_init fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_start_polling(HAL_GDMA_CHANNEL_0, (uint32_t)dst_addr1, (uint32_t)src_addr, len);
    if (ret) {
        printf("channel0 hal_gdma_start_polling fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_stop(HAL_GDMA_CHANNEL_0);
    if (ret) {
        printf("channel0 hal_gdma_stop,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_deinit(HAL_GDMA_CHANNEL_0);
    if (ret) {
        printf("channel0 hal_gdma_deinit fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }

    /* GDMA channel 1 polling mode */
    ret = hal_gdma_init(HAL_GDMA_CHANNEL_1);
    if (ret) {
        printf("channel1 hal_gdma_init fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_start_polling(HAL_GDMA_CHANNEL_1, (uint32_t)dst_addr2, (uint32_t)src_addr, len);
    if (ret) {
        printf("channel1 hal_gdma_start_polling fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_stop(HAL_GDMA_CHANNEL_1);
    if (ret) {
        printf("channel1 hal_gdma_stop,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_deinit(HAL_GDMA_CHANNEL_1);
    if (ret) {
        printf("channel1 hal_gdma_deinit fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }

    /* GDMA channel 2 polling mode */
    ret = hal_gdma_init(HAL_GDMA_CHANNEL_2);
    if (ret) {
        printf("channel2 hal_gdma_init fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_start_polling(HAL_GDMA_CHANNEL_2, (uint32_t)dst_addr3, (uint32_t)src_addr, len);
    if (ret) {
        printf("channel2 hal_gdma_start_polling fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_stop(HAL_GDMA_CHANNEL_2);
    if (ret) {
        printf("channel2 hal_gdma_stop,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_deinit(HAL_GDMA_CHANNEL_2);
    if (ret) {
        printf("channel2 hal_gdma_deinit fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }

    for (i = 0 ; i < len / (sizeof(unsigned int)); i++) {
        if (*((unsigned int *)dst_addr1 + i) != i) {
            printf("channel 0 test fail\r\n");
            printf("0x%p: 0x%x != 0x%x\r\n", (unsigned int *)dst_addr1 + i, *((unsigned int *)dst_addr1 + i), i);
            return CI_FAIL;
        }
    }

    for (i = 0 ; i < len / (sizeof(unsigned int)); i++) {
        if (*((unsigned int *)dst_addr2 + i) != i) {
            printf("channel 1 test fail\r\n");
            printf("0x%p: 0x%x != 0x%x\r\n", (unsigned int *)dst_addr2 + i, *((unsigned int *)dst_addr2 + i), i);
            return CI_FAIL;
        }
    }

    for (i = 0 ; i < len / (sizeof(unsigned int)); i++) {
        if (*((unsigned int *)dst_addr3 + i) != i) {
            printf("channel 2 test fail\r\n");
            printf("0x%p: 0x%x != 0x%x\r\n", (unsigned int *)dst_addr3 + i, *((unsigned int *)dst_addr3 + i), i);
            return CI_FAIL;
        }
    }

    return CI_PASS;
}

ci_status_t GDMA_INTERRUPT_TEST(unsigned int *src_addr, unsigned int *dst_addr1, unsigned int *dst_addr2,
                                unsigned int *dst_addr3, unsigned int len)
{
    unsigned int i;
    int ret;
    hal_gdma_running_status_t run_sta = 0;

    for (i = 0 ; i < len / (sizeof(unsigned int)); i++) {
        *((unsigned int *)src_addr + i) = i;
    }

    memset(dst_addr1, 0, len);
    memset(dst_addr2, 0, len);
    memset(dst_addr3, 0, len);

    /* GDMA channel 0 interrupt mode */
    ret = hal_gdma_init(HAL_GDMA_CHANNEL_0);
    if (ret) {
        printf("channel0 init fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_register_callback(HAL_GDMA_CHANNEL_0, GDMA0_ISR_HANDLER, NULL);
    if (ret) {
        printf("channel0 hal_gdma_register_callback fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_start_interrupt(HAL_GDMA_CHANNEL_0, (uint32_t)dst_addr1, (uint32_t)src_addr, len);
    if (ret) {
        printf("channel0 hal_gdma_start_interrupt fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    hal_gdma_get_running_status(HAL_GDMA_CHANNEL_0, &run_sta);
    while (run_sta) {
        hal_gdma_get_running_status(HAL_GDMA_CHANNEL_0, &run_sta);
    }
    ret = hal_gdma_stop(HAL_GDMA_CHANNEL_0);
    if (ret) {
        printf("channel0 hal_gdma_stop,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_deinit(HAL_GDMA_CHANNEL_0);
    if (ret) {
        printf("channel0 hal_gdma_deinit fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }

    /* GDMA channel 1 interrupt mode */
    ret = hal_gdma_init(HAL_GDMA_CHANNEL_1);
    if (ret) {
        printf("channel1 init fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_register_callback(HAL_GDMA_CHANNEL_1, GDMA1_ISR_HANDLER, NULL);
    if (ret) {
        printf("channel1 hal_gdma_register_callback fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_start_interrupt(HAL_GDMA_CHANNEL_1, (uint32_t)dst_addr2, (uint32_t)src_addr, len);
    if (ret) {
        printf("channel1 hal_gdma_start_interrupt fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    hal_gdma_get_running_status(HAL_GDMA_CHANNEL_1, &run_sta);
    while (run_sta) {
        hal_gdma_get_running_status(HAL_GDMA_CHANNEL_1, &run_sta);
    }
    ret = hal_gdma_stop(HAL_GDMA_CHANNEL_1);
    if (ret) {
        printf("channel1 hal_gdma_stop,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_deinit(HAL_GDMA_CHANNEL_1);
    if (ret) {
        printf("channel1 hal_gdma_deinit fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }

    /* GDMA channel 2 interrupt mode */
    ret = hal_gdma_init(HAL_GDMA_CHANNEL_2);
    if (ret) {
        printf("channel2 init fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_register_callback(HAL_GDMA_CHANNEL_2, GDMA2_ISR_HANDLER, NULL);
    if (ret) {
        printf("channel2 hal_gdma_register_callback fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_start_interrupt(HAL_GDMA_CHANNEL_2, (uint32_t)dst_addr3, (uint32_t)src_addr, len);
    if (ret) {
        printf("channel2 hal_gdma_start_interrupt fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    hal_gdma_get_running_status(HAL_GDMA_CHANNEL_2, &run_sta);
    while (run_sta) {
        hal_gdma_get_running_status(HAL_GDMA_CHANNEL_2, &run_sta);
    }
    ret = hal_gdma_stop(HAL_GDMA_CHANNEL_2);
    if (ret) {
        printf("channel2 hal_gdma_stop,ret=%d\r\n", ret);
        return CI_FAIL;
    }
    ret = hal_gdma_deinit(HAL_GDMA_CHANNEL_2);
    if (ret) {
        printf("channel2 hal_gdma_deinit fail,ret=%d\r\n", ret);
        return CI_FAIL;
    }

    for (i = 0 ; i < len / (sizeof(unsigned int)); i++) {
        if (*((unsigned int *)dst_addr1 + i) != i) {
            printf("channel 0 test fail\r\n");
            printf("0x%p: 0x%x != 0x%x\r\n", (unsigned int *)dst_addr1 + i, *((unsigned int *)dst_addr1 + i), i);
            return CI_FAIL;
        }
    }

    for (i = 0 ; i < len / (sizeof(unsigned int)); i++) {
        if (*((unsigned int *)dst_addr2 + i) != i) {
            printf("channel 1 test fail\r\n");
            printf("0x%p: 0x%x != 0x%x\r\n", (unsigned int *)dst_addr2 + i, *((unsigned int *)dst_addr2 + i), i);
            return CI_FAIL;
        }
    }

    for (i = 0 ; i < len / (sizeof(unsigned int)); i++) {
        if (*((unsigned int *)dst_addr3 + i) != i) {
            printf("channel 2 test fail\r\n");
            printf("0x%p: 0x%x != 0x%x\r\n", (unsigned int *)dst_addr3 + i, *((unsigned int *)dst_addr3 + i), i);
            return CI_FAIL;
        }
    }

    return CI_PASS;
}

ci_status_t ci_gdma_interrupt_mode_transfer_sample(void)
{
    unsigned int *src_addr, *dst_addr1, *dst_addr2, *dst_addr3, len = 0x1000;
    int ret;

    int loop = 0;

    src_addr = (unsigned int *)SYS_MALLOC_NC(len);
    dst_addr1 = (unsigned int *)SYS_MALLOC_NC(len);
    dst_addr2 = (unsigned int *)SYS_MALLOC_NC(len);
    dst_addr3 = (unsigned int *)SYS_MALLOC_NC(len);

    printf("src_addr:%p\r\ndst_addr1:%p\r\ndst_addr2:%p\r\ndst_addr3:%p\r\nlen_1:0x%x\r\n",
           src_addr, dst_addr1, dst_addr2, dst_addr3, len);

    for (loop = 0; loop < 1000; loop++) {
        ret = GDMA_INTERRUPT_TEST(src_addr, dst_addr1, dst_addr2, dst_addr3, len);
        if (0 != ret) {
            break;
        }
        printf("[UT_GDMA]loop:%d pass!!!\r\n", loop);
    }
    if (0 != ret) {
        printf("[UT_GDMA]Interrupt mode test fail!!!\r\n");
    } else {
        printf("[UT_GDMA]Interrupt mode test pass!!!\r\n");
    }
    SYS_FREE_NC(src_addr);
    SYS_FREE_NC(dst_addr1);
    SYS_FREE_NC(dst_addr2);
    SYS_FREE_NC(dst_addr3);

    return (0 != ret ? CI_FAIL : CI_PASS);
}

ci_status_t ci_gdma_polling_mode_transfer_sample(void)
{
    unsigned int *src_addr, *dst_addr1, *dst_addr2, *dst_addr3, len = 0x1000;
    int ret;

    int loop = 0;

    src_addr = (unsigned int *)SYS_MALLOC_NC(len);
    dst_addr1 = (unsigned int *)SYS_MALLOC_NC(len);
    dst_addr2 = (unsigned int *)SYS_MALLOC_NC(len);
    dst_addr3 = (unsigned int *)SYS_MALLOC_NC(len);

    printf("src_addr:%p\r\ndst_addr1:%p\r\ndst_addr2:%p\r\ndst_addr3:%p\r\nlen_1:0x%x\r\n",
           src_addr, dst_addr1, dst_addr2, dst_addr3, len);

    for (loop = 0; loop < 1000; loop++) {
        ret = GDMA_POLLING_TEST(src_addr, dst_addr1, dst_addr2, dst_addr3, len);
        if (0 != ret) {
            break;
        }
        printf("[UT_GDMA]loop:%d pass!!!\r\n", loop);
    }
    if (0 != ret) {
        printf("[UT_GDMA]Polling mode test fail!!!\r\n");
    } else {
        printf("[UT_GDMA]Polling mode test pass!!!\r\n");
    }
    SYS_FREE_NC(src_addr);
    SYS_FREE_NC(dst_addr1);
    SYS_FREE_NC(dst_addr2);
    SYS_FREE_NC(dst_addr3);

    return (0 != ret ? CI_FAIL : CI_PASS);
}


ci_status_t ci_gdma_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Sample code: interrupt mode data tranfer test", ci_gdma_interrupt_mode_transfer_sample},
        {"Sample code: polling mode data tranfer test", ci_gdma_polling_mode_transfer_sample},
    };
    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}

