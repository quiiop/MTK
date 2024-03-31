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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "sys_init.h"
#if defined(MTK_MINICLI_ENABLE)
#include "cli_def.h"
#endif /* #if defined(MTK_MINICLI_ENABLE) */

#ifdef MTK_UT_ENABLE
#include "ut.h"
#endif /* #ifdef MTK_UT_ENABLE */

#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager.h"
#endif /* #ifdef HAL_SLEEP_MANAGER_ENABLED */

#ifdef HAL_WDT_MODULE_ENABLED
#include "hal_wdt.h"
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
#include "common.h"
#include "mt7933_pos.h"
#include "hal_nvic.h"

#ifdef MTK_MT7933_CONSYS_WIFI_ENABLE
#include "gl_init.h"
#endif /* #ifdef MTK_MT7933_CONSYS_WIFI_ENABLE */

#ifdef MTK_MT7933_BT_ENABLE
#include "bt_driver.h"
#endif /* #ifdef MTK_MT7933_BT_ENABLE */

#ifdef MTK_BT_ENABLE
//#include "bt_init.h"
//#include "hal_psram.h"
#endif /* #ifdef MTK_BT_ENABLE */

#ifdef MTK_TFM_ENABLE
#include "tfm_ns_interface_iotsdk_init.h"
#endif /* #ifdef MTK_TFM_ENABLE */

#ifdef HAL_GCPU_MODULE_ENABLED
#include "hal_gcpu_internal.h"
#endif /* #ifdef HAL_GCPU_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
#include "bsp_gpio_ept_config.h"
#endif /* #ifdef HAL_GPIO_MODULE_ENABLED */

#include "lwipopts.h"
/* Task priorities. */

#ifdef MTK_UT_ENABLE
#define utTask_PRIORITY (configMAX_PRIORITIES - 1)
#endif /* #ifdef MTK_UT_ENABLE */

#ifdef MTK_MT7933_CONSYS_ENABLE
SemaphoreHandle_t gConnsysCalLock = NULL;
SemaphoreHandle_t gConnsysRadioOnLock = NULL;
#endif /* #ifdef MTK_MT7933_CONSYS_ENABLE */
#ifdef HAL_WDT_MODULE_ENABLED
void wdt_timeout_handle(hal_wdt_reset_status_t wdt_reset_status)
{
    printf("%s: status:%u\n", __FUNCTION__, (unsigned int)wdt_reset_status);
    /* assert 0 to trigger exception hanling flow */
    configASSERT(0);
}

static void wdt_init(void)
{
    hal_wdt_config_t wdt_init;
#if defined(MTK_SWLA_ENABLE) && defined(MTK_SWLA_WDT_RESET_TRACE)
    /**
     * Dump SWLA trace after watch dog reset, if system hang.
     **/
    wdt_init.mode = HAL_WDT_MODE_RESET;
#else /* #if defined(MTK_SWLA_ENABLE) && defined(MTK_SWLA_WDT_RESET_TRACE) */
    /**
     * WDT interrupt to trigger exception flow, if system hang.
     **/
    wdt_init.mode = HAL_WDT_MODE_INTERRUPT;
#endif /* #if defined(MTK_SWLA_ENABLE) && defined(MTK_SWLA_WDT_RESET_TRACE) */
    wdt_init.seconds = 60;
    hal_wdt_init(&wdt_init);
    hal_wdt_register_callback(wdt_timeout_handle);
#ifdef MTK_SYSTEM_HANG_CHECK_ENABLE
    hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
#else /* #ifdef MTK_SYSTEM_HANG_CHECK_ENABLE */
    hal_wdt_disable(HAL_WDT_DISABLE_MAGIC); //Disable WDT when MTK_SYSTEM_HANG_CHECK_ENABLE = n
#endif /* #ifdef MTK_SYSTEM_HANG_CHECK_ENABLE */
}
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */


/* for idle task feed wdt (DO NOT enter sleep mode)*/
void vApplicationIdleHook(void)
{
#ifdef HAL_WDT_MODULE_ENABLED
    hal_wdt_feed(HAL_WDT_FEED_MAGIC);
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
}

#if( configSUPPORT_STATIC_ALLOCATION == 1 )

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
    *ppxTimerTaskTCBBuffer = (StaticTask_t *) pvPortMalloc(sizeof(StaticTask_t));
    if (*ppxTimerTaskTCBBuffer != NULL) {
        *ppxTimerTaskStackBuffer = (StackType_t *) pvPortMalloc((((size_t) * pulTimerTaskStackSize) * sizeof(StackType_t)));
    }
}
/*-----------------------------------------------------------*/

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
    *ppxIdleTaskTCBBuffer = (StaticTask_t *) pvPortMalloc(sizeof(StaticTask_t));
    if (*ppxIdleTaskTCBBuffer != NULL) {
        *ppxIdleTaskStackBuffer = (StackType_t *) pvPortMalloc((((size_t) * pulIdleTaskStackSize) * sizeof(StackType_t)));
    }
}

#endif /* #if( configSUPPORT_STATIC_ALLOCATION == 1 ) */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/


#ifdef __LWIP_DEMO__

#include "hal.h"
#include "wifi_api.h"
#include "lwip/sockets.h"

extern int lwip_net_ready(void);

#ifdef __SNTP_DEMO__
#include "lwip/apps/sntp.h"
#include <time.h>

static SemaphoreHandle_t sntp_got_time;


void sntp_set_system_time(uint32_t sec)
{
    struct tm *gt = NULL;
    hal_rtc_time_t r_time;
    hal_rtc_status_t st = HAL_RTC_STATUS_OK;
    time_t utc_sec = (time_t)sec;

    LOG_I(common, "sntp_set_system_time input:  %"U32_F" s\n", sec);
    gt = gmtime((time_t *)&utc_sec);
    if (gt == NULL) {
        LOG_I(common, "gmtime fail\n", sec);
        gt = localtime((time_t *)&utc_sec);
    }

    if (gt == NULL) {
        LOG_I(common, "generate fail\n", sec);
        return;
    }

    r_time.rtc_year = (gt->tm_year % 100);
    r_time.rtc_mon = gt->tm_mon + 1;
    r_time.rtc_day = gt->tm_mday;
    r_time.rtc_week = gt->tm_wday;
    r_time.rtc_hour = gt->tm_hour;
    r_time.rtc_min = gt->tm_min;
    r_time.rtc_sec = gt->tm_sec;
    st = hal_rtc_set_time(&r_time);

    LOG_I(common, "sntp(%d-%d-%d ", r_time.rtc_year, r_time.rtc_mon, r_time.rtc_day);
    LOG_I(common, "%d:%d:%d)\n", r_time.rtc_hour, r_time.rtc_min, r_time.rtc_sec);
    LOG_I(common, "sntp st1(%u)\n", st);
    xSemaphoreGive(sntp_got_time);
}



void sntp_app_task(void *para)
{
#if (!SNTP_SERVER_DNS)
    struct ip4_addr test_addr;
#endif /* #if (!SNTP_SERVER_DNS) */
    hal_rtc_time_t r_time;
    hal_rtc_status_t st = HAL_RTC_STATUS_OK;

    LOG_I(common, "sntp_app_enter");
    sntp_got_time = xSemaphoreCreateBinary();

    //SNTP example start.
    LOG_I(common, "Begin to init SNTP");

    /** Set this to 1 to allow config of SNTP server(s) by DNS name */
#if SNTP_SERVER_DNS
    sntp_setservername(0, "1.cn.pool.ntp.org");
    sntp_setservername(1, "1.hk.pool.ntp.org");
#else /* #if SNTP_SERVER_DNS */
    IP4_ADDR(&test_addr, 213, 161, 194, 93);
    sntp_setserver(0, (const ip_addr_t *)(&test_addr));
    IP4_ADDR(&test_addr, 129, 6, 15, 29);
    sntp_setserver(1, (const ip_addr_t *)(&test_addr));
#endif /* #if SNTP_SERVER_DNS */
    sntp_init();


    xSemaphoreTake(sntp_got_time, portMAX_DELAY);

    vTaskDelay(1000);
    st = hal_rtc_get_time(&r_time);
    LOG_I(common, "GMT(%u-%u-%u ", r_time.rtc_year + 2000, r_time.rtc_mon, r_time.rtc_day);
    LOG_I(common, "%u:%u:%u)\n", r_time.rtc_hour, r_time.rtc_min, r_time.rtc_sec);
    LOG_I(common, "hal_rtc_get_time st(%u)\n", st);

    LOG_I(common, "SNTP success");
    sntp_stop();

    LOG_I(common, "example project test success.");

    vTaskDelete(NULL);
}

void sntp_app_enter(void)
{
    xTaskCreate(sntp_app_task, "sntp_app", 1024 * 4, NULL, TASK_PRIORITY_NORMAL, NULL);
}

#endif /* #ifdef __SNTP_DEMO__ */

#if defined(__TCP_SERVER_DEMO__)

#define SOCK_TCP_SRV_PORT        6500

void tcp_server_app_task(void *para)
{
    int s;
    int c;
    int ret;
    int rlen;
    struct sockaddr_in addr;
    char srv_buf[32] = {0};
    LOG_I(common, "tcp_server_test starts");

    (void)para;
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = lwip_htons(SOCK_TCP_SRV_PORT);
    addr.sin_addr.s_addr = lwip_htonl(IPADDR_ANY);

    /* Create the socket */
    s = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        LOG_I(common, "TCP server create failed");
        goto done;
    }

    ret = lwip_bind(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_I(common, "TCP server bind failed");
        goto clean;
    }

    ret = lwip_listen(s, 0);
    if (ret < 0) {
        LOG_I(common, "TCP server listen failed");
        goto clean;
    }

    do {
        socklen_t sockaddr_len = sizeof(addr);
        c = lwip_accept(s, (struct sockaddr *)&addr, &sockaddr_len);
        if (c < 0) {
            LOG_I(common, "TCP server accept error");
            break;   //connection request.
        }

        LOG_I(common, "TCP server waiting for data...");
        while ((rlen = lwip_read(c, srv_buf, sizeof(srv_buf) - 1)) != 0) {
            if (rlen < 0) {
                LOG_I(common, "read error");
                break;
            }
            srv_buf[rlen] = 0; //for the next statement - printf string.
            LOG_I(common, "TCP server received data:%s", srv_buf);

            lwip_write(c, srv_buf, rlen);      // sonar server
        }

        lwip_close(c);
    } while (0);

clean:
    lwip_close(s);
    LOG_I(common, "TCP server s close:ret = %d", ret);
done:
    LOG_I(common, "TCP server test completed");
    vTaskDelete(NULL);
}

void tcp_server_app_enter(void)
{
    xTaskCreate(tcp_server_app_task, "tcp_srv", 1024 * 4, NULL, TASK_PRIORITY_NORMAL, NULL);
}

#endif /* #if defined(__TCP_SERVER_DEMO__) */

#if defined(__UDP_SERVER_DEMO__)

#define SOCK_UDP_SRV_PORT        6600
#define TRX_PACKET_COUNT         5

void udp_server_app_task(void *para)
{
    int s;
    int ret;
    struct sockaddr_in addr, clnt_addr;
    char rcv_buf[32] = {0};
    int count = 0;
    LOG_I(common, "udp_server_test starts");

    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = lwip_htons(SOCK_UDP_SRV_PORT);
    addr.sin_addr.s_addr = lwip_htonl(IPADDR_ANY);

    /* Create the socket */
    s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        LOG_I(common, "UDP server create failed");
        goto idle;
    }

    ret = lwip_bind(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        lwip_close(s);
        LOG_I(common, "UDP server bind failed");
        goto idle;
    }

    while (count < TRX_PACKET_COUNT) {
        socklen_t clnt_len = sizeof(clnt_addr);
        ret = lwip_recvfrom(s, rcv_buf, sizeof(rcv_buf), 0, (struct sockaddr *)&clnt_addr, &clnt_len);
        if (ret <= 0) {
            lwip_close(s);
            LOG_I(common, "UDP server recv failed");
            goto idle;
        }
        LOG_I(common, "UDP server received data:%s", rcv_buf);

        lwip_sendto(s, rcv_buf, strlen(rcv_buf), 0, (struct sockaddr *)&clnt_addr, clnt_len);

        count++;
    }

    ret = lwip_close(s);
    LOG_I(common, "UDP server s close:ret = %d", ret);
idle:
    LOG_I(common, "UDP server test completed");
    vTaskDelete(NULL);
}

void udp_server_app_enter(void)
{
    xTaskCreate(udp_server_app_task, "udp_srv", 1024 * 4, NULL, TASK_PRIORITY_NORMAL, NULL);
}

#endif /* #if defined(__UDP_SERVER_DEMO__) */

#if defined(__TCP_CLIENT_DEMO__)

#ifndef TRX_PACKET_COUNT
#define TRX_PACKET_COUNT         5
#endif /* #ifndef TRX_PACKET_COUNT */

#ifndef SOCK_TCP_SRV_PORT
#define SOCK_TCP_SRV_PORT        6500
#endif /* #ifndef SOCK_TCP_SRV_PORT */

#define SOCK_TCP_SRV_IP        "192.168.43.28"

void tcp_client_app_task(void *para)
{
    int s;
    int ret;
    struct sockaddr_in addr;
    int count = 0;
    int rcv_len, rlen;
    char rcv_buf[32] = {0};
    ip4_addr_t server_ip;

    vTaskDelay(5000);
    char send_data[] = "Hello Server!";

    LOG_I(common, "tcp_client_test starts");

    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = lwip_htons(SOCK_TCP_SRV_PORT);
    ip4addr_aton((const char *)SOCK_TCP_SRV_IP, &server_ip);
    inet_addr_from_ip4addr(&addr.sin_addr, &server_ip);

    /* Create the socket */
    s = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        LOG_I(common, "TCP client create failed");
        goto idle;
    }

    /* Connect */
    ret = lwip_connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        lwip_close(s);
        LOG_I(common, "TCP client connect failed");
        goto idle;
    }

    while (count < TRX_PACKET_COUNT) {
        /* Write something */
        ret = lwip_write(s, send_data, sizeof(send_data));
        LOG_I(common, "TCP client write:ret = %d", ret);

        LOG_I(common, "TCP client waiting for data...");
        rcv_len = 0;
        while (rcv_len < (int)sizeof(send_data)) {  //sonar client
            rlen = lwip_recv(s, &rcv_buf[rcv_len], sizeof(rcv_buf) - 1 - rcv_len, 0);
            rcv_len += rlen;
        }
        LOG_I(common, "TCP client received data:%s", rcv_buf);

        count++;
        vTaskDelay(2000);
    }

    /* close */
    ret = lwip_close(s);
    LOG_I(common, "TCP client s close:ret = %d", ret);
idle:
    LOG_I(common, "TCP client test completed");
    vTaskDelete(NULL);
}

void tcp_client_app_enter(void)
{
    xTaskCreate(tcp_client_app_task, "tcp_clt", 1024 * 4, NULL, TASK_PRIORITY_NORMAL, NULL);
}

#endif /* #if defined(__TCP_CLIENT_DEMO__) */

#if defined(__UDP_CLIENT_DEMO__)
#ifndef SOCK_UDP_SRV_PORT
#define SOCK_UDP_SRV_PORT        6600
#endif /* #ifndef SOCK_UDP_SRV_PORT */
#ifndef TRX_PACKET_COUNT
#define TRX_PACKET_COUNT         5
#endif /* #ifndef TRX_PACKET_COUNT */

#define SOCK_UDP_SRV_IP        "192.168.43.28"

void udp_client_app_task(void *para)
{
    int s;
    int ret;
    int rlen;
    struct sockaddr_in addr;
    int count = 0;
    char rcv_buf[32] = {0};
    char send_data[] = "Hello Server!";
    ip4_addr_t server_ip;
    LOG_I(common, "udp_client_test starts");

    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = lwip_htons(SOCK_UDP_SRV_PORT);
    ip4addr_aton((const char *)SOCK_UDP_SRV_IP, &server_ip);
    inet_addr_from_ip4addr(&addr.sin_addr, &server_ip);

    /* Create the socket */
    s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        LOG_I(common, "UDP client create failed");
        goto idle;
    }

    /* Connect */
    ret = lwip_connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        lwip_close(s);
        LOG_I(common, "UDP client connect failed");
        goto idle;
    }

    while (count < TRX_PACKET_COUNT) {
        /* Write something */
        ret = lwip_write(s, send_data, sizeof(send_data));
        LOG_I(common, "UDP client write:ret = %d", ret);

        LOG_I(common, "UDP client waiting for server data...");
        rlen = lwip_read(s, rcv_buf, sizeof(rcv_buf) - 1);
        rcv_buf[rlen] = 0;
        LOG_I(common, "UDP client received data:%s", rcv_buf);
        count++;
        vTaskDelay(2000);
    }

    /* Close */
    ret = lwip_close(s);
    LOG_I(common, "UDP client s close:ret = %d", ret);
idle:
    LOG_I(common, "UDP client test completed");
    vTaskDelete(NULL);
}

void udp_client_app_enter(void)
{
    xTaskCreate(udp_client_app_task, "udp_clt", 1024 * 4, NULL, TASK_PRIORITY_NORMAL, NULL);
}

#endif /* #if defined(__UDP_CLIENT_DEMO__) */


#if defined(__MTK_MDNS_RESPONDER_DEMO__)
#include "lwip/mld6.h"
#include "mdns.h"
#include "dns_sd.h"

#define require_noerr(x,y) if(x) {goto y;}

#define SERVICE_NAME              ("MIPlayer")
#define SERVICE_TYPE              ("_music._tcp")
#define SERVICE_DOMAIN            ("local")
#define SERVICE_PORT              (12345)

#define KEY_DEVICEID              ("deviceid")
#define KEY_SEED                  ("seed")
#define KEY_OS                    ("os")

static DNSServiceRef client = NULL;
static TXTRecordRef txtRecord;
static uint8_t seed = 1;

log_create_module(mdns_example, PRINT_LEVEL_INFO);

static void print_user_action(const char *str)
{
    LOG_I(mdns_example, "**************************************************");
    LOG_I(mdns_example, "User Action:  %s", str);
    LOG_I(mdns_example, "**************************************************");
}

/**
  * @brief     mDNS print TXT Record info
  * @return    None
  */
static void mdns_txtrecord_info(void)
{
    // Get bytes length of TXT Record
    uint16_t length = TXTRecordGetLength(&txtRecord);

    // Get pointer of TXT Record
    const void *ptr = TXTRecordGetBytesPtr(&txtRecord);

    // Get key-value item count of TXT Record
    uint16_t count = TXTRecordGetCount(length, ptr);

    // Print TXT Record value
    char txtStr[100] = {0};
    strncpy(txtStr, ptr, length);
    LOG_I(mdns_example, "TXTRecord length=%d ptr=%s count=%d", length, txtStr, count);

    // Determine if TXT Record contains a specified key
    int ret = TXTRecordContainsKey(length, ptr, KEY_SEED);
    LOG_I(mdns_example, "TXTRecordContainsKey seed: %s\n", ret == 1 ? "yes" : "no");

    // Get value for a given key from TXT Record
    uint8_t valueLen = 0;
    const void *value = TXTRecordGetValuePtr(length, ptr, KEY_OS, &valueLen);
    char valueStr[20] = {0};
    strncpy(valueStr, value, valueLen);
    LOG_I(mdns_example, "TXTRecordGetValuePtr [os] value: %s strlen=%d valueLen=%d\n",
          valueStr, strlen(valueStr), valueLen);
}

/**
  * @brief     mDNS print TXT Record info
  * @return    None
  */
static DNSServiceErrorType mdns_update_txtrecord(void)
{
    // Remove Key-Value
    DNSServiceErrorType ret = TXTRecordRemoveValue(&txtRecord, KEY_OS);
    LOG_I(mdns_example, "TXTRecordRemoveValue [os] ret=%d", ret);
    // Update Key-Value (remove -> re-set)
    ret = TXTRecordRemoveValue(&txtRecord, KEY_SEED);
    LOG_I(mdns_example, "TXTRecordRemoveValue [seed] ret=%d", ret);
    char seedString[16];
    memset(seedString, 0, 16);
    snprintf(seedString, 16, "%d", ++seed);
    ret = TXTRecordSetValue(&txtRecord, KEY_SEED, strlen(seedString), seedString);
    LOG_I(mdns_example, "TXTRecordSetValue [seed=%d] ret=%d", seed, ret);

    // Update TXT Record
    ret = DNSServiceUpdateRecord(client,                                   // DNSServiceRef
                                 NULL,                                     // DNSRecordRef
                                 0,                                        // DNSServiceFlags
                                 TXTRecordGetLength(&txtRecord),           // txt record length
                                 TXTRecordGetBytesPtr(&txtRecord),         // txt record pointer
                                 0                                         // ttl
                                );
    LOG_I(mdns_example, "DNSServiceUpdateRecord ret=%d", ret);
    return ret;
}

/**
  * @brief     mDNS Publish Service demo
  * @return    None
  */
static DNSServiceErrorType mdns_publish_service(void)
{
    DNSServiceErrorType err;
    DNSServiceFlags flags = 0;
    const char *device = "filogic";
    const char *os = "FreeRTOS";
    char seedString[16];

    // Create TXT Record
    TXTRecordCreate(&txtRecord, 0, NULL);

    // Add Device ID
    err = TXTRecordSetValue(&txtRecord, KEY_DEVICEID, strlen(device), device);
    require_noerr(err, exit);

    // Add Seed Number
    memset(seedString, 0, 16);
    snprintf(seedString, 16, "%d", seed);
    err = TXTRecordSetValue(&txtRecord, KEY_SEED, strlen(seedString), seedString);
    require_noerr(err, exit);

    // Add OS
    err = TXTRecordSetValue(&txtRecord, KEY_OS, strlen(os), os);
    require_noerr(err, exit);

    LOG_I(mdns_example, "Register Bonjour Service: %s type: %s domain: %s port: %d",
          SERVICE_NAME, SERVICE_TYPE, SERVICE_DOMAIN, SERVICE_PORT);

    // Register Bonjour Service
    err = DNSServiceRegister(&client,                      // DNSServiceRef
                             flags,                                         // DNSServiceFlags
                             kDNSServiceInterfaceIndexAny,                  // interface index
                             SERVICE_NAME,                                  // service name
                             SERVICE_TYPE,                                  // service type
                             SERVICE_DOMAIN,                                // domain
                             NULL,                                          // host
                             htons(SERVICE_PORT),                           // port
                             TXTRecordGetLength(&txtRecord),                // txt record length
                             TXTRecordGetBytesPtr(&txtRecord),              // txt record pointer
                             NULL,                                          // callback
                             NULL);                                         // context
    if (err == kDNSServiceErr_NoError) {
        LOG_I(mdns_example, "Register Bonjour Service successfully!");
    } else {
        LOG_E(mdns_example, "Register Bonjour Service failed %d.", err);
    }
    require_noerr(err, exit);
    return err;

exit:
    TXTRecordDeallocate(&txtRecord);
    return err;
}

/**
  * @brief     mDNS Daemon Task entry
  * @param[in] void *not_used:Not used
  * @return    None
  */
static void mdnsd_entry(void *not_used)
{
    LOG_I(mdns_example, "mdnsd_entry start");
    mdnsd_start();
    LOG_I(mdns_example, "mdnsd_entry return");
    client = NULL;
    vTaskDelete(NULL);
}

void mdns_responder_app_task(void *para)
{
    // Support IPv6 Address after link_up and Got IP
#if LWIP_IPV6
    struct netif *sta_if = netif_default;
    ip6_addr_t mld_address;
#endif /* #if LWIP_IPV6 */

    vTaskDelay(5 * 1000 / portTICK_RATE_MS);

#if LWIP_IPV6
    //netif_create_ip6_linklocal_address(sta_if, 1);
    sta_if->ip6_autoconfig_enabled = 1;
    ip6_addr_set_solicitednode(&mld_address, netif_ip6_addr(sta_if, 0)->addr[3]);
    mld6_joingroup(netif_ip6_addr(sta_if, 0), &mld_address);
#endif /* #if LWIP_IPV6 */

    // xTaskHandle create mDNS daemon task
    if (pdPASS != xTaskCreate(mdnsd_entry,
                              "mdnsd",
                              (15 * 1024) / sizeof(portSTACK_TYPE),
                              NULL,
                              TASK_PRIORITY_NORMAL,
                              NULL)) {
        LOG_I(mdns_example, "Cannot create mdnsd_task");
    }
    LOG_I(mdns_example, "Begin to create mdnsd_task");

    // Publish mDNS Service
    vTaskDelay(10 * 1000 / portTICK_RATE_MS);
    DNSServiceErrorType err = mdns_publish_service();
    if (err == kDNSServiceErr_NoError) {
        print_user_action("Please use Bonjour Browser Application to discover/resolve the service in 30 seconds");
    }

    // Print TXT Record info
    vTaskDelay(30 * 1000 / portTICK_RATE_MS);
    mdns_txtrecord_info();

    // Update TXT Record
    err += mdns_update_txtrecord();
    if (err == kDNSServiceErr_NoError) {
        print_user_action("Please verify the updated TXT Record in 30 seconds");
    }

    // Unregister/Release Service
    vTaskDelay(30 * 1000 / portTICK_RATE_MS);
    LOG_I(mdns_example, "DNSServiceRefDeallocate start");
    TXTRecordDeallocate(&txtRecord);
    DNSServiceRefDeallocate(client);
    LOG_I(mdns_example, "DNSServiceRefDeallocate end");
    print_user_action("Please verify that the service has deregistered/disappeared in 60 seconds");

    // Stop mdnsd task until completion or timeout after 10 seconds
    vTaskDelay(60 * 1000 / portTICK_RATE_MS);
    LOG_I(mdns_example, "Stop mdnsd task");
    mdnsd_stop();

    // Verify result
    LOG_I(mdns_example, "user_entry err=%d client=%d", err, client);
    if (err == kDNSServiceErr_NoError && client == NULL) {
        LOG_I(mdns_example, "example project test success.\n");
    } else {
        LOG_I(mdns_example, "example project test fail.\n");
    }

    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS); // release CPU
    }
}

void mdns_responder_app_enter(void)
{
    xTaskCreate(mdns_responder_app_task, "mdnsdemo", 1024 * 4, NULL, TASK_PRIORITY_NORMAL, NULL);
}

#endif /* #if defined(__MTK_MDNS_RESPONDER_DEMO__) */


int32_t wifi_event_handler_app(wifi_event_t event, uint8_t *payload,
                               uint32_t length)
{
    LOG_I(common, "wifi event is %d", event);
    return 1;
}

void demo_app_task(void *para)
{
    wifi_config_t config = {0};
    //wifi_config_ext_t config_ext = {0};

    config.opmode = WIFI_MODE_STA_ONLY;
    strcpy((char *)config.sta_config.ssid, (const char *)"MTK_SOFT_AP");
    config.sta_config.ssid[11] = 0;
    config.sta_config.ssid_length = 11;

    config.sta_config.bssid_present = 0;
    strcpy((char *)config.sta_config.password, (const char *)"12345678");
    config.sta_config.password[8] = 0;
    config.sta_config.password_length = 8;

    wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE, wifi_event_handler_app);
    wifi_init(&config, NULL);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_CONNECTED, wifi_event_handler_app);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_SCAN_COMPLETE, wifi_event_handler_app);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_DISCONNECTED, wifi_event_handler_app);
    wifi_config_set_security_mode(WIFI_PORT_STA, WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK, WIFI_ENCRYPT_TYPE_AES_ENABLED);
    wifi_config_reload_setting();

    //Wait net ready.
    lwip_net_ready();

#ifdef __SNTP_DEMO__
    sntp_app_enter();
#endif /* #ifdef __SNTP_DEMO__ */
#if defined(__TCP_SERVER_DEMO__)
    tcp_server_app_enter();
#endif /* #if defined(__TCP_SERVER_DEMO__) */
#if defined(__UDP_SERVER_DEMO__)
    udp_server_app_enter();
#endif /* #if defined(__UDP_SERVER_DEMO__) */
#if defined(__TCP_CLIENT_DEMO__)
    tcp_client_app_enter();
#endif /* #if defined(__TCP_CLIENT_DEMO__) */
#if defined(__UDP_CLIENT_DEMO__)
    udp_client_app_enter();
#endif /* #if defined(__UDP_CLIENT_DEMO__) */
#if defined(__MTK_MDNS_RESPONDER_DEMO__)
    mdns_responder_app_enter();
#endif /* #if defined(__MTK_MDNS_RESPONDER_DEMO__) */
    while (1) {
        uint8_t link_status = 0;
        wifi_connection_get_link_status(&link_status);
        LOG_I(common, "wifi link is %d", link_status);
        vTaskDelay(1000);
    }
}


void demo_app_init(void)
{
    xTaskCreate(demo_app_task, "demo_app", 1024 * 4, NULL, TASK_PRIORITY_NORMAL, NULL);
}

#endif /* #ifdef __LWIP_DEMO__ */

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
    /* Do system initialization, eg: hardware, nvdm, logging and random seed. */
    system_init();

#ifdef HAL_GPIO_MODULE_ENABLED
    bsp_ept_gpio_setting_init();
#endif /* #ifdef HAL_GPIO_MODULE_ENABLED */

#if defined(MTK_MINICLI_ENABLE)
    /* Initialize cli task to enable user input cli command from uart port.*/
    cli_def_create();
    cli_task_create();
#endif /* #if defined(MTK_MINICLI_ENABLE) */

    /* Call this function to indicate the system initialize done. */
    //SysInitStatus_Set();

#ifdef HAL_WDT_MODULE_ENABLED
    wdt_init();
#endif /* #ifdef HAL_WDT_MODULE_ENABLED */
    printf("Hello Mediatek!!\r\n\r\n");

#ifdef MTK_TFM_ENABLE
    tfm_ns_interface_iotsdk_init();
#endif /* #ifdef MTK_TFM_ENABLE */

#ifdef MTK_MT7933_CONSYS_ENABLE
    gConnsysCalLock = xSemaphoreCreateMutex();
    gConnsysRadioOnLock = xSemaphoreCreateMutex();
#if defined(BRINGUP_WIFI_ENABLE) || defined(BRINGUP_BT_ENABLE)
    mt7933_conninfra_init();
#ifdef MTK_BT_ENABLE
    //bt_driver_init();
#endif /* #ifdef MTK_BT_ENABLE */
#endif /* #if defined(BRINGUP_WIFI_ENABLE) || defined(BRINGUP_BT_ENABLE) */
#ifdef MTK_MT7933_CONSYS_WIFI_ENABLE
    /* mask out connsys on by default beofre ES for build sanity check */
    /* wifi_init_task(); */
#endif /* #ifdef MTK_MT7933_CONSYS_WIFI_ENABLE */
#endif /* #ifdef MTK_MT7933_CONSYS_ENABLE */

#ifdef HAL_GCPU_MODULE_ENABLED
    gcpu_init();
#endif /* #ifdef HAL_GCPU_MODULE_ENABLED */

    //#ifdef MTK_UT_ENABLE
    //    if (xTaskCreate((TaskFunction_t)ut_main, "utTask", 1024, NULL, utTask_PRIORITY, NULL) != pdPASS)
    //    {
    //        printf("Task creation failed!.\r\n");
    //        while (1);
    //    }
    //#endif

#ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE
    printf("AUDIO DRIVER INIT\n");
    extern void audio_init(void);
    audio_init();
#endif /* #ifdef MTK_MT7933_AUDIO_DRIVER_ENABLE */

#ifdef MTK_BT_ENABLE
    extern void bt_app_common_init(void);
    bt_app_common_init();
#endif /* #ifdef MTK_BT_ENABLE */

#ifdef __LWIP_DEMO__
    demo_app_init();
#endif /* #ifdef __LWIP_DEMO__ */

    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for (;;);
}


