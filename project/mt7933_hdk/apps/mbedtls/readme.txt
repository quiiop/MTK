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
 * @addtogroup mt793x_hdk_mbedtls mbedtls
 * @{

@par Overview
  - Project Demonstration
    - This project demonstrates the Wi-Fi/BT connectivity and Audio Service with low power feature 
  - Application features
    - Act as a Wi-Fi station to connect to a Wi-Fi network.
    - Act as a Wi-Fi AP to accept connection to the MT7933 HDK using a
      handheld device or a laptop computer.
    - Ping out or into the device.
    - CM33 for Wi-Fi/BT Command Line Interface (CLI) via CM33_UART(baudrate: 921600) 
    is supported in this project
    - DSP for Audio Command Line Interface (CLI) via DSP_UART(baudrate: 115200) 
    is supported in this project.  


@par Directory contents
  - Source and header files         
    - \b src/aes_demo.c:                  AES encryption & decryption demonstration.
                                              This file was derived from
                                              https://tls.mbed.org/, MediaTek modified it
                                              to integrate it in this example project.
    - \b src/md5_demo.c:                  MD5 demonstration. This file was derived from
                                              https://tls.mbed.org/, MediaTek modified it
                                              to integrate it in this example project.
    - \b src/mpi_demo.c:                  MPI demonstration. This file was derived from
                                              https://tls.mbed.org/, MediaTek modified it
                                              to integrate it in this example project.
    - \b src/pk_encrypt.c:                PK encryption demonstration. This file was
                                              derived from https://tls.mbed.org/, MediaTek
                                              modified it to integrate it in this example
                                              project.
    - \b src/pk_decrypt.c:                PK decryption demonstration. This file was
                                              derived from https://tls.mbed.org/, MediaTek
                                              modified it to integrate it in this example
                                              project.
    - \b src/rsa_encrypt.c:               RSA encryption demonstration. This file was
                                              derived from https://tls.mbed.org/, MediaTek
                                              modified it to integrate it in this example
                                              project.
    - \b src/rsa_decrypt.c:               RSA decryption demonstration. This file was
                                              derived from https://tls.mbed.org/, MediaTek
                                              modified it to integrate it in this example
                                              project.
    - \b src/selftest.c:                  Self-test demonstration. This file was
                                              derived from https://tls.mbed.org/, MediaTek
                                              modified it to integrate it in this example
                                              project.
    - \b src/ssl_client.c:                SSL client demonstration. This file was
                                              derived from https://tls.mbed.org/, MediaTek
                                              modified it to integrate it in this example
                                              project.
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

@par Run the examples
  - Connect the FTDI board to a PC with a USB cable.
  - Build the example project("./build.sh mt7933_hdk mbedtls") and download the binary file to the MT793x HDK.
    See more information in MT793x_Flash_Burning_Tool_v2.77_ext.docx to Download code.
  - Reboot the HDK, the console will show "$" message to
    indicate the HDK is booting up.
  - Use '?' and enter to query the available command line options.
    Note that the command line options are still under development and subject
    to change without notice.
  - Below are 2 examples to demonstrate the Wi-Fi station, Wi-Fi access
    point modes of the 793x HDK, BT GATT mode, BT Mesh mode and Aduio example.



  - Example 1. Wi-Fi station mode (SEC mode).
    - Find your Wi-Fi access point settings: Before connecting to a Wi-Fi access point, the following information needs to be collected:
      -# The SSID of your Wi-Fi access point.
      -# The authentication mode of your Wi-Fi access point. In general, the authentication mode is WPA PSK or WPA2 PSK. To change the mode, please refer to **Table of Authentication Modes** for the list of supported authentication modes.
      -# The password of your Wi-Fi access point.
      -# The encryption mode of your Wi-Fi access point. In general, AES or TKIP is used. To change the mode, please refer to **Table of Encyption Modes**  for the list of supported encryption modes.
    - Once the information is collected, use the following commands to configure the MT7933. The example below assumes either WPA PSK or WPA2 PSK is used for authentication, TKIP or AES for encryption, 'MTK_SOFT_AP'
    for the SSID, and the password of the WPA or WPA2 is '12345678' 
        \code
        wifi init
        wifi config set ssid 0 MTK_SOFT_AP
        wifi config set sec 0 9 8
        wifi config set psk 0 12345678
        wifi config set reload
        \endcode

    - After calling reload CLI by CM33_UART, if everything1` is correct, similar messages will be shown in the console to notify your HDK has received an IP address.
        \code
        DHCP got IP:192.168.1.100
        \endcode

    - ping: It assumes IP 19.168.1.100 is received. If the IP address is fetched and the network is operating, the MT7933 can ping other devices on the network with the following command in the console.
        \code
        ping 192.168.1.100
        The ping stops after sending three packets to 192.168.1.100.
        The ping usage is: ping <ip address> <times>
        \endcode 
    - Table of Authentication Modes 
      | Mode                                    |    Support or Not     |
      |-----------------------------------------|-----------------------|
      |  WIFI_AUTH_MODE_OPEN = 0                |       Open mode       |
      |  WIFI_AUTH_MODE_SHARED = 1              |     Not supported     |
      |  WIFI_AUTH_MODE_AUTO_WEP = 2            |     Not supported     |
      |  WIFI_AUTH_MODE_WPA = 3                 |     Not supported     |
      |  WIFI_AUTH_MODE_WPA_PSK = 4             |       WPA_PSK         |
      |  WIFI_AUTH_MODE_WPA_None = 5            |     Not supported     |
      |  WIFI_AUTH_MODE_WPA2 = 6                |     Not supported     |
      |  WIFI_AUTH_MODE_WPA2_PSK = 7            |       WPA2_PSK        |
      |  WIFI_AUTH_MODE_WPA_WPA2 = 8            |     Not supported     |
      |  WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK = 9    |       Mixture mode    |
      |  WIFI_AUTH_MODE_WPA3 = 10               |     Not supported     |
      |  WIFI_AUTH_MODE_WPA3_PSK = 11           |       WPA3_PSK        |
    - Table of Encyption Modes 
      | Mode                                              |    Support or Not     |
      |---------------------------------------------------|-----------------------|
      |  WIFI_ENCRYPT_TYPE_WEP_ENABLED = 0                |   WEP encryption type |
      |  WIFI_ENCRYPT_TYPE_ENCRYPT1_ENABLED = 0           |   WEP encryption type |
      |  WIFI_ENCRYPT_TYPE_WEP_DISABLED = 1               |     No encryption     |
      |  WIFI_ENCRYPT_TYPE_ENCRYPT_DISABLED = 1           |     No encryption     |
      |  WIFI_ENCRYPT_TYPE_WEP_KEY_ABSENT = 2             |     Not supported     |
      |  WIFI_ENCRYPT_TYPE_ENCRYPT_KEY_ABSENT = 2         |     Not supported     |
      |  WIFI_ENCRYPT_TYPE_WEP_NOT_SUPPORTED = 3          |     Not supported     |
      |  WIFI_ENCRYPT_TYPE_ENCRYPT_NOT_SUPPORTED = 3      |     Not supported     |
      |  WIFI_ENCRYPT_TYPE_TKIP_ENABLED = 4               |    TKIP encryption    |
      |  WIFI_ENCRYPT_TYPE_ENCRYPT2_ENABLED = 4           |    TKIP encryption    |
      |  WIFI_ENCRYPT_TYPE_AES_ENABLED = 6                |     AES encryption    |
      |  WIFI_ENCRYPT_TYPE_ENCRYPT3_ENABLED = 6           |     AES encryption    |  
      |  WIFI_ENCRYPT_TYPE_AES_KEY_ABSENT = 7             |     Not supported     |
      |  WIFI_ENCRYPT_TYPE_TKIP_AES_MIX = 8               |    TKIP or AES mix    |
      |  WIFI_ENCRYPT_TYPE_ENCRYPT4_ENABLED = 8           |    TKIP or AES mix    |
      |  WIFI_ENCRYPT_TYPE_TKIP_AES_KEY_ABSENT = 9        |     Not supported     |
      |  WIFI_ENCRYPT_TYPE_GROUP_WEP40_ENABLED = 10       |     Not supported     |
      |  WIFI_ENCRYPT_TYPE_GROUP_WEP104_ENABLED = 11      |     Not supported     |

  - Example 2. Run the application
    - Once the network is connected, use the following commands to do mbedtls test.
        \code
        mbedtls test
        \endcode 
*/
/**
 * @}
 * @}
 * @}
 */