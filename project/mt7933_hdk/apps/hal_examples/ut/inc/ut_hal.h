/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
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


#ifndef __UT_HAL_H__
#define __UT_HAL_H__

#include "hal.h"

#ifdef UT_HAL_ADC_MODULE_ENABLE
ut_status_t ut_hal_adc(void);
#endif /* #ifdef UT_HAL_ADC_MODULE_ENABLE */

#ifdef UT_HAL_AES_MODULE_ENABLE
ut_status_t ut_hal_aes(void);
#endif /* #ifdef UT_HAL_AES_MODULE_ENABLE */

#ifdef UT_HAL_CACHE_MODULE_ENABLE
ut_status_t ut_hal_cache(void);
#endif /* #ifdef UT_HAL_CACHE_MODULE_ENABLE */

#ifdef UT_HAL_CLOCK_MODULE_ENABLE
ut_status_t ut_hal_clock(void);
#endif /* #ifdef UT_HAL_CLOCK_MODULE_ENABLE */

#ifdef UT_HAL_DES_MODULE_ENABLE
ut_status_t ut_hal_des(void);
#endif /* #ifdef UT_HAL_DES_MODULE_ENABLE */

#ifdef UT_HAL_EFUSE_MODULE_ENABLE
ut_status_t ut_hal_efuse(void);
#endif /* #ifdef UT_HAL_EFUSE_MODULE_ENABLE */

#ifdef UT_HAL_EINT_MODULE_ENABLE
ut_status_t ut_hal_eint(void);
#endif /* #ifdef UT_HAL_EINT_MODULE_ENABLE */

#ifdef UT_HAL_FLASH_MODULE_ENABLE
ut_status_t ut_hal_flash(void);
#endif /* #ifdef UT_HAL_FLASH_MODULE_ENABLE */

#ifdef UT_HAL_GDMA_MODULE_ENABLE
ut_status_t ut_hal_gdma(void);
#endif /* #ifdef UT_HAL_GDMA_MODULE_ENABLE */

#ifdef UT_HAL_GPIO_MODULE_ENABLE
ut_status_t ut_hal_gpio(void);
#endif /* #ifdef UT_HAL_GPIO_MODULE_ENABLE */

#ifdef UT_HAL_GPT_MODULE_ENABLE
ut_status_t ut_hal_gpt(void);
#endif /* #ifdef UT_HAL_GPT_MODULE_ENABLE */

#ifdef UT_HAL_I2C_MASTER_MODULE_ENABLE
ut_status_t ut_hal_i2c_master(void);
#endif /* #ifdef UT_HAL_I2C_MASTER_MODULE_ENABLE */

#ifdef UT_HAL_NVIC_MODULE_ENABLE
ut_status_t ut_hal_nvic(void);
#endif /* #ifdef UT_HAL_NVIC_MODULE_ENABLE */

#ifdef UT_HAL_IRRX_MODULE_ENABLE
ut_status_t ut_hal_irrx(void);
#endif /* #ifdef UT_HAL_IRRX_MODULE_ENABLE */

#ifdef UT_HAL_KEYPAD_MODULE_ENABLE
ut_status_t ut_hal_keypad(void);
#endif /* #ifdef UT_HAL_KEYPAD_MODULE_ENABLE */

#ifdef UT_HAL_MD5_MODULE_ENABLE
ut_status_t ut_hal_md5(void);
#endif /* #ifdef UT_HAL_MD5_MODULE_ENABLE */

#ifdef UT_HAL_MPU_MODULE_ENABLE
ut_status_t ut_hal_mpu(void);
#endif /* #ifdef UT_HAL_MPU_MODULE_ENABLE */

#ifdef UT_HAL_ASIC_MPU_MODULE_ENABLE
ut_status_t ut_hal_asic_mpu(void);
#endif /* #ifdef UT_HAL_ASIC_MPU_MODULE_ENABLE */

#ifdef UT_HAL_PMU_MODULE_ENABLE
ut_status_t ut_hal_pmu(void);
#endif /* #ifdef UT_HAL_PMU_MODULE_ENABLE */

#ifdef UT_HAL_PSRAM_MODULE_ENABLE
ut_status_t ut_hal_psram(void);
#endif /* #ifdef UT_HAL_PSRAM_MODULE_ENABLE */

#ifdef UT_HAL_PWM_MODULE_ENABLE
ut_status_t ut_hal_pwm(void);
#endif /* #ifdef UT_HAL_PWM_MODULE_ENABLE */

#ifdef UT_HAL_RTC_MODULE_ENABLE
ut_status_t ut_hal_rtc(void);
#endif /* #ifdef UT_HAL_RTC_MODULE_ENABLE */

#ifdef UT_HAL_SPI_MASTER_MODULE_ENABLE
ut_status_t ut_hal_spi_master(void);
#endif /* #ifdef UT_HAL_SPI_MASTER_MODULE_ENABLE */

#ifdef UT_HAL_SPI_SLAVE_MODULE_ENABLE
ut_status_t ut_hal_spi_slave(void);
#endif /* #ifdef UT_HAL_SPI_SLAVE_MODULE_ENABLE */

#ifdef UT_HAL_SD_MODULE_ENABLE
ut_status_t ut_hal_sd(void);
#endif /* #ifdef UT_HAL_SD_MODULE_ENABLE */

#ifdef UT_HAL_SDIO_MODULE_ENABLE
ut_status_t ut_hal_sdio(void);
#endif /* #ifdef UT_HAL_SDIO_MODULE_ENABLE */

#ifdef UT_HAL_SDIO_SLAVE_MODULE_ENABLE
ut_status_t ut_hal_sdio_slave(void);
#endif /* #ifdef UT_HAL_SDIO_SLAVE_MODULE_ENABLE */

#ifdef UT_HAL_SHA_MODULE_ENABLE
ut_status_t ut_hal_sha(void);
#endif /* #ifdef UT_HAL_SHA_MODULE_ENABLE */

#ifdef UT_HAL_SLEEP_MANAGER_MODULE_ENABLE
ut_status_t ut_hal_sleep_manager(void);
#endif /* #ifdef UT_HAL_SLEEP_MANAGER_MODULE_ENABLE */

#ifdef UT_HAL_TRNG_MODULE_ENABLE
ut_status_t ut_hal_trng(void);
#endif /* #ifdef UT_HAL_TRNG_MODULE_ENABLE */

#ifdef UT_HAL_UART_MODULE_ENABLE
ut_status_t ut_hal_uart(void);
#endif /* #ifdef UT_HAL_UART_MODULE_ENABLE */

#ifdef UT_HAL_SSUSB_GADGET_ENABLE
ut_status_t ut_hal_ssusb_gadget(void);
#endif /* #ifdef UT_HAL_SSUSB_GADGET_ENABLE */

#ifdef UT_HAL_SSUSB_HOST_ENABLE
ut_status_t ut_hal_ssusb_host(void);
#endif /* #ifdef UT_HAL_SSUSB_HOST_ENABLE */

#ifdef UT_HAL_WDT_MODULE_ENABLE
ut_status_t ut_hal_wdt(void);
#endif /* #ifdef UT_HAL_WDT_MODULE_ENABLE */

#ifdef UT_HAL_DWT_MODULE_ENABLE
ut_status_t ut_hal_dwt(void);
#endif /* #ifdef UT_HAL_DWT_MODULE_ENABLE */

#endif /* #ifndef __UT_HAL_H__ */

