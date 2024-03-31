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

#ifndef _SSUSB_HOST_MSC_H
#define _SSUSB_HOST_MSC_H

/**
* @addtogroup SSUSB
* @{
* @addtogroup SSUSBHost Host Mass Storage
* @{
* This section introduces the USB Host Mass Storage APIs export to file system.
* @section SSUSB_Usage_Chapter How to use this driver.
*
* - Initialize the USB Host hardware.
*  - Step1: Call #get_msc_device() to get usb mass storage device instance.
*  - Step2: Call #get_capacity() to get usb mass storage device capacity if needed.
*  - Step3: Call #get_block_size() to get usb mass storage device block size if needed.
*  - Step4: Call #erase_sector() to erase usb mass storage device sector.
*  - Step5: Call #readwrite_blocks_512() to read/write usb mass storage device.
*/

/** @brief  64bit Variable */
typedef unsigned long long u64;


/** @defgroup ssusb_SSUSBHost_Host_Mass_Storage Enums
  * @{
*/

/** @brief   USB data transfer direction.*/
typedef enum {
    cbw_direction_data_in = 0x80,  /**< device->host transfer */
    cbw_direction_data_out = 0,    /**< host->device transfer */
} cbw_direction;

/**
  * @}
  */

/** @brief  represents a usb device.*/
struct usbdev;

/**
 * @brief       Get usb mass storage device instance.
 *
 * @return      return struct usbdev or NULL.
 */
struct usbdev *get_msc_device(void);

/**
 * @brief       Get usb mass storage device capacity.
 *
 * @param[in]   dev is the device to be queried.
 * @return      return mass storage device capacity or a negative error.
 */
u64 get_capacity(struct usbdev *dev);

/**
 * @brief       Get usb mass storage device block size.
 *
 * @param[in]   dev is the device to be queried.
 * @return      return mass storage device block size or a negative error.
 */
u32 get_block_size(struct usbdev *dev);

/**
 * @brief       Erase usb mass storage device sector.
 *
 * @param[in]   dev is the device to be erased.
 * @param[in]   start is the first sector to be erased.
 * @param[in]   n is the number of sectors to be erased.
 * @return      zero on success or a negative error.
 */
int erase_sector(struct usbdev *dev, int start, int n);

/**
 * @brief       Read/Write usb mass storage device.
 *
 * @param[in]   dev is the device to be read/write.
 * @param[in]   start is the first block to be read/write.
 * @param[in]   n is the number of blocks to be read/write.
 * @param[in]   dir direction of access: cbw_direction_data_in == read, cbw_direction_data_out == write
 * @param[in]   buf buffer to read into or write from. Must be at least n*512 bytes
 * @return      zero on success or a negative error.
 */
int readwrite_blocks_512(struct usbdev *dev, int start, int n, cbw_direction dir, unsigned char *buf);

/**
* @}
* @}
*/

#endif
