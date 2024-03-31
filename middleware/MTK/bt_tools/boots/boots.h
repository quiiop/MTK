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
#ifndef __BOOTS_H__
#define __BOOTS_H__

#define OS_FREERTOS

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "boots_cfg.h"
#include "syslog.h"

#ifdef MTK_BOOTS_SLT_ENABLE
#include "memory_attribute.h"
#endif
//---------------------------------------------------------------------------
/** BOOTS_VERSION:
 *  Major is for architecture or big change.,
 *  Minor is for feature add or change.,
 *  Revision is for bugfix.
 */
#define BOOTS_VERSION           "1.1.2_22080901"

#define NONE    "\033[m"
#define RED     "\033[0;32;31m"
#define GREEN   "\033[0;32;32m"
#define YELLOW  "\033[0;32;33m"
#define BLUE    "\033[0;32;34m"

/* change this value to support more interface */
#define MAX_CHIP_NO             4

#define IF_NAME_SIZE           16
#define BUFFER_USING_HEAP      1  // 0:static buffer in sysram 1:Malloc
#ifdef MTK_BOOTS_SLT_ENABLE
#define HCI_BUF_SIZE         3120 //should bigger than the max script len per line.
#define CMD_BUF_LEN          2048
#define EVENT_BUF_LEN        2048
#else
#define HCI_BUF_SIZE         4096
#define CMD_BUF_LEN          2048 // for HW DVT
#define EVENT_BUF_LEN        2048 // for HW DVT
#endif

#if BUFFER_USING_HEAP
#define REMAIN_SIZE          2048
#endif

#define BOOTS_MSG_LVL_DBG       4
#define BOOTS_MSG_LVL_INFO      3
#define BOOTS_MSG_LVL_WARN      2
#define BOOTS_MSG_LVL_ERR       1
#define BOOTS_MSG_LVL_NONE      0

#define BPRINT_D_RAW(fmt, ...) \
    do { if (boots_get_dbg_level() >= BOOTS_MSG_LVL_DBG)  \
        LOG_I(BOOTS, fmt, ##__VA_ARGS__);   } while (0);
#define BPRINT_D(fmt, ...) \
    do { if (boots_get_dbg_level() >= BOOTS_MSG_LVL_DBG)  \
        LOG_I(BOOTS, fmt, ##__VA_ARGS__);   } while (0);
#define BPRINT_I(fmt, ...) \
    do { if (boots_get_dbg_level() >= BOOTS_MSG_LVL_INFO) \
        LOG_I(BOOTS, fmt, ##__VA_ARGS__);   } while (0);
#define BPRINT_W(fmt, ...) \
    do { if (boots_get_dbg_level() >= BOOTS_MSG_LVL_WARN) \
        LOG_W(BOOTS, fmt, ##__VA_ARGS__);   } while (0);
#define BPRINT_E(fmt, ...) \
    do { if (boots_get_dbg_level() >= BOOTS_MSG_LVL_ERR)  \
        LOG_E(BOOTS, fmt, ##__VA_ARGS__);   } while (0);

#define BPRINT(fmt, ...) \
    do { LOG_I(BOOTS, fmt, ##__VA_ARGS__);  } while (0);

#define SHOW_RAW(len, buf) \
    do { \
        if (boots_get_dbg_level() >= BOOTS_MSG_LVL_DBG) { \
            BPRINT("Raw data: "); \
            LOG_HEXDUMP_I(BOOTS, NULL, buf, len); \
        } \
    } while (0);

#define MAX(a, b) \
    (((a) > (b)) ? (a) : (b))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) \
    (sizeof(arr) / sizeof((arr)[0]))
#endif

#define CHECK_USERID() \
    if (getuid() != 0) { \
        BPRINT_E("Please run boots as ROOT!!!"); \
        exit(1); \
    }

#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif

#define STREAM_TO_UINT32(p) \
    (((uint32_t)(*(p))) | (((uint32_t)(*(p + 1))) << 8) | \
     (((uint32_t)(*(p + 2))) << 16) | (((uint32_t)(*(p + 3))) << 24))
#define STREAM_TO_UINT16(p) \
    (((uint16_t)(*(p))) | (((uint16_t)(*(p + 1))) << 8))

#define UINT32_TO_STREAM(p, val) { \
    *(p) = (uint8_t)(val); \
    *(p + 1) = (uint8_t)((val) >> 8); \
    *(p + 2) = (uint8_t)((val) >> 16); \
    *(p + 3) = (uint8_t)((val) >> 24); \
}
#define UINT16_TO_STREAM(p, val) { \
    *(p) = (uint8_t)(val); \
    *(p + 1) = (uint8_t)((val) >> 8); \
}

/** Boots BT Interface */
typedef enum {
    BOOTS_IF_NONE = 0,
    BOOTS_BTIF_STPBT,
    BOOTS_BTIF_HCI,
    BOOTS_BTIF_ALL,
    BOOTS_CSIF_SKT,
    BOOTS_CSIF_UART,
    BOOTS_CSIF_ETH,
    BOOTS_CLIF_USER,
    BOOTS_CLIF_UART,
    BOOTS_CSIF_BUF,
} boots_if_e;

enum{
    BOOTS_SRV_TX,
    BOOTS_SRV_RX,
    BOOTS_SRV_UART,
};

typedef enum {
    BOOTS_SCRIPT_NONE = 0,
    BOOTS_SCRIPT_STARTED,
    BOOTS_SCRIPT_FAIL,
    BOOTS_SCRIPT_SUCCESS,
} boots_script_sts_e;

typedef struct {
    boots_if_e inf; // interface
    char *n;        // name
    char *p;        // path
} boots_btif_s;

typedef struct {
    boots_if_e      btif[MAX_CHIP_NO];  // BT controller interface
    boots_if_e      csif;               // cli/srv communication interface
    boots_if_e      clif;               // cli input interface
    char            bt[MAX_CHIP_NO][IF_NAME_SIZE];  // btif name, useless current.
    char            cs[IF_NAME_SIZE];   // csif name
    char            cli[IF_NAME_SIZE];  // clif name
    int             cs_speed;
    int             cli_speed;
    // file descriptor of BT controller or script HCI idx
    int             btfd[MAX_CHIP_NO];
    uint8_t         file_id;           // [0] use default file [1] another test file
} boots_if_s;

//---------------------------------------------------------------------------
extern boots_btif_s boots_btif[];

//---------------------------------------------------------------------------
// The following msg definition, the size can't more than IF_NAME_SIZE
#define BOOTS_MULTICHIP "boots:mc-"
#define BOOTS_CLIEND    "boots:clientend"
typedef struct {
    uint8_t         buf[HCI_BUF_SIZE];
    char            ctrlif[IF_NAME_SIZE];   // enable this, if multi_chip is true
    uint16_t        buf_len;                // length of buf
    uint16_t        pos;
} boots_buf_s;

typedef struct {
    uint8_t         *buf;
    char            *ctrlif;
    uint16_t        buf_len;
} boots_buf_sp;

//---------------------------------------------------------------------------
void boots_set_dbg_level(uint8_t lvl);
uint8_t boots_get_dbg_level(void);
int boots_main(int argc, char *argv[]);
void boots_srv_main(void *parm);
void boots_get_srv_handle(TaskHandle_t *handle);
bool boots_srv_rx_available(void);

//---------------------------------------------------------------------------
#endif // __BOOTS_H__
