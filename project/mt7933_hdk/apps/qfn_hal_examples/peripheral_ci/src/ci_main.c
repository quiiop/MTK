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

#include "FreeRTOS.h"
#include "task.h"
#include "ci_cli.h"
#include "ci.h"

#define MAX_CI_NAME_LEN 23

typedef ci_status_t (*TEST_FUNC_T)(unsigned int);

typedef struct {
    char                 *CiName;
    TEST_FUNC_T          pfSampleTestFunc;
    unsigned int         result;
} CI_MODULE;

#define CI_UT(name)

#define CI_ITEMS(name) \
{ \
    .CiName = #name, \
    .pfSampleTestFunc = ci_##name##_sample_main, \
    .result = 0, \
    CI_UT(name) \
},

static CI_MODULE ci_module[] = {
#ifdef CI_SDIOM_ENABLE
    CI_ITEMS(sdiom)
#endif /* #ifdef CI_SDIOM_ENABLE */

#ifdef CI_SDIOS_ENABLE
    CI_ITEMS(sdios)
#endif /* #ifdef CI_SDIOS_ENABLE */

#ifdef CI_SPI_ENABLE
    CI_ITEMS(spi)
#endif /* #ifdef CI_SPI_ENABLE */

#ifdef CI_RTC_ENABLE
    CI_ITEMS(rtc)
#endif /* #ifdef CI_RTC_ENABLE */

#ifdef CI_EINT_ENABLE
    CI_ITEMS(eint)
#endif /* #ifdef CI_EINT_ENABLE */

#ifdef CI_GCPU_ENABLE
    CI_ITEMS(gcpu)
#endif /* #ifdef CI_GCPU_ENABLE */

#ifdef CI_ECC_ENABLE
    CI_ITEMS(ecc)
#endif /* #ifdef CI_ECC_ENABLE */

#ifdef CI_GDMA_ENABLE
    CI_ITEMS(gdma)
#endif /* #ifdef CI_GDMA_ENABLE */

#ifdef CI_ADC_ENABLE
    CI_ITEMS(adc)
#endif /* #ifdef CI_ADC_ENABLE */

#ifdef CI_NVIC_ENABLE
    CI_ITEMS(nvic)
#endif /* #ifdef CI_NVIC_ENABLE */

#ifdef CI_PWM_ENABLE
    CI_ITEMS(pwm)
#endif /* #ifdef CI_PWM_ENABLE */

#ifdef CI_GPT_ENABLE
    CI_ITEMS(gpt)
#endif /* #ifdef CI_GPT_ENABLE */

#ifdef CI_WDT_ENABLE
    CI_ITEMS(wdt)
#endif /* #ifdef CI_WDT_ENABLE */

#ifdef CI_SLEEPMANAGER_ENABLE
    CI_ITEMS(sleepmanager)
#endif /* #ifdef CI_SLEEPMANAGER_ENABLE */

#ifdef CI_PMU_ENABLE
    CI_ITEMS(pmu)
#endif /* #ifdef CI_PMU_ENABLE */

#ifdef CI_NVDM_ENABLE
    CI_ITEMS(nvdm)
#endif /* #ifdef CI_NVDM_ENABLE */

#ifdef CI_CACHE_ENABLE
    CI_ITEMS(cache)
#endif /* #ifdef CI_CACHE_ENABLE */

#ifdef CI_MPU_ENABLE
    CI_ITEMS(mpu)
#endif /* #ifdef CI_MPU_ENABLE */

#ifdef CI_GPIO_ENABLE
    CI_ITEMS(gpio)
#endif /* #ifdef CI_GPIO_ENABLE */

#ifdef CI_FLASH_ENABLE
    CI_ITEMS(flash)
#endif /* #ifdef CI_FLASH_ENABLE */

#ifdef CI_TRNG_ENABLE
    CI_ITEMS(trng)
#endif /* #ifdef CI_TRNG_ENABLE */

    {
        .CiName = NULL,
        .pfSampleTestFunc = NULL,
        .result = 0,
    }
};

unsigned int ci_items_len = sizeof(ci_module) / sizeof(ci_module[0]);

ci_status_t test_execution(struct test_entry *case_list, unsigned int num_case)
{
    ci_status_t result = CI_PASS;

    for (unsigned int i = 0; i < num_case; i++) {
        printf("%s..\n", case_list[i].test_name);
        if (case_list[i].test_func() == CI_FAIL) {
            result = CI_FAIL;
            printf("%s: FAIL\n", case_list[i].test_name);
        } else {
            printf("%s: PASS\n", case_list[i].test_name);
        }
    }

    return result;
}

static void show_result(char *ci_name, ci_status_t result)
{
    switch (result) {
        case CI_PASS:
            printf("CI item:%s,result:%s\n", ci_name, "PASS");
            break;
        case CI_FAIL:
            printf("CI item:%s,result:%s\n", ci_name, "FAIL");
            break;
        case CI_ERROR_PARAM:
            printf("CI item:%s,result:%s\n", ci_name, "WRONG PARAMETER");
            break;
        default:
            break;
    }
}

static void usage(void)
{
    printf("usage: ci [IP_NAME] sample [portnum]\n");
    printf("[IP_NAME]:\n");
    for (unsigned int testListNumber = 0; testListNumber < ci_items_len - 1; testListNumber++) {
        printf("\t%s\n", ci_module[testListNumber].CiName);
    }
    printf("example:ci i2c sample 0\n");
}

uint8_t ci_cli(uint8_t len, char *param[])
{
    char *ipName = param[0];
    char *testType = param[1];
    unsigned int portnum = atoi(param[2]);
    unsigned int testListNumber = 0;
    unsigned int isFound = 0;

    if (len < 3) {
        usage();
        goto end;
    }

    for (testListNumber = 0; testListNumber < ci_items_len - 1; testListNumber++) {
        if (strncmp(ci_module[testListNumber].CiName, ipName, MAX_CI_NAME_LEN) == 0) {
            isFound = 1;
            if (strncmp(testType, "sample", 6) == 0) {
                ci_module[testListNumber].result = ci_module[testListNumber].pfSampleTestFunc(portnum);
                break;
            } else {
                isFound = 0;
            }
        }
    }

    (isFound) ? (show_result(ci_module[testListNumber].CiName, ci_module[testListNumber].result)) : usage();

end:
    return 0;
}
