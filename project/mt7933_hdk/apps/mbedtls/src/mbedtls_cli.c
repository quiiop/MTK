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
#include "mbedtls_cli.h"
#include "hal_log.h"
#include "memory_attribute.h"
#include "wifi_netif.h"
#include "wifi_api.h"
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include "mbedtls_example.h"
#include "task_def.h"

/* Create the log control block as user wishes. Here we use 'template' as module name.
 * User needs to define their own log control blocks as project needs.
 * Please refer to the log dev guide under /doc folder for more details.
 */
log_create_module(mbedtls_proj, PRINT_LEVEL_INFO);

static char g_plaintext[512] = "===Hello! This is plaintext!===";

static char pkey_test_pub_rsa[] =
    "-----BEGIN PUBLIC KEY-----\r\n"
    "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCvzifpyCVxJXDC6nNg84+Uu4ed\r\n"
    "m2izOV2WD30lyuz0/VY5AS6ZEiqtUSxImkrJkqdda46VRyhSll5Ep+tHsdRc6ADl\r\n"
    "NQtOfz4EZAHrze4jkXnJ6duzb0S5ZSyDCTpVOZCZsYMYE0o2O4JglNss0zeT6DaR\r\n"
    "c13AKjrGPrywT6yfSwIDAQAB\r\n"
    "-----END PUBLIC KEY-----\r\n";

static char pkey_test_key_rsa[] =
    "-----BEGIN RSA PRIVATE KEY-----\r\n"
    "MIICXAIBAAKBgQCvzifpyCVxJXDC6nNg84+Uu4edm2izOV2WD30lyuz0/VY5AS6Z\r\n"
    "EiqtUSxImkrJkqdda46VRyhSll5Ep+tHsdRc6ADlNQtOfz4EZAHrze4jkXnJ6duz\r\n"
    "b0S5ZSyDCTpVOZCZsYMYE0o2O4JglNss0zeT6DaRc13AKjrGPrywT6yfSwIDAQAB\r\n"
    "AoGAESTigYrSE+mZyHhCjibSTqfG/tij6i5i8PpLsv7KAs4dtWtnFuhNnx82WVIq\r\n"
    "juOtTI/rlKUeyob9ZGaXrCMsWUWSutCKus9myRjpGxEqWHPF0Ge6KiFKMdyTYvNk\r\n"
    "ymC3sFKDImcF619wj80e13GS4iHGrkFA5mZO1F9cxtI+RUECQQDiwzlfGlJyY8be\r\n"
    "3y1L5nKeLpXF4riz7RQQKQR5WOH6Y2HkkKrUlZeMIe668L330OWBfSsOTkGEXRUp\r\n"
    "Y+OwT1mrAkEAxnj6p9rRvtbGhl6Mlj3xWzBwXRHwsxPnjex7IEEIovRL3FmyA+4V\r\n"
    "t9TMeKxc+aurh+X7Pyk5itvcqC/5fBxw4QJASwui3AeBC5xbv3yKqBjPC+yM4p2C\r\n"
    "1QD759E7StGQj+X+Cr+Z2ZrcOaMtN67en7oBilYbPrPFWQHZNAZ73uiT2wJBAKU8\r\n"
    "X2KT+P+rDAkeemkzFNfYkhPKNdzBe4xbD38g5bHVNbs0KdK/yvELh1gIGDf8xogT\r\n"
    "3oMNLU0AEssrdcfwXcECQDsjIvLARh4qfzAcDtzkYseKNxl5C1fev0TXqYHseplC\r\n"
    "ezfOKLU5FqbA5mow/QaAAzLCw4vIv3D4HBIC+ZRRU7E=\r\n"
    "-----END RSA PRIVATE KEY-----\r\n";

/**
  * @brief      This function runs the task of mbedtls example.
  * @return     0, if OK.\n
  *             Error code, if errors occurred.\n
  */
static int mbedtls_example(void)
{
    char rsa_output[512];
    size_t olen = 0;
    int ret = 0, test_result = 0;

    LOG_I(mbedtls_proj, "%s is running\n", __func__);

    LOG_I(mbedtls_proj, "AES demonstration starts...\n");
    ret = aes_main();
    if (ret < 0) {
        test_result = -1;
    }
    LOG_I(mbedtls_proj, "AES demonstration ends...\n");

    LOG_I(mbedtls_proj, "MD5 demonstration starts...\n");
    ret = md5_main();
    if (ret < 0) {
        test_result = -1;
    }
    LOG_I(mbedtls_proj, "MD5 demonstration ends...\n");

    LOG_I(mbedtls_proj, "MPI demonstration starts...\n");
    ret = mpi_demo_main();
    if (ret < 0) {
        test_result = -1;
    }
    LOG_I(mbedtls_proj, "MPI demonstration ends...\n");

    LOG_I(mbedtls_proj, "RSA encryption & decryption demonstration starts...\n");
    ret = rsa_encrypt_main(g_plaintext, rsa_output);
    if (ret < 0) {
        test_result = -1;
    }
    ret = rsa_decrypt_main(rsa_output);
    if (ret < 0) {
        test_result = -1;
    }

    LOG_I(mbedtls_proj, "RSA encryption & decryption demonstration ends...\n");

    LOG_I(mbedtls_proj, "Public key-based encryption & decryption demonstration starts...\n");
    ret = pk_encrypt_main(g_plaintext, rsa_output, &olen, 512, pkey_test_pub_rsa, sizeof(pkey_test_pub_rsa));
    if (ret < 0) {
        test_result = -1;
    }
    ret = pk_decrypt_main(rsa_output, olen, pkey_test_key_rsa, sizeof(pkey_test_key_rsa));
    if (ret < 0) {
        test_result = -1;
    }

    LOG_I(mbedtls_proj, "Public key-based encryption & decryption demonstration ends...\n");

    LOG_I(mbedtls_proj, "Selftest demonstration starts...\n");
    ret = tls_selftest_main(0, NULL);
    if (ret < 0) {
        test_result = -1;
    }

    LOG_I(mbedtls_proj, "Selftest demonstration ends...\n");

    LOG_I(mbedtls_proj, "SSL client demonstration starts...\n");
    ret = ssl_client_main();
    if (ret < 0) {
        test_result = -1;
    }
    LOG_I(mbedtls_proj, "SSL client demonstration ends...\n");

    if (!test_result) {
        LOG_I(mbedtls_proj, "example project test success\n");
    } else {
        LOG_I(mbedtls_proj, "example project test fail\n");
    }

    return 0;
}

/**
  * @brief      Create a task for mbedtls example
  * @param[in]  void *args: Not used
  * @return     None
  */
static void app_entry(void *args)
{
    lwip_net_ready();

    mbedtls_example();

    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS); // release CPU
    }
}


static unsigned char mbedtls_client_task(uint8_t len, char *param[])
{
    /* Create a user task for demo when and how to use wifi config API  to change WiFI settings,
        * Most WiFi APIs must be called in task scheduler, the system will work wrong if called in main(),
        * For which API must be called in task, please refer to wifi_api.h or WiFi API reference.
        * xTaskCreate(user_wifi_app_entry,
        *       UNIFY_USR_DEMO_TASK_NAME,
        *       UNIFY_USR_DEMO_TASK_STACKSIZE / 4,
        *       NULL, UNIFY_USR_DEMO_TASK_PRIO, NULL);
        */
    if (pdPASS != xTaskCreate(app_entry,
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

cmd_t mbedtls_cli_cmds[] = {
    {"test", "mbedtls test cli", mbedtls_client_task, NULL },
    { NULL, NULL, NULL, NULL }
};

