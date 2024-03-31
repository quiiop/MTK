/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "wifi_api.h"

#include "MQTTClient.h"
#include "mqtt.h"
#include "syslog.h"

#define TEST_MQTT_MOSQUITTO_CA_CRT                                      \
"-----BEGIN CERTIFICATE-----\r\n"                                       \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n"  \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n"  \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n"  \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n"  \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n"  \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n"  \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n"  \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n"  \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n"  \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n"  \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n"  \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n"  \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n"  \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n"  \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n"  \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n"  \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n"  \
"rqXRfboQnoZsG4q5WTP468SQvvG5\r\n"  \
"-----END CERTIFICATE-----"
/* CA1 */


#define TEST_MQTT_CA_CRT                                                \
"-----BEGIN CERTIFICATE-----\r\n"                                       \
"MIIDWTCCAkGgAwIBAgIUQL9/CQjqMyW9borwV1g9yGBn1HowDQYJKoZIhvcNAQEL\r\n"  \
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\r\n"  \
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIxMTEwNDExNTQz\r\n"  \
"MVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\r\n"  \
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAJWfYpECVVyQueBnMMRW\r\n"  \
"Bp17QiEvpMKTdzfjnlWug9ht2qQCr5W2og/L3VAeFl52LR5DPiFePshvr6KqnbC3\r\n"  \
"SfwJWAXgS157biHgQhlW5mO3FoWaiWxZZWmTn577Sal8mYchM19JWCM3xKufwRYm\r\n"  \
"vacxp5VSTEL6CEamz5zwBcoeJaoipylEmEAf61M6paP6B6QqgkvdUCnMQas/9dOK\r\n"  \
"5iziSmx6dhqBpxJ44bABCpwFWGOUZr1+Vi+b9G+37P26VTYwxk2GnI1m4WQYopbj\r\n"  \
"i4vvF9dun9oJxq8YIw23GrQRHKsMZTkQO/sr+j6EXi0ajHlh8YG+JoUQhCt5sXgd\r\n"  \
"fzECAwEAAaNgMF4wHwYDVR0jBBgwFoAUCaInwyhUPRy/NgxId/6zQ/u9vukwHQYD\r\n"  \
"VR0OBBYEFLmWeXEkyaGLo+mCUkkuGBe7gVMsMAwGA1UdEwEB/wQCMAAwDgYDVR0P\r\n"  \
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQDJ92dltLgK9IkR/LL6X9bf5KKa\r\n"  \
"FuSUVRgwbZHYVSqMnTDW1e5qF5Hri6Y8Eh8U5hRldqFW2rJfqVN9ZvuuSWVaQklz\r\n"  \
"hCLNtxX8z5ryl6+WOdIoVl+6XP/qHaHsednfUxTFYShG2/JKq19nfs4mSlcgIsAE\r\n"  \
"nqiCbIhkJyCBYV6vWrcGbDre+1Y1BjiNFXHZ/jFP92X+ZzPuLgfgY/nyvGoBkN0d\r\n"  \
"ipoN+qEfz6b8Kk44oPDkIvUemh8gcKYJill+w8U3bHQ12CZotwNm0KfGFhnVhMCr\r\n"  \
"4p/vmI6Klr106ldUIzMxwEVtluCMP5D/vqA+Yyf4NBZvpaqLOs2dUs1vNvzP\r\n"  \
"-----END CERTIFICATE-----"
/*NOT used*/


#define TEST_MQTT_CLI_CRT                                               \
"-----BEGIN CERTIFICATE-----\r\n"                                       \
"MIIDWTCCAkGgAwIBAgIUQL9/CQjqMyW9borwV1g9yGBn1HowDQYJKoZIhvcNAQEL\r\n"  \
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\r\n"  \
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIxMTEwNDExNTQz\r\n"  \
"MVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\r\n"  \
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAJWfYpECVVyQueBnMMRW\r\n"  \
"Bp17QiEvpMKTdzfjnlWug9ht2qQCr5W2og/L3VAeFl52LR5DPiFePshvr6KqnbC3\r\n"  \
"SfwJWAXgS157biHgQhlW5mO3FoWaiWxZZWmTn577Sal8mYchM19JWCM3xKufwRYm\r\n"  \
"vacxp5VSTEL6CEamz5zwBcoeJaoipylEmEAf61M6paP6B6QqgkvdUCnMQas/9dOK\r\n"  \
"5iziSmx6dhqBpxJ44bABCpwFWGOUZr1+Vi+b9G+37P26VTYwxk2GnI1m4WQYopbj\r\n"  \
"i4vvF9dun9oJxq8YIw23GrQRHKsMZTkQO/sr+j6EXi0ajHlh8YG+JoUQhCt5sXgd\r\n"  \
"fzECAwEAAaNgMF4wHwYDVR0jBBgwFoAUCaInwyhUPRy/NgxId/6zQ/u9vukwHQYD\r\n"  \
"VR0OBBYEFLmWeXEkyaGLo+mCUkkuGBe7gVMsMAwGA1UdEwEB/wQCMAAwDgYDVR0P\r\n"  \
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQDJ92dltLgK9IkR/LL6X9bf5KKa\r\n"  \
"FuSUVRgwbZHYVSqMnTDW1e5qF5Hri6Y8Eh8U5hRldqFW2rJfqVN9ZvuuSWVaQklz\r\n"  \
"hCLNtxX8z5ryl6+WOdIoVl+6XP/qHaHsednfUxTFYShG2/JKq19nfs4mSlcgIsAE\r\n"  \
"nqiCbIhkJyCBYV6vWrcGbDre+1Y1BjiNFXHZ/jFP92X+ZzPuLgfgY/nyvGoBkN0d\r\n"  \
"ipoN+qEfz6b8Kk44oPDkIvUemh8gcKYJill+w8U3bHQ12CZotwNm0KfGFhnVhMCr\r\n"  \
"4p/vmI6Klr106ldUIzMxwEVtluCMP5D/vqA+Yyf4NBZvpaqLOs2dUs1vNvzP\r\n"  \
"-----END CERTIFICATE-----"
/*certificate.pem.crt*/


#define TEST_MQTT_CLI_KEY                                               \
"-----BEGIN RSA PRIVATE KEY-----\r\n"                                   \
"MIIEpAIBAAKCAQEAlZ9ikQJVXJC54GcwxFYGnXtCIS+kwpN3N+OeVa6D2G3apAKv\r\n"  \
"lbaiD8vdUB4WXnYtHkM+IV4+yG+voqqdsLdJ/AlYBeBLXntuIeBCGVbmY7cWhZqJ\r\n"  \
"bFllaZOfnvtJqXyZhyEzX0lYIzfEq5/BFia9pzGnlVJMQvoIRqbPnPAFyh4lqiKn\r\n"  \
"KUSYQB/rUzqlo/oHpCqCS91QKcxBqz/104rmLOJKbHp2GoGnEnjhsAEKnAVYY5Rm\r\n"  \
"vX5WL5v0b7fs/bpVNjDGTYacjWbhZBiiluOLi+8X126f2gnGrxgjDbcatBEcqwxl\r\n"  \
"ORA7+yv6PoReLRqMeWHxgb4mhRCEK3mxeB1/MQIDAQABAoIBAG3ruUsVLschYB03\r\n"  \
"UkF7fcKmcg/VY7K7CcPailEiM6SGGDGT85EGqor2V7HrWL2vDjiEaBm1lmREgQD9\r\n"  \
"XXodPdJWvv8aJ+TSwzdJ/OWGzR2GvZoLzVf3WSS2Gnd+30AP1O7MemRPitvctbTs\r\n"  \
"Wbs/XOd++EIgQvyVmgzlsfL29xnj6me2jd3fo6aV0pvwdhtCoq4yO+02JLMa81V5\r\n"  \
"NvJyw4V0TIfONf/le2eCV38CZCUXsvsNUp1bCZz6h7vheQSnBoZKiOtovv7NaHpG\r\n"  \
"M9DVtn3bD2V8gshlBDDB5o22HFVCeEJSKiq8ESM0FLIJbNa15JKY8qoizlgV+DTi\r\n"  \
"OCdwAp0CgYEAxtLhH1/2stFG9fuTbE/BdCe4qoRwAT39XU8EAYYqa+QW5fRbYL+Q\r\n"  \
"yOSdxW9I2XJtN81BKUTgCB8mi0KYCgkLg8q9HfjYtVl5pCzeMnkXGIJcBZeloge8\r\n"  \
"4dbBjW8h4bC8h9M7oQgocbVzuZNIfnBEBfRNYCASo2ChMwlVCAocMdsCgYEAwKZj\r\n"  \
"HtYNR9hXLyEdqTY6mM8zIKxjsLHtuoSmyECV6KujsKwxUmppM6aXbJ4/O9C1v4lS\r\n"  \
"x4DkwC3BO8bwvaU4JrQaiDM8ocJeS0fxA6tub9a9lceJA+uvXi5tFEjHM+a/OmE+\r\n"  \
"NxqJDTZqhfBUC8MY+1Uz7okpmoLmtFQ7nYbS/uMCgYEAmaONaSRLy6SidCTip/j9\r\n"  \
"odesAMB0mNEhP4B2tQLLNzI7a5knH50IseTP8ihrn/SnZ06KAu0BPeVscIKWYHLa\r\n"  \
"3g3FlCqes4yfzfkJ/vDJoxvhJkUoLgxSriW4zaVKBk//b+oQEwDt1+iCs62XgHUa\r\n"  \
"a4t8W0KNhIiAKKMWfS7q2K8CgYEAvs9OF4mtJ77IQ37KUQHMMfPvUOpN5wGkU3v1\r\n"  \
"cRw/Hm96No+mXlnVDvpuun9WZBlJGoeZ/M3WQ95NqLZXVY7iObtjGAmfgRvxpyB/\r\n"  \
"P8I5JP6NUl+Kft6eGGtlUJTw8KAYQWt1YsCeg/6krTqnE+tNwAnY4Obr1QTYGj3f\r\n"  \
"uhuCuj8CgYAYoFomEz6jr54kyqquGo5LAu90PJMjdvA8s+2yGLzibjC3NGI/yx4j\r\n"  \
"8l/QIS7vsn4jGXU2pnuzYHC55K62Ho5qVOBPbRuw6WpakaVUNFjrlnEL3SIfacQN\r\n"  \
"phoxlo1QB+s1MPgxyxsjpGoi7jXDS9aWoXpxneTR2BOKzhGUM9RAjQ==\r\n"  \
"-----END RSA PRIVATE KEY-----"
/*private key*/

static const char mqtt_ca_cert[] = TEST_MQTT_MOSQUITTO_CA_CRT;
static const size_t mqtt_ca_crt_len  = sizeof(mqtt_ca_cert);

#define MQTT_SERVER     "a1dfsfceso2wjo-ats.iot.us-east-2.amazonaws.com"
#define MQTT_PORT       "8883"
#define MQTT_TOPIC      "test123"
#define MQTT_CLIENT_ID  "mqtt-7933-client-ssl"
#define MQTT_MSG_VER    "0.50"

static int arrivedcount = 0;

log_create_module(mqtt_tls, PRINT_LEVEL_INFO);


/**
* @brief          MQTT message RX handle
* @param[in]      MQTT received message data
* @return         None
*/
static void messageArrived(MessageData *md)
{
    MQTTMessage *message = md->message;

    LOG_I(mqtt_tls, "Message arrived: qos %d, retained %d, dup %d, packetid %d\n",
          message->qos, message->retained, message->dup, message->id);
    LOG_I(mqtt_tls, "Payload %d.%s\n", (size_t)(message->payloadlen), (char *)(message->payload));
    ++arrivedcount;
}

/**
* @brief          MQTT client example entry function
* @return         None
*/
extern int mqtt_client_example_ssl(void)
{
    int rc = 0;
    unsigned char msg_buf[100];     /* Buffer for outgoing messages, such as unsubscribe. */
    unsigned char msg_readbuf[100]; /* Buffer for incoming messages, such as unsubscribe ACK. */
    char buf[100];                  /* Buffer for application payload. */

    Network n;  /* TCP network */
    Client c;   /* MQTT client */
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    char *topic = MQTT_TOPIC;
    MQTTMessage message;

    const char mqtt_cli_cert[] = TEST_MQTT_CLI_CRT;
    const char mqtt_cli_key[] = TEST_MQTT_CLI_KEY;
    const char mqtt_cli_pwd[] = "";
    const size_t mqtt_cli_crt_len  = sizeof(mqtt_cli_cert);
    const size_t mqtt_cli_key_len  = sizeof(mqtt_cli_key);
    const size_t mqtt_cli_pwd_len  = 0;

    /* Initialize MQTTnetwork structure */
    NewNetwork(&n);

    /* Connect to remote server */
    LOG_I(mqtt_tls, "TLS Connect to %s:%s\n", MQTT_SERVER, MQTT_PORT);
    rc = TLSConnectNetwork(&n, MQTT_SERVER, MQTT_PORT, mqtt_ca_cert, mqtt_ca_crt_len,
                           mqtt_cli_cert, mqtt_cli_crt_len,
                           mqtt_cli_key, mqtt_cli_key_len,
                           mqtt_cli_pwd, mqtt_cli_pwd_len);

    if (rc != 0) {
        LOG_I(mqtt_tls, "TCP connect fail,status -%4X\n", -rc);
        return rc;
    }

    /* Initialize MQTT client structure */
    MQTTClient(&c, &n, 120000/*Legend: 12000*/, msg_buf, 100, msg_readbuf, 100);

    /* The packet header of MQTT connection request */
    data.willFlag = 0;
    data.MQTTVersion = 3;
    data.clientID.cstring = MQTT_CLIENT_ID;
    data.username.cstring = NULL;
    data.password.cstring = NULL;
    data.keepAliveInterval = 200;
    data.cleansession = 1;

    /* Send MQTT connection request to the remote MQTT server */
    rc = MQTTConnect(&c, &data);

    if (rc != 0) {
        LOG_I(mqtt_tls, "MQTT connect failed,status -%4X\n", -rc);
        return rc;
    }

    LOG_I(mqtt_tls, "Subscribing to %s\n", topic);
    rc = MQTTSubscribe(&c, topic, /*QOS1*/QOS0, messageArrived);
    LOG_I(mqtt_tls, "Client Subscribed %d\n", rc);

    /* QoS 0 */
    sprintf(buf, "{\n  \"msg\": \"MQTT test1 from mt7933\"\n}");
    message.qos = QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void *)buf;
    message.payloadlen = strlen(buf) + 1;
    rc = MQTTPublish(&c, topic, &message);

    while (arrivedcount < 1) {
        MQTTYield(&c, 10000);/*Legend: 1105 Add timeout*/
    }


    /* QoS 1 */
    sprintf(buf, "Hello World! QoS 1 message from app version %s\n", MQTT_MSG_VER);
    message.qos = QOS1;
    message.payloadlen = strlen(buf) + 1;
    rc = MQTTPublish(&c, topic, &message);
    while (arrivedcount < 2) {
        MQTTYield(&c, 1000);
    }
#if 0
    /* QoS 2 */
    sprintf(buf, "Hello World! QoS 2 message from app version %s\n", MQTT_MSG_VER);
    message.qos = QOS2;
    message.payloadlen = strlen(buf) + 1;
    rc = MQTTPublish(&c, topic, &message);
    while (arrivedcount < 3) {
        MQTTYield(&c, 1000);
    }
#endif /* #if 0 */

    if ((rc = MQTTUnsubscribe(&c, topic)) != 0) {
        LOG_I(mqtt_tls, "The return from unsubscribe was %d\n", rc);
    }
    LOG_I(mqtt_tls, "MQTT unsubscribe done\n");

    if ((rc = MQTTDisconnect(&c)) != 0) {
        LOG_I(mqtt_tls, "The return from disconnect was %d\n", rc);
    }
    LOG_I(mqtt_tls, "MQTT disconnect done\n");

    n.disconnect(&n);
    LOG_I(mqtt_tls, "Network disconnect done\n");
    LOG_I(mqtt_tls, "example project test success.");
    return 0;
}
