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

#if defined(UT_HAL_ENABLE) && defined (UT_HAL_CACHE_MODULE_ENABLE)

#include "hal_cache.h"
#include "memory_map.h"
#include "hal_gpt_internal.h"

#define UT_CACHE_CASE_01
#define UT_CACHE_CASE_02

#define UT_CACHE_SIZE               0x1000
#define UT_CACHE_REGION             HAL_CACHE_REGION_15
#define UT_CACHE_REGION_CNT_IDX     1                   //0~7: 0 , 8~15: 1
#define MAX_CACHE_TEST_NUM 15


//__attribute__ ((__section__(".rom_rtos"),__aligned__(4096)))      char g_ut_cache_rom[UT_CACHE_SIZE];
__attribute__((__section__(".sysram_zidata"), __aligned__(4096))) char g_ut_cache_sysram[UT_CACHE_SIZE];
//__attribute__ ((__section__(".ram_zidata"),__aligned__(4096)))    char g_ut_cache_ram[UT_CACHE_SIZE];

/* Max region number is 16 */
hal_cache_region_config_t region_cfg_tbl[] = {
    /* cacheable address, cacheable size(both MUST be 4k bytes aligned) */
    //{(uint32_t)g_ut_cache_rom,    UT_CACHE_SIZE},

    /* virtual sysram */
    {(uint32_t)g_ut_cache_sysram, UT_CACHE_SIZE},

    /* virtual memory */
    //{g_ut_cache_ram, UT_CACHE_SIZE},
};

hal_cache_region_t  region_number = (hal_cache_region_t)(sizeof(region_cfg_tbl) / sizeof(region_cfg_tbl[0]));

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void cache_mdump(char *title, char *pbuf, int size)
{
    if (!pbuf || !size)
        return;

    if (title)
        printf("\t%s\r\n", title);
    for (int i = 0; i < size; i++) {
        if (!(i % 16)) {
            i ? printf("\r\n\t") : printf("\t");
        }
        printf("%02x ", *(pbuf + i));
    }
    printf("\r\n");
}

static void cache_ut_begin(uint32_t *cacheRegionEnabled)
{
    hal_cache_region_t region;

    /* Keep original enabled regions information */
    *cacheRegionEnabled = hal_cache_get_region_en();

    /* Disable Cache Before Cache UT */
    hal_cache_disable();

    /* Disable Cache Regions Before UT */
    for (region = HAL_CACHE_REGION_0; region < HAL_CACHE_REGION_MAX; region++) {
        hal_cache_region_disable(region);
    }
}

static void cache_ut_end(uint32_t cacheRegionEnabled)
{
    hal_cache_region_t region;

    /* Recover Enabled Regions */
    for (region = HAL_CACHE_REGION_0; region < HAL_CACHE_REGION_MAX; region++) {
        if (cacheRegionEnabled & (1 << region))
            hal_cache_region_enable(region);
    }

    /* Enable Cache */
    hal_cache_enable();
}

#ifdef UT_CACHE_CASE_01
static ut_status_t cache_test_case_01(void)
{
    char *pc;

    uint32_t hitCnt_h, hitCnt_l, accCnt_h, accCnt_l;
    uint32_t idx, cntVal;

    hal_cache_region_t region;


    /* Enable Cache Region For UT Case */
    for (region = HAL_CACHE_REGION_0; region < region_number; region++) {
        /* Configure Cache Region for Cache UT */
        hal_cache_region_config(UT_CACHE_REGION, &region_cfg_tbl[region]);
        hal_cache_region_enable(UT_CACHE_REGION);
    }

    /* Enable Cache */
    hal_cache_enable();

    for (region = HAL_CACHE_REGION_0; region < region_number; region++) {
        /* Reset Variable */
        hitCnt_h = hitCnt_l = accCnt_h = accCnt_l = 0;

        /* Start Access Test */
        pc = (char *)region_cfg_tbl[region].cache_region_address;

        for (idx = 0; idx < MAX_CACHE_TEST_NUM; idx++) {
            cntVal = *((volatile uint32_t *)(pc));
        }

        hal_cache_get_hit_count(UT_CACHE_REGION_CNT_IDX, &hitCnt_h, &hitCnt_l, &accCnt_h, &accCnt_l);
        //printf("[Cache]: Read - hitCnt(h:%u, l:%u), accCnt(h:%u, l:%u)\r\n", hitCnt_h, hitCnt_l, accCnt_h, accCnt_l);
        printf("\r\n<<Test Case 01>>\r\n");
        if (hitCnt_l == (MAX_CACHE_TEST_NUM - 1))
            printf("\tAccess(%lu), Hit Count(%lu)\r\n\tTest Result: Pass!\r\n", accCnt_l, hitCnt_l);
        else
            printf("\tAccess(%lu), Hit Count(%lu)\r\n\tTestResult: Fail!\r\n", accCnt_l, hitCnt_l);

        if (cntVal); //Remove Warning
    }

    /* Disable Cache */
    hal_cache_disable();

    /* Disable Cache Region For UT Case */
    for (region = HAL_CACHE_REGION_0; region < region_number; region++) {
        /* Disable Cache UT Region */
        hal_cache_region_disable(region);
    }

    return UT_STATUS_OK;
}
#endif /* #ifdef UT_CACHE_CASE_01 */

#ifdef UT_CACHE_CASE_02
static ut_status_t cache_test_case_02(void)
{
    char               *pbuf      = NULL;
    char               *pbufphy   = NULL;
    uint32_t            bufsize   = 0;
    uint32_t            usTmStart = 0;
    uint32_t            usTmEnd   = 0;
    int                 idx;
    hal_cache_region_t  region;
    hal_cache_status_t  cstatus;


    /* Enable Cache Region For UT Case */
    for (region = HAL_CACHE_REGION_0; region < region_number; region++) {
        /* Configure Cache Region for Cache UT */
        hal_cache_region_config(region, &region_cfg_tbl[region]);
        hal_cache_region_enable(region);
    }

    /* Enable Cache */
    hal_cache_enable();

    printf("\r\n<<Test Case 02>>\r\n");
    for (region = HAL_CACHE_REGION_0; region < region_number; region++) {
        pbuf    = (char *)region_cfg_tbl[region].cache_region_address;
        pbufphy = (char *)HAL_CACHE_VIRTUAL_TO_PHYSICAL((uint32_t)pbuf);
        bufsize = region_cfg_tbl[region].cache_region_size;

        printf("\tTesting Addr: Virtual(%p), Phy(%p)\r\n", pbuf, pbufphy);

        // Update Virtual Addr Content
        memset(pbuf, 0x12, bufsize);

        // Read Content to Cache Line
        //cache_mdump("Virtual",pbuf, bufsize);
        //memcpy(pbuf, pbuf, bufsize);
        for (idx = 0; idx < bufsize; idx++) {
            if (pbuf[idx] != 0x12)
                printf("[Warning]: Content Error\r\n");
        }

        // Update Phy Addr Content
        memset(pbufphy, 0x5A, bufsize);

        // Dump Check before invalid cache line
        //printf("\r\n\tVirtual Addr Content - \r\n");
        //cache_mdump("(First 16 Bytes)",pbuf, 16);
        //cache_mdump("(Last 16 Bytes)",pbuf+(bufsize-16), 16);
        //printf("\r\n\tPhy Addr Content - \r\n");
        //cache_mdump("(First 16 Bytes)",pbufphy, 16);
        //cache_mdump("(Last 16 Bytes)",pbufphy+(bufsize-16), 16);
        //
        if (pbuf[0] != pbufphy[0])
            printf("\tVirtual[0] != Physical[0]\r\n\tTest Result = Pass\r\n");
        else
            printf("\tVirtual[0] == Physical[0]\r\n\tTest Result = Fail\r\n");

        usTmStart = gpt_current_count(gp_gpt[HAL_GPT_US_PORT]);
        cstatus = hal_cache_invalidate_multiple_cache_lines((uint32_t)pbuf, bufsize);
        //cstatus = hal_cache_invalidate_all_cache_lines();
        usTmEnd = gpt_current_count(gp_gpt[HAL_GPT_US_PORT]);
        if (cstatus != HAL_CACHE_STATUS_OK)
            printf("\r\n\t[Warning]: Cache Invalidate Multiple Lines Fail\r\n");
        else
            printf("\r\n\tExecute Time for Invalid Cache Line: %lu us\r\n", usTmEnd - usTmStart);

        // Dump Check after invalid cache line
        //cache_mdump("Virtual",pbuf, bufsize);
        //printf("\r\n\tVirtual Addr Content - After Invalid Cache Line ...\r\n");
        //cache_mdump("(First 16 Bytes)",pbuf, 16);
        //cache_mdump("(Last 16 Bytes)",pbuf+(bufsize-16), 16);
        //
        if (pbuf[0] != pbufphy[0])
            printf("\tVirtual[0] != Physical[0]\r\n\tTest Result = Fail\r\n");
        else
            printf("\tVirtual[0] == Physical[0]\r\n\tTest Result = Pass\r\n");

    }

    /* Disable Cache */
    hal_cache_disable();

    /* Enable Cache Region For UT Case */
    for (region = HAL_CACHE_REGION_0; region < region_number; region++) {
        /* Disable Cache UT Region */
        hal_cache_region_disable(region);
    }

    return UT_STATUS_OK;
}
#endif /* #ifdef UT_CACHE_CASE_02 */

ut_status_t ut_hal_cache(void)
{
    uint32_t    orgRegEn = 0;
    ut_status_t ret = UT_STATUS_OK;

    /* Should be inited at sys_init.c */
    //hal_cache_init();
    //hal_cache_set_size(HAL_CACHE_SIZE_32KB);

    // Disable & Backup Original Cache Setting
    cache_ut_begin(&orgRegEn);

    printf("\r\n\r\n<<Cache UT Case>>\r\n");

#ifdef UT_CACHE_CASE_01
    ret = cache_test_case_01();
#endif /* #ifdef UT_CACHE_CASE_01 */

#ifdef UT_CACHE_CASE_02
    ret = cache_test_case_02();
#endif /* #ifdef UT_CACHE_CASE_02 */

    // Restore & Enable Original Cache Setting
    cache_ut_end(orgRegEn);

    cache_mdump(NULL, NULL, 0); //Remove Warning

    return ret;
}

#endif /* #if defined(UT_HAL_ENABLE) && defined (UT_HAL_CACHE_MODULE_ENABLE) */
