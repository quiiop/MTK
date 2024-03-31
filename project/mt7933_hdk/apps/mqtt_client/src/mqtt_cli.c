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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "memory_map.h"
#include "mqtt_cli.h"
#include "hal_log.h"
#include "memory_attribute.h"
#include "wifi_netif.h"
#include "wifi_api.h"
#include "mqtt.h"
#include "MQTTClient.h"
#include "task_def.h"

/*******************************************************************************
 * MQTT TASK
 ******************************************************************************/
/**
  * Create the log control block for lwip socket example.
  * User needs to define their own log control blocks as project needs.
  * Please refer to the log dev guide under /doc folder for more details.
  */
log_create_module(mqtt_client_main, PRINT_LEVEL_INFO);
/**
  * @brief     Create a task for MQTT client example
  * @param[in] void *args:Not used
  * @retval    None
  */
static void user_entry(void *args)
{
    while(1)
    {
        if (lwip_net_ready())
        {
            LOG_I(mqtt_client_main,"!!lwip_net_ready!!\r\n");
            break;
        }
    }
#if 0
    /* test MQTT */
    if (0 == mqtt_client_example()) 
    {
        LOG_I(mqtt_client_main,"mqtt_client_example !!!PASS!!!\r\n");
    }
#endif
    /* test MQTT over SSL */
    if (0 == mqtt_client_example_ssl())
    {
        LOG_I(mqtt_client_main,"mqtt_client_example_ssl !!PASS!!\r\n");
    }

    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

/* int mqtt_client_task(void) */
static unsigned char mqtt_client_task(uint8_t len, char *param[])
{
    /* Create a user task for demo when and how to use wifi config API  to change WiFI settings,
        * Most WiFi APIs must be called in task scheduler, the system will work wrong if called in main(),
        * For which API must be called in task, please refer to wifi_api.h or WiFi API reference.
        * xTaskCreate(user_wifi_app_entry,
        *       UNIFY_USR_DEMO_TASK_NAME,
        *       UNIFY_USR_DEMO_TASK_STACKSIZE / 4,
        *       NULL, UNIFY_USR_DEMO_TASK_PRIO, NULL);
        */    
    if (pdPASS != xTaskCreate(user_entry,
                              APP_TASK_NAME,
                              APP_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                              NULL,
                              APP_TASK_PRIO,
                              NULL)) {
        LOG_E(common, "create user task fail");
        return -1;
    } 
    return 0;
}

cmd_t mqtt_cli_cmds[] = {
    {"client","mqtt client connect",mqtt_client_task, NULL },
    { NULL, NULL, NULL, NULL }
};
