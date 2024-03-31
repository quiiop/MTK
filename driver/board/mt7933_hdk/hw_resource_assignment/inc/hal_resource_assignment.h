/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#ifndef __HAL_RESOURCE_ASSIGNMENT_H__
#define __HAL_RESOURCE_ASSIGNMENT_H__

#include "stdio.h"
#include "stdint.h"
#include "stdarg.h"
#include "hal.h"

#ifdef HAL_HW_SEMAPHORE_MODULE_ENABLED
/**
 * @addtogroup HAL
 * @{
 * @addtogroup HW_SEMAPHORE
 * @{
 * @defgroup hal_hw_semaphore_define Define
 * @{
 */

/**
 * @brief  This macro defines the SEMAPHORE for SYSLOG.
 */
#define HW_SEMAPHORE_SYSLOG        0
/**
 * @brief  This macro defines the SEMAPHORE for EXCEPTION.
 */
#define HW_SEMAPHORE_EXCEPTION     1
/**
 * @brief  This macro defines the SEMAPHORE for HB.
 */
#define HW_SEMAPHORE_HB            2
/**
 * @brief  This macro defines the SEMAPHORE for AUDIO_SINK.
 */
#define HW_SEMAPHORE_AUDIO_SINK    3
/**
 * @brief  This macro defines the SEMAPHORE for CIRCULAR_BUFFER.
 */
#define HW_SEMAPHORE_DSP_CIRCULAR_BUFFER 4
/**
 * @brief  This macro defines the SEMAPHORE for DVFS.
 */
#define HW_SEMAPHORE_DVFS 5
/**
 * @brief  This macro defines the DSP0_PLAYBACK.
 */
#define HW_SEMAPHORE_AUDIO_CM4_DSP0_PLAYBACK 6
/**
 * @brief  This macro defines the SEMAPHORE for SLEEP.
 */
#define HW_SEMAPHORE_SLEEP 7
/**
 * @brief  This macro defines the SEMAPHORE for syslog porting layer.
 */
#define HW_SEMAPHORE_SYSLOG_WRAP_LAYER 8
/**
 * @brief  This macro defines the SEMAPHORE for APLL.
 */
#define HW_SEMAPHORE_APLL 9

/**
  * @}
  */

/**
 * @}
 * @}
 */

#endif /* #ifdef HAL_HW_SEMAPHORE_MODULE_ENABLED */

/* CCNI user config */


#ifdef HAL_CCNI_MODULE_ENABLED
#include "hal_ccni_config.h"
#endif /* #ifdef HAL_CCNI_MODULE_ENABLED */


#ifdef HAL_HW_RESOURCE_ASSIGNMENT_CHECK
/*HAL HW resource assignment*/
typedef enum {
    HAL_MODULE_EINT_ID = 0,
    HAL_MODULE_GDMA_ID,
    HAL_MODULE_GPIO_ID,
    HAL_MODULE_GPT_ID,
    HAL_MODULE_I2S_ID,
    HAL_MODULE_I2C_ID,
    HAL_MODULE_SPI_MASTER_ID,
    HAL_MODULE_SPI_SLAVE_ID,
    HAL_MODULE_UART_ID,
    HAL_MODULE_MAX,
} hal_module_id_t;
#endif /* #ifdef HAL_HW_RESOURCE_ASSIGNMENT_CHECK */


/*
system private memory 12KB
--------------------------------
|   logging -- 10K
--------------------------------
|   exception--1K
--------------------------------
|   CCNI -- 768 B
--------------------------------
|   core status --16 B
--------------------------------
|   DVFS status --64 B
--------------------------------
|   RTC freq status --4 B
--------------------------------
|   reserved -- 172B
--------------------------------
*/

#define HW_SYSRAM_PRIVATE_MEMORY_START  0x0423D000
#define HW_SYSRAM_PRIVATE_MEMORY_LEN    0x3000

#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_START HW_SYSRAM_PRIVATE_MEMORY_START
#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_LEN   0x2400

#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_USB_VAR_START (HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_START + HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_LEN)
#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_USB_VAR_LEN   0x200

#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_UART_VAR_START (HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_USB_VAR_START + HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_USB_VAR_LEN)
#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_UART_VAR_LEN   0x50

#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_DUMP_VAR_START (HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_UART_VAR_START + HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_UART_VAR_LEN)
#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_DUMP_VAR_LEN   0x50

#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_ONLINE_START (HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_DUMP_VAR_START + HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_DUMP_VAR_LEN)
#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_ONLINE_VAR_LEN   0x18

#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_VERSION_VAR_START (HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_ONLINE_START + HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_ONLINE_VAR_LEN)
#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_VERSION_VAR_LEN   0xC0

#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_BUILD_TIME_VAR_START (HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_VERSION_VAR_START + HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_VERSION_VAR_LEN)
#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_BUILD_TIME_VAR_LEN   0x80

#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_RESERVED_VAR_START (HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_BUILD_TIME_VAR_START + HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_BUILD_TIME_VAR_LEN)
#define HW_SYSRAM_PRIVATE_MEMORY_SYSLOG_RESERVED_VAR_LEN   0x20

#define HW_SYSRAM_PRIVATE_MEMORY_EXCEPTION_START 0x0423F800
#define HW_SYSRAM_PRIVATE_MEMORY_EXCEPTION_LEN   0x400

#define HW_SYSRAM_PRIVATE_MEMORY_CCNI_START 0x0423FC00
#define HW_SYSRAM_PRIVATE_MEMORY_CCNI_LEN   0x300

#define HW_SYSRAM_PRIVATE_MEMORY_CORE_STATUS_START 0x0423FF00
#define HW_SYSRAM_PRIVATE_MEMORY_STATUS_LEN   0x10

#define HW_SYSRAM_PRIVATE_MEMORY_DVFS_STATUS_START 0x0423FF10
#define HW_SYSRAM_PRIVATE_MEMORY_DVFS_LEN   0x40

#define HW_SYSRAM_PRIVATE_MEMORY_RTC_FREQ_START 0x0423FF50
#define HW_SYSRAM_PRIVATE_MEMORY_RTC_FREQ_LEN   0x4

#define HW_SYSRAM_PRIVATE_MEMORY_RESERVED_START 0x0423FF54
#define HW_SYSRAM_PRIVATE_MEMORY_RESERVED_LEN   0xac



/* Private memory init */
void private_memory_init(void);

/* Memory View Transform */
uint32_t hal_memview_cm4_to_dsp0(uint32_t cm4_address);
uint32_t hal_memview_cm4_to_dsp1(uint32_t cm4_address);
uint32_t hal_memview_cm4_to_infrasys(uint32_t cm4_address);
uint32_t hal_memview_dsp0_to_cm4(uint32_t dsp0_address);
uint32_t hal_memview_dsp0_to_dsp1(uint32_t dsp0_address);
uint32_t hal_memview_dsp0_to_infrasys(uint32_t dsp0_address);
uint32_t hal_memview_dsp1_to_cm4(uint32_t dsp1_address);
uint32_t hal_memview_dsp1_to_dsp0(uint32_t dsp1_address);
uint32_t hal_memview_dsp1_to_infrasys(uint32_t cm4_address);
uint32_t hal_memview_infrasys_to_cm4(uint32_t infrasys_address);
uint32_t hal_memview_infrasys_to_dsp0(uint32_t infrasys_address);
uint32_t hal_memview_infrasys_to_dsp1(uint32_t infrasys_address);
/* Memory View Transform end*/


#endif /* #ifndef __HAL_RESOURCE_ASSIGNMENT_H__ */
