/**
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2016. All rights reserved.
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

//---------------------------------------------------------------------------
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "task_def.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef PICUS_LOG2FILE
#include <dirent.h>
#endif
#include "picus.h"
#include "bt_driver.h"
#if (PICUS_CHIPID == 0x6631)
#include "connsys_debug_utility.h"
#include "wmt.h"
#elif (PICUS_CHIPID == 0x7933)
#include "btif_mt7933.h"
#endif

#ifdef __ICCARM__
#include <getopt.h>
#include <time.h>
#else
#include <sys/time.h>
#endif /* __ICCARM__ */

#define PICUS_VERSION     "1.0.22071901"
#define LOG_VERSION 0x100

/* Enable this macro for monitor the amount of picus log */
//#define PICUS_LOG_COUNT_MONITOR

#define DUMP_PICUS_NAME_EXT ".picus"
#define DUMP_PICUS_NAME_PREFIX "dump_"
#define FWLOG_DEFAULT_PATH  "/data"
#define FWLOG_PATH_LENGTH   (128)

#define RX_BUF_SIZE   (1024 * 10)   //if read buffer is too small, sys log may lost due to buffer full in bus driver
#define FWLOG_DEFAULT_SIZE  (10 * 1024 * 1024)
#define FWLOG_DEFAULT_NUM  (1)
#define PICUS_ACL_HDR_LEN  (5)
#define PICUS_HDR_LEN      (24)

//fwlog packet header ({0xAB,0xCD, 10, (unsigned char)((len & 0xff00) >> 8), (unsigned char)(len & 0xff)}) when log to UART
#define COM_PORT_FWLOG_PKT_HDR_LEN    (5)
#define COM_PORT_FWLOG_PKT_TAIL_LEN   (2)
#define COM_PORT_FWLOG_END_SIZE       (4)
static unsigned char fwlog_pkt_hdr[COM_PORT_FWLOG_PKT_HDR_LEN] = {0xAB, 0xCD, 0x0A, 0x00, 0x00};
static unsigned char fwlog_pkt_tail[COM_PORT_FWLOG_PKT_TAIL_LEN] = {0xDC, 0xBA};

//coredump packet header ({0xAB,0xCD, 2, (unsigned char)((len & 0xff00) >> 8), (unsigned char)(len & 0xff), 0xFF F0 xx xx(2 byte length) 6F FC xx xx(2 byte length)}) when log to UART
#define COM_PORT_COREDUMP_PKT_HDR_LEN    (13)
static unsigned char coredump_pkt_hdr[COM_PORT_COREDUMP_PKT_HDR_LEN] = {0xAB, 0xCD, 0x02, 0x00, 0x00, 0xFF, 0xF0, 0x00, 0x00, 0x6F, 0xFC, 0x00, 0x00};

static const unsigned long long BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

#define PICUS_BUFFER_THRESHOLD  (1024)
#ifdef PICUS_WITHOUT_BUFFER
FWLOG_PACKET_T packet_queue; //store left packet.
#else
#define PICUS_TASK_PRI (BLUETOOTH_TASK_PRIO - 1) /* Should lower than the priority of bt_task */
#define PICUS_POP_MAX_RETRY 3
FWLOG_PACKET_T packet_buf = {0};
xSemaphoreHandle packet_queue_mtx = NULL;
#endif

static TaskHandle_t picus_rx_task_hdl = NULL;

static unsigned int g_log_file_num = FWLOG_DEFAULT_NUM;
static int g_log_file_size = FWLOG_DEFAULT_SIZE;
static int g_log_file_remain_size = FWLOG_DEFAULT_SIZE;
int g_log_level = FWLOG_LVL_SQC;
#if (PICUS_CHIPID == 0x6631)
static unsigned char g_rx_buf[RX_BUF_SIZE] = {0};
static int g_log_via = FWLOG_VIA_EMI;
static unsigned char g_log_dest = LOG_TO_FS;
#elif (PICUS_CHIPID == 0x7933)
#define DEFAULT_PICUS_UART_PORT HAL_UART_2
#define DEFAULT_PICUS_UART_BAUDRATE 921600
static int g_log_out_port = DEFAULT_PICUS_UART_PORT;
static int uart1_baudrate = (int)DEFAULT_PICUS_UART_BAUDRATE;
static int g_log_via = FWLOG_VIA_ACL;
static unsigned char g_log_dest = LOG_TO_UART;
#endif
static char g_log_path[FWLOG_PATH_LENGTH] = {0};
static int g_current_fd = 0;
unsigned int g_current_idx = 0;
static char g_current_log_full_name[FWLOG_PATH_LENGTH] = {0};
static unsigned long long g_timestamp = 0;
static int g_driver_is_ready = 0;
static unsigned char g_send_enable_fwlog = 1; //default enable fwlog
static bool set_bperf = pdFALSE;
static uint16_t log_full_called_cnt = 0;
static uint32_t log_drop_total_data = 0;

#ifdef PICUS_LOG_COUNT_MONITOR
#define LOG_TOTAL_DATA_UPPER_BOUND (1024 * 100)
static uint32_t g_fwlog_total_size = 0;
#endif

#define C2N(x) (x <= '9' ? x - '0' : (x > 'F' ? x - 'a' + 10 : x - 'A' + 10))

//picus_write_to_uart shall be platform specific
#pragma weak picus_write_to_uart = default_picus_write_to_uart
extern int picus_write_to_uart(unsigned char *buf, int len);
static int picus_log_out(unsigned char act, unsigned char dest, unsigned char pkt_type,
                        unsigned char *data, unsigned int len);
static int default_picus_write_to_uart(unsigned char *buf, int len)
{
    //bt_driver_dump_buffer("write picus log", buf, len, 1);
    return bt_uart_write(g_log_out_port, buf, len);
}

//---------------------------------------------------------------------------
int g_debuglevel = WARN;        // show priority than WARN
void DBGPRINT(int level, const char *format, ...)
{
    int n = 0;
    int ret = 0;

    if (level > g_debuglevel)
        return;

    char buf[1024] = "";
    va_list Arg;
    va_start(Arg, format);
    n = vsnprintf(buf, 300, format, Arg);
    if (n < 0 || n > 300)
        LOG_E(PICUS, "vsnprintf fail\n");
    va_end(Arg);
#if (PICUS_CHIPID == 0x7933)
    LOG_I(PICUS, "%s", buf);
#else
    printf("[picus] %s\n", buf);
#endif
    ret = fflush(stdout);
    if (ret)
        LOG_E(PICUS, "fflush error\n");

    return;
}

static void usage(void)
{
    DBGPRINT(SHOW, "Usage: picus [option] [path | command]");
    DBGPRINT(SHOW, "[option]");
    DBGPRINT(SHOW, "\t-d [command]\tSend debug command");
    DBGPRINT(SHOW, "\t  \t\tUsing \"kill\" command to kill all picus");
    DBGPRINT(SHOW, "\t  \t\tUsing \"trigger\" command to trigger fw assert");
    DBGPRINT(SHOW, "\t  \t\tUsing \"rssi\" command to read rssi");
#ifndef MTK_BT_PICUS_CLI_LITE
    DBGPRINT(SHOW, "\t  \t\tUsing \"bperf\" command to read Bluetooth KPI data");
    DBGPRINT(SHOW, "\t  \t\tUsing \"inquiry\" command to send inquiry command");
    DBGPRINT(SHOW, "\t  \t\tUsing \"ble_scan_on\" command to enable ble scan");
    DBGPRINT(SHOW, "\t  \t\tUsing \"ble_scan_off\" command to disable ble scan");
    DBGPRINT(SHOW, "\t  \t\tUsing \"afh\" command to read afh table");
    DBGPRINT(SHOW, "\t  \t\tUsing \"per\" command to read per");
    DBGPRINT(SHOW, "\t  \t\tUsing \"en_rssi\" command to enable read rssi/channel every package");
    DBGPRINT(SHOW, "\t  \t\tUsing \"dis_rssi\" command to disable read rssi/channel every package");
    DBGPRINT(SHOW, "\t-c [command]\tsend command");
    DBGPRINT(SHOW, "\t-p [path]\tOutput the file to specific dictionary");
    DBGPRINT(SHOW, "\t-n [NO]\t\tChange the output file number");
    DBGPRINT(SHOW, "\t-s [bytes]\tChange the output file size");
    DBGPRINT(SHOW, "\t-t [seconds]\tChange the bperf average timer length");
#endif /* #ifndef MTK_BT_PICUS_CLI_LITE */
    DBGPRINT(SHOW, "\t-f\t\tLog level: debud mode (can't be used with -o at the same time)");
    DBGPRINT(SHOW, "\t-o\t\tLog level: low power mode (can't be used with -f at the same time)");
    DBGPRINT(SHOW, "\t-v [via]\tlog via: 0: event, 1: EMI, 2: ACL(Default), 3: Debug Uart");
    DBGPRINT(SHOW, "\t-x [dest]\tlog dest: 0: to file system, others: to UART");
    DBGPRINT(SHOW, "\t-b [baudrate]\tChange UART baudrate.");
    DBGPRINT(SHOW, "\t-u\t\tChange UART port number.");
}

unsigned short int rtos_htobe16(unsigned short int input_val)
{
    unsigned short int temp = 0;
    temp = (input_val & 0x00FF) << 8;
    temp |= ((input_val & 0xFF00) >> 8) & 0x00FF;
    return temp;
}

unsigned int rtos_htobe32(unsigned int input_val)
{
    int temp = 0;
    temp = (input_val & 0x000000FF) << 24;
    temp |= (input_val & 0x0000FF00) << 8;
    temp |= ((input_val & 0x00FF0000) >> 8) & 0x0000FF00;
    temp |= ((input_val & 0xFF000000) >> 24) & 0x000000FF;
    return temp;
}

unsigned long long rtos_htobe64(unsigned long long input_val)
{
    unsigned long long temp = 0;
    temp |= (input_val & 0x00000000000000FF) << 56;
    temp |= (input_val & 0x000000000000FF00) << 40;
    temp |= (input_val & 0x0000000000FF0000) << 24;
    temp |= (input_val & 0x00000000FF000000) << 8;
    temp |= ((input_val & 0x000000FF00000000) >> 8) & 0x00000000FF000000;
    temp |= ((input_val & 0x0000FF0000000000) >> 24) & 0x0000000000FF0000;
    temp |= ((input_val & 0x00FF000000000000) >> 40) & 0x000000000000FF00;
    temp |= ((input_val & 0xFF00000000000000) >> 56) & 0x00000000000000FF;
    return temp;
}

static void picus_send_packet(const char *str)
{
    //Note, the input str shall be the format of "xx xx xx xx xx"
    unsigned int i = 0;
    unsigned int bn = 0;
    unsigned char cmd[128] = {0};

    if ((NULL == str) || (strlen(str) < 2))
        return;
    while(str[i] != '\0') {
        char x = str[i];
        char y = str[i+1];
        if (str[i] == ' ') {
            i++;
            continue;
        }
        if (bn >= sizeof(cmd)) {
            DBGPRINT(SHOW, "packet is truncated and not sent!!!\n");
            return;
        }
        if (((x >= '0' && x <= '9') || (x >= 'A' && x <= 'F') || (x >= 'a' && x <= 'f')) &&
            ((y >= '0' && y <= '9') || (y >= 'A' && y <= 'F') || (y >= 'a' && y <= 'f'))) {
            cmd[bn] = (unsigned char)((C2N(x) << 4) + C2N(y));
            i += 2;
            bn++;
        } else {
            DBGPRINT(SHOW, "Invalid packet str with byte [%c%c]!!!\n", x, y);
            return;
        }
    }
    DBGPRINT(SHOW, "send packet: %s", str);
    bt_driver_tx_debug(cmd, bn);
}

static unsigned long btsnoop_timestamp(void)
{
#ifdef __ICCARM__
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    // Timestamp is in microseconds.
    g_timestamp = tv.tv_sec * 1000000ULL;
    g_timestamp += tv.tv_nsec / 1000;
    g_timestamp += BTSNOOP_EPOCH_DELTA;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    // Timestamp is in microseconds.
    g_timestamp = tv.tv_sec * 1000000ULL;
    g_timestamp += tv.tv_usec;
    g_timestamp += BTSNOOP_EPOCH_DELTA;
#endif /* __ICCARM__ */
    return g_timestamp;
}

static void fillheader(unsigned char *header, int headerlen,
        unsigned short int dump_file_seq_num)
{
    int copy_header_len = 0;
    unsigned int logversion = rtos_htobe32(LOG_VERSION);
    memset(header, 0, headerlen);
    memcpy(header, &logversion, sizeof(logversion));
    copy_header_len += 4;   /** 4 byte for logversion */
    copy_header_len += 4;   /** 4 byte for chip id, not implement yet */
    dump_file_seq_num = rtos_htobe16(dump_file_seq_num);
    memcpy(header + copy_header_len, &dump_file_seq_num, sizeof(dump_file_seq_num));
    copy_header_len += 2;   /** 2 byte for sequence number */
    copy_header_len += 6;   /** first hci log length(2), zero(4) */
    btsnoop_timestamp();
    g_timestamp = 0; //rtos_htobe64(g_timestamp);
    memcpy(header + copy_header_len, &g_timestamp, sizeof(g_timestamp));
}

#ifdef PICUS_LOG2FILE
static int create_new_log_file(int index)
{
    time_t local_timestamp;
    char timestamp_buffer[24];
    char dump_file_name[128] = {0};
    unsigned char header[24] = {0};
    unsigned char padding[8] = {0};
    int fd = 0;

    /* get current timestamp */
    time(&local_timestamp);
    strftime(timestamp_buffer, 24, "%Y%m%d%H%M%S", localtime(&local_timestamp));
    snprintf(dump_file_name, sizeof(dump_file_name), "%s/" DUMP_PICUS_NAME_PREFIX "%s_%d" DUMP_PICUS_NAME_EXT, g_log_path, timestamp_buffer, index);

    /* dump file for picus log */
    if ((fd = open(dump_file_name, O_WRONLY|O_CREAT, 0666)) < 0) {
        DBGPRINT(SHOW, "create log file %s fail, errno:%d", dump_file_name, errno);
        return -1;
    } else {
        DBGPRINT(SHOW, "log file %s is created, dumping...", dump_file_name);
        snprintf(g_current_log_full_name, sizeof(g_current_log_full_name), "%s", dump_file_name);
    }

    fillheader(header, sizeof(header), index);
    write(fd, header, sizeof(header));
    write(fd, padding, sizeof(padding));
    g_log_file_remain_size -= sizeof(header) + sizeof(padding);

    return fd;
}
#endif

static void remove_old_log_files(char *log_path, int all, int index)
{
#ifdef PICUS_LOG2FILE
    /* check already exist file under log_path */
    char temp_picus_filename[36] = {0};
    char picus_fullname[256] = {0};

    DIR *p_dir = opendir(log_path);
    if (p_dir != NULL) {
        struct dirent *p_file;
        while ((p_file = readdir(p_dir)) != NULL) {
            /* ignore . and .. directory */
            if (strncmp(p_file->d_name, "..", 2) == 0
                || strncmp(p_file->d_name, ".", 1) == 0) {
                continue;
            }
            memset(temp_picus_filename, 0, sizeof(temp_picus_filename));
            memset(picus_fullname, 0, sizeof(picus_fullname));
            if (strstr(p_file->d_name, DUMP_PICUS_NAME_EXT) != NULL) {
                if (all) {   //remove all old log files
                    snprintf(picus_fullname, sizeof(picus_fullname), "%s/%s", log_path, p_file->d_name);
                    if (remove(picus_fullname)) {
                        DBGPRINT(SHOW, "The old log:%s can't remove, errno:%d", p_file->d_name, errno);
                    } else {
                        DBGPRINT(SHOW, "The old log: %s is removed", p_file->d_name);
                    }
                } else {    //remove a specific log file
                    snprintf(temp_picus_filename, sizeof(temp_picus_filename), "_%d.picus", index);
                    if (strstr(p_file->d_name, temp_picus_filename) != NULL) {
                        snprintf(picus_fullname, sizeof(picus_fullname), "%s/%s", log_path, p_file->d_name);
                        if (remove(picus_fullname)) {
                            DBGPRINT(SHOW, "The old log: %s can't remove, errno:%d", p_file->d_name, errno);
                        } else {
                            DBGPRINT(SHOW, "The old log: %s is removed", p_file->d_name);
                        }
                    }
                }
            }
        }
        closedir(p_dir);
    } else {
        DBGPRINT(SHOW, "readdir %s fail, errno:%d", log_path, errno);
    }
#else
    return;
#endif
}

static int picus_rx_task_exit(void)
{
#ifdef PICUS_LOG2FILE
    if (g_current_fd > 0)
        close(g_current_fd);
    g_current_fd = 0;
#else
    picus_log_out(LOG_END, g_log_dest, PKT_FWLOG, NULL, 0);
#endif
    memset(g_current_log_full_name, 0, sizeof(g_current_log_full_name));

#ifndef PICUS_WITHOUT_BUFFER
    if (packet_buf.buffer) {
        vPortFree(packet_buf.buffer);
        packet_buf.buffer = NULL;
    }
#endif

    if (picus_rx_task_hdl)
        vTaskDelete(picus_rx_task_hdl);
    picus_rx_task_hdl = NULL;

#ifndef PICUS_WITHOUT_BUFFER
    vSemaphoreDelete(packet_queue_mtx);
    packet_queue_mtx = NULL;

#endif
    g_log_file_num = FWLOG_DEFAULT_NUM;
    g_log_file_size = FWLOG_DEFAULT_SIZE;
    g_log_file_remain_size = FWLOG_DEFAULT_SIZE;
#if (PICUS_CHIPID == 0x6631)
    g_log_via = FWLOG_VIA_EMI;
    g_log_dest = LOG_TO_FS;
#elif (PICUS_CHIPID == 0x7933)
    g_log_via = FWLOG_VIA_ACL;
    g_log_dest = LOG_TO_UART;
#endif
    g_current_fd = 0;
    g_current_idx = 0;
    g_timestamp = 0;
    g_driver_is_ready = 0;
    g_send_enable_fwlog = 1; //default enable fwlog

    return 0;
}

#if (PICUS_CHIPID == 0x6631)
static void picus_rx_event_cb(void)
{
    if (picus_rx_task_hdl)
        xTaskNotifyGive(picus_rx_task_hdl);
}
#endif

#ifndef PICUS_WITHOUT_BUFFER
static bool _is_packet_queue_empty(void)
{
    if (packet_buf.size == FWLOG_BUF_SIZE) {
        return pdTRUE;
    }
    return pdFALSE;
}

static unsigned int _get_packet_remain_size(void)
{
    return packet_buf.size;
}

static unsigned int _write_packet_data(unsigned char *data, unsigned int len)
{
    if ((len + packet_buf.write_idx) > FWLOG_BUF_SIZE) {
        return 0;
    } else {
        memcpy(packet_buf.buffer + packet_buf.write_idx, data, len);
        packet_buf.write_idx += len;
    }
    //DBGPRINT(SHOW, "Add %d bytes", len);
    packet_buf.size -= len;

    return len;
}
#endif
static int picus_log_out(unsigned char act, unsigned char dest, unsigned char pkt_type,
                        unsigned char *data, unsigned int len)
{
    unsigned int nWritten = 0;
#ifdef PICUS_LOG2FILE
    if (dest == LOG_TO_FS) {
        if (act == LOG_START) {
            DBGPRINT(SHOW, "fwlog dump to FS start");
            remove_old_log_files(g_log_path, 0, g_current_idx);
            g_current_fd = create_new_log_file(g_current_idx);
            if (g_current_fd <= 0) {
                DBGPRINT(SHOW, "fatal error: create new log file fail, errno:%d. picus exit", errno);
                picus_rx_task_exit();
            }
            close(g_current_fd);
        } else if (act == LOG_DUMPING) {
            if (NULL == data || 0 == len) {
                DBGPRINT(SHOW, "%s invalid data", __func__);
                return;
            }
            //Noted, this is for special file system(e.g. littlefs), which needs close() to flush data to flash. Therefore, it shall be opened again for writting.
            if ((g_current_fd = open(g_current_log_full_name, O_WRONLY, 0666)) <= 0) {
                DBGPRINT(SHOW, "fatal error: open log file %s fail, errno: %d, picus exit", g_current_log_full_name, errno);
                picus_rx_task_exit();
            }
            lseek(g_current_fd, 0, SEEK_END);
            nWritten = write(g_current_fd, data, len);
            if (nWritten != len) {
                DBGPRINT(SHOW, "write may fail, nRead(%d) != nWritten(%d)\n", len, nWritten);
            }
            g_log_file_remain_size -= nWritten;
            close(g_current_fd); //Noted, this is for special file system(e.g. littlefs), which needs close() to flush data to flash.
        } else if (act == LOG_END) {
            DBGPRINT(SHOW, "fwlog dump to FS end");
            close(g_current_fd);
            g_current_fd = 0;
        }
    }
#else   //LOG TO UART
    if (dest == LOG_TO_UART) {
        if (act == LOG_START) {
            if (pkt_type == PKT_FWLOG) {
                //send start packet  (data[0] == 0 && data[1] == 0 && data[2] != 0 && data[3] == 0)
                unsigned char header[PICUS_HDR_LEN + COM_PORT_FWLOG_PKT_HDR_LEN + COM_PORT_FWLOG_PKT_TAIL_LEN] = {0}; //24byte header + com port hdr + tail

                DBGPRINT(SHOW, "fwlog dump to UART start...");
                fwlog_pkt_hdr[3] = 0;
                fwlog_pkt_hdr[4] = PICUS_HDR_LEN + COM_PORT_FWLOG_PKT_TAIL_LEN;
                memcpy(header, fwlog_pkt_hdr, COM_PORT_FWLOG_PKT_HDR_LEN);
                fillheader(&header[COM_PORT_FWLOG_PKT_HDR_LEN], sizeof(header) - COM_PORT_FWLOG_PKT_HDR_LEN, 0);
                header[PICUS_HDR_LEN + COM_PORT_FWLOG_PKT_HDR_LEN] = fwlog_pkt_tail[0];
                header[PICUS_HDR_LEN + COM_PORT_FWLOG_PKT_HDR_LEN + 1] = fwlog_pkt_tail[1];
                picus_write_to_uart(header, sizeof(header));
                g_log_file_remain_size -= sizeof(header);
            } else if (pkt_type == PKT_COREDUMP) {
                DBGPRINT(SHOW, "coredump dump to UART start, no need start pattern");
            }
        } else if (act == LOG_DUMPING) {
#ifdef PICUS_WITHOUT_BUFFER
            if (packet_queue.len > 0) {
                nWritten = picus_write_to_uart(packet_queue.packet, packet_queue.len);
                if (nWritten != packet_queue.len) {
                    packet_queue.len = packet_queue.len - nWritten;
                    //DBGPRINT(SHOW, "%s write packet still fail, orginal log will loss. diff: %d w:%d", __func__, packet_queue.len, nWritten);
                    unsigned char *temp_pkt = packet_queue.packet;
                    packet_queue.packet = (unsigned char *)pvPortMalloc(packet_queue.len);
                    if (!packet_queue.packet) {
                        DBGPRINT(SHOW, "%s malloc packet buf fail", __func__);
                        vPortFree(temp_pkt);
                        packet_queue.len = 0;
                        return -1;
                    }
                    memcpy(packet_queue.packet, &temp_pkt[nWritten], packet_queue.len);
                    vPortFree(temp_pkt);
                    return -1;
                } else {
                    packet_queue.len = 0;
                    vPortFree(packet_queue.packet);
                }
                nWritten = 0;
            }
#endif
            if (NULL == data || 0 == len) {
                DBGPRINT(SHOW, "%s invalid data", __func__);
                return 0;
            }
            nWritten = picus_write_to_uart(data, len);
#ifdef PICUS_WITHOUT_BUFFER
            if (nWritten != len) {
                packet_queue.len = (len - nWritten) + COM_PORT_FWLOG_PKT_TAIL_LEN;
                packet_queue.packet = (unsigned char *)pvPortMalloc(packet_queue.len);
                if (!packet_queue.packet) {
                    packet_queue.len = 0;
                    DBGPRINT(SHOW, "%s malloc packet buf fail", __func__);
                    return -1;
                }
                memcpy(packet_queue.packet, &data[nWritten], packet_queue.len - COM_PORT_FWLOG_PKT_TAIL_LEN);
                packet_queue.packet[packet_queue.len - 2] = fwlog_pkt_tail[0];
                packet_queue.packet[packet_queue.len - 1] = fwlog_pkt_tail[1];
            }
#endif
            g_log_file_remain_size -= nWritten;
        } else if (act == LOG_END) {
            if (pkt_type == PKT_FWLOG) {
                DBGPRINT(SHOW, "fwlog dump to UART end");
#ifdef PICUS_WITHOUT_BUFFER
                if (packet_queue.len > 0) {
                    nWritten = picus_write_to_uart(packet_queue.packet, packet_queue.len);
                    if (nWritten != packet_queue.len) {
                        DBGPRINT(SHOW, "%s write packet still fail, last log loss.", __func__);
                    }
                    packet_queue.len = 0;
                    vPortFree(packet_queue.packet);
                }
#endif
                //send end pattern (data[0] == 0 && data[1] == 0 && data[2] != 0 && data[3] == 0)
                unsigned char tmp[COM_PORT_FWLOG_PKT_HDR_LEN + COM_PORT_FWLOG_END_SIZE + COM_PORT_FWLOG_PKT_TAIL_LEN] = {0, 0, 1, 0};
                fwlog_pkt_hdr[3] = 0;    //remove the 5byte magic header when calcute payload length
                fwlog_pkt_hdr[4] = COM_PORT_FWLOG_END_SIZE + COM_PORT_FWLOG_PKT_TAIL_LEN;
                memcpy(tmp, fwlog_pkt_hdr, COM_PORT_FWLOG_PKT_HDR_LEN);
                tmp[COM_PORT_FWLOG_PKT_HDR_LEN] = 0;
                tmp[COM_PORT_FWLOG_PKT_HDR_LEN + 1] = 0;
                tmp[COM_PORT_FWLOG_PKT_HDR_LEN + 2] = 1;
                tmp[COM_PORT_FWLOG_PKT_HDR_LEN + 3] = 0;

                tmp[COM_PORT_FWLOG_PKT_HDR_LEN + 4] = fwlog_pkt_tail[0];
                tmp[COM_PORT_FWLOG_PKT_HDR_LEN + 5] = fwlog_pkt_tail[1];
                picus_write_to_uart(tmp, sizeof(tmp));
            } else if (pkt_type == PKT_COREDUMP) {
                DBGPRINT(SHOW, "coredump dump to UART end, no need end pattern");
            }
        }
    }
#endif
    return nWritten;
}

int picus_debug_data_handle(unsigned char *buf, unsigned int len)
{
    uint32_t opcode = 0;

    if (buf[0] != HCI_PKT_EVT)
        return -1;

    if (!set_bperf)
        return -1;

    opcode = buf[4] | (buf[5] << 8);
    if (opcode != HCI_VND_READ_RSSI && opcode != HCI_VND_PER_STATISTIC)
        return -1;

    DBGPRINT(SHOW, "event return, picus debug cmd: 0x%04x", opcode);

    switch(opcode) {
    case HCI_VND_READ_RSSI:
    {
        int rssi = 0;
        rssi = (int)(buf[9]);
        if (rssi) {
            rssi = 256 - rssi;
            DBGPRINT(SHOW, "%sPacket header is RSSI%s, %shandle%s:0x%02X%02X, %sRSSI%s:-%d",
                    LIGHT_CYAN, NONECOLOR,
                    LIGHT_CYAN, NONECOLOR, buf[8], buf[7],
                    LIGHT_CYAN, NONECOLOR, rssi);
        }
        break;
    }
#ifndef MTK_BT_PICUS_CLI_LITE
    case HCI_VND_PER_STATISTIC:
    {
        uint8_t i = 0;
        uint8_t link_count = buf[15];
        uint8_t event_length = buf[2];
        uint8_t per_packet = event_length - PER_PREFIX_LEN - 3; // 3 bytes before data_len
        uint8_t per_group_len = 0;
        /* prevent divide link count is 0 */
        if (link_count)
            per_group_len = ((per_packet / link_count) == PER_GROUP_LEN) ? PER_GROUP_LEN : PER_LEGACY_GROUP_LEN;
        else
            per_group_len = PER_LEGACY_GROUP_LEN;
        DBGPRINT(SHOW, "%s", per_group_len == PER_GROUP_LEN ? "mtk per" : "legacy per");
        DBGPRINT(SHOW, "link_cnt = %d", link_count);
        DBGPRINT(SHOW, "bt cnt(t/r): %d/%d, ble cnt(t/r): %d/%d",
                buf[7] + (buf[8] << 8), buf[9] + (buf[10] << 8),
                buf[11] + (buf[12] << 8), buf[13] + (buf[14] << 8));
        if (link_count)
            DBGPRINT(SHOW, "=======================================================");
        for (i = 0; i < link_count; i++) {
            uint16_t per_idx = PER_PREFIX_LEN + per_group_len * i;
            uint16_t per_link_tx_count = 0;
            uint16_t per_link_tx_total_count = 0;
            uint16_t per_link_tx_error_count = 0;
            uint16_t per_link_tx_per = 0;
            uint16_t per_link_rx_count = 0;
            uint16_t per_link_rx_total_count = 0;
            uint16_t per_link_rx_error_count = 0;
            uint16_t per_link_rx_per = 0;
            uint16_t per_internal_tx_per = 0;
            uint16_t l2cap_avg = 0;
            uint16_t l2cap_max = 0;
            DBGPRINT(SHOW, "bt_addr = %02x:%02x:%02x:%02x:%02x:%02x",
                    buf[per_idx + 5], buf[per_idx + 4], buf[per_idx + 3],
                    buf[per_idx + 2], buf[per_idx + 1], buf[per_idx]);
            DBGPRINT(SHOW, "link_type = %s",
                    buf[per_idx + 6] == 0 ? "bt master" :
                    buf[per_idx + 6] == 1 ? "bt slave" :
                    buf[per_idx + 6] == 2 ? "ble master" :
                    buf[per_idx + 6] == 3 ? "ble slave" : "unknown");
            per_link_tx_count = buf[per_idx + 7] + (buf[per_idx + 8] << 8);
            per_link_tx_total_count = buf[per_idx + 9] + (buf[per_idx + 10] << 8);
            per_link_tx_error_count = buf[per_idx + 11] + (buf[per_idx + 12] << 8);
            if (per_link_tx_total_count)
                per_link_tx_per = ((per_link_tx_error_count * 100) / per_link_tx_total_count);
            DBGPRINT(SHOW, "tx cnt of link = %d", per_link_tx_count);
            DBGPRINT(SHOW, "tx per = %d(%d/%d)", per_link_tx_per, per_link_tx_error_count,
                    per_link_tx_total_count);

            per_link_rx_count = buf[per_idx + 13] + (buf[per_idx + 14] << 8);
            per_link_rx_total_count = buf[per_idx + 15] + (buf[per_idx + 16] << 8);
            per_link_rx_error_count = buf[per_idx + 17] + (buf[per_idx + 18] << 8);
            if (per_link_rx_total_count)
                per_link_rx_per = ((per_link_rx_error_count * 100) / per_link_rx_total_count);
            DBGPRINT(SHOW, "rx cnt of link = %d", per_link_rx_count);
            DBGPRINT(SHOW, "rx per = %d(%d/%d)", per_link_rx_per,
                    per_link_rx_error_count, per_link_rx_total_count);

            DBGPRINT(SHOW, "tx_pwr: lower bound idx = %d, last used idx = %d",
                    buf[per_idx + 19], buf[per_idx + 20]);
            DBGPRINT(SHOW, "last used tx_pwr = %ddbm", buf[per_idx + 21]);

            l2cap_avg =  buf[per_idx + 22] + (buf[per_idx + 23] << 8);
            DBGPRINT(SHOW, "avg_latency = %d, %.4fms", l2cap_avg, l2cap_avg * 0.3125);
            l2cap_max =  buf[per_idx + 24] + (buf[per_idx + 25] << 8);
            DBGPRINT(SHOW, "max_latency = %d, %.4fms", l2cap_max, l2cap_max * 0.3125);
            if (per_group_len == PER_GROUP_LEN) {
                per_internal_tx_per = (uint16_t)(buf[per_idx + 26]);
                /* Internal Tx PER range : 0~100 */
                if (per_internal_tx_per > 100)
                    DBGPRINT(SHOW, "invalid inner tx per = %d", per_internal_tx_per);
                else
                    DBGPRINT(SHOW, "inner tx per = %d %%", per_internal_tx_per);
            }
            DBGPRINT(SHOW, "=======================================================");
        }
        break;
    }
#endif /* #ifndef MTK_BT_PICUS_CLI_LITE */
    }
    return 0;
}


int picus_push_packet(unsigned char *buf, unsigned int len) //for UART mode
{
    if ((NULL == buf) || (len <= 0)) {
        DBGPRINT(SHOW, "%s invalid args", __func__);
        return -1;
    }
    picus_debug_data_handle(buf, len);
#ifdef PICUS_WITHOUT_BUFFER

    if (g_log_dest == LOG_TO_UART) {
        if (buf[0] == 0x02 && buf[1] == 0x6F && buf[2] == 0xFC) {   //coredump in ACL
            coredump_pkt_hdr[3] = (unsigned char)(((len -5 + 8) & 0xff00) >> 8);    //remove the 5byte magic header when calcute payload length
            coredump_pkt_hdr[4] = (unsigned char)((len -5 + 8) & 0xff);
            picus_log_out(LOG_DUMPING, g_log_dest, PKT_COREDUMP, coredump_pkt_hdr, COM_PORT_COREDUMP_PKT_HDR_LEN);
        } else if ((buf[0] == 0x02) && ((buf[1] == 0xFF && buf[2] == 0x05) || (buf[1] == 0xFE && buf[2] == 0x05))) { //fwlog in ACL
            if (g_log_out_port > HAL_UART_0 && hal_uart_get_available_send_space(g_log_out_port) < PICUS_BUFFER_THRESHOLD)
                return 0;
            fwlog_pkt_hdr[3] = (unsigned char)(((len - PICUS_ACL_HDR_LEN + COM_PORT_FWLOG_PKT_TAIL_LEN) & 0xff00) >> 8);      //remove the 5byte acl header when calcute payload length
            fwlog_pkt_hdr[4] = (unsigned char)((len - PICUS_ACL_HDR_LEN + COM_PORT_FWLOG_PKT_TAIL_LEN ) & 0xff);
            if (picus_log_out(LOG_DUMPING, g_log_dest, PKT_FWLOG, fwlog_pkt_hdr, COM_PORT_FWLOG_PKT_HDR_LEN) == -1)
                return 0;
        }
    }

    if (picus_log_out(LOG_DUMPING, g_log_dest, PKT_FWLOG, &buf[PICUS_ACL_HDR_LEN], len - PICUS_ACL_HDR_LEN) == (len - PICUS_ACL_HDR_LEN))
        picus_log_out(LOG_DUMPING, g_log_dest, PKT_FWLOG, fwlog_pkt_tail, COM_PORT_FWLOG_PKT_TAIL_LEN);

    if (g_log_file_remain_size <= 0 && picus_rx_task_hdl) {
        xTaskNotifyGive(picus_rx_task_hdl);
    }
    return 0;
#else
    unsigned char padding_len = 0;

    /*DBGPRINT(SHOW, "%s %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x", \
        __func__, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);*/

    if (xSemaphoreTake(packet_queue_mtx, portMAX_DELAY) == pdFALSE)
        DBGPRINT(TRACE, "%s: sema take failed", __func__);

#ifdef PICUS_LOG_COUNT_MONITOR
    static portTickType begin = 0;

    g_fwlog_total_size += len;
    if (g_fwlog_total_size == len)
        begin = xTaskGetTickCount();
    else if (g_fwlog_total_size > LOG_TOTAL_DATA_UPPER_BOUND) {
        DBGPRINT(SHOW, "Picus log %lu bytes / %d ms", g_fwlog_total_size, (xTaskGetTickCount() - begin) * portTICK_PERIOD_MS);
        g_fwlog_total_size = 0;
    }
#endif

    if (_get_packet_remain_size() < len + COM_PORT_FWLOG_PKT_TAIL_LEN){
        log_drop_total_data += len;
        if ((log_full_called_cnt & 0x000F) == 0) {
            DBGPRINT(SHOW, "picus log buf full, remain: %u, drop_cnt: %u, drop_data: %lu",
                _get_packet_remain_size(), log_full_called_cnt, log_drop_total_data);
        } else if (log_full_called_cnt == 0xFFFF) {
            DBGPRINT(SHOW, "picus log buf full, drop count to max 0xFFFF, drop_data: %lu", log_drop_total_data);
            log_drop_total_data = 0;
            log_full_called_cnt = 0;
        }
        log_full_called_cnt++;
        xSemaphoreGive(packet_queue_mtx);
        return -1;
    }
    if (log_full_called_cnt) {
        DBGPRINT(SHOW, "picus log buf full, drop_total_cnt: %u, drop_total_data: %lu",
            log_full_called_cnt, log_drop_total_data);
        log_full_called_cnt = 0;
        log_drop_total_data = 0;
    }

    if (buf[0] == 0x02 && buf[1] == 0x6F && buf[2] == 0xFC) {   //coredump in ACL
        if (g_log_dest == LOG_TO_UART) {
            coredump_pkt_hdr[3] = (unsigned char)(((len -5 + 8) & 0xff00) >> 8);    //remove the 5byte magic header when calcute payload length
            coredump_pkt_hdr[4] = (unsigned char)((len -5 + 8) & 0xff);
            _write_packet_data(coredump_pkt_hdr, COM_PORT_COREDUMP_PKT_HDR_LEN);
            _write_packet_data(&buf[5], len - 5);
        } else {
            _write_packet_data(&buf[5], len - 5);
        }
    } else if ((buf[0] == 0x02) && ((buf[1] == 0xFF && buf[2] == 0x05) || (buf[1] == 0xFE && buf[2] == 0x05))) { //fwlog in ACL
        if (g_log_dest == LOG_TO_UART) {
            fwlog_pkt_hdr[3] = (unsigned char)(((len - PICUS_ACL_HDR_LEN + padding_len + COM_PORT_FWLOG_PKT_TAIL_LEN) & 0xff00) >> 8);
            fwlog_pkt_hdr[4] = (unsigned char)((len - PICUS_ACL_HDR_LEN + padding_len + COM_PORT_FWLOG_PKT_TAIL_LEN) & 0xff);
            _write_packet_data(fwlog_pkt_hdr, COM_PORT_FWLOG_PKT_HDR_LEN);
            _write_packet_data(buf + PICUS_ACL_HDR_LEN, len - PICUS_ACL_HDR_LEN);
            _write_packet_data(fwlog_pkt_tail, COM_PORT_FWLOG_PKT_TAIL_LEN);
        } else {
            _write_packet_data(&buf[5], len - 5);
        }
    }

    //DBGPRINT(SHOW, "%d is pushed, len:0x%x", idx, packet_queue[idx].len);
    //bt_driver_dump_buffer("push picus log", packet_queue[idx].packet, packet_queue[idx].len, 1);
    xSemaphoreGive(packet_queue_mtx);

    if (picus_rx_task_hdl)
        xTaskNotifyGive(picus_rx_task_hdl);
    return 0;
#endif
}

bool picus_pop_packet(void)  //for UART mode
{
#ifdef PICUS_WITHOUT_BUFFER
    return pdFALSE;
#else
    int ret = 0;
    unsigned int write_len = 0;
    int retry = PICUS_POP_MAX_RETRY;
    if (xSemaphoreTake(packet_queue_mtx, portMAX_DELAY) == pdFALSE) {
        DBGPRINT(SHOW, "%s: sema take failed", __func__);
        return pdFALSE;
    }
    if (_is_packet_queue_empty() == pdTRUE) {
        xSemaphoreGive(packet_queue_mtx);
        return pdFALSE;
    }
    if (packet_buf.size < FWLOG_BUF_SIZE) {
        do {
            ret = picus_log_out(LOG_DUMPING, g_log_dest, PKT_FWLOG,/*it is useless for DUMPING action*/ \
                    packet_buf.buffer + write_len, FWLOG_BUF_SIZE - packet_buf.size - write_len);
            write_len += ret;
        } while (write_len != (FWLOG_BUF_SIZE - packet_buf.size) && retry--);
        if (retry < 0) {
            DBGPRINT(SHOW, "%s: Data Write FAIL (%d,%d)", __func__, write_len, FWLOG_BUF_SIZE - packet_buf.size);
        }

        packet_buf.size = FWLOG_BUF_SIZE;
        packet_buf.write_idx = 0;
        //DBGPRINT(SHOW, "popped %d bytes", write_len);
    }
    xSemaphoreGive(packet_queue_mtx);
    return pdTRUE;
#endif
}

void picus_coredump_out(unsigned char dump_type, unsigned char *dump_info, unsigned char *data, unsigned int len)
{
    unsigned char dump_pkt_hdr[COM_PORT_FWLOG_PKT_HDR_LEN] = {0xAB, 0xCD, 0x00, 0x00, 0x00};
    unsigned char dump_pkt_tail[COM_PORT_FWLOG_PKT_HDR_LEN + COM_PORT_FWLOG_END_SIZE + COM_PORT_FWLOG_PKT_TAIL_LEN] = {0, 0, 1, 0};
    unsigned char mem_dump_header[COM_PORT_FWLOG_PKT_HDR_LEN + 13 + COM_PORT_FWLOG_PKT_TAIL_LEN] = {0}; //12byte header + com port hdr + tail
    unsigned int write_len = 0;
    unsigned int packet_len = 0;

    if (dump_type != DUMP_CR)
        DBGPRINT(SHOW, "%s, type:%d len:%d", __func__, dump_type, len);
    if (len == 0)
        return;

    if (g_log_out_port > HAL_UART_0)
        bt_uart_init(g_log_out_port, uart1_baudrate);
    if (dump_type == DUMP_MEM) { // write header for mem type
        dump_pkt_hdr[2] = dump_type;
        dump_pkt_hdr[3] = 0;
        dump_pkt_hdr[4] = 13 + COM_PORT_FWLOG_PKT_TAIL_LEN;
        memcpy(mem_dump_header, dump_pkt_hdr, COM_PORT_FWLOG_PKT_HDR_LEN);
        memcpy(&mem_dump_header[COM_PORT_FWLOG_PKT_HDR_LEN], dump_info, 13);
        memcpy(&mem_dump_header[COM_PORT_FWLOG_PKT_HDR_LEN + 13], fwlog_pkt_tail, COM_PORT_FWLOG_PKT_TAIL_LEN);
        while(g_log_out_port > HAL_UART_0 && hal_uart_get_available_send_space(g_log_out_port) < sizeof(mem_dump_header))
            vTaskDelay(pdMS_TO_TICKS(20));
        picus_write_to_uart(mem_dump_header, sizeof(mem_dump_header));
    }

    while(write_len != len) {
        dump_pkt_hdr[2] = dump_type;
        if ((len - write_len) > PICUS_BUFFER_THRESHOLD)
            packet_len = PICUS_BUFFER_THRESHOLD;
        else
            packet_len = len - write_len;
        dump_pkt_hdr[3] = (unsigned char)(((packet_len + COM_PORT_FWLOG_PKT_TAIL_LEN) & 0xff00) >> 8);
        dump_pkt_hdr[4] = (unsigned char)((packet_len + COM_PORT_FWLOG_PKT_TAIL_LEN ) & 0xff);
        while (g_log_out_port > HAL_UART_0 && hal_uart_get_available_send_space(g_log_out_port) < PICUS_BUFFER_THRESHOLD * 2)
            vTaskDelay(pdMS_TO_TICKS(20));
        picus_write_to_uart(dump_pkt_hdr, COM_PORT_FWLOG_PKT_HDR_LEN);
        picus_write_to_uart(&data[write_len], packet_len);
        picus_write_to_uart(fwlog_pkt_tail, COM_PORT_FWLOG_PKT_TAIL_LEN);
        write_len += packet_len;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    dump_pkt_hdr[2] = dump_type;
    dump_pkt_hdr[3] = 0;
    dump_pkt_hdr[4] = COM_PORT_FWLOG_END_SIZE + COM_PORT_FWLOG_PKT_TAIL_LEN;
    memcpy(dump_pkt_tail, dump_pkt_hdr, COM_PORT_FWLOG_PKT_HDR_LEN);
    dump_pkt_tail[COM_PORT_FWLOG_PKT_HDR_LEN] = 0;
    dump_pkt_tail[COM_PORT_FWLOG_PKT_HDR_LEN + 1] = 0;
    dump_pkt_tail[COM_PORT_FWLOG_PKT_HDR_LEN + 2] = 1;
    dump_pkt_tail[COM_PORT_FWLOG_PKT_HDR_LEN + 3] = 0;
    dump_pkt_tail[COM_PORT_FWLOG_PKT_HDR_LEN + 4] = fwlog_pkt_tail[0];
    dump_pkt_tail[COM_PORT_FWLOG_PKT_HDR_LEN + 5] = fwlog_pkt_tail[1];

    if (dump_type == DUMP_CR && data != NULL) //only need to write tail when end of CR dump
        return;

    while (g_log_out_port > HAL_UART_0 && hal_uart_get_available_send_space(g_log_out_port) < sizeof(dump_pkt_tail))
        vTaskDelay(pdMS_TO_TICKS(20));
    picus_write_to_uart(dump_pkt_tail, sizeof(dump_pkt_tail));
    DBGPRINT(SHOW, "%s, end writing", __func__);
}

static void picus_config_fw_debug_info(int b_on)
{
    unsigned char cmd[] = {0x01, 0x5d, 0xfc, 0x04, 0x00, 0x00, 0x01, 0x00};//picus log via emi, level off
    char *log_via_str[] = {"Event", "EMI", "ACL", "Debug Uart"};

    if (g_log_via > FWLOG_VIA_UART || g_log_via < FWLOG_VIA_EVENT)
        return;
    DBGPRINT(SHOW, "set log control, via:%10s, level:%d, enable:%d",
        log_via_str[g_log_via], g_log_level, g_send_enable_fwlog);
    if (b_on && g_send_enable_fwlog) {
        cmd[6] = g_log_via;
        cmd[7] = g_log_level;
    }
    //bt_driver_tx_debug(cmd, sizeof(cmd));
    // config will called when power on and off, it will leads 3s timeout,so use driver_tx directly
    bt_driver_tx(cmd, sizeof(cmd));
}

static void bt_state_change_noitfy(int state)
{
    DBGPRINT(SHOW, "%s, %d -> %d, picus_task: 0x%p", __func__, g_driver_is_ready, state, picus_rx_task_hdl);
    if (!picus_rx_task_hdl)
        return;
    if (state) {    //BT open event
        if (!g_driver_is_ready) {
            g_driver_is_ready = state;
            if (picus_rx_task_hdl)
                xTaskNotifyGive(picus_rx_task_hdl);
            //Enable picus log here ???
            if (g_send_enable_fwlog) {
                picus_config_fw_debug_info(pdTRUE);
            }
        }
    } else {    //BT close event
        //Disable picus log here ???
        if (g_driver_is_ready) {
            g_driver_is_ready = state;
            // this case will lead EventwaitBit 3s timeout then lead set driver own fail when power off,
            // because it will hangup at bt_task when do power off, so not set Event bit for EventwaitBit till timeout
            // we should not open it again, for CR: AUTO00129817
            //picus_config_fw_debug_info(pdFALSE);
            //picus_send_packet("01 03 0c 00"); //send HCI reset for protential timing issue
        }
    }
}

static void picus_rx_task_main(void * arg)
{
#if (PICUS_CHIPID == 0x6631)
    int nRead = 0;
    int nWritten = 0;
    connsys_log_register_event_cb(CONNLOG_TYPE_BT, picus_rx_event_cb);
#elif (PICUS_CHIPID == 0x7933)
    btmtk_register_fwlog_recv_cb(picus_push_packet);
#endif

    bt_driver_register_state_change_cb(bt_state_change_noitfy, &g_driver_is_ready);
    DBGPRINT(SHOW, "%s is running, BT driver is %s", __func__, g_driver_is_ready ? "Ready" : "Not Ready");
    if (!g_driver_is_ready) {
        DBGPRINT(SHOW, "wait for driver ready...");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    } else {
        picus_config_fw_debug_info(pdTRUE);
    }

    //set default file path
    snprintf(g_log_path, FWLOG_PATH_LENGTH, "%s", FWLOG_DEFAULT_PATH);
    remove_old_log_files(g_log_path, 1, 0); //remove all log files in current path

    /* main loop, receive data from driver */
    do {
        if (g_log_file_remain_size == g_log_file_size) {
            picus_log_out(LOG_START, g_log_dest, PKT_FWLOG, NULL, 0);
        }
#if (PICUS_CHIPID == 0x6631)
        nRead = nWritten = 0;
        if (connsys_log_get_buf_size(CONNLOG_TYPE_BT) <= 0) {
            DBGPRINT(SHOW, "wait for log data...");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //wait for data from driver
        }
        //repeatly read all the log and write to log file
        do {
            nRead = connsys_log_read(CONNLOG_TYPE_BT, g_rx_buf, RX_BUF_SIZE);
            if (nRead > 0) {
                picus_log_out(LOG_DUMPING, g_log_dest, PKT_FWLOG, g_rx_buf, nRead);
            }
        } while (nRead > 0);
#elif (PICUS_CHIPID == 0x7933)
        if (pdFALSE == picus_pop_packet()) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    //wait for data
        }
#endif

        /* switch file name if file size is over file_size */
        if (g_log_file_remain_size <= 0) {
            picus_log_out(LOG_END, g_log_dest, PKT_FWLOG, NULL, 0);
            g_log_file_remain_size = g_log_file_size;
            if (g_log_file_num - 1 > g_current_idx) {
                g_current_idx++;
            } else {
                g_current_idx = 0;
            }
        }
    } while (1);
}

int picus_rx_task_create(void)
{
    DBGPRINT(SHOW, "%s", __func__);

    if (!picus_rx_task_hdl) {
#ifndef PICUS_WITHOUT_BUFFER
        if ((packet_queue_mtx = xSemaphoreCreateMutex()) == NULL) {
            DBGPRINT(SHOW, "%s create mutex fail.", __func__);
            return -1;
        }

        packet_buf.buffer = (unsigned char *)pvPortMalloc(FWLOG_BUF_SIZE * sizeof(unsigned char));
        if (!packet_buf.buffer) {
            DBGPRINT(SHOW, "%s malloc buffer fail.", __func__);
            return -1;
        }
        memset(packet_buf.buffer, 0, FWLOG_BUF_SIZE * sizeof(unsigned char));
        packet_buf.write_idx = 0;
        packet_buf.size = FWLOG_BUF_SIZE;
#endif
        if (pdPASS != xTaskCreate(picus_rx_task_main,
                                "picus_rx",
                                (1024*4)/sizeof(StackType_t),
                                NULL,
                                (UBaseType_t)PICUS_TASK_PRI,
                                &picus_rx_task_hdl)) {
            DBGPRINT(SHOW, "%s cannot create picus_rx_task.", __func__);
#ifndef PICUS_WITHOUT_BUFFER
            vSemaphoreDelete(packet_queue_mtx);
#endif
            return -1;
        }
    } else {
        DBGPRINT(SHOW, "picus_rx_task is already running");
    }

    return 0;
}


int picus_cmd_handler(int argc, char *argv[])
{
    int i = 0;
    int opt = 0;
    int ret = 0;
    bool is_baudrate_changed = false;
    bool is_dst_changed = false;

#ifdef __ICCARM__
    struct opt_data optdata;
    char *optarg;

    cli_getopt_init(&optdata);
#else
    optind = 0;
#endif /* __ICCARM__ */

    DBGPRINT(SHOW, "argc = %d, version: %s", argc, PICUS_VERSION);
    for (i = 0; i < argc; i++) {
        DBGPRINT(SHOW, "argv[%d] = %s", i, argv[i]);
    }

#ifdef __ICCARM__
    while ((opt = cli_getopt(argc, argv, "t:d:c:p:n:s:b:l:v:b:x:u:mfo", &optdata)) != -1) {
	    optarg = optdata.arg;
#else
    while ((opt = getopt(argc, argv, "t:d:c:p:n:s:b:l:v:b:x:u:mfo")) != -1) {
#endif /* __ICCARM__ */
        /* If your option didn't use argument, please don't use ':' into getopt */
        switch (opt) {
        case 'd':
            if (strcmp(optarg, "kill") == 0) {
                if (picus_rx_task_hdl) {
                    picus_config_fw_debug_info(pdFALSE); // disable picus log firstly
                    btmtk_enable_bperf(pdFALSE);         // disable bperf
                    btmtk_register_fwlog_recv_cb(NULL);
                    set_bperf = pdFALSE;
                    picus_rx_task_exit();
                }
                if (g_log_out_port > 0)
                    bt_uart_deinit(g_log_out_port);
                DBGPRINT(SHOW, "Delete picus rx task");
                goto done;
            } else if (strcmp(optarg, "trigger") == 0) {
                picus_config_fw_debug_info(pdFALSE); // disable picus log firstly
                DBGPRINT(SHOW, "Manual Trigger FW Assert.");
                bt_driver_trigger_controller_codedump();
                goto done;
            } else if (strcmp(optarg, "rssi") == 0) {
                DBGPRINT(SHOW, "Send read rssi command.");
                for (i = 0; i < 6; i++) {
                    char command[64] = {0};
                    memset(command, 0, sizeof(command));
                    int default_bredr_handle = 32;
                    /* Send Read RSSI command for bredr, handle is 0x0032 ~ 0x0037 */
                    ret = snprintf(command, sizeof(command), "01 61 FC 02 %d 00", default_bredr_handle + i);
                    if (ret < 0)
                        DBGPRINT(ERROR, "snprintf failed");
                    picus_send_packet(command);
                    vTaskDelay(10/portTICK_PERIOD_MS);
                    /* Send Read RSSI command for LE, handle is 0x0200 ~ 0x0205 */
                    ret = snprintf(command, sizeof(command), "01 61 FC 02 %02d 02", i);
                    if (ret < 0)
                        DBGPRINT(ERROR, "snprintf failed");
                    picus_send_packet(command);
                    vTaskDelay(10/portTICK_PERIOD_MS);
                }
                goto done;
            }
#ifndef MTK_BT_PICUS_CLI_LITE
            else if (strcmp(optarg, "per") == 0) {
                DBGPRINT(SHOW, "Send read per command.");
                picus_send_packet("01 11 FD 00");
                goto done;
            } else if (strcmp(optarg, "ble_scan_on") == 0) {
                DBGPRINT(SHOW, "Send ble scan disable command. (Duplicate_Filter:True)");
                picus_send_packet("01 0c 20 02 00 01");
                DBGPRINT(SHOW, "Send APCF delete command.");
                picus_send_packet("01 57 fd 03 01 01 00");
                picus_send_packet("01 57 fd 12 01 00 00 00 00 01 00 00 81 00 00 00 00 00 00 00 00 00");
                DBGPRINT(SHOW, "Send ble set scan parameter command. (5000ms/5000ms)");
                picus_send_packet("01 0b 20 07 01 40 1f 40 1f 01 00");
                DBGPRINT(SHOW, "Send ble scan enable command. (Duplicate_Filter:False)");
                picus_send_packet("01 0c 20 02 01 00");
                goto done;
            } else if (strcmp(optarg, "ble_scan_off") == 0) {
                DBGPRINT(SHOW, "Send ble scan disable command. (Duplicate_Filter:True)");
                picus_send_packet("01 0c 20 02 00 01");
                goto done;
            } else if (strcmp(optarg, "inquiry") == 0) {
                DBGPRINT(SHOW, "Send inquiry command.");
                picus_send_packet("01 01 04 05 33 8b 9e 0a 00");
                goto done;
            } else if (strcmp(optarg, "bperf") == 0) {
                DBGPRINT(SHOW, "Enable bperf.");
                btmtk_enable_bperf(pdTRUE);
                set_bperf = pdTRUE;
            } else if (strcmp(optarg, "hci") == 0) {
            } else if (strcmp(optarg, "afh") == 0) {
            } else if (strcmp(optarg, "en_rssi") == 0) {
            } else if (strcmp(optarg, "dis_rssi") == 0) {
            }
#endif /* #ifndef MTK_BT_PICUS_CLI_LITE */
            break;
#ifndef MTK_BT_PICUS_CLI_LITE
        case 'c':   /* send command */
            DBGPRINT(SHOW, "-c optarg = %s, not support for rtos", optarg);
            goto done;
        case 'p':   /* change path */
            DBGPRINT(SHOW, "-p optarg = %s, not support for rtos", optarg);
            break;
        case 'n':   /* change file number*/
            if (picus_rx_task_hdl) {
                DBGPRINT(SHOW, "change file number is not allowed while picus is running.");
                break;
            }
            g_log_file_num = (unsigned int)atoi(optarg);
            DBGPRINT(SHOW, "Change the number of file to %d.", g_log_file_num);
            break;
        case 's':   /* change file size*/
            if (picus_rx_task_hdl) {
                DBGPRINT(SHOW, "change file size is not allowed while picus is running.");
                break;
            }
            g_log_file_size = atoi(optarg);
            g_log_file_remain_size = g_log_file_size;
            DBGPRINT(SHOW, "Change the size of file to %d.", g_log_file_size);
            break;
        case 'l':   /* change buf from driver size*/
            DBGPRINT(SHOW, "-l optarg = %s, not support for rtos", optarg);
            break;
#endif  /* #ifndef MTK_BT_PICUS_CLI_LITE */
        case 'f':   /* log level */
            if (g_log_level == FWLOG_LVL_LOW_POWER) {
                DBGPRINT(ERROR, "log level has been set to %d ", g_log_level);
            } else
                g_log_level = FWLOG_LVL_FULL;
            DBGPRINT(SHOW, "Set fwlog level to %d.", g_log_level);
            goto done;
        case 'o':   /* lowpower mode */
            if (g_log_level == FWLOG_LVL_FULL) {
                DBGPRINT(ERROR, "log level has been set to %d ", g_log_level);
            } else {
                g_log_level = FWLOG_LVL_LOW_POWER;
                DBGPRINT(SHOW, "Set fwlog level to %d.", g_log_level);
            }
            goto done;
        case 'v':   /* log via */
        {
            int temp = atoi(optarg);
            char *log_via_str[] = {"Event", "EMI", "ACL", "Debug Uart"};
            if (picus_rx_task_hdl) {
                DBGPRINT(SHOW, "change log via is not allowed while picus is running.");
            }
            if (temp > FWLOG_VIA_UART || temp < FWLOG_VIA_EVENT) {
                DBGPRINT(SHOW, "Invalid log_via value %d.", temp);
            } else {
                g_log_via = temp;
                DBGPRINT(SHOW, "Change log_via to %s.", log_via_str[g_log_via]);
            }
            goto done;
        }
        case 'b':   /* log baudrate of uart */
        {
#if (PICUS_CHIPID == 0x7933)
            if (picus_rx_task_hdl) {
                DBGPRINT(SHOW, "change log baudrate is not allowed while picus is running.");
                goto done;
            }
            uart1_baudrate = atoi(optarg);
            is_baudrate_changed = true;
            DBGPRINT(SHOW, "Change baudtate as %d.", uart1_baudrate);
#endif
            break;
        }
        case 'x':   /* log to fs or uart */
        {
            if (picus_rx_task_hdl) {
                DBGPRINT(SHOW, "change log destination is not allowed while picus is running.");
                goto done;
            }
            unsigned char temp = atoi(optarg);
            g_log_dest = (temp != 0) ? LOG_TO_UART : LOG_TO_FS;
            is_dst_changed = true;
            DBGPRINT(SHOW, "Change log_via to %s.", (temp != 0) ? "UART" : "FS");
            break;
        }
        case 'u':   /* log to new uart port */
        {
#if (PICUS_CHIPID == 0x7933)
            unsigned char port = atoi(optarg);
            if (port >= HAL_UART_MAX) {
                DBGPRINT(SHOW, "port num should be less than %d", HAL_UART_MAX);
                goto done;
            }
            g_log_out_port = port;
            DBGPRINT(SHOW, "Change log to UART %d", g_log_out_port);
#endif
            break;
        }
        case 'm':   /* An debug API to ignore fwlog enabling command after task start*/
            if (!picus_rx_task_hdl) {
                DBGPRINT(SHOW, "Do not enable fwlog after start.");
                g_send_enable_fwlog = 0;
            }
            break;
        /* command Usage */
        case '?':
        default:
            usage();
            goto done;
        }
    }

    /* Picus expects baudrate changing only happens when log destination is uart. */
    if (is_baudrate_changed && (!is_dst_changed || g_log_dest != LOG_TO_UART))
        goto done;

    if (g_log_dest == LOG_TO_UART)
        bt_uart_init(g_log_out_port, uart1_baudrate);

    picus_rx_task_create();

done:
    return 0;
}


//---------------------------------------------------------------------------
