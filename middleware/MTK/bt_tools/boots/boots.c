/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * Copyright(C) 2018 MediaTek Inc
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

//- vim: set ts=4 sts=4 sw=4 et: --------------------------------------------
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "boots.h"
#include "boots_uart.h"
#include "boots_pkt.h"
#include "boots_stress.h"
#include "boots_srv.h"
#include "bt_driver.h"
#ifdef HAL_SLEEP_MANAGER_ENABLED
#include "hal_sleep_manager_internal.h"
#endif

#ifdef __ICCARM__
#include <getopt.h>
#include <time.h>
#else
#include <sys/time.h>
#endif /* __ICCARM__ */

//---------------------------------------------------------------------------
#define LOG_TAG "boots"

//---------------------------------------------------------------------------
enum {
    SLT_INDEX_DUT = 3,
    SLT_INDEX_GOLDEN = 4,
};

int boots_loop_timer;
rssi_set_s rssi_setting;
int g_relay_mode = 0;
static boots_if_s boots_if;
static uint8_t g_boots_log_lvl = BOOTS_MSG_LVL_DEFAULT;

static boots_buf_s buf = {
    .buf = {0},
    .ctrlif = "\n",
    .buf_len = 0,
};

boots_btif_s boots_btif[] = {
    {BOOTS_BTIF_STPBT,  "stpbt",    "/dev/stpbt"},
    {BOOTS_BTIF_HCI,    "hci",      NULL},
    {BOOTS_IF_NONE,     "",         NULL},      // Should leave bottom
};

#ifdef HAL_SLEEP_MANAGER_ENABLED
static bool boots_sleep_lock_now = false;
static uint8_t boots_sleep_lock_handle = 0;
static void boots_sleep_manager_lock (void)
{
    if (!boots_sleep_lock_handle) {
        boots_sleep_lock_handle = hal_sleep_manager_set_sleep_handle("boots");
        if (!boots_sleep_lock_handle) {
            BPRINT_E("%s: Can not get sleep handle!");
            return;
        }
    }

    hal_sleep_manager_lock_sleep (boots_sleep_lock_handle);
    boots_sleep_lock_now = true;
}

static void boots_sleep_manager_unlock (void)
{
    hal_sleep_manager_unlock_sleep (boots_sleep_lock_handle);
    boots_sleep_lock_now = false;
}
#endif

static uint32_t boots_get_timestamp(void)
{
#ifdef __ICCARM__
    struct timespec time_val;
    clock_gettime(CLOCK_REALTIME, &time_val);
    return ((time_val.tv_sec*1000000) + (time_val.tv_nsec/1000));
#else
    struct timeval time_val;
    gettimeofday(&time_val, NULL);
    return ((time_val.tv_sec*1000000) + (time_val.tv_usec));
#endif
}

//---------------------------------------------------------------------------
void boots_set_dbg_level(uint8_t lvl)
{
    if (lvl > BOOTS_MSG_LVL_DBG)
        g_boots_log_lvl = BOOTS_MSG_LVL_DBG;
    else
        g_boots_log_lvl = lvl;

    BPRINT("%s, lvl: %d", __func__, lvl);
}

uint8_t boots_get_dbg_level(void)
{
    return g_boots_log_lvl;
}

//---------------------------------------------------------------------------
static void boots_chk_if(int argc, char **argv)
{
    // Please follow the boots_if_e
    char *c_inf[] = {"None", "stpbt", "hci", "All", "Socket", "UART", "Ethernet",
                     "User", "Tester_UART", "buffer"};
    unsigned int i = 0;
    int sn = 0;

    if (argc < 0) {
        BPRINT_E("%s: argc < 0", __func__);
        return;
    }
#ifdef BOOTS_VERBOSE_MSG
    BPRINT_D("%s: argc = %d", __func__, argc);
    for (i = 0; i < argc; i++)
        BPRINT_D("argv[%d]:%s", i, argv[i]);
    i = 0;
#endif

    memset(&boots_if, 0, sizeof(boots_if));
    while (i < (unsigned int)argc) {
        // check interface for BT side
        if (!memcmp(argv[i], "tty", strlen("tty"))) {
            if (i + 1 < (unsigned int)argc) {
                boots_if.csif = BOOTS_CSIF_UART;
                if (memcmp(argv[i], "/dev/", strlen("/dev/"))) {
                    sn = snprintf(boots_if.cs, sizeof(boots_if.cs), "/dev/%s", argv[i]);
                    if (sn < 0 || sn > (int)sizeof(boots_if.cs)) {
                        BPRINT_E("tty: snprintf to boots_if.cs error");
                        break;
                    }
                }

                boots_if.cs_speed = strtol(argv[i + 1], NULL, 10);
                if (boots_if.cs_speed < 0) {
                    BPRINT_E("tty: cs_speed = %d, < 0", boots_if.cs_speed);
                    break;
                }
                BPRINT_D("cs_speed: %d", boots_if.cs_speed);
                i += 2;
            } else {
                BPRINT_E("Lack a parameter for %s", argv[i]);
                return;
            }
        } else if (!memcmp(argv[i], "eth", strlen("eth"))) {
            boots_if.csif = BOOTS_CSIF_ETH;
            if (strlen(argv[i + 1]) > IF_NAME_SIZE) {
                BPRINT_E("eth: argv param > 16, return");
                break;
            }
            memcpy(boots_if.cs, argv[i + 1], strlen(argv[i + 1]));
            i += 2;
        } else if (!memcmp(argv[i], "-relay", strlen("-relay"))) {
            sn = snprintf(boots_if.cs, sizeof(boots_if.cs), "%s", argv[i + 1]);
            if (sn < 0 || sn > (int)sizeof(boots_if.cs)) {
                BPRINT_E("-relay: snprintf to boots_if.cs error");
                break;
            }
#ifdef HAL_SLEEP_MANAGER_ENABLED
            boots_sleep_manager_lock();
#endif
            boots_if.cs_speed = strtol(argv[i + 2], NULL, 10);
            if (boots_if.cs_speed < 0) {
                BPRINT_E("relay: cs_speed = %d, < 0", boots_if.cs_speed);
                break;
            }
            BPRINT_D("cs_speed: %d", boots_if.cs_speed);
            i += 3;
            boots_if.csif = BOOTS_CSIF_UART;
            boots_if.clif = BOOTS_CLIF_USER;
            g_relay_mode = 1;
            break;
        } else if (!memcmp(argv[i], "-f", strlen("-f"))) {
            sn = snprintf(boots_if.cs, sizeof(boots_if.cs), "%s", argv[i + 1]);
            if (sn < 0 || sn > (int)sizeof(boots_if.cs)) {
                BPRINT_E("-f: snprintf to boots_if.cs error");
                break;
            }
            boots_if.cs_speed = strtol(argv[i + 2], NULL, 10);
            if (boots_if.cs_speed < 0) {
                BPRINT_E("f: cs_speed = %d, < 0", boots_if.cs_speed);
                break;
            }
            BPRINT_D("cs_speed: %d", boots_if.cs_speed);
            i += 3;
            boots_if.csif = BOOTS_CSIF_BUF;
            boots_if.clif = BOOTS_CLIF_USER;
            if (argc == 5 && !memcmp(argv[i], "fileid", strlen("fileid"))) {
                boots_if.file_id = (uint8_t)strtoul(argv[i + 1], NULL, 10);
                if (boots_if.file_id == 0xFF || errno == ERANGE) {
                    BPRINT_E("strtoul file_id error");
                    return;
                }
                i += 2;
            }
            BPRINT_D("select file id: %d", boots_if.file_id);
            break;

        } else if (*argv[i] == '-') {
            i++;
        } else {
            i++;
        }
    }

    if (boots_if.csif == BOOTS_IF_NONE) {
        boots_if.csif = BOOTS_CSIF_BUF;
        boots_if.clif = BOOTS_CLIF_USER;
        memcpy(boots_if.cs, "0", strlen("0") + 1);
    }

    BPRINT_D("cs: %s, cs_speed: %d", boots_if.cs, boots_if.cs_speed);
    BPRINT_D("%s(%d) <-> %s(%d) <-> server", c_inf[boots_if.clif],
            boots_if.clif, c_inf[boots_if.csif], boots_if.csif);
    // use UNUSED macro for debug warnig
    UNUSED(c_inf);
}

//---------------------------------------------------------------------------
#define USAGE_DETAILS      (1 << 0)
#define USAGE_DETAILS_MTK  (1 << 1)
static void boots_cmd_usage(uint8_t detail)
{
    unsigned int i = 0;
    extern boots_cmds_s commands[];

    BPRINT("Boots - MTK Bluetooth Test Suite ver:%s", BOOTS_VERSION);
    BPRINT("Modular Commands:");
    BPRINT("detail %d", detail);
    for (i = 0; commands[i].cmd != NULL; i++) {
        if (commands[i].hidden == false) {
            BPRINT("  %s - %s", commands[i].cmd, commands[i].comment);
        } else if (detail & USAGE_DETAILS_MTK) {
            BPRINT("  %s - %s", commands[i].cmd, commands[i].comment);
        }

        if ((detail & USAGE_DETAILS) && (NULL != commands[i].details)) {
            if (commands[i].hidden == false) {
                BPRINT("%s", commands[i].details);
            } else if (detail & USAGE_DETAILS_MTK) {
                BPRINT("%s", commands[i].details);
            }
        }
    }
    BPRINT("For specfic command details use: ./boots -c <CMD> -h\n");
}

//---------------------------------------------------------------------------
static void boots_help(void)
{
    BPRINT("Boots - MTK Bluetooth Test Suite ver:%s", BOOTS_VERSION);
    BPRINT("Usage:");
    BPRINT("    boots [BT Interface] <InputMethod/RelayMode> [parameters]");
    BPRINT("    (Unchangeable order)");
    BPRINT("BT Interface:");
    BPRINT("InputMethod: [parameters]");
    BPRINT("\t-l        log level, ex: bt btpriv boots -l 1 (1:err, 2:warn, 3:info, 4:debug");
    BPRINT("\t-r        Raw data, ex: btpriv boots -r CMD 03 0C 00");
    BPRINT("\t-c        Command, for more command information on the usage of everyone command use:");
    BPRINT("\t              bt btpriv boots -c [detail]");
    BPRINT("\t-relay    bt btpriv boots -relay [start/stop] [port number] [baudrate]");
    BPRINT("\t              ex: bt btpriv boots -relay start 2 115200");
    BPRINT("\t              ex: bt btpriv boots -relay stop 2");
    BPRINT("Others:\n");
    BPRINT("\t-d        Stop service if any background service\n");
}

//---------------------------------------------------------------------------
static ssize_t boots_read(void *buf, size_t buf_size, int inf)
{
    ssize_t ret = 0;

    if (!buf || !buf_size || !inf) {
        BPRINT_E("%s: Invalid argument(buf: %p, buf size: %d, inf: %d)",
                __func__, buf, (int)buf_size, inf);
        return -1;
    }

    if (inf == BOOTS_CSIF_BUF) {

    } else if (inf == BOOTS_CSIF_UART || inf == BOOTS_CLIF_UART) {

    } else {
        BPRINT_E("%s: Incorrect interface(%d)", __func__, inf);
    }

    return ret;
}

//---------------------------------------------------------------------------
static ssize_t boots_write(int fd, const void *buf, size_t len, int inf)
{
    ssize_t ret = 0;

    if (!buf || !len || !inf) {
        BPRINT_E("%s: Invalid argument(buf: %p, len: %d, inf: %d)", __func__, buf, (int)len, inf);
        return -1;

    } else if (fd < 0) {
        BPRINT_E("%s: Bad file descriptor(%d)", __func__, fd);
        return -1;
    }

    BPRINT_D("%s: len %d, fd = %d, inf = %d", __func__, len, fd, inf);
    SHOW_RAW(len, buf);

    // write locally
    if (inf == BOOTS_CSIF_BUF) {
        if (boots_srv_get_server_tx_handle() == NULL) {
            BPRINT_E("%s: Why server_tx_handle == NULL??", __func__);
            return -1;
        }

        if (len) {
            boots_srv_set_cmd_buffer_content(buf, len);
            xTaskNotifyGive(boots_srv_get_server_tx_handle());
            ret = len;
        }

    // write to UART
    } else if (inf == BOOTS_CSIF_UART || inf == BOOTS_CLIF_UART) {
        ret = boots_uart_write_freertos(atoi(boots_if.cs), buf, len);

    } else {
        BPRINT_E("%s: Incorrect interface(%d)", __func__, inf);
    }

    return ret;
}


//---------------------------------------------------------------------------
void boots_set_btif(char *ifchar, int hciidx)
{
    unsigned char i = 0;
    int icmp = 0;
    static unsigned char btif_cnt = 0;
    char c_name[IF_NAME_SIZE];
    char *c_inf[] = {"None", "stpbt", "hci", "All", "Socket", "UART", "Ethernet",
                     "User", "Tester_UART"}; // Please follow the boots_if_e

    BPRINT_D("%s: %s, %d", __func__, ifchar, hciidx);
    for (i = 0; i < BOOTS_BTIF_ALL; i++) {
        //BPRINT_D("%s %s", boots_btif[i].n, ifchar);
        icmp = strncmp(boots_btif[i].n, ifchar, strlen(boots_btif[i].n));
        if ((icmp == 0) && (btif_cnt < MAX_CHIP_NO) && boots_btif[i].n) {
            boots_if.btif[btif_cnt] = boots_btif[i].inf;
            boots_if.btfd[btif_cnt] = hciidx;
            strncpy(boots_if.bt[btif_cnt], ifchar, IF_NAME_SIZE-1);
            boots_if.bt[btif_cnt][IF_NAME_SIZE-1] = '\0';
            btif_cnt++;
        }
    }

    i = 0;
    //BPRINT_D("---------------------------------------");
    while (i < MAX_CHIP_NO && boots_if.btif[i] > 0) {
        memcpy(c_name, boots_if.bt[i], IF_NAME_SIZE - 1);
        c_name[IF_NAME_SIZE-1] = '\0';
        BPRINT_I("[%d]btif:%d(%9s), btfdidx:%d, %5s", i, boots_if.btif[i],
                  c_inf[boots_if.btif[i]], boots_if.btfd[i], c_name);
        i++;
    }
    //BPRINT_D("---------------------------------------");
    // use UNUSED macro for debug warnig
    UNUSED(c_inf);
}

//---------------------------------------------------------------------------
int boots_main(int argc, char *argv[])
{
    BPRINT_I("boots main - Ver. %s", BOOTS_VERSION);
    BPRINT_I("boots main - log level = %d", boots_get_dbg_level());

    static pkt_list_s *pkt = NULL;
    pkt_list_s *trx_head = NULL;
    pkt_list_s *trx_prev = NULL;
    static script_set_s sfile = {NULL, 0, 0, 0, 0};

    size_t len = 0;
    int cont = 0;
    int type = 0;
    int q_cnt = 0;
    bool multi_chip = false;
    bool start_TRX = false;
    bool script_end = false;
    boots_script_sts_e script_state = BOOTS_SCRIPT_NONE;

    uint32_t time_start = 0;
    uint32_t time_end = 0;
    uint32_t diff_time;
    int atoi_val = 0;

    bool notify_ret = 0;

#ifdef __ICCARM__
    struct opt_data opt;
    int32_t optind;
    int32_t opterr;
    int32_t optopt;

    cli_getopt_init(&opt);
#else
    optind = 0;
#endif /* __ICCARM__ */


    // reset global variable
    g_relay_mode = 0;

    if (argc == 2 && !memcmp("-c", argv[1], strlen("-c"))) {
        // Print commands
        boots_cmd_usage(0);
        return 0;

    } else if (argc == 3 && !memcmp("-c", argv[1], strlen("-c"))) {
        if (!memcmp("detail", argv[2], strlen("detail"))) {
            boots_cmd_usage(1);
            return 0;

        } else if (!memcmp("mtk", argv[2], strlen("mtk"))) {
            boots_cmd_usage(3);
            return 0;

        } else {
            // do nothing for command without any parameters.
        }

    } else if (argc == 3 && !memcmp("-l", argv[1], strlen("-l"))) {
        boots_set_dbg_level((uint8_t)atoi(argv[2]));
        return 0;

    } else if (argc == 2 && !memcmp("-d", argv[1], strlen("-d"))) {
        kill_boots_srv();
        boots_uart_deinit_freertos();
#ifdef HAL_SLEEP_MANAGER_ENABLED
        if (boots_sleep_lock_now == true)
            boots_sleep_manager_unlock();
#endif
        return 0;

    } else if (argc < 3) {
        boots_help();
        return 0;
    }

    // Confirm interface between client/server, upper layer
    boots_chk_if(argc - 1, &argv[1]);
    BPRINT_D("boots_if.csif %d", boots_if.csif);

    /** Communication Interface with boots_srv */
    if (boots_if.csif == BOOTS_CSIF_BUF) {
        BPRINT_D("Boots start.");
        create_boots_srv();
        boots_client_semaphore_create();

    } else if (boots_if.csif == BOOTS_CSIF_UART) {
        BPRINT_D("Boots start.");
        BPRINT_I("get uart port %s, baudrate %d", boots_if.cs, boots_if.cs_speed);
        create_boots_srv();
        boots_client_semaphore_create();

        if (boots_uart_init_freertos(atoi(boots_if.cs), boots_if.cs_speed)) {
            BPRINT_E("%s: uart init failed", __func__);
            goto exit;
        }
        goto exit;

    } else {
        BPRINT_E("Unknown communication interface");
        goto exit;
    }

    /** Input Interface for boots */
    if (boots_if.clif == BOOTS_CLIF_USER) {
#ifdef __ICCARM__
        if ((type = cli_getopt(argc, argv, "frcok", &opt)) != -1) {
            opterr = opt.err;
            optopt = opt.opt;
            optind = opt.idx;
#else
        if ((type = getopt(argc, argv, "frcok")) != -1) {
#endif
            BPRINT_D("optopt: %d, opterr: %d, optind: %d, type: %c", optopt, opterr, optind, type);
            switch (type) {
            case 'f':   /** SLT script */
                sfile.script = boots_script_open(BOOTS_SCRIPT_SLT, boots_if.file_id);
                if (!sfile.script) {
                    BPRINT_E("open SLT script failed");
                    goto exit;
                }

                BPRINT_D("%s: port = %d, speed = %d", __func__, atoi(boots_if.cs), boots_if.cs_speed);
                if (boots_uart_init_freertos(atoi(boots_if.cs), boots_if.cs_speed)) {
                    BPRINT_E("%s: uart init failed", __func__);
                    goto exit;
                }
                script_state = BOOTS_SCRIPT_STARTED;
                break;

            case 'r':   /** raw data */
                pkt = boots_raw_cmd_handler(argv + optind, argc - optind);
                break;

            case 'c':   /** command set */
                pkt = boots_cmd_set_handler(argv + optind, argc - optind);
                break;

            case 'k':   /** loopback script */
                if (memcmp("local", argv[optind], strlen("local")) != 0) {
                    BPRINT_E("only local loopback is supported");
                    goto exit;
                }
                sfile.script = boots_script_open(BOOTS_SCRIPT_LOOPBACK, boots_if.file_id);
                if (!sfile.script) {
                    BPRINT_E("open ACL loopback script failed");
                    goto exit;
                }
                script_state = BOOTS_SCRIPT_STARTED;
                break;

            default:
                BPRINT_W("Unknown type: %c", type);
                break;
            };
            if (!pkt && !sfile.script) goto exit;
        }

    } else {
        BPRINT_E("Unknown communication interface");
        goto exit;
    }

    /** Process handler */
    do {
        int ret = -1, i = 0;
        uint8_t xfer_idx = 0;
        uint32_t mdelay = 0;

        /* read script line by line before start TRX */
        /* since different data type when doing setting */
        if (sfile.script && !start_TRX) {
            if (boots_pkt_list_amount(pkt) != 0) {
                BPRINT_W("pkt list amount is not ZERO: %lu", (long unsigned int)boots_pkt_list_amount(pkt));
                //boots_pkt_list_destroy(pkt);    // force destroy list
            }
            pkt = boots_script_get(&sfile.script);
            if (NULL == pkt) {
                BPRINT_W("boots_script_get return NULL");
                break;
            }
            if (pkt->s_type == SCRIPT_TX || pkt->s_type == SCRIPT_RX || pkt->s_type == SCRIPT_RSSI
                            || pkt->s_type == SCRIPT_PROC || pkt->s_type == SCRIPT_WAITRX
                            || pkt->s_type == SCRIPT_WAIT || pkt->s_type == SCRIPT_USBALT
                            || pkt->s_type == SCRIPT_LOOP || pkt->s_type == SCRIPT_LOOPEND) {
                start_TRX = true;
                q_cnt++;
                if (trx_prev == NULL) {
                    trx_head = pkt;
                    trx_prev = pkt;
                }
                /* in case that first line is LOOP */
                if (pkt->s_type == SCRIPT_LOOP) {
                    sfile.loop = pkt->u_cnt.loop;
                    if (sfile.script)
                        sfile.loop_pos = sfile.script;
                }
            } else if (pkt == NULL)
                break;  // Could end of file
        }

        /* start TRX */
        if (sfile.script && start_TRX && !script_end && q_cnt < SCRIPT_LIST_MAX) {
            BPRINT_D("Incoming new script data");
            trx_head = pkt;
            if (trx_prev != pkt) {
                trx_prev = pkt;
                while (trx_prev->next != pkt) {
                    trx_prev = trx_prev->next;
                }
            }
            do {
                pkt = boots_script_get(&sfile.script);
                if (pkt == NULL)
                    break;  // Could end of file

                if (pkt->s_type == SCRIPT_TX || pkt->s_type == SCRIPT_RX || pkt->s_type == SCRIPT_RSSI
                        || pkt->s_type == SCRIPT_PROC || pkt->s_type == SCRIPT_WAITRX
                        || pkt->s_type == SCRIPT_WAIT || pkt->s_type == SCRIPT_USBALT
                        || pkt->s_type == SCRIPT_LOOP || pkt->s_type == SCRIPT_LOOPEND) {
                    q_cnt++;
                    trx_prev->next = pkt;
                    trx_prev = pkt;
                    pkt->next = trx_head;

                } else if (pkt->s_type == SCRIPT_END) {
                    script_end = true;
                    trx_prev->next = pkt;
                    trx_prev = pkt;
                    pkt->next = trx_head;
                }

                if (pkt->s_type == SCRIPT_LOOP) {
                    sfile.loop = pkt->u_cnt.loop;
                    if (sfile.script)
                        sfile.loop_pos = sfile.script;
                } else if (pkt->s_type == SCRIPT_LOOPEND) {
                    if (!sfile.loop) {
                        sfile.loop_pos = 0;
                    } else {
                        if ((--sfile.loop) && sfile.script)
                            sfile.script = sfile.loop_pos;
                    }
                }

            } while (q_cnt < SCRIPT_LIST_MAX && !script_end);
            pkt = trx_head;
        }

        // default timeout
        if (sfile.timo) {
            mdelay = sfile.timo;
        } else {
            mdelay = 3000;
        }

        if (pkt) {
            BPRINT_D("s_type: 0x%02X, p_type: 0x%02X, xfer_idx:%d", pkt->s_type, pkt->p_type, pkt->xfer_idx);
            xfer_idx = pkt->xfer_idx;
            switch (pkt->s_type) {
            case SCRIPT_NONE:       // modular commands
            case SCRIPT_CMD:        // hci_cmd script
            case SCRIPT_STRESS:     // stress test script
            case SCRIPT_LOOPBACK:   // loopback test script
            case SCRIPT_LPTIMER:    // loopback test script with timer
                if (pkt->s_type == SCRIPT_NONE)
                    BPRINT_D("SCRIPT_NONE");
                boots_pkt_node_pop(&pkt, buf.buf, &len);
                buf.buf_len = len;
                break;
            case SCRIPT_TX:         // combo tool script
                boots_pkt_node_pop(&pkt, buf.buf, &len);
                buf.buf_len = len;

                if (xfer_idx == SLT_INDEX_DUT) {
                    BPRINT_D("script cmd sends to DUT");
                    boots_if.csif = BOOTS_CSIF_BUF;
                } else if (xfer_idx == SLT_INDEX_GOLDEN){
                    BPRINT_D("script cmd sends to golden");
                    boots_if.csif = BOOTS_CSIF_UART;
                } else if (xfer_idx == 0) {
                    BPRINT_I("script cmd sends locally");
                    boots_if.csif = BOOTS_CSIF_BUF;
                } else {
                    BPRINT_W("xfer_idx = 0x%x is out of expected", xfer_idx);
                    goto exit;
                }
                if (len) {
                    if (multi_chip == true) {
                        for (i = 0; i < MAX_CHIP_NO; i++) {
                            if (xfer_idx == boots_if.btfd[i])
                                strncpy(buf.ctrlif, &boots_if.bt[i][0], strlen(boots_if.bt[i]));
                        }
                    }
                    // Send command, if input by user or script
                    atoi_val = atoi(boots_if.cs);
                    if (atoi_val < 0) {
                        BPRINT_W("SCRIPT_TX: atoi for boots_if.cs error");
                        continue;
                    }
                    ret = boots_write(atoi_val, (void *)buf.buf, len, boots_if.csif);
                    boots_pkt_handler(&buf, NULL, 0, xfer_idx);
                    memset((void *)&buf, 0, sizeof(buf));
                    len = 0;
                }
                q_cnt--;
                if (ret <= 0)
                    break;
                continue;
            case SCRIPT_RX:
                if (boots_srv_rx_available() == false && buf.buf_len == 0) {
                    BPRINT_D("wait rx buffer available");
                    notify_ret = boots_client_semaphore_take(pdMS_TO_TICKS(mdelay));
                }
                if (buf.buf_len == 0) {
                    boots_srv_get_event_buffer_content(buf.buf, &len);
                    buf.buf_len = len;
                } else {
                    BPRINT_D("buf.buf_len is %d", buf.buf_len);
                }

                for (i = 0; i < MAX_CHIP_NO ; i++) {
                    if (strncmp(boots_if.bt[i], buf.ctrlif, sizeof(buf.ctrlif)) == 0) {
                        xfer_idx = boots_if.btfd[i];
                        break;
                    }
                }

                if (buf.buf_len > 0) {
                    // pkt handler will pop pkt node
                    cont = boots_pkt_handler(&buf, pkt ? &pkt : NULL, 1, xfer_idx);
                    BPRINT_D("RX cont: %d, len: %d", cont, (int)buf.buf_len);
                    if (cont == 0 && sfile.script) {
                        // cont 0 means to get incorrect content, stop
                        boots_script_close();
                        sfile.script = NULL;
                        script_state = BOOTS_SCRIPT_FAIL;
                        len = 0;
                        goto exit;
                    }
                    if (buf.buf_len <= 0) {
                        memset((void *)&buf, 0, sizeof(buf));
                    }
                } else if (notify_ret == pdFALSE){
                    // If Rx timeout, should end script process.
                    BPRINT_I("Get RX timeout(%ldms)!", mdelay);
                    boots_script_close();
                    sfile.script = NULL;
                    script_state = BOOTS_SCRIPT_FAIL;
                    len = 0;
                    goto exit;
                }
                q_cnt--;
                continue;
            case SCRIPT_WAITRX:
                do {
                    if (boots_srv_rx_available() == false) {
                        BPRINT_D("wait event buffer available");
                        boots_client_semaphore_take(pdMS_TO_TICKS(mdelay));
                    }
                    if (buf.buf_len == 0) {
                        boots_srv_get_event_buffer_content(buf.buf, &len);
                        buf.buf_len = len;
                    } else
                        BPRINT_D("(WARITRX)buf.buf_len is %d", buf.buf_len);

                    for (i = 0; i < MAX_CHIP_NO ; i++) {
                        if (strcmp(boots_if.bt[i], buf.ctrlif) == 0) {
                            xfer_idx = boots_if.btfd[i];
                            break;
                        }
                    }
                    if (len > 0) {
                        cont = boots_pkt_handler(&buf, pkt ? &pkt : NULL, 1, xfer_idx);
                        BPRINT_D("WAITRX cont: %d, len: %d", cont, (int)buf.buf_len);
                        if (cont == 0 && sfile.script) {
                            // cont 0 means to get incorrect content, stop
                            boots_script_close();
                            sfile.script = NULL;
                            script_state = BOOTS_SCRIPT_FAIL;
                            len = 0;
                            goto exit;
                        }
                        if (buf.buf_len <= 0)
                            memset((void *)&buf, 0, sizeof(buf));

                        len = buf.buf_len;
                    }
                } while (cont == 2);
                q_cnt--;
                continue;
            case SCRIPT_TITLE:
            #ifdef BOOTS_VERBOSE_MSG
                BPRINT_I("%s TITLE: %s", sfile.fn, pkt->u_cnt.msg); // Just print msg
            #else
                BPRINT_I("TITLE: %s", pkt->u_cnt.msg); // Just print msg
            #endif
                boots_pkt_node_pop(&pkt, NULL, NULL);
                continue;
            case SCRIPT_PROC:
            #ifdef BOOTS_VERBOSE_MSG
                BPRINT_I(BLUE"%s PROC: %s"NONE, sfile.fn, pkt->u_cnt.msg);    // Just print msg
            #else
                BPRINT_I(BLUE"PROC: %s"NONE, pkt->u_cnt.msg);  // Just print msg
            #endif
                boots_pkt_node_pop(&pkt, NULL, NULL);
                q_cnt--;
                continue;
            case SCRIPT_TIMEOUT:
                if (pkt->u_cnt.timo) {
                    sfile.timo = pkt->u_cnt.timo;
                }
                boots_pkt_node_pop(&pkt, NULL, NULL);
                continue;
            case SCRIPT_WAIT:
                BPRINT_I(BLUE"WAIT: %d ms"NONE, pkt->u_cnt.wait);  // Just print wait time
                if (pkt->u_cnt.wait) {
                    vTaskDelay(pdMS_TO_TICKS(pkt->u_cnt.wait));
                }
                boots_pkt_node_pop(&pkt, NULL, NULL);
                q_cnt--;
                continue;
            case SCRIPT_USBALT:
                if (pkt) {
                    char alt[8];
                    char *usbalt[5] = {"usbalt", "-a", alt};
                    pkt_list_s *tmp = NULL;
                    int n = 0;

                    BPRINT_I("SCRIPT_USBALT: %d", pkt->u_cnt.usbalt);
                    n = sprintf(alt, "%d", pkt->u_cnt.usbalt);
                    if (n < 0) {
                        BPRINT_E("%s, SCRIPT_USBALT, sprintf error", __func__);
                        break;
                    }
                    tmp = boots_cmd_set_handler(usbalt, 3);
                    boots_pkt_node_pop(&pkt, NULL, NULL);
                    boots_pkt_node_pop(&tmp, buf.buf, &len);
                    buf.buf_len = len;
                }
                if (len) {
                    if (multi_chip == true) {
                        for (i = 0; i < MAX_CHIP_NO; i++) {
                            if (xfer_idx == boots_if.btfd[i])
                                strncpy(buf.ctrlif, &boots_if.bt[i][0], strlen(boots_if.bt[i]));
                        }
                        len = sizeof(buf);
                    }
                }
                q_cnt--;
                break;
            case SCRIPT_LOOP:
                sfile.loop = pkt->u_cnt.loop;
                sfile.loop_pos = sfile.script;
                boots_pkt_node_pop(&pkt, NULL, NULL);
                BPRINT_I(BLUE"Looping, %p, %p"NONE, (void *)sfile.loop, (void *)sfile.loop_pos);
                q_cnt--;
                continue;
            case SCRIPT_LOOPEND:
                if (!sfile.loop) {
                    sfile.loop_pos = 0;
                    BPRINT_D("Loop End");
                } else {
                    if (--sfile.loop)
                        sfile.script = sfile.loop_pos;
                    BPRINT_D("Loop End, remain %d times",sfile.loop);
                }
                boots_pkt_node_pop(&pkt, NULL, NULL);
                q_cnt--;
                continue;
            case SCRIPT_RSSI: // Background RSSI scan
                if (pkt->u_cnt.rssi_s->stop)
                    boots_pkt_cleanup_report_rssi(1);
                else {
                    boots_pkt_cleanup_report_rssi(0);
                    memcpy(&rssi_setting, pkt->u_cnt.rssi_s, sizeof(rssi_set_s));
                }
                buf.buf[0] = SCRIPT_RSSI;
                memcpy(&buf.buf[1], pkt->u_cnt.rssi_s, sizeof(rssi_set_s));
                len = sizeof(rssi_set_s) + 1;
                atoi_val = atoi(boots_if.cs);
                if (atoi_val < 0) {
                    BPRINT_W("SCRIPT_RSSI: atoi for boots_if.cs error");
                    continue;
                }
                boots_write(atoi_val, (void *)buf.buf, len, boots_if.csif);
                memset((void *)&buf, 0, sizeof(buf));
                len = 0;
                boots_pkt_node_pop(&pkt, NULL, NULL);
                q_cnt--;
                continue;
            case SCRIPT_HCI: {
                char *p = strstr(pkt->u_cnt.hci, ":") + 1;
                char mc[IF_NAME_SIZE];
                atoi_val = atoi(strtok(pkt->u_cnt.hci, ":"));
                if (atoi_val < 0) {
                    BPRINT_W("SCRIPT_HCI: atoi for pkt->u_cnt.hci error");
                    continue;
                }
                boots_set_btif(p, atoi_val);
                strncpy(mc, BOOTS_MULTICHIP, sizeof(BOOTS_MULTICHIP));
                strncpy(mc + strlen(BOOTS_MULTICHIP), p, strlen(p));
                if (sizeof(BOOTS_MULTICHIP) + strlen(p) >= IF_NAME_SIZE) {
                    BPRINT_E("%s, name size > 16", __func__);
                    continue;
                } else
                    mc[sizeof(BOOTS_MULTICHIP) + strlen(p)] = '\0';
                BPRINT_D("SCRIPT_HCI: multiple controller interface is \"%s\"", mc);
                multi_chip = true;
                boots_pkt_node_pop(&pkt, NULL, NULL);
                } continue;

            case SCRIPT_END:
                BPRINT_I("Script End");
                boots_pkt_node_pop(&pkt, NULL, NULL);

                boots_script_close();
                sfile.script = NULL;
                script_state = BOOTS_SCRIPT_SUCCESS;
                goto exit;
            default:
                BPRINT_W("Got unexpected type: 0x%X", pkt->s_type);
                boots_pkt_node_pop(&pkt, NULL, NULL);
                break;
            }
        }

        if (len) {
            if (pkt && (pkt->s_type == SCRIPT_STRESS || pkt->s_type == SCRIPT_LOOPBACK || pkt->s_type == SCRIPT_LPTIMER)) {
                if (BOOTS_STRESS_MEASURE_IN_BOOTS)
                    boots_stress_record_timestamp(BOOTS_STRESS_TIMESTAMP_SEND_CMD_START);
            }

            // Send command, if input by user or script
            boots_write(atoi(boots_if.cs), (void *)buf.buf, len, boots_if.csif);
            if (pkt && (pkt->s_type == SCRIPT_STRESS || pkt->s_type == SCRIPT_LOOPBACK || pkt->s_type == SCRIPT_LPTIMER)) {
                if (BOOTS_STRESS_MEASURE_IN_BOOTS)
                    boots_stress_record_timestamp(BOOTS_STRESS_TIMESTAMP_SEND_CMD_FINISH);
                if (BOOTS_STRESS_SHOW_ALL_CMD)
                    boots_pkt_handler(&buf, NULL, 0, pkt->xfer_idx);
                if (pkt->s_type == SCRIPT_LPTIMER)
                    time_start = boots_get_timestamp();
            }
            memset((void *)&buf, 0, sizeof(buf));
            len = 0;
            boots_client_semaphore_take(pdMS_TO_TICKS(mdelay));
        }

        boots_srv_get_event_buffer_content(buf.buf, &len);
        if (buf.buf_len == 0) {
            buf.buf_len = len;
        } else {
            BPRINT_D("(ST)buf.buf_len is %d", buf.buf_len);
        }

        if (len > 0 && boots_if.clif == BOOTS_CLIF_USER) {
            cont = boots_pkt_handler(&buf, pkt ? &pkt : NULL, 1, xfer_idx);

        } else if (len > 0 && boots_if.clif == BOOTS_CLIF_UART) {
            // write to UART(CBT)
            boots_write(atoi(boots_if.cs), (void *)buf.buf, len, boots_if.csif);
            boots_pkt_handler(&buf, NULL, 0, xfer_idx);
        }

        if (pkt && pkt->s_type == SCRIPT_STRESS) {
            if (BOOTS_STRESS_MEASURE_IN_BOOTS)
                boots_stress_record_timestamp(BOOTS_STRESS_TIMESTAMP_RECEIVE_EVENT_FINISH);
        } else if (pkt && (pkt->s_type == SCRIPT_LOOPBACK || pkt->s_type == SCRIPT_LPTIMER)) {
            if (BOOTS_STRESS_MEASURE_IN_BOOTS && !BOOTS_STRESS_MEASURE_LBT_TOTAL_LATENCY)
                boots_stress_record_timestamp(BOOTS_STRESS_TIMESTAMP_RECEIVE_EVENT_FINISH);
            if (buf.buf[0] == HCI_EVENT_PKT && len == 8) {
                // should read again
                len = boots_read(buf.buf, sizeof(buf), boots_if.csif);
            } else if (buf.buf[0] == HCI_EVENT_PKT && buf.buf[8] == HCI_ACL_PKT && len == (8 + pkt->len)) {
                // current loopback test iteration is finished
            } else if (buf.buf[0] == HCI_ACL_PKT) {
                // current loopback test iteration is finished
            } else {
                BPRINT_E("Receive unknonw type %02X, len=%d(EVENT/ACL is expected)", buf.buf[0], (int)len);
                goto exit;
            }
            if (BOOTS_STRESS_MEASURE_IN_BOOTS && BOOTS_STRESS_MEASURE_LBT_TOTAL_LATENCY)
                boots_stress_record_timestamp(BOOTS_STRESS_TIMESTAMP_RECEIVE_EVENT_FINISH);
            if (pkt->s_type == SCRIPT_LPTIMER) {
                time_end = boots_get_timestamp();
                diff_time = time_end - time_start;
                if (boots_loop_timer > 0 && (unsigned int)boots_loop_timer > (diff_time / 1000))
                    vTaskDelay(pdMS_TO_TICKS(boots_loop_timer - (diff_time / 1000)));
            }
        }

        memset((void *)&buf, 0, sizeof(buf));
        len = 0;
        BPRINT_D("%s: cont = %d, sfile.script = %p", __func__, cont, (void *)sfile.script);
    } while (cont || sfile.script);

exit:
    boots_pkt_list_destroy(pkt);
    pkt = NULL;
    boots_client_semaphore_delete();
    BPRINT_D("Input End");

    if (script_state > BOOTS_SCRIPT_NONE) {
        BPRINT_D("final script state = %d", script_state);
        if (script_state != BOOTS_SCRIPT_SUCCESS)
            return -1;
    }

    return 0;
}
//---------------------------------------------------------------------------
