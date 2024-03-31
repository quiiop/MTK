/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its
 * licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek
 * Software if you have agreed to and been bound by the applicable license
 * agreement with MediaTek ("License Agreement") and been granted explicit
 * permission to do so within the License Agreement ("Permitted User").
 * If you are not a Permitted User, please cease any access or use of MediaTek
 * Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY
 * DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE.
 * MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A
 * PARTICULAR STANDARD OR OPEN FORUM.
 * RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
 * LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */
/* Include Files */

#include <dramc_common.h>
#include <reg_pemi_reg.h>
#include <emi.h>
#include <hal_psram_internal.h>

#if CFG_BOOT_ARGUMENT
#define bootarg g_dram_buf->bootarg
#endif /* #if CFG_BOOT_ARGUMENT */

#if 0
static unsigned int get_dramc_addr(dram_addr_t *dram_addr,
                                   unsigned int offset);
#endif /* #if 0 */

#if SUPPORT_TYPE_PSRAM
/*  Global Variables */
int pemi_setting_index;
EMI_SETTINGS pemi_settings[] = {0};
EMI_SETTINGS default_pemi_setting;

/*  External references */
extern char *opt_dle_value;


#define PEMI_APB_BASE    (IO_PHYS + 0x000C0000)

#define PEMI_REG_CONA   (PEMI_APB_BASE + PEMI_CONA)
#define PEMI_REG_CONF   (PEMI_APB_BASE + PEMI_CONF)
#define PEMI_REG_CONH   (PEMI_APB_BASE + PEMI_CONH)

void PEMI_ESL_Setting1(void)
{
    //ESL setting
    //# Row =14bit
    *((UINT32P)(PEMI_APB_BASE + 0x00000010)) = 0x0a1a0b1a;
    *((UINT32P)(PEMI_APB_BASE + 0x00000018)) = 0x3657587a;
    *((UINT32P)(PEMI_APB_BASE + 0x00000020)) = 0xffff0848;
    *((UINT32P)(PEMI_APB_BASE + 0x00000028)) = 0x00000000;
    *((UINT32P)(PEMI_APB_BASE + 0x00000030)) = 0x2b2b2a38;
    *((UINT32P)(PEMI_APB_BASE + 0x00000038)) = 0x00000000;
    *((UINT32P)(PEMI_APB_BASE + 0x00000040)) = 0x00008803;
    *((UINT32P)(PEMI_APB_BASE + 0x00000060)) = 0x00000dff;
#if SUPPORT_TYPE_PCDDR4
    *((UINT32P)(PEMI_APB_BASE + 0x00000060)) |= (0x1 << 11); /*CONM[11] :age_slow*/
#endif /* #if SUPPORT_TYPE_PCDDR4 */
    *((UINT32P)(PEMI_APB_BASE + 0x00000078)) = 0x12000000;
    *((UINT32P)(PEMI_APB_BASE + 0x0000007c)) = 0x00002300;

    *((UINT32P)(PEMI_APB_BASE + 0x000000d0)) = 0xffffffff;
    *((UINT32P)(PEMI_APB_BASE + 0x000000d8)) = 0xff888888;

    *((UINT32P)(PEMI_APB_BASE + 0x000000e8)) = 0x00200027;
    *((UINT32P)(PEMI_APB_BASE + 0x000000f0)) = 0x38460000;
    *((UINT32P)(PEMI_APB_BASE + 0x000000f8)) = 0x00000000;
    *((UINT32P)(PEMI_APB_BASE + 0x000000fc)) = 0x400f000f;

    *((UINT32P)(PEMI_APB_BASE + 0x00000100)) = 0xffff5c5b; //cpu
    *((UINT32P)(PEMI_APB_BASE + 0x00000108)) = 0xffff5c5b; //cpu

    *((UINT32P)(PEMI_APB_BASE + 0x00000110)) = 0xffff7042; //M2
    *((UINT32P)(PEMI_APB_BASE + 0x00000128)) = 0xffff7042; //M5
    *((UINT32P)(PEMI_APB_BASE + 0x00000130)) = 0xffff7042; //M6
    *((UINT32P)(PEMI_APB_BASE + 0x00000138)) = 0xffff7043; //MM

    *((UINT32P)(PEMI_APB_BASE + 0x00000140)) = 0x20406188;
    *((UINT32P)(PEMI_APB_BASE + 0x00000144)) = 0x20406188;
    *((UINT32P)(PEMI_APB_BASE + 0x00000148)) = 0x37684848;
    *((UINT32P)(PEMI_APB_BASE + 0x0000014c)) = 0x3719595e;
    *((UINT32P)(PEMI_APB_BASE + 0x00000150)) = 0x64f3fc79;
    *((UINT32P)(PEMI_APB_BASE + 0x00000154)) = 0x64f3fc79;
    *((UINT32P)(PEMI_APB_BASE + 0x00000158)) = 0x00000000;

}

void pemi_esl_setting2(void)
{

}

void pemi_patch(void)
{
}

void pemi_init(DRAMC_CTX_T *p)
{
    if (PSRAM_MODE_UHS_PSRAM_8MB == psram_type_get_from_AO_RG()) {
        *((volatile unsigned *)(PEMI_APB_BASE + PEMI_CONA)) = 0x00001021;
    } else if (PSRAM_MODE_UHS_PSRAM_16MB == psram_type_get_from_AO_RG()) {
        *((volatile unsigned *)(PEMI_APB_BASE + PEMI_CONA)) = 0x00002021;
    } else {
        mcSHOW_DBG_MSG("wrong type uhs psram = %d \n", psram_type_get_from_AO_RG());
        return;
    }
    *((volatile unsigned *)(PEMI_APB_BASE + PEMI_CONH)) = 0x00000003;
    *((volatile unsigned *)(PEMI_APB_BASE + PEMI_CONF)) = 0x00000000;
    *((volatile unsigned *)(PEMI_APB_BASE + PEMI_CONM)) = 0xdff;

    dsb();

    mcDELAY_US(50);
    mcSHOW_DBG_MSG("\n\tPEMI_CONA:0x%x \n\tPEMI_CONH:0x%x \n\tPEMI_CONF:0x%x \n", (*((volatile unsigned *)(PEMI_APB_BASE + PEMI_CONA))),
                   (*((volatile unsigned *)(PEMI_APB_BASE + PEMI_CONH))), (*((volatile unsigned *)(PEMI_APB_BASE + PEMI_CONF))));
    mcSHOW_DBG_MSG("\tPEMI_CONM:0x%x\n", (*((volatile unsigned *)(PEMI_APB_BASE + PEMI_CONM))));
    mcSHOW_DBG_MSG("PEMI Init Done \n");
}


void pemi_init2(void)
{
    pemi_esl_setting2();
    pemi_patch();
}

#endif /* #if SUPPORT_TYPE_PSRAM */
