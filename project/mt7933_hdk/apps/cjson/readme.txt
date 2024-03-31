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
 * @addtogroup mt793x_hdk_cjson cjson
 * @{

@par Overview
  - Application description
    - This application shows the usage of APIs of cJSON module. 
  - Features of the application
    - This application demonstrates how to parse a cJSON string and how to
      create a cJSON string.
    - This application explains how to:
    - 1. Parse JSON text blocks.
    - 2. Rebuild the JSON objects and print them out.
  - Results
    - The output is in a log.

@par Hardware and software environment
  \includedoc bga.dox

@par Directory contents
  - Source and header files
    - \b src/cjson_cli.h:                 CLI command definition.       
    - \b src/cjson_cli.c:                 CLI command for cjson client test.     
    - \b src/cJSON_test.c:                cJSON test file. This file was derived from
                                          https://github.com/DaveGamble/cJSON. Mediatek
                                          modified it to integrate it in this example
                                              projct.    
    - \b src/bt_setting.c                 Set Bluetooth debug setting.
    - \b src/cli_cmds.c                   CLI commands of this project.
    - \b src/cli_def.c                    CLI initialize sequence code.
    - \b src/fota_flash_config.c          Partition layout used by FOTA. 
    - \b src/hci_log.c                    Bluetooth HCI log file. The user must.
                                              initiate the UART port for HCI logging.
    - \b src/low_pwr.c                    GPIO setting file for low power.
    - \b src/main.c                       Entry point of the application program.
    - \b src/mesh                         Bluetooth mesh application sample code.
    - \b src/network_default_config.c     The default configuration of network data which will be used for the initialization of STA and AP mode.
    - \b src/platform_default_config.c    NVDM Default Configure for Platform.
    - \b src/region_init.c                Memory Region Definition for this project, it is used like coredump allowed region.
    - \b src/sys_init.c                   Aggregated initialization routines.
    - \b src/system_mt7933.c              MT793x basic configuration file.
    - \b src/nvdm_config.c                Load NVDM setting for WIFI.
    - \b src/wifi_cfg_default_config.c    NVDM Setting for WIFI Driver and Firmware
    - \b inc/cli_cmds.h                   Declares the reference point of CLI
                                              commands of cli_cmds.c. To be used
                                              by cli_def.c
    - \b inc/FreeRTOSConfig.h             MT793x FreeRTOS configuration file.
    - \b inc/ept_gpio_drv.h               The GPIO configuration file generated
                                              by Easy Pinmux Tool(EPT). Please do
                                              not edit the file.
    - \b inc/kernel_service_config.h      MTK proprietary service configuration file.
    - \b inc/lwipopts.h                   LwIP configuration.
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

@par Run the cjson examples
  - Connect the FTDI board to a PC with a USB cable.
  - Build the example project and download the binary file to the MT793x HDK.
    See more information in MT793x_Flash_Burning_Tool_v2.77_ext.docx to Download code.
  - Reboot the HDK, the console will show "$" message to
    indicate the HDK is booting up.
  - Use '?' and enter to query the available command line options.
    Note that the command line options are still under development and subject
    to change without notice.
  - Below are cJson test flow.
  - Use cJson CLI to run example test codes.


  - Step 1. cJson Test.
    - Enter CLI command to run cJson application. 
        \code
        cjson test
        \endcode
  - Run the application. The result is displayed in the log. "example project
    test success" printed in the log indicates a success.
*/
/**
 * @}
 * @}
 * @}
 */
