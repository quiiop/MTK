 /* Copyright Statement:
  *
  * (C) 2005-2022  MediaTek Technology Corp. All rights reserved.
  *
  * This software/firmware and related documentation ("MediaTek Software") are
  * protected under relevant copyright laws. The information contained herein
  * is confidential and proprietary to MediaTek Technology Corp. ("MediaTek") and/or its licensors.
  * Without the prior written permission of MediaTek and/or its licensors,
  * any reproduction, modification, use or disclosure of MediaTek Software,
  * and information contained herein, in whole or in part, shall be strictly prohibited.
  * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
  * if you have agreed to and been bound by the applicable license agreement with
  * MediaTek ("License Agreement") and been granted explicit permission to do so within
  * the License Agreement ("Permitted User").  If you are not a Permitted User,
  * please cease any access or use of MediaTek Software immediately.
  * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
  * THAT MediaTek SOFTWARE RECEIVED FROM MediaTek AND/OR ITS REPRESENTATIVES
  * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MediaTek EXPRESSLY DISCLAIMS ANY AND ALL
  * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
  * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
  * NEITHER DOES MediaTek PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
  * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
  * SUPPLIED WITH MediaTek SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
  * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
  * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
  * CONTAINED IN MediaTek SOFTWARE. MediaTek SHALL ALSO NOT BE RESPONSIBLE FOR ANY MediaTek
  * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
  * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MediaTek'S ENTIRE AND
  * CUMULATIVE LIABILITY WITH RESPECT TO MediaTek SOFTWARE RELEASED HEREUNDER WILL BE,
  * AT MediaTek'S OPTION, TO REVISE OR REPLACE MediaTek SOFTWARE AT ISSUE,
  * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
  * MediaTek FOR SUCH MediaTek SOFTWARE AT ISSUE.
  *
*/

/**
 * @addtogroup mt793x_hdk mt793x_hdk
 * @{
 * @addtogroup mt793x_hdk_apps apps
 * @{
 * @addtogroup mt793x_hdk_audio_ref_design audio_ref_design
 * @{

@par Overview
  - Project Demonstration
    - This project demonstrates how to make MT7933 be a MP3 player with USB storage
  - Application features
    - CM33 for USB operation Command Line Interface (CLI) via CM33_UART(baudrate: 921600) 
    is supported in this project 
    - CM33 for MP3 player Command Line Interface (CLI) via CM33_UART(baudrate: 921600) 
    is supported in this project 

@par Hardware and software environment
  \includedoc bga.dox
  - Imgae of Device with speaker
      \htmlonly <style> img[src="audio_ref_design.png"]{float: left;width:40%;} </style> \endhtmlonly 
      @image html audio_ref_design.png 


@par Directory contents
  - Source and header files         
    - \b src/cli_cmds.c                   CLI commands of this project.
    - \b src/cli_def.c                    CLI initialize sequence code.
    - \b src/main.c                       Entry point of the application program.
    - \b mp3_codec_demo.c          MP3 playback demo
    - \b src/platform_default_config.c    NVDM Default Configure for Platform.
    - \b src/region_init.c                Memory Region Definition for this project, it is used like coredump allowed region.
    - \b src/sys_init.c                   Aggregated initialization routines.
    - \b src/system_mt7933.c              MT793x basic configuration file.
    - \b inc/cli_cmds.h                   Declares the reference point of CLI
                                              commands of cli_cmds.c. To be used
                                              by cli_def.c
    - \b inc/FreeRTOSConfig.h             MT793x FreeRTOS configuration file.
    - \b inc/ept_gpio_drv.h               The GPIO configuration file generated
                                              by Easy Pinmux Tool(EPT). Please do
                                              not edit the file.
    - \b inc/kernel_service_config.h      MTK proprietary service configuration file.
    - \b inc/sys_init.h                   Prototype declaration for \b src/sys_init.c.
    - \b inc/task_def.h                   The configuration of running tasks of
                                              the project.
    - \b inc/project_config.h.            Defines the maximum number of Bluetooth connections and timer.
                                          Defines the buffer size of the TX/RX, timer and connection.                                

  - Project configuration files using GCC.
    - \b GCC/Makefile              GNU Makefile for this project.
    - \b GCC/feature.mk            Generic feature options configuration
                                        file.
    - \b GCC/hal_feature.mk        Generic HAL Driver feature options
                                        configuration file.        
    - \b GCC/ld/memory.ld          Linker script which define memory layout used by mt7933_flash.ld.
    - \b GCC/ld/mt7933_flash.ld    Defines code placement.       
    - \b GCC/startup_mt7933.s      MT793x startup file.

@par Run the examples
  - Connect the FTDI board to a PC with a USB cable.
  - Build the example project and download the binary file to the MT793x HDK.
    See more information in MT793x_Flash_Burning_Tool_v2.77_ext.docx to Download code.
  - Reboot the HDK, the console will show "$" message to
    indicate the HDK is booting up.
  - Use '?' and enter to query the available command line options.
    Note that the command line options are still under development and subject
    to change without notice.
  - Below example demonstrate the how to mount USB and play a mp3 file.

  - Example 1. MP3 playback demo
    - Prepare an U-disk, put in the mp3 file which you want to play.
    - Prepare 2 Micro USB cables, 1 OTG cable and the U-disk which contains mp3 files, connect to RFB.
    - After power on, please enable usb first.
      \code
      ssusb enable
      \endcode
    - To make sure file system on USB works well, please type "ff mount USB" and "ff ls USB:/" in order. If file system works well, it will list all files stored in the U-disk.
      \code
      ff mount USB
      ff ls USB:/
      \endcode
    - Now we can play mp3 files, please use "mp3 play USB:/[filename]" to play audio. For example, if I want to play Stairway_To_Heaven.mp3, I would type "mp3 play USB:/Stairway_To_Heaven.mp3".
      \code
      mp3 play USB:/Stairway_To_Heaven.mp3
      \endcode
    audio ( "mp3 pause" ) and resume the audio ( "mp3 resume" ).
      \code
      mp3 pause
      mp3 resume
      \endcode

*/
/**
 * @}
 * @}
 * @}
 */