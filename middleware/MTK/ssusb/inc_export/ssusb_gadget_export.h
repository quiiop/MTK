/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/*
 * MediaTek Inc. (C) 2018. All rights reserved.
 *
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

#ifndef _SSUSB_GADGET_EXPORT_H
#define _SSUSB_GADGET_EXPORT_H

/**
* @addtogroup SSUSB
* @{
* @addtogroup Gadget
* @{
* This section introduces the USB Gadget APIs including terms and acronyms,
* supported features, details on how to use this driver.
* @section SSUSB_Usage_Chapter How to use this driver.
*
* - Initialize the USB Gadget hardware.
*  - Step1: Call #serial_usb_init() to initialize the USB hardware and CDC class driver.
*  - Step2: Call #serial_configured() to get the configure status of the device.
*  - Step3: Call #serial_usbtty_putcn()/serial_usbtty_getcn to write/read data if configured.
*  - Step4: Call #serial_usb_exit() to exit class driver.
*/


/*****************************************************************************
* Functions
*****************************************************************************/

/**
 * @brief     USB CDC serial (ACM) init function.
 *
 * @return    zero on success or a negative error.
 */
int serial_usb_init(void);

/**
 * @brief     USB CDC serial (ACM) exit function.
 *
 * @return    None.
 */
void serial_usb_exit(void);

/**
 * @brief     Get the configure status of the device.
 *
 * @return    1 on configured or 0 on not configured.
 */
int serial_configured(void);

/**
 * @brief       Write data to the buffer.
 *
 * @param[in]   buf is the memory to be write.
 * @param[in]   count is the length to be write.
 * @return      The length has been write or a negative error.
 */
int serial_usbtty_putcn(char *buf, int count);

/**
 * @brief       Read data from the buffer.
 *
 * @param[in]   buf is the memory to be read.
 * @param[in]   count is the length to be read.
 * @return      The actual length has been read or a negative error.
 */
int serial_usbtty_getcn(char *buf, int count);

/**
* @}
* @}
*/

#endif /* #ifndef _SSUSB_GADGET_EXPORT_H */

