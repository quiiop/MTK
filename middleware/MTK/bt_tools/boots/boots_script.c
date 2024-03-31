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
 * MediaTek Inc. (C) 2016~2017. All rights reserved.
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
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include "boots_pkt.h"
#include "script_slt_simple.h"
#include "script_slt_simple_test.h"
#include "script_loopback_simple.h"

#ifdef MTK_BOOTS_SCRIPT_SLT_FULL
#include "script_slt_full.h"
#endif
#ifdef MTK_BOOTS_SCRIPT_LOOPBACK_FULL
#include "script_loopback_full.h"
#endif

//---------------------------------------------------------------------------
#define LOG_TAG "boots_script"

//---------------------------------------------------------------------------
typedef struct per_cmd_info {
    char *name;
    uint8_t scriptType;
    int nameLen;
    bool (*func)(int, char *, char *, uint8_t *, void **, int *);
} S_per_cmd_info;

typedef struct tr_xfer {
    uint8_t packet[HCI_BUF_SIZE];    /** Save TX/RX packet content */
    uint8_t idx;                     /** For TX/RX index, ex: TX3 */
} tr_xfer_s;

//---------------------------------------------------------------------------
#define FUN_DECLAR(name) \
    static bool FUN_##name(int index, char *prefix, char *content, \
            uint8_t *pktType, void **value, int *value_len)
FUN_DECLAR(TITLE);
FUN_DECLAR(PROC);
FUN_DECLAR(TX);
FUN_DECLAR(RX);
FUN_DECLAR(WAITRX);
FUN_DECLAR(LOOP);
FUN_DECLAR(LOOPEND);
FUN_DECLAR(TIMEOUT);
FUN_DECLAR(WAIT);
FUN_DECLAR(USBALT);
FUN_DECLAR(CMD);
FUN_DECLAR(HCIUSB);
FUN_DECLAR(HCI);
FUN_DECLAR(END);

//---------------------------------------------------------------------------
#define SCRIPTLINE(name) {#name, SCRIPT_##name, sizeof(#name) - 1, FUN_##name}
static S_per_cmd_info script_line_s[] = {
    SCRIPTLINE(TITLE),
    SCRIPTLINE(PROC),
    SCRIPTLINE(TX),
    SCRIPTLINE(RX),
    SCRIPTLINE(WAITRX),
    SCRIPTLINE(LOOPEND),    /* should be ahead of LOOP */
    SCRIPTLINE(LOOP),
    SCRIPTLINE(WAIT),
    SCRIPTLINE(TIMEOUT),
    SCRIPTLINE(USBALT),
    SCRIPTLINE(TIMEOUT),
    SCRIPTLINE(CMD),
    SCRIPTLINE(HCIUSB),
    SCRIPTLINE(HCI),
    SCRIPTLINE(END)
};

//---------------------------------------------------------------------------
static uint8_t q_pos[HCI_BUF_SIZE];
static int script_line_num = sizeof(script_line_s) / sizeof(S_per_cmd_info);
static tr_xfer_s tr_packet;
static int paramValue = 0;
static char *g_line = NULL;

//---------------------------------------------------------------------------
int xatoi(char *p, int len)
{
    int n = 0;
    uint8_t v = 0;

    for (n = 0; n < len; n++) {
        if ((*p >= '0') && (*p <= '9'))
            v = v * 16 + (*p - '0');
        else if ((*p >= 'A') && (*p <= 'F'))
            v = v * 16 + (10 + *p - 'A');
        else if ((*p >= 'a') && (*p <= 'f'))
            v = v * 16 + (10 + *p - 'a');
        else if (*p == '?')
            return -1;
        else
            return 0;
        p++;
    }
    return v;
}

//---------------------------------------------------------------------------
void filter_space_c(char** ppStr, int head_filter, int tail_filter)
{
    char* str;
    int start, end, strLen;

    if (ppStr != NULL && *ppStr != NULL){
        str = *ppStr;
        // 1. head fliter
        if ((head_filter == 1) && (*str != '\0')) {
            for (start = 0; *str == ' ' || *str == '\t'; start++)
               str++;
        }
        // 2. tail filter
        if ((tail_filter == 1) && (*str != '\0')){
            strLen = strlen(str);
            for (end = strLen - 1; end >= 0 && (str[end] == ' ' || str[end] == '\t'); end--)
                str[end] = '\0';
        }
        *ppStr = str;
    }
}

//---------------------------------------------------------------------------
int packet_safe_atoi(char valueStr[], uint8_t value[], int size)
{
    char *token = NULL, *savepStr=NULL;
    uint8_t *valueIs = value;

    int i = 0, repeat = 0, split_num = 0, len = 0;
    int num = 0;
    int ret = 0;

    if (size > HCI_BUF_SIZE) {
        BPRINT_W("Incorrect size %d", size);
        size = HCI_BUF_SIZE;
    }

    //Titan: remove memset to reduce CPU time
    //memset(q_pos, 0, HCI_BUF_SIZE);
    //memset(valueIs, 0, size);
    token=strtok_r(valueStr, " ", &savepStr);
    while(token != NULL){
        len = strlen(token);
        repeat = len/2 + len%2;

        do{
            split_num = ((len >= 2)?(2):(1));
            ret = xatoi((token + i * 2), split_num);
            if (num >= HCI_BUF_SIZE || num >= size || num < 0) {
                BPRINT_E("num(%d) index is over!!(%d or %d)", num, HCI_BUF_SIZE, size);
                return 0;
            }

            if (ret < 0) {
                /* for this position is question mark */
                q_pos[num] = 1;
                valueIs[num] = 0;
            } else {
                q_pos[num] = 0;
                valueIs[num] = ret;
            }
            ++num;
            ++i;
            len -= i*2;
        }while((--repeat) > 0);
        memcpy(valueIs + num, q_pos, num);
        if(num > 0){
            token=strtok_r(NULL, " ", &savepStr);
            i = 0;
        }
    }
/*
    if(num != 0){
        int i = 0, tmpN = num;
        printf("packet[num==%d]>", num);
        while(tmpN){
            printf("%02x", valueIs[i]);
            printf(" ");
            --tmpN;
            ++i;
        }
        printf("\n");
    }
*/
    return num;
}

//---------------------------------------------------------------------------
static bool FUN_TITLE(int index, char *prefix, char *content, uint8_t *pktType, void **value, int *value_len)
{
    UNUSED(prefix);
    *pktType = 0;
    *value_len = ((content == NULL)?0:(strlen(content) + 1));
    *value = (void*)((*value_len == 0)?NULL:content);

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_TITLE(), index is not in range");
        return false;
    }

    BPRINT_D("LinePrefix:%s ScriptType:0x%02X, PktType:0x%02X, \"%s\", len:%d",
            script_line_s[index].name, script_line_s[index].scriptType, *pktType,
            (char *)*value, *value_len);
    return true;
}

//---------------------------------------------------------------------------
static bool FUN_PROC(int index, char *prefix, char *content, uint8_t *pktType, void **value, int *value_len)
{
    UNUSED(prefix);
    *pktType = 0;
    *value_len = ((content == NULL)?0:(strlen(content) + 1));
    *value = (void*)((*value_len == 0)?NULL:content);

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_PROC(), index is not in range");
        return false;
    }

    BPRINT_D("LinePrefix:%s ScriptType:0x%02X, PktType:0x%02X, \"%s\", len:%d",
            script_line_s[index].name, script_line_s[index].scriptType, *pktType,
            (char *)*value, *value_len);
    return true;
}

//---------------------------------------------------------------------------
static bool FUN_HCIUSB(int index, char *prefix, char *content, uint8_t *pktType,
        void **value, int *value_len)
{
    UNUSED(prefix);
    UNUSED(content);
    UNUSED(pktType);
    UNUSED(value);
    UNUSED(value_len);

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_HCIUSB(), index is not in range");
        return false;
    }

    // This syntax is only for combo tool in non-relay mode to USB interface
    BPRINT_W("%s: Ignore \"%s\"(0x%02X), it only works on ComboTool",
            __func__, script_line_s[index].name, script_line_s[index].scriptType);
    return false;
}

//---------------------------------------------------------------------------
static bool FUN_HCI(int index, char *prefix, char *content, uint8_t *pktType,
        void **value, int *value_len)
{
    // Parsing for HIF
    static char ifmap[16] = {'\0'};
    unsigned char cnt = 0;

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_HCI(), index is not in range");
        return false;
    }
    if (prefix == NULL) {
        BPRINT_E("FUN_HCI(), prefix is NULL");
        return false;
    }
    if (content == NULL) {
        BPRINT_E("FUN_HCI(), content is NULL");
        return false;
    }

    *pktType = 0;
    BPRINT_D("%s: %s %s", __func__, prefix, content);
    cnt = strlen(prefix + strlen(script_line_s[index].name));
    strncpy(ifmap, prefix + strlen(script_line_s[index].name), cnt < 14? cnt: 14);
    strncat(ifmap, ":", 1);
    cnt = cnt + 1 + strlen(content);
    if (cnt > 15) {
        BPRINT_E("%s, content size large", __func__);
        return false;
    }
    strncat(ifmap, content, strlen(content));
    ifmap[cnt] = '\0';
    BPRINT_D("%s", ifmap);
    *value_len = strlen(ifmap) + 1;
    *value = (void *)ifmap;

    BPRINT_D("LinePrefix:%s ScriptType:0x%02X, PktType:0x%02X, \"%s\", len:%d",
            script_line_s[index].name, script_line_s[index].scriptType, *pktType,
            (char *)*value, *value_len);
    return true;
}

//---------------------------------------------------------------------------
bool FUN_TX(int index, char *prefix, char *content, uint8_t *pktType,
        void **value, int *value_len)
{
    BPRINT_D("FUN: %s %s", prefix, content);
    *pktType = (content != NULL) ? xatoi(content, 2) : 0;
    *value_len = 0;
    *value = NULL;

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_TX(), index is not in range");
        return false;
    }

    if (content != NULL) {
        *value_len = packet_safe_atoi(content, tr_packet.packet, HCI_BUF_SIZE);
        if (*value_len > 0) {
            int tmp = 0;
            tmp = atoi(prefix + strlen(script_line_s[index].name));
            if (tmp < 0 || tmp > UINT8_MAX) {
                BPRINT_E("FUN: tr_packet.idx convert to int error");
                return false;
            }
            tr_packet.idx = (uint8_t)tmp;
            BPRINT_D("FUN: T/RX packet idx: %d", tr_packet.idx);
            *value = &tr_packet;
        } else {
            *value_len = 0;
            *value = NULL;
        }
    }

    BPRINT_D("LinePrefix:%s ScriptType:0x%02X, PktType:0x%02X, len:%d",
            script_line_s[index].name, script_line_s[index].scriptType, *pktType,
            *value_len);
    return true;
}

//---------------------------------------------------------------------------
static bool FUN_RX(int index, char *prefix, char *content, uint8_t *pktType,
        void **value, int *value_len)
{
    return FUN_TX(index, prefix, content, pktType, value, value_len);
}

//---------------------------------------------------------------------------
static bool FUN_WAITRX(int index, char *prefix, char *content, uint8_t *pktType, void **value, int *value_len)
{
    return FUN_TX(index, prefix, content, pktType, value, value_len);
}

//---------------------------------------------------------------------------
static bool FUN_LOOP(int index, char *prefix, char *content, uint8_t *pktType, void **value, int *value_len)
{
    UNUSED(prefix);
    *pktType = 0;
    *value_len = 1;
    paramValue = ((*value_len == 0)?(0):(atoi(content)));
    *value = &paramValue;

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_LOOP(), index is not in range");
        return false;
    }

    BPRINT_D("LinePrefix:%s ScriptType:0x%02X, PktType:0x%02X, LoopCount:%d",
            script_line_s[index].name, script_line_s[index].scriptType, *pktType,
            *((int*)(*value)));
    return true;
}

//---------------------------------------------------------------------------
static bool FUN_LOOPEND(int index, char *prefix, char *content, uint8_t *pktType, void **value, int *value_len)
{
    UNUSED(prefix);
    UNUSED(content);
    *pktType = 0;
    *value_len = 0;
    *value = NULL;

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_LOOPEND(), index is not in range");
        return false;
    }

    BPRINT_D("LinePrefix:%s ScriptType:0x%02X, PktType:0x%02X",
            script_line_s[index].name, script_line_s[index].scriptType,
            *pktType);
    return true;
}

//---------------------------------------------------------------------------
static bool FUN_TIMEOUT(int index, char *prefix, char *content, uint8_t *pktType, void **value, int *value_len)
{
    UNUSED(prefix);
    *pktType = 0;
    *value_len = 1;
    paramValue = ((*value_len == 0)?(0):(atoi(content)));
    *value = &paramValue;

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_TIMEOUT(), index is not in range");
        return false;
    }

    BPRINT_D("LinePrefix:%s ScriptType:0x%02X, PktType:0x%02X, timo:%d",
            script_line_s[index].name, script_line_s[index].scriptType, *pktType,
            *((int*)(*value)));
    return true;
}

//---------------------------------------------------------------------------
static bool FUN_WAIT(int index, char *prefix, char *content, uint8_t *pktType, void **value, int *value_len)
{
    UNUSED(prefix);
    *pktType = 0;
    *value_len = ((content == NULL)?0:(strlen(content) + 1));
    paramValue = ((*value_len == 0)?(0):(atoi(content)));
    *value = &paramValue;

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_WAIT(), index is not in range");
        return false;
    }

    BPRINT_D("LinePrefix:%s ScriptType:0x%02X, PktType:0x%02X, wait:%d",
            script_line_s[index].name, script_line_s[index].scriptType, *pktType,
            *((int*)(*value)));
    return true;
}

//---------------------------------------------------------------------------
static bool FUN_USBALT(int index, char *prefix, char *content, uint8_t *pktType, void **value, int *value_len)
{
    return FUN_TX(index, prefix, content, pktType, value, value_len);
/*
    *pktType = 0;
    *value_len = 1;
    paramValue = ((*value_len == 0)?(0):(atoi(content)));
    *value = &paramValue;

    BPRINT_D("LinePrefix:%s ScriptType:0x%02x, PktType:0x%02x, USBALT:%d",
            script_line_s[index].name, script_line_s[index].scriptType, *pktType,
            *((int*)(*value)));
    return true;
*/
}

//---------------------------------------------------------------------------
static bool FUN_CMD(int index, char *prefix, char *content, uint8_t *pktType, void **value, int *value_len)
{
    UNUSED(prefix);
    *pktType = 0x01;
    *value_len = 0;
    *value = NULL;

    if((content != NULL)) {
        *value_len = packet_safe_atoi(content, &tr_packet.packet[1], HCI_BUF_SIZE - 1);
        if (*value_len > 0) {
            tr_packet.packet[0] = HCI_CMD_PKT;
            *value_len += 1;
            *value = &tr_packet;
        }
        else{
            *value_len = 0;
            *value = NULL;
        }
    }

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_CMD(), index is not in range");
        return false;
    }

    BPRINT_D("LinePrefix:%s ScriptType:0x%02X, PktType:0x%02X",
            script_line_s[index].name, script_line_s[index].scriptType, *pktType);
    return true;
}

//---------------------------------------------------------------------------
static bool FUN_END(int index, char *prefix, char *content, uint8_t *pktType, void **value, int *value_len)
{
    UNUSED(prefix);
    UNUSED(content);
    *pktType = 0;
    *value_len = 0;
    *value = NULL;

    if (index < 0 || index >= script_line_num) {
        BPRINT_E("FUN_END(), index is not in range");
        return false;
    }

    BPRINT_D("LinePrefix:%s ScriptType:0x%02X, PktType:0x%02X",
            script_line_s[index].name, script_line_s[index].scriptType, *pktType);
    return true;
}

//---------------------------------------------------------------------------
static pkt_list_s *script_line_handle(char *lineName, char *lineValue)
{
    int i = 0;
    uint8_t xfer_idx = 0;
    static int value_len = 0;
    static void *value = NULL;
    static uint8_t p_type = 0;

    for (i = 0; i < script_line_num; i++) {
        if (!strncmp(lineName, script_line_s[i].name, script_line_s[i].nameLen)) {
            value_len = 0;
            value = NULL;
            p_type = 0;
            if (script_line_s[i].func(i, lineName, lineValue, &p_type, &value, &value_len) == true) {
                switch (script_line_s[i].scriptType) {
                case SCRIPT_TX:
                case SCRIPT_RX:
                case SCRIPT_WAITRX:
                case SCRIPT_USBALT:
                    xfer_idx = ((tr_xfer_s *)value)->idx;
                    break;
                default:
                    break;
                }
                return boots_pkt_node_push(script_line_s[i].scriptType, p_type,
                        value, value_len, xfer_idx, NULL, NULL);
            }
        }
    }
    return NULL;
}

//---------------------------------------------------------------------------
pkt_list_s *script_line_parse(char *line)
{
    char *tmpStr = line;
    char *delimStr = ":=";
    char *subdelimStr = "//";
    char *lineName, *lineValue;
    char *saveptr;
    int slen;

    // 1. filter out space until found out a char
    filter_space_c(&tmpStr, 1, 0);

    // 2. filter out annotation line
    if ((*tmpStr == '#') || (strncmp(tmpStr, "//", 2) == 0)) {
        // do nothing, just ignore

    } else {
        slen = strlen(tmpStr);
        // 3. filter out new line('\n' 0A) & carriage ret ('\r' 0D)
        if (tmpStr[slen - 1] == 0x0A) {
            tmpStr[slen - 1] = '\0';
            if (tmpStr[slen - 2] == 0x0D) {
                tmpStr[slen - 2] = '\0';
            }
        }
        slen = strlen(tmpStr);

        if (slen == 0) {
            // do nothing--only ignore
        } else {
            //BPRINT_D("line <%s>", tmpStr);
            // 4. parse
            lineName = strtok_r(tmpStr, delimStr, &saveptr);
            if (lineName != NULL) {
                lineValue = strtok_r(saveptr, subdelimStr, &lineValue);
            } else {
                return NULL;
            }
            //BPRINT_D("orgName:%s, orgValue:%s", lineName, lineValue);

            // 5. filter out space
            //BPRINT_D("filter out space ...");
            filter_space_c(&lineName, 1, 1);
            filter_space_c(&lineValue, 1, 1);
            //BPRINT_D("newName:%s, newValue:%s", lineName, lineValue);

            // 6. map Type and create node
            return script_line_handle(lineName, lineValue);
        }
    }
    return NULL;
}

//---------------------------------------------------------------------------
unsigned char *boots_script_open(boots_script_type type, uint8_t file_id)
{
    if (type == BOOTS_SCRIPT_SLT) {
#ifdef MTK_BOOTS_SCRIPT_SLT_FULL
        if (file_id == 0)
            return (unsigned char *)___script_slt_full;
        else
            return (unsigned char *)___script_slt_simple_test;
#else
       if (file_id == 0)
            return (unsigned char *)___script_slt_simple;
       else if (file_id == 1)
            return (unsigned char *)___script_slt_simple_test;
       else
            return (unsigned char *)___script_slt_simple;
#endif

    } else if (type == BOOTS_SCRIPT_LOOPBACK) {
#ifdef MTK_BOOTS_SCRIPT_LOOPBACK_FULL
        return (unsigned char *)___script_loopback_full;
#else
        return (unsigned char *)___script_loopback_simple;
#endif

    } else {
        BPRINT_E("%s, type(%d) is not allowed", __func__, type);
        return NULL;
    }
}

//---------------------------------------------------------------------------
void boots_script_close(void)
{
    if (g_line) {
        vPortFree(g_line);
        g_line = NULL;
    }
}

//---------------------------------------------------------------------------
pkt_list_s *boots_script_get(unsigned char **out)
{
    char *ptr = NULL;
    char *begin = (char *)*out;
    pkt_list_s *n = NULL;
    uint32_t line_len = 0;

    if (begin == NULL) return NULL;
    if (!g_line)
        g_line = (char *)pvPortMalloc(HCI_BUF_SIZE);

    do {
        //BPRINT_I("%s: 0x%x, 0x%x, 0x%x", __func__, out, *out, begin);
        //memset(g_line, 0, HCI_BUF_SIZE); //Don't need memset due to we'll add \0 at end.
        ptr = strchr((const char *)begin, '\n');
        if (ptr == NULL) {
            BPRINT_I("%s: end of script", __func__);
            break;
        }
        line_len = ptr - begin + 1;
        if (line_len > HCI_BUF_SIZE) {
            BPRINT_E("script len(%d) is larger thean hci_buf(%d)", (int)line_len, HCI_BUF_SIZE);
            n = NULL;
            break;
        }
        memcpy(g_line, begin, line_len);
        g_line[ptr - begin + 1] = '\0';
        n = script_line_parse(g_line);

        // update
        begin = ++ptr;
        *out = (unsigned char *)begin;
    } while (!n);

    return n;
}
